/*
 * AbstractAdaptationLogic.cpp
 *****************************************************************************
 * Copyright (C) 2012, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#include "AbstractAdaptationLogic.h"

using namespace libdash::framework::adaptation;
using namespace dash::mpd;

AbstractAdaptationLogic::AbstractAdaptationLogic    (dash::mpd::IMPD *mpd, dash::mpd::IPeriod *period, dash::mpd::IAdaptationSet *adaptationSet, bool isVid) :
                         mpd                        (mpd),
                         period                     (period),
                         adaptationSet              (adaptationSet),
                         representation             (NULL),
						 isVideo					(isVid)
{
}
AbstractAdaptationLogic::~AbstractAdaptationLogic   ()
{
}

uint32_t            AbstractAdaptationLogic::GetPosition        ()
{
    return 0;
}
void                AbstractAdaptationLogic::SetPosition        (uint32_t segmentNumber)
{
    this->segmentNumber = segmentNumber;
}
IRepresentation*    AbstractAdaptationLogic::GetRepresentation  ()
{
    return this->representation;
}
void                AbstractAdaptationLogic::SetRepresentation  (IPeriod *period, IAdaptationSet *adaptationSet, IRepresentation *representation)
{
    this->period            = period;
    this->adaptationSet     = adaptationSet;
    this->representation    = representation;
}
