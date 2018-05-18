/*
 * DASHReceiver.cpp
 *****************************************************************************
 * Copyright (C) 2012, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#include "DASHReceiver.h"
#include<stdio.h>

using namespace libdash::framework::input;
using namespace libdash::framework::buffer;
using namespace libdash::framework::mpd;
using namespace dash::mpd;

using duration_in_seconds = std::chrono::duration<double, std::ratio<1, 1> >;

DASHReceiver::DASHReceiver          (IMPD *mpd, IDASHReceiverObserver *obs, MediaObjectBuffer *buffer, uint32_t bufferSize,bool ndnEnabled, double ndnAlpha) :
              mpd                   (mpd),
              period                (NULL),
              adaptationSet         (NULL),
              representation        (NULL),
              adaptationSetStream   (NULL),
              representationStream  (NULL),
              segmentNumber         (0),
              observer              (obs),
              buffer                (buffer),
              bufferSize            (bufferSize),
              isBuffering           (false),
			  withFeedBack			(false),
			  isNDN					(ndnEnabled),
			  ndnAlpha				(ndnAlpha),
			  previousQuality		(0),
			  isPaused				(false),
			  threadComplete		(false),
			  isScheduledPaced		(false),
			  targetDownload		(0.0),
			  downloadingTime		(0.0)
{
    this->period                = this->mpd->GetPeriods().at(0);
    this->adaptationSet         = this->period->GetAdaptationSets().at(0);
    this->representation        = this->adaptationSet->GetRepresentation().at(0);

    this->adaptationSetStream   = new AdaptationSetStream(mpd, period, adaptationSet);
    this->representationStream  = adaptationSetStream->GetRepresentationStream(this->representation);
    this->segmentOffset         = CalculateSegmentOffset();

    this->conn = NULL;
    this->initConn = NULL;
    if(isNDN)
    {
#ifdef NDNICPDOWNLOAD
    	this->conn = new NDNConnection(this->ndnAlpha);
    	this->initConn = new NDNConnection(this->ndnAlpha);
#else
    	this->conn = new NDNConnectionConsumerApi(this->ndnAlpha);
    	this->initConn = new NDNConnectionConsumerApi(this->ndnAlpha);
#endif
    }
    InitializeCriticalSection(&this->monitorMutex);
    InitializeCriticalSection(&this->monitorPausedMutex);
    InitializeConditionVariable(&this->paused);
}
DASHReceiver::~DASHReceiver		()
{
    delete this->adaptationSetStream;
    DeleteCriticalSection(&this->monitorMutex);
    DeleteCriticalSection(&this->monitorPausedMutex);
    DeleteConditionVariable(&this->paused);
}

void			DASHReceiver::SetAdaptationLogic(adaptation::IAdaptationLogic *_adaptationLogic)
{
	this->adaptationLogic = _adaptationLogic;
	this->withFeedBack = this->adaptationLogic->IsRateBased();
}
bool			DASHReceiver::Start						()
{
    if(this->isBuffering)
        return false;

    this->isBuffering       = true;
    this->bufferingThread   = CreateThreadPortable (DoBuffering, this);

    if(this->bufferingThread == NULL)
    {
        this->isBuffering = false;
        return false;
    }

    return true;
}
void			DASHReceiver::Stop						()
{
    if(!this->isBuffering)
        return;

    this->isBuffering = false;
    this->buffer->SetEOS(true);

    if(this->bufferingThread != NULL)
    {
        JoinThread(this->bufferingThread);
        DestroyThreadPortable(this->bufferingThread);
    }
}
MediaObject*	DASHReceiver::GetNextSegment	()
{
    ISegment *seg = NULL;

    EnterCriticalSection(&this->monitorPausedMutex);
    	while(this->isPaused)
    		SleepConditionVariableCS(&this->paused, &this->monitorPausedMutex, INFINITE);

    if(this->segmentNumber >= this->representationStream->GetSize())
    {
    	LeaveCriticalSection(&this->monitorPausedMutex);
    	return NULL;
    }
    seg = this->representationStream->GetMediaSegment(this->segmentNumber + this->segmentOffset);

    if (seg != NULL)
    {
    	std::vector<IRepresentation *> rep = this->adaptationSet->GetRepresentation();
    	int quality = 0;
    	while(quality != rep.size() - 1)
    	{
    		if(rep.at(quality) == this->representation)
    			break;
    		quality++;
    	}
    	Debug("DASH receiver: Next segment is: %s / %s\n",((dash::network::IChunk*)seg)->Host().c_str(),((dash::network::IChunk*)seg)->Path().c_str());
    	L("DASH_Receiver:\t%s\t%d\n", ((dash::network::IChunk*)seg)->Path().c_str() ,quality);
    												//this->representation->GetBandwidth());
    	if(quality != previousQuality)
    	{
    		L("DASH_Receiver:\tQUALITY_SWITCH\t%s\t%d\t%d\n", (quality > previousQuality) ?"UP" : "DOWN", previousQuality, quality);
    		previousQuality = quality;
    	}
        MediaObject *media = new MediaObject(seg, this->representation,this->withFeedBack);
        this->segmentNumber++;
       	LeaveCriticalSection(&this->monitorPausedMutex);
        return media;
    }
   	LeaveCriticalSection(&this->monitorPausedMutex);
    return NULL;
}
MediaObject*	DASHReceiver::GetSegment		(uint32_t segNum)
{
    ISegment *seg = NULL;

    if(segNum >= this->representationStream->GetSize())
        return NULL;

    seg = this->representationStream->GetMediaSegment(segNum + segmentOffset);

    if (seg != NULL)
    {
        MediaObject *media = new MediaObject(seg, this->representation);
        return media;
    }

    return NULL;
}
MediaObject*	DASHReceiver::GetInitSegment	()
{
    ISegment *seg = NULL;

    seg = this->representationStream->GetInitializationSegment();

    if (seg != NULL)
    {
        MediaObject *media = new MediaObject(seg, this->representation);
        return media;
    }

    return NULL;
}
MediaObject*	DASHReceiver::FindInitSegment	(dash::mpd::IRepresentation *representation)
{
    if (!this->InitSegmentExists(representation))
        return NULL;

    return this->initSegments[representation];
}
uint32_t                    DASHReceiver::GetPosition               ()
{
    return this->segmentNumber;
}
void                        DASHReceiver::SetPosition               (uint32_t segmentNumber)
{
    // some logic here

    this->segmentNumber = segmentNumber;
}
void                        DASHReceiver::SetPositionInMsecs        (uint32_t milliSecs)
{
    // some logic here

    this->positionInMsecs = milliSecs;
}
void                        DASHReceiver::SetRepresentation         (IPeriod *period, IAdaptationSet *adaptationSet, IRepresentation *representation)
{
    EnterCriticalSection(&this->monitorMutex);

    bool periodChanged = false;

    if (this->representation == representation)
    {
        LeaveCriticalSection(&this->monitorMutex);
        return;
    }

    this->representation = representation;

    if (this->adaptationSet != adaptationSet)
    {
        this->adaptationSet = adaptationSet;

        if (this->period != period)
        {
            this->period = period;
            periodChanged = true;
        }

        delete this->adaptationSetStream;
        this->adaptationSetStream = NULL;

        this->adaptationSetStream = new AdaptationSetStream(this->mpd, this->period, this->adaptationSet);
    }

    this->representationStream  = this->adaptationSetStream->GetRepresentationStream(this->representation);
    this->DownloadInitSegment(this->representation);

    if (periodChanged)
    {
        this->segmentNumber = 0;
        this->CalculateSegmentOffset();
    }

    LeaveCriticalSection(&this->monitorMutex);
}

libdash::framework::adaptation::IAdaptationLogic* DASHReceiver::GetAdaptationLogic	()
{
	return this->adaptationLogic;
}
dash::mpd::IRepresentation* DASHReceiver::GetRepresentation         ()
{
    return this->representation;
}
uint32_t                    DASHReceiver::CalculateSegmentOffset    ()
{
    if (mpd->GetType() == "static")
        return 0;

    uint32_t firstSegNum = this->representationStream->GetFirstSegmentNumber();
    uint32_t currSegNum  = this->representationStream->GetCurrentSegmentNumber();
    uint32_t startSegNum = currSegNum - 2*bufferSize;

    return (startSegNum > firstSegNum) ? startSegNum : firstSegNum;
}
void                        DASHReceiver::NotifySegmentDownloaded   ()
{
    this->observer->OnSegmentDownloaded();
}
void						DASHReceiver::NotifyBitrateChange(dash::mpd::IRepresentation *representation)
{
	if(this->representation != representation)
	{
		this->representation = representation;
		this->SetRepresentation(this->period,this->adaptationSet,this->representation);
	}
}
void                        DASHReceiver::DownloadInitSegment    (IRepresentation* rep)
{
    if (this->InitSegmentExists(rep))
        return;

    MediaObject *initSeg = NULL;
    initSeg = this->GetInitSegment();

    if (initSeg)
    {
        initSeg->StartDownload(this->initConn);
        this->initSegments[rep] = initSeg;
        initSeg->WaitFinished(true);
    }
}
bool                        DASHReceiver::InitSegmentExists      (IRepresentation* rep)
{
    if (this->initSegments.find(rep) != this->initSegments.end())
        return true;

    return false;
}

void					DASHReceiver::Notifybps					(uint64_t bps)
{
	if(this)
	{
		if(this->adaptationLogic)
		{
			if(this->withFeedBack)
			{
				this->adaptationLogic->BitrateUpdate(bps);
			}
		}
	}
}
void					DASHReceiver::NotifyDlTime				(double time)
{
	if(this)
	{
		if(this->adaptationLogic)
		{
			if(this->withFeedBack)
			{
				this->adaptationLogic->DLTimeUpdate(time);
			}
		}
	}
}
void					DASHReceiver::NotifyCheckedAdaptationLogic()
{
	this->adaptationLogic->CheckedByDASHReceiver();
}
//Is only called when this->adaptationLogic->IsBufferBased
void 					DASHReceiver::OnSegmentBufferStateChanged(uint32_t fillstateInPercent)
{
	this->adaptationLogic->BufferUpdate(fillstateInPercent);
}
void					DASHReceiver::OnEOS(bool value)
{
	this->adaptationLogic->OnEOS(value);
}
/* Thread that does the buffering of segments */
void*                       DASHReceiver::DoBuffering               (void *receiver)
{
    DASHReceiver *dashReceiver = (DASHReceiver *) receiver;

    dashReceiver->DownloadInitSegment(dashReceiver->GetRepresentation());

    MediaObject *media = dashReceiver->GetNextSegment();
    dashReceiver->NotifyCheckedAdaptationLogic();
    media->SetDASHReceiver(dashReceiver);
    std::chrono::time_point<std::chrono::system_clock> m_start_time = std::chrono::system_clock::now();
    while(media != NULL && dashReceiver->isBuffering)
    {
    	if(dashReceiver->isScheduledPaced)
    	{
    		double delay = std::chrono::duration_cast<duration_in_seconds>(std::chrono::system_clock::now() - m_start_time).count();
    		if(delay < dashReceiver->targetDownload)
    		{
    			sleep(dashReceiver->targetDownload - delay);
    		}
    	}
    	m_start_time = std::chrono::system_clock::now();
        media->StartDownload(dashReceiver->conn);
        media->WaitFinished(false);
        if (!dashReceiver->buffer->PushBack(media))
        {
        	dashReceiver->threadComplete = true;
        	return NULL;
        }

        dashReceiver->NotifySegmentDownloaded();
        media = dashReceiver->GetNextSegment();
        dashReceiver->NotifyCheckedAdaptationLogic();
        if(media)
        	media->SetDASHReceiver(dashReceiver);
    }

    dashReceiver->buffer->SetEOS(true);
    dashReceiver->threadComplete = true;
    return NULL;
}

void					DASHReceiver::ShouldAbort				()
{
	Debug("DASH RECEIVER SEGMENT --\n");
	this->segmentNumber--;
	Debug("DASH RECEIVER ABORT REQUEST\n");
	this->buffer->ShouldAbort();
}
void					DASHReceiver::Seeking					(int offset)
{
	EnterCriticalSection(&this->monitorPausedMutex);
	this->isPaused = true;
	this->ShouldAbort();
	this->buffer->Clear();
	this->segmentNumber = this->segmentNumber + offset;
	if((int)this->segmentNumber < 0)
		this->segmentNumber = 0;
	this->isPaused = false;
	WakeAllConditionVariable(&this->paused);
	LeaveCriticalSection(&this->monitorPausedMutex);
}

void					DASHReceiver::FastForward				()
{
	//Seeking 30s in the future (15 segments = 2 x 15 seconds)
	this->Seeking(15);
}

void					DASHReceiver::FastRewind				()
{
	this->Seeking(-15);
    if(this->threadComplete)
    {
    	this->buffer->SetEOS(false);
        this->isBuffering       = true;
        this->bufferingThread   = CreateThreadPortable (DoBuffering, this);
        this->threadComplete = false;
    }
}

void					DASHReceiver::SetTargetDownloadingTime	(double target)
{
	this->isScheduledPaced = true;
	this->targetDownload = target;
}
