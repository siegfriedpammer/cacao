/* main.c - contains main() and variables for the global options

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   Institut f. Computersprachen, TU Wien
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser, M. Probst,
   S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich,
   J. Wenninger

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

   This module does the following tasks:
     - Command line option handling
     - Calling initialization routines
     - Calling the class loader
     - Running the main method

   $Id: cacao.c 1359 2004-07-28 10:48:36Z twisti $

*/


#include <stdlib.h>
#include <string.h>
#include "exceptions.h"
#include "main.h"
#include "options.h"
#include "global.h"
#include "tables.h"
#include "loader.h"
#include "jit/jit.h"
#include "asmpart.h"
#include "builtin.h"
#include "native.h"
#include "statistics.h"
#include "mm/boehm.h"
#include "threads/thread.h"
#include "toolbox/logging.h"
#include "toolbox/memory.h"
#include "jit/parseRTstats.h"
#include "nat/java_io_File.h"            /* required by java_lang_Runtime.h   */
#include "nat/java_util_Properties.h"    /* required by java_lang_Runtime.h   */
#include "nat/java_lang_Runtime.h"
#include "nat/java_lang_Throwable.h"

#ifdef TYPEINFO_DEBUG_TEST
#include "typeinfo.h"
#endif


bool cacao_initializing;

char *classpath;                        /* contains classpath                 */
char *mainstring;
static classinfo *mainclass;

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
void **stackbottom = 0;
#endif


/* internal function: get_opt *************************************************
	
	decodes the next command line option
	
******************************************************************************/

#define OPT_DONE       -1
#define OPT_ERROR       0
#define OPT_IGNORE      1

#define OPT_CLASSPATH   2
#define OPT_D           3
#define OPT_MS          4
#define OPT_MX          5
#define OPT_VERBOSE1    6
#define OPT_VERBOSE     7
#define OPT_VERBOSEGC   8
#define OPT_VERBOSECALL 9
#define OPT_NOIEEE      10
#define OPT_SOFTNULL    11
#define OPT_TIME        12
#define OPT_STAT        13
#define OPT_LOG         14
#define OPT_CHECK       15
#define OPT_LOAD        16
#define OPT_METHOD      17
#define OPT_SIGNATURE   18
#define OPT_SHOW        19
#define OPT_ALL         20
#define OPT_OLOOP       24
#define OPT_INLINING	25
#define OPT_RT          26
#define OPT_XTA         27 
#define OPT_VTA         28
#define OPT_VERBOSETC   29
#define OPT_NOVERIFY    30
#define OPT_LIBERALUTF  31
#define OPT_VERBOSEEXCEPTION 32
#define OPT_EAGER            33


struct {char *name; bool arg; int value;} opts[] = {
	{"classpath",        true,   OPT_CLASSPATH},
	{"cp",               true,   OPT_CLASSPATH},
	{"D",                true,   OPT_D},
	{"Xms",              true,   OPT_MS},
	{"Xmx",              true,   OPT_MX},
	{"ms",               true,   OPT_MS},
	{"mx",               true,   OPT_MX},
	{"noasyncgc",        false,  OPT_IGNORE},
	{"noverify",         false,  OPT_NOVERIFY},
	{"liberalutf",       false,  OPT_LIBERALUTF},
	{"oss",              true,   OPT_IGNORE},
	{"ss",               true,   OPT_IGNORE},
	{"v",                false,  OPT_VERBOSE1},
	{"verbose",          false,  OPT_VERBOSE},
	{"verbosegc",        false,  OPT_VERBOSEGC},
	{"verbosecall",      false,  OPT_VERBOSECALL},
	{"verboseexception", false,  OPT_VERBOSEEXCEPTION},
#ifdef TYPECHECK_VERBOSE
	{"verbosetc",        false,  OPT_VERBOSETC},
#endif
#if defined(__ALPHA__)
	{"noieee",           false,  OPT_NOIEEE},
#endif
	{"softnull",         false,  OPT_SOFTNULL},
	{"time",             false,  OPT_TIME},
	{"stat",             false,  OPT_STAT},
	{"log",              true,   OPT_LOG},
	{"c",                true,   OPT_CHECK},
	{"l",                false,  OPT_LOAD},
    { "eager",            false,  OPT_EAGER },
	{"m",                true,   OPT_METHOD},
	{"sig",              true,   OPT_SIGNATURE},
	{"s",                true,   OPT_SHOW},
	{"all",              false,  OPT_ALL},
	{"oloop",            false,  OPT_OLOOP},
	{"i",		         true,   OPT_INLINING},
	{"rt",               false,  OPT_RT},
	{"xta",              false,  OPT_XTA},
	{"vta",              false,  OPT_VTA},
	{NULL,               false,  0}
};

static int opt_ind = 1;
static char *opt_arg;


static int get_opt(int argc, char **argv)
{
	char *a;
	int i;
	
	if (opt_ind >= argc) return OPT_DONE;
	
	a = argv[opt_ind];
	if (a[0] != '-') return OPT_DONE;

	for (i = 0; opts[i].name; i++) {
		if (!opts[i].arg) {
			if (strcmp(a + 1, opts[i].name) == 0) { /* boolean option found */
				opt_ind++;
				return opts[i].value;
			}

		} else {
			if (strcmp(a + 1, opts[i].name) == 0) { /* parameter option found */
				opt_ind++;
				if (opt_ind < argc) {
					opt_arg = argv[opt_ind];
					opt_ind++;
					return opts[i].value;
				}
				return OPT_ERROR;

			} else {
				size_t l = strlen(opts[i].name);
				if (strlen(a + 1) > l) {
					if (memcmp(a + 1, opts[i].name, l) == 0) {
						opt_ind++;
						opt_arg = a + 1 + l;
						return opts[i].value;
					}
				}
			}
		}
	} /* end for */	

	return OPT_ERROR;
}


/******************** interne Function: print_usage ************************

Prints the correct usage syntax to stdout.

***************************************************************************/

static void print_usage()
{
	printf("USAGE: cacao [options] classname [program arguments]\n");
	printf("Options:\n");
	printf("          -cp path ............. specify a path to look for classes\n");
	printf("          -classpath path ...... specify a path to look for classes\n");
	printf("          -Dpropertyname=value . add an entry to the property list\n");
	printf("          -Xmx maxmem[kK|mM] ... specify the size for the heap\n");
	printf("          -Xms initmem[kK|mM] .. specify the initial size for the heap\n");
	printf("          -mx maxmem[kK|mM] .... specify the size for the heap\n");
	printf("          -ms initmem[kK|mM] ... specify the initial size for the heap\n");
	printf("          -v ................... write state-information\n");
	printf("          -verbose ............. write more information\n");
	printf("          -verbosegc ........... write message for each GC\n");
	printf("          -verbosecall ......... write message for each call\n");
	printf("          -verboseexception .... write message for each step of stack unwinding\n");
#ifdef TYPECHECK_VERBOSE
	printf("          -verbosetc ........... write debug messages while typechecking\n");
#endif
#if defined(__ALPHA__)
	printf("          -noieee .............. don't use ieee compliant arithmetic\n");
#endif
	printf("          -noverify ............ don't verify classfiles\n");
	printf("          -liberalutf........... don't warn about overlong UTF-8 sequences\n");
	printf("          -softnull ............ use software nullpointer check\n");
	printf("          -time ................ measure the runtime\n");
	printf("          -stat ................ detailed compiler statistics\n");
	printf("          -log logfile ......... specify a name for the logfile\n");
	printf("          -c(heck)b(ounds) ..... don't check array bounds\n");
	printf("                  s(ync) ....... don't check for synchronization\n");
	printf("          -oloop ............... optimize array accesses in loops\n"); 
	printf("          -l ................... don't start the class after loading\n");
	printf("          -eager ............... perform eager class loading and linking\n");
	printf("          -all ................. compile all methods, no execution\n");
	printf("          -m ................... compile only a specific method\n");
	printf("          -sig ................. specify signature for a specific method\n");
	printf("          -s(how)a(ssembler) ... show disassembled listing\n");
	printf("                 c(onstants) ... show the constant pool\n");
	printf("                 d(atasegment).. show data segment listing\n");
	printf("                 i(ntermediate). show intermediate representation\n");
	printf("                 m(ethods)...... show class fields and methods\n");
	printf("                 u(tf) ......... show the utf - hash\n");
	printf("          -i     n ............. activate inlining\n");
	printf("                 v ............. inline virtual methods\n");
	printf("                 e ............. inline methods with exceptions\n");
	printf("                 p ............. optimize argument renaming\n");
	printf("                 o ............. inline methods of foreign classes\n");
	printf("          -rt .................. use rapid type analysis\n");
	printf("          -xta ................. use x type analysis\n");
	printf("          -vta ................. use variable type analysis\n");
}   


#ifdef TYPECHECK_STATISTICS
void typecheck_print_statistics(FILE *file);
#endif


/*
 * void exit_handler(void)
 * -----------------------
 * The exit_handler function is called upon program termination to shutdown
 * the various subsystems and release the resources allocated to the VM.
 */
void exit_handler(void)
{
	/********************* Print debug tables ************************/
				
	if (showmethods) class_showmethods(mainclass);
	if (showconstantpool) class_showconstantpool(mainclass);
	if (showutf) utf_show();

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
	clear_thread_flags();		/* restores standard file descriptor
	                               flags */
#endif

	/************************ Free all resources *******************/

	loader_close();
	tables_close();

	MFREE(classpath, u1, strlen(classpath));

	if (verbose || getcompilingtime || opt_stat) {
		log_text("CACAO terminated");
		if (opt_stat) {
			print_stats();
#ifdef TYPECHECK_STATISTICS
			typecheck_print_statistics(get_logfile());
#endif
		}
		if (getcompilingtime)
			print_times();
		mem_usagelog(1);
	}
}


/************************** Function: main *******************************

   The main program.
   
**************************************************************************/

int main(int argc, char **argv)
{
	s4 i, j;
	void *dummy;
	
	/********** interne (nur fuer main relevante Optionen) **************/
   
	char logfilename[200] = "";
	u4 heapmaxsize = 64 * 1024 * 1024;
	u4 heapstartsize = 200 * 1024;
	char *cp;
	bool startit = true;
	char *specificmethodname = NULL;
	char *specificsignature = NULL;

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
	stackbottom = &dummy;
#endif
	
	if (atexit(exit_handler))
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Unable to register exit_handler");


	/************ Collect info from the environment ************************/

	classpath = MNEW(u1, 2);
	strcpy(classpath, ".");

	/* get classpath environment */
	cp = getenv("CLASSPATH");
	if (cp) {
		classpath = MREALLOC(classpath,
							 u1,
							 strlen(classpath),
							 strlen(classpath) + 1 + strlen(cp) + 1);
		strcat(classpath, ":");
		strncat(classpath, cp, strlen(cp));
	}

	/***************** Interpret the command line *****************/
   
	checknull = false;
	opt_noieee = false;

	while ((i = get_opt(argc, argv)) != OPT_DONE) {
		switch (i) {
		case OPT_IGNORE: break;
			
		case OPT_CLASSPATH:
			classpath = MREALLOC(classpath,
								 u1,
								 strlen(classpath),
								 strlen(classpath) + 1 + strlen(opt_arg) + 1);
			strcat(classpath, ":");
			strncat(classpath, opt_arg, strlen(opt_arg));
			break;
				
		case OPT_D:
			{
				int n;
				int l = strlen(opt_arg);
				for (n = 0; n < l; n++) {
					if (opt_arg[n] == '=') {
						opt_arg[n] = '\0';
						attach_property(opt_arg, opt_arg + n + 1);
						goto didit;
					}
				}
				print_usage();
				exit(10);
					
			didit: ;
			}	
			break;

		case OPT_MS:
		case OPT_MX:
			{
				char c;
				c = opt_arg[strlen(opt_arg) - 1];

				if (c == 'k' || c == 'K') {
					j = 1024 * atoi(opt_arg);

				} else if (c == 'm' || c == 'M') {
					j = 1024 * 1024 * atoi(opt_arg);

				} else j = atoi(opt_arg);

				if (i == OPT_MX) heapmaxsize = j;
				else heapstartsize = j;
			}
			break;

		case OPT_VERBOSE1:
			verbose = true;
			break;

		case OPT_VERBOSE:
			verbose = true;
			loadverbose = true;
			linkverbose = true;
			initverbose = true;
			compileverbose = true;
			break;

		case OPT_VERBOSEEXCEPTION:
			verboseexception = true;
			break;

		case OPT_VERBOSEGC:
			collectverbose = true;
			break;

#ifdef TYPECHECK_VERBOSE
		case OPT_VERBOSETC:
			typecheckverbose = true;
			break;
#endif
				
		case OPT_VERBOSECALL:
			runverbose = true;
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
					
		case OPT_STAT:
			opt_stat = true;
			break;
					
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
					print_usage();
					exit(10);
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
					showdisassemble = true;
					compileverbose = true;
					break;
				case 'c':
					showconstantpool = true;
					break;
				case 'd':
					showddatasegment = true;
					break;
				case 'i':
					showintermediate = true;
					compileverbose = true;
					break;
				case 'm':
					showmethods = true;
					break;
				case 'u':
					showutf = true;
					break;
				default:
					print_usage();
					exit(10);
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
					useinlining = true;
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
					print_usage();
					exit(10);
				}
			}
			break;

		case OPT_RT:
			opt_rt = true;
			break;

		case OPT_XTA:
			opt_xta = false; /**not yet **/
			break;

		case OPT_VTA:
			/***opt_vta = true; not yet **/
			break;

		default:
			print_usage();
			exit(10);
		}
	}
   
   
	if (opt_ind >= argc) {
   		print_usage();
   		exit(10);
	}

   	mainstring = argv[opt_ind++];
   	for (i = strlen(mainstring) - 1; i >= 0; i--) {     /* Transform dots into slashes */
 	 	if (mainstring[i] == '.') mainstring[i] = '/';  /* in the class name */
	}


	/**************************** Program start *****************************/

	log_init(logfilename);
	if (verbose)
		log_text("CACAO started -------------------------------------------------------");

	/* initialize the garbage collector */
	gc_init(heapmaxsize, heapstartsize);

	tables_init();
	suck_init(classpath);

	cacao_initializing = true;

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
  	initThreadsEarly();
#endif

	/* install architecture dependent signal handler used for exceptions */
	init_exceptions();

	/* initializes jit compiler and codegen stuff */
	jit_init();

	loader_init((u1 *) &dummy);

	jit_init();

	/* initialize exceptions used in the system */
	init_system_exceptions();

	native_loadclasses();

#if defined(USE_THREADS)
  	initThreads((u1 *) &dummy);
#endif

	*threadrootmethod = NULL;

	/*That's important, otherwise we get into trouble, if the Runtime static
	  initializer is called before (circular dependency. This is with
	  classpath 0.09. Another important thing is, that this has to happen
	  after initThreads!!! */

	if (!class_init(class_new(utf_new_char("java/lang/System"))))
		throw_main_exception_exit();

	cacao_initializing = false;

	/************************* Start worker routines ********************/

	if (startit) {
		methodinfo *mainmethod;
		java_objectarray *a; 

		/* create, load and link the main class */
		mainclass = class_new(utf_new_char(mainstring));

		if (!class_load(mainclass))
			throw_main_exception_exit();

		if (!class_link(mainclass))
			throw_main_exception_exit();

		mainmethod = class_resolveclassmethod(mainclass,
											  utf_new_char("main"), 
											  utf_new_char("([Ljava/lang/String;)V"),
											  mainclass,
											  false);

		/* problems with main method? */
/*  		if (*exceptionptr) */
/*  			throw_exception_exit(); */

		/* there is no main method or it isn't static */
		if (!mainmethod || !(mainmethod->flags & ACC_STATIC)) {
			*exceptionptr =
				new_exception_message(string_java_lang_NoSuchMethodError,
									  "main");
			throw_main_exception_exit();
		}

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

		*threadrootmethod = mainmethod;


		/* here we go... */
		asm_calljavafunction(mainmethod, a, NULL, NULL, NULL);

		/* exception occurred? */
		if (*exceptionptr)
			throw_main_exception();

#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
		joinAllThreads();
#else
  		killThread(currentThread);
#endif
#endif

		/* now exit the JavaVM */

		cacao_exit(0);
	}

	/************* If requested, compile all methods ********************/

	if (compileall) {
		classinfo *c;
		methodinfo *m;
		u4 slot;
		s4 i;

		/* create all classes found in the classpath */
		/* XXX currently only works with zip/jar's */
		create_all_classes();

		/* load and link all classes */
		for (slot = 0; slot < class_hash.size; slot++) {
			c = class_hash.ptr[slot];

			while (c) {
				if (!c->loaded)
					if (!class_load(c))
						throw_main_exception_exit();

				if (!c->linked)
					if (!class_link(c))
						throw_main_exception_exit();

				/* compile all class methods */
				for (i = 0; i < c->methodscount; i++) {
					m = &(c->methods[i]);
					if (m->jcode) {
						(void) jit_compile(m);
					}
				}

				c = c->hashlink;
			}
		}
	}


	/******** If requested, compile a specific method ***************/

	if (specificmethodname) {
		methodinfo *m;

		/* create, load and link the main class */
		mainclass = class_new(utf_new_char(mainstring));

		if (!class_load(mainclass))
			throw_main_exception_exit();

		if (!class_link(mainclass))
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

   Calls java.lang.Runtime.exit(I)V to exit the JavaVM correctly.

*******************************************************************************/

void cacao_exit(s4 status)
{
	classinfo *c;
	methodinfo *m;
	java_lang_Runtime *rt;

	/* class should already be loaded, but who knows... */

	c = class_new(utf_new_char("java/lang/Runtime"));

	if (!class_load(c))
		throw_main_exception_exit();

	if (!class_link(c))
		throw_main_exception_exit();

	/* first call Runtime.getRuntime()Ljava.lang.Runtime; */

	m = class_resolveclassmethod(c,
								 utf_new_char("getRuntime"),
								 utf_new_char("()Ljava/lang/Runtime;"),
								 class_java_lang_Object,
								 true);

	if (!m)
		throw_main_exception_exit();

	rt = (java_lang_Runtime *) asm_calljavafunction(m,
													(void *) 0,
													NULL,
													NULL,
													NULL);

	/* exception occurred? */

	if (*exceptionptr)
		throw_main_exception_exit();

	/* then call Runtime.exit(I)V */

	m = class_resolveclassmethod(c,
								 utf_new_char("exit"),
								 utf_new_char("(I)V"),
								 class_java_lang_Object,
								 true);
	
	if (!m)
		throw_main_exception_exit();

	asm_calljavafunction(m, rt, (void *) 0, NULL, NULL);

	/* this should never happen */

	throw_cacao_exception_exit(string_java_lang_InternalError,
							   "Problems with Runtime.exit(I)V");
}


/*************************** Shutdown function *********************************

	Terminates the system immediately without freeing memory explicitly (to be
	used only for abnormal termination)
	
*******************************************************************************/

void cacao_shutdown(s4 status)
{
	/**** RTAprint ***/

	if (verbose || getcompilingtime || opt_stat) {
		log_text("CACAO terminated by shutdown");
		if (opt_stat)
			print_stats();
		if (getcompilingtime)
			print_times();
		mem_usagelog(0);
		dolog("Exit status: %d\n", (int) status);
	}

	exit(status);
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
