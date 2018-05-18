/*
 * RateBasedAdaptation.h
 *****************************************************************************
 * Copyright (C) 2016,
 * Jacques Samain <jsamain@cisco.com>
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#ifndef LIBDASH_FRAMEWORK_ADAPTATION_BUFFERBASEDADAPTATION_H_
#define LIBDASH_FRAMEWORK_ADAPTATION_BUFFERBASEDADAPTATION_H_

#include "AbstractAdaptationLogic.h"
#include "../MPD/AdaptationSetStream.h"
#include "../Input/IDASHReceiverObserver.h"

namespace libdash
{
    namespace framework
    {
        namespace adaptation
        {
            class BufferBasedAdaptation : public AbstractAdaptationLogic
            {
                public:
                    BufferBasedAdaptation            (dash::mpd::IMPD *mpd, dash::mpd::IPeriod *period, dash::mpd::IAdaptationSet *adaptationSet, bool isVid, double arg1, int arg2);
                    virtual ~BufferBasedAdaptation   ();

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
                    void 							CheckedByDASHReceiver();

                private:
                    uint64_t						currentBitrate;
                    std::vector<uint64_t>			availableBitrates;
                    sampleplayer::managers::IMultimediaManagerBase	*multimediaManager;
                    dash::mpd::IRepresentation		*representation;
                    uint32_t						reservoirThreshold;
                    uint32_t						maxThreshold;
                    uint32_t						lastBufferFill;
                    bool							bufferEOS;
                    bool							shouldAbort;
            };
        }
    }
}

#endif /* LIBDASH_FRAMEWORK_ADAPTATION_BUFFERBASEDADAPTATION_H_ */
