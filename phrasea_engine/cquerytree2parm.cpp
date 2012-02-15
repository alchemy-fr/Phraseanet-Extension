#include "base_header.h"
#include "phrasea_types.h"
#include "sql.h"

#include "mutex.h"
#include "cquerytree2parm.h"

#if defined(PHP_WIN32) && 0
Cquerytree2Parm::Cquerytree2Parm(CNODE *n, int depth, SQLCONN *sqlconn,                   zval *result, char *sqltrec, char **psortField, int sortMethod)
#else
Cquerytree2Parm::Cquerytree2Parm(CNODE *n, int depth, SQLCONN *sqlconn, CMutex *sqlmutex, zval *result, char *sqltrec, char **psortField, int sortMethod)
#endif
{
	this->n = n;
	this->depth = depth;
	this->sqlconn = sqlconn;
#if defined(PHP_WIN32) && 0
#else
	this->sqlmutex = sqlmutex;
#endif
	this->result = result;
	this->sqltrec = sqltrec;
	this->psortField = psortField;
	this->sortMethod = sortMethod;
}

