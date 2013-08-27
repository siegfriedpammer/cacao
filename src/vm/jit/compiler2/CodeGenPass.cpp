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

#include "toolbox/logging.hpp"

#include "mm/codememory.hpp"
#include "vm/types.hpp"
#include "vm/jit/jit.hpp"
#include "vm/jit/methodtree.hpp"

#include "vm/jit/disass.hpp"

#include "mm/memory.hpp"

#include "md.hpp"

#define DEBUG_NAME "compiler2/CodeGen"

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
		u1* start = CM->get_start();
		MI->emit(CM);
		LOG2("MInst: " << MI << " emitted instruction:" << nl);
		if (DEBUG_COND_N(2)) {
			u1* end = CM->get_start();
			if ( start == end) {
				LOG2("none" << nl);
			} else {
				disassemble(CM->get_start(),start);
			}
		}
	}
	// create stack frame
	JD.get_Backend()->create_frame(CM,JD.get_StackSlotManager());
	// resolve jumps
	CM->resolve();
	// finish
	finish(JD);
	return true;
}

void CodeGenPass::finish(JITData &JD) {
	s4       mcodelen;
	s4       alignedmcodelen;
#if 0
	jumpref *jr;
#endif
	u1      *epoint;
	s4       alignedlen;

	/* Get required compiler data. */

	codeinfo*     code = JD.get_jitdata()->code;
#if 0
	codegendata*  cd   = jd->cd;
	registerdata* rd   = jd->rd;
#endif

	/* prevent compiler warning */


	/* calculate the code length */

	mcodelen = (s4) (cm.get_end() - cm.get_start());

#if 0
	STATISTICS(count_code_len += mcodelen);
	STATISTICS(count_data_len += cd->dseglen);
#endif

	alignedmcodelen = MEMORY_ALIGN(mcodelen, MAX_ALIGN);

#if 0
	cd->dseglen = MEMORY_ALIGN(cd->dseglen, MAX_ALIGN);
	alignedlen = alignedmcodelen + cd->dseglen;
#endif


	/* allocate new memory */

	code->mcodelength = mcodelen;
	code->mcode       = CNEW(u1, alignedmcodelen);

	/* set the entrypoint of the method */

	assert(code->entrypoint == NULL);
	code->entrypoint = epoint = (code->mcode);

	/* fill the data segment (code->entrypoint must already be set!) */

#if 0
	dseg_finish(jd);
#endif

	/* copy code to the new location */

	MCOPY((void *) code->entrypoint, cm.get_start(), u1, mcodelen);

	/* Fill runtime information about generated code. */

#if 0
	code->stackframesize     = cd->stackframesize;
	code->synchronizedoffset = rd->memuse * 8;
	code->savedintcount      = INT_SAV_CNT - rd->savintreguse;
	code->savedfltcount      = FLT_SAV_CNT - rd->savfltreguse;
#else
	code->stackframesize     = 0;
	code->synchronizedoffset = 0;
	code->savedintcount      = 0;
	code->savedfltcount      = 0;
#endif
#if defined(HAS_ADDRESS_REGISTER_FILE)
	code->savedadrcount      = ADR_SAV_CNT - rd->savadrreguse;
#endif

	/* Create the exception table. */

#if 0
	exceptiontable_create(jd);
#endif

	/* Create the linenumber table. */

#if 0
	code->linenumbertable = new LinenumberTable(jd);
#endif

	/* jump table resolving */
#if 0
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

#endif
	/* Insert method into methodtree to find the entrypoint. */

	methodtree_insert(code->entrypoint, code->entrypoint + mcodelen);

#if defined(__I386__) || defined(__X86_64__) || defined(__XDSPCORE__) || defined(__M68K__) || defined(ENABLE_INTRP)
	/* resolve data segment references */

#if 0
	dseg_resolve_datareferences(jd);
#endif
#endif

	/* flush the instruction and data caches */

	md_cacheflush(code->mcode, code->mcodelength);
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
