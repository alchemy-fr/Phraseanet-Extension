#include "base_header.h"


CSPOT::CSPOT(int start, int len)
{
	this->_nextspot = NULL;
	this->start = start;
	this->len = len;
}



CHIT::CHIT(int iw)
{
	this->nexthit = NULL;
	this->iws = this->iwe = iw;
}

CHIT::CHIT(int iws, int iwe)
{
	this->nexthit = NULL;
	this->iws = iws;
	this->iwe = iwe;
}



CSHA::CSHA()
{
	memset(this->_v, 0, sizeof (this->_v));
}

CSHA::CSHA(const unsigned char *v)
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

bool CSHA::operator==(const CSHA &rhs)
{
	return (strcmp((const char *) (this->_v), (const char *) (rhs._v)) == 0);
}

bool CSHA::operator!=(const CSHA &rhs)
{
	return (!(*this == rhs));
}

CSHA::operator const char *() const
{
	return ((const char *) (this->_v));
}

bool CSHA::operator<(const CSHA &rhs)
{
	return (strcmp((const char *) (this->_v), (const char *) (rhs._v)) < 0);
}

bool CSHA::operator<=(const CSHA &rhs)
{
	return (strcmp((const char *) (this->_v), (const char *) (rhs._v)) <= 0);
}

bool CSHA::operator>(const CSHA &rhs)
{
	return (strcmp((const char *) (this->_v), (const char *) (rhs._v)) > 0);
}

bool CSHA::operator>=(const CSHA &rhs)
{
	return (strcmp((const char *) (this->_v), (const char *) (rhs._v)) >= 0);
}



CANSWER::CANSWER()
{
	this->sha2 = NULL;
	this->sortkey.s = NULL;
	this->firsthit = this->lasthit = NULL;
	this->firstspot = this->lastspot = NULL;
}

CANSWER::~CANSWER()
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

void CANSWER::addSpot(int start, int len)
{
	CSPOT *c;
	for(c=this->firstspot; c; c=c->_nextspot)
	{
		if(c->start==start && c->len==len)
		{
			// already on list, forget
			return;
		}
	}
	// add to list
	if( (c = new CSPOT(start, len)) )
	{
		if(!this->firstspot)
			this->firstspot = c;
		else
			this->lastspot->_nextspot = c;
		this->lastspot = c;
	}
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
	this->lasthit = NULL;
}

//		void CANSWER::printSpots()
//		{
//			zend_printf("@%p : rid=%d , firsthit=%p , lasthit=%p\n\tSPOTS :", this, this->rid, this->firsthit, this->lasthit);
//			for(CSPOT *s=this->firstspot; s; s=s->_nextspot)
//				zend_printf(" [%p: %d, %d -&gt;%p]", s, s->start, s->len, s->_nextspot);
//			zend_printf("\n\tHITS :");
//			for(CHIT *h=this->firsthit; h; h=h->nexthit)
//				zend_printf(" [%p: %d, %d -&gt;%p]", h, h->iws, h->iwe, h->nexthit);
//			zend_printf("\n");
//		}



bool PCANSWERCOMPRID_DESC::operator() (const PCANSWER& lhs, const PCANSWER& rhs) const
{
	return lhs->rid > rhs->rid;
}

/*
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
*/


CNODE::CNODE(int type)
{
	this->type = type;
	this->nleaf = 0;
	this->isempty = false;
	this->time_C = -1;
	this->time_sqlQuery = this->time_sqlStore = this->time_sqlFetch = -1;
}

