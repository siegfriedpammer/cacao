/* src/native/vm/reflect.h - helper functions for java/lang/reflect

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

   $Id: reflect.h 8172 2007-06-30 14:14:52Z twisti $

*/


#include "config.h"

#include <stdint.h>

#include "native/jni.h"
#include "native/native.h"

/* keep this order of the native includes */

#include "native/include/java_lang_String.h"

#if defined(ENABLE_JAVASE)
# if defined(WITH_CLASSPATH_SUN)
#  include "native/include/java_nio_ByteBuffer.h"       /* required by j.l.CL */
# endif
# include "native/include/java_lang_ClassLoader.h"
#endif

#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_Class.h"

#if defined(ENABLE_JAVASE)
# include "native/include/java_lang_reflect_Constructor.h"
# include "native/include/java_lang_reflect_Field.h"
# include "native/include/java_lang_reflect_Method.h"
#endif

#include "vmcore/field.h"
#include "vmcore/method.h"


/* function prototypes ********************************************************/

java_lang_reflect_Constructor *reflect_constructor_new(methodinfo *m);
java_lang_reflect_Field       *reflect_field_new(fieldinfo *f);
java_lang_reflect_Method      *reflect_method_new(methodinfo *m);


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
