/* src/vm/optimizing/ssa3.c

   Copyright (C) 2008
   CACAOVM - Verein zu Foerderung der freien virtuellen Machine CACAO

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   SSA transformation PROTOTYPE based on:

   Moessenboeck, H., 
   Adding Static Single Assignment Form and a Graph Coloring Register 
   Allocator to the Java Hotspot Client Compiler, 2000 
   (http://www.ssw.uni-linz.ac.at/Research/Reports/Report15.html)

   TODO

   * Adapt for exception handling
   * Eliminiation of redundand PHI functions
   * Create PHI functions lazyly, when they are used for the first time
     (I suspect that currently PHIs are created that are never used).
*/

#include "vm/jit/jit.h"
#include "vm/global.h"
#include "mm/memory.h"
#include "mm/dumpmemory.h"
#include "toolbox/list.h"

#if 1
#define printf(...) do { if (getenv("VERB")) printf(__VA_ARGS__); } while (0)
#define show_method(...) do { if (getenv("VERB")) show_method(__VA_ARGS__); } while (0)
#define show_basicblock(...) do { if (getenv("VERB")) show_basicblock(__VA_ARGS__); } while (0)
#endif

/*
TODO
move set and get uses into jit.h
*/

#define ICMD_PHI 666

#define eqi(a, b) (((a) && (b) || (!(a) && !(b)))

#define nn(x) ((x) < 0 ? 0 : (x))

#define ass(dst, src) *(dst) = *(src)

static inline const char *instruction_name(const instruction *iptr) {
	if (iptr == NULL) {
		return "null";
	} else if (iptr->opc == ICMD_PHI) {
		return "phi";
	} else {
		return icmd_table[iptr->opc].name;
	}
}

static inline s4 instruction_line(const instruction *iptr) {
	if (iptr == NULL) {
		return -1;
	} else {
		return iptr->line;
	}
}

typedef struct phi_function {
	s4 dst;
	s4 *args;
	s4 local;
	instruction instr;
} phi_function;

typedef struct basicblock_info {
	bool visited;
	bool active;
	bool traversed;
	unsigned backward_branches;

	instruction **state_array;

	unsigned phi_count;
	unsigned phi_max;
	phi_function *phi_functions;
	unsigned complete_predecessors;
	bool cow;
} basicblock_info;

typedef struct ssa_info {
	jitdata *jd;

	varinfo locals[3000]; /* XXX hardcoded max */
	unsigned max_locals;
	unsigned locals_count;

	s4 s_buf[3];
} ssa_info;

static unsigned get_predecessor_count(basicblock *bb) {
	unsigned ret;
	basicblock **itpred;

	ret = nn(bb->predecessorcount);

	FOR_EACH_EXPREDECESSOR(bb, itpred) {
		ret += (*itpred)->exouts;
	}

	return ret;
}

static unsigned get_predecessor_index(basicblock *from, basicblock *to) {
	basicblock **itpred;
	unsigned j = 0;

	for (itpred = to->predecessors; itpred != to->predecessors + nn(to->predecessorcount); ++itpred) {
		if (*itpred == from) break;
		j++;
	}
	
	if (j == nn(to->predecessorcount)) {
		assert(j != nn(to->predecessorcount));
	}

	return j;
}

static void phi_set_argument(basicblock *bb, instruction *phi, unsigned j, instruction *value) {
	phi_function *pf = (phi_function *)(
		(char *)phi - OFFSET(phi_function, instr)
	);
	assert(j < phi->s1.argcount);
	if (value == NULL) {
		pf->args[j] = pf->local;
	} else {
		pf->args[j] = value->dst.varindex;
	}
	/*phi->sx.s23.s2.args[j] = value->dst.varindex;*/
	printf(" *** in bb%d setting phi arg %d for local %d to %s@%d (%d)\n", bb->nr, j, pf->local, instruction_name(value), instruction_line(value), value ? value->dst.varindex : -1);
}

static inline basicblock_info *bb_info(basicblock *bb) {
	return (basicblock_info *)(bb->vp);
}

static void mark_loops(basicblock *bb) {
	basicblock_info *bbi = bb_info(bb);
	basicblock **itsucc;

	if (! bbi->visited) {
		bbi->visited = true;
		bbi->active = true;
		FOR_EACH_SUCCESSOR(bb, itsucc) {
			mark_loops(*itsucc);
		}
		FOR_EACH_EXHANDLER(bb, itsucc) {
			mark_loops(*itsucc);
		}
		bbi->active = false;
	} else if (bbi->active) {
		bbi->backward_branches += 1;
	}
}

static instruction *create_phi(ssa_info *ssa, basicblock *bb, s4 local) {
	basicblock_info *bbi = bb_info(bb);
	phi_function *phi;
	s4 *itarg;
	unsigned arg_count = get_predecessor_count(bb);

	printf(" *** BB%d phi #%d for local %d\n", bb->nr, bbi->phi_count, local);

	phi = bbi->phi_functions + bbi->phi_count;
	assert(bbi->phi_count < bbi->phi_max);
	bbi->phi_count += 1;


	phi->local = local;
	phi->args = DMNEW(s4, arg_count);
	for (itarg = phi->args; itarg != phi->args + arg_count; ++itarg) {
		/* Invalidate */
		*itarg = 0x7fffffff;
	}

	assert(ssa->locals_count < ssa->max_locals);
	ssa->locals[ssa->locals_count] = ssa->jd->var[local];
	phi->dst = ssa->locals_count;
	ssa->locals_count += 1;

	phi->instr.opc = ICMD_PHI;
	phi->instr.dst.varindex = phi->dst;
	phi->instr.s1.argcount = arg_count;
	phi->instr.sx.s23.s2.args = NULL;

	return &(phi->instr);
}

static bool is_my_phi(basicblock *bb, instruction *iptr) {
	basicblock_info *bbi = bb_info(bb);
	if (iptr == NULL) {
		return false;
	}
	if (iptr->opc != ICMD_PHI) {
		return false;
	}
	if (	
		((char *)bbi->phi_functions <= (char *)iptr) &&
		((char *)iptr < (char *)(bbi->phi_functions + bbi->phi_count))
	) {
		return true;
	} else {
		return false;
	}
}

static void rename_def(ssa_info *ssa, basicblock *bptr, instruction *iptr) {
	basicblock_info *bbi = bb_info(bptr);
	assert(bbi->state_array);

	bbi->state_array[iptr->dst.varindex] = iptr;

	assert(ssa->locals_count < ssa->max_locals);

	ssa->locals[ssa->locals_count] = ssa->jd->var[iptr->dst.varindex];
	iptr->dst.varindex = ssa->locals_count;
	printf(" *** BB%d %s def %d => %d\n", bptr->nr, instruction_name(iptr), iptr->dst.varindex, ssa->locals_count);

	ssa->locals_count += 1;
}

static void rename_use(ssa_info *ssa, basicblock *bptr, instruction *iptr, s4 *use) {
	basicblock_info *bbi = bb_info(bptr);
	assert(bbi->state_array);

	if (bbi->state_array[*use] != NULL) {
		printf(" *** BB%d %s use %d => %d (%s) \n", bptr->nr, instruction_name(iptr), *use, bbi->state_array[*use]->dst.varindex, instruction_name(bbi->state_array[*use]) );
		*use = bbi->state_array[*use]->dst.varindex;
	} else {
		printf(" *** BB%d %s use keep\n", bptr->nr, instruction_name(iptr));
	}
}

static void get_uses(ssa_info *ssa, instruction *iptr, s4 **puses, unsigned *puses_count) {
	unsigned uses_count = 0;

	switch (icmd_table[iptr->opc].dataflow) {
		case DF_3_TO_0:
		case DF_3_TO_1:
			ssa->s_buf[2] = iptr->sx.s23.s3.varindex;
			uses_count += 1;

		case DF_2_TO_0:
		case DF_2_TO_1:
			ssa->s_buf[1] = iptr->sx.s23.s2.varindex;
			uses_count += 1;

		case DF_1_TO_0:
		case DF_1_TO_1:
		case DF_COPY:
		case DF_MOVE:
			ssa->s_buf[0] = iptr->s1.varindex;
			uses_count += 1;

			*puses_count = uses_count;
			*puses = ssa->s_buf;
			break;
	
		case DF_N_TO_1:
		case DF_INVOKE:
		case DF_BUILTIN:

			*puses = iptr->sx.s23.s2.args;
			*puses_count = iptr->s1.argcount;
			break;

		default:

			*puses_count = 0;
			break;
	}
}

static void set_uses(ssa_info *ssa, instruction *iptr, s4 *uses, unsigned uses_count) {
	if (uses == ssa->s_buf) {
		switch (uses_count) {
			case 3:
				iptr->sx.s23.s3.varindex = ssa->s_buf[2];
			case 2:
				iptr->sx.s23.s2.varindex = ssa->s_buf[1];
			case 1:
				iptr->s1.varindex = ssa->s_buf[0];
		}
	}
}

typedef void (*traverse_fun)(ssa_info *ssa, basicblock *bb);

static void traverse(ssa_info *ssa, basicblock *bb, traverse_fun fun) {
	basicblock_info *bbi = bb_info(bb);
	basicblock **itsucc, **itpred;
	basicblock_info *succi;
	unsigned complete_preds;
	unsigned j;

	/*if (bbi->traversed) {
		return;
	}*/

	/* Process block */

	fun(ssa, bb);

	/*bbi->traversed = true;*/

	/* Recurse */

	FOR_EACH_SUCCESSOR(bb, itsucc) {
		merge(ssa, bbi->state_array, *itsucc, get_predecessor_index(bb, *itsucc));
		succi = bb_info(*itsucc);
		succi->complete_predecessors += 1;
		if (succi->complete_predecessors == (/*nn((*itsucc)->predecessorcount)*/ get_predecessor_count(*itsucc) - succi->backward_branches)) {
			printf(" *** Traverse bb%d => bb%d\n", bb->nr, (*itsucc)->nr);
			traverse(ssa, *itsucc, fun);
		}
	}

	FOR_EACH_EXHANDLER(bb, itsucc) {
		succi = bb_info(*itsucc);
		succi->complete_predecessors += bb->exouts; /* XXX this might be 0 */
		if (succi->complete_predecessors == (/*nn((*itsucc)->predecessorcount)*/ get_predecessor_count(*itsucc) - succi->backward_branches)) {
			printf(" *** Traverse bb%d => bb%d\n", bb->nr, (*itsucc)->nr);
			traverse(ssa, *itsucc, fun);
		}
	}

}

void merge(ssa_info *ssa, instruction **state_array, basicblock *succ, unsigned j) {
	basicblock_info *succi = bb_info(succ);
	instruction *phi;
	unsigned i;
	unsigned a;

#define mprintf(fmt, ...) printf(" *** merge bb? => bb%d > " fmt, succ->nr, __VA_ARGS__)

	mprintf("(%d, %p)\n", j, succi->state_array);

	if (succi->state_array == NULL) {

		succi->state_array = DMNEW(instruction *, ssa->jd->localcount);

		mprintf("creating state_array %p\n", succi->state_array );


		if (succi->backward_branches > 0) {
			mprintf("bb%d is loop header, creating phis\n", succ->nr);
			for (i = 0; i < ssa->jd->localcount; ++i) {
				succi->state_array[i] = create_phi(ssa, succ, i);
			}
		} else {
			/*
			printf(" *** merge bb%d cow state array\n", succ->nr);
			succi->state_array = bbi->state_array;
			succi->cow = true;
			*/
			MCOPY(succi->state_array, state_array, instruction *, ssa->jd->localcount);
			return;
		}
	}

	for (i = 0; i < ssa->jd->localcount; ++i) {
		mprintf("local %d bb: %s@%d, succ: %s@%d\n", i, instruction_name(state_array[i]), instruction_line(state_array[i]), instruction_name(succi->state_array[i]), instruction_line(succi->state_array[i]));

		if (succi->traversed) {
			/* Back edge, all phis already created */
			/* Only fill in phi arguments */
			/* We have created phis for *ALL* locals, so there is random access */
			phi_set_argument(succ, &(succi->phi_functions[i].instr), j, state_array[i]);
		} else if (state_array[i] != succi->state_array[i]) {
			mprintf("merge bb%d need phi for local %d\n", succ->nr, i);
			/* Create new state array if needed */

			/*if (succi->cow) {
				printf(" *** merge bb%d cloning cow state array\n", succ->nr);
				state_array = DMNEW(instruction *, ssa->jd->localcount);
				MCOPY(state_array, succi->state_array, instruction *, ssa->jd->localcount);
				succi->state_array = state_array;
				succi->cow = false;
			}*/

			if (is_my_phi(succ, succi->state_array[i])) {
				/* State array is already phi function, just need to fill in argument */
				phi_set_argument(succ, succi->state_array[i], j, state_array[i]);
			} else {
				/* Create phi and initialize all arguments with current state array value
				 * (We might have already merged this value from within several blocks).
				 */
				phi = create_phi(ssa, succ, i);
				for (a = 0; a < get_predecessor_count(succ); ++a) {
					phi_set_argument(succ, phi, a, succi->state_array[i]);
				}
				phi_set_argument(succ, phi, j, state_array[i]);
				succi->state_array[i] = phi;
			}
		}
	}
#undef mprintf
}

static unsigned get_ex_predecessor_index(basicblock *from, unsigned pei, basicblock *to) {
	unsigned i, j;
	basicblock **itpred;
	instruction *iti;

	j = nn(to->predecessorcount);

	FOR_EACH_EXPREDECESSOR(to, itpred) {
		if ((*itpred)->nr == from->nr) {
			j += pei;
			return j;
		} else {
			j += (*itpred)->exouts;
		}
	}

	assert(0);
}

static void handle_pei(ssa_info *ssa, basicblock *bb, unsigned pei) {
	basicblock_info *bbi = bb_info(bb);
	basicblock **itsucc;

	FOR_EACH_EXHANDLER(bb, itsucc) {
		merge(ssa, bbi->state_array, *itsucc, get_ex_predecessor_index(bb, pei, *itsucc));
	}
}

static void traverse_fun_impl(ssa_info *ssa, basicblock *bb) {
	basicblock_info *bbi = bb_info(bb), *predi;
	basicblock **itpred, *pred;
	unsigned i;
	bool need_phi;
	instruction *iptr;
	unsigned uses_count;
	s4 *uses, *ituse;
	unsigned pei = 0;

	assert(! bbi->traversed);
	bbi->traversed = true;

	/*
	if (bbi->state_array) {
		return;
	}
	*/

	/* bb 0 */

	if (bb->predecessorcount == 0) {
		assert(! bbi->state_array);
		bbi->state_array = DMNEW(instruction *, ssa->jd->localcount);
		MZERO(bbi->state_array, instruction *, ssa->jd->localcount);
		goto process_instructions;
	}


	/*
	if ((bb->predecessorcount == 1) && (bb->predecessors[0]->successorcount == 1)) {
		bbi->state_array = bb_info(bb->predecessors[0])->state_array;
		assert(bbi->state_array);
		goto process_instructions;
	}
	*/

	/*
	bbi->state_array = DMNEW(instruction *, ssa->jd->localcount);
	MZERO(bbi->state_array, instruction *, ssa->jd->localcount);
	*/

	/*
	if (bbi->backward_branches > 0) {
		for (i = 0; i < ssa->jd->localcount; ++i) {
			bbi->state_array[i] = create_phi(ssa, bb, i);
		}
	} else {
		for (i = 0; i < ssa->jd->localcount; ++i) {
			need_phi = false;
			assert(bb_info(bb->predecessors[0])->state_array);

			FOR_EACH_PREDECESSOR(bb, itpred) {
				assert(bb_info(*itpred)->state_array);
				if (bb_info(bb->predecessors[0])->state_array[i] != bb_info(*itpred)->state_array[i]) {
					need_phi = true;
					break;
				}
			}

			if (need_phi) {
				bbi->state_array[i] = create_phi(ssa, bb, i);
			} else {
				bbi->state_array[i] = bb_info(bb->predecessors[0])->state_array[i];
			}

		}
	}
	*/

process_instructions:

	assert(bbi->state_array);

	FOR_EACH_INSTRUCTION(bb, iptr) {

		if (icmd_table[iptr->opc].flags & ICMDTABLE_PEI) {
			handle_pei(ssa, bb, pei++);
		}

		get_uses(ssa, iptr, &uses, &uses_count);

		for (ituse = uses; ituse != uses + uses_count; ++ituse) {
			if (var_is_local(ssa->jd, *ituse)) {
				rename_use(ssa, bb, iptr, ituse);
			} else {
				*ituse += 0xFFFF;
			}
		}

		set_uses(ssa, iptr, uses, uses_count);

		if (instruction_has_dst(iptr)) {
			if (var_is_local(ssa->jd, iptr->dst.varindex)) {
				rename_def(ssa, bb, iptr);
			} else {
				iptr->dst.varindex += 0xFFFF;
			}
		}
	}
}

static void ssa_rename_others(ssa_info *ssa) {

	basicblock *bb;
	instruction *iptr;
	s4 *itinout, *uses, *ituse;
	unsigned uses_count;
	unsigned off = ssa->locals_count - ssa->jd->localcount;

	FOR_EACH_BASICBLOCK(ssa->jd, bb) {

		if (! basicblock_reached(bb)) continue;

		for (itinout = bb->invars; itinout != bb->invars + bb->indepth; ++itinout) {
			*itinout += off;
		}

		for (itinout = bb->outvars; itinout != bb->outvars + bb->outdepth; ++itinout) {
			*itinout += off;
		}

		FOR_EACH_INSTRUCTION(bb, iptr) {
			if (instruction_has_dst(iptr)) {
				if (iptr->dst.varindex >= 0xFFFF) {
					iptr->dst.varindex += off;
					iptr->dst.varindex -= 0xFFFF;
				}
			}
			get_uses(ssa, iptr, &uses, &uses_count);
			for (ituse = uses; ituse != uses + uses_count; ++ituse) {
				if (*ituse >= 0xFFFF) {
					*ituse += off;
					*ituse -= 0xFFFF;
				}
			}
			set_uses(ssa, iptr, uses, uses_count);
		}
	}
}

static void fill_in_phi_args(ssa_info *ssa) {
	basicblock *bb;
	basicblock_info *bbi;
	phi_function *itphi;
	unsigned j;
	basicblock **itpred;
	basicblock_info *predi;

	FOR_EACH_BASICBLOCK(ssa->jd, bb) {
		if (!(bb->flags >= BBREACHED)) continue;
		bbi = bb_info(bb);
		j = 0;
		FOR_EACH_PREDECESSOR(bb, itpred) {
			predi = bb_info(*itpred);
			for (itphi = bbi->phi_functions; itphi != bbi->phi_functions + bbi->phi_count; ++itphi) {
				if (predi->state_array[itphi->local]) {
					itphi->args[j] = predi->state_array[itphi->local]->dst.varindex;
				} else {
					itphi->args[j] = itphi->local;
				}
			}
			++j;
		}
	}
}

static void ssa_export(ssa_info *ssa) {
	unsigned i, j;
	jitdata *jd = ssa->jd;
	varinfo *vars, *it;
	s4 vartop, varindex;

	vartop = ssa->locals_count + jd->vartop - jd->localcount;
	vars = DMNEW(varinfo, vartop);

	MCOPY(vars, ssa->locals, varinfo, ssa->locals_count);
	MCOPY(vars + ssa->locals_count, jd->var + jd->localcount, varinfo, jd->vartop - jd->localcount);

	jd->var = vars;
	jd->vartop = jd->varcount = vartop;

	jd->local_map = DMREALLOC(jd->local_map, s4, 5 * jd->maxlocals, 5 * (jd->maxlocals + ssa->locals_count - jd->localcount));

	for (i = 0; i < ssa->locals_count - jd->localcount; ++i) {
		for (j = 0; j < 5; ++j) {
			varindex = jd->localcount + i;
			if (jd->var[varindex].type != j) {
				varindex = UNUSED;
			}
			jd->local_map[((jd->maxlocals + i) * 5) + j] = varindex;
		}
	}

	jd->maxlocals += (ssa->locals_count - jd->localcount);
	jd->localcount = ssa->locals_count;
}

static basicblock *create_block_intern(ssa_info *ssa, basicblock *from, basicblock *to, unsigned j) {
	basicblock *mid;
	basicblock_info *toi;
	instruction *iptr;
	phi_function *itph;
	
	mid = DNEW(basicblock);
	MZERO(mid, basicblock, 1);

	toi = bb_info(to);
	assert(toi);

	mid->nr = ssa->jd->basicblockcount;
	ssa->jd->basicblockcount += 1;
	mid->mpc = -1;
	mid->type = 666;
	mid->icount = toi->phi_count + 1;
	iptr = mid->iinstr = DMNEW(instruction, mid->icount);
	MZERO(mid->iinstr, instruction, mid->icount);

	for (itph = toi->phi_functions; itph != toi->phi_functions + toi->phi_count; ++itph) {
		iptr->opc = ICMD_COPY;
		iptr->dst.varindex = itph->dst;
		iptr->s1.varindex =  itph->args[j];
		assert(j < itph->instr.s1.argcount);
		assert(itph->dst < ssa->locals_count);
		assert(itph->args[j] < ssa->locals_count);
		iptr++;
	}

	iptr->opc = ICMD_GOTO;
	iptr->dst.block = to;

	return mid;
}

static basicblock *create_block(ssa_info *ssa, basicblock *from, basicblock *to) {
	basicblock *ret;
	ret = create_block_intern(ssa, from, to, get_predecessor_index(from, to));

	while (from->next) {
		from = from->next;
	}

	from->next = ret;

	return ret;
}

static void crate_fallthrough(ssa_info *ssa, basicblock *bptr) {
	unsigned j;
	basicblock_info *toi;
	instruction *iptr;
	phi_function *itph;

	if (bptr->next == NULL) return;

	j = get_predecessor_index(bptr, bptr->next);

	toi = bb_info(bptr->next);
	assert(toi);

	bptr->iinstr = DMREALLOC(bptr->iinstr, instruction, bptr->icount, bptr->icount + toi->phi_count);
	iptr = bptr->iinstr + bptr->icount;
	bptr->icount += toi->phi_count;

	for (itph = toi->phi_functions; itph != toi->phi_functions + toi->phi_count; ++itph) {
		iptr->opc = ICMD_COPY;
		iptr->dst.varindex = itph->dst;
		iptr->s1.varindex =  itph->args[j];
		assert(itph->dst < ssa->locals_count);
		assert(itph->args[j] < ssa->locals_count);
		iptr++;
	}

}

static void ssa_create_phi_moves(ssa_info *ssa) {
	basicblock *bptr;
	instruction *iptr;

	s4 i, l;
	branch_target_t *table;
	lookup_target_t *lookup;
	bool gt;

	for (bptr = ssa->jd->basicblocks; bptr; bptr = bptr->next) {
		if (bptr->type == 666) {
			bptr->type = BBTYPE_STD;
			continue;
		}
		if (! bptr->vp) continue;
		if (! (bptr->flags >= BBREACHED)) continue;
		gt = false;
		for (iptr = bptr->iinstr; iptr != bptr->iinstr + bptr->icount; ++iptr) {
			switch (icmd_table[iptr->opc].controlflow) {
				case CF_IF:
				case CF_RET:
				case CF_GOTO:
					iptr->dst.block = create_block(ssa, bptr, iptr->dst.block);	
					break;
				case CF_TABLE:
					table = iptr->dst.table;
					l = iptr->sx.s23.s2.tablelow;
					i = iptr->sx.s23.s3.tablehigh;
					i = i - l + 1;
					i += 1; /* default */
					while (--i >= 0) {
						table->block = create_block(ssa, bptr, table->block);
						++table;
					}
					break;
				case CF_LOOKUP:
					lookup = iptr->dst.lookup;
					i = iptr->sx.s23.s2.lookupcount;
					while (--i >= 0) {
						lookup->target.block = create_block(ssa, bptr, lookup->target.block);
						lookup++;
					}
					iptr->sx.s23.s3.lookupdefault.block = create_block(ssa, bptr, iptr->sx.s23.s3.lookupdefault.block);
					break;
				case CF_JSR:
					iptr->sx.s23.s3.jsrtarget.block = create_block(ssa, bptr, iptr->sx.s23.s3.jsrtarget.block);
					break;
			}
			if ((iptr->opc == ICMD_GOTO) || (iptr->opc == ICMD_JSR) || (iptr->opc == ICMD_RET) || icmd_table[iptr->opc].controlflow == CF_END)
				gt = true;
			else if (iptr->opc != ICMD_NOP)
				gt = false;
		}
		if (! bptr->next) continue;
		if (! (bptr->next->flags >= BBREACHED)) continue;
		if (bptr->next->type == 666) continue;
		if (!gt) crate_fallthrough(ssa, bptr);
	}
}

static basicblock *split_basicblock_at(ssa_info *ssa, basicblock *bptr, instruction *iptr) {

	basicblock *newblock;
	basicblock *tosplit;
	basicblock **pnext;
	unsigned icount;
	basicblock *it;
	unsigned pos;

	unsigned origidx = iptr - bptr->iinstr;

	printf(" *** split basicblock bb%d at %s/%d/%d\n", bptr->nr, instruction_name(iptr), instruction_line(iptr), origidx);
	assert(origidx < bptr->icount);


	if (! bptr->subbasicblocks) {
		bptr->subbasicblocks = DNEW(basicblock);
		ass(bptr->subbasicblocks, bptr);
		bptr->subbasicblocks->next = NULL;
	}

	tosplit = bptr->subbasicblocks;
	pos = 0;

	while (tosplit->next && (origidx >= (pos + tosplit->icount))) {
		assert(bptr->nr == tosplit->nr);
		pos += tosplit->icount;
		tosplit = tosplit->next;
	}

	assert(bptr->nr == tosplit->nr);
	
	icount = iptr - tosplit->iinstr + 1;
	assert(icount <= tosplit->icount);

	if (icount < tosplit->icount) {
		newblock = DNEW(basicblock);
		ass(newblock, tosplit);
	
		tosplit->next = newblock;
		tosplit->icount = icount;

		newblock->icount -= icount;
		newblock->iinstr += icount;
		newblock->next = NULL;

		assert(tosplit->nr == bptr->nr);
		assert(newblock->nr == bptr->nr);
		assert(newblock->next == NULL);
	} else {
		printf("xx split\n");
	}

	return tosplit;
}

static exception_entry *create_exception_handler(ssa_info *ssa, basicblock *from, unsigned pei, basicblock *to, classref_or_classinfo catchtype) {
	basicblock *it;
	exception_entry *ee = DNEW(exception_entry);
	basicblock *exh = create_block_intern(ssa, from, to, get_ex_predecessor_index(from, pei, to));
	exh->type = BBTYPE_EXH;
	to->type = BBTYPE_STD;

	exh->indepth = exh->outdepth = 1;
	exh->invars = exh->outvars = DNEW(s4);
	/* assigned in caller */

	ee->start = from;
	ee->end = from->next;
	ee->handler = exh;
	ee->catchtype = catchtype;
	ee->next = NULL;
	ee->down = NULL;

	for (it = ssa->jd->basicblocks; it->next; it = it->next);

	it->next = exh;

	return ee;
}

static void ssa_create_ex_phi_moves(ssa_info *ssa) {
	basicblock *bptr;
	instruction *iptr;
	basicblock_info *bbi;
	exception_entry *ite;
	exception_entry *firstee, *lastee, *ee;
	basicblock *ittry, *try;
	classref_or_classinfo catchtype;
	unsigned pei;
	unsigned exhandler_count = 0;
	varinfo *v;

	FOR_EACH_BASICBLOCK(ssa->jd, bptr) {
		if (! bptr->vp) continue;
		if (! (bptr->flags >= BBREACHED)) continue;
		if (bptr->expredecessorcount == 0) continue;
		bbi = bb_info(bptr);
		if (bbi->phi_count == 0) continue;

		for (ite = ssa->jd->exceptiontable; ite; ite = ite->down) {
			/* Traverse all exhandlers */
			if (bptr == ite->handler) {
				printf(" *** mess with handler bb%d\n", bptr->nr);
				catchtype = ite->catchtype;
				firstee = lastee = NULL;
				/* If bptr is handler, remove exhandler */
				/* Traverse all guarded blocks */
				for (ittry = ite->start; ittry != ite->end; ittry = ittry->next) {
					pei = 0;
					FOR_EACH_INSTRUCTION(ittry, iptr) {
						if (icmd_table[iptr->opc].flags & ICMDTABLE_PEI) {
							try = split_basicblock_at(ssa, ittry, iptr);
							ee = create_exception_handler(ssa, try, pei, bptr, catchtype);
							ee->handler->invars[0] = ssa->jd->vartop + exhandler_count;
							exhandler_count += 1;
							ssa->jd->exceptiontablelength += 1;
							if (firstee == NULL) {
								firstee = lastee = ee;
							} else {
								lastee->next = ee;
								lastee->down = ee;
								lastee = ee;
							}
							pei += 1;
						}
					}
				}
				if (firstee) {
					lastee->next = ite->next;
					lastee->down = ite->down;
					*ite = *firstee;
					ite = lastee;
				}
			}
		}
	}
	
	/* Allocate interface vars */

	ssa->jd->var = DMREALLOC(ssa->jd->var, varinfo, ssa->jd->vartop, ssa->jd->vartop + exhandler_count);
	for (v = ssa->jd->var + ssa->jd->vartop; v != ssa->jd->var + ssa->jd->vartop + exhandler_count; ++v) {
		v->type = TYPE_ADR;
		v->flags = INOUT;
	}
	ssa->jd->vartop += exhandler_count;
	ssa->jd->varcount += exhandler_count;
}

void yssa(jitdata *jd) {
	basicblock *it;
	basicblock_info *iti;
	ssa_info *ssa;

	if (jd->localcount == 0) return;

	printf("=============== [ before %s ] =========================\n", jd->m->name->text);
	show_method(jd, 3);
	printf("=============== [ /before ] =========================\n");

	ssa = DNEW(ssa_info);
	ssa->jd = jd;
	ssa->max_locals = sizeof(ssa->locals) / sizeof(ssa->locals[0]);
	MCOPY(ssa->locals, jd->var, varinfo, jd->localcount);
	ssa->locals_count = jd->localcount;

	FOR_EACH_BASICBLOCK(jd, it) {
		if (basicblock_reached(it)) {
			iti = DNEW(basicblock_info);
			MZERO(iti, basicblock_info, 1);
			if (jd->localcount > 0) {
				iti->phi_functions = DMNEW(phi_function, jd->localcount);
				iti->phi_max = jd->localcount;
			}
			it->vp = iti;
		} else {
			it->vp = NULL;
		}
	}

	mark_loops(jd->basicblocks);

	traverse(ssa, jd->basicblocks, traverse_fun_impl);

	ssa_rename_others(ssa);

	/*fill_in_phi_args(ssa);*/

	ssa_export(ssa);

	ssa_create_phi_moves(ssa);
	ssa_create_ex_phi_moves(ssa);

	printf("=============== [ after ] =========================\n");
	show_method(jd, 3);
	printf("=============== [ /after ] =========================\n");
}

void eliminate_subbasicblocks(jitdata *jd) {
	basicblock *bptr, *next;

	FOR_EACH_BASICBLOCK(jd, bptr) {
		if (bptr->subbasicblocks) {
			next = bptr->next;
			*bptr = *(bptr->subbasicblocks);
			bptr->subbasicblocks = NULL;
			while (bptr->next) {
				bptr = bptr->next;
			}
			bptr->next = next;
		}
	}
	printf("=============== [ elim ] =========================\n");
	show_method(jd, 3);
	printf("=============== [ /elim ] =========================\n");
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
 * vim:noexpandtab:sw=4:ts=4:
 */
