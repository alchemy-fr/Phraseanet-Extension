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
	#define THREAD_ENTRYPOINT unsigned __stdcall
//	#define THREAD_ENTRYPOINT DWORD WINAPI
	#define ATHREAD HANDLE
	#define NULLTHREAD 0L
	#define THREAD_START(thread, entrypoint, parm) ((thread = (HANDLE)_beginthreadex(NULL, 0, (entrypoint), (void *)(parm), 0, NULL)) != (HANDLE)(-1))
	#define THREAD_JOIN(thread) WaitForSingleObject(thread, INFINITE)
	#define THREAD_EXIT(r) _endthreadex(r)
	#define SLEEP(nsec) Sleep(1000*(nsec))
	#define MYSQL_THREAD_SAFE false
//	#define MYSQL_THREAD_SAFE mysql_thread_safe()
#else
	#define THREAD_ENTRYPOINT void *
	#define ATHREAD pthread_t
	#define NULLTHREAD NULL
	#define THREAD_START(thread, entrypoint, parm) (pthread_create(&(thread), NULL, (entrypoint), (void *)(parm)) == 0)
	#define THREAD_JOIN(thread) pthread_join(thread, NULL)
	#define THREAD_EXIT(r) pthread_exit(r)
	#define SLEEP(nsec)	{ struct timespec delay = { (nsec), 0 }; nanosleep(&delay, NULL); }
	#define MYSQL_THREAD_SAFE false
//	#define MYSQL_THREAD_SAFE mysql_thread_safe()
#endif

#endif

