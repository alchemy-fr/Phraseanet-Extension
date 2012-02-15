#ifndef THREAD_INCLUDED
#define THREAD_INCLUDED 1

#if defined(_MSC_VER)
// # include <tchar.h>
#else
# define TCHAR		char
# define _T(x)		x
# define _tprintf	printf
# define _tmain		main
#endif

#ifdef WIN32
	#include <windows.h>
	#include <process.h>
	#define THREAD_ENTRYPOINT void
	#define ATHREAD HANDLE
	#define NULLTHREAD 0L
	#define THREAD_START(thread, entrypoint, parm) ((thread = (HANDLE)_beginthread((entrypoint), 0, (void *)(parm))) != (HANDLE)(-1))
	#define THREAD_JOIN(thread) WaitForSingleObject(thread, INFINITE)
	#define THREAD_EXIT(r) _endthread()
	#define SLEEP(nsec) Sleep(1000*(nsec))
#else
	#define THREAD_ENTRYPOINT void *
	#define ATHREAD pthread_t
	#define NULLTHREAD NULL
	#define THREAD_START(thread, entrypoint, parm) (pthread_create(&(thread), NULL, (entrypoint), (void *)(parm)) == 0)
	#define THREAD_JOIN(thread) pthread_join(thread, NULL)
	#define THREAD_EXIT(r) pthread_exit(r)
	#define SLEEP(nsec)	{ struct timespec delay = { (nsec), 0 }; nanosleep(&delay, NULL); }
#endif

#endif

