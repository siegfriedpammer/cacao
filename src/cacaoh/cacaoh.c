/* src/cacaoh/cacaoh.c - main for header generation (cacaoh)

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

   Authors: Reinhard Grafl

   Changes: Mark Probst
            Philipp Tomsich
            Christian Thalinger

   $Id: cacaoh.c 5809 2006-10-20 13:09:54Z twisti $

*/


#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm/types.h"

#include "cacaoh/headers.h"
#include "mm/boehm.h"
#include "mm/memory.h"
#include "native/include/java_lang_Throwable.h"

#if defined(ENABLE_THREADS)
# include "threads/native/threads.h"
#endif

#include "toolbox/logging.h"
#include "vm/classcache.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/hashtable.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/statistics.h"
#include "vm/stringlocal.h"
#include "vm/suck.h"


/* define heap sizes **********************************************************/

#define HEAP_MAXSIZE      2 * 1024 * 1024   /* default 2MB                    */
#define HEAP_STARTSIZE         100 * 1024   /* default 100kB                  */


/* define cacaoh options ******************************************************/

enum {
	OPT_HELP,
	OPT_VERSION,
	OPT_VERBOSE,
	OPT_DIRECTORY,
	OPT_CLASSPATH,
	OPT_BOOTCLASSPATH,

	DUMMY
};


opt_struct opts[] = {
	{ "help",             false, OPT_HELP          },
	{ "version",          false, OPT_VERSION       },
	{ "verbose",          false, OPT_VERBOSE       },
	{ "d",                true,  OPT_DIRECTORY     },
	{ "classpath",        true,  OPT_CLASSPATH     },
	{ "bootclasspath",    true,  OPT_BOOTCLASSPATH },
	{ NULL,               false, 0                 }
};


/* usage ***********************************************************************

   Obviously prints usage information of cacaoh.

*******************************************************************************/

static void usage(void)
{
	printf("Usage: cacaoh [options] <classes>\n"
		   "\n"
		   "Options:\n"
		   "    -help                 Print this message\n"
		   "    -classpath <path>     \n"
		   "    -bootclasspath <path> \n"
		   "    -d <dir>              Output directory\n"
		   "    -version              Print version information\n"
		   "    -verbose              Enable verbose output\n");

	/* exit with error code */

	exit(1);
}


/* version *********************************************************************

   Prints cacaoh version information.

*******************************************************************************/

static void version(void)
{
	printf("cacaoh version "VERSION"\n");
	printf("Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,\n");
	printf("C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,\n");
	printf("E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,\n");
	printf("J. Wenninger, Institut f. Computersprachen - TU Wien\n\n");

	printf("This program is free software; you can redistribute it and/or\n");
	printf("modify it under the terms of the GNU General Public License as\n");
	printf("published by the Free Software Foundation; either version 2, or (at\n");
	printf("your option) any later version.\n\n");

	printf("This program is distributed in the hope that it will be useful, but\n");
	printf("WITHOUT ANY WARRANTY; without even the implied warranty of\n");
	printf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n");
	printf("General Public License for more details.\n");

	exit(0);
}


/* forward declarations *******************************************************/

static JavaVMInitArgs *cacaoh_options_prepare(int argc, char **argv);


/* main ************************************************************************

   Main program.
   
*******************************************************************************/

int main(int argc, char **argv)
{
	JavaVMInitArgs *vm_args;
	s4 i, j;
	s4 opt;
	classinfo *c;
	char *opt_directory;

	/********** internal (only used by main) *****************************/
   
	char *bootclasspath;
	char *classpath;
	char *cp;
	s4    cplen;
	u4    heapmaxsize;
	u4    heapstartsize;

	if (argc < 2)
		usage();


#if defined(DISABLE_GC)
	nogc_init(HEAP_MAXSIZE, HEAP_STARTSIZE);
#endif

	/* set the bootclasspath */

	cp = getenv("BOOTCLASSPATH");

	if (cp) {
		bootclasspath = MNEW(char, strlen(cp) + strlen("0"));
		strcpy(bootclasspath, cp);
	}
	else {
		cplen = strlen(CACAO_VM_ZIP) +
			strlen(":") +
			strlen(CLASSPATH_GLIBJ_ZIP) +
			strlen("0");

		bootclasspath = MNEW(char, cplen);
		strcat(bootclasspath, CACAO_VM_ZIP);
		strcat(bootclasspath, ":");
		strcat(bootclasspath, CLASSPATH_GLIBJ_ZIP);
	}


	/* set the classpath */

	cp = getenv("CLASSPATH");

	if (cp) {
		classpath = MNEW(char, strlen(cp) + strlen("0"));
		strcat(classpath, cp);
	}
	else {
		classpath = MNEW(char, strlen(".") + strlen("0"));
		strcpy(classpath, ".");
	}


	/* initialize options with default values */

	opt_verbose = false;
	opt_directory = NULL;

	heapmaxsize = HEAP_MAXSIZE;
	heapstartsize = HEAP_STARTSIZE;


	/* parse the options ******************************************************/

	vm_args = cacaoh_options_prepare(argc, argv);

	while ((opt = options_get(opts, vm_args)) != OPT_DONE) {
		switch (opt) {
		case OPT_IGNORE:
			break;

		case OPT_HELP:
			usage();
			break;

		case OPT_CLASSPATH:
			/* forget old classpath and set the argument as new classpath */
			MFREE(classpath, char, strlen(classpath));

			classpath = MNEW(char, strlen(opt_arg) + strlen("0"));
			strcpy(classpath, opt_arg);
			break;

		case OPT_BOOTCLASSPATH:
			/* Forget default bootclasspath and set the argument as
			   new boot classpath. */
			MFREE(bootclasspath, char, strlen(bootclasspath));

			bootclasspath = MNEW(char, strlen(opt_arg) + strlen("0"));
			strcpy(bootclasspath, opt_arg);
			break;

		case OPT_DIRECTORY:
			opt_directory = MNEW(char, strlen(opt_arg) + strlen("0"));
			strcpy(opt_directory, opt_arg);
			break;

		case OPT_VERSION:
			version();
			break;

		case OPT_VERBOSE:
			opt_verbose = true;
			loadverbose = true;
			linkverbose = true;
			break;

		default:
			usage();
		}
	}
			
	/**************************** Program start **************************/

	if (opt_verbose) {
		log_init(NULL);
		log_text("Java - header-generator started"); 
	}
	
#if defined(ENABLE_THREADS)
	/* pre-initialize some core thread stuff, like the stopworldlock,
	   thus this has to happen _before_ gc_init()!!! */

	threads_preinit();
#endif

	/* initialize the garbage collector */

	gc_init(heapmaxsize, heapstartsize);

	/* initialize the string hashtable stuff: lock (must be done
	   _after_ threads_preinit) */

	if (!string_init())
		throw_main_exception_exit();

	/* initialize the utf8 hashtable stuff: lock, often used utf8 strings
	   (must be done _after_ threads_preinit) */

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

	suck_add(bootclasspath);

	/* Also add the normal classpath, so the bootstrap class loader
	   can find the files. */

	suck_add(classpath);

	/* initialize the loader subsystems (must be done _after_
       classcache_init) */

	if (!loader_init())
		throw_main_exception_exit();


	/* load Java classes ******************************************************/
   	
	for (i = opt_index; i < vm_args->nOptions; i++) {
   		cp = vm_args->options[i].optionString;

		/* convert classname */

   		for (j = strlen(cp) - 1; j >= 0; j--) {
			switch (cp[j]) {
			case '.':
				cp[j] = '/';
				break;
			case '_':
				cp[j] = '$';
				break;
  	 		}
		}
	
		/* exceptions are catched with new_exception call */

		if (!(c = load_class_bootstrap(utf_new_char(cp))))
			throw_cacao_exception_exit(string_java_lang_NoClassDefFoundError,
									   cp);

		if (!link_class(c))
			throw_cacao_exception_exit(string_java_lang_LinkageError,
									   cp);

		headerfile_generate(c, opt_directory);
	}

	/************************ Release all resources **********************/

	loader_close();

	if (opt_verbose) {
		log_text("Java - header-generator stopped");
#if defined(ENABLE_STATISTICS)
		mem_usagelog(true);
#endif
	}
	
	return 0;
}


/* cacaoh_options_prepare ******************************************************

   Prepare the JavaVMInitArgs.

*******************************************************************************/

static JavaVMInitArgs *cacaoh_options_prepare(int argc, char **argv)
{
	JavaVMInitArgs *vm_args;
	s4              i;

	vm_args = NEW(JavaVMInitArgs);

	vm_args->nOptions = argc - 1;
	vm_args->options  = MNEW(JavaVMOption, argc);

	for (i = 1; i < argc; i++)
		vm_args->options[i - 1].optionString = argv[i];

	return vm_args;
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
