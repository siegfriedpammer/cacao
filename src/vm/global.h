/* global.h - global definitions

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser,
   M. Probst, S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck,
   P. Tomsich, J. Wenninger

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Reinhard Grafl
            Andreas Krall

   Changes: Mark Probst
            Philipp Tomsich
			Edwin Steiner
            Joseph Wenninger

   $Id: global.h 1687 2004-12-05 23:57:27Z twisti $

*/


#ifndef _GLOBAL_H
#define _GLOBAL_H

#include "config.h"
#include "types.h"


/* resolve typedef cycles *****************************************************/

typedef struct utf utf;
typedef struct literalstring literalstring;
typedef struct java_objectheader java_objectheader; 
typedef struct classinfo classinfo; 
typedef struct _vftbl vftbl_t;
typedef u1* methodptr;
typedef struct fieldinfo  fieldinfo; 
typedef struct exceptiontable exceptiontable;
typedef struct methodinfo methodinfo; 
typedef struct lineinfo lineinfo; 
typedef struct arraydescriptor arraydescriptor;


/* additional data types ******************************************************/

typedef void *voidptr;                  /* generic pointer                    */
typedef void (*functionptr) ();         /* generic function pointer           */

typedef int   bool;                     /* boolean data type                  */

#define true  1
#define false 0


/* additional includes ********************************************************/

#include "toolbox/list.h"
#include "vm/jit/inline/sets.h"


#if defined(USE_THREADS) && defined(NATIVE_THREADS)
#include <pthread.h>
#include <semaphore.h>
#endif

/* define path to rt.jar plus ending : ****************************************/

#define CACAO_LIBRARY_PATH    "/jre/lib/"ARCH_DIR"/"
#define CACAO_RT_JAR_PATH     "/jre/lib/rt.jar:"


#define STATISTICS          /* if enabled collects program statistics         */

/* 
 * SIZE_FROM_CLASSINFO toggles between the bitmap_based and the new method 
 * of determining the sizes of objects on the heap.
 */
#define SIZE_FROM_CLASSINFO

/*
 * CACAO_TYPECHECK activates typechecking (part of bytecode verification)
 */
#define CACAO_TYPECHECK

/*
 * TYPECHECK_STACK_COMPCAT activates full checking of computational
 * categories for stack manipulations (POP,POP2,SWAP,DUP,DUP2,DUP_X1,
 * DUP2_X1,DUP_X2,DUP2_X2).
 */
#define TYPECHECK_STACK_COMPCAT

/*
 * Macros for configuration of the typechecking code
 *
 * TYPECHECK_STATISTICS activates gathering statistical information.
 * TYPEINFO_DEBUG activates debug checks and debug helpers in typeinfo.c
 * TYPECHECK_DEBUG activates debug checks in typecheck.c
 * TYPEINFO_DEBUG_TEST activates the typeinfo test at startup.
 * TYPECHECK_VERBOSE_IMPORTANT activates important debug messages
 * TYPECHECK_VERBOSE activates all debug messages
 */
#ifdef CACAO_TYPECHECK
/*#define TYPECHECK_STATISTICS
#define TYPEINFO_DEBUG
#define TYPECHECK_DEBUG
#define TYPEINFO_DEBUG_TEST
#define TYPECHECK_VERBOSE
#define TYPECHECK_VERBOSE_IMPORTANT*/
#if defined(TYPECHECK_VERBOSE) || defined(TYPECHECK_VERBOSE_IMPORTANT)
#define TYPECHECK_VERBOSE_OPT
#endif
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
} imm_union;


#define PRIMITIVETYPE_COUNT  9  /* number of primitive types */

/* CAUTION: Don't change the numerical values! These constants are
 * used as indices into the primitive type table.
 */
#define PRIMITIVETYPE_INT     0
#define PRIMITIVETYPE_LONG    1
#define PRIMITIVETYPE_FLOAT   2
#define PRIMITIVETYPE_DOUBLE  3
#define PRIMITIVETYPE_BYTE    4
#define PRIMITIVETYPE_CHAR    5
#define PRIMITIVETYPE_SHORT   6
#define PRIMITIVETYPE_BOOLEAN 7
#define PRIMITIVETYPE_VOID    8


#define MAX_ALIGN 8             /* most generic alignment for JavaVM values   */


/* basic data types ***********************************************************/

/* CAUTION: jit/jit.h relies on these numerical values! */
#define TYPE_INT      0         /* the JavaVM types must numbered in the      */
#define TYPE_LONG     1         /* same order as the ICMD_Ixxx to ICMD_Axxx   */
#define TYPE_FLOAT    2         /* instructions (LOAD and STORE)              */
#define TYPE_DOUBLE   3         /* integer, long, float, double, address      */
#define TYPE_ADDRESS  4         /* all other types can be numbered arbitrarly */

#define TYPE_VOID    10


/* Java class file constants **************************************************/

#define MAGIC         0xcafebabe
#define MINOR_VERSION 0
#define MAJOR_VERSION 48

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

#define ACC_PUBLIC                0x0001
#define ACC_PRIVATE               0x0002
#define ACC_PROTECTED             0x0004
#define ACC_STATIC                0x0008
#define ACC_FINAL                 0x0010
#define ACC_SUPER                 0x0020
#define ACC_SYNCHRONIZED          0x0020
#define ACC_VOLATILE              0x0040
#define ACC_TRANSIENT             0x0080
#define ACC_NATIVE                0x0100
#define ACC_INTERFACE             0x0200
#define ACC_ABSTRACT              0x0400
#define ACC_STRICT                0x0800


/* constant pool entries *******************************************************

	All constant pool entries need a data structure which contain the entrys
	value. In some cases this structure exist already, in the remaining cases
	this structure must be generated:

		kind                      structure                     generated?
	----------------------------------------------------------------------
    CONSTANT_Class               classinfo                           no
    CONSTANT_Fieldref            constant_FMIref                    yes
    CONSTANT_Methodref           constant_FMIref                    yes
    CONSTANT_InterfaceMethodref  constant_FMIref                    yes
    CONSTANT_String              unicode                             no
    CONSTANT_Integer             constant_integer                   yes
    CONSTANT_Float               constant_float                     yes
    CONSTANT_Long                constant_long                      yes
    CONSTANT_Double              constant_double                    yes
    CONSTANT_NameAndType         constant_nameandtype               yes
    CONSTANT_Utf8                unicode                             no
    CONSTANT_UNUSED              -

*******************************************************************************/

/* data structures for hashtables ********************************************


	All utf-symbols, javastrings and classes are stored in global hashtables,
	so every symbol exists only once. Equal symbols have identical pointers.
	The functions for adding hashtable elements search the table for the 
	element with the specified name/text and return it on success. Otherwise a 
	new hashtable element is created.

    The hashtables use external linking for handling collisions. The hashtable 
	structure contains a pointer <ptr> to the array of hashtable slots. The 
	number of hashtable slots and therefore the size of this array is specified 
	by the element <size> of hashtable structure. <entries> contains the number
	of all hashtable elements stored in the table, including those in the 
	external chains.
	The hashtable element structures (utf, literalstring, classinfo) contain
	both a pointer to the next hashtable element as a link for the external hash 
	chain and the key of the element. The key is computed from the text of
	the string or the classname by using up to 8 characters.
	
	If the number of entries in the hashtable exceeds twice the size of the 
	hashtableslot-array it is supposed that the average length of the 
	external chains has reached a value beyond 2. Therefore the functions for
	adding hashtable elements (utf_new, class_new, literalstring_new) double
	the hashtableslot-array. In this restructuring process all elements have
	to be inserted into the new hashtable and new external chains must be built.


example for the layout of a hashtable:

hashtable.ptr-->  +-------------------+
                  |                   |
                           ...
                  |                   |
                  +-------------------+   +-------------------+   +-------------------+
                  | hashtable element |-->| hashtable element |-->| hashtable element |-->NULL
                  +-------------------+   +-------------------+   +-------------------+
                  | hashtable element |
                  +-------------------+   +-------------------+   
                  | hashtable element |-->| hashtable element |-->NULL
                  +-------------------+   +-------------------+   
                  | hashtable element |-->NULL
                  +-------------------+
                  |                   |
                           ...
                  |                   |
                  +-------------------+

*/


/* data structure for utf8 symbols ********************************************/

struct utf {
	utf        *hashlink;       /* link for external hash chain               */
	int         blength;        /* text length in bytes                       */           
	char       *text;           /* pointer to text                            */
};


/* data structure of internal javastrings stored in global hashtable **********/

struct literalstring {
	literalstring     *hashlink;     /* link for external hash chain          */
	java_objectheader *string;  
};


/* data structure for storing information needed for a stacktrace across native functions*/
struct native_stackframeinfo {
	void *oldThreadspecificHeadValue;
	void **addressOfThreadspecificHead;
	methodinfo *method;
#ifdef __ALPHA__
	void *savedpv;
#endif
	void *beginOfJavaStackframe; /*only used if != 0*/
	void *returnToFromNative;

#if 0
	void *returnFromNative;
	void *addrReturnFromNative;
	methodinfo *method;
	struct native_stackframeinfo *next;
	struct native_stackframeinfo *prev;
#endif
};

typedef struct native_stackframeinfo native_stackframeinfo;

struct stacktraceelement {
#if POINTERSIZE == 8
	u8 linenumber;
#else
	u4 linenumber;
#endif
	methodinfo *method;
};

typedef struct stacktraceelement stacktraceelement;

typedef struct stackTraceBuffer {
        int needsFree;
        struct stacktraceelement* start;
        size_t size;
        size_t full;
} stackTraceBuffer;



/* data structure for calls from c code to java methods */

struct jni_callblock {
	u8 itemtype;
	u8 item;
};

typedef struct jni_callblock jni_callblock;


/* data structure for accessing hashtables ************************************/

typedef struct {            
	u4 size;
	u4 entries;        /* number of entries in the table */
	void **ptr;        /* pointer to hashtable */
} hashtable;


/* data structures of remaining constant pool entries *************************/

typedef struct {            /* Fieldref, Methodref and InterfaceMethodref     */
	classinfo *class;       /* class containing this field/method/interface   */
	utf       *name;        /* field/method/interface name                    */
	utf       *descriptor;  /* field/method/interface type descriptor string  */
} constant_FMIref;


typedef struct {            /* Integer                                        */
	s4 value;
} constant_integer;

	
typedef struct {            /* Float                                          */
	float value;
} constant_float;


typedef struct {            /* Long                                           */
	s8 value;
} constant_long;
	

typedef struct {            /* Double                                         */
	double value;
} constant_double;


typedef struct {            /* NameAndType (Field or Method)                  */
	utf *name;              /* field/method name                              */
	utf *descriptor;        /* field/method type descriptor string            */
} constant_nameandtype;


/* data structures of the runtime system **************************************/

/* objects *********************************************************************

	All objects (and arrays) which resides on the heap need the following
	header at the beginning of the data structure.
*/

struct java_objectheader {              /* header for all objects             */
	vftbl_t *vftbl;                     /* pointer to virtual function table  */
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	void *monitorPtr;
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
#ifdef SIZE_FROM_CLASSINFO
	s4 alignedsize; /* phil */
#endif
} java_arrayheader;



/* structs for all kinds of arrays ********************************************/

typedef struct java_chararray {
	java_arrayheader header;
	u2 data[1];
} java_chararray;

typedef struct java_floatheader {
	java_arrayheader header;
	float data[1];
} java_floatarray;

typedef struct java_doublearray {
	java_arrayheader header;
	double data[1];
} java_doublearray;

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

/*  objectarray and arrayarray need identical memory layout (access methods
    use the same machine code */

typedef struct java_objectarray {
	java_arrayheader header;
	java_objectheader *data[1];
} java_objectarray;


/* structure for primitive classes ********************************************/

typedef struct primitivetypeinfo {
	classinfo *class_wrap;               /* class for wrapping primitive type */
	classinfo *class_primitive;          /* primitive class                   */
	char      *wrapname;                 /* name of class for wrapping        */
	char      typesig;                   /* one character type signature      */
	char      *name;                     /* name of primitive class           */
	char      *arrayname;                /* name of primitive array class     */
	classinfo *arrayclass;               /* primitive array class             */
	vftbl_t     *arrayvftbl;             /* vftbl of primitive array class    */
} primitivetypeinfo;


/* field, method and class structures *****************************************/

typedef	struct xtafldinfo {
	bool       fieldChecked; 		
	classinfo *fldClassType;
	classSet  *XTAclassSet;          /* field class type set                  */
} xtafldinfo;


/* fieldinfo ******************************************************************/

struct fieldinfo {	      /* field of a class                                 */
	s4  flags;            /* ACC flags                                        */
	s4  type;             /* basic data type                                  */
	utf *name;            /* name of field                                    */
	utf *descriptor;      /* JavaVM descriptor string of field                */
	
	s4  offset;           /* offset from start of object (instance variables) */

	imm_union value;      /* storage for static values (class variables)      */

	classinfo *class;     /* needed by typechecker. Could be optimized        */
	                      /* away by using constant_FMIref instead of         */
	                      /* fieldinfo throughout the compiler.               */
	
	xtafldinfo *xta;
};


/* exceptiontable *************************************************************/

struct exceptiontable {         /* exceptiontable entry in a method           */
	s4              startpc;    /* start pc of guarded area (inclusive)       */
	struct basicblock *start;

	s4              endpc;      /* end pc of guarded area (exklusive)         */
	struct basicblock *end;

	s4              handlerpc;  /* pc of exception handler                    */
	struct basicblock *handler;

	classinfo      *catchtype;  /* catchtype of exception (NULL == catchall)  */
	exceptiontable *next;       /* used to build a list of exception when     */
	                            /* loops are copied */
	exceptiontable *down;       /* instead of the old array, a list is used   */
};


/* methodinfo  static info ****************************************************/

typedef struct xtainfo {
	s4          XTAmethodUsed;     /* XTA if used in callgraph - not used /used */
	classSet    *XTAclassSet;      /* method class type set                 */ 
	/*classSet 	*PartClassSet */   /* method class type set                 */ 

	classSetNode    *paramClassSet; /* cone set of methods parameters       */

	methSet  	*calls;            /* methods this method calls   	        */ 
	methSet  	*calledBy;         /* methods that call this method         */ 
	methSet  	*marked;  /*not in Dez*/         /* methods that marked by this method    */ 
	methSet         *markedBy;
	fldSet          *fldsUsed;         /* fields used by this method             */ 
	/*methSetNode  *interfaceCalls*/   /* methods this method calls as interface */ 
	bool           chgdSinceLastParse; /* Changed since last parse ?          */
} xtainfo; 


/* lineinfo *****************************************************************/

struct lineinfo {
	u2 start_pc;
	u2 line_number;
};


/* methodinfo *****************************************************************/

struct methodinfo {                 /* method structure                       */
	java_objectheader header;       /* we need this in jit's monitorenter     */
	s4          flags;              /* ACC flags                              */
	utf        *name;               /* name of method                         */
	utf        *descriptor;         /* JavaVM descriptor string of method     */
	s4          returntype;         /* only temporary valid, return type      */
	classinfo  *returnclass;        /* pointer to classinfo for the rtn type  */ /*XTA*/ 
	s4          paramcount;         /* only temporary valid, parameter count  */
	u1         *paramtypes;         /* only temporary valid, parameter types  */
	classinfo **paramclass;         /* pointer to classinfo for a parameter   */ /*XTA*/

	bool        isleafmethod;       /* does method call subroutines           */

	classinfo  *class;              /* class, the method belongs to           */
	s4          vftblindex;         /* index of method in virtual function    */
	                                /* table (if it is a virtual method)      */
	s4          maxstack;           /* maximum stack depth of method          */
	s4          maxlocals;          /* maximum number of local variables      */
	s4          jcodelength;        /* length of JavaVM code                  */
	u1         *jcode;              /* pointer to JavaVM code                 */

	s4          basicblockcount;    /* number of basic blocks                 */
	struct basicblock *basicblocks; /* points to basic block array            */
	s4         *basicblockindex;    /* a table which contains for every byte  */
	                                /* of JavaVM code a basic block index if  */
	                                /* at this byte is the start of a basic   */
	                                /* block                                  */

	s4          instructioncount;   /* number of JavaVM instructions          */
	struct instruction *instructions; /* points to intermediate code instructions */

	s4          stackcount;         /* number of stack elements               */
	struct stackelement *stack;     /* points to intermediate code instructions */

	s4          exceptiontablelength;/* exceptiontable length                 */
	exceptiontable *exceptiontable; /* the exceptiontable                     */

	u2          thrownexceptionscount;/* number of exceptions attribute       */
	classinfo **thrownexceptions;   /* checked exceptions a method may throw  */

	u2          linenumbercount;    /* number of linenumber attributes        */
	lineinfo   *linenumbers;        /* array of lineinfo items                */

	int       c_debug_nr;           /* a counter to number all BB with an     */
	                                /* unique value                           */

	u1         *stubroutine;        /* stub for compiling or calling natives  */
	s4          mcodelength;        /* legth of generated machine code        */
	functionptr mcode;              /* pointer to machine code                */
	functionptr entrypoint;         /* entry point in machine code            */

	/*rtainfo   rta;*/
	xtainfo    *xta;

	s4          methodUsed;         /* marked (might be used later) /not used /used */
	s4          monoPoly;           /* call is mono or poly or unknown        */ /*RT stats */
        /* should # method def'd and used be kept after static parse (will it be used?) */
	s4	        subRedefs;
	s4	        subRedefsUsed;
	s4	        nativelyoverloaded; /* used in header.c and only valid there  */
};


/* innerclassinfo *************************************************************/

typedef struct innerclassinfo {
	classinfo *inner_class;       /* inner class pointer                      */
	classinfo *outer_class;       /* outer class pointer                      */
	utf       *name;              /* innerclass name                          */
	s4         flags;             /* ACC flags                                */
} innerclassinfo;


/* classinfo ******************************************************************/

struct classinfo {                /* class structure                          */
	java_objectheader header;     /* classes are also objects                 */
	java_objectarray* signers;
	struct java_security_ProtectionDomain* pd;
	struct java_lang_VMClass* vmClass;
	struct java_lang_reflect_Constructor* constructor;

	s4 initializing_thread;       /* gnu classpath                            */
	s4 erroneous_state;           /* gnu classpath                            */
	struct gnu_classpath_RawData* vmData; /* gnu classpath                    */

	s4          flags;            /* ACC flags                                */
	utf        *name;             /* class name                               */

	s4          cpcount;          /* number of entries in constant pool       */
	u1         *cptags;           /* constant pool tags                       */
	voidptr    *cpinfos;          /* pointer to constant pool info structures */

	classinfo  *super;            /* super class pointer                      */
	classinfo  *sub;              /* sub class pointer                        */
	classinfo  *nextsub;          /* pointer to next class in sub class list  */

	s4          interfacescount;  /* number of interfaces                     */
	classinfo **interfaces;       /* pointer to interfaces                    */

	s4          fieldscount;      /* number of fields                         */
	fieldinfo  *fields;           /* field table                              */

	s4          methodscount;     /* number of methods                        */
	methodinfo *methods;          /* method table                             */

	listnode    listnode;         /* linkage                                  */

	bool        initialized;      /* true, if class already initialized       */
	bool        initializing;     /* flag for the compiler                    */
	bool        loaded;           /* true, if class already loaded            */
	bool        linked;           /* true, if class already linked            */
	s4          index;            /* hierarchy depth (classes) or index       */
	                              /* (interfaces)                             */
	s4          instancesize;     /* size of an instance of this class        */
#ifdef SIZE_FROM_CLASSINFO
	s4          alignedsize;      /* size of an instance, aligned to the      */
	                              /* allocation size on the heap              */
#endif

	vftbl_t    *vftbl;            /* pointer to virtual function table        */

	methodinfo *finalizer;        /* finalizer method                         */

    u2          innerclasscount;  /* number of inner classes                  */
    innerclassinfo *innerclass;

    classinfo  *hashlink;         /* link for external hash chain             */
	bool        classvftbl;       /* has its own copy of the Class vtbl       */

	s4          classUsed;        /* 0= not used 1 = used   CO-RT             */

	classSetNode *impldBy;        /* implemented by class set                 */
	utf        *packagename;      /* full name of the package                 */
	utf        *sourcefile;       /* classfile name containing this class     */
	java_objectheader *classloader; /* NULL for bootstrap classloader         */
};

/* check if class is an array class. Only use for linked classes! */
#define CLASS_IS_ARRAY(clsinfo)  ((clsinfo)->vftbl->arraydesc != NULL)


/* virtual function table ******************************************************

	The vtbl has a bidirectional layout with open ends at both sides.
	interfacetablelength gives the number of entries of the interface table at
	the start of the vftbl. The vftbl pointer points to &interfacetable[0].
	vftbllength gives the number of entries of table at the end of the vftbl.

	runtime type check (checkcast):

	Different methods are used for runtime type check depending on the
	argument of checkcast/instanceof.
	
	A check against a class is implemented via relative numbering on the class
	hierachy tree. The tree is numbered in a depth first traversal setting
	the base field and the diff field. The diff field gets the result of
	(high - base) so that a range check can be implemented by an unsigned
	compare. A sub type test is done by checking the inclusion of base of
	the sub class in the range of the superclass.

	A check against an interface is implemented via the interfacevftbl. If the
	interfacevftbl contains a nonnull value a class is a subclass of this
	interface.

	interfacetable:

	Like standard virtual methods interface methods are called using
	virtual function tables. All interfaces are numbered sequentially
	(starting with zero). For each class there exist an interface table
	of virtual function tables for each implemented interface. The length
	of the interface table is determined by the highest number of an
	implemented interface.

	The following example assumes a class which implements interface 0 and 3:

	interfacetablelength = 4

                  | ...       |            +----------+
	              +-----------+            | method 2 |---> method z
	              | class     |            | method 1 |---> method y
	              +-----------+            | method 0 |---> method x
	              | ivftbl  0 |----------> +----------+
	vftblptr ---> +-----------+
                  | ivftbl -1 |--> NULL    +----------+
                  | ivftbl -2 |--> NULL    | method 1 |---> method x
                  | ivftbl -3 |-----+      | method 0 |---> method a
                  +-----------+     +----> +----------+
     
                              +---------------+
	                          | length 3 = 2  |
	                          | length 2 = 0  |
	                          | length 1 = 0  |
	                          | length 0 = 3  |
	interfacevftbllength ---> +---------------+

*******************************************************************************/

struct _vftbl {
	methodptr   *interfacetable[1];    /* interface table (access via macro)  */

	classinfo   *class;                /* class, the vtbl belongs to          */

	arraydescriptor *arraydesc;        /* for array classes, otherwise NULL   */

	s4           vftbllength;          /* virtual function table length       */
	s4           interfacetablelength; /* interface table length              */

	s4           baseval;              /* base for runtime type check         */
	                                   /* (-index for interfaces)             */
	s4           diffval;              /* high - base for runtime type check  */

	s4          *interfacevftbllength; /* length of interface vftbls          */
	
	methodptr    table[1];             /* class vftbl                         */
};

#define VFTBLINTERFACETABLE(v,i)       (v)->interfacetable[-i]


/* arraydescriptor ************************************************************

    For every array class an arraydescriptor is allocated which
    describes the array class.
	The arraydescriptor is referenced from the vftbl of the array
	class.

*******************************************************************************/

struct arraydescriptor {
	vftbl_t *componentvftbl; /* vftbl of the component type, NULL for primit. */
	vftbl_t *elementvftbl;   /* vftbl of the element type, NULL for primitive */
	s2       arraytype;      /* ARRAYTYPE_* constant                          */
	s2       dimension;      /* dimension of the array (always >= 1)          */
	s4       dataoffset;     /* offset of the array data from object pointer  */
	s4       componentsize;  /* size of a component in bytes                  */
	s2       elementtype;    /* ARRAYTYPE_* constant                          */
};


/* flag variables *************************************************************/

extern bool cacao_initializing;

#ifdef TYPECHECK_VERBOSE_OPT
extern bool typecheckverbose;
#endif

/*extern int pClassHeir;*/
/*extern int pCallgraph;*/
/*extern int pOpcodes;*/
/*extern int pStats;*/

/*extern void RT_jit_parse(methodinfo *m);*/


/* table of primitive types ***************************************************/

/* This array can be indexed by the PRIMITIVETYPE_ and ARRAYTYPE_
 * constants (except ARRAYTYPE_OBJECT).
 */
extern primitivetypeinfo primitivetype_table[PRIMITIVETYPE_COUNT];


/* macros for descriptor parsing **********************************************/

/* SKIP_FIELDDESCRIPTOR:
 * utf_ptr must point to the first character of a field descriptor.
 * After the macro call utf_ptr points to the first character after
 * the field descriptor.
 *
 * CAUTION: This macro does not check for an unexpected end of the
 * descriptor. Better use SKIP_FIELDDESCRIPTOR_SAFE.
 */
#define SKIP_FIELDDESCRIPTOR(utf_ptr)							\
	do { while (*(utf_ptr)=='[') (utf_ptr)++;					\
		if (*(utf_ptr)++=='L')									\
			while(*(utf_ptr)++ != ';') /* skip */; } while(0)

/* SKIP_FIELDDESCRIPTOR_SAFE:
 * utf_ptr must point to the first character of a field descriptor.
 * After the macro call utf_ptr points to the first character after
 * the field descriptor.
 *
 * Input:
 *     utf_ptr....points to first char of descriptor
 *     end_ptr....points to first char after the end of the string
 *     errorflag..must be initialized (to false) by the caller!
 * Output:
 *     utf_ptr....points to first char after the descriptor
 *     errorflag..set to true if the string ended unexpectedly
 */
#define SKIP_FIELDDESCRIPTOR_SAFE(utf_ptr,end_ptr,errorflag)			\
	do { while ((utf_ptr) != (end_ptr) && *(utf_ptr)=='[') (utf_ptr)++;	\
		if ((utf_ptr) == (end_ptr))										\
			(errorflag) = true;											\
		else															\
			if (*(utf_ptr)++=='L') {									\
				while((utf_ptr) != (end_ptr) && *(utf_ptr)++ != ';')	\
					/* skip */;											\
				if ((utf_ptr)[-1] != ';')								\
					(errorflag) = true; }} while(0)


/* Synchronization ************************************************************/

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void cast_lock();
void cast_unlock();
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
 */
