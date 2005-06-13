/* src/native/native.c - table of native functions

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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

   Changes: Christian Thalinger

   $Id: native.c 2644 2005-06-13 13:42:48Z twisti $

*/


#include <assert.h>

#if !defined(STATIC_CLASSPATH)
# include "src/libltdl/ltdl.h"
#endif

#include "config.h"
#include "types.h"
#include "cacao/cacao.h"
#include "mm/memory.h"
#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_Throwable.h"
#include "toolbox/logging.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/resolve.h"
#include "vm/stringlocal.h"
#include "vm/tables.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/jit.h"


/* include table of native functions ******************************************/

#include "native/include/java_lang_Cloneable.h"
#include "native/include/java_util_Properties.h"
#include "native/include/java_io_InputStream.h"
#include "native/include/java_io_PrintStream.h"

#include "native/include/gnu_classpath_VMStackWalker.h"
#include "native/include/gnu_classpath_VMSystemProperties.h"
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_VMClass.h"
#include "native/include/java_lang_VMClassLoader.h"
#include "native/include/java_lang_VMObject.h"
#include "native/include/java_lang_VMRuntime.h"
#include "native/include/java_lang_VMString.h"
#include "native/include/java_lang_VMSystem.h"
#include "native/include/java_lang_VMThread.h"
#include "native/include/java_lang_VMThrowable.h"
#include "native/include/java_lang_reflect_Constructor.h"
#include "native/include/java_lang_reflect_Field.h"
#include "native/include/java_lang_reflect_Proxy.h"
#include "native/include/java_lang_reflect_Method.h"
#include "native/include/java_security_VMAccessController.h"

#if defined(STATIC_CLASSPATH)

/* these are required to prevent compiler warnings */

#include "native/include/java_net_DatagramPacket.h"
#include "native/include/java_net_InetAddress.h"
#include "native/include/java_net_SocketImpl.h"

#include "native/include/gnu_java_net_PlainDatagramSocketImpl.h"
#include "native/include/gnu_java_net_PlainSocketImpl.h"
#include "native/include/gnu_java_nio_PipeImpl.h"
#include "native/include/gnu_java_nio_channels_FileChannelImpl.h"
#include "native/include/gnu_java_nio_charset_iconv_IconvEncoder.h"
#include "native/include/gnu_java_nio_charset_iconv_IconvDecoder.h"
#include "native/include/java_lang_VMProcess.h"
#include "native/include/java_nio_MappedByteBufferImpl.h"
#include "native/include/java_nio_channels_spi_SelectorProvider.h"

/* now include the native table */

#include "native/nativetable.inc"

#else /* defined(STATIC_CLASSPATH) */

/* Ensure that symbols for functions implemented within CACAO are used and    */
/* exported to dlopen.                                                        */

static functionptr dummynativetable[] = {
	(functionptr) Java_gnu_classpath_VMStackWalker_getClassContext,

	(functionptr) Java_gnu_classpath_VMSystemProperties_preInit,

	(functionptr) Java_java_lang_VMClass_isInstance,
	(functionptr) Java_java_lang_VMClass_isAssignableFrom,
	(functionptr) Java_java_lang_VMClass_isInterface,
	(functionptr) Java_java_lang_VMClass_isPrimitive,
	(functionptr) Java_java_lang_VMClass_getName,
	(functionptr) Java_java_lang_VMClass_getSuperclass,
	(functionptr) Java_java_lang_VMClass_getInterfaces,
	(functionptr) Java_java_lang_VMClass_getComponentType,
	(functionptr) Java_java_lang_VMClass_getModifiers,
	(functionptr) Java_java_lang_VMClass_getDeclaringClass,
	(functionptr) Java_java_lang_VMClass_getDeclaredClasses,
	(functionptr) Java_java_lang_VMClass_getDeclaredFields,
	(functionptr) Java_java_lang_VMClass_getDeclaredMethods,
	(functionptr) Java_java_lang_VMClass_getDeclaredConstructors,
	(functionptr) Java_java_lang_VMClass_getClassLoader,
	(functionptr) Java_java_lang_VMClass_forName,
	(functionptr) Java_java_lang_VMClass_isArray,
	(functionptr) Java_java_lang_VMClass_initialize,
	(functionptr) Java_java_lang_VMClass_loadArrayClass,
	(functionptr) Java_java_lang_VMClass_throwException,

	(functionptr) Java_java_lang_VMClassLoader_defineClass,
	(functionptr) Java_java_lang_VMClassLoader_resolveClass,
	(functionptr) Java_java_lang_VMClassLoader_loadClass,
	(functionptr) Java_java_lang_VMClassLoader_getPrimitiveClass,
	(functionptr) Java_java_lang_VMClassLoader_nativeGetResources,

	(functionptr) Java_java_lang_VMObject_getClass,
	(functionptr) Java_java_lang_VMObject_clone,
	(functionptr) Java_java_lang_VMObject_notify,
	(functionptr) Java_java_lang_VMObject_notifyAll,
	(functionptr) Java_java_lang_VMObject_wait,

	(functionptr) Java_java_lang_VMRuntime_availableProcessors,
	(functionptr) Java_java_lang_VMRuntime_freeMemory,
	(functionptr) Java_java_lang_VMRuntime_totalMemory,
	(functionptr) Java_java_lang_VMRuntime_maxMemory,
	(functionptr) Java_java_lang_VMRuntime_gc,
	(functionptr) Java_java_lang_VMRuntime_runFinalization,
	(functionptr) Java_java_lang_VMRuntime_runFinalizationForExit,
	(functionptr) Java_java_lang_VMRuntime_traceInstructions,
	(functionptr) Java_java_lang_VMRuntime_traceMethodCalls,
	(functionptr) Java_java_lang_VMRuntime_runFinalizersOnExit,
	(functionptr) Java_java_lang_VMRuntime_exit,
	(functionptr) Java_java_lang_VMRuntime_nativeLoad,
	(functionptr) Java_java_lang_VMRuntime_mapLibraryName,

	(functionptr) Java_java_lang_VMString_intern,

	(functionptr) Java_java_lang_VMSystem_arraycopy,
	(functionptr) Java_java_lang_VMSystem_identityHashCode,

	(functionptr) Java_java_lang_VMThread_start,
	(functionptr) Java_java_lang_VMThread_interrupt,
	(functionptr) Java_java_lang_VMThread_isInterrupted,
	(functionptr) Java_java_lang_VMThread_suspend,
	(functionptr) Java_java_lang_VMThread_resume,
	(functionptr) Java_java_lang_VMThread_nativeSetPriority,
	(functionptr) Java_java_lang_VMThread_nativeStop,
	(functionptr) Java_java_lang_VMThread_currentThread,
	(functionptr) Java_java_lang_VMThread_yield,
	(functionptr) Java_java_lang_VMThread_interrupted,
	(functionptr) Java_java_lang_VMThread_holdsLock,

	(functionptr) Java_java_lang_VMThrowable_fillInStackTrace,
	(functionptr) Java_java_lang_VMThrowable_getStackTrace,

	(functionptr) Java_java_lang_reflect_Constructor_getModifiers,
	(functionptr) Java_java_lang_reflect_Constructor_constructNative,

	(functionptr) Java_java_lang_reflect_Field_getModifiers,
	(functionptr) Java_java_lang_reflect_Field_getType,
	(functionptr) Java_java_lang_reflect_Field_get,
	(functionptr) Java_java_lang_reflect_Field_getBoolean,
	(functionptr) Java_java_lang_reflect_Field_getByte,
	(functionptr) Java_java_lang_reflect_Field_getChar,
	(functionptr) Java_java_lang_reflect_Field_getShort,
	(functionptr) Java_java_lang_reflect_Field_getInt,
	(functionptr) Java_java_lang_reflect_Field_getLong,
	(functionptr) Java_java_lang_reflect_Field_getFloat,
	(functionptr) Java_java_lang_reflect_Field_getDouble,
	(functionptr) Java_java_lang_reflect_Field_set,
	(functionptr) Java_java_lang_reflect_Field_setBoolean,
	(functionptr) Java_java_lang_reflect_Field_setByte,
	(functionptr) Java_java_lang_reflect_Field_setChar,
	(functionptr) Java_java_lang_reflect_Field_setShort,
	(functionptr) Java_java_lang_reflect_Field_setInt,
	(functionptr) Java_java_lang_reflect_Field_setLong,
	(functionptr) Java_java_lang_reflect_Field_setFloat,
	(functionptr) Java_java_lang_reflect_Field_setDouble,

	(functionptr) Java_java_lang_reflect_Proxy_getProxyClass0,
	(functionptr) Java_java_lang_reflect_Proxy_getProxyData0,
	(functionptr) Java_java_lang_reflect_Proxy_generateProxyClass0,

	(functionptr) Java_java_lang_reflect_Method_getModifiers,
	(functionptr) Java_java_lang_reflect_Method_getReturnType,
	(functionptr) Java_java_lang_reflect_Method_getParameterTypes,
	(functionptr) Java_java_lang_reflect_Method_getExceptionTypes,
	(functionptr) Java_java_lang_reflect_Method_invokeNative,

	(functionptr) Java_java_security_VMAccessController_getStack,
};

#endif /* defined(STATIC_CLASSPATH) */


/************* use classinfo structure as java.lang.Class object **************/

void use_class_as_object(classinfo *c) 
{
	if (!c->classvftbl) {
		/* is the class loaded */
		if (!c->loaded) {
/*  			if (!class_load(c)) */
/*  				throw_exception_exit(); */
			log_text("use_class_as_object: class_load should not happen");
			assert(0);
		}

		/* is the class linked */
		if (!c->linked)
			if (!link_class(c))
				throw_exception_exit();

		assert(class_java_lang_Class);

		c->header.vftbl = class_java_lang_Class->vftbl;
		c->classvftbl = true;
  	}
}


/************************** tables for methods ********************************/

#undef JOWENN_DEBUG
#undef JOWENN_DEBUG1

#ifdef STATIC_CLASSPATH
#define NATIVETABLESIZE  (sizeof(nativetable)/sizeof(struct nativeref))

/* table for fast string comparison */
static nativecompref nativecomptable[NATIVETABLESIZE];

/* string comparsion table initialized */
static bool nativecompdone = false;
#endif


/* XXX don't define this in a header file!!! */

static struct nativeCall nativeCalls[] =
{
#include "nativecalls.inc"
};

#define NATIVECALLSSIZE    (sizeof(nativeCalls) / sizeof(struct nativeCall))

struct nativeCompCall nativeCompCalls[NATIVECALLSSIZE];


/* native_loadclasses **********************************************************

   Load classes required for native methods.

*******************************************************************************/

bool native_init(void)
{
#if !defined(STATIC_CLASSPATH)
	void *p;

	/* We need to access the dummy native table, not only to remove a warning */
	/* but to be sure that the table is not optimized away (gcc does this     */
	/* since 3.4).                                                            */

	p = &dummynativetable;

	/* initialize libltdl */

	if (lt_dlinit()) {
		/* XXX how can we throw an exception here? */
		log_text(lt_dlerror());

		return false;
	}
#endif

	/* everything's ok */

	return true;
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
#ifdef STATIC_CLASSPATH
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
#else
/* dynamic classpath */
  return 0;
#endif
}


/* native_make_overloaded_function *********************************************

   XXX

*******************************************************************************/

#if !defined(STATIC_CLASSPATH)
static char *native_make_overloaded_function(char *name, utf *desc)
{
	s4    namelen;
	char *utf_ptr;
	u2    c;
	s4    i;

	utf_ptr = desc->text;
	namelen = strlen("__");

	/* calculate additional length */

	while ((c = utf_nextu2(&utf_ptr)) != ')') {
		switch (c) {
		case 'Z':
		case 'B':
		case 'C':
		case 'S':
		case 'I':
		case 'J':
		case 'F':
		case 'D':
			namelen++;
			break;
		case '[':
			namelen += 2;
			break;
		case 'L':
			namelen++;
			while (utf_nextu2(&utf_ptr) != ';')
				namelen++;
			namelen += 2;
			break;
		case '(':
			break;
		default:
			assert(0);
		}
	}


	/* reallocate memory */

	i = strlen(name);

	DMREALLOC(name, char, i, namelen);

	utf_ptr = desc->text;

	name[i++] = '_';
	name[i++] = '_';

	while ((c = utf_nextu2(&utf_ptr)) != ')') {
		switch (c) {
		case 'Z':
		case 'B':
		case 'C':
		case 'S':
		case 'J':
		case 'I':
		case 'F':
		case 'D':
			name[i++] = c;
			break;
		case '[':
			name[i++] = '_';
			name[i++] = '3';
			break;
		case 'L':
			name[i++] = 'L';
			while ((c = utf_nextu2(&utf_ptr)) != ';')
				if (((c >= 'a') && (c <= 'z')) ||
					((c >= 'A') && (c <= 'Z')) ||
					((c >= '0') && (c <= '9')))
					name[i++] = c;
				else
					name[i++] = '_';
			name[i++] = '_';
			name[i++] = '2';
			break;
		case '(':
			break;
		default:
			assert(0);
		}
	}

	/* close string */

	name[i] = '\0';

	return name;
}


/* native_resolve_function *****************************************************

   Resolves a native function, maybe from a dynamic library.

*******************************************************************************/

functionptr native_resolve_function(methodinfo *m)
{
	lt_dlhandle  handle;
	lt_ptr       sym;
	char        *name;
	s4           namelen;
	char        *utf_ptr;
	char        *utf_endptr;
	s4           dumpsize;
	s4           i;

	/* get the handle for the main program */

	handle = lt_dlopen(NULL);

	if (!handle) {
		*exceptionptr =
			new_internalerror("lt_dlopen: can't get handle for the main program");
		return NULL;
	}

	/* calculate length of native function name */

	namelen = strlen("Java_") + utf_strlen(m->class->name) + strlen("_") +
		utf_strlen(m->name) + strlen("0");

	/* check for underscores in class name */

	utf_ptr = m->class->name->text;
	utf_endptr = UTF_END(m->class->name);

	while (utf_ptr < utf_endptr)
		if (utf_nextu2(&utf_ptr) == '_')
			namelen++;

	/* check for underscores in method name */

	utf_ptr = m->name->text;
	utf_endptr = UTF_END(m->name);

	while (utf_ptr < utf_endptr)
		if (utf_nextu2(&utf_ptr) == '_')
			namelen++;

	/* allocate memory */

	dumpsize = dump_size();

	name = DMNEW(char, namelen);


	/* generate name of native functions */

	strcpy(name, "Java_");
	i = strlen("Java_");

	utf_ptr = m->class->name->text;
	utf_endptr = UTF_END(m->class->name);

	for (; utf_ptr < utf_endptr; utf_ptr++, i++) {
		name[i] = *utf_ptr;

		/* escape sequence for '_' is '_1' */

		if (name[i] == '_')
			name[++i] = '1';

		/* replace '/' with '_' */

		if (name[i] == '/')
			name[i] = '_';
	}

	/* seperator between class and method */

	name[i++] = '_';

	utf_ptr = m->name->text;
	utf_endptr = UTF_END(m->name);

	for (; utf_ptr < utf_endptr; utf_ptr++, i++) {
		name[i] = *utf_ptr;

		/* escape sequence for '_' is '_1' */

		if (name[i] == '_')
			name[++i] = '1';
	}

	/* close string */

	name[i] = '\0';


	/* try to find the native function symbol */

	sym = lt_dlsym(handle, name);

	if (!sym) {
		/* we didn't find the symbol yet, try to resolve an overloaded        */
		/* function (having the types in it's name)                           */

		name = native_make_overloaded_function(name, m->descriptor);

		/* try to find the overloaded symbol */

		sym = lt_dlsym(handle, name);

		if (!sym) {
			*exceptionptr =
				new_exception_utfmessage(string_java_lang_UnsatisfiedLinkError,
										 m->name);
			return NULL;
		}
	}

	/* release memory */

	dump_release(dumpsize);

	return (functionptr) (ptrint) sym;
}
#endif /* !defined(STATIC_CLASSPATH) */


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


/* native_new_and_init *********************************************************

   Creates a new object on the heap and calls the initializer.
   Returns the object pointer or NULL if memory is exhausted.
			
*******************************************************************************/

java_objectheader *native_new_and_init(classinfo *c)
{
	methodinfo *m;
	java_objectheader *o;

	if (!c)
		return *exceptionptr;

	/* create object */

	o = builtin_new(c);
	
	if (!o)
		return NULL;

	/* find initializer */

	m = class_findmethod(c, utf_init, utf_void__void);
	                      	                      
	/* initializer not found */

	if (!m)
		return o;

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

	/* create object */

	o = builtin_new(c);

	if (!o)
		return NULL;

	/* find initializer */

	m = class_resolveclassmethod(c,
								 utf_init,
								 utf_java_lang_String__void,
								 NULL,
								 true);

	/* initializer not found */

	if (!m)
		return NULL;

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

	/* create object */

	o = builtin_new(c);
	
	if (!o)
		return NULL;

	/* find initializer */

	m = class_resolveclassmethod(c, utf_init, utf_int__void, NULL, true);

	/* initializer not found  */

	if (!m)
		return NULL;

	/* call initializer */

	asm_calljavafunction(m, o, (void *) (ptrint) i, NULL, NULL);

	return o;
}


java_objectheader *native_new_and_init_throwable(classinfo *c, java_lang_Throwable *t)
{
	methodinfo *m;
	java_objectheader *o;

	if (!c)
		return *exceptionptr;

	/* create object */

	o = builtin_new(c);
	
	if (!o)
		return NULL;

	/* find initializer */

	m = class_findmethod(c, utf_init, utf_java_lang_Throwable__void);
	                      	                      
	/* initializer not found */

	if (!m)
		return NULL;

	/* call initializer */

	asm_calljavafunction(m, o, t, NULL, NULL);

	return o;
}


void copy_vftbl(vftbl_t **dest, vftbl_t *src)
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
    buffer = MNEW(char, buffer_size);
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
	        for (utf_ptr--; utf_ptr < UTF_END(c->name); utf_ptr++) {
				*pos++ = *utf_ptr; /* copy text */
			}

	    } else {	   	
			/* check for primitive types */
			for (j = 0; j < PRIMITIVETYPE_COUNT; j++) {
				char *utf_pos	= utf_ptr - 1;
				char *primitive = primitivetype_table[j].wrapname;

				/* compare text */
				while (utf_pos < UTF_END(c->name)) {
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
			for (utf_ptr--; utf_ptr < UTF_END(c->name); utf_ptr++) {
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
    MFREE(buffer, char, buffer_size);

    return result;
}


/* get_parametertypes **********************************************************

   Use the descriptor of a method to generate a java/lang/Class array
   which contains the classes of the parametertypes of the method.

*******************************************************************************/

java_objectarray *get_parametertypes(methodinfo *m) 
{
	methoddesc       *md;
	typedesc         *paramtypes;
	s4                paramcount;
    java_objectarray *result;
	s4                i;

	md = m->parseddesc;

	/* is the descriptor fully parsed? */

	if (!m->parseddesc->params)
		if (!descriptor_params_from_paramtypes(md, m->flags))
			return NULL;

	paramtypes = md->paramtypes;
	paramcount = md->paramcount;

	/* skip `this' pointer */

	if (!(m->flags & ACC_STATIC)) {
		paramtypes++;
		paramcount--;
	}

	/* create class-array */

	result = builtin_anewarray(paramcount, class_java_lang_Class);

    /* get classes */

	for (i = 0; i < paramcount; i++) {
		if (!resolve_class_from_typedesc(&paramtypes[i], false,
										 (classinfo **) &result->data[i]))
			return NULL;

		use_class_as_object((classinfo *) result->data[i]);
	}

	return result;
}


/* get_exceptiontypes **********************************************************

   get the exceptions which can be thrown by a method

*******************************************************************************/

java_objectarray* get_exceptiontypes(methodinfo *m)
{
    u2 excount;
    u2 i;
    java_objectarray *result;
	classinfo *cls;

	excount = m->thrownexceptionscount;

    /* create class-array */
	assert(class_java_lang_Class);

    result = builtin_anewarray(excount, class_java_lang_Class);

    for (i = 0; i < excount; i++) {
		if (!resolve_classref_or_classinfo(NULL,m->thrownexceptions[i],resolveEager,false,&cls))
			return NULL; /* exception */
		use_class_as_object(cls);
		result->data[i] = (java_objectheader *)cls;
    }

    return result;
}


/******************************************************************************************

	get the returntype class of a method

*******************************************************************************************/

classinfo *get_returntype(methodinfo *m) 
{
	classinfo *cls;

	if (!resolve_class_from_typedesc(&(m->parseddesc->returntype),false,&cls))
		return NULL; /* exception */

	use_class_as_object(cls);
	return cls;
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


java_objectarray *builtin_asm_createclasscontextarray(classinfo **end, classinfo **start)
{
	/* XXX platform dependend */

	java_objectarray *tmpArray;
	int i;
	classinfo **current;
	classinfo *c;
	size_t size;

	size = (((size_t) start) - ((size_t) end)) / sizeof(classinfo*);

	/*printf("end %p, start %p, size %ld\n",end,start,size);*/

	if (size > 0) {
		if (*start == class_java_lang_SecurityManager) {
			size--;
			start--;
		}
	}

	tmpArray = builtin_anewarray(size, class_java_lang_Class);

	for(i = 0, current = start; i < size; i++, current--) {
		c = *current;
		/*		printf("%d\n",i);
                utf_display(c->name);*/
		use_class_as_object(c);
		tmpArray->data[i] = (java_objectheader *) c;
	}

	return tmpArray;
}


java_lang_ClassLoader *builtin_asm_getclassloader(classinfo **end, classinfo **start)
{
	/* XXX platform dependend */

	int i;
	classinfo **current;
	classinfo *c;
	size_t size;

	size = (((size_t) start) - ((size_t) end)) / sizeof(classinfo*);

	/*	log_text("builtin_asm_getclassloader");
        printf("end %p, start %p, size %ld\n",end,start,size);*/

	if (size > 0) {
		if (*start == class_java_lang_SecurityManager) {
			size--;
			start--;
		}
	}

	for(i = 0, current = start; i < size; i++, current--) {
		c = *current;

		if (c == class_java_security_PrivilegedAction)
			return NULL;

		if (c->classloader)
			return (java_lang_ClassLoader *) c->classloader;
	}

	return NULL;
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
