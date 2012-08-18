#include "Scalar.hpp"

#include <algorithm>
#include <sstream>

void Scalar::upperBoundOfMaximumWith(const Scalar& other)
{
	if (_instruction == other._instruction)
	{
		if (other._constant > _constant)
			_constant = other._constant;
	}
	else
	{
		s4 left = _constant + _instruction.lower();
		s4 right = _constant + _instruction.upper();
		s4 otherLeft = other._constant + other._instruction.lower();
		s4 otherRight = other._constant + other._instruction.upper();

		if (right <= otherLeft)
		{
			*this = other;
		}
		else if (right <= otherRight)
		{
			*this = Scalar(otherRight);
		}
		else if (left < otherRight)
		{
			*this = Scalar(right);
		}
	}
}

void Scalar::lowerBoundOfMinimumWith(const Scalar& other)
{
	if (_instruction == other._instruction)
	{
		if (other._constant < _constant)
			_constant = other._constant;
	}
	else
	{
		s4 left = _constant + _instruction.lower();
		s4 right = _constant + _instruction.upper();
		s4 otherLeft = other._constant + other._instruction.lower();
		s4 otherRight = other._constant + other._instruction.upper();

		if (left >= otherRight)
		{
			*this = other;
		}
		else if (left >= otherLeft)
		{
			*this = Scalar(otherLeft);
		}
		else if (right > otherLeft)
		{
			*this = Scalar(left);
		}
	}
}

bool Scalar::tryAdd(const Scalar& other)
{
	s8 c = static_cast<s8>(_constant) + other._constant;

	// Does constant overflow?
	if (c < min() || c > max())
		return false;

	if (other._instruction.lower() == 0 && other._instruction.upper() == 0)
	{
		// Does (constant + instruction) overflow?
		if (min() <= c + _instruction.lower() && c + _instruction.upper() <= max())
		{
			_constant = static_cast<s4>(c);
			return true;
		}
	}
	else if (_instruction.lower() == 0 && _instruction.upper() == 0)
	{
		// Does (constant + instruction) overflow?
		if (min() <= c + other._instruction.lower() && c + other._instruction.upper() <= max())
		{
			*this = Scalar(static_cast<s4>(c), other._instruction);
			return true;
		}
	}

	return false;
}

bool Scalar::trySubtract(const Scalar& other)
{
	s8 c = static_cast<s8>(_constant) - other._constant;

	// Does constant overflow?
	if (c < min() || c > max())
		return false;

	if (other._instruction.lower() == 0 && other._instruction.upper() == 0)
	{
		// Does (constant + instruction) overflow?
		if (min() <= c + _instruction.lower() && c + _instruction.upper() <= max())
		{
			_constant = static_cast<s4>(c);
			return true;
		}
	}
	else if (_instruction == other._instruction)   // (c0 + i) - (c1 + i) == c0 - c1
	{
		*this = Scalar(static_cast<s4>(c));
		return true;
	}

	return false;
}


std::ostream& operator<<(std::ostream& out, const Scalar& scalar)
{
	if (scalar.constant() == Scalar::min())
		out << "MIN";
	else if (scalar.constant() == Scalar::max())
		out << "MAX";
	else if (scalar.constant() != 0)
		out << scalar.constant();

	if (scalar.instruction().lower() != 0 || scalar.instruction().upper() != 0)
	{
		if (scalar.constant() != 0)
			out << '+';
		out << scalar.instruction();
	}

	if (scalar.constant() == 0 && scalar.instruction().lower() == 0 && scalar.instruction().upper() == 0)
		out << '0';
	
	return out;
}
