/* src/toolbox/list.c - double linked list

   Copyright (C) 1996-2005, 2006, 2007, 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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

*/


#include "config.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "mm/memory.h"

#include "threads/mutex.hpp"

#include "toolbox/list.h"


/* list_create *****************************************************************

   Allocates a new list and initializes the lock object.

*******************************************************************************/

list_t *list_create(int nodeoffset)
{
	list_t *l;

	l = NEW(list_t);

	l->mutex      = Mutex_new();
	l->first      = NULL;
	l->last       = NULL;
	l->nodeoffset = nodeoffset;
	l->size       = 0;

	return l;
}


/* list_free *******************************************************************

   Free a list.

*******************************************************************************/

void list_free(list_t *l)
{
	assert(l != NULL);

	Mutex_delete(l->mutex);

	FREE(l, list_t);
}


/* list_create_dump ************************************************************

   Allocates a new list on the dump memory.

   ATTENTION: This list does NOT initialize the locking object!!!

*******************************************************************************/

list_t *list_create_dump(int nodeoffset)
{
	list_t *l;

	l = DNEW(list_t);

	l->mutex      = NULL;
	l->first      = NULL;
	l->last       = NULL;
	l->nodeoffset = nodeoffset;
	l->size       = 0;

	return l;
}


/* list_lock *******************************************************************

   Locks the list.

*******************************************************************************/

void list_lock(list_t *l)
{
	Mutex_lock(l->mutex);
}


/* list_unlock *****************************************************************

   Unlocks the list.

*******************************************************************************/

void list_unlock(list_t *l)
{
	Mutex_unlock(l->mutex);
}


/* list_add_first **************************************************************

   Adds the element as first element.

*******************************************************************************/

void list_add_first(list_t *l, void *element)
{
	listnode_t *ln;

	ln = (listnode_t *) (((uint8_t *) element) + l->nodeoffset);

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

	/* Increase number of elements. */

	l->size++;
}


/* list_add_last ***************************************************************

   Adds the element as last element.

*******************************************************************************/

void list_add_last(list_t *l, void *element)
{
	listnode_t *ln;

	ln = (listnode_t *) (((uint8_t *) element) + l->nodeoffset);

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

	/* Increase number of elements. */

	l->size++;
}


/* list_add_before *************************************************************

   Adds the element newelement to the list l before element.

   [ A ] <-> [ newn ] <-> [ n ] <-> [ B ]

*******************************************************************************/

void list_add_before(list_t *l, void *element, void *newelement)
{
	listnode_t *ln;
	listnode_t *newln;

	ln    = (listnode_t *) (((uint8_t *) element) + l->nodeoffset);
	newln = (listnode_t *) (((uint8_t *) newelement) + l->nodeoffset);

	/* Set the new links. */

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

	/* Increase number of elements. */

	l->size++;
}


/* list_remove ***************************************************************

   Removes the element.

*******************************************************************************/

void list_remove(list_t *l, void *element)
{
	listnode_t *ln;

	ln = (listnode_t *) (((uint8_t *) element) + l->nodeoffset);
	
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

	/* Decrease number of elements. */

	l->size--;
}

 
/* list_first ******************************************************************

   Returns the first element of the list.

*******************************************************************************/

void *list_first(list_t *l)
{
	void *el;

	if (l->first == NULL)
		el = NULL;
	else
		el = ((uint8_t *) l->first) - l->nodeoffset;

	return el;
}


/* list_last *******************************************************************

   Returns the last element of the list.

*******************************************************************************/

void *list_last(list_t *l)
{
	void *el;

	if (l->last == NULL)
		el = NULL;
	else
		el = ((uint8_t *) l->last) - l->nodeoffset;

	return el;
}


/* list_next *******************************************************************

   Returns the next element of element from the list.

*******************************************************************************/

void *list_next(list_t *l, void *element)
{
	listnode_t *ln;
	void       *el;

	ln = (listnode_t *) (((uint8_t *) element) + l->nodeoffset);

	if (ln->next == NULL)
		el = NULL;
	else
		el = ((uint8_t *) ln->next) - l->nodeoffset;

	return el;
}

	
/* list_prev *******************************************************************

   Returns the previous element of element from the list.

*******************************************************************************/

void *list_prev(list_t *l, void *element)
{
	listnode_t *ln;
	void       *el;

	ln = (listnode_t *) (((uint8_t *) element) + l->nodeoffset);

	if (ln->prev == NULL)
		el = NULL;
	else
		el = ((uint8_t *) ln->prev) - l->nodeoffset;

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
