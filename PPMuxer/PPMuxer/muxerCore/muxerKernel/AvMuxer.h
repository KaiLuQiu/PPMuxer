//
//  AvMuxer.h
//  PPMuxer
//
//  Created by 邱开禄 on 2019/12/31.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#ifndef AvMuxer_H
#define AvMuxer_H
#include "MediaCommon.h"

NS_MEDIA_BEGIN

class AvMuxer {
public:
    AvMuxer();
    ~AvMuxer();
    
    /*
     * AvMuxer初始化
     */
    int init(const char * fileName, int nWidth, int nHeight);
    
    /*
     * 开始编码
     */
    int start();
    
    /*
     * 结束编码
     */
    int stop();
    
    /*
     * 输入data进行编码过程
     */
    int VideoEncode(const unsigned char *pdata);
private:
    
    /*
     * flush编码器
     */
    int flushVideoEncode();
    
    int allocFrame();
    
    AVCodecContext      *p_VideoCodecContext;   // 解码上下文信息
    AVFormatContext     *p_FormatContext;       // 媒体流format上下文信息
    AVOutputFormat      *p_OutFormat;           // 输出格式的信息
    AVStream            *p_VideoStream;         // 视频流
    AVFrame             *p_VideoFrame;          // 解码的视频帧
    SwsContext          *p_SwsContex;
    long                pCurVideoFrameIndex;    // 当前视频编码的帧索引
    int                 pDuration;              // 视频总时长
    const char          *pFileName;             // 视频文件
};

NS_MEDIA_END

#endif // AvMuxer_H
