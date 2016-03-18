//
//  OpenGLView20.h
//  MyTest
//
//  Created by smy  on 12/20/11.
//  Copyright (c) 2011 ZY.SYM. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <OpenGLES/EAGL.h>
#include <sys/time.h>

@class OpenGLView20;
@protocol OpenGLViewDelgate <NSObject>

@optional
- (void)openglViewMoviePlaying;
- (void)videoPlaying:(OpenGLView20 *)glView;
@end

@interface OpenGLView20 : UIView
{
	/** 
	 OpenGL绘图上下文
	 */
    EAGLContext             *_glContext; 
	
	/** 
	 帧缓冲区
	 */
    GLuint                  _framebuffer; 
	
	/** 
	 渲染缓冲区
	 */
    GLuint                  _renderBuffer; 
	
	/** 
	 着色器句柄
	 */
    GLuint                  _program;  
	
	/** 
	 YUV纹理数组
	 */
    GLuint                  _textureYUV[3]; 
	
	/** 
	 视频宽度
	 */
    GLuint                  _videoW;  
	
	/** 
	 视频高度
	 */
    GLuint                  _videoH;
    
    GLsizei                 _viewScale;
	   
    //void                    *_pYuvData;
    
    /**
     * 视频起始位置
     **/
    GLuint                  _Startx;
    GLuint                  _Starty;
    
#ifdef DEBUG
    struct timeval      _time;
    NSInteger           _frameRate;
#endif
}
#pragma mark - 接口
- (void)displayYUV420pData:(void *)data width:(NSInteger)w height:(NSInteger)h;
- (void)setVideoSize:(GLuint)width height:(GLuint)height;

@property (assign, nonatomic) BOOL offScreen, /*关闭屏幕(锁屏)*/
isRefreshVideoSize;                           /*强制刷新视图尺寸 默认NO 当此属性为yes 时每针视频都是刷新尺寸 为no时只有是视频尺寸变化/窗口尺寸变化时才会刷新尺寸*/
@property (assign, nonatomic) id<OpenGLViewDelgate> playDelegate;
/** 
 清除画面
 */
- (void)clearFrame;
@end
