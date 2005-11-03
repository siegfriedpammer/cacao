/* src/cacaoh/cacaoh.c - main for header generation (cacaoh)

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

   Changes: Mark Probst
            Philipp Tomsich
            Christian Thalinger

   $Id: cacaoh.c 3548 2005-11-03 20:39:46Z twisti $

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "vm/types.h"

#include "cacaoh/headers.h"
#include "mm/boehm.h"
#include "mm/memory.h"
#include "native/include/java_lang_Throwable.h"

#if defined(USE_THREADS)
# if defined(NATIVE_THREADS)
#  include "threads/native/threads.h"
# else
#  include "threads/green/threads.h"
# endif
#endif

#include "toolbox/logging.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/statistics.h"
#include "vm/stringlocal.h"
#include "vm/tables.h"


/* define heap sizes **********************************************************/

#define HEAP_MAXSIZE      2 * 1024 * 1024;  /* default 2MB                    */
#define HEAP_STARTSIZE    100 * 1024;       /* default 100kB                  */


/* define cacaoh options ******************************************************/

#define OPT_HELP          2
#define OPT_VERSION       3
#define OPT_VERBOSE       4
#define OPT_DIRECTORY     5
#define OPT_CLASSPATH     6
#define OPT_BOOTCLASSPATH 7

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

	exit(0);
}


/* main ************************************************************************

   Main program.
   
*******************************************************************************/

int main(int argc, char **argv)
{
	s4 i, a;
	classinfo *c;
	char *opt_directory;
	void *dummy;

	/********** internal (only used by main) *****************************/
   
	char *bootclasspath;
	char *classpath;
	char *cp;
	s4    cplen;
	u4    heapmaxsize;
	u4    heapstartsize;

	if (argc < 2)
		usage();


	/* set the bootclasspath */

	cp = getenv("BOOTCLASSPATH");
	if (cp) {
		bootclasspath = MNEW(char, strlen(cp) + strlen("0"));
		strcpy(bootclasspath, cp);

	} else {
		cplen = strlen(CACAO_INSTALL_PREFIX) +
			strlen(CACAO_VM_ZIP_PATH) +
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


	/* initialize options with default values */

	opt_verbose = false;
	opt_directory = NULL;

	heapmaxsize = HEAP_MAXSIZE;
	heapstartsize = HEAP_STARTSIZE;

	while ((i = get_opt(argc, argv, opts)) != OPT_DONE) {
		switch (i) {
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
			/* Forget default bootclasspath and set the argument as new boot  */
			/* classpath.                                                     */
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
	
	/* initialize the garbage collector */

	gc_init(heapmaxsize, heapstartsize);

	tables_init();
	
	/* initialize the loader with bootclasspath */

	suck_init(bootclasspath);

	/* Also add the normal classpath, so the bootstrap class loader can find  */
	/* the files.                                                             */

	suck_init(classpath);
   
#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
	threads_preinit();
#endif
	initLocks();
#endif

	/* initialize some cacao subsystems */

	utf8_init();
	loader_init((u1 *) &dummy);


	/*********************** Load JAVA classes  **************************/
   	
	nativemethod_chain = chain_new();
	nativeclass_chain = chain_new();
	
	for (a = opt_ind; a < argc; a++) {
   		cp = argv[a];

		/* convert classname */

   		for (i = strlen(cp) - 1; i >= 0; i--) {
			switch (cp[i]) {
			case '.': cp[i] = '/';
				break;
			case '_': cp[i] = '$';
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
	tables_close();

	if (opt_verbose) {
		log_text("Java - header-generator stopped");
#if defined(STATISTICS)
		mem_usagelog(true);
#endif
	}
	
	return 0;
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
