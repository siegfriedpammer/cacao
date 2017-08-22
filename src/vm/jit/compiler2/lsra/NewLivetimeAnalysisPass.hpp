/* src/vm/jit/compiler2/ExamplePass.hpp - ExamplePass

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

#ifndef _JIT_COMPILER2_EXAMPLEPASS
#define _JIT_COMPILER2_EXAMPLEPASS

#include <boost/dynamic_bitset.hpp>

#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MachineLoopPass.hpp"
#include "vm/jit/compiler2/Pass.hpp"

MM_MAKE_NAME(NewLivetimeAnalysisPass)

namespace cacao {
namespace jit {
namespace compiler2 {

using LiveTy = boost::dynamic_bitset<>;
using LiveMapTy = alloc::map<MachineBasicBlock*, LiveTy>::type;

class NewLivetimeAnalysisPass : public Pass, public memory::ManagerMixin<NewLivetimeAnalysisPass> {
public:
	NewLivetimeAnalysisPass() : Pass() {}
	virtual bool run(JITData& JD);
	virtual PassUsage& get_PassUsage(PassUsage& PU) const;

private:
	std::size_t num_operands;
	LiveMapTy liveInMap;
	LiveMapTy liveOutMap;

	void dag_dfs(MachineBasicBlock* block);
	void looptree_dfs(MachineBasicBlock* block);
	boost::dynamic_bitset<> phi_uses(MachineBasicBlock* block);
	boost::dynamic_bitset<> phi_defs(MachineBasicBlock* block);

	LiveTy& get_livety(LiveMapTy& liveMap, MachineBasicBlock* block)
	{
		auto it = liveMap.find(block);
		if (it == liveMap.end()) {
			LiveTy live(num_operands, 0ul);
			it = liveMap.insert(std::make_pair(block, live)).first;
		}
		return it->second;
	}
	LiveTy& get_live_in(MachineBasicBlock* block) { return get_livety(liveInMap, block); }
	LiveTy& get_live_out(MachineBasicBlock* block) { return get_livety(liveOutMap, block); }

// To get better debug information, we keep a map that maps operant IDs, to
// operands
#if !defined(NDEBUG)
	using OperandMap = alloc::map<std::size_t, MachineOperand*>::type;
	OperandMap operandMap;

	void log_livety(OStream& OS, const LiveTy& liveSet);
#endif
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_EXAMPLEPASS */

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
