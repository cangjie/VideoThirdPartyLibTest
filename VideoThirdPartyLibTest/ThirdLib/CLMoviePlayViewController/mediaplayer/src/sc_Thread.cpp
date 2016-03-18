#include "sc_Thread.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include<time.h>

CSCThread::CSCThread(void)
{
	m_Callback = NULL;
	m_pUserData = NULL;
	m_hThread = NULL;
	m_bStopThread = true;
}


CSCThread::~CSCThread(void)
{
	End(0);
}

void* CALLBACK ProcSCThread(void* lpParameter)
{
	CSCThread *pThread = (CSCThread*)lpParameter;
	pThread->ThreadProcProxy();
	return NULL;
}

bool CSCThread::Begin(ThreadProc time_proc,void* pUserData)
{
	if(time_proc == NULL)
		return false;
	m_Callback = time_proc;
	m_pUserData = pUserData;
	m_bStopThread = false;
//	pthread_attr_t attr;
//	pthread_attr_init(&attr);
//  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
	if (pthread_create((pthread_t*)&m_hThread,NULL, ProcSCThread,this) == -1)
	{
		perror("Thread: create failed");
//		pthread_attr_destroy (&attr);
		return false;
	}
//	pthread_attr_destroy (&attr);
	return true;
}

bool CSCThread::End(int nWaitSec)
{
	if(m_hThread)
	{
		m_bStopThread = true;

		struct timespec ts;
        int ret = 0;//clock_gettime(1, &ts);
		ts.tv_sec += nWaitSec;
		printf("\e[1;31mEnd-%d\e[0m\n",nWaitSec);
        int s =1;//= pthread_timedjoin_np((pthread_t)m_hThread, NULL, &ts);
		
		if (s != 0)
		{
			printf("\e[1;31mEnd-wait timed out:error=%d %s ret=%d\e[0m\n",s,strerror(s),ret);
			pthread_cancel((pthread_t)m_hThread);
			pthread_join((pthread_t)m_hThread, NULL);
		}
		m_hThread = NULL;
	}
	return true;
}

void CSCThread::ThreadProcProxy()
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
	m_Callback(m_pUserData);
}

bool CSCThread::GetStop()
{
    pthread_testcancel();
	return m_bStopThread;
}
