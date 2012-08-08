#include "Interval.hpp"

bool Interval::isWithinBounds(s4 arrayVariable) const
{
	if ((_lower.array() == NO_ARRAY) &&
		(_upper.array() == arrayVariable))   // constLower .. arrayVariable.length + constUpper
	{
		// (arrayVariable.length + constUpper) cannot over-/underflow if the following condition holds.
		return (0 <= _lower.constant()) && (_upper.constant() < 0);
	}

	// In case of (_arrayLengthLower == NO_ARRAY) and (_arrayLengthUpper == NO_ARRAY)
	// the ABC can be removed if (_constUpper < _constLower) holds.
	// But in this case the corresponding instruction is never reached!

	return false;
}

void Interval::intersectWith(const Interval& other)
{
	_lower.lowerBoundOfMaximumWith(other._lower);
	_upper.upperBoundOfMinimumWith(other._upper);
}

void Interval::unionWith(const Interval& other)
{
	_lower.lowerBoundOfMinimumWith(other._lower);
	_upper.upperBoundOfMaximumWith(other._upper);
}

void Interval::tryRemove(const Scalar& point)
{
	// Remove point from interval if it is an endpoint.
	if (point == _lower)
	{
		// Check if an overflow can happen when _lower is increased by one.
		if (_lower.array() == NO_ARRAY)
		{
			if (_lower.constant() < Max())
				_lower.constant(_lower.constant() + 1);
		}
		else
		{
			if (_lower.constant() < 0)
				_lower.constant(_lower.constant() + 1);
		}
	}
	else if (point == _upper)
	{
		// Check if an underflow can happen when _upper is decreased by one.
		if (_upper.array() == NO_ARRAY)
		{
			if (_upper.constant() > Min())
				_upper.constant(_upper.constant() - 1);
		}
		else
		{
			if (Min() < _upper.constant() && _upper.constant() <= 0)
				_upper.constant(_upper.constant() - 1);
		}
	}
}

std::ostream& operator<<(std::ostream& out, const Interval& interval)
{
	return out << '[' << interval.lower() << ',' << interval.upper() << ']';
}
