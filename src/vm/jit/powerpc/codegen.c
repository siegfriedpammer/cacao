/* vm/jit/powerpc/codegen.c - machine code generator for 32-bit powerpc

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
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

   Authors: Andreas Krall
            Stefan Ring

   Changes: Christian Thalinger

   $Id: codegen.c 2014 2005-03-08 06:27:57Z christian $

*/


#include <stdio.h>
#include <signal.h>

#include "config.h"
#include "cacao/cacao.h"
#include "native/native.h"
#include "vm/builtin.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/tables.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/jit.h"
#include "vm/jit/parse.h"
#include "vm/jit/reg.h"
#include "vm/jit/powerpc/arch.h"
#include "vm/jit/powerpc/codegen.h"
#include "vm/jit/powerpc/types.h"


/* register descripton - array ************************************************/

/* #define REG_RES   0         reserved register for OS or code generator     */
/* #define REG_RET   1         return value register                          */
/* #define REG_EXC   2         exception value register (only old jit)        */
/* #define REG_SAV   3         (callee) saved register                        */
/* #define REG_TMP   4         scratch temporary register (caller saved)      */
/* #define REG_ARG   5         argument register (caller saved)               */

/* #define REG_END   -1        last entry in tables */
 
static int nregdescint[] = {
	REG_RES, REG_RES, REG_TMP, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, 
	REG_ARG, REG_ARG, REG_ARG, REG_RES, REG_RES, REG_RES, REG_SAV, REG_SAV, 
	REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP,
	REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV,
	REG_END };

/* for use of reserved registers, see comment above */
	
static int nregdescfloat[] = {
	REG_RES, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG,
	REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_SAV, REG_SAV, 
	REG_RES, REG_RES, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP,
	REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV,
	REG_END };


/* Include independent code generation stuff -- include after register        */
/* descriptions to avoid extern definitions.                                  */

#include "vm/jit/codegen.inc"
#include "vm/jit/reg.inc"
#ifdef LSRA
#include "vm/jit/lsra.inc"
#endif


void asm_cacheflush(void *, long);

/* #include <architecture/ppc/cframe.h> */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void thread_restartcriticalsection(void *u)
{
	/* XXX set pc to restart address */
}
#endif

#if defined(__DARWIN__)
#include <mach/message.h>

int cacao_catch_Handler(mach_port_t thread)
{
#if defined(USE_THREADS)
	unsigned int *regs;
	unsigned int crashpc;
	s4 instr, reg;
	java_objectheader *xptr;

	/* Mach stuff */
	thread_state_flavor_t flavor = PPC_THREAD_STATE;
	mach_msg_type_number_t thread_state_count = PPC_THREAD_STATE_COUNT;
	ppc_thread_state_t thread_state;
	kern_return_t r;
	
	if (checknull)
		return 0;

	r = thread_get_state(thread, flavor,
		(natural_t*)&thread_state, &thread_state_count);
	if (r != KERN_SUCCESS)
		panic("thread_get_state failed");

	regs = &thread_state.r0;
	crashpc = thread_state.srr0;

	instr = *(s4*) crashpc;
	reg = (instr >> 16) & 31;

	if (!regs[reg]) {
/*      This is now handled in asmpart because it needs to run in the throwing
 *      thread */
/*		xptr = new_nullpointerexception(); */

		regs[REG_ITMP2_XPC] = crashpc;
/*		regs[REG_ITMP1_XPTR] = (u4) xptr; */
		thread_state.srr0 = (u4) asm_handle_nullptr_exception;

		r = thread_set_state(thread, flavor,
			(natural_t*)&thread_state, thread_state_count);
		if (r != KERN_SUCCESS)
			panic("thread_set_state failed");

		return 1;
	}

	throw_cacao_exception_exit(string_java_lang_InternalError, "Segmentation fault at %p", regs[reg]);
#endif

	return 0;
}
#endif /* __DARWIN__ */


void init_exceptions(void)
{
	GC_dirty_init(1);

#if !defined(__DARWIN__)
	nregdescint[2] = REG_RES;
#endif
}

void adjust_argvars(stackptr s, int d, int *fa, int *ia)
{
	if (!d) {
		*fa = 0; *ia = 0;
		return;
	}
	adjust_argvars(s->prev, d-1, fa, ia);
	if (s->varkind == ARGVAR)
		s->varnum = (IS_FLT_DBL_TYPE(s->type)) ? *fa : *ia;
	*fa += (IS_FLT_DBL_TYPE(s->type) != 0);
	*ia += (IS_2_WORD_TYPE(s->type)) ? 2 : 1;
}


#define intmaxf(a,b) (((a)<(b)) ? (b) : (a))

void preregpass(methodinfo *m, registerdata *rd)
{
	int paramsize;
	stackptr    src;
	basicblock  *bptr;
	instruction *iptr;
	int s3, len;

	rd->ifmemuse = 0;

	for (bptr = m->basicblocks; bptr != NULL; bptr = bptr->next) {
		len = bptr->icount;
		for (iptr = bptr->iinstr, src = bptr->instack;
		    len > 0;
		    src = iptr->dst, len--, iptr++)
		{
			if (bptr->flags < BBREACHED)
				continue;
			switch (iptr->opc) {
				case ICMD_BUILTIN1:
					s3 = 1;
					goto countparams;
				case ICMD_BUILTIN2:
					s3 = 2;
					goto countparams;
				case ICMD_BUILTIN3:
					s3 = 3;
					goto countparams;

				case ICMD_INVOKEVIRTUAL:
				case ICMD_INVOKESPECIAL:
				case ICMD_INVOKESTATIC:
				case ICMD_INVOKEINTERFACE:
					s3 = iptr->op1;
countparams:
					{
						int ia, fa;
						adjust_argvars(src, s3, &fa, &ia);
					}
					paramsize = 0;
					for (; --s3 >= 0; src = src->prev) {
						paramsize += IS_2_WORD_TYPE(src->type) ? 2 : 1;
					}
					rd->ifmemuse = intmaxf(rd->ifmemuse, paramsize);
					break;

				case ICMD_MULTIANEWARRAY:
					s3 = iptr->op1;
					paramsize = rd->intreg_argnum + s3;
					rd->ifmemuse = intmaxf(rd->ifmemuse, paramsize);
					break;
			}
		}
	}

	rd->ifmemuse += 6;
	rd->maxmemuse = rd->ifmemuse;
}


/* function gen_mcode **********************************************************

	generates machine code

*******************************************************************************/

void codegen(methodinfo *m, codegendata *cd, registerdata *rd)
{
	s4 len, s1, s2, s3, d;
	s4 a;
	s4 parentargs_base;
	s4          *mcodeptr;
	stackptr    src;
	varinfo     *var;
	basicblock  *bptr;
	instruction *iptr;
	exceptiontable *ex;

	{
	s4 i, p, pa, t, l;
	s4 savedregs_num;

	savedregs_num = 0;

	/* space to save used callee saved registers */

	savedregs_num += (rd->savintregcnt - rd->maxsavintreguse);
	savedregs_num += 2 * (rd->savfltregcnt - rd->maxsavfltreguse);

	parentargs_base = rd->maxmemuse + savedregs_num;

#ifdef USE_THREADS                 /* space to save argument of monitor_enter */

	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		parentargs_base++;

#endif

	/* create method header */

	parentargs_base = (parentargs_base + 3) & ~3;

#if POINTERSIZE == 4
	(void) dseg_addaddress(cd, m);                          /* Filler         */
#endif
	(void) dseg_addaddress(cd, m);                          /* MethodPointer  */
	(void) dseg_adds4(cd, parentargs_base * 4);             /* FrameSize      */

#ifdef USE_THREADS

	/* IsSync contains the offset relative to the stack pointer for the
	   argument of monitor_exit used in the exception handler. Since the
	   offset could be zero and give a wrong meaning of the flag it is
	   offset by one.
	*/

	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		(void) dseg_adds4(cd, (rd->maxmemuse + 1) * 4);     /* IsSync         */
	else

#endif

		(void) dseg_adds4(cd, 0);                           /* IsSync         */
	                                       
	(void) dseg_adds4(cd, m->isleafmethod);                 /* IsLeaf         */
	(void) dseg_adds4(cd, rd->savintregcnt - rd->maxsavintreguse); /* IntSave */
	(void) dseg_adds4(cd, rd->savfltregcnt - rd->maxsavfltreguse); /* FltSave */
	(void) dseg_adds4(cd, cd->exceptiontablelength);        /* ExTableSize    */

	/* create exception table */

	for (ex = cd->exceptiontable; ex != NULL; ex = ex->down) {
		dseg_addtarget(cd, ex->start);
   		dseg_addtarget(cd, ex->end);
		dseg_addtarget(cd, ex->handler);
		(void) dseg_addaddress(cd, ex->catchtype);
	}
	
	/* initialize mcode variables */
	
	mcodeptr = (s4 *) cd->mcodebase;
	cd->mcodeend = (s4 *) (cd->mcodebase + cd->mcodesize);
	MCODECHECK(128 + m->paramcount);

	/* create stack frame (if necessary) */

	if (!m->isleafmethod) {
		M_MFLR(REG_ITMP3);
		M_AST(REG_ITMP3, REG_SP, 8);
	}

	if (parentargs_base) {
		M_STWU(REG_SP, REG_SP, -parentargs_base * 4);
	}

	/* save return address and used callee saved registers */

	p = parentargs_base;
	for (i = rd->savintregcnt - 1; i >= rd->maxsavintreguse; i--) {
		p--; M_IST(rd->savintregs[i], REG_SP, 4 * p);
	}
	for (i = rd->savfltregcnt - 1; i >= rd->maxsavfltreguse; i--) {
		p-=2; M_DST(rd->savfltregs[i], REG_SP, 4 * p);
	}

	/* save monitorenter argument */

#if defined(USE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		if (m->flags & ACC_STATIC) {
			p = dseg_addaddress(cd, m->class);
			M_ALD(REG_ITMP1, REG_PV, p);
			M_AST(REG_ITMP1, REG_SP, 4 * rd->maxmemuse);

		} else {
			M_AST(rd->argintregs[0], REG_SP, 4 * rd->maxmemuse);
		}
	}			
#endif

	/* copy argument registers to stack and call trace function with pointer
	   to arguments on stack.
	*/

	if (runverbose) {
		s4 longargs = 0;
		s4 fltargs = 0;
		s4 dblargs = 0;

		M_MFLR(REG_ITMP3);
		/* XXX must be a multiple of 16 */
		M_LDA(REG_SP, REG_SP, -(24 + (INT_ARG_CNT + FLT_ARG_CNT + 1) * 8));

		M_IST(REG_ITMP3, REG_SP, 24 + (INT_ARG_CNT + FLT_ARG_CNT) * 8);

		M_CLR(REG_ITMP1);    /* clear help register */

		/* save all arguments into the reserved stack space */
		for (p = 0; p < m->paramcount && p < TRACE_ARGS_NUM; p++) {
			t = m->paramtypes[p];

			if (IS_INT_LNG_TYPE(t)) {
				/* overlapping u8's are on the stack */
				if ((p + longargs + dblargs) < (INT_ARG_CNT - IS_2_WORD_TYPE(t))) {
					s1 = rd->argintregs[p + longargs + dblargs];

					if (!IS_2_WORD_TYPE(t)) {
						M_IST(REG_ITMP1, REG_SP, 24 + p * 8);
						M_IST(s1, REG_SP, 24 + p * 8 + 4);

					} else {
						M_IST(s1, REG_SP, 24 + p  * 8);
						M_IST(rd->secondregs[s1], REG_SP, 24 + p * 8 + 4);
						longargs++;
					}

				} else {
                    a = dseg_adds4(cd, 0xdeadbeef);
                    M_ILD(REG_ITMP1, REG_PV, a);
                    M_IST(REG_ITMP1, REG_SP, 24 + p * 8);
                    M_IST(REG_ITMP1, REG_SP, 24 + p * 8 + 4);
				}

			} else {
				if ((fltargs + dblargs) < FLT_ARG_CNT) {
					s1 = rd->argfltregs[fltargs + dblargs];

					if (!IS_2_WORD_TYPE(t)) {
						M_IST(REG_ITMP1, REG_SP, 24 + p * 8);
						M_FST(s1, REG_SP, 24 + p * 8 + 4);
						fltargs++;
						
					} else {
						M_DST(s1, REG_SP, 24 + p * 8);
						dblargs++;
					}

				} else {
					/* this should not happen */
				}
			}
		}

		/* TODO: save remaining integer and flaot argument registers */

		/* load first 4 arguments into integer argument registers */
		for (p = 0; p < 8; p++) {
			d = rd->argintregs[p];
			M_ILD(d, REG_SP, 24 + p * 4);
		}

		p = dseg_addaddress(cd, m);
		M_ALD(REG_ITMP1, REG_PV, p);
		M_AST(REG_ITMP1, REG_SP, 11 * 8);    /* 24 (linkage area) + 32 (4 * s8 parameter area regs) + 32 (4 * s8 parameter area stack) = 88 */
		p = dseg_addaddress(cd, (void *) builtin_trace_args);
		M_ALD(REG_ITMP2, REG_PV, p);
		M_MTCTR(REG_ITMP2);
		M_JSR;

		longargs = 0;
		fltargs = 0;
		dblargs = 0;

        /* restore arguments into the reserved stack space */
        for (p = 0; p < m->paramcount && p < TRACE_ARGS_NUM; p++) {
            t = m->paramtypes[p];

            if (IS_INT_LNG_TYPE(t)) {
                if ((p + longargs + dblargs) < INT_ARG_CNT) {
                    s1 = rd->argintregs[p + longargs + dblargs];

					if (!IS_2_WORD_TYPE(t)) {
						M_ILD(s1, REG_SP, 24 + p * 8 + 4);

					} else {
						M_ILD(s1, REG_SP, 24 + p  * 8);
						M_ILD(rd->secondregs[s1], REG_SP, 24 + p * 8 + 4);
						longargs++;
					}
				}

            } else {
                if ((fltargs + dblargs) < FLT_ARG_CNT) {
                    s1 = rd->argfltregs[fltargs + dblargs];

					if (!IS_2_WORD_TYPE(t)) {
						M_FLD(s1, REG_SP, 24 + p * 8 + 4);
						fltargs++;

					} else {
						M_DLD(s1, REG_SP, 24 + p * 8);
						dblargs++;
					}
				}
            }
        }

		M_ILD(REG_ITMP3, REG_SP, 24 + (INT_ARG_CNT + FLT_ARG_CNT) * 8);

		M_LDA(REG_SP, REG_SP, 24 + (INT_ARG_CNT + FLT_ARG_CNT + 1) * 8);
		M_MTLR(REG_ITMP3);
	}

	/* take arguments out of register or stack frame */

	{
		int narg=0, niarg=0;
		int arg, iarg;

 	for (p = 0, l = 0; p < m->paramcount; p++) {
		arg = narg; iarg = niarg;
 		t = m->paramtypes[p];
 		var = &(rd->locals[l][t]);
 		l++, niarg++;
 		if (IS_2_WORD_TYPE(t))    /* increment local counter for 2 word types */
 			l++, niarg++;
 		if (var->type < 0)
 			continue;
		if (IS_INT_LNG_TYPE(t)) {                    /* integer args          */
 			if (iarg < INT_ARG_CNT -
					(IS_2_WORD_TYPE(t)!=0)) {        /* register arguments    */
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
 					M_TINTMOVE(t, rd->argintregs[iarg], var->regoff);

				} else {                             /* reg arg -> spilled    */
					M_IST(rd->argintregs[iarg], REG_SP, 4 * var->regoff);
					if (IS_2_WORD_TYPE(t))
						M_IST(rd->secondregs[rd->argintregs[iarg]], REG_SP, 4 * var->regoff + 4);
				}

			} else {                                 /* stack arguments       */
 				pa = iarg + 6;
 				if (!(var->flags & INMEMORY)) {      /* stack arg -> register */ 
					M_ILD(var->regoff, REG_SP, 4 * (parentargs_base + pa));
					if (IS_2_WORD_TYPE(t))
						M_ILD(rd->secondregs[var->regoff], REG_SP, 4 * (parentargs_base + pa) + 4);

				} else {                              /* stack arg -> spilled  */
 					M_ILD(REG_ITMP1, REG_SP, 4 * (parentargs_base + pa));
 					M_IST(REG_ITMP1, REG_SP, 4 * var->regoff);
					if (IS_2_WORD_TYPE(t)) {
						M_ILD(REG_ITMP1, REG_SP, 4 * (parentargs_base + pa) + 4);
						M_IST(REG_ITMP1, REG_SP, 4 * var->regoff + 4);
					}
				}
			}

		} else {                                     /* floating args         */   
			++narg;
 			if (arg < FLT_ARG_CNT) {                 /* register arguments    */
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
 					M_FLTMOVE(rd->argfltregs[arg], var->regoff);

 				} else {			                 /* reg arg -> spilled    */
					M_DST(rd->argfltregs[arg], REG_SP, 4 * var->regoff);
 				}

 			} else {                                 /* stack arguments       */
 				pa = iarg + 6;
 				if (!(var->flags & INMEMORY)) {      /* stack-arg -> register */
					if (IS_2_WORD_TYPE(t)) {
						M_DLD(var->regoff, REG_SP, 4 * (parentargs_base + pa));

					} else {
						M_FLD(var->regoff, REG_SP, 4 * (parentargs_base + pa));
					}

 				} else {                             /* stack-arg -> spilled  */
					if (IS_2_WORD_TYPE(t)) {
						M_DLD(REG_FTMP1, REG_SP, 4 * (parentargs_base + pa));

					} else {
						M_FLD(REG_FTMP1, REG_SP, 4 * (parentargs_base + pa));
					}
					M_DST(REG_FTMP1, REG_SP, 4 * var->regoff);
				}
			}
		}
	} /* end for */
	}

	/* call monitorenter function */

#if defined(USE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		s4 func_enter = (m->flags & ACC_STATIC) ?
			(s4) builtin_staticmonitorenter : (s4) builtin_monitorenter;
		p = dseg_addaddress(cd, (void *) func_enter);
		M_ALD(REG_ITMP3, REG_PV, p);
		M_MTCTR(REG_ITMP3);
		M_ALD(rd->argintregs[0], REG_SP, rd->maxmemuse * 4);
		M_JSR;
	}
#endif
	}

	/* end of header generation */

	/* walk through all basic blocks */
	for (bptr = m->basicblocks; bptr != NULL; bptr = bptr->next) {

		bptr->mpc = (s4) ((u1 *) mcodeptr - cd->mcodebase);

		if (bptr->flags >= BBREACHED) {

		/* branch resolving */

		{
		branchref *brefs;
		for (brefs = bptr->branchrefs; brefs != NULL; brefs = brefs->next) {
			gen_resolvebranch((u1*) cd->mcodebase + brefs->branchpos, 
			                  brefs->branchpos,
							  bptr->mpc);
			}
		}

		/* copy interface registers to their destination */

		src = bptr->instack;
		len = bptr->indepth;
		MCODECHECK(64+len);

#ifdef LSRA
		if (opt_lsra) {
			while (src != NULL) {
				len--;
				if ((len == 0) && (bptr->type != BBTYPE_STD)) {
					/* 							d = reg_of_var(m, src, REG_ITMP1); */
					if (!(src->flags & INMEMORY))
						d= src->regoff;
					else
						d=REG_ITMP1;
					M_INTMOVE(REG_ITMP1, d);
					store_reg_to_var_int(src, d);

					src = src->prev;
				}
			} else {
#endif
		while (src != NULL) {
			len--;
			if ((len == 0) && (bptr->type != BBTYPE_STD)) {
				d = reg_of_var(rd, src, REG_ITMP1);
				M_INTMOVE(REG_ITMP1, d);
				store_reg_to_var_int(src, d);

			} else {
				d = reg_of_var(rd, src, REG_IFTMP);
				if ((src->varkind != STACKVAR)) {
					s2 = src->type;
					if (IS_FLT_DBL_TYPE(s2)) {
						if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
							s1 = rd->interfaces[len][s2].regoff;
							M_FLTMOVE(s1,d);

						} else {
							if (IS_2_WORD_TYPE(s2)) {
								M_DLD(d, REG_SP, 4 * rd->interfaces[len][s2].regoff);

							} else {
								M_FLD(d, REG_SP, 4 * rd->interfaces[len][s2].regoff);
							}
						}
						store_reg_to_var_flt(src, d);

					} else {
						if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
							s1 = rd->interfaces[len][s2].regoff;
							M_TINTMOVE(s2,s1,d);

						} else {
							M_ILD(d, REG_SP, 4 * rd->interfaces[len][s2].regoff);
							if (IS_2_WORD_TYPE(s2))
								M_ILD(rd->secondregs[d], REG_SP, 4 * rd->interfaces[len][s2].regoff + 4);
						}
						store_reg_to_var_int(src, d);
					}
				}
			}
			src = src->prev;
		}

#ifdef LSRA
		}
#endif
		/* walk through all instructions */
		
		src = bptr->instack;
		len = bptr->icount;
		for (iptr = bptr->iinstr; len > 0; src = iptr->dst, len--, iptr++) {

	MCODECHECK(64);           /* an instruction usually needs < 64 words      */
	switch (iptr->opc) {

		case ICMD_NOP:        /* ...  ==> ...                                 */
			break;

		case ICMD_NULLCHECKPOP: /* ..., objectref  ==> ...                    */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_TST(s1);
			M_BEQ(0);
			codegen_addxnullrefs(cd, mcodeptr);
			break;

		/* constant operations ************************************************/

		case ICMD_ICONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.i = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			ICONST(d, iptr->val.i);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.l = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			LCONST(d, iptr->val.l);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.f = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			a = dseg_addfloat(cd, iptr->val.f);
			M_FLD(d, REG_PV, a);
			store_reg_to_var_flt(iptr->dst, d);
			break;
			
		case ICMD_DCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.d = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			a = dseg_adddouble(cd, iptr->val.d);
			M_DLD(d, REG_PV, a);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_ACONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.a = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			ICONST(d, (u4) iptr->val.a);
			store_reg_to_var_int(iptr->dst, d);
			break;


		/* load/store operations **********************************************/

		case ICMD_ILOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_LLOAD:      /* op1 = local variable                         */
		case ICMD_ALOAD:

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY) {
				M_ILD(d, REG_SP, 4 * var->regoff);
				if (IS_2_WORD_TYPE(var->type))
					M_ILD(rd->secondregs[d], REG_SP, 4 * var->regoff + 4);
			} else {
				M_TINTMOVE(var->type, var->regoff, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FLOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_DLOAD:      /* op1 = local variable                         */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY)
				if (IS_2_WORD_TYPE(var->type))
					M_DLD(d, REG_SP, 4 * var->regoff);
				else
					M_FLD(d, REG_SP, 4 * var->regoff);
			else {
				M_FLTMOVE(var->regoff, d);
			}
			store_reg_to_var_flt(iptr->dst, d);
			break;


		case ICMD_ISTORE:     /* ..., value  ==> ...                          */
		case ICMD_LSTORE:     /* op1 = local variable                         */
		case ICMD_ASTORE:

			if ((src->varkind == LOCALVAR) &&
			    (src->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				var_to_reg_int(s1, src, REG_ITMP1);
				M_IST(s1, REG_SP, 4 * var->regoff);
				if (IS_2_WORD_TYPE(var->type))
					M_IST(rd->secondregs[s1], REG_SP, 4 * var->regoff + 4);
			} else {
				var_to_reg_int(s1, src, var->regoff);
				M_TINTMOVE(var->type, s1, var->regoff);
			}
			break;

		case ICMD_FSTORE:     /* ..., value  ==> ...                          */
		case ICMD_DSTORE:     /* op1 = local variable                         */

			if ((src->varkind == LOCALVAR) &&
			    (src->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				var_to_reg_flt(s1, src, REG_FTMP1);
				if (var->type == TYPE_DBL)
					M_DST(s1, REG_SP, 4 * var->regoff);
				else
					M_FST(s1, REG_SP, 4 * var->regoff);
			} else {
				var_to_reg_flt(s1, src, var->regoff);
				M_FLTMOVE(s1, var->regoff);
			}
			break;


		/* pop/dup/swap operations ********************************************/

		/* attention: double and longs are only one entry in CACAO ICMDs      */

		case ICMD_POP:        /* ..., value  ==> ...                          */
		case ICMD_POP2:       /* ..., value, value  ==> ...                   */
			break;

		case ICMD_DUP:        /* ..., a ==> ..., a, a                         */
			M_COPY(src, iptr->dst);
			break;

		case ICMD_DUP_X1:     /* ..., a, b ==> ..., b, a, b                   */

			M_COPY(src,       iptr->dst);
			M_COPY(src->prev, iptr->dst->prev);
			M_COPY(iptr->dst, iptr->dst->prev->prev);
			break;

		case ICMD_DUP_X2:     /* ..., a, b, c ==> ..., c, a, b, c             */

			M_COPY(src,             iptr->dst);
			M_COPY(src->prev,       iptr->dst->prev);
			M_COPY(src->prev->prev, iptr->dst->prev->prev);
			M_COPY(iptr->dst,       iptr->dst->prev->prev->prev);
			break;

		case ICMD_DUP2:       /* ..., a, b ==> ..., a, b, a, b                */

			M_COPY(src,       iptr->dst);
			M_COPY(src->prev, iptr->dst->prev);
			break;

		case ICMD_DUP2_X1:    /* ..., a, b, c ==> ..., b, c, a, b, c          */

			M_COPY(src,             iptr->dst);
			M_COPY(src->prev,       iptr->dst->prev);
			M_COPY(src->prev->prev, iptr->dst->prev->prev);
			M_COPY(iptr->dst,       iptr->dst->prev->prev->prev);
			M_COPY(iptr->dst->prev, iptr->dst->prev->prev->prev->prev);
			break;

		case ICMD_DUP2_X2:    /* ..., a, b, c, d ==> ..., c, d, a, b, c, d    */

			M_COPY(src,                   iptr->dst);
			M_COPY(src->prev,             iptr->dst->prev);
			M_COPY(src->prev->prev,       iptr->dst->prev->prev);
			M_COPY(src->prev->prev->prev, iptr->dst->prev->prev->prev);
			M_COPY(iptr->dst,             iptr->dst->prev->prev->prev->prev);
			M_COPY(iptr->dst->prev,       iptr->dst->prev->prev->prev->prev->prev);
			break;

		case ICMD_SWAP:       /* ..., a, b ==> ..., b, a                      */

			M_COPY(src,       iptr->dst->prev);
			M_COPY(src->prev, iptr->dst);
			break;


		/* integer operations *************************************************/

		case ICMD_INEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_int(s1, src, REG_ITMP1); 
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_NEG(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

        case ICMD_LNEG:       /* ..., value  ==> ..., - value                 */

            var_to_reg_int(s1, src, REG_ITMP1);
            d = reg_of_var(rd, iptr->dst, REG_ITMP3);
            M_SUBFIC(rd->secondregs[s1], 0, rd->secondregs[d]);
            M_SUBFZE(s1, d);
            store_reg_to_var_int(iptr->dst, d);
            break;

		case ICMD_I2L:        /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, rd->secondregs[d]);
			M_SRA_IMM(rd->secondregs[d], 31, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */

			var_to_reg_int0(s1, src, REG_ITMP2, 0, 1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_INTMOVE(s1, d );
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_BSEXT(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2CHAR:   /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
            M_CZEXT(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2SHORT:  /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_SSEXT(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_IADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_IADD(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.i >= -32768) && (iptr->val.i <= 32767)) {
				M_IADD_IMM(s1, iptr->val.i, d);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_IADD(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

        case ICMD_LADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

            var_to_reg_int0(s1, src->prev, REG_ITMP1, 0, 1);
            var_to_reg_int0(s2, src, REG_ITMP2, 0, 1);
            d = reg_of_var(rd, iptr->dst, REG_ITMP3);
            M_ADDC(s1, s2, rd->secondregs[d]);
            var_to_reg_int0(s1, src->prev, REG_ITMP1, 1, 0);
            var_to_reg_int0(s2, src, REG_ITMP3, 1, 0);
            M_ADDE(s1, s2, d);
            store_reg_to_var_int(iptr->dst, d);
            break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.l = constant                             */

			s3 = iptr->val.l & 0xffffffff;
			var_to_reg_int0(s1, src, REG_ITMP1, 0, 1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((s3 >= -32768) && (s3 <= 32767)) {
				M_ADDIC(s1, s3, rd->secondregs[d]);
				}
			else {
				ICONST(REG_ITMP2, s3);
				M_ADDC(s1, REG_ITMP2, rd->secondregs[d]);
				}
			var_to_reg_int0(s1, src, REG_ITMP1, 1, 0);
			s3 = iptr->val.l >> 32;
			if (s3 == -1)
				M_ADDME(s1, d);
			else if (s3 == 0)
				M_ADDZE(s1, d);
			else {
				ICONST(REG_ITMP3, s3);
				M_ADDE(s1, REG_ITMP3, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_ISUB(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.i >= -32767) && (iptr->val.i <= 32768)) {
				M_IADD_IMM(s1, -iptr->val.i, d);
				}
			else {
				ICONST(REG_ITMP2, -iptr->val.i);
				M_IADD(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

        case ICMD_LSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

            var_to_reg_int0(s1, src->prev, REG_ITMP1, 0, 1);
            var_to_reg_int0(s2, src, REG_ITMP2, 0, 1);
            d = reg_of_var(rd, iptr->dst, REG_ITMP3);
            M_SUBC(s1, s2, rd->secondregs[d]);
            var_to_reg_int0(s1, src->prev, REG_ITMP1, 1, 0);
            var_to_reg_int0(s2, src, REG_ITMP3, 1, 0);
            M_SUBE(s1, s2, d);
            store_reg_to_var_int(iptr->dst, d);
            break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* val.l = constant                             */

			s3 = (-iptr->val.l) & 0xffffffff;
			var_to_reg_int0(s1, src, REG_ITMP1, 0, 1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((s3 >= -32768) && (s3 <= 32767)) {
				M_ADDIC(s1, s3, rd->secondregs[d]);
				}
			else {
				ICONST(REG_ITMP2, s3);
				M_ADDC(s1, REG_ITMP2, rd->secondregs[d]);
				}
			var_to_reg_int0(s1, src, REG_ITMP1, 1, 0);
			s3 = (-iptr->val.l) >> 32;
			if (s3 == -1)
				M_ADDME(s1, d);
			else if (s3 == 0)
				M_ADDZE(s1, d);
			else {
				ICONST(REG_ITMP3, s3);
				M_ADDE(s1, REG_ITMP3, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */
			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_IDIV(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IREM:       /* ..., val1, val2  ==> ..., val1 % val2        */
			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_IDIV(s1, s2, d);
			M_IMUL(d, s2, d);
			M_ISUB(s1, d, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_IMUL(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.i >= -32768) && (iptr->val.i <= 32767)) {
				M_IMUL_IMM(s1, iptr->val.i, d);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_IMUL(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IDIVPOW2:   /* ..., value  ==> ..., value << constant       */
		                      
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_SRA_IMM(s1, iptr->val.i, d);
			M_ADDZE(d, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_AND_IMM(s2, 0x1f, REG_ITMP3);
			M_SLL(s1, REG_ITMP3, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_SLL_IMM(s1, iptr->val.i & 0x1f, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_AND_IMM(s2, 0x1f, REG_ITMP3);
			M_SRA(s1, REG_ITMP3, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_SRA_IMM(s1, iptr->val.i & 0x1f, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_AND_IMM(s2, 0x1f, REG_ITMP2);
			M_SRL(s1, REG_ITMP2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr->val.i & 0x1f)
				M_SRL_IMM(s1, iptr->val.i & 0x1f, d);
			else
				M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_AND(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 65535)) {
				M_AND_IMM(s1, iptr->val.i, d);
				}
			/*
			else if (iptr->val.i == 0xffffff) {
				M_RLWINM(s1, 0, 8, 31, d);
				}
			*/
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_AND(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			var_to_reg_int0(s1, src->prev, REG_ITMP1, 0, 1);
            var_to_reg_int0(s2, src, REG_ITMP2, 0, 1);
            d = reg_of_var(rd, iptr->dst, REG_ITMP3);
            M_AND(s1, s2, rd->secondregs[d]);
            var_to_reg_int0(s1, src->prev, REG_ITMP1, 1, 0);
            var_to_reg_int0(s2, src, REG_ITMP3, 1, 0);
            M_AND(s1, s2, d);
            store_reg_to_var_int(iptr->dst, d);
            break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.l = constant                             */

			s3 = iptr->val.l & 0xffffffff;
			var_to_reg_int0(s1, src, REG_ITMP1, 0, 1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((s3 >= 0) && (s3 <= 65535)) {
				M_AND_IMM(s1, s3, rd->secondregs[d]);
				}
			else {
				ICONST(REG_ITMP2, s3);
				M_AND(s1, REG_ITMP2, rd->secondregs[d]);
				}
			var_to_reg_int0(s1, src, REG_ITMP1, 1, 0);
			s3 = iptr->val.l >> 32;
			if ((s3 >= 0) && (s3 <= 65535)) {
				M_AND_IMM(s1, s3, d);
				}
			else {
				ICONST(REG_ITMP3, s3);
				M_AND(s1, REG_ITMP3, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_MOV(s1, REG_ITMP2);
			M_CMPI(s1, 0);
			M_BGE(1 + 2*(iptr->val.i >= 32768));
			if (iptr->val.i >= 32768) {
				M_ADDIS(REG_ZERO, iptr->val.i>>16, REG_ITMP2);
				M_OR_IMM(REG_ITMP2, iptr->val.i, REG_ITMP2);
				M_IADD(s1, REG_ITMP2, REG_ITMP2);
				} 
			else
				M_IADD_IMM(s1, iptr->val.i, REG_ITMP2);
			{
				int b=0, m = iptr->val.i;
				while (m >>= 1)
					++b;
				M_RLWINM(REG_ITMP2, 0, 0, 30-b, REG_ITMP2);
			}
			M_ISUB(s1, REG_ITMP2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_OR(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 65535)) {
				M_OR_IMM(s1, iptr->val.i, d);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_OR(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LOR:       /* ..., val1, val2  ==> ..., val1 | val2        */

			var_to_reg_int0(s1, src->prev, REG_ITMP1, 0, 1);
            var_to_reg_int0(s2, src, REG_ITMP2, 0, 1);
            d = reg_of_var(rd, iptr->dst, REG_ITMP3);
            M_OR(s1, s2, rd->secondregs[d]);
            var_to_reg_int0(s1, src->prev, REG_ITMP1, 1, 0);
            var_to_reg_int0(s2, src, REG_ITMP3, 1, 0);
            M_OR(s1, s2, d);
            store_reg_to_var_int(iptr->dst, d);
            break;

		case ICMD_LORCONST:  /* ..., value  ==> ..., value | constant        */
		                      /* val.l = constant                             */

			s3 = iptr->val.l & 0xffffffff;
			var_to_reg_int0(s1, src, REG_ITMP1, 0, 1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((s3 >= 0) && (s3 <= 65535)) {
				M_OR_IMM(s1, s3, rd->secondregs[d]);
				}
			else {
				ICONST(REG_ITMP2, s3);
				M_OR(s1, REG_ITMP2, rd->secondregs[d]);
				}
			var_to_reg_int0(s1, src, REG_ITMP1, 1, 0);
			s3 = iptr->val.l >> 32;
			if ((s3 >= 0) && (s3 <= 65535)) {
				M_OR_IMM(s1, s3, d);
				}
			else {
				ICONST(REG_ITMP3, s3);
				M_OR(s1, REG_ITMP3, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_XOR(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 65535)) {
				M_XOR_IMM(s1, iptr->val.i, d);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_XOR(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			var_to_reg_int0(s1, src->prev, REG_ITMP1, 0, 1);
            var_to_reg_int0(s2, src, REG_ITMP2, 0, 1);
            d = reg_of_var(rd, iptr->dst, REG_ITMP3);
            M_XOR(s1, s2, rd->secondregs[d]);
            var_to_reg_int0(s1, src->prev, REG_ITMP1, 1, 0);
            var_to_reg_int0(s2, src, REG_ITMP3, 1, 0);
            M_XOR(s1, s2, d);
            store_reg_to_var_int(iptr->dst, d);
            break;

		case ICMD_LXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.l = constant                             */

			s3 = iptr->val.l & 0xffffffff;
			var_to_reg_int0(s1, src, REG_ITMP1, 0, 1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((s3 >= 0) && (s3 <= 65535)) {
				M_XOR_IMM(s1, s3, rd->secondregs[d]);
				}
			else {
				ICONST(REG_ITMP2, s3);
				M_XOR(s1, REG_ITMP2, rd->secondregs[d]);
				}
			var_to_reg_int0(s1, src, REG_ITMP1, 1, 0);
			s3 = iptr->val.l >> 32;
			if ((s3 >= 0) && (s3 <= 65535)) {
				M_XOR_IMM(s1, s3, d);
				}
			else {
				ICONST(REG_ITMP3, s3);
				M_XOR(s1, REG_ITMP3, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LCMP:       /* ..., val1, val2  ==> ..., val1 cmp val2      */

			var_to_reg_int0(s1, src->prev, REG_ITMP3, 1, 0);
			var_to_reg_int0(s2, src, REG_ITMP2, 1, 0);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			{
				int tempreg =
					(d==s1 || d==s2 || d==rd->secondregs[s1] || d==rd->secondregs[s2]);
				int dreg = tempreg ? REG_ITMP1 : d;
				s4 *br1;
				M_IADD_IMM(REG_ZERO, 1, dreg);
				M_CMP(s1, s2);
				M_BGT(0);
				br1 = mcodeptr;
				M_BLT(0);
				var_to_reg_int0(s1, src->prev, REG_ITMP3, 0, 1);
				var_to_reg_int0(s2, src, REG_ITMP2, 0, 1);
				M_CMPU(s1, s2);
				M_BGT(3);
				M_BEQ(1);
				M_IADD_IMM(dreg, -1, dreg);
				M_IADD_IMM(dreg, -1, dreg);
				gen_resolvebranch(br1, (u1*) br1, (u1*) mcodeptr);
				gen_resolvebranch(br1+1, (u1*) (br1+1), (u1*) (mcodeptr-2));
				M_INTMOVE(dreg, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IINC:       /* ..., value  ==> ..., value + constant        */
		                      /* op1 = variable, val.i = constant             */

            var = &(rd->locals[iptr->op1][TYPE_INT]);
            if (var->flags & INMEMORY) {
                s1 = REG_ITMP1;
                M_ILD(s1, REG_SP, 4 * var->regoff);
                }
            else
                s1 = var->regoff;
			{
				u4 m = iptr->val.i;
				if (m&0x8000)
					m += 65536;
				if (m&0xffff0000)
					M_ADDIS(s1, m>>16, s1);
				if (m&0xffff)
					M_IADD_IMM(s1, m&0xffff, s1);
			}
            if (var->flags & INMEMORY)
				M_IST(s1, REG_SP, 4 * var->regoff);
			break;


		/* floating operations ************************************************/

		case ICMD_FNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_FMOVN(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_FMOVN(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_FADD(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_DADD(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_FSUB(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_DSUB(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_FMUL(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DMUL:       /* ..., val1, val2  ==> ..., val1 *** val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_DMUL(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_FDIV(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_DDIV(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
		
		case ICMD_F2I:       /* ..., value  ==> ..., (int) value              */
		case ICMD_D2I:
			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_CLR(d);
			a = dseg_addfloat(cd, 0.0);
			M_FLD(REG_FTMP2, REG_PV, a);
			M_FCMPU(s1, REG_FTMP2);
			M_BNAN(4);
			a = dseg_adds4(cd, 0);
			M_CVTDL_C(s1, REG_FTMP1);
			M_LDA (REG_ITMP1, REG_PV, a);
			M_STFIWX(REG_FTMP1, 0, REG_ITMP1);
			M_ILD (d, REG_PV, a);
			store_reg_to_var_int(iptr->dst, d);
			break;
		
		case ICMD_F2D:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_FLTMOVE(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
					
		case ICMD_D2F:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_CVTDF(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
		
		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */
		case ICMD_DCMPL:
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_FCMPU(s2, s1);
			M_IADD_IMM(0, -1, d);
			M_BNAN(4);
			M_BGT(3);
			M_IADD_IMM(0, 0, d);
			M_BGE(1);
			M_IADD_IMM(0, 1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */
		case ICMD_DCMPG:
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_FCMPU(s1, s2);
			M_IADD_IMM(0, 1, d);
			M_BNAN(4);
			M_BGT(3);
			M_IADD_IMM(0, 0, d);
			M_BGE(1);
			M_IADD_IMM(0, -1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;
			

		/* memory operations **************************************************/

		case ICMD_ARRAYLENGTH: /* ..., arrayref  ==> ..., length              */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			gen_nullptr_check(s1);
			M_ILD(d, s1, OFFSET(java_arrayheader, size));
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_AALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			M_SLL_IMM(s2, 2, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_objectarray, data[0]), REG_ITMP2);
			M_LWZX(d, s1, REG_ITMP2);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			M_SLL_IMM(s2, 3, REG_ITMP2);
			M_IADD(s1, REG_ITMP2, REG_ITMP2);
			M_ILD(d, REG_ITMP2, OFFSET(java_longarray, data[0]));
			M_ILD(rd->secondregs[d], REG_ITMP2, OFFSET(java_longarray, data[0])+4);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			M_SLL_IMM(s2, 2, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_intarray, data[0]), REG_ITMP2);
			M_LWZX(d, s1, REG_ITMP2);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			M_SLL_IMM(s2, 2, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_floatarray, data[0]), REG_ITMP2);
			M_LFSX(d, s1, REG_ITMP2);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			M_SLL_IMM(s2, 3, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_doublearray, data[0]), REG_ITMP2);
			M_LFDX(d, s1, REG_ITMP2);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_BALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			M_IADD_IMM(s2, OFFSET(java_chararray, data[0]), REG_ITMP2);
			M_LBZX(d, s1, REG_ITMP2);
			M_BSEXT(d, d);
			store_reg_to_var_int(iptr->dst, d);
			break;			

		case ICMD_SALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			M_SLL_IMM(s2, 1, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_shortarray, data[0]), REG_ITMP2);
			M_LHAX(d, s1, REG_ITMP2);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_CALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			M_SLL_IMM(s2, 1, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_chararray, data[0]), REG_ITMP2);
			M_LHZX(d, s1, REG_ITMP2);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_int0(s3, src, REG_ITMP3, 1, 0);
			M_SLL_IMM(s2, 3, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_intarray, data[0]), REG_ITMP2);
			M_STWX(s3, s1, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, 4, REG_ITMP2);
			var_to_reg_int0(s3, src, REG_ITMP3, 0, 1);
			M_STWX(s3, s1, REG_ITMP2);
			break;

		case ICMD_IASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_int(s3, src, REG_ITMP3);
			M_SLL_IMM(s2, 2, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_intarray, data[0]), REG_ITMP2);
			M_STWX(s3, s1, REG_ITMP2);
			break;

		case ICMD_FASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_flt(s3, src, REG_FTMP3);
			M_SLL_IMM(s2, 2, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_floatarray, data[0]), REG_ITMP2);
			M_STFSX(s3, s1, REG_ITMP2);
			break;

		case ICMD_DASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_flt(s3, src, REG_FTMP3);
			M_SLL_IMM(s2, 3, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_doublearray, data[0]), REG_ITMP2);
			M_STFDX(s3, s1, REG_ITMP2);
			break;

		case ICMD_BASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_int(s3, src, REG_ITMP3);
			M_IADD_IMM(s2, OFFSET(java_bytearray, data[0]), REG_ITMP2);
			M_STBX(s3, s1, REG_ITMP2);
			break;

		case ICMD_SASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_int(s3, src, REG_ITMP3);
			M_SLL_IMM(s2, 1, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_shortarray, data[0]), REG_ITMP2);
			M_STHX(s3, s1, REG_ITMP2);
			break;

		case ICMD_CASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_int(s3, src, REG_ITMP3);
			M_SLL_IMM(s2, 1, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_chararray, data[0]), REG_ITMP2);
			M_STHX(s3, s1, REG_ITMP2);
			break;

		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */
		                      /* op1 = type, val.a = field address            */

			/* if class isn't yet initialized, do it */
			if (!((fieldinfo *) iptr->val.a)->class->initialized) {
				/* call helper function which patches this code */
				a = dseg_addaddress(cd, ((fieldinfo *) iptr->val.a)->class);
				M_ALD(REG_ITMP1, REG_PV, a);
				a = dseg_addaddress(cd, asm_check_clinit);
				M_ALD(REG_PV, REG_PV, a);
				M_MTCTR(REG_PV);
				M_JSR;

				/* recompute pv */
				s1 = (s4) ((u1 *) mcodeptr - cd->mcodebase);
				M_MFLR(REG_ITMP1);
				if (s1 <= 32768) M_LDA(REG_PV, REG_ITMP1, -s1);
				else {
					s4 ml = -s1, mh = 0;
					while (ml < -32768) { ml += 65536; mh--; }
					M_LDA(REG_PV, REG_ITMP1, ml);
					M_LDAH(REG_PV, REG_PV, mh);
				}
			}

			a = dseg_addaddress(cd, &(((fieldinfo *) iptr->val.a)->value));
			M_ALD(REG_ITMP1, REG_PV, a);
			switch (iptr->op1) {
				case TYPE_INT:
					var_to_reg_int(s2, src, REG_ITMP2);
					M_IST(s2, REG_ITMP1, 0);
					break;
				case TYPE_LNG:
					var_to_reg_int(s2, src, REG_ITMP3);
					M_IST(s2, REG_ITMP1, 0);
					M_IST(rd->secondregs[s2], REG_ITMP1, 4);
					break;
				case TYPE_ADR:
					var_to_reg_int(s2, src, REG_ITMP2);
					M_AST(s2, REG_ITMP1, 0);
					break;
				case TYPE_FLT:
					var_to_reg_flt(s2, src, REG_FTMP2);
					M_FST(s2, REG_ITMP1, 0);
					break;
				case TYPE_DBL:
					var_to_reg_flt(s2, src, REG_FTMP2);
					M_DST(s2, REG_ITMP1, 0);
					break;
				default: panic ("internal error");
				}
			break;

		case ICMD_GETSTATIC:  /* ...  ==> ..., value                          */
		                      /* op1 = type, val.a = field address            */

            /* if class isn't yet initialized, do it */
            if (!((fieldinfo *) iptr->val.a)->class->initialized) {
                /* call helper function which patches this code */
                a = dseg_addaddress(cd, ((fieldinfo *) iptr->val.a)->class);
                M_ALD(REG_ITMP1, REG_PV, a);
                a = dseg_addaddress(cd, asm_check_clinit);
                M_ALD(REG_PV, REG_PV, a);
                M_MTCTR(REG_PV);
                M_JSR;

                /* recompute pv */
                s1 = (s4) ((u1 *) mcodeptr - cd->mcodebase);
                M_MFLR(REG_ITMP1);
                if (s1 <= 32768) M_LDA(REG_PV, REG_ITMP1, -s1);
                else {
                    s4 ml = -s1, mh = 0;
                    while (ml < -32768) { ml += 65536; mh--; }
                    M_LDA(REG_PV, REG_ITMP1, ml);
                    M_LDAH(REG_PV, REG_PV, mh);
                }
            }

			a = dseg_addaddress(cd, &(((fieldinfo *) iptr->val.a)->value));
			M_ALD(REG_ITMP1, REG_PV, a);
			switch (iptr->op1) {
				case TYPE_INT:
					d = reg_of_var(rd, iptr->dst, REG_ITMP3);
					M_ILD(d, REG_ITMP1, 0);
					store_reg_to_var_int(iptr->dst, d);
					break;
				case TYPE_LNG:
					d = reg_of_var(rd, iptr->dst, REG_ITMP3);
					M_ILD(d, REG_ITMP1, 0);
					M_ILD(rd->secondregs[d], REG_ITMP1, 4);
					store_reg_to_var_int(iptr->dst, d);
					break;
				case TYPE_ADR:
					d = reg_of_var(rd, iptr->dst, REG_ITMP3);
					M_ALD(d, REG_ITMP1, 0);
					store_reg_to_var_int(iptr->dst, d);
					break;
				case TYPE_FLT:
					d = reg_of_var(rd, iptr->dst, REG_FTMP1);
					M_FLD(d, REG_ITMP1, 0);
					store_reg_to_var_flt(iptr->dst, d);
					break;
				case TYPE_DBL:				
					d = reg_of_var(rd, iptr->dst, REG_FTMP1);
					M_DLD(d, REG_ITMP1, 0);
					store_reg_to_var_flt(iptr->dst, d);
					break;
				default: panic ("internal error");
				}
			break;


		case ICMD_PUTFIELD:   /* ..., value  ==> ...                          */
		                      /* op1 = type, val.i = field offset             */

			a = ((fieldinfo *)(iptr->val.a))->offset;
			switch (iptr->op1) {
				case TYPE_INT:
					var_to_reg_int(s1, src->prev, REG_ITMP1);
					var_to_reg_int(s2, src, REG_ITMP2);
					gen_nullptr_check(s1);
					M_IST(s2, s1, a);
					break;
				case TYPE_LNG:
					var_to_reg_int(s1, src->prev, REG_ITMP1);
					var_to_reg_int(s2, src, REG_ITMP3);
					gen_nullptr_check(s1);
					M_IST(s2, s1, a);
					M_IST(rd->secondregs[s2], s1, a+4);
					break;
				case TYPE_ADR:
					var_to_reg_int(s1, src->prev, REG_ITMP1);
					var_to_reg_int(s2, src, REG_ITMP2);
					gen_nullptr_check(s1);
					M_AST(s2, s1, a);
					break;
				case TYPE_FLT:
					var_to_reg_int(s1, src->prev, REG_ITMP1);
					var_to_reg_flt(s2, src, REG_FTMP2);
					gen_nullptr_check(s1);
					M_FST(s2, s1, a);
					break;
				case TYPE_DBL:
					var_to_reg_int(s1, src->prev, REG_ITMP1);
					var_to_reg_flt(s2, src, REG_FTMP2);
					gen_nullptr_check(s1);
					M_DST(s2, s1, a);
					break;
				default: panic ("internal error");
				}
			break;

		case ICMD_GETFIELD:   /* ...  ==> ..., value                          */
		                      /* op1 = type, val.i = field offset             */

			a = ((fieldinfo *)(iptr->val.a))->offset;
			switch (iptr->op1) {
				case TYPE_INT:
					var_to_reg_int(s1, src, REG_ITMP1);
					d = reg_of_var(rd, iptr->dst, REG_ITMP3);
					gen_nullptr_check(s1);
					M_ILD(d, s1, a);
					store_reg_to_var_int(iptr->dst, d);
					break;
				case TYPE_LNG:
					var_to_reg_int(s1, src, REG_ITMP1);
					d = reg_of_var(rd, iptr->dst, REG_ITMP3);
					gen_nullptr_check(s1);
					M_ILD(d, s1, a);
					M_ILD(rd->secondregs[d], s1, a+4);
					store_reg_to_var_int(iptr->dst, d);
					break;
				case TYPE_ADR:
					var_to_reg_int(s1, src, REG_ITMP1);
					d = reg_of_var(rd, iptr->dst, REG_ITMP3);
					gen_nullptr_check(s1);
					M_ALD(d, s1, a);
					store_reg_to_var_int(iptr->dst, d);
					break;
				case TYPE_FLT:
					var_to_reg_int(s1, src, REG_ITMP1);
					d = reg_of_var(rd, iptr->dst, REG_FTMP1);
					gen_nullptr_check(s1);
					M_FLD(d, s1, a);
					store_reg_to_var_flt(iptr->dst, d);
					break;
				case TYPE_DBL:				
					var_to_reg_int(s1, src, REG_ITMP1);
					d = reg_of_var(rd, iptr->dst, REG_FTMP1);
					gen_nullptr_check(s1);
					M_DLD(d, s1, a);
					store_reg_to_var_flt(iptr->dst, d);
					break;
				default: panic ("internal error");
				}
			break;


		/* branch operations **************************************************/

		case ICMD_ATHROW:       /* ..., objectref ==> ... (, objectref)       */

			a = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP2, REG_PV, a);
			M_MTCTR(REG_ITMP2);
			var_to_reg_int(s1, src, REG_ITMP1);
			M_INTMOVE(s1, REG_ITMP1_XPTR);

			if (m->isleafmethod) M_MFLR(REG_ITMP3);  /* save LR */
			M_BL(0);            /* get current PC */
			M_MFLR(REG_ITMP2_XPC);
			if (m->isleafmethod) M_MTLR(REG_ITMP3);  /* restore LR */
			M_RTS;              /* jump to CTR */

			ALIGNCODENOP;
			break;

		case ICMD_GOTO:         /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */
			M_BR(0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			ALIGNCODENOP;
			break;

		case ICMD_JSR:          /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */

			if (m->isleafmethod) M_MFLR(REG_ITMP2);
			M_BL(0);
			M_MFLR(REG_ITMP1);
			M_IADD_IMM(REG_ITMP1, m->isleafmethod ? 16 : 12, REG_ITMP1);
			if (m->isleafmethod) M_MTLR(REG_ITMP2);
			M_BR(0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;
			
		case ICMD_RET:          /* ... ==> ...                                */
		                        /* op1 = local variable                       */

			var = &(rd->locals[iptr->op1][TYPE_ADR]);
			if (var->flags & INMEMORY) {
				M_ALD(REG_ITMP1, REG_SP, 4 * var->regoff);
				M_MTCTR(REG_ITMP1);
				}
			else
				M_MTCTR(var->regoff);
			M_RTS;
			ALIGNCODENOP;
			break;

		case ICMD_IFNULL:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_TST(s1);
			M_BEQ(0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFNONNULL:    /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_TST(s1);
			M_BNE(0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFLT:
		case ICMD_IFLE:
		case ICMD_IFNE:
		case ICMD_IFGT:
		case ICMD_IFGE:
		case ICMD_IFEQ:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if ((iptr->val.i >= -32768) && (iptr->val.i <= 32767)) {
				M_CMPI(s1, iptr->val.i);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_CMP(s1, REG_ITMP2);
				}
			switch (iptr->opc)
			{
				case ICMD_IFLT:
					M_BLT(0);
					break;
				case ICMD_IFLE:
					M_BLE(0);
					break;
				case ICMD_IFNE:
					M_BNE(0);
					break;
				case ICMD_IFGT:
					M_BGT(0);
					break;
				case ICMD_IFGE:
					M_BGE(0);
					break;
				case ICMD_IFEQ:
					M_BEQ(0);
					break;
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;


		case ICMD_IF_LEQ:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

            var_to_reg_int0(s1, src, REG_ITMP1, 0, 1);
            var_to_reg_int0(s2, src, REG_ITMP2, 1, 0);
			if (iptr->val.l == 0) {
				M_OR(s1, s2, REG_ITMP3);
				M_CMPI(REG_ITMP3, 0);

  			} else if ((iptr->val.l >= -32768) && (iptr->val.l <= 32767)) {
  				M_CMPI(s2, (u4) (iptr->val.l >> 32));
				M_BNE(2);
  				M_CMPI(s1, (u4) (iptr->val.l & 0xffffffff));

  			} else {
  				ICONST(REG_ITMP3, (u4) (iptr->val.l >> 32));
  				M_CMP(s2, REG_ITMP3);
				M_BNE(3);
  				ICONST(REG_ITMP3, (u4) (iptr->val.l & 0xffffffff));
				M_CMP(s1, REG_ITMP3)
			}
			M_BEQ(0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;
			
		case ICMD_IF_LLT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */
            var_to_reg_int0(s1, src, REG_ITMP1, 0, 1);
            var_to_reg_int0(s2, src, REG_ITMP2, 1, 0);
/*  			if (iptr->val.l == 0) { */
/*  				M_OR(s1, s2, REG_ITMP3); */
/*  				M_CMPI(REG_ITMP3, 0); */

/*    			} else  */
			if ((iptr->val.l >= -32768) && (iptr->val.l <= 32767)) {
  				M_CMPI(s2, (u4) (iptr->val.l >> 32));
				M_BLT(0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
				M_BGT(2);
  				M_CMPI(s1, (u4) (iptr->val.l & 0xffffffff));

  			} else {
  				ICONST(REG_ITMP3, (u4) (iptr->val.l >> 32));
  				M_CMP(s2, REG_ITMP3);
				M_BLT(0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
				M_BGT(3);
  				ICONST(REG_ITMP3, (u4) (iptr->val.l & 0xffffffff));
				M_CMP(s1, REG_ITMP3)
			}
			M_BLT(0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;
			
		case ICMD_IF_LLE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

            var_to_reg_int0(s1, src, REG_ITMP1, 0, 1);
            var_to_reg_int0(s2, src, REG_ITMP2, 1, 0);
/*  			if (iptr->val.l == 0) { */
/*  				M_OR(s1, s2, REG_ITMP3); */
/*  				M_CMPI(REG_ITMP3, 0); */

/*    			} else  */
			if ((iptr->val.l >= -32768) && (iptr->val.l <= 32767)) {
  				M_CMPI(s2, (u4) (iptr->val.l >> 32));
				M_BLT(0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
				M_BGT(2);
  				M_CMPI(s1, (u4) (iptr->val.l & 0xffffffff));

  			} else {
  				ICONST(REG_ITMP3, (u4) (iptr->val.l >> 32));
  				M_CMP(s2, REG_ITMP3);
				M_BLT(0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
				M_BGT(3);
  				ICONST(REG_ITMP3, (u4) (iptr->val.l & 0xffffffff));
				M_CMP(s1, REG_ITMP3)
			}
			M_BLE(0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;
			
		case ICMD_IF_LNE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

            var_to_reg_int0(s1, src, REG_ITMP1, 0, 1);
            var_to_reg_int0(s2, src, REG_ITMP2, 1, 0);
			if (iptr->val.l == 0) {
				M_OR(s1, s2, REG_ITMP3);
				M_CMPI(REG_ITMP3, 0);

  			} else if ((iptr->val.l >= -32768) && (iptr->val.l <= 32767)) {
  				M_CMPI(s2, (u4) (iptr->val.l >> 32));
				M_BEQ(2);
  				M_CMPI(s1, (u4) (iptr->val.l & 0xffffffff));

  			} else {
  				ICONST(REG_ITMP3, (u4) (iptr->val.l >> 32));
  				M_CMP(s2, REG_ITMP3);
				M_BEQ(3);
  				ICONST(REG_ITMP3, (u4) (iptr->val.l & 0xffffffff));
				M_CMP(s1, REG_ITMP3)
			}
			M_BNE(0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;
			
		case ICMD_IF_LGT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

            var_to_reg_int0(s1, src, REG_ITMP1, 0, 1);
            var_to_reg_int0(s2, src, REG_ITMP2, 1, 0);
/*  			if (iptr->val.l == 0) { */
/*  				M_OR(s1, s2, REG_ITMP3); */
/*  				M_CMPI(REG_ITMP3, 0); */

/*    			} else  */
			if ((iptr->val.l >= -32768) && (iptr->val.l <= 32767)) {
  				M_CMPI(s2, (u4) (iptr->val.l >> 32));
				M_BGT(0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
				M_BLT(2);
  				M_CMPI(s1, (u4) (iptr->val.l & 0xffffffff));

  			} else {
  				ICONST(REG_ITMP3, (u4) (iptr->val.l >> 32));
  				M_CMP(s2, REG_ITMP3);
				M_BGT(0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
				M_BLT(3);
  				ICONST(REG_ITMP3, (u4) (iptr->val.l & 0xffffffff));
				M_CMP(s1, REG_ITMP3)
			}
			M_BGT(0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;
			
		case ICMD_IF_LGE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */
            var_to_reg_int0(s1, src, REG_ITMP1, 0, 1);
            var_to_reg_int0(s2, src, REG_ITMP2, 1, 0);
/*  			if (iptr->val.l == 0) { */
/*  				M_OR(s1, s2, REG_ITMP3); */
/*  				M_CMPI(REG_ITMP3, 0); */

/*    			} else  */
			if ((iptr->val.l >= -32768) && (iptr->val.l <= 32767)) {
  				M_CMPI(s2, (u4) (iptr->val.l >> 32));
				M_BGT(0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
				M_BLT(2);
  				M_CMPI(s1, (u4) (iptr->val.l & 0xffffffff));

  			} else {
  				ICONST(REG_ITMP3, (u4) (iptr->val.l >> 32));
  				M_CMP(s2, REG_ITMP3);
				M_BGT(0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
				M_BLT(3);
  				ICONST(REG_ITMP3, (u4) (iptr->val.l & 0xffffffff));
				M_CMP(s1, REG_ITMP3)
			}
			M_BGE(0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

			/* CUT: alle _L */
		case ICMD_IF_ICMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPEQ:    /* op1 = target JavaVM pc                     */
		case ICMD_IF_ACMPEQ:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BEQ(0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPNE:    /* op1 = target JavaVM pc                     */
		case ICMD_IF_ACMPNE:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BNE(0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPLT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPLT:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BLT(0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPGT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPGT:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BGT(0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPLE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPLE:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BLE(0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPGE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPGE:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BGE(0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_LRETURN:
		case ICMD_ARETURN:

		case ICMD_FRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_DRETURN:

		case ICMD_RETURN:      /* ...  ==> ...                                */

#if defined(USE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				a = dseg_addaddress(cd, (void *) (builtin_monitorexit));
				M_ALD(REG_ITMP3, REG_PV, a);
				M_MTCTR(REG_ITMP3);
				M_ALD(rd->argintregs[0], REG_SP, rd->maxmemuse * 4);
				M_JSR;
			}
#endif
			switch (iptr->opc) {
			case ICMD_IRETURN:
			case ICMD_LRETURN:
			case ICMD_ARETURN:
				var_to_reg_int(s1, src, REG_RESULT);
				M_TINTMOVE(src->type, s1, REG_RESULT);
				goto nowperformreturn;

			case ICMD_FRETURN:
			case ICMD_DRETURN:
				var_to_reg_flt(s1, src, REG_FRESULT);
				M_FLTMOVE(s1, REG_FRESULT);
				goto nowperformreturn;
			}

nowperformreturn:
			{
			s4 i, p;
			
			p = parentargs_base;
			
			/* restore return address                                         */

			if (!m->isleafmethod) {
				M_ALD(REG_ITMP3, REG_SP, 4 * p + 8);
				M_MTLR(REG_ITMP3);
			}

			/* restore saved registers                                        */

			for (i = rd->savintregcnt - 1; i >= rd->maxsavintreguse; i--) {
				p--; M_ILD(rd->savintregs[i], REG_SP, 4 * p);
			}
			for (i = rd->savfltregcnt - 1; i >= rd->maxsavfltreguse; i--) {
				p -= 2; M_DLD(rd->savfltregs[i], REG_SP, 4 * p);
			}

			/* deallocate stack                                               */

			if (parentargs_base) {
				M_LDA(REG_SP, REG_SP, parentargs_base*4);
			}

			/* call trace function */

			if (runverbose) {
				M_MFLR(REG_ITMP3);
				M_LDA(REG_SP, REG_SP, -10 * 8);
				M_DST(REG_FRESULT, REG_SP, 48+0);
				M_IST(REG_RESULT, REG_SP, 48+8);
				M_AST(REG_ITMP3, REG_SP, 48+12);
				M_IST(REG_RESULT2, REG_SP, 48+16);
				a = dseg_addaddress(cd, m);

				/* keep this order */
				switch (iptr->opc) {
				case ICMD_IRETURN:
				case ICMD_ARETURN:
					M_MOV(REG_RESULT, rd->argintregs[2]);
					M_CLR(rd->argintregs[1]);
					break;

				case ICMD_LRETURN:
					M_MOV(REG_RESULT2, rd->argintregs[2]);
					M_MOV(REG_RESULT, rd->argintregs[1]);
					break;
				}
				M_ALD(rd->argintregs[0], REG_PV, a);

				M_FLTMOVE(REG_FRESULT, rd->argfltregs[0]);
				M_FLTMOVE(REG_FRESULT, rd->argfltregs[1]);
				a = dseg_addaddress(cd, (void *) builtin_displaymethodstop);
				M_ALD(REG_ITMP2, REG_PV, a);
				M_MTCTR(REG_ITMP2);
				M_JSR;
				M_DLD(REG_FRESULT, REG_SP, 48+0);
				M_ILD(REG_RESULT, REG_SP, 48+8);
				M_ALD(REG_ITMP3, REG_SP, 48+12);
				M_ILD(REG_RESULT2, REG_SP, 48+16);
				M_LDA(REG_SP, REG_SP, 10 * 8);
				M_MTLR(REG_ITMP3);
			}

			M_RET;
			ALIGNCODENOP;
			}
			break;


		case ICMD_TABLESWITCH:  /* ..., index ==> ...                         */
			{
			s4 i, l, *s4ptr;
			void **tptr;

			tptr = (void **) iptr->target;

			s4ptr = iptr->val.a;
			l = s4ptr[1];                          /* low     */
			i = s4ptr[2];                          /* high    */
			
			var_to_reg_int(s1, src, REG_ITMP1);
			if (l == 0) {
				M_INTMOVE(s1, REG_ITMP1);
			} else if (l <= 32768) {
				M_LDA(REG_ITMP1, s1, -l);
			} else {
				ICONST(REG_ITMP2, l);
				M_ISUB(s1, REG_ITMP2, REG_ITMP1);
			}
			i = i - l + 1;

			/* range check */

			M_CMPUI(REG_ITMP1, i - 1);
			M_BGT(0);

			/* codegen_addreference(cd, BlockPtrOfPC(s4ptr[0]), mcodeptr); */
			codegen_addreference(cd, (basicblock *) tptr[0], mcodeptr);

			/* build jump table top down and use address of lowest entry */

			/* s4ptr += 3 + i; */
			tptr += i;

			while (--i >= 0) {
				/* dseg_addtarget(cd, BlockPtrOfPC(*--s4ptr)); */
				dseg_addtarget(cd, (basicblock *) tptr[0]); 
				--tptr;
				}
			}

			/* length of dataseg after last dseg_addtarget is used by load */

			M_SLL_IMM(REG_ITMP1, 2, REG_ITMP1);
			M_IADD(REG_ITMP1, REG_PV, REG_ITMP2);
			M_ALD(REG_ITMP2, REG_ITMP2, -(cd->dseglen));
			M_MTCTR(REG_ITMP2);
			M_RTS;
			ALIGNCODENOP;
			break;


		case ICMD_LOOKUPSWITCH: /* ..., key ==> ...                           */
			{
			s4 i, l, val, *s4ptr;
			void **tptr;

			tptr = (void **) iptr->target;

			s4ptr = iptr->val.a;
			l = s4ptr[0];                          /* default  */
			i = s4ptr[1];                          /* count    */
			
			MCODECHECK((i<<2)+8);
			var_to_reg_int(s1, src, REG_ITMP1);
			while (--i >= 0) {
				s4ptr += 2;
				++tptr;

				val = s4ptr[0];
				if ((val >= -32768) && (val <= 32767)) {
					M_CMPI(s1, val);
					} 
				else {
					a = dseg_adds4(cd, val);
					M_ILD(REG_ITMP2, REG_PV, a);
					M_CMP(s1, REG_ITMP2);
					}
				M_BEQ(0);
				/* codegen_addreference(cd, BlockPtrOfPC(s4ptr[1]), mcodeptr); */
				codegen_addreference(cd, (basicblock *) tptr[0], mcodeptr); 
				}

			M_BR(0);
			/* codegen_addreference(cd, BlockPtrOfPC(l), mcodeptr); */
			
			tptr = (void **) iptr->target;
			codegen_addreference(cd, (basicblock *) tptr[0], mcodeptr);

			ALIGNCODENOP;
			break;
			}


		case ICMD_BUILTIN3:     /* ..., arg1, arg2, arg3 ==> ...              */
		                        /* op1 = return type, val.a = function pointer*/
			s3 = 3;
			goto gen_method;

		case ICMD_BUILTIN2:     /* ..., arg1, arg2 ==> ...                    */
		                        /* op1 = return type, val.a = function pointer*/
			s3 = 2;
			goto gen_method;

		case ICMD_BUILTIN1:     /* ..., arg1 ==> ...                          */
		                        /* op1 = return type, val.a = function pointer*/
			s3 = 1;
			goto gen_method;

		case ICMD_INVOKESTATIC: /* ..., [arg1, [arg2 ...]] ==> ...            */
		                        /* op1 = arg count, val.a = method pointer    */

		case ICMD_INVOKESPECIAL:/* ..., objectref, [arg1, [arg2 ...]] ==> ... */
		                        /* op1 = arg count, val.a = method pointer    */

		case ICMD_INVOKEVIRTUAL:/* ..., objectref, [arg1, [arg2 ...]] ==> ... */
		                        /* op1 = arg count, val.a = method pointer    */

		case ICMD_INVOKEINTERFACE:/*.., objectref, [arg1, [arg2 ...]] ==> ... */
		                        /* op1 = arg count, val.a = method pointer    */

			s3 = iptr->op1;

gen_method: {
			methodinfo *lm;
			classinfo  *ci;

			MCODECHECK((s3 << 1) + 64);

			{
			/* copy arguments to registers or stack location                  */

				stackptr srcsave = src;
				int s3save = s3;
				int fltcnt = 0;
				int argsize = 0;
			for (; --s3 >= 0; src = src->prev) {
				argsize += IS_2_WORD_TYPE(src->type) ? 2 : 1;
				if (IS_FLT_DBL_TYPE(src->type))
					++fltcnt;
				}

			for (s3 = s3save, src = srcsave; --s3 >= 0; src = src->prev) {
				argsize -= IS_2_WORD_TYPE(src->type) ? 2 : 1;
				if (IS_FLT_DBL_TYPE(src->type))
					--fltcnt;
				if (src->varkind == ARGVAR)
					continue;
				if (IS_INT_LNG_TYPE(src->type)) {
					if (argsize < INT_ARG_CNT) {
						s1 = rd->argintregs[argsize];
						var_to_reg_int(d, src, s1);
						if (argsize < INT_ARG_CNT-1) {
							M_TINTMOVE(src->type, d, s1);

						} else {
							M_INTMOVE(d, s1);
							if (IS_2_WORD_TYPE(src->type))
								M_IST(rd->secondregs[d], REG_SP, 4 * (argsize + 6) + 4);
						}

					} else {
						var_to_reg_int(d, src, REG_ITMP1);
						M_IST(d, REG_SP, 4 * (argsize + 6));
						if (IS_2_WORD_TYPE(src->type))
							M_IST(rd->secondregs[d], REG_SP, 4 * (argsize + 6) + 4);
					}

				} else {
					if (fltcnt < FLT_ARG_CNT) {
						s1 = rd->argfltregs[fltcnt];
						var_to_reg_flt(d, src, s1);
						M_FLTMOVE(d, s1);

					} else {
						var_to_reg_flt(d, src, REG_FTMP1);
						if (IS_2_WORD_TYPE(src->type))
							M_DST(d, REG_SP, 4 * (argsize + 6));
						else
							M_FST(d, REG_SP, 4 * (argsize + 6));
					}
				}
			} /* end of for */
			}

			lm = iptr->val.a;
			switch (iptr->opc) {
				case ICMD_BUILTIN3:
				case ICMD_BUILTIN2:
				case ICMD_BUILTIN1:
					a = dseg_addaddress(cd, (void *) lm);

					M_ALD(REG_PV, REG_PV, a); /* Pointer to built-in-function */
					d = iptr->op1;
					goto makeactualcall;

				case ICMD_INVOKESTATIC:
				case ICMD_INVOKESPECIAL:
					a = dseg_addaddress(cd, lm->stubroutine);

					M_ALD(REG_PV, REG_PV, a);       /* method pointer in r27 */

					d = lm->returntype;
					goto makeactualcall;

				case ICMD_INVOKEVIRTUAL:

					gen_nullptr_check(rd->argintregs[0]);
					M_ALD(REG_METHODPTR, rd->argintregs[0],
						  OFFSET(java_objectheader, vftbl));
					M_ALD(REG_PV, REG_METHODPTR, OFFSET(vftbl_t, table[0]) +
						  sizeof(methodptr) * lm->vftblindex);

					d = lm->returntype;
					goto makeactualcall;

				case ICMD_INVOKEINTERFACE:
					ci = lm->class;
					
					gen_nullptr_check(rd->argintregs[0]);
					M_ALD(REG_METHODPTR, rd->argintregs[0],
						  OFFSET(java_objectheader, vftbl));    
					M_ALD(REG_METHODPTR, REG_METHODPTR,
					      OFFSET(vftbl_t, interfacetable[0]) -
					      sizeof(methodptr*) * ci->index);
					M_ALD(REG_PV, REG_METHODPTR,
						  sizeof(methodptr) * (lm - ci->methods));

					d = lm->returntype;
					goto makeactualcall;

				default:
					d = 0;
					error ("Unkown ICMD-Command: %d", iptr->opc);
				}

makeactualcall:
			M_MTCTR(REG_PV);
			M_JSR;

			/* recompute pv */

			s1 = (s4) ((u1 *) mcodeptr - cd->mcodebase);
			M_MFLR(REG_ITMP1);
			if (s1 <= 32768) M_LDA(REG_PV, REG_ITMP1, -s1);
			else {
				s4 ml = -s1, mh = 0;
				while (ml < -32768) { ml += 65536; mh--; }
				M_LDA(REG_PV, REG_ITMP1, ml);
				M_LDAH(REG_PV, REG_PV, mh);
			}

			/* d contains return type */

			if (d != TYPE_VOID) {
				if (IS_INT_LNG_TYPE(iptr->dst->type)) {
					s1 = reg_of_var(rd, iptr->dst, REG_RESULT);
					M_TINTMOVE(iptr->dst->type, REG_RESULT, s1);
					store_reg_to_var_int(iptr->dst, s1);

				} else {
					s1 = reg_of_var(rd, iptr->dst, REG_FRESULT);
					M_FLTMOVE(REG_FRESULT, s1);
					store_reg_to_var_flt(iptr->dst, s1);
				}
			}
			}
			break;


		case ICMD_INSTANCEOF: /* ..., objectref ==> ..., intresult            */

		                      /* op1:   0 == array, 1 == class                */
		                      /* val.a: (classinfo*) superclass               */

/*          superclass is an interface:
 *
 *          return (sub != NULL) &&
 *                 (sub->vftbl->interfacetablelength > super->index) &&
 *                 (sub->vftbl->interfacetable[-super->index] != NULL);
 *
 *          superclass is a class:
 *
 *          return ((sub != NULL) && (0
 *                  <= (sub->vftbl->baseval - super->vftbl->baseval) <=
 *                  super->vftbl->diffvall));
 */

			{
			classinfo *super = (classinfo*) iptr->val.a;
			
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
            codegen_threadcritrestart(cd, (u1*) mcodeptr - cd->mcodebase);
#endif
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (s1 == d) {
				M_MOV(s1, REG_ITMP1);
				s1 = REG_ITMP1;
				}
			M_CLR(d);
			if (iptr->op1) {                               /* class/interface */
				M_TST(s1);
				if (super->flags & ACC_INTERFACE) {        /* interface       */
					M_BEQ(9);
					M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
					M_ILD(REG_ITMP2, REG_ITMP1, OFFSET(vftbl_t, interfacetablelength));
					M_LDATST(REG_ITMP2, REG_ITMP2, - super->index);
					M_BLE(5);
					M_ALD(REG_ITMP1, REG_ITMP1,
					      OFFSET(vftbl_t, interfacetable[0]) -
					      super->index * sizeof(methodptr*));
					M_TST(REG_ITMP1);
					M_CLR(d);
					M_BEQ(1);
					M_IADD_IMM(REG_ZERO, 1, d);
					}
				else {                                     /* class           */
					M_BEQ(10);
					M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
					a = dseg_addaddress(cd, (void*) super->vftbl);
					M_ALD(REG_ITMP2, REG_PV, a);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
					codegen_threadcritstart(cd, (u1*) mcodeptr - cd->mcodebase);
#endif
					M_ILD(REG_ITMP1, REG_ITMP1, OFFSET(vftbl_t, baseval));
					M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl_t, baseval));
					M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, diffval));
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
					codegen_threadcritstop(cd, (u1*) mcodeptr - cd->mcodebase);
#endif
					M_ISUB(REG_ITMP1, REG_ITMP3, REG_ITMP1);
					M_CMPU(REG_ITMP1, REG_ITMP2);
					M_CLR(d);
					M_BGT(1);
					M_IADD_IMM(REG_ZERO, 1, d);
					}
				}
			else
				panic ("internal error: no inlined array instanceof");
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_CHECKCAST:  /* ..., objectref ==> ..., objectref            */

		                      /* op1:   0 == array, 1 == class                */
		                      /* val.a: (classinfo*) superclass               */

/*          superclass is an interface:
 *
 *          OK if ((sub == NULL) ||
 *                 (sub->vftbl->interfacetablelength > super->index) &&
 *                 (sub->vftbl->interfacetable[-super->index] != NULL));
 *
 *          superclass is a class:
 *
 *          OK if ((sub == NULL) || (0
 *                 <= (sub->vftbl->baseval - super->vftbl->baseval) <=
 *                 super->vftbl->diffvall));
 */

			{
			classinfo *super = (classinfo*) iptr->val.a;
			
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			codegen_threadcritrestart(cd, (u1*) mcodeptr - cd->mcodebase);
#endif
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			var_to_reg_int(s1, src, d);
			if (iptr->op1) {                               /* class/interface */
				M_TST(s1);
				if (super->flags & ACC_INTERFACE) {        /* interface       */
					M_BEQ(7);
					M_ALD(REG_ITMP2, s1, OFFSET(java_objectheader, vftbl));
					M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl_t, interfacetablelength));
					M_LDATST(REG_ITMP3, REG_ITMP3, - super->index);
					M_BLE(0);
					codegen_addxcastrefs(cd, mcodeptr);
					M_ALD(REG_ITMP3, REG_ITMP2,
					      OFFSET(vftbl_t, interfacetable[0]) -
					      super->index * sizeof(methodptr*));
					M_TST(REG_ITMP3);
					M_BEQ(0);
					codegen_addxcastrefs(cd, mcodeptr);
					}
				else {                                     /* class           */
                    M_BEQ(8 + (s1 == REG_ITMP1));
                    M_ALD(REG_ITMP2, s1, OFFSET(java_objectheader, vftbl));
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
					codegen_threadcritstart(cd, (u1*) mcodeptr - cd->mcodebase);
#endif
                    M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl_t, baseval));
                    a = dseg_addaddress(cd, (void*) super->vftbl);
                    M_ALD(REG_ITMP2, REG_PV, a);
                    if (d != REG_ITMP1) {
                        M_ILD(REG_ITMP1, REG_ITMP2, OFFSET(vftbl_t, baseval));
                        M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, diffval));
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
						codegen_threadcritstop(cd, (u1*) mcodeptr - cd->mcodebase);
#endif
                        M_ISUB(REG_ITMP3, REG_ITMP1, REG_ITMP3);
                        }
                    else {
                        M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, baseval));
                        M_ISUB(REG_ITMP3, REG_ITMP2, REG_ITMP3);
                        M_ALD(REG_ITMP2, REG_PV, a);
                        M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, diffval));
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
						codegen_threadcritstop(cd, (u1*) mcodeptr - cd->mcodebase);
#endif
                        }
                    M_CMPU(REG_ITMP3, REG_ITMP2);
                    M_BGT(0);
                    codegen_addxcastrefs(cd, mcodeptr);
                    }
				}
			else
				panic ("internal error: no inlined array checkcast");
			}
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_CHECKASIZE:  /* ..., size ==> ..., size                     */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_CMPI(s1, 0);
			M_BLT(0);
			codegen_addxcheckarefs(cd, mcodeptr);
			break;

		case ICMD_CHECKEXCEPTION:   /* ... ==> ...                            */

			M_CMPI(REG_RESULT, 0);
			M_BEQ(0);
			codegen_addxexceptionrefs(cd, mcodeptr);
			break;

		case ICMD_MULTIANEWARRAY:/* ..., cnt1, [cnt2, ...] ==> ..., arrayref  */
		                      /* op1 = dimension, val.a = array descriptor    */

			/* check for negative sizes and copy sizes to stack if necessary  */

			MCODECHECK((iptr->op1 << 1) + 64);

			for (s1 = iptr->op1; --s1 >= 0; src = src->prev) {
				var_to_reg_int(s2, src, REG_ITMP1);
				M_CMPI(s2, 0);
				M_BLT(0);
				codegen_addxcheckarefs(cd, mcodeptr);

				/* copy SAVEDVAR sizes to stack */

				if (src->varkind != ARGVAR) {
					M_IST(s2, REG_SP, (s1 + INT_ARG_CNT + 6) * 4);
				}
			}

			/* a0 = dimension count */

			ICONST(rd->argintregs[0], iptr->op1);

			/* a1 = arraydescriptor */

			a = dseg_addaddress(cd, iptr->val.a);
			M_ALD(rd->argintregs[1], REG_PV, a);

			/* a2 = pointer to dimensions = stack pointer */

			M_LDA(rd->argintregs[2], REG_SP, (INT_ARG_CNT + 6) * 4);

			a = dseg_addaddress(cd, (void *) builtin_nmultianewarray);
			M_ALD(REG_PV, REG_PV, a);
			M_MTCTR(REG_PV);
			M_JSR;
			s1 = (s4) ((u1 *) mcodeptr - cd->mcodebase);
			M_MFLR(REG_ITMP1);
			if (s1 <= 32768)
				M_LDA (REG_PV, REG_ITMP1, -s1);
			else {
				s4 ml = -s1, mh = 0;
				while (ml < -32768) {ml += 65536; mh--;}
				M_LDA(REG_PV, REG_ITMP1, ml);
				M_LDAH(REG_PV, REG_PV, mh);
			    }
			s1 = reg_of_var(rd, iptr->dst, REG_RESULT);
			M_INTMOVE(REG_RESULT, s1);
			store_reg_to_var_int(iptr->dst, s1);
			break;


		default: error ("Unknown pseudo command: %d", iptr->opc);
	
	} /* switch */
		
	} /* for instruction */
		
	/* copy values to interface registers */

	src = bptr->outstack;
	len = bptr->outdepth;
	MCODECHECK(64 + len);
#ifdef LSRA
	if (!opt_lsra)
#endif
	while (src) {
		len--;
		if ((src->varkind != STACKVAR)) {
			s2 = src->type;
			if (IS_FLT_DBL_TYPE(s2)) {
				var_to_reg_flt(s1, src, REG_FTMP1);
				if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
					M_FLTMOVE(s1, rd->interfaces[len][s2].regoff);

				} else {
					M_DST(s1, REG_SP, rd->interfaces[len][s2].regoff * 4);
				}

			} else {
				var_to_reg_int(s1, src, REG_ITMP1);
				if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
					M_TINTMOVE(s2, s1, rd->interfaces[len][s2].regoff);

				} else {
					M_IST(s1, REG_SP, rd->interfaces[len][s2].regoff * 4);
					if (IS_2_WORD_TYPE(s2))
						M_IST(rd->secondregs[s1], REG_SP, rd->interfaces[len][s2].regoff * 4 + 4);
				}
			}
		}
		src = src->prev;
	}
	} /* if (bptr -> flags >= BBREACHED) */
	} /* for basic block */

	/* bptr -> mpc = (int)((u1*) mcodeptr - mcodebase); */

	{
	/* generate bound check stubs */

	s4 *xcodeptr = NULL;
	branchref *bref;

	for (bref = cd->xboundrefs; bref != NULL; bref = bref->next) {
		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(14);

		/* move index register into REG_ITMP1 */
		M_MOV(bref->reg, REG_ITMP1);
		M_LDA(REG_ITMP2_XPC, REG_PV, bref->branchpos - 4);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr - 1);

		} else {
			xcodeptr = mcodeptr;

			M_STWU(REG_SP, REG_SP, -(24 + 2 * 4));
			M_IST(REG_ITMP2_XPC, REG_SP, 24 + 1 * 4);

			M_MOV(REG_ITMP1, rd->argintregs[0]);

			a = dseg_addaddress(cd, new_arrayindexoutofboundsexception);
			M_ALD(REG_ITMP2, REG_PV, a);
			M_MTCTR(REG_ITMP2);
			M_JSR;
			M_MOV(REG_RESULT, REG_ITMP1_XPTR);

			M_ILD(REG_ITMP2_XPC, REG_SP, 24 + 1 * 4);
			M_IADD_IMM(REG_SP, 24 + 2 * 4, REG_SP);

			a = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);

			M_MTCTR(REG_ITMP3);
			M_RTS;
		}
	}

	/* generate negative array size check stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xcheckarefs; bref != NULL; bref = bref->next) {
		if ((m->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  (u1 *) xcodeptr - (u1 *) cd->mcodebase - 4);
			continue;
		}

		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(12);

		M_LDA(REG_ITMP2_XPC, REG_PV, bref->branchpos - 4);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr - 1);

		} else {
			xcodeptr = mcodeptr;

			M_STWU(REG_SP, REG_SP, -(24 + 1 * 4));
			M_IST(REG_ITMP2_XPC, REG_SP, 24 + 0 * 4);

            a = dseg_addaddress(cd, new_negativearraysizeexception);
            M_ALD(REG_ITMP2, REG_PV, a);
            M_MTCTR(REG_ITMP2);
            M_JSR;
            M_MOV(REG_RESULT, REG_ITMP1_XPTR);

			M_ILD(REG_ITMP2_XPC, REG_SP, 24 + 0 * 4);
			M_IADD_IMM(REG_SP, 24 + 1 * 4, REG_SP);

			a = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);

			M_MTCTR(REG_ITMP3);
			M_RTS;
		}
	}

	/* generate cast check stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xcastrefs; bref != NULL; bref = bref->next) {
		if ((m->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  (u1 *) xcodeptr - (u1 *) cd->mcodebase - 4);
			continue;
		}

		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(12);

		M_LDA(REG_ITMP2_XPC, REG_PV, bref->branchpos - 4);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr - 1);

		} else {
			xcodeptr = mcodeptr;

			M_STWU(REG_SP, REG_SP, -(24 + 1 * 4));
            M_IST(REG_ITMP2_XPC, REG_SP, 24 + 0 * 4);

            a = dseg_addaddress(cd, new_classcastexception);
            M_ALD(REG_ITMP2, REG_PV, a);
            M_MTCTR(REG_ITMP2);
            M_JSR;
            M_MOV(REG_RESULT, REG_ITMP1_XPTR);

			M_ILD(REG_ITMP2_XPC, REG_SP, 24 + 0 * 4);
			M_IADD_IMM(REG_SP, 24 + 1 * 4, REG_SP);

			a = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);

			M_MTCTR(REG_ITMP3);
			M_RTS;
		}
	}

	/* generate exception check stubs */

	xcodeptr = NULL;

	for (bref = cd->xexceptionrefs; bref != NULL; bref = bref->next) {
		if ((m->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  (u1 *) xcodeptr - (u1 *) cd->mcodebase - 4);
			continue;
		}

		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(14);

		M_LDA(REG_ITMP2_XPC, REG_PV, bref->branchpos - 4);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr - 1);

		} else {
			xcodeptr = mcodeptr;

			M_STWU(REG_SP, REG_SP, -(24 + 1 * 4));
			M_IST(REG_ITMP2_XPC, REG_SP, 24 + 0 * 4);

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
            a = dseg_addaddress(cd, builtin_get_exceptionptrptr);
            M_ALD(REG_ITMP2, REG_PV, a);
            M_MTCTR(REG_ITMP2);
            M_JSR;

			/* get the exceptionptr from the ptrprt and clear it */
            M_ALD(REG_ITMP1_XPTR, REG_RESULT, 0);
			M_CLR(REG_ITMP3);
			M_AST(REG_ITMP3, REG_RESULT, 0);
#else
			a = dseg_addaddress(cd, &_exceptionptr);
			M_ALD(REG_ITMP2, REG_PV, a);

			M_ALD(REG_ITMP1_XPTR, REG_ITMP2, 0);
			M_CLR(REG_ITMP3);
			M_AST(REG_ITMP3, REG_ITMP2, 0);
#endif

            M_ILD(REG_ITMP2_XPC, REG_SP, 24 + 0 * 4);
            M_IADD_IMM(REG_SP, 24 + 1 * 4, REG_SP);

			a = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_MTCTR(REG_ITMP3);
			M_RTS;
		}
	}

	/* generate null pointer check stubs */

	xcodeptr = NULL;

	for (bref = cd->xnullrefs; bref != NULL; bref = bref->next) {
		if ((m->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  (u1 *) xcodeptr - (u1 *) cd->mcodebase - 4);
			continue;
		}

		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(12);

		M_LDA(REG_ITMP2_XPC, REG_PV, bref->branchpos - 4);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr - 1);

		} else {
			xcodeptr = mcodeptr;

			M_STWU(REG_SP, REG_SP, -(24 + 1 * 4));
			M_IST(REG_ITMP2_XPC, REG_SP, 24 + 0 * 4);

            a = dseg_addaddress(cd, new_nullpointerexception);
            M_ALD(REG_ITMP2, REG_PV, a);
            M_MTCTR(REG_ITMP2);
            M_JSR;
            M_MOV(REG_RESULT, REG_ITMP1_XPTR);

			M_ILD(REG_ITMP2_XPC, REG_SP, 24 + 0 * 4);
			M_IADD_IMM(REG_SP, 24 + 1 * 4, REG_SP);

			a = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_MTCTR(REG_ITMP3);
			M_RTS;
		}
	}
	}

	codegen_finish(m, cd, (s4) ((u1 *) mcodeptr - cd->mcodebase));

    asm_cacheflush((void *) m->entrypoint, ((u1 *) mcodeptr - cd->mcodebase));
}


/* function createcompilerstub *************************************************

	creates a stub routine which calls the compiler
	
*******************************************************************************/

#define COMPSTUBSIZE 6

u1 *createcompilerstub(methodinfo *m)
{
	s4 *s = CNEW (s4, COMPSTUBSIZE);    /* memory to hold the stub            */
	s4 *mcodeptr = s;                   /* code generation pointer            */

	M_LDA(REG_ITMP1, REG_PV, 4*4);
	M_ALD(REG_PV, REG_PV, 5*4);
	M_MTCTR(REG_PV);
	M_RTS;

	s[4] = (s4) m;                      /* literals to be adressed            */
	s[5] = (s4) asm_call_jit_compiler;  /* jump directly via PV from above    */

    asm_cacheflush((void*) s, (u1*) mcodeptr - (u1*) s);

#if defined(STATISTICS)
	if (opt_stat)
		count_cstub_len += COMPSTUBSIZE * 4;
#endif

	return (u1*) s;
}


/* function removecompilerstub *************************************************

     deletes a compilerstub from memory  (simply by freeing it)

*******************************************************************************/

void removecompilerstub(u1 *stub) 
{
	CFREE(stub, COMPSTUBSIZE * 4);
}


/* function: createnativestub **************************************************

	creates a stub routine which calls a native method

*******************************************************************************/

#define NATIVESTUBSIZE      200
#define NATIVESTUBOFFSET    9

u1 *createnativestub(functionptr f, methodinfo *m)
{
	s4 *s;                              /* memory to hold the stub            */
	s4 *cs;
	s4 *mcodeptr;                       /* code generation pointer            */
	s4 stackframesize = 0;              /* size of stackframe if needed       */
	s4 disp;
	registerdata *rd;
	t_inlining_globals *id;
	s4 dumpsize;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* setup registers before using it */
	
	rd = DNEW(registerdata);
	id = DNEW(t_inlining_globals);

	inlining_setup(m, id);
	reg_setup(m, rd, id);

	descriptor2types(m);                /* set paramcount and paramtypes      */

	s = CNEW(s4, NATIVESTUBSIZE);
	cs = s + NATIVESTUBOFFSET;
	mcodeptr = cs;

	*(cs-1) = (u4) f;                   /* address of native method           */
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	*(cs-2) = (u4) builtin_get_exceptionptrptr;
#else
	*(cs-2) = (u4) (&_exceptionptr);    /* address of exceptionptr            */
#endif
	*(cs-3) = (u4) asm_handle_nat_exception; /* addr of asm exception handler */
	*(cs-4) = (u4) (&env);              /* addr of jni_environement           */
	*(cs-5) = (u4) builtin_trace_args;
	*(cs-6) = (u4) m;
	*(cs-7) = (u4) builtin_displaymethodstop;
	*(cs-8) = (u4) m->class;
	*(cs-9) = (u4) asm_check_clinit;

	M_MFLR(REG_ITMP1);
	M_AST(REG_ITMP1, REG_SP, 8);        /* store return address               */
	M_STWU(REG_SP, REG_SP, -64);        /* build up stackframe                */

	/* if function is static, check for initialized */

	if (m->flags & ACC_STATIC) {
		/* if class isn't yet initialized, do it */
		if (!m->class->initialized) {
			/* call helper function which patches this code */
			M_ALD(REG_ITMP1, REG_PV, -8 * 4);     /* class                    */
			M_ALD(REG_PV, REG_PV, -9 * 4);        /* asm_check_clinit         */
			M_MTCTR(REG_PV);
			M_JSR;
			disp = -(s4) (mcodeptr - cs) * 4;
			M_MFLR(REG_ITMP1);
			M_LDA(REG_PV, REG_ITMP1, disp);       /* recompute pv from ra     */
		}
	}

	if (runverbose) {
		s4 p, t, s1, d, a;
        s4 longargs = 0;
        s4 fltargs = 0;
		s4 dblargs = 0;

/*          M_MFLR(REG_ITMP3); */
		/* XXX must be a multiple of 16 */
        M_LDA(REG_SP, REG_SP, -(24 + (INT_ARG_CNT + FLT_ARG_CNT + 1) * 8));

/*          M_IST(REG_ITMP3, REG_SP, 24 + (INT_ARG_CNT + FLT_ARG_CNT) * 8); */

		M_CLR(REG_ITMP1);

        /* save all arguments into the reserved stack space */
        for (p = 0; p < m->paramcount && p < TRACE_ARGS_NUM; p++) {
            t = m->paramtypes[p];

            if (IS_INT_LNG_TYPE(t)) {
                /* overlapping u8's are on the stack */
                if ((p + longargs + dblargs) < (INT_ARG_CNT - IS_2_WORD_TYPE(t))) {
                    s1 = rd->argintregs[p + longargs + dblargs];

                    if (!IS_2_WORD_TYPE(t)) {
                        M_IST(REG_ITMP1, REG_SP, 24 + p * 8);
                        M_IST(s1, REG_SP, 24 + p * 8 + 4);

                    } else {
                        M_IST(s1, REG_SP, 24 + p  * 8);
                        M_IST(rd->secondregs[s1], REG_SP, 24 + p * 8 + 4);
                        longargs++;
                    }

                } else {
					/* we do not have a data segment here */
                    /* a = dseg_adds4(cd, 0xdeadbeef);
					   M_ILD(REG_ITMP1, REG_PV, a); */
					M_LDA(REG_ITMP1, REG_ZERO, -1);
                    M_IST(REG_ITMP1, REG_SP, 24 + p * 8);
                    M_IST(REG_ITMP1, REG_SP, 24 + p * 8 + 4);
                }

            } else {
                if ((fltargs + dblargs) < FLT_ARG_CNT) {
                    s1 = rd->argfltregs[fltargs + dblargs];

                    if (!IS_2_WORD_TYPE(t)) {
                        M_IST(REG_ITMP1, REG_SP, 24 + p * 8);
                        M_FST(s1, REG_SP, 24 + p * 8 + 4);
                        fltargs++;

                    } else {
                        M_DST(s1, REG_SP, 24 + p * 8);
                        dblargs++;
                    }

                } else {
                    /* this should not happen */
                }
            }
        }

        /* TODO: save remaining integer and flaot argument registers */

        /* load first 4 arguments into integer argument registers */
        for (p = 0; p < 8; p++) {
            d = rd->argintregs[p];
            M_ILD(d, REG_SP, 24 + p * 4);
        }

        M_ALD(REG_ITMP1, REG_PV, -6 * 4);
        M_AST(REG_ITMP1, REG_SP, 11 * 8);    /* 24 (linkage area) + 32 (4 * s8 parameter area regs) + 32 (4 * s8 parameter area stack) = 88 */
		M_ALD(REG_ITMP2, REG_PV, -5 * 4);
        M_MTCTR(REG_ITMP2);
        M_JSR;

        longargs = 0;
        fltargs = 0;
        dblargs = 0;

        /* restore arguments into the reserved stack space */
        for (p = 0; p < m->paramcount && p < TRACE_ARGS_NUM; p++) {
            t = m->paramtypes[p];

            if (IS_INT_LNG_TYPE(t)) {
                if ((p + longargs + dblargs) < INT_ARG_CNT) {
                    s1 = rd->argintregs[p + longargs + dblargs];

                    if (!IS_2_WORD_TYPE(t)) {
                        M_ILD(s1, REG_SP, 24 + p * 8 + 4);

                    } else {
                        M_ILD(s1, REG_SP, 24 + p  * 8);
                        M_ILD(rd->secondregs[s1], REG_SP, 24 + p * 8 + 4);
                        longargs++;
                    }
                }

            } else {
                if ((fltargs + dblargs) < FLT_ARG_CNT) {
                    s1 = rd->argfltregs[fltargs + dblargs];

                    if (!IS_2_WORD_TYPE(t)) {
                        M_FLD(s1, REG_SP, 24 + p * 8 + 4);
                        fltargs++;

                    } else {
                        M_DLD(s1, REG_SP, 24 + p * 8);
                        dblargs++;
                    }
                }
            }
        }

/*          M_ILD(REG_ITMP3, REG_SP, 24 + (INT_ARG_CNT + FLT_ARG_CNT) * 8); */

        M_LDA(REG_SP, REG_SP, 24 + (INT_ARG_CNT + FLT_ARG_CNT + 1) * 8);
/*          M_MTLR(REG_ITMP3); */
	}

	/* save argument registers on stack -- if we have to */
	if ((m->flags & ACC_STATIC && m->paramcount > (INT_ARG_CNT - 2)) ||
		m->paramcount > (INT_ARG_CNT - 1)) {
		s4 i;
		s4 paramshiftcnt = (m->flags & ACC_STATIC) ? 2 : 1;
		s4 stackparamcnt = (m->paramcount > INT_ARG_CNT) ? m->paramcount - INT_ARG_CNT : 0;

		stackframesize = stackparamcnt + paramshiftcnt;

		M_LDA(REG_SP, REG_SP, -stackframesize * 8);

		panic("nativestub");
	}

	if (m->flags & ACC_STATIC) {
        M_MOV(rd->argintregs[5], rd->argintregs[7]);
        M_MOV(rd->argintregs[4], rd->argintregs[6]);
        M_MOV(rd->argintregs[3], rd->argintregs[5]);
        M_MOV(rd->argintregs[2], rd->argintregs[4]);
        M_MOV(rd->argintregs[1], rd->argintregs[3]);
        M_MOV(rd->argintregs[0], rd->argintregs[2]);

		/* put class into second argument register */
		M_ALD(rd->argintregs[1], REG_PV, -8 * 4);

	} else {
		M_MOV(rd->argintregs[6], rd->argintregs[7]); 
		M_MOV(rd->argintregs[5], rd->argintregs[6]); 
		M_MOV(rd->argintregs[4], rd->argintregs[5]); 
		M_MOV(rd->argintregs[3], rd->argintregs[4]);
		M_MOV(rd->argintregs[2], rd->argintregs[3]);
		M_MOV(rd->argintregs[1], rd->argintregs[2]);
		M_MOV(rd->argintregs[0], rd->argintregs[1]);
	}

	/* put env into first argument register */
	M_ALD(rd->argintregs[0], REG_PV, -4 * 4);

	M_ALD(REG_PV, REG_PV, -1 * 4);      /* load adress of native method       */
	M_MTCTR(REG_PV);
	M_JSR;
	disp = -(s4) (mcodeptr - cs) * 4;
	M_MFLR(REG_ITMP1);
	M_LDA(REG_PV, REG_ITMP1, disp);     /* recompute pv from ra               */

	/* remove stackframe if there is one */
	if (stackframesize) {
		M_LDA(REG_SP, REG_SP, stackframesize * 8);
	}

	/* 20 instructions */
	if (runverbose) {
		M_MFLR(REG_ITMP3);
		M_LDA(REG_SP, REG_SP, -10 * 8);
		M_DST(REG_FRESULT, REG_SP, 48+0);
		M_IST(REG_RESULT, REG_SP, 48+8);
		M_AST(REG_ITMP3, REG_SP, 48+12);
		M_IST(REG_RESULT2, REG_SP, 48+16);

		/* keep this order */
		switch (m->returntype) {
		case TYPE_INT:
		case TYPE_ADDRESS:
			M_MOV(REG_RESULT, rd->argintregs[2]);
			M_CLR(rd->argintregs[1]);
			break;

		case TYPE_LONG:
			M_MOV(REG_RESULT2, rd->argintregs[2]);
			M_MOV(REG_RESULT, rd->argintregs[1]);
			break;
		}
		M_ALD(rd->argintregs[0], REG_PV, -6 * 4);

		M_FLTMOVE(REG_FRESULT, rd->argfltregs[0]);
		M_FLTMOVE(REG_FRESULT, rd->argfltregs[1]);
		M_ALD(REG_ITMP2, REG_PV, -7 * 4);/* builtin_displaymethodstop         */
		M_MTCTR(REG_ITMP2);
		M_JSR;
		M_DLD(REG_FRESULT, REG_SP, 48+0);
		M_ILD(REG_RESULT, REG_SP, 48+8);
		M_ALD(REG_ITMP3, REG_SP, 48+12);
		M_ILD(REG_RESULT2, REG_SP, 48+16);
		M_LDA(REG_SP, REG_SP, 10 * 8);
		M_MTLR(REG_ITMP3);
	}

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	if (IS_FLT_DBL_TYPE(m->returntype))
		if (IS_2_WORD_TYPE(m->returntype))
			M_DST(REG_FRESULT, REG_SP, 56);
		else
			M_FST(REG_FRESULT, REG_SP, 56);
	else {
		M_IST(REG_RESULT, REG_SP, 56);
		if (IS_2_WORD_TYPE(m->returntype))
			M_IST(REG_RESULT2, REG_SP, 60);
	}

	M_ALD(REG_ITMP2, REG_PV, -2 * 4);   /* builtin_get_exceptionptrptr        */
	M_MTCTR(REG_ITMP2);
	M_JSR;
	disp = -(s4) (mcodeptr - (s4 *) cs) * 4;
	M_MFLR(REG_ITMP1);
	M_LDA(REG_PV, REG_ITMP1, disp);
	M_MOV(REG_RESULT, REG_ITMP2);

	if (IS_FLT_DBL_TYPE(m->returntype))
		if (IS_2_WORD_TYPE(m->returntype))
			M_DLD(REG_FRESULT, REG_SP, 56);
		else
			M_FLD(REG_FRESULT, REG_SP, 56);
	else {
		M_ILD(REG_RESULT, REG_SP, 56);
		if (IS_2_WORD_TYPE(m->returntype))
			M_ILD(REG_RESULT2, REG_SP, 60);
	}
#else
	M_ALD(REG_ITMP2, REG_PV, -2 * 4);   /* get address of exceptionptr        */
#endif
	M_ALD(REG_ITMP1, REG_ITMP2, 0);     /* load exception into reg. itmp1     */
	M_TST(REG_ITMP1);
	M_BNE(4);                           /* if no exception then return        */

	M_ALD(REG_ITMP1, REG_SP, 64 + 8);   /* load return address                */
	M_MTLR(REG_ITMP1);
	M_LDA(REG_SP, REG_SP, 64);          /* remove stackframe                  */

	M_RET;
	
	M_CLR(REG_ITMP3);
	M_AST(REG_ITMP3, REG_ITMP2, 0);     /* store NULL into exceptionptr       */

	M_ALD(REG_ITMP3, REG_SP, 64 + 8);   /* load return address                */
	M_MTLR(REG_ITMP3);
	M_LDA(REG_SP, REG_SP, 64);          /* remove stackframe                  */

	M_LDA(REG_ITMP2, REG_ITMP1, -4);    /* move fault address into reg. itmp2 */

	M_ALD(REG_ITMP3, REG_PV, -3 * 4);   /* load asm exception handler address */
	M_MTCTR(REG_ITMP3);
	M_RTS;
	
#if 0
	dolog_plain("stubsize: %d (for %d params)\n", (int) (mcodeptr - (s4 *) s), m->paramcount);
#endif

    asm_cacheflush((void *) s, (u1*) mcodeptr - (u1*) s);

#if defined(STATISTICS)
	if (opt_stat)
		count_nstub_len += NATIVESTUBSIZE * 4;
#endif

	/* release dump area */

	dump_release(dumpsize);

	return (u1*) (s + NATIVESTUBOFFSET);
}


/* function: removenativestub **************************************************

    removes a previously created native-stub from memory
    
*******************************************************************************/

void removenativestub(u1 *stub)
{
	CFREE((s4 *) stub - NATIVESTUBOFFSET, NATIVESTUBSIZE * 4);
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
