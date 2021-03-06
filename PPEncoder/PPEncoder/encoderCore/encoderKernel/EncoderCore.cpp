//
//  EncoderCore.cpp
//  PPEncoder
//
//  Created by 邱开禄 on 2019/12/31.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#include "EncoderCore.h"

NS_MEDIA_BEGIN

EncoderCore::EncoderCore():
p_VideoCodecContext(NULL),
p_AudioCodecContext(NULL),
p_FormatContext(NULL),
p_OutFormat(NULL),
p_VideoStream(NULL),
p_AudioStream(NULL),
p_AudioFrame(NULL),
p_SwsContex(NULL),
p_SwrContex(NULL),
pCurVideoFrameIndex(0),
pCurAudioFrameIndex(0),
pDuration(0),
pFileName(NULL),
pVideoStreamIndex(1),
pAudioStreamIndex(0)
{

}

EncoderCore::~EncoderCore()
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

    if (p_AudioFrame) {
        av_frame_free(&p_AudioFrame);
        p_AudioFrame = NULL;
    }
    
    if (p_SwsContex) {
        sws_freeContext(p_SwsContex);
        p_SwsContex = NULL;
    }
    
    if(p_SwrContex) {
        swr_free(&p_SwrContex);
        p_SwrContex = NULL;
    }
    
}

int EncoderCore::init(const char* fileName, int duration, EncodeParam param)
{
    if (NULL == fileName) {
        return -1;
    }
    int ret;
    pFileName = fileName;
    pDuration = duration;
    pEncodeParam = param;

    // alloc avformat上下文
    p_FormatContext = avformat_alloc_context();
    
    // FFmpeg程序推测出文件类型->视频压缩数据格式类型
    p_OutFormat = av_guess_format(NULL, pFileName, NULL);
    
    // 指定输出格式类型
    p_FormatContext->oformat = p_OutFormat;

    if (!(p_OutFormat->flags & AVFMT_NOFILE)) {
        if (avio_open(&p_FormatContext->pb, pFileName, AVIO_FLAG_WRITE) < 0) {
            printf("EncoderCore: avio_open fail!!!\n");
            return -1;
        }
    }
    
    if (p_OutFormat->video_codec == AV_CODEC_ID_NONE) {
        printf("EncoderCore: AV CODEC ID NONE!!!\n");
        return -1;
    }
    
    ret = initAudioEncdoe();
    if (ret < 0) {
        printf("EncoderCore: initAudioEncdoe fail!!!\n");
        return -1;
    }
    
    ret = initVideoEncdoe();
    if (ret < 0) {
        printf("EncoderCore: initVideoEncdoe fail!!!\n");
        return -1;
    }
    
    if(!SwsContextInit(pEncodeParam.pVideoInPixelFormat, pEncodeParam.pVideoInWidth, pEncodeParam.pVideoInHeight, pEncodeParam.pVideoOutPixelFormat, pEncodeParam.pVideoOutWidth, pEncodeParam.pVideoOutHeight)) {
        printf("EncoderCore: SwsContextInit fail!!!\n");
        return -1;
    }

    return 0;
}

int EncoderCore::initVideoEncdoe()
{
    // 创建输出码流->创建了一块内存空间->并不知道他是什么类型流->希望他是视频流
    p_VideoStream = avformat_new_stream(p_FormatContext, NULL);
    
    // 设置流的timebase
    p_VideoStream->time_base = (AVRational){1, pEncodeParam.pVideoOutFrameRate};
    p_VideoStream->duration = pDuration;
    pVideoStreamIndex = p_VideoStream->index;

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
    
    //2.7 设置GOP->影响到视频质量问题-画面组-一组连续画面
        //MPEG格式画面类型:3种类型->分为：I帧、P帧、B帧
        //I帧->内部编码帧-原始帧(原始视频数据)
        //      完整画面->关键帧(必须的有，如果没有I，那么就无法进行编码，解码)
        //      视频第1帧 ->视频序列中的第一个帧始终都是I帧，因为它是关键帧
        //P帧->向前预测帧->预测前面的一帧类型，处理数据(前面->I帧，B帧)
        //      P帧数据 - 根据前面的一帧数据->进行处理得到P帧
        //B帧->前后预测帧（双向预测帧）->前面一帧和后面一帧
        //      B帧压缩率高，但是对解码性能要求较高
        //总结：I只需要考虑自己 = 1帧 ，p帧考虑自己+前面一帧 = 2帧,B帧考虑自己+前后帧 = 3帧
        //      P帧和B帧是对I帧压缩
        //每250帧，插入1个I帧，I帧越小，视频越小-默认值-视频不一样
    p_VideoCodecContext->gop_size = 250;
    
    //2.8设置量化参数->数学算法(高级算法)
    // 总结：量化系数小,视频越是清晰
    //  一般情况下都是默认值，最小量化系数默认值是10，最大量化系数默认值是51
    p_VideoCodecContext->max_qdiff = 4;
    p_VideoCodecContext->max_qdiff = 4;

    p_VideoCodecContext->qmin = 10;
    p_VideoCodecContext->qcompress = 1.0;

    p_VideoCodecContext->max_b_frames = 1;

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
        // 量化比例的范围为0～51，其中0为无损模式，23为缺省值，51可能是最差的。该数字越小，图像质量越好。从主观上讲，18~28是一个合理的范围。18往往被认为从视觉上看是无损的，它的输出视频质量和输入视频一模一样或者说相差无几。但从技术的角度来讲，它依然是有损压缩。若Crf值加6，输出码率大概减少一半；若Crf值减6，输出码率翻倍。通常是在保证可接受视频质量的前提下选择一个最大的Crf值，如果输出视频质量很好，那就尝试一个更大的值，如果看起来很糟，那就尝试一个小一点值。
        // 如果设置了AVCodecContext中bit_rate的大小，则采用abr的控制方式
        // 如果没有设置AVCodecContext中的bit_rate，则默认按照crf方式编码，crf默认大小为23
        av_dict_set(&param, "crf", "28.0", 0);
        // 调优
        //key:tune->调优
        //value:zerolatency->零延迟
        av_dict_set(&param, "tune", "zerolatency", 0);
        // H.264有四种画质级别,分别是baseline, extended, main, high：
        av_dict_set(&param, "profile", "main", 0);

    }
    
    // 这个在多线程处理的时候要加锁，之后记住优化这部分代码
    if (avcodec_open2(p_VideoCodecContext, avcodec, &param) < 0) {
        return -1;
    }
    
    return 0;
}

int EncoderCore::initAudioEncdoe()
{
    // 创建输出码流->创建了一块内存空间->并不知道他是什么类型流->希望他是视频流
    p_AudioStream = avformat_new_stream(p_FormatContext, NULL);
    if (!p_AudioStream) {
        return -1;
    }
    p_AudioStream->duration = pDuration;
    
    pAudioStreamIndex = p_AudioStream->index;
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

int EncoderCore::start()
{
    int ret = avformat_write_header(p_FormatContext, NULL);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

int EncoderCore::finish()
{
    int ret = av_write_trailer(p_FormatContext);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

int EncoderCore::AudioEncode(AVFrame *srcframe)
{
    int ret;
    int got_frame = 0;
    AVPacket en_pkt;
    av_init_packet(&en_pkt);
    en_pkt.data = NULL;
    en_pkt.size = 0;
    
    // 将时间转化为流时间
    int64_t apts = av_rescale_q(pCurAudioFrameIndex, p_AudioCodecContext->time_base, p_AudioStream->time_base);
    printf("AudioEncode audio pts %d\n", apts);

    if (srcframe->format != pEncodeParam.pAudioOutSample_fmt ||
        pEncodeParam.pAudioInChannelLayout != pEncodeParam.pAudioOutChannelLayout ||
        srcframe->sample_rate != pEncodeParam.pAudioOutSampleRate ||
        srcframe->nb_samples != pEncodeParam.pAudioOutSampleSize ||
        !p_SwrContex) {
        // 根据pFrame的信息更新重采样器
        if(ResSampleInit(srcframe, pEncodeParam.pAudioInChannelLayout) < 0) {
        }
    }
    
    // 进行重采样过程
    if (p_SwrContex) {
        AVFrame* dst_frame = av_frame_alloc();
        dst_frame->nb_samples = pEncodeParam.pAudioOutSampleSize;
        dst_frame->channel_layout = pEncodeParam.pAudioOutChannelLayout;
        dst_frame->format = pEncodeParam.pAudioOutSample_fmt;
        dst_frame->sample_rate = pEncodeParam.pAudioOutSampleRate;
        dst_frame->pts = apts;
        
        int error = swr_convert_frame(p_SwrContex, dst_frame, srcframe);
        if(error < 0) {
            printf("EncoderCore: swr_convert_frame fail!!!\n");
            return -1;
        }
        // 音频编码之前需要进行重采样
        pCurAudioFrameIndex += dst_frame->nb_samples;
        ret = avcodec_encode_audio2(p_AudioCodecContext, &en_pkt, dst_frame, &got_frame);
        if (ret < 0) {
            av_packet_unref(&en_pkt);
            return -1;
        }
        av_frame_free(&dst_frame);
    } else {
        srcframe->pts = apts;
        pCurAudioFrameIndex += srcframe->nb_samples;
        ret = avcodec_encode_audio2(p_AudioCodecContext, &en_pkt, srcframe, &got_frame);
        if (ret < 0) {
            av_packet_unref(&en_pkt);
            return -1;
        }
    }
    
    if (got_frame) {
        en_pkt.stream_index = pAudioStreamIndex;
        en_pkt.duration = av_rescale_q(pDuration, p_AudioCodecContext->time_base, p_AudioStream->time_base);
        en_pkt.dts = en_pkt.pts;
        printf("audio dts = %ld, pts = %ld, duration = %d\n",en_pkt.dts, en_pkt.pts, en_pkt.duration);

        ret = av_interleaved_write_frame(p_FormatContext, &en_pkt);
        
        pCurAudioFrameIndex++;
    }
    if(en_pkt.data) {
        free(en_pkt.data);
    }
    av_free_packet(&en_pkt);
    return 0;
}

int EncoderCore::VideoEncode(const unsigned char *pdata)
{
    return 1;
}

int EncoderCore::VideoEncode(AVFrame *srcframe)
{
    int ret;
    int got_frame = 0;

    AVPacket en_pkt;
    av_init_packet(&en_pkt);
    en_pkt.data = NULL;
    en_pkt.size = 0;
    
    int dstWidth = pEncodeParam.pVideoOutWidth;
    int dstHeight = pEncodeParam.pVideoOutHeight;
    
    AVFrame* dst_frame = av_frame_alloc();
    dst_frame->format = pEncodeParam.pVideoOutPixelFormat;
    dst_frame->width = dstWidth;
    dst_frame->height = dstHeight;
    
    int dst_bytes_num = avpicture_get_size(pEncodeParam.pVideoOutPixelFormat, dstWidth, dstHeight);
    uint8_t* dst_buff = (uint8_t*)av_malloc(dst_bytes_num);

    avpicture_fill((AVPicture*)dst_frame, dst_buff, pEncodeParam.pVideoOutPixelFormat,
                     dstWidth, dstHeight);
    printf("%d %d %d \n", dst_frame->linesize[0],  dst_frame->linesize[1], dst_frame->linesize[2]);

    if (swsScale(srcframe, dst_frame) == false) {
        av_free(dst_buff);
        av_frame_free(&dst_frame);
        return -1;
    }
    
    // avstream里的 time_base 是单元时间 在av_write_header里设置的
    // AVCodecContex里的 time_base 表示帧率用户设置的
    // 在编码过程中，通过这两个time_base 和 pts 可以计算（av_rescale_q）出编码packet里的实际pts
    dst_frame->pts = av_rescale_q(pCurVideoFrameIndex++, p_VideoCodecContext->time_base, p_VideoStream->time_base);
    printf("VideoEncode video pts %d\n", dst_frame->pts);
    ret = avcodec_encode_video2(p_VideoCodecContext, &en_pkt, dst_frame, &got_frame);
    
    if (ret < 0) {
        av_packet_unref(&en_pkt);
        av_free(dst_buff);
        av_frame_free(&dst_frame);
        return -1;
    }
    
    if (got_frame) {
        en_pkt.stream_index = pVideoStreamIndex;
        en_pkt.duration = pDuration;
        printf("video dts = %ld, pts = %d, duration = %d\n",en_pkt.dts, en_pkt.pts, en_pkt.duration);

        av_interleaved_write_frame(p_FormatContext, &en_pkt);
    }
    
    if(en_pkt.data) {
        free(en_pkt.data);
    }
    
    av_free_packet(&en_pkt);
    av_free(dst_buff);
    av_frame_free(&dst_frame);
    return 0;
}

int EncoderCore::updateVideoFrame(const unsigned char *pdata)
{
    return 0;
}

int EncoderCore::updateAudioFrame(AVFrame *frameIn, AVFrame *frameOut)
{
    
    frameOut->pts = pCurAudioFrameIndex * p_AudioCodecContext->frame_size;
    return 0;
}

int EncoderCore::flushAudioEncode()
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

int EncoderCore::flushVideoEncode()
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


bool EncoderCore::SwsContextInit(AVPixelFormat srcAvFormat, int srcWidth, int srcHeigth, AVPixelFormat dstAvFormat, int dstWidth, int dstHeigth)
{
    if (p_SwsContex) {
        sws_freeContext(p_SwsContex);
        p_SwsContex = NULL;
    }

    p_SwsContex = sws_getContext(srcWidth, srcHeigth, srcAvFormat, dstWidth, dstHeigth, dstAvFormat, SWS_BICUBIC, NULL, NULL, NULL);
    
    if (!p_SwsContex) {
        printf("mediaCore: sws_getCachedContext error\n");
        return false;
    }
    return true;
}


bool EncoderCore::swsScale(AVFrame *inframe, AVFrame *outframe)
{
    if (NULL == p_SwsContex || NULL == inframe || NULL == outframe) {
        printf("mediaCore: frame or sws_ctx is NULL\n");
        return false;
    }
    int height = sws_scale(p_SwsContex,
                              (const uint8_t **)inframe->data,
                              inframe->linesize,
                              0,
                              inframe->height,
                              outframe->data,
                              outframe->linesize
                            );
    if(height <= 0) {
        printf("mediaCore: sws_scale error\n");
        return false;
    }
    return true;
}

//bool EncoderCore::yuvTorgb(AVFrame *inframe, unsigned char *outData, int& nWidth, int& nHeight)
//{
//    if (NULL == sws_ctx || NULL == inframe || NULL == outData
//        || nWidth <= 0 || nHeight <= 0 ) {
//        printf("mediaCore: frame or sws_ctx is NULL\n");
//        return false;
//    }
//    uint8_t *data[AV_NUM_DATA_POINTERS] = {0};
//    data[0] = (uint8_t *)outData;
//    int lines[AV_NUM_DATA_POINTERS] = {0};
//    lines[0] = nWidth * 4;
//    int height = sws_scale(sws_ctx,
//                              (const uint8_t **)inframe->data,
//                              inframe->linesize,
//                              0,
//                              inframe->height,
//                              data,
//                              lines
//                            );
//    if(height <= 0) {
//        printf("mediaCore: sws_scale error\n");
//        return false;
//    }
//    return true;
//}

int EncoderCore::ResSampleInit(AVFrame* inframe, int64_t dec_channel_layout)
{
    if(p_SwrContex == NULL) {
        p_SwrContex = swr_alloc();
    }
    // 释放之前的重采样对象
    swr_free(&p_SwrContex);
    
    // 对音频转上下文设置参数信息
    // 使用pFrame(源)和p_PlayerContext->audioInfoTarget(目标)中的音频参数来设置p_SwrContex
    p_SwrContex = swr_alloc_set_opts(p_SwrContex, pEncodeParam.pAudioOutChannelLayout, pEncodeParam.pAudioOutSample_fmt,
                       pEncodeParam.pAudioOutSampleRate, dec_channel_layout, (AVSampleFormat)inframe->format, inframe->sample_rate,
                       0, NULL);

    
    if (swr_init(p_SwrContex) < 0) {
        av_log(NULL, AV_LOG_ERROR,
               "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
               inframe->sample_rate, av_get_sample_fmt_name((AVSampleFormat)inframe->format), inframe->channels,
               pEncodeParam.pAudioOutSampleRate, av_get_sample_fmt_name(pEncodeParam.pAudioOutSample_fmt), pEncodeParam.pAudioOutChannels);
        swr_free(&p_SwrContex);
        return -1;
    }
    return 1;
}

int EncoderCore::audioResample(uint8_t **out, int out_samples, AVFrame* frame)
{
    int resampled_data_size;
    
    if (!frame || !out) {
        return 0;
    }
    
    const uint8_t **in = (const uint8_t **)frame->extended_data;

    // 音频重采样：返回值是重采样后得到的音频数据中单个声道的样本数
    int len = swr_convert(p_SwrContex, out, out_samples, in, frame->nb_samples);
    if (len < 0) {
        return 0;
    }
    
    // 重采样返回的一帧音频数据大小(以字节为单位)
    resampled_data_size = len * pEncodeParam.pAudioOutChannels * av_get_bytes_per_sample(pEncodeParam.pAudioOutSample_fmt);
    return resampled_data_size;
}


NS_MEDIA_END
