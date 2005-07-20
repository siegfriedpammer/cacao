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

   $Id: VMThrowable.c 3070 2005-07-20 00:33:27Z michi $

*/


#include <assert.h>

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
#include "vm/exceptions.h"


/*
 * Class:     java/lang/VMThrowable
 * Method:    fillInStackTrace
 * Signature: (Ljava/lang/Throwable;)Ljava/lang/VMThrowable;
 */
JNIEXPORT java_lang_VMThrowable* JNICALL Java_java_lang_VMThrowable_fillInStackTrace(JNIEnv *env, jclass clazz, java_lang_Throwable *par1)
{
	java_lang_VMThrowable *vmthrow;

	if ((*dontfillinexceptionstacktrace) == true) {
		/*log_text("dontfillinexceptionstacktrace");*/
		return 0;
	}

	vmthrow = (java_lang_VMThrowable *)
		native_new_and_init(class_java_lang_VMThrowable);

	if (!vmthrow) {
		log_text("Needed instance of class  java.lang.VMThrowable could not be created");
		assert(0);
	}

#if defined(__ALPHA__) || defined(__ARM__) || defined(__I386__) || defined(__MIPS__) || defined(__POWERPC__) || defined(__X86_64__)
	cacao_stacktrace_NormalTrace((void **) &(vmthrow->vmData));
#endif
	return vmthrow;
}


static java_objectarray *generateStackTraceArray(JNIEnv *env,stacktraceelement *el, s4 size)
{
	long resultPos;
	methodinfo *m;
	java_objectarray *oa;

	m = class_findmethod(class_java_lang_StackTraceElement,
						 utf_init,
						 utf_new_char("(Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Z)V"));

	if (!m) {
		log_text("java.lang.StackTraceElement misses needed constructor");	
		assert(0);
	}

	oa = builtin_anewarray(size, class_java_lang_StackTraceElement);

	if (!oa)
		return 0;

/*	printf("Should return an array with %ld element(s)\n",size);*/
	/*pos--;*/
	
	for(resultPos = 0; size > 0; size--, el++, resultPos++) {
		java_objectheader *element;

		if (el->method == NULL) {
			resultPos--;
			continue;
		}

		element = builtin_new(class_java_lang_StackTraceElement);
		if (!element) {
			log_text("Memory for stack trace element could not be allocated");
			assert(0);
		}

		/* XXX call constructor once jni is fixed to allow more than three parameters */

#if 0
		(*env)->CallVoidMethod(env,element,m,
			javastring_new(el->method->class->sourcefile),
			source[size].linenumber,
			javastring_new(el->method->class->name),
			javastring_new(el->method->name),
			el->method->flags & ACC_NATIVE);
#else
		if (!(el->method->flags & ACC_NATIVE))
			setfield_critical(class_java_lang_StackTraceElement,
							  element,
							  "fileName",
							  "Ljava/lang/String;",
							  jobject,
							  (jobject) javastring_new(el->method->class->sourcefile));

/*  		setfield_critical(c,element,"className",          "Ljava/lang/String;",  jobject,  */
/*  		(jobject) javastring_new(el->method->class->name)); */

		setfield_critical(class_java_lang_StackTraceElement,
						  element,
						  "declaringClass",
						  "Ljava/lang/String;",
						  jobject, 
						  (jobject) Java_java_lang_VMClass_getName(env, NULL, (java_lang_Class *) el->method->class));

		setfield_critical(class_java_lang_StackTraceElement,
						  element,
						  "methodName",
						  "Ljava/lang/String;",
						  jobject,
						  (jobject) javastring_new(el->method->name));

		setfield_critical(class_java_lang_StackTraceElement,
						  element,
						  "lineNumber",
						  "I",
						  jint, 
						  (jint) ((el->method->flags & ACC_NATIVE) ? -1 : (el->linenumber)));

		setfield_critical(class_java_lang_StackTraceElement,
						  element,
						  "isNative",
						  "Z",
						  jboolean, 
						  (jboolean) ((el->method->flags & ACC_NATIVE) ? 1 : 0));
#endif			

		oa->data[resultPos] = element;
	}

	return oa;
}


/*
 * Class:     java/lang/VMThrowable
 * Method:    getStackTrace
 * Signature: (Ljava/lang/Throwable;)[Ljava/lang/StackTraceElement;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMThrowable_getStackTrace(JNIEnv *env, java_lang_VMThrowable *this, java_lang_Throwable *t)
{
#if defined(__ALPHA__) || defined(__ARM__) || defined(__I386__) || defined(__MIPS__) || defined(__POWERPC__) || defined(__X86_64__)
	stackTraceBuffer  *buf;
	stacktraceelement *elem;
	stacktraceelement *tmpelem;
	u8                 size;
	s4                 elemcount;
	classinfo         *c;
	bool               inexceptionclass;
	bool               leftexceptionclass;

	buf = (stackTraceBuffer *) this->vmData;
	c = t->header.vftbl->class;

	if (!buf) {
		log_text("Invalid java.lang.VMThrowable.vmData field in java.lang.VMThrowable.getStackTrace native code");
		assert(0);
	}
	
	size = buf->full;

	if (size < 2) {
		log_text("Invalid java.lang.VMThrowable.vmData field in java.lang.VMThrowable.getStackTrace native code (length<2)");
		assert(0);
	}

	/* skip first 2 elements in stacktrace buffer:                            */
	/*   0: VMThrowable.fillInStackTrace                                      */
	/*   1: Throwable.fillInStackTrace                                        */

	elem = &(buf->start[2]);
	size -= 2;

	if (size && elem->method != 0) {
		/* not a builtin native wrapper*/

		if ((elem->method->class->name == utf_java_lang_Throwable) &&
			(elem->method->name == utf_init)) {
			/* We assume that we are within the initializer of the exception  */
			/* object, the exception object itself should not appear in the   */
			/* stack trace, so we skip till we reach the first function,      */
			/* which is not an init function.                                 */

			inexceptionclass = false;
			leftexceptionclass = false;

			while (size > 0) {
				/* check if we are in the exception class */

				if (elem->method->class == c)
					inexceptionclass = true;

				/* check if we left the exception class */

				if (inexceptionclass && (elem->method->class != c))
					leftexceptionclass = true;

				/* found exception start point if we left the initalizers or  */
				/* we left the exception class                                */

				if ((elem->method->name != utf_init) || leftexceptionclass)
					break;

				elem++;
				size--;
			}
		}
	}
	
	for (elemcount = 0, tmpelem = elem; size > 0; size--, tmpelem++) {
		if (tmpelem->method)
			elemcount++;
	}

	return generateStackTraceArray(env, elem, elemcount);
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
