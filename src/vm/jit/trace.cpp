/* src/vm/jit/trace.cpp - Functions for tracing from java code.

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

#include <stdio.h>

#include "arch.h"
#include "md-abi.h"

#include "mm/memory.hpp"

#include "native/llni.h"

#include "threads/thread.hpp"

#include "toolbox/logging.hpp"

#include "vm/array.hpp"
#include "vm/global.h"
#include "vm/globals.hpp"
#include "vm/javaobjects.hpp"
#include "vm/options.h"
#include "vm/string.hpp"
#include "vm/utf8.h"

#include "vm/jit/argument.hpp"
#include "vm/jit/codegen-common.hpp"
#include "vm/jit/trace.hpp"
#include "vm/jit/show.hpp"


#if !defined(NDEBUG)

// FIXME For now we export everything as C functions.
extern "C" {

/* global variables ***********************************************************/

#if !defined(ENABLE_THREADS)
s4 _no_threads_tracejavacallindent = 0;
u4 _no_threads_tracejavacallcount= 0;
#endif


/* trace_java_call_print_argument **********************************************

   XXX: Document me!

*******************************************************************************/

static char *trace_java_call_print_argument(methodinfo *m, char *logtext, s4 *logtextlen, typedesc *paramtype, imm_union imu)
{
	java_object_t *o;
	classinfo     *c;
	utf           *u;
	u4             len;

	switch (paramtype->type) {
	case TYPE_INT:
		sprintf(logtext + strlen(logtext), "%d (0x%08x)", (int32_t)imu.l, (int32_t)imu.l);
		break;

	case TYPE_LNG:
#if SIZEOF_VOID_P == 4
		sprintf(logtext + strlen(logtext), "%lld (0x%016llx)", imu.l, imu.l);
#else
		sprintf(logtext + strlen(logtext), "%ld (0x%016lx)", imu.l, imu.l);
#endif
		break;

	case TYPE_FLT:
		sprintf(logtext + strlen(logtext), "%g (0x%08x)", imu.f, imu.i);
		break;

	case TYPE_DBL:
#if SIZEOF_VOID_P == 4
		sprintf(logtext + strlen(logtext), "%g (0x%016llx)", imu.d, imu.l);
#else
		sprintf(logtext + strlen(logtext), "%g (0x%016lx)", imu.d, imu.l);
#endif
		break;

	case TYPE_ADR:
#if SIZEOF_VOID_P == 4
		sprintf(logtext + strlen(logtext), "0x%08x", (ptrint) imu.l);
#else
		sprintf(logtext + strlen(logtext), "0x%016lx", (ptrint) imu.l);
#endif

		/* Workaround for sun.misc.Unsafe methods.  In the future
		   (exact GC) we should check if the address is on the GC
		   heap. */

		if ((m->clazz       != NULL) &&
			(m->clazz->name == utf_new_char("sun/misc/Unsafe")))
			break;

		/* Cast to java.lang.Object. */

		o = (java_handle_t*) (uintptr_t) imu.l;

		/* Check return argument for java.lang.Class or
		   java.lang.String. */

		if (o != NULL) {
			if (o->vftbl->clazz == class_java_lang_String) {
				/* get java.lang.String object and the length of the
				   string */

				u = javastring_toutf(o, false);

				len = strlen(" (String = \"") + utf_bytes(u) + strlen("\")");

				/* realloc memory for string length */

				logtext = (char*) DumpMemory::reallocate(logtext, *logtextlen, *logtextlen + len);
				*logtextlen += len;

				/* convert to utf8 string and strcat it to the logtext */

				strcat(logtext, " (String = \"");
				utf_cat(logtext, u);
				strcat(logtext, "\")");
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

				len = strlen(" (Class = \"") + utf_bytes(u) + strlen("\")");

				/* realloc memory for string length */

				logtext = (char*) DumpMemory::reallocate(logtext, *logtextlen, *logtextlen + len);
				*logtextlen += len;

				/* strcat to the logtext */

				strcat(logtext, " (Class = \"");
				utf_cat_classname(logtext, u);
				strcat(logtext, "\")");
			}
		}
	}

	return logtext;
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
	char       *logtext;
	s4          logtextlen;
	s4          i;
	s4          pos;

	/* We don't trace builtin functions here because the argument
	   passing happens via the native ABI and does not fit these
	   functions. */

	if (method_is_builtin(m))
		return;

#if defined(ENABLE_DEBUG_FILTER)
	if (!show_filters_test_verbosecall_enter(m))
		return;
#endif

#if defined(ENABLE_VMLOG)
	vmlog_cacao_enter_method(m);
	return;
#endif

	md = m->parseddesc;

	/* calculate message length */

	logtextlen =
		strlen("4294967295 ") +
		strlen("-2147483647-") +        /* INT_MAX should be sufficient       */
		TRACEJAVACALLINDENT +
		strlen("called: ") +
		((m->clazz == NULL) ? strlen("NULL") : utf_bytes(m->clazz->name)) +
		strlen(".") +
		utf_bytes(m->name) +
		utf_bytes(m->descriptor);

	/* Actually it's not possible to have all flags printed, but:
	   safety first! */

	logtextlen +=
		strlen(" PUBLIC") +
		strlen(" PRIVATE") +
		strlen(" PROTECTED") +
		strlen(" STATIC") +
		strlen(" FINAL") +
		strlen(" SYNCHRONIZED") +
		strlen(" VOLATILE") +
		strlen(" TRANSIENT") +
		strlen(" NATIVE") +
		strlen(" INTERFACE") +
		strlen(" ABSTRACT") +
		strlen(" METHOD_BUILTIN");

	/* add maximal argument length */

	logtextlen +=
		strlen("(") +
		strlen("-9223372036854775808 (0x123456789abcdef0), ") * md->paramcount +
		strlen("...(255)") +
		strlen(")");

	// Create new dump memory area.
	DumpMemoryArea dma;

	// TODO Use a std::string here.
	logtext = (char*) DumpMemory::allocate(sizeof(char) * logtextlen);

	TRACEJAVACALLCOUNT++;

	sprintf(logtext, "%10d ", TRACEJAVACALLCOUNT);
	sprintf(logtext + strlen(logtext), "-%d-", TRACEJAVACALLINDENT);

	pos = strlen(logtext);

	for (i = 0; i < TRACEJAVACALLINDENT; i++)
		logtext[pos++] = '\t';

	strcpy(logtext + pos, "called: ");

	if (m->clazz != NULL)
		utf_cat_classname(logtext, m->clazz->name);
	else
		strcat(logtext, "NULL");
	strcat(logtext, ".");
	utf_cat(logtext, m->name);
	utf_cat(logtext, m->descriptor);

	if (m->flags & ACC_PUBLIC)         strcat(logtext, " PUBLIC");
	if (m->flags & ACC_PRIVATE)        strcat(logtext, " PRIVATE");
	if (m->flags & ACC_PROTECTED)      strcat(logtext, " PROTECTED");
	if (m->flags & ACC_STATIC)         strcat(logtext, " STATIC");
	if (m->flags & ACC_FINAL)          strcat(logtext, " FINAL");
	if (m->flags & ACC_SYNCHRONIZED)   strcat(logtext, " SYNCHRONIZED");
	if (m->flags & ACC_VOLATILE)       strcat(logtext, " VOLATILE");
	if (m->flags & ACC_TRANSIENT)      strcat(logtext, " TRANSIENT");
	if (m->flags & ACC_NATIVE)         strcat(logtext, " NATIVE");
	if (m->flags & ACC_INTERFACE)      strcat(logtext, " INTERFACE");
	if (m->flags & ACC_ABSTRACT)       strcat(logtext, " ABSTRACT");

	strcat(logtext, "(");

	for (i = 0; i < md->paramcount; ++i) {
		arg = argument_jitarray_load(md, i, arg_regs, stack);
		logtext = trace_java_call_print_argument(m, logtext, &logtextlen,
												 &md->paramtypes[i], arg);
		if (i != (md->paramcount - 1)) {
			strcat(logtext, ", ");
		}
	}

	strcat(logtext, ")");

	log_text(logtext);

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
	char       *logtext;
	s4          logtextlen;
	s4          i;
	s4          pos;
	imm_union   val;

	/* We don't trace builtin functions here because the argument
	   passing happens via the native ABI and does not fit these
	   functions. */

	if (method_is_builtin(m))
		return;

#if defined(ENABLE_DEBUG_FILTER)
	if (!show_filters_test_verbosecall_exit(m))
		return;
#endif

#if defined(ENABLE_VMLOG)
	vmlog_cacao_leave_method(m);
	return;
#endif

	md = m->parseddesc;

	/* outdent the log message */

	if (TRACEJAVACALLINDENT)
		TRACEJAVACALLINDENT--;
	else
		log_text("trace_java_call_exit: WARNING: unmatched unindent");

	/* calculate message length */

	logtextlen =
		strlen("4294967295 ") +
		strlen("-2147483647-") +        /* INT_MAX should be sufficient       */
		TRACEJAVACALLINDENT +
		strlen("finished: ") +
		((m->clazz == NULL) ? strlen("NULL") : utf_bytes(m->clazz->name)) +
		strlen(".") +
		utf_bytes(m->name) +
		utf_bytes(m->descriptor) +
		strlen(" SYNCHRONIZED") + strlen("(") + strlen(")");

	/* add maximal argument length */

	logtextlen += strlen("->0.4872328470301428 (0x0123456789abcdef)");

	// Create new dump memory area.
	DumpMemoryArea dma;

	// TODO Use a std::string here.
	logtext = (char*) DumpMemory::allocate(sizeof(char) * logtextlen);

	/* generate the message */

	sprintf(logtext, "           ");
	sprintf(logtext + strlen(logtext), "-%d-", TRACEJAVACALLINDENT);

	pos = strlen(logtext);

	for (i = 0; i < TRACEJAVACALLINDENT; i++)
		logtext[pos++] = '\t';

	strcpy(logtext + pos, "finished: ");
	if (m->clazz != NULL)
		utf_cat_classname(logtext, m->clazz->name);
	else
		strcat(logtext, "NULL");
	strcat(logtext, ".");
	utf_cat(logtext, m->name);
	utf_cat(logtext, m->descriptor);

	if (!IS_VOID_TYPE(md->returntype.type)) {
		strcat(logtext, "->");
		val = argument_jitreturn_load(md, return_regs);

		logtext =
			trace_java_call_print_argument(m, logtext, &logtextlen,
										   &md->returntype, val);
	}

	log_text(logtext);
}


/* trace_exception *************************************************************

   Traces an exception which is handled by exceptions_handle_exception.

*******************************************************************************/

void trace_exception(java_object_t *xptr, methodinfo *m, void *pos)
{
	char *logtext;
	s4    logtextlen;
	codeinfo *code;

	/* calculate message length */

	if (xptr) {
		logtextlen =
			strlen("Exception ") + utf_bytes(xptr->vftbl->clazz->name);
	} 
	else {
		logtextlen = strlen("Some Throwable");
	}

	logtextlen += strlen(" thrown in ");

	if (m) {
		logtextlen +=
			utf_bytes(m->clazz->name) +
			strlen(".") +
			utf_bytes(m->name) +
			utf_bytes(m->descriptor) +
			strlen("(NOSYNC,NATIVE");

#if SIZEOF_VOID_P == 8
		logtextlen +=
			strlen(")(0x123456789abcdef0) at position 0x123456789abcdef0 (");
#else
		logtextlen += strlen(")(0x12345678) at position 0x12345678 (");
#endif

		if (m->clazz->sourcefile == NULL)
			logtextlen += strlen("<NO CLASSFILE INFORMATION>");
		else
			logtextlen += utf_bytes(m->clazz->sourcefile);

		logtextlen += strlen(":65536)");

	} 
	else {
		logtextlen += strlen("call_java_method");
	}

	logtextlen += strlen("0");

	// Create new dump memory area.
	DumpMemoryArea dma;

	// TODO Use a std::string here.
	logtext = (char*) DumpMemory::allocate(sizeof(char) * logtextlen);

	if (xptr) {
		strcpy(logtext, "Exception ");
		utf_cat_classname(logtext, xptr->vftbl->clazz->name);

	} else {
		strcpy(logtext, "Some Throwable");
	}

	strcat(logtext, " thrown in ");

	if (m) {
		utf_cat_classname(logtext, m->clazz->name);
		strcat(logtext, ".");
		utf_cat(logtext, m->name);
		utf_cat(logtext, m->descriptor);

		if (m->flags & ACC_SYNCHRONIZED)
			strcat(logtext, "(SYNC");
		else
			strcat(logtext, "(NOSYNC");

		if (m->flags & ACC_NATIVE) {
			strcat(logtext, ",NATIVE");

			code = m->code;

#if SIZEOF_VOID_P == 8
			sprintf(logtext + strlen(logtext),
					")(0x%016lx) at position 0x%016lx",
					(ptrint) code->entrypoint, (ptrint) pos);
#else
			sprintf(logtext + strlen(logtext),
					")(0x%08x) at position 0x%08x",
					(ptrint) code->entrypoint, (ptrint) pos);
#endif

		} else {

			/* XXX preliminary: This should get the actual codeinfo */
			/* in which the exception happened.                     */
			code = m->code;
			
#if SIZEOF_VOID_P == 8
			sprintf(logtext + strlen(logtext),
					")(0x%016lx) at position 0x%016lx (",
					(ptrint) code->entrypoint, (ptrint) pos);
#else
			sprintf(logtext + strlen(logtext),
					")(0x%08x) at position 0x%08x (",
					(ptrint) code->entrypoint, (ptrint) pos);
#endif

			if (m->clazz->sourcefile == NULL)
				strcat(logtext, "<NO CLASSFILE INFORMATION>");
			else
				utf_cat(logtext, m->clazz->sourcefile);

			sprintf(logtext + strlen(logtext), ":%d)", 0);
		}

	} else
		strcat(logtext, "call_java_method");

	log_text(logtext);
}


/* trace_exception_builtin *****************************************************

   Traces an exception which is thrown by builtin_throw_exception.

*******************************************************************************/

void trace_exception_builtin(java_handle_t* h)
{
	char                *logtext;
	s4                   logtextlen;

	java_lang_Throwable jlt(h);

	// Get detail message.
	java_handle_t* s = NULL;

	if (jlt.get_handle() != NULL)
		s = jlt.get_detailMessage();

	java_lang_String jls(s);

	/* calculate message length */

	logtextlen = strlen("Builtin exception thrown: ") + strlen("0");

	if (jlt.get_handle() != NULL) {
		logtextlen += utf_bytes(jlt.get_vftbl()->clazz->name);

		if (jls.get_handle()) {
			CharArray ca(jls.get_value());
			// FIXME This is not handle capable!
			uint16_t* ptr = (uint16_t*) ca.get_raw_data_ptr();
			logtextlen += strlen(": ") +
				u2_utflength(ptr + jls.get_offset(), jls.get_count());
		}
	} 
	else {
		logtextlen += strlen("(nil)");
	}

	// Create new dump memory area.
	DumpMemoryArea dma;

	logtext = (char*) DumpMemory::allocate(sizeof(char) * logtextlen);

	strcpy(logtext, "Builtin exception thrown: ");

	if (jlt.get_handle()) {
		utf_cat_classname(logtext, jlt.get_vftbl()->clazz->name);

		if (s) {
			char *buf;

			buf = javastring_tochar(jls.get_handle());
			strcat(logtext, ": ");
			strcat(logtext, buf);
			MFREE(buf, char, strlen(buf) + 1);
		}

	} else {
		strcat(logtext, "(nil)");
	}

	log_text(logtext);
}

} // extern "C"

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
