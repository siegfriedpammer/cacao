/* src/vm/vm.c - VM startup and shutdown functions

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

   Authors: Christian Thalinger

   Changes: Martin Platter

   $Id: vm.c 4357 2006-01-22 23:33:38Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#if defined(WITH_JRE_LAYOUT)
# include <libgen.h>
# include <unistd.h>
#endif

#include "vm/types.h"

#include "mm/boehm.h"
#include "mm/memory.h"
#include "native/jni.h"
#include "native/native.h"

#if defined(ENABLE_THREADS)
# include "threads/native/threads.h"
#endif

#include "vm/classcache.h"
#include "vm/exceptions.h"
#include "vm/finalizer.h"
#include "vm/global.h"
#include "vm/initialize.h"
#include "vm/options.h"
#include "vm/properties.h"
#include "vm/signallocal.h"
#include "vm/stringlocal.h"
#include "vm/suck.h"
#include "vm/vm.h"
#include "vm/jit/jit.h"
#include "vm/jit/asmpart.h"

#include "vm/jit/recompile.h"

#include "vm/jit/profile/profile.h"
#include "vm/rt-timing.h"

#if defined(ENABLE_JVMTI)
#include "native/jvmti/cacaodbg.h"
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

	OPT_ESA,
	OPT_DSA,

	/* Java non-standard options */

	OPT_JIT,
	OPT_INTRP,

	OPT_BOOTCLASSPATH,
	OPT_BOOTCLASSPATH_A,
	OPT_BOOTCLASSPATH_P,

	OPT_GLIBJ,

	OPT_PROF,
	OPT_PROF_OPTION,

	OPT_MS,
	OPT_MX,

	/* CACAO options */

	OPT_VERBOSE1,
	OPT_NOIEEE,
	OPT_SOFTNULL,

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
#endif

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
	{ "softnull",          false, OPT_SOFTNULL },
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
	{ "Xglibj:",           true,  OPT_GLIBJ },

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
	{ "Xprof:",            true,  OPT_PROF_OPTION },
	{ "Xprof",             false, OPT_PROF },

	/* keep these at the end of the list */

#if defined(ENABLE_INLINING)
	{ "i",                 true,  OPT_INLINING },
#endif

#if !defined(NDEBUG)
	{ "m",                 true,  OPT_METHOD },
#endif

	{ "s",                 true,  OPT_SHOW },
	{ "debug-color",      false,  OPT_DEBUGCOLOR },

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

	puts("Java options:");
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
	puts("    -esa | -enablesystemassertions");
	puts("                             enable system assertions");
	puts("    -dsa | -disablesystemassertions");
	puts("                             disable system assertions");
	puts("");

#ifdef ENABLE_JVMTI
	puts("    -agentlib:<agent-lib-name>=<options>  library to load containg JVMTI agent");
	puts ("                                         for jdwp help use: -agentlib:jdwp=help");
	puts("    -agentpath:<path-to-agent>=<options>  path to library containg JVMTI agent");
#endif

	puts("CACAO options:");
	puts("    -v                       write state-information");
	puts("    -verbose[:call|exception|jit]");
	puts("                             enable specific verbose output");
	puts("    -debug-color             colored output for ANSI terms");
#ifdef TYPECHECK_VERBOSE
	puts("    -verbosetc               write debug messages while typechecking");
#endif
#if defined(__ALPHA__)
	puts("    -noieee                  don't use ieee compliant arithmetic");
#endif
#if defined(ENABLE_VERIFIER)
	puts("    -noverify                don't verify classfiles");
#endif
	puts("    -softnull                use software nullpointer check");
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

	puts("    -s(how)...               show...");
	puts("           c(onstants)       the constant pool");
	puts("           m(ethods)         class fields and methods");
	puts("           u(tf)             the utf - hash");
	puts("           i(ntermediate)    intermediate representation");
#if defined(ENABLE_DISASSEMBLER)
	puts("           a(ssembler)       disassembled listing");
	puts("           e(xceptionstubs)  disassembled exception stubs (only with -sa)");
	puts("           n(ative)          disassembled native stubs");
#endif
	puts("           d(atasegment)     data segment listing");
#if defined(ENABLE_INLINING)
	puts("    -i     n(line)           activate inlining");
	puts("           v(irtual)         inline virtual methods (uses/turns rt option on)");
	puts("           e(exception)      inline methods with exceptions");
	puts("           p(aramopt)        optimize argument renaming");
	puts("           o(utsiders)       inline methods of foreign classes");
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
	puts("    -Xglibj:<zip/jar files and directories separated by :>");
	puts("                             value is used as Java core library, but the");
	puts("                             hardcoded VM interface classes are prepended");
	printf("    -Xms<size>               set the initial size of the heap (default: %dMB)\n", HEAP_STARTSIZE / 1024 / 1024);
	printf("    -Xmx<size>               set the maximum size of the heap (default: %dMB)\n", HEAP_MAXSIZE / 1024 / 1024);
	printf("    -Xss<size>               set the thread stack size (default: %dkB)\n", STACK_SIZE / 1024);
	puts("    -Xprof[:bb]              collect and print profiling data");
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


/* version *********************************************************************

   Only prints cacao version information.

*******************************************************************************/

static void version(bool opt_exit)
{
	puts("java version \""JAVA_VERSION"\"");
	puts("CACAO version "VERSION"");

	puts("Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,");
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
	puts("  java.boot.class.path           : "CACAO_VM_ZIP":"CLASSPATH_GLIBJ_ZIP"");
	puts("  gnu.classpath.boot.library.path: "CLASSPATH_LIBDIR"/classpath\n");

	puts("Runtime variables:\n");
	printf("  maximum heap size              : %d\n", opt_heapmaxsize);
	printf("  initial heap size              : %d\n", opt_heapstartsize);
	printf("  stack size                     : %d\n", opt_stacksize);
	printf("  java.boot.class.path           : %s\n", bootclasspath);
	printf("  gnu.classpath.boot.library.path: %s\n", classpath_libdir);

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


/* vm_create *******************************************************************

   Creates a JVM.  Called by JNI_CreateJavaVM.

*******************************************************************************/

bool vm_create(JavaVMInitArgs *vm_args)
{
	char *cp;
	s4    len;
	s4    opt;
	s4    i, j, k;
	bool  opt_version;
	bool  opt_exit;

#if defined(ENABLE_JVMTI)
	lt_dlhandle  handle;
	char *libname, *agentarg;
	bool jdwp,agentbypath;
	jdwp = agentbypath = false;
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

	/* set the VM starttime */

	_Jv_jvm->starttime = builtin_currenttimemillis();

	/* get stuff from the environment *****************************************/

#if defined(DISABLE_GC)
	nogc_init(HEAP_MAXSIZE, HEAP_STARTSIZE);
#endif

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
		bootclasspath = MNEW(char, strlen(cp) + strlen("0"));
		strcpy(bootclasspath, cp);
	}
	else {
#if defined(WITH_JRE_LAYOUT)
		len =
			strlen(cacao_prefix) +
			strlen("/share/cacao/vm.zip") +
			strlen(":") +
			strlen(cacao_prefix) +
			strlen("/share/classpath/glibj.zip") +
			strlen("0");

		bootclasspath = MNEW(char, len);
		strcat(bootclasspath, cacao_prefix);
		strcat(bootclasspath, "/share/cacao/vm.zip");
		strcat(bootclasspath, ":");
		strcat(bootclasspath, cacao_prefix);
		strcat(bootclasspath, "/share/classpath/glibj.zip");
#else
		len = strlen(CACAO_VM_ZIP) +
			strlen(":") +
			strlen(CLASSPATH_GLIBJ_ZIP) +
			strlen("0");

		bootclasspath = MNEW(char, len);
		strcat(bootclasspath, CACAO_VM_ZIP);
		strcat(bootclasspath, ":");
		strcat(bootclasspath, CLASSPATH_GLIBJ_ZIP);
#endif
	}

	/* set the classpath */

	cp = getenv("CLASSPATH");

	if (cp != NULL) {
		classpath = MNEW(char, strlen(cp) + strlen("0"));
		strcat(classpath, cp);
	}
	else {
		classpath = MNEW(char, strlen(".") + strlen("0"));
		strcpy(classpath, ".");
	}


	/* interpret the options **************************************************/

	opt_version       = false;
	opt_exit          = false;

	checknull         = false;
	opt_noieee        = false;

	opt_heapmaxsize   = HEAP_MAXSIZE;
	opt_heapstartsize = HEAP_STARTSIZE;
	opt_stacksize     = STACK_SIZE;


#if defined(ENABLE_JVMTI)
	/* initialize JVMTI related  **********************************************/
	jvmti = false;
#endif

	/* initialize properties before commandline handling */

	if (!properties_init())
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Unable to init properties");

	/* add some default properties */

	properties_add("java.endorsed.dirs", ""CACAO_PREFIX"/jre/lib/endorsed");

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
			MFREE(classpath, char, strlen(classpath));

			classpath = MNEW(char, strlen(opt_arg) + strlen("0"));
			strcpy(classpath, opt_arg);
			break;

		case OPT_D:
			for (j = 0; j < strlen(opt_arg); j++) {
				if (opt_arg[j] == '=') {
					opt_arg[j] = '\0';
					properties_add(opt_arg, opt_arg + j + 1);
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

			MFREE(bootclasspath, char, strlen(bootclasspath));

			bootclasspath = MNEW(char, strlen(opt_arg) + strlen("0"));
			strcpy(bootclasspath, opt_arg);
			break;

		case OPT_BOOTCLASSPATH_A:
			/* append to end of bootclasspath */

			len = strlen(bootclasspath);

			bootclasspath = MREALLOC(bootclasspath,
									 char,
									 len,
									 len + strlen(":") +
									 strlen(opt_arg) + strlen("0"));

			strcat(bootclasspath, ":");
			strcat(bootclasspath, opt_arg);
			break;

		case OPT_BOOTCLASSPATH_P:
			/* prepend in front of bootclasspath */

			cp = bootclasspath;
			len = strlen(cp);

			bootclasspath = MNEW(char, strlen(opt_arg) + strlen(":") +
								 len + strlen("0"));

			strcpy(bootclasspath, opt_arg);
			strcat(bootclasspath, ":");
			strcat(bootclasspath, cp);

			MFREE(cp, char, len);
			break;

		case OPT_GLIBJ:
			/* use as Java core library, but prepend VM interface classes */

			MFREE(bootclasspath, char, strlen(bootclasspath));

			len = strlen(CACAO_VM_ZIP) +
				strlen(":") +
				strlen(opt_arg) +
				strlen("0");

			bootclasspath = MNEW(char, len);

			strcpy(bootclasspath, CACAO_VM_ZIP);
			strcat(bootclasspath, ":");
			strcat(bootclasspath, opt_arg);
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
			if (strcmp("class", opt_arg) == 0)
				opt_verboseclass = true;

			else if (strcmp("gc", opt_arg) == 0)
				opt_verbosegc = true;

			else if (strcmp("jni", opt_arg) == 0)
				opt_verbosejni = true;

			else if (strcmp("call", opt_arg) == 0)
				opt_verbosecall = true;

			else if (strcmp("jit", opt_arg) == 0) {
				opt_verbose = true;
				loadverbose = true;
				linkverbose = true;
				initverbose = true;
				compileverbose = true;
			}
			else if (strcmp("exception", opt_arg) == 0)
				opt_verboseexception = true;
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

		case OPT_SOFTNULL:
			checknull = true;
			break;

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
			for (j = 0; j < strlen(opt_arg); j++) {
				switch (opt_arg[j]) {
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
			for (j = 0; j < strlen(opt_arg); j++) {		
				switch (opt_arg[j]) {
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
		case OPT_INLINING:
			for (j = 0; j < strlen(opt_arg); j++) {		
				switch (opt_arg[j]) {
				case 'n':
					/* define in options.h; Used in main.c, jit.c
					   & inline.c inlining is currently
					   deactivated */
					break;
				case 'v':
					inlinevirtuals = true;
					break;
				case 'e':
					inlineexceptions = true;
					break;
				case 'p':
					inlineparamopt = true;
					break;
				case 'o':
					inlineoutsiders = true;
					break;
				default:
					usage();
				}
			}
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

		case OPT_ESA:
			_Jv_jvm->Java_java_lang_VMClassLoader_defaultAssertionStatus = true;
			break;

		case OPT_DSA:
			_Jv_jvm->Java_java_lang_VMClassLoader_defaultAssertionStatus = false;
			break;

		case OPT_PROF_OPTION:
			/* use <= to get the last \0 too */

			for (j = 0, k = 0; j <= strlen(opt_arg); j++) {
				if (opt_arg[j] == ',')
					opt_arg[j] = '\0';

				if (opt_arg[j] == '\0') {
					if (strcmp("bb", opt_arg + k) == 0)
						opt_prof_bb = true;

					else {
						printf("Unknown option: -Xprof:%s\n", opt_arg + k);
						usage();
					}

					/* set k to next char */

					k = j + 1;
				}
			}
			/* fall through */

		case OPT_PROF:
			opt_prof = true;
			break;

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

		default:
			printf("Unknown option: %s\n",
				   vm_args->options[opt_index].optionString);
			usage();
		}
	}


	/* Now we have all options handled and we can print the version
	   information. */

	if (opt_version)
		version(opt_exit);


	/* get the main class *****************************************************/

	if (opt_index < vm_args->nOptions) {
		mainstring = vm_args->options[opt_index++].optionString;

		/* Put the jar file into the classpath (if any). */

		if (opt_jar == true) {
			/* free old classpath */

			MFREE(classpath, char, strlen(classpath));

			/* put jarfile into classpath */

			classpath = MNEW(char, strlen(mainstring) + strlen("0"));

			strcpy(classpath, mainstring);
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

	/* initialize the garbage collector */

	gc_init(opt_heapmaxsize, opt_heapstartsize);

#if defined(ENABLE_INTRP)
	/* Allocate main thread stack on the Java heap. */

	if (opt_intrp) {
		intrp_main_stack = GCMNEW(u1, opt_stacksize);
		MSET(intrp_main_stack, 0, u1, opt_stacksize);
	}
#endif

#if defined(ENABLE_THREADS)
  	threads_preinit();
#endif

	/* initialize the string hashtable stuff: lock (must be done
	   _after_ threads_preinit) */

	if (!string_init())
		throw_main_exception_exit();

	/* initialize the utf8 hashtable stuff: lock, often used utf8
	   strings (must be done _after_ threads_preinit) */

	if (!utf8_init())
		throw_main_exception_exit();

	/* initialize the classcache hashtable stuff: lock, hashtable
	   (must be done _after_ threads_preinit) */

	if (!classcache_init())
		throw_main_exception_exit();

	/* initialize the loader with bootclasspath (must be done _after_
	   thread_preinit) */

	if (!suck_init())
		throw_main_exception_exit();

	suck_add_from_property("java.endorsed.dirs");
	suck_add(bootclasspath);

	/* initialize the memory subsystem (must be done _after_
	   threads_preinit) */

	if (!memory_init())
		throw_main_exception_exit();

	/* initialize the finalizer stuff (must be done _after_
	   threads_preinit) */

	if (!finalizer_init())
		throw_main_exception_exit();

	/* install architecture dependent signal handler used for exceptions */

	signal_init();

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
		throw_main_exception_exit();

	if (!linker_init())
		throw_main_exception_exit();

	if (!native_init())
		throw_main_exception_exit();

	if (!exceptions_init())
		throw_main_exception_exit();

	if (!builtin_init())
		throw_main_exception_exit();

	/* Initialize the JNI subsystem (must be done _before_
	   threads_init, as threads_init can call JNI methods
	   (e.g. NewGlobalRef). */

	if (!jni_init())
		throw_main_exception_exit();

#if defined(ENABLE_THREADS)
  	if (!threads_init())
		throw_main_exception_exit();
#endif

	/* That's important, otherwise we get into trouble, if the Runtime
	   static initializer is called before (circular dependency. This
	   is with classpath 0.09. Another important thing is, that this
	   has to happen after initThreads!!! */

	if (!initialize_class(class_java_lang_System))
		throw_main_exception_exit();

#if defined(ENABLE_PROFILING)
	/* initialize profiling */

	if (!profile_init())
		throw_main_exception_exit();
#endif

#if defined(ENABLE_THREADS)
	/* initialize recompilation */

	if (!recompile_init())
		throw_main_exception_exit();
		
	/* finally, start the finalizer thread */

	if (!finalizer_start_thread())
		throw_main_exception_exit();

	/* start the recompilation thread (must be done before the
	   profiling thread) */

	if (!recompile_start_thread())
		throw_main_exception_exit();

# if defined(ENABLE_PROFILING)
	/* start the profile sampling thread */

	if (opt_prof)
		if (!profile_start_thread())
			throw_main_exception_exit();
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
	utf              *mainutf;
	classinfo        *mainclass;
	methodinfo       *m;
	java_objectarray *oa; 
	s4                oalength;
	utf              *u;
	java_lang_String *s;
	s4                status;
	s4                i;

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

	if (opt_jar == true)
		/* open jar file with java.util.jar.JarFile */
		mainstring = vm_get_mainclass_from_jar(mainstring);

	/* load the main class */

	mainutf = utf_new_char(mainstring);

	if (!(mainclass = load_class_from_sysloader(mainutf)))
		throw_main_exception_exit();

	/* error loading class, clear exceptionptr for new exception */

	if (*exceptionptr || !mainclass) {
		/*  			*exceptionptr = NULL; */

		/*  			*exceptionptr = */
		/*  				new_exception_message(string_java_lang_NoClassDefFoundError, */
		/*  									  mainstring); */
		throw_main_exception_exit();
	}

	if (!link_class(mainclass))
		throw_main_exception_exit();
			
	/* find the `main' method of the main class */

	m = class_resolveclassmethod(mainclass,
								 utf_new_char("main"), 
								 utf_new_char("([Ljava/lang/String;)V"),
								 class_java_lang_Object,
								 false);

	if (*exceptionptr) {
		throw_main_exception_exit();
	}

	/* there is no main method or it isn't static */

	if ((m == NULL) || !(m->flags & ACC_STATIC)) {
		*exceptionptr = NULL;

		*exceptionptr =
			new_exception_message(string_java_lang_NoSuchMethodError, "main");
		throw_main_exception_exit();
	}

	/* build argument array */

	oalength = vm_args->nOptions - opt_index;

	oa = builtin_anewarray(oalength, class_java_lang_String);

	for (i = 0; i < oalength; i++) {
		u = utf_new_char(vm_args->options[opt_index + i].optionString);
		s = javastring_new(u);

		oa->data[i] = (java_objectheader *) s;
	}

#ifdef TYPEINFO_DEBUG_TEST
	/* test the typeinfo system */
	typeinfo_test();
#endif
	/*class_showmethods(currentThread->group->header.vftbl->class);	*/

#if defined(ENABLE_JVMTI)
	jvmti_set_phase(JVMTI_PHASE_LIVE);
#endif

	/* increase total started thread count */

	_Jv_jvm->total_started_thread_count++;

	/* start the main thread */

	(void) vm_call_method(m, NULL, oa);

	/* exception occurred? */

	if (*exceptionptr) {
		throw_main_exception();
		status = 1;
	}

	/* unload the JavaVM */

	vm_destroy(vm);

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

	if (!link_class(class_java_lang_System))
		throw_main_exception_exit();

	/* call java.lang.System.exit(I)V */

	m = class_resolveclassmethod(class_java_lang_System,
								 utf_new_char("exit"),
								 utf_int__void,
								 class_java_lang_Object,
								 true);
	
	if (m == NULL)
		throw_main_exception_exit();

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

		mem_usagelog(1);

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
	java_lang_String  *s;

	c = load_class_from_sysloader(utf_new_char("java/util/jar/JarFile"));

	if (c == NULL)
		throw_main_exception_exit();
	
	/* create JarFile object */

	o = builtin_new(c);

	if (o == NULL)
		throw_main_exception_exit();


	m = class_resolveclassmethod(c,
								 utf_init, 
								 utf_java_lang_String__void,
								 class_java_lang_Object,
								 true);

	if (m == NULL)
		throw_main_exception_exit();

	s = javastring_new_from_ascii(mainstring);

	(void) vm_call_method(m, o, s);

	if (*exceptionptr)
		throw_main_exception_exit();

	/* get manifest object */

	m = class_resolveclassmethod(c,
								 utf_new_char("getManifest"), 
								 utf_new_char("()Ljava/util/jar/Manifest;"),
								 class_java_lang_Object,
								 true);

	if (m == NULL)
		throw_main_exception_exit();

	o = vm_call_method(m, o);

	if (o == NULL) {
		fprintf(stderr, "Could not get manifest from %s (invalid or corrupt jarfile?)\n", mainstring);
		vm_exit(1);
	}


	/* get Main Attributes */

	m = class_resolveclassmethod(o->vftbl->class,
								 utf_new_char("getMainAttributes"), 
								 utf_new_char("()Ljava/util/jar/Attributes;"),
								 class_java_lang_Object,
								 true);

	if (m == NULL)
		throw_main_exception_exit();

	o = vm_call_method(m, o);

	if (o == NULL) {
		fprintf(stderr, "Could not get main attributes from %s (invalid or corrupt jarfile?)\n", mainstring);
		vm_exit(1);
	}


	/* get property Main-Class */

	m = class_resolveclassmethod(o->vftbl->class,
								 utf_new_char("getValue"), 
								 utf_new_char("(Ljava/lang/String;)Ljava/lang/String;"),
								 class_java_lang_Object,
								 true);

	if (m == NULL)
		throw_main_exception_exit();

	s = javastring_new_from_ascii("Main-Class");

	o = vm_call_method(m, o, s);

	if (o == NULL)
		throw_main_exception_exit();

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

						exceptions_print_exception(*exceptionptr);

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

							exceptions_print_exception(*exceptionptr);
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

	if (!(mainclass = load_class_bootstrap(utf_new_char(mainstring))))
		throw_main_exception_exit();

	if (!link_class(mainclass))
		throw_main_exception_exit();

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

	if (m == NULL) {
		char message[MAXLOGTEXT];
		sprintf(message, "%s%s", opt_method,
				opt_signature ? opt_signature : "");

		*exceptionptr =
			new_exception_message(string_java_lang_NoSuchMethodException,
								  message);
										 
		throw_main_exception_exit();
	}
		
	jit_compile(m);
}
#endif /* !defined(NDEBUG) */


/* vm_vmargs_from_valist *******************************************************

   XXX

*******************************************************************************/

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
		switch (paramtypes->decltype) {
		/* primitive types */
		case PRIMITIVETYPE_BOOLEAN: 
		case PRIMITIVETYPE_BYTE:
		case PRIMITIVETYPE_CHAR:
		case PRIMITIVETYPE_SHORT: 
		case PRIMITIVETYPE_INT:
			vmargs[i].type   = TYPE_INT;
			vmargs[i].data.l = (s8) va_arg(ap, s4);
			break;

		case PRIMITIVETYPE_LONG:
			vmargs[i].type   = TYPE_LNG;
			vmargs[i].data.l = (s8) va_arg(ap, s8);
			break;

		case PRIMITIVETYPE_FLOAT:
			vmargs[i].type   = TYPE_FLT;
#if defined(__ALPHA__)
			/* this keeps the assembler function much simpler */

			vmargs[i].data.d = (jdouble) va_arg(ap, jdouble);
#else
			vmargs[i].data.f = (jfloat) va_arg(ap, jdouble);
#endif
			break;

		case PRIMITIVETYPE_DOUBLE:
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


/* vm_vmargs_from_jvalue *******************************************************

   XXX

*******************************************************************************/

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
		/* primitive types */
		case PRIMITIVETYPE_BOOLEAN: 
		case PRIMITIVETYPE_BYTE:
		case PRIMITIVETYPE_CHAR:
		case PRIMITIVETYPE_SHORT: 
		case PRIMITIVETYPE_INT:
			vmargs[i].type   = TYPE_INT;
			vmargs[i].data.l = (s8) args[j].i;
			break;

		case PRIMITIVETYPE_LONG:
			vmargs[i].type   = TYPE_LNG;
			vmargs[i].data.l = (s8) args[j].j;
			break;

		case PRIMITIVETYPE_FLOAT:
			vmargs[i].type = TYPE_FLT;
#if defined(__ALPHA__)
			/* this keeps the assembler function much simpler */

			vmargs[i].data.d = (jdouble) args[j].f;
#else
			vmargs[i].data.f = args[j].f;
#endif
			break;

		case PRIMITIVETYPE_DOUBLE:
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
}


/* vm_call_method_jvalue *******************************************************

   Calls a Java method with a variable number of arguments, passed via
   a jvalue array, and returns an address.

*******************************************************************************/

java_objectheader *vm_call_method_jvalue(methodinfo *m, java_objectheader *o,
										 jvalue *args)
{
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
}


/* vm_call_method_vmarg ********************************************************

   Calls a Java method with a variable number of arguments, passed via
   a vm_arg array, and returns an address.

*******************************************************************************/

java_objectheader *vm_call_method_vmarg(methodinfo *m, s4 vmargscount,
										vm_arg *vmargs)
{
	java_objectheader *o;

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
   a va_list, and returns an integer (s4).

*******************************************************************************/

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


/* vm_call_method_int_jvalue ***************************************************

   Calls a Java method with a variable number of arguments, passed via
   a jvalue array, and returns an integer (s4).

*******************************************************************************/

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


/* vm_call_method_int_vmarg ****************************************************

   Calls a Java method with a variable number of arguments, passed via
   a vm_arg array, and returns an integer (s4).

*******************************************************************************/

s4 vm_call_method_int_vmarg(methodinfo *m, s4 vmargscount, vm_arg *vmargs)
{
	s4 i;

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


/* vm_call_method_long_jvalue **************************************************

   Calls a Java method with a variable number of arguments, passed via
   a jvalue array, and returns a long (s8).

*******************************************************************************/

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


/* vm_call_method_long_vmarg ***************************************************

   Calls a Java method with a variable number of arguments, passed via
   a vm_arg array, and returns a long (s8).

*******************************************************************************/

s8 vm_call_method_long_vmarg(methodinfo *m, s4 vmargscount, vm_arg *vmargs)
{
	s8 l;

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


/* vm_call_method_float_jvalue *************************************************

   Calls a Java method with a variable number of arguments, passed via
   a jvalue array, and returns a float.

*******************************************************************************/

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


/* vm_call_method_float_vmarg **************************************************

   Calls a Java method with a variable number of arguments and returns
   an float.

*******************************************************************************/

float vm_call_method_float_vmarg(methodinfo *m, s4 vmargscount, vm_arg *vmargs)
{
	float f;

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


/* vm_call_method_double_jvalue ************************************************

   Calls a Java method with a variable number of arguments, passed via
   a jvalue array, and returns a double.

*******************************************************************************/

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


/* vm_call_method_double_vmarg *************************************************

   Calls a Java method with a variable number of arguments and returns
   a double.

*******************************************************************************/

double vm_call_method_double_vmarg(methodinfo *m, s4 vmargscount,
								   vm_arg *vmargs)
{
	double d;

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
