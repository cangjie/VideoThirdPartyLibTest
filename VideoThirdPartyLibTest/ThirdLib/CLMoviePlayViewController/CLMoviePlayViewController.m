//
//  CLMovePlayViewController.m
//  player
//
//  Created by chuliangliang on 15/5/31.
//  Copyright (c) 2015年 liwu. All rights reserved.
//

#import "CLMoviePlayViewController.h"

#import "OpenGLView20.h"
#include "MediaPlayer.h"
#import "MediaCollection.h"
#include "PublishInterface.h"


typedef enum
{
    CLMovieShowStyle_OnlyTea,       /*默认 只有老师视频*/
    CLMovieShowStyle_StuAndTea,     /*老师和学生*/
    CLMovieShowStyle_MyselfAndTea,  /*老师和登录用户*/
}CLMovieShowStyle;

@interface CLMoviePlayViewController ()<OpenGLViewDelgate>
{
    
    OpenGLView20 *videoGLView;  //老师视频
    OpenGLView20 *stuVideoGLView; //学生视频
    UIView *myVideoView;    //登录用户自己的视频预览区
    MediaCollection *mediaCollection; /*音视频采集器*/
    BOOL devicePortrait;
    UIView *v_ac;
 
}
@property (strong, nonatomic) UIActivityIndicatorView *teaActivityView;
@property (assign, nonatomic) CLMovieShowStyle movieShowStyle; //视频显示的类型
@end

@implementation CLMoviePlayViewController
- (id) initWithFrame:(CGRect)frame
{
    self = [super init];
    if (self) {
        self.view.frame = frame;
        [self _initSubViews];
    }
    return self;
    
}
- (void)dealloc {
    [self clearnALL];
    videoGLView = nil;
    self.videoUrl = nil;
    self.audioUrl = nil;
    self.studentSrcUrl = nil;
    self.srcUrl = nil;
    self.loginUserVideoPush = nil;
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}
- (void)clearnALL
{
    switch (self.movieShowStyle) {
        case CLMovieShowStyle_StuAndTea:
        {
            [self stopStudentVideo];
        }
            break;
        case CLMovieShowStyle_MyselfAndTea:
        {
            [self stopLoginUserVideo];
        }
            break;
        default:
            break;
    }
    [self stop];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    
}

- (void)_initSubViews{
    devicePortrait = YES;
    self.canAutoLayout = YES;
    self.view.backgroundColor = [UIColor blackColor];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(applicationWillResignActive:)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:[UIApplication sharedApplication]];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationWillBecame:)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:[UIApplication sharedApplication]];

    
    self.teaActivityView = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhite];
    self.teaActivityView.frame = CGRectMake(0, 0, 40, 40);
    self.teaActivityView.center = self.view.center;
    self.teaActivityView.hidesWhenStopped = YES;
    [self.teaActivityView stopAnimating];
    [self.view addSubview:self.teaActivityView];

    
    videoGLView = [[OpenGLView20 alloc] initWithFrame:self.view.bounds];
    videoGLView.center = self.view.center;
    videoGLView.layer.borderWidth = 1.0f;
    videoGLView.layer.borderColor = [UIColor redColor].CGColor;
    videoGLView.playDelegate = self;
    [self.view addSubview:videoGLView];

    
    
    stuVideoGLView = [[OpenGLView20 alloc] initWithFrame:self.view.bounds];
    stuVideoGLView.hidden = YES;
    stuVideoGLView.backgroundColor = [UIColor blackColor];
    stuVideoGLView.layer.borderWidth = 1.0f;
    stuVideoGLView.layer.borderColor = [UIColor greenColor].CGColor;

    [self.view addSubview:stuVideoGLView];

    
    self.movieShowStyle = CLMovieShowStyle_OnlyTea;
    
    self.uid = [UserAccount shareInstance].uid;
}

- (void)updateContentFrame:(CGRect)frame isAnimate:(BOOL)animate canAutoLayout:(BOOL)autoLayout
{
    self.canAutoLayout = autoLayout;
    self.view.frame = frame;
    [self resetSubViews:animate];
}
- (void)viewDidLoad {
    [super viewDidLoad];
    
    
}

- (void)setMovieShowStyle:(CLMovieShowStyle)movieShowStyle {
    _movieShowStyle = movieShowStyle;
    [self resetSubViews:NO];
}

/**
 * 重新布局子视图
 **/
- (void)resetSubViews:(BOOL)animated
{
    if (animated) {

        if (self.movieShowStyle == CLMovieShowStyle_OnlyTea) {
            stuVideoGLView.hidden = YES;
            myVideoView.hidden = YES;
            [UIView animateWithDuration:0.3 animations:^{
                videoGLView.frame = self.view.bounds;
                self.teaActivityView.left = (videoGLView.bounds.size.width - self.teaActivityView.width) * 0.5;
                self.teaActivityView.top = (videoGLView.bounds.size.height - self.teaActivityView.height) * 0.5;
            }];
        }else if (self.movieShowStyle == CLMovieShowStyle_StuAndTea) {
            CGRect teaVideoFrame = CGRectMake(0, 0, (self.view.width - 5) * 0.5,self.view.height);
            CGRect stuVideoFrame = CGRectMake(teaVideoFrame.origin.x + teaVideoFrame.size.width + 5, 0, teaVideoFrame.size.width,teaVideoFrame.size.height);
            stuVideoGLView.hidden = NO;
            myVideoView.hidden = YES;
            [UIView animateWithDuration:0.3 animations:^{
                videoGLView.frame = teaVideoFrame;
                stuVideoGLView.frame = stuVideoFrame;
                
                
                self.teaActivityView.left = (videoGLView.bounds.size.width - self.teaActivityView.width) * 0.5;
                self.teaActivityView.top = (videoGLView.bounds.size.height - self.teaActivityView.height) * 0.5;
            }];
        }else if (self.movieShowStyle == CLMovieShowStyle_MyselfAndTea) {
            
            CGRect teaVideoFrame = CGRectMake(0, 0, (self.view.width - 5) * 0.5,self.view.height);
            CGRect stuVideoFrame = CGRectMake(teaVideoFrame.origin.x + teaVideoFrame.size.width + 5, 0, teaVideoFrame.size.width,teaVideoFrame.size.height);
            stuVideoGLView.hidden = YES;
            myVideoView.hidden = NO;
            [UIView animateWithDuration:0.3 animations:^{
                videoGLView.frame = teaVideoFrame;
                myVideoView.frame = stuVideoFrame;
                
                
                self.teaActivityView.left = (videoGLView.bounds.size.width - self.teaActivityView.width) * 0.5;
                self.teaActivityView.top = (videoGLView.bounds.size.height - self.teaActivityView.height) * 0.5;
            }];
            if (mediaCollection) {
                [mediaCollection.video updatevideoPreviewFrame:CGRectMake(0, 0, stuVideoFrame.size.width, stuVideoFrame.size.height)];
            }


        }
    }else {
        if (self.movieShowStyle == CLMovieShowStyle_OnlyTea) {
            stuVideoGLView.hidden = YES;
            myVideoView.hidden = YES;
            videoGLView.frame = self.view.bounds;
            self.teaActivityView.left = (videoGLView.bounds.size.width - self.teaActivityView.width) * 0.5;
            self.teaActivityView.top = (videoGLView.bounds.size.height - self.teaActivityView.height) * 0.5;

        }else if (self.movieShowStyle == CLMovieShowStyle_StuAndTea) {
            CGRect stuVideoFrame = CGRectMake(0, 0, (self.view.width - 5) * 0.5,self.view.height);
            CGRect teaVideoFrame = CGRectMake(stuVideoFrame.origin.x + 5 + stuVideoFrame.size.width, 0, stuVideoFrame.size.width,stuVideoFrame.size.height);
            stuVideoGLView.hidden = NO;
            myVideoView.hidden = YES;
            
            videoGLView.frame = teaVideoFrame;
            stuVideoGLView.frame = stuVideoFrame;
            
            self.teaActivityView.left = (videoGLView.bounds.size.width - self.teaActivityView.width) * 0.5;
            self.teaActivityView.top = (videoGLView.bounds.size.height - self.teaActivityView.height) * 0.5;
        }if (self.movieShowStyle == CLMovieShowStyle_MyselfAndTea) {
            
            CGRect teaVideoFrame = CGRectMake(0, 0, (self.view.width - 5) * 0.5,self.view.height);
            CGRect stuVideoFrame = CGRectMake(teaVideoFrame.origin.x + teaVideoFrame.size.width + 5, 0, teaVideoFrame.size.width,teaVideoFrame.size.height);
            stuVideoGLView.hidden = YES;
            myVideoView.hidden = NO;
            
            videoGLView.frame = teaVideoFrame;
            myVideoView.frame = stuVideoFrame;
            
            
            self.teaActivityView.left = (videoGLView.bounds.size.width - self.teaActivityView.width) * 0.5;
            self.teaActivityView.top = (videoGLView.bounds.size.height - self.teaActivityView.height) * 0.5;
            if (mediaCollection) {
                [mediaCollection.video updatevideoPreviewFrame:myVideoView.bounds];
            }
            
        }

    }
}

#pragma mark - 
#pragma mark -  OpenGLViewDelgate
- (void)openglViewMoviePlaying {
    
    if ([self.teaActivityView isAnimating]) {
        ClassRoomLog(@"视频开始播放 ==> 隐藏 菊花");
        [self.teaActivityView stopAnimating];
    }
}


- (void)viewWillLayoutSubviews {
    
    if (!self.canAutoLayout) {
        ClassRoomLog(@"视频播放器 ===> 自动布局取消");
        return;
    }
    UIInterfaceOrientation interfaceOrientation=[[UIApplication sharedApplication] statusBarOrientation];
    if (interfaceOrientation == UIInterfaceOrientationPortrait || interfaceOrientation == UIInterfaceOrientationPortraitUpsideDown) {
        //翻转为竖屏时
        if (!devicePortrait) {
            devicePortrait = YES;
            [self resetSubViews:NO];
        }
    }else if (interfaceOrientation == UIInterfaceOrientationLandscapeLeft || interfaceOrientation == UIInterfaceOrientationLandscapeRight) {
        if (devicePortrait) {
            //翻转为横屏
            devicePortrait = NO;
            [self resetSubViews:NO];
        }
    }
    
}

/**
 * 程序挂起
 **/
- (void) applicationWillResignActive: (NSNotification *)notification
{
    
    CSLog(@"MovePlayer ==>> 进入后台");
    videoGLView.offScreen = YES;
    [videoGLView clearFrame];
}

/**
 * 程序进入前台
 **/
- (void) applicationWillBecame: (NSNotification *)notification {
    CSLog(@"MovePlayer ==>> 回到前台");
    videoGLView.offScreen = NO;
}


- (void)changeVideo:(int)videoId
{
    if (videoId == self.currentVideoID) {
        CSLog(@"MovePlayer ==>> 教师视频id 相同返回");
        return;
    }
    self.currentVideoID = videoId;
    

    if (self.srcUrl.length <= 0) {
        return;
    }
    
    struct PlayAddress pa[4];
    int nPaNum = 0;
    int nPlayType = VIDEOTYPE|AUDIOTYPE;
    
    AVP_parsePalyAddrURL([self.srcUrl UTF8String],pa,nPaNum);
    pa[0].nUserID = (long)self.uid;
    pa[0].bIsStudent =false;
    pa[1].bIsStudent =false;
    pa[2].bIsStudent =false;
    pa[3].bIsStudent =false;
    switch (self.currentVideoID) {
        case 1:
        {
            pa[1].bIsMainVideo = true;
            pa[1].bIsVideShow = true;
            pa[2].bIsMainVideo = false;
            pa[2].bIsVideShow = false;
            pa[3].bIsVideShow = false;
            pa[3].bIsMainVideo = false;
            pa[1].hwnd = (__bridge void*)videoGLView;
        }
            break;
        case 2:
        {
            pa[1].bIsMainVideo = false;
            pa[1].bIsVideShow = false;
            pa[2].bIsMainVideo = true;
            pa[2].bIsVideShow = true;
            pa[3].bIsVideShow = false;
            pa[3].bIsMainVideo = false;
            pa[2].hwnd = (__bridge void*)videoGLView;
        }
            break;
        case 3:
        {
            pa[1].bIsMainVideo = false;
            pa[1].bIsVideShow = false;
            pa[2].bIsMainVideo = false;
            pa[2].bIsVideShow = false;
            pa[3].bIsVideShow = true;
            pa[3].bIsMainVideo = true;
            pa[3].hwnd = (__bridge void*)videoGLView;
            
        }
            break;
        default:
            break;
    }

    AVP_Change([self.srcUrl UTF8String], nPlayType, pa, nPaNum, NULL);
}

- (void)play
{
    
    if (self.srcUrl.length <= 0 ) {
        CSLog(@"视频切换返回");
        return;
    }
    [self.teaActivityView startAnimating];
    
    _state = CLMovieState_Playing;
    CSLog(@"MovePlayer ==> 开始播放 音视频地址:%@",self.srcUrl);
    struct PlayAddress pa[4];
    int nPaNum = 0;
    int nPlayType = VIDEOTYPE|AUDIOTYPE;
    
    AVP_parsePalyAddrURL([self.srcUrl UTF8String],pa,nPaNum);
    pa[0].nUserID = (long)self.uid;
    pa[0].bIsStudent =false;
    pa[1].bIsStudent =false;
    pa[2].bIsStudent =false;
    pa[3].bIsStudent =false;
    switch (self.currentVideoID) {
        case 1:
        {
            pa[1].bIsMainVideo = true;
            pa[1].bIsVideShow = true;
            pa[2].bIsMainVideo = false;
            pa[2].bIsVideShow = false;
            pa[3].bIsVideShow = false;
            pa[3].bIsMainVideo = false;
            pa[1].hwnd = (__bridge void*)videoGLView;
//            pa[2].hwnd = (__bridge void*)videoGLView1;
//            pa[3].hwnd = (__bridge void*)videoGLView2;
        }
            break;
        case 2:
        {
            pa[1].bIsMainVideo = false;
            pa[1].bIsVideShow = false;
            pa[2].bIsMainVideo = true;
            pa[2].bIsVideShow = true;
            pa[3].bIsVideShow = false;
            pa[3].bIsMainVideo = false;
            pa[2].hwnd = (__bridge void*)videoGLView;

        }
            break;
        case 3:
        {
            pa[1].bIsMainVideo = false;
            pa[1].bIsVideShow = false;
            pa[2].bIsMainVideo = false;
            pa[2].bIsVideShow = false;
            pa[3].bIsVideShow = true;
            pa[3].bIsMainVideo = true;
            pa[3].hwnd = (__bridge void*)videoGLView;

        }
            break;
        default:
            break;
    }
    AVP_Play([self.srcUrl UTF8String],nPlayType,pa,nPaNum,NULL);
    videoGLView.offScreen = NO;
}


- (void)stop
{
    if (self.srcUrl.length <= 0) {
        return;
    }
    _state = CLMovieState_Stop;
    struct PlayAddress pa[4];
    int nPaNum = 0;
    
    pa[0].nUserID = (long)self.uid;
    pa[0].bIsStudent =false;
    pa[1].bIsStudent =false;
    pa[2].bIsStudent =false;
    pa[3].bIsStudent =false;
    switch (self.currentVideoID) {
        case 1:
        {
            pa[1].bIsMainVideo = true;
            pa[1].bIsVideShow = true;
            pa[2].bIsMainVideo = false;
            pa[2].bIsVideShow = false;
            pa[3].bIsVideShow = false;
            pa[3].bIsMainVideo = false;
            pa[1].hwnd = (__bridge void*)videoGLView;
        }
            break;
        case 2:
        {
            pa[1].bIsMainVideo = false;
            pa[1].bIsVideShow = false;
            pa[2].bIsMainVideo = true;
            pa[2].bIsVideShow = true;
            pa[3].bIsVideShow = false;
            pa[3].bIsMainVideo = false;
            pa[2].hwnd = (__bridge void*)videoGLView;
            
        }
            break;
        case 3:
        {
            pa[1].bIsMainVideo = false;
            pa[1].bIsVideShow = false;
            pa[2].bIsMainVideo = false;
            pa[2].bIsVideShow = false;
            pa[3].bIsVideShow = true;
            pa[3].bIsMainVideo = true;
            pa[3].hwnd = (__bridge void*)videoGLView;
            
        }
            break;
        default:
            break;
    }
    
    pa[1].hwnd = NULL;
    pa[2].hwnd = NULL;
    pa[3].hwnd = NULL;
    
    AVP_Stop([self.srcUrl UTF8String], pa, nPaNum);
    
    videoGLView.offScreen = YES;
    [videoGLView clearFrame];
}



//学生视频
- (void)playStudentVideo:(NSString *)stuScrUrl
{
    self.studentSrcUrl = stuScrUrl;
    if (self.studentSrcUrl.length <= 0 ) {
        CSLog(@"视频切换返回");
        return;
    }
    
    self.movieShowStyle = CLMovieShowStyle_StuAndTea;
    
    CSLog(@"MovePlayer ==> 开始播放学生 音视频地址:%@",self.studentSrcUrl);
    struct PlayAddress pa[2];
    int nPaNum = 0;
    int nPlayType = VIDEOTYPE|AUDIOTYPE;
    
    
    AVP_parsePalyAddrURL([self.studentSrcUrl UTF8String],pa,nPaNum);
    pa[0].nUserID = (long)self.uid;
    pa[0].bIsStudent =true;
    pa[1].bIsStudent =true;
    pa[1].bIsMainVideo = true;
    pa[1].bIsVideShow = true;
    
    stuVideoGLView.hidden = NO;
    pa[1].hwnd = (__bridge void*)stuVideoGLView;

    AVP_Play([self.studentSrcUrl UTF8String],nPlayType,pa,nPaNum,NULL);
    stuVideoGLView.offScreen = NO;
    
}
- (void)stopStudentVideo
{
    if (self.studentSrcUrl.length <= 0) {
        return;
    }
    
    self.movieShowStyle = CLMovieShowStyle_OnlyTea;
    struct PlayAddress pa[2];
    int nPaNum = 0;
    
    pa[0].nUserID = (long)self.uid;
    pa[0].bIsStudent =true;
    pa[1].bIsStudent =true;
    pa[1].bIsMainVideo = true;
    pa[1].bIsVideShow = true;

    
    pa[1].hwnd = NULL;
    
    AVP_Stop([self.studentSrcUrl UTF8String], pa, nPaNum);
    
    stuVideoGLView.offScreen = YES;
    [stuVideoGLView clearFrame];
    stuVideoGLView.hidden = YES;
    
    self.studentSrcUrl = nil;
}

- (void)startLoginUserVideo:(NSString *)pushVideoUrl
{
    if ([Utils objectIsNull:pushVideoUrl]) {
        return;
    }
    self.movieShowStyle = CLMovieShowStyle_MyselfAndTea;
    self.loginUserVideoPush = pushVideoUrl;
    
    //初始化视频预览页
    if (!myVideoView) {
        myVideoView = [[UIView alloc] initWithFrame:self.view.bounds];
        myVideoView.backgroundColor = [UIColor blackColor];
        myVideoView.hidden = YES;
        [self.view addSubview:myVideoView];
    }
    [self resetSubViews:NO];
    InitLibEnv((__bridge HWND)(myVideoView));
    mediaCollection = (__bridge MediaCollection *)getMediaMediaCollection();
    mediaCollection.video.hasCameraPullAwayAndNearly = NO;
    mediaCollection.video.hasFocusCursorWithTap = NO;
    
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(startPushMedia) object:nil];
    [self performSelector:@selector(startPushMedia) withObject:nil afterDelay:1.0f];
}

//延时1s 开始传输媒体信息
- (void)startPushMedia
{

    const char *szURl = [self.loginUserVideoPush UTF8String];
    int nStreamType = SOURCECAMERA;
    MediaLevel m = HIGHLEVEL;
    if ([self.loginUserVideoPush rangeOfString:@"|@|"].location!= NSNotFound) {
        nStreamType = SOURCECAMERA|SOURCEDEVAUDIO;
    }else {
        nStreamType = SOURCECAMERA;
    }
    struct PublishParam param;
    param.VU[0].nSelectCameraID = 0;
    param.VU[0].nType = SOURCECAMERA;
    param.nVUNum = 1;
    param.ml = m;
    param.mr = LISTENERROLE;
    rtmpPushStreamToServerBegin(szURl,nStreamType,param);

}

- (void)stopLoginUserVideo
{
    self.movieShowStyle = CLMovieShowStyle_OnlyTea;
    [self resetSubViews:NO];
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(startPushMedia) object:nil];
    if (self.loginUserVideoPush) {
        const char *szURl = [self.loginUserVideoPush UTF8String];
        rtmpPushStreamToServerEnd(szURl);
    }
    UninitLibEnv();
    [mediaCollection stopMediaCollection];
    myVideoView = nil;
}
@end
