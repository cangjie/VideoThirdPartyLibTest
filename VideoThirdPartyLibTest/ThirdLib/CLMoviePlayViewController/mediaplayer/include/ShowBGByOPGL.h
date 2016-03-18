#ifndef SHOWBGBYOPGL_H
#define SHOWBGBYOPGL_H
#include "ShowVideo.h"
#include "PlayHeader.h"
#include "sc_CSLock.h"


class CShowBGByOPGL : public CShowVideo
{
public:
	CShowBGByOPGL(void* hBGbitmap,void* hShowWnd);
	virtual ~CShowBGByOPGL();
public:
	virtual bool ShowVideo(unsigned char* pBuf,unsigned int nBufSize,int nVH,int nVW) ;
	virtual bool DrawBGImage();
private:
	virtual bool InitShowEnv();
	virtual bool UninitShowEnv();
	void CopyBGImage(void* hBGbitmap);
private:

	char*           m_pRGBBuf;
	unsigned int    m_nRGBSize;
	int				m_nBGPicWidth ;
	int				m_nBGPicHeight;
	HShowWindow		m_show_rect;
	CMutexLock      m_drawBGLock;
private:
    void *          m_kxMovieGLView ;

};

#endif