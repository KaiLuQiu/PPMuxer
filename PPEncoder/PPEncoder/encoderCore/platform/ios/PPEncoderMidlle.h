//
//  PPEncoderMidlle.h
//  PPEncoder
//
//  Created by 邱开禄 on 2020/01/05.
//  Copyright © 2020年 邱开禄. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "PPEncoderDelegate.h"

#ifndef PPEncoderMidlle_H
#define PPEncoderMidlle_H

@interface PPEncoderMidlle : NSObject
/*
 * 回调实现
 */
@property (nonatomic, assign) id <OnPreparedListener> pPreparedListener;
@property (nonatomic, assign) id <OnCompletionListener> pCompletionListener;
@property (nonatomic, assign) id <OnSeekCompletionListener> pSeekCompletionListener;
@property (nonatomic, assign) id <OnErrorListener> pErrorListener;
@property (nonatomic, assign) id <OnInfoListener> pInfoListener;

-(id) initEncoder:(const char*)URL
         FilePath:(const char*)FilePath;

-(void)prepareAsync;

-(void)start;

-(void)pause:(Boolean)isPause;

-(void)stop;

-(void)seek:(float)pos;

-(int64_t)getDuration;

-(void)dealloc;

/*
 * 设置准备完成监听器
 */
-(void)setOnPreparedListener:(id<OnPreparedListener>)listener;

/*
 * 设置播放完成监听器
 */
-(void)setOnCompletionListener:(id<OnCompletionListener>)listener;

/*
 * 设置seek完成监听器
 */
-(void)setOnSeekCompletionListener:(id<OnSeekCompletionListener>)listener;

/*
 * 设置底层error监听器
 */
-(void)setOnErrorListener:(id<OnErrorListener>)listener;

/*
 * 设置底层Info信息监听器
 */
-(void)setOnInfoListener:(id<OnInfoListener>)listener;


@end
#endif
