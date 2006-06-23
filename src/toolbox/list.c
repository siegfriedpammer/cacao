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

   $Id: list.c 5049 2006-06-23 12:07:26Z twisti $

*/


#include "config.h"

#include <stdlib.h>

#include "vm/types.h"

#include "mm/memory.h"

#if defined(ENABLE_THREADS)
# include "threads/native/threads.h"
#endif

#include "toolbox/list.h"
#include "vm/builtin.h"


/* list_create *****************************************************************

   Allocates a new list and initializes the lock object.

*******************************************************************************/

list *list_create(s4 nodeoffset)
{
	list *l;

	l = NEW(list);

#if defined(ENABLE_THREADS)
	lock_init_object_lock((java_objectheader *) l);
#endif

	l->first      = NULL;
	l->last       = NULL;
	l->nodeoffset = nodeoffset;

	return l;
}


void list_add_first(list *l, void *element)
{
	listnode *ln;

	ln = (listnode *) (((u1 *) element) + l->nodeoffset);

	BUILTIN_MONITOR_ENTER(l);

	if (l->first) {
		ln->prev       = NULL;
		ln->next       = l->first;
		l->first->prev = ln;
		l->first       = ln;
	}
	else {
		ln->prev = NULL;
		ln->next = NULL;
		l->last  = ln;
		l->first = ln;
	}

	BUILTIN_MONITOR_EXIT(l);
}


/* list_add_last ***************************************************************

   Adds the element as last element.

*******************************************************************************/

void list_add_last(list *l, void *element)
{
	listnode *ln;

	ln = (listnode *) (((u1 *) element) + l->nodeoffset);

	BUILTIN_MONITOR_ENTER(l);

	if (l->last) {
		ln->prev      = l->last;
		ln->next      = NULL;
		l->last->next = ln;
		l->last       = ln;
	}
	else {
		ln->prev = NULL;
		ln->next = NULL;
		l->last  = ln;
		l->first = ln;
	}

	BUILTIN_MONITOR_EXIT(l);
}


/* list_add_list_unsynced ******************************************************

   Adds the element as last element but does NO locking!

   ATTENTION: This function is used during bootstrap.  DON'T USE IT!!!

*******************************************************************************/

void list_add_last_unsynced(list *l, void *element)
{
	listnode *ln;

	ln = (listnode *) (((u1 *) element) + l->nodeoffset);

	if (l->last) {
		ln->prev      = l->last;
		ln->next      = NULL;
		l->last->next = ln;
		l->last       = ln;
	}
	else {
		ln->prev = NULL;
		ln->next = NULL;
		l->last  = ln;
		l->first = ln;
	}
}


/* list_add_before *************************************************************

   Adds the element newelement to the list l before element.

   [ A ] <-> [ newn ] <-> [ n ] <-> [ B ]

*******************************************************************************/

void list_add_before(list *l, void *element, void *newelement)
{
	listnode *ln;
	listnode *newln;

	ln    = (listnode *) (((u1 *) element) + l->nodeoffset);
	newln = (listnode *) (((u1 *) newelement) + l->nodeoffset);

	BUILTIN_MONITOR_ENTER(l);

	/* set the new links */

	newln->prev = ln->prev;
	newln->next = ln;

	if (newln->prev)
		newln->prev->next = newln;

	ln->prev = newln;

	/* set list's first and last if necessary */

	if (l->first == ln)
		l->first = newln;

	if (l->last == ln)
		l->last = newln;

	BUILTIN_MONITOR_EXIT(l);
}


void list_remove(list *l, void *element)
{
	listnode *ln;

	ln = (listnode *) (((u1 *) element) + l->nodeoffset);
	
	BUILTIN_MONITOR_ENTER(l);

	if (ln->next)
		ln->next->prev = ln->prev;
	else
		l->last = ln->prev;

	if (ln->prev)
		ln->prev->next = ln->next;
	else
		l->first = ln->next;

	ln->next = NULL;
	ln->prev = NULL;

	BUILTIN_MONITOR_EXIT(l);
}

 
void *list_first(list *l)
{
	void *el;

	BUILTIN_MONITOR_ENTER(l);

	if (l->first == NULL)
		el = NULL;
	else
		el = ((u1 *) l->first) - l->nodeoffset;

	BUILTIN_MONITOR_EXIT(l);

	return el;
}


void *list_last(list *l)
{
	void *el;

	BUILTIN_MONITOR_ENTER(l);

	if (l->last == NULL)
		el = NULL;
	else
		el = ((u1 *) l->last) - l->nodeoffset;

	BUILTIN_MONITOR_EXIT(l);

	return el;
}


void *list_next(list *l, void *element)
{
	listnode *ln;
	void     *el;

	ln = (listnode *) (((u1 *) element) + l->nodeoffset);

	BUILTIN_MONITOR_ENTER(l);

	if (ln->next == NULL)
		el = NULL;
	else
		el = ((u1 *) ln->next) - l->nodeoffset;

	BUILTIN_MONITOR_EXIT(l);

	return el;
}

	
void *list_prev(list *l, void *element)
{
	listnode *ln;
	void     *el;

	ln = (listnode *) (((u1 *) element) + l->nodeoffset);

	BUILTIN_MONITOR_ENTER(l);

	if (ln->prev == NULL)
		el = NULL;
	else
		el = ((u1 *) ln->prev) - l->nodeoffset;

	BUILTIN_MONITOR_EXIT(l);

	return el;
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
