#ifndef _INTERVAL_MAP_HPP
#define _INTERVAL_MAP_HPP

#include <iostream>
#include <cassert>
#include <algorithm>

#include "Interval.hpp"
#include "DynamicVector.hpp"

/**
 * Maps variable names to intervals.
 */
class IntervalMap
{
	DynamicVector<Interval> _intervals;

public:

	size_t size() const { return _intervals.size(); }

	/**
	 * Returns the interval of the specified variable.
	 */
	Interval& operator[](size_t varIndex);

	/**
	 * Computes the union set of this map with the specified map.
	 * This object will hold the result.
	 */
	void unionWith(const IntervalMap&);


	friend std::ostream& operator<<(std::ostream&, IntervalMap&);
};


inline Interval& IntervalMap::operator[](size_t varIndex)
{
	return _intervals[varIndex];
}

inline void IntervalMap::unionWith(const IntervalMap& other)
{
	_intervals.resize(std::min(_intervals.size(), other._intervals.size()));

	for (size_t i = 0; i < _intervals.size(); i++)
	{
		_intervals[i].unionWith(other._intervals[i]);
	}
}

#endif
