/* src/vm/jit/compiler2/SSADeconstructionPass.hpp - SSADeconstructionPass

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

#ifndef _JIT_COMPILER2_SSADECONSTRUCTIONPASS
#define _JIT_COMPILER2_SSADECONSTRUCTIONPASS

#include <list>
#include <set>

#include "vm/jit/compiler2/Pass.hpp"

MM_MAKE_NAME(SSADeconstructionPass)

namespace cacao {
namespace jit {
namespace compiler2 {

class MachineBasicBlock;
class MachineInstruction;
class MachineOperand;

class ParallelCopyImpl {
public:
	ParallelCopyImpl(JITData& JD) : JD(JD) {}

	void add(MachineOperand* src_op, MachineOperand* dst_op) {
		src.push_back(src_op);
		dst.push_back(dst_op);
	}

	void calculate();

	std::vector<MachineInstruction*>& get_operations() {
		return operations;
	}

private:
	JITData& JD;

	std::vector<MachineOperand*> src;
	std::vector<MachineOperand*> dst;

	std::vector<MachineInstruction*> operations;

	void find_safe_operations(std::set<unsigned>&);
	void implement_safe_operations(std::set<unsigned>&);
	void implement_swaps();
	bool swap_registers();
};

class SSADeconstructionPass : public Pass, public Artifact, public memory::ManagerMixin<SSADeconstructionPass> {
public:
	SSADeconstructionPass() : Pass() {}
	virtual bool run(JITData& JD);
	virtual PassUsage& get_PassUsage(PassUsage& PU) const;

private:
	JITData* JD;

	void process_block(MachineBasicBlock*);
	void insert_transfer_at_end(MachineBasicBlock*, MachineBasicBlock*, unsigned);
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_SSADECONSTRUCTIONPASS */

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
