/* src/vm/jit/linenumbertable.h - linenumber table

   Copyright (C) 2007, 2008
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


#ifndef _LINENUMBERTABLE_H
#define _LINENUMBERTABLE_H

/* forward typedefs ***********************************************************/

typedef struct linenumbertable_t            linenumbertable_t;
typedef struct linenumbertable_entry_t      linenumbertable_entry_t;
typedef struct linenumbertable_list_entry_t linenumbertable_list_entry_t;


#include "config.h"

#include <stdint.h>

#include "toolbox/list.h"

#include "vm/method.h"

#include "vm/jit/jit.h"
#include "vm/jit/code.h"
#include "vm/jit/codegen-common.h"

#include "vm/jit/ir/instruction.hpp"


#ifdef __cplusplus
extern "C" {
#endif

/* linenumbertable_t **********************************************************/

struct linenumbertable_t {
	int32_t                  length;    /* length of the entries array        */
	linenumbertable_entry_t *entries;   /* actual linenumber entries          */
};


/* linenumbertable_entry_t *****************************************************

   NOTE: See doc/inlining_stacktrace.txt for special meanings of line
   and pc.

*******************************************************************************/

struct linenumbertable_entry_t {
	int32_t  linenumber;                /* linenumber of this entry           */
	void    *pc;                        /* PC where this linenumber starts    */
};


/* linenumbertable_list_entry_t ***********************************************/

struct linenumbertable_list_entry_t {
	int32_t    linenumber;      /* line number, used for inserting into the   */
	                            /* table and for validity checking            */
	                            /* -1......start of inlined body              */
	                            /* -2......end of inlined body                */
	                            /* <= -3...special entry with methodinfo *    */
								/* (see doc/inlining_stacktrace.txt)          */
	ptrint     mpc;             /* machine code program counter of first      */
	                            /* instruction for given line                 */
	                            /* NOTE: for linenumber <= -3 this is a the   */
	                            /* (methodinfo *) of the inlined method       */
	listnode_t linkage;
};


/* function prototypes ********************************************************/

void    linenumbertable_create(jitdata *jd);

void    linenumbertable_list_entry_add(codegendata *cd, int32_t linenumber);
void    linenumbertable_list_entry_add_inline_start(codegendata *cd, instruction *iptr);
void    linenumbertable_list_entry_add_inline_end(codegendata *cd, instruction *iptr);

int32_t linenumbertable_linenumber_for_pc(methodinfo **pm, codeinfo *code, void *pc);

#ifdef __cplusplus
}
#endif

#endif /* _LINENUMBERTABLE_H */


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
