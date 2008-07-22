/* src/vm/exceptions.c - exception related functions

   Copyright (C) 1996-2005, 2006, 2007, 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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

*/


#include "config.h"

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "vm/types.h"

#include "md-abi.h"

#include "mm/memory.h"

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"

#include "native/include/java_lang_String.h"
#include "native/include/java_lang_Thread.h"
#include "native/include/java_lang_Throwable.h"

#include "threads/lock-common.h"
#include "threads/thread.hpp"

#include "toolbox/util.h"

#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/stringlocal.h"
#include "vm/vm.hpp"

#include "vm/jit/asmpart.h"
#include "vm/jit/jit.h"
#include "vm/jit/methodheader.h"
#include "vm/jit/patcher-common.h"
#include "vm/jit/show.h"
#include "vm/jit/stacktrace.hpp"
#include "vm/jit/trace.hpp"

#include "vmcore/class.h"
#include "vmcore/globals.hpp"
#include "vmcore/loader.h"
#include "vmcore/method.h"
#include "vmcore/options.h"
#include "vmcore/system.h"

#if defined(ENABLE_VMLOG)
#include <vmlog_cacao.h>
#endif


/* for raising exceptions from native methods *********************************/

#if !defined(ENABLE_THREADS)
java_object_t *_no_threads_exceptionptr = NULL;
#endif


/* exceptions_get_exception ****************************************************

   Returns the current exception pointer of the current thread.

*******************************************************************************/

java_handle_t *exceptions_get_exception(void)
{
	java_object_t *o;
	java_handle_t *e;
#if defined(ENABLE_THREADS)
	threadobject  *t;

	t = THREADOBJECT;
#endif

	/* Get the exception. */

	LLNI_CRITICAL_START;

#if defined(ENABLE_THREADS)
	o = t->_exceptionptr;
#else
	o = _no_threads_exceptionptr;
#endif

	e = LLNI_WRAP(o);

	LLNI_CRITICAL_END;

	/* Return the exception. */

	return e;
}


/* exceptions_set_exception ****************************************************

   Sets the exception pointer of the current thread.

*******************************************************************************/

void exceptions_set_exception(java_handle_t *e)
{
	threadobject  *t;
	java_object_t *o;

#if defined(ENABLE_THREADS)
	t = THREADOBJECT;
#else
	t = NULL;
#endif

	/* Set the exception. */

	LLNI_CRITICAL_START;

	o = LLNI_UNWRAP(e);

#if !defined(NDEBUG)
	if (opt_DebugExceptions) {
		printf("[exceptions_set_exception  : t=%p, o=%p, class=",
			   (void *) t, (void *) o);
		class_print(o->vftbl->clazz);
		printf("]\n");
	}
#endif

#if defined(ENABLE_THREADS)
	t->_exceptionptr = o;
#else
	_no_threads_exceptionptr = o;
#endif

	LLNI_CRITICAL_END;
}


/* exceptions_clear_exception **************************************************

   Clears the current exception pointer of the current thread.

*******************************************************************************/

void exceptions_clear_exception(void)
{
	threadobject *t;

#if defined(ENABLE_THREADS)
	t = THREADOBJECT;
#else
	t = NULL;
#endif

	/* Set the exception. */

#if !defined(NDEBUG)
	if (opt_DebugExceptions) {
		printf("[exceptions_clear_exception: t=%p]\n", (void *) t);
	}
#endif

#if defined(ENABLE_THREADS)
	t->_exceptionptr = NULL;
#else
	_no_threads_exceptionptr = NULL;
#endif
}


/* exceptions_get_and_clear_exception ******************************************

   Gets the exception pointer of the current thread and clears it.
   This function may return NULL.

*******************************************************************************/

java_handle_t *exceptions_get_and_clear_exception(void)
{
	java_handle_t *o;

	/* Get the exception... */

	o = exceptions_get_exception();

	/* ...and clear the exception if it is set. */

	if (o != NULL)
		exceptions_clear_exception();

	/* return the exception */

	return o;
}


/* exceptions_abort ************************************************************

   Prints exception to be thrown and aborts.

   IN:
      classname....class name
      message......exception message

*******************************************************************************/

static void exceptions_abort(utf *classname, utf *message)
{
	log_println("exception thrown while VM is initializing: ");

	log_start();
	utf_display_printable_ascii_classname(classname);

	if (message != NULL) {
		log_print(": ");
		utf_display_printable_ascii_classname(message);
	}

	log_finish();

	vm_abort("Aborting...");
}


/* exceptions_new_class_utf ****************************************************

   Creates an exception object with the given class and initalizes it
   with the given utf message.

   IN:
      c ......... exception class
	  message ... the message as an utf *

   RETURN VALUE:
     an exception pointer (in any case -- either it is the newly
     created exception, or an exception thrown while trying to create
     it).

*******************************************************************************/

static java_handle_t *exceptions_new_class_utf(classinfo *c, utf *message)
{
	java_handle_t *s;
	java_handle_t *o;

	if (VM_is_initializing()) {
		/* This can happen when global class variables are used which
		   are not initialized yet. */

		if (c == NULL)
			exceptions_abort(NULL, message);
		else
			exceptions_abort(c->name, message);
	}

	s = javastring_new(message);

	if (s == NULL)
		return exceptions_get_exception();

	o = native_new_and_init_string(c, s);

	if (o == NULL)
		return exceptions_get_exception();

	return o;
}


/* exceptions_new_utf **********************************************************

   Creates an exception object with the given name and initalizes it.

   IN:
      classname....class name in UTF-8

*******************************************************************************/

static java_handle_t *exceptions_new_utf(utf *classname)
{
	classinfo     *c;
	java_handle_t *o;

	if (VM_is_initializing())
		exceptions_abort(classname, NULL);

	c = load_class_bootstrap(classname);

	if (c == NULL)
		return exceptions_get_exception();

	o = native_new_and_init(c);

	if (o == NULL)
		return exceptions_get_exception();

	return o;
}


/* exceptions_new_utf_javastring ***********************************************

   Creates an exception object with the given name and initalizes it
   with the given java/lang/String message.

   IN:
      classname....class name in UTF-8
	  message......the message as a java.lang.String

   RETURN VALUE:
      an exception pointer (in any case -- either it is the newly created
	  exception, or an exception thrown while trying to create it).

*******************************************************************************/

static java_handle_t *exceptions_new_utf_javastring(utf *classname,
													java_handle_t *message)
{
	java_handle_t *o;
	classinfo     *c;
   
	if (VM_is_initializing())
		exceptions_abort(classname, NULL);

	c = load_class_bootstrap(classname);

	if (c == NULL)
		return exceptions_get_exception();

	o = native_new_and_init_string(c, message);

	if (o == NULL)
		return exceptions_get_exception();

	return o;
}


/* exceptions_new_utf_utf ******************************************************

   Creates an exception object with the given name and initalizes it
   with the given utf message.

   IN:
      classname....class name in UTF-8
	  message......the message as an utf *

   RETURN VALUE:
      an exception pointer (in any case -- either it is the newly created
	  exception, or an exception thrown while trying to create it).

*******************************************************************************/

static java_handle_t *exceptions_new_utf_utf(utf *classname, utf *message)
{
	classinfo     *c;
	java_handle_t *o;

	if (VM_is_initializing())
		exceptions_abort(classname, message);

	c = load_class_bootstrap(classname);

	if (c == NULL)
		return exceptions_get_exception();

	o = exceptions_new_class_utf(c, message);

	return o;
}


/* exceptions_throw_class_utf **************************************************

   Creates an exception object with the given class, initalizes and
   throws it with the given utf message.

   IN:
      c ......... exception class
	  message ... the message as an utf *

*******************************************************************************/

static void exceptions_throw_class_utf(classinfo *c, utf *message)
{
	java_handle_t *o;

	o = exceptions_new_class_utf(c, message);

	exceptions_set_exception(o);
}


/* exceptions_throw_utf ********************************************************

   Creates an exception object with the given name, initalizes and
   throws it.

   IN:
      classname....class name in UTF-8

*******************************************************************************/

static void exceptions_throw_utf(utf *classname)
{
	java_handle_t *o;

	o = exceptions_new_utf(classname);

	if (o == NULL)
		return;

	exceptions_set_exception(o);
}


/* exceptions_throw_utf_throwable **********************************************

   Creates an exception object with the given name and initalizes it
   with the given java/lang/Throwable exception.

   IN:
      classname....class name in UTF-8
	  cause........the given Throwable

*******************************************************************************/

static void exceptions_throw_utf_throwable(utf *classname,
										   java_handle_t *cause)
{
	classinfo           *c;
	java_handle_t       *o;
	methodinfo          *m;
	java_lang_Throwable *object;

	if (VM_is_initializing())
		exceptions_abort(classname, NULL);

	object = (java_lang_Throwable *) cause;

	c = load_class_bootstrap(classname);

	if (c == NULL)
		return;

	/* create object */

	o = builtin_new(c);
	
	if (o == NULL)
		return;

	/* call initializer */

	m = class_resolveclassmethod(c,
								 utf_init,
								 utf_java_lang_Throwable__void,
								 NULL,
								 true);
	                      	                      
	if (m == NULL)
		return;

	(void) vm_call_method(m, o, cause);

	exceptions_set_exception(o);
}


/* exceptions_throw_utf_exception **********************************************

   Creates an exception object with the given name and initalizes it
   with the given java/lang/Exception exception.

   IN:
      classname....class name in UTF-8
	  exception....the given Exception

*******************************************************************************/

static void exceptions_throw_utf_exception(utf *classname,
										   java_handle_t *exception)
{
	classinfo     *c;
	java_handle_t *o;
	methodinfo    *m;

	if (VM_is_initializing())
		exceptions_abort(classname, NULL);

	c = load_class_bootstrap(classname);

	if (c == NULL)
		return;

	/* create object */

	o = builtin_new(c);
	
	if (o == NULL)
		return;

	/* call initializer */

	m = class_resolveclassmethod(c,
								 utf_init,
								 utf_java_lang_Exception__V,
								 NULL,
								 true);
	                      	                      
	if (m == NULL)
		return;

	(void) vm_call_method(m, o, exception);

	exceptions_set_exception(o);
}


/* exceptions_throw_utf_cause **************************************************

   Creates an exception object with the given name and initalizes it
   with the given java/lang/Throwable exception with initCause.

   IN:
      classname....class name in UTF-8
	  cause........the given Throwable

*******************************************************************************/

static void exceptions_throw_utf_cause(utf *classname, java_handle_t *cause)
{
	classinfo           *c;
	java_handle_t       *o;
	methodinfo          *m;
	java_lang_String    *s;
	java_lang_Throwable *object;

	if (VM_is_initializing())
		exceptions_abort(classname, NULL);

	object = (java_lang_Throwable *) cause;

	c = load_class_bootstrap(classname);

	if (c == NULL)
		return;

	/* create object */

	o = builtin_new(c);
	
	if (o == NULL)
		return;

	/* call initializer */

	m = class_resolveclassmethod(c,
								 utf_init,
								 utf_java_lang_String__void,
								 NULL,
								 true);
	                      	                      
	if (m == NULL)
		return;

	LLNI_field_get_ref(object, detailMessage, s);

	(void) vm_call_method(m, o, s);

	/* call initCause */

	m = class_resolveclassmethod(c,
								 utf_initCause,
								 utf_java_lang_Throwable__java_lang_Throwable,
								 NULL,
								 true);

	if (m == NULL)
		return;

	(void) vm_call_method(m, o, cause);

	exceptions_set_exception(o);
}


/* exceptions_throw_utf_utf ****************************************************

   Creates an exception object with the given name, initalizes and
   throws it with the given utf message.

   IN:
      classname....class name in UTF-8
	  message......the message as an utf *

*******************************************************************************/

static void exceptions_throw_utf_utf(utf *classname, utf *message)
{
	java_handle_t *o;

	o = exceptions_new_utf_utf(classname, message);

	exceptions_set_exception(o);
}


/* exceptions_new_abstractmethoderror ****************************************

   Generates a java.lang.AbstractMethodError for the VM.

*******************************************************************************/

java_handle_t *exceptions_new_abstractmethoderror(void)
{
	java_handle_t *o;

	o = exceptions_new_utf(utf_java_lang_AbstractMethodError);

	return o;
}


/* exceptions_new_error ********************************************************

   Generates a java.lang.Error for the VM.

*******************************************************************************/

#if defined(ENABLE_JAVAME_CLDC1_1)
static java_handle_t *exceptions_new_error(utf *message)
{
	java_handle_t *o;

	o = exceptions_new_utf_utf(utf_java_lang_Error, message);

	return o;
}
#endif


/* exceptions_asm_new_abstractmethoderror **************************************

   Generates a java.lang.AbstractMethodError for
   asm_abstractmethoderror.

*******************************************************************************/

java_object_t *exceptions_asm_new_abstractmethoderror(u1 *sp, u1 *ra)
{
	stackframeinfo_t  sfi;
	java_handle_t    *e;
	java_object_t    *o;

	/* Fill and add a stackframeinfo (XPC is equal to RA). */

	stacktrace_stackframeinfo_add(&sfi, NULL, sp, ra, ra);

	/* create the exception */

#if defined(ENABLE_JAVASE)
	e = exceptions_new_abstractmethoderror();
#else
	e = exceptions_new_error(utf_java_lang_AbstractMethodError);
#endif

	/* Remove the stackframeinfo. */

	stacktrace_stackframeinfo_remove(&sfi);

	/* unwrap the exception */
	/* ATTENTION: do the this _after_ the stackframeinfo was removed */

	o = LLNI_UNWRAP(e);

	return o;
}


/* exceptions_new_arraystoreexception ******************************************

   Generates a java.lang.ArrayStoreException for the VM.

*******************************************************************************/

java_handle_t *exceptions_new_arraystoreexception(void)
{
	java_handle_t *o;

	o = exceptions_new_utf(utf_java_lang_ArrayStoreException);

	return o;
}


/* exceptions_throw_abstractmethoderror ****************************************

   Generates and throws a java.lang.AbstractMethodError for the VM.

*******************************************************************************/

void exceptions_throw_abstractmethoderror(void)
{
	exceptions_throw_utf(utf_java_lang_AbstractMethodError);
}


/* exceptions_throw_classcircularityerror **************************************

   Generates and throws a java.lang.ClassCircularityError for the
   classloader.

   IN:
      c....the class in which the error was found

*******************************************************************************/

void exceptions_throw_classcircularityerror(classinfo *c)
{
	exceptions_throw_utf_utf(utf_java_lang_ClassCircularityError, c->name);
}


/* exceptions_throw_classformaterror *******************************************

   Generates and throws a java.lang.ClassFormatError for the VM.

   IN:
      c............the class in which the error was found
	  message......UTF-8 format string

*******************************************************************************/

void exceptions_throw_classformaterror(classinfo *c, const char *message, ...)
{
	char    *msg;
	s4       msglen;
	va_list  ap;
	utf     *u;

	/* calculate message length */

	msglen = 0;

	if (c != NULL)
		msglen += utf_bytes(c->name) + strlen(" (");

	va_start(ap, message);
	msglen += get_variable_message_length(message, ap);
	va_end(ap);

	if (c != NULL)
		msglen += strlen(")");

	msglen += strlen("0");

	/* allocate a buffer */

	msg = MNEW(char, msglen);

	/* print message into allocated buffer */

	if (c != NULL) {
		utf_copy_classname(msg, c->name);
		strcat(msg, " (");
	}

	va_start(ap, message);
	vsprintf(msg + strlen(msg), message, ap);
	va_end(ap);

	if (c != NULL)
		strcat(msg, ")");

	u = utf_new_char(msg);

	/* free memory */

	MFREE(msg, char, msglen);

	/* throw exception */

	exceptions_throw_utf_utf(utf_java_lang_ClassFormatError, u);
}


/* exceptions_throw_classnotfoundexception *************************************

   Generates and throws a java.lang.ClassNotFoundException for the
   VM.

   IN:
      name.........name of the class not found as a utf *

*******************************************************************************/

void exceptions_throw_classnotfoundexception(utf *name)
{	
	exceptions_throw_class_utf(class_java_lang_ClassNotFoundException, name);
}


/* exceptions_throw_noclassdeffounderror ***************************************

   Generates and throws a java.lang.NoClassDefFoundError.

   IN:
      name.........name of the class not found as a utf *

*******************************************************************************/

void exceptions_throw_noclassdeffounderror(utf *name)
{
	exceptions_throw_utf_utf(utf_java_lang_NoClassDefFoundError, name);
}


/* exceptions_throw_noclassdeffounderror_cause *********************************

   Generates and throws a java.lang.NoClassDefFoundError with the
   given cause.

*******************************************************************************/

void exceptions_throw_noclassdeffounderror_cause(java_handle_t *cause)
{
	exceptions_throw_utf_cause(utf_java_lang_NoClassDefFoundError, cause);
}


/* exceptions_throw_noclassdeffounderror_wrong_name ****************************

   Generates and throws a java.lang.NoClassDefFoundError with a
   specific message:

   IN:
      name.........name of the class not found as a utf *

*******************************************************************************/

void exceptions_throw_noclassdeffounderror_wrong_name(classinfo *c, utf *name)
{
	char *msg;
	s4    msglen;
	utf  *u;

	msglen = utf_bytes(c->name) + strlen(" (wrong name: ") +
		utf_bytes(name) + strlen(")") + strlen("0");

	msg = MNEW(char, msglen);

	utf_copy_classname(msg, c->name);
	strcat(msg, " (wrong name: ");
	utf_cat_classname(msg, name);
	strcat(msg, ")");

	u = utf_new_char(msg);

	MFREE(msg, char, msglen);

	exceptions_throw_noclassdeffounderror(u);
}


/* exceptions_throw_exceptionininitializererror ********************************

   Generates and throws a java.lang.ExceptionInInitializerError for
   the VM.

   IN:
      cause......cause exception object

*******************************************************************************/

void exceptions_throw_exceptionininitializererror(java_handle_t *cause)
{
	exceptions_throw_utf_throwable(utf_java_lang_ExceptionInInitializerError,
								   cause);
}


/* exceptions_throw_incompatibleclasschangeerror *******************************

   Generates and throws a java.lang.IncompatibleClassChangeError for
   the VM.

   IN:
      message......UTF-8 message format string

*******************************************************************************/

void exceptions_throw_incompatibleclasschangeerror(classinfo *c, const char *message)
{
	char *msg;
	s4    msglen;
	utf  *u;

	/* calculate exception message length */

	msglen = utf_bytes(c->name) + strlen(message) + strlen("0");

	/* allocate memory */

	msg = MNEW(char, msglen);

	utf_copy_classname(msg, c->name);
	strcat(msg, message);

	u = utf_new_char(msg);

	/* free memory */

	MFREE(msg, char, msglen);

	/* throw exception */

	exceptions_throw_utf_utf(utf_java_lang_IncompatibleClassChangeError, u);
}


/* exceptions_throw_instantiationerror *****************************************

   Generates and throws a java.lang.InstantiationError for the VM.

*******************************************************************************/

void exceptions_throw_instantiationerror(classinfo *c)
{
	exceptions_throw_utf_utf(utf_java_lang_InstantiationError, c->name);
}


/* exceptions_throw_internalerror **********************************************

   Generates and throws a java.lang.InternalError for the VM.

   IN:
      message......UTF-8 message format string

*******************************************************************************/

void exceptions_throw_internalerror(const char *message, ...)
{
	va_list  ap;
	char    *msg;
	s4       msglen;
	utf     *u;

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

	u = utf_new_char(msg);

	/* free memory */

	MFREE(msg, char, msglen);

	/* throw exception */

	exceptions_throw_utf_utf(utf_java_lang_InternalError, u);
}


/* exceptions_throw_linkageerror ***********************************************

   Generates and throws java.lang.LinkageError with an error message.

   IN:
      message......UTF-8 message
	  c............class related to the error. If this is != NULL
	               the name of c is appended to the error message.

*******************************************************************************/

void exceptions_throw_linkageerror(const char *message, classinfo *c)
{
	utf  *u;
	char *msg;
	int   len;

	/* calculate exception message length */

	len = strlen(message) + 1;

	if (c != NULL)
		len += utf_bytes(c->name);
		
	/* allocate memory */

	msg = MNEW(char, len);

	/* generate message */

	strcpy(msg, message);

	if (c != NULL)
		utf_cat_classname(msg, c->name);

	u = utf_new_char(msg);

	/* free memory */

	MFREE(msg, char, len);

	exceptions_throw_utf_utf(utf_java_lang_LinkageError, u);
}


/* exceptions_throw_nosuchfielderror *******************************************

   Generates and throws a java.lang.NoSuchFieldError with an error
   message.

   IN:
      c............class in which the field was not found
	  name.........name of the field

*******************************************************************************/

void exceptions_throw_nosuchfielderror(classinfo *c, utf *name)
{
	char *msg;
	s4    msglen;
	utf  *u;

	/* calculate exception message length */

	msglen = utf_bytes(c->name) + strlen(".") + utf_bytes(name) + strlen("0");

	/* allocate memory */

	msg = MNEW(char, msglen);

	/* generate message */

	utf_copy_classname(msg, c->name);
	strcat(msg, ".");
	utf_cat(msg, name);

	u = utf_new_char(msg);

	/* free memory */

	MFREE(msg, char, msglen);

	exceptions_throw_utf_utf(utf_java_lang_NoSuchFieldError, u);
}


/* exceptions_throw_nosuchmethoderror ******************************************

   Generates and throws a java.lang.NoSuchMethodError with an error
   message.

   IN:
      c............class in which the method was not found
	  name.........name of the method
	  desc.........descriptor of the method

*******************************************************************************/

void exceptions_throw_nosuchmethoderror(classinfo *c, utf *name, utf *desc)
{
	char *msg;
	s4    msglen;
	utf  *u;

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

	u = utf_new_char(msg);

	/* free memory */

	MFREE(msg, char, msglen);

#if defined(ENABLE_JAVASE)
	exceptions_throw_utf_utf(utf_java_lang_NoSuchMethodError, u);
#else
	exceptions_throw_utf_utf(utf_java_lang_Error, u);
#endif
}


/* exceptions_throw_outofmemoryerror *******************************************

   Generates and throws an java.lang.OutOfMemoryError for the VM.

*******************************************************************************/

void exceptions_throw_outofmemoryerror(void)
{
	exceptions_throw_utf(utf_java_lang_OutOfMemoryError);
}


/* exceptions_throw_unsatisfiedlinkerror ***************************************

   Generates and throws a java.lang.UnsatisfiedLinkError for the
   classloader.

   IN:
	  name......UTF-8 name string

*******************************************************************************/

void exceptions_throw_unsatisfiedlinkerror(utf *name)
{
#if defined(ENABLE_JAVASE)
	exceptions_throw_utf_utf(utf_java_lang_UnsatisfiedLinkError, name);
#else
	exceptions_throw_utf_utf(utf_java_lang_Error, name);
#endif
}


/* exceptions_throw_unsupportedclassversionerror *******************************

   Generates and throws a java.lang.UnsupportedClassVersionError for
   the classloader.

   IN:
      c............class in which the method was not found
	  message......UTF-8 format string

*******************************************************************************/

void exceptions_throw_unsupportedclassversionerror(classinfo *c, u4 ma, u4 mi)
{
	char *msg;
    s4    msglen;
	utf  *u;

	/* calculate exception message length */

	msglen =
		utf_bytes(c->name) +
		strlen(" (Unsupported major.minor version 00.0)") +
		strlen("0");

	/* allocate memory */

	msg = MNEW(char, msglen);

	/* generate message */

	utf_copy_classname(msg, c->name);
	sprintf(msg + strlen(msg), " (Unsupported major.minor version %d.%d)",
			ma, mi);

	u = utf_new_char(msg);

	/* free memory */

	MFREE(msg, char, msglen);

	/* throw exception */

	exceptions_throw_utf_utf(utf_java_lang_UnsupportedClassVersionError, u);
}


/* exceptions_throw_verifyerror ************************************************

   Generates and throws a java.lang.VerifyError for the JIT compiler.

   IN:
      m............method in which the error was found
	  message......UTF-8 format string

*******************************************************************************/

void exceptions_throw_verifyerror(methodinfo *m, const char *message, ...)
{
	va_list  ap;
	char    *msg;
	s4       msglen;
	utf     *u;

	/* calculate exception message length */

	msglen = 0;

	if (m != NULL)
		msglen =
			strlen("(class: ") + utf_bytes(m->clazz->name) +
			strlen(", method: ") + utf_bytes(m->name) +
			strlen(" signature: ") + utf_bytes(m->descriptor) +
			strlen(") ") + strlen("0");

	va_start(ap, message);
	msglen += get_variable_message_length(message, ap);
	va_end(ap);

	/* allocate memory */

	msg = MNEW(char, msglen);

	/* generate message */

	if (m != NULL) {
		strcpy(msg, "(class: ");
		utf_cat_classname(msg, m->clazz->name);
		strcat(msg, ", method: ");
		utf_cat(msg, m->name);
		strcat(msg, " signature: ");
		utf_cat(msg, m->descriptor);
		strcat(msg, ") ");
	}

	va_start(ap, message);
	vsprintf(msg + strlen(msg), message, ap);
	va_end(ap);

	u = utf_new_char(msg);

	/* free memory */

	MFREE(msg, char, msglen);

	/* throw exception */

	exceptions_throw_utf_utf(utf_java_lang_VerifyError, u);
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

void exceptions_throw_verifyerror_for_stack(methodinfo *m, int type)
{
	char *msg;
	s4    msglen;
	char *typename;
	utf  *u;

	/* calculate exception message length */

	msglen = 0;

	if (m != NULL)
		msglen = strlen("(class: ") + utf_bytes(m->clazz->name) +
			strlen(", method: ") + utf_bytes(m->name) +
			strlen(" signature: ") + utf_bytes(m->descriptor) +
			strlen(") Expecting to find longest-------typename on stack") 
			+ strlen("0");

	/* allocate memory */

	msg = MNEW(char, msglen);

	/* generate message */

	if (m != NULL) {
		strcpy(msg, "(class: ");
		utf_cat_classname(msg, m->clazz->name);
		strcat(msg, ", method: ");
		utf_cat(msg, m->name);
		strcat(msg, " signature: ");
		utf_cat(msg, m->descriptor);
		strcat(msg, ") ");
	}
	else {
		msg[0] = 0;
	}

	strcat(msg, "Expecting to find ");

	switch (type) {
		case TYPE_INT: typename = "integer"; break;
		case TYPE_LNG: typename = "long"; break;
		case TYPE_FLT: typename = "float"; break;
		case TYPE_DBL: typename = "double"; break;
		case TYPE_ADR: typename = "object/array"; break;
		case TYPE_RET: typename = "returnAddress"; break;
		default:       typename = "<INVALID>"; assert(0); break;
	}

	strcat(msg, typename);
	strcat(msg, " on stack");

	u = utf_new_char(msg);

	/* free memory */

	MFREE(msg, char, msglen);

	/* throw exception */

	exceptions_throw_utf_utf(utf_java_lang_VerifyError, u);
}


/* exceptions_new_arithmeticexception ******************************************

   Generates a java.lang.ArithmeticException for the JIT compiler.

*******************************************************************************/

java_handle_t *exceptions_new_arithmeticexception(void)
{
	java_handle_t *o;

	o = exceptions_new_utf_utf(utf_java_lang_ArithmeticException,
							   utf_division_by_zero);

	return o;
}


/* exceptions_new_arrayindexoutofboundsexception *******************************

   Generates a java.lang.ArrayIndexOutOfBoundsException for the VM
   system.

*******************************************************************************/

java_handle_t *exceptions_new_arrayindexoutofboundsexception(s4 index)
{
	java_handle_t *o;
	methodinfo    *m;
	java_handle_t *s;

	/* convert the index into a String, like Sun does */

	m = class_resolveclassmethod(class_java_lang_String,
								 utf_new_char("valueOf"),
								 utf_new_char("(I)Ljava/lang/String;"),
								 class_java_lang_Object,
								 true);

	if (m == NULL)
		return exceptions_get_exception();

	s = vm_call_method(m, NULL, index);

	if (s == NULL)
		return exceptions_get_exception();

	o = exceptions_new_utf_javastring(utf_java_lang_ArrayIndexOutOfBoundsException,
									  s);

	if (o == NULL)
		return exceptions_get_exception();

	return o;
}


/* exceptions_throw_arrayindexoutofboundsexception *****************************

   Generates and throws a java.lang.ArrayIndexOutOfBoundsException for
   the VM.

*******************************************************************************/

void exceptions_throw_arrayindexoutofboundsexception(void)
{
	exceptions_throw_utf(utf_java_lang_ArrayIndexOutOfBoundsException);
}


/* exceptions_throw_arraystoreexception ****************************************

   Generates and throws a java.lang.ArrayStoreException for the VM.

*******************************************************************************/

void exceptions_throw_arraystoreexception(void)
{
	exceptions_throw_utf(utf_java_lang_ArrayStoreException);
}


/* exceptions_new_classcastexception *******************************************

   Generates a java.lang.ClassCastException for the JIT compiler.

*******************************************************************************/

java_handle_t *exceptions_new_classcastexception(java_handle_t *o)
{
	java_handle_t *e;
	classinfo     *c;
	utf           *classname;

	LLNI_class_get(o, c);

	classname = c->name;

	e = exceptions_new_utf_utf(utf_java_lang_ClassCastException, classname);

	return e;
}


/* exceptions_throw_clonenotsupportedexception *********************************

   Generates and throws a java.lang.CloneNotSupportedException for the
   VM.

*******************************************************************************/

void exceptions_throw_clonenotsupportedexception(void)
{
	exceptions_throw_utf(utf_java_lang_CloneNotSupportedException);
}


/* exceptions_throw_illegalaccessexception *************************************

   Generates and throws a java.lang.IllegalAccessException for the VM.

*******************************************************************************/

void exceptions_throw_illegalaccessexception(utf *message)
{
	exceptions_throw_utf_utf(utf_java_lang_IllegalAccessException, message);
}


/* exceptions_throw_illegalargumentexception ***********************************

   Generates and throws a java.lang.IllegalArgumentException for the
   VM.

*******************************************************************************/

void exceptions_throw_illegalargumentexception(void)
{
	exceptions_throw_utf(utf_java_lang_IllegalArgumentException);
}


/* exceptions_throw_illegalmonitorstateexception *******************************

   Generates and throws a java.lang.IllegalMonitorStateException for
   the VM.

*******************************************************************************/

void exceptions_throw_illegalmonitorstateexception(void)
{
	exceptions_throw_utf(utf_java_lang_IllegalMonitorStateException);
}


/* exceptions_throw_instantiationexception *************************************

   Generates and throws a java.lang.InstantiationException for the VM.

*******************************************************************************/

void exceptions_throw_instantiationexception(classinfo *c)
{
	exceptions_throw_utf_utf(utf_java_lang_InstantiationException, c->name);
}


/* exceptions_throw_interruptedexception ***************************************

   Generates and throws a java.lang.InterruptedException for the VM.

*******************************************************************************/

void exceptions_throw_interruptedexception(void)
{
	exceptions_throw_utf(utf_java_lang_InterruptedException);
}


/* exceptions_throw_invocationtargetexception **********************************

   Generates and throws a java.lang.reflect.InvocationTargetException
   for the VM.

   IN:
      cause......cause exception object

*******************************************************************************/

void exceptions_throw_invocationtargetexception(java_handle_t *cause)
{
	exceptions_throw_utf_throwable(utf_java_lang_reflect_InvocationTargetException,
								   cause);
}


/* exceptions_throw_negativearraysizeexception *********************************

   Generates and throws a java.lang.NegativeArraySizeException for the
   VM.

*******************************************************************************/

void exceptions_throw_negativearraysizeexception(void)
{
	exceptions_throw_utf(utf_java_lang_NegativeArraySizeException);
}


/* exceptions_new_nullpointerexception *****************************************

   Generates a java.lang.NullPointerException for the VM system.

*******************************************************************************/

java_handle_t *exceptions_new_nullpointerexception(void)
{
	java_handle_t *o;

	o = exceptions_new_utf(utf_java_lang_NullPointerException);

	return o;
}


/* exceptions_throw_nullpointerexception ***************************************

   Generates a java.lang.NullPointerException for the VM system and
   throw it in the VM system.

*******************************************************************************/

void exceptions_throw_nullpointerexception(void)
{
	exceptions_throw_utf(utf_java_lang_NullPointerException);
}


/* exceptions_throw_privilegedactionexception **********************************

   Generates and throws a java.security.PrivilegedActionException.

*******************************************************************************/

void exceptions_throw_privilegedactionexception(java_handle_t *exception)
{
	exceptions_throw_utf_exception(utf_java_security_PrivilegedActionException,
								   exception);
}


/* exceptions_throw_stringindexoutofboundsexception ****************************

   Generates and throws a java.lang.StringIndexOutOfBoundsException
   for the VM.

*******************************************************************************/

void exceptions_throw_stringindexoutofboundsexception(void)
{
	exceptions_throw_utf(utf_java_lang_StringIndexOutOfBoundsException);
}


/* exceptions_fillinstacktrace *************************************************

   Calls the fillInStackTrace-method of the currently thrown
   exception.

*******************************************************************************/

java_handle_t *exceptions_fillinstacktrace(void)
{
	java_handle_t *o;
	classinfo     *c;
	methodinfo    *m;

	/* get exception */

	o = exceptions_get_and_clear_exception();

	assert(o);

	/* resolve methodinfo pointer from exception object */

	LLNI_class_get(o, c);

#if defined(ENABLE_JAVASE)
	m = class_resolvemethod(c,
							utf_fillInStackTrace,
							utf_void__java_lang_Throwable);
#elif defined(ENABLE_JAVAME_CLDC1_1)
	m = class_resolvemethod(c,
							utf_fillInStackTrace,
							utf_void__void);
#else
#error IMPLEMENT ME!
#endif

	/* call function */

	(void) vm_call_method(m, o);

	/* return exception object */

	return o;
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

#if defined(ENABLE_JIT)
void *exceptions_handle_exception(java_object_t *xptro, void *xpc, void *pv, void *sp)
{
	stackframeinfo_t        sfi;
	java_handle_t          *xptr;
	methodinfo             *m;
	codeinfo               *code;
	exceptiontable_t       *et;
	exceptiontable_entry_t *ete;
	s4                      i;
	classref_or_classinfo   cr;
	classinfo              *c;
#if defined(ENABLE_THREADS)
	java_object_t          *o;
#endif
	void                   *result;

#ifdef __S390__
	/* Addresses are 31 bit integers */
#	define ADDR_MASK(x) (void *) ((uintptr_t) (x) & 0x7FFFFFFF)
#else
#	define ADDR_MASK(x) (x)
#endif

	xptr = LLNI_WRAP(xptro);
	xpc  = ADDR_MASK(xpc);

	/* Fill and add a stackframeinfo (XPC is equal to RA). */

	stacktrace_stackframeinfo_add(&sfi, pv, sp, xpc, xpc);

	result = NULL;

	/* Get the codeinfo for the current method. */

	code = code_get_codeinfo_for_pv(pv);

	/* Get the methodinfo pointer from the codeinfo pointer. For
	   asm_vm_call_method the codeinfo pointer is NULL and we simply
	   can return the proper exception handler. */

	if (code == NULL) {
		result = (void *) (uintptr_t) &asm_vm_call_method_exception_handler;
		goto exceptions_handle_exception_return;
	}

	m = code->m;

#if !defined(NDEBUG)
	/* print exception trace */

	if (opt_TraceExceptions)
		trace_exception(LLNI_DIRECT(xptr), m, xpc);

# if defined(ENABLE_VMLOG)
	vmlog_cacao_throw(xptr);
# endif
#endif

	/* Get the exception table. */

	et = code->exceptiontable;

	if (et != NULL) {
	/* Iterate over all exception table entries. */

	ete = et->entries;

	for (i = 0; i < et->length; i++, ete++) {
		/* is the xpc is the current catch range */

		if ((ADDR_MASK(ete->startpc) <= xpc) && (xpc < ADDR_MASK(ete->endpc))) {
			cr = ete->catchtype;

			/* NULL catches everything */

			if (cr.any == NULL) {
#if !defined(NDEBUG)
				/* Print stacktrace of exception when caught. */

# if defined(ENABLE_VMLOG)
				vmlog_cacao_catch(xptr);
# endif

				if (opt_TraceExceptions) {
					exceptions_print_exception(xptr);
					stacktrace_print_exception(xptr);
				}
#endif

				result = ete->handlerpc;
				goto exceptions_handle_exception_return;
			}

			/* resolve or load/link the exception class */

			if (IS_CLASSREF(cr)) {
				/* The exception class reference is unresolved. */
				/* We have to do _eager_ resolving here. While the
				   class of the exception object is guaranteed to be
				   loaded, it may well have been loaded by a different
				   loader than the defining loader of m's class, which
				   is the one we must use to resolve the catch
				   class. Thus lazy resolving might fail, even if the
				   result of the resolution would be an already loaded
				   class. */

				c = resolve_classref_eager(cr.ref);

				if (c == NULL) {
					/* Exception resolving the exception class, argh! */
					goto exceptions_handle_exception_return;
				}

				/* Ok, we resolved it. Enter it in the table, so we
				   don't have to do this again. */
				/* XXX this write should be atomic. Is it? */

				ete->catchtype.cls = c;
			}
			else {
				c = cr.cls;

				/* XXX I don't think this case can ever happen. -Edwin */
				if (!(c->state & CLASS_LOADED))
					/* use the methods' classloader */
					if (!load_class_from_classloader(c->name,
													 m->clazz->classloader))
						goto exceptions_handle_exception_return;

				/* XXX I think, if it is not linked, we can be sure
				   that the exception object is no (indirect) instance
				   of it, no?  -Edwin  */
				if (!(c->state & CLASS_LINKED))
					if (!link_class(c))
						goto exceptions_handle_exception_return;
			}

			/* is the thrown exception an instance of the catch class? */

			if (builtin_instanceof(xptr, c)) {
#if !defined(NDEBUG)
				/* Print stacktrace of exception when caught. */

# if defined(ENABLE_VMLOG)
				vmlog_cacao_catch(xptr);
# endif

				if (opt_TraceExceptions) {
					exceptions_print_exception(xptr);
					stacktrace_print_exception(xptr);
				}
#endif

				result = ete->handlerpc;
				goto exceptions_handle_exception_return;
			}
		}
	}
	}

#if defined(ENABLE_THREADS)
	/* Is this method realization synchronized? */

	if (code_is_synchronized(code)) {
		/* Get synchronization object. */

		o = *((java_object_t **) (((uintptr_t) sp) + code->synchronizedoffset));

		assert(o != NULL);

		lock_monitor_exit(LLNI_QUICKWRAP(o));
	}
#endif

	/* none of the exceptions catch this one */

#if !defined(NDEBUG)
# if defined(ENABLE_VMLOG)
	vmlog_cacao_unwnd_method(m);
# endif

# if defined(ENABLE_DEBUG_FILTER)
	if (show_filters_test_verbosecall_exit(m)) {
# endif

	/* outdent the log message */

	if (opt_verbosecall) {
		if (TRACEJAVACALLINDENT)
			TRACEJAVACALLINDENT--;
		else
			log_text("exceptions_handle_exception: WARNING: unmatched unindent");
	}

# if defined(ENABLE_DEBUG_FILTER)
	}
# endif
#endif /* !defined(NDEBUG) */

	result = NULL;

exceptions_handle_exception_return:

	/* Remove the stackframeinfo. */

	stacktrace_stackframeinfo_remove(&sfi);

	return result;
}
#endif /* defined(ENABLE_JIT) */


/* exceptions_print_exception **************************************************

   Prints an exception, the detail message and the cause, if
   available, with CACAO internal functions to stdout.

*******************************************************************************/

void exceptions_print_exception(java_handle_t *xptr)
{
	java_lang_Throwable   *t;
#if defined(ENABLE_JAVASE)
	java_lang_Throwable   *cause;
#endif
	java_lang_String      *s;
	classinfo             *c;
	utf                   *u;

	t = (java_lang_Throwable *) xptr;

	if (t == NULL) {
		puts("NULL\n");
		return;
	}

#if defined(ENABLE_JAVASE)
	LLNI_field_get_ref(t, cause, cause);
#endif

	/* print the root exception */

	LLNI_class_get(t, c);
	utf_display_printable_ascii_classname(c->name);

	LLNI_field_get_ref(t, detailMessage, s);

	if (s != NULL) {
		u = javastring_toutf((java_handle_t *) s, false);

		printf(": ");
		utf_display_printable_ascii(u);
	}

	putc('\n', stdout);

#if defined(ENABLE_JAVASE)
	/* print the cause if available */

	if ((cause != NULL) && (cause != t)) {
		printf("Caused by: ");
		
		LLNI_class_get(cause, c);
		utf_display_printable_ascii_classname(c->name);

		LLNI_field_get_ref(cause, detailMessage, s);

		if (s != NULL) {
			u = javastring_toutf((java_handle_t *) s, false);

			printf(": ");
			utf_display_printable_ascii(u);
		}

		putc('\n', stdout);
	}
#endif
}


/* exceptions_print_current_exception ******************************************

   Prints the current pending exception, the detail message and the
   cause, if available, with CACAO internal functions to stdout.

*******************************************************************************/

void exceptions_print_current_exception(void)
{
	java_handle_t *o;

	o = exceptions_get_exception();

	exceptions_print_exception(o);
}


/* exceptions_print_stacktrace *************************************************

   Prints a pending exception with Throwable.printStackTrace().  If
   there happens an exception during printStackTrace(), we print the
   thrown exception and the original one.

   NOTE: This function calls Java code.

*******************************************************************************/

void exceptions_print_stacktrace(void)
{
	java_handle_t    *e;
	java_handle_t    *ne;
	classinfo        *c;
	methodinfo       *m;

#if defined(ENABLE_THREADS)
	threadobject     *t;
	java_lang_Thread *to;
#endif

	/* Get and clear exception because we are calling Java code
	   again. */

	e = exceptions_get_and_clear_exception();

	if (e == NULL)
		return;

#if 0
	/* FIXME Enable me. */
	if (builtin_instanceof(e, class_java_lang_ThreadDeath)) {
		/* Don't print anything if we are being killed. */
	}
	else
#endif
	{
		/* Get the exception class. */

		LLNI_class_get(e, c);

		/* Find the printStackTrace() method. */

		m = class_resolveclassmethod(c,
									 utf_printStackTrace,
									 utf_void__void,
									 class_java_lang_Object,
									 false);

		if (m == NULL)
			vm_abort("exceptions_print_stacktrace: printStackTrace()V not found");

		/* Print message. */

		fprintf(stderr, "Exception ");

#if defined(ENABLE_THREADS)
		/* Print thread name.  We get the thread here explicitly as we
		   need it afterwards. */

		t  = thread_get_current();
		to = (java_lang_Thread *) thread_get_object(t);

		if (to != NULL) {
			fprintf(stderr, "in thread \"");
			thread_fprint_name(t, stderr);
			fprintf(stderr, "\" ");
		}
#endif

		/* Print the stacktrace. */

		if (builtin_instanceof(e, class_java_lang_Throwable)) {
			(void) vm_call_method(m, e);

			/* If this happens we are EXTREMLY out of memory or have a
			   serious problem while printStackTrace.  But may be
			   another exception, so print it. */

			ne = exceptions_get_exception();

			if (ne != NULL) {
				fprintf(stderr, "Exception while printStackTrace(): ");

				/* Print the current exception. */

				exceptions_print_exception(ne);
				stacktrace_print_exception(ne);

				/* Now print the original exception. */

				fprintf(stderr, "Original exception was: ");
				exceptions_print_exception(e);
				stacktrace_print_exception(e);
			}
		}
		else {
			fprintf(stderr, ". Uncaught exception of type ");
#if !defined(NDEBUG)
			/* FIXME This prints to stdout. */
			class_print(c);
#else
			fprintf(stderr, "UNKNOWN");
#endif
			fprintf(stderr, ".");
		}

		fflush(stderr);
	}
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
 * vim:noexpandtab:sw=4:ts=4:
 */
