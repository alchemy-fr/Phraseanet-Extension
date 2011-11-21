#include "base_header.h"
#include "phrasea_types.h"
#include "sql.h"

#include "cquerytree2parm.h"

Cquerytree2Parm::Cquerytree2Parm(CNODE *n, int depth, SQLCONN *sqlconn, pthread_mutex_t *sqlmutex, zval *result, char *sqltrec, char **psortField, int sortMethod)
{
	this->n = n;
	this->depth = depth;
	this->sqlconn = sqlconn;
	this->sqlmutex = sqlmutex;
	this->result = result;
	this->sqltrec = sqltrec;
	this->psortField = psortField;
	this->sortMethod = sortMethod;
}

