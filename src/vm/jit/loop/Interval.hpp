#ifndef _INTERVAL_HPP
#define _INTERVAL_HPP

#include <limits>
#include <iostream>

#include "vm/types.h"
#include "Scalar.hpp"

/**
 * An integer interval of the form
 * [ X.length + ] a .. [ Y.length + ] b
 * where both bounds are included.
 */
class Interval
{
	Scalar _lower;
	Scalar _upper;

public:

	static const s4 NO_ARRAY = Scalar::NO_ARRAY;

	static s4 Min() { return Scalar::Min(); }
	static s4 Max() { return Scalar::Max(); }

	/**
	 * Creates the interval MIN .. MAX
	 */
	Interval()
		: _lower(Scalar(Min(), NO_ARRAY))
		, _upper(Scalar(Max(), NO_ARRAY))
	{}

	Scalar lower() const { return _lower; }
	void lower(Scalar s) { _lower = s; }

	Scalar upper() const { return _upper; }
	void upper(Scalar s) { _upper = s; }

	/**
	 * true if it can be proven that arrayVariable can be accessed at an index
	 * whose value lies within this interval.
	 */
	bool isWithinBounds(s4 arrayVariable) const;

	/**
	 * Computes a superset of the intersection of this interval with the specified interval.
	 * The result is stored in this object.
	 */
	void intersectWith(const Interval&);

	/**
	 * Computes a superset of the union of this interval with the specified interval.
	 * The result is stored in this object.
	 */
	void unionWith(const Interval&);

	/**
	 * Tries to remove the specified scalar from this interval.
	 * This interval is only changed if the result can be represented as an interval.
	 */
	void tryRemove(const Scalar&);
};

std::ostream& operator<<(std::ostream&, const Interval&);

#endif
