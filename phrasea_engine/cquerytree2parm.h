#ifndef CQUERYTREE2PARM_H
#define CQUERYTREE2PARM_H 1

#include "mutex.h"

class SQLCONN;

class Cquerytree2Parm
{
	public:
		CNODE *n;
		int depth;
		SQLCONN *sqlconn;
		zval *result;
		char *sqltrec;
		char **psortField;
		int sortMethod;
		bool business;
		CMutex *sqlmutex;
		Cquerytree2Parm(CNODE *n, int depth, SQLCONN *sqlconn, CMutex *sqlmutex, zval *result, char *sqltrec, char **psortField, int sortMethod, bool business);
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
