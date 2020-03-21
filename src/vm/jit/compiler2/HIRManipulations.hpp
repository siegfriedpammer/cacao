/* src/vm/jit/compiler2/HIRManipulations.hpp - HIRManipulations

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
#ifndef _JIT_COMPILER2_HIRMANIPULATIONS
#define _JIT_COMPILER2_HIRMANIPULATIONS

#define DEBUG_NAME "compiler2/HIRManipulations"

#include "toolbox/logging.hpp"
#include "vm/jit/compiler2/Instructions.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

class HIRManipulations {
private:
    class SplitBasicBlockOperation;
    class CoalesceBasicBlocksOperation;
public:
    /**
     * Splits a basic block at a given instruction. All depending instructions will be moved into the new basic block.
     * Additionally the end instruction of the given basic block will be moved to the new basic block, even if no 
     * dependency exists. The given basic block will not have an EndInst after this method. The instruction itself
     * will still be in the old basic block.
     * @returns the new basic block
     **/
	static BeginInst* split_basic_block(BeginInst* bb, Instruction* split_at);
    /**
     * Moves a given instruction into another method.
     **/
	static void move_instruction_to_method(Instruction* to_move, Method* target_method);
    /**
     * Connects two basic blocks with an GOTOInst.
     **/
	static void connect_with_jump(BeginInst* source, BeginInst* target);
    /**
     * Removes an instruction including all dependency relationships. Additionally the instruction
     * will be delted from the containing method.
     */
	static void remove_instruction(Instruction* to_remove);
    /**
     * Coalesce basic blocks.
     */
	static void coalesce_bbs(Method* M);
	static void replace_value_without_source_states(Value* old_value, Value* new_value);
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#undef DEBUG_NAME

#endif /* _JIT_COMPILER2_HIRMANIPULATIONS */

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
