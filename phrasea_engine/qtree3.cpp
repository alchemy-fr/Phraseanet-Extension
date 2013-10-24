#include "base_header.h"


#include "thread.h"

#define MAXSQL 8192		//  max length of sql query

static const char *math2sql[] = {"=",  "<>",  ">",  "<",  ">=",  "<="};

char *kwclause(Cquerytree2Parm *qp, KEYWORD *k);
void sprintfSQL(char *str, const char *format, ...);
void doOperatorAND(CNODE *n);
void doOperatorPROX(CNODE *n);
void doOperatorOR(CNODE *n);
void doOperatorEXCEPT(CNODE *n);


void doOperatorAND3(CNODE *n)
{
	std::multiset<PCANSWER, PCANSWERCOMPRID_DESC> ll = n->content.boperator.l->answers;
	std::multiset<PCANSWER, PCANSWERCOMPRID_DESC> lr = n->content.boperator.r->answers;
	std::multiset<PCANSWER, PCANSWERCOMPRID_DESC>::iterator lastinsert = n->answers.begin();
	while(!ll.empty() && !lr.empty())
	{
		CANSWER *al = *(ll.begin());
		CANSWER *ar = *(lr.begin());
		if(al->rid > ar->rid)
		{
			delete(al);
			ll.erase(ll.begin());
		}
		else if(ar->rid > al->rid)
		{
			delete(ar);
			lr.erase(lr.begin());
		}
		else
		{
			// here al.rid == ar.rid, we will keep al to insert into result, and delete ar
			if(!al->sha2)
			{
				al->sha2 = ar->sha2;
				ar->sha2 = NULL; // to prevent deletion of ar to delete sha !
			}

			// a 'AND' has no 'hits'
			al->freeHits();
			lastinsert = n->answers.insert(lastinsert, al);

			delete(ar);

			ll.erase(ll.begin());
			lr.erase(lr.begin());
		}
	}
	while(!ll.empty())
	{
		delete(*(ll.begin()));
		ll.erase(ll.begin());
	}
	while(!lr.empty())
	{
		delete(*(lr.begin()));
		lr.erase(lr.begin());
	}
}

void doOperatorOR3(CNODE *n)
{
	std::multiset<PCANSWER, PCANSWERCOMPRID_DESC> ll = n->content.boperator.l->answers;
	std::multiset<PCANSWER, PCANSWERCOMPRID_DESC> lr = n->content.boperator.r->answers;
	std::multiset<PCANSWER, PCANSWERCOMPRID_DESC>::iterator lastinsert = n->answers.begin();
	while(!ll.empty() && !lr.empty())
	{
		CANSWER *al = *(ll.begin());
		CANSWER *ar = *(lr.begin());
		if(al->rid > ar->rid)
		{
			lastinsert = n->answers.insert(lastinsert, al);
			ll.erase(ll.begin());
		}
		else if(ar->rid > al->rid)
		{
			lastinsert = n->answers.insert(lastinsert, ar);
			lr.erase(lr.begin());
		}
		else
		{
			// here al.rid == ar.rid, we will keep al to insert into result, and delete ar

			// attach hits of 'ar' at the end of 'al'
			if(ar->firsthit)
			{
				if(al->lasthit)
					al->lasthit->nexthit = ar->firsthit;
				else
					al->firsthit = ar->firsthit;
				al->lasthit = ar->lasthit;
			}
			// detach old hits from 'ar' to prevent deletion
			ar->firsthit = ar->lasthit = NULL;

			lastinsert = n->answers.insert(lastinsert, al);

			// drop 'ar'
			delete ar;

			ll.erase(ll.begin());
			lr.erase(lr.begin());
		}
	}
	if(!ll.empty())
	{
		n->answers.insert(ll.begin(), ll.end());
		ll.clear();
	}
	if(!lr.empty())
	{
		n->answers.insert(lr.begin(), lr.end());
		lr.clear();
	}
}

void doOperatorPROX3(CNODE *n)
{
	CHIT *hitl, *hitr, *hit;
	int prox = n->content.boperator.numparm;
	int prox0;

	std::multiset<PCANSWER, PCANSWERCOMPRID_DESC> ll = n->content.boperator.l->answers;
	std::multiset<PCANSWER, PCANSWERCOMPRID_DESC> lr = n->content.boperator.r->answers;
	std::multiset<PCANSWER, PCANSWERCOMPRID_DESC>::iterator lastinsert = n->answers.begin();
	while(!ll.empty() && !lr.empty())
	{
		CANSWER *al = *(ll.begin());
		CANSWER *ar = *(lr.begin());
		if(al->rid > ar->rid)
		{
			delete(al);
			ll.erase(ll.begin());
		}
		else if(ar->rid > al->rid)
		{
			delete(ar);
			lr.erase(lr.begin());
		}
		else
		{
			// here al.rid == ar.rid, we will keep al, deleting ar
			if(!al->sha2)
			{
				al->sha2 = ar->sha2;
				ar->sha2 = NULL; // to prevent deletion of ar to delete sha !
			}

			// keep a ptr on hits of 'al'
			hitl = al->firsthit;
			// detach hits of 'al' (if near, a new hit will be allocated)
			al->firsthit = al->lasthit = NULL;

			// compare every hit of 'al' with each hit of 'ar'
			while(hitl)
			{
				for(hitr = ar->firsthit; hitr; hitr = hitr->nexthit)
				{
//zend_printf("al->rid=%d, ar->rid=%d, hitl=[s:%d, e:%d], hitr=[s:%d, e:%d]\n", al->rid, ar->rid, hitl->iws, hitl->iwe, hitr->iws, hitr->iwe);
					if(n->type == PHRASEA_OP_BEFORE || n->type == PHRASEA_OP_NEAR)
					{
						prox0 = hitr->iws - hitl->iwe;
//zend_printf("  (%d) : prox0=%d (prox=%d)\n", __LINE__, prox0, prox);
						if(prox0 >= 1 && prox0 <= prox + 1)
						{
//zend_printf("    new CHIT(%d, %d)\n", hitl->iws, hitr->iwe);
							// if near, allocate a new hit for 'al'
							if((hit = new CHIT(hitl->iws, hitr->iwe)))
							{
								if(!al->firsthit)
									al->firsthit = hit;
								if(al->lasthit)
									al->lasthit->nexthit = hit;
								al->lasthit = hit;
							}
						}
					}
					if(n->type == PHRASEA_OP_AFTER || n->type == PHRASEA_OP_NEAR)
					{
						prox0 = hitl->iws - hitr->iwe;
// zend_printf("  (%d) : prox0=%d (prox=%d)\n", __LINE__, prox0, prox);
						if(prox0 >= 1 && prox0 <= prox + 1)
						{
							// if near, allocate a new hit for 'al'
							if((hit = new CHIT(hitr->iwe, hitl->iws)))
							{
								if(!al->firsthit)
									al->firsthit = hit;
								if(al->lasthit)
									al->lasthit->nexthit = hit;
								al->lasthit = hit;
							}
						}
					}
				}
				hit = hitl->nexthit;
				delete hitl;
				hitl = hit;
			}

			// if near was ok, 'al' has new hits
			if(al->firsthit) // yes : keep it
			{
				lastinsert = n->answers.insert(lastinsert, al);
			}
			else // no : delete
			{
				delete(al);
			}

			// drop 'ar'
			delete(ar);

			ll.erase(ll.begin());
			lr.erase(lr.begin());
		}
	}
	while(!ll.empty())
	{
		delete(*(ll.begin()));
		ll.erase(ll.begin());
	}
	while(!lr.empty())
	{
		delete(*(lr.begin()));
		lr.erase(lr.begin());
	}
}

void doOperatorEXCEPT3(CNODE *n)
{
	std::multiset<PCANSWER, PCANSWERCOMPRID_DESC> ll = n->content.boperator.l->answers;
	std::multiset<PCANSWER, PCANSWERCOMPRID_DESC> lr = n->content.boperator.r->answers;
	std::multiset<PCANSWER, PCANSWERCOMPRID_DESC>::iterator lastinsert = n->answers.begin();
	while(!ll.empty() && !lr.empty())
	{
		CANSWER *al = *(ll.begin());
		CANSWER *ar = *(lr.begin());
		if(al->rid > ar->rid)
		{
			lastinsert = n->answers.insert(lastinsert, al);
			ll.erase(ll.begin());
		}
		else if(ar->rid > al->rid)
		{
			delete ar;
			lr.erase(lr.begin());
		}
		else
		{
			delete al;
			ll.erase(ll.begin());
			delete ar;
			lr.erase(lr.begin());
		}
	}
	if(!ll.empty())
	{
		n->answers.insert(ll.begin(), ll.end());
		ll.clear();
	}
	while(!lr.empty())
	{
		delete(*(lr.begin()));
		lr.erase(lr.begin());
	}
}

THREAD_ENTRYPOINT querytree3(void *_qp)
{
	TSRMLS_FETCH();

//	if(MYSQL_THREAD_SAFE)
//		mysql_thread_init();

	char sql[10240];
	char *p;
	int l;
	KEYWORD *plk;
	zval *objl = NULL;
	zval *objr = NULL;
	CHRONO chrono_all;
	char *sqlerr = NULL;
#ifndef PHP_WIN32
//	pthread_attr_t thread_attr;
#endif

#ifdef WIN32
#else
	ATHREAD threadl, threadr;
#endif
	Cquerytree2Parm *qp = (Cquerytree2Parm *) _qp;

//	zend_printf("%s (%d) : depth=%d, business='%s' <br/>\n", __FILE__, __LINE__, qp->depth, qp->business);
// zend_printf("%s[%d] \n", __FILE__, __LINE__);

	// struct Squerytree2Parm *qp = (Squerytree2Parm *) _qp;
	sql[0] = '\0';
	startChrono(chrono_all);
	if(qp->n)
	{
		qp->n->time_C = -1;
		qp->n->time_sqlQuery = qp->n->time_sqlStore = qp->n->time_sqlFetch = -1;

		if(qp->result)
			add_assoc_long(qp->result, (char *) "type", qp->n->type);

		switch(qp->n->type)
		{

			case PHRASEA_OP_NULL: // empty query
				qp->n->nleaf = 0;
				break;

			case PHRASEA_KW_ALL: // all
				if(qp->n->content.numparm.v == 0)
				{
					if(*(qp->psortField))
					{
						// ===== OK =====
						sprintfSQL(sql, "SELECT record_id, record.coll_id"
								", prop.value AS skey"
								// ", NULL AS iw"
								// ", NULL AS sha256"
								" FROM %s"
								" INNER JOIN prop USING(record_id) WHERE prop.name='%s'"
								, qp->sqltrec
								, *(qp->psortField)
								);

					}
					else
					{
						// ===== OK =====
						sprintfSQL(sql, "SELECT record_id, record.coll_id"
								// ", NULL AS skey"
								// ", NULL AS iw"
								// ", NULL AS sha256"
								" FROM %s"
								, qp->sqltrec
								);
					}
				}
				else
				{
					if(*(qp->psortField))
					{
						// ===== OK =====
						sprintfSQL(sql, "SELECT record_id, record.coll_id"
								", prop.value AS skey"
								// ", NULL AS iw"
								// ", NULL AS sha256"
								" FROM %s"
								" INNER JOIN prop USING(record_id) WHERE prop.name='%s'"
								" ORDER BY skey DESC LIMIT %i"
								, qp->sqltrec
								, *(qp->psortField)
								, qp->n->content.numparm.v
								);
					}
					else
					{
						// ===== OK =====
						sprintfSQL(sql, "SELECT record_id, record.coll_id"
								// ", NULL AS skey"
								// ", NULL AS iw"
								// ", NULL AS sha256"
								" FROM %s"
								" ORDER BY record_id DESC LIMIT %i"
								, qp->sqltrec
								, qp->n->content.numparm.v
								);
					}
				}
				qp->sqlconn->phrasea_query3(sql, qp, &sqlerr);
				break;

			case PHRASEA_KW_LAST: // last
				if(qp->n->content.numparm.v > 0)
				{
					if(*(qp->psortField))
					{
						// ===== OK =====
						sprintfSQL(sql, "SELECT r.record_id, r.coll_id"
								", prop.value AS skey"
								// ", NULL AS iw"
								// ", NULL AS sha256"
								" FROM ("
								"SELECT record_id, record.coll_id"
								" FROM %s"
								" ORDER BY record_id DESC LIMIT %i"
								") AS r"
								" INNER JOIN prop USING(record_id) WHERE prop.name='%s'"
								, qp->sqltrec
								, qp->n->content.numparm.v
								, *(qp->psortField)
								);
					}
					else
					{
						// ===== OK =====
						sprintfSQL(sql, "SELECT record_id, record.coll_id"
								// ", NULL AS skey"
								// ", NULL AS iw"
								// ", NULL AS sha256"
								" FROM %s"
								" ORDER BY record_id DESC LIMIT %i"
								, qp->sqltrec
								, qp->n->content.numparm.v
								);
					}
					qp->sqlconn->phrasea_query3(sql, qp, &sqlerr);
				}
				break;

			case PHRASEA_KEYLIST: // simple word
				plk = qp->n->content.multileaf.firstkeyword;
				if(plk && (p = kwclause(qp, plk)))
				{
					// const char *sql_business = qp->business ? "" : "AND !idx.business";
					if(*(qp->psortField))
					{
						// ===== OK =====
						sprintfSQL(sql, "SELECT idx.record_id, record.coll_id"
									", prop.value AS skey"
									", iw"
									// ", NULL AS sha256"
									" FROM ((kword INNER JOIN idx USING(kword_id))"
									" INNER JOIN %s USING(record_id))"
									" INNER JOIN prop USING(record_id)"
									" WHERE (%s) AND (%s%s) AND prop.name='%s'"
									, qp->sqltrec
									, p
									, qp->business ? "!idx.business " : "1"
									, qp->business ? qp->business : ""
									, *(qp->psortField)
									);
					}
					else
					{
						// ===== OK =====
						sprintfSQL(sql, "SELECT idx.record_id, record.coll_id"
									", NULL AS skey"
									", iw"
									// ", NULL AS sha256"
									" FROM (kword INNER JOIN idx USING(kword_id))"
									" INNER JOIN %s USING(record_id)"
									" WHERE (%s) AND (%s%s)"
									, qp->sqltrec
									, p
									, qp->business ? "!idx.business " : "1"
									, qp->business ? qp->business : ""
									);
					}
					EFREE(p);

					if(plk->nextkeyword)
					{
						MAKE_STD_ZVAL(objl);
						array_init(objl);
						while(plk)
						{
							add_next_index_string(objl, plk->kword, true);
							plk = plk->nextkeyword;
						}
						if(qp->result)
							add_assoc_zval(qp->result, (char *) "keyword", objl);
					}
					else
					{
						if(qp->result)
							add_assoc_string(qp->result, (char *) "keyword", plk->kword, true);
					}

					qp->sqlconn->phrasea_query3(sql, qp, &sqlerr);
				}
				break;

			case PHRASEA_OP_IN:
				if(qp->n->content.boperator.l && qp->n->content.boperator.r && qp->n->content.boperator.l->type == PHRASEA_KEYLIST && qp->n->content.boperator.r->type == PHRASEA_KEYLIST
					&& qp->n->content.boperator.r->content.multileaf.firstkeyword && qp->n->content.boperator.r->content.multileaf.firstkeyword->kword)
				{
					plk = qp->n->content.boperator.l->content.multileaf.firstkeyword;
					if(plk && (p = kwclause(qp, plk)))
					{
						// SECURITY : escape fieldname
						char *pfield = qp->n->content.boperator.r->content.multileaf.firstkeyword->kword;
						int   lfield = strlen(pfield);
						char *pfield_esc = NULL;
						int   lfield_esc = qp->sqlconn->escape_string(pfield, lfield);

						if( (pfield_esc = (char*)EMALLOC(lfield_esc)) )
						{
							lfield_esc = qp->sqlconn->escape_string(pfield, lfield, pfield_esc);

							// const char *sql_business = qp->business ? "" : "AND !idx.business";
							if(*(qp->psortField))
							{
								// ===== OK =====
								sprintfSQL(sql, "SELECT idx.record_id, record.coll_id"
											", prop.value AS skey"
											", iw"
											// ", NULL AS sha256"
											" FROM (((kword INNER JOIN idx USING(kword_id))"
											" INNER JOIN %s USING(record_id))"
											" INNER JOIN xpath USING(xpath_id))"
											" INNER JOIN prop USING(record_id)"
											" WHERE (%s) AND (%s%s) AND xpath REGEXP 'DESCRIPTION\\\\[0\\\\]/%s\\\\[[0-9]+\\\\]' AND prop.name='%s'"
											, qp->sqltrec
											, p
											, qp->business ? "!idx.business " : "1"
											, qp->business ? qp->business : ""
											, pfield_esc
											, *(qp->psortField)
											);
							}
							else
							{
								// ===== OK =====
								sprintfSQL(sql, "SELECT idx.record_id, record.coll_id"
											", NULL AS skey"
											", iw"
											// ", NULL AS sha256"
											" FROM ((kword INNER JOIN idx USING(kword_id))"
											" INNER JOIN %s USING(record_id))"
											" INNER JOIN xpath USING(xpath_id)"
											" WHERE (%s) AND (%s%s) AND xpath REGEXP 'DESCRIPTION\\\\[0\\\\]/%s\\\\[[0-9]+\\\\]'"
											, qp->sqltrec
											, p
											, qp->business ? "!idx.business " : "1"
											, qp->business ? qp->business : ""
											, pfield_esc
											);
							}
							EFREE(pfield_esc);

							if(qp->result)
							{
								if(plk->nextkeyword)
								{
									MAKE_STD_ZVAL(objl);
									array_init(objl);
									while(plk)
									{
										add_next_index_string(objl, plk->kword, true);
										plk = plk->nextkeyword;
									}
									add_assoc_zval(qp->result, (char *) "keyword", objl);
								}
								else
								{
									add_assoc_string(qp->result, (char *) "keyword", plk->kword, true);
								}
								add_assoc_string(qp->result, (char *) "field", qp->n->content.boperator.r->content.multileaf.firstkeyword->kword, true);
							}
							qp->sqlconn->phrasea_query3(sql, qp, &sqlerr);
						}
						else
						{
							// pfield_esc alloc error
						}
						EFREE(p);
					}
					else
					{
						// kwclause (p) alloc error
					}
				}
				break;

			case PHRASEA_OP_EQUAL:
			case PHRASEA_OP_NOTEQU:
			case PHRASEA_OP_GT:
			case PHRASEA_OP_LT:
			case PHRASEA_OP_GEQT:
			case PHRASEA_OP_LEQT:
				if(qp->n->content.boperator.l && qp->n->content.boperator.r && qp->n->content.boperator.l->type == PHRASEA_KEYLIST && qp->n->content.boperator.r->type == PHRASEA_KEYLIST)
				{
					if(qp->n->content.boperator.l->content.multileaf.firstkeyword && qp->n->content.boperator.r->content.multileaf.firstkeyword)
					{
						if(qp->n->content.boperator.l->content.multileaf.firstkeyword->kword && qp->n->content.boperator.r->content.multileaf.firstkeyword->kword)
						{
							if(qp->result)
							{
								add_assoc_string(qp->result, (char *) "field", qp->n->content.boperator.l->content.multileaf.firstkeyword->kword, true);
								add_assoc_string(qp->result, (char *) "value", qp->n->content.boperator.r->content.multileaf.firstkeyword->kword, true);
							}

							sql[0] = '\0';

							// SECURITY : escape fieldname...
							char *pfield = qp->n->content.boperator.l->content.multileaf.firstkeyword->kword;
							int   lfield = strlen(pfield);
							char *pfield_esc = NULL;
							int   lfield_esc = qp->sqlconn->escape_string(pfield, lfield);
							// ...and fieldvalue
							char *pvalue = qp->n->content.boperator.r->content.multileaf.firstkeyword->kword;
							int   lvalue = strlen(pvalue);
							char *pvalue_esc = NULL;
							int   lvalue_esc = qp->sqlconn->escape_string(pvalue, lvalue);

							if( (pfield_esc = (char*)EMALLOC(lfield_esc)) && (pvalue_esc = (char*)EMALLOC(lvalue_esc)) )
							{
								lfield_esc = qp->sqlconn->escape_string(pfield, lfield, pfield_esc);
								lvalue_esc = qp->sqlconn->escape_string(pvalue, lvalue, pvalue_esc);

								// technical field sha256 ?

								if(!sql[0] && strcmp(pfield, "sha256") == 0)
								{
									if(qp->n->type == PHRASEA_OP_EQUAL && strcmp(pvalue, "sha256") == 0)
									{
										// special query "sha256=sha256" (tuples)
										*(qp->psortField) = NULL; // !!!!! this special query cancels sort !!!!!
										// ===== OK =====
										sprintfSQL(sql, "SELECT record_id, coll_id"
												// ", NULL AS skey"
												// ", NULL AS iw"
												", sha256"
												" FROM (SELECT sha256, SUM(1) AS n FROM %s GROUP BY sha256 HAVING n>1) AS b"
												" INNER JOIN record USING(sha256)"
												, qp->sqltrec
												);
									}
									else
									{
										// ===== OK =====
										if(*(qp->psortField))
										{
											sprintfSQL(sql, "SELECT record_id, record.coll_id"
													", prop.value AS skey"
													// ", NULL AS iw"
													// ", NULL AS sha256"
													" FROM %s"
													" INNER JOIN prop USING(record_id)"
													" WHERE sha256%s'%s' AND prop.name='%s'"
													, qp->sqltrec
													, math2sql[qp->n->type - PHRASEA_OP_EQUAL]
													, pvalue_esc
													, *(qp->psortField)
													);
										}
										else
										{
											// ===== OK =====
											sprintfSQL(sql, "SELECT record_id, record.coll_id FROM %s WHERE sha256%s'%s'"
													, qp->sqltrec
													, math2sql[qp->n->type - PHRASEA_OP_EQUAL]
													, pvalue_esc
													);
										}
									}
								}


								// technical field recordid ?
								if(!sql[0] && strcmp(pfield, "recordid") == 0)
								{
									if(*(qp->psortField))
									{
										// ===== OK =====
										sprintfSQL(sql, "SELECT record.record_id, record.coll_id"
												", prop.value AS skey"
												// ", NULL AS iw"
												// ", NULL AS sha256"
												" FROM %s"
												" INNER JOIN prop USING(record_id)"
												" WHERE record.record_id%s'%s' AND prop.name='%s'"
												, qp->sqltrec
												, math2sql[qp->n->type - PHRASEA_OP_EQUAL]
												, pvalue_esc
												, *(qp->psortField)
												);
									}
									else
									{
										// ===== OK =====
										sprintfSQL(sql, "SELECT record_id, record.coll_id"
												// ", NULL AS skey"
												// ", NULL AS iw"
												// ", NULL AS sha256"
												" FROM %s WHERE record_id%s'%s'" // ORDER BY record_id DESC"
												, qp->sqltrec
												, math2sql[qp->n->type - PHRASEA_OP_EQUAL]
												, pvalue_esc
												);
									}
								}

								// technical field recordtype ?
								if(!sql[0] && strcmp(pfield, "recordtype") == 0)
								{
									if(*(qp->psortField))
									{
										// ===== OK =====
										sprintfSQL(sql, "SELECT record_id, record.coll_id"
												", prop.value AS skey"
												// ", NULL AS iw"
												// ", NULL AS sha256"
												" FROM %s"
												" INNER JOIN prop USING(record_id)"
												" WHERE record.type%s'%s' AND prop.name='%s'"
												, qp->sqltrec
												, math2sql[qp->n->type - PHRASEA_OP_EQUAL]
												, pvalue_esc
												, *(qp->psortField)
												);
									}
									else
									{
										// ===== OK =====
										sprintfSQL(sql, "SELECT record_id, record.coll_id"
												// ", NULL AS skey"
												// ", NULL AS iw"
												// ", NULL AS sha256"
												" FROM %s"
												" WHERE type%s'%s'" // ORDER BY record_id DESC"
												, qp->sqltrec
												, math2sql[qp->n->type - PHRASEA_OP_EQUAL]
												, pvalue_esc
												);
									}
								}

								// technical field thumbnail or preview or document ?
								if(!sql[0] && (strcmp(pfield, "thumbnail") == 0 || strcmp(pfield, "preview") == 0 || strcmp(pfield, "document") == 0))
								{
									char w[] = "NOT(ISNULL(subdef.record_id))";
									if(qp->n->content.boperator.r->content.multileaf.firstkeyword->kword[0] == '0')
										strcpy(w, "ISNULL(subdef.record_id)");
									if(*(qp->psortField))
									{
										// ===== OK =====
										sprintfSQL(sql, "SELECT record.record_id, record.coll_id"
												", prop.value AS skey"
												// ", NULL AS iw"
												// ", NULL AS sha256"
												" FROM (%s LEFT JOIN subdef ON(record.record_id=subdef.record_id AND subdef.name='%s'))"
												" INNER JOIN prop ON(prop.record_id=record.record_id)"
												" WHERE %s AND prop.name='%s'"
												, qp->sqltrec
												, pfield
												, w
												, *(qp->psortField)
												);
									}
									else
									{
										// ===== OK =====
										sprintfSQL(sql, "SELECT record.record_id, record.coll_id"
												// ", NULL AS skey"
												// ", NULL AS iw"
												// ", NULL AS sha256"
												" FROM %s"
												" LEFT JOIN subdef ON(record.record_id=subdef.record_id AND subdef.name='%s')"
												" WHERE %s" // ORDER BY record_id DESC"
												, qp->sqltrec
												, pfield
												, w
												);
									}
								}

								// champ technique recordstatus ?
								if(!sql[0] && strcmp(pfield, "recordstatus") == 0)
								{
									// convert status mask 01xx10x1x0110x0 to mask_and and mask_xor
									char buff_and[65];	// 64 bits and nul
									char buff_xor[65];	// 64 bits and nul
									int i; l=MIN(strlen(pvalue), 64);
									for(i=0; i<l; i++)
									{
										buff_and[i] = buff_xor[i] = '0';
										switch(pvalue[i])
										{
											case '0':
												buff_and[i] = '1';
												break;
											case '1':
												buff_and[i] = buff_xor[i] = '1';
												break;
										}
									}
									if(i==0)	// fix 0b to 0b0;
									{
										buff_and[i] = buff_xor[i] = '0';
										i++;
									}
									buff_and[i] = buff_xor[i] = '\0';

									if(*(qp->psortField))
									{
										// ===== OK =====
										sprintfSQL(sql, "SELECT record_id, record.coll_id"
												", prop.value AS skey"
												// ", NULL AS iw"
												// ", NULL AS sha256"
												" FROM %s INNER JOIN prop USING(record_id)"
												" WHERE ((status ^ 0b%s) & 0b%s = 0) AND prop.name='%s'"
												, qp->sqltrec
												, buff_xor
												, buff_and
												, *(qp->psortField)
												);
									}
									else
									{
										// ===== OK =====
										sprintfSQL(sql, "SELECT record_id, record.coll_id"
												// ", NULL AS skey"
												// ", NULL AS iw"
												// ", NULL AS sha256"
												" FROM %s"
												" WHERE ((status ^ 0b%s) & 0b%s = 0)"
												, qp->sqltrec
												, buff_xor
												, buff_and
												);
									}
								}

								if(!sql[0]) // it was not a technical field = it was a field of the bas (structure)
								{
									// const char *sql_business = qp->business ? "" : "AND !prop.business";

									for(char *p = pfield_esc; *p; p++)
									{
										if(*p >= 'a' && *p <= 'z')
											*p += ('A' - 'a');
									}
									if(*(qp->psortField))
									{
										// ===== OK =====
										sprintfSQL(sql, "SELECT record.record_id, record.coll_id"
												", propsort.value AS skey"
												// ", NULL AS iw"
												// ", NULL AS sha256"
												" FROM (prop INNER JOIN %s USING(record_id))"
												" INNER JOIN prop AS propsort USING(record_id)"
												" WHERE prop.name='%s' AND prop.value%s'%s' AND (%s%s) AND propsort.name='%s'"
												, qp->sqltrec
												, pfield_esc
												, math2sql[qp->n->type - PHRASEA_OP_EQUAL]
												, pvalue_esc
												, qp->business ? "!prop.business " : "1"
												, qp->business ? qp->business : ""
												, *(qp->psortField)
												);
									}
									else
									{
										// ===== OK =====
										sprintfSQL(sql, "SELECT record.record_id, record.coll_id"
												// ", NULL AS skey"
												// ", NULL AS iw"
												// ", NULL AS sha256"
												" FROM prop INNER JOIN %s USING(record_id)"
												" WHERE name='%s' AND value%s'%s' AND (%s%s)"
												, qp->sqltrec
												, pfield
												, math2sql[qp->n->type - PHRASEA_OP_EQUAL]
												, pvalue
												, qp->business ? "!prop.business " : "1"
												, qp->business ? qp->business : ""
												);
									}
								}

								qp->sqlconn->phrasea_query3(sql, qp, &sqlerr);
							}
							else
							{
								// pfield or pvalue alloc error
							}
							if(pfield_esc)
								EFREE(pfield_esc);
							if(pvalue_esc)
								EFREE(pvalue_esc);
						}
					}
				}
				break;

			case PHRASEA_OP_COLON:
				if(qp->n->content.boperator.l && qp->n->content.boperator.r && qp->n->content.boperator.l->type == PHRASEA_KEYLIST && qp->n->content.boperator.r->type == PHRASEA_KEYLIST)
				{
					if(qp->n->content.boperator.l->content.multileaf.firstkeyword && qp->n->content.boperator.r->content.multileaf.firstkeyword)
					{
						if(qp->n->content.boperator.l->content.multileaf.firstkeyword->kword && qp->n->content.boperator.r->content.multileaf.firstkeyword->kword)
						{
							// SECURITY : escape fieldname
							char *pfield = qp->n->content.boperator.l->content.multileaf.firstkeyword->kword;
							int  lfield = strlen(pfield);
							int lfield_esc = qp->sqlconn->escape_string(pfield, lfield);
							char *pfield_esc = (char*) EMALLOC(lfield_esc);

							if(pfield_esc)
							{
								lfield_esc = pfield_esc[qp->sqlconn->escape_string(pfield, lfield, pfield_esc)] = '\0';
							}
							char *fvalue1 = NULL; // for sql : " thit.value LIKE 'xxx' OR thit.value LIKE 'yyy' "
							int l1 = 1; // '\0' final
							char *fvalue2 = NULL; // for ret : " xxx | yyy "
							int l2 = 1; // '\0' final

							KEYWORD *k;
							for(k = qp->n->content.boperator.r->content.multileaf.firstkeyword; k; k = k->nextkeyword)
							{
//qp->sqlconn->thesaurus_search(k->kword);
								l = strlen(k->kword);
								l2 += l; // xxx
								k->l_esc = qp->sqlconn->escape_string(k->kword, strlen(k->kword));	// needed

								l1 += 17 + k->l_esc + 1; // "thit.value LIKE 'xxx'"
								if(k->nextkeyword)
								{
									l1 += 4; // " OR "
									l2 += 3; // ' | '
								}
							}
							fvalue1 = (char *) EMALLOC(l1);
							fvalue2 = (char *) EMALLOC(l2);
							if(fvalue1 && fvalue2 && pfield_esc)
							{
								int l;
								// const char *sql_business = qp->business ? "" : "AND !thit.business";
								char *p1 = fvalue1;
								char *p2 = fvalue2;
								for(k = qp->n->content.boperator.r->content.multileaf.firstkeyword; k; k = k->nextkeyword)
								{
									l = strlen(k->kword);

									memcpy(p1, "thit.value LIKE '", 17);
									p1 += 17;
									p1 += qp->sqlconn->escape_string(k->kword, l, p1);
									*p1++ = '\'';
									if(k->nextkeyword)
									{
										memcpy(p1, " OR ", 4);
										p1 += 4;
									}

									memcpy(p2, k->kword, l);
									p2 += l;
									if(k->nextkeyword)
									{
										memcpy(p2, " | ", 3);
										p2 += 3;
									}
								}
								*p1 = '\0';
								*p2 = '\0';

								if(qp->result)
								{
									add_assoc_string(qp->result, (char *) "field", pfield, true);
									if(fvalue2)
										add_assoc_string(qp->result, (char *) "value", fvalue2, true);
								}

								if(pfield[0] == '*' && pfield[1] == '\0')
								{
									// fieldname is '*' : avoid test on name
									if(*(qp->psortField))
									{
										// ===== OK =====
										sprintfSQL(sql, "SELECT record_id, record.coll_id"
												", prop.value AS skey"
												// ", NULL AS iw"
												// ", NULL AS sha256"
												" FROM (thit INNER JOIN %s USING(record_id))"
												" INNER JOIN prop USING(record_id)"
												" WHERE (%s) AND (%s%s) AND prop.name='%s'"
												, qp->sqltrec
												, fvalue1
												, qp->business ? "!thit.business " : "1"
												, qp->business ? qp->business : ""
												, *(qp->psortField)
												);
									}
									else
									{
										// ===== OK =====
										sprintfSQL(sql, "SELECT record_id, record.coll_id"
												// ", NULL AS skey"
												// ", NULL AS iw"
												// ", NULL AS sha256"
												" FROM thit INNER JOIN %s USING(record_id)"
												" WHERE (%s) AND (%s%s)"
												, qp->sqltrec
												, fvalue1
												, qp->business ? "!thit.business " : "1"
												, qp->business ? qp->business : ""
												);
									}
								}
								else
								{
									// there is a fieldname, must be included in sql
									if(*(qp->psortField))
									{
										// ===== OK =====
										sprintfSQL(sql, "SELECT record_id, record.coll_id"
												", prop.value AS skey"
												// ", NULL AS iw"
												// ", NULL AS sha256"
												" FROM (thit INNER JOIN %s USING(record_id))"
												" INNER JOIN prop USING(record_id)"
												" WHERE (%s) AND (%s%s) AND thit.name='%s' AND prop.name='%s'"
												, qp->sqltrec
												, fvalue1
												, qp->business ? "!thit.business " : "1"
												, qp->business ? qp->business : ""
												, pfield_esc
												, *(qp->psortField)
												);
									}
									else
									{
										// ===== OK =====
										sprintfSQL(sql, "SELECT record_id, record.coll_id"
												// ", NULL AS skey"
												// ", NULL AS iw"
												// ", NULL AS sha256"
												" FROM thit INNER JOIN %s USING(record_id)"
												" WHERE (%s) AND (%s%s) AND thit.name='%s'"
												, qp->sqltrec
												, fvalue1
												, qp->business ? "!thit.business " : "1"
												, qp->business ? qp->business : ""
												, pfield_esc
												);
									}
								}
// zend_printf("SQL2 : %s \n", sql);
								qp->sqlconn->phrasea_query3(sql, qp, &sqlerr);
							}
							else
							{
								// fvalue1 or fvalue2 alloc erroe
							}
							if(fvalue1)
								EFREE(fvalue1);
							if(fvalue2)
								EFREE(fvalue2);
							if(pfield_esc)
								EFREE(pfield_esc);
						}
					}
				}
				break;

			case PHRASEA_OP_AND: // and
				if(qp->n->content.boperator.l && qp->n->content.boperator.r)
				{
					MAKE_STD_ZVAL(objl);
					MAKE_STD_ZVAL(objr);
					array_init(objl);
					array_init(objr);

					Cquerytree2Parm qpl(qp->n->content.boperator.l, qp->depth + 1, qp->sqlconn, qp->sqlmutex, objl, qp->sqltrec, qp->psortField, qp->sortMethod, qp->business, qp->stemmer, qp->lng);
					Cquerytree2Parm qpr(qp->n->content.boperator.r, qp->depth + 1, qp->sqlconn, qp->sqlmutex, objr, qp->sqltrec, qp->psortField, qp->sortMethod, qp->business, qp->stemmer, qp->lng);
					if(MYSQL_THREAD_SAFE)
					{
#ifdef WIN32
						HANDLE  hThreadArray[2];
						unsigned dwThreadIdArray[2];
						hThreadArray[0] = (HANDLE)_beginthreadex(
							NULL,                   // default security attributes
							0,                      // use default stack size
							querytree3,			    // thread function name
							(void*)&qpl,             // argument to thread function
							0,                      // use default creation flags
							&dwThreadIdArray[0]);   // returns the thread identifier
						hThreadArray[1] = (HANDLE)_beginthreadex(
							NULL,                   // default security attributes
							0,                      // use default stack size
							querytree3,			    // thread function name
							(void*)&qpr,             // argument to thread function
							0,                      // use default creation flags
							&dwThreadIdArray[1]);   // returns the thread identifier
						WaitForMultipleObjects(2, hThreadArray, TRUE, INFINITE);
						CloseHandle(hThreadArray[0]);
						CloseHandle(hThreadArray[1]);
#else
						THREAD_START(threadl, querytree3, &qpl);
						THREAD_START(threadr, querytree3, &qpr);
						THREAD_JOIN(threadl);
						THREAD_JOIN(threadr);
#endif
					}
					else
					{
						querytree3((void *) &qpl);
						querytree3((void *) &qpr);
					}

					if(qp->result)
					{
						add_assoc_zval(qp->result, (char *) "lbranch", objl);
						add_assoc_zval(qp->result, (char *) "rbranch", objr);
					}

					CHRONO chrono;
					startChrono(chrono);

					doOperatorAND(qp->n);

					qp->n->time_C = stopChrono(chrono);
				}
				break;

			case PHRASEA_OP_NEAR: // near
			case PHRASEA_OP_BEFORE: // near
			case PHRASEA_OP_AFTER: // near
				if(qp->n->content.boperator.l && qp->n->content.boperator.r)
				{
					MAKE_STD_ZVAL(objl);
					MAKE_STD_ZVAL(objr);
					array_init(objl);
					array_init(objr);

					Cquerytree2Parm qpl(qp->n->content.boperator.l, qp->depth + 1, qp->sqlconn, qp->sqlmutex, objl, qp->sqltrec, qp->psortField, qp->sortMethod, qp->business, qp->stemmer, qp->lng);
					Cquerytree2Parm qpr(qp->n->content.boperator.r, qp->depth + 1, qp->sqlconn, qp->sqlmutex, objr, qp->sqltrec, qp->psortField, qp->sortMethod, qp->business, qp->stemmer, qp->lng);
					if(MYSQL_THREAD_SAFE)
					{
#ifdef WIN32
						HANDLE  hThreadArray[2];
						unsigned dwThreadIdArray[2];
						hThreadArray[0] = (HANDLE)_beginthreadex(
							NULL,                   // default security attributes
							0,                      // use default stack size
							querytree3,			    // thread function name
							(void*)&qpl,             // argument to thread function
							0,                      // use default creation flags
							&dwThreadIdArray[0]);   // returns the thread identifier
						hThreadArray[1] = (HANDLE)_beginthreadex(
							NULL,                   // default security attributes
							0,                      // use default stack size
							querytree3,			    // thread function name
							(void*)&qpr,             // argument to thread function
							0,                      // use default creation flags
							&dwThreadIdArray[1]);   // returns the thread identifier
						WaitForMultipleObjects(2, hThreadArray, TRUE, INFINITE);
						CloseHandle(hThreadArray[0]);
						CloseHandle(hThreadArray[1]);
#else
						THREAD_START(threadl, querytree3, &qpl);
						THREAD_START(threadr, querytree3, &qpr);
						THREAD_JOIN(threadl);
						THREAD_JOIN(threadr);
#endif
					}
					else
					{
						querytree3((void *) &qpl);
						querytree3((void *) &qpr);
					}

					if(qp->result)
					{
						add_assoc_zval(qp->result, (char *) "lbranch", objl);
						add_assoc_zval(qp->result, (char *) "rbranch", objr);
					}

					CHRONO chrono;
					startChrono(chrono);

					doOperatorPROX(qp->n);

					qp->n->time_C = stopChrono(chrono);
				}
				break;

			case PHRASEA_OP_OR: // or
				if(qp->n->content.boperator.l && qp->n->content.boperator.r)
				{
					MAKE_STD_ZVAL(objl);
					MAKE_STD_ZVAL(objr);
					array_init(objl);
					array_init(objr);

					Cquerytree2Parm qpl(qp->n->content.boperator.l, qp->depth + 1, qp->sqlconn, qp->sqlmutex, objl, qp->sqltrec, qp->psortField, qp->sortMethod, qp->business, qp->stemmer, qp->lng);
					Cquerytree2Parm qpr(qp->n->content.boperator.r, qp->depth + 1, qp->sqlconn, qp->sqlmutex, objr, qp->sqltrec, qp->psortField, qp->sortMethod, qp->business, qp->stemmer, qp->lng);
					if(MYSQL_THREAD_SAFE)
					{
#ifdef WIN32
						HANDLE  hThreadArray[2];
						unsigned dwThreadIdArray[2];
						hThreadArray[0] = (HANDLE)_beginthreadex(
							NULL,                   // default security attributes
							0,                      // use default stack size
							querytree3,			    // thread function name
							(void*)&qpl,             // argument to thread function
							0,                      // use default creation flags
							&dwThreadIdArray[0]);   // returns the thread identifier
						hThreadArray[1] = (HANDLE)_beginthreadex(
							NULL,                   // default security attributes
							0,                      // use default stack size
							querytree3,			    // thread function name
							(void*)&qpr,             // argument to thread function
							0,                      // use default creation flags
							&dwThreadIdArray[1]);   // returns the thread identifier
						WaitForMultipleObjects(2, hThreadArray, TRUE, INFINITE);
						CloseHandle(hThreadArray[0]);
						CloseHandle(hThreadArray[1]);
#else
						THREAD_START(threadl, querytree3, &qpl);
						THREAD_START(threadr, querytree3, &qpr);
						THREAD_JOIN(threadl);
						THREAD_JOIN(threadr);
#endif
					}
					else
					{
						querytree3((void *) &qpl);
						querytree3((void *) &qpr);
					}

					if(qp->result)
					{
						add_assoc_zval(qp->result, (char *) "lbranch", objl);
						add_assoc_zval(qp->result, (char *) "rbranch", objr);
					}

					CHRONO chrono;
					startChrono(chrono);

					doOperatorOR(qp->n);
					qp->n->time_C = stopChrono(chrono);
				}
				break;

			case PHRASEA_OP_EXCEPT: // except
				if(qp->n->content.boperator.l && qp->n->content.boperator.r)
				{
					MAKE_STD_ZVAL(objl);
					MAKE_STD_ZVAL(objr);
					array_init(objl);
					array_init(objr);

					Cquerytree2Parm qpl(qp->n->content.boperator.l, qp->depth + 1, qp->sqlconn, qp->sqlmutex, objl, qp->sqltrec, qp->psortField, qp->sortMethod, qp->business, qp->stemmer, qp->lng);
					Cquerytree2Parm qpr(qp->n->content.boperator.r, qp->depth + 1, qp->sqlconn, qp->sqlmutex, objr, qp->sqltrec, qp->psortField, qp->sortMethod, qp->business, qp->stemmer, qp->lng);
					if(MYSQL_THREAD_SAFE)
					{
#ifdef WIN32
						HANDLE  hThreadArray[2];
						unsigned dwThreadIdArray[2];
						hThreadArray[0] = (HANDLE)_beginthreadex(
							NULL,                   // default security attributes
							0,                      // use default stack size
							querytree3,			    // thread function name
							(void*)&qpl,             // argument to thread function
							0,                      // use default creation flags
							&dwThreadIdArray[0]);   // returns the thread identifier
						hThreadArray[1] = (HANDLE)_beginthreadex(
							NULL,                   // default security attributes
							0,                      // use default stack size
							querytree3			    // thread function name
							(void*)&qpr,             // argument to thread function
							0,                      // use default creation flags
							&dwThreadIdArray[1]);   // returns the thread identifier
						WaitForMultipleObjects(2, hThreadArray, TRUE, INFINITE);
						CloseHandle(hThreadArray[0]);
						CloseHandle(hThreadArray[1]);
#else
						THREAD_START(threadl, querytree3, &qpl);
						THREAD_START(threadr, querytree3, &qpr);
						THREAD_JOIN(threadl);
						THREAD_JOIN(threadr);
#endif
					}
					else
					{
						querytree3((void *) &qpl);
						querytree3((void *) &qpr);
					}

					if(qp->result)
					{
						add_assoc_zval(qp->result, (char *) "lbranch", objl);
						add_assoc_zval(qp->result, (char *) "rbranch", objr);
					}

					CHRONO chrono;
					startChrono(chrono);

					doOperatorEXCEPT(qp->n);

					qp->n->time_C = stopChrono(chrono);
				}
				break;

		}


		if(qp->result)
		{
			add_assoc_double(qp->result, (char *) "time_all", stopChrono(chrono_all));
			if(qp->n->time_C != -1)
				add_assoc_double(qp->result, (char *) "time_C", qp->n->time_C);
			if(sql[0] != '\0')
			{
				add_assoc_string(qp->result, (char *) "sql", sql, true);
				if(sqlerr)
					add_assoc_string(qp->result, (char *) "sqlerr", sqlerr, true);
				else
					add_assoc_null(qp->result, (char *) "sqlerr");
			}
			if(qp->n->time_sqlQuery != -1)
				add_assoc_double(qp->result, (char *) "time_sqlQuery", qp->n->time_sqlQuery);
			if(qp->n->time_sqlStore != -1)
				add_assoc_double(qp->result, (char *) "time_sqlStore", qp->n->time_sqlStore);
			if(qp->n->time_sqlFetch != -1)
				add_assoc_double(qp->result, (char *) "time_sqlFetch", qp->n->time_sqlFetch);
			add_assoc_long(qp->result, (char *) "nbanswers", qp->n->answers.size());
		}
		if(sqlerr)
			EFREE(sqlerr);
	}
	else
	{
		// zend_printf("querytree : null node\n");
	}

	if(MYSQL_THREAD_SAFE)
	{
//		mysql_thread_end();
		THREAD_EXIT(0);
	}
	return 0;
}

