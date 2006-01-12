/* src/vm/properties.c - handling commandline properties

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

   $Id: properties.c 4145 2006-01-12 21:06:07Z twisti $

*/


#include "config.h"

#include <stdlib.h>

#include "vm/types.h"

#include "vm/global.h"
#include "native/include/java_lang_String.h"
#include "native/include/java_util_Properties.h"
#include "toolbox/list.h"
#include "vm/method.h"
#include "vm/options.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"


/* temporary property structure ***********************************************/

typedef struct list_properties_entry list_properties_entry;

struct list_properties_entry {
	char     *key;
	char     *value;
	listnode linkage;
};


/* global variables ***********************************************************/

static list *list_properties = NULL;

static java_util_Properties *psystem;
static methodinfo *mput;


/* properties_init *************************************************************

   Initialize the properties list.

*******************************************************************************/

bool properties_init(void)
{
	list_properties = NEW(list);

	list_init(list_properties, OFFSET(list_properties_entry, linkage));

	/* everything's ok */

	return true;
}


/* properties_postinit *********************************************************

   Post-initialize the properties.  The passed properties table is the
   Java system properties table.

*******************************************************************************/

bool properties_postinit(java_util_Properties *p)
{
	/* set global Java system properties pointer */

	psystem = p;

	/* search for method to add properties */

	mput = class_resolveclassmethod(p->header.vftbl->class,
									utf_new_char("put"),
									utf_new_char("(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;"),
									NULL,
									true);

	if (mput == NULL)
		return false;

	return true;
}


/* properties_add **************************************************************

   Adds a property entry for a command line property definition.

*******************************************************************************/

void properties_add(char *key, char *value)
{
	list_properties_entry *p;

	p = NEW(list_properties_entry);
	p->key   = key;
	p->value = value;

	list_addlast(list_properties, p);
}


/* get_property ****************************************************************

   Get a property entry from a command line property definition.

*******************************************************************************/

char *properties_get(char *key)
{
	list_properties_entry *p;

	for (p = list_first(list_properties); p != NULL;
		 p = list_next(list_properties, p)) {
		if (strcmp(p->key, key) == 0)
			return p->value;
	}

	return NULL;
}


/* properties_system_add *******************************************************

   Adds a property to the Java system properties.

*******************************************************************************/

void properties_system_add(char *key, char *value)
{
	java_lang_String *k;
	java_lang_String *v;

	k = javastring_new_char(key);
	v = javastring_new_char(value);

	ASM_CALLJAVAFUNCTION(mput, psystem, k, v, NULL);
}


/* properties_system_add_all ***************************************************

   Adds a all properties from the properties list to the Java system
   properties.

*******************************************************************************/

void properties_system_add_all(void)
{
	list_properties_entry *p;

	for (p = list_first(list_properties); p != NULL;
		 p = list_first(list_properties)) {
		/* add to the Java system properties */

		properties_system_add(p->key, p->value);

		/* remove the entry from the list */

		list_remove(list_properties, p);

		/* and free the memory */

		FREE(p, list_properties_entry);
	}
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
