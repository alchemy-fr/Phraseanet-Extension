#ifndef SQL_H
#define SQL_H 1

#include "phrasea_types.h"	// define NODE
#include "cquerytree2parm.h"

// void phrasea_query(char *sql, Cquerytree2Parm *qp);


class SQLCONN
{
	public:
		SQLCONN(char *host, unsigned int port, char *user, char *passwd, char *dbname);
		bool connect();
		void close();
		~SQLCONN();
		char *ukey;
//		bool isok();
		bool query(char *sql, int len = -1);
		const char *error();
		int escape_string(char *str, int len = -1, char *outbuff = NULL);
		void phrasea_query(char *sql, Cquerytree2Parm *qp);
		my_ulonglong affected_rows();
		// #WIN - _int64 affected_rows();
		void *get_native_conn();

		// class SQLRES *store_result();
		friend class SQLRES;
		friend class SQLROW;

		char host[1024];
		int port;
		char user[1024];
		char passwd[1024];
		char dbname[1024];
	private:
		bool connok;
		MYSQL mysql_connx;
		int mysql_active_result_id;

};

class SQLROW
{
	public:
		SQLROW();
		~SQLROW();
		char *field(int n);
		friend class SQLRES;
		friend class SQLCONN;
	private:
		MYSQL_ROW   row;
		class SQLRES *parent_res;
};

class SQLRES
{
	public:
		SQLRES(SQLCONN *parent_conn);
		~SQLRES();
		bool query(char *sql);
		class SQLROW *fetch_row();
		int get_nrows();
		friend class SQLROW;
		friend class SQLCONN;
		unsigned long *fetch_lengths();
	private:
		class SQLROW sqlrow;
		MYSQL_RES   *res;
		SQLCONN *parent_conn;
		int nrows, ncols, currow;
};

#endif

