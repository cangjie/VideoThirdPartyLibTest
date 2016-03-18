#ifndef PLAYVIDEOUNIT_H
#define PLAYVIDEOUNIT_H
#include <sys/time.h>
#include "VideoJitterBuffer.h"
#include "AudioJitterBuffer.h"

#include "rtmpprocess.h"
#include "udpprocess.h"
#include "MediaDecode.h"
#include "PlayFile.h"
#include "sc_CSLock.h"
#include "ShowVideo.h"

#include <map>
using namespace std;

class CPlayVideoUnit;

struct ParseFlvParam
{
	CAudioJitterBuffer *audJitterBuf;
	bool                bIsAudio;
	bool                bIsAAC;
	int					samplerate;
	int					nchannel;  
	CPlayVideoUnit *    pPVU;

	ParseFlvParam()
	{
		audJitterBuf = NULL;
		bIsAudio = false;
		bIsAAC = false;
		samplerate = 0;
		nchannel = 0;
		pPVU = NULL;
	}
};

typedef void (* fEvent_Callback )(int64_t& nPlayAudioTime,bool & nHaveAudio,void* dwUser);

class CPlayVideoUnit
{
public:
	CPlayVideoUnit(fEvent_Callback fcallback,void* dwUser);

	~CPlayVideoUnit();
public:
	bool PlayMedia(const char* szURL,void* hPlayHand,void* hBGbitmap,ThreadProc thread_proc,void* pUserData,bool bIsMainVideo = false,bool bIsShowVideo = false);
	bool ChangeMedia(void* hwnd,void* hBGbitmap,bool bIsMainVideo = false,bool bIsShowVideo = false);
	bool StopMeida(void* hPlayHand);

	int ReadMediaData();
	inline CVideoJitterBuffer* GetVideoJitterBuffer() {return &m_vidJitterBuf;}
	void OnShowVideo();
	int  GetShowHandCount();
	void netStopCommond();

	bool  PlayFile(const char* szURL,void* hPlayHand,void* hBGbitmap,play_audio_callback pCallback,void* dwUser);
	void  stopFile();
	bool PauseFile(bool bIsPause);
	bool SeekFile(unsigned int nPalyPos);
	unsigned int GetFileDuration();
	unsigned int getFileCurPlayTime();
	bool SwitchPaly(play_audio_callback pCallback,void* dwUser,bool bIsFlag = false);
	void  SDLPlayAudio(void *udata,unsigned char *stream,int len);

	inline const char* getPalyAddress() { return m_szPlayVideoURL ;}
	static int flvParse(unsigned char**pFlvPBuf,unsigned int& nFlvParseBufSize,unsigned int& nFlvParseBufPos,unsigned char * pBuf,unsigned int nBufSize,ParseFlvParam &pNode);
	static CNetProcess *CreateNetProcess(const char* szURL);
	static void DestoryNetProcess(CNetProcess * np);

	void  setPlayAudioTime(int64_t nPlayAudioTime,bool bIsHaveAudio);
private:
	 CShowVideo*  FindByHand(void* hShowWnd);
	 void		  UpdateShowVideo(void* hBGbitmap,void* hShowWnd,int nType);
	 void         AddShowVideo(void* hBGbitmap,void* hShowWnd,int nType);
	 void         DelShowVideo(void* hShowWnd);
	 void         ClearAllShowVideo();
	 void         DrawVideo(unsigned char* pBuf,unsigned int nBufSize,int nVH,int nVW);
	 void         DrawBGImage();
	 int          flvParseVideo(unsigned char*pstart,unsigned int data_size,int nstmp);


public:
	unsigned char * m_pFlvParseBuf;
	unsigned int   m_nFlvParseBufSize;
	unsigned int   m_FlvParsePos;

	unsigned char  *m_pFlvBuf;
	unsigned int    m_nFlvBufSize;
	CSCThread	    m_threadReadMedia;
private:
	map<long,CShowVideo*>  m_mapShowHand;
	CMutexLock          m_mapShowLock;
	long				m_isVideoPlaying;

	CNetProcess *		m_netProcess;
	CSCThread			m_threadShowVideo;

	CPlayFile			*m_playFile;
	bool				m_bIsPlayLocal;
private:


	CVideoJitterBuffer   m_vidJitterBuf;
	CMediaDecoder        m_decoder; 

	unsigned char m_H264PPSAndSPS[128];
	unsigned int  m_nH264PPSAndSPSSize;
private:
	int64_t			m_nPlayVideoTime;
	int64_t			m_nPlayAudioTime;
	struct timeval  m_DrawBGBegin;
	bool            m_bIsHaveAudio;
	char			m_szPlayVideoURL[512];
	bool			m_bIsMainVideo;
    bool            m_bIsShowVideo ;
    void*           m_dwUser;
    fEvent_Callback m_fcallback;
};
#endif