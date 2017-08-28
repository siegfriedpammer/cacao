/* src/vm/jit/compiler2/lsra/NextUse.hpp - NextUse

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

#ifndef _JIT_COMPILER2_LSRA_NEXTUSE
#define _JIT_COMPILER2_LSRA_NEXTUSE

#include <map>

#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MachineOperand.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

class MachineInstruction;
class SpillPass;

#define USES_INFINITY 10000000
#define USES_PENDING   9999999
#define UNKNOWN_OUTERMOST_LOOP ((unsigned)-1)

inline bool uses_is_infinite(unsigned time)
{
	return time >= USES_INFINITY;
}

inline bool uses_is_pending(unsigned time)
{
	return time == USES_PENDING;
}

struct BlockOperand {
	MachineBasicBlock* block;
	MachineOperand* operand;

	bool operator<(const BlockOperand& right) const {
		if (block->get_id() == right.block->get_id()) {
			return operand->get_id() < right.operand->get_id();
		}
		return block->get_id() < right.block->get_id();
	}
};

struct UseTime {
	MachineBasicBlock* block;
	MachineOperand* operand;
	unsigned outermost_loop;
	unsigned next_use;	
	unsigned visited;
};

struct NextUseTime {
	unsigned time;
	unsigned outermost_loop;
	MachineInstruction* before;
};

/**
 * Used by the SpillPass to get global next use values.
 * Will be calculated as needed based from the data provided
 * by the liveness analysis
 */
class NextUse {
public:
	void initialize(SpillPass* sp);
	NextUseTime get_next_use(MachineBasicBlock* from, MachineOperand* def);

private:
	SpillPass* SP;
	unsigned visited_counter = 0;

	using NextUseMap = std::map<BlockOperand, UseTime>;
	NextUseMap next_use_map;

	UseTime* get_or_set_use_block(MachineBasicBlock* block, MachineOperand* operand);
	NextUseTime get_next_use_(MachineBasicBlock* from, MachineOperand* def);
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_LSRA_NEXTUSE */

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
