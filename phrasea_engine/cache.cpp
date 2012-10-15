#include "base_header.h"

#include "../php_phrasea2/php_phrasea2.h"


// ---------------------------------------------------------------------------------------------------------------
// CACHE_COLL

#define GETPREFS 1

#define PAD sizeof(char *)
#define LSTRPAD(l) (((l)+PAD) & ~(PAD-1))


CACHE_COLL::CACHE_COLL(long coll_id, long base_id, const char *name, const char *prefs) //, bool registered)
{
  int lstr, lram;
	this->coll_id = coll_id;
	this->base_id = base_id;
	this->name = NULL;
	this->name_lenPAD = 0;
	this->prefs = NULL;
	this->prefs_lenPAD = 0;
//	this->registered = registered;
	this->binsize = sizeof(this->coll_id)			// coll_id
					+ sizeof(this->base_id)			// base_id
//					+ sizeof(this->phserver_port)	// phserver_port
//					+ sizeof(long)					// registered
					+ PAD							// name	(if null)
#if GETPREFS
					+ PAD							// prefs (if null)
#endif
					;
	if(name)
	{
		lstr = strlen(name);
		lram = LSTRPAD(lstr);			// room for the final '\0', rounded to the next multiple of PAD
		if(this->name = (char *)EMALLOC(lram))
		{
			this->name_lenPAD = lram;
			memcpy(this->name, name, lstr+1);
			while(lstr<lram)
				this->name[lstr++] = '\0';	// pad with 0
			this->binsize += lram-PAD;		// PAD was already counted
		}
	}
#if GETPREFS
	if(prefs)
	{
		lstr = strlen(prefs);
		lram = LSTRPAD(lstr);			// room for the final '\0', rounded to the next multiple of PAD
		if(this->prefs = (char *)EMALLOC(lram))
		{
			this->prefs_lenPAD = lram;
			memcpy(this->prefs, prefs, lstr+1);
			while(lstr<lram)
				this->prefs[lstr++] = '\0';	// pad with 0
			this->binsize += lram-PAD;		// PAD was already counted
		}
	}
#endif
	this->nextcoll = NULL;
}

long CACHE_COLL::get_binsize()
{
	return(this->binsize);
}

long *CACHE_COLL::serialize_bin(long *binbuff)
{
	*binbuff++ = this->coll_id;
	*binbuff++ = this->base_id;
//	*binbuff++ = (long)(this->registered);
	if(this->name_lenPAD > 0)
	{
		memcpy(binbuff, this->name, this->name_lenPAD);
		binbuff += (this->name_lenPAD)/sizeof(long);
	}
	else
	{
		*binbuff++ = 0;
	}
#if GETPREFS
	if(this->prefs_lenPAD > 0)
	{
		memcpy(binbuff, this->prefs, this->prefs_lenPAD);
		binbuff += (this->prefs_lenPAD)/sizeof(long);
	}
	else
	{
		*binbuff++ = 0;
	}
#endif
	return(binbuff);
}

void CACHE_COLL::serialize_php(zval *zcolllist)
{
  zval *zcoll;
	MAKE_STD_ZVAL(zcoll);
	array_init(zcoll);
	add_assoc_long(zcoll, (char *)"coll_id", this->coll_id);
	add_assoc_long(zcoll, (char *)"base_id", this->base_id);
	add_assoc_string(zcoll, (char *)"name", this->name, true);
#if GETPREFS
	add_assoc_string(zcoll, (char *)"prefs", this->prefs, true);
#endif
//	add_assoc_bool(zcoll, (char *)"registered", this->registered);
	add_next_index_zval(zcolllist, zcoll);
}

void CACHE_COLL::dump()
{
	zend_printf("|  |  +--coll_id=%li\n", this->coll_id);
	zend_printf("|  |  |  base_id=%li\n", this->base_id);
	zend_printf("|  |  |  name='%s' (binsize=%li)\n", this->name ? this->name : "null", this->name_lenPAD);
#if GETPREFS
	zend_printf("|  |  |  prefs='%s' (binsize=%li)\n", this->prefs ? this->prefs : "null", this->prefs_lenPAD);
#endif
//	zend_printf("|  |  |  registered=%s\n", this->registered ? "TRUE":"FALSE");
	zend_printf("|  |  +--(coll binsize=%li)\n", this->get_binsize());
	zend_printf("|  |\n");
}

CACHE_COLL::~CACHE_COLL()
{
	if(this->name)
	{
		EFREE(this->name);
		this->name = NULL;
	}
#if GETPREFS
	if(this->prefs)
	{
		EFREE(this->prefs);
		this->prefs = NULL;
	}
#endif
	return;
}



// ---------------------------------------------------------------------------------------------------------------
// CACHE_BASE



CACHE_BASE::CACHE_BASE(long base_id, const char *host, long port, const char *user, const char *passwd, const char *dbname, const char *xmlstruct, long sbas_id, const char *viewname) //, bool online)
{
//	this->online = online;
 	this->base_id = base_id;
 	this->sbas_id = sbas_id;
	this->port = port;
//	this->engine = PHRASEA_MYSQLENGINE;
	this->conn = NULL;

	this->viewname = NULL;
	this->viewname_lenPAD = PAD;
	if(viewname)
	{
		long lstr = strlen(viewname);
		long lram = LSTRPAD(lstr);			// room for the final '\0', rounded to the next multiple of PAD
		if(this->viewname = (char *)EMALLOC(lram))
		{
			this->viewname_lenPAD = lram;
			memcpy(this->viewname, viewname, lstr+1);
			while(lstr<lram)
				this->viewname[lstr++] = '\0';	// pad with 0
		}
	}

	this->host = NULL;
	this->host_lenPAD = PAD;
	if(host)
	{
		long lstr = strlen(host);
		long lram = LSTRPAD(lstr);			// room for the final '\0', rounded to the next multiple of PAD
		if(this->host = (char *)EMALLOC(lram))
		{
			this->host_lenPAD = lram;
			memcpy(this->host, host, lstr+1);
			while(lstr<lram)
				this->host[lstr++] = '\0';	// pad with 0
		}
	}

	this->user = NULL;
	this->user_lenPAD = PAD;
	if(user)
	{
		long lstr = strlen(user);
		long lram = LSTRPAD(lstr);			// room for the final '\0', rounded to the next multiple of PAD
		if(this->user = (char *)EMALLOC(lram))
		{
			this->user_lenPAD = lram;
			memcpy(this->user, user, lstr+1);
			while(lstr<lram)
				this->user[lstr++] = '\0';	// pad with 0
		}
	}

	this->passwd = NULL;
	this->passwd_lenPAD = PAD;
	if(passwd)
	{
		long lstr = strlen(passwd);
		long lram = LSTRPAD(lstr);			// room for the final '\0', rounded to the next multiple of PAD
		if(this->passwd = (char *)EMALLOC(lram))
		{
			this->passwd_lenPAD = lram;
			memcpy(this->passwd, passwd, lstr+1);
			while(lstr<lram)
				this->passwd[lstr++] = '\0';	// pad with 0
		}
	}

	this->dbname = NULL;
	this->dbname_lenPAD = PAD;
	if(dbname)
	{
		long lstr = strlen(dbname);
		long lram = LSTRPAD(lstr);			// room for the final '\0', rounded to the next multiple of PAD
		if(this->dbname = (char *)EMALLOC(lram))
		{
			this->dbname_lenPAD = lram;
			memcpy(this->dbname, dbname, lstr+1);
			while(lstr<lram)
				this->dbname[lstr++] = '\0';	// pad with 0
		}
	}

	this->xmlstruct = NULL;
	this->xmlstruct_lenPAD = PAD;
	if(xmlstruct)
	{
		long lstr = strlen(xmlstruct);
		long lram = LSTRPAD(lstr);			// room for the final '\0', rounded to the next multiple of PAD
		if(this->xmlstruct = (char *)EMALLOC(lram))
		{
			this->xmlstruct_lenPAD = lram;
			memcpy(this->xmlstruct, xmlstruct, lstr+1);
			while(lstr<lram)
				this->xmlstruct[lstr++] = '\0';		// pad with 0
		}
	}
	this->binsize = sizeof(this->base_id)			// base_id
					+ sizeof(this->sbas_id)			// sbas_id
//					+ sizeof(this->online)			// online
					+ this->viewname_lenPAD			// viewname
					+ this->host_lenPAD				// host
					+ sizeof(this->port)			// port
					+ this->user_lenPAD				// user
					+ this->passwd_lenPAD			// pwd
//					+ sizeof(this->engine)			// engine
					+ this->dbname_lenPAD			// dbname
					+ this->xmlstruct_lenPAD		// xmlstruct
					+ sizeof(long);					// count of collections
	this->firstcoll = NULL;
	this->nextbase = NULL;
}

long CACHE_BASE::get_local_base_id(long distant_coll_id)
{
  CACHE_COLL *cc;
	for(cc=this->firstcoll; cc && cc->coll_id != distant_coll_id; cc = cc->nextcoll)
		;
	if(cc)
		return(cc->base_id);
	return(-1);
}
/**
 * same as prev, but verifying we are registered
*/
long CACHE_BASE::get_local_base_id2(long distant_coll_id)
{
    CACHE_COLL *cc;
	for(cc=this->firstcoll; cc && cc->coll_id != distant_coll_id; cc = cc->nextcoll)
		;
	if(cc) // && cc->registered)
		return(cc->base_id);
	return(-1);
}

long CACHE_BASE::get_binsize()
{
  long binsize = this->binsize;
  CACHE_COLL *cc;
	for(cc=this->firstcoll; cc ; cc = cc->nextcoll)
		binsize += cc->get_binsize();
	return(binsize);
}

CACHE_COLL *CACHE_BASE::addcoll(long coll_id, long base_id, const char *name, const char *prefs) //, bool registered)
{
  CACHE_COLL *cc, *ncc = new CACHE_COLL(coll_id, base_id, name, prefs); // , registered);
	for(cc=this->firstcoll; cc && cc->nextcoll; cc = cc->nextcoll)
		;
	if(cc)
		return(cc->nextcoll = ncc);
	else
		return(this->firstcoll = ncc);
}


void CACHE_BASE::dump()
{
  CACHE_COLL *cc;
	zend_printf("|  +--base_id=%li\n", this->base_id);
	zend_printf("|  |  sbas_id=%li\n", this->sbas_id);
//	zend_printf("|  |  online=%s\n", this->online ? "TRUE":"FALSE");
	zend_printf("|  |  viewname=%s (binsize=%li)\n", this->viewname, this->viewname_lenPAD);
	zend_printf("|  |  host=%s (binsize=%li)\n", this->host, this->host_lenPAD);
	zend_printf("|  |  port=%li\n", this->port);
	zend_printf("|  |  user=%s (binsize=%li)\n", this->user, this->user_lenPAD);
	zend_printf("|  |  passwd=%s (binsize=%li)\n", this->passwd, this->passwd_lenPAD);
	zend_printf("|  |  dbname=%s (binsize=%li)\n", this->dbname, this->dbname_lenPAD);
	zend_printf("|  |  xmlstruct=%s (binsize=%li)\n", this->xmlstruct, this->xmlstruct_lenPAD);
	zend_printf("|  |  conn=%p\n", this->conn);
	for(cc = this->firstcoll; cc; cc = cc->nextcoll)
		cc->dump();
	zend_printf("|  +--(base binsize=%li)\n", this->get_binsize());
	zend_printf("|\n");
}

void CACHE_BASE::serialize_php(zval *zbaselist) //, bool everything)
{
  CACHE_COLL *cc;
  zval *zbase, *zcolllist;
	MAKE_STD_ZVAL(zbase);
	array_init(zbase);
	add_assoc_long(zbase, (char *)"base_id", this->base_id);
	add_assoc_long(zbase, (char *)"sbas_id", this->sbas_id);
//	add_assoc_bool(zbase, (char *)"online", this->online);
	add_assoc_string(zbase, (char *)"viewname", this->viewname, true);
	add_assoc_string(zbase, (char *)"host", this->host, true);
	add_assoc_long(zbase, (char *)"port", this->port);
	add_assoc_string(zbase, (char *)"user", this->user, true);
	add_assoc_string(zbase, (char *)"passwd", this->passwd, true);
//	add_assoc_long(zbase, (char *)"engine", this->engine);
	add_assoc_string(zbase, (char *)"dbname", this->dbname, true);
	if(this->xmlstruct)
		add_assoc_string(zbase, (char *)"xmlstruct", this->xmlstruct, true);

	MAKE_STD_ZVAL(zcolllist);
	array_init(zcolllist);
	for(cc = this->firstcoll; cc; cc = cc->nextcoll)
	{
//		if(everything || cc->registered)		// do not return collections if we are not registered
			cc->serialize_php(zcolllist);
	}
	add_assoc_zval(zbase, (char *)"collections", zcolllist);
	add_next_index_zval(zbaselist, zbase);
}

long *CACHE_BASE::serialize_bin(long *binbuff)
{
  CACHE_COLL *cc;
  long ncolls = 0;
  long *p;
	*binbuff++ = this->base_id;

	*binbuff++ = this->sbas_id;

//	*binbuff++ = (long)(this->online);

	if(this->viewname)
		memcpy(binbuff, this->viewname, this->viewname_lenPAD);
	else
		*binbuff = 0x00000000;
	binbuff += this->viewname_lenPAD / sizeof(long);

	if(this->host)
		memcpy(binbuff, this->host, this->host_lenPAD);
	else
		*binbuff = 0x00000000;
	binbuff += this->host_lenPAD / sizeof(long);

	*binbuff++ = this->port;

	if(this->user)
		memcpy(binbuff, this->user, this->user_lenPAD);
	else
		*binbuff = 0x00000000;
	binbuff += this->user_lenPAD / sizeof(long);

	if(this->passwd)
		memcpy(binbuff, this->passwd, this->passwd_lenPAD);
	else
		*binbuff = 0x00000000;
	binbuff += this->passwd_lenPAD / sizeof(long);

//	*binbuff++ = this->engine;

	if(this->dbname)
		memcpy(binbuff, this->dbname, this->dbname_lenPAD);
	else
		*binbuff = 0x00000000;
	binbuff += this->dbname_lenPAD / sizeof(long);

	if(this->xmlstruct)
		memcpy(binbuff, this->xmlstruct, this->xmlstruct_lenPAD);
	else
		*binbuff = 0x00000000;
	binbuff += this->xmlstruct_lenPAD / sizeof(long);

	binbuff[0] = 0;
	p = binbuff+1;
	for(cc = this->firstcoll; cc; cc = cc->nextcoll)
	{
		binbuff[0]++;
		p = cc->serialize_bin(p);
	}
	return(p);
}

CACHE_BASE::~CACHE_BASE()
{
	CACHE_COLL *cc;
	if(this->viewname)
		EFREE(this->viewname);
	if(this->host)
		EFREE(this->host);
	if(this->dbname)
		EFREE(this->dbname);
	if(this->user)
		EFREE(this->user);
	if(this->passwd)
		EFREE(this->passwd);
	if(this->xmlstruct)
		EFREE(this->xmlstruct);
	if(this->conn)
		delete(this->conn);
	while(this->firstcoll)
	{
		cc = this->firstcoll->nextcoll;
		delete this->firstcoll;
		this->firstcoll = cc;
	}
}



// ---------------------------------------------------------------------------------------------------------------
// CACHE_SESSION



CACHE_SESSION::CACHE_SESSION(long session_id, SQLCONN *epublisher)
{
	this->session_id = session_id;
	this->epublisher = epublisher;
	this->firstbase = NULL;
}

bool CACHE_SESSION::restore(long session_id)
{
  // TSRMLS_FETCH();
  char sql[256];
  bool ret = false;
	sprintf(sql, "SELECT session FROM cache WHERE session_id=%li", session_id);
	SQLRES res(this->epublisher);
	if(res.query(sql))
	{
		SQLROW *row = res.fetch_row();
		if(row && row->field(0, NULL))
		{
			unsigned long *lengths = res.fetch_lengths();
			if(lengths[0] > 0)
			{
				this->unserialize_bin(row->field(0, NULL));
				ret = true;
			}
		}
	}
	return(ret);
}


bool CACHE_SESSION::save()
{
  bool ret = false;
  MYSQL_STMT   *stmt;
  MYSQL_BIND   bind[2];
  char *query = (char *)("UPDATE cache SET session=? WHERE session_id=?");
  char *binbuff;
  unsigned long binlen;
  int session_id = this->session_id;
  MYSQL *mysql = (MYSQL *)epublisher->get_native_conn();

	if( (stmt = mysql_stmt_init(mysql)) )
	{
		if( (mysql_stmt_prepare(stmt, query, strlen(query))) == 0 )
		{
			binlen = this->get_binsize();		// get size needed
			if(binbuff = (char *)EMALLOC(binlen))
			{
				memset(binbuff, 0, binlen);
				binlen = this->serialize_bin((long *)binbuff);		// serialize and get the real size

				memset(bind, 0, sizeof(bind));

				bind[0].buffer_type = MYSQL_TYPE_VAR_STRING;
				bind[0].buffer = binbuff;
				bind[0].buffer_length = binlen;
				bind[0].is_null= 0;
				bind[0].length= &binlen;

				bind[1].buffer_type= MYSQL_TYPE_LONG;
				bind[1].buffer= (char *)&session_id;
				bind[1].is_null= 0;
				bind[1].length= 0;

				if (mysql_stmt_bind_param(stmt, bind) == 0)
				{
					if (mysql_stmt_execute(stmt) == 0)
					{
						if(mysql_stmt_affected_rows(stmt) == 1)
						{
							ret = true;
						}
					}
				}
				EFREE(binbuff);
			}
		}
		mysql_stmt_close(stmt);
	}
	return(ret);
}


long CACHE_SESSION::get_binsize()
{
  long binsize = sizeof(this->session_id) + sizeof(long);
  CACHE_BASE *cb;
	for(cb=this->firstbase; cb; cb = cb->nextbase)
		binsize += cb->get_binsize();
	return(binsize);
}

SQLCONN *CACHE_SESSION::connect(long base_id)
{
  CACHE_BASE *cb;
  CACHE_COLL *cc;
  SQLCONN *conn = NULL;
	for(cb=this->firstbase; cb; cb = cb->nextbase)
	{
		for(cc=cb->firstcoll; cc && cc->base_id != base_id; cc=cc->nextcoll)
			;
		if(cc)
			break;
	}
	if(cb)
	{
		if(cb->conn)
		{
// ftrace("LINE %d, conn=%p\n", __LINE__, cb->conn);
			return(cb->conn);
		}
// ftrace("LINE %d, dbname=%s\n", __LINE__, cb->dbname);
		return(cb->conn = new SQLCONN(cb->host, cb->port, cb->user, cb->passwd, cb->dbname));
	}
	return(NULL);
}

long CACHE_SESSION::get_session_id()
{
	return(this->session_id);
}

long CACHE_SESSION::get_local_base_id(long local_base_id, long distant_coll_id)
{
  CACHE_BASE *cb;
	for(cb=this->firstbase; cb && cb->base_id != local_base_id; cb = cb->nextbase)
		;
	if(cb)
		return(cb->get_local_base_id(distant_coll_id));
	return(-1);
}
/*
 * same as prev, except that local_base_id can be a daughter coll_id and not only the 'main' coll_id
*/
long CACHE_SESSION::get_local_base_id2(long local_base_id, long distant_coll_id)
{
    CACHE_BASE *cb;
    CACHE_COLL *cc;
    for(cb=this->firstbase; cb; cb = cb->nextbase)
	{
		for(cc=cb->firstcoll; cc && cc->base_id != local_base_id; cc=cc->nextcoll)
			;
		if(cc)
			break;
	}
	if(cb)
		return(cb->get_local_base_id2(distant_coll_id));
	return(-1);
}

long CACHE_SESSION::get_distant_coll_id(long local_base_id)
{
  CACHE_BASE *cb;
  CACHE_COLL *cc;
	for(cb=this->firstbase; cb; cb = cb->nextbase)
	{
		for(cc=cb->firstcoll; cc && cc->base_id != local_base_id; cc = cc->nextcoll)
			;
		if(cc)
			return(cc->coll_id);
	}
	return(-1);
}
/*
void CACHE_SESSION::set_registered(long local_base_id, bool registered)
{
  CACHE_BASE *cb;
  CACHE_COLL *cc;
	for(cb=this->firstbase; cb; cb = cb->nextbase)
	{
		for(cc=cb->firstcoll; cc && cc->base_id != local_base_id; cc = cc->nextcoll)
			;
		if(cc)
			cc->registered = registered;
	}
}
*/
CACHE_BASE *CACHE_SESSION::addbase(long base_id, const char *host, long port, const char *user, const char *passwd, const char *dbname, const char *xmlstruct, long sbas_id, const char *viewname) //, bool online)
{
  CACHE_BASE *cb, *ncb = new CACHE_BASE(base_id, host, port, user, passwd, dbname, xmlstruct, sbas_id, viewname); //, online);
	for(cb=this->firstbase; cb && cb->nextbase; cb = cb->nextbase)
		;
	if(cb)
		return(cb->nextbase = ncb);
	else
		return(this->firstbase = ncb);
}

void CACHE_SESSION::dump()
{
  CACHE_BASE *cb;
	zend_printf("<pre>+--session_id=%li\n", this->session_id);
	for(cb = this->firstbase; cb; cb = cb->nextbase)
		cb->dump();
	zend_printf("+-- (session binsize=%li)\n", this->get_binsize());
	zend_printf("</pre>\n");
}

void CACHE_SESSION::serialize_php(zval *result) //, bool everything)
{
  CACHE_BASE *cb;
//  CACHE_COLL *cc;
//  bool basereg;
  zval *zbaselist;
	array_init(result);
	add_assoc_long(result, (char *)"session_id", this->session_id);
	MAKE_STD_ZVAL(zbaselist);
	array_init(zbaselist);
	for(cb = this->firstbase; cb; cb = cb->nextbase)
	{
//		basereg = everything;
//		for(cc=cb->firstcoll; !basereg && cc; cc=cc->nextcoll)
//			basereg = cc->registered;
//
//		if(everything || (cb->online && basereg))		// don't return offline bases
			cb->serialize_php(zbaselist); // , everything);
	}
	add_assoc_zval(result, (char *)"bases", zbaselist);
}

int CACHE_SESSION::serialize_bin(long *binbuff = NULL)
{
  CACHE_BASE *cb;
  unsigned int l = this->get_binsize();
  long nbases = 0;
  long *p;
	if(binbuff == NULL)
		return(l);			// just return the needed size
	binbuff[0] = this->session_id;
	binbuff[1] = 0;
	p = binbuff+2;
	for(cb = this->firstbase; cb; cb = cb->nextbase)
	{
		binbuff[1]++;
		p = cb->serialize_bin(p);
	}
	return(l);
}

void CACHE_SESSION::unserialize_bin(const char *bin)
{
	unsigned long *p = (unsigned long *)bin;
	this->session_id = p[0];
	unsigned long nbases = p[1];
	p += 2;
	CACHE_BASE *cb;
	while(nbases--)
	{
		long base_id = *p++;
		long sbas_id = *p++;
//		bool online = *p++ ? true : false;
		char *viewname = (char *)p;
		p += LSTRPAD(strlen(viewname)) / sizeof(long);
		char *host = (char *)p;
		p += LSTRPAD(strlen(host)) / sizeof(long);
		long port = *p++;
		char *user = (char *)p;
		p += LSTRPAD(strlen(user)) / sizeof(long);
		char *passwd = (char *)p;
		p += LSTRPAD(strlen(passwd)) / sizeof(long);
//		long engine = *p++;
		char *dbname = (char *)p;
		p += LSTRPAD(strlen(dbname)) / sizeof(long);
		char *xmlstruct = (char *)p;
		p += LSTRPAD(strlen(xmlstruct)) / sizeof(long);
		unsigned long ncoll = *p++;

		cb = this->addbase(base_id, host, port, user, passwd, dbname, xmlstruct, sbas_id, viewname); // , online);

		while(ncoll--)
		{
		  int l;
			long coll_id = *p++;

			long base_id = *p++;

//			long registered = *p++;

			char *name = (char *)p;
			l = LSTRPAD(strlen(name));
			p += l/sizeof(long);
#if GETPREFS

			char *prefs = (char *)p;
			l = LSTRPAD(strlen(prefs));
			p += l/sizeof(long);
#endif
			cb->addcoll(coll_id, base_id, name, prefs); // , registered ? true : false);

		}
	}
}

CACHE_SESSION::~CACHE_SESSION()
{
  CACHE_BASE *cb;
	while(this->firstbase)
	{
		cb = this->firstbase->nextbase;
		delete this->firstbase;
		this->firstbase = cb;
	}
	return;
}

