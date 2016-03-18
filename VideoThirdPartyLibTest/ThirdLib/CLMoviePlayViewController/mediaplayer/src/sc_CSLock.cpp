#include "sc_CSLock.h"
#include <pthread.h>

CMutexLock::CMutexLock(void)
{
	m_pLock = NULL;
	m_pLock = new  pthread_mutex_t;
	pthread_mutexattr_t mutexattr;   // Mutex attribute variable
	pthread_mutexattr_init(&mutexattr);
	// Set the mutex as a recursive mutex
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
	// create the mutex with the attributes set
	pthread_mutex_init((pthread_mutex_t*)m_pLock, &mutexattr);
	//After initializing the mutex, the thread attribute can be destroyed
	pthread_mutexattr_destroy(&mutexattr);
}

CMutexLock::~CMutexLock(void)
{
	if(m_pLock)
	{
		pthread_mutex_destroy(( pthread_mutex_t*)m_pLock);
		pthread_mutex_t *p = (pthread_mutex_t*)m_pLock;
		delete p;
		m_pLock = NULL;
	}
}

void CMutexLock::Lock()
{
	if(m_pLock)
	{
		pthread_mutex_lock(( pthread_mutex_t*)m_pLock);
	}
}
void CMutexLock::Unlock()
{
	if(m_pLock)
	{
		pthread_mutex_unlock(( pthread_mutex_t*)m_pLock);
	}
}

void* CMutexLock::GetMutex()
{
	return m_pLock;
}

CSCAutoLock::CSCAutoLock(CMutexLock& lock)
{
	lock.Lock();
	m_pLock = &lock;
}

CSCAutoLock::CSCAutoLock()
{
	m_pLock = NULL;
}
CSCAutoLock::~CSCAutoLock()
{
	if (m_pLock)
		m_pLock->Unlock();
}
