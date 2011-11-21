#ifndef PHRASEA_TYPES_H
#define PHRASEA_TYPES_H 1
/*
#include <set>
#include <string>

#include "sql.h"
 */

#ifdef __ppc__	// os x
#define PH_INT32 int
#define PH_INT64 long long
#define PH_ATOI64(s)	atoll(s)
#else 
#ifdef WIN32	// windows
#define PH_INT32 int
#define PH_INT64 __int64
#define PH_ATOI64(s)	_atoi64(s)
#else	// linux ?
#define PH_INT32 int
#define PH_INT64 __int64
#define PH_ATOI64(s)	_atoi64(s)
#endif
#endif


#ifndef true
#define true TRUE
#endif

#ifndef false
#define false FALSE
#endif

#ifndef NONULLSTRING
#define NONULLSTRING(x) ((x)?(x):"")
#endif

#define PHRASEA_OP_NULL -1
#define PHRASEA_OP_OR 1
#define PHRASEA_OP_AND 2
#define PHRASEA_KW_ALL 3
#define PHRASEA_KW_LAST 4
#define PHRASEA_OP_EXCEPT 5
#define PHRASEA_OP_NEAR 6
#define PHRASEA_OP_BEFORE 7
#define PHRASEA_OP_AFTER 8
#define PHRASEA_OP_IN 9

#define PHRASEA_OP_EQUAL 10
#define PHRASEA_OP_NOTEQU 11
#define PHRASEA_OP_GT 12
#define PHRASEA_OP_LT 13
#define PHRASEA_OP_GEQT 14
#define PHRASEA_OP_LEQT 15
#define PHRASEA_OP_COLON 16

#define PHRASEA_KEYLIST 17

#define PHRASEA_KW_FIRST 18

#define PHRASEA_NBR_OPS 19		// max nbr of ops (to size the array of sqls)


#define MAXHITLENGTH 14

#define DEFAULTLAST 12

enum
{
	SQL_SB_0 = 0, SQL_SB_1 = 1
};

enum
{
	PHRASEA_MULTIDOC_DOCONLY = 0, PHRASEA_MULTIDOC_REGONLY = 1
};

enum
{
	SQL_SORT_0 = 0, SQL_SORT_1 = 1
};

enum
{
	PHRASEA_ORDER_DESC = 0, PHRASEA_ORDER_ASC = 1, PHRASEA_ORDER_ASK = 2
};

enum
{
	SORTMETHOD_STR = 0, SORTMETHOD_INT = 1
};

typedef struct collid_pair
{
	long local_base_id; // id de base locale (champ 'base_id' dans la table 'bas' de la base 'phrasea' locale)
	long distant_coll_id; // id de collection distante (champ 'coll_id' dans la table 'coll' de la base distante)
}
COLLID_PAIR;

class CSPOT
{
public:
	int start;
	int len;
	CSPOT *_nextspot;

	CSPOT(int start, int len)
	{
		this->_nextspot = NULL;
		this->start = start;
		this->len = len;
	}
};

class CHIT
{
public:
	int iws;
	int iwe;
	CHIT *nexthit;

	CHIT(int iw)
	{
		this->nexthit = NULL;
		this->iws = this->iwe = iw;
	}

	CHIT(int iws, int iwe)
	{
		this->nexthit = NULL;
		this->iws = iws;
		this->iwe = iwe;
	}
};

class CSHA
{
public:
	unsigned char _v[65];
public:

	CSHA()
	{
		memset(this->_v, 0, sizeof (this->_v));
	}

	CSHA(const unsigned char *v)
	{
		memset(this->_v, 0, sizeof (this->_v));
		if(v)
		{
			unsigned char *p = this->_v;
			for(register int i = 0; i < 64 && *v; i++)
				*p++ = *v++;
			*p = '\0';
		}
	}

	bool operator==(const CSHA &rhs)
	{
		return (strcmp((const char *) (this->_v), (const char *) (rhs._v)) == 0);
	}

	bool operator!=(const CSHA &rhs)
	{
		return (!(*this == rhs));
	}

	operator const char *()
	{
		return ((const char *) (this->_v));
	}

	bool operator<(const CSHA &rhs)
	{
		return (strcmp((const char *) (this->_v), (const char *) (rhs._v)) < 0);
	}

	bool operator<=(const CSHA &rhs)
	{
		return (strcmp((const char *) (this->_v), (const char *) (rhs._v)) <= 0);
	}

	bool operator>(const CSHA &rhs)
	{
		return (strcmp((const char *) (this->_v), (const char *) (rhs._v)) > 0);
	}

	bool operator>=(const CSHA &rhs)
	{
		return (strcmp((const char *) (this->_v), (const char *) (rhs._v)) >= 0);
	}
};

class CANSWER
{
public:
	//	enum { SORTTYPE_UNKNOWN = -1, SORTTYPE_TEXT = 1, SORTTYPE_INT = 2, SORTTYPE_DATE = 4 };
	int rid;
	//		int prid;
	int cid;
	//		unsigned long long llstatus;	// 64 bits
	CSHA *sha2;

	struct
	{
		std::string *s;
		long long l;
	} sortkey;
	CHIT *firsthit, *lasthit;
	CSPOT *firstspot, *lastspot;
	int nspots;

	void freeHits()
	{
		CHIT *h;
		while(this->firsthit)
		{
			h = this->firsthit->nexthit;
			delete(this->firsthit);
			this->firsthit = h;
		}
		this->lasthit = NULL;
	}

	CANSWER()
	{
		this->sha2 = NULL;
		this->sortkey.s = NULL;
		this->firsthit = this->lasthit = NULL;
		this->firstspot = this->lastspot = NULL;
		//zend_printf("new CANSWER @=%p (sz=%d)\n", this, sizeof(CANSWER));
	}

	~CANSWER()
	{
		if(this->sha2)
			delete(this->sha2);
		if(this->sortkey.s)
			delete(this->sortkey.s);
		this->freeHits();
		CSPOT *s;
		while(this->firstspot)
		{
			s = this->firstspot->_nextspot;
			delete(this->firstspot);
			this->firstspot = s;
		}
	}
	//		void printSpots()
	//		{
	//			zend_printf("@%p : rid=%d , firsthit=%p , lasthit=%p\n\tSPOTS :", this, this->rid, this->firsthit, this->lasthit);
	//			for(CSPOT *s=this->firstspot; s; s=s->_nextspot)
	//				zend_printf(" [%p: %d, %d -&gt;%p]", s, s->start, s->len, s->_nextspot);
	//			zend_printf("\n\tHITS :");
	//			for(CHIT *h=this->firsthit; h; h=h->nexthit)
	//				zend_printf(" [%p: %d, %d -&gt;%p]", h, h->iws, h->iwe, h->nexthit);
	//			zend_printf("\n");
	//		}
};
typedef CANSWER *PCANSWER;

class PCANSWERCOMPRID_DESC
{
public:

	bool operator() (const PCANSWER& lhs, const PCANSWER& rhs) const
	{
		return lhs->rid > rhs->rid;
	}
};

/*
class PCANSWERCOMP_INT:PCANSWERCOMP
{
	public:
		bool operator() (const PCANSWER lhs, const PCANSWER rhs) const
		{
			return(lhs->sortkey.l > rhs->sortkey.l);
		}
};

class PCANSWERCOMP_STR:PCANSWERCOMP
{
	public:
		bool operator() (const PCANSWER lhs, const PCANSWER rhs) const
		{
			return( (lhs->sortkey.s->compare(*(rhs->sortkey.s))) < 0 );
		}
};

class PCANSWERCOMP_SHA:PCANSWERCOMP
{
	public:
		bool operator() (const PCANSWER lhs, const PCANSWER rhs) const
		{
			return( lhs->sha2 < rhs->sha2 );
		}
};
 */


typedef struct keyword
{
	char *kword;
	struct keyword *nextkeyword;
}
KEYWORD;

class CNODE
{
private:
	;
public:

	CNODE(int type)
	{
		this->type = type;
		this->nbranswers = 0;
		this->nleaf = 0;
		this->isempty = FALSE;
		this->time_C = -1;
		this->time_sqlQuery = this->time_sqlStore = this->time_sqlFetch = -1;
	}

	int type;
	bool isempty;
	std::set<PCANSWER, PCANSWERCOMPRID_DESC> answers;
	int nbranswers;
	int nleaf;
	double time_C;
	double time_sqlQuery, time_sqlStore, time_sqlFetch;

	union
	{

		struct
		{
			int v;
		} numparm;

		struct
		{
			char *kword;
		} leaf;

		struct
		{
			KEYWORD *firstkeyword, *lastkeyword;
		} multileaf;

		struct
		{
			CNODE *l;
			CNODE *r;
			int numparm;
		} boperator;

		struct
		{
			char *fieldname;
			char *strvalue;
			double dblvalue;
		} voperator;
	}
	content;
};



// les structures 'calqu?es' des donn?es en cache

typedef struct cache_spot
{
	unsigned int start;
	unsigned int len;
}
CACHE_SPOT;

typedef struct cache_answer
{
	int rid;
	int bid;
	unsigned int spots_index;
	unsigned int nspots;
}
CACHE_ANSWER;

// impl?mentation d'une resource "phrasea_connection" : en fait une connexion ? pg ou mysql
// http://groups.google.fr/groups?q=zend_register_list_destructors_ex+persistent&hl=fr&lr=&ie=UTF-8&selm=cvshelly1039456453%40cvsserver&rnum=8

typedef struct _php_phrasea_conn
{

	union
	{
		MYSQL mysql_conn;
	}
	sqlconn;
	int mysql_active_result_id;
}
PHP_PHRASEA_CONN;

#endif
