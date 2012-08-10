#ifndef _NUMERIC_INSTRUCTION_HPP
#define _NUMERIC_INSTRUCTION_HPP

#include <limits>
#include <iostream>
#include <cassert>

#include "vm/types.h"


/**
 * Represents a constant numeric instruction.
 */
class NumericInstruction
{
public:

	enum Kind   // sorted
	{
		ZERO = 0,
		ARRAY_LENGTH,
		VARIABLE
	};

private:

	Kind	_kind;
	s4		_var;

	static s4 lowerBounds[3];
	static s4 upperBounds[3];

	NumericInstruction(Kind kind, s4 variable)
		: _kind(kind)
		, _var(variable)
	{}

public:
	
	NumericInstruction(const NumericInstruction& other)
		: _kind(other._kind)
		, _var(other._var)
	{}
	
	static NumericInstruction newZero();
	static NumericInstruction newArrayLength(s4 variable);
	static NumericInstruction newVariable(s4 variable);

	Kind kind() const { return _kind; }
	s4 variable() const { return _var; }

	/**
	 * The smallest value this instruction can return.
	 */
	s4 lower() const;

	/**
	 * The largest value this instruction can return.
	 */
	s4 upper() const;
};


inline NumericInstruction NumericInstruction::newZero()
{
	return NumericInstruction(ZERO, 0);
}

inline NumericInstruction NumericInstruction::newArrayLength(s4 variable)
{
	return NumericInstruction(ARRAY_LENGTH, variable);
}

inline NumericInstruction NumericInstruction::newVariable(s4 variable)
{
	return NumericInstruction(VARIABLE, variable);
}

inline bool operator==(const NumericInstruction& a, const NumericInstruction& b)
{
	return a.kind() == b.kind() && a.variable() == b.variable();
}

inline bool operator!=(const NumericInstruction& a, const NumericInstruction& b)
{
	return a.kind() != b.kind() || a.variable() != b.variable();
}

inline s4 NumericInstruction::lower() const
{
	return lowerBounds[_kind];
	/*switch (_kind)
	{
		case ZERO:			return 0;
		case ARRAY_LENGTH:	return 0;
		case VARIABLE:		return std::numeric_limits<s4>::min();
		default:			assert(false);
	}
	return 0;*/
}

inline s4 NumericInstruction::upper() const
{
	return upperBounds[_kind];
	/*switch (_kind)
	{
		case ZERO:			return 0;
		case ARRAY_LENGTH:	return std::numeric_limits<s4>::max();
		case VARIABLE:		return std::numeric_limits<s4>::max();
		default:			assert(false);
	}
	return 0;*/
}

inline std::ostream& operator<<(std::ostream& out, const NumericInstruction& instruction)
{
	switch (instruction.kind())
	{
		case NumericInstruction::ZERO:			return out << '0';
		case NumericInstruction::ARRAY_LENGTH:	return out << '(' << instruction.variable() << ").length";
		case NumericInstruction::VARIABLE:		return out << '(' << instruction.variable() << ')';
		default:								assert(false);
	}
	return out;
}

#endif
