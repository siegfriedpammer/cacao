/* toolbox/chain.h - management of doubly linked lists with external linking

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser,
   M. Probst, S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck,
   P. Tomsich, J. Wenninger

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

   $Id: chain.h 557 2003-11-02 22:51:59Z twisti $

*/


#ifndef _CHAIN_H
#define _CHAIN_H

typedef struct chainlink {          /* structure for list element */
	struct chainlink *next;
	struct chainlink *prev;
	void *element;
} chainlink;

typedef struct chain {	            /* structure for list */
	int  usedump;   

	chainlink *first;
	chainlink *last;
	chainlink *active;
} chain;


/* function prototypes */
chain *chain_new();
chain *chain_dnew();
void chain_free(chain *c);

void chain_addafter(chain *c, void *element);
void chain_addbefore(chain *c, void *element);
void chain_addlast(chain *c, void *element);
void chain_addfirst(chain *c, void *element);

void chain_remove(chain *c);
void *chain_remove_go_prev(chain *c);
void chain_removespecific(chain *c, void *element);

void *chain_next(chain *c);
void *chain_prev(chain *c);
void *chain_this(chain *c);

void *chain_first(chain *c);
void *chain_last(chain *c);


/*
--------------------------- interface description ------------------------

Usage of these functions for list management is possible without additional
preparation in the element structures, as opposed to the module 'list'.

Consequently, the functions are a little slower and need more memory.

A new list is created with
	chain_new
or  chain_dnew.
The latter allocates all additional data structures on the dump memory (faster)
for which no explicit freeing is necessary after the processing. Care needs to
be taken to not accidentally free parts of these structures by calling
'dump_release' too early.

After usage, a list can be freed with
	chain_free.
(use only if the list was created with 'chain_new')


Adding elements is easy with:
	chain_addafter, chain_addlast, chain_addbefore, chain_addfirst		
	
Search the list with:
	chain_first, chain_last, chain_prev, chain_next, chain_this
	
Delete elements from the list:
	chain_remove, chain_remove_go_prev, chain_removespecific
	
	
ATTENTION: As mentioned earlier, there are no pointers to the list or to other
nodes inside the list elements, so list elements cannot be used as pointers
into the list. Therefore a 'cursor' is used to make one element current. Every
insertion/deletion occurs at a position relative to this cursor.

*/

#endif /* _CHAIN_H */


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
