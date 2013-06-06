/* src/vm/jit/compiler2/ResolveImmediatePass.cpp - ResolveImmediatePass

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

#include "vm/jit/compiler2/ResolveImmediatePass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"

#include "vm/jit/compiler2/ListSchedulingPass.hpp"
#include "vm/jit/compiler2/BasicBlockSchedulingPass.hpp"
#include "vm/jit/compiler2/LoweringPass.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/ResolveImmediate"

namespace cacao {
namespace jit {
namespace compiler2 {

bool ResolveImmediatePass::run(JITData &JD) {
	BasicBlockSchedule *BS = get_Pass<BasicBlockSchedulingPass>();
	InstructionSchedule<Instruction> *IS = get_Pass<ListSchedulingPass>();
	LoweringPass *LP = get_Pass<LoweringPass>();

	// for all basic blocks
	for (BasicBlockSchedule::const_bb_iterator i = BS->bb_begin(),
			e = BS->bb_end(); i != e ; ++i) {
		BeginInst *BI = *i;
		assert(BI);
		// for all instructions in the current basic block
		for (InstructionSchedule<Instruction>::const_inst_iterator i = IS->inst_begin(BI),
				e = IS->inst_end(BI); i != e; ++i) {
			Instruction *I = *i;
			LoweredInstDAG *dag = LP->get_LoweredInstDAG(I);
			assert(dag);
			// for all machine instructions
			bool restart;
			do {
				// TODO this is not so beautiful
				restart = false;
				for (LoweredInstDAG::mi_iterator pos = dag->mi_begin() ,
						e = dag->mi_end(); pos != e ; ++pos) {
					MachineInstruction *MI = *pos;
					for (unsigned i = 0, e = MI->size_op(); i < e ; ++i) {
						MachineOperand *MO = MI->get(i);
						assert(MO);
						Immediate *imm = MO->to_Immediate();
						if (imm && !MI->accepts_immediate(i)) {
							LOG2("MInst (" << MI << ") does not accept immediate as " << i << " parameter" << nl);
							VirtualRegister *dst = new VirtualRegister();
							MachineMoveInst *mov = new MachineMoveInst(dst,imm);
							dag->mi_insert(pos,mov);
							(*MI)[i] = dst;
							// check if we have modified a dag parameter
							// handle dag parameters first!
							for (unsigned dag_it = 0, dag_et = dag->input_size(); dag_it < dag_et ; ++dag_it) {
								LoweredInstDAG::InputParameterTy para = dag->get(dag_it);
								if (para.first == MI && para.second == i) {
									LOG2("We modified a dag parameter -> fix it!" << nl);
									dag->set_input(dag_it,mov, 0);
								}
							}
							// we need to restart this because iterators will be invalid
							restart = true;
							break;
						}
					}
					if (restart) {
						break;
					}
				}
			} while (restart);
		}
	}
	return true;
}

// pass usage
PassUsage& ResolveImmediatePass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires(BasicBlockSchedulingPass::ID);
	PU.add_requires(ListSchedulingPass::ID);
	PU.add_requires(LoweringPass::ID);
	return PU;
}

// the address of this variable is used to identify the pass
char ResolveImmediatePass::ID = 0;

// register pass
static PassRegistery<ResolveImmediatePass> X("ResolveImmediatePass");

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
