/* src/vm/jit/stacktrace.c - machine independent stacktrace system

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

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "vm/types.h"

#include "mm/gc-common.h"
#include "mm/memory.h"

#include "vm/jit/stacktrace.h"

#include "vm/global.h"                   /* required here for native includes */
#include "native/jni.h"
#include "native/llni.h"
#include "native/include/java_lang_Throwable.h"

#if defined(WITH_CLASSPATH_GNU)
# include "native/include/java_lang_VMThrowable.h"
#endif

#if defined(ENABLE_THREADS)
# include "threads/native/threads.h"
#else
# include "threads/none/threads.h"
#endif

#include "toolbox/logging.h"

#include "vm/array.h"
#include "vm/builtin.h"
#include "vm/cycles-stats.h"
#include "vm/exceptions.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"

#include "vm/jit/asmpart.h"
#include "vm/jit/codegen-common.h"
#include "vm/jit/linenumbertable.h"
#include "vm/jit/methodheader.h"

#include "vmcore/class.h"
#include "vmcore/loader.h"
#include "vmcore/options.h"


/* global variables ***********************************************************/
#if !defined(ENABLE_THREADS)
stackframeinfo_t *_no_threads_stackframeinfo = NULL;
#endif

CYCLES_STATS_DECLARE(stacktrace_overhead        ,100,1)
CYCLES_STATS_DECLARE(stacktrace_fillInStackTrace,40,5000)
CYCLES_STATS_DECLARE(stacktrace_getClassContext ,40,5000)
CYCLES_STATS_DECLARE(stacktrace_getCurrentClass ,40,5000)
CYCLES_STATS_DECLARE(stacktrace_getStack        ,40,10000)


/* stacktrace_stackframeinfo_add ***********************************************

   Fills a stackframe info structure with the given or calculated
   values and adds it to the chain.

*******************************************************************************/

void stacktrace_stackframeinfo_add(stackframeinfo_t *sfi, u1 *pv, u1 *sp, u1 *ra, u1 *xpc)
{
	stackframeinfo_t **psfi;
	codeinfo          *code;
#if !defined(__I386__) && !defined(__X86_64__) && !defined(__S390__) && !defined(__M68K__)
	bool               isleafmethod;
#endif
#if defined(ENABLE_JIT)
	s4                 framesize;
#endif

	/* get current stackframe info pointer */

	psfi = &STACKFRAMEINFO;

	/* sometimes we don't have pv handy (e.g. in asmpart.S:
       L_asm_call_jit_compiler_exception or in the interpreter). */

	if (pv == NULL) {
#if defined(ENABLE_INTRP)
		if (opt_intrp)
			pv = codegen_get_pv_from_pc(ra);
		else
#endif
			{
#if defined(ENABLE_JIT)
# if defined(__SPARC_64__)
				pv = md_get_pv_from_stackframe(sp);
# else
				pv = md_codegen_get_pv_from_pc(ra);
# endif
#endif
			}
	}

	/* Get codeinfo pointer for the parent Java method. */

	code = code_get_codeinfo_for_pv(pv);

	/* XXX */
/* 	assert(m != NULL); */

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	/* When using the interpreter, we pass RA to the function. */

	if (!opt_intrp) {
# endif
# if defined(__I386__) || defined(__X86_64__) || defined(__S390__) || defined(__M68K__)
		/* On i386 and x86_64 we always have to get the return address
		   from the stack. */
		/* m68k has return address on stack always */
		/* On S390 we use REG_RA as REG_ITMP3, so we have always to get
		   the RA from stack. */

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

	/* Calculate XPC when not given.  The XPC is then the return
	   address of the current method minus 1 because the RA points to
	   the instruction after the call instruction.  This is required
	   e.g. for method stubs. */

	if (xpc == NULL) {
		xpc = (void *) (((intptr_t) ra) - 1);
	}

	/* Fill new stackframeinfo structure. */

	sfi->prev = *psfi;
	sfi->code = code;
	sfi->pv   = pv;
	sfi->sp   = sp;
	sfi->ra   = ra;
	sfi->xpc  = xpc;

#if !defined(NDEBUG)
	if (opt_DebugStackFrameInfo) {
		log_start();
		log_print("[stackframeinfo add   : sfi=%p, method=%p, pv=%p, sp=%p, ra=%p, xpc=%p, method=",
				  sfi, sfi->code->m, sfi->pv, sfi->sp, sfi->ra, sfi->xpc);
		method_print(sfi->code->m);
		log_print("]");
		log_finish();
	}
#endif

	/* Store new stackframeinfo pointer. */

	*psfi = sfi;

	/* set the native world flag for the current thread */
	/* ATTENTION: This flag tells the GC how to treat this thread in case of
	   a collection. Set this flag _after_ a valid stackframe info was set. */

	THREAD_NATIVEWORLD_ENTER;
}


/* stacktrace_stackframeinfo_remove ********************************************

   Remove the given stackframeinfo from the chain in the current
   thread.

*******************************************************************************/

void stacktrace_stackframeinfo_remove(stackframeinfo_t *sfi)
{
	stackframeinfo_t **psfi;

	/* clear the native world flag for the current thread */
	/* ATTENTION: Clear this flag _before_ removing the stackframe info */

	THREAD_NATIVEWORLD_EXIT;

	/* get current stackframe info pointer */

	psfi = &STACKFRAMEINFO;

#if !defined(NDEBUG)
	if (opt_DebugStackFrameInfo) {
		log_start();
		log_print("[stackframeinfo remove: sfi=%p, method=%p, pv=%p, sp=%p, ra=%p, xpc=%p, method=",
				  sfi, sfi->code->m, sfi->pv, sfi->sp, sfi->ra, sfi->xpc);
		method_print(sfi->code->m);
		log_print("]");
		log_finish();
	}
#endif

	/* restore the old pointer */

	*psfi = sfi->prev;
}


/* stacktrace_entry_add ********************************************************

   Adds a new entry to the stacktrace buffer.

*******************************************************************************/

static inline stacktracebuffer *stacktrace_entry_add(stacktracebuffer *stb, methodinfo *m, u2 line)
{
	stacktrace_entry *ste;
	u4                stb_size_old;
	u4                stb_size_new;

	/* check if we already reached the buffer capacity */

	if (stb->used >= stb->capacity) {
		/* calculate size of stacktracebuffer */

		stb_size_old = sizeof(stacktracebuffer) +
					   sizeof(stacktrace_entry) * stb->capacity -
					   sizeof(stacktrace_entry) * STACKTRACE_CAPACITY_DEFAULT;

		stb_size_new = stb_size_old +
					   sizeof(stacktrace_entry) * STACKTRACE_CAPACITY_INCREMENT;

		/* reallocate new memory */

		stb = DMREALLOC(stb, u1, stb_size_old, stb_size_new);

		/* set new buffer capacity */

		stb->capacity = stb->capacity + STACKTRACE_CAPACITY_INCREMENT;
	}

	/* insert the current entry */

	ste = &(stb->entries[stb->used]);

	ste->method     = m;
	ste->linenumber = line;

	/* increase entries used count */

	stb->used += 1;

	return stb;
}


/* stacktrace_method_add *******************************************************

   Add stacktrace entries[1] for the given method to the stacktrace
   buffer.

   IN:
       stb....stacktracebuffer to fill
	   sfi....current stackframeinfo
   OUT:
       stacktracebuffer after possible reallocation.

   [1] In case of inlined methods there may be more than one stacktrace
       entry for a codegen-level method. (see doc/inlining_stacktrace.txt)

*******************************************************************************/

static inline stacktracebuffer *stacktrace_method_add(stacktracebuffer *stb, stackframeinfo_t *sfi)
{
	codeinfo   *code;
	void       *pv;
	void       *xpc;
	methodinfo *m;
	int32_t     linenumber;

	/* Get values from the stackframeinfo. */

	code = sfi->code;
	pv   = sfi->pv;
	xpc  = sfi->xpc;

	m = code->m;

	/* Skip builtin methods. */

	if (m->flags & ACC_METHOD_BUILTIN)
		return stb;

	/* Search the line number table. */

	linenumber = linenumbertable_linenumber_for_pc(&m, code, xpc);

	/* Add a new entry to the staktrace. */

	stb = stacktrace_entry_add(stb, m, linenumber);

	return stb;
}


/* stacktrace_stackframeinfo_fill **********************************************

   Fill the temporary stackframeinfo structure with the values given
   in sfi.

   IN:
       tmpsfi ... temporary stackframeinfo
       sfi ...... stackframeinfo to be used in the next iteration

*******************************************************************************/

static inline void stacktrace_stackframeinfo_fill(stackframeinfo_t *tmpsfi, stackframeinfo_t *sfi)
{
	/* Sanity checks. */

	assert(tmpsfi != NULL);
	assert(sfi != NULL);

	/* Fill the temporary stackframeinfo. */

	tmpsfi->code = sfi->code;
	tmpsfi->pv   = sfi->pv;
	tmpsfi->sp   = sfi->sp;
	tmpsfi->ra   = sfi->ra;
	tmpsfi->xpc  = sfi->xpc;

	/* Set the previous stackframe info of the temporary one to the
	   next in the chain. */

	tmpsfi->prev = sfi->prev;
}


/* stacktrace_stackframeinfo_next **********************************************

   Walk the stack (or the stackframeinfo-chain) to the next method and
   return the new stackframe values in the temporary stackframeinfo
   passed.

   IN:
       tmpsfi ... temporary stackframeinfo of current method

*******************************************************************************/

static inline void stacktrace_stackframeinfo_next(stackframeinfo_t *tmpsfi)
{
	codeinfo         *code;
	void             *pv;
	void             *sp;
	void             *ra;
	void             *xpc;
	uint32_t          framesize;
	stackframeinfo_t *prevsfi;

	/* Sanity check. */

	assert(tmpsfi != NULL);

	/* Get values from the stackframeinfo. */

	code = tmpsfi->code;
	pv   = tmpsfi->pv;
	sp   = tmpsfi->sp;
	ra   = tmpsfi->ra;
	xpc  = tmpsfi->xpc;

	/* Get the current stack frame size. */

	framesize = *((uint32_t *) (((intptr_t) pv) + FrameSize));

	/* Get the RA of the current stack frame (RA to the parent Java
	   method) if the current method is a non-leaf method.  Otherwise
	   the value in the stackframeinfo is correct (from the signal
	   handler). */

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (opt_intrp)
		ra = intrp_md_stacktrace_get_returnaddress(sp, framesize);
	else
# endif
		{
			if (!code_is_leafmethod(code))
				ra = md_stacktrace_get_returnaddress(sp, framesize);
		}
#else
	ra = intrp_md_stacktrace_get_returnaddress(sp, framesize);
#endif

	/* Get the PV for the parent Java method. */

#if defined(ENABLE_INTRP)
	if (opt_intrp)
		pv = codegen_get_pv_from_pc(ra);
	else
#endif
		{
#if defined(ENABLE_JIT)
# if defined(__SPARC_64__)
			sp = md_get_framepointer(sp);
			pv = md_get_pv_from_stackframe(sp);
# else
			pv = md_codegen_get_pv_from_pc(ra);
# endif
#endif
		}

	/* Get the codeinfo pointer for the parent Java method. */

	code = code_get_codeinfo_for_pv(pv);

	/* Calculate the SP for the parent Java method. */

#if defined(ENABLE_INTRP)
	if (opt_intrp)
		sp = *(u1 **) (sp - framesize);
	else
#endif
		{
#if defined(__I386__) || defined (__X86_64__) || defined (__M68K__)
			sp = (void *) (((intptr_t) sp) + framesize + SIZEOF_VOID_P);
#elif defined(__SPARC_64__)
			/* already has the new sp */
#else
			sp = (void *) (((intptr_t) sp) + framesize);
#endif
		}

	/* If the new codeinfo pointer is NULL we reached a
	   asm_vm_call_method function.  In this case we get the next
	   values from the previous stackframeinfo in the chain.
	   Otherwise the new values have been calculated before. */

	if (code == NULL) {
		prevsfi = tmpsfi->prev;

		/* If the previous stackframeinfo in the chain is NULL we
		   reached the top of the stacktrace.  We set code and prev to
		   NULL to mark the end, which is checked in
		   stacktrace_stackframeinfo_end_check. */

		if (prevsfi == NULL) {
			tmpsfi->code = NULL;
			tmpsfi->prev = NULL;
			return;
		}

		/* Fill the temporary stackframeinfo with the new values. */

		stacktrace_stackframeinfo_fill(tmpsfi, prevsfi);
	}
	else {
		/* Store the new values in the stackframeinfo.  NOTE: We
		   subtract 1 from the RA to get the XPC, because the RA
		   points to the instruction after the call instruction. */

		tmpsfi->code = code;
		tmpsfi->pv   = pv;
		tmpsfi->sp   = sp;
		tmpsfi->ra   = ra;
		tmpsfi->xpc  = (void *) (((intptr_t) ra) - 1);
	}
}


/* stacktrace_stackframeinfo_end_check *****************************************

   Check if we reached the end of the stacktrace.

   IN:
       tmpsfi ... temporary stackframeinfo of current method

   RETURN:
       true .... the end is reached
	   false ... the end is not reached

*******************************************************************************/

static inline bool stacktrace_stackframeinfo_end_check(stackframeinfo_t *tmpsfi)
{
	/* Sanity check. */

	assert(tmpsfi != NULL);

	if ((tmpsfi->code == NULL) && (tmpsfi->prev == NULL))
		return true;

	return false;
}


/* stacktrace_create ***********************************************************

   Generates a stacktrace from the thread passed into a
   stacktracebuffer.  The stacktracebuffer is allocated on the
   dump memory.
   
   NOTE: The first element in the stackframe chain must always be a
         native stackframeinfo (e.g. VMThrowable.fillInStackTrace() is
         a native function).

   RETURN VALUE:
      pointer to the stacktracebuffer, or
      NULL if there is no stacktrace available for the
      given thread.

*******************************************************************************/

stacktracebuffer *stacktrace_create(stackframeinfo_t *sfi)
{
	stacktracebuffer *stb;
	stackframeinfo_t  tmpsfi;
	bool              skip_fillInStackTrace;
	bool              skip_init;

	skip_fillInStackTrace = true;
	skip_init             = true;

	/* Create a stacktracebuffer in dump memory. */

	stb = DNEW(stacktracebuffer);

	stb->capacity = STACKTRACE_CAPACITY_DEFAULT;
	stb->used     = 0;

#if !defined(NDEBUG)
	if (opt_DebugStackTrace) {
		printf("\n\n---> stacktrace creation start (fillInStackTrace):\n");
		fflush(stdout);
	}
#endif

	/* Put the data from the stackframeinfo into a temporary one. */

	/* XXX This is not correct, but a workaround for threads-dump for
	   now. */
/* 	assert(sfi != NULL); */
	if (sfi == NULL)
		return NULL;

	/* Iterate till we're done. */

	for (stacktrace_stackframeinfo_fill(&tmpsfi, sfi);
		 stacktrace_stackframeinfo_end_check(&tmpsfi) == false;
		 stacktrace_stackframeinfo_next(&tmpsfi)) {
#if !defined(NDEBUG)
		/* Print current method information. */

		if (opt_DebugStackTrace) {
			log_start();
			log_print("[stacktrace: method=%p, pv=%p, sp=%p, ra=%p, xpc=%p, method=",
					  tmpsfi.code->m, tmpsfi.pv, tmpsfi.sp, tmpsfi.ra,
					  tmpsfi.xpc);
			method_print(tmpsfi.code->m);
			log_print("]");
			log_finish();
		}
#endif

		/* This logic is taken from
		   hotspot/src/share/vm/classfile/javaClasses.cpp
		   (java_lang_Throwable::fill_in_stack_trace). */

		if (skip_fillInStackTrace == true) {
			/* Check "fillInStackTrace" only once, so we negate the
			   flag after the first time check. */

#if defined(WITH_CLASSPATH_GNU)
			/* For GNU Classpath we also need to skip
			   VMThrowable.fillInStackTrace(). */

			if ((tmpsfi.code->m->class == class_java_lang_VMThrowable) &&
				(tmpsfi.code->m->name  == utf_fillInStackTrace))
				continue;
#endif

			skip_fillInStackTrace = false;

			if (tmpsfi.code->m->name == utf_fillInStackTrace)
				continue;
		}

		/* Skip <init> methods of the exceptions klass.  If there is
		   <init> methods that belongs to a superclass of the
		   exception we are going to skipping them in stack trace. */

		if (skip_init == true) {
			if (tmpsfi.code->m->name == utf_init) {
/* 				throwable->is_a(method->method_holder())) { */
				continue;
			}
			else {
				/* If no "Throwable.init()" method found, we stop
				   checking it next time. */

				skip_init = false;
			}
		}

		/* Add this method to the stacktrace. */

		stb = stacktrace_method_add(stb, &tmpsfi);
	}

#if !defined(NDEBUG)
	if (opt_DebugStackTrace) {
		printf("---> stacktrace creation finished.\n\n");
		fflush(stdout);
	}
#endif

	/* return the stacktracebuffer */

	if (stb->used == 0)
		return NULL;
	else
		return stb;
}


/* stacktrace_fillInStackTrace *************************************************

   Generate a stacktrace from the current thread for
   java.lang.VMThrowable.fillInStackTrace.

*******************************************************************************/

java_handle_bytearray_t *stacktrace_fillInStackTrace(void)
{
	stacktracebuffer        *stb;
	java_handle_bytearray_t *ba;
	s4                       ba_size;
	s4                       dumpsize;
	CYCLES_STATS_DECLARE_AND_START_WITH_OVERHEAD

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* create a stacktrace from the current thread */

	stb = stacktrace_create(STACKFRAMEINFO);

	if (stb == NULL)
		goto return_NULL;

	/* allocate memory from the GC heap and copy the stacktrace buffer */
	/* ATTENTION: use a bytearray for this to not confuse the GC */

	ba_size = sizeof(stacktracebuffer) +
	          sizeof(stacktrace_entry) * stb->used -
	          sizeof(stacktrace_entry) * STACKTRACE_CAPACITY_DEFAULT;
	ba = builtin_newarray_byte(ba_size);

	if (ba == NULL)
		goto return_NULL;

	MCOPY(LLNI_array_data(ba), stb, u1, ba_size);

	/* release dump memory */

	dump_release(dumpsize);

	CYCLES_STATS_END_WITH_OVERHEAD(stacktrace_fillInStackTrace,
								   stacktrace_overhead)
	return ba;

return_NULL:
	dump_release(dumpsize);

	CYCLES_STATS_END_WITH_OVERHEAD(stacktrace_fillInStackTrace,
								   stacktrace_overhead)

	return NULL;
}


/* stacktrace_getClassContext **************************************************

   Creates a Class context array.

   RETURN VALUE:
      the array of java.lang.Class objects, or
	  NULL if an exception has been thrown

*******************************************************************************/

java_handle_objectarray_t *stacktrace_getClassContext(void)
{
	stacktracebuffer          *stb;
	stacktrace_entry          *ste;
	java_handle_objectarray_t *oa;
	s4                         oalength;
	s4                         i;
	s4                         dumpsize;
	CYCLES_STATS_DECLARE_AND_START

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* create a stacktrace for the current thread */

	stb = stacktrace_create(STACKFRAMEINFO);

	if (stb == NULL)
		goto return_NULL;

	/* calculate the size of the Class array */

	for (i = 0, oalength = 0; i < stb->used; i++)
		if (stb->entries[i].method != NULL)
			oalength++;

	/* The first entry corresponds to the method whose implementation */
	/* calls stacktrace_getClassContext. We remove that entry.        */

	ste = &(stb->entries[0]);
	ste++;
	oalength--;

	/* allocate the Class array */

	oa = builtin_anewarray(oalength, class_java_lang_Class);
	if (!oa)
		goto return_NULL;

	/* fill the Class array from the stacktracebuffer */

	for(i = 0; i < oalength; i++, ste++) {
		if (ste->method == NULL) {
			i--;
			continue;
		}

		LLNI_array_direct(oa, i) = (java_object_t *) ste->method->class;
	}

	/* release dump memory */

	dump_release(dumpsize);

	CYCLES_STATS_END(stacktrace_getClassContext)

	return oa;

return_NULL:
	dump_release(dumpsize);

	CYCLES_STATS_END(stacktrace_getClassContext)

	return NULL;
}


/* stacktrace_getCurrentClass **************************************************

   Find the current class by walking the stack trace.

   Quote from the JNI documentation:
	 
   In the Java 2 Platform, FindClass locates the class loader
   associated with the current native method.  If the native code
   belongs to a system class, no class loader will be
   involved. Otherwise, the proper class loader will be invoked to
   load and link the named class. When FindClass is called through the
   Invocation Interface, there is no current native method or its
   associated class loader. In that case, the result of
   ClassLoader.getBaseClassLoader is used."

*******************************************************************************/

#if defined(ENABLE_JAVASE)
classinfo *stacktrace_getCurrentClass(void)
{
	stacktracebuffer  *stb;
	stacktrace_entry  *ste;
	methodinfo        *m;
	s4                 i;
	s4                 dumpsize;
	CYCLES_STATS_DECLARE_AND_START

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* create a stacktrace for the current thread */

	stb = stacktrace_create(STACKFRAMEINFO);

	if (stb == NULL)
		goto return_NULL; /* XXX exception: how to distinguish from normal NULL return? */

	/* iterate over all stacktrace entries and find the first suitable
	   class */

	for (i = 0, ste = &(stb->entries[0]); i < stb->used; i++, ste++) {
		m = ste->method;

		if (m == NULL)
			continue;

		if (m->class == class_java_security_PrivilegedAction)
			goto return_NULL;

		if (m->class != NULL) {
			dump_release(dumpsize);

			CYCLES_STATS_END(stacktrace_getCurrentClass)

			return m->class;
		}
	}

	/* no Java method found on the stack */

return_NULL:
	dump_release(dumpsize);

	CYCLES_STATS_END(stacktrace_getCurrentClass)

	return NULL;
}
#endif /* ENABLE_JAVASE */


/* stacktrace_getStack *********************************************************

   Create a 2-dimensional array for java.security.VMAccessControler.

   RETURN VALUE:
      the arrary, or
	  NULL if an exception has been thrown

*******************************************************************************/

#if defined(ENABLE_JAVASE)
java_handle_objectarray_t *stacktrace_getStack(void)
{
	stacktracebuffer          *stb;
	stacktrace_entry          *ste;
	java_handle_objectarray_t *oa;
	java_handle_objectarray_t *classes;
	java_handle_objectarray_t *methodnames;
	classinfo                 *c;
	java_handle_t             *string;
	s4                         i;
	s4                         dumpsize;
	CYCLES_STATS_DECLARE_AND_START

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* create a stacktrace for the current thread */

	stb = stacktrace_create(STACKFRAMEINFO);

	if (stb == NULL)
		goto return_NULL;

	/* get the first stacktrace entry */

	ste = &(stb->entries[0]);

	/* allocate all required arrays */

	oa = builtin_anewarray(2, arrayclass_java_lang_Object);

	if (oa == NULL)
		goto return_NULL;

	classes = builtin_anewarray(stb->used, class_java_lang_Class);

	if (classes == NULL)
		goto return_NULL;

	methodnames = builtin_anewarray(stb->used, class_java_lang_String);

	if (methodnames == NULL)
		goto return_NULL;

	/* set up the 2-dimensional array */

	array_objectarray_element_set(oa, 0, (java_handle_t *) classes);
	array_objectarray_element_set(oa, 1, (java_handle_t *) methodnames);

	/* iterate over all stacktrace entries */

	for (i = 0, ste = &(stb->entries[0]); i < stb->used; i++, ste++) {
		c = ste->method->class;

		LLNI_array_direct(classes, i) = (java_object_t *) c;

		string = javastring_new(ste->method->name);

		if (string == NULL)
			goto return_NULL;

		array_objectarray_element_set(methodnames, i, string);
	}

	/* return the 2-dimensional array */

	dump_release(dumpsize);

	CYCLES_STATS_END(stacktrace_getStack)

	return oa;

return_NULL:
	dump_release(dumpsize);

	CYCLES_STATS_END(stacktrace_getStack)

	return NULL;
}
#endif /* ENABLE_JAVASE */


/* stacktrace_print_trace_from_buffer ******************************************

   Print the stacktrace of a given stacktracebuffer with CACAO intern
   methods (no Java help). This method is used by
   stacktrace_dump_trace and builtin_trace_exception.

*******************************************************************************/

void stacktrace_print_trace_from_buffer(stacktracebuffer *stb)
{
	stacktrace_entry *ste;
	methodinfo       *m;
	s4                i;

	ste = &(stb->entries[0]);

	for (i = 0; i < stb->used; i++, ste++) {
		m = ste->method;

		printf("\tat ");
		utf_display_printable_ascii_classname(m->class->name);
		printf(".");
		utf_display_printable_ascii(m->name);
		utf_display_printable_ascii(m->descriptor);

		if (m->flags & ACC_NATIVE) {
			puts("(Native Method)");

		} else {
			printf("(");
			utf_display_printable_ascii(m->class->sourcefile);
			printf(":%d)\n", (u4) ste->linenumber);
		}
	}

	/* just to be sure */

	fflush(stdout);
}


/* stacktrace_print_exception **************************************************

   Print the stacktrace of a given exception. More or less a wrapper
   to stacktrace_print_trace_from_buffer.

*******************************************************************************/

void stacktrace_print_exception(java_handle_t *h)
{
	java_lang_Throwable     *t;
#if defined(WITH_CLASSPATH_GNU)
	java_lang_VMThrowable   *vmt;
#endif
	java_handle_bytearray_t *ba;
	stacktracebuffer        *stb;

	t = (java_lang_Throwable *) h;

	if (t == NULL)
		return;

	/* now print the stacktrace */

#if defined(WITH_CLASSPATH_GNU)

	LLNI_field_get_ref(t,   vmState, vmt);
	LLNI_field_get_ref(vmt, vmData,  ba);

#elif defined(WITH_CLASSPATH_SUN) || defined(WITH_CLASSPATH_CLDC1_1)

	LLNI_field_get_ref(t, backtrace, ba);

#else
# error unknown classpath configuration
#endif

	assert(ba);
	stb = (stacktracebuffer *) LLNI_array_data(ba);

	stacktrace_print_trace_from_buffer(stb);
}


#if defined(ENABLE_CYCLES_STATS)
void stacktrace_print_cycles_stats(FILE *file)
{
	CYCLES_STATS_PRINT_OVERHEAD(stacktrace_overhead,file);
	CYCLES_STATS_PRINT(stacktrace_fillInStackTrace,file);
	CYCLES_STATS_PRINT(stacktrace_getClassContext ,file);
	CYCLES_STATS_PRINT(stacktrace_getCurrentClass ,file);
	CYCLES_STATS_PRINT(stacktrace_getStack        ,file);
}
#endif


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
