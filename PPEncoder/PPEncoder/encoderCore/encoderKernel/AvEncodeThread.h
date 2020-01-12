//
//  AvEncodeThread.h
//  PPEncoder
//
//  Created by 邱开禄 on 2020/01/05.
//  Copyright © 2020 邱开禄. All rights reserved.
//

#ifndef AvEncodeThread_H
#define AvEncodeThread_H
#include "MediaDefineInfo.h"
#include <thread>
#include "EventHandler.h"
#include "EncoderCore.h"
#include "mediaCore.h"
NS_MEDIA_BEGIN
class AvEncodeThread : public std::thread
{
public:
    AvEncodeThread();
    ~AvEncodeThread();
    bool init(PlayerContext *playerContext, EventHandler *handler, EncodeParam params, const char *outFile);

    bool start();

    void run();
    
    bool finish();
    
    bool stop();
    
    /*
     * 将msg指令入队列
     */
    bool queueMessage(msgInfo msg);
private:
    Frame *getOneValidAudioFrame();
    
private:
//    PacketQueue         videoEncodeRingBuffer;  // 存储编码后的packet video队列
//    PacketQueue         audioEncodeRingBuffer;  // 存储编码后的packet audio队列
    PlayerContext       *pPlayerContext;
    EventHandler        *pHandler;
    EncoderCore         *pEncoder;
    bool                pPause;                   // 当前是否是pause状态
    bool                pNeedStop;
    message             *pMessageQueue;               // 当前的message信息
    msgInfo             pCurMessage;                    // 当前的播放状态
};
NS_MEDIA_END
#endif // AvEncodeThread_H
