/* nat/VMString.c - java/lang/String

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

   Authors: Roman Obermaiser

   Changes: Christian Thalinger

   $Id: VMString.c 913 2004-02-05 21:23:19Z twisti $

*/


#include <stdlib.h>
#include "jni.h"
#include "native.h"
#include "java_lang_String.h"

#include <stdio.h>

/*
 * Class:     java_lang_VMString
 * Method:    intern
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT java_lang_String* JNICALL Java_java_lang_VMString_intern(JNIEnv *env, jclass clazz, java_lang_String *this)
{
/*  	printf("Java_java_lang_VMString_intern: count=%d offset=%d", this->count, this->offset); */
/*  	utf_display(javastring_toutf(this, 0)); */
/*  	printf("\n"); */
/*  	fflush(stdout); */

	if (this) {
		/* search table so identical strings will get identical pointers */
		return (java_lang_String *) literalstring_u2(this->value, this->count, this->offset, true);

	} else {
		return NULL;
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
 */
