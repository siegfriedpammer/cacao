/* jit/loop/graph.c - control flow graph

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

   Authors: Christopher Kruegel

   Changes: Christian Thalinger

   Contains the functions which build a list, that represents the
   control flow graph of the procedure, that is being analyzed.

   $Id: graph.c 1141 2004-06-05 23:19:24Z twisti $

*/


#include <stdio.h>
#include "jit/jit.h"
#include "jit/loop/graph.h"
#include "jit/loop/loop.h"
#include "toolbox/memory.h"


void LoopContainerInit(struct LoopContainer *lc, int i)
{
	struct LoopElement *le = DMNEW(struct LoopElement, 1);

	le->next = NULL;

	lc->parent = NULL;

	lc->tree_right = NULL;
	lc->tree_down = NULL;

	lc->exceptions = NULL;

	lc->in_degree = 0;
	le->node = lc->loop_head = i;
	le->block = &block[i];

	/* lc->nodes = (int *) malloc(sizeof(int)*block_count);
	lc->nodes[0] = i; */

	lc->nodes = le;
}
	

/*
   depthFirst() builds the control flow graph out of the intermediate code of  
   the procedure, that is to be optimized and stores the list in the global 
   variable c_dTable 
*/	        							
void depthFirst()
{
	int i;

/*	allocate memory and init gobal variables needed by function dF(int, int)	*/
  
/*  	if ((c_defnum = (int *) malloc(block_count * sizeof(int))) == NULL)		 */
/*  		c_mem_error(); */
/*  	if ((c_numPre = (int *) malloc(block_count * sizeof(int))) == NULL) */
/*  		c_mem_error(); */
/*  	if ((c_parent = (int *) malloc(block_count * sizeof(int))) == NULL) */
/*  		c_mem_error(); */
/*  	if ((c_reverse = (int *) malloc(block_count * sizeof(int))) == NULL) */
/*  		c_mem_error(); */
	
/*  	if ((c_pre = (int **) malloc(block_count * sizeof(int *))) == NULL) */
/*  		c_mem_error();  */

/*  	if ((c_dTable = (struct depthElement **) malloc(block_count * sizeof(struct depthElement *))) == NULL) */
/*  		c_mem_error(); */
	
/*  	for (i = 0; i < block_count; ++i) { */
/*  		c_defnum[i] = c_parent[i] = -1; */
/*  		c_numPre[i] = c_reverse[i] = 0; */

/*  		if ((c_pre[i] = (int *) malloc(block_count * sizeof(int))) == NULL) */
/*  			c_mem_error(); */
/*  		c_dTable[i] = NULL; */
/*  	    } */
  
	c_defnum = DMNEW(int, block_count);
	c_numPre = DMNEW(int, block_count);
	c_parent = DMNEW(int, block_count);
	c_reverse = DMNEW(int, block_count);
	c_pre = DMNEW(int *, block_count);
	c_dTable = DMNEW(struct depthElement *, block_count);
	
	for (i = 0; i < block_count; ++i) {
		c_defnum[i] = c_parent[i] = -1;
		c_numPre[i] = c_reverse[i] = 0;

		c_pre[i] = DMNEW(int, block_count);
		c_dTable[i] = NULL;
	}
  
	c_globalCount = 0;
	c_allLoops = NULL;
  
	dF(-1, 0);	/* call helper function dF that traverses basic block structure	*/
}


/*	
   dF starts from the first block of the given procedure and traverses the 
   control flow graph in a depth-first order, thereby building up the adeacency
   list c_dTable
*/ 
void dF(int from, int blockIndex)
{
	instruction *ip;
	s4 *s4ptr;
	int high, low, count;
	struct depthElement *hp;
	struct LoopContainer *tmp; 
	int cnt, *ptr;
	
	if (from >= 0) {	
/*	the current basic block has a predecessor (ie. is not the first one)		*/
/*  		if ((hp = (struct depthElement *) malloc(sizeof(struct depthElement))) == NULL) */
		/*  			c_mem_error(); */
		hp = DNEW(struct depthElement);/* create new depth element					*/

		hp->next = c_dTable[from];	/* insert values							*/
		hp->value = blockIndex;
		hp->changes = NULL;
		
		c_dTable[from] = hp;	/* insert into table							*/
	}
  
	if (from == blockIndex) {	/* insert one node loops into loop container	*/
/*  		if ((tmp = (struct LoopContainer *) malloc(sizeof(struct LoopContainer))) == NULL) */
/*  			c_mem_error(); */
		tmp = DNEW(struct LoopContainer);
		LoopContainerInit(tmp, blockIndex);
		tmp->next = c_allLoops;
		c_allLoops = tmp;
	}

#ifdef C_DEBUG
	if (blockIndex > block_count) {
		panic("DepthFirst: BlockIndex exceeded\n");
	}		
#endif

	ip = block[blockIndex].iinstr + block[blockIndex].icount -1;
										/* set ip to last instruction			*/
									
	if (c_defnum[blockIndex] == -1) {	/* current block has not been visited	*/
	    c_defnum[blockIndex] = c_globalCount;	/* update global count			*/
	    c_parent[blockIndex] = from;	/* write parent block of current one	*/
		c_reverse[c_globalCount] = blockIndex;
		++c_globalCount;
		
		if (!block[blockIndex].icount) {
										/* block does not contain instructions	*/
			dF(blockIndex, blockIndex+1);
		    }
		else { 							/* for all successors, do				*/
			switch (ip->opc) {			/* check type of last instruction		*/
			case ICMD_RETURN:
			case ICMD_IRETURN:
			case ICMD_LRETURN:
			case ICMD_FRETURN:
			case ICMD_DRETURN:
			case ICMD_ARETURN:
			case ICMD_ATHROW:
				break;					/* function returns -> end of graph		*/        
				
			case ICMD_IFEQ:
			case ICMD_IFNE:
			case ICMD_IFLT:
			case ICMD_IFGE:
			case ICMD_IFGT:
			case ICMD_IFLE:
			case ICMD_IFNULL:
			case ICMD_IFNONNULL:
			case ICMD_IF_ICMPEQ:
			case ICMD_IF_ICMPNE:
			case ICMD_IF_ICMPLT:
			case ICMD_IF_ICMPGE:
			case ICMD_IF_ICMPGT:
			case ICMD_IF_ICMPLE:
			case ICMD_IF_ACMPEQ:
			case ICMD_IF_ACMPNE:				/* branch -> check next block	*/
			   dF(blockIndex, blockIndex + 1);
			   /* fall throu */
			   
			case ICMD_GOTO:
				dF(blockIndex, block_index[ip->op1]);         
				break;							/* visit branch (goto) target	*/
				
			case ICMD_TABLESWITCH:				/* switch statement				*/
				s4ptr = ip->val.a;
				
				dF(blockIndex, block_index[*s4ptr]);	/* default branch		*/
				
				s4ptr++;
				low = *s4ptr;
				s4ptr++;
				high = *s4ptr;
				
				count = (high-low+1);
				
				while (--count >= 0) {
					s4ptr++;
					dF(blockIndex, block_index[*s4ptr]);
				    }
				break;
				
			case ICMD_LOOKUPSWITCH:				/* switch statement				*/
				s4ptr = ip->val.a;
			   
				dF(blockIndex, block_index[*s4ptr]);	/* default branch		*/
				
				++s4ptr;
				count = *s4ptr++;
				
				while (--count >= 0) {
					dF(blockIndex, block_index[s4ptr[1]]);
					s4ptr += 2;
				    }
				break;

			case ICMD_JSR:
				c_last_jump = blockIndex;
				dF(blockIndex, block_index[ip->op1]);         
				break;
				
			case ICMD_RET:
				dF(blockIndex, c_last_jump+1);
				break;
				
			default:
				dF(blockIndex, blockIndex + 1);
				break;	
			    }                         
		    }
	    } 

	for (ptr = c_pre[blockIndex], cnt = 0; cnt < c_numPre[blockIndex]; ++cnt, ++ptr)
	{
		if (*ptr == from)
			break;
	}

	if (cnt >= c_numPre[blockIndex]) {	
		c_pre[blockIndex][c_numPre[blockIndex]] = from;
		                                    /* add predeccessors to list c_pre 		*/
		c_numPre[blockIndex]++;				/* increase number of predecessors      */		
	    }
    
}

/* 
   a slightly modified version of dF(int, int) that is used to traverse the part 
   of the control graph that is not reached by normal program flow but by the 
   raising of exceptions (code of catch blocks)
*/
void dF_Exception(int from, int blockIndex)
{
	instruction *ip;
	s4 *s4ptr;
	int high, low, count;
	struct depthElement *hp;

	if (c_exceptionVisit[blockIndex] < 0)	/* has block been visited, return	*/
		c_exceptionVisit[blockIndex] = 1;
	else
		return;

	if (c_dTable[blockIndex] != NULL)		/* back to regular code section 	*/
		return;

	if (from >= 0) {		/* build exception graph (in c_exceptionGraph) 	   	*/
/*  	    if ((hp = (struct depthElement *) malloc(sizeof(struct depthElement))) == NULL) */
/*  			c_mem_error(); */
	    hp = DNEW(struct depthElement);
		hp->next = c_exceptionGraph[from];
		hp->value = blockIndex;
		hp->changes = NULL;

		c_exceptionGraph[from] = hp;
	}
	
#ifdef C_DEBUG
	if (blockIndex > block_count) {
		panic("DepthFirst: BlockIndex exceeded");
	}
#endif

	ip = block[blockIndex].iinstr + block[blockIndex].icount -1;
	
	if (!block[blockIndex].icount)
		dF_Exception(blockIndex, blockIndex+1);
	else {
		switch (ip->opc) {
		case ICMD_RETURN:
		case ICMD_IRETURN:
		case ICMD_LRETURN:
		case ICMD_FRETURN:
		case ICMD_DRETURN:
		case ICMD_ARETURN:
		case ICMD_ATHROW:
			break;                                 
		
		case ICMD_IFEQ:
		case ICMD_IFNE:
		case ICMD_IFLT:
		case ICMD_IFGE:
		case ICMD_IFGT:
		case ICMD_IFLE:
		case ICMD_IFNULL:
		case ICMD_IFNONNULL:
		case ICMD_IF_ICMPEQ:
		case ICMD_IF_ICMPNE:
		case ICMD_IF_ICMPLT:
		case ICMD_IF_ICMPGE:
		case ICMD_IF_ICMPGT:
		case ICMD_IF_ICMPLE:
		case ICMD_IF_ACMPEQ:
		case ICMD_IF_ACMPNE:
			dF_Exception(blockIndex, blockIndex + 1);
			/* fall throu */
	  
		case ICMD_GOTO:
			dF_Exception(blockIndex, block_index[ip->op1]);         
			break;
	  
		case ICMD_TABLESWITCH:
			s4ptr = ip->val.a;
			
			/* default branch */
			dF_Exception(blockIndex, block_index[*s4ptr]);
			
			s4ptr++;
			low = *s4ptr;
			s4ptr++;
			high = *s4ptr;
			
			count = (high-low+1);

			while (--count >= 0) {
				s4ptr++;
				dF_Exception(blockIndex, block_index[*s4ptr]);
			    }
			break;

		case ICMD_LOOKUPSWITCH:
			s4ptr = ip->val.a;
 
			/* default branch */
			dF_Exception(blockIndex, block_index[*s4ptr]);
			
			++s4ptr;
			count = *s4ptr++;

			while (--count >= 0) {
				dF_Exception(blockIndex, block_index[s4ptr[1]]);
				s4ptr += 2;
			    }  
			break;

		case ICMD_JSR:
			c_last_jump = blockIndex;
			dF_Exception(blockIndex, block_index[ip->op1]);         
			break;
	
		case ICMD_RET:
			dF_Exception(blockIndex, c_last_jump+1);
			break;
			
		default:
			dF_Exception(blockIndex, blockIndex + 1);
			break;	
		    }                         
        }
}


/*
  Test function -> will be removed in final release
*/
void resultPass1()
{
	int i, j;
	struct depthElement *hp;
	
	printf("\n\n****** PASS 1 ******\n\n");
	printf("Number of Nodes: %d\n\n", c_globalCount);
 
	printf("Predecessors:\n");
	for (i=0; i<block_count; ++i) {
		printf("Block %d:\t", i);
		for (j=0; j<c_numPre[i]; ++j)
			printf("%d ", c_pre[i][j]);
		printf("\n");
	}
	printf("\n");

	printf("Graph:\n");
	for (i=0; i<block_count; ++i) {
		printf("Block %d:\t", i);
		hp = c_dTable[i];
		
		while (hp != NULL) {
			printf("%d ", hp->value);
			hp = hp->next;
		    }
		printf("\n");
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
