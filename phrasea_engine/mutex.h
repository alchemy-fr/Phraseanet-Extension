#ifndef MUTEX_H
#define MUTEX_H 1

#ifdef PHP_WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

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