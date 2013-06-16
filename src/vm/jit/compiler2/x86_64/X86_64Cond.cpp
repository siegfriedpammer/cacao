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

const X86_64Cond::COND X86_64Cond::O(0x00);
const X86_64Cond::COND X86_64Cond::NO(0x01);
const X86_64Cond::COND X86_64Cond::B(0x02);
const X86_64Cond::COND X86_64Cond::C(0x02);
const X86_64Cond::COND X86_64Cond::NAE(0x02);
const X86_64Cond::COND X86_64Cond::NB(0x03);
const X86_64Cond::COND X86_64Cond::NC(0x03);
const X86_64Cond::COND X86_64Cond::AE(0x03);
const X86_64Cond::COND X86_64Cond::Z(0x04);
const X86_64Cond::COND X86_64Cond::E(0x04);
const X86_64Cond::COND X86_64Cond::NZ(0x05);
const X86_64Cond::COND X86_64Cond::NE(0x05);
const X86_64Cond::COND X86_64Cond::BE(0x06);
const X86_64Cond::COND X86_64Cond::NA(0x06);
const X86_64Cond::COND X86_64Cond::NBE(0x07);
const X86_64Cond::COND X86_64Cond::A(0x07);
const X86_64Cond::COND X86_64Cond::S(0x08);
const X86_64Cond::COND X86_64Cond::NS(0x09);
const X86_64Cond::COND X86_64Cond::P(0x0a);
const X86_64Cond::COND X86_64Cond::PE(0x0a);
const X86_64Cond::COND X86_64Cond::NP(0x0b);
const X86_64Cond::COND X86_64Cond::PO(0x0b);
const X86_64Cond::COND X86_64Cond::L(0x0c);
const X86_64Cond::COND X86_64Cond::NGE(0x0c);
const X86_64Cond::COND X86_64Cond::NL(0x0d);
const X86_64Cond::COND X86_64Cond::GE(0x0d);
const X86_64Cond::COND X86_64Cond::LE(0x0e);
const X86_64Cond::COND X86_64Cond::NG(0x0e);
const X86_64Cond::COND X86_64Cond::NLE(0x0f);
const X86_64Cond::COND X86_64Cond::G(0x0f);
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
