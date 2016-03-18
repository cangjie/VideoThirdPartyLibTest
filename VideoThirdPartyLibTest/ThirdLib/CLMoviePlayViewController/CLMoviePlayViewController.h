//
//  CLMovePlayViewController.h
//  player
//
//  Created by chuliangliang on 15/5/31.
//  Copyright (c) 2015年 liwu. All rights reserved.
//

#import <UIKit/UIKit.h>

typedef enum{
    CLMovieState_Nomal,
    CLMovieState_Loading,
    CLMovieState_Playing,
    CLMovieState_Stop,
    CLMovieState_PlayFinish,
}CLMovieState; //视频播放器状态


@interface CLMoviePlayViewController : UIViewController
- (id) initWithFrame:(CGRect)fram;
@property (retain, nonatomic) NSString *videoUrl, *audioUrl;
@property (retain, nonatomic) NSString *srcUrl, //老师音视频地址
*studentSrcUrl,//学生音视频地址
*loginUserVideoPush; //登录用户音视频推流地址
@property (assign, nonatomic) int currentVideoID;

@property (assign, nonatomic) BOOL hasChangeUrl;
@property (readonly,assign, nonatomic) CLMovieState state;
@property (assign, nonatomic) long long uid;
@property (assign, nonatomic) BOOL canAutoLayout;
- (void)updateContentFrame:(CGRect)frame isAnimate:(BOOL)animate canAutoLayout:(BOOL)autoLayout;

- (void)changeVideo:(int)videoId;
- (void)play;
- (void)stop;
- (void)clearnALL;

//学生视频
- (void)playStudentVideo:(NSString *)stuScrUrl;
- (void)stopStudentVideo;

//上传自己音视频
- (void)startLoginUserVideo:(NSString *)pushVideoUrl;
- (void)stopLoginUserVideo;
@end
