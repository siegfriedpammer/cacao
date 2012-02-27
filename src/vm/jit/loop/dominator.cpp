#include <cstring>
#include <sstream>
#include <vector>
#include <cassert>

#include "dominator.hpp"
#include "toolbox/logging.hpp"
#include "vm/method.hpp"


void depthFirstTraversal(jitdata* jd, basicblock* block);
void printBasicBlocks(jitdata*);
void link(basicblock* v, basicblock* w);
basicblock* eval(basicblock*);
void compress(basicblock*);


void createRoot(jitdata* jd)
{
	assert(jd->basicblocks);

	// Create a new basicblock and initialize all fields that will be used.
	jd->ld->root = new basicblock;
	jd->ld->root->nr = ROOT_NUMBER;
	jd->ld->root->type = BBTYPE_STD;
	jd->ld->root->successorcount = 1;   // at least 1 arrayslot for jd->basicblocks

	// Find all exception handling basicblocks.
	for (basicblock* block = jd->basicblocks->next; block != 0; block = block->next)
	{
		if (block->type == BBTYPE_EXH)
			jd->ld->root->successorcount++;
	}

	// Allocate and fill successor array with all found basicblocks.
	jd->ld->root->successors = new basicblock*[jd->ld->root->successorcount];
	jd->ld->root->successors[0] = jd->basicblocks;
	s4 index = 1;
	for (basicblock* block = jd->basicblocks->next; block != 0; block = block->next)
	{
		if (block->type == BBTYPE_EXH)
		{
			jd->ld->root->successors[index] = block;
			index++;
		}
	}
}

/**
 * Calculates the immediate dominators of all basicblocks.
 */
void calculateDominators(jitdata* jd)
{
	assert(jd->ld->root);

	/******************
	 *     Step 1     *
	 ******************/

	// Insert an arbitrary element so that the first real item has index 1. That simplifies the implementation.
	jd->ld->vertex.push_back(0);

	assert(jd->ld->vertex.size() == 1);

	for (basicblock* block = jd->basicblocks; block != 0; block = block->next)
	{
		block->dominatorData = new DominatorData;
	}

	// jd->ld->root is not contained in the linked list jd->basicblocks.
	jd->ld->root->dominatorData = new DominatorData;
	
	depthFirstTraversal(jd, jd->ld->root);

	assert(static_cast<s4>(jd->ld->vertex.size()) == jd->ld->n + 1);
	for (s4 i = jd->ld->n; i >= 2; i--)
	{
		basicblock* w = jd->ld->vertex[i];

		/******************
		 *     Step 2     *
		 ******************/

		typedef std::vector<basicblock*>::iterator Iterator;

		for (Iterator it = w->dominatorData->pred.begin(); it != w->dominatorData->pred.end(); ++it)
		{
			basicblock* v = *it;

			basicblock* u = eval(v);
			if (u->dominatorData->semi < w->dominatorData->semi)
				w->dominatorData->semi = u->dominatorData->semi;
		}

		jd->ld->vertex[w->dominatorData->semi]->dominatorData->bucket.push_back(w);
		link(w->dominatorData->parent, w);

		/******************
		 *     Step 3     *
		 ******************/

		std::vector<basicblock*>& bucket = w->dominatorData->parent->dominatorData->bucket;
		for (Iterator it = bucket.begin(); it != bucket.end(); ++it)
		{
			basicblock* v = *it;

			basicblock* u = eval(v);
			if (u->dominatorData->semi < v->dominatorData->semi)
				v->dominatorData->dom = u;
			else
				v->dominatorData->dom = w->dominatorData->parent;
		}
		bucket.clear();
	}

	/******************
	 *     Step 4     *
	 ******************/

	for (s4 i = 2; i <= jd->ld->n; i++)
	{
		basicblock* w = jd->ld->vertex[i];

		if (w->dominatorData->dom != jd->ld->vertex[w->dominatorData->semi])
			w->dominatorData->dom = w->dominatorData->dom->dominatorData->dom;
	}
	jd->ld->root->dominatorData->dom = 0;
}

void depthFirstTraversal(jitdata* jd, basicblock* block)
{
	block->dominatorData->semi = ++jd->ld->n;
	jd->ld->vertex.push_back(block);
	block->dominatorData->label = block;

	// Check if jd->ld->vertex[jd->ld->n] == block
	assert(static_cast<s4>(jd->ld->vertex.size()) == jd->ld->n + 1);
	
	for (s4 i = 0; i < block->successorcount; i++)
	{
		basicblock* successor = block->successors[i];

		if (successor->dominatorData->semi == 0)
		{
			successor->dominatorData->parent = block;
			depthFirstTraversal(jd, successor);
		}

		successor->dominatorData->pred.push_back(block);
	}
}

void link(basicblock* v, basicblock* w)
{
	assert(v);
	assert(w);

	w->dominatorData->ancestor = v;
}

basicblock* eval(basicblock* block)
{
	assert(block);

	if (block->dominatorData->ancestor == 0)
	{
		return block;
	}
	else
	{
		compress(block);
		return block->dominatorData->label;
	}
}

void compress(basicblock* block)
{
	basicblock* ancestor = block->dominatorData->ancestor;

	assert(ancestor != 0);

	if (ancestor->dominatorData->ancestor != 0)
	{
		compress(ancestor);
		if (ancestor->dominatorData->label->dominatorData->semi < block->dominatorData->label->dominatorData->semi)
		{
			block->dominatorData->label = ancestor->dominatorData->label;
		}
		block->dominatorData->ancestor = ancestor->dominatorData->ancestor;
	}
}

/**
 * Uses the immediate dominators to build a dominator tree which can be traversed from the root to the leaves.
 */
void buildDominatorTree(jitdata* jd)
{
	for (basicblock* block = jd->basicblocks; block != 0; block = block->next)
	{
		if (block->dominatorData->dom)
			block->dominatorData->dom->dominatorData->children.push_back(block);
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

/*
void loopFoo(jitdata* jd)
{
	if (!initialized)
		init();

	if (equals(jd->m->clazz->name, classHello)
		&& (equals(jd->m->name, methodFill) || equals(jd->m->name, methodPrint)))
	{
		log_message_utf("method: ", jd->m->name);

		for (s4 i = 0; i < jd->instructioncount; i++)
		{
			if (jd->instructions[i].opc == ICMD_IASTORE
				|| jd->instructions[i].opc == ICMD_IALOAD)
			{
				std::stringstream str;
				str << "line: " << jd->instructions[i].line;
				log_text(str.str().c_str());
				
				// remove check
				jd->instructions[i].flags.bits &= ~INS_FLAG_CHECK;

				if (INSTRUCTION_MUST_CHECK(&(jd->instructions[i])))
					log_text("check!");
				else
					log_text("don't check!");
			}
		}
	}
}
*/

