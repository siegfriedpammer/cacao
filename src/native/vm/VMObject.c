/* src/native/vm/VMObject.c - java/lang/VMObject

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
            Christian Thalinger

   $Id: VMObject.c 2459 2005-05-12 23:21:10Z twisti $

*/


#include <stdlib.h>
#include <string.h>

#include "mm/boehm.h"
#include "mm/memory.h"
#include "toolbox/logging.h"
#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_Cloneable.h"
#include "native/include/java_lang_Object.h"

#if defined(USE_THREADS)
# if defined(NATIVE_THREADS)
#  include "threads/native/threads.h"
# else
#  include "threads/green/threads.h"
#  include "threads/green/locks.h"
# endif
#endif

#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/stringlocal.h"


/*
 * Class:     java/lang/VMObject
 * Method:    getClass
 * Signature: (Ljava/lang/Object;)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMObject_getClass(JNIEnv *env, jclass clazz, java_lang_Object *obj)
{
	classinfo *c;

	if (!obj)
		return NULL;

	c = ((java_objectheader *) obj)->vftbl->class;

	use_class_as_object(c);

	return (java_lang_Class *) c;
}


/*
 * Class:     java/lang/VMObject
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
		initObjectLock((java_objectheader *) new);
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
	initObjectLock((java_objectheader *) new);
#endif

    return new;
}


/*
 * Class:     java/lang/VMObject
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
 * Class:     java/lang/VMObject
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
 * Class:     java/lang/VMObject
 * Method:    wait
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMObject_wait(JNIEnv *env, jclass clazz, java_lang_Object *this, s8 time, s4 par3)
{
	if (runverbose)
		log_text("java_lang_VMObject_wait called");

#if defined(USE_THREADS)
	wait_cond_for_object(&this->header, time, par3);
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
