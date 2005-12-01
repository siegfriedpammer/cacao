/* src/native/tools/gennativetable.c - generate nativetable.h for native.c

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

   Authors: Christian Thalinger

   Changes:

   $Id: gennativetable.c 3833 2005-12-01 23:22:45Z twisti $

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "vm/types.h"

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

#include "toolbox/chain.h"
#include "vm/classcache.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/tables.h"


int main(int argc, char **argv)
{
	char *bootclasspath;
	char *cp;

	classcache_name_entry *nmen;
	classcache_class_entry *clsen;
	classinfo *c;
	s4 i;
	s4 j;
	u4 slot;
	methodinfo *m;
	methodinfo *m2;
	bool nativelyoverloaded;

	u4 heapmaxsize = 4 * 1024 * 1024;
	u4 heapstartsize = 100 * 1024;
	void *dummy;

	/* set the bootclasspath */

	cp = getenv("BOOTCLASSPATH");
	if (cp) {
		bootclasspath = MNEW(char, strlen(cp) + 1);
		strcpy(bootclasspath, cp);
	}

	/* initialize the garbage collector */
	gc_init(heapmaxsize, heapstartsize);

	tables_init();

	suck_init(bootclasspath);
   
#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
	initThreadsEarly();
#endif
	initLocks();
#endif

	/* initialize some cacao subsystems */

	utf8_init();
	loader_init((u1 *) &dummy);


	/*********************** Load JAVA classes  **************************/

	nativemethod_chain = chain_new();
	ident_chain = chain_new();

	/* load all classes from bootclasspath */

	loader_load_all_classes();

	/* link all classes */

	for (slot = 0; slot < classcache_hash.size; slot++) {
		nmen = (classcache_name_entry *) classcache_hash.ptr[slot];

		for (; nmen; nmen = nmen->hashlink) {
			/* iterate over all class entries */

			for (clsen = nmen->classes; clsen; clsen = clsen->next) {
				c = clsen->classobj;

				if (!c)
					continue;

				/* exceptions are catched with new_exception call */

				if (!c->linked)
					(void) link_class(c);

				/* find overloaded methods */

				for (i = 0; i < c->methodscount; i++) {
					m = &(c->methods[i]);

					if (!(m->flags & ACC_NATIVE))
						continue;

					if (!m->nativelyoverloaded) {
						nativelyoverloaded = false;
				
						for (j = i + 1; j < c->methodscount; j++) {
							m2 = &(c->methods[j]);

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
		}
	}

	/* create table of native-methods */

	file = stdout;

	fprintf(file, "/* This file is machine generated, don't edit it! */\n\n"); 

	m = chain_first(nativemethod_chain);

	while (m) {
		printmethod(m);
		m = chain_next(nativemethod_chain);
	}

	fprintf(file, "static nativeref nativetable[] = {\n");

	m = chain_first(nativemethod_chain);

	while (m) {
        fprintf(file, "   { \"");

		print_classname(m->class);
		fprintf(file, "\",\n     \"");
		utf_fprint(file, m->name);
		fprintf(file, "\",\n     \"");
		utf_fprint(file, m->descriptor);
		fprintf(file, "\",\n     ");

		if (m->flags & ACC_STATIC)
			fprintf(file, "true");
		else
			fprintf(file, "false");

		fprintf(file, ",\n     ");
		fprintf(file, "(functionptr) Java_");
		printID(m->class->name);
		fprintf(file, "_");
		printID(m->name);
	 
		if (m->nativelyoverloaded)
			printOverloadPart(m->descriptor);

		fprintf(file,"\n   },\n");

		m = chain_next(nativemethod_chain);
	}

	chain_free(nativemethod_chain);
	chain_free(ident_chain);

	fprintf(file, "};\n");

	fclose(file);
	
	/* release all resources */

	loader_close();

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
