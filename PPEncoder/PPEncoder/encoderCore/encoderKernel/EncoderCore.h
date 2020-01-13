//
//  EncoderCore.h
//  PPEncoder
//
//  Created by 邱开禄 on 2019/12/31.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#ifndef EncoderCore_H
#define EncoderCore_H
#include "MediaCommon.h"
#include "mediaCore.h"

NS_MEDIA_BEGIN
class EncoderCore {
public:
    EncoderCore();
    ~EncoderCore();
    
    /*
     * EncoderCore初始化
     */
    int init(const char* fileName, int duration, EncodeParam param);
    
    /*
     * 开始编码
     */
    int start();
    
    /*
     * 结束编码
     */
    int finish();
    
    /*
     * 输入data进行video编码过程
     */
    int VideoEncode(const unsigned char *pdata);
    
    /*
     * 输入frame进行video编码过程
     */
    int VideoEncode(AVFrame *srcframe);

    /*
     * 输入frame进行audio编码过程
     */
    int AudioEncode(AVFrame *srcframe);
    
    /*
     * flush编码器
     */
    int flushVideoEncode();
    
    /*
     * flush编码器
     */
    int flushAudioEncode();
    
    int audioResample(uint8_t **out, int out_samples, AVFrame* frame);

    bool swsScale(AVFrame *inframe, AVFrame *outframe);
    
    bool yuvTorgb(AVFrame *inframe, unsigned char *outData, int& nWidth, int& nHeight);
private:
    int ResSampleInit(AVFrame* inframe, int64_t dec_channel_layout);

    bool SwsContextInit(AVPixelFormat srcAvFormat,  int srcWidth, int srcHeigth, AVPixelFormat dstAvFormat, int dstWidth, int dstHeigth);
    /*
     * 初始化audio编码器
     */
    int initAudioEncdoe();
    
    /*
     * 初始化video编码器
     */
    int initVideoEncdoe();
    
    /*
     * data转frame
     */
    int updateVideoFrame(const unsigned char *pdata);

    /*
     * data转frame
     */
    int updateAudioFrame(AVFrame *frameIn, AVFrame *frameOut);
    
    AVCodecContext      *p_VideoCodecContext;   // video解码上下文信息
    AVCodecContext      *p_AudioCodecContext;   // Audio解码上下文信息
    
    AVStream            *p_VideoStream;         // 视频流
    AVStream            *p_AudioStream;         // 音频流
    
    AVFormatContext     *p_FormatContext;       // 媒体流format上下文信息

    SwsContext          *p_SwsContex;
    // 音视频转码上下文
    SwrContext          *p_SwrContex;
    
    AVOutputFormat      *p_OutFormat;           // 输出格式的信息
    AVFrame             *p_AudioFrame;          // 解码的音频帧

    long                pCurVideoFrameIndex;    // 当前视频编码的帧索引
    long                pCurAudioFrameIndex;    // 当前音频编码的帧索引
    
    int                 pDuration;              // 视频总时长
    const char          *pFileName;             // 视频文件
    
    int                 pWidth;
    int                 pHeight;
    
    EncodeParam         pEncodeParam;
    
    int                 pVideoStreamIndex;
    int                 pAudioStreamIndex;
};

NS_MEDIA_END

#endif // EncoderCore_H
