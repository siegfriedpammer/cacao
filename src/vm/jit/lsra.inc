/* jit/lsra.inc - linear scan register allocator

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004
   Institut f. Computersprachen, TU Wien
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser, M. Probst,
   S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich,
   J. Wenninger, C. Ullrich

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

   Authors: Christian Ullrich

   $Id: lsra.inc 2008 2005-03-07 08:43:08Z christian $

*/
#include <stdio.h>
#include <stdlib.h>

#include "mm/memory.h"
#include "vm/options.h"
#include "vm/jit/lsra.h"
#include "vm/jit/reg.h"
#include "vm/statistics.h"

/* #include "string.h" */


#define MAX_TMP_REG_COUNT 100
#define MAX_SAV_REG_COUNT 100


#ifdef LSRA_TESTLT
#define VS 200
#define MAX_TMP_REG_COUNT 0
#define MAX_SAV_REG_COUNT 0
#endif

#define PASSING_THROUGH_LT -2  /* is used as instruction index in *i_list of a lifetime, which is a "passthrough" stackslot */


bool lsra(methodinfo *m, codegendata *cd, registerdata *rd, t_inlining_globals *id)
{

	lsradata *ls;
#if defined(STATISTICS)
	int *locals_start,*locals_end;
	struct lifetime *lt;
	int i,j;
#endif	


#ifdef LSRA_DEBUG
	int b_index;
	stackptr in,out;
	int      ind, outd;


	while (b_index < m->basicblockcount ) {

		if (m->basicblocks[b_index].flags >= BBREACHED) {

			in=m->basicblocks[b_index].instack;
			ind=m->basicblocks[b_index].indepth;
			for (;ind != 0;in=in->prev, ind--) {
				if (in->varkind == ARGVAR) printf("ARGVAR in instack: \n");
				if (in->varkind == LOCALVAR) printf("LOCALVAR in instack\n");
			}
			out=m->basicblocks[b_index].outstack;
			outd=m->basicblocks[b_index].outdepth;
			for (;outd != 0;out=out->prev, outd--) {
				if (out->varkind == ARGVAR) printf("ARGVAR in outstack\n");
				if (out->varkind == LOCALVAR) printf("LOCALVAR in outstack\n");
			}
		}
			b_index++;
	}
#endif

#if defined(LSRA_DEBUG) || defined(LSRA_DUMP_LOOPDATA)|| defined(LSRA_TESTLT)
	char name[1256], name1[1256];

	utf_sprint(name, m->class->name);
	utf_sprint(name1, m->name);
	strcat(name, ".");
	strcat(name, name1);
	utf_sprint(name1, m->descriptor);
	strcat(name, name1);


	printf("/******************************************************/\n");
	printf("LSRA Start for %s\n", name); 

	if (strcmp(name,"java/util/AbstractCollection.isEmpty()Z")==0) {
		printf("-------------------\n");
	}
	if (m->isleafmethod)
		printf("**Leafmethod**\n");
#endif

	ls=DNEW(lsradata);
	lsra_init(m, cd, id, ls);
	if (!lsra_setup(m, cd, rd, ls))
		return false;


#if defined(STATISTICS)
	/* find conflicts between locals for statistics */
	if (opt_stat) {
		locals_start = DMNEW(int, cd->maxlocals);
		locals_end   = DMNEW(int, cd->maxlocals);

		for (lt = ls->lifetimes; lt != NULL; lt=lt->next) {
			if (lt->v_index >= 0) { /* -> local var */
				locals_start[lt->v_index] = lt->i_start;
				locals_end[lt->v_index]   = lt->i_end;
			}
		}
		for (i=0; i < cd->maxlocals; i++)
			for (j=i+1; j < cd->maxlocals; j++)
				if ( !((locals_end[i] < locals_start[j]) || (locals_end[j]<locals_start[i])) )
					count_locals_conflicts += 2;
	 }
#endif	

	/* Run LSRA */
	lsra_main(m, ls, rd, cd);

	return true;
}

void lsra_DFS_(lsradata *ls, bool *mark, int bblock, int *n) {
	struct _list *p;

	if (!mark[bblock])
	{
		mark[bblock] = true;
		for (p=ls->pred[bblock]; p != NULL; p=p->next)
			lsra_DFS_(ls, mark, p->value, n);
		ls->sorted[*n]=bblock;
		*n=*n-1;
	}
}

void lsra_DFS(methodinfo *m, lsradata *ls) {
	bool *mark;
	int i,n;

	mark=DMNEW(bool, m->basicblockcount);
	n=m->basicblockcount;

	for (i=0; i <= m->basicblockcount; i++) mark[i]=false;
   
	lsra_DFS_(ls, mark, m->basicblockcount, &n);
	
}

void lsra_DFS_2(methodinfo *m, lsradata *ls) {
	int *stack;
	int *visited;
	int stack_top;
	bool not_finished;
	int i,p;
    struct _list *succ;

	stack = DMNEW( int, m->basicblockcount + 1);
	visited = DMNEW( bool, m->basicblockcount + 1);
	for (i = 0; i <= m->basicblockcount; i++) {
		visited[i] = 0;
		ls->sorted[i]=-1;
		ls->sorted_rev[i]=-1;
	}

    stack[0] = 0; /* start with Block 0 */
	stack_top = 1;
	visited[0] = ls->num_pred[0]; /* Start Block is handled right and can be put in sorted */
	p = m->basicblockcount;
	not_finished = true;
	while (not_finished) {
		while (stack_top != 0) {
			stack_top--;
			i = stack[stack_top];


			ls->sorted[p]=i;
			p--;
			for (succ = ls->succ[i]; succ != NULL; succ = succ->next) {
				visited[succ->value]++;
				if (visited[succ->value] == ls->num_pred[succ->value]) {
					/* push the node on the stack, only if all ancestors have been visited */
					stack[stack_top] = succ->value;
					stack_top++;
				}
			}
		}
		not_finished = false;
		for (i=1; i <= m->basicblockcount; i++) {
			/* search for visited blocks, which have not reached the c_numPre */
			/* and put them on the stack -> happens with backedges */
			if ((visited[i] != 0) && (visited[i] < ls->num_pred[i])) {
				stack[stack_top] = i;
				stack_top++;
				visited[i] = ls->num_pred[i];
				not_finished=true;
				break;
			}
		}
	}
}

void lsra_get_backedges( methodinfo *m, lsradata *ls) {
	struct _list **next, *s;
	struct _backedge *n;
	int i,j;
	/* todofirst remove artificial end basicblock from ls->sorted, succ and pred */
    j=-1;
	for (i=0; i < m->basicblockcount; i++) {
		for (next=&(ls->succ[i]); *next != NULL; next=&((*next)->next)) {
			if ( (*next)->value == m->basicblockcount ) { /* artificial end bb found */
				*next = (*next)->next;
				if (*next == NULL) break;
			}
		}
		for (next=&(ls->pred[i]); *next != NULL; next=&((*next)->next)) {
			if ( (*next)->value == m->basicblockcount ) { /* artificial end bb found */
				*next = (*next)->next;
				if (*next == NULL) break;
			}
		}
		if (j==-1)
			if (ls->sorted[i] == m->basicblockcount) j=i;
  	}
	if (j!=-1)
		for (i=j+1; i <= m->basicblockcount; i++)
			ls->sorted[i-1]=ls->sorted[i];
	for (i=0; i < m->basicblockcount; i++)
		if (ls->sorted[i] != -1) 
			ls->sorted_rev[ls->sorted[i]]=i;
	/* now look for backedges */
	ls->backedge_count = 0;
	for(i=0; i < m->basicblockcount; i++) {
		if (ls->sorted[i] != -1)
			for(s=ls->succ[ls->sorted[i]]; s != NULL; s=s->next) {
				if (i < ls->sorted_rev[s->value]) {
					n=DNEW(struct _backedge);
					n->start = max(i, ls->sorted_rev[s->value]);
					n->end = min(i, ls->sorted_rev[s->value]);
					n->next = ls->_backedges;
					ls->_backedges = n;
					ls->backedge_count++;
/* 					printf("Backedge: %i %i\n", ls->sorted[i], s->value); */
				}
			}
	}
	/* put ls->_backedges in ls->backedge array */
	ls->backedge = DMNEW(struct _backedge *, ls->backedge_count);
	for (n=ls->_backedges, i=0; n != NULL; n=n->next, i++)
		ls->backedge[i] = n;
	/* union backedges? */
	/* - sort backedge by increasing start: */
/* 	printf("unsorted: \n"); */
/* 	for (i=0; i < ls->backedge_count; i++) */
/* 		printf("Backedge: %i - %i, %i - %i\n",ls->sorted[ls->backedge[i]->start],ls->sorted[ls->backedge[i]->end],ls->backedge[i]->start, ls->backedge[i]->end); */
	for (i=0; i < ls->backedge_count; i++)
		for (j=i+1; j < ls->backedge_count; j++) 
			if (ls->backedge[i]->start < ls->backedge[j]->start) { /* -> swap */
				n=ls->backedge[i];
				ls->backedge[i]=ls->backedge[j];
				ls->backedge[j]=n;
			}
/* 	printf("sorted: \n"); */
/* 	for (i=0; i < ls->backedge_count; i++) */
/* 		printf("Backedge: %i - %i, %i - %i\n",ls->sorted[ls->backedge[i]->start],ls->sorted[ls->backedge[i]->end],ls->backedge[i]->start, ls->backedge[i]->end); */
	/* - merge overlapping backedges */
	for (i=0; i < ls->backedge_count-1; i++)
		if (ls->backedge[i]->end <= ls->backedge[i+1]->start) {/* overlapping -> merge */
			ls->backedge[i+1]->start = ls->backedge[i]->start;
			ls->backedge[i+1]->end = min (ls->backedge[i+1]->end, ls->backedge[i]->end);
			ls->backedge[i] = NULL;
		}
/* 	printf("merged: \n"); */
/* 	for (i=0; i < ls->backedge_count; i++) */
/* 		if (ls->backedge[i] != NULL) */
/* 			printf("Backedge: %i - %i, %i - %i\n",ls->sorted[ls->backedge[i]->start],ls->sorted[ls->backedge[i]->end],ls->backedge[i]->start, ls->backedge[i]->end); */
	/* - remove backedge[] == NULL from array */

	for ( j = ls->backedge_count -1; ((j>=0) && ( ls->backedge[j] == NULL)); j--);
	i=j;
	while (i >= 0) {
		if (ls->backedge[i] == NULL) { /* swap be[i] mit be[j] */
			n=ls->backedge[j];
			ls->backedge[j] = ls->backedge[i];
			ls->backedge[i] = n;
			i--;
			j--;
			ls->backedge_count--;
		} else i--;
	}
#ifdef LSRA_DEBUG
	printf("ready: \n");
	for (i=0; i < ls->backedge_count; i++)
		printf("Backedge: %i - %i, %i - %i\n",ls->sorted[ls->backedge[i]->start],ls->sorted[ls->backedge[i]->end],ls->backedge[i]->start, ls->backedge[i]->end);
#endif
}

void lsra_add_cfg( methodinfo *m, lsradata *ls, int from, int to, bool sbr) {
	struct _list *n;
	if (!sbr) {
		n=DNEW(struct _list);

		for (;(to < m->basicblockcount) && (m->basicblocks[to].flags < BBREACHED); to++);
			
		n->value=to;
		n->next=ls->succ[from];
		ls->succ[from]=n;

		n=DNEW(struct _list);
		n->value=from;
		n->next=ls->pred[to];
		ls->pred[to]=n;
		ls->num_pred[to]++;
	}
}
void lsra_make_cfg(methodinfo *m, lsradata *ls)
{
	instruction *ip;
	s4 *s4ptr;
	int high, low, count;
	int b_index;

	b_index=0;
	while (b_index < m->basicblockcount ) {
		if (m->basicblocks[b_index].flags >= BBREACHED) {
			ip = m->basicblocks[b_index].iinstr + m->basicblocks[b_index].icount -1;
			/* set ip to last instruction			*/
									
			if (m->basicblocks[b_index].icount) {
				/* block contains instructions	*/
				switch (ip->opc) {			/* check type of last instruction		*/
				case ICMD_RETURN:
				case ICMD_IRETURN:
				case ICMD_LRETURN:
				case ICMD_FRETURN:
				case ICMD_DRETURN:
				case ICMD_ARETURN:

				case ICMD_ATHROW:
					lsra_add_cfg(m, ls, b_index, m->basicblockcount, false);
					ls->end_bb=b_index;
					break;					/* function returns -> end of graph		*/        
				
				case ICMD_IFNULL:
				case ICMD_IFNONNULL:
				case ICMD_IFEQ:
				case ICMD_IFNE:
				case ICMD_IFLT:
				case ICMD_IFGE:
				case ICMD_IFGT:
				case ICMD_IFLE:
				case ICMD_IF_LEQ:
				case ICMD_IF_LNE:
				case ICMD_IF_LLT:
				case ICMD_IF_LGE:
				case ICMD_IF_LGT:
				case ICMD_IF_LLE:
				case ICMD_IF_ICMPEQ:
				case ICMD_IF_ICMPNE:
				case ICMD_IF_ICMPLT:
				case ICMD_IF_ICMPGE:
				case ICMD_IF_ICMPGT:
				case ICMD_IF_ICMPLE:
				case ICMD_IF_LCMPEQ:
				case ICMD_IF_LCMPNE:
				case ICMD_IF_LCMPLT:
				case ICMD_IF_LCMPGE:
				case ICMD_IF_LCMPGT:
				case ICMD_IF_LCMPLE:
				case ICMD_IF_ACMPEQ:
				case ICMD_IF_ACMPNE:				/* branch -> check next block	*/
					lsra_add_cfg(m, ls, b_index, b_index+1, false);
					/* fall throu */
			   
				case ICMD_GOTO:
					lsra_add_cfg(m, ls, b_index,  m->basicblockindex[ip->op1],false);
					break;							/* visit branch (goto) target	*/
				
				case ICMD_TABLESWITCH:				/* switch statement				*/
					s4ptr = ip->val.a;
				
					lsra_add_cfg(m, ls, b_index,  m->basicblockindex[*s4ptr],false);
				
					s4ptr++;
					low = *s4ptr;
					s4ptr++;
					high = *s4ptr;
				
					count = (high-low+1);
				
					while (--count >= 0) {
						s4ptr++;
						lsra_add_cfg(m, ls, b_index,  m->basicblockindex[*s4ptr],false);
				    }
					break;
				
				case ICMD_LOOKUPSWITCH:				/* switch statement				*/
					s4ptr = ip->val.a;
			   
					lsra_add_cfg(m, ls, b_index,  m->basicblockindex[*s4ptr],false);
				
					++s4ptr;
					count = *s4ptr++;
				
					while (--count >= 0) {
						lsra_add_cfg(m, ls, b_index,  m->basicblockindex[s4ptr[1]],false);
						s4ptr += 2;
				    }
					break;

				case ICMD_JSR:
/* 					ld->c_last_jump = blockIndex; */
					lsra_add_cfg(m, ls, b_index,  m->basicblockindex[ip->op1],true);
/* 					dF(m, ld, blockIndex, m->basicblockindex[ip->op1]);          */
					break;
				
				case ICMD_RET:
					lsra_add_cfg(m, ls, b_index,  m->basicblockindex[ip->op1],true);
/* 					dF(m, ld, blockIndex, ld->c_last_jump+1); */
					break;
				
				default:
					lsra_add_cfg(m, ls, b_index, b_index + 1 ,false);
					break;	
			    } /* switch (ip->opc)*/                        
		    }     /* if (m->basicblocks[blockIndex].icount) */
	    }         /* if (m->basicblocks[b_index].flags >= BBREACHED) */
		b_index++;
	}             /* while (b_index < m->basicblockcount ) */
}




void lsra_init(methodinfo *m, codegendata *cd, t_inlining_globals *id, lsradata *ls) 
{
	int i;

	/* Init LSRA Data Structures */
	/* lifetimes für alle Basicblocks allokieren */
	ls->pred = DMNEW(struct _list *, m->basicblockcount+1); /* + 1 for a artificial exit node */
	ls->succ = DMNEW(struct _list *, m->basicblockcount+1); /* + 1 for a artificial exit node */
	ls->sorted = DMNEW(int , m->basicblockcount+1); /* + 1 for a artificial exit node */
	ls->sorted_rev = DMNEW(int , m->basicblockcount+1); /* + 1 for a artificial exit node */
	ls->num_pred = DMNEW(int , m->basicblockcount+1); /* + 1 for a artificial exit node */
	for (i=0; i<m->basicblockcount; i++) {
		ls->pred[i]=NULL;
		ls->succ[i]=NULL;
		ls->sorted[i]=-1;
		ls->sorted_rev[i]=-1;
		ls->num_pred[i]=0;
	}
	ls->pred[m->basicblockcount]=NULL;
	ls->succ[m->basicblockcount]=NULL;
	ls->sorted[m->basicblockcount]=-1;
	ls->sorted_rev[m->basicblockcount]=-1;
	ls->num_pred[m->basicblockcount]=0;	

	ls->ss_lifetimes=NULL;
#ifdef LSRA_DEBUG
	if (cd->maxlocals != id->cumlocals) panic("lsra: Welche locals nehmen?\n");
#endif
	ls->locals_lifetimes = DMNEW(struct lifetime *, cd->maxlocals);
	for (i=0; i < cd->maxlocals; i++) ls->locals_lifetimes[i]=NULL;
	ls->lifetimes=NULL;
	ls->stackslots=NULL;
	ls->end_bb= -1;
	ls->_backedges = NULL;
	ls->icount_max = 0;
}

bool lsra_test(methodinfo *m, codegendata *cd)
{
	int i;
	bool jsr;
	instruction *ip;

	/* in case of exceptionhandlers or subroutines return to regalloc */
	if (cd->exceptiontablelength > 0)
		return false;

	jsr=false;
	for (i=0; i< m->basicblockcount; i++) {
		ip = m->basicblocks[i].iinstr + m->basicblocks[i].icount -1;/* set ip to last instruction	*/
		if (ip->opc == ICMD_JSR) {
			/* check Instack of sub */
			printf("SBR Instackdepth: %3i\n",m->basicblocks[m->basicblockindex[ip->op1]].indepth);
			jsr=true;
		}
	}

/* 	if (jsr) return false; */
	return (!jsr);
}

bool lsra_setup(methodinfo *m, codegendata *cd, registerdata *rd, lsradata *ls)
{
#ifdef LSRA_DEBUG
	basicblock  *bptr;
#endif
	int i,p;
	s4  t;
	struct lifetime *lt, *n;
	int v_index;
	struct stackslot *ss;
	bool drop;
	struct depthElement *de;

	int *stack;
	int *visited;
	int stack_top;
	bool not_finished;

	struct _list *nl;

#ifdef LSRA_DUMP_LOOPDATA
	struct LoopContainer *lc;
	struct LoopElement *le;
#endif
	
	/* in case of exceptionhandlers or subroutines return to regalloc */
	if (!lsra_test(m,cd)) return false;

	/* Setup LSRA Data structures */
	if (opt_loops) 
		return false;

	lsra_make_cfg(m, ls);
#ifdef LSRA_DEBUG	
	printf("Successors:\n");
	for (i=0; i <= m->basicblockcount; i++) {
		printf("%3i->: ",i);
		for (nl=ls->succ[i]; nl!= NULL; nl=nl->next)
			printf("%3i ",nl->value);
		printf("\n");
	}
	lsra_DFS(m, ls);
	printf("Predecessors:\n");
	for (i=0; i <= m->basicblockcount; i++) {
		printf("%3i->: ",i);
		for (nl=ls->pred[i]; nl!= NULL; nl=nl->next)
			printf("%3i ",nl->value);
		printf("\n");
	}
	printf("Sorted1: ");
	for (i=0; i <= m->basicblockcount; i++) printf("%3i ", ls->sorted[i]);
	printf("\n");
#endif
	lsra_DFS_2(m, ls);  
#ifdef LSRA_DEBUG	
	printf("Sorted2: ");
	for (i=0; i <= m->basicblockcount; i++) printf("%3i ", ls->sorted[i]);
	printf("\n");
#endif
	lsra_get_backedges( m, ls);
#ifdef LSRA_DEBUG	
	printf("Successors:\n");
	for (i=0; i < m->basicblockcount; i++) {
		printf("%3i->: ",i);
		for (nl=ls->succ[i]; nl!= NULL; nl=nl->next)
			printf("%3i ",nl->value);
		printf("\n");
	}
	printf("Predecessors:\n");
	for (i=0; i < m->basicblockcount; i++) {
		printf("%3i->: ",i);
		for (nl=ls->pred[i]; nl!= NULL; nl=nl->next)
			printf("%3i ",nl->value);
		printf("\n");
	}
	printf("Sorted2: ");
	for (i=0; i < m->basicblockcount; i++) printf("%3i ", ls->sorted[i]);
	printf("\n");
	printf("Sorted_rev: ");
	for (i=0; i < m->basicblockcount; i++) printf("%3i ", ls->sorted_rev[i]);
	printf("\n");
#endif


#ifdef LSRA_DEBUG
	/* sicherheitshalber Konsistenz von m->basicblocks[..] mit basicblocks->next (Liste) überprüfen */
	printf("LSRA bb prüfen\n");
	i=0;
	bptr = m->basicblocks;
	while (bptr != NULL) {
		if (i > m->basicblockcount){
			panic("linked bb list does not correspond with bb array(1)\n");
		}
		if (bptr != &(m->basicblocks[i])){
			panic("linked bb list does not correspond with bb array(2)\n");
		}

		i++;
		bptr=bptr->next;
	}
	if (i<m->basicblockcount){
		panic("linked bb list does not correspond with bb array(3)\n");
	}

#endif


	ls->v_index = -1;
	for (i=0; i < m->basicblockcount; i++) {
		if (ls->sorted[i] != -1) {
			lsra_scan_registers_canditates(m, ls, ls->sorted[i]);
			lsra_join_lifetimes(m, ls, ls->sorted[i]);
		}
	}

	/* Todo - add Parameter initialisiation for locals 0 .. paramcount ! */
	/* Parameter initialisieren = local Vars schreibzugriff bei 0,0*/

 	for (p = 0, i = 0; p < m->paramcount; p++) {
 		t = m->paramtypes[p];

		if (rd->locals[i][t].type >= 0)	
			lsra_usage_local(ls, i, t, 0, -1, LSRA_STORE); /* Param to Local init happens before normal Code */
 		i++;
 		if (IS_2_WORD_TYPE(t))    /* increment local counter for 2 word types */
 			i++;
	}  /* end for */

	ls->lifetimes=ls->ss_lifetimes;
	/* add local_lifetimes to lifetimes */
	for (i=0; i < cd->maxlocals; i++) {
		if (ls->locals_lifetimes[i] != NULL) {
			for (lt=ls->locals_lifetimes[i]; lt->next != NULL; lt=lt->next);
			lt->next=ls->lifetimes;
			ls->lifetimes=ls->locals_lifetimes[i];
		}
	}	

	lsra_calc_lifetime_length(m, ls, cd);
#ifdef LSRA_PRINTLIFETIMES
	printf("Basicblockcount: %4i Max Instr/BB: %4i\n",m->basicblockcount, ls->icount_max);
/* 	print_lifetimes(rd, ls, ls->lifetimes); */
#endif
	return true;
}

void lsra_join_ss( struct lsradata *ls, struct stackelement *in, struct stackelement *out) {
	struct lifetime *lt, *lto, *ltn;
	struct stackslot *ss;
	int vn;

	if (in->varnum != out->varnum) {
		for (lt = ls->ss_lifetimes; (lt != NULL) && (lt->v_index != in->varnum); lt = lt->next);
	
		if (lt == NULL) panic("lsra_join_ss: lt for in not found\n");

		if (out->varnum >= 0) { /* no lifetime for this slot till now */
			out->varnum = in->varnum;
			lsra_add_ss(lt, out);
		} else {

			for (ltn = lto = ls->ss_lifetimes; (lto != NULL) && (lto->v_index != out->varnum); ltn=lto, lto = lto->next);

			if (lto == NULL) panic("lsra_join_ss: lt for out not found\n");
	
			/* take Lifetime lto out of ls->lifetimes */
			if (lto == ls->ss_lifetimes) {
				ls->ss_lifetimes = ls->ss_lifetimes->next;
			} else {
				ltn->next = lto->next;
			}
			/* merge lto into lt of in */
			while (lto->local_ss != NULL) {
				ss=lto->local_ss;
				lto->local_ss = ss->next;
				ss->next = lt->local_ss;
				lt->local_ss = ss;
				ss->s->varnum = lt->v_index;
			}
			lt->savedvar |= lto->savedvar;

			/* Todo add join of bb_first_def, i_first_def und *_last_use */
			/* ?change bb* to sorted index?!!! */
			if (lto->bb_first_def < lt->bb_first_def) {
				lt->bb_first_def = lto->bb_first_def;
				lt->i_first_def = lto->i_first_def;
			} else if ((lto->bb_first_def == lt->bb_first_def) && ( lto->i_first_def < lt->i_first_def)) {
				lt->i_first_def = lto->i_first_def;
			}	
			if (lto->bb_last_use > lt->bb_last_use) {
				lt->bb_last_use = lto->bb_last_use;
				lt->i_last_use = lto->i_last_use;
			} else if ((lto->bb_last_use == lt->bb_last_use) && ( lto->i_last_use > lt->i_last_use)) {
				lt->i_last_use = lto->i_last_use;
			}	
		}
	}
}

/* join instack of Basic Block b_index with outstack of predecessors */
void lsra_join_lifetimes(methodinfo *m, lsradata *ls, int b_index) {
	struct stackelement *in, *i, *out;
    struct _list *pred;

	in=m->basicblocks[b_index].instack;
	if (in != NULL) {
		for (pred = ls->pred[b_index]; pred != NULL; pred = pred->next) {
			out = m->basicblocks[pred->value].outstack;
			for (i=in; (i != NULL); i = i->prev, out=out->prev) {
				lsra_join_ss(ls, i, out);
			}
		}
	}
}

void lsra_main(methodinfo *m, lsradata *ls, registerdata *rd, codegendata *cd)
{
	struct lifetime *lt, *lt_prev, *lt_temp, *int_lt, *int_lt_last, *flt_lt, *flt_lt_last;
#ifdef LSRA_DEBUG
	int lt_count,lt_int_count,lt_flt_count,lt_left_count;
#endif
	int i;
	int lsra_mem_use;
	int sav_reg_count, tmp_reg_count;
	struct lsra_reg *reg;
	int reg_count;
/* 	varinfo *v; */
	int type;
	int flags; /* 0 INMEMORY->lifetimes, 1 INTREG->int_lt, 2 FLTREG->flt_lt */
	int lsra_reg_use;


	/* first split lifetimes for integer and float registers */
	int_lt_last=int_lt=NULL;
	flt_lt_last=flt_lt=NULL;

#ifdef LSRA_DEBUG
	for (lt_count=0,lt=ls->lifetimes; lt!=NULL;lt=lt->next,lt_count++);
#endif

	for (lt_prev=lt=ls->lifetimes;lt!=NULL;) {
		lt->reg = -1;
		
		if (lt->v_index < 0) { /* stackslot */

#ifdef LSRA_DEBUG
			if (lt->local_ss == NULL) panic("lsra_main Lifetime Stackslot invalid\n");
#endif
			type = lt->local_ss->s->type;
		} else { /* local var */
			if (rd->locals[lt->v_index][lt->type].type>=0) {
				type = rd->locals[lt->v_index][lt->type].type;
			} else panic("Type Data Mismatch 2\n");
		}

		switch (type) {
		case TYPE_LNG:
#if defined(__I386__)
			/*
			 * for i386 put all longs in memory
			 */
			flags=0;
			break;
#endif
		case TYPE_INT:
		case TYPE_ADR:
			flags=1;
			break;
		case TYPE_DBL:
#if defined(__I386__)
			/*
			 * for i386 put all longs in memory
			 */
			flags=0;
			break;
#endif
		case TYPE_FLT:
			flags=2;
			break;
		default:
			panic("Unknown Type\n");
		}

		if (flags!=0) {
			switch (flags) {
			case 1: /* l->lifetimes -> int_lt */
				if (int_lt == NULL) {
					int_lt_last=int_lt=lt;
				} else {
					int_lt_last->next=lt;
					int_lt_last=lt;
				}
				break;
			case 2: /* l->lifetimes -> flt_lt */
				if (flt_lt==NULL) {
					flt_lt_last=flt_lt=lt;
				} else {
					flt_lt_last->next=lt;
					flt_lt_last=lt;
				}
				break;
			}
			lt_temp=lt;
			if (lt == ls->lifetimes) {
				lt=lt_prev=ls->lifetimes=ls->lifetimes->next;
			} else {
				lt_prev->next=lt->next;
				lt=lt->next;
			}
			lt_temp->next=0;
		} else {
			lt_prev=lt;
			lt=lt->next;
		}
	}
	lsra_sort_lt(&int_lt);
	lsra_sort_lt(&(ls->lifetimes));
	lsra_sort_lt(&flt_lt);

#ifdef LSRA_DEBUG
	for (lt_int_count=0,lt=int_lt; lt!=NULL;lt=lt->next,lt_int_count++);
	for (lt_flt_count=0,lt=flt_lt; lt!=NULL;lt=lt->next,lt_flt_count++);
	for (lt_left_count=0,lt=ls->lifetimes; lt!=NULL;lt=lt->next,lt_left_count++);

	printf("\nLifetimes: %3i left: %3i Intlt: %3i Fltlt: %3i \n",lt_count,lt_left_count,lt_int_count,lt_flt_count);
	if (lt_count != lt_int_count + lt_flt_count + lt_left_count) {
		panic ("lifetimes missing\n");
	} 
#endif
	lsra_reg_use=rd->savintregcnt;
	if (int_lt!=NULL) {
		for (reg_count = 0; nregdescint[reg_count] != REG_END; reg_count++);

		reg=DMNEW(struct lsra_reg,reg_count);
		sav_reg_count=0;
		for (i=0; i<reg_count ; i++) {
			if (sav_reg_count >= MAX_SAV_REG_COUNT)
				break;
			if (nregdescint[i]==REG_SAV) {
				reg[sav_reg_count].reg_index=i;
				reg[sav_reg_count].use=0;
				sav_reg_count++;
			}
		}
		tmp_reg_count=sav_reg_count;
		for (i=0; i<reg_count ; i++) {
#if defined(__I386__)
			if (i==ECX) continue;
			if (i==EDX) continue;
#endif
			if ((tmp_reg_count-sav_reg_count) >= MAX_TMP_REG_COUNT)
				break;
			if (nregdescint[i]==REG_TMP) {
				reg[tmp_reg_count].reg_index=i;
				reg[tmp_reg_count].use=0;
				tmp_reg_count++;
			}
		}
		if (m->isleafmethod) {
			for (i = reg_count-1; i >= 0 ; i--) {
		   		if ((tmp_reg_count-sav_reg_count) >= MAX_TMP_REG_COUNT)
		   			break;
		   		if (nregdescint[i]==REG_ARG) {
		   			reg[tmp_reg_count].reg_index=i;
		   			reg[tmp_reg_count].use=0;
		   			tmp_reg_count++;
		   		}
		   	}
#if defined(__X86_64__)
			i=reg[tmp_reg_count-1].reg_index;
			reg[tmp_reg_count-1].reg_index = reg[tmp_reg_count-4].reg_index;
			reg[tmp_reg_count-4].reg_index = i;
			i=reg[tmp_reg_count-2].reg_index;
			reg[tmp_reg_count-2].reg_index = reg[tmp_reg_count-3].reg_index;
			reg[tmp_reg_count-3].reg_index = i;

#endif
   		}
		_lsra_main(m, ls, int_lt, reg, tmp_reg_count, sav_reg_count, &lsra_mem_use, &lsra_reg_use);
		if (lsra_reg_use > rd->savintregcnt) lsra_reg_use=rd->savintregcnt;
	}
	rd->maxsavintreguse= lsra_reg_use;
	lsra_reg_use=rd->savfltregcnt;

	if (flt_lt!=NULL){
		for (reg_count = 0; nregdescfloat[reg_count] != REG_END; reg_count++);

		reg=DMNEW(struct lsra_reg,reg_count);
		sav_reg_count=0;
		for (i=0; i<reg_count ; i++) {
			if (sav_reg_count >= MAX_SAV_REG_COUNT)
				break;
			if ((nregdescfloat[i]==REG_SAV) /* || (m->isleafmethod && (nregdescfloat[i]==REG_ARG)) */) {
				reg[sav_reg_count].reg_index=i;
				reg[sav_reg_count].use=0;
				sav_reg_count++;
			}
		}

		tmp_reg_count=sav_reg_count;
		for (i=0; i<reg_count ; i++) {
			if ((tmp_reg_count-sav_reg_count) >= MAX_TMP_REG_COUNT)
				break;
			if (nregdescfloat[i]==REG_TMP) {
				reg[tmp_reg_count].reg_index=i;
				reg[tmp_reg_count].use=0;
				tmp_reg_count++;
			}
		}
		if (m->isleafmethod)
			for (i = reg_count-1; i >= 0 ; i--) {
				if ((tmp_reg_count-sav_reg_count) >= MAX_TMP_REG_COUNT)
					break;
				if (nregdescfloat[i]==REG_ARG) {
					reg[tmp_reg_count].reg_index=i;
					reg[tmp_reg_count].use=0;
					tmp_reg_count++;
				}
			}
		_lsra_main(m,ls, flt_lt, reg, tmp_reg_count, sav_reg_count, &lsra_mem_use, &lsra_reg_use);
		if (lsra_reg_use > rd->savfltregcnt) lsra_reg_use=rd->savfltregcnt;
	}

	rd->maxsavfltreguse=lsra_reg_use;

	/* allocate stack space for passing arguments to called methods */

#if !defined(SPECIALMEMUSE)
	/* For this to work properly the integer argument register count must be  */
	/* less or equal the float argument register count (e.g. x86_64).         */
	/* (arch.h: INT_ARG_CNT <= FLT_ARG_CNT)                                   */
	if (rd->arguments_num > INT_ARG_CNT) {
		lsra_mem_use = rd->arguments_num - INT_ARG_CNT;

	} else {
		lsra_mem_use = 0;
	}
#endif

#ifdef LSRA_DEBUG
	printf("Alloc Rest\n");
#endif
	lsra_alloc(m, rd, ls->lifetimes,&lsra_mem_use);
#ifdef LSRA_DEBUG
	printf("Alloc Int\n");
#endif
	lsra_alloc(m, rd, int_lt,&lsra_mem_use);
#ifdef LSRA_DEBUG
	printf("Alloc Flt\n");
#endif
	lsra_alloc(m, rd, flt_lt,&lsra_mem_use);

#ifdef LSRA_PRINTLIFETIMES
	printf("Int RA complete \n");
	printf("Lifetimes after splitting int: \n");
	print_lifetimes(rd, ls, int_lt);

	printf("Flt RA complete \n");
	printf("Lifetimes after splitting flt:\n");
	print_lifetimes(rd, ls, flt_lt);

	printf("Rest RA complete \n");
	printf("Lifetimes after leftt:\n");
	print_lifetimes(rd, ls, ls->lifetimes);
#endif

	rd->maxmemuse=lsra_mem_use;



#ifdef LSRA_TESTLT
	printf("TTTTTTTTTTTTTTTTTTTTTTTTTT\nTesting Lifetimes int\n");
	test_lifetimes( m , ls, int_lt, cd);
	printf("Testing Lifetimes flt\n");
	test_lifetimes( m , ls, flt_lt, cd);
	printf("Testing Lifetimes rest\n");
	test_lifetimes(m, ls, ls->lifetimes, cd);
#endif


}

void lsra_alloc(methodinfo *m, registerdata *rd, struct lifetime *lifet, int *mem_use)
{
	int flags,regoff;
	struct lifetime *lt;
	struct freemem *fmem;
	struct stackslot *n;
	
	fmem=DNEW(struct freemem);
	fmem->off=-1;
	fmem->next=NULL;

	for (lt=lifet;lt!=NULL;lt=lt->next) {
#ifdef LSRA_MEMORY
		lt->reg=-1;
#endif
		if (lt->reg==-1) {
			flags=INMEMORY;

			regoff=lsra_getmem(lt, fmem, mem_use);
		} else {
			flags=lt->savedvar;
			regoff=lt->reg;
		}

		if (lt->v_index < 0) {
			for (n=lt->local_ss; n!=NULL; n=n->next) {
				lsra_setflags( &(n->s->flags), flags);
				n->s->regoff=regoff;
			}
		} else { /* local var */
			if (rd->locals[lt->v_index][lt->type].type>=0) {
/* 				lsra_setflags( &(rd->locals[lt->v_index][lt->type].flags), flags); */
				rd->locals[lt->v_index][lt->type].flags= flags;
				rd->locals[lt->v_index][lt->type].regoff=regoff;
			} else panic("Type Data mismatch 1\n");
		}		
		lt->reg = regoff;
	}
}

void lsra_setflags(int *flags, int newflags)
{
	if ( newflags & INMEMORY)
		*flags |= INMEMORY;
	else
		*flags &= ~INMEMORY;
	
	if (newflags & SAVEDVAR)
		*flags |= SAVEDVAR;
}

int lsra_getmem(struct lifetime *lt, struct freemem *fmem, int *mem_use)
{
	struct freemem *fm, *p;

	/* noch kein Speicher vergeben, oder alle Enden später */
	if ((fmem->next == NULL) || (fmem->next->end > lt->i_start)) 
		fm=lsra_getnewmem(mem_use);
	else {
		/* Speicherstelle frei */
		fm=fmem->next;
		fmem->next=fm->next;
		fm->next=NULL;
	}
	fm->end=lt->i_end;
	for (p=fmem; (p->next!=NULL) && (p->next->end < fm->end); p=p->next);
	fm->next=p->next;
	p->next=fm;
	return fm->off;
}

struct freemem *lsra_getnewmem(int *mem_use)
{
	struct freemem *fm;

	fm=DNEW(struct freemem);
	fm->next=NULL;
	fm->off=*mem_use;
	(*mem_use)++;
	return fm;
}

void _lsra_main( methodinfo *m, lsradata *ls, struct lifetime *lifet, struct lsra_reg *reg, int tmp_reg_count, int sav_reg_count, int *mem_use, int *reg_use)
{
	struct lifetime *lt;
	int i;
	int _reg_use;
	int reg_count, active_count;
	
	if ((tmp_reg_count+sav_reg_count) == 0) {
		for (lt=lifet; lt != NULL; lt=lt->next)
			lt->reg=-1;
		return;
	}

	ls->active_tmp = NULL;
	ls->active_sav = NULL;
	ls->active_tmp_count=0;
	ls->active_sav_count=0;

#ifdef DONT_COMPILE
	if (m->isleafmethod) {
		/* do some precoloring -> all Locals with v_index < m->paramcount */
		/* precoloring of params */
		/* Local Vars, which get parameters passed have i_start == -1 and are      */
		/* at the beginning of the lifetimes list (is sortet by increasing i_start */
		for (lt=lifet; (lt!=NULL) && (lt->i_start == -1); lt = lt->next) {
#ifdef LSRA_DEBUG
			if ((lt->varnum < 0) || (lt->varnum >= m->paramcount))
				panic("_lsra_main: lifetime <> parameter conflict\n");
			if (m->paramtypes[lt->varnum] != lt->type)
				panic("_lsra_main: lifetime <> parameter type conflict\n");
#endif

		}
	}
#endif

	for (lt=lifet; lt != NULL; lt=lt->next) {
		lsra_expire_old_intervalls(ls, lt,reg);
		if (lt->savedvar && (!m->isleafmethod)) {
			reg_count=sav_reg_count;
			active_count=ls->active_sav_count;
		}
		else {
			reg_count=tmp_reg_count;
			active_count=ls->active_sav_count+ls->active_tmp_count;
		}
		if (active_count == reg_count)
			spill_at_intervall(ls, lt);
		else {
			for (i=reg_count-1;i>=0;i--) {
				if (reg[i].use==0) {
					reg[i].use=1;
					lt->reg=reg[i].reg_index;
					_reg_use=i;
					if (_reg_use<*reg_use) *reg_use=_reg_use;
					break;
				}
			}
			if (i < sav_reg_count)
				lsra_add_active(lt, &(ls->active_sav), &(ls->active_sav_count));
			else
				lsra_add_active(lt, &(ls->active_tmp), &(ls->active_tmp_count));
		}
	}
}

void lsra_add_active(struct lifetime *lt, struct active_lt **active, int *active_count)
{
	struct active_lt *alt,*alt1,*alt2;
	alt=DNEW(struct active_lt);
	alt->lt=lt;

	for(alt1=alt2=*active; alt1 != NULL; alt2=alt1, alt1=alt1->next)
		if (alt1->lt->i_end > lt->i_end) break;

	if (alt1 == *active) {
		alt->next = *active;
		*active = alt;
	} else {
		alt->next = alt2->next;
		alt2->next = alt;
	}
	(*active_count)++;
}


void lsra_expire_old_intervalls(lsradata *ls, struct lifetime *lt, struct lsra_reg *reg)
{
	_lsra_expire_old_intervalls(lt, reg, &(ls->active_tmp), &(ls->active_tmp_count));
	_lsra_expire_old_intervalls(lt, reg, &(ls->active_sav), &(ls->active_sav_count));
}

void _lsra_expire_old_intervalls(struct lifetime *lt, struct lsra_reg *reg, struct active_lt **active, int *active_count)
{
	struct active_lt *alt,*alt1;
	int i;

	for (alt1=alt=*active; alt != NULL; alt1=alt, alt=alt->next) {
		if (alt->lt->i_end >= lt->i_start) return;
		if (alt == *active)
			*active = (*active)->next;
		else
			alt1->next=alt->next;

		for (i=0;reg[i].reg_index != alt->lt->reg;i++);
		reg[i].use=0;
		(*active_count)--;
	}
}

void spill_at_intervall(lsradata *ls, struct lifetime *lt )
{
	if (lt->savedvar)
		_spill_at_intervall(lt, &(ls->active_sav), &(ls->active_sav_count));
	else {
		_spill_at_intervall(lt, &(ls->active_tmp), &(ls->active_tmp_count));
		if (lt->reg == -1) /* kein tmp mehr frei gewesen */
			_spill_at_intervall(lt, &(ls->active_sav), &(ls->active_sav_count));
	}
/* 	if (lt->reg == -2) panic("spill_at_intervall: Keine Register mehr frei gewesen!\n"); */
}

void _spill_at_intervall(struct lifetime *lt, struct active_lt **active, int *active_count)
{
	struct active_lt *alt,*alt1;
	if (*active == NULL) {
		lt->reg=-1;
		return;
	}
	/* get last intervall from active */
	for (alt1=alt=*active; alt->next != NULL; alt1=alt, alt=alt->next);
	
	if ((alt->lt->i_end > lt->i_end) /* || (alt->lt->usagecount < lt->usagecount) */ ) {
			lt->reg=alt->lt->reg;
			alt->lt->reg=-1;
		
			if (alt == *active)
				*active=(*active)->next;
			else
				alt1->next=alt->next;
			/* 		FREE(alt,struct active_lt); */
			(*active_count)--;
			lsra_add_active(lt, active, active_count);
	} else {
		lt->reg=-1;
	}
}


void lsra_calc_lifetime_length(methodinfo *m, lsradata *ls, codegendata *cd)
{
	struct lifetime *lt;
	int i;
	struct stackslot *ss;

#if 0
	int lifetimecount;
	int max_local_ss;
	int cum_local_ss,local_ss_count;
	int i_count;
#endif
	/**********/
	/* Todo: */
	/* for Exceptionhandler Blocks loops were not analyed */
	/* -> temporary solution:  expand all lifetimes used in a exceptionhandler to the extend of the corresponding exc.handler*/
	/**/
	/**/
	/* Falls die einzelnen Blöcke einer Loop nicht durchgehend nummeriert sind */
	/* auch nicht alle in block_loop eintragen! */


	/* extend lifetime over backedges */
	/* now iterate through lifetimes and expand them */
	
#if 0
	lifetimecount = max_local_ss = cum_local_ss = 0;
#endif

	for(lt=ls->lifetimes ;lt != NULL; lt=lt->next) {
#if 0
		lifetimecount ++;
		local_ss_count = 0;
		for (ss=lt->local_ss; ss != 0; ss = ss->next, local_ss_count++);
		if (local_ss_count > max_local_ss) max_local_ss = local_ss_count;
		cum_local_ss+=local_ss_count;
#endif

		lt->i_start =(m->basicblockcount - lt->bb_first_def) * ls->icount_max + lt->i_first_def;
		lt->i_end = (m->basicblockcount - lt->bb_last_use) * ls->icount_max + lt->i_last_use;
		if (lt->i_start > lt->i_end) 
			printf("--------- Warning: last use before first def! ------------vi: %i start: %i end: %i\n", lt->v_index, lt->i_start, lt->i_end);

#ifdef DONT_COMPILE_OLD_CODE
		/* expand lifetimes in a exceptionhandler to at least the whole handler */
		/* TODO do a loop analyze for the exceptionhandler*/

		/* every lifetime of a guarded area, which is used in the exc. handler, */
		/* has to be expanded to at least the whole guarded area */
		for (i=0; i < cd->exceptiontablelength; i++) {
			if ( !((bfirst > ls->ex[i].handler_max) || ( blast < ls->ex[i].handler_min)) ) {
				/* lifetime lt lies within the exceptionhandler */
				/* expand to at least the extends of this exceptionhandler */

				/* -> Lifetime start has to be at minimum the start of the exceptionhandler */
				if (bfirst >= ls->ex[i].handler_min) {
					bfirst=ls->ex[i].handler_min;
					ifirst=0;
				}
				/* -> Lifetime end has to be at minimum the end of the exceptionhandler */
				if (blast <= ls->ex[i].handler_max) {
					blast=ls->ex[i].handler_max;
					ilast= m->basicblocks[ls->ex[i].handler_max].icount-1;
				}
			} 
		}
#endif

		if (lt->bb_first_def != lt->bb_last_use) {
			/* Lifetime goes over more than one Basic Blocks -> */
			/* check for necessary extension over backedges     */
			/* very conservative but fast approach by now       */
			/* see lsra_get_backedges                           */

			for (i=0; i < ls->backedge_count; i++) {
				if (!( (lt->bb_first_def < ls->backedge[i]->end) ||
					   (lt->bb_last_use > ls->backedge[i]->start) )) {
					/* Live intervall intersects with a backedge */
					if (lt->bb_first_def <= ls->backedge[i]->start)
						lt->i_start = (m->basicblockcount - ls->backedge[i]->start) * ls->icount_max;
					if (lt->bb_last_use >= ls->backedge[i]->end)
						lt->i_end = (m->basicblockcount - ls->backedge[i]->end) * ls->icount_max +  m->basicblocks[ls->sorted[ls->backedge[i]->end]].icount-1;
				}
			}
		}
	}
#if 0
	i_count=0;
	for (i=0; i<m->basicblockcount; i++)
		if (m->basicblocks[i].flags >= BBREACHED)
			i_count+=m->basicblocks[i].icount;
	printf("Instr: %5i Lifetimes: %5i Local SS Max: %4i Cum: %4i m->maxlifetimes %4i\n",i_count, lifetimecount, max_local_ss, cum_local_ss, m->maxlifetimes);
#endif
}

#define P_MAX 21
void _lsra_merge_lt(struct lifetime **p, int i1, int i2)
{
	struct lifetime *iptr, *iptr1, *iptr2;

	if ( (iptr1=p[i2])==NULL) return;
    if ( (iptr=p[i1])==NULL) return;

	iptr2=p[i1]=NULL;
	p[i2]=NULL;

	while  ((iptr != NULL) && (iptr1 != NULL)) {
		if (iptr->i_start < iptr1->i_start) {
			if (iptr2==NULL) {
				p[i1]=iptr;
			} else {
				iptr2->next=iptr;
			}
			iptr2=iptr;
			iptr=iptr->next;
		} else {
			if (iptr2==NULL) {
				p[i1]=iptr1;
			} else {
				iptr2->next=iptr1;
			}
			iptr2=iptr1;
			iptr1=iptr1->next;
		}
	}
	if (iptr==NULL)
		iptr2->next=iptr1;
	if (iptr1==NULL)
		iptr2->next=iptr;
}

void lsra_merge_lt(struct lifetime **p, int top)
{
	int i,j;

	for (j=1; j<top; j*=2)
		for (i=1; i<top; i+=2*j)
			_lsra_merge_lt(p, i, i+j);
	_lsra_merge_lt(p, 0, 1);
}
	
void lsra_sort_lt(struct lifetime **lifet)
{
	/* sort lifetimes by increasing start point */
/* 	struct lifetime **plt,**plt1; */
	struct lifetime *lt, *temp, *tmp;
	int i, top;
	struct lifetime **p;

	p=DMNEW(struct lifetime *, P_MAX);
	for (i=0; i<P_MAX; i++) p[i]=NULL;

	top=0;

	for (lt=*lifet; lt!= NULL;) {
		temp=lt;
		lt=lt->next;
		if (lt == NULL) {
			p[top]=temp;
			temp->next=NULL;
		} else {
			tmp=lt;
			lt=lt->next;

			if (temp->i_start < tmp->i_start) {
				p[top]=temp;
				/* temp->next == tmp */
				tmp->next=NULL;
			} else {
				p[top]=tmp;
				tmp->next=temp;
				temp->next=NULL;
			}
		}
		top++;
		if (top == P_MAX) {
			lsra_merge_lt(p, P_MAX);
			top=1;
		}
	}
	lsra_merge_lt(p, top);
	*lifet=p[0];
}

#ifdef LSRA_PRINTLIFETIMES
void print_lifetimes(registerdata *rd, lsradata *ls, struct lifetime *lt)
{
	struct lifetime *n;
	struct _i_list *ni;
	int type,flags,regoff,j,varkind;
	/* 	int i; */

	for (n=lt,j=0; n!=NULL; n=n->next,j++) {
		if (n->v_index < 0) { /* stackslot */
			type = n->local_ss->s->type;
			flags=n->local_ss->s->flags;
			regoff=n->local_ss->s->regoff;
			varkind=n->local_ss->s->varkind;
		} else { /* local var */
			if (rd->locals[n->v_index][n->type].type>=0) {
				type = rd->locals[n->v_index][n->type].type;
				flags=rd->locals[n->v_index][n->type].flags;
				regoff=rd->locals[n->v_index][n->type].regoff;
				varkind=-1;
			} else 
				panic("Type Data mismatch 3\n");
		}
		printf("i_Start: %3i(%3i,%3i) i_stop: %3i(%3i,%3i) reg: %3i VI: %3i type: %3i flags: %3i varkind: %3i \n",n->i_start, ls->sorted[n->bb_first_def], n->i_first_def,n->i_end, ls->sorted[n->bb_last_use], n->i_last_use,regoff,n->v_index,type,flags, varkind);

	}
	printf( "%3i Lifetimes printed \n",j);
}
#endif

struct stackslot *lsra_make_ss(stackptr s, int bb_index)
{
	struct stackslot *ss;

	ss=DNEW(struct stackslot);
	ss->bb=bb_index;
	ss->s=s;
	return ss;
}

/* merge i_list from lt1 to lt in order */
void lsra_merge_i_lists(struct lifetime *lt, struct lifetime *lt1)
{
	struct _i_list *iptr, *iptr1, *iptr2;

#ifdef LSRA_DEBUG
	bool joining_lt; 

	joining_lt = false;
#endif

	/* merge i_lists in order */
	iptr=lt->i_list;
	iptr2=lt->i_list=NULL;
	iptr1=lt1->i_list;
	while  ((iptr != NULL) && (iptr1 != NULL)) {
		if (iptr1->instr == PASSING_THROUGH_LT) { 
			/* throw away, just for joining */
			iptr1=iptr1->next;
#ifdef LSRA_DEBUG
			joining_lt = true;
#endif
		} else {
			if ((iptr->b_index > iptr1->b_index)|| ((iptr->b_index == iptr1->b_index) && (iptr->instr > iptr1->instr))) {
				if (lt->i_list==NULL) {
					lt->i_list=iptr;
				} else {
					iptr2->next=iptr;
				}
				iptr2=iptr;
				iptr=iptr->next;
			} else {
				if (lt->i_list==NULL) {
					lt->i_list=iptr1;
				} else {
					iptr2->next=iptr1;
				}
				iptr2=iptr1;
				iptr1=iptr1->next;
			}
		}
	}
#ifdef LSRA_DEBUG
	if (!joining_lt)
		if (iptr2 == NULL)
			panic("lsra_merge_i_lists: Empty Instruction List in Lifetime\n");
#endif
	if (iptr==NULL) {
		if (lt->i_list==NULL)
			lt->i_list=iptr1;
		else
			iptr2->next=iptr1;
	}
	if (iptr1==NULL) {
		if (lt->i_list==NULL)
			lt->i_list=iptr;
		else
			iptr2->next=iptr;
	}
}

/* merge local_ss from lt1 to lt in order */
void lsra_merge_local_ss(struct lifetime *lt, struct lifetime *lt1)
{
	struct stackslot *ssptr, *ssptr1, *ssptr2;

	/* merge stackslots in order */
	ssptr=lt->local_ss;
	ssptr2=lt->local_ss=NULL;
	ssptr1=lt1->local_ss;
	while  ((ssptr != NULL) && (ssptr1 != NULL)) {

		if (ssptr->s > ssptr1->s) {
			if (lt->local_ss==NULL) {
				lt->local_ss=ssptr;
			} else {
				ssptr2->next=ssptr;
			}
			ssptr2=ssptr;
			ssptr=ssptr->next;
		} else {
			if (lt->local_ss==NULL) {
				lt->local_ss=ssptr1;
			} else {
				ssptr2->next=ssptr1;
			}
			ssptr2=ssptr1;
			ssptr1=ssptr1->next;
		}
	}
#ifdef LSRA_DEBUG
	if (ssptr2 == NULL)
		panic("lsra_merge_local_ss: Empty Stackslot List in Lifetime\n");
#endif
	if (ssptr==NULL) {
		if (lt->local_ss==NULL)
			lt->local_ss=ssptr1;
		else
			ssptr2->next=ssptr1;
	}
	if (ssptr1==NULL) {
		if (lt->local_ss==NULL)
			lt->local_ss=ssptr;
		else
			ssptr2->next=ssptr;
	}
}


struct _i_list *lsra_add_i_list(struct _i_list *i_list, int instr, int b_index, int store)
{
	struct _i_list *n;

	n=DNEW(struct _i_list);
	n->instr=instr;
	n->b_index=b_index;
	n->store=store;
	n->next=i_list;
	return n;
}

void lsra_add_ss(struct lifetime *lt, stackptr s) {
	struct stackslot *ss;
	/* Stackslot noch nicht eingetragen? */

	for (ss = lt->local_ss; (ss!=NULL) && (ss->s != s); ss=ss->next);

	if (ss==NULL) {
		ss = DNEW(struct stackslot);
		ss->s = s;
		ss->s->varnum = lt->v_index;
		ss->next = lt->local_ss;
		lt->local_ss = ss;
		if (s != NULL) lt->savedvar |= s->flags & SAVEDVAR;
	}
}

struct lifetime *get_ss_lifetime(lsradata *ls, int v_index) {
	struct lifetime *ss_lt;
	
	if (v_index >= 0) {/* new Lifetime *//* existiert noch nicht -> neue anlegen */
			v_index = ls->v_index--;
			ss_lt = NULL;
	} else
		for (ss_lt=ls->ss_lifetimes; (ss_lt != NULL) && (ss_lt->v_index != v_index); ss_lt=ss_lt->next);

	if (ss_lt == NULL) { /* existiert noch nicht -> neue anlegen */
		ss_lt=DNEW(struct lifetime);
		ss_lt->v_index = v_index;
		ss_lt->bb_last_use = -1;
		ss_lt->bb_first_def = -1;
		ss_lt->local_ss = NULL;
		ss_lt->savedvar = 0;
		ss_lt->next = ls->ss_lifetimes;
		ls->ss_lifetimes = ss_lt;
	}
	return ss_lt;
}

#define lsra_new_stack(ls, s, block, instr) _lsra_new_stack(ls, s, block, instr, LSRA_STORE)
void _lsra_new_stack(lsradata *ls, stackptr s, int block, int instr, int store)
{
	struct lifetime *n;

	if (s->varkind == LOCALVAR) {
		lsra_usage_local(ls, s->varnum, s->type, block, instr, LSRA_STORE);
	} else if (s->varkind != ARGVAR) {

		n=get_ss_lifetime( ls, s->varnum );
		lsra_add_ss( n, s);

		/* remember first def -> overwrite everytime */
		n->bb_first_def = ls->sorted_rev[block];
		n->i_first_def = instr;

		if (ls->edx_free != -1) {
			if (n->bb_last_use == block) {
				if (n->i_last_use <= ls->edx_free) {
					/* ->  Lifetime wegschmeißen -> Stackslot nach EDX! */
					ls->edx_free = -1;
#ifdef LSRA_DEBUG
					printf("------ SS Index %i in EDX\n", s->varnum);
#endif
/* 					n->regoff = EDX; */
				}
			}
		}
	}
}

#define lsra_from_stack(ls, s, block, instr) _lsra_from_stack(ls, s, block, instr, LSRA_LOAD)
#define lsra_pop_from_stack(ls, s, block, instr) _lsra_from_stack(ls, s, block, instr, LSRA_POP)
void _lsra_from_stack(lsradata *ls, stackptr s, int block, int instr, int store)
{
	struct lifetime *n;

	if (s->varkind == LOCALVAR) {
		lsra_usage_local(ls, s->varnum, s->type, block, instr, LSRA_LOAD);
	} else if (s->varkind != ARGVAR) {

		n=get_ss_lifetime( ls, s->varnum );
		lsra_add_ss( n, s);

		/* remember last USE, so only write, if USE Field is undefine (==-1) */
		if (n->bb_last_use == -1) {
			n->bb_last_use = ls->sorted_rev[block];
			n->i_last_use = instr;
		}
	}
}

void lsra_usage_local(lsradata *ls, s4 v_index, int type, int block, int instr, int store)
{
	struct lifetime *n;

	/* Lifetime vom richtigen Type suchen */
	for (n=ls->locals_lifetimes[v_index]; (n!=NULL) && (n->type!=type);n=n->next);

	if (n==NULL) {
#ifdef LSRA_DEBUG
/* 		if (store != LSRA_STORE) printf("lsra_local_store: Read before write Local var: %i paramcount: ?\n", v_index); */
#endif
		lsra_new_local(ls, v_index, type);
		/* neue Lifetimes werden immer am Anfang der Liste eingehängt */
		n=ls->locals_lifetimes[v_index];
	}
	/* add access at (block, instr) to intruction list */
	if (store == LSRA_LOAD) { 
		/* remember last USE, so only write, if USE Field is undefine (==-1) */
		if (n->bb_last_use == -1) {
			n->bb_last_use = ls->sorted_rev[block];
			n->i_last_use = instr;
		}
	} else {
		/* store == LSRA_STORE, remember first def -> overwrite everytime */
		n->bb_first_def = ls->sorted_rev[block];
		n->i_first_def = instr;
	}
}	

void lsra_new_local(lsradata *ls, s4 v_index, int type)
{
	struct lifetime *n;

	n=DNEW(struct lifetime);
	n->local_ss=NULL;
	n->i_list=NULL;
	n->v_index=v_index;
	n->type=type;
	n->savedvar = SAVEDVAR;

	n->bb_last_use = -1;
	n->bb_first_def = -1;

	n->next=ls->locals_lifetimes[v_index];
	ls->locals_lifetimes[v_index]=n;
}

#ifdef LSRA_DEBUG
void lsra_dump_stack(stackptr s)
{
	while (s!=NULL) {
		printf("%p(R%3i N%3i K%3i T%3i F%3i) ",(void *)s,s->regoff, s->varnum, s->varkind, s->type, s->flags);
		s=s->prev;
	}
	printf("\n");
}
#endif


void lsra_scan_registers_canditates(methodinfo *m, lsradata *ls, int b_index)
{
	int i;
	int opcode;
	int iindex;
	stackptr    src;
	stackptr    dst;
	instruction *iptr;
	stackptr in,out;
	int      id, od;

	ls->edx_free = -1;

	src = m->basicblocks[b_index].instack;
	for (;src != NULL; src=src->prev) {
		if (src->varkind == ARGVAR )  /* no ARGVAR possible at BB Boundaries! -> change to TEMPVAR */
			src->varkind = TEMPVAR;
		if (src->varkind == STACKVAR )  /* no Interfaces at BB Boundaries with LSRA! -> change to TEMPVAR */
			src->varkind = TEMPVAR;
		if (src->varkind == LOCALVAR ) printf("----- Error: LOCALVAR at BB instack -----\n");
	}
	src = m->basicblocks[b_index].outstack;
	for (;src != NULL; src=src->prev) {
		if (src->varkind == ARGVAR )  printf("----- Error: ARGVAR at BB outstack -----\n");
		if (src->varkind == STACKVAR )  /* no Interfaces at BB Boundaries with LSRA! -> change to TEMPVAR */
			src->varkind = TEMPVAR;
		if (src->varkind == LOCALVAR ) printf("----- Error: LOCALVAR at BB outstack -----\n");
	}
			
#ifdef DONT_COMPILE_OLD_CODE
	if (dst != NULL) { /* create Lifetimes for pass-through Stackslots */
		in=m->basicblocks[b_index].instack;
		id=m->basicblocks[b_index].indepth;
		if (m->basicblocks[b_index].type != BBTYPE_STD) {
			/* Pay attention to the top Stackslot in BBTYPE_EXH and BBTYPE_SBR Basicblocks  */
			/* this is not a passthrough, but set from the "system" to the exception object or */
			/* the return adress -> just create a lifetime with a write at instr==0            */ 
			lsra_new_stack(ls, in, b_index, 0);
			in=in->prev;
			--id;
		} 

		out=m->basicblocks[b_index].outstack;
		od=m->basicblocks[b_index].outdepth;

		/* ignore all in-stackslots not in outstack */
		for (;id>od; in=in->prev, --id); 
		/* ignore all out-stackslots not in instack */
		for (;od>id; out=out->prev, --od);
		/* ignore all non equal stackslots from in and outstack */
		for (;in != out; in=in->prev, out=out->prev, --id); 
		/* set up a lifetime for the rest: */
		/* stackslot adress equal, stackslot"number" equal */
		for (;in!=NULL; in=in->prev) {
			/* Make 2 entries -> one for the instack, one for the out stack */
			lsra_new_stack(ls, in, b_index, PASSING_THROUGH_LT);
			lsra_new_stack(ls, in, b_index, PASSING_THROUGH_LT);
		}
	}
#endif
	iptr = m->basicblocks[b_index].iinstr;
	iindex = m->basicblocks[b_index].icount - 1;
	iptr+=iindex;

	if ((iindex+1) > ls->icount_max)
		ls->icount_max = iindex+1;


	for (;iindex >= 0; iindex--, iptr--)  {
		dst = iptr->dst;
		opcode = iptr->opc;
		if (iindex) /* > 0 */
			src=(iptr-1)->dst;
		else
			src=m->basicblocks[b_index].instack;

		if (icmd_uses_tmp[opcode].edx)
			ls->edx_free = -1;
		else
			if (ls->edx_free == -1) ls->edx_free = iindex;

#ifdef LSRA_DEBUG
		/* 				printf("bb: %3i bcount: %3i iindex: %3i ilen: %3i opcode: %3i %s\n",b_index,m->basicblockcount,iindex,len,opcode,icmd_names[opcode]); */
		/* 				lsra_dump_stack(src); */
		/* 				lsra_dump_stack(dst); */
#endif
		switch (opcode) {

			/* pop 0 push 0 */
		case ICMD_RET:
			lsra_usage_local(ls,iptr->op1,TYPE_ADR, b_index,iindex,LSRA_LOAD); /* local read (return adress) */
			break;
		case ICMD_JSR:
		case ICMD_GOTO:
		case ICMD_NOP:
		case ICMD_ELSE_ICONST:
		case ICMD_CHECKEXCEPTION:
		case ICMD_CHECKASIZE:
		case ICMD_INLINE_START:
		case ICMD_INLINE_END:
		case ICMD_RETURN:
			break;                             
		case ICMD_IINC:
			lsra_usage_local(ls,iptr->op1,TYPE_INT, b_index,iindex,LSRA_LOAD); /* local */
			lsra_usage_local(ls,iptr->op1,TYPE_INT, b_index,iindex,LSRA_STORE); /* local = local+<const> */
			break;

			/* pop 0 push 1 const */
			/* const->stack */
					
		case ICMD_ICONST:
		case ICMD_LCONST:
		case ICMD_FCONST:
		case ICMD_DCONST:
		case ICMD_ACONST:
			/* new stack slot */
			lsra_new_stack(ls,dst,b_index,iindex); /* const->stack */
			break;

			/* pop 0 push 1 load */
			/* local->stack */
					
		case ICMD_ILOAD:
		case ICMD_LLOAD:
		case ICMD_FLOAD:
		case ICMD_DLOAD:
		case ICMD_ALOAD:
			if (dst->varkind != LOCALVAR) {
				lsra_usage_local(ls,iptr->op1,opcode-ICMD_ILOAD, b_index,iindex,LSRA_LOAD); /* local->value */
				lsra_new_stack(ls,dst,b_index,iindex); /* value->stack */
			} else if (dst->varnum != iptr->op1) {
				lsra_usage_local(ls,iptr->op1,opcode-ICMD_ILOAD, b_index,iindex,LSRA_LOAD); /* local->value */
				lsra_usage_local(ls,dst->varnum,opcode-ICMD_ILOAD, b_index,iindex,LSRA_STORE); /* local->value */
			}

			break;

			/* pop 2 push 1 */
			/* Stack(arrayref,index)->stack */

		case ICMD_IALOAD:
		case ICMD_LALOAD:
		case ICMD_FALOAD:
		case ICMD_DALOAD:
		case ICMD_AALOAD:

		case ICMD_BALOAD:
		case ICMD_CALOAD:
		case ICMD_SALOAD:

			lsra_new_stack(ls,dst,b_index,iindex); /* arrayref[index]->stack */
			lsra_from_stack(ls, src,b_index,iindex); /* stack->index */
			lsra_from_stack(ls, src->prev,b_index,iindex); /* stack->arrayref */
			break;

			/* pop 3 push 0 */
			/* stack(arrayref,index,value)->arrayref[index]=value */

		case ICMD_IASTORE:
		case ICMD_LASTORE:
		case ICMD_FASTORE:
		case ICMD_DASTORE:
		case ICMD_AASTORE:

		case ICMD_BASTORE:
		case ICMD_CASTORE:
		case ICMD_SASTORE:

			lsra_from_stack(ls, src,b_index,iindex); /* stack -> value */
			lsra_from_stack(ls, src->prev,b_index,iindex); /* stack -> index */
			lsra_from_stack(ls, src->prev->prev,b_index,iindex); /* stack -> arrayref */
			break;

		case ICMD_POP: /* throw away a stackslot -> check if used anyway! */
			lsra_pop_from_stack(ls,src,b_index,iindex);
			break;

			/* pop 1 push 0 store */
			/* stack -> local */

		case ICMD_ISTORE:
		case ICMD_LSTORE:
		case ICMD_FSTORE:
		case ICMD_DSTORE:
		case ICMD_ASTORE:
			if (src->varkind != LOCALVAR) {
				lsra_from_stack(ls, src,b_index,iindex); /* stack -> value */
				lsra_usage_local(ls,iptr->op1,opcode-ICMD_ISTORE, b_index,iindex,LSRA_STORE); /* local->value */
			} else if (src->varnum != iptr->op1) {
				lsra_usage_local(ls,iptr->op1,opcode-ICMD_ISTORE, b_index,iindex,LSRA_STORE); /* local->value */
				lsra_usage_local(ls,src->varnum,opcode-ICMD_ISTORE, b_index,iindex,LSRA_LOAD); /* local->value */
			}
			break;

			/* pop 1 push 0 */

		case ICMD_IRETURN:
		case ICMD_LRETURN:
		case ICMD_FRETURN:
		case ICMD_DRETURN:
		case ICMD_ARETURN: /* stack(value) -> [empty] */
		case ICMD_ATHROW: /* stack(objref) -> undefined */
			lsra_from_stack(ls, src,b_index,iindex); /* stack -> value */
			break;
		case ICMD_PUTSTATIC: /* stack(value) -> static_field */
			/* pop 1 push 0 branch */
		case ICMD_NULLCHECKPOP: /****** ????? -1 -> stack *********/
		case ICMD_MONITORENTER:
		case ICMD_MONITOREXIT:
			lsra_from_stack(ls, src,b_index,iindex); /* stack -> value */
			break;

		case ICMD_IFNULL: /* stack(value) -> branch? */
		case ICMD_IFNONNULL:
		case ICMD_IFEQ:
		case ICMD_IFNE:
		case ICMD_IFLT:
		case ICMD_IFGE:
		case ICMD_IFGT:
		case ICMD_IFLE:
		case ICMD_IF_LEQ:
		case ICMD_IF_LNE:
		case ICMD_IF_LLT:
		case ICMD_IF_LGE:
		case ICMD_IF_LGT:
		case ICMD_IF_LLE:
			lsra_from_stack(ls, src,b_index,iindex); /* stack -> value */
			break;

			/* pop 1 push 0 table branch */

		case ICMD_TABLESWITCH:
			lsra_from_stack(ls, src,b_index,iindex); /* stack -> value */
			break;
		case ICMD_LOOKUPSWITCH:
			lsra_from_stack(ls, src,b_index,iindex); /* stack -> value */
			break;

			/* pop 2 push 0 */

		case ICMD_POP2: /* throw away 2 stackslots -> check if used anyway! */
			lsra_pop_from_stack(ls,src,b_index,iindex);
			lsra_pop_from_stack(ls,src->prev,b_index,iindex);
			break;

			/* pop 2 push 0 branch */

		case ICMD_IF_ICMPEQ: /* stack (v1,v2) -> branch(v1,v2) */
		case ICMD_IF_ICMPNE:
		case ICMD_IF_ICMPLT:
		case ICMD_IF_ICMPGE:
		case ICMD_IF_ICMPGT:
		case ICMD_IF_ICMPLE:

		case ICMD_IF_LCMPEQ:
		case ICMD_IF_LCMPNE:
		case ICMD_IF_LCMPLT:
		case ICMD_IF_LCMPGE:
		case ICMD_IF_LCMPGT:
		case ICMD_IF_LCMPLE:

		case ICMD_IF_ACMPEQ:
		case ICMD_IF_ACMPNE:
			lsra_from_stack(ls, src,b_index,iindex); 	   /* stack -> value*/
			lsra_from_stack(ls, src->prev,b_index,iindex); /* stack -> objref*/
			break;

			/* pop 2 push 0 */

		case ICMD_PUTFIELD: /* stack(objref,value) -> objref->method=value */

		case ICMD_IASTORECONST:
		case ICMD_LASTORECONST:
		case ICMD_AASTORECONST:
		case ICMD_BASTORECONST:
		case ICMD_CASTORECONST:
		case ICMD_SASTORECONST:
			lsra_from_stack(ls, src,b_index,iindex); 	   /* stack -> value*/
			lsra_from_stack(ls, src->prev,b_index,iindex); /* stack -> objref*/
			break;

			/* pop 0 push 1 dup */
			/* merge dupped vars??? */
		case ICMD_DUP:
			/* lsra_from_stack(ls, src,b_index,iindex);*/ /* inc usage_count! */
			lsra_new_stack(ls,dst,b_index,iindex);
/* 			dup_mark(&dup, src); */
/* 			dup_mark(&dup, dst); */
/* 			dup_next(&dup); */
			break;

			/* pop 0 push 2 dup */
					
		case ICMD_DUP2:
			lsra_new_stack(ls,dst->prev,b_index,iindex);
			lsra_new_stack(ls,dst,b_index,iindex); 
			lsra_from_stack(ls, src,b_index,iindex); /* or inc usage_count! */
			lsra_from_stack(ls, src->prev,b_index,iindex); /* inc usage_count! */

/* 			dup_mark(&dup, src); */
/* 			dup_mark(&dup, dst); */
/* 			dup_mark(&dup, dst->prev->prev); */
/* 			dup_next(&dup); */
/* 			dup_mark(&dup, src->prev); */
/* 			dup_mark(&dup, dst->prev); */
/* 			dup_mark(&dup, dst->prev->prev->prev); */
/* 			dup_next(&dup); */
			break;

			/* pop 2 push 3 dup */
					
		case ICMD_DUP_X1:
			lsra_from_stack(ls, src,b_index,iindex); /* from for to, or it will not work! inc usage_count! */
			lsra_from_stack(ls, src->prev,b_index,iindex); /*from for to, or it will not work!  inc usage_count! */
			lsra_new_stack(ls,dst->prev->prev,b_index,iindex);
			lsra_new_stack(ls,dst->prev,b_index,iindex);
			lsra_new_stack(ls,dst,b_index,iindex); 
/* 			dup_mark(&dup, src); */
/* 			dup_mark(&dup, dst); */
/* 			dup_mark(&dup, dst->prev->prev); */
/* 			dup_next(&dup); */
/* 			dup_mark(&dup, src->prev); */
/* 			dup_mark(&dup, dst->prev); */
/* 			dup_next(&dup); */
			break;

			/* pop 3 push 4 dup */
					
		case ICMD_DUP_X2:
			lsra_from_stack(ls, src,b_index,iindex); /* from for to, or it will not work! inc usage_count! */
			lsra_from_stack(ls, src->prev,b_index,iindex); /*from for to, or it will not work!  inc usage_count! */
			lsra_from_stack(ls, src->prev->prev,b_index,iindex); /*from for to, or it will not work!  inc usage_count! */
			lsra_new_stack(ls,dst->prev->prev->prev,b_index,iindex);
			lsra_new_stack(ls,dst->prev->prev,b_index,iindex);
			lsra_new_stack(ls,dst->prev,b_index,iindex);
			lsra_new_stack(ls,dst,b_index,iindex); 
/* 			dup_mark(&dup, src); */
/* 			dup_mark(&dup, dst); */
/* 			dup_mark(&dup, dst->prev->prev->prev); */
/* 			dup_next(&dup); */
/* 			dup_mark(&dup, src->prev); */
/* 			dup_mark(&dup, dst->prev); */
/* 			dup_next(&dup); */
/* 			dup_mark(&dup, src->prev->prev); */
/* 			dup_mark(&dup, dst->prev->prev); */
/* 			dup_next(&dup); */
			break;

			/* pop 3 push 5 dup */
					
		case ICMD_DUP2_X1:
			lsra_from_stack(ls, src,b_index,iindex); /* from for to, or it will not work! inc usage_count! */
			lsra_from_stack(ls, src->prev,b_index,iindex); /*from for to, or it will not work!  inc usage_count! */
			lsra_from_stack(ls, src->prev->prev,b_index,iindex); /*from for to, or it will not work!  inc usage_count! */
			lsra_new_stack(ls,dst->prev->prev->prev->prev,b_index,iindex);
			lsra_new_stack(ls,dst->prev->prev->prev,b_index,iindex);
			lsra_new_stack(ls,dst->prev->prev,b_index,iindex);
			lsra_new_stack(ls,dst->prev,b_index,iindex);
			lsra_new_stack(ls,dst,b_index,iindex); 
/* 			dup_mark(&dup, src); */
/* 			dup_mark(&dup, dst); */
/* 			dup_mark(&dup, dst->prev->prev->prev); */
/* 			dup_next(&dup); */
/* 			dup_mark(&dup, src->prev); */
/* 			dup_mark(&dup, dst->prev); */
/* 			dup_mark(&dup, dst->prev->prev->prev->prev); */
/* 			dup_next(&dup); */
/* 			dup_mark(&dup, src->prev->prev); */
/* 			dup_mark(&dup, dst->prev->prev); */
/* 			dup_next(&dup); */
			break;

			/* pop 4 push 6 dup */
					
		case ICMD_DUP2_X2:
			lsra_from_stack(ls, src,b_index,iindex); /* from for to, or it will not work! inc usage_count! */
			lsra_from_stack(ls, src->prev,b_index,iindex); /*from for to, or it will not work!  inc usage_count! */
			lsra_from_stack(ls, src->prev->prev,b_index,iindex); /*from for to, or it will not work!  inc usage_count! */
			lsra_from_stack(ls, src->prev->prev->prev,b_index,iindex); /*from for to, or it will not work!  inc usage_count! */
			lsra_new_stack(ls,dst->prev->prev->prev->prev->prev,b_index,iindex);
			lsra_new_stack(ls,dst->prev->prev->prev->prev,b_index,iindex);
			lsra_new_stack(ls,dst->prev->prev->prev,b_index,iindex);
			lsra_new_stack(ls,dst->prev->prev,b_index,iindex);
			lsra_new_stack(ls,dst->prev,b_index,iindex);
			lsra_new_stack(ls,dst,b_index,iindex); 
/* 			dup_mark(&dup, src); */
/* 			dup_mark(&dup, dst); */
/* 			dup_mark(&dup, dst->prev->prev->prev->prev); */
/* 			dup_next(&dup); */
/* 			dup_mark(&dup, src->prev); */
/* 			dup_mark(&dup, dst->prev); */
/* 			dup_mark(&dup, dst->prev->prev->prev->prev->prev); */
/* 			dup_next(&dup); */
/* 			dup_mark(&dup, src->prev->prev); */
/* 			dup_mark(&dup, dst->prev->prev); */
/* 			dup_next(&dup); */
/* 			dup_mark(&dup, src->prev->prev->prev); */
/* 			dup_mark(&dup, dst->prev->prev->prev); */
/* 			dup_next(&dup); */
			break;

			/* pop 2 push 2 swap */
					
		case ICMD_SWAP:
			lsra_from_stack(ls, src,b_index,iindex); /* from for to, or it will not work! inc usage_count! */
			lsra_from_stack(ls, src->prev,b_index,iindex); /*from for to, or it will not work!  inc usage_count! */
			lsra_new_stack(ls,dst->prev,b_index,iindex);
			lsra_new_stack(ls,dst,b_index,iindex);
/* 			dup_mark(&dup, src); */
/* 			dup_mark(&dup, dst->prev); */
/* 			dup_next(&dup); */
/* 			dup_mark(&dup, src->prev); */
/* 			dup_mark(&dup, dst); */
/* 			dup_next(&dup); */
			break;

			/* pop 2 push 1 */
					
		case ICMD_IADD:
		case ICMD_ISUB:
		case ICMD_IMUL:
		case ICMD_IDIV:
		case ICMD_IREM:

		case ICMD_ISHL:
		case ICMD_ISHR:
		case ICMD_IUSHR:
		case ICMD_IAND:
		case ICMD_IOR:
		case ICMD_IXOR:

		case ICMD_LADD:
		case ICMD_LSUB:
		case ICMD_LMUL:
		case ICMD_LDIV:
		case ICMD_LREM:

		case ICMD_LOR:
		case ICMD_LAND:
		case ICMD_LXOR:

		case ICMD_LSHL:
		case ICMD_LSHR:
		case ICMD_LUSHR:

		case ICMD_FADD:
		case ICMD_FSUB:
		case ICMD_FMUL:
		case ICMD_FDIV:
		case ICMD_FREM:

		case ICMD_DADD:
		case ICMD_DSUB:
		case ICMD_DMUL:
		case ICMD_DDIV:
		case ICMD_DREM:

		case ICMD_LCMP:
		case ICMD_FCMPL:
		case ICMD_FCMPG:
		case ICMD_DCMPL:
		case ICMD_DCMPG:
			lsra_from_stack(ls, src,b_index,iindex);
			lsra_from_stack(ls, src->prev,b_index,iindex);
			lsra_new_stack(ls,dst,b_index,iindex);
			break;

			/* pop 1 push 1 */
		case ICMD_IADDCONST:
		case ICMD_ISUBCONST:
		case ICMD_IMULCONST:
		case ICMD_IDIVPOW2:
		case ICMD_IREMPOW2:
		case ICMD_IANDCONST:
		case ICMD_IORCONST:
		case ICMD_IXORCONST:
		case ICMD_ISHLCONST:
		case ICMD_ISHRCONST:
		case ICMD_IUSHRCONST:

		case ICMD_LADDCONST:
		case ICMD_LSUBCONST:
		case ICMD_LMULCONST:
		case ICMD_LDIVPOW2:
		case ICMD_LREMPOW2:
		case ICMD_LANDCONST:
		case ICMD_LORCONST:
		case ICMD_LXORCONST:
		case ICMD_LSHLCONST:
		case ICMD_LSHRCONST:
		case ICMD_LUSHRCONST:

		case ICMD_IFEQ_ICONST:
		case ICMD_IFNE_ICONST:
		case ICMD_IFLT_ICONST:
		case ICMD_IFGE_ICONST:
		case ICMD_IFGT_ICONST:
		case ICMD_IFLE_ICONST:

		case ICMD_INEG:
		case ICMD_INT2BYTE:
		case ICMD_INT2CHAR:
		case ICMD_INT2SHORT:
		case ICMD_LNEG:
		case ICMD_FNEG:
		case ICMD_DNEG:

		case ICMD_I2L:
		case ICMD_I2F:
		case ICMD_I2D:
		case ICMD_L2I:
		case ICMD_L2F:
		case ICMD_L2D:
		case ICMD_F2I:
		case ICMD_F2L:
		case ICMD_F2D:
		case ICMD_D2I:
		case ICMD_D2L:
		case ICMD_D2F:

		case ICMD_CHECKCAST:

		case ICMD_ARRAYLENGTH:
		case ICMD_INSTANCEOF:

		case ICMD_NEWARRAY:
		case ICMD_ANEWARRAY:

		case ICMD_GETFIELD:
			lsra_from_stack(ls, src,b_index,iindex);
			lsra_new_stack(ls,dst,b_index,iindex);
			break;

			/* pop 0 push 1 */
					
		case ICMD_GETSTATIC:
		case ICMD_NEW:
			lsra_new_stack(ls,dst,b_index,iindex);
			break;

			/* pop many push any */
		case ICMD_INVOKEVIRTUAL:
		case ICMD_INVOKESPECIAL:
		case ICMD_INVOKESTATIC:
		case ICMD_INVOKEINTERFACE:
			i = iptr->op1;
			while (--i >= 0) {
				lsra_from_stack(ls, src,b_index,iindex);
				src = src->prev;
			}
			if (((methodinfo*)iptr->val.a)->returntype != TYPE_VOID) {
				lsra_new_stack(ls,dst,b_index,iindex);
			}
			break;

		case ICMD_BUILTIN3:
			lsra_from_stack(ls, src,b_index,iindex);
			src = src->prev;
		case ICMD_BUILTIN2:
			lsra_from_stack(ls, src,b_index,iindex);
			src = src->prev;
		case ICMD_BUILTIN1:
			lsra_from_stack(ls, src,b_index,iindex);
			src = src->prev; /* ??????????? */
			if (iptr->op1 != TYPE_VOID)
				lsra_new_stack(ls,dst,b_index,iindex);
			break;

		case ICMD_MULTIANEWARRAY:
			i = iptr->op1;
			while (--i >= 0) {
				lsra_from_stack(ls, src,b_index,iindex);
				src = src->prev;
			}
			lsra_new_stack(ls,dst,b_index,iindex);
			break;

		default:
			printf("ICMD %d at %d\n", iptr->opc, (int)(iptr - m->instructions));
			panic("Missing ICMD code during register allocation");
		} /* switch */

	}
		/* 		dup_join(ls, &dup, b_index); */
}

#ifdef LSRA_TESTLT
int _test_lifetimes(methodinfo *m, lsradata *ls, int b_index, int *values, bool* bb_visited, struct lifetime *lifet)
{
	int b_new_index;
	struct lifetime *lt;
	struct _i_list *il;

	struct depthElement *de;
	struct stackslot *ss;
	int *v, i, j;

/* 	bb_visited[b_index]++; */
	
	b_new_index = b_index;

	if ((m->basicblocks[b_index].type == BBTYPE_EXH) || (m->basicblocks[b_index].type == BBTYPE_SBR)) {
		/* initialize first stackslot which comes from the "system" */
		for (lt = lifet; lt != NULL; lt = lt->next) {
			for (il=lt->i_list; il != NULL; il = il->next) {
				if ((il->b_index == b_new_index) && (il->instr == 0) && (il->store == LSRA_STORE)) {
					for (ss=lt->local_ss; (ss != NULL) && (ss->s != m->basicblocks[b_index].instack); ss = ss->next);
					if (ss != NULL) {
						values[lt->reg] = lt->v_index;
/* 						printf("BB %3i EXH/SBR init lt v_index %3i reg (%3i)=%3i\n", b_index, lt->v_index, lt->reg, values[lt->reg]); */
					}
				}
			}
		}
	}			 

	for (i = -1; i < m->basicblocks[b_index].icount; i++) {
		/* search for Reads (LSRA_LOAD or LSRA_POP) at (b_new_index, i) and verify values[lt->reg]==v_index*/
		for (lt = lifet; lt != NULL; lt = lt->next) {
			for (il=lt->i_list; il != NULL; il = il->next) {
				if ((il->b_index == b_new_index) && (il->instr == i) && ((il->store == LSRA_LOAD) || (il->store == LSRA_POP))) {
					if (values[lt->reg] != lt->v_index) {
						if (values[lt->reg] == VS)
							printf("BB %3i Instr %3i lt v_index %3i reg %3i Warning: not initialized\n", b_index, i, lt->v_index, lt->reg);
						else {
							printf("BB %3i Instr %3i lt v_index %3i reg %3i Error: %3i \n", b_index, i, lt->v_index, lt->reg, values[lt->reg]);
							printf("Backtracing: \n");
							return (lt->reg);
						}
					}
				}
			}
		}

		/* search for Writes (LSRA_STORE) and do it values[lt->reg]=lt->v_index */
		for (lt = lifet; lt != NULL; lt = lt->next) {
			for (il=lt->i_list; il != NULL; il = il->next) {
				if ((il->b_index == b_new_index) && (il->instr == i) && (il->store == LSRA_STORE)) {
					values[lt->reg]=lt->v_index;
/* 					printf("BB %3i Instr %3i v_index %3i reg(%3i)=%3i\n", b_index, i, lt->v_index, lt->reg, values[lt->reg]); */
				}
			}
		}
	}
 
/* 	for (de = ld->c_dTable[b_index]; de != NULL; de = de->next) { */
/* 		if (( de->value > b_index ) || (bb_visited[de->value] < 3)) { */ /* dont go in loops */
			/* make an own copy for each test */
/* 			v=DMNEW(int, VS); */
/* 			for (i=0; i<VS; i++) v[i]=values[i]; */
/* 			if ((i=_test_lifetimes(m, ld, ls, de->value, v, bb_visited, lifet)) != -1) { */
/* 				printf(" BB %3i ((%3i)=%3i) ",b_index, i, values[i]); */
/* 				return i; */
/* 			} */
/* 		} */
/* 	} */

	i=0;

	for (de = ld->c_dTable[b_index]; de != NULL; de = de->next)
		i++;

	if (i != 0) {
		j = rand() % i;

		for (i=0, de = ld->c_dTable[b_index]; i!=j; i++, de=de->next);

		if ((i=_test_lifetimes(m, ld, ls, de->value, values, bb_visited, lifet)) != -1) {
			/* 		printf(" BB %3i ((%3i)=%3i) ",b_index, i, values[i]); */
			return i;
		}
	}

	return -1;
}

void test_lifetimes( methodinfo *m, lsradata *ls, struct lifetime *lifet, codegendata *cd)
{
	int *values, i;
	int *bb_visited;
	int j, handler_min1, handler_min;
	struct depthElement *de;

	/* first patch in jumps from the last block in a guarded area (ls->ex[i]->guarded_max) */
	/* to the coresponding exceptionhandler (ls->ex[i]->handler_min), so the tests is  */
	/* valid for exceptions too. Afterwards eliminate this jumps again */
	
/* 	for (i=0; i < cd->exceptiontablelength; i++) { */
/* 		for (j=ls->bb_rev[ls->ex[i].guarded_max]; j>=0; j--) { */
			/* this patched jump only from blocks which are not deleted or exeptionhandler itself */
/* 			if (!((m->basicblocks[j].flags < BBREACHED) || (m->basicblocks[j].type == BBTYPE_EXH))) */
/* 				break; */
/* 		} */
/* 		if (j == -1) panic( "lsra_clean_Graph: Problem with Basic Block Order\n"); */
/* 		de=DNEW(struct depthElement); */

/* 		handler_min = ls->bb_rev[ls->ex[i].handler_min]; */
/* 		handler_min1 = ls->bb_rev[ls->ex[i].handler_max]; */
/* 		if (handler_min1 < handler_min) handler_min = handler_min1; */

/* 		de->value = handler_min; */
/* 		de->next = ld->c_dTable[j]; */
/* 		ld->c_dTable[j]=de; */
/* 	} */

	if ( (values = calloc( VS, sizeof(int))) == NULL )
		 panic("test_lifetimes: out of memory\n");

	for (j=0; j < 100; j++ ) {
		for (i=0; i < VS; i++) values[i]=VS;
/* 		bb_visited=DMNEW(bool, m->basicblockcount); */
/* 		for (i=0; i < m->basicblockcount; i++) bb_visited[i]=false; */

/* 		bb_visited[0]=1; */
		if (_test_lifetimes(m, ld, ls, 0, values, bb_visited, lifet) != -1) printf("\n");
	}


	free(values);

	/* now taking out the patched "exception" jumps again */
/* 	for (i=0; i < cd->exceptiontablelength; i++) { */
/* 		for (j=ls->bb_rev[ls->ex[i].guarded_max]; j>=0; j--) { */
 			/* this patched jump only from blocks which are not deleted or exeptionhandler itself */ 
/* 			if (!((m->basicblocks[j].flags < BBREACHED) || (m->basicblocks[j].type == BBTYPE_EXH))) */
/* 				break; */
/* 		} */
/* 		ld->c_dTable[j]=ld->c_dTable[j]->next; */
/* 	} */
}
#endif

#ifdef __I386__

#define NO   0
#define YES 64
#define L   32
#define D   16
#define DP   8
#define S    4
#define SP   2
#define SPP  1

struct tmp_reg icmd_uses_tmp[256] ={
{  NO,  NO,  NO},	/* ICMD_NOP */
{   D,  NO,  NO},	/* ICMD_ACONST */	
{  NO,  NO,  NO},	/* ICMD_NULLCHECKPOP */
{   D,  NO,  NO},	/* ICMD_ICONST */	
{  NO,  NO,  NO}, /* ICMD_UNDEF4	*/
{ YES,  NO,  NO}, /* ICMD_IDIVPOW2	*/ 
{ YES, YES,  NO}, /* ICMD_LDIVPOW2	*/
{  NO,  NO,  NO}, /* ICMD_UNDEF7	*/
{  NO,  NO,  NO}, /* ICMD_UNDEF8	*/
{   D,  NO,  NO},	/* ICMD_LCONST */	
{ YES, YES, YES}, /* ICMD_LCMPCONST	*/
{ YES,  NO,  NO}, /* ICMD_FCONST	*/	
{  NO,  NO,  NO}, /* ICMD_UNDEF12	*/
{  NO,  NO,  NO}, /* ICMD_ELSE_ICONST	*/
{ YES,  NO,  NO}, /* ICMD_DCONST	*/	
{  NO,  NO,  NO}, /* ICMD_IFEQ_ICONST	*/
{  NO,  NO,  NO}, /* ICMD_IFNE_ICONST	*/
{  NO,  NO,  NO}, /* ICMD_IFLT_ICONST	*/
{  NO,  NO,  NO}, /* ICMD_IFGE_ICONST	*/
{  NO,  NO,  NO}, /* ICMD_IFGT_ICONST	*/
{  NO,  NO,  NO}, /* ICMD_IFLE_ICONST	*/
{ L|D,  NO, NO}, /* ICMD_ILOAD	*/ 
{  NO,  NO,  NO}, /* ICMD_LLOAD	*/
{  NO,  NO,  NO}, /* ICMD_FLOAD	*/
{  NO,  NO,  NO}, /* ICMD_DLOAD	*/	
{ L|D,  NO,  NO}, /* ICMD_ALOAD	*/
{  NO,  NO,  NO}, /* ICMD_IADDCONST	*/
{  NO,  NO,  NO}, /* ICMD_ISUBCONST	*/
{   D,  NO,  NO}, /* ICMD_IMULCONST	*/
{  NO,  NO,  NO}, /* ICMD_IANDCONST	*/
{  NO,  NO,  NO}, /* ICMD_IORCONST	*/
{  NO,  NO,  NO}, /* ICMD_IXORCONST	*/
{  NO,  NO,  NO}, /* ICMD_ISHLCONST	*/
{  NO,  NO,  NO}, /* ICMD_ISHRCONST	*/
{  NO,  NO,  NO}, /* ICMD_IUSHRCONST	*/
{ YES,  NO,  NO}, /* ICMD_IREMPOW2	*/
{ S|D,  NO,  NO}, /* ICMD_LADDCONST	*/
{ YES,  NO,  NO}, /* ICMD_LSUBCONST	*/	
{ YES, YES, YES}, /* ICMD_LMULCONST	*/
{  NO,  NO,  NO}, /* ICMD_LANDCONST	*/
{  NO,  NO,  NO}, /* ICMD_LORCONST	*/
{  NO,  NO,  NO}, /* ICMD_LXORCONST	*/
{ YES, YES,  NO}, /* ICMD_LSHLCONST	*/
{ YES, YES,  NO}, /* ICMD_LSHRCONST	*/
{ YES, YES,  NO}, /* ICMD_LUSHRCONST	*/
{ YES, YES,  NO}, /* ICMD_LREMPOW2	*/
{ YES, YES,  NO}, /* ICMD_IALOAD	*/
{ YES, YES, YES}, /* ICMD_LALOAD	*/
{ YES, YES,  NO}, /* ICMD_FALOAD	*/
{ YES, YES,  NO}, /* ICMD_DALOAD	*/
{ YES, YES,  NO}, /* ICMD_AALOAD	*/
{ YES, YES,  NO}, /* ICMD_BALOAD	*/
{ YES, YES,  NO}, /* ICMD_CALOAD	*/
{ YES, YES,  NO}, /* ICMD_SALOAD	*/
{ L|S,  NO,  NO}, /* ICMD_ISTORE	*/
{  NO,  NO,  NO}, /* ICMD_LSTORE	*/
{  NO,  NO,  NO}, /* ICMD_FSTORE	*/
{  NO,  NO,  NO}, /* ICMD_DSTORE	*/	
{ L|S,  NO,  NO}, /* ICMD_ASTORE	*/
{ YES, YES,  NO}, /* ICMD_IF_LEQ	*/
{ YES, YES,  NO}, /* ICMD_IF_LNE	*/
{  NO,  NO,  NO}, /* ICMD_IF_LLT	*/
{  NO,  NO,  NO}, /* ICMD_IF_LGE	*/
{  NO,  NO,  NO}, /* ICMD_IF_LGT	*/
{  NO,  NO,  NO}, /* ICMD_IF_LLE	*/
{ YES, YES,  NO}, /* ICMD_IF_LCMPEQ	*/
{ YES, YES,  NO}, /* ICMD_IF_LCMPNE	*/
{ YES,  NO,  NO}, /* ICMD_IF_LCMPLT	*/
{ YES,  NO,  NO}, /* ICMD_IF_LCMPGE	*/
{ YES,  NO,  NO}, /* ICMD_IF_LCMPGT	*/
{ YES,  NO,  NO}, /* ICMD_IF_LCMPLE	*/
{  NO,  NO,  NO}, /* ICMD_UNDEF71	*/
{  NO,  NO,  NO}, /* ICMD_UNDEF72	*/
{  NO,  NO,  NO}, /* ICMD_UNDEF73	*/
{  NO,  NO,  NO}, /* ICMD_UNDEF74	*/
{  NO,  NO,  NO}, /* ICMD_UNDEF75	*/
{  NO,  NO,  NO}, /* ICMD_UNDEF76	*/
{  NO,  NO,  NO}, /* ICMD_UNDEF77	*/
{  NO,  NO,  NO}, /* ICMD_UNDEF78	*/
{ YES, YES, YES}, /* ICMD_IASTORE	*/
{ YES, YES, YES}, /* ICMD_LASTORE	*/
{ YES, YES,  NO}, /* ICMD_FASTORE	*/
{ YES, YES,  NO}, /* ICMD_DASTORE	*/
{ YES, YES,  YES}, /* ICMD_AASTORE	*/
{ YES, YES, YES}, /* ICMD_BASTORE	*/
{ YES, YES, YES}, /* ICMD_CASTORE	*/
{ YES, YES, YES}, /* ICMD_SASTORE	*/
{  NO,  NO,  NO}, /* ICMD_POP	*/
{  NO,  NO,  NO}, /* ICMD_POP2	*/
{ S|D,  NO,  NO}, /* ICMD_DUP	*/
{ YES,  NO,  NO}, /* ICMD_DUP_X1	*/
{ YES,  NO,  NO}, /* ICMD_DUP_X2	*/
{ S|D|SP|DP, NO, NO}, /* ICMD_DUP2	*/
{ YES,  NO,  NO}, /* ICMD_DUP2_X1	*/
{ YES,  NO,  NO}, /* ICMD_DUP2_X2	*/	
{ YES,  NO,  NO}, /* ICMD_SWAP	*/
{  NO,  NO,  NO}, /* ICMD_IADD	*/
{ YES,  NO,  NO}, /* ICMD_LADD	*/
{  NO,  NO,  NO}, /* ICMD_FADD	*/
{  NO,  NO,  NO}, /* ICMD_DADD	*/
{ YES,  NO,  NO}, /* ICMD_ISUB	*/
{ YES,  NO,  NO}, /* ICMD_LSUB	*/
{  NO,  NO,  NO}, /* ICMD_FSUB	*/
{  NO,  NO,  NO}, /* ICMD_DSUB	*/
{   D, NO, NO}, /* ICMD_IMUL	*/
{ YES,  YES, YES}, /* ICMD_LMUL	*/
{  NO,  NO,  NO}, /* ICMD_FMUL	*/
{  NO,  NO,  NO}, /* ICMD_DMUL	*/
{  YES,   S,  NO}, /* ICMD_IDIV	*/
{  YES, YES,  YES}, /* ICMD_LDIV	*/
{  NO,  NO,  NO}, /* ICMD_FDIV	*/
{  NO,  NO,  NO}, /* ICMD_DDIV	*/
{ YES,  S,  YES}, /* ICMD_IREM	*/
{ YES, YES, YES}, /* ICMD_LREM	*/
{  NO,  NO,  NO}, /* ICMD_FREM	*/
{  NO,  NO,  NO}, /* ICMD_DREM	*/
{ S|D,  NO,  NO}, /* ICMD_INEG	*/	
{ YES,  NO,  NO}, /* ICMD_LNEG	*/	
{  NO,  NO,  NO}, /* ICMD_FNEG	*/
{  NO,  NO,  NO}, /* ICMD_DNEG	*/
{  NO,  NO,  NO}, /* ICMD_ISHL	*/
{ YES, YES, YES}, /* ICMD_LSHL	*/
{  NO,  NO,  NO}, /* ICMD_ISHR	*/
{ YES, YES, YES}, /* ICMD_LSHR	*/
{  NO,  NO,  NO}, /* ICMD_IUSHR	*/
{ YES, YES, YES}, /* ICMD_LUSHR	*/
{  NO,  NO,  NO}, /* ICMD_IAND	*/
{  NO,  NO,  NO}, /* ICMD_LAND	*/
{  NO,  NO,  NO}, /* ICMD_IOR	*/
{  NO,  NO,  NO}, /* ICMD_LOR	*/
{  NO,  NO,  NO}, /* ICMD_IXOR	*/
{  NO,  NO,  NO}, /* ICMD_LXOR	*/
{  NO,  NO,  NO}, /* ICMD_IINC	*/
{ S|D,  NO,  NO}, /* ICMD_I2L	*/
{ YES,  NO,  NO}, /* ICMD_I2F	*/
{ YES,  NO,  NO}, /* ICMD_I2D	*/
{ S|D,  NO,  NO}, /* ICMD_L2I	*/
{  NO,  NO,  NO}, /* ICMD_L2F	*/
{  NO,  NO,  NO}, /* ICMD_L2D	*/
{  YES, NO,  NO}, /* ICMD_F2I	*/
{  YES, NO, YES}, /* ICMD_F2L	*/
{  NO,  NO,  NO}, /* ICMD_F2D	*/
{  YES, NO,  NO}, /* ICMD_D2I	*/
{  YES, NO, YES}, /* ICMD_D2L	*/
{  NO,  NO,  NO}, /* ICMD_D2F	*/
{ S|D,  NO,  NO}, /* ICMD_INT2BYTE	*/
{ S|D,  NO,  NO}, /* ICMD_INT2CHAR	*/
{ S|D,  NO,  NO}, /* ICMD_INT2SHORT	*/
{ YES, YES, YES}, /* ICMD_LCMP	*/
{ YES,  NO,  NO}, /* ICMD_FCMPL	*/
{ YES,  NO,  NO}, /* ICMD_FCMPG	*/
{ YES,  NO,  NO}, /* ICMD_DCMPL	*/
{ YES,  NO,  NO}, /* ICMD_DCMPG	*/
{  NO,  NO,  NO}, /* ICMD_IFEQ	*/
{  NO,  NO,  NO}, /* ICMD_IFNE	*/
{  NO,  NO,  NO}, /* ICMD_IFLT	*/
{  NO,  NO,  NO}, /* ICMD_IFGE	*/
{  NO,  NO,  NO}, /* ICMD_IFGT	*/
{  NO,  NO,  NO}, /* ICMD_IFLE	*/
{ S|SP, NO,  NO}, /* ICMD_IF_ICMPEQ	*/
{ S|SP, NO,  NO}, /* ICMD_IF_ICMPNE	*/
{ S|SP, NO,  NO}, /* ICMD_IF_ICMPLT	*/
{ S|SP, NO,  NO}, /* ICMD_IF_ICMPGE	*/
{ S|SP, NO,  NO}, /* ICMD_IF_ICMPGT	*/
{ S|SP, NO,  NO}, /* ICMD_IF_ICMPLE	*/
{ S|SP, NO,  NO}, /* ICMD_IF_ACMPEQ	*/
{ S|SP, NO,  NO}, /* ICMD_IF_ACMPNE	*/
{  NO,  NO,  NO}, /* ICMD_GOTO	*/
{  YES, NO,  NO}, /* ICMD_JSR	*/
{   L,  NO,  NO}, /* ICMD_RET	*/
{ YES, YES,  NO}, /* ICMD_TABLESWITCH	*/
{ YES,  NO,  NO}, /* ICMD_LOOKUPSWITCH	*/
{ YES, YES, YES}, /* ICMD_IRETURN	*/
{ YES, YES, YES}, /* ICMD_LRETURN	*/
{ YES, YES, YES}, /* ICMD_FRETURN	*/
{ YES, YES, YES}, /* ICMD_DRETURN	*/
{ YES, YES, YES}, /* ICMD_ARETURN	*/
{ YES, YES, YES}, /* ICMD_RETURN	*/
{ YES, YES,  NO}, /* ICMD_GETSTATIC	*/   
{ YES, YES,  NO}, /* ICMD_PUTSTATIC	*/   
{ YES, YES,  NO}, /* ICMD_GETFIELD	*/    
{ YES, YES,  NO}, /* ICMD_PUTFIELD	*/   
{ YES, YES, YES}, /* ICMD_INVOKEVIRTUAL	*/    
{ YES, YES, YES}, /* ICMD_INVOKESPECIAL	*/  
{ YES, YES, YES}, /* ICMD_INVOKESTATIC	*/     
{ YES, YES, YES}, /* ICMD_INVOKEINTERFACE*/
{  NO,  NO,  NO}, /* ICMD_CHECKASIZE	*/
{ YES, YES, YES}, /* ICMD_NEW	*/
{ YES, YES, YES}, /* ICMD_NEWARRAY	*/
{ YES, YES, YES}, /* ICMD_ANEWARRAY	*/
{ YES,  NO,  NO}, /* ICMD_ARRAYLENGTH	*/
{ YES, YES, YES}, /* ICMD_ATHROW	*/
{ YES, YES, YES}, /* ICMD_CHECKCAST	*/
{ YES, YES, YES}, /* ICMD_INSTANCEOF	*/
{ YES, YES, YES}, /* ICMD_MONITORENTER	*/
{ YES, YES, YES}, /* ICMD_MONITOREXIT	*/
{  NO,  NO,  NO}, /* ICMD_UNDEF196	*/
{ YES,  NO,  NO}, /* ICMD_MULTIANEWARRAY	*/
{  NO,  NO,  NO}, /* ICMD_IFNULL	*/
{  NO,  NO,  NO}, /* ICMD_IFNONNULL	*/
{ YES, YES, YES}, /* ICMD_UNDEF200	*/
{ YES, YES, YES}, /* ICMD_UNDEF201	*/
{ YES, YES, YES}, /* ICMD_UNDEF202	*/
{ YES,  NO,  NO}, /* ICMD_CHECKEXCEPTION	*/
{ YES, YES,  NO}, /* ICMD_IASTORECONST	*/
{ YES, YES,  NO}, /* ICMD_LASTORECONST	*/
{ YES, YES, YES}, /* ICMD_FASTORECONST	*/
{ YES, YES, YES}, /* ICMD_DASTORECONST	*/
{ YES, YES, YES}, /* ICMD_AASTORECONST	*/
{ YES, YES, YES}, /* ICMD_BASTORECONST	*/
{ YES, YES, YES}, /* ICMD_CASTORECONST	*/
{ YES, YES, YES}, /* ICMD_SASTORECONST	*/
{  NO,  NO,  NO}, /* 	"UNDEF"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF"	*/
{  NO,  NO,  NO}, /* 	"UNDEF216"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF217"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF218"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF219"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF220",	*/
{  NO,  NO,  NO}, /* 	"UNDEF"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF",	*/
{  NO,  NO,  NO}, /* 	"UNDEF226"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF227"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF228"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF229"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF230",	*/
{  NO,  NO,  NO}, /* 	"UNDEF"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF",	*/
{  NO,  NO,  NO}, /* 	"UNDEF236"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF237"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF238"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF239"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF240",	*/
{  NO,  NO,  NO}, /* 	"UNDEF"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF",	*/
{  NO,  NO,  NO}, /* 	"UNDEF246"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF247"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF248"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF249"	*/
{  NO,  NO,  NO}, /* 	,"UNDEF250",	*/
{  NO,  NO,  NO}, /* ICMD_INLINE_START	*/
{  NO,  NO,  NO}, /* ICMD_INLINE_END	*/
{ YES,  NO, YES}, /* ICMD_BUILTIN3	*/  
{ YES,  NO, YES}, /* ICMD_BUILTIN2	*/   
{ YES,  NO, YES}, /* ICMD_BUILTIN1	*/   
};
#endif


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
