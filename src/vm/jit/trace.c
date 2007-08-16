/* src/vm/jit/trace.c - Functions for tracing from java code.

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

   $Id: trace.c 8317 2007-08-16 06:53:26Z pm $

*/

#include "arch.h"
#include "md-abi.h"

#include "mm/memory.h"

#if defined(ENABLE_THREADS)
#include "threads/native/threads.h"
#else
#include "threads/none/threads.h"
#endif

#include "toolbox/logging.h"

#include "vm/global.h"
#include "vm/stringlocal.h"
#include "vm/jit/codegen-common.h"
#include "vm/jit/trace.h"
#include "vm/jit/show.h"

#include "vmcore/utf8.h"

#include <stdio.h>

#if !defined(NDEBUG)

#if !defined(ENABLE_THREADS)
s4 _no_threads_tracejavacallindent = 0;
u4 _no_threads_tracejavacallcount= 0;
#endif

/* _array_load_param **********************************************************
 
   Returns the argument specified by pd and td from one of the passed arrays
   and returns it.

*******************************************************************************/

static imm_union _array_load_param(paramdesc *pd, typedesc *td, uint64_t *arg_regs, uint64_t *stack) {
	imm_union ret;

	switch (td->type) {
		case TYPE_INT:
		case TYPE_ADR:
			if (pd->inmemory) {
#if (SIZEOF_VOID_P == 8)
				ret.l = (int64_t)stack[pd->index];
#else
				ret.l = *(int32_t *)(stack + pd->index);
#endif
			} else {
				ret.l = arg_regs[pd->index];
			}
			break;
		case TYPE_LNG:
			if (pd->inmemory) {
				ret.l = (int64_t)stack[pd->index];
			} else {
#if (SIZEOF_VOID_P == 8)
				ret.l = (int64_t)arg_regs[pd->index];
#else
				ret.l = (int64_t)(
					(arg_regs[GET_HIGH_REG(pd->index)] << 32) |
					(arg_regs[GET_LOW_REG(pd->index)] & 0xFFFFFFFF)
				);
#endif
			}
			break;
		case TYPE_FLT:
			if (pd->inmemory) {
				ret.l = (int64_t)stack[pd->index];
			} else {
				ret.l = (int64_t)arg_regs[pd->index + INT_ARG_CNT];
			}
			break;
		case TYPE_DBL:
			if (pd->inmemory) {
				ret.l = (int64_t)stack[pd->index];
			} else {
				ret.l = (int64_t)arg_regs[pd->index + INT_ARG_CNT];
			}
			break;
	}

	return ret;
}

/* _array_load_return_value ***************************************************

   Loads the proper return value form the return registers array and returns it.

*******************************************************************************/

static imm_union _array_load_return_value(typedesc *td, uint64_t *return_regs) {
	imm_union ret;

	switch (td->type) {
		case TYPE_INT:
		case TYPE_ADR:
			ret.l = return_regs[0];
			break;
		case TYPE_LNG:
#if (SIZEOF_VOID_P == 8)
			ret.l = (int64_t)return_regs[0];
#else
			ret.l = (int64_t)(
				(return_regs[0] << 32) | (return_regs[1] & 0xFFFFFFFF)
			);
#endif
			break;
		case TYPE_FLT:
			ret.l = (int64_t)return_regs[2];
			break;
		case TYPE_DBL:
			ret.l = (int64_t)return_regs[2];
			break;
	}

	return ret;
}

static char *trace_java_call_print_argument(char *logtext, s4 *logtextlen,
								         	typedesc *paramtype, imm_union imu)
{
	java_handle_t     *o;
	classinfo         *c;
	utf               *u;
	u4                 len;

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

		/* cast to java.lang.Object */

		o = (java_handle_t *) (ptrint) imu.l;

		/* check return argument for java.lang.Class or java.lang.String */

		if (o != NULL) {
			if (o->vftbl->class == class_java_lang_String) {
				/* get java.lang.String object and the length of the
				   string */

				u = javastring_toutf(o, false);

				len = strlen(" (String = \"") + utf_bytes(u) + strlen("\")");

				/* realloc memory for string length */

				logtext = DMREALLOC(logtext, char, *logtextlen, *logtextlen + len);
				*logtextlen += len;

				/* convert to utf8 string and strcat it to the logtext */

				strcat(logtext, " (String = \"");
				utf_cat(logtext, u);
				strcat(logtext, "\")");
			}
			else {
				if (o->vftbl->class == class_java_lang_Class) {
					/* if the object returned is a java.lang.Class
					   cast it to classinfo structure and get the name
					   of the class */

					c = (classinfo *) o;

					u = c->name;
				}
				else {
					/* if the object returned is not a java.lang.String or
					   a java.lang.Class just print the name of the class */

					u = o->vftbl->class->name;
				}

				len = strlen(" (Class = \"") + utf_bytes(u) + strlen("\")");

				/* realloc memory for string length */

				logtext = DMREALLOC(logtext, char, *logtextlen, *logtextlen + len);
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

void trace_java_call_enter(methodinfo *m, uint64_t *arg_regs, uint64_t *stack) {
	methoddesc *md;
	paramdesc *pd;
	typedesc *td;
	imm_union arg;
	char       *logtext;
	s4          logtextlen;
	s4          dumpsize;
	s4          i;
	s4          pos;

#if defined(ENABLE_DEBUG_FILTER)
	if (! show_filters_test_verbosecall_enter(m)) return;
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
		utf_bytes(m->class->name) +
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
		strlen(" ABSTRACT");

	/* add maximal argument length */

	logtextlen +=
		strlen("(") +
		strlen("-9223372036854775808 (0x123456789abcdef0), ") * md->paramcount +
		strlen("...(255)") +
		strlen(")");

	/* allocate memory */

	dumpsize = dump_size();

	logtext = DMNEW(char, logtextlen);

	TRACEJAVACALLCOUNT++;

	sprintf(logtext, "%10d ", TRACEJAVACALLCOUNT);
	sprintf(logtext + strlen(logtext), "-%d-", TRACEJAVACALLINDENT);

	pos = strlen(logtext);

	for (i = 0; i < TRACEJAVACALLINDENT; i++)
		logtext[pos++] = '\t';

	strcpy(logtext + pos, "called: ");

	utf_cat_classname(logtext, m->class->name);
	strcat(logtext, ".");
	utf_cat(logtext, m->name);
	utf_cat(logtext, m->descriptor);

	if (m->flags & ACC_PUBLIC)       strcat(logtext, " PUBLIC");
	if (m->flags & ACC_PRIVATE)      strcat(logtext, " PRIVATE");
	if (m->flags & ACC_PROTECTED)    strcat(logtext, " PROTECTED");
   	if (m->flags & ACC_STATIC)       strcat(logtext, " STATIC");
   	if (m->flags & ACC_FINAL)        strcat(logtext, " FINAL");
   	if (m->flags & ACC_SYNCHRONIZED) strcat(logtext, " SYNCHRONIZED");
   	if (m->flags & ACC_VOLATILE)     strcat(logtext, " VOLATILE");
   	if (m->flags & ACC_TRANSIENT)    strcat(logtext, " TRANSIENT");
   	if (m->flags & ACC_NATIVE)       strcat(logtext, " NATIVE");
   	if (m->flags & ACC_INTERFACE)    strcat(logtext, " INTERFACE");
   	if (m->flags & ACC_ABSTRACT)     strcat(logtext, " ABSTRACT");

	strcat(logtext, "(");

	for (i = 0; i < md->paramcount; ++i) {
		pd = &md->params[i];
		td = &md->paramtypes[i];
		arg = _array_load_param(pd, td, arg_regs, stack);
		logtext = trace_java_call_print_argument(
			logtext, &logtextlen, td, arg
		);
		if (i != (md->paramcount - 1)) {
			strcat(logtext, ", ");
		}
	}

	strcat(logtext, ")");

	log_text(logtext);

	/* release memory */

	dump_release(dumpsize);

	TRACEJAVACALLINDENT++;

}

void trace_java_call_exit(methodinfo *m, uint64_t *return_regs)
{
	methoddesc *md;
	char       *logtext;
	s4          logtextlen;
	s4          dumpsize;
	s4          i;
	s4          pos;
	imm_union   val;

#if defined(ENABLE_DEBUG_FILTER)
	if (! show_filters_test_verbosecall_exit(m)) return;
#endif

#if defined(ENABLE_VMLOG)
	vmlog_cacao_leave_method(m);
	return;
#endif

	md = m->parseddesc;

	/* calculate message length */

	logtextlen =
		strlen("4294967295 ") +
		strlen("-2147483647-") +        /* INT_MAX should be sufficient       */
		TRACEJAVACALLINDENT +
		strlen("finished: ") +
		utf_bytes(m->class->name) +
		strlen(".") +
		utf_bytes(m->name) +
		utf_bytes(m->descriptor) +
		strlen(" SYNCHRONIZED") + strlen("(") + strlen(")");

	/* add maximal argument length */

	logtextlen += strlen("->0.4872328470301428 (0x0123456789abcdef)");

	/* allocate memory */

	dumpsize = dump_size();

	logtext = DMNEW(char, logtextlen);

	/* outdent the log message */

	if (TRACEJAVACALLINDENT)
		TRACEJAVACALLINDENT--;
	else
		log_text("WARNING: unmatched TRACEJAVACALLINDENT--");

	/* generate the message */

	sprintf(logtext, "           ");
	sprintf(logtext + strlen(logtext), "-%d-", TRACEJAVACALLINDENT);

	pos = strlen(logtext);

	for (i = 0; i < TRACEJAVACALLINDENT; i++)
		logtext[pos++] = '\t';

	strcpy(logtext + pos, "finished: ");
	utf_cat_classname(logtext, m->class->name);
	strcat(logtext, ".");
	utf_cat(logtext, m->name);
	utf_cat(logtext, m->descriptor);

	if (!IS_VOID_TYPE(md->returntype.type)) {
		strcat(logtext, "->");
		val = _array_load_return_value(&md->returntype, return_regs);

		logtext =
			trace_java_call_print_argument(logtext, &logtextlen, &md->returntype, val);
	}

	log_text(logtext);

	/* release memory */

	dump_release(dumpsize);

}

#endif /* !defined(NDEBUG) */

