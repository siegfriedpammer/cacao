/* src/vm/jit/compiler2/lsra/SpillPass.hpp - SpillPass

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

#ifndef _JIT_COMPILER2_LSRA_SPILLPASS
#define _JIT_COMPILER2_LSRA_SPILLPASS

#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MachineInstruction.hpp"
#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/lsra/LoopPressure.hpp"
#include "vm/jit/compiler2/lsra/NextUse.hpp"

MM_MAKE_NAME(SpillPass)

namespace cacao {
namespace jit {
namespace compiler2 {

enum class Available { Everywhere, Nowhere, Partly, Unknown };

struct Location {
	MachineOperand* operand;
	unsigned time;
	bool spilled;

	bool operator<(const Location& other) {
		return time < other.time;
	}
};

using Workset = std::vector<Location>;

using WorksetUPtrTy = std::unique_ptr<Workset>;

struct BlockInfo {
	WorksetUPtrTy start_workset;
	WorksetUPtrTy end_workset;
};

using BlockInfosTy = alloc::map<MachineBasicBlock*, BlockInfo>::type;

class SpillPass : public Pass, public memory::ManagerMixin<SpillPass> {
public:
	SpillPass() : Pass() {}
	virtual bool run(JITData& JD);
	virtual PassUsage& get_PassUsage(PassUsage& PU) const;

private:
	NextUse next_use;
	BlockInfosTy block_infos;
	LoopPressureAnalysis loop_pressure;

	WorksetUPtrTy decide_start_workset(MachineBasicBlock* block);

	using BlockOrder = alloc::list<MachineBasicBlock*>::type;
	BlockOrder reverse_postorder;

	void build_reverse_postorder(MachineBasicBlock* block);

	Available
	available_in_all_preds(MachineBasicBlock* block,
	                       const std::map<MachineBasicBlock*, const Workset*>& pred_worksets,
	                       MachineInstruction* instruction,
	                       MachineOperand* operand,
	                       bool is_phi);

	Location to_take_or_not_to_take(MachineBasicBlock* block,
	                                MachineOperand* operand,
	                                bool is_phi_result,
	                                Available available);

	void displace(Workset& workset,
	              const Workset& new_vals,
	              bool const is_usage,
	              MachineInstruction* instruction);
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_LSRA_SPILLPASS */

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
