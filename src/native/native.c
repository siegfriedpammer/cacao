/****************************** native.c ***************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

    Contains the tables for native methods.
	The .hh files created with the header file generator are all included here
	as are the C functions implementing these methods.
	
	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at	
	         Roman Obermaisser   EMAIL: cacao@complang.tuwien.ac.at	
	         Andreas Krall       EMAIL: cacao@complang.tuwien.ac.at	

	Last Change: 1996/11/14

*******************************************************************************/

#include <unistd.h>
#include <time.h>
#include "global.h"
#include "native.h"
#include "nativetypes.hh"
#include "builtin.h"
#include "asmpart.h"
#include "tables.h"
#include "loader.h"
#include <math.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <utime.h>

#include "threads/thread.h"                       /* schani */
#include "threads/locks.h"

/* Include files for IO functions */

#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#ifdef _OSF_SOURCE 
#include <sys/mode.h>
#endif
#include <sys/stat.h>

#include "../threads/threadio.h"                    

/* searchpath for classfiles */
static char *classpath;

/* for java-string to char conversion */
#define MAXSTRINGSIZE 1000                          

/******************** systemclasses required for native methods ***************/

static classinfo *class_java_lang_Class;
static classinfo *class_java_lang_Cloneable;
static classinfo *class_java_lang_CloneNotSupportedException;
static classinfo *class_java_lang_System;
static classinfo *class_java_lang_ClassLoader;
static classinfo *class_java_lang_ClassNotFoundException;
static classinfo *class_java_lang_InstantiationException;
static classinfo *class_java_lang_NoSuchMethodError;   
static classinfo *class_java_lang_NoSuchFieldError;
static classinfo *class_java_lang_ClassFormatError;
static classinfo *class_java_lang_IllegalArgumentException;
static classinfo *class_java_lang_ArrayIndexOutOfBoundsException;
static classinfo *class_java_lang_NoSuchFieldException;
static classinfo *class_java_io_SyncFailedException;
static classinfo *class_java_io_IOException;
static classinfo *class_java_io_UnixFileSystem;
static classinfo *class_java_security_PrivilegedActionException;
static classinfo *class_java_net_UnknownHostException;
static classinfo *class_java_net_SocketException;
static classinfo *class_java_lang_NoSuchMethodException;
static classinfo *class_java_lang_Double;
static classinfo *class_java_lang_Float;
static classinfo *class_java_lang_Long;
static classinfo *class_java_lang_Byte;
static classinfo *class_java_lang_Short;
static classinfo *class_java_lang_Boolean;
static classinfo *class_java_lang_Void;
static classinfo *class_java_lang_Character;
static classinfo *class_java_lang_Integer;

/* the system classloader object */
struct java_lang_ClassLoader *SystemClassLoader = NULL;

/* for raising exceptions from native methods */
java_objectheader* exceptionptr = NULL;

/************* use classinfo structure as java.lang.Class object **************/

static void use_class_as_object (classinfo *c) 
{
	vftbl *vt = class_java_lang_Class -> vftbl;
	vftbl *newtbl;
	copy_vftbl(&newtbl, vt);
	newtbl->baseval = c->header.vftbl->baseval;
	newtbl->diffval = c->header.vftbl->diffval;
	c->header.vftbl = newtbl;
}

/*********************** include Java Native Interface ************************/ 

#include "jni.c"

/*************************** include native methods ***************************/ 

#include "nat/Object.c"
#include "nat/String.c"
#include "nat/ClassLoader.c"
#include "nat/Class.c"
#include "nat/Compiler.c"
#include "nat/Double.c"
#include "nat/Float.c"
#include "nat/Math.c"
#include "nat/Package.c"
#include "nat/Runtime.c"
#include "nat/SecurityManager.c"
#include "nat/System.c"
#include "nat/Thread.c"
#include "nat/Throwable.c"
#include "nat/Finalizer.c"
#include "nat/Array.c"
#include "nat/Constructor.c"
#include "nat/Field.c"
#include "nat/Method.c"
#include "nat/FileDescriptor.c"
#include "nat/FileInputStream.c"
#include "nat/FileOutputStream.c"
#include "nat/FileSystem.c"
#include "nat/ObjectInputStream.c"
#include "nat/ObjectStreamClass.c"
#include "nat/RandomAccessFile.c"
#include "nat/ResourceBundle.c"
#include "nat/JarFile.c"
#include "nat/Adler32.c"
#include "nat/CRC32.c"
#include "nat/Deflater.c"
#include "nat/Inflater.c"
#include "nat/ZipEntry.c"
#include "nat/ZipFile.c"
#include "nat/BigInteger.c"
#include "nat/InetAddress.c"
#include "nat/InetAddressImpl.c"
#include "nat/DatagramPacket.c"
#include "nat/PlainDatagramSocketImpl.c"
#include "nat/PlainSocketImpl.c"
#include "nat/SocketInputStream.c"
#include "nat/SocketOutputStream.c"
#include "nat/AccessController.c"
#include "nat/ClassLoader_NativeLibrary.c"
#include "nat/UnixFileSystem.c"

/************************** tables for methods ********************************/

/* table for locating native methods */
static struct nativeref {
	char *classname;
	char *methodname;
	char *descriptor;
	bool isstatic;
	functionptr func;
} nativetable [] = {

#include "nativetable.hh"

};


#define NATIVETABLESIZE  (sizeof(nativetable)/sizeof(struct nativeref))

/* table for fast string comparison */
static struct nativecompref {
	utf *classname;
	utf *methodname;
	utf *descriptor;
	bool isstatic;
	functionptr func;
	} nativecomptable [NATIVETABLESIZE];

/* string comparsion table initialized */
static bool nativecompdone = false;


/*********************** function: native_loadclasses **************************

	load classes required for native methods	

*******************************************************************************/

void native_loadclasses()
{
	/* class_new adds the class to the list of classes to be loaded */
	class_java_lang_Cloneable = 
		class_new ( utf_new_char ("java/lang/Cloneable") );
	class_java_lang_CloneNotSupportedException = 
		class_new ( utf_new_char ("java/lang/CloneNotSupportedException") );
	class_java_lang_Class =
		class_new ( utf_new_char ("java/lang/Class") );
	class_java_io_IOException = 
		class_new ( utf_new_char ("java/io/IOException") );
	class_java_lang_ClassNotFoundException =
		class_new ( utf_new_char ("java/lang/ClassNotFoundException") );
	class_java_lang_InstantiationException =
		class_new ( utf_new_char ("java/lang/InstantiationException") );
	class_java_lang_NoSuchMethodError =
		class_new ( utf_new_char ("java/lang/NoSuchMethodError") );
	class_java_lang_NoSuchFieldError =
		class_new ( utf_new_char ("java/lang/NoSuchFieldError") );	
	class_java_lang_ClassFormatError =
		class_new ( utf_new_char ("java/lang/ClassFormatError") );	
	class_java_io_SyncFailedException =
	        class_new ( utf_new_char ("java/io/SyncFailedException") );
	class_java_io_UnixFileSystem =
	        class_new ( utf_new_char ("java/io/UnixFileSystem") );
	class_java_lang_System =
	        class_new ( utf_new_char ("java/lang/System") );
	class_java_lang_ClassLoader =
	        class_new ( utf_new_char ("java/lang/ClassLoader") );	
	class_java_security_PrivilegedActionException =
	        class_new( utf_new_char("java/security/PrivilegedActionException"));
 	class_java_net_UnknownHostException = 
 	        loader_load( utf_new_char ("java/net/UnknownHostException") );
 	class_java_net_SocketException = 
 	        loader_load( utf_new_char ("java/net/SocketException") );
	class_java_lang_IllegalArgumentException =
	        class_new( utf_new_char("java/lang/IllegalArgumentException"));
	class_java_lang_ArrayIndexOutOfBoundsException =
	        class_new( utf_new_char ("java/lang/ArrayIndexOutOfBoundsException") );
	class_java_lang_NoSuchFieldException =
	        class_new( utf_new_char ("java/lang/NoSuchFieldException") );	    
	class_java_lang_NoSuchMethodException = 
	        class_new( utf_new_char ("java/lang/NoSuchMethodException") );	    

	/* load classes for wrapping primitive types */
	class_java_lang_Double =
		class_new( utf_new_char ("java/lang/Double") );
	class_java_lang_Float =
		class_new( utf_new_char ("java/lang/Float") );
	class_java_lang_Character =
	        class_new( utf_new_char ("java/lang/Character") );
	class_java_lang_Integer =
	        class_new( utf_new_char ("java/lang/Integer") );
	class_java_lang_Long =
	        class_new( utf_new_char ("java/lang/Long") );
	class_java_lang_Byte =
	        class_new( utf_new_char ("java/lang/Byte") );
	class_java_lang_Short =
	        class_new( utf_new_char ("java/lang/Short") );
	class_java_lang_Boolean =
	        class_new( utf_new_char ("java/lang/Boolean") );
	class_java_lang_Void =
	        class_new( utf_new_char ("java/lang/Void") );

	/* load to avoid dynamic classloading */
	class_new(utf_new_char("sun/net/www/protocol/file/Handler"));
	class_new(utf_new_char("sun/net/www/protocol/jar/Handler"));	
	class_new(utf_new_char("sun/io/CharToByteISO8859_1"));
	
	/* start classloader */
	loader_load(utf_new_char("sun/io/ByteToCharISO8859_1")); 
}


/*************** adds a class to the vector of loaded classes ****************/

void systemclassloader_addclass(classinfo *c)
{
        methodinfo *m;

	/* find method addClass of java.lang.ClassLoader */
	m = class_resolvemethod (
		class_java_lang_ClassLoader, 
		utf_new_char("addClass"),
		utf_new_char("(Ljava/lang/Class;)")
    	    );

	if (!m) panic("warning: cannot initialize classloader");

	/* prepare class to be passed as argument */
      	use_class_as_object (c);

	/* call 'addClass' */
	asm_calljavamethod(m,
                           (java_objectheader*) SystemClassLoader, 
    	                   (java_objectheader*) c,
    	                   NULL,  
    	                   NULL
			  );       
}

/*************** adds a library to the vector of loaded libraries *************/

void systemclassloader_addlibrary(java_objectheader *o)
{
        methodinfo *m;

	/* find method addElement of java.util.Vector */
	m = class_resolvemethod (
	        loader_load ( utf_new_char ("java/util/Vector") ),
		utf_new_char("addElement"),
		utf_new_char("(Ljava/lang/Object;)V")
    	    );

	if (!m) panic("cannot initialize classloader");

	/* call 'addElement' */
  	asm_calljavamethod(m,
			   SystemClassLoader->nativeLibraries,
			   o,
    	                   NULL,  
    	                   NULL
			  );       
}

/*****************************************************************************

	create systemclassloader object and initialize instance fields  

******************************************************************************/

void init_systemclassloader() 
{
  if (!SystemClassLoader) {

	/* create object and call initializer */
	SystemClassLoader = (java_lang_ClassLoader*) native_new_and_init(class_java_lang_ClassLoader);	
	heap_addreference((void**) &SystemClassLoader);

	/* systemclassloader has no parent */
	SystemClassLoader->parent      = NULL;
	SystemClassLoader->initialized = true;
  }
}


/********************* add loaded library name  *******************************/

void systemclassloader_addlibname(java_objectheader *o)
{
        methodinfo *m;
	java_objectheader *LibraryNameVector;
	jfieldID id;

	m = class_resolvemethod (
	        loader_load ( utf_new_char ("java/util/Vector") ),
		utf_new_char("addElement"),
		utf_new_char("(Ljava/lang/Object;)V")
    	    );

	if (!m) panic("cannot initialize classloader");

	id = env.GetStaticFieldID(&env,class_java_lang_ClassLoader,"loadedLibraryNames","Ljava/util/Vector;");
	if (!id) panic("can not access ClassLoader");

  	asm_calljavamethod(m,
    	                   GetStaticObjectField(&env,class_java_lang_ClassLoader,id),
			   o,
    	                   NULL,  
    	                   NULL
			  );       
}


/********************* function: native_setclasspath **************************/
 
void native_setclasspath (char *path)
{
	/* set searchpath for classfiles */
	classpath = path;
}

/***************** function: throw_classnotfoundexception *********************/

void throw_classnotfoundexception()
{
	/* throws a ClassNotFoundException */
	exceptionptr = native_new_and_init (class_java_lang_ClassNotFoundException);
}


/*********************** Function: native_findfunction *************************

	Looks up a method (must have the same class name, method name, descriptor
	and 'static'ness) and returns a function pointer to it.
	Returns: function pointer or NULL (if there is no such method)

	Remark: For faster operation, the names/descriptors are converted from C
		strings to Unicode the first time this function is called.

*******************************************************************************/

functionptr native_findfunction (utf *cname, utf *mname, 
                                 utf *desc, bool isstatic)
{
	int i;
	/* entry of table for fast string comparison */
	struct nativecompref *n;
        /* for warning message if no function is found */
	char *buffer;	         	
	int buffer_len, pos;

	isstatic = isstatic ? true : false;

	if (!nativecompdone) {
		for (i = 0; i < NATIVETABLESIZE; i++) {
			nativecomptable[i].classname  = 
					utf_new_char(nativetable[i].classname);
			nativecomptable[i].methodname = 
					utf_new_char(nativetable[i].methodname);
			nativecomptable[i].descriptor = 
					utf_new_char(nativetable[i].descriptor);
			nativecomptable[i].isstatic   = 
					nativetable[i].isstatic;
			nativecomptable[i].func       = 
					nativetable[i].func;
			}
		nativecompdone = true;
		}

	for (i = 0; i < NATIVETABLESIZE; i++) {
		n = &(nativecomptable[i]);

		if (cname == n->classname && mname == n->methodname &&
		    desc == n->descriptor && isstatic == n->isstatic)
			return n->func;
		}

	/* no function was found, display warning */

	buffer_len = 
	  utf_strlen(cname) + utf_strlen(mname) + utf_strlen(desc) + 64;

	buffer = MNEW(char, buffer_len);

	strcpy(buffer, "warning: native function ");
        utf_sprint(buffer+strlen(buffer), mname);
	strcpy(buffer+strlen(buffer), ": ");
        utf_sprint(buffer+strlen(buffer), desc);
	strcpy(buffer+strlen(buffer), " not found in class ");
        utf_sprint(buffer+strlen(buffer), cname);

	log_text(buffer);	

	MFREE(buffer, char, buffer_len);

	return NULL;
}


/********************** function: javastring_new *******************************

	creates a new object of type java/lang/String with the text of 
	the specified utf8-string

	return: pointer to the string or NULL if memory is exhausted.	

*******************************************************************************/

java_objectheader *javastring_new (utf *u)
{
	char *utf_ptr = u->text;        /* current utf character in utf string    */
	int utflength = utf_strlen(u);  /* length of utf-string if uncompressed   */
	java_lang_String *s;		    /* result-string                          */
	java_chararray *a;
	s4 i;
	
	s = (java_lang_String*) builtin_new (class_java_lang_String);
	a = builtin_newarray_char (utflength);

	/* javastring or character-array could not be created */
	if ((!a) || (!s))
		return NULL;

	/* decompress utf-string */
	for (i = 0; i < utflength; i++)
		a->data[i] = utf_nextu2(&utf_ptr);
	
	/* set fields of the javastring-object */
	s -> value  = a;
	s -> offset = 0;
	s -> count  = utflength;

	return (java_objectheader*) s;
}

/********************** function: javastring_new_char **************************

	creates a new java/lang/String object which contains the convertet
	C-string passed via text.

	return: the object pointer or NULL if memory is exhausted.

*******************************************************************************/

java_objectheader *javastring_new_char (char *text)
{
	s4 i;
	s4 len = strlen(text); /* length of the string */
	java_lang_String *s;   /* result-string */
	java_chararray *a;
	
	s = (java_lang_String*) builtin_new (class_java_lang_String);
	a = builtin_newarray_char (len);

	/* javastring or character-array could not be created */
	if ((!a) || (!s)) return NULL;

	/* copy text */
	for (i = 0; i < len; i++)
		a->data[i] = text[i];
	
	/* set fields of the javastring-object */
	s -> value = a;
	s -> offset = 0;
	s -> count = len;

	return (java_objectheader*) s;
}


/************************* function javastring_tochar **************************

	converts a Java string into a C string.
	
	return: pointer to C string
	
	Caution: every call of this function overwrites the previous string !!!
	
*******************************************************************************/

static char stringbuffer[MAXSTRINGSIZE];

char *javastring_tochar (java_objectheader *so) 
{
	java_lang_String *s = (java_lang_String*) so;
	java_chararray *a;
	s4 i;
	
	if (!s)
		return "";
	a = s->value;
	if (!a)
		return "";
	if (s->count > MAXSTRINGSIZE)
		return "";
	for (i = 0; i < s->count; i++)
		stringbuffer[i] = a->data[s->offset+i];
	stringbuffer[i] = '\0';
	return stringbuffer;
}


/****************** function class_findfield_approx ****************************
	
	searches in 'classinfo'-structure for a field with the
	specified name

*******************************************************************************/
 
fieldinfo *class_findfield_approx (classinfo *c, utf *name)
{
	s4 i;
	for (i = 0; i < c->fieldscount; i++) {
		/* compare field names */
		if ((c->fields[i].name == name))
			return &(c->fields[i]);
		}

	/* field was not found, raise exception */	
	exceptionptr = native_new_and_init(class_java_lang_NoSuchFieldException);
	return NULL;
}


/********************** function: native_new_and_init *************************

	Creates a new object on the heap and calls the initializer.
	Returns the object pointer or NULL if memory is exhausted.
			
*******************************************************************************/

java_objectheader *native_new_and_init (classinfo *c)
{
	methodinfo *m;
	java_objectheader *o = builtin_new (c);         /*          create object */

	if (!o) return NULL;
	
	/* find initializer */

	m = class_findmethod(c, utf_new_char("<init>"), utf_new_char("()V"));
	                      	                      
	if (!m) {                                       /* initializer not found  */
		if (verbose) {
			sprintf(logtext, "Warning: class has no instance-initializer: ");
			utf_sprint(logtext + strlen(logtext), c->name);
			dolog();
			}
		return o;
		}

	/* call initializer */

	asm_calljavamethod (m, o, NULL, NULL, NULL);
	return o;
}

/******************** function: stringtable_update ****************************

	traverses the javastring hashtable and sets the vftbl-entries of
	javastrings which were temporarily set to NULL, because	
	java.lang.Object was not yet loaded

*******************************************************************************/
 
void stringtable_update ()
{
	java_lang_String *js;   
	java_chararray *a;
	literalstring *s;	/* hashtable entry */
	int i;

	for (i = 0; i < string_hash.size; i++) {
		s = string_hash.ptr[i];
		if (s) {
			while (s) {
								
				js = (java_lang_String *) s->string;
				
				if (!js || !(a = js->value)) 
					/* error in hashtable found */
					panic("invalid literalstring in hashtable");

				if (!js->header.vftbl) 
					/* vftbl of javastring is NULL */ 
					js->header.vftbl = class_java_lang_String -> vftbl;

				if (!a->header.objheader.vftbl) 
					/* vftbl of character-array is NULL */ 
					a->header.objheader.vftbl = class_array -> vftbl;

				/* follow link in external hash chain */
				s = s->hashlink;
			}	
		}		
	}
}


/************************* function: u2_utflength ***************************

	returns the utf length in bytes of a u2 array 

*****************************************************************************/


u4 u2_utflength(u2 *text, u4 u2_length)
{
	u4 result_len =  0;  /* utf length in bytes  */
	u2 ch;               /* current unicode character */
	u4 len;
	
        for (len = 0; len < u2_length; len++) {
	  
	  /* next unicode character */
	  ch = *text++;
	  
	  /* determine bytes required to store unicode character as utf */
	  if (ch && (ch < 0x80)) 
	    result_len++;
	  else if (ch < 0x800)
	    result_len += 2;	
	  else 
	    result_len += 3;	
	}

    return result_len;
}

/********************* function: utf_new_u2 ***********************************

	make utf symbol from u2 array, 
	if isclassname is true '.' is replaced by '/'

*******************************************************************************/

utf *utf_new_u2(u2 *unicode_pos, u4 unicode_length, bool isclassname)
{
	char *buffer; /* memory buffer for  unicode characters */
    	char *pos;    /* pointer to current position in buffer */
    	u4 left;      /* unicode characters left */
    	u4 buflength; /* utf length in bytes of the u2 array  */
	utf *result;  /* resulting utf-string */
    	int i;    	

	/* determine utf length in bytes and allocate memory */    	
	buflength = u2_utflength(unicode_pos, unicode_length); 
    	buffer    = MNEW(char,buflength);
 
 	/* memory allocation failed */
	if (!buffer) return NULL;

    	left = buflength;
	pos  = buffer;

    	for (i = 0; i++ < unicode_length; unicode_pos++) {
		/* next unicode character */
		u2 c = *unicode_pos;
		
		if ((c != 0) && (c < 0x80)) {
		/* 1 character */	
		left--;
	    	if ((int) left < 0) break;
		/* convert classname */
		if (isclassname && c=='.') 
		  *pos++ = '/';
		else
		  *pos++ = (char) c;
		} else if (c < 0x800) { 	    
		/* 2 characters */				
	    	unsigned char high = c >> 6;
	    	unsigned char low  = c & 0x3F;
		left = left - 2;
	    	if ((int) left < 0) break;
	    	*pos++ = high | 0xC0; 
	    	*pos++ = low  | 0x80;	  
		} else {	 
	    	/* 3 characters */				
	    	char low  = c & 0x3f;
	    	char mid  = (c >> 6) & 0x3F;
	    	char high = c >> 12;
		left = left - 3;
	    	if ((int) left < 0) break;
	    	*pos++ = high | 0xE0; 
	    	*pos++ = mid  | 0x80;  
	    	*pos++ = low  | 0x80;   
		}
    	}
	
	/* insert utf-string into symbol-table */
	result = utf_new(buffer,buflength);
    	MFREE(buffer, char, buflength);
	return result;
}

/********************* function: javastring_toutf *****************************

	make utf symbol from javastring

*******************************************************************************/

utf *javastring_toutf(java_lang_String *string, bool isclassname)
{
        java_lang_String *str = (java_lang_String *) string;
	return utf_new_u2(str->value->data,str->count, isclassname);
}

/********************* function: literalstring_u2 *****************************

    searches for the javastring with the specified u2-array in 
    the string hashtable, if there is no such string a new one is 
    created 

    if copymode is true a copy of the u2-array is made

*******************************************************************************/

java_objectheader *literalstring_u2 (java_chararray *a, u4 length, bool copymode )
{
    literalstring *s;                /* hashtable element */
    java_lang_String *js;            /* u2-array wrapped in javastring */
    java_chararray *stringdata;      /* copy of u2-array */      
    u4 key;   
    u4 slot;  
    u2 i;

    /* find location in hashtable */
    key  = unicode_hashkey (a->data, length);
    slot = key & (string_hash.size-1);
    s    = string_hash.ptr[slot];

    while (s) {
	
      js = (java_lang_String *) s->string;
	
      if (js->count == length) {
      	/* compare text */
	for (i=0; i<length; i++) 
	  if (js->value->data[i] != a->data[i]) goto nomatch;
					
	/* string already in hashtable, free memory */
	if (!copymode)
	  lit_mem_free(a, sizeof(java_chararray) + sizeof(u2)*(length-1)+10);

	return (java_objectheader *) js;
      }

      nomatch:
      /* follow link in external hash chain */
      s = s->hashlink;
    }

    if (copymode) {
      /* create copy of u2-array for new javastring */
      u4 arraysize = sizeof(java_chararray) + sizeof(u2)*(length-1)+10;
      stringdata = lit_mem_alloc ( arraysize );	
      memcpy(stringdata, a, arraysize );	
    }  
    else
      stringdata = a;

    /* location in hashtable found, complete arrayheader */
    if (class_array==NULL) panic("class_array not initialized");
    stringdata -> header.objheader.vftbl = class_array -> vftbl;
    stringdata -> header.size = length;	
    stringdata -> header.arraytype = ARRAYTYPE_CHAR;	

    /* create new javastring */
    js = LNEW (java_lang_String);
    js -> header.vftbl = class_java_lang_String -> vftbl;
    js -> value  = stringdata;
    js -> offset = 0;
    js -> count  = length;

    /* create new literalstring */
    s = NEW (literalstring);
    s->hashlink = string_hash.ptr[slot];
    s->string   = (java_objectheader *) js;
    string_hash.ptr[slot] = s;

    /* update numbe of hashtable entries */
    string_hash.entries++;

    /* reorganization of hashtable */       
    if ( string_hash.entries > (string_hash.size*2)) {

      /* reorganization of hashtable, average length of 
         the external chains is approx. 2                */  

      u4 i;
      literalstring *s;     
      hashtable newhash; /* the new hashtable */
      
      /* create new hashtable, double the size */
      init_hashtable(&newhash, string_hash.size*2);
      newhash.entries=string_hash.entries;
      
      /* transfer elements to new hashtable */
      for (i=0; i<string_hash.size; i++) {
	s = string_hash.ptr[i];
	while (s) {
	  literalstring *nexts = s -> hashlink;	 
	  js   = (java_lang_String*) s->string;
	  slot = (unicode_hashkey(js->value->data,js->count)) & (newhash.size-1);
	  
	  s->hashlink = newhash.ptr[slot];
	  newhash.ptr[slot] = s;
	
	  /* follow link in external hash chain */  
	  s = nexts;
	}
      }
	
      /* dispose old table */	
      MFREE (string_hash.ptr, void*, string_hash.size);
      string_hash = newhash;
    }
			
    return (java_objectheader *) js;
}

/******************** Function: literalstring_new *****************************

    creates a new javastring with the text of the utf-symbol
    and inserts it into the string hashtable

*******************************************************************************/

java_objectheader *literalstring_new (utf *u)
{
    char *utf_ptr = u->text;         /* pointer to current unicode character in utf string */
    u4 utflength  = utf_strlen(u);   /* length of utf-string if uncompressed */
    java_chararray *a;               /* u2-array constructed from utf string */
    java_objectheader *js;
    u4 i;
    
    /* allocate memory */ 
    a = lit_mem_alloc (sizeof(java_chararray) + sizeof(u2)*(utflength-1)+10 );	
    /* convert utf-string to u2-array */
    for (i=0; i<utflength; i++) a->data[i] = utf_nextu2(&utf_ptr);	

    return literalstring_u2(a, utflength, false);
}


/********************** function: literalstring_free **************************

        removes a javastring from memory		       

******************************************************************************/

void literalstring_free (java_objectheader* sobj)
{
	java_lang_String *s = (java_lang_String*) sobj;
	java_chararray *a = s->value;

	log_text("literalstring_free called");
	
	/* dispose memory of java.lang.String object */
	LFREE (s, java_lang_String);
	/* dispose memory of java-characterarray */
	LFREE (a, sizeof(java_chararray) + sizeof(u2)*(a->header.size-1)); /* +10 ?? */
}




void copy_vftbl(vftbl **dest, vftbl *src)
{
	*dest = heap_allocate(sizeof(vftbl) + sizeof(methodptr)*(src->vftbllength-1), false, NULL);
	memcpy(*dest, src, sizeof(vftbl) - sizeof(methodptr));
	memcpy(&(*dest)->table, &src->table, src->vftbllength * sizeof(methodptr));
}
