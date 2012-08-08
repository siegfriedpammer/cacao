#include "Scalar.hpp"

#include <algorithm>
#include <sstream>

void Scalar::upperBoundOfMinimumWith(const Scalar& other)
{
	if (_array == NO_ARRAY)
	{
		if (other._array == NO_ARRAY)
		{
			// _constant = min(_constant, other._constant)
			if (other._constant < _constant)
				_constant = other._constant;
		}
		else
		{
			if (other._constant <= 0)   // no overflow
			{
				_constant = other._constant;
				_array = other._array;
				/*
				// _constant = min(_constant, other._constant + MAX)
				if (other._constant + Max() < _constant)
					_constant = other._constant + Max();
				*/
			}
			// _constant stays the same if an overflow can occur.
		}
	}
	else
	{
		if (other._array == NO_ARRAY)
		{
			/*
			_array = NO_ARRAY;
			
			if (_constant <= 0)   // no overflow
				_constant = std::min(_constant + Max(), other._constant);
			else
				_constant = other._constant;
			*/
		}
		else
		{
			if (_constant > other._constant)
			{
				_constant = other._constant;
				_array = other._array;
			}
			// else _constant and _array stay the same
		}
	}
}

void Scalar::lowerBoundOfMaximumWith(const Scalar& other)
{
	if (_array == NO_ARRAY)
	{
		if (other._array == NO_ARRAY)
		{
			// _constant = max(_constant, other._constant)
			if (other._constant > _constant)
				_constant = other._constant;
		}
		else
		{
			if (_constant < other._constant)
			{
				_constant = other._constant;
				_array = other._array;
			}
			// else _constant and _array stay the same
		}
	}
	else
	{
		if (other._array == NO_ARRAY)
		{
			if (other._constant > _constant)
			{
				_constant = other._constant;
				_array = NO_ARRAY;
			}
			// else _constant and _array stay the same
		}
		else
		{
			if (_constant < other._constant)
			{
				_constant = other._constant;
				_array = other._array;
			}
			// else _constant and _array stay the same
		}
	}
}

void Scalar::upperBoundOfMaximumWith(const Scalar& other)
{
	if (_array == NO_ARRAY)
	{
		if (other._array == NO_ARRAY)
		{
			// _constant = max(_constant, other._constant)
			if (other._constant > _constant)
				_constant = other._constant;
		}
		else
		{
			if (_constant > 0 || other._constant > 0)
			{
				_constant = Max();
				// _array stays the same
			}
			else
			{
				if (other._constant > _constant)
					_constant = other._constant;
				_array = other._array;
			}
		}
	}
	else
	{
		if (other._array == NO_ARRAY)
		{
			if (_constant > 0 || other._constant > 0)
			{
				_constant = Max();
				_array = NO_ARRAY;
			}
			else
			{
				if (other._constant > _constant)
					_constant = other._constant;
			}
		}
		else
		{
			if (_array == other._array)
			{
				if (_constant > 0 || other._constant > 0)
				{
					_constant = Max();
					_array = NO_ARRAY;
				}
				else
				{
					if (other._constant > _constant)
						_constant = other._constant;
				}
			}
			else
			{
				_constant = Max();
				_array = NO_ARRAY;
			}
		}
	}
}

void Scalar::lowerBoundOfMinimumWith(const Scalar& other)
{
	if (_array == NO_ARRAY)
	{
		if (other._array == NO_ARRAY)
		{
			if (other._constant < _constant)
				_constant = other._constant;
		}
		else
		{
			if (other._constant > 0)
			{
				_constant = Min();
			}
			else
			{
				if (other._constant < _constant)
					_constant = other._constant;
			}
			// _array stays the same
		}
	}
	else
	{
		if (other._array == NO_ARRAY)
		{
			if (_constant > 0)
			{
				_constant = Min();
			}
			else
			{
				if (other._constant < _constant)
					_constant = other._constant;
			}
			_array = NO_ARRAY;
		}
		else
		{
			if (_constant > 0 || other._constant > 0)
			{
				_constant = Min();
				_array = NO_ARRAY;
			}
			else
			{
				if (other._constant < _constant)
					_constant = other._constant;

				if (_array != other._array)
					_array = NO_ARRAY;
			}
		}
	}
}

bool Scalar::tryAdd(const Scalar& other)
{
	if (_array == NO_ARRAY)
	{
		if (other._array == NO_ARRAY)
		{
			if (other._constant > 0)
			{
				if (_constant + other._constant >= _constant)   // no overflow
				{
					_constant += other._constant;
					return true;
				}
				else   // overflow
				{
					return false;
				}
			}
			else
			{
				if (_constant + other._constant <= _constant)   // no underflow
				{
					_constant += other._constant;
					return true;
				}
				else   // underflow
				{
					return false;
				}
			}
		}
		else
		{
			if (other._constant > 0)
			{
				// When other._constant + arraylength is increased, an overflow can happen.
				// When it is decreased, an underflow is possible.
				return false;
			}
			else
			{
				s4 sum = _constant + other._constant;
				if (sum <= _constant &&		// no underflow
					sum <= 0)				// sum + arraylength cannot overflow
				{
					_constant = sum;
					_array = other._array;
					return true;
				}
				else   // underflow/overflow possible
				{
					return false;
				}
			}
		}
	}
	else
	{
		if (other._array == NO_ARRAY)
		{
			if (_constant > 0)
			{
				// When _constant + arraylength is increased, an overflow can happen.
				// When it is decreased, an underflow is possible.
				return false;
			}
			else
			{
				s4 sum = _constant + other._constant;
				if (sum <= other._constant &&	// no underflow
					sum <= 0)					// sum + arraylength cannot overflow
				{
					_constant = sum;
					// _array stays the same.
					return true;
				}
				else   // underflow/overflow possible
				{
					return false;
				}
			}
		}
		else
		{
			// arraylength + arraylength can overflow
			return false;
		}
	}
}

bool Scalar::trySubtract(const Scalar& other)
{
	if (_array == NO_ARRAY)
	{
		if (other._array == NO_ARRAY)
		{
			if (other._constant > 0)
			{
				if (_constant - other._constant <= _constant)   // no underflow
				{
					_constant -= other._constant;
					return true;
				}
				else   // underflow
				{
					return false;
				}
			}
			else
			{
				if (_constant - other._constant >= _constant)   // no overflow
				{
					_constant -= other._constant;
					return true;
				}
				else   // overflow
				{
					return false;
				}
			}
		}
		else
		{
			// -arraylength is not representable
			return false;
		}
	}
	else
	{
		if (other._array == NO_ARRAY)
		{
			if (_constant > 0)
			{
				// When _constant + arraylength is increased, an overflow can happen.
				// When it is decreased, an underflow is possible.
				return false;
			}
			else
			{
				if (other._constant > 0)
				{
					if (_constant - other._constant <= _constant)   // no underflow
					{
						_constant -= other._constant;
						return true;
					}
					else   // underflow
					{
						return false;
					}
				}
				else
				{
					if (_constant - other._constant >= _constant)   // no overflow
					{
						_constant -= other._constant;
						return true;
					}
					else   // overflow
					{
						return false;
					}
				}
			}
		}
		else
		{
			return false;
			/*if (_array == other._array)
			{
				if (_constant <= 0 && other._constant <= 0)   // arraylength + constant cannot overflow
				{
					if (_constant - other._constant >= _constant)   // no overflow
				}
				else
				{
				}
			}
			else
			{
				// not representable
				return false;
			}*/
		}
	}
}


std::ostream& operator<<(std::ostream& out, const Scalar& scalar)
{
	if (scalar.array() != Scalar::NO_ARRAY)
	{
		out << '(' << scalar.array() << ')';
		if (scalar.constant() >= 0)
			out << '+';
	}

	if (scalar.constant() == Scalar::Min())
		out << "MIN";
	else if (scalar.constant() == Scalar::Max())
		out << "MAX";
	else
		out << scalar.constant();
	
	return out;
}
