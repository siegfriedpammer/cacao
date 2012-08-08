#include "IntervalMap.hpp"

std::ostream& operator<<(std::ostream& out, const IntervalMap& map)
{
	for (size_t i = 0; i < map._size; i++)
	{
		Scalar l = map._intervals[i].lower();
		Scalar u = map._intervals[i].upper();
		if (l.array() != Interval::NO_ARRAY || l.constant() != Interval::Min() ||
			u.array() != Interval::NO_ARRAY || u.constant() != Interval::Max())
		{
			out << i << ':' << map._intervals[i] << ' ';
		}
	}
	return out;
}
