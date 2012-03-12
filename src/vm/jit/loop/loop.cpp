#include <sstream>

#include "loop.hpp"
#include "dominator.hpp"
#include "toolbox/logging.hpp"


namespace
{
	void findLoopBackEdges(jitdata* jd);
	void findLoops(jitdata* jd);
	void reverseDepthFirstTraversal(basicblock* next, LoopContainer* loop);
	void printBasicBlocks(jitdata* jd);


	void printBasicBlocks(jitdata* jd)
	{
		// print basic block graph
		log_text("----- Basic Blocks -----");
		for (basicblock* bb = jd->basicblocks; bb != 0; bb = bb->next)
		{
			std::stringstream str;
			str << bb->nr;
			if (bb->type == BBTYPE_EXH)
				str << "*";
			else
				str << " ";
			str << " --> ";
			for (s4 i = 0; i < bb->successorcount; i++)
			{
				str << bb->successors[i]->nr << " ";
			}
			log_text(str.str().c_str());
		}

		// print immediate dominators
		log_text("------ Dominators ------");
		for (basicblock* bb = jd->basicblocks; bb != 0; bb = bb->next)
		{
			std::stringstream str;
			str << bb->nr << " --> ";
			if (bb->ld->dom)
				str << bb->ld->dom->nr;
			else
				str << "X";
			log_text(str.str().c_str());
		}

		// print dominator tree
		log_text("------ Dominator Tree ------");
		for (basicblock* bb = jd->basicblocks; bb != 0; bb = bb->next)
		{
			std::stringstream str;
			str << bb->nr << " --> ";
			for (size_t i = 0; i < bb->ld->children.size(); i++)
			{
				str << bb->ld->children[i]->nr << " ";
			}
			log_text(str.str().c_str());
		}

		log_text("------------------------");
	}

	void findLoopBackEdges(jitdata* jd)
	{
		// Iterate over depthBackEdges and filter those out, which are also loopBackEdges.
		for (std::vector<Edge>::const_iterator edge = jd->ld->depthBackEdges.begin(); edge != jd->ld->depthBackEdges.end(); ++edge)
		{
			// Check if edge->to dominates edge->from.
			// The case edge->to == edge->from is also considered.
			basicblock* ancestor = edge->from;
			while (ancestor)
			{
				if (ancestor == edge->to)
				{
					jd->ld->loopBackEdges.push_back(*edge);
					break;
				}

				// search the dominator tree bottom-up
				ancestor = ancestor->ld->dom;
			}
		}
	}

	/**
	 * For every loopBackEdge there is a loop.
	 * This function finds all basicblocks which belong to that loop.
	 */
	void findLoops(jitdata* jd)
	{
		for (std::vector<Edge>::const_iterator edge = jd->ld->loopBackEdges.begin(); edge != jd->ld->loopBackEdges.end(); ++edge)
		{
			basicblock* head = edge->to;
			basicblock* foot = edge->from;

			LoopContainer* loop = new LoopContainer;
			jd->ld->loops.push_back(loop);
			
			loop->header = head;

			// find all basicblocks contained in this loop
			reverseDepthFirstTraversal(foot, loop);
		}
	}

	void reverseDepthFirstTraversal(basicblock* next, LoopContainer* loop)
	{
		if (next->ld->visited == loop)	// already visited
			return;

		if (next == loop->header)		// Stop the traversal at the header node.
			return;

		loop->nodes.push_back(next);

		// Mark basicblock to prevent it from being visited again.
		next->ld->visited = loop;

		for (s4 i = 0; i < next->predecessorcount; i++)
		{
			reverseDepthFirstTraversal(next->predecessors[i], loop);
		}
	}
}


void removeArrayBoundChecks(jitdata* jd)
{
	log_message_method("removeArrayBoundChecks: ", jd->m);

	createRoot(jd);
	calculateDominators(jd);
	buildDominatorTree(jd);
	findLoopBackEdges(jd);
	findLoops(jd);

	printBasicBlocks(jd);		// for debugging
}


