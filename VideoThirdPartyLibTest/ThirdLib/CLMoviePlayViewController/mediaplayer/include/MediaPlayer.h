#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H
#include <stddef.h>

typedef void* HIBITMAP;
typedef void*  HWNDHANDLE;

#define VIDEOTYPE 0x10000000
#define AUDIOTYPE 0x20000000

#define MPLATER_API

struct  PlayAddress
{
	int nMediaType;
	char szPushAddr[256];
	HWNDHANDLE hwnd;
	bool   bIsMainVideo;
	bool   bIsStudent;
	long   nUserID;
    bool   bIsVideShow;
	PlayAddress()
	{
		szPushAddr[0] = '\0';
		nMediaType = 0;
		hwnd = NULL;
		bIsMainVideo = false;
		bIsStudent = true;
		nUserID = 0;
        bIsVideShow = false;
	}
};

MPLATER_API bool   AVP_parsePalyAddrURL(const char* szPlayUrl,PlayAddress pa[4],int& nPaNum);

MPLATER_API bool   AVP_Play(const char* szPlayUrl,int nPlayStreamType,PlayAddress arrAddress[4],int narrAddressNum, HIBITMAP hBGbitmap);

MPLATER_API bool   AVP_Change(const char* szPlayUrl,int nPlayStreamType,PlayAddress arrAddress[4],int narrAddressNum,HIBITMAP hBGbitmap);


MPLATER_API bool   AVP_Stop(const char* szPlayUrl,PlayAddress arrAddress[4],int narrAddressNum);

MPLATER_API bool   AVP_PlayFile(const char* szFileName,HWNDHANDLE hwnd,HIBITMAP hBGbitmap);

MPLATER_API bool   AVP_StopFile(const char* szFileName,HWNDHANDLE hwnd);

MPLATER_API bool   AVP_PauseFile(const char* szFileName,bool bIsPause);

MPLATER_API bool   AVP_SeekFile(const char* szFileName,unsigned int  nPalyPos);

MPLATER_API unsigned int  AVP_GetFileDuration(const char* szFileName);

MPLATER_API unsigned int  getFileCurPlayTime(const char* szFileName);

MPLATER_API bool  playFileSwitch(const char* szCurPlayLocalFileName);

#endif