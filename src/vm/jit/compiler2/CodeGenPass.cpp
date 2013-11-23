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
#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/RegisterAllocatorPass.hpp"

#include "toolbox/logging.hpp"

#include "mm/codememory.hpp"
#include "vm/types.hpp"
#include "vm/jit/jit.hpp"
#include "vm/jit/methodtree.hpp"

#include "vm/jit/disass.hpp"

#include "mm/memory.hpp"

#include "md.hpp"

#define DEBUG_NAME "compiler2/CodeGen"

STAT_DECLARE_VAR(std::size_t, compiler_last_codesize, 0)
STAT_DECLARE_VAR(std::size_t, num_remaining_moves,0)

namespace cacao {
namespace jit {
namespace compiler2 {

bool CodeGenPass::run(JITData &JD) {
	MachineInstructionSchedule *MIS = get_Pass<MachineInstructionSchedulingPass>();
	CodeMemory *CM = JD.get_CodeMemory();
	CodeSegment &CS = CM->get_CodeSegment();

	// NOTE reverse so we see jump targets (which are not backedges) before
	// the jump.
	for (MachineInstructionSchedule::const_reverse_iterator i = MIS->rbegin(),
			e = MIS->rend() ; i != e ; ++i ) {
		MachineBasicBlock *MBB = *i;
		for (MachineBasicBlock::const_reverse_iterator i = MBB->rbegin(),
				e = MBB->rend(); i != e ; ++i) {
			MachineInstruction *MI = *i;
			std::size_t start = CS.size();
			LOG2("MInst: " << MI << " emitted instruction:" << nl);
			MI->emit(CM);
			std::size_t end = CS.size();
			#if defined(ENABLE_STATISTICS)
			if (MI->is_move() && start != end) {
				STATISTICS(++num_remaining_moves);
			}
			#endif
			if (DEBUG_COND_N(2)) {
				if ( start == end) {
					LOG2("none" << nl);
				} else {
					alloc::vector<u1>::type tmp;
					while(start != end--) {
						tmp.push_back(CS.at(end));
					}
	#if defined(ENABLE_DISASSEMBLER)
					disassemble(&tmp.front(), &tmp.front() + tmp.size());
	#else
					// TODO print hex code
	#endif
				}
			}
		}
	}
	// create stack frame
	JD.get_Backend()->create_frame(CM,JD.get_StackSlotManager());
	// link code memory
	CM->link();
	// finish
	finish(JD);
	return true;
}

void CodeGenPass::finish(JITData &JD) {
	CodeMemory *CM = JD.get_CodeMemory();
	CodeSegment &CS = CM->get_CodeSegment();
	DataSegment &DS = CM->get_DataSegment();
#if 0
	s4       alignedmcodelen;
	jumpref *jr;
	s4       alignedlen;
#endif
	u1      *epoint;

	/* Get required compiler data. */

	codeinfo*     code = JD.get_jitdata()->code;
#if 0
	codegendata*  cd   = jd->cd;
	registerdata* rd   = jd->rd;
#endif

	/* prevent compiler warning */


	/* calculate the code length */

	#if 0
	s4 mcodelen = (s4) (JD.get_CodeMemory()->get_end() - JD.get_CodeMemory()->get_start());
	s4 alignedmcodelen = MEMORY_ALIGN(mcodelen, MAX_ALIGN);
	s4 dseglen = (s4) JD.get_CodeMemory()->data_size();
	s4 aligneddseglen = MEMORY_ALIGN(dseglen, MAX_ALIGN);


	s4 alignedlen = alignedmcodelen + aligneddseglen;
	#endif

#if 0
	STATISTICS(count_code_len += mcodelen);
	STATISTICS(count_data_len += cd->dseglen);
#endif

	/* allocate new memory */

	// Note that the DataSegment and the CodeSegment shall be aligned!
	code->mcodelength = DS.size() + CS.size();
	code->mcode       = CNEW(u1, code->mcodelength);

	/* set the entrypoint of the method */

	assert(code->entrypoint == NULL);
	code->entrypoint = epoint = (code->mcode + DS.size());

	/* fill the data segment (code->entrypoint must already be set!) */
	MCOPY((void *) code->mcode, DS.get_start(), u1, DS.size());

	#if 0
	size_t offset = 1;
	/// @Cpp11 Could use vector::data()
	for (CodeMemory::const_data_iterator i = JD.get_CodeMemory()->data_begin(),
			e = JD.get_CodeMemory()->data_end() ; i != e; ++i) {
		u1* ptr = epoint - offset++;
		*ptr = *i;
		LOG3("" << ptr
		  << ": " << hex << *i << " " << *ptr << dec << nl);
		assert(ptr >= code->mcode);
	}
	#endif

	LOG2("mcode: " << code->mcode << nl);
	LOG2("entry: " << code->entrypoint << nl);

	if (DEBUG_COND_N(2)) {
		for(u8 *ptr = reinterpret_cast<u8*>(code->mcode),
				*e = reinterpret_cast<u8*>(code->entrypoint); ptr < e; ++ptr) {
			LOG2("" << setz(16) << hex << ptr
			  << ": " << setz(16) << (u8)*ptr << dec << nl);
		}
	}

	STATISTICS(compiler_last_codesize = code->mcodelength);

#if 0
	STATISTICS(count_code_len += mcodelen);
	STATISTICS(count_data_len += cd->dseglen);
#endif


#if 0
	dseg_finish(jd);
#endif

	/* copy code to the new location */

	CS.reverse();
	MCOPY((void *) code->entrypoint, CS.get_start(), u1, CS.size());

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

	/* jump table resolving */
	for (jr = cd->jumpreferences; jr != NULL; jr = jr->next)
		*((functionptr *) ((ptrint) epoint + jr->tablepos)) =
			(functionptr) ((ptrint) epoint + (ptrint) jr->target->mpc);

#endif
	/* patcher resolving */

	patcher_resolve(code);
	LOG2("Patchers:" << nl);
	#if !defined(NDEBUG)
	DEBUG2(patcher_list_show(code));
	#endif
#if 0
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

	methodtree_insert(code->entrypoint, code->entrypoint + CS.size());

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
	PU.add_requires<MachineInstructionSchedulingPass>();
	PU.add_requires<RegisterAllocatorPass>();
	return PU;
}
// the address of this variable is used to identify the pass
char CodeGenPass::ID = 0;

// registrate Pass
static PassRegistry<CodeGenPass> X("CodeGenPass");

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
