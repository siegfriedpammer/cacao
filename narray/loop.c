/* loop.c **********************************************************************

        Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

        See file COPYRIGHT for information on usage and disclaimer of warranties.

        Contains the functions which use the control flow graph to do loop detection.
		The loop detection is performed according to Lengauer-Tarjan algorithm 
		that uses dominator trees (found eg. in modern compiler implementation
		by a.w. appel)

        Authors: Christopher Kruegel      EMAIL: cacao@complang.tuwien.ac.at

        Last Change: 1998/17/02

*******************************************************************************/

/*	This function allocates and initializes variables, that are used by the
	 loop detection algorithm
*/
void setup()
{
	int i;

	c_semi_dom = (int *) malloc(block_count * sizeof(int));
	c_idom = (int *) malloc(block_count * sizeof(int));
	c_same_dom = (int *) malloc(block_count * sizeof(int));
	c_numBucket = (int *) malloc(block_count * sizeof(int));
	c_ancestor = (int *) malloc(block_count * sizeof(int));
	c_contains = (int *) malloc(block_count * sizeof(int));
	c_stack = (int *) malloc(block_count * sizeof(int));

	c_bucket = (int **) malloc(block_count * sizeof(int *));
  
	for (i=0; i<block_count; ++i) {
		c_numBucket[i] = 0;
		c_stack[i] = c_ancestor[i] = c_semi_dom[i] = c_same_dom[i] = c_idom[i] = -1;
	  
		c_bucket[i] = (int *) malloc(block_count * sizeof(int));
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

/*	A helper function needed by detectLoops() that checks, whether a given 
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


/*	These stack functions are helper functions for createLoop(int, int)  
	to manage the set of nodes in the current loop.
*/
void push(int i, struct LoopContainer *lc)
{
	struct LoopElement *le = lc->nodes, *t;

	if (!c_contains[i])
	{
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


/*	This function is a helper function, that finds all nodes, that belong to 
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

/*	This function is called by higher level routines to perform the loop 
	detection and set up the c_allLoops list.
*/
void analyseGraph()
{
  setup();
  dominators();
  detectLoops();
}

/*	Test function -> will be removed in final release
*/
void resultPass2()
{
  int i, j;
  struct LoopContainer *lc = c_allLoops;
  struct LoopElement *le;
  
  printf("\n\n****** PASS 2 ******\n\n");
  
  printf("Loops:\n\n");
  
  j=0;
  while (lc != NULL) {
	  printf("Loop [%d]: ", ++j);

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



