/* nat/VMObject.c - java/lang/Object

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

   Changes: Joseph Wenninger

   $Id: VMObject.c 1344 2004-07-21 17:12:53Z twisti $

*/


#include <stdlib.h>
#include <string.h>
#include "exceptions.h"
#include "jni.h"
#include "builtin.h"
#include "loader.h"
#include "native.h"
#include "options.h"
#include "mm/boehm.h"
#include "threads/locks.h"
#include "toolbox/logging.h"
#include "toolbox/memory.h"
#include "java_lang_Cloneable.h"
#include "java_lang_Object.h"


/*
 * Class:     java/lang/Object
 * Method:    clone
 * Signature: ()Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_java_lang_VMObject_clone(JNIEnv *env, jclass clazz, java_lang_Cloneable *this)
{
	classinfo *c;
	java_lang_Object *new;
	arraydescriptor *desc;
    
	if ((desc = this->header.vftbl->arraydesc) != NULL) {
		/* We are cloning an array */
        
		u4 size = desc->dataoffset + desc->componentsize * ((java_arrayheader *) this)->size;
        
		new = (java_lang_Object *) heap_allocate(size, (desc->arraytype == ARRAYTYPE_OBJECT), NULL);
		memcpy(new, this, size);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
		new->header.monitorBits = 0;
#endif
        
		return new;
	}
    
    /* We are cloning a non-array */
    if (!builtin_instanceof((java_objectheader *) this, class_java_lang_Cloneable)) {
        *exceptionptr =
			new_exception(string_java_lang_CloneNotSupportedException);
        return NULL;
    }

    /* XXX should use vftbl */
    c = this->header.vftbl->class;
    new = (java_lang_Object *) builtin_new(c);

    if (!new)
        return NULL;

    memcpy(new, this, c->instancesize);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	new->header.monitorBits = 0;
#endif

    return new;
}


/*
 * Class:     java/lang/Object
 * Method:    notify
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMObject_notify(JNIEnv *env, jclass clazz, java_lang_Object *this)
{
	if (runverbose)
		log_text("java_lang_Object_notify called");

#if defined(USE_THREADS)
	signal_cond_for_object(&this->header);
#endif
}


/*
 * Class:     java/lang/Object
 * Method:    notifyAll
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMObject_notifyAll(JNIEnv *env, jclass clazz, java_lang_Object *this)
{
	if (runverbose)
		log_text("java_lang_Object_notifyAll called");

#if defined(USE_THREADS)
	broadcast_cond_for_object(&this->header);
#endif
}


/*
 * Class:     java/lang/Object
 * Method:    wait
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMObject_wait(JNIEnv *env, jclass clazz, java_lang_Object *this, s8 time, s4 par3)
{
	if (runverbose)
		log_text("java_lang_VMObject_wait called");

#if defined(USE_THREADS)
	wait_cond_for_object(&this->header, time);
#endif
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
