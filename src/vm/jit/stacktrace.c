/* src/vm/jit/stacktrace.c - machine independet stacktrace system

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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

   Authors: Joseph Wenninger

   Changes: Christian Thalinger

   $Id: stacktrace.c 4157 2006-01-12 21:30:41Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "vm/types.h"

#include "mm/boehm.h"
#include "native/native.h"

#include "vm/global.h"                   /* required here for native includes */
#include "native/include/java_lang_ClassLoader.h"
#include "native/include/java_lang_Throwable.h"
#include "native/include/java_lang_VMThrowable.h"

#include "toolbox/logging.h"
#include "vm/builtin.h"
#include "vm/class.h"
#include "vm/exceptions.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/codegen-common.h"
#include "vm/jit/methodheader.h"


/* lineNumberTableEntry *******************************************************/

/* Keep the type of line the same as the pointer type, otherwise we run into  */
/* alignment troubles (like on MIPS64).                                       */

typedef struct lineNumberTableEntry {
	ptrint  line;
	u1     *pc;
} lineNumberTableEntry;


typedef struct lineNumberTableEntryInlineBegin {
	/* this should have the same layout and size as the lineNumberTableEntry */
	ptrint      lineNrOuter;
	methodinfo *method;
} lineNumberTableEntryInlineBegin;

#ifndef ENABLE_JVMTI
typedef bool(*CacaoStackTraceCollector)(void **, stackTraceBuffer*);
#endif

#define BLOCK_INITIALSIZE 40
#define BLOCK_SIZEINCREMENT 40


/* global variables ***********************************************************/

#if !defined(USE_THREADS)
stackframeinfo *_no_threads_stackframeinfo = NULL;
#endif


/* stacktrace_create_stackframeinfo ********************************************

   Creates an stackframe info structure for inline code in the
   interpreter.

*******************************************************************************/

#if defined(ENABLE_INTRP)
void stacktrace_create_stackframeinfo(stackframeinfo *sfi, u1 *pv, u1 *sp,
									  u1 *ra)
{
	stackframeinfo **psfi;
	methodinfo      *m;

	/* get current stackframe info pointer */

	psfi = STACKFRAMEINFO;

	/* if we don't have pv handy */

	if (pv == NULL) {
#if defined(ENABLE_INTRP)
		if (opt_intrp)
			pv = codegen_findmethod(ra);
		else
#endif
			{
#if defined(ENABLE_JIT)
				pv = md_codegen_findmethod(ra);
#endif
			}
	}

	/* get methodinfo pointer from data segment */

	m = *((methodinfo **) (pv + MethodPointer));

	/* fill new stackframe info structure */

	sfi->prev   = *psfi;
	sfi->method = m;
	sfi->pv     = pv;
	sfi->sp     = sp;
	sfi->ra     = ra;

	/* xpc is the same as ra, but is required in fillInStackTrace */

	sfi->xpc    = ra;

	/* store new stackframe info pointer */

	*psfi = sfi;
}
#endif /* defined(ENABLE_INTRP) */

/* stacktrace_create_inline_stackframeinfo *************************************

   Creates an stackframe info structure for an inline exception stub.

*******************************************************************************/

void stacktrace_create_inline_stackframeinfo(stackframeinfo *sfi, u1 *pv,
											 u1 *sp, u1 *ra, u1 *xpc)
{
	stackframeinfo **psfi;

	/* get current stackframe info pointer */

	psfi = STACKFRAMEINFO;

#if defined(ENABLE_INTRP)
	if (opt_intrp) {
		/* if we don't have pv handy */

		if (pv == NULL)
			pv = codegen_findmethod(ra);

	}
#endif

	/* fill new stackframe info structure */

	sfi->prev   = *psfi;
	sfi->method = NULL;
	sfi->pv     = pv;
	sfi->sp     = sp;
	sfi->ra     = ra;
	sfi->xpc    = xpc;

	/* store new stackframe info pointer */

	*psfi = sfi;
}


/* stacktrace_create_extern_stackframeinfo *************************************

   Creates an stackframe info structure for an extern exception
   (hardware or assembler).

*******************************************************************************/

void stacktrace_create_extern_stackframeinfo(stackframeinfo *sfi, u1 *pv,
											 u1 *sp, u1 *ra, u1 *xpc)
{
	stackframeinfo **psfi;
#if !defined(__I386__) && !defined(__X86_64__)
	bool             isleafmethod;
#endif
	s4               framesize;

	/* get current stackframe info pointer */

	psfi = STACKFRAMEINFO;

	/* sometimes we don't have pv handy (e.g. in asmpart.S:
       L_asm_call_jit_compiler_exception or in the interpreter). */

	if (pv == NULL) {
#if defined(ENABLE_INTRP)
		if (opt_intrp)
			pv = codegen_findmethod(ra);
		else
#endif
			{
#if defined(ENABLE_JIT)
				pv = md_codegen_findmethod(ra);
#endif
			}
	}

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	/* When using the interpreter, we pass RA to the function. */

	if (!opt_intrp) {
# endif
# if defined(__I386__) || defined(__X86_64__)
		/* On i386 and x86_64 we always have to get the return address
		   from the stack. */

		framesize = *((u4 *) (pv + FrameSize));

		ra = md_stacktrace_get_returnaddress(sp, framesize);
# else
		/* If the method is a non-leaf function, we need to get the return
		   address from the stack. For leaf functions the return address
		   is set correctly. This makes the assembler and the signal
		   handler code simpler. */

		isleafmethod = *((s4 *) (pv + IsLeaf));

		if (!isleafmethod) {
			framesize = *((u4 *) (pv + FrameSize));

			ra = md_stacktrace_get_returnaddress(sp, framesize);
		}
# endif
# if defined(ENABLE_INTRP)
	}
# endif
#endif

	/* fill new stackframe info structure */

	sfi->prev   = *psfi;
	sfi->method = NULL;
	sfi->pv     = pv;
	sfi->sp     = sp;
	sfi->ra     = ra;
	sfi->xpc    = xpc;

	/* store new stackframe info pointer */

	*psfi = sfi;
}


/* stacktrace_create_native_stackframeinfo *************************************

   Creates a stackframe info structure for a native stub.

*******************************************************************************/

void stacktrace_create_native_stackframeinfo(stackframeinfo *sfi, u1 *pv,
											 u1 *sp, u1 *ra)
{
	stackframeinfo **psfi;
	methodinfo      *m;

	/* get methodinfo pointer from data segment */

	m = *((methodinfo **) (pv + MethodPointer));

	/* get current stackframe info pointer */

	psfi = STACKFRAMEINFO;

	/* fill new stackframe info structure */

	sfi->prev   = *psfi;
	sfi->method = m;
	sfi->pv     = NULL;
	sfi->sp     = sp;
	sfi->ra     = ra;
	sfi->xpc    = NULL;

	/* store new stackframe info pointer */

	*psfi = sfi;
}


/* stacktrace_remove_stackframeinfo ********************************************

   XXX

*******************************************************************************/

void stacktrace_remove_stackframeinfo(stackframeinfo *sfi)
{
	stackframeinfo **psfi;

	/* get current stackframe info pointer */

	psfi = STACKFRAMEINFO;

	/* restore the old pointer */

	*psfi = sfi->prev;
}


/* stacktrace_inline_arithmeticexception ***************************************

   Creates an ArithemticException for inline stub.

*******************************************************************************/

java_objectheader *stacktrace_inline_arithmeticexception(u1 *pv, u1 *sp,
														 u1 *ra, u1 *xpc)
{
	stackframeinfo     sfi;
	java_objectheader *o;

	/* create stackframeinfo */

	stacktrace_create_inline_stackframeinfo(&sfi, pv, sp, ra, xpc);

	/* create exception */

	o = new_arithmeticexception();

	/* remove stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	return o;
}


/* stacktrace_inline_arrayindexoutofboundsexception ****************************

   Creates an ArrayIndexOutOfBoundsException for inline stub.

*******************************************************************************/

java_objectheader *stacktrace_inline_arrayindexoutofboundsexception(u1 *pv,
																	u1 *sp,
																	u1 *ra,
																	u1 *xpc,
																	s4 index)
{
	stackframeinfo     sfi;
	java_objectheader *o;

	/* create stackframeinfo */

	stacktrace_create_inline_stackframeinfo(&sfi, pv, sp, ra, xpc);

	/* create exception */

	o = new_arrayindexoutofboundsexception(index);

	/* remove stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	return o;
}


/* stacktrace_inline_arraystoreexception ***************************************

   Creates an ArrayStoreException for inline stub.

*******************************************************************************/

java_objectheader *stacktrace_inline_arraystoreexception(u1 *pv, u1 *sp, u1 *ra,
														 u1 *xpc)
{
	stackframeinfo     sfi;
	java_objectheader *o;

	/* create stackframeinfo */

	stacktrace_create_inline_stackframeinfo(&sfi, pv, sp, ra, xpc);

	/* create exception */

	o = new_arraystoreexception();

	/* remove stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	return o;
}


/* stacktrace_inline_classcastexception ****************************************

   Creates an ClassCastException for inline stub.

*******************************************************************************/

java_objectheader *stacktrace_inline_classcastexception(u1 *pv, u1 *sp, u1 *ra,
														u1 *xpc)
{
	stackframeinfo     sfi;
	java_objectheader *o;

	/* create stackframeinfo */

	stacktrace_create_inline_stackframeinfo(&sfi, pv, sp, ra, xpc);

	/* create exception */

	o = new_classcastexception();

	/* remove stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	return o;
}


/* stacktrace_inline_nullpointerexception **************************************

   Creates an NullPointerException for inline stub.

*******************************************************************************/

java_objectheader *stacktrace_inline_nullpointerexception(u1 *pv, u1 *sp,
														  u1 *ra, u1 *xpc)
{
	stackframeinfo     sfi;
	java_objectheader *o;

	/* create stackframeinfo */

	stacktrace_create_inline_stackframeinfo(&sfi, pv, sp, ra, xpc);

	/* create exception */

	o = new_nullpointerexception();

	/* remove stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	return o;
}


/* stacktrace_hardware_arithmeticexception *************************************

   Creates an ArithemticException for inline stub.

*******************************************************************************/

java_objectheader *stacktrace_hardware_arithmeticexception(u1 *pv, u1 *sp,
														   u1 *ra, u1 *xpc)
{
	stackframeinfo     sfi;
	java_objectheader *o;

	/* create stackframeinfo */

	stacktrace_create_extern_stackframeinfo(&sfi, pv, sp, ra, xpc);

	/* create exception */

	o = new_arithmeticexception();

	/* remove stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	return o;
}


/* stacktrace_hardware_nullpointerexception ************************************

   Creates an NullPointerException for the SIGSEGV signal handler.

*******************************************************************************/

java_objectheader *stacktrace_hardware_nullpointerexception(u1 *pv, u1 *sp,
															u1 *ra, u1 *xpc)
{
	stackframeinfo     sfi;
	java_objectheader *o;

	/* create stackframeinfo */

	stacktrace_create_extern_stackframeinfo(&sfi, pv, sp, ra, xpc);

	/* create exception */

	o = new_nullpointerexception();

	/* remove stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	return o;
}


/* stacktrace_inline_fillInStackTrace ******************************************

   Fills in the correct stacktrace into an existing exception object
   (this one is for inline exception stubs).

*******************************************************************************/

java_objectheader *stacktrace_inline_fillInStackTrace(u1 *pv, u1 *sp, u1 *ra,
													  u1 *xpc)
{
	stackframeinfo     sfi;
	java_objectheader *o;
	methodinfo        *m;

	/* create stackframeinfo */

	stacktrace_create_inline_stackframeinfo(&sfi, pv, sp, ra, xpc);

	/* get exception */

	o = *exceptionptr;

	/* clear exception */

	*exceptionptr = NULL;

	/* resolve methodinfo pointer from exception object */

	m = class_resolvemethod(o->vftbl->class,
							utf_fillInStackTrace,
							utf_void__java_lang_Throwable);

	/* call function */

	ASM_CALLJAVAFUNCTION(m, o, NULL, NULL, NULL);

	/* remove stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	return o;
}


/* addEntry ********************************************************************

   XXX

*******************************************************************************/

static void addEntry(stackTraceBuffer *buffer, methodinfo *method, u2 line)
{
	if (buffer->size > buffer->full) {
		stacktraceelement *tmp = &(buffer->start[buffer->full]);

		tmp->method = method;
		tmp->linenumber = line;
		buffer->full = buffer->full + 1;

	} else {
		stacktraceelement *newBuffer;

		newBuffer =
			(stacktraceelement *) malloc((buffer->size + BLOCK_SIZEINCREMENT) *
										 sizeof(stacktraceelement));

		if (newBuffer == 0) {
			log_text("OOM during stacktrace creation");
			assert(0);
		}

		memcpy(newBuffer, buffer->start, buffer->size * sizeof(stacktraceelement));
		if (buffer->needsFree)
			free(buffer->start);

		buffer->start = newBuffer;
		buffer->size = buffer->size + BLOCK_SIZEINCREMENT;
		buffer->needsFree = 1;

		addEntry(buffer, method, line);
	}
}


/* stacktrace_fillInStackTrace_methodRecursive *********************************

   XXX

*******************************************************************************/

static bool stacktrace_fillInStackTrace_methodRecursive(stackTraceBuffer *buffer,
														methodinfo *m,
														lineNumberTableEntry *lntentry,
														s4 lntsize,
														u1 *pc)
{
#if 0
	lineNumberTableEntryInlineBegin *lntinline;
#endif

	/* find the line number for the specified pc (going backwards) */

	for (; lntsize > 0; lntsize--, lntentry--) {
		/* did we reach the current line? */

		if (pc >= lntentry->pc) {
			/* check for special inline entries */

			switch (lntentry->line) {
#if 0
			/* XXX TWISTI we have to think about this inline stuff again */

			case -1: /* begin of inlined method */
				lntinline = (lineNumberTableEntryInlineBegin *) (--lntentry);
				lntentry++;
				lntsize--; lntsize--;
				if (stacktrace_fillInStackTrace_methodRecursive(buffer,
																ilStart->method,
																ent,
																&ent,
																&ahead,
																pc)) {
					addEntry(buffer, m, ilStart->lineNrOuter);

					return true;
				}
				break;

			case -2: /* end of inlined method */
				*entry = ent;
				*entriesAhead = ahead;
				return false;
				break;
#endif

			default:
				addEntry(buffer, m, lntentry->line);
				return true;
			}
		}
	}

	/* check if we are before the actual JIT code */

	if ((ptrint) pc < (ptrint) m->entrypoint) {
		dolog("Current pc before start of code: %p < %p", pc, m->entrypoint);
		assert(0);
	}

	/* otherwise just add line 0 */

	addEntry(buffer, m, 0);

	return true;
}


/* stacktrace_fillInStackTrace_method ******************************************

   XXX

*******************************************************************************/

static void stacktrace_fillInStackTrace_method(stackTraceBuffer *buffer,
											   methodinfo *method, u1 *pv,
											   u1 *pc)
{
	ptrint                lntsize;      /* size of line number table          */
	u1                   *lntstart;     /* start of line number table         */
	lineNumberTableEntry *lntentry;     /* points to last entry in the table  */

	/* get size of line number table */

	lntsize  = *((ptrint *) (pv + LineNumberTableSize));
	lntstart = *((u1 **)    (pv + LineNumberTableStart));

	/* subtract the size of the line number entry of the structure, since the */
	/* line number table start points to the pc */

	lntentry = (lineNumberTableEntry *) (lntstart - SIZEOF_VOID_P);

	if (lntsize == 0) {
		/* this happens when an exception is thrown in the native stub */

		addEntry(buffer, method, 0);

	} else {
		if (!stacktrace_fillInStackTrace_methodRecursive(buffer,
														 method,
														 lntentry,
														 lntsize,
														 pc)) {
			log_text("Trace point not found in suspected method");
			assert(0);
		}
	}
}


/* cacao_stacktrace_fillInStackTrace *******************************************

   XXX

*******************************************************************************/
#ifdef ENABLE_JVMTI
bool cacao_stacktrace_fillInStackTrace(void **target,
									   CacaoStackTraceCollector coll,
									   threadobject* thread)
#else
static bool cacao_stacktrace_fillInStackTrace(void **target,
									   CacaoStackTraceCollector coll)
#endif

{
	stacktraceelement primaryBlock[BLOCK_INITIALSIZE*sizeof(stacktraceelement)];
	stackTraceBuffer  buffer;
	stackframeinfo   *sfi;
	methodinfo       *m;
	u1               *pv;
	u1               *sp;
	u4                framesize;
	u1               *ra;
	u1               *xpc;
	bool              result;

	/* prevent compiler warnings */

	pv = NULL;
	sp = NULL;
	ra = NULL;

	/* In most cases this should be enough -> one malloc less. I don't think  */
	/* temporary data should be allocated with the GC, only the result.       */

	buffer.needsFree = 0;
	buffer.start = primaryBlock;
	buffer.size = BLOCK_INITIALSIZE; /*  *sizeof(stacktraceelement); */
	buffer.full = 0;

	/* the first element in the stackframe chain must always be a native      */
	/* stackframeinfo (VMThrowable.fillInStackTrace is a native function)     */

#ifdef ENABLE_JVMTI
	if (thread == NULL) 
		sfi = *STACKFRAMEINFO; /* invocation from Throwable */
	else
		sfi = thread->info._stackframeinfo; /* invocation from JVMTI */
#else
	sfi = *STACKFRAMEINFO;
#endif

	if (!sfi) {
		*target = NULL;
		return true;
	}

#define PRINTMETHODS 0

#if PRINTMETHODS
	printf("\n\nfillInStackTrace start:\n");
	fflush(stdout);
#endif

	/* loop while we have a method pointer (asm_calljavafunction has NULL) or */
	/* there is a stackframeinfo in the chain                                 */

	m = NULL;

	while (m || sfi) {
		/* m == NULL should only happen for the first time and inline         */
		/* stackframe infos, like from the exception stubs or the patcher     */
		/* wrapper                                                            */

		if (m == NULL) {
			/* for native stub stackframe infos, pv is always NULL */

			if (sfi->pv == NULL) {
				/* get methodinfo, sp and ra from the current stackframe info */

				m  = sfi->method;
				sp = sfi->sp;           /* sp of parent Java function         */
				ra = sfi->ra;

				if (m)
					addEntry(&buffer, m, 0);

#if PRINTMETHODS
				printf("ra=%p sp=%p, ", ra, sp);
				utf_display_classname(m->class->name);
				printf(".");
				utf_display(m->name);
				utf_display(m->descriptor);
				printf(": native stub\n");
				fflush(stdout);
#endif
				/* this is an native stub stackframe info, so we can get the */
				/* parent pv from the return address (ICMD_INVOKE*) */

#if defined(ENABLE_INTRP)
				if (opt_intrp)
					pv = codegen_findmethod(ra);
				else
#endif
					{
#if defined(ENABLE_JIT)
						pv = md_codegen_findmethod(ra);
#endif
					}

				/* get methodinfo pointer from parent data segment */

				m = *((methodinfo **) (pv + MethodPointer));

			} else {
				/* Inline stackframe infos are special: they have a xpc of    */
				/* the actual exception position and the return address saved */
				/* since an inline stackframe info can also be in a leaf      */
				/* method (no return address saved on stack!!!).              */
				/* ATTENTION: This one is also for hardware exceptions!!!     */

				/* get methodinfo, sp and ra from the current stackframe info */

				m   = sfi->method;      /* m == NULL                          */
				pv  = sfi->pv;          /* pv of parent Java function         */
				sp  = sfi->sp;          /* sp of parent Java function         */
				ra  = sfi->ra;          /* ra of parent Java function         */
				xpc = sfi->xpc;         /* actual exception position          */

#if PRINTMETHODS
				printf("ra=%p sp=%p, ", ra, sp);
				printf("NULL: inline stub\n");
				fflush(stdout);
#endif

				/* get methodinfo from current Java method */

				m = *((methodinfo **) (pv + MethodPointer));

				/* if m == NULL, this is a asm_calljavafunction call */

				if (m != NULL) {
#if PRINTMETHODS
					printf("ra=%p sp=%p, ", ra, sp);
					utf_display_classname(m->class->name);
					printf(".");
					utf_display(m->name);
					utf_display(m->descriptor);
					printf(": inline stub parent");
					fflush(stdout);
#endif

#if defined(ENABLE_INTRP)
					if (!opt_intrp) {
#endif

					/* add it to the stacktrace */

					stacktrace_fillInStackTrace_method(&buffer, m, pv,
													   (u1 *) ((ptrint) xpc));

					/* get the current stack frame size */

					framesize = *((u4 *) (pv + FrameSize));

#if PRINTMETHODS
					printf(", framsize=%d\n", framesize);
					fflush(stdout);
#endif

					/* set stack pointer to stackframe of parent Java */
					/* function of the current Java function */

#if defined(__I386__) || defined (__X86_64__)
					sp += framesize + SIZEOF_VOID_P;
#else
					sp += framesize;
#endif

					/* get data segment and methodinfo pointer from parent */
					/* method */

#if defined(ENABLE_JIT)
					pv = md_codegen_findmethod(ra);
#endif

					m = *((methodinfo **) (pv + MethodPointer));

#if defined(ENABLE_INTRP)
					}
#endif
				}
#if PRINTMETHODS
				else {
					printf("ra=%p sp=%p, ", ra, sp);
					printf("asm_calljavafunction\n");
					fflush(stdout);
				}
#endif
			}

			/* get previous stackframeinfo in the chain */

			sfi = sfi->prev;

		} else {
#if PRINTMETHODS
			printf("ra=%p sp=%p, ", ra, sp);
			utf_display_classname(m->class->name);
			printf(".");
			utf_display(m->name);
			utf_display(m->descriptor);
			printf(": JIT");
			fflush(stdout);
#endif

			/* JIT method found, add it to the stacktrace (we subtract 1 from */
			/* the return address since it points the the instruction after */
			/* call) */

			stacktrace_fillInStackTrace_method(&buffer, m, pv,
											   (u1 *) ((ptrint) ra) - 1);

			/* get the current stack frame size */

			framesize = *((u4 *) (pv + FrameSize));

#if PRINTMETHODS
			printf(", framsize=%d\n", framesize);
			fflush(stdout);
#endif

			/* get return address of current stack frame */

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
			if (opt_intrp)
				ra = intrp_md_stacktrace_get_returnaddress(sp, framesize);
			else
# endif
				ra = md_stacktrace_get_returnaddress(sp, framesize);
#else
			ra = intrp_md_stacktrace_get_returnaddress(sp, framesize);
#endif

			/* get data segment and methodinfo pointer from parent method */

#if defined(ENABLE_INTRP)
			if (opt_intrp)
				pv = codegen_findmethod(ra);
			else
#endif
				{
#if defined(ENABLE_JIT)
					pv = md_codegen_findmethod(ra);
#endif
				}

			m = *((methodinfo **) (pv + MethodPointer));

			/* walk the stack */

#if defined(ENABLE_INTRP)
			if (opt_intrp)
				sp = *(u1 **)(sp - framesize);
			else
#endif
				{
#if defined(__I386__) || defined (__X86_64__)
					sp += framesize + SIZEOF_VOID_P;
#else
					sp += framesize;
#endif
				}
		}
	}
			
	if (coll)
		result = coll(target, &buffer);

	if (buffer.needsFree)
		free(buffer.start);

	return result;
}


/* stackTraceCollector *********************************************************

   XXX

*******************************************************************************/
#ifdef ENABLE_JVMTI
bool stackTraceCollector(void **target, stackTraceBuffer *buffer)
#else
static bool stackTraceCollector(void **target, stackTraceBuffer *buffer)
#endif
{
	stackTraceBuffer *dest;

	dest = *target = heap_allocate(sizeof(stackTraceBuffer) + buffer->full * sizeof(stacktraceelement), true, 0);

	if (!dest)
		return false;

	memcpy(*target, buffer, sizeof(stackTraceBuffer));
	memcpy(dest + 1, buffer->start, buffer->full * sizeof(stacktraceelement));

	dest->needsFree = 0;
	dest->size = dest->full;
	dest->start = (stacktraceelement *) (dest + 1);

	return true;
}


bool cacao_stacktrace_NormalTrace(void **target)
{
#ifdef ENABLE_JVMTI
	return cacao_stacktrace_fillInStackTrace(target, &stackTraceCollector, NULL);
#else
	return cacao_stacktrace_fillInStackTrace(target, &stackTraceCollector);
#endif

}



static bool classContextCollector(void **target, stackTraceBuffer *buffer)
{
	java_objectarray  *oa;
	stacktraceelement *current;
	stacktraceelement *start;
	size_t size;
	size_t targetSize;
	s4 i;

	size = buffer->full;
	targetSize = 0;

	for (i = 0; i < size; i++)
		if (buffer->start[i].method != 0)
			targetSize++;

	start = buffer->start;
	start++;
	targetSize--;

	if (targetSize > 0) {
		if (start->method &&
			(start->method->class == class_java_lang_SecurityManager)) {
			targetSize--;
			start++;
		}
	}

	oa = builtin_anewarray(targetSize, class_java_lang_Class);

	if (!oa)
		return false;

	for(i = 0, current = start; i < targetSize; i++, current++) {
		if (!current->method) {
			i--;
			continue;
		}

		oa->data[i] = (java_objectheader *) current->method->class;
	}

	*target = oa;

	return true;
}



java_objectarray *cacao_createClassContextArray(void)
{
	java_objectarray *array = NULL;

#ifdef ENABLE_JVMTI
	if (!cacao_stacktrace_fillInStackTrace((void **) &array,
										   &classContextCollector, NULL))
#else
	if (!cacao_stacktrace_fillInStackTrace((void **) &array,
										   &classContextCollector))
#endif
		return NULL;

	return array;
}


/* stacktrace_classLoaderCollector *********************************************

   XXX

*******************************************************************************/

static bool stacktrace_classLoaderCollector(void **target,
											stackTraceBuffer *buffer)
{
	stacktraceelement *current;
	stacktraceelement *start;
	methodinfo        *m;
	ptrint             size;
	s4                 i;

	size = buffer->full;
	start = &(buffer->start[0]);

	for(i = 0, current = start; i < size; i++, current++) {
		m = current->method;

		if (!m)
			continue;

		if (m->class == class_java_security_PrivilegedAction) {
			*target = NULL;
			return true;
		}

		if (m->class->classloader) {
			*target = (java_lang_ClassLoader *) m->class->classloader;
			return true;
		}
	}

	*target = NULL;

	return true;
}


/* cacao_currentClassLoader ****************************************************

   XXX

*******************************************************************************/

java_objectheader *cacao_currentClassLoader(void)
{
	java_objectheader *header = NULL;


#ifdef ENABLE_JVMTI
	if (!cacao_stacktrace_fillInStackTrace((void**)&header,
										   &stacktrace_classLoaderCollector,
										   NULL))
#else
	if (!cacao_stacktrace_fillInStackTrace((void**)&header,
										   &stacktrace_classLoaderCollector))
#endif
		return NULL;

	return header;
}


static bool getStackCollector(void **target, stackTraceBuffer *buffer)
{
	java_objectarray  *oa;
	java_objectarray  *classes;
	java_objectarray  *methodnames;
	java_lang_String  *str;
	classinfo         *c;
	stacktraceelement *current;
	s4                 i, size;

/*  	*result = (java_objectarray **) target; */

	size = buffer->full;

	oa = builtin_anewarray(2, arrayclass_java_lang_Object);

	if (!oa)
		return false;

	classes = builtin_anewarray(size, class_java_lang_Class);

	if (!classes)
		return false;

	methodnames = builtin_anewarray(size, class_java_lang_String);

	if (!methodnames)
		return false;

	oa->data[0] = (java_objectheader *) classes;
	oa->data[1] = (java_objectheader *) methodnames;

	for (i = 0, current = &(buffer->start[0]); i < size; i++, current++) {
		c = current->method->class;

		classes->data[i] = (java_objectheader *) c;
		str = javastring_new(current->method->name);

		if (!str)
			return false;

		methodnames->data[i] = (java_objectheader *) str;
	}

	*target = oa;

	return true;
}


java_objectarray *cacao_getStackForVMAccessController(void)
{
	java_objectarray *result = NULL;

#ifdef ENABLE_JVMTI
	if (!cacao_stacktrace_fillInStackTrace((void **) &result,
										   &getStackCollector,NULL))
#else
	if (!cacao_stacktrace_fillInStackTrace((void **) &result,
										   &getStackCollector))
#endif
		return NULL;

	return result;
}


/* stacktrace_print_trace_from_buffer ******************************************

   Print the stacktrace of a given stackTraceBuffer with CACAO intern
   methods (no Java help). This method is used by
   stacktrace_dump_trace and builtin_trace_exception.

*******************************************************************************/

static void stacktrace_print_trace_from_buffer(stackTraceBuffer *stb)
{
	stacktraceelement *ste;
	methodinfo        *m;
	s4                 i;

	ste = stb->start;

	for (i = 0; i < stb->size; i++, ste++) {
		m = ste->method;

		printf("\tat ");
		utf_display_classname(m->class->name);
		printf(".");
		utf_display(m->name);
		utf_display(m->descriptor);

		if (m->flags & ACC_NATIVE) {
			puts("(Native Method)");

		} else {
			printf("(");
			utf_display(m->class->sourcefile);
			printf(":%d)\n", (u4) ste->linenumber);
		}
	}

	/* just to be sure */

	fflush(stdout);
}


/* stacktrace_dump_trace *******************************************************

   This method is call from signal_handler_sigusr1 to dump the
   stacktrace of the current thread to stdout.

*******************************************************************************/

void stacktrace_dump_trace(void)
{
	stackTraceBuffer      *buffer;

#if 0
	/* get thread stackframeinfo */

	info = &THREADINFO->_stackframeinfo;

	/* fill stackframeinfo structure */

	tmp.oldThreadspecificHeadValue = *info;
	tmp.addressOfThreadspecificHead = info;
	tmp.method = NULL;
	tmp.sp = NULL;
	tmp.ra = _mc->gregs[REG_RIP];

	*info = &tmp;
#endif

	/* generate stacktrace */

	cacao_stacktrace_NormalTrace((void **) &buffer);

	/* print stacktrace */

	if (buffer) {
		stacktrace_print_trace_from_buffer(buffer);

	} else {
		puts("\t<<No stacktrace available>>");
		fflush(stdout);
	}
}


/* stacktrace_print_trace ******************************************************

   Print the stacktrace of a given exception. More or less a wrapper
   to stacktrace_print_trace_from_buffer.

*******************************************************************************/

void stacktrace_print_trace(java_objectheader *xptr)
{
	java_lang_Throwable   *t;
	java_lang_VMThrowable *vmt;
	stackTraceBuffer      *stb;

	t = (java_lang_Throwable *) xptr;

	/* now print the stacktrace */

	vmt = t->vmState;
	stb = (stackTraceBuffer *) vmt->vmData;

	stacktrace_print_trace_from_buffer(stb);
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
