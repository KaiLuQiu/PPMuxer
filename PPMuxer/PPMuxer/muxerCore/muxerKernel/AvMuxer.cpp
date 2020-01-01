//
//  AvMuxer.cpp
//  PPMuxer
//
//  Created by 邱开禄 on 2019/12/31.
//  Copyright © 2019 邱开禄. All rights reserved.
//
#include "AvMuxer.h"

NS_MEDIA_BEGIN

AvMuxer::AvMuxer():
p_VideoCodecContext(NULL),
p_AudioCodecContext(NULL),
p_FormatContext(NULL),
p_OutFormat(NULL),
p_VideoStream(NULL),
p_AudioStream(NULL),
p_VideoFrame(NULL),
p_AudioFrame(NULL),
p_SwsContex(NULL),
pCurVideoFrameIndex(0),
pCurAudioFrameIndex(0),
pDuration(0),
pFileName(NULL),
pWidth(0),
pHeight(0),
pVideoStreamIndex(1),
pAudioStreamIndex(0)
{

}

AvMuxer::~AvMuxer()
{
    if (p_VideoCodecContext) {
        avcodec_close(p_VideoCodecContext);
    }
    
    if (p_FormatContext) {
        if (p_FormatContext->pb) {
            avio_close(p_FormatContext->pb);
        }
        avformat_free_context(p_FormatContext);
        p_FormatContext = NULL;
    }
    
    if (p_VideoFrame) {
        av_frame_free(&p_VideoFrame);
        p_VideoFrame = NULL;
    }
    
    if (p_AudioFrame) {
        av_frame_free(&p_AudioFrame);
        p_AudioFrame = NULL;
    }
    
    if (p_SwsContex) {
        sws_freeContext(p_SwsContex);
        p_SwsContex = NULL;
    }
}

void AvMuxer::initEncodeOutputParam(EncodeParam param)
{
    pEncodeParam = param;
}

int AvMuxer::init(const char * fileName, int nWidth, int nHeight)
{
    int ret;
    pFileName = fileName;
    pWidth = nWidth;
    pHeight = nHeight;
    // 注册编码器、解码器等
    av_register_all();

    // alloc avformat上下文
    p_FormatContext = avformat_alloc_context();
    
    // FFmpeg程序推测出文件类型->视频压缩数据格式类型
    p_OutFormat = av_guess_format(NULL, pFileName, NULL);
    
    // 指定输出格式类型
    p_FormatContext->oformat = p_OutFormat;

    if (!(p_OutFormat->flags & AVFMT_NOFILE)) {
        if (avio_open(&p_FormatContext->pb, pFileName, AVIO_FLAG_READ_WRITE) < 0) {
            return -1;
        }
    }
    
    if (p_OutFormat->video_codec != AV_CODEC_ID_NONE) {
        return -1;
    }
    
    if (p_SwsContex) {
        sws_freeContext(p_SwsContex);
        p_SwsContex = NULL;
    }
    p_SwsContex = sws_getContext(nWidth, nHeight, pEncodeParam.pVideoInPixelFormat, pEncodeParam.pVideoOutWidth, pEncodeParam.pVideoOutHeight, pEncodeParam.pVideoOutPixelFormat, SWS_BICUBIC, NULL, NULL, NULL);
    
    ret = initVideoEncdoe(nWidth, nHeight);
    if (ret < 0) {
        return -1;
    }

    ret = initAudioEncdoe();
    if (ret < 0) {
        return -1;
    }
    
    if (AllocFrame() < 0) {
        return -1;
    }

    return 0;
}

int AvMuxer::initVideoEncdoe(int nWidth, int nHeight)
{
    // 创建输出码流->创建了一块内存空间->并不知道他是什么类型流->希望他是视频流
    p_VideoStream = avformat_new_stream(p_FormatContext, NULL);
    // 设置流的timebase
    p_VideoStream->time_base = (AVRational){1, pEncodeParam.pVideoOutFrameRate};
    p_VideoStream->duration = pDuration * AV_TIME_BASE;
    // 获取编码器上下文
    p_VideoCodecContext = p_VideoStream->codec;
    
    p_VideoCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
    p_VideoCodecContext->pix_fmt = pEncodeParam.pVideoOutPixelFormat;
    p_VideoCodecContext->width = pEncodeParam.pVideoOutWidth;
    p_VideoCodecContext->height = pEncodeParam.pVideoOutHeight;
    
    p_VideoCodecContext->time_base.num = 1;
    
    // 设置帧率（目前先写固定值）
    p_VideoCodecContext->time_base.den = pEncodeParam.pVideoOutFrameRate;
    
    p_VideoCodecContext->codec_id = pEncodeParam.pVideoOutCodecId;
    
    p_VideoCodecContext->gop_size = 250;
    p_VideoCodecContext->max_qdiff = 4;
    p_VideoCodecContext->max_qdiff = 4;

    p_VideoCodecContext->qmin = 10;
    p_VideoCodecContext->qcompress = 1.0;

    p_VideoCodecContext->max_b_frames = 600;

    AVCodec *avcodec = avcodec_find_encoder(p_VideoCodecContext->codec_id);
    if (!avcodec) {
        return -1;
    }

    AVDictionary *param = 0;
    if (p_VideoCodecContext->codec_id == AV_CODEC_ID_H264) {
        //需要查看x264源码->x264.c文件
        // 预备参数
        //key:preset
        //value:slow->慢
        //value:superfast->超快
        av_dict_set(&param, "preset", "slow", 0);
        av_dict_set(&param, "crf", "28.0", 0);
        // 调优
        //key:tune->调优
        //value:zerolatency->零延迟
        av_dict_set(&param, "tune", "zerolatency", 0);
        av_dict_set(&param, "profile", "main", 0);

    }
    
    if (avcodec_open2(p_VideoCodecContext, avcodec, &param) < 0) {
        return -1;
    }
    
    return 0;
}

int AvMuxer::initAudioEncdoe()
{
    // 创建输出码流->创建了一块内存空间->并不知道他是什么类型流->希望他是视频流
    p_AudioStream = avformat_new_stream(p_FormatContext, NULL);
    if (!p_AudioStream) {
        return -1;
    }
    p_AudioStream->duration = pDuration * AV_TIME_BASE;
    
    p_AudioCodecContext = p_AudioStream->codec;
    p_AudioCodecContext->codec = avcodec_find_encoder(pEncodeParam.pAudioOutCodecId);
    p_AudioCodecContext->sample_rate = pEncodeParam.pAudioOutSampleRate;
    p_AudioCodecContext->channel_layout = pEncodeParam.pAudioOutChannelLayout;
    p_AudioCodecContext->channels = pEncodeParam.pAudioOutChannels;
    p_AudioCodecContext->sample_fmt = pEncodeParam.pAudioOutSample_fmt;
    p_AudioCodecContext->frame_size = pEncodeParam.pAudioOutSampleSize;
    p_AudioCodecContext->codec_tag = 0;
    
    p_AudioCodecContext->time_base = {1, pEncodeParam.pAudioOutSampleRate};
    p_AudioCodecContext->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    if (p_FormatContext->oformat->flags & AVFMT_GLOBALHEADER) {
        p_AudioCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    
    if (avcodec_open2(p_AudioCodecContext, p_AudioCodecContext->codec, 0) < 0) {
        return -1;
    }

    return 0;
}

int AvMuxer::start()
{
    int ret = avformat_write_header(p_FormatContext, NULL);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

int AvMuxer::stop()
{
    int ret = av_write_trailer(p_FormatContext);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

int AvMuxer::AllocFrame()
{
    if (p_VideoFrame) {
        av_frame_free(&p_VideoFrame);
        p_VideoFrame = NULL;
    }
    if (p_AudioFrame) {
        av_frame_free(&p_AudioFrame);
        p_AudioFrame = NULL;
    }
    
    // 开辟一块内存空间
    p_VideoFrame = av_frame_alloc();
    if (!p_VideoFrame)
        return -1;

    p_VideoFrame->width = pEncodeParam.pVideoOutWidth;
    p_VideoFrame->height = pEncodeParam.pVideoOutHeight;
    p_VideoFrame->format = pEncodeParam.pVideoOutPixelFormat;
    
    int ret = av_frame_get_buffer(p_VideoFrame, 0);
    if (ret < 0) {
        av_frame_free(&p_VideoFrame);
        return -1;
    }

    p_AudioFrame = av_frame_alloc();
    p_AudioFrame->nb_samples = pEncodeParam.pAudioOutSampleSize;
    p_AudioFrame->channel_layout = pEncodeParam.pAudioOutChannelLayout;
    p_AudioFrame->format = pEncodeParam.pAudioOutSample_fmt;
    p_AudioFrame->sample_rate = pEncodeParam.pAudioOutSampleRate;
    
    ret = av_frame_get_buffer(p_AudioFrame, 0);
    if (ret < 0) {
        av_frame_free(&p_AudioFrame);
        return -1;
    }
    
    return 0;
}

int AvMuxer::AudioEncode(AVFrame *frame)
{
    int ret;
    AVPacket en_pkt;
    av_init_packet(&en_pkt);
    en_pkt.data = NULL;
    en_pkt.size = 0;
    
    updateAudioFrame(frame, p_AudioFrame);
    
    int got_frame = 0;
    ret = avcodec_encode_audio2(p_AudioCodecContext, &en_pkt, p_AudioFrame, &got_frame);
    if (ret < 0) {
        av_packet_unref(&en_pkt);

        return -1;
    }
    
    if (got_frame) {
        en_pkt.stream_index = pAudioStreamIndex;
        en_pkt.pts = frame->pts;
        en_pkt.dts = en_pkt.pts;
        en_pkt.duration = pDuration;
            
        ret = av_interleaved_write_frame(p_FormatContext, &en_pkt);
        
        pCurAudioFrameIndex++;
    }
    if(en_pkt.data) {
        free(en_pkt.data);
    }
    av_free_packet(&en_pkt);
    return 0;
}


int AvMuxer::VideoEncode(const unsigned char *pdata)
{
    int ret;
    if (NULL == pdata) {
        return -1;
    }
    updateVideoFrame(pdata, p_VideoFrame);
    ret = VideoEncode(p_VideoFrame);
    return ret;
}

int AvMuxer::VideoEncode(AVFrame *frame)
{
    AVPacket en_pkt;
    av_init_packet(&en_pkt);
    
    int ret = avcodec_send_frame(p_VideoCodecContext, p_VideoFrame);
    if (ret < 0) {
        free(en_pkt.data);
        av_packet_unref(&en_pkt);
        return -1;
    }
        
    ret = avcodec_receive_packet(p_VideoCodecContext, &en_pkt);
    if(ret == 0) {
        //编码成功
        // 将视频压缩数据-写入到输出文件中-outFilePath;
        en_pkt.stream_index = pVideoStreamIndex;
        en_pkt.duration = pDuration;
        int result = av_interleaved_write_frame(p_FormatContext, &en_pkt);
        if (result < 0) {
            free(en_pkt.data);
            av_packet_unref(&en_pkt);
            return -1;
        }
    } else {
        free(en_pkt.data);
        av_packet_unref(&en_pkt);
        return -1;
    }
    free(en_pkt.data);
    av_free_packet(&en_pkt);
    return 0;
}

int AvMuxer::updateVideoFrame(const unsigned char *pdata, AVFrame *frame)
{
    if (NULL == p_SwsContex) {
        return -1;
    }
    
    AVFrame * tempFrame = av_frame_alloc();
    tempFrame->format = pEncodeParam.pVideoInPixelFormat;
    
    avpicture_fill((AVPicture *)tempFrame, pdata, pEncodeParam.pVideoInPixelFormat, pWidth, pHeight);
    
    tempFrame->data[0] += tempFrame->linesize[0] * (pHeight - 1);
    tempFrame->linesize[0] *= -1;
    
    sws_scale(p_SwsContex, tempFrame->data, tempFrame->linesize, 0, pHeight, frame->data, frame->linesize);
    
    frame->data[0] += frame->linesize[0] * (pEncodeParam.pVideoOutHeight - 1);
    frame->linesize[0] *= -1;
    frame->data[1] += frame->linesize[1] * ((pEncodeParam.pVideoOutHeight >> 1) - 1);
    frame->linesize[1] *= -1;
    frame->data[2] += frame->linesize[2] * ((pEncodeParam.pVideoOutHeight >> 1) - 1);
    frame->linesize[2] *= -1;
    
    frame->pts = av_rescale_q(pCurVideoFrameIndex++, p_VideoCodecContext->time_base, p_VideoStream->time_base);
    av_frame_free(&tempFrame);
    return 0;
}

int AvMuxer::updateAudioFrame(AVFrame *frameIn, AVFrame *frameOut)
{
    
    frameOut->pts = pCurAudioFrameIndex * p_AudioCodecContext->frame_size;
    return 0;
}

int AvMuxer::flushAudioEncode()
{
    int ret = 0;
    int got_frame;
    if (!(p_FormatContext->streams[pAudioStreamIndex]->codec->codec->capabilities &
          AV_CODEC_CAP_DELAY))
        return 0;
    
    while (1) {
        AVPacket enc_pkt;
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        
        ret = avcodec_encode_audio2(p_FormatContext->streams[pAudioStreamIndex]->codec, &enc_pkt,
                                    NULL, &got_frame);
        av_frame_free(NULL);
        if (ret < 0)
            break;
        if (!got_frame) {
            ret = 0;
            break;
        }
        enc_pkt.stream_index = pAudioStreamIndex;
        enc_pkt.duration = pDuration;
        /* mux encoded frame */
        ret = av_interleaved_write_frame(p_FormatContext, &enc_pkt);
        if (ret < 0)
            break;
        av_free_packet(&enc_pkt);
        pCurAudioFrameIndex++;
    }
    return ret;
}

int AvMuxer::flushVideoEncode()
{
    int ret = 0;
    int got_frame;
    if (!(p_FormatContext->streams[pVideoStreamIndex]->codec->codec->capabilities &
          AV_CODEC_CAP_DELAY))
        return 0;
    
    while (1) {
        AVPacket enc_pkt;
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        
        ret = avcodec_encode_video2(p_FormatContext->streams[pVideoStreamIndex]->codec, &enc_pkt,
                                    NULL, &got_frame);
        av_frame_free(NULL);
        if (ret < 0)
            break;
        if (!got_frame) {
            ret = 0;
            break;
        }
        enc_pkt.stream_index = pVideoStreamIndex;
        enc_pkt.duration = pDuration;
        /* mux encoded frame */
        ret = av_interleaved_write_frame(p_FormatContext, &enc_pkt);
        if (ret < 0)
            break;
        av_free_packet(&enc_pkt);
    }
    return ret;
}

NS_MEDIA_END
