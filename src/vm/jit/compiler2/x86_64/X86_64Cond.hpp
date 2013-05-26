/* src/vm/jit/compiler2/X86_64Cond.hpp - X86_64Cond

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

#ifndef _JIT_COMPILER2_X86_64COND
#define _JIT_COMPILER2_X86_64COND

#include "vm/jit/compiler2/x86_64/X86_64.hpp"
#include "vm/jit/compiler2/Backend.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
/**
 * x86_64 registers flags
 */
class X86_64Cond {
public:
	enum COND {
		C,   /* carry (CF = 1). */
		NAE, /* not above or equal (CF = 1). */
		NB,  /* not below (CF = 0). */
		NC,  /* not carry (CF = 0). */
		AE,  /* above or equal (CF = 0). */
		Z,   /* zero (ZF = 1). */
		E,   /* equal (ZF = 1). */
		NZ,  /* not zero (ZF = 0). */
		NE,  /* not equal (ZF = 0). */
		BE,  /* below or equal (CF = 1 or ZF = 1). */
		NA,  /* not above (CF = 1 or ZF = 1). */
		NBE, /* not below or equal (CF = 0 and ZF = 0). */
		A,   /* above (CF = 0 and ZF = 0). */
		S,   /* sign (SF = 1). */
		NS,  /* not sign (SF = 0). */
		P,   /* parity (PF = 1). */
		PE,  /* parity even (PF = 1). */
		NP,  /* not parity (PF = 0). */
		PO,  /* parity odd (PF = 0). */
		L,   /* less (SF <> OF). */
		NGE, /* not greater or equal (SF <> OF). */
		NL,  /* not less (SF = OF). */
		GE,  /* greater or equal (SF = OF). */
		LE,  /* less or equal (ZF = 1 or SF <> OF). */
		NG,  /* not greater (ZF = 1 or SF <> OF). */
		NLE, /* not less or equal (ZF = 0 and SF = OF). */
		G,   /* greater (ZF = 0 and SF = OF). */
		NO_COND
	};
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_X86_64COND */


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
