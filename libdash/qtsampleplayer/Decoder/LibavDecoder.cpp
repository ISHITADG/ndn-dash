/*
 * LibavDecoder.cpp
 *****************************************************************************
 * Copyright (C) 2012, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#include "LibavDecoder.h"

#include <fstream>
#include <iostream>

//To circumvent the stupid refactoring in libavcodec...
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

using namespace libdash::framework::input;
using namespace sampleplayer::decoder;

#define AVERROR_RESSOURCE_NOT_AVAILABLE -11

int lock_call_back(void ** mutex, enum AVLockOp op) {
	switch(op) {
		case AV_LOCK_CREATE:
			*mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
			pthread_mutex_init((pthread_mutex_t *)(*mutex), NULL);
			break;
		case AV_LOCK_OBTAIN:
			pthread_mutex_lock((pthread_mutex_t *)(*mutex));
			break;
		case AV_LOCK_RELEASE:
			pthread_mutex_unlock((pthread_mutex_t *)(*mutex));
			break;
		case AV_LOCK_DESTROY:
			pthread_mutex_destroy((pthread_mutex_t *)(*mutex));
			free(*mutex);
			break;
	}

	return 0;
}


static int          IORead                           (void *opaque, uint8_t *buf, int buf_size)
{
    IDataReceiver* datrec = (IDataReceiver*)opaque;
    int ret = datrec->Read(buf, buf_size);
//    if(datrec->IsAudio())
/*    {
    	FILE * fp;
    	fp = fopen("/home/ndnops/IORead.bin", "ab");
    	fwrite(buf,sizeof(uint8_t), sizeof(uint8_t)*ret,fp);
    	fclose(fp);
    }
   // */
    return ret;
}

bool LibavDecoder::IsAudio	()
{
	return this->isAudio;
}

LibavDecoder::LibavDecoder  (IDataReceiver* rec) :
         receiver           (rec),
         errorHappened      (false),
         bufferSize         (32768),
		 isAudio			(false),
		 first				(true)
{
	av_register_all();
	av_lockmgr_register(&lock_call_back);
}
LibavDecoder::~LibavDecoder ()
{
}

void                LibavDecoder::AttachAudioObserver     (IAudioObserver *observer)
{
    audioObservers.push_back(observer);
}
void                LibavDecoder::AttachVideoObserver     (IVideoObserver *observer)
{
    videoObservers.push_back(observer);
}
void                LibavDecoder::Notify                  (AVFrame *frame, StreamConfig *config)
{
    switch(config->stream->codec->codec_type)
    {
        case AVMEDIA_TYPE_VIDEO:
            this->NotifyVideo(frame, config);
            break;
        case AVMEDIA_TYPE_AUDIO:
        {
        	this->NotifyAudio(frame, config);
            break;
        }
        case AVMEDIA_TYPE_SUBTITLE:
            break;
        case AVMEDIA_TYPE_UNKNOWN:
            break;
        default:
            break;
    }
}
void                LibavDecoder::NotifyVideo             (AVFrame * avFrame, StreamConfig* decoConf)
{
    videoFrameProperties props;

    props.fireError     = false;
    props.frame         = decoConf->frameCnt;
    props.height        = decoConf->stream->codec->height;
    props.width         = decoConf->stream->codec->width;
    props.linesize      = avFrame->linesize;
    props.streamIndex   = decoConf->stream->index;

    if(decoConf->stream->codec->pix_fmt == AV_PIX_FMT_YUV420P)
        props.pxlFmt = yuv420p;
    if(decoConf->stream->codec->pix_fmt == AV_PIX_FMT_YUV422P)
        props.pxlFmt = yuv422p;

    for(size_t i = 0; i < videoObservers.size(); i++)
        videoObservers.at(i)->OnVideoDataAvailable((const uint8_t **)avFrame->data, &props);
}
void                LibavDecoder::NotifyAudio             (AVFrame * avFrame, StreamConfig* decoConf)
{
    audioFrameProperties props;

    props.fireError   = false;
    props.streamIndex = decoConf->stream->index;
    props.linesize    = avFrame->linesize[0];
    props.channels    = decoConf->stream->codec->channels;
    props.sampleRate  = decoConf->stream->codec->sample_rate;
    props.samples     = avFrame->nb_samples;

    AVAudioResampleContext *resampCtx;

    Debug("Allocating resample context");
    resampCtx = avresample_alloc_context();

    if(!resampCtx)
    {
    	Debug("Couldn't alloc resample context...\n");
    }

    av_opt_set_int(resampCtx, "in_sample_fmt", decoConf->codecContext->sample_fmt,0);
    av_opt_set_int(resampCtx, "in_sample_rate", decoConf->stream->codec->sample_rate,0);
    av_opt_set_int(resampCtx, "in_channel_layout", AV_CH_LAYOUT_STEREO,0);
    av_opt_set_int(resampCtx, "out_sample_fmt", AV_SAMPLE_FMT_S32,0);
    av_opt_set_int(resampCtx, "out_sample_rate", decoConf->stream->codec->sample_rate,0);
    av_opt_set_int(resampCtx, "out_channel_layout", AV_CH_LAYOUT_STEREO,0);
    av_opt_set_int(resampCtx, "internal_sample_fmt", AV_SAMPLE_FMT_FLTP,0);

    if(avresample_open(resampCtx) < 0)
    	Debug("Error openning the resampling context\n");

    int data_size = av_samples_get_buffer_size(NULL, decoConf->codecContext->channels, avFrame->nb_samples,decoConf->codecContext->sample_fmt, 1);
    props.linesize = data_size;

    uint8_t *output;
    int out_linesize;
    int out_samples = avresample_available(resampCtx) + av_rescale_rnd(avresample_get_delay(resampCtx) + avFrame->nb_samples,decoConf->stream->codec->sample_rate, decoConf->stream->codec->sample_rate, AV_ROUND_UP);
   // avresample_get_out_samples(resampCtx, avFrame->nb_samples);
    Debug("out_samples: %d\n", out_samples);
    av_samples_alloc(&output, &out_linesize, 2, out_samples,AV_SAMPLE_FMT_FLT, 0);
    out_samples = avresample_convert(resampCtx, &output, out_linesize, out_samples,avFrame->data, avFrame->linesize[0], avFrame->nb_samples);

    //for(size_t i = 0; i < audioObservers.size(); i++)
        //    audioObservers.at(i)->OnAudioDataAvailable((const uint8_t **) avFrame->data, &props);
    for(size_t i = 0; i < audioObservers.size(); i++)
            audioObservers.at(i)->OnAudioDataAvailable((const uint8_t **) &output, &props);

}
AVFormatContext*    LibavDecoder::OpenInput               ()
{
    AVFormatContext *avFormatContextPtr = NULL;
    int             err                 = 0;

    this->iobuffer                      = (unsigned char*) av_malloc(bufferSize);
    avFormatContextPtr                  = avformat_alloc_context();
    avFormatContextPtr->pb              = avio_alloc_context(this->iobuffer, bufferSize, 0, receiver, IORead, NULL, NULL);
    avFormatContextPtr->pb->seekable    = 0;

    err = avformat_open_input(&avFormatContextPtr, "", NULL, NULL);
    if (err < 0)
    {
        this->Error("Error while calling avformat_open_input", err);
        return NULL;
    }

    err = avformat_find_stream_info(avFormatContextPtr, 0);
    if (err < 0)
    {
        this->Error("Error while calling avformat_find_stream_info", err);
        return NULL;
    }

    return avFormatContextPtr;
}
void                LibavDecoder::InitStreams             (AVFormatContext *avFormatContextPtr)
{
    AVStream      *tempStream   = NULL;
    AVCodec       *tempCodec    = NULL;
    StreamConfig  tempConfig;

    for (size_t streamcnt = 0; streamcnt < avFormatContextPtr->nb_streams; ++streamcnt)
    {
        tempStream = avFormatContextPtr->streams[streamcnt];

        if ((tempStream->codec->codec_type == AVMEDIA_TYPE_VIDEO) ||
            (tempStream->codec->codec_type == AVMEDIA_TYPE_AUDIO))
        {
            tempConfig.stream         = tempStream;
           tempCodec                 = avcodec_find_decoder(tempStream->codec->codec_id);

	   tempConfig.codecContext   = tempStream->codec;
            tempConfig.frameCnt       = 0;
	    //tempStream->codec->hwaccel = ff_find_hwaccel(tempStream->codec->codec_id, tempStream->codec->pix_fmt);
            avcodec_open2(tempConfig.codecContext, tempCodec, NULL);
            this->streamconfigs.push_back(tempConfig);
        }
        if(tempStream->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        	this->isAudio = true;
    }
}

AVHWAccel * LibavDecoder::ff_find_hwaccel(enum AVCodecID codec_id, enum AVPixelFormat pix_fmt)
{
	AVHWAccel *hwaccel = NULL;

	while(hwaccel = av_hwaccel_next(hwaccel))
	{
		if((hwaccel->id == codec_id) && hwaccel->pix_fmt == pix_fmt)
			return hwaccel;
	}
	return NULL;
}

StreamConfig*       LibavDecoder::GetNextPacket           (AVFormatContext* avFormatContextPtr, AVPacket* avpkt)
{
    while (true)
    {
        int err = av_read_frame(avFormatContextPtr, avpkt);

        if ((size_t)err == AVERROR_EOF)
            return NULL;

        if (err == AVERROR_RESSOURCE_NOT_AVAILABLE)
        {
            av_free_packet(avpkt);
            continue;
        }

        if (err < 0)
        {
            this->Error("Error while av_read_frame", err);
            return NULL;
        }

        StreamConfig *tempConfig = this->FindStreamConfig(avpkt->stream_index);
        if (tempConfig)
            return tempConfig;

        av_free_packet(avpkt);
    }
}
StreamConfig*       LibavDecoder::FindStreamConfig        (int streamIndex)
{
    size_t configsCount = this->streamconfigs.size();

    for (size_t i = 0; i < configsCount; i++)
    {
        if (this->streamconfigs.at(i).stream->index == streamIndex)
            return &this->streamconfigs.at(i);
    }

    return NULL;
}
int                 LibavDecoder::DecodeMedia             (AVFrame *frame, AVPacket *avpkt, StreamConfig *decConfig, int *got_frame)
{
    switch(decConfig->stream->codec->codec_type)
    {
        case AVMEDIA_TYPE_VIDEO:
		{
			avcodec_get_frame_defaults(frame);
	    		return avcodec_decode_video2(decConfig->codecContext, frame, got_frame, avpkt);
		}
	case AVMEDIA_TYPE_AUDIO:
        	{
        	    return avcodec_decode_audio4(decConfig->codecContext, frame, got_frame, avpkt);
        	}
        case AVMEDIA_TYPE_SUBTITLE:
            break;
        case AVMEDIA_TYPE_UNKNOWN:
            break;
        default:
            break;
    }
    return -1;
}
int                 LibavDecoder::DecodeFrame             (AVFrame *frame, AVPacket* avpkt, StreamConfig* decConfig)
{
    int len         = 0;
    int got_frame   = 1;
    while (avpkt->size > 0)
    {
        /* TODO handle multi frame packets */
        len = this->DecodeMedia(frame, avpkt, decConfig, &got_frame);

        if(len < 0)
        {
            this->Error("Error while decoding frame", len);
            return -1;
        }

        if(got_frame)
        {
            this->Notify(frame, decConfig);
            decConfig->frameCnt++;
        }
        avpkt->data += len;
        avpkt->size -= len;
    }
  //  av_free_packet(avpkt);
    return 0;
}
void                LibavDecoder::Flush                   ()
{
    int gotFrame        = 0;
    int configsCount    = this->streamconfigs.size();

    av_free_packet(&this->avpkt);
    av_init_packet(&this->avpkt);

    for (int i = 0; i < configsCount; i++)
    {
        do
        {
            this->DecodeMedia(this->frame, &this->avpkt, &this->streamconfigs.at(i), &gotFrame);

            if (gotFrame)
	    {
		this->Notify(this->frame, &this->streamconfigs.at(i));
	    }

        } while(gotFrame);
    }
    av_free_packet(&this->avpkt);
}

bool                LibavDecoder::Init                    ()
{
	this->avFormatContextPtr = this->OpenInput();

    if (this->errorHappened)
        return false;

    //this->frame = avcodec_alloc_frame();
    this->frame = av_frame_alloc();

    av_init_packet(&this->avpkt);

    this->InitStreams(avFormatContextPtr);

    return true;
}
bool                LibavDecoder::Decode                  ()
{
    StreamConfig *decConfig = this->GetNextPacket(this->avFormatContextPtr, &this->avpkt);

    if(decConfig == NULL)
        return false;

    if(this->DecodeFrame(this->frame, &this->avpkt, decConfig) < 0)
        return false;

    return true;
}
void                LibavDecoder::Stop                    ()
{
    this->FreeConfigs();
    avformat_close_input(&this->avFormatContextPtr);
    av_free(this->frame);
}
void                LibavDecoder::FreeConfigs             ()
{
    for(size_t i = 0; i < this->streamconfigs.size(); i++)
        avcodec_close(this->streamconfigs.at(i).codecContext);
}
void                LibavDecoder::Error                   (std::string errormsg, int errorcode)
{
    videoFrameProperties    videoprops;
    audioFrameProperties    audioprops;
    char                    errorCodeMsg[1024];
    std::stringstream       sstm;

    av_strerror(errorcode, errorCodeMsg, 1024);
    sstm << "ERROR: " << errormsg << ", ErrorCodeMsg: " << errorCodeMsg;

    videoprops.fireError        = true;
    videoprops.errorMessage = (char*)malloc(sstm.str().size()+1);
    strcpy(videoprops.errorMessage, sstm.str().c_str());

    Debug("Error LibavDecoder: %s\n\nNow onto solving it...\n\n",sstm.str().c_str());

    for (size_t i = 0; i < videoObservers.size(); i++)
        videoObservers.at(i)->OnVideoDataAvailable(NULL, &videoprops);

    audioprops.fireError       = true;
    audioprops.errorMessage = (char*)malloc(sstm.str().size()+1);
    strcpy(audioprops.errorMessage, sstm.str().c_str());

    for (size_t j = 0; j < audioObservers.size(); j++)
        audioObservers.at(j)->OnAudioDataAvailable(NULL, &audioprops);

    this->errorHappened    = true;
    free(audioprops.errorMessage);
    free(videoprops.errorMessage);
}
