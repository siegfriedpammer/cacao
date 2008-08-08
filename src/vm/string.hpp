/* src/vm/string.hpp - string header

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


#ifndef _STRINGLOCAL_H
#define _STRINGLOCAL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct literalstring literalstring;


#include "config.h"

#include "vm/types.h"

#include "toolbox/hashtable.h"

#include "vm/global.h"
#include "vm/os.hpp"
#include "vm/utf8.h"


/* data structure of internal javastrings stored in global hashtable **********/

struct literalstring {
	literalstring     *hashlink;        /* link for external hash chain       */
	java_object_t     *string;  
};


/* function prototypes ********************************************************/

/* initialize string subsystem */
bool string_init(void);

void stringtable_update(void);

/* creates a new object of type java/lang/String from a utf-text */
java_handle_t *javastring_new(utf *text);

/* creates a new object of type java/lang/String from a utf-text, changes slashes to dots */
java_handle_t *javastring_new_slash_to_dot(utf *text);

/* creates a new object of type java/lang/String from an ASCII c-string */
java_handle_t *javastring_new_from_ascii(const char *text);

/* creates a new object of type java/lang/String from UTF-8 */
java_handle_t *javastring_new_from_utf_string(const char *utfstr);

/* creates a new object of type java/lang/String from (possibly invalid) UTF-8 */
java_handle_t *javastring_safe_new_from_utf8(const char *text);

/* make c-string from a javastring (debugging) */
char *javastring_tochar(java_handle_t *string);

/* make utf symbol from javastring */
utf *javastring_toutf(java_handle_t *string, bool isclassname);

/* creates a new javastring with the text of the utf-symbol */
java_object_t *literalstring_new(utf *u);

java_handle_t *javastring_intern(java_handle_t *s);
void           javastring_fprint(java_handle_t *s, FILE *stream);

#ifdef __cplusplus
}
#endif

#endif /* _STRINGLOCAL_H */


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
