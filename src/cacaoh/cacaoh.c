/* cacaoh/cacaoh.c - main for header generation (cacaoh)

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

   $Id: cacaoh.c 1735 2004-12-07 14:33:27Z twisti $

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "types.h"
#include "cacaoh/headers.h"
#include "mm/boehm.h"
#include "mm/memory.h"

#if defined(USE_THREADS)
# if defined(NATIVE_THREADS)
#  include "threads/native/threads.h"
# else
#  include "threads/green/threads.h"
# endif
#endif

#include "toolbox/logging.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/tables.h"


/* define cacaoh options ******************************************************/

#define OPT_HELP      2
#define OPT_VERSION   3
#define OPT_VERBOSE   4
#define OPT_DIRECTORY 5

opt_struct opts[] = {
	{ "help",             false, OPT_HELP      },
	{ "version",          false, OPT_VERSION   },
	{ "verbose",          false, OPT_VERBOSE   },
	{ "d",                true,  OPT_DIRECTORY },
	{ NULL,               false, 0 }
};


/* usage ***********************************************************************

   Obviously prints usage information of cacaoh.

*******************************************************************************/

static void usage()
{
	printf("Usage: cacaoh [options] <classes>\n"
		   "\n"
		   "Options:\n"
		   "        -help           Print this message\n"
		   "        -version        Print version information\n"
		   "        -verbose        Enable verbose output\n"
		   "        -d <dir>        Output directory\n");

	/* exit with error code */

	exit(1);
}


static void version()
{
	printf("cacaoh "VERSION"\n");
	exit(0);
}


/* main ************************************************************************

   Main program.
   
*******************************************************************************/

int main(int argc, char **argv)
{
	s4 i, a;
	char *cp;
	classinfo *c;
	char *opt_directory;

	/********** internal (only used by main) *****************************/
   
	char classpath[500] = "";
	u4 heapmaxsize = 2 * 1024 * 1024;
	u4 heapstartsize = 100 * 1024;


	/************ Collect some info from the environment *****************/

	if (argc < 2)
		usage();

	cp = getenv("CLASSPATH");
	if (cp) {
		strcpy(classpath + strlen(classpath), ":");
		strcpy(classpath + strlen(classpath), cp);
	}

	/* initialize options with default values */

	opt_verbose = false;
	opt_directory = NULL;

	while ((i = get_opt(argc, argv, opts)) != OPT_DONE) {
		switch (i) {
		case OPT_IGNORE:
			break;

		case OPT_HELP:
			usage();
			break;

		case OPT_VERSION:
			version();
			break;

		case OPT_VERBOSE:
			opt_verbose = true;
			break;

		case OPT_DIRECTORY:
			opt_directory = MNEW(char, strlen(opt_arg));
			strcpy(opt_directory, opt_arg);
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

	suck_init(classpath);
   
#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
	initThreadsEarly();
#endif
	initLocks();
#endif

	loader_init();


	/*********************** Load JAVA classes  **************************/
   	
	nativemethod_chain = chain_new();
	nativeclass_chain = chain_new();
	
	for (a = opt_ind; a < argc; a++) {
   		cp = argv[a];

		/* convert classname */
   		for (i = strlen(cp) - 1; i >= 0; i--) {
			switch (cp[i]) {
			case '.': cp[i]='/';
				break;
			case '_': cp[i]='$';    
  	 		}
		}
	
		c = class_new(utf_new_char(cp));

		/* exceptions are catched with new_exception call */
		class_load(c);
		class_link(c);

		headerfile_generate(c, opt_directory);
	}

	/************************ Release all resources **********************/

	loader_close();
	tables_close(literalstring_free);

	if (opt_verbose) {
		log_text("Java - header-generator stopped");
		log_cputime();
		mem_usagelog(1);
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
