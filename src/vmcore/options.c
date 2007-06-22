/* src/vmcore/options.c - contains global options

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

   $Id: options.c 8134 2007-06-22 14:49:10Z twisti $

*/


#include "config.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(HAVE_STRING_H)
# include <string.h>
#endif

#include <limits.h>

#include "mm/memory.h"

#include "native/jni.h"

#include "vm/vm.h"

#include "vmcore/options.h"


/* command line option ********************************************************/

s4    opt_index = 0;            /* index of processed arguments               */
char *opt_arg;                  /* this one exports the option argument       */

bool opt_foo = false;           /* option for development                     */

bool opt_jar = false;

#if defined(ENABLE_JIT)
bool opt_jit = true;            /* JIT mode execution (default)               */
bool opt_intrp = false;         /* interpreter mode execution                 */
#else
bool opt_jit = false;           /* JIT mode execution                         */
bool opt_intrp = true;          /* interpreter mode execution (default)       */
#endif

bool opt_run = true;

s4   opt_heapmaxsize   = 0;     /* maximum heap size                          */
s4   opt_heapstartsize = 0;     /* initial heap size                          */
s4   opt_stacksize     = 0;     /* thread stack size                          */

bool opt_verbose = false;
bool opt_debugcolor = false;	/* use ANSI terminal sequences 		      */
bool compileall = false;

bool loadverbose = false;
bool linkverbose = false;
bool initverbose = false;

bool opt_verboseclass     = false;
bool opt_verbosegc        = false;
bool opt_verbosejni       = false;
bool opt_verbosecall      = false;      /* trace all method invocation        */
bool opt_verbosethreads   = false;

bool showmethods = false;
bool showconstantpool = false;
bool showutf = false;

char *opt_method = NULL;
char *opt_signature = NULL;

bool compileverbose =  false;           /* trace compiler actions             */
bool showstack = false;

bool opt_showdisassemble    = false;    /* generate disassembler listing      */
bool opt_shownops           = false;
bool opt_showddatasegment   = false;    /* generate data segment listing      */
bool opt_showintermediate   = false;    /* generate intermediate code listing */
bool opt_showexceptionstubs = false;
bool opt_shownativestub     = false;

bool checkbounds = true;       /* check array bounds                         */
bool opt_noieee = false;       /* don't implement ieee compliant floats      */
bool checksync = true;         /* do synchronization                         */
#if defined(ENABLE_LOOP)
bool opt_loops = false;        /* optimize array accesses in loops           */
#endif

bool makeinitializations = true;

#if defined(ENABLE_STATISTICS)
bool opt_stat    = false;
bool opt_getloadingtime = false;   /* to measure the runtime                 */
bool opt_getcompilingtime = false; /* compute compile time                   */
#endif
#if defined(ENABLE_VERIFIER)
bool opt_verify  = true;       /* true if classfiles should be verified      */
#endif
bool opt_eager   = false;

#if defined(ENABLE_PROFILING)
bool opt_prof    = false;
bool opt_prof_bb = false;
#endif


/* inlining options ***********************************************************/

#if defined(ENABLE_INLINING)
bool opt_inlining = false;
#if defined(ENABLE_INLINING_DEBUG) || !defined(NDEBUG)
s4 opt_replace_verbose = 0;
s4 opt_inline_debug_min_size = 0;
s4 opt_inline_debug_max_size = INT_MAX;
s4 opt_inline_debug_end_counter = INT_MAX;
bool opt_inline_debug_all = false;
#endif /* defined(ENABLE_INLINING_DEBUG) || !defined(NDEBUG) */
#if !defined(NDEBUG)
bool opt_inline_debug_log = false;
#endif /* !defined(NDEBUG) */
#endif /* defined(ENABLE_INLINING) */


/* optimization options *******************************************************/

#if defined(ENABLE_IFCONV)
bool opt_ifconv = false;
#endif

#if defined(ENABLE_LSRA) || defined(ENABLE_SSA)
bool opt_lsra = false;
#endif


/* interpreter options ********************************************************/

#if defined(ENABLE_INTRP)
bool opt_no_dynamic = false;            /* suppress dynamic superinstructions */
bool opt_no_replication = false;        /* don't use replication in intrp     */
bool opt_no_quicksuper = false;         /* instructions for quickening cannot be
										   part of dynamic superinstructions */

s4   opt_static_supers = 0x7fffffff;
bool vm_debug = false;          /* XXX this should be called `opt_trace'      */
#endif

#if defined(ENABLE_DEBUG_FILTER)
const char *opt_filter_verbosecall_include = 0;
const char *opt_filter_verbosecall_exclude = 0;
const char *opt_filter_show_method = 0;
#endif


/* -XX options ****************************************************************/

/* NOTE: For better readability keep these alpha-sorted. */

int32_t  opt_MaxPermSize               = 0;
int32_t  opt_PermSize                  = 0;
int32_t  opt_ProfileGCMemoryUsage      = 0;
int32_t  opt_ProfileMemoryUsage        = 0;
FILE    *opt_ProfileMemoryUsageGNUPlot = NULL;
int32_t  opt_ThreadStackSize           = 0;
int32_t  opt_TraceExceptions           = 0;
#if defined(ENABLE_REPLACEMENT)
int32_t  opt_TraceReplacement          = 0;
#endif


enum {
	OPT_MaxPermSize,
	OPT_PermSize,
	OPT_ProfileGCMemoryUsage,
	OPT_ProfileMemoryUsage,
	OPT_ProfileMemoryUsageGNUPlot,
	OPT_ThreadStackSize,
	OPT_TraceExceptions,
	OPT_TraceReplacement
};


option_t options_XX[] = {
	{ "MaxPermSize",               OPT_MaxPermSize,               "" },
	{ "PermSize",                  OPT_PermSize,                  "" },
	{ "ProfileGCMemoryUsage",      OPT_ProfileGCMemoryUsage,      "" },
	{ "ProfileMemoryUsage",        OPT_ProfileMemoryUsage,        "" },
	{ "ProfileMemoryUsageGNUPlot", OPT_ProfileMemoryUsageGNUPlot, "" },
	{ "ThreadStackSize",           OPT_ThreadStackSize,           "" },
	{ "TraceExceptions",           OPT_TraceExceptions,           "" },
#if defined(ENABLE_REPLACEMENT)
	{ "TraceReplacement",          OPT_TraceReplacement,          "" },
#endif
};


/* options_get *****************************************************************

   DOCUMENT ME!!!

*******************************************************************************/

s4 options_get(opt_struct *opts, JavaVMInitArgs *vm_args)
{
	char *option;
	s4    i;

	if (opt_index >= vm_args->nOptions)
		return OPT_DONE;

	/* get the current option */

	option = vm_args->options[opt_index].optionString;

	if ((option == NULL) || (option[0] != '-'))
		return OPT_DONE;

	for (i = 0; opts[i].name; i++) {
		if (!opts[i].arg) {
			/* boolean option found */

			if (strcmp(option + 1, opts[i].name) == 0) {
				opt_index++;
				return opts[i].value;
			}

		} else {
			/* parameter option found */

			/* with a space between */

			if (strcmp(option + 1, opts[i].name) == 0) {
				opt_index++;

				if (opt_index < vm_args->nOptions) {

#if defined(HAVE_STRDUP)
					opt_arg = strdup(vm_args->options[opt_index].optionString);
#else
# error !HAVE_STRDUP
#endif

					opt_index++;
					return opts[i].value;
				}

				return OPT_ERROR;

			} else {
				/* parameter and option have no space between */

				/* FIXME: this assumption is plain wrong, hits you if there is a
				 * parameter with no argument starting with same letter as param with argument
				 * but named after that one, ouch! */

				size_t l = strlen(opts[i].name);

				if (strlen(option + 1) > l) {
					if (memcmp(option + 1, opts[i].name, l) == 0) {
						opt_index++;

#if defined(HAVE_STRDUP)
						opt_arg = strdup(option + 1 + l);
#else
# error !HAVE_STRDUP
#endif

						return opts[i].value;
					}
				}
			}
		}
	}

	return OPT_ERROR;
}


/* options_xx ******************************************************************

   Handle -XX: options.

*******************************************************************************/

void options_xx(const char *name)
{
	char    *start;
	char    *end;
	int32_t  length;
	int32_t  enable;
	char    *value;
	int32_t  option;
	char    *filename;
	FILE    *file;
	int32_t  i;

	/* Check if the option is a boolean option. */

	if (name[0] == '+') {
		start  = name + 1;
		enable = 1;
	}
	else if (name[0] == '-') {
		start  = name + 1;
		enable = 0;
	}
	else {
		start  = name;
		enable = -1;
	}

	/* Search for a '=' in the option name and get the option name
	   length and the value of the option. */

	end = strchr(start, '=');

	if (end == NULL) {
		length = strlen(start);
		value  = NULL;
	}
	else {
		length = end - start;
		value  = end + 1;
	}

	/* search the option in the option array */

	option = OPT_ERROR;

	for (i = 0; options_XX[i].name != NULL; i++) {
		if (strncmp(options_XX[i].name, start, length) == 0) {
			option = options_XX[i].option;
			break;
		}
	}

	/* process the option */

	switch (option) {
	case OPT_MaxPermSize:
		/* currently ignored */
		break;

	case OPT_PermSize:
		/* currently ignored */
		break;

	case OPT_ProfileGCMemoryUsage:
		if (value == NULL)
			opt_ProfileGCMemoryUsage = 5;
		else
			opt_ProfileGCMemoryUsage = atoi(value);
		break;

	case OPT_ProfileMemoryUsage:
		if (value == NULL)
			opt_ProfileMemoryUsage = 5;
		else
			opt_ProfileMemoryUsage = atoi(value);

# if defined(ENABLE_STATISTICS)
		/* we also need statistics */

		opt_stat = true;
# endif
		break;

	case OPT_ProfileMemoryUsageGNUPlot:
		if (value == NULL)
			filename = "profile.dat";
		else
			filename = value;

		file = fopen(filename, "w");

		if (file == NULL)
			vm_abort("options_xx: fopen failed: %s", strerror(errno));

		opt_ProfileMemoryUsageGNUPlot = file;
		break;

	case OPT_ThreadStackSize:
		/* currently ignored */
		break;

	case OPT_TraceExceptions:
		opt_TraceExceptions = enable;
		break;

#if defined(ENABLE_REPLACEMENT)
	case OPT_TraceReplacement:
		if (value == NULL)
			opt_TraceReplacement = 1;
		else
			opt_TraceReplacement = atoi(value);
		break;
#endif

	default:
		printf("Unknown option: -XX:%s\n", name);
		break;
	}
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
