/************************** toolbox/list.c *************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Not documented, see chain.h.

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1996/10/03

*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "list.h"


void list_init (list *l, int nodeoffset)
{
	l->first = NULL;
	l->last = NULL;
	l->nodeoffset = nodeoffset;
}

void list_addlast (list *l, void *element)
{
	listnode *n = (listnode*) ( ((char*) element) + l->nodeoffset );

	if (l->last) {
		n->prev = l->last;
		n->next = NULL;
		l->last->next = n;
		l->last = n;
		}
	else {
		n->prev = NULL;
		n->next = NULL;
		l->last = n;
		l->first = n;
		}
}

void list_addfirst (list *l, void *element)
{
	listnode *n = (listnode*) ( ((char*) element) + l->nodeoffset );

	if (l->first) {
		n->prev = NULL;
		n->next = l->first;
		l->first->prev = n;
		l->first = n;
		}
	else {
		n->prev = NULL;
		n->next = NULL;
		l->last = n;
		l->first = n;
		}
}


void list_remove (list *l, void *element)
{
	listnode *n = (listnode*) ( ((char*) element) + l->nodeoffset );
	
	if (n->next) n->next->prev = n->prev;
	        else l->last = n->prev;
	if (n->prev) n->prev->next = n->next;
	        else l->first = n->next;
}

 
void *list_first (list *l)
{
	if (!l->first) return NULL;
	return ((char*) l->first) - l->nodeoffset;
}


void *list_last (list *l)
{
	if (!l->last) return NULL;
	return ((char*) l->last) - l->nodeoffset;
}


void *list_next (list *l, void *element)
{
	listnode *n;
	n = (listnode*) ( ((char*) element) + l->nodeoffset );
	if (!n->next) return NULL;
	return ((char*) n->next) - l->nodeoffset;
}

	
void *list_prev (list *l, void *element)
{
	listnode *n;
	n = (listnode*) ( ((char*) element) + l->nodeoffset );
	if (!n->prev) return NULL;
	return ((char*) n->prev) - l->nodeoffset;
}

