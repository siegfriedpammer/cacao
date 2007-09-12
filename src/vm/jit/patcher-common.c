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

*/


#include "config.h"

#include <assert.h>
#include <stdint.h>

#include "codegen.h"                   /* for PATCHER_NOPS */

#include "mm/memory.h"

#include "native/native.h"

#include "threads/lock-common.h"

#include "toolbox/list.h"
#include "toolbox/logging.h"           /* XXX remove me! */

#include "vm/exceptions.h"
#include "vm/initialize.h"
#include "vm/resolve.h"
#include "vm/vm.h"                     /* for vm_abort */

#include "vm/jit/code.h"
#include "vm/jit/jit.h"
#include "vm/jit/md.h"
#include "vm/jit/patcher-common.h"

#include "vmcore/options.h"


/* patcher_function_list *******************************************************

   This is a list which maps patcher function pointers to the according
   names of the patcher functions. It is only usefull for debugging
   purposes.

*******************************************************************************/

#if !defined(NDEBUG)
typedef struct patcher_function_list_t {
	functionptr  patcher;
	char        *name;
} patcher_function_list_t;

static patcher_function_list_t patcher_function_list[] = {
	{ PATCHER_initialize_class,              "initialize_class" },
	{ PATCHER_resolve_class,                 "resolve_class" },
	{ PATCHER_resolve_native_function,       "resolve_native_function" },
	{ PATCHER_invokestatic_special,          "invokestatic_special" },
	{ PATCHER_invokevirtual,                 "invokevirtual" },
	{ PATCHER_invokeinterface,               "invokeinterface" },
	{ NULL,                                  "-UNKNOWN PATCHER FUNCTION-" }
};
#endif


/* patcher_list_create *********************************************************

   Creates an empty patcher list for the given codeinfo.

*******************************************************************************/

void patcher_list_create(codeinfo *code)
{
	code->patchers = list_create(OFFSET(patchref_t, linkage));
}


/* patcher_list_reset **********************************************************

   Resets the patcher list inside a codeinfo. This is usefull when
   resetting a codeinfo for recompiling.

*******************************************************************************/

void patcher_list_reset(codeinfo *code)
{
	patchref_t *pr;

	/* free all elements of the list */

	while((pr = list_first(code->patchers)) != NULL) {
		list_remove(code->patchers, pr);

		FREE(pr, patchref_t);

#if defined(ENABLE_STATISTICS)
		if (opt_stat)
			size_patchref -= sizeof(patchref_t);
#endif
	}
}

/* patcher_list_free ***********************************************************

   Frees the patcher list and all its entries for the given codeinfo.

*******************************************************************************/

void patcher_list_free(codeinfo *code)
{
	/* free all elements of the list */

	patcher_list_reset(code);

	/* free the list itself */

	FREE(code->patchers, list_t);
}


/* patcher_list_find ***********************************************************

   Find an entry inside the patcher list for the given codeinfo
   by specifying the program counter of the patcher position.

   NOTE: Caller should hold the patcher list lock or maintain
   exclusive access otherwise.

*******************************************************************************/

static patchref_t *patcher_list_find(codeinfo *code, u1 *pc)
{
	patchref_t *pr;

	/* walk through all patcher references for the given codeinfo */

	pr = list_first_unsynced(code->patchers);
	while (pr) {

/*#define TRACE_PATCHER_FIND*/
#ifdef TRACE_PATCHER_FIND
		log_println("patcher_list_find: %p == %p", pr->mpc, pc);
#endif

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

#if !defined(NDEBUG)
	if (patcher_list_find(code, (u1 *) (intptr_t) patchmpc) != NULL)
		vm_abort("patcher_add_patch_ref: different patchers at same position.");
#endif

	/* allocate patchref on heap (at least freed together with codeinfo) */

	pr = NEW(patchref_t);
	list_add_first_unsynced(code->patchers, pr);

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		size_patchref += sizeof(patchref_t);
#endif

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

#if defined(ENABLE_JIT) && (defined(__I386__) || defined(__M68K__) || defined(__SPARC_64__) || defined(__X86_64__))

	/* XXX We can remove that when we don't use UD2 anymore on i386
	   and x86_64. */

	/* On some architectures the patcher stub call instruction might
	   be longer than the actual instruction generated.  On this
	   architectures we store the last patcher call position and after
	   the basic block code generation is completed, we check the
	   range and maybe generate some nop's. */
	/* The nops are generated in codegen_emit in each codegen */

	cd->lastmcodeptr = cd->mcodeptr + PATCHER_CALL_SIZE;
#endif
}


/* patcher_handler *************************************************************

   Handles the request to patch JIT code at the given patching
   position. This function is normally called by the signal
   handler.

   NOTE: The patcher list lock is used to maintain exclusive
   access of the patched position (in fact of the whole code).
   After patching has suceeded, the patcher reference should be
   removed from the patcher list to avoid double patching.

*******************************************************************************/

#if !defined(NDEBUG)
/* XXX this indent is not thread safe! */
/* XXX if you want it thread safe, place patcher_depth in threadobject! */
static int patcher_depth = 0;
#define TRACE_PATCHER_INDENT for (i=0; i<patcher_depth; i++) printf("\t")
#endif /* !defined(NDEBUG) */

java_handle_t *patcher_handler(u1 *pc)
{
	codeinfo      *code;
	patchref_t    *pr;
	bool           result;
	java_handle_t *e;
#if !defined(NDEBUG)
	patcher_function_list_t *l;
	int                      i;
#endif

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
		LOCK_MONITOR_EXIT(code->patchers);
		return NULL;
	}

#if !defined(NDEBUG)
	if (opt_DebugPatcher) {
		for (l = patcher_function_list; l->patcher != NULL; l++)
			if (l->patcher == pr->patcher)
				break;

		TRACE_PATCHER_INDENT; printf("patching in "); method_print(code->m); printf(" at %p\n", (void *) pr->mpc);
		TRACE_PATCHER_INDENT; printf("\tpatcher function = %s <%p>\n", l->name, (void *) pr->patcher);
		TRACE_PATCHER_INDENT; printf("\tmcodes before = "); for (i=0; i<5; i++) printf("0x%08x ", *((u4 *) pr->mpc + i)); printf("\n");
		patcher_depth++;
		assert(patcher_depth > 0);
	}
#endif

	/* cast the passed function to a patcher function */

	patcher_function = (bool (*)(patchref_t *)) (ptrint) pr->patcher;

	/* call the proper patcher function */

	result = (patcher_function)(pr);

#if !defined(NDEBUG)
	if (opt_DebugPatcher) {
		assert(patcher_depth > 0);
		patcher_depth--;
		TRACE_PATCHER_INDENT; printf("\tmcodes after  = "); for (i=0; i<5; i++) printf("0x%08x ", *((u4 *) pr->mpc + i)); printf("\n");
		if (result == false) {
			TRACE_PATCHER_INDENT; printf("\tPATCHER EXCEPTION!\n");
		}
	}
#endif

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


/* patcher_initialize_class ****************************************************

   Initalizes a given classinfo pointer.
   This function does not patch any data.

*******************************************************************************/

bool patcher_initialize_class(patchref_t *pr)
{
	classinfo *c;

	/* get stuff from the patcher reference */

	c = (classinfo *) pr->ref;

	/* check if the class is initialized */

	if (!(c->state & CLASS_INITIALIZED))
		if (!initialize_class(c))
			return false;

	/* patch back original code */

	patcher_patch_code(pr);

	return true;
}


/* patcher_resolve_class *******************************************************

   Resolves a given unresolved class reference.
   This function does not patch any data.

*******************************************************************************/

#ifdef ENABLE_VERIFIER
bool patcher_resolve_class(patchref_t *pr)
{
	unresolved_class *uc;

	/* get stuff from the patcher reference */

	uc = (unresolved_class *) pr->ref;

	/* resolve the class and check subtype constraints */

	if (!resolve_class_eager_no_access_check(uc))
		return false;

	/* patch back original code */

	patcher_patch_code(pr);

	return true;
}
#endif /* ENABLE_VERIFIER */


/* patcher_resolve_native_function *********************************************

   Resolves the native function for a given methodinfo.
   This function patches one data segment word.

*******************************************************************************/

bool patcher_resolve_native_function(patchref_t *pr)
{
	methodinfo  *m;
	uint8_t     *datap;
	functionptr  f;

	/* get stuff from the patcher reference */

	m     = (methodinfo *) pr->ref;
	datap = (uint8_t *)    pr->datap;

	/* resolve native function */

	if (!(f = native_resolve_function(m)))
		return false;

	/* patch native function pointer */

	*((intptr_t *) datap) = (intptr_t) f;

	/* synchronize data cache */

	md_dcacheflush(datap, SIZEOF_VOID_P);

	/* patch back original code */

	patcher_patch_code(pr);

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
 * vim:noexpandtab:sw=4:ts=4:
 */

