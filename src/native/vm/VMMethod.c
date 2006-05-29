/* src/native/vm/VMMethod.c - jdwp->jvmti interface

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

Authors: Samuel Vinson
Martin Platter
         

Changes: 


$Id: $

*/

#include "native/jni.h"
#include "native/include/gnu_classpath_jdwp_VMMethod.h"
#include "native/jvmti/jvmti.h"
#include "native/jvmti/VMjdwp.h"

/*
 * Class:     gnu/classpath/jdwp/VMMethod
 * Method:    getName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_gnu_classpath_jdwp_VMMethod_getName(JNIEnv *env, struct gnu_classpath_jdwp_VMMethod* this) 
{
    jvmtiError err;
    char* errdesc;
    char *name, *signature, *generic;
    jstring stringname;
    
    if (JVMTI_ERROR_NONE != (err= (*jvmtienv)->
                             GetMethodName(jvmtienv, (jmethodID)this->_methodId,
                                           &name, &signature, &generic))) {
        (*jvmtienv)->GetErrorName(jvmtienv,err, &errdesc);
        fprintf(stderr,"jvmti error: %s\n",errdesc);
        fflush(stderr);
        (*jvmtienv)->Deallocate(jvmtienv,(unsigned char*)errdesc);
        return NULL;
    }
    
    stringname = (*env)->NewStringUTF(env,name);
    (*jvmtienv)->Deallocate(jvmtienv,(unsigned char*)name);
    (*jvmtienv)->Deallocate(jvmtienv,(unsigned char*)signature);
    (*jvmtienv)->Deallocate(jvmtienv,(unsigned char*)generic);

    return stringname;
}


/*
 * Class:     gnu/classpath/jdwp/VMMethod
 * Method:    getSignature
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_gnu_classpath_jdwp_VMMethod_getSignature(JNIEnv *env, struct gnu_classpath_jdwp_VMMethod* this) 
{
    jvmtiError err;
    char* errdesc;
    char *name, *signature, *generic;
    struct java_lang_String* stringsignature;
    
    if (JVMTI_ERROR_NONE != (err= (*jvmtienv)->
                             GetMethodName(jvmtienv, (jmethodID)this->_methodId,
                                           &name, &signature, &generic))) {
        (*jvmtienv)->GetErrorName(jvmtienv,err, &errdesc);
        fprintf(stderr,"jvmti error: %s\n",errdesc);
        fflush(stderr);
        (*jvmtienv)->Deallocate(jvmtienv,(unsigned char*)errdesc);
        return NULL;
    }
    
    stringsignature = (*env)->NewStringUTF(env,signature);
    (*jvmtienv)->Deallocate(jvmtienv,(unsigned char*)name);
    (*jvmtienv)->Deallocate(jvmtienv,(unsigned char*)signature);
    (*jvmtienv)->Deallocate(jvmtienv,(unsigned char*)generic);
    
    return stringsignature;
}


/*
 * Class:     gnu/classpath/jdwp/VMMethod
 * Method:    getModifiers
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_gnu_classpath_jdwp_VMMethod_getModifiers(JNIEnv *env, struct gnu_classpath_jdwp_VMMethod* this) 
{
    jvmtiError err;
    char* errdesc;
    jint modifiers;
	
    if (JVMTI_ERROR_NONE != (err= (*jvmtienv)->
                             GetMethodModifiers(jvmtienv, 
                                                (jmethodID) this->_methodId,
                                                &modifiers))) {
        (*jvmtienv)->GetErrorName(jvmtienv,err, &errdesc);
        fprintf(stderr,"jvmti error: %s\n",errdesc);
        fflush(stderr);
        (*jvmtienv)->Deallocate(jvmtienv,(unsigned char*)errdesc);
        return 0;
    }
    
    return modifiers;
}


/*
 * Class:     gnu/classpath/jdwp/VMMethod
 * Method:    getLineTable
 * Signature: ()Lgnu/classpath/jdwp/util/LineTable;
 */
JNIEXPORT struct gnu_classpath_jdwp_util_LineTable* JNICALL Java_gnu_classpath_jdwp_VMMethod_getLineTable(JNIEnv *env, struct gnu_classpath_jdwp_VMMethod* this) 
{
    jclass cl;
    jmethodID m;
    jobject ol;
    jlongArray jlineCI;
    jintArray jlineNum;
    jint count = 0, i;
    int *lineNum;
    long *lineCI;
    jvmtiLineNumberEntry *lne;
    jlocation start,end;
    
    jvmtiError err;
    char* errdesc;

    if (JVMTI_ERROR_NONE != (err= (*jvmtienv)->
                             GetLineNumberTable(jvmtienv, 
                                                (jmethodID)this->_methodId, 
                                                &count, &lne))) {
        (*jvmtienv)->GetErrorName(jvmtienv,err, &errdesc);
        fprintf(stderr,"jvmti error: %s\n",errdesc);
        fflush(stderr);
        (*jvmtienv)->Deallocate(jvmtienv,(unsigned char*)errdesc);
        return NULL;
    }

    cl = (*env)->FindClass(env,"gnu.classpath.jdwp.util.LineTable");
    if (!cl) return NULL;

    m = (*env)->GetMethodID(env, cl, "<init>", "(JJ[I[J)V");
    if (!m) return NULL;
	
    jlineNum = (*env)->NewIntArray(env, count);
    if (!jlineNum) return NULL;
    jlineCI = (*env)->NewLongArray(env, count);
    if (!jlineCI) return NULL;
    lineNum = (*env)->GetIntArrayElements(env, jlineNum, NULL);
    lineCI = (*env)->GetLongArrayElements(env, jlineCI, NULL);
    for (i = 0; i < count; ++i) {
        lineNum[i] = lne[i].line_number;
        lineCI[i] = lne[i].start_location;
    }
    (*env)->ReleaseLongArrayElements(env, jlineCI, lineCI, 0);
    (*env)->ReleaseIntArrayElements(env, jlineNum, lineNum, 0);
    (*jvmtienv)->Deallocate(jvmtienv,lne);

    if (JVMTI_ERROR_NONE != (err= (*jvmtienv)->
                             GetMethodLocation(jvmtienv, 
                                               (jmethodID)this->_methodId, 
                                                &start, &end))) {
        (*jvmtienv)->GetErrorName(jvmtienv,err, &errdesc);
        fprintf(stderr,"jvmti error: %s\n",errdesc);
        fflush(stderr);
        (*jvmtienv)->Deallocate(jvmtienv,(unsigned char*)errdesc);
        return NULL;
    }

    ol = (*env)->NewObject(env, cl, m, start, 
                           end, jlineNum, jlineCI);

    return (struct gnu_classpath_jdwp_util_LineTable*)ol;
 
}


/*
 * Class:     gnu/classpath/jdwp/VMMethod
 * Method:    getVariableTable
 * Signature: ()Lgnu/classpath/jdwp/util/VariableTable;
 */
JNIEXPORT struct gnu_classpath_jdwp_util_VariableTable* JNICALL Java_gnu_classpath_jdwp_VMMethod_getVariableTable(JNIEnv *env, struct gnu_classpath_jdwp_VMMethod* this)
{
    fprintf(stderr,"VMMethod_getVariableTable: IMPLEMENT ME!!!");
    return 0;
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
