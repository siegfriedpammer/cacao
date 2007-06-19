/* src/vm/vm.c - VM startup and shutdown functions

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

   $Id: vm.c 8106 2007-06-19 22:50:17Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(WITH_JRE_LAYOUT)
# include <libgen.h>
# include <unistd.h>
#endif

#include "vm/types.h"

#include "arch.h"
#include "md-abi.h"

#include "vm/jit/abi-asm.h"

#include "mm/gc-common.h"
#include "mm/memory.h"

#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_String.h" /* required by java_lang_Class.h */
#include "native/include/java_lang_Class.h"

#include "native/include/java_lang_Byte.h"
#include "native/include/java_lang_Character.h"
#include "native/include/java_lang_Short.h"
#include "native/include/java_lang_Integer.h"
#include "native/include/java_lang_Boolean.h"
#include "native/include/java_lang_Long.h"
#include "native/include/java_lang_Float.h"
#include "native/include/java_lang_Double.h"

#include "threads/threads-common.h"

#include "toolbox/logging.h"

#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/finalizer.h"
#include "vm/global.h"
#include "vm/initialize.h"
#include "vm/properties.h"
#include "vm/signallocal.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"

#include "vm/jit/jit.h"
#include "vm/jit/md.h"
#include "vm/jit/asmpart.h"

#if defined(ENABLE_PROFILING)
# include "vm/jit/optimizing/profile.h"
#endif

#include "vm/jit/optimizing/recompile.h"

#include "vmcore/classcache.h"
#include "vmcore/options.h"
#include "vmcore/primitive.h"
#include "vmcore/statistics.h"
#include "vmcore/suck.h"

#if defined(ENABLE_JVMTI)
# include "native/jvmti/cacaodbg.h"
#endif

#if defined(ENABLE_VMLOG)
#include <vmlog_cacao.h>
#endif


/* Invocation API variables ***************************************************/

_Jv_JavaVM *_Jv_jvm;                    /* denotes a Java VM                  */
_Jv_JNIEnv *_Jv_env;                    /* pointer to native method interface */


/* global variables ***********************************************************/

s4 vms = 0;                             /* number of VMs created              */

bool vm_initializing = false;
bool vm_exiting = false;

char      *cacao_prefix = NULL;
char      *cacao_libjvm = NULL;
char      *classpath_libdir = NULL;

char      *_Jv_bootclasspath;           /* contains the boot classpath        */
char      *_Jv_classpath;               /* contains the classpath             */
char      *_Jv_java_library_path;

char      *mainstring = NULL;
classinfo *mainclass = NULL;

char *specificmethodname = NULL;
char *specificsignature = NULL;

bool startit = true;

#if defined(ENABLE_INTRP)
u1 *intrp_main_stack = NULL;
#endif


/* define heap sizes **********************************************************/

#define HEAP_MAXSIZE      128 * 1024 * 1024 /* default 128MB                  */
#define HEAP_STARTSIZE      2 * 1024 * 1024 /* default 2MB                    */
#define STACK_SIZE                64 * 1024 /* default 64kB                   */


/* define command line options ************************************************/

enum {
	OPT_FOO,

	/* Java options */

	OPT_JAR,

	OPT_D32,
	OPT_D64,

	OPT_CLASSPATH,
	OPT_D,

	OPT_VERBOSE,

	OPT_VERSION,
	OPT_SHOWVERSION,
	OPT_FULLVERSION,

	OPT_HELP,
	OPT_X,
	OPT_XX,

	OPT_EA,
	OPT_DA,

	OPT_ESA,
	OPT_DSA,

	/* Java non-standard options */

	OPT_JIT,
	OPT_INTRP,

	OPT_BOOTCLASSPATH,
	OPT_BOOTCLASSPATH_A,
	OPT_BOOTCLASSPATH_P,

	OPT_BOOTCLASSPATH_C,

#if defined(ENABLE_PROFILING)
	OPT_PROF,
	OPT_PROF_OPTION,
#endif

	OPT_MS,
	OPT_MX,

	/* CACAO options */

	OPT_VERBOSE1,
	OPT_NOIEEE,

#if defined(ENABLE_STATISTICS)
	OPT_TIME,
	OPT_STAT,
#endif

	OPT_LOG,
	OPT_CHECK,
	OPT_LOAD,
	OPT_SHOW,
	OPT_DEBUGCOLOR,

#if !defined(NDEBUG)
	OPT_ALL,
	OPT_METHOD,
	OPT_SIGNATURE,
#endif

#if defined(ENABLE_VERIFIER)
	OPT_NOVERIFY,
#if defined(TYPECHECK_VERBOSE)
	OPT_VERBOSETC,
#endif
#endif /* defined(ENABLE_VERIFIER) */
	OPT_EAGER,

	/* optimization options */

#if defined(ENABLE_LOOP)
	OPT_OLOOP,
#endif
	
#if defined(ENABLE_IFCONV)
	OPT_IFCONV,
#endif

#if defined(ENABLE_LSRA) || defined(ENABLE_SSA)
	OPT_LSRA,
#endif

#if defined(ENABLE_INLINING)
	OPT_INLINING,
#if !defined(NDEBUG)
	OPT_INLINE_LOG,
#endif
#if defined(ENABLE_INLINING_DEBUG)
	OPT_INLINE_DEBUG_ALL,
	OPT_INLINE_DEBUG_END,
	OPT_INLINE_DEBUG_MIN,
	OPT_INLINE_DEBUG_MAX,
	OPT_INLINE_REPLACE_VERBOSE,
	OPT_INLINE_REPLACE_VERBOSE2,
#endif /* defined(ENABLE_INLINING_DEBUG) */
#endif /* defined(ENABLE_INLINING) */

#if defined(ENABLE_INTRP)
	/* interpreter options */

	OPT_NO_DYNAMIC,
	OPT_NO_REPLICATION,
	OPT_NO_QUICKSUPER,
	OPT_STATIC_SUPERS,
	OPT_TRACE,
#endif

	OPT_SS,

#ifdef ENABLE_JVMTI
	OPT_DEBUG,
	OPT_XRUNJDWP,
	OPT_NOAGENT,
	OPT_AGENTLIB,
	OPT_AGENTPATH,
#endif

#if defined(ENABLE_DEBUG_FILTER)
	OPT_FILTER_VERBOSECALL_INCLUDE,
	OPT_FILTER_VERBOSECALL_EXCLUDE,
	OPT_FILTER_SHOW_METHOD,
#endif

	DUMMY
};


opt_struct opts[] = {
	{ "foo",               false, OPT_FOO },

	/* Java options */

	{ "jar",               false, OPT_JAR },

	{ "d32",               false, OPT_D32 },
	{ "d64",               false, OPT_D64 },
	{ "client",            false, OPT_IGNORE },
	{ "server",            false, OPT_IGNORE },
	{ "jvm",               false, OPT_IGNORE },
	{ "hotspot",           false, OPT_IGNORE },

	{ "classpath",         true,  OPT_CLASSPATH },
	{ "cp",                true,  OPT_CLASSPATH },
	{ "D",                 true,  OPT_D },
	{ "version",           false, OPT_VERSION },
	{ "showversion",       false, OPT_SHOWVERSION },
	{ "fullversion",       false, OPT_FULLVERSION },
	{ "help",              false, OPT_HELP },
	{ "?",                 false, OPT_HELP },
	{ "X",                 false, OPT_X },
	{ "XX:",               true,  OPT_XX },

	{ "ea:",               true,  OPT_EA },
	{ "da:",               true,  OPT_DA },
	{ "ea",                false, OPT_EA },
	{ "da",                false, OPT_DA },

	{ "esa",                     false, OPT_ESA },
	{ "enablesystemassertions",  false, OPT_ESA },
	{ "dsa",                     false, OPT_DSA },
	{ "disablesystemassertions", false, OPT_DSA },

	{ "noasyncgc",         false, OPT_IGNORE },
#if defined(ENABLE_VERIFIER)
	{ "noverify",          false, OPT_NOVERIFY },
#endif
	{ "v",                 false, OPT_VERBOSE1 },
	{ "verbose:",          true,  OPT_VERBOSE },

#if defined(ENABLE_VERIFIER) && defined(TYPECHECK_VERBOSE)
	{ "verbosetc",         false, OPT_VERBOSETC },
#endif
#if defined(__ALPHA__)
	{ "noieee",            false, OPT_NOIEEE },
#endif
#if defined(ENABLE_STATISTICS)
	{ "time",              false, OPT_TIME },
	{ "stat",              false, OPT_STAT },
#endif
	{ "log",               true,  OPT_LOG },
	{ "c",                 true,  OPT_CHECK },
	{ "l",                 false, OPT_LOAD },
	{ "eager",             false, OPT_EAGER },

#if !defined(NDEBUG)
	{ "all",               false, OPT_ALL },
	{ "sig",               true,  OPT_SIGNATURE },
#endif

#if defined(ENABLE_LOOP)
	{ "oloop",             false, OPT_OLOOP },
#endif
#if defined(ENABLE_IFCONV)
	{ "ifconv",            false, OPT_IFCONV },
#endif
#if defined(ENABLE_LSRA) || defined(ENABLE_SSA)
	{ "lsra",              false, OPT_LSRA },
#endif

#if defined(ENABLE_INTRP)
	/* interpreter options */

	{ "trace",             false, OPT_TRACE },
	{ "static-supers",     true,  OPT_STATIC_SUPERS },
	{ "no-dynamic",        false, OPT_NO_DYNAMIC },
	{ "no-replication",    false, OPT_NO_REPLICATION },
	{ "no-quicksuper",     false, OPT_NO_QUICKSUPER },
#endif

	/* JVMTI Agent Command Line Options */
#ifdef ENABLE_JVMTI
	{ "agentlib:",         true,  OPT_AGENTLIB },
	{ "agentpath:",        true,  OPT_AGENTPATH },
#endif

	/* Java non-standard options */

	{ "Xjit",              false, OPT_JIT },
	{ "Xint",              false, OPT_INTRP },
	{ "Xbootclasspath:",   true,  OPT_BOOTCLASSPATH },
	{ "Xbootclasspath/a:", true,  OPT_BOOTCLASSPATH_A },
	{ "Xbootclasspath/p:", true,  OPT_BOOTCLASSPATH_P },
	{ "Xbootclasspath/c:", true,  OPT_BOOTCLASSPATH_C },

#ifdef ENABLE_JVMTI
	{ "Xdebug",            false, OPT_DEBUG },
	{ "Xnoagent",          false, OPT_NOAGENT },
	{ "Xrunjdwp",          true,  OPT_XRUNJDWP },
#endif 

	{ "Xms",               true,  OPT_MS },
	{ "ms",                true,  OPT_MS },
	{ "Xmx",               true,  OPT_MX },
	{ "mx",                true,  OPT_MX },
	{ "Xss",               true,  OPT_SS },
	{ "ss",                true,  OPT_SS },

#if defined(ENABLE_PROFILING)
	{ "Xprof:",            true,  OPT_PROF_OPTION },
	{ "Xprof",             false, OPT_PROF },
#endif

	/* inlining options */

#if defined(ENABLE_INLINING)
#if defined(ENABLE_INLINING_DEBUG)
	{ "ia",                false, OPT_INLINE_DEBUG_ALL },
	{ "ii",                true,  OPT_INLINE_DEBUG_MIN },
	{ "im",                true,  OPT_INLINE_DEBUG_MAX },
	{ "ie",                true,  OPT_INLINE_DEBUG_END },
	{ "ir",                false, OPT_INLINE_REPLACE_VERBOSE },
	{ "iR",                false, OPT_INLINE_REPLACE_VERBOSE2 },
#endif /* defined(ENABLE_INLINING_DEBUG) */
#if !defined(NDEBUG)
	{ "il",                false, OPT_INLINE_LOG },
#endif
	{ "i",                 false, OPT_INLINING },
#endif /* defined(ENABLE_INLINING) */

	/* keep these at the end of the list */

#if !defined(NDEBUG)
	{ "m",                 true,  OPT_METHOD },
#endif

	{ "s",                 true,  OPT_SHOW },
	{ "debug-color",      false,  OPT_DEBUGCOLOR },

#if defined(ENABLE_DEBUG_FILTER)
	{ "XXfi",              true,  OPT_FILTER_VERBOSECALL_INCLUDE },
	{ "XXfx",              true,  OPT_FILTER_VERBOSECALL_EXCLUDE },
	{ "XXfm",              true,  OPT_FILTER_SHOW_METHOD },
#endif

	{ NULL,                false, 0 }
};


/* usage ***********************************************************************

   Prints the correct usage syntax to stdout.

*******************************************************************************/

void usage(void)
{
	puts("Usage: cacao [-options] classname [arguments]");
	puts("               (to run a class file)");
	puts("   or  cacao [-options] -jar jarfile [arguments]");
	puts("               (to run a standalone jar file)\n");

	puts("where options include:");
	puts("    -d32                     use 32-bit data model if available");
	puts("    -d64                     use 64-bit data model if available");
	puts("    -client                  compatibility (currently ignored)");
	puts("    -server                  compatibility (currently ignored)");
	puts("    -jvm                     compatibility (currently ignored)");
	puts("    -hotspot                 compatibility (currently ignored)\n");

	puts("    -cp <path>               specify a path to look for classes");
	puts("    -classpath <path>        specify a path to look for classes");
	puts("    -D<name>=<value>         add an entry to the property list");
	puts("    -verbose[:class|gc|jni]  enable specific verbose output");
	puts("    -version                 print product version and exit");
	puts("    -fullversion             print jpackage-compatible product version and exit");
	puts("    -showversion             print product version and continue");
	puts("    -help, -?                print this help message");
	puts("    -X                       print help on non-standard Java options");
	puts("    -XX                      print help on CACAO options");
    puts("    -ea[:<packagename>...|:<classname>]");
    puts("    -enableassertions[:<packagename>...|:<classname>]");
	puts("                             enable assertions with specified granularity");
	puts("    -da[:<packagename>...|:<classname>]");
	puts("    -disableassertions[:<packagename>...|:<classname>]");
	puts("                             disable assertions with specified granularity");
	puts("    -esa | -enablesystemassertions");
	puts("                             enable system assertions");
	puts("    -dsa | -disablesystemassertions");
	puts("                             disable system assertions");

#ifdef ENABLE_JVMTI
	puts("    -agentlib:<agent-lib-name>=<options>  library to load containg JVMTI agent");
	puts ("                                         for jdwp help use: -agentlib:jdwp=help");
	puts("    -agentpath:<path-to-agent>=<options>  path to library containg JVMTI agent");
#endif

	/* exit with error code */

	exit(1);
}   


static void Xusage(void)
{
#if defined(ENABLE_JIT)
	puts("    -Xjit                    JIT mode execution (default)");
#endif
#if defined(ENABLE_INTRP)
	puts("    -Xint                    interpreter mode execution");
#endif
	puts("    -Xbootclasspath:<zip/jar files and directories separated by :>");
    puts("                             value is set as bootstrap class path");
	puts("    -Xbootclasspath/a:<zip/jar files and directories separated by :>");
	puts("                             value is appended to the bootstrap class path");
	puts("    -Xbootclasspath/p:<zip/jar files and directories separated by :>");
	puts("                             value is prepended to the bootstrap class path");
	puts("    -Xbootclasspath/c:<zip/jar files and directories separated by :>");
	puts("                             value is used as Java core library, but the");
	puts("                             hardcoded VM interface classes are prepended");
	printf("    -Xms<size>               set the initial size of the heap (default: %dMB)\n", HEAP_STARTSIZE / 1024 / 1024);
	printf("    -Xmx<size>               set the maximum size of the heap (default: %dMB)\n", HEAP_MAXSIZE / 1024 / 1024);
	printf("    -Xss<size>               set the thread stack size (default: %dkB)\n", STACK_SIZE / 1024);

#if defined(ENABLE_PROFILING)
	puts("    -Xprof[:bb]              collect and print profiling data");
#endif

#if defined(ENABLE_JVMTI)
    /* -Xdebug option depend on gnu classpath JDWP options. options: 
	 transport=dt_socket,address=<hostname:port>,server=(y|n),suspend(y|n) */
	puts("    -Xdebug                  enable remote debugging\n");
	puts("    -Xrunjdwp transport=[dt_socket|...],address=<hostname:port>,server=[y|n],suspend=[y|n]\n");
	puts("                             enable remote debugging\n");
#endif 

	/* exit with error code */

	exit(1);
}   


static void XXusage(void)
{
	puts("    -v                       write state-information");
#if !defined(NDEBUG)
	puts("    -verbose[:call|exception|jit|memory|threads]");
	puts("                             enable specific verbose output");
	puts("    -debug-color             colored output for ANSI terms");
#endif
#ifdef TYPECHECK_VERBOSE
	puts("    -verbosetc               write debug messages while typechecking");
#endif
#if defined(__ALPHA__)
	puts("    -noieee                  don't use ieee compliant arithmetic");
#endif
#if defined(ENABLE_VERIFIER)
	puts("    -noverify                don't verify classfiles");
#endif
#if defined(ENABLE_STATISTICS)
	puts("    -time                    measure the runtime");
	puts("    -stat                    detailed compiler statistics");
#endif
	puts("    -log logfile             specify a name for the logfile");
	puts("    -c(heck)b(ounds)         don't check array bounds");
	puts("            s(ync)           don't check for synchronization");
#if defined(ENABLE_LOOP)
	puts("    -oloop                   optimize array accesses in loops");
#endif
	puts("    -l                       don't start the class after loading");
	puts("    -eager                   perform eager class loading and linking");
#if !defined(NDEBUG)
	puts("    -all                     compile all methods, no execution");
	puts("    -m                       compile only a specific method");
	puts("    -sig                     specify signature for a specific method");
#endif

	puts("    -s...                    show...");
	puts("      (c)onstants            the constant pool");
	puts("      (m)ethods              class fields and methods");
	puts("      (u)tf                  the utf - hash");
	puts("      (i)ntermediate         intermediate representation");
#if defined(ENABLE_DISASSEMBLER)
	puts("      (a)ssembler            disassembled listing");
	puts("      n(o)ps                 show NOPs in disassembler output");
	puts("      (e)xceptionstubs       disassembled exception stubs (only with -sa)");
	puts("      (n)ative               disassembled native stubs");
#endif
	puts("      (d)atasegment          data segment listing");

#if defined(ENABLE_INLINING)
	puts("    -i                       activate inlining");
#if !defined(NDEBUG)
	puts("    -il                      log inlining");
#endif
#if defined(ENABLE_INLINING_DEBUG)
	puts("    -ia                      use inlining for all methods");
	puts("    -ii <size>               set minimum size for inlined result");
	puts("    -im <size>               set maximum size for inlined result");
	puts("    -ie <number>             stop inlining after the given number of roots");
	puts("    -ir                      log on-stack replacement");
	puts("    -iR                      log on-stack replacement, more verbose");
#endif /* defined(ENABLE_INLINING_DEBUG) */
#endif /* defined(ENABLE_INLINING) */

#if defined(ENABLE_IFCONV)
	puts("    -ifconv                  use if-conversion");
#endif
#if defined(ENABLE_LSRA)
	puts("    -lsra                    use linear scan register allocation");
#endif
#if defined(ENABLE_SSA)
	puts("    -lsra                    use linear scan register allocation (with SSA)");
#endif
#if defined(ENABLE_DEBUG_FILTER)
	puts("    -XXfi <regex>            begin of dynamic scope for verbosecall filter");
	puts("    -XXfx <regex>            end of dynamic scope for verbosecall filter");
	puts("    -XXfm <regex>            filter for show options");
#endif
	/* exit with error code */

	exit(1);
}


/* version *********************************************************************

   Only prints cacao version information.

*******************************************************************************/

static void version(bool opt_exit)
{
	puts("java version \""JAVA_VERSION"\"");
	puts("CACAO version "VERSION"");

	puts("Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,");
	puts("C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,");
	puts("E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,");
	puts("J. Wenninger, Institut f. Computersprachen - TU Wien\n");

	puts("This program is free software; you can redistribute it and/or");
	puts("modify it under the terms of the GNU General Public License as");
	puts("published by the Free Software Foundation; either version 2, or (at");
	puts("your option) any later version.\n");

	puts("This program is distributed in the hope that it will be useful, but");
	puts("WITHOUT ANY WARRANTY; without even the implied warranty of");
	puts("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU");
	puts("General Public License for more details.\n");

	puts("Configure/Build options:\n");
	puts("  ./configure: "VERSION_CONFIGURE_ARGS"");
#if defined(__VERSION__)
	puts("  CC         : "VERSION_CC" ("__VERSION__")");
#else
	puts("  CC         : "VERSION_CC"");
#endif
	puts("  CFLAGS     : "VERSION_CFLAGS"\n");

	puts("Default variables:\n");
	printf("  maximum heap size              : %d\n", HEAP_MAXSIZE);
	printf("  initial heap size              : %d\n", HEAP_STARTSIZE);
	printf("  stack size                     : %d\n", STACK_SIZE);
#if defined(WITH_CLASSPATH_GNU)
	puts("  java.boot.class.path           : "CACAO_VM_ZIP":"CLASSPATH_CLASSES"");
#else
	puts("  java.boot.class.path           : "CLASSPATH_CLASSES"");
#endif
	puts("  gnu.classpath.boot.library.path: "CLASSPATH_LIBDIR"/classpath\n");

	puts("Runtime variables:\n");
	printf("  maximum heap size              : %d\n", opt_heapmaxsize);
	printf("  initial heap size              : %d\n", opt_heapstartsize);
	printf("  stack size                     : %d\n", opt_stacksize);
	printf("  libjvm.so                      : %s\n", cacao_libjvm);
	printf("  java.boot.class.path           : %s\n", _Jv_bootclasspath);
	printf("  gnu.classpath.boot.library.path: %s\n", classpath_libdir);
	printf("  java.class.path                : %s\n", _Jv_classpath);

	/* exit normally, if requested */

	if (opt_exit)
		exit(0);
}


/* fullversion *****************************************************************

   Prints a Sun compatible version information (required e.g. by
   jpackage, www.jpackage.org).

*******************************************************************************/

static void fullversion(void)
{
	puts("java full version \"cacao-"JAVA_VERSION"\"");

	/* exit normally */

	exit(0);
}


/* forward declarations *******************************************************/

static char *vm_get_mainclass_from_jar(char *mainstring);
#if !defined(NDEBUG)
static void  vm_compile_all(void);
static void  vm_compile_method(void);
#endif


/* vm_createjvm ****************************************************************

   Implementation for JNI_CreateJavaVM.

*******************************************************************************/

bool vm_createjvm(JavaVM **p_vm, void **p_env, void *vm_args)
{
	JavaVMInitArgs *_vm_args;
	_Jv_JNIEnv     *env;
	_Jv_JavaVM     *vm;

	/* get the arguments for the new JVM */

	_vm_args = (JavaVMInitArgs *) vm_args;

	/* get the VM and Env tables (must be set before vm_create) */

	env = NEW(_Jv_JNIEnv);

#if defined(ENABLE_JNI)
	env->env = &_Jv_JNINativeInterface;
#endif

	/* XXX Set the global variable.  Maybe we should do that differently. */

	_Jv_env = env;

	/* create and fill a JavaVM structure */

	vm = NEW(_Jv_JavaVM);

#if defined(ENABLE_JNI)
	vm->functions = &_Jv_JNIInvokeInterface;
#endif

	/* XXX Set the global variable.  Maybe we should do that differently. */
	/* XXX JVMTI Agents needs a JavaVM  */

	_Jv_jvm = vm;

	/* actually create the JVM */

	if (!vm_create(_vm_args))
		goto error;

#if defined(ENABLE_JNI)
	/* setup the local ref table (must be created after vm_create) */

	if (!jni_init_localref_table())
		goto error;
#endif

	/* now return the values */

	*p_vm  = (JavaVM *) vm;
	*p_env = (void *) env;

	return true;

 error:
	/* release allocated memory */

	FREE(env, _Jv_JNIEnv);
	FREE(vm, _Jv_JavaVM);

	return false;
}


/* vm_create *******************************************************************

   Creates a JVM.  Called by vm_createjvm.

*******************************************************************************/

bool vm_create(JavaVMInitArgs *vm_args)
{
	char *cp;
	s4    len;
	s4    opt;
	s4    i, j;
	bool  opt_version;
	bool  opt_exit;

#if defined(ENABLE_JVMTI)
	lt_dlhandle  handle;
	char *libname, *agentarg;
	bool jdwp,agentbypath;
	jdwp = agentbypath = false;
#endif

#if defined(ENABLE_VMLOG)
	vmlog_cacao_init(vm_args);
#endif

	/* check the JNI version requested */

	switch (vm_args->version) {
	case JNI_VERSION_1_1:
		break;
	case JNI_VERSION_1_2:
	case JNI_VERSION_1_4:
		break;
	default:
		return false;
	}

	/* we only support 1 JVM instance */

	if (vms > 0)
		return false;

	if (atexit(vm_exit_handler))
		vm_abort("atexit failed: %s\n", strerror(errno));

	if (opt_verbose)
		log_text("CACAO started -------------------------------------------------------");

	/* We need to check if the actual size of a java.lang.Class object
	   is smaller or equal than the assumption made in
	   src/vmcore/class.h. */

	if (sizeof(java_lang_Class) > sizeof(dummy_java_lang_Class))
		vm_abort("vm_create: java_lang_Class structure is bigger than classinfo.object (%d > %d)", sizeof(java_lang_Class), sizeof(dummy_java_lang_Class));

	/* set the VM starttime */

	_Jv_jvm->starttime = builtin_currenttimemillis();

	/* get stuff from the environment *****************************************/

#if defined(WITH_JRE_LAYOUT)
	/* SUN also uses a buffer of 4096-bytes (strace is your friend). */

	cacao_prefix = MNEW(char, 4096);

	if (readlink("/proc/self/exe", cacao_prefix, 4095) == -1)
		vm_abort("readlink failed: %s\n", strerror(errno));

	/* get the path of the current executable */

	cacao_prefix = dirname(cacao_prefix);

	if ((strlen(cacao_prefix) + strlen("/..") + strlen("0")) > 4096)
		vm_abort("libjvm name to long for buffer\n");

	/* concatenate the library name */

	strcat(cacao_prefix, "/..");

	/* now set path to libjvm.so */

	len = strlen(cacao_prefix) + strlen("/lib/libjvm") + strlen("0");

	cacao_libjvm = MNEW(char, len);
	strcpy(cacao_libjvm, cacao_prefix);
	strcat(cacao_libjvm, "/lib/libjvm");

	/* and finally set the path to GNU Classpath libraries */

	len = strlen(cacao_prefix) + strlen("/lib/classpath") + strlen("0");

	classpath_libdir = MNEW(char, len);
	strcpy(classpath_libdir, cacao_prefix);
	strcat(classpath_libdir, "/lib/classpath");
#else
	cacao_prefix     = CACAO_PREFIX;
	cacao_libjvm     = CACAO_LIBDIR"/libjvm";
	classpath_libdir = CLASSPATH_LIBDIR"/classpath";
#endif

	/* set the bootclasspath */

	cp = getenv("BOOTCLASSPATH");

	if (cp != NULL) {
		_Jv_bootclasspath = MNEW(char, strlen(cp) + strlen("0"));
		strcpy(_Jv_bootclasspath, cp);
	}
	else {
#if defined(WITH_JRE_LAYOUT)
		len =
# if defined(WITH_CLASSPATH_GNU)
			strlen(cacao_prefix) +
			strlen("/share/cacao/vm.zip") +
			strlen(":") +
# endif
			strlen(cacao_prefix) +
			strlen("/share/classpath/glibj.zip") +
			strlen("0");

		_Jv_bootclasspath = MNEW(char, len);
# if defined(WITH_CLASSPATH_GNU)
		strcat(_Jv_bootclasspath, cacao_prefix);
		strcat(_Jv_bootclasspath, "/share/cacao/vm.zip");
		strcat(_Jv_bootclasspath, ":");
# endif
		strcat(_Jv_bootclasspath, cacao_prefix);
		strcat(_Jv_bootclasspath, "/share/classpath/glibj.zip");
#else
		len =
# if defined(WITH_CLASSPATH_GNU)
			strlen(CACAO_VM_ZIP) +
			strlen(":") +
# endif
			strlen(CLASSPATH_CLASSES) +
			strlen("0");

		_Jv_bootclasspath = MNEW(char, len);
# if defined(WITH_CLASSPATH_GNU)
		strcat(_Jv_bootclasspath, CACAO_VM_ZIP);
		strcat(_Jv_bootclasspath, ":");
# endif
		strcat(_Jv_bootclasspath, CLASSPATH_CLASSES);
#endif
	}

	/* set the classpath */

	cp = getenv("CLASSPATH");

	if (cp != NULL) {
		_Jv_classpath = MNEW(char, strlen(cp) + strlen("0"));
		strcat(_Jv_classpath, cp);
	}
	else {
		_Jv_classpath = MNEW(char, strlen(".") + strlen("0"));
		strcpy(_Jv_classpath, ".");
	}

	/* get and set java.library.path */

	_Jv_java_library_path = getenv("LD_LIBRARY_PATH");

	if (_Jv_java_library_path == NULL)
		_Jv_java_library_path = "";

	/* interpret the options **************************************************/

	opt_version       = false;
	opt_exit          = false;

	opt_noieee        = false;

	opt_heapmaxsize   = HEAP_MAXSIZE;
	opt_heapstartsize = HEAP_STARTSIZE;
	opt_stacksize     = STACK_SIZE;


#if defined(ENABLE_JVMTI)
	/* initialize JVMTI related  **********************************************/
	jvmti = false;
#endif

	/* initialize and fill properties before command-line handling */

	if (!properties_init())
		vm_abort("properties_init failed");

	/* iterate over all passed options */

	while ((opt = options_get(opts, vm_args)) != OPT_DONE) {
		switch (opt) {
		case OPT_FOO:
			opt_foo = true;
			break;

		case OPT_IGNORE:
			break;
			
		case OPT_JAR:
			opt_jar = true;
			break;

		case OPT_D32:
#if SIZEOF_VOID_P == 8
			puts("Running a 32-bit JVM is not supported on this platform.");
			exit(1);
#endif
			break;

		case OPT_D64:
#if SIZEOF_VOID_P == 4
			puts("Running a 64-bit JVM is not supported on this platform.");
			exit(1);
#endif
			break;

		case OPT_CLASSPATH:
			/* forget old classpath and set the argument as new classpath */
			MFREE(_Jv_classpath, char, strlen(_Jv_classpath));

			_Jv_classpath = MNEW(char, strlen(opt_arg) + strlen("0"));
			strcpy(_Jv_classpath, opt_arg);
			break;

		case OPT_D:
			for (i = 0; i < strlen(opt_arg); i++) {
				if (opt_arg[i] == '=') {
					opt_arg[i] = '\0';
					properties_add(opt_arg, opt_arg + i + 1);
					goto opt_d_done;
				}
			}

			/* if no '=' is given, just create an empty property */

			properties_add(opt_arg, "");

		opt_d_done:
			break;

		case OPT_BOOTCLASSPATH:
			/* Forget default bootclasspath and set the argument as
			   new boot classpath. */

			MFREE(_Jv_bootclasspath, char, strlen(_Jv_bootclasspath));

			_Jv_bootclasspath = MNEW(char, strlen(opt_arg) + strlen("0"));
			strcpy(_Jv_bootclasspath, opt_arg);
			break;

		case OPT_BOOTCLASSPATH_A:
			/* append to end of bootclasspath */

			len = strlen(_Jv_bootclasspath);

			_Jv_bootclasspath = MREALLOC(_Jv_bootclasspath,
										 char,
										 len + strlen("0"),
										 len + strlen(":") +
										 strlen(opt_arg) + strlen("0"));

			strcat(_Jv_bootclasspath, ":");
			strcat(_Jv_bootclasspath, opt_arg);
			break;

		case OPT_BOOTCLASSPATH_P:
			/* prepend in front of bootclasspath */

			cp = _Jv_bootclasspath;
			len = strlen(cp);

			_Jv_bootclasspath = MNEW(char, strlen(opt_arg) + strlen(":") +
									 len + strlen("0"));

			strcpy(_Jv_bootclasspath, opt_arg);
			strcat(_Jv_bootclasspath, ":");
			strcat(_Jv_bootclasspath, cp);

			MFREE(cp, char, len);
			break;

		case OPT_BOOTCLASSPATH_C:
			/* use as Java core library, but prepend VM interface classes */

			MFREE(_Jv_bootclasspath, char, strlen(_Jv_bootclasspath));

			len = strlen(CACAO_VM_ZIP) +
				strlen(":") +
				strlen(opt_arg) +
				strlen("0");

			_Jv_bootclasspath = MNEW(char, len);

			strcpy(_Jv_bootclasspath, CACAO_VM_ZIP);
			strcat(_Jv_bootclasspath, ":");
			strcat(_Jv_bootclasspath, opt_arg);
			break;

#if defined(ENABLE_JVMTI)
		case OPT_DEBUG:
			/* this option exists only for compatibility reasons */
			break;

		case OPT_NOAGENT:
			/* I don't know yet what Xnoagent should do. This is only for 
			   compatiblity with eclipse - motse */
			break;

		case OPT_XRUNJDWP:
			agentbypath = true;
			jvmti       = true;
			jdwp        = true;

			len =
				strlen(CACAO_LIBDIR) +
				strlen("/libjdwp.so=") +
				strlen(opt_arg) +
				strlen("0");

			agentarg = MNEW(char, len);

			strcpy(agentarg, CACAO_LIBDIR);
			strcat(agentarg, "/libjdwp.so=");
			strcat(agentarg, &opt_arg[1]);
			break;

		case OPT_AGENTPATH:
			agentbypath = true;

		case OPT_AGENTLIB:
			jvmti = true;
			agentarg = opt_arg;
			break;
#endif
			
		case OPT_MX:
		case OPT_MS:
		case OPT_SS:
			{
				char c;
				c = opt_arg[strlen(opt_arg) - 1];

				if ((c == 'k') || (c == 'K')) {
					j = atoi(opt_arg) * 1024;

				} else if ((c == 'm') || (c == 'M')) {
					j = atoi(opt_arg) * 1024 * 1024;

				} else
					j = atoi(opt_arg);

				if (opt == OPT_MX)
					opt_heapmaxsize = j;
				else if (opt == OPT_MS)
					opt_heapstartsize = j;
				else
					opt_stacksize = j;
			}
			break;

		case OPT_VERBOSE1:
			opt_verbose = true;
			break;

		case OPT_VERBOSE:
			if (strcmp("class", opt_arg) == 0) {
				opt_verboseclass = true;
			}
			else if (strcmp("gc", opt_arg) == 0) {
				opt_verbosegc = true;
			}
			else if (strcmp("jni", opt_arg) == 0) {
				opt_verbosejni = true;
			}
#if !defined(NDEBUG)
			else if (strcmp("call", opt_arg) == 0) {
				opt_verbosecall = true;
			}
			else if (strcmp("exception", opt_arg) == 0) {
				opt_verboseexception = true;
			}
			else if (strcmp("jit", opt_arg) == 0) {
				opt_verbose = true;
				loadverbose = true;
				linkverbose = true;
				initverbose = true;
				compileverbose = true;
			}
			else if (strcmp("memory", opt_arg) == 0) {
				opt_verbosememory = true;

# if defined(ENABLE_STATISTICS)
				/* we also need statistics */

				opt_stat = true;
# endif
			}
			else if (strcmp("threads", opt_arg) == 0) {
				opt_verbosethreads = true;
			}
#endif
			else {
				printf("Unknown -verbose option: %s\n", opt_arg);
				usage();
			}
			break;

		case OPT_DEBUGCOLOR:
			opt_debugcolor = true;
			break;

#if defined(ENABLE_VERIFIER) && defined(TYPECHECK_VERBOSE)
		case OPT_VERBOSETC:
			opt_typecheckverbose = true;
			break;
#endif
				
		case OPT_VERSION:
			opt_version = true;
			opt_exit    = true;
			break;

		case OPT_FULLVERSION:
			fullversion();
			break;

		case OPT_SHOWVERSION:
			opt_version = true;
			break;

		case OPT_NOIEEE:
			opt_noieee = true;
			break;

#if defined(ENABLE_VERIFIER)
		case OPT_NOVERIFY:
			opt_verify = false;
			break;
#endif

#if defined(ENABLE_STATISTICS)
		case OPT_TIME:
			opt_getcompilingtime = true;
			opt_getloadingtime = true;
			break;
					
		case OPT_STAT:
			opt_stat = true;
			break;
#endif
					
		case OPT_LOG:
			log_init(opt_arg);
			break;
			
		case OPT_CHECK:
			for (i = 0; i < strlen(opt_arg); i++) {
				switch (opt_arg[i]) {
				case 'b':
					checkbounds = false;
					break;
				case 's':
					checksync = false;
					break;
				default:
					usage();
				}
			}
			break;
			
		case OPT_LOAD:
			opt_run = false;
			makeinitializations = false;
			break;

		case OPT_EAGER:
			opt_eager = true;
			break;

#if !defined(NDEBUG)
		case OPT_ALL:
			compileall = true;
			opt_run = false;
			makeinitializations = false;
			break;

		case OPT_METHOD:
			opt_run = false;
			opt_method = opt_arg;
			makeinitializations = false;
			break;

		case OPT_SIGNATURE:
			opt_signature = opt_arg;
			break;
#endif

		case OPT_SHOW:       /* Display options */
			for (i = 0; i < strlen(opt_arg); i++) {		
				switch (opt_arg[i]) {
				case 'c':
					showconstantpool = true;
					break;

				case 'u':
					showutf = true;
					break;

				case 'm':
					showmethods = true;
					break;

				case 'i':
					opt_showintermediate = true;
					compileverbose = true;
					break;

#if defined(ENABLE_DISASSEMBLER)
				case 'a':
					opt_showdisassemble = true;
					compileverbose = true;
					break;

				case 'o':
					opt_shownops = true;
					break;

				case 'e':
					opt_showexceptionstubs = true;
					break;

				case 'n':
					opt_shownativestub = true;
					break;
#endif

				case 'd':
					opt_showddatasegment = true;
					break;

				default:
					usage();
				}
			}
			break;
			
#if defined(ENABLE_LOOP)
		case OPT_OLOOP:
			opt_loops = true;
			break;
#endif

#if defined(ENABLE_INLINING)
#if defined(ENABLE_INLINING_DEBUG)
		case OPT_INLINE_DEBUG_ALL:
			opt_inline_debug_all = true;
			break;
		case OPT_INLINE_DEBUG_END:
			opt_inline_debug_end_counter = atoi(opt_arg);
			break;
		case OPT_INLINE_DEBUG_MIN:
			opt_inline_debug_min_size = atoi(opt_arg);
			break;
		case OPT_INLINE_DEBUG_MAX:
			opt_inline_debug_max_size = atoi(opt_arg);
			break;
		case OPT_INLINE_REPLACE_VERBOSE:
			opt_replace_verbose = 1;
			break;
		case OPT_INLINE_REPLACE_VERBOSE2:
			opt_replace_verbose = 2;
			break;
#endif /* defined(ENABLE_INLINING_DEBUG) */
#if !defined(NDEBUG)
		case OPT_INLINE_LOG:
			opt_inline_debug_log = true;
			break;
#endif /* !defined(NDEBUG) */

		case OPT_INLINING:
			opt_inlining = true;
			break;
#endif /* defined(ENABLE_INLINING) */

#if defined(ENABLE_IFCONV)
		case OPT_IFCONV:
			opt_ifconv = true;
			break;
#endif

#if defined(ENABLE_LSRA) || defined(ENABLE_SSA)
		case OPT_LSRA:
			opt_lsra = true;
			break;
#endif

		case OPT_HELP:
			usage();
			break;

		case OPT_X:
			Xusage();
			break;

		case OPT_XX:
			options_xx(opt_arg);
			break;

		case OPT_EA:
			/* currently ignored */
			break;

		case OPT_DA:
			/* currently ignored */
			break;

		case OPT_ESA:
			_Jv_jvm->Java_java_lang_VMClassLoader_defaultAssertionStatus = true;
			break;

		case OPT_DSA:
			_Jv_jvm->Java_java_lang_VMClassLoader_defaultAssertionStatus = false;
			break;

#if defined(ENABLE_PROFILING)
		case OPT_PROF_OPTION:
			/* use <= to get the last \0 too */

			for (i = 0, j = 0; i <= strlen(opt_arg); i++) {
				if (opt_arg[i] == ',')
					opt_arg[i] = '\0';

				if (opt_arg[i] == '\0') {
					if (strcmp("bb", opt_arg + j) == 0)
						opt_prof_bb = true;

					else {
						printf("Unknown option: -Xprof:%s\n", opt_arg + j);
						usage();
					}

					/* set k to next char */

					j = i + 1;
				}
			}
			/* fall through */

		case OPT_PROF:
			opt_prof = true;
			break;
#endif

		case OPT_JIT:
#if defined(ENABLE_JIT)
			opt_jit = true;
#else
			printf("-Xjit option not enabled.\n");
			exit(1);
#endif
			break;

		case OPT_INTRP:
#if defined(ENABLE_INTRP)
			opt_intrp = true;
#else
			printf("-Xint option not enabled.\n");
			exit(1);
#endif
			break;

#if defined(ENABLE_INTRP)
		case OPT_STATIC_SUPERS:
			opt_static_supers = atoi(opt_arg);
			break;

		case OPT_NO_DYNAMIC:
			opt_no_dynamic = true;
			break;

		case OPT_NO_REPLICATION:
			opt_no_replication = true;
			break;

		case OPT_NO_QUICKSUPER:
			opt_no_quicksuper = true;
			break;

		case OPT_TRACE:
			vm_debug = true;
			break;
#endif

#if defined(ENABLE_DEBUG_FILTER)
		case OPT_FILTER_VERBOSECALL_INCLUDE:
			opt_filter_verbosecall_include = opt_arg;
			break;

		case OPT_FILTER_VERBOSECALL_EXCLUDE:
			opt_filter_verbosecall_exclude = opt_arg;
			break;

		case OPT_FILTER_SHOW_METHOD:
			opt_filter_show_method = opt_arg;
			break;

#endif
		default:
			printf("Unknown option: %s\n",
				   vm_args->options[opt_index].optionString);
			usage();
		}
	}

	/* get the main class *****************************************************/

	if (opt_index < vm_args->nOptions) {
		mainstring = vm_args->options[opt_index++].optionString;

		/* Put the jar file into the classpath (if any). */

		if (opt_jar == true) {
			/* free old classpath */

			MFREE(_Jv_classpath, char, strlen(_Jv_classpath));

			/* put jarfile into classpath */

			_Jv_classpath = MNEW(char, strlen(mainstring) + strlen("0"));

			strcpy(_Jv_classpath, mainstring);
		}
		else {
			/* replace .'s with /'s in classname */

			for (i = strlen(mainstring) - 1; i >= 0; i--)
				if (mainstring[i] == '.')
					mainstring[i] = '/';
		}
	}

#if defined(ENABLE_JVMTI)
	if (jvmti) {
		jvmti_set_phase(JVMTI_PHASE_ONLOAD);
		jvmti_agentload(agentarg, agentbypath, &handle, &libname);

		if (jdwp)
			MFREE(agentarg, char, strlen(agentarg));

		jvmti_set_phase(JVMTI_PHASE_PRIMORDIAL);
	}
#endif

	/* initialize this JVM ****************************************************/

	vm_initializing = true;

#if defined(ENABLE_THREADS)
	/* pre-initialize some core thread stuff, like the stopworldlock,
	   thus this has to happen _before_ gc_init()!!! */

  	threads_preinit();
#endif

	/* initialize the garbage collector */

	gc_init(opt_heapmaxsize, opt_heapstartsize);

	/* install architecture dependent signal handlers */

	if (!signal_init())
		vm_abort("vm_create: signal_init failed");

#if defined(ENABLE_INTRP)
	/* Allocate main thread stack on the Java heap. */

	if (opt_intrp) {
		intrp_main_stack = GCMNEW(u1, opt_stacksize);
		MSET(intrp_main_stack, 0, u1, opt_stacksize);
	}
#endif

	/* AFTER: threads_preinit */

	if (!string_init())
		vm_abort("vm_create: string_init failed");

	/* AFTER: threads_preinit */

	if (!utf8_init())
		vm_abort("vm_create: utf8_init failed");

	/* AFTER: thread_preinit */

	if (!suck_init())
		vm_abort("vm_create: suck_init failed");

	suck_add_from_property("java.endorsed.dirs");

	/* Now we have all options handled and we can print the version
	   information.

	   AFTER: suck_add_from_property("java.endorsed.dirs"); */

	if (opt_version)
		version(opt_exit);

	/* AFTER: utf8_init */

	suck_add(_Jv_bootclasspath);

	/* Now re-set some of the properties that may have changed. This
	   must be done after _all_ environment variables have been
	   processes (e.g. -jar handling).

	   AFTER: suck_add_from_property, since it may change the
	   _Jv_bootclasspath pointer. */

	if (!properties_postinit())
		vm_abort("vm_create: properties_postinit failed");

	/* initialize the classcache hashtable stuff: lock, hashtable
	   (must be done _after_ threads_preinit) */

	if (!classcache_init())
		vm_abort("vm_create: classcache_init failed");

	/* initialize the memory subsystem (must be done _after_
	   threads_preinit) */

	if (!memory_init())
		vm_abort("vm_create: memory_init failed");

	/* initialize the finalizer stuff (must be done _after_
	   threads_preinit) */

	if (!finalizer_init())
		vm_abort("vm_create: finalizer_init failed");

	/* initialize the codegen subsystems */

	codegen_init();

	/* initializes jit compiler */

	jit_init();

	/* machine dependent initialization */

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (opt_intrp)
		intrp_md_init();
	else
# endif
		md_init();
#else
	intrp_md_init();
#endif

	/* initialize the loader subsystems (must be done _after_
       classcache_init) */

	if (!loader_init())
		vm_abort("vm_create: loader_init failed");

	/* Link some important VM classes. */
	/* AFTER: utf8_init */

	if (!linker_init())
		vm_abort("vm_create: linker_init failed");

	if (!primitive_init())
		vm_abort("vm_create: primitive_init failed");

	/* Initialize the native subsystem. */

	if (!native_init())
		vm_abort("vm_create: native_init failed");

	if (!exceptions_init())
		vm_abort("vm_create: exceptions_init failed");

	if (!builtin_init())
		vm_abort("vm_create: builtin_init failed");

#if defined(ENABLE_JNI)
	/* Initialize the JNI subsystem (must be done _before_
	   threads_init, as threads_init can call JNI methods
	   (e.g. NewGlobalRef). */

	if (!jni_init())
		vm_abort("vm_create: jni_init failed");
#endif

#if defined(ENABLE_THREADS)
  	if (!threads_init())
		vm_abort("vm_create: threads_init failed");
#endif

#if defined(ENABLE_PROFILING)
	/* initialize profiling */

	if (!profile_init())
		vm_abort("vm_create: profile_init failed");
#endif

#if defined(ENABLE_THREADS)
	/* initialize recompilation */

	if (!recompile_init())
		vm_abort("vm_create: recompile_init failed");

	/* start the signal handler thread */

#if defined(__LINUX__)
	/* XXX Remove for exact-GC. */
	if (threads_pthreads_implementation_nptl)
#endif
		if (!signal_start_thread())
			vm_abort("vm_create: signal_start_thread failed");

	/* finally, start the finalizer thread */

	if (!finalizer_start_thread())
		vm_abort("vm_create: finalizer_start_thread failed");

# if !defined(NDEBUG)
	/* start the memory profiling thread */

	if (opt_verbosememory)
		if (!memory_start_thread())
			vm_abort("vm_create: memory_start_thread failed");
# endif

	/* start the recompilation thread (must be done before the
	   profiling thread) */

	if (!recompile_start_thread())
		vm_abort("vm_create: recompile_start_thread failed");

# if defined(ENABLE_PROFILING)
	/* start the profile sampling thread */

/* 	if (opt_prof) */
/* 		if (!profile_start_thread()) */
/* 			vm_abort("vm_create: profile_start_thread failed"); */
# endif
#endif

#if defined(ENABLE_JVMTI)
	if (jvmti) {
		/* add agent library to native library hashtable */
		native_hashtable_library_add(utf_new_char(libname), class_java_lang_Object->classloader, handle);
	}
#endif

	/* increment the number of VMs */

	vms++;

	/* initialization is done */

	vm_initializing = false;

	/* everything's ok */

	return true;
}


/* vm_run **********************************************************************

   Runs the main-method of the passed class.

*******************************************************************************/

void vm_run(JavaVM *vm, JavaVMInitArgs *vm_args)
{
	utf               *mainutf;
	classinfo         *mainclass;
	java_objectheader *e;
	methodinfo        *m;
	java_objectarray  *oa; 
	s4                 oalength;
	utf               *u;
	java_objectheader *s;
	s4                 status;
	s4                 i;

#if !defined(NDEBUG)
	if (compileall) {
		vm_compile_all();
		return;
	}

	if (opt_method != NULL) {
		vm_compile_method();
		return;
	}
#endif /* !defined(NDEBUG) */

	/* should we run the main-method? */

	if (mainstring == NULL)
		usage();

	/* set return value to OK */

	status = 0;

	if (opt_jar == true) {
		/* open jar file with java.util.jar.JarFile */

		mainstring = vm_get_mainclass_from_jar(mainstring);

		if (mainstring == NULL)
			vm_exit(1);
	}

	/* load the main class */

	mainutf = utf_new_char(mainstring);

#if defined(ENABLE_JAVAME_CLDC1_1)
	mainclass = load_class_bootstrap(mainutf);
#else
	mainclass = load_class_from_sysloader(mainutf);
#endif

	/* error loading class */

	e = exceptions_get_and_clear_exception();

	if ((e != NULL) || (mainclass == NULL)) {
		exceptions_throw_noclassdeffounderror_cause(e);
		exceptions_print_stacktrace(); 
		vm_exit(1);
	}

	if (!link_class(mainclass)) {
		exceptions_print_stacktrace();
		vm_exit(1);
	}
			
	/* find the `main' method of the main class */

	m = class_resolveclassmethod(mainclass,
								 utf_new_char("main"), 
								 utf_new_char("([Ljava/lang/String;)V"),
								 class_java_lang_Object,
								 false);

	if (exceptions_get_exception()) {
		exceptions_print_stacktrace();
		vm_exit(1);
	}

	/* there is no main method or it isn't static */

	if ((m == NULL) || !(m->flags & ACC_STATIC)) {
		exceptions_clear_exception();
		exceptions_throw_nosuchmethoderror(mainclass,
										   utf_new_char("main"), 
										   utf_new_char("([Ljava/lang/String;)V"));

		exceptions_print_stacktrace();
		vm_exit(1);
	}

	/* build argument array */

	oalength = vm_args->nOptions - opt_index;

	oa = builtin_anewarray(oalength, class_java_lang_String);

	for (i = 0; i < oalength; i++) {
		u = utf_new_char(vm_args->options[opt_index + i].optionString);
		s = javastring_new(u);

		oa->data[i] = s;
	}

#ifdef TYPEINFO_DEBUG_TEST
	/* test the typeinfo system */
	typeinfo_test();
#endif
	/*class_showmethods(currentThread->group->header.vftbl->class);	*/

#if defined(ENABLE_JVMTI)
	jvmti_set_phase(JVMTI_PHASE_LIVE);
#endif

	/* set ThreadMXBean variables */

	_Jv_jvm->java_lang_management_ThreadMXBean_ThreadCount++;
	_Jv_jvm->java_lang_management_ThreadMXBean_TotalStartedThreadCount++;

	if (_Jv_jvm->java_lang_management_ThreadMXBean_ThreadCount >
		_Jv_jvm->java_lang_management_ThreadMXBean_PeakThreadCount)
		_Jv_jvm->java_lang_management_ThreadMXBean_PeakThreadCount =
			_Jv_jvm->java_lang_management_ThreadMXBean_ThreadCount;

	/* start the main thread */

	(void) vm_call_method(m, NULL, oa);

	/* exception occurred? */

	if (exceptions_get_exception()) {
		exceptions_print_stacktrace();
		status = 1;
	}

	/* unload the JavaVM */

	(void) vm_destroy(vm);

	/* and exit */

	vm_exit(status);
}


/* vm_destroy ******************************************************************

   Unloads a Java VM and reclaims its resources.

*******************************************************************************/

s4 vm_destroy(JavaVM *vm)
{
#if defined(ENABLE_THREADS)
	threads_join_all_threads();
#endif

	/* everything's ok */

	return 0;
}


/* vm_exit *********************************************************************

   Calls java.lang.System.exit(I)V to exit the JavaVM correctly.

*******************************************************************************/

void vm_exit(s4 status)
{
	methodinfo *m;

	/* signal that we are exiting */

	vm_exiting = true;

	assert(class_java_lang_System);
	assert(class_java_lang_System->state & CLASS_LOADED);

#if defined(ENABLE_JVMTI)
	if (jvmti || (dbgcom!=NULL)) {
		jvmti_set_phase(JVMTI_PHASE_DEAD);
		if (jvmti) jvmti_agentunload();
	}
#endif

	if (!link_class(class_java_lang_System)) {
		exceptions_print_stacktrace();
		exit(1);
	}

	/* call java.lang.System.exit(I)V */

	m = class_resolveclassmethod(class_java_lang_System,
								 utf_new_char("exit"),
								 utf_int__void,
								 class_java_lang_Object,
								 true);
	
	if (m == NULL) {
		exceptions_print_stacktrace();
		exit(1);
	}

	/* call the exit function with passed exit status */

	(void) vm_call_method(m, NULL, status);

	/* If we had an exception, just ignore the exception and exit with
	   the proper code. */

	vm_shutdown(status);
}


/* vm_shutdown *****************************************************************

   Terminates the system immediately without freeing memory explicitly
   (to be used only for abnormal termination).
	
*******************************************************************************/

void vm_shutdown(s4 status)
{
	if (opt_verbose 
#if defined(ENABLE_STATISTICS)
		|| opt_getcompilingtime || opt_stat
#endif
	   ) 
	{
		log_text("CACAO terminated by shutdown");
		dolog("Exit status: %d\n", (s4) status);

	}

#if defined(ENABLE_JVMTI)
	/* terminate cacaodbgserver */
	if (dbgcom!=NULL) {
		pthread_mutex_lock(&dbgcomlock);
		dbgcom->running=1;
		pthread_mutex_unlock(&dbgcomlock);
		jvmti_cacaodbgserver_quit();
	}	
#endif

	exit(status);
}


/* vm_exit_handler *************************************************************

   The exit_handler function is called upon program termination.

   ATTENTION: Don't free system resources here! Some threads may still
   be running as this is called from VMRuntime.exit(). The OS does the
   cleanup for us.

*******************************************************************************/

void vm_exit_handler(void)
{
#if !defined(NDEBUG)
	if (showmethods)
		class_showmethods(mainclass);

	if (showconstantpool)
		class_showconstantpool(mainclass);

	if (showutf)
		utf_show();

# if defined(ENABLE_PROFILING)
	if (opt_prof)
		profile_printstats();
# endif
#endif /* !defined(NDEBUG) */

#if defined(ENABLE_RT_TIMING)
 	rt_timing_print_time_stats(stderr);
#endif

#if defined(ENABLE_CYCLES_STATS)
	builtin_print_cycles_stats(stderr);
	stacktrace_print_cycles_stats(stderr);
#endif

	if (opt_verbose 
#if defined(ENABLE_STATISTICS)
		|| opt_getcompilingtime || opt_stat
#endif
	   ) 
	{
		log_text("CACAO terminated");

#if defined(ENABLE_STATISTICS)
		if (opt_stat) {
			print_stats();
#ifdef TYPECHECK_STATISTICS
			typecheck_print_statistics(get_logfile());
#endif
		}

		if (opt_getcompilingtime)
			print_times();
#endif /* defined(ENABLE_STATISTICS) */
	}
	/* vm_print_profile(stderr);*/
}


/* vm_abort ********************************************************************

   Prints an error message and aborts the VM.

*******************************************************************************/

void vm_abort(const char *text, ...)
{
	va_list ap;

	/* print the log message */

	log_start();

	va_start(ap, text);
	log_vprint(text, ap);
	va_end(ap);

	log_finish();

	/* now abort the VM */

	abort();
}


/* vm_get_mainclass_from_jar ***************************************************

   Gets the name of the main class from a JAR's manifest file.

*******************************************************************************/

static char *vm_get_mainclass_from_jar(char *mainstring)
{
	classinfo         *c;
	java_objectheader *o;
	methodinfo        *m;
	java_objectheader *s;

	c = load_class_from_sysloader(utf_new_char("java/util/jar/JarFile"));

	if (c == NULL) {
		exceptions_print_stacktrace();
		return NULL;
	}

	/* create JarFile object */

	o = builtin_new(c);

	if (o == NULL) {
		exceptions_print_stacktrace();
		return NULL;
	}

	m = class_resolveclassmethod(c,
								 utf_init, 
								 utf_java_lang_String__void,
								 class_java_lang_Object,
								 true);

	if (m == NULL) {
		exceptions_print_stacktrace();
		return NULL;
	}

	s = javastring_new_from_ascii(mainstring);

	(void) vm_call_method(m, o, s);

	if (exceptions_get_exception()) {
		exceptions_print_stacktrace();
		return NULL;
	}

	/* get manifest object */

	m = class_resolveclassmethod(c,
								 utf_new_char("getManifest"), 
								 utf_new_char("()Ljava/util/jar/Manifest;"),
								 class_java_lang_Object,
								 true);

	if (m == NULL) {
		exceptions_print_stacktrace();
		return NULL;
	}

	o = vm_call_method(m, o);

	if (o == NULL) {
		fprintf(stderr, "Could not get manifest from %s (invalid or corrupt jarfile?)\n", mainstring);
		return NULL;
	}


	/* get Main Attributes */

	m = class_resolveclassmethod(o->vftbl->class,
								 utf_new_char("getMainAttributes"), 
								 utf_new_char("()Ljava/util/jar/Attributes;"),
								 class_java_lang_Object,
								 true);

	if (m == NULL) {
		exceptions_print_stacktrace();
		return NULL;
	}

	o = vm_call_method(m, o);

	if (o == NULL) {
		fprintf(stderr, "Could not get main attributes from %s (invalid or corrupt jarfile?)\n", mainstring);
		return NULL;
	}


	/* get property Main-Class */

	m = class_resolveclassmethod(o->vftbl->class,
								 utf_new_char("getValue"), 
								 utf_new_char("(Ljava/lang/String;)Ljava/lang/String;"),
								 class_java_lang_Object,
								 true);

	if (m == NULL) {
		exceptions_print_stacktrace();
		return NULL;
	}

	s = javastring_new_from_ascii("Main-Class");

	o = vm_call_method(m, o, s);

	if (o == NULL) {
		exceptions_print_stacktrace();
		return NULL;
	}

	return javastring_tochar(o);
}


/* vm_compile_all **************************************************************

   Compile all methods found in the bootclasspath.

*******************************************************************************/

#if !defined(NDEBUG)
static void vm_compile_all(void)
{
	classinfo              *c;
	methodinfo             *m;
	u4                      slot;
	classcache_name_entry  *nmen;
	classcache_class_entry *clsen;
	s4                      i;

	/* create all classes found in the bootclasspath */
	/* XXX currently only works with zip/jar's */

	loader_load_all_classes();

	/* link all classes */

	for (slot = 0; slot < hashtable_classcache.size; slot++) {
		nmen = (classcache_name_entry *) hashtable_classcache.ptr[slot];

		for (; nmen; nmen = nmen->hashlink) {
			/* iterate over all class entries */

			for (clsen = nmen->classes; clsen; clsen = clsen->next) {
				c = clsen->classobj;

				if (c == NULL)
					continue;

				if (!(c->state & CLASS_LINKED)) {
					if (!link_class(c)) {
						fprintf(stderr, "Error linking: ");
						utf_fprint_printable_ascii_classname(stderr, c->name);
						fprintf(stderr, "\n");

						/* print out exception and cause */

						exceptions_print_current_exception();

						/* goto next class */

						continue;
					}
				}

				/* compile all class methods */

				for (i = 0; i < c->methodscount; i++) {
					m = &(c->methods[i]);

					if (m->jcode != NULL) {
						if (!jit_compile(m)) {
							fprintf(stderr, "Error compiling: ");
							utf_fprint_printable_ascii_classname(stderr, c->name);
							fprintf(stderr, ".");
							utf_fprint_printable_ascii(stderr, m->name);
							utf_fprint_printable_ascii(stderr, m->descriptor);
							fprintf(stderr, "\n");

							/* print out exception and cause */

							exceptions_print_current_exception();
						}
					}
				}
			}
		}
	}
}
#endif /* !defined(NDEBUG) */


/* vm_compile_method ***********************************************************

   Compile a specific method.

*******************************************************************************/

#if !defined(NDEBUG)
static void vm_compile_method(void)
{
	methodinfo *m;

	/* create, load and link the main class */

	mainclass = load_class_bootstrap(utf_new_char(mainstring));

	if (mainclass == NULL)
		exceptions_print_stacktrace();

	if (!link_class(mainclass))
		exceptions_print_stacktrace();

	if (opt_signature != NULL) {
		m = class_resolveclassmethod(mainclass,
									 utf_new_char(opt_method),
									 utf_new_char(opt_signature),
									 mainclass,
									 false);
	}
	else {
		m = class_resolveclassmethod(mainclass,
									 utf_new_char(opt_method),
									 NULL,
									 mainclass,
									 false);
	}

	if (m == NULL)
		vm_abort("vm_compile_method: java.lang.NoSuchMethodException: %s.%s",
				 opt_method, opt_signature ? opt_signature : "");
		
	jit_compile(m);
}
#endif /* !defined(NDEBUG) */


/* vm_array_store_int **********************************************************

   Helper function to store an integer into the argument array, taking
   care of architecture specific issues.

*******************************************************************************/

static void vm_array_store_int(uint64_t *array, paramdesc *pd, int32_t value)
{
	int32_t index;

	if (!pd->inmemory) {
		index        = pd->index;
		array[index] = (int64_t) value;
	}
	else {
		index        = ARG_CNT + pd->regoff;
#if WORDS_BIGENDIAN == 1 && !defined(__POWERPC64__)
		array[index] = ((int64_t) value) << 32;
#else
		array[index] = (int64_t) value;
#endif
	}
}


/* vm_array_store_lng **********************************************************

   Helper function to store a long into the argument array, taking
   care of architecture specific issues.

*******************************************************************************/

static void vm_array_store_lng(uint64_t *array, paramdesc *pd, int64_t value)
{
	int32_t index;

#if SIZEOF_VOID_P == 8
	if (!pd->inmemory)
		index = pd->index;
	else
		index = ARG_CNT + pd->regoff;

	array[index] = value;
#else
	if (!pd->inmemory) {
		/* move low and high 32-bits into it's own argument slot */

		index        = GET_LOW_REG(pd->index);
		array[index] = value & 0x00000000ffffffff;

		index        = GET_HIGH_REG(pd->index);
		array[index] = value >> 32;
	}
	else {
		index        = ARG_CNT + pd->regoff;
		array[index] = value;
	}
#endif
}


/* vm_array_store_flt **********************************************************

   Helper function to store a float into the argument array, taking
   care of architecture specific issues.

*******************************************************************************/

static void vm_array_store_flt(uint64_t *array, paramdesc *pd, uint64_t value)
{
	int32_t index;

	if (!pd->inmemory) {
		index        = INT_ARG_CNT + pd->index;
#if WORDS_BIGENDIAN == 1 && !defined(__POWERPC64__)
		array[index] = value >> 32;
#else
		array[index] = value;
#endif
	}
	else {
		index        = ARG_CNT + pd->regoff;
		array[index] = value;
	}
}


/* vm_array_store_dbl **********************************************************

   Helper function to store a double into the argument array, taking
   care of architecture specific issues.

*******************************************************************************/

static void vm_array_store_dbl(uint64_t *array, paramdesc *pd, uint64_t value)
{
	int32_t index;

	if (!pd->inmemory)
		index = INT_ARG_CNT + pd->index;
	else
		index = ARG_CNT + pd->regoff;

	array[index] = value;
}


/* vm_array_store_adr **********************************************************

   Helper function to store an address into the argument array, taking
   care of architecture specific issues.

*******************************************************************************/

static void vm_array_store_adr(uint64_t *array, paramdesc *pd, void *value)
{
	int32_t index;

	if (!pd->inmemory) {
#if defined(HAS_ADDRESS_REGISTER_FILE)
		/* When the architecture has address registers, place them
		   after integer and float registers. */

		index        = INT_ARG_CNT + FLT_ARG_CNT + pd->index;
#else
		index        = pd->index;
#endif
		array[index] = (uint64_t) (intptr_t) value;
	}
	else {
		index        = ARG_CNT + pd->regoff;
#if SIZEOF_VOID_P == 8
		array[index] = (uint64_t) (intptr_t) value;
#else
# if WORDS_BIGENDIAN == 1 && !defined(__POWERPC64__)
		array[index] = ((uint64_t) (intptr_t) value) << 32;
# else
		array[index] = (uint64_t) (intptr_t) value;
# endif
#endif
	}
}


/* vm_vmargs_from_valist *******************************************************

   XXX

*******************************************************************************/

#if !defined(__MIPS__) && !defined(__X86_64__) && !defined(__POWERPC64__)
static void vm_vmargs_from_valist(methodinfo *m, java_objectheader *o,
								  vm_arg *vmargs, va_list ap)
{
	typedesc *paramtypes;
	s4        i;

	paramtypes = m->parseddesc->paramtypes;

	/* if method is non-static fill first block and skip `this' pointer */

	i = 0;

	if (o != NULL) {
		/* the `this' pointer */
		vmargs[0].type   = TYPE_ADR;
		vmargs[0].data.l = (u8) (ptrint) o;

		paramtypes++;
		i++;
	} 

	for (; i < m->parseddesc->paramcount; i++, paramtypes++) {
		switch (paramtypes->type) {
		case TYPE_INT:
			vmargs[i].type   = TYPE_INT;
			vmargs[i].data.l = (s8) va_arg(ap, s4);
			break;

		case TYPE_LNG:
			vmargs[i].type   = TYPE_LNG;
			vmargs[i].data.l = (s8) va_arg(ap, s8);
			break;

		case TYPE_FLT:
			vmargs[i].type   = TYPE_FLT;
#if defined(__ALPHA__)
			/* this keeps the assembler function much simpler */

			vmargs[i].data.d = (jdouble) va_arg(ap, jdouble);
#else
			vmargs[i].data.f = (jfloat) va_arg(ap, jdouble);
#endif
			break;

		case TYPE_DBL:
			vmargs[i].type   = TYPE_DBL;
			vmargs[i].data.d = (jdouble) va_arg(ap, jdouble);
			break;

		case TYPE_ADR: 
			vmargs[i].type   = TYPE_ADR;
			vmargs[i].data.l = (u8) (ptrint) va_arg(ap, void*);
			break;
		}
	}
}
#else
uint64_t *vm_array_from_valist(methodinfo *m, java_objectheader *o, va_list ap)
{
	methoddesc *md;
	paramdesc  *pd;
	typedesc   *td;
	uint64_t   *array;
	int32_t     i;
	imm_union   value;

	/* get the descriptors */

	md = m->parseddesc;
	pd = md->params;
	td = md->paramtypes;

	/* allocate argument array */

	array = DMNEW(uint64_t, INT_ARG_CNT + FLT_ARG_CNT + md->memuse);

	/* if method is non-static fill first block and skip `this' pointer */

	i = 0;

	if (o != NULL) {
		/* the `this' pointer */
		vm_array_store_adr(array, pd, o);

		pd++;
		td++;
		i++;
	} 

	for (; i < md->paramcount; i++, pd++, td++) {
		switch (td->type) {
		case TYPE_INT:
			value.i = va_arg(ap, int32_t);
			vm_array_store_int(array, pd, value.i);
			break;

		case TYPE_LNG:
			value.l = va_arg(ap, int64_t);
			vm_array_store_lng(array, pd, value.l);
			break;

		case TYPE_FLT:
#if defined(__ALPHA__) || defined(__POWERPC64__)
			/* this keeps the assembler function much simpler */

			value.d = (double) va_arg(ap, double);
#else
			value.f = (float) va_arg(ap, double);
#endif
			vm_array_store_flt(array, pd, value.l);
			break;

		case TYPE_DBL:
			value.d = va_arg(ap, double);
			vm_array_store_dbl(array, pd, value.l);
			break;

		case TYPE_ADR: 
			value.a = va_arg(ap, void*);
			vm_array_store_adr(array, pd, value.a);
			break;
		}
	}

	return array;
}
#endif


/* vm_vmargs_from_jvalue *******************************************************

   XXX

*******************************************************************************/

#if !defined(__MIPS__) && !defined(__X86_64__) && !defined(__POWERPC64__)
static void vm_vmargs_from_jvalue(methodinfo *m, java_objectheader *o,
								  vm_arg *vmargs, jvalue *args)
{
	typedesc *paramtypes;
	s4        i;
	s4        j;

	paramtypes = m->parseddesc->paramtypes;

	/* if method is non-static fill first block and skip `this' pointer */

	i = 0;

	if (o != NULL) {
		/* the `this' pointer */
		vmargs[0].type   = TYPE_ADR;
		vmargs[0].data.l = (u8) (ptrint) o;

		paramtypes++;
		i++;
	} 

	for (j = 0; i < m->parseddesc->paramcount; i++, j++, paramtypes++) {
		switch (paramtypes->decltype) {
		case TYPE_INT:
			vmargs[i].type   = TYPE_INT;
			vmargs[i].data.l = (s8) args[j].i;
			break;

		case TYPE_LNG:
			vmargs[i].type   = TYPE_LNG;
			vmargs[i].data.l = (s8) args[j].j;
			break;

		case TYPE_FLT:
			vmargs[i].type = TYPE_FLT;
#if defined(__ALPHA__)
			/* this keeps the assembler function much simpler */

			vmargs[i].data.d = (jdouble) args[j].f;
#else
			vmargs[i].data.f = args[j].f;
#endif
			break;

		case TYPE_DBL:
			vmargs[i].type   = TYPE_DBL;
			vmargs[i].data.d = args[j].d;
			break;

		case TYPE_ADR: 
			vmargs[i].type   = TYPE_ADR;
			vmargs[i].data.l = (u8) (ptrint) args[j].l;
			break;
		}
	}
}
#else
static uint64_t *vm_array_from_jvalue(methodinfo *m, java_objectheader *o,
									  jvalue *args)
{
	methoddesc *md;
	paramdesc  *pd;
	typedesc   *td;
	uint64_t   *array;
	int32_t     i;
	int32_t     j;

	/* get the descriptors */

	md = m->parseddesc;
	pd = md->params;
	td = md->paramtypes;

	/* allocate argument array */

#if defined(HAS_ADDRESS_REGISTER_FILE)
	array = DMNEW(uint64_t, INT_ARG_CNT + FLT_ARG_CNT + ADR_ARG_CNT + md->memuse);
#else
	array = DMNEW(uint64_t, INT_ARG_CNT + FLT_ARG_CNT + md->memuse);
#endif

	/* if method is non-static fill first block and skip `this' pointer */

	i = 0;

	if (o != NULL) {
		/* the `this' pointer */
		vm_array_store_adr(array, pd, o);

		pd++;
		td++;
		i++;
	} 

	for (j = 0; i < md->paramcount; i++, j++, pd++, td++) {
		switch (td->decltype) {
		case TYPE_INT:
			vm_array_store_int(array, pd, args[j].i);
			break;

		case TYPE_LNG:
			vm_array_store_lng(array, pd, args[j].j);
			break;

		case TYPE_FLT:
			vm_array_store_flt(array, pd, args[j].j);
			break;

		case TYPE_DBL:
			vm_array_store_dbl(array, pd, args[j].j);
			break;

		case TYPE_ADR: 
			vm_array_store_adr(array, pd, args[j].l);
			break;
		}
	}

	return array;
}
#endif

/* vm_vmargs_from_objectarray **************************************************

   XXX

*******************************************************************************/

#if !defined(__MIPS__) && !defined(__X86_64__) && !defined(__POWERPC64__)
bool vm_vmargs_from_objectarray(methodinfo *m, java_objectheader *o,
								vm_arg *vmargs, java_objectarray *params)
{
	java_objectheader *param;
	typedesc          *paramtypes;
	classinfo         *c;
	int32_t            i;
	int32_t            j;
	int64_t            value;

	paramtypes = m->parseddesc->paramtypes;

	/* if method is non-static fill first block and skip `this' pointer */

	i = 0;

	if (o != NULL) {
		/* this pointer */
		vmargs[0].type   = TYPE_ADR;
		vmargs[0].data.l = (uint64_t) (intptr_t) o;

		paramtypes++;
		i++;
	}

	for (j = 0; i < m->parseddesc->paramcount; i++, j++, paramtypes++) {
		switch (paramtypes->type) {
		/* primitive types */
		case TYPE_INT:
		case TYPE_LNG:
		case TYPE_FLT:
		case TYPE_DBL:
			param = params->data[j];

			if (param == NULL)
				goto illegal_arg;

			/* internally used data type */
			vmargs[i].type = paramtypes->type;

			/* convert the value according to its declared type */

			c = param->vftbl->class;

			switch (paramtypes->decltype) {
			case PRIMITIVETYPE_BOOLEAN:
				if (c == class_java_lang_Boolean)
					value = (int64_t) ((java_lang_Boolean *) param)->value;
				else
					goto illegal_arg;

				vmargs[i].data.l = value;
				break;

			case PRIMITIVETYPE_BYTE:
				if (c == class_java_lang_Byte)
					value = (int64_t) ((java_lang_Byte *) param)->value;
				else
					goto illegal_arg;

				vmargs[i].data.l = value;
				break;

			case PRIMITIVETYPE_CHAR:
				if (c == class_java_lang_Character)
					value = (int64_t) ((java_lang_Character *) param)->value;
				else
					goto illegal_arg;

				vmargs[i].data.l = value;
				break;

			case PRIMITIVETYPE_SHORT:
				if (c == class_java_lang_Short)
					value = (int64_t) ((java_lang_Short *) param)->value;
				else if (c == class_java_lang_Byte)
					value = (int64_t) ((java_lang_Byte *) param)->value;
				else
					goto illegal_arg;

				vmargs[i].data.l = value;
				break;

			case PRIMITIVETYPE_INT:
				if (c == class_java_lang_Integer)
					value = (int64_t) ((java_lang_Integer *) param)->value;
				else if (c == class_java_lang_Short)
					value = (int64_t) ((java_lang_Short *) param)->value;
				else if (c == class_java_lang_Byte)
					value = (int64_t) ((java_lang_Byte *) param)->value;
				else
					goto illegal_arg;

				vmargs[i].data.l = value;
				break;

			case PRIMITIVETYPE_LONG:
				if (c == class_java_lang_Long)
					value = (int64_t) ((java_lang_Long *) param)->value;
				else if (c == class_java_lang_Integer)
					value = (int64_t) ((java_lang_Integer *) param)->value;
				else if (c == class_java_lang_Short)
					value = (int64_t) ((java_lang_Short *) param)->value;
				else if (c == class_java_lang_Byte)
					value = (int64_t) ((java_lang_Byte *) param)->value;
				else
					goto illegal_arg;

				vmargs[i].data.l = value;
				break;

			case PRIMITIVETYPE_FLOAT:
				if (c == class_java_lang_Float)
					vmargs[i].data.f = (jfloat) ((java_lang_Float *) param)->value;
				else
					goto illegal_arg;
				break;

			case PRIMITIVETYPE_DOUBLE:
				if (c == class_java_lang_Double)
					vmargs[i].data.d = (jdouble) ((java_lang_Double *) param)->value;
				else if (c == class_java_lang_Float)
					vmargs[i].data.f = (jfloat) ((java_lang_Float *) param)->value;
				else
					goto illegal_arg;
				break;

			default:
				goto illegal_arg;
			}
			break;
		
		case TYPE_ADR:
			if (!resolve_class_from_typedesc(paramtypes, true, true, &c))
				return false;

			if (params->data[j] != 0) {
				if (paramtypes->arraydim > 0) {
					if (!builtin_arrayinstanceof(params->data[j], c))
						goto illegal_arg;

				} else {
					if (!builtin_instanceof(params->data[j], c))
						goto illegal_arg;
				}
			}

			vmargs[i].type   = TYPE_ADR;
			vmargs[i].data.l = (u8) (ptrint) params->data[j];
			break;

		default:
			goto illegal_arg;
		}
	}

/*  	if (rettype) */
/*  		*rettype = descr->returntype.decltype; */

	return true;

illegal_arg:
	exceptions_throw_illegalargumentexception();
	return false;
}
#else
uint64_t *vm_array_from_objectarray(methodinfo *m, java_objectheader *o,
									java_objectarray *params)
{
	methoddesc        *md;
	paramdesc         *pd;
	typedesc          *td;
	uint64_t          *array;
	java_objectheader *param;
	classinfo         *c;
	int32_t            i;
	int32_t            j;
	imm_union          value;

	/* get the descriptors */

	md = m->parseddesc;
	pd = md->params;
	td = md->paramtypes;

	/* allocate argument array */

	array = DMNEW(uint64_t, INT_ARG_CNT + FLT_ARG_CNT + md->memuse);

	/* if method is non-static fill first block and skip `this' pointer */

	i = 0;

	if (o != NULL) {
		/* this pointer */
		vm_array_store_adr(array, pd, o);

		pd++;
		td++;
		i++;
	}

	for (j = 0; i < md->paramcount; i++, j++, pd++, td++) {
		param = params->data[j];

		switch (td->type) {
		case TYPE_INT:
			if (param == NULL)
				goto illegal_arg;

			/* convert the value according to its declared type */

			c = param->vftbl->class;

			switch (td->decltype) {
			case PRIMITIVETYPE_BOOLEAN:
				if (c == class_java_lang_Boolean)
					value.i = ((java_lang_Boolean *) param)->value;
				else
					goto illegal_arg;
				break;

			case PRIMITIVETYPE_BYTE:
				if (c == class_java_lang_Byte)
					value.i = ((java_lang_Byte *) param)->value;
				else
					goto illegal_arg;
				break;

			case PRIMITIVETYPE_CHAR:
				if (c == class_java_lang_Character)
					value.i = ((java_lang_Character *) param)->value;
				else
					goto illegal_arg;
				break;

			case PRIMITIVETYPE_SHORT:
				if (c == class_java_lang_Short)
					value.i = ((java_lang_Short *) param)->value;
				else if (c == class_java_lang_Byte)
					value.i = ((java_lang_Byte *) param)->value;
				else
					goto illegal_arg;
				break;

			case PRIMITIVETYPE_INT:
				if (c == class_java_lang_Integer)
					value.i = ((java_lang_Integer *) param)->value;
				else if (c == class_java_lang_Short)
					value.i = ((java_lang_Short *) param)->value;
				else if (c == class_java_lang_Byte)
					value.i = ((java_lang_Byte *) param)->value;
				else
					goto illegal_arg;
				break;

			default:
				goto illegal_arg;
			}

			vm_array_store_int(array, pd, value.i);
			break;

		case TYPE_LNG:
			if (param == NULL)
				goto illegal_arg;

			/* convert the value according to its declared type */

			c = param->vftbl->class;

			switch (td->decltype) {
			case PRIMITIVETYPE_LONG:
				if (c == class_java_lang_Long)
					value.l = ((java_lang_Long *) param)->value;
				else if (c == class_java_lang_Integer)
					value.l = (int64_t) ((java_lang_Integer *) param)->value;
				else if (c == class_java_lang_Short)
					value.l = (int64_t) ((java_lang_Short *) param)->value;
				else if (c == class_java_lang_Byte)
					value.l = (int64_t) ((java_lang_Byte *) param)->value;
				else
					goto illegal_arg;
				break;

			default:
				goto illegal_arg;
			}

			vm_array_store_lng(array, pd, value.l);
			break;

		case TYPE_FLT:
			if (param == NULL)
				goto illegal_arg;

			/* convert the value according to its declared type */

			c = param->vftbl->class;

			switch (td->decltype) {
			case PRIMITIVETYPE_FLOAT:
				if (c == class_java_lang_Float)
					value.f = ((java_lang_Float *) param)->value;
				else
					goto illegal_arg;
				break;

			default:
				goto illegal_arg;
			}

			vm_array_store_flt(array, pd, value.l);
			break;

		case TYPE_DBL:
			if (param == NULL)
				goto illegal_arg;

			/* convert the value according to its declared type */

			c = param->vftbl->class;

			switch (td->decltype) {
			case PRIMITIVETYPE_DOUBLE:
				if (c == class_java_lang_Double)
					value.d = ((java_lang_Double *) param)->value;
				else if (c == class_java_lang_Float)
					value.f = ((java_lang_Float *) param)->value;
				else
					goto illegal_arg;
				break;

			default:
				goto illegal_arg;
			}

			vm_array_store_dbl(array, pd, value.l);
			break;
		
		case TYPE_ADR:
			if (!resolve_class_from_typedesc(td, true, true, &c))
				return false;

			if (param != NULL) {
				if (td->arraydim > 0) {
					if (!builtin_arrayinstanceof(param, c))
						goto illegal_arg;
				}
				else {
					if (!builtin_instanceof(param, c))
						goto illegal_arg;
				}
			}

			vm_array_store_adr(array, pd, param);
			break;

		default:
			goto illegal_arg;
		}
	}

	return array;

illegal_arg:
	exceptions_throw_illegalargumentexception();
	return NULL;
}
#endif


/* vm_call_method **************************************************************

   Calls a Java method with a variable number of arguments and returns
   an address.

*******************************************************************************/

java_objectheader *vm_call_method(methodinfo *m, java_objectheader *o, ...)
{
	va_list            ap;
	java_objectheader *ro;

	va_start(ap, o);
	ro = vm_call_method_valist(m, o, ap);
	va_end(ap);

	return ro;
}


/* vm_call_method_valist *******************************************************

   Calls a Java method with a variable number of arguments, passed via
   a va_list, and returns an address.

*******************************************************************************/

java_objectheader *vm_call_method_valist(methodinfo *m, java_objectheader *o,
										 va_list ap)
{
#if !defined(__MIPS__) && !defined(__X86_64__) && !defined(__POWERPC64__)
	s4                 vmargscount;
	vm_arg            *vmargs;
	java_objectheader *ro;
	s4                 dumpsize;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* get number of Java method arguments */

	vmargscount = m->parseddesc->paramcount;

	/* allocate vm_arg array */

	vmargs = DMNEW(vm_arg, vmargscount);

	/* fill the vm_arg array from a va_list */

	vm_vmargs_from_valist(m, o, vmargs, ap);

	/* call the Java method */

	ro = vm_call_method_vmarg(m, vmargscount, vmargs);

	/* release dump area */

	dump_release(dumpsize);

	return ro;
#else
	java_objectheader *ro;
	int32_t            dumpsize;
	uint64_t          *array;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* fill the argument array from a va_list */

	array = vm_array_from_valist(m, o, ap);

	/* call the Java method */

	ro = vm_call_array(m, array);

	/* release dump area */

	dump_release(dumpsize);

	return ro;
#endif
}


/* vm_call_method_jvalue *******************************************************

   Calls a Java method with a variable number of arguments, passed via
   a jvalue array, and returns an address.

*******************************************************************************/

java_objectheader *vm_call_method_jvalue(methodinfo *m, java_objectheader *o,
										 jvalue *args)
{
#if !defined(__MIPS__) && !defined(__X86_64__) && !defined(__POWERPC64__)
	s4                 vmargscount;
	vm_arg            *vmargs;
	java_objectheader *ro;
	s4                 dumpsize;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* get number of Java method arguments */

	vmargscount = m->parseddesc->paramcount;

	/* allocate vm_arg array */

	vmargs = DMNEW(vm_arg, vmargscount);

	/* fill the vm_arg array from a va_list */

	vm_vmargs_from_jvalue(m, o, vmargs, args);

	/* call the Java method */

	ro = vm_call_method_vmarg(m, vmargscount, vmargs);

	/* release dump area */

	dump_release(dumpsize);

	return ro;
#else
	java_objectheader *ro;
	int32_t            dumpsize;
	uint64_t          *array;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* fill the argument array from a va_list */

	array = vm_array_from_jvalue(m, o, args);

	/* call the Java method */

	ro = vm_call_array(m, array);

	/* release dump area */

	dump_release(dumpsize);

	return ro;
#endif
}


/* vm_call_array ***************************************************************

   Calls a Java method with a variable number of arguments, passed via
   an argument array, and returns an address.

*******************************************************************************/

#if !defined(__MIPS__) && !defined(__X86_64__) && !defined(__POWERPC64__)
java_objectheader *vm_call_method_vmarg(methodinfo *m, s4 vmargscount,
										vm_arg *vmargs)
{
	java_objectheader *o;

	STATISTICS(count_calls_native_to_java++);

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (opt_intrp)
		o = intrp_asm_vm_call_method(m, vmargscount, vmargs);
	else
# endif
		o = asm_vm_call_method(m, vmargscount, vmargs);
#else
	o = intrp_asm_vm_call_method(m, vmargscount, vmargs);
#endif

	return o;
}
#else
java_objectheader *vm_call_array(methodinfo *m, uint64_t *array)
{
	methoddesc        *md;
	java_objectheader *o;

	md = m->parseddesc;

	/* compile the method if not already done */

	if (m->code == NULL)
		if (!jit_compile(m))
			return NULL;

	STATISTICS(count_calls_native_to_java++);

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (opt_intrp)
		o = intrp_asm_vm_call_method(m, vmargscount, vmargs);
	else
# endif
		o = asm_vm_call_method(m->code->entrypoint, array, md->memuse);
#else
	o = intrp_asm_vm_call_method(m, vmargscount, vmargs);
#endif

	return o;
}
#endif


/* vm_call_int_array ***********************************************************

   Calls a Java method with a variable number of arguments, passed via
   an argument array, and returns an integer (int32_t).

*******************************************************************************/

#if !defined(__MIPS__) && !defined(__X86_64__) && !defined(__POWERPC64__)
s4 vm_call_method_int_vmarg(methodinfo *m, s4 vmargscount, vm_arg *vmargs)
{
	s4 i;

	STATISTICS(count_calls_native_to_java++);

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (opt_intrp)
		i = intrp_asm_vm_call_method_int(m, vmargscount, vmargs);
	else
# endif
		i = asm_vm_call_method_int(m, vmargscount, vmargs);
#else
	i = intrp_asm_vm_call_method_int(m, vmargscount, vmargs);
#endif

	return i;
}
#else
int32_t vm_call_int_array(methodinfo *m, uint64_t *array)
{
	methoddesc *md;
	int32_t     i;

	md = m->parseddesc;

	/* compile the method if not already done */

	if (m->code == NULL)
		if (!jit_compile(m))
			return 0;

	STATISTICS(count_calls_native_to_java++);

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (opt_intrp)
		i = intrp_asm_vm_call_method_int(m, vmargscount, vmargs);
	else
# endif
		i = asm_vm_call_method_int(m->code->entrypoint, array, md->memuse);
#else
	i = intrp_asm_vm_call_method_int(m, vmargscount, vmargs);
#endif

	return i;
}
#endif


/* vm_call_method_int **********************************************************

   Calls a Java method with a variable number of arguments and returns
   an integer (s4).

*******************************************************************************/

s4 vm_call_method_int(methodinfo *m, java_objectheader *o, ...)
{
	va_list ap;
	s4      i;

	va_start(ap, o);
	i = vm_call_method_int_valist(m, o, ap);
	va_end(ap);

	return i;
}


/* vm_call_method_int_valist ***************************************************

   Calls a Java method with a variable number of arguments, passed via
   a va_list, and returns an integer (int32_t).

*******************************************************************************/

#if !defined(__MIPS__) && !defined(__X86_64__) && !defined(__POWERPC64__)
s4 vm_call_method_int_valist(methodinfo *m, java_objectheader *o, va_list ap)
{
	s4      vmargscount;
	vm_arg *vmargs;
	s4      i;
	s4      dumpsize;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* get number of Java method arguments */

	vmargscount = m->parseddesc->paramcount;

	/* allocate vm_arg array */

	vmargs = DMNEW(vm_arg, vmargscount);

	/* fill the vm_arg array from a va_list */

	vm_vmargs_from_valist(m, o, vmargs, ap);

	/* call the Java method */

	i = vm_call_method_int_vmarg(m, vmargscount, vmargs);

	/* release dump area */

	dump_release(dumpsize);

	return i;
}
#else
int32_t vm_call_method_int_valist(methodinfo *m, java_objectheader *o, va_list ap)
{
	int32_t   dumpsize;
	uint64_t *array;
	int32_t   i;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* fill the argument array from a va_list */

	array = vm_array_from_valist(m, o, ap);

	/* call the Java method */

	i = vm_call_int_array(m, array);

	/* release dump area */

	dump_release(dumpsize);

	return i;
}
#endif


/* vm_call_method_int_jvalue ***************************************************

   Calls a Java method with a variable number of arguments, passed via
   a jvalue array, and returns an integer (s4).

*******************************************************************************/

#if !defined(__MIPS__) && !defined(__X86_64__) && !defined(__POWERPC64__)
s4 vm_call_method_int_jvalue(methodinfo *m, java_objectheader *o, jvalue *args)
{
	s4      vmargscount;
	vm_arg *vmargs;
	s4      i;
	s4      dumpsize;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* get number of Java method arguments */

	vmargscount = m->parseddesc->paramcount;

	/* allocate vm_arg array */

	vmargs = DMNEW(vm_arg, vmargscount);

	/* fill the vm_arg array from a va_list */

	vm_vmargs_from_jvalue(m, o, vmargs, args);

	/* call the Java method */

	i = vm_call_method_int_vmarg(m, vmargscount, vmargs);

	/* release dump area */

	dump_release(dumpsize);

	return i;
}
#else
int32_t vm_call_method_int_jvalue(methodinfo *m, java_objectheader *o, jvalue *args)
{
	int32_t   dumpsize;
	uint64_t *array;
	int32_t   i;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* fill the argument array from a va_list */

	array = vm_array_from_jvalue(m, o, args);

	/* call the Java method */

	i = vm_call_int_array(m, array);

	/* release dump area */

	dump_release(dumpsize);

	return i;
}
#endif


/* vm_call_long_array **********************************************************

   Calls a Java method with a variable number of arguments, passed via
   an argument array, and returns a long (int64_t).

*******************************************************************************/

#if !defined(__MIPS__) && !defined(__X86_64__) && !defined(__POWERPC64__)
s8 vm_call_method_long_vmarg(methodinfo *m, s4 vmargscount, vm_arg *vmargs)
{
	s8 l;

	STATISTICS(count_calls_native_to_java++);

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (opt_intrp)
		l = intrp_asm_vm_call_method_long(m, vmargscount, vmargs);
	else
# endif
		l = asm_vm_call_method_long(m, vmargscount, vmargs);
#else
	l = intrp_asm_vm_call_method_long(m, vmargscount, vmargs);
#endif

	return l;
}
#else
int64_t vm_call_long_array(methodinfo *m, uint64_t *array)
{
	methoddesc *md;
	int64_t     l;

	md = m->parseddesc;

	/* compile the method if not already done */

	if (m->code == NULL)
		if (!jit_compile(m))
			return 0;

	STATISTICS(count_calls_native_to_java++);

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (opt_intrp)
		l = intrp_asm_vm_call_method_long(m, vmargscount, vmargs);
	else
# endif
		l = asm_vm_call_method_long(m->code->entrypoint, array, md->memuse);
#else
	l = intrp_asm_vm_call_method_long(m, vmargscount, vmargs);
#endif

	return l;
}
#endif


/* vm_call_method_long *********************************************************

   Calls a Java method with a variable number of arguments and returns
   a long (s8).

*******************************************************************************/

s8 vm_call_method_long(methodinfo *m, java_objectheader *o, ...)
{
	va_list ap;
	s8      l;

	va_start(ap, o);
	l = vm_call_method_long_valist(m, o, ap);
	va_end(ap);

	return l;
}


/* vm_call_method_long_valist **************************************************

   Calls a Java method with a variable number of arguments, passed via
   a va_list, and returns a long (s8).

*******************************************************************************/

#if !defined(__MIPS__) && !defined(__X86_64__) && !defined(__POWERPC64__)
s8 vm_call_method_long_valist(methodinfo *m, java_objectheader *o, va_list ap)
{
	s4      vmargscount;
	vm_arg *vmargs;
	s8      l;
	s4      dumpsize;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* get number of Java method arguments */

	vmargscount = m->parseddesc->paramcount;

	/* allocate vm_arg array */

	vmargs = DMNEW(vm_arg, vmargscount);

	/* fill the vm_arg array from a va_list */

	vm_vmargs_from_valist(m, o, vmargs, ap);

	/* call the Java method */

	l = vm_call_method_long_vmarg(m, vmargscount, vmargs);

	/* release dump area */

	dump_release(dumpsize);

	return l;
}
#else
int64_t vm_call_method_long_valist(methodinfo *m, java_objectheader *o, va_list ap)
{
	int32_t   dumpsize;
	uint64_t *array;
	int64_t   l;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* fill the argument array from a va_list */

	array = vm_array_from_valist(m, o, ap);

	/* call the Java method */

	l = vm_call_long_array(m, array);

	/* release dump area */

	dump_release(dumpsize);

	return l;
}
#endif


/* vm_call_method_long_jvalue **************************************************

   Calls a Java method with a variable number of arguments, passed via
   a jvalue array, and returns a long (s8).

*******************************************************************************/

#if !defined(__MIPS__) && !defined(__X86_64__) && !defined(__POWERPC64__)
s8 vm_call_method_long_jvalue(methodinfo *m, java_objectheader *o, jvalue *args)
{
	s4      vmargscount;
	vm_arg *vmargs;
	s8      l;
	s4      dumpsize;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* get number of Java method arguments */

	vmargscount = m->parseddesc->paramcount;

	/* allocate vm_arg array */

	vmargs = DMNEW(vm_arg, vmargscount);

	/* fill the vm_arg array from a va_list */

	vm_vmargs_from_jvalue(m, o, vmargs, args);

	/* call the Java method */

	l = vm_call_method_long_vmarg(m, vmargscount, vmargs);

	/* release dump area */

	dump_release(dumpsize);

	return l;
}
#else
int64_t vm_call_method_long_jvalue(methodinfo *m, java_objectheader *o, jvalue *args)
{
	int32_t   dumpsize;
	uint64_t *array;
	int64_t   l;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* fill the argument array from a va_list */

	array = vm_array_from_jvalue(m, o, args);

	/* call the Java method */

	l = vm_call_long_array(m, array);

	/* release dump area */

	dump_release(dumpsize);

	return l;
}
#endif


/* vm_call_float_array *********************************************************

   Calls a Java method with a variable number of arguments and returns
   an float.

*******************************************************************************/

#if !defined(__MIPS__) && !defined(__X86_64__) && !defined(__POWERPC64__)
float vm_call_method_float_vmarg(methodinfo *m, s4 vmargscount, vm_arg *vmargs)
{
	float f;

	vm_abort("IMPLEMENT ME!");

	STATISTICS(count_calls_native_to_java++);

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (opt_intrp)
		f = intrp_asm_vm_call_method_float(m, vmargscount, vmargs);
	else
# endif
		f = asm_vm_call_method_float(m, vmargscount, vmargs);
#else
	f = intrp_asm_vm_call_method_float(m, vmargscount, vmargs);
#endif

	return f;
}
#else
float vm_call_float_array(methodinfo *m, uint64_t *array)
{
	methoddesc *md;
	float       f;

	md = m->parseddesc;

	/* compile the method if not already done */

	if (m->code == NULL)
		if (!jit_compile(m))
			return 0;

	STATISTICS(count_calls_native_to_java++);

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (opt_intrp)
		f = intrp_asm_vm_call_method_float(m, vmargscount, vmargs);
	else
# endif
		f = asm_vm_call_method_float(m->code->entrypoint, array, md->memuse);
#else
	f = intrp_asm_vm_call_method_float(m, vmargscount, vmargs);
#endif

	return f;
}
#endif


/* vm_call_method_float ********************************************************

   Calls a Java method with a variable number of arguments and returns
   an float.

*******************************************************************************/

float vm_call_method_float(methodinfo *m, java_objectheader *o, ...)
{
	va_list ap;
	float   f;

	va_start(ap, o);
	f = vm_call_method_float_valist(m, o, ap);
	va_end(ap);

	return f;
}


/* vm_call_method_float_valist *************************************************

   Calls a Java method with a variable number of arguments, passed via
   a va_list, and returns a float.

*******************************************************************************/

#if !defined(__MIPS__) && !defined(__X86_64__) && !defined(__POWERPC64__)
float vm_call_method_float_valist(methodinfo *m, java_objectheader *o,
								  va_list ap)
{
	s4      vmargscount;
	vm_arg *vmargs;
	float   f;
	s4      dumpsize;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* get number of Java method arguments */

	vmargscount = m->parseddesc->paramcount;

	/* allocate vm_arg array */

	vmargs = DMNEW(vm_arg, vmargscount);

	/* fill the vm_arg array from a va_list */

	vm_vmargs_from_valist(m, o, vmargs, ap);

	/* call the Java method */

	f = vm_call_method_float_vmarg(m, vmargscount, vmargs);

	/* release dump area */

	dump_release(dumpsize);

	return f;
}
#else
float vm_call_method_float_valist(methodinfo *m, java_objectheader *o, va_list ap)
{
	int32_t   dumpsize;
	uint64_t *array;
	float     f;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* fill the argument array from a va_list */

	array = vm_array_from_valist(m, o, ap);

	/* call the Java method */

	f = vm_call_float_array(m, array);

	/* release dump area */

	dump_release(dumpsize);

	return f;
}
#endif


/* vm_call_method_float_jvalue *************************************************

   Calls a Java method with a variable number of arguments, passed via
   a jvalue array, and returns a float.

*******************************************************************************/

#if !defined(__MIPS__) && !defined(__X86_64__) && !defined(__POWERPC64__)
float vm_call_method_float_jvalue(methodinfo *m, java_objectheader *o,
								  jvalue *args)
{
	s4      vmargscount;
	vm_arg *vmargs;
	float   f;
	s4      dumpsize;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* get number of Java method arguments */

	vmargscount = m->parseddesc->paramcount;

	/* allocate vm_arg array */

	vmargs = DMNEW(vm_arg, vmargscount);

	/* fill the vm_arg array from a va_list */

	vm_vmargs_from_jvalue(m, o, vmargs, args);

	/* call the Java method */

	f = vm_call_method_float_vmarg(m, vmargscount, vmargs);

	/* release dump area */

	dump_release(dumpsize);

	return f;
}
#else
float vm_call_method_float_jvalue(methodinfo *m, java_objectheader *o, jvalue *args)
{
	int32_t   dumpsize;
	uint64_t *array;
	float     f;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* fill the argument array from a va_list */

	array = vm_array_from_jvalue(m, o, args);

	/* call the Java method */

	f = vm_call_float_array(m, array);

	/* release dump area */

	dump_release(dumpsize);

	return f;
}
#endif


/* vm_call_double_array ********************************************************

   Calls a Java method with a variable number of arguments and returns
   a double.

*******************************************************************************/

#if !defined(__MIPS__) && !defined(__X86_64__) && !defined(__POWERPC64__)
double vm_call_method_double_vmarg(methodinfo *m, s4 vmargscount,
								   vm_arg *vmargs)
{
	double d;

	vm_abort("IMPLEMENT ME!");

	STATISTICS(count_calls_native_to_java++);

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (opt_intrp)
		d = intrp_asm_vm_call_method_double(m, vmargscount, vmargs);
	else
# endif
		d = asm_vm_call_method_double(m, vmargscount, vmargs);
#else
	d = intrp_asm_vm_call_method_double(m, vmargscount, vmargs);
#endif

	return d;
}
#else
double vm_call_double_array(methodinfo *m, uint64_t *array)
{
	methoddesc *md;
	double      d;

	md = m->parseddesc;

	/* compile the method if not already done */

	if (m->code == NULL)
		if (!jit_compile(m))
			return 0;

	STATISTICS(count_calls_native_to_java++);

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (opt_intrp)
		d = intrp_asm_vm_call_method_double(m, vmargscount, vmargs);
	else
# endif
		d = asm_vm_call_method_double(m->code->entrypoint, array, md->memuse);
#else
	d = intrp_asm_vm_call_method_double(m, vmargscount, vmargs);
#endif

	return d;
}
#endif


/* vm_call_method_double *******************************************************

   Calls a Java method with a variable number of arguments and returns
   a double.

*******************************************************************************/

double vm_call_method_double(methodinfo *m, java_objectheader *o, ...)
{
	va_list ap;
	double  d;

	va_start(ap, o);
	d = vm_call_method_double_valist(m, o, ap);
	va_end(ap);

	return d;
}


/* vm_call_method_double_valist ************************************************

   Calls a Java method with a variable number of arguments, passed via
   a va_list, and returns a double.

*******************************************************************************/

#if !defined(__MIPS__) && !defined(__X86_64__) && !defined(__POWERPC64__)
double vm_call_method_double_valist(methodinfo *m, java_objectheader *o,
									va_list ap)
{
	s4      vmargscount;
	vm_arg *vmargs;
	double  d;
	s4      dumpsize;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* get number of Java method arguments */

	vmargscount = m->parseddesc->paramcount;

	/* allocate vm_arg array */

	vmargs = DMNEW(vm_arg, vmargscount);

	/* fill the vm_arg array from a va_list */

	vm_vmargs_from_valist(m, o, vmargs, ap);

	/* call the Java method */

	d = vm_call_method_double_vmarg(m, vmargscount, vmargs);

	/* release dump area */

	dump_release(dumpsize);

	return d;
}
#else
double vm_call_method_double_valist(methodinfo *m, java_objectheader *o, va_list ap)
{
	int32_t   dumpsize;
	uint64_t *array;
	double    d;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* fill the argument array from a va_list */

	array = vm_array_from_valist(m, o, ap);

	/* call the Java method */

	d = vm_call_double_array(m, array);

	/* release dump area */

	dump_release(dumpsize);

	return d;
}
#endif


/* vm_call_method_double_jvalue ************************************************

   Calls a Java method with a variable number of arguments, passed via
   a jvalue array, and returns a double.

*******************************************************************************/

#if !defined(__MIPS__) && !defined(__X86_64__) && !defined(__POWERPC64__)
double vm_call_method_double_jvalue(methodinfo *m, java_objectheader *o,
									jvalue *args)
{
	s4      vmargscount;
	vm_arg *vmargs;
	double  d;
	s4      dumpsize;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* get number of Java method arguments */

	vmargscount = m->parseddesc->paramcount;

	/* allocate vm_arg array */

	vmargs = DMNEW(vm_arg, vmargscount);

	/* fill the vm_arg array from a va_list */

	vm_vmargs_from_jvalue(m, o, vmargs, args);

	/* call the Java method */

	d = vm_call_method_double_vmarg(m, vmargscount, vmargs);

	/* release dump area */

	dump_release(dumpsize);

	return d;
}
#else
double vm_call_method_double_jvalue(methodinfo *m, java_objectheader *o, jvalue *args)
{
	int32_t   dumpsize;
	uint64_t *array;
	double    d;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* fill the argument array from a va_list */

	array = vm_array_from_jvalue(m, o, args);

	/* call the Java method */

	d = vm_call_double_array(m, array);

	/* release dump area */

	dump_release(dumpsize);

	return d;
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
 */
