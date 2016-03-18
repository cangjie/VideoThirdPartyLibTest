//
//  CLMovieView.m
//  Class8Online
//
//  Created by chuliangliang on 15/9/25.
//  Copyright (c) 2015年 chuliangliang. All rights reserved.
//

#import "CLMovieView.h"
#import "OpenGLView20.h"
#include "MediaPlayer.h"
#import "MediaCollection.h"
#include "PublishInterface.h"

const int teaVideoGlviewTAG = 2001;
const int askStuVideoGlviewTAG = 2002;

@interface CLMovieView () <OpenGLViewDelgate>

@property (strong, nonatomic) OpenGLView20 *teaVideoGlView,                     /*教师视频展示View*/
*askStuVideoGlView;                                                             /*被提问的学生视频展示View*/
@property (strong, nonatomic) UIView *loginUserVideoView;                       /*登录用户自己的视频展示View*/

@property (strong, nonatomic) UIActivityIndicatorView *teaActivityView,         /*教师视频加载菊花*/
*askStuActivityView;                                                            /*被提问学生视频加载菊花*/

@property (strong, nonatomic)MediaCollection *mediaCollection;                  /*音视频采集器*/

@end

@implementation CLMovieView

- (void)dealloc {
    [self clearnALL];
    //视频view
    if (_teaVideoGlView) {
        _teaVideoGlView = nil;
    }
    if (_askStuVideoGlView) {
        _askStuVideoGlView = nil;
    }
    
    if (_loginUserVideoView) {
        _loginUserVideoView = nil;
    }
    
    //菊花
    if (_teaActivityView) {
        _teaActivityView = nil;
    }
    if (_askStuActivityView) {
        _askStuActivityView = nil;
    }
    
    //NSString
    self.teaVideosUrl = nil;
    self.askStudentVideoUrl = nil;
    self.askLoginUserVideoPushUrl = nil;
    
    //Remove Notifications
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (id)initWithFrame:(CGRect)frame
          aMediaUrl:(NSString *)mUrl
         teaVideoId:(int)vid
        askMediaUrl:(NSString *)askUrl
     atCurrentStyle:(CLMovieCurrentStyle)aStyle
{
    self = [super initWithFrame:frame];
    if (self) {
        _clMovieCurrentStyle = aStyle;
        self.teaVideosUrl = mUrl;
        self.didChangeTeaVideosId = vid;
        
        
        switch (aStyle) {
            case CLMovieCurrentStyle_OnlyTea:
            {
                //只有老师视频
            }
                break;
            case CLMovieCurrentStyle_TeaAndStu:
            {
                //只有老师视频和提问学生(非登录用户)
                self.askStudentVideoUrl = askUrl;
            }
                break;
            case CLMovieCurrentStyle_TeaAndLoginUser:
            {
                //只有老师视频和登陆用户上传视频
                self.askLoginUserVideoPushUrl = askUrl;
            }
                break;
            default:
                break;
        }
        self.uid = [UserAccount shareInstance].uid;
        [self addNotifications];
    }
    return self;
}

//=====================================
//TODO: 注册通知
//=====================================
- (void)addNotifications {
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(applicationWillResignActive:)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:[UIApplication sharedApplication]];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationWillBecame:)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:[UIApplication sharedApplication]];
}


/**
 * 程序挂起
 **/
- (void) applicationWillResignActive: (NSNotification *)notification
{    
    CSLog(@"CLMovieView ==>> 进入后台");
    self.teaVideoGlView.offScreen = YES;
    [self.teaVideoGlView clearFrame];
    
    self.askStuVideoGlView.offScreen = YES;
    [self.askStuVideoGlView clearFrame];
}

/**
 * 程序进入前台
 **/
- (void) applicationWillBecame: (NSNotification *)notification {
    CSLog(@"CLMovieView ==>> 回到前台");
    self.teaVideoGlView.offScreen = NO;
    self.askStuVideoGlView.offScreen = NO;
}


//======================================
//TODO: 初始化视频展示View
//======================================
/**
 *教师视频View
 **/
- (OpenGLView20 *)teaVideoGlView {
    if (!_teaVideoGlView) {
        _teaVideoGlView = [[OpenGLView20 alloc] initWithFrame:self.bounds];
        _teaVideoGlView.playDelegate = self;
        _teaVideoGlView.tag = teaVideoGlviewTAG;
        [self addSubview:_teaVideoGlView];
    }
    return _teaVideoGlView;
}

/**
 *被提问的同学视频view
 **/
- (OpenGLView20 *)askStuVideoGlView {
    if (!_askStuVideoGlView) {
        _askStuVideoGlView = [[OpenGLView20 alloc] initWithFrame:self.bounds];
        _askStuVideoGlView.playDelegate = self;
        _askStuVideoGlView.tag = askStuVideoGlviewTAG;
        [self addSubview:_askStuVideoGlView];
    }
    return _askStuVideoGlView;
}
/**
 *登录用户自己的视频view
 **/
- (UIView *)loginUserVideoView {
    if (!_loginUserVideoView) {
        _loginUserVideoView = [[UIView alloc] initWithFrame:self.bounds];
        _loginUserVideoView.backgroundColor = [UIColor blackColor];
        [self addSubview:_loginUserVideoView];
    }
    return _loginUserVideoView;
}

#pragma mark - 
#pragma mark - OpenGLViewDelgate
- (void)videoPlaying:(OpenGLView20 *)glView
{
    if (glView.tag == teaVideoGlviewTAG && [self.teaActivityView isAnimating]) {
        [self.teaActivityView stopAnimating];
        self.teaVideoGlView.isRefreshVideoSize = NO;
    }else if (glView.tag == askStuVideoGlviewTAG && [self.askStuActivityView isAnimating]) {
        [self.askStuActivityView stopAnimating];
        self.askStuVideoGlView.isRefreshVideoSize = NO;
    }
}


//======================================
//TODO: 初始化视频加载菊花
//======================================
/**
 *初始化教师视频加载菊花
 **/
- (UIActivityIndicatorView *) teaActivityView {
    if (!_teaActivityView) {
        _teaActivityView= [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhite];
        _teaActivityView.frame = CGRectMake(0, 0, 40, 40);
        _teaActivityView.hidesWhenStopped = YES;
        [_teaActivityView stopAnimating];
        [self insertSubview:_teaActivityView atIndex:0];

    }
    return _teaActivityView;
}
/**
 *初始化被提问学生(非登录用户)视频加载菊花
 **/
- (UIActivityIndicatorView *) askStuActivityView {
    if (!_askStuActivityView) {
        _askStuActivityView = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhite];
        _askStuActivityView.frame = CGRectMake(0, 0, 40, 40);
        _askStuActivityView.hidesWhenStopped = YES;
        [_askStuActivityView stopAnimating];
        [self insertSubview:_askStuActivityView atIndex:0];
        
    }
    return _askStuActivityView;
}

/**
 * 重新布局所有子视图 之 只有教师视频
 **/
- (void)resetAllSubViewWithOnlyTeaVideo
{
    self.askStuVideoGlView.hidden = YES;
    self.loginUserVideoView.hidden = YES;
    self.teaVideoGlView.hidden = NO;
    
    self.teaVideoGlView.frame = self.bounds;
    self.teaActivityView.left = (self.teaVideoGlView.width - self.teaActivityView.width) * 0.5;
    self.teaActivityView.top = (self.teaVideoGlView.height - self.teaActivityView.height) * 0.5;
}


/**
 * 重新布局所有子视图 之 只有教师视频 + 被提问学生视频
 **/
- (void)resetAllSubViewWithTeaVideoAndAskStuVideo
{
    self.askStuVideoGlView.hidden = NO;
    self.loginUserVideoView.hidden = YES;
    self.teaVideoGlView.hidden = NO;
    
    CGRect tRect = CGRectMake(0, 0, (self.width - 5) * 0.5,self.height);
    CGRect sRect = CGRectMake(tRect.origin.x + tRect.size.width + 5, 0, tRect.size.width,tRect.size.height);
    
    self.teaVideoGlView.frame = tRect;
    self.teaActivityView.left = (tRect.size.width - self.teaActivityView.width) * 0.5;
    self.teaActivityView.top = (tRect.size.height - self.teaActivityView.height) * 0.5;

    self.askStuVideoGlView.frame = sRect;
    self.askStuActivityView.left = sRect.origin.x + (sRect.size.width - self.askStuActivityView.width) * 0.5;
    self.askStuActivityView.top = (sRect.size.height - self.askStuActivityView.height) * 0.5;
}

/**
 * 重新布局所有子视图 之 只有教师视频 + 登录用户
 **/
- (void)resetAllSubViewWithTeaVideoAndLoginUser
{
    self.askStuVideoGlView.hidden = YES;
    self.loginUserVideoView.hidden = NO;
    self.teaVideoGlView.hidden = NO;
    
    CGRect tRect = CGRectMake(0, 0, (self.width - 5) * 0.5,self.height);
    CGRect sRect = CGRectMake(tRect.origin.x + tRect.size.width + 5, 0, tRect.size.width,tRect.size.height);
    
    self.teaVideoGlView.frame = tRect;
    self.teaActivityView.left = (tRect.size.width - self.teaActivityView.width) * 0.5;
    self.teaActivityView.top = (tRect.size.height - self.teaActivityView.height) * 0.5;

    self.loginUserVideoView.frame = sRect;
    [self.mediaCollection.video updatevideoPreviewFrame:CGRectMake(0, 0, self.loginUserVideoView.width, self.loginUserVideoView.height)];
}

/**
 *重新布局所有子视图
 **/
- (void)resetAllSubViewsFrame
{
    switch (self.clMovieCurrentStyle) {
        case CLMovieCurrentStyle_OnlyTea:
        {
            //只有老师视频
            [self resetAllSubViewWithOnlyTeaVideo];
        }
            break;
        case CLMovieCurrentStyle_TeaAndStu:
        {
            //只有老师视频和提问学生(非登录用户)
            [self resetAllSubViewWithTeaVideoAndAskStuVideo];
        }
            break;
        case CLMovieCurrentStyle_TeaAndLoginUser:
        {
            //只有老师视频和登陆用户上传视频
            [self resetAllSubViewWithTeaVideoAndLoginUser];
        }
            break;
        default:
            break;
    }
}

- (void)setFrame:(CGRect)frame {
    [super setFrame:frame];
    [self resetAllSubViewsFrame];
}

- (void)setClMovieCurrentStyle:(CLMovieCurrentStyle)clMovieCurrentStyle
{
    _clMovieCurrentStyle = clMovieCurrentStyle;
    [self resetAllSubViewsFrame];
}

//===============================================================
//TODO: 教师音视频相关操作
//===============================================================
/**
 *开始播放教师视频
 **/
- (void)startPlayTeaVido:(int)videoID
{
    if (self.teaVideosUrl.length <= 0 ) {
        CSLog(@"CLMovieView ===>> ERROR: 教师视频地址为空");
        return;
    }
    self.teaVideoGlView.isRefreshVideoSize = YES;
    self.clMovieCurrentStyle = CLMovieCurrentStyle_OnlyTea;
    
    [self.teaActivityView startAnimating];
    
    self.didChangeTeaVideosId = videoID;
    CSLog(@"CLMovieView ===>> 开始播放 音视频地址:%@",self.teaVideosUrl);
    struct PlayAddress pa[4];
    int nPaNum = 0;
    int nPlayType = VIDEOTYPE|AUDIOTYPE;
    
    AVP_parsePalyAddrURL([self.teaVideosUrl UTF8String],pa,nPaNum);
    pa[0].nUserID = (long)self.uid;
    pa[0].bIsStudent =false;
    pa[1].bIsStudent =false;
    pa[2].bIsStudent =false;
    pa[3].bIsStudent =false;
    switch (self.didChangeTeaVideosId) {
        case 1:
        {
            pa[1].bIsMainVideo = true;
            pa[1].bIsVideShow = true;
            pa[2].bIsMainVideo = false;
            pa[2].bIsVideShow = false;
            pa[3].bIsVideShow = false;
            pa[3].bIsMainVideo = false;
            pa[1].hwnd = (__bridge void*)self.teaVideoGlView;
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
            pa[2].hwnd = (__bridge void*)self.teaVideoGlView;
            
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
            pa[3].hwnd = (__bridge void*)self.teaVideoGlView;
            
        }
            break;
        default:
            break;
    }
    AVP_Play([self.teaVideosUrl UTF8String],nPlayType,pa,nPaNum,NULL);
    self.teaVideoGlView.offScreen = NO;
}

/**
 *切换老师视频
 **/
- (void)changeTeaVideo:(int)videoID
{
    if (videoID == self.didChangeTeaVideosId) {
        CSLog(@"CLMovieView ===>> 切换的教师视频id与正在播放是视频id *** 相同*** 不做处理");
        return;
    }
    self.didChangeTeaVideosId = videoID;
    
    
    if (self.teaVideosUrl.length <= 0) {
        CSLog(@"CLMovieView ===>> ERROR: 切换教师视频时教师视频URL为空");
        return;
    }
    CSLog(@"CLMovieView ===>> 切换教师视频时教师视频 ID: %d",self.didChangeTeaVideosId);
    
    struct PlayAddress pa[4];
    int nPaNum = 0;
    int nPlayType = VIDEOTYPE|AUDIOTYPE;
    
    AVP_parsePalyAddrURL([self.teaVideosUrl UTF8String],pa,nPaNum);
    pa[0].nUserID = (long)self.uid;
    pa[0].bIsStudent =false;
    pa[1].bIsStudent =false;
    pa[2].bIsStudent =false;
    pa[3].bIsStudent =false;
    switch (self.didChangeTeaVideosId) {
        case 1:
        {
            pa[1].bIsMainVideo = true;
            pa[1].bIsVideShow = true;
            pa[2].bIsMainVideo = false;
            pa[2].bIsVideShow = false;
            pa[3].bIsVideShow = false;
            pa[3].bIsMainVideo = false;
            pa[1].hwnd = (__bridge void*)self.teaVideoGlView;
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
            pa[2].hwnd = (__bridge void*)self.teaVideoGlView;
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
            pa[3].hwnd = (__bridge void*)self.teaVideoGlView;
            
        }
            break;
        default:
            break;
    }
    
    AVP_Change([self.teaVideosUrl UTF8String], nPlayType, pa, nPaNum, NULL);
}

/**
 *停止老师视频
 **/
- (void)stopTeaVideo
{
    if (self.teaVideosUrl.length <= 0) {
        CSLog(@"CLMovieView ===>> ERROR: 关闭教师视频时教师视频URL为空 关闭失败");
        return;
    }
    struct PlayAddress pa[4];
    int nPaNum = 0;
    
    pa[0].nUserID = (long)self.uid;
    pa[0].bIsStudent =false;
    pa[1].bIsStudent =false;
    pa[2].bIsStudent =false;
    pa[3].bIsStudent =false;
    switch (self.didChangeTeaVideosId) {
        case 1:
        {
            pa[1].bIsMainVideo = true;
            pa[1].bIsVideShow = true;
            pa[2].bIsMainVideo = false;
            pa[2].bIsVideShow = false;
            pa[3].bIsVideShow = false;
            pa[3].bIsMainVideo = false;
            pa[1].hwnd = (__bridge void*)self.teaVideoGlView;
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
            pa[2].hwnd = (__bridge void*)self.teaVideoGlView;
            
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
            pa[3].hwnd = (__bridge void*)self.teaVideoGlView;
            
        }
            break;
        default:
            break;
    }
    
    pa[1].hwnd = NULL;
    pa[2].hwnd = NULL;
    pa[3].hwnd = NULL;
    
    AVP_Stop([self.teaVideosUrl UTF8String], pa, nPaNum);
    
    self.teaVideoGlView.offScreen = YES;
    [self.teaVideoGlView clearFrame];
}


//===============================================================
//TODO: 提问学生(非登录用户)音视频相关操作
//===============================================================
/**
 *开始播放被提问的学生(非登录用户)音视频
 **/
- (void)startPlayAskStu:(NSString *)stuVideoUrl
{
    self.askStudentVideoUrl = stuVideoUrl;
    if (self.askStudentVideoUrl.length <= 0 ) {
        CSLog(@"CLMovieView ===>> ERROR: 开始播放被提问学生(非登录用户)视频时教师视频URL为空 播放失败");
        return;
    }
    self.askStuVideoGlView.isRefreshVideoSize = YES;
    [self.askStuActivityView startAnimating];
    self.clMovieCurrentStyle = CLMovieCurrentStyle_TeaAndStu;
    
    CSLog(@"CLMovieView ===>> 开始播放学生音视频地址:%@",self.askStudentVideoUrl);
    
    struct PlayAddress pa[2];
    int nPaNum = 0;
    int nPlayType = VIDEOTYPE|AUDIOTYPE;
    AVP_parsePalyAddrURL([self.askStudentVideoUrl UTF8String],pa,nPaNum);
    pa[0].nUserID = (long)self.uid;
    pa[0].bIsStudent =true;
    pa[1].bIsStudent =true;
    pa[1].bIsMainVideo = true;
    pa[1].bIsVideShow = true;
    pa[1].hwnd = (__bridge void*)self.askStuVideoGlView;
    
    AVP_Play([self.askStudentVideoUrl UTF8String],nPlayType,pa,nPaNum,NULL);
    self.askStuVideoGlView.offScreen = NO;
}

/**
 *停止播放被提问学生音视频
 **/
- (void)stopAskStuVideo
{
    if (self.askStudentVideoUrl.length <= 0) {
        CSLog(@"CLMovieView ===>> ERROR: 关闭被提问学生(非登录用户)视频时教师视频URL为空 关闭失败");
        return;
    }
    CSLog(@"CLMovieView ===>> 关闭被提问学生(非登录用户)视频时教师视频URL:%@",self.askStudentVideoUrl);
    
    self.clMovieCurrentStyle = CLMovieCurrentStyle_OnlyTea;
    struct PlayAddress pa[2];
    int nPaNum = 0;
    pa[0].nUserID = (long)self.uid;
    pa[0].bIsStudent =true;
    pa[1].bIsStudent =true;
    pa[1].bIsMainVideo = true;
    pa[1].bIsVideShow = true;
    pa[1].hwnd = NULL;
    
    AVP_Stop([self.askStudentVideoUrl UTF8String], pa, nPaNum);
    self.askStuVideoGlView.offScreen = YES;
    [self.askStuVideoGlView clearFrame];
    self.askStudentVideoUrl = nil;
}


//===============================================================
//TODO: 登录用户音视频相关操作
//===============================================================
/**
 *开始上传登录用户的音视频
 **/
- (void)startLoginUserVideo:(NSString *)pushVideoUrl
{
    if ([Utils objectIsNull:pushVideoUrl]) {
        CSLog(@"CLMovieView ===>> ERROR: 登录用户视频上传URL为空 打开失败失败");
        return;
    }
    self.clMovieCurrentStyle = CLMovieCurrentStyle_TeaAndLoginUser;
    self.askLoginUserVideoPushUrl = pushVideoUrl;
    
    //初始化视频预览页
    InitLibEnv((__bridge HWND)self.loginUserVideoView);
    self.mediaCollection = (__bridge MediaCollection *)getMediaMediaCollection();
    self.mediaCollection.video.hasCameraPullAwayAndNearly = NO;
    self.mediaCollection.video.hasFocusCursorWithTap = NO;
    
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(startPushLoginUserMedia) object:nil];
    [self performSelector:@selector(startPushLoginUserMedia) withObject:nil afterDelay:1.0f];

}

/**
 *延时一秒后开始传输登录用户音视频流
 **/
- (void)startPushLoginUserMedia {
    const char *szURl = [self.askLoginUserVideoPushUrl UTF8String];
    int nStreamType = SOURCECAMERA;
    MediaLevel m = HIGHLEVEL;
    if ([self.askLoginUserVideoPushUrl rangeOfString:@"|@|"].location!= NSNotFound) {
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

/**
 *停止上传登录用户的音视频
 **/
- (void)stopLoginUserVideo
{
    self.clMovieCurrentStyle = CLMovieCurrentStyle_OnlyTea;
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(startPushLoginUserMedia) object:nil];
    if (self.askLoginUserVideoPushUrl.length > 0) {
        const char *szURl = [self.self.askLoginUserVideoPushUrl UTF8String];
        rtmpPushStreamToServerEnd(szURl);
    }
    UninitLibEnv();
    [self.mediaCollection stopMediaCollection];
    self.mediaCollection = nil;
}



//===============================================================
//TODO: 公用操作
//===============================================================
/**
 *清除所有
 **/
- (void)clearnALL
{
    switch (self.clMovieCurrentStyle) {
        case CLMovieCurrentStyle_TeaAndStu:
        {
            [self stopAskStuVideo];
        }
            break;
        case CLMovieCurrentStyle_TeaAndLoginUser:
        {
            [self stopLoginUserVideo];
        }
            break;
        default:
            break;
    }
    [self stopTeaVideo];
}


@end
