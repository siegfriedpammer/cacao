/* src/native/vm/java_lang_reflect_Method.c

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

   $Id: java_lang_reflect_Method.c 8063 2007-06-11 14:44:58Z twisti $

*/


#include "config.h"

#include <assert.h>

#include "vm/types.h"

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"

#include "native/include/java_lang_Object.h"

#include "native/include/java_lang_reflect_Method.h"

#include "vm/access.h"
#include "vm/builtin.h"
#include "vm/initialize.h"

#include "vmcore/class.h"
#include "vmcore/method.h"


/*
 * Class:     java/lang/reflect/Method
 * Method:    invoke
 * Signature: (Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;
 */
java_lang_Object *_Jv_java_lang_reflect_Method_invoke(java_lang_reflect_Method *this, java_lang_Object *o, java_handle_objectarray_t *args)
{
	classinfo  *c;
	methodinfo *m;
	s4          override;
	int32_t     slot;

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	m = &(c->methods[slot]);


	/* check method access */

	/* check if we should bypass security checks (AccessibleObject) */

#if defined(WITH_CLASSPATH_GNU)
	LLNI_field_get_val(this, flag, override);
#elif defined(WITH_CLASSPATH_SUN)
	LLNI_field_get_val(this, override, override);
#else
# error unknown classpath configuration
#endif

	if (override == false) {
		if (!access_check_method(m, 1))
			return NULL;
	}

	/* check if method class is initialized */

	if (!(c->state & CLASS_INITIALIZED))
		if (!initialize_class(c))
			return NULL;

	/* call the Java method via a helper function */

	return (java_lang_Object *) _Jv_jni_invokeNative(m, (java_handle_t *) o, args);
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
