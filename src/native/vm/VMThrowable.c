/* nat/VMThrowable.c - java/lang/Throwable

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

   Authors: Joseph Wenninger

   $Id: VMThrowable.c 1082 2004-05-26 15:04:54Z jowenn $

*/


#include "global.h"
#include "jni.h"
#include "asmpart.h"
#include "loader.h"
#include "builtin.h"
#include "tables.h"
#include "native.h"
#include "java_lang_Throwable.h"
#include "java_lang_VMThrowable.h"


/*
 * Class:     java/lang/VMThrowable
 * Method:    fillInStackTrace
 * Signature: (Ljava/lang/Throwable;)Ljava/lang/VMThrowable;
 */
JNIEXPORT java_lang_VMThrowable* JNICALL Java_java_lang_VMThrowable_fillInStackTrace(JNIEnv *env, jclass clazz, java_lang_Throwable *par1)
{
	classinfo *class_java_lang_VMThrowable = NULL;
	java_lang_VMThrowable *vmthrow;

	if (!class_java_lang_VMThrowable)
		class_java_lang_VMThrowable = class_new(utf_new_char("java/lang/VMThrowable"));

	if (class_java_lang_VMThrowable == NULL)
		panic("Needed class java.lang.VMThrowable missing");

	vmthrow = (java_lang_VMThrowable *) native_new_and_init(class_java_lang_VMThrowable);

	if (vmthrow == NULL)
		panic("Needed instance of class  java.lang.VMThrowable could not be created");

#if defined(__I386__)
	(void) asm_get_stackTrace(&(vmthrow->vmData));
#else
	vmthrow->vmData=0;
#endif

	return vmthrow;
}



java_objectarray* generateStackTraceArray(JNIEnv *env,stacktraceelement *source,long pos,long size)
{
	long resultPos;
	methodinfo *m;
	classinfo *c;
	java_objectarray *oa;

	c = class_new(utf_new_char("java/lang/StackTraceElement"));

	if (!c->loaded)
		class_load(c);

	if (!c->linked)
		class_link(c);

	m = class_findmethod(c,
						 utf_new_char("<init>"),
						 utf_new_char("(Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Z)V"));

	if (!m)
		panic("java.lang.StackTraceElement misses needed constructor");	

	oa = builtin_anewarray(size, c);

	if (!oa)
		return 0;

/*	printf("Should return an array with %ld element(s)\n",size);*/
	pos--;
	
	
	for(resultPos=0;pos>=0;resultPos++,pos--) {

		if (source[pos].method==0) {
			resultPos--;
			continue;
		}

		java_objectheader *element=builtin_new(c);
		if (!element) {
			panic("Memory for stack trace element could not be allocated");
		}
#ifdef __GNUC__
#warning call constructor once jni is fixed to allow more than three parameters
#endif
#if 0
		(*env)->CallVoidMethod(env,element,m,
			javastring_new(source[pos].method->class->sourcefile),
			source[size].linenumber,
			javastring_new(source[pos].method->class->name),
			javastring_new(source[pos].method->name),
			source[pos].method->flags & ACC_NATIVE);
#else
		if (!(source[pos].method->flags & ACC_NATIVE))setfield_critical(c,element,"fileName",          
		"Ljava/lang/String;",  jobject, 
		(jobject) javastring_new(source[pos].method->class->sourcefile));
		setfield_critical(c,element,"className",          "Ljava/lang/String;",  jobject, 
		(jobject) javastring_new(source[pos].method->class->name));
		setfield_critical(c,element,"methodName",          "Ljava/lang/String;",  jobject, 
		(jobject) javastring_new(source[pos].method->name));
		setfield_critical(c,element,"lineNumber",          "I",  jint, 
		(jint) ((source[pos].method->flags & ACC_NATIVE) ? -1:(source[pos].linenumber)));
		setfield_critical(c,element,"isNative",          "Z",  jboolean, 
		(jboolean) ((source[pos].method->flags & ACC_NATIVE) ? 1:0));


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
	long  pos;
	long  maxpos;
	long  sizediff;
	utf*  classname=par1->header.vftbl->class->name;
	utf*  init=utf_new_char("<init>");
	utf*  throwable=utf_new_char("java/lang/Throwable");
	stacktraceelement *el=(stacktraceelement*)this->vmData;

	/*	log_text("Java_java_lang_VMThrowable_getStackTrace");
	utf_display(par1->header.vftbl->class->name);
	printf("\n----------------------------------------------\n");*/

	sizediff=0;
	if (el == 0) {
		return generateStackTraceArray(env, el, 0,0);
	}	

	for (pos = 0; !((el[pos].method == 0) && (el[pos].linenumber ==-1)); pos++) {
		if (el[pos].method==0) sizediff++;
	}

	if (pos == 0) {
		panic("Stacktrace cannot have zero length");
	}

	pos--;
	pos--;
	maxpos = pos;
	if (el[pos].method->class->name == throwable && el[pos].method->name == init) {
		for (; pos >= 0 && el[pos].method->name == init && el[pos].method->class->name != classname; pos--);
		pos--;
		if (pos < 0) {
			log_text("Invalid stack trace for Throwable.getStackTrace()");
		}
	}
	
	/* build the result array*/
	pos++; /*arraysize*/

	return generateStackTraceArray(env,el,pos,pos-sizediff);	
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
