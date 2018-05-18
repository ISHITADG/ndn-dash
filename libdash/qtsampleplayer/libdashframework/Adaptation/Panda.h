/*
 * Panda.h
 *
 *  Created on: Oct 17, 2016
 *      Author: ndnops
 */

#ifndef LIBDASH_FRAMEWORK_ADAPTATION_PANDA_H_
#define LIBDASH_FRAMEWORK_ADAPTATION_PANDA_H_

#include "AbstractAdaptationLogic.h"
#include "../MPD/AdaptationSetStream.h"
#include "../Input/IDASHReceiverObserver.h"

namespace libdash
{
	namespace framework
	{
		namespace adaptation
		{
			class PandaAdaptation : public AbstractAdaptationLogic
			{
				public:
					PandaAdaptation(dash::mpd::IMPD *mpd, dash::mpd::IPeriod *period, dash::mpd::IAdaptationSet *adaptationSet, bool isVid, double arg1);
					virtual ~PandaAdaptation();

					virtual LogicType               GetType             ();
			        virtual bool					IsUserDependent		();
		            virtual bool 					IsRateBased			();
	                virtual bool 					IsBufferBased		();
                    virtual void 					BitrateUpdate		(uint64_t bps);
                    virtual void 					DLTimeUpdate		(double time);
                    virtual void 					BufferUpdate		(uint32_t bufferFill);
                    void 							SetBitrate			(uint64_t bufferFill);
                    uint64_t						GetBitrate			();
                    virtual void 					SetMultimediaManager	(sampleplayer::managers::IMultimediaManagerBase *_mmManager);
                    void							NotifyBitrateChange	();
                    void							OnEOS				(bool value);
                    void 							CheckedByDASHReceiver();
                    void							ewma				(uint64_t bps);
		    void						Quantizer			();
                private:
                    uint64_t						currentBitrate;
                    uint64_t						targetBps;
                    std::vector<uint64_t>			availableBitrates;
                    sampleplayer::managers::IMultimediaManagerBase	*multimediaManager;
                    dash::mpd::IRepresentation		*representation;
                    double						param_Alpha;
                    double						param_Epsilon;
                    uint64_t						averageBw;
                    double						param_K;
                    double						param_W;
                    double						param_Beta;
                    double						param_Bmin;
                    double						interTime;
                    double						targetInterTime;
                    double						downloadTime;
                    uint32_t						bufferLevel;
                    double						segmentDuration;
		    double						deltaUp;
		    double						deltaDown;
		    size_t						current;
			};

		} /* namespace adaptation */
	} /* namespace framework */
} /* namespace libdash */

#endif /* LIBDASH_FRAMEWORK_ADAPTATION_PANDA_H_ */
