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
#include "vm/jit/compiler2/CodeMemory.hpp"
#include "vm/types.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace x86_64 {

inline u1 get_rex(X86_64Register *reg, X86_64Register *rm = NULL,
		bool opsiz64 = true) {
	const unsigned rex_w = 3;
	const unsigned rex_r = 2;
	//const unsigned rex_x = 1;
	const unsigned rex_b = 0;

	u1 rex = 0x40;

	// 64-bit operand size
	if (opsiz64) {
		rex |= (1 << rex_w);
	}

	if (reg->extented) {
		rex |= (1 << rex_r);
	}
	if (rm && rm->extented) {
		rex |= (1 << rex_b);
	}
	return rex;
}

inline u1 get_modrm_u1(u1 mod, u1 reg, u1 rm) {
	const unsigned modrm_mod = 6;
	const unsigned modrm_reg = 3;
	const unsigned modrm_rm = 0;

	u1 modrm = (0x3 & mod) << modrm_mod;
	modrm |= (0x7 & reg) << modrm_reg;
	modrm |= (0x7 & rm) << modrm_rm;

	return modrm;
}
inline u1 get_modrm(u1 mod, X86_64Register *reg, X86_64Register *rm) {
	return get_modrm_u1(mod,reg->get_index(), rm->get_index());
}
inline u1 get_modrm_reg2reg(X86_64Register *reg, X86_64Register *rm) {
	return get_modrm(0x3,reg,rm);
}
inline u1 get_modrm_1reg(u1 reg, X86_64Register *rm) {
	return get_modrm_u1(0x3,reg,rm->get_index());
}

struct InstructionEncoding {
	template <class T>
	static void reg2reg(CodeMemory *CM, T opcode,
			X86_64Register *reg, X86_64Register *rm) {
		CodeFragment code = CM->get_CodeFragment(2 + sizeof(T));

		code[0] = get_rex(reg,rm);

		for (int i = 0, e = sizeof(T) ; i < e ; ++i) {
			code[i + 1] = (u1) 0xff & (opcode >> (8 * (e - i - 1)));
		}

		code[1 + sizeof(T)] = get_modrm_reg2reg(reg,rm);
	}
	template <class T>
	static void reg2rbp_disp8(CodeMemory *CM, T opcode,
			X86_64Register *reg, s1 disp) {
		CodeFragment code = CM->get_CodeFragment(3 + sizeof(T));

		code[0] = get_rex(reg);

		for (int i = 0, e = sizeof(T) ; i < e ; ++i) {
			code[i + 1] = (u1) 0xff & (opcode >> (8 * (e - i - 1)));
		}

		code[1 + sizeof(T)] = get_modrm(0x1,reg,&RBP);
		code[2 + sizeof(T)] = u1(disp);
	}
	template <class T>
	static void reg2rbp_disp32(CodeMemory *CM, T opcode,
			X86_64Register *reg, s4 disp) {
		CodeFragment code = CM->get_CodeFragment(6 + sizeof(T));

		code[0] = get_rex(reg);

		for (int i = 0, e = sizeof(T) ; i < e ; ++i) {
			code[i + 1] = (u1) 0xff & (opcode >> (8 * (e - i - 1)));
		}

		code[1 + sizeof(T)] = get_modrm(0x2,reg,&RBP);
		code[2 + sizeof(T)] = u1( 0xff & (disp >> (0 * 8)));
		code[3 + sizeof(T)] = u1( 0xff & (disp >> (1 * 8)));
		code[4 + sizeof(T)] = u1( 0xff & (disp >> (2 * 8)));
		code[5 + sizeof(T)] = u1( 0xff & (disp >> (3 * 8)));
	}
	template <class O,class I>
	static void reg2imm(CodeMemory *CM, O opcode,
			X86_64Register *reg, I imm) {
		CodeFragment code = CM->get_CodeFragment(1 + sizeof(O) + sizeof(I));

		code[0] = get_rex(reg);

		for (int i = 0, e = sizeof(O) ; i < e ; ++i) {
			code[i + 1] = (u1) 0xff & (opcode >> (8 * (e - i - 1)));
		}
		for (int i = 0, e = sizeof(I) ; i < e ; ++i) {
			code[i + sizeof(O) + 1] = (u1) 0xff & (imm >> (8 * i));
		}

	}
	template <class O,class I>
	static void reg2imm_modrm(CodeMemory *CM, O opcode,
			u1 reg, X86_64Register *rm, I imm) {
		CodeFragment code = CM->get_CodeFragment(2 + sizeof(O) + sizeof(I));

		code[0] = get_rex(rm);

		for (int i = 0, e = sizeof(O) ; i < e ; ++i) {
			code[i + 1] = (u1) 0xff & (opcode >> (8 * (e - i - 1)));
		}
		code[1 + sizeof(O)] = get_modrm_1reg(reg,rm);
		for (int i = 0, e = sizeof(I) ; i < e ; ++i) {
			code[i + sizeof(O) + 2] = (u1) 0xff & (imm >> (8 * i));
		}

	}
	template <class O,class I>
	static void imm_op(CodeMemory *CM, O opcode, I imm) {
		CodeFragment code = CM->get_CodeFragment(sizeof(O) + sizeof(I));

		for (int i = 0, e = sizeof(O) ; i < e ; ++i) {
			code[i] = (u1) 0xff & (opcode >> (8 * (e - i - 1)));
		}
		for (int i = 0, e = sizeof(I) ; i < e ; ++i) {
			code[i + sizeof(O)] = (u1) 0xff & (imm >> (8 * i));
		}

	}
	template <class O,class I>
	static void imm_op(CodeFragment &code, O opcode, I imm) {
		assert(code.size() == (sizeof(O) + sizeof(I)));

		for (int i = 0, e = sizeof(O) ; i < e ; ++i) {
			code[i] = (u1) 0xff & (opcode >> (8 * (e - i - 1)));
		}
		for (int i = 0, e = sizeof(I) ; i < e ; ++i) {
			code[i + sizeof(O)] = (u1) 0xff & (imm >> (8 * i));
		}

	}
};
#if 0
template <>
static void InstructionEncoding::reg2reg<u1>(
		CodeMemory *CM, u1 opcode,
		X86_64Register *src, X86_64Register *dst) {
	CodeFragment code = CM->get_CodeFragment(3);
	code[0] = get_rex(src,dst);
	code[1] = opcode;
	code[2] = get_modrm_reg2reg(src,dst);
}
#endif

} // end namespace x86_64
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
