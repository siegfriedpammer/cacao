/* global.h ********************************************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Contains global definitions which are used in the whole program, includes
	some files and contains global used macros.

	Authors: Reinhard Grafl              EMAIL: cacao@complang.tuwien.ac.at
	         Andreas  Krall   (andi)     EMAIL: cacao@complang.tuwien.ac.at
	Changes: Mark     Probst  (schani)   EMAIL: cacao@complang.tuwien.ac.at
			 Philipp  Tomsich (phil)     EMAIL: cacao@complang.tuwien.ac.at

	Last Change: $Id: global.h 390 2003-07-15 15:52:21Z carolyn $

*******************************************************************************/

#ifndef __global_h_
#define __global_h_

#include "config.h"

#define NEW_GC              /* if enabled, includes the new gc. -- phil.      */

#define STATISTICS          /* if enabled collects program statistics         */

/* 
 * JIT_MARKER_SUPPORT is the define used to toggle Just-in-time generated
 * marker functions on and off.
 *
 * SIZE_FROM_CLASSINFO toggles between the bitmap_based and the new method 
 * of determining the sizes of objects on the heap.
 */
#undef JIT_MARKER_SUPPORT        /* phil */
#define SIZE_FROM_CLASSINFO

/* standard includes **********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "toolbox/memory.h"
#include "toolbox/chain.h"
#include "toolbox/list.h"
#include "toolbox/loging.h"

/* system dependent types *****************************************************/

#include "types.h"


/* additional data types ******************************************************/

typedef void *voidptr;          /* generic pointer */

typedef int   bool;             /* boolean data type */

#define true  1
#define false 0

#define PRIMITIVETYPE_COUNT  9  /* number of primitive types */

typedef void (*functionptr) (); /* generic function pointer */


#define MAX_ALIGN 8             /* most generic alignment for JavaVM values   */

/* shutdown function **********************************************************/

void cacao_shutdown(s4 status);


/* basic data types ***********************************************************/

#define TYPE_INT      0         /* the JavaVM types must numbered in the      */
#define TYPE_LONG     1         /* same order as the ICMD_Ixxx to ICMD_Axxx   */
#define TYPE_FLOAT    2         /* instructions (LOAD and STORE)              */
#define TYPE_DOUBLE   3         /* integer, long, float, double, address      */
#define TYPE_ADDRESS  4         /* all other types can be numbered arbitrarly */

#define TYPE_VOID    10


/* Java class file constants **************************************************/

#define MAGIC         0xcafebabe
#define MINOR_VERSION 3
#define MAJOR_VERSION 45

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

#define CONSTANT_Arraydescriptor      13
#define CONSTANT_UNUSED                0

#define ACC_PUBLIC                0x0001
#define ACC_PRIVATE               0x0002
#define ACC_PROTECTED             0x0004
#define ACC_STATIC                0x0008
#define ACC_FINAL                 0x0010
#define ACC_SYNCHRONIZED          0x0020
#define ACC_VOLATILE              0x0040
#define ACC_TRANSIENT             0x0080
#define ACC_NATIVE                0x0100
#define ACC_INTERFACE             0x0200
#define ACC_ABSTRACT              0x0400


/* resolve typedef cycles *****************************************************/

typedef struct utf utf;
typedef struct literalstring literalstring;
typedef struct java_objectheader java_objectheader; 
typedef struct classinfo classinfo; 
typedef struct vftbl vftbl;
typedef u1* methodptr;


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
    CONSTANT_Arraydescriptor     constant_arraydescriptor           yes
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

/* data structure for accessing hashtables ************************************/

typedef struct {            
  u4 size;
  u4 entries;        /* number of entries in the table */
  void **ptr;        /* pointer to hashtable */
} hashtable;

/* data structures of remaining constant pool entries *************************/

typedef struct {                /* Fieldref, Methodref and InterfaceMethodref     */
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

/*  arraydescriptor describes array types. Basic array types contain their
	type in the arraytype field, objectclass contains a class pointer for
	arrays of objects (arraytype == ARRAYTYPE_OBJECT), elementdescriptor
	contains a pointer to an arraydescriptor which describes the element
	types in the case of arrays of arrays (arraytype == ARRAYTYPE_ARRAY).
*/

typedef struct constant_arraydescriptor {
	int arraytype;
	classinfo *objectclass;
	struct constant_arraydescriptor *elementdescriptor;
} constant_arraydescriptor;


/* data structures of the runtime system **************************************/

/* objects *********************************************************************

	All objects (and arrays) which resides on the heap need the following
	header at the beginning of the data structure.
*/

struct java_objectheader {              /* header for all objects             */
	vftbl *vftbl;                       /* pointer to virtual function table  */
};



/* arrays **********************************************************************

	All arrays are objects (they need the object header with a pointer to a
	vvftbl (array class table). There is only one class for all arrays. The
	type of an array is stored directly in the array object. Following types
	are defined:
*/

#define ARRAYTYPE_INT      0
#define ARRAYTYPE_LONG     1
#define ARRAYTYPE_FLOAT    2
#define ARRAYTYPE_DOUBLE   3
#define ARRAYTYPE_BYTE     4
#define ARRAYTYPE_CHAR     5
#define ARRAYTYPE_SHORT    6
#define ARRAYTYPE_BOOLEAN  7
#define ARRAYTYPE_OBJECT   8
#define ARRAYTYPE_ARRAY    9

typedef struct java_arrayheader {       /* header for all arrays              */
	java_objectheader objheader;        /* object header                      */
	s4 size;                            /* array size                         */
#ifdef SIZE_FROM_CLASSINFO
	s4 alignedsize; /* phil */
#endif
	s4 arraytype;                       /* array type from previous list      */
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
	classinfo *elementtype;
	java_objectheader *data[1];
} java_objectarray;

typedef struct java_arrayarray {
	java_arrayheader header;
	constant_arraydescriptor *elementdescriptor;
	java_arrayheader *data[1];
} java_arrayarray;


/* structure for primitive classes ********************************************/

typedef struct primitivetypeinfo {
	classinfo *class_wrap;               /* class for wrapping primitive type */
	classinfo *class_primitive;          /* primitive class                   */
	char *wrapname;                      /* name of class for wrapping        */
	char typesig;                        /* one character type signature      */
	char *name;                          /* name of primitive class           */
} primitivetypeinfo;

/* field, method and class structures *****************************************/

/* fieldinfo ******************************************************************/

typedef struct fieldinfo {/* field of a class                                 */
	s4       flags;       /* ACC flags                                        */
	s4       type;        /* basic data type                                  */
	utf *name;            /* name of field                                    */
	utf *descriptor;      /* JavaVM descriptor string of field                */
	
	s4       offset;      /* offset from start of object (instance variables) */

	union {               /* storage for static values (class variables)      */
		s4 i; 
		s8 l;
		float f;
		double d;
		void *a; 
	} value;

} fieldinfo;

struct basicblock;

/* exceptiontable *************************************************************/

typedef struct xtable { /* exceptiontable entry in a method           */ 
	s4         startpc;         /* start pc of guarded area (inclusive)       */
	struct basicblock *start;

	s4         endpc;           /* end pc of guarded area (exklusive)         */
	struct basicblock *end;

	s4         handlerpc;       /* pc of exception handler                    */
	struct basicblock *handler;

	classinfo *catchtype;       /* catchtype of exception (NULL == catchall)  */
	struct xtable *next;        /* used to build a list of exception when     */
	                            /* loops are copied */
	struct xtable *down;        /* instead of the old array, a list is used   */
} xtable;


typedef struct exceptiontable { /* exceptiontable entry in a method           */ 
	s4         startpc;         /* start pc of guarded area (inclusive)       */
	s4         endpc;           /* end pc of guarded area (exklusive)         */
	s4         handlerpc;       /* pc of exception handler                    */
	classinfo *catchtype;       /* catchtype of exception (NULL == catchall)  */
} exceptiontable;


/* methodinfo *****************************************************************/

typedef struct methodinfo {         /* method structure                       */
	s4	       flags;               /* ACC flags                              */
	utf       *name;                /* name of method                         */
	utf       *descriptor;          /* JavaVM descriptor string of method     */
	s4         returntype;          /* only temporary valid, return type      */
	s4         paramcount;          /* only temporary valid, parameter count  */
	u1        *paramtypes;          /* only temporary valid, parameter types  */
	classinfo *class;               /* class, the method belongs to           */
	s4         vftblindex;          /* index of method in virtual function table
	                                   (if it is a virtual method)            */
	s4         maxstack;            /* maximum stack depth of method          */
	s4         maxlocals;           /* maximum number of local variables      */
	s4         jcodelength;         /* length of JavaVM code                  */
	u1        *jcode;               /* pointer to JavaVM code                 */

	s4         exceptiontablelength;/* exceptiontable length                  */
	exceptiontable *exceptiontable; 
                                    /* the exceptiontable                     */

	u1        *stubroutine;         /* stub for compiling or calling natives  */	
	s4         mcodelength;         /* legth of generated machine code        */
	u1        *mcode;               /* pointer to machine code                */
	u1        *entrypoint;          /* entry point in machine code            */

        s4        methodUsed; 		/* -1=marked (might be used) 0=not used 1=used CO-RT*/
	s4	  numSubDefs;		/* # sub definitions marked USED          */

	s4          natCalls;     	/* number of methods  calls               */
	
//	methodinfo *natCalledMethods;   /* methods called table                   */
        
	s4          XTAclasscount;     /* number of classes in XTA class set       */
	classinfo   *XTAclassSet;       /* XTA class set*/


} methodinfo;


/* innerclassinfo *************************************************************/

typedef struct innerclassinfo {
	classinfo *inner_class;       /* inner class pointer                      */
	classinfo *outer_class;       /* outer class pointer                      */
	utf *name;                    /* innerclass name                          */ 
	s4 flags;                     /* ACC flags                                */
} innerclassinfo;

/* classinfo ******************************************************************/

struct classinfo {                /* class structure                          */
	java_objectheader header;     /* classes are also objects                 */

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

	bool        initialized;      /* true, if class already initialised       */ 
	bool        linked;           /* true, if class already linked            */
	s4	    index;            /* hierarchy depth (classes) or index
	                                 (interfaces)                             */ 
	s4          instancesize;     /* size of an instance of this class        */
#ifdef SIZE_FROM_CLASSINFO
	s4          alignedsize;      /* size of an instance, aligned to the 
									 allocation size on the heap */
#endif

	vftbl      *vftbl;            /* pointer to virtual function table        */

	methodinfo *finalizer;        /* finalizer method                         */
#ifdef JIT_MARKER_SUPPORT
	methodinfo *marker; 
#endif

    u2             innerclasscount;   /* number of inner classes              */
    innerclassinfo *innerclass;

    classinfo      *hashlink;         /* link for external hash chain         */
	bool        classvftbl;       /* has its own copy of the Class vtbl       */

        s4          classUsed;        /* 0= not used 1 = used   CO-RT             */

	classinfo  *impldBy;          /* implemented by class pointer             */
	classinfo  *nextimpldBy;      /* ptr to next class in impldBy class list  */

};


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

struct vftbl {
	methodptr   *interfacetable[1];    /* interface table (access via macro)  */

	classinfo   *class;                /* class, the vtbl belongs to          */

	s4           vftbllength;          /* virtual function table length       */
	s4           interfacetablelength; /* interface table length              */

	s4           baseval;              /* base for runtime type check         */
	s4           diffval;              /* high - base for runtime type check  */

	s4          *interfacevftbllength; /* length of interface vftbls          */
	
	methodptr    table[1];             /* class vftbl                         */
};

#define VFTBLINTERFACETABLE(v,i)       (v)->interfacetable[-i]


/* references to some system classes ******************************************/

extern classinfo *class_java_lang_Object;
extern classinfo *class_java_lang_String;
extern classinfo *class_java_lang_ClassCastException;
extern classinfo *class_java_lang_NullPointerException;
extern classinfo *class_java_lang_ArrayIndexOutOfBoundsException;
extern classinfo *class_java_lang_NegativeArraySizeException;
extern classinfo *class_java_lang_OutOfMemoryError;
extern classinfo *class_java_lang_ArithmeticException;
extern classinfo *class_java_lang_ArrayStoreException;
extern classinfo *class_java_lang_ThreadDeath;
extern classinfo *class_array;

/* instances of some system classes *******************************************/

extern java_objectheader *proto_java_lang_ClassCastException;
extern java_objectheader *proto_java_lang_NullPointerException;
extern java_objectheader *proto_java_lang_ArrayIndexOutOfBoundsException;
extern java_objectheader *proto_java_lang_NegativeArraySizeException;
extern java_objectheader *proto_java_lang_OutOfMemoryError;
extern java_objectheader *proto_java_lang_ArithmeticException;
extern java_objectheader *proto_java_lang_ArrayStoreException;
extern java_objectheader *proto_java_lang_ThreadDeath;


/* flag variables *************************************************************/

extern bool compileall;
extern bool runverbose;         
extern bool verbose;         
extern bool opt_rt;             /* Rapid Type Analysis for better inlining CO-RT*/
extern bool opt_xta;            /* X Type Analysis for better inlining    CO-XTA*/

extern int pClassHeir;
extern int pCallgraph;
extern int pOpcodes;
extern int pStats;

extern void RT_jit_parse(methodinfo *m);
extern void printCallgraph ();
extern void printRThierarchyInfo(methodinfo *m);
extern void printObjectClassHeirarchy();

extern void XTA_jit_parse(methodinfo *m);

/* statistic variables ********************************************************/

extern int count_class_infos;
extern int count_const_pool_len;
extern int count_vftbl_len;
extern int count_utf_len;
extern int count_all_methods;
extern int count_vmcode_len;
extern int count_extable_len;
extern int count_class_loads;
extern int count_class_inits;
extern int count_utf_new;
extern int count_utf_new_found;

/* table of primitive types ***************************************************/

extern primitivetypeinfo primitivetype_table[PRIMITIVETYPE_COUNT];

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
