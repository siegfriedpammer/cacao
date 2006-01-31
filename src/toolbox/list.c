/* src/toolbox/list.c - double linked list

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Contact: cacao@cacaojvm.org

   Authors: Reinhard Grafl

   $Id: list.c 4394 2006-01-31 22:27:23Z twisti $

*/


#include "config.h"

#include <stdlib.h>

#include "vm/types.h"

#include "toolbox/list.h"


void list_init(list *l, int nodeoffset)
{
	l->first = NULL;
	l->last = NULL;
	l->nodeoffset = nodeoffset;
}


void list_addfirst(list *l, void *element)
{
	listnode *n = (listnode *) (((u1 *) element) + l->nodeoffset);

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


void list_addlast(list *l, void *element)
{
	listnode *n = (listnode *) (((u1 *) element) + l->nodeoffset);

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


/* list_add_before *************************************************************

   Adds the element newelement to the list l before element.

   [ A ] <-> [ newn ] <-> [ n ] <-> [ B ]

*******************************************************************************/

void list_add_before(list *l, void *element, void *newelement)
{
	listnode *n    = (listnode *) (((u1 *) element) + l->nodeoffset);
	listnode *newn = (listnode *) (((u1 *) newelement) + l->nodeoffset);

	/* set the new links */

	newn->prev = n->prev;
	newn->next = n;

	if (newn->prev)
		newn->prev->next = newn;

	n->prev = newn;

	/* set list's first and last if necessary */

	if (l->first == n)
		l->first = newn;

	if (l->last == n)
		l->last = newn;
}


void list_remove(list *l, void *element)
{
	listnode *n = (listnode *) (((u1 *) element) + l->nodeoffset);
	
	if (n->next)
		n->next->prev = n->prev;
	else
		l->last = n->prev;

	if (n->prev)
		n->prev->next = n->next;
	else
		l->first = n->next;

	n->next = NULL;
	n->prev = NULL;
}

 
void *list_first(list *l)
{
	if (!l->first)
		return NULL;

	return ((u1 *) l->first) - l->nodeoffset;
}


void *list_last(list *l)
{
	if (!l->last)
		return NULL;

	return ((u1 *) l->last) - l->nodeoffset;
}


void *list_next(list *l, void *element)
{
	listnode *n;

	n = (listnode *) (((u1 *) element) + l->nodeoffset);

	if (!n->next)
		return NULL;

	return ((u1 *) n->next) - l->nodeoffset;
}

	
void *list_prev(list *l, void *element)
{
	listnode *n;

	n = (listnode *) (((u1 *) element) + l->nodeoffset);

	if (!n->prev)
		return NULL;

	return ((u1 *) n->prev) - l->nodeoffset;
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
