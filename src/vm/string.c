/* src/vm/string.c - java.lang.String related functions

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
            Andreas Krall

   Changes: Christian Thalinger

   $Id: string.c 3984 2005-12-22 10:56:20Z twisti $

*/


#include <assert.h>

#include "config.h"
#include "vm/types.h"

#include "vm/global.h"

#include "mm/memory.h"
#include "native/include/java_lang_String.h"
#include "vm/exceptions.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/stringlocal.h"
#include "vm/utf8.h"


/* global variables ***********************************************************/

/* hashsize must be power of 2 */

#define HASHTABLE_STRING_SIZE    2048   /* initial size of javastring-hash    */

hashtable hashtable_string;             /* hashtable for javastrings          */

#if defined(USE_THREADS)
static java_objectheader *lock_hashtable_string;
#endif


/* global string definitions **************************************************/

/* exception/error super class */

const char *string_java_lang_Throwable =
    "java/lang/Throwable";

const char *string_java_lang_VMThrowable =
    "java/lang/VMThrowable";


/* specify some exception strings for code generation */

const char *string_java_lang_ArithmeticException =
    "java/lang/ArithmeticException";

const char *string_java_lang_ArithmeticException_message =
    "/ by zero";

const char *string_java_lang_ArrayIndexOutOfBoundsException =
    "java/lang/ArrayIndexOutOfBoundsException";

const char *string_java_lang_ArrayStoreException =
    "java/lang/ArrayStoreException";

const char *string_java_lang_ClassCastException =
    "java/lang/ClassCastException";

const char *string_java_lang_ClassNotFoundException =
	"java/lang/ClassNotFoundException";

const char *string_java_lang_CloneNotSupportedException =
    "java/lang/CloneNotSupportedException";

const char *string_java_lang_Exception =
    "java/lang/Exception";

const char *string_java_lang_IllegalAccessException =
    "java/lang/IllegalAccessException";

const char *string_java_lang_IllegalArgumentException =
    "java/lang/IllegalArgumentException";

const char *string_java_lang_IllegalMonitorStateException =
    "java/lang/IllegalMonitorStateException";

const char *string_java_lang_IndexOutOfBoundsException =
    "java/lang/IndexOutOfBoundsException";

const char *string_java_lang_InstantiationException =
    "java/lang/InstantiationException";

const char *string_java_lang_InterruptedException =
    "java/lang/InterruptedException";

const char *string_java_lang_NegativeArraySizeException =
    "java/lang/NegativeArraySizeException";

const char *string_java_lang_NoSuchFieldException =
	"java/lang/NoSuchFieldException";

const char *string_java_lang_NoSuchMethodException =
	"java/lang/NoSuchMethodException";

const char *string_java_lang_NullPointerException =
    "java/lang/NullPointerException";

const char *string_java_lang_reflect_InvocationTargetException =
    "java/lang/reflect/InvocationTargetException";


/* specify some error strings for code generation */

const char *string_java_lang_AbstractMethodError =
    "java/lang/AbstractMethodError";

const char *string_java_lang_ClassCircularityError =
    "java/lang/ClassCircularityError";

const char *string_java_lang_ClassFormatError =
    "java/lang/ClassFormatError";

const char *string_java_lang_Error =
    "java/lang/Error";

const char *string_java_lang_ExceptionInInitializerError =
    "java/lang/ExceptionInInitializerError";

const char *string_java_lang_IncompatibleClassChangeError =
    "java/lang/IncompatibleClassChangeError";

const char *string_java_lang_InstantiationError =
    "java/lang/InstantiationError";

const char *string_java_lang_InternalError =
    "java/lang/InternalError";

const char *string_java_lang_LinkageError =
    "java/lang/LinkageError";

const char *string_java_lang_NoClassDefFoundError =
    "java/lang/NoClassDefFoundError";

const char *string_java_lang_NoSuchFieldError =
	"java/lang/NoSuchFieldError";

const char *string_java_lang_NoSuchMethodError =
	"java/lang/NoSuchMethodError";

const char *string_java_lang_OutOfMemoryError =
    "java/lang/OutOfMemoryError";

const char *string_java_lang_UnsatisfiedLinkError =
    "java/lang/UnsatisfiedLinkError";

const char *string_java_lang_UnsupportedClassVersionError =
    "java/lang/UnsupportedClassVersionError";

const char *string_java_lang_VerifyError =
    "java/lang/VerifyError";

const char *string_java_lang_VirtualMachineError =
    "java/lang/VirtualMachineError";


/* string_init *****************************************************************

   Initialize the string hashtable lock.

*******************************************************************************/

bool string_init(void)
{
	/* create string (javastring) hashtable */

	hashtable_create(&hashtable_string, HASHTABLE_STRING_SIZE);

#if defined(USE_THREADS)
	/* create string hashtable lock object */

	lock_hashtable_string = NEW(java_objectheader);

# if defined(NATIVE_THREADS)
	initObjectLock(lock_hashtable_string);
# endif
#endif

	/* everything's ok */

	return true;
}


/* stringtable_update **********************************************************

   Traverses the javastring hashtable and sets the vftbl-entries of
   javastrings which were temporarily set to NULL, because
   java.lang.Object was not yet loaded.

*******************************************************************************/
 
void stringtable_update(void)
{
	java_lang_String *js;   
	java_chararray *a;
	literalstring *s;       /* hashtable entry */
	int i;

	for (i = 0; i < hashtable_string.size; i++) {
		s = hashtable_string.ptr[i];
		if (s) {
			while (s) {
                                                               
				js = (java_lang_String *) s->string;
                               
				if (!js || !js->value) {
					/* error in hashtable found */
					log_text("invalid literalstring in hashtable");
					assert(0);
				}

				a = js->value;

				if (!js->header.vftbl) 
					/* vftbl of javastring is NULL */ 
					js->header.vftbl = class_java_lang_String->vftbl;

				if (!a->header.objheader.vftbl) 
					/* vftbl of character-array is NULL */ 
					a->header.objheader.vftbl = primitivetype_table[ARRAYTYPE_CHAR].arrayvftbl;

				/* follow link in external hash chain */
				s = s->hashlink;
			}       
		}               
	}
}


/* javastring_new **************************************************************

   creates a new object of type java/lang/String with the text of 
   the specified utf8-string

   return: pointer to the string or NULL if memory is exhausted.	

*******************************************************************************/

java_lang_String *javastring_new(utf *u)
{
	char *utf_ptr;                  /* current utf character in utf string    */
	u4 utflength;                   /* length of utf-string if uncompressed   */
	java_lang_String *s;            /* result-string                          */
	java_chararray *a;
	s4 i;

	if (!u) {
		*exceptionptr = new_nullpointerexception();
		return NULL;
	}

	utf_ptr = u->text;
	utflength = utf_strlen(u);

	s = (java_lang_String *) builtin_new(class_java_lang_String);
	a = builtin_newarray_char(utflength);

	/* javastring or character-array could not be created */
	if (!a || !s)
		return NULL;

	/* decompress utf-string */
	for (i = 0; i < utflength; i++)
		a->data[i] = utf_nextu2(&utf_ptr);
	
	/* set fields of the javastring-object */
	s->value  = a;
	s->offset = 0;
	s->count  = utflength;

	return s;
}

/* javastring_new_slash_to_dot *************************************************

   creates a new object of type java/lang/String with the text of 
   the specified utf8-string with slashes changed to dots

   return: pointer to the string or NULL if memory is exhausted.	

*******************************************************************************/

java_lang_String *javastring_new_slash_to_dot(utf *u)
{
	char *utf_ptr;                  /* current utf character in utf string    */
	u4 utflength;                   /* length of utf-string if uncompressed   */
	java_lang_String *s;            /* result-string                          */
	java_chararray *a;
	s4 i;
	u2 ch;

	if (!u) {
		*exceptionptr = new_nullpointerexception();
		return NULL;
	}

	utf_ptr = u->text;
	utflength = utf_strlen(u);

	s = (java_lang_String *) builtin_new(class_java_lang_String);
	a = builtin_newarray_char(utflength);

	/* javastring or character-array could not be created */
	if (!a || !s)
		return NULL;

	/* decompress utf-string */
	for (i = 0; i < utflength; i++) {
		ch = utf_nextu2(&utf_ptr);
		if (ch == '/')
			ch = '.';
		a->data[i] = ch;
	}
	
	/* set fields of the javastring-object */
	s->value  = a;
	s->offset = 0;
	s->count  = utflength;

	return s;
}


/* javastring_new_char *********************************************************

   creates a new java/lang/String object which contains the convertet
   C-string passed via text.

   return: the object pointer or NULL if memory is exhausted.

*******************************************************************************/

java_lang_String *javastring_new_char(const char *text)
{
	s4 i;
	s4 len;                             /* length of the string               */
	java_lang_String *s;                /* result-string                      */
	java_chararray *a;

	if (!text) {
		*exceptionptr = new_nullpointerexception();
		return NULL;
	}

	len = strlen(text);

	s = (java_lang_String *) builtin_new(class_java_lang_String);
	a = builtin_newarray_char(len);

	/* javastring or character-array could not be created */
	if (!a || !s)
		return NULL;

	/* copy text */
	for (i = 0; i < len; i++)
		a->data[i] = text[i];
	
	/* set fields of the javastring-object */
	s->value  = a;
	s->offset = 0;
	s->count  = len;

	return s;
}


/* javastring_tochar ***********************************************************

   converts a Java string into a C string.
	
   return: pointer to C string
	
   Caution: calling method MUST release the allocated memory!
	
*******************************************************************************/

char *javastring_tochar(java_objectheader *so) 
{
	java_lang_String *s = (java_lang_String *) so;
	java_chararray *a;
	char *buf;
	s4 i;
	
	if (!s)
		return "";

	a = s->value;

	if (!a)
		return "";

	buf = MNEW(char, s->count + 1);

	for (i = 0; i < s->count; i++)
		buf[i] = a->data[s->offset + i];

	buf[i] = '\0';

	return buf;
}


/* javastring_toutf ************************************************************

   Make utf symbol from javastring.

*******************************************************************************/

utf *javastring_toutf(java_lang_String *string, bool isclassname)
{
	java_lang_String *str = (java_lang_String *) string;

	return utf_new_u2(str->value->data + str->offset, str->count, isclassname);
}


/* javastring_strlen ***********************************************************

   Returns the length of the Java string.
	
*******************************************************************************/

s4 javastring_strlen(java_lang_String *s)
{
	if (!s)
		return 0;

	return s->count;
}


/* literalstring_u2 ************************************************************

   Searches for the javastring with the specified u2-array in the
   string hashtable, if there is no such string a new one is created.

   If copymode is true a copy of the u2-array is made.

*******************************************************************************/

java_objectheader *literalstring_u2(java_chararray *a, u4 length, u4 offset,
									bool copymode)
{
    literalstring    *s;                /* hashtable element                  */
    java_lang_String *js;               /* u2-array wrapped in javastring     */
    java_chararray   *stringdata;       /* copy of u2-array                   */
    u4                key;
    u4                slot;
    u2                i;

#if defined(USE_THREADS)
	builtin_monitorenter(lock_hashtable_string);
#endif

    /* find location in hashtable */

    key  = unicode_hashkey(a->data + offset, length);
    slot = key & (hashtable_string.size - 1);
    s    = hashtable_string.ptr[slot];

    while (s) {
		js = (java_lang_String *) s->string;

		if (length == js->count) {
			/* compare text */

			for (i = 0; i < length; i++)
				if (a->data[offset + i] != js->value->data[i])
					goto nomatch;

			/* string already in hashtable, free memory */

			if (!copymode)
				mem_free(a, sizeof(java_chararray) + sizeof(u2) * (length - 1) + 10);

#if defined(USE_THREADS)
			builtin_monitorexit(lock_hashtable_string);
#endif

			return (java_objectheader *) js;
		}

	nomatch:
		/* follow link in external hash chain */
		s = s->hashlink;
    }

    if (copymode) {
		/* create copy of u2-array for new javastring */
		u4 arraysize = sizeof(java_chararray) + sizeof(u2) * (length - 1) + 10;
		stringdata = mem_alloc(arraysize);
/*    		memcpy(stringdata, a, arraysize); */
  		memcpy(&(stringdata->header), &(a->header), sizeof(java_arrayheader));
  		memcpy(&(stringdata->data), &(a->data) + offset, sizeof(u2) * (length - 1) + 10);

    } else {
		stringdata = a;
	}

    /* location in hashtable found, complete arrayheader */

    stringdata->header.objheader.vftbl =
		primitivetype_table[ARRAYTYPE_CHAR].arrayvftbl;
    stringdata->header.size = length;

	/* XXX TWISTI: is this necessary? */
	if (!class_java_lang_String)
		class_java_lang_String = load_class_bootstrap(utf_java_lang_String);

	assert(class_java_lang_String);
	assert(class_java_lang_String->state & CLASS_LOADED);

	/* if we use eager loading, we have to check loaded String class */

	if (opt_eager)
		list_addfirst(&unlinkedclasses, class_java_lang_String);

	/* create new javastring */

	js = NEW(java_lang_String);

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	initObjectLock(&js->header);
#endif

	js->header.vftbl = class_java_lang_String->vftbl;
	js->value  = stringdata;
	js->offset = 0;
	js->count  = length;

	/* create new literalstring */

	s = NEW(literalstring);
	s->hashlink = hashtable_string.ptr[slot];
	s->string   = (java_objectheader *) js;
	hashtable_string.ptr[slot] = s;

	/* update number of hashtable entries */

	hashtable_string.entries++;

	/* reorganization of hashtable */       

	if (hashtable_string.entries > (hashtable_string.size * 2)) {
		/* reorganization of hashtable, average length of the external
		   chains is approx. 2 */

		u4                i;
		literalstring    *s;
		literalstring    *nexts;
		java_lang_String *tmpjs;
		hashtable         newhash;                       /* the new hashtable */
      
		/* create new hashtable, double the size */

		hashtable_create(&newhash, hashtable_string.size * 2);
		newhash.entries = hashtable_string.entries;
      
		/* transfer elements to new hashtable */

		for (i = 0; i < hashtable_string.size; i++) {
			s = hashtable_string.ptr[i];

			while (s) {
				nexts = s->hashlink;
				tmpjs = (java_lang_String *) s->string;
				slot  = unicode_hashkey(tmpjs->value->data, tmpjs->count) & (newhash.size - 1);
	  
				s->hashlink = newhash.ptr[slot];
				newhash.ptr[slot] = s;
	
				/* follow link in external hash chain */
				s = nexts;
			}
		}
	
		/* dispose old table */

		MFREE(hashtable_string.ptr, void*, hashtable_string.size);
		hashtable_string = newhash;
	}

#if defined(USE_THREADS)
	builtin_monitorexit(lock_hashtable_string);
#endif

	return (java_objectheader *) js;
}


/* literalstring_new ***********************************************************

   Creates a new javastring with the text of the utf-symbol and inserts it into
   the string hashtable.

*******************************************************************************/

java_objectheader *literalstring_new(utf *u)
{
    char           *utf_ptr;         /* pointer to current unicode character  */
	                                 /* utf string                            */
    u4              utflength;       /* length of utf-string if uncompressed  */
    java_chararray *a;               /* u2-array constructed from utf string  */
    u4              i;

	utf_ptr = u->text;
	utflength = utf_strlen(u);

    /* allocate memory */ 
    a = mem_alloc(sizeof(java_chararray) + sizeof(u2) * (utflength - 1) + 10);

    /* convert utf-string to u2-array */
    for (i = 0; i < utflength; i++)
		a->data[i] = utf_nextu2(&utf_ptr);

    return literalstring_u2(a, utflength, 0, false);
}


/* literalstring_free **********************************************************

   Removes a javastring from memory.

*******************************************************************************/

void literalstring_free(java_objectheader* sobj)
{
	java_lang_String *s;
	java_chararray *a;

	s = (java_lang_String *) sobj;
	a = s->value;

	/* dispose memory of java.lang.String object */
	FREE(s, java_lang_String);

	/* dispose memory of java-characterarray */
	FREE(a, sizeof(java_chararray) + sizeof(u2) * (a->header.size - 1)); /* +10 ?? */
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
