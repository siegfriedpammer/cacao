/* src/vmcore/field.c - field functions

   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
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

   $Id: field.c 8249 2007-07-31 12:59:03Z panzi $

*/


#include "config.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "mm/memory.h"

#include "vm/types.h"

#include "vm/exceptions.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"

#include "vmcore/annotation.h"
#include "vmcore/class.h"
#include "vmcore/descriptor.h"
#include "vmcore/field.h"
#include "vmcore/loader.h"
#include "vmcore/options.h"
#include "vmcore/primitive.h"
#include "vmcore/references.h"
#include "vmcore/suck.h"
#include "vmcore/utf8.h"


/* field_load ******************************************************************

   Load everything about a class field from the class file and fill a
   fieldinfo structure.

*******************************************************************************/

#define field_load_NOVALUE  0xffffffff /* must be bigger than any u2 value! */

bool field_load(classbuffer *cb, fieldinfo *f, descriptor_pool *descpool)
{
	classinfo *c;
	u4 attrnum, i;
	u4 jtype;
	u4 pindex = field_load_NOVALUE;     /* constantvalue_index */
	utf *u;

	c = cb->class;

	if (!suck_check_classbuffer_size(cb, 2 + 2 + 2))
		return false;

	f->flags = suck_u2(cb);

	if (!(u = class_getconstant(c, suck_u2(cb), CONSTANT_Utf8)))
		return false;

	f->name = u;

	if (!(u = class_getconstant(c, suck_u2(cb), CONSTANT_Utf8)))
		return false;

	f->descriptor = u;
	f->parseddesc = NULL;

	if (!descriptor_pool_add(descpool, u, NULL))
		return false;

	/* descriptor_pool_add accepts method descriptors, so we have to
	   check against them here before the call of
	   descriptor_to_basic_type below. */

	if (u->text[0] == '(') {
		exceptions_throw_classformaterror(c, "Method descriptor used for field");
		return false;
	}

#ifdef ENABLE_VERIFIER
	if (opt_verify) {
		/* check name */
		if (!is_valid_name_utf(f->name) || f->name->text[0] == '<') {
			exceptions_throw_classformaterror(c,
											  "Illegal Field name \"%s\"",
											  f->name->text);
			return false;
		}

		/* check flag consistency */
		i = f->flags & (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED);

		if ((i != 0 && i != ACC_PUBLIC && i != ACC_PRIVATE && i != ACC_PROTECTED) ||
			((f->flags & (ACC_FINAL | ACC_VOLATILE)) == (ACC_FINAL | ACC_VOLATILE))) {
			exceptions_throw_classformaterror(c,
											  "Illegal field modifiers: 0x%X",
											  f->flags);
			return false;
		}

		if (c->flags & ACC_INTERFACE) {
			if (((f->flags & (ACC_STATIC | ACC_PUBLIC | ACC_FINAL))
				!= (ACC_STATIC | ACC_PUBLIC | ACC_FINAL)) ||
				f->flags & ACC_TRANSIENT) {
				exceptions_throw_classformaterror(c,
												  "Illegal field modifiers: 0x%X",
												  f->flags);
				return false;
			}
		}
	}
#endif /* ENABLE_VERIFIER */

	/* data type */

	jtype = descriptor_to_basic_type(f->descriptor);

	f->class  = c;
	f->type   = jtype;
	f->offset = 0;                             /* offset from start of object */

	switch (f->type) {
	case TYPE_INT:
		f->value.i = 0;
		break;

	case TYPE_FLT:
		f->value.f = 0.0;
		break;

	case TYPE_DBL:
		f->value.d = 0.0;
		break;

	case TYPE_ADR:
		f->value.a = NULL;
		if (!(f->flags & ACC_STATIC))
			c->flags |= ACC_CLASS_HAS_POINTERS;
		break;

	case TYPE_LNG:
#if U8_AVAILABLE
		f->value.l = 0;
#else
		f->value.l.low  = 0;
		f->value.l.high = 0;
#endif
		break;
	}

	/* read attributes */
	if (!suck_check_classbuffer_size(cb, 2))
		return false;

	attrnum = suck_u2(cb);
	for (i = 0; i < attrnum; i++) {
		if (!suck_check_classbuffer_size(cb, 2))
			return false;

		if (!(u = class_getconstant(c, suck_u2(cb), CONSTANT_Utf8)))
			return false;

		if (u == utf_ConstantValue) {
			if (!suck_check_classbuffer_size(cb, 4 + 2))
				return false;

			/* check attribute length */

			if (suck_u4(cb) != 2) {
				exceptions_throw_classformaterror(c, "Wrong size for VALUE attribute");
				return false;
			}
			
			/* constant value attribute */

			if (pindex != field_load_NOVALUE) {
				exceptions_throw_classformaterror(c, "Multiple ConstantValue attributes");
				return false;
			}
			
			/* index of value in constantpool */

			pindex = suck_u2(cb);
		
			/* initialize field with value from constantpool */		

			switch (jtype) {
			case TYPE_INT: {
				constant_integer *ci; 

				if (!(ci = class_getconstant(c, pindex, CONSTANT_Integer)))
					return false;

				f->value.i = ci->value;
			}
			break;
					
			case TYPE_LNG: {
				constant_long *cl; 

				if (!(cl = class_getconstant(c, pindex, CONSTANT_Long)))
					return false;

				f->value.l = cl->value;
			}
			break;

			case TYPE_FLT: {
				constant_float *cf;

				if (!(cf = class_getconstant(c, pindex, CONSTANT_Float)))
					return false;

				f->value.f = cf->value;
			}
			break;
											
			case TYPE_DBL: {
				constant_double *cd;

				if (!(cd = class_getconstant(c, pindex, CONSTANT_Double)))
					return false;

				f->value.d = cd->value;
			}
			break;
						
			case TYPE_ADR:
				if (!(u = class_getconstant(c, pindex, CONSTANT_String)))
					return false;

				/* create javastring from compressed utf8-string */
				f->value.a = literalstring_new(u);
				break;
	
			default: 
				vm_abort("field_load: invalid field type %d", jtype);
			}
		}
#if defined(ENABLE_JAVASE)
		else if (u == utf_Signature) {
			/* Signature */

			if (!loader_load_attribute_signature(cb, &(f->signature)))
				return false;
		}

#if defined(ENABLE_ANNOTATIONS)
		else if (u == utf_RuntimeVisibleAnnotations) {
			/* RuntimeVisibleAnnotations */
			if (!annotation_load_field_attribute_runtimevisibleannotations(cb, f))
				return false;
		}
		else if (u == utf_RuntimeInvisibleAnnotations) {
			/* RuntimeInvisibleAnnotations */
			if (!annotation_load_field_attribute_runtimeinvisibleannotations(cb, f))
				return false;
		}
#endif
#endif
		else {
			/* unknown attribute */

			if (!loader_skip_attribute_body(cb))
				return false;
		}
	}

	/* everything was ok */

	return true;
}


/* field_get_type **************************************************************

   Returns the type of the field as class.

*******************************************************************************/

classinfo *field_get_type(fieldinfo *f)
{
	typedesc  *td;
	utf       *u;
	classinfo *c;

	td = f->parseddesc;

	if (td->type == TYPE_ADR) {
		assert(td->classref);

		u = td->classref->name;

		/* load the class of the field-type with the field's
		   classloader */

		c = load_class_from_classloader(u, f->class->classloader);
	}
	else {
		c = primitive_class_get_by_type(td->decltype);
	}

	return c;
}


/* field_free ******************************************************************

   Frees a fields' resources.

*******************************************************************************/

void field_free(fieldinfo *f)
{
	/* empty */
}


#if defined(ENABLE_ANNOTATIONS)
/* field_get_annotations ******************************************************

   Gets a fields' annotations (or NULL if none).

*******************************************************************************/

annotation_bytearray_t *field_get_annotations(fieldinfo *f)
{
	classinfo *c = f->class;
	int slot = f - c->fields;

	if (c->field_annotations != NULL &&
	    c->field_annotations->size > slot) {
		return c->field_annotations->data[slot];
	}

	return NULL;
}
#endif


/* field_printflags ************************************************************

   (debugging only)

*******************************************************************************/

#if !defined(NDEBUG)
void field_printflags(fieldinfo *f)
{
	if (f == NULL) {
		printf("NULL");
		return;
	}

	if (f->flags & ACC_PUBLIC)       printf(" PUBLIC");
	if (f->flags & ACC_PRIVATE)      printf(" PRIVATE");
	if (f->flags & ACC_PROTECTED)    printf(" PROTECTED");
	if (f->flags & ACC_STATIC)       printf(" STATIC");
	if (f->flags & ACC_FINAL)        printf(" FINAL");
	if (f->flags & ACC_SYNCHRONIZED) printf(" SYNCHRONIZED");
	if (f->flags & ACC_VOLATILE)     printf(" VOLATILE");
	if (f->flags & ACC_TRANSIENT)    printf(" TRANSIENT");
	if (f->flags & ACC_NATIVE)       printf(" NATIVE");
	if (f->flags & ACC_INTERFACE)    printf(" INTERFACE");
	if (f->flags & ACC_ABSTRACT)     printf(" ABSTRACT");
}
#endif


/* field_print *****************************************************************

   (debugging only)

*******************************************************************************/

#if !defined(NDEBUG)
void field_print(fieldinfo *f)
{
	if (f == NULL) {
		printf("(fieldinfo*)NULL");
		return;
	}

	utf_display_printable_ascii_classname(f->class->name);
	printf(".");
	utf_display_printable_ascii(f->name);
	printf(" ");
	utf_display_printable_ascii(f->descriptor);	

	field_printflags(f);
}
#endif


/* field_println ***************************************************************

   (debugging only)

*******************************************************************************/

#if !defined(NDEBUG)
void field_println(fieldinfo *f)
{
	field_print(f);
	printf("\n");
}
#endif

/* field_fieldref_print ********************************************************

   (debugging only)

*******************************************************************************/

#if !defined(NDEBUG)
void field_fieldref_print(constant_FMIref *fr)
{
	if (fr == NULL) {
		printf("(constant_FMIref *)NULL");
		return;
	}

	if (IS_FMIREF_RESOLVED(fr)) {
		printf("<field> ");
		field_print(fr->p.field);
	}
	else {
		printf("<fieldref> ");
		utf_display_printable_ascii_classname(fr->p.classref->name);
		printf(".");
		utf_display_printable_ascii(fr->name);
		printf(" ");
		utf_display_printable_ascii(fr->descriptor);
	}
}
#endif

/* field_fieldref_println ******************************************************

   (debugging only)

*******************************************************************************/

#if !defined(NDEBUG)
void field_fieldref_println(constant_FMIref *fr)
{
	field_fieldref_print(fr);
	printf("\n");
}
#endif

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
