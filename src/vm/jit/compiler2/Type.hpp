/* src/vm/jit/compiler2/Type.hpp - Types used in the 2nd stage compiler IR

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

#ifndef _JIT_COMPILER2_TYPE
#define _JIT_COMPILER2_TYPE

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
class VoidType;

/**
 * Type Class
 *
 * See JVM spec 2.2
 */
class Type {
public:
	enum TypeID {
		PrimitiveTypeID,
		ReferenceTypeID,
		NumericTypeID,
		BooleanTypeID,
		ReturnAddressTypeID,
		IntegralTypeID,
		FloatingPointTypeID,
		ByteTypeID,
		ShortTypeID,
		IntTypeID,
		LongTypeID,
		CharTypeID,
		FloatTypeID,
		DoubleTypeID,
		GlobalStateTypeID,
		VoidTypeID
	};

	struct PrimitiveType {
		operator Type::TypeID() { return PrimitiveTypeID; }
	};
	struct ReferenceType {
		operator Type::TypeID() { return ReferenceTypeID; }
	};
	struct NumericType {
		operator Type::TypeID() { return NumericTypeID; }
	};
	struct BooleanType {
		operator Type::TypeID() { return BooleanTypeID; }
	};
	struct ReturnAddressType {
		operator Type::TypeID() { return ReturnAddressTypeID; }
	};
	struct IntegralType {
		operator Type::TypeID() { return IntegralTypeID; }
	};
	struct FloatingPointType {
		operator Type::TypeID() { return FloatingPointTypeID; }
	};
	struct ByteType {
		operator Type::TypeID() { return ByteTypeID; }
	};
	struct ShortType {
		operator Type::TypeID() { return ShortTypeID; }
	};
	struct IntType {
		operator Type::TypeID() { return IntTypeID; }
	};
	struct LongType {
		operator Type::TypeID() { return LongTypeID; }
	};
	struct CharType {
		operator Type::TypeID() { return CharTypeID; }
	};
	struct FloatType {
		operator Type::TypeID() { return FloatTypeID; }
	};
	struct DoubleType {
		operator Type::TypeID() { return DoubleTypeID; }
	};
	struct GlobalStateType {
		operator Type::TypeID() { return GlobalStateTypeID; }
	};
	struct VoidType {
		operator Type::TypeID() { return VoidTypeID; }
	};
};

const char* get_type_name(const Type::TypeID &type);

// conversion functions
const char * get_var_type(int type);
Type::TypeID convert_var_type(int type);

OStream& operator<<(OStream &OS, const Type::TypeID &type);

} // end namespace cacao
} // end namespace jit
} // end namespace compiler2

#endif /* _JIT_COMPILER2_TYPE */


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
