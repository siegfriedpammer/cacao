/* src/native/vm/java_lang_String.c

   Copyright (C) 2007 R. Grafl, A. Krall, C. Kruegel,
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

   $Id: java_lang_VMString.c 7910 2007-05-16 08:02:52Z twisti $

*/


#include "config.h"

#include <stdlib.h>

#include "native/jni.h"
#include "native/llni.h"

#include "native/include/java_lang_String.h"

#include "vm/stringlocal.h"


/*
 * Class:     java/lang/String
 * Method:    intern
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
java_lang_String *_Jv_java_lang_String_intern(java_lang_String *s)
{
	java_handle_t *o;

	if (s == NULL)
		return NULL;

	/* search table so identical strings will get identical pointers */

	o = literalstring_u2(LLNI_field_direct(s, value), LLNI_field_direct(s, count), LLNI_field_direct(s, offset), true);

	return (java_lang_String *) o;
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
