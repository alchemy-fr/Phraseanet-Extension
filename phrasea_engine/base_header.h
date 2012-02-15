#ifndef BASE_HEADER_H
#define BASE_HEADER_H 1

#ifndef TRUE
	#define TRUE 1
#endif
#ifndef FALSE
	#define FALSE 0
#endif
#ifndef true
	#define true 1
#endif
#ifndef false
	#define false 0
#endif

#include <sys/types.h>
#include <sys/timeb.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <set>
#include <map>
#include <string>
#include <sstream>


#pragma warning(push)
#pragma warning(disable:4005)		// disable warning about double def of _WIN32_WINNT
#include "php.h"
#pragma warning(pop)

#include "ext/standard/info.h"
#include "ext/standard/php_string.h"
#include "zend_exceptions.h"

#ifdef PHP_WIN32
# include <winsock2.h>
# define signal(a, b) NULL
#elif defined(NETWARE)
# include <sys/socket.h>
# define signal(a, b) NULL
#else
# if HAVE_SIGNAL_H
#  include <signal.h>
# endif
# if HAVE_SYS_TYPES_H
#  include <sys/types.h>
# endif
# include <netdb.h>
# include <netinet/in.h>
#endif

// ******* tosee : mysql.h moved
#include <mysql.h>

// ******* tosee : fichiers headers de pgsql
#ifdef PHP_WIN32
// ******* tosee : HAVE_MEMMOVE is already defined including php.h
# undef HAVE_MEMMOVE
#else
#  undef PACKAGE_VERSION
#  undef PACKAGE_TARNAME
#  undef PACKAGE_NAME
#  undef PACKAGE_STRING
#  undef PACKAGE_BUGREPORT
#endif

#include "trace_memory.h"

#endif

