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

   $Id: native.c 4212 2006-01-15 14:12:05Z twisti $

*/


#include "config.h"

#include <assert.h>

#if !defined(ENABLE_STATICVM)
# include <ltdl.h>
#endif

#include "vm/types.h"

#include "cacao/cacao.h"
#include "mm/memory.h"
#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_Throwable.h"
#include "toolbox/logging.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/hashtable.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/resolve.h"
#include "vm/stringlocal.h"
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
#include "native/include/java_lang_reflect_Method.h"
#include "native/include/java_lang_reflect_VMProxy.h"
#include "native/include/java_security_VMAccessController.h"

#if defined(ENABLE_STATICVM)

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

#else /* defined(ENABLE_STATICVM) */

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
	(functionptr) Java_java_lang_VMClass_throwException,

	(functionptr) Java_java_lang_VMClassLoader_defineClass,
	(functionptr) Java_java_lang_VMClassLoader_resolveClass,
	(functionptr) Java_java_lang_VMClassLoader_loadClass,
	(functionptr) Java_java_lang_VMClassLoader_getPrimitiveClass,
	(functionptr) Java_java_lang_VMClassLoader_nativeGetResources,
	(functionptr) Java_java_lang_VMClassLoader_findLoadedClass,

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

	(functionptr) Java_java_lang_reflect_Method_getModifiers,
	(functionptr) Java_java_lang_reflect_Method_getReturnType,
	(functionptr) Java_java_lang_reflect_Method_getParameterTypes,
	(functionptr) Java_java_lang_reflect_Method_getExceptionTypes,
	(functionptr) Java_java_lang_reflect_Method_invokeNative,

	(functionptr) Java_java_lang_reflect_VMProxy_getProxyClass,
	(functionptr) Java_java_lang_reflect_VMProxy_getProxyData,
	(functionptr) Java_java_lang_reflect_VMProxy_generateProxyClass,

	(functionptr) Java_java_security_VMAccessController_getStack,
};

#endif /* defined(ENABLE_STATICVM) */


/* tables for methods *********************************************************/

#ifdef ENABLE_STATICVM
#define NATIVETABLESIZE  (sizeof(nativetable)/sizeof(struct nativeref))

/* table for fast string comparison */
static nativecompref nativecomptable[NATIVETABLESIZE];

/* string comparsion table initialized */
static bool nativecompdone = false;
#endif


/* global variables ***********************************************************/

#if !defined(ENABLE_STATICVM)
static hashtable hashtable_library;
static lt_dlhandle mainhandle;
#endif


/* native_loadclasses **********************************************************

   Load classes required for native methods.

*******************************************************************************/

bool native_init(void)
{
#if !defined(ENABLE_STATICVM)
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

	/* get the handle for the main program */

	if (!(mainhandle = lt_dlopen(NULL)))
		return false;

	/* initialize library hashtable, 10 entries should be enough */

	hashtable_create(&hashtable_library, 10);

#endif

	/* everything's ok */

	return true;
}


/* native_hashtable_library_add ************************************************

   Adds an entry to the native library hashtable.

*******************************************************************************/

#if !defined(ENABLE_STATICVM)
void native_hashtable_library_add(utf *filename, java_objectheader *loader,
								  lt_dlhandle handle)
{
	hashtable_library_loader_entry *le;
	hashtable_library_name_entry   *ne; /* library name                       */
	u4   key;                           /* hashkey                            */
	u4   slot;                          /* slot in hashtable                  */

	/* normally addresses are aligned to 4, 8 or 16 bytes */

	key  = ((u4) (ptrint) loader) >> 4;        /* align to 16-byte boundaries */
	slot = key & (hashtable_library.size - 1);
	le   = hashtable_library.ptr[slot];

	/* search external hash chain for the entry */

	while (le) {
		if (le->loader == loader)
			break;

		le = le->hashlink;                  /* next element in external chain */
	}

	/* no loader found? create a new entry */

	if (!le) {
		le = NEW(hashtable_library_loader_entry);

		le->loader = loader;
		le->namelink = NULL;

		/* insert entry into hashtable */

		le->hashlink =
			(hashtable_library_loader_entry *) hashtable_library.ptr[slot];
		hashtable_library.ptr[slot] = le;

		/* update number of hashtable-entries */

		hashtable_library.entries++;
	}


	/* search for library name */

	ne = le->namelink;

	while (ne) {
		if (ne->name == filename)
			return;

		ne = ne->hashlink;                  /* next element in external chain */
	}

	/* not found? add the library name to the classloader */

	ne = NEW(hashtable_library_name_entry);

	ne->name = filename;
	ne->handle = handle;

	/* insert entry into external chain */

	ne->hashlink = le->namelink;
	le->namelink = ne;
}
#endif /* !defined(ENABLE_STATICVM) */


/* native_hashtable_library_find ***********************************************

   Find an entry in the native library hashtable.

*******************************************************************************/

#if !defined(ENABLE_STATICVM)
hashtable_library_name_entry *native_hashtable_library_find(utf *filename,
															java_objectheader *loader)
{
	hashtable_library_loader_entry *le;
	hashtable_library_name_entry   *ne; /* library name                       */
	u4   key;                           /* hashkey                            */
	u4   slot;                          /* slot in hashtable                  */

	/* normally addresses are aligned to 4, 8 or 16 bytes */

	key  = ((u4) (ptrint) loader) >> 4;        /* align to 16-byte boundaries */
	slot = key & (hashtable_library.size - 1);
	le   = hashtable_library.ptr[slot];

	/* search external hash chain for the entry */

	while (le) {
		if (le->loader == loader)
			break;

		le = le->hashlink;                  /* next element in external chain */
	}

	/* no loader found? return NULL */

	if (!le)
		return NULL;

	/* search for library name */

	ne = le->namelink;

	while (ne) {
		if (ne->name == filename)
			return ne;

		ne = ne->hashlink;                  /* next element in external chain */
	}

	/* return entry, if no entry was found, ne is NULL */

	return ne;
}
#endif /* !defined(ENABLE_STATICVM) */


/* native_findfunction *********************************************************

   Looks up a method (must have the same class name, method name,
   descriptor and 'static'ness) and returns a function pointer to it.
   Returns: function pointer or NULL (if there is no such method)

   Remark: For faster operation, the names/descriptors are converted
   from C strings to Unicode the first time this function is called.

*******************************************************************************/

#if defined(ENABLE_STATICVM)
functionptr native_findfunction(utf *cname, utf *mname, utf *desc,
								bool isstatic)
{
	/* entry of table for fast string comparison */
	struct nativecompref *n;
	s4 i;

	isstatic = isstatic ? true : false;
	
	if (!nativecompdone) {
		for (i = 0; i < NATIVETABLESIZE; i++) {
			nativecomptable[i].classname  = 
				utf_new_char(nativetable[i].classname);

			nativecomptable[i].methodname = 
				utf_new_char(nativetable[i].methodname);

			nativecomptable[i].descriptor =
				utf_new_char(nativetable[i].descriptor);

			nativecomptable[i].isstatic   = nativetable[i].isstatic;
			nativecomptable[i].func       = nativetable[i].func;
		}

		nativecompdone = true;
	}

	for (i = 0; i < NATIVETABLESIZE; i++) {
		n = &(nativecomptable[i]);

		if (cname == n->classname && mname == n->methodname &&
		    desc == n->descriptor && isstatic == n->isstatic)
			return n->func;
	}

		
	/* no function was found, throw exception */

	*exceptionptr =
			new_exception_utfmessage(string_java_lang_UnsatisfiedLinkError,
									 mname);

	return NULL;
}
#endif /* defined(ENABLE_STATICVM) */


/* native_make_overloaded_function *********************************************

   XXX

*******************************************************************************/

#if !defined(ENABLE_STATICVM)
static char *native_make_overloaded_function(char *name, utf *desc)
{
	char *newname;
	s4    namelen;
	char *utf_ptr;
	u2    c;
	s4    i;

	utf_ptr = desc->text;
	namelen = strlen(name) + strlen("__") + strlen("0");

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

	newname = DMNEW(char, namelen);
	MCOPY(newname, name, char, i);

	utf_ptr = desc->text;

	newname[i++] = '_';
	newname[i++] = '_';

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
			newname[i++] = c;
			break;
		case '[':
			newname[i++] = '_';
			newname[i++] = '3';
			break;
		case 'L':
			newname[i++] = 'L';
			while ((c = utf_nextu2(&utf_ptr)) != ';')
				if (((c >= 'a') && (c <= 'z')) ||
					((c >= 'A') && (c <= 'Z')) ||
					((c >= '0') && (c <= '9')))
					newname[i++] = c;
				else
					newname[i++] = '_';
			newname[i++] = '_';
			newname[i++] = '2';
			break;
		case '(':
			break;
		default:
			assert(0);
		}
	}

	/* close string */

	newname[i] = '\0';

	return newname;
}


/* native_resolve_function *****************************************************

   Resolves a native function, maybe from a dynamic library.

*******************************************************************************/

functionptr native_resolve_function(methodinfo *m)
{
	lt_ptr                     sym;
	char                      *name;
	char                      *newname;
	s4                         namelen;
	char                      *utf_ptr;
	char                      *utf_endptr;
	s4                         dumpsize;
	hashtable_library_loader_entry *le;
	hashtable_library_name_entry   *ne;
	u4                         key;     /* hashkey                            */
	u4                         slot;    /* slot in hashtable                  */
	u4                         i;


	/* verbose output */

	if (opt_verbosejni) {
		printf("[Dynamic-linking native method ");
		utf_display_classname(m->class->name);
		printf(".");
		utf_display(m->name);
		printf(" ... ");
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


	/* generate overloaded function (having the types in it's name)           */

	newname = native_make_overloaded_function(name, m->descriptor);

	/* check the library hash entries of the classloader of the methods's     */
	/* class                                                                  */

	sym = NULL;

	/* normally addresses are aligned to 4, 8 or 16 bytes */

	key  = ((u4) (ptrint) m->class->classloader) >> 4;    /* align to 16-byte */
	slot = key & (hashtable_library.size - 1);
	le   = hashtable_library.ptr[slot];

	/* iterate through loaders in this hash slot */

	while (le && !sym) {
		/* iterate through names in this loader */

		ne = le->namelink;
			
		while (ne && !sym) {
			sym = lt_dlsym(ne->handle, name);

			if (!sym)
				sym = lt_dlsym(ne->handle, newname);

			ne = ne->hashlink;
		}

		le = le->hashlink;
	}

	if (sym)
		if (opt_verbosejni)
			printf("JNI ]\n");


	/* if not found, try to find the native function symbol in the main       */
	/* program                                                                */

	if (!sym) {
		sym = lt_dlsym(mainhandle, name);

		if (!sym)
			sym = lt_dlsym(mainhandle, newname);

		if (sym)
			if (opt_verbosejni)
				printf("internal ]\n");
	}


	/* no symbol found? throw exception */

	if (!sym)
		*exceptionptr =
			new_exception_utfmessage(string_java_lang_UnsatisfiedLinkError,
									 m->name);

	/* release memory */

	dump_release(dumpsize);

	return (functionptr) (ptrint) sym;
}
#endif /* !defined(ENABLE_STATICVM) */


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

	/* try to find the initializer */

	m = class_findmethod(c, utf_init, utf_void__void);
	                      	                      
	/* ATTENTION: returning the object here is ok, since the class may
       not have an initializer */

	if (!m)
		return o;

	/* call initializer */

	ASM_CALLJAVAFUNCTION(m, o, NULL, NULL, NULL);

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

	ASM_CALLJAVAFUNCTION(m, o, s, NULL, NULL);

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

	ASM_CALLJAVAFUNCTION(m, o, (void *) (ptrint) i, NULL, NULL);

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

	ASM_CALLJAVAFUNCTION(m, o, t, NULL, NULL);

	return o;
}


/* native_get_parametertypes ***************************************************

   Use the descriptor of a method to generate a java/lang/Class array
   which contains the classes of the parametertypes of the method.

*******************************************************************************/

java_objectarray *native_get_parametertypes(methodinfo *m)
{
	methoddesc       *md;
	typedesc         *paramtypes;
	s4                paramcount;
    java_objectarray *oa;
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

	oa = builtin_anewarray(paramcount, class_java_lang_Class);

	if (!oa)
		return NULL;

    /* get classes */

	for (i = 0; i < paramcount; i++)
		if (!resolve_class_from_typedesc(&paramtypes[i], true, false,
										 (classinfo **) &oa->data[i]))
			return NULL;

	return oa;
}


/* native_get_exceptiontypes ***************************************************

   Get the exceptions which can be thrown by a method.

*******************************************************************************/

java_objectarray *native_get_exceptiontypes(methodinfo *m)
{
	java_objectarray *oa;
	classinfo        *c;
	u2                i;

	/* create class-array */

	oa = builtin_anewarray(m->thrownexceptionscount, class_java_lang_Class);

	if (!oa)
		return NULL;

	for (i = 0; i < m->thrownexceptionscount; i++) {
		if (!resolve_classref_or_classinfo(NULL, m->thrownexceptions[i],
										   resolveEager, true, false, &c))
			return NULL;

		oa->data[i] = (java_objectheader *) c;
	}

	return oa;
}


/* native_get_returntype *******************************************************

   Get the returntype class of a method.

*******************************************************************************/

classinfo *native_get_returntype(methodinfo *m)
{
	classinfo *c;

	if (!resolve_class_from_typedesc(&(m->parseddesc->returntype), true, false,
									 &c))
		return NULL;

	return c;
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
