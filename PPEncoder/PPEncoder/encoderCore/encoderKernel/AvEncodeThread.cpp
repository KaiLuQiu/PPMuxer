//
//  AvEncodeThread.cpp
//  PPEncoder
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#include "AvEncodeThread.h"
#include "FrameQueueFunc.h"

NS_MEDIA_BEGIN
AvEncodeThread::AvEncodeThread():
pNeedStop(0),
pPause(false)
{
    pPlayerContext = NULL;
    pHandler = NULL;
    pEncoder = new (std::nothrow)AvEncoder();
    if(!pEncoder) {
        printf("AvEncodeThread: new pEncoder fail!!! \n");
    }
    pMessageQueue = new (std::nothrow)message();
    if (NULL == pMessageQueue) {
        printf("VideoRefreshThread: message is NULL!!!\n");
    }
    pCurMessage.cmd = MESSAGE_CMD_NONE;
    pCurMessage.data = -1;
}

AvEncodeThread::~AvEncodeThread()
{
    SAFE_DELETE(pEncoder);
}

bool AvEncodeThread::init(PlayerContext *playerContext, EventHandler *handler, EncodeParam params, const char *outFile)
{
    if (NULL == handler || NULL == playerContext
        || NULL == pEncoder || NULL == outFile)
        return false;
    pPlayerContext = playerContext;
    pHandler = handler;
    
    int duration = playerContext->ic->duration;
    int nWidth = playerContext->width;
    int nHeight = playerContext->height;
    
    pEncoder->init(outFile, nWidth, nHeight, duration, params);
    
    return true;
}

bool AvEncodeThread::start()
{
    if(NULL == pEncoder) {
        return false;
    }
    pEncoder->start();
    thread Av_encode_thread([this]()-> void {              //此处使用lamda表达式
        run();
    });
    Av_encode_thread.detach();
    return true;
}

void AvEncodeThread::run()
{
    while(!pNeedStop)
    {
        if (pMessageQueue != NULL) {
            pMessageQueue->message_dequeue(pCurMessage);
            if (MESSAGE_CMD_PAUSE == pCurMessage.cmd) {
                // 发送暂停状态的消息
                if (NULL != pHandler)
                    pHandler->sendOnPause();
                pPause = true;
            } else if(MESSAGE_CMD_START == pCurMessage.cmd) {
                // 发送开始播放的消息
                if (NULL != pHandler)
                    pHandler->sendOnStart();
                pPause = false;
            }
        }
        
        if (!pPlayerContext)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        // 如果当前的进入pause状态则进入等待阶段
        if (true == pPause) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        // 说明当前存在视频流
        if (pPlayerContext->ic->streams[pPlayerContext->videoStreamIndex])
        {
            // 判断decoder queue中是否存在数据
            if (FrameQueueFunc::frame_queue_nb_remaining(&pPlayerContext->videoDecodeRingBuffer) == 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                // 如果没有则，delay
                continue;
            }
            else
            {
                Frame *vp;
                // 获取下一笔要现实的帧
                vp = FrameQueueFunc::frame_queue_peek(&pPlayerContext->videoDecodeRingBuffer);
                
                // 当前的这笔数据流不连续，则跳过获取下一笔
                if (vp->serial != pPlayerContext->videoRingBuffer.serial) {
                    FrameQueueFunc::frame_queue_next(&pPlayerContext->videoDecodeRingBuffer);
                    continue;
                }

                //暂时不做av sync操作，带音频模块的接入
                FrameQueueFunc::frame_queue_next(&pPlayerContext->videoDecodeRingBuffer);
                pEncoder->VideoEncode(vp->frame);
            }
        }
        
        // 说明当前存在视频流
        if (pPlayerContext->ic->streams[pPlayerContext->audioStreamIndex])
        {
            Frame *pFrame = getOneValidAudioFrame();
            if(NULL == pFrame) {
                continue;
            }
        }
    }
}

// 从FrameBufferQueue中获取一个有效的frame
Frame *AvEncodeThread::getOneValidAudioFrame()
{
    Frame *pFrame = NULL;

    if (!pPlayerContext) {
        return NULL;
    }
    
    // 这边是为了获取一个连续且帧
    do {
        // 获取当前待显示的帧，如果条件不满足则进入wait状态
        if (!(pFrame = FrameQueueFunc::frame_queue_peek_readable(&pPlayerContext->audioDecodeRingBuffer))) {
            return NULL;
        }
        // 获取了一帧之后，要把当前着帧销毁，读索引要++
        FrameQueueFunc::frame_queue_next(&pPlayerContext->audioDecodeRingBuffer);
    } while (pFrame->serial != pPlayerContext->audioRingBuffer.serial);
    return pFrame;
}

bool AvEncodeThread::finish()
{
    if(NULL == pEncoder) {
        return false;
    }
    pEncoder->finish();
    return true;
}

bool AvEncodeThread::stop()
{
    pNeedStop = 1;
    return true;
}

bool AvEncodeThread::queueMessage(msgInfo msg)
{
    if (NULL == pMessageQueue) {
        printf("message is NULL!!!\n");
        return false;
    }
    pMessageQueue->message_queue(msg);
    return true;
}


NS_MEDIA_END
