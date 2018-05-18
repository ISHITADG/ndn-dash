/*
 * MediaObject.h
 *****************************************************************************
 * Copyright (C) 2012, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#ifndef LIBDASH_FRAMEWORK_INPUT_MEDIAOBJECT_H_
#define LIBDASH_FRAMEWORK_INPUT_MEDIAOBJECT_H_

#include "../Adaptation/IAdaptationLogic.h"
#include "IMPD.h"
#include "IDownloadObserver.h"
#include "IDASHMetrics.h"
#include "../Portable/MultiThreading.h"

#ifdef NDNICPDOWNLOAD
	#include "NDNConnection.h"
#else
	#include "NDNConnectionConsumerApi.h"
#endif

namespace libdash
{
    namespace framework
    {
	namespace adaptation
	{
		class IAdaptationLogic;
	}
        namespace input
        {
           	class DASHReceiver;
            class MediaObject : public dash::network::IDownloadObserver, public dash::metrics::IDASHMetrics
            {
                public:
                    MediaObject             (dash::mpd::ISegment *segment, dash::mpd::IRepresentation *rep, bool withFeedBack = false);
                    virtual ~MediaObject    ();

                    bool                        StartDownload       ();
                    bool                        StartDownload       (INDNConnection* conn);
                    void                        AbortDownload       ();
                    void                        WaitFinished        (bool isInit=false);
                    int                         Read                (uint8_t *data, size_t len);
                    int                         Peek                (uint8_t *data, size_t len);
                    int                         Peek                (uint8_t *data, size_t len, size_t offset);
                    dash::mpd::IRepresentation* GetRepresentation   ();
                    const char*					GetPath				();

                    void						SetFeedBack			(bool flag);
                    virtual void    			OnDownloadStateChanged  (dash::network::DownloadState state);
                    virtual void   				OnDownloadRateChanged   (uint64_t bytesDownloaded); //Here the bytesDownloaded is in fact the bitrate bps
                    virtual void   				OnDownloadTimeChanged   (double dnltime);

                    /*
                     * IDASHMetrics
                     */
                    const std::vector<dash::metrics::ITCPConnection *>&     GetTCPConnectionList    () const;
                    const std::vector<dash::metrics::IHTTPTransaction *>&   GetHTTPTransactionList  () const;

                    void						SetAdaptationLogic(framework::adaptation::IAdaptationLogic *_adaptationLogic);
                    void 						SetDASHReceiver(input::DASHReceiver *_dashReceiver);
                private:
                    dash::mpd::ISegment             *segment;
                    dash::mpd::IRepresentation      *rep;
                    dash::network::DownloadState    state;
                    uint64_t						bps;
                    double							dnltime;
                    bool							withFeedBack;

                    input::DASHReceiver			 *dashReceiver;
                    adaptation::IAdaptationLogic *adaptationLogic;
                    mutable CRITICAL_SECTION    stateLock;
                    mutable CONDITION_VARIABLE  stateChanged;
            };
        }
    }
}

#endif /* LIBDASH_FRAMEWORK_INPUT_MEDIAOBJECT_H_ */
