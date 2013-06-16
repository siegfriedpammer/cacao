/* src/vm/jit/compiler2/X86_64EmitHelper.hpp - X86_64EmitHelper

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

#ifndef _JIT_COMPILER2_X86_64EMITHELPER
#define _JIT_COMPILER2_X86_64EMITHELPER

#include "vm/jit/compiler2/x86_64/X86_64Register.hpp"
#include "vm/types.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

inline u1 get_rex(X86_64Register *reg, X86_64Register *rm) {
	const unsigned rex_w = 3;
	const unsigned rex_r = 2;
	//const unsigned rex_x = 1;
	const unsigned rex_b = 0;

	u1 rex = 0x40;
	// 64-bit operand size
	rex |= (1 << rex_w);

	if (reg->extented_gpr) {
		rex |= (1 << rex_r);
	}
	if (rm->extented_gpr) {
		rex |= (1 << rex_b);
	}
	return rex;
}

inline u1 get_modrm(X86_64Register *reg, X86_64Register *rm) {
	const unsigned modrm_mod = 6;
	const unsigned modrm_reg = 3;
	const unsigned modrm_rm = 0;

	u1 modrm = 0x3 << modrm_mod;
	modrm |= reg->get_index() << modrm_reg;
	modrm |= rm->get_index() << modrm_rm;

	return modrm;
}

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_X86_64EMITHELPER */


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
