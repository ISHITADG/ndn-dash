/*
 * RateBasedAdaptation.h
 *****************************************************************************
 * Copyright (C) 2016,
 * Jacques Samain <jsamain@cisco.com>
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#ifndef LIBDASH_FRAMEWORK_ADAPTATION_BUFFERBASEDADAPTATIONTHREE_H_
#define LIBDASH_FRAMEWORK_ADAPTATION_BUFFERBASEDADAPTATIONTHREE_H_

#include "AbstractAdaptationLogic.h"
#include "../MPD/AdaptationSetStream.h"
#include "../Input/IDASHReceiverObserver.h"

namespace libdash
{
    namespace framework
    {
        namespace adaptation
        {
            class BufferBasedThreeThresholdAdaptation : public AbstractAdaptationLogic
            {
                public:
            	BufferBasedThreeThresholdAdaptation            (dash::mpd::IMPD *mpd, dash::mpd::IPeriod *period, dash::mpd::IAdaptationSet *adaptationSet, bool isVid, double arg1, int arg2, int arg3);
                    virtual ~BufferBasedThreeThresholdAdaptation   ();

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

                private:
                    uint64_t						currentBitrate;
                    std::vector<uint64_t>			availableBitrates;
                    sampleplayer::managers::IMultimediaManagerBase	*multimediaManager;
                    dash::mpd::IRepresentation		*representation;
                    uint32_t						secondThreshold;
                    uint32_t						thirdThreshold;
                    uint32_t						lastBufferFill;
                    bool							bufferEOS;
                    bool							shouldAbort;
                    uint32_t						firstThreshold;
                    uint64_t						instantBw;
                    int								myQuality;
                    double							slackParam;
                    bool							isCheckedForReceiver;
            };
        }
    }
}

#endif /* LIBDASH_FRAMEWORK_ADAPTATION_BUFFERBASEDADAPTATIONTHREE_H_ */
