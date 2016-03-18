
#include "quit.h"

void* CALLBACK _ExitPlay(void* dwUser)
{
    StopUnit * pThis=(StopUnit*)dwUser;
	pThis->stopEx();
    return NULL;
}


StopUnit:: StopUnit()
{
	rp = NULL;
	hwnd = NULL;
	nIndex = -1;
}

StopUnit:: ~StopUnit()
{
}


void StopUnit::clear()
{
	if(rp)
	{
		delete rp;
		rp = NULL;
	}
	hwnd = NULL;
	nIndex = -1;
}

void StopUnit::stop()
{
	m_thread_stop.Begin((ThreadProc)_ExitPlay,this);
}

void StopUnit::stopEx()
{
	if(rp)
	{
		rp->StopALLMedia();
		delete rp;
		rp = NULL;
	}
	clear();
}