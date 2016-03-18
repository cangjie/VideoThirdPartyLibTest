#ifndef PLAYPROCESS_H
#define PLAYPROCESS_H

#include "PCMBuffer.h"
#include "playaudio.h"
#include "PlayVideoUnit.h"

#include <map>

using namespace std;
struct IMediaPlayerEvent;
#define MAXPLAYERNUM  24

class CPlayProcess;

struct ViedePlayNode
{
	CPlayProcess * pPlayProcess;
	CPlayVideoUnit* pVideoPlayUnit;
	ViedePlayNode()
	{
		pPlayProcess = NULL;
		pVideoPlayUnit = NULL;
	}
};

class CPlayProcess
{
public:
	CPlayProcess();
	~CPlayProcess();
public:
	bool  PlayMedia(const char* szURL,int nPlayStreamType,void* hPlayHand,void* hBGbitmap,bool bIsPlay,long nUserID,bool bIsMainVideo =false,bool bIsStudent=true,bool bIsShowVideo = false);
	bool  StopMedia(const char* szURL,void* hPlayHand);
	bool  StopALLMedia();
	bool  ChangeMedia(const char* szURL,int nPlayStreamType,HWNDHANDLE hwnd,HIBITMAP hBGbitmap,bool bIsPlay,long nUserID,bool bIsMainVideo =false,bool bIsStudent=true,bool bIsShowVideo = false);

	bool  PlayFile(const char* szURL,void* hPlayHand,void* hBGbitmap);
	void  stopFile(const char* szURL);
	bool PauseFile(const char* szURL,bool bIsPause);
	bool SeekFile(const char* szURL,unsigned int nPalyPos);
	unsigned int GetFileDuration(const char* szURL);
	unsigned int getFileCurPlayTime(const char* szURL);
	bool SwitchPaly(const char* szURL,bool bIsFlag = false);

	static void  fill_audio(void *udata,unsigned char *stream,int len);
	void  SDLPlayAudio(void *udata,unsigned char *stream,int len);

	inline bool getIsPlayFile() {return  m_bIsPlayLocal;}


	void OnReadMediaDataTimer(void * pUser);
	void OnReadAudioDataTimer(int nEventID);
	int  GetShowHandCount(const char* szURL);
	void SetCallBack(IMediaPlayerEvent* pCallback);

	static void static_SyncAudio(int64_t & nPlayAudioTime,bool& nHaveAudio,void* dwUser);
	void SyncAudio(int64_t & nPlayAudioTime,bool& nHaveAudio);

    bool GetPCMData(unsigned char** pcm,int& pcmSize,int nSysSpan,int& nChannel,int& nSsamplerate);
    static void OnPlayTimer(int nEventID);
    
    static int ResamplePCM( short* pcm,unsigned int pcmsize,int nSrcSample,int nDestSample);
    static int SoundMixing( short* pcm,unsigned int pcmsize,bool& bIsFirst);
private:
	bool  DecodePCMDatatoAAC(bool bIsWaitFlag = true);
    static void StopAudioEx();
    void StopAudio();
    void UpdatePlayAudioTime(int nSysSpan,bool bIsHaveVideo,int nPlayAudioSysDiffTime);

	CNetProcess *CreateNetProcess(const char* szURL);
	void DestoryNetProcess(CNetProcess * np);
private:
	ViedePlayNode* findPVUnit(const char* szURL);
	ViedePlayNode* RemovePVUnit(const char* szURL);
	void            ADDPVUnit(ViedePlayNode* pvu);
	 
private:
	char   m_szPlayAudioURL[512];
private:
	bool   m_bIsSeparate;
	long    m_isAudioPlaying;
	long    m_isVideoPlaying;
private:
	CNetProcess          *m_netAudioProcess;
	list<ViedePlayNode*>  m_playVideoUnit;
	CMutexLock            m_playVideoLock;
	bool                  m_bIsPlayLocal; 
private:

	unsigned char *m_pFlvAudioParseBuf;
	unsigned int   m_nFlvAudioParseBufSize;
	unsigned int   m_FlvAudioParsePos;

	unsigned char  *m_pFlvAudioBuf;
	unsigned int    m_nFlvAudioBufSize;
 
private:
	CMediaDecoder            m_decoder;   
	CAudioJitterBuffer       m_audJitterBuf;
	CPCMBuffer               m_pcmLowbuffer;
	CPCMBuffer               m_pcmHighbuffer;
    static  short*          m_pResampleBuf;
    static  short*          m_pMixBuf;
private:
	CMutexLock               m_sAudioLock;
private:
	int						m_samplerate;
	int						m_nchannel;
    
	static struct timeval   m_nLastPlayPcmSysTime;
	static struct timeval   m_nLastStopPcmSysTime;
	static struct timeval   m_begPlayAudioSysTime;
    
	int64_t                          m_begPlayAudoPtsTime;
	int64_t                          m_nCurPlayAudoPtsTime;
    int64_t                          m_nPlayAudioSysDiffTime;
private:
	CSCThread		m_threadReadAudio;
	int64_t			m_nPlayAudioTime;
	bool            m_bIsHaveAudio;
	bool            m_bIsOpenAudio;
    CMutexLock      m_PlayAudioLock;

	IMediaPlayerEvent* m_pCallback;
public:
	static CPlayProcess *g_PlayrArray[MAXPLAYERNUM];
	static CMutexLock    m_PlayrArrayLock;
	static int           m_nRefCount;
	static int		     m_nPlayAudioTimer;
    
private:
    static CPCMPlayAudio   m_playSound;
   
};
#endif