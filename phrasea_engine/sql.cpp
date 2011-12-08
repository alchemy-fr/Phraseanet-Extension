#include <mysql.h>

#include "base_header.h"
#include "phrasea_clock_t.h"

#include "sql.h"

void ftrace(char *fmt, ...);

#define QUOTE(x) _QUOTE(x)
#define _QUOTE(a) #a 

SQLCONN::SQLCONN(char *host, unsigned int port, char *user, char *passwd, char *dbname)
{
	this->ukey = NULL;
	this->connok = false;
	this->mysql_active_result_id = 0;

	mysql_init(&(this->mysql_conn));

#ifdef MYSQL_OPT_RECONNECT
	my_bool reconnect = 1;
	mysql_options(&(this->mysql_conn), MYSQL_OPT_RECONNECT, &reconnect);
#else
#endif
	my_bool compress = 1;
	mysql_options(&(this->mysql_conn), MYSQL_OPT_COMPRESS, &compress);
	if(mysql_real_connect(&(this->mysql_conn), host, user, passwd, dbname, port, NULL, CLIENT_COMPRESS) != NULL)
	{
#ifdef MYSQLENCODE
		if(mysql_set_character_set(&(this->mysql_conn), QUOTE(MYSQLENCODE)) == 0)
#endif
		{
			int l = strlen(host) + 1 + 65 + 1 + (dbname ? strlen(dbname) : 0);
			if(this->ukey = (char *) EMALLOC(l))
			{
				sprintf(this->ukey, "%s_%u_%s", host, (unsigned int) port, (dbname ? dbname : ""));
				this->connok = true;
				//				no need to mysql_select_db since it's done on mysql_real_connect
				//				if(dbname && (mysql_select_db(&(this->mysql_conn), dbname) != 0))
				//				{
				//					// zend_printf("mysql_select_db failed<br>\n");
				//					mysql_close(&(this->mysql_conn));
				//					this->connok = false;
				//				}
			}
		}
	}
}

void *SQLCONN::get_native_conn()
{
	return (&(this->mysql_conn));
}

SQLCONN::~SQLCONN()
{
	if(this->ukey)
		EFREE(this->ukey);
	if(this->connok)
		mysql_close(&(this->mysql_conn));
}

int SQLCONN::escape_string(char *str, int len, char *outbuff)
{
	if(len == -1)
		len = strlen(str);
	if(outbuff == NULL)
		return ((2 * len) + 1); // no buffer allocated : simply return the needed size
	return (mysql_real_escape_string(&(this->mysql_conn), outbuff, str, len));
}

bool SQLCONN::query(char *sql, int len)
{
	if(this->connok)
	{
		if(len == -1)
			len = strlen(sql);
		return ( (mysql_real_query(&(this->mysql_conn), sql, len)) == 0);
	}
	return (false);
}

const char *SQLCONN::error()
{
	if(this->connok)
		return (mysql_error(&(this->mysql_conn)));

	return (NULL);
}

my_ulonglong SQLCONN::affected_rows()
{
	if(this->connok)
		return (mysql_affected_rows(&(this->mysql_conn)));

	return (long) (-1);
}

bool SQLCONN::isok()
{
	return (this->connok);
}

bool SQLRES::query(char *sql)
{
	if(mysql_query(&(this->parent_conn->mysql_conn), sql) == 0)
	{
		if(this->res)
		{
			mysql_free_result(this->res);
			this->res = NULL;
		}
		if(this->res = mysql_store_result(&(this->parent_conn->mysql_conn)))
		{
			// !!!!!!!!!!!!!!!!!!!!! convert from _int64 to int !!!!!!!!!!!!!!!!!
			this->nrows = (int) mysql_num_rows(this->res);
			this->ncols = (int) mysql_num_fields(this->res);
		}
		return (true);
	}
	return (false);
}

int SQLRES::get_nrows()
{
	return (this->nrows);
}

SQLRES::SQLRES(SQLCONN *parent_conn)
{
	this->parent_conn = parent_conn;
	this->res = NULL;
	this->sqlrow.parent_res = this;
	this->ncols = 0;
	this->ncols = 0;
}

SQLRES::~SQLRES()
{
	if(this->res)
		mysql_free_result(this->res);
}

SQLROW *SQLRES::fetch_row()
{
	if(this->parent_conn->connok && this->res)
	{
		if((this->sqlrow.row = mysql_fetch_row(this->res)))
			return (&(this->sqlrow));
	}
	return (NULL);
}

unsigned long *SQLRES::fetch_lengths()
{
	if(this->parent_conn->connok && this->res)
		return (mysql_fetch_lengths(this->res));

	return (NULL);
}

SQLROW::SQLROW()
{
}

SQLROW::~SQLROW()
{
}

char *SQLROW::field(int n)
{
	return (this->row[n]);
}

#define SQLFIELD_RID 0
#define SQLFIELD_CID 1
#define SQLFIELD_SKEY 2
#define SQLFIELD_HITSTART 3
#define SQLFIELD_HITLEN 4
#define SQLFIELD_IW 5
#define SQLFIELD_SHA256 6


// send a 'leaf' query to a databox
// results are returned into the node (qp->n)

void SQLCONN::phrasea_query(char *sql, Cquerytree2Parm *qp, bool reverse)
{
	CHRONO chrono;

	std::pair < std::set<PCANSWER, PCANSWERCOMPRID_DESC>::iterator, bool> insert_ret;

	mysql_thread_init();

	startChrono(chrono);

	// zend_printf("SQL:%s<br/>\n", sql);

	MYSQL_STMT *stmt;
	if((stmt = mysql_stmt_init(&(this->mysql_conn))))
	{
		pthread_mutex_lock(qp->sqlmutex);
		bool mutex_locked = true;

		if(mysql_stmt_prepare(stmt, sql, strlen(sql)) == 0)
		{
			// Execute the SELECT query
			if(mysql_stmt_execute(stmt) == 0)
			{
				qp->n->time_sqlQuery = stopChrono(chrono);

				// Bind the result buffers for all columns before fetching them
				MYSQL_BIND bind[7];
				unsigned long length[7];
				my_bool is_null[7];
				my_bool error[7];
				long int_result[7];
				unsigned char sha256[65];
				char skey[201];

				memset(bind, 0, sizeof (bind));

				// every field is int...
				for(int i = 0; i < 7; i++)
				{
					is_null[i] = true; // so each not fetched column (not listed in sql) will be null

					// INTEGER COLUMN(S)
					bind[i].buffer_type = MYSQL_TYPE_LONG;
					bind[i].buffer = (char *) (&int_result[i]);
					bind[i].is_null = &is_null[i];
					bind[i].length = &length[i];
					bind[i].error = &error[i];
				}
				// ... except :

				// sha256 column : 256 bits
				bind[SQLFIELD_SHA256].buffer_type = MYSQL_TYPE_STRING;
				bind[SQLFIELD_SHA256].buffer_length = 65;
				bind[SQLFIELD_SHA256].buffer = (char *) sha256;

				// skey column : 100 chars utf8
				bind[SQLFIELD_SKEY].buffer_type = MYSQL_TYPE_STRING;
				bind[SQLFIELD_SKEY].buffer_length = 201;
				bind[SQLFIELD_SKEY].buffer = (char *) skey;

				// Bind the result buffers
				if(mysql_stmt_bind_result(stmt, bind) == 0)
				{
					bool ok_to_fetch = true;

					// Now buffer all results to client (optional step)
					startChrono(chrono);
					ok_to_fetch = (mysql_stmt_store_result(stmt) == 0);
					qp->n->time_sqlStore = stopChrono(chrono);

					//					if(mutex_locked)
					//					{
					//						pthread_mutex_unlock(qp->sqlmutex);
					//						mutex_locked = false;
					//					}

					// fetch results
					if(ok_to_fetch)
					{
						long rid;
						long lastrid = -1;

						startChrono(chrono);
						while(mysql_stmt_fetch(stmt) == 0)
						{
							rid = int_result[SQLFIELD_RID];

							CANSWER *answer;
							if((answer = new CANSWER()))
							{
								answer->rid = rid;
								insert_ret = qp->n->answers.insert(answer);
								if(insert_ret.second == false)
								{
									// this rid already exists
									delete answer;
									answer = *(insert_ret.first);
								}
								else
								{
									// qp->n->nbranswers++;

									// a new rid
									answer->cid = int_result[SQLFIELD_CID];

									if(!is_null[SQLFIELD_SHA256])
										answer->sha2 = new CSHA(sha256);

									if(!is_null[SQLFIELD_SKEY])
									{
										skey[length[SQLFIELD_SKEY]] = '\0';
										switch(qp->sortMethod)
										{
											case SORTMETHOD_STR:
												answer->sortkey.s = new std::string(skey);
												break;
											case SORTMETHOD_INT:
												answer->sortkey.l = atoll(skey);
												break;
										}
									}
								}
							}

							if(!is_null[SQLFIELD_IW] && answer)
							{
								CHIT *hit;
								if(hit = new CHIT(int_result[SQLFIELD_IW]))
								{
									if(!(answer->firsthit))
										answer->firsthit = hit;
									if(answer->lasthit)
										answer->lasthit->nexthit = hit;
									answer->lasthit = hit;
								}
							}
							if(!is_null[SQLFIELD_HITSTART] && !is_null[SQLFIELD_HITLEN] && answer)
							{
								CSPOT *spot;
								if(spot = new CSPOT(int_result[SQLFIELD_HITSTART], int_result[SQLFIELD_HITLEN]))
								{
									if(!(answer->firstspot))
										answer->firstspot = spot;
									if(answer->lastspot)
										answer->lastspot->_nextspot = spot;
									answer->lastspot = spot;
								}
							}
						}
						qp->n->time_sqlFetch = stopChrono(chrono);
					}
					else // store error
					{
						zend_printf("ERR:%s<br/>\n", mysql_stmt_error(stmt));
					}
				}
				else // bind error
				{
					zend_printf("ERR:%s<br/>\n", mysql_stmt_error(stmt));
				}
			}
			else // execute error
			{
				zend_printf("ERR:%s<br/>\n", mysql_stmt_error(stmt));
			}
		}
		else // prepare error
		{
			zend_printf("ERR:%s<br/>\n", mysql_stmt_error(stmt));
		}

		mysql_stmt_close(stmt);

		if(mutex_locked)
			pthread_mutex_unlock(qp->sqlmutex);
	}

	mysql_thread_end();
}

