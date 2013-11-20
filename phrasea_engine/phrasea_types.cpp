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


bool PCANSWERCOMPRID_DESC::operator() (const PCANSWER& lhs, const PCANSWER& rhs) const
{
	return lhs->rid > rhs->rid;
}


CNODE::CNODE(int type)
{
	this->type = type;
	this->nleaf = 0;
	this->isempty = false;
	this->time_C = -1;
	this->time_sqlQuery = this->time_sqlStore = this->time_sqlFetch = -1;
	this->time_connect = this->time_createtmp = this->time_inserttmp = -1;

//	this->n = 0;
}

