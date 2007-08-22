/* src/native/vm/nativevm.c - register the native functions

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

   $Id: native.c 7906 2007-05-14 17:25:33Z twisti $

*/


#include "config.h"

#include <stdint.h>

#if defined(WITH_CLASSPATH_SUN)
# include <string.h>
#endif

#include "native/vm/nativevm.h"

#include "vmcore/method.h"

#if defined(WITH_CLASSPATH_SUN)
# include "mm/memory.h"

# include "native/native.h"

# include "vm/properties.h"
# include "vm/vm.h"

# include "vmcore/class.h"
# include "vmcore/utf8.h"
#endif


/* nativevm_preinit ************************************************************

   Pre-initialize the implementation specific native stuff.

*******************************************************************************/

bool nativevm_preinit(void)
{
	/* register native methods of all classes implemented */

#if defined(ENABLE_JAVASE)

# if defined(WITH_CLASSPATH_GNU)

	_Jv_gnu_classpath_VMStackWalker_init();
	_Jv_gnu_classpath_VMSystemProperties_init();
	_Jv_gnu_java_lang_management_VMClassLoadingMXBeanImpl_init();
	_Jv_gnu_java_lang_management_VMMemoryMXBeanImpl_init();
	_Jv_gnu_java_lang_management_VMRuntimeMXBeanImpl_init();
	_Jv_gnu_java_lang_management_VMThreadMXBeanImpl_init();
	_Jv_java_lang_VMClass_init();
	_Jv_java_lang_VMClassLoader_init();
	_Jv_java_lang_VMObject_init();
	_Jv_java_lang_VMRuntime_init();
	_Jv_java_lang_VMSystem_init();
	_Jv_java_lang_VMString_init();
	_Jv_java_lang_VMThread_init();
	_Jv_java_lang_VMThrowable_init();
	_Jv_java_lang_management_VMManagementFactory_init();
	_Jv_java_lang_reflect_Constructor_init();
	_Jv_java_lang_reflect_Field_init();
	_Jv_java_lang_reflect_Method_init();
	_Jv_java_lang_reflect_VMProxy_init();
	_Jv_java_security_VMAccessController_init();
	_Jv_java_util_concurrent_atomic_AtomicLong_init();
	_Jv_sun_misc_Unsafe_init();

#if defined(ENABLE_ANNOTATIONS)
	_Jv_sun_reflect_ConstantPool_init();
#endif

# elif defined(WITH_CLASSPATH_SUN)

	char        *boot_library_path;
	int          len;
	char        *p;
	utf         *u;
	lt_dlhandle  handle;

	boot_library_path = properties_get("sun.boot.library.path");

	len =
		strlen(boot_library_path) +
		strlen("/libjava.so") +
		strlen("0");

	p = MNEW(char, len);

	strcpy(p, boot_library_path);
	strcat(p, "/libjava.so");

	u = utf_new_char(p);

	MFREE(p, char, len);

	handle = native_library_open(u);
	native_library_add(u, NULL, handle);

	_Jv_sun_misc_Unsafe_init();

# else

#  error unknown classpath configuration

# endif

#elif defined(ENABLE_JAVAME_CLDC1_1)

	_Jv_com_sun_cldc_io_ResourceInputStream_init();
	_Jv_com_sun_cldc_io_j2me_socket_Protocol_init();
	_Jv_com_sun_cldchi_io_ConsoleOutputStream_init();
	_Jv_com_sun_cldchi_jvm_JVM_init();
	_Jv_java_lang_Class_init();
	_Jv_java_lang_Double_init();
	_Jv_java_lang_Float_init();
	_Jv_java_lang_Math_init();
	_Jv_java_lang_Object_init();
	_Jv_java_lang_Runtime_init();
	_Jv_java_lang_String_init();
	_Jv_java_lang_System_init();
	_Jv_java_lang_Thread_init();
	_Jv_java_lang_Throwable_init();

#else

# error unknown Java configuration

#endif

	/* everything's ok */

	return true;
}


/* nativevm_init ***************************************************************

   Initialize the implementation specific native stuff.

*******************************************************************************/

bool nativevm_init(void)
{
#if defined(ENABLE_JAVASE)

# if defined(WITH_CLASSPATH_GNU)

	/* nothing to do */

# elif defined(WITH_CLASSPATH_SUN)

	methodinfo *m;

	m = class_resolveclassmethod(class_java_lang_System,
								 utf_new_char("initializeSystemClass"),
								 utf_void__void,
								 class_java_lang_Object,
								 false);

	if (m == NULL)
		return false;

	(void) vm_call_method(m, NULL);

# else

#  error unknown classpath configuration

# endif

#elif defined(ENABLE_JAVAME_CLDC1_1)

	/* nothing to do */

#else

# error unknown Java configuration

#endif

	/* everything's ok */

	return true;
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
