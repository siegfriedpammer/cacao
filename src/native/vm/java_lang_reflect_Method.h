/* src/native/vm/java_lang_reflect_Method.h

   Copyright (C) 2007 R. Grafl, A. Krall, C. Kruegel, C. Oates,
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

*/


#ifndef _JV_JAVA_LANG_REFLECT_METHOD_H
#define _JV_JAVA_LANG_REFLECT_METHOD_H

#include "config.h"
#include "vm/types.h"

#include "native/jni.h"

#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_reflect_Method.h"

#include "native/vm/java_lang_reflect_Method.h"

#include "vm/global.h"


/* function prototypes ********************************************************/

java_lang_Object *_Jv_java_lang_reflect_Method_invoke(java_lang_reflect_Method *this, java_lang_Object *o, java_handle_objectarray_t *args);

#endif /* _JV_JAVA_LANG_REFLECT_METHOD_H */


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
