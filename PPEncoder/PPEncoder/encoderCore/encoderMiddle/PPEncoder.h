//
//  PPEncoder.h
//  PPEncoder
//
//  Created by 邱开禄 on 2020/01/05.
//  Copyright © 2020 邱开禄. All rights reserved.
//

#ifndef PPEncoder_H
#define PPEncoder_H
#include <string>
#include "MediaDefineInfo.h"
#include "mediaCore.h"
#include "Demuxthread.h"
#include "VideoDecodeThread.h"
#include "AudioDecodeThread.h"
#include "AvEncodeThread.h"
#include "EventHandler.h"

NS_MEDIA_BEGIN

class PPEncoder
{
public:
    /*
     * PPEncoder单例模式：饿汉模式
     */
    static PPEncoder *getInstance() {
        if(NULL == p_Encoder) {
            SDL_LockMutex(p_Mutex);
            if(NULL == p_Encoder) {
                p_Encoder = new (std::nothrow)PPEncoder();
                if(p_Encoder == NULL) {
                    printf("PPlayer getInstance is NULL!\n");
                }
            }
            SDL_UnlockMutex(p_Mutex);
        }
        return p_Encoder;
    }
  
    /*
     * 设置url
     */
    void setDataSource(std::string url);
    
    void setOutFilePath(std::string path);
    
    void setEncodeParam(EncodeParam &params);
    /*
     * 执行播放器的prepareAsync状态，此过程会开启demuxer, audiodecode videodecode等线程。做好初始化工作。
     */
    bool prepareAsync();

    void prepare();
    /*
     * 当收到prepared之后，上层播放器可以设置start状态开始播放视频
     */
    bool start();
    
    /*
     * 进入暂停状态
     */
    bool pause(bool state);
    
    /*
     * 进行seek的过程
     */
    int seek(float pos);
    
    /*
     * 进行resume重新同步过程
     */
    bool resume();

    /*
     * stop播放器
     */
    bool stop();
    
    /*
     * flush当前的播放器，也就是吧demuxer audio video decode等数据都清空掉
     */
    void flush();
    
    /*
     * 设置是否循环播放视频
     */
    bool setLoop(bool loop);

    /*
     * 获取总时长
     */
    long getDuration();

    /*
     * 获取播放器上下文信息，写在class内相当于内联函数
     */
    PlayerContext* getPlayerContext()
    {
        return pPlayerContext;
    }
    
    PPEncoder();
    
    void pp_get_msg(Message& msg);
    
    void setHandle(EventHandler *handle);
    /*
     * 获析构函数一定不能私有话，否则可能导致内存泄漏
     */
    virtual ~PPEncoder();
private:
    std::string         pUrl;
    std::string         pOutFilePath;
    EncodeParam         pEncodeParams;
    static SDL_mutex    *p_Mutex;
    static PPEncoder    *p_Encoder;
    PlayerContext       *pPlayerContext;
    EventHandler        *p_Handler;
    mediaCore           *p_MediaCore;
    VideoDecodeThread   *p_VideoDecoderThread;
    DemuxThread         *p_DemuxerThread;
    AudioDecodeThread   *p_AudioDecoderThread;
    AvEncodeThread      *p_EncoderThread;
};

NS_MEDIA_END
#endif // PPLAYER_H
