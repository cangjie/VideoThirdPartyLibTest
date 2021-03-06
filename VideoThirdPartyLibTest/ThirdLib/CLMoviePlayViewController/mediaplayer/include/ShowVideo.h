#ifndef SHOWVIDEO_H
#define SHOWVIDEO_H

#define DRAWTYPEBX 0
#define DRAWTYPEGDI 1
#define DRAWTYPEOPGL 2

class CShowVideo
{
public:
	CShowVideo(int nType) {m_nType = nType;}
	virtual ~CShowVideo() {};
public:
	virtual bool InitShowEnv() = 0;
	virtual bool UninitShowEnv() = 0;
	virtual bool ShowVideo(unsigned char* pBuf,unsigned int nBufSize,int nVH,int nVW) = 0;
	virtual bool DrawBGImage() = 0;
	virtual inline int  getShowType()
	{
		return m_nType;
	}
private:
	int m_nType;
};
#endif