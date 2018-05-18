/*
 * AdaptationLogicFactory.h
 *****************************************************************************
 * Copyright (C) 2013, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#ifndef LIBDASH_FRAMEWORK_ADAPTATION_ADAPTATIONLOGICFACTORY_H_
#define LIBDASH_FRAMEWORK_ADAPTATION_ADAPTATIONLOGICFACTORY_H_

#include "IAdaptationLogic.h"
#include "AlwaysLowestLogic.h"
#include "ManualAdaptation.h"
#include "RateBasedAdaptation.h"
#include "BufferBasedAdaptation.h"
#include "BufferBasedAdaptationWithRateBased.h"
#include "BufferBasedThreeThresholdAdaptation.h"
#include "Panda.h"
#include "Bola.h"

namespace libdash
{
    namespace framework
    {
        namespace adaptation
        {
            class AdaptationLogicFactory
            {
                public:
                    static IAdaptationLogic* Create(libdash::framework::adaptation::LogicType logic, 
                                                    dash::mpd::IMPD *mpd, dash::mpd::IPeriod *period, dash::mpd::IAdaptationSet *adaptationSet, bool isVid, double arg1 = 0., int arg2=0, int arg3=0);
            };
        }
    }
}

#endif /* LIBDASH_FRAMEWORK_ADAPTATION_ADAPTATIONLOGICFACTORY_H_ */
