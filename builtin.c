/****************************** builtin.c **************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Enthaelt die C-Funktionen fuer alle JavaVM-Befehle, die sich nicht direkt
	auf Maschinencode "ubersetzen lassen. Im Code f"ur die Methoden steht
	dann ein Funktionsaufruf (nat"urlich mit den Aufrufskonventionen von C).

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
	         Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at
	         Mark Probst         EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1996/12/03

*******************************************************************************/

#include <assert.h>
#include <values.h>

#include "global.h"
#include "builtin.h"

#include "loader.h"
#include "tables.h"

#include "threads/thread.h"
#include "threads/locks.h"              /* schani */

#include "sysdep/native-math.h"

builtin_descriptor builtin_desc[] = {
	{(functionptr) builtin_instanceof,         "instanceof"},
	{(functionptr) builtin_checkcast,          "checkcast"},
	{(functionptr) builtin_arrayinstanceof,    "arrayinstanceof"},
	{(functionptr) builtin_checkarraycast,     "checkarraycast"},
	{(functionptr) asm_builtin_checkarraycast, "checkarraycast"},
	{(functionptr) asm_builtin_aastore,        "aastore"},
	{(functionptr) builtin_new,                "new"},
	{(functionptr) builtin_anewarray,          "anewarray"},
	{(functionptr) builtin_newarray_array,     "newarray_array"},
	{(functionptr) builtin_newarray_boolean,   "newarray_boolean"},
	{(functionptr) builtin_newarray_char,      "newarray_char"},
	{(functionptr) builtin_newarray_float,     "newarray_float"},
	{(functionptr) builtin_newarray_double,    "newarray_double"},
	{(functionptr) builtin_newarray_byte,      "newarray_byte"},
	{(functionptr) builtin_newarray_short,     "newarray_short"},
	{(functionptr) builtin_newarray_int,       "newarray_int"},
	{(functionptr) builtin_newarray_long,      "newarray_long"},
	{(functionptr) builtin_displaymethodstart, "displaymethodstart"},
	{(functionptr) builtin_displaymethodstop,  "displaymethodstop"},
	{(functionptr) builtin_monitorenter,       "monitorenter"},
	{(functionptr) asm_builtin_monitorenter,   "monitorenter"},
	{(functionptr) builtin_monitorexit,        "monitorexit"},
	{(functionptr) asm_builtin_monitorexit,    "monitorexit"},
	{(functionptr) builtin_idiv,               "idiv"},
	{(functionptr) asm_builtin_idiv,           "idiv"},
	{(functionptr) builtin_irem,               "irem"},
	{(functionptr) asm_builtin_irem,           "irem"},
	{(functionptr) builtin_ladd,               "ladd"},
	{(functionptr) builtin_lsub,               "lsub"},
	{(functionptr) builtin_lmul,               "lmul"},
	{(functionptr) builtin_ldiv,               "ldiv"},
	{(functionptr) asm_builtin_ldiv,           "ldiv"},
	{(functionptr) builtin_lrem,               "lrem"},
	{(functionptr) asm_builtin_lrem,           "lrem"},
	{(functionptr) builtin_lshl,               "lshl"},
	{(functionptr) builtin_lshr,               "lshr"},
	{(functionptr) builtin_lushr,              "lushr"},
	{(functionptr) builtin_land,               "land"},
	{(functionptr) builtin_lor,                "lor"},
	{(functionptr) builtin_lxor,               "lxor"},
	{(functionptr) builtin_lneg,               "lneg"},
	{(functionptr) builtin_lcmp,               "lcmp"},
	{(functionptr) builtin_fadd,               "fadd"},
	{(functionptr) builtin_fsub,               "fsub"},
	{(functionptr) builtin_fmul,               "fmul"},
	{(functionptr) builtin_fdiv,               "fdiv"},
	{(functionptr) builtin_frem,               "frem"},
	{(functionptr) builtin_fneg,               "fneg"},
	{(functionptr) builtin_fcmpl,              "fcmpl"},
	{(functionptr) builtin_fcmpg,              "fcmpg"},
	{(functionptr) builtin_dadd,               "dadd"},
	{(functionptr) builtin_dsub,               "dsub"},
	{(functionptr) builtin_dmul,               "dmul"},
	{(functionptr) builtin_ddiv,               "ddiv"},
	{(functionptr) builtin_drem,               "drem"},
	{(functionptr) builtin_dneg,               "dneg"},
	{(functionptr) builtin_dcmpl,              "dcmpl"},
	{(functionptr) builtin_dcmpg,              "dcmpg"},
	{(functionptr) builtin_i2l,                "i2l"},
	{(functionptr) builtin_i2f,                "i2f"},
	{(functionptr) builtin_i2d,                "i2d"},
	{(functionptr) builtin_l2i,                "l2i"},
	{(functionptr) builtin_l2f,                "l2f"},
	{(functionptr) builtin_l2d,                "l2d"},
	{(functionptr) builtin_f2i,                "f2i"},
	{(functionptr) builtin_f2l,                "f2l"},
	{(functionptr) builtin_f2d,                "f2d"},
	{(functionptr) builtin_d2i,                "d2i"},
	{(functionptr) builtin_d2l,                "d2l"},
	{(functionptr) builtin_d2f,                "d2f"},
	{(functionptr) NULL,                       "unknown"}
	};


/*****************************************************************************
                                TYPCHECKS
*****************************************************************************/



/*************** interne Funktion: builtin_isanysubclass *********************

	"uberpr"uft, ob eine Klasse eine Unterklasse einer anderen Klasse ist.
	Dabei gelten auch Interfaces, die eine Klasse implementiert, als
	deren Oberklassen. 
	R"uckgabewert:  1 ... es trifft zu
	                0 ... es trifft nicht zu
	                
*****************************************************************************/	                

static s4 builtin_isanysubclass (classinfo *sub, classinfo *super)
{
	if (super->flags & ACC_INTERFACE)
		return (sub->vftbl->interfacetablelength > super->index) &&
		       (sub->vftbl->interfacetable[-super->index] != NULL);

	return (unsigned) (sub->vftbl->baseval - super->vftbl->baseval) <=
	       (unsigned) (super->vftbl->diffval);
}


/****************** Funktion: builtin_instanceof *****************************

	"Uberpr"uft, ob ein Objekt eine Instanz einer Klasse (oder einer davon
	abgeleiteten Klasse) ist, oder ob die Klasse des Objekts ein Interface 
	implementiert.
	Return:   1, wenn ja
	          0, wenn nicht, oder wenn Objekt ein NULL-Zeiger
	         
*****************************************************************************/

s4 builtin_instanceof(java_objectheader *obj, classinfo *class)
{
#ifdef DEBUG
	log_text ("builtin_instanceof called");
#endif
	
	if (!obj) return 0;
	return builtin_isanysubclass (obj->vftbl->class, class);
}



/**************** Funktion: builtin_checkcast *******************************

	"Uberpr"uft, ob ein Objekt eine Instanz einer Klasse (oder einer davon
	abgeleiteten Klasse ist).
	Unterschied zu builtin_instanceof: Ein NULL-Zeiger ist immer richtig
	Return:   1, wenn ja, oder wenn Objekt ein NULL-Zeiger 
              0, wenn nicht
              
****************************************************************************/

s4 builtin_checkcast(java_objectheader *obj, classinfo *class)
{
#ifdef DEBUG
	log_text ("builtin_checkcast called");
#endif

	if (obj == NULL)
		return 1;
	if (builtin_isanysubclass (obj->vftbl->class, class))
		return 1;

#if DEBUG
	printf ("#### checkcast failed ");
	unicode_display (obj->vftbl->class->name);
	printf (" -> ");
	unicode_display (class->name);
	printf ("\n");
#endif

	return 0;
}


/*********** interne Funktion: builtin_descriptorscompatible ******************

	"uberpr"uft, ob zwei Array-Typdescriptoren compartible sind, d.h.,
	ob ein Array vom Typ 'desc' gefahrlos einer Variblen vom Typ 'target'
	zugewiesen werden kann.
	Return: 1, wenn ja
	        0, wenn nicht
	        
******************************************************************************/

static s4 builtin_descriptorscompatible
	(constant_arraydescriptor *desc, constant_arraydescriptor *target)
{
	if (desc==target) return 1;
	if (desc->arraytype != target->arraytype) return 0;
	switch (target->arraytype) {
		case ARRAYTYPE_OBJECT: 
			return builtin_isanysubclass (desc->objectclass, target->objectclass);
		case ARRAYTYPE_ARRAY:
			return builtin_descriptorscompatible 
			  (desc->elementdescriptor, target->elementdescriptor);
		default: return 1;
		}
}



/******************** Funktion: builtin_checkarraycast ***********************

	"uberpr"uft, ob ein gegebenes Objekt tats"achlich von einem 
	Untertyp des geforderten Arraytyps ist.
	Dazu muss das Objekt auf jeden Fall ein Array sein. 
	Bei einfachen Arrays (int,short,double,etc.) muss der Typ genau 
	"ubereinstimmen.
	Bei Arrays von Objekten muss der Elementtyp des tats"achlichen Arrays
	ein Untertyp (oder der selbe Typ) vom geforderten Elementtyp sein.
	Bei Arrays vom Arrays (die eventuell wieder Arrays von Arrays
	sein k"onnen) m"ussen die untersten Elementtypen in der entsprechenden
	Unterklassenrelation stehen.

	Return: 1, wenn Cast in Ordung ist
			0, wenn es nicht geht
	
	Achtung: ein Cast mit einem NULL-Zeiger geht immer gut.
			
*****************************************************************************/

s4 builtin_checkarraycast(java_objectheader *o, constant_arraydescriptor *desc)
{
	java_arrayheader *a = (java_arrayheader*) o;

	if (!o) return 1;
	if (o->vftbl->class != class_array) {
		return 0;
		}
		
	if (a->arraytype != desc->arraytype) {
		return 0;
		}
	
	switch (a->arraytype) {
		case ARRAYTYPE_OBJECT: {
			java_objectarray *oa = (java_objectarray*) o;
			return builtin_isanysubclass (oa->elementtype, desc->objectclass);
			}
		case ARRAYTYPE_ARRAY: {
			java_arrayarray *aa = (java_arrayarray*) o;
			return builtin_descriptorscompatible
			   (aa->elementdescriptor, desc->elementdescriptor);
			}
		default:   
			return 1;
		}
}


s4 builtin_arrayinstanceof
	(java_objectheader *obj, constant_arraydescriptor *desc)
{
	if (!obj) return 1;
	return builtin_checkarraycast (obj, desc);
}


/************************** exception functions *******************************

******************************************************************************/

java_objectheader *builtin_throw_exception (java_objectheader *exceptionptr) {
	unicode_display (exceptionptr->vftbl->class->name);
	printf ("\n");
	fflush (stdout);
	return exceptionptr;
}


/******************* Funktion: builtin_canstore *******************************

	"uberpr"uft, ob ein Objekt in einem Array gespeichert werden 
	darf.
	Return: 1, wenn es geht
			0, wenn nicht

******************************************************************************/


s4 builtin_canstore (java_objectarray *a, java_objectheader *o)
{
	if (!o) return 1;
	
	switch (a->header.arraytype) {
	case ARRAYTYPE_OBJECT:
		if ( ! builtin_checkcast (o, a->elementtype) ) {
			return 0;
			}
		return 1;
		break;

	case ARRAYTYPE_ARRAY:
		if ( ! builtin_checkarraycast 
		         (o, ((java_arrayarray*)a)->elementdescriptor) ) {
			return 0;
			}
		return 1;
		break;

	default:
		panic ("builtin_canstore called with invalid arraytype");
		return 0;
	}
}



/*****************************************************************************
                          ARRAYOPERATIONEN
*****************************************************************************/



/******************** Funktion: builtin_new **********************************

	Legt ein neues Objekt einer Klasse am Heap an.
	Return: Der Zeiger auf das Objekt, oder NULL, wenn kein Speicher
			mehr frei ist.
			
*****************************************************************************/

#define ALIGNMENT 3
#define align_size(size)	((size + ((1 << ALIGNMENT) - 1)) & ~((1 << ALIGNMENT) - 1))

java_objectheader *builtin_new (classinfo *c)
{
	java_objectheader *o;

#ifdef SIZE_FROM_CLASSINFO
	c->alignedsize = align_size(c->instancesize);
	o = heap_allocate ( c->alignedsize, true, c->finalizer );
#else
	o = heap_allocate ( c->instancesize, true, c->finalizer );
#endif
	if (!o) return NULL;
	
	memset (o, 0, c->instancesize);

	o -> vftbl = c -> vftbl;
	return o;
}



/******************** Funktion: builtin_anewarray ****************************

	Legt ein Array von Zeigern auf Objekte am Heap an.
	Parameter: 
		size ......... Anzahl der Elemente
		elementtype .. ein Zeiger auf die classinfo-Struktur des Typs
		               der Elemente

	Return: Zeiger auf das Array, oder NULL (wenn kein Speicher frei)

*****************************************************************************/

static
void* __builtin_newarray(s4 base_size,
						 s4 size, 
						 bool references,
						 int elementsize,
						 int arraytype)
{
	java_arrayheader *a;
#ifdef SIZE_FROM_CLASSINFO
	s4 alignedsize = align_size(base_size + (size-1) * elementsize);
	a = heap_allocate ( alignedsize, true, NULL );
#else	
	a = heap_allocate ( sizeof(java_objectarray) + (size-1) * elementsize, 
	                    references, 
						NULL);
#endif
	if (!a) return NULL;

#ifdef SIZE_FROM_CLASSINFO
	memset(a, 0, alignedsize);
#else
	memset(a, 0, base_size + (size-1) * elementsize);
#endif	

	a -> objheader.vftbl = class_array -> vftbl;
	a -> size = size;
#ifdef SIZE_FROM_CLASSINFO
	a -> alignedsize = alignedsize;
#endif
	a -> arraytype = arraytype;

	return a;
}


java_objectarray *builtin_anewarray (s4 size, classinfo *elementtype)
{
	java_objectarray *a;	
	a = (java_objectarray*)__builtin_newarray(sizeof(java_objectarray),
											  size, 
											  true, 
											  sizeof(void*), 
											  ARRAYTYPE_OBJECT);
	if (!a) return NULL;

	a -> elementtype = elementtype;
	return a;
}



/******************** Funktion: builtin_newarray_array ***********************

	Legt ein Array von Zeigern auf Arrays am Heap an.
	Paramter: size ......... Anzahl der Elemente
	          elementdesc .. Zeiger auf die Arraybeschreibungs-Struktur f"ur
	                         die Element-Arrays.

	Return: Zeiger auf das Array, oder NULL

*****************************************************************************/

java_arrayarray *builtin_newarray_array 
        (s4 size, constant_arraydescriptor *elementdesc)
{
	java_arrayarray *a;	
	a = (java_arrayarray*)__builtin_newarray(sizeof(java_arrayarray),
											 size, 
											 true, 
											 sizeof(void*), 
											 ARRAYTYPE_ARRAY);
	if (!a) return NULL;

	a -> elementdescriptor = elementdesc;
	return a;
}


/******************** Funktion: builtin_newarray_boolean ************************

	Legt ein Array von Bytes am Heap an, das allerdings als Boolean-Array 
	gekennzeichnet wird (wichtig bei Casts!)

	Return: Zeiger auf das Array, oder NULL 

*****************************************************************************/

java_booleanarray *builtin_newarray_boolean (s4 size)
{
	java_booleanarray *a;	
	a = (java_booleanarray*)__builtin_newarray(sizeof(java_booleanarray),
											   size, 
											   false, 
											   sizeof(u1), 
											   ARRAYTYPE_BOOLEAN);
	return a;
}

/******************** Funktion: builtin_newarray_char ************************

	Legt ein Array von 16-bit-Integers am Heap an.
	Return: Zeiger auf das Array, oder NULL 

*****************************************************************************/

java_chararray *builtin_newarray_char (s4 size)
{
	java_chararray *a;	
	a = (java_chararray*)__builtin_newarray(sizeof(java_chararray),
											size, 
											false, 
											sizeof(u2), 
											ARRAYTYPE_CHAR);
	return a;
}


/******************** Funktion: builtin_newarray_float ***********************

	Legt ein Array von 32-bit-IEEE-float am Heap an.
	Return: Zeiger auf das Array, oder NULL 

*****************************************************************************/

java_floatarray *builtin_newarray_float (s4 size)
{
	java_floatarray *a;	
	a = (java_floatarray*)__builtin_newarray(sizeof(java_floatarray),
											 size, 
											 false, 
											 sizeof(float), 
											 ARRAYTYPE_FLOAT);
	return a;
}


/******************** Funktion: builtin_newarray_double ***********************

	Legt ein Array von 64-bit-IEEE-float am Heap an.
	Return: Zeiger auf das Array, oder NULL 

*****************************************************************************/

java_doublearray *builtin_newarray_double (s4 size)
{
	java_doublearray *a;	
	a = (java_doublearray*)__builtin_newarray(sizeof(java_doublearray),
											  size, 
											  false, 
											  sizeof(double), 
											  ARRAYTYPE_DOUBLE);
	return a;
}




/******************** Funktion: builtin_newarray_byte ***********************

	Legt ein Array von 8-bit-Integers am Heap an.
	Return: Zeiger auf das Array, oder NULL 

*****************************************************************************/

java_bytearray *builtin_newarray_byte (s4 size)
{
	java_bytearray *a;	
	a = (java_bytearray*)__builtin_newarray(sizeof(java_bytearray),
											size, 
											false, 
											sizeof(u1), 
											ARRAYTYPE_BYTE);
	return a;
}


/******************** Funktion: builtin_newarray_short ***********************

	Legt ein Array von 16-bit-Integers am Heap an.
	Return: Zeiger auf das Array, oder NULL 

*****************************************************************************/

java_shortarray *builtin_newarray_short (s4 size)
{
	java_shortarray *a;	
	a = (java_shortarray*)__builtin_newarray(sizeof(java_shortarray),
											   size, 
											   false, 
											   sizeof(s2), 
											   ARRAYTYPE_SHORT);
	return a;
}


/******************** Funktion: builtin_newarray_int ***********************

	Legt ein Array von 32-bit-Integers am Heap an.
	Return: Zeiger auf das Array, oder NULL 

*****************************************************************************/

java_intarray *builtin_newarray_int (s4 size)
{
	java_intarray *a;	
	a = (java_intarray*)__builtin_newarray(sizeof(java_intarray),
										   size, 
										   false, 
										   sizeof(s4), 
										   ARRAYTYPE_INT);
	return a;
}


/******************** Funktion: builtin_newarray_long ***********************

	Legt ein Array von 64-bit-Integers am Heap an.
	Return: Zeiger auf das Array, oder NULL 

*****************************************************************************/

java_longarray *builtin_newarray_long (s4 size)
{
	java_longarray *a;	
	a = (java_longarray*)__builtin_newarray(sizeof(java_longarray),
											size, 
											false, 
											sizeof(s8), 
											ARRAYTYPE_LONG);
	return a;
}



/***************** Funktion: builtin_multianewarray ***************************

	Legt ein mehrdimensionales Array am Heap an.
	Die Gr"ossen der einzelnen Dimensionen werden in einem Integerarray
	"ubergeben. Der Typ es zu erzeugenden Arrays wird als 
	Referenz auf eine constant_arraydescriptor - Struktur "ubergeben.

	Return: Ein Zeiger auf das Array, oder NULL, wenn kein Speicher mehr
	        vorhanden ist.

******************************************************************************/

	/* Hilfsfunktion */

static java_arrayheader *multianewarray_part (java_intarray *dims, int thisdim,
                       constant_arraydescriptor *desc)
{
	u4 size,i;
	java_arrayarray *a;

	size = dims -> data[thisdim];
	
	if (thisdim == (dims->header.size-1)) {
		/* letzte Dimension schon erreicht */
		
		switch (desc -> arraytype) {
		case ARRAYTYPE_BOOLEAN:  
			return (java_arrayheader*) builtin_newarray_boolean (size); 
		case ARRAYTYPE_CHAR:  
			return (java_arrayheader*) builtin_newarray_char (size); 
		case ARRAYTYPE_FLOAT:  
			return (java_arrayheader*) builtin_newarray_float (size); 
		case ARRAYTYPE_DOUBLE:  
			return (java_arrayheader*) builtin_newarray_double (size); 
		case ARRAYTYPE_BYTE:  
			return (java_arrayheader*) builtin_newarray_byte (size); 
		case ARRAYTYPE_SHORT:  
			return (java_arrayheader*) builtin_newarray_short (size); 
		case ARRAYTYPE_INT:  
			return (java_arrayheader*) builtin_newarray_int (size); 
		case ARRAYTYPE_LONG:  
			return (java_arrayheader*) builtin_newarray_long (size); 
		case ARRAYTYPE_OBJECT:
			return (java_arrayheader*) builtin_anewarray (size, desc->objectclass);
		
		case ARRAYTYPE_ARRAY:
			return (java_arrayheader*) builtin_newarray_array (size, desc->elementdescriptor);
		
		default: panic ("Invalid arraytype in multianewarray");
		}
		}

	/* wenn letzte Dimension noch nicht erreicht wurde */

	if (desc->arraytype != ARRAYTYPE_ARRAY) 
		panic ("multianewarray with too many dimensions");

	a = builtin_newarray_array (size, desc->elementdescriptor);
	if (!a) return NULL;
	
	for (i=0; i<size; i++) {
		java_arrayheader *ea = 
		  multianewarray_part (dims, thisdim+1, desc->elementdescriptor);
		if (!ea) return NULL;

		a -> data[i] = ea;
		}
		
	return (java_arrayheader*) a;
}


java_arrayheader *builtin_multianewarray (java_intarray *dims,
                      constant_arraydescriptor *desc)
{
	return multianewarray_part (dims, 0, desc);
}


static java_arrayheader *nmultianewarray_part (int n, long *dims, int thisdim,
                       constant_arraydescriptor *desc)
{
	int size, i;
	java_arrayarray *a;

	size = (int) dims[thisdim];
	
	if (thisdim == (n - 1)) {
		/* letzte Dimension schon erreicht */
		
		switch (desc -> arraytype) {
		case ARRAYTYPE_BOOLEAN:  
			return (java_arrayheader*) builtin_newarray_boolean(size); 
		case ARRAYTYPE_CHAR:  
			return (java_arrayheader*) builtin_newarray_char(size); 
		case ARRAYTYPE_FLOAT:  
			return (java_arrayheader*) builtin_newarray_float(size); 
		case ARRAYTYPE_DOUBLE:  
			return (java_arrayheader*) builtin_newarray_double(size); 
		case ARRAYTYPE_BYTE:  
			return (java_arrayheader*) builtin_newarray_byte(size); 
		case ARRAYTYPE_SHORT:  
			return (java_arrayheader*) builtin_newarray_short(size); 
		case ARRAYTYPE_INT:  
			return (java_arrayheader*) builtin_newarray_int(size); 
		case ARRAYTYPE_LONG:  
			return (java_arrayheader*) builtin_newarray_long(size); 
		case ARRAYTYPE_OBJECT:
			return (java_arrayheader*) builtin_anewarray(size,
			                           desc->objectclass);
		case ARRAYTYPE_ARRAY:
			return (java_arrayheader*) builtin_newarray_array(size,
			                           desc->elementdescriptor);
		
		default: panic ("Invalid arraytype in multianewarray");
		}
		}

	/* wenn letzte Dimension noch nicht erreicht wurde */

	if (desc->arraytype != ARRAYTYPE_ARRAY) 
		panic ("multianewarray with too many dimensions");

	a = builtin_newarray_array(size, desc->elementdescriptor);
	if (!a) return NULL;
	
	for (i = 0; i < size; i++) {
		java_arrayheader *ea = 
			nmultianewarray_part(n, dims, thisdim + 1, desc->elementdescriptor);
		if (!ea) return NULL;

		a -> data[i] = ea;
		}
		
	return (java_arrayheader*) a;
}


java_arrayheader *builtin_nmultianewarray (int size,
                      constant_arraydescriptor *desc, long *dims)
{
	(void) builtin_newarray_int(size); /* for compatibility with -old */
	return nmultianewarray_part (size, dims, 0, desc);
}




/************************* Funktion: builtin_aastore *************************

	speichert eine Referenz auf ein Objekt in einem Object-Array oder
	in einem Array-Array.
	Dabei wird allerdings vorher "uberpr"uft, ob diese Operation 
	zul"assig ist.

	Return: 1, wenn alles OK ist
	        0, wenn dieses Objekt nicht in dieses Array gespeichert werden
	           darf

*****************************************************************************/

s4 builtin_aastore (java_objectarray *a, s4 index, java_objectheader *o)
{
	if (builtin_canstore(a,o)) {
		a->data[index] = o;
		return 1;
		}
	return 0;
}






/*****************************************************************************
                      METHODEN-PROTOKOLLIERUNG

	Verschiedene Funktionen, mit denen eine Meldung ausgegeben werden
	kann, wann immer Methoden aufgerufen oder beendet werden.
	(f"ur Debug-Zwecke)

*****************************************************************************/


u4 methodindent=0;

java_objectheader *builtin_trace_exception (java_objectheader *exceptionptr,
                   methodinfo *method, int *pos, int noindent) {

	if (!noindent)
		methodindent--;
	if (verbose || runverbose) {
		printf("Exception ");
		unicode_display (exceptionptr->vftbl->class->name);
		printf(" thrown in ");
		if (method) {
			unicode_display (method->class->name);
			printf(".");
			unicode_display (method->name);
			if (method->flags & ACC_SYNCHRONIZED)
				printf("(SYNC)");
			else
				printf("(NOSYNC)");
			printf("(%p) at position %p\n", method->entrypoint, pos);
			}
		else
			printf("call_java_method\n");
		fflush (stdout);
		}
	return exceptionptr;
}


void builtin_trace_args(long a0, long a1, long a2, long a3, long a4, long a5,
                        methodinfo *method)
{
	sprintf (logtext, "                                             ");
	sprintf (logtext+methodindent, "called: ");
	unicode_sprint (logtext+strlen(logtext), method->class->name);
	sprintf (logtext+strlen(logtext), ".");
	unicode_sprint (logtext+strlen(logtext), method->name);
	unicode_sprint (logtext+strlen(logtext), method->descriptor);
	sprintf (logtext+strlen(logtext), "(");
	switch (method->paramcount) {
		case 6:
			sprintf(logtext+strlen(logtext), "%lx, %lx, %lx, %lx, %lx, %lx",
	    	                                   a0,  a1,  a2,  a3,  a4,  a5);
			break;
		case 5:
			sprintf(logtext+strlen(logtext), "%lx, %lx, %lx, %lx, %lx",
	    	                                   a0,  a1,  a2,  a3,  a4);
			break;
		case 4:
			sprintf(logtext+strlen(logtext), "%lx, %lx, %lx, %lx",
	    	                                   a0,  a1,  a2,  a3);
			break;
		case 3:
			sprintf(logtext+strlen(logtext), "%lx, %lx, %lx", a0,  a1,  a2);
			break;
		case 2:
			sprintf(logtext+strlen(logtext), "%lx, %lx", a0,  a1);
			break;
		case 1:
			sprintf(logtext+strlen(logtext), "%lx", a0);
			break;
		}
	sprintf (logtext+strlen(logtext), ")");

	dolog ();
	methodindent++;
}

void builtin_displaymethodstart(methodinfo *method)
{
	sprintf (logtext, "                                             ");
	sprintf (logtext+methodindent, "called: ");
	unicode_sprint (logtext+strlen(logtext), method->class->name);
	sprintf (logtext+strlen(logtext), ".");
	unicode_sprint (logtext+strlen(logtext), method->name);
	unicode_sprint (logtext+strlen(logtext), method->descriptor);
	dolog ();
	methodindent++;
}

void builtin_displaymethodstop(methodinfo *method, long l, double d)
{
	methodindent--;
	sprintf (logtext, "                                             ");
	sprintf (logtext+methodindent, "finished: ");
	unicode_sprint (logtext+strlen(logtext), method->class->name);
	sprintf (logtext+strlen(logtext), ".");
	unicode_sprint (logtext+strlen(logtext), method->name);
	unicode_sprint (logtext+strlen(logtext), method->descriptor);
	switch (method->returntype) {
		case TYPE_INT:
		case TYPE_LONG:
			sprintf (logtext+strlen(logtext), "->%ld", l);
			break;
		case TYPE_FLOAT:
		case TYPE_DOUBLE:
			sprintf (logtext+strlen(logtext), "->%g", d);
			break;
		case TYPE_ADDRESS:
			sprintf (logtext+strlen(logtext), "->%p", (void*) l);
			break;
		}
	dolog ();
}

void builtin_displaymethodexception(methodinfo *method)
{
	sprintf (logtext, "                                             ");
	sprintf (logtext+methodindent, "exception abort: ");
	unicode_sprint (logtext+strlen(logtext), method->class->name);
	sprintf (logtext+strlen(logtext), ".");
	unicode_sprint (logtext+strlen(logtext), method->name);
	unicode_sprint (logtext+strlen(logtext), method->descriptor);
	dolog ();
}


/****************************************************************************
             SYNCHRONIZATION FUNCTIONS
*****************************************************************************/

/*
 * Lock the mutex of an object.
 */
#ifdef USE_THREADS
void
internal_lock_mutex_for_object (java_objectheader *object)
{
    mutexHashEntry *entry;
    int hashValue;

    assert(object != 0);

    hashValue = MUTEX_HASH_VALUE(object);
    entry = &mutexHashTable[hashValue];

    if (entry->object != 0)
    {
		if (entry->mutex.count == 0 && entry->conditionCount == 0)
		{
			entry->object = 0;
			entry->mutex.holder = 0;
			entry->mutex.count = 0;
			entry->mutex.muxWaiters = 0;
		}
	else
	{
	    while (entry->next != 0 && entry->object != object)
		entry = entry->next;

	    if (entry->object != object)
	    {
			entry->next = firstFreeOverflowEntry;
			firstFreeOverflowEntry = firstFreeOverflowEntry->next;

			entry = entry->next;
			entry->object = 0;
			entry->next = 0;
			assert(entry->conditionCount == 0);
	    }
	}
    }
    else
    {
		entry->mutex.holder = 0;
		entry->mutex.count = 0;
		entry->mutex.muxWaiters = 0;
    }

    if (entry->object == 0)
        entry->object = object;
    
    internal_lock_mutex(&entry->mutex);
}
#endif


/*
 * Unlocks the mutex of an object.
 */
#ifdef USE_THREADS
void
internal_unlock_mutex_for_object (java_objectheader *object)
{
    int hashValue;
    mutexHashEntry *entry;

    hashValue = MUTEX_HASH_VALUE(object);
    entry = &mutexHashTable[hashValue];

    if (entry->object == object)
		internal_unlock_mutex(&entry->mutex);
    else
    {
		while (entry->next != 0 && entry->next->object != object)
			entry = entry->next;

		assert(entry->next != 0);

		internal_unlock_mutex(&entry->next->mutex);

		if (entry->next->mutex.count == 0 && entry->conditionCount == 0)
		{
			mutexHashEntry *unlinked = entry->next;

			entry->next = unlinked->next;
			unlinked->next = firstFreeOverflowEntry;
			firstFreeOverflowEntry = unlinked;
		}
    }
}
#endif

void
builtin_monitorenter (java_objectheader *o)
{
#ifdef USE_THREADS
    int hashValue;

	assert(blockInts == 0);

    ++blockInts;

    hashValue = MUTEX_HASH_VALUE(o);
    if (mutexHashTable[hashValue].object == o
		&& mutexHashTable[hashValue].mutex.holder == currentThread)
		++mutexHashTable[hashValue].mutex.count;
    else
		internal_lock_mutex_for_object(o);

	--blockInts;

	assert(blockInts == 0);
#endif
}

void builtin_monitorexit (java_objectheader *o)
{
#ifdef USE_THREADS
    int hashValue;

	assert(blockInts == 0);

    ++blockInts;

    hashValue = MUTEX_HASH_VALUE(o);
    if (mutexHashTable[hashValue].object == o)
    {
		if (mutexHashTable[hashValue].mutex.count == 1
			&& mutexHashTable[hashValue].mutex.muxWaiters != 0)
			internal_unlock_mutex_for_object(o);
		else
			--mutexHashTable[hashValue].mutex.count;
    }
    else
		internal_unlock_mutex_for_object(o);

	--blockInts;

	assert(blockInts == 0);
#endif
}


/*****************************************************************************
                      DIVERSE HILFSFUNKTIONEN
*****************************************************************************/



/*********** Funktionen f"ur die Integerdivision *****************************
 
	Auf manchen Systemen (z.B. DEC ALPHA) wird durch die CPU keine Integer-
	division unterst"utzt.
	Daf"ur gibt es dann diese Hilfsfunktionen

******************************************************************************/

s4 builtin_idiv (s4 a, s4 b) { return a/b; }
s4 builtin_irem (s4 a, s4 b) { return a%b; }


/************** Funktionen f"ur Long-Arithmetik *******************************

	Auf Systemen, auf denen die CPU keine 64-Bit-Integers unterst"utzt,
	werden diese Funktionen gebraucht

******************************************************************************/


s8 builtin_ladd (s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	return a+b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lsub (s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	return a-b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lmul (s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	return a*b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_ldiv (s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	return a/b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lrem (s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	return a%b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lshl (s8 a, s4 b) 
{ 
#if U8_AVAILABLE
	return a<<(b&63);
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lshr (s8 a, s4 b) 
{ 
#if U8_AVAILABLE
	return a>>(b&63);
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lushr (s8 a, s4 b) 
{ 
#if U8_AVAILABLE
	return ((u8)a)>>(b&63);
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_land (s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	return a&b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lor (s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	return a|b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lxor (s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	return a^b; 
#else
	return builtin_i2l(0);
#endif
}

s8 builtin_lneg (s8 a) 
{ 
#if U8_AVAILABLE
	return -a;
#else
	return builtin_i2l(0);
#endif
}

s4 builtin_lcmp (s8 a, s8 b) 
{ 
#if U8_AVAILABLE
	if (a<b) return -1;
	if (a>b) return 1;
	return 0;
#else
	return 0;
#endif
}





/*********** Funktionen f"ur die Floating-Point-Operationen ******************/

float builtin_fadd (float a, float b)
{
	if (isnanf(a)) return FLT_NAN;
	if (isnanf(b)) return FLT_NAN;
	if (finitef(a)) {
		if (finitef(b)) return a+b;
		else return b;
		}
	else {
		if (finitef(b)) return a;
		else {
			if (copysignf(1.0, a)==copysignf(1.0, b)) return a;
			else  return FLT_NAN;
			}
		}
}

float builtin_fsub (float a, float b)
{
	return builtin_fadd (a, builtin_fneg(b));
}

float builtin_fmul (float a, float b)
{
	if (isnanf(a)) return FLT_NAN;
	if (isnanf(b)) return FLT_NAN;
	if (finitef(a)) {
		if (finitef(b)) return a*b;
		else {
			if (a==0) return FLT_NAN;
			     else return copysignf(b, copysignf(1.0, b)*a);
			}
		}
	else {
		if (finitef(b)) {
			if (b==0) return FLT_NAN;
			     else return copysignf(a, copysignf(1.0, a)*b);
			}
		else {
			return copysignf(a, copysignf(1.0, a)*copysignf(1.0, b));
			}
		}
}

float builtin_fdiv (float a, float b)
{
	if (finitef(a) && finitef(b)) {
		if (b != 0)
			return a / b;
		else {
			if (a > 0)
				return FLT_POSINF;
			else if (a < 0)
				return FLT_NEGINF;
			}
		}
	return FLT_NAN;
}

float builtin_frem (float a, float b)
{

/* return (float) builtin_drem((double) a, (double) b); */

	float f;

	if (finite((double) a) && finite((double) b)) {
		f = a / b;
		if (finite((double) f))
			return fmodf(a, b);
		return FLT_NAN;
		}
	if (isnan((double) b))
		return FLT_NAN;
	if (finite((double) a))
		return a;
	return FLT_NAN;

/*	float f;

	if (finitef(a) && finitef(b)) {
		f = a / b;
		if (finitef(f))
			return a - floorf(f) * b;
		return FLT_NAN;
		}
	if (isnanf(b))
		return FLT_NAN;
	if (finitef(a))
		return a;
	return FLT_NAN; */
}


float builtin_fneg (float a)
{
	if (isnanf(a)) return a;
	else {
		if (finitef(a)) return -a;
		           else return copysignf(a,-copysignf(1.0, a));
		}
}

s4 builtin_fcmpl (float a, float b)
{
	if (isnanf(a)) return -1;
	if (isnanf(b)) return -1;
	if (!finitef(a) || !finitef(b)) {
		a = finitef(a) ? 0 : copysignf(1.0,  a);
		b = finitef(b) ? 0 : copysignf(1.0, b);
		}
	if (a>b) return 1;
	if (a==b) return 0;
	return -1;
}

s4 builtin_fcmpg (float a, float b)
{
	if (isnanf(a)) return 1;
	if (isnanf(b)) return 1;
	if (!finitef(a) || !finitef(b)) {
		a = finitef(a) ? 0 : copysignf(1.0, a);
		b = finitef(b) ? 0 : copysignf(1.0, b);
		}
	if (a>b) return 1;
	if (a==b) return 0;
	return -1;
}



/*********** Funktionen f"ur doppelt genaue Fliesskommazahlen ***************/

double builtin_dadd (double a, double b)
{
	if (isnan(a)) return DBL_NAN;
	if (isnan(b)) return DBL_NAN;
	if (finite(a)) {
		if (finite(b)) return a+b;
		else return b;
		}
	else {
		if (finite(b)) return a;
		else {
			if (copysign(1.0, a)==copysign(1.0, b)) return a;
			else  return DBL_NAN;
			}
		}
}

double builtin_dsub (double a, double b)
{
	return builtin_dadd (a, builtin_dneg(b));
}

double builtin_dmul (double a, double b)
{
	if (isnan(a)) return DBL_NAN;
	if (isnan(b)) return DBL_NAN;
	if (finite(a)) {
		if (finite(b)) return a*b;
		else {
			if (a==0) return DBL_NAN;
			     else return copysign(b, copysign(1.0, b)*a);
			}
		}
	else {
		if (finite(b)) {
			if (b==0) return DBL_NAN;
			     else return copysign(a, copysign(1.0, a)*b);
			}
		else {
			return copysign(a, copysign(1.0, a)*copysign(1.0, b));
			}
		}
}

double builtin_ddiv (double a, double b)
{
	if (finite(a) && finite(b)) {
		if (b != 0)
			return a / b;
		else {
			if (a > 0)
				return DBL_POSINF;
			else if (a < 0)
				return DBL_NEGINF;
			}
		}
	return DBL_NAN;
}

double builtin_drem (double a, double b)
{
	double d;

	if (finite(a) && finite(b)) {
		d = a / b;
		if (finite(d)) {
			if ((d < 1.0) && (d > 0.0))
				return a;
			return fmod(a, b);
			}
		return DBL_NAN;
		}
	if (isnan(b))
		return DBL_NAN;
	if (finite(a))
		return a;
	return DBL_NAN;
}

double builtin_dneg (double a)
{
	if (isnan(a)) return a;
	else {
		if (finite(a)) return -a;
		          else return copysign(a,-copysign(1.0, a));
		}
}

s4 builtin_dcmpl (double a, double b)
{
	if (isnan(a)) return -1;
	if (isnan(b)) return -1;
	if (!finite(a) || !finite(b)) {
		a = finite(a) ? 0 : copysign(1.0, a);
		b = finite(b) ? 0 : copysign(1.0, b);
		}
	if (a>b) return 1;
	if (a==b) return 0;
	return -1;
}

s4 builtin_dcmpg (double a, double b)
{
	if (isnan(a)) return 1;
	if (isnan(b)) return 1;
	if (!finite(a) || !finite(b)) {
		a = finite(a) ? 0 : copysign(1.0, a);
		b = finite(b) ? 0 : copysign(1.0, b);
		}
	if (a>b) return 1;
	if (a==b) return 0;
	return -1;
}


/*********************** Umwandlungsoperationen ****************************/

s8 builtin_i2l (s4 i)
{
#if U8_AVAILABLE
	return i;
#else
	s8 v; v.high = 0; v.low=i; return v;
#endif
}

float builtin_i2f (s4 a)
{
float f = (float) a;
return f;
}

double builtin_i2d (s4 a)
{
double d = (double) a;
return d;
}


s4 builtin_l2i (s8 l)
{
#if U8_AVAILABLE
	return (s4) l;
#else
	return l.low;
#endif
}

float builtin_l2f (s8 a)
{
#if U8_AVAILABLE
	float f = (float) a;
	return f;
#else
	return 0.0;
#endif
}

double builtin_l2d (s8 a)
{
#if U8_AVAILABLE
	double d = (double) a;
	return d;
#else
	return 0.0;
#endif
}


s4 builtin_f2i(float a) 
{

return builtin_d2i((double) a);

/*	float f;
	
	if (isnanf(a))
		return 0;
	if (finitef(a)) {
		if (a > 2147483647)
			return 2147483647;
		if (a < (-2147483648))
			return (-2147483648);
		return (s4) a;
		}
	f = copysignf((float) 1.0, a);
	if (f > 0)
		return 2147483647;
	return (-2147483648); */
}


s8 builtin_f2l (float a)
{

return builtin_d2l((double) a);

/*	float f;
	
	if (finitef(a)) {
		if (a > 9223372036854775807L)
			return 9223372036854775807L;
		if (a < (-9223372036854775808L))
			return (-9223372036854775808L);
		return (s8) a;
		}
	if (isnanf(a))
		return 0;
	f = copysignf((float) 1.0, a);
	if (f > 0)
		return 9223372036854775807L;
	return (-9223372036854775808L); */
}


double builtin_f2d (float a)
{
	if (finitef(a)) return (double) a;
	else {
		if (isnanf(a)) return DBL_NAN;
		else           return copysign(DBL_POSINF, (double) copysignf(1.0, a) );
		}
}


s4 builtin_d2i (double a) 
{ 
	double d;
	
	if (finite(a)) {
		if (a >= 2147483647)
			return 2147483647;
		if (a <= (-2147483648))
			return (-2147483648);
		return (s4) a;
		}
	if (isnan(a))
		return 0;
	d = copysign(1.0, a);
	if (d > 0)
		return 2147483647;
	return (-2147483648);
}


s8 builtin_d2l (double a)
{
	double d;
	
	if (finite(a)) {
		if (a >= 9223372036854775807L)
			return 9223372036854775807L;
		if (a <= (-9223372036854775807L-1))
			return (-9223372036854775807L-1);
		return (s8) a;
		}
	if (isnan(a))
		return 0;
	d = copysign(1.0, a);
	if (d > 0)
		return 9223372036854775807L;
	return (-9223372036854775807L-1);
}


float builtin_d2f (double a)
{
	if (finite(a)) return (float) a;
	else {
		if (isnan(a)) return FLT_NAN;
		else          return copysignf (FLT_POSINF, (float) copysign(1.0, a));
		}
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
 */
