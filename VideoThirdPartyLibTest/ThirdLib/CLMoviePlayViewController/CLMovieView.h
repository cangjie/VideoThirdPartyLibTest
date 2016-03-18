//
//  CLMovieView.h
//  Class8Online
//
//  Created by chuliangliang on 15/9/25.
//  Copyright (c) 2015年 chuliangliang. All rights reserved.
//

#import <UIKit/UIKit.h>

typedef enum
{
    CLMovieCurrentStyle_OnlyTea = 10,                       /*只有老师音视频*/
    CLMovieCurrentStyle_TeaAndStu,                          /*只有老师与被提问的学生音视频*/
    CLMovieCurrentStyle_TeaAndLoginUser,                    /*只有老师与登录用户音视频*/
}CLMovieCurrentStyle;

@interface CLMovieView : UIView
@property (strong, nonatomic) NSString *askStudentVideoUrl, /*被提问的学生(非登录用户)的音视频拉取地址*/
*teaVideosUrl,                                              /*上课讲师音视频拉取地址*/
*askLoginUserVideoPushUrl;                                  /*登录用户音视频推流地址*/
@property (assign, nonatomic) int didChangeTeaVideosId;     /*当前切换到的教师视频id*/
@property (assign, nonatomic) long long uid;
@property (assign, nonatomic) CLMovieCurrentStyle clMovieCurrentStyle;

- (id)initWithFrame:(CGRect)frame
          aMediaUrl:(NSString *)mUrl
         teaVideoId:(int)vid
        askMediaUrl:(NSString *)askUrl
     atCurrentStyle:(CLMovieCurrentStyle)aStyle;


//===============================================================
//教师音视频相关操作
//===============================================================
- (void)startPlayTeaVido:(int)videoID;                      /*开始播放教师视频*/
- (void)changeTeaVideo:(int)videoID;                        /*切换老师视频*/
- (void)stopTeaVideo;                                       /*停止老师视频*/

//===============================================================
//提问学生(非登录用户)音视频相关操作
//===============================================================
- (void)startPlayAskStu:(NSString *)stuVideoUrl;            /*开始播放被提问的学生(非登录用户)音视频*/
- (void)stopAskStuVideo;                                    /*停止播放被提问学生音视频*/


//===============================================================
//登录用户音视频相关操作
//===============================================================
- (void)startLoginUserVideo:(NSString *)pushVideoUrl;       /*开始上传登录用户的音视频*/
- (void)stopLoginUserVideo;                                 /*停止上传登录用户的音视频*/


//===============================================================
//公用操作
//===============================================================
- (void)clearnALL;                                          /**清除所有**/

@end
