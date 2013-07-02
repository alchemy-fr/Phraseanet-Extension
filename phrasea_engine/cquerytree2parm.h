#ifndef CQUERYTREE2PARM_H
#define CQUERYTREE2PARM_H 1

#pragma warning(push)
#pragma warning(disable:4005)		// disable warning about double def of _WIN32_WINNT
#include "php.h"
#pragma warning(pop)


#include "mutex.h"
#include "../libstemmer_c/include/libstemmer.h"

class SQLCONN;

class Cquerytree2Parm
{
	public:
		CNODE *n;
		int depth;
		SQLCONN *sqlconn;
		zval *result;
		const char *sqltrec;
		char **psortField;
		int sortMethod;
		const char *business;
		CMutex *sqlmutex;
		struct sb_stemmer *stemmer;
		const char *lng;
		Cquerytree2Parm(CNODE *n, int depth, SQLCONN *sqlconn, CMutex *sqlmutex, zval *result, const char *sqltrec, char **psortField, int sortMethod, const char * business, sb_stemmer *stemmer, const char *lng);
};

struct Squerytree2Parm
{
		CNODE *n;
		int depth;
		SQLCONN *sqlconn;
		zval *result;
		char *sqltrec;
		char **psortField;
		int sortMethod;
		CMutex *sqlmutex;
};

#endif
