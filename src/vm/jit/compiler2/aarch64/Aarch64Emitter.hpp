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

#include <cmath>

#include "vm/jit/compiler2/alloc/vector.hpp"
#include "vm/jit/compiler2/CodeMemory.hpp"
#include "vm/jit/compiler2/aarch64/Aarch64Cond.hpp"

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

/// Represents a half word
struct H {
	u1 reg;
	explicit H(u1 reg) : reg(reg) {}
};

/// Represents a byte
struct B {
	u1 reg;
	explicit B(u1 reg) : reg(reg) {}
};

/// Represents a 64-bit SIMD&FP register
struct D {
	u1 reg;
	explicit D(u1 reg) : reg(reg) {}
};

/// Represents a 32-bit SIMD&FP register
struct S {
	u1 reg;
	explicit S(u1 reg) : reg(reg) {}
};


// Predefine zero register
const W wzr(0x1f);
const X xzr(0x1f);

const X XSP(0x1f);
const X XFP(0x1d);

class Shift {
public:
	struct SHIFT {
		const u1 code;
	private:
		SHIFT(u1 code) : code(code) {}
		friend class Shift;
	};

	static const SHIFT LSL;
	static const SHIFT LSR;
	static const SHIFT ASR;
};

class Emitter {
private:
	typedef alloc::vector<u4>::type instruction_list;
	instruction_list instructions;

public:
	explicit Emitter() : instructions() {}

	template<typename T> void sbfm(const T& rd, const T& rn, u1 immr, u1 imms);
	void sxtw(const X& xd, const W& wn) { 
		sbfm(xd, static_cast<X>(wn.reg), 0, 31); 
	}
	template<typename T> void sxtb(const T& rd, const T& rn) {
		sbfm(rd, rn, 0, 7);
	}
	template<typename T> void sxth(const T& rd, const T& rn) {
		sbfm(rd, rn, 0, 15);
	}
	template<typename T> void ubfm(const T& rd, const T& rn, u1 immr, u1 imms);
	void uxth(const W& wd, const W& wn) { ubfm(wd, wn, 0, 15); }
	void ubfx(const X& wd, const X& wn) {
		ubfm(wd, wn, 0, 31);
	}

	template<typename T> void add(const T& rd, const T& rn, s2 imm);
	template<typename T> void sub(const T& rd, const T& rn, s2 imm);
	template<typename T> void subs(const T& rd, const T& rn, s2 imm);
	template<typename T> void cmp(const T& rn, s2 imm) {
		subs(static_cast<T>(XSP.reg), rn, imm);
	}
	
	template<typename T> void movn(const T& rd, u2 imm);
	template<typename T> void movz(const T& rd, u2 imm);
	template<typename T> void movk(const T& rd, u2 imm, u1 shift = 0);

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

	void adr(const X& xd, s4 offset) {
		u1 low = offset & 0x3;
		u4 high = (offset >> 2) & 0x7ffff;
		u4 instr = 0x10000000 | lsl(low, 29) | lsl(high, 5) | xd.reg;
		instructions.push_back(instr);
	}

	template<typename T> void ldr(const T& rt, s4 offset);

	template<typename T> void ldur(const T& rt, const X& rn, s2 imm = 0);
	template<typename T> void stur(const T& rt, const X& rn, s2 imm = 0);

	template<typename T> void ldr(const T& rt, const X& rn, s2 imm = 0);
	template<typename T> void str(const T& rt, const X& rn, s2 imm = 0);

	template<typename T> void add(const T& rd, const T& rn, const T& rm,
								  Shift::SHIFT shift = Shift::LSL, u1 amount = 0);
	template<typename T> void sub(const T& rd, const T& rn, const T& rm);
	template<typename T> void subs(const T& rd, const T& rn, const T& rm);
	template<typename T> void neg(const T& rd, const T& rm);
	template<typename T> void cmp(const T& rn, const T& rm);

	template<typename T> void csel(const T& rd, const T& rn, const T& rm, 
	                               Cond::COND cond);

	template<typename T> void mul(const T& rd, const T& rn, const T& rm);
	template<typename T> void madd(const T& rd, const T& rn, const T& rm,
	                               const T& ra);
	template<typename T> void msub(const T& rd, const T& rn, const T& rm,
	                               const T& ra);

	void nop() { instructions.push_back(0xd503201f); }
	template<typename T> void mov(const T& rd, const T& rm);
	template<typename T> void andd(const T& rd, const T& rn, const T& rm);
	template<typename T> void orr(const T& rd, const T& rn, const T& rm);

	template<typename T> void sdiv(const T& rd, const T& rn, const T& rm);

	template<typename T> void fcmp(const T& rn, const T& rm);

	template<typename T> void fmov(const T& rd, const T& rn);
	template<typename T> void fneg(const T& rd, const T& rn);
	template<typename T, typename S> void fcvt(const T& rd, const S& rn);

	template<typename T> void fadd(const T& rd, const T& rn, const T& rm);
	template<typename T> void fdiv(const T& rd, const T& rn, const T& rm);
	template<typename T> void fmul(const T& rd, const T& rn, const T& rm);
	template<typename T> void fsub(const T& rd, const T& rn, const T& rm);

	template<typename T, typename S> void scvtf(const T& rd, const S& rn);

	void emit(CodeMemory* cm);
	void emit(CodeFragment& cf);
	void emitRaw(u4 instr) { instructions.push_back(instr); }

private:
	template<typename T>
	u4 lsl(T a, u1 amount) { return a << amount; }

	void add_subtract_immediate(u1 sf, u1 op, u1 s, u1 shift, s2 imm, 
	                            u1 rn, u1 rd) {
		assert(imm >= 0 && imm <= 4095);
		u4 instr = 0x11000000 | lsl(sf, 31) | lsl(op, 30) | lsl(s, 29)
							  | lsl(shift, 22) | lsl(imm, 10) | lsl(rn, 5) | rd;
		instructions.push_back(instr);
	}

	void bitfield(u1 sf, u1 opc, u1 n, u1 immr, u1 imms, u1 rn, u1 rd) {
		u4 instr = 0x13000000 | lsl(sf, 31) | lsl(opc, 29) | lsl(n, 22)
							  | lsl(immr, 16) | lsl(imms, 10) | lsl(rn, 5) | rd;
		instructions.push_back(instr);
	}

	void move_wide_immediate(u1 sf, u1 opc, u1 hw, u2 imm, u1 rd) {
		u4 instr = 0x12800000 | lsl(sf, 31) | lsl(opc, 29) | lsl(hw, 21)
		                      | lsl(imm, 5) | rd;
		instructions.push_back(instr);
	}

	void load_literal(u1 opc, u1 v, s4 imm, u1 rt) {
		// TODO: validate imm
		s4 off = (imm / 4) & 0x7ffff;
		off += 1;
		u4 instr = 0x18000000 | lsl(opc, 30) | lsl(v, 26) | lsl(off, 5) | rt;
		instructions.push_back(instr);
	}

	void load_store_unscaled(u1 size, u1 v, u1 opc, s2 imm, u1 rn, u1 rt) {
		assert(imm >= -256 && imm <= 255);
		u4 instr = 0x38000000 | lsl(size, 30) | lsl(v, 26) | lsl(opc, 22) 
		                      | lsl(imm & 0x1ff, 12) | lsl(rn, 5) | rt;
		instructions.push_back(instr);
	}

	void load_store_unsigned(u1 size, u1 v, u1 opc, s2 imm, u1 rn, u1 rt) {
		imm = imm / std::pow(2, size);
		assert(imm >= 0 && imm <= 4096);
		u4 instr = 0x39000000 | lsl(size, 30) | lsl(v, 26) | lsl(opc, 22)
							  | lsl(imm, 10) | lsl(rn, 5) | rt;
		instructions.push_back(instr);
	}

	void add_subtract_shifted_register(u1 sf, u1 opc, u1 s, u1 shift, u1 rm,
	                                   u1 imm, u1 rn, u1 rd) {
		u4 instr = 0x0b000000 | lsl(sf, 31) | lsl(opc, 30) | lsl(s, 29)
				   | lsl(shift, 22) | lsl(rm, 16) | lsl(imm, 10) | lsl(rn, 5)
				   | rd;
		instructions.push_back(instr);
	}

	void conditional_select(u1 sf, u1 op, u1 s, u1 rm, u1 cond, u1 op2, 
	                        u1 rn, u1 rd) {
		u4 instr = 0x1a800000 | lsl(sf, 31) | lsl(op, 30) | lsl(s, 29)
				   			  | lsl(rm, 16) | lsl(cond, 12) | lsl(op2, 10) 
							  | lsl(rn, 5) | rd;
		instructions.push_back(instr);
	}

	void data_processing_2_source(u1 sf, u1 s, u1 rm, u1 op, u1 rn, u1 rd) {
    	u4 instr = 0x1ac00000 | lsl(sf, 31) | lsl(s, 29) | lsl(rm, 16) 
							  | lsl(op, 10) | lsl(rn, 5) | rd;
		instructions.push_back(instr);
	}

	void data_processing_3_source(u1 sf, u1 op54, u1 op31, u1 rm, u1 o0,
	                              u1 ra, u1 rn, u1 rd) {
		u4 instr = 0x1b000000 | lsl(sf, 31) | lsl(op54, 29) | lsl(op31, 21) 
			       | lsl(rm, 16) | lsl(o0, 15) | lsl(ra, 10) | lsl(rn, 5) | rd;
		instructions.push_back(instr);
	}

	void logical_shifted_register(u1 sf, u1 opc, u1 n, u1 rm, u1 rn, u1 rd) {
		u4 instr = 0x0a000000 | lsl(sf, 31) | lsl(opc, 29) | lsl(n, 21)
							  | lsl(rm, 16) | lsl(rn, 5) | rd;
		instructions.push_back(instr);
	}

	void fp_compare(u1 m, u1 s, u1 type, u1 rm, u1 op, u1 rn, u1 op2) {
		u4 instr = 0x1e202000 | lsl(m, 31) | lsl(s, 29) | lsl(type, 22)
							  | lsl(rm, 16) | lsl(op, 14) | lsl(rn, 5) | op2;
		instructions.push_back(instr);
	}

	void fp_data_processing_1(u1 m, u1 s, u1 type, u1 op, u1 rn, u1 rd) {
		u4 instr = 0x1e204000 | lsl(m, 31) | lsl(s, 29) | lsl(type, 22)
							  | lsl(op, 15) | lsl(rn, 5) | rd;
		instructions.push_back(instr);
	}

	void fp_data_processing_2(u1 m, u1 s, u1 type, u1 rm, u1 op, u1 rn, u1 rd) {
		u4 instr = 0x1e200800 | lsl(m, 31) | lsl(s, 29) | lsl(type, 22)
							  | lsl(rm, 16) | lsl(op, 12) | lsl(rn, 5) | rd;
		instructions.push_back(instr);
	}

	void conversion_fp_integer(u1 sf, u1 s, u1 type, u1 rmode, u1 op, 
	                           u1 rn, u1 rd) {
		u4 instr = 0x1e200000 | lsl(sf, 31) | lsl(s, 29) | lsl(type, 22)
		                      | lsl(rmode, 19) | lsl(op, 16) | lsl(rn, 5) | rd;
		instructions.push_back(instr);
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
