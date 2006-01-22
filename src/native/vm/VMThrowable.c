/* src/native/vm/VMThrowable.c - java/lang/VMThrowable

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

   Authors: Joseph Wenninger

   Changes: Christian Thalinger

   $Id: VMThrowable.c 4357 2006-01-22 23:33:38Z twisti $

*/


#include <assert.h>

#include "config.h"
#include "vm/types.h"

#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_StackTraceElement.h"
#include "native/include/java_lang_Throwable.h"
#include "native/include/java_lang_VMClass.h"
#include "native/include/java_lang_VMThrowable.h"
#include "vm/builtin.h"
#include "vm/class.h"
#include "vm/exceptions.h"
#include "vm/loader.h"
#include "vm/stringlocal.h"
#include "vm/jit/stacktrace.h"


/*
 * Class:     java/lang/VMThrowable
 * Method:    fillInStackTrace
 * Signature: (Ljava/lang/Throwable;)Ljava/lang/VMThrowable;
 */
JNIEXPORT java_lang_VMThrowable* JNICALL Java_java_lang_VMThrowable_fillInStackTrace(JNIEnv *env, jclass clazz, java_lang_Throwable *par1)
{
	java_lang_VMThrowable *vmthrow;

	vmthrow = (java_lang_VMThrowable *)
		native_new_and_init(class_java_lang_VMThrowable);

	if (!vmthrow) {
		log_text("Needed instance of class  java.lang.VMThrowable could not be created");
		assert(0);
	}

#if defined(__ALPHA__) || defined(__ARM__) || defined(__I386__) || defined(__MIPS__) || defined(__POWERPC__) || defined(__X86_64__)
	if (!cacao_stacktrace_NormalTrace((void **) &(vmthrow->vmData)))
		return NULL;
#endif

	return vmthrow;
}


/*
 * Class:     java/lang/VMThrowable
 * Method:    getStackTrace
 * Signature: (Ljava/lang/Throwable;)[Ljava/lang/StackTraceElement;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMThrowable_getStackTrace(JNIEnv *env, java_lang_VMThrowable *this, java_lang_Throwable *t)
{
#if defined(__ALPHA__) || defined(__ARM__) || defined(__I386__) || defined(__MIPS__) || defined(__POWERPC__) || defined(__X86_64__)
	stackTraceBuffer            *buf;
	stacktraceelement           *elem;
	stacktraceelement           *tmpelem;
	s4                           size;
	s4                           i;
	classinfo                   *c;
	bool                         inexceptionclass;
	bool                         leftexceptionclass;

	methodinfo                  *m;
	java_objectarray            *oa;
	s4                           oalength;
	java_lang_StackTraceElement *ste;
	java_lang_String            *filename;
	s4                           linenumber;
	java_lang_String            *declaringclass;

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


	/* now fill the stacktrace into java objects */

	m = class_findmethod(class_java_lang_StackTraceElement,
						 utf_init,
						 utf_new_char("(Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Z)V"));

	if (!m)
		return NULL;

	/* count entries with a method name */

	for (oalength = 0, i = size, tmpelem = elem; i > 0; i--, tmpelem++)
		if (tmpelem->method)
			oalength++;

	/* create the stacktrace element array */

	oa = builtin_anewarray(oalength, class_java_lang_StackTraceElement);

	if (!oa)
		return NULL;

	for (i = 0; size > 0; size--, elem++, i++) {
		if (elem->method == NULL) {
			i--;
			continue;
		}

		/* allocate a new stacktrace element */

		ste = (java_lang_StackTraceElement *)
			builtin_new(class_java_lang_StackTraceElement);

		if (!ste)
			return NULL;

		/* get filename */

		if (!(elem->method->flags & ACC_NATIVE)) {
			if (elem->method->class->sourcefile)
				filename = javastring_new(elem->method->class->sourcefile);
			else
				filename = NULL;

		} else {
			filename = NULL;
		}

		/* get line number */

		if (elem->method->flags & ACC_NATIVE)
			linenumber = -1;
		else
			linenumber = (elem->linenumber == 0) ? -1 : elem->linenumber;

		/* get declaring class name */

		declaringclass = Java_java_lang_VMClass_getName(env, NULL, (java_lang_Class *) elem->method->class);


		/* fill the stacktrace element */

		ste->fileName       = filename;
		ste->lineNumber     = linenumber;
		ste->declaringClass = declaringclass;
		ste->methodName     = javastring_new(elem->method->name);
		ste->isNative       = (elem->method->flags & ACC_NATIVE) ? 1 : 0;

		oa->data[i] = (java_objectheader *) ste;
	}

	return oa;
#else
	return NULL;
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
