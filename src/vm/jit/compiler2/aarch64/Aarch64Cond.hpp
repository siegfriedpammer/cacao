/* src/vm/jit/compiler2/aarch64/Aarch64Cond.hpp

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

#ifndef _JIT_COMPILER2_AARCH64_COND
#define _JIT_COMPILER2_AARCH64_COND

#include "vm/types.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace aarch64 {

class Cond {
public:
	struct COND {
		const u1 code;
	private:
		COND(u1 code) : code(code) {}
		friend class Cond;
	};

	static const COND EQ; ///< Equal (Z == 1)
	static const COND NE; ///< Not equal (Z == 0)
	static const COND CS; ///< Carry set (C == 1)
	static const COND CC; ///< Carry clear (C == 0)
	static const COND MI; ///< Minus, negative (N == 1)
	static const COND PL; ///< Plus, positive or zero (N == 0)
	static const COND VS; ///< Overflow (V == 1)
	static const COND VC; ///< No overflow (V == 0)
	static const COND HI; ///< Unsigned higher (C == 1 && Z == 0)
	static const COND LS; ///< Unsigned lower or same (!(C == 1 && Z == 0))
	static const COND GE; ///< Signed greater than or equal (N == V)
	static const COND LT; ///< Signed less then (N != V)
	static const COND GT; ///< Signed greater then (Z == 0 && N == V)
	static const COND LE; ///< Signed less than or equal (!(Z == 0 && N == V))
	static const COND AL; ///< Always (Any)
};

} // end namespace aarch64
} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_AARCH64_COND */


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
