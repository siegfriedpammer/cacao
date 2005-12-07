/* src/cacao/cacao.c - contains main() of cacao

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Reinhard Grafl

   Changes: Andi Krall
            Mark Probst
            Philipp Tomsich
            Christian Thalinger

   This module does the following tasks:
     - Command line option handling
     - Calling initialization routines
     - Calling the class loader
     - Running the main method

   $Id: cacao.c 3904 2005-12-07 17:44:37Z twisti $

*/


#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "cacao/cacao.h"
#include "mm/boehm.h"
#include "mm/memory.h"
#include "native/jni.h"
#include "native/native.h"

#if defined(ENABLE_JVMTI)
#include "native/jvmti/jvmti.h"
#include "native/jvmti/dbg.h"
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#endif

#include "toolbox/logging.h"
#include "vm/classcache.h"
#include "vm/exceptions.h"
#include "vm/finalizer.h"
#include "vm/global.h"
#include "vm/initialize.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/signallocal.h"
#include "vm/statistics.h"
#include "vm/stringlocal.h"
#include "vm/suck.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/jit.h"

#ifdef TYPEINFO_DEBUG_TEST
#include "vm/jit/verify/typeinfo.h"
#endif


/* define heap sizes **********************************************************/

#define HEAP_MAXSIZE      64 * 1024 * 1024  /* default 64MB                   */
#define HEAP_STARTSIZE    2 * 1024 * 1024   /* default 2MB                    */
#define STACK_SIZE        128 * 1024        /* default 128kB                  */

#if defined(ENABLE_INTRP)
u1 *intrp_main_stack;
#endif


/* CACAO related stuff ********************************************************/

bool cacao_initializing;
bool cacao_exiting;


/* Invocation API variables ***************************************************/

JavaVM *jvm;                        /* denotes a Java VM                      */
JNIEnv *env;                        /* pointer to native method interface     */
 
JDK1_1InitArgs vm_args;             /* JDK 1.1 VM initialization arguments    */


char *bootclasspath;                    /* contains the boot classpath        */
char *classpath;                        /* contains the classpath             */

char *mainstring;
static classinfo *mainclass;

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
void **stackbottom = 0;
#endif


/* define command line options ************************************************/

#define OPT_CLASSPATH        2
#define OPT_D                3
#define OPT_MS               4
#define OPT_MX               5
#define OPT_VERBOSE1         6
#define OPT_VERBOSE          7
#define OPT_VERBOSESPECIFIC  8
#define OPT_VERBOSECALL      9
#define OPT_NOIEEE           10
#define OPT_SOFTNULL         11
#define OPT_TIME             12

#if defined(STATISTICS)
#define OPT_STAT             13
#endif /* defined(STATISTICS) */

#define OPT_LOG              14
#define OPT_CHECK            15
#define OPT_LOAD             16
#define OPT_METHOD           17
#define OPT_SIGNATURE        18
#define OPT_SHOW             19
#define OPT_ALL              20
#define OPT_OLOOP            24
#define OPT_INLINING	     25

#define STATIC_ANALYSIS
#if defined(STATIC_ANALYSIS)
# define OPT_RT              26
# define OPT_XTA             27 
# define OPT_VTA             28
#endif /* defined(STATIC_ANALYSIS) */

#define OPT_VERBOSETC        29
#define OPT_NOVERIFY         30
#define OPT_LIBERALUTF       31
#define OPT_VERBOSEEXCEPTION 32
#define OPT_EAGER            33

#if defined(LSRA)
#define OPT_LSRA             34
#endif /* defined(LSRA) */

#define OPT_JAR              35
#define OPT_BOOTCLASSPATH    36
#define OPT_BOOTCLASSPATH_A  37
#define OPT_BOOTCLASSPATH_P  38
#define OPT_VERSION          39
#define OPT_SHOWVERSION      40
#define OPT_FULLVERSION      41

#define OPT_HELP             100
#define OPT_X                101

#define OPT_JIT              102
#define OPT_INTRP            103

#define OPT_STATIC_SUPERS    104
#define OPT_TRACE            105

#define OPT_SS               106

#ifdef ENABLE_JVMTI
#define OPT_DEBUG            107
#define OPT_AGENTLIB         108
#define OPT_AGENTPATH        109
#endif 

#define OPT_NO_DYNAMIC       110

opt_struct opts[] = {
	{ "classpath",         true,  OPT_CLASSPATH },
	{ "cp",                true,  OPT_CLASSPATH },
	{ "D",                 true,  OPT_D },
	{ "noasyncgc",         false, OPT_IGNORE },
	{ "noverify",          false, OPT_NOVERIFY },
	{ "liberalutf",        false, OPT_LIBERALUTF },
	{ "v",                 false, OPT_VERBOSE1 },
	{ "verbose",           false, OPT_VERBOSE },
	{ "verbose:",          true,  OPT_VERBOSESPECIFIC },
	{ "verbosecall",       false, OPT_VERBOSECALL },
	{ "verboseexception",  false, OPT_VERBOSEEXCEPTION },
#ifdef TYPECHECK_VERBOSE
	{ "verbosetc",         false, OPT_VERBOSETC },
#endif
#if defined(__ALPHA__)
	{ "noieee",            false, OPT_NOIEEE },
#endif
	{ "softnull",          false, OPT_SOFTNULL },
	{ "time",              false, OPT_TIME },
#if defined(STATISTICS)
	{ "stat",              false, OPT_STAT },
#endif
	{ "log",               true,  OPT_LOG },
	{ "c",                 true,  OPT_CHECK },
	{ "l",                 false, OPT_LOAD },
	{ "eager",             false, OPT_EAGER },
	{ "sig",               true,  OPT_SIGNATURE },
	{ "all",               false, OPT_ALL },
	{ "oloop",             false, OPT_OLOOP },
#ifdef STATIC_ANALYSIS
	{ "rt",                false, OPT_RT },
	{ "xta",               false, OPT_XTA },
	{ "vta",               false, OPT_VTA },
#endif
#ifdef LSRA
	{ "lsra",              false, OPT_LSRA },
#endif
	{ "jar",               false, OPT_JAR },
	{ "version",           false, OPT_VERSION },
	{ "showversion",       false, OPT_SHOWVERSION },
	{ "fullversion",       false, OPT_FULLVERSION },
	{ "help",              false, OPT_HELP },
	{ "?",                 false, OPT_HELP },

	/* interpreter options */

	{ "trace",             false, OPT_TRACE },
	{ "static-supers",     true,  OPT_STATIC_SUPERS },
	{ "no-dynamic",        false, OPT_NO_DYNAMIC },

	/* JVMTI Agent Command Line Options */
#ifdef ENABLE_JVMTI
	{ "agentlib:",         true,  OPT_AGENTLIB },
	{ "agentpath:",        true,  OPT_AGENTPATH },
#endif

	/* X options */

	{ "X",                 false, OPT_X },
	{ "Xjit",              false, OPT_JIT },
	{ "Xint",              false, OPT_INTRP },
	{ "Xbootclasspath:",   true,  OPT_BOOTCLASSPATH },
	{ "Xbootclasspath/a:", true,  OPT_BOOTCLASSPATH_A },
	{ "Xbootclasspath/p:", true,  OPT_BOOTCLASSPATH_P },
#ifdef ENABLE_JVMTI
	{ "Xdebug",            false, OPT_DEBUG },
#endif 
	{ "Xms",               true,  OPT_MS },
	{ "Xmx",               true,  OPT_MX },
	{ "Xss",               true,  OPT_SS },
	{ "ms",                true,  OPT_MS },
	{ "mx",                true,  OPT_MX },
	{ "ss",                true,  OPT_SS },

	/* keep these at the end of the list */

	{ "i",                 true,  OPT_INLINING },
	{ "m",                 true,  OPT_METHOD },
	{ "s",                 true,  OPT_SHOW },

	{ NULL,                false, 0 }
};


/* usage ***********************************************************************

   Prints the correct usage syntax to stdout.

*******************************************************************************/

static void usage(void)
{
	printf("Usage: cacao [-options] classname [arguments]\n");
	printf("               (to run a class file)\n");
	printf("       cacao [-options] -jar jarfile [arguments]\n");
	printf("               (to run a standalone jar file)\n\n");

	printf("Java options:\n");
	printf("    -cp <path>               specify a path to look for classes\n");
	printf("    -classpath <path>        specify a path to look for classes\n");
	printf("    -D<name>=<value>         add an entry to the property list\n");
	printf("    -verbose[:class|gc|jni]  enable specific verbose output\n");
	printf("    -version                 print product version and exit\n");
	printf("    -fullversion             print jpackage-compatible product version and exit\n");
	printf("    -showversion             print product version and continue\n");
	printf("    -help, -?                print this help message\n");
	printf("    -X                       print help on non-standard Java options\n\n");

#ifdef ENABLE_JVMTI
	printf("    -agentlib:<agent-lib-name>=<options>  library to load containg JVMTI agent\n");
	printf("    -agentpath:<path-to-agent>=<options>  path to library containg JVMTI agent\n");
#endif

	printf("CACAO options:\n");
	printf("    -v                       write state-information\n");
	printf("    -verbose                 write more information\n");
	printf("    -verbosegc               write message for each GC\n");
	printf("    -verbosecall             write message for each call\n");
	printf("    -verboseexception        write message for each step of stack unwinding\n");
#ifdef TYPECHECK_VERBOSE
	printf("    -verbosetc               write debug messages while typechecking\n");
#endif
#if defined(__ALPHA__)
	printf("    -noieee                  don't use ieee compliant arithmetic\n");
#endif
	printf("    -noverify                don't verify classfiles\n");
	printf("    -liberalutf              don't warn about overlong UTF-8 sequences\n");
	printf("    -softnull                use software nullpointer check\n");
	printf("    -time                    measure the runtime\n");
#if defined(STATISTICS)
	printf("    -stat                    detailed compiler statistics\n");
#endif
	printf("    -log logfile             specify a name for the logfile\n");
	printf("    -c(heck)b(ounds)         don't check array bounds\n");
	printf("            s(ync)           don't check for synchronization\n");
	printf("    -oloop                   optimize array accesses in loops\n"); 
	printf("    -l                       don't start the class after loading\n");
	printf("    -eager                   perform eager class loading and linking\n");
	printf("    -all                     compile all methods, no execution\n");
	printf("    -m                       compile only a specific method\n");
	printf("    -sig                     specify signature for a specific method\n");
	printf("    -s(how)a(ssembler)       show disassembled listing\n");
	printf("           c(onstants)       show the constant pool\n");
	printf("           d(atasegment)     show data segment listing\n");
	printf("           e(xceptionstubs)  show disassembled exception stubs (only with -sa)\n");
	printf("           i(ntermediate)    show intermediate representation\n");
	printf("           m(ethods)         show class fields and methods\n");
	printf("           n(ative)          show disassembled native stubs\n");
	printf("           u(tf)             show the utf - hash\n");
	printf("    -i     n(line)           activate inlining\n");
	printf("           v(irtual)         inline virtual methods (uses/turns rt option on)\n");
	printf("           e(exception)      inline methods with exceptions\n");
	printf("           p(aramopt)        optimize argument renaming\n");
	printf("           o(utsiders)       inline methods of foreign classes\n");
#ifdef STATIC_ANALYSIS
	printf("    -rt                      use rapid type analysis\n");
	printf("    -xta                     use x type analysis\n");
	printf("    -vta                     use variable type analysis\n");
#endif
#ifdef LSRA
	printf("    -lsra                    use linear scan register allocation\n");
#endif

	/* exit with error code */

	exit(1);
}   


static void Xusage(void)
{
#if defined(ENABLE_JIT)
	printf("    -Xjit             JIT mode execution (default)\n");
#endif
#if defined(ENABLE_INTRP)
	printf("    -Xint             interpreter mode execution\n");
#endif
	printf("    -Xbootclasspath:<zip/jar files and directories separated by :>\n");
    printf("                      value is set as bootstrap class path\n");
	printf("    -Xbootclasspath/a:<zip/jar files and directories separated by :>\n");
	printf("                      value is appended to the bootstrap class path\n");
	printf("    -Xbootclasspath/p:<zip/jar files and directories separated by :>\n");
	printf("                      value is prepended to the bootstrap class path\n");
	printf("    -Xms<size>        set the initial size of the heap (default: 2MB)\n");
	printf("    -Xmx<size>        set the maximum size of the heap (default: 64MB)\n");
	printf("    -Xss<size>        set the thread stack size (default: 128kB)\n");
#if defined(ENABLE_JVMTI)
	printf("    -Xdebug<transport> enable remote debugging\n");
#endif 

	/* exit with error code */

	exit(1);
}   


/* version *********************************************************************

   Only prints cacao version information.

*******************************************************************************/

static void version(void)
{
	printf("java version \""JAVA_VERSION"\"\n");
	printf("CACAO version "VERSION"\n");

	printf("Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,\n");
	printf("R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,\n");
	printf("C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,\n");
	printf("Institut f. Computersprachen - TU Wien\n\n");

	printf("This program is free software; you can redistribute it and/or\n");
	printf("modify it under the terms of the GNU General Public License as\n");
	printf("published by the Free Software Foundation; either version 2, or (at\n");
	printf("your option) any later version.\n\n");

	printf("This program is distributed in the hope that it will be useful, but\n");
	printf("WITHOUT ANY WARRANTY; without even the implied warranty of\n");
	printf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n");
	printf("General Public License for more details.\n");
}


/* fullversion *****************************************************************

   Prints a Sun compatible version information (required e.g. by
   jpackage, www.jpackage.org).

*******************************************************************************/

static void fullversion(void)
{
	printf("java full version \"cacao-"JAVA_VERSION"\"\n");

	/* exit normally */

	exit(0);
}


#ifdef TYPECHECK_STATISTICS
void typecheck_print_statistics(FILE *file);
#endif

/* setup_debugger_process *****************************************************

   Helper function to start JDWP threads

*******************************************************************************/
#if defined(ENABLE_JVMTI)

static void setup_debugger_process(char* transport) {
	java_objectheader *o;
	methodinfo *m;
	java_lang_String  *s;

	/* new gnu.classpath.jdwp.Jdwp() */
	mainclass = 
		load_class_from_sysloader(utf_new_char("gnu.classpath.jdwp.Jdwp"));
	if (!mainclass)
		throw_main_exception_exit();

	o = builtin_new(mainclass);

	if (!o)
		throw_main_exception_exit();
	
	m = class_resolveclassmethod(mainclass,
								 utf_init, 
								 utf_java_lang_String__void,
								 class_java_lang_Object,
								 true);
	if (!m)
		throw_main_exception_exit();

	asm_calljavafunction(m, o, NULL, NULL, NULL);

	/* configure(transport,NULL) */
	m = class_resolveclassmethod(
		mainclass, utf_new_char("configure"), 
		utf_new_char("(Ljava/lang/String;Ljava/lang/Thread;)V"),
		class_java_lang_Object,
		false);


	s = javastring_new_char(transport);
	asm_calljavafunction(m, o, s, NULL, NULL);
	if (!m)
		throw_main_exception_exit();

	/* _doInitialization */
	m = class_resolveclassmethod(mainclass,
								 utf_new_char("_doInitialization"), 
								 utf_new_char("()V"),
								 mainclass,
								 false);
	
	if (!m)
		throw_main_exception_exit();

	asm_calljavafunction(m, o, NULL, NULL, NULL);
}
#endif


/* getmainclassfromjar *********************************************************

   Gets the name of the main class form a JAR's manifest file.

*******************************************************************************/

static char *getmainclassnamefromjar(char *mainstring)
{
	classinfo         *c;
	java_objectheader *o;
	methodinfo        *m;
	java_lang_String  *s;

	c = load_class_from_sysloader(utf_new_char("java/util/jar/JarFile"));

	if (!c)
		throw_main_exception_exit();
	
	/* create JarFile object */

	o = builtin_new(c);

	if (!o)
		throw_main_exception_exit();


	m = class_resolveclassmethod(c,
								 utf_init, 
								 utf_java_lang_String__void,
								 class_java_lang_Object,
								 true);

	if (!m)
		throw_main_exception_exit();

	s = javastring_new_char(mainstring);

	asm_calljavafunction(m, o, s, NULL, NULL);

	if (*exceptionptr)
		throw_main_exception_exit();

	/* get manifest object */

	m = class_resolveclassmethod(c,
								 utf_new_char("getManifest"), 
								 utf_new_char("()Ljava/util/jar/Manifest;"),
								 class_java_lang_Object,
								 true);

	if (!m)
		throw_main_exception_exit();

	o = asm_calljavafunction(m, o, NULL, NULL, NULL);

	if (!o) {
		fprintf(stderr, "Could not get manifest from %s (invalid or corrupt jarfile?)\n", mainstring);
		cacao_exit(1);
	}


	/* get Main Attributes */

	m = class_resolveclassmethod(o->vftbl->class,
								 utf_new_char("getMainAttributes"), 
								 utf_new_char("()Ljava/util/jar/Attributes;"),
								 class_java_lang_Object,
								 true);

	if (!m)
		throw_main_exception_exit();

	o = asm_calljavafunction(m, o, NULL, NULL, NULL);

	if (!o) {
		fprintf(stderr, "Could not get main attributes from %s (invalid or corrupt jarfile?)\n", mainstring);
		cacao_exit(1);
	}


	/* get property Main-Class */

	m = class_resolveclassmethod(o->vftbl->class,
								 utf_new_char("getValue"), 
								 utf_new_char("(Ljava/lang/String;)Ljava/lang/String;"),
								 class_java_lang_Object,
								 true);

	if (!m)
		throw_main_exception_exit();

	s = javastring_new_char("Main-Class");

	o = asm_calljavafunction(m, o, s, NULL, NULL);

	if (!o)
		throw_main_exception_exit();

	return javastring_tochar(o);
}


void exit_handler(void);


/* main ************************************************************************

   The main program.
   
*******************************************************************************/

int main(int argc, char **argv)
{
	s4 i, j;
	void *dummy;
	
	/* local variables ********************************************************/
   
	char logfilename[200] = "";
	u4 heapmaxsize;
	u4 heapstartsize;
	char *cp;
	s4    cplen;
	bool startit = true;
	char *specificmethodname = NULL;
	char *specificsignature = NULL;
	bool jar = false;
#if defined(ENABLE_JVMTI)
	bool dbg = false;
	char *transport;
	int waitval;
#endif


#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
	stackbottom = &dummy;
#endif
	
	if (atexit(exit_handler))
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Unable to register exit_handler");

	/* initialize global variables */

	cacao_exiting = false;


	/************ Collect info from the environment ************************/

#if defined(DISABLE_GC)
	nogc_init(HEAP_MAXSIZE, HEAP_STARTSIZE);
#endif

	/* set the bootclasspath */

	cp = getenv("BOOTCLASSPATH");
	if (cp) {
		bootclasspath = MNEW(char, strlen(cp) + strlen("0"));
		strcpy(bootclasspath, cp);

	} else {
		cplen = strlen(CACAO_INSTALL_PREFIX) + strlen(CACAO_VM_ZIP_PATH) +
			strlen(":") +
			strlen(CLASSPATH_INSTALL_DIR) +
			strlen(CLASSPATH_GLIBJ_ZIP_PATH) +
			strlen("0");

		bootclasspath = MNEW(char, cplen);
		strcpy(bootclasspath, CACAO_INSTALL_PREFIX);
		strcat(bootclasspath, CACAO_VM_ZIP_PATH);
		strcat(bootclasspath, ":");
		strcat(bootclasspath, CLASSPATH_INSTALL_DIR);
		strcat(bootclasspath, CLASSPATH_GLIBJ_ZIP_PATH);
	}


	/* set the classpath */

	cp = getenv("CLASSPATH");
	if (cp) {
		classpath = MNEW(char, strlen(cp) + strlen("0"));
		strcat(classpath, cp);

	} else {
		classpath = MNEW(char, strlen(".") + strlen("0"));
		strcpy(classpath, ".");
	}


	/***************** Interpret the command line *****************/
   
	checknull = false;
	opt_noieee = false;

	heapmaxsize = HEAP_MAXSIZE;
	heapstartsize = HEAP_STARTSIZE;
	opt_stacksize = STACK_SIZE;

	while ((i = get_opt(argc, argv, opts)) != OPT_DONE) {
		switch (i) {
		case OPT_IGNORE:
			break;
			
		case OPT_BOOTCLASSPATH:
			/* Forget default bootclasspath and set the argument as new boot  */
			/* classpath.                                                     */
			MFREE(bootclasspath, char, strlen(bootclasspath));

			bootclasspath = MNEW(char, strlen(opt_arg) + strlen("0"));
			strcpy(bootclasspath, opt_arg);
			break;

		case OPT_BOOTCLASSPATH_A:
			/* append to end of bootclasspath */
			cplen = strlen(bootclasspath);

			bootclasspath = MREALLOC(bootclasspath,
									 char,
									 cplen,
									 cplen + strlen(":") +
									 strlen(opt_arg) + strlen("0"));

			strcat(bootclasspath, ":");
			strcat(bootclasspath, opt_arg);
			break;

		case OPT_BOOTCLASSPATH_P:
			/* prepend in front of bootclasspath */
			cp = bootclasspath;
			cplen = strlen(cp);

			bootclasspath = MNEW(char, strlen(opt_arg) + strlen(":") +
								 cplen + strlen("0"));

			strcpy(bootclasspath, opt_arg);
			strcat(bootclasspath, ":");
			strcat(bootclasspath, cp);

			MFREE(cp, char, cplen);
			break;

		case OPT_CLASSPATH:
			/* forget old classpath and set the argument as new classpath */
			MFREE(classpath, char, strlen(classpath));

			classpath = MNEW(char, strlen(opt_arg) + strlen("0"));
			strcpy(classpath, opt_arg);
			break;

		case OPT_JAR:
			jar = true;
			break;

#if defined(ENABLE_JVMTI)
		case OPT_DEBUG:
			dbg = true;
			transport = opt_arg;
			break;

		case OPT_AGENTPATH:
		case OPT_AGENTLIB:
			set_jvmti_phase(JVMTI_PHASE_ONLOAD);
			agentload(opt_arg);
			set_jvmti_phase(JVMTI_PHASE_PRIMORDIAL);
			break;
#endif
			
		case OPT_D:
			{
				for (j = 0; j < strlen(opt_arg); j++) {
					if (opt_arg[j] == '=') {
						opt_arg[j] = '\0';
						create_property(opt_arg, opt_arg + j + 1);
						goto didit;
					}
				}

				/* if no '=' is given, just create an empty property */
				create_property(opt_arg, "");
					
			didit: ;
			}	
			break;

		case OPT_MX:
		case OPT_MS:
		case OPT_SS:
			{
				char c;
				c = opt_arg[strlen(opt_arg) - 1];

				if (c == 'k' || c == 'K') {
					j = 1024 * atoi(opt_arg);

				} else if (c == 'm' || c == 'M') {
					j = 1024 * 1024 * atoi(opt_arg);

				} else
					j = atoi(opt_arg);

				if (i == OPT_MX)
					heapmaxsize = j;
				else if (i == OPT_MS)
					heapstartsize = j;
				else
					opt_stacksize = j;
			}
			break;

		case OPT_VERBOSE1:
			opt_verbose = true;
			break;

		case OPT_VERBOSE:
			opt_verbose = true;
			loadverbose = true;
			linkverbose = true;
			initverbose = true;
			compileverbose = true;
			break;

		case OPT_VERBOSESPECIFIC:
			if (strcmp("class", opt_arg) == 0)
				opt_verboseclass = true;

			else if (strcmp("gc", opt_arg) == 0)
				opt_verbosegc = true;

			else if (strcmp("jni", opt_arg) == 0)
				opt_verbosejni = true;
			break;

		case OPT_VERBOSEEXCEPTION:
			opt_verboseexception = true;
			break;

#ifdef TYPECHECK_VERBOSE
		case OPT_VERBOSETC:
			typecheckverbose = true;
			break;
#endif
				
		case OPT_VERBOSECALL:
			runverbose = true;
			break;

		case OPT_VERSION:
			version();
			exit(0);
			break;

		case OPT_FULLVERSION:
			fullversion();
			break;

		case OPT_SHOWVERSION:
			version();
			break;

		case OPT_NOIEEE:
			opt_noieee = true;
			break;

		case OPT_NOVERIFY:
			opt_verify = false;
			break;

		case OPT_LIBERALUTF:
			opt_liberalutf = true;
			break;

		case OPT_SOFTNULL:
			checknull = true;
			break;

		case OPT_TIME:
			getcompilingtime = true;
			getloadingtime = true;
			break;
					
#if defined(STATISTICS)
		case OPT_STAT:
			opt_stat = true;
			break;
#endif
					
		case OPT_LOG:
			strcpy(logfilename, opt_arg);
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
			startit = false;
			makeinitializations = false;
			break;

		case OPT_EAGER:
			opt_eager = true;
			break;

		case OPT_METHOD:
			startit = false;
			specificmethodname = opt_arg;
			makeinitializations = false;
			break;
         		
		case OPT_SIGNATURE:
			specificsignature = opt_arg;
			break;
         		
		case OPT_ALL:
			compileall = true;
			startit = false;
			makeinitializations = false;
			break;
         		
		case OPT_SHOW:       /* Display options */
			for (j = 0; j < strlen(opt_arg); j++) {		
				switch (opt_arg[j]) {
				case 'a':
					opt_showdisassemble = true;
					compileverbose = true;
					break;
				case 'c':
					showconstantpool = true;
					break;
				case 'd':
					opt_showddatasegment = true;
					break;
				case 'e':
					opt_showexceptionstubs = true;
					break;
				case 'i':
					opt_showintermediate = true;
					compileverbose = true;
					break;
				case 'm':
					showmethods = true;
					break;
				case 'n':
					opt_shownativestub = true;
					break;
				case 'u':
					showutf = true;
					break;
				default:
					usage();
				}
			}
			break;
			
		case OPT_OLOOP:
			opt_loops = true;
			break;

		case OPT_INLINING:
			for (j = 0; j < strlen(opt_arg); j++) {		
				switch (opt_arg[j]) {
				case 'n':
				     /* define in options.h; Used in main.c, jit.c & inline.c */
#ifdef INAFTERMAIN
					useinliningm = true;
					useinlining = false;
#else
					useinlining = true;
#endif
					break;
				case 'v':
					inlinevirtuals = true;
					opt_rt = true;
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

#ifdef STATIC_ANALYSIS
		case OPT_RT:
			opt_rt = true; /* default for inlining */
			break;

		case OPT_XTA:
			opt_xta = true; /* in test currently */
			break;

		case OPT_VTA:
			printf("\nVTA is not yet available\n");
			opt_vta = false;
			/***opt_vta = true; not yet **/
			break;
#endif

#ifdef LSRA
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

		case OPT_STATIC_SUPERS:
			opt_static_supers = atoi(opt_arg);
			break;

		case OPT_NO_DYNAMIC:
			opt_no_dynamic = true;
			break;

		case OPT_TRACE:
			vm_debug = true;
			break;

		default:
			printf("Unknown option: %s\n", argv[opt_ind]);
			usage();
		}
	}

	if (opt_ind >= argc)
   		usage();


	/* transform dots into slashes in the class name */

   	mainstring = argv[opt_ind++];

	if (!jar) { 
        /* do not mangle jar filename */

		for (i = strlen(mainstring) - 1; i >= 0; i--) {
			if (mainstring[i] == '.') mainstring[i] = '/';
		}

	} else {
		/* put jarfile in classpath */

		cp = classpath;

		classpath = MNEW(char, strlen(mainstring) + strlen(":") +
						 strlen(classpath) + strlen("0"));

		strcpy(classpath, mainstring);
		strcat(classpath, ":");
		strcat(classpath, cp);
		
		MFREE(cp, char, strlen(cp));
	}

	/**************************** Program start *****************************/

	log_init(logfilename);

	if (opt_verbose)
		log_text("CACAO started -------------------------------------------------------");

	/* initialize JavaVM */

	vm_args.version = 0x00010001; /* New in 1.1.2: VM version */

	/* Get the default initialization arguments and set the class path */

	JNI_GetDefaultJavaVMInitArgs(&vm_args);

	vm_args.minHeapSize = heapstartsize;
	vm_args.maxHeapSize = heapmaxsize;

	vm_args.classpath = classpath;
 
	/* load and initialize a Java VM, return a JNI interface pointer in env */

	JNI_CreateJavaVM(&jvm, &env, &vm_args);

#if defined(ENABLE_JVMTI)
	set_jvmti_phase(JVMTI_PHASE_START);
#endif

	/* initialize the garbage collector */

	gc_init(heapmaxsize, heapstartsize);

#if defined(ENABLE_INTRP)
	/* allocate main thread stack */

	if (opt_intrp) {
		intrp_main_stack = (u1 *) alloca(opt_stacksize);
		MSET(intrp_main_stack, 0, u1, opt_stacksize);
	}
#endif

	cacao_initializing = true;

#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
  	threads_preinit();
#endif
	initLocks();
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

	if (!suck_init(bootclasspath))
		throw_main_exception_exit();

	/* initialize the memory subsystem (must be done _after_
	   threads_preinit) */

	if (!memory_init())
		throw_main_exception_exit();

	/* initialize the finalizer stuff: lock, linked list (must be done
	   _after_ threads_preinit) */

	if (!finalizer_init())
		throw_main_exception_exit();

	/* install architecture dependent signal handler used for exceptions */

	signal_init();

	/* initialize the codegen subsystems */

	codegen_init();

	/* initializes jit compiler */

	jit_init();

	/* machine dependent initialization */

	md_init();

	/* initialize the loader subsystems (must be done _after_
       classcache_init) */

	if (!loader_init((u1 *) &dummy))
		throw_main_exception_exit();

	if (!linker_init())
		throw_main_exception_exit();

	if (!native_init())
		throw_main_exception_exit();

	if (!exceptions_init())
		throw_main_exception_exit();

	if (!builtin_init())
		throw_main_exception_exit();

#if defined(USE_THREADS)
  	if (!threads_init((u1 *) &dummy))
		throw_main_exception_exit();
#endif

	/* That's important, otherwise we get into trouble, if the Runtime
	   static initializer is called before (circular dependency. This
	   is with classpath 0.09. Another important thing is, that this
	   has to happen after initThreads!!! */

	if (!initialize_class(class_java_lang_System))
		throw_main_exception_exit();

	/* JNI init creates a Java object (this means running Java code) */

	if (!jni_init())
		throw_main_exception_exit();

#if defined(USE_THREADS)
	/* finally, start the finalizer thread */

	if (!finalizer_start_thread())
		throw_main_exception_exit();
#endif

	cacao_initializing = false;


	/* start worker routines **************************************************/

	if (startit) {
		classinfo        *mainclass;    /* java/lang/Class                    */
		methodinfo       *m;
		java_objectarray *a; 
		s4                status;

		/* set return value to OK */

		status = 0;

		if (jar) {
			/* open jar file with java.util.jar.JarFile */
			mainstring = getmainclassnamefromjar(mainstring);
		}

		/* load the main class */

		if (!(mainclass = load_class_from_sysloader(utf_new_char(mainstring))))
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

		if (!m || !(m->flags & ACC_STATIC)) {
			*exceptionptr = NULL;

			*exceptionptr =
				new_exception_message(string_java_lang_NoSuchMethodError,
									  "main");
			throw_main_exception_exit();
		}

		/* build argument array */

		a = builtin_anewarray(argc - opt_ind, class_java_lang_String);
		for (i = opt_ind; i < argc; i++) {
			a->data[i - opt_ind] = 
				(java_objectheader *) javastring_new(utf_new_char(argv[i]));
		}

#ifdef TYPEINFO_DEBUG_TEST
		/* test the typeinfo system */
		typeinfo_test();
#endif
		/*class_showmethods(currentThread->group->header.vftbl->class);	*/

#if defined(ENABLE_JVMTI) && defined(NATIVE_THREADS)
		if(dbg) {
			debuggee = fork();
			if (debuggee == (-1)) {
				log_text("fork error");
				exit(1);
			} else {
				if (debuggee == 0) {
					/* child: allow helper process to trace us  */
					if (TRACEME != 0)  exit(0);
					
					/* give parent/helper debugger process control */
					kill(0, SIGTRAP);  /* do we need this at this stage ? */

					/* continue with normal startup */	

				} else {

					/* parent/helper debugger process */
					wait(&waitval);

					remotedbgjvmtienv = new_jvmtienv();
					/* set eventcallbacks */
					if (JVMTI_ERROR_NONE == 
						remotedbgjvmtienv->
						SetEventCallbacks(remotedbgjvmtienv,
										  &jvmti_jdwp_EventCallbacks,
										  sizeof(jvmti_jdwp_EventCallbacks))){
						log_text("unable to setup event callbacks");
						cacao_exit(1);						
					}

					/* setup listening process (JDWP) */
					setup_debugger_process(transport);

					/* start to be debugged program */
					CONT(debuggee);

                    /* exit debugger process - todo: cleanup */
					joinAllThreads();
					cacao_exit(0);
				}
			}
		}
		else 
			debuggee= -1;
		
#endif
		/* here we go... */

		asm_calljavafunction(m, a, NULL, NULL, NULL);

		/* exception occurred? */
		if (*exceptionptr) {
			throw_main_exception();
			status = 1;
		}

#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
		joinAllThreads();
#else
  		killThread(currentThread);
#endif
#endif

		/* now exit the JavaVM */

/*  		(*jvm)->DestroyJavaVM(jvm); */

		cacao_exit(status);
	}

	/************* If requested, compile all methods ********************/

	if (compileall) {
		classinfo *c;
		methodinfo *m;
		u4 slot;
		s4 i;
		classcache_name_entry *nmen;
		classcache_class_entry *clsen;

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

					if (!c)
						continue;

					if (!(c->state & CLASS_LINKED)) {
						if (!link_class(c)) {
							fprintf(stderr, "Error linking: ");
							utf_fprint_classname(stderr, c->name);
							fprintf(stderr, ".");
							utf_fprint(stderr, m->name);
							utf_fprint(stderr, m->descriptor);
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

						if (m->jcode) {
							if (!jit_compile(m)) {
								fprintf(stderr, "Error compiling: ");
								utf_fprint_classname(stderr, c->name);
								fprintf(stderr, ".");
								utf_fprint(stderr, m->name);
								utf_fprint(stderr, m->descriptor);
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


	/******** If requested, compile a specific method ***************/

	if (specificmethodname) {
		methodinfo *m;

		/* create, load and link the main class */

		if (!(mainclass = load_class_bootstrap(utf_new_char(mainstring))))
			throw_main_exception_exit();

		if (!link_class(mainclass))
			throw_main_exception_exit();

		if (specificsignature) {
			m = class_resolveclassmethod(mainclass,
										 utf_new_char(specificmethodname),
										 utf_new_char(specificsignature),
										 mainclass,
										 false);
		} else {
			m = class_resolveclassmethod(mainclass,
										 utf_new_char(specificmethodname),
										 NULL,
										 mainclass,
										 false);
		}

		if (!m) {
			char message[MAXLOGTEXT];
			sprintf(message, "%s%s", specificmethodname,
					specificsignature ? specificsignature : "");

			*exceptionptr =
				new_exception_message(string_java_lang_NoSuchMethodException,
									  message);
										 
			throw_main_exception_exit();
		}
		
		jit_compile(m);
	}

	cacao_shutdown(0);

	/* keep compiler happy */

	return 0;
}


/* cacao_exit ******************************************************************

   Calls java.lang.System.exit(I)V to exit the JavaVM correctly.

*******************************************************************************/

void cacao_exit(s4 status)
{
	methodinfo *m;

	assert(class_java_lang_System);
	assert(class_java_lang_System->state & CLASS_LOADED);

#if defined(ENABLE_JVMTI)
	set_jvmti_phase(JVMTI_PHASE_DEAD);
	agentunload();
#endif

	if (!link_class(class_java_lang_System))
		throw_main_exception_exit();

	/* signal that we are exiting */

	cacao_exiting = true;

	/* call java.lang.System.exit(I)V */

	m = class_resolveclassmethod(class_java_lang_System,
								 utf_new_char("exit"),
								 utf_int__void,
								 class_java_lang_Object,
								 true);
	
	if (!m)
		throw_main_exception_exit();

	/* call the exit function with passed exit status */

	/* both inlinevirtual and outsiders not allowed on exit */
	/*   not sure if permanant or temp restriction          */
	if (inlinevirtuals) inlineoutsiders = false; 

	asm_calljavafunction(m, (void *) (ptrint) status, NULL, NULL, NULL);

	/* this should never happen */

	if (*exceptionptr)
		throw_exception_exit();

	throw_cacao_exception_exit(string_java_lang_InternalError,
							   "System.exit(I)V returned without exception");
}


/*************************** Shutdown function *********************************

	Terminates the system immediately without freeing memory explicitly (to be
	used only for abnormal termination)
	
*******************************************************************************/

void cacao_shutdown(s4 status)
{

#if defined(ENABLE_JVMTI)
	agentunload();
#endif

	if (opt_verbose || getcompilingtime || opt_stat) {
		log_text("CACAO terminated by shutdown");
		dolog("Exit status: %d\n", (s4) status);
	}

	exit(status);
}


/* exit_handler ****************************************************************

   The exit_handler function is called upon program termination.

   ATTENTION: Don't free system resources here! Some threads may still
   be running as this is called from VMRuntime.exit(). The OS does the
   cleanup for us.

*******************************************************************************/

void exit_handler(void)
{
	/********************* Print debug tables ************************/

#if defined(ENABLE_DEBUG)
	if (showmethods)
		class_showmethods(mainclass);

	if (showconstantpool)
		class_showconstantpool(mainclass);

	if (showutf)
		utf_show();
#endif

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
	clear_thread_flags();		/* restores standard file descriptor
	                               flags */
#endif

	if (opt_verbose || getcompilingtime || opt_stat) {
		log_text("CACAO terminated");

#if defined(STATISTICS)
		if (opt_stat) {
			print_stats();
#ifdef TYPECHECK_STATISTICS
			typecheck_print_statistics(get_logfile());
#endif
		}

		mem_usagelog(1);

		if (getcompilingtime)
			print_times();
#endif
	}
	/* vm_print_profile(stderr);*/
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
