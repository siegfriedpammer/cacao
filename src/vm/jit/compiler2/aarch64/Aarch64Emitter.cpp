/* src/vm/jit/compiler2/aarch64/Aarch64Emitter.cpp

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

#include "vm/jit/compiler2/aarch64/Aarch64Emitter.hpp"
#include "vm/jit/compiler2/CodeMemory.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace aarch64 {


const Shift::SHIFT Shift::LSL(0x0);
const Shift::SHIFT Shift::LSR(0x1);
const Shift::SHIFT Shift::ASR(0x2);

Reg::RegConfiguration Reg::XConf{1, 0, 3, 0, &Reg::XZR};
Reg::RegConfiguration Reg::WConf{0, 0, 2, 0, &Reg::WZR};
Reg::RegConfiguration Reg::HConf{0, 0, 1, 0, &Reg::WZR};
Reg::RegConfiguration Reg::BConf{0, 0, 0, 0, &Reg::WZR};

Reg::RegConfiguration Reg::DConf{1, 1, 3, 1, &Reg::XZR};
Reg::RegConfiguration Reg::SConf{0, 0, 2, 1, &Reg::WZR};

Reg Reg::XZR(0x1f, &XConf);
Reg Reg::WZR(0x1f, &WConf);

Reg Reg::XSP(0x1f, &XConf);
Reg Reg::XFP(0x1d, &XConf);


void Emitter::emit(CodeMemory* cm) {
	alloc::vector<u4>::type::reverse_iterator rit = instructions.rbegin();
	for (; rit != instructions.rend() ; ++rit) {
		CodeFragment cf = cm->get_CodeFragment(4);
		cf[0] = *rit & 0xff;
		cf[1] = (*rit >> 8) & 0xff;
		cf[2] = (*rit >> 16) & 0xff;
		cf[3] = (*rit >> 24) & 0xfff;
	}
}


void Emitter::emit(CodeFragment& cf) {
    assert(instructions.size() == 1);
    u4 instr = instructions[0];
    cf[0] = instr & 0xff;
    cf[1] = (instr >> 8) & 0xff;
    cf[2] = (instr >> 16) & 0xff;
    cf[3] = (instr >> 24) & 0xff;
}

void Emitter::ldr(const Reg& rt, s4 offset) {
    load_literal(rt.sf(), rt.v(), offset, rt.r());
}

void Emitter::ldur(const Reg& rt, const Reg& rn, s2 imm) {
    load_store_unscaled(rt.size(), rt.v(), 1, imm, rn.r(), rt.r());
}

void Emitter::stur(const Reg& rt, const Reg& rn, s2 imm) {
    load_store_unscaled(rt.size(), rt.v(), 0, imm, rn.r(), rt.r());
}

void Emitter::ldr(const Reg& rt, const Reg& rn, s2 imm) {
    load_store_unsigned(rt.size(), rt.v(), 1, imm, rn.r(), rt.r());
}

void Emitter::str(const Reg& rt, const Reg& rn, s2 imm) {
    load_store_unsigned(rt.size(), rt.v(), 0, imm, rn.r(), rt.r());
}

void Emitter::ldr(const Reg& rt, const Reg& rn, const Reg& rm) {
    load_store_register(rt.size(), rt.v(), 1, rm.r(), 3, 0, rn.r(), rt.r());
}

void Emitter::sbfm(const Reg& rd, const Reg& rn, u1 immr, u1 imms) {
    bitfield(rd.sf(), 0, rd.sf(), immr, imms, rn.r(), rd.r());
}

void Emitter::ubfm(const Reg& rd, const Reg& rn, u1 immr, u1 imms) {
    bitfield(rd.sf(), 2, rd.sf(), immr, imms, rn.r(), rd.r());
}

void Emitter::movn(const Reg& rd, u2 imm) {
	move_wide_immediate(rd.sf(), 0, 0, imm, rd.r());
}

void Emitter::movz(const Reg& rd, u2 imm) {
	move_wide_immediate(rd.sf(), 2, 0, imm, rd.r());
}

void Emitter::movk(const Reg& rd, u2 imm, u1 shift) {
	move_wide_immediate(rd.sf(), 3, shift, imm, rd.r());
}

void Emitter::add(const Reg& rd, const Reg& rn, s2 imm) {
    add_subtract_immediate(rd.sf(), 0, 0, 0, imm, rn.r(), rd.r());
}

void Emitter::sub(const Reg& rd, const Reg& rn, s2 imm) {
    add_subtract_immediate(rd.sf(), 1, 0, 0, imm, rn.r(), rd.r());
}

void Emitter::subs(const Reg& rd, const Reg& rn, s2 imm) {
    add_subtract_immediate(rd.sf(), 1, 1, 0, imm, rn.r(), rd.r());
}

void Emitter::andd(const Reg& rd, const Reg& rn, const Reg& rm) {
	logical_shifted_register(rd.sf(), 0, 0, rm.r(), rn.r(), rd.r());
}

void Emitter::orr(const Reg& rd, const Reg& rn, const Reg& rm) {
	logical_shifted_register(rd.sf(), 1, 0, rm.r(), rn.r(), rd.r());
}

void Emitter::sdiv(const Reg& rd, const Reg& rn, const Reg& rm) {
    data_processing_2_source(rd.sf(), 0, rm.r(), 3, rn.r(), rd.r());
}

void Emitter::madd(const Reg& rd, const Reg& rn, const Reg& rm, const Reg& ra) {
    data_processing_3_source(rd.sf(), 0, 0, rm.r(), 0, ra.r(), rn.r(), rd.r());
}

void Emitter::msub(const Reg& rd, const Reg& rn, const Reg& rm, const Reg& ra) {
    data_processing_3_source(rd.sf(), 0, 0, rm.r(), 1, ra.r(), rn.r(), rd.r());
}

void Emitter::add(const Reg& rd, const Reg& rn, const Reg& rm, 
                  Shift::SHIFT shift, u1 amount) {
    add_subtract_shifted_register(rd.sf(), 0, 0, shift.code, rm.r(), 
                                  amount, rn.r(), rd.r());
}

void Emitter::subs(const Reg& rd, const Reg& rn, const Reg& rm) {
    add_subtract_shifted_register(rd.sf(), 1, 1, 0, rm.r(), 0, rn.r(), rd.r());
}

void Emitter::cmp(const Reg& rn, const Reg& rm) { subs(*rn.zero(), rn, rm); }

void Emitter::sub(const Reg& rd, const Reg& rn, const Reg& rm) {
    add_subtract_shifted_register(rd.sf(), 1, 0, 0, rm.r(), 0, rn.r(), rd.r());
}

void Emitter::neg(const Reg& rd, const Reg& rm) {  sub(rd, *rd.zero(), rm); }

void Emitter::csel(const Reg& rd, const Reg& rn, const Reg& rm, 
                   Cond::COND cond) {
    conditional_select(rd.sf(), 0, 0, rm.r(), cond.code, 0, rn.r(), rd.r());
}

void Emitter::mul(const Reg& rd, const Reg& rn, const Reg& rm) {
    madd(rd, rn, rm, *rd.zero());
}

void Emitter::fneg(const Reg& rd, const Reg& rn) {
    fp_data_processing_1(0, 0, rd.type(), 2, rn.r(), rd.r());
}

void Emitter::fadd(const Reg& rd, const Reg& rn, const Reg& rm) {
    fp_data_processing_2(0, 0, rd.type(), rm.r(), 2, rn.r(), rd.r());
}

void Emitter::fsub(const Reg& rd, const Reg& rn, const Reg& rm) {
    fp_data_processing_2(0, 0, rd.type(), rm.r(), 3, rn.r(), rd.r());
}

void Emitter::fmul(const Reg& rd, const Reg& rn, const Reg& rm) {
    fp_data_processing_2(0, 0, rd.type(), rm.r(), 0, rn.r(), rd.r());
}

void Emitter::fdiv(const Reg& rd, const Reg& rn, const Reg& rm) {
    fp_data_processing_2(0, 0, rd.type(), rm.r(), 1, rn.r(), rd.r());
}

void Emitter::fcmp(const Reg& rn, const Reg& rm) {
    fp_compare(0, 0, rn.type(), rm.r(), 0, rn.r(), 0);
}

void Emitter::fmov(const Reg& rd, const Reg& rn) {
    fp_data_processing_1(0, 0, rd.type(), 0, rn.r(), rd.r());
}

void Emitter::fcvt(const Reg& rd, const Reg& rn) {
    fp_data_processing_1(0, 0, rn.type(), 4 + rd.type(), rn.r(), rd.r());
}

void Emitter::scvtf(const Reg& rd, const Reg& rn) {
    conversion_fp_integer(rn.sf(), 0, rd.type(), 0, 2, rn.r(), rd.r());
}

} // end namespace aarch64
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
