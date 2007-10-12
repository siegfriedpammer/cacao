/* src/vm/string.c - java.lang.String related functions

   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
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

*/


#include "config.h"

#include <assert.h>

#include "vm/types.h"

#include "vm/global.h"

#include "mm/memory.h"

#include "native/jni.h"
#include "native/llni.h"

#include "native/include/java_lang_String.h"

#include "threads/lock-common.h"

#include "vm/array.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/primitive.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"

#include "vmcore/options.h"
#include "vmcore/statistics.h"
#include "vmcore/utf8.h"


/* global variables ***********************************************************/

/* hashsize must be power of 2 */

#define HASHTABLE_STRING_SIZE    2048   /* initial size of javastring-hash    */

hashtable hashtable_string;             /* hashtable for javastrings          */

#if defined(ENABLE_THREADS)
static java_object_t *lock_hashtable_string;
#endif


/* XXX preliminary typedef, will be removed once string.c and utf8.c are
   unified. */

#if defined(ENABLE_HANDLES)
typedef heap_java_lang_String heapstring_t;
#else
typedef java_lang_String heapstring_t;
#endif


/* string_init *****************************************************************

   Initialize the string hashtable lock.

*******************************************************************************/

bool string_init(void)
{
	/* create string (javastring) hashtable */

	hashtable_create(&hashtable_string, HASHTABLE_STRING_SIZE);

#if defined(ENABLE_THREADS)
	/* create string hashtable lock object */

	lock_hashtable_string = NEW(java_object_t);

	LOCK_INIT_OBJECT_LOCK(lock_hashtable_string);
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
	heapstring_t     *js;
	java_chararray_t *a;
	literalstring    *s;       /* hashtable entry */
	int i;

	for (i = 0; i < hashtable_string.size; i++) {
		s = hashtable_string.ptr[i];
		if (s) {
			while (s) {
				js = (heapstring_t *) s->string;
                               
				if ((js == NULL) || (js->value == NULL)) {
					/* error in hashtable found */

					vm_abort("stringtable_update: invalid literalstring in hashtable");
				}

				a = js->value;

				if (!js->header.vftbl) 
					/* vftbl of javastring is NULL */ 
					js->header.vftbl = class_java_lang_String->vftbl;

				if (!a->header.objheader.vftbl) 
					/* vftbl of character-array is NULL */ 
					a->header.objheader.vftbl =
						primitive_arrayclass_get_by_type(ARRAYTYPE_CHAR)->vftbl;

				/* follow link in external hash chain */
				s = s->hashlink;
			}       
		}               
	}
}


/* javastring_new_from_utf_buffer **********************************************

   Create a new object of type java/lang/String with the text from
   the specified utf8 buffer.

   IN:
      buffer.......points to first char in the buffer
	  blength......number of bytes to read from the buffer

   RETURN VALUE:
      the java.lang.String object, or
      NULL if an exception has been thrown

*******************************************************************************/

static java_handle_t *javastring_new_from_utf_buffer(const char *buffer,
														 u4 blength)
{
	const char *utf_ptr;            /* current utf character in utf string    */
	u4 utflength;                   /* length of utf-string if uncompressed   */
	java_handle_t     *o;
	java_lang_String  *s;           /* result-string                          */
	java_handle_chararray_t *a;
	u4 i;

	assert(buffer);

	utflength = utf_get_number_of_u2s_for_buffer(buffer,blength);

	o = builtin_new(class_java_lang_String);
	a = builtin_newarray_char(utflength);

	/* javastring or character-array could not be created */

	if ((o == NULL) || (a == NULL))
		return NULL;

	/* decompress utf-string */

	utf_ptr = buffer;

	for (i = 0; i < utflength; i++)
		LLNI_array_direct(a, i) = utf_nextu2((char **) &utf_ptr);
	
	/* set fields of the javastring-object */

	s = (java_lang_String *) o;

	LLNI_field_set_ref(s, value , a);
	LLNI_field_set_val(s, offset, 0);
	LLNI_field_set_val(s, count , utflength);

	return o;
}


/* javastring_safe_new_from_utf8 ***********************************************

   Create a new object of type java/lang/String with the text from
   the specified UTF-8 string. This function is safe for invalid UTF-8.
   (Invalid characters will be replaced by U+fffd.)

   IN:
      text.........the UTF-8 string, zero-terminated.

   RETURN VALUE:
      the java.lang.String object, or
      NULL if an exception has been thrown

*******************************************************************************/

java_handle_t *javastring_safe_new_from_utf8(const char *text)
{
	java_handle_t           *o;
	java_handle_chararray_t *a;
	java_lang_String        *s;
	s4 nbytes;
	s4 len;

	assert(text);

	/* Get number of bytes. We need this to completely emulate the messy */
	/* behaviour of the RI. :(                                           */

	nbytes = strlen(text);

	/* calculate number of Java characters */

	len = utf8_safe_number_of_u2s(text, nbytes);

	/* allocate the String object and the char array */

	o = builtin_new(class_java_lang_String);
	a = builtin_newarray_char(len);

	/* javastring or character-array could not be created? */

	if ((o == NULL) || (a == NULL))
		return NULL;

	/* decompress UTF-8 string */

	utf8_safe_convert_to_u2s(text, nbytes, LLNI_array_data(a));

	/* set fields of the String object */

	s = (java_lang_String *) o;

	LLNI_field_set_ref(s, value , a);
	LLNI_field_set_val(s, offset, 0);
	LLNI_field_set_val(s, count , len);

	return o;
}


/* javastring_new_from_utf_string **********************************************

   Create a new object of type java/lang/String with the text from
   the specified zero-terminated utf8 string.

   IN:
      buffer.......points to first char in the buffer
	  blength......number of bytes to read from the buffer

   RETURN VALUE:
      the java.lang.String object, or
      NULL if an exception has been thrown

*******************************************************************************/

java_handle_t *javastring_new_from_utf_string(const char *utfstr)
{
	assert(utfstr);

	return javastring_new_from_utf_buffer(utfstr, strlen(utfstr));
}


/* javastring_new **************************************************************

   creates a new object of type java/lang/String with the text of 
   the specified utf8-string

   return: pointer to the string or NULL if memory is exhausted.	

*******************************************************************************/

java_handle_t *javastring_new(utf *u)
{
	char *utf_ptr;                  /* current utf character in utf string    */
	u4 utflength;                   /* length of utf-string if uncompressed   */
	java_handle_t           *o;
	java_handle_chararray_t *a;
	java_lang_String        *s;
	s4 i;

	if (u == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	utf_ptr = u->text;
	utflength = utf_get_number_of_u2s(u);

	o = builtin_new(class_java_lang_String);
	a = builtin_newarray_char(utflength);

	/* javastring or character-array could not be created */

	if ((o == NULL) || (a == NULL))
		return NULL;

	/* decompress utf-string */

	for (i = 0; i < utflength; i++)
		LLNI_array_direct(a, i) = utf_nextu2(&utf_ptr);
	
	/* set fields of the javastring-object */

	s = (java_lang_String *) o;

	LLNI_field_set_ref(s, value , a);
	LLNI_field_set_val(s, offset, 0);
	LLNI_field_set_val(s, count , utflength);

	return o;
}


/* javastring_new_slash_to_dot *************************************************

   creates a new object of type java/lang/String with the text of 
   the specified utf8-string with slashes changed to dots

   return: pointer to the string or NULL if memory is exhausted.	

*******************************************************************************/

java_handle_t *javastring_new_slash_to_dot(utf *u)
{
	char *utf_ptr;                  /* current utf character in utf string    */
	u4 utflength;                   /* length of utf-string if uncompressed   */
	java_handle_t           *o;
	java_handle_chararray_t *a;
	java_lang_String        *s;
	s4 i;
	u2 ch;

	if (u == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	utf_ptr = u->text;
	utflength = utf_get_number_of_u2s(u);

	o = builtin_new(class_java_lang_String);
	a = builtin_newarray_char(utflength);

	/* javastring or character-array could not be created */
	if ((o == NULL) || (a == NULL))
		return NULL;

	/* decompress utf-string */

	for (i = 0; i < utflength; i++) {
		ch = utf_nextu2(&utf_ptr);
		if (ch == '/')
			ch = '.';
		LLNI_array_direct(a, i) = ch;
	}
	
	/* set fields of the javastring-object */

	s = (java_lang_String *) o;

	LLNI_field_set_ref(s, value , a);
	LLNI_field_set_val(s, offset, 0);
	LLNI_field_set_val(s, count , utflength);

	return o;
}


/* javastring_new_from_ascii ***************************************************

   creates a new java/lang/String object which contains the given ASCII
   C-string converted to UTF-16.

   IN:
      text.........string of ASCII characters

   RETURN VALUE:
      the java.lang.String object, or 
      NULL if an exception has been thrown.

*******************************************************************************/

java_handle_t *javastring_new_from_ascii(const char *text)
{
	s4 i;
	s4 len;                             /* length of the string               */
	java_handle_t           *o;
	java_lang_String        *s;
	java_handle_chararray_t *a;

	if (text == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	len = strlen(text);

	o = builtin_new(class_java_lang_String);
	a = builtin_newarray_char(len);

	/* javastring or character-array could not be created */

	if ((o == NULL) || (a == NULL))
		return NULL;

	/* copy text */

	for (i = 0; i < len; i++)
		LLNI_array_direct(a, i) = text[i];
	
	/* set fields of the javastring-object */

	s = (java_lang_String *) o;

	LLNI_field_set_ref(s, value , a);
	LLNI_field_set_val(s, offset, 0);
	LLNI_field_set_val(s, count , len);

	return o;
}


/* javastring_tochar ***********************************************************

   converts a Java string into a C string.
	
   return: pointer to C string
	
   Caution: calling method MUST release the allocated memory!
	
*******************************************************************************/

char *javastring_tochar(java_handle_t *so) 
{
	java_lang_String        *s = (java_lang_String *) so;
	java_handle_chararray_t *a;
	int32_t                  count;
	int32_t                  offset;
	char *buf;
	s4 i;
	
	if (!s)
		return "";

	LLNI_field_get_ref(s, value, a);

	if (!a)
		return "";

	LLNI_field_get_val(s, count, count);
	LLNI_field_get_val(s, offset, offset);

	buf = MNEW(char, count + 1);

	for (i = 0; i < count; i++)
		buf[i] = LLNI_array_direct(a, offset + i);

	buf[i] = '\0';

	return buf;
}


/* javastring_toutf ************************************************************

   Make utf symbol from javastring.

*******************************************************************************/

utf *javastring_toutf(java_handle_t *string, bool isclassname)
{
	java_lang_String        *s;
	java_handle_chararray_t *value;
	int32_t                  count;
	int32_t                  offset;

	s = (java_lang_String *) string;

	if (s == NULL)
		return utf_null;

	LLNI_field_get_ref(s, value, value);
	LLNI_field_get_val(s, count, count);
	LLNI_field_get_val(s, offset, offset);

	return utf_new_u2(LLNI_array_data(value) + offset, count, isclassname);
}


/* literalstring_u2 ************************************************************

   Searches for the literalstring with the specified u2-array in the
   string hashtable, if there is no such string a new one is created.

   If copymode is true a copy of the u2-array is made.

*******************************************************************************/

static java_object_t *literalstring_u2(java_chararray_t *a, u4 length,
									   u4 offset, bool copymode)
{
    literalstring    *s;                /* hashtable element                  */
    heapstring_t     *js;               /* u2-array wrapped in javastring     */
    java_chararray_t *ca;               /* copy of u2-array                   */
    u4                key;
    u4                slot;
    u2                i;

	LOCK_MONITOR_ENTER(lock_hashtable_string);

    /* find location in hashtable */

    key  = unicode_hashkey(a->data + offset, length);
    slot = key & (hashtable_string.size - 1);
    s    = hashtable_string.ptr[slot];

    while (s) {
		js = (heapstring_t *) s->string;

		if (length == js->count) {
			/* compare text */

			for (i = 0; i < length; i++)
				if (a->data[offset + i] != js->value->data[i])
					goto nomatch;

			/* string already in hashtable, free memory */

			if (!copymode)
				mem_free(a, sizeof(java_chararray_t) + sizeof(u2) * (length - 1) + 10);

			LOCK_MONITOR_EXIT(lock_hashtable_string);

			return (java_object_t *) js;
		}

	nomatch:
		/* follow link in external hash chain */
		s = s->hashlink;
    }

    if (copymode) {
		/* create copy of u2-array for new javastring */
		u4 arraysize = sizeof(java_chararray_t) + sizeof(u2) * (length - 1) + 10;
		ca = mem_alloc(arraysize);
/*    		memcpy(ca, a, arraysize); */
  		memcpy(&(ca->header), &(a->header), sizeof(java_array_t));
  		memcpy(&(ca->data), &(a->data) + offset, sizeof(u2) * (length - 1) + 10);

    } else {
		ca = a;
	}

    /* location in hashtable found, complete arrayheader */

    ca->header.objheader.vftbl =
		primitive_arrayclass_get_by_type(ARRAYTYPE_CHAR)->vftbl;
    ca->header.size            = length;

	assert(class_java_lang_String);
	assert(class_java_lang_String->state & CLASS_LOADED);

	/* create new javastring */

	js = NEW(heapstring_t);

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		size_string += sizeof(heapstring_t);
#endif

#if defined(ENABLE_THREADS)
	lock_init_object_lock(&js->header);
#endif

	js->header.vftbl = class_java_lang_String->vftbl;
	js->value  = ca;
	js->offset = 0;
	js->count  = length;

	/* create new literalstring */

	s = NEW(literalstring);

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		size_string += sizeof(literalstring);
#endif

	s->hashlink = hashtable_string.ptr[slot];
	s->string   = (java_object_t *) js;
	hashtable_string.ptr[slot] = s;

	/* update number of hashtable entries */

	hashtable_string.entries++;

	/* reorganization of hashtable */       

	if (hashtable_string.entries > (hashtable_string.size * 2)) {
		/* reorganization of hashtable, average length of the external
		   chains is approx. 2 */

		u4             i;
		literalstring *s;
		literalstring *nexts;
		heapstring_t  *tmpjs;
		hashtable      newhash;                          /* the new hashtable */
      
		/* create new hashtable, double the size */

		hashtable_create(&newhash, hashtable_string.size * 2);
		newhash.entries = hashtable_string.entries;
      
		/* transfer elements to new hashtable */

		for (i = 0; i < hashtable_string.size; i++) {
			s = hashtable_string.ptr[i];

			while (s) {
				nexts = s->hashlink;
				tmpjs = (heapstring_t *) s->string;
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

	LOCK_MONITOR_EXIT(lock_hashtable_string);

	return (java_object_t *) js;
}


/* literalstring_new ***********************************************************

   Creates a new literalstring with the text of the utf-symbol and inserts
   it into the string hashtable.

*******************************************************************************/

java_object_t *literalstring_new(utf *u)
{
    char             *utf_ptr;       /* pointer to current unicode character  */
	                                 /* utf string                            */
    u4                utflength;     /* length of utf-string if uncompressed  */
    java_chararray_t *a;             /* u2-array constructed from utf string  */
    u4                i;

	utf_ptr = u->text;
	utflength = utf_get_number_of_u2s(u);

    /* allocate memory */ 
    a = mem_alloc(sizeof(java_chararray_t) + sizeof(u2) * (utflength - 1) + 10);

    /* convert utf-string to u2-array */
    for (i = 0; i < utflength; i++)
		a->data[i] = utf_nextu2(&utf_ptr);

    return literalstring_u2(a, utflength, 0, false);
}


/* literalstring_free **********************************************************

   Removes a literalstring from memory.

*******************************************************************************/

static void literalstring_free(java_object_t* string)
{
	heapstring_t     *s;
	java_chararray_t *a;

	s = (heapstring_t *) string;
	a = s->value;

	/* dispose memory of java.lang.String object */
	FREE(s, heapstring_t);

	/* dispose memory of java-characterarray */
	FREE(a, sizeof(java_chararray_t) + sizeof(u2) * (a->header.size - 1)); /* +10 ?? */
}


/* javastring_intern ***********************************************************

   Intern the given Java string.

   XXX NOTE: Literal Strings are direct references since they are not placed
   onto the GC-Heap. That's why this function looks so "different".

*******************************************************************************/

java_handle_t *javastring_intern(java_handle_t *s)
{
	java_lang_String *so;
	java_chararray_t *value;
	int32_t           count;
	int32_t           offset;
/* 	java_lang_String *o; */
	java_object_t    *o; /* XXX see note above */

	so = (java_lang_String *) s;

	value  = LLNI_field_direct(so, value); /* XXX see note above */
	LLNI_field_get_val(so, count, count);
	LLNI_field_get_val(so, offset, offset);

	o = literalstring_u2(value, count, offset, true);

	return LLNI_WRAP(o); /* XXX see note above */
}


/* javastring_print ************************************************************

   Print the given Java string.

*******************************************************************************/

void javastring_print(java_handle_t *s)
{
	java_lang_String        *so;
	java_handle_chararray_t *value;
	int32_t                  count;
	int32_t                  offset;
	uint16_t                 c;
	int                      i;

	so = (java_lang_String *) s;

	LLNI_field_get_ref(so, value, value);
	LLNI_field_get_val(so, count, count);
	LLNI_field_get_val(so, offset, offset);

	for (i = offset; i < offset + count; i++) {
		c = LLNI_array_direct(value, i);
		putchar(c);
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
