#include "flvHeader.h"
#include "MediaPlayer.h"
#include "PlayProcess.h"
#include <unistd.h>

#import "sc_Timer.h"



CPlayProcess * CPlayProcess:: g_PlayrArray[MAXPLAYERNUM];
CMutexLock   CPlayProcess::m_PlayrArrayLock;
int    CPlayProcess::m_nRefCount = 0;
int	CPlayProcess::m_nPlayAudioTimer = 0;

struct timeval   CPlayProcess::m_nLastPlayPcmSysTime;
struct timeval   CPlayProcess::m_nLastStopPcmSysTime;
struct timeval   CPlayProcess::m_begPlayAudioSysTime;
CPCMPlayAudio   CPlayProcess::m_playSound;
//CMutexLock      CPlayProcess::m_PlayAudioLock;


 short*  CPlayProcess::m_pResampleBuf = NULL;
 short*  CPlayProcess::m_pMixBuf = NULL;

selfOBTimer *g_audioTimer = nil;

void playAudioTimer()
{
    CPlayProcess::OnPlayTimer(1000);
}

void* CALLBACK _TimerReadMediaCallBack(void* dwUser)
{

      ViedePlayNode * pThis=(ViedePlayNode*)dwUser;
	  if( pThis && pThis->pPlayProcess)
	  {
		  pThis->pPlayProcess->OnReadMediaDataTimer(pThis->pVideoPlayUnit);
	  }
    return NULL;

}

void* CALLBACK _TimerReadAudioCallBack(void* dwUser)
{

    CPlayProcess * pThis=(CPlayProcess*)dwUser;
	
    pThis->OnReadAudioDataTimer(1);
    return NULL;

}


CPlayProcess::CPlayProcess()
{
	m_szPlayAudioURL[0] = '\0';

	m_bIsSeparate = true;

	m_netAudioProcess = NULL;
	m_isVideoPlaying = 0;
	m_isAudioPlaying = 0;

	m_pFlvAudioParseBuf = NULL;
	m_nFlvAudioParseBufSize = 512 * 1024;
	m_FlvAudioParsePos = 0;

	m_nFlvAudioBufSize = 1 * 1024;
	m_pFlvAudioBuf = (unsigned char*)malloc(m_nFlvAudioBufSize);

	m_samplerate = 32000;
	m_nchannel = 1;

	m_playVideoUnit.clear();

	m_nPlayAudioTime = 0;
	m_pCallback = NULL;

	m_bIsHaveAudio = false;
	m_bIsOpenAudio = false;

    if(CPlayProcess::m_nRefCount <= 0)
    {
        CPlayProcess::m_nLastPlayPcmSysTime.tv_sec = 0;
        CPlayProcess::m_nLastPlayPcmSysTime.tv_usec = 0;
        CPlayProcess::m_nLastStopPcmSysTime.tv_sec = 0;
        CPlayProcess::m_nLastStopPcmSysTime.tv_usec = 0;
        CPlayProcess::m_begPlayAudioSysTime.tv_sec = 0;
        CPlayProcess::m_begPlayAudioSysTime.tv_usec =0;
    }
    
    m_begPlayAudoPtsTime = 0;
    m_nCurPlayAudoPtsTime = 0;
    m_nPlayAudioSysDiffTime = 0;

	m_pcmHighbuffer.setConfig(true);

	m_bIsPlayLocal = false;
}

CPlayProcess::~CPlayProcess()
{
	StopALLMedia();

	if(m_netAudioProcess)
	{
		CPlayVideoUnit::DestoryNetProcess(m_netAudioProcess);
		m_netAudioProcess = NULL;
	}

	if(m_pFlvAudioBuf)
	{
		free(m_pFlvAudioBuf);
		m_pFlvAudioBuf = NULL;
	}

	if(m_pFlvAudioParseBuf)
	{
		free(m_pFlvAudioParseBuf);
		m_pFlvAudioParseBuf = NULL;
		m_FlvAudioParsePos = 0;
	}

}


bool  CPlayProcess::PlayFile(const char* szURL,void* hPlayHand,void* hBGbitmap)
{
	ViedePlayNode* pvu = findPVUnit(szURL);
	bool bIsExit = true;
	if(pvu == NULL)
	{
		pvu = new ViedePlayNode();
		pvu->pVideoPlayUnit = new CPlayVideoUnit(static_SyncAudio,this);
		pvu->pPlayProcess = this;
		ADDPVUnit(pvu);
	}
	return pvu->pVideoPlayUnit->PlayFile(szURL,hPlayHand,hBGbitmap,fill_audio,this);
}

bool CPlayProcess::PauseFile(const char* szURL,bool bIsPause)
{
	ViedePlayNode* pvu = findPVUnit(szURL);
	if(pvu && pvu->pVideoPlayUnit)
	{
		return pvu->pVideoPlayUnit->PauseFile(bIsPause);
	}
	return false;
}

bool CPlayProcess::SeekFile(const char* szURL,unsigned int nPalyPos)
{
	ViedePlayNode* pvu = findPVUnit(szURL);
	if(pvu && pvu->pVideoPlayUnit)
	{
		return pvu->pVideoPlayUnit->SeekFile(nPalyPos);
	}
	return false;
}

bool CPlayProcess::SwitchPaly(const char* szURL,bool bIsFlag)
{
	ViedePlayNode* pvu = findPVUnit(szURL);
	if(pvu && pvu->pVideoPlayUnit)
	{
		return pvu->pVideoPlayUnit->SwitchPaly(fill_audio,this,bIsFlag);
	}
	return false;
}

unsigned int CPlayProcess::GetFileDuration(const char* szURL)
{
	ViedePlayNode* pvu = findPVUnit(szURL);
	if(pvu && pvu->pVideoPlayUnit)
	{
		return pvu->pVideoPlayUnit->GetFileDuration();
	}
	return 0;
}

unsigned int CPlayProcess::getFileCurPlayTime(const char* szURL)
{
	ViedePlayNode* pvu = findPVUnit(szURL);
	if(pvu && pvu->pVideoPlayUnit)
	{
		return pvu->pVideoPlayUnit->getFileCurPlayTime();
	}
	return 0;
}

void CPlayProcess::stopFile(const char* szURL)
{

	ViedePlayNode* pvu = findPVUnit(szURL);
	if(pvu && pvu->pVideoPlayUnit)
	{
		return pvu->pVideoPlayUnit->stopFile();
	}
}


ViedePlayNode* CPlayProcess::findPVUnit(const char* szURL)
{
	ViedePlayNode* vpn = NULL;
	if(szURL)
	{
		m_playVideoLock.Lock();
		list<ViedePlayNode*>::iterator iter;
		for(iter = m_playVideoUnit.begin();iter != m_playVideoUnit.end();iter++)
		{
			vpn = (*iter);
			if(vpn && vpn->pVideoPlayUnit && strcmp(szURL,vpn->pVideoPlayUnit->getPalyAddress()) == 0)
			{
				break;
			}
			else
			{
				vpn = NULL;
			}
		}
		m_playVideoLock.Unlock();
	}
	return vpn;
}

ViedePlayNode* CPlayProcess::RemovePVUnit(const char* szURL)
{
	ViedePlayNode* pPVU = NULL;
	if(szURL)
	{
		m_playVideoLock.Lock();
		list<ViedePlayNode*>::iterator iter;
		for(iter = m_playVideoUnit.begin();iter != m_playVideoUnit.end();iter++)
		{
			pPVU = (*iter);
			if(pPVU && pPVU->pVideoPlayUnit && strcmp(szURL,pPVU->pVideoPlayUnit ->getPalyAddress()) == 0)
			{
				m_playVideoUnit.erase(iter);
				break;
			}
			else
			{
				pPVU = NULL;
			}
		}
		m_playVideoLock.Unlock();
	}
	return pPVU;
}


void  CPlayProcess::ADDPVUnit(ViedePlayNode* pvu)
{
	if(pvu)
	{
		m_playVideoLock.Lock();
		m_playVideoUnit.push_back(pvu);
		m_playVideoLock.Unlock();
	}
}

void CPlayProcess::OnReadMediaDataTimer(void * pUser)
{

	CPlayVideoUnit* puv = (CPlayVideoUnit* )pUser;
	if(puv)
	{
		while(!puv->m_threadReadMedia.GetStop())
		{
			if(puv->m_pFlvBuf)
			{
				int nReadSize = puv->ReadMediaData();
				if(nReadSize > 0)
				{
					ParseFlvParam node;
					node.audJitterBuf = &m_audJitterBuf;
					node.pPVU = puv;
					if(CPlayVideoUnit::flvParse(&puv->m_pFlvParseBuf,puv->m_nFlvParseBufSize,puv->m_FlvParsePos,puv->m_pFlvBuf,nReadSize,node) == 1)
					{
						puv->netStopCommond();
					}

					if(node.bIsAudio && node.bIsAAC)
					{
						m_samplerate = node.samplerate;
						m_nchannel = node.nchannel;
					}

					while(!m_bIsSeparate && DecodePCMDatatoAAC(false));
				}
				else
				{
					usleep(100*1000);
				}
			}
			else
			{
				usleep(10*1000);
			}
		}
	}
}

void CPlayProcess::OnReadAudioDataTimer(int nEventID)
{
	while(!m_threadReadAudio.GetStop())
	{	
		if(m_bIsOpenAudio && m_netAudioProcess  && m_pFlvAudioBuf)
		{
			int nReadSize = m_netAudioProcess->ReadMedia(m_pFlvAudioBuf,m_nFlvAudioBufSize);
			if(nReadSize > 0)
			{
				ParseFlvParam node;
				node.audJitterBuf = &m_audJitterBuf;
				node.pPVU = NULL;
				if(CPlayVideoUnit::flvParse(&m_pFlvAudioParseBuf,m_nFlvAudioParseBufSize,m_FlvAudioParsePos,m_pFlvAudioBuf,nReadSize,node) == 1)
				{
					if(m_bIsOpenAudio&&m_netAudioProcess)
					{
						moudle_commond_t cmd = STOP;
						m_netAudioProcess->SetCommond(cmd,NULL);
					}
				}
				if(m_bIsOpenAudio)
					while(DecodePCMDatatoAAC(false));
				else
				{
					m_audJitterBuf.reset();
				}
			}
			else
			{
				usleep(100*1000);
			}
		}
		else
		{
			usleep(100);
		}
	}
}

bool  CPlayProcess::DecodePCMDatatoAAC(bool bIsWaitFlag)
{
	AACAudioFrame aaf;
	AudioFrame     af;
	aaf.nSamplerate = m_samplerate;
	aaf.nChannel = m_nchannel;
	int nRe = 3;
	if(m_audJitterBuf.GetStatus())
	    nRe = m_audJitterBuf.Pop((char**)&aaf.pFrame,aaf.nFrameSize,(unsigned int&)aaf.pts,bIsWaitFlag);

	af.nPCMBufSize = 0;
	if(nRe <= 2)
	{
		if(aaf.pFrame)
		{
			af.nSamplerate = aaf.nSamplerate;
			af.nChannel = aaf.nChannel;
			m_decoder.DecodeAACAudio(aaf.pFrame,aaf.nFrameSize,aaf.pts,af);

			if(af.pPCMBuffer && af.nPCMBufSize > 0)
			{
				m_PlayAudioLock.Lock();
				if(m_nPlayAudioTime == 0)
				{
					m_nPlayAudioTime = aaf.pts;
					m_begPlayAudoPtsTime = aaf.pts;
				}

				if(aaf.nSamplerate == 16000)
				{
					if(aaf.nSamplerate != m_pcmLowbuffer.GetSsamplerate())
					{
						StopAudio();
						m_nPlayAudioTime = aaf.pts;
						m_begPlayAudoPtsTime = aaf.pts;
					}
					m_pcmLowbuffer.ReSetConfig(m_audJitterBuf.GetAvgRecvTime());
					m_pcmLowbuffer.WritePCMData(af.pPCMBuffer,af.nPCMBufSize,af.nSamplerate,af.nChannel);
                    
					m_nCurPlayAudoPtsTime = aaf.pts - m_begPlayAudoPtsTime - m_pcmLowbuffer.GetCacheDataTime();
                    
					if(abs((int)(m_nPlayAudioSysDiffTime - m_nCurPlayAudoPtsTime)) > 300)
					{
						unsigned int playcCurTime = aaf.pts - m_pcmLowbuffer.GetCacheDataTime() > 0 ? aaf.pts - m_pcmLowbuffer.GetCacheDataTime() : 0;
						m_nPlayAudioTime = playcCurTime;
						m_begPlayAudoPtsTime = playcCurTime;
					}
				}
				else if(aaf.nSamplerate == 32000)
				{
					if(aaf.nSamplerate != m_pcmHighbuffer.GetSsamplerate())
					{
						StopAudio();
						m_nPlayAudioTime = aaf.pts;
						m_begPlayAudoPtsTime = aaf.pts;
					}
					m_pcmHighbuffer.ReSetConfig(m_audJitterBuf.GetAvgRecvTime());
					m_pcmHighbuffer.WritePCMData(af.pPCMBuffer,af.nPCMBufSize,af.nSamplerate,af.nChannel);
                    
					m_nCurPlayAudoPtsTime = aaf.pts - m_begPlayAudoPtsTime - m_pcmHighbuffer.GetCacheDataTime();
                    
					if(abs((int)(m_nPlayAudioSysDiffTime - m_nCurPlayAudoPtsTime)) > 300)
					{
						unsigned int playcCurTime = aaf.pts - m_pcmHighbuffer.GetCacheDataTime() > 0 ? aaf.pts - m_pcmHighbuffer.GetCacheDataTime() : 0;
						m_nPlayAudioTime = playcCurTime;
						m_begPlayAudoPtsTime = playcCurTime;
					}
				}
				m_PlayAudioLock.Unlock();
				return true;
			}
		}
	}
	return false;
}

void CPlayProcess::static_SyncAudio(int64_t & nPlayAudioTime,bool& nHaveAudio,void* dwUser)
{
	CPlayProcess * pro = (CPlayProcess*)dwUser;
	if(pro)
	{
		pro->SyncAudio(nPlayAudioTime,nHaveAudio);
	}
}

void CPlayProcess::SyncAudio(int64_t & nPlayAudioTime,bool& nHaveAudio)
{
	nPlayAudioTime = m_nPlayAudioTime;
	nHaveAudio = m_bIsHaveAudio;
}


bool CPlayProcess::GetPCMData(unsigned char** pcm,int& pcmSize,int nSysSpan,int& nChannel,int& nSsamplerate)
{
   	if(m_nPlayAudioTime > 0 && (m_pcmLowbuffer.GetCacheStatus() || m_pcmHighbuffer.GetCacheStatus()))
    {
        unsigned int   xxsize = 0;
        if(nSysSpan >= PCMFRAMETIME)
        {
            unsigned int bufsize = 0;
            unsigned int nBusySize = 0;
            unsigned int nAudSpan = 0;
            if(nSysSpan > PCMFRAMETIME * 10)
                nSysSpan = PCMFRAMETIME * 10;
            
            unsigned int nReadOneSize = PCMFRAMESIZE * m_nchannel * (m_samplerate/SIMPLERATENUM);
            float fltPcmBuffSize = nReadOneSize * 1.0 / PCMFRAMETIME * nSysSpan;
            bufsize = fltPcmBuffSize;
            
            if(m_samplerate == 16000)
            {
                if(m_pcmHighbuffer.GetBusyPCMDataSize() > 0)
                {
                    m_pcmHighbuffer.ClearBuffer();
                }
                m_pcmLowbuffer.ReadPCMData(pcm,bufsize,xxsize,nAudSpan);
                nBusySize = m_pcmLowbuffer.GetBusyPCMDataSize();
            }
            else if(m_samplerate == 32000)
            {
                if(m_pcmLowbuffer.GetBusyPCMDataSize() > 0)
                {
                    m_pcmLowbuffer.ClearBuffer();
                }
                m_pcmHighbuffer.ReadPCMData(pcm,bufsize,xxsize,nAudSpan);
                nBusySize = m_pcmHighbuffer.GetBusyPCMDataSize();
            }
            
            if((*pcm) && bufsize > 0)
            {
                unsigned int nPSize = (nReadOneSize * 1.2) / 16 * 16;
                if(bufsize > nPSize)
                {
                    (*pcm)  = (*pcm)  + bufsize - nPSize;
                    bufsize = nPSize;
                }
                pcmSize = bufsize;
                nChannel = m_nchannel;
                nSsamplerate = m_samplerate;
                return true;
            }
        }
    }
    return false;
}

int CPlayProcess::ResamplePCM( short* pcm,unsigned int pcmsize,int nSrcSample,int nDestSample)
{
    if(pcm == NULL || pcmsize <= 0 || nSrcSample ==  nDestSample)
        return 0;
    
    if(m_pResampleBuf == NULL)
    {
        m_pResampleBuf = new  short[2560 * 25];
    }
    short* input_samples = pcm;
    int in_samples = pcmsize;
    int out_samples = pcmsize * 2;
    short* output_samples = m_pResampleBuf;
    
    int osample;
    
    uint32_t isample = 0;
    uint32_t istep = ((in_samples-2) << 16) / (out_samples - 2);
    for(osample = 0; osample < out_samples - 1; osample++)
    {
        int s1;
        int s2;
        short os;
        uint32_t t = (isample & 0xffff);
        s1 = input_samples[(isample >> 16)];
        s2 = input_samples[(isample >> 16) + 1];
        os = (s1 * (0x10000 - t) + s2 * t) >> 16;
        
        output_samples[osample] = os;
        isample += istep;
    }
    output_samples[out_samples -1] = input_samples[in_samples -1];
    return out_samples;
}


int CPlayProcess::SoundMixing( short* pcm,unsigned int pcmsize,bool& bIsFirst)
{
    if(pcm == NULL || pcmsize <= 0)
        return 0;
    if(m_pMixBuf == NULL)
    {
        m_pMixBuf = new  short[2560 * 25];
    }
    static unsigned int nSize = 0;
    if(m_pMixBuf)
    {
        if(bIsFirst)
        {
            memcpy(m_pMixBuf,pcm,pcmsize*sizeof(short));
            bIsFirst = false;
            nSize = pcmsize ;
        }
        else
        {
            short data1 = 0;
            short data2 = 0;
            int nMinSize = nSize > pcmsize ? pcmsize : nSize;
            int nMaxSize = nSize > pcmsize ? nSize : pcmsize;
            int i = 0;
            for(i = 0 ;i < nMinSize ;i++)
            {
                data1 = m_pMixBuf[i];
                data2 = pcm[i];
                if(data1 < 0 && data2 < 0)
                    m_pMixBuf[i] = data1 + data2 - (data1 * data2 / -(pow(2,16-1)-1));
                else
                    m_pMixBuf[i] = data1 + data2 - (data1 * data2 / (pow(2,16-1)-1));
            }
            
            if(pcmsize > nMinSize)
            {
                memcpy((char*)m_pMixBuf+(nMinSize*2),(char*)pcm+(nMinSize*2), (pcmsize-nMinSize)*sizeof(short));
            }
            nSize = nMaxSize;
        }
    }
    return nSize;
}

void CPlayProcess::OnPlayTimer(int nEventID)//调用要回调的成员方法
{
    CPlayProcess::m_PlayrArrayLock.Lock();
	if(m_nRefCount > 0) //时间戳递增说明有音频数据可以播放
	{
		unsigned char* buf = NULL;
		int   bufsize = 0;
		unsigned int   xxsize = 0;
        struct timeval nCurSysTime;
        gettimeofday(&nCurSysTime,NULL);
        
		if(m_nLastPlayPcmSysTime.tv_sec == 0)
		{
			m_nLastPlayPcmSysTime = nCurSysTime;
		}

		m_nLastStopPcmSysTime = nCurSysTime;

		if(m_begPlayAudioSysTime.tv_sec == 0)
		{
			m_begPlayAudioSysTime = nCurSysTime;
		}
        
		int nSysSpan = (nCurSysTime.tv_sec - m_nLastPlayPcmSysTime.tv_sec)*1000 + (nCurSysTime.tv_usec - m_nLastPlayPcmSysTime.tv_usec)/1000;
     
		unsigned int nStopTime = 200;
		if(nSysSpan > nStopTime)
		{
            StopAudioEx();
		}
		else if(nSysSpan >= PCMFRAMETIME)
		{
            int nChannel = 1;
            int nSsamplerate = 0;
            int nDestSample = 0;
            
            int nPlayAudioSysDiffTime = (nCurSysTime.tv_sec - m_begPlayAudioSysTime.tv_sec)*1000 +(nCurSysTime.tv_usec - m_begPlayAudioSysTime.tv_usec)/1000 ;
            bool bIsMixFirst = true;
            int nMixSize = 0;
            for(int i = 0;i < MAXPLAYERNUM ;i++)
            {
                if(CPlayProcess::g_PlayrArray[i])
                {
                    if(CPlayProcess::g_PlayrArray[i]->GetPCMData(&buf,bufsize, nSysSpan,nChannel,nSsamplerate) )
                    {
                        if(buf && bufsize > 0)
                        {
                            CPlayProcess::g_PlayrArray[i]->UpdatePlayAudioTime(nSysSpan,true,nPlayAudioSysDiffTime);
                            if(CPlayProcess::m_nRefCount > 1)
                            {
                                if((nSsamplerate != 32000 || nChannel != 1 ))
                                {
                                    int nDestSize = ResamplePCM((short*)buf,bufsize/2,nSsamplerate,32000);
                                    if(nDestSize > 0)
                                    {
                                        nMixSize = SoundMixing((short*)m_pResampleBuf,nDestSize,bIsMixFirst);
                                    }
                                    
                                }
                                else
                                {
                                    nMixSize = SoundMixing((short*)buf,bufsize/2,bIsMixFirst);
                                }
                                nDestSample = 32000;
                            }
                            else
                            {
                                nMixSize = SoundMixing((short*)buf,bufsize/2,bIsMixFirst);
                                nDestSample = nSsamplerate;
                            }
                        }
                    }
                }
            }
            
			if(m_pMixBuf && nMixSize > 0)
			{
				m_nLastPlayPcmSysTime = nCurSysTime;
				m_playSound.PlaySoundByDS((unsigned char*)m_pMixBuf,nMixSize*2,nChannel,nDestSample);
			}
		}
	}
	else
	{
        struct timeval nCurSysTime;
        gettimeofday(&nCurSysTime,NULL);
		int nSysSpan = (nCurSysTime.tv_sec - m_nLastStopPcmSysTime.tv_sec)*1000 + (nCurSysTime.tv_usec - m_nLastStopPcmSysTime.tv_usec)/1000;
		if(nSysSpan > 500)
		{
			m_playSound.StopSound();
		}
	}
    CPlayProcess::m_PlayrArrayLock.Unlock();

	
}

void CPlayProcess::UpdatePlayAudioTime(int nSysSpan,bool bIsHaveVideo,int nPlayAudioSysDiffTime)
{
    m_nPlayAudioTime += nSysSpan;
    m_bIsHaveAudio = bIsHaveVideo;
    m_nPlayAudioSysDiffTime = nPlayAudioSysDiffTime;
}

void CPlayProcess::StopAudioEx()
{
    CPlayProcess::m_PlayrArrayLock.Lock();
    for(int i = 0;i < MAXPLAYERNUM ;i++)
    {
        if(CPlayProcess::g_PlayrArray[i])
        {
            CPlayProcess::g_PlayrArray[i]->StopAudio();
        }
    }
    m_nLastPlayPcmSysTime.tv_sec = 0;
    m_nLastPlayPcmSysTime.tv_usec = 0;
    m_begPlayAudioSysTime.tv_sec = 0;
    m_begPlayAudioSysTime.tv_usec = 0;
    m_playSound.StopSound();
    CPlayProcess::m_PlayrArrayLock.Unlock();
}

void CPlayProcess::StopAudio()
{
	m_PlayAudioLock.Lock();

	m_pcmLowbuffer.ClearBuffer();
	m_pcmHighbuffer.ClearBuffer();
    m_bIsHaveAudio = false;
	m_nPlayAudioTime = 0;
	m_PlayAudioLock.Unlock();

}

void  CPlayProcess::fill_audio(void *udata,unsigned char *stream,int len)
{   
	CPlayProcess *palyPro = (CPlayProcess *)udata;
	if(palyPro)
	{
		palyPro->SDLPlayAudio(udata,stream,len);
	}
}

void  CPlayProcess::SDLPlayAudio(void *udata,unsigned char *stream,int len)
{
	if(m_playVideoUnit.size())
	{
		ViedePlayNode* pnode = m_playVideoUnit.front();
		if(pnode && pnode->pVideoPlayUnit)
		{
			pnode->pVideoPlayUnit->SDLPlayAudio(udata,stream,len);
		}
	}
}

int  CPlayProcess::GetShowHandCount(const char* szURL)
{
	if(szURL)
	{
		ViedePlayNode* pvu = findPVUnit(szURL);
		if(pvu && pvu->pVideoPlayUnit)
		{
			return pvu->pVideoPlayUnit->GetShowHandCount();
		}
	}
	return 0;
}

void CPlayProcess::SetCallBack(IMediaPlayerEvent* pCallback)
{
	m_pCallback = pCallback;
}

bool  CPlayProcess::ChangeMedia(const char* szURL,int nPlayStreamType,HWNDHANDLE hwnd,HIBITMAP hBGbitmap,bool bIsPlay,long nUserID,bool bIsMainVideo,bool bIsStudent,bool bIsShowVideo)
{
	int nTempPlayVideo = nPlayStreamType & VIDEOTYPE;
	int nTempPlayAudio = nPlayStreamType & AUDIOTYPE;

	if(bIsStudent)
	{
		m_samplerate = 16000;
		m_nchannel = 1;
	}
	if(nTempPlayVideo == VIDEOTYPE)
	{
		ViedePlayNode* pvu = findPVUnit(szURL);
		if(pvu && pvu->pVideoPlayUnit)
		{
			return pvu->pVideoPlayUnit->ChangeMedia(hwnd,hBGbitmap,bIsMainVideo,bIsShowVideo);
		}
	}

	if(nTempPlayAudio == AUDIOTYPE)
	{
		m_bIsOpenAudio = bIsPlay;
		if(m_bIsOpenAudio)
		{
			if(m_isAudioPlaying == 0)
			{
				if(m_netAudioProcess == NULL)
				{
					m_netAudioProcess = CPlayVideoUnit::CreateNetProcess(szURL);
					
				}
				if( m_netAudioProcess )
				{
					m_audJitterBuf.Init();
					moudle_commond_t cmd = OPEN;
					m_netAudioProcess->Activate(szURL,nUserID);
					if(!m_netAudioProcess->SetCommond(cmd,NULL))
					{
						return false;	
					}
					if(m_pFlvAudioParseBuf == NULL)
					{
						m_pFlvAudioParseBuf = (unsigned char*)malloc(m_nFlvAudioParseBufSize);
					}
				}

				CPlayProcess::m_PlayrArrayLock.Lock();
	
			//	if(CPlayProcess::m_nPlayAudioTimer == 0 || CPlayProcess::m_nRefCount == 0)
                if(g_audioTimer == nil || CPlayProcess::m_nRefCount == 0)
				{
					CPlayProcess::m_nRefCount = 0;
                    if(g_audioTimer == nil)
                    {
                        g_audioTimer = [[selfOBTimer alloc]init];
                    }
                    if(g_audioTimer)
                    {
                        [g_audioTimer timeStart : playAudioTimer span:0.0001];
                    }
                    //todo liwu
					//CPlayProcess::m_nPlayAudioTimer = timeSetEvent(5,1,&TimerPlayCallBack,this,1);
				}
				for(int i = 0; i < MAXPLAYERNUM ; i++)
				{
					if(g_PlayrArray[i] == NULL)
					{
						g_PlayrArray[i] = this;
						CPlayProcess::m_nRefCount++;
						break;
					}
				}
				CPlayProcess::m_PlayrArrayLock.Unlock();
                m_isAudioPlaying = 1;
				if(m_threadReadAudio.GetStop())
					m_threadReadAudio.Begin(_TimerReadAudioCallBack,this);
			}
		}
		else
		{
			if(m_isAudioPlaying == 1)
			{
				if(m_netAudioProcess)
				{
					moudle_commond_t cmd = STOP;
					m_netAudioProcess->SetCommond(cmd,NULL);
					StopAudio();
					m_FlvAudioParsePos = 0;
					m_pcmLowbuffer.ClearBuffer();
					m_pcmHighbuffer.ClearBuffer();

					CPlayProcess::m_PlayrArrayLock.Lock();
					for(int i = 0; i < MAXPLAYERNUM ; i++)
					{
						if(g_PlayrArray[i] == this)
						{
							CPlayProcess::m_nRefCount--;
							g_PlayrArray[i] = NULL;
						}
					}
					CPlayProcess::m_PlayrArrayLock.Unlock();
                    m_isAudioPlaying = 0;
				}
			}
		}
	}
	return true;
}

bool  CPlayProcess::PlayMedia(const char* szURL,int nPlayStreamType,void* hPlayHand,void* hBGbitmap,bool bIsPlay,long nUserID,bool bIsMainVideo,bool bIsStudent,bool bIsShowVideo)
{
	int nTempPlayAudio = nPlayStreamType & AUDIOTYPE;
	int nTempPlayVideo = nPlayStreamType & VIDEOTYPE;

	if(bIsStudent)
	{
		m_samplerate = 16000;
		m_nchannel = 1;
	}

	if(nTempPlayVideo == VIDEOTYPE)
	{
		ViedePlayNode* pvu = findPVUnit(szURL);
		bool bIsExit = true;
		if(pvu == NULL)
		{
			pvu = new ViedePlayNode();
			pvu->pVideoPlayUnit = new CPlayVideoUnit(static_SyncAudio,this);
			pvu->pPlayProcess = this;
			bIsExit = false;
		}

		if(nTempPlayAudio == AUDIOTYPE)
		{
			m_audJitterBuf.Init();
			m_bIsSeparate = false;
		}

		pvu->pVideoPlayUnit->PlayMedia(szURL,hPlayHand,hBGbitmap,_TimerReadMediaCallBack,pvu,bIsMainVideo,bIsShowVideo);
		
		if( !bIsExit )
			ADDPVUnit(pvu);

	}

	if(nTempPlayAudio == AUDIOTYPE )
	{
		if(m_isAudioPlaying == 0)
		{
			m_bIsOpenAudio = bIsPlay;
			if(m_bIsOpenAudio)
			{
				if( nTempPlayVideo != VIDEOTYPE)
				{
					if(m_netAudioProcess == NULL)
					{
						m_netAudioProcess = CPlayVideoUnit::CreateNetProcess(szURL);
					
					}
					if( m_netAudioProcess )
					{
						m_audJitterBuf.Init();
						moudle_commond_t cmd = OPEN;
						m_netAudioProcess->Activate(szURL,nUserID);
						if(!m_netAudioProcess->SetCommond(cmd,NULL))
						{
							return false;	
						}
						if(m_pFlvAudioParseBuf == NULL)
						{
							m_pFlvAudioParseBuf = (unsigned char*)malloc(m_nFlvAudioParseBufSize);
						}
					}
				}

				CPlayProcess::m_PlayrArrayLock.Lock();
	
				//if(CPlayProcess::m_nPlayAudioTimer == 0 || CPlayProcess::m_nRefCount == 0)
                 if(g_audioTimer == nil || CPlayProcess::m_nRefCount == 0)
				{
					CPlayProcess::m_nRefCount = 0;
                    if(g_audioTimer == nil)
                    {
                        g_audioTimer = [[selfOBTimer alloc]init];
                    }
                    if(g_audioTimer)
                    {
                        [g_audioTimer timeStart : playAudioTimer span:0.0001];
                    }
					//CPlayProcess::m_nPlayAudioTimer = timeSetEvent(5,1,&TimerPlayCallBack,this,1);
				}
				for(int i = 0; i < MAXPLAYERNUM ; i++)
				{
					if(g_PlayrArray[i] == NULL)
					{
						g_PlayrArray[i] = this;
						CPlayProcess::m_nRefCount++;
						break;
					}
				}
				CPlayProcess::m_PlayrArrayLock.Unlock();
                m_isAudioPlaying = 1;

				m_bIsOpenAudio = true;
				if(m_threadReadAudio.GetStop())
					m_threadReadAudio.Begin(_TimerReadAudioCallBack,this);
			}
		}
	}
	return true;

}

bool  CPlayProcess::StopALLMedia()
{
	if(m_playVideoUnit.size() > 0)
	{
		m_playVideoLock.Lock();
		list<ViedePlayNode*>::iterator iter;
		for(iter = m_playVideoUnit.begin(); iter != m_playVideoUnit.end();iter++)
		{
			ViedePlayNode* pNode = (*iter);
			if(pNode)
			{
				if(pNode->pVideoPlayUnit)
				{
					if(m_bIsPlayLocal)
						pNode->pVideoPlayUnit->stopFile();
					else
						pNode->pVideoPlayUnit->StopMeida(NULL);
					delete pNode->pVideoPlayUnit;
					pNode->pVideoPlayUnit = NULL;
				}
				delete pNode;
				pNode = NULL;
			}
		}
		m_playVideoUnit.clear();
		m_playVideoLock.Unlock();

		if(m_isAudioPlaying == 1)
		{
			m_threadReadAudio.End();
			if(m_netAudioProcess)
			{
				moudle_commond_t cmd = STOP;
				m_netAudioProcess->SetCommond(cmd,NULL);
			}
            m_isAudioPlaying = 0;
		}

		CPlayProcess::m_PlayrArrayLock.Lock();
		for(int i = 0; i < MAXPLAYERNUM ; i++)
		{
			if(g_PlayrArray[i] == this)
			{
				CPlayProcess::m_nRefCount--;
				g_PlayrArray[i] = NULL;
			}
		}
		if(CPlayProcess::m_nRefCount == 0 && CPlayProcess::m_nPlayAudioTimer)
		{

            if(g_audioTimer)
            {
                [g_audioTimer timeEnd];
            }
			CPlayProcess::m_nPlayAudioTimer = 0;
		}

		m_audJitterBuf.unInit();
		m_FlvAudioParsePos = 0;
		StopAudio();
		m_pcmLowbuffer.ClearBuffer();
		m_pcmHighbuffer.ClearBuffer();
		CPlayProcess::m_PlayrArrayLock.Unlock();
	}
	return true;
}

bool  CPlayProcess::StopMedia(const char* szURL,void* hPlayHand)
{
    bool bIsStop = false;
	ViedePlayNode* pvu = findPVUnit(szURL);
	if(pvu && pvu->pVideoPlayUnit)
	{
		bIsStop = pvu->pVideoPlayUnit->StopMeida(hPlayHand);
	}

	if(m_bIsSeparate)
	{
		if(m_isAudioPlaying == 1)
		{
			m_threadReadAudio.End();
			if(m_netAudioProcess)
			{
				moudle_commond_t cmd = STOP;
				m_netAudioProcess->SetCommond(cmd,NULL);
				bIsStop = true;
			}
            m_isAudioPlaying = 0;
		}
	}

	if(bIsStop)
	{
		CPlayProcess::m_PlayrArrayLock.Lock();
		m_begPlayAudioSysTime.tv_sec = 0;
        m_begPlayAudioSysTime.tv_usec = 0;
		m_begPlayAudoPtsTime = 0;
		m_nCurPlayAudoPtsTime = 0;
		m_nPlayAudioSysDiffTime = 0;
		for(int i = 0; i < MAXPLAYERNUM ; i++)
		{
			if(g_PlayrArray[i] == this)
			{
				CPlayProcess::m_nRefCount--;
				g_PlayrArray[i] = NULL;
			}
		}
		if(CPlayProcess::m_nRefCount == 0 && CPlayProcess::m_nPlayAudioTimer)
		{
            if(g_audioTimer)
            {
                [g_audioTimer timeEnd];
            }
			CPlayProcess::m_nPlayAudioTimer = 0;
		}
		m_audJitterBuf.unInit();
		m_FlvAudioParsePos = 0;
		StopAudio();
		m_pcmLowbuffer.ClearBuffer();
		m_pcmHighbuffer.ClearBuffer();
		CPlayProcess::m_PlayrArrayLock.Unlock();
	}
	return true;
}
