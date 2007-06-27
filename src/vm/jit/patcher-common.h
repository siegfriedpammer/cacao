/* src/vm/jit/patcher-common.h - architecture independent code patching stuff

   Copyright (C) 2007 R. Grafl, A. Krall, C. Kruegel,
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

   $Id$

*/


#ifndef _PATCHER_COMMON_H
#define _PATCHER_COMMON_H

/* forward typedefs ***********************************************************/

#include "config.h"
#include "vm/types.h"

#include "toolbox/list.h"

#include "vm/global.h"

#include "vm/jit/jit.h"


/* patchref_t ******************************************************************

   A patcher reference contains information about a code position
   which needs additional code patching during runtime.

*******************************************************************************/

typedef struct patchref_t {
	ptrint       mpc;           /* position in code segment                   */
	s4           disp;          /* displacement of ref in the data segment    */
	functionptr  patcher;       /* patcher function to call                   */
	voidptr      ref;           /* reference passed                           */
	u8           mcode;         /* machine code to be patched back in         */
	bool         done;          /* XXX preliminary: patch already applied?    */
	listnode_t   linkage;
} patchref_t;


/* macros *********************************************************************/


/* function prototypes ********************************************************/

void patcher_list_create(codeinfo *code);
void patcher_list_free(codeinfo *code);

void patcher_add_patch_ref(jitdata *jd, functionptr patcher, voidptr ref,
                           s4 disp);

java_objectheader *patcher_handler(u1 *pc);


#endif /* _PATCHER_COMMON_H */


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
