

#include "PCMBuffer.h"
#include <stdio.h>
#include "wchar.h"
#include <string>
#include <stdlib.h>

CPCMBuffer::CPCMBuffer()
{
	m_pWPCMBuffer = NULL;
	m_nWPCMBufferSize = 50 * PCMFRAMESIZE;
	m_nRPCMBufferSize = PCMFRAMESIZE;
	m_pRPCMBuffer = NULL;
	nWritePCMPos = 0;
	nReadPCMPos = 0;
	m_nCycleWAR = 0;
	m_bFirstCahce = true;
	m_nErrCount = 0;
	m_nEacheNum = 3;
	m_nMaxEacheNum = 25;
	m_samplerate = 0;
	m_nchannel = 0;
	m_lastsamplerate = 0;
	m_lastnchannel = 0;
	m_nLastPcmFormatSize = 0;
	m_bIsFlag = false;
}

CPCMBuffer::~CPCMBuffer()
{
	if(m_pWPCMBuffer)
	{
		free(m_pWPCMBuffer);
		m_pWPCMBuffer = NULL;
	}
	if(m_pRPCMBuffer)
	{
		free(m_pRPCMBuffer);
		m_pRPCMBuffer = NULL;
	}
	nWritePCMPos = 0;
	nReadPCMPos = 0;
}

void CPCMBuffer::setConfig(bool bIsFlag)
{
	m_bIsFlag = bIsFlag;
	if(m_bIsFlag)
	{
		m_nEacheNum = 8;// ��ʼ����ʼ����֡��Ƶ�ſ�ʼ����
		m_nMaxEacheNum = 50; //��󻺴��20֡��Ƶ
		m_nWPCMBufferSize = 100 * PCMFRAMESIZE;
	}
}

void CPCMBuffer::ReSetConfig(unsigned int nJitterAvgTime)
{
	if(m_nEacheNum * PCMFRAMETIME < nJitterAvgTime)
	{
		m_nEacheNum = (nJitterAvgTime) / PCMFRAMETIME + 1;
	}
	else
		m_nEacheNum = 3;

	if(m_bIsFlag)
	{
		if(m_nEacheNum == 3)
			m_nEacheNum = 8;
		else
			m_nEacheNum *= 2;
		m_nEacheNum = m_nEacheNum > m_nMaxEacheNum ? m_nMaxEacheNum : m_nEacheNum;
	}

}

void CPCMBuffer::ClearBuffer()
{
	m_bFirstCahce = true;
	nWritePCMPos = 0;
	nReadPCMPos = 0;
	m_nCycleWAR = 0;
	m_nErrCount = 0;
	m_samplerate = 0;
	m_nchannel = 0;
	m_lastsamplerate = 0;
	m_lastnchannel = 0;
	if(m_bIsFlag)
	{
		m_nEacheNum = 8;// ��ʼ����ʼ����֡��Ƶ�ſ�ʼ����
		m_nMaxEacheNum = 50; //��󻺴��20֡��Ƶ
	}
	else
	{
		m_nEacheNum = 3;// ��ʼ����ʼ����֡��Ƶ�ſ�ʼ����
		m_nMaxEacheNum = 25; //��󻺴��20֡��Ƶ
	}
	m_nLastPcmFormatSize = 0;
}

unsigned int CPCMBuffer::GetCacheDataTime()
{
	int nPCMSize = PCMFRAMESIZE * m_nchannel * (m_samplerate / SIMPLERATENUM);
	float fltCacheTime  = (GetBusyPCMDataSize()* 1.0 / nPCMSize) * PCMFRAMETIME;
	unsigned int nCacheTime  = fltCacheTime / 10 * 10;
	//char szbuf[256] = {'/0'};
//	sprintf(szbuf,"recv data cache time              %d\n",nCacheTime);
	//OutputDebugString(szbuf);
	return nCacheTime;
}

unsigned int CPCMBuffer::GetBusyPCMDataSize()
{
	return (nWritePCMPos - nReadPCMPos) + (m_nCycleWAR > 0 ? m_nWPCMBufferSize:0);
}

void CPCMBuffer::WritePCMData(unsigned char* data,unsigned int nLen,int	samplerate,int channel)
{
	m_mutexLock.Lock();
	int nBusyDataSize = (nWritePCMPos - nReadPCMPos) + (m_nCycleWAR > 0 ? m_nWPCMBufferSize:0);

	if(m_bFirstCahce && nBusyDataSize / PCMFRAMESIZE >= m_nEacheNum )
	{
		m_bFirstCahce = false;
	}

	if(m_samplerate != samplerate || m_nchannel != channel)
	{
		m_lastsamplerate = m_samplerate;
		m_lastnchannel = m_nchannel;
		m_samplerate = samplerate;
		m_nchannel = channel;
		m_nLastPcmFormatSize = nBusyDataSize;
	}

	if( nBusyDataSize / PCMFRAMESIZE  > m_nMaxEacheNum)
	{
		
		int nPCMSize = PCMFRAMESIZE * m_nchannel * (m_samplerate / SIMPLERATENUM);
		float fltTime = PCMFRAMETIME * 1.0 / nPCMSize * nLen;
		m_nDorpTime += fltTime;
		m_mutexLock.Unlock();
		return ;
	}

	if(m_pWPCMBuffer == NULL)
		m_pWPCMBuffer = (unsigned char*)malloc(m_nWPCMBufferSize);
	if(m_pWPCMBuffer)
	{
		int nLastFree = m_nWPCMBufferSize - nWritePCMPos;
		if(nLastFree >= nLen)
		{
			memcpy(m_pWPCMBuffer+nWritePCMPos,data,nLen);
		}
		else
		{
			memcpy(m_pWPCMBuffer+nWritePCMPos,data,nLastFree);
			memcpy(m_pWPCMBuffer,data+nLastFree,nLen - nLastFree);
		}
		nWritePCMPos += nLen;
		if(nWritePCMPos >= m_nWPCMBufferSize)
			m_nCycleWAR++;
		nWritePCMPos %= m_nWPCMBufferSize;
		if(m_nCycleWAR > 1 && nWritePCMPos > nReadPCMPos)
			nReadPCMPos = nWritePCMPos;
	}
	m_mutexLock.Unlock();
}

void CPCMBuffer::ReadPCMData(unsigned char** data,unsigned int& nLen,unsigned int& nZreoData,unsigned int& nPtsSpan)
{
	int nCopySize = 0;

	m_mutexLock.Lock();
	int nBusyDataSize = (nWritePCMPos - nReadPCMPos) + (m_nCycleWAR > 0 ? m_nWPCMBufferSize:0);
	if(m_bFirstCahce && nBusyDataSize / PCMFRAMESIZE < m_nEacheNum )
	{
		m_mutexLock.Unlock();
		nLen = 0;
		return ;
	}
	/*if(nBusyDataSize > (m_nMaxEacheNum * 0.5) * PCMFRAMESIZE)
	{
		//nReadPCMPos = ((nBusyDataSize / PCMFRAMESIZE - 1) > 0 ? (nBusyDataSize / PCMFRAMESIZE - 1) : 0) *  PCMFRAMESIZE ;
		nPtsSpan += (PCMFRAMETIME * 5);
		nReadPCMPos += (PCMFRAMETIME * 5);
		nReadPCMPos %= m_nWPCMBufferSize;
	}
	else if(nBusyDataSize > (m_nMaxEacheNum * 0.4) * PCMFRAMESIZE)
	{
		nPtsSpan += (PCMFRAMETIME * 4);
		nReadPCMPos += (PCMFRAMETIME * 4);
		nReadPCMPos %= m_nWPCMBufferSize;
	}
	else if(nBusyDataSize > (m_nMaxEacheNum * 0.3) * PCMFRAMESIZE)
	{
		nPtsSpan += PCMFRAMETIME;
		nReadPCMPos += PCMFRAMETIME;
		nReadPCMPos %= m_nWPCMBufferSize;
	}*/
	
	if(nLen > m_nRPCMBufferSize)
	{
		if(m_pRPCMBuffer)
		{
			free(m_pRPCMBuffer);
			m_pRPCMBuffer = NULL;
		}
		m_nRPCMBufferSize = nLen;
	}

	if(m_pRPCMBuffer == NULL)
	{
		m_pRPCMBuffer = (unsigned char*)malloc(m_nRPCMBufferSize);
	}
	
	if(m_pRPCMBuffer)
	{
		if(nBusyDataSize > PCMFRAMESIZE)
		{
			unsigned  nDataSize =  0;
			memset(m_pRPCMBuffer,0,PCMFRAMESIZE);
			if(nWritePCMPos > nReadPCMPos)
			{
				nDataSize = nWritePCMPos - nReadPCMPos;
			}
			else if(nWritePCMPos == nReadPCMPos)
			{
				if(m_nCycleWAR > 0)
					nDataSize = m_nWPCMBufferSize;
				else
					nDataSize = 0;
			}
			else 
			{
				nDataSize = (m_nWPCMBufferSize - nReadPCMPos) + nWritePCMPos;
			}

			nCopySize = nDataSize > nLen ? nLen : nDataSize;

			if(nCopySize > 0)
			{
				if(nReadPCMPos + nCopySize > m_nWPCMBufferSize)
				{
					int nLastDataSize = m_nWPCMBufferSize - nReadPCMPos;
					memcpy(m_pRPCMBuffer,m_pWPCMBuffer+nReadPCMPos,nLastDataSize);
					memcpy(m_pRPCMBuffer+nLastDataSize,m_pWPCMBuffer,nCopySize-nLastDataSize);
				}
				else
				{
					memcpy(m_pRPCMBuffer,m_pWPCMBuffer+nReadPCMPos,nCopySize);
				}
				nReadPCMPos += nCopySize;
				if(nReadPCMPos >= m_nWPCMBufferSize)
					m_nCycleWAR--;
				nReadPCMPos %= m_nWPCMBufferSize;

				if(nCopySize < PCMFRAMESIZE)
					m_nErrCount++;
				else
					m_nErrCount = 0;
			}
			else
			{
				m_nErrCount++;
				memset(m_pRPCMBuffer,0,PCMFRAMESIZE);
				nCopySize = 0;
			}
		}
		else
		{
			m_nErrCount++;
			memset(m_pRPCMBuffer,0,PCMFRAMESIZE);
			nCopySize = 0;
		}
		
	}

	nZreoData = nLen - nCopySize;
	nLen = nCopySize;
	*data = m_pRPCMBuffer;
	m_mutexLock.Unlock();
}


