#ifndef CQUERYTREE2PARM_H
#define CQUERYTREE2PARM_H 1

class SQLCONN;

class Cquerytree2Parm
{
	public:
		CNODE *n;
		int depth;
		SQLCONN *sqlconn;
		pthread_mutex_t *sqlmutex;
		zval *result;
		char *sqltrec;
		char **psortField;
		int sortMethod;
		Cquerytree2Parm(CNODE *n, int depth, SQLCONN *sqlconn, pthread_mutex_t *sqlmutex, zval *result, char *sqltrec, char **psortField, int sortMethod);
};

#endif
