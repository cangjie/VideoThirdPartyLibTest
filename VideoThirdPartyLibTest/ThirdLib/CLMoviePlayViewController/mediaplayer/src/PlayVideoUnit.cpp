#include "MediaPlayer.h"
#include "PlayVideoUnit.h"
#include "flvHeader.h"
#include "ShowBGByOPGL.h"
#include <unistd.h>

int sample_rate[13] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350};

void* CALLBACK  _ShowVideo(void* dwUser)
{

    CPlayVideoUnit * pThis=(CPlayVideoUnit*)dwUser;
    pThis->OnShowVideo();
    return NULL;

}


CPlayVideoUnit::CPlayVideoUnit(fEvent_Callback fcallback,void* dwUser)
{
	m_isVideoPlaying = 0;
	m_mapShowHand.clear();
	m_netProcess = NULL;

	m_nFlvParseBufSize = 512 * 1024;
	m_pFlvParseBuf = NULL;
	m_FlvParsePos = 0;

	m_nFlvBufSize = 128*1024;
	m_pFlvBuf = (unsigned char*)malloc(m_nFlvBufSize);

	memset(m_H264PPSAndSPS,0,128);
	m_nH264PPSAndSPSSize = 0;

	m_nPlayVideoTime = 0;
	m_nPlayAudioTime = 0;
	m_bIsHaveAudio = false;

	m_DrawBGBegin.tv_sec = 0;
    m_DrawBGBegin.tv_usec = 0;
	m_bIsPlayLocal = false;
	m_szPlayVideoURL[0] = '\0';
	m_bIsMainVideo = false;
	m_playFile = NULL;
    m_dwUser = dwUser;
    m_fcallback = fcallback;
    m_bIsShowVideo = false;
}

CPlayVideoUnit::~CPlayVideoUnit()
{
	if(m_netProcess)
	{
		DestoryNetProcess(m_netProcess);
		m_netProcess = NULL;
	}

	if(m_playFile)
	{
		delete m_playFile;
		m_playFile = NULL;
	}

	if(m_pFlvBuf)
	{
		free(m_pFlvBuf);
		m_pFlvBuf = NULL;
	}
	if(m_pFlvParseBuf)
	{
		free(m_pFlvParseBuf);
		m_pFlvParseBuf = NULL;
	}

	ClearAllShowVideo();
}


CShowVideo*  CPlayVideoUnit::FindByHand(void* hShowWnd)
{
	CShowVideo*  pVS = NULL;
	m_mapShowLock.Lock();
	map<long,CShowVideo*>::iterator iter;
	iter = m_mapShowHand.find((long)hShowWnd);
	if(iter != m_mapShowHand.end())
		pVS = iter->second;
	m_mapShowLock.Unlock();
	return pVS;
}


void  CPlayVideoUnit::UpdateShowVideo(void* hBGbitmap,void* hShowWnd,int nType)
{
	CShowVideo* pSV = NULL;
    if(nType == DRAWTYPEOPGL)
    {
        pSV = new CShowBGByOPGL(hBGbitmap,hShowWnd);
    }
	m_mapShowLock.Lock();
	ClearAllShowVideo();
	m_mapShowHand[(long)hShowWnd] = pSV;
	m_mapShowLock.Unlock();
}

void  CPlayVideoUnit::AddShowVideo(void* hBGbitmap,void* hShowWnd,int nType)
{
	CShowVideo* pSV = NULL;
    if(nType == DRAWTYPEOPGL)
    {
        pSV = new CShowBGByOPGL(hBGbitmap,hShowWnd);
    }
	m_mapShowLock.Lock();

	m_mapShowHand[(long)hShowWnd] = pSV;
	m_mapShowLock.Unlock();
}

void  CPlayVideoUnit::DelShowVideo(void* hShowWnd)
{
	m_mapShowLock.Lock();
	map<long,CShowVideo*>::iterator iter;
	iter = m_mapShowHand.find((long)hShowWnd);
	if(iter != m_mapShowHand.end())
	{
		if(iter->second)
		{
			delete iter->second;
			iter->second = NULL;
		}
		m_mapShowHand.erase(iter);
	}
	m_mapShowLock.Unlock();
}

void  CPlayVideoUnit::ClearAllShowVideo()
{
	m_mapShowLock.Lock();
	map<long,CShowVideo*>::iterator iter;
	for(iter = m_mapShowHand.begin();iter != m_mapShowHand.end(); iter++)
	{
		CShowVideo* pSV = iter->second;
		if(pSV)
		{
            
		}
	}
	m_mapShowHand.clear();
	m_mapShowLock.Unlock();
}

CNetProcess * CPlayVideoUnit::CreateNetProcess(const char* szURL)
{
	CNetProcess* pn = NULL;
	if(szURL == NULL)
		return pn;
	if(strncmp(szURL,"rtmp://",strlen("rtmp://")) == 0)
	{
		pn = new CRTMPProcess();
	}
	else if(strncmp(szURL,"udp://",strlen("udp://")) == 0)
	{
		pn = new CUDPProcess();
	}
	else if(strncmp(szURL,"http://",strlen("http://")) == 0)
	{
	}
	return pn;
}

void CPlayVideoUnit::DestoryNetProcess(CNetProcess * np)
{
	if(np)
	{
		delete np;
		np = NULL;
	}
}

 void  CPlayVideoUnit::DrawVideo(unsigned char* pBuf,unsigned int nBufSize,int nVH,int nVW)
 {
	m_mapShowLock.Lock();
	map<long,CShowVideo*>::iterator iter;
	for(iter = m_mapShowHand.begin();iter != m_mapShowHand.end(); iter++)
	{
		CShowVideo* pSV = iter->second;
		if(pSV)
		{
			pSV->ShowVideo(pBuf,nBufSize,nVH,nVW);
		}
	}
	m_mapShowLock.Unlock();
 }

 void CPlayVideoUnit::DrawBGImage()
 {
	m_mapShowLock.Lock();
	map<long,CShowVideo*>::iterator iter;
	for(iter = m_mapShowHand.begin();iter != m_mapShowHand.end(); iter++)
	{
		CShowVideo* pSV = iter->second;
		if(pSV)
		{
			pSV->DrawBGImage();
		}
	}
	m_mapShowLock.Unlock();
 }

int  CPlayVideoUnit::GetShowHandCount()
{
	int nSize = 0;
	m_mapShowLock.Lock();
	nSize = m_mapShowHand.size();
	m_mapShowLock.Unlock();
	return nSize;
}

void  CPlayVideoUnit::setPlayAudioTime(int64_t nPlayAudioTime,bool bIsHaveAudio)
{
	m_nPlayAudioTime = nPlayAudioTime;
	m_bIsHaveAudio = bIsHaveAudio;
}


void CPlayVideoUnit::OnShowVideo()
{
	VideoFrame  vf;
    int nShowTime = 0;
	while(!m_threadShowVideo.GetStop())
	{
		H264VideoFrame hvf;
		int nSleepTime = 10 * 1000;
        struct timeval beg;
        gettimeofday(&beg,NULL);
		if(m_bIsPlayLocal)
		{
			if(m_playFile)
			{
				int nSpan = 0;
				m_playFile->decode_one_video_frame(vf,nSpan);
				m_nPlayVideoTime = vf.mVideoFramePts;
				if( m_nPlayVideoTime - m_nPlayAudioTime  > 200)
				{
					nSleepTime = (nSpan * 1.2) * 1000;
				}
				else if (m_nPlayAudioTime - m_nPlayVideoTime  > 200)
				{
					nSleepTime = (nSpan * 0.8) * 1000;
				}
				else
				{
					nSleepTime = nSpan  * 1000;
				}
				
                gettimeofday(&m_DrawBGBegin,NULL);
			}
		}
		else
		{
			m_vidJitterBuf.pop(hvf);
			if(hvf.pFrame)
			{
                if(m_bIsShowVideo)
                {
                    if(m_bIsMainVideo && m_fcallback)
                    {
                        m_fcallback(m_nPlayAudioTime,m_bIsHaveAudio,m_dwUser);
                    }
                
                    m_nPlayVideoTime = hvf.pts;
                    m_decoder.DecodeH264Video(hvf.pFrame,hvf.nFrameSize,hvf.pts,vf);
                    if(m_bIsHaveAudio && m_nPlayVideoTime - m_nPlayAudioTime  > 100)
                    {
                        nSleepTime = (hvf.nSpan * 1.2) * 1000;
                    }
                    else if (m_bIsHaveAudio && m_nPlayAudioTime - m_nPlayVideoTime  > 100)
                    {
                        if(m_nPlayAudioTime - m_nPlayVideoTime  > 1000)
                            nSleepTime = 0;
                        else
                            nSleepTime = (hvf.nSpan * 0.5) * 1000;
                    }
                    else
                    {
                        nSleepTime = (hvf.nSpan * 0.9)  * 1000;
                    }
                
                }
                gettimeofday(&m_DrawBGBegin,NULL);
                m_vidJitterBuf.freeVideoFrameMem(hvf);
			}
		}

        struct timeval nCurTime;
        gettimeofday(&nCurTime,NULL);
        int nDrawDiff = (nCurTime.tv_sec - m_DrawBGBegin.tv_sec)*1000 + (nCurTime.tv_usec - m_DrawBGBegin.tv_usec)/1000;
		if((vf.pVideoFrameBuf && vf.nVideoFrameSize > 0) && (nDrawDiff < 10000))
		{
			DrawVideo((unsigned char*)vf.pVideoFrameBuf,vf.nVideoFrameSize,vf.nVideoFrameHeight,vf.nVideoFrameWidth);
		}
		else
		{
			DrawBGImage();
		}
        struct timeval endshow;
        gettimeofday(&endshow,NULL);
        
        int nTSleepTime = 0;
        int nShowOneTime = (endshow.tv_sec - beg.tv_sec)*1000 + (endshow.tv_usec - beg.tv_usec)/1000;
        nShowTime += (nShowOneTime * 1000);
        
        int nTempTime = nSleepTime - nShowTime;
        if(nTempTime > 0 )
        {
            nTSleepTime = nTempTime;
            nShowTime = 0;
        }
        else
        {
            nTSleepTime = 0;
            nShowTime -= nSleepTime;
        }
        
        struct timeval bSleepTime;
        gettimeofday(&bSleepTime,NULL);
        
		//av_usleep(nSleepTime);
        usleep(nTSleepTime);
        struct timeval end;
        gettimeofday(&end,NULL);
        int ndiff = ((end.tv_sec - bSleepTime.tv_sec)*1000 + (end.tv_usec - bSleepTime.tv_usec)/1000) * 1000;
        if(ndiff > nTSleepTime)
        {
            nShowTime += (ndiff - nTSleepTime);
        }
       // printf("####################################### %d %d %d %d %d\n",nSleepTime/1000,nTSleepTime/1000,ndiff,nShowOneTime,nShowTime/1000);
	}
}

bool CPlayVideoUnit::PlayMedia(const char* szURL,void* hPlayHand,void* hBGbitmap,ThreadProc thread_proc,void* pUserData,bool bIsMainVideo,bool bIsShowVideo)
{

    m_bIsShowVideo = bIsShowVideo;
	if(FindByHand(hPlayHand) == NULL)
	{
		AddShowVideo(hBGbitmap,hPlayHand,DRAWTYPEOPGL);
	}

    if(m_isVideoPlaying == 1)
        return false;

	if(m_netProcess == NULL)
	{
		m_netProcess = CreateNetProcess(szURL);
	}

	m_nPlayAudioTime = 0;
	m_bIsHaveAudio = false;

	if(m_netProcess)
	{
		moudle_commond_t cmd = OPEN;
		m_netProcess->Activate(szURL,0);
		if(!m_netProcess->SetCommond(cmd,NULL))
			return false;			

		strcpy(m_szPlayVideoURL,szURL);

		if(m_pFlvParseBuf == NULL)
		{
			m_pFlvParseBuf = (unsigned char*)malloc(m_nFlvParseBufSize);
		}
		m_bIsMainVideo = bIsMainVideo;
		m_threadReadMedia.Begin(thread_proc,pUserData);
		m_threadShowVideo.Begin(_ShowVideo,this);
		
	}
    m_isVideoPlaying = 1;
	return true;
}


bool CPlayVideoUnit::ChangeMedia(void* hwnd,void* hBGbitmap,bool bIsMainVideo,bool bIsShowVideo)
{
	m_bIsMainVideo = bIsMainVideo;
    m_bIsShowVideo = bIsShowVideo;
    
	if(!m_bIsMainVideo)
	{
		m_nPlayAudioTime = 0;
		m_bIsHaveAudio = false;
	}

	if(FindByHand(hwnd) == NULL)
	{
		UpdateShowVideo(hBGbitmap,hwnd,DRAWTYPEOPGL);
	}
	return true;
}

bool CPlayVideoUnit::StopMeida(void* hPlayHand)
{
	bool bIsStop = false;
	if(m_isVideoPlaying == 1)
	{
		if(m_mapShowHand.size() == 1)
		{
			DelShowVideo(hPlayHand);
			if(m_netProcess)
			{
				moudle_commond_t cmd = STOP;
				m_netProcess->SetCommond(cmd,NULL);
			}
			m_threadReadMedia.End();
			m_threadShowVideo.End();
			bIsStop = true;
		}
		else
		{
			DelShowVideo(hPlayHand);
		}
	}

	if(bIsStop)
	{
		m_vidJitterBuf.clear();
		m_FlvParsePos = 0;
	}
	return bIsStop;
}

int CPlayVideoUnit::ReadMediaData()
{
	if(m_netProcess)
		return m_netProcess->ReadMedia(m_pFlvBuf,m_nFlvBufSize);
	return 0;
}

void CPlayVideoUnit::netStopCommond()
{
	if(m_netProcess)
	{
		moudle_commond_t cmd = STOP;
		m_netProcess->SetCommond(cmd,NULL);
	}
}

int CPlayVideoUnit::flvParse(unsigned char**pFlvPBuf,unsigned int& nFlvParseBufSize,unsigned int& nFlvParseBufPos,unsigned char * pBuf,unsigned int nBufSize,ParseFlvParam &pNode)
{
	if(pBuf == NULL  || nBufSize <= 0 )
		return 2;
	unsigned int nBufBusySize = 0;

	unsigned char*pFlvParseBuf = (*pFlvPBuf);

	if(nBufSize > (nFlvParseBufSize  - nFlvParseBufPos))
	{
		nFlvParseBufSize = nFlvParseBufPos + nBufSize * 2;
		unsigned char *pTemp = (unsigned char*)malloc(nFlvParseBufSize);
		memcpy(pTemp,pFlvParseBuf,nFlvParseBufPos);
		free(pFlvParseBuf);
		pFlvParseBuf = pTemp;
	}
	memcpy(pFlvParseBuf + nFlvParseBufPos,pBuf,nBufSize);

	nBufBusySize = nFlvParseBufPos + nBufSize;
	if(nFlvParseBufPos == 0 && pFlvParseBuf[0] == 'F' && pFlvParseBuf[1] == 'L' && pFlvParseBuf[2] == 'V' && nBufBusySize > 13)
	{
		nFlvParseBufPos += 13;
		nBufBusySize -= 13;
	}

	flv_tag_header_t  *ph = NULL;
	int tag_header_size = sizeof(flv_tag_header_t);
	while(nBufBusySize >  tag_header_size)
	{
		uint64_t npts = 0;
		int nstmp = 0;
		unsigned int data_size = 0;
		int tag_all_size = 0;
		int tag_size = 0;

		ph = (flv_tag_header_t *)(pFlvParseBuf+ nFlvParseBufPos);
		swicth_int(ph->tag_data_length,sizeof(ph->tag_data_length),&data_size);

		tag_all_size = tag_header_size + data_size + sizeof(int);
		FLVFILE_COPYSTAMP_INT(nstmp,ph->timestamp);

		if(tag_all_size > (nFlvParseBufSize-nFlvParseBufPos)) //Êý¾Ý³ö´í
		{
			nFlvParseBufPos = 0;
			return 1;
		}
		unsigned char * pprev = pFlvParseBuf + (nFlvParseBufPos + tag_all_size - sizeof(int));
		swicth_int(pprev,sizeof(tag_size),&tag_size);


		if(tag_size != tag_all_size -4)
		{
			return 0;
		}
		if(nBufBusySize < tag_all_size)
			break;

		nFlvParseBufPos += tag_header_size;
		nBufBusySize -= tag_header_size;

		switch(ph->tag_header_type )
		{
			case 0x08:
			{
				AACAudioFrame aaf;
				unsigned char* pstart = pFlvParseBuf + nFlvParseBufPos;
				pNode.bIsAudio = true;
				if(pstart[0] == 0xAF && pstart[1] == 0x00)
				{
					unsigned char nsamplerate = 0;
					unsigned char nchannel = 0;
					nsamplerate = ((( pstart[2] & 0x07) << 1) | (( pstart[3] & 0x80 ) >> 7));
					nchannel = ((pstart[3] & 0x78) >> 3);

					pNode.samplerate = sample_rate[nsamplerate];
					pNode.nchannel = 1;//nchannel;
					aaf.nFrameSize = data_size - 2;
					aaf.pFrame = pstart+2;
					aaf.pts = 0;
					pNode.bIsAAC = true;

					if(pNode.audJitterBuf)
						pNode.audJitterBuf->Push((char*)aaf.pFrame,aaf.nFrameSize,aaf.pts);
				}
				else if(pstart[0] == 0xAF && pstart[1] == 0x01)
				{
					aaf.pts = nstmp;
					aaf.nFrameSize  = data_size - 2;
					if(pNode.audJitterBuf)
						pNode.audJitterBuf->Push((char*)pstart+2,aaf.nFrameSize,aaf.pts);
				}
			}
			break;
			case 0x09:
			{
				//video
				unsigned char* pstart = pFlvParseBuf + nFlvParseBufPos;
				if(pNode.pPVU)
					pNode.pPVU->flvParseVideo(pstart,data_size,nstmp);
			}
			break;
			case 0x12:
			{
				int n = 0;
			}
			break;
			default:
			break;
		}
		nFlvParseBufPos += (data_size + sizeof(int));
		nBufBusySize -= (data_size + sizeof(int));      
    }
	memcpy(pFlvParseBuf,pFlvParseBuf + nFlvParseBufPos,nBufBusySize);
	nFlvParseBufPos = nBufBusySize;
	return 0;
}

int    CPlayVideoUnit::flvParseVideo(unsigned char*pstart,unsigned int data_size,int nstmp)
{
	if(pstart[0] == 0x17 && pstart[1] == 0x00)
	{
		pstart += 13;
		if(pstart[0] == 0x67)
		{
						
			m_nH264PPSAndSPSSize = data_size - 13 + 4 + 4;
			m_H264PPSAndSPS[0] = 0x00;
			m_H264PPSAndSPS[1] = 0x00;
			m_H264PPSAndSPS[2] = 0x00;
			m_H264PPSAndSPS[3] = 0x01;
			int nPos = data_size - 13- 4;
			memcpy(m_H264PPSAndSPS+4,pstart,nPos);
						
			pstart += nPos ;
			m_H264PPSAndSPS[nPos+4] = 0x00;
			m_H264PPSAndSPS[nPos+5] = 0x00;
			m_H264PPSAndSPS[nPos+6] = 0x00;
			m_H264PPSAndSPS[nPos+7] = 0x01;
			memcpy(m_H264PPSAndSPS+4+nPos+4,pstart,4);
		}
	}
	else
	{
		pstart += 9;
		unsigned char nNalType = pstart[0] & 0x1f;
		if(nNalType == 5)
		{
			m_H264PPSAndSPS[m_nH264PPSAndSPSSize] = 0x00;
			m_H264PPSAndSPS[m_nH264PPSAndSPSSize+1] = 0x00;
			m_H264PPSAndSPS[m_nH264PPSAndSPSSize+2] = 0x00;
			m_H264PPSAndSPS[m_nH264PPSAndSPSSize+3] = 0x01;
			m_vidJitterBuf.push(m_H264PPSAndSPS,m_nH264PPSAndSPSSize+4,pstart,data_size - 9,nstmp);
		}
		else
		{
			unsigned char prefix[4] = {0x00,0x00,0x00,0x01};
			m_vidJitterBuf.push(prefix,4,pstart,data_size - 9,nstmp);
		}			
	}
	return 1;
}


bool  CPlayVideoUnit::PlayFile(const char* szURL,void* hPlayHand,void* hBGbitmap,play_audio_callback pCallback,void* dwUser)
{
	if(FindByHand(hPlayHand) == NULL)
	{
		AddShowVideo(hBGbitmap,hPlayHand,DRAWTYPEBX);
	}

	if(m_isVideoPlaying == 1)
	{
		return false;
	}
	m_bIsPlayLocal = true;

	if(m_playFile == NULL)
		m_playFile = new CPlayFile();

	if(m_playFile)
	{
		m_playFile->OpenLocalMediaFile(szURL);
		m_threadShowVideo.Begin(_ShowVideo,this);
		m_playFile->audio_open(pCallback,dwUser);
        m_isVideoPlaying = 1;
		return true;
	}
	return false;
}

bool CPlayVideoUnit::PauseFile(bool bIsPause)
{
	if(m_playFile)
	{
		return m_playFile->pause_in_file(bIsPause);
	}
	return false;
}

bool CPlayVideoUnit::SeekFile(unsigned int nPalyPos)
{
	if(m_playFile)
	{
		return m_playFile->seek_in_file(nPalyPos);
	}
	return false;
}

bool CPlayVideoUnit::SwitchPaly(play_audio_callback pCallback,void* dwUser,bool bIsFlag)
{
	if(m_playFile)
	{
		return m_playFile->switch_paly(pCallback,this,bIsFlag);
	}
	return false;
}

unsigned int CPlayVideoUnit::GetFileDuration()
{
	if(m_playFile)
	{
		return m_playFile->get_in_file_duration();
	}
	return 0;
}
unsigned int CPlayVideoUnit::getFileCurPlayTime()
{
	if(m_playFile)
	{
		return m_playFile->get_in_file_current_play_time();
	}
	return 0;
}

void CPlayVideoUnit::stopFile()
{
	if(m_isVideoPlaying == 1)
	{
		if(m_playFile)
		{
			m_threadShowVideo.End();
			m_playFile->CloseLocalMediaFile();
			delete m_playFile;
			m_playFile = NULL;
		}
	}
}

void  CPlayVideoUnit::SDLPlayAudio(void *udata,unsigned char *stream,int len)
{
	AudioFrame af;
	if(m_playFile && m_playFile->decode_one_audio_frame(af))
	{
		m_nPlayAudioTime = af.mVideoFramePts;
		m_playFile->audio_play(udata,stream,len,af);
	}
}