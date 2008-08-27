/* src/vm/jit/linenumbertable.cpp - linenumber handling stuff

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

#include <assert.h>
#include <stdint.h>

#include "mm/memory.h"

#if defined(ENABLE_STATISTICS)
# include "vm/options.h"
# include "vm/statistics.h"
#endif

#include "vm/jit/code.hpp"
#include "vm/jit/codegen-common.hpp"
#include "vm/jit/linenumbertable.hpp"


#if defined(__S390__)
#  define ADDR_MASK(type, x) ((type)((uintptr_t)(x) & 0x7FFFFFFF))
#else
#  define ADDR_MASK(type, x) (x)
#endif

/* linenumbertable_create ******************************************************

   Creates the linenumber table.  We allocate an array and store the
   linenumber entry in reverse-order, so we can search the correct
   linenumber more easily.

*******************************************************************************/

void linenumbertable_create(jitdata *jd)
{
	codeinfo                     *code;
	codegendata                  *cd;
	linenumbertable_t            *lnt;
	linenumbertable_entry_t      *lnte;
	list_t                       *l;
	linenumbertable_list_entry_t *le;
	uint8_t                      *pv;
	void                         *pc;

	/* Get required compiler data. */

	code = jd->code;
	cd   = jd->cd;

	/* Don't allocate a linenumber table if we don't need one. */

	l = cd->linenumbers;

	if (l->size == 0)
		return;

	/* Allocate the linenumber table and the entries array. */

	lnt  = NEW(linenumbertable_t);
	lnte = MNEW(linenumbertable_entry_t, l->size);

#if defined(ENABLE_STATISTICS)
	if (opt_stat) {
		count_linenumbertable++;

		size_linenumbertable +=
			sizeof(linenumbertable_t) +
			sizeof(linenumbertable_entry_t) * l->size;
	}
#endif

	/* Fill the linenumber table. */

	lnt->length  = l->size;
	lnt->entries = lnte;

	/* Fill the linenumber table entries in reverse order, so the
	   search can be forward. */

	/* FIXME I only made this change to prevent a problem when moving
	   to C++. This should be changed back when this file has
	   converted to C++. */

	pv = ADDR_MASK(uint8_t *, code->entrypoint);

	for (le = (linenumbertable_list_entry_t*) list_first(l); le != NULL; le = (linenumbertable_list_entry_t*) list_next(l, le), lnte++) {
		/* If the entry contains an mcode pointer (normal case),
		   resolve it (see doc/inlining_stacktrace.txt for
		   details). */

		if (le->linenumber >= -2)
			pc = pv + le->mpc;
		else
			pc = (void *) le->mpc;

		/* Fill the linenumber table entry. */

		lnte->linenumber = le->linenumber;
		lnte->pc         = pc;
	}

	/* Store the linenumber table in the codeinfo. */

	code->linenumbertable = lnt;
}


/* linenumbertable_list_entry_add **********************************************

   Add a line number reference.

   IN:
      cd.............current codegen data
      linenumber.....number of line that starts with the given mcodeptr

*******************************************************************************/

void linenumbertable_list_entry_add(codegendata *cd, int32_t linenumber)
{
	linenumbertable_list_entry_t *le;

	le = (linenumbertable_list_entry_t*) DumpMemory::allocate(sizeof(linenumbertable_list_entry_t));

	le->linenumber = linenumber;
	le->mpc        = cd->mcodeptr - cd->mcodebase;

	list_add_first(cd->linenumbers, le);
}


/* linenumbertable_list_entry_add_inline_start *********************************

   Add a marker to the line number table indicating the start of an
   inlined method body. (see doc/inlining_stacktrace.txt)

   IN:
      cd ..... current codegen data
      iptr ... the ICMD_INLINE_BODY instruction

*******************************************************************************/

void linenumbertable_list_entry_add_inline_start(codegendata *cd, instruction *iptr)
{
	linenumbertable_list_entry_t *le;
	insinfo_inline               *insinfo;
	uintptr_t                     mpc;

	le = (linenumbertable_list_entry_t*) DumpMemory::allocate(sizeof(linenumbertable_list_entry_t));

	le->linenumber = (-2); /* marks start of inlined method */
	le->mpc        = (mpc = cd->mcodeptr - cd->mcodebase);

	list_add_first(cd->linenumbers, le);

	insinfo = iptr->sx.s23.s3.inlineinfo;

	insinfo->startmpc = mpc; /* store for corresponding INLINE_END */
}


/* linenumbertable_list_entry_add_inline_end ***********************************

   Add a marker to the line number table indicating the end of an
   inlined method body. (see doc/inlining_stacktrace.txt)

   IN:
      cd ..... current codegen data
      iptr ... the ICMD_INLINE_END instruction

   Note:
      iptr->method must point to the inlined callee.

*******************************************************************************/

void linenumbertable_list_entry_add_inline_end(codegendata *cd, instruction *iptr)
{
	linenumbertable_list_entry_t *le;
	insinfo_inline               *insinfo;

	insinfo = iptr->sx.s23.s3.inlineinfo;

	assert(insinfo);

	le = (linenumbertable_list_entry_t*) DumpMemory::allocate(sizeof(linenumbertable_list_entry_t));

	/* special entry containing the methodinfo * */
	le->linenumber = (-3) - iptr->line;
	le->mpc        = (uintptr_t) insinfo->method;

	list_add_first(cd->linenumbers, le);

	le = (linenumbertable_list_entry_t*) DumpMemory::allocate(sizeof(linenumbertable_list_entry_t));

	/* end marker with PC of start of body */
	le->linenumber = (-1);
	le->mpc        = insinfo->startmpc;

	list_add_first(cd->linenumbers, le);
}


/* linenumbertable_linenumber_for_pc_intern ************************************

   This function search the line number table for the line
   corresponding to a given pc. The function recurses for inlined
   methods.

*******************************************************************************/

static s4 linenumbertable_linenumber_for_pc_intern(methodinfo **pm, linenumbertable_entry_t *lnte, int32_t lntsize, void *pc)
{
	linenumbertable_entry_t *lntinline;   /* special entry for inlined method */

	pc = ADDR_MASK(void *, pc);

	for (; lntsize > 0; lntsize--, lnte++) {
		/* Note: In case of inlining this may actually compare the pc
		   against a methodinfo *, yielding a non-sensical
		   result. This is no problem, however, as we ignore such
		   entries in the switch below. This way we optimize for the
		   common case (ie. a real pc in lntentry->pc). */

		if (pc >= lnte->pc) {
			/* did we reach the current line? */

			if (lnte->linenumber >= 0)
				return lnte->linenumber;

			/* we found a special inline entry (see
			   doc/inlining_stacktrace.txt for details */

			switch (lnte->linenumber) {
			case -1: 
				/* begin of inlined method (ie. INLINE_END
				   instruction) */

				lntinline = --lnte;            /* get entry with methodinfo * */
				lnte--;                        /* skip the special entry      */
				lntsize -= 2;

				/* search inside the inlined method */

				if (linenumbertable_linenumber_for_pc_intern(pm, lnte, lntsize, pc)) {
					/* the inlined method contained the pc */

					*pm = (methodinfo *) lntinline->pc;

					assert(lntinline->linenumber <= -3);

					return (-3) - lntinline->linenumber;
				}

				/* pc was not in inlined method, continue search.
				   Entries inside the inlined method will be skipped
				   because their lntentry->pc is higher than pc.  */
				break;

			case -2: 
				/* end of inlined method */

				return 0;

				/* default: is only reached for an -3-line entry after
				   a skipped -2 entry. We can safely ignore it and
				   continue searching.  */
			}
		}
	}

	/* PC not found. */

	return 0;
}


/* linenumbertable_linenumber_for_pc *******************************************

   A wrapper for linenumbertable_linenumber_for_pc_intern.

   NOTE: We have a intern version because the function is called
   recursively for inlined methods.

*******************************************************************************/

int32_t linenumbertable_linenumber_for_pc(methodinfo **pm, codeinfo *code, void *pc)
{
	linenumbertable_t *lnt;
	int32_t            linenumber;

	/* Get line number table. */

	lnt = code->linenumbertable;

	if (lnt == NULL)
		return 0;

	/* Get the line number. */

	linenumber = linenumbertable_linenumber_for_pc_intern(pm, lnt->entries, lnt->length, pc);

	return linenumber;
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
