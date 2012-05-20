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
		block->ld = new BasicblockLoopData;
	}

	// jd->ld->root is not contained in the linked list jd->basicblocks.
	jd->ld->root->ld = new BasicblockLoopData;
	
	depthFirstTraversal(jd, jd->ld->root);

	assert(static_cast<s4>(jd->ld->vertex.size()) == jd->ld->n + 1);
	for (s4 i = jd->ld->n; i >= 2; i--)
	{
		basicblock* w = jd->ld->vertex[i];

		/******************
		 *     Step 2     *
		 ******************/

		typedef std::vector<basicblock*>::iterator Iterator;

		for (Iterator it = w->ld->pred.begin(); it != w->ld->pred.end(); ++it)
		{
			basicblock* v = *it;

			basicblock* u = eval(v);
			if (u->ld->semi < w->ld->semi)
				w->ld->semi = u->ld->semi;
		}

		jd->ld->vertex[w->ld->semi]->ld->bucket.push_back(w);
		link(w->ld->parent, w);

		/******************
		 *     Step 3     *
		 ******************/

		std::vector<basicblock*>& bucket = w->ld->parent->ld->bucket;
		for (Iterator it = bucket.begin(); it != bucket.end(); ++it)
		{
			basicblock* v = *it;

			basicblock* u = eval(v);
			if (u->ld->semi < v->ld->semi)
				v->ld->dom = u;
			else
				v->ld->dom = w->ld->parent;
		}
		bucket.clear();
	}

	/******************
	 *     Step 4     *
	 ******************/

	for (s4 i = 2; i <= jd->ld->n; i++)
	{
		basicblock* w = jd->ld->vertex[i];

		if (w->ld->dom != jd->ld->vertex[w->ld->semi])
			w->ld->dom = w->ld->dom->ld->dom;
	}
	jd->ld->root->ld->dom = 0;
}

void depthFirstTraversal(jitdata* jd, basicblock* block)
{
	block->ld->semi = ++jd->ld->n;
	jd->ld->vertex.push_back(block);
	block->ld->label = block;

	// Check if jd->ld->vertex[jd->ld->n] == block
	assert(static_cast<s4>(jd->ld->vertex.size()) == jd->ld->n + 1);
	
	for (s4 i = 0; i < block->successorcount; i++)
	{
		basicblock* successor = block->successors[i];

		if (successor->ld->semi == 0)   // visited the first time?
		{
			successor->ld->parent = block;
			depthFirstTraversal(jd, successor);
		}
		else   // back edge found
		{
			jd->ld->depthBackEdges.push_back(Edge(block, successor));
		}

		successor->ld->pred.push_back(block);
	}
}

void link(basicblock* v, basicblock* w)
{
	assert(v);
	assert(w);

	w->ld->ancestor = v;
}

basicblock* eval(basicblock* block)
{
	assert(block);

	if (block->ld->ancestor == 0)
	{
		return block;
	}
	else
	{
		compress(block);
		return block->ld->label;
	}
}

void compress(basicblock* block)
{
	basicblock* ancestor = block->ld->ancestor;

	assert(ancestor != 0);

	if (ancestor->ld->ancestor != 0)
	{
		compress(ancestor);
		if (ancestor->ld->label->ld->semi < block->ld->label->ld->semi)
		{
			block->ld->label = ancestor->ld->label;
		}
		block->ld->ancestor = ancestor->ld->ancestor;
	}
}

/**
 * Uses the immediate dominators to build a dominator tree which can be traversed from the root to the leaves.
 */
void buildDominatorTree(jitdata* jd)
{
	for (basicblock* block = jd->basicblocks; block != 0; block = block->next)
	{
		if (block->ld->dom)
		{
			std::vector<basicblock*>& children = block->ld->dom->ld->children;
			
			// Every basicblock has a pointer to the next sibling in the dominator tree.
			if (!children.empty())
				children.back()->ld->nextSibling = block;

			children.push_back(block);
		}
	}
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

