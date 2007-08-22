/* src/vmcore/annotation.h - class annotations

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


#ifndef _ANNOTATION_H
#define _ANNOTATION_H

/* forward typedefs ***********************************************************/

typedef struct annotation_bytearray_t annotation_bytearray_t;
typedef struct annotation_t           annotation_t;
typedef struct element_value_t        element_value_t;
typedef struct annotation_bytearrays_t annotation_bytearrays_t;

#include "config.h"
#include "vm/types.h"

#include "vm/global.h"

#include "vmcore/class.h"
#include "vmcore/field.h"
#include "vmcore/method.h"
#include "vmcore/loader.h"
#include "vmcore/utf8.h"


/* annotation_bytearray *******************************************************/

struct annotation_bytearray_t {
	uint32_t size;
	uint8_t  data[1];
};


/* annotation_bytearrays ******************************************************/

struct annotation_bytearrays_t {
	uint32_t size;
	annotation_bytearray_t *data[1];
};


/* function prototypes ********************************************************/

annotation_bytearray_t *annotation_bytearray_new(uint32_t size);

void annotation_bytearray_free(annotation_bytearray_t *ba);

annotation_bytearrays_t *annotation_bytearrays_new(uint32_t size);

bool annotation_bytearrays_resize(annotation_bytearrays_t **bas,
	uint32_t size);

bool annotation_bytearrays_insert(annotation_bytearrays_t **bas,
	uint32_t index, annotation_bytearray_t *ba);

void annotation_bytearrays_free(annotation_bytearrays_t *bas);

bool annotation_load_class_attribute_runtimevisibleannotations(
	classbuffer *cb);

bool annotation_load_class_attribute_runtimeinvisibleannotations(
	classbuffer *cb);

bool annotation_load_method_attribute_runtimevisibleannotations(
	classbuffer *cb, methodinfo *m);

bool annotation_load_method_attribute_runtimeinvisibleannotations(
	classbuffer *cb, methodinfo *m);

bool annotation_load_field_attribute_runtimevisibleannotations(
	classbuffer *cb, fieldinfo *f);

bool annotation_load_field_attribute_runtimeinvisibleannotations(
	classbuffer *cb, fieldinfo *f);

bool annotation_load_method_attribute_annotationdefault(
	classbuffer *cb, methodinfo *m);

bool annotation_load_method_attribute_runtimevisibleparameterannotations(
	classbuffer *cb, methodinfo *m);

bool annotation_load_method_attribute_runtimeinvisibleparameterannotations(
	classbuffer *cb, methodinfo *m);

#endif /* _ANNOTATION_H */


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
