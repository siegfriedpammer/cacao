/* src/vm/jit/compiler2/MachineInstructionPrinterPass.cpp - MachineInstructionPrinterPass

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

#include "vm/jit/compiler2/MachineInstructionPrinterPass.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/Method.hpp"
#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/ListSchedulingPass.hpp"
#include "vm/jit/compiler2/BasicBlockSchedulingPass.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/LinearScanAllocatorPass.hpp"
#include "vm/jit/compiler2/CodeGenPass.hpp"

#include "toolbox/GraphPrinter.hpp"

#include "vm/utf8.hpp"
#include "vm/jit/jit.hpp"
#include "vm/class.hpp"

#include <sstream>

#define DEBUG_NAME "compiler2/MachineInstructionPrinterPass"

namespace cacao {
namespace jit {
namespace compiler2 {

PassUsage& MachineInstructionPrinterPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires<MachineInstructionSchedulingPass>();
	PU.add_run_before<LinearScanAllocatorPass>();
	PU.add_run_before<CodeGenPass>();
	return PU;
}
// the address of this variable is used to identify the pass
char MachineInstructionPrinterPass::ID = 0;

// register pass
static PassRegistry<MachineInstructionPrinterPass> X("MachineInstructionPrinterPass");

// run pass
bool MachineInstructionPrinterPass::run(JITData &JD) {
	MachineInstructionSchedule *MIS = get_Pass<MachineInstructionSchedulingPass>();
	assert(MIS);
	for (MachineInstructionSchedule::iterator i = MIS->begin(), e = MIS->end();
			i != e; ++i) {
		MachineBasicBlock *MBB = *i;
		LOG(*MBB << " ");
		DEBUG(print_ptr_container(dbg(),MBB->pred_begin(),MBB->pred_end()) << nl);
		// print label
		LOG(*MBB->front() << nl);
		// print phi
		for (MachineBasicBlock::const_phi_iterator i = MBB->phi_begin(),
				e = MBB->phi_end(); i != e; ++i) {
			MachinePhiInst *phi = *i;
			LOG(*phi << nl);
		}
		// print remaining instructions
		for (MachineBasicBlock::iterator i = ++MBB->begin(),  e = MBB->end();
				i != e ; ++i) {
			MachineInstruction *MI = *i;
			LOG(*MI << nl);
		}
	}
	return true;
}

} // end namespace cacao
} // end namespace jit
} // end namespace compiler2


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
