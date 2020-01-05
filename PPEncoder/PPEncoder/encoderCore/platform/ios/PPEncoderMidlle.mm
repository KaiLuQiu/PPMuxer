//
//  PPEncoderMidlle.mm
//  PPEncoder
//
//  Created by 邱开禄 on 2020/01/05.
//  Copyright © 2020年 邱开禄. All rights reserved.
//

#ifdef __cplusplus
    #import <opencv2/imgproc/types_c.h>
    #import <opencv2/objdetect/objdetect.hpp>
    #import <opencv2/core/core.hpp>
    #import <opencv2/highgui/highgui.hpp>
    #import <opencv2/imgproc/imgproc.hpp>
    #import "opencv2/opencv.hpp"
#endif
#import "PPEncoderMidlle.h"
#include "PPEncoder.h"
#include "PPEncoder_C_Interface.h"

@interface PPEncoderMidlle ()
    @property media::EventHandler *pHandler;
    @property media::msg_loop pMsgLoopCallback;
    -(int) media_player_msg_loop:(media::Message &)msg;
@end

@implementation PPEncoderMidlle

-(id) initEncoder:(const char*)URL
         FilePath:(const char*)FilePath
{
    if(!(self = [super init])) {
        return nil;
    }
    media::PPEncoder::getInstance()->setDataSource(URL);
    media::PPEncoder::getInstance()->setOutFilePath(FilePath);
    
    EncodeParam params;
    media::PPEncoder::getInstance()->setEncodeParam(params);

    self.pMsgLoopCallback = msg_loop;
    
    self.pHandler = new (std::nothrow)media::EventHandler();
    if (NULL == self.pHandler) {
        printf("new handler fail!!! \n");
    }
    self.pHandler->setMediaPlayer((__bridge void*)self, self.pMsgLoopCallback);
    media::PPEncoder::getInstance()->setHandle(self.pHandler);
    return self;
}

-(void) prepareAsync {
    media::PPEncoder::getInstance()->prepareAsync();
}

-(void) start {
    PlayerState curState = media::PPEncoder::getInstance()->getPlayerContext()->playerState;
    // prepared状态后，才能正式播放
    if(curState == PLAYER_MEDIA_PREPARED) {
        media::PPEncoder::getInstance()->start();
    }
}

-(void) pause:(Boolean)isPause {
    PlayerState curState = media::PPEncoder::getInstance()->getPlayerContext()->playerState;
    if (curState == PLAYER_MEDIA_START ||
        curState == PLAYER_MEDIA_SEEK ||
        curState == PLAYER_MEDIA_RESUME ||
        curState == PLAYER_MEDIA_FLUSH ||
        curState == PLAYER_MEDIA_PAUSE) {
        media::PPEncoder::getInstance()->pause(isPause);
    }
}

-(void) stop {
    
}

-(void) seek:(float)pos {
    media::PPEncoder::getInstance()->seek(pos);
}

-(int64_t)getDuration {
   return media::PPEncoder::getInstance()->getDuration() / 1000;
}

-(void)dealloc {
    SAFE_DELETE(self.pHandler);
}


void msg_loop(void* playerInstance, media::Message & msg)
{
    // 通过将self指针桥接为oc 对象来调用oc方法
    [(__bridge id)playerInstance media_player_msg_loop:msg];
}

-(int) media_player_msg_loop:(media::Message &)msg
{
    media::PPEncoder::getInstance()->pp_get_msg(msg);
    switch(msg.m_what)
    {
        case PLAYER_MEDIA_NOP:
            break;

        case PLAYER_MEDIA_SEEK:
            break;

        case PLAYER_MEDIA_PREPARED:
            if (self.pPreparedListener)
                [self.pPreparedListener onPrepared];
            break;

        case PLAYER_MEDIA_SEEK_COMPLETE:
            if (self.pSeekCompletionListener)
                [self.pSeekCompletionListener onCompletion];
            break;

        case PLAYER_MEDIA_SEEK_FAIL:
            break;

        case PLAYER_MEDIA_PLAYBACK_COMPLETE:
            if (self.pCompletionListener)
                [self.pCompletionListener onCompletion];
            break;

        case PLAYER_MEDIA_SET_VIDEO_SIZE:

            break;
        case PLAYER_MEDIA_ERROR:
            if (self.pErrorListener)
                [self.pErrorListener onCompletion];
            break;

        case PLAYER_MEDIA_INFO:
            if (self.pInfoListener)
                [self.pInfoListener onCompletion];
            break;

        case PLAYER_MEDIA_PAUSE:

            break;
        case PLAYER_MEDIA_START:

            break;
        default:
            break;
    }
    return 0;
}

-(void)setOnPreparedListener:(id<OnPreparedListener>)listener {
    self.pPreparedListener = listener;
}

-(void)setOnCompletionListener:(id<OnCompletionListener>)listener {
    self.pCompletionListener = listener;
}

-(void)setOnSeekCompletionListener:(id<OnSeekCompletionListener>)listener {
    self.pSeekCompletionListener = listener;
}

-(void)setOnErrorListener:(id<OnErrorListener>)listener {
    self.pErrorListener = listener;
}

-(void)setOnInfoListener:(id<OnInfoListener>)listener {
    self.pInfoListener = listener;
}

@end
