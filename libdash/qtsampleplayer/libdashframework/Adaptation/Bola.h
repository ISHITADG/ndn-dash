/*
 * Bola.h
 *****************************************************************************
 * Copyright (C) 2016,
 * Michele Tortelli and Jacques Samain, <michele.tortelli@telecom-paristech.fr>, <jsamain@cisco.com>
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#ifndef LIBDASH_FRAMEWORK_ADAPTATION_BOLA_H_
#define LIBDASH_FRAMEWORK_ADAPTATION_BOLA_H_

#include "AbstractAdaptationLogic.h"
#include "../MPD/AdaptationSetStream.h"
#include "../Input/IDASHReceiverObserver.h"

namespace libdash
{
    namespace framework
    {
        namespace adaptation
        {
            class BolaAdaptation : public AbstractAdaptationLogic
            {
                public:
            	BolaAdaptation            (dash::mpd::IMPD *mpd, dash::mpd::IPeriod *period, dash::mpd::IAdaptationSet *adaptationSet, bool isVid, double arg1, int arg2);
                    virtual ~BolaAdaptation();

                    virtual LogicType               GetType             ();
                    virtual bool					IsUserDependent		();
                    virtual bool 					IsRateBased			();
                    virtual bool 					IsBufferBased		();
                    virtual void 					BitrateUpdate		(uint64_t bps);
                    virtual void 					DLTimeUpdate		(double time);
                    virtual void 					BufferUpdate		(uint32_t bufferFill);
                    void 							SetBitrate			(uint32_t bufferFill);
                    uint64_t						GetBitrate			();
                    virtual void 					SetMultimediaManager	(sampleplayer::managers::IMultimediaManagerBase *_mmManager);
                    void							NotifyBitrateChange	();
                    void							OnEOS				(bool value);
                    void							CheckedByDASHReceiver();

                    int 							getQualityFromThroughput(uint64_t bps);
                    int 							getQualityFromBufferLevel(double bufferLevelSec);

                private:
                    enum BolaState
					{
                    	ONE_BITRATE,			// If one bitrate (or init failed), always NO_CHANGE
						STARTUP,				// Download fragments at most recently measured throughput
						STARTUP_NO_INC,			// If quality increased then decreased during startup, then quality cannot be increased.
						STEADY					// The buffer is primed (should be above bufferTarget)
					};

                    bool							initState;
                    double 							bufferMaxSizeSeconds;		// Usually set to 30s
                    double 							bufferTargetSeconds;  	// It is passed as an init parameter.
                    														// It states the difference between STARTUP and STEADY
                    														// 12s following dash.js implementation

                    double 							bolaBufferTargetSeconds; 	// BOLA introduces a virtual buffer level in order to make quality decisions
                    															// as it was filled (instead of the actual bufferTargetSeconds)

                    double 							bolaBufferMaxSeconds; 	// When using the virtual buffer, it must be capped.

                    uint32_t 						bufferTargetPerc;		// Computed considering a bufferSize = 30s
                    double 							totalDuration;			// Total video duration in seconds (taken from MPD)
                    double 							segmentDuration;		// Segment duration in seconds

                    std::vector<uint64_t>			availableBitrates;
                    std::vector<double>				utilityVector;
                    uint32_t						bitrateCount;			// Number of available bitrates
                    BolaState						bolaState;				// Keeps track of Bola state

                    // Bola Vp and gp (multiplied by the segment duration 'p')
                    // They are dimensioned such that log utility would always prefer
                    // - the lowest bitrate when bufferLevel = segmentDuration
                    // - the highest bitrate when bufferLevel = bufferTarget
                    double							Vp;
                    double 							gp;

                    bool							safetyGuarantee;
                    double							maxRtt;

                    double 							virtualBuffer;

                    uint64_t						currentBitrate;
                    int 							currentQuality;
                    uint64_t						batchBw;
                    int								batchBwCount;
                    std::vector<uint64_t>			batchBwSamples;
                    uint64_t						instantBw;
                    uint64_t						averageBw;

                    double 							lastDownloadTimeInstant;
                    double 							currentDownloadTimeInstant;
                    double							lastSegmentDownloadTime;

                    uint32_t						lastBufferFill;
                    bool							bufferEOS;
                    bool							shouldAbort;
                    double							alphaRate;
                    bool							isCheckedForReceiver;

                    sampleplayer::managers::IMultimediaManagerBase	*multimediaManager;
                    dash::mpd::IRepresentation		*representation;
            };
        }
    }
}

#endif /* LIBDASH_FRAMEWORK_ADAPTATION_BOLA_H_ */
