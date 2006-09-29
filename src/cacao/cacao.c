/* src/cacao/cacao.c - contains main() of cacao

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, J. Wenninger, Institut f. Computersprachen - TU Wien

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

   Contact: cacao@cacaojvm.org

   Authors: Reinhard Grafl

   Changes: Andi Krall
            Mark Probst
            Philipp Tomsich
            Christian Thalinger

   $Id: cacao.c 5579 2006-09-29 11:37:12Z twisti $

*/


#include "config.h"

#include <assert.h>

#if defined(ENABLE_LIBJVM)
# include <ltdl.h>
#endif

#include <stdlib.h>
#include <string.h>

#include "vm/types.h"

#include "native/jni.h"

#if defined(ENABLE_JVMTI)
# include "native/jvmti/jvmti.h"
# include "native/jvmti/cacaodbg.h"
#endif

#include "vm/vm.h"


/* forward declarations *******************************************************/

static JavaVMInitArgs *cacao_options_prepare(int argc, char **argv);


/* main ************************************************************************

   The main program.
   
*******************************************************************************/

int main(int argc, char **argv)
{
#if defined(ENABLE_LIBJVM)	
	/* Variables for JNI_CreateJavaVM dlopen call. */
	lt_dlhandle     libjvm_handle;
	lt_ptr          libjvm_createvm;
	lt_ptr          libjvm_vm_run;

	s4 (*JNI_CreateJavaVM)(JavaVM **, void **, void *);
	void (*vm_run)(JavaVM *, JavaVMInitArgs *);
#endif

	JavaVM         *vm;                 /* denotes a Java VM                  */
	JNIEnv         *env;
	JavaVMInitArgs *vm_args;

	/* prepare the options */

	vm_args = cacao_options_prepare(argc, argv);
	
	/* load and initialize a Java VM, return a JNI interface pointer in env */

#if defined(ENABLE_LIBJVM)
	if (lt_dlinit()) {
		fprintf(stderr, "lt_dlinit failed: %s\n", lt_dlerror());
		abort();
	}

	if (!(libjvm_handle = lt_dlopenext(CACAO_LIBDIR"/libjvm"))) {
		fprintf(stderr, "lt_dlopenext failed: %s\n", lt_dlerror());
		abort();
	}

	if (!(libjvm_createvm = lt_dlsym(libjvm_handle, "JNI_CreateJavaVM"))) {
		fprintf(stderr, "lt_dlsym failed: %s\n", lt_dlerror());
		abort();
	}

	JNI_CreateJavaVM =
		(s4 (*)(JavaVM **, void **, void *)) (ptrint) libjvm_createvm;
#endif

	/* create the Java VM */

	JNI_CreateJavaVM(&vm, (void *) &env, vm_args);

#if defined(ENABLE_JVMTI)
	pthread_mutex_init(&dbgcomlock,NULL);
	if (jvmti) jvmti_set_phase(JVMTI_PHASE_START);
#endif

#if defined(ENABLE_LIBJVM)
	if (!(libjvm_vm_run = lt_dlsym(libjvm_handle, "vm_run"))) {
		fprintf(stderr, "lt_dlsym failed: %s\n", lt_dlerror());
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
