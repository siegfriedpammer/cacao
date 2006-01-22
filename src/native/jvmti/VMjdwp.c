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


   $Id: VMjdwp.c 4357 2006-01-22 23:33:38Z twisti $

*/

#include "jvmti.h"
#include "vm/loader.h"
#include "vm/exceptions.h"
#include "vm/jit/asmpart.h"

static jmethodID notifymid = NULL;
static jclass Jdwpclass = NULL;

void notify (jobject event){
	methodinfo *m;
    if (notifymid == NULL) {
        Jdwpclass = 
            load_class_from_sysloader(utf_new_char("gnu.classpath.jdwp.Jdwp"));
        if (!Jdwpclass)
            throw_main_exception_exit();
        
        notifymid = class_resolveclassmethod(
            Jdwpclass,
            utf_new_char("notify"), 
            utf_new_char("(Lgnu.classpath.jdwp.event.Event;)V"),
            class_java_lang_Object,
            true);

        if (!notifymid)
            throw_main_exception_exit();
    }
    
    asm_calljavafunction(m, Jdwpclass, event, NULL, NULL);
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
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
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
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
}

static void VMInit (jvmtiEnv *jvmti_env, 
                    JNIEnv* jni_env,
                    jthread thread) {
  log_text ("JVMTI-Event: IMPLEMENT ME!!!");
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


jvmtiEventCallbacks jvmti_jdwp_EventCallbacks = {
    &VMInit,
    &VMDeath,
    &ThreadStart,
    &ThreadEnd,
    &ClassFileLoadHook,
    &ClassLoad,
    &ClassPrepare,
    &VMStart,
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
