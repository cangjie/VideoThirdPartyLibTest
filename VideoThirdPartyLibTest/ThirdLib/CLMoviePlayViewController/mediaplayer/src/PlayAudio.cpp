#include "PCMDataPlayer.h"

#include "playaudio.h"
#include <stddef.h>

PCMDataPlayer* g_pcmPlayer = nil;

CPCMPlayAudio::CPCMPlayAudio()
{
	m_samplerate = 0;
	m_nchannel = 0;
	PlaySoundByDSInit();
}

CPCMPlayAudio::~CPCMPlayAudio()
{
    if (g_pcmPlayer != nil)
    {
        [g_pcmPlayer stop];
        g_pcmPlayer = nil;
    }
}



bool CPCMPlayAudio::PlaySoundByDSInit()
{
    if (g_pcmPlayer != nil)
    {
        [g_pcmPlayer stop];
         g_pcmPlayer = nil;
    }
    g_pcmPlayer = [[PCMDataPlayer alloc] init];
    
	return true;

}

bool CPCMPlayAudio::PlaySoundByDS(unsigned char* pBuf,unsigned int nSize,int nChannel, int nSample)
{
    if(nChannel != m_nchannel ||  nSample !=  m_samplerate)
    {
        if(g_pcmPlayer != nil)
        {
            printf("#########################################\n");
            [g_pcmPlayer reset : nSample channel : nChannel];
        }
        
        m_nchannel = nChannel;
        m_samplerate = nSample;
    }
    [g_pcmPlayer play : pBuf length : nSize samp : nSample channel : nChannel];
	return true;
}

bool CPCMPlayAudio::StopSound()
{
    if(g_pcmPlayer != nil)
        [g_pcmPlayer stop];
	return true;
}