/* src/vm/jit/powerpc/codegen.c - machine code generator for 32-bit PowerPC

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

   $Id: codegen.c 2686 2005-06-14 17:28:36Z twisti $

*/


#include <assert.h>
#include <stdio.h>
#include <signal.h>

#include "config.h"

#include "md.h"
#include "md-abi.h"
#include "md-abi.inc"

#include "vm/jit/powerpc/arch.h"
#include "vm/jit/powerpc/codegen.h"
#include "vm/jit/powerpc/types.h"

#include "cacao/cacao.h"
#include "native/native.h"
#include "vm/builtin.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/stringlocal.h"
#include "vm/tables.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/codegen.inc"
#include "vm/jit/jit.h"

#if defined(LSRA)
# include "vm/jit/lsra.inc"
#endif

#include "vm/jit/parse.h"
#include "vm/jit/patcher.h"
#include "vm/jit/reg.h"
#include "vm/jit/reg.inc"


void asm_cacheflush(void *, long);

/* #include <architecture/ppc/cframe.h> */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void thread_restartcriticalsection(void *u)
{
	/* XXX set pc to restart address */
}
#endif


/* codegen *********************************************************************

   Generates machine code.

*******************************************************************************/

void codegen(methodinfo *m, codegendata *cd, registerdata *rd)
{
	s4              len, s1, s2, s3, d;
	ptrint          a;
	s4              parentargs_base;
	s4             *mcodeptr;
	stackptr        src;
	varinfo        *var;
	basicblock     *bptr;
	instruction    *iptr;
	exceptiontable *ex;
	methodinfo     *lm;                 /* local methodinfo for ICMD_INVOKE*  */
	builtintable_entry *bte;
	methoddesc     *md;

	{
	s4 i, p, t, l;
	s4 savedregs_num;

	savedregs_num = 0;

	/* space to save used callee saved registers */

	savedregs_num += (rd->savintregcnt - rd->maxsavintreguse);
	savedregs_num += 2 * (rd->savfltregcnt - rd->maxsavfltreguse);

	parentargs_base = rd->maxmemuse + savedregs_num;

#if defined(USE_THREADS)               /* space to save argument of monitor_enter   */
	                               /* and Return Values to survive monitor_exit */
	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		parentargs_base += 3;

#endif

	/* create method header */

	parentargs_base = (parentargs_base + 3) & ~3;

#if SIZEOF_VOID_P == 4
	(void) dseg_addaddress(cd, m);                          /* Filler         */
#endif
	(void) dseg_addaddress(cd, m);                          /* MethodPointer  */
	(void) dseg_adds4(cd, parentargs_base * 4);             /* FrameSize      */

#if defined(USE_THREADS)

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
		(void) dseg_addaddress(cd, ex->catchtype.cls);
	}
	
	/* initialize mcode variables */
	
	mcodeptr = (s4 *) cd->mcodebase;
	cd->mcodeend = (s4 *) (cd->mcodebase + cd->mcodesize);
	MCODECHECK(128 + m->paramcount);

	/* create stack frame (if necessary) */

	if (!m->isleafmethod) {
		M_MFLR(REG_ITMP3);
		M_AST(REG_ITMP3, REG_SP, LA_LR_OFFSET);
	}

	if (parentargs_base) {
		M_STWU(REG_SP, REG_SP, -parentargs_base * 4);
	}

	/* save return address and used callee saved registers */

	p = parentargs_base;
	for (i = rd->savintregcnt - 1; i >= rd->maxsavintreguse; i--) {
		p--; M_IST(rd->savintregs[i], REG_SP, p * 4);
	}
	for (i = rd->savfltregcnt - 1; i >= rd->maxsavfltreguse; i--) {
		p -= 2; M_DST(rd->savfltregs[i], REG_SP, p * 4);
	}

	/* take arguments out of register or stack frame */

	md = m->parseddesc;

 	for (p = 0, l = 0; p < md->paramcount; p++) {
 		t = md->paramtypes[p].type;
 		var = &(rd->locals[l][t]);
 		l++;
 		if (IS_2_WORD_TYPE(t))    /* increment local counter for 2 word types */
 			l++;
 		if (var->type < 0)
 			continue;
		s1 = md->params[p].regoff;
		if (IS_INT_LNG_TYPE(t)) {                    /* integer args          */
			s2 = rd->argintregs[s1];
 			if (!md->params[p].inmemory) {           /* register arguments    */
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
 					M_TINTMOVE(t, s2, var->regoff);

				} else {                             /* reg arg -> spilled    */
					M_IST(s2, REG_SP, var->regoff * 4);
					if (IS_2_WORD_TYPE(t))
						M_IST(rd->secondregs[s2], REG_SP, 4 * var->regoff + 4);
				}

			} else {                                 /* stack arguments       */
 				if (!(var->flags & INMEMORY)) {      /* stack arg -> register */
					M_ILD(var->regoff, REG_SP, (parentargs_base + s1) * 4);
					if (IS_2_WORD_TYPE(t))
						M_ILD(rd->secondregs[var->regoff], REG_SP, (parentargs_base + s1) * 4 + 4);

				} else {                             /* stack arg -> spilled  */
 					M_ILD(REG_ITMP1, REG_SP, (parentargs_base + s1) * 4);
 					M_IST(REG_ITMP1, REG_SP, var->regoff * 4);
					if (IS_2_WORD_TYPE(t)) {
						M_ILD(REG_ITMP1, REG_SP, (parentargs_base + s1) * 4 + 4);
						M_IST(REG_ITMP1, REG_SP, var->regoff * 4 + 4);
					}
				}
			}

		} else {                                     /* floating args         */
 			if (!md->params[p].inmemory) {           /* register arguments    */
				s2 = rd->argfltregs[s1];
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
 					M_FLTMOVE(s2, var->regoff);

 				} else {			                 /* reg arg -> spilled    */
					if (IS_2_WORD_TYPE(t))
						M_DST(s2, REG_SP, var->regoff * 4);
					else
						M_FST(s2, REG_SP, var->regoff * 4);
 				}

 			} else {                                 /* stack arguments       */
 				if (!(var->flags & INMEMORY)) {      /* stack-arg -> register */
					if (IS_2_WORD_TYPE(t))
						M_DLD(var->regoff, REG_SP, (parentargs_base + s1) * 4);

					else
						M_FLD(var->regoff, REG_SP, (parentargs_base + s1) * 4);

 				} else {                             /* stack-arg -> spilled  */
					if (IS_2_WORD_TYPE(t)) {
						M_DLD(REG_FTMP1, REG_SP, (parentargs_base + s1) * 4);
						M_DST(REG_FTMP1, REG_SP, var->regoff * 4);

					} else {
						M_FLD(REG_FTMP1, REG_SP, (parentargs_base + s1) * 4);
						M_FST(REG_FTMP1, REG_SP, var->regoff * 4);
					}
				}
			}
		}
	} /* end for */

	/* save monitorenter argument */

#if defined(USE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		/* stack offset for monitor argument */

		s1 = rd->maxmemuse;

#if 0
		if (runverbose) {
			M_LDA(REG_SP, REG_SP, -(INT_ARG_CNT * 4 + FLT_ARG_CNT * 8));

			for (p = 0; p < INT_ARG_CNT; p++)
				M_IST(rd->argintregs[p], REG_SP, p * 4);

			for (p = 0; p < FLT_ARG_CNT * 2; p += 2)
				M_DST(rd->argfltregs[p], REG_SP, (INT_ARG_CNT + p) * 4);

			s1 += INT_ARG_CNT + FLT_ARG_CNT;
		}
#endif

		/* decide which monitor enter function to call */

		if (m->flags & ACC_STATIC) {
			p = dseg_addaddress(cd, m->class);
			M_ALD(REG_ITMP1, REG_PV, p);
			M_AST(REG_ITMP1, REG_SP, s1 * 4);
			M_MOV(REG_ITMP1, rd->argintregs[0]);
			p = dseg_addaddress(cd, BUILTIN_staticmonitorenter);
			M_ALD(REG_ITMP3, REG_PV, p);
			M_MTCTR(REG_ITMP3);
			M_JSR;

		} else {
			M_TST(rd->argintregs[0]);
			M_BEQ(0);
			codegen_addxnullrefs(cd, mcodeptr);
			M_AST(rd->argintregs[0], REG_SP, s1 * 4);
			p = dseg_addaddress(cd, BUILTIN_monitorenter);
			M_ALD(REG_ITMP3, REG_PV, p);
			M_MTCTR(REG_ITMP3);
			M_JSR;
		}

#if 0
		if (runverbose) {
			for (p = 0; p < INT_ARG_CNT; p++)
				M_ILD(rd->argintregs[p], REG_SP, p * 4);

			for (p = 0; p < FLT_ARG_CNT; p++)
				M_DLD(rd->argfltregs[p], REG_SP, (INT_ARG_CNT + p) * 4);


			M_LDA(REG_SP, REG_SP, (INT_ARG_CNT + FLT_ARG_CNT) * 8);
		}
#endif
	}
#endif

	/* call trace function */

	if (runverbose) {
		s4 longargs = 0;
		s4 fltargs = 0;
		s4 dblargs = 0;

		M_MFLR(REG_ITMP3);
		/* XXX must be a multiple of 16 */
		M_LDA(REG_SP, REG_SP, -(LA_SIZE + (INT_ARG_CNT + FLT_ARG_CNT + 1) * 8));

		M_IST(REG_ITMP3, REG_SP, LA_SIZE + (INT_ARG_CNT + FLT_ARG_CNT) * 8);

		M_CLR(REG_ITMP1);    /* clear help register */

		/* save all arguments into the reserved stack space */

		for (p = 0; p < md->paramcount && p < TRACE_ARGS_NUM; p++) {
			t = md->paramtypes[p].type;

			if (IS_INT_LNG_TYPE(t)) {
				/* overlapping u8's are on the stack */
				if ((p + longargs + dblargs) < (INT_ARG_CNT - IS_2_WORD_TYPE(t))) {
					s1 = rd->argintregs[p + longargs + dblargs];

					if (!IS_2_WORD_TYPE(t)) {
						M_IST(REG_ITMP1, REG_SP, LA_SIZE + p * 8);
						M_IST(s1, REG_SP, LA_SIZE + p * 8 + 4);

					} else {
						M_IST(s1, REG_SP, LA_SIZE + p  * 8);
						M_IST(rd->secondregs[s1], REG_SP, LA_SIZE + p * 8 + 4);
						longargs++;
					}

				} else {
					a = dseg_adds4(cd, 0xdeadbeef);
					M_ILD(REG_ITMP1, REG_PV, a);
					M_IST(REG_ITMP1, REG_SP, LA_SIZE + p * 8);
					M_IST(REG_ITMP1, REG_SP, LA_SIZE + p * 8 + 4);
				}

			} else {
				if ((fltargs + dblargs) < FLT_ARG_CNT) {
					s1 = rd->argfltregs[fltargs + dblargs];

					if (!IS_2_WORD_TYPE(t)) {
						M_IST(REG_ITMP1, REG_SP, LA_SIZE + p * 8);
						M_FST(s1, REG_SP, LA_SIZE + p * 8 + 4);
						fltargs++;
						
					} else {
						M_DST(s1, REG_SP, LA_SIZE + p * 8);
						dblargs++;
					}

				} else {
					/* this should not happen */
				}
			}
		}

		/* load first 4 arguments into integer argument registers */

		for (p = 0; p < 8; p++) {
			d = rd->argintregs[p];
			M_ILD(d, REG_SP, LA_SIZE + p * 4);
		}

		p = dseg_addaddress(cd, m);
		M_ALD(REG_ITMP1, REG_PV, p);
#if defined(__DARWIN__)
		M_AST(REG_ITMP1, REG_SP, LA_SIZE + 8 * 8); /* 24 (linkage area) + 32 (4 * s8 parameter area regs) + 32 (4 * s8 parameter area stack) = 88 */
#else
		M_AST(REG_ITMP1, REG_SP, LA_SIZE + 4 * 8);
#endif
		p = dseg_addaddress(cd, (void *) builtin_trace_args);
		M_ALD(REG_ITMP2, REG_PV, p);
		M_MTCTR(REG_ITMP2);
		M_JSR;

		longargs = 0;
		fltargs = 0;
		dblargs = 0;

		/* restore arguments from the reserved stack space */

		for (p = 0; p < md->paramcount && p < TRACE_ARGS_NUM; p++) {
			t = md->paramtypes[p].type;

			if (IS_INT_LNG_TYPE(t)) {
				if ((p + longargs + dblargs) < INT_ARG_CNT) {
					s1 = rd->argintregs[p + longargs + dblargs];

					if (!IS_2_WORD_TYPE(t)) {
						M_ILD(s1, REG_SP, LA_SIZE + p * 8 + 4);

					} else {
						M_ILD(s1, REG_SP, LA_SIZE + p * 8);
						M_ILD(rd->secondregs[s1], REG_SP, LA_SIZE + p * 8 + 4);
						longargs++;
					}
				}

			} else {
				if ((fltargs + dblargs) < FLT_ARG_CNT) {
					s1 = rd->argfltregs[fltargs + dblargs];

					if (!IS_2_WORD_TYPE(t)) {
						M_FLD(s1, REG_SP, LA_SIZE + p * 8 + 4);
						fltargs++;

					} else {
						M_DLD(s1, REG_SP, LA_SIZE + p * 8);
						dblargs++;
					}
				}
			}
		}

		M_ILD(REG_ITMP3, REG_SP, LA_SIZE + (INT_ARG_CNT + FLT_ARG_CNT) * 8);

		M_LDA(REG_SP, REG_SP, LA_SIZE + (INT_ARG_CNT + FLT_ARG_CNT + 1) * 8);
		M_MTLR(REG_ITMP3);
	}

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
				}
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

			MCODECHECK(64);   /* an instruction usually needs < 64 words      */

			switch (iptr->opc) {
			case ICMD_NOP:    /* ...  ==> ...                                 */
			case ICMD_INLINE_START:
			case ICMD_INLINE_END:
				break;

		case ICMD_CHECKNULL:  /* ..., objectref  ==> ..., objectref           */

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
			} else {
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
			} else {
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
			} else {
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
			} else {
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
			} else {
				ICONST(REG_ITMP2, s3);
				M_AND(s1, REG_ITMP2, rd->secondregs[d]);
			}
			var_to_reg_int0(s1, src, REG_ITMP1, 1, 0);
			s3 = iptr->val.l >> 32;
			if ((s3 >= 0) && (s3 <= 65535)) {
				M_AND_IMM(s1, s3, d);
			} else {
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
			} else
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
				M_ILD(s1, REG_SP, var->regoff * 4);
			} else
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
				M_IST(s1, REG_SP, var->regoff * 4);
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
			M_ILD(rd->secondregs[d], REG_ITMP2, OFFSET(java_longarray, data[0]) + 4);
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
			M_IADD_IMM(REG_ITMP2, OFFSET(java_longarray, data[0]), REG_ITMP2);
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


		case ICMD_GETSTATIC:  /* ...  ==> ..., value                          */
		                      /* op1 = type, val.a = field address            */

			if (!iptr->val.a) {
				codegen_addpatchref(cd, mcodeptr,
									PATCHER_get_putstatic,
									(unresolved_field *) iptr->target);

				if (showdisassemble)
					M_NOP;

				a = 0;

			} else {
				fieldinfo *fi = iptr->val.a;

				if (!fi->class->initialized) {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_clinit, fi->class);

					if (showdisassemble)
						M_NOP;
				}

				a = (ptrint) &(fi->value);
  			}

			a = dseg_addaddress(cd, a);
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
			}
			break;

		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */
		                      /* op1 = type, val.a = field address            */


			if (!iptr->val.a) {
				codegen_addpatchref(cd, mcodeptr,
									PATCHER_get_putstatic,
									(unresolved_field *) iptr->target);

				if (showdisassemble)
					M_NOP;

				a = 0;

			} else {
				fieldinfo *fi = iptr->val.a;

				if (!fi->class->initialized) {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_clinit, fi->class);

					if (showdisassemble)
						M_NOP;
				}

				a = (ptrint) &(fi->value);
  			}

			a = dseg_addaddress(cd, a);
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
			}
			break;


		case ICMD_GETFIELD:   /* ...  ==> ..., value                          */
		                      /* op1 = type, val.i = field offset             */

			var_to_reg_int(s1, src, REG_ITMP1);
			gen_nullptr_check(s1);

			if (!iptr->val.a) {
				codegen_addpatchref(cd, mcodeptr,
									PATCHER_get_putfield,
									(unresolved_field *) iptr->target);

				if (showdisassemble)
					M_NOP;

				a = 0;

			} else {
				a = ((fieldinfo *) (iptr->val.a))->offset;
			}

			switch (iptr->op1) {
			case TYPE_INT:
				d = reg_of_var(rd, iptr->dst, REG_ITMP3);
				M_ILD(d, s1, a);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_LNG:
				d = reg_of_var(rd, iptr->dst, REG_ITMP3);
				M_ILD(d, s1, a);
				M_ILD(rd->secondregs[d], s1, a + 4);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_ADR:
				d = reg_of_var(rd, iptr->dst, REG_ITMP3);
				M_ALD(d, s1, a);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_FLT:
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				M_FLD(d, s1, a);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			case TYPE_DBL:				
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				M_DLD(d, s1, a);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			}
			break;

		case ICMD_PUTFIELD:   /* ..., value  ==> ...                          */
		                      /* op1 = type, val.i = field offset             */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			gen_nullptr_check(s1);

			if (!IS_FLT_DBL_TYPE(iptr->op1)) {
				var_to_reg_int(s2, src, REG_ITMP2);
			} else {
				var_to_reg_flt(s2, src, REG_FTMP2);
			}

			if (!iptr->val.a) {
				codegen_addpatchref(cd, mcodeptr,
									PATCHER_get_putfield,
									(unresolved_field *) iptr->target);

				if (showdisassemble)
					M_NOP;

				a = 0;

			} else {
				a = ((fieldinfo *) (iptr->val.a))->offset;
			}

			switch (iptr->op1) {
			case TYPE_INT:
				M_IST(s2, s1, a);
				break;
			case TYPE_LNG:
				M_IST(s2, s1, a);
				M_IST(rd->secondregs[s2], s1, a + 4);
				break;
			case TYPE_ADR:
				M_AST(s2, s1, a);
				break;
			case TYPE_FLT:
				M_FST(s2, s1, a);
				break;
			case TYPE_DBL:
				M_DST(s2, s1, a);
				break;
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
			} else
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
				var_to_reg_int(s1, src, REG_RESULT);
				M_TINTMOVE(src->type, s1, REG_RESULT);
				goto nowperformreturn;

		case ICMD_FRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_DRETURN:
				var_to_reg_flt(s1, src, REG_FRESULT);
				M_FLTMOVE(s1, REG_FRESULT);
				goto nowperformreturn;

		case ICMD_RETURN:      /* ...  ==> ...                                */

nowperformreturn:
			{
			s4 i, p;
			
			p = parentargs_base;

			/* call trace function */

			if (runverbose) {
				M_MFLR(REG_ITMP3);
				M_LDA(REG_SP, REG_SP, -10 * 8);
				M_DST(REG_FRESULT, REG_SP, 48+0);
				M_IST(REG_RESULT, REG_SP, 48+8);
				M_AST(REG_ITMP3, REG_SP, 48+12);
				M_IST(REG_RESULT2, REG_SP, 48+16);

				/* keep this order */
				switch (iptr->opc) {
				case ICMD_IRETURN:
				case ICMD_ARETURN:
#if defined(__DARWIN__)
					M_MOV(REG_RESULT, rd->argintregs[2]);
					M_CLR(rd->argintregs[1]);
#else
					M_MOV(REG_RESULT, rd->argintregs[3]);
					M_CLR(rd->argintregs[2]);
#endif
					break;

				case ICMD_LRETURN:
#if defined(__DARWIN__)
					M_MOV(REG_RESULT2, rd->argintregs[2]);
					M_MOV(REG_RESULT, rd->argintregs[1]);
#else
					M_MOV(REG_RESULT2, rd->argintregs[3]);
					M_MOV(REG_RESULT, rd->argintregs[2]);
#endif
					break;
				}

				a = dseg_addaddress(cd, m);
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
			
#if defined(USE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				/* we need to save the proper return value */
				switch (iptr->opc) {
				case ICMD_IRETURN:
				case ICMD_ARETURN:
					M_IST(REG_RESULT , REG_SP, rd->maxmemuse * 4 + 4);
				case ICMD_LRETURN:
					M_IST(REG_RESULT2, REG_SP, rd->maxmemuse * 4 + 8);
					break;
				case ICMD_FRETURN:
					M_FST(REG_FRESULT, REG_SP, rd->maxmemuse * 4 + 4);
					break;
				case ICMD_DRETURN:
					M_DST(REG_FRESULT, REG_SP, rd->maxmemuse * 4 + 4);
					break;
				}

				a = dseg_addaddress(cd, BUILTIN_monitorexit);
				M_ALD(REG_ITMP3, REG_PV, a);
				M_MTCTR(REG_ITMP3);
				M_ALD(rd->argintregs[0], REG_SP, rd->maxmemuse * 4);
				M_JSR;

				/* and now restore the proper return value */
				switch (iptr->opc) {
				case ICMD_IRETURN:
				case ICMD_ARETURN:
					M_ILD(REG_RESULT , REG_SP, rd->maxmemuse * 4 + 4);
				case ICMD_LRETURN:
					M_ILD(REG_RESULT2, REG_SP, rd->maxmemuse * 4 + 8);
					break;
				case ICMD_FRETURN:
					M_FLD(REG_FRESULT, REG_SP, rd->maxmemuse * 4 + 4);
					break;
				case ICMD_DRETURN:
					M_DLD(REG_FRESULT, REG_SP, rd->maxmemuse * 4 + 4);
					break;
				}
			}
#endif

			/* restore return address                                         */

			if (!m->isleafmethod) {
				M_ALD(REG_ITMP3, REG_SP, p * 4 + LA_LR_OFFSET);
				M_MTLR(REG_ITMP3);
			}

			/* restore saved registers                                        */

			for (i = rd->savintregcnt - 1; i >= rd->maxsavintreguse; i--) {
				p--; M_ILD(rd->savintregs[i], REG_SP, p * 4);
			}
			for (i = rd->savfltregcnt - 1; i >= rd->maxsavfltreguse; i--) {
				p -= 2; M_DLD(rd->savfltregs[i], REG_SP, p * 4);
			}

			/* deallocate stack                                               */

			if (parentargs_base)
				M_LDA(REG_SP, REG_SP, parentargs_base * 4);

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


		case ICMD_BUILTIN:      /* ..., [arg1, [arg2 ...]] ==> ...            */
		                        /* op1 = arg count val.a = builtintable entry */

			bte = iptr->val.a;
			md = bte->md;
			goto gen_method;

		case ICMD_INVOKESTATIC: /* ..., [arg1, [arg2 ...]] ==> ...            */
		                        /* op1 = arg count, val.a = method pointer    */

		case ICMD_INVOKESPECIAL:/* ..., objectref, [arg1, [arg2 ...]] ==> ... */
		case ICMD_INVOKEVIRTUAL:/* op1 = arg count, val.a = method pointer    */
		case ICMD_INVOKEINTERFACE:

			lm = iptr->val.a;

			if (lm)
				md = lm->parseddesc;
			else {
				unresolved_method *um = iptr->target;
				md = um->methodref->parseddesc.md;
			}

gen_method:
			s3 = iptr->op1;

			MCODECHECK((s3 << 1) + 64);

			/* copy arguments to registers or stack location */

			for (s3 = s3 - 1; s3 >= 0; s3--, src = src->prev) {
				if (src->varkind == ARGVAR)
					continue;
				if (IS_INT_LNG_TYPE(src->type)) {
					if (!md->params[s3].inmemory) {
						s1 = rd->argintregs[md->params[s3].regoff];
						var_to_reg_int(d, src, s1);
						M_TINTMOVE(src->type, d, s1);
					} else {
						var_to_reg_int(d, src, REG_ITMP1);
						M_IST(d, REG_SP, md->params[s3].regoff * 4);
						if (IS_2_WORD_TYPE(src->type))
							M_IST(rd->secondregs[d], 
								  REG_SP, md->params[s3].regoff * 4 + 4);
					}
						
				} else {
					if (!md->params[s3].inmemory) {
						s1 = rd->argfltregs[md->params[s3].regoff];
						var_to_reg_flt(d, src, s1);
						M_FLTMOVE(d, s1);
					} else {
						var_to_reg_flt(d, src, REG_FTMP1);
						if (IS_2_WORD_TYPE(src->type))
							M_DST(d, REG_SP, md->params[s3].regoff * 4);
						else
							M_FST(d, REG_SP, md->params[s3].regoff * 4);
					}
				}
			} /* end of for */

			switch (iptr->opc) {
			case ICMD_BUILTIN:
				if (iptr->target) {
					codegen_addpatchref(cd, mcodeptr, bte->fp, iptr->target);

					if (showdisassemble)
						M_NOP;

					a = 0;

				} else {
					a = (ptrint) bte->fp;
				}

				a = dseg_addaddress(cd, a);
				d = md->returntype.type;

				M_ALD(REG_PV, REG_PV, a); /* Pointer to built-in-function */
				break;

			case ICMD_INVOKESPECIAL:
				gen_nullptr_check(rd->argintregs[0]);
				M_ILD(REG_ITMP1, rd->argintregs[0], 0); /* hardware nullptr   */
				/* fall through */

			case ICMD_INVOKESTATIC:
				if (!lm) {
					unresolved_method *um = iptr->target;

					codegen_addpatchref(cd, mcodeptr,
										PATCHER_invokestatic_special, um);

					if (showdisassemble)
						M_NOP;

					a = 0;
					d = md->returntype.type;

				} else {
					a = (ptrint) lm->stubroutine;
					d = md->returntype.type;
				}

				a = dseg_addaddress(cd, a);
				M_ALD(REG_PV, REG_PV, a);       /* method pointer in r27 */
				break;

			case ICMD_INVOKEVIRTUAL:
				gen_nullptr_check(rd->argintregs[0]);

				if (!lm) {
					unresolved_method *um = iptr->target;

					codegen_addpatchref(cd, mcodeptr,
										PATCHER_invokevirtual, um);

					if (showdisassemble)
						M_NOP;

					s1 = 0;
					d = md->returntype.type;

				} else {
					s1 = OFFSET(vftbl_t, table[0]) +
						sizeof(methodptr) * lm->vftblindex;
					d = md->returntype.type;
				}

				M_ALD(REG_METHODPTR, rd->argintregs[0],
					  OFFSET(java_objectheader, vftbl));
				M_ALD(REG_PV, REG_METHODPTR, s1);
				break;

			case ICMD_INVOKEINTERFACE:
				gen_nullptr_check(rd->argintregs[0]);

				if (!lm) {
					unresolved_method *um = iptr->target;

					codegen_addpatchref(cd, mcodeptr,
										PATCHER_invokeinterface, um);

					if (showdisassemble)
						M_NOP;

					s1 = 0;
					s2 = 0;
					d = md->returntype.type;

				} else {
					s1 = OFFSET(vftbl_t, interfacetable[0]) -
						sizeof(methodptr*) * lm->class->index;

					s2 = sizeof(methodptr) * (lm - lm->class->methods);

					d = md->returntype.type;
				}

				M_ALD(REG_METHODPTR, rd->argintregs[0],
					  OFFSET(java_objectheader, vftbl));    
				M_ALD(REG_METHODPTR, REG_METHODPTR, s1);
				M_ALD(REG_PV, REG_METHODPTR, s2);
				break;
			}

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
			break;


		case ICMD_CHECKCAST:  /* ..., objectref ==> ..., objectref            */

		                      /* op1:   0 == array, 1 == class                */
		                      /* val.a: (classinfo*) superclass               */

			/*  superclass is an interface:
			 *
			 *  OK if ((sub == NULL) ||
			 *         (sub->vftbl->interfacetablelength > super->index) &&
			 *         (sub->vftbl->interfacetable[-super->index] != NULL));
			 *
			 *  superclass is a class:
			 *
			 *  OK if ((sub == NULL) || (0
			 *         <= (sub->vftbl->baseval - super->vftbl->baseval) <=
			 *         super->vftbl->diffvall));
			 */

			{
			classinfo *super;
			vftbl_t   *supervftbl;
			s4         superindex;

			super = (classinfo *) iptr->val.a;

			if (!super) {
				superindex = 0;
				supervftbl = NULL;

			} else {
				superindex = super->index;
				supervftbl = super->vftbl;
			}
			
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			codegen_threadcritrestart(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
			var_to_reg_int(s1, src, REG_ITMP1);

			/* calculate interface checkcast code size */

			s2 = 7;
			if (!super)
				s2 += (showdisassemble ? 1 : 0);

			/* calculate class checkcast code size */

			s3 = 8 + (s1 == REG_ITMP1);
			if (!super)
				s3 += (showdisassemble ? 1 : 0);

			/* if class is not resolved, check which code to call */

			if (!super) {
				M_TST(s1);
				M_BEQ(3 + (showdisassemble ? 1 : 0) + s2 + 1 + s3);

				codegen_addpatchref(cd, mcodeptr,
									PATCHER_checkcast_instanceof_flags,
									(constant_classref *) iptr->target);

				if (showdisassemble)
					M_NOP;

				a = dseg_adds4(cd, 0); /* super->flags */
				M_ILD(REG_ITMP2, REG_PV, a);
				M_AND_IMM(REG_ITMP2, ACC_INTERFACE, REG_ITMP2);
				M_BEQ(s2 + 1);
			}

			/* interface checkcast code */

			if (!super || (super->flags & ACC_INTERFACE)) {
				if (super) {
					M_TST(s1);
					M_BEQ(s2);

				} else {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_checkcast_instanceof_interface,
										(constant_classref *) iptr->target);

					if (showdisassemble)
						M_NOP;
				}

				M_ALD(REG_ITMP2, s1, OFFSET(java_objectheader, vftbl));
				M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl_t, interfacetablelength));
				M_LDATST(REG_ITMP3, REG_ITMP3, -superindex);
				M_BLE(0);
				codegen_addxcastrefs(cd, mcodeptr);
				M_ALD(REG_ITMP3, REG_ITMP2,
					OFFSET(vftbl_t, interfacetable[0]) -
					superindex * sizeof(methodptr*));
				M_TST(REG_ITMP3);
				M_BEQ(0);
				codegen_addxcastrefs(cd, mcodeptr);

				if (!super)
					M_BR(s3);
			}

			/* class checkcast code */

			if (!super || !(super->flags & ACC_INTERFACE)) {
				if (super) {
					M_TST(s1);
					M_BEQ(s3);

				} else {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_checkcast_class,
										(constant_classref *) iptr->target);

					if (showdisassemble)
						M_NOP;
				}

				M_ALD(REG_ITMP2, s1, OFFSET(java_objectheader, vftbl));
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
				codegen_threadcritstart(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
				M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl_t, baseval));
				a = dseg_addaddress(cd, supervftbl);
				M_ALD(REG_ITMP2, REG_PV, a);
				if (s1 != REG_ITMP1) {
					M_ILD(REG_ITMP1, REG_ITMP2, OFFSET(vftbl_t, baseval));
					M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, diffval));
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
					codegen_threadcritstop(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
					M_ISUB(REG_ITMP3, REG_ITMP1, REG_ITMP3);
				} else {
					M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, baseval));
					M_ISUB(REG_ITMP3, REG_ITMP2, REG_ITMP3);
					M_ALD(REG_ITMP2, REG_PV, a);
					M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, diffval));
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
					codegen_threadcritstop(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
				}
				M_CMPU(REG_ITMP3, REG_ITMP2);
				M_BGT(0);
				codegen_addxcastrefs(cd, mcodeptr);
			}
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			}
			break;

		case ICMD_INSTANCEOF: /* ..., objectref ==> ..., intresult            */

		                      /* op1:   0 == array, 1 == class                */
		                      /* val.a: (classinfo*) superclass               */

			/*  superclass is an interface:
			 *
			 *  return (sub != NULL) &&
			 *         (sub->vftbl->interfacetablelength > super->index) &&
			 *         (sub->vftbl->interfacetable[-super->index] != NULL);
			 *
			 *  superclass is a class:
			 *
			 *  return ((sub != NULL) && (0
			 *          <= (sub->vftbl->baseval - super->vftbl->baseval) <=
			 *          super->vftbl->diffvall));
			 */

			{
			classinfo *super;
			vftbl_t   *supervftbl;
			s4         superindex;

			super = (classinfo *) iptr->val.a;

			if (!super) {
				superindex = 0;
				supervftbl = NULL;

			} else {
				superindex = super->index;
				supervftbl = super->vftbl;
			}
			
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
            codegen_threadcritrestart(cd, (u1*) mcodeptr - cd->mcodebase);
#endif
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (s1 == d) {
				M_MOV(s1, REG_ITMP1);
				s1 = REG_ITMP1;
			}

			/* calculate interface instanceof code size */

			s2 = 9;
			if (!super)
				s2 += (showdisassemble ? 1 : 0);

			/* calculate class instanceof code size */

			s3 = 10;
			if (!super)
				s3 += (showdisassemble ? 1 : 0);

			M_CLR(d);

			/* if class is not resolved, check which code to call */

			if (!super) {
				M_TST(s1);
				M_BEQ(3 + (showdisassemble ? 1 : 0) + s2 + 1 + s3);

				codegen_addpatchref(cd, mcodeptr,
									PATCHER_checkcast_instanceof_flags,
									(constant_classref *) iptr->target);

				if (showdisassemble)
					M_NOP;

				a = dseg_adds4(cd, 0); /* super->flags */
				M_ILD(REG_ITMP2, REG_PV, a);
				M_AND_IMM(REG_ITMP2, ACC_INTERFACE, REG_ITMP2);
				M_BEQ(s2 + 1);
			}

			/* interface instanceof code */

			if (!super || (super->flags & ACC_INTERFACE)) {
				if (super) {
					M_TST(s1);
					M_BEQ(9);

				} else {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_checkcast_instanceof_interface,
										(constant_classref *) iptr->target);

					if (showdisassemble)
						M_NOP;
				}

				M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
				M_ILD(REG_ITMP2, REG_ITMP1, OFFSET(vftbl_t, interfacetablelength));
				M_LDATST(REG_ITMP2, REG_ITMP2, -superindex);
				M_BLE(5);
				M_ALD(REG_ITMP1, REG_ITMP1,
					OFFSET(vftbl_t, interfacetable[0]) -
					superindex * sizeof(methodptr*));
				M_TST(REG_ITMP1);
				M_CLR(d);
				M_BEQ(1);
				M_IADD_IMM(REG_ZERO, 1, d);

				if (!super)
					M_BR(s3);
			}

			/* class instanceof code */

			if (!super || !(super->flags & ACC_INTERFACE)) {
				if (super) {
					M_TST(s1);
					M_BEQ(s3);

				} else {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_instanceof_class,
										(constant_classref *) iptr->target);

					if (showdisassemble) {
						M_NOP;
					}
				}

				M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
				a = dseg_addaddress(cd, supervftbl);
				M_ALD(REG_ITMP2, REG_PV, a);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
				codegen_threadcritstart(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
				M_ILD(REG_ITMP1, REG_ITMP1, OFFSET(vftbl_t, baseval));
				M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl_t, baseval));
				M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, diffval));
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
				codegen_threadcritstop(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
				M_ISUB(REG_ITMP1, REG_ITMP3, REG_ITMP1);
				M_CMPU(REG_ITMP1, REG_ITMP2);
				M_CLR(d);
				M_BGT(1);
				M_IADD_IMM(REG_ZERO, 1, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			}
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

				if (src->varkind != ARGVAR)
#if defined(__DARWIN__)
					M_IST(s2, REG_SP, LA_SIZE + (s1 + INT_ARG_CNT) * 4);
#else
					M_IST(s2, REG_SP, LA_SIZE + (s1 + 3) * 4);
#endif
			}

			/* is patcher function set? */

			if (iptr->target) {
				codegen_addpatchref(cd, mcodeptr,
									(functionptr) iptr->target, iptr->val.a);

				if (showdisassemble)
					M_NOP;

				a = 0;

			} else {
				a = (ptrint) iptr->val.a;
			}

			/* a0 = dimension count */

			ICONST(rd->argintregs[0], iptr->op1);

			/* a1 = arraydescriptor */

			a = dseg_addaddress(cd, iptr->val.a);
			M_ALD(rd->argintregs[1], REG_PV, a);

			/* a2 = pointer to dimensions = stack pointer */

#if defined(__DARWIN__)
			M_LDA(rd->argintregs[2], REG_SP, LA_SIZE + INT_ARG_CNT * 4);
#else
			M_LDA(rd->argintregs[2], REG_SP, LA_SIZE + 3 * 4);
#endif

			a = dseg_addaddress(cd, BUILTIN_multianewarray);
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


		default:
			throw_cacao_exception_exit(string_java_lang_InternalError,
									   "Unknown ICMD %d", iptr->opc);
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

	/* generate patcher stub call code */

	{
		patchref *pref;
		u4        mcode;
		s4       *tmpmcodeptr;

		for (pref = cd->patchrefs; pref != NULL; pref = pref->next) {
			/* check code segment size */

			MCODECHECK(16);

			/* Get machine code which is patched back in later. The call is   */
			/* 1 instruction word long.                                       */

			xcodeptr = (s4 *) (cd->mcodebase + pref->branchpos);
			mcode = *xcodeptr;

			/* patch in the call to call the following code (done at compile  */
			/* time)                                                          */

			tmpmcodeptr = mcodeptr;         /* save current mcodeptr          */
			mcodeptr = xcodeptr;            /* set mcodeptr to patch position */

			M_BL(tmpmcodeptr - (xcodeptr + 1));

			mcodeptr = tmpmcodeptr;         /* restore the current mcodeptr   */

			/* create stack frame - keep stack 16-byte aligned */

			M_AADD_IMM(REG_SP, -8 * 4, REG_SP);

			/* move return address onto stack */

			M_MFLR(REG_ITMP3);
			M_AST(REG_ITMP3, REG_SP, 4 * 4);

			/* move pointer to java_objectheader onto stack */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			/* order reversed because of data segment layout */

			(void) dseg_addaddress(cd, get_dummyLR());          /* monitorPtr */
			a = dseg_addaddress(cd, NULL);                      /* vftbl      */

			M_LDA(REG_ITMP3, REG_PV, a);
			M_AST(REG_ITMP3, REG_SP, 3 * 4);
#else
			M_CLR(REG_ITMP3);
			M_AST(REG_ITMP3, REG_SP, 3 * 4);
#endif

			/* move machine code onto stack */

			a = dseg_adds4(cd, mcode);
			M_ILD(REG_ITMP3, REG_PV, a);
			M_IST(REG_ITMP3, REG_SP, 2 * 4);

			/* move class/method/field reference onto stack */

			a = dseg_addaddress(cd, pref->ref);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_AST(REG_ITMP3, REG_SP, 1 * 4);

			/* move patcher function pointer onto stack */

			a = dseg_addaddress(cd, pref->patcher);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_AST(REG_ITMP3, REG_SP, 0 * 4);

			a = dseg_addaddress(cd, asm_wrapper_patcher);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_MTCTR(REG_ITMP3);
			M_RTS;
		}
	}

	}

	codegen_finish(m, cd, (ptrint) ((u1 *) mcodeptr - cd->mcodebase));

	asm_cacheflush((void *) m->entrypoint, ((u1 *) mcodeptr - cd->mcodebase));
}


/* createcompilerstub **********************************************************

   Creates a stub routine which calls the compiler.
	
*******************************************************************************/

#define COMPSTUBSIZE 6

functionptr createcompilerstub(methodinfo *m)
{
	s4 *s = CNEW(s4, COMPSTUBSIZE);     /* memory to hold the stub            */
	s4 *mcodeptr = s;                   /* code generation pointer            */

	M_LDA(REG_ITMP1, REG_PV, 4*4);
	M_ALD(REG_PV, REG_PV, 5*4);
	M_MTCTR(REG_PV);
	M_RTS;

	s[4] = (s4) m;                      /* literals to be adressed            */
	s[5] = (s4) asm_call_jit_compiler;  /* jump directly via PV from above    */

	asm_cacheflush((void *) s, (u1 *) mcodeptr - (u1 *) s);

#if defined(STATISTICS)
	if (opt_stat)
		count_cstub_len += COMPSTUBSIZE * 4;
#endif

	return (functionptr) (ptrint) s;
}


/* createnativestub ************************************************************

   Creates a stub routine which calls a native method.

*******************************************************************************/

functionptr createnativestub(functionptr f, methodinfo *m, codegendata *cd,
							 registerdata *rd)
{
	s4         *mcodeptr;               /* code generation pointer            */
	s4          stackframesize;         /* size of stackframe if needed       */
	s4          disp;
	methoddesc *md;
	methoddesc *nmd;
	s4          nativeparams;
	s4          i, j;                   /* count variables                    */
	s4          t;
	s4          s1, s2, off;

	/* set some variables */

	nativeparams = (m->flags & ACC_STATIC) ? 2 : 1;

	/* create new method descriptor with additional native parameters */

	md = m->parseddesc;
	
	nmd = (methoddesc *) DMNEW(u1, sizeof(methoddesc) - sizeof(typedesc) +
							   md->paramcount * sizeof(typedesc) +
							   nativeparams * sizeof(typedesc));

	nmd->paramcount = md->paramcount + nativeparams;

	nmd->params = DMNEW(paramdesc, nmd->paramcount);

	nmd->paramtypes[0].type = TYPE_ADR; /* add environment pointer            */

	if (m->flags & ACC_STATIC)
		nmd->paramtypes[1].type = TYPE_ADR; /* add class pointer              */

	MCOPY(nmd->paramtypes + nativeparams, md->paramtypes, typedesc,
		  md->paramcount);

	md_param_alloc(nmd);


	/* create method header */

	(void) dseg_addaddress(cd, m);                          /* MethodPointer  */
	(void) dseg_adds4(cd, 0 * 8);                           /* FrameSize      */
	(void) dseg_adds4(cd, 0);                               /* IsSync         */
	(void) dseg_adds4(cd, 0);                               /* IsLeaf         */
	(void) dseg_adds4(cd, 0);                               /* IntSave        */
	(void) dseg_adds4(cd, 0);                               /* FltSave        */
	(void) dseg_addlinenumbertablesize(cd);
	(void) dseg_adds4(cd, 0);                               /* ExTableSize    */


	/* initialize mcode variables */
	
	mcodeptr = (s4 *) cd->mcodebase;
	cd->mcodeend = (s4 *) (cd->mcodebase + cd->mcodesize);


	M_MFLR(REG_ITMP1);
	M_AST(REG_ITMP1, REG_SP, LA_LR_OFFSET); /* store return address           */

	/* Build up new Stackframe for native call */

	stackframesize = nmd->memuse;

	stackframesize = (stackframesize + 3) & ~3; /* Keep Stack 16 Byte aligned */
	M_STWU(REG_SP, REG_SP, -(stackframesize * 4)); /* build up stackframe     */

	/* if function is static, check for initialized */

	if ((m->flags & ACC_STATIC) && !m->class->initialized) {
		codegen_addpatchref(cd, mcodeptr, PATCHER_clinit, m->class);

		if (showdisassemble)
			M_NOP;
	}

	if (runverbose) {
		s4 longargs = 0;
		s4 fltargs = 0;
		s4 dblargs = 0;

		/* XXX must be a multiple of 16 */
		M_LDA(REG_SP, REG_SP, -(LA_SIZE + (INT_ARG_CNT + FLT_ARG_CNT + 1) * 8));

		M_CLR(REG_ITMP1);

		/* save all arguments into the reserved stack space */

		for (i = 0; i < md->paramcount && i < TRACE_ARGS_NUM; i++) {
			t = md->paramtypes[i].type;

			if (IS_INT_LNG_TYPE(t)) {
				/* overlapping u8's are on the stack */
				if ((i + longargs + dblargs) < (INT_ARG_CNT - IS_2_WORD_TYPE(t))) {
					s1 = rd->argintregs[i + longargs + dblargs];

					if (!IS_2_WORD_TYPE(t)) {
						M_IST(REG_ITMP1, REG_SP, LA_SIZE + i * 8);
						M_IST(s1, REG_SP, LA_SIZE + i * 8 + 4);
					} else {
						M_IST(s1, REG_SP, LA_SIZE + i  * 8);
						M_IST(rd->secondregs[s1], REG_SP, LA_SIZE + i * 8 + 4);
						longargs++;
					}

				} else {
					/* we do not have a data segment here */
					/* a = dseg_adds4(cd, 0xdeadbeef);
					M_ILD(REG_ITMP1, REG_PV, a); */
					M_LDA(REG_ITMP1, REG_ZERO, -1);
					M_IST(REG_ITMP1, REG_SP, LA_SIZE + i * 8);
					M_IST(REG_ITMP1, REG_SP, LA_SIZE + i * 8 + 4);
				}

			} else {
				if ((fltargs + dblargs) < FLT_ARG_CNT) {
					s1 = rd->argfltregs[fltargs + dblargs];

					if (!IS_2_WORD_TYPE(t)) {
						M_IST(REG_ITMP1, REG_SP, LA_SIZE + i * 8);
						M_FST(s1, REG_SP, LA_SIZE + i * 8 + 4);
						fltargs++;
					} else {
						M_DST(s1, REG_SP, LA_SIZE + i * 8);
						dblargs++;
					}

				} else {
					/* this should not happen */
				}
			}
		}

		/* TODO: save remaining integer and float argument registers */

		/* load first 4 arguments into integer argument registers */

		for (i = 0; i < INT_ARG_CNT; i++)
			M_ILD(rd->argintregs[i], REG_SP, LA_SIZE + i * 4);

		off = dseg_addaddress(cd, m);
		M_ALD(REG_ITMP1, REG_PV, off);
#if defined(__DARWIN__)
		/* 24 (linkage area) + 32 (4 * s8 parameter area regs) +              */
		/* 32 (4 * s8 parameter area stack) = 88                              */
		M_AST(REG_ITMP1, REG_SP, LA_SIZE + 8 * 8);
#else
		M_AST(REG_ITMP1, REG_SP, LA_SIZE + 4 * 8);
#endif
		off = dseg_addaddress(cd, builtin_trace_args);
		M_ALD(REG_ITMP2, REG_PV, off);
		M_MTCTR(REG_ITMP2);
		M_JSR;

		longargs = 0;
		fltargs = 0;
		dblargs = 0;

		/* restore arguments into the reserved stack space */

		for (i = 0; i < md->paramcount && i < TRACE_ARGS_NUM; i++) {
			t = md->paramtypes[i].type;

			if (IS_INT_LNG_TYPE(t)) {
				if ((i + longargs + dblargs) < INT_ARG_CNT) {
					s1 = rd->argintregs[i + longargs + dblargs];

					if (!IS_2_WORD_TYPE(t)) {
						M_ILD(s1, REG_SP, LA_SIZE + i * 8 + 4);
					} else {
						M_ILD(s1, REG_SP, LA_SIZE + i * 8);
						M_ILD(rd->secondregs[s1], REG_SP, LA_SIZE + i * 8 + 4);
						longargs++;
					}
				}

			} else {
				if ((fltargs + dblargs) < FLT_ARG_CNT) {
					s1 = rd->argfltregs[fltargs + dblargs];

					if (!IS_2_WORD_TYPE(t)) {
						M_FLD(s1, REG_SP, LA_SIZE + i * 8 + 4);
						fltargs++;
					} else {
						M_DLD(s1, REG_SP, LA_SIZE + i * 8);
						dblargs++;
					}
				}
			}
		}

		M_LDA(REG_SP, REG_SP, LA_SIZE + (INT_ARG_CNT + FLT_ARG_CNT + 1) * 8);
	}

	/* copy or spill arguments to new locations */

	for (i = md->paramcount - 1, j = i + nativeparams; i >= 0; i--, j--) {
		t = md->paramtypes[i].type;

		if (IS_INT_LNG_TYPE(t)) {
			if (!md->params[i].inmemory) {
				s1 = rd->argintregs[md->params[i].regoff];

				if (!nmd->params[j].inmemory) {
					s2 = rd->argintregs[nmd->params[j].regoff];
					M_TINTMOVE(t, s1, s2);

				} else {
					s2 = nmd->params[j].regoff;

					M_IST(s1, REG_SP, s2 * 4);
					if (IS_2_WORD_TYPE(t))
						M_IST(rd->secondregs[s1], REG_SP, s2 * 4 + 4);
				}

			} else {
				s1 = md->params[i].regoff + stackframesize;
				s2 = nmd->params[j].regoff;

				M_ILD(REG_ITMP1, REG_SP, s1 * 4);
				M_IST(REG_ITMP1, REG_SP, s2 * 4);
				if (IS_2_WORD_TYPE(t)) {
					M_ILD(REG_ITMP1, REG_SP, s1 * 4 + 4);
					M_IST(REG_ITMP1, REG_SP, s2 * 4 + 4);
				}
			}

		} else {
			/* We only copy spilled float arguments, as the float argument    */
			/* registers keep unchanged.                                      */

			if (md->params[i].inmemory) {
				s1 = md->params[i].regoff + stackframesize;
				s2 = nmd->params[j].regoff;

				if (IS_2_WORD_TYPE(t)) {
					M_DLD(REG_FTMP1, REG_SP, s1 * 4);
					M_DST(REG_FTMP1, REG_SP, s2 * 4);

				} else {
					M_FLD(REG_FTMP1, REG_SP, s1 * 4);
					M_FST(REG_FTMP1, REG_SP, s2 * 4);
				}
			}
		}
	}

	/* put class into second argument register */

	if (m->flags & ACC_STATIC) {
		off = dseg_addaddress(cd, m->class);
		M_ALD(rd->argintregs[1], REG_PV, off);
	}

	/* put env into first argument register */

	off = dseg_addaddress(cd, &env);
	M_ALD(rd->argintregs[0], REG_PV, off);

	/* generate the actual native call */

#if !defined(STATIC_CLASSPATH)
	if (f == NULL) {
		codegen_addpatchref(cd, mcodeptr, PATCHER_resolve_native, m);

		if (showdisassemble)
			M_NOP;
	}
#endif

	off = dseg_addaddress(cd, f);
	M_ALD(REG_PV, REG_PV, off);
	M_MTCTR(REG_PV);
	M_JSR;
	disp = (s4) ((u1 *) mcodeptr - cd->mcodebase);
	M_MFLR(REG_ITMP1);
	M_LDA(REG_PV, REG_ITMP1, -disp);    /* recompute pv from ra               */

	/* 16 instructions */

	if (runverbose) {
#if defined(__DARWIN__)
		M_LDA(REG_SP, REG_SP, -(LA_SIZE + ((1 + 2 + 2 + 1) + 2 + 2) * 4));
		M_IST(REG_RESULT, REG_SP, LA_SIZE + ((1 + 2 + 2 + 1) + 0) * 4);
		M_IST(REG_RESULT2, REG_SP, LA_SIZE + ((1 + 2 + 2 + 1) + 1) * 4);
		M_DST(REG_FRESULT, REG_SP, LA_SIZE + ((1 + 2 + 2 + 1) + 2) * 4);
#else
		M_LDA(REG_SP, REG_SP, -(LA_SIZE + (2 + 2) * 4));
		M_IST(REG_RESULT, REG_SP, LA_SIZE + 0 * 4);
		M_IST(REG_RESULT2, REG_SP, LA_SIZE + 1 * 4);
		M_DST(REG_FRESULT, REG_SP, LA_SIZE + 2 * 4);
#endif

		/* keep this order */
		switch (m->returntype) {
		case TYPE_INT:
		case TYPE_ADDRESS:
#if defined(__DARWIN__)
			M_MOV(REG_RESULT, rd->argintregs[2]);
			M_CLR(rd->argintregs[1]);
#else
			M_MOV(REG_RESULT, rd->argintregs[3]);
			M_CLR(rd->argintregs[2]);
#endif
			break;

		case TYPE_LONG:
#if defined(__DARWIN__)
			M_MOV(REG_RESULT2, rd->argintregs[2]);
			M_MOV(REG_RESULT, rd->argintregs[1]);
#else
			M_MOV(REG_RESULT2, rd->argintregs[3]);
			M_MOV(REG_RESULT, rd->argintregs[2]);
#endif
			break;
		}

		M_FLTMOVE(REG_FRESULT, rd->argfltregs[0]);
		M_FLTMOVE(REG_FRESULT, rd->argfltregs[1]);
		off = dseg_addaddress(cd, m);
		M_ALD(rd->argintregs[0], REG_PV, off);

		off = dseg_addaddress(cd, builtin_displaymethodstop);
		M_ALD(REG_ITMP2, REG_PV, off);
		M_MTCTR(REG_ITMP2);
		M_JSR;

#if defined(__DARWIN__)
		M_ILD(REG_RESULT, REG_SP, LA_SIZE + ((1 + 2 + 2 + 1) + 0) * 4);
		M_ILD(REG_RESULT2, REG_SP, LA_SIZE + ((1 + 2 + 2 + 1) + 1) * 4);
		M_DLD(REG_FRESULT, REG_SP, LA_SIZE + ((1 + 2 + 2 + 1) + 2) * 4);
		M_LDA(REG_SP, REG_SP, LA_SIZE + ((1 + 2 + 2 + 1) + 2 + 2) * 4);
#else
		M_ILD(REG_RESULT, REG_SP, LA_SIZE + 0 * 4);
		M_ILD(REG_RESULT2, REG_SP, LA_SIZE + 1 * 4);
		M_DLD(REG_FRESULT, REG_SP, LA_SIZE + 2 * 4);
		M_LDA(REG_SP, REG_SP, LA_SIZE + (2 + 2) * 4);
#endif
	}

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	switch (md->returntype.type) {
	case TYPE_LNG:
		M_IST(REG_RESULT2, REG_SP, stackframesize * 4 - 4 /* 60*/);
		/* fall through */
	case TYPE_INT:
	case TYPE_ADR:
		M_IST(REG_RESULT, REG_SP, stackframesize * 4 - 8 /*56*/);
		break;
	case TYPE_FLT:
		M_FST(REG_FRESULT, REG_SP, stackframesize * 4 - 8 /*56*/);
		break;
	case TYPE_DBL:
		M_DST(REG_FRESULT, REG_SP, stackframesize * 4 - 8 /*56*/);
		break;
	}

	off = dseg_addaddress(cd, builtin_get_exceptionptrptr);
	M_ALD(REG_ITMP2, REG_PV, off);
	M_MTCTR(REG_ITMP2);
	M_JSR;
	disp = (s4) ((u1 *) mcodeptr - cd->mcodebase);
	M_MFLR(REG_ITMP1);
	M_LDA(REG_PV, REG_ITMP1, -disp);
	M_MOV(REG_RESULT, REG_ITMP2);

	switch (md->returntype.type) {
	case TYPE_LNG:
		M_ILD(REG_RESULT2, REG_SP, stackframesize * 4 - 4 /*60*/);
		/* fall through */
	case TYPE_INT:
	case TYPE_ADR:
		M_ILD(REG_RESULT, REG_SP, stackframesize * 4 - 8 /*56*/);
		break;
	case TYPE_FLT:
		M_FLD(REG_FRESULT, REG_SP, stackframesize * 4 - 8 /*56*/);
		break;
	case TYPE_DBL:
		M_DLD(REG_FRESULT, REG_SP, stackframesize * 4 - 8 /*56*/);
		break;
	}
#else
	off = dseg_addaddress(cd, &_exceptionptr)
	M_ALD(REG_ITMP2, REG_PV, off);
#endif
	M_ALD(REG_ITMP1_XPTR, REG_ITMP2, 0);/* load exception into reg. itmp1     */
	M_TST(REG_ITMP1_XPTR);
	M_BNE(4);                           /* if no exception then return        */

	M_ALD(REG_ITMP1, REG_SP, stackframesize * 4 + LA_LR_OFFSET); /* load ra   */
	M_MTLR(REG_ITMP1);
	M_LDA(REG_SP, REG_SP, stackframesize * 4); /* remove stackframe           */

	M_RET;

	M_CLR(REG_ITMP3);
	M_AST(REG_ITMP3, REG_ITMP2, 0);     /* store NULL into exceptionptr       */

	M_ALD(REG_ITMP2, REG_SP, stackframesize * 4 + LA_LR_OFFSET); /* load ra   */
	M_MTLR(REG_ITMP2);

	M_LDA(REG_SP, REG_SP, stackframesize * 4); /* remove stackframe           */

	M_IADD_IMM(REG_ITMP2, -4, REG_ITMP2_XPC); /* fault address                */

	off = dseg_addaddress(cd, asm_handle_nat_exception);
	M_ALD(REG_ITMP3, REG_PV, off);
	M_MTCTR(REG_ITMP3);
	M_RTS;

	/* generate patcher stub call code */

	{
		patchref *pref;
		s4       *xcodeptr;
		s4        mcode;
		s4       *tmpmcodeptr;

		for (pref = cd->patchrefs; pref != NULL; pref = pref->next) {
			/* Get machine code which is patched back in later. The call is   */
			/* 1 instruction word long.                                       */

			xcodeptr = (s4 *) (cd->mcodebase + pref->branchpos);
			mcode = (u4) *xcodeptr;

			/* patch in the call to call the following code (done at compile  */
			/* time)                                                          */

			tmpmcodeptr = mcodeptr;         /* save current mcodeptr          */
			mcodeptr = xcodeptr;            /* set mcodeptr to patch position */

			M_BL(tmpmcodeptr - (xcodeptr + 1));

			mcodeptr = tmpmcodeptr;         /* restore the current mcodeptr   */

			/* create stack frame - keep stack 16-byte aligned                */

			M_AADD_IMM(REG_SP, -8 * 4, REG_SP);

			/* move return address onto stack */

			M_MFLR(REG_ITMP3);
			M_AST(REG_ITMP3, REG_SP, 4 * 4);

			/* move pointer to java_objectheader onto stack */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			/* order reversed because of data segment layout */

			(void) dseg_addaddress(cd, get_dummyLR());          /* monitorPtr */
			off = dseg_addaddress(cd, NULL);                    /* vftbl      */

			M_LDA(REG_ITMP3, REG_PV, off);
			M_AST(REG_ITMP3, REG_SP, 3 * 4);
#else
			M_CLR(REG_ITMP3);
			M_AST(REG_ITMP3, REG_SP, 3 * 4);
#endif

			/* move machine code onto stack */

			off = dseg_adds4(cd, mcode);
			M_ILD(REG_ITMP3, REG_PV, off);
			M_IST(REG_ITMP3, REG_SP, 2 * 4);

			/* move class/method/field reference onto stack */

			off = dseg_addaddress(cd, pref->ref);
			M_ALD(REG_ITMP3, REG_PV, off);
			M_AST(REG_ITMP3, REG_SP, 1 * 4);

			/* move patcher function pointer onto stack */

			off = dseg_addaddress(cd, pref->patcher);
			M_ALD(REG_ITMP3, REG_PV, off);
			M_AST(REG_ITMP3, REG_SP, 0 * 4);

			off = dseg_addaddress(cd, asm_wrapper_patcher);
			M_ALD(REG_ITMP3, REG_PV, off);
			M_MTCTR(REG_ITMP3);
			M_RTS;
		}
	}

	codegen_finish(m, cd, (s4) ((u1 *) mcodeptr - cd->mcodebase));

	asm_cacheflush((void *) m->entrypoint, ((u1 *) mcodeptr - cd->mcodebase));

	return m->entrypoint;
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
