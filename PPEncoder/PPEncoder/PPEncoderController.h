//
//  Controller.h
//  PPEncode
//
//  Created by 邱开禄 on 2019/11/15.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface PPEncoderController : UIViewController

#define SCREENWIDTH         ([UIScreen mainScreen].bounds.size.width)
#define SCREENHEIGHT        ([UIScreen mainScreen].bounds.size.height)
#define SCREENWIDTH_D40     (SCREENWIDTH / 40)
#define SCREENHEIGHT_D40    (SCREENHEIGHT / 40)

@property (nonatomic, strong) UIButton*         pStartButton;
@property (retain,nonatomic)  UISwitch*         pPauseSwitch;
@property (nonatomic, strong) UIButton*         pStopButton;
@property (nonatomic, strong) UIButton*         pSeekButton;

@property (nonatomic)         UITextField*      pText;
@property (nonatomic)         UILabel*          pCurDuration;
@property (nonatomic)         UILabel*          pSeekText;

-(void)passViewController:(NSMutableArray *)urlArray;
@end

NS_ASSUME_NONNULL_END
