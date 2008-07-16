/* src/native/vm/openjdk/hpi.c - HotSpot HPI interface functions

   Copyright (C) 2008
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

/* We include hpi_md.h before hpi.h as the latter includes the
   former. */

#include INCLUDE_HPI_MD_H
#include INCLUDE_HPI_H

#include "mm/memory.h"

#include "native/jni.h"
#include "native/native.h"

#include "vm/properties.h"
#include "vm/vm.hpp"

#include "vmcore/options.h"
#include "vmcore/system.h"
#include "vmcore/utf8.h"


/* VM callback functions ******************************************************/

static vm_calls_t callbacks = {
	/* TODO What should we use here? */
/*   jio_fprintf, */
/*   unimplemented_panic, */
/*   unimplemented_monitorRegister, */
	NULL,
	NULL,
	NULL,
	
	NULL, /* unused */
	NULL, /* unused */
	NULL  /* unused */
};


/* HPI interfaces *************************************************************/

GetInterfaceFunc      hpi_get_interface = NULL;
HPI_FileInterface    *hpi_file          = NULL;
HPI_SocketInterface  *hpi_socket        = NULL;
HPI_LibraryInterface *hpi_library       = NULL;
HPI_SystemInterface  *hpi_system        = NULL;


/* hpi_initialize **************************************************************

   Initialize the Host Porting Interface (HPI).

*******************************************************************************/

void hpi_initialize(void)
{
	const char* boot_library_path;
	int   len;
	char* p;
	utf*  u;
	void* handle;
	void* dll_initialize;
	int   result;

    jint (JNICALL * DLL_Initialize)(GetInterfaceFunc *, void *);

	TRACESUBSYSTEMINITIALIZATION("hpi_init");

	/* Load libhpi.so */

	boot_library_path = properties_get("sun.boot.library.path");

	len =
		system_strlen(boot_library_path) +
		system_strlen("/native_threads/libhpi.so") +
		system_strlen("0");

	p = MNEW(char, len);

	system_strcpy(p, boot_library_path);
	system_strcat(p, "/native_threads/libhpi.so");

	u = utf_new_char(p);

    if (opt_TraceHPI)
		log_println("hpi_init: Loading HPI %s ", p);

	MFREE(p, char, len);

	handle = native_library_open(u);

	if (handle == NULL)
		if (opt_TraceHPI)
			vm_abort("hpi_init: HPI open failed");

	/* Resolve the DLL_Initialize function from the library. */

	dll_initialize = system_dlsym(handle, "DLL_Initialize");

    DLL_Initialize = (jint (JNICALL *)(GetInterfaceFunc *, void *)) (intptr_t) dll_initialize;

    if (opt_TraceHPI && DLL_Initialize == NULL)
		log_println("hpi_init: HPI dlsym of DLL_Initialize failed: %s",
					system_dlerror());

    if (DLL_Initialize == NULL ||
        (*DLL_Initialize)(&hpi_get_interface, &callbacks) < 0) {

        if (opt_TraceHPI)
			vm_abort("hpi_init: HPI DLL_Initialize failed");
    }

	native_library_add(u, NULL, handle);

    if (opt_TraceHPI)
		log_println("hpi_init: HPI loaded successfully");

	/* Resolve the interfaces. */
	/* NOTE: The intptr_t-case is only to prevent the a compiler
	   warning with -O2: warning: dereferencing type-punned pointer
	   will break strict-aliasing rules */

	result = (*hpi_get_interface)((void **) (intptr_t) &hpi_file, "File", 1);

	if (result != 0)
		vm_abort("hpi_init: Can't find HPI_FileInterface");

	result = (*hpi_get_interface)((void **) (intptr_t) &hpi_library, "Library", 1);

	if (result != 0)
		vm_abort("hpi_init: Can't find HPI_LibraryInterface");

	result = (*hpi_get_interface)((void **) (intptr_t) &hpi_system, "System", 1);

	if (result != 0)
		vm_abort("hpi_init: Can't find HPI_SystemInterface");
}


/* hpi_initialize_socket_library ***********************************************

   Initialize the library Host Porting Interface (HPI).

*******************************************************************************/

int hpi_initialize_socket_library(void)
{
	int result;

	/* Resolve the socket library interface. */

	result = (*hpi_get_interface)((void **) (intptr_t) &hpi_socket, "Socket", 1);

	if (result != 0) {
		if (opt_TraceHPI)
			log_println("hpi_initialize_socket_library: Can't find HPI_SocketInterface");

		return JNI_ERR;
	}

	return JNI_OK;
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
