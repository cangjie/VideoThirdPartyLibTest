#ifndef PLAYAUDIO_H
#define PLAYAUDIO_H

class CPCMPlayAudio
{
public:
	CPCMPlayAudio();
	~CPCMPlayAudio();
	bool StopSound();
	bool PlaySoundByDS(unsigned char* pBuf,unsigned int nSize,int nChannel, int nSample);
private:
	bool PlaySoundByDSInit();
private:
	int						m_samplerate;
	int						m_nchannel;

};

#endif