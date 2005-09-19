/* native/jni.h - JNI types and data structures

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

   Changes: Christian Thalinger

   $Id: jni.h 3221 2005-09-19 13:31:31Z twisti $

*/


#ifndef _JNI_H
#define _JNI_H

#include <stdio.h>
#include <stdarg.h>

#include "vm/types.h"

#include "vm/field.h"
#include "vm/global.h"
#include "vm/method.h"


/* JNI versions */

#define JNI_VERSION_1_1    0x00010001
#define JNI_VERSION_1_2    0x00010002
#define JNI_VERSION_1_4    0x00010004

#define JNIEXPORT
#define JNICALL


/* JNI-Boolean */

#define JNI_FALSE        0
#define JNI_TRUE         1


/* Error codes */

#define JNI_OK           0
#define JNI_ERR          (-1)
#define JNI_EDETACHED    (-2)              /* thread detached from the VM */
#define JNI_EVERSION     (-3)              /* JNI version error */


/* Release<PrimitiveType>ArrayElements modes */

#define JNI_COMMIT       1
#define JNI_ABORT        2


/* JNI datatypes */

#define jboolean        u1
#define jbyte           s1
#define jchar           u2
#define jshort          s2
#define jint            s4
#define jlong           s8
#define jfloat          float
#define jdouble         double

#define jobject         java_objectheader*
#define jclass          struct classinfo*
#define jthrowable      jobject
#define jweak           jobject
#define jarray          java_arrayheader*
#define jlongArray      java_longarray*
#define jbooleanArray   java_booleanarray*
#define jintArray       java_intarray*
#define jcharArray      java_chararray*
#define jbyteArray      java_bytearray*
#define jshortArray     java_shortarray*
#define jdoubleArray    java_doublearray*
#define jfloatArray     java_floatarray*
#define jobjectArray    java_objectarray*
#define jstring         jobject

#define jsize           jint
#define jfieldID        fieldinfo*
#define jmethodID       methodinfo*	


typedef union jvalue {
    jboolean z;
    jbyte    b;
    jchar    c;
    jshort   s;
    jint     i;
    jlong    j;
    jfloat   f;
    jdouble  d;
    jobject  l;
} jvalue;


typedef struct JDK1_1InitArgs JDK1_1InitArgs;

struct JDK1_1InitArgs {
	/* The first two fields were reserved in JDK 1.1, and
	   formally introduced in JDK 1.1.2. */
	/* Java VM version */
	jint version;

	/* System properties. */
	char **properties;

	/* whether to check the Java source files are newer than
	 * compiled class files. */
	jint checkSource;

	/* maximum native stack size of Java-created threads. */
	jint nativeStackSize;

	/* maximum Java stack size. */
	jint javaStackSize;

	/* initial heap size. */
	jint minHeapSize;

	/* maximum heap size. */
	jint maxHeapSize;

	/* controls whether Java byte code should be verified:
	 * 0 -- none, 1 -- remotely loaded code, 2 -- all code. */ 
	jint verifyMode;

	/* the local directory path for class loading. */
	const char *classpath;

	/* a hook for a function that redirects all VM messages. */
	jint (*vfprintf)(FILE *fp, const char *format,
					 va_list args);

	/* a VM exit hook. */
	void (*exit)(jint code);

	/* a VM abort hook. */
	void (*abort)();

	/* whether to enable class GC. */
	jint enableClassGC;

	/* whether GC messages will appear. */
	jint enableVerboseGC;

	/* whether asynchronous GC is allowed. */
	jint disableAsyncGC;

	/* Three reserved fields. */
	jint reserved0;
	jint reserved1;
	jint reserved2;
};


typedef struct JDK1_1AttachArgs {
	/*
	 * JDK 1.1 does not need any arguments to attach a
	 * native thread. The padding is here to satisfy the C
	 * compiler which does not permit empty structures.
	 */
	void *__padding;
} JDK1_1AttachArgs;


typedef struct JNIInvokeInterface *JavaVM;
/*  typedef struct _JavaVM *JavaVM; */

/*  struct _JavaVM { */
struct JNIInvokeInterface {
   void *(*reserved0) ();
   void *(*reserved1) ();
   void *(*reserved2) ();
   jint (*DestroyJavaVM) (JavaVM *);
   jint (*AttachCurrentThread) (JavaVM *, void **, void *);
   jint (*DetachCurrentThread) (JavaVM *);
   jint (*GetEnv) (JavaVM *, void **, jint);
   jint (*AttachCurrentThreadAsDaemon) (JavaVM *, void **, void *);
};


/* native method name, signature and function pointer for use in RegisterNatives */

typedef struct {
    char *name;
    char *signature;
    void *fnPtr;
} JNINativeMethod;


/* 
	JNI function table, contains functions for the following usages:	
	
	Version Information 
	Class Operations 
	Exceptions 
	Global and Local References 
	Object Operations 
	Accessing Fields of Objects 
	Calling Instance Methods 
	Accessing Static Fields 
	Calling Static Methods 
	String Operations 
	Array Operations 
	Registering Native Methods 
	Monitor Operations 
	Java VM Interface 
*/

typedef struct JNINativeInterface *JNIEnv;
/*  typedef struct JNI_Table *JNIEnv; */

/*  struct JNI_Table { */
struct JNINativeInterface {
    /* reserverd for future JNI-functions */
    void *unused0;
    void *unused1;
    void *unused2;
    void *unused3;
    
    /* version information */
    
    jint (*GetVersion) (JNIEnv*);

    /* class operations */

    jclass (*DefineClass) (JNIEnv*, const char *name, jobject loader, const jbyte *buf, jsize len);
    jclass (*FindClass) (JNIEnv*, const char *name);

    jmethodID (*FromReflectedMethod) (JNIEnv*, jobject method);
    jfieldID (*FromReflectedField) (JNIEnv*, jobject field);

    jobject (*ToReflectedMethod) (JNIEnv*, jclass cls, jmethodID methodID, jboolean isStatic);

    jclass (*GetSuperclass) (JNIEnv*, jclass sub);
    jboolean (*IsAssignableFrom) (JNIEnv*, jclass sub, jclass sup);

    jobject (*ToReflectedField) (JNIEnv*, jclass cls, jfieldID fieldID, jboolean isStatic);

    /* exceptions */ 

    jint (*Throw) (JNIEnv*, jthrowable obj);
    jint (*ThrowNew) (JNIEnv*, jclass clazz, const char *msg);
    jthrowable (*ExceptionOccurred) (JNIEnv*);
    void (*ExceptionDescribe) (JNIEnv*);
    void (*ExceptionClear) (JNIEnv*);
    void (*FatalError) (JNIEnv*, const char *msg);

    /* global and local references */

    jint (*PushLocalFrame) (JNIEnv*, jint capacity);
    jobject (*PopLocalFrame) (JNIEnv*, jobject result);
    
    jobject (*NewGlobalRef) (JNIEnv*, jobject lobj);
    void (*DeleteGlobalRef) (JNIEnv*, jobject gref);
    void (*DeleteLocalRef) (JNIEnv*, jobject obj);
    jboolean (*IsSameObject) (JNIEnv*, jobject obj1, jobject obj2);
    jobject (*NewLocalRef) (JNIEnv*, jobject ref);
    jint (*EnsureLocalCapacity) (JNIEnv*, jint capacity);

    /* object operations */ 

    jobject (*AllocObject) (JNIEnv*, jclass clazz);
    jobject (*NewObject) (JNIEnv*, jclass clazz, jmethodID methodID, ...);
    jobject (*NewObjectV) (JNIEnv*, jclass clazz, jmethodID methodID, va_list args);
    jobject (*NewObjectA) (JNIEnv*, jclass clazz, jmethodID methodID, jvalue *args);

    jclass (*GetObjectClass) (JNIEnv*, jobject obj);
    jboolean (*IsInstanceOf) (JNIEnv*, jobject obj, jclass clazz);

    jmethodID (*GetMethodID) (JNIEnv*, jclass clazz, const char *name, const char *sig);

    /* calling instance methods */ 

    jobject (*CallObjectMethod) (JNIEnv*, jobject obj, jmethodID methodID, ...);
    jobject (*CallObjectMethodV) (JNIEnv*, jobject obj, jmethodID methodID, va_list args);
    jobject (*CallObjectMethodA) (JNIEnv*, jobject obj, jmethodID methodID, jvalue * args);

    jboolean (*CallBooleanMethod) (JNIEnv*, jobject obj, jmethodID methodID, ...);
    jboolean (*CallBooleanMethodV) (JNIEnv*, jobject obj, jmethodID methodID, va_list args);
    jboolean (*CallBooleanMethodA) (JNIEnv*, jobject obj, jmethodID methodID, jvalue * args);

    jbyte (*CallByteMethod) (JNIEnv*, jobject obj, jmethodID methodID, ...);
    jbyte (*CallByteMethodV) (JNIEnv*, jobject obj, jmethodID methodID, va_list args);
    jbyte (*CallByteMethodA) (JNIEnv*, jobject obj, jmethodID methodID, jvalue *args);

    jchar (*CallCharMethod) (JNIEnv*, jobject obj, jmethodID methodID, ...);
    jchar (*CallCharMethodV) (JNIEnv*, jobject obj, jmethodID methodID, va_list args);
    jchar (*CallCharMethodA) (JNIEnv*, jobject obj, jmethodID methodID, jvalue *args);

    jshort (*CallShortMethod) (JNIEnv*, jobject obj, jmethodID methodID, ...);
    jshort (*CallShortMethodV) (JNIEnv*, jobject obj, jmethodID methodID, va_list args);
    jshort (*CallShortMethodA) (JNIEnv*, jobject obj, jmethodID methodID, jvalue *args);

    jint (*CallIntMethod) (JNIEnv*, jobject obj, jmethodID methodID, ...);
    jint (*CallIntMethodV) (JNIEnv*, jobject obj, jmethodID methodID, va_list args);
    jint (*CallIntMethodA) (JNIEnv*, jobject obj, jmethodID methodID, jvalue *args);

    jlong (*CallLongMethod) (JNIEnv*, jobject obj, jmethodID methodID, ...);
    jlong (*CallLongMethodV) (JNIEnv*, jobject obj, jmethodID methodID, va_list args);
    jlong (*CallLongMethodA) (JNIEnv*, jobject obj, jmethodID methodID, jvalue *args);

    jfloat (*CallFloatMethod) (JNIEnv*, jobject obj, jmethodID methodID, ...);
    jfloat (*CallFloatMethodV) (JNIEnv*, jobject obj, jmethodID methodID, va_list args);
    jfloat (*CallFloatMethodA) (JNIEnv*, jobject obj, jmethodID methodID, jvalue *args);

    jdouble (*CallDoubleMethod) (JNIEnv*, jobject obj, jmethodID methodID, ...);
    jdouble (*CallDoubleMethodV) (JNIEnv*, jobject obj, jmethodID methodID, va_list args);
    jdouble (*CallDoubleMethodA) (JNIEnv*, jobject obj, jmethodID methodID, jvalue *args);

    void (*CallVoidMethod) (JNIEnv*, jobject obj, jmethodID methodID, ...);
    void (*CallVoidMethodV) (JNIEnv*, jobject obj, jmethodID methodID, va_list args);
    void (*CallVoidMethodA) (JNIEnv*, jobject obj, jmethodID methodID, jvalue * args);

    jobject (*CallNonvirtualObjectMethod) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, ...);
    jobject (*CallNonvirtualObjectMethodV) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, va_list args);
    jobject (*CallNonvirtualObjectMethodA) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, jvalue * args);

    jboolean (*CallNonvirtualBooleanMethod) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, ...);
    jboolean (*CallNonvirtualBooleanMethodV) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, va_list args);
    jboolean (*CallNonvirtualBooleanMethodA) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, jvalue * args);

    jbyte (*CallNonvirtualByteMethod) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, ...);
    jbyte (*CallNonvirtualByteMethodV) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, va_list args);
    jbyte (*CallNonvirtualByteMethodA) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, jvalue *args);

    jchar (*CallNonvirtualCharMethod) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, ...);
    jchar (*CallNonvirtualCharMethodV) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, va_list args);
    jchar (*CallNonvirtualCharMethodA) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, jvalue *args);

    jshort (*CallNonvirtualShortMethod) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, ...);
    jshort (*CallNonvirtualShortMethodV) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, va_list args);
    jshort (*CallNonvirtualShortMethodA) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, jvalue *args);

    jint (*CallNonvirtualIntMethod) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, ...);
    jint (*CallNonvirtualIntMethodV) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, va_list args);
    jint (*CallNonvirtualIntMethodA) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, jvalue *args);

    jlong (*CallNonvirtualLongMethod) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, ...);
    jlong (*CallNonvirtualLongMethodV) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, va_list args);
    jlong (*CallNonvirtualLongMethodA) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, jvalue *args);

    jfloat (*CallNonvirtualFloatMethod) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, ...);
    jfloat (*CallNonvirtualFloatMethodV) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, va_list args);
    jfloat (*CallNonvirtualFloatMethodA) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, jvalue *args);

    jdouble (*CallNonvirtualDoubleMethod) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, ...);
    jdouble (*CallNonvirtualDoubleMethodV) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, va_list args);
    jdouble (*CallNonvirtualDoubleMethodA) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, jvalue *args);

    void (*CallNonvirtualVoidMethod) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, ...);
    void (*CallNonvirtualVoidMethodV) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, va_list args);
    void (*CallNonvirtualVoidMethodA) (JNIEnv*, jobject obj, jclass clazz, jmethodID methodID, jvalue * args);

    /* accessing fields */

    jfieldID (*GetFieldID) (JNIEnv*, jclass clazz, const char *name, const char *sig);

    jobject (*GetObjectField) (JNIEnv*, jobject obj, jfieldID fieldID);
    jboolean (*GetBooleanField) (JNIEnv*, jobject obj, jfieldID fieldID);
    jbyte (*GetByteField) (JNIEnv*, jobject obj, jfieldID fieldID);
    jchar (*GetCharField) (JNIEnv*, jobject obj, jfieldID fieldID);
    jshort (*GetShortField) (JNIEnv*, jobject obj, jfieldID fieldID);
    jint (*GetIntField) (JNIEnv*, jobject obj, jfieldID fieldID);
    jlong (*GetLongField) (JNIEnv*, jobject obj, jfieldID fieldID);
    jfloat (*GetFloatField) (JNIEnv*, jobject obj, jfieldID fieldID);
    jdouble (*GetDoubleField) (JNIEnv*, jobject obj, jfieldID fieldID);

    void (*SetObjectField) (JNIEnv*, jobject obj, jfieldID fieldID, jobject val);
    void (*SetBooleanField) (JNIEnv*, jobject obj, jfieldID fieldID, jboolean val);
    void (*SetByteField) (JNIEnv*, jobject obj, jfieldID fieldID, jbyte val);
    void (*SetCharField) (JNIEnv*, jobject obj, jfieldID fieldID, jchar val);
    void (*SetShortField) (JNIEnv*, jobject obj, jfieldID fieldID, jshort val);
    void (*SetIntField) (JNIEnv*, jobject obj, jfieldID fieldID, jint val);
    void (*SetLongField) (JNIEnv*, jobject obj, jfieldID fieldID, jlong val);
    void (*SetFloatField) (JNIEnv*, jobject obj, jfieldID fieldID, jfloat val);
    void (*SetDoubleField) (JNIEnv*, jobject obj, jfieldID fieldID, jdouble val);

    /* calling static methods */ 
    
    jmethodID (*GetStaticMethodID) (JNIEnv*, jclass clazz, const char *name, const char *sig);

    jobject (*CallStaticObjectMethod) (JNIEnv*, jclass clazz, jmethodID methodID, ...);
    jobject (*CallStaticObjectMethodV) (JNIEnv*, jclass clazz, jmethodID methodID, va_list args);
    jobject (*CallStaticObjectMethodA) (JNIEnv*, jclass clazz, jmethodID methodID, jvalue *args);

    jboolean (*CallStaticBooleanMethod) (JNIEnv*, jclass clazz, jmethodID methodID, ...);
    jboolean (*CallStaticBooleanMethodV) (JNIEnv*, jclass clazz, jmethodID methodID, va_list args);
    jboolean (*CallStaticBooleanMethodA) (JNIEnv*, jclass clazz, jmethodID methodID, jvalue *args);

    jbyte (*CallStaticByteMethod) (JNIEnv*, jclass clazz, jmethodID methodID, ...);
    jbyte (*CallStaticByteMethodV) (JNIEnv*, jclass clazz, jmethodID methodID, va_list args);
    jbyte (*CallStaticByteMethodA) (JNIEnv*, jclass clazz, jmethodID methodID, jvalue *args);

    jchar (*CallStaticCharMethod) (JNIEnv*, jclass clazz, jmethodID methodID, ...);
    jchar (*CallStaticCharMethodV) (JNIEnv*, jclass clazz, jmethodID methodID, va_list args);
    jchar (*CallStaticCharMethodA) (JNIEnv*, jclass clazz, jmethodID methodID, jvalue *args);

    jshort (*CallStaticShortMethod) (JNIEnv*, jclass clazz, jmethodID methodID, ...);
    jshort (*CallStaticShortMethodV) (JNIEnv*, jclass clazz, jmethodID methodID, va_list args);
    jshort (*CallStaticShortMethodA) (JNIEnv*, jclass clazz, jmethodID methodID, jvalue *args);

    jint (*CallStaticIntMethod) (JNIEnv*, jclass clazz, jmethodID methodID, ...);
    jint (*CallStaticIntMethodV) (JNIEnv*, jclass clazz, jmethodID methodID, va_list args);
    jint (*CallStaticIntMethodA) (JNIEnv*, jclass clazz, jmethodID methodID, jvalue *args);

    jlong (*CallStaticLongMethod) (JNIEnv*, jclass clazz, jmethodID methodID, ...);
    jlong (*CallStaticLongMethodV) (JNIEnv*, jclass clazz, jmethodID methodID, va_list args);
    jlong (*CallStaticLongMethodA) (JNIEnv*, jclass clazz, jmethodID methodID, jvalue *args);

    jfloat (*CallStaticFloatMethod) (JNIEnv*, jclass clazz, jmethodID methodID, ...);
    jfloat (*CallStaticFloatMethodV) (JNIEnv*, jclass clazz, jmethodID methodID, va_list args);
    jfloat (*CallStaticFloatMethodA) (JNIEnv*, jclass clazz, jmethodID methodID, jvalue *args);

    jdouble (*CallStaticDoubleMethod) (JNIEnv*, jclass clazz, jmethodID methodID, ...);
    jdouble (*CallStaticDoubleMethodV) (JNIEnv*, jclass clazz, jmethodID methodID, va_list args);
    jdouble (*CallStaticDoubleMethodA) (JNIEnv*, jclass clazz, jmethodID methodID, jvalue *args);

    void (*CallStaticVoidMethod) (JNIEnv*, jclass cls, jmethodID methodID, ...);
    void (*CallStaticVoidMethodV) (JNIEnv*, jclass cls, jmethodID methodID, va_list args);
    void (*CallStaticVoidMethodA) (JNIEnv*, jclass cls, jmethodID methodID, jvalue * args);

    /* accessing static fields */

    jfieldID (*GetStaticFieldID) (JNIEnv*, jclass clazz, const char *name, const char *sig);
    jobject (*GetStaticObjectField) (JNIEnv*, jclass clazz, jfieldID fieldID);
    jboolean (*GetStaticBooleanField) (JNIEnv*, jclass clazz, jfieldID fieldID);
    jbyte (*GetStaticByteField) (JNIEnv*, jclass clazz, jfieldID fieldID);
    jchar (*GetStaticCharField) (JNIEnv*, jclass clazz, jfieldID fieldID);
    jshort (*GetStaticShortField) (JNIEnv*, jclass clazz, jfieldID fieldID);
    jint (*GetStaticIntField) (JNIEnv*, jclass clazz, jfieldID fieldID);
    jlong (*GetStaticLongField) (JNIEnv*, jclass clazz, jfieldID fieldID);
    jfloat (*GetStaticFloatField) (JNIEnv*, jclass clazz, jfieldID fieldID);
    jdouble (*GetStaticDoubleField) (JNIEnv*, jclass clazz, jfieldID fieldID);

    void (*SetStaticObjectField) (JNIEnv*, jclass clazz, jfieldID fieldID, jobject value);
    void (*SetStaticBooleanField) (JNIEnv*, jclass clazz, jfieldID fieldID, jboolean value);
    void (*SetStaticByteField) (JNIEnv*, jclass clazz, jfieldID fieldID, jbyte value);
    void (*SetStaticCharField) (JNIEnv*, jclass clazz, jfieldID fieldID, jchar value);
    void (*SetStaticShortField) (JNIEnv*, jclass clazz, jfieldID fieldID, jshort value);
    void (*SetStaticIntField) (JNIEnv*, jclass clazz, jfieldID fieldID, jint value);
    void (*SetStaticLongField) (JNIEnv*, jclass clazz, jfieldID fieldID, jlong value);
    void (*SetStaticFloatField) (JNIEnv*, jclass clazz, jfieldID fieldID, jfloat value);
    void (*SetStaticDoubleField) (JNIEnv*, jclass clazz, jfieldID fieldID, jdouble value);

    /* string operations */ 

    jstring (*NewString) (JNIEnv*, const jchar *unicode, jsize len);
    jsize (*GetStringLength) (JNIEnv*, jstring str);
    const jchar *(*GetStringChars) (JNIEnv*, jstring str, jboolean *isCopy);
    void (*ReleaseStringChars) (JNIEnv*, jstring str, const jchar *chars);

    jstring (*NewStringUTF) (JNIEnv*, const char *bytes);
    jsize (*GetStringUTFLength) (JNIEnv*, jstring str);
    const char* (*GetStringUTFChars) (JNIEnv*, jstring str, jboolean *isCopy);
    void (*ReleaseStringUTFChars) (JNIEnv*, jstring str, const char* chars);

    /* array operations */

    jsize (*GetArrayLength) (JNIEnv*, jarray array);

    jobjectArray (*NewObjectArray) (JNIEnv*, jsize len, jclass clazz, jobject init);
    jobject (*GetObjectArrayElement) (JNIEnv*, jobjectArray array, jsize index);
    void (*SetObjectArrayElement) (JNIEnv*, jobjectArray array, jsize index, jobject val);

    jbooleanArray (*NewBooleanArray) (JNIEnv*, jsize len);
    jbyteArray (*NewByteArray) (JNIEnv*, jsize len);
    jcharArray (*NewCharArray) (JNIEnv*, jsize len);
    jshortArray (*NewShortArray) (JNIEnv*, jsize len);
    jintArray (*NewIntArray) (JNIEnv*, jsize len);
    jlongArray (*NewLongArray) (JNIEnv*, jsize len);
    jfloatArray (*NewFloatArray) (JNIEnv*, jsize len);
    jdoubleArray (*NewDoubleArray) (JNIEnv*, jsize len);

    jboolean * (*GetBooleanArrayElements) (JNIEnv*, jbooleanArray array, jboolean *isCopy);
    jbyte * (*GetByteArrayElements) (JNIEnv*, jbyteArray array, jboolean *isCopy);
    jchar * (*GetCharArrayElements) (JNIEnv*, jcharArray array, jboolean *isCopy);
    jshort * (*GetShortArrayElements) (JNIEnv*, jshortArray array, jboolean *isCopy);
    jint * (*GetIntArrayElements) (JNIEnv*, jintArray array, jboolean *isCopy);
    jlong * (*GetLongArrayElements) (JNIEnv*, jlongArray array, jboolean *isCopy);
    jfloat * (*GetFloatArrayElements) (JNIEnv*, jfloatArray array, jboolean *isCopy);
    jdouble * (*GetDoubleArrayElements) (JNIEnv*, jdoubleArray array, jboolean *isCopy);

    void (*ReleaseBooleanArrayElements) (JNIEnv*, jbooleanArray array, jboolean *elems, jint mode);
    void (*ReleaseByteArrayElements) (JNIEnv*, jbyteArray array, jbyte *elems, jint mode);
    void (*ReleaseCharArrayElements) (JNIEnv*, jcharArray array, jchar *elems, jint mode);
    void (*ReleaseShortArrayElements) (JNIEnv*, jshortArray array, jshort *elems, jint mode);
    void (*ReleaseIntArrayElements) (JNIEnv*, jintArray array, jint *elems, jint mode);
    void (*ReleaseLongArrayElements) (JNIEnv*, jlongArray array, jlong *elems, jint mode);
    void (*ReleaseFloatArrayElements) (JNIEnv*, jfloatArray array, jfloat *elems, jint mode);
    void (*ReleaseDoubleArrayElements) (JNIEnv*, jdoubleArray array, jdouble *elems, jint mode);

    void (*GetBooleanArrayRegion) (JNIEnv*, jbooleanArray array, jsize start, jsize l, jboolean *buf);
    void (*GetByteArrayRegion) (JNIEnv*, jbyteArray array, jsize start, jsize len, jbyte *buf);
    void (*GetCharArrayRegion) (JNIEnv*, jcharArray array, jsize start, jsize len, jchar *buf);
    void (*GetShortArrayRegion) (JNIEnv*, jshortArray array, jsize start, jsize len, jshort *buf);
    void (*GetIntArrayRegion) (JNIEnv*, jintArray array, jsize start, jsize len, jint *buf);
    void (*GetLongArrayRegion) (JNIEnv*, jlongArray array, jsize start, jsize len, jlong *buf);
    void (*GetFloatArrayRegion) (JNIEnv*, jfloatArray array, jsize start, jsize len, jfloat *buf);
    void (*GetDoubleArrayRegion) (JNIEnv*, jdoubleArray array, jsize start, jsize len, jdouble *buf);

    void (*SetBooleanArrayRegion) (JNIEnv*, jbooleanArray array, jsize start, jsize l, jboolean *buf);
    void (*SetByteArrayRegion) (JNIEnv*, jbyteArray array, jsize start, jsize len, jbyte *buf);
    void (*SetCharArrayRegion) (JNIEnv*, jcharArray array, jsize start, jsize len, jchar *buf);
    void (*SetShortArrayRegion) (JNIEnv*, jshortArray array, jsize start, jsize len, jshort *buf);
    void (*SetIntArrayRegion) (JNIEnv*, jintArray array, jsize start, jsize len, jint *buf);
    void (*SetLongArrayRegion) (JNIEnv*, jlongArray array, jsize start, jsize len, jlong *buf);
    void (*SetFloatArrayRegion) (JNIEnv*, jfloatArray array, jsize start, jsize len, jfloat *buf);
    void (*SetDoubleArrayRegion) (JNIEnv*, jdoubleArray array, jsize start, jsize len, jdouble *buf);

    /* registering native methods */

    jint (*RegisterNatives) (JNIEnv*, jclass clazz, const JNINativeMethod *methods, jint nMethods);
    jint (*UnregisterNatives) (JNIEnv*, jclass clazz);

    /* monitor operations */

    jint (*MonitorEnter) (JNIEnv*, jobject obj);
    jint (*MonitorExit) (JNIEnv*, jobject obj);

    /* JavaVM interface */

    jint (*GetJavaVM) (JNIEnv*, JavaVM **vm);

	/* new JNI 1.2 functions */

    void (*GetStringRegion) (JNIEnv*, jstring str, jsize start, jsize len, jchar *buf);
    void (*GetStringUTFRegion) (JNIEnv*, jstring str, jsize start, jsize len, char *buf);

    void * (*GetPrimitiveArrayCritical) (JNIEnv*, jarray array, jboolean *isCopy);
    void (*ReleasePrimitiveArrayCritical) (JNIEnv*, jarray array, void *carray, jint mode);

    const jchar * (*GetStringCritical) (JNIEnv*, jstring string, jboolean *isCopy);
    void (*ReleaseStringCritical) (JNIEnv*, jstring string, const jchar *cstring);

    jweak (*NewWeakGlobalRef) (JNIEnv*, jobject obj);
    void (*DeleteWeakGlobalRef) (JNIEnv*, jweak ref);

    jboolean (*ExceptionCheck) (JNIEnv*);

	/* new JNI 1.4 functions */

	jobject (*NewDirectByteBuffer) (JNIEnv *env, void* address, jlong capacity);
	void* (*GetDirectBufferAddress) (JNIEnv *env, jobject buf);
	jlong (*GetDirectBufferCapacity) (JNIEnv *env, jobject buf);
};


/* function prototypes ********************************************************/

jint JNI_GetDefaultJavaVMInitArgs(void *vm_args);
jint JNI_GetCreatedJavaVMs(JavaVM **vmBuf, jsize bufLen, jsize *nVMs);
jint JNI_CreateJavaVM(JavaVM **p_vm, JNIEnv **p_env, void *vm_args);

jfieldID getFieldID_critical(JNIEnv *env, jclass clazz, char *name, char *sig);

jobject GetObjectField(JNIEnv *env, jobject obj, jfieldID fieldID);
jboolean GetBooleanField(JNIEnv *env, jobject obj, jfieldID fieldID);
jbyte GetByteField(JNIEnv *env, jobject obj, jfieldID fieldID);
jchar GetCharField(JNIEnv *env, jobject obj, jfieldID fieldID);
jshort GetShortField(JNIEnv *env, jobject obj, jfieldID fieldID);
jint GetIntField(JNIEnv *env, jobject obj, jfieldID fieldID);
jlong GetLongField(JNIEnv *env, jobject obj, jfieldID fieldID);
jfloat GetFloatField(JNIEnv *env, jobject obj, jfieldID fieldID);
jdouble GetDoubleField(JNIEnv *env, jobject obj, jfieldID fieldID);

void SetObjectField(JNIEnv *env, jobject obj, jfieldID fieldID, jobject val);
void SetBooleanField(JNIEnv *env, jobject obj, jfieldID fieldID, jboolean val);
void SetByteField(JNIEnv *env, jobject obj, jfieldID fieldID, jbyte val);
void SetCharField(JNIEnv *env, jobject obj, jfieldID fieldID, jchar val);
void SetShortField(JNIEnv *env, jobject obj, jfieldID fieldID, jshort val);
void SetIntField(JNIEnv *env, jobject obj, jfieldID fieldID, jint val);
void SetLongField(JNIEnv *env, jobject obj, jfieldID fieldID, jlong val);
void SetFloatField(JNIEnv *env, jobject obj, jfieldID fieldID, jfloat val);
void SetDoubleField(JNIEnv *env, jobject obj, jfieldID fieldID, jdouble val);


#define setField(obj,typ,var,val) *((typ*) ((long int) obj + (long int) var->offset))=val;
#define getField(obj,typ,var)     *((typ*) ((long int) obj + (long int) var->offset))
#define setfield_critical(clazz,obj,name,sig,jdatatype,val) setField(obj,jdatatype,getFieldID_critical(env,clazz,name,sig),val);

jobject *jni_method_invokeNativeHelper(JNIEnv *env,struct methodinfo *mi,jobject obj, java_objectarray *params);

void jni_init ();

extern void* ptr_env;
extern struct JNINativeInterface JNI_JNIEnvTable;

#endif /* _JNI_H */


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
