#ifndef MUTEX_H
#define MUTEX_H 1

#ifdef PHP_WIN32
#include <windows.h>
#endif

#include <stdio.h>

class CMutex
{
public:
	CMutex();
	~CMutex();
	void lock();
	void unlock();
private:
#ifdef PHP_WIN32
	HANDLE sqlmutex;
#else
	pthread_mutex_t sqlmutex;
#endif
};
#endif