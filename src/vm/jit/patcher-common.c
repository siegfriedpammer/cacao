/* src/vm/jit/patcher-common.c - architecture independent code patching stuff

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

   $Id$

*/


#include "config.h"

#include <assert.h>

#include "codegen.h"                   /* for PATCHER_NOPS */

#include "mm/memory.h"

#include "toolbox/list.h"
#include "toolbox/logging.h"           /* XXX remove me! */

#include "vm/exceptions.h"
#include "vm/vm.h"                     /* for vm_abort */

#include "vm/jit/code.h"
#include "vm/jit/jit.h"
#include "vm/jit/patcher.h"
#include "vm/jit/patcher-common.h"

#include "vmcore/options.h"


/* patcher_list_create *********************************************************

   TODO

*******************************************************************************/

void patcher_list_create(codeinfo *code)
{
	code->patchers = list_create(OFFSET(patchref_t, linkage));
}


/* patcher_list_free ***********************************************************

   TODO

*******************************************************************************/

void patcher_list_free(codeinfo *code)
{
	patchref_t *pr;

	/* free all elements of the list */

	while((pr = list_first(code->patchers)) != NULL) {
		list_remove(code->patchers, pr);

		FREE(pr, patchref_t);
	}

	/* free the list itself */

	FREE(code->patchers, list_t);
}


/* patcher_list_find ***********************************************************

   TODO

   NOTE: Caller should hold the patcher list lock or maintain
   exclusive access otherwise.

*******************************************************************************/

static patchref_t *patcher_list_find(codeinfo *code, u1 *pc)
{
	patchref_t *pr;

	/* walk through all patcher references for the given codeinfo */

	pr = list_first_unsynced(code->patchers);
	while (pr) {

		if (pr->mpc == (ptrint) pc)
			return pr;

		pr = list_next_unsynced(code->patchers, pr);
	}

	return NULL;
}


/* patcher_add_patch_ref *******************************************************

   Appends a new patcher reference to the list of patching positions.

*******************************************************************************/

void patcher_add_patch_ref(jitdata *jd, functionptr patcher, voidptr ref,
                           s4 disp)
{
	codegendata *cd;
    codeinfo    *code;
    patchref_t  *pr;
    s4           patchmpc;

	cd       = jd->cd;
    code     = jd->code;
    patchmpc = cd->mcodeptr - cd->mcodebase;

    /* allocate patchref on heap (at least freed together with codeinfo) */

	pr = NEW(patchref_t);
	list_add_first_unsynced(code->patchers, pr);

    /* set patcher information (mpc is resolved later) */

    pr->mpc     = patchmpc;
    pr->disp    = disp;
    pr->patcher = patcher;
    pr->ref     = ref;
	pr->mcode   = 0;
	pr->done    = false;

    /* Generate NOPs for opt_shownops. */

    if (opt_shownops)
        PATCHER_NOPS;
}


/* patcher_handler *************************************************************

   TODO

*******************************************************************************/

java_objectheader *patcher_handler(u1 *pc)
{
	codeinfo          *code;
	patchref_t        *pr;
	bool               result;
	java_objectheader *e;

	/* define the patcher function */

	bool (*patcher_function)(patchref_t *);

	/* search the codeinfo for the given PC */

	code = code_find_codeinfo_for_pc(pc);
	assert(code);

	/* enter a monitor on the patcher list */

	LOCK_MONITOR_ENTER(code->patchers);

	/* search the patcher information for the given PC */

	pr = patcher_list_find(code, pc);

	if (pr == NULL)
		vm_abort("patcher_handler: Unable to find patcher reference.");

	if (pr->done) {
		log_println("patcher_handler: double-patching detected!");
		LOCK_MONITOR_ENTER(code->patchers);
		return NULL;
	}

	/* cast the passed function to a patcher function */

	patcher_function = (bool (*)(patchref_t *)) (ptrint) pr->patcher;

	/* call the proper patcher function */

	result = (patcher_function)(pr);

	/* check for return value and exit accordingly */

	if (result == false) {
		e = exceptions_get_and_clear_exception();

		LOCK_MONITOR_EXIT(code->patchers);

		return e;
	}

	pr->done = true; /* XXX this is only preliminary to prevent double-patching */

	LOCK_MONITOR_EXIT(code->patchers);

	return NULL;
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

