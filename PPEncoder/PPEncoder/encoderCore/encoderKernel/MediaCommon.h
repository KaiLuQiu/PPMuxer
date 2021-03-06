//
//  MediaCommon.h
//  PPEncoder
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#ifndef MediaCommon_H
#define MediaCommon_H

#include <list>
#include <vector>
#include <string.h>
#include <stdlib.h>

extern "C"{
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/rational.h>
#include <libavutil/time.h>
#include <libavutil/samplefmt.h>
#include <libavutil/imgutils.h>
}

#include "SDL.h"
#include <SDL_mutex.h>
#include <SDL_thread.h>
#include <SDL_timer.h>
#include <SDL_mutex.h>
#include <SDL_audio.h>
#include <SDL_main.h>
#include <SDL_pixels.h>

#define NS_MEDIA_BEGIN namespace media {
#define NS_MEDIA_END  }

//#define FRAME_QUEUE_SIZE 24
// 这边的QUEUE_SIZE最好为3，否则将会导致decode thread中的丢帧效果失效，原因是queue size过大，里面的数据量多，导致VD和VO两者直接使用的最新一笔数据pts差值变大，从而导致frame_drops_early失效
#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 18
#define PCM_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))

#ifndef SAFE_AV_FREE
#define SAFE_AV_FREE(p) if(p != NULL) {av_free(p); p = NULL;}
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(x) { if (x) delete (x); (x) = NULL; }    //定义安全释放函数
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(x) { if (x) delete [] (x); (x) = NULL; }    //定义安全释放函数
#endif
#ifndef SAFE_FREE
#define SAFE_FREE(p) if(p != NULL) {free(p); p = NULL;}
#endif

typedef enum {
    PLAYER_MEDIA_NOP,
    PLAYER_MEDIA_PREPARED,
    PLAYER_MEDIA_PLAYBACK_COMPLETE,
    PLAYER_MEDIA_BUFFERING_UPDATE,
    PLAYER_MEDIA_SEEK_COMPLETE,
    PLAYER_MEDIA_SET_VIDEO_SIZE,
    PLAYER_MEDIA_TIMED_TEXT,
    PLAYER_MEDIA_ERROR,
    PLAYER_MEDIA_INFO,

    PLAYER_MEDIA_PAUSE,
    PLAYER_MEDIA_RESUME,
    PLAYER_MEDIA_SEEK,
    PLAYER_MEDIA_SEEKING,
    PLAYER_MEDIA_SEEK_FAIL,
    PLAYER_MEDIA_START,
    PLAYER_MEDIA_STOP,
    PLAYER_MEDIA_FLUSH,
    PLAYER_MEDIA_FORCE_EOF,
    PLAYER_MEDIA_REINIT,
}PlayerState;


typedef struct P_AVPacket_T {
    P_AVPacket_T()
    {
        memset(&pkt, 0, sizeof(AVPacket));
        serial = 0;
    }
    ~P_AVPacket_T()
    {
        serial = 0;
    }
    AVPacket pkt;           //demuxer parse出来的包信息
    int serial;             //序号信息
} P_AVPacket;

//packtet队列信息
typedef struct PacketQueue_T {
    PacketQueue_T()
    {
        AvPacketList.clear();
        nb_packets = 0;
        size = 0;
        duration = 0;
        abort_request = 0;
        serial = 0;
        mutex = SDL_CreateMutex();
        cond = SDL_CreateCond();
    }
    ~PacketQueue_T()
    {
        auto item = AvPacketList.begin();
        for(; item != AvPacketList.end(); )
        {
            auto item_e = item++;
            SAFE_DELETE(*item_e);
            AvPacketList.erase(item_e);
        }
        AvPacketList.clear();
        
        nb_packets = 0;
        size = 0;
        duration = 0;
        abort_request = 0;
        serial = 0;
        SDL_DestroyMutex(mutex);
        SDL_DestroyCond(cond);
    }
    std::list<P_AVPacket *> AvPacketList;
    int nb_packets;         // 队列中packet的数量
    int size;               // 队列所占内存空间大小
    int64_t duration;       // 队列中所有packet总的播放时长
    int abort_request;
    int serial;             // 播放序列，所谓播放序列就是一段连续的播放动作，一个seek操作会启动一段新的播放序列
    SDL_mutex *mutex;
    SDL_cond  *cond;
} PacketQueue;

typedef struct AudioInfo_T {
    AudioInfo_T()
    {
        freq = 0;
        channels = 0;
        channel_layout = 0;
        fmt = AV_SAMPLE_FMT_NONE;
        frame_size = 0;
        bytes_per_sec = 0;
        sample_rate = 0;
    }
    ~AudioInfo_T()
    {
        freq = 0;
        channels = 0;
        channel_layout = 0;
        fmt = AV_SAMPLE_FMT_NONE;
        frame_size = 0;
        bytes_per_sec = 0;
        sample_rate = 0;
    }
    int freq;
    int channels;
    int64_t channel_layout;
    enum AVSampleFormat fmt;
    int frame_size;
    int bytes_per_sec;          //每秒的
    int sample_rate;
} AudioInfo;

//解码帧信息
typedef struct Frame {
    Frame()
    {
        frame = NULL;
        memset(&sub, 0, sizeof(AVSubtitle));
        serial = -1;
        pts = -1;
        duration = -1;
        pos = -1;
        width = -1;
        height = -1;
        format = -1;
        memset(&sar, 0, sizeof(AVRational));
        uploaded = -1;
        flip_v = -1;
    }
    ~Frame()
    {
        if (frame) {
            av_frame_free(&frame);
            frame = NULL;
        }
        memset(&sub, 0, sizeof(AVSubtitle));
        serial = -1;
        pts = -1;
        duration = -1;
        pos = -1;
        width = -1;
        height = -1;
        format = -1;
        memset(&sar, 0, sizeof(AVRational));
        uploaded = -1;
        flip_v = -1;
    }
    AVFrame *frame;
    AVSubtitle sub;
    int serial;
    double pts;           //解码时间戳
    double duration;      //帧时长信息
    int64_t pos;          //在码流的位置信息
    int width;            //视频的宽
    int height;           //视频的高
    int format;           //图像的格式
    AVRational sar;
    int uploaded;
    int flip_v;           //镜像
} Frame;

typedef struct FrameQueue {
    FrameQueue()
    {
        rindex = 0;
        windex = 0;
        size = 0;
        max_size = 0;
        keep_last = 0;
        rindex_shown = 0;
        mutex = SDL_CreateMutex();
        cond = SDL_CreateCond();
        pktq = NULL;
    }
    ~FrameQueue()
    {
        for(int i = 0; i < FRAME_QUEUE_SIZE; i++) {
            Frame FrameData = queue[i];
            if(FrameData.frame) {
                av_frame_free(&FrameData.frame);
                FrameData.frame = NULL;
            }
        }
        rindex = 0;
        windex = 0;
        size = 0;
        max_size = 0;
        keep_last = 0;
        rindex_shown = 0;
        if(mutex != NULL)
        {
            SDL_DestroyMutex(mutex);
            mutex = NULL;
        }
        if(cond != NULL)
        {
            SDL_DestroyCond(cond);
            cond = NULL;
        }
        SAFE_DELETE(pktq);
    }
    Frame queue[FRAME_QUEUE_SIZE];
    int rindex;
    int windex;
    int size;             // 总帧数
    int max_size;
    int keep_last;        // keep_last是一个bool值，表示是否在环形缓冲区的读写过程中保留最后一个读节点不被覆写
    int rindex_shown;
    SDL_mutex *mutex;
    SDL_cond *cond;
    PacketQueue *pktq;
} FrameQueue;

typedef struct DecoderContext_T {
    DecoderContext_T()
    {
        codecContext = NULL;
        pkt_serial = -1;
        finished = 0;
        packet_pending = 0;
        start_pts = 0;
        next_pts = 0;
    }
    ~DecoderContext_T()
    {
        if(codecContext) {
            avcodec_free_context(&codecContext);
            codecContext = NULL;
        }
        pkt_serial = -1;
        finished = 0;
        packet_pending = 0;
        start_pts = 0;
        next_pts = 0;
    }
    AVCodecContext *codecContext;
    AVPacket pkt;
    int pkt_serial;
    int finished;
    int packet_pending;
    int64_t start_pts;
    AVRational start_pts_tb;
    int64_t next_pts;
    AVRational next_pts_tb;
} DecoderContext;

typedef struct EncodeParam_T{
    EncodeParam_T() {
        pVideoInPixelFormat = AV_PIX_FMT_YUV420P;
        pVideoInWidth = -1;
        pVideoInHeight = -1;
        pVideoOutPixelFormat = AV_PIX_FMT_YUV420P;
        pVideoOutCodecId = AV_CODEC_ID_H264;
        pVideoOutFrameRate = 30;
        pVideoOutWidth = 1080;
        pVideoOutHeight = 1920;
        pAudioOutChannels = 2;
        pAudioOutChannelLayout = AV_CH_LAYOUT_STEREO;
        pAudioOutCodecId = AV_CODEC_ID_AAC;
        pAudioOutSampleSize = 1024;
        pAudioOutSampleRate = 44100;
        // float, planar
        pAudioOutSample_fmt = AV_SAMPLE_FMT_FLTP;
        pAudioInChannelLayout = AV_CH_LAYOUT_STEREO;
    }
    ~EncodeParam_T() {
        
    }
    AVPixelFormat   pVideoInPixelFormat;
    int             pVideoInWidth;
    int             pVideoInHeight;
    AVPixelFormat   pVideoOutPixelFormat;
    AVCodecID       pVideoOutCodecId;
    int             pVideoOutFrameRate;
    int             pVideoOutWidth;
    int             pVideoOutHeight;
    int             pAudioOutChannels;
    uint64_t        pAudioOutChannelLayout;
    AVCodecID       pAudioOutCodecId;
    int             pAudioOutSampleSize;
    int             pAudioOutSampleRate;
    AVSampleFormat  pAudioOutSample_fmt;
    uint64_t        pAudioInChannelLayout;
}EncodeParam;
#endif

