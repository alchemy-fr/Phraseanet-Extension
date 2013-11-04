#include "base_header.h"


#include "../php_phrasea2/php_phrasea2.h"
#include "thread.h"
#include "sql.h"


CNODE *qtree2tree(zval **root, int depth); // in qtree.cpp
void freetree(CNODE *n); // in qtree.cpp
THREAD_ENTRYPOINT querytree2(void *_qp); // in qtree.cpp
THREAD_ENTRYPOINT querytree_public(void *_qp); // in qtree.cpp

bool pcanswercomp_rid_desc(PCANSWER lhs, PCANSWER rhs);
bool pcanswercomp_int_asc(PCANSWER lhs, PCANSWER rhs);
bool pcanswercomp_int_desc(PCANSWER lhs, PCANSWER rhs);
bool pcanswercomp_str_asc(PCANSWER lhs, PCANSWER rhs);
bool pcanswercomp_str_desc(PCANSWER lhs, PCANSWER rhs);
bool pcanswercomp_sha(PCANSWER lhs, PCANSWER rhs);

typedef struct {
					// in
					zval *zcolllist;
					long sbasid;
					zval *zquery;
					SQLRES *res;
					long multidocMode;
					zval *zqarray;
					char *zsortfield;
					int zsortfieldlen;
					char *zsrchlng;
					int zsrchlnglen;
					int sortmethod;
					char *sqlbusiness_c;
					int sortorder;
					// out
					CNODE *query;
					std::multiset<PCANSWER, bool(*)(PCANSWER, PCANSWER)> *ret;
} PHRASEA_QUERY_PARM;

void phrasea_public_query_c(PHRASEA_QUERY_PARM *qp3_parm);




ZEND_FUNCTION(phrasea_highlight)
{
	CHRONO time_phpfct;
	startChrono(time_phpfct);

	zval *zqarray;
	long zsbid, zrid;
	long zdebug = 0;

	char *zsrchlng = NULL;
	int zsrchlnglen;

// zend_printf("%s[%d] \n", __FILE__, __LINE__);
	switch(ZEND_NUM_ARGS())
	{
		case 4: // sbid, rid, qarray, lng
			if(zend_parse_parameters(4 TSRMLS_CC, (char *) "llas", &zsbid, &zrid, &zqarray, &zsrchlng, &zsrchlnglen ) == FAILURE)
			{
				RETURN_FALSE;
			}
			break;
		case 5: // sbid, rid, qarray, lng, debug
			if(zend_parse_parameters(5 TSRMLS_CC, (char *) "llasl", &zsbid, &zrid, &zqarray, &zsrchlng, &zsrchlnglen, &zdebug ) == FAILURE)
			{
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;	// returns !
			break;
	}

	if(Z_TYPE_P(zqarray) != IS_ARRAY)
		RETURN_FALSE;


// zend_printf("%s[%d] \n", __FILE__, __LINE__);

	SQLCONN *epublisher = PHRASEA2_G(epublisher);
	if(!epublisher)
	{
		// we need conn !
		//		zend_throw_exception(spl_ce_LogicException, "No connection set (check that phrasea_conn(...) returned true).", 0 TSRMLS_CC);
		RETURN_FALSE;
	}
// zend_printf("%s[%d] \n", __FILE__, __LINE__);



	array_init(return_value);

	zval *zqueries = NULL;
	if(zdebug)
	{
		MAKE_STD_ZVAL(zqueries);
		array_init(zqueries);
	}

	zval *zanswers;
	MAKE_STD_ZVAL(zanswers);
	array_init(zanswers);


// zend_printf("%s[%d] \n", __FILE__, __LINE__);
	SQLRES res(epublisher);
// zend_printf("%s[%d] \n", __FILE__, __LINE__);

// zend_printf("%s[%d] \n", __FILE__, __LINE__);

	// get the adresse of the distant database
	std::stringstream sql;
	// SECURITY : sbasid is type long, no risk of injection
	sql << "SELECT sbas_id, host, port, sqlengine, dbname, user, pwd FROM sbas WHERE sbas_id=" << zsbid;
	if(zdebug)
		add_assoc_string(return_value, (char *) "sql_sbas", ((char *) (sql.str().c_str())), true);
	if(res.query((char *) (sql.str().c_str())))
	{
// zend_printf("%s[%d] \n", __FILE__, __LINE__);
		SQLROW *row = res.fetch_row();
		if(row)
		{
// zend_printf("%s[%d] \n", __FILE__, __LINE__);
			// on se connecte sur la base distante
			SQLCONN *conn = new SQLCONN(row->field(1, "127.0.0.1"), atoi(row->field(2, "3306")), row->field(5, "root"), row->field(6, ""), row->field(4, "dbox"));
			if(conn && conn->connect())
			{
// zend_printf("%s[%d] \n", __FILE__, __LINE__);


				struct sb_stemmer *stemmer = NULL;
				// SECURITY : escape search_lng
				char srch_lng[33];
				char srch_lng_esc[65];

				srch_lng[0] = srch_lng_esc[0] = '\0';
				if(zsrchlng != NULL)
				{
					if(zsrchlnglen > 32)
						zsrchlnglen = 32;
					memcpy((void*)srch_lng, (void*)zsrchlng, zsrchlnglen);
					srch_lng[zsrchlnglen] = '\0';
					srch_lng_esc[conn->escape_string(srch_lng, zsrchlnglen, srch_lng_esc)] = '\0';

					stemmer = sb_stemmer_new(srch_lng, "UTF_8");
				}

				// change the php query to a tree of nodes
				CNODE *query;
				query = qtree2tree(&zqarray, 0);

				CMutex sqlmutex;
				char *zsortfield = NULL;
				char **pzsortfield = &zsortfield;

				Cquerytree2Parm qp(
									query,
									'M',
									0,
									conn,

									(char *)row->field(1, "127.0.0.1"),	// host
									atoi(row->field(2, "3306")),	// port
									(char *)row->field(5, "root"),			// user
									(char *)row->field(6, ""),				// pwd
									(char *)row->field(4, "dbox"),			// base
									NULL,						// tmptable

									NULL,		// sqltmp

									&sqlmutex,
									zqueries,
									"record",
									/*t_collmask,*/
									pzsortfield,				// sortfield,
									0,							// sortmethod,
									"",							// sqlbusiness_c,
									stemmer,						// stemmer
									srch_lng_esc,						// srch_lng_esc
									zrid			// new : only this rid
						);
// zend_printf("%s[%d] \n", __FILE__, __LINE__);

				querytree2((void *) &qp);

				CANSWER *answer;
				CSPOT *spot;
				int n_answers = 0, n_spots = 0;
				std::multiset<PCANSWER, PCANSWERCOMPRID_DESC>::iterator it;
				for(it = query->answers.begin(); it != query->answers.end(); it++)
				{
					answer = *(it);
					n_answers++;

					zval *zanswer;
					MAKE_STD_ZVAL(zanswer);
					array_init(zanswer);

					zval *zspots;
					MAKE_STD_ZVAL(zspots);
					array_init(zspots);

					answer->nspots = 0;
					for(spot = answer->firstspot; spot; spot = spot->_nextspot)
					{
						n_spots++;
						answer->nspots++;

						zval *zspot;
						MAKE_STD_ZVAL(zspot);
						array_init(zspot);

						add_assoc_long(zspot, (char *) "start", spot->start);
						add_assoc_long(zspot, (char *) "len", spot->len);

						add_next_index_zval(zspots, zspot);
					}

					add_assoc_long(zanswer, (char *) "sbid", zsbid);
					add_assoc_long(zanswer, (char *) "rid", answer->rid);
					add_assoc_zval(zanswer, (char *) "spots", zspots);

					add_next_index_zval(zanswers, zanswer);
				}

				if(zqueries)
					add_assoc_zval(return_value, (char *) "queries", zqueries);
				add_assoc_zval(return_value, (char *) "results", zanswers);

				// free the tree
				freetree(query);
			}
			if(conn)
				delete conn;
		}
	}

	if(zdebug)
		add_assoc_double(return_value, (char *) "time_phpfct", stopChrono(time_phpfct));
}


ZEND_FUNCTION(phrasea_public_query)
{
	CHRONO time_phpfct;
	startChrono(time_phpfct);

	zval *zbarray;
	HashTable *barray_hash;
	HashPosition bpointer;

	long multidocMode = PHRASEA_MULTIDOC_DOCONLY;
	long zdebug = 0;

	char *zsortfield = NULL;
	int zsortfieldlen;

	zval *zbusiness;

	int sortorder = 0; // no sort
	int sortmethod = SORTMETHOD_STR;

	char *sqlbusiness_c = NULL;

	char *zsrchlng = NULL;
	int zsrchlnglen;

	long record_offset, record_count;

	switch(ZEND_NUM_ARGS())
	{
		case 7: // barray, multidocMode, sortfield, search_business, srch_lng
			if(zend_parse_parameters(7 TSRMLS_CC, (char *) "alsasll", &zbarray, &multidocMode, &zsortfield, &zsortfieldlen, &zbusiness, &zsrchlng, &zsrchlnglen, &record_offset, &record_count) == FAILURE)
			{
				RETURN_FALSE;
			}
			break;
		case 8: // barray, multidocMode, sortfield, search_business, srch_lng, debug
			if(zend_parse_parameters(8 TSRMLS_CC, (char *) "alsaslll", &zbarray, &multidocMode, &zsortfield, &zsortfieldlen, &zbusiness, &zsrchlng, &zsrchlnglen, &record_offset, &record_count, &zdebug) == FAILURE)
			{
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;	// returns !
			break;
	}

	if(record_offset < 0 || record_count < 0)
	{
		RETURN_FALSE;
	}

	if(zsortfieldlen > 0)
	{
		sortorder = 1; // default : desc
		if(zsortfield[0] == '+')
		{
			zsortfield++;
			zsortfieldlen--;
			sortorder = -1;
		}
		else if(zsortfield[0] == '-')
		{
			zsortfield++;
			zsortfieldlen--;
		}
	}
	if(zsortfieldlen > 0)
	{
		sortmethod = SORTMETHOD_STR; // default : desc
		if(zsortfield[0] == '0')
		{
			zsortfield++;
			zsortfieldlen--;
			sortmethod = SORTMETHOD_INT;
		}
	}
	if(zsortfieldlen == 0)
		zsortfield = NULL;

	std::stringstream sqlbusiness_strm;
	sqlbusiness_strm << "OR FIND_IN_SET(record.coll_id, '";
	zval **tmp1;
	bool first = true;
	int n = 0;

	for(int i=0; true; i++)
	{
		if(zend_hash_index_find(HASH_OF(zbusiness), i, (void **) &tmp1) == SUCCESS)
		{
			if(Z_TYPE_PP(tmp1) == IS_LONG)
			{
				if(!first)
					sqlbusiness_strm << ",";
				sqlbusiness_strm << Z_LVAL_P(*tmp1);
				first = false;
				n++;
			}
		}
		else
			break;
	}
	if(n > 0)
	{
		sqlbusiness_strm << "')";
		std::string sqlbusiness_str = sqlbusiness_strm.str();
		if(sqlbusiness_c = (char *)EMALLOC(sqlbusiness_str.length()+1))
			memcpy(sqlbusiness_c, sqlbusiness_str.c_str(), sqlbusiness_str.length()+1);
	}
	else
	{
		if(sqlbusiness_c = (char *)EMALLOC(1))
			sqlbusiness_c[0] = '\0';
	}

	SQLCONN *epublisher = PHRASEA2_G(epublisher);
	if(!epublisher)
	{
		// we need conn !
		//		zend_throw_exception(spl_ce_LogicException, "No connection set (check that phrasea_conn(...) returned true).", 0 TSRMLS_CC);
		RETURN_FALSE;
	}


	array_init(return_value);

	zval *zqueries = NULL;
	if(zdebug)
	{
		MAKE_STD_ZVAL(zqueries);
		array_init(zqueries);
	}

	zval *zanswers;
	MAKE_STD_ZVAL(zanswers);
	array_init(zanswers);


	SQLRES res(epublisher);


	barray_hash = Z_ARRVAL_P(zbarray);
	zval **zbase;
	for(zend_hash_internal_pointer_reset_ex(barray_hash, &bpointer);
		zend_hash_get_current_data_ex(barray_hash, (void**) &zbase, &bpointer) == SUCCESS;
		zend_hash_move_forward_ex(barray_hash, &bpointer))
	{

// zend_printf("%s[%d] \n", __FILE__, __LINE__);
		if(Z_TYPE_PP(zbase) != IS_ARRAY)
			continue;

// zend_printf("%s[%d] \n", __FILE__, __LINE__);
		zval **zsbid;
		if(zend_hash_find(Z_ARRVAL_PP(zbase), "sbas_id", sizeof("sbas_id"), (void **)&zsbid) != SUCCESS || Z_TYPE_PP(zsbid) != IS_LONG)
			continue;

// zend_printf("%s[%d] \n", __FILE__, __LINE__);
		zval **zcolllist;
		if(zend_hash_find(Z_ARRVAL_PP(zbase), "searchcoll", sizeof("searchcoll"), (void **)&zcolllist) != SUCCESS || Z_TYPE_PP(zcolllist) != IS_ARRAY)
			continue;

// zend_printf("%s[%d] \n", __FILE__, __LINE__);
		zval **zqarray;
		if(zend_hash_find(Z_ARRVAL_PP(zbase), "arrayq", sizeof("arrayq"), (void **)&zqarray) != SUCCESS || Z_TYPE_PP(zqarray) != IS_ARRAY)
			continue;

// zend_printf("%s[%d] zbid=%ld \n", __FILE__, __LINE__, Z_LVAL_PP(zsbid) );


		zval *zquery = NULL;
		if(zdebug)
		{
			MAKE_STD_ZVAL(zquery);
			array_init(zquery);
		}


		PHRASEA_QUERY_PARM phrasea_query_parm = {
					// in
					*zcolllist,
					Z_LVAL_PP(zsbid),
					zquery,
					&res,
					multidocMode,
					*zqarray,
					zsortfield,
					zsortfieldlen,
 					zsrchlng,
					zsrchlnglen,
					sortmethod,
					sqlbusiness_c,
					sortorder,
					// out
					NULL,
					NULL
		};

// zend_printf("%s[%d] \n", __FILE__, __LINE__);

		CHRONO time_sbas;
		startChrono(time_sbas);

		phrasea_public_query_c(&phrasea_query_parm);

		size_t n = phrasea_query_parm.query->answers.size();

// zend_printf("%s[%d] qp3_parm.query->answers.size() = %ld \n", __FILE__, __LINE__, n);

		if(n < record_offset)
		{
// zend_printf("%s[%d] full sbas (n=%ld) ignored, record_offset=%ld -> %ld \n", __FILE__, __LINE__, n, record_offset, record_offset - n);
			record_offset -= n;
		}
		else
		{
			CANSWER *answer;
			// dump the sorted multiset to cache
#ifdef WIN32
			std::multiset<PCANSWER, bool(__cdecl *)(PCANSWER,PCANSWER)>::iterator ipmset;
#else
			std::multiset<PCANSWER, bool>::iterator ipmset;
#endif
			for(ipmset = phrasea_query_parm.ret->begin(); record_count>0 && ipmset != phrasea_query_parm.ret->end(); ipmset++)
			{
				answer = *(ipmset);
				if(record_offset > 0)
				{
// zend_printf("%s[%d] sbid=%ld, cid=%ld, bid=%ld, rid=%ld (offset=%ld -> ignored) \n", __FILE__, __LINE__, qp3_parm.sbasid, answer->cid, answer->bid, answer->rid, record_offset );
					record_offset--;
				}
				else
				{
// zend_printf("%s[%d] sbid=%ld, cid=%ld, bid=%ld, rid=%ld (result %ld) \n", __FILE__, __LINE__, qp3_parm.sbasid, answer->cid, answer->bid, answer->rid, record_count );
					zval *zanswer;
					MAKE_STD_ZVAL(zanswer);
					array_init(zanswer);

					add_assoc_long(zanswer, (char *) "sbid", phrasea_query_parm.sbasid);
					//add_assoc_long(zanswer, (char *) "cid", answer->cid);
					//add_assoc_long(zanswer, (char *) "bid", answer->bid);
					add_assoc_long(zanswer, (char *) "rid", answer->rid);

					add_next_index_zval(zanswers, zanswer);

					record_count--;
				}
			}
		}

		freetree(phrasea_query_parm.query);

		if(zquery && zqueries)
		{
			add_assoc_double(zquery, (char *) "time_all", stopChrono(time_sbas));
			add_next_index_zval(zqueries, zquery);
		}

		if(record_count <= 0)
		{
// zend_printf("%s[%d] count done \n", __FILE__, __LINE__ );
			break;
		}
	}

	if(zqueries)
		add_assoc_zval(return_value, (char *) "queries", zqueries);

	add_assoc_zval(return_value, (char *) "results", zanswers);

// zend_printf("%s[%d] \n", __FILE__, __LINE__);

	if(sqlbusiness_c)
		EFREE(sqlbusiness_c);

	if(zdebug)
		add_assoc_double(return_value, (char *) "time_phpfct", stopChrono(time_phpfct));
}



void phrasea_public_query_c(PHRASEA_QUERY_PARM *qp3_parm)
{
	TSRMLS_FETCH();

	long session = 0L;

	std::map<long, long> t_collid;			// key : distant_coll_id (dbox side) ==> value : local_base_id (appbox side)

	// replace list of 'local' collections  (base_ids) by list of 'distant' collections (coll ids)
	if(Z_TYPE_PP(&(qp3_parm->zcolllist)) == IS_ARRAY)
	{
// zend_printf("%s[%d] \n", __FILE__, __LINE__);
		// fill the distant_coll_id to local_base_id map
		for(int i = 0; true; i++)
		{
			zval **tmp1;
			if(zend_hash_index_find(HASH_OF(qp3_parm->zcolllist), i, (void **) &tmp1) == SUCCESS)
			{
				if(Z_TYPE_PP(tmp1) == IS_LONG)
				{
					long distant_coll_id = PHRASEA2_G(global_session)->get_distant_coll_id(Z_LVAL_P(*tmp1));
					if(distant_coll_id != -1)
					{
						t_collid[distant_coll_id] = Z_LVAL_P(*tmp1);
					}
				}
			}
			else
				break;
		}

		if(t_collid.size() > 0)
		{
// zend_printf("%s[%d] \n", __FILE__, __LINE__);
			CHRONO time_connect;
			startChrono(time_connect);

			// get the adresse of the distant database
			std::stringstream sql;
			// SECURITY : sbasid is type long, no risk of injection
			sql << "SELECT sbas_id, host, port, sqlengine, dbname, user, pwd FROM sbas WHERE sbas_id=" << qp3_parm->sbasid;

			if(qp3_parm->zquery)
				add_assoc_string(qp3_parm->zquery, (char *) "sql_sbas", ((char *) (sql.str().c_str())), true);

			if(qp3_parm->res->query((char *) (sql.str().c_str())))
			{
// zend_printf("%s[%d] \n", __FILE__, __LINE__);
				SQLROW *row = qp3_parm->res->fetch_row();
				if(row)
				{
					if(qp3_parm->zquery)
						add_assoc_double(qp3_parm->zquery, (char *) "time_select", stopChrono(time_connect));

					startChrono(time_connect);
// zend_printf("%s[%d] \n", __FILE__, __LINE__);
					// on se connecte sur la base distante
					SQLCONN *conn = new SQLCONN(row->field(1, "127.0.0.1"), atoi(row->field(2, "3306")), row->field(5, "root"), row->field(6, ""), row->field(4, "dbox"));
					if(conn && conn->connect())
					{
						if(qp3_parm->zquery)
							add_assoc_double(qp3_parm->zquery, (char *) "time_connect", stopChrono(time_connect));

						CHRONO chrono;
						startChrono(chrono);

						char lbuff[40];

						std::map<long, long>::iterator it_coll;
						int i;
						std::string sqltmp;
						sqltmp = "INSERT INTO `_tmpmask` (coll_id) VALUES ";
						for(i=0, it_coll = t_collid.begin(); it_coll != t_collid.end(); it_coll++, i++)
						{
							if(i>0)
								sqltmp += ',';
							sprintf(lbuff, "(%lu)", it_coll->first);
							sqltmp += lbuff;
						}

						// small sql that joins record and collusr
						char *sqltrec = (char *)"(record)";

						if(qp3_parm->multidocMode == PHRASEA_MULTIDOC_ALL)
						{
							sqltrec = (char *)"(record INNER JOIN _tmpmask ON _tmpmask.coll_id=record.coll_id)";
							// sprintf(sqltrec, "(record INNER JOIN %s ON %s.coll_id=record.coll_id)", tmptable, tmptable);
						}
						else
						{
							if(qp3_parm->multidocMode == PHRASEA_MULTIDOC_DOCONLY)
							{
								sqltrec = (char *)"(record INNER JOIN _tmpmask ON _tmpmask.coll_id=record.coll_id AND parent_record_id=0)";
								// sprintf(sqltrec, "(record INNER JOIN %s ON %s.coll_id=record.coll_id AND parent_record_id=0)", tmptable, tmptable);
							}
							else
							{
								sqltrec = (char *)"(record INNER JOIN _tmpmask ON _tmpmask.coll_id=record.coll_id AND parent_record_id=1)";
								// sprintf(sqltrec, "(record INNER JOIN %s ON %s.coll_id=record.coll_id AND parent_record_id=1)", tmptable, tmptable);
							}
						}

						// change the php query to a tree of nodes
						qp3_parm->query = qtree2tree(&(qp3_parm->zqarray), 0);

						char **pzsortfield; // pass as ptr because querytree2 may reset it to null during exec of 'sha256=sha256'
						// SECURITY : escape zsortfield
						if(qp3_parm->zsortfield != NULL)
						{
							if(qp3_parm->zsortfieldlen > 32)
								qp3_parm->zsortfieldlen = 32;
							char zsortfield_esc[65];
							zsortfield_esc[conn->escape_string(qp3_parm->zsortfield, qp3_parm->zsortfieldlen, zsortfield_esc)] = '\0';

							char *psortfield_esc = zsortfield_esc;
							pzsortfield = &psortfield_esc; // pass as ptr because querytree2 may reset it to null during exec of 'sha256=sha256'
						}
						else
						{
							pzsortfield = &(qp3_parm->zsortfield); // pass as ptr because querytree2 may reset it to null during exec of 'sha256=sha256'
						}

						struct sb_stemmer *stemmer = NULL;
						// SECURITY : escape search_lng
						char srch_lng[33];
						char srch_lng_esc[65];

						srch_lng[0] = srch_lng_esc[0] = '\0';
						if(qp3_parm->zsrchlng != NULL)
						{
							if(qp3_parm->zsrchlnglen > 32)
								qp3_parm->zsrchlnglen = 32;
							memcpy((void*)srch_lng, (void*)qp3_parm->zsrchlng, qp3_parm->zsrchlnglen);
							srch_lng[qp3_parm->zsrchlnglen] = '\0';
							srch_lng_esc[conn->escape_string(srch_lng, qp3_parm->zsrchlnglen, srch_lng_esc)] = '\0';

							stemmer = sb_stemmer_new(srch_lng, "UTF_8");
						}

						// here we query phrasea !
						CMutex sqlmutex;

						Cquerytree2Parm querytree_parm(
										 qp3_parm->query,
										 'M',					// main thread
										 0,						// depth 0
										 conn,

										 (char *)row->field(1, "127.0.0.1"),	// host
										 atoi(row->field(2, "3306")),	// port
										 (char *)row->field(5, "root"),			// user
										 (char *)row->field(6, ""),				// pwd
										 (char *)row->field(4, "dbox"),			// base
//										 (char *)tmptable,						// tmptable
										 (char *)NULL,						// tmptable

										 (char *)sqltmp.c_str(),

										 &sqlmutex,
										 qp3_parm->zquery,
										 sqltrec,
										 /*t_collmask,*/
										 pzsortfield,
										 qp3_parm->sortmethod,
										 qp3_parm->sqlbusiness_c,
										 stemmer,
										 srch_lng_esc,
										 0				//	0 : full query, do not filter on rid
						);

						startChrono(chrono);


						querytree_public((void *) &querytree_parm);


						if(qp3_parm->zquery)
							add_assoc_double(qp3_parm->zquery, (char *) "time_queries", stopChrono(chrono));

						if(stemmer)
							sb_stemmer_delete(stemmer);

						// serialize in binary to write on cache files
						CANSWER *answer;
						CSPOT *spot;
						int n_answers = 0, n_spots = 0;
						bool sortBySha256 = false;

						// count answers, spots, ...
						// ...and check if any 'sha256' is returned (in case results will be sorted by sha256)
						CHRONO time_sort;
						startChrono(time_sort);

						long current_cid = -1;
						long current_bid = -1;
						std::multiset<PCANSWER, PCANSWERCOMPRID_DESC>::iterator it;
						for(it = qp3_parm->query->answers.begin(); it != qp3_parm->query->answers.end(); it++)
						{
							answer = *(it);

							if(answer->cid != current_cid)
							{
								// change of collection, search id of matching local base
								if((current_bid = t_collid[answer->cid]) == 0) // 0 if cid is unknown (default int constructor)
									current_bid = -1;
								current_cid = answer->cid;
							}
							answer->bid = current_bid;

							n_answers++;
							answer->nspots = 0;
							for(spot = answer->firstspot; spot; spot = spot->_nextspot)
							{
								n_spots++;
								answer->nspots++;
							}
							if(answer->sha2)
								sortBySha256 = true;
						}

						// if we need sort
						if(sortBySha256)
						{
							qp3_parm->ret = new std::multiset<PCANSWER, bool(*)(PCANSWER, PCANSWER)>(pcanswercomp_sha);
						}
						else if(qp3_parm->zsortfield)
						{
							if(qp3_parm->sortmethod == SORTMETHOD_STR)
							{
								if(qp3_parm->sortorder >= 0)
									qp3_parm->ret = new std::multiset<PCANSWER, bool(*)(PCANSWER, PCANSWER)>(pcanswercomp_str_asc);
								else
									qp3_parm->ret = new std::multiset<PCANSWER, bool(*)(PCANSWER, PCANSWER)>(pcanswercomp_str_desc);
							}
							else
							{
								if(qp3_parm->sortorder >= 0)
									qp3_parm->ret = new std::multiset<PCANSWER, bool(*)(PCANSWER, PCANSWER)>(pcanswercomp_int_asc);
								else
									qp3_parm->ret = new std::multiset<PCANSWER, bool(*)(PCANSWER, PCANSWER)>(pcanswercomp_int_desc);
							}
						}
						if(qp3_parm->ret)
						{
							// return sorted result
							qp3_parm->ret->insert(qp3_parm->query->answers.begin(), qp3_parm->query->answers.end());
						}
						else
						{
							qp3_parm->ret = new std::multiset<PCANSWER, bool(*)(PCANSWER, PCANSWER)>(pcanswercomp_rid_desc);
							qp3_parm->ret->insert(qp3_parm->query->answers.begin(), qp3_parm->query->answers.end());
						}

						if(qp3_parm->zquery)
							add_assoc_double(qp3_parm->zquery, (char *) "time_sort", stopChrono(time_sort));

					}

					if(conn)
						delete conn;
				}
			}
		}
	}

}
