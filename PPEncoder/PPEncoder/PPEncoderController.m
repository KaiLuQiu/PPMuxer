//
//  PPEncoderController.m
//  PPEncode
//
//  Created by 邱开禄 on 2019/11/15.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#import "PPEncoderController.h"
#import <VideoToolbox/VideoToolbox.h>
#import <AVFoundation/AVFoundation.h>
#import <GameController/GameController.h>
#import <CoreMotion/CoreMotion.h>
#include "PPEncoderMidlle.h"
#include "PPEncoderParam.h"

@interface PPEncoderController (){
    PPEncoderMidlle             *_encoder;
    NSMutableArray              *_videoUrlArray;
}

@end

@implementation PPEncoderController

- (void)viewWillDisappear:(BOOL)animated{
    [super viewWillDisappear:animated];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor whiteColor];
    [self InitText];
    [self InitLabel];
    [self InitButton];
    
    NSString* path = [self getFileFromMainbundleAbsolutePath:@"video/hiphop.mp4"];
    const char *outFilePath = [[[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0] stringByAppendingPathComponent:@"bboy.mp4"] cStringUsingEncoding:NSUTF8StringEncoding];    
    _encoder = [[PPEncoderMidlle alloc] initEncoder:[path UTF8String] FilePath:outFilePath];
    
    // 设置监听器
    [self setPlayerStateListener];
    
    [self PrepareAsync];

}

-(void)passViewController:(NSMutableArray *)urlArray
{
    _videoUrlArray = urlArray;
}

- (void) InitLabel {
    _pCurDuration = [[UILabel alloc] initWithFrame:CGRectMake(SCREENWIDTH_D40 * 30, SCREENHEIGHT_D40 * 32, SCREENWIDTH_D40 * 10, SCREENHEIGHT_D40 * 2)];
    _pCurDuration.textAlignment = NSTextAlignmentCenter;
    [self.view addSubview:_pCurDuration];
    
    _pSeekText = [[UILabel alloc] initWithFrame:CGRectMake(SCREENWIDTH_D40 * 10, SCREENHEIGHT_D40 * 40, SCREENWIDTH_D40 * 20, SCREENHEIGHT_D40 * 1)];
    _pSeekText.textColor = [UIColor orangeColor];
    _pSeekText.textAlignment = NSTextAlignmentCenter;
    [self.view addSubview:_pSeekText];
}

#pragma mark 初始化文本控件
- (void) InitText {
    _pText = [[UITextField alloc] initWithFrame:CGRectMake(SCREENWIDTH_D40 * 20, SCREENHEIGHT_D40 * 23, SCREENWIDTH_D40 * 10, SCREENHEIGHT_D40 * 2)];
    _pText.textColor = [UIColor redColor];
    _pText.textAlignment = NSTextAlignmentCenter;
    [_pText setText:@"正常播放:"];
    [self.view addSubview:_pText];
}

#pragma mark 初始化按钮控件
- (void) InitButton {
    _pStartButton = [UIButton buttonWithType:UIButtonTypeSystem];
    _pStartButton.frame = CGRectMake(SCREENWIDTH_D40 * 30, SCREENHEIGHT_D40 * 20, SCREENWIDTH_D40 * 10, SCREENHEIGHT_D40 * 2);
    [_pStartButton setTitle:@"开始转码" forState:UIControlStateNormal];
    [_pStartButton setTitleColor:[UIColor colorWithRed:1 green:1 blue:1 alpha:1] forState:(UIControlState)UIControlStateNormal];
    _pStartButton.contentMode = UIViewContentModeCenter;
    _pStartButton.backgroundColor = [UIColor colorWithRed:0 green:1 blue:0 alpha:1];
    [_pStartButton addTarget:self action:@selector(clickStartButton:) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:_pStartButton];

    _pPauseSwitch = [[UISwitch alloc] init];
    _pPauseSwitch.frame = CGRectMake(SCREENWIDTH_D40 * 30, SCREENHEIGHT_D40 * 23, SCREENWIDTH_D40 * 10, SCREENHEIGHT_D40 * 2);
    _pPauseSwitch.on = NO;
    [_pPauseSwitch setOnTintColor:[UIColor orangeColor]];
    [_pPauseSwitch setThumbTintColor:[UIColor blueColor]];
    [_pPauseSwitch setTintColor:[UIColor greenColor]];
    [_pPauseSwitch addTarget:self action:@selector(clickPauseSwitch:) forControlEvents:UIControlEventValueChanged];
    [self.view addSubview:_pPauseSwitch];
    
    _pStopButton = [UIButton buttonWithType:UIButtonTypeSystem];
    _pStopButton.frame = CGRectMake(SCREENWIDTH_D40 * 30, SCREENHEIGHT_D40 * 26, SCREENWIDTH_D40 * 10, SCREENHEIGHT_D40 * 2);
    [_pStopButton setTitle:@"停止转码" forState:UIControlStateNormal];
    [_pStopButton setTitleColor:[UIColor colorWithRed:1 green:1 blue:1 alpha:1] forState:(UIControlState)UIControlStateNormal];
    _pStopButton.contentMode = UIViewContentModeCenter;
    _pStopButton.backgroundColor = [UIColor colorWithRed:0 green:1 blue:0 alpha:1];
    [_pStopButton addTarget:self action:@selector(clickStopButton:) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:_pStopButton];
    
    _pSeekButton = [UIButton buttonWithType:UIButtonTypeSystem];
    _pSeekButton.frame = CGRectMake(SCREENWIDTH_D40 * 25, SCREENHEIGHT_D40 * 20, SCREENWIDTH_D40 * 10, SCREENHEIGHT_D40 * 2);
    [_pSeekButton setTitle:@"seek" forState:UIControlStateNormal];
    [_pSeekButton setTitleColor:[UIColor colorWithRed:1 green:1 blue:1 alpha:1] forState:(UIControlState)UIControlStateNormal];
    _pSeekButton.contentMode = UIViewContentModeCenter;
    _pSeekButton.backgroundColor = [UIColor colorWithRed:0 green:1 blue:0 alpha:1];
    [_pSeekButton addTarget:self action:@selector(clickSeekButton:) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:_pSeekButton];
}


- (void)clickStartButton:(id)sender {
    if(self.pStartButton == nil) {
        return;
    }
    [_encoder start];
}

- (void)clickPauseSwitch:(UISwitch *)sw {
    if(self.pPauseSwitch == nil) {
        return;
    }
    if (sw.on == YES) {
        [_pText setText:@"暂停播放:"];
        [_encoder pause:true];
    } else {
        [_pText setText:@"正常播放:"];
        [_encoder pause:false];
    }
}

- (void)clickStopButton:(id)sender {
    if(self.pStopButton == nil) {
        return;
    }
}

- (void)clickSeekButton:(id)sender {
    if(self.pSeekButton == nil) {
        return;
    }
    [_encoder seek:0.5];
}

/**
 *  @fun:getFileFromMainbundleAbsolutePath 获取资源文件绝对路径，以mainbundle为基础路径进行路径拼接，
 *  @fileCompent: 资源文件存放目录的相对路径
 */
- (NSString*) getFileFromMainbundleAbsolutePath:(NSString*) fileCompent
{
    return [NSString stringWithFormat:@"%@/%@",[[NSBundle mainBundle] resourcePath], fileCompent];
}

#pragma mark 设置监听器
-(void) setPlayerStateListener {
    [_encoder setOnPreparedListener:self];
    [_encoder setOnInfoListener:self];
    [_encoder setOnErrorListener:self];
    [_encoder setOnCompletionListener:self];
    [_encoder setOnSeekCompletionListener:self];
}

#pragma mark PrepareAsync
-(void)PrepareAsync{
    [_encoder prepareAsync];
}

#pragma mark PPlayer PROTOCOL
#pragma mark - OnPreparedListener
-(void) onPrepared {
    printf("pplayer: Prepared !!!\n");
}

#pragma mark - OnCompletionListener
-(void) onCompletion {
    
}

#pragma mark - OnSeekCompletionListener
-(void) OnSeekCompletion {
    
}

#pragma mark - OnErrorListener
-(void) OnError {
    
}

#pragma mark - OnInfoListener
-(void) OnInfo {
    
}
@end

