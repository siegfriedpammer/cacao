/* jit/loop/loop.h - array bound removal header

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

   $Id: loop.h 557 2003-11-02 22:51:59Z twisti $

*/


#ifndef _LOOP_H
#define _LOOP_H

#include "jit.h"


/*	Different types for struct Trace										*/
#define TRACE_UNKNOWN 0			/* unknown									*/
#define TRACE_ICONST  1			/* integer constant value					*/
#define TRACE_ALENGTH 2			/* array length value						*/
#define TRACE_IVAR    3			/* integer variable reference				*/
#define TRACE_AVAR    4			/* object (array) reference					*/

/*	The ways a variable can be used in a loop								*/
#define ARRAY_INDEX	  0			/* var used as array index					*/
#define VAR_MOD		  1			/* var changes its value					*/

/*	The way, integer variables change their values							*/
#define D_UP		  0			/* var is only increased					*/
#define D_DOWN		  1			/* var is only decreased					*/
#define D_UNKNOWN	  2			/* not known								*/

/*	The different types of operators in loop conditions						*/
#define OP_EQ		  0			/* operator:	==							*/
#define OP_LT         1			/* operator:	<							*/
#define OP_GE	      2			/* operator:	>=							*/
#define OP_UNKNOWN	  3			/* operator:	unknown						*/

/*	Possible types of static tests (constraints) in	struct Constraint		*/
 
#define TEST_ZERO			0	/* check variable against const. lower bound*/
#define TEST_ALENGTH		1	/* check variable against array length		*/
#define TEST_CONST_ZERO		2	/* check constant against const. lower bound*/
#define TEST_CONST_ALENGTH	3	/* check variable against array length		*/
#define TEST_UNMOD_ZERO		4	/* check var. that is constant in loop against*/
								/* constant lower bound						*/
#define TEST_UNMOD_ALENGTH	5	/* check var. that is constant in loop against*/
								/* array length								*/
#define TEST_RS_ZERO		6	/* check constant part of loop condition against*/
								/* constant lower bound							*/
#define TEST_RS_ALENGTH		7	/* check constant part of loop condition against*/
								/* array length									*/

/*	Possible types of bound check optimizations									*/
#define OPT_UNCHECKED	0		/* access not checked yet - first visit			*/
#define OPT_NONE		1		/* no optimization								*/
#define OPT_FULL		2		/* fully remove bound check						*/
#define OPT_LOWER		3		/* remove check againt zero						*/
#define OPT_UPPER		4		/* remove check against array length			*/

/*	The different ways, remove_boundcheck(.) can be called						*/ 
#define BOUNDCHECK_REGULAR	0	/* perform regular optimization					*/
#define BOUNDCHECK_SPECIAL	1	/* only optimize header node - and ignore		*/
								/* information from loop condition				*/

#define LOOP_PART       0x1     /* a flag that marks a BB part of a loop        */
#define HANDLER_PART    0x2     /* a flag that marks a BB part of ex-handler    */
#define HANDLER_VISITED 0x4     /* flag to prevent loop if copying catch blocks */



/*	This struct records information about interesting vars (vars that are modified
	or used as an array index in loops.
*/
struct LoopVar {
	int value;					/* reference to array of local variables		*/
	int modified;				/* set if value of var is changed				*/
	int index;					/* set if var is used as array index			*/
	int static_l;				/* var is never decremented -> static lower		*/
								/* bound possible								*/
	int static_u;				/* var is never incremented -> static upper		*/
								/* bound possible								*/
	int dynamic_l;
	int dynamic_l_v;			/* variable is left side of loop condition in	*/
								/* variable + dynamic_l >= right side			*/
	int dynamic_u;
	int dynamic_u_v;			/* variable is left side of loop condition in	*/
								/* variable + dynamic_u < right side			*/
	struct LoopVar *next;		/* list pointer									*/
};


/*	This struct records the needed static test of variables before loop entry.	*/
struct Constraint {
	int type;					/* type of test to perform						*/
	int arrayRef;				/* array reference involved in test (if any)	*/
	int varRef;					/* which variable to test (if involved)			*/
	int constant;				/* which constant to test (if involved)			*/
	struct Constraint *next;	/* list pointer									*/
};


/* This structure is used to record variables that change their value in loops.	*/
struct Changes {
	int var;					/* variable involved							*/
	int lower_bound;			/* a minimum lower bound that is guaranteed		*/
	int upper_bound;			/* a maximum upper bound that is guaranteed		*/
								/* IMPORTANT: if lower_bound > upper_bound		*/
								/* there are no	guarantees at all				*/
};


/*	This struct is used to build the control flow graph and stores the variable	
	changes at the beginning of each basic block.
*/
struct depthElement {
	int value;					/* number of successor of this block			*/
	struct depthElement *next;	/* list pointer									*/
	struct Changes **changes;	/* pointer to array of variable changes			*/
};


/*	Used to build a list of all basicblock, the loop consists of				
*/
struct LoopElement {
	int node;
	basicblock	*block;			
	struct LoopElement *next;
};


/*
   This structure stores informations about a single loop
*/
struct LoopContainer {
	int toOpt;							/* does this loop need optimization		*/
	struct LoopElement *nodes;          /* list of BBs this loop consists of    */
	int loop_head;                      
	int in_degree;                      /* needed to topological sort loops to  */
	                                    /* get the order of optimizing them     */
	struct LoopContainer *next;			/* list pointer							*/
	struct LoopContainer *parent;		/* points to parent loop, if this BB    */
										/* is head of a loop					*/
	struct LoopContainer *tree_right;   /* used for tree hierarchie of loops    */
	struct LoopContainer *tree_down;
	xtable *exceptions;                 /* list of exception in that loop       */
};


/* global variables */
extern int c_debug_nr;
extern int *c_defnum;
extern int *c_parent;
extern int *c_reverse;
extern int c_globalCount;
extern int *c_numPre;
extern int **c_pre;
extern int c_last_jump;
extern basicblock *c_last_target;
extern struct depthElement **c_dTable;
extern struct depthElement **c_exceptionGraph;
extern struct LoopContainer *c_allLoops;
extern struct LoopContainer *c_loop_root;
extern int *c_exceptionVisit;


/* global loop variables */
extern int *c_semi_dom;
extern int *c_idom;
extern int *c_same_dom;
extern int *c_ancestor;
extern int *c_numBucket;				
extern int **c_bucket;
extern int *c_contains;
extern int *c_stack;
extern int c_stackPointer;


/* global analyze variables	*/
extern struct LoopContainer *root;
extern int c_needed_instr;
extern int c_rs_needed_instr;
extern int *c_nestedLoops;
extern int *c_hierarchie;
extern int *c_toVisit;
extern int *c_current_loop;
extern int c_current_head;
extern int *c_var_modified;
extern struct Trace *c_rightside;
extern struct Constraint **c_constraints;
extern struct LoopVar *c_loopvars;
extern basicblock *c_first_block_copied;
extern basicblock *c_last_block_copied;
extern int *c_null_check;
extern bool c_needs_redirection;
extern basicblock *c_newstart;
extern int c_old_xtablelength;


/* global statistic variables */
#ifdef STATISTICS

extern int c_stat_num_loops;
extern int c_stat_array_accesses;
extern int c_stat_full_opt;
extern int c_stat_no_opt;
extern int c_stat_lower_opt;
extern int c_stat_upper_opt;
extern int c_stat_or;
extern int c_stat_exception;
extern int c_stat_sum_accesses;
extern int c_stat_sum_full;
extern int c_stat_sum_no;
extern int c_stat_sum_lower;
extern int c_stat_sum_upper;
extern int c_stat_sum_or;
extern int c_stat_sum_exception;

#endif


/* function prototypes */
void analyseGraph();
void c_mem_error();

#endif /* _LOOP_H */


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

