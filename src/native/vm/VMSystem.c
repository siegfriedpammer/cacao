/* native/vm/VMSystem.c - java/lang/VMSystem

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

   Changes: Joseph Wenninger

   $Id: VMSystem.c 1735 2004-12-07 14:33:27Z twisti $

*/


#include <string.h>
#include <time.h>

#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_Object.h"
#include "toolbox/logging.h"
#include "vm/exceptions.h"
#include "vm/builtin.h"


/*
 * Class:     java/lang/System
 * Method:    arraycopy
 * Signature: (Ljava/lang/Object;ILjava/lang/Object;II)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMSystem_arraycopy(JNIEnv *env, jclass clazz, java_lang_Object *source, s4 sp, java_lang_Object *dest, s4 dp, s4 len)
{
	s4 i;
	java_arrayheader *s = (java_arrayheader *) source;
	java_arrayheader *d = (java_arrayheader *) dest;
	arraydescriptor *sdesc;
	arraydescriptor *ddesc;

	/*	printf("arraycopy: %p:%x->%p:%x\n",source,sp,dest,dp);
		fflush(stdout);*/

	if (!s || !d) { 
		*exceptionptr = new_exception(string_java_lang_NullPointerException);
		return; 
	}

	sdesc = s->objheader.vftbl->arraydesc;
	ddesc = d->objheader.vftbl->arraydesc;

	if (!sdesc || !ddesc || (sdesc->arraytype != ddesc->arraytype)) {
		*exceptionptr = new_exception(string_java_lang_ArrayStoreException);
		return; 
	}

	if ((len < 0) || (sp < 0) || (sp + len > s->size) || (dp < 0) || (dp + len > d->size)) {
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);
		return; 
	}

	if (sdesc->componentvftbl == ddesc->componentvftbl) {
		/* We copy primitive values or references of exactly the same type */
		s4 dataoffset = sdesc->dataoffset;
		s4 componentsize = sdesc->componentsize;
		memmove(((u1 *) d) + dataoffset + componentsize * dp,
				((u1 *) s) + dataoffset + componentsize * sp,
				(size_t) len * componentsize);

	} else {
		/* We copy references of different type */
		java_objectarray *oas = (java_objectarray *) s;
		java_objectarray *oad = (java_objectarray *) d;
                
		if (dp <= sp) {
			for (i = 0; i < len; i++) {
				java_objectheader *o = oas->data[sp + i];
				if (!builtin_canstore(oad, o)) {
					*exceptionptr = new_exception(string_java_lang_ArrayStoreException);
					return;
				}
				oad->data[dp + i] = o;
			}

		} else {
			/* XXX this does not completely obey the specification!
			 * If an exception is thrown only the elements above
			 * the current index have been copied. The
			 * specification requires that only the elements
			 * *below* the current index have been copied before
			 * the throw.
			 */
			for (i = len - 1; i >= 0; i--) {
				java_objectheader *o = oas->data[sp + i];
				if (!builtin_canstore(oad, o)) {
					*exceptionptr = new_exception(string_java_lang_ArrayStoreException);
					return;
				}
				oad->data[dp + i] = o;
			}
		}
	}
}


/*
 * Class:     java/lang/System
 * Method:    identityHashCode
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMSystem_identityHashCode(JNIEnv *env, jclass clazz, java_lang_Object *par1)
{
	return ((char *) par1) - ((char *) 0);
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
