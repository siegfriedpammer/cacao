/************************* toolbox/chain.c *************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Not documented, see chain.h.

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1996/10/03

*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "global.h"
#include "memory.h"
#include "chain.h"



chain *chain_new ()
{
	chain *c;
	
	c = NEW (chain);
	c -> usedump = 0;
	c -> first = NULL;
	c -> last = NULL;
	c -> active = NULL;

	return c;
}

chain *chain_dnew ()
{
	chain *c;
	
	c = DNEW (chain);
	c -> usedump = 1;
	c -> first = NULL;
	c -> last = NULL;
	c -> active = NULL;

	return c;
}


void chain_free (chain *c)
{
	chainlink *l;

	assert (! c->usedump);

	l = c->first;
	while (l) {
		chainlink *nextl = l->next;
		
		FREE (l, chainlink);
		l = nextl;	
		}
	
	FREE (c, chain);
}


void chain_addafter(chain *c, void *element)
{
	chainlink *active,*newlink;

    active = c->active;

	if (c -> usedump) newlink = DNEW (chainlink);
	else              newlink = NEW (chainlink);

	newlink -> element = element;
	
	if (active) {
		newlink -> next = active -> next;
		newlink -> prev = active;
		
		active -> next = newlink;
		if (newlink -> next) newlink -> next -> prev = newlink;
		                else c -> last = newlink;
		}
	else {	
		newlink -> next = NULL;
		newlink -> prev = NULL;

		c -> active = newlink;	
		c -> first = newlink;
		c -> last = newlink;
		}
}


void chain_addbefore(chain *c, void *element)
{
	chainlink *active,*newlink;

    active = c->active;

	if (c -> usedump) newlink = DNEW (chainlink);
	else              newlink = NEW (chainlink);
	
	newlink -> element = element;
	
	if (active) {
		newlink -> next = active;
		newlink -> prev = active -> prev;
		
		active -> prev = newlink;
		if (newlink -> prev) newlink -> prev -> next = newlink;
		else                 c -> first = newlink;
		}
	else {	
		newlink -> next = NULL;
		newlink -> prev = NULL;

		c -> active = newlink;	
		c -> first = newlink;
		c -> last = newlink;
		}
}

void chain_addlast (chain *c, void *e)
{
	chain_last (c);
	chain_addafter (c, e);
}

void chain_addfirst (chain *c, void *e)
{
	chain_first (c);
	chain_addbefore (c, e);
}


void chain_remove (chain *c)
{
	chainlink *active;
	
	active = c -> active;
	assert (active);

	if (active -> next) {
		active -> next -> prev = active -> prev;
		}
	else {
		c -> last = active -> prev;
		}
	
	if (active -> prev) {
		active -> prev -> next = active -> next;
		}
	else {
		c -> first = active -> next;
		}


	if (active->prev) 
		c -> active = active->prev;
	else
		c -> active = active->next;

	if (! c -> usedump) FREE (active, chainlink);
}


void *chain_remove_go_prev (chain *c)
{
	chain_remove (c);
	return chain_this (c);
}



void chain_removespecific(chain *c, void *e)
{
	void *ce;
	
	ce = chain_first (c);
	while (ce) {
		if (e == ce) { chain_remove (c); return; }
        ce = chain_next (c);		
		}
		
}

void *chain_next(chain *c)
{
	chainlink *active;
	
	active = c -> active;
	if (!active) return NULL;
	
	if (active -> next) {
		c -> active = active -> next;
		return c -> active -> element;
		}
		
	return NULL;
}

void *chain_prev(chain *c)
{
	chainlink *active;
	
	active = c -> active;
	if (!active) return NULL;
	
	if (active -> prev) {
		c -> active = active -> prev;
		return c -> active -> element;
		}

	return NULL;
}


void *chain_this(chain *c)
{
	if (c -> active) return c -> active -> element;
	return NULL;
}


void *chain_first(chain *c)
{
	c -> active = c -> first;
	if (c -> active) return c -> active -> element;
	return NULL;
}

void *chain_last(chain *c)
{
	c -> active = c -> last;
	if (c -> active) return c -> active -> element;;
	return NULL;
}

