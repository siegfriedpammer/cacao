/* src/vm/jit/compiler2/MachineDominatorPass.cpp - MachineDominatorPass

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

#include "vm/jit/compiler2/MachineDominatorPass.hpp"

#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

template <>
MachineDominatorPass::NodeTy* MachineDominatorPass::get_init_node(JITData& JD)
{
	auto MIS = get_Pass<MachineInstructionSchedulingPass>();
	return MIS->front();
}

template<>
std::size_t MachineDominatorPass::get_nodes_size(JITData &JD)
{
	auto MIS = get_Pass<MachineInstructionSchedulingPass>();
	return MIS->size();
}

template<>
auto MachineDominatorPass::node_begin(JITData &JD)
{
	auto MIS = get_Pass<MachineInstructionSchedulingPass>();
	return MIS->begin();
}

template<>
auto MachineDominatorPass::node_end(JITData &JD)
{
	auto MIS = get_Pass<MachineInstructionSchedulingPass>();
	return MIS->end();
}

template<>
MachineInstruction::successor_iterator MachineDominatorPass::succ_begin(MachineDominatorPass::NodeTy* block)
{
	return block->back()->successor_begin();
}

template<>
MachineInstruction::successor_iterator MachineDominatorPass::succ_end(MachineDominatorPass::NodeTy* block)
{
	return block->back()->successor_end();
}

template<>
template<>
MachineDominatorPass::NodeTy* MachineDominatorPass::get(MachineBasicBlock* block)
{
	return block;
}

// pass usage
template <>
PassUsage& MachineDominatorPass::get_PassUsage(PassUsage& PU) const
{
	PU.add_requires<MachineInstructionSchedulingPass>();
	return PU;
}

// register pass
static PassRegistry<MachineDominatorPass> X("MachineDominatorPass");

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
