/* src/native/vm/nativevm.cpp - Register native VM interface functions.

   Copyright (C) 1996-2012
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

#include "vm/class.hpp"
#include "vm/exceptions.hpp"
#include "vm/initialize.hpp"
#include "vm/method.hpp"
#include "vm/options.h"
#include "vm/os.hpp"

#if defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)
# include "mm/memory.hpp"

# include "native/native.hpp"

#ifndef WITH_JAVA_RUNTIME_LIBRARY_OPENJDK_7
# include "native/vm/openjdk/hpi.hpp"
#endif

# include "toolbox/sequence.hpp"

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

	// Use sequence builder to assemble library path.
	SequenceBuilder sb;

	sb.cat(boot_library_path);
	sb.cat("/libjava.so");

	// XXX This should actually be sb.export_symbol()
	utf* u = utf_new_char(sb.c_str());

	NativeLibrary nl(u);
	void* handle = nl.open();

	if (handle == NULL)
		os::abort("nativevm_init: failed to open libjava.so at: %s", sb.c_str());

	NativeLibraries& nls = vm->get_nativelibraries();
	nls.add(nl);

#ifndef WITH_JAVA_RUNTIME_LIBRARY_OPENJDK_7
	// Initialize the HPI.
	HPI& hpi = vm->get_hpi();
	hpi.initialize();
#endif

	_Jv_sun_misc_Perf_init();
	_Jv_sun_misc_Unsafe_init();

#  if !defined(NDEBUG)
	// Sanity check current time in milliseconds, because negative values
	// might confuse OpenJDKs sanity checks.
	if (opt_PrintWarnings && (builtin_currenttimemillis() < 0))
		log_println("nativevm_preinit: Current time in milliseconds is negative, please check your time!");
#  endif

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

bool nativevm_init(void)
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
		return false;

	(void) vm_call_method(m, NULL);

	if (exceptions_get_exception() != NULL)
		return false;

# else
#  error unknown classpath configuration
# endif

#elif defined(ENABLE_JAVAME_CLDC1_1)

	// Nothing to do.

#else
# error unknown Java configuration
#endif

	return true;
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
