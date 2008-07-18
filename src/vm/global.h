/* src/vm/global.h - global definitions

   Copyright (C) 1996-2005, 2006, 2007, 2008
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


#ifndef _GLOBAL_H
#define _GLOBAL_H

#include "config.h"

#include <stdbool.h>
#include <stdint.h>

#include "vm/types.h"


/* additional data types ******************************************************/

typedef void (*functionptr) (void);     /* generic function pointer           */
typedef u1* methodptr;

#if defined(ENABLE_SSA)
/* immediate to get an addidional target Local Var Index */
/* for IINC in Combination with SSA */
struct imm {
	s4 i;
	s4 op1_t;
};
#endif

/* immediate data union */

typedef union {
	s4          i;
	s8          l;
	float       f;
	double      d;
	void       *a;
	functionptr fp;
	u1          b[8];
#if defined(ENABLE_SSA)
	struct imm  _i;
#endif
} imm_union;


/* alignment macros ***********************************************************/

#define ALIGN_EVEN(a)                   ((a) = (((a) + 1) & ~1))
#define ALIGN_ODD(a)                    ((a) =   (a) | 1       )

#define ALIGN_2(a)                      ALIGN_EVEN(a)


/* printf format defines ******************************************************/

/* Define printf formats which change size between 32- and 64-bit. */

#if SIZEOF_VOID_P == 8
# define PRINTF_FORMAT_INT64_T    "%ld"
#else
# define PRINTF_FORMAT_INT64_T    "%lld"
#endif


/* convenience macros *********************************************************/

/* Makes a string of the argument (which is not macro-expanded). */

#define STR(a)  #a


/* forward typedefs ***********************************************************/

typedef struct java_object_t java_object_t; 
typedef struct java_objectarray_t java_objectarray_t;


#define MAX_ALIGN 8             /* most generic alignment for JavaVM values   */


/* basic data types ***********************************************************/

/* The JavaVM types must numbered in the same order as the ICMD_Ixxx
   to ICMD_Axxx instructions (LOAD and STORE).  All other types can be
   numbered arbitrarily. */

#define TYPE_INT     0
#define TYPE_LNG     1
#define TYPE_FLT     2
#define TYPE_DBL     3
#define TYPE_ADR     4

#define TYPE_RET     8   /* must not share bits with TYPE_FLT or TYPE_LNG */

#define TYPE_VOID    10


#define IS_INT_LNG_TYPE(a)      (!((a) & TYPE_FLT))
#define IS_FLT_DBL_TYPE(a)      ((a) & TYPE_FLT)
#define IS_2_WORD_TYPE(a)       ((a) & TYPE_LNG)

#define IS_INT_TYPE(a)          ((a) == TYPE_INT)
#define IS_LNG_TYPE(a)          ((a) == TYPE_LNG)
#define IS_FLT_TYPE(a)          ((a) == TYPE_FLT)
#define IS_DBL_TYPE(a)          ((a) == TYPE_DBL)
#define IS_ADR_TYPE(a)          ((a) == TYPE_ADR)

#define IS_VOID_TYPE(a)         ((a) == TYPE_VOID)


/* some Java related defines **************************************************/

#define JAVA_VERSION    "1.5.0"         /* this version is supported by CACAO */
#define CLASS_VERSION   "50.0"


/* Java class file constants **************************************************/

#define MAGIC             0xCAFEBABE
#define MAJOR_VERSION     50
#define MINOR_VERSION     0


/* Constant pool tags *********************************************************/

#define CONSTANT_Class                 7
#define CONSTANT_Fieldref              9
#define CONSTANT_Methodref            10
#define CONSTANT_InterfaceMethodref   11
#define CONSTANT_String                8
#define CONSTANT_Integer               3
#define CONSTANT_Float                 4
#define CONSTANT_Long                  5
#define CONSTANT_Double                6
#define CONSTANT_NameAndType          12
#define CONSTANT_Utf8                  1

#define CONSTANT_UNUSED                0


/* Class/Field/Method access and property flags *******************************/

#define ACC_UNDEF               -1      /* used internally                    */
#define ACC_NONE                 0      /* used internally                    */

#define ACC_PUBLIC          0x0001
#define ACC_PRIVATE         0x0002
#define ACC_PROTECTED       0x0004
#define ACC_STATIC          0x0008
#define ACC_FINAL           0x0010
#define ACC_SUPER           0x0020
#define ACC_SYNCHRONIZED    0x0020
#define ACC_VOLATILE        0x0040
#define ACC_BRIDGE          0x0040
#define ACC_TRANSIENT       0x0080
#define ACC_VARARGS         0x0080
#define ACC_NATIVE          0x0100
#define ACC_INTERFACE       0x0200
#define ACC_ABSTRACT        0x0400
#define ACC_STRICT          0x0800
#define ACC_SYNTHETIC       0x1000
#define ACC_ANNOTATION      0x2000
#define ACC_ENUM            0x4000
#define ACC_MIRANDA         0x8000

/* special flags used in classinfo ********************************************/

#define ACC_CLASS_REFLECT_MASK      0x0000ffff/* flags reported by reflection */

#define ACC_CLASS_PRIMITIVE         0x00010000
#define ACC_CLASS_MEMBER            0x00020000
#define ACC_CLASS_ANONYMOUS         0x00040000

#define ACC_CLASS_HAS_POINTERS      0x00080000/* instance contains pointers   */

#define ACC_CLASS_REFERENCE_MASK    0x00700000
#define ACC_CLASS_REFERENCE_SOFT    0x00100000
#define ACC_CLASS_REFERENCE_WEAK    0x00200000
#define ACC_CLASS_REFERENCE_PHANTOM 0x00400000


/* special flags used in methodinfo *******************************************/

#define ACC_METHOD_BUILTIN     0x00010000     /* use for descriptor parsing   */
#define ACC_METHOD_IMPLEMENTED 0x00020000     /* there is an implementation   */
#define ACC_METHOD_MONOMORPHIC 0x00040000     /* currently monomorphic method */
#define ACC_METHOD_EA          0x00080000     /* method being escape analyzed */
#define ACC_METHOD_MONOMORPHY_USED \
                               0x00100000
#define ACC_METHOD_PARENT_MONOMORPHY_USED \
                               0x00200000


/* data structures of the runtime system **************************************/

/* java_object_t ***************************************************************

   All objects (and arrays) which resides on the heap need the
   following header at the beginning of the data structure.

   TODO: Include detailed description from the Wiki (ObjectHeader) here.

*******************************************************************************/

#define HDRFLAG_MARK1         0x02
#define HDRFLAG_MARK2         0x04
#define HDRFLAG_UNCOLLECTABLE 0x08
#define HDRFLAG_HASH_TAKEN    0x10
#define HDRFLAG_HASH_ATTACHED 0x20
#define HDRFLAG_REFERENCING   0x40

struct java_object_t {                 /* header for all objects              */
	struct _vftbl *vftbl;              /* pointer to virtual function table   */
#if defined(ENABLE_THREADS)
	uintptr_t      lockword;
#endif
#if defined(ENABLE_GC_CACAO)
	uintptr_t      hdrflags;           /* word containing the GC bits         */
#endif
#if defined(ENABLE_ESCAPE_CHECK)
	void *method;
	void *thread;
	uintptr_t escape;
#endif
};


/* arrays **********************************************************************

	All arrays are objects (they need the object header with a pointer
	to a vftbl (array class table). There is one class for each array
	type. The array type is described by an arraydescriptor struct
	which is referenced by the vftbl.
*/

typedef struct java_array_t {           /* header for all arrays              */
	java_object_t objheader;            /* object header                      */
	s4 size;                            /* array size                         */
} java_array_t;



/* structs for all kinds of arrays ********************************************/

/*  booleanarray and bytearray need identical memory layout (access methods
    use the same machine code */

typedef struct java_booleanarray_t {
	java_array_t header;
	u1 data[1];
} java_booleanarray_t;

typedef struct java_bytearray_t {
	java_array_t header;
	s1 data[1];
} java_bytearray_t;

typedef struct java_chararray_t {
	java_array_t header;
	u2 data[1];
} java_chararray_t;

typedef struct java_shortarray_t {
	java_array_t header;
	s2 data[1];
} java_shortarray_t;

typedef struct java_intarray_t {
	java_array_t header;
	s4 data[1];
} java_intarray_t;

typedef struct java_longarray_t {
	java_array_t header;
	s8 data[1];
} java_longarray_t;

typedef struct java_floatarray_t {
	java_array_t header;
	float data[1];
} java_floatarray_t;

typedef struct java_doublearray_t {
	java_array_t header;
	double data[1];
} java_doublearray_t;

/*  objectarray and arrayarray need identical memory layout (access methods
    use the same machine code */

struct java_objectarray_t {
	java_array_t   header;
	java_object_t *data[1];
};


/* java_handle_t ***************************************************************

   TODO: document me!

*******************************************************************************/

#if defined(ENABLE_HANDLES)
typedef struct java_handle_t {
	java_object_t *heap_object;
} java_handle_t;

typedef struct java_handle_objectarray_t  { java_objectarray_t  *heap_object; } java_handle_objectarray_t;
typedef struct java_handle_booleanarray_t { java_booleanarray_t *heap_object; } java_handle_booleanarray_t;
typedef struct java_handle_bytearray_t    { java_bytearray_t    *heap_object; } java_handle_bytearray_t;
typedef struct java_handle_chararray_t    { java_chararray_t    *heap_object; } java_handle_chararray_t;
typedef struct java_handle_shortarray_t   { java_shortarray_t   *heap_object; } java_handle_shortarray_t;
typedef struct java_handle_intarray_t     { java_intarray_t     *heap_object; } java_handle_intarray_t;
typedef struct java_handle_longarray_t    { java_longarray_t    *heap_object; } java_handle_longarray_t;
typedef struct java_handle_floatarray_t   { java_floatarray_t   *heap_object; } java_handle_floatarray_t;
typedef struct java_handle_doublearray_t  { java_doublearray_t  *heap_object; } java_handle_doublearray_t;
#else
typedef java_object_t       java_handle_t;
typedef java_objectarray_t  java_handle_objectarray_t;
typedef java_booleanarray_t java_handle_booleanarray_t;
typedef java_bytearray_t    java_handle_bytearray_t;
typedef java_chararray_t    java_handle_chararray_t;
typedef java_shortarray_t   java_handle_shortarray_t;
typedef java_intarray_t     java_handle_intarray_t;
typedef java_longarray_t    java_handle_longarray_t;
typedef java_floatarray_t   java_handle_floatarray_t;
typedef java_doublearray_t  java_handle_doublearray_t;
#endif


/* global constants related to the verifier ***********************************/

/* The verifier needs additional variables in the variable array. Since these */
/* must be reserved and set by parse.c and stack.c, we define these numbers   */
/* here to avoid mysterious hard-coded constants.                             */
/* stack.c needs an extra variable if the verifier is disabled.               */

#if defined(ENABLE_VERIFIER)
#    define VERIFIER_EXTRA_LOCALS  1
#    define VERIFIER_EXTRA_VARS    1
#    define STACK_EXTRA_VARS       0
#else
#    define VERIFIER_EXTRA_LOCALS  0
#    define VERIFIER_EXTRA_VARS    0
#    define STACK_EXTRA_VARS       1
#endif

#endif /* _GLOBAL_H */


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
