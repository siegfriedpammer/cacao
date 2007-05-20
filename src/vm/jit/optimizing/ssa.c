/* src/vm/jit/optimizing/ssa.c - static single-assignment form

   Copyright (C) 2005, 2006 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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

   $Id: $

*/
#include <stdio.h>
#include <stdlib.h>

#include "mm/memory.h"

#include "toolbox/bitvector.h"
#include "toolbox/worklist.h"

#include "vm/builtin.h"

#include "vm/jit/jit.h" /* icmd_table */

#include "vm/jit/optimizing/dominators.h"
#include "vm/jit/optimizing/graph.h"
#include "vm/jit/optimizing/lifetimes.h"
#include "vm/jit/optimizing/lsra.h"

#include "vm/jit/optimizing/ssa.h"

#if defined(SSA_DEBUG_VERBOSE)
#include "vmcore/options.h"   /* compileverbose */
#endif

/* function prototypes */
void ssa_set_local_def(lsradata *, int , int);
void ssa_set_interface(jitdata *, basicblock *, s4 *);

void dead_code_elimination(jitdata *jd, graphdata *gd);
void copy_propagation(jitdata *jd, graphdata *gd);
void ssa_replace_use_sites(jitdata *, graphdata *, struct lifetime *,
						int , worklist *);
void ssa_place_phi_functions(jitdata *jd, graphdata *gd, dominatordata *dd);
void ssa_rename_init(jitdata *jd, graphdata *gd);
void ssa_rename(jitdata *jd, graphdata *gd, dominatordata *dd);
void ssa_rename_(jitdata *jd,  graphdata *gd, dominatordata *dd, int n);

void ssa_set_def(lsradata *, int , int );
void ssa_set_local_def(lsradata *, int , int );
void ssa_set_iovar(lsradata *, s4 , int , s4 *);
void ssa_set_interface(jitdata *, basicblock *, s4 *);

void ssa_generate_phi_moves(jitdata *, graphdata *);
int ssa_rename_def_(lsradata *ls, int a);

#ifdef SSA_DEBUG_VERBOSE
void ssa_print_trees(jitdata *jd, graphdata *gd, dominatordata *dd);
void ssa_print_lt(lsradata *ls);
void ssa_show_variable(jitdata *jd, int index, varinfo *v, int stage);
void ssa_print_phi(lsradata *, graphdata *);
#endif

/* ssa ************************************************************************

SSA main procedure:

******************************************************************************/

void ssa(jitdata *jd, graphdata *gd) {
	struct dominatordata *dd;
	lsradata *ls;

	ls = jd->ls;

	dd = compute_Dominators(gd, ls->basicblockcount);
	computeDF(gd, dd, ls->basicblockcount, 0);

	ssa_place_phi_functions(jd, gd, dd);
	ssa_rename(jd, gd, dd);
#ifdef SSA_DEBUG_VERBOSE
	if (compileverbose) {
		printf("Phi before Cleanup\n");
		ssa_print_phi(ls, gd);
		ssa_print_lt(ls);
	}
#endif
	lt_scanlifetimes(jd, gd, dd);
#ifdef SSA_DEBUG_VERBOSE
	if (compileverbose) {
		ssa_print_lt(ls);
	}
#endif
/* 	dead_code_elimination(jd, gd); */
#ifdef SSA_DEBUG_VERBOSE
	if (compileverbose) {
		printf("Phi after dead code elemination\n");
		ssa_print_phi(ls, gd);
		ssa_print_lt(ls);
	}
#endif
/* 	copy_propagation(jd, gd); */
#ifdef SSA_DEBUG_VERBOSE
	if (compileverbose) {
		printf("Phi after copy propagation\n");
		ssa_print_phi(ls, gd);
		ssa_print_lt(ls);
	}
#endif

	ssa_generate_phi_moves(jd, gd);
	transform_BB(jd, gd);


#ifdef SSA_DEBUG_CHECK
	{
		int i, j, pred, in_d, out_d;
		graphiterator iter_pred;
		s4 *in, *out;
		bool phi_define;
		methodinfo *m;

		m = jd->m;

		for(i = 0; i < ls->basicblockcount; i++) {
			if (ls->basicblocks[i]->indepth != 0) {
				pred = graph_get_first_predecessor(gd, i, &iter_pred);
				for (; (pred != -1); pred = graph_get_next(&iter_pred)) {
					in_d = ls->basicblocks[i]->indepth - 1;
					in = ls->basicblocks[i]->invars;
					for (; in_d >= 0; in_d--) {
						phi_define = false;
						for (j = 0; (!phi_define) && (j<ls->ssavarcount); j++) {
							if (ls->phi[i][j] != NULL)
								if (ls->phi[i][j][0] == in[in_d])
									phi_define = true;
						}
						if (!phi_define) {
							/* in not defined in phi function -> check with */
							/* outstack(s)  of predecessor(s) */
							out_d = ls->basicblocks[pred]->outdepth - 1;
							out = ls->basicblocks[pred]->outvars;
							_SSA_ASSERT(out_d >= in_d);
							for(; out_d > in_d; out_d--);
							if ((in[in_d] != out[out_d]) || 
							(VAR(in[in_d])->flags != VAR(out[out_d])->flags)) {
								printf("Method: %s %s\n",
									   m->class->name->text, m->name->text);
									printf("Error: Stack Varnum Mismatch BBin %3i BBout %3i Stackdepth %3i\n", i, pred, in_d);
								if (compileverbose)
									printf("Error: Stack Varnum Mismatch BBin %3i BBout %3i Stackdepth %3i\n", i, pred, in_d);
/* 								else */
/* 									_SSA_ASSERT(0); */
							}
						}
					}
				}
			}
		}
	}

#endif


#ifdef SSA_DEBUG_VERBOSE
	if (compileverbose)
		ssa_print_trees(jd, gd, dd);
#endif
}

/* ssa_init *******************************************************************

Initialise data structures for ssa

IOVARS of same stackdepth and same type are coalesced:
interface_map[ 5 * stackdepth + type ] = new_varindex with
0 <= new_varindex < ls->ssavarcount

TODO: check if coalescing of IOVARS of same stackdepth and type only of adjacent
basic blocks could decrease the number of phi functions and so improve ssa 
analysis performance!

All LOCALVARS and IOVARS get a new unique varindex:
ls->new_varindex[0..jd->varcount[ = new_varindex with
0 <= new_varindex < ls->ssavarcount

The jd->varcount bits long bitvectors ls->var_def[0..jd->basicblockindex+1[
 are set  to the definitions of LOCALVARS and IOVARS. (So the only the first 
ls->ssavarcount bits of each of these vectors contain valid data, but 
ls->ssavarcount is computed at the same time as the definitons are stored.)

The basic block number used as index for the bitvector array ls->var_def is 
already shifted by one to make space for the new basic block 0 for parameter
initialization.

******************************************************************************/

void ssa_init(jitdata *jd) {
	int         p, t, v;
	methoddesc  *md;
	int         i, l, b_index, len;
	instruction *iptr;
	basicblock  *bptr;
	s4          *interface_map; /* holds an new unique index for every used  */
	                            /* basic block inoutvar described by a dupel */
                                /*(depth,type). Used to coalesce the inoutvars*/
	methodinfo   *m;
	lsradata     *ls;
	builtintable_entry *bte;

	ls = jd->ls;
	m = jd->m;
	md = m->parseddesc;


#if defined(SSA_DEBUG_CHECK) || defined(SSA_DEBUG_VERBOSE)
# if defined(SSA_DEBUG_VERBOSE)
	if (compileverbose) {
		printf("%s %s ",m->class->name->text, m->name->text);
		if (jd->isleafmethod)
			printf("**Leafmethod**");
		printf("\n");
	}
# endif
	if (strcmp(m->class->name->text,"spec/benchmarks/_213_javac/Parser")==0)
		if (strcmp(m->name->text,"parseTerm")==0)
# if defined(SSA_DEBUG_VERBOSE)
			if (compileverbose) 
				printf("12-------------------12\n");
# else
	        { int dummy=1; dummy++; }
# endif
#endif

#ifdef SSA_DEBUG_VERBOSE
    if (compileverbose) 
		printf("ssa_init: basicblockcount %3i localcount %3i\n",
			   jd->basicblockcount, jd->localcount);
#endif

	/* As first step all definitions of local variables and in/out vars are */
	/* gathered. in/outvars are coalesced for same type and depth           */
	/* "normal" tempvars (just living within one basicblock are) ignored    */

	/* ls->var holds the index to jd->vars  */

	ls->num_defs = DMNEW(int, jd->varcount);
	ls->new_varindex = DMNEW(int , jd->varcount);

	for(p = 0; p < jd->varcount; p++) {
		ls->num_defs[p] = 0;
		ls->new_varindex[p] = UNUSED;
	}

	/* init Var Definition bitvectors */

	ls->var_def = DMNEW(int *, jd->basicblockcount + 1);
	for(i = 0; i < jd->basicblockcount + 1; i++) {
		ls->var_def[i] =  bv_new(jd->varcount);
	}

	ls->ssavarcount = 0;

	/* Add parameters first in right order, so the new local indices */
	/* 0..p will correspond to "their" parameters */
	/* They get defined at the artificial Block 0, the real method bbs will be*/
	/* moved to start at block 1 */

	/* don't look at already eliminated (not used) parameters (locals) */

 	for (p = 0, l = 0; p < md->paramcount; p++) {
 		t = md->paramtypes[p].type;
		i = jd->local_map[l * 5 + t];
 		l++;
 		if (IS_2_WORD_TYPE(t))    /* increment local counter a second time  */
 			l++;                  /* for 2 word types */
		if (i == UNUSED)
			continue;
		ssa_set_local_def(ls, -1, i);
	}

	_SSA_ASSERT(ls->ssavarcount < jd->varcount);

	/* coalesce bb in/out vars */

	interface_map = DMNEW(s4, jd->stackcount * 5);
	for(i = 0; i < jd->stackcount * 5; i++)
		interface_map[i] = UNUSED;

	bptr = jd->basicblocks;

	for(; bptr != NULL; bptr = bptr->next) {
#ifdef SSA_DEBUG_VERBOSE
	if (compileverbose)
		printf("ssa_init: BB %3i flags %3x\n",bptr->nr, bptr->flags);
#endif
		if (bptr->flags >= BBREACHED) {

			/* 'valid' Basic Block */

			b_index = bptr->nr;

			len = bptr->icount;
			iptr = bptr->iinstr;
			ssa_set_interface(jd, bptr, interface_map);

			/* !!!!!!!!! not true for now !!!!!!!!! */
			/* All baseline optimizations from stack.c are turned off for */
			/* SSA! */

			for (; len > 0; len--, iptr++) {

				/* Look for definitions (iptr->dst). INVOKE and BUILTIN have */
				/* an optional dst - so they to be checked first */

				v = UNUSED;
				if (icmd_table[iptr->opc].dataflow == DF_INVOKE) {
						INSTRUCTION_GET_METHODDESC(iptr,md);
						if (md->returntype.type != TYPE_VOID)
							v = iptr->dst.varindex;
				}
				else if (icmd_table[iptr->opc].dataflow == DF_BUILTIN) {
						bte = iptr->sx.s23.s3.bte;
						md = bte->md;
						if (md->returntype.type != TYPE_VOID)
							v = iptr->dst.varindex;
				}
				else if (icmd_table[iptr->opc].dataflow >= DF_DST_BASE) {
 					v = iptr->dst.varindex;
				}

				if (v != UNUSED) {
					if (( v < jd->localcount) || ( VAR(v)->flags & INOUT )) {
				  /* !IS_TEMPVAR && !IS_PREALLOC == (IS_LOCALVAR || IS_INOUT) */

/* 						_SSA_ASSERT(ls->new_varindex[v] != UNUSED); */
						ssa_set_local_def(ls, b_index, v);
					}
				}
			}
		}
	}
	_SSA_ASSERT(ls->ssavarcount < jd->varcount);
#ifdef SSA_DEBUG_VERBOSE
	if (compileverbose) {

		printf("ssa_init: Vars: Orig:%3i SSAVar: %3i\n", jd->varcount, 
			   ls->ssavarcount);
		for(i = 0; i < jd->varcount; i++) {
			if ((i < jd->localcount) || ( VAR(i)->flags & INOUT)) {
				printf("%3i(%3i,%3x) ",i,VAR(i)->type, VAR(i)->flags);
				ssa_show_variable(jd, i, VAR(i),0);
				if (i < ls->ssavarcount) {
					printf(" -> %3i", ls->new_varindex[i]);
				}
				printf("\n");
			}
		}
	}
#endif
}

/* ssa_set_def ****************************************************************

Helper for ssa_set_local_def and ssa_set_interface

The definition of a var is stored in the bitvector array ls->var_def.

The number of definitons of each var is counted, so the number of new vars with
SSA is known.

******************************************************************************/

void ssa_set_def(lsradata *ls, int b_index, int varindex) {

	/* b_index + 1 to leave space for the param init block 0 */

	bv_set_bit(ls->var_def[b_index + 1], varindex);

	/* count number of defs for every var since SSA */
	/* will create a new var for every definition */

	ls->num_defs[varindex]++;
}

/* ssa_set_local_def **********************************************************

Helper for ssa_init

Assigns a new unique index for the local var varindex (if not already done so)
and then calls ssa_set_def to remember the definition in the bitvector array
ls->var_def

******************************************************************************/

void ssa_set_local_def(lsradata *ls, int b_index, int varindex) {

	if (ls->new_varindex[varindex] == UNUSED) {
		ls->new_varindex[varindex] = ls->ssavarcount++;
	}

	ssa_set_def(ls, b_index, ls->new_varindex[varindex]);
}

/* ssa_set_local_def **********************************************************

Helper for ssa_set_interface

IN: ls              pointer to lsradata structure
    iovar           varindex of INOUTVAR to process
	map_index       stackdepth * 5 + type, used for coalescing IOVARS.

IN/OUT
	interface_map   used for coalescing IOVARS. interface_map[map_index] == 
	                UNUSED, if this map_index (==stackdepth,type tupel) did not
					occur till now. Then interface_map[map_index] will be set
					to a new unique index.
	ls->new_varindex will be set to this new unique index to map the old
	                 varindizes to the new ones.

Assigns a new unique index for the local var varindex (if not already done so)
and then calls ssa_set_def to remember the definition in the bitvector array
ls->var_def

******************************************************************************/

void ssa_set_iovar(lsradata *ls, s4 iovar, int map_index, s4 *interface_map) {
		if (interface_map[map_index] == UNUSED) 
			interface_map[map_index] = ls->ssavarcount++;

		ls->new_varindex[iovar] = interface_map[map_index];
}


/* ssa_set_interface ***********************************************************

Helper for ssa_init

IN: ls              pointer to lsradata structure
    *bptr           pointer to the basic block to be processed

IN/OUT
	interface_map   used for coalescing IOVARS. interface_map[map_index] == 
	                UNUSED, if this map_index (==stackdepth,type tupel) did not
					occur till now. Then interface_map[map_index] will be set
					to a new unique index. (see ssa_set_iovar)

Searches the basic block given by *bptr for IN and OUTVARS and coalesces them
for each unique stackdepth,type dupel. For each OUTVAR with a different or no 
INVAR at the same stackdepth the definition of this OUTVAR in this basic block 
is remembered in ls->var_def. (see ssa_set_def)

******************************************************************************/

void ssa_set_interface(jitdata *jd, basicblock *bptr, s4 *interface_map) {
	s4 *out, *in;
	int in_d, out_d;
	int o_map_index, i_map_index;
	lsradata *ls;

	ls = jd->ls;

	out = bptr->outvars;
	in = bptr->invars;
	in_d = bptr->indepth - 1;
	out_d = bptr->outdepth - 1;

	/* ignore top Stackelement of instack in case of EXH or SBR blocks */
	/* These are no Interface stackslots! */
	if ((bptr->type == BBTYPE_EXH) ||
		(bptr->type == BBTYPE_SBR)) {
		in_d--;
	}

	/* invars with no corresponding outvars are not defined here */
	/* just set up the interface_map */

	for(;(in_d > out_d); in_d--) {
		i_map_index = in_d * 5 + VAR(in[in_d])->type;
		ssa_set_iovar(ls, in[in_d], i_map_index, interface_map);
	}

	while((out_d >= 0)) {
		/* set up interface_map */

		o_map_index = out_d * 5 + VAR(out[out_d])->type;
		if (in_d >= 0) {
			i_map_index = in_d * 5 + VAR(in[in_d])->type;
			ssa_set_iovar(ls, in[in_d], i_map_index, interface_map);
		}
		ssa_set_iovar(ls, out[out_d], o_map_index, interface_map);
		if (in_d == out_d) {
			if (in[in_d] != out[out_d]) {

				/* out interface stackslot is defined in this basic block */

/* 				ssa_set_def(ls, bptr->nr + 1, ls->new_varindex[out[out_d]]); */
			}
			out_d--;
			in_d--;
		}
		else {

			/* in_d < out_d */
			/* out interface stackslot is defined in this basic block */

/* 			ssa_set_def(ls, bptr->nr + 1, ls->new_varindex[out[out_d]]); */
			out_d--;
		}
	}
}

/* ssa_place_phi_functions *****************************************************

ls->phi[n][a][p] is created and populated.

For each basicblock Y in the dominance frontier of a basicblock n (0 <= n < 
ls->basicblockcount)in which a variable (0 <= a < ls->ssavarcount) is defined an
entry in ls->phi[Y][a] is created.
This entry is an array with the number of predecessors of Y elements + 1 elements.
This elements are all set to the variable a and represent the phi function which
will get ai = phi(ai1, ai2, ..., aip) after ssa_rename.

*******************************************************************************/


void ssa_place_phi_functions(jitdata *jd, graphdata *gd, dominatordata *dd)
{
	int a,i,j,n,Y;
	bitvector *def_sites;
	bitvector *A_phi;    /* [0..ls->basicblockcount[ of ls->ssavarcount Bit */
	worklist *W;
	int num_pred;

	lsradata *ls;

	ls = jd->ls;

	W = wl_new(ls->basicblockcount);

	def_sites = DMNEW(bitvector, ls->ssavarcount);
	for(a = 0; a < ls->ssavarcount; a++)
		def_sites[a] = bv_new(ls->basicblockcount);

	ls->phi = DMNEW(int **, ls->basicblockcount);
	A_phi = DMNEW(bitvector, ls->basicblockcount);
	for(i = 0; i < ls->basicblockcount; i++) {
		ls->phi[i] = DMNEW(int *, ls->ssavarcount);
		for(j = 0; j < ls->ssavarcount; j++)
			ls->phi[i][j] = NULL;
		A_phi[i] = bv_new(ls->ssavarcount);
	}

	/* copy var_def to def_sites */
	/* var_def is valid for 0.. jd->basicblockcount (bb 0 for param init) */

	for(n = 0; n <= jd->basicblockcount; n++)
		for(a = 0; a < ls->ssavarcount; a++)
			if (bv_get_bit(ls->var_def[n], a))
				bv_set_bit(def_sites[a], n);
#ifdef SSA_DEBUG_VERBOSE
	if (compileverbose) {
		printf("var Definitions:\n");
		for(i = 0; i < ls->ssavarcount; i++) {
			printf("def_sites[%3i]=%p:",i,(void *)def_sites[i]);
			for(j = 0; j < ls->basicblockcount; j++) {
				if ((j % 5) == 0) printf(" ");
				if (bv_get_bit(def_sites[i], j))
					printf("1");
				else
					printf("0");
			}
			printf(" (");

			printf("\n");
		}
	}
#endif

	for(a = 0; a < ls->ssavarcount; a++) {

		/* W<-def_sites(a) */

		for(n = 0; n < ls->basicblockcount; n++)
			if (bv_get_bit(def_sites[a],n)) {
				wl_add(W, n);
			}
				
		while (!wl_is_empty(W)) { /* W not empty */
			n = wl_get(W);

			for(i = 0; i < dd->num_DF[n]; i++) {
				Y = dd->DF[n][i];
				/* Node Y is in Dominance Frontier of n -> */
				/* insert phi function for a at top of Y*/
				_SSA_CHECK_BOUNDS(Y, 0, ls->basicblockcount);
				if (bv_get_bit( A_phi[Y], a) == 0) { 
					/* a is not a Element of A_phi[Y] */
					/* a <- phi(a,a...,a) to be inserted at top of Block Y */
					/* phi has as many arguments, as Y has predecessors    */
#if 0
#if 0
					/* do not add a phi function for interface stackslots */
					/* if a predecessor is not a def site of a <==>       */
					/* the block does not have the corresponding inslot*/

/* 					if ((ls->var_to_index[a] >= 0) || */
/* 						(bv_get_bit(def_sites[a],  */
/* 									graph_get_first_predecessor(gd, Y, &iter)))) */

#endif
					/* for interface stackslots add a phi function only */
					/* if the basicblock has the corresponding incoming */
					/* stackslot -> it could be, that the stackslot is */
					/* not live anymore at Y */

#endif
					num_pred =  graph_get_num_predecessor(gd, Y);
					ls->phi[Y][a] = DMNEW(int, num_pred + 1);
					for (j = 0; j < num_pred + 1; j++)
						ls->phi[Y][a][j] = a;
					/* increment the number of definitions of a by one */
					/* for this phi function */
					ls->num_defs[a]++;
					
					bv_set_bit(A_phi[Y], a);
					if (bv_get_bit(ls->var_def[Y],a)==0) {
						/* Iterated Dominance Frontier Criterion:*/
						/* if Y had no definition for a insert it*/
						/* into the Worklist, since now it       */
						/* defines a through the phi function    */
						wl_add(W, Y);
					}
				}
			}
		}
	}
}

/* ssa_rename ******************************************************************

Rename the variables a (0 <= a < ls->ssavarcount) so that each new variable
has only one definition (SSA form).

ls->def_count[0..ls->ssavarcount[ holds the number of definitions of each var.
ls->var_0[0..ls->ssavarcount[ will be set to the new index of the first 
                              definition of each old var.
ls->varcount_with_indices     will be se to the new maximum varcount of LOCAL
                              and IOVARS.

All other vars (TEMPVAR and PREALLOC) will get a new unique index above 
ls->varcount_with_indices.

jd->var and jd->varcount will be set for this renamed vars.

*******************************************************************************/

void ssa_rename(jitdata *jd, graphdata *gd, dominatordata *dd)
{
	int i, mi, l, j, p, t;
	int type, flags;
	methoddesc *md = jd->m->parseddesc;

	varinfo *new_vars;
	lsradata *ls;

	ls = jd->ls;
	
	ssa_rename_init(jd, gd);

	/* Consider definition of Local Vars initialized with Arguments */
	/* in Block 0 */
	/* init is regarded as use too-> ssa_rename_use ->bullshit!!*/
 	for (p = 0, l= 0; p < md->paramcount; p++) {
 		t = md->paramtypes[p].type;
		mi = l * 5 + t;
		i = jd->local_map[mi];
		l++;
		if (IS_2_WORD_TYPE(t))
			l++;
		if (i == UNUSED)
			continue;
		/* !!!!! locals are now numbered as the parameters !!!! */
		/* !!!!! no additional increment for 2 word types !!!!! */
		/* this happens later on! here we still need the increment */
	    /* index of var can be in the range from 0 up to not including */
	    /* CD->maxlocals */

		/* ignore return value, since first definition gives 0 -> */
		/* no rename necessary */
		
		i = ls->new_varindex[i];
		j = ssa_rename_def_(ls, i);
		_SSA_ASSERT(j == 0);
		jd->local_map[mi] = i;
	}
	ssa_rename_(jd, gd, dd, 0);

#if 0
	/* DO _NOT_ DO THIS! Look at java.util.stringtokenizer.counttokens! */
	/* if there is no use of the defined Var itself by the phi function */
	/* for a loop path, in which this var is not used, it will not be life */
	/* in this path and overwritten! */

	/* Invalidate all xij from phi(xi0)=xi1,xi2,xi3,..,xin with xij == xi0 */
	/* this happens if the phi function is the first definition of x or in a */
	/* path with a backedge xi has no definition */ 
	/* a phi(xij) = ...,xij,... with the only use and definition of xij by */
	/* this phi function would otherwise "deadlock" the dead code elemination */
	/* invalidate means set it to ls->max_vars_with_indices */
	/* a phi function phi(xi0)=xi1,xi2,...xin wiht xij == xi0 for all j in */
	/* [1,n] can be removed */

	for(i = 0; i < ls->ssavarcount; i++) {
		for(t = 0; t < ls->basicblockcount; t++) {
			if (ls->phi[t][i] != 0) {
				remove_phi = true;
				for(p = 1; p <= graph_get_num_predecessor(gd, t); p++) {
					if (ls->phi[t][i][0] == ls->phi[t][i][p])
						ls->phi[t][i][p] = ls->varcount_with_indices;
					else 
						remove_phi = false;
				}
			}
			if (remove_phi)
				ls->phi[t][i] = NULL;
		}
	}
#endif

#if defined(SSA_DEBUG_CHECK) || defined(SSA_DEBUG_VERBOSE)
# if defined(SSA_DEBUG_VERBOSE)
	if (compileverbose) {
		printf("%s %s ",jd->m->class->name->text, jd->m->name->text);
		if (jd->isleafmethod)
			printf("**Leafmethod**");
		printf("\n");
	}
# endif
	if (strcmp(jd->m->class->name->text,"fp")==0)
		if (strcmp(jd->m->name->text,"testfloat")==0)
# if defined(SSA_DEBUG_VERBOSE)
			if (compileverbose) 
				printf("12-------------------12\n");
# else
	        { int dummy=1; dummy++; }
# endif
#endif
	/* recreate rd->locals[][] */
	/* now only one (local_index/type) pair exists anymore     */
	/* all var[t][i] with var_to_index[var[t][i]] >= 0 are locals */
	/* max local index after SSA indexing is in ls->local_0[ls->max_locals] */
	
	new_vars = DMNEW(varinfo, ls->vartop);
	for(i = 0; i < ls->vartop ; i++)
		new_vars[i].type = UNUSED;
	for(i = 0; i < jd->varcount; i++) {
			p = ls->new_varindex[i];
			if (p != UNUSED) {
				if (p < ls->ssavarcount)
					p = ls->var_0[p];
				new_vars[p].type  = VAR(i)->type;
				new_vars[p].flags = VAR(i)->flags;
				ls->lifetime[p].v_index = p;
				ls->lifetime[p].type = VAR(i)->type;
			}
	}

	/* take care of newly indexed local & in/out vars */

	for(i = 0; i < ls->ssavarcount; i++) {
		j = ls->var_0[i];
		type = new_vars[j].type;
		flags = new_vars[j].flags;
		j++;
		for (; j < ls->var_0[i + 1]; j++) {
			new_vars[j].type = type;
			new_vars[j].flags = flags;
			ls->lifetime[j].v_index = j;
			ls->lifetime[j].type = type;
		}
	}

#ifdef SSA_DEBUG_VERBOSE
	if (compileverbose) {

		printf("ssa_rename: Vars: Orig:%3i SSAVar: %3i\n", jd->varcount,
			   ls->ssavarcount);
		for(i = 0; i < jd->varcount; i++) {
			printf("%3i(%3i,%3x) ",i,VAR(i)->type, VAR(i)->flags);
			ssa_show_variable(jd, i, VAR(i),0);
			j = ls->new_varindex[i];
			if ((j != UNUSED) && (j < ls->ssavarcount))
				printf(" -> %3i ... %3i", ls->var_0[j], ls->var_0[j + 1] - 1);
			else
				printf(" -> %3i", j);
			printf("\n");
		}
	}
#endif

 	jd->var = new_vars;
 	jd->varcount = ls->vartop;
	jd->vartop = ls->vartop;

#ifdef SSA_DEBUG_VERBOSE
	if (compileverbose) {
		printf("ssa_rename: Vars: Orig:%3i SSAVar: %3i\n", jd->varcount,
			   ls->ssavarcount);
		for(i = 0; i < jd->varcount; i++) {
			printf("%3i(%3i,%3x) ",i,VAR(i)->type, VAR(i)->flags);
			ssa_show_variable(jd, i, VAR(i),0);
			printf("\n");
		}
	}
#endif
}

/* ssa_rename_init *************************************************************

Setup the data structure for ssa_rename

ls->def_count[0..ls->ssavarcount[ holds the number of definitions of each var.
ls->var_0[0..ls->ssavarcount[ will be set to the new index of the first 
                              definition of each old var.
ls->varcount_with_indices     will be se to the new maximum varcount of LOCAL
                              and IOVARS.

All other vars (TEMPVAR and PREALLOC) will get a new unique index above 
ls->varcount_with_indices.

jd->var and jd->varcount will be set for this renamed vars.

*******************************************************************************/

void ssa_rename_init(jitdata *jd, graphdata *gd) 
{
	int a, i, p;
	lsradata *ls;

	ls = jd->ls;
	
	/* set up new locals */
	/* ls->new_varindex[0..jd->varcount[ holds the new unique index */
	/* for locals and iovars */

	/* ls->num_defs[index] gives the number of indizes which will be created  */
	/* from SSA */

	/* -> vars will be numbered in this sequence: L0(0)..L0(i) L1(0)..L1(j) ..*/
	/* ls->var_0[X] will point to each LX(0)  */
	/* ls->var_0[ls->ssavarcount] will hold ls->varcount_with_indices */

	/* as first step cummulate the num_defs array for locals and iovars       */
	/* last element is the maximum var count */

	ls->var_0 = DMNEW(int, max(1, ls->ssavarcount + 1));
	ls->var_0[0] = 0;
	ls->varcount_with_indices = 0;
	for(i = 0; i < ls->ssavarcount; i++) {
		ls->varcount_with_indices += ls->num_defs[i];
		ls->var_0[i+1] = ls->var_0[i] + ls->num_defs[i];
	}

#if 0
	/* Change the var indices in phi from La to La(0) */

	for(i = 0; i < ls->basicblockcount; i++)
		for (a = 0; a < ls->ssavarcount; a++)
			if (ls->phi[i][a] != NULL)				
				for(p = 0; p < graph_get_num_predecessor(gd, i) + 1; p++)
					ls->phi[i][a][p] = ls->var_0[a];
#endif
	
	/* Initialization */

	ls->count     = DMNEW(int, max(1, ls->ssavarcount));
	ls->stack     = DMNEW(int *, max(1, ls->ssavarcount));
	ls->stack_top = DMNEW(int, max(1, ls->ssavarcount));
	for(a = 0; a < ls->ssavarcount; a++) {
		ls->count[a] = 0;
		ls->stack_top[a] = 0;

		/* stack a has to hold number of defs of a Elements + 1 */

		ls->stack[a] = DMNEW(int, ls->num_defs[a] + 1);
		ls->stack[a][ls->stack_top[a]++] = 0;
	}

	if (ls->ssavarcount > 0) {

		/* Create the num_var_use Array */

		ls->num_var_use = DMNEW(int *, ls->basicblockcount);
		for(i = 0; i < ls->basicblockcount; i++) {
			ls->num_var_use[i] =DMNEW(int, max(1, ls->varcount_with_indices));
			for(a = 0; a < ls->varcount_with_indices; a++)
				ls->num_var_use[i][a] = 0;
		}

		/* Create the use_sites Array of Bitvectors*/
		/* use max(1,..), to ensure that the array is created! */

		ls->use_sites =  DMNEW(bitvector, max(1, ls->varcount_with_indices));
		for(a = 0; a < ls->varcount_with_indices; a++)
			ls->use_sites[a] = bv_new(ls->basicblockcount);
	}

	/* init lifetimes */
	/* count number of TEMPVARs */

	ls->lifetimecount = 0;
	for(i = 0; i < jd->varcount; i++)
		if ((i >= jd->localcount) || (!(jd->var[i].flags & (INOUT | PREALLOC))))
			ls->lifetimecount++;

	ls->varcount = ls->varcount_with_indices + ls->lifetimecount;

	ls->lifetimecount = ls->varcount;
	ls->lifetime = DMNEW(struct lifetime, ls->lifetimecount);
	ls->lt_used = DMNEW(int, ls->lifetimecount);
	ls->lt_int = DMNEW(int, ls->lifetimecount);
	ls->lt_int_count = 0;
	ls->lt_flt = DMNEW(int, ls->lifetimecount);
	ls->lt_flt_count = 0;
	ls->lt_mem = DMNEW(int, ls->lifetimecount);
	ls->lt_mem_count = 0;
	for (i=0; i < ls->lifetimecount; i++) {
		ls->lifetime[i].type = UNUSED;
		ls->lifetime[i].savedvar = 0;		
		ls->lifetime[i].flags = 0;
		ls->lifetime[i].usagecount = 0;
		ls->lifetime[i].bb_last_use = -1;
		ls->lifetime[i].bb_first_def = -1;
		ls->lifetime[i].use = NULL;
		ls->lifetime[i].def = NULL;
		ls->lifetime[i].last_use = NULL;
	}

	/* for giving TEMP and PREALLOC vars a new unique index */

	ls->vartop = ls->varcount_with_indices; 

#ifdef SSA_DEBUG_VERBOSE
	if (compileverbose) {
		printf("ssa_rename_init: Vars: Orig:%3i SSAVar: %3i\n", jd->varcount,
			   ls->ssavarcount);
		for(i = 0; i < jd->varcount; i++) {
			if ((i < jd->localcount) || ( VAR(i)->flags & INOUT)) {
				printf("%3i(%3i,%3x) ",i,VAR(i)->type, VAR(i)->flags);
				ssa_show_variable(jd, i, VAR(i),0);
				if ((i < ls->ssavarcount) || (VAR(i)->flags & INOUT)) {
					printf(" -> %3i", ls->new_varindex[i]);
				}
				printf("\n");
			}
		}
		ssa_print_phi(ls, gd);
	}
#endif
}

int ssa_rename_def_(lsradata *ls, int a) {
	int i;
	
	_SSA_CHECK_BOUNDS(a,0,ls->ssavarcount);
	ls->count[a]++;
	i = ls->count[a] - 1;
	/* push i on stack[a] */
	_SSA_CHECK_BOUNDS(ls->stack_top[a], 0, ls->num_defs[a] + 1);
	ls->stack[a][ls->stack_top[a]++] = i;
	return i;
}

int ssa_rename_def(jitdata *jd, int *def_count, int a) {
	int i, a1, ret;
	lsradata *ls;

	ls = jd->ls;
	
	a1 = ls->new_varindex[a];
	_SSA_CHECK_BOUNDS(a1, UNUSED, ls->varcount);
	if ((a1 != UNUSED) && (a1 < ls->ssavarcount)) {
		/* local or inoutvar -> normal ssa renaming */
		_SSA_ASSERT((a < jd->localcount) || (VAR(a)->flags & INOUT));
		/* !IS_TEMPVAR && !IS_PREALLOC == (IS_LOCALVAR || IS_INOUT) */

		def_count[a1]++;
		i = ssa_rename_def_(ls, a1);
		ret = ls->var_0[a1] + i;
	}
	else {
		/* TEMP or PREALLOC var */
		if (a1 == UNUSED) {
			ls->new_varindex[a] = ls->vartop;
			ret = ls->vartop;
			ls->vartop++;
			_SSA_ASSERT( ls->vartop < ls->varcount);
		}
		else
			ret = a1;
	}
	return ret;
}

void ssa_rename_use_(lsradata *ls, int n, int a) {
	_SSA_CHECK_BOUNDS(a, 0, ls->varcount_with_indices);
	if (ls->ssavarcount > 0) {
		bv_set_bit(ls->use_sites[a], n);
		ls->num_var_use[n][a]++;
	}
}

int ssa_rename_use(lsradata *ls, int n, int a) {
	int a1, i;
	int ret;

	a1 = ls->new_varindex[a];
	_SSA_CHECK_BOUNDS(a1, UNUSED, ls->varcount);
	if ((a1 != UNUSED) && (a1 < ls->ssavarcount)) {
		/* local or inoutvar -> normal ssa renaming */
		/* i <- top(stack[a]) */

		_SSA_CHECK_BOUNDS(ls->stack_top[a1]-1, 0, ls->num_defs[a1]+1);
		i = ls->stack[a1][ls->stack_top[a1] - 1]; 
		_SSA_CHECK_BOUNDS(i, 0, ls->num_defs[a1]);

		ret = ls->var_0[a1] + i;
	}
	else {
		/* TEMP or PREALLOC var */
		if (a1 == UNUSED) {
			ls->new_varindex[a] = ls->vartop;
			ret = ls->vartop;
			ls->vartop++;
			_SSA_ASSERT( ls->vartop < ls->varcount);
		}
		else
			ret = a1;
	}

	return ret;
}

#ifdef SSA_DEBUG_VERBOSE
void ssa_rename_print(instruction *iptr, char *op, int from,  int to) {
	if (compileverbose) {
		printf("ssa_rename_: ");
		if (iptr != NULL)
			printf("%s ", opcode_names[iptr->opc]);
		else
			printf("       ");

		printf("%s: %3i->%3i\n", op, from, to);
	}
}
#endif

void ssa_rename_(jitdata *jd, graphdata *gd, dominatordata *dd, int n) {
	int a, i, j, k, iindex, Y, v;
	int in_d, out_d;
	int *def_count;
	/* [0..ls->varcount[ Number of Definitions of this var in this  */
	/* Basic Block. Used to remove the entries off the stack at the */
	/* end of the function */
	instruction *iptr;
	s4 *in, *out, *argp;
	graphiterator iter_succ, iter_pred;
	struct lifetime *lt;

	methoddesc *md;
	methodinfo *m;
	builtintable_entry *bte;
	lsradata *ls;

	ls = jd->ls;
	m  = jd->m;

#ifdef SSA_DEBUG_VERBOSE
	if (compileverbose)
		printf("ssa_rename_: BB %i\n",n);
#endif

	_SSA_CHECK_BOUNDS(n, 0, ls->basicblockcount);

	def_count = DMNEW(int, max(1, ls->ssavarcount));
	for(i = 0; i < ls->ssavarcount; i++)
		def_count[i] = 0;

	/* change Store of possible phi functions from a to ai*/

	for(a = 0; a < ls->ssavarcount; a++)
		if (ls->phi[n][a] != NULL) {
			def_count[a]++;
				/* do not mark this store as use - maybee this phi function */
				/* can be removed for unused Vars*/
			j = ls->var_0[a] + ssa_rename_def_(ls, a);
#ifdef SSA_DEBUG_VERBOSE
			ssa_rename_print( NULL, "phi-st", ls->phi[n][a][0], j);
#endif
			ls->phi[n][a][0] = j;
		}

	in   = ls->basicblocks[n]->invars;
	in_d = ls->basicblocks[n]->indepth - 1;

	/* change use of instack Interface stackslots except top SBR and EXH */
	/* stackslots */

	if ((ls->basicblocks[n]->type == BBTYPE_EXH) ||
		(ls->basicblocks[n]->type == BBTYPE_SBR)) {
		in_d--;
	}
/* 	out   = ls->basicblocks[n]->outvars; */
/* 	out_d = ls->basicblocks[n]->outdepth - 1; */

/* 	for(; out_d > in_d; out_d--); */

	for (; in_d >= 0; in_d--) {
		/* Possible Use of ls->new_varindex[jd->var[in_d]] */
		_SSA_ASSERT(ls->new_varindex[in[in_d]] != UNUSED);

		a = ls->new_varindex[in[in_d]];
		_SSA_CHECK_BOUNDS(a, 0, ls->ssavarcount);

		/* i <- top(stack[a]) */

		_SSA_CHECK_BOUNDS(ls->stack_top[a]-1, 0, ls->num_defs[a]+1);
		i = ls->stack[a][ls->stack_top[a]-1]; 
		_SSA_CHECK_BOUNDS(i, 0, ls->num_defs[a]);

		/* Replace use of x with xi */

#ifdef SSA_DEBUG_VERBOSE
			ssa_rename_print( NULL, "invar", in[in_d], ls->var_0[a]+i);
#endif
		in[in_d] = ls->var_0[a] + i;
		lt = ls->lifetime + in[in_d];

		lt->v_index = in[in_d];
		lt->bb_last_use = -1;
	}

	iptr = ls->basicblocks[n]->iinstr;

	for(iindex = 0; iindex < ls->basicblocks[n]->icount; iindex++, iptr++) {

		/* check for use (s1, s2, s3 or special (argp) ) */

		switch (icmd_table[iptr->opc].dataflow) {
		case DF_3_TO_0:
		case DF_3_TO_1: /* icmd has s1, s2 and s3 */
			j = ssa_rename_use(ls, n, iptr->sx.s23.s3.varindex);
#ifdef SSA_DEBUG_VERBOSE
			ssa_rename_print( iptr, "s3 ", iptr->sx.s23.s3.varindex, j);
#endif
			iptr->sx.s23.s3.varindex = j;

			/* now "fall through" for handling of s2 and s1 */

		case DF_2_TO_0:
		case DF_2_TO_1: /* icmd has s1 and s2 */
			j = ssa_rename_use(ls, n, iptr->sx.s23.s2.varindex);
#ifdef SSA_DEBUG_VERBOSE
			ssa_rename_print( iptr, "s2 ", iptr->sx.s23.s2.varindex, j);
#endif
			iptr->sx.s23.s2.varindex = j;

			/* now "fall through" for handling of s1 */

		case DF_1_TO_0:
		case DF_1_TO_1:
		case DF_MOVE:
		case DF_COPY:
			j = ssa_rename_use(ls, n, iptr->s1.varindex);
#ifdef SSA_DEBUG_VERBOSE
			ssa_rename_print( iptr, "s1 ", iptr->s1.varindex, j);
#endif
			iptr->s1.varindex = j;
			break;

		case DF_INVOKE:
		case DF_BUILTIN:
		case DF_N_TO_1:
			/* do not use md->paramcount, pass-through stackslots have */
			/* to be renamed, too */
			i = iptr->s1.argcount;
			argp = iptr->sx.s23.s2.args;
			while (--i >= 0) {
				j = ssa_rename_use(ls, n, *argp);
#ifdef SSA_DEBUG_VERBOSE
				ssa_rename_print( iptr, "arg", *argp, j);
#endif
				*argp = j;
				argp++;
			}
			break;
		}
			

		/* Look for definitions (iptr->dst). INVOKE and BUILTIN have */
		/* an optional dst - so they to be checked first */

		v = UNUSED;
		if (icmd_table[iptr->opc].dataflow == DF_INVOKE) {
			INSTRUCTION_GET_METHODDESC(iptr,md);
			if (md->returntype.type != TYPE_VOID)
				v = iptr->dst.varindex;
		}
		else if (icmd_table[iptr->opc].dataflow == DF_BUILTIN) {
			bte = iptr->sx.s23.s3.bte;
			md = bte->md;
			if (md->returntype.type != TYPE_VOID)
				v = iptr->dst.varindex;
		}
		else if (icmd_table[iptr->opc].dataflow >= DF_DST_BASE) {
			v = iptr->dst.varindex;
		}

		if (v != UNUSED) {
			j = ssa_rename_def(jd, def_count, iptr->dst.varindex);
#ifdef SSA_DEBUG_VERBOSE
			ssa_rename_print( iptr, "dst", iptr->dst.varindex, j);
#endif
			iptr->dst.varindex = j;
		}

				/* ?????????????????????????????????????????????????????????? */
				/* Mark Def as use, too. Since param initialisation is in     */
				/* var_def and this would not remove this locals, if not used */
				/* elsewhere   */
				/* ?????????????????????????????????????????????????????????? */

	}

	/* change outstack Interface stackslots */
	out = ls->basicblocks[n]->outvars;
	out_d = ls->basicblocks[n]->outdepth - 1;

	for (;out_d >= 0; out_d--) {
		/* Possible Use of ls->new_varindex[jd->var[out_d]] */
		_SSA_ASSERT(ls->new_varindex[out[out_d]] != UNUSED);

		a = ls->new_varindex[out[out_d]];
		_SSA_CHECK_BOUNDS(a, 0, ls->ssavarcount);

		/* i <- top(stack[a]) */

		_SSA_CHECK_BOUNDS(ls->stack_top[a]-1, 0, ls->num_defs[a]+1);
		i = ls->stack[a][ls->stack_top[a]-1]; 
		_SSA_CHECK_BOUNDS(i, 0, ls->num_defs[a]);

		/* Replace use of x with xi */

#ifdef SSA_DEBUG_VERBOSE
			ssa_rename_print( NULL, "outvar", out[out_d], ls->var_0[a]+i);
#endif
		out[out_d] = ls->var_0[a] + i;
		lt = ls->lifetime + out[out_d];

		lt->v_index = out[out_d];
		lt->bb_last_use = -1;
	}

	/* change use in phi Functions of Successors */

	Y = graph_get_first_successor(gd, n, &iter_succ);
	for(; Y != -1; Y = graph_get_next(&iter_succ)) {
		_SSA_CHECK_BOUNDS(Y, 0, ls->basicblockcount);
		k = graph_get_first_predecessor(gd, Y, &iter_pred);
		for (j = 0; (k != -1) && (k != n); j++)
			k = graph_get_next(&iter_pred);
		_SSA_ASSERT(k == n);

		/* n is jth Predecessor of Y */

		for(a = 0; a < ls->ssavarcount; a++)
			if (ls->phi[Y][a] != NULL) {

				/* i <- top(stack[a]) */
				
				if (ls->stack_top[a] == 1) {
					/* no definition till now in controll flow */
#ifdef SSA_DEBUG_VERBOSE
					if (compileverbose) {
						printf("Succ %3i Arg %3i \n", Y, j);
						ssa_rename_print( NULL, "phi-use", ls->phi[Y][a][j+1], UNUSED);
					}
#endif
					ls->phi[Y][a][j+1] = UNUSED;
				}
				else {
					_SSA_CHECK_BOUNDS(ls->stack_top[a]-1, 0, ls->num_defs[a]+1);
					i = ls->stack[a][ls->stack_top[a]-1]; 
					_SSA_CHECK_BOUNDS(i, 0, ls->num_defs[a]);

					/* change jth operand from a0 to ai */

					i = ls->var_0[a] + i;
#ifdef SSA_DEBUG_VERBOSE
					if (compileverbose) {
						printf("Succ %3i Arg %3i \n", Y, j);
						ssa_rename_print( NULL, "phi-use", ls->phi[Y][a][j+1], i);
					}
#endif
					ls->phi[Y][a][j+1] = i;
					_SSA_CHECK_BOUNDS(ls->phi[Y][a][j+1], 0,
									  ls->varcount_with_indices);

					/* use by phi function has to be remembered, too */

					ssa_rename_use_(ls, n, ls->phi[Y][a][j+1]);
				}

				/* ????????????????????????????????????????????? */
				/* why was this only for local vars before ?     */
				/* ????????????????????????????????????????????? */

			}
	}
	
	/* Call ssa_rename_ for all Childs of n of the Dominator Tree */
	for(i = 0; i < ls->basicblockcount; i++)
		if (dd->idom[i] == n)
			ssa_rename_(jd, gd, dd, i);

	/* pop Stack[a] for each definition of a var a in the original S */
	for(a = 0; a < ls->ssavarcount; a++) {
		ls->stack_top[a] -= def_count[a];
		_SSA_ASSERT(ls->stack_top[a] >= 0);
	}
}



#ifdef SSA_DEBUG_VERBOSE
void ssa_print_trees(jitdata *jd, graphdata *gd, dominatordata *dd) {
	int i,j;
	lsradata *ls;

	ls = jd->ls;

	printf("ssa_printtrees: maxlocals %3i", jd->localcount);
		
	printf("Dominator Tree: \n");
	for(i = 0; i < ls->basicblockcount; i++) {
		printf("%3i:",i);
		for(j = 0; j < ls->basicblockcount; j++) {
			if (dd->idom[j] == i) {
				printf(" %3i", j);
			}
		}
		printf("\n");
	}

	printf("Dominator Forest:\n");
	for(i = 0; i < ls->basicblockcount; i++) {
		printf("%3i:",i);
		for(j = 0; j < dd->num_DF[i]; j++) {
				printf(" %3i", dd->DF[i][j]);
		}
		printf("\n");
	}
#if 0
	if (ls->ssavarcount > 0) {
	printf("Use Sites\n");
   	for(i = 0; i < ls->ssavarcount; i++) {
		printf("use_sites[%3i]=%p:",i,(void *)ls->use_sites[i]);
		for(j = 0; j < ls->basicblockcount; j++) {
			if ((j % 5) == 0) printf(" ");
			if (bv_get_bit(ls->use_sites[i], j))
				printf("1");
			else
				printf("0");
		}
		printf("\n");
	}
	}
#endif
	printf("var Definitions:\n");
   	for(i = 0; i < jd->basicblockcount; i++) {
		printf("var_def[%3i]=%p:",i,(void *)ls->var_def[i]);
		for(j = 0; j < ls->ssavarcount; j++) {
			if ((j % 5) == 0) printf(" ");
			if (bv_get_bit(ls->var_def[i], j))
				printf("1");
			else
				printf("0");
		}
		printf(" (");
		for(j=0; j < ((((ls->ssavarcount * 5+7)/8) + sizeof(int) - 1)/sizeof(int));
			j++)
			printf("%8x",ls->var_def[i][j]);
		printf(")\n");
	}
}

void ssa_print_phi(lsradata *ls, graphdata *gd) {
	int i,j,k;

	printf("phi Functions (varcount_with_indices: %3i):\n", 
		   ls->varcount_with_indices);
	for(i = 0; i < ls->basicblockcount; i++) {
		for(j = 0; j < ls->ssavarcount; j++) {
			if (ls->phi[i][j] != NULL) {
				printf("BB %3i %3i = phi(", i, ls->phi[i][j][0]);
				for(k = 1; k <= graph_get_num_predecessor(gd, i); k++)
					printf("%3i ",ls->phi[i][j][k]);
				printf(")\n");
			}
		}
	}

}

#endif

void ssa_generate_phi_moves(jitdata *jd, graphdata *gd) {
	int a, i, j, pred;
	graphiterator iter;
	lsradata *ls;

	ls = jd->ls;

	/* count moves to be inserted at the end of each block in moves[] */
	ls->num_phi_moves = DMNEW(int, ls->basicblockcount);
	for(i = 0; i < ls->basicblockcount; i++)
		ls->num_phi_moves[i] = 0;
	for(i = 0; i < ls->basicblockcount; i++)
		for(a = 0; a < ls->ssavarcount; a++)
			if (ls->phi[i][a] != NULL) {
#if 0
				if (ls->lifetime[ls->phi[i][a][0]].use == NULL) {
					/* Var defined (only <- SSA Form!) in this phi function */
					/* and not used anywhere -> delete phi function and set */
					/* var to unused */

					/* TODO: first delete use sites of arguments of phi */
					/* function */
					VAR(ls->lifetime[ls->phi[i][a][0]].v_index)->type = UNUSED;
					ls->lifetime[ls->phi[i][a][0]].def = NULL;
					ls->phi[i][a] = NULL;
				}
				else 
#endif
					{
					pred = graph_get_first_predecessor(gd, i, &iter);
					for(; pred != -1; pred = graph_get_next(&iter)) {
						ls->num_phi_moves[pred]++;
					}
				}
			}

	/* allocate ls->phi_moves */
	ls->phi_moves = DMNEW( int **, ls->basicblockcount);
	for(i = 0; i < ls->basicblockcount; i++) {
		ls->phi_moves[i] = DMNEW( int *, ls->num_phi_moves[i]);
		for(j = 0; j <ls->num_phi_moves[i]; j++)
			ls->phi_moves[i][j] = DMNEW(int, 2);
#ifdef SSA_DEBUG_VERBOSE
		if (compileverbose)
			printf("ssa_generate_phi_moves: ls_num_phi_moves[%3i] = %3i\n",
				   i, ls->num_phi_moves[i]);
#endif
	}

	/* populate ls->phi_moves */
	for(i = 0; i < ls->basicblockcount; i++)
		ls->num_phi_moves[i] = 0;
	for(i = 0; i < ls->basicblockcount; i++)
		for(a = 0; a < ls->ssavarcount; a++)
			if (ls->phi[i][a] != NULL) {
				pred = graph_get_first_predecessor(gd, i, &iter);
				for(j = 0; pred != -1; j++, pred = graph_get_next(&iter)) {
					/* target is phi[i][a][0] */
					/* source is phi[i][a][j+1] */
					if (ls->phi[i][a][j+1] != ls->varcount_with_indices) {
						/* valid move */
						if (ls->phi[i][a][0] != ls->phi[i][a][j+1]) {
							ls->phi_moves[pred][ls->num_phi_moves[pred]][0] =
								ls->phi[i][a][0];
							ls->phi_moves[pred][(ls->num_phi_moves[pred])++][1]=
								ls->phi[i][a][j+1];
						}
					}
				}
			}
}

/****************************************************************************
Optimizations
****************************************************************************/


/****************************************************************************
Dead Code Elimination
****************************************************************************/
void dead_code_elimination(jitdata *jd, graphdata *gd) {
	int a, i, source;
	worklist *W;

	instruction *iptr;
	struct lifetime *lt, *s_lt;

	bool remove_statement;
	struct site *use;

	lsradata *ls;
#ifdef SSA_DEBUG_VERBOSE
	methodinfo *m;

	m  = jd->m;
#endif
	ls = jd->ls;

	W = wl_new(ls->lifetimecount);
	if (ls->lifetimecount > 0) {
		/* put all lifetimes on Worklist */
		for(a = 0; a < ls->lifetimecount; a++) {
			if (ls->lifetime[a].type != -1) {
				wl_add(W, a);
			}
		}
	}

	/* Remove unused lifetimes */
	while(!wl_is_empty(W)) {
		/* take a var out of Worklist */
		a = wl_get(W);

		lt = &(ls->lifetime[a]);
		if ((lt->def == NULL) || (lt->type == -1))
			/* lifetime was already removed -> no defs anymore */
			continue;

		/* Remove lifetimes, which are only used in a phi function, which 
		   defines them! */
		remove_statement = (lt->use != NULL) && (lt->use->iindex < 0);
		for(use = lt->use; (remove_statement && (use != NULL)); 
			use = use->next)
		{
			remove_statement = remove_statement && 
				(use->b_index == lt->def->b_index) &&
				(use->iindex == lt->def->iindex);
		}
		if (remove_statement) {
#ifdef SSA_DEBUG_CHECK
			/* def == use can only happen in phi functions */
			if (remove_statement)
				_SSA_ASSERT(lt->use->iindex < 0);
#endif
			/* give it free for removal */
			lt->use = NULL;
		}

		if (lt->use == NULL) {
			/* Look at statement of definition of a and remove it,           */
			/* if the Statement has no sideeffects other than the assignemnt */
			/* of a */
			if (lt->def->iindex < 0 ) {
				/* phi function */
				/* delete use of sources , delete phi functions  */

				_SSA_ASSERT(ls->phi[lt->def->b_index][-lt->def->iindex-1] !=
							NULL);

				for (i = 1;i <= graph_get_num_predecessor(gd, lt->def->b_index);
					 i++) 
					{
						source =
							ls->phi[lt->def->b_index][-lt->def->iindex-1][i];
						if ((source != ls->varcount_with_indices) && 
							(source != lt->v_index)) {

							/* phi Argument was not already removed (already in 
							   because of selfdefinition) */

							s_lt = &(ls->lifetime[source]);

							/* remove it */

							lt_remove_use_site(s_lt,lt->def->b_index,
											lt->def->iindex);

							/*  put it on the Worklist */

							wl_add(W, source);
						}
					}
				/* now delete phi function itself */
				ls->phi[lt->def->b_index][-lt->def->iindex-1] = NULL;
			} else {
				/* "normal" Use by ICMD */
				remove_statement = false;
				if (lt->def->b_index != 0) {

					/* do not look at artificial block 0 (parameter init) */

					iptr = ls->basicblocks[lt->def->b_index]->iinstr + 
						lt->def->iindex;

					if (icmd_table[iptr->opc].flags & ICMDTABLE_PEI)
						remove_statement = false;

					/* if ICMD could throw an exception do not remove it! */

					else {

						/* ICMD_INVOKE*, ICMD_BUILTIN and ICMD_MULTIANEWARRAY */
						/* have possible sideeffects -> do not remove them    */

/* 						remove_statement = !(ICMD_HAS_SPECIAL(iptr->opc)); */

						remove_statement = !(
							(icmd_table[iptr->opc].dataflow == DF_INVOKE) ||
							(icmd_table[iptr->opc].dataflow == DF_BUILTIN) ||
							(icmd_table[iptr->opc].dataflow == DF_N_TO_1));

						if (remove_statement) {
							switch (icmd_table[iptr->opc].dataflow) {
							case DF_3_TO_0:
							case DF_3_TO_1: /* icmd has s1, s2 and s3 */
								a = iptr->sx.s23.s3.varindex;
								s_lt = ls->lifetime + a;
								lt_remove_use_site(s_lt, lt->def->b_index,
												   lt->def->iindex);
								wl_add(W, a);

								/* now "fall through" for handling of s2 and s1 */

							case DF_2_TO_0:
							case DF_2_TO_1: /* icmd has s1 and s2 */
								a = iptr->sx.s23.s2.varindex;
								s_lt = ls->lifetime + a;
								lt_remove_use_site(s_lt, lt->def->b_index,
												   lt->def->iindex);
								wl_add(W, a);

								/* now "fall through" for handling of s1 */

							case DF_1_TO_0:
							case DF_1_TO_1:
							case DF_MOVE:
							case DF_COPY:
								a = iptr->s1.varindex;
								s_lt = ls->lifetime + a;
								lt_remove_use_site(s_lt, lt->def->b_index,
												   lt->def->iindex);
								wl_add(W, a);
							}
						}
					}

					if (remove_statement) {

						/* remove statement */

#ifdef SSA_DEBUG_VERBOSE
						if (compileverbose)
							printf("INFO: %s %s:at BB %3i II %3i NOP-<%s\n",
								   m->class->name->text, m->name->text, 
								   lt->def->b_index, lt->def->iindex, 
								   icmd_table[iptr->opc].name);
#endif
						iptr->opc = ICMD_NOP;
  					}
				} /* (lt->def->b_index != 0) */
			} /* if (lt->def->iindex < 0 ) else */

			/* remove definition of a */

			VAR(lt->v_index)->type = -1;
			lt->type = -1;
			lt->def = NULL;
		} /* if (lt->use == NULL) */
	} /* while(!wl_is_empty(W)) */
} /* dead_code_elimination */

/****************************************************************************
Simple Constant Propagation
****************************************************************************/

void simple_constant_propagation() {
}

/****************************************************************************
Optimization
*******************************************************************************/
void copy_propagation(jitdata *jd, graphdata *gd) {
	int a, i, source;
	int only_source;

	worklist *W;

	instruction *iptr;
	struct lifetime *lt, *s_lt;
	s4 *in;

	lsradata *ls;

	ls = jd->ls;

	W = wl_new(ls->lifetimecount);
	if (ls->lifetimecount > 0) {
		/* put all lifetimes on Worklist */
		for(a = 0; a < ls->lifetimecount; a++) {
			if (ls->lifetime[a].type != -1) {
				wl_add(W, a);
			}
		}
	}

	while(!wl_is_empty(W)) {
		/* take a var out of Worklist */
		a = wl_get(W);

		lt = ls->lifetime + a;
		if (lt->type == -1)
			continue;
		_SSA_ASSERT(lt->def != NULL);
		_SSA_ASSERT(lt->use != NULL);
		if (lt->def->iindex < 0 ) {

			/* phi function */
			/* look, if phi function degenerated to a x = phi(y) */
			/* and if so, substitute y for every use of x */

			_SSA_ASSERT(ls->phi[lt->def->b_index][-lt->def->iindex-1] != NULL);

			only_source = ls->varcount_with_indices;
			for (i = 1; i <= graph_get_num_predecessor(gd, lt->def->b_index);
				 i++) {
					source = ls->phi[lt->def->b_index][-lt->def->iindex-1][i];
					if (source != ls->varcount_with_indices) {	
						if (only_source == ls->varcount_with_indices) {
							/* first valid source argument of phi function */
							only_source = source;
						} else {
							/* second valid source argument of phi function */
							/* exit for loop */
							only_source = ls->varcount_with_indices;
							break;
						}
					}
			}
			if (only_source != ls->varcount_with_indices) {
				
				/* replace all use sites of lt with the var_index only_source */

				ssa_replace_use_sites( jd, gd, lt, only_source, W);

				/* delete def of lt and replace uses of lt with "only_source" */

				ls->phi[lt->def->b_index][-lt->def->iindex-1] = NULL;

				s_lt = ls->lifetime + only_source;

				VAR(lt->v_index)->type = -1;
				lt_remove_use_site(s_lt, lt->def->b_index, lt->def->iindex);
				lt->def = NULL;
				/* move use sites from lt to s_lt */
				lt_move_use_sites(lt, s_lt);
				lt->type = -1;
			} /* if (only_source != ls->varcount_with_indices) */
		} else { /* if (lt->def->iindex < 0 )*/	
			/* def in "normal" ICMD */
#if 0
			iptr = ls->basicblocks[lt->def->b_index]->iinstr + 
				lt->def->iindex;
		
			if ( localvar ) {
				if (lt->def->b_index == 0)
					continue;
				
				switch(iptr->opc) {
				case ICMD_ISTORE:
				case ICMD_LSTORE:
				case ICMD_FSTORE:
				case ICMD_DSTORE:
			case ICMD_ASTORE:
					if (lt->def->iindex == 0) {
						/* first instruction in bb -> instack==bb->instack */
						in = ls->basicblocks[lt->def->b_index]->instack;
					} else {
						/* instack is (iptr-1)->dst */
						in = (iptr-1)->dst;
					}
					
					if (in->varkind != LOCALVAR) {
#ifdef SSA_DEBUG_VERBOSE
						if (compileverbose)
							printf("copy propagation xstore: BB %3i I %3i: %3i -> %3i\n", lt->def->b_index, lt->def->iindex, iptr->op1, in->varnum);
#endif
						s_lt = &(ls->lifetime[-in->varnum-1]);
						
						for (ss = s_lt->local_ss; ss != NULL; ss = ss->next) {
							ss->s->varkind = LOCALVAR;
							ss->s->varnum = iptr->op1;
						}
						
						/* replace all use sites of s_lt with the var_index */
						/* iptr->op1 */

						ssa_replace_use_sites(jd, gd, s_lt, iptr->op1, W);
				
						/* s_lt->def is the new def site of lt */
						/* the old ->def site will get a use site of def */
						/* only one def site */
						_SSA_ASSERT(lt->def->next == NULL);
						_SSA_ASSERT(s_lt->def != NULL);
						_SSA_ASSERT(s_lt->def->next == NULL);

						/* replace def of s_lt with iptr->op1 */
						if (s_lt->def->iindex < 0) {
							/* phi function */
							_SSA_ASSERT(ls->phi[s_lt->def->b_index]
									       [-s_lt->def->iindex-1]
									!= NULL);
							ls->phi[s_lt->def->b_index][-s_lt->def->iindex-1][0]
								= iptr->op1;
						} else
							if (in->varnum != iptr->op1)
								printf("copy propagation: LOCALVAR ss->ISTORE BB %i II %i\n",
									   lt->def->b_index, lt->def->iindex);
							

						/* move def to use sites of lt */
						lt->def->next = lt->use;
						lt->use = lt->def;
						
						lt->def = s_lt->def;

						s_lt->def = NULL;


						/* move use sites from s_lt to lt */
						move_use_sites(s_lt, lt);
						move_stackslots(s_lt, lt);
						s_lt->type = -1;
					}
					break;
				}
			} else {
				/* Def Interface Stackslot */

				switch(iptr->opc) {
				case ICMD_ILOAD:
				case ICMD_LLOAD:
				case ICMD_FLOAD:
				case ICMD_DLOAD:
				case ICMD_ALOAD:
					only_source = lt->local_ss->s->varnum;
					if (lt->local_ss->s->varkind != LOCALVAR) {
#ifdef SSA_DEBUG_VERBOSE
						if (compileverbose)
							printf("copy propagation xload: BB %3i I %3i: %3i -> %3i\n", lt->def->b_index, lt->def->iindex, iptr->op1, lt->local_ss->s->varnum);
#endif
						_SSA_ASSERT(iptr->dst->varnum == lt->local_ss->s->varnum);
						for (ss = lt->local_ss; ss != NULL; ss = ss->next) {
							ss->s->varkind = LOCALVAR;
							ss->s->varnum = iptr->op1;
						}

						/* replace all use sites of lt with the var_index iptr->op1*/

						ssa_replace_use_sites(jd, gd, lt, iptr->op1, W);
				
						lt->def = NULL;

						s_lt = &(ls->lifetime[ls->maxlifetimes + iptr->op1]);

						/* move use sites from lt to s_lt */
						move_use_sites(lt, s_lt);
						move_stackslots(lt, s_lt);
						lt->type = -1;
					} else
						if (lt->local_ss->s->varnum != iptr->op1)
							printf("copy propagation: ILOAD -> LOCALVAR ss BB %i II %i\n",
								   lt->def->b_index, lt->def->iindex);
					
					break;
				}
			} /* localvar or interface stackslot */
#endif
		} /* i(lt->def->iindex < 0 ) */
		
	} /* while(!wl_is_empty(W)) */
}

void ssa_replace_use_sites(jitdata *jd, graphdata *gd, struct lifetime *lt,
						   int new_v_index, worklist *W) {
	struct site *s;
	int i, source;
	instruction *iptr;
	s4 *argp;

	methoddesc *md;
	builtintable_entry *bte;
	lsradata *ls;

	ls = jd->ls;
	md = jd->m->parseddesc;
	

	for(s = lt->use; s != NULL; s = s->next) {
		if (s->iindex < 0) {

			/* Use in phi function */

			for (i = 1;i <= graph_get_num_predecessor(gd, s->b_index); i++) {
				source = ls->phi[s->b_index][-s->iindex-1][i];
				if (source == lt->v_index) {	
#ifdef SSA_DEBUG_VERBOSE
					if (W != NULL) {
						if (compileverbose)
							printf("copy propagation phi: BB %3i I %3i: %3i -> \
                                     %3i\n", s->b_index, s->iindex,
								   new_v_index, source);
					}
#endif
					ls->phi[s->b_index][-s->iindex-1][i]
						= new_v_index;
				}
			}
			if (W != NULL) {

				/* Add var, which is defined by this phi function to */
				/* the worklist */

				source = ls->phi[s->b_index][-s->iindex-1][0];
				wl_add(W, source);
			}
		}
		else { /* use in ICMD */
	
			iptr = ls->basicblocks[s->b_index]->iinstr + 
				s->iindex;

		/* check for use (s1, s2, s3 or special (argp) ) */

			i = UNUSED;
			switch (icmd_table[iptr->opc].dataflow) {
			case DF_3_TO_0:
			case DF_3_TO_1: /* icmd has s1, s2 and s3 */
				if (iptr->sx.s23.s3.varindex == lt->v_index) {
#ifdef SSA_DEBUG_VERBOSE
					if (W != NULL) {
						if (compileverbose)
							printf("copy propagation loc: BB %3i I %3i: %3i -> \
                                    %3i\n", s->b_index, s->iindex,
								   new_v_index, iptr->sx.s23.s3.varindex);
					}
#endif
					iptr->sx.s23.s3.varindex = new_v_index;
				}

				/* now "fall through" for handling of s2 and s1 */

			case DF_2_TO_0:
			case DF_2_TO_1: /* icmd has s1 and s2 */
				if (iptr->sx.s23.s2.varindex == lt->v_index) {
#ifdef SSA_DEBUG_VERBOSE
					if (W != NULL) {
						if (compileverbose)
							printf("copy propagation loc: BB %3i I %3i: %3i -> \
                                    %3i\n", s->b_index, s->iindex,
								   new_v_index, iptr->sx.s23.s2.varindex);
					}
#endif
					iptr->sx.s23.s2.varindex = new_v_index;
				}

				/* now "fall through" for handling of s1 */

			case DF_1_TO_0:
			case DF_1_TO_1:
			case DF_MOVE:
			case DF_COPY:
				if (iptr->s1.varindex == lt->v_index) {
#ifdef SSA_DEBUG_VERBOSE
					if (W != NULL) {
						if (compileverbose)
							printf("copy propagation loc: BB %3i I %3i: %3i -> \
                                    %3i\n", s->b_index, s->iindex,
								   new_v_index, iptr->s1.varindex);
					}
#endif
					iptr->s1.varindex = new_v_index;
				}
				break;

				/* TODO: */
				/* ? really look at md->paramcount or iptr->s1.argcount */
				/* has to be taken, so that pass-through stackslots get */
				/* renamed, too? */

			case DF_INVOKE:
				INSTRUCTION_GET_METHODDESC(iptr,md);
				i = md->paramcount;
				break;
			case DF_BUILTIN:
				bte = iptr->sx.s23.s3.bte;
				md = bte->md;
				i = md->paramcount;
				break;
			case DF_N_TO_1:
				i = iptr->s1.argcount;
				break;
			}
			if (i != UNUSED) {
				argp = iptr->sx.s23.s2.args;
				while (--i >= 0) {
					if (*argp == lt->v_index) {
#ifdef SSA_DEBUG_VERBOSE
						if (W != NULL) {
							if (compileverbose)
								printf(
									   "copy propagation loc: BB %3i I %3i: %3i -> %3i\n"
									   , s->b_index, s->iindex, new_v_index, *argp);
						}
#endif
						*argp = new_v_index;
							
					}
					argp++;
				}
			}

		} /* if (s->iindex < 0) */
	} /* for(s = lt->use; s != NULL; s = s->next) */
}

#ifdef SSA_DEBUG_VERBOSE
void ssa_print_lt(lsradata *ls) {
	int i;
	struct lifetime *lt;
	struct site *use;

	printf("SSA LT Def/Use\n");
	for(i = 0; i < ls->lifetimecount; i++) {
		lt = ls->lifetime + i;
		if (lt->type != UNUSED) {
			printf("VI %3i Type %3i Def: ",lt->v_index, lt->type);
			if (lt->def != NULL)
				printf("%3i,%3i Use: ",lt->def->b_index, lt->def->iindex);
			else
				printf("%3i,%3i Use: ",0,0);
			for(use = lt->use; use != NULL; use = use->next)
				printf("%3i,%3i ",use->b_index, use->iindex);
			printf("\n");
		}
	}
}

void ssa_show_variable(jitdata *jd, int index, varinfo *v, int stage)
{
	char type;
	char kind;

	switch (v->type) {
		case TYPE_INT: type = 'i'; break;
		case TYPE_LNG: type = 'l'; break;
		case TYPE_FLT: type = 'f'; break;
		case TYPE_DBL: type = 'd'; break;
		case TYPE_ADR: type = 'a'; break;
		case TYPE_RET: type = 'r'; break;
		default:       type = '?';
	}

	if (index < jd->localcount) {
		kind = 'L';
		if (v->flags & (PREALLOC | INOUT))
				printf("<INVALID FLAGS!>");
	}
	else {
		if (v->flags & PREALLOC) {
			kind = 'A';
			if (v->flags & INOUT)
				printf("<INVALID FLAGS!>");
		}
		else if (v->flags & INOUT)
			kind = 'I';
		else
			kind = 'T';
	}

	printf("%c%c%3d", kind, type, index);

	if (v->flags & SAVEDVAR)
		putchar('!');

	putchar(' ');
	fflush(stdout);
}
#endif

/****************************************************************************
Optimization
****************************************************************************/

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
