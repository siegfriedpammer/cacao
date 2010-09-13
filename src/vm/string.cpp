/* src/vm/string.cpp - java.lang.String related functions

   Copyright (C) 1996-2005, 2006, 2007, 2008, 2010
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

#include "vm/os.hpp"

#include "vm/types.h"

#include "vm/global.h"

#include "mm/memory.hpp"

#include "native/llni.h"

#include "threads/lock.hpp"

#include "vm/array.hpp"
#include "vm/jit/builtin.hpp"
#include "vm/exceptions.hpp"
#include "vm/globals.hpp"
#include "vm/javaobjects.hpp"
#include "vm/options.h"
#include "vm/primitive.hpp"
#include "vm/statistics.h"
#include "vm/string.hpp"
#include "vm/utf8.h"


/* global variables ***********************************************************/

/* hashsize must be power of 2 */

#define HASHTABLE_STRING_SIZE    2048   /* initial size of javastring-hash    */

static hashtable hashtable_string;      /* hashtable for javastrings          */
static Mutex* mutex;


/* string_init *****************************************************************

   Initialize the string hashtable lock.

*******************************************************************************/

bool string_init(void)
{
	TRACESUBSYSTEMINITIALIZATION("string_init");

	/* create string (javastring) hashtable */

	hashtable_create(&hashtable_string, HASHTABLE_STRING_SIZE);

	mutex = new Mutex();

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
	literalstring    *s;       /* hashtable entry */

	for (unsigned int i = 0; i < hashtable_string.size; i++) {
		s = (literalstring*) hashtable_string.ptr[i];

		if (s) {
			while (s) {
				// FIXME
				java_lang_String js(LLNI_WRAP(s->string));
                               
				if (js.is_null() || (js.get_value() == NULL)) {
					/* error in hashtable found */
					os::abort("stringtable_update: invalid literalstring in hashtable");
				}

				java_chararray_t* a = (java_chararray_t*) js.get_value();

				if (js.get_vftbl() == NULL)
					// FIXME
					LLNI_UNWRAP(js.get_handle())->vftbl = class_java_lang_String->vftbl;

				if (a->header.objheader.vftbl == NULL)
					a->header.objheader.vftbl = Primitive::get_arrayclass_by_type(ARRAYTYPE_CHAR)->vftbl;

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

static java_handle_t *javastring_new_from_utf_buffer(const char *buffer, u4 blength)
{
	const char *utf_ptr;            /* current utf character in utf string    */

	assert(buffer);

	int32_t utflength = utf_get_number_of_u2s_for_buffer(buffer, blength);

	java_handle_t*           h  = builtin_new(class_java_lang_String);
	CharArray ca(utflength);

	/* javastring or character-array could not be created */

	if ((h == NULL) || ca.is_null())
		return NULL;

	// XXX: Fix me!
	uint16_t* ptr = (uint16_t*) ca.get_raw_data_ptr();

	/* decompress utf-string */

	utf_ptr = buffer;

	for (int32_t i = 0; i < utflength; i++)
		ptr[i] = utf_nextu2((char **) &utf_ptr);
	
	/* set fields of the javastring-object */

	java_lang_String jls(h, ca.get_handle(), utflength);

	return jls.get_handle();
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
	if (text == NULL)
		return NULL;

	/* Get number of bytes. We need this to completely emulate the messy */
	/* behaviour of the RI. :(                                           */

	int32_t nbytes = strlen(text);

	/* calculate number of Java characters */

	int32_t len = utf8_safe_number_of_u2s(text, nbytes);

	/* allocate the String object and the char array */

	java_handle_t*           h  = builtin_new(class_java_lang_String);
	CharArray ca(len);

	/* javastring or character-array could not be created? */

	if ((h == NULL) || ca.is_null())
		return NULL;

	// XXX: Fix me!
	uint16_t* ptr = (uint16_t*) ca.get_raw_data_ptr();

	/* decompress UTF-8 string */

	utf8_safe_convert_to_u2s(text, nbytes, ptr);

	/* set fields of the String object */

	java_lang_String jls(h, ca.get_handle(), len);

	return jls.get_handle();
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
	if (u == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	char*   utf_ptr   = u->text;
	int32_t utflength = utf_get_number_of_u2s(u);

	java_handle_t*           h  = builtin_new(class_java_lang_String);
	CharArray ca(utflength);

	/* javastring or character-array could not be created */

	if ((h == NULL) || ca.is_null())
		return NULL;

	// XXX: Fix me!
	uint16_t* ptr = (uint16_t*) ca.get_raw_data_ptr();

	/* decompress utf-string */

	for (int32_t i = 0; i < utflength; i++)
		ptr[i] = utf_nextu2(&utf_ptr);
	
	/* set fields of the javastring-object */

	java_lang_String jls(h, ca.get_handle(), utflength);

	return jls.get_handle();
}


/* javastring_new_slash_to_dot *************************************************

   creates a new object of type java/lang/String with the text of 
   the specified utf8-string with slashes changed to dots

   return: pointer to the string or NULL if memory is exhausted.	

*******************************************************************************/

java_handle_t *javastring_new_slash_to_dot(utf *u)
{
	if (u == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	char*   utf_ptr   = u->text;
	int32_t utflength = utf_get_number_of_u2s(u);

	java_handle_t*           h  = builtin_new(class_java_lang_String);
	CharArray ca(utflength);

	/* javastring or character-array could not be created */
	if ((h == NULL) || ca.is_null())
		return NULL;

	// XXX: Fix me!
	uint16_t* ptr = (uint16_t*) ca.get_raw_data_ptr();

	/* decompress utf-string */

	for (int32_t i = 0; i < utflength; i++) {
		uint16_t ch = utf_nextu2(&utf_ptr);

		if (ch == '/')
			ch = '.';

		ptr[i] = ch;
	}
	
	/* set fields of the javastring-object */

	java_lang_String jls(h, ca.get_handle(), utflength);

	return jls.get_handle();
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
	if (text == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	int32_t len = strlen(text);

	java_handle_t*           h  = builtin_new(class_java_lang_String);
	CharArray ca(len);

	/* javastring or character-array could not be created */

	if ((h == NULL) || ca.is_null())
		return NULL;

	// XXX: Fix me!
	uint16_t* ptr = (uint16_t*) ca.get_raw_data_ptr();

	/* copy text */

	for (int32_t i = 0; i < len; i++)
		ptr[i] = text[i];
	
	/* set fields of the javastring-object */

	java_lang_String jls(h, ca.get_handle(), len);

	return jls.get_handle();
}


/* javastring_tochar ***********************************************************

   converts a Java string into a C string.
	
   return: pointer to C string
	
   Caution: calling method MUST release the allocated memory!
	
*******************************************************************************/

char* javastring_tochar(java_handle_t* h)
{
	java_lang_String jls(h);

	if (jls.is_null())
		return (char*) "";

	CharArray ca(jls.get_value());

	if (ca.is_null())
		return (char*) "";

	int32_t count  = jls.get_count();
	int32_t offset = jls.get_offset();

	char* buf = MNEW(char, count + 1);

	// XXX: Fix me!
	uint16_t* ptr = (uint16_t*) ca.get_raw_data_ptr();

	int32_t i;
	for (i = 0; i < count; i++)
		buf[i] = ptr[offset + i];

	buf[i] = '\0';

	return buf;
}


/* javastring_toutf ************************************************************

   Make utf symbol from javastring.

*******************************************************************************/

utf *javastring_toutf(java_handle_t *string, bool isclassname)
{
	java_lang_String jls(string);

	if (jls.is_null())
		return utf_null;

	CharArray ca(jls.get_value());

	if (ca.is_null())
		return utf_null;

	int32_t count  = jls.get_count();
	int32_t offset = jls.get_offset();

	// XXX: Fix me!
	uint16_t* ptr = (uint16_t*) ca.get_raw_data_ptr();

	return utf_new_u2(ptr + offset, count, isclassname);
}


/* literalstring_u2 ************************************************************

   Searches for the literalstring with the specified u2-array in the
   string hashtable, if there is no such string a new one is created.

   If copymode is true a copy of the u2-array is made.

*******************************************************************************/

static java_handle_t *literalstring_u2(java_handle_chararray_t *a, int32_t length,
									   u4 offset, bool copymode)
{
	literalstring    *s;                /* hashtable element                  */
	u4                key;
	u4                slot;
	u2                i;

	mutex->lock();

	// XXX: Fix me!
	CharArray ca(a);
	uint16_t* ptr = (uint16_t*) ca.get_raw_data_ptr();

	/* find location in hashtable */

	key  = unicode_hashkey(ptr + offset, length);
	slot = key & (hashtable_string.size - 1);
	s    = (literalstring*) hashtable_string.ptr[slot];

	while (s) {
		// FIXME
		java_lang_String js(LLNI_WRAP(s->string));

		if (length == js.get_count()) {
			/* compare text */

			for (i = 0; i < length; i++) {
				// FIXME This is not handle capable!
				CharArray jsca(js.get_value());
				uint16_t* sptr = (uint16_t*) jsca.get_raw_data_ptr();
				
				if (ptr[offset + i] != sptr[i])
					goto nomatch;
			}

			/* string already in hashtable, free memory */

			if (!copymode)
				mem_free(a, sizeof(java_chararray_t) + sizeof(u2) * (length - 1) + 10);

			mutex->unlock();

			return js.get_handle();
		}

	nomatch:
		/* follow link in external hash chain */
		s = s->hashlink;
	}

	java_chararray_t* acopy;
	if (copymode) {
		/* create copy of u2-array for new javastring */
		u4 arraysize = sizeof(java_chararray_t) + sizeof(u2) * (length - 1) + 10;
		acopy = (java_chararray_t*) mem_alloc(arraysize);
/*    		memcpy(ca, a, arraysize); */
		memcpy(&(acopy->header), &(((java_chararray_t*) a)->header), sizeof(java_array_t));
		memcpy(&(acopy->data), &(((java_chararray_t*) a)->data) + offset, sizeof(u2) * (length - 1) + 10);
	}
	else {
		acopy = (java_chararray_t*) a;
	}

	/* location in hashtable found, complete arrayheader */

	acopy->header.objheader.vftbl = Primitive::get_arrayclass_by_type(ARRAYTYPE_CHAR)->vftbl;
	acopy->header.size            = length;

	assert(class_java_lang_String);
	assert(class_java_lang_String->state & CLASS_LOADED);

	// Create a new java.lang.String object on the system heap.
	java_object_t* o = (java_object_t*) MNEW(uint8_t, class_java_lang_String->instancesize);

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		size_string += sizeof(class_java_lang_String->instancesize);
#endif

#if defined(ENABLE_THREADS)
	Lockword(o->lockword).init();
#endif

	o->vftbl = class_java_lang_String->vftbl;

	CharArray cacopy((java_handle_chararray_t*) acopy);
	java_lang_String jls(o, cacopy.get_handle(), length);

	/* create new literalstring */

	s = NEW(literalstring);

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		size_string += sizeof(literalstring);
#endif

	s->hashlink = (literalstring*) hashtable_string.ptr[slot];
	s->string   = (java_object_t*) LLNI_UNWRAP(jls.get_handle());
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
		hashtable      newhash;                          /* the new hashtable */
      
		/* create new hashtable, double the size */

		hashtable_create(&newhash, hashtable_string.size * 2);
		newhash.entries = hashtable_string.entries;
      
		/* transfer elements to new hashtable */

		for (i = 0; i < hashtable_string.size; i++) {
			s = (literalstring*) hashtable_string.ptr[i];

			while (s) {
				nexts = s->hashlink;
				java_lang_String tmpjls(LLNI_WRAP(s->string));
				// FIXME This is not handle capable!
				slot  = unicode_hashkey(((java_chararray_t*) LLNI_UNWRAP(tmpjls.get_value()))->data, tmpjls.get_count()) & (newhash.size - 1);
	  
				s->hashlink = (literalstring*) newhash.ptr[slot];
				newhash.ptr[slot] = s;
	
				/* follow link in external hash chain */
				s = nexts;
			}
		}
	
		/* dispose old table */

		MFREE(hashtable_string.ptr, void*, hashtable_string.size);
		hashtable_string = newhash;
	}

	mutex->unlock();

	return (java_object_t*) LLNI_UNWRAP(jls.get_handle());
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
	a = (java_chararray_t*) mem_alloc(sizeof(java_chararray_t) + sizeof(u2) * (utflength - 1) + 10);

	/* convert utf-string to u2-array */
	for (i = 0; i < utflength; i++)
		a->data[i] = utf_nextu2(&utf_ptr);

	return literalstring_u2((java_handle_chararray_t*) a, utflength, 0, false);
}


/* literalstring_free **********************************************************

   Removes a literalstring from memory.

*******************************************************************************/

#if 0
/* TWISTI This one is currently not used. */

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
#endif


/* javastring_intern ***********************************************************

   Intern the given Java string.

*******************************************************************************/

java_handle_t *javastring_intern(java_handle_t *string)
{
	java_lang_String jls(string);

	CharArray ca(jls.get_value());

	int32_t count  = jls.get_count();
	int32_t offset = jls.get_offset();

	java_handle_t* o = literalstring_u2(ca.get_handle(), count, offset, true);

	return o;
}


/* javastring_fprint ***********************************************************

   Print the given Java string to the given stream.

*******************************************************************************/

void javastring_fprint(java_handle_t *s, FILE *stream)
{
	java_lang_String jls(s);

	CharArray ca(jls.get_value());

	int32_t count  = jls.get_count();
	int32_t offset = jls.get_offset();

	// XXX: Fix me!
	uint16_t* ptr = (uint16_t*) ca.get_raw_data_ptr();

	for (int32_t i = offset; i < offset + count; i++) {
		uint16_t c = ptr[i];
		fputc(c, stream);
	}
}


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
