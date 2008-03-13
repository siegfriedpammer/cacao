/* src/vm/jit/executionstate.h - execution-state handling

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


#ifndef _EXECUTIONSTATE_H
#define _EXECUTIONSTATE_H

/* forward typedefs ***********************************************************/

typedef struct executionstate_t executionstate_t;


#include "config.h"

#include <stdint.h>

#include "arch.h"
#include "md-abi.h"

#include "vm/jit/code.h"


/* executionstate_t ************************************************************

   An execution-state represents the state of a thread containing all
   registers that are important.  This structure is an internal
   structure similar to mcontext_t.

*******************************************************************************/

struct executionstate_t {
	uint8_t   *pc;                                         /* program counter */
	uint8_t   *sp;                             /* stack pointer within method */
	uint8_t   *pv;                             /* procedure value. NULL means */
	                                           /* search the AVL tree         */
	uint8_t   *ra;                          /* return address / link register */

	uintptr_t  intregs[INT_REG_CNT];                       /* register values */
	double     fltregs[FLT_REG_CNT];                       /* register values */
#if defined(HAS_ADDRESS_REGISTER_FILE)
	uintptr_t  adrregs[ADR_REG_CNT];                       /* register values */
#endif

	codeinfo  *code;                      /* codeinfo corresponding to the pv */
};


/* prototypes *****************************************************************/

#if !defined(NDEBUG)
void executionstate_sanity_check(void *context);
void executionstate_println(executionstate_t *es);
#endif

/* Machine and OS dependent functions (code in ARCH_DIR/OS_DIR/md-os.c) */

void md_executionstate_read(executionstate_t *es, void *ucontext);
void md_executionstate_write(executionstate_t *es, void *ucontext);

#endif /* _EXECUTIONSTATE_H */


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
