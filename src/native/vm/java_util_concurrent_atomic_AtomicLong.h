/* src/native/vm/java_util_concurrent_atomic_AtomicLong.h

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

   $Id: java_lang_VMObject.c 6213 2006-12-18 17:36:06Z twisti $

*/


#ifndef _JV_UTIL_CONCURRENT_ATOMIC_ATOMICLONG_H
#define _JV_UTIL_CONCURRENT_ATOMIC_ATOMICLONG_H

#include "config.h"

#include <stdint.h>

#include "native/jni.h"


/* function prototypes ********************************************************/

int32_t _Jv_java_util_concurrent_atomic_AtomicLong_VMSupportsCS8(JNIEnv *env, jclass clazz);

#endif /* _JV_UTIL_CONCURRENT_ATOMIC_ATOMICLONG_H */


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
