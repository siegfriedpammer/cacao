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


template<> const W& zero<W>() { return wzr; }
template<> const X& zero<X>() { return xzr; }

template<> const u1 sFlag<W>() { return 0; }
template<> const u1 sFlag<X>() { return 1; }
template<> const u1 sFlag<S>() { return 0; }
template<> const u1 sFlag<D>() { return 1; }

template<> const u1 type<S>() { return 0; }
template<> const u1 type<D>() { return 1; }

template<> const u1 size<B>() { return 0; }
template<> const u1 size<H>() { return 1; }
template<> const u1 size<W>() { return 2; }
template<> const u1 size<X>() { return 3; }
template<> const u1 size<S>() { return 2; }
template<> const u1 size<D>() { return 3; }

template<> const u1 v<B>() { return 0; }
template<> const u1 v<H>() { return 0; }
template<> const u1 v<W>() { return 0; }
template<> const u1 v<X>() { return 0; }
template<> const u1 v<S>() { return 1; }
template<> const u1 v<D>() { return 1; }


const Shift::SHIFT Shift::LSL(0x0);
const Shift::SHIFT Shift::LSR(0x1);
const Shift::SHIFT Shift::ASR(0x2);


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
