/* src/vmcore/annotation.c - class annotations

   Copyright (C) 2006, 2007 R. Grafl, A. Krall, C. Kruegel, C. Oates,
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/


#include "config.h"

#include <assert.h>
#include <stdint.h>

#include "native/llni.h"

#include "vm/types.h"
#include "vm/array.h"
#include "vm/builtin.h"
#include "vm/primitive.h"

#include "mm/memory.h"

#include "toolbox/logging.h"

#include "vmcore/annotation.h"
#include "vmcore/class.h"
#include "vmcore/suck.h"

#if !defined(ENABLE_ANNOTATIONS)
# error annotation support has to be enabled when compling this file!
#endif


/* annotation_bytearrays_resize ***********************************************

   Resize an array of bytearrays.

*******************************************************************************/

static bool annotation_bytearrays_resize(java_handle_objectarray_t **bas,
	uint32_t size)
{
	java_handle_objectarray_t *newbas = NULL;
	uint32_t minsize;
	uint32_t oldsize;
	
	assert(bas != NULL);
	
	if (*bas != NULL) {
		oldsize = array_length_get((java_handle_t*)*bas);

		/* if the size already fits do nothing */
		if (size == oldsize) {
			return true;
		}
	}

	newbas = builtin_anewarray(size,
		primitive_arrayclass_get_by_type(PRIMITIVETYPE_BYTE));
	
	if (newbas == NULL) {
		/* out of memory */
		return false;
	}
	
	/* is there a old byte array array? */
	if (*bas != NULL) {
		minsize = size < oldsize ? size : oldsize;

		MCOPY(LLNI_array_data(newbas), LLNI_array_data(*bas), java_object_t*, minsize);
	}

	*bas = newbas;

	return true;
}


/* annotation_bytearrays_insert ***********************************************

   Insert a bytearray into an array of bytearrays.

*******************************************************************************/

static bool annotation_bytearrays_insert(java_handle_objectarray_t **bas,
	uint32_t index, java_handle_bytearray_t *ba)
{
	uint32_t size = 0;

	assert(bas != NULL);

	/* do nothing if NULL is inserted but no array exists */
	if (ba == NULL && *bas == NULL) {
		return true;
	}

	/* get lengths if array exists */
	if (*bas != NULL) {
		size = array_length_get((java_handle_t*)*bas);
	}

	if (ba == NULL) {
		/* insert NULL only if array is big enough */
		if (size > index) {
			array_objectarray_element_set(*bas, index, NULL);
		}
	}
	else {
		/* resize array if it's not enough for inserted value */
		if (size <= index) {
			if (!annotation_bytearrays_resize(bas, index + 1)) {
				/* out of memory */
				return false;
			}
		}

		array_objectarray_element_set(*bas, index, (java_handle_t*)ba);
	}
	
	return true;
}


/* annotation_load_attribute_body *********************************************

   This function loads the body of a generic attribute.

   XXX: Maybe this function should be called loader_load_attribute_body and
        located in vmcore/loader.c?

   attribute_info {
       u2 attribute_name_index;
       u4 attribute_length;
       u1 info[attribute_length];
   }

   IN:
       cb.................classbuffer from which to read the data.
       errormsg_prefix....prefix for error messages (if any).

   OUT:
       attribute..........bytearray-pointer which will be set to the read data.
   
   RETURN VALUE:
       true if all went good. false otherwhise.

*******************************************************************************/

static bool annotation_load_attribute_body(classbuffer *cb,
	java_handle_bytearray_t **attribute, const char *errormsg_prefix)
{
	uint32_t size = 0;
	java_handle_bytearray_t *ba = NULL;

	assert(cb != NULL);
	assert(attribute != NULL);

	if (!suck_check_classbuffer_size(cb, 4)) {
		log_println("%s: size missing", errormsg_prefix);
		return false;
	}

	/* load attribute_length */
	size = suck_u4(cb);
	
	if (!suck_check_classbuffer_size(cb, size)) {
		log_println("%s: invalid size", errormsg_prefix);
		return false;
	}
	
	/* if attribute_length == 0 then NULL is
	 * the right value for this attribute */
	if (size > 0) {
		ba = builtin_newarray_byte(size);

		if (ba == NULL) {
			/* out of memory */
			return false;
		}

		/* load data */
		suck_nbytes((uint8_t*)LLNI_array_data(ba), cb, size);

		/* return data */
		*attribute = ba;
	}
	
	return true;
}


/* annotation_load_method_attribute_annotationdefault *************************

   Load annotation default value.

   AnnotationDefault_attribute {
       u2 attribute_name_index;
       u4 attribute_length;
       element_value default_value;
   }

   IN:
       cb.................classbuffer from which to read the data.
       m..................methodinfo for the method of which the annotation
                          default value is read and into which the value is
                          stored into.

   RETURN VALUE:
       true if all went good. false otherwhise.

*******************************************************************************/

bool annotation_load_method_attribute_annotationdefault(
		classbuffer *cb, methodinfo *m)
{
	int slot = 0;
	java_handle_bytearray_t    *annotationdefault  = NULL;
	java_handle_objectarray_t **annotationdefaults = NULL;

	assert(cb != NULL);
	assert(m != NULL);

	annotationdefaults = &(m->class->method_annotationdefaults);

	if (!annotation_load_attribute_body(
			cb, &annotationdefault,
			"invalid annotation default method attribute")) {
		return false;
	}

	if (annotationdefault != NULL) {
		slot = m - m->class->methods;

		if (!annotation_bytearrays_insert(
				annotationdefaults, slot, annotationdefault)) {
			return false;
		}
	}

	return true;
}


/* annotation_load_method_attribute_runtimevisibleparameterannotations ********

   Load runtime visible parameter annotations.

   RuntimeVisibleParameterAnnotations_attribute {
       u2 attribute_name_index;
       u4 attribute_length;
       u1 num_parameters;
       {
           u2 num_annotations;
           annotation annotations[num_annotations];
       } parameter_annotations[num_parameters];
   }

   IN:
       cb.................classbuffer from which to read the data.
       m..................methodinfo for the method of which the parameter
                          annotations are read and into which the parameter
                          annotations are stored into.

   RETURN VALUE:
       true if all went good. false otherwhise.

*******************************************************************************/

bool annotation_load_method_attribute_runtimevisibleparameterannotations(
		classbuffer *cb, methodinfo *m)
{
	int slot = 0;
	java_handle_bytearray_t    *annotations          = NULL;
	java_handle_objectarray_t **parameterannotations = NULL;

	assert(cb != NULL);
	assert(m != NULL);

	parameterannotations = &(m->class->method_parameterannotations);

	if (!annotation_load_attribute_body(
			cb, &annotations,
			"invalid runtime visible parameter annotations method attribute")) {
		return false;
	}

	if (annotations != NULL) {
		slot = m - m->class->methods;

		if (!annotation_bytearrays_insert(
				parameterannotations, slot, annotations)) {
			return false;
		}
	}

	return true;
}


/* annotation_load_method_attribute_runtimeinvisibleparameterannotations ******
 
   Load runtime invisible parameter annotations.

   <quote cite="http://jcp.org/en/jsr/detail?id=202">
   The RuntimeInvisibleParameterAnnotations attribute is similar to the
   RuntimeVisibleParameterAnnotations attribute, except that the annotations
   represented by a RuntimeInvisibleParameterAnnotations attribute must not be
   made available for return by reflective APIs, unless the the JVM has
   specifically been instructed to retain these annotations via some
   implementation-specific mechanism such as a command line flag. In the
   absence of such instructions, the JVM ignores this attribute.
   </quote>

   Hotspot loads them into the same bytearray as the runtime visible parameter
   annotations (after the runtime visible parameter annotations). But in J2SE
   the bytearray will only be parsed as if ther is only one annotation
   structure in it, so the runtime invisible parameter annotatios will be
   ignored.

   Therefore I do not even bother to read them.

   RuntimeInvisibleParameterAnnotations_attribute {
       u2 attribute_name_index;
       u4 attribute_length;
       u1 num_parameters;
       {
           u2 num_annotations;
           annotation annotations[num_annotations];
       } parameter_annotations[num_parameters];
   }

   IN:
       cb.................classbuffer from which to read the data.
       m..................methodinfo for the method of which the parameter
                          annotations are read and into which the parameter
                          annotations are stored into.

   RETURN VALUE:
       true if all went good. false otherwhise.

*******************************************************************************/

bool annotation_load_method_attribute_runtimeinvisibleparameterannotations(
		classbuffer *cb, methodinfo *m)
{
	return loader_skip_attribute_body(cb);
}


/* annotation_load_class_attribute_runtimevisibleannotations ******************
   
   Load runtime visible annotations of a class.
   
*******************************************************************************/

bool annotation_load_class_attribute_runtimevisibleannotations(
	classbuffer *cb)
{
	return annotation_load_attribute_body(
		cb, &(cb->class->annotations),
		"invalid runtime visible annotations class attribute");
}


/* annotation_load_class_attribute_runtimeinvisibleannotations ****************
   
   Load runtime invisible annotations of a class (just skip them).
   
*******************************************************************************/

bool annotation_load_class_attribute_runtimeinvisibleannotations(
	classbuffer *cb)
{
	return loader_skip_attribute_body(cb);
}


/* annotation_load_method_attribute_runtimevisibleannotations *****************
   
   Load runtime visible annotations of a method.
   
*******************************************************************************/

bool annotation_load_method_attribute_runtimevisibleannotations(
	classbuffer *cb, methodinfo *m)
{
	int slot = 0;
	java_handle_bytearray_t    *annotations        = NULL;
	java_handle_objectarray_t **method_annotations = NULL;

	assert(cb != NULL);
	assert(m != NULL);

	method_annotations = &(m->class->method_annotations);

	if (!annotation_load_attribute_body(
			cb, &annotations,
			"invalid runtime visible annotations method attribute")) {
		return false;
	}

	if (annotations != NULL) {
		slot = m - m->class->methods;

		if (!annotation_bytearrays_insert(
				method_annotations, slot, annotations)) {
			return false;
		}
	}

	return true;
}


/* annotation_load_method_attribute_runtimeinvisibleannotations ****************
   
   Load runtime invisible annotations of a method (just skip them).
   
*******************************************************************************/

bool annotation_load_method_attribute_runtimeinvisibleannotations(
	classbuffer *cb, methodinfo *m)
{
	return loader_skip_attribute_body(cb);
}


/* annotation_load_field_attribute_runtimevisibleannotations ******************
   
   Load runtime visible annotations of a field.
   
*******************************************************************************/

bool annotation_load_field_attribute_runtimevisibleannotations(
	classbuffer *cb, fieldinfo *f)
{
	int slot = 0;
	java_handle_bytearray_t    *annotations       = NULL;
	java_handle_objectarray_t **field_annotations = NULL;

	assert(cb != NULL);
	assert(f != NULL);

	field_annotations = &(f->class->field_annotations);

	if (!annotation_load_attribute_body(
			cb, &annotations,
			"invalid runtime visible annotations field attribute")) {
		return false;
	}

	if (annotations != NULL) {
		slot = f - f->class->fields;

		if (!annotation_bytearrays_insert(
				field_annotations, slot, annotations)) {
			return false;
		}
	}

	return true;
}


/* annotation_load_field_attribute_runtimeinvisibleannotations ****************
   
   Load runtime invisible annotations of a field (just skip them).
   
*******************************************************************************/

bool annotation_load_field_attribute_runtimeinvisibleannotations(
	classbuffer *cb, fieldinfo *f)
{
	return loader_skip_attribute_body(cb);
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
