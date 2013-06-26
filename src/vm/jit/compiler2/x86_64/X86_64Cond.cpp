/* src/vm/jit/compiler2/X86_64Cond.cpp - X86_64Cond

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
#include "vm/jit/compiler2/x86_64/X86_64Cond.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace x86_64 {

const Cond::COND Cond::O(0x00);
const Cond::COND Cond::NO(0x01);
const Cond::COND Cond::B(0x02);
const Cond::COND Cond::C(0x02);
const Cond::COND Cond::NAE(0x02);
const Cond::COND Cond::NB(0x03);
const Cond::COND Cond::NC(0x03);
const Cond::COND Cond::AE(0x03);
const Cond::COND Cond::Z(0x04);
const Cond::COND Cond::E(0x04);
const Cond::COND Cond::NZ(0x05);
const Cond::COND Cond::NE(0x05);
const Cond::COND Cond::BE(0x06);
const Cond::COND Cond::NA(0x06);
const Cond::COND Cond::NBE(0x07);
const Cond::COND Cond::A(0x07);
const Cond::COND Cond::S(0x08);
const Cond::COND Cond::NS(0x09);
const Cond::COND Cond::P(0x0a);
const Cond::COND Cond::PE(0x0a);
const Cond::COND Cond::NP(0x0b);
const Cond::COND Cond::PO(0x0b);
const Cond::COND Cond::L(0x0c);
const Cond::COND Cond::NGE(0x0c);
const Cond::COND Cond::NL(0x0d);
const Cond::COND Cond::GE(0x0d);
const Cond::COND Cond::LE(0x0e);
const Cond::COND Cond::NG(0x0e);
const Cond::COND Cond::NLE(0x0f);
const Cond::COND Cond::G(0x0f);

} // end namespace x86_64
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
