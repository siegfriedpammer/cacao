/* src/threads/critical.c - restartable critical sections

   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
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

*/


#include "config.h"

#include <stddef.h>
#include <stdint.h>

#include "vm/types.h"

#include "threads/critical.h"

#include "toolbox/avl.h"


/* the AVL tree containing the critical sections */

static avl_tree_t *criticaltree;


/* prototypes *****************************************************************/

static int critical_comparator(const void *treenode, const void *node);


/* critical_init ***************************************************************

   Init global data structures.

*******************************************************************************/

void critical_init(void)
{
    criticaltree = avl_create(&critical_comparator);
}


/* critical_comparator *********************************************************

   Comparison function for AVL tree of critical section.

   IN:
       treenode....node in the tree
	   node........node to compare with tree-node

   RETURN VALUE:
       -1, 0, +1 for (pa <, ==, > pb)

*******************************************************************************/

static int critical_comparator(const void *treenode, const void *node)
{
	const critical_section_node_t *treecsn;
	const critical_section_node_t *csn;

	treecsn = treenode;
	csn     = node;

#ifdef __S390__
#	define ADDR_MASK(x) ((u1 *)((s4)(x) & 0x7FFFFFFF))
#else
#	define ADDR_MASK(x) (x)
#endif

	/* compare for avl_find if we have found an entry */

	if (
		(ADDR_MASK(treecsn->start) <= ADDR_MASK(csn->start)) && 
		(ADDR_MASK(csn->start) < ADDR_MASK(treecsn->end))
	)
		return 0;

	/* these are for walking the tree */

	if (ADDR_MASK(treecsn->start) < ADDR_MASK(csn->start))
		return -1;
	else
		return 1;

#undef ADDR_MASK
}


/* critical_section_register ***************************************************
 
   Register a critical section.

   IN:
       csn....node for the critical section

*******************************************************************************/

void critical_section_register(critical_section_node_t *csn)
{
	(void) avl_insert(criticaltree, csn);
}


/* critical_find_restart_point *************************************************

   Find a restart point for the given PC, in case it is in a critical
   section.

   IN:
       pc.........PC

   OUT:
       PC of the restart point, or
	   NULL if the given mcodeptr is not in a critical section

*******************************************************************************/

u1 *critical_find_restart_point(u1 *pc)
{
	critical_section_node_t        csnpc;
	const critical_section_node_t *csn;

	/* fill the temporary node for comparison */

	csnpc.start = pc;

	/* see if there's an entry for that PC */

	csn = avl_find(criticaltree, &csnpc);

	if (csn == NULL)
		return NULL;

	return csn->restart;
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
 * vim:noexpandtab:sw=4:ts=4:
 */
