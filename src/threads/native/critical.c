/* src/threads/native/critical.c - restartable critical sections

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

   Authors: Stefan Ring
   			Edwin Steiner

   Changes: Christian Thalinger

   $Id: threads.c 4903 2006-05-11 12:48:43Z edwin $

*/

#include "config.h"
#include "vm/types.h"

#include <stddef.h>

#include "threads/native/critical.h"
#include "vm/jit/asmpart.h"
#include "toolbox/avl.h"


/* the AVL tree containing the critical sections */

static avl_tree *criticaltree;


/* prototypes *****************************************************************/

static s4 critical_compare(const void *pa, const void *pb);
static void critical_register_asm_critical_sections(void);


/* critical_init ***************************************************************

   Init global data structures.

*******************************************************************************/

void critical_init(void)
{
    criticaltree = avl_create(&critical_compare);

	critical_register_asm_critical_sections();
}

/* critical_compare ************************************************************

   Comparison function for AVL tree of critical section.

   IN:
       pa...............first node
	   pb...............second node

   RETURN VALUE:
       -1, 0, +1 for (pa <, ==, > pb)

*******************************************************************************/

static s4 critical_compare(const void *pa, const void *pb)
{
	const critical_section_node_t *na = pa;
	const critical_section_node_t *nb = pb;

	if (na->mcodebegin < nb->mcodebegin)
		return -1;
	if (na->mcodebegin > nb->mcodebegin)
		return 1;
	return 0;
}


/* critical_find ***************************************************************
 
   Find the critical region the given pc is in.

   IN:
       mcodeptr.........PC

   OUT:
       pointer to critical region node, or
	   NULL if critical region was not found.
   
*******************************************************************************/

static const critical_section_node_t *critical_find(u1 *mcodeptr)
{
    avl_node *n;
    const critical_section_node_t *m;

    n = criticaltree->root;
	m = NULL;

    if (!n)
        return NULL;

    for (;;) {
        const critical_section_node_t *d = n->data;

        if (mcodeptr == d->mcodebegin)
            return d;

        if (mcodeptr < d->mcodebegin) {
            if (n->childs[0]) {
                n = n->childs[0];
			}
            else {
                return m;
			}
        }
		else {
            if (n->childs[1]) {
                m = n->data;
                n = n->childs[1];
            }
			else {
                return n->data;
			}
        }
    }
}


/* critical_register_critical_section ******************************************
 
   Register a critical section.

   IN:
       n................node for the critical section

*******************************************************************************/

void critical_register_critical_section(critical_section_node_t *n)
{
	avl_insert(criticaltree, n);
}


/* critical_find_restart_point *************************************************

   Find a restart point for the given PC, in case it is in a critical section.

   IN:
       mcodeptr.........PC

   OUT:
       PC of the restart point, or
	   NULL if the given mcodeptr is not in a critical section

*******************************************************************************/

u1 *critical_find_restart_point(u1 *mcodeptr)
{
	const critical_section_node_t *n = critical_find(mcodeptr);

	/* XXX should we check >= n->mcodebegin */
	return (n && mcodeptr < n->mcodeend && mcodeptr > n->mcodebegin) ? n->mcoderestart : NULL;
}


/* critical_register_asm_critical_sections *************************************

   Register critical sections defined in the array asm_criticalsections.

*******************************************************************************/

static void critical_register_asm_critical_sections(void)
{
	/* XXX TWISTI: this is just a quick hack */
#if defined(ENABLE_JIT)
	critical_section_node_t *n = &asm_criticalsections;

	while (n->mcodebegin)
		critical_register_critical_section(n++);
#endif
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
