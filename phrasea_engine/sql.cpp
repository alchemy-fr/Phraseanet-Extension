#include "base_header.h"


#define QUOTE(x) _QUOTE(x)
#define _QUOTE(a) #a

void sprintfSQLERR(char **str, const char *format, ...)
{
	int l = 0;
	va_list args;
	va_start(args, format);
	*str = NULL;
	l = vsnprintf(*str, l, format, args);
	if( (*str = (char *)(EMALLOC(l+1))) )
	{
	 	va_start(args, format);
		vsnprintf(*str, l+1, format, args);
	}
	va_end(args);
}


SQLCONN::SQLCONN(const char *host, unsigned int port, const char *user, const char *passwd, const char *dbname)
{
	this->ukey = NULL;
	this->connok = false;
	this->mysql_active_result_id = 0;
//	this->stmt_th = NULL;
//	this->DOMThesaurus = NULL;
//	this->thesaurus_buffer = NULL;
//	this->XPathCtx_thesaurus = NULL;

	strcpy(this->host, host);
	strcpy(this->user, user);
	strcpy(this->passwd, passwd);
	strcpy(this->dbname, dbname);
	this->port = port;

	// this->mysql_conn = NULL;
	int l = strlen(host) + 1 + 65 + 1 + (dbname ? strlen(dbname) : 0);
	if( (this->ukey = (char *) EMALLOC(l)) != NULL)
		sprintf(this->ukey, "%s_%u_%s", host, (unsigned int) port, (dbname ? dbname : ""));
}

SQLCONN::~SQLCONN()
{
//	if(this->ukey)
//		EFREE(this->ukey);
//	if(this->connok)
//		mysql_close(&(this->mysql_conn));
	this->close();
	if(this->ukey)
		EFREE(this->ukey);
//	if(this->thesaurus_buffer)
//		EFREE(this->thesaurus_buffer);
//	if(this->stmt_th)
//		mysql_stmt_close(this->stmt_th);
//	if(this->XPathCtx_thesaurus)
//		xmlXPathFreeContext(this->XPathCtx_thesaurus);
//	if(this->DOMThesaurus)
//		xmlFreeDoc(this->DOMThesaurus);
}

unsigned long SQLCONN::thread_id()
{
	return(mysql_thread_id(&(this->mysql_connx)));
}


bool SQLCONN::connect()
{
	bool ret = false;
	if(!this->connok)
	{
		mysql_init(&(this->mysql_connx));

#ifdef MYSQL_OPT_RECONNECT
		my_bool reconnect = 1;
		mysql_options(&(this->mysql_conn), MYSQL_OPT_RECONNECT, &reconnect);
#else
#endif
		my_bool compress = 1;
		mysql_options(&(this->mysql_connx), MYSQL_OPT_COMPRESS, &compress);

		if(mysql_real_connect(&(this->mysql_connx), this->host, this->user, this->passwd, this->dbname, this->port, NULL, CLIENT_COMPRESS) != NULL)
		{
#ifdef MYSQLENCODE
			if(mysql_set_character_set(&(this->mysql_connx), QUOTE(MYSQLENCODE)) == 0)
#endif
			{
				this->connok = true;
			}
			else
			{
				// mysql_set_character_set failed
				this->close();
			}
		}
		else
		{
			// connect failed
			this->close();
		}
	}
	else
	{
	}
	return(this->connok);
}
/*
bool SQLCONN::isok()
{
	return (this->connect());
}
*/

void SQLCONN::close()
{
	if(this->connok)
		mysql_close(&(this->mysql_connx));
	// this->ukey = NULL;
	this->connok = false;
	// this->mysql_conn = NULL;
}

void *SQLCONN::get_native_conn()
{
	if(this->connect())
		return(&(this->mysql_connx));
	else
		return(NULL);
}

int SQLCONN::escape_string(const char *str, int len, char *outbuff)
{
	int ret = -1;
	if(this->connect())
	{
		if(len == -1)
			len = strlen(str);
		if(outbuff == NULL)
			return ((2 * len) + 1); // no buffer allocated : simply return the needed size
		ret = mysql_real_escape_string(&(this->mysql_connx), outbuff, str, len);
		outbuff[ret] = '\0';
	//	this->close();
	}
	return(ret);
}

bool SQLCONN::query(const char *sql, int len)
{
	bool ret = false;
	if(this->connect())
	{
//	if(this->connok)
//	{
		if(len == -1)
			len = strlen(sql);
		ret = (mysql_real_query(&(this->mysql_connx), sql, len) == 0);
//		this->close();
	}
	return (ret);
}

const char *SQLCONN::error()
{
	if(this->connect())
	{
//	if(this->connok)
		return (mysql_error(&(this->mysql_connx)));
	}
	return (NULL);
}

my_ulonglong SQLCONN::affected_rows()
{
	if(this->connect())
	{
	//if(this->connok)
		return (mysql_affected_rows(&(this->mysql_connx)));
	}
	return (long) (-1);
}

// ============================================================================


SQLRES::SQLRES(SQLCONN *parent_conn)
{
	parent_conn->connect();

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

bool SQLRES::query(const char *sql)
{
	if(mysql_query(&(this->parent_conn->mysql_connx), sql) == 0)
	{
		if(this->res)
		{
			mysql_free_result(this->res);
			this->res = NULL;
		}
		if( (this->res = mysql_store_result(&(this->parent_conn->mysql_connx))) != NULL)
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

// ===============================================================================

SQLROW::SQLROW()
{
}

SQLROW::~SQLROW()
{
}

const char *SQLROW::field(int n, const char *replaceNULL)
{
	return (this->row[n] == NULL ? replaceNULL : this->row[n]);
}

#define SQLFIELD_RID 0
#define SQLFIELD_CID 1
#define SQLFIELD_SKEY 2
#define SQLFIELD_HITSTART 3
#define SQLFIELD_HITLEN 4
#define SQLFIELD_IW 5
#define SQLFIELD_SHA256 6

/*
void SQLCONN::thesaurus_search(const char *term)
{
	if(!this->DOMThesaurus)
	{
		if(!this->stmt_th)
		{
			if( this->stmt_th = mysql_stmt_init((MYSQL *)(this->get_native_conn())) )
			{
				char *sql = (char *)"SELECT value FROM pref WHERE prop='thesaurus'";
				if(mysql_stmt_prepare(this->stmt_th, sql, 45) != 0)
				{
					mysql_stmt_close(this->stmt_th);
					this->stmt_th = NULL;
				}
			}
		}

		if(this->thesaurus_buffer == NULL)
		{
			if( (this->thesaurus_buffer = (char *)(EMALLOC(this->thesaurus_buffer_size = 10000))) == NULL )
			{
				// malloc error
				return;
			}
		}

		if(this->stmt_th)
		{
			MYSQL_BIND bindo;
			my_bool is_null = false;;
			my_bool error = false;
			unsigned long length=0;

			memset(&bindo, 0, sizeof(bindo));
			bindo.buffer_type   = MYSQL_TYPE_STRING;
			bindo.buffer        = (void *)(this->thesaurus_buffer);
			bindo.buffer_length = this->thesaurus_buffer_size;
			bindo.is_null       = &is_null;
			bindo.length        = &length;
			bindo.error         = &error;

			if(mysql_stmt_execute(this->stmt_th) == 0)
			{
				int row_count = 0;

				int ret = 0;

				if(mysql_stmt_bind_result(this->stmt_th, &bindo) == 0)
				{

					ret = mysql_stmt_fetch(this->stmt_th);
#ifdef MYSQL_DATA_TRUNCATED
					if(ret == MYSQL_DATA_TRUNCATED)
						ret = 0;		//  will be catched comparing buffer sizes
#endif
					if(ret==0 && length > this->thesaurus_buffer_size+1)
					{
						// buffer too small, realloc
						EFREE(this->thesaurus_buffer);
						if( (this->thesaurus_buffer = (char *)EMALLOC(this->thesaurus_buffer_size = length+1)) )
						{
							bindo.length        = 0;
							bindo.buffer        = (void *)(this->thesaurus_buffer);
							bindo.buffer_length = this->thesaurus_buffer_size;
							if(mysql_stmt_fetch_column(this->stmt_th, &bindo, 0, 0) != 0)
								ret = -1;
						}
						else
						{
							ret = -1;
						}
					}
					if(ret == 0)
					{
						this->DOMThesaurus = xmlParseMemory(this->thesaurus_buffer, length);
						if(this->DOMThesaurus != NULL)
						{
							// Create xpath evaluation context
							this->XPathCtx_thesaurus = xmlXPathNewContext(this->DOMThesaurus);
						}
					}
				}
			}
		}
	}

	if(this->XPathCtx_thesaurus)
	{
//		"/thesaurus//sy"
//		xmlXPathEvalExpression((const xmlChar*)("/record/description/" "*"), this->XPathCtx_thesaurus);
	}
}
*/


// send a 'leaf' query to a databox
// results are returned into the node (qp->n)

void SQLCONN::phrasea_query(const char *sql, Cquerytree2Parm *qp, char **sqlerr)
{
	qp->sqlconn->connect();
	MYSQL *xconn = (MYSQL *)(qp->sqlconn->get_native_conn());
	CHRONO chrono;

    std::pair <std::multiset<PCANSWER, PCANSWERCOMPRID_DESC>::iterator, bool> insert_ret;

	startChrono(chrono);

	MYSQL_STMT *stmt;
	if((stmt = mysql_stmt_init(xconn)))
	{
//		qp->sqlmutex->lock();
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
					length[i] = 0;
					is_null[i] = true; // so each not fetched column (not listed in sql) will be null
					error[i] = false;
					int_result[i] = 0;
					// INTEGER COLUMN(S)
					bind[i].buffer_type = MYSQL_TYPE_LONG;
					bind[i].buffer = (char *) (&int_result[i]);
					bind[i].is_null = &is_null[i];
					bind[i].length = &length[i];
					bind[i].error = &error[i];
				}
				// ... except :

				// sha256 column : 256 bits
				memset(sha256, 0, sizeof (sha256));
				bind[SQLFIELD_SHA256].buffer_type = MYSQL_TYPE_STRING;
				bind[SQLFIELD_SHA256].buffer_length = 65;
				bind[SQLFIELD_SHA256].buffer = (char *) sha256;

				// skey column : 100 chars utf8
				memset(skey, 0, sizeof (skey));
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
//		qp->n->n = 0;
						while(mysql_stmt_fetch(stmt) == 0)
						{
//		qp->n->n++;

							rid = int_result[SQLFIELD_RID];

							CANSWER *answer;
							std::multiset<PCANSWER, PCANSWERCOMPRID_DESC>::iterator where;
							if((answer = new CANSWER()))
							{
								answer->rid = rid;

//								insert_ret = qp->n->answers.insert(answer);
//								if(insert_ret.second == false)
								where = qp->n->answers.find(answer);
								if(where != qp->n->answers.end())
								{
									// this rid already exists
									delete answer;
									answer = *where;
								}
								else
								{
									qp->n->answers.insert(answer);
									// n->nbranswers++;

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
#ifdef PHP_WIN32
												answer->sortkey.l = _strtoui64(skey, NULL, 10);
#else
												answer->sortkey.l = atoll(skey);
#endif
												break;
										}
									}
								}
							}

							if(!is_null[SQLFIELD_IW] && answer)
							{
								CHIT *hit;
								if( (hit = new CHIT(int_result[SQLFIELD_IW])) != NULL)
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
								if( (spot = new CSPOT(int_result[SQLFIELD_HITSTART], int_result[SQLFIELD_HITLEN])) != NULL)
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
						sprintfSQLERR(sqlerr, "ERR: line %d : %s<br/>\n", __LINE__, mysql_stmt_error(stmt));
					}
				}
				else // bind error
				{
					sprintfSQLERR(sqlerr, "ERR: line %d : %s<br/>\n", __LINE__, mysql_stmt_error(stmt));
				}
			}
			else // execute error
			{
				sprintfSQLERR(sqlerr, "ERR: line %d : %s<br/>\n", __LINE__, mysql_stmt_error(stmt));
			}
		}
		else // prepare error
		{
			sprintfSQLERR(sqlerr, "ERR: line %d : %s\n%s\n", __LINE__, mysql_stmt_error(stmt), sql);
		}

		mysql_stmt_close(stmt);
//		qp->sqlmutex->unlock();
	}
//	mysql_thread_end();
}

// send a 'leaf' query to a databox
// results are returned into the node (qp->n)

