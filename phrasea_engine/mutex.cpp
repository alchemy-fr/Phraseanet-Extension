#include "mutex.h"

CMutex::CMutex()
{
#ifdef PHP_WIN32
	this->sqlmutex = CreateMutex(NULL, FALSE, NULL);
#else
	pthread_mutex_init(&(this->sqlmutex), NULL);
#endif
}

CMutex::~CMutex()
{
#ifdef PHP_WIN32
	CloseHandle(this->sqlmutex);
#else
	pthread_mutex_destroy(&(this->sqlmutex));
#endif
}

void CMutex::lock()
{
#ifdef PHP_WIN32
	WaitForSingleObject(this->sqlmutex, INFINITE);
#else
	pthread_mutex_lock(&(this->sqlmutex));
#endif
}

void CMutex::unlock()
{
#ifdef PHP_WIN32
	ReleaseMutex(this->sqlmutex);
#else
	pthread_mutex_unlock(&(this->sqlmutex));
#endif
}
