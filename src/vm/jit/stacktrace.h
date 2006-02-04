/* src/vm/jit/stacktrace.h - header file for stacktrace generation

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

   Authors: Christian Thalinger

   Changes:

   $Id: stacktrace.h 4433 2006-02-04 20:15:23Z twisti $

*/


#ifndef _STACKTRACE_H
#define _STACKTRACE_H

/* forward typedefs ***********************************************************/

typedef struct stackframeinfo stackframeinfo;
typedef struct stacktracebuffer stacktracebuffer;
typedef struct stacktrace_entry stacktrace_entry;

#include "config.h"
#include "vm/types.h"

#include "vm/method.h"


/* stackframeinfo **************************************************************

   ATTENTION: Keep the number of elements of this structure even, to
   make sure that the stack keeps aligned (e.g. 16-bytes for x86_64).

*******************************************************************************/

struct stackframeinfo {
	stackframeinfo *prev;               /* pointer to prev stackframeinfo     */
	methodinfo     *method;             /* methodinfo of current function     */
	u1             *pv;                 /* PV of current function             */
	u1             *sp;                 /* SP of parent Java function         */
	u1             *ra;                 /* RA to parent Java function         */
	u1             *xpc;                /* XPC (for inline stubs)             */
};

#if defined(USE_THREADS)
#define STACKFRAMEINFO    (stackframeinfo **) (&THREADINFO->_stackframeinfo)
#else
extern stackframeinfo *_no_threads_stackframeinfo;

#define STACKFRAMEINFO    (&_no_threads_stackframeinfo)
#endif


/* stacktrace_entry ***********************************************************/

struct stacktrace_entry {
#if SIZEOF_VOID_P == 8
	u8          linenumber;
#else
	u4          linenumber;
#endif
	methodinfo *method;
};


/* stacktracebuffer ***********************************************************/

#define STACKTRACE_CAPACITY_DEFAULT      80
#define STACKTRACE_CAPACITY_INCREMENT    80

struct stacktracebuffer {
	s4                capacity;         /* size of the buffer                 */
	s4                used;             /* current entries in the buffer      */
	stacktrace_entry *entries;          /* the actual entries                 */
};


/* function prototypes ********************************************************/

#if defined(ENABLE_INTRP)
void stacktrace_create_stackframeinfo(stackframeinfo *sfi, u1 *pv, u1 *sp,
									  u1 *ra);
#endif

void stacktrace_create_inline_stackframeinfo(stackframeinfo *sfi, u1 *pv,
											 u1 *sp, u1 *ra, u1 *xpc);

void stacktrace_create_extern_stackframeinfo(stackframeinfo *sfi, u1 *pv,
											 u1 *sp, u1 *ra, u1 *xpc);

void stacktrace_create_native_stackframeinfo(stackframeinfo *sfi, u1 *pv,
											 u1 *sp, u1 *ra);

void stacktrace_remove_stackframeinfo(stackframeinfo *sfi);

/* inline exception creating functions */
java_objectheader *stacktrace_inline_arithmeticexception(u1 *pv, u1 *sp, u1 *ra,
														 u1 *xpc);

java_objectheader *stacktrace_inline_arrayindexoutofboundsexception(u1 *pv,
																	u1 *sp,
																	u1 *ra,
																	u1 *xpc,
																	s4 index);

java_objectheader *stacktrace_inline_arraystoreexception(u1 *pv, u1 *sp, u1 *ra,
														 u1 *xpc);

java_objectheader *stacktrace_inline_classcastexception(u1 *pv, u1 *sp, u1 *ra,
														u1 *xpc);

java_objectheader *stacktrace_inline_nullpointerexception(u1 *pv, u1 *sp,
														  u1 *ra, u1 *xpc);


/* hardware exception creating functions */
java_objectheader *stacktrace_hardware_arithmeticexception(u1 *pv, u1 *sp,
														   u1 *ra, u1 *xpc);

java_objectheader *stacktrace_hardware_nullpointerexception(u1 *pv, u1 *sp,
															u1 *ra, u1 *xpc);

/* refill the stacktrace of an existing exception */
java_objectheader *stacktrace_inline_fillInStackTrace(u1 *pv, u1 *sp, u1 *ra,
													  u1 *xpc);

stacktracebuffer  *stacktrace_fillInStackTrace(void);
java_objectarray  *stacktrace_getClassContext(void);
java_objectheader *stacktrace_getCurrentClassLoader(void);
java_objectarray  *stacktrace_getStack(void);

void stacktrace_dump_trace(void);
void stacktrace_print_trace(java_objectheader *xptr);


/* machine dependent functions (code in ARCH_DIR/md.c) */

#if defined(ENABLE_JIT)
u1 *md_stacktrace_get_returnaddress(u1 *sp, u4 framesize);
#endif

#if defined(ENABLE_INTRP)
u1 *intrp_md_stacktrace_get_returnaddress(u1 *sp, u4 framesize);
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
 */
