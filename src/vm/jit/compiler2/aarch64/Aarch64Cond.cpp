/* src/vm/jit/compiler2/aarch64/Aarch64Cond.cpp

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

#include "vm/jit/compiler2/aarch64/Aarch64Cond.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace aarch64 {

const Cond::COND Cond::EQ(0x0);
const Cond::COND Cond::NE(0x1);
const Cond::COND Cond::CS(0x2);
const Cond::COND Cond::CC(0x3);
const Cond::COND Cond::MI(0x4);
const Cond::COND Cond::PL(0x5);
const Cond::COND Cond::VS(0x6);
const Cond::COND Cond::VC(0x7);
const Cond::COND Cond::HI(0x8);
const Cond::COND Cond::LS(0x9);
const Cond::COND Cond::GE(0xa);
const Cond::COND Cond::LT(0xb);
const Cond::COND Cond::GT(0xc);
const Cond::COND Cond::LE(0xd);
const Cond::COND Cond::AL(0xe);

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
