/* loader.c ********************************************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Contains the functions of the class loader.

	Author:  Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
	Changes: Mark Probst         EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/06/03

*******************************************************************************/


#include <assert.h>

#include "global.h"
#include "loader.h"

#include "tables.h"
#include "native.h"
#include "builtin.h"
#include "jit.h"
#ifdef OLD_COMPILER
#include "compiler.h"
#endif
#include "asmpart.h"

#include "threads/thread.h"                        /* schani */


/* global variables ***********************************************************/

extern bool newcompiler;        /* true if new compiler is used               */    		

int count_class_infos = 0;      /* variables for measurements                 */
int count_const_pool_len = 0;
int count_vftbl_len = 0;
int count_all_methods = 0;
int count_vmcode_len = 0;
int count_extable_len = 0;

bool loadverbose = false;    /* switches for debug messages                   */
bool linkverbose = false;
bool initverbose = false;

bool makeinitializations = true;

bool getloadingtime = false;
long int loadingtime = 0;


static s4 interfaceindex;    /* sequential numbering of interfaces            */ 

static list unloadedclasses; /* list of all referenced but not loaded classes */
static list unlinkedclasses; /* list of all loaded but not linked classes     */
       list linkedclasses;   /* list of all completely linked classes         */



/* important system classes ***************************************************/

classinfo *class_java_lang_Object;
classinfo *class_java_lang_String;
classinfo *class_java_lang_ClassCastException;
classinfo *class_java_lang_NullPointerException;
classinfo *class_java_lang_ArrayIndexOutOfBoundsException;
classinfo *class_java_lang_NegativeArraySizeException;
classinfo *class_java_lang_OutOfMemoryError;
classinfo *class_java_lang_ArithmeticException;
classinfo *class_java_lang_ArrayStoreException;
classinfo *class_java_lang_ThreadDeath;                 /* schani */

classinfo *class_array;


/* instances of important system classes **************************************/

java_objectheader *proto_java_lang_ClassCastException;
java_objectheader *proto_java_lang_NullPointerException;
java_objectheader *proto_java_lang_ArrayIndexOutOfBoundsException;
java_objectheader *proto_java_lang_NegativeArraySizeException;
java_objectheader *proto_java_lang_OutOfMemoryError;
java_objectheader *proto_java_lang_ArithmeticException;
java_objectheader *proto_java_lang_ArrayStoreException;
java_objectheader *proto_java_lang_ThreadDeath;         /* schani */




/******************************************************************************/
/******************* Einige Support-Funkionen *********************************/
/******************************************************************************/


/********** interne Funktion: printflags  (nur zu Debug-Zwecken) **************/

static void printflags (u2 f)
{
   if ( f & ACC_PUBLIC )       printf (" PUBLIC");
   if ( f & ACC_PRIVATE )      printf (" PRIVATE");
   if ( f & ACC_PROTECTED )    printf (" PROTECTED");
   if ( f & ACC_STATIC )       printf (" STATIC");
   if ( f & ACC_FINAL )        printf (" FINAL");
   if ( f & ACC_SYNCHRONIZED ) printf (" SYNCHRONIZED");
   if ( f & ACC_VOLATILE )     printf (" VOLATILE");
   if ( f & ACC_TRANSIENT )    printf (" TRANSIENT");
   if ( f & ACC_NATIVE )       printf (" NATIVE");
   if ( f & ACC_INTERFACE )    printf (" INTERFACE");
   if ( f & ACC_ABSTRACT )     printf (" ABSTRACT");
}


/************************* Funktion: skipattribute *****************************

	"uberliest im ClassFile eine (1) 'attribute'-Struktur 

*******************************************************************************/

static void skipattribute ()
{
	suck_u2 ();
	skip_nbytes(suck_u4());	
}

/********************** Funktion: skipattributebody ****************************

	"uberliest im Classfile ein attribut, wobei die 16-bit - attribute_name - 
	Referenz schon gelesen worden ist.
	
*******************************************************************************/

static void skipattributebody ()
{
	skip_nbytes(suck_u4());
}


/************************* Funktion: skipattributes ****************************

	"uberliest im ClassFile eine gew"unschte Anzahl von attribute-Strukturen
	
*******************************************************************************/

static void skipattributes (u4 num)
{
	u4 i;
	for (i = 0; i < num; i++)
		skipattribute();
}



/************************** Funktion: loadUtf8 *********************************

	liest aus dem ClassFile einen Utf8-String (=komprimierter unicode-text) 
	und legt daf"ur ein unicode-Symbol an. 
	Return: Zeiger auf das Symbol 

*******************************************************************************/

#define MAXUNICODELEN 5000
static u2 unicodebuffer[MAXUNICODELEN];

static unicode *loadUtf8 ()
{
	u4 unicodelen;
	u4 letter;

	u4 utflen;
	u4 b1,b2,b3;

	unicodelen = 0;

	utflen = suck_u2 ();
	while (utflen > 0) {
		b1 = suck_u1 ();
		utflen --;
		if (b1<0x80) letter = b1;
		else {
			b2 = suck_u1 ();
			utflen --;
			if (b1<0xe0) letter = ((b1 & 0x1f) << 6) | (b2 & 0x3f);
			else {
				b3 = suck_u1 ();
				utflen --;
				letter = ((b1 & 0x0f) << 12) | ((b2 & 0x3f) << 6) | (b3 & 0x3f);
				}
			}	
	

		if (unicodelen >= MAXUNICODELEN) {
			panic ("String constant too long");
			}
		
		unicodebuffer[unicodelen++] = letter;			
		}

	
	return unicode_new_u2 (unicodebuffer, unicodelen);
}



/******************** interne Funktion: checkfieldtype ************************/

static void checkfieldtype (u2 *text, u4 *count, u4 length)
{
	u4 l;

	if (*count >= length) panic ("Type-descriptor exceeds unicode length");
	
	l = text[(*count)++];
	
	switch (l) {
		default: panic ("Invalid symbol in type descriptor");
		         return;
		case 'B':
		case 'C':
		case 'D':
		case 'F':
		case 'I':
		case 'J':
		case 'S':
		case 'Z': return;
		
		case '[': checkfieldtype (text, count, length);
		          return;
		          
		case 'L': 
			{
			u4 tlen,tstart = *count;
			
			if (*count >= length) 
			   panic ("Objecttype descriptor of length zero");
			
			while ( text[*count] != ';' ) {
				(*count)++;
				if (*count >= length) 
				   panic ("Missing ';' in objecttype-descriptor");
				}
			
			tlen = (*count) - tstart;
			(*count)++;

			if (tlen == 0) panic ("Objecttype descriptor with empty name");
					
			class_get ( unicode_new_u2 (text+tstart, tlen) );
			}
		}	
}


/******************* Funktion: checkfielddescriptor ****************************

	"uberpr"uft, ob ein Field-Descriptor ein g"ultiges Format hat.
	Wenn nicht, dann wird das System angehalten.
	Au"serdem werden alle Klassen, die hier referenziert werden, 
	in die Liste zu ladender Klassen eingetragen.
	
*******************************************************************************/

void checkfielddescriptor (unicode *d)
{
	u4 count=0;
	checkfieldtype (d->text, &count, d->length);
	if (count != d->length) panic ("Invalid type-descritor encountered");
}


/******************* Funktion: checkmethoddescriptor ***************************

	"uberpr"uft, ob ein Method-Descriptor ein g"ultiges Format hat.
	Wenn nicht, dann wird das System angehalten.
	Au"serdem werden alle Klassen, die hier referenziert werden, 
	in die Liste zu ladender Klassen eingetragen.
	
*******************************************************************************/

void checkmethoddescriptor (unicode *d)
{
	u2 *text=d->text;
	u4 length=d->length;
	u4 count=0;
	
	if (length<2) panic ("Method descriptor too short");
	if (text[0] != '(') panic ("Missing '(' in method descriptor");
	count=1;
	
	while (text[count] != ')') {
		checkfieldtype (text,&count,length);
		if ( count > length-2 ) panic ("Unexpected end of descriptor");
		}
		
	count++;
	if (text[count] == 'V') count++;
	else                    checkfieldtype (text, &count,length);
		
	if (count != length) panic ("Method-descriptor has exceeding chars");
}


/******************** Funktion: buildarraydescriptor ***************************

	erzeugt zu einem namentlich als u2-String vorliegenden Arraytyp eine
	entsprechende constant_arraydescriptor - Struktur 
	
*******************************************************************************/

static constant_arraydescriptor * buildarraydescriptor(u2 *name, u4 namelen)
{
	constant_arraydescriptor *d;
	
	if (name[0]!='[') panic ("Attempt to build arraydescriptor for non-array");
	d = NEW (constant_arraydescriptor);
	d -> objectclass = NULL;
	d -> elementdescriptor = NULL;

#ifdef STATISTICS
	count_const_pool_len += sizeof(constant_arraydescriptor);
#endif

	switch (name[1]) {
	case 'Z': d -> arraytype = ARRAYTYPE_BOOLEAN; break;
	case 'B': d -> arraytype = ARRAYTYPE_BYTE; break;
	case 'C': d -> arraytype = ARRAYTYPE_CHAR; break;
	case 'D': d -> arraytype = ARRAYTYPE_DOUBLE; break;
	case 'F': d -> arraytype = ARRAYTYPE_FLOAT; break;
	case 'I': d -> arraytype = ARRAYTYPE_INT; break;
	case 'J': d -> arraytype = ARRAYTYPE_LONG; break;
	case 'S': d -> arraytype = ARRAYTYPE_SHORT; break;

	case '[':
		d -> arraytype = ARRAYTYPE_ARRAY; 
		d -> elementdescriptor = buildarraydescriptor (name+1, namelen-1);
		break;
		
	case 'L':
		d -> arraytype = ARRAYTYPE_OBJECT;
		d -> objectclass = class_get ( unicode_new_u2 (name+2, namelen-3) );
		break;
	}
	return d;
}


/******************* Funktion: freearraydescriptor *****************************

	entfernt eine mit buildarraydescriptor erzeugte Struktur wieder 
	aus dem Speicher
	
*******************************************************************************/

static void freearraydescriptor (constant_arraydescriptor *d)
{
	while (d) {
		constant_arraydescriptor *n = d->elementdescriptor;
		FREE (d, constant_arraydescriptor);
		d = n;
		}
}

/*********************** Funktion: displayarraydescriptor *********************/

static void displayarraydescriptor (constant_arraydescriptor *d)
{
	switch (d->arraytype) {
	case ARRAYTYPE_BOOLEAN: printf ("boolean[]"); break;
	case ARRAYTYPE_BYTE: printf ("byte[]"); break;
	case ARRAYTYPE_CHAR: printf ("char[]"); break;
	case ARRAYTYPE_DOUBLE: printf ("double[]"); break;
	case ARRAYTYPE_FLOAT: printf ("float[]"); break;
	case ARRAYTYPE_INT: printf ("int[]"); break;
	case ARRAYTYPE_LONG: printf ("long[]"); break;
	case ARRAYTYPE_SHORT: printf ("short[]"); break;
	case ARRAYTYPE_ARRAY: displayarraydescriptor(d->elementdescriptor); printf("[]"); break;
	case ARRAYTYPE_OBJECT: unicode_display(d->objectclass->name); printf("[]"); break;
	}
}



/******************************************************************************/
/******************** Funktionen fuer Fields **********************************/
/******************************************************************************/


/************************ Funktion: field_load *********************************

	l"adt alle Informationen f"ur eine Feld einer Methode aus dem ClassFile,
	und f"ullt mit diesen Infos eine schon existierende 'fieldinfo'-Struktur.
	Bei 'static'-Fields wird auch noch ein Platz auf dem Datensegment
	reserviert.

*******************************************************************************/

static void field_load (fieldinfo *f, classinfo *c)
{
	u4 attrnum,i;
	u4 jtype;

	f -> flags = suck_u2 ();
	f -> name = class_getconstant (c, suck_u2(), CONSTANT_Utf8);
	f -> descriptor = class_getconstant (c, suck_u2(), CONSTANT_Utf8);
	f -> type = jtype = desc_to_type (f->descriptor);
	f -> offset = 0;

	switch (f->type) {
	case TYPE_INT:        f->value.i = 0; break;
	case TYPE_FLOAT:      f->value.f = 0.0; break;
	case TYPE_DOUBLE:     f->value.d = 0.0; break;
	case TYPE_ADDRESS:    f->value.a = NULL; 
	                      heap_addreference (&(f->value.a));
	                      break;
	case TYPE_LONG:
#if U8_AVAILABLE
		f->value.l = 0; break;
#else
		f->value.l.low = 0; f->value.l.high = 0; break;
#endif 
	}
	
	attrnum = suck_u2();
	for (i=0; i<attrnum; i++) {
		u4 pindex;
		unicode *aname;

		aname = class_getconstant (c, suck_u2(), CONSTANT_Utf8);

		if ( aname != unicode_new_char ("ConstantValue") ) {
			skipattributebody ();
			}
		else {
			suck_u4();
			pindex = suck_u2();
				
			switch (jtype) {
				case TYPE_INT: {
					constant_integer *ci = 
					   	class_getconstant(c, pindex, CONSTANT_Integer);
					f->value.i = ci -> value;
					}
					break;
					
				case TYPE_LONG: {
					constant_long *cl = 
					   class_getconstant(c, pindex, CONSTANT_Long);
	
					f->value.l = cl -> value;
					}
					break;

				case TYPE_FLOAT: {
					constant_float *cf = 
					    class_getconstant(c, pindex, CONSTANT_Float);
	
					f->value.f = cf->value;
					}
					break;
											
				case TYPE_DOUBLE: {
					constant_double *cd = 
					    class_getconstant(c, pindex, CONSTANT_Double);
	
					f->value.d = cd->value;
					}
					break;
						
				case TYPE_ADDRESS: {
					unicode *u = 
						class_getconstant(c, pindex, CONSTANT_String);
					f->value.a = literalstring_new(u);
					}
					break;
	
				default: 
					log_text ("Invalid Constant - Type");

				}

			}
		}
		
}


/********************** Funktion: field_free **********************************/

static void field_free (fieldinfo *f)
{
}


/************** Funktion: field_display (nur zu Debug-Zwecken) ****************/

static void field_display (fieldinfo *f)
{
	printf ("   ");
	printflags (f -> flags);
	printf (" ");
	unicode_display (f -> name);
	printf (" ");
	unicode_display (f -> descriptor);	
	printf (" offset: %ld\n", (long int) (f -> offset) );
}




/******************************************************************************/
/************************* Funktionen f"ur Methods ****************************/ 
/******************************************************************************/


/*********************** Funktion: method_load *********************************

	l"adt die Infos f"ur eine Methode aus dem ClassFile und f"ullt damit 
	eine schon existierende 'methodinfo'-Struktur aus.
	Bei allen native-Methoden wird au"serdem gleich der richtige 
	Funktionszeiger eingetragen, bei JavaVM-Methoden einstweilen ein
	Zeiger auf den Compiler 
	
*******************************************************************************/

static void method_load (methodinfo *m, classinfo *c)
{
	u4 attrnum,i,e;
	
#ifdef STATISTICS
	count_all_methods++;
#endif

	m -> class = c;
	
	m -> flags = suck_u2 ();
	m -> name =  class_getconstant (c, suck_u2(), CONSTANT_Utf8);
	m -> descriptor = class_getconstant (c, suck_u2(), CONSTANT_Utf8);
	
	m -> jcode = NULL;
	m -> exceptiontable = NULL;
	m -> entrypoint = NULL;
	m -> mcode = NULL;
	m -> stubroutine = NULL;
	
	if (! (m->flags & ACC_NATIVE) ) {
		m -> stubroutine = createcompilerstub (m);
		}
	else {
		functionptr f = native_findfunction 
	 	       (c->name, m->name, m->descriptor, (m->flags & ACC_STATIC) != 0);
		if (f) {
#ifdef OLD_COMPILER
		if (newcompiler)
#endif
			m -> stubroutine = createnativestub (f, m);
#ifdef OLD_COMPILER
		else
			m -> stubroutine = oldcreatenativestub (f, m);
#endif
			}
		}
	
	
	attrnum = suck_u2();
	for (i=0; i<attrnum; i++) {
		unicode *aname;

		aname = class_getconstant (c, suck_u2(), CONSTANT_Utf8);

		if ( aname != unicode_new_char("Code"))  {
			skipattributebody ();
			}
		else {
			if (m -> jcode) panic ("Two code-attributes for one method!");
			
			suck_u4();
			m -> maxstack = suck_u2();
			m -> maxlocals = suck_u2();
			m -> jcodelength = suck_u4();
			m -> jcode = MNEW (u1, m->jcodelength);
			suck_nbytes (m->jcode, m->jcodelength);
			m -> exceptiontablelength = suck_u2 ();
			m -> exceptiontable = 
			   MNEW (exceptiontable, m->exceptiontablelength);

#ifdef STATISTICS
	count_vmcode_len += m->jcodelength + 18;
	count_extable_len += 8 * m->exceptiontablelength;
#endif

			for (e=0; e < m->exceptiontablelength; e++) {
				u4 idx;
				m -> exceptiontable[e].startpc = suck_u2();
				m -> exceptiontable[e].endpc = suck_u2();
				m -> exceptiontable[e].handlerpc = suck_u2();

				idx = suck_u2();
				if (!idx) m -> exceptiontable[e].catchtype = NULL;
				else {
					m -> exceptiontable[e].catchtype = 
				      class_getconstant (c, idx, CONSTANT_Class);
					}
				}			

			skipattributes ( suck_u2() );
			}
			
		}


}


/********************* Funktion: method_free ***********************************

	gibt allen Speicher, der extra f"ur eine Methode angefordert wurde,
	wieder frei

*******************************************************************************/

static void method_free (methodinfo *m)
{
	if (m->jcode) MFREE (m->jcode, u1, m->jcodelength);
	if (m->exceptiontable) 
		MFREE (m->exceptiontable, exceptiontable, m->exceptiontablelength);
	if (m->mcode) CFREE (m->mcode, m->mcodelength);
	if (m->stubroutine) {
		if (m->flags & ACC_NATIVE) removenativestub (m->stubroutine);
		else                       removecompilerstub (m->stubroutine);
		}
}


/************** Funktion: method_display  (nur zu Debug-Zwecken) **************/

void method_display (methodinfo *m)
{
	printf ("   ");
	printflags (m -> flags);
	printf (" ");
	unicode_display (m -> name);
	printf (" "); 
	unicode_display (m -> descriptor);
	printf ("\n");
}


/******************** Funktion: method_canoverwrite ****************************

	"uberpr"ft, ob eine Methode mit einer anderen typ- und namensidentisch 
	ist (also mit einer Methodendefinition eine andere "uberschrieben 
	werden kann).
	
*******************************************************************************/  

static bool method_canoverwrite (methodinfo *m, methodinfo *old)
{
	if (m->name != old->name) return false;
	if (m->descriptor != old->descriptor) return false;
	if (m->flags & ACC_STATIC) return false;
	return true;
}




/******************************************************************************/
/************************ Funktionen fuer Class *******************************/
/******************************************************************************/


/******************** Funktion: class_get **************************************

	Sucht im System die Klasse mit dem gew"unschten Namen, oder erzeugt
	eine neue 'classinfo'-Struktur (und h"angt sie in die Liste der zu
	ladenen Klassen ein).

*******************************************************************************/

classinfo *class_get (unicode *u)
{
	classinfo *c;
	
	if (u->class) return u->class;
	
#ifdef STATISTICS
	count_class_infos += sizeof(classinfo);
#endif

	c = NEW (classinfo);
	c -> flags = 0;
	c -> name = u;
	c -> cpcount = 0;
	c -> cptags = NULL;
	c -> cpinfos = NULL;
	c -> super = NULL;
	c -> sub = NULL;
	c -> nextsub = NULL;
	c -> interfacescount = 0;
	c -> interfaces = NULL;
	c -> fieldscount = 0;
	c -> fields = NULL;
	c -> methodscount = 0;
	c -> methods = NULL;
	c -> linked = false;
	c -> index = 0;
	c -> instancesize = 0;
	c -> header.vftbl = NULL;
	c -> vftbl = NULL;
	c -> initialized = false;
	
	unicode_setclasslink (u,c);
	list_addlast (&unloadedclasses, c);
		
	return c;
}



/******************** Funktion: class_getconstant ******************************

	holt aus dem ConstantPool einer Klasse den Wert an der Stelle 'pos'.
	Der Wert mu"s vom Typ 'ctype' sein, sonst wird das System angehalten.

*******************************************************************************/

voidptr class_getconstant (classinfo *c, u4 pos, u4 ctype) 
{
	if (pos >= c->cpcount) 
		panic ("Attempt to access constant outside range");
	if (c->cptags[pos] != ctype) {
		sprintf (logtext, "Type mismatch on constant: %d requested, %d here",
		 (int) ctype, (int) c->cptags[pos] );
		error();
		}
		
	return c->cpinfos[pos];
}


/********************* Funktion: class_constanttype ****************************

	Findet heraus, welchen Typ ein Eintrag in den ConstantPool einer 
	Klasse hat.
	
*******************************************************************************/

u4 class_constanttype (classinfo *c, u4 pos)
{
	if (pos >= c->cpcount) 
		panic ("Attempt to access constant outside range");
	return c->cptags[pos];
}


/******************** Funktion: class_loadcpool ********************************

	l"adt den gesammten ConstantPool einer Klasse.
	
	Dabei werden die einzelnen Eintr"age in ein wesentlich einfachers 
	Format gebracht (Klassenreferenzen werden aufgel"ost, ...)
	F"ur eine genaue "Ubersicht "uber das kompakte Format siehe: 'global.h' 

*******************************************************************************/

static void class_loadcpool (classinfo *c)
{

	typedef struct forward_class {      /* Diese Strukturen dienen dazu, */
		struct forward_class *next;     /* die Infos, die beim ersten */
		u2 thisindex;                   /* Durchgang durch den ConstantPool */
		u2 name_index;                  /* gelesen werden, aufzunehmen. */
	} forward_class;                    /* Erst nachdem der ganze Pool */ 
                                        /* gelesen wurde, k"onnen alle */
	typedef struct forward_string {	    /* Felder kompletiert werden */
		struct forward_string *next;    /* (und das auch nur in der richtigen */
		u2 thisindex;	                /* Reihenfolge) */
		u2 string_index;
	} forward_string;

	typedef struct forward_nameandtype {
		struct forward_nameandtype *next;
		u2 thisindex;
		u2 name_index;
		u2 sig_index;
	} forward_nameandtype;

	typedef struct forward_fieldmethint {	
		struct forward_fieldmethint *next;
		u2 thisindex;
		u1 tag;
		u2 class_index;
		u2 nameandtype_index;
	} forward_fieldmethint;



	u4 idx;
	long int dumpsize = dump_size ();

	forward_class *forward_classes = NULL;
	forward_string *forward_strings = NULL;
	forward_nameandtype *forward_nameandtypes = NULL;
	forward_fieldmethint *forward_fieldmethints = NULL;

	u4 cpcount       = c -> cpcount = suck_u2();
	u1 *cptags       = c -> cptags  = MNEW (u1, cpcount);
	voidptr *cpinfos = c -> cpinfos =  MNEW (voidptr, cpcount);

#ifdef STATISTICS
	count_const_pool_len += (sizeof(voidptr) + 1) * cpcount;
#endif

	
	for (idx=0; idx<cpcount; idx++) {
		cptags[idx] = CONSTANT_UNUSED;
		cpinfos[idx] = NULL;
		}

			
		/******* Erster Durchgang *******/
		/* Alle Eintr"age, die nicht unmittelbar aufgel"ost werden k"onnen, 
		   werden einmal `auf Vorrat' in tempor"are Strukturen eingelesen,
		   und dann am Ende nocheinmal durchgegangen */

	idx = 1;
	while (idx < cpcount) {
		u4 t = suck_u1 ();
		switch ( t ) {

			case CONSTANT_Class: { 
				forward_class *nfc = DNEW(forward_class);

				nfc -> next = forward_classes; 											
				forward_classes = nfc;

				nfc -> thisindex = idx;
				nfc -> name_index = suck_u2 ();

				idx++;
				break;
				}
			
			case CONSTANT_Fieldref:
			case CONSTANT_Methodref:
			case CONSTANT_InterfaceMethodref: {
				forward_fieldmethint *nff = DNEW (forward_fieldmethint);
				
				nff -> next = forward_fieldmethints;
				forward_fieldmethints = nff;

				nff -> thisindex = idx;
				nff -> tag = t;
				nff -> class_index = suck_u2 ();
				nff -> nameandtype_index = suck_u2 ();

				idx ++;						
				break;
				}
				
			case CONSTANT_String: {
				forward_string *nfs = DNEW (forward_string);
				
				nfs -> next = forward_strings;
				forward_strings = nfs;
				
				nfs -> thisindex = idx;
				nfs -> string_index = suck_u2 ();
				
				idx ++;
				break;
				}

			case CONSTANT_NameAndType: {
				forward_nameandtype *nfn = DNEW (forward_nameandtype);
				
				nfn -> next = forward_nameandtypes;
				forward_nameandtypes = nfn;
				
				nfn -> thisindex = idx;
				nfn -> name_index = suck_u2 ();
				nfn -> sig_index = suck_u2 ();
				
				idx ++;
				break;
				}

			case CONSTANT_Integer: {
				constant_integer *ci = NEW (constant_integer);

#ifdef STATISTICS
	count_const_pool_len += sizeof(constant_integer);
#endif

				ci -> value = suck_s4 ();
				cptags [idx] = CONSTANT_Integer;
				cpinfos [idx] = ci;
				idx ++;
				
				break;
				}
				
			case CONSTANT_Float: {
				constant_float *cf = NEW (constant_float);

#ifdef STATISTICS
	count_const_pool_len += sizeof(constant_float);
#endif

				cf -> value = suck_float ();
				cptags [idx] = CONSTANT_Float;
				cpinfos[idx] = cf;
				idx ++;
				break;
				}
				
			case CONSTANT_Long: {
				constant_long *cl = NEW(constant_long);
					
#ifdef STATISTICS
	count_const_pool_len += sizeof(constant_long);
#endif

				cl -> value = suck_s8 ();
				cptags [idx] = CONSTANT_Long;
				cpinfos [idx] = cl;
				idx += 2;
				break;
				}
			
			case CONSTANT_Double: {
				constant_double *cd = NEW(constant_double);
				
#ifdef STATISTICS
	count_const_pool_len += sizeof(constant_double);
#endif

				cd -> value = suck_double ();
				cptags [idx] = CONSTANT_Double;
				cpinfos [idx] = cd;
				idx += 2;
				break;
				}
				
			case CONSTANT_Utf8: {
				unicode *u;

				u = loadUtf8 ();

				cptags [idx] = CONSTANT_Utf8;
				cpinfos [idx] = u;
				idx++;
				break;
				}
										
			default:
				sprintf (logtext, "Unkown constant type: %d",(int) t);
				error ();
		
			}  /* end switch */
			
		} /* end while */
		


	   /* Aufl"osen der noch unfertigen Eintr"age */

	while (forward_classes) {
		unicode *name =
		  class_getconstant (c, forward_classes -> name_index, CONSTANT_Utf8);
		
		if ( (name->length>0) && (name->text[0]=='[') ) {
			checkfielddescriptor (name); 

			cptags  [forward_classes -> thisindex] = CONSTANT_Arraydescriptor;
			cpinfos [forward_classes -> thisindex] = 
			   buildarraydescriptor(name->text, name->length);

			}
		else {		
			cptags  [forward_classes -> thisindex] = CONSTANT_Class;
			cpinfos [forward_classes -> thisindex] = class_get (name);
			}
		forward_classes = forward_classes -> next;
		
		}

	while (forward_strings) {
		unicode *text = 
		  class_getconstant (c, forward_strings -> string_index, CONSTANT_Utf8);
			
		cptags   [forward_strings -> thisindex] = CONSTANT_String;
		cpinfos  [forward_strings -> thisindex] = text;
		
		forward_strings = forward_strings -> next;
		}	

	while (forward_nameandtypes) {
		constant_nameandtype *cn = NEW (constant_nameandtype);	

#ifdef STATISTICS
		count_const_pool_len += sizeof(constant_nameandtype);
#endif

		cn -> name = class_getconstant 
		   (c, forward_nameandtypes -> name_index, CONSTANT_Utf8);
		cn -> descriptor = class_getconstant
		   (c, forward_nameandtypes -> sig_index, CONSTANT_Utf8);
		 
		cptags   [forward_nameandtypes -> thisindex] = CONSTANT_NameAndType;
		cpinfos  [forward_nameandtypes -> thisindex] = cn;
		
		forward_nameandtypes = forward_nameandtypes -> next;
		}


	while (forward_fieldmethints)  {
		constant_nameandtype *nat;
		constant_FMIref *fmi = NEW (constant_FMIref);

#ifdef STATISTICS
		count_const_pool_len += sizeof(constant_FMIref);
#endif

		nat = class_getconstant
			(c, forward_fieldmethints -> nameandtype_index, CONSTANT_NameAndType);

		fmi -> class = class_getconstant 
			(c, forward_fieldmethints -> class_index, CONSTANT_Class);
		fmi -> name = nat -> name;
		fmi -> descriptor = nat -> descriptor;

		cptags [forward_fieldmethints -> thisindex] = forward_fieldmethints -> tag;
		cpinfos [forward_fieldmethints -> thisindex] = fmi;
	
		switch (forward_fieldmethints -> tag) {
		case CONSTANT_Fieldref:  checkfielddescriptor (fmi->descriptor);
		                         break;
		case CONSTANT_InterfaceMethodref: 
		case CONSTANT_Methodref: checkmethoddescriptor (fmi->descriptor);
		                         break;
		}		
	
		forward_fieldmethints = forward_fieldmethints -> next;

		}


	dump_release (dumpsize);
}


/********************** Funktion: class_load ***********************************

	l"adt alle Infos f"ur eine ganze Klasse aus einem ClassFile. Die
	'classinfo'-Struktur mu"s bereits angelegt worden sein.
	
	Die Superklasse und die Interfaces, die diese Klasse implementiert,
	m"ussen zu diesem Zeitpunkt noch nicht geladen sein, die 
	Verbindung dazu wird sp"ater in der Funktion 'class_link' hergestellt.
	
	Die gelesene Klasse wird dann aus der Liste 'unloadedclasses' ausgetragen
	und in die Liste 'unlinkedclasses' eingh"angt.
	
*******************************************************************************/

static void class_load (classinfo *c)
{
	u4 i;
	u4 mi,ma;


	if (loadverbose) {
		sprintf (logtext, "Loading class: ");
		unicode_sprint (logtext+strlen(logtext), c->name );
		dolog();
		}


	suck_start (c->name);

	if (suck_u4() != MAGIC) panic("Can not find class-file signature");   
	mi = suck_u2(); 
	ma = suck_u2();
	if (ma != MAJOR_VERSION) {
		sprintf (logtext, "Can only support major version %d, but not %d",
		                 MAJOR_VERSION, (int) ma);
		error();
		}
	if (mi > MINOR_VERSION)  {
		sprintf (logtext, "Minor version %d is not yet supported.", (int) mi);
		error();
  		}


	class_loadcpool (c);
	
	c -> flags = suck_u2 ();
	suck_u2 ();       /* this */
	
	if ( (i = suck_u2 () ) ) {
		c -> super = class_getconstant (c, i, CONSTANT_Class);
		}
	else {
		c -> super = NULL;
		}
			 
	c -> interfacescount = suck_u2 ();
	c -> interfaces = MNEW (classinfo*, c -> interfacescount);
	for (i=0; i < c -> interfacescount; i++) {
		c -> interfaces [i] = 
		      class_getconstant (c, suck_u2(), CONSTANT_Class);
		}

	c -> fieldscount = suck_u2 ();
	c -> fields = MNEW (fieldinfo, c -> fieldscount);
	for (i=0; i < c -> fieldscount; i++) {
		field_load (&(c->fields[i]), c);
		}

	c -> methodscount = suck_u2 ();
	c -> methods = MNEW (methodinfo, c -> methodscount);
	for (i=0; i < c -> methodscount; i++) {
		method_load (&(c -> methods [i]), c);
		}

#ifdef STATISTICS
	count_class_infos += sizeof(classinfo*) * c -> interfacescount;
	count_class_infos += sizeof(fieldinfo) * c -> fieldscount;
	count_class_infos += sizeof(methodinfo) * c -> methodscount;
#endif


	skipattributes ( suck_u2() );


	suck_stop ();

	list_remove (&unloadedclasses, c);
	list_addlast (&unlinkedclasses, c);
}



/************** interne Funktion: class_highestinterface ***********************

	wird von der Funktion class_link ben"otigt, um festzustellen, wie gro"s
	die Interfacetable einer Klasse sein mu"s.

*******************************************************************************/

static s4 class_highestinterface (classinfo *c) 
{
	s4 h;
	s4 i;
	
	if ( ! (c->flags & ACC_INTERFACE) ) {
	  	sprintf (logtext, "Interface-methods count requested for non-interface:  ");
    	unicode_sprint (logtext+strlen(logtext), c->name);
    	error();
    	}
    
    h = c->index;
	for (i=0; i<c->interfacescount; i++) {
		s4 h2 = class_highestinterface (c->interfaces[i]);
		if (h2>h) h=h2;
		}
	return h;
}


/* class_addinterface **********************************************************

	wird von der Funktion class_link ben"otigt, um eine Virtual Function 
	Table f"ur ein Interface (und alle weiteren von diesem Interface
	implementierten Interfaces) in eine Klasse einzutragen.

*******************************************************************************/	

static void class_addinterface (classinfo *c, classinfo *ic)
{
	s4     j, m;
	s4     i     = ic->index;
	vftbl *vftbl = c->vftbl;
	
	if (i >= vftbl->interfacetablelength)
		panic ("Inernal error: interfacetable overflow");
	if (vftbl->interfacetable[-i])
		return;

	if (ic->methodscount == 0) {  /* fake entry needed for subtype test */
		vftbl->interfacevftbllength[i] = 1;
		vftbl->interfacetable[-i] = MNEW(methodptr, 1);
		vftbl->interfacetable[-i][0] = NULL;
		}
	else {
		vftbl->interfacevftbllength[i] = ic->methodscount;
		vftbl->interfacetable[-i] = MNEW(methodptr, ic->methodscount); 

#ifdef STATISTICS
	count_vftbl_len += sizeof(methodptr) *
	                         (ic->methodscount + (ic->methodscount == 0));
#endif

		for (j=0; j<ic->methodscount; j++) {
			classinfo *sc = c;
			while (sc) {
				for (m = 0; m < sc->methodscount; m++) {
					methodinfo *mi = &(sc->methods[m]);
					if (method_canoverwrite(mi, &(ic->methods[j]))) {
						vftbl->interfacetable[-i][j] = 
						                          vftbl->table[mi->vftblindex];
						goto foundmethod;
						}
					}
				sc = sc->super;
				}
			 foundmethod: ;
			}
		}

	for (j = 0; j < ic->interfacescount; j++) 
		class_addinterface(c, ic->interfaces[j]);
}


/********************** Funktion: class_link ***********************************

	versucht, eine Klasse in das System voll zu integrieren (linken). Dazu 
	m"ussen sowol die Superklasse, als auch alle implementierten
	Interfaces schon gelinkt sein.
	Diese Funktion berechnet sowohl die L"ange (in Bytes) einer Instanz 
	dieser Klasse, als auch die Virtual Function Tables f"ur normale
	Methoden als auch Interface-Methoden.
	
	Wenn die Klasse erfolgreich gelinkt werden kann, dann wird sie aus
	der Liste 'unlinkedclasses' ausgeh"angt, und in die Klasse 'linkedclasses'
	eingetragen.
	Wenn nicht, dann wird sie ans Ende der Liste 'unlinkedclasses' gestellt.

	Achtung: Bei zyklischen Klassendefinitionen ger"at das Programm hier in
	         eine Endlosschleife!!  (Da muss ich mir noch was einfallen lassen)

*******************************************************************************/

static void class_link (classinfo *c)
{
	s4 supervftbllength;          /* vftbllegnth of super class               */
	s4 vftbllength;               /* vftbllength of current class             */
	s4 interfacetablelength;      /* interface table length                   */
	classinfo *super = c->super;  /* super class                              */
	classinfo *ic, *c2;           /* intermediate class variables             */
	vftbl *v;                     /* vftbl of current class                   */
	s4 i;                         /* interface/method/field counter           */                     


	/*  check if all superclasses are already linked, if not put c at end of
	    unlinked list and return. Additionally initialize class fields.       */

	/*  check interfaces */

	for (i = 0; i < c->interfacescount; i++) {
		ic = c->interfaces[i];
		if (!ic->linked) {
			list_remove(&unlinkedclasses, c);
			list_addlast(&unlinkedclasses, c);
			return;	
			}
		}
	
	/*  check super class */

	if (super == NULL) {          /* class java.long.Object */
		c->index = 0;
		c->instancesize = sizeof(java_objectheader);
		
		vftbllength = supervftbllength = 0;

		c->finalizer = NULL;
		}
	else {
		if (!super->linked) {
			list_remove(&unlinkedclasses, c);
			list_addlast(&unlinkedclasses, c);
			return;	
			}

		if (c->flags & ACC_INTERFACE)
			c->index = interfaceindex++;
		else
			c->index = super->index + 1;
		
		c->instancesize = super->instancesize;
		
		vftbllength = supervftbllength = super->vftbl->vftbllength;
		
		c->finalizer = super->finalizer;
		}


	if (linkverbose) {
		sprintf (logtext, "Linking Class: ");
		unicode_sprint (logtext+strlen(logtext), c->name );
		dolog ();
		}

	/* compute vftbl length */

	for (i = 0; i < c->methodscount; i++) {
		methodinfo *m = &(c->methods[i]);
			
		if (!(m->flags & ACC_STATIC)) { /* is instance method */
			classinfo *sc = super;
			while (sc) {
				int j;
				for (j = 0; j < sc->methodscount; j++) {
					if (method_canoverwrite(m, &(sc->methods[j]))) {
						m->vftblindex = sc->methods[j].vftblindex;
						goto foundvftblindex;
						}
					}
				sc = sc->super;
				}
			m->vftblindex = (vftbllength++);
foundvftblindex: ;
			}
		}	
	
#ifdef STATISTICS
	count_vftbl_len += sizeof(vftbl) + sizeof(methodptr)*(vftbllength-1);
#endif

	/* compute interfacetable length */

	interfacetablelength = 0;
	c2 = c;
	while (c2) {
		for (i = 0; i < c2->interfacescount; i++) {
			s4 h = class_highestinterface (c2->interfaces[i]) + 1;
			if (h > interfacetablelength)
				interfacetablelength = h;
			}
		c2 = c2->super;
		}

	/* allocate virtual function table */

	v = (vftbl*) mem_alloc(sizeof(vftbl) + sizeof(methodptr) *
	            (vftbllength - 1) + sizeof(methodptr*) *
	            (interfacetablelength - (interfacetablelength > 0)));
	v = (vftbl*) (((methodptr*) v) + (interfacetablelength - 1) *
	                                 (interfacetablelength > 1));
	c->header.vftbl = c->vftbl = v;
	v->class = c;
	v->vftbllength = vftbllength;
	v->interfacetablelength = interfacetablelength;

	/* copy virtual function table of super class */

	for (i = 0; i < supervftbllength; i++) 
	   v->table[i] = super->vftbl->table[i];
	
	/* add method stubs into virtual function table */

	for (i = 0; i < c->methodscount; i++) {
		methodinfo *m = &(c->methods[i]);
		if (!(m->flags & ACC_STATIC)) {
			v->table[m->vftblindex] = m->stubroutine;
			}
		}

	/* compute instance size and offset of each field */
	
	for (i = 0; i < c->fieldscount; i++) {
		s4 dsize;
		fieldinfo *f = &(c->fields[i]);
		
		if (!(f->flags & ACC_STATIC) ) {
			dsize = desc_typesize (f->descriptor);
			c->instancesize = ALIGN (c->instancesize, dsize);
			f->offset = c->instancesize;
			c->instancesize += dsize;
			}
		}

	/* initialize interfacetable and interfacevftbllength */
	
	v->interfacevftbllength = MNEW (s4, interfacetablelength);

#ifdef STATISTICS
	count_vftbl_len += (4 + sizeof(s4)) * v->interfacetablelength;
#endif

	for (i = 0; i < interfacetablelength; i++) {
		v->interfacevftbllength[i] = 0;
		v->interfacetable[-i] = NULL;
		}
	
	/* add interfaces */
	
	for (c2 = c; c2 != NULL; c2 = c2->super)
		for (i = 0; i < c2->interfacescount; i++) {
			class_addinterface (c, c2->interfaces[i]);
			}

	/* add finalizer method (not for java.lang.Object) */

	if (super != NULL) {
		methodinfo *fi;
		static unicode *finame = NULL;
		static unicode *fidesc = NULL;

		if (finame == NULL)
			finame = unicode_new_char("finalize");
		if (fidesc == NULL)
			fidesc = unicode_new_char("()V");

		fi = class_findmethod (c, finame, fidesc);
		if (fi != NULL) {
			if (!(fi->flags & ACC_STATIC)) {
				c->finalizer = fi;
				}
			}
		}

	/* final tasks */

	c->linked = true;	

	list_remove (&unlinkedclasses, c);
	list_addlast (&linkedclasses, c);
}


/******************* Funktion: class_freepool **********************************

	Gibt alle Resourcen, die der ConstantPool einer Klasse ben"otigt,
	wieder frei.

*******************************************************************************/

static void class_freecpool (classinfo *c)
{
	u4 idx;
	u4 tag;
	voidptr info;
	
	for (idx=0; idx < c->cpcount; idx++) {
		tag = c->cptags[idx];
		info = c->cpinfos[idx];
		
		if (info != NULL) {
			switch (tag) {
			case CONSTANT_Fieldref:
			case CONSTANT_Methodref:
			case CONSTANT_InterfaceMethodref:
				FREE (info, constant_FMIref);
				break;
			case CONSTANT_Integer:
				FREE (info, constant_integer);
				break;
			case CONSTANT_Float:
				FREE (info, constant_float);
				break;
			case CONSTANT_Long:
				FREE (info, constant_long);
				break;
			case CONSTANT_Double:
				FREE (info, constant_double);
				break;
			case CONSTANT_NameAndType:
				FREE (info, constant_nameandtype);
				break;
			case CONSTANT_Arraydescriptor:
				freearraydescriptor (info);
				break;
			}
			}
		}

	MFREE (c -> cptags,  u1, c -> cpcount);
	MFREE (c -> cpinfos, voidptr, c -> cpcount);
}


/*********************** Funktion: class_free **********************************

	Gibt alle Resourcen, die eine ganze Klasse ben"otigt, frei

*******************************************************************************/

static void class_free (classinfo *c)
{
	s4 i;
	vftbl *v;
		
	unicode_unlinkclass (c->name);
	
	class_freecpool (c);

	MFREE (c->interfaces, classinfo*, c->interfacescount);

	for (i = 0; i < c->fieldscount; i++)
		field_free(&(c->fields[i]));
	MFREE (c->fields, fieldinfo, c->fieldscount);
	
	for (i = 0; i < c->methodscount; i++)
		method_free(&(c->methods[i]));
	MFREE (c->methods, methodinfo, c->methodscount);

	if ((v = c->vftbl) != NULL) {
		for (i = 0; i < v->interfacetablelength; i++) {
			MFREE (v->interfacetable[-i], methodptr, v->interfacevftbllength[i]);
			}
		MFREE (v->interfacevftbllength, s4, v->interfacetablelength);

		i = sizeof(vftbl) + sizeof(methodptr) * (v->vftbllength - 1) +
		    sizeof(methodptr*) * (v->interfacetablelength -
		                         (v->interfacetablelength > 0));
		v = (vftbl*) (((methodptr*) v) - (v->interfacetablelength - 1) *
	                                     (v->interfacetablelength > 1));
		mem_free (v, i);
		}
		
	FREE (c, classinfo);
}


/************************* Funktion: class_findfield ***************************
	
	sucht in einer 'classinfo'-Struktur nach einem Feld mit gew"unschtem
	Namen und Typ.

*******************************************************************************/

fieldinfo *class_findfield (classinfo *c, unicode *name, unicode *desc)
{
	s4 i;
	for (i = 0; i < c->fieldscount; i++) {
		if ((c->fields[i].name == name) && (c->fields[i].descriptor == desc))
			return &(c->fields[i]);
		}
	panic ("Can not find field given in CONSTANT_Fieldref");
	return NULL;
}


/************************* Funktion: class_findmethod **************************
	
	sucht in einer 'classinfo'-Struktur nach einer Methode mit gew"unschtem
	Namen und Typ.
	Wenn als Typ NULL angegeben wird, dann ist der Typ egal.

*******************************************************************************/

methodinfo *class_findmethod (classinfo *c, unicode *name, unicode *desc)
{
	s4 i;
	for (i = 0; i < c->methodscount; i++) {
		if ((c->methods[i].name == name) && ((desc == NULL) ||
		                                   (c->methods[i].descriptor == desc)))
			return &(c->methods[i]);
		}
	return NULL;
}


/************************* Funktion: class_resolvemethod ***********************
	
	sucht eine Klasse und alle Superklassen ab, um eine Methode zu finden.

*******************************************************************************/


methodinfo *class_resolvemethod (classinfo *c, unicode *name, unicode *desc)
{
	while (c) {
		methodinfo *m = class_findmethod (c, name, desc);
		if (m) return m;
		c = c->super;
		}
	return NULL;
}
	


/************************* Funktion: class_issubclass **************************

	"uberpr"uft, ob eine Klasse von einer anderen Klasse abgeleitet ist.		
	
*******************************************************************************/

bool class_issubclass (classinfo *sub, classinfo *super)
{
	for (;;) {
		if (!sub) return false;
		if (sub==super) return true;
		sub = sub -> super;
		}
}



/****************** Initialisierungsfunktion f"ur eine Klasse ******************

	In Java kann jede Klasse ein statische Initialisierungsfunktion haben.
	Diese Funktion mu"s aufgerufen werden, BEVOR irgendwelche Methoden der
	Klasse aufgerufen werden, oder auf statische Variablen zugegriffen
	wird.

*******************************************************************************/

#ifdef USE_THREADS
extern int blockInts;
#endif

void class_init (classinfo *c)
{
 	methodinfo *m;
 	java_objectheader *exceptionptr;
	s4 i;
	int b;

	if (!makeinitializations) return;
 	if (c->initialized) return;
  	c -> initialized = true;
 	
  	if (c->super) class_init (c->super);
	for (i=0; i < c->interfacescount; i++) class_init(c->interfaces[i]);

	m = class_findmethod (c, 
	                      unicode_new_char ("<clinit>"), 
	                      unicode_new_char ("()V"));
	if (!m) {
		if (initverbose) {
			sprintf (logtext, "Class ");
			unicode_sprint (logtext+strlen(logtext), c->name);
			sprintf (logtext+strlen(logtext), " has no initializer");	
			dolog ();
			}
		return;
		}
		
	if (! (m->flags & ACC_STATIC)) panic ("Class initializer is not static!");
	
	if (initverbose) {
		sprintf (logtext, "Starting initializer for class: ");
		unicode_sprint (logtext+strlen(logtext), c->name);
		dolog ();
	}

#ifdef USE_THREADS
	b = blockInts;
	blockInts = 0;
#endif

	exceptionptr = asm_calljavamethod (m, NULL,NULL,NULL,NULL);

#ifdef USE_THREADS
	assert(blockInts == 0);
	blockInts = b;
#endif

	if (exceptionptr) {	
		printf ("#### Initializer has thrown: ");
		unicode_display (exceptionptr->vftbl->class->name);
		printf ("\n");
		fflush (stdout);
		}

	if (initverbose) {
		sprintf (logtext, "Finished initializer for class: ");
		unicode_sprint (logtext+strlen(logtext), c->name);
		dolog ();
	}

}




/********* Funktion: class_showconstantpool   (nur f"ur Debug-Zwecke) *********/

void class_showconstantpool (classinfo *c) 
{
	u4 i;
	voidptr e;

	printf ("---- dump of constant pool ----\n");

	for (i=0; i<c->cpcount; i++) {
		printf ("#%d:  ", (int) i);
		
		e = c -> cpinfos [i];
		if (e) {
			
			switch (c -> cptags [i]) {
				case CONSTANT_Class:
					printf ("Classreference -> ");
					unicode_display ( ((classinfo*)e) -> name );
					break;
				
				case CONSTANT_Fieldref:
					printf ("Fieldref -> "); goto displayFMI;
				case CONSTANT_Methodref:
					printf ("Methodref -> "); goto displayFMI;
				case CONSTANT_InterfaceMethodref:
					printf ("InterfaceMethod -> "); goto displayFMI;
				  displayFMI:
					{
					constant_FMIref *fmi = e;
					unicode_display ( fmi->class->name );
					printf (".");
					unicode_display ( fmi->name);
					printf (" ");
					unicode_display ( fmi->descriptor );
				    }
					break;

				case CONSTANT_String:
					printf ("String -> ");
					unicode_display (e);
					break;
				case CONSTANT_Integer:
					printf ("Integer -> %d", (int) ( ((constant_integer*)e) -> value) );
					break;
				case CONSTANT_Float:
					printf ("Float -> %f", ((constant_float*)e) -> value);
					break;
				case CONSTANT_Double:
					printf ("Double -> %f", ((constant_double*)e) -> value);
					break;
				case CONSTANT_Long:
					{
					u8 v = ((constant_long*)e) -> value;
#if U8_AVAILABLE
					printf ("Long -> %ld", (long int) v);
#else
					printf ("Long -> HI: %ld, LO: %ld\n", 
					    (long int) v.high, (long int) v.low);
#endif 
					}
					break;
				case CONSTANT_NameAndType:
					{ constant_nameandtype *cnt = e;
					  printf ("NameAndType: ");
					  unicode_display (cnt->name);
					  printf (" ");
					  unicode_display (cnt->descriptor);
					}
					break;
				case CONSTANT_Utf8:
					printf ("Utf8 -> ");
					unicode_display (e);
					break;
				case CONSTANT_Arraydescriptor:	{
					printf ("Arraydescriptor: ");
					displayarraydescriptor (e);
					}
					break;
				default: 
					panic ("Invalid type of ConstantPool-Entry");
				}
				
			}

		printf ("\n");
		}
	
}



/********** Funktion: class_showmethods   (nur f"ur Debug-Zwecke) *************/

void class_showmethods (classinfo *c)
{
	s4 i;
	
	printf ("--------- Fields and Methods ----------------\n");
	printf ("Flags: ");	printflags (c->flags);	printf ("\n");

	printf ("This: "); unicode_display (c->name); printf ("\n");
	if (c->super) {
		printf ("Super: "); unicode_display (c->super->name); printf ("\n");
		}
	printf ("Index: %d\n", c->index);
	
	printf ("interfaces:\n");	
	for (i=0; i < c-> interfacescount; i++) {
		printf ("   ");
		unicode_display (c -> interfaces[i] -> name);
		printf (" (%d)\n", c->interfaces[i] -> index);
		}

	printf ("fields:\n");		
	for (i=0; i < c -> fieldscount; i++) {
		field_display (&(c -> fields[i]));
		}

	printf ("methods:\n");
	for (i=0; i < c -> methodscount; i++) {
		methodinfo *m = &(c->methods[i]);
		if ( !(m->flags & ACC_STATIC)) 
			printf ("vftblindex: %d   ", m->vftblindex);

		method_display ( m );

		}

	printf ("Virtual function table:\n");
	for (i=0; i<c->vftbl->vftbllength; i++) {
		printf ("entry: %d,  %ld\n", i, (long int) (c->vftbl->table[i]) );
		}

}



/******************************************************************************/
/******************* Funktionen fuer den Class-loader generell ****************/
/******************************************************************************/


/********************* Funktion: loader_load ***********************************

	l"adt und linkt die ge"unschte Klasse und alle davon 
	referenzierten Klassen und Interfaces
	Return: Einen Zeiger auf diese Klasse

*******************************************************************************/

classinfo *loader_load (unicode *topname)
{
	classinfo *top;
	classinfo *c;
	long int starttime=0,stoptime=0;
	
	intsDisable();                           /* schani */

	if (getloadingtime) starttime = getcputime();

	top = class_get (topname);

	while ( (c = list_first(&unloadedclasses)) ) {
		class_load (c);
		}

	while ( (c = list_first(&unlinkedclasses)) ) {
		class_link (c);
		}

	if (getloadingtime) {
		stoptime = getcputime();
		loadingtime += (stoptime-starttime);
		}

	intsRestore();                          /* schani */

	return top;
}


/******************* interne Funktion: loader_createarrayclass *****************

	Erzeugt (und linkt) eine Klasse f"ur die Arrays.
	
*******************************************************************************/

static classinfo *loader_createarrayclass ()
{
	classinfo *c;
	c = class_get ( unicode_new_char ("The_Array_Class") );
	
	list_remove (&unloadedclasses, c);
	list_addlast (&unlinkedclasses, c);
	c -> super = class_java_lang_Object;
	
	class_link (c);
	return c;
}



/********************** Funktion: loader_init **********************************

	Initialisiert alle Listen und l"adt alle Klassen, die vom System
	und vom Compiler direkt ben"otigt werden.

*******************************************************************************/
 
void loader_init ()
{
	interfaceindex = 0;
	
	list_init (&unloadedclasses, OFFSET(classinfo, listnode) );
	list_init (&unlinkedclasses, OFFSET(classinfo, listnode) );
	list_init (&linkedclasses, OFFSET(classinfo, listnode) );


	class_java_lang_Object = 
		loader_load ( unicode_new_char ("java/lang/Object") );
	class_java_lang_String = 
		loader_load ( unicode_new_char ("java/lang/String") );
	class_java_lang_ClassCastException = 
		loader_load ( unicode_new_char ("java/lang/ClassCastException") );
	class_java_lang_NullPointerException = 
		loader_load ( unicode_new_char ("java/lang/NullPointerException") );
	class_java_lang_ArrayIndexOutOfBoundsException = loader_load ( 
	     unicode_new_char ("java/lang/ArrayIndexOutOfBoundsException") );
	class_java_lang_NegativeArraySizeException = loader_load ( 
	     unicode_new_char ("java/lang/NegativeArraySizeException") );
	class_java_lang_OutOfMemoryError = loader_load ( 
	     unicode_new_char ("java/lang/OutOfMemoryError") );
	class_java_lang_ArrayStoreException =
		loader_load ( unicode_new_char ("java/lang/ArrayStoreException") );
	class_java_lang_ArithmeticException = 
		loader_load ( unicode_new_char ("java/lang/ArithmeticException") );
	class_java_lang_ThreadDeath =                             /* schani */
	        loader_load ( unicode_new_char ("java/lang/ThreadDeath") );

	class_array = loader_createarrayclass ();


	proto_java_lang_ClassCastException = 
		builtin_new(class_java_lang_ClassCastException);
	heap_addreference ( (void**) &proto_java_lang_ClassCastException);

	proto_java_lang_NullPointerException = 
		builtin_new(class_java_lang_NullPointerException);
	heap_addreference ( (void**) &proto_java_lang_NullPointerException);

	proto_java_lang_ArrayIndexOutOfBoundsException = 
		builtin_new(class_java_lang_ArrayIndexOutOfBoundsException);
	heap_addreference ( (void**) &proto_java_lang_ArrayIndexOutOfBoundsException);

	proto_java_lang_NegativeArraySizeException = 
		builtin_new(class_java_lang_NegativeArraySizeException);
	heap_addreference ( (void**) &proto_java_lang_NegativeArraySizeException);

	proto_java_lang_OutOfMemoryError = 
		builtin_new(class_java_lang_OutOfMemoryError);
	heap_addreference ( (void**) &proto_java_lang_OutOfMemoryError);

	proto_java_lang_ArithmeticException = 
		builtin_new(class_java_lang_ArithmeticException);
	heap_addreference ( (void**) &proto_java_lang_ArithmeticException);

	proto_java_lang_ArrayStoreException = 
		builtin_new(class_java_lang_ArrayStoreException);
	heap_addreference ( (void**) &proto_java_lang_ArrayStoreException);

	proto_java_lang_ThreadDeath =                             /* schani */
	        builtin_new(class_java_lang_ThreadDeath);
	heap_addreference ( (void**) &proto_java_lang_ThreadDeath);
}




/********************* Funktion: loader_initclasses ****************************

	initialisiert alle geladenen aber noch nicht initialisierten Klassen

*******************************************************************************/

void loader_initclasses ()
{
	classinfo *c;
	
	intsDisable();                     /* schani */

	if (makeinitializations) {
		c = list_first (&linkedclasses);
		while (c) {
			class_init (c);
			c = list_next (&linkedclasses, c);
			}
		}

	intsRestore();                      /* schani */
}

static s4 classvalue = 0;

static void loader_compute_class_values (classinfo *c)
{
	classinfo *subs;

	c->vftbl->baseval = ++classvalue;
	subs = c->sub;
	while (subs != NULL) {
		loader_compute_class_values(subs);
		subs = subs->nextsub;
		}
	c->vftbl->diffval = classvalue - c->vftbl->baseval;
/*
	{
	int i;
	for (i = 0; i < c->index; i++)
		printf(" ");
	printf("%3d  %3d  ", (int) c->vftbl->baseval, c->vftbl->diffval);
	unicode_display(c->name);
	printf("\n");
	}
*/
}


void loader_compute_subclasses ()
{
	classinfo *c;
	
	intsDisable();                     /* schani */

	c = list_first (&linkedclasses);
	while (c) {
		if (!(c->flags & ACC_INTERFACE) && (c->super != NULL)) {
			c->nextsub = c->super->sub;
			c->super->sub = c;
			}
		c = list_next (&linkedclasses, c);
		}

	loader_compute_class_values(class_java_lang_Object);

	intsRestore();                      /* schani */
}



/******************** Funktion: loader_close ***********************************

	gibt alle Resourcen wieder frei
	
*******************************************************************************/

void loader_close ()
{
	classinfo *c;

	while ( (c=list_first(&unloadedclasses)) ) {
		list_remove (&unloadedclasses,c);
		class_free (c);
		}
	while ( (c=list_first(&unlinkedclasses)) ) {
		list_remove (&unlinkedclasses,c);
		class_free (c);
		}
	while ( (c=list_first(&linkedclasses)) ) {
		list_remove (&linkedclasses,c);
		class_free (c);
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
