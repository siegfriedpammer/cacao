/* src/native/vm/VMThrowable.c - java/lang/VMThrowable

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

   Authors: Joseph Wenninger

   Changes: Christian Thalinger

   $Id: VMThrowable.c 2152 2005-03-30 19:28:40Z twisti $

*/


#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_Throwable.h"
#include "native/include/java_lang_VMClass.h"
#include "native/include/java_lang_VMThrowable.h"
#include "vm/builtin.h"
#include "vm/class.h"
#include "vm/loader.h"
#include "vm/stringlocal.h"
#include "vm/tables.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/stacktrace.h"


/*
 * Class:     java/lang/VMThrowable
 * Method:    fillInStackTrace
 * Signature: (Ljava/lang/Throwable;)Ljava/lang/VMThrowable;
 */
JNIEXPORT java_lang_VMThrowable* JNICALL Java_java_lang_VMThrowable_fillInStackTrace(JNIEnv *env, jclass clazz, java_lang_Throwable *par1)
{
	java_lang_VMThrowable *vmthrow;

	vmthrow = (java_lang_VMThrowable *) native_new_and_init(class_java_lang_VMThrowable);

	if (!vmthrow)
		panic("Needed instance of class  java.lang.VMThrowable could not be created");

#if defined(__I386__) || defined(__ALPHA__)
	cacao_stacktrace_NormalTrace(&(vmthrow->vmData));
#endif
	return vmthrow;
}


static
java_objectarray* generateStackTraceArray(JNIEnv *env,stacktraceelement *el,long size)
{
	long resultPos;
	methodinfo *m;
	classinfo *c;
	java_objectarray *oa;

	c = class_new(utf_new_char("java/lang/StackTraceElement"));

	if (!c->loaded)
		load_class_bootstrap(c);

	if (!c->linked)
		link_class(c);

	m = class_findmethod(c,
						 utf_init,
						 utf_new_char("(Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Z)V"));

	if (!m)
		panic("java.lang.StackTraceElement misses needed constructor");	

	oa = builtin_anewarray(size, c);

	if (!oa)
		return 0;

/*	printf("Should return an array with %ld element(s)\n",size);*/
	/*pos--;*/
	
	for(resultPos=0;size>0;size--,el++,resultPos++) {
		java_objectheader *element;

		if (el->method==0) {
			resultPos--;
			continue;
		}

		element=builtin_new(c);
		if (!element) {
			panic("Memory for stack trace element could not be allocated");
		}
#ifdef __GNUC__
#warning call constructor once jni is fixed to allow more than three parameters
#endif
#if 0
		(*env)->CallVoidMethod(env,element,m,
			javastring_new(el->method->class->sourcefile),
			source[size].linenumber,
			javastring_new(el->method->class->name),
			javastring_new(el->method->name),
			el->method->flags & ACC_NATIVE);
#else
		if (!(el->method->flags & ACC_NATIVE))setfield_critical(c,element,"fileName",          
		"Ljava/lang/String;",  jobject, 
		(jobject) javastring_new(el->method->class->sourcefile));
/*  		setfield_critical(c,element,"className",          "Ljava/lang/String;",  jobject,  */
/*  		(jobject) javastring_new(el->method->class->name)); */
		setfield_critical(c,element,"declaringClass",          "Ljava/lang/String;",  jobject, 
		(jobject) Java_java_lang_VMClass_getName(env, NULL, (java_lang_Class *) el->method->class));
		setfield_critical(c,element,"methodName",          "Ljava/lang/String;",  jobject, 
		(jobject) javastring_new(el->method->name));
		setfield_critical(c,element,"lineNumber",          "I",  jint, 
		(jint) ((el->method->flags & ACC_NATIVE) ? -1:(el->linenumber)));
		setfield_critical(c,element,"isNative",          "Z",  jboolean, 
		(jboolean) ((el->method->flags & ACC_NATIVE) ? 1:0));


#endif			

		oa->data[resultPos]=element;
	}

	return oa;

}


/*
 * Class:     java/lang/VMThrowable
 * Method:    getStackTrace
 * Signature: (Ljava/lang/Throwable;)[Ljava/lang/StackTraceElement;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMThrowable_getStackTrace(JNIEnv *env, java_lang_VMThrowable *this, java_lang_Throwable *par1)
{
#if defined(__I386__) || defined(__ALPHA__)
	stackTraceBuffer *buf=(stackTraceBuffer*)this->vmData;
	u8 size;
	stacktraceelement *el;
	classinfo  *excClass=par1->header.vftbl->class;
	long destElementCount;
	stacktraceelement *tmpEl;

	if (!buf) panic("Invalid java.lang.VMThrowable.vmData field in java.lang.VMThrowable.getStackTrace native code");
	
	size=buf->full;
	if (size<=2) panic("Invalid java.lang.VMThrowable.vmData field in java.lang.VMThrowable.getStackTrace native code (length<=2)");
	size -=2;
	el=&(buf->start[2]); /* element 0==VMThrowable.fillInStackTrace native call, 1==Throwable.fillInStackTrace*/
	if (el->method!=0) { /* => not a builtin native wrapper*/
		if ((el->method->class->name == utf_java_lang_Throwable) &&
			(el->method->name == utf_init)) {
			/* We assume that we are within the initializer of the exception object, the exception object itself should not appear
				in the stack trace, so we skip till we reach the first function, which is not an init function or till we reach the exception object class*/
			for (; (size>0) && (el->method->name == utf_init) && (el->method->class!=excClass); el++, size--) {
				/* just loop*/
			}
			size --;
			el++;
			/*if (size<1) {
				log_text("Invalid stacktrace for VMThrowable.getStackTrace()");
			}*/
		}
	}

	
	for (destElementCount = 0, tmpEl=el; size>0; size--,tmpEl++) {
		if (tmpEl->method!=0) destElementCount++;
	}

	return generateStackTraceArray(env,el,destElementCount);
#else
	return 0;
#endif
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
