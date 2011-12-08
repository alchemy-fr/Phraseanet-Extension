#include <pthread.h>

#include "base_header.h"
#include "phrasea_clock_t.h"

#include "../php_phrasea2.h"

#include "cquerytree2parm.h"

void freetree(CNODE *n);

// true global ok here
static const char *math2sql[] = {(char *) "=", (char *) "<>", (char *) ">", (char *) "<", (char *) ">=", (char *) "<="};

char *kwclause(KEYWORD *k)
{
	KEYWORD *k0 = k;
	char *p;
	int l = 0;
	char *s = NULL, c1, c2;
	bool hasmeta;
	int i;

	// at i=0, calculate the needed size ; then write at i=1
	for(i = 0; i < 2; i++)
	{
		l = 0;
		for(k = k0; k; k = k->nextkeyword)
		{
			l += s ? s[l++] = '(', 0 : 1; // "("
			hasmeta = FALSE;
			for(p = k->kword; !hasmeta && *p; p++)
			{
				if(*p == '*' || *p == '?')
					hasmeta = TRUE;
			}
			if(hasmeta)
			{
				l += s ? sprintf(s + l, "keyword LIKE '") : 14; // "keyword LIKE '"
			}
			else
			{
				l += s ? sprintf(s + l, "keyword='") : 9; // "keyword='";
			}

			for(p = k->kword; *p; p++)
			{
				switch(*p)
				{
					case '\'':
						l += s ? s[l++] = '\\', s[l++] = '\'', 0 : 2; // "\'"
						break;
					case '*':
						l += s ? s[l++] = '%', 0 : 1; // "%"
						break;
					case '?':
						l += s ? s[l++] = '_', 0 : 1; // "_"
						break;
					default:
						l += s ? s[l++] = *p, 0 : 1; // "c"
						break;
				}
			}
			l += s ? s[l++] = '\'', s[l++] = ')', 0 : 2; // "')"
			if(k->nextkeyword)
				l += s ? sprintf(s + l, " OR ") : 4; // " OR "
		}
		l += s ? s[l++] = '\0', 0 : 1; // \0 final
		if(!s && l > 0)
		{
			if(!(s = (char *) (EMALLOC(l))))
				break;
		}
	}
	return (s);
}

CNODE *qtree2tree(zval **root, int depth)
{
	TSRMLS_FETCH();
	zval **tmp1, **tmp2;
	CNODE *n = NULL;
	int i;
	KEYWORD *k;

	if(Z_TYPE_PP(root) == IS_ARRAY)
	{
		// the first elem (num) gives the type of node
		if(zend_hash_index_find(HASH_OF(*root), 0, (void **) &tmp1) == SUCCESS && Z_TYPE_PP(tmp1) == IS_LONG)
		{
			if(n = new CNODE(Z_LVAL_P(*tmp1)))
			{
				switch(n->type)
				{
					case PHRASEA_KEYLIST:
						n->content.multileaf.firstkeyword = n->content.multileaf.lastkeyword = NULL;
						for(i = 1; TRUE; i++)
						{
							if(zend_hash_index_find(HASH_OF(*root), i, (void **) &tmp1) == SUCCESS && Z_TYPE_PP(tmp1) == IS_STRING)
							{
								if(k = (KEYWORD *) (EMALLOC(sizeof (KEYWORD))))
								{
									k->kword = ESTRDUP(Z_STRVAL_P(*tmp1));
									k->nextkeyword = NULL;
									if(!(n->content.multileaf.firstkeyword))
										n->content.multileaf.firstkeyword = k;
									if(n->content.multileaf.lastkeyword)
										n->content.multileaf.lastkeyword->nextkeyword = k;
									n->content.multileaf.lastkeyword = k;
								}
							}
							else
								break;
						}
						break;
					case PHRASEA_KW_LAST: // last
						n->content.numparm.v = DEFAULTLAST;
						if(zend_hash_index_find(HASH_OF(*root), 1, (void **) &tmp1) == SUCCESS && Z_TYPE_PP(tmp1) == IS_LONG)
						{
							n->content.numparm.v = Z_LVAL_P(*tmp1);
						}
						n->nleaf = 1;
						break;
					case PHRASEA_KW_ALL: // all
						n->content.numparm.v = 0;
						if(zend_hash_index_find(HASH_OF(*root), 1, (void **) &tmp1) == SUCCESS && Z_TYPE_PP(tmp1) == IS_LONG)
						{
							n->content.numparm.v = Z_LVAL_P(*tmp1);
						}
						n->nleaf = 1;
						break;
					case PHRASEA_OP_AND: // and
					case PHRASEA_OP_OR: // or
					case PHRASEA_OP_EXCEPT: // except
					case PHRASEA_OP_EQUAL:
					case PHRASEA_OP_COLON:
					case PHRASEA_OP_NOTEQU:
					case PHRASEA_OP_GT:
					case PHRASEA_OP_LT:
					case PHRASEA_OP_GEQT:
					case PHRASEA_OP_LEQT:
					case PHRASEA_OP_IN: // in
						n->content.boperator.numparm = 0;
						n->content.boperator.l = n->content.boperator.r = NULL;
						if((zend_hash_index_find(HASH_OF(*root), 1, (void **) &tmp1) == SUCCESS) && (zend_hash_index_find(HASH_OF(*root), 2, (void **) &tmp2) == SUCCESS))
						{
							n->content.boperator.l = qtree2tree(tmp1, depth + 1);
							n->content.boperator.r = qtree2tree(tmp2, depth + 1);
							n->nleaf = n->content.boperator.l->nleaf + n->content.boperator.r->nleaf;
						}
						break;
					case PHRASEA_OP_NEAR: // near
					case PHRASEA_OP_BEFORE: // near
					case PHRASEA_OP_AFTER: // near
						n->content.boperator.numparm = 12;
						n->content.boperator.l = n->content.boperator.r = NULL;
						i = 1;
						if((zend_hash_index_find(HASH_OF(*root), i, (void **) &tmp1) == SUCCESS) && Z_TYPE_PP(tmp1) == IS_LONG)
						{
							n->content.boperator.numparm = Z_LVAL_P(*tmp1);
							i++;
						}
						if((zend_hash_index_find(HASH_OF(*root), i, (void **) &tmp1) == SUCCESS) && (zend_hash_index_find(HASH_OF(*root), i + 1, (void **) &tmp2) == SUCCESS))
						{
							n->content.boperator.l = qtree2tree(tmp1, depth + 1);
							n->content.boperator.r = qtree2tree(tmp2, depth + 1);
							n->nleaf = n->content.boperator.l->nleaf + n->content.boperator.r->nleaf;
						}
						break;
					default:
						delete n;
						n = NULL;
						break;
				}
			}
		}
		else
		{
			n = new CNODE(PHRASEA_OP_NULL);
		}
	}
	else
	{
		n = new CNODE(PHRASEA_OP_NULL);
	}
	return (n);
}

void doOperatorAND(CNODE *n)
{
	std::set<PCANSWER, PCANSWERCOMPRID_DESC> ll = n->content.boperator.l->answers;
	std::set<PCANSWER, PCANSWERCOMPRID_DESC> lr = n->content.boperator.r->answers;
	std::set<PCANSWER, PCANSWERCOMPRID_DESC>::iterator lastinsert = n->answers.begin();
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
			// attach spots of 'ar' at the end of 'al'
			if(ar->firstspot)
			{
				if(al->lastspot)
					al->lastspot->_nextspot = ar->firstspot;
				else
					al->firstspot = ar->firstspot;
				al->lastspot = ar->lastspot;
			}

			// detach old spots from 'ar' to prevent deletion
			ar->firstspot = ar->lastspot = NULL;

			// a 'AND' has no 'hits'
			al->freeHits();
			lastinsert = n->answers.insert(lastinsert, al);

			// n->nbranswers++;

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

void doOperatorOR(CNODE *n)
{
	std::set<PCANSWER, PCANSWERCOMPRID_DESC> ll = n->content.boperator.l->answers;
	std::set<PCANSWER, PCANSWERCOMPRID_DESC> lr = n->content.boperator.r->answers;
	std::set<PCANSWER, PCANSWERCOMPRID_DESC>::iterator lastinsert = n->answers.begin();
	while(!ll.empty() && !lr.empty())
	{
		CANSWER *al = *(ll.begin());
		CANSWER *ar = *(lr.begin());
		if(al->rid > ar->rid)
		{
			lastinsert = n->answers.insert(lastinsert, al);
			ll.erase(ll.begin());
			// n->nbranswers++;
		}
		else if(ar->rid > al->rid)
		{
			lastinsert = n->answers.insert(lastinsert, ar);
			lr.erase(lr.begin());
			// n->nbranswers++;
		}
		else
		{
			// here al.rid == ar.rid, we will keep al to insert into result, and delete ar
			// attach spots of 'ar' at the end of 'al'
			if(ar->firstspot)
			{
				if(al->lastspot)
					al->lastspot->_nextspot = ar->firstspot;
				else
					al->firstspot = ar->firstspot;
				al->lastspot = ar->lastspot;
			}
			// detach old spots from 'ar' to prevent deletion
			ar->firstspot = ar->lastspot = NULL;

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

			// n->nbranswers++;

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
	// n->nbranswers = n->answers.size();
}

void doOperatorPROX(CNODE *n)
{
	/* to see later : optimizations
	if(n->content.boperator.l->isempty && n->content.boperator.r->isempty)
	{
		n->isempty = TRUE;
		n->time_C = stopChrono(chrono);
		break;
	}
	if(n->content.boperator.l->isempty)
	{
		n->firstanswer = n->content.boperator.r->firstanswer;
		n->lastanswer = n->content.boperator.r->lastanswer;
		n->nbranswers = n->content.boperator.r->nbranswers;

		n->content.boperator.r->firstanswer = n->content.boperator.r->lastanswer = NULL;

		n->time_C = stopChrono(chrono);
		break;
	}
	if(n->content.boperator.r->isempty)
	{
		n->firstanswer = n->content.boperator.l->firstanswer;
		n->lastanswer = n->content.boperator.l->lastanswer;
		n->nbranswers = n->content.boperator.l->nbranswers;

		n->content.boperator.l->firstanswer = n->content.boperator.l->lastanswer = NULL;

		n->time_C = stopChrono(chrono);
		break;
	}
	 */
	CHIT *hitl, *hitr, *hit;
	int prox = n->content.boperator.numparm;
	int prox0;

	std::set<PCANSWER, PCANSWERCOMPRID_DESC> ll = n->content.boperator.l->answers;
	std::set<PCANSWER, PCANSWERCOMPRID_DESC> lr = n->content.boperator.r->answers;
	std::set<PCANSWER, PCANSWERCOMPRID_DESC>::iterator lastinsert = n->answers.begin();
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
			// attach spots of 'ar' at the end of 'al'
			if(ar->firstspot)
			{
				if(al->lastspot)
					al->lastspot->_nextspot = ar->firstspot;
				else
					al->firstspot = ar->firstspot;
				al->lastspot = ar->lastspot;
			}
			// detach old spots from 'ar' to prevent deletion
			ar->firstspot = ar->lastspot = NULL;

			// keep a ptr on hits of 'al'
			hitl = al->firsthit;
			// detach hits of 'al' (if near, a new hit will be allocated)
			al->firsthit = al->lasthit = NULL;

			// compare every hit of 'al' with each hit of 'ar'
			while(hitl)
			{
				for(hitr = ar->firsthit; hitr; hitr = hitr->nexthit)
				{
					if(n->type == PHRASEA_OP_BEFORE || n->type == PHRASEA_OP_NEAR)
					{
						prox0 = hitr->iws - hitl->iwe;
						if(prox0 >= 0 && prox0 <= prox + 1)
						{
							// if near, allocate a new hit for 'al'
							if((hit = new CHIT(hitl->iws, hitl->iwe)))
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
						if(prox0 >= 0 && prox0 <= prox + 1)
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

				// n->nbranswers++;
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

void doOperatorEXCEPT(CNODE *n)
{
	std::set<PCANSWER, PCANSWERCOMPRID_DESC> ll = n->content.boperator.l->answers;
	std::set<PCANSWER, PCANSWERCOMPRID_DESC> lr = n->content.boperator.r->answers;
	std::set<PCANSWER, PCANSWERCOMPRID_DESC>::iterator lastinsert = n->answers.begin();
	while(!ll.empty() && !lr.empty())
	{
		CANSWER *al = *(ll.begin());
		CANSWER *ar = *(lr.begin());
		if(al->rid > ar->rid)
		{
			lastinsert = n->answers.insert(lastinsert, al);
			ll.erase(ll.begin());
			// n->nbranswers++;
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

void *querytree2(void *_qp)
{
	char sql[102400];
	char *p;
	int l;
	KEYWORD *plk;
	zval *objl = NULL;
	zval *objr = NULL;
	CHRONO chrono_all;

	pthread_attr_t thread_attr;
	pthread_t threadl, threadr;
	Cquerytree2Parm *qp = (Cquerytree2Parm *) _qp;

	TSRMLS_FETCH();

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
				// qp->n->nbranswers = 0;
				qp->n->nleaf = 0;
				break;

			case PHRASEA_KW_ALL: // all
				if(qp->n->content.numparm.v == 0)
				{
					if(*(qp->psortField))
					{
						// ===== OK =====
						sprintf(sql, "SELECT record_id, record.coll_id"
								", prop.value AS skey"
								// ", NULL AS hitstart, NULL AS hitlen, NULL AS iw"
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
						sprintf(sql, "SELECT record_id, record.coll_id"
								// ", NULL AS skey"
								// ", NULL AS hitstart, NULL AS hitlen, NULL AS iw"
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
						sprintf(sql, "SELECT record_id, record.coll_id"
								", prop.value AS skey"
								// ", NULL AS hitstart, NULL AS hitlen, NULL AS iw"
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
						sprintf(sql, "SELECT record_id, record.coll_id"
								// ", NULL AS skey"
								// ", NULL AS hitstart, NULL AS hitlen, NULL AS iw"
								// ", NULL AS sha256"
								" FROM %s"
								" ORDER BY record_id DESC LIMIT %i"
								, qp->sqltrec
								, qp->n->content.numparm.v
								);
					}
				}
				qp->sqlconn->phrasea_query(sql, qp);
				break;

			case PHRASEA_KW_LAST: // last
				if(qp->n->content.numparm.v > 0)
				{
					if(*(qp->psortField))
					{
						// ===== OK =====
						sprintf(sql, "SELECT r.record_id, r.coll_id"
								", prop.value AS skey"
								// ", NULL AS hitstart, NULL AS hitlen, NULL AS iw"
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
						sprintf(sql, "SELECT record_id, record.coll_id"
								// ", NULL AS skey"
								// ", NULL AS hitstart, NULL AS hitlen, NULL AS iw"
								// ", NULL AS sha256"
								" FROM %s"
								" ORDER BY record_id DESC LIMIT %i"
								, qp->sqltrec
								, qp->n->content.numparm.v
								);
					}
					qp->sqlconn->phrasea_query(sql, qp);
				}
				break;

			case PHRASEA_KEYLIST: // simple word
				plk = qp->n->content.multileaf.firstkeyword;
				if(plk && (p = kwclause(plk)))
				{
					if(*(qp->psortField))
					{
						// ===== OK =====
						l = sprintf(sql, "SELECT idx.record_id, record.coll_id"
									", prop.value AS skey"
									", hitstart, hitlen, iw"
									// ", NULL AS sha256"
									" FROM ((kword INNER JOIN idx USING(kword_id))"
									" INNER JOIN %s USING(record_id))"
									" INNER JOIN prop USING(record_id)"
									" WHERE (%s) AND prop.name='%s'"
									, qp->sqltrec
									, p
									, *(qp->psortField)
									);
					}
					else
					{
						// ===== OK =====
						l = sprintf(sql, "SELECT idx.record_id, record.coll_id"
									", NULL AS skey"
									", hitstart, hitlen, iw"
									// ", NULL AS sha256"
									" FROM (kword INNER JOIN idx USING(kword_id))"
									" INNER JOIN %s USING(record_id)"
									" WHERE (%s)"
									, qp->sqltrec
									, p
									);
					}
					EFREE(p);

					if(plk->nextkeyword)
					{
						MAKE_STD_ZVAL(objl);
						array_init(objl);
						while(plk)
						{
							add_next_index_string(objl, plk->kword, TRUE);
							plk = plk->nextkeyword;
						}
						if(qp->result)
							add_assoc_zval(qp->result, (char *) "keyword", objl);
					}
					else
					{
						if(qp->result)
							add_assoc_string(qp->result, (char *) "keyword", plk->kword, TRUE);
					}

					qp->sqlconn->phrasea_query(sql, qp);
				}
				break;

			case PHRASEA_OP_IN:
				if(qp->n->content.boperator.l && qp->n->content.boperator.r && qp->n->content.boperator.l->type == PHRASEA_KEYLIST && qp->n->content.boperator.r->type == PHRASEA_KEYLIST
					&& qp->n->content.boperator.r->content.multileaf.firstkeyword && qp->n->content.boperator.r->content.multileaf.firstkeyword->kword)
				{
					plk = qp->n->content.boperator.l->content.multileaf.firstkeyword;
					if(plk && (p = kwclause(plk)))
					{
						if(*(qp->psortField))
						{
							// ===== OK =====
							l = sprintf(sql, "SELECT idx.record_id, record.coll_id"
										", prop.value AS skey"
										", hitstart, hitlen, iw"
										// ", NULL AS sha256"
										" FROM (((kword INNER JOIN idx USING(kword_id))"
										" INNER JOIN %s USING(record_id))"
										" INNER JOIN xpath USING(xpath_id))"
										" INNER JOIN prop USING(record_id)"
										" WHERE (%s) AND xpath REGEXP 'DESCRIPTION\\\\[0\\\\]/%s\\\\[[0-9]+\\\\]' AND prop.name='%s'"
										, qp->sqltrec
										, p
										, qp->n->content.boperator.r->content.multileaf.firstkeyword->kword
										, *(qp->psortField)
										);
						}
						else
						{
							// ===== OK =====
							l = sprintf(sql, "SELECT idx.record_id, record.coll_id"
										", NULL AS skey"
										", hitstart, hitlen, iw"
										// ", NULL AS sha256"
										" FROM ((kword INNER JOIN idx USING(kword_id))"
										" INNER JOIN %s USING(record_id))"
										" INNER JOIN xpath USING(xpath_id)"
										" WHERE (%s) AND xpath REGEXP 'DESCRIPTION\\\\[0\\\\]/%s\\\\[[0-9]+\\\\]'"
										, qp->sqltrec
										, p
										, qp->n->content.boperator.r->content.multileaf.firstkeyword->kword
										);
						}
						EFREE(p);

						if(qp->result)
						{
							if(plk->nextkeyword)
							{
								MAKE_STD_ZVAL(objl);
								array_init(objl);
								while(plk)
								{
									add_next_index_string(objl, plk->kword, TRUE);
									plk = plk->nextkeyword;
								}
								add_assoc_zval(qp->result, (char *) "keyword", objl);
							}
							else
							{
								add_assoc_string(qp->result, (char *) "keyword", plk->kword, TRUE);
							}
							add_assoc_string(qp->result, (char *) "field", qp->n->content.boperator.r->content.multileaf.firstkeyword->kword, TRUE);
						}
						qp->sqlconn->phrasea_query(sql, qp);
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
								add_assoc_string(qp->result, (char *) "field", qp->n->content.boperator.l->content.multileaf.firstkeyword->kword, TRUE);
								add_assoc_string(qp->result, (char *) "value", qp->n->content.boperator.r->content.multileaf.firstkeyword->kword, TRUE);
							}

							sql[0] = '\0';

							char *fname = qp->n->content.boperator.l->content.multileaf.firstkeyword->kword;

							// technical field sha256 ?
							if(!sql[0] && strcmp(fname, "sha256") == 0)
							{
								if(qp->n->type == PHRASEA_OP_EQUAL && strcmp(qp->n->content.boperator.r->content.multileaf.firstkeyword->kword, "sha256") == 0)
								{
									// special query "sha256=sha256" (tuples)
									*(qp->psortField) = NULL; // !!!!! this special query cancels sort !!!!!
									// ===== OK =====
									sprintf(sql, "SELECT record_id, coll_id"
											", NULL AS skey"
											", NULL AS hitstart, NULL AS hitlen, NULL AS iw"
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
										sprintf(sql, "SELECT record_id, record.coll_id"
												", prop.value AS skey"
												// ", NULL AS hitstart, NULL AS hitlen, NULL AS iw"
												// ", NULL AS sha256"
												" FROM %s"
												" INNER JOIN prop USING(record_id)"
												" WHERE sha256%s'%s' AND prop.name='%s'"
												, qp->sqltrec
												, math2sql[qp->n->type - PHRASEA_OP_EQUAL]
												, qp->n->content.boperator.r->content.multileaf.firstkeyword->kword
												, *(qp->psortField)
												);
									}
									else
									{
										// ===== OK =====
										sprintf(sql, "SELECT record_id, record.coll_id"
												// ", NULL AS skey"
												// ", NULL AS hitstart, NULL AS hitlen, NULL AS iw"
												// ", NULL AS sha256"
												" FROM %s WHERE sha256%s'%s'" // ORDER BY record_id DESC"
												, qp->sqltrec
												, math2sql[qp->n->type - PHRASEA_OP_EQUAL]
												, qp->n->content.boperator.r->content.multileaf.firstkeyword->kword
												);
									}
								}
							}

							// technical field recordid ?
							if(!sql[0] && strcmp(fname, "recordid") == 0)
							{
								if(*(qp->psortField))
								{
									// ===== OK =====
									sprintf(sql, "SELECT record.record_id, record.coll_id"
											", prop.value AS skey"
											// ", NULL AS hitstart, NULL AS hitlen, NULL AS iw"
											// ", NULL AS sha256"
											" FROM %s"
											" INNER JOIN prop USING(record_id)"
											" WHERE record.record_id%s%s AND prop.name='%s'"
											, qp->sqltrec
											, math2sql[qp->n->type - PHRASEA_OP_EQUAL]
											, qp->n->content.boperator.r->content.multileaf.firstkeyword->kword
											, *(qp->psortField)
											);
								}
								else
								{
									// ===== OK =====
									sprintf(sql, "SELECT record_id, record.coll_id"
											// ", NULL AS skey"
											// ", NULL AS hitstart, NULL AS hitlen, NULL AS iw"
											// ", NULL AS sha256"
											" FROM %s WHERE record_id%s%s" // ORDER BY record_id DESC"
											, qp->sqltrec
											, math2sql[qp->n->type - PHRASEA_OP_EQUAL]
											, qp->n->content.boperator.r->content.multileaf.firstkeyword->kword
											);
								}
							}

							// technical field recordtype ?
							if(!sql[0] && strcmp(fname, "recordtype") == 0)
							{
								if(*(qp->psortField))
								{
									// ===== OK =====
									sprintf(sql, "SELECT record_id, record.coll_id"
											", prop.value AS skey"
											// ", NULL AS hitstart, NULL AS hitlen, NULL AS iw"
											// ", NULL AS sha256"
											" FROM %s"
											" INNER JOIN prop USING(record_id)"
											" WHERE record.type%s'%s' AND prop.name='%s'"
											, qp->sqltrec
											, math2sql[qp->n->type - PHRASEA_OP_EQUAL]
											, qp->n->content.boperator.r->content.multileaf.firstkeyword->kword
											, *(qp->psortField)
											);
								}
								else
								{
									// ===== OK =====
									sprintf(sql, "SELECT record_id, record.coll_id"
											// ", NULL AS skey"
											// ", NULL AS hitstart, NULL AS hitlen, NULL AS iw"
											// ", NULL AS sha256"
											" FROM %s"
											" WHERE type%s'%s'" // ORDER BY record_id DESC"
											, qp->sqltrec
											, math2sql[qp->n->type - PHRASEA_OP_EQUAL]
											, qp->n->content.boperator.r->content.multileaf.firstkeyword->kword
											);
								}
							}

							// technical field thumbnail or preview or document ?
							if(!sql[0] && (strcmp(fname, "thumbnail") == 0 || strcmp(fname, "preview") == 0 || strcmp(fname, "document") == 0))
							{
								char w[] = "NOT(ISNULL(subdef.record_id))";
								if(qp->n->content.boperator.r->content.multileaf.firstkeyword->kword[0] == '0')
									strcpy(w, "ISNULL(subdef.record_id)");
								if(*(qp->psortField))
								{
									// ===== OK =====
									sprintf(sql, "SELECT record_id, record.coll_id"
											", prop.value AS skey"
											// ", NULL AS hitstart, NULL AS hitlen, NULL AS iw"
											// ", NULL AS sha256"
											" FROM (%s LEFT JOIN subdef USING(record_id))"
											" INNER JOIN prop USING(record_id)"
											" WHERE %s AND subdef.name='%s' AND prop.name='%s'"
											, qp->sqltrec
											, w
											, fname
											, *(qp->psortField)
											);
								}
								else
								{
									// ===== OK =====
									sprintf(sql, "SELECT record_id, record.coll_id"
											// ", NULL AS skey"
											// ", NULL AS hitstart, NULL AS hitlen, NULL AS iw"
											// ", NULL AS sha256"
											" FROM %s"
											" LEFT JOIN subdef USING(record_id)"
											" WHERE %s AND subdef.name='%s'" // ORDER BY record_id DESC"
											, qp->sqltrec
											, w
											, fname
											);
								}
							}

							// champ technique recordstatus ?
							if(!sql[0] && strcmp(fname, "recordstatus") == 0)
							{
#ifdef __no_WIN__
								char buff_and[64];
								char *val;
								xINT64 mask_and;
								//	unsigned long long mask_xor;
								for(val = qp->n->content.boperator.r->content.multileaf.firstkeyword->kword; *val; val++)
								{
									switch(*val)
									{
										case '0':
											mask_and = (mask_and << 1) && 0xFFFFFFFFFFFFFFFF;
											//				mask_xor = (mask_xor<<1) && 0x0000000000000000;
											break;
										case '1':
											mask_and = (mask_and << 1) && 0xFFFFFFFFFFFFFFFF;
											//				mask_xor = (mask_xor<<1) && 0x0000000000000001;
											break;
										default:
											mask_and = (mask_and << 1) && 0xFFFFFFFFFFFFFFFE;
											//				mask_xor = (mask_xor<<1) && 0x0000000000000000;
											break;
									}
								}
#else
								// pas de support direct 64 bits
								char buff_and[] = "0x????????????????";
								char buff_xor[] = "0x????????????????";
								int l, q, b;
								unsigned char h_and, h_xor;
								l = strlen(qp->n->content.boperator.r->content.multileaf.firstkeyword->kword) - 1;
								for(q = 15; q >= 0; q--)
								{
									h_and = h_xor = 0;
									for(b = 0; b < 4; b++)
									{
										if(l >= 0)
										{
											switch(qp->n->content.boperator.r->content.multileaf.firstkeyword->kword[l])
											{
												case '0':
													h_and |= (1 << b);
													break;
												case '1':
													h_and |= (1 << b);
													h_xor |= (1 << b);
													break;
											}
											l--;
										}
									}
									buff_and[2 + q] = (h_and > 9) ? ('a' + (h_and - 10)) : ('0' + h_and);
									buff_xor[2 + q] = (h_xor > 9) ? ('a' + (h_xor - 10)) : ('0' + h_xor);
								}
#endif
								if(*(qp->psortField))
								{
									// ===== OK =====
									sprintf(sql, "SELECT record_id, record.coll_id"
											", prop.value AS skey"
											// ", NULL AS hitstart, NULL AS hitlen, NULL AS iw"
											// ", NULL AS sha256"
											" FROM %s INNER JOIN prop USING(record_id)"
											" WHERE ((status ^ %s) & %s = 0) AND prop.name='%s'"
											, qp->sqltrec
											, buff_xor
											, buff_and
											, *(qp->psortField)
											);
								}
								else
								{
									// ===== OK =====
									sprintf(sql, "SELECT record_id, record.coll_id"
											// ", NULL AS skey"
											// ", NULL AS hitstart, NULL AS hitlen, NULL AS iw"
											// ", NULL AS sha256"
											" FROM %s"
											" WHERE ((status ^ %s) & %s = 0)"
											, qp->sqltrec
											, buff_xor
											, buff_and
											);
								}
							}

							if(!sql[0]) // it was not a technical field = it was a field of the bas (structure)
							{
								for(char *p = fname; *p; p++)
								{
									if(*p >= 'a' && *p <= 'z')
										*p += ('A' - 'a');
								}
								if(*(qp->psortField))
								{
									// ===== OK =====
									sprintf(sql, "SELECT record.record_id, record.coll_id"
											", prop.value AS skey"
											// ", NULL AS hitstart, NULL AS hitlen, NULL AS iw"
											// ", NULL AS sha256"
											" FROM (prop AS pp INNER JOIN %s USING(record_id))"
											" INNER JOIN prop USING(record_id)"
											" WHERE pp.name='%s' AND pp.value%s'%s' AND prop.name='%s'"
											, qp->sqltrec
											, fname
											, math2sql[qp->n->type - PHRASEA_OP_EQUAL]
											, qp->n->content.boperator.r->content.multileaf.firstkeyword->kword
											, *(qp->psortField)
											);
								}
								else
								{
									// ===== OK =====
									sprintf(sql, "SELECT record.record_id, record.coll_id"
											// ", NULL AS skey"
											// ", NULL AS hitstart, NULL AS hitlen, NULL AS iw"
											// ", NULL AS sha256"
											" FROM prop INNER JOIN %s USING(record_id)"
											" WHERE name='%s' AND value%s'%s'"
											, qp->sqltrec
											, fname
											, math2sql[qp->n->type - PHRASEA_OP_EQUAL]
											, qp->n->content.boperator.r->content.multileaf.firstkeyword->kword
											);
								}
							}

							qp->sqlconn->phrasea_query(sql, qp);
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
							char *fname = qp->n->content.boperator.l->content.multileaf.firstkeyword->kword;

							char *fvalue1 = NULL; // for sql : " thit.value LIKE 'xxx' OR thit.value LINK 'yyy' "
							int l1 = 1; // "\0" final
							char *fvalue2 = NULL; // for ret : " xxx | yyy "
							int l2 = 1; // "\0" final

							KEYWORD *k;
							for(k = qp->n->content.boperator.r->content.multileaf.firstkeyword; k; k = k->nextkeyword)
							{
								l = strlen(k->kword);

								l1 += 17 + l + 1; // "thit.value LIKE 'xxx'"
								l2 += l; // xxx
								if(k->nextkeyword)
								{
									l1 += 4; // " OR "
									l2 += 3; // ' | '
								}
							}
							fvalue1 = (char *) EMALLOC(l1);
							fvalue2 = (char *) EMALLOC(l2);
							l1 = l2 = 0;
							int l;
							if(fvalue1)
							{
								for(k = qp->n->content.boperator.r->content.multileaf.firstkeyword; k; k = k->nextkeyword)
								{
									l = strlen(k->kword);

									memcpy(fvalue1 + l1, "thit.value LIKE '", 17);
									l1 += 17;
									memcpy(fvalue1 + l1, k->kword, l);
									l1 += l;
									fvalue1[l1++] = '\'';
									if(k->nextkeyword)
									{
										memcpy(fvalue1 + l1, " OR ", 4);
										l1 += 4;
									}

									if(fvalue2)
									{
										memcpy(fvalue2 + l2, k->kword, l);
										l2 += l;
										if(k->nextkeyword)
										{
											memcpy(fvalue2 + l2, " | ", 3);
											l2 += 3;
										}
									}
								}
								fvalue1[l1++] = '\0';
								fvalue2[l2++] = '\0';

								if(qp->result)
								{
									add_assoc_string(qp->result, (char *) "field", qp->n->content.boperator.l->content.multileaf.firstkeyword->kword, TRUE);
									if(fvalue2)
										add_assoc_string(qp->result, (char *) "value", fvalue2, TRUE);
								}

								if(fname[0] == '*' && fname[1] == '\0')
								{
									// fieldname is '*' : avoid test on name
									if(*(qp->psortField))
									{
										// ===== OK =====
										sprintf(sql, "SELECT record_id, record.coll_id"
												", prop.value AS skey"
												", hitstart, hitlen"
												// ", NULL AS iw"
												// ", NULL AS sha256"
												" FROM (thit INNER JOIN %s USING(record_id))"
												" INNER JOIN prop USING(record_id)"
												" WHERE (%s) AND prop.name='%s'"
												, qp->sqltrec
												, fvalue1
												, *(qp->psortField)
												);
									}
									else
									{
										// ===== OK =====
										sprintf(sql, "SELECT record_id, record.coll_id"
												", NULL AS skey"
												", hitstart, hitlen"
												// ", NULL AS iw"
												// ", NULL AS sha256"
												" FROM thit INNER JOIN %s USING(record_id)"
												" WHERE (%s)"
												, qp->sqltrec
												, fvalue1
												);
									}
								}
								else
								{
									// there is a fieldname, must be included in sql
									if(*(qp->psortField))
									{
										// ===== OK =====
										sprintf(sql, "SELECT record_id, record.coll_id"
												", prop.value AS skey"
												", hitstart, hitlen"
												// ", NULL AS iw"
												// ", NULL AS sha256"
												" FROM (thit INNER JOIN %s USING(record_id))"
												" INNER JOIN prop USING(record_id)"
												" WHERE (%s) AND thit.name='%s' AND prop.name='%s'"
												, qp->sqltrec
												, fvalue1
												, fname
												, *(qp->psortField)
												);
									}
									else
									{
										// ===== OK =====
										sprintf(sql, "SELECT record_id, record.coll_id"
												", NULL AS skey"
												", hitstart, hitlen"
												// ", NULL AS iw"
												// ", NULL AS sha256"
												" FROM thit INNER JOIN %s USING(record_id)"
												" WHERE (%s) AND thit.name='%s'"
												, qp->sqltrec
												, fvalue1
												, fname
												);
									}
								}

								EFREE(fvalue1);

								qp->sqlconn->phrasea_query(sql, qp);
							}
							if(fvalue2)
								EFREE(fvalue2);
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

					Cquerytree2Parm qpl(qp->n->content.boperator.l, qp->depth + 1, qp->sqlconn, qp->sqlmutex, objl, qp->sqltrec, qp->psortField, qp->sortMethod);
					Cquerytree2Parm qpr(qp->n->content.boperator.r, qp->depth + 1, qp->sqlconn, qp->sqlmutex, objr, qp->sqltrec, qp->psortField, qp->sortMethod);

					if(!mysql_thread_safe())
					{
						querytree2((void *) &qpl);
						querytree2((void *) &qpr);
					}
					else
					{
						pthread_attr_init(&thread_attr);
						pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
						pthread_create(&threadl, &thread_attr, querytree2, (void *) &qpl); // (thread,attr,start_routine,arg)
						pthread_create(&threadr, &thread_attr, querytree2, (void *) &qpr); // (thread,attr,start_routine,arg)
						pthread_attr_destroy(&thread_attr);
						pthread_join(threadl, NULL);
						pthread_join(threadr, NULL);
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

					Cquerytree2Parm qpl(qp->n->content.boperator.l, qp->depth + 1, qp->sqlconn, qp->sqlmutex, objl, qp->sqltrec, qp->psortField, qp->sortMethod);
					Cquerytree2Parm qpr(qp->n->content.boperator.r, qp->depth + 1, qp->sqlconn, qp->sqlmutex, objr, qp->sqltrec, qp->psortField, qp->sortMethod);

					if(!mysql_thread_safe())
					{
						querytree2((void *) &qpl);
						querytree2((void *) &qpr);
					}
					else
					{
						pthread_attr_init(&thread_attr);
						pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
						pthread_create(&threadl, &thread_attr, querytree2, (void *) &qpl); // (thread,attr,start_routine,arg)
						pthread_create(&threadr, &thread_attr, querytree2, (void *) &qpr); // (thread,attr,start_routine,arg)
						pthread_attr_destroy(&thread_attr);
						pthread_join(threadl, NULL);
						pthread_join(threadr, NULL);
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

					Cquerytree2Parm qpl(qp->n->content.boperator.l, qp->depth + 1, qp->sqlconn, qp->sqlmutex, objl, qp->sqltrec, qp->psortField, qp->sortMethod);
					Cquerytree2Parm qpr(qp->n->content.boperator.r, qp->depth + 1, qp->sqlconn, qp->sqlmutex, objr, qp->sqltrec, qp->psortField, qp->sortMethod);

					if(!mysql_thread_safe())
					{
						querytree2((void *) &qpl);
						querytree2((void *) &qpr);
					}
					else
					{
						pthread_attr_init(&thread_attr);
						pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
						pthread_create(&threadl, &thread_attr, querytree2, (void *) &qpl); // (thread,attr,start_routine,arg)
						pthread_create(&threadr, &thread_attr, querytree2, (void *) &qpr); // (thread,attr,start_routine,arg)
						pthread_attr_destroy(&thread_attr);
						pthread_join(threadl, NULL);
						pthread_join(threadr, NULL);
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

					Cquerytree2Parm qpl(qp->n->content.boperator.l, qp->depth + 1, qp->sqlconn, qp->sqlmutex, objl, qp->sqltrec, qp->psortField, qp->sortMethod);
					Cquerytree2Parm qpr(qp->n->content.boperator.r, qp->depth + 1, qp->sqlconn, qp->sqlmutex, objr, qp->sqltrec, qp->psortField, qp->sortMethod);

					if(!mysql_thread_safe())
					{
						querytree2((void *) &qpl);
						querytree2((void *) &qpr);
					}
					else
					{
						pthread_attr_init(&thread_attr);
						pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
						pthread_create(&threadl, &thread_attr, querytree2, (void *) &qpl); // (thread,attr,start_routine,arg)
						pthread_create(&threadr, &thread_attr, querytree2, (void *) &qpr); // (thread,attr,start_routine,arg)
						pthread_attr_destroy(&thread_attr);
						pthread_join(threadl, NULL);
						pthread_join(threadr, NULL);
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
				add_assoc_string(qp->result, (char *) "sql", sql, true);
			if(qp->n->time_sqlQuery != -1)
				add_assoc_double(qp->result, (char *) "time_sqlQuery", qp->n->time_sqlQuery);
			if(qp->n->time_sqlStore != -1)
				add_assoc_double(qp->result, (char *) "time_sqlStore", qp->n->time_sqlStore);
			if(qp->n->time_sqlFetch != -1)
				add_assoc_double(qp->result, (char *) "time_sqlFetch", qp->n->time_sqlFetch);
//			add_assoc_long(qp->result, (char *) "nbanswers", qp->n->nbranswers);
			add_assoc_long(qp->result, (char *) "nbanswers", qp->n->answers.size());
		}
	}
	else
	{
		// zend_printf("querytree : null node\n");
	}
}

void freetree(CNODE *n)
{
	KEYWORD *k;
	if(n)
	{
		switch(n->type)
		{
			case PHRASEA_KEYLIST:
				while(n->content.multileaf.firstkeyword)
				{
					if(n->content.multileaf.firstkeyword->kword)
						EFREE(n->content.multileaf.firstkeyword->kword);
					k = n->content.multileaf.firstkeyword->nextkeyword;
					EFREE(n->content.multileaf.firstkeyword);
					n->content.multileaf.firstkeyword = k;
				}
				n->content.multileaf.lastkeyword = NULL;
				break;
				//			case PHRASEA_KEYWORD:
				//				if(n->content.leaf.kword)
				//				{
				//					EFREE(n->content.leaf.kword);
				//					n->content.leaf.kword = NULL;
				//				}
				//				break;
			case PHRASEA_OP_EQUAL:
			case PHRASEA_OP_NOTEQU:
			case PHRASEA_OP_GT:
			case PHRASEA_OP_LT:
			case PHRASEA_OP_GEQT:
			case PHRASEA_OP_LEQT:
			case PHRASEA_OP_AND:
			case PHRASEA_OP_OR:
			case PHRASEA_OP_EXCEPT:
			case PHRASEA_OP_NEAR:
			case PHRASEA_OP_BEFORE:
			case PHRASEA_OP_AFTER:
			case PHRASEA_OP_IN:
				freetree(n->content.boperator.l);
				freetree(n->content.boperator.r);
				break;
			default:
				break;
		}
		delete n;
		// zend_printf("n ");
	}
}

