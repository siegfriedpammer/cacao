/* src/vm/exceptions.c - exception related functions

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

   Authors: Christian Thalinger

   Changes: Edwin Steiner

   $Id: exceptions.c 5585 2006-09-29 14:09:51Z edwin $

*/


#include "config.h"

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "vm/types.h"

#include "mm/memory.h"
#include "native/native.h"
#include "native/include/java_lang_String.h"
#include "native/include/java_lang_Throwable.h"
#include "toolbox/logging.h"
#include "toolbox/util.h"
#include "vm/class.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/jit.h"
#include "vm/jit/methodheader.h"


/* for raising exceptions from native methods *********************************/

#if !defined(ENABLE_THREADS)
java_objectheader *_no_threads_exceptionptr = NULL;
#endif


/* init_system_exceptions ******************************************************

   Load and link exceptions used in the system.

*******************************************************************************/

bool exceptions_init(void)
{
	/* java/lang/Throwable */

	if (!(class_java_lang_Throwable =
		  load_class_bootstrap(utf_java_lang_Throwable)) ||
		!link_class(class_java_lang_Throwable))
		return false;


	/* java/lang/VMThrowable */

	if (!(class_java_lang_VMThrowable =
		  load_class_bootstrap(utf_java_lang_VMThrowable)) ||
		!link_class(class_java_lang_VMThrowable))
		return false;


	/* java/lang/Error */

	if (!(class_java_lang_Error = load_class_bootstrap(utf_java_lang_Error)) ||
		!link_class(class_java_lang_Error))
		return false;

	/* java/lang/AbstractMethodError */

	if (!(class_java_lang_AbstractMethodError =
		  load_class_bootstrap(utf_java_lang_AbstractMethodError)) ||
		!link_class(class_java_lang_AbstractMethodError))
		return false;

	/* java/lang/LinkageError */

	if (!(class_java_lang_LinkageError =
		  load_class_bootstrap(utf_java_lang_LinkageError)) ||
		!link_class(class_java_lang_LinkageError))
		return false;

	/* java/lang/NoClassDefFoundError */

	if (!(class_java_lang_NoClassDefFoundError =
		  load_class_bootstrap(utf_java_lang_NoClassDefFoundError)) ||
		!link_class(class_java_lang_NoClassDefFoundError))
		return false;

	/* java/lang/NoSuchMethodError */

	if (!(class_java_lang_NoSuchMethodError =
		  load_class_bootstrap(utf_java_lang_NoSuchMethodError)) ||
		!link_class(class_java_lang_NoSuchMethodError))
		return false;

	/* java/lang/OutOfMemoryError */

	if (!(class_java_lang_OutOfMemoryError =
		  load_class_bootstrap(utf_java_lang_OutOfMemoryError)) ||
		!link_class(class_java_lang_OutOfMemoryError))
		return false;


	/* java/lang/Exception */

	if (!(class_java_lang_Exception =
		  load_class_bootstrap(utf_java_lang_Exception)) ||
		!link_class(class_java_lang_Exception))
		return false;

	/* java/lang/ClassCastException */

	if (!(class_java_lang_ClassCastException =
		  load_class_bootstrap(utf_java_lang_ClassCastException)) ||
		!link_class(class_java_lang_ClassCastException))
		return false;

	/* java/lang/ClassNotFoundException */

	if (!(class_java_lang_ClassNotFoundException =
		  load_class_bootstrap(utf_java_lang_ClassNotFoundException)) ||
		!link_class(class_java_lang_ClassNotFoundException))
		return false;

	/* java/lang/IllegalArgumentException */

	if (!(class_java_lang_IllegalArgumentException =
		  load_class_bootstrap(utf_java_lang_IllegalArgumentException)) ||
		!link_class(class_java_lang_IllegalArgumentException))
		return false;

	/* java/lang/IllegalMonitorStateException */

	if (!(class_java_lang_IllegalMonitorStateException =
		  load_class_bootstrap(utf_java_lang_IllegalMonitorStateException)) ||
		!link_class(class_java_lang_IllegalMonitorStateException))
		return false;

	/* java/lang/NullPointerException */

	if (!(class_java_lang_NullPointerException =
		  load_class_bootstrap(utf_java_lang_NullPointerException)) ||
		!link_class(class_java_lang_NullPointerException))
		return false;


	return true;
}


static void throw_exception_exit_intern(bool doexit)
{
	java_objectheader *xptr;
	classinfo *c;
	methodinfo *pss;

	xptr = *exceptionptr;

	if (xptr) {
		/* clear exception, because we are calling jit code again */
		*exceptionptr = NULL;

		c = xptr->vftbl->class;

		pss = class_resolveclassmethod(c,
									   utf_printStackTrace,
									   utf_void__void,
									   class_java_lang_Object,
									   false);

		/* print the stacktrace */

		if (pss) {
			(void) vm_call_method(pss, xptr);

			/* This normally means, we are EXTREMLY out of memory or have a   */
			/* serious problem while printStackTrace. But may be another      */
			/* exception, so print it.                                        */

			if (*exceptionptr) {
				java_lang_Throwable *t;

				t = (java_lang_Throwable *) *exceptionptr;

				fprintf(stderr, "Exception while printStackTrace(): ");
				utf_fprint_printable_ascii_classname(stderr, t->header.vftbl->class->name);

				if (t->detailMessage) {
					char *buf;

					buf = javastring_tochar((java_objectheader *) t->detailMessage);
					fprintf(stderr, ": %s", buf);
					MFREE(buf, char, strlen(buf));
				}
					
				fprintf(stderr, "\n");
			}

		} else {
			utf_fprint_printable_ascii_classname(stderr, c->name);
			fprintf(stderr, ": printStackTrace()V not found!\n");
		}

		fflush(stderr);

		/* good bye! */

		if (doexit)
			exit(1);
	}
}


void throw_exception(void)
{
	throw_exception_exit_intern(false);
}


void throw_exception_exit(void)
{
	throw_exception_exit_intern(true);
}


void throw_main_exception(void)
{
	fprintf(stderr, "Exception in thread \"main\" ");
	fflush(stderr);

	throw_exception_exit_intern(false);
}


void throw_main_exception_exit(void)
{
	fprintf(stderr, "Exception in thread \"main\" ");
	fflush(stderr);

	throw_exception_exit_intern(true);
}


void throw_cacao_exception_exit(const char *exception, const char *message, ...)
{
	s4 i;
	char *tmp;
	s4 len;
	va_list ap;

	len = strlen(exception);
	tmp = MNEW(char, len + 1);
	strncpy(tmp, exception, len);
	tmp[len] = '\0';

	/* convert to classname */

   	for (i = len - 1; i >= 0; i--)
 	 	if (tmp[i] == '/') tmp[i] = '.';

	fprintf(stderr, "Exception in thread \"main\" %s", tmp);

	MFREE(tmp, char, len);

	if (strlen(message) > 0) {
		fprintf(stderr, ": ");

		va_start(ap, message);
		vfprintf(stderr, message, ap);
		va_end(ap);
	}

	fprintf(stderr, "\n");
	fflush(stderr);

	/* good bye! */

	exit(1);
}


/* exceptions_throw_outofmemory_exit *******************************************

   Just print an: java.lang.InternalError: Out of memory

*******************************************************************************/

void exceptions_throw_outofmemory_exit(void)
{
	throw_cacao_exception_exit(string_java_lang_InternalError,
							   "Out of memory");
}


/* new_exception ***************************************************************

   Creates an exception object with the given name and initalizes it.

   IN:
      classname....class name in UTF-8

   RETURN VALUE:
      an exception pointer (in any case -- either it is the newly created
	  exception, or an exception thrown while trying to create it).

*******************************************************************************/

java_objectheader *new_exception(const char *classname)
{
	java_objectheader *o;
	classinfo         *c;

	if (!(c = load_class_bootstrap(utf_new_char(classname))))
		return *exceptionptr;

	o = native_new_and_init(c);

	if (!o)
		return *exceptionptr;

	return o;
}


/* new_exception_message *******************************************************

   Creates an exception object with the given name and initalizes it
   with the given char message.

   IN:
      classname....class name in UTF-8
	  message......message in UTF-8

   RETURN VALUE:
      an exception pointer (in any case -- either it is the newly created
	  exception, or an exception thrown while trying to create it).

*******************************************************************************/

java_objectheader *new_exception_message(const char *classname,
										 const char *message)
{
	java_lang_String *s;

	s = javastring_new_from_utf_string(message);
	if (!s)
		return *exceptionptr;

	return new_exception_javastring(classname, s);
}


/* new_exception_throwable *****************************************************

   Creates an exception object with the given name and initalizes it
   with the given java/lang/Throwable exception.

   IN:
      classname....class name in UTF-8
	  throwable....the given Throwable

   RETURN VALUE:
      an exception pointer (in any case -- either it is the newly created
	  exception, or an exception thrown while trying to create it).

*******************************************************************************/

java_objectheader *new_exception_throwable(const char *classname,
										   java_lang_Throwable *throwable)
{
	java_objectheader *o;
	classinfo         *c;
   
	if (!(c = load_class_bootstrap(utf_new_char(classname))))
		return *exceptionptr;

	o = native_new_and_init_throwable(c, throwable);

	if (!o)
		return *exceptionptr;

	return o;
}


/* new_exception_utfmessage ****************************************************

   Creates an exception object with the given name and initalizes it
   with the given utf message.

   IN:
      classname....class name in UTF-8
	  message......the message as an utf *

   RETURN VALUE:
      an exception pointer (in any case -- either it is the newly created
	  exception, or an exception thrown while trying to create it).

*******************************************************************************/

java_objectheader *new_exception_utfmessage(const char *classname, utf *message)
{
	java_lang_String *s;

	s = javastring_new(message);
	if (!s)
		return *exceptionptr;

	return new_exception_javastring(classname, s);
}


/* new_exception_javastring ****************************************************

   Creates an exception object with the given name and initalizes it
   with the given java/lang/String message.

   IN:
      classname....class name in UTF-8
	  message......the message as a java.lang.String

   RETURN VALUE:
      an exception pointer (in any case -- either it is the newly created
	  exception, or an exception thrown while trying to create it).

*******************************************************************************/

java_objectheader *new_exception_javastring(const char *classname,
											java_lang_String *message)
{
	java_objectheader *o;
	classinfo         *c;
   
	if (!(c = load_class_bootstrap(utf_new_char(classname))))
		return *exceptionptr;

	o = native_new_and_init_string(c, message);

	if (!o)
		return *exceptionptr;

	return o;
}


/* new_exception_int ***********************************************************

   Creates an exception object with the given name and initalizes it
   with the given int value.

   IN:
      classname....class name in UTF-8
	  i............the integer

   RETURN VALUE:
      an exception pointer (in any case -- either it is the newly created
	  exception, or an exception thrown while trying to create it).

*******************************************************************************/

java_objectheader *new_exception_int(const char *classname, s4 i)
{
	java_objectheader *o;
	classinfo         *c;
   
	if (!(c = load_class_bootstrap(utf_new_char(classname))))
		return *exceptionptr;

	o = native_new_and_init_int(c, i);

	if (!o)
		return *exceptionptr;

	return o;
}


/* exceptions_new_abstractmethoderror ******************************************

   Generates a java.lang.AbstractMethodError for the VM.

*******************************************************************************/

java_objectheader *exceptions_new_abstractmethoderror(void)
{
	java_objectheader *e;

	e = native_new_and_init(class_java_lang_AbstractMethodError);

	if (e == NULL)
		return *exceptionptr;

	return e;
}


/* exceptions_asm_new_abstractmethoderror **************************************

   Generates a java.lang.AbstractMethodError for
   asm_abstractmethoderror.

*******************************************************************************/

java_objectheader *exceptions_asm_new_abstractmethoderror(u1 *sp, u1 *ra)
{
	stackframeinfo     sfi;
	java_objectheader *e;

	/* create the stackframeinfo (XPC is equal to RA) */

	stacktrace_create_extern_stackframeinfo(&sfi, NULL, sp, ra, ra);

	/* create the exception */

	e = exceptions_new_abstractmethoderror();

	/* remove the stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	return e;
}


/* exceptions_throw_abstractmethoderror ****************************************

   Generates a java.lang.AbstractMethodError for the VM and throws it.

*******************************************************************************/

void exceptions_throw_abstractmethoderror(void)
{
	*exceptionptr = exceptions_new_abstractmethoderror();
}


/* new_classformaterror ********************************************************

   generates a java.lang.ClassFormatError for the classloader

   IN:
      c............the class in which the error was found
	  message......UTF-8 format string

   RETURN VALUE:
      an exception pointer (in any case -- either it is the newly created
	  exception, or an exception thrown while trying to create it).

*******************************************************************************/

java_objectheader *new_classformaterror(classinfo *c, const char *message, ...)
{
	java_objectheader *o;
	char              *msg;
	s4                 msglen;
	va_list            ap;

	/* calculate message length */

	msglen = 0;

	if (c)
		msglen += utf_bytes(c->name) + strlen(" (");

	va_start(ap, message);
	msglen += get_variable_message_length(message, ap);
	va_end(ap);

	if (c)
		msglen += strlen(")");

	msglen += strlen("0");

	/* allocate a buffer */

	msg = MNEW(char, msglen);

	/* print message into allocated buffer */

	if (c) {
		utf_copy_classname(msg, c->name);
		strcat(msg, " (");
	}

	va_start(ap, message);
	vsprintf(msg + strlen(msg), message, ap);
	va_end(ap);

	if (c)
		strcat(msg, ")");

	o = new_exception_message(string_java_lang_ClassFormatError, msg);

	MFREE(msg, char, msglen);

	return o;
}


/* exceptions_throw_classformaterror *******************************************

   Generate a java.lang.ClassFormatError for the VM system and throw it.

   IN:
      c............the class in which the error was found
	  message......UTF-8 format string

   RETURN VALUE:
      an exception pointer (in any case -- either it is the newly created
	  exception, or an exception thrown while trying to create it).

*******************************************************************************/

void exceptions_throw_classformaterror(classinfo *c, const char *message, ...)
{
	va_list ap;

	va_start(ap, message);
	*exceptionptr = new_classformaterror(c, message, ap);
	va_end(ap);
}


/* new_classnotfoundexception **************************************************

   Generates a java.lang.ClassNotFoundException for the classloader.

   IN:
      name.........name of the class not found as a utf *

   RETURN VALUE:
      an exception pointer (in any case -- either it is the newly created
	  exception, or an exception thrown while trying to create it).

*******************************************************************************/

java_objectheader *new_classnotfoundexception(utf *name)
{
	java_objectheader *o;
	java_lang_String  *s;

	s = javastring_new(name);
	if (!s)
		return *exceptionptr;

	o = native_new_and_init_string(class_java_lang_ClassNotFoundException, s);

	if (!o)
		return *exceptionptr;

	return o;
}


/* new_noclassdeffounderror ****************************************************

   Generates a java.lang.NoClassDefFoundError

   IN:
      name.........name of the class not found as a utf *

   RETURN VALUE:
      an exception pointer (in any case -- either it is the newly created
	  exception, or an exception thrown while trying to create it).

*******************************************************************************/

java_objectheader *new_noclassdeffounderror(utf *name)
{
	java_objectheader *o;
	java_lang_String  *s;

	s = javastring_new(name);
	if (!s)
		return *exceptionptr;

	o = native_new_and_init_string(class_java_lang_NoClassDefFoundError, s);

	if (!o)
		return *exceptionptr;

	return o;
}


/* classnotfoundexception_to_noclassdeffounderror ******************************

   Check the *exceptionptr for a ClassNotFoundException. If it is one,
   convert it to a NoClassDefFoundError.

*******************************************************************************/

void classnotfoundexception_to_noclassdeffounderror(void)
{
	java_objectheader *xptr;
	java_objectheader *cause;

	/* get the cause */

	cause = *exceptionptr;

	/* convert ClassNotFoundException's to NoClassDefFoundError's */

	if (builtin_instanceof(cause, class_java_lang_ClassNotFoundException)) {
		/* clear exception, because we are calling jit code again */

		*exceptionptr = NULL;

		/* create new error */

		xptr =
			new_exception_javastring(string_java_lang_NoClassDefFoundError,
					((java_lang_Throwable *) cause)->detailMessage);

		/* we had an exception while creating the error */

		if (*exceptionptr)
			return;

		/* set new exception */

		*exceptionptr = xptr;
	}
}


/* new_internalerror ***********************************************************

   Generates a java.lang.InternalError for the VM.

   IN:
      message......UTF-8 message format string

   RETURN VALUE:
      an exception pointer (in any case -- either it is the newly created
	  exception, or an exception thrown while trying to create it).

*******************************************************************************/

java_objectheader *new_internalerror(const char *message, ...)
{
	java_objectheader *o;
	va_list            ap;
	char              *msg;
	s4                 msglen;

	/* calculate exception message length */

	va_start(ap, message);
	msglen = get_variable_message_length(message, ap);
	va_end(ap);

	/* allocate memory */

	msg = MNEW(char, msglen);

	/* generate message */

	va_start(ap, message);
	vsprintf(msg, message, ap);
	va_end(ap);

	/* create exception object */

	o = new_exception_message(string_java_lang_InternalError, msg);

	/* free memory */

	MFREE(msg, char, msglen);

	return o;
}


/* exceptions_new_linkageerror *************************************************

   Generates a java.lang.LinkageError with an error message.

   IN:
      message......UTF-8 message
	  c............class related to the error. If this is != NULL
	               the name of c is appended to the error message.

   RETURN VALUE:
      an exception pointer (in any case -- either it is the newly created
	  exception, or an exception thrown while trying to create it).

*******************************************************************************/

java_objectheader *exceptions_new_linkageerror(const char *message,
											   classinfo *c)
{
	java_objectheader *o;
	char              *msg;
	s4                 msglen;

	/* calculate exception message length */

	msglen = strlen(message) + 1;
	if (c) {
		msglen += utf_bytes(c->name);
	}
		
	/* allocate memory */

	msg = MNEW(char, msglen);

	/* generate message */

	strcpy(msg,message);
	if (c) {
		utf_cat_classname(msg, c->name);
	}

	o = native_new_and_init_string(class_java_lang_LinkageError,
								   javastring_new_from_utf_string(msg));

	/* free memory */

	MFREE(msg, char, msglen);

	if (!o)
		return *exceptionptr;

	return o;
}


/* exceptions_new_nosuchmethoderror ********************************************

   Generates a java.lang.NoSuchMethodError with an error message.

   IN:
      c............class in which the method was not found
	  name.........name of the method
	  desc.........descriptor of the method

   RETURN VALUE:
      an exception pointer (in any case -- either it is the newly created
	  exception, or an exception thrown while trying to create it).

*******************************************************************************/

java_objectheader *exceptions_new_nosuchmethoderror(classinfo *c,
													utf *name, utf *desc)
{
	java_objectheader *o;
	char              *msg;
	s4                 msglen;

	/* calculate exception message length */

	msglen = utf_bytes(c->name) + strlen(".") + utf_bytes(name) +
		utf_bytes(desc) + strlen("0");

	/* allocate memory */

	msg = MNEW(char, msglen);

	/* generate message */

	utf_copy_classname(msg, c->name);
	strcat(msg, ".");
	utf_cat(msg, name);
	utf_cat(msg, desc);

	o = native_new_and_init_string(class_java_lang_NoSuchMethodError,
								   javastring_new_from_utf_string(msg));

	/* free memory */

	MFREE(msg, char, msglen);

	if (!o)
		return *exceptionptr;

	return o;
}


/* exceptions_throw_nosuchmethoderror ******************************************

   Generates a java.lang.NoSuchMethodError with an error message.

   IN:
      c............class in which the method was not found
	  name.........name of the method
	  desc.........descriptor of the method

*******************************************************************************/

void exceptions_throw_nosuchmethoderror(classinfo *c, utf *name, utf *desc)
{
	*exceptionptr = exceptions_new_nosuchmethoderror(c, name, desc);
}


/* new_unsupportedclassversionerror ********************************************

   Generate a java.lang.UnsupportedClassVersionError for the classloader

   IN:
      c............class in which the method was not found
	  message......UTF-8 format string

   RETURN VALUE:
      an exception pointer (in any case -- either it is the newly created
	  exception, or an exception thrown while trying to create it).

*******************************************************************************/

java_objectheader *new_unsupportedclassversionerror(classinfo *c, const char *message, ...)
{
	java_objectheader *o;
	va_list            ap;
	char              *msg;
    s4                 msglen;

	/* calculate exception message length */

	msglen = utf_bytes(c->name) + strlen(" (") + strlen(")") + strlen("0");

	va_start(ap, message);
	msglen += get_variable_message_length(message, ap);
	va_end(ap);

	/* allocate memory */

	msg = MNEW(char, msglen);

	/* generate message */

	utf_copy_classname(msg, c->name);
	strcat(msg, " (");

	va_start(ap, message);
	vsprintf(msg + strlen(msg), message, ap);
	va_end(ap);

	strcat(msg, ")");

	/* create exception object */

	o = new_exception_message(string_java_lang_UnsupportedClassVersionError,
							  msg);

	/* free memory */

	MFREE(msg, char, msglen);

	return o;
}


/* exceptions_new_verifyerror **************************************************

   Generates a java.lang.VerifyError for the JIT compiler.

   IN:
      m............method in which the error was found
	  message......UTF-8 format string

   RETURN VALUE:
      an exception pointer (in any case -- either it is the newly created
	  exception, or an exception thrown while trying to create it).

*******************************************************************************/

java_objectheader *exceptions_new_verifyerror(methodinfo *m,
											  const char *message, ...)
{
	java_objectheader *o;
	va_list            ap;
	char              *msg;
	s4                 msglen;

	useinlining = false; /* at least until sure inlining works with exceptions*/

	/* calculate exception message length */

	msglen = 0;

	if (m)
		msglen = strlen("(class: ") + utf_bytes(m->class->name) +
			strlen(", method: ") + utf_bytes(m->name) +
			strlen(" signature: ") + utf_bytes(m->descriptor) +
			strlen(") ") + strlen("0");

	va_start(ap, message);
	msglen += get_variable_message_length(message, ap);
	va_end(ap);

	/* allocate memory */

	msg = MNEW(char, msglen);

	/* generate message */

	if (m) {
		strcpy(msg, "(class: ");
		utf_cat_classname(msg, m->class->name);
		strcat(msg, ", method: ");
		utf_cat(msg, m->name);
		strcat(msg, " signature: ");
		utf_cat(msg, m->descriptor);
		strcat(msg, ") ");
	}

	va_start(ap, message);
	vsprintf(msg + strlen(msg), message, ap);
	va_end(ap);

	/* create exception object */

	o = new_exception_message(string_java_lang_VerifyError, msg);

	/* free memory */

	MFREE(msg, char, msglen);

	return o;
}


/* exceptions_throw_verifyerror ************************************************

   Throws a java.lang.VerifyError for the VM system.

*******************************************************************************/

void exceptions_throw_verifyerror(methodinfo *m, const char *message, ...)
{
	*exceptionptr = exceptions_new_verifyerror(m, message);
}


/* exceptions_throw_verifyerror_for_stack **************************************

   throws a java.lang.VerifyError for an invalid stack slot type

   IN:
      m............method in which the error was found
	  type.........the expected type

   RETURN VALUE:
      an exception pointer (in any case -- either it is the newly created
	  exception, or an exception thrown while trying to create it).

*******************************************************************************/

void exceptions_throw_verifyerror_for_stack(methodinfo *m,int type)
{
	java_objectheader *o;
	char              *msg;
	s4                 msglen;
	char              *typename;

	/* calculate exception message length */

	msglen = 0;

	if (m)
		msglen = strlen("(class: ") + utf_bytes(m->class->name) +
			strlen(", method: ") + utf_bytes(m->name) +
			strlen(" signature: ") + utf_bytes(m->descriptor) +
			strlen(") Expecting to find longest-------typename on stack") 
			+ strlen("0");

	/* allocate memory */

	msg = MNEW(char, msglen);

	/* generate message */

	if (m) {
		strcpy(msg, "(class: ");
		utf_cat_classname(msg, m->class->name);
		strcat(msg, ", method: ");
		utf_cat(msg, m->name);
		strcat(msg, " signature: ");
		utf_cat(msg, m->descriptor);
		strcat(msg, ") ");
	}
	else {
		msg[0] = 0;
	}

	strcat(msg,"Expecting to find ");
	switch (type) {
		case TYPE_INT: typename = "integer"; break;
		case TYPE_LNG: typename = "long"; break;
		case TYPE_FLT: typename = "float"; break;
		case TYPE_DBL: typename = "double"; break;
		case TYPE_ADR: typename = "object/array"; break;
		default:       typename = "<INVALID>"; assert(0); break;
	}
	strcat(msg, typename);
	strcat(msg, " on stack");

	/* create exception object */

	o = new_exception_message(string_java_lang_VerifyError, msg);

	/* free memory */

	MFREE(msg, char, msglen);

	*exceptionptr = o;
}


/* new_arithmeticexception *****************************************************

   Generates a java.lang.ArithmeticException for the jit compiler.

*******************************************************************************/

java_objectheader *new_arithmeticexception(void)
{
	java_objectheader *e;

	e = new_exception_message(string_java_lang_ArithmeticException,
							  string_java_lang_ArithmeticException_message);

	if (!e)
		return *exceptionptr;

	return e;
}


/* exceptions_new_arrayindexoutofboundsexception *******************************

   Generates a java.lang.ArrayIndexOutOfBoundsException for the VM
   system.

*******************************************************************************/

java_objectheader *new_arrayindexoutofboundsexception(s4 index)
{
	java_objectheader *e;
	methodinfo        *m;
	java_objectheader *o;
	java_lang_String  *s;

	/* convert the index into a String, like Sun does */

	m = class_resolveclassmethod(class_java_lang_String,
								 utf_new_char("valueOf"),
								 utf_new_char("(I)Ljava/lang/String;"),
								 class_java_lang_Object,
								 true);

	if (m == NULL)
		return *exceptionptr;

	o = vm_call_method(m, NULL, index);

	s = (java_lang_String *) o;

	if (s == NULL)
		return *exceptionptr;

	e = new_exception_javastring(string_java_lang_ArrayIndexOutOfBoundsException,
								 s);

	if (e == NULL)
		return *exceptionptr;

	return e;
}


/* exceptions_throw_arrayindexoutofboundsexception *****************************

   Generates a java.lang.ArrayIndexOutOfBoundsException for the VM
   system.

*******************************************************************************/

void exceptions_throw_arrayindexoutofboundsexception(void)
{
	java_objectheader *e;

	e = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

	if (!e)
		return;

	*exceptionptr = e;
}


/* exceptions_new_arraystoreexception ******************************************

   Generates a java.lang.ArrayStoreException for the VM compiler.

*******************************************************************************/

java_objectheader *exceptions_new_arraystoreexception(void)
{
	java_objectheader *e;

	e = new_exception(string_java_lang_ArrayStoreException);
/*  	e = native_new_and_init(class_java_lang_ArrayStoreException); */

	if (!e)
		return *exceptionptr;

	return e;
}


/* exceptions_throw_arraystoreexception ****************************************

   Generates a java.lang.ArrayStoreException for the VM system and
   throw it in the VM system.

*******************************************************************************/

void exceptions_throw_arraystoreexception(void)
{
	*exceptionptr = exceptions_new_arraystoreexception();
}


/* exceptions_new_classcastexception *******************************************

   Generates a java.lang.ClassCastException for the JIT compiler.

*******************************************************************************/

java_objectheader *exceptions_new_classcastexception(java_objectheader *o)
{
	java_objectheader *e;
	utf               *classname;
	java_lang_String  *s;

	classname = o->vftbl->class->name;

	s = javastring_new(classname);

	e = native_new_and_init_string(class_java_lang_ClassCastException, s);

	if (e == NULL)
		return *exceptionptr;

	return e;
}


/* exceptions_new_illegalargumentexception *************************************

   Generates a java.lang.IllegalArgumentException for the VM system.

*******************************************************************************/

java_objectheader *new_illegalargumentexception(void)
{
	java_objectheader *e;

	e = native_new_and_init(class_java_lang_IllegalArgumentException);

	if (!e)
		return *exceptionptr;

	return e;
}


/* exceptions_throw_illegalargumentexception ***********************************

   Generates a java.lang.IllegalArgumentException for the VM system
   and throw it in the VM system.

*******************************************************************************/

void exceptions_throw_illegalargumentexception(void)
{
	*exceptionptr = new_illegalargumentexception();
}


/* exceptions_new_illegalmonitorstateexception *********************************

   Generates a java.lang.IllegalMonitorStateException for the VM
   thread system.

*******************************************************************************/

java_objectheader *exceptions_new_illegalmonitorstateexception(void)
{
	java_objectheader *e;

	e = native_new_and_init(class_java_lang_IllegalMonitorStateException);

	if (e == NULL)
		return *exceptionptr;

	return e;
}


/* exceptions_throw_illegalmonitorstateexception *******************************

   Generates a java.lang.IllegalMonitorStateException for the VM
   system and throw it in the VM system.

*******************************************************************************/

void exceptions_throw_illegalmonitorstateexception(void)
{
	*exceptionptr = exceptions_new_illegalmonitorstateexception();
}


/* exceptions_new_negativearraysizeexception ***********************************

   Generates a java.lang.NegativeArraySizeException for the VM system.

*******************************************************************************/

java_objectheader *new_negativearraysizeexception(void)
{
	java_objectheader *e;

	e = new_exception(string_java_lang_NegativeArraySizeException);

	if (!e)
		return *exceptionptr;

	return e;
}


/* exceptions_throw_negativearraysizeexception *********************************

   Generates a java.lang.NegativeArraySizeException for the VM system.

*******************************************************************************/

void exceptions_throw_negativearraysizeexception(void)
{
	*exceptionptr = new_negativearraysizeexception();
}


/* new_nullpointerexception ****************************************************

   generates a java.lang.NullPointerException for the jit compiler

*******************************************************************************/

java_objectheader *new_nullpointerexception(void)
{
	java_objectheader *e;

	e = native_new_and_init(class_java_lang_NullPointerException);

	if (!e)
		return *exceptionptr;

	return e;
}


/* exceptions_throw_nullpointerexception ***************************************

   Generates a java.lang.NullPointerException for the VM system and
   throw it in the VM system.

*******************************************************************************/

void exceptions_throw_nullpointerexception(void)
{
	*exceptionptr = new_nullpointerexception();
}


/* exceptions_new_stringindexoutofboundsexception ******************************

   Generates a java.lang.StringIndexOutOfBoundsException for the VM
   system.

*******************************************************************************/

java_objectheader *exceptions_new_stringindexoutofboundsexception(void)
{
	java_objectheader *e;

	e = new_exception(string_java_lang_StringIndexOutOfBoundsException);

	if (e == NULL)
		return *exceptionptr;

	return e;
}


/* exceptions_throw_stringindexoutofboundsexception ****************************

   Throws a java.lang.StringIndexOutOfBoundsException for the VM
   system.

*******************************************************************************/

void exceptions_throw_stringindexoutofboundsexception(void)
{
	*exceptionptr = exceptions_new_stringindexoutofboundsexception();
}


/* exceptions_get_and_clear_exception ******************************************

   Gets the exception pointer of the current thread and clears it.
   This function may return NULL.

*******************************************************************************/

java_objectheader *exceptions_get_and_clear_exception(void)
{
	java_objectheader **p;
	java_objectheader  *e;

	/* get the pointer of the exception pointer */

	p = exceptionptr;

	/* get the exception */

	e = *p;

	/* and clear the exception */

	*p = NULL;

	/* return the exception */

	return e;
}


/* exceptions_handle_exception *************************************************

   Try to find an exception handler for the given exception and return it.
   If no handler is found, exit the monitor of the method (if any)
   and return NULL.

   IN:
      xptr.........the exception object
	  xpc..........PC of where the exception was thrown
	  pv...........Procedure Value of the current method
	  sp...........current stack pointer

   RETURN VALUE:
      the address of the first matching exception handler, or
	  NULL if no handler was found

*******************************************************************************/

u1 *exceptions_handle_exception(java_objectheader *xptr, u1 *xpc, u1 *pv, u1 *sp)
{
	methodinfo            *m;
	codeinfo              *code;
	s4                     framesize;
	s4                     issync;
	exceptionentry        *ex;
	s4                     exceptiontablelength;
	s4                     i;
	classref_or_classinfo  cr;
	classinfo             *c;
#if defined(ENABLE_THREADS)
	java_objectheader     *o;
#endif

	/* get info from the method header */

	code                 = *((codeinfo **)      (pv + CodeinfoPointer));
	framesize            = *((s4 *)             (pv + FrameSize));
	issync               = *((s4 *)             (pv + IsSync));
	ex                   =   (exceptionentry *) (pv + ExTableStart);
	exceptiontablelength = *((s4 *)             (pv + ExTableSize));

	/* Get the methodinfo pointer from the codeinfo pointer. For
	   asm_vm_call_method the codeinfo pointer is NULL. */

	m = (code == NULL) ? NULL : code->m;

#if !defined(NDEBUG)
	/* print exception trace */

	if (opt_verbose || opt_verbosecall || opt_verboseexception)
		builtin_trace_exception(xptr, m, xpc, 1);
#endif

	for (i = 0; i < exceptiontablelength; i++) {
		/* ATTENTION: keep this here, as we need to decrement the
           pointer before the loop executes! */

		ex--;

		/* If the start and end PC is NULL, this means we have the
		   special case of asm_vm_call_method.  So, just return the
		   proper exception handler. */

		if ((ex->startpc == NULL) && (ex->endpc == NULL))
			return (u1 *) (ptrint) &asm_vm_call_method_exception_handler;

		/* is the xpc is the current catch range */

		if ((ex->startpc <= xpc) && (xpc < ex->endpc)) {
			cr = ex->catchtype;

			/* NULL catches everything */

			if (cr.any == NULL) {
#if !defined(NDEBUG)
				/* Print stacktrace of exception when caught. */

				if (opt_verboseexception) {
					exceptions_print_exception(xptr);
					stacktrace_print_trace(xptr);
				}
#endif

				return ex->handlerpc;
			}

			/* resolve or load/link the exception class */

			if (IS_CLASSREF(cr)) {
				/* The exception class reference is unresolved. */
				/* We have to do _eager_ resolving here. While the class of */
				/* the exception object is guaranteed to be loaded, it may  */
				/* well have been loaded by a different loader than the     */
				/* defining loader of m's class, which is the one we must   */
				/* use to resolve the catch class. Thus lazy resolving      */
				/* might fail, even if the result of the resolution would   */
				/* be an already loaded class.                              */

				c = resolve_classref_eager(cr.ref);

				if (c == NULL) {
					/* Exception resolving the exception class, argh! */
					return NULL;
				}

				/* Ok, we resolved it. Enter it in the table, so we don't */
				/* have to do this again.                                 */
				/* XXX this write should be atomic. Is it?                */

				ex->catchtype.cls = c;
			} else {
				c = cr.cls;

				/* XXX I don't think this case can ever happen. -Edwin */
				if (!(c->state & CLASS_LOADED))
					/* use the methods' classloader */
					if (!load_class_from_classloader(c->name,
													 m->class->classloader))
						return NULL;

				/* XXX I think, if it is not linked, we can be sure that     */
				/* the exception object is no (indirect) instance of it, no? */
				/* -Edwin                                                    */
				if (!(c->state & CLASS_LINKED))
					if (!link_class(c))
						return NULL;
			}

			/* is the thrown exception an instance of the catch class? */

			if (builtin_instanceof(xptr, c)) {
#if !defined(NDEBUG)
				/* Print stacktrace of exception when caught. */

				if (opt_verboseexception) {
					exceptions_print_exception(xptr);
					stacktrace_print_trace(xptr);
				}
#endif

				return ex->handlerpc;
			}
		}
	}

#if defined(ENABLE_THREADS)
	/* is this method synchronized? */

	if (issync) {
		/* get synchronization object */

# if defined(__MIPS__) && (SIZEOF_VOID_P == 4)
		/* XXX change this if we ever want to use 4-byte stackslots */
		o = *((java_objectheader **) (sp + issync - 8));
# else
		o = *((java_objectheader **) (sp + issync - SIZEOF_VOID_P));
#endif

		assert(o != NULL);

		lock_monitor_exit(o);
	}
#endif

	/* none of the exceptions catch this one */

	return NULL;
}


/* exceptions_print_exception **************************************************

   Prints an exception, the detail message and the cause, if
   available, with CACAO internal functions to stdout.

*******************************************************************************/

#if !defined(NDEBUG)
void exceptions_print_exception(java_objectheader *xptr)
{
	java_lang_Throwable   *t;
	java_lang_Throwable   *cause;
	utf                   *u;

	t = (java_lang_Throwable *) xptr;

	if (t == NULL) {
		puts("NULL\n");
		return;
	}

	cause = t->cause;

	/* print the root exception */

	utf_display_printable_ascii_classname(t->header.vftbl->class->name);

	if (t->detailMessage) {
		u = javastring_toutf(t->detailMessage, false);

		printf(": ");
		utf_display_printable_ascii(u);
	}

	putc('\n', stdout);

	/* print the cause if available */

	if (cause && (cause != t)) {
		printf("Caused by: ");
		utf_display_printable_ascii_classname(cause->header.vftbl->class->name);

		if (cause->detailMessage) {
			u = javastring_toutf(cause->detailMessage, false);

			printf(": ");
			utf_display_printable_ascii(u);
		}

		putc('\n', stdout);
	}
}
#endif /* !defined(NDEBUG) */


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
 * vim:noexpandtab:sw=4:ts=4:
 */
