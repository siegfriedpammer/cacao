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

   $Id: VMRuntime.c 1449 2004-11-05 14:08:48Z twisti $

*/


#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
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
#include "nat/java_util_Properties.h"    /* needed for java_lang_Runtime.h */
#include "nat/java_lang_VMRuntime.h"

#include "config.h"
#ifndef STATIC_CLASSPATH
#include <dlfcn.h>
#endif

#undef JOWENN_DEBUG

/* should we run all finalizers on exit? */
static bool finalizeOnExit = false;

#define MAXPROPS 100
#if 0
static int activeprops = 29;


/* insert all properties as defined in VMRuntime.java */

static char *proplist[MAXPROPS][2] = {
	{ "java.version", VERSION },
	{ "java.vendor", "CACAO Team" },
	{ "java.vendor.url", "http://www.complang.tuwien.ac.at/java/cacao/" },
	{ "java.home", NULL },
	{ "java.vm.specification.version", "1.0" },

	{ "java.vm.specification.vendor", "Sun Microsystems Inc." },
	{ "java.vm.specification.name", "Java Virtual Machine Specification" },
	{ "java.vm.version", VERSION },
	{ "java.vm.vendor", "CACAO Team" },
	{ "java.vm.name", "CACAO" },

	{ "java.specification.version", "1.4" },
	{ "java.specification.vendor", "Sun Microsystems Inc." },
	{ "java.specification.name", "Java Platform API Specification" },
	{ "java.class.version", "48.0" },
	{ "java.class.path", NULL },

	{ "java.library.path" , NULL },
	{ "java.io.tmpdir", "/tmp"},
	{ "java.compiler", "cacao.jit" },
	{ "java.ext.dirs", NULL },
	{ "os.name", NULL },

	{ "os.arch", NULL },
	{ "os.version", NULL },
	{ "file.separator", "/" },
	{ "java.library.path", NULL},
	{ "java.class.version", "45.3" },

	{ "java.version", PACKAGE":"VERSION },
	{ "java.vendor", "CACAO Team" },
	{ "java.vendor.url", "http://www.complang.tuwien.ac.at/java/cacao/" },
	{ "java.vm.name", "CACAO"}, 
	{ "java.tmpdir", "/tmp/"},

	{ "java.io.tmpdir", "/tmp/"},
	{ "path.separator", ":" },
	{ "line.separator", "\n" },
	{ "user.name", NULL },
	{ "user.home", NULL },

	{ "user.dir",  NULL },
	{ "user.language", "en" },
	{ "user.region", "US" },
	{ "user.country", "US" },
	{ "user.timezone", "Europe/Vienna" },

	/* XXX do we need this one? */
	{ "java.protocol.handler.pkgs", "gnu.java.net.protocol"}
};
#endif
static int activeprops = 20;  
static char *proplist[MAXPROPS][2] = {
	{ "java.class.path", NULL },
	{ "java.home", NULL },
	{ "user.home", NULL },  
	{ "user.name", NULL },
	{ "user.dir",  NULL },
                                
	{ "os.arch", NULL },
	{ "os.name", NULL },
	{ "os.version", NULL },
        { "java.library.path",NULL},
                                         
	{ "java.class.version", "45.3" },
	{ "java.version", PACKAGE":"VERSION },
	{ "java.vendor", "CACAO Team" },
	{ "java.vendor.url", "http://www.complang.tuwien.ac.at/java/cacao/" },
	{ "java.vm.name", "CACAO"}, 
	{ "java.tmpdir", "/tmp/"},
	{ "java.io.tmpdir", "/tmp/"},

	{ "path.separator", ":" },
	{ "file.separator", "/" },
	{ "line.separator", "\n" },
	{ "java.protocol.handler.pkgs", "gnu.java.net.protocol"}
};

void attach_property(char *name, char *value)
{
	if (activeprops >= MAXPROPS)
		panic("Too many properties defined");

	proplist[activeprops][0] = name;
	proplist[activeprops][1] = value;
	activeprops++;
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
	//	gc_finalize_all();
/*  	} */
/*  	log_text("Java_java_lang_VMRuntime_runFinalizationForExit called"); */
	//gc_finalize_all();
	//gc_invoke_finalizers();
	//gc_call();
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

	log_text("Java_java_lang_VMRuntime_nativeLoad");

	data = javastring_toutf(par1, 0);
	
	if (!data) {
		log_text("nativeLoad: Error: empty string");
		return 1;
	}
	
	buffer_len = utf_strlen(data) + 40;

	  	
	buffer = MNEW(char, buffer_len);
	strcpy(buffer, "Java_java_lang_VMRuntime_nativeLoad:");
	utf_sprint(buffer + strlen((char *) data), data);
	log_text(buffer);	
        
  
	MFREE(buffer, char, buffer_len);


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
 * Class:     java_lang_VMRuntime
 * Method:    nativeGetLibname
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT java_lang_String* JNICALL Java_java_lang_VMRuntime_nativeGetLibname(JNIEnv *env, jclass clazz, java_lang_String *par1, java_lang_String *par2)
{
	char *buffer;
	int buffer_len;
	utf *data;
	java_lang_String *resultString;	
	data = javastring_toutf(par2, 0);
	
	if (!data) {
		log_text("nativeGetLibName: Error: empty string");
		return 0;;
	}
	
	buffer_len = utf_strlen(data) + 6 /*lib .so */ +1 /*0*/;
	buffer = MNEW(char, buffer_len);
	sprintf(buffer,"lib");
	utf_sprint(buffer+3,data);
	strcat(buffer,".so");
#ifdef JOWENN_DEBUG
        log_text("nativeGetLibName:");
	log_text(buffer);
#endif
	
	resultString=javastring_new_char(buffer);	

	MFREE(buffer, char, buffer_len);
	return resultString;
}


/*
 * Class:     java_lang_VMRuntime
 * Method:    insertSystemProperties
 * Signature: (Ljava/util/Properties;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_insertSystemProperties(JNIEnv *env, jclass clazz, java_util_Properties *p)
{

#define BUFFERSIZE 200
	u4 i;
	methodinfo *m;
	char buffer[BUFFERSIZE];
	struct utsname utsnamebuf;

#if 0
	proplist[14][1] = classpath;
	//proplist[1][1] = getenv("JAVA_HOME");

	/* get properties from system */
	uname(&utsnamebuf);
	proplist[19][1] = utsnamebuf.sysname;
	proplist[20][1] = utsnamebuf.machine;
	proplist[21][1] = utsnamebuf.release;

	proplist[25][1] = getenv("USER");
	proplist[26][1] = getenv("HOME");
	proplist[27][1] = getcwd(buffer, BUFFERSIZE);

#if defined(STATIC_CLASSPATH)
	proplist[23][1] = ".";
#else
	proplist[23][1] = getenv("LD_LIBRARY_PATH");
#endif
#endif
	proplist[0][1] = classpath;
	proplist[1][1] = getenv("JAVA_HOME");
	proplist[2][1] = getenv("HOME");
	proplist[3][1] = getenv("USER");
	proplist[4][1] = getcwd(buffer, BUFFERSIZE);

	/* get properties from system */
	uname(&utsnamebuf);
	proplist[5][1] = utsnamebuf.machine;
	proplist[6][1] = utsnamebuf.sysname;
	proplist[7][1] = utsnamebuf.release;

#if defined(STATIC_CLASSPATH)
	proplist[8][1] = ".";
#else
	proplist[8][1] = getenv("LD_LIBRARY_PATH");
#endif

	if (!p) {
		*exceptionptr = new_exception(string_java_lang_NullPointerException);
		return;
	}

	/* search for method to add properties */
	m = class_resolveclassmethod(p->header.vftbl->class,
								 utf_new_char("put"),
								 utf_new_char("(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;"),
								 clazz,
								 true);

	if (!m)
		return;

	/* add the properties */
	for (i = 0; i < activeprops; i++) {
		if (proplist[i][1] == NULL)
			proplist[i][1] = "null";

		asm_calljavafunction(m,
							 p,
							 javastring_new_char(proplist[i][0]),
							 javastring_new_char(proplist[i][1]),
							 NULL);
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
