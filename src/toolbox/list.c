/* toolbox/list.c - 

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   Institut f. Computersprachen, TU Wien
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser, M. Probst,
   S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich,
   J. Wenninger

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Reinhard Grafl

   $Id: list.c 1621 2004-11-30 13:06:55Z twisti $

*/


#include <stdlib.h>

#include "toolbox/list.h"


void list_init(list *l, int nodeoffset)
{
	l->first = NULL;
	l->last = NULL;
	l->nodeoffset = nodeoffset;
}


void list_addlast(list *l, void *element)
{
	listnode *n = (listnode*) (((char*) element) + l->nodeoffset);

	if (l->last) {
		n->prev = l->last;
		n->next = NULL;
		l->last->next = n;
		l->last = n;

	} else {
		n->prev = NULL;
		n->next = NULL;
		l->last = n;
		l->first = n;
	}
}


void list_addfirst(list *l, void *element)
{
	listnode *n = (listnode*) (((char*) element) + l->nodeoffset);

	if (l->first) {
		n->prev = NULL;
		n->next = l->first;
		l->first->prev = n;
		l->first = n;

	} else {
		n->prev = NULL;
		n->next = NULL;
		l->last = n;
		l->first = n;
	}
}


void list_remove(list *l, void *element)
{
	listnode *n = (listnode*) (((char*) element) + l->nodeoffset);
	
	if (n->next) {
		n->next->prev = n->prev;

	} else {
		l->last = n->prev;
	}

	if (n->prev) {
		n->prev->next = n->next;

	} else {
		l->first = n->next;
	}

	n->next = NULL;
	n->prev = NULL;
}

 
void *list_first(list *l)
{
	if (!l->first)
		return NULL;

	return ((char*) l->first) - l->nodeoffset;
}


void *list_last(list *l)
{
	if (!l->last)
		return NULL;

	return ((char*) l->last) - l->nodeoffset;
}


void *list_next(list *l, void *element)
{
	listnode *n;

	n = (listnode*) (((char*) element) + l->nodeoffset);

	if (!n->next)
		return NULL;

	return ((char*) n->next) - l->nodeoffset;
}

	
void *list_prev(list *l, void *element)
{
	listnode *n;

	n = (listnode*) (((char*) element) + l->nodeoffset);

	if (!n->prev)
		return NULL;

	return ((char*) n->prev) - l->nodeoffset;
}


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
