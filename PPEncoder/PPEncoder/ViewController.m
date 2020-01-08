//
//  ViewController.m
//  PPEncode
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//


#import "ViewController.h"
#import "PPEncoderController.h"
#import "WPhotoViewController.h"

@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
//    PPEncoderController *EncoderController = [self.storyboard instantiateViewControllerWithIdentifier:@"EncoderController"];
//        [self presentViewController:EncoderController animated:YES completion:nil];
    self.pOpenVideoButton = [UIButton buttonWithType:UIButtonTypeCustom];
    self.pOpenVideoButton.frame = CGRectMake((self.view.frame.size.width-160*phoneScale)/2, self.view.frame.size.height-(60+160)*phoneScale, 160*phoneScale, 160*phoneScale);
    self.pOpenVideoButton.layer.cornerRadius = 160*phoneScale/2;
    self.pOpenVideoButton.layer.masksToBounds = YES;
    [self.pOpenVideoButton setImage:[UIImage imageNamed:@"image/CreateNew"] forState:UIControlStateNormal];
    [self.pOpenVideoButton addTarget:self action:@selector(addButClick) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:self.pOpenVideoButton];
}

-(void)addButClick
{
    WPhotoViewController *WphotoVC = [[WPhotoViewController alloc] init];
    // 选择图片的最大数
    WphotoVC.selectPhotoOfMax = 1;
    __weak typeof(WphotoVC) WphotoVCs = WphotoVC;

    [WphotoVC setSelectPhotosBack:^(NSMutableArray *videoUrlArr) {
        [WphotoVCs dismissViewControllerAnimated:YES completion:^{
            PPEncoderController *EncoderController = [self.storyboard instantiateViewControllerWithIdentifier:@"EncoderController"];
            [EncoderController passViewController:videoUrlArr];
            [self presentViewController:EncoderController animated:YES completion:nil];
        }];
    }];
    [self presentViewController:WphotoVC animated:YES completion:nil];
}

@end
