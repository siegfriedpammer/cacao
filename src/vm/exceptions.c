/* exceptions.c - exception related functions

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

   Authors: Christian Thalinger

   $Id: exceptions.c 1332 2004-07-21 15:48:08Z twisti $

*/


#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "asmpart.h"
#include "global.h"
#include "loader.h"
#include "native.h"
#include "tables.h"
#include "jit/jit.h"
#include "toolbox/logging.h"
#include "toolbox/memory.h"
#include "nat/java_lang_String.h"
#include "nat/java_lang_Throwable.h"


/* system exception classes required in cacao */

classinfo *class_java_lang_Throwable;
classinfo *class_java_lang_Exception;
classinfo *class_java_lang_Error;
classinfo *class_java_lang_OutOfMemoryError;


/* exception/error super class */

char *string_java_lang_Throwable =
    "java/lang/Throwable";

char *string_java_lang_VMThrowable =
    "java/lang/VMThrowable";


/* specify some exception strings for code generation */

char *string_java_lang_ArithmeticException =
    "java/lang/ArithmeticException";

char *string_java_lang_ArithmeticException_message =
    "/ by zero";

char *string_java_lang_ArrayIndexOutOfBoundsException =
    "java/lang/ArrayIndexOutOfBoundsException";

char *string_java_lang_ArrayStoreException =
    "java/lang/ArrayStoreException";

char *string_java_lang_ClassCastException =
    "java/lang/ClassCastException";

char *string_java_lang_ClassNotFoundException =
	"java/lang/ClassNotFoundException";

char *string_java_lang_CloneNotSupportedException =
    "java/lang/CloneNotSupportedException";

char *string_java_lang_Exception =
    "java/lang/Exception";

char *string_java_lang_IllegalArgumentException =
    "java/lang/IllegalArgumentException";

char *string_java_lang_IllegalMonitorStateException =
    "java/lang/IllegalMonitorStateException";

char *string_java_lang_NegativeArraySizeException =
    "java/lang/NegativeArraySizeException";

char *string_java_lang_NoSuchFieldException =
	"java/lang/NoSuchFieldException";

char *string_java_lang_NoSuchMethodException =
	"java/lang/NoSuchMethodException";

char *string_java_lang_NullPointerException =
    "java/lang/NullPointerException";


/* specify some error strings for code generation */

char *string_java_lang_AbstractMethodError =
    "java/lang/AbstractMethodError";

char *string_java_lang_ClassCircularityError =
    "java/lang/ClassCircularityError";

char *string_java_lang_ClassFormatError =
    "java/lang/ClassFormatError";

char *string_java_lang_Error =
    "java/lang/Error";

char *string_java_lang_ExceptionInInitializerError =
    "java/lang/ExceptionInInitializerError";

char *string_java_lang_IncompatibleClassChangeError =
    "java/lang/IncompatibleClassChangeError";

char *string_java_lang_InternalError =
    "java/lang/InternalError";

char *string_java_lang_LinkageError =
    "java/lang/LinkageError";

char *string_java_lang_NoClassDefFoundError =
    "java/lang/NoClassDefFoundError";

char *string_java_lang_NoSuchFieldError =
	"java/lang/NoSuchFieldError";

char *string_java_lang_NoSuchMethodError =
	"java/lang/NoSuchMethodError";

char *string_java_lang_OutOfMemoryError =
    "java/lang/OutOfMemoryError";

char *string_java_lang_VerifyError =
    "java/lang/VerifyError";

char *string_java_lang_VirtualMachineError =
    "java/lang/VirtualMachineError";


/* init_system_exceptions *****************************************************

   load, link and compile exceptions used in the system

*******************************************************************************/

void init_system_exceptions()
{
	/* java/lang/Throwable */

	class_java_lang_Throwable =
		class_new(utf_new_char(string_java_lang_Throwable));
	class_load(class_java_lang_Throwable);
	class_link(class_java_lang_Throwable);

	/* java/lang/Exception */

	class_java_lang_Exception =
		class_new(utf_new_char(string_java_lang_Exception));
	class_load(class_java_lang_Exception);
	class_link(class_java_lang_Exception);

	/* java/lang/Error */

	class_java_lang_Error =
		class_new(utf_new_char(string_java_lang_Error));
	class_load(class_java_lang_Error);
	class_link(class_java_lang_Error);

	/* java/lang/OutOfMemoryError */

	class_java_lang_OutOfMemoryError =
		class_new(utf_new_char(string_java_lang_OutOfMemoryError));
	class_load(class_java_lang_OutOfMemoryError);
	class_link(class_java_lang_OutOfMemoryError);
}


static void throw_exception_exit_intern(bool doexit)
{
	java_objectheader *xptr;
	classinfo *c;
	methodinfo *pss;

	if (*exceptionptr) {
		xptr = *exceptionptr;

		/* clear exception, because we are calling jit code again */
		*exceptionptr = NULL;

		c = xptr->vftbl->class;

		pss = class_resolveclassmethod(c,
									   utf_new_char("printStackTrace"),
									   utf_new_char("()V"),
									   class_java_lang_Object,
									   false);

		/* print the stacktrace */
		if (pss) {
			asm_calljavafunction(pss, xptr, NULL, NULL, NULL);

			/* this normally means, we are EXTREMLY out of memory, but may be
			   any other exception */
			if (*exceptionptr) {
				utf_fprint_classname(stderr, c->name);
				fprintf(stderr, "\n");
			}

		} else {
			utf_fprint_classname(stderr, c->name);
			fprintf(stderr, ": printStackTrace()V not found!\n");
		}

		fflush(stderr);

		/* good bye! */
		if (doexit) {
			exit(1);
		}
	}
}


void throw_exception()
{
	throw_exception_exit_intern(false);
}


void throw_exception_exit()
{
	throw_exception_exit_intern(true);
}


void throw_main_exception()
{
	fprintf(stderr, "Exception in thread \"main\" ");
	fflush(stderr);

	throw_exception_exit_intern(false);
}


void throw_main_exception_exit()
{
	fprintf(stderr, "Exception in thread \"main\" ");
	fflush(stderr);

	throw_exception_exit_intern(true);
}


void throw_cacao_exception_exit(char *exception, char *message)
{
	s4 i;
	char *tmp;
	s4 len;

	len = strlen(exception);
	tmp = MNEW(char, len);
	strncpy(tmp, exception, len);

	/* convert to classname */

   	for (i = len - 1; i >= 0; i--) {
 	 	if (tmp[i] == '/') tmp[i] = '.';
	}

	fprintf(stderr, "Exception in thread \"main\" %s", tmp);

	MFREE(tmp, char, len);

	if (strlen(message) > 0)
		fprintf(stderr, ": %s", message);

	fprintf(stderr, "\n");
	fflush(stderr);

	/* good bye! */
	exit(1);
}


#define CREATENEW_EXCEPTION(ex) \
	java_objectheader *newEx; \
	java_objectheader *oldexception=*exceptionptr;\
	*exceptionptr=0;\
	newEx=ex;\
	*exceptionptr=oldexception;\
	return newEx;

java_objectheader *new_exception(char *classname)
{
	classinfo *c = class_new(utf_new_char(classname));

	CREATENEW_EXCEPTION(native_new_and_init(c));
}

java_objectheader *new_exception_message(char *classname, char *message)
{
	classinfo *c = class_new(utf_new_char(classname));

	CREATENEW_EXCEPTION(native_new_and_init_string(c, javastring_new_char(message)));
}


java_objectheader *new_exception_throwable(char *classname, java_lang_Throwable *throwable)
{
	classinfo *c = class_new(utf_new_char(classname));

	CREATENEW_EXCEPTION(native_new_and_init_throwable(c, throwable));
}


java_objectheader *new_exception_utfmessage(char *classname, utf *message)
{
	classinfo *c = class_new(utf_new_char(classname));

	CREATENEW_EXCEPTION(native_new_and_init_string(c, javastring_new(message)));
}


java_objectheader *new_exception_javastring(char *classname, java_lang_String *message)
{
	classinfo *c = class_new(utf_new_char(classname));

	CREATENEW_EXCEPTION(native_new_and_init_string(c, message));
}


java_objectheader *new_exception_int(char *classname, s4 i)
{
	classinfo *c = class_new(utf_new_char(classname));

	CREATENEW_EXCEPTION(native_new_and_init_int(c, i));
}


/* new_classformaterror ********************************************************

   generates an java.lang.ClassFormatError for the classloader

*******************************************************************************/

java_objectheader *new_classformaterror(classinfo *c, char *message, ...)
{
	char msg[MAXLOGTEXT];
	va_list ap;

	utf_sprint_classname(msg, c->name);
	sprintf(msg + strlen(msg), " (");

	va_start(ap, message);
	vsprintf(msg + strlen(msg), message, ap);
	va_end(ap);

	sprintf(msg + strlen(msg), ")");

	return new_exception_message(string_java_lang_ClassFormatError, msg);
}


/* new_verifyerror *************************************************************

   generates an java.lang.VerifyError for the jit compiler

*******************************************************************************/

java_objectheader *new_verifyerror(methodinfo *m, char *message)
{
	java_objectheader *o;
	char *msg;
	s4 len;

	len = 8 + utf_strlen(m->class->name) +
		10 + utf_strlen(m->name) +
		13 + utf_strlen(m->descriptor) +
		2 + strlen(message) + 1;
		
	msg = MNEW(char, len);

	sprintf(msg, "(class: ");
	utf_sprint(msg + strlen(msg), m->class->name);
	sprintf(msg + strlen(msg), ", method: ");
	utf_sprint(msg + strlen(msg), m->name);
	sprintf(msg + strlen(msg), ", signature: ");
	utf_sprint(msg + strlen(msg), m->descriptor);
	sprintf(msg + strlen(msg), ") %s", message);

	o = new_exception_message(string_java_lang_VerifyError, msg);

	MFREE(msg, u1, len);

	return o;
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
