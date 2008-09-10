/* src/native/vm/nativevm.cpp - Register native VM interface functions.

   Copyright (C) 2007, 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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


#include "config.h"

#include <stdint.h>

#include "native/vm/nativevm.hpp"

#include "vm/class.h"
#include "vm/initialize.h"
#include "vm/method.h"
#include "vm/options.h"
#include "vm/os.hpp"

#if defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)
# include "mm/memory.h"

# include "native/native.hpp"

# include "native/vm/openjdk/hpi.hpp"

# include "vm/globals.hpp"
# include "vm/properties.hpp"
# include "vm/utf8.h"
# include "vm/vm.hpp"
#endif


/* nativevm_preinit ************************************************************

   Pre-initialize the implementation specific native stuff.

*******************************************************************************/

void nativevm_preinit(void)
{
	TRACESUBSYSTEMINITIALIZATION("nativevm_preinit");

	/* Register native methods of all classes implemented. */

#if defined(ENABLE_JAVASE)
# if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)

	_Jv_gnu_classpath_VMStackWalker_init();
	_Jv_gnu_classpath_VMSystemProperties_init();
	_Jv_gnu_java_lang_VMCPStringBuilder_init();
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
	_Jv_java_lang_reflect_VMConstructor_init();
	_Jv_java_lang_reflect_VMField_init();
	_Jv_java_lang_reflect_VMMethod_init();
	//_Jv_java_lang_reflect_VMProxy_init();
	_Jv_java_security_VMAccessController_init();
	_Jv_java_util_concurrent_atomic_AtomicLong_init();
	_Jv_sun_misc_Unsafe_init();

#if defined(ENABLE_ANNOTATIONS)
	_Jv_sun_reflect_ConstantPool_init();
#endif

# elif defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)

	// Load libjava.so
	VM* vm = VM::get_current();
	Properties& properties = vm->get_properties();
	const char* boot_library_path = properties.get("sun.boot.library.path");

	size_t len =
		os::strlen(boot_library_path) +
		os::strlen("/libjava.so") +
		os::strlen("0");

	char* p = MNEW(char, len);

	os::strcpy(p, boot_library_path);
	os::strcat(p, "/libjava.so");

	utf* u = utf_new_char(p);

	void* handle = native_library_open(u);

	if (handle == NULL)
		vm_abort("nativevm_init: failed to open libjava.so at: %s", p);

	MFREE(p, char, len);

	native_library_add(u, NULL, handle);

	// Initialize the HPI.
	HPI& hpi = vm->get_hpi();
	hpi.initialize();

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
}


/* nativevm_init ***************************************************************

   Initialize the implementation specific native stuff.

*******************************************************************************/

void nativevm_init(void)
{
	TRACESUBSYSTEMINITIALIZATION("nativevm_init");

#if defined(ENABLE_JAVASE)

# if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)

	// Nothing to do.

# elif defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)

	methodinfo* m = class_resolveclassmethod(class_java_lang_System,
											 utf_new_char("initializeSystemClass"),
											 utf_void__void,
											 class_java_lang_Object,
											 false);

	if (m == NULL)
		os::abort("nativevm_init: Error resolving java.lang.System.initializeSystemClass()");

	(void) vm_call_method(m, NULL);

# else
#  error unknown classpath configuration
# endif

#elif defined(ENABLE_JAVAME_CLDC1_1)

	// Nothing to do.

#else
# error unknown Java configuration
#endif
}


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
