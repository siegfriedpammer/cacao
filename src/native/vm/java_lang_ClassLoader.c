/* src/native/vm/java_lang_ClassLoader.c - java/lang/ClassLoader

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

   $Id: java_lang_VMClass.c 6131 2006-12-06 22:15:57Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <string.h>

#include "vm/types.h"

#include "mm/memory.h"

#include "vm/global.h"                          /* required by native headers */

#include "native/jni.h"
#include "native/llni.h"

/* keep this order of the native includes */

#include "native/include/java_lang_Object.h"

#if defined(ENABLE_JAVASE)
# include "native/include/java_lang_String.h"            /* required by j.l.C */

# if defined(WITH_CLASSPATH_SUN)
#  include "native/include/java_nio_ByteBuffer.h"       /* required by j.l.CL */
# endif

# include "native/include/java_lang_ClassLoader.h"       /* required by j.l.C */
# include "native/include/java_lang_Class.h"
# include "native/include/java_security_ProtectionDomain.h"
#endif

#include "vm/exceptions.h"
#include "vm/stringlocal.h"

#include "vmcore/class.h"
#include "vmcore/classcache.h"
#include "vmcore/options.h"

#if defined(ENABLE_STATISTICS)
# include "vmcore/statistics.h"
#endif


/*
 * Class:     java/lang/ClassLoader
 * Method:    defineClass
 * Signature: (Ljava/lang/ClassLoader;Ljava/lang/String;[BIILjava/security/ProtectionDomain;)Ljava/lang/Class;
 */
java_lang_Class *_Jv_java_lang_ClassLoader_defineClass(java_lang_ClassLoader *cl, java_lang_String *name, java_handle_bytearray_t *data, s4 offset, s4 len, java_security_ProtectionDomain *pd)
{
	utf               *utfname;
	classinfo         *c;
	classloader       *loader;
	java_lang_Class   *o;

#if defined(ENABLE_JVMTI)
	jint new_class_data_len = 0;
	unsigned char* new_class_data = NULL;
#endif

	/* check if data was passed */

	if (data == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	/* check the indexes passed */

	if ((offset < 0) || (len < 0) || ((offset + len) > LLNI_array_size(data))) {
		exceptions_throw_arrayindexoutofboundsexception();
		return NULL;
	}

	/* add classloader to classloader hashtable */

	assert(cl);
	loader = loader_hashtable_classloader_add((java_handle_t *) cl);

	if (name != NULL) {
		/* convert '.' to '/' in java string */

		utfname = javastring_toutf((java_handle_t *) name, true);
	} 
	else {
		utfname = NULL;
	}

#if defined(ENABLE_JVMTI)
	/* XXX again this will not work because of the indirection cell for classloaders */
	assert(0);
	/* fire Class File Load Hook JVMTI event */

	if (jvmti)
		jvmti_ClassFileLoadHook(utfname, len, (unsigned char *) data->data, 
								loader, (java_handle_t *) pd, 
								&new_class_data_len, &new_class_data);
#endif

	/* define the class */

#if defined(ENABLE_JVMTI)
	/* check if the JVMTI wants to modify the class */

	if (new_class_data == NULL)
		c = class_define(utfname, loader, new_class_data_len, new_class_data); 
	else
#endif
		c = class_define(utfname, loader, len, (const uint8_t *) &LLNI_array_direct(data, offset));

	if (c == NULL)
		return NULL;

	/* for convenience */

	o = LLNI_classinfo_wrap(c);

#if defined(WITH_CLASSPATH_GNU)
	/* set ProtectionDomain */

	LLNI_field_set_ref(o, pd, pd);
#endif

	return o;
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
 * vim:noexpandtab:sw=4:ts=4:
 */
