/* src/vm/jit/compiler2/X86_64Instructions.hpp - X86_64Instructions

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

#include "vm/jit/compiler2/x86_64/X86_64Instructions.hpp"
#include "vm/jit/compiler2/x86_64/X86_64EmitHelper.hpp"
#include "vm/jit/compiler2/MachineInstructions.hpp"
#include "vm/jit/compiler2/CodeMemory.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/x86_64"

namespace cacao {
namespace jit {
namespace compiler2 {

void X86_64RetInst::emit(CodeMemory* CM) const {
	CodeFragment code = CM->get_CodeFragment(1);
	code[0] = 0xc3;
}

void X86_64MovInst::emit(CodeMemory* CM) const {
	X86_64Register *src_reg = operands[0].op->to_Register()
		->to_MachineRegister()->to_NaviveRegister();
	X86_64Register *dst_reg = result.op->to_Register()
		->to_MachineRegister()->to_NaviveRegister();

	CodeFragment code = CM->get_CodeFragment(3);
	code[0] = get_rex(src_reg,dst_reg);
	code[1] = 0x89; // MOV
	code[2] = get_modrm(src_reg,dst_reg);
}

void X86_64IMulInst::emit(CodeMemory* CM) const {
	X86_64Register *src_reg = operands[0].op->to_Register()->to_MachineRegister()->to_NaviveRegister();
	X86_64Register *dst_reg = result.op->to_Register()->to_MachineRegister()->to_NaviveRegister();

	CodeFragment code = CM->get_CodeFragment(4);
	code[0] = get_rex(src_reg,dst_reg);
	code[1] = 0x0f; // IMUL1
	code[2] = 0xaf; // IMUL2
	code[3] = get_modrm(src_reg,dst_reg);
}


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
