#ifndef _LOOP_HPP
#define _LOOP_HPP


typedef struct MethodLoopData MethodLoopData;
typedef struct BasicblockLoopData BasicblockLoopData;
typedef struct LoopContainer LoopContainer;
typedef struct Edge Edge;


#if defined(__cplusplus)


#include <vector>

#include "vm/jit/jit.hpp"

/**
 * Per-method data used in jitdata.
 */
struct MethodLoopData
{
	std::vector<basicblock*>	vertex;
	s4 							n;
	basicblock*					root;

	// During the depth first traversal in dominator.cpp an edge is added to this vector
	// if it points from the current node to a node which has already been visited.
	// Not every edge in this vector is also a loopBackEdge!
	std::vector<Edge>			depthBackEdges;

	// Contains all edges (a,b) from depthBackEdges where b dominates a.
	std::vector<Edge>			loopBackEdges;

	// Contains pointers to all loops in this method.
	std::vector<LoopContainer*>	loops;

	// Every method has exactly one (pseudo) root loop that is executed exactly once.
	LoopContainer*				rootLoop;

	MethodLoopData()
		: n(0)
		, root(0)
		, rootLoop(0)
	{}
};

/**
 * Per-basicblock data used in basicblock.
 * Contains information about the dominator tree.
 */
struct BasicblockLoopData
{
	basicblock*					parent;
	std::vector<basicblock*>	pred;
	s4							semi;
	std::vector<basicblock*>	bucket;
	basicblock*					ancestor;
	basicblock*					label; 

	basicblock*					dom;		// after calculateDominators: the immediate dominator
	std::vector<basicblock*>	children;	// the children of a node in the dominator tree

	// Used to prevent this basicblock from being visited again during a traversal.
	// This is NOT a pointer to the loop this basicblock belongs to because such a loop is not unique.
	LoopContainer*				visited;	

	BasicblockLoopData()
		: parent(0)
		, semi(0)
		, ancestor(0)
		, label(0)
		, dom(0)
		, visited(0)
	{}
};

/**
 * Represents a single loop.
 */
struct LoopContainer
{
	LoopContainer*					parent;		// the parent loop or 0 if this is the root loop
	std::vector<LoopContainer*>		children;	// all loops contained in this loop

	basicblock*						header;		// the unique entry point of this loop
	std::vector<basicblock*>		nodes;		// all nodes contained in this loop except the header

	LoopContainer()
		: parent(0)
		, header(0)
	{}
};

/**
 * An edge in the control flow graph.
 */
struct Edge
{
	basicblock*		from;
	basicblock*		to;

	Edge()
		: from(0)
		, to(0)
	{}

	Edge(basicblock* from, basicblock* to)
		: from(from)
		, to(to)
	{}
};


void removeArrayBoundChecks(jitdata*);


#endif // __cplusplus
#endif // _LOOP_HPP
