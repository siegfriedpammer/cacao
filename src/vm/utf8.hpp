/* src/vm/utf8.h - utf8 string functions

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


#ifndef _UTF_H
#define _UTF_H

/* forward typedefs ***********************************************************/

typedef struct utf utf;

#include "config.h"

#include <stdio.h>

#include "vm/types.h"

#include "vm/global.h"


/* data structure for utf8 symbols ********************************************/

struct utf {
	utf  *hashlink;                     /* link for external hash chain       */
	s4    blength;                      /* text length in bytes               */
	char *text;                         /* pointer to text                    */
};

/* to determine the end of utf strings */

#define UTF_END(u)    ((char *) u->text + u->blength)


/* utf-symbols for pointer comparison of frequently used strings **************/

#define UTF8( NAME, STR ) extern utf *utf_##NAME;
#include "vm/utf8.inc"

/* function prototypes ********************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* initialize the utf8 subsystem */
void utf8_init(void);

u4 utf_hashkey(const char *text, u4 length);
u4 utf_full_hashkey(const char *text, u4 length);

/* determine hashkey of a unicode-symbol */
u4 unicode_hashkey(u2 *text, u2 length);

/* create new utf-symbol */
utf *utf_new(const char *text, u2 length);

/* make utf symbol from u2 array */
utf *utf_new_u2(u2 *unicodedata, u4 unicodelength, bool isclassname);

utf *utf_new_char(const char *text);
utf *utf_new_char_classname(const char *text);

/* get number of bytes */
u4 utf_bytes(utf *u);

/* get next unicode character of a utf-string */
u2 utf_nextu2(char **utf);

/* get (number of) unicode characters of a utf string (safe) */
s4 utf8_safe_number_of_u2s(const char *text, s4 nbytes);
void utf8_safe_convert_to_u2s(const char *text, s4 nbytes, u2 *buffer);

/* get (number of) unicode characters of a utf string (UNSAFE!) */
u4 utf_get_number_of_u2s(utf *u);
u4 utf_get_number_of_u2s_for_buffer(const char *buffer, u4 blength);

/* determine utf length in bytes of a u2 array */
u4 u2_utflength(u2 *text, u4 u2_length);

void utf_copy(char *buffer, utf *u);
void utf_cat(char *buffer, utf *u);
void utf_copy_classname(char *buffer, utf *u);
void utf_cat_classname(char *buffer, utf *u);

/* write utf symbol to file/buffer */
void utf_display_printable_ascii(utf *u);
void utf_display_printable_ascii_classname(utf *u);

void utf_sprint_convert_to_latin1(char *buffer, utf *u);
void utf_sprint_convert_to_latin1_classname(char *buffer, utf *u);

void utf_strcat_convert_to_latin1(char *buffer, utf *u);
void utf_strcat_convert_to_latin1_classname(char *buffer, utf *u);

void utf_fprint_printable_ascii(FILE *file, utf *u);
void utf_fprint_printable_ascii_classname(FILE *file, utf *u);

/* check if a UTF-8 string is valid */
bool is_valid_utf(char *utf_ptr, char *end_pos);

/* check if a UTF-8 string may be used as a class/field/method name */
bool is_valid_name(char *utf_ptr, char *end_pos);
bool is_valid_name_utf(utf *u);

/* show utf-table */
void utf_show(void);

#ifdef __cplusplus
}
#endif

#endif /* _UTF_H */


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
