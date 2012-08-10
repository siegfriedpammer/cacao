#include "IntervalMap.hpp"

std::ostream& operator<<(std::ostream& out, const IntervalMap& map)
{
	static const Interval defaultInterval = Interval();   // [MIN,MAX]

	for (size_t i = 0; i < map._size; i++)
	{
		if (map._intervals[i] != defaultInterval)
			out << i << ':' << map._intervals[i] << ' ';
	}
	return out;
}
