/* src/vm/field.cpp - field functions

   Copyright (C) 1996-2005, 2006, 2007, 2008, 2010
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
#include <stdio.h>

#include "mm/memory.hpp"

#include "native/llni.hpp"

#include "vm/types.h"

#include "vm/annotation.hpp"
#include "vm/array.hpp"
#include "vm/jit/builtin.hpp"
#include "vm/class.hpp"
#include "vm/descriptor.hpp"
#include "vm/exceptions.hpp"
#include "vm/field.hpp"
#include "vm/global.h"
#include "vm/globals.hpp"
#include "vm/loader.hpp"
#include "vm/options.hpp"
#include "vm/primitive.hpp"
#include "vm/references.h"
#include "vm/string.hpp"
#include "vm/suck.hpp"
#include "vm/utf8.hpp"
#include "vm/vm.hpp"


/* field_load ******************************************************************

   Load everything about a class field from the class file and fill a
   fieldinfo structure.

*******************************************************************************/

#define field_load_NOVALUE  0xffffffff /* must be bigger than any u2 value! */

bool field_load(classbuffer *cb, fieldinfo *f, descriptor_pool *descpool)
{
	classinfo *c;
	u4 attrnum, i;
	u4 pindex = field_load_NOVALUE;     /* constantvalue_index */
	Utf8String u;

	/* Get class. */

	c = cb->clazz;

	f->clazz = c;

	/* Get access flags. */

	if (!suck_check_classbuffer_size(cb, 2 + 2 + 2))
		return false;

	f->flags = suck_u2(cb);

	/* Get name. */

	if (!(u = (utf*) class_getconstant(c, suck_u2(cb), CONSTANT_Utf8)))
		return false;

	f->name = u;

	/* Get descriptor. */

	if (!(u = (utf*) class_getconstant(c, suck_u2(cb), CONSTANT_Utf8)))
		return false;

	f->descriptor = u;
	f->parseddesc = NULL;

	if (!descriptor_pool_add(descpool, u, NULL))
		return false;

	/* descriptor_pool_add accepts method descriptors, so we have to
	   check against them here before the call of
	   descriptor_to_basic_type below. */

	if (UTF_AT(u, 0) == '(') {
		exceptions_throw_classformaterror(c, "Method descriptor used for field");
		return false;
	}

#ifdef ENABLE_VERIFIER
	if (opt_verify) {
		/* check name */
		if (!Utf8String(f->name).is_valid_name() || UTF_AT(f->name, 0) == '<') {
			exceptions_throw_classformaterror(c,
			                                  "Illegal Field name \"%s\"",
			                                  UTF_TEXT(f->name));
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

	f->type = descriptor_to_basic_type(f->descriptor);

	/* For static-fields allocate memory for the value and set the
	   value to 0. */

	if (f->flags & ACC_STATIC) {
		switch (f->type) {
		case TYPE_INT:
		case TYPE_LNG:
		case TYPE_FLT:
		case TYPE_DBL:
			f->value = NEW(imm_union);
			break;

		case TYPE_ADR:
#if !defined(ENABLE_GC_BOEHM)
			f->value = NEW(imm_union);
#else
			f->value = GCNEW_UNCOLLECTABLE(imm_union, 1);
#endif
			break;

		default:
			vm_abort("field_load: invalid field type %d", f->type);
		}

		/* Set the field to zero, for float and double fields set the
		   correct 0.0 value. */

		switch (f->type) {
		case TYPE_INT:
		case TYPE_LNG:
		case TYPE_ADR:
			f->value->l = 0;
			break;

		case TYPE_FLT:
			f->value->f = 0.0;
			break;

		case TYPE_DBL:
			f->value->d = 0.0;
			break;
		}
	}
	else {
		/* For instance-fields set the offset to 0. */

		f->offset = 0;

		/* For final fields, which are not static, we need a value
		   structure. */

		if (f->flags & ACC_FINAL) {
			f->value = NEW(imm_union);
			/* XXX hack */
			f->value->l = 0;
		}

		switch (f->type) {
		case TYPE_ADR:
			c->flags |= ACC_CLASS_HAS_POINTERS;
			break;
		}
	}

	/* read attributes */

	if (!suck_check_classbuffer_size(cb, 2))
		return false;

	attrnum = suck_u2(cb);

	for (i = 0; i < attrnum; i++) {
		if (!suck_check_classbuffer_size(cb, 2))
			return false;

		if (!(u = (utf*) class_getconstant(c, suck_u2(cb), CONSTANT_Utf8)))
			return false;

		if (u == utf8::ConstantValue) {
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

			switch (f->type) {
			case TYPE_INT: {
				constant_integer *ci; 

				if (!(ci = (constant_integer*) class_getconstant(c, pindex, CONSTANT_Integer)))
					return false;

				f->value->i = ci->value;
			}
			break;

			case TYPE_LNG: {
				constant_long *cl; 

				if (!(cl = (constant_long*) class_getconstant(c, pindex, CONSTANT_Long)))
					return false;

				f->value->l = cl->value;
			}
			break;

			case TYPE_FLT: {
				constant_float *cf;

				if (!(cf = (constant_float*) class_getconstant(c, pindex, CONSTANT_Float)))
					return false;

				f->value->f = cf->value;
			}
			break;

			case TYPE_DBL: {
				constant_double *cd;

				if (!(cd = (constant_double*) class_getconstant(c, pindex, CONSTANT_Double)))
					return false;

				f->value->d = cd->value;
			}
			break;

			case TYPE_ADR:
				if (!(u = (utf*) class_getconstant(c, pindex, CONSTANT_String)))
					return false;

				/* Create Java-string from compressed UTF8-string. */

				if (!(class_java_lang_String->flags & CLASS_LINKED))
					linker_create_string_later(reinterpret_cast<java_object_t**>(&f->value->a), u);
				else
					f->value->a = JavaString::literal(u);
				break;

			default: 
				vm_abort("field_load: invalid field type %d", f->type);
			}
		}
#if defined(ENABLE_JAVASE)
		else if (u == utf8::Signature) {
			/* Signature */

			if (!loader_load_attribute_signature(cb, &(f->signature)))
				return false;
		}

#if defined(ENABLE_ANNOTATIONS)
		else if (u == utf8::RuntimeVisibleAnnotations) {
			/* RuntimeVisibleAnnotations */
			if (!annotation_load_field_attribute_runtimevisibleannotations(cb, f))
				return false;
		}
		else if (u == utf8::RuntimeInvisibleAnnotations) {
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

		c = load_class_from_classloader(u, f->clazz->classloader);
	}
	else {
		c = Primitive::get_class_by_type(td->primitivetype);
	}

	return c;
}


/* field_free ******************************************************************

   Frees a fields' resources.

*******************************************************************************/

void field_free(fieldinfo *f)
{
	/* free memory for fields which have a value */

	if (f->value)
#if defined(ENABLE_GC_BOEHM)
		if (f->type != TYPE_ADR)
#endif
			FREE(f->value, imm_union);
}


/* field_get_annotations ******************************************************

   Get a fields' unparsed annotations in a byte array.

   IN:
       f........the field of which the annotations should be returned

   RETURN VALUE:
       The unparsed annotations in a byte array (or NULL if there aren't any).

*******************************************************************************/

java_handle_bytearray_t *field_get_annotations(fieldinfo *f)
{
#if defined(ENABLE_ANNOTATIONS)
	classinfo               *c;           /* declaring class           */
	int                      slot;        /* slot of this field        */
	java_handle_t           *field_annotations;  /* array of unparsed  */
	               /* annotations of all fields of the declaring class */

	c    = f->clazz;
	slot = f - c->fields;

	LLNI_classinfo_field_get(c, field_annotations, field_annotations);

	ObjectArray oa(field_annotations);

	/* the field_annotations array might be shorter then the field
	 * count if the fields above a certain index have no annotations.
	 */
	if (field_annotations != NULL && oa.get_length() > slot) {
		return (java_handle_bytearray_t*) oa.get_element(slot);
	} else {
		return NULL;
	}
#else
	return NULL;
#endif
}


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

	utf_display_printable_ascii_classname(f->clazz->name);
	printf(".");
	utf_display_printable_ascii(f->name);
	printf(" ");
	utf_display_printable_ascii(f->descriptor);	

	field_printflags(f);

	if (!(f->flags & ACC_STATIC)) {
		printf(", offset: %d", f->offset);
	}
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
 * vim:noexpandtab:sw=4:ts=4:
 */
