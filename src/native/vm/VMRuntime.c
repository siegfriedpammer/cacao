/* nat/Runtime.c - java/lang/Runtime

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

   Authors: Roman Obermaiser

   Changes: Joseph Wenninger
            Christian Thalinger

   $Id: VMRuntime.c 1514 2004-11-17 11:52:46Z twisti $

*/


#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>

#include "config.h"
#include "exceptions.h"
#include "main.h"
#include "jni.h"
#include "builtin.h"
#include "exceptions.h"
#include "loader.h"
#include "native.h"
#include "tables.h"
#include "asmpart.h"
#include "mm/boehm.h"
#include "toolbox/logging.h"
#include "toolbox/memory.h"
#include "nat/java_io_File.h"
#include "nat/java_lang_String.h"
#include "nat/java_lang_Process.h"
#include "nat/java_util_Properties.h"    /* needed for java_lang_VMRuntime.h */
#include "nat/java_lang_VMRuntime.h"

#ifndef STATIC_CLASSPATH
#include <dlfcn.h>
#endif

/* this should work on BSD */
/*
#if defined(__DARWIN__)
#include <sys/sysctl.h>
#endif
*/

#undef JOWENN_DEBUG

/* should we run all finalizers on exit? */
static bool finalizeOnExit = false;

/* temporary property structure */

typedef struct property property;

struct property {
	char     *key;
	char     *value;
	property *next;
};

static property *properties = NULL;


/* create_property *************************************************************

   Create a property entry for a command line property definition.

*******************************************************************************/

void create_property(char *key, char *value)
{
	property *p;

	p = NEW(property);
	p->key = key;
	p->value = value;
	p->next = properties;
	properties = p;
}


/* insert_property *************************************************************

   Used for inserting a property into the system's properties table. Method m
   (usually put) and the properties table must be given.

*******************************************************************************/

static void insert_property(methodinfo *m, java_util_Properties *p, char *key,
							char *value)
{
	asm_calljavafunction(m,
						 p,
						 javastring_new_char(key),
						 javastring_new_char(value),
						 NULL);
}


/*
 * Class:     java_lang_VMRuntime
 * Method:    execInternal
 * Signature: ([Ljava/lang/String;[Ljava/lang/String;Ljava/io/File;)Ljava/lang/Process;
 */
JNIEXPORT java_lang_Process* JNICALL Java_java_lang_VMRuntime_execInternal(JNIEnv *env, jclass clazz, java_objectarray *cmd, java_objectarray *shellenv, java_io_File *workingdir)
{
	log_text("Java_java_lang_Runtime_execInternal called");

	return NULL;
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    exitInternal
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_exit(JNIEnv *env, jclass clazz, s4 par1)
{
	if (finalizeOnExit)
		gc_finalize_all();

	cacao_shutdown(par1);
}


/*
 * Class:     java/lang/Runtime
 * Method:    freeMemory
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_lang_VMRuntime_freeMemory(JNIEnv *env, jclass clazz)
{
	return gc_get_free_bytes();
}


/*
 * Class:     java/lang/Runtime
 * Method:    gc
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_gc(JNIEnv *env, jclass clazz)
{
	gc_call();
}


/*
 * Class:     java/lang/Runtime
 * Method:    runFinalization
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_runFinalization(JNIEnv *env, jclass clazz)
{
	gc_invoke_finalizers();
}


/*
 * Class:     java/lang/Runtime
 * Method:    runFinalizersOnExit
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_runFinalizersOnExit(JNIEnv *env, jclass clazz, s4 value)
{
#ifdef __GNUC__
#warning threading
#endif
	finalizeOnExit = value;
}


/*
 * Class:     java/lang/Runtime
 * Method:    runFinalizationsForExit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_runFinalizationForExit(JNIEnv *env, jclass clazz)
{
/*  	if (finalizeOnExit) { */
/*  		gc_call(); */
	/* gc_finalize_all(); */
/*  	} */
/*  	log_text("Java_java_lang_VMRuntime_runFinalizationForExit called"); */
	/*gc_finalize_all();*/
	/*gc_invoke_finalizers();*/
	/*gc_call();*/
}


/*
 * Class:     java/lang/Runtime
 * Method:    totalMemory
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_lang_VMRuntime_totalMemory(JNIEnv *env, jclass clazz)
{
	return gc_get_heap_size();
}


/*
 * Class:     java/lang/Runtime
 * Method:    traceInstructions
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_traceInstructions(JNIEnv *env, jclass clazz, s4 par1)
{
	/* not supported */
}


/*
 * Class:     java/lang/Runtime
 * Method:    traceMethodCalls
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_traceMethodCalls(JNIEnv *env, jclass clazz, s4 par1)
{
	/* not supported */
}


/*
 * Class:     java_lang_Runtime
 * Method:    availableProcessors
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMRuntime_availableProcessors(JNIEnv *env, jclass clazz)
{
#if defined(_SC_NPROC_ONLN)
	return (s4) sysconf(_SC_NPROC_ONLN);

#elif defined(_SC_NPROCESSORS_ONLN)
	return (s4) sysconf(_SC_NPROCESSORS_ONLN);

#elif defined(__DARWIN__)
	/* this should work in BSD */
	/*
	int ncpu, mib[2], rc;
	size_t len;

	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;
	len = sizeof(ncpu);
	rc = sysctl(mib, 2, &ncpu, &len, NULL, 0);

	return (s4) ncpu;
	*/

	host_basic_info_data_t hinfo;
	mach_msg_type_number_t hinfo_count = HOST_BASIC_INFO_COUNT;
	kern_return_t rc;

	rc = host_info(mach_host_self(), HOST_BASIC_INFO,
				   (host_info_t) &hinfo, &hinfo_count);
 
	if (rc != KERN_SUCCESS) {
		return -1;
	}

    return (s4) hinfo.avail_cpus;

#else
	return 1;
#endif
}


/*
 * Class:     java_lang_Runtime
 * Method:    nativeLoad
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMRuntime_nativeLoad(JNIEnv *env, jclass clazz, java_lang_String *par1)
{
	int retVal=0;

	char *buffer;
	int buffer_len;
	utf *data;

#ifdef JOWENN_DEBUG
	log_text("Java_java_lang_VMRuntime_nativeLoad");
#endif

	data = javastring_toutf(par1, 0);
	
	if (!data) {
		log_text("nativeLoad: Error: empty string");
		return 1;
	}
	
#if JOWENN_DEBUG	
	buffer_len = utf_strlen(data) + 40;

	buffer = MNEW(char, buffer_len);
	strcpy(buffer, "Java_java_lang_VMRuntime_nativeLoad:");
	utf_sprint(buffer + strlen((char *) data), data);
	log_text(buffer);	
        
  
	MFREE(buffer, char, buffer_len);
#endif

#ifndef STATIC_CLASSPATH
	/*here it could be interesting to store the references in a list eg for nicely cleaning up or for certain platforms*/
        if (dlopen(data->text,RTLD_NOW | RTLD_GLOBAL)) {
		log_text("LIBLOADED");
                retVal=1;
        }
#else
	retVal=1;
#endif

	return retVal;
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    nativeGetLibname
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT java_lang_String* JNICALL Java_java_lang_VMRuntime_nativeGetLibname(JNIEnv *env, jclass clazz, java_lang_String *pathname, java_lang_String *libname)
{
	char *buffer;
	int buffer_len;
	utf *u;
	java_lang_String *s;

	if (!libname) {
		*exceptionptr = new_nullpointerexception();
		return NULL;
	}

	u = javastring_toutf(libname, 0);
	
	if (!u) {
		log_text("nativeGetLibName: Error: empty string");
		return 0;;
	}
	
	buffer_len = utf_strlen(u) + 6 /*lib .so */ +1 /*0*/;
	buffer = MNEW(char, buffer_len);

	sprintf(buffer, "lib");
	utf_sprint(buffer + 3, u);
	strcat(buffer, ".so");

#ifdef JOWENN_DEBUG
	log_text("nativeGetLibName:");
	log_text(buffer);
#endif
	
	s = javastring_new_char(buffer);	

	MFREE(buffer, char, buffer_len);

	return s;
}


/*
 * Class:     java_lang_VMRuntime
 * Method:    insertSystemProperties
 * Signature: (Ljava/util/Properties;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_insertSystemProperties(JNIEnv *env, jclass clazz, java_util_Properties *p)
{

#define BUFFERSIZE 200
	methodinfo *m;
	char cwd[BUFFERSIZE];
	char *java_home;
	char *user;
	char *home;
	struct utsname utsnamebuf;

	if (!p) {
		*exceptionptr = new_exception(string_java_lang_NullPointerException);
		return;
	}

	/* get properties from system */

	(void) getcwd(cwd, BUFFERSIZE);
	java_home = getenv("JAVA_HOME");
	user = getenv("USER");
	home = getenv("HOME");
	uname(&utsnamebuf);

	/* search for method to add properties */

	m = class_resolveclassmethod(p->header.vftbl->class,
								 utf_new_char("put"),
								 utf_new_char("(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;"),
								 clazz,
								 true);

	if (!m)
		return;

	insert_property(m, p, "java.version", VERSION);
	insert_property(m, p, "java.vendor", "CACAO Team");
	insert_property(m, p, "java.vendor.url", "http://www.complang.tuwien.ac.at/java/cacao/");
	insert_property(m, p, "java.home", java_home ? java_home : "null");
	insert_property(m, p, "java.vm.specification.version", "1.0");
	insert_property(m, p, "java.vm.specification.vendor", "Sun Microsystems Inc.");
	insert_property(m, p, "java.vm.specification.name", "Java Virtual Machine Specification");
	insert_property(m, p, "java.vm.version", VERSION);
	insert_property(m, p, "java.vm.vendor", "CACAO Team");
	insert_property(m, p, "java.vm.name", "CACAO");
	insert_property(m, p, "java.specification.version", "1.4");
	insert_property(m, p, "java.specification.vendor", "Sun Microsystems Inc.");
	insert_property(m, p, "java.specification.name", "Java Platform API Specification");
	insert_property(m, p, "java.class.version", "48.0");
	insert_property(m, p, "java.class.path", classpath);
#if defined(STATIC_CLASSPATH)
	insert_property(m, p, "java.library.path" , ".");
#else
	insert_property(m, p, "java.library.path" , getenv("LD_LIBRARY_PATH"));
#endif
	insert_property(m, p, "java.io.tmpdir", "/tmp");
	insert_property(m, p, "java.compiler", "cacao.jit");
	insert_property(m, p, "java.ext.dirs", "");
 	insert_property(m, p, "os.name", utsnamebuf.sysname);
	insert_property(m, p, "os.arch", utsnamebuf.machine);
	insert_property(m, p, "os.version", utsnamebuf.release);
	insert_property(m, p, "file.separator", "/");
	/* insert_property(m, p, "file.encoding", "null"); -- this must be set properly */
	insert_property(m, p, "path.separator", ":");
	insert_property(m, p, "line.separator", "\n");
	insert_property(m, p, "user.name", user ? user : "null");
	insert_property(m, p, "user.home", home ? home : "null");
	insert_property(m, p, "user.dir", cwd ? cwd : "null");

#if 0
	/* how do we get them? */
	{ "user.language", "en" },
	{ "user.region", "US" },
	{ "user.country", "US" },
	{ "user.timezone", "Europe/Vienna" },

	/* XXX do we need this one? */
	{ "java.protocol.handler.pkgs", "gnu.java.net.protocol"}
#endif
	insert_property(m,p,"java.protocol.handler.pkgs","gnu.java.net.protocol");

	/* insert properties defined on commandline */

	while (properties) {
		property *tp;

		insert_property(m, p, properties->key, properties->value);

		tp = properties;
		properties = properties->next;
		FREE(tp, property);
	}

	return;
}


/*
 * Class:     java_lang_VMRuntime
 * Method:    maxMemory
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_lang_VMRuntime_maxMemory(JNIEnv *env, jclass clazz)
{
	return gc_get_max_heap_size();
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
