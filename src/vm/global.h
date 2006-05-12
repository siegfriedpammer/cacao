/* src/vm/global.h - global definitions

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
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

   Contact: cacao@cacaojvm.org

   Authors: Reinhard Grafl
            Andreas Krall

   Changes: Mark Probst
            Philipp Tomsich
            Edwin Steiner
            Joseph Wenninger
            Christian Thalinger

   $Id: global.h 4908 2006-05-12 16:49:50Z edwin $

*/


#ifndef _GLOBAL_H
#define _GLOBAL_H

#include "config.h"
#include "vm/types.h"


/* additional data types ******************************************************/

typedef void *voidptr;                  /* generic pointer                    */
typedef void (*functionptr) (void);     /* generic function pointer           */
typedef u1* methodptr;

typedef int   bool;                     /* boolean data type                  */

#define true  1
#define false 0


/* immediate data union */

typedef union {
	s4          i;
	s8          l;
	float       f;
	double      d;
	void       *a;
	functionptr fp;
	u1          b[8];
} imm_union;


/* forward typedefs ***********************************************************/

typedef struct java_objectheader java_objectheader; 
typedef struct java_objectarray java_objectarray;


/* define some CACAO paths ****************************************************/

#if defined(ENABLE_ZLIB)
# define CACAO_VM_ZIP_PATH          CACAO_PREFIX "/share/cacao/" VM_ZIP_STRING
#else
# define CACAO_VM_ZIP_PATH          CACAO_PREFIX "/share/cacao/"
#endif

#define CLASSPATH_GLIBJ_ZIP_PATH    CLASSPATH_PREFIX "/share/classpath/" GLIBJ_ZIP_STRING
#define CLASSPATH_LIBRARY_PATH      CLASSPATH_LIBDIR "/classpath"


/* if we have threads disabled this one is not defined ************************/

#if !defined(USE_THREADS)
#define THREADSPECIFIC
#endif


#define MAX_ALIGN 8             /* most generic alignment for JavaVM values   */


/* basic data types ***********************************************************/

/* CAUTION: jit/jit.h relies on these numerical values! */
#define TYPE_INT      0         /* the JavaVM types must numbered in the      */
#define TYPE_LONG     1         /* same order as the ICMD_Ixxx to ICMD_Axxx   */
#define TYPE_FLOAT    2         /* instructions (LOAD and STORE)              */
#define TYPE_DOUBLE   3         /* integer, long, float, double, address      */
#define TYPE_ADDRESS  4         /* all other types can be numbered arbitrarly */

#define TYPE_VOID    10

/* primitive data types *******************************************************/

/* These values are used in parsed descriptors and in some other
   places were the different types handled internally as TYPE_INT have
   to be distinguished. */

#define PRIMITIVETYPE_COUNT  11  /* number of primitive types (+ dummies)     */

/* CAUTION: Don't change the numerical values! These constants are
   used as indices into the primitive type table. */

#define PRIMITIVETYPE_INT     TYPE_INT
#define PRIMITIVETYPE_LONG    TYPE_LONG
#define PRIMITIVETYPE_FLOAT   TYPE_FLOAT
#define PRIMITIVETYPE_DOUBLE  TYPE_DOUBLE
#define PRIMITIVETYPE_DUMMY1  TYPE_ADR     /* not used! */
#define PRIMITIVETYPE_BYTE    5
#define PRIMITIVETYPE_CHAR    6
#define PRIMITIVETYPE_SHORT   7
#define PRIMITIVETYPE_BOOLEAN 8
#define PRIMITIVETYPE_DUMMY2  9            /* not used! */
#define PRIMITIVETYPE_VOID    TYPE_VOID

/* some Java related defines **************************************************/

#define JAVA_VERSION    "1.4.2"         /* this version is supported by CACAO */
#define CLASS_VERSION   "49.0"


/* Java class file constants **************************************************/

#define MAGIC             0xCAFEBABE
#define MAJOR_VERSION     49
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

#define ACC_CLASS_REFLECT_MASK 0x0000ffff     /* flags reported by reflection */
#define ACC_CLASS_HAS_POINTERS 0x00010000     /* instance contains pointers   */


/* data structures of the runtime system **************************************/

/* java_objectheader ***********************************************************

   All objects (and arrays) which resides on the heap need the
   following header at the beginning of the data structure.

*******************************************************************************/

struct java_objectheader {              /* header for all objects             */
	struct _vftbl            *vftbl;    /* pointer to virtual function table  */
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	struct lock_record_t *monitorPtr;
#endif
};


/* arrays **********************************************************************

	All arrays are objects (they need the object header with a pointer
	to a vftbl (array class table). There is one class for each array
	type. The array type is described by an arraydescriptor struct
	which is referenced by the vftbl.
*/

/* CAUTION: Don't change the numerical values! These constants (with
 * the exception of ARRAYTYPE_OBJECT) are used as indices in the
 * primitive type table.
 */
#define ARRAYTYPE_INT      PRIMITIVETYPE_INT
#define ARRAYTYPE_LONG     PRIMITIVETYPE_LONG
#define ARRAYTYPE_FLOAT    PRIMITIVETYPE_FLOAT
#define ARRAYTYPE_DOUBLE   PRIMITIVETYPE_DOUBLE
#define ARRAYTYPE_BYTE     PRIMITIVETYPE_BYTE
#define ARRAYTYPE_CHAR     PRIMITIVETYPE_CHAR
#define ARRAYTYPE_SHORT    PRIMITIVETYPE_SHORT
#define ARRAYTYPE_BOOLEAN  PRIMITIVETYPE_BOOLEAN
#define ARRAYTYPE_OBJECT   PRIMITIVETYPE_VOID     /* don't use as index! */

typedef struct java_arrayheader {       /* header for all arrays              */
	java_objectheader objheader;        /* object header                      */
	s4 size;                            /* array size                         */
} java_arrayheader;



/* structs for all kinds of arrays ********************************************/

/*  booleanarray and bytearray need identical memory layout (access methods
    use the same machine code */

typedef struct java_booleanarray {
	java_arrayheader header;
	u1 data[1];
} java_booleanarray;

typedef struct java_bytearray {
	java_arrayheader header;
	s1 data[1];
} java_bytearray;

typedef struct java_chararray {
	java_arrayheader header;
	u2 data[1];
} java_chararray;

typedef struct java_shortarray {
	java_arrayheader header;
	s2 data[1];
} java_shortarray;

typedef struct java_intarray {
	java_arrayheader header;
	s4 data[1];
} java_intarray;

typedef struct java_longarray {
	java_arrayheader header;
	s8 data[1];
} java_longarray;

typedef struct java_floatarray {
	java_arrayheader header;
	float data[1];
} java_floatarray;

typedef struct java_doublearray {
	java_arrayheader header;
	double data[1];
} java_doublearray;

/*  objectarray and arrayarray need identical memory layout (access methods
    use the same machine code */

struct java_objectarray {
	java_arrayheader   header;
	java_objectheader *data[1];
};


/* Synchronization ************************************************************/

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void compiler_lock();
void compiler_unlock();
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
