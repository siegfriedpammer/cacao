/* src/vmcore/primitivecore.c - core functions for primitive types

   Copyright (C) 2007 R. Grafl, A. Krall, C. Kruegel,
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

   $Id: linker.c 8042 2007-06-07 17:43:29Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <stdint.h>

#include "vm/global.h"
#include "vm/primitive.h"

#include "vmcore/class.h"
#include "vmcore/utf8.h"


/* primitivetype_table *********************************************************

   Structure for primitive classes: contains the class for wrapping
   the primitive type, the primitive class, the name of the class for
   wrapping, the one character type signature and the name of the
   primitive class.
 
   CAUTION: Don't change the order of the types. This table is indexed
   by the ARRAYTYPE_ constants (except ARRAYTYPE_OBJECT).

*******************************************************************************/

primitivetypeinfo primitivetype_table[PRIMITIVETYPE_COUNT] = {
	{ "int"     , NULL, NULL, NULL, "java/lang/Integer",   'I', "[I", NULL },
	{ "long"    , NULL, NULL, NULL, "java/lang/Long",      'J', "[J", NULL },
	{ "float"   , NULL, NULL, NULL, "java/lang/Float",     'F', "[F", NULL },
	{ "double"  , NULL, NULL, NULL, "java/lang/Double",    'D', "[D", NULL },
	{ NULL      , NULL, NULL, NULL, NULL,                   0 , NULL, NULL },
	{ "byte"    , NULL, NULL, NULL, "java/lang/Byte",      'B', "[B", NULL },
	{ "char"    , NULL, NULL, NULL, "java/lang/Character", 'C', "[C", NULL },
	{ "short"   , NULL, NULL, NULL, "java/lang/Short",     'S', "[S", NULL },
	{ "boolean" , NULL, NULL, NULL, "java/lang/Boolean",   'Z', "[Z", NULL },
	{ NULL      , NULL, NULL, NULL, NULL,                   0 , NULL, NULL },
#if defined(ENABLE_JAVASE)
   	{ "void"    , NULL, NULL, NULL, "java/lang/Void",      'V', NULL, NULL }
#else
	{ NULL      , NULL, NULL, NULL, NULL,                   0 , NULL, NULL },
#endif
};


/* primitive_init **************************************************************

   Create classes representing primitive types.

*******************************************************************************/

bool primitive_init(void)
{  
	utf       *name;
	classinfo *c;
	utf       *u;
	int        i;

	for (i = 0; i < PRIMITIVETYPE_COUNT; i++) {
		/* skip dummies */

		if (primitivetype_table[i].cname == NULL)
			continue;

		/* create UTF-8 name */

		name = utf_new_char(primitivetype_table[i].cname);

		primitivetype_table[i].name = name;

		/* create primitive class */

		c = class_create_classinfo(name);

		/* primitive classes don't have a super class */

		c->super.any = NULL;

		/* set flags and mark it as primitive class */

		c->flags = ACC_PUBLIC | ACC_FINAL | ACC_ABSTRACT | ACC_CLASS_PRIMITIVE;
		
		/* prevent loader from loading primitive class */

		c->state |= CLASS_LOADED;

		/* INFO: don't put primitive classes into the classcache */

		if (!link_class(c))
			return false;

		primitivetype_table[i].class_primitive = c;

		/* create class for wrapping the primitive type */

		u = utf_new_char(primitivetype_table[i].wrapname);
		c = load_class_bootstrap(u);

		if (c == NULL)
			return false;

		primitivetype_table[i].class_wrap = c;

		/* create the primitive array class */

		if (primitivetype_table[i].arrayname) {
			u = utf_new_char(primitivetype_table[i].arrayname);
			c = class_create_classinfo(u);
			c = load_newly_created_array(c, NULL);

			if (c == NULL)
				return false;

			primitivetype_table[i].arrayclass = c;

			assert(c->state & CLASS_LOADED);

			if (!(c->state & CLASS_LINKED))
				if (!link_class(c))
					return false;
		}
	}

	return true;
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
 * vim:noexpandtab:sw=4:ts=4:
 */