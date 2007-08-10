/* src/vm/primitive.c - primitive types

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


/* primitive_class_get_by_name *************************************************

   Returns the primitive class of the given class name.

*******************************************************************************/

classinfo *primitive_class_get_by_name(utf *name)
{
	int i;

	/* search table of primitive classes */

	for (i = 0; i < PRIMITIVETYPE_COUNT; i++)
		if (primitivetype_table[i].name == name)
			return primitivetype_table[i].class_primitive;

	/* keep compiler happy */

	return NULL;
}


/* primitive_class_get_by_type *************************************************

   Returns the primitive class of the given type.

*******************************************************************************/

classinfo *primitive_class_get_by_type(int type)
{
	return primitivetype_table[type].class_primitive;
}


/* primitive_class_get_by_char *************************************************

   Returns the primitive class of the given type.

*******************************************************************************/

classinfo *primitive_class_get_by_char(char ch)
{
	int index;

	switch (ch) {
	case 'I':
		index = PRIMITIVETYPE_INT;
		break;
	case 'J':
		index = PRIMITIVETYPE_LONG;
		break;
	case 'F':
		index = PRIMITIVETYPE_FLOAT;
		break;
	case 'D':
		index = PRIMITIVETYPE_DOUBLE;
		break;
	case 'B':
		index = PRIMITIVETYPE_BYTE;
		break;
	case 'C':
		index = PRIMITIVETYPE_CHAR;
		break;
	case 'S':
		index = PRIMITIVETYPE_SHORT;
		break;
	case 'Z':
		index = PRIMITIVETYPE_BOOLEAN;
		break;
	case 'V':
		index = PRIMITIVETYPE_VOID;
		break;
	default:
		return NULL;
	}

	return primitivetype_table[index].class_primitive;
}


/* primitive_arrayclass_get_by_name ********************************************

   Returns the primitive array-class of the given primitive class
   name.

*******************************************************************************/

classinfo *primitive_arrayclass_get_by_name(utf *name)
{
	int i;

	/* search table of primitive classes */

	for (i = 0; i < PRIMITIVETYPE_COUNT; i++)
		if (primitivetype_table[i].name == name)
			return primitivetype_table[i].arrayclass;

	/* keep compiler happy */

	return NULL;
}


/* primitive_arrayclass_get_by_type ********************************************

   Returns the primitive array-class of the given type.

*******************************************************************************/

classinfo *primitive_arrayclass_get_by_type(int type)
{
	return primitivetype_table[type].arrayclass;
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
