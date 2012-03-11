#include <sstream>

#include "loop.hpp"
#include "dominator.hpp"
#include "toolbox/logging.hpp"


namespace
{
	void findLoopBackEdges(jitdata* jd)
	{
		for (std::vector<Edge>::const_iterator edge = jd->ld->depthBackEdges.begin(); edge != jd->ld->depthBackEdges.end(); ++edge)
		{
			// check if edge->to dominates edge->from
			basicblock* ancestor = edge->from;
			while (ancestor)
			{
				if (ancestor == edge->to)
				{
					jd->ld->loopBackEdges.push_back(*edge);
					break;
				}

				// search the dominator tree bottom-up
				ancestor = ancestor->dominatorData->dom;
			}
		}
	}

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
			if (bb->dominatorData->dom)
				str << bb->dominatorData->dom->nr;
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
			for (size_t i = 0; i < bb->dominatorData->children.size(); i++)
			{
				str << bb->dominatorData->children[i]->nr << " ";
			}
			log_text(str.str().c_str());
		}

		log_text("------------------------");
	}
}


void removeArrayBoundChecks(jitdata* jd)
{
	log_message_method("calculateDominators: ", jd->m);

	createRoot(jd);
	calculateDominators(jd);
	buildDominatorTree(jd);
	findLoopBackEdges(jd);

	printBasicBlocks(jd);		// for debugging
}


