/* gennativetable.c - generate nativetable.h for native.c

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

   Authors: Christian Thalinger

   Changes:

   $Id: gennativetable.c 1264 2004-07-01 14:21:05Z jowenn $

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "types.h"
#include "global.h"
#include "headers.h"
#include "loader.h"
#include "tables.h"
#include "mm/boehm.h"
#include "threads/thread.h"
#include "toolbox/chain.h"
#include "toolbox/memory.h"


int main(int argc, char **argv)
{
	s4 i, j, k;
	char classpath[500] = "";
	char *cp;
	classinfo *c;
	methodinfo *m;
	methodinfo *m2;
	bool nativelyoverloaded;
	u4 heapmaxsize = 2 * 1024 * 1024;
	u4 heapstartsize = 100 * 1024;
	void *dummy;

	/* XXX change me */
	char classname[1024];

	if (argc < 2) {
   		printf("Usage: gennativetable class [class...]\n");
   		exit(1);
	}

	cp = getenv("CLASSPATH");
	if (cp) {
		strcpy(classpath + strlen(classpath), ":");
		strcpy(classpath + strlen(classpath), cp);
	}

	/* initialize the garbage collector */
	gc_init(heapmaxsize, heapstartsize);

	tables_init();

	suck_init(classpath);
   

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	initThreadsEarly();
#endif
	loader_init((u1 *) &dummy);


	/*********************** Load JAVA classes  **************************/

   	nativeclass_chain = chain_new();
	nativemethod_chain = chain_new();

	for (i = 1; i < argc; i++) {
   		cp = argv[i];

		/* convert classname */
   		for (j = strlen(cp) - 1; j >= 0; j--) {
			switch (cp[j]) {
			case '.': cp[j] = '/';
				break;
			case '_': cp[j] = '$';
  	 		}
		}
	
		c = class_new(utf_new_char(cp));

		/* exceptions are catched with new_exception call */
		class_load(c);
		class_link(c);

		chain_addlast(nativeclass_chain, c);

		/* find overloaded methods */
		for (j = 0; j < c->methodscount; j++) {
			m = &(c->methods[j]);

			if (!(m->flags & ACC_NATIVE))
				continue;

			if (!m->nativelyoverloaded) {
				nativelyoverloaded = false;
				
				for (k = j + 1; k < c->methodscount; k++) {
					m2 = &(c->methods[k]);

					if (!(m2->flags & ACC_NATIVE))
						continue;

					if (m->name == m2->name) {
						m2->nativelyoverloaded = true;
						nativelyoverloaded = true;
					}
				}
				m->nativelyoverloaded = nativelyoverloaded;
			}
		}

		for (j = 0; j < c->methodscount; j++) {
			m = &(c->methods[j]);

			if (m->flags & ACC_NATIVE) {
				chain_addlast(nativemethod_chain, m);
			}
		}
	}

	/* create table of native-methods */

/*  	file = fopen("nativetable.h", "w"); */
	file = stdout;

	if (!file)
		panic("Can not open file 'nativetable' to store native-link-table");

	fprintf(file, "/* Table of native methods: nativetables.hh */\n");
	fprintf(file, "/* This file is machine generated, don't edit it !*/\n\n"); 

	c = chain_first(nativeclass_chain);
	while (c) {
		gen_header_filename(classname, c->name);
		fprintf(file, "#include \"nat/%s.h\"\n", classname);
		c = chain_next(nativeclass_chain);
	}
	chain_free(nativeclass_chain);

	fprintf(file, "\n\n#include \"native.h\"\n\n");
	fprintf(file, "static nativeref nativetable[] = {\n");

	m = chain_first(nativemethod_chain);
	while (m) {
		printnativetableentry(m);
		m = chain_next(nativemethod_chain);
	}
	chain_free(nativemethod_chain);

	fprintf(file, "};\n");

	fclose(file);

	/* release all resources */

	loader_close();
	tables_close(literalstring_free);

	/* everything is ok */

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

  
