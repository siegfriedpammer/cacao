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

class Reg {
public:
	static Reg X(u1 reg) { return Reg(reg, &XConf); }
	static Reg W(u1 reg) { return Reg(reg, &WConf); }
	static Reg H(u1 reg) { return Reg(reg, &HConf); }
	static Reg B(u1 reg) { return Reg(reg, &BConf); }
	
	static Reg D(u1 reg) { return Reg(reg, &DConf); }
	static Reg S(u1 reg) { return Reg(reg, &SConf); }

	static Reg XZR;
	static Reg WZR;

	static Reg XSP;
	static Reg XFP;

	u1 r() const { return reg; }

	u1 sf() const { return conf->sf; }
	u1 type() const { return conf->type; }
	u1 size() const { return conf->size; }
	u1 v() const { return conf->v; }
	const Reg* zero() const { return conf->zero; }

private:
	struct RegConfiguration {
		u1 sf;
		u1 type;
		u1 size;
		u1 v;

		const Reg* const zero;
	};

	explicit Reg(u1 reg, const RegConfiguration* const conf) 
			: reg(reg), conf(conf) {}

	u1 reg;
	const RegConfiguration* const conf;

	static RegConfiguration XConf;
	static RegConfiguration WConf;
	static RegConfiguration HConf;
	static RegConfiguration BConf;

	static RegConfiguration DConf;
	static RegConfiguration SConf;
};


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

	void sbfm(const Reg& rd, const Reg& rn, u1 immr, u1 imms);
	void sxtw(const Reg& xd, const Reg& wn) { sbfm(xd, wn, 0, 31); }
	void sxtb(const Reg& rd, const Reg& rn) { sbfm(rd, rn, 0, 7); }
	void sxth(const Reg& rd, const Reg& rn) { sbfm(rd, rn, 0, 15); }

	void ubfm(const Reg& rd, const Reg& rn, u1 immr, u1 imms);
	void uxth(const Reg& wd, const Reg& wn) { ubfm(wd, wn, 0, 15); }
	void ubfx(const Reg& wd, const Reg& wn) { ubfm(wd, wn, 0, 31); }

	void add(const Reg& rd, const Reg& rn, s2 imm);
	void sub(const Reg& rd, const Reg& rn, s2 imm);
	void subs(const Reg& rd, const Reg& rn, s2 imm);
	void cmp(const Reg& rn, s2 imm) { subs(Reg::XZR, rn, imm); }
	
	void movn(const Reg& rd, u2 imm);
	void movz(const Reg& rd, u2 imm);
	void movk(const Reg& rd, u2 imm, u1 shift = 0);

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

	void blr(const Reg& rd) {
		u4 instr = 0xd63f0000 | lsl(rd.r(), 5);
		instructions.push_back(instr);
	}

	void ldr(const Reg& rt, s4 offset);

	void ldur(const Reg& rt, const Reg& rn, s2 imm = 0);
	void stur(const Reg& rt, const Reg& rn, s2 imm = 0);

	void ldr(const Reg& rt, const Reg& rn, s2 imm = 0);
	void str(const Reg& rt, const Reg& rn, s2 imm = 0);

	void add(const Reg& rd, const Reg& rn, const Reg& rm, 
	         Shift::SHIFT shift = Shift::LSL, u1 amount = 0);
	void sub(const Reg& rd, const Reg& rn, const Reg& rm);
	void subs(const Reg& rd, const Reg& rn, const Reg& rm);
	void neg(const Reg& rd, const Reg& rm);
	void cmp(const Reg& rn, const Reg& rm);

	void csel(const Reg& rd, const Reg& rn, const Reg& rm, Cond::COND cond);

	void mul(const Reg& rd, const Reg& rn, const Reg& rm);
	void madd(const Reg& rd, const Reg& rn, const Reg& rm, const Reg& ra);
	void msub(const Reg& rd, const Reg& rn, const Reg& rm, const Reg& ra);

	void nop() { instructions.push_back(0xd503201f); }
	void mov(const Reg& rd, const Reg& rm) { orr(rd, *rd.zero(), rm); }
	void andd(const Reg& rd, const Reg& rn, const Reg& rm);
	void orr(const Reg& rd, const Reg& rn, const Reg& rm);

	void sdiv(const Reg& rd, const Reg& rn, const Reg& rm);

	void fcmp(const Reg& rn, const Reg& rm);

	void fmov(const Reg& rd, const Reg& rn);
	void fneg(const Reg& rd, const Reg& rn);
	void fcvt(const Reg& rd, const Reg& rn);

	void fadd(const Reg& rd, const Reg& rn, const Reg& rm);
	void fdiv(const Reg& rd, const Reg& rn, const Reg& rm);
	void fmul(const Reg& rd, const Reg& rn, const Reg& rm);
	void fsub(const Reg& rd, const Reg& rn, const Reg& rm);

	void scvtf(const Reg& rd, const Reg& rn);

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
