/*
 * Panda.cpp
 *****************************************************************************
 * Based on paper "Probe and Adapt: Adaptation for HTTP Video Streaming At Scale", Zhi Li et al.
 *
 * Copyright (C) 2016,
 * Jacques Samain <jsamain@cisco.com>
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#include "Panda.h"
#include<stdio.h>


using namespace dash::mpd;
using namespace libdash::framework::adaptation;
using namespace libdash::framework::input;
using namespace libdash::framework::mpd;

PandaAdaptation::PandaAdaptation(IMPD *mpd, IPeriod *period, IAdaptationSet *adaptationSet, bool isVid, double arg1) :
                          AbstractAdaptationLogic   (mpd, period, adaptationSet, isVid)
{
//	this->param_Alpha = ((arg1 < 1) ? ((arg1 >= 0) ? arg1 : 0.8) : 0.8);
	this->param_Alpha = 0.2;
	this->param_Beta = 0.2;
	this->param_Bmin = 44;
	this->param_K = 0.14;
	this->param_W = 0.3;
	this->param_Epsilon = 0.15;
	this->segmentDuration = 2.0;
	this->averageBw = 0;
	this->bufferLevel = 0;
	this->downloadTime = 0.0;
	this->interTime = 0.0;
	this->isVideo = isVid;
	this->mpd = mpd;
	this->adaptationSet = adaptationSet;
	this->period = period;
	this->targetBps = 0;
	this->targetInterTime = 0.0;
	this->multimediaManager = NULL;
	this->representation = NULL;
	this->currentBitrate = 0;
	L("Panda parameters: K= %f, Bmin = %f, alpha = %f, beta = %f, W = %f\n", param_K, param_Bmin, param_Alpha, param_Beta, param_W);
}

PandaAdaptation::~PandaAdaptation() {
}

LogicType		PandaAdaptation::GetType             ()
{
	return adaptation::Panda;
}

bool			PandaAdaptation::IsUserDependent		()
{
   	return false;
}

bool			PandaAdaptation::IsRateBased			()
{
   	return true;
}

bool			PandaAdaptation::IsBufferBased		()
{
  	return true;
}

void			PandaAdaptation::SetMultimediaManager (sampleplayer::managers::IMultimediaManagerBase *_mmManager)
{
   	this->multimediaManager = _mmManager;
}

void			PandaAdaptation::NotifyBitrateChange	()
{
	if(this->multimediaManager->IsStarted() && !this->multimediaManager->IsStopping())
	if(this->isVideo)
		this->multimediaManager->SetVideoQuality(this->period, this->adaptationSet, this->representation);
	else
		this->multimediaManager->SetAudioQuality(this->period, this->adaptationSet, this->representation);
}

uint64_t		PandaAdaptation::GetBitrate				()
{
	return this->currentBitrate;
}

void		PandaAdaptation::Quantizer				()
{
	this->deltaUp = param_W + (double)this->averageBw * this->param_Epsilon;
	this->deltaDown = param_W;

	size_t imin = 0, imax = 0;
	uint64_t bitrateMin, bitrateMax;
	std::vector<IRepresentation *> representations;
	representations = this->adaptationSet->GetRepresentation();
	for(size_t i= 0; i < representations.size(); i++)
	{
		if(representations.at(i)->GetBandwidth() > this->averageBw - this->deltaDown)
		{
			if(i > 0)
				imin = i - 1;
			break;
		}
	}
	for(size_t i= 0; i < representations.size(); i++)
	{
		if(representations.at(i)->GetBandwidth() > this->averageBw - this->deltaUp)
		{
			if(i > 0)
				imax = i - 1;
			break;
		}
	}

	bitrateMin = representations.at(imin)->GetBandwidth();
	bitrateMax = representations.at(imax)->GetBandwidth();
	if(this->currentBitrate < bitrateMax)
	{
		L("ADAPTATION_LOGIC:\tPanda\tFor %s:\tBW_estimation(ewma): %lu, choice: %lu\n", (this->isVideo ? "video" : "audio"), this->averageBw, imax);
		this->representation = representations.at(imax);
		this->currentBitrate = representations.at(imax)->GetBandwidth();
		this->current = imax;
	}
	else if(this->currentBitrate <= bitrateMin && this->currentBitrate >= bitrateMax)
	{
		L("ADAPTATION_LOGIC:\tPanda\tFor %s:\tBW_estimation(ewma): %lu, choice: %lu\n", (this->isVideo ? "video" : "audio"), this->averageBw, imin);
		this->representation = representations.at(imin);
		this->currentBitrate = representations.at(imin)->GetBandwidth();
		this->current = imin;
	}
	else
	{
		L("ADAPTATION_LOGIC:\tPanda\tFor %s:\tBW_estimation(ewma): %lu, choice: %lu\n", (this->isVideo ? "video" : "audio"), this->averageBw, current);
	}
}
void 		PandaAdaptation::SetBitrate				(uint64_t bps)
{
	std::vector<IRepresentation *> representations;
	representations = this->adaptationSet->GetRepresentation();
	size_t i = 0;
	interTime = targetInterTime > downloadTime ? targetInterTime : downloadTime;
	//printf("targetIT: %f, downloadTime: %f, interTime: %f\n", targetInterTime, downloadTime, interTime);
	//printf("targetBps: %lu, paramk: %f, paramW : %f, bps: %lu, sub: %d\n", targetBps, param_K, param_W, bps, (int)((long int) targetBps - (long int)bps));
	Debug("targetIT: %f, downloadTime: %f, interTime: %f\n", targetInterTime, downloadTime, interTime);
	Debug("targetBps: %lu, paramk: %f, paramW : %f, bps: %lu, sub: %d\n", targetBps, param_K, param_W, bps, (int)((long int) targetBps - (long int)bps));
	if(targetBps)
		targetBps = targetBps + param_K * interTime * (param_W - (((long int)targetBps - (long int)bps) > 0 ? (long int)targetBps - (long int)bps : 0));
	else
		targetBps = bps;

	//printf("result: %lu\n", targetBps);
	Debug("result: %lu\n", targetBps);
	//this->ewma(targetBps);

	if(this->averageBw)
	    this->averageBw = this->averageBw - this->param_Alpha * interTime * ((long int)this->averageBw - (long int)targetBps);
	else
	    this->averageBw = targetBps;
/*	for(i = 0;i < representations.size();i++)
	{
		if(representations.at(i)->GetBandwidth() > this->averageBw - this->param_W)
		{
			if(i > 0)
				i--;
			break;
        }
    }
	if((size_t)i == (size_t)(representations.size()))
		i = i-1;

	L("ADAPTATION_LOGIC:\tPanda\tFor %s:\tBW_estimation(ewma): %lu, choice: %lu\n", (this->isVideo ? "video" : "audio"), this->averageBw, i);
	this->representation = representations.at(i);
	this->currentBitrate = this->representation->GetBandwidth();
*/

	this->Quantizer();
	//printf("calculating targetIT: currentBitrate: %lu, segmentDuration %f, averageBw %lu, parambeta %f, bufferlevel %lu, parambmin %f\n", currentBitrate, segmentDuration, averageBw, param_Beta, bufferLevel, param_Bmin);
	L("calculating targetIT: currentBitrate: %lu, segmentDuration %f, averageBw %lu, parambeta %f, bufferlevel %u, parambmin %f\n", currentBitrate, segmentDuration, averageBw, param_Beta, bufferLevel, param_Bmin);
	if(this->averageBw)
		targetInterTime = (int)this->currentBitrate * segmentDuration / (int)this->averageBw + param_Beta * ((int)bufferLevel - (int)param_Bmin);
	else
		targetInterTime = 0.0;
	targetInterTime = (targetInterTime > 0) ? targetInterTime : 0.0;
	this->multimediaManager->SetTargetDownloadingTime(this->isVideo, targetInterTime);
}

void			PandaAdaptation::BitrateUpdate			(uint64_t bps)
{
	//printf("Panda adaptation: speed received: %lu\n", bps);
	L("Panda adaptation: speed received: %lu\n", bps);

	this->SetBitrate(bps);
	this->NotifyBitrateChange();
}

void 			PandaAdaptation::DLTimeUpdate		(double time)
{
	this->downloadTime = time;
}

void			PandaAdaptation::ewma					(uint64_t bps)
{
	if(averageBw)
	{
		averageBw = param_Alpha*averageBw + (1-param_Alpha)*bps;
	}
	else
	{
		averageBw = bps;
	}
}

void 			PandaAdaptation::OnEOS					(bool value)
{
}

void			PandaAdaptation::CheckedByDASHReceiver	()
{
}

void			PandaAdaptation::BufferUpdate			(uint32_t bufferfill)
{
	this->bufferLevel = bufferfill;
}
