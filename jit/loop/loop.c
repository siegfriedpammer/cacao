/* jit/loop/loop.c - array bound removal

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser,
   M. Probst, S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck,
   P. Tomsich, J. Wenninger

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Christopher Kruegel      EMAIL: cacao@complang.tuwien.ac.at

   The loop detection is performed according to Lengauer-Tarjan
   algorithm that uses dominator trees (found eg. in modern compiler
   implementation by a.w. appel)

   $Id: loop.c 665 2003-11-21 18:36:43Z jowenn $

*/


#include <stdio.h>
#include <stdlib.h>
#include "global.h"	
#include "jit/jit.h"	
#include "loop.h"
#include "graph.h"
#include "tracing.h"
#include "toolbox/loging.h"
#include "toolbox/memory.h"

/* GLOBAL VARS																*/

int c_debug_nr;                 /* a counter to number all BB with an unique    */
                                /* value                                        */

/* modified by graph.c															*/

int *c_defnum;					/* array that stores a number for each node	when*/
								/* control flow graph is traveres depth first	*/
int *c_parent;					/* for each node that array stores its parent	*/
int *c_reverse;					/* for each def number that array stores the	*/
								/* corresponding node							*/
int c_globalCount;				/* counter for def numbering					*/
int *c_numPre;					/* array that stores for each node its number	*/
								/* predecessors									*/
int **c_pre;					/* array of array that stores predecessors		*/
int c_last_jump;				/* stores the source node of the last jsr instr	*/
struct basicblock *c_last_target;      /* stores the source BB of the last jsr instr	*/

struct depthElement **c_dTable;	/* adjacency list for control flow graph		*/
struct depthElement **c_exceptionGraph;	/* adjacency list for exception graph	*/

struct LoopContainer *c_allLoops;		/* list of all loops					*/
struct LoopContainer *c_loop_root;		/* root of loop hierarchie tree			*/

int *c_exceptionVisit;			/* array that stores a flag for each node part	*/
								/* of the exception graph						*/

/* modified by loop.c															*/

int *c_semi_dom;				/* store for each node its semi dominator		*/
int *c_idom;					/* store for each node its dominator			*/
int *c_same_dom;				/* temp array to hold nodes with same dominator	*/
int *c_ancestor;				/* store for each node its ancestor with lowest	*/
								/* semi dominator								*/
int *c_numBucket;				
int **c_bucket;

int *c_contains;				/* store for each node whether it's part of loop*/
int *c_stack;					/* a simple stack as array						*/
int c_stackPointer;				/* stackpointer									*/


/* modified by analyze.c														*/

struct LoopContainer *root;     /* the root pointer for the hierarchie tree of  */
                                /* all loops in that procedure                  */

int c_needed_instr;				/* number of instructions that have to be		*/
								/* inserted before loop header to make sure		*/
								/* array optimization is legal					*/
int c_rs_needed_instr;			/* number of instructions needed to load the	*/
								/* value ofthe right side of the loop condition	*/
int *c_nestedLoops;				/* store for each node the header node of the	*/
								/* loop this node belongs to, -1 for none		*/
int *c_hierarchie;              /* store a loop hierarchie                      */
int *c_toVisit;					/* set for each node that is part of the loop	*/

int *c_current_loop;			/* for each node:                               */
								/* store 0:	node is not part of loop			*/
								/* store 1:	node is loop header					*/
								/* store 2:	node is in loop but not part of any	*/
								/*			nested loop                         */
								/* store 3:	node is part of nested loop			*/

int c_current_head;				/* store number of node that is header of loop	*/
int *c_var_modified;			/* store for each local variable whether its	*/
								/* value is changed in the loop					*/

struct Trace *c_rightside;		/* right side of loop condition					*/
struct Constraint **c_constraints;
								/* array that stores for each variable a list	*/
								/* static tests (constraints) that have to be	*/
								/* performed before loop entry					*/
								/* IMPORTANT: c_constraints[maxlocals] stores	*/
								/*			  the tests for constants and the	*/
								/*			  right side of loop condition		*/
	
struct LoopVar *c_loopvars;		/* a list of all intersting variables of the	*/
								/* current loop (variables that are modified or	*/
								/* used as array index							*/

struct basicblock *c_first_block_copied; /* pointer to the first block, that is copied */
                                  /* during loop duplication                    */

struct basicblock *c_last_block_copied;  /* last block, that is copied during loop     */
                                  /* duplication                                */

int *c_null_check;              /* array to store for local vars, whether they  */
                                /* need to be checked against the null reference*/
                                /* in the loop head                             */

bool c_needs_redirection;       /* if a loop header is inserted as first block  */
                                /* into the global BB list, this is set to true */
                                 
struct basicblock *c_newstart;         /* if a loop header is inserted as first block  */
                                /* into the gloal BB list, this pointer is the  */
                                /* new start                                    */
int c_old_xtablelength;         /* used to store the original tablelength       */

/* set debug mode																*/
#define C_DEBUG


/* declare statistic variables													*/
#ifdef STATISTICS

int c_stat_num_loops;			/* number of loops								*/

/* statistics per loop															*/
int c_stat_array_accesses;		/* number of array accesses						*/

int c_stat_full_opt;			/* number of fully optimized accesses			*/
int c_stat_no_opt;				/* number of not optimized accesses				*/
int c_stat_lower_opt;			/* number of accesses where check against zero	*/
								/* is removed									*/
int c_stat_upper_opt;			/* number of accesses where check against array	*/
								/* lengh is removed								*/
int c_stat_or;					/* set if optimization is cancelled because of	*/
								/* or in loop condition							*/
int c_stat_exception;			/* set if optimization is cancelled because of	*/
								/* index var modified in catch block			*/

/* statistics per procedure														*/
int c_stat_sum_accesses;		/* number of array accesses						*/

int c_stat_sum_full;			/* number of fully optimized accesses			*/
int c_stat_sum_no;				/* number of not optimized accesses				*/
int c_stat_sum_lower;			/* number of accesses where check against zero	*/
								/* is removed									*/
int c_stat_sum_upper;			/* number of accesses where check against array	*/
								/* lengh is removed								*/
int c_stat_sum_or;				/* set if optimization is cancelled because of	*/
								/* or in loop condition							*/
int c_stat_sum_exception;		/* set if optimization is cancelled because of	*/

#endif


/*	
   This function allocates and initializes variables, that are used by the
   loop detection algorithm
*/
void setup()
{
	int i;

	c_semi_dom = DMNEW(int, block_count);
	c_idom = DMNEW(int, block_count);
	c_same_dom = DMNEW(int, block_count);
	c_numBucket = DMNEW(int, block_count);
	c_ancestor = DMNEW(int, block_count);
	c_contains = DMNEW(int, block_count);
	c_stack = DMNEW(int, block_count);
	c_bucket = DMNEW(int*, block_count);
  
	for (i = 0; i < block_count; ++i) {
		c_numBucket[i] = 0;
		c_stack[i] = c_ancestor[i] = c_semi_dom[i] = c_same_dom[i] = c_idom[i] = -1;
	  
		c_bucket[i] = DMNEW(int, block_count);
	}
}


/*	This function is a helper function for dominators and has to find the 
	ancestor of the node v in the control graph, which semi-dominator has the  
	lowest def-num.
*/
int findLowAnc(int v)
{
	int u = v;			/* u is the node which has the current lowest semi-dom	*/
  
	while (c_ancestor[v] != -1) {	/* as long as v has an ancestor, continue	*/
		if (c_defnum[c_semi_dom[v]] < c_defnum[c_semi_dom[u]])	
									/* if v's semi-dom is smaller				*/
			u = v;					/* it gets the new current node u			*/
		v = c_ancestor[v];			/* climb one step up in the tree			*/
		}
	return u;			/* return node with the lowest semi-dominator def-num	*/
}


/*	This function builds the dominator tree out of a given control flow graph and 
	stores its results in c_idom[]. It first calculates the number of possible
	dominators in c_bucket and eventually determines the single dominator in a 
	final pass.
*/
void dominators()
{
	int i, j, semi, s, n, v, actual, p, y;
  
	for (n=(c_globalCount-1); n>0; --n) {	/* for all nodes (except last), do	*/
		actual = c_reverse[n];
		semi = p = c_parent[actual];		
	
		/* for all predecessors of current node, do								*/
		for (i=0; i<c_numPre[actual]; ++i) {
			v = c_pre[actual][i];
      
			if (c_defnum[v] <= c_defnum[actual])
				s = v;			/* if predecessor has lower def-num	than node	*/
								/* it becomes candidate for semi dominator		*/
			else
				s = c_semi_dom[findLowAnc(v)];
								/* else the semi-dominator of it's ancestor		*/
								/* with lowest def-num becomes candidate		*/
			
			if (c_defnum[s] < c_defnum[semi])
				semi = s;		/* if the def-num of the new candidate is lower */
								/* than old one, it gets new semi dominator		*/
			}
    
		/* write semi dominator -> according to	SEMIDOMINATOR THEOREM			*/
		c_semi_dom[actual] = semi;				
		c_ancestor[actual] = p;					
    
		c_bucket[semi][c_numBucket[semi]] = actual;
		c_numBucket[semi]++;	/* defer calculation of dominator to final pass */
      

		/* first clause of DOMINATOR THEOREM, try to find dominator now			*/
		for (j=0; j<c_numBucket[p]; ++j) {
			v = c_bucket[p][j];
			y = findLowAnc(v);
      
			if (c_semi_dom[y] == c_semi_dom[v])	
				c_idom[v] = p;			/* if y's dominator is already known	*/
										/* found it and write to c_idom			*/
			else
				c_same_dom[v] = y;		/* wait till final pass					*/
			}
		
		c_numBucket[p] = 0;
		}
  
	/* final pass to get missing dominators ->second clause of DOMINATOR THEORM	*/
	for (j=1; j<(c_globalCount-1); ++j) {		
		if (c_same_dom[c_reverse[j]] != -1)	
			c_idom[c_reverse[j]] = c_idom[c_same_dom[c_reverse[j]]];
		}
}


/*	
   A helper function needed by detectLoops() that checks, whether a given 
   connection between two nodes in the control flow graph is possibly part
   of a loop (is a backEdge).
*/
int isBackEdge(int from, int to)
{
	int tmp = c_idom[to];	/* speed optimization: if the to-node is dominated	*/
	while (tmp != -1) {		/* by the from node as it is most of the time, 		*/
		if (tmp == from)	/* there is no backEdge								*/
			return 0;
		tmp = c_idom[tmp];
		}

	tmp = c_idom[from];		/* if from-node doesn't dominate to-node, we have	*/
	while (tmp != -1) {		/* to climb all the way up from the from-node to	*/
		if (tmp == to)		/* the top to check, whether it is dominated by to	*/
			return 1;		/* if so, return a backedge							*/
		tmp = c_idom[tmp];
		}

	return 0;				/* else, there is no backedge						*/
}


/*	
   These stack functions are helper functions for createLoop(int, int)  
   to manage the set of nodes in the current loop.
*/
void push(int i, struct LoopContainer *lc)
{
	struct LoopElement *le = lc->nodes, *t;

	if (!c_contains[i])	{
		t = DMNEW(struct LoopElement, 1);
		
		t->node = i;
		t->block = &block[i];

		c_contains[i] = 1;

		if (i < le->node)
		{
			t->next = lc->nodes;
			lc->nodes = t;
		}
		else
		{
			while ((le->next != NULL) && (le->next->node < i))
				le = le->next;

			t->next = le->next;
			le->next = t;
		}

		c_stack[c_stackPointer++] = i;
	}
}


int pop()
{
	return (c_stack[--c_stackPointer]);
}


int isFull()
{
	return (c_stackPointer);
}


/*	
   This function is a helper function, that finds all nodes, that belong to 
   the loop with a known header node and a member node of the loop (and a 
   back edge between these two nodes).
*/
void createLoop(int header, int member)
{
	int i, nextMember;

	struct LoopContainer *currentLoop = (struct LoopContainer *) malloc(sizeof(struct LoopContainer));
	LoopContainerInit(currentLoop, header);		/* set up loop structure		*/
	
	for (i=0; i<block_count; ++i)
		c_contains[i] = 0;
	c_contains[header] = 1;

	c_stackPointer = 0;				/* init stack with first node of the loop	*/
	push(member, currentLoop);

	while (isFull()) {				/* while there are still unvisited nodes	*/
		nextMember = pop();
		
		/* push all predecessors, while they are not equal to loop header		*/
		for (i=0; i<c_numPre[nextMember]; ++i)			
			push(c_pre[nextMember][i], currentLoop);		
		}

	currentLoop->next = c_allLoops;
	c_allLoops = currentLoop;
}


/*	After all dominators have been calculated, the loops can be detected and
	 added to the global list c_allLoops.
*/
void detectLoops()
{
	int i;
	struct depthElement *h;
	
	/* for all edges in the control flow graph do								*/
	for (i=0; i<block_count; ++i) {			
		h = c_dTable[i];

		while (h != NULL) {
			/* if it's a backedge, than add a new loop to list					*/
			if (isBackEdge(i, h->value))	 
				createLoop(h->value, i);
			h = h->next;
			}
		}
}


/*	
   This function is called by higher level routines to perform the loop 
   detection and set up the c_allLoops list.
*/
void analyseGraph()
{
  setup();
  dominators();
  detectLoops();
}


/*
   Test function -> will be removed in final release
*/
void resultPass2()
{
  int i;
  struct LoopContainer *lc = c_allLoops;
  struct LoopElement *le;
  
  printf("\n\n****** PASS 2 ******\n\n");
  
  printf("Loops:\n\n");
  
  i = 0;
  while (lc != NULL) {
	  printf("Loop [%d]: ", ++i);

  	  le = lc->nodes;
	  while (le != NULL) {
	    printf("%d ", le->node);
		printf("\n");
		le = le->next;
	  }

	  lc = lc->next;
  }

  printf("\n");

}


void c_mem_error()
{
  panic("C_ERROR: Not enough memeory");
} 


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
