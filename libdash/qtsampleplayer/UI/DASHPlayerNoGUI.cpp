/*
 * DASHPlayerNoGUI.cpp
 *****************************************************************************
 * Copyright (C) 2016
 *
 * Jacques Samain <jsamain@cisco.com>
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#include "DASHPlayerNoGUI.h"
#include <iostream>

using namespace libdash::framework::adaptation;
using namespace libdash::framework::mpd;
using namespace libdash::framework::buffer;
using namespace sampleplayer;
using namespace sampleplayer::renderer;
using namespace sampleplayer::managers;
using namespace dash::mpd;
using namespace std;

DASHPlayerNoGUI::DASHPlayerNoGUI  (int argc, char ** argv, pthread_cond_t *mainCond, bool nodecoding) :
				mainCond	(mainCond),
				isRunning	(true),
				noDecoding	(nodecoding)
{
	InitializeCriticalSection(&this->monitorMutex);

	//	this->SetSettings(0, 0, 0, 0, 0);
	this->videoElement      = NULL;
	this->audioElement      = NULL;
	this->mpd 				= NULL;
	this->url 				= NULL;
	this->adaptLogic	 	= LogicType::RateBased;
	this->isNDN 			= false;
	this->ndnAlpha			= 20;
	this->alphaRate			= 0;
	this->bufferTargetSeconds = 0;
	this->reservoirThreshold = 0;
	this->maxThreshold		= 0;
	this->isBuff			= false;
	this->multimediaManager = new MultimediaManager(this->videoElement, this->audioElement, noDecoding);
	this->multimediaManager->SetFrameRate(25);
	this->multimediaManager->AttachManagerObserver(this);
	this->parseArgs(argc, argv);

	if(this->url == NULL)
	{
		this->isRunning = false;
		pthread_cond_broadcast(mainCond);
		//delete(this);
		return;
	}
	else
	{
		if(this->OnDownloadMPDPressed(string(this->url)))
		{
			this->OnStartButtonPressed(0,0,0,0,0);
		}
		else
		{
			this->isRunning = false;
			pthread_cond_broadcast(mainCond);
		}

	}
}
DASHPlayerNoGUI::~DASHPlayerNoGUI ()
{
	this->multimediaManager->Stop();
	if(this->audioElement)
		this->audioElement->StopPlayback();
	this->audioElement = NULL;

	delete(this->multimediaManager);
	delete(this->audioElement);

	DeleteCriticalSection(&this->monitorMutex);
}

void DASHPlayerNoGUI::OnStartButtonPressed               (int period, int videoAdaptationSet, int videoRepresentation, int audioAdaptationSet, int audioRepresentation)
{
	this->OnSettingsChanged(period,videoAdaptationSet,videoRepresentation, audioAdaptationSet, audioRepresentation);

//	if(!(this->isBuff ?
//			(this->multimediaManager->SetVideoAdaptationLogic((LogicType)this->adaptLogic,this->alphaRate, this->reservoirThreshold, this->maxThreshold) && (this->multimediaManager->SetAudioAdaptationLogic((LogicType)this->adaptLogic, this->alphaRate, this->reservoirThreshold, this->maxThreshold))) : (this->multimediaManager->SetVideoAdaptationLogic((LogicType)this->adaptLogic,this->alphaRate) && (this->multimediaManager->SetAudioAdaptationLogic((LogicType)this->adaptLogic, this->alphaRate)))))

	bool setOk;
	if(this->isBuff)
	{
		switch((LogicType)this->adaptLogic)
		{
			case Bola:
			{
				setOk = this->multimediaManager->SetVideoAdaptationLogic((LogicType)this->adaptLogic,this->alphaRate, this->bufferTargetSeconds, this->maxThreshold);
				setOk = this->multimediaManager->SetAudioAdaptationLogic((LogicType)this->adaptLogic, this->alphaRate, this->bufferTargetSeconds, this->maxThreshold);
				break;
			}
			default:
			{
				setOk = this->multimediaManager->SetVideoAdaptationLogic((LogicType)this->adaptLogic,this->alphaRate, this->reservoirThreshold, this->maxThreshold);
				setOk = this->multimediaManager->SetAudioAdaptationLogic((LogicType)this->adaptLogic, this->alphaRate, this->reservoirThreshold, this->maxThreshold);
				break;
			}

		}
	}
	else
	{
		setOk = this->multimediaManager->SetVideoAdaptationLogic((LogicType)this->adaptLogic,this->alphaRate);
		setOk = this->multimediaManager->SetAudioAdaptationLogic((LogicType)this->adaptLogic, this->alphaRate);
	}

	if(!setOk)
	{
		printf("Error setting Video/Audio adaptation logic...\n");
		return;
	}

	L("DASH PLAYER:	STARTING VIDEO\n");
	this->multimediaManager->Start(this->isNDN, this->ndnAlpha);
}
void DASHPlayerNoGUI::OnStopButtonPressed                ()
{
	this->multimediaManager->Stop();
	this->isRunning = false;
	pthread_cond_broadcast(mainCond);
}
bool DASHPlayerNoGUI::IsRunning							 ()
{
	return this->isRunning;
}
void DASHPlayerNoGUI::parseArgs							 (int argc, char ** argv)
{
	if(argc == 1)
	{
		helpMessage(argv[0]);
		return;
	}
	else
	{
		int i = 0;
		while(i < argc)
		{
			if(!strcmp(argv[i],"-u"))
			{
				this->url = argv[i+1];
				i++;
				i++;
				continue;
			}
			if(!strcmp(argv[i],"-n"))
			{
				this->isNDN = true;
				i++;
				continue;
			}
			if(!strcmp(argv[i],"-nr"))
			{
				this->isNDN = true;
				this->ndnAlpha = atof(argv[i+1]);
				i++;
				i++;
				continue;
						}
			if(!strcmp(argv[i], "-b"))
			{
				this->adaptLogic = LogicType::BufferBased;
				this->isBuff = true;
				this->reservoirThreshold = atoi(argv[i+1]);
				this->maxThreshold = atoi(argv[i+2]);
				i = i + 3;
				continue;
			}
			if(!strcmp(argv[i], "-br"))
			{
				this->adaptLogic = LogicType::BufferRateBased;
				this->isBuff = true;
				this->alphaRate = atof(argv[i+1]);
				this->reservoirThreshold = atoi(argv[i+2]);
				this->maxThreshold = atoi(argv[i+3]);
				i = i + 3;
				printf("BufferRateBased chosen, next arg is: %s\n", argv[i]);
				continue;
			}
			if(!strcmp(argv[i], "-bola"))
			{
				this->adaptLogic = LogicType::Bola;
				this->isBuff = true;
				this->alphaRate = atof(argv[i+1]);
				this->bufferTargetSeconds = atoi(argv[i+2]);
				i = i + 2;
				printf("Bola chosen, next arg is: %s\n", argv[i]);
				continue;
			}
			if(!strcmp(argv[i], "-bt"))
			{
				this->adaptLogic = LogicType::BufferBasedThreeThreshold;
				this->isBuff = true;
				this->alphaRate = atoi(argv[i+1]);
				this->reservoirThreshold = atoi(argv[i+2]);
				this->maxThreshold = atoi(argv[i+3]);
				i = i + 3;
				continue;
			}
			if(!strcmp(argv[i], "-r"))
			{
				this->adaptLogic = LogicType::RateBased;
				this->alphaRate = atof(argv[i+1]);
				i = i + 2;
				continue;
			}
			if(!strcmp(argv[i], "-p"))
			{
				this->adaptLogic = LogicType::Panda;
				this->alphaRate = atof(argv[i+1]);
				i = i + 2;
				continue;
			}

			if(!strcmp(argv[i],"-a"))
			{
				int j =0;
				for(j = 0; j < LogicType::Count; j++)
				{
					if(!strcmp(LogicType_string[j],argv[i+1]))
					{
						this->adaptLogic = (LogicType)j;
						break;
					}
				}
				if(j == LogicType::Count)
				{
					printf("the different adaptation logics implemented are:\n");
					for(j = 0;j < LogicType::Count; j++)
					{
						printf("*%s\n",LogicType_string[j]);
					}
					printf("By default, the %s logic is selected.\n", LogicType_string[this->adaptLogic]);
				}
				i++;
				i++;
				continue;
			}
			i++;
		}
	}
}

void DASHPlayerNoGUI::helpMessage						 (char * name)
{
	printf("Usage: %s -u url -a adaptationLogic -n\n" \
			"-u:\tThe MPD's url\n" \
			"-a:\tThe adaptationLogic:\n" \
			"\t*AlwaysLowest\n" \
			"\t*RateBased(default)\n" \
			"\t*BufferBased\n" \
			"-n:\tFlag to use NDN instead of TCP\n" \
			"-nr alpha:\tFlag to use NDN instead of TCP\n" \
			"-b reservoirThreshold maxThreshold (both in %)\n" \
			"-br alpha reservoirThreshold maxThreshold \n" \
			"-r alpha\n", name);
}
void DASHPlayerNoGUI::OnSettingsChanged                  (int period, int videoAdaptationSet, int videoRepresentation, int audioAdaptationSet, int audioRepresentation)
{
	if(this->multimediaManager->GetMPD() == NULL)
		return; // TODO dialog or symbol that indicates that error

	if (!this->SettingsChanged(period, videoAdaptationSet, videoRepresentation, audioAdaptationSet, audioRepresentation))
		return;

	IPeriod                         *currentPeriod      = this->multimediaManager->GetMPD()->GetPeriods().at(period);
	std::vector<IAdaptationSet *>   videoAdaptationSets = AdaptationSetHelper::GetVideoAdaptationSets(currentPeriod);
	std::vector<IAdaptationSet *>   audioAdaptationSets = AdaptationSetHelper::GetAudioAdaptationSets(currentPeriod);

	if (videoAdaptationSet >= 0 && videoRepresentation >= 0 && !videoAdaptationSets.empty())
	{
		this->multimediaManager->SetVideoQuality(currentPeriod,
				videoAdaptationSets.at(videoAdaptationSet),
				videoAdaptationSets.at(videoAdaptationSet)->GetRepresentation().at(videoRepresentation));
	}
	else
	{
		this->multimediaManager->SetVideoQuality(currentPeriod, NULL, NULL);
	}

	if (audioAdaptationSet >= 0 && audioRepresentation >= 0 && !audioAdaptationSets.empty())
	{
		this->multimediaManager->SetAudioQuality(currentPeriod,
				audioAdaptationSets.at(audioAdaptationSet),
				audioAdaptationSets.at(audioAdaptationSet)->GetRepresentation().at(audioRepresentation));
	}
	else
	{
		this->multimediaManager->SetAudioQuality(currentPeriod, NULL, NULL);
	}
}
void DASHPlayerNoGUI::OnEOS								()
{
	this->OnStopButtonPressed();
}
bool DASHPlayerNoGUI::OnDownloadMPDPressed               (const std::string &url)
{
	if(this->isNDN)
	{
		if(!this->multimediaManager->InitNDN(url))
		{
			printf("Problem parsing the mpd. NDN is enabled.");
			return false;
		}
	}
	else
	{
		if(!this->multimediaManager->Init(url))
		{
			printf("Problem parsing the mpd. NDN is disabled.");
			return false; // TODO dialog or symbol that indicates that error
		}
	}
	return true;
}
bool DASHPlayerNoGUI::SettingsChanged                    (int period, int videoAdaptationSet, int videoRepresentation, int audioAdaptationSet, int audioRepresentation)
{
	return true;
	/*	EnterCriticalSection(&this->monitorMutex);

	bool settingsChanged = false;

	if (this->currentSettings.videoRepresentation != videoRepresentation ||
			this->currentSettings.audioRepresentation != audioRepresentation ||
			this->currentSettings.videoAdaptationSet != videoAdaptationSet ||
			this->currentSettings.audioAdaptationSet != audioAdaptationSet ||
			this->currentSettings.period != period)
		settingsChanged = true;

	if (settingsChanged)
		this->SetSettings(period, videoAdaptationSet, videoRepresentation, audioAdaptationSet, audioRepresentation);

	LeaveCriticalSection(&this->monitorMutex);

	return settingsChanged;
	 */
}
void DASHPlayerNoGUI::SetSettings                        (int period, int videoAdaptationSet, int videoRepresentation, int audioAdaptationSet, int audioRepresentation)
{
	this->currentSettings.period                = period;
	this->currentSettings.videoAdaptationSet    = videoAdaptationSet;
	this->currentSettings.videoRepresentation   = videoRepresentation;
	this->currentSettings.audioAdaptationSet    = audioAdaptationSet;
	this->currentSettings.audioRepresentation   = audioRepresentation;
}

void DASHPlayerNoGUI::OnVideoBufferStateChanged          (uint32_t fillstateInPercent)
{

}
void DASHPlayerNoGUI::OnVideoSegmentBufferStateChanged   (uint32_t fillstateInPercent)
{

}
void DASHPlayerNoGUI::OnAudioBufferStateChanged          (uint32_t fillstateInPercent)
{

}
void DASHPlayerNoGUI::OnAudioSegmentBufferStateChanged   (uint32_t fillstateInPercent)
{

}
