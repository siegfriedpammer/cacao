/* src/vm/jit/compiler2/lsra/NewSpillPass.hpp - NewSpillPass

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

#ifndef _JIT_COMPILER2_LSRA_NEWSPILLPASS
#define _JIT_COMPILER2_LSRA_NEWSPILLPASS

#include <map>

#include <boost/dynamic_bitset.hpp>

#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MachineLoopPass.hpp"
#include "vm/jit/compiler2/Pass.hpp"

MM_MAKE_NAME(NewSpillPass)

#define LOG_NAMED_PTR_CONTAINER_N(n, prefix, container)                                            \
	do {                                                                                           \
		if (DEBUG_COND_N(n)) {                                                                     \
			LOG_N(n, prefix);                                                                      \
			DEBUG_N(n, print_ptr_container(dbg(), std::begin(container), std::end(container)));    \
			LOG_N(n, nl);                                                                          \
		}                                                                                          \
	} while (0)

#define LOG_NAMED_PTR_CONTAINER(prefix, container) LOG_NAMED_PTR_CONTAINER_N(0, prefix, container)
#define LOG1_NAMED_PTR_CONTAINER(prefix, container) LOG_NAMED_PTR_CONTAINER_N(1, prefix, container)
#define LOG2_NAMED_PTR_CONTAINER(prefix, container) LOG_NAMED_PTR_CONTAINER_N(2, prefix, container)
#define LOG3_NAMED_PTR_CONTAINER(prefix, container) LOG_NAMED_PTR_CONTAINER_N(3, prefix, container)

namespace cacao {
namespace jit {
namespace compiler2 {

using LiveTy = boost::dynamic_bitset<>;
class MachineInstruction;
class MachineOperand;
class Backend;
class RegisterClass;
class StackSlotManager;

using RegisterSet = std::vector<MachineOperand*>;
using RegisterSetUPtrTy = std::unique_ptr<RegisterSet>;

class NewSpillPass;

class SpillInfo {
public:
	explicit SpillInfo(NewSpillPass* pass) : sp(pass) {}

	void add_spill(MachineOperand*, const MIIterator&);
	void add_reload(MachineOperand*, const MIIterator&);

	void insert_instructions(const Backend&, StackSlotManager&) const;

private:
	NewSpillPass* sp;

	struct SpilledOperand {
		explicit SpilledOperand(MachineOperand* op, const MIIterator& position)
		    : operand(op), spill_position(position)
		{
		}

		MachineOperand* operand;
		MachineOperand* stackslot;
		MIIterator spill_position;
		std::vector<MIIterator> reload_positions;
	};
	using SpilledOperandUPtrTy = std::unique_ptr<SpilledOperand>;

	std::map<std::size_t, SpilledOperandUPtrTy> spilled_operands;
};

class Occurrence;

class NewSpillPass : public Pass, public memory::ManagerMixin<NewSpillPass> {
public:
	NewSpillPass() : Pass(), spill_info(this) {}
	virtual bool run(JITData& JD);
	virtual PassUsage& get_PassUsage(PassUsage& PU) const;

private:
	SpillInfo spill_info;
	Backend* backend;
	StackSlotManager* ssm;

	std::map<MachineBasicBlock*, RegisterSetUPtrTy> worksets_entry;
	std::map<MachineBasicBlock*, RegisterSetUPtrTy> worksets_exit;

	std::map<MachineBasicBlock*, RegisterSetUPtrTy> spillsets_entry;
	std::map<MachineBasicBlock*, RegisterSetUPtrTy> spillsets_exit;

	const RegisterClass* current_rc; ///< Registerclass of the current pass run
	unsigned current_rc_idx; ///< Index of RegisterClass, used to access loop pressure

	void process_block(MachineBasicBlock*);
	void fix_block_boundaries();
	void limit(RegisterSet&, RegisterSet&, const MIIterator&, const MIIterator&, unsigned);
	RegisterSetUPtrTy compute_workset(MachineBasicBlock*);
	RegisterSetUPtrTy compute_spillset(MachineBasicBlock*, const RegisterSet&);
	RegisterSetUPtrTy used_in_loop(MachineBasicBlock*);
	RegisterSetUPtrTy used_in_loop(MachineLoop*, LiveTy&);	

	void sort_by_next_use(RegisterSet&, MachineInstruction*) const;

	void reconstruct_ssa(const Occurrence&, const std::vector<Occurrence>&) const;
	const Occurrence& compute_reaching_def(const MIIterator&, const std::vector<Occurrence>&) const;

	friend class SpillInfo;
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_LSRA_NEWSPILLPASS */

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
