#ifndef SQL_H
#define SQL_H 1

/*
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#if defined(LIBXML_XPATH_ENABLED) && defined(LIBXML_SAX1_ENABLED) && defined(LIBXML_OUTPUT_ENABLED)
#else
	#error XPath support not compiled in libxml
#endif
*/

#include "phrasea_types.h"	// define NODE
#include "cquerytree2parm.h"

class SQLCONN
{
	public:
		SQLCONN(const char *host, unsigned int port, const char *user, const char *passwd, const char *dbname);
		bool connect();
		void close();
		~SQLCONN();
		char *ukey;
//		bool isok();
		bool query(const char *sql, int len = -1);
		const char *error();
		int escape_string(const char *str, int len = -1, char *outbuff = NULL);
		void phrasea_query(const char *sql, Cquerytree2Parm *qp, char **sqlerr);
		my_ulonglong affected_rows();
		// #WIN - _int64 affected_rows();
		void *get_native_conn();
/*
		void thesaurus_search(const char *term);
		MYSQL_STMT *stmt_th;
		char *thesaurus_buffer;					// xml xtring
		unsigned long thesaurus_buffer_size;	//
		xmlDocPtr          DOMThesaurus;		// thesaurus libxml
		xmlXPathContextPtr XPathCtx_thesaurus;	// thesaurus xpath
*/
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
		const char *field(int n, const char *replaceNULL); // = NULL);
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
		bool query(const char *sql);
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

