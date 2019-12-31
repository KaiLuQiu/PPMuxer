//
//  ViewController.m
//  video2str
//
//  Created by 邱开禄 on 2019/12/31.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#import <opencv2/imgproc/types_c.h>
#import <opencv2/objdetect/objdetect.hpp>
#import <opencv2/core/core.hpp>
#import <opencv2/highgui/highgui.hpp>
#import <opencv2/imgproc/imgproc.hpp>
#import "opencv2/opencv.hpp"
#import "ViewController.h"

using namespace std;
using namespace cv;

@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    CGRect rect = [UIScreen mainScreen].bounds;

    UIImage *image = [UIImage imageNamed:@"6_03"];
    cv::Mat mat = cv::Mat();

}


@end
