/* src/toolbox/list.cpp - linked list

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

#include <stdint.h>

#include "mm/memory.h"

#include "threads/mutex.hpp"

#include "toolbox/list.hpp"


/* list_add_before *************************************************************

   Adds the element newelement to the list l before element.

   [ A ] <-> [ newn ] <-> [ n ] <-> [ B ]

*******************************************************************************/

#if 0
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
#endif


/* C interface functions ******************************************************/

extern "C" {
	List<void*>* List_new(void)              { return new List<void*>(); }
	void         List_delete(List<void*>* l) { delete(l); }

	void*        List_back(List<void*>* l) { return l->back(); }
	void*        List_front(List<void*>* l) { return l->front(); }
	void         List_push_back(List<void*>* l, void* e) { l->push_back(e); }
	void         List_push_front(List<void*>* l, void* e) { l->push_front(e); }
	void         List_remove(List<void*>* l, void* e) { l->remove(e); }
	int          List_size(List<void*>* l) { return l->size(); }

	void         List_lock(List<void*>* l)   { l->lock(); }
	void         List_unlock(List<void*>* l) { l->unlock(); }

	List<void*>::iterator List_iterator_begin(List<void*>* l) { return l->begin(); }
	List<void*>::iterator List_iterator_end(List<void*>* l) { return l->end(); }
	List<void*>::iterator List_iterator_plusplus(List<void*>::iterator it) { return ++it; }

	void* List_iterator_deref(List<void*>::iterator it) { return *it; }

	List<void*>::reverse_iterator List_rbegin(List<void*>* l) { return l->rbegin(); }
	List<void*>::reverse_iterator List_rend(List<void*>* l) { return l->rend(); }
	List<void*>::reverse_iterator List_reverse_iterator_plusplus(List<void*>::reverse_iterator it) { return ++it; }

	void* List_reverse_iterator_deref(List<void*>::reverse_iterator it) { return *it; }

	DumpList<void*>* DumpList_new(void) { return new DumpList<void*>(); }
	void             DumpList_push_back(DumpList<void*>* l, void* e) { l->push_back(e); }
	void             DumpList_push_front(DumpList<void*>* l, void* e) { l->push_front(e); }
	void             DumpList_remove(DumpList<void*>* l, void* e) { l->remove(e); }

	DumpList<void*>::iterator DumpList_iterator_begin(DumpList<void*>* l) { return l->begin(); }
	DumpList<void*>::iterator DumpList_iterator_end(DumpList<void*>* l) { return l->end(); }
	DumpList<void*>::iterator DumpList_iterator_plusplus(DumpList<void*>::iterator it) { return ++it; }

	void* DumpList_iterator_deref(DumpList<void*>::iterator it) { return *it; }
}


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
