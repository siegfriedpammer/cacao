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

   $Id: VMRuntime.c 1147 2004-06-06 13:20:11Z twisti $

*/


#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include "main.h"
#include "jni.h"
#include "builtin.h"
#include "loader.h"
#include "native.h"
#include "tables.h"
#include "asmpart.h"
#include "mm/boehm.h"
#include "toolbox/logging.h"
#include "toolbox/memory.h"
#include "java_io_File.h"
#include "java_lang_String.h"
#include "java_lang_Process.h"
#include "java_util_Properties.h"    /* required by java_lang_Runtime.h */
#include "java_lang_Runtime.h"


#define JOWENN_DEBUG

/* should we run all finalizers on exit? */
static s4 finalizeOnExit = false;

#define MAXPROPS 100
static int activeprops = 19;  
   
static char *proplist[MAXPROPS][2] = {
	{ "java.class.path", NULL },
	{ "java.home", NULL },
	{ "user.home", NULL },  
	{ "user.name", NULL },
	{ "user.dir",  NULL },
                                
	{ "os.arch", NULL },
	{ "os.name", NULL },
	{ "os.version", NULL },
                                         
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
        if (activeprops >= MAXPROPS) panic("Too many properties defined");
        proplist[activeprops][0] = name;
        proplist[activeprops][1] = value;
        activeprops++;
}
/*
 * Class:     java_lang_Runtime
 * Method:    execInternal
 * Signature: ([Ljava/lang/String;[Ljava/lang/String;Ljava/io/File;)Ljava/lang/Process;
 */
JNIEXPORT java_lang_Process* JNICALL Java_java_lang_Runtime_execInternal(JNIEnv *env, java_lang_Runtime *this, java_objectarray *cmd, java_objectarray *shellenv, java_io_File *workingdir)
{
	log_text("Java_java_lang_Runtime_execInternal called");

	return NULL;
}


/*
 * Class:     java/lang/Runtime
 * Method:    exitInternal
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_lang_Runtime_exitInternal(JNIEnv *env, java_lang_Runtime *this, s4 par1)
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
JNIEXPORT s8 JNICALL Java_java_lang_Runtime_freeMemory(JNIEnv *env, java_lang_Runtime *this)
{
	return gc_get_free_bytes();
}


/*
 * Class:     java/lang/Runtime
 * Method:    gc
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Runtime_gc(JNIEnv *env, java_lang_Runtime *this)
{
	gc_call();
}


/*
 * Class:     java/lang/Runtime
 * Method:    runFinalization
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Runtime_runFinalization(JNIEnv *env, java_lang_Runtime *this)
{
	gc_invoke_finalizers();
}


/*
 * Class:     java/lang/Runtime
 * Method:    runFinalizersOnExit
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_Runtime_runFinalizersOnExitInternal(JNIEnv *env, jclass clazz, s4 par1)
{
	finalizeOnExit = par1;
}


/*
 * Class:     java/lang/Runtime
 * Method:    totalMemory
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_lang_Runtime_totalMemory(JNIEnv *env, java_lang_Runtime *this)
{
	return gc_get_heap_size();
}


/*
 * Class:     java/lang/Runtime
 * Method:    traceInstructions
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_Runtime_traceInstructions(JNIEnv *env, java_lang_Runtime *this, s4 par1)
{
	/* not supported */
}


/*
 * Class:     java/lang/Runtime
 * Method:    traceMethodCalls
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_Runtime_traceMethodCalls(JNIEnv *env, java_lang_Runtime *this, s4 par1)
{
	/* not supported */
}


/*
 * Class:     java_lang_Runtime
 * Method:    availableProcessors
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_Runtime_availableProcessors(JNIEnv *env, java_lang_Runtime *this)
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
JNIEXPORT s4 JNICALL Java_java_lang_Runtime_nativeLoad(JNIEnv *env, java_lang_Runtime *this, java_lang_String *par1)
{
#ifdef JOWENN_DEBUG	
	char *buffer;
	int buffer_len;
	utf *data;
	
	data = javastring_toutf(par1, 0);
	
	if (!data) {
		log_text("nativeLoad: Error: empty string");
		return 1;
	}
	
	buffer_len = utf_strlen(data) + 40;

	  	
	buffer = MNEW(char, buffer_len);

	strcpy(buffer, "Java_java_lang_Runtime_nativeLoad:");
	utf_sprint(buffer + strlen((char *) data), data);
	log_text(buffer);	

	MFREE(buffer, char, buffer_len);
#endif
	log_text("Java_java_lang_Runtime_nativeLoad");

	return 1;
}


/*
 * Class:     java_lang_Runtime
 * Method:    nativeGetLibname
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT java_lang_String* JNICALL Java_java_lang_Runtime_nativeGetLibname(JNIEnv *env, jclass clazz, java_lang_String *par1, java_lang_String *par2)
{
#ifdef JOWENN_DEBUG	
	char *buffer;
	int buffer_len;
	utf *data;
	
	data = javastring_toutf(par2, 0);
	
	if (!data) {
		log_text("nativeGetLibName: Error: empty string");
		return 0;;
	}
	
	buffer_len = utf_strlen(data) + 40;
	
	buffer = MNEW(char, buffer_len);

	strcpy(buffer, "Java_java_lang_Runtime_nativeGetLibname:");
	utf_sprint(buffer + strlen((char *) data), data);
	log_text(buffer);	

	MFREE(buffer, char, buffer_len);
#endif
	log_text("Java_java_lang_Runtime_nativeGetLibname");

	return 0;
}


/*
 * Class:     java_lang_Runtime
 * Method:    insertSystemProperties
 * Signature: (Ljava/util/Properties;)V
 */
JNIEXPORT void JNICALL Java_java_lang_Runtime_insertSystemProperties(JNIEnv *env, jclass clazz, java_util_Properties *p)
{

#define BUFFERSIZE 200
	u4 i;
	methodinfo *m;
	char buffer[BUFFERSIZE];
	struct utsname utsnamebuf;

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

	if (!p) {
		*exceptionptr = new_exception(string_java_lang_NullPointerException);
		return;
	}

	/* search for method to add properties */
	m = class_resolvemethod(p->header.vftbl->class,
							utf_new_char("put"),
							utf_new_char("(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;")
							);

	if (!m) {
		*exceptionptr = new_exception_message(string_java_lang_NoSuchMethodError,
											  "java.lang.Properties.put(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;)");
		return;
	}

	/* add the properties */
	for (i = 0; i < activeprops; i++) {

		if (proplist[i][1] == NULL) proplist[i][1] = "";

		asm_calljavafunction(m,
							 p,
							 javastring_new_char(proplist[i][0]),
							 javastring_new_char(proplist[i][1]),
							 NULL
							 );
	}

	return;
}


/*
 * Class:     java_lang_Runtime
 * Method:    maxMemory
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_lang_Runtime_maxMemory(JNIEnv *env, java_lang_Runtime *this)
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
