/* src/vm/jit/compiler2/Conditional.hpp - Conditionals used in the 2nd stage compiler IR

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

#ifndef _JIT_COMPILER2_CONDITIONAL
#define _JIT_COMPILER2_CONDITIONAL

#include "vm/method.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

class PrimitiveType;
class ReferenceType;
class NumericType;
class BooleanType;
class ReturnAddressType;
class IntegralType;
class FloatingPointType;
class ByteType;
class ShortType;
class IntType;
class LongType;
class CharType;
class FloatType;
class DoubleType;

/**
 * Type Class
 *
 * See JVM spec 2.2
 */
class Conditional {
public:
	enum CondID {
		EQ,
		NE,
		LT,
		GE,
		GT,
		LE,
		NoCond
	};
};

} // end namespace cacao
} // end namespace jit
} // end namespace compiler2

#endif /* _JIT_COMPILER2_CONDITIONAL */


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
