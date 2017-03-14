/* src/vm/jit/compiler2/CodeGenPass.hpp - CodeGenPass

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

#ifndef _JIT_COMPILER2_CODEGENPASS
#define _JIT_COMPILER2_CODEGENPASS

#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/CodeMemory.hpp"
#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "toolbox/Option.hpp"

MM_MAKE_NAME(CodeGenPass)

namespace cacao {
namespace jit {
namespace compiler2 {

struct MBBCompare {
	bool operator()(MachineBasicBlock *lhs, MachineBasicBlock *rhs) const {
		return lhs->self_iterator() < rhs->self_iterator();
	}
};

/**
 * CodeGenPass
 * TODO: more info
 */
class CodeGenPass : public Pass, public memory::ManagerMixin<CodeGenPass> {
public:
	typedef alloc::map<MachineBasicBlock*,std::size_t,MBBCompare>::type BasicBlockMap;
#ifdef ENABLE_LOGGING
	static Option<bool> print_code;
	static Option<bool> print_data;
#endif
private:

	typedef std::vector<MachineInstruction*> MInstListTy;
	typedef alloc::map<MachineInstruction*,std::size_t>::type InstructionPositionMap;

	/// Map a MachineInstruction to a offset in the current CodeMemory.
	///
	/// This map stores for each MachineInstruction the offset of the native
	/// code that was written to the CodeSegment by MachineInstruction::emit().
	InstructionPositionMap instruction_positions;

	/// Create the final rplpoint structures for the current method.
	///
	/// Create a rplpoint structure for each MachineReplacementPointInst in
	/// the range @p first to @p last and store them in the codeinfo of @p JD.
	///
	/// @param first Forward iterator that points to the first
	///              MachineReplacementPointInst* in the range.
	/// @param last  Forward iterator that marks the end of the range.
	/// @param JD    The JITData of the currently compiled method.
	template<class ForwardIt>	
	void resolve_replacement_points(ForwardIt first, ForwardIt last, JITData &JD);

	/**
	 * finish code generation
	 *
	 * Finishes the code generation. A new memory, large enough for both
	 * data and code, is allocated and data and code are copied together
	 * to their final layout, unresolved jumps are resolved, ...
	 */
	void finish(JITData &JD);
	BasicBlockMap bbmap;
public:
	static char ID;
	CodeGenPass() : Pass() {}
	virtual bool run(JITData &JD);
	virtual PassUsage& get_PassUsage(PassUsage &PU) const;
	std::size_t get_block_size(MachineBasicBlock *MBB) const;
	BasicBlockMap::const_iterator begin() const { return bbmap.begin(); }
	BasicBlockMap::const_iterator end() const { return bbmap.end(); }
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_CODEGENPASS */


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
