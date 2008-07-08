/* src/cacao/cacao.c - contains main() of cacao

   Copyright (C) 1996-2005, 2006, 2007, 2008
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

#include <assert.h>

#if defined(ENABLE_JRE_LAYOUT)
# include <errno.h>
# include <libgen.h>
# include <unistd.h>
#endif

#include <stdlib.h>
#include <string.h>

#include "vm/types.h"

#include "native/jni.h"
#include "native/native.h"

#if defined(ENABLE_JVMTI)
# include "native/jvmti/jvmti.h"
# include "native/jvmti/cacaodbg.h"
#endif

#include "vmcore/system.h"

#include "vm/vm.h"


/* Defines. *******************************************************************/

#define LIBJVM_NAME    NATIVE_LIBRARY_PREFIX"jvm"NATIVE_LIBRARY_SUFFIX


/* forward declarations *******************************************************/

static JavaVMInitArgs *cacao_options_prepare(int argc, char **argv);


/* main ************************************************************************

   The main program.
   
*******************************************************************************/

int main(int argc, char **argv)
{
#if defined(ENABLE_LIBJVM)
	char*       path;

# if defined(ENABLE_JRE_LAYOUT)
	int         len;
# endif
#endif

#if defined(ENABLE_LIBJVM)	
	/* Variables for JNI_CreateJavaVM dlopen call. */
	void*       libjvm_handle;
	void*       libjvm_vm_createjvm;
	void*       libjvm_vm_run;
	const char* lterror;

	bool (*vm_createjvm)(JavaVM **, void **, void *);
	void (*vm_run)(JavaVM *, JavaVMInitArgs *);
#endif

	JavaVM         *vm;                 /* denotes a Java VM                  */
	JNIEnv         *env;
	JavaVMInitArgs *vm_args;

	/* prepare the options */

	vm_args = cacao_options_prepare(argc, argv);
	
	/* load and initialize a Java VM, return a JNI interface pointer in env */

#if defined(ENABLE_LIBJVM)
# if defined(ENABLE_JRE_LAYOUT)
	/* SUN also uses a buffer of 4096-bytes (strace is your friend). */

	path = malloc(sizeof(char) * 4096);

	if (readlink("/proc/self/exe", path, 4095) == -1) {
		fprintf(stderr, "main: readlink failed: %s\n", strerror(errno));
		abort();
	}

	/* get the path of the current executable */

	path = dirname(path);
	len  = strlen(path) + strlen("/../lib/"LIBJVM_NAME) + strlen("0");

	if (len > 4096) {
		fprintf(stderr, "main: libjvm name to long for buffer\n");
		abort();
	}

	/* concatinate the library name */

	strcat(path, "/../lib/"LIBJVM_NAME);
# else
	path = CACAO_LIBDIR"/"LIBJVM_NAME;
# endif

	/* First try to open where dlopen searches, e.g. LD_LIBRARY_PATH.
	   If not found, try the absolute path. */

	libjvm_handle = system_dlopen(LIBJVM_NAME, RTLD_NOW);

	if (libjvm_handle == NULL) {
		/* save the error message */

		lterror = strdup(system_dlerror());

		libjvm_handle = system_dlopen(path, RTLD_NOW);

		if (libjvm_handle == NULL) {
			/* print the first error message too */

			fprintf(stderr, "main: system_dlopen failed: %s\n", lterror);

			/* and now the current one */

			fprintf(stderr, "main: system_dlopen failed: %s\n",
					system_dlerror());
			abort();
		}

		/* free the error string */

		free((void *) lterror);
	}

	libjvm_vm_createjvm = system_dlsym(libjvm_handle, "vm_createjvm");

	if (libjvm_vm_createjvm == NULL) {
		fprintf(stderr, "main: lt_dlsym failed: %s\n", system_dlerror());
		abort();
	}

	vm_createjvm =
		(bool (*)(JavaVM **, void **, void *)) (ptrint) libjvm_vm_createjvm;
#endif

	/* create the Java VM */

	(void) vm_createjvm(&vm, (void *) &env, vm_args);

#if defined(ENABLE_JVMTI)
# error This should be a JVMTI function.
	Mutex_init(&dbgcomlock);
	if (jvmti) jvmti_set_phase(JVMTI_PHASE_START);
#endif

#if defined(ENABLE_LIBJVM)
	libjvm_vm_run = system_dlsym(libjvm_handle, "vm_run");

	if (libjvm_vm_run == NULL) {
		fprintf(stderr, "main: system_dlsym failed: %s\n", system_dlerror());
		abort();
	}

	vm_run = (void (*)(JavaVM *, JavaVMInitArgs *)) (ptrint) libjvm_vm_run;
#endif

	/* run the VM */

	vm_run(vm, vm_args);

	/* keep compiler happy */

	return 0;
}


/* cacao_options_prepare *******************************************************

   Prepare the JavaVMInitArgs.

*******************************************************************************/

static JavaVMInitArgs *cacao_options_prepare(int argc, char **argv)
{
	JavaVMInitArgs *vm_args;
	s4              i;

	vm_args = malloc(sizeof(JavaVMInitArgs));

	vm_args->version            = JNI_VERSION_1_2;
	vm_args->nOptions           = argc - 1;
	vm_args->options            = malloc(sizeof(JavaVMOption) * argc);
	vm_args->ignoreUnrecognized = JNI_FALSE;

	for (i = 1; i < argc; i++)
		vm_args->options[i - 1].optionString = argv[i];

	return vm_args;
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
