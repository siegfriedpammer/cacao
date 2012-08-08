#ifndef _INTERVAL_MAP_HPP
#define _INTERVAL_MAP_HPP

#include <iostream>
#include <cassert>

#include "Interval.hpp"

/**
 * Contains an interval for every variable.
 */
class IntervalMap
{
	Interval*	_intervals;
	size_t		_size;

public:

	explicit IntervalMap(size_t varCount);
	IntervalMap(const IntervalMap& other);

	~IntervalMap();

	size_t size() const { return _size; }

	/**
	 * Returns the interval of the specified variable.
	 */
	Interval& of(size_t varIndex);
	const Interval& of(size_t varIndex) const;

	/**
	 * Returns the interval of the specified variable.
	 */
	Interval& operator[](size_t varIndex);
	const Interval& operator[](size_t varIndex) const;

	/**
	 * Computes the union set of this map with the specified map.
	 * This object will hold the result.
	 */
	void unionWith(const IntervalMap&);

	IntervalMap& operator=(const IntervalMap& other);


	friend std::ostream& operator<<(std::ostream&, const IntervalMap&);
};



inline IntervalMap::IntervalMap(size_t varCount)
{
	_intervals = new Interval[varCount];
	_size = varCount;
}

inline IntervalMap::IntervalMap(const IntervalMap& other)
{
	_size = other._size;
	_intervals = new Interval[_size];
	for (size_t i = 0; i < _size; i++)
	{
		_intervals[i] = other._intervals[i];
	}
}

inline IntervalMap::~IntervalMap()
{
	delete[] _intervals;
}

inline Interval& IntervalMap::of(size_t varIndex)
{
	assert(0 <= varIndex);
	assert(varIndex < _size);

	return _intervals[varIndex];
}

inline const Interval& IntervalMap::of(size_t varIndex) const
{
	assert(0 <= varIndex);
	assert(varIndex < _size);

	return _intervals[varIndex];
}

inline Interval& IntervalMap::operator[](size_t varIndex)
{
	return of(varIndex);
}

inline const Interval& IntervalMap::operator[](size_t varIndex) const
{
	return of(varIndex);
}

inline void IntervalMap::unionWith(const IntervalMap& other)
{
	assert(_size == other._size);

	for (size_t i = 0; i < _size; i++)
	{
		_intervals[i].unionWith(other._intervals[i]);
	}
}

inline IntervalMap& IntervalMap::operator=(const IntervalMap& other)
{
	if (this == &other)
		return *this;

	delete[] _intervals;

	_size = other._size;
	_intervals = new Interval[_size];
	for (size_t i = 0; i < _size; i++)
	{
		_intervals[i] = other._intervals[i];
	}

	return *this;
}

#endif
