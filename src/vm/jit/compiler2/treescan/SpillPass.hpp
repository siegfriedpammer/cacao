/* src/vm/jit/compiler2/treescan/SpillPass.hpp - SpillPass

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

#ifndef _JIT_COMPILER2_LSRA_SpillPass
#define _JIT_COMPILER2_LSRA_SpillPass

#include <map>

#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MachineLoopPass.hpp"
#include "vm/jit/compiler2/MachineOperandSet.hpp"
#include "vm/jit/compiler2/Pass.hpp"

MM_MAKE_NAME(SpillPass)

namespace cacao {
namespace jit {
namespace compiler2 {

class MachineInstruction;
class MachineOperand;
class MachineOperandFactory;
class Backend;
class RegisterClass;
class StackSlotManager;

class SpillPass;

class SpillInfo {
public:
	explicit SpillInfo(SpillPass* pass) : sp(pass) {}

	void add_spill(MachineOperand*, const MIIterator&);
	void add_reload(MachineOperand*, const MIIterator&);
	void add_spill_phi(MachinePhiInst*);

	void insert_instructions(const Backend&, StackSlotManager&) const;

private:
	SpillPass* sp;

	struct SpilledOperand {
		SpilledOperand(MachineOperand* op, const MIIterator& position)
		    : operand(op), spill_positions{position}
		{
		}

		SpilledOperand(MachineOperand* op) : operand(op) {}

		MachineOperand* operand;
		MachineOperand* stackslot = nullptr;
		std::vector<MIIterator> spill_positions;
		std::vector<MIIterator> reload_positions;

		// When spilling whole PHIs, we set this flag, the spilled
		// arguments are then added to spilled_operands
		bool spilled_phi = false;
		MachinePhiInst* phi_instruction = nullptr;
	};
	using SpilledOperandUPtrTy = std::unique_ptr<SpilledOperand>;

	std::map<std::size_t, SpilledOperandUPtrTy> spilled_operands;
	SpilledOperand& get_spilled_operand(MachineOperand*);

	void replace_registers_with_stackslots_for_deopt() const;

	friend class SSAReconstructor;
};

class SpillPass : public Pass, public memory::ManagerMixin<SpillPass> {
public:
	SpillPass() : Pass(), spill_info(this) {}
	virtual bool run(JITData& JD);
	virtual PassUsage& get_PassUsage(PassUsage& PU) const;

private:
	SpillInfo spill_info;
	Backend* backend;
	StackSlotManager* ssm;
	MachineOperandFactory* mof;

	std::map<MachineBasicBlock*, OperandSet> worksets_entry;
	std::map<MachineBasicBlock*, OperandSet> worksets_exit;

	std::map<MachineBasicBlock*, OperandSet> spillsets_entry;
	std::map<MachineBasicBlock*, OperandSet> spillsets_exit;

	const RegisterClass* current_rc; ///< Registerclass of the current pass run
	unsigned current_rc_idx;         ///< Index of RegisterClass, used to access loop pressure

	void process_block(MachineBasicBlock*);
	void fix_block_boundaries();
	void limit(OperandSet&, OperandSet&, const MIIterator&, MIIterator, unsigned);
	OperandSet compute_workset(MachineBasicBlock*);
	OperandSet compute_spillset(MachineBasicBlock*, const OperandSet&);
	OperandSet used_in_loop(MachineBasicBlock*);
	OperandSet used_in_loop(MachineLoop*, OperandSet&);

	void sort_by_next_use(OperandList&, MachineInstruction*) const;

	enum class Availability { Unknown, Everywhere, Partly, Nowhere };
	Availability available_in_all_preds(MachineBasicBlock*, MachineOperand*, MachinePhiInst*);

	struct Location {
		enum class TimeType { Normal, Infinity, Pending };

		TimeType time_type;
		unsigned time;
		MachineOperand* operand;
		bool spilled;
	};
	Location to_take_or_not_to_take(MachineBasicBlock*, MachineOperand*, Availability);

	bool strictly_dominates(const MIIterator&, const MIIterator&);

	friend class SpillInfo;
	friend class SSAReconstructor;
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_LSRA_SpillPass */

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
