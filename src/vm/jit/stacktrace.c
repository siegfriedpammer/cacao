/* src/vm/jit/stacktrace.c - machine independet stacktrace system

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

   Authors: Joseph Wenninger

   Changes: Christian Thalinger

   $Id: stacktrace.c 4435 2006-02-04 23:59:54Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "vm/types.h"

#include "mm/boehm.h"
#include "mm/memory.h"
#include "native/native.h"

#include "vm/global.h"                   /* required here for native includes */
#include "native/include/java_lang_ClassLoader.h"
#include "native/include/java_lang_Throwable.h"
#include "native/include/java_lang_VMThrowable.h"

#if defined(USE_THREADS)
# if defined(NATIVE_THREADS)
#  include "threads/native/threads.h"
# else
#  include "threads/green/threads.h"
# endif
#else
# include "threads/none/threads.h"
#endif

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


/* linenumbertable_entry ******************************************************/

/* Keep the type of line the same as the pointer type, otherwise we
   run into alignment troubles (like on MIPS64). */

typedef struct linenumbertable_entry linenumbertable_entry;

struct linenumbertable_entry {
	ptrint  line;
	u1     *pc;
};


#if 0
typedef struct lineNumberTableEntryInlineBegin {
	/* this should have the same layout and size as the lineNumberTableEntry */
	ptrint      lineNrOuter;
	methodinfo *method;
} lineNumberTableEntryInlineBegin;
#endif


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

	/* xpc is the same as ra, but is required in stacktrace_create */

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
#if defined(ENABLE_JIT)
	s4               framesize;
#endif

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
#endif /* defined(ENABLE_JIT) */

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


/* stacktrace_add_entry ********************************************************

   Adds a new entry to the stacktrace buffer.

*******************************************************************************/

static void stacktrace_add_entry(stacktracebuffer *stb, methodinfo *m, u2 line)
{
	stacktrace_entry *ste;

	/* check if we already reached the buffer capacity */

	if (stb->used >= stb->capacity) {
		/* reallocate new memory */

		stb->entries = DMREALLOC(stb->entries, stacktrace_entry, stb->capacity,
								 stb->capacity + STACKTRACE_CAPACITY_INCREMENT);

		/* set new buffer capacity */

		stb->capacity = stb->capacity + STACKTRACE_CAPACITY_INCREMENT;
	}

	/* insert the current entry */

	ste = &(stb->entries[stb->used]);

	ste->method     = m;
	ste->linenumber = line;

	/* increase entries used count */

	stb->used += 1;
}


/* stacktrace_fillInStackTrace_method ******************************************

   XXX

*******************************************************************************/

static bool stacktrace_add_method(stacktracebuffer *stb, methodinfo *m, u1 *pv,
								  u1 *pc)
{
	ptrint                 lntsize;     /* size of line number table          */
	u1                    *lntstart;    /* start of line number table         */
	linenumbertable_entry *lntentry;    /* points to last entry in the table  */

	/* get size of line number table */

	lntsize  = *((ptrint *) (pv + LineNumberTableSize));
	lntstart = *((u1 **)    (pv + LineNumberTableStart));

	/* Subtract the size of the line number entry of the structure,
	   since the line number table start points to the pc. */

	lntentry = (linenumbertable_entry *) (lntstart - SIZEOF_VOID_P);

	/* Find the line number for the specified PC (going backwards
	   in the linenumber table). The linenumber table size is zero
	   in native stubs. */

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
					stacktrace_add_entry(buffer, m, ilStart->lineNrOuter);

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
				stacktrace_add_entry(stb, m, lntentry->line);
				return true;
			}
		}
	}

	/* check if we are before the actual JIT code */

	if ((ptrint) pc < (ptrint) m->entrypoint) {
		dolog("Current PC before start of code: %p < %p", pc, m->entrypoint);
		assert(0);
	}

	/* If we get here, just add the entry with line number 0. */

	stacktrace_add_entry(stb, m, 0);

	return true;
}


/* stacktrace_create ***********************************************************

   Generates a stacktrace from the thread passed into a
   stacktracebuffer.  The stacktracebuffer is allocated on the GC
   heap.

*******************************************************************************/

stacktracebuffer *stacktrace_create(threadobject* thread)
{
	stacktracebuffer *stb;
	stacktracebuffer *gcstb;
	s4                dumpsize;
	stackframeinfo   *sfi;
	methodinfo       *m;
	u1               *pv;
	u1               *sp;
	u4                framesize;
	u1               *ra;
	u1               *xpc;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* prevent compiler warnings */

	pv = NULL;
	sp = NULL;
	ra = NULL;

	/* create a stacktracebuffer in dump memory */

	stb = DNEW(stacktracebuffer);

	stb->capacity = STACKTRACE_CAPACITY_DEFAULT;
	stb->used     = 0;
	stb->entries  = DMNEW(stacktrace_entry, STACKTRACE_CAPACITY_DEFAULT);

	/* The first element in the stackframe chain must always be a
	   native stackframeinfo (VMThrowable.fillInStackTrace is a native
	   function). */

#if defined(USE_THREADS)
	sfi = thread->info._stackframeinfo;
#else
	sfi = &_no_threads_stackframeinfo;
#endif

#define PRINTMETHODS 0

#if PRINTMETHODS
	printf("\n\nfillInStackTrace start:\n");
	fflush(stdout);
#endif

	/* Loop while we have a method pointer (asm_calljavafunction has
	   NULL) or there is a stackframeinfo in the chain. */

	m = NULL;

	while (m || sfi) {
		/* m == NULL should only happen for the first time and inline
		   stackframe infos, like from the exception stubs or the
		   patcher wrapper. */

		if (m == NULL) {
			/* for native stub stackframe infos, pv is always NULL */

			if (sfi->pv == NULL) {
				/* get methodinfo, sp and ra from the current stackframe info */

				m  = sfi->method;
				sp = sfi->sp;           /* sp of parent Java function         */
				ra = sfi->ra;

				if (m)
					stacktrace_add_entry(stb, m, 0);

#if PRINTMETHODS
				printf("ra=%p sp=%p, ", ra, sp);
				method_print(m);
				printf(": native stub\n");
				fflush(stdout);
#endif
				/* This is an native stub stackframe info, so we can
				   get the parent pv from the return address
				   (ICMD_INVOKE*). */

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
				/* Inline stackframe infos are special: they have a
				   xpc of the actual exception position and the return
				   address saved since an inline stackframe info can
				   also be in a leaf method (no return address saved
				   on stack!!!).  ATTENTION: This one is also for
				   hardware exceptions!!! */

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
					method_print(m);
					printf(": inline stub parent");
					fflush(stdout);
#endif

#if defined(ENABLE_INTRP)
					if (!opt_intrp) {
#endif

					/* add the method to the stacktrace */

					stacktrace_add_method(stb, m, pv, (u1 *) ((ptrint) xpc));

					/* get the current stack frame size */

					framesize = *((u4 *) (pv + FrameSize));

#if PRINTMETHODS
					printf(", framesize=%d\n", framesize);
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
			method_print(m);
			printf(": JIT");
			fflush(stdout);
#endif

			/* JIT method found, add it to the stacktrace (we subtract
			   1 from the return address since it points the the
			   instruction after call). */

			stacktrace_add_method(stb, m, pv, (u1 *) ((ptrint) ra) - 1);

			/* get the current stack frame size */

			framesize = *((u4 *) (pv + FrameSize));

#if PRINTMETHODS
			printf(", framesize=%d\n", framesize);
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
				sp = *(u1 **) (sp - framesize);
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

	/* allocate memory from the GC heap and copy the stacktrace buffer */

	gcstb = GCNEW(stacktracebuffer);


	gcstb->capacity = stb->capacity;
	gcstb->used     = stb->used;
	gcstb->entries  = GCMNEW(stacktrace_entry, stb->used);

	MCOPY(gcstb->entries, stb->entries, stacktrace_entry, stb->used);

	/* just to be sure */

	stb = NULL;
	assert(gcstb != NULL);

	/* release dump memory */

	dump_release(dumpsize);

	/* return the stacktracebuffer */

	return gcstb;
}


/* stacktrace_fillInStackTrace *************************************************

   Generate a stacktrace from the current thread for
   java.lang.VMThrowable.fillInStackTrace.

*******************************************************************************/

stacktracebuffer *stacktrace_fillInStackTrace(void)
{
	stacktracebuffer *stb;

	/* create a stacktrace from the current thread */

	stb = stacktrace_create(THREADOBJECT);

	return stb;
}


/* stacktrace_getClassContext **************************************************

   Creates a Class context array.

*******************************************************************************/

java_objectarray *stacktrace_getClassContext(void)
{
	stacktracebuffer  *stb;
	stacktrace_entry  *ste;
	java_objectarray  *oa;
	s4                 oalength;
	s4                 i;

	/* create a stacktrace for the current thread */

	stb = stacktrace_create(THREADOBJECT);

	/* calculate the size of the Class array */

	for (i = 0, oalength = 0; i < stb->used; i++)
		if (stb->entries[i].method != NULL)
			oalength++;

	ste = &(stb->entries[0]);
	ste++;
	oalength--;

	/* XXX document me */

	if (oalength > 0) {
		if (ste->method &&
			(ste->method->class == class_java_lang_SecurityManager)) {
			ste++;
			oalength--;
		}
	}

	/* allocate the Class array */

	oa = builtin_anewarray(oalength, class_java_lang_Class);

	if (!oa)
		return NULL;

	/* fill the Class array from the stacktracebuffer */

	for(i = 0; i < oalength; i++, ste++) {
		if (ste->method == NULL) {
			i--;
			continue;
		}

		oa->data[i] = (java_objectheader *) ste->method->class;
	}

	return oa;
}


/* stacktrace_getCurrentClassLoader ********************************************

   XXX

*******************************************************************************/

java_objectheader *stacktrace_getCurrentClassLoader(void)
{
	stacktracebuffer  *stb;
	stacktrace_entry  *ste;
	methodinfo        *m;
	java_objectheader *cl;
	s4                 i;

	cl = NULL;

	/* create a stacktrace for the current thread */

	stb = stacktrace_create(THREADOBJECT);

	/* iterate over all stacktrace entries and find the first suitable
	   classloader */

	for (i = 0, ste = &(stb->entries[0]); i < stb->used; i++, ste++) {
		m = ste->method;

		if (!m)
			continue;

		if (m->class == class_java_security_PrivilegedAction) {
			cl = NULL;
			break;
		}

		if (m->class->classloader) {
			cl = m->class->classloader;
			break;
		}
	}

	/* return the classloader */

	return cl;
}


/* stacktrace_getStack *********************************************************

   Create a 2-dimensional array for java.security.VMAccessControler.

*******************************************************************************/

java_objectarray *stacktrace_getStack(void)
{
	stacktracebuffer *stb;
	stacktrace_entry *ste;
	java_objectarray *oa;
	java_objectarray *classes;
	java_objectarray *methodnames;
	classinfo        *c;
	java_lang_String *str;
	s4                i;

	/* create a stacktrace for the current thread */

	stb = stacktrace_create(THREADOBJECT);

	/* get the first stacktrace entry */

	ste = &(stb->entries[0]);

	/* allocate all required arrays */

	oa = builtin_anewarray(2, arrayclass_java_lang_Object);

	if (!oa)
		return NULL;

	classes = builtin_anewarray(stb->used, class_java_lang_Class);

	if (!classes)
		return NULL;

	methodnames = builtin_anewarray(stb->used, class_java_lang_String);

	if (!methodnames)
		return NULL;

	/* set up the 2-dimensional array */

	oa->data[0] = (java_objectheader *) classes;
	oa->data[1] = (java_objectheader *) methodnames;

	/* iterate over all stacktrace entries */

	for (i = 0, ste = &(stb->entries[0]); i < stb->used; i++, ste++) {
		c = ste->method->class;

		classes->data[i] = (java_objectheader *) c;
		str = javastring_new(ste->method->name);

		if (!str)
			return NULL;

		methodnames->data[i] = (java_objectheader *) str;
	}

	/* return the 2-dimensional array */

	return oa;
}


/* stacktrace_print_trace_from_buffer ******************************************

   Print the stacktrace of a given stacktracebuffer with CACAO intern
   methods (no Java help). This method is used by
   stacktrace_dump_trace and builtin_trace_exception.

*******************************************************************************/

static void stacktrace_print_trace_from_buffer(stacktracebuffer *stb)
{
	stacktrace_entry *ste;
	methodinfo       *m;
	s4                i;

	ste = &(stb->entries[0]);

	for (i = 0; i < stb->used; i++, ste++) {
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
	stackframeinfo   *psfi;
	stacktracebuffer *stb;

#if 0
	/* get methodinfo pointer from data segment */

	m = *((methodinfo **) (pv + MethodPointer));

	/* get current stackframe info pointer */

	psfi = STACKFRAMEINFO;

	/* fill new stackframe info structure */

	sfi->prev   = *psfi;
	sfi->method = NULL;
	sfi->pv     = NULL;
	sfi->sp     = sp;
	sfi->ra     = ra;

	/* store new stackframe info pointer */

	*psfi = sfi;
#endif

	/* create a stacktrace for the current thread */

	stb = stacktrace_create(THREADOBJECT);

	/* print stacktrace */

	if (stb) {
		stacktrace_print_trace_from_buffer(stb);

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
	stacktracebuffer      *stb;

	t = (java_lang_Throwable *) xptr;

	/* now print the stacktrace */

	vmt = t->vmState;
	stb = (stacktracebuffer *) vmt->vmData;

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
 * vim:noexpandtab:sw=4:ts=4:
 */
