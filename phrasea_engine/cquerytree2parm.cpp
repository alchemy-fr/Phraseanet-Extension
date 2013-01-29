#include "base_header.h"
#include "phrasea_types.h"
#include "sql.h"

#include "mutex.h"
#include "cquerytree2parm.h"

Cquerytree2Parm::Cquerytree2Parm(CNODE *n, int depth, SQLCONN *sqlconn, CMutex *sqlmutex, zval *result, const char *sqltrec, char **psortField, int sortMethod, const char *business, sb_stemmer *stemmer, const char *lng)
{
	this->n = n;
	this->depth = depth;
	this->sqlconn = sqlconn;
	this->sqlmutex = sqlmutex;
	this->result = result;
	this->sqltrec = sqltrec;
	this->psortField = psortField;
	this->sortMethod = sortMethod;
	this->business = business;
	this->stemmer = stemmer;
	this->lng = lng;
}

