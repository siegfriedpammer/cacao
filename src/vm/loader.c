
/* loader.c ********************************************************************

	Copyright (c) 1999 A. Krall, R. Grafl, R. Obermaiser, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Contains the functions of the class loader.

	Author:  Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
	Changes: Andreas Krall       EMAIL: cacao@complang.tuwien.ac.at
	         Roman Obermaiser    EMAIL: cacao@complang.tuwien.ac.at
	         Mark Probst         EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1999/11/08

*******************************************************************************/


#include <assert.h>

#include "global.h"
#include "loader.h"
#include "native.h"
#include "tables.h"
#include "builtin.h"
#include "jit.h"
#ifdef OLD_COMPILER
#include "compiler.h"
#endif
#include "asmpart.h"

#include "threads/thread.h"
#include <sys/stat.h>

/* global variables ***********************************************************/

extern bool newcompiler;        /* true if new compiler is used               */    		

int count_class_infos = 0;      /* variables for measurements                 */
int count_const_pool_len = 0;
int count_vftbl_len = 0;
int count_all_methods = 0;
int count_vmcode_len = 0;
int count_extable_len = 0;

bool loadverbose = false;       /* switches for debug messages                */
bool linkverbose = false;
bool initverbose = false;

bool makeinitializations = true;

bool getloadingtime  = false;   /* to measure the runtime                     */
long int loadingtime = 0;

static s4 interfaceindex;       /* sequential numbering of interfaces         */ 

list unloadedclasses;       /* list of all referenced but not loaded classes  */
list unlinkedclasses;       /* list of all loaded but not linked classes      */
list linkedclasses;         /* list of all completely linked classes          */


/* utf-symbols for pointer comparison of frequently used strings */

static utf *utf_innerclasses; 		/* InnerClasses   		   */
static utf *utf_constantvalue; 		/* ConstantValue 		   */
static utf *utf_code;			    /* Code 			       */
static utf *utf_finalize;		    /* finalize 			   */
static utf *utf_fidesc;   		    /* ()V 				       */
static utf *utf_clinit;  		    /* <clinit> 			   */
static utf *utf_initsystemclass;	/* initializeSystemClass   */
static utf *utf_systemclass;		/* java/lang/System 	   */


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
classinfo *class_java_lang_ThreadDeath;
classinfo *class_array = NULL;


/******************************************************************************

   structure for primitive classes: contains the class for wrapping the 
   primitive type, the primitive class, the name of the class for wrapping, 
   the one character type signature and the name of the primitive class
 
 ******************************************************************************/
 
primitivetypeinfo primitivetype_table[PRIMITIVETYPE_COUNT] = { 
		{ NULL, NULL, "java/lang/Double",    'D', "double"  },
		{ NULL, NULL, "java/lang/Float",     'F', "float"   },
  		{ NULL, NULL, "java/lang/Character", 'C', "char"    },
  		{ NULL, NULL, "java/lang/Integer",   'I', "int"     },
  		{ NULL, NULL, "java/lang/Long",      'J', "long"    },
  		{ NULL, NULL, "java/lang/Byte",	     'B', "byte"    },
  		{ NULL, NULL, "java/lang/Short",     'S', "short"   },
  		{ NULL, NULL, "java/lang/Boolean",   'Z', "boolean" },
  		{ NULL, NULL, "java/lang/Void",	     'V', "void"    }};


/* instances of important system classes **************************************/

java_objectheader *proto_java_lang_ClassCastException;
java_objectheader *proto_java_lang_NullPointerException;
java_objectheader *proto_java_lang_ArrayIndexOutOfBoundsException;
java_objectheader *proto_java_lang_NegativeArraySizeException;
java_objectheader *proto_java_lang_OutOfMemoryError;
java_objectheader *proto_java_lang_ArithmeticException;
java_objectheader *proto_java_lang_ArrayStoreException;
java_objectheader *proto_java_lang_ThreadDeath;


/************* functions for reading classdata *********************************

    getting classdata in blocks of variable size
    (8,16,32,64-bit integer or float)

*******************************************************************************/

static char *classpath    = "";     /* searchpath for classfiles              */
static u1 *classbuffer    = NULL;   /* pointer to buffer with classfile-data  */
static u1 *classbuf_pos;            /* current position in classfile buffer   */
static int classbuffer_size;        /* size of classfile-data                 */

/* transfer block of classfile data into a buffer */

#define suck_nbytes(buffer,len) memcpy(buffer,classbuf_pos+1,len); \
                                classbuf_pos+=len;

/* skip block of classfile data */

#define skip_nbytes(len) classbuf_pos+=len;

#define suck_u1() (*++classbuf_pos)
#define suck_s8() (s8) suck_u8()
#define suck_s2() (s2) suck_u2()
#define suck_s4() (s4) suck_u4()
#define suck_s1() (s1) suck_u1()
#define suck_u2() (u2) ((suck_u1()<<8)+suck_u1())
#define suck_u4() (u4) ((((u4)suck_u1())<<24)+(((u4)suck_u1())<<16)+(((u4)suck_u1())<<8)+((u4)suck_u1()))


/* get u8 from classfile data */

static u8 suck_u8 ()
{
#if U8_AVAILABLE
	u8 lo,hi;
	hi = suck_u4();
	lo = suck_u4();
	return (hi<<32) + lo;
#else
	u8 v;
	v.high = suck_u4();
	v.low = suck_u4();
	return v;
#endif
}

/* get float from classfile data */

static float suck_float ()
{
	float f;

#if !WORDS_BIGENDIAN 
		u1 buffer[4];
		u2 i;
		for (i=0; i<4; i++) buffer[3-i] = suck_u1 ();
		memcpy ( (u1*) (&f), buffer, 4);
#else 
		suck_nbytes ( (u1*) (&f), 4 );
#endif

	PANICIF (sizeof(float) != 4, "Incompatible float-format");
	
	return f;
}

/* get double from classfile data */
static double suck_double ()
{
	double d;

#if !WORDS_BIGENDIAN 
		u1 buffer[8];
		u2 i;	
		for (i=0; i<8; i++) buffer[7-i] = suck_u1 ();
		memcpy ( (u1*) (&d), buffer, 8);
#else 
		suck_nbytes ( (u1*) (&d), 8 );
#endif

	PANICIF (sizeof(double) != 8, "Incompatible double-format" );
	
	return d;
}

/************************** function suck_init *********************************

	called once at startup, sets the searchpath for the classfiles

*******************************************************************************/

void suck_init (char *cpath)
{
	classpath   = cpath;
	classbuffer = NULL;
}


/************************** function suck_start ********************************

	returns true if classbuffer is already loaded or a file for the
	specified class has succussfully been read in. All directories of
	the searchpath are used to find the classfile (<classname>.class).
	Returns false if no classfile is found and writes an error message. 
	
*******************************************************************************/


bool suck_start (utf *classname) {

#define MAXFILENAME 1000 	        /* maximum length of a filename           */
	
	char filename[MAXFILENAME+10];  /* room for '.class'                      */	
	char *pathpos;                  /* position in searchpath                 */
	char c, *utf_ptr;               /* pointer to the next utf8-character     */
	FILE *classfile;
	int  filenamelen, err;
	struct stat buffer;
	
	if (classbuffer)                /* classbuffer is already valid */
		return true;

	pathpos = classpath;
	
	while (*pathpos) {

		/* skip path separator */

		while (*pathpos == ':')
			pathpos++;
 
 		/* extract directory from searchpath */

		filenamelen = 0;
		while ((*pathpos) && (*pathpos!=':')) {
		    PANICIF (filenamelen >= MAXFILENAME, "Filename too long") ;
			filename[filenamelen++] = *(pathpos++);
			}

		filename[filenamelen++] = '/';  
   
   		/* add classname to filename */

		utf_ptr = classname->text;
		while (utf_ptr < utf_end(classname)) {
			PANICIF (filenamelen >= MAXFILENAME, "Filename too long");
			c = *utf_ptr++;
			if ((c <= ' ' || c > 'z') && (c != '/'))     /* invalid character */ 
				c = '?'; 
			filename[filenamelen++] = c;	
			}
      
		/* add suffix */

		strcpy (filename+filenamelen, ".class");

		classfile = fopen(filename, "r");
		if (classfile) {                                       /* file exists */
			
			/* determine size of classfile */

			err = stat (filename, &buffer);

			if (!err) {                                /* read classfile data */				
				classbuffer_size = buffer.st_size;				
				classbuffer      = MNEW(u1, classbuffer_size);
				classbuf_pos     = classbuffer-1;
				fread(classbuffer, 1, classbuffer_size, classfile);
				fclose(classfile);
				return true;
			}
		}
	}

	sprintf (logtext,"Warning: Can not open class file '%s'", filename);
	dolog();

	return false;
}


/************************** function suck_stop *********************************

	frees memory for buffer with classfile data.
	Caution: this function may only be called if buffer has been allocated
	         by suck_start with reading a file
	
*******************************************************************************/

void suck_stop () {

	/* determine amount of classdata not retrieved by suck-operations         */

	int classdata_left = ((classbuffer+classbuffer_size)-classbuf_pos-1);

	if (classdata_left > 0) {        	
		/* surplus */        	
		sprintf (logtext,"There are %d access bytes at end of classfile",
	                 classdata_left);
		dolog();
	}

	/* free memory */

	MFREE(classbuffer, u1, classbuffer_size);
	classbuffer = NULL;
}

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

/******************** function: innerclass_getconstant ************************

    like class_getconstant, but if cptags is ZERO null is returned					 
	
*******************************************************************************/

voidptr innerclass_getconstant (classinfo *c, u4 pos, u4 ctype) 
{
	/* invalid position in constantpool */
	if (pos >= c->cpcount) 
		panic ("Attempt to access constant outside range");

	/* constantpool entry of type 0 */	
	if (!c->cptags[pos])
		return NULL;

	/* check type of constantpool entry */
	if (c->cptags[pos] != ctype) {
		sprintf (logtext, "Type mismatch on constant: %d requested, %d here",
		 (int) ctype, (int) c->cptags[pos] );
		error();
		}
		
	return c->cpinfos[pos];
}

/************************ function: attribute_load ****************************

    read attributes from classfile
	
*******************************************************************************/

static void attribute_load (u4 num, classinfo *c)
{
	u4 i,j;

	for (i = 0; i < num; i++) {
		/* retrieve attribute name */	
		utf *aname = class_getconstant (c, suck_u2(), CONSTANT_Utf8);

		if ( aname == utf_innerclasses)  {			
			/* innerclasses attribute */
				
			/* skip attribute length */						
			suck_u4(); 
			/* number of records */
			c->innerclasscount = suck_u2();
			/* allocate memory for innerclass structure */
			c->innerclass = MNEW (innerclassinfo, c->innerclasscount);

			for (j=0;j<c->innerclasscount;j++) {
				
				/*  The innerclass structure contains a class with an encoded name, 
				    its defining scope, its simple name  and a bitmask of the access flags. 
				    If an inner class is not a member, its outer_class is NULL, 
				    if a class is anonymous, its name is NULL.  			    */
   								
				innerclassinfo *info = c->innerclass + j;

				info->inner_class = innerclass_getconstant(c, suck_u2(), CONSTANT_Class); /* CONSTANT_Class_info index */
				info->outer_class = innerclass_getconstant(c, suck_u2(), CONSTANT_Class); /* CONSTANT_Class_info index */
				info->name  = innerclass_getconstant(c, suck_u2(), CONSTANT_Utf8);        /* CONSTANT_Utf8_info index  */
				info->flags = suck_u2 ();					          /* access_flags bitmask      */
			}
		} else {
			/* unknown attribute */
			skipattributebody ();
		}
	}
}

/******************* function: checkfielddescriptor ****************************

	checks whether a field-descriptor is valid and aborts otherwise
	all referenced classes are inserted into the list of unloaded classes
	
*******************************************************************************/

static void checkfielddescriptor (char *utf_ptr, char *end_pos)
{
	char *tstart;  /* pointer to start of classname */
	char ch;
	
	switch (*utf_ptr++) {
	case 'B':
	case 'C':
	case 'I':
	case 'S':
	case 'Z':  
	case 'J':  
	case 'F':  
	case 'D':
		/* primitive type */  
		break;

	case 'L':
		/* save start of classname */
		tstart = utf_ptr;  
		
		/* determine length of classname */
		while ( *utf_ptr++ != ';' )
			if (utf_ptr>=end_pos) 
				panic ("Missing ';' in objecttype-descriptor");

		/* cause loading of referenced class */			
		class_new ( utf_new(tstart, utf_ptr-tstart-1) );
		break;

	case '[' : 
		/* array type */
		while ((ch = *utf_ptr++)=='[') 
			/* skip */ ;

		/* component type of array */
		switch (ch) {
		case 'B':
		case 'C':
		case 'I':
		case 'S':
		case 'Z':  
		case 'J':  
		case 'F':  
		case 'D':
			/* primitive type */  
			break;
			
		case 'L':
			/* save start of classname */
			tstart = utf_ptr;
			
			/* determine length of classname */
			while ( *utf_ptr++ != ';' )
				if (utf_ptr>=end_pos) 
					panic ("Missing ';' in objecttype-descriptor");

			/* cause loading of referenced class */			
			class_new ( utf_new(tstart, utf_ptr-tstart-1) );
			break;

		default:   
			panic ("Ill formed methodtype-descriptor");
		}
		break;
			
	default:   
		panic ("Ill formed methodtype-descriptor");
	}			

        /* exceeding characters */        	
	if (utf_ptr!=end_pos) panic ("descriptor has exceeding chars");
}


/******************* function checkmethoddescriptor ****************************

    checks whether a method-descriptor is valid and aborts otherwise.
    All referenced classes are inserted into the list of unloaded classes.
	
*******************************************************************************/

static void checkmethoddescriptor (utf *d)
{
	char *utf_ptr = d->text;     /* current position in utf text   */
	char *end_pos = utf_end(d);  /* points behind utf string       */
	char *tstart;                /* pointer to start of classname  */
	char c,ch;

	/* method descriptor must start with parenthesis */
	if (*utf_ptr++ != '(') panic ("Missing '(' in method descriptor");

	/* check arguments */
	while ((c = *utf_ptr++) != ')') {
		switch (c) {
		case 'B':
		case 'C':
		case 'I':
		case 'S':
		case 'Z':  
		case 'J':  
		case 'F':  
		case 'D':
			/* primitive type */  
			break;

		case 'L':
			/* save start of classname */ 
			tstart = utf_ptr;
			
			/* determine length of classname */
			while ( *utf_ptr++ != ';' )
				if (utf_ptr>=end_pos) 
					panic ("Missing ';' in objecttype-descriptor");
			
			/* cause loading of referenced class */
			class_new ( utf_new(tstart, utf_ptr-tstart-1) );
			break;
	   
		case '[' :
			/* array type */ 
			while ((ch = *utf_ptr++)=='[') 
				/* skip */ ;

			/* component type of array */
			switch (ch) {
			case 'B':
			case 'C':
			case 'I':
			case 'S':
			case 'Z':  
			case 'J':  
			case 'F':  
			case 'D':
				/* primitive type */  
				break;

			case 'L':
				/* save start of classname */
				tstart = utf_ptr;
			
				/* determine length of classname */	
				while ( *utf_ptr++ != ';' )
					if (utf_ptr>=end_pos) 
						panic ("Missing ';' in objecttype-descriptor");

				/* cause loading of referenced class */
				class_new ( utf_new(tstart, utf_ptr-tstart-1) );
				break;

			default:   
				panic ("Ill formed methodtype-descriptor");
			} 
			break;
			
		default:   
			panic ("Ill formed methodtype-descriptor");
		}			
	}

	/* check returntype */
	if (*utf_ptr=='V') {
		/* returntype void */
		if ((utf_ptr+1) != end_pos) panic ("Method-descriptor has exceeding chars");
	}
	else
		/* treat as field-descriptor */
		checkfielddescriptor (utf_ptr,end_pos);
}


/******************** Funktion: buildarraydescriptor ***************************

	erzeugt zu einem namentlich als utf-String vorliegenden Arraytyp eine
	entsprechende constant_arraydescriptor - Struktur 
	
*******************************************************************************/

constant_arraydescriptor * buildarraydescriptor(char *utf_ptr, u4 namelen)
{
	constant_arraydescriptor *d;
	
	if (*utf_ptr++ != '[') panic ("Attempt to build arraydescriptor for non-array");

	d = NEW (constant_arraydescriptor);
	d -> objectclass = NULL;
	d -> elementdescriptor = NULL;

#ifdef STATISTICS
	count_const_pool_len += sizeof(constant_arraydescriptor);
#endif

	switch (*utf_ptr) {
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
		d -> elementdescriptor = buildarraydescriptor (utf_ptr, namelen-1);
		break;
		
	case 'L':
		d -> arraytype = ARRAYTYPE_OBJECT;
		d -> objectclass = class_new ( utf_new(utf_ptr+1, namelen-3) );
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
	case ARRAYTYPE_OBJECT: utf_display(d->objectclass->name); printf("[]"); break;
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

	f -> flags = suck_u2 ();                                           /* ACC flags 		  */
	f -> name = class_getconstant (c, suck_u2(), CONSTANT_Utf8);       /* name of field               */
	f -> descriptor = class_getconstant (c, suck_u2(), CONSTANT_Utf8); /* JavaVM descriptor           */
	f -> type = jtype = desc_to_type (f->descriptor);		   /* data type                   */
	f -> offset = 0;						   /* offset from start of object */
	
	switch (f->type) {
	case TYPE_INT:        f->value.i = 0; break;
	case TYPE_FLOAT:      f->value.f = 0.0; break;
	case TYPE_DOUBLE:     f->value.d = 0.0; break;
	case TYPE_ADDRESS:    f->value.a = NULL; 			      
	                      heap_addreference (&(f->value.a));           /* make global reference (GC)  */	
	                      break;
	case TYPE_LONG:
#if U8_AVAILABLE
		f->value.l = 0; break;
#else
		f->value.l.low = 0; f->value.l.high = 0; break;
#endif 
	}

	/* read attributes */
	attrnum = suck_u2();
	for (i=0; i<attrnum; i++) {
		u4 pindex;
		utf *aname;

		aname = class_getconstant (c, suck_u2(), CONSTANT_Utf8);
		
		if ( aname != utf_constantvalue ) {
			/* unknown attribute */
			skipattributebody ();
			}
		else {
			/* constant value attribute */
			
			/* skip attribute length */
			suck_u4();
			/* index of value in constantpool */		
			pindex = suck_u2();
		
			/* initialize field with value from constantpool */		
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
					utf *u = class_getconstant(c, pindex, CONSTANT_String);
					/* create javastring from compressed utf8-string */					
					f->value.a = literalstring_new(u);
					}
					break;
	
				default: 
					log_text ("Invalid Constant - Type");

				}

			}
		}
}


/********************** function: field_free **********************************/

static void field_free (fieldinfo *f)
{
	/* empty */
}


/************** Funktion: field_display (nur zu Debug-Zwecken) ****************/

static void field_display (fieldinfo *f)
{
	printf ("   ");
	printflags (f -> flags);
	printf (" ");
	utf_display (f -> name);
	printf (" ");
	utf_display (f -> descriptor);	
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
		utf *aname;

		aname = class_getconstant (c, suck_u2(), CONSTANT_Utf8);

		if ( aname != utf_code)  {
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
	utf_display (m -> name);
	printf (" "); 
	utf_display (m -> descriptor);
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


/******************** function: class_getconstant ******************************

	retrieves the value at position 'pos' of the constantpool of a class
	if the type of the value is other than 'ctype' the system is stopped

*******************************************************************************/

voidptr class_getconstant (classinfo *c, u4 pos, u4 ctype) 
{
	/* invalid position in constantpool */	
	if (pos >= c->cpcount) 
		panic ("Attempt to access constant outside range");

	/* check type of constantpool entry */
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


/******************** function: class_loadcpool ********************************

	loads the constantpool of a class, 
	the entries are transformed into a simpler format 
	by resolving references
	(a detailed overview of the compact structures can be found in global.h)	

*******************************************************************************/

static void class_loadcpool (classinfo *c)
{

	/* The following structures are used to save information which cannot be 
	   processed during the first pass. After the complete constantpool has 
	   been traversed the references can be resolved. 
	   (only in specific order)						   */
	
	/* CONSTANT_Class_info entries */
	typedef struct forward_class {      
		struct forward_class *next; 
		u2 thisindex;               
		u2 name_index;              
	} forward_class;                    
                                        
	/* CONSTANT_String */                                      
	typedef struct forward_string {	
		struct forward_string *next; 
		u2 thisindex;	             
		u2 string_index;
	} forward_string;

	/* CONSTANT_NameAndType */
	typedef struct forward_nameandtype {
		struct forward_nameandtype *next;
		u2 thisindex;
		u2 name_index;
		u2 sig_index;
	} forward_nameandtype;

	/* CONSTANT_Fieldref, CONSTANT_Methodref or CONSTANT_InterfaceMethodref */
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

	/* number of entries in the constant_pool table  */
	u4 cpcount       = c -> cpcount = suck_u2();
	/* allocate memory */
	u1 *cptags       = c -> cptags  = MNEW (u1, cpcount);
	voidptr *cpinfos = c -> cpinfos = MNEW (voidptr, cpcount);

#ifdef STATISTICS
	count_const_pool_len += (sizeof(voidptr) + 1) * cpcount;
#endif
	
	/* initialize constantpool */
	for (idx=0; idx<cpcount; idx++) {
		cptags[idx] = CONSTANT_UNUSED;
		cpinfos[idx] = NULL;
		}

			
		/******* first pass *******/
		/* entries which cannot be resolved now are written into 
		   temporary structures and traversed again later        */
		   
	idx = 1;
	while (idx < cpcount) {
		/* get constant type */
		u4 t = suck_u1 (); 
		switch ( t ) {

			case CONSTANT_Class: { 
				forward_class *nfc = DNEW(forward_class);

				nfc -> next = forward_classes; 											
				forward_classes = nfc;

				nfc -> thisindex = idx;
				/* reference to CONSTANT_NameAndType */
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
				/* constant type */
				nff -> tag = t;
				/* class or interface type that contains the declaration of the field or method */
				nff -> class_index = suck_u2 (); 
				/* name and descriptor of the field or method */
				nff -> nameandtype_index = suck_u2 ();

				idx ++;
				break;
				}
				
			case CONSTANT_String: {
				forward_string *nfs = DNEW (forward_string);
				
				nfs -> next = forward_strings;
				forward_strings = nfs;
				
				nfs -> thisindex = idx;
				/* reference to CONSTANT_Utf8_info with string characters */
				nfs -> string_index = suck_u2 ();
				
				idx ++;
				break;
				}

			case CONSTANT_NameAndType: {
				forward_nameandtype *nfn = DNEW (forward_nameandtype);
				
				nfn -> next = forward_nameandtypes;
				forward_nameandtypes = nfn;
				
				nfn -> thisindex = idx;
				/* reference to CONSTANT_Utf8_info containing simple name */
				nfn -> name_index = suck_u2 ();
				/* reference to CONSTANT_Utf8_info containing field or method descriptor */
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

				/* number of bytes in the bytes array (not string-length) */
				u4 length = suck_u2();
				cptags [idx]  = CONSTANT_Utf8;
				/* insert utf-string into the utf-symboltable */
				cpinfos [idx] = utf_new(classbuf_pos+1, length);
				/* skip bytes of the string */
				skip_nbytes(length);
				idx++;
				break;
				}
										
			default:
				sprintf (logtext, "Unkown constant type: %d",(int) t);
				error ();
		
			}  /* end switch */
			
		} /* end while */
		


	   /* resolve entries in temporary structures */

	while (forward_classes) {
		utf *name =
		  class_getconstant (c, forward_classes -> name_index, CONSTANT_Utf8);
		
		if ( (name->blength>0) && (name->text[0]=='[') ) {
			/* check validity of descriptor */
			checkfielddescriptor (name->text, utf_end(name)); 

			cptags  [forward_classes -> thisindex] = CONSTANT_Arraydescriptor;
			cpinfos [forward_classes -> thisindex] = 
			   buildarraydescriptor(name->text, name->blength);

			}
		else {					
			cptags  [forward_classes -> thisindex] = CONSTANT_Class;
			/* retrieve class from class-table */
			cpinfos [forward_classes -> thisindex] = class_new (name);
			}
		forward_classes = forward_classes -> next;
		
		}

	while (forward_strings) {
		utf *text = 
		  class_getconstant (c, forward_strings -> string_index, CONSTANT_Utf8);
	
		/* resolve utf-string */		
		cptags   [forward_strings -> thisindex] = CONSTANT_String;
		cpinfos  [forward_strings -> thisindex] = text;
		
		forward_strings = forward_strings -> next;
		}	

	while (forward_nameandtypes) {
		constant_nameandtype *cn = NEW (constant_nameandtype);	

#ifdef STATISTICS
		count_const_pool_len += sizeof(constant_nameandtype);
#endif

		/* resolve simple name and descriptor */
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
		/* resolve simple name and descriptor */
		nat = class_getconstant
			(c, forward_fieldmethints -> nameandtype_index, CONSTANT_NameAndType);

		fmi -> class = class_getconstant 
			(c, forward_fieldmethints -> class_index, CONSTANT_Class);
		fmi -> name = nat -> name;
		fmi -> descriptor = nat -> descriptor;

		cptags [forward_fieldmethints -> thisindex] = forward_fieldmethints -> tag;
		cpinfos [forward_fieldmethints -> thisindex] = fmi;
	
		switch (forward_fieldmethints -> tag) {
		case CONSTANT_Fieldref:  /* check validity of descriptor */
					 checkfielddescriptor (fmi->descriptor->text,utf_end(fmi->descriptor));
		                         break;
		case CONSTANT_InterfaceMethodref: 
		case CONSTANT_Methodref: /* check validity of descriptor */
					 checkmethoddescriptor (fmi->descriptor);
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

static int class_load (classinfo *c)
{
	u4 i;
	u4 mi,ma;

	/* output for debugging purposes */
	if (loadverbose) {		
		sprintf (logtext, "Loading class: ");
		utf_sprint (logtext+strlen(logtext), c->name );
		dolog();
		}
	
	/* load classdata, throw exception on error */
	if (!suck_start (c->name)) {
		throw_classnotfoundexception();		   
		return false;
	}
	
	/* check signature */		
	if (suck_u4() != MAGIC) panic("Can not find class-file signature");   	
	/* check version */
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
	
	/* ACC flags */
	c -> flags = suck_u2 (); 
	/* this class */
	suck_u2 ();       
	
	/* retrieve superclass */
	if ( (i = suck_u2 () ) ) {
		c -> super = class_getconstant (c, i, CONSTANT_Class);
		}
	else {
		c -> super = NULL;
		}
			 
	/* retrieve interfaces */
	c -> interfacescount = suck_u2 ();
	c -> interfaces = MNEW (classinfo*, c -> interfacescount);
	for (i=0; i < c -> interfacescount; i++) {
		c -> interfaces [i] = 
		      class_getconstant (c, suck_u2(), CONSTANT_Class);
		}

	/* load fields */
	c -> fieldscount = suck_u2 ();
	c -> fields = MNEW (fieldinfo, c -> fieldscount);
	for (i=0; i < c -> fieldscount; i++) {
		field_load (&(c->fields[i]), c);
		}

	/* load methods */
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

	/* load variable-length attribute structures */	
	attribute_load (suck_u2(), c);

	/* free memory */
	suck_stop ();

	/* remove class from list of unloaded classes and 
	   add to list of unlinked classes	          */
	list_remove (&unloadedclasses, c);
	list_addlast (&unlinkedclasses, c);

	return true;
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
    	utf_sprint (logtext+strlen(logtext), c->name);
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
		utf_sprint (logtext+strlen(logtext), c->name );
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
		static utf *finame = NULL;
		static utf *fidesc = NULL;

		if (finame == NULL)
			finame = utf_finalize;
		if (fidesc == NULL)
			fidesc = utf_fidesc;

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

	if (c->innerclasscount)
		MFREE (c->innerclass, innerclassinfo, c->innerclasscount);

	FREE (c, classinfo);
}

/************************* Funktion: class_findfield ***************************
	
	sucht in einer 'classinfo'-Struktur nach einem Feld mit gew"unschtem
	Namen und Typ.

*******************************************************************************/


fieldinfo *class_findfield (classinfo *c, utf *name, utf *desc)
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

methodinfo *class_findmethod (classinfo *c, utf *name, utf *desc)
{
	s4 i;
	for (i = 0; i < c->methodscount; i++) {
		if ((c->methods[i].name == name) && ((desc == NULL) ||
		                                   (c->methods[i].descriptor == desc)))
			return &(c->methods[i]);
		}
	return NULL;
}

/************************* Funktion: class_findmethod_approx ******************
	
	sucht in einer 'classinfo'-Struktur nach einer Methode mit gew"unschtem
	Namen und Typ.
	Wenn als Typ NULL angegeben wird, dann ist der Typ egal.
	beim Vergleichen des Descriptors wird der R"uckgabewert nicht betrachtet

*******************************************************************************/

methodinfo *class_findmethod_approx (classinfo *c, utf *name, utf *desc)
{
	s4 i;

	for (i = 0; i < c->methodscount; i++) 
		if (c->methods[i].name == name) {
			utf *meth_descr = c->methods[i].descriptor;
			
			if (desc == NULL) 
				/* ignore type */
				return &(c->methods[i]);

			if (desc->blength <= meth_descr->blength) {
					   /* current position in utf text   */
					   char *desc_utf_ptr = desc->text;      
					   char *meth_utf_ptr = meth_descr->text;					  
					   /* points behind utf strings */
					   char *desc_end = utf_end(desc);         
					   char *meth_end = utf_end(meth_descr);   
					   char ch;

					   /* compare argument types */
					   while (desc_utf_ptr<desc_end && meth_utf_ptr<meth_end) {

						   if ((ch=*desc_utf_ptr++) != (*meth_utf_ptr++))
							   break; /* no match */

						   if (ch==')')
							   return &(c->methods[i]);   /* all parameter types equal */
					   }
			}
		}

	return NULL;
}

/***************** Funktion: class_resolvemethod_approx ***********************
	
	sucht eine Klasse und alle Superklassen ab, um eine Methode zu finden.
	(ohne Beachtung des R"uckgabewertes)

*******************************************************************************/

methodinfo *class_resolvemethod_approx (classinfo *c, utf *name, utf *desc)
{
	while (c) {
		/* search for method (ignore returntype) */
		methodinfo *m = class_findmethod_approx (c, name, desc);
		/* method found */
		if (m) return m;
		/* search superclass */
		c = c->super;
		}
	return NULL;
}


/************************* Funktion: class_resolvemethod ***********************
	
	sucht eine Klasse und alle Superklassen ab, um eine Methode zu finden.

*******************************************************************************/


methodinfo *class_resolvemethod (classinfo *c, utf *name, utf *desc)
{
	while (c) {
		methodinfo *m = class_findmethod (c, name, desc);
		if (m) return m;
		/* search superclass */
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
	                      utf_clinit, 
	                      utf_fidesc);
	if (!m) {
		if (initverbose) {
			sprintf (logtext, "Class ");
			utf_sprint (logtext+strlen(logtext), c->name);
  			sprintf (logtext+strlen(logtext), " has no initializer");	
			dolog ();
			}
		return;
		}
		
	if (! (m->flags & ACC_STATIC)) panic ("Class initializer is not static!");
	
	if (initverbose) {
		sprintf (logtext, "Starting initializer for class: ");
		utf_sprint (logtext+strlen(logtext), c->name);
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
		printf ("#### Initializer of ");
		utf_display (c->name);
		printf (" has thrown: ");
		utf_display (exceptionptr->vftbl->class->name);
		printf ("\n");
		fflush (stdout);
		}

	if (initverbose) {
		sprintf (logtext, "Finished initializer for class: ");
		utf_sprint (logtext+strlen(logtext), c->name);
		dolog ();
	}

	if (c->name == utf_systemclass) {
		/* class java.lang.System requires explicit initialization */
		
		if (initverbose)
			printf ("#### Initializing class System");

		/* find initializing method */	   
		m = class_findmethod (c, 
					utf_initsystemclass,
					utf_fidesc);

		if (!m) {
			/* no method found */
			log("initializeSystemClass failed");
			return;
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
			printf ("#### initializeSystemClass has thrown: ");
			utf_display (exceptionptr->vftbl->class->name);
			printf ("\n");
			fflush (stdout);		  
		}
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
					utf_display ( ((classinfo*)e) -> name );
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
					utf_display ( fmi->class->name );
					printf (".");
					utf_display ( fmi->name);
					printf (" ");
					utf_display ( fmi->descriptor );
				    }
					break;

				case CONSTANT_String:
					printf ("String -> ");
					utf_display (e);
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
					  utf_display (cnt->name);
					  printf (" ");
					  utf_display (cnt->descriptor);
					}
					break;
				case CONSTANT_Utf8:
					printf ("Utf8 -> ");
					utf_display (e);
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

	printf ("This: "); utf_display (c->name); printf ("\n");
	if (c->super) {
		printf ("Super: "); utf_display (c->super->name); printf ("\n");
		}
	printf ("Index: %d\n", c->index);
	
	printf ("interfaces:\n");	
	for (i=0; i < c-> interfacescount; i++) {
		printf ("   ");
		utf_display (c -> interfaces[i] -> name);
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

static int loader_inited = 0;

/********************* Funktion: loader_load ***********************************

	l"adt und linkt die ge"unschte Klasse und alle davon 
	referenzierten Klassen und Interfaces
	Return: Einen Zeiger auf diese Klasse

*******************************************************************************/

classinfo *loader_load (utf *topname)
{
	classinfo *top;
	classinfo *c;
	long int starttime=0,stoptime=0;
	
	intsDisable();                           /* schani */

	if (getloadingtime) starttime = getcputime();

	top = class_new (topname);

	/* load classes */
	while ( (c = list_first(&unloadedclasses)) ) {
		if (!class_load (c)) {
			list_remove (&unloadedclasses, c);
			top=NULL;
		    }
        }

	/* link classes */
	while ( (c = list_first(&unlinkedclasses)) ) {
		class_link (c);
		}

	/* measure time */
	if (getloadingtime) {
		stoptime = getcputime();
		loadingtime += (stoptime-starttime);
		}

	if (loader_inited)
		loader_compute_subclasses();

	intsRestore();                          /* schani */

	return top; 
}


/**************** function: create_primitive_classes ***************************

	create classes representing primitive types 

********************************************************************************/


void create_primitive_classes()
{  
	int i;

	for (i=0;i<PRIMITIVETYPE_COUNT;i++) {
		/* create primitive class */
		classinfo *c = class_new ( utf_new_char(primitivetype_table[i].name) );
		
		/* prevent loader from loading primitive class */
		list_remove (&unloadedclasses, c);
		/* add to unlinked classes */
		list_addlast (&unlinkedclasses, c);		
		c -> super = class_java_lang_Object;
		class_link (c);

		primitivetype_table[i].class_primitive = c;

		/* create class for wrapping the primitive type */
		primitivetype_table[i].class_wrap =
	        	class_new( utf_new_char(primitivetype_table[i].wrapname) );
	}
}

/***************** function: create_array_class ********************************

	create class representing an array

********************************************************************************/


classinfo *create_array_class(utf *u)
{  
	classinfo *c = class_new (u);
	/* prevent loader from loading the array class */
	list_remove (&unloadedclasses, c);
	/* add to unlinked classes */
	list_addlast (&unlinkedclasses, c);
	c -> super = class_java_lang_Object;
	class_link(c);

	return c;
}

/********************** Funktion: loader_init **********************************

	Initialisiert alle Listen und l"adt alle Klassen, die vom System
	und vom Compiler direkt ben"otigt werden.

*******************************************************************************/
 
void loader_init ()
{
	utf *string_class;
	interfaceindex = 0;
	
	list_init (&unloadedclasses, OFFSET(classinfo, listnode) );
	list_init (&unlinkedclasses, OFFSET(classinfo, listnode) );
	list_init (&linkedclasses, OFFSET(classinfo, listnode) );

	/* create utf-symbols for pointer comparison of frequently used strings */
	utf_innerclasses    = utf_new_char("InnerClasses");
	utf_constantvalue   = utf_new_char("ConstantValue");
	utf_code	        = utf_new_char("Code");
	utf_finalize	    = utf_new_char("finalize");
	utf_fidesc	        = utf_new_char("()V");
	utf_clinit	        = utf_new_char("<clinit>");
	utf_initsystemclass = utf_new_char("initializeSystemClass");
	utf_systemclass     = utf_new_char("java/lang/System");

	/* create class for arrays */
	class_array = class_new ( utf_new_char ("The_Array_Class") );
 	list_remove (&unloadedclasses, class_array);

	/* create class for strings, load it after class Object was loaded */
	string_class = utf_new_char ("java/lang/String");
	class_java_lang_String = class_new(string_class);
 	list_remove (&unloadedclasses, class_java_lang_String);

	class_java_lang_Object = 
		loader_load ( utf_new_char ("java/lang/Object") );

	list_addlast(&unloadedclasses, class_java_lang_String);

	class_java_lang_String = 
		loader_load ( string_class );
	class_java_lang_ClassCastException = 
		loader_load ( utf_new_char ("java/lang/ClassCastException") );
	class_java_lang_NullPointerException = 
		loader_load ( utf_new_char ("java/lang/NullPointerException") );
	class_java_lang_ArrayIndexOutOfBoundsException = loader_load ( 
	     utf_new_char ("java/lang/ArrayIndexOutOfBoundsException") );
	class_java_lang_NegativeArraySizeException = loader_load ( 
	     utf_new_char ("java/lang/NegativeArraySizeException") );
	class_java_lang_OutOfMemoryError = loader_load ( 
	     utf_new_char ("java/lang/OutOfMemoryError") );
	class_java_lang_ArrayStoreException =
		loader_load ( utf_new_char ("java/lang/ArrayStoreException") );
	class_java_lang_ArithmeticException = 
		loader_load ( utf_new_char ("java/lang/ArithmeticException") );
	class_java_lang_ThreadDeath =                             /* schani */
	        loader_load ( utf_new_char ("java/lang/ThreadDeath") );

	/* link class for arrays */
	list_addlast (&unlinkedclasses, class_array);
	class_array -> super = class_java_lang_Object;
	class_link (class_array);

	/* correct vftbl-entries (retarded loading of class java/lang/String) */
	stringtable_update(); 

	/* create classes representing primitive types */
	create_primitive_classes();
		
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

	loader_inited = 1;
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

static s4 classvalue;

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
	utf_display(c->name);
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
		if (!(c->flags & ACC_INTERFACE)) {
			c->nextsub = 0;
			c->sub = 0;
			}
		c = list_next (&linkedclasses, c);
		}

	c = list_first (&linkedclasses);
	while (c) {
		if (!(c->flags & ACC_INTERFACE) && (c->super != NULL)) {
			c->nextsub = c->super->sub;
			c->super->sub = c;
			}
		c = list_next (&linkedclasses, c);
		}

	classvalue = 0;
	loader_compute_class_values(class_java_lang_Object);

	intsRestore();                      /* schani */
}



/******************** function classloader_buffer ******************************
 
    sets buffer for reading classdata

*******************************************************************************/

void classload_buffer(u1 *buf, int len)
{
	classbuffer        =  buf;
	classbuffer_size   =  len;
	classbuf_pos       =  buf - 1;
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

