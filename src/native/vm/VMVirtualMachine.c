/* src/native/vm/VMVirtualMachine.c - jdwp->jvmti interface

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

Authors: Martin Platter

Changes: 


$Id: VMVirtualMachine.c 4357 2006-01-22 23:33:38Z twisti $

*/

#include "toolbox/logging.h"
#include "native/jni.h"
#include "native/include/java_lang_Thread.h"
#include "native/include/java_nio_ByteBuffer.h"
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_ClassLoader.h"
#include "native/include/java_lang_reflect_Method.h"
#include "native/include/gnu_classpath_jdwp_event_EventRequest.h"
#include "native/include/gnu_classpath_jdwp_VMVirtualMachine.h"
#include "native/jvmti/jvmti.h"


/*
 * Class:     gnu_classpath_jdwp_VMVirtualMachine
 * Method:    suspendThread
 * Signature: (Ljava/lang/Thread;)V
 */
JNIEXPORT void JNICALL Java_gnu_classpath_jdwp_VMVirtualMachine_suspendThread(JNIEnv *env, jclass clazz, struct java_lang_Thread* par1)
{
    remotedbgjvmtienv->SuspendThread(remotedbgjvmtienv, (jthread) par1);
}

/*
 * Class:     gnu_classpath_jdwp_VMVirtualMachine
 * Method:    resumeThread
 * Signature: (Ljava/lang/Thread;)V
 */
JNIEXPORT void JNICALL Java_gnu_classpath_jdwp_VMVirtualMachine_resumeThread(JNIEnv *env, jclass clazz, struct java_lang_Thread* par1)
{
    remotedbgjvmtienv->ResumeThread(remotedbgjvmtienv, (jthread) par1);
}


/*
 * Class:     gnu_classpath_jdwp_VMVirtualMachine
 * Method:    getSuspendCount
 * Signature: (Ljava/lang/Thread;)I
 */
JNIEXPORT s4 JNICALL Java_gnu_classpath_jdwp_VMVirtualMachine_getSuspendCount(JNIEnv *env, jclass clazz, struct java_lang_Thread* par1) {
    log_text ("JVMTI-Call: IMPLEMENT ME!!!");
	return 0;
}

/*
 * Class:     gnu_classpath_jdwp_VMVirtualMachine
 * Method:    getAllLoadedClassesCount
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_gnu_classpath_jdwp_VMVirtualMachine_getAllLoadedClassesCount(JNIEnv *env, jclass clazz) {
    jint count;
    jclass* classes;

    remotedbgjvmtienv->GetLoadedClasses(remotedbgjvmtienv, &count, &classes);
    return count;
}

/* Class:     gnu/classpath/jdwp/VMVirtualMachine
 * Method:    getClassStatus
 * Signature: (Ljava/lang/Class;)I
 */
JNIEXPORT s4 JNICALL Java_gnu_classpath_jdwp_VMVirtualMachine_getClassStatus(JNIEnv *env, jclass clazz, struct java_lang_Class* par1) {
    log_text ("JVMTI-Call: IMPLEMENT ME!!!");
	return 0;
}


/*
 * Class:     gnu/classpath/jdwp/VMVirtualMachine
 * Method:    getFrames
 * Signature: (Ljava/lang/Thread;II)Ljava/util/ArrayList;
 */
JNIEXPORT struct java_util_ArrayList* JNICALL Java_gnu_classpath_jdwp_VMVirtualMachine_getFrames(JNIEnv *env, jclass clazz, struct java_lang_Thread* par1, s4 par2, s4 par3) {
    log_text ("JVMTI-Call: IMPLEMENT ME!!!");
	return 0;
}


/*
 * Class:     gnu/classpath/jdwp/VMVirtualMachine
 * Method:    getFrame
 * Signature: (Ljava/lang/Thread;Ljava/nio/ByteBuffer;)Lgnu/classpath/jdwp/VMFrame;
 */
JNIEXPORT struct gnu_classpath_jdwp_VMFrame* JNICALL Java_gnu_classpath_jdwp_VMVirtualMachine_getFrame(JNIEnv *env, jclass clazz, struct java_lang_Thread* par1, struct java_nio_ByteBuffer* par2) {
    log_text ("JVMTI-Call: IMPLEMENT ME!!!");
	return 0;
}


/*
 * Class:     gnu/classpath/jdwp/VMVirtualMachine
 * Method:    getFrameCount
 * Signature: (Ljava/lang/Thread;)I
 */
JNIEXPORT s4 JNICALL Java_gnu_classpath_jdwp_VMVirtualMachine_getFrameCount(JNIEnv *env, jclass clazz, struct java_lang_Thread* par1) {
    log_text ("JVMTI-Call: IMPLEMENT ME!!!");
	return 0;
}


/*
 * Class:     gnu/classpath/jdwp/VMVirtualMachine
 * Method:    getThreadStatus
 * Signature: (Ljava/lang/Thread;)I
 */
JNIEXPORT s4 JNICALL Java_gnu_classpath_jdwp_VMVirtualMachine_getThreadStatus(JNIEnv *env, jclass clazz, struct java_lang_Thread* par1) {
    log_text ("JVMTI-Call: IMPLEMENT ME!!!");
	return 0;
}


/*
 * Class:     gnu/classpath/jdwp/VMVirtualMachine
 * Method:    getLoadRequests
 * Signature: (Ljava/lang/ClassLoader;)Ljava/util/ArrayList;
 */
JNIEXPORT struct java_util_ArrayList* JNICALL Java_gnu_classpath_jdwp_VMVirtualMachine_getLoadRequests(JNIEnv *env, jclass clazz, struct java_lang_ClassLoader* par1) {
    log_text ("JVMTI-Call: IMPLEMENT ME!!!");
	return 0;
}


/*
 * Class:     gnu/classpath/jdwp/VMVirtualMachine
 * Method:    executeMethod
 * Signature: (Ljava/lang/Object;Ljava/lang/Thread;Ljava/lang/Class;Ljava/lang/reflect/Method;[Ljava/lang/Object;Z)Lgnu/classpath/jdwp/util/MethodResult;
 */
JNIEXPORT struct gnu_classpath_jdwp_util_MethodResult* JNICALL Java_gnu_classpath_jdwp_VMVirtualMachine_executeMethod(JNIEnv *env, jclass clazz, struct java_lang_Object* par1, struct java_lang_Thread* par2, struct java_lang_Class* par3, struct java_lang_reflect_Method* par4, java_objectarray* par5, s4 par6) {
    log_text ("JVMTI-Call: IMPLEMENT ME!!!");
	return 0;
}


/*
 * Class:     gnu/classpath/jdwp/VMVirtualMachine
 * Method:    getVarTable
 * Signature: (Ljava/lang/Class;Ljava/lang/reflect/Method;)Lgnu/classpath/jdwp/util/VariableTable;
 */
JNIEXPORT struct gnu_classpath_jdwp_util_VariableTable* JNICALL Java_gnu_classpath_jdwp_VMVirtualMachine_getVarTable(JNIEnv *env, jclass clazz, struct java_lang_Class* par1, struct java_lang_reflect_Method* par2) {
    log_text ("JVMTI-Call: IMPLEMENT ME!!!");
	return 0;
}


/*
 * Class:     gnu/classpath/jdwp/VMVirtualMachine
 * Method:    getLineTable
 * Signature: (Ljava/lang/Class;Ljava/lang/reflect/Method;)Lgnu/classpath/jdwp/util/LineTable;
 */
JNIEXPORT struct gnu_classpath_jdwp_util_LineTable* JNICALL Java_gnu_classpath_jdwp_VMVirtualMachine_getLineTable(JNIEnv *env, jclass clazz, struct java_lang_Class* par1, struct java_lang_reflect_Method* par2) {
    log_text ("JVMTI-Call: IMPLEMENT ME!!!");
	return 0;
}


/*
 * Class:     gnu/classpath/jdwp/VMVirtualMachine
 * Method:    getSourceFile
 * Signature: (Ljava/lang/Class;)Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_gnu_classpath_jdwp_VMVirtualMachine_getSourceFile(JNIEnv *env, jclass clazz, struct java_lang_Class* par1) {
    log_text ("JVMTI-Call: IMPLEMENT ME!!!");
	return 0;
}


/*
 * Class:     gnu/classpath/jdwp/VMVirtualMachine
 * Method:    registerEvent
 * Signature: (Lgnu/classpath/jdwp/event/EventRequest;)V
 */
JNIEXPORT void JNICALL Java_gnu_classpath_jdwp_VMVirtualMachine_registerEvent(JNIEnv *env, jclass clazz, struct gnu_classpath_jdwp_event_EventRequest* par1) {
    log_text ("JVMTI-Call: IMPLEMENT ME!!!");
}


/*
 * Class:     gnu/classpath/jdwp/VMVirtualMachine
 * Method:    unregisterEvent
 * Signature: (Lgnu/classpath/jdwp/event/EventRequest;)V
 */
JNIEXPORT void JNICALL Java_gnu_classpath_jdwp_VMVirtualMachine_unregisterEvent(JNIEnv *env, jclass clazz, struct gnu_classpath_jdwp_event_EventRequest* par1) {
    log_text ("JVMTI-Call: IMPLEMENT ME!!!");
}


/*
 * Class:     gnu/classpath/jdwp/VMVirtualMachine
 * Method:    clearEvents
 * Signature: (B)V
 */
JNIEXPORT void JNICALL Java_gnu_classpath_jdwp_VMVirtualMachine_clearEvents(JNIEnv *env, jclass clazz, s4 par1) {
    log_text ("JVMTI-Call: IMPLEMENT ME!!!");
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
