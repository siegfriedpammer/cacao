/* src/vm/jit/profile.c - runtime profiling

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, J. Wenninger, Institut f. Computersprachen - TU Wien

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

   Changes:

   $Id: cacao.c 4357 2006-01-22 23:33:38Z twisti $

*/


#include "config.h"

#include <assert.h>

#include "vm/types.h"

#include "mm/memory.h"
#include "vm/class.h"
#include "vm/classcache.h"
#include "vm/method.h"
#include "vm/options.h"


/* list_method_entry **********************************************************/

typedef struct list_method_entry list_method_entry;

struct list_method_entry {
	methodinfo *m;
	listnode    linkage;
};


/* profile_printstats **********************************************************

   Prints profiling statistics gathered during runtime.

*******************************************************************************/

#if !defined(NDEBUG)
void profile_printstats(void)
{
	list                   *l;
	list_method_entry      *lme;
	list_method_entry      *tlme;
	classinfo              *c;
	methodinfo             *m;
	u4                      slot;
	classcache_name_entry  *nmen;
	classcache_class_entry *clsen;
	s4                      i;
	s4                      j;
	u4                      frequency;
	s8                      cycles;

	frequency = 0;
	cycles    = 0;

	/* create new method list */

	l = NEW(list);

	list_init(l, OFFSET(list_method_entry, linkage));

	/* iterate through all classes and methods */

	for (slot = 0; slot < hashtable_classcache.size; slot++) {
		nmen = (classcache_name_entry *) hashtable_classcache.ptr[slot];

		for (; nmen; nmen = nmen->hashlink) {
			/* iterate over all class entries */

			for (clsen = nmen->classes; clsen; clsen = clsen->next) {
				c = clsen->classobj;

				if (!c)
					continue;

				/* compile all class methods */

				for (i = 0; i < c->methodscount; i++) {
					m = &(c->methods[i]);

					/* was this method actually called? */

					if (m->frequency > 0) {
						/* add to overall stats */

						frequency += m->frequency;
						cycles    += m->cycles;

						/* create new list entry */

						lme = NEW(list_method_entry);
						lme->m = m;

						/* sort the new entry into the list */
						
						if ((tlme = list_first(l)) == NULL) {
							list_addfirst(l, lme);

						} else {
							for (; tlme != NULL; tlme = list_next(l, tlme)) {
								/* check the frequency */

								if (m->frequency > tlme->m->frequency) {
									list_add_before(l, tlme, lme);
									break;
								}
							}

							/* if we are at the end of the list, add
							   it at last */

							if (tlme == NULL)
								list_addlast(l, lme);
						}
					}
				}
			}
		}
	}

	/* print all methods sorted */

	printf(" frequency     ratio         cycles     ratio   method name\n");
	printf("----------- --------- -------------- --------- -------------\n");

	/* now iterate through the list and print it */

	for (lme = list_first(l); lme != NULL; lme = list_next(l, lme)) {
		/* get method of the list element */

		m = lme->m;

		printf("%10d   %.5f   %12ld   %.5f   ",
			   m->frequency,
			   (double) m->frequency / (double) frequency,
			   m->cycles,
			   (double) m->cycles / (double) cycles);

		method_println(m);

		/* print basic block frequencies */

		if (opt_prof_bb) {
			for (j = 0; j < m->basicblockcount; j++)
				printf("                                                    L%03d: %10d\n",
					   j, m->bbfrequency[j]);
		}
	}

	printf("-----------           -------------- \n");
	printf("%10d             %12ld\n", frequency, cycles);
}
#endif /* !defined(NDEBUG) */


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
