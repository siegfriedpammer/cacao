/* src/vm/jit/trace.cpp - Functions for tracing from java code.

   Copyright (C) 1996-2013
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

#include <cstdio>

#include "md-abi.hpp"

#include "native/llni.hpp"

#include "threads/thread.hpp"

#include "toolbox/logging.hpp"
#include "toolbox/buffer.hpp"

#include "vm/array.hpp"
#include "vm/global.hpp"
#include "vm/globals.hpp"
#include "vm/hook.hpp"
#include "vm/javaobjects.hpp"
#include "vm/options.hpp"
#include "vm/string.hpp"
#include "vm/utf8.hpp"

#include "vm/jit/argument.hpp"
#include "vm/jit/codegen-common.hpp"
#include "vm/jit/trace.hpp"
#include "vm/jit/show.hpp"

#if !defined(NDEBUG)

/* global variables ***********************************************************/

#if !defined(ENABLE_THREADS)
s4 _no_threads_tracejavacallindent = 0;
u4 _no_threads_tracejavacallcount= 0;
#endif


/* trace_java_call_print_argument **********************************************

   XXX: Document me!

*******************************************************************************/

static void trace_java_call_print_argument(Buffer<>& logtext, methodinfo *m, typedesc *paramtype, imm_union imu)
{
	java_object_t *o;
	classinfo     *c;
	Utf8String     u;

	switch (paramtype->type) {
	case TYPE_INT:
		logtext.writef("%d (0x%08x)", (int32_t)imu.l, (int32_t)imu.l);
		break;

	case TYPE_LNG:
#if SIZEOF_VOID_P == 4
		logtext.writef("%lld (0x%016llx)", imu.l, imu.l);
#else
		logtext.writef("%ld (0x%016lx)", imu.l, imu.l);
#endif
		break;

	case TYPE_FLT:
		logtext.writef("%g (0x%08x)", imu.f, imu.i);
		break;

	case TYPE_DBL:
#if SIZEOF_VOID_P == 4
		logtext.writef("%g (0x%016llx)", imu.d, imu.l);
#else
		logtext.writef("%g (0x%016lx)", imu.d, imu.l);
#endif
		break;

	case TYPE_ADR:
#if SIZEOF_VOID_P == 4
		logtext.writef("0x%08x", (ptrint) imu.l);
#else
		logtext.writef("0x%016lx", (ptrint) imu.l);
#endif

		/* Workaround for sun.misc.Unsafe methods.  In the future
		   (exact GC) we should check if the address is on the GC
		   heap. */

		if ((m->clazz       != NULL) &&
			(m->clazz->name == Utf8String::from_utf8("sun/misc/Unsafe")))
			break;

		/* Cast to java.lang.Object. */

		o = (java_handle_t*) (uintptr_t) imu.l;

		/* Check return argument for java.lang.Class or
		   java.lang.String. */

		if (o != NULL) {
			if (o->vftbl->clazz == class_java_lang_String) {
				/* convert java.lang.String object to utf8 string and strcat it to the logtext */

				logtext.write(" (String = \"")
				       .write(o)
				       .write("\")");
			}
			else {
				if (o->vftbl->clazz == class_java_lang_Class) {
					/* if the object returned is a java.lang.Class
					   cast it to classinfo structure and get the name
					   of the class */

					c = (classinfo *) o;

					u = c->name;
				}
				else {
					/* if the object returned is not a java.lang.String or
					   a java.lang.Class just print the name of the class */

					u = o->vftbl->clazz->name;
				}

				/* strcat to the logtext */

				logtext.write(" (Class = \"")
				       .write_slash_to_dot(u)
				       .write("\")");
			}
		}
	}
}

/* trace_java_call_enter ******************************************************
 
   Traces an entry into a java method.

   arg_regs: Array containing all argument registers as int64_t values in
   the same order as listed in m->methoddesc. The array is usually allocated
   on the stack and used for restoring the argument registers later.

   stack: Pointer to first on stack argument in the same format passed to 
   asm_vm_call_method.

*******************************************************************************/

void trace_java_call_enter(methodinfo *m, uint64_t *arg_regs, uint64_t *stack)
{
	methoddesc *md;
	imm_union   arg;
	s4          i;

	/* We can only trace "slow" builtin functions (those with a stub)
	 * here, because the argument passing of "fast" ones happens via
	 * the native ABI and does not fit these functions. */

	if (method_is_builtin(m)) {
		if (!opt_TraceBuiltinCalls)
			return;
	}
	else {
		if (!opt_TraceJavaCalls)
			return;
#if defined(ENABLE_DEBUG_FILTER)
		if (!show_filters_test_verbosecall_enter(m))
			return;
#endif
	}

#if defined(ENABLE_VMLOG)
	vmlog_cacao_enter_method(m);
	return;
#endif

	// Hook point on entry into Java method.
	Hook::method_enter(m);

	md = m->parseddesc;

	// Create new dump memory area.
	DumpMemoryArea dma;

	Buffer<> logtext;

	TRACEJAVACALLCOUNT++;

	logtext.writef("%10d ", TRACEJAVACALLCOUNT);
	logtext.writef("-%d-", TRACEJAVACALLINDENT);

	for (i = 0; i < TRACEJAVACALLINDENT; i++)
		logtext.write('\t');

	logtext.write("called: ");

	if (m->clazz != NULL)
		logtext.write_slash_to_dot(m->clazz->name);
	else
		logtext.write("NULL");
	logtext.write(".");
	logtext.write(m->name);
	logtext.write(m->descriptor);

	if (m->flags & ACC_PUBLIC)         logtext.write(" PUBLIC");
	if (m->flags & ACC_PRIVATE)        logtext.write(" PRIVATE");
	if (m->flags & ACC_PROTECTED)      logtext.write(" PROTECTED");
	if (m->flags & ACC_STATIC)         logtext.write(" STATIC");
	if (m->flags & ACC_FINAL)          logtext.write(" FINAL");
	if (m->flags & ACC_SYNCHRONIZED)   logtext.write(" SYNCHRONIZED");
	if (m->flags & ACC_VOLATILE)       logtext.write(" VOLATILE");
	if (m->flags & ACC_TRANSIENT)      logtext.write(" TRANSIENT");
	if (m->flags & ACC_NATIVE)         logtext.write(" NATIVE");
	if (m->flags & ACC_INTERFACE)      logtext.write(" INTERFACE");
	if (m->flags & ACC_ABSTRACT)       logtext.write(" ABSTRACT");

	logtext.write("(");

	for (i = 0; i < md->paramcount; ++i) {
		arg = argument_jitarray_load(md, i, arg_regs, stack);
		trace_java_call_print_argument(logtext, m, &md->paramtypes[i], arg);
		
		if (i != (md->paramcount - 1)) {
			logtext.write(", ");
		}
	}

	logtext.write(")");

	log_text((char*) logtext);

	TRACEJAVACALLINDENT++;
}

/* trace_java_call_exit ********************************************************

   Traces an exit form a java method.

   return_regs: Array of size 1 containing return register.
   The array is usually allocated on the stack and used for restoring the
   registers later.

*******************************************************************************/

void trace_java_call_exit(methodinfo *m, uint64_t *return_regs)
{
	methoddesc *md;
	s4          i;
	imm_union   val;

	/* We can only trace "slow" builtin functions (those with a stub)
	 * here, because the argument passing of "fast" ones happens via
	 * the native ABI and does not fit these functions. */

	if (method_is_builtin(m)) {
		if (!opt_TraceBuiltinCalls)
			return;
	}
	else {
		if (!opt_TraceJavaCalls)
			return;
#if defined(ENABLE_DEBUG_FILTER)
		if (!show_filters_test_verbosecall_exit(m))
			return;
#endif
	}

#if defined(ENABLE_VMLOG)
	vmlog_cacao_leave_method(m);
	return;
#endif

	// Hook point upon exit from Java method.
	Hook::method_exit(m);

	md = m->parseddesc;

	/* outdent the log message */

	if (TRACEJAVACALLINDENT)
		TRACEJAVACALLINDENT--;
	else
		log_text("trace_java_call_exit: WARNING: unmatched unindent");

	// Create new dump memory area.
	DumpMemoryArea dma;

	Buffer<> logtext;

	/* generate the message */

	logtext.write("           ");
	logtext.writef("-%d-", TRACEJAVACALLINDENT);

	for (i = 0; i < TRACEJAVACALLINDENT; i++)
		logtext.write('\t');

	logtext.write("finished: ");
	if (m->clazz != NULL)
		logtext.write_slash_to_dot(m->clazz->name);
	else
		logtext.write("NULL");
	logtext.write(".");
	logtext.write(m->name);
	logtext.write(m->descriptor);

	if (!IS_VOID_TYPE(md->returntype.type)) {
		logtext.write("->");
		val = argument_jitreturn_load(md, return_regs);

		trace_java_call_print_argument(logtext, m, &md->returntype, val);
	}

	log_text((char*) logtext);
}


/* trace_exception *************************************************************

   Traces an exception which is handled by exceptions_handle_exception.

*******************************************************************************/

void trace_exception(java_object_t *xptr, methodinfo *m, void *pos)
{
	codeinfo *code;

	// Create new dump memory area.
	DumpMemoryArea dma;

	Buffer<> logtext;

	if (xptr) {
		logtext.write("Exception ");
		logtext.write_slash_to_dot(xptr->vftbl->clazz->name);

	} else {
		logtext.write("Some Throwable");
	}

	logtext.write(" thrown in ");

	if (m) {
		logtext.write_slash_to_dot(m->clazz->name);
		logtext.write(".");
		logtext.write(m->name);
		logtext.write(m->descriptor);

		if (m->flags & ACC_SYNCHRONIZED)
			logtext.write("(SYNC");
		else
			logtext.write("(NOSYNC");

		if (m->flags & ACC_NATIVE) {
			logtext.write(",NATIVE");

			code = m->code;

#if SIZEOF_VOID_P == 8
			logtext.writef(
				")(0x%016lx) at position 0x%016lx",
				(ptrint) code->entrypoint, (ptrint) pos);
#else
			logtext.writef(
					")(0x%08x) at position 0x%08x",
					(ptrint) code->entrypoint, (ptrint) pos);
#endif

		} else {

			/* XXX preliminary: This should get the actual codeinfo */
			/* in which the exception happened.                     */
			code = m->code;
			
#if SIZEOF_VOID_P == 8
			logtext.writef(
					")(0x%016lx) at position 0x%016lx (",
					(ptrint) code->entrypoint, (ptrint) pos);
#else
			logtext.writef(
					")(0x%08x) at position 0x%08x (",
					(ptrint) code->entrypoint, (ptrint) pos);
#endif

			if (m->clazz->sourcefile == NULL)
				logtext.write("<NO CLASSFILE INFORMATION>");
			else
				logtext.write(m->clazz->sourcefile);

			logtext.writef(":%d)", 0);
		}

	} else
		logtext.write("call_java_method");

	log_text((char*) logtext);
}


/* trace_exception_builtin *****************************************************

   Traces an exception which is thrown by builtin_throw_exception.

*******************************************************************************/

void trace_exception_builtin(java_handle_t* h)
{
	java_lang_Throwable jlt(h);

	// Get detail message.
	java_handle_t* s = NULL;

	if (jlt.get_handle() != NULL)
		s = jlt.get_detailMessage();

	java_lang_String jls(s);

	// Create new dump memory area.
	DumpMemoryArea dma;

	Buffer<> logtext;

	logtext.write("Builtin exception thrown: ");

	if (jlt.get_handle()) {
		logtext.write_slash_to_dot(jlt.get_vftbl()->clazz->name);

		if (s) {
			logtext.write(": ");
			logtext.write(JavaString(jls.get_handle()));
		}

	} else {
		logtext.write("(nil)");
	}

	log_text((char*) logtext);
}

#endif /* !defined(NDEBUG) */


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
