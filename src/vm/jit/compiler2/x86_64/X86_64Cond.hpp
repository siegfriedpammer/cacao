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
	struct COND {
		const u1 code;
	private:
		COND(u1 code) : code(code) {}
		friend class X86_64Cond;
	};
#if 0
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
#endif

	static const COND O; ///< overflow (OF = 1)
	static const COND NO; ///< not overflow (OF = 0)
	static const COND B; ///< below (CF = 1)
	static const COND C; ///< carry (CF = 1)
	static const COND NAE; ///< not above or equal (CF = 1)
	static const COND NB; ///< not below (CF = 0)
	static const COND NC; ///< not carry (CF = 0)
	static const COND AE; ///< above or equal (CF = 0)
	static const COND Z; ///< zero (ZF = 1)
	static const COND E; ///< equal (ZF = 1)
	static const COND NZ; ///< not zero (ZF = 0)
	static const COND NE; ///< not equal (ZF = 0)
	static const COND BE; ///< below or equal (CF = 1 or ZF = 1)
	static const COND NA; ///< not above (CF = 1 or ZF = 1)
	static const COND NBE; ///< not below or equal (CF = 0 and ZF = 0)
	static const COND A; ///< above (CF = 0 and ZF = 0)
	static const COND S; ///< sign (SF = 1)
	static const COND NS; ///< not sign (SF = 0)
	static const COND P; ///< parity (PF = 1)
	static const COND PE; ///< parity even (PF = 1)
	static const COND NP; ///< not parity (PF = 0)
	static const COND PO; ///< parity odd (PF = 0)
	static const COND L; ///< less (SF <> OF)
	static const COND NGE; ///< not greater or equal (SF <> OF)
	static const COND NL; ///< not less (SF = OF)
	static const COND GE; ///< greater or equal (SF = OF)
	static const COND LE; ///< less or equal (ZF = 1 or SF <> OF)
	static const COND NG; ///< not greater (ZF = 1 or SF <> OF)
	static const COND NLE; ///< not less or equal (ZF = 0 and SF = OF)
	static const COND G; ///< greater (ZF = 0 and SF = OF)
#if 0
// vim search and replace
// declare
// :'<,'>s/J\([^ ]*\) [^ ]* 7\(.\) cb Jump if \(.*\)\.$/\tstatic const COND \1; \/\/\/< \3/cg
// define
// :'<,'>s/J\([^ ]*\) [^ ]* 7\(.\) cb Jump if \(.*\)\.$/const X86_64Cond::COND X86_64Cond::\1 = {0x0\L\2};/cg

// raw data
JO rel8off 70 cb Jump if overflow (OF = 1).
JNO rel8off 71 cb Jump if not overflow (OF = 0).
JB rel8off 72 cb Jump if below (CF = 1).
JC rel8off 72 cb Jump if carry (CF = 1).
JNAE rel8off 72 cb Jump if not above or equal (CF = 1).
JNB rel8off 73 cb Jump if not below (CF = 0).
JNC rel8off 73 cb Jump if not carry (CF = 0).
JAE rel8off 73 cb Jump if above or equal (CF = 0).
JZ rel8off 74 cb Jump if zero (ZF = 1).
JE rel8off 74 cb Jump if equal (ZF = 1).
JNZ rel8off 75 cb Jump if not zero (ZF = 0).
JNE rel8off 75 cb Jump if not equal (ZF = 0).
JBE rel8off 76 cb Jump if below or equal (CF = 1 or ZF = 1).
JNA rel8off 76 cb Jump if not above (CF = 1 or ZF = 1).
JNBE rel8off 77 cb Jump if not below or equal (CF = 0 and ZF = 0).
JA rel8off 77 cb Jump if above (CF = 0 and ZF = 0).
JS rel8off 78 cb Jump if sign (SF = 1).
JNS rel8off 79 cb Jump if not sign (SF = 0).
JP rel8off 7A cb Jump if parity (PF = 1).
JPE rel8off 7A cb Jump if parity even (PF = 1).
JNP rel8off 7B cb Jump if not parity (PF = 0).
JPO rel8off 7B cb Jump if parity odd (PF = 0).
JL rel8off 7C cb Jump if less (SF <> OF).
JNGE rel8off 7C cb Jump if not greater or equal (SF <> OF).
JNL rel8off 7D cb Jump if not less (SF = OF).
JGE rel8off 7D cb Jump if greater or equal (SF = OF).
JLE rel8off 7E cb Jump if less or equal (ZF = 1 or SF <> OF).
JNG rel8off 7E cb Jump if not greater (ZF = 1 or SF <> OF).
JNLE rel8off 7F cb Jump if not less or equal (ZF = 0 and SF = OF).
JG rel8off 7F cb Jump if greater (ZF = 0 and SF = OF).
#endif
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
