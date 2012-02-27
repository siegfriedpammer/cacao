#ifndef _LOOP_HPP
#define _LOOP_HPP


typedef struct LoopData LoopData;
typedef struct DominatorData DominatorData;


#if defined(__cplusplus)


#include <vector>

#include "vm/jit/jit.hpp"

/**
 * Per-method data used in jitdata.
 */
struct LoopData
{
	std::vector<basicblock*>	vertex;
	s4 							n;
	basicblock*					root;

	LoopData()
		: n(0)
		, root(0)
	{}
};

/**
 * Per-basicblock data used in basicblock.
 */
struct DominatorData
{
	basicblock*					parent;
	std::vector<basicblock*>	pred;
	s4							semi;
	std::vector<basicblock*>	bucket;
	basicblock*					ancestor;
	basicblock*					label; 

	basicblock*					dom;		// after calculateDominators: the immediate dominator
	std::vector<basicblock*>	children;	// the children of a node in the dominator tree

	DominatorData()
		: parent(0)
		, semi(0)
		, ancestor(0)
		, label(0)
		, dom(0)
	{}
};


void removeArrayBoundChecks(jitdata*);


#endif // __cplusplus
#endif // _LOOP_HPP
