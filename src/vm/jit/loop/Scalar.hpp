#ifndef _SCALAR_HPP
#define _SCALAR_HPP

#include <limits>
#include <iostream>
#include <cassert>

#include "vm/types.h"
#include "NumericInstruction.hpp"


/**
 * An integral value of the form
 * Constant + NumericInstruction.
 */
class Scalar
{
	s4					_constant;
	NumericInstruction	_instruction;

public:

	static s4 min() { return std::numeric_limits<s4>::min(); }
	static s4 max() { return std::numeric_limits<s4>::max(); }

	/**
	 * Creates a scalar which equals zero.
	 */
	Scalar()
		: _constant(0)
		, _instruction(NumericInstruction::newZero())
	{}

	explicit Scalar(s4 constant)
		: _constant(constant)
		, _instruction(NumericInstruction::newZero())
	{}

	explicit Scalar(const NumericInstruction& instruction)
		: _constant(0)
		, _instruction(instruction)
	{}

	/**
	 * Creates a scalar which equals constant + instruction.
	 * The expression must not over- or underflow!
	 */
	Scalar(s4 constant, const NumericInstruction& instruction)
		: _constant(constant)
		, _instruction(instruction)
	{
		s8 cu = static_cast<s8>(constant) + instruction.upper();
		s8 cl = static_cast<s8>(constant) + instruction.lower();

		// check for overflow
		assert(std::numeric_limits<s4>::min() <= cu && cu <= std::numeric_limits<s4>::max());
		assert(std::numeric_limits<s4>::min() <= cl && cl <= std::numeric_limits<s4>::max());
	}

	s4 constant() const { return _constant; }
	NumericInstruction instruction() const { return _instruction; }

	s4 lower() const { return _constant + _instruction.lower(); }
	s4 upper() const { return _constant + _instruction.upper(); }
	
	/**
	 * Computes an upper bound of the minimum of this and the specified scalar.
	 * The result is stored in this object.
	 */
	void upperBoundOfMinimumWith(const Scalar&);
	
	/**
	 * Computes a lower bound of the maximum of this and the specified scalar.
	 * The result is stored in this object.
	 */
	void lowerBoundOfMaximumWith(const Scalar&);
	
	/**
	 * Computes an upper bound of the maximum of this and the specified scalar.
	 * The result is stored in this object.
	 */
	void upperBoundOfMaximumWith(const Scalar&);
	
	/**
	 * Computes a lower bound of the minimum of this and the specified scalar.
	 * The result is stored in this object.
	 */
	void lowerBoundOfMinimumWith(const Scalar&);

	/**
	 * Tries to add a scalar to this scalar.
	 * If an overflow can happen or the result is not representable as a scalar,
	 * this scalar will not be changed and false will be returned.
	 * Otherwise the return value is true.
	 */
	bool tryAdd(const Scalar&);

	/**
	 * Tries to subtract a scalar from this scalar.
	 * If an overflow can happen or the result is not representable as a scalar,
	 * this scalar will not be changed and false will be returned.
	 * Otherwise the return value is true.
	 */
	bool trySubtract(const Scalar&);
};


inline void Scalar::upperBoundOfMinimumWith(const Scalar& other)
{
	if (_constant > other._constant)
		*this = other;
}

inline void Scalar::lowerBoundOfMaximumWith(const Scalar& other)
{
	if (_constant < other._constant)
		*this = other;
}

// True if a equals b for every possible value of the instruction, false otherwise.
inline bool operator==(const Scalar& a, const Scalar& b)
{
	return a.constant() == b.constant() && a.instruction() == b.instruction();
}

// True if a does not equal b for every possible value of the instruction, false otherwise.
inline bool operator!=(const Scalar& a, const Scalar& b)
{
	return a.constant() != b.constant() || a.instruction() != b.instruction();
}

std::ostream& operator<<(std::ostream&, const Scalar&);

#endif
