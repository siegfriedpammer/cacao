/* src/vm/jit/compiler2/aarch64/Aarch64Emitter.hpp

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

#ifndef _JIT_COMPILER2_AARCH64_EMITTER
#define _JIT_COMPILER2_AARCH64_EMITTER

#include "vm/jit/compiler2/alloc/vector.hpp"
#include "vm/jit/compiler2/CodeMemory.hpp"

#include "vm/types.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace aarch64 {

/// Represents a 64-bit GP register
struct X {
	u1 reg;
	explicit X(u1 reg) : reg(reg) {}
};

/// Represents the lower 32-bit part of a GP register
struct W {
	u1 reg;
	explicit W(u1 reg) : reg(reg) {}
};

// Predefine zero register
const W wzr(0x1f);
const X xzr(0x1f);

class Emitter {
private:
	typedef alloc::vector<u4>::type instruction_list;
	instruction_list instructions;

public:
	explicit Emitter() : instructions() {}

	template<typename T> void movn(const T& rd, u2 imm);
	template<typename T> void movz(const T& rd, u2 imm);

	void bcond(u1 cond, s4 offset) {
		s4 off = offset >> 2;
		off++;
		u4 instr = 0x54000000 | lsl(off, 5) | cond;
		instructions.push_back(instr);
	}

	void b(s4 offset) {
		s4 off = offset >> 2;
		off++;
		u4 instr = 0x14000000 | (off & 0x3ffffff);
		instructions.push_back(instr);
	}

	template<typename T> void add(const T& rd, const T& rn, const T& rm);
	template<typename T> void sub(const T& rd, const T& rn, const T& rm);
	template<typename T> void subs(const T& rd, const T& rn, const T& rm);
	template<typename T> void cmp(const T& rn, const T& rm);

	template<typename T> void mul(const T& rd, const T& rn, const T& rm);
	template<typename T> void madd(const T& rd, const T& rn, const T& rm,
	                               const T& ra);

	void nop() { instructions.push_back(0xd503201f); }
	template<typename T> void mov(const T& rd, const T& rm);
	template<typename T> void orr(const T& rd, const T& rn, const T& rm);

	template<typename T> void sdiv(const T& rd, const T& rn, const T& rm);

	void emit(CodeMemory* cm);
	void emit(CodeFragment& cf);

private:
	template<typename T>
	u4 lsl(T a, u1 amount) { return a << amount; }

	u4 move_wide_immediate(u1 sf, u1 opc, u2 imm, u1 rd) {
		return 0x12800000 | lsl(sf, 31) | lsl(opc, 29) | lsl(imm, 5) | rd;
	}

	u4 branch(u1 op0, u2 op1) {
		return 0x14000000 | lsl(op0, 29) | lsl(op1, 22);
	}

	u4 logical_shifted_register(u1 sf, u1 opc, u1 n) {
		return 0x0a000000 | lsl(sf, 31) | lsl(opc, 29) | lsl(n, 21);
	}

	u4 add_subtract_shifted_register(u1 sf, u1 opc, u1 s) {
		return 0x0b000000 | lsl(sf, 31) | lsl(opc, 30) | lsl(s, 29);
	}

	u4 data_processing_3_source(u1 sf, u1 op54, u1 op31, u1 o0) {
		return 0x1b000000 | lsl(sf, 31) | lsl(op54, 29) | lsl(op31, 21) 
					      | lsl(o0, 15);
	}

	u4 data_processing_2_source(u1 sf, u1 s, u1 rm, u1 op, u1 rn, u1 rd) {
    	return 0x1ac00000 | lsl(sf, 31) | lsl(s, 29) | lsl(rm, 16) | lsl(op, 10)
           	              | lsl(rn, 5) | rd;
	}	
};

// Separted all the template definitions to keep file size "reasonable"
#define _INSIDE_EMITTER_HPP
#include "vm/jit/compiler2/aarch64/Aarch64EmitterT.hpp"
#undef _INSIDE_EMITTER_HPP


} // end namespace aarch64
} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif // JIT_COMPILER2_AARCH64_EMITTER


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
