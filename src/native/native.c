/* native.c - table of native functions

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
            Roman Obermaisser
            Andreas Krall

   The .hh files created with the header file generator are all
   included here as are the C functions implementing these methods.

   $Id: native.c 1190 2004-06-19 12:44:12Z twisti $

*/


#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <utime.h>

#include "config.h"
#include "global.h"
#include "main.h"
#include "jni.h"
#include "native.h"
#include "nativetypes.hh"
#include "builtin.h"
#include "asmpart.h"
#include "tables.h"
#include "loader.h"
#include "jni.h"
#include "jit/jit.h"
#include "toolbox/logging.h"
#include "toolbox/memory.h"
#include "threads/thread.h"
#include "threads/threadio.h"
#include "threads/locks.h"
#include "nat/java_lang_VMClass.h"
#include "nat/java_lang_Throwable.h"

/* Include files for IO functions */

#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#ifdef _OSF_SOURCE 
#include <sys/mode.h>
#endif
#include <sys/stat.h>

#include "threads/threadio.h"

/* searchpath for classfiles */
char *classpath;

/* for java-string to char conversion */
#define MAXSTRINGSIZE 1000                          


/******************** systemclasses required for native methods ***************/

classinfo *class_java_lang_Class;
classinfo *class_java_lang_VMClass;
classinfo *class_java_lang_System;
classinfo *class_java_lang_ClassLoader;
classinfo *class_gnu_java_lang_SystemClassLoader;
classinfo *class_java_lang_SecurityManager;
classinfo *class_java_lang_Double;
classinfo *class_java_lang_Float;
classinfo *class_java_lang_Long;
classinfo *class_java_lang_Byte;
classinfo *class_java_lang_Short;
classinfo *class_java_lang_Boolean;
classinfo *class_java_lang_Void;
classinfo *class_java_lang_Character;
classinfo *class_java_lang_Integer;

methodinfo *method_vmclass_init;


/* system exception classes required while compiling */

classinfo *class_java_lang_Throwable;


/* exception/error super class */

char *string_java_lang_Throwable =
    "java/lang/Throwable";

char *string_java_lang_VMThrowable =
    "java/lang/VMThrowable";


/* specify some exception strings for code generation */

char *string_java_lang_ArithmeticException =
    "java/lang/ArithmeticException";

char *string_java_lang_ArithmeticException_message =
    "/ by zero";

char *string_java_lang_ArrayIndexOutOfBoundsException =
    "java/lang/ArrayIndexOutOfBoundsException";

char *string_java_lang_ArrayStoreException =
    "java/lang/ArrayStoreException";

char *string_java_lang_ClassCastException =
    "java/lang/ClassCastException";

char *string_java_lang_ClassNotFoundException =
	"java/lang/ClassNotFoundException";

char *string_java_lang_CloneNotSupportedException =
    "java/lang/CloneNotSupportedException";

char *string_java_lang_Exception =
    "java/lang/Exception";

char *string_java_lang_IllegalArgumentException =
    "java/lang/IllegalArgumentException";

char *string_java_lang_IllegalMonitorStateException =
    "java/lang/IllegalMonitorStateException";

char *string_java_lang_NegativeArraySizeException =
    "java/lang/NegativeArraySizeException";

char *string_java_lang_NoSuchFieldException =
	"java/lang/NoSuchFieldException";

char *string_java_lang_NoSuchMethodException =
	"java/lang/NoSuchMethodException";

char *string_java_lang_NullPointerException =
    "java/lang/NullPointerException";


/* specify some error strings for code generation */

char *string_java_lang_AbstractMethodError =
    "java/lang/AbstractMethodError";

char *string_java_lang_ClassCircularityError =
    "java/lang/ClassCircularityError";

char *string_java_lang_ClassFormatError =
    "java/lang/ClassFormatError";

char *string_java_lang_Error =
    "java/lang/Error";

char *string_java_lang_ExceptionInInitializerError =
    "java/lang/ExceptionInInitializerError";

char *string_java_lang_IncompatibleClassChangeError =
    "java/lang/IncompatibleClassChangeError";

char *string_java_lang_InternalError =
    "java/lang/InternalError";

char *string_java_lang_LinkageError =
    "java/lang/LinkageError";

char *string_java_lang_NoClassDefFoundError =
    "java/lang/NoClassDefFoundError";

char *string_java_lang_NoSuchFieldError =
	"java/lang/NoSuchFieldError";

char *string_java_lang_NoSuchMethodError =
	"java/lang/NoSuchMethodError";

char *string_java_lang_OutOfMemoryError =
    "java/lang/OutOfMemoryError";

char *string_java_lang_VerifyError =
    "java/lang/VerifyError";

char *string_java_lang_VirtualMachineError =
    "java/lang/VirtualMachineError";


/* the system classloader object */
struct java_lang_ClassLoader *SystemClassLoader = NULL;

/* for raising exceptions from native methods */
#if !defined(USE_THREADS) || !defined(NATIVE_THREADS)
java_objectheader* _exceptionptr = NULL;
#endif

/************* use classinfo structure as java.lang.Class object **************/

void use_class_as_object(classinfo *c) 
{
	if (!c->classvftbl) {
		c->classvftbl = true;

		c->header.vftbl = class_java_lang_Class->vftbl;
  	}
	     
}


/*************************** include native methods ***************************/ 

#ifdef USE_GTK 
#include "nat/GdkGraphics.c"
#include "nat/GtkComponentPeer.c"
#include "nat/GdkPixbufDecoder.c"
#include "nat/GtkScrollPanePeer.c"
#include "nat/GtkFileDialogPeer.c"
#include "nat/GtkLabelPeer.c"
#endif


/************************** tables for methods ********************************/

#undef JOWENN_DEBUG
#undef JOWENN_DEBUG1

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


/******************************************************************************/
/******************************************************************************/
#include "natcalls.h"

/* string call comparison table initialized */

/******************************************************************************/
/******************************************************************************/

/* init_system_exceptions *****************************************************

   load, link and compile exceptions used in the system

*******************************************************************************/

void init_system_exceptions()
{
	classinfo *c;

	/* java/lang/Throwable */

	class_java_lang_Throwable =
		class_new(utf_new_char(string_java_lang_Throwable));
	class_load(class_java_lang_Throwable);
	class_link(class_java_lang_Throwable);
#if 0
	compile_all_class_methods(class_java_lang_Throwable);

	/* java/lang/VMThrowable */

	c = class_new(utf_new_char(string_java_lang_VMThrowable));
	class_load(c);
	class_link(c);
	compile_all_class_methods(c);

	/* java/lang/ClassFormatError */

	c = class_new(utf_new_char(string_java_lang_ClassFormatError));
	class_load(c);
	class_link(c);
	compile_all_class_methods(c);

	/* java/lang/Error */

	c = class_new(utf_new_char(string_java_lang_Error));
	class_load(c);
	class_link(c);
	compile_all_class_methods(c);

	/* java/lang/Exception */

	c = class_new(utf_new_char(string_java_lang_Exception));
	class_load(c);
	class_link(c);
	compile_all_class_methods(c);

	/* java/lang/IncompatibleClassChangeError */

	c = class_new(utf_new_char(string_java_lang_IncompatibleClassChangeError));
	class_load(c);
	class_link(c);
	compile_all_class_methods(c);

	/* java/lang/LinkageError */

	c = class_new(utf_new_char(string_java_lang_LinkageError));
	class_load(c);
	class_link(c);
	compile_all_class_methods(c);

	/* java/lang/NoClassDefFoundError */

	c = class_new(utf_new_char(string_java_lang_NoClassDefFoundError));
	class_load(c);
	class_link(c);
	compile_all_class_methods(c);

	/* java/lang/NoSuchFieldError */

	c = class_new(utf_new_char(string_java_lang_NoSuchFieldError));
	class_load(c);
	class_link(c);
	compile_all_class_methods(c);

	/* java/lang/NoSuchMethodError */

	c = class_new(utf_new_char(string_java_lang_NoSuchMethodError));
	class_load(c);
	class_link(c);
	compile_all_class_methods(c);

	/* java/lang/OutOfMemoryError */

	c = class_new(utf_new_char(string_java_lang_OutOfMemoryError));
	class_load(c);
	class_link(c);
	compile_all_class_methods(c);

	/* java/lang/VerifyError */

	c = class_new(utf_new_char(string_java_lang_VerifyError));
	class_load(c);
	class_link(c);
	compile_all_class_methods(c);

	/* java/lang/VirtualMachineError */

	c = class_new(utf_new_char(string_java_lang_VirtualMachineError));
	class_load(c);
	class_link(c);
	compile_all_class_methods(c);
#endif
}


void compile_all_class_methods(classinfo *c)
{
	s4 i;

	for (i = 0; i < c->methodscount; i++) {
		(void) jit_compile(&(c->methods[i]));
	}
}


static void throw_exception_exit_intern(bool doexit)
{
	java_objectheader *xptr;
	classinfo *c;
	methodinfo *pss;

	if (*exceptionptr) {
		xptr = *exceptionptr;

		/* clear exception, because we are calling jit code again */
		*exceptionptr = NULL;

		c = xptr->vftbl->class;

		pss = class_resolveclassmethod(c,
									   utf_new_char("printStackTrace"),
									   utf_new_char("()V"),
									   class_java_lang_Object,
									   false);

		/* print the stacktrace */
		if (pss) {
			asm_calljavafunction(pss, xptr, NULL, NULL, NULL);

			/* this normally means, we are EXTREMLY out of memory, but may be
			   any other exception */
			if (*exceptionptr) {
				utf_fprint_classname(stderr, c->name);
				fprintf(stderr, "\n");
			}

		} else {
			utf_fprint_classname(stderr, c->name);
			fprintf(stderr, ": printStackTrace()V not found!\n");
		}

		fflush(stderr);

		/* good bye! */
		if (doexit) {
			exit(1);
		}
	}
}


void throw_exception()
{
	throw_exception_exit_intern(false);
}


void throw_exception_exit()
{
	throw_exception_exit_intern(true);
}


void throw_main_exception()
{
	fprintf(stderr, "Exception in thread \"main\" ");
	fflush(stderr);

	throw_exception_exit_intern(false);
}


void throw_main_exception_exit()
{
	fprintf(stderr, "Exception in thread \"main\" ");
	fflush(stderr);

	throw_exception_exit_intern(true);
}


void throw_cacao_exception_exit(char *exception, char *message)
{
	s4 i;
	char *tmp;
	s4 len;

	len = strlen(exception);
	tmp = MNEW(char, len);
	strncpy(tmp, exception, len);

	/* convert to classname */

   	for (i = len - 1; i >= 0; i--) {
 	 	if (tmp[i] == '/') tmp[i] = '.';
	}

	fprintf(stderr, "Exception in thread \"main\" %s", tmp);

	MFREE(tmp, char, len);

	if (strlen(message) > 0)
		fprintf(stderr, ": %s", message);

	fprintf(stderr, "\n");
	fflush(stderr);

	/* good bye! */
	exit(1);
}


#define CREATENEW_EXCEPTION(ex) \
	java_objectheader *newEx; \
	java_objectheader *oldexception=*exceptionptr;\
	*exceptionptr=0;\
	newEx=ex;\
	*exceptionptr=oldexception;\
	return newEx;

java_objectheader *new_exception(char *classname)
{
	classinfo *c = class_new(utf_new_char(classname));

	CREATENEW_EXCEPTION(native_new_and_init(c));
}

java_objectheader *new_exception_message(char *classname, char *message)
{
	classinfo *c = class_new(utf_new_char(classname));

	CREATENEW_EXCEPTION(native_new_and_init_string(c, javastring_new_char(message)));
}


java_objectheader *new_exception_throwable(char *classname, java_lang_Throwable *throwable)
{
	classinfo *c = class_new(utf_new_char(classname));

	CREATENEW_EXCEPTION(native_new_and_init_throwable(c, throwable));
}


java_objectheader *new_exception_utfmessage(char *classname, utf *message)
{
	classinfo *c = class_new(utf_new_char(classname));

	CREATENEW_EXCEPTION(native_new_and_init_string(c, javastring_new(message)));
}


java_objectheader *new_exception_javastring(char *classname, java_lang_String *message)
{
	classinfo *c = class_new(utf_new_char(classname));

	CREATENEW_EXCEPTION(native_new_and_init_string(c, message));
}


java_objectheader *new_exception_int(char *classname, s4 i)
{
	classinfo *c = class_new(utf_new_char(classname));

	CREATENEW_EXCEPTION(native_new_and_init_int(c, i));
}


/*********************** function: native_loadclasses **************************

	load classes required for native methods	

*******************************************************************************/

void native_loadclasses()
{
	static int classesLoaded = 0; /*temporary hack JoWenn*/

	if (classesLoaded)
		return;

	classesLoaded = 1;

	class_java_lang_Cloneable =
		class_new(utf_new_char("java/lang/Cloneable"));
	class_load(class_java_lang_Cloneable);
	class_link(class_java_lang_Cloneable);

	class_java_lang_Class =
		class_new(utf_new_char("java/lang/Class"));
	class_load(class_java_lang_Class);
	class_link(class_java_lang_Class);

	class_java_lang_VMClass =
		class_new(utf_new_char("java/lang/VMClass"));
	class_load(class_java_lang_VMClass);
	class_link(class_java_lang_VMClass);

	class_java_lang_ClassLoader =
		class_new(utf_new_char("java/lang/ClassLoader"));
	class_load(class_java_lang_ClassLoader);
	class_link(class_java_lang_ClassLoader);

	/* load classes for wrapping primitive types */
	class_java_lang_Double = class_new(utf_new_char("java/lang/Double"));
	class_load(class_java_lang_Double);
	class_link(class_java_lang_Double);

	class_java_lang_Float =	class_new(utf_new_char("java/lang/Float"));
	class_load(class_java_lang_Float);
	class_link(class_java_lang_Float);

	class_java_lang_Character =	class_new(utf_new_char("java/lang/Character"));
	class_load(class_java_lang_Character);
	class_link(class_java_lang_Character);

	class_java_lang_Integer = class_new(utf_new_char("java/lang/Integer"));
	class_load(class_java_lang_Integer);
	class_link(class_java_lang_Integer);

	class_java_lang_Long = class_new(utf_new_char("java/lang/Long"));
	class_load(class_java_lang_Long);
	class_link(class_java_lang_Long);

	class_java_lang_Byte = class_new(utf_new_char("java/lang/Byte"));
	class_load(class_java_lang_Byte);
	class_link(class_java_lang_Byte);

	class_java_lang_Short = class_new(utf_new_char("java/lang/Short"));
	class_load(class_java_lang_Short);
	class_link(class_java_lang_Short);

	class_java_lang_Boolean = class_new(utf_new_char("java/lang/Boolean"));
	class_load(class_java_lang_Boolean);
	class_link(class_java_lang_Boolean);

	class_java_lang_Void = class_new(utf_new_char("java/lang/Void"));
	class_load(class_java_lang_Void);
	class_link(class_java_lang_Void);

}


/*****************************************************************************

	create systemclassloader object and initialize instance fields  

******************************************************************************/

void init_systemclassloader() 
{
	log_text("init_systemclassloader");
	if (!SystemClassLoader) {
		native_loadclasses();
		log_text("Initializing new system class loader");
		/* create object and call initializer */
  		SystemClassLoader = (java_lang_ClassLoader *) native_new_and_init(class_new(utf_new_char("gnu/java/lang/SystemClassLoader")));

		/* systemclassloader has no parent */
		SystemClassLoader->parent      = NULL;
		SystemClassLoader->initialized = true;
	}
	log_text("leaving system class loader");
}


/********************* function: native_setclasspath **************************/
 
void native_setclasspath(char *path)
{
	/* set searchpath for classfiles */
	classpath = path;
}


/*********************** Function: native_findfunction *************************

	Looks up a method (must have the same class name, method name, descriptor
	and 'static'ness) and returns a function pointer to it.
	Returns: function pointer or NULL (if there is no such method)

	Remark: For faster operation, the names/descriptors are converted from C
		strings to Unicode the first time this function is called.

*******************************************************************************/

functionptr native_findfunction(utf *cname, utf *mname, 
								utf *desc, bool isstatic)
{
	int i;
	/* entry of table for fast string comparison */
	struct nativecompref *n;
	/* for warning message if no function is found */
	char *buffer;	         	
	int buffer_len;

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

#ifdef JOWENN_DEBUG
	buffer_len = 
		utf_strlen(cname) + utf_strlen(mname) + utf_strlen(desc) + 64;
	
	buffer = MNEW(char, buffer_len);

	strcpy(buffer, "searching matching function in native table:");
	utf_sprint(buffer+strlen(buffer), mname);
	strcpy(buffer+strlen(buffer), ": ");
	utf_sprint(buffer+strlen(buffer), desc);
	strcpy(buffer+strlen(buffer), " for class ");
	utf_sprint(buffer+strlen(buffer), cname);

	log_text(buffer);	

	MFREE(buffer, char, buffer_len);
#endif
		
	for (i = 0; i < NATIVETABLESIZE; i++) {
		n = &(nativecomptable[i]);

		if (cname == n->classname && mname == n->methodname &&
		    desc == n->descriptor && isstatic == n->isstatic)
			return n->func;
#ifdef JOWENN_DEBUG
			else {
				if (cname == n->classname && mname == n->methodname )  log_text("static and descriptor mismatch");
			
				else {
					buffer_len = 
					  utf_strlen(n->classname) + utf_strlen(n->methodname) + utf_strlen(n->descriptor) + 64;
	
					buffer = MNEW(char, buffer_len);

					strcpy(buffer, "comparing with:");
					utf_sprint(buffer+strlen(buffer), n->methodname);
					strcpy (buffer+strlen(buffer), ": ");
					utf_sprint(buffer+strlen(buffer), n->descriptor);
					strcpy(buffer+strlen(buffer), " for class ");
					utf_sprint(buffer+strlen(buffer), n->classname);

					log_text(buffer);	

					MFREE(buffer, char, buffer_len);
				}
			} 
#endif
	}

		
	/* no function was found, display warning */

	buffer_len = 
		utf_strlen(cname) + utf_strlen(mname) + utf_strlen(desc) + 64;

	buffer = MNEW(char, buffer_len);

	strcpy(buffer, "warning: native function ");
	utf_sprint(buffer + strlen(buffer), mname);
	strcpy(buffer + strlen(buffer), ": ");
	utf_sprint(buffer + strlen(buffer), desc);
	strcpy(buffer + strlen(buffer), " not found in class ");
	utf_sprint(buffer + strlen(buffer), cname);

	log_text(buffer);	

	MFREE(buffer, char, buffer_len);

/*  	exit(1); */

	/* keep compiler happy */
	return NULL;
}


/********************** function: javastring_new *******************************

	creates a new object of type java/lang/String with the text of 
	the specified utf8-string

	return: pointer to the string or NULL if memory is exhausted.	

*******************************************************************************/

java_lang_String *javastring_new(utf *u)
{
	char *utf_ptr = u->text;        /* current utf character in utf string    */
	int utflength = utf_strlen(u);  /* length of utf-string if uncompressed   */
	java_lang_String *s;		    /* result-string                          */
	java_chararray *a;
	s4 i;
	
	s = (java_lang_String *) builtin_new(class_java_lang_String);
	a = builtin_newarray_char(utflength);

	/* javastring or character-array could not be created */
	if (!a || !s)
		return NULL;

	/* decompress utf-string */
	for (i = 0; i < utflength; i++)
		a->data[i] = utf_nextu2(&utf_ptr);
	
	/* set fields of the javastring-object */
	s->value  = a;
	s->offset = 0;
	s->count  = utflength;

	return s;
}


/********************** function: javastring_new_char **************************

	creates a new java/lang/String object which contains the convertet
	C-string passed via text.

	return: the object pointer or NULL if memory is exhausted.

*******************************************************************************/

java_lang_String *javastring_new_char(char *text)
{
	s4 i;
	s4 len = strlen(text); /* length of the string */
	java_lang_String *s;   /* result-string */
	java_chararray *a;
	
	s = (java_lang_String *) builtin_new(class_java_lang_String);
	a = builtin_newarray_char(len);

	/* javastring or character-array could not be created */
	if (!a || !s)
		return NULL;

	/* copy text */
	for (i = 0; i < len; i++)
		a->data[i] = text[i];
	
	/* set fields of the javastring-object */
	s->value  = a;
	s->offset = 0;
	s->count  = len;

	return s;
}


/************************* function javastring_tochar **************************

	converts a Java string into a C string.
	
	return: pointer to C string
	
	Caution: every call of this function overwrites the previous string !!!
	
*******************************************************************************/

static char stringbuffer[MAXSTRINGSIZE];

char *javastring_tochar(java_objectheader *so) 
{
	java_lang_String *s = (java_lang_String *) so;
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
		stringbuffer[i] = a->data[s->offset + i];

	stringbuffer[i] = '\0';

	return stringbuffer;
}


/****************** function class_findfield_approx ****************************
	
	searches in 'classinfo'-structure for a field with the
	specified name

*******************************************************************************/
 
fieldinfo *class_findfield_approx(classinfo *c, utf *name)
{
	s4 i;

	for (i = 0; i < c->fieldscount; i++) {
		/* compare field names */
		if ((c->fields[i].name == name))
			return &(c->fields[i]);
	}

	/* field was not found, raise exception */	
	*exceptionptr = new_exception(string_java_lang_NoSuchFieldException);

	return NULL;
}


s4 class_findfield_index_approx(classinfo *c, utf *name)
{
	s4 i;

	for (i = 0; i < c->fieldscount; i++) {
		/* compare field names */
		if ((c->fields[i].name == name))
			return i;
	}

	/* field was not found, raise exception */	
	*exceptionptr = new_exception(string_java_lang_NoSuchFieldException);

	return -1;
}


/********************** function: native_new_and_init *************************

	Creates a new object on the heap and calls the initializer.
	Returns the object pointer or NULL if memory is exhausted.
			
*******************************************************************************/

java_objectheader *native_new_and_init(classinfo *c)
{
	methodinfo *m;
	java_objectheader *o;

	if (!c)
		return *exceptionptr;

	o = builtin_new(c);                 /* create object                      */
	
	if (!o)
		return NULL;

	/* find initializer */

	m = class_findmethod(c,
						 utf_new_char("<init>"),
						 utf_new_char("()V"));
	                      	                      
	if (!m) {                           /* initializer not found              */
		if (verbose) {
			char logtext[MAXLOGTEXT];
			sprintf(logtext, "Warning: class has no instance-initializer: ");
			utf_sprint_classname(logtext + strlen(logtext), c->name);
			log_text(logtext);
		}
		return o;
	}

	/* call initializer */

	asm_calljavafunction(m, o, NULL, NULL, NULL);

	return o;
}


java_objectheader *native_new_and_init_string(classinfo *c, java_lang_String *s)
{
	methodinfo *m;
	java_objectheader *o;

	if (!c)
		return *exceptionptr;

	o = builtin_new(c);          /* create object          */

	if (!o)
		return NULL;

	/* find initializer */

	m = class_findmethod(c,
						 utf_new_char("<init>"),
						 utf_new_char("(Ljava/lang/String;)V"));
	                      	                      
	if (!m) {                                       /* initializer not found  */
		if (verbose) {
			char logtext[MAXLOGTEXT];
			sprintf(logtext, "Warning: class has no instance-initializer: ");
			utf_sprint_classname(logtext + strlen(logtext), c->name);
			log_text(logtext);
		}
		return o;
	}

	/* call initializer */

	asm_calljavafunction(m, o, s, NULL, NULL);

	return o;
}


java_objectheader *native_new_and_init_int(classinfo *c, s4 i)
{
	methodinfo *m;
	java_objectheader *o;

	if (!c)
		return *exceptionptr;

	o = builtin_new(c);          /* create object          */
	
	if (!o) return NULL;

	/* find initializer */

	m = class_findmethod(c,
						 utf_new_char("<init>"),
						 utf_new_char("(I)V"));
	                      	                      
	if (!m) {                                       /* initializer not found  */
		if (verbose) {
			char logtext[MAXLOGTEXT];
			sprintf(logtext, "Warning: class has no instance-initializer: ");
			utf_sprint_classname(logtext + strlen(logtext), c->name);
			log_text(logtext);
		}
		return o;
	}

	/* call initializer */

#if defined(__I386__) || defined(__POWERPC__)
	asm_calljavafunction(m, o, (void *) i, NULL, NULL);
#else
	asm_calljavafunction(m, o, (void *) (s8) i, NULL, NULL);
#endif

	return o;
}


java_objectheader *native_new_and_init_throwable(classinfo *c, java_lang_Throwable *t)
{
	methodinfo *m;
	java_objectheader *o;

	if (!c)
		return *exceptionptr;

	o = builtin_new(c);          /* create object          */
	
	if (!o) return NULL;

	/* find initializer */

	m = class_findmethod(c,
						 utf_new_char("<init>"),
						 utf_new_char("(Ljava/lang/Throwable;)V"));
	                      	                      
	if (!m) {                                       /* initializer not found  */
		if (verbose) {
			char logtext[MAXLOGTEXT];
			sprintf(logtext, "Warning: class has no instance-initializer: ");
			utf_sprint_classname(logtext + strlen(logtext), c->name);
			log_text(logtext);
		}
		return o;
	}

	/* call initializer */

	asm_calljavafunction(m, o, t, NULL, NULL);

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
				
				if (!js || !js->value) 
					/* error in hashtable found */
					panic("invalid literalstring in hashtable");

				a = js->value;

				if (!js->header.vftbl) 
					/* vftbl of javastring is NULL */ 
					js->header.vftbl = class_java_lang_String->vftbl;

				if (!a->header.objheader.vftbl) 
					/* vftbl of character-array is NULL */ 
					a->header.objheader.vftbl = primitivetype_table[ARRAYTYPE_CHAR].arrayvftbl;

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
	/* printf("utf_new_u2: unicode_length=%d\n",unicode_length);    	*/
	buflength = u2_utflength(unicode_pos, unicode_length); 
	buffer    = MNEW(char, buflength);
 
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
			if (isclassname && c == '.')
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

/*  	printf("javastring_toutf offset: %d, len %d\n",str->offset, str->count); */
/*  	fflush(stdout); */

	return utf_new_u2(str->value->data + str->offset, str->count, isclassname);
}


/********************* function: literalstring_u2 *****************************

    searches for the javastring with the specified u2-array in 
    the string hashtable, if there is no such string a new one is 
    created 

    if copymode is true a copy of the u2-array is made

*******************************************************************************/

java_objectheader *literalstring_u2(java_chararray *a, u4 length, u4 offset,
									bool copymode)
{
    literalstring *s;                /* hashtable element */
    java_lang_String *js;            /* u2-array wrapped in javastring */
    java_chararray *stringdata;      /* copy of u2-array */      
	classinfo *c;
    u4 key;
    u4 slot;
    u2 i;

//#define DEBUG_LITERALSTRING_U2
#ifdef DEBUG_LITERALSTRING_U2
    printf("literalstring_u2: length=%d, offset=%d\n", length, offset);
	fflush(stdout);
#endif
    
    /* find location in hashtable */
    key  = unicode_hashkey(a->data + offset, length);
    slot = key & (string_hash.size - 1);
    s    = string_hash.ptr[slot];

    while (s) {
		js = (java_lang_String *) s->string;

		if (length == js->count) {
			/* compare text */
			for (i = 0; i < length; i++) {
				if (a->data[offset + i] != js->value->data[i])
					goto nomatch;
			}

			/* string already in hashtable, free memory */
			if (!copymode)
				lit_mem_free(a, sizeof(java_chararray) + sizeof(u2) * (length - 1) + 10);

#ifdef DEBUG_LITERALSTRING_U2
			printf("literalstring_u2: foundentry at %p\n", js);
			utf_display(javastring_toutf(js, 0));
			printf("\n\n");
			fflush(stdout);
#endif
			return (java_objectheader *) js;
		}

	nomatch:
		/* follow link in external hash chain */
		s = s->hashlink;
    }

    if (copymode) {
		/* create copy of u2-array for new javastring */
		u4 arraysize = sizeof(java_chararray) + sizeof(u2) * (length - 1) + 10;
		stringdata = lit_mem_alloc(arraysize);
/*    		memcpy(stringdata, a, arraysize); */
  		memcpy(&(stringdata->header), &(a->header), sizeof(java_arrayheader));
  		memcpy(&(stringdata->data), &(a->data) + offset, sizeof(u2) * (length - 1) + 10);

    } else {
		stringdata = a;
	}

    /* location in hashtable found, complete arrayheader */
    stringdata->header.objheader.vftbl = primitivetype_table[ARRAYTYPE_CHAR].arrayvftbl;
    stringdata->header.size = length;

	/* if we use eager loading, we have to check loaded String class */
	if (opt_eager) {
		class_java_lang_String =
			class_new_intern(utf_new_char("java/lang/String"));

		if (!class_load(class_java_lang_String))
			return NULL;

		list_addfirst(&unlinkedclasses, class_java_lang_String);
	}

    /* create new javastring */
    js = LNEW(java_lang_String);
	js->header.vftbl = class_java_lang_String->vftbl;
    js->value  = stringdata;
    js->offset = 0;
    js->count  = length;

#ifdef DEBUG_LITERALSTRING_U2
	printf("literalstring_u2: newly created at %p\n", js);
	utf_display(javastring_toutf(js, 0));
	printf("\n\n");
	fflush(stdout);
#endif
			
    /* create new literalstring */
    s = NEW(literalstring);
    s->hashlink = string_hash.ptr[slot];
    s->string   = (java_objectheader *) js;
    string_hash.ptr[slot] = s;

    /* update number of hashtable entries */
    string_hash.entries++;

    /* reorganization of hashtable */       
    if (string_hash.entries > (string_hash.size * 2)) {
		/* reorganization of hashtable, average length of 
         the external chains is approx. 2                */  

		u4 i;
		literalstring *s;
		hashtable newhash; /* the new hashtable */
      
		/* create new hashtable, double the size */
		init_hashtable(&newhash, string_hash.size * 2);
		newhash.entries = string_hash.entries;
      
		/* transfer elements to new hashtable */
		for (i = 0; i < string_hash.size; i++) {
			s = string_hash.ptr[i];
			while (s) {
				literalstring *nexts = s->hashlink;
				js   = (java_lang_String *) s->string;
				slot = unicode_hashkey(js->value->data, js->count) & (newhash.size - 1);
	  
				s->hashlink = newhash.ptr[slot];
				newhash.ptr[slot] = s;
	
				/* follow link in external hash chain */  
				s = nexts;
			}
		}
	
		/* dispose old table */	
		MFREE(string_hash.ptr, void*, string_hash.size);
		string_hash = newhash;
    }

    return (java_objectheader *) js;
}


/******************** Function: literalstring_new *****************************

    creates a new javastring with the text of the utf-symbol
    and inserts it into the string hashtable

*******************************************************************************/

java_objectheader *literalstring_new(utf *u)
{
    char *utf_ptr = u->text;         /* pointer to current unicode character in utf string */
    u4 utflength  = utf_strlen(u);   /* length of utf-string if uncompressed */
    java_chararray *a;               /* u2-array constructed from utf string */
    u4 i;

    /* allocate memory */ 
    a = lit_mem_alloc(sizeof(java_chararray) + sizeof(u2) * (utflength - 1) + 10);

    /* convert utf-string to u2-array */
    for (i = 0; i < utflength; i++)
		a->data[i] = utf_nextu2(&utf_ptr);

    return literalstring_u2(a, utflength, 0, false);
}


/********************** function: literalstring_free **************************

        removes a javastring from memory		       

******************************************************************************/

void literalstring_free(java_objectheader* sobj)
{
	java_lang_String *s = (java_lang_String *) sobj;
	java_chararray *a = s->value;

	/* dispose memory of java.lang.String object */
	LFREE(s, java_lang_String);

	/* dispose memory of java-characterarray */
	LFREE(a, sizeof(java_chararray) + sizeof(u2) * (a->header.size - 1)); /* +10 ?? */
}


void copy_vftbl(vftbl **dest, vftbl *src)
{
    *dest = src;
#if 0
    /* XXX this kind of copying does not work (in the general
     * case). The interface tables would have to be copied, too. I
     * don't see why we should make a copy anyway. -Edwin
     */
	*dest = mem_alloc(sizeof(vftbl) + sizeof(methodptr)*(src->vftbllength-1));
	memcpy(*dest, src, sizeof(vftbl) - sizeof(methodptr));
	memcpy(&(*dest)->table, &src->table, src->vftbllength * sizeof(methodptr));
#endif
}


/****************************************************************************************** 											   			

	creates method signature (excluding return type) from array of 
	class-objects representing the parameters of the method 

*******************************************************************************************/


utf *create_methodsig(java_objectarray* types, char *retType)
{
    char *buffer;       /* buffer for building the desciptor */
    char *pos;          /* current position in buffer */
    utf *result;        /* the method signature */
    u4 buffer_size = 3; /* minimal size=3: room for parenthesis and returntype */
    u4 i, j;
 
    if (!types) return NULL;

    /* determine required buffer-size */    
    for (i = 0; i < types->header.size; i++) {
		classinfo *c = (classinfo *) types->data[i];
		buffer_size  = buffer_size + c->name->blength + 2;
    }

    if (retType) buffer_size += strlen(retType);

    /* allocate buffer */
    buffer = MNEW(u1, buffer_size);
    pos    = buffer;
    
    /* method-desciptor starts with parenthesis */
    *pos++ = '(';

    for (i = 0; i < types->header.size; i++) {
		char ch;	   

		/* current argument */
	    classinfo *c = (classinfo *) types->data[i];

	    /* current position in utf-text */
	    char *utf_ptr = c->name->text; 
	    
	    /* determine type of argument */
	    if ((ch = utf_nextu2(&utf_ptr)) == '[') {
	    	/* arrayclass */
	        for (utf_ptr--; utf_ptr < utf_end(c->name); utf_ptr++) {
				*pos++ = *utf_ptr; /* copy text */
			}

	    } else {	   	
			/* check for primitive types */
			for (j = 0; j < PRIMITIVETYPE_COUNT; j++) {
				char *utf_pos	= utf_ptr - 1;
				char *primitive = primitivetype_table[j].wrapname;

				/* compare text */
				while (utf_pos < utf_end(c->name)) {
					if (*utf_pos++ != *primitive++) goto nomatch;
				}

				/* primitive type found */
				*pos++ = primitivetype_table[j].typesig;
				goto next_type;

			nomatch:
				;
			}

			/* no primitive type and no arrayclass, so must be object */
			*pos++ = 'L';

			/* copy text */
			for (utf_ptr--; utf_ptr < utf_end(c->name); utf_ptr++) {
				*pos++ = *utf_ptr;
			}

			*pos++ = ';';

		next_type:
			;
		}  
    }	    

    *pos++ = ')';

    if (retType) {
		for (i = 0; i < strlen(retType); i++) {
			*pos++ = retType[i];
		}
    }

    /* create utf-string */
    result = utf_new(buffer, (pos - buffer));
    MFREE(buffer, u1, buffer_size);

    return result;
}


/******************************************************************************************

	retrieve the next argument or returntype from a descriptor
	and return the corresponding class 

*******************************************************************************************/

classinfo *get_type(char **utf_ptr,char *desc_end, bool skip)
{
    classinfo *c = class_from_descriptor(*utf_ptr,desc_end,utf_ptr,
                                         (skip) ? CLASSLOAD_SKIP : CLASSLOAD_LOAD);
    if (!c)
	/* unknown type */
	panic("illegal descriptor");

    if (skip) return NULL;

    use_class_as_object(c);
    return c;
}


/******************************************************************************************

	use the descriptor of a method to generate a java/lang/Class array
	which contains the classes of the parametertypes of the method

*******************************************************************************************/

java_objectarray* get_parametertypes(methodinfo *m) 
{
    utf  *descr    =  m->descriptor;    /* method-descriptor */ 
    char *utf_ptr  =  descr->text;      /* current position in utf-text */
    char *desc_end =  utf_end(descr);   /* points behind utf string     */
    java_objectarray* result;
    int parametercount = 0;
    int i;

    /* skip '(' */
    utf_nextu2(&utf_ptr);
  
    /* determine number of parameters */
    while ( *utf_ptr != ')' ) {
    	get_type(&utf_ptr,desc_end,true);
	parametercount++;
    }

    /* create class-array */
    result = builtin_anewarray(parametercount, class_java_lang_Class);

    utf_ptr  =  descr->text;
    utf_nextu2(&utf_ptr);

    /* get returntype classes */
    for (i = 0; i < parametercount; i++)
	    result->data[i] = (java_objectheader *) get_type(&utf_ptr,desc_end, false);

    return result;
}





/******************************************************************************************

	get the exceptions which can be thrown by a method	

*******************************************************************************************/

java_objectarray* get_exceptiontypes(methodinfo *m) {
    u2 exccount=m->thrownexceptionscount;
    u2 i;
    java_objectarray *result;
    /* create class-array */
    result = builtin_anewarray(exccount, class_java_lang_Class);
    for (i=0;i<exccount;i++) {
	java_objectheader *oh=(java_objectheader*)(m->thrownexceptions[i]);
	use_class_as_object(oh);
	result->data[i]=oh;
    }
    return result;
}





/******************************************************************************************

	get the returntype class of a method

*******************************************************************************************/

classinfo *get_returntype(methodinfo *m) 
{
	char *utf_ptr;   /* current position in utf-text */
	char *desc_end;  /* points behind utf string     */
        utf *desc = m->descriptor; /* method-descriptor  */

	utf_ptr  = desc->text;
	desc_end = utf_end(desc);

	/* ignore parametertypes */
        while ((utf_ptr<desc_end) && utf_nextu2(&utf_ptr)!=')')
		/* skip */ ;

	return get_type(&utf_ptr,desc_end, false);
}


/*****************************************************************************/
/*****************************************************************************/


/*--------------------------------------------------------*/
void printNativeCall(nativeCall nc) {
  int i,j;

  printf("\n%s's Native Methods call:\n",nc.classname); fflush(stdout);
  for (i=0; i<nc.methCnt; i++) {  
      printf("\tMethod=%s %s\n",nc.methods[i].methodname, nc.methods[i].descriptor);fflush(stdout);

    for (j=0; j<nc.callCnt[i]; j++) {  
        printf("\t\t<%i,%i>aCalled = %s %s %s\n",i,j,
	nc.methods[i].methodCalls[j].classname, 
	nc.methods[i].methodCalls[j].methodname, 
	nc.methods[i].methodCalls[j].descriptor);fflush(stdout);
      }
    }
  printf("-+++++--------------------\n");fflush(stdout);
}

/*--------------------------------------------------------*/
void printCompNativeCall(nativeCompCall nc) {
  int i,j;
  printf("printCompNativeCall BEGIN\n");fflush(stdout); 
  printf("\n%s's Native Comp Methods call:\n",nc.classname->text);fflush(stdout);
  utf_display(nc.classname); fflush(stdout);
  
  for (i=0; i<nc.methCnt; i++) {  
    printf("\tMethod=%s %s\n",nc.methods[i].methodname->text,nc.methods[i].descriptor->text);fflush(stdout);
    utf_display(nc.methods[i].methodname); fflush(stdout);
    utf_display(nc.methods[i].descriptor);fflush(stdout);
    printf("\n");fflush(stdout);

    for (j=0; j<nc.callCnt[i]; j++) {  
      printf("\t\t<%i,%i>bCalled = ",i,j);fflush(stdout);
	utf_display(nc.methods[i].methodCalls[j].classname);fflush(stdout);
	utf_display(nc.methods[i].methodCalls[j].methodname); fflush(stdout);
	utf_display(nc.methods[i].methodCalls[j].descriptor);fflush(stdout);
	printf("\n");fflush(stdout);
      }
    }
printf("---------------------\n");fflush(stdout);
}


/*--------------------------------------------------------*/
classMeth findNativeMethodCalls(utf *c, utf *m, utf *d ) 
{
    int i = 0;
    int j = 0;
    int cnt = 0;
    classMeth mc;
    mc.i_class = i;
    mc.j_method = j;
    mc.methCnt = cnt;

    return mc;
}

/*--------------------------------------------------------*/
nativeCall* findNativeClassCalls(char *aclassname ) {
int i;

for (i=0;i<NATIVECALLSSIZE; i++) {
   /* convert table to utf later to speed up search */ 
   if (strcmp(nativeCalls[i].classname, aclassname) == 0) 
	return &nativeCalls[i];
   }

return NULL;
}
/*--------------------------------------------------------*/
/*--------------------------------------------------------*/
void utfNativeCall(nativeCall nc, nativeCompCall *ncc) {
  int i,j;


  ncc->classname = utf_new_char(nc.classname); 
  ncc->methCnt = nc.methCnt;
  
  for (i=0; i<nc.methCnt; i++) {  
    ncc->methods[i].methodname = utf_new_char(nc.methods[i].methodname);
    ncc->methods[i].descriptor = utf_new_char(nc.methods[i].descriptor);
    ncc->callCnt[i] = nc.callCnt[i];

    for (j=0; j<nc.callCnt[i]; j++) {  

	ncc->methods[i].methodCalls[j].classname  = utf_new_char(nc.methods[i].methodCalls[j].classname);

        if (strcmp("", nc.methods[i].methodCalls[j].methodname) != 0) {
          ncc->methods[i].methodCalls[j].methodname = utf_new_char(nc.methods[i].methodCalls[j].methodname);
          ncc->methods[i].methodCalls[j].descriptor = utf_new_char(nc.methods[i].methodCalls[j].descriptor);
          }
        else {
          ncc->methods[i].methodCalls[j].methodname = NULL;
          ncc->methods[i].methodCalls[j].descriptor = NULL;
          }
      }
    }
}



/*--------------------------------------------------------*/

bool natcall2utf(bool natcallcompdone) {
int i;

if (natcallcompdone) 
	return true;

for (i=0;i<NATIVECALLSSIZE; i++) {
   utfNativeCall  (nativeCalls[i], &nativeCompCalls[i]);  
   }

return true;
}

/*--------------------------------------------------------*/


java_objectarray *builtin_asm_createclasscontextarray(classinfo **end,classinfo **start)
{
#if defined(__GNUC__)
#warning platform dependend
#endif
        java_objectarray *tmpArray;
        int i;
        classinfo **current;
	classinfo *c;
        size_t size=(((size_t)start)-((size_t)end)) / sizeof (classinfo*);
        /*printf("end %p, start %p, size %ld\n",end,start,size);*/
        if (!class_java_lang_Class)
                class_java_lang_Class = class_new(utf_new_char ("java/lang/Class"));
        if (!class_java_lang_SecurityManager)
                class_java_lang_SecurityManager = class_new(utf_new_char ("java/lang/SecurityManager"));
	if (size>0) {
		if (start==class_java_lang_SecurityManager) {
			size--;
			start--;
		}
	}
        tmpArray=builtin_newarray(size, class_array_of(class_java_lang_Class)->vftbl);

        for(i=0,current=start;i<size;i++,current--) {
		c=*current;
/*		printf("%d\n",i);
                utf_display(c->name);*/
		use_class_as_object(c);
		tmpArray->data[i]=c;
        }
	return tmpArray;

}

java_lang_ClassLoader *builtin_asm_getclassloader(classinfo **end,classinfo **start)
{
#if defined(__GNUC__)
#warning platform dependend
#endif
        int i;
        classinfo **current;
	classinfo *c;
	classinfo *privilegedAction;
        size_t size=(((size_t)start)-((size_t)end)) / sizeof (classinfo*);
/*	log_text("builtin_asm_getclassloader");
        printf("end %p, start %p, size %ld\n",end,start,size);*/

        if (!class_java_lang_SecurityManager)
                class_java_lang_SecurityManager = class_new(utf_new_char ("java/lang/SecurityManager"));
	if (size>0) {
		if (start==class_java_lang_SecurityManager) {
			size--;
			start--;
		}
	}

	privilegedAction=class_new(utf_new_char("java/security/PrivilegedAction"));

        for(i=0,current=start;i<size;i++,current--) {
		c=*current;
		if (c==privilegedAction) return NULL;
		if (c->classloader) return c->classloader;
        }
	return NULL;




/*
        log_text("Java_java_lang_VMSecurityManager_currentClassLoader");
        init_systemclassloader();

        return SystemClassLoader;*/
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
