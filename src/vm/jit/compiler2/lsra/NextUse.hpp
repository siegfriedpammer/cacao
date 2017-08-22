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

namespace cacao {
namespace jit {
namespace compiler2 {

class MachineBasicBlock;
class MachineInstruction;
class SpillPass;

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
	NextUseTime get_next_use(MachineBasicBlock* from, MachineInstruction* def);
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_LSRA_NEXTUSE */
