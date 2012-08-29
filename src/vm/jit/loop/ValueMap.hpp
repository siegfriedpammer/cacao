#ifndef _VALUE_MAP_HPP
#define _VALUE_MAP_HPP

#include <vector>

#include "Value.hpp"

/**
 * Contains a Value-object for every variable.
 * Initially every variable v is mapped to the value v + 0.
 */
class ValueMap
{
	std::vector<Value> _values;

public:

	//explicit ValueMap(size_t varCount);

	Value& operator[](size_t varIndex);
};

/*
inline ValueMap::ValueMap(size_t varCount)
{
	_values.reserve(varCount);
	for (size_t i = 0; i < varCount; i++)
	{
		_values.push_back(Value::newAddition(i, 0));
	}
}*/

inline Value& ValueMap::operator[](size_t varIndex)
{
	for (size_t i = _values.size(); i <= varIndex; i++)
	{
		_values.push_back(Value::newAddition(i, 0));
	}
	return _values[varIndex];
}

#endif
