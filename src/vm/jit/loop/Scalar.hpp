#ifndef _SCALAR_HPP
#define _SCALAR_HPP

#include <limits>
#include <iostream>

#include "vm/types.h"

/**
 * An integral value of the form
 * Constant [ + Array.length ].
 */
class Scalar
{
	s4 _constant;
	s4 _array;

public:

	static const s4 NO_ARRAY = -1;

	static s4 Min() { return std::numeric_limits<s4>::min(); }
	static s4 Max() { return std::numeric_limits<s4>::max(); }

	/**
	 * Creates a scalar which equals zero.
	 */
	Scalar()
		: _constant(0)
		, _array(NO_ARRAY)
	{}

	Scalar(s4 constant, s4 arrayVariable)
		: _constant(constant)
		, _array(arrayVariable)
	{}

	s4 constant() const { return _constant; }
	void constant(s4 value) { _constant = value; }

	s4 array() const { return _array; }
	void array(s4 value) { _array = value; }
	
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
	 * If an overflow can happen, this scalar will not be changed and false will be returned.
	 * Otherwise the return value is true.
	 */
	bool tryAdd(const Scalar&);

	/**
	 * Tries to subtract a scalar from this scalar.
	 * If an overflow can happen, this scalar will not be changed and false will be returned.
	 * Otherwise the return value is true.
	 */
	bool trySubtract(const Scalar&);
};


// True if a equals b for every possible array length, false otherwise.
inline bool operator==(const Scalar& a, const Scalar& b)
{
	return a.constant() == b.constant() && a.array() == b.array();
}

// True if a does not equal b for every possible array length, false otherwise.
inline bool operator!=(const Scalar& a, const Scalar& b)
{
	return a.constant() != b.constant() || a.array() != b.array();
}

std::ostream& operator<<(std::ostream&, const Scalar&);

#endif
