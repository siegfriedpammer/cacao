#include "IntervalMap.hpp"

std::ostream& operator<<(std::ostream& out, IntervalMap& map)
{
	static const Interval defaultInterval = Interval();   // [MIN,MAX]

	for (size_t i = 0; i < map._intervals.size(); i++)
	{
		if (map._intervals[i] != defaultInterval)
			out << i << ':' << map._intervals[i] << ' ';
	}

	return out;
}
