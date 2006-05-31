/* src/native/vm/VMjdwp.c - jvmti->jdwp interface

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Contact: cacao@cacaojvm.org

   Author: Martin Platter

   Changes:             


   $Id: VMjdwp.c 4996 2006-05-31 13:53:16Z motse $

*/

#include "native/jvmti/jvmti.h"
#include "native/jvmti/VMjdwp.h"

#include <stdlib.h>
#include <string.h>

void printjvmtierror(char *desc, jvmtiError err) {
    char* errdesc;
	
	if (err == JVMTI_ERROR_NONE) return;
	(*jvmtienv)->GetErrorName(jvmtienv,err, &errdesc);
	fprintf(stderr,"%s: jvmti error %s\n",desc, errdesc);
	fflush(stderr);
	(*jvmtienv)->Deallocate(jvmtienv,(unsigned char*)errdesc);
}



static jmethodID notifymid = NULL;
static jclass Jdwpclass = NULL;

static void notify (JNIEnv* jni_env, jobject event){
	fprintf(stderr,"VMjdwp notfiy called");

    if (notifymid == NULL) {
		notifymid = (*jni_env)->
			GetStaticMethodID(jni_env,Jdwpclass,
							  "notify","(Lgnu/classpath/jdwp/event/Event;)V");
			if ((*jni_env)->ExceptionOccurred(jni_env) != NULL) {
				fprintf(stderr,"could not get notify method\n");
				(*jni_env)->ExceptionDescribe(jni_env);
				exit(1); 
			}
    }
    
	(*jni_env)->CallStaticVoidMethod(jni_env,Jdwpclass,notifymid,event);
    if ((*jni_env)->ExceptionOccurred(jni_env) != NULL) {
        fprintf(stderr,"Exception occourred in notify mehtod\n");
		(*jni_env)->ExceptionDescribe(jni_env);
		exit(1); 
	}

}

/*static void ThreadStart (jvmtiEnv *jvmti_env,
                         JNIEnv* jni_env,
                         jthread thread){
	jclass cl;
	jmethodID cc;
	jobject obj;

	GETJNIMETHOD(jni_env,cl,"gnu/classpath/jdwp/event/ThreadStartEvent",cc,"<init>","(Ljava/lang/Thread;)V"); 

	obj = builtin_new(cl);
	if (!obj) throw_main_exception_exit();

	fprintf(stderr,"VMjdwp:ThreadStart: thread %p\n",thread);
	fflush(stderr);

	vm_call_method((methodinfo*)cc, obj, thread);

	if (*exceptionptr)
		throw_main_exception_exit();

	notify (jni_env,obj);
	} */


/* setup_jdwp_thread **********************************************************

   Helper function to start JDWP listening thread

*******************************************************************************/

static void setup_jdwp_thread(JNIEnv* jni_env) {
	jobject o;
	jmethodID m;
	jstring  s;

	/* new gnu.classpath.jdwp.Jdwp() */
    Jdwpclass = (*jni_env)->FindClass(jni_env, "gnu/classpath/jdwp/Jdwp"); 
    if ((*jni_env)->ExceptionOccurred(jni_env) != NULL) {
        fprintf(stderr,"could not find gnu/classpath/jdwp/Jdwp\n");
		(*jni_env)->ExceptionDescribe(jni_env);
		exit(1); 
	}

	
	m = (*jni_env)->GetMethodID(jni_env,Jdwpclass,"<init>","()V");
    if ((*jni_env)->ExceptionOccurred(jni_env) != NULL) {
        fprintf(stderr,"could not get Jdwp constructor\n");
		(*jni_env)->ExceptionDescribe(jni_env);
		exit(1); 
	}
	
	o = (*jni_env)->NewObject(jni_env, Jdwpclass, m);
    if ((*jni_env)->ExceptionOccurred(jni_env) != NULL) {
        fprintf(stderr,"error calling Jdwp constructor\n");
		(*jni_env)->ExceptionDescribe(jni_env);
		exit(1); 
	}
	
	
	/* configure(jdwpoptions) */
	m = (*jni_env)->GetMethodID(jni_env,Jdwpclass,"configure",
								"(Ljava/lang/String;)V");
    if ((*jni_env)->ExceptionOccurred(jni_env) != NULL) {
        fprintf(stderr,"could not get Jdwp configure method\n");
		(*jni_env)->ExceptionDescribe(jni_env);
		exit(1); 
	}


	s = (*jni_env)->NewStringUTF(jni_env,jdwpoptions);
    if (s == NULL) {
        fprintf(stderr,"could not get new java string from jdwp options\n");
		exit(1); 
	}

	free(jdwpoptions);
	
	(*jni_env)->CallVoidMethod(jni_env,o,m,s);
    if ((*jni_env)->ExceptionOccurred(jni_env) != NULL) {
        fprintf(stderr,"Exception occourred in Jdwp configure\n");
		(*jni_env)->ExceptionDescribe(jni_env);
		exit(1); 
	}

	m = (*jni_env)->GetMethodID(jni_env,Jdwpclass,"_doInitialization","()V");
    if ((*jni_env)->ExceptionOccurred(jni_env) != NULL) {
        fprintf(stderr,"could not get Jdwp _doInitialization method\n");
		(*jni_env)->ExceptionDescribe(jni_env);
		exit(1); 
	}


	(*jni_env)->CallVoidMethod(jni_env,o,m);
    if ((*jni_env)->ExceptionOccurred(jni_env) != NULL) {
        fprintf(stderr,"Exception occourred in Jdwp _doInitialization\n");
		(*jni_env)->ExceptionDescribe(jni_env);
		exit(1); 
	}
}


static void VMInit (jvmtiEnv *jvmti_env, 
                    JNIEnv* jni_env,
                    jthread thread) {
	jclass cl;
	jmethodID m;
	jobject eventobj;
	jvmtiError err; 

	fprintf(stderr,"JDWP VMInit\n");

	/* startup gnu classpath jdwp thread */
	setup_jdwp_thread(jni_env);

	fprintf(stderr,"JDWP listening thread started\n");

    cl = (*jni_env)->FindClass(jni_env, 
									  "gnu/classpath/jdwp/event/VmInitEvent");
    if ((*jni_env)->ExceptionOccurred(jni_env) != NULL) {
        fprintf(stderr,"could not find class VMInitEvent\n");
		(*jni_env)->ExceptionDescribe(jni_env);
		exit(1); 
	}

	m = (*jni_env)->GetMethodID(jni_env,cl,"<init>",
								"(Ljava/lang/Thread;)V");
    if ((*jni_env)->ExceptionOccurred(jni_env) != NULL) {
        fprintf(stderr,"could not get VmInitEvent constructor\n");
		(*jni_env)->ExceptionDescribe(jni_env);
		exit(1); 
	}

	eventobj = (*jni_env)->NewObject(jni_env, cl, m, thread);
    if ((*jni_env)->ExceptionOccurred(jni_env) != NULL) {
        fprintf(stderr,"error calling VmInitEvent constructor\n");
		(*jni_env)->ExceptionDescribe(jni_env);
		exit(1); 
	}


	notify (jni_env,eventobj);

	if (suspend) {
		fprintf(stderr,"suspend initial thread\n");
		err = (*jvmti_env)->SuspendThread(jvmti_env,thread);
		printjvmtierror("error suspending initial thread",err);
	}
}

static void VMDeath (jvmtiEnv *jvmti_env,
                     JNIEnv* jni_env) {
  fprintf(stderr,"JVMTI-Event: IMPLEMENT ME!!!");
}

static void usage() {	
	puts("usage jdwp:[help]|(<option>=<value>),*");
	puts("   transport=[dt_socket|...]");
	puts("   address=<hostname:port>");
	puts("   server=[y|n]");
	puts("   suspend=[y|n]");
}

static bool processoptions(char *options) {
	int i,len;
	
	if (strncmp(options,"help",4) == 0) {
		usage();
		return false;
	}

	suspend = true; 	/* default value */


	/* copy options for later use in java jdwp listen thread configure */
	jdwpoptions = malloc(sizeof(char)*strlen(options));
	strncpy(jdwpoptions, options, sizeof(char)*strlen(options));

	len = strlen(options);
	
	i=0;
	while (i<len) {
		if (strncmp("suspend=",&options[i],8)==0) {
			if (8>=strlen(&options[i])) {
				if ((options[i+8]== 'y') || (options[i+8]== 'n')) {
					suspend = options[i+8]== 'y';
				} else {
					printf("jdwp error argument: %s\n",options);
					usage();
					return -1;
				}
			}
		} else {
			/* these options will be handled by jdwp java configure */
			if ((strncmp("transport=",options,10)==0) ||
				(strncmp("server=",options,7)==0)) {
			} else {
				printf("jdwp unkown argument: %s\n",options);
				usage();
				return false;
			}
		}
		while ((options[i]!=',')&&(i<len)) i++;
		i++;
	}
	return true;	
}


JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) { 
	jint rc;
	int end, i=0;
	jvmtiCapabilities cap;
	jvmtiError e;


	fprintf(stderr,"jdwp Agent_OnLoad options: %s\n",options);
	if (!processoptions(options)) return -1;
	
	rc = (*vm)->GetEnv(vm, (void**)&jvmtienv, JVMTI_VERSION_1_0);
	if (rc != JNI_OK) {         
		fprintf(stderr, "jdwp: Unable to get jvmtiEnv error=%d\n", rc);
		return -1;		
	}
	
	/* set eventcallbacks */
	if (JVMTI_ERROR_NONE != 
		(e = (*jvmtienv)->SetEventCallbacks(jvmtienv,
									   &jvmti_jdwp_EventCallbacks,
									   sizeof(jvmtiEventCallbacks)))){
		printjvmtierror("jdwp: unable to setup event callbacks", e);
		return -1;
	}

	e = (*jvmtienv)->GetPotentialCapabilities(jvmtienv, &cap);
	printjvmtierror("jdwp: unable to get potential capabilities", e);
	if (e == JVMTI_ERROR_NONE) 
		e = (*jvmtienv)->AddCapabilities(jvmtienv, &cap);
	if (e != JVMTI_ERROR_NONE) {
		printjvmtierror("jdwp: error adding jvmti capabilities", e);
		return -1;
	}
	

	end = sizeof(jvmtiEventCallbacks) / sizeof(void*);
	for (i = 0; i < end; i++) {
		/* enable VM callbacks  */
		if (((void**)&jvmti_jdwp_EventCallbacks)[i] != NULL) {
			e = (*jvmtienv)->SetEventNotificationMode(jvmtienv,
													  JVMTI_ENABLE,
													  JVMTI_EVENT_START_ENUM+i,
													  NULL);
			
			if (JVMTI_ERROR_NONE != e) {
				printjvmtierror("jdwp: unable to setup event notification mode",e);
				return -1;
			}
		}
	}

	return 0;
}
	

jvmtiEventCallbacks jvmti_jdwp_EventCallbacks = {
    &VMInit,
    &VMDeath,
    NULL, /*    &ThreadStart,*/
    NULL, /*    &ThreadEnd, */
    NULL, /* &ClassFileLoadHook, */
    NULL, /* &ClassLoad, */
    NULL, /* &ClassPrepare,*/
    NULL, /* &VMStart */
    NULL, /* &Exception, */
    NULL, /* &ExceptionCatch, */
    NULL, /* &SingleStep, */
    NULL, /* &FramePop, */
    NULL, /* &Breakpoint, */
    NULL, /* &FieldAccess, */
    NULL, /* &FieldModification, */
    NULL, /* &MethodEntry, */
    NULL, /* &MethodExit, */
    NULL, /* &NativeMethodBind, */
    NULL, /* &CompiledMethodLoad, */
    NULL, /* &CompiledMethodUnload, */
    NULL, /* &DynamicCodeGenerated, */
    NULL, /* &DataDumpRequest, */
    NULL,
    NULL, /* &MonitorWait, */
    NULL, /* &MonitorWaited, */
    NULL, /* &MonitorContendedEnter, */
    NULL, /* &MonitorContendedEntered, */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, /* &GarbageCollectionStart, */
    NULL, /* &GarbageCollectionFinish, */
    NULL, /* &ObjectFree, */
    NULL, /* &VMObjectAlloc, */
};


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
