/* src/vm/jit/compiler2/PhiCoalescingPass.hpp - PhiCoalescingPass

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

#ifndef _JIT_COMPILER2_PHICOALESCINGPASS
#define _JIT_COMPILER2_PHICOALESCINGPASS

#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/MachineOperandSet.hpp"

MM_MAKE_NAME(PhiCoalescingPass)

namespace cacao {
namespace jit {
namespace compiler2 {

class Backend;
class MachineBasicBlock;
class MachinePhiInst;
class MachineOperand;

class PhiCoalescingPass : public Pass, public Artifact, public memory::ManagerMixin<PhiCoalescingPass> {
public:
	static Option<bool> enabled;
	PhiCoalescingPass() : Pass() {}
	bool run(JITData& JD) override;
	PassUsage& get_PassUsage(PassUsage& PU) const override;
	bool is_enabled() const override { return PhiCoalescingPass::enabled; }

	PhiCoalescingPass* provide_Artifact(ArtifactInfo::IDTy) override {
		return this;
	}

	struct EquivalenceClass {
		std::vector<MachineOperand*> operands;
		MachineOperand* last_assigned_color;
	};
	/// Returns the equivalence class the given operand belongs to
	/// or nullptr if it not part of any
	/// This is used by the RegisterAssignmentPass
	EquivalenceClass* get_equivalence_class_for(MachineOperand*);

private:
	Backend* backend;

	void handle_phi(MachinePhiInst* instruction);
	
	std::unordered_map<std::size_t, unsigned> preorder_nrs;
	std::unordered_map<std::size_t, unsigned> max_preorder_nrs;

	void dom_dfs(MachineBasicBlock*, unsigned&);
	bool dominates(MachineBasicBlock*, MachineBasicBlock*);

	unsigned preorder(MachineOperand*);
	unsigned max_preorder(MachineOperand*);

	std::vector<EquivalenceClass> equivalence_classes;
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_PHICOALESCINGPASS */

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
