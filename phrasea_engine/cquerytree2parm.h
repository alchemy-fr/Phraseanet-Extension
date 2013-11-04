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
		char branch;
		int depth;
		SQLCONN *sqlconn;

		char *host; //										 row->field(1, "127.0.0.1"),	// host
		int port;	//								 atoi(row->field(2, "3306")),	// port
		char *user; //								 row->field(5, "root"),			// user
		char *pwd;	//								 row->field(6, ""),				// pwd
		char *base;	//								 row->field(4, "dbox"),			// base
		char *tmptable;	//								 tmptable						// tmptable

		char *sqltmp;			// sql create tmp

		zval *result;
		const char *sqltrec;
		char **psortField;
		int sortMethod;
		const char *business;
		CMutex *sqlmutex;
		struct sb_stemmer *stemmer;
		const char *lng;
		long rid;
		Cquerytree2Parm(
				CNODE *n,
				char branch,
				int depth,
				SQLCONN *sqlconn,

				char *host,
				int port,
				char *user,
				char *pwd,
				char *base,
				char *tmptable,

				char *sqltmp,

				CMutex *sqlmutex,
				zval *result,
				const char *sqltrec,
				char **psortField,
				int sortMethod,
				const char * business,
				sb_stemmer *stemmer,
				const char *lng,
				const unsigned long rid
			);
		~Cquerytree2Parm();
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
