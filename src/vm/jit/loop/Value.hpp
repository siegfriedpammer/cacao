#ifndef _VALUE_HPP
#define _VALUE_HPP

#include <limits>

#include "vm/types.h"

/**
 * Represents the result of the addition of a certain IR-variable with a certain constant.
 * The addition may overflow.
 */
class Value
{
	s4 _variable;   // If _variable == -1, this value is considered as "unknown".
	s4 _constant;

	/**
	 * Initializes this value with (variable + constant).
	 */
	Value(s4 variable, s4 constant)
		: _variable(variable)
		, _constant(constant)
	{}

public:

	static Value newAddition(s4 variable, s4 constant);
	static Value newUnknown();

	s4 variable() const;
	s4 constant() const;

	bool isUnknown() const;

	/**
	 * Adds a constant to this value.
	 * If this value is "unknown", it stays unknown.
	 */
	void addConstant(s4 constant);

	/**
	 * Subtracts a constant from this value.
	 * If this value is "unknown", it stays unknown.
	 */
	void subtractConstant(s4 constant);
};

inline Value Value::newAddition(s4 variable, s4 constant)
{
	assert(variable != -1);
	return Value(variable, constant);
}

inline Value Value::newUnknown()
{
	return Value(-1, 0);
}

inline s4 Value::variable() const
{
	assert(_variable != -1);
	return _variable;
}

inline s4 Value::constant() const
{
	return _constant;
}

inline bool Value::isUnknown() const
{
	return _variable == -1;
}

inline void Value::addConstant(s4 constant)
{
	s8 sum = static_cast<s8>(_constant) + constant;
	if (std::numeric_limits<s4>::min() <= sum && sum <= std::numeric_limits<s4>::max())
		_constant = static_cast<s4>(sum);
	else
		_variable = -1;   // In case of an overflow, we get an "unknown" value.
}

inline void Value::subtractConstant(s4 constant)
{
	s8 sum = static_cast<s8>(_constant) - constant;
	if (std::numeric_limits<s4>::min() <= sum && sum <= std::numeric_limits<s4>::max())
		_constant = static_cast<s4>(sum);
	else
		_variable = -1;   // In case of an overflow, we get an "unknown" value.
}

#endif
