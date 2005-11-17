/* native/vm/VMString.c - java/lang/VMString

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

   Authors: Roman Obermaiser

   Changes: Christian Thalinger

   $Id: VMString.c 3688 2005-11-17 10:21:35Z twisti $

*/


#include <stdlib.h>

#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_String.h"
#include "vm/stringlocal.h"


/*
 * Class:     java/lang/VMString
 * Method:    intern
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT java_lang_String* JNICALL Java_java_lang_VMString_intern(JNIEnv *env, jclass clazz, java_lang_String *str)
{
	java_objectheader *o;

	if (!str)
		return NULL;

	/* search table so identical strings will get identical pointers */

	o = literalstring_u2(str->value, str->count, str->offset, true);

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
