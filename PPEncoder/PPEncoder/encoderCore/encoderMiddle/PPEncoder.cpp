//
//  PPEncoder.cpp
//  PPEncoder
//
//  Created by 邱开禄 on 2020/01/05.
//  Copyright © 2020 邱开禄. All rights reserved.
//

#include "PPEncoder.h"
NS_MEDIA_BEGIN

PPEncoder* PPEncoder::p_Encoder = nullptr;
// 类的静态指针需要在此初始化
SDL_mutex* PPEncoder::p_Mutex = SDL_CreateMutex();

PPEncoder::PPEncoder():
pUrl(""),
pOutFilePath("")
{
    p_Handler = NULL;
    pPlayerContext = new (std::nothrow)PlayerContext();
    if (NULL == pPlayerContext) {
        printf("PPEncoder: new playerInfo fail!!! \n");
    }
    pPlayerContext->volumeValue = 50.0;
    
    p_VideoDecoderThread = new (std::nothrow)VideoDecodeThread();
    if (NULL == p_VideoDecoderThread) {
        printf("PPEncoder: new VideoDecodeThread fail!!! \n");
    }
    
    p_DemuxerThread = new (std::nothrow)DemuxThread();
    if (NULL == p_DemuxerThread) {
        printf("PPEncoder: new DemuxThread fail!!! \n");
    }
    
    p_AudioDecoderThread = new (std::nothrow)AudioDecodeThread();
    if (NULL == p_AudioDecoderThread) {
        printf("PPEncoder: new AudioDecodeThread fail!!! \n");
    }
    
    p_EncoderThread = new (std::nothrow)AvEncodeThread();
    if (NULL == p_EncoderThread) {
        printf("PPEncoder: new mediaCore fail!!! \n");
    }
    
    p_MediaCore = new (std::nothrow)mediaCore();
    if (NULL == p_MediaCore) {
        printf("PPEncoder: new mediaCore fail!!! \n");
    }
}

PPEncoder::~PPEncoder()
{
    SAFE_DELETE(pPlayerContext);
    SAFE_DELETE(p_VideoDecoderThread);
    SAFE_DELETE(p_DemuxerThread);
    SAFE_DELETE(p_AudioDecoderThread);
    SAFE_DELETE(p_MediaCore);
    SAFE_DELETE(p_EncoderThread);
}

void PPEncoder::setHandle(EventHandler *handle)
{
    p_Handler = handle;
}

// 这边暂时只保留url信息
void PPEncoder::setDataSource(std::string url)
{
    pUrl = url;
}

void PPEncoder::setOutFilePath(std::string path)
{
    pOutFilePath = path;
}

void PPEncoder::setEncodeParam(EncodeParam &params)
{
    pEncodeParams = params;
}

bool PPEncoder::prepareAsync()
{
    if (NULL == p_Handler && NULL == pPlayerContext) {
        printf("PPlayer: prepareAsync error pHandler or pPlayerContext is NULL!!!\n");
        return false;
    }
    if (NULL == p_MediaCore || NULL == p_DemuxerThread || NULL == p_VideoDecoderThread
        || NULL == p_AudioDecoderThread) {
        printf("PPlayer: p_Core or p_Demuxer or p_VideoDecoder or p_AudioDecoder is NULL\n");
        return false;
    }
    p_MediaCore->Init(pPlayerContext, p_Handler);
    // avformat和avcodec都打开了
    bool ret = p_MediaCore->StreamOpen(pUrl);
    if(ret == true)
    {
        p_DemuxerThread->init(pPlayerContext, p_Handler, p_MediaCore);
        // 初始化videodecoder，主要是startPacketQueue
        p_VideoDecoderThread->init(pPlayerContext, p_Handler, p_MediaCore);
        // 初始化videodecoder，主要是startPacketQueue
        p_AudioDecoderThread->init(pPlayerContext, p_Handler, p_MediaCore);
        
        p_EncoderThread->init(pPlayerContext, p_Handler, p_MediaCore, pEncodeParams, pOutFilePath.c_str());
        // 开启demuxer线程读取数据包
        p_DemuxerThread->start();
        // videoDecode和audioDecode可以在prepareAsync的时候就开启，当显示线程则不可。为了加快第一帧的show
        p_VideoDecoderThread->start();
        p_AudioDecoderThread->start();
    }
    // 这边一般要render第一帧之后才能上发prepared消息
    p_Handler->sendOnPrepared();
    return true;
}

void PPEncoder::prepare()
{
    
}

bool PPEncoder::start()
{
    p_EncoderThread->start();
    p_Handler->sendOnStart();
    return true;
}

bool PPEncoder::pause(bool state)
{
    if (NULL == p_DemuxerThread) {
        printf("PPEncoder: p_Demuxer is NULL\n");
        return false;
    }
    msgInfo msg;
    // 暂停播放
    if(true == state) {
        msg.cmd = MESSAGE_CMD_PAUSE;
        msg.data = -1;
        p_DemuxerThread->queueMessage(msg);
    }
    else {
        msg.cmd = MESSAGE_CMD_START;
        msg.data = -1;
        p_DemuxerThread->queueMessage(msg);
    }
    
    return true;
}

int PPEncoder::seek(float pos)
{
    int ret;
    if (NULL == p_DemuxerThread) {
        printf("PPlayer: p_Demuxer is NULL\n");
        return -1;
    }
    if (PLAYER_MEDIA_NOP != pPlayerContext->playerState) {
        msgInfo msg;
        msg.cmd = MESSAGE_CMD_SEEK;
        msg.data = pos;
        // seek到当前位置的后一个I frame
        p_DemuxerThread->queueMessage(msg);
    } else {
        ret = -1;
    }
    return ret;
}

bool PPEncoder::resume()
{
    return true;
}

bool PPEncoder::stop()
{
    return true;
}

void PPEncoder::flush()
{
    
}

bool PPEncoder::setLoop(bool loop)
{
    return true;
}

long PPEncoder::getDuration()
{
    PlayerContext* playerInfo = pPlayerContext;

     if (!playerInfo || !playerInfo->ic)
        return 0;
    int64_t duration = av_rescale(playerInfo->ic->duration, 1000, AV_TIME_BASE);
    if (duration < 0)
        return 0;
    
    return (long)duration;
}

void PPEncoder::pp_get_msg(Message& msg)
{
    switch(msg.m_what){
    case PLAYER_MEDIA_NOP:
        break;

    case PLAYER_MEDIA_SEEK:
        break;

    case PLAYER_MEDIA_PREPARED:
            
        pPlayerContext->playerState = PLAYER_MEDIA_PREPARED;
        break;
    case PLAYER_MEDIA_SEEK_COMPLETE:

        break;
    case PLAYER_MEDIA_SEEK_FAIL:
        break;

    case PLAYER_MEDIA_PLAYBACK_COMPLETE:
            
        break;
    case PLAYER_MEDIA_SET_VIDEO_SIZE:
            
        break;
    case PLAYER_MEDIA_ERROR:
            
        break;
    case PLAYER_MEDIA_INFO:
            
        break;
    case PLAYER_MEDIA_PAUSE:
        
        pPlayerContext->playerState = PLAYER_MEDIA_PAUSE;
        break;
    case PLAYER_MEDIA_START:
            
        pPlayerContext->playerState = PLAYER_MEDIA_START;
        break;
    default:
        break;
    }
}
NS_MEDIA_END
