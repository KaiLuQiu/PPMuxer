//
//  PPMuxerController.m
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/15.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#import <opencv2/imgproc/types_c.h>
#import <opencv2/objdetect/objdetect.hpp>
#import <opencv2/core/core.hpp>
#import <opencv2/highgui/highgui.hpp>
#import <opencv2/imgproc/imgproc.hpp>
#import "opencv2/opencv.hpp"

#import "PPMuxerController.h"
#import <VideoToolbox/VideoToolbox.h>
#import <AVFoundation/AVFoundation.h>
#import <GameController/GameController.h>
#import <CoreMotion/CoreMotion.h>
#import "OpenGLView.h"
#include "render_frame.h"

@interface PPMuxerController (){
    NSMutableArray              *_videoUrlArray;
}

@end

@implementation PPMuxerController

- (void)viewWillDisappear:(BOOL)animated{
    [super viewWillDisappear:animated];
}

- (void)viewDidLoad {
    [super viewDidLoad];

}

-(void)passViewController:(NSMutableArray *)urlArray
{
    _videoUrlArray = urlArray;
}


@end

