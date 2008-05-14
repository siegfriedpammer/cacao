/* src/vm/jit/stacktrace.c - machine independent stacktrace system

   Copyright (C) 1996-2005, 2006, 2007, 2008
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


#include "config.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "vm/types.h"

#include "md.h"

#include "mm/gc-common.h"
#include "mm/memory.h"

#include "vm/jit/stacktrace.h"

#include "vm/global.h"                   /* required here for native includes */
#include "native/jni.h"
#include "native/llni.h"

#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_Throwable.h"

#if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)
# include "native/include/gnu_classpath_Pointer.h"
# include "native/include/java_lang_VMThrowable.h"
#endif

#include "threads/thread.h"

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
#include "vm/jit/methodtree.h"

#include "vmcore/class.h"
#include "vmcore/loader.h"
#include "vmcore/method.h"
#include "vmcore/options.h"


/* global variables ***********************************************************/

CYCLES_STATS_DECLARE(stacktrace_overhead        , 100, 1)
CYCLES_STATS_DECLARE(stacktrace_fillInStackTrace, 40,  5000)
CYCLES_STATS_DECLARE(stacktrace_get,              40,  5000)
CYCLES_STATS_DECLARE(stacktrace_getClassContext , 40,  5000)
CYCLES_STATS_DECLARE(stacktrace_getCurrentClass , 40,  5000)
CYCLES_STATS_DECLARE(stacktrace_get_stack       , 40,  10000)


/* stacktrace_stackframeinfo_add ***********************************************

   Fills a stackframe info structure with the given or calculated
   values and adds it to the chain.

*******************************************************************************/

void stacktrace_stackframeinfo_add(stackframeinfo_t *sfi, u1 *pv, u1 *sp, u1 *ra, u1 *xpc)
{
	stackframeinfo_t *currentsfi;
	codeinfo         *code;
#if defined(ENABLE_JIT)
	s4                 framesize;
#endif

	/* Get current stackframe info. */

	currentsfi = threads_get_current_stackframeinfo();

	/* sometimes we don't have pv handy (e.g. in asmpart.S:
       L_asm_call_jit_compiler_exception or in the interpreter). */

	if (pv == NULL) {
#if defined(ENABLE_INTRP)
		if (opt_intrp)
			pv = methodtree_find(ra);
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
		/* If the method is a non-leaf function, we need to get the
		   return address from the stack.  For leaf functions the
		   return address is set correctly.  This makes the assembler
		   and the signal handler code simpler.  The code is NULL is
		   the asm_vm_call_method special case. */

		if ((code == NULL) || !code_is_leafmethod(code)) {
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

	sfi->prev = currentsfi;
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

	threads_set_current_stackframeinfo(sfi);

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
	/* Clear the native world flag for the current thread. */
	/* ATTENTION: Clear this flag _before_ removing the stackframe info. */

	THREAD_NATIVEWORLD_EXIT;

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

	/* Set previous stackframe info. */

	threads_set_current_stackframeinfo(sfi->prev);
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

#if !defined(NDEBUG)
	if (opt_DebugStackTrace)
		log_println("[stacktrace fill]");
#endif
}


/* stacktrace_stackframeinfo_next **********************************************

   Walk the stack (or the stackframeinfo-chain) to the next method and
   return the new stackframe values in the temporary stackframeinfo
   passed.

   ATTENTION: This function does NOT skip builtin methods!

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
		pv = methodtree_find(ra);
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

#if !defined(NDEBUG)
	/* Print current method information. */

	if (opt_DebugStackTrace) {
		log_start();
		log_print("[stacktrace: method=%p, pv=%p, sp=%p, ra=%p, xpc=%p, method=",
				  tmpsfi->code->m, tmpsfi->pv, tmpsfi->sp, tmpsfi->ra,
				  tmpsfi->xpc);
		method_print(tmpsfi->code->m);
		log_print("]");
		log_finish();
	}
#endif
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

	if ((tmpsfi->code == NULL) && (tmpsfi->prev == NULL)) {
#if !defined(NDEBUG)
		if (opt_DebugStackTrace)
			log_println("[stacktrace stop]");
#endif

		return true;
	}

	return false;
}


/* stacktrace_depth ************************************************************

   Calculates and returns the depth of the current stacktrace.

   IN:
       sfi ... stackframeinfo where to start the stacktrace

   RETURN:
       depth of the stacktrace

*******************************************************************************/

static int stacktrace_depth(stackframeinfo_t *sfi)
{
	stackframeinfo_t  tmpsfi;
	int               depth;
	methodinfo       *m;

#if !defined(NDEBUG)
	if (opt_DebugStackTrace)
		log_println("[stacktrace_depth]");
#endif

	/* XXX This is not correct, but a workaround for threads-dump for
	   now. */
/* 	assert(sfi != NULL); */
	if (sfi == NULL)
		return 0;

	/* Iterate over all stackframes. */

	depth = 0;

	for (stacktrace_stackframeinfo_fill(&tmpsfi, sfi);
		 stacktrace_stackframeinfo_end_check(&tmpsfi) == false;
		 stacktrace_stackframeinfo_next(&tmpsfi)) {
		/* Get methodinfo. */

		m = tmpsfi.code->m;

		/* Skip builtin methods. */

		if (m->flags & ACC_METHOD_BUILTIN)
			continue;

		depth++;
	}

	return depth;
}


/* stacktrace_get **************************************************************

   Builds and returns a stacktrace starting from the given stackframe
   info and returns the stacktrace structure wrapped in a Java
   byte-array to not confuse the GC.

   IN:
       sfi ... stackframe info to start stacktrace from

   RETURN:
       stacktrace as Java byte-array

*******************************************************************************/

java_handle_bytearray_t *stacktrace_get(stackframeinfo_t *sfi)
{
	stackframeinfo_t         tmpsfi;
	int                      depth;
	java_handle_bytearray_t *ba;
	int32_t                  ba_size;
	stacktrace_t            *st;
	stacktrace_entry_t      *ste;
	methodinfo              *m;
	bool                     skip_fillInStackTrace;
	bool                     skip_init;

	CYCLES_STATS_DECLARE_AND_START_WITH_OVERHEAD

#if !defined(NDEBUG)
	if (opt_DebugStackTrace)
		log_println("[stacktrace_get]");
#endif

	skip_fillInStackTrace = true;
	skip_init             = true;

	depth = stacktrace_depth(sfi);

	if (depth == 0)
		return NULL;

	/* Allocate memory from the GC heap and copy the stacktrace
	   buffer. */
	/* ATTENTION: Use a Java byte-array for this to not confuse the
	   GC. */
	/* FIXME: We waste some memory here as we skip some entries
	   later. */

	ba_size = sizeof(stacktrace_t) + sizeof(stacktrace_entry_t) * depth;

	ba = builtin_newarray_byte(ba_size);

	if (ba == NULL)
		goto return_NULL;

	/* Get a stacktrace entry pointer. */
	/* ATTENTION: We need a critical section here because we use the
	   byte-array data pointer directly. */

	LLNI_CRITICAL_START;

	st = (stacktrace_t *) LLNI_array_data(ba);

	ste = st->entries;

	/* Iterate over the whole stack. */

	for (stacktrace_stackframeinfo_fill(&tmpsfi, sfi);
		 stacktrace_stackframeinfo_end_check(&tmpsfi) == false;
		 stacktrace_stackframeinfo_next(&tmpsfi)) {
		/* Get the methodinfo. */

		m = tmpsfi.code->m;

		/* Skip builtin methods. */

		if (m->flags & ACC_METHOD_BUILTIN)
			continue;

		/* This logic is taken from
		   hotspot/src/share/vm/classfile/javaClasses.cpp
		   (java_lang_Throwable::fill_in_stack_trace). */

		if (skip_fillInStackTrace == true) {
			/* Check "fillInStackTrace" only once, so we negate the
			   flag after the first time check. */

#if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)
			/* For GNU Classpath we also need to skip
			   VMThrowable.fillInStackTrace(). */

			if ((m->clazz == class_java_lang_VMThrowable) &&
				(m->name  == utf_fillInStackTrace))
				continue;
#endif

			skip_fillInStackTrace = false;

			if (m->name == utf_fillInStackTrace)
				continue;
		}

		/* Skip <init> methods of the exceptions klass.  If there is
		   <init> methods that belongs to a superclass of the
		   exception we are going to skipping them in stack trace. */

		if (skip_init == true) {
			if ((m->name == utf_init) &&
				(class_issubclass(m->clazz, class_java_lang_Throwable))) {
				continue;
			}
			else {
				/* If no "Throwable.init()" method found, we stop
				   checking it next time. */

				skip_init = false;
			}
		}

		/* Store the stacktrace entry and increment the pointer. */

		ste->code = tmpsfi.code;
		ste->pc   = tmpsfi.xpc;

		ste++;
	}

	/* Store the number of entries in the stacktrace structure. */

	st->length = ste - st->entries;

	LLNI_CRITICAL_END;

	/* release dump memory */

/* 	dump_release(dumpsize); */

	CYCLES_STATS_END_WITH_OVERHEAD(stacktrace_fillInStackTrace,
								   stacktrace_overhead)
	return ba;

return_NULL:
/* 	dump_release(dumpsize); */

	CYCLES_STATS_END_WITH_OVERHEAD(stacktrace_fillInStackTrace,
								   stacktrace_overhead)

	return NULL;
}


/* stacktrace_get_current ******************************************************

   Builds and returns a stacktrace from the current thread and returns
   the stacktrace structure wrapped in a Java byte-array to not
   confuse the GC.

   RETURN:
       stacktrace as Java byte-array

*******************************************************************************/

java_handle_bytearray_t *stacktrace_get_current(void)
{
	stackframeinfo_t        *sfi;
	java_handle_bytearray_t *ba;

	sfi = threads_get_current_stackframeinfo();
	ba  = stacktrace_get(sfi);

	return ba;
}


/* stacktrace_get_caller_class *************************************************

   Get the class on the stack at the given depth.  This function skips
   various special classes or methods.

   ARGUMENTS:
       depth ... depth to get caller class of

   RETURN:
       caller class

*******************************************************************************/

#if defined(ENABLE_JAVASE)
classinfo *stacktrace_get_caller_class(int depth)
{
	stackframeinfo_t *sfi;
	stackframeinfo_t  tmpsfi;
	methodinfo       *m;
	classinfo        *c;
	int               i;

#if !defined(NDEBUG)
	if (opt_DebugStackTrace)
		log_println("[stacktrace_get_caller_class]");
#endif

	/* Get the stackframeinfo of the current thread. */

	sfi = threads_get_current_stackframeinfo();

	/* Iterate over the whole stack until we reached the requested
	   depth. */

	i = 0;

	for (stacktrace_stackframeinfo_fill(&tmpsfi, sfi);
		 stacktrace_stackframeinfo_end_check(&tmpsfi) == false;
		 stacktrace_stackframeinfo_next(&tmpsfi)) {

		m = tmpsfi.code->m;
		c = m->clazz;

		/* Skip builtin methods. */

		if (m->flags & ACC_METHOD_BUILTIN)
			continue;

#if defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)
		/* NOTE: See hotspot/src/share/vm/runtime/vframe.cpp
		   (vframeStreamCommon::security_get_caller_frame). */

		/* This is java.lang.reflect.Method.invoke(), skip it. */

		if (m == method_java_lang_reflect_Method_invoke)
			continue;

		/* This is an auxiliary frame, skip it. */

		if (class_issubclass(c, class_sun_reflect_MagicAccessorImpl))
			continue;
#endif

		/* We reached the requested depth. */

		if (i >= depth)
			return c;

		i++;
	}

	return NULL;
}
#endif


/* stacktrace_first_nonnull_classloader ****************************************

   Returns the first non-null (user-defined) classloader on the stack.
   If none is found NULL is returned.

   RETURN:
       classloader

*******************************************************************************/

classloader_t *stacktrace_first_nonnull_classloader(void)
{
	stackframeinfo_t *sfi;
	stackframeinfo_t  tmpsfi;
	methodinfo       *m;
	classloader_t    *cl;

#if !defined(NDEBUG)
	if (opt_DebugStackTrace)
		log_println("[stacktrace_first_nonnull_classloader]");
#endif

	/* Get the stackframeinfo of the current thread. */

	sfi = threads_get_current_stackframeinfo();

	/* Iterate over the whole stack. */

	for (stacktrace_stackframeinfo_fill(&tmpsfi, sfi);
		 stacktrace_stackframeinfo_end_check(&tmpsfi) == false;
		 stacktrace_stackframeinfo_next(&tmpsfi)) {

		m  = tmpsfi.code->m;
		cl = class_get_classloader(m->clazz);

		if (cl != NULL)
			return cl;
	}

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
	stackframeinfo_t           *sfi;
	stackframeinfo_t            tmpsfi;
	int                         depth;
	java_handle_objectarray_t  *oa;
	java_object_t             **data;
	int                         i;
	methodinfo                 *m;

	CYCLES_STATS_DECLARE_AND_START

#if !defined(NDEBUG)
	if (opt_DebugStackTrace)
		log_println("[stacktrace_getClassContext]");
#endif

	sfi = threads_get_current_stackframeinfo();

	/* Get the depth of the current stack. */

	depth = stacktrace_depth(sfi);

	/* The first stackframe corresponds to the method whose
	   implementation calls this native function.  We remove that
	   entry. */

	depth--;
	stacktrace_stackframeinfo_fill(&tmpsfi, sfi);
	stacktrace_stackframeinfo_next(&tmpsfi);

	/* Allocate the Class array. */

	oa = builtin_anewarray(depth, class_java_lang_Class);

	if (oa == NULL) {
		CYCLES_STATS_END(stacktrace_getClassContext);

		return NULL;
	}

	/* Fill the Class array from the stacktrace list. */

	LLNI_CRITICAL_START;

	data = LLNI_array_data(oa);

	/* Iterate over the whole stack. */

	i = 0;

	for (;
		 stacktrace_stackframeinfo_end_check(&tmpsfi) == false;
		 stacktrace_stackframeinfo_next(&tmpsfi)) {
		/* Get methodinfo. */

		m = tmpsfi.code->m;

		/* Skip builtin methods. */

		if (m->flags & ACC_METHOD_BUILTIN)
			continue;

		/* Store the class in the array. */

		data[i] = (java_object_t *) m->clazz;

		i++;
	}

	LLNI_CRITICAL_END;

	CYCLES_STATS_END(stacktrace_getClassContext)

	return oa;
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
classinfo *stacktrace_get_current_class(void)
{
	stackframeinfo_t *sfi;
	stackframeinfo_t  tmpsfi;
	methodinfo       *m;

	CYCLES_STATS_DECLARE_AND_START;

#if !defined(NDEBUG)
	if (opt_DebugStackTrace)
		log_println("[stacktrace_get_current_class]");
#endif

	/* Get the stackframeinfo of the current thread. */

	sfi = threads_get_current_stackframeinfo();

	/* If the stackframeinfo is NULL then FindClass is called through
	   the Invocation Interface and we return NULL */

	if (sfi == NULL) {
		CYCLES_STATS_END(stacktrace_getCurrentClass);

		return NULL;
	}

	/* Iterate over the whole stack. */

	for (stacktrace_stackframeinfo_fill(&tmpsfi, sfi);
		 stacktrace_stackframeinfo_end_check(&tmpsfi) == false;
		 stacktrace_stackframeinfo_next(&tmpsfi)) {
		/* Get the methodinfo. */

		m = tmpsfi.code->m;

		if (m->clazz == class_java_security_PrivilegedAction) {
			CYCLES_STATS_END(stacktrace_getCurrentClass);

			return NULL;
		}

		if (m->clazz != NULL) {
			CYCLES_STATS_END(stacktrace_getCurrentClass);

			return m->clazz;
		}
	}

	/* No Java method found on the stack. */

	CYCLES_STATS_END(stacktrace_getCurrentClass);

	return NULL;
}
#endif /* ENABLE_JAVASE */


/* stacktrace_get_stack ********************************************************

   Create a 2-dimensional array for java.security.VMAccessControler.

   RETURN VALUE:
      the arrary, or
         NULL if an exception has been thrown

*******************************************************************************/

#if defined(ENABLE_JAVASE) && defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)
java_handle_objectarray_t *stacktrace_get_stack(void)
{
	stackframeinfo_t          *sfi;
	stackframeinfo_t           tmpsfi;
	int                        depth;
	java_handle_objectarray_t *oa;
	java_handle_objectarray_t *classes;
	java_handle_objectarray_t *methodnames;
	methodinfo                *m;
	java_handle_t             *string;
	int                        i;

	CYCLES_STATS_DECLARE_AND_START

#if !defined(NDEBUG)
	if (opt_DebugStackTrace)
		log_println("[stacktrace_get_stack]");
#endif

	/* Get the stackframeinfo of the current thread. */

	sfi = threads_get_current_stackframeinfo();

	/* Get the depth of the current stack. */

	depth = stacktrace_depth(sfi);

	if (depth == 0)
		return NULL;

	/* Allocate the required arrays. */

	oa = builtin_anewarray(2, arrayclass_java_lang_Object);

	if (oa == NULL)
		goto return_NULL;

	classes = builtin_anewarray(depth, class_java_lang_Class);

	if (classes == NULL)
		goto return_NULL;

	methodnames = builtin_anewarray(depth, class_java_lang_String);

	if (methodnames == NULL)
		goto return_NULL;

	/* Set up the 2-dimensional array. */

	array_objectarray_element_set(oa, 0, (java_handle_t *) classes);
	array_objectarray_element_set(oa, 1, (java_handle_t *) methodnames);

	/* Iterate over the whole stack. */
	/* TODO We should use a critical section here to speed things
	   up. */

	i = 0;

	for (stacktrace_stackframeinfo_fill(&tmpsfi, sfi);
		 stacktrace_stackframeinfo_end_check(&tmpsfi) == false;
		 stacktrace_stackframeinfo_next(&tmpsfi)) {
		/* Get the methodinfo. */

		m = tmpsfi.code->m;

		/* Skip builtin methods. */

		if (m->flags & ACC_METHOD_BUILTIN)
			continue;

		/* Store the class in the array. */
		/* NOTE: We use a LLNI-macro here, because a classinfo is not
		   a handle. */

		LLNI_array_direct(classes, i) = (java_object_t *) m->clazz;

		/* Store the name in the array. */

		string = javastring_new(m->name);

		if (string == NULL)
			goto return_NULL;

		array_objectarray_element_set(methodnames, i, string);

		i++;
	}

	CYCLES_STATS_END(stacktrace_get_stack)

	return oa;

return_NULL:
	CYCLES_STATS_END(stacktrace_get_stack)

	return NULL;
}
#endif


/* stacktrace_print_entry ****************************************************

   Print line for a stacktrace entry.

   ARGUMENTS:
       m ............ methodinfo of the entry
       linenumber ... linenumber of the entry

*******************************************************************************/

static void stacktrace_print_entry(methodinfo *m, int32_t linenumber)
{
	/* Sanity check. */

	assert(m != NULL);

	printf("\tat ");

	if (m->flags & ACC_METHOD_BUILTIN)
		printf("NULL");
	else
		utf_display_printable_ascii_classname(m->clazz->name);

	printf(".");
	utf_display_printable_ascii(m->name);
	utf_display_printable_ascii(m->descriptor);

	if (m->flags & ACC_NATIVE) {
		puts("(Native Method)");
	}
	else {
		if (m->flags & ACC_METHOD_BUILTIN) {
			puts("(builtin)");
		}
		else {
			printf("(");
			utf_display_printable_ascii(m->clazz->sourcefile);
			printf(":%d)\n", linenumber);
		}
	}

	fflush(stdout);
}


/* stacktrace_print ************************************************************

   Print the given stacktrace with CACAO intern methods only (no Java
   code required).

   This method is used by stacktrace_dump_trace and
   builtin_trace_exception.

   IN:
       st ... stacktrace to print

*******************************************************************************/

void stacktrace_print(stacktrace_t *st)
{
	stacktrace_entry_t *ste;
	methodinfo         *m;
	int32_t             linenumber;
	int                 i;

	ste = &(st->entries[0]);

	for (i = 0; i < st->length; i++, ste++) {
		m = ste->code->m;

		/* Get the line number. */

		linenumber = linenumbertable_linenumber_for_pc(&m, ste->code, ste->pc);

		stacktrace_print_entry(m, linenumber);
	}
}


/* stacktrace_print_current ****************************************************

   Print the current stacktrace of the current thread.

   NOTE: This function prints all frames of the stacktrace and does
   not skip frames like stacktrace_get.

*******************************************************************************/

void stacktrace_print_current(void)
{
	stackframeinfo_t *sfi;
	stackframeinfo_t  tmpsfi;
	codeinfo         *code;
	methodinfo       *m;
	int32_t           linenumber;

	sfi = threads_get_current_stackframeinfo();

	if (sfi == NULL) {
		puts("\t<<No stacktrace available>>");
		fflush(stdout);
		return;
	}

	for (stacktrace_stackframeinfo_fill(&tmpsfi, sfi);
		 stacktrace_stackframeinfo_end_check(&tmpsfi) == false;
		 stacktrace_stackframeinfo_next(&tmpsfi)) {
		/* Get the methodinfo. */

		code = tmpsfi.code;
		m    = code->m;

		/* Get the line number. */

		linenumber = linenumbertable_linenumber_for_pc(&m, code, tmpsfi.xpc);

		stacktrace_print_entry(m, linenumber);
	}
}


/* stacktrace_print_of_thread **************************************************

   Print the current stacktrace of the given thread.

   ARGUMENTS:
       t ... thread

*******************************************************************************/

#if defined(ENABLE_THREADS)
void stacktrace_print_of_thread(threadobject *t)
{
	stackframeinfo_t *sfi;
	stackframeinfo_t  tmpsfi;
	codeinfo         *code;
	methodinfo       *m;
	int32_t           linenumber;

	/* Build a stacktrace for the passed thread. */

	sfi = t->_stackframeinfo;
	
	if (sfi == NULL) {
		puts("\t<<No stacktrace available>>");
		fflush(stdout);
		return;
	}

	for (stacktrace_stackframeinfo_fill(&tmpsfi, sfi);
		 stacktrace_stackframeinfo_end_check(&tmpsfi) == false;
		 stacktrace_stackframeinfo_next(&tmpsfi)) {
		/* Get the methodinfo. */

		code = tmpsfi.code;
		m    = code->m;

		/* Get the line number. */

		linenumber = linenumbertable_linenumber_for_pc(&m, code, tmpsfi.xpc);

		stacktrace_print_entry(m, linenumber);
	}
}
#endif


/* stacktrace_print_exception **************************************************

   Print the stacktrace of a given exception (more or less a wrapper
   to stacktrace_print).

   IN:
       h ... handle of exception to print

*******************************************************************************/

void stacktrace_print_exception(java_handle_t *h)
{
	java_lang_Throwable     *o;

#if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)
	java_lang_VMThrowable   *vmt;
#endif

	java_lang_Object        *backtrace;
	java_handle_bytearray_t *ba;
	stacktrace_t            *st;

	o = (java_lang_Throwable *) h;

	if (o == NULL)
		return;

	/* now print the stacktrace */

#if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)

	LLNI_field_get_ref(o,   vmState, vmt);
	LLNI_field_get_ref(vmt, vmdata,  backtrace);

#elif defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK) || defined(WITH_JAVA_RUNTIME_LIBRARY_CLDC1_1)

	LLNI_field_get_ref(o, backtrace, backtrace);

#else
# error unknown classpath configuration
#endif

	ba = (java_handle_bytearray_t *) backtrace;

	/* Sanity check. */

	assert(ba != NULL);

	/* We need a critical section here as we use the byte-array data
	   pointer directly. */

	LLNI_CRITICAL_START;
	
	st = (stacktrace_t *) LLNI_array_data(ba);

	stacktrace_print(st);

	LLNI_CRITICAL_END;
}


#if defined(ENABLE_CYCLES_STATS)
void stacktrace_print_cycles_stats(FILE *file)
{
	CYCLES_STATS_PRINT_OVERHEAD(stacktrace_overhead, file);
	CYCLES_STATS_PRINT(stacktrace_get,               file);
	CYCLES_STATS_PRINT(stacktrace_getClassContext ,  file);
	CYCLES_STATS_PRINT(stacktrace_getCurrentClass ,  file);
	CYCLES_STATS_PRINT(stacktrace_get_stack,         file);
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
