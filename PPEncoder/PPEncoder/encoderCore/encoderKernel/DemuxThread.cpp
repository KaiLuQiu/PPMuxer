//
//  DemuxThread.cpp
//  PPEncoder
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#include "Demuxthread.h"

NS_MEDIA_BEGIN

// 对于packetQueue中我们需要在一个向队列中先放置一个flush_pkt，主要用来作为非连续的两端数据的“分界”标记
// 大概是为了每次seek操作后插入flush_pkt，更新serial，开启新的播放序列

DemuxThread::DemuxThread()
{
    pHandler = NULL;
    seek_by_bytes = -1;
    pNeedStop = false;
    videoPackeQueueFunc = NULL;
    audioPackeQueueFunc = NULL;
    pSeek = false;
    pSeekPos = -1;
    pMessageQueue = new message();
    if (NULL == pMessageQueue) {
        printf("message is NULL!!!\n");
    }
    pMutex = SDL_CreateMutex();
}

DemuxThread::~DemuxThread()
{
    SAFE_DELETE(pMessageQueue);
    if (pMutex) {
        SDL_DestroyMutex(pMutex);
        pMutex = NULL;
    }
}

bool DemuxThread::init(PlayerContext *playerContext, EventHandler *handler, mediaCore *p_Core)
{
    if (NULL == handler || NULL == playerContext || NULL == p_Core)
        return false;
    pHandler = handler;
    pPlayerContext = playerContext;
    pMediaCore = p_Core;
    videoRingBuffer = &playerContext->videoRingBuffer;
    audioRingBuffer = &playerContext->audioRingBuffer;

    video_flush_pkt = pPlayerContext->video_flush_pkt;

    audio_flush_pkt = pPlayerContext->audio_flush_pkt;
    
    if(pPlayerContext->videoStreamIndex >= 0)
    {
        pPlayerContext->videoPacketQueueFunc = new (std::nothrow)PacketQueueFunc(video_flush_pkt);
        if(!pPlayerContext->videoPacketQueueFunc)
        {
            printf("pPlayerContext->videoPackeQueueFunc error!\n");
        }
        videoPackeQueueFunc = pPlayerContext->videoPacketQueueFunc;
        
    }
    if(pPlayerContext->audioStreamIndex >= 0)
    {
        pPlayerContext->audioPacketQueueFunc = new (std::nothrow)PacketQueueFunc(audio_flush_pkt);
        if(!pPlayerContext->audioPacketQueueFunc)
        {
            printf("pPlayerContext->audioPacketQueueFunc error!\n");
        }
        audioPackeQueueFunc = pPlayerContext->audioPacketQueueFunc;
    }
    
    videoPackeQueueFunc->packet_queue_init(videoRingBuffer);
    audioPackeQueueFunc->packet_queue_init(audioRingBuffer);
    return true;
}

void DemuxThread::flush()
{
    
}

void DemuxThread::start()
{
    thread read_thread([this]()-> void {              //此处使用lamda表达式
        run();
    });
    read_thread.detach();
}

void DemuxThread::stop()
{
    pNeedStop = true;
}

void DemuxThread::run()
{
    int ret = -1;
    int64_t stream_start_time;          //流的开始时间
    int pkt_in_play_range = 0;          //当前的duation时间
    AVDictionaryEntry *t;
    AVPacket pkt;
    memset(&pkt, 0, sizeof(AVPacket));
    
    while(!pNeedStop)
    {
        if(!pPlayerContext)             //表示当前的播放器上下文还没有准备好，可以先delay10ms
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            printf("pPlayerContext is NUll\n");
            continue;
        }
        if (pMessageQueue != NULL) {
            pMessageQueue->message_dequeue(pCurMessage);
            if (MESSAGE_CMD_SEEK == pCurMessage.cmd) {
                pSeek = true;
                pSeekPos = pCurMessage.data;
            }
        }
        if (true == pSeek) {
            if (pSeekPos != -1) {
                ret = pMediaCore->Seek(pSeekPos, AVSEEK_FLAG_BACKWARD);
                if (ret >= 0) {
                    // 清空audio packet队列
                    audioPackeQueueFunc->packet_queue_flush(audioRingBuffer);
                    audioPackeQueueFunc->packet_queue_put(audioRingBuffer, pPlayerContext->audio_flush_pkt);
                    // 清空video packet队列
                    videoPackeQueueFunc->packet_queue_flush(videoRingBuffer);
                    videoPackeQueueFunc->packet_queue_put(videoRingBuffer, pPlayerContext->video_flush_pkt);
                    // 发送seek完成的消息
                    if (NULL != pHandler)
                        pHandler->sendOnSeekCompletion();
                } else {
                    // 发送seek 失败的消息
                    if (NULL != pHandler)
                        pHandler->sendOnSeekFail();
                }
            }
            pSeekPos = -1;
            pSeek = false;
        }

        if(videoPackeQueueFunc == NULL || audioPackeQueueFunc == NULL)
        {
            printf("PackeQueueFunc is NUll\n");
            break;
        }
        if(videoRingBuffer->size + audioRingBuffer->size > MAX_SIZE)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            printf("ringbuffer is full\n");
            continue;
        }
        
        int ret = av_read_frame(pPlayerContext->ic, &pkt);
        if(ret < 0)
        {
            if ((ret == AVERROR_EOF || avio_feof(pPlayerContext->ic->pb)) && !pPlayerContext->eof) {
                if (pPlayerContext->videoStreamIndex >= 0) {
                    // 这边推2笔空buff是因为，第一笔是为了获取eof，第二笔只是暂时处理decodeThread的bug
                    videoPackeQueueFunc->packet_queue_put_nullpacket(videoRingBuffer, pPlayerContext->videoStreamIndex);
                    videoPackeQueueFunc->packet_queue_put_nullpacket(videoRingBuffer, pPlayerContext->videoStreamIndex);
                }
                if (pPlayerContext->audioStreamIndex >= 0) {
                    audioPackeQueueFunc->packet_queue_put_nullpacket(audioRingBuffer, pPlayerContext->audioStreamIndex);
                    audioPackeQueueFunc->packet_queue_put_nullpacket(audioRingBuffer, pPlayerContext->audioStreamIndex);
                }
                pPlayerContext->eof = 1;
            }
//            if (pPlayerContext->ic->pb && pPlayerContext->ic->pb->error)
            //让线程等待10ms
//            SDL_LockMutex(pMutex);
//            std::this_thread::sleep_for(std::chrono::milliseconds(10));
//            SDL_UnlockMutex(pMutex);
            continue;
        }

        if (pkt.stream_index == pPlayerContext->audioStreamIndex)
        {
            audioPackeQueueFunc->packet_queue_put(audioRingBuffer, &pkt);
        }
        else if (pkt.stream_index == pPlayerContext->videoStreamIndex)
        {
            videoPackeQueueFunc->packet_queue_put(videoRingBuffer, &pkt);
        }
        else
        {
            av_packet_unref(&pkt);
        }
                
        
    }
}

void DemuxThread::setSeekType(int type)
{
    seek_by_bytes = type;
}

bool DemuxThread::queueMessage(msgInfo msg)
{
    if (NULL == pMessageQueue) {
        printf("message is NULL!!!");
        return false;
    }
    pMessageQueue->message_queue(msg);
    return true;
}

NS_MEDIA_END
