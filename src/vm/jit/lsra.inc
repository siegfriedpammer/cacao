/* vm/jit/lsra.inc - linear scan register allocator

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

   $Id: lsra.inc 1621 2004-11-30 13:06:55Z twisti $

*/

/* #define LSRA_DEBUG */
/* #define LSRA_SAVEDVAR */
/* #define LSRA_MEMORY */
/* #define LSRA_PRINTLIFETIMES */


#include <stdio.h>
#include <stdlib.h>

#ifdef LSRA_DEBUG
#define LSRA_PRINTLIFETIMES
#endif

#include "mm/memory.h"
#include "vm/options.h"
#include "vm/jit/lsra.h"
#include "vm/jit/reg.h"
#include "vm/jit/loop/graph.h"
#include "vm/jit/loop/loop.h"


void lsra(methodinfo *m, codegendata *cd, registerdata *rd, loopdata *ld, t_inlining_globals *id)
{

	lsradata *ls;

#ifdef LSRA_DEBUG
	char name[1256], name1[1256];

	utf_sprint(name, m->class->name);
	utf_sprint(name1, m->name);
	strcat(name, name1);
	utf_sprint(name1, m->descriptor);
	strcat(name, name1);


	printf("LSRA Start for %s\n", name); 

	if (strcmp(name,"java/lang/SysteminitProperties()V")==0) {
		printf("-------------------\n");
	}
#endif

	ls=DNEW(lsradata);
	lsra_init(m, cd, id, ls);
	lsra_setup(m, cd, rd, ls, ld);
	

	/* Run LSRA */
	lsra_main(m, ls, rd);

	return;
}

void lsra_init(methodinfo *m, codegendata *cd, t_inlining_globals *id, lsradata *ls) 
{
	int i;

	/* Init LSRA Data Structures */
	ls->back_edge_panic=false;
	/* lifetimes für alle Basicblocks allokieren */
	ls->ss_lifetimes=DMNEW(struct lifetime *, m->basicblockcount);
	for (i=0; i<m->basicblockcount; i++) ls->ss_lifetimes[i]=NULL;
#ifdef LSRA_DEBUG
	if (cd->maxlocals != id->cumlocals) panic("lsra: Welche locals nehmen?\n");
#endif
	ls->locals_lifetimes=DMNEW(struct lifetime *, cd->maxlocals);
	for (i=0; i < cd->maxlocals; i++) ls->locals_lifetimes[i]=NULL;
	ls->lifetimes=NULL;
	ls->stackslots=NULL;
}

void lsra_setup(methodinfo *m, codegendata *cd, registerdata *rd, lsradata *ls, loopdata *ld)
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
	
	/* Setup LSRA Data structures */
	if (opt_loops) panic("lsra with -oloop not possible!\n");
	if (!opt_loops) {
		depthFirst(m, ld);
		analyseGraph(m, ld);
	}

	/* BBDELETED Blöcke aus loopdata->c_dTable rausschmeissen! */
	/* Exceptionhandler in loopdata->c_dTable hinzufügen       */
#ifdef LSRA_DEBUG
	printf("LSRA lsra_clean_Graph\n");
#endif
	lsra_clean_Graph(m, cd, ls, ld);

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

	printf("LSRA lsra_dump_Graph\n");
	lsra_dump_Graph(m, ld->c_dTable);
#endif


	/* Parameter initialisieren = local Vars schreibzugriff bei 0,0*/

 	for (p = 0, i = 0; p < m->paramcount; p++) {
 		t = m->paramtypes[p];

		if (rd->locals[i][t].type >= 0)	
			lsra_usage_local(ls, i, t, 0, 0, LSRA_STORE);
 		i++;
 		if (IS_2_WORD_TYPE(t))    /* increment local counter for 2 word types */
 			i++;
	}  /* end for */

#ifdef LSRA_DEBUG
	printf("LSRA lsra_scan_register_canditates\n");
#endif
	lsra_scan_registers_canditates(m, ls);
	lsra_join_lifetimes(m, cd, ls, ld);

	v_index=-1;

	/* ls->lifetimes contains only the joined stackslotlifetimes */
	for (lt=ls->lifetimes; lt != NULL; lt=lt->next) {
		lt->v_index=v_index;
#ifdef LSRA_SAVEDVAR
		lt->savedvar=SAVEDVAR;
#endif
		for (ss=lt->local_ss; ss != NULL; ss=ss->next) {
			ss->s->varnum=v_index;
			ss->s->varkind=TEMPVAR; /* just another time */
		}
	}

	/* add ss_lifetimes[i] to ls->lifetimes or local_lifetimes[lt->s->varnum] */
	for (i=0; i < m->basicblockcount; i++) {
		if (m->basicblocks[i].flags >= BBREACHED) {
			for (; ls->ss_lifetimes[i] != NULL;) {
				lt=ls->ss_lifetimes[i];
#ifdef LSRA_SAVEDVAR
				lt->savedvar=SAVEDVAR;
#endif
#ifdef LSRA_DEBUG
				if (lt->local_ss == NULL) panic("lsra_setup: normal Stackslot Lifetimes local_ss == NULL\n");
#endif
				drop=false;
				for (ss=lt->local_ss; (ss!=NULL) && (!drop); ss=ss->next) {
					if (lt->local_ss->next == NULL) { /* only one Stackslot in local_ss */
						/* Special Treatment for "lonely" LOCAL and ARGVARs */
						if (ss->s->varkind == LOCALVAR) {
							/* join with LOCALVAR */
							/* local Lifetime vom richtigen Type suchen */
							for (n=ls->locals_lifetimes[lt->local_ss->s->varnum]; (n!=NULL) && (n->type!=lt->local_ss->s->type);n=n->next);
							lsra_merge_i_lists(n, lt);
							if (n->local_ss == NULL)  /* "pure" Local without Stackslots */
								n->local_ss = lt->local_ss;
							else
								lsra_merge_local_ss(n, lt);

							drop = true;
						}
						if (lt->local_ss->s->varkind == ARGVAR) {
							drop = true;
						}
					} else { 
						/* no special treatment (only one Stackslot Lifetimes)? */
						ss->s->varnum=v_index;
						ss->s->varkind=TEMPVAR; /* only TEMPVAR possible for now */
					}
				}
				if (drop)
					ls->ss_lifetimes[i]=lt->next;
				else {
					/* link into ls->lifetimes */
					ls->ss_lifetimes[i]=lt->next;
					lt->next=ls->lifetimes;
					ls->lifetimes=lt;
					lt->v_index=v_index--;
				}
			} /* for */
		} /* if */
	}

	/* add local_lifetimes to lifetimes */
	for (i=0; i < cd->maxlocals; i++) {
		if (ls->locals_lifetimes[i] != NULL) {
			for (lt=ls->locals_lifetimes[i]; lt->next != NULL; lt=lt->next);
			lt->next=ls->lifetimes;
			ls->lifetimes=ls->locals_lifetimes[i];
		}
	}	

	/* calc lifetime length */
#ifdef LSRA_DEBUG
	printf("Lifetimes before calc_lifetime_length: \n");
	print_lifetimes(rd, ls->lifetimes);
	printf("LSRA lsra_calc_lifetime_lengthe\n");
#endif
	lsra_calc_lifetime_length(m, ls, cd, ld);
}

int lsra_get_sbr_end(methodinfo *m, loopdata *ld, int bb, int *bb_visited)
{
	int j, bb_end;
	struct depthElement *de;
	instruction *ip;

	bb_visited[bb]=true;
	ip = m->basicblocks[bb].iinstr + m->basicblocks[bb].icount -1;/* set ip to last instruction			*/
	if (ip->opc == ICMD_RET)
		return bb;
	/* This Block is not the end -> search all following Blocks */
	j=bb_end=-1;
	for (de=ld->c_dTable[bb]; de != NULL; de=de->next) {
		if (!bb_visited[de->value]) {
			j=lsra_get_sbr_end(m, ld, de->value, bb_visited);
			if (j!=-1) { /* No new return path found */
				if ((bb_end != -1) && (j!=bb_end)) panic("SBR has more than one exit Blocks\n");
				bb_end=j;
			}
		}
	}
	return bb_end;
}

void lsra_clean_Graph( methodinfo *m, codegendata *cd, lsradata *ls, loopdata *ld)
{
	int i,j;
	struct depthElement *de, *n;
	int *bb_succ, *bb_visited, *ptr, index;
	exceptiontable *ex;
	struct depthElement **table;
	bool back_edge;
	struct LoopContainer *lc;

	table=ld->c_dTable;

	if (table == NULL) {
		return;
	}

	/* Exceptionhandler noch in c_dTable aufnehmen */
	ex=cd->exceptiontable;
#ifdef LSRA_DEBUG
	printf("ExTable(%i): ", cd->exceptiontablelength);
#endif

	for (; ex != NULL; ex = ex->down) {

#ifdef LSRA_DEBUG
		printf("%i ",ex->handler->debug_nr);
#endif
		dF(m, ld, -1, ex->handler->debug_nr);
	}


	/* Setting up successors of deleted Basic Blocks, in case c_dTable has an edge */
	/* to a deleted block, so it can be replaced with the next "normal" Block */
	bb_succ= DMNEW(int, m->basicblockcount);
	for (i=0; i < m->basicblockcount; i++) {
		if (m->basicblocks[i].flags >= BBREACHED)
			bb_succ[i]=i;
		else {
			for(j=i; ((j < m->basicblockcount) && (m->basicblocks[j].flags < BBREACHED)); j++);
			if (j < m->basicblockcount)
				bb_succ[i]=j;
			else
				bb_succ[i]=-1;
		}
	}

	back_edge=false;
	for(i=0; i < m->basicblockcount; i++) {
		if (m->basicblocks[i].flags < BBREACHED) {
			table[i]=NULL;
		} else {
			for (de=table[i]; de != NULL; de=de->next) {
				if (de->value < i) back_edge=true;
				if (bb_succ[de->value] != de->value)
					de->value = bb_succ[de->value];
				if (de->value == -1) panic("lsra_clean_Graph: Sprung ins nichts....");
			}
		}
	}

	if (back_edge) {
		if ( ld->c_allLoops == NULL ) {
			/* Keine Loops in Datenstruktur aber backedge!                     */
			/* sollte nach BasicBlock umsortieren (revererse Depth First sort) */
			/* und überprüfen von jit/loop/loop.c nicht mehr nötig sein        */
			/* TODO bis dahin eine loop über die backedge eintragen            */
			/*      anstatt back_edge_panic zu setzen und alle Lifetimes über die */
			/*      gesamt Methode auszudehnen                                    */
/* 			ls->back_edge_panic = true; */

			/* create loops from all back edges */
			
			for(i=0; i < m->basicblockcount; i++) {
				if (m->basicblocks[i].flags >= BBREACHED) {
					for (de=table[i]; de != NULL; de=de->next) {
						if (de->value < i) {
							if (ld->c_allLoops == NULL) {
								ld->c_allLoops = lc = DNEW(struct LoopContainer);
							} else {
								lc->next=DNEW(struct LoopContainer);
								lc=lc->next;
							}
							lc->nodes=DNEW(struct LoopElement);
							lc->nodes->next=DNEW(struct LoopElement);
							lc->nodes->next->next=NULL;

							lc->nodes->block=&(m->basicblocks[i]);
							lc->nodes->next->block=&(m->basicblocks[de->value]);
							lc->next = NULL;
						}
					}
				}
			}
#ifdef LSRA_DEBUG
			printf("-------------Warnung Back Edge + no LOOP..\n");
#endif
		}
	}

	bb_visited=DMNEW(int, m->basicblockcount);
	for (i=0; i<m->basicblockcount; i++) {
		bb_visited[i]=false;
	}

	/* Add all return possibilities to subroutine returns!           */
	/* --- subroutines will be inlined ---- -> then cancel this part */
#ifdef LSRA_DEBUG
	printf("LSRA Subroutine patching\n");
#endif
	for (i=0; i < m->basicblockcount; i++ ) {
		/* Search all Subroutine Headers */
		if (m->basicblocks[i].type == BBTYPE_SBR) {
#ifdef LSRA_DEBUG
			printf("Subroutine at BB %3i num pred: %3i Pred: ",i, ld->c_numPre[i]);
			for (ptr=ld->c_pre[i], index=0; index < ld->c_numPre[i]; ++index, ++ptr)
				printf("%3i ", *ptr);
			printf("\n");
#endif
			if (ld->c_numPre[i] > 1) { /* only if more than one call */
#ifdef LSRA_DEBUG
				printf("Searching End of Subroutine: ");
#endif
				j=lsra_get_sbr_end(m, ld, i, bb_visited); /* get Ending Block of Subroutine */
#ifdef LSRA_DEBUG
				printf("%3i \n",j);
#endif
				/* ensure, that all Predecessors+1 of the Subroutine Header are in the list */
				/* in the List is only one Predecessor */
#ifdef LSRA_DEBUG
				if (ld->c_dTable[j] == NULL) panic("No SBR Return in c_dTable\n");
				if (ld->c_dTable[j]->next != NULL) panic("More than one SBR Return in c_dTable\n");
#endif
				de=ld->c_dTable[j];
				for (ptr=ld->c_pre[i], index=0; index < ld->c_numPre[i]; ++index, ++ptr) {
					if (*ptr+1!=de->value) { /* Make new Entry */
						n=DNEW(struct depthElement);
						n->value=*ptr+1;
						n->next=de->next;
						de->next=n;
					}
				}
			}
#ifdef LSRA_DEBUG
			printf( "(%3i)table[%3i %3i]:  ",m->basicblocks[j].flags,j,m->basicblocks[j].debug_nr);
			for (de=ld->c_dTable[j]; de != NULL; de=de->next) {
				printf( "%3i ", de->value);
			}
			printf("\n");
#endif
		}
	}
}

#ifdef LSRA_DEBUG
void lsra_dump_Graph(methodinfo *m, struct depthElement **table)
{
	int i;
	struct depthElement *de;
	
	if (table == NULL) {
		printf ("table == NULL\n");
		return;
	}

	for(i=0; i < m->basicblockcount; i++) {

			switch (m->basicblocks[i].type) {

			case BBTYPE_STD:
				printf("STD ");
				break;
			case BBTYPE_EXH:
				printf("EXH ");
				break;
			case BBTYPE_SBR:
				printf("SBR ");
				break;

			default:
				printf("%3i ", m->basicblocks[i].flags);
				break;
			}

			printf( "(F%3i)table[%3i %3i]:  ",m->basicblocks[i].flags,i,m->basicblocks[i].debug_nr);
			for (de=table[i]; de != NULL; de=de->next) {
				printf( "%3i ", de->value);
		}
		printf("\n");
	}
	printf( "table dump end\n");
}
#endif

void lsra_main(methodinfo *m, lsradata *ls, registerdata *rd)
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
		for (i=0; i<reg_count ; i++)
			if (nregdescint[i]==REG_SAV) {
				reg[sav_reg_count].reg_index=i;
				reg[sav_reg_count].use=0;
				sav_reg_count++;
			}
		tmp_reg_count=sav_reg_count;
		for (i=0; i<reg_count ; i++) {
#if defined(__I386__)
			if ((method_uses_ecx) && (i==ECX)) continue;
			if ((method_uses_edx) && (i==EDX)) continue;
#endif
			if (nregdescint[i]==REG_TMP) {
				reg[tmp_reg_count].reg_index=i;
				reg[tmp_reg_count].use=0;
				tmp_reg_count++;
			}
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
		for (i=0; i<reg_count ; i++)
			if ((nregdescfloat[i]==REG_SAV) || (m->isleafmethod && (nregdescfloat[i]==REG_ARG))) {
				reg[sav_reg_count].reg_index=i;
				reg[sav_reg_count].use=0;
				sav_reg_count++;
			}

		tmp_reg_count=sav_reg_count;
		for (i=0; i<reg_count ; i++)
			if (nregdescfloat[i]==REG_TMP) {
				reg[tmp_reg_count].reg_index=i;
				reg[tmp_reg_count].use=0;
				tmp_reg_count++;
			}
		_lsra_main(m,ls, flt_lt, reg, tmp_reg_count, sav_reg_count, &lsra_mem_use, &lsra_reg_use);
		if (lsra_reg_use > rd->savfltregcnt) lsra_reg_use=rd->savfltregcnt;
	}

	rd->maxsavfltreguse=lsra_reg_use;

#ifndef SPECIALMEMUSE
#if defined(__X86_64__)
	/*
	 * XXX: we have a problem here, but allocating a little more stack space
	 *      is better than having a bug
	 */
	/*  	if (arguments_num > (intreg_argnum + fltreg_argnum)) */
	/*  		ifmemuse = arguments_num - (intreg_argnum + fltreg_argnum); */
	if (rd->arguments_num > rd->fltreg_argnum)
		lsra_mem_use = rd->arguments_num - rd->fltreg_argnum;
#else
	if (rd->arguments_num > rd->intreg_argnum)
		lsra_mem_use = rd->arguments_num - rd->intreg_argnum;
#endif
	else
		lsra_mem_use = 0;
#endif
#ifdef SPECIALMEMUSE
	lsra_mem_use = rd->ifmemuse;
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
	print_lifetimes(rd, int_lt);

	printf("Flt RA complete \n");
	printf("Lifetimes after splitting flt:\n");
	print_lifetimes(rd, flt_lt);

	printf("Rest RA complete \n");
	printf("Lifetimes after leftt:\n");
	print_lifetimes(rd, ls->lifetimes);
#endif

	rd->maxmemuse=lsra_mem_use;
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

void _lsra_main( methodinfo *m, lsradata *ls, struct lifetime *lifet, 	struct lsra_reg *reg, int tmp_reg_count, int sav_reg_count, int *mem_use, int *reg_use)
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
	if (lt->reg == -2) panic("spill_at_intervall: Keine Register mehr frei gewesen!\n");
}

void _spill_at_intervall(struct lifetime *lt, struct active_lt **active, int *active_count)
{
	struct active_lt *alt,*alt1;
	if (*active == NULL) {
		lt->reg=-2;
		return;
	}
	/* get last intervall from active */
	for (alt1=alt=*active; alt->next != NULL; alt1=alt, alt=alt->next);
	
	if ((alt->lt->i_end > lt->i_end) && (alt->lt->usagecount < lt->usagecount)) {
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

int lsra_getmaxblock(methodinfo *m, loopdata *ld, int bb_start)
{
	int i, *blocks;
	int finished;
	struct depthElement *hp;
	int bb_max;
	
	blocks=DMNEW(int, m->basicblockcount);
	/* ExTable Ablauf kann in die STD Code BB wieder "hineinspringen" */
	/* -> deshalb */
	/* TODO vorher alle "normalen" BB mit 2=bereits besucht initialisieren */
	bb_max=0;

	for (i=0; i<m->basicblockcount; i++) blocks[i]=-1;

	blocks[bb_start]=1;

	finished=0;
	while (!finished) {
		finished=1;
		for (i=0; i<m->basicblockcount; i++) {
	
			if (blocks[i] == 1) {
				if (i > bb_max) bb_max = i;
				blocks[i]=2; /* visited */
				for (hp=ld->c_dTable[i]; hp!=NULL; hp=hp->next) {
					if (blocks[hp->value] != 2) {
						blocks[hp->value]=1; /* to visit */
						finished=0;
					}
				}
			}
		}
	}
#ifdef LSRA_DEBUG
	printf("ExTable searching: BB_MAX %3i ",bb_max);
	for (i=0; i<m->basicblockcount; i++) 
		if (blocks[i] != -1) printf("%3i ",i);
	printf("\n");
#endif
	return bb_max;
}


void lsra_calc_lifetime_length(methodinfo *m, lsradata *ls, codegendata *cd, loopdata *ld)
{
	struct lifetime *lt;
	struct _i_list *il;
	struct l_loop *loops;
	struct b_loop *block_loop;
	exceptiontable *ex;
	int *exh_max; /* Exception Handler End BB */
	int *exh_min; /* Exception Handler Start BB */
	int *ex_max;  /* Exception guarded Area End BB */
	int *ex_min;  /* Exception guarded Area Start BB */

	int blast,bfirst,ilast,ifirst,usage;

	int i, j, num_loops;
	struct LoopContainer *lc;
	struct LoopElement *le;

	/**********/
	/* Todo: */
	/* Für Exceptionhandler Blocks wurde keine Loop Analyse durchgeführt! */
	/* -> deshalb vorerst die einzelnen Handler als eine Loop eintragen */
	/* oder loop analyse extra für jeden Handler aufrufen */
	/**/
	/* lifetime der Vars des ExHandlers über guarded Bereich ausdehnen! */
	/**/
	/* Falls die einzelnen Blöcke einer Loop nicht durchgehend nummeriert sind */
	/* auch nicht alle in block_loop eintragen! */

	exh_max = DMNEW(int, cd->exceptiontablelength); 
	exh_min = DMNEW(int, cd->exceptiontablelength); 
	ex_max =  DMNEW(int, cd->exceptiontablelength); 
	ex_min =  DMNEW(int, cd->exceptiontablelength); 

	/* BB der Exhandler bestimmen + BB der guarded Area hinzu */
	ex = cd->exceptiontable;
	for (i=0; ex != NULL; i++,ex = ex->down) {
 		if (ex->handler == NULL) {
#ifdef LSRA_DEBUG
			printf("lsra_init_blocks EXCEPTIONTABLE without handler!\n");
#endif
		} else {
			if ((ex->handler->debug_nr < 0) || (ex->handler->debug_nr >= m->basicblockcount)) {
#ifdef LSRA_DEBUG
				printf("lsra_init_blocks EXCEPTIONTABLE Handler Blocknummer invalid! %i\n", ex->handler->debug_nr);
#endif
			} else {
				exh_min[i]=ex->handler->debug_nr;
				exh_max[i]=lsra_getmaxblock(m, ld, ex->handler->debug_nr);

				ex_min[i]=ex->start->debug_nr;
				ex_max[i]=ex->end->debug_nr;
#ifdef LSRA_DEBUG
				printf("EX %3i exmin %3i exmax %3i exhmin %3i exhmax %3i\n",i,ex_min[i],ex_max[i],exh_min[i],exh_max[i]);
#endif
 			} 
		}
	}

	/* extend lifetime within loops */
  
	for (num_loops=0,lc =ld->c_allLoops;lc!=NULL;num_loops++,lc=lc->next);

	/* set up loops[i].b_first .b_last to hold the first and last node of all loops */
	loops=DMNEW(struct l_loop,num_loops);
	for (i=0,lc=ld->c_allLoops;i<num_loops;i++,lc=lc->next) {

		le = lc->nodes;
		bfirst=m->basicblockcount;
		blast=0;
		while (le != NULL) {
			if (le->node<bfirst) bfirst=le->node;
			if (le->node>blast) blast=le->node;
			le = le->next;
		}
		loops[i].b_first=bfirst;
		loops[i].b_last=blast;
		loops[i].nesting=0;
	}

	/* sort loops by b_first desc*/
	for (i=0; i<num_loops-1;i++) {
		for (j=i+1; j<num_loops;j++) {
			if (loops[i].b_first < loops[j].b_first) {
				bfirst=loops[j].b_first;
				blast=loops[j].b_last;
				loops[j].b_first=loops[i].b_first;
				loops[j].b_last=loops[i].b_last;
				loops[i].b_first=bfirst;
				loops[i].b_last=blast;
			}
		}
	}

	/* check for nesting_level, overlapping */
	for (i=0; i<num_loops-1;i++) {
		/*! loops[i].b_first >= loops[i+1].b_first !*/
		if (loops[i+1].b_last>=loops[i].b_first) {
			if (loops[i+1].b_last<loops[i].b_last) {
				/* overlapping -> make one loop of both */
				loops[i+1].b_last=loops[i].b_last;
				loops[i].b_first=-1;
				loops[i].b_last=-1;
			} else {
				loops[i].nesting++; /* only boolean if nested... */
			}
		}
	}

	/* cumulate basicblocks[i].icount in block_loop[i].instr*/
	block_loop=DMNEW(struct b_loop, m->basicblockcount);
	for (i=0;i<m->basicblockcount; i++) {
		block_loop[i].loop=-1;
		if (i!=0) 
			block_loop[i].instr=block_loop[i-1].instr+m->basicblocks[i-1].icount;
		else
			block_loop[i].instr=0;
	}

	/* set block_loop[j].loop to loop index, if Basic Block is in this loop */
	for (i=0; i<num_loops;i++) {
		if (loops[i].b_first!=-1) {
			/* valid loop */
			for (j=loops[i].b_first;j<=loops[i].b_last;j++) block_loop[j].loop=i;
		}
	}

	/* now iterate through lifetimes and expand them */
	lt=ls->lifetimes;
	while(lt!=NULL) {

		if (ls->back_edge_panic) {
			lt->i_start=0;
			lt->i_end=block_loop[m->basicblockcount-1].instr+m->basicblocks[m->basicblockcount-1].icount;
		} else {
			usage = 1;
			il=lt->i_list;
			blast=il->b_index;
			ilast=il->instr;
			while (il->next!=NULL) {
				if ((il->b_index != il->next->b_index) || ((il->b_index == il->next->b_index) && (il->instr != il->next->instr))) {
					if (block_loop[il->b_index].loop == -1)
						usage++; /* not in a loop */
				}
				il=il->next;
			}
			bfirst=il->b_index;
			ifirst=il->instr;
			/* expand lifetimes in a exceptionhandler to at least the whole handler */
			/* TODO do a loop analyze for the exceptionhandler, too*/
			for (i=0; i < cd->exceptiontablelength; i++) {
			
				if ( !((bfirst > exh_max[i]) || ( blast < exh_min[i])) ) {
					/* Überschneidung mit exh_???[i]-> lt liegt in Exceptionhandler */
					/* -> Start auf mindestens die erste Instruktion des geschützten Bereiches legen */
					if (bfirst >= exh_min[i]) {
						bfirst=exh_min[i];
						ifirst=0;
					}
					/* -> Ende auf mindestens die letzte Instruktion des geschützten Bereiches legen */
					if (blast <= exh_max[i]) {
						blast=exh_max[i];
						ilast= m->basicblocks[exh_max[i]].icount-1;
					}
				} 
			}

			ilast+=block_loop[blast].instr;   /* add icount of previous Basic Blocks */
			ifirst+=block_loop[bfirst].instr; /* add icount of previous Basic Blocks */

			if ((j=block_loop[bfirst].loop)==-1)
				j=block_loop[blast].loop;

			if (j!=-1) {
				if (loops[j].b_first<=bfirst) {
					bfirst=loops[j].b_first;
					ifirst=block_loop[bfirst].instr;
					usage+=loops[j].nesting*100;
				}
				if (blast <= loops[j].b_last) {
					blast=loops[j].b_last;
					ilast=block_loop[blast].instr+m->basicblocks[blast].icount;
					usage+=loops[j].nesting*100;
				}
			}

			lt->i_start=ifirst;
			lt->i_end=ilast;
			i=ilast-ifirst;
			if (i==0) i=1;
			lt->usagecount=usage;
		}
		lt=lt->next;
	}

#ifdef LSRA_DEBUG
	for (i=0; i<num_loops;i++) {
		printf("LoopNR: %3i Start: %3i End: %3i Nesting: %3i Block_loop[%3i]:",i,loops[i].b_first,loops[i].b_last,loops[i].nesting,i);
		for (j=0;j<m->basicblockcount;j++)
			if (block_loop[j].loop==i) printf(" %3i",j);
		printf("\n");
	}
#endif   
}

/* void lsra_sort_lt(struct lifetime **lifet) */
/* { */
/* 	struct lifetime *lt, lt_new, *temp, *tmp; */

/*   	lt_new.next=NULL; */

/* 	for (lt=*lifet; lt!= NULL;) { */
/* 		temp=lt->next; */

/* 		for(tmp=&lt_new; (tmp->next!= NULL) && (tmp->next->i_start < lt->i_start); tmp=tmp->next); */
/* 		lt->next=tmp->next; */
/* 		tmp->next=lt; */
/* 		lt=temp; */
/* 	} */
/* 	*lifet=lt_new.next; */
/* } */


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
void print_lifetimes(registerdata *rd, struct lifetime *lt)
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
		printf("i_Start: %3i i_stop: %3i reg: %3i VI: %3i Usage: %3i type: %3i flags: %3i varkind: %3i ILst: ",n->i_start,n->i_end,regoff,n->v_index,n->usagecount,type,flags, varkind);
		for (ni=n->i_list; ni!=NULL; ni=ni->next) {
			if (ni==ni->next) panic("loop in instruction list!\n");
			printf( "(%3i,%3i) ",ni->b_index,ni->instr);
		}
		printf("\n");
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

	/* merge i_lists in order */
	iptr=lt->i_list;
	iptr2=lt->i_list=NULL;
	iptr1=lt1->i_list;
	while  ((iptr != NULL) && (iptr1 != NULL)) {
		if (iptr1->instr == -1) { 
			/* throw away, just for joining */
			iptr1=iptr1->next;
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

#ifdef LSRA_DEBUG
void dump_join( struct lifetime *lt_passing)
{
	struct lifetime *lt;
	struct stackslot *ss;
	int i;

	for (lt=lt_passing,i=0; lt != NULL; lt=lt->next, ++i) {
		printf("Lifetime(%2i)\n  PT: ",i);
		for ( ss=lt->passthrough; ss!=NULL; ss=ss->next)
			printf("(%2i, %2i, %6p) ",ss->bb, ss->s->varnum, (void *)ss->s);
		printf("\n  SS: ");
		for (ss = lt->local_ss; ss!=NULL; ss=ss->next)
			printf("(%2i, %2i, %6p) ", ss->bb, ss->s->varnum, (void *)ss->s);
		printf("\n");
	}
}
#endif

void lsra_join_lifetimes(methodinfo *m, codegendata *cd, lsradata *ls, loopdata *ld)
{
	int i, j, index, stacks_top;
	int in1, out1, in2, out2, temp;
	struct depthElement *de;
	
	int *in_stacks, *out_stacks;
	stackptr out, s;
	struct stackslot **stacks, *ss, *ss1, *ss_new;
	struct lifetime *lt, *lt_prev, *lt_new, *lt_passing;
	struct stackslot *p, *q, *r;
	bool drop, pt;

#ifdef LSRA_DEBUG
	stackptr in;

	in=m->basicblocks[0].instack; /* ?falls Instack des Startblocks != leer ?*/
	if (in != NULL) printf("------------ Instack von BB0 nicht leer! -------\n");
#endif

	/* join copies of dup/swap? or let just codegen copy the contents?  */
	/* -> ?TODO? save at ICMD_DUP* and ICMD_SWAP Copy Info              */

	/* out_stacks hold an index to stacks, which holds the joined stacks */
	/* in_stacks hold an index to out_stacks, with which they are joined */
	/* an initial index of -1 mark, that they where not visited yet      */
	in_stacks =   DMNEW(int, m->basicblockcount);
	out_stacks = DMNEW(int, m->basicblockcount);
	stacks = DMNEW(struct stackslot *, m->basicblockcount);
/* 	for (i=0; i< m->basicblockcount; i++) stacks[i]=NULL; */
 	for (i=0; i < m->basicblockcount; i++ ) in_stacks[i]=out_stacks[i]=-1;
	stacks_top=0;

	for (i=0; i < m->basicblockcount; i++)
		if (m->basicblocks[i].flags >= BBREACHED) {
			if ((out=m->basicblocks[i].outstack) != NULL) {
				ss=lsra_make_ss(out, i);
				ss->next=NULL;
				stacks[stacks_top]=ss;
				out_stacks[i]=stacks_top++;
				for (de=ld->c_dTable[i]; de != NULL; de=de->next) {
					if (in_stacks[de->value] == -1) { /* not visited==joined yet */
						in_stacks[de->value] = i;
						ss=lsra_make_ss(m->basicblocks[de->value].instack, de->value);
						ss->next=stacks[out_stacks[i]];
						stacks[out_stacks[i]]=ss;
					} else { /* in stacks already joined -> join with others */
						/* join this outstack to index in_stack[de->value] points to */
						for (ss=stacks[out_stacks[i]]; ss->next != NULL; ss=ss->next); /* get last element */
						ss->next=stacks[out_stacks[in_stacks[de->value]]];
						stacks[out_stacks[in_stacks[de->value]]]=stacks[out_stacks[i]];
						stacks[out_stacks[i]]=NULL;
						/* update all prior out_stacks indexes to this new join */
						for (j=0; j <= i; j++) {
							if (out_stacks[j] == out_stacks[i]) {
								out_stacks[j]=out_stacks[in_stacks[de->value]];
							}
						}
					}
				}
			}
		}

	/* leere einträge aus stacks entfernen */
	for (i=index=0; i< stacks_top;) {
		if (stacks[index]!=NULL) { /* nächsten freien Platz suchen */
			++index;
			++i;
		} else {
			if (stacks[i]==NULL) { /* nächsten bestetzten Platz zum verschieben suchen*/
				++i;
			} else { /* von i nach index umhängen */
				stacks[index]=stacks[i];
				stacks[i]=NULL;
				++index;
			}
		}
	}

	lt_passing=NULL;
	/* 0 <= i < index | join all corresponding stackslots of in- and outstacks of stacks[i] to lt_new */
	/* and put lt_new in lt_passing (if passthrough ss exist in them) or ls->lifetimes                */
	for (i=0; i < index; i++) {
		while (stacks[i]->s != NULL) {
			lt_new=NULL;
			for (ss=stacks[i]; ss!=NULL; ss=ss->next) {
				/* search stackslot ss->s lifetimelist of basicblock(ss->bb) (ss_lifetimes) */
				/* remember the link before found lifetime, to remove it afterwards         */
				for (lt_prev=lt=ls->ss_lifetimes[ss->bb]; lt!=NULL; lt_prev=lt, lt=lt->next) {
					for (ss1=lt->local_ss; (ss1!=NULL) && (ss1->s != ss->s); ss1 = ss1->next);
					if (ss1 != NULL) break; /* found */
				}
#ifdef LSRA_DEBUG
				if (lt==NULL) panic("lsra_join_lifetimes error in lifetimelist\n");
#endif
				/* Remove found lifetime from Block Stackslot Lifetimelist */
				if (lt==ls->ss_lifetimes[ss->bb]) {
					ls->ss_lifetimes[ss->bb]=lt->next;
				} else {
					lt_prev->next=lt->next;
				}
				lt->next=NULL;
					
				drop = false;
				if (lt->i_list->instr==-1) {
					if (out_stacks[in_stacks[ss->bb]] == out_stacks[ss->bb]) {
						/* Throughpassing Basicblock is already in one "join Group" */
						/* If this stackslot ((lt->local_ss)ss1->s == ss->s) is already in lt_new->local_ss */
						/* do not add it a second time! */
						if (lt_new != NULL) {
							for (ss_new=lt_new->local_ss; (ss_new != NULL) && (ss_new->s != ss1->s); ss_new = ss_new->next);
							if (ss_new != NULL) drop = true;
						}
					}
				}
				if (!drop) {
					/* lt->s->varkind=TEMPVAR */; /* no LOCALVAR, ARGVAR or STACKVAR with joined lifetimes */
					lt->v_index=-1; /* not a local var */

					/* join Lifetime lt to lt_new */
					if (lt_new == NULL) {
						lt_new = lt;
						lt_new->passthrough = NULL;
					} else {
						lsra_merge_i_lists( lt_new, lt);
						lsra_merge_local_ss( lt_new, lt);
					}


					pt=(lt->i_list->instr==-1); /* remember this now, because merge_i_list could once destroy this link*/

					/* add stackslot to local_ss of lt_new */
					ss_new = DNEW(struct stackslot);
					
					lt_new->savedvar |= (lt->savedvar & SAVEDVAR);

					if (pt) { /*lt->i_list->instr==-1) {*/
					/* BB passes this stackslot through -> join later with other side!*/
						if (out_stacks[in_stacks[ss->bb]] != out_stacks[ss->bb]) { 
							/* Stackslot ist not passed through to same (this) "join group" */

							p = DNEW(struct stackslot);
							p->bb = ss->bb; /* Basic Block index of joined Lifetime */
							p->s = ss->s;
							/* sort p in lt_new->passthrough list by increasing stackslot adress               */
							/* there are no two stackslots allowed, which "join" the same "join groups"    */
							/* in case one has to be dropped, keep the one with the "greater" address      */
							r = NULL;
							drop = false;
							in2=out_stacks[in_stacks[p->bb]];
							out2=out_stacks[p->bb];
							if (in2 > out2) { temp=in2; in2=out2; out2=temp; }
							for (q=lt_new->passthrough; (q != NULL);) {
								in1=out_stacks[in_stacks[q->bb]];
								out1=out_stacks[q->bb];
								if (in1 > out1) { temp=in1; in1=out1; out1=temp; }

								if ((in1 == in2) && (out1 == out2)) { /* p->s would join to same group -> drop the one with the lower address */
									if (p->s < q->s)  /* drop p->s, it has the lower address */
										drop = true;
									else { /* drop q from the list, since it has the lower address */
										if (q == lt_new->passthrough ) {
											lt_new->passthrough=q->next;
											q=q->next;
										} else { /* r points to the previous element */
											r->next=q->next;
										}
									}
								}
								if (q != NULL) {
									if (q->s < p->s)
										r=q; /* remember position for sorting p in */
									q=q->next;
								}
							}
							if (!drop) {
								if (r == NULL) {
									/* List Empty or all elements greater than p->s */
									p->next=lt_new->passthrough;
									lt_new->passthrough=p;
								} else {
									p->next=r->next;
									r->next=p;
								}
							}
						}
					}
				}
				ss->s=ss->s->prev; /* Next Stackslot for next Iteration */
			} /*for */
			if (lt_new->passthrough != NULL) {
				lt_new->next=lt_passing;
				lt_passing=lt_new;
			} else {
				lt_new->next=ls->lifetimes;
				ls->lifetimes=lt_new;
			}
		}/* while */
	} /* for */

	/* join lifetimes with same stackslots in ->passthrough in lt_passing */
	for (lt=lt_passing; lt != NULL; lt=lt->next) {
		while (lt->passthrough != NULL) {
#ifdef LSRA_DEBUG
			printf("before \n");
			dump_join(lt_passing);
#endif
			s=lt->passthrough->s;
			lt->passthrough=lt->passthrough->next; /* drop this stackslot from lt_passthrough list */
			/* search for next lifetime, which has the same stackslot in passthrough  */
			/* lt_new->next will point to it                                          */
			/* there has to be another lifetime after lt with same s in passthrough   */
			for (lt_new=lt; (lt_new->next != NULL); lt_new=lt_new->next) {
				/* passthrough is sorted by increasing address of s */
				/* remember in q the list element before p with p->s == s */
				for (p=lt_new->next->passthrough; (p!=NULL) && (p->s < s); q=p,p=p->next);
				if ((p != NULL) && (p->s == s)) { 
					/* found -> now drop this stackslot from lt_new's passtrough list */
					if (p == lt_new->next->passthrough) { /* first element in list */
						lt_new->next->passthrough = lt_new->next->passthrough->next;
					} else
						q->next=p->next;
					break;
				}
			}
#ifdef LSRA_DEBUG
			if (lt_new->next==NULL)
				panic("lsra_join_lifetimes error in lt_passing lifetimelist\n");
#endif
			/* join lt and lt_new->next to lt */
			lt->savedvar |= (lt_new->next->savedvar & SAVEDVAR);

			/* join local_ss lists */
#ifdef LSRA_DEBUG
			if (lt_new->next->local_ss == NULL)
				panic("lsra_join_lifetimes Lifetime without stackslot\n");
#endif
/* 			for (ss=lt_new->next->local_ss; (ss->next != NULL); ss=ss->next); */
/* 			ss->next=lt->local_ss; */
/* 			lt->local_ss=lt_new->next->local_ss; */
			lsra_merge_local_ss(lt, lt_new->next);

			/* merge i_lists in order */
			lsra_merge_i_lists(lt, lt_new->next);

			/* join passthrough lists in ascending order  */
			/* if there are duplicates, it happened that a join was done through the   */
			/* other joins till now, so just drop both of them                         */
			p=lt->passthrough;
			q=lt_new->next->passthrough;
			lt->passthrough=NULL;
			ss=NULL; /* pointer to the end of lt->passthrough */
			while ( (p!=NULL) && (q!=NULL) ) {
				if (p->s > q->s) {
					if (ss==NULL) {
						lt->passthrough=q;
					} else {
						ss->next=q;
					}
					ss=q;
					q=q->next;
				} else {
					if (q->s == p->s) {
						/* Drop both stackslots -> they where just joined through some other joins */
						q=q->next;
						p=p->next;
					} else {
						/* p->s < q->s */
						if (ss==NULL) {
							lt->passthrough=p;
						} else {
							ss->next=p;
						}
						ss=p;
						p=p->next;
					}
				}
			}
			if (q == NULL) {
				if (lt->passthrough == NULL)
					lt->passthrough=p;
				else
					ss->next = p;
			}
			if (p == NULL) {
				if (lt->passthrough == NULL)
					lt->passthrough=q;
				else
					ss->next = q;
			}
			lt_new->next=lt_new->next->next; /* throw away joined lifetime lt_new->next */
#ifdef LSRA_DEBUG
			printf("after\n");
			dump_join(lt_passing);
#endif
		} /* while */
	} /* for */

	if (lt_passing!=NULL) {
		for (lt=lt_passing; (lt->next!=NULL); lt=lt->next);
		lt->next=ls->lifetimes;
		ls->lifetimes=lt_passing;
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
	if ((lt->local_ss==NULL) || (lt->local_ss->s != s)) {
		ss = DNEW(struct stackslot);
		ss->s = s;
		ss->next = lt->local_ss;
		lt->local_ss = ss;
		if (s != NULL) lt->savedvar |= s->flags & SAVEDVAR;
	}
}

#define lsra_new_stack(ls, s, block, instr) _lsra_new_stack(ls, s, block, instr, LSRA_STORE)
void _lsra_new_stack(lsradata *ls, stackptr s, int block, int instr, int store)
{
	struct lifetime *n;

	n=DNEW(struct lifetime);

	n->savedvar = (s->flags & SAVEDVAR);
	n->local_ss=NULL;
	lsra_add_ss( n, s);
	n->usagecount=1;
	if (s->varkind == LOCALVAR)
		n->v_index=s->varnum;
	else
		n->v_index=-1;
	n->i_list=NULL;

	n->next=ls->ss_lifetimes[block];
	ls->ss_lifetimes[block]=n;

	n->i_list=lsra_add_i_list(n->i_list, instr, block, store);
}

#define lsra_from_stack(ls, s, block, instr) _lsra_from_stack(ls, s, block, instr, LSRA_LOAD)
#define lsra_pop_from_stack(ls, s, block, instr) _lsra_from_stack(ls, s, block, instr, LSRA_POP)
void _lsra_from_stack(lsradata *ls, stackptr s, int block, int instr, int store)
{
	struct lifetime *n;
	/* ss_lifetimes[block]->local_ss have exactly one entry! */
	for (n=ls->ss_lifetimes[block]; (n!=NULL) && (n->local_ss->s!=s);n=n->next);
	if (n==NULL) {
		_lsra_new_stack(ls, s, block, instr, store);
		/* ist mit dem neuen joinen hinterher immer so, wenn ein Stackslot in einen BB "reinkommt" */
/* 		printf("type %i flags %i  varkind %i varnum %i regoff %i \n",s->type,s->flags ,s->varkind ,s->varnum ,s->regoff); */
/* 		panic("lsra_from_stack: Var on Stack not found"); */
	} else {
		n->i_list=lsra_add_i_list(n->i_list, instr, block, store);
	}
}

void lsra_usage_local(lsradata *ls, s4 v_index, int type, int block, int instr, int store)
{
	struct lifetime *n;

	/* Lifetime vom richtigen Type suchen */
	for (n=ls->locals_lifetimes[v_index]; (n!=NULL) && (n->type!=type);n=n->next);

	if (n==NULL) {
#ifdef LSRA_DEBUG
		if (store != LSRA_STORE) printf("lsra_local_store: Read before write Local var: %i paramcount: ?\n", v_index);
#endif
		lsra_new_local(ls, v_index, type);
		/* neue Lifetimes werden immer am Anfang der Liste eingehängt */
		n=ls->locals_lifetimes[v_index];
	}
	/* add access at (block, instr) to intruction list */
	n->i_list=lsra_add_i_list(n->i_list, instr, block, store);
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

void dup_mark( struct dup *dup,stackptr s)
{
	struct stackslot *n;
	n = DNEW(struct stackslot);
	if (dup->ss == NULL) {
		dup->ss = n;
		n->next = NULL;
	} else {
		n->next=dup->ss;
		dup->ss=n;
	}
	n->s=s;
}

void dup_next( struct dup *dup)
{
	struct dup *n;

	n = DNEW(struct dup);
	n->next = dup->next;
	dup->next=n;
	n->ss = dup->ss;
	dup->ss = NULL;
}

void dup_join( struct lsradata *ls, struct dup *dup, int block)
{
	struct dup *d;
	struct stackslot *ss, *ss1;
	bool pt; /* joins with passthrough lifetimes not yet possible! */
	struct lifetime *join_lt[3]; /* max three lifetimes to join */
	struct lifetime *join_lt_prev[3]; /* previous elements for unlinking */
	struct lifetime *lt, *lt_prev;
	int i, join_lt_top, join_to;

	for (i=0; i<3; i++) join_lt[i]=join_lt_prev[i]=NULL;
	/* walk through dup structure and clean it up for next block */
	/* the first dup entry is alway empty for the next group to come*/
	for (d=dup->next; d!= NULL; d = dup->next=dup->next->next) { 
		pt=false;
		join_lt_top=0;
		for (ss=d->ss; (ss != NULL) && (!pt); ss = ss->next) {
			for (lt = lt_prev = ls->ss_lifetimes[block]; lt != NULL ; lt_prev=lt, lt=lt->next) {
				for (ss1=lt->local_ss; (ss1!=NULL) && (ss1->s != ss->s); ss1 = ss1->next);
				if (ss1 != NULL) break; /* found */
			}
			if (lt == NULL) panic("dup_join Lifetimes not found\n");
			pt=(lt->i_list->instr == -1); /* joins with passthrough lifetimes not yet possible! */
			pt|=(lt->i_list->next == NULL); /* joins with "interface" Stackslots not yet possible! */
			if (!pt) {
				join_lt_prev[join_lt_top]=lt_prev;
				join_lt[join_lt_top++]=lt;
			}
		}
		if (!pt) { /* now join */
			/* look if one element is the root of the list (joint_lt == join_lt_prev == ls->ss_lifetimes[block]) */
			for (join_to=0; (join_to < join_lt_top) && (join_lt[join_to] != join_lt_prev[join_to]); join_to++);
			if (join_to == join_lt_top) /* no root element in join array */
				join_to=0;
			join_lt[join_to]->v_index = -1;
			for (i=0; i<join_lt_top; i++) {
				if (i != join_to) {
					/* now join finally */
					lsra_merge_i_lists(join_lt[join_to], join_lt[i]);
					lsra_merge_local_ss(join_lt[join_to], join_lt[i]);
					join_lt[join_to]->savedvar|=(join_lt[i]->savedvar & SAVEDVAR);
					/* drop join_lt[i] from list */
					join_lt_prev[i]->next = join_lt[i]->next;
				}
			}
		}
	}
	dup->next = NULL;
}

void lsra_scan_registers_canditates(methodinfo *m, lsradata *ls)
{
	int i;
	int opcode;
	int iindex;
	int len;
	stackptr    src;
	stackptr    dst;
	instruction *iptr;
	stackptr in,out;
	int      id, od;
	int b_index;
	struct dup dup;
	
	dup.ss=NULL;
	dup.next=NULL;
	
	b_index=0;
	while (b_index < m->basicblockcount ) {

		if (m->basicblocks[b_index].flags >= BBREACHED) {
			dst = m->basicblocks[b_index].instack;
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
					lsra_new_stack(ls, in, b_index, -1);
					lsra_new_stack(ls, in, b_index, -1);
				}
			}
			iptr = m->basicblocks[b_index].iinstr;
			len = m->basicblocks[b_index].icount;
			iindex=0;

			while (iindex<len)  {
				src = dst;
				dst = iptr->dst;
				opcode = iptr->opc;
#ifdef LSRA_DEBUG
				printf("bb: %3i bcount: %3i iindex: %3i ilen: %3i opcode: %3i %s\n",b_index,m->basicblockcount,iindex,len,opcode,icmd_names[opcode]);
				lsra_dump_stack(src);
				lsra_dump_stack(dst);
#endif
				switch (opcode) {

				case ICMD_RET:
					lsra_usage_local(ls,iptr->op1,TYPE_ADR, b_index,iindex,LSRA_LOAD); /* local read (return adress) */
					
					/* pop 0 push 0 */

				case ICMD_NOP:
				case ICMD_ELSE_ICONST:
				case ICMD_CHECKEXCEPTION:
				case ICMD_CHECKASIZE:
				case ICMD_IINC:
				case ICMD_JSR:
				case ICMD_RETURN:
				case ICMD_GOTO:
				case ICMD_INLINE_START:
				case ICMD_INLINE_END:
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
					lsra_usage_local(ls,iptr->op1,opcode-ICMD_ILOAD, b_index,iindex,LSRA_LOAD); /* local->value */
					lsra_new_stack(ls,dst,b_index,iindex); /* value->stack */
					/* ?Reference to local var?-> attention if local var is changed */
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
					lsra_from_stack(ls, src,b_index,iindex); /* stack -> value */
					lsra_usage_local(ls,iptr->op1,opcode-ICMD_ISTORE, b_index,iindex,LSRA_STORE); /* local->value */
 					break;

					/* pop 1 push 0 */

				case ICMD_IRETURN:
				case ICMD_LRETURN:
				case ICMD_FRETURN:
				case ICMD_DRETURN:
				case ICMD_ARETURN: /*stack(value) -> [empty] */

				case ICMD_ATHROW: /* stack(objref) -> undefined */

				case ICMD_PUTSTATIC: /* stack(value) -> static_field */
					/* pop 1 push 0 branch */

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

					/* pop 1 push 0 table branch */

				case ICMD_TABLESWITCH:
				case ICMD_LOOKUPSWITCH:

				case ICMD_NULLCHECKPOP: /****** ????? -1 -> stack *********/
				case ICMD_MONITORENTER:
				case ICMD_MONITOREXIT:
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
					dup_mark(&dup, src);
					dup_mark(&dup, dst);
					dup_next(&dup);
					break;

					/* pop 0 push 2 dup */
					
				case ICMD_DUP2:
					lsra_new_stack(ls,dst->prev,b_index,iindex);
					lsra_new_stack(ls,dst,b_index,iindex); 
					lsra_from_stack(ls, src,b_index,iindex); /* or inc usage_count! */
					lsra_from_stack(ls, src->prev,b_index,iindex); /* inc usage_count! */

					dup_mark(&dup, src);
					dup_mark(&dup, dst);
					dup_mark(&dup, dst->prev->prev);
					dup_next(&dup);
					dup_mark(&dup, src->prev);
					dup_mark(&dup, dst->prev);
					dup_mark(&dup, dst->prev->prev->prev);
					dup_next(&dup);
					break;

					/* pop 2 push 3 dup */
					
				case ICMD_DUP_X1:
					lsra_from_stack(ls, src,b_index,iindex); /* from for to, or it will not work! inc usage_count! */
					lsra_from_stack(ls, src->prev,b_index,iindex); /*from for to, or it will not work!  inc usage_count! */
					lsra_new_stack(ls,dst->prev->prev,b_index,iindex);
					lsra_new_stack(ls,dst->prev,b_index,iindex);
					lsra_new_stack(ls,dst,b_index,iindex); 
					dup_mark(&dup, src);
					dup_mark(&dup, dst);
					dup_mark(&dup, dst->prev->prev);
					dup_next(&dup);
					dup_mark(&dup, src->prev);
					dup_mark(&dup, dst->prev);
					dup_next(&dup);
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
					dup_mark(&dup, src);
					dup_mark(&dup, dst);
					dup_mark(&dup, dst->prev->prev->prev);
					dup_next(&dup);
					dup_mark(&dup, src->prev);
					dup_mark(&dup, dst->prev);
					dup_next(&dup);
					dup_mark(&dup, src->prev->prev);
					dup_mark(&dup, dst->prev->prev);
					dup_next(&dup);
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
					dup_mark(&dup, src);
					dup_mark(&dup, dst);
					dup_mark(&dup, dst->prev->prev->prev);
					dup_next(&dup);
					dup_mark(&dup, src->prev);
					dup_mark(&dup, dst->prev);
					dup_mark(&dup, dst->prev->prev->prev->prev);
					dup_next(&dup);
					dup_mark(&dup, src->prev->prev);
					dup_mark(&dup, dst->prev->prev);
					dup_next(&dup);
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
					dup_mark(&dup, src);
					dup_mark(&dup, dst);
					dup_mark(&dup, dst->prev->prev->prev->prev);
					dup_next(&dup);
					dup_mark(&dup, src->prev);
					dup_mark(&dup, dst->prev);
					dup_mark(&dup, dst->prev->prev->prev->prev->prev);
					dup_next(&dup);
					dup_mark(&dup, src->prev->prev);
					dup_mark(&dup, dst->prev->prev);
					dup_next(&dup);
					dup_mark(&dup, src->prev->prev->prev);
					dup_mark(&dup, dst->prev->prev->prev);
					dup_next(&dup);
					break;

					/* pop 2 push 2 swap */
					
				case ICMD_SWAP:
					lsra_from_stack(ls, src,b_index,iindex); /* from for to, or it will not work! inc usage_count! */
					lsra_from_stack(ls, src->prev,b_index,iindex); /*from for to, or it will not work!  inc usage_count! */
					lsra_new_stack(ls,dst->prev,b_index,iindex);
					lsra_new_stack(ls,dst,b_index,iindex);
					dup_mark(&dup, src);
					dup_mark(&dup, dst->prev);
					dup_next(&dup);
					dup_mark(&dup, src->prev);
					dup_mark(&dup, dst);
					dup_next(&dup);
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
					{
						i = iptr->op1;
						while (--i >= 0) {
							lsra_from_stack(ls, src,b_index,iindex);
							src = src->prev;
						}
						if (((methodinfo*)iptr->val.a)->returntype != TYPE_VOID) {
							lsra_new_stack(ls,dst,b_index,iindex);
						}
						break;
					}

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
				iptr++;
				iindex++;
			} /* while instructions */
		} /* if */
		
		dup_join(ls, &dup, b_index);

		b_index++;
	} /* while blocks */
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
