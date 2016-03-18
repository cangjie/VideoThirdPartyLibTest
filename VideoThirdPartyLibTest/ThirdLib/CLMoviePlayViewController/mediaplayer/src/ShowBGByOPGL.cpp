#include "ShowBGByOPGL.h"
#include <string.h>
#include "MediaDecode.h"


#import "OpenGLView20.h"


CShowBGByOPGL::CShowBGByOPGL(void* hBGbitmap,void* hShowWnd):CShowVideo(DRAWTYPEGDI)
{
	m_pRGBBuf = NULL;
	m_nRGBSize = 0;
	m_nBGPicWidth = 0;
	m_nBGPicHeight = 0;

	InitShowEnv();

	memset(&m_show_rect,0,sizeof(HShowWindow));
	m_show_rect.hwnd = hShowWnd;
    m_kxMovieGLView = hShowWnd;
   // m_kxMovieGLView  = (__bridge OpenGLView20*)hShowWnd;
	//CopyBGImage(hBGbitmap);
}

CShowBGByOPGL::~CShowBGByOPGL()
{
	UninitShowEnv();
}

/*static NSData * copyFrameData(UInt8 *src, int linesize, int width, int height)
{
    //width = MIN(linesize, width);
    NSMutableData *md = [NSMutableData dataWithLength: width * height];
    Byte *dst = (Byte*)md.mutableBytes;
   // for (NSUInteger i = 0; i < height; ++i) {
    //    memcpy(dst, src, width);
   //     dst += width;
   //     src += linesize;
   // }
    int nSize  = width * height;
    memcpy(dst, src, nSize);
    dst += nSize;
    return md;
}*/

bool CShowBGByOPGL::ShowVideo(unsigned char* pBuf,unsigned int nBufSize,int nVH,int nVW)
{
    if(m_kxMovieGLView && pBuf)
    {
        //AVFrame*_videoFrame = (AVFrame*)pBuf;
        OpenGLView20* kxMovieGLView  = (__bridge OpenGLView20*)m_kxMovieGLView;
        if (kxMovieGLView) {
            [kxMovieGLView displayYUV420pData:pBuf width:nVW height:nVH];
        }

      /* KxVideoFrameYUV * yuvFrame  = [[KxVideoFrameYUV alloc] init];
        
        unsigned char* pY = pBuf;
        unsigned char* pU = (unsigned char*)(pBuf + (nVW * nVH));
        unsigned char* pV = (unsigned char*)(pBuf + (nVW * nVH) + (nVW * nVH)/4);
        yuvFrame.luma = copyFrameData(pY,
                                      0,
                                      nVW,
                                      nVH);
        
        yuvFrame.chromaB = copyFrameData(pU,
                                         0,
                                         nVW / 2,
                                         nVH / 2);
        
        yuvFrame.chromaR = copyFrameData(pV,
                                          0,
                                         nVW / 2,
                                         nVH / 2);
                                         
        KxVideoFrame* frame = yuvFrame;
        
        frame.height = nVH;
        frame.width = nVW;
        frame.duration = 0;
        frame.position = 0;
        frame.format = KxVideoFrameFormatYUV;
        [g_kxMovieGLView render:frame];*/
        
    }
	return true;
}


bool CShowBGByOPGL::InitShowEnv()
{
  /*  if(g_kxMovieGLView == nil)
    {
        //if(_decoder.validVideo)
        {
//            UIViewController *pself = (UIViewController*)m_show_rect.hwnd ;
//            
//            CGRect bounds = pself.view.bounds;
            g_kxMovieGLView = [[KxMovieGLView alloc] initWithFrame:CGRectMake(0, 0, 320, 320) decoder:_decoder];
        }
    }*/
	return true;
}

bool CShowBGByOPGL::UninitShowEnv()
{
	return true;
}

void CShowBGByOPGL::CopyBGImage(void* hBGbitmap)
{

}

bool CShowBGByOPGL::DrawBGImage()
{
	return false;
}
