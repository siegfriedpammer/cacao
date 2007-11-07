/* src/vm/jit/stacktrace.h - header file for stacktrace generation

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


#ifndef _STACKTRACE_H
#define _STACKTRACE_H

/* forward typedefs ***********************************************************/

typedef struct stackframeinfo_t   stackframeinfo_t;
typedef struct stacktrace_entry_t stacktrace_entry_t;
typedef struct stacktrace_t       stacktrace_t;

#include "config.h"

#include <stdint.h>

#include "vm/types.h"

#include "md-abi.h"

#include "vm/global.h"

#include "vm/jit/code.h"

#include "vmcore/class.h"


/* stackframeinfo **************************************************************

   ATTENTION: Keep the number of elements of this structure even, to
   make sure that the stack keeps aligned (e.g. 16-bytes for x86_64).

*******************************************************************************/

struct stackframeinfo_t {
	stackframeinfo_t *prev;             /* pointer to prev stackframeinfo     */
	codeinfo         *code;             /* codeinfo of current method         */
	u1               *pv;               /* PV of current function             */
	u1               *sp;               /* SP of parent Java function         */
	u1               *ra;               /* RA to parent Java function         */
	u1               *xpc;              /* XPC (for inline stubs)             */
#if defined(ENABLE_GC_CACAO)
	/* 
	 * The exact GC needs to be able to recover saved registers, so the
	 * native-stub saves these registers here
	 */
# if defined(HAS_ADDRESS_REGISTER_FILE)
	uintptr_t         adrregs[ADR_SAV_CNT];
# else
	uintptr_t         intregs[INT_SAV_CNT];
# endif
#endif
};


/* stacktrace_entry_t *********************************************************/

struct stacktrace_entry_t {
	codeinfo *code;                     /* codeinfo pointer of this method    */
	void     *pc;                       /* PC in this method                  */
};


/* stacktrace_t ***************************************************************/

struct stacktrace_t {
	int32_t            length;          /* length of the entries array        */
	stacktrace_entry_t entries[1];      /* stacktrace entries                 */
};


/* function prototypes ********************************************************/

void                       stacktrace_stackframeinfo_add(stackframeinfo_t *sfi, u1 *pv, u1 *sp, u1 *ra, u1 *xpc);
void                       stacktrace_stackframeinfo_remove(stackframeinfo_t *sfi);

java_handle_bytearray_t   *stacktrace_get(void);

#if defined(ENABLE_JAVASE)
classloader               *stacktrace_first_nonnull_classloader(void);
java_handle_objectarray_t *stacktrace_getClassContext(void);
classinfo                 *stacktrace_get_current_class(void);
java_handle_objectarray_t *stacktrace_get_stack(void);
#endif

void                       stacktrace_print(stacktrace_t *st);
void                       stacktrace_print_exception(java_handle_t *h);

/* machine dependent functions (code in ARCH_DIR/md.c) */

#if defined(ENABLE_JIT)
# if defined(__SPARC_64__)
u1 *md_get_framepointer(u1 *sp);
u1 *md_get_pv_from_stackframe(u1 *sp);
# endif
#endif

#if defined(ENABLE_INTRP)
u1 *intrp_md_stacktrace_get_returnaddress(u1 *sp, u4 framesize);
#endif

#if defined(ENABLE_CYCLES_STATS)
void stacktrace_print_cycles_stats(FILE *file);
#endif

#endif /* _STACKTRACE_H */


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
