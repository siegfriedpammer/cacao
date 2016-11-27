/* src/vm/jit/compiler2/aarch64/Aarch64EmitterT.hpp

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

#ifndef _INSIDE_EMITTER_HPP
#error This header is only allowed to be included in Aarch64Emitter.hpp!
#endif


template<typename T> const T& zero();
template<typename T> const u1 sFlag();
template<typename T> const u1 type();
template<typename T> const u1 size();
template<typename T> const u1 v();

template<typename T> 
void Emitter::sbfm(const T& rd, const T& rn, u1 immr, u1 imms) {
    bitfield(sFlag<T>(), 0, sFlag<T>(), immr, imms, rn.reg, rd.reg);
}

template<typename T> 
void Emitter::ubfm(const T& rd, const T& rn, u1 immr, u1 imms) {
    bitfield(sFlag<T>(), 2, sFlag<T>(), immr, imms, rn.reg, rd.reg);
}

template<typename T> 
void Emitter::add(const T& rd, const T& rn, s2 imm) {
    add_subtract_immediate(sFlag<T>(), 0, 0, 0, imm, rn.reg, rd.reg);
}

template<typename T> 
void Emitter::sub(const T& rd, const T& rn, s2 imm) {
    add_subtract_immediate(sFlag<T>(), 1, 0, 0, imm, rn.reg, rd.reg);
}

template<typename T> 
void Emitter::subs(const T& rd, const T& rn, s2 imm) {
    add_subtract_immediate(sFlag<T>(), 1, 1, 0, imm, rn.reg, rd.reg);
}

template<typename T>
void Emitter::movn(const T& rd, u2 imm) {
	move_wide_immediate(sFlag<T>(), 0, 0, imm, rd.reg);
}

template<typename T>
void Emitter::movz(const T& rd, u2 imm) {
	move_wide_immediate(sFlag<T>(), 2, 0, imm, rd.reg);
}

template<typename T>
void Emitter::movk(const T& rd, u2 imm, u1 shift) {
	move_wide_immediate(sFlag<T>(), 3, shift, imm, rd.reg);
}

template<typename T> 
void Emitter::ldr(const T& rt, s4 offset) {
    load_literal(sFlag<T>(), v<T>(), offset, rt.reg);
}

template<typename T>
void Emitter::ldur(const T& rt, const X& rn, s2 imm) {
    load_store_unscaled(size<T>(), v<T>(), 1, imm, rn.reg, rt.reg);
}

template<typename T>
void Emitter::stur(const T& rt, const X& rn, s2 imm) {
    load_store_unscaled(size<T>(), v<T>(), 0, imm, rn.reg, rt.reg);
}

template<typename T> 
void Emitter::ldr(const T& rt, const X& rn, s2 imm) {
    load_store_unsigned(size<T>(), v<T>(), 1, imm, rn.reg, rt.reg);
}

template<typename T> 
void Emitter::str(const T& rt, const X& rn, s2 imm) {
    load_store_unsigned(size<T>(), v<T>(), 0, imm, rn.reg, rt.reg);
}

template<typename T>
void Emitter::add(const T& rd, const T& rn, const T& rm, Shift::SHIFT shift,
                  u1 amount) {
    add_subtract_shifted_register(sFlag<T>(), 0, 0, shift.code, rm.reg, 
                                  amount, rn.reg, rd.reg);
}

template<typename T>
void Emitter::sub(const T& rd, const T& rn, const T& rm) {
    add_subtract_shifted_register(sFlag<T>(), 1, 0, 0, rm.reg, 0, 
                                  rn.reg, rd.reg);
}

template<typename T>
void Emitter::subs(const T& rd, const T& rn, const T& rm) {
    add_subtract_shifted_register(sFlag<T>(), 1, 1, 0, rm.reg, 0, 
                                  rn.reg, rd.reg);
}

template<typename T> 
void Emitter::neg(const T& rd, const T& rm) {  sub(rd, zero<T>(), rm); }

template<typename T>
void Emitter::cmp(const T& rn, const T& rm) { subs(zero<T>(), rn, rm); }

template<typename T> 
void Emitter::csel(const T& rd, const T& rn, const T& rm, Cond::COND cond) {
    conditional_select(sFlag<T>(), 0, 0, rm.reg, cond.code, 0, rn.reg, rd.reg);
}

template<typename T>
void Emitter::mul(const T& rd, const T& rn, const T& rm) {
    madd(rd, rn, rm, zero<T>());
}

template<typename T>
void Emitter::madd(const T& rd, const T& rn, const T& rm, const T& ra) {
    data_processing_3_source(sFlag<T>(), 0, 0, rm.reg, 0, ra.reg, 
                             rn.reg, rd.reg);
}

template<typename T>
void Emitter::msub(const T& rd, const T& rn, const T& rm, const T& ra) {
    data_processing_3_source(sFlag<T>(), 0, 0, rm.reg, 1, ra.reg, 
                             rn.reg, rd.reg);
}

template<typename T>
void Emitter::mov(const T& rd, const T& rm) { orr(rd, zero<T>(), rm); }

template<typename T>
void Emitter::andd(const T& rd, const T& rn, const T& rm) {
	logical_shifted_register(sFlag<T>(), 0, 0, rm.reg, rn.reg, rd.reg);
}

template<typename T>
void Emitter::orr(const T& rd, const T& rn, const T& rm) {
	logical_shifted_register(sFlag<T>(), 1, 0, rm.reg, rn.reg, rd.reg);
}

template <typename T>
void Emitter::sdiv(const T& rd, const T& rn, const T& rm) {
    data_processing_2_source(sFlag<T>(), 0, rm.reg, 3, rn.reg, rd.reg);
}

template<typename T> 
void Emitter::fcmp(const T& rn, const T& rm) {
    fp_compare(0, 0, type<T>(), rm.reg, 0, rn.reg, 0);
}

template<typename T> 
void Emitter::fmov(const T& rd, const T& rn) {
    fp_data_processing_1(0, 0, type<T>(), 0, rn.reg, rd.reg);
}

template<typename T> 
void Emitter::fneg(const T& rd, const T& rn) {
    fp_data_processing_1(0, 0, type<T>(), 2, rn.reg, rd.reg);
}

template<typename T, typename S> 
void Emitter::fcvt(const T& rd, const S& rn) {
    fp_data_processing_1(0, 0, type<S>(), 4 + type<T>(), rn.reg, rd.reg);
}

template <typename T>
void Emitter::fadd(const T& rd, const T& rn, const T& rm) {
    fp_data_processing_2(0, 0, type<T>(), rm.reg, 2, rn.reg, rd.reg);
}

template <typename T>
void Emitter::fdiv(const T& rd, const T& rn, const T& rm) {
    fp_data_processing_2(0, 0, type<T>(), rm.reg, 1, rn.reg, rd.reg);
}

template <typename T>
void Emitter::fmul(const T& rd, const T& rn, const T& rm) {
    fp_data_processing_2(0, 0, type<T>(), rm.reg, 0, rn.reg, rd.reg);
}

template <typename T>
void Emitter::fsub(const T& rd, const T& rn, const T& rm) {
    fp_data_processing_2(0, 0, type<T>(), rm.reg, 3, rn.reg, rd.reg);
}

template<typename T, typename S> 
void Emitter::scvtf(const T& rd, const S& rn) {
    conversion_fp_integer(sFlag<S>(), 0, type<T>(), 0, 2, rn.reg, rd.reg);
}

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
