/* src/native/vm/java_lang_Object.h - java/lang/Object functions

   Copyright (C) 2006 R. Grafl, A. Krall, C. Kruegel, C. Oates,
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Contact: cacao@cacaojvm.org

   Authors: Christian Thalinger

   $Id: java_lang_VMObject.c 6213 2006-12-18 17:36:06Z twisti $

*/


#ifndef _JV_JAVA_LANG_OBJECT_H
#define _JV_JAVA_LANG_OBJECT_H

#include "config.h"
#include "vm/types.h"

#include "native/jni.h"
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_Cloneable.h"
#include "native/include/java_lang_Object.h"


/* function prototypes ********************************************************/

java_lang_Class  *_Jv_java_lang_Object_getClass(java_lang_Object *obj);
java_lang_Object *_Jv_java_lang_Object_clone(java_lang_Cloneable *this);
void              _Jv_java_lang_Object_notify(java_lang_Object *this);
void              _Jv_java_lang_Object_notifyAll(java_lang_Object *this);
void              _Jv_java_lang_Object_wait(java_lang_Object *o, s8 ms, s4 ns);

#endif /* _JV_JAVA_LANG_OBJECT_H */


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