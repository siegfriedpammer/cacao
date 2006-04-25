/* src/vm/options.c - contains global options

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
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

   Contact: cacao@cacaojvm.org

   Authors: Christian Thalinger

   Changes:

   $Id: options.c 4833 2006-04-25 12:00:58Z edwin $

*/


#include "config.h"

#include <string.h>

#include "vm/types.h"

#include "mm/memory.h"
#include "native/jni.h"
#include "vm/options.h"


/* command line option ********************************************************/

s4    opt_index = 0;            /* index of processed arguments               */
char *opt_arg;                  /* this one exports the option argument       */

bool opt_jar = false;

#if defined(ENABLE_JIT)
bool opt_jit = true;            /* JIT mode execution (default)               */
bool opt_intrp = false;         /* interpreter mode execution                 */
#else
bool opt_jit = false;           /* JIT mode execution                         */
bool opt_intrp = true;          /* interpreter mode execution (default)       */
#endif

bool opt_run = true;

s4   opt_stacksize = 0;         /* thread stack size                          */

bool opt_verbose = false;
bool compileall = false;

bool loadverbose = false;
bool linkverbose = false;
bool initverbose = false;

bool opt_verboseclass     = false;
bool opt_verbosegc        = false;
bool opt_verbosejni       = false;
bool opt_verbosecall      = false;      /* trace all method invocation        */
bool opt_verboseexception = false;

bool showmethods = false;
bool showconstantpool = false;
bool showutf = false;

char *opt_method = NULL;
char *opt_signature = NULL;

bool compileverbose =  false;           /* trace compiler actions             */
bool showstack = false;
bool opt_showdisassemble = false;       /* generate disassembler listing      */
bool opt_showddatasegment = false;      /* generate data segment listing      */
bool opt_showintermediate = false;      /* generate intermediate code listing */
bool opt_showexceptionstubs = false;
bool opt_shownativestub = false;

bool useinlining = false;      /* use method inlining                        */
bool inlinevirtuals = false;   /* inline unique virtual methods              */
bool inlineexceptions = false; /* inline methods, that contain excptions     */
bool inlineparamopt = false;   /* optimize parameter passing to inlined methods */
bool inlineoutsiders = false;  /* inline methods, that are not member of the invoker's class */

bool checkbounds = true;       /* check array bounds                         */
bool checknull = true;         /* check null pointers                        */
bool opt_noieee = false;       /* don't implement ieee compliant floats      */
bool checksync = true;         /* do synchronization                         */
#if defined(ENABLE_LOOP)
bool opt_loops = false;        /* optimize array accesses in loops           */
#endif

bool makeinitializations = true;

bool getloadingtime = false;   /* to measure the runtime                     */
bool getcompilingtime = false; /* compute compile time                       */

bool opt_stat    = false;
#if defined(ENABLE_VERIFIER)
bool opt_verify  = true;       /* true if classfiles should be verified      */
#endif
bool opt_eager   = false;

bool opt_prof    = false;
bool opt_prof_bb = false;


/* optimization options *******************************************************/

#if defined(ENABLE_IFCONV)
bool opt_ifconv = false;
#endif

#if defined(ENABLE_LSRA)
bool opt_lsra = false;
#endif


/* interpreter options ********************************************************/

#if defined(ENABLE_INTRP)
bool opt_no_dynamic = false;            /* suppress dynamic superinstructions */
bool opt_no_replication = false;        /* don't use replication in intrp     */
bool opt_no_quicksuper = false;         /* instructions for quickening cannot be
										   part of dynamic superinstructions */

s4   opt_static_supers = 0x7fffffff;
bool vm_debug = false;          /* XXX this should be called `opt_trace'      */
#endif


/* options_get *****************************************************************

   DOCUMENT ME!!!

*******************************************************************************/

s4 options_get(opt_struct *opts, JavaVMInitArgs *vm_args)
{
	char *option;
	s4    i;

	if (opt_index >= vm_args->nOptions)
		return OPT_DONE;

	/* get the current option */

	option = vm_args->options[opt_index].optionString;

	if ((option == NULL) || (option[0] != '-'))
		return OPT_DONE;

	for (i = 0; opts[i].name; i++) {
		if (!opts[i].arg) {
			/* boolean option found */

			if (strcmp(option + 1, opts[i].name) == 0) {
				opt_index++;
				return opts[i].value;
			}

		} else {
			/* parameter option found */

			/* with a space between */

			if (strcmp(option + 1, opts[i].name) == 0) {
				opt_index++;

				if (opt_index < vm_args->nOptions) {
					opt_arg = strdup(vm_args->options[opt_index].optionString);
					opt_index++;
					return opts[i].value;
				}

				return OPT_ERROR;

			} else {
				/* parameter and option have no space between */

				size_t l = strlen(opts[i].name);

				if (strlen(option + 1) > l) {
					if (memcmp(option + 1, opts[i].name, l) == 0) {
						opt_index++;
						opt_arg = strdup(option + 1 + l);
						return opts[i].value;
					}
				}
			}
		}
	}

	return OPT_ERROR;
}


/* options_prepare *************************************************************

   Prepare the JavaVMInitArgs.

*******************************************************************************/

JavaVMInitArgs *options_prepare(int argc, char **argv)
{
	JavaVMInitArgs *vm_args;
	s4              i;

	vm_args = NEW(JavaVMInitArgs);

	vm_args->version            = JNI_VERSION_1_2;
	vm_args->nOptions           = argc - 1;
	vm_args->options            = MNEW(JavaVMOption, argc);
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
