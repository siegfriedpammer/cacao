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

template<typename T>
void Emitter::movn(const T& rd, u2 imm) {
	u4 instr = move_wide_immediate(sFlag<T>(), 0, imm, rd.reg);
	instructions.push_back(instr);
}

template<typename T>
void Emitter::movz(const T& rd, u2 imm) {
	u4 instr = move_wide_immediate(sFlag<T>(), 2, imm, rd.reg);
	instructions.push_back(instr);
} 

template<typename T>
void Emitter::add(const T& rd, const T& rn, const T& rm) {
    u4 instr = add_subtract_shifted_register(sFlag<T>(), 0, 0);
    instr |= lsl(rm.reg, 16) | lsl(rn.reg, 5) | rd.reg;
    instructions.push_back(instr);
}

template<typename T>
void Emitter::sub(const T& rd, const T& rn, const T& rm) {
    u4 instr = add_subtract_shifted_register(sFlag<T>(), 1, 0);
    instr |= lsl(rm.reg, 16) | lsl(rn.reg, 5) | rd.reg;
    instructions.push_back(instr);
}

template<typename T>
void Emitter::subs(const T& rd, const T& rn, const T& rm) {
    u4 instr = add_subtract_shifted_register(sFlag<T>(), 1, 1);
    instr |= lsl(rm.reg, 16) | lsl(rn.reg, 5) | rd.reg;
    instructions.push_back(instr);
}

template<typename T>
void Emitter::cmp(const T& rn, const T& rm) { subs(zero<T>(), rn, rm); }

template<typename T>
void Emitter::mul(const T& rd, const T& rn, const T& rm) {
    madd(rd, rn, rm, zero<T>());
}

template<typename T>
void Emitter::madd(const T& rd, const T& rn, const T& rm, const T& ra) {
    u4 instr = data_processing_3_source(sFlag<T>(), 0, 0, 0);
    instr |= lsl(rm.reg, 16) | lsl(ra.reg, 10) | lsl(rn.reg, 5) | rd.reg;
    instructions.push_back(instr);
}

template<typename T>
void Emitter::mov(const T& rd, const T& rm) { orr(rd, zero<T>(), rm); }

template<typename T>
void Emitter::orr(const T& rd, const T& rn, const T& rm) {
	u4 instr = logical_shifted_register(sFlag<T>(), 1, 0);
	instr |= lsl(rm.reg, 16) | lsl(rn.reg, 5) | rd.reg;
	instructions.push_back(instr);
}

template <typename T>
void Emitter::sdiv(const T& rd, const T& rn, const T& rm) {
    u4 instr = data_processing_2_source(sFlag<T>(), 0, rm.reg, 3, 
                                        rn.reg, rd.reg);
    instructions.push_back(instr);
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
