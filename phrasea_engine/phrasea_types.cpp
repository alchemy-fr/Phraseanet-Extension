#include <set>
#include <string>

#include "sql.h"
#include "phrasea_types.h"


void CSPOT::CSPOT()
{
	this->nextspot = NULL;
}

void CHIT::CHIT()
{
	this->nexthit = NULL;
}


void CSHA::CSHA()
		{
			memset(this->_v, 0, sizeof(this->_v));
		}
void CSHA::CSHA(const unsigned char *v)
{
	memset(this->_v, 0, sizeof(this->_v));
	if(v)
	{
		unsigned char *p=this->_v;
		for(register int i=0 ; i<64 && *v; i++)
			*p++ = *v++;
		*p = '\0';
	}
}
bool CSHA::operator==(const CSHA &rhs)
{
	return(strcmp((const char *)(this->_v), (const char *)(rhs._v))==0);
}
bool CSHA::operator!=(const CSHA &rhs)
{
	return(!(*this==rhs));
}
const char *CSHA::operator const char *()
{
	return((const char *)(this->_v));
}


void CANSWER::freeHits()
{
	CHIT *h;
	while(this->firsthit)
	{
		h = this->firsthit->nexthit;
		delete(this->firsthit);
		this->firsthit = h;
	}
}
void CANSWER::CANSWER()
{
	this->sha2 = NULL;
	this->sorttyp = SORTTYPE_UNKNOWN;
	// this->sortkey = NULL;
	this->firsthit = this->lasthit = NULL;
	this->firstspot = this->lastspot = NULL;
	//	this->nextanswer = NULL;
}
void CANSWER::~CANSWER()
{
	if(this->sha2)
		delete(this->sha2);
	if(this->sorttyp == SORTTYPE_TEXT && this->sortkey.s)
		delete(this->sortkey.s);
	this->freeHits();
	CSPOT *s;
	while(this->firstspot)
	{
		s = this->firstspot->nextspot;
		delete(this->firstspot);
		this->firstspot = s;
	}
}


bool PCANSWERCOMPRID_DESC::operator() (const PCANSWER& lhs, const PCANSWER& rhs) const
{
	return lhs->rid > rhs->rid;
}

bool PCANSWERCOMP::operator() (const PCANSWER& lhs, const PCANSWER& rhs) const
{
//			return lhs->rid > rhs->rid;
	if(lhs->sorttyp != rhs->sorttyp)
		return false;
	else if(lhs->sorttyp==CANSWER::SORTTYPE_INT || lhs->sorttyp==CANSWER::SORTTYPE_DATE)
		return(lhs->sortkey.l > rhs->sortkey.l);
	else if(lhs->sorttyp==CANSWER::SORTTYPE_TEXT && lhs->sortkey.s && rhs->sortkey.s)
		return( (lhs->sortkey.s->compare(*(rhs->sortkey.s))) < 0 );
	else
		return lhs->rid > rhs->rid;
}


void CNODE::CNODE(int type)
{
	this->type = type;
	this->nbranswers = 0;
	this->nleaf = 0;
	this->isempty = FALSE;
	this->time_C = -1;
	this->time_sqlQuery = this->time_sqlStore = this->time_sqlFetch = -1;
}

