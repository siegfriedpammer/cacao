/* src/vm/jit/compiler2/Type.cpp - Types used in the 2nd stage compiler IR

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

#include "vm/jit/compiler2/Type.hpp"

#include "toolbox/OStream.hpp"

#include <cassert>

namespace cacao {
namespace jit {
namespace compiler2 {

// conversion functions
const char * get_var_type(int type)
{
	switch(type) {
	case TYPE_INT: return "TYPE_INT";
	case TYPE_LNG: return "TYPE_LNG";
	case TYPE_FLT: return "TYPE_FLT";
	case TYPE_DBL: return "TYPE_DBL";
	case TYPE_ADR: return "TYPE_ADR";
	case TYPE_RET: return "TYPE_RET";
	case TYPE_VOID: return "TYPE_VOID";
	}
	return "(unknown type)";
}

Type::TypeID convert_var_type(int type)
{
	switch(type) {
	case TYPE_INT:  return Type::IntTypeID;
	case TYPE_LNG:  return Type::LongTypeID;
	case TYPE_FLT:  return Type::FloatTypeID;
	case TYPE_DBL:  return Type::DoubleTypeID;
	case TYPE_VOID: return Type::VoidTypeID;
	case TYPE_ADR:  return Type::ReferenceTypeID; // XXX is this right?
	case TYPE_RET:
	default: /* do nothing */ ;
	}
	err() << BoldRed << "error: " << reset_color << "type " << BoldWhite
		  << get_var_type(type) << reset_color << " (0x0" << hex << type << dec << ") "
		  << " not yet supported!" << nl;
	//assert( 0 && "Unsupported type");
	return Type::VoidTypeID;
}

OStream& operator<<(OStream &OS, const Type::TypeID &type) {
	switch(type) {
		case Type::PrimitiveTypeID: return OS << "PrimitiveTypeID";
		case Type::ReferenceTypeID: return OS << "ReferenceTypeID";
		case Type::NumericTypeID: return OS << "NumericTypeID";
		case Type::BooleanTypeID: return OS << "BooleanTypeID";
		case Type::ReturnAddressTypeID: return OS << "ReturnAddressTypeID";
		case Type::IntegralTypeID: return OS << "IntegralTypeID";
		case Type::FloatingPointTypeID: return OS << "FloatingPointTypeID";
		case Type::ByteTypeID: return OS << "ByteTypeID";
		case Type::ShortTypeID: return OS << "ShortTypeID";
		case Type::IntTypeID: return OS << "IntTypeID";
		case Type::LongTypeID: return OS << "LongTypeID";
		case Type::CharTypeID: return OS << "CharTypeID";
		case Type::FloatTypeID: return OS << "FloatTypeID";
		case Type::DoubleTypeID: return OS << "DoubleTypeID";
		case Type::VoidTypeID: return OS << "VoidTypeID";
	}
	assert(0 && "unreachable");
	return OS;
}
#if 0
/**
 * Type Class
 *
 * See JVM spec 2.2
 */
class Type {
public:
	static int ID;
};

class PrimitiveType : public Type {
};

class ReferenceType : public Type {
};


class NumericType : public PrimitiveType {
};


/**
 * The values of the boolean type encode the truth values true and false,
 * and the default value is false. 
 */
class BooleanType : public PrimitiveType {
};

/**
 * The values of the returnAddress type are pointers to the opcodes of Java
 * Virtual Machine instructions. Of the primitive types, only the returnAddress
 * type is not directly associated with a Java programming language type. 
 */
class ReturnAddressType : public PrimitiveType {
};


class IntegralType : public NumericType {
};
class FloatingPointType : public NumericType {
};

/**
 * whose values are 8-bit signed two's-complement integers, and whose default value is zero
 */
class ByteType : public IntegralType {
};

/**
 * whose values are 16-bit signed two's-complement integers, and whose default value is zero
 */
class ShortType : public IntegralType {
};

/**
 * whose values are 32-bit signed two's-complement integers, and whose default value is zero
 */
class IntType : public IntegralType {
};

/**
 * whose values are 64-bit signed two's-complement integers, and whose default value is zero
 */
class LongType : public IntegralType {
};

/**
 * whose values are 16-bit unsigned integers representing Unicode code points
 * in the Basic Multilingual Plane, encoded with UTF-16, and whose default value
 * is the null code point ('\u0000') 
 */
class CharType : public IntegralType {
};


/**
 * whose values are elements of the float value set or, where supported, the
 * float-extended-exponent value set, and whose default value is positive zero
 */
class FloatType : public FloatingPointType {
};

/**
 * whose values are elements of the double value set or, where supported, the
 * double-extended-exponent value set, and whose default value is positive zero
 */
class DoubleType : public FloatingPointType {
};
#endif


} // end namespace cacao
} // end namespace jit
} // end namespace compiler2

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
