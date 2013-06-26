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
		VoidTypeID
	};

	const TypeID type;

	explicit Type(TypeID type) : type(type) {}

	inline bool isPrimitiveType()     const {
		return type == PrimitiveTypeID || isNumericType()
		    || isBooleanType() || isReturnAddressType();
	}
	inline bool isNumericType()       const {
		return type == NumericTypeID || isIntegralType() || isFloatingPointType();
	}
	inline bool isIntegralType()      const {
		return type == IntegralTypeID || isByteType() || isShortType()
		    || isIntType() || isLongType() || isCharType();
	}
	inline bool isFloatingPointType() const {
		return type == FloatingPointTypeID || isFloatType() || isDoubleType();
	}
	inline bool isReferenceType()     const { return type == ReferenceTypeID; }
	inline bool isBooleanType()       const { return type == BooleanTypeID; }
	inline bool isReturnAddressType() const { return type == ReturnAddressTypeID; }
	inline bool isByteType()          const { return type == ByteTypeID; }
	inline bool isShortType()         const { return type == ShortTypeID; }
	inline bool isIntType()           const { return type == IntTypeID; }
	inline bool isLongType()          const { return type == LongTypeID; }
	inline bool isCharType()          const { return type == CharTypeID; }
	inline bool isFloatType()         const { return type == FloatTypeID; }
	inline bool isDoubleType()        const { return type == DoubleTypeID; }
	inline bool isVoidType()          const { return type == VoidTypeID; }


	virtual PrimitiveType*     toPrimitiveType()     { return NULL; }
	virtual ReferenceType*     toReferenceType()     { return NULL; }
	virtual NumericType*       toNumericType()       { return NULL; }
	virtual BooleanType*       toBooleanType()       { return NULL; }
	virtual ReturnAddressType* toReturnAddressType() { return NULL; }
	virtual IntegralType*      toIntegralType()      { return NULL; }
	virtual FloatingPointType* toFloatingPointType() { return NULL; }
	virtual ByteType*          toByteType()          { return NULL; }
	virtual ShortType*         toShortType()         { return NULL; }
	virtual IntType*           toIntType()           { return NULL; }
	virtual LongType*          toLongType()          { return NULL; }
	virtual CharType*          toCharType()          { return NULL; }
	virtual FloatType*         toFloatType()         { return NULL; }
	virtual DoubleType*        toDoubleType()        { return NULL; }
	virtual VoidType*          toVoidType()          { return NULL; }
};

class PrimitiveType : public Type {
protected:
	explicit PrimitiveType(TypeID id) : Type(id) {}
public:
	explicit PrimitiveType() : Type(PrimitiveTypeID) {}
	virtual PrimitiveType* toPrimitiveType() { return this; }
};

class ReferenceType : public Type {
protected:
	explicit ReferenceType(TypeID id) : Type(id) {}
public:
	explicit ReferenceType() : Type(ReferenceTypeID) {}
	virtual ReferenceType* toReferenceType() { return this; }
};


class NumericType : public PrimitiveType {
protected:
	explicit NumericType(TypeID id) : PrimitiveType(id) {}
public:
	explicit NumericType() : PrimitiveType(NumericTypeID) {}
	virtual NumericType* toNumericType() { return this; }
};


/**
 * The values of the boolean type encode the truth values true and false,
 * and the default value is false. 
 */
class BooleanType : public PrimitiveType {
public:
	explicit BooleanType() : PrimitiveType(BooleanTypeID) {}
	virtual BooleanType* toBooleanType() { return this; }
};

/**
 * The values of the returnAddress type are pointers to the opcodes of Java
 * Virtual Machine instructions. Of the primitive types, only the returnAddress
 * type is not directly associated with a Java programming language type. 
 */
class ReturnAddressType : public PrimitiveType {
public:
	explicit ReturnAddressType() : PrimitiveType(ReturnAddressTypeID) {}
	virtual ReturnAddressType* toReturnAddressType() { return this; }
};


class IntegralType : public NumericType {
protected:
	explicit IntegralType(TypeID id) : NumericType(id) {}
public:
	explicit IntegralType() : NumericType(IntegralTypeID) {}
	virtual IntegralType* toIntegralType() { return this; }
};
class FloatingPointType : public NumericType {
protected:
	explicit FloatingPointType(TypeID id) : NumericType(id) {}
public:
	explicit FloatingPointType() : NumericType(FloatingPointTypeID) {}
	virtual FloatingPointType* toFloatingPointType() { return this; }
};

/**
 * whose values are 8-bit signed two's-complement integers, and whose default value is zero
 */
class ByteType : public IntegralType {
public:
	explicit ByteType() : IntegralType(ByteTypeID) {}
	virtual ByteType* toByteType() { return this; }
};

/**
 * whose values are 16-bit signed two's-complement integers, and whose default value is zero
 */
class ShortType : public IntegralType {
public:
	explicit ShortType() : IntegralType(ShortTypeID) {}
	virtual ShortType* toShortType() { return this; }
};

/**
 * whose values are 32-bit signed two's-complement integers, and whose default value is zero
 */
class IntType : public IntegralType {
public:
	explicit IntType() : IntegralType(IntTypeID) {}
	virtual IntType* toIntType() { return this; }
};

/**
 * whose values are 64-bit signed two's-complement integers, and whose default value is zero
 */
class LongType : public IntegralType {
public:
	explicit LongType() : IntegralType(LongTypeID) {}
	virtual LongType* toLongType() { return this; }
};

/**
 * whose values are 16-bit unsigned integers representing Unicode code points
 * in the Basic Multilingual Plane, encoded with UTF-16, and whose default value
 * is the null code point ('\u0000') 
 */
class CharType : public IntegralType {
public:
	explicit CharType() : IntegralType(CharTypeID) {}
	virtual CharType* toCharType() { return this; }
};


/**
 * whose values are elements of the float value set or, where supported, the
 * float-extended-exponent value set, and whose default value is positive zero
 */
class FloatType : public FloatingPointType {
public:
	explicit FloatType() : FloatingPointType(FloatTypeID) {}
	virtual FloatType* toFloatType() { return this; }
};

/**
 * whose values are elements of the double value set or, where supported, the
 * double-extended-exponent value set, and whose default value is positive zero
 */
class DoubleType : public FloatingPointType {
public:
	explicit DoubleType() : FloatingPointType(DoubleTypeID) {}
	virtual DoubleType* toDoubleType() { return this; }
};

/**
 *
 */
class VoidType : public Type {
public:
	explicit VoidType() : Type(VoidTypeID) {}
	virtual VoidType* toVoidType() { return this; }
};


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
