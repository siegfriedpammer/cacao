/* src/vm/jit/compiler2/CodeGenPass.cpp - CodeGenPass

   Copyright (C) 2013
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

#include "vm/jit/compiler2/CodeGenPass.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

bool CodeGenPass::run(JITData &JD) {
	MachineInstructionSchedule *MIS = get_Pass<MachineInstructionSchedulingPass>();
	CodeMemory *CM = &cm;

	// NOTE reverse so we see jump targets (which are not backedges) before
	// the jump.
	for (MachineInstructionSchedule::const_reverse_iterator i = MIS->rbegin(),
			e = MIS->rend() ; i != e ; ++i ) {
		MachineInstruction *MI = *i;
		MI->emit(CM);
	}
	// finish
	finish(JD);
	return true;
}

/**
 * finish code generation
 *
 * Finishes the code generation. A new memory, large enough for both
 * data and code, is allocated and data and code are copied together
 * to their final layout, unresolved jumps are resolved, ...
 */
void CodeGenPass::finish(JITData &JD) {
#if 0
	s4       mcodelen;
#if defined(ENABLE_INTRP)
	s4       ncodelen;
#endif
	s4       alignedmcodelen;
	jumpref *jr;
	u1      *epoint;
	s4       alignedlen;

	/* Get required compiler data. */

	codeinfo*     code = jd->code;
	codegendata*  cd   = jd->cd;
	registerdata* rd   = jd->rd;

	/* prevent compiler warning */

#if defined(ENABLE_INTRP)
	ncodelen = 0;
#endif

	/* calculate the code length */

	mcodelen = (s4) (cd->mcodeptr - cd->mcodebase);

	STATISTICS(count_code_len += mcodelen);
	STATISTICS(count_data_len += cd->dseglen);

	alignedmcodelen = MEMORY_ALIGN(mcodelen, MAX_ALIGN);

#if defined(ENABLE_INTRP)
	if (opt_intrp)
		ncodelen = cd->ncodeptr - cd->ncodebase;
	else {
		ncodelen = 0; /* avoid compiler warning */
	}
#endif

	cd->dseglen = MEMORY_ALIGN(cd->dseglen, MAX_ALIGN);
	alignedlen = alignedmcodelen + cd->dseglen;

#if defined(ENABLE_INTRP)
	if (opt_intrp) {
		alignedlen += ncodelen;
	}
#endif

	/* allocate new memory */

	code->mcodelength = mcodelen + cd->dseglen;
	code->mcode       = CNEW(u1, alignedlen);

	/* set the entrypoint of the method */

	assert(code->entrypoint == NULL);
	code->entrypoint = epoint = (code->mcode + cd->dseglen);

	/* fill the data segment (code->entrypoint must already be set!) */

	dseg_finish(jd);

	/* copy code to the new location */

	MCOPY((void *) code->entrypoint, cd->mcodebase, u1, mcodelen);

#if defined(ENABLE_INTRP)
	/* relocate native dynamic superinstruction code (if any) */

	if (opt_intrp) {
		cd->mcodebase = code->entrypoint;

		if (ncodelen > 0) {
			u1 *ncodebase = code->mcode + cd->dseglen + alignedmcodelen;

			MCOPY((void *) ncodebase, cd->ncodebase, u1, ncodelen);

			/* flush the instruction and data caches */

			md_cacheflush(ncodebase, ncodelen);

			/* set some cd variables for dynamic_super_rerwite */

			cd->ncodebase = ncodebase;

		} else {
			cd->ncodebase = NULL;
		}

		dynamic_super_rewrite(cd);
	}
#endif

	/* Fill runtime information about generated code. */

	code->stackframesize     = cd->stackframesize;
	code->synchronizedoffset = rd->memuse * 8;
	code->savedintcount      = INT_SAV_CNT - rd->savintreguse;
	code->savedfltcount      = FLT_SAV_CNT - rd->savfltreguse;
#if defined(HAS_ADDRESS_REGISTER_FILE)
	code->savedadrcount      = ADR_SAV_CNT - rd->savadrreguse;
#endif

	/* Create the exception table. */

	exceptiontable_create(jd);

	/* Create the linenumber table. */

	code->linenumbertable = new LinenumberTable(jd);

	/* jump table resolving */

	for (jr = cd->jumpreferences; jr != NULL; jr = jr->next)
		*((functionptr *) ((ptrint) epoint + jr->tablepos)) =
			(functionptr) ((ptrint) epoint + (ptrint) jr->target->mpc);

	/* patcher resolving */

	patcher_resolve(jd);

#if defined(ENABLE_REPLACEMENT)
	/* replacement point resolving */
	{
		int i;
		rplpoint *rp;

		rp = code->rplpoints;
		for (i=0; i<code->rplpointcount; ++i, ++rp) {
			rp->pc = (u1*) ((ptrint) epoint + (ptrint) rp->pc);
		}
	}
#endif /* defined(ENABLE_REPLACEMENT) */

	/* Insert method into methodtree to find the entrypoint. */

	methodtree_insert(code->entrypoint, code->entrypoint + mcodelen);

#if defined(__I386__) || defined(__X86_64__) || defined(__XDSPCORE__) || defined(__M68K__) || defined(ENABLE_INTRP)
	/* resolve data segment references */

	dseg_resolve_datareferences(jd);
#endif

	/* flush the instruction and data caches */

	md_cacheflush(code->mcode, code->mcodelength);
#endif
}

// pass usage
PassUsage& CodeGenPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires(MachineInstructionSchedulingPass::ID);
	return PU;
}
// the address of this variable is used to identify the pass
char CodeGenPass::ID = 0;

// registrate Pass
static PassRegistery<CodeGenPass> X("CodeGenPass");

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao


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
