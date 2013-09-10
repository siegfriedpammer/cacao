/* src/vm/jit/compiler2/MachineInstructionSchedulingPass.cpp - MachineInstructionSchedulingPass

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

#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"

#include "vm/jit/compiler2/ListSchedulingPass.hpp"
#include "vm/jit/compiler2/BasicBlockSchedulingPass.hpp"
#include "vm/jit/compiler2/LoweringPass.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/machineinstructionscheduling"

namespace cacao {
namespace jit {
namespace compiler2 {

void MachineInstructionSchedulingPass::initialize() {
#if 0
	list.clear();
	map.clear();
#endif
}

bool MachineInstructionSchedulingPass::run(JITData &JD) {
#if 0
	BasicBlockSchedule *BS = get_Pass<BasicBlockSchedulingPass>();
	InstructionSchedule<Instruction> *IS = get_Pass<ListSchedulingPass>();
	LoweringPass *LP = get_Pass<LoweringPass>();

	// for all basic blocks
	for (BasicBlockSchedule::const_bb_iterator i = BS->bb_begin(),
			e = BS->bb_end(); i != e ; ++i) {
		BeginInst *BI = *i;
		assert(BI);
		unsigned begin = list.size();
		// for all instructions in the current basic block
		for (InstructionSchedule<Instruction>::const_inst_iterator i = IS->inst_begin(BI),
				e = IS->inst_end(BI); i != e; ++i) {
			Instruction *I = *i;
			const LoweredInstDAG *dag = LP->get_LoweredInstDAG(I);
			assert(dag);
			for (LoweredInstDAG::const_mi_iterator i = dag->mi_begin() ,
					e = dag->mi_end(); i != e ; ++i) {
				MachineInstruction *MI = *i;
				list.push_back(MI);
			}
		}
		unsigned end = list.size();
		map[BI] = std::make_pair(begin,end);
	}
	if (DEBUG_COND) {
		for (BasicBlockSchedule::const_bb_iterator i = BS->bb_begin(),
				e = BS->bb_end(); i != e ; ++i) {
			BeginInst *BI = *i;
			assert(BI);
			LOG("BasicBlock: " << BI << nl);
			MachineInstructionRangeTy range = get_range(BI);
			assert(range.first != range.second);
			for (unsigned i = range.first, e = range.second; i < e; ++i) {
				MachineInstruction *MI = get(i);
				assert(MI);
				LOG("  " << setw(4) << i << ": " << MI << nl);
			}
		}
	}
#endif
	return true;
}

// pass usage
PassUsage& MachineInstructionSchedulingPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires(BasicBlockSchedulingPass::ID);
	PU.add_requires(ListSchedulingPass::ID);
	PU.add_requires(LoweringPass::ID);
	return PU;
}

// the address of this variable is used to identify the pass
char MachineInstructionSchedulingPass::ID = 0;

// register pass
static PassRegistery<MachineInstructionSchedulingPass> X("MachineInstructionSchedulingPass");

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
