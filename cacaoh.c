/* cacaoh.c - main for header generation (cacaoh)

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser,
   M. Probst, S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck,
   P. Tomsich, J. Wenninger

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

   $Id: cacaoh.c 1371 2004-08-01 21:55:39Z stefan $

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "global.h"
#include "headers.h"
#include "loader.h"
#include "tables.h"
#include "mm/boehm.h"
#include "threads/thread.h"
#include "toolbox/logging.h"
#include "toolbox/memory.h"


/************************** Function: main *******************************

   Main program.
   
**************************************************************************/

int main(int argc, char **argv)
{
	s4 i,a;
	char *cp;
	classinfo *c;
		

	/********** internal (only used by main) *****************************/
   
	char classpath[500] = "";
	u4 heapmaxsize = 2 * 1024 * 1024;
	u4 heapstartsize = 100 * 1024;


	/************ Collect some info from the environment *****************/

	if (argc < 2) {
		printf("Usage: cacaoh class [class..]\n");
   		exit(1);
	}

	cp = getenv("CLASSPATH");
	if (cp) {
		strcpy(classpath + strlen(classpath), ":");
		strcpy(classpath + strlen(classpath), cp);
	}


	/**************************** Program start **************************/

	log_init(NULL);
	log_text("Java - header-generator started"); 
	
	/* initialize the garbage collector */
	gc_init(heapmaxsize, heapstartsize);

	tables_init();

	suck_init(classpath);
   

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	initThreadsEarly();
#endif
	initLocks();
	loader_init();


	/*********************** Load JAVA classes  **************************/
   	
	nativemethod_chain = chain_new();
	nativeclass_chain = chain_new();
	
	for (a = 1; a < argc; a++) {
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

		headerfile_generate(c);
	}

	/************************ Release all resources **********************/

	loader_close();
	tables_close(literalstring_free);

	/* Print "finished" message */

	log_text("Java - header-generator stopped");
	log_cputime();
	mem_usagelog(1);
	
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
