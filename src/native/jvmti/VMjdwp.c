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


   $Id: VMjdwp.c 4944 2006-05-23 15:31:19Z motse $

*/

#include "native/jvmti/jvmti.h"
#include "native/jvmti/cacaodbg.h"
#include "native/jvmti/VMjdwp.h"
#include "vm/loader.h"
#include "vm/exceptions.h"
#include "vm/jit/asmpart.h"

#include <stdlib.h>


static methodinfo *notifymid = NULL;
static classinfo *Jdwpclass = NULL;


#define FINDCLASS(jni_env,class,classname) \
  class = load_class_from_sysloader(utf_new_char(classname)); \
  if (!class) throw_main_exception_exit();
 

#define GETJNIMETHOD(jni_env,class,classname,method,methodname,methodsig) \
  FINDCLASS(jni_env,class,classname) \
  method = class_resolveclassmethod (class,utf_new_char(methodname), \
									 utf_new_char(methodsig),        \
									 class_java_lang_Object,true);   \
  if (!method) throw_main_exception_exit();
 
#define GETJNISTATICMETHOD(jni_env,class,classname,method,methodname,methodsig) \
  FINDCLASS(jni_env,class,classname) \
  method = class_resolveclassmethod (class,utf_new_char(methodname), \
									 utf_new_char(methodsig),        \
									 class_java_lang_Object,true);   \
  if (!method) throw_main_exception_exit();  


static void notify (JNIEnv* env, jobject event){
	log_text("VMjdwp notfiy called");

    if (notifymid == NULL) {
		GETJNISTATICMETHOD(env,Jdwpclass,"gnu/classpath/jdwp/Jdwp",notifymid,
						   "notify","(Lgnu/classpath/jdwp/event/Event;)V");
		
    }
    
	vm_call_method(notifymid, NULL ,event);
	if (*exceptionptr)
		throw_main_exception_exit();

}


static void SingleStep (jvmtiEnv *jvmti_env,
                        JNIEnv* jni_env,
                        jthread thread,
                        jmethodID method,
                        jlocation location) {
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void Breakpoint (jvmtiEnv *jvmti_env,
                        JNIEnv* jni_env,
                        jthread thread,
                        jmethodID method,
                        jlocation location) {
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void FieldAccess    (jvmtiEnv *jvmti_env,
                            JNIEnv* jni_env,
                            jthread thread,
                            jmethodID method,
                            jlocation location,
                            jclass field_klass,
                            jobject object,
                            jfieldID field)
{
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void FieldModification (jvmtiEnv *jvmti_env,
                               JNIEnv* jni_env,
                               jthread thread,
                               jmethodID method,
                               jlocation location,
                               jclass field_klass,
                               jobject object,
                               jfieldID field,
                               char signature_type,
                               jvalue new_value) {
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void FramePop (jvmtiEnv *jvmti_env,
                      JNIEnv* jni_env,
                      jthread thread,
                      jmethodID method,
                      jboolean was_popped_by_exception){
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void MethodEntry (jvmtiEnv *jvmti_env,
                         JNIEnv* jni_env,
                         jthread thread,
                         jmethodID method){
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void MethodExit (jvmtiEnv *jvmti_env,
                        JNIEnv* jni_env,
                        jthread thread,
                        jmethodID method,
                        jboolean was_popped_by_exception,
                        jvalue return_value){
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void NativeMethodBind (jvmtiEnv *jvmti_env,
                              JNIEnv* jni_env,
                              jthread thread,
                              jmethodID method,
                              void* address,
                              void** new_address_ptr){
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void Exception (jvmtiEnv *jvmti_env,
                       JNIEnv* jni_env,
                       jthread thread,
                       jmethodID method,
                       jlocation location,
                       jobject exception,
                       jmethodID catch_method,
                       jlocation catch_location){
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void ExceptionCatch (jvmtiEnv *jvmti_env,
                            JNIEnv* jni_env,
                            jthread thread,
                            jmethodID method,
                            jlocation location,
                            jobject exception){
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void ThreadStart (jvmtiEnv *jvmti_env,
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
}

static void ThreadEnd (jvmtiEnv *jvmti_env,
                       JNIEnv* jni_env,
                       jthread thread){
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void ClassLoad (jvmtiEnv *jvmti_env,
                       JNIEnv* jni_env,
                       jthread thread,
                       jclass klass){
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void ClassPrepare (jvmtiEnv *jvmti_env,
                          JNIEnv* jni_env,
                          jthread thread,
                          jclass klass){
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void ClassFileLoadHook (jvmtiEnv *jvmti_env,
                               JNIEnv* jni_env,
                               jclass class_being_redefined,
                               jobject loader,
                               const char* name,
                               jobject protection_domain,
                               jint class_data_len,
                               const unsigned char* class_data,
                               jint* new_class_data_len,
                               unsigned char** new_class_data){
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void VMStart (jvmtiEnv *jvmti_env,
                     JNIEnv* jni_env) {
  log_text ("JVMTI-Event:VMStart IMPLEMENT ME!!!");
}

static void VMInit (jvmtiEnv *jvmti_env, 
                    JNIEnv* jni_env,
                    jthread thread) {
	classinfo* cl;
	methodinfo* cc;
	java_objectheader* obj;

	log_text ("JVMTI-Event:VMInit");

	GETJNIMETHOD(jni_env,cl,"gnu/classpath/jdwp/event/VmInitEvent",cc,"<init>",
				 "(Ljava/lang/Thread;)V"); 

	fprintf(stderr,"VMjdwp:VMInit: 1\n");

	obj = builtin_new(cl);
	if (!obj) throw_main_exception_exit();

	fprintf(stderr,"VMjdwp:VMInit: thread %p\n",thread);
	fflush(stderr);

	vm_call_method((methodinfo*)cc, obj, thread);

	if (*exceptionptr)
		throw_main_exception_exit();

	notify (jni_env,obj);
}

static void VMDeath (jvmtiEnv *jvmti_env,
                     JNIEnv* jni_env) {
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}


static void CompiledMethodLoad    (jvmtiEnv *jvmti_env,
                                   jmethodID method,
                                   jint code_size,
                                   const void* code_addr,
                                   jint map_length,
                                   const jvmtiAddrLocationMap* map,
                                   const void* compile_info) {
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void CompiledMethodUnload (jvmtiEnv *jvmti_env,
                                  jmethodID method,
                                  const void* code_addr){
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void DynamicCodeGenerated (jvmtiEnv *jvmti_env,
                                  const char* name,
                                  const void* address,
                                  jint length){
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void DataDumpRequest (jvmtiEnv *jvmti_env){
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void MonitorContendedEnter (jvmtiEnv *jvmti_env,
                                   JNIEnv* jni_env,
                                   jthread thread,
                                   jobject object){
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void MonitorContendedEntered (jvmtiEnv *jvmti_env,
                                     JNIEnv* jni_env,
                                     jthread thread,
                                     jobject object){
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void MonitorWait (jvmtiEnv *jvmti_env,
                         JNIEnv* jni_env,
                         jthread thread,
                         jobject object,
                         jlong timeout){
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void MonitorWaited (jvmtiEnv *jvmti_env,
                           JNIEnv* jni_env,
                           jthread thread,
                           jobject object,
                           jboolean timed_out){
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void VMObjectAlloc (jvmtiEnv *jvmti_env,
                           JNIEnv* jni_env,
                           jthread thread,
                           jobject object,
                           jclass object_klass,
                           jlong size){
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void ObjectFree (jvmtiEnv *jvmti_env,
                        jlong tag){
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void GarbageCollectionStart (jvmtiEnv *jvmti_env){
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void GarbageCollectionFinish (jvmtiEnv *jvmti_env){
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}


/* it would be more apropriate to call this function from gnu-cp jdwp */
bool jvmti_VMjdwpInit() {
	int end, i=0;
	jvmtiCapabilities cap;
	jvmtiError e;

	log_text("cacao vm - create new jvmti environment");
	jvmtienv = jvmti_new_environment();

	/* set eventcallbacks */
	if (JVMTI_ERROR_NONE != 
		(*jvmtienv)->SetEventCallbacks(jvmtienv,
							   &jvmti_jdwp_EventCallbacks,
							   sizeof(jvmti_jdwp_EventCallbacks))){
		log_text("unable to setup event callbacks");
		return false;
	}
	
	e = (*jvmtienv)->GetPotentialCapabilities(jvmtienv, &cap);
	if (e == JVMTI_ERROR_NONE) {
		e = (*jvmtienv)->AddCapabilities(jvmtienv, &cap);
	}
	if (e != JVMTI_ERROR_NONE) {
		log_text("error adding jvmti capabilities");
		exit(1);
	}

	end = sizeof(jvmti_jdwp_EventCallbacks) / sizeof(void*);
	for (i = 0; i < end; i++) {
		/* enable standard VM callbacks  */
		if (((void**)&jvmti_jdwp_EventCallbacks)[i] != NULL) {
			e = (*jvmtienv)->SetEventNotificationMode(jvmtienv,
												JVMTI_ENABLE,
												JVMTI_EVENT_START_ENUM+i,
												NULL);

			if (JVMTI_ERROR_NONE != e) {
				log_text("unable to setup event notification mode");
				return false;
			}
		}
	}
	return true;
}
	
	jvmtiEventCallbacks jvmti_jdwp_EventCallbacks = {
    &VMInit,
    &VMDeath,
    &ThreadStart,
    &ThreadEnd,
    &ClassFileLoadHook,
    &ClassLoad,
    &ClassPrepare,
    NULL, /* &VMStart */
    &Exception,
    &ExceptionCatch,
    &SingleStep,
    &FramePop,
    &Breakpoint,
    &FieldAccess,
    &FieldModification,
    &MethodEntry,
    &MethodExit,
    &NativeMethodBind,
    &CompiledMethodLoad,
    &CompiledMethodUnload,
    &DynamicCodeGenerated,
    &DataDumpRequest,
    NULL,
    &MonitorWait,
    &MonitorWaited,
    &MonitorContendedEnter,
    &MonitorContendedEntered,
    NULL,
    NULL,
    NULL,
    NULL,
    &GarbageCollectionStart,
    &GarbageCollectionFinish,
    &ObjectFree,
    &VMObjectAlloc,
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
