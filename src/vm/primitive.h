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


#ifndef _PRIMITIVE_H
#define _PRIMITIVE_H

#include "config.h"

#include <stdint.h>

#include "vm/global.h"

#include "vmcore/class.h"
#include "vmcore/linker.h"
#include "vmcore/utf8.h"


/* primitive data types *******************************************************/

/* These values are used in parsed descriptors and in some other
   places were the different types handled internally as TYPE_INT have
   to be distinguished. */

#define PRIMITIVETYPE_COUNT  11  /* number of primitive types (+ dummies)     */

/* CAUTION: Don't change the numerical values! These constants are
   used as indices into the primitive type table. */

#define PRIMITIVETYPE_INT     TYPE_INT
#define PRIMITIVETYPE_LONG    TYPE_LNG
#define PRIMITIVETYPE_FLOAT   TYPE_FLT
#define PRIMITIVETYPE_DOUBLE  TYPE_DBL
#define PRIMITIVETYPE_DUMMY1  TYPE_ADR     /* not used! */
#define PRIMITIVETYPE_BYTE    5
#define PRIMITIVETYPE_CHAR    6
#define PRIMITIVETYPE_SHORT   7
#define PRIMITIVETYPE_BOOLEAN 8
#define PRIMITIVETYPE_DUMMY2  9            /* not used! */
#define PRIMITIVETYPE_VOID    TYPE_VOID


/* CAUTION: Don't change the numerical values! These constants (with
   the exception of ARRAYTYPE_OBJECT) are used as indices in the
   primitive type table. */

#define ARRAYTYPE_INT         PRIMITIVETYPE_INT
#define ARRAYTYPE_LONG        PRIMITIVETYPE_LONG
#define ARRAYTYPE_FLOAT       PRIMITIVETYPE_FLOAT
#define ARRAYTYPE_DOUBLE      PRIMITIVETYPE_DOUBLE
#define ARRAYTYPE_BYTE        PRIMITIVETYPE_BYTE
#define ARRAYTYPE_CHAR        PRIMITIVETYPE_CHAR
#define ARRAYTYPE_SHORT       PRIMITIVETYPE_SHORT
#define ARRAYTYPE_BOOLEAN     PRIMITIVETYPE_BOOLEAN
#define ARRAYTYPE_OBJECT      PRIMITIVETYPE_VOID     /* don't use as index! */


/* primitivetypeinfo **********************************************************/

struct primitivetypeinfo {
	char      *cname;                    /* char name of primitive class      */
	utf       *name;                     /* name of primitive class           */
	classinfo *class_wrap;               /* class for wrapping primitive type */
	classinfo *class_primitive;          /* primitive class                   */
	char      *wrapname;                 /* name of class for wrapping        */
	char       typesig;                  /* one character type signature      */
	char      *arrayname;                /* name of primitive array class     */
	classinfo *arrayclass;               /* primitive array class             */
};


/* global variables ***********************************************************/

/* This array can be indexed by the PRIMITIVETYPE_ and ARRAYTYPE_
   constants (except ARRAYTYPE_OBJECT). */

extern primitivetypeinfo primitivetype_table[PRIMITIVETYPE_COUNT];


/* function prototypes ********************************************************/

/* this function is in src/vmcore/primitivecore.c */
bool       primitive_init(void);

classinfo *primitive_class_get_by_name(utf *name);
classinfo *primitive_class_get_by_type(int type);
classinfo *primitive_class_get_by_char(char ch);
classinfo *primitive_arrayclass_get_by_name(utf *name);
classinfo *primitive_arrayclass_get_by_type(int type);

java_handle_t *primitive_box(int type, imm_union value);
imm_union      primitive_unbox(int type, java_handle_t *o);

java_handle_t *primitive_box_boolean(int32_t value);
java_handle_t *primitive_box_byte(int32_t value);
java_handle_t *primitive_box_char(int32_t value);
java_handle_t *primitive_box_short(int32_t value);
java_handle_t *primitive_box_int(int32_t value);
java_handle_t *primitive_box_long(int64_t value);
java_handle_t *primitive_box_float(float value);
java_handle_t *primitive_box_double(double value);

int32_t        primitive_unbox_boolean(java_handle_t *o);
int32_t        primitive_unbox_byte(java_handle_t *o);
int32_t        primitive_unbox_char(java_handle_t *o);
int32_t        primitive_unbox_short(java_handle_t *o);
int32_t        primitive_unbox_int(java_handle_t *o);
int64_t        primitive_unbox_long(java_handle_t *o);
float          primitive_unbox_float(java_handle_t *o);
double         primitive_unbox_double(java_handle_t *o);

#endif /* _PRIMITIVE_H */


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
