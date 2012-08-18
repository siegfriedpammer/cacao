#ifndef _LOOP_LIST_HPP
#define _LOOP_LIST_HPP

#include "LoopContainer.hpp"

#include <algorithm>

namespace
{
	bool depthGreaterThan(const LoopContainer* a, const LoopContainer* b)
	{	
		return a->depth > b->depth;
	}
}

class LoopList
{
	std::vector<LoopContainer*> _loops;

public:

	typedef std::vector<LoopContainer*>::iterator iterator;

	iterator begin() { return _loops.begin(); }
	iterator end() { return _loops.end(); }

	/**
	 * Inserts the specified loop if it is not already contained in this list.
	 */
	void insert(LoopContainer* loop);

	/**
	 * Removes the specified loop from the list if the list contains it.
	 */
	void erase(LoopContainer* loop);

	/**
	 * Sort the loops in this list according to their depth in the loop hierarchy.
	 * The first loop will have the highest depth.
	 */
	void sort();

	/**
	 * Searches this list for the specified loop and returns an iterator to the found element.
	 */
	iterator find(LoopContainer* loop) { return std::find(_loops.begin(), _loops.end(), loop); }
};

inline void LoopList::insert(LoopContainer* loop)
{
	iterator it = std::find(_loops.begin(), _loops.end(), loop);
	if (it == _loops.end())
		_loops.push_back(loop);
}

inline void LoopList::erase(LoopContainer* loop)
{
	iterator it = std::find(_loops.begin(), _loops.end(), loop);
	if (it != _loops.end())
		_loops.erase(it);
}

inline void LoopList::sort()
{
	std::sort(_loops.begin(), _loops.end(), depthGreaterThan);
}

#endif
