/* src/vm/primitivecore.c - core functions for primitive types

   Copyright (C) 2007, 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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

*/


#include "config.h"

#include <assert.h>
#include <stdint.h>

#include "vm/class.h"
#include "vm/global.h"
#include "vm/globals.hpp"
#include "vm/options.h"
#include "vm/primitive.hpp"
#include "vm/utf8.h"
#include "vm/vm.hpp"


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

   Fill the primitive type table with the primitive-type classes,
   array-classes and wrapper classes.  This is important in the VM
   startup.

   We split this primitive-type table initialization because of
   annotations in the bootstrap classes.

   But we may get a problem if we have annotations in:

   java/lang/Object
   java/lang/Cloneable
   java/io/Serializable

   Also see: loader_preinit and linker_preinit.

*******************************************************************************/

void primitive_init(void)
{  
	utf       *name;
	classinfo *c;
	utf       *u;
	classinfo *ac;
	int        i;

	TRACESUBSYSTEMINITIALIZATION("primitive_init");

	/* Load and link primitive-type classes and array-classes. */

	for (i = 0; i < PRIMITIVETYPE_COUNT; i++) {
		/* Skip dummy entries. */

		if (primitivetype_table[i].cname == NULL)
			continue;

		/* create UTF-8 name */

		name = utf_new_char(primitivetype_table[i].cname);

		primitivetype_table[i].name = name;

		/* create primitive class */

		c = class_create_classinfo(name);

		/* Primitive type classes don't have a super class. */

		c->super = NULL;

		/* set flags and mark it as primitive class */

		c->flags = ACC_PUBLIC | ACC_FINAL | ACC_ABSTRACT | ACC_CLASS_PRIMITIVE;
		
		/* prevent loader from loading primitive class */

		c->state |= CLASS_LOADED;

		/* INFO: don't put primitive classes into the classcache */

		if (!link_class(c))
			vm_abort("linker_init: linking failed");

		/* Just to be sure. */

		assert(c->state & CLASS_LOADED);
		assert(c->state & CLASS_LINKED);

		primitivetype_table[i].class_primitive = c;

		/* Create primitive array class. */

		if (primitivetype_table[i].arrayname != NULL) {
			u  = utf_new_char(primitivetype_table[i].arrayname);
			ac = class_create_classinfo(u);
			ac = load_newly_created_array(ac, NULL);

			if (ac == NULL)
				vm_abort("primitive_init: loading failed");

			assert(ac->state & CLASS_LOADED);

			if (!link_class(ac))
				vm_abort("primitive_init: linking failed");

			/* Just to be sure. */

			assert(ac->state & CLASS_LOADED);
			assert(ac->state & CLASS_LINKED);

			primitivetype_table[i].arrayclass = ac;
		}
	}

	/* We use two for-loops to have the array-classes already in the
	   primitive-type table (hint: annotations in wrapper-classes). */

	for (i = 0; i < PRIMITIVETYPE_COUNT; i++) {
		/* Skip dummy entries. */

		if (primitivetype_table[i].cname == NULL)
			continue;

		/* Create class for wrapping the primitive type. */

		u = utf_new_char(primitivetype_table[i].wrapname);
		c = load_class_bootstrap(u);

		if (c == NULL)
			vm_abort("primitive_init: loading failed");

		if (!link_class(c))
			vm_abort("primitive_init: linking failed");

		/* Just to be sure. */

		assert(c->state & CLASS_LOADED);
		assert(c->state & CLASS_LINKED);

		primitivetype_table[i].class_wrap = c;
	}
}


/* primitive_postinit **********************************************************

   Finish the primitive-type table initialization.  In this step we
   set the vftbl of the primitive-type classes.

   This is necessary because java/lang/Class is loaded and linked
   after the primitive types have been linked.

   We have to do that in an extra function, as the primitive types are
   not stored in the classcache.

*******************************************************************************/

void primitive_postinit(void)
{
	classinfo *c;
	int        i;

	TRACESUBSYSTEMINITIALIZATION("primitive_postinit");

	assert(class_java_lang_Class);
	assert(class_java_lang_Class->vftbl);

	for (i = 0; i < PRIMITIVETYPE_COUNT; i++) {
		/* Skip dummy entries. */

		if (primitivetype_table[i].cname == NULL)
			continue;

		c = primitivetype_table[i].class_primitive;

		c->object.header.vftbl = class_java_lang_Class->vftbl;
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
 * vim:noexpandtab:sw=4:ts=4:
 */
