/* src/vm/options.c - contains global options

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

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "mm/memory.h"

#include "native/jni.h"

#include "vm/options.h"
#include "vm/os.hpp"
#include "vm/vm.hpp"


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
bool initverbose = false;

bool opt_verboseclass     = false;
bool opt_verbosegc        = false;
bool opt_verbosejni       = false;
bool opt_verbosecall      = false;      /* trace all method invocation        */

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

#if defined(ENABLE_PROFILING)
bool opt_prof    = false;
bool opt_prof_bb = false;
#endif

#if defined(ENABLE_OPAGENT)
bool opt_opagent = false;
#endif

/* optimization options *******************************************************/

#if defined(ENABLE_IFCONV)
bool opt_ifconv = false;
#endif

#if defined(ENABLE_LSRA) || defined(ENABLE_SSA)
bool opt_lsra = false;
#endif
#if defined(ENABLE_SSA)
bool opt_ssa_dce = false;          /* enable dead code elemination */
bool opt_ssa_cp = false;           /* enable copy propagation      */
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

/* Options which must always be available (production options in
   HotSpot). */

int64_t  opt_MaxDirectMemorySize          = -1;
int      opt_MaxPermSize                  = 0;
int      opt_PermSize                     = 0;
int      opt_ThreadStackSize              = 0;

/* Debugging options which can be turned off. */

int      opt_DebugExceptions              = 0;
int      opt_DebugFinalizer               = 0;
#if defined(ENABLE_JITCACHE)
int      opt_DebugJitCache                = 0;
#endif
int      opt_DebugLocalReferences         = 0;
int      opt_DebugLocks                   = 0;
int      opt_DebugPackage                 = 0;
int      opt_DebugPatcher                 = 0;
int      opt_DebugProperties              = 0;
int      opt_DebugStackFrameInfo          = 0;
int      opt_DebugStackTrace              = 0;
int      opt_DebugThreads                 = 0;
#if defined(ENABLE_DISASSEMBLER)
int      opt_DisassembleStubs             = 0;
#endif
#if defined(ENABLE_OPAGENT)
int      opt_EnableOpagent                = 0;
#endif
#if defined(ENABLE_GC_CACAO)
int      opt_GCDebugRootSet               = 0;
int      opt_GCStress                     = 0;
#endif
#if defined(ENABLE_INLINING)
int      opt_Inline                       = 0;
#if defined(ENABLE_INLINING_DEBUG) || !defined(NDEBUG)
int      opt_InlineAll                    = 0;
int      opt_InlineCount                  = INT_MAX;
int      opt_InlineMaxSize                = INT_MAX;
int      opt_InlineMinSize                = 0;
#endif
#endif
int      opt_PrintConfig                  = 0;
int      opt_ProfileGCMemoryUsage         = 0;
int      opt_ProfileMemoryUsage           = 0;
FILE    *opt_ProfileMemoryUsageGNUPlot    = NULL;
#if defined(ENABLE_REPLACEMENT)
int      opt_TestReplacement              = 0;
#endif
int      opt_TraceCompilerCalls           = 0;
int      opt_TraceExceptions              = 0;
int      opt_TraceHPI                     = 0;
#if defined(ENABLE_INLINING) && !defined(NDEBUG)
int      opt_TraceInlining                = 0;
#endif
int      opt_TraceJavaCalls               = 0;
int      opt_TraceJNICalls                = 0;
int      opt_TraceJVMCalls                = 0;
int      opt_TraceJVMCallsVerbose         = 0;
int      opt_TraceLinkClass               = 0;
#if defined(ENABLE_REPLACEMENT)
int      opt_TraceReplacement             = 0;
#endif
int      opt_TraceSubsystemInitialization = 0;
int      opt_TraceTraps                   = 0;


enum {
	OPT_TYPE_BOOLEAN,
	OPT_TYPE_VALUE
};

enum {
	/* Options which must always be available (production options in
	   HotSpot). */

	OPT_MaxDirectMemorySize,
	OPT_MaxPermSize,
	OPT_PermSize,
	OPT_ThreadStackSize,

	/* Debugging options which can be turned off. */

	OPT_DebugExceptions,
	OPT_DebugFinalizer,
  OPT_DebugJitCache,
	OPT_DebugLocalReferences,
	OPT_DebugLocks,
	OPT_DebugPackage,
	OPT_DebugPatcher,
	OPT_DebugProperties,
	OPT_DebugStackFrameInfo,
	OPT_DebugStackTrace,
	OPT_DebugThreads,
	OPT_DisassembleStubs,
	OPT_EnableOpagent,
	OPT_GCDebugRootSet,
	OPT_GCStress,
	OPT_Inline,
	OPT_InlineAll,
	OPT_InlineCount,
	OPT_InlineMaxSize,
	OPT_InlineMinSize,
	OPT_PrintConfig,
	OPT_ProfileGCMemoryUsage,
	OPT_ProfileMemoryUsage,
	OPT_ProfileMemoryUsageGNUPlot,
	OPT_TestReplacement,
	OPT_TraceCompilerCalls,
	OPT_TraceExceptions,
	OPT_TraceHPI,
	OPT_TraceInlining,
	OPT_TraceJavaCalls,
	OPT_TraceJNICalls,
	OPT_TraceJVMCalls,
	OPT_TraceJVMCallsVerbose,
	OPT_TraceLinkClass,
	OPT_TraceReplacement,
	OPT_TraceSubsystemInitialization,
	OPT_TraceTraps,
	OPT_Vmlog,
	OPT_VmlogStrings,
	OPT_VmlogIgnore
};


option_t options_XX[] = {
	/* Options which must always be available (production options in
	   HotSpot). */

	{ "MaxDirectMemorySize",          OPT_MaxDirectMemorySize,          OPT_TYPE_VALUE,   "Maximum total size of NIO direct-buffer allocations" },
	{ "MaxPermSize",                  OPT_MaxPermSize,                  OPT_TYPE_VALUE,   "not implemented" },
	{ "PermSize",                     OPT_PermSize,                     OPT_TYPE_VALUE,   "not implemented" },
	{ "ThreadStackSize",              OPT_ThreadStackSize,              OPT_TYPE_VALUE,   "TODO" },

	/* Debugging options which can be turned off. */

	{ "DebugExceptions",              OPT_DebugExceptions,              OPT_TYPE_BOOLEAN, "debug exceptions" },
	{ "DebugFinalizer",               OPT_DebugFinalizer,               OPT_TYPE_BOOLEAN, "debug finalizer thread" },
#if defined (ENABLE_JITCACHE)
  { "DebugJitCache",                OPT_DebugJitCache,                OPT_TYPE_BOOLEAN, "debug JIT cache actions" },
#endif
	{ "DebugLocalReferences",         OPT_DebugLocalReferences,         OPT_TYPE_BOOLEAN, "print debug information for local reference tables" },
	{ "DebugLocks",                   OPT_DebugLocks,                   OPT_TYPE_BOOLEAN, "print debug information for locks" },
	{ "DebugPackage",                 OPT_DebugPackage,                 OPT_TYPE_BOOLEAN, "debug Java boot-packages" },
	{ "DebugPatcher",                 OPT_DebugPatcher,                 OPT_TYPE_BOOLEAN, "debug JIT code patching" },
	{ "DebugProperties",              OPT_DebugProperties,              OPT_TYPE_BOOLEAN, "print debug information for properties" },
	{ "DebugStackFrameInfo",          OPT_DebugStackFrameInfo,          OPT_TYPE_BOOLEAN, "TODO" },
	{ "DebugStackTrace",              OPT_DebugStackTrace,              OPT_TYPE_BOOLEAN, "debug stacktrace creation" },
	{ "DebugThreads",                 OPT_DebugThreads,                 OPT_TYPE_BOOLEAN, "print debug information for threads" },
#if defined(ENABLE_DISASSEMBLER)
	{ "DisassembleStubs",             OPT_DisassembleStubs,             OPT_TYPE_BOOLEAN, "disassemble builtin and native stubs when generated" },
#endif
#if defined(ENABLE_OPAGENT)
	{ "EnableOpagent",                OPT_EnableOpagent,                OPT_TYPE_BOOLEAN, "enable providing JIT output to Oprofile" },
#endif
#if defined(ENABLE_GC_CACAO)
	{ "GCDebugRootSet",               OPT_GCDebugRootSet,               OPT_TYPE_BOOLEAN, "GC: print root-set at collection" },
	{ "GCStress",                     OPT_GCStress,                     OPT_TYPE_BOOLEAN, "GC: forced collection at every allocation" },
#endif
#if defined(ENABLE_INLINING)
	{ "Inline",                       OPT_Inline,                       OPT_TYPE_BOOLEAN, "enable method inlining" },
#if defined(ENABLE_INLINING_DEBUG) || !defined(NDEBUG)
	{ "InlineAll",                    OPT_InlineAll,                    OPT_TYPE_BOOLEAN, "use inlining in all compilations" },
	{ "InlineCount",                  OPT_InlineCount,                  OPT_TYPE_VALUE,   "stop inlining after the given number of roots" },
	{ "InlineMaxSize",                OPT_InlineMaxSize,                OPT_TYPE_VALUE,   "maximum size for inlined result" },
	{ "InlineMinSize",                OPT_InlineMinSize,                OPT_TYPE_VALUE,   "minimum size for inlined result" },
#endif
#endif
	{ "PrintConfig",                  OPT_PrintConfig,                  OPT_TYPE_BOOLEAN, "print VM configuration" },
	{ "ProfileGCMemoryUsage",         OPT_ProfileGCMemoryUsage,         OPT_TYPE_VALUE,   "profiles GC memory usage in the given interval, <value> is in seconds (default: 5)" },
	{ "ProfileMemoryUsage",           OPT_ProfileMemoryUsage,           OPT_TYPE_VALUE,   "TODO" },
	{ "ProfileMemoryUsageGNUPlot",    OPT_ProfileMemoryUsageGNUPlot,    OPT_TYPE_VALUE,   "TODO" },
#if defined(ENABLE_REPLACEMENT)
	{ "TestReplacement",              OPT_TestReplacement,              OPT_TYPE_BOOLEAN, "activate all replacement points during code generation" },
#endif
	{ "TraceCompilerCalls",           OPT_TraceCompilerCalls,           OPT_TYPE_BOOLEAN, "trace JIT compiler calls" },
	{ "TraceExceptions",              OPT_TraceExceptions,              OPT_TYPE_BOOLEAN, "trace Exception throwing" },
	{ "TraceHPI",                     OPT_TraceHPI,                     OPT_TYPE_BOOLEAN, "Trace Host Porting Interface (HPI)" },
#if defined(ENABLE_INLINING) && !defined(NDEBUG)
	{ "TraceInlining",                OPT_TraceInlining,                OPT_TYPE_VALUE,   "trace method inlining with the given verbosity level (default: 1)" },
#endif
#if !defined(ENABLE_VMLOG)
	{ "TraceJavaCalls",               OPT_TraceJavaCalls,               OPT_TYPE_BOOLEAN, "trace Java method calls" },
#endif
	{ "TraceJNICalls",                OPT_TraceJNICalls,                OPT_TYPE_BOOLEAN, "trace JNI method calls" },
	{ "TraceJVMCalls",                OPT_TraceJVMCalls,                OPT_TYPE_BOOLEAN, "trace JVM method calls but omit very frequent ones" },
	{ "TraceJVMCallsVerbose",         OPT_TraceJVMCallsVerbose,         OPT_TYPE_BOOLEAN, "trace all JVM method calls" },
	{ "TraceLinkClass",               OPT_TraceLinkClass,               OPT_TYPE_BOOLEAN, "trace class linking" },
#if defined(ENABLE_REPLACEMENT)
	{ "TraceReplacement",             OPT_TraceReplacement,             OPT_TYPE_VALUE,   "trace on-stack replacement with the given verbosity level (default: 1)" },
#endif
	{ "TraceSubsystemInitialization", OPT_TraceSubsystemInitialization, OPT_TYPE_BOOLEAN, "trace initialization of subsystems" },
	{ "TraceTraps",                   OPT_TraceTraps,                   OPT_TYPE_BOOLEAN, "trace traps generated by JIT code" },
#if defined(ENABLE_VMLOG)
	{ "Vmlog",                        OPT_Vmlog,                        OPT_TYPE_VALUE,   "prefix for vmlog trace files (enables vmlog)" },
	{ "VmlogStrings",                 OPT_VmlogStrings,                 OPT_TYPE_VALUE,   "prefix of vmlog string file to load" },
	{ "VmlogIgnore",                  OPT_VmlogIgnore,                  OPT_TYPE_VALUE,   "prefix of vmlog ignore file to load" },
#endif

	/* end marker */

	{ NULL,                           -1,                               -1,               NULL }
};


/* options_get *****************************************************************

   DOCUMENT ME!!!

*******************************************************************************/

int options_get(opt_struct *opts, JavaVMInitArgs *vm_args)
{
	char *option;
	int   i;

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
					opt_arg = os_strdup(vm_args->options[opt_index].optionString);
					opt_index++;
					return opts[i].value;
				}

				return OPT_ERROR;

			} else {
				/* parameter and option have no space between */

				/* FIXME: this assumption is plain wrong, hits you if there is a
				 * parameter with no argument starting with same letter as param with argument
				 * but named after that one, ouch! */

				size_t l = os_strlen(opts[i].name);

				if (os_strlen(option + 1) > l) {
					if (memcmp(option + 1, opts[i].name, l) == 0) {
						opt_index++;
						opt_arg = os_strdup(option + 1 + l);
						return opts[i].value;
					}
				}
			}
		}
	}

	return OPT_ERROR;
}


/* options_xxusage *************************************************************

   Print usage message for debugging options.

*******************************************************************************/

static void options_xxusage(void)
{
	option_t   *opt;
	int         length;
	int         i;
	const char *c;

	/* Prevent compiler warning. */

	length = 0;

	for (opt = options_XX; opt->name != NULL; opt++) {
		printf("    -XX:");

		switch (opt->type) {
		case OPT_TYPE_BOOLEAN:
			printf("+%s", opt->name);
			length = os_strlen("    -XX:+") + os_strlen(opt->name);
			break;

		case OPT_TYPE_VALUE:
			printf("%s=<value>", opt->name);
			length = os_strlen("    -XX:") + os_strlen(opt->name) +
				os_strlen("=<value>");
			break;

		default:
			vm_abort("options_xxusage: unkown option type %d", opt->type);
		}

		/* Check if the help fits into one 80-column line.
		   Documentation starts at column 29. */

		if (length < (29 - 1)) {
			/* Print missing spaces up to column 29. */

			for (i = length; i < 29; i++)
				printf(" ");
		}
		else {
			printf("\n");
			printf("                             "); /* 29 spaces */
		}

		/* Check documentation length. */

		length = os_strlen(opt->doc);

		if (length < (80 - 29)) {
			printf("%s", opt->doc);
		}
		else {
			for (c = opt->doc, i = 29; *c != 0; c++, i++) {
				/* If we are at the end of the line, break it. */

				if (i == 80) {
					printf("\n");
					printf("                             "); /* 29 spaces */
					i = 29;
				}

				printf("%c", *c);
			}
		}

		printf("\n");
	}

	/* exit with error code */

	exit(1);
}


/* options_xx ******************************************************************

   Handle -XX: options.

*******************************************************************************/

void options_xx(JavaVMInitArgs *vm_args)
{
	const char *name;
	const char *start;
	char       *end;
	int         length;
	int         enable;
	char       *value;
	option_t   *opt;
	char       *filename;
	FILE       *file;
	int         i;

	/* Iterate over all passed options. */

	for (i = 0; i < vm_args->nOptions; i++) {
		/* Get the current option. */

		name = vm_args->options[i].optionString;

		/* Check for help (-XX). */

		if (strcmp(name, "-XX") == 0)
			options_xxusage();

		/* Check if the option start with -XX. */

		start = strstr(name, "-XX:");

		if ((start == NULL) || (start != name))
			continue;

		/* Check if the option is a boolean option. */

		if (name[4] == '+') {
			start  = name + 4 + 1;
			enable = 1;
		}
		else if (name[4] == '-') {
			start  = name + 4 + 1;
			enable = 0;
		}
		else {
			start  = name + 4;
			enable = -1;
		}

		/* Search for a '=' in the option name and get the option name
		   length and the value of the option. */

		end = strchr(start, '=');

		if (end == NULL) {
			length = os_strlen(start);
			value  = NULL;
		}
		else {
			length = end - start;
			value  = end + 1;
		}

		/* Search the option in the option array. */

		for (opt = options_XX; opt->name != NULL; opt++) {
			if (strncmp(opt->name, start, length) == 0) {
				/* Check if the options passed fits to the type. */

				switch (opt->type) {
				case OPT_TYPE_BOOLEAN:
					if ((enable == -1) || (value != NULL))
						options_xxusage();
					break;
				case OPT_TYPE_VALUE:
					if ((enable != -1) || (value == NULL))
						options_xxusage();
					break;
				default:
					vm_abort("options_xx: unknown option type %d for option %s",
							 opt->type, opt->name);
				}

				break;
			}
		}

		/* Process the option. */

		switch (opt->value) {

		/* Options which must always be available (production options
		   in HotSpot). */

		case OPT_MaxDirectMemorySize:
			opt_MaxDirectMemorySize = os_atoi(value);
			break;

		case OPT_MaxPermSize:
			/* Currently ignored. */
			break;

		case OPT_PermSize:
			/* Currently ignored. */
			break;

		case OPT_ThreadStackSize:
			/* currently ignored */
			break;

		/* Debugging options which can be turned off. */

		case OPT_DebugExceptions:
			opt_DebugExceptions = enable;
			break;

		case OPT_DebugFinalizer:
			opt_DebugFinalizer = enable;
			break;

#if defined(ENABLE_JITCACHE)
    case OPT_DebugJitCache:
      opt_DebugJitCache = enable;
      break;
#endif

		case OPT_DebugLocalReferences:
			opt_DebugLocalReferences = enable;
			break;

		case OPT_DebugLocks:
			opt_DebugLocks = enable;
			break;

		case OPT_DebugPackage:
			opt_DebugPackage = enable;
			break;

		case OPT_DebugPatcher:
			opt_DebugPatcher = enable;
			break;

		case OPT_DebugProperties:
			opt_DebugProperties = enable;
			break;

		case OPT_DebugStackFrameInfo:
			opt_DebugStackFrameInfo = enable;
			break;

		case OPT_DebugStackTrace:
			opt_DebugStackTrace = enable;
			break;

		case OPT_DebugThreads:
			opt_DebugThreads = enable;
			break;

#if defined(ENABLE_DISASSEMBLER)
		case OPT_DisassembleStubs:
			opt_DisassembleStubs = enable;
			break;
#endif

#if defined(ENABLE_OPAGENT)
		case OPT_EnableOpagent:
			opt_EnableOpagent = enable;
			break;
#endif

#if defined(ENABLE_GC_CACAO)
		case OPT_GCDebugRootSet:
			opt_GCDebugRootSet = enable;
			break;

		case OPT_GCStress:
			opt_GCStress = enable;
			break;
#endif

#if defined(ENABLE_INLINING)
		case OPT_Inline:
			opt_Inline = enable;
			break;
#if defined(ENABLE_INLINING_DEBUG) || !defined(NDEBUG)
		case OPT_InlineAll:
			opt_InlineAll = enable;
			break;

		case OPT_InlineCount:
			if (value != NULL)
				opt_InlineCount = os_atoi(value);
			break;

		case OPT_InlineMaxSize:
			if (value != NULL)
				opt_InlineMaxSize = os_atoi(value);
			break;

		case OPT_InlineMinSize:
			if (value != NULL)
				opt_InlineMinSize = os_atoi(value);
			break;
#endif
#endif

		case OPT_PrintConfig:
			opt_PrintConfig = enable;
			break;

		case OPT_ProfileGCMemoryUsage:
			if (value == NULL)
				opt_ProfileGCMemoryUsage = 5;
			else
				opt_ProfileGCMemoryUsage = os_atoi(value);
			break;

		case OPT_ProfileMemoryUsage:
			if (value == NULL)
				opt_ProfileMemoryUsage = 5;
			else
				opt_ProfileMemoryUsage = os_atoi(value);

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
				vm_abort_errno("options_xx: fopen failed");

			opt_ProfileMemoryUsageGNUPlot = file;
			break;

#if defined(ENABLE_REPLACEMENT)
		case OPT_TestReplacement:
			opt_TestReplacement = enable;
			break;
#endif

		case OPT_TraceCompilerCalls:
			opt_TraceCompilerCalls = enable;
			break;

		case OPT_TraceExceptions:
			opt_TraceExceptions = enable;
			break;

		case OPT_TraceHPI:
			opt_TraceHPI = enable;
			break;

#if defined(ENABLE_INLINING) && !defined(NDEBUG)
		case OPT_TraceInlining:
			if (value == NULL)
				opt_TraceInlining = 1;
			else
				opt_TraceInlining = os_atoi(value);
			break;
#endif

		case OPT_TraceJavaCalls:
			opt_verbosecall = enable;
			opt_TraceJavaCalls = enable;
			break;

		case OPT_TraceJNICalls:
			opt_TraceJNICalls = enable;
			break;

		case OPT_TraceJVMCalls:
			opt_TraceJVMCalls = enable;
			break;

		case OPT_TraceJVMCallsVerbose:
			opt_TraceJVMCallsVerbose = enable;
			break;

		case OPT_TraceLinkClass:
			opt_TraceLinkClass = enable;
			break;

#if defined(ENABLE_REPLACEMENT)
		case OPT_TraceReplacement:
			if (value == NULL)
				opt_TraceReplacement = 1;
			else
				opt_TraceReplacement = os_atoi(value);
			break;
#endif

		case OPT_TraceSubsystemInitialization:
			opt_TraceSubsystemInitialization = enable;
			break;

		case OPT_TraceTraps:
			opt_TraceTraps = enable;
			break;

#if defined(ENABLE_VMLOG)
		case OPT_Vmlog:
			if (value == NULL)
				vmlog_cacao_set_prefix("vmlog");
			else
				vmlog_cacao_set_prefix(value);
			opt_verbosecall = 1;
			opt_TraceJavaCalls = 1;
			break;

		case OPT_VmlogStrings:
			if (value != NULL)
				vmlog_cacao_set_stringprefix(value);
			break;

		case OPT_VmlogIgnore:
			if (value != NULL)
				vmlog_cacao_set_ignoreprefix(value);
			break;
#endif

		default:
			printf("Unknown -XX option: %s\n", name);
			break;
		}
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
