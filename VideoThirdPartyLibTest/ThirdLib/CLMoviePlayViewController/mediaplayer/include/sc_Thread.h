#pragma once

#ifndef CALLBACK
#define CALLBACK
#endif

typedef void* HANDLE_THREAD;

typedef void* (*ThreadProc)(void* pUserData);


class CSCThread
{
public:
	CSCThread(void);
	~CSCThread(void);
	bool Begin(ThreadProc thread_proc, void* pUserData);
	bool End(int nWaitSec = 10);
	bool GetStop();
	void ThreadProcProxy();	
private:
	ThreadProc	    m_Callback;
	HANDLE_THREAD   m_hThread;
    void*           m_pUserData;
    bool            m_bStopThread;
};
