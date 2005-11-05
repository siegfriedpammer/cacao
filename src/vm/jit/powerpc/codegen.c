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
            Christian Ullrich

   $Id: codegen.c 3578 2005-11-05 16:31:41Z twisti $

*/


#include <assert.h>
#include <stdio.h>
#include <signal.h>

#include "config.h"
#include "vm/types.h"

#include "md-abi.h"
#include "md-abi.inc"

#include "vm/jit/powerpc/arch.h"
#include "vm/jit/powerpc/codegen.h"

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


s4 *codegen_trace_args( methodinfo *m, codegendata *cd, registerdata *rd,
						s4 *mcodeptr, s4 parentargs_base, bool nativestub);

/* codegen *********************************************************************

   Generates machine code.

*******************************************************************************/

void codegen(methodinfo *m, codegendata *cd, registerdata *rd)
{
	s4                  len, s1, s2, s3, d, disp;
	ptrint              a;
	s4                  parentargs_base;
	s4                 *mcodeptr;
	stackptr            src;
	varinfo            *var;
	basicblock         *bptr;
	instruction        *iptr;
	exceptiontable     *ex;
	u2                  currentline;
	methodinfo         *lm;             /* local methodinfo for ICMD_INVOKE*  */
	builtintable_entry *bte;
	methoddesc         *md;

	/* prevent compiler warnings */

	d = 0;
	lm = NULL;
	bte = NULL;

	{
	s4 i, p, t, l;
	s4 savedregs_num;

	savedregs_num = 0;

	/* space to save used callee saved registers */

	savedregs_num += (INT_SAV_CNT - rd->savintreguse);
	savedregs_num += 2 * (FLT_SAV_CNT - rd->savfltreguse);

	parentargs_base = rd->memuse + savedregs_num;

#if defined(USE_THREADS)
	/* space to save argument of monitor_enter and Return Values to survive */
    /* monitor_exit. The stack position for the argument can not be shared  */
	/* with place to save the return register on PPC, since both values     */
	/* reside in R3 */
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		/* reserve 2 slots for long/double return values for monitorexit */

		if (IS_2_WORD_TYPE(m->parseddesc->returntype.type))
			parentargs_base += 3;
		else
			parentargs_base += 2;
	}

#endif

	/* create method header */

	parentargs_base = (parentargs_base + 3) & ~3;

	(void) dseg_addaddress(cd, m);                          /* MethodPointer  */
	(void) dseg_adds4(cd, parentargs_base * 4);             /* FrameSize      */

#if defined(USE_THREADS)
	/* IsSync contains the offset relative to the stack pointer for the
	   argument of monitor_exit used in the exception handler. Since the
	   offset could be zero and give a wrong meaning of the flag it is
	   offset by one.
	*/

	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		(void) dseg_adds4(cd, (rd->memuse + 1) * 4);        /* IsSync         */
	else
#endif
		(void) dseg_adds4(cd, 0);                           /* IsSync         */
	                                       
	(void) dseg_adds4(cd, m->isleafmethod);                 /* IsLeaf         */
	(void) dseg_adds4(cd, INT_SAV_CNT - rd->savintreguse);  /* IntSave        */
	(void) dseg_adds4(cd, FLT_SAV_CNT - rd->savfltreguse);  /* FltSave        */

	dseg_addlinenumbertablesize(cd);

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
		M_MFLR(REG_ZERO);
		M_AST(REG_ZERO, REG_SP, LA_LR_OFFSET);
	}

	if (parentargs_base) {
		M_STWU(REG_SP, REG_SP, -parentargs_base * 4);
	}

	/* save return address and used callee saved registers */

	p = parentargs_base;
	for (i = INT_SAV_CNT - 1; i >= rd->savintreguse; i--) {
		p--; M_IST(rd->savintregs[i], REG_SP, p * 4);
	}
	for (i = FLT_SAV_CNT - 1; i >= rd->savfltreguse; i--) {
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
			if (IS_2_WORD_TYPE(t))
				s2 = PACK_REGS(rd->argintregs[GET_LOW_REG(s1)],
							   rd->argintregs[GET_HIGH_REG(s1)]);
			else
				s2 = rd->argintregs[s1];
 			if (!md->params[p].inmemory) {           /* register arguments    */
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
 					M_TINTMOVE(t, s2, var->regoff);

				} else {                             /* reg arg -> spilled    */
					if (IS_2_WORD_TYPE(t)) {
						M_IST(GET_HIGH_REG(s2), REG_SP, var->regoff * 4);
						M_IST(GET_LOW_REG(s2), REG_SP, var->regoff * 4 + 4);
					} else {
						M_IST(s2, REG_SP, var->regoff * 4);
					}
				}

			} else {                                 /* stack arguments       */
 				if (!(var->flags & INMEMORY)) {      /* stack arg -> register */
					if (IS_2_WORD_TYPE(t)) {
						M_ILD(GET_HIGH_REG(var->regoff), REG_SP,
							  (parentargs_base + s1) * 4);
						M_ILD(GET_LOW_REG(var->regoff), REG_SP,
							  (parentargs_base + s1) * 4 + 4);
					} else {
						M_ILD(var->regoff, REG_SP, (parentargs_base + s1) * 4);
					}

				} else {                             /* stack arg -> spilled  */
#if 1
 					M_ILD(REG_ITMP1, REG_SP, (parentargs_base + s1) * 4);
 					M_IST(REG_ITMP1, REG_SP, var->regoff * 4);
					if (IS_2_WORD_TYPE(t)) {
						M_ILD(REG_ITMP1, REG_SP, (parentargs_base + s1) * 4 +4);
						M_IST(REG_ITMP1, REG_SP, var->regoff * 4 + 4);
					}
#else
					/* Reuse Memory Position on Caller Stack */
					var->regoff = parentargs_base + s1;
#endif
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
#if 1
					if (IS_2_WORD_TYPE(t)) {
						M_DLD(REG_FTMP1, REG_SP, (parentargs_base + s1) * 4);
						M_DST(REG_FTMP1, REG_SP, var->regoff * 4);
						var->regoff = parentargs_base + s1;

					} else {
						M_FLD(REG_FTMP1, REG_SP, (parentargs_base + s1) * 4);
						M_FST(REG_FTMP1, REG_SP, var->regoff * 4);
					}
#else
					/* Reuse Memory Position on Caller Stack */
					var->regoff = parentargs_base + s1;
#endif
				}
			}
		}
	} /* end for */

	/* save monitorenter argument */

#if defined(USE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		/* stack offset for monitor argument */

		s1 = rd->memuse;

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
		mcodeptr = codegen_trace_args(m, cd, rd, mcodeptr, parentargs_base, false);

	} /* if (runverbose) */
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
					/* d = reg_of_var(m, src, REG_ITMP1); */
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
				if (src->type == TYPE_LNG)
					d = reg_of_var(rd, src, PACK_REGS(REG_ITMP2, REG_ITMP1));
				else
					d = reg_of_var(rd, src, REG_IFTMP);
				if ((src->varkind != STACKVAR)) {
					s2 = src->type;
					if (IS_FLT_DBL_TYPE(s2)) {
						if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
							s1 = rd->interfaces[len][s2].regoff;
							M_FLTMOVE(s1,d);
						} else {
							if (IS_2_WORD_TYPE(s2)) {
								M_DLD(d, REG_SP,
									  4 * rd->interfaces[len][s2].regoff);
							} else {
								M_FLD(d, REG_SP,
									  4 * rd->interfaces[len][s2].regoff);
							}	
						}
						store_reg_to_var_flt(src, d);
					} else {
						if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
							s1 = rd->interfaces[len][s2].regoff;
							M_TINTMOVE(s2,s1,d);
						} else {
							if (IS_2_WORD_TYPE(s2)) {
								M_ILD(GET_HIGH_REG(d), REG_SP,
									  4 * rd->interfaces[len][s2].regoff);
								M_ILD(GET_LOW_REG(d), REG_SP,
									  4 * rd->interfaces[len][s2].regoff + 4);
							} else {
								M_ILD(d, REG_SP,
									  4 * rd->interfaces[len][s2].regoff);
							}
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
		currentline = 0;

		for (iptr = bptr->iinstr; len > 0; src = iptr->dst, len--, iptr++) {
			if (iptr->line != currentline) {
				dseg_addlinenumber(cd, iptr->line, (u1 *) mcodeptr);
				currentline = iptr->line;
			}

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

			d = reg_of_var(rd, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
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
		case ICMD_ALOAD:      /* op1 = local variable                         */

			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1))
				break;
			if (var->flags & INMEMORY) {
				M_ILD(d, REG_SP, var->regoff * 4);
			} else {
				M_TINTMOVE(var->type, var->regoff, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			d = reg_of_var(rd, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			if ((iptr->dst->varkind == LOCALVAR) &&
				(iptr->dst->varnum == iptr->op1))
				break;
			if (var->flags & INMEMORY) {
				M_ILD(GET_HIGH_REG(d), REG_SP, var->regoff * 4);
				M_ILD(GET_LOW_REG(d), REG_SP, var->regoff * 4 + 4);
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
					M_DLD(d, REG_SP, var->regoff * 4);
				else
					M_FLD(d, REG_SP, var->regoff * 4);
			else {
				M_FLTMOVE(var->regoff, d);
			}
			store_reg_to_var_flt(iptr->dst, d);
			break;


		case ICMD_ISTORE:     /* ..., value  ==> ...                          */
		case ICMD_ASTORE:     /* op1 = local variable                         */

			if ((src->varkind == LOCALVAR) && (src->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				var_to_reg_int(s1, src, REG_ITMP1);
				M_IST(s1, REG_SP, var->regoff * 4);
			} else {
				var_to_reg_int(s1, src, var->regoff);
				M_TINTMOVE(var->type, s1, var->regoff);
			}
			break;

		case ICMD_LSTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			if ((src->varkind == LOCALVAR) && (src->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				var_to_reg_int(s1, src, PACK_REGS(REG_ITMP2, REG_ITMP1));
				M_IST(GET_HIGH_REG(s1), REG_SP, var->regoff * 4);
				M_IST(GET_LOW_REG(s1), REG_SP, var->regoff * 4 + 4);
			} else {
				var_to_reg_int(s1, src, var->regoff);
				M_TINTMOVE(var->type, s1, var->regoff);
			}
			break;

		case ICMD_FSTORE:     /* ..., value  ==> ...                          */
		case ICMD_DSTORE:     /* op1 = local variable                         */

			if ((src->varkind == LOCALVAR) && (src->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				var_to_reg_flt(s1, src, REG_FTMP1);
				if (var->type == TYPE_DBL)
					M_DST(s1, REG_SP, var->regoff * 4);
				else
					M_FST(s1, REG_SP, var->regoff * 4);
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
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_NEG(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_int(s1, src, PACK_REGS(REG_ITMP2, REG_ITMP1));
			d = reg_of_var(rd, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			M_SUBFIC(GET_LOW_REG(s1), 0, GET_LOW_REG(d));
			M_SUBFZE(GET_HIGH_REG(s1), GET_HIGH_REG(d));
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_I2L:        /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			M_INTMOVE(s1, GET_LOW_REG(d));
			M_SRA_IMM(GET_LOW_REG(d), 31, GET_HIGH_REG(d));
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */

			var_to_reg_int_low(s1, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_BSEXT(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2CHAR:   /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_CZEXT(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2SHORT:  /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_SSEXT(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_IADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_IADD(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if ((iptr->val.i >= -32768) && (iptr->val.i <= 32767)) {
				M_IADD_IMM(s1, iptr->val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_IADD(s1, REG_ITMP2, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_int_low(s1, src->prev, REG_ITMP1);
			var_to_reg_int_low(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			M_ADDC(s1, s2, GET_LOW_REG(d));
			var_to_reg_int_high(s1, src->prev, REG_ITMP1);
			var_to_reg_int_high(s2, src, REG_ITMP3);   /* don't use REG_ITMP2 */
			M_ADDE(s1, s2, GET_HIGH_REG(d));
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.l = constant                             */

			s3 = iptr->val.l & 0xffffffff;
			var_to_reg_int_low(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			if ((s3 >= -32768) && (s3 <= 32767)) {
				M_ADDIC(s1, s3, GET_LOW_REG(d));
			} else {
				ICONST(REG_ITMP2, s3);
				M_ADDC(s1, REG_ITMP2, GET_LOW_REG(d));
			}
			var_to_reg_int_high(s1, src, REG_ITMP1);
			s3 = iptr->val.l >> 32;
			if (s3 == -1) {
				M_ADDME(s1, GET_HIGH_REG(d));
			} else if (s3 == 0) {
				M_ADDZE(s1, GET_HIGH_REG(d));
			} else {
				ICONST(REG_ITMP3, s3);                 /* don't use REG_ITMP2 */
				M_ADDE(s1, REG_ITMP3, GET_HIGH_REG(d));
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_ISUB(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if ((iptr->val.i >= -32767) && (iptr->val.i <= 32768)) {
				M_IADD_IMM(s1, -iptr->val.i, d);
			} else {
				ICONST(REG_ITMP2, -iptr->val.i);
				M_IADD(s1, REG_ITMP2, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_int_low(s1, src->prev, REG_ITMP1);
			var_to_reg_int_low(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			M_SUBC(s1, s2, GET_LOW_REG(d));
			var_to_reg_int_high(s1, src->prev, REG_ITMP1);
			var_to_reg_int_high(s2, src, REG_ITMP3);   /* don't use REG_ITMP2 */
			M_SUBE(s1, s2, GET_HIGH_REG(d));
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* val.l = constant                             */

			s3 = (-iptr->val.l) & 0xffffffff;
			var_to_reg_int_low(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			if ((s3 >= -32768) && (s3 <= 32767)) {
				M_ADDIC(s1, s3, GET_LOW_REG(d));
			} else {
				ICONST(REG_ITMP2, s3);
				M_ADDC(s1, REG_ITMP2, GET_LOW_REG(d));
			}
			var_to_reg_int_high(s1, src, REG_ITMP1);
			s3 = (-iptr->val.l) >> 32;
			if (s3 == -1)
				M_ADDME(s1, GET_HIGH_REG(d));
			else if (s3 == 0)
				M_ADDZE(s1, GET_HIGH_REG(d));
			else {
				ICONST(REG_ITMP3, s3);                 /* don't use REG_ITMP2 */
				M_ADDE(s1, REG_ITMP3, GET_HIGH_REG(d));
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			M_TST(s2);
			M_BEQ(0);
			codegen_addxdivrefs(cd, mcodeptr);
			M_LDAH(REG_ITMP3, REG_ZERO, 0x8000);
			M_CMP(REG_ITMP3, s1);
			M_BNE(3 + (s1 != d));
			M_CMPI(s2, -1);
			M_BNE(1 + (s1 != d));
			M_INTMOVE(s1, d);
			M_BR(1);
			M_IDIV(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_TST(s2);
			M_BEQ(0);
			codegen_addxdivrefs(cd, mcodeptr);
			M_LDAH(REG_ITMP3, REG_ZERO, 0x8000);
			M_CMP(REG_ITMP3, s1);
			M_BNE(4);
			M_CMPI(s2, -1);
			M_BNE(2);
			M_CLR(d);
			M_BR(3);
			M_IDIV(s1, s2, REG_ITMP3);
			M_IMUL(REG_ITMP3, s2, REG_ITMP3);
			M_ISUB(s1, REG_ITMP3, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */
		case ICMD_LREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			bte = iptr->val.a;
			md = bte->md;

			var_to_reg_int(s2, src, PACK_REGS(REG_ITMP2, REG_ITMP1));
			M_OR_TST(GET_HIGH_REG(s2), GET_LOW_REG(s2), REG_ITMP3);
			M_BEQ(0);
			codegen_addxdivrefs(cd, mcodeptr);

			s3 = PACK_REGS(rd->argintregs[GET_LOW_REG(md->params[1].regoff)],
						   rd->argintregs[GET_HIGH_REG(md->params[1].regoff)]);
			M_TINTMOVE(TYPE_LNG, s2, s3);

			var_to_reg_int(s1, src->prev, PACK_REGS(REG_ITMP2, REG_ITMP1));
			s3 = PACK_REGS(rd->argintregs[GET_LOW_REG(md->params[0].regoff)],
						   rd->argintregs[GET_HIGH_REG(md->params[0].regoff)]);
			M_TINTMOVE(TYPE_LNG, s1, s3);

			disp = dseg_addaddress(cd, bte->fp);
			M_ALD(REG_ITMP1, REG_PV, disp);
			M_MTCTR(REG_ITMP1);
			M_JSR;

			d = reg_of_var(rd, iptr->dst, PACK_REGS(REG_RESULT2, REG_RESULT));
			M_TINTMOVE(TYPE_LNG, PACK_REGS(REG_RESULT2, REG_RESULT), d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_IMUL(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if ((iptr->val.i >= -32768) && (iptr->val.i <= 32767)) {
				M_IMUL_IMM(s1, iptr->val.i, d);
			} else {
				ICONST(REG_ITMP3, iptr->val.i);
				M_IMUL(s1, REG_ITMP3, d);
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
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_AND_IMM(s2, 0x1f, REG_ITMP3);
			M_SLL(s1, REG_ITMP3, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_SLL_IMM(s1, iptr->val.i & 0x1f, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_AND_IMM(s2, 0x1f, REG_ITMP3);
			M_SRA(s1, REG_ITMP3, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_SRA_IMM(s1, iptr->val.i & 0x1f, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_AND_IMM(s2, 0x1f, REG_ITMP2);
			M_SRL(s1, REG_ITMP2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if (iptr->val.i & 0x1f) {
				M_SRL_IMM(s1, iptr->val.i & 0x1f, d);
			} else {
				M_INTMOVE(s1, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_AND(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 65535)) {
				M_AND_IMM(s1, iptr->val.i, d);
				}
			/*
			else if (iptr->val.i == 0xffffff) {
				M_RLWINM(s1, 0, 8, 31, d);
				}
			*/
			else {
				ICONST(REG_ITMP3, iptr->val.i);
				M_AND(s1, REG_ITMP3, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			var_to_reg_int_low(s1, src->prev, REG_ITMP1);
			var_to_reg_int_low(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			M_AND(s1, s2, GET_LOW_REG(d));
			var_to_reg_int_high(s1, src->prev, REG_ITMP1);
			var_to_reg_int_high(s2, src, REG_ITMP3);   /* don't use REG_ITMP2 */
			M_AND(s1, s2, GET_HIGH_REG(d));
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.l = constant                             */

			s3 = iptr->val.l & 0xffffffff;
			var_to_reg_int_low(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			if ((s3 >= 0) && (s3 <= 65535)) {
				M_AND_IMM(s1, s3, GET_LOW_REG(d));
			} else {
				ICONST(REG_ITMP3, s3);
				M_AND(s1, REG_ITMP3, GET_LOW_REG(d));
			}
			var_to_reg_int_high(s1, src, REG_ITMP1);
			s3 = iptr->val.l >> 32;
			if ((s3 >= 0) && (s3 <= 65535)) {
				M_AND_IMM(s1, s3, GET_HIGH_REG(d));
			} else {
				ICONST(REG_ITMP3, s3);                 /* don't use REG_ITMP2 */
				M_AND(s1, REG_ITMP3, GET_HIGH_REG(d));
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_MOV(s1, REG_ITMP2);
			M_CMPI(s1, 0);
			M_BGE(1 + 2*(iptr->val.i >= 32768));
			if (iptr->val.i >= 32768) {
				M_ADDIS(REG_ZERO, iptr->val.i >> 16, REG_ITMP2);
				M_OR_IMM(REG_ITMP2, iptr->val.i, REG_ITMP2);
				M_IADD(s1, REG_ITMP2, REG_ITMP2);
			} else {
				M_IADD_IMM(s1, iptr->val.i, REG_ITMP2);
			}
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
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_OR(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 65535)) {
				M_OR_IMM(s1, iptr->val.i, d);
			} else {
				ICONST(REG_ITMP3, iptr->val.i);
				M_OR(s1, REG_ITMP3, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LOR:       /* ..., val1, val2  ==> ..., val1 | val2        */

			var_to_reg_int_low(s1, src->prev, REG_ITMP1);
			var_to_reg_int_low(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			M_OR(s1, s2, GET_LOW_REG(d));
			var_to_reg_int_high(s1, src->prev, REG_ITMP1);
			var_to_reg_int_high(s2, src, REG_ITMP3);   /* don't use REG_ITMP2 */
			M_OR(s1, s2, GET_HIGH_REG(d));
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.l = constant                             */

			s3 = iptr->val.l & 0xffffffff;
			var_to_reg_int_low(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			if ((s3 >= 0) && (s3 <= 65535)) {
				M_OR_IMM(s1, s3, GET_LOW_REG(d));
			} else {
				ICONST(REG_ITMP3, s3);
				M_OR(s1, REG_ITMP3, GET_LOW_REG(d));
			}
			var_to_reg_int_high(s1, src, REG_ITMP1);
			s3 = iptr->val.l >> 32;
			if ((s3 >= 0) && (s3 <= 65535)) {
				M_OR_IMM(s1, s3, GET_HIGH_REG(d));
			} else {
				ICONST(REG_ITMP3, s3);                 /* don't use REG_ITMP2 */
				M_OR(s1, REG_ITMP3, GET_HIGH_REG(d));
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_XOR(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 65535)) {
				M_XOR_IMM(s1, iptr->val.i, d);
			} else {
				ICONST(REG_ITMP3, iptr->val.i);
				M_XOR(s1, REG_ITMP3, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			var_to_reg_int_low(s1, src->prev, REG_ITMP1);
			var_to_reg_int_low(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			M_XOR(s1, s2, GET_LOW_REG(d));
			var_to_reg_int_high(s1, src->prev, REG_ITMP1);
			var_to_reg_int_high(s2, src, REG_ITMP3);   /* don't use REG_ITMP2 */
			M_XOR(s1, s2, GET_HIGH_REG(d));
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.l = constant                             */

			s3 = iptr->val.l & 0xffffffff;
			var_to_reg_int_low(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			if ((s3 >= 0) && (s3 <= 65535)) {
				M_XOR_IMM(s1, s3, GET_LOW_REG(d));
			} else {
				ICONST(REG_ITMP3, s3);
				M_XOR(s1, REG_ITMP3, GET_LOW_REG(d));
			}
			var_to_reg_int_high(s1, src, REG_ITMP1);
			s3 = iptr->val.l >> 32;
			if ((s3 >= 0) && (s3 <= 65535)) {
				M_XOR_IMM(s1, s3, GET_HIGH_REG(d));
			} else {
				ICONST(REG_ITMP3, s3);                 /* don't use REG_ITMP2 */
				M_XOR(s1, REG_ITMP3, GET_HIGH_REG(d));
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LCMP:       /* ..., val1, val2  ==> ..., val1 cmp val2      */
			/*******************************************************************
                TODO: CHANGE THIS TO A VERSION THAT WORKS !!!
			*******************************************************************/
			var_to_reg_int_high(s1, src->prev, REG_ITMP3);
			var_to_reg_int_high(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			{
				int tempreg = false;
				int dreg;
				s4  *br1;

				if (src->prev->flags & INMEMORY) {
					tempreg = tempreg || (d == REG_ITMP3) || (d == REG_ITMP2);
				} else {
					tempreg = tempreg || (d == GET_HIGH_REG(src->prev->regoff))
						        || (d == GET_LOW_REG(src->prev->regoff));
				}
				if (src->flags & INMEMORY) {
					tempreg = tempreg || (d == REG_ITMP3) || (d == REG_ITMP2);
				} else {
					tempreg = tempreg || (d == GET_HIGH_REG(src->regoff))
                                 || (d == GET_LOW_REG(src->regoff));
				}

				dreg = tempreg ? REG_ITMP1 : d;
				M_IADD_IMM(REG_ZERO, 1, dreg);
				M_CMP(s1, s2);
				M_BGT(0);
				br1 = mcodeptr;
				M_BLT(0);
				var_to_reg_int_low(s1, src->prev, REG_ITMP3);
				var_to_reg_int_low(s2, src, REG_ITMP2);
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
			} else {
				s1 = var->regoff;
			}
			{
				u4 m = iptr->val.i;
				if (m & 0x8000)
					m += 65536;
				if (m & 0xffff0000)
					M_ADDIS(s1, m >> 16, s1);
				if (m & 0xffff)
					M_IADD_IMM(s1, m & 0xffff, s1);
			}
			if (var->flags & INMEMORY) {
				M_IST(s1, REG_SP, var->regoff * 4);
			}
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

		case ICMD_DMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

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
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_CLR(d);
			disp = dseg_addfloat(cd, 0.0);
			M_FLD(REG_FTMP2, REG_PV, disp);
			M_FCMPU(s1, REG_FTMP2);
			M_BNAN(4);
			disp = dseg_adds4(cd, 0);
			M_CVTDL_C(s1, REG_FTMP1);
			M_LDA(REG_ITMP1, REG_PV, disp);
			M_STFIWX(REG_FTMP1, 0, REG_ITMP1);
			M_ILD(d, REG_PV, disp);
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
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			M_FCMPU(s2, s1);
			M_IADD_IMM(REG_ZERO, -1, d);
			M_BNAN(4);
			M_BGT(3);
			M_IADD_IMM(REG_ZERO, 0, d);
			M_BGE(1);
			M_IADD_IMM(REG_ZERO, 1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */
		case ICMD_DCMPG:
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			M_FCMPU(s1, s2);
			M_IADD_IMM(REG_ZERO, 1, d);
			M_BNAN(4);
			M_BGT(3);
			M_IADD_IMM(REG_ZERO, 0, d);
			M_BGE(1);
			M_IADD_IMM(REG_ZERO, -1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;
			

		/* memory operations **************************************************/

		case ICMD_ARRAYLENGTH: /* ..., arrayref  ==> ..., length              */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			gen_nullptr_check(s1);
			M_ILD(d, s1, OFFSET(java_arrayheader, size));
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_BALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_IADD_IMM(s2, OFFSET(java_chararray, data[0]), REG_ITMP2);
			M_LBZX(d, s1, REG_ITMP2);
			M_BSEXT(d, d);
			store_reg_to_var_int(iptr->dst, d);
			break;			

		case ICMD_CALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_SLL_IMM(s2, 1, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_chararray, data[0]), REG_ITMP2);
			M_LHZX(d, s1, REG_ITMP2);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_SALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_SLL_IMM(s2, 1, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_shortarray, data[0]), REG_ITMP2);
			M_LHAX(d, s1, REG_ITMP2);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_SLL_IMM(s2, 2, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_intarray, data[0]), REG_ITMP2);
			M_LWZX(d, s1, REG_ITMP2);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_SLL_IMM(s2, 3, REG_ITMP2);
			M_IADD(s1, REG_ITMP2, REG_ITMP2);
			M_ILD(GET_HIGH_REG(d), REG_ITMP2, OFFSET(java_longarray, data[0]));
			M_ILD(GET_LOW_REG(d), REG_ITMP2, OFFSET(java_longarray,
													data[0]) + 4);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
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
			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_SLL_IMM(s2, 3, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_doublearray, data[0]), REG_ITMP2);
			M_LFDX(d, s1, REG_ITMP2);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_AALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_SLL_IMM(s2, 2, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_objectarray, data[0]), REG_ITMP2);
			M_LWZX(d, s1, REG_ITMP2);
			store_reg_to_var_int(iptr->dst, d);
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

		case ICMD_LASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int_high(s3, src, REG_ITMP3);
			M_SLL_IMM(s2, 3, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_longarray, data[0]), REG_ITMP2);
			M_STWX(s3, s1, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, 4, REG_ITMP2);
			var_to_reg_int_low(s3, src, REG_ITMP3);
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

		case ICMD_AASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, rd->argintregs[0]);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
/* 			if (iptr->op1 == 0) { */
				gen_nullptr_check(s1);
				gen_bound_check;
/* 			} */
			var_to_reg_int(s3, src, rd->argintregs[1]);

			M_INTMOVE(s1, rd->argintregs[0]);
			M_INTMOVE(s3, rd->argintregs[1]);
			bte = iptr->val.a;
			disp = dseg_addaddress(cd, bte->fp);
			M_ALD(REG_ITMP1, REG_PV, disp);
			M_MTCTR(REG_ITMP1);
			M_JSR;
			M_TST(REG_RESULT);
			M_BEQ(0);
			codegen_addxstorerefs(cd, mcodeptr);

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			var_to_reg_int(s3, src, REG_ITMP3);
			M_SLL_IMM(s2, 2, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_objectarray, data[0]), REG_ITMP2);
			M_STWX(s3, s1, REG_ITMP2);
			break;


		case ICMD_GETSTATIC:  /* ...  ==> ..., value                          */
		                      /* op1 = type, val.a = field address            */

			if (!iptr->val.a) {
				disp = dseg_addaddress(cd, NULL);

				codegen_addpatchref(cd, mcodeptr,
									PATCHER_get_putstatic,
									(unresolved_field *) iptr->target, disp);

				if (opt_showdisassemble)
					M_NOP;

			} else {
				fieldinfo *fi = iptr->val.a;

				disp = dseg_addaddress(cd, &(fi->value));

				if (!fi->class->initialized) {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_clinit, fi->class, disp);

					if (opt_showdisassemble)
						M_NOP;
				}
  			}

			M_ALD(REG_ITMP1, REG_PV, disp);
			switch (iptr->op1) {
			case TYPE_INT:
				d = reg_of_var(rd, iptr->dst, REG_ITMP2);
				M_ILD_INTERN(d, REG_ITMP1, 0);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_LNG:
				d = reg_of_var(rd, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
				M_ILD_INTERN(GET_LOW_REG(d), REG_ITMP1, 4);/* keep this order */
				M_ILD_INTERN(GET_HIGH_REG(d), REG_ITMP1, 0);/*keep this order */
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_ADR:
				d = reg_of_var(rd, iptr->dst, REG_ITMP2);
				M_ALD_INTERN(d, REG_ITMP1, 0);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_FLT:
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				M_FLD_INTERN(d, REG_ITMP1, 0);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			case TYPE_DBL:				
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				M_DLD_INTERN(d, REG_ITMP1, 0);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			}
			break;

		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */
		                      /* op1 = type, val.a = field address            */


			if (!iptr->val.a) {
				disp = dseg_addaddress(cd, NULL);

				codegen_addpatchref(cd, mcodeptr,
									PATCHER_get_putstatic,
									(unresolved_field *) iptr->target, disp);

				if (opt_showdisassemble)
					M_NOP;

			} else {
				fieldinfo *fi = iptr->val.a;

				disp = dseg_addaddress(cd, &(fi->value));

				if (!fi->class->initialized) {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_clinit, fi->class, disp);

					if (opt_showdisassemble)
						M_NOP;
				}
  			}

			M_ALD(REG_ITMP1, REG_PV, disp);
			switch (iptr->op1) {
			case TYPE_INT:
				var_to_reg_int(s2, src, REG_ITMP2);
				M_IST_INTERN(s2, REG_ITMP1, 0);
				break;
			case TYPE_LNG:
				var_to_reg_int(s2, src, PACK_REGS(REG_ITMP2, REG_ITMP3));
				M_IST_INTERN(GET_HIGH_REG(s2), REG_ITMP1, 0);
				M_IST_INTERN(GET_LOW_REG(s2), REG_ITMP1, 4);
				break;
			case TYPE_ADR:
				var_to_reg_int(s2, src, REG_ITMP2);
				M_AST_INTERN(s2, REG_ITMP1, 0);
				break;
			case TYPE_FLT:
				var_to_reg_flt(s2, src, REG_FTMP2);
				M_FST_INTERN(s2, REG_ITMP1, 0);
				break;
			case TYPE_DBL:
				var_to_reg_flt(s2, src, REG_FTMP2);
				M_DST_INTERN(s2, REG_ITMP1, 0);
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
									(unresolved_field *) iptr->target, 0);

				if (opt_showdisassemble)
					M_NOP;

				disp = 0;

			} else {
				disp = ((fieldinfo *) (iptr->val.a))->offset;
			}

			switch (iptr->op1) {
			case TYPE_INT:
				d = reg_of_var(rd, iptr->dst, REG_ITMP2);
				M_ILD(d, s1, disp);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_LNG:
   				d = reg_of_var(rd, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
				M_ILD(GET_LOW_REG(d), s1, disp + 4);       /* keep this order */
				M_ILD(GET_HIGH_REG(d), s1, disp);          /* keep this order */
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_ADR:
				d = reg_of_var(rd, iptr->dst, REG_ITMP2);
				M_ALD(d, s1, disp);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_FLT:
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				M_FLD(d, s1, disp);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			case TYPE_DBL:				
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				M_DLD(d, s1, disp);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			}
			break;

		case ICMD_PUTFIELD:   /* ..., value  ==> ...                          */
		                      /* op1 = type, val.i = field offset             */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			gen_nullptr_check(s1);

			if (!IS_FLT_DBL_TYPE(iptr->op1)) {
				if (IS_2_WORD_TYPE(iptr->op1)) {
					var_to_reg_int(s2, src, PACK_REGS(REG_ITMP2, REG_ITMP3));
				} else {
					var_to_reg_int(s2, src, REG_ITMP2);
				}
			} else {
				var_to_reg_flt(s2, src, REG_FTMP2);
			}

			if (!iptr->val.a) {
				codegen_addpatchref(cd, mcodeptr,
									PATCHER_get_putfield,
									(unresolved_field *) iptr->target, 0);

				if (opt_showdisassemble)
					M_NOP;

				disp = 0;

			} else {
				disp = ((fieldinfo *) (iptr->val.a))->offset;
			}

			switch (iptr->op1) {
			case TYPE_INT:
				M_IST(s2, s1, disp);
				break;
			case TYPE_LNG:
				M_IST(GET_LOW_REG(s2), s1, disp + 4);      /* keep this order */
				M_IST(GET_HIGH_REG(s2), s1, disp);         /* keep this order */
				break;
			case TYPE_ADR:
				M_AST(s2, s1, disp);
				break;
			case TYPE_FLT:
				M_FST(s2, s1, disp);
				break;
			case TYPE_DBL:
				M_DST(s2, s1, disp);
				break;
			}
			break;


		/* branch operations **************************************************/

		case ICMD_ATHROW:       /* ..., objectref ==> ... (, objectref)       */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_INTMOVE(s1, REG_ITMP1_XPTR);

			if (iptr->val.a) {
				codegen_addpatchref(cd, mcodeptr,
									PATCHER_athrow_areturn,
									(unresolved_class *) iptr->val.a, 0);

				if (opt_showdisassemble)
					M_NOP;
			}

			disp = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP2, REG_PV, disp);
			M_MTCTR(REG_ITMP2);

			if (m->isleafmethod) M_MFLR(REG_ITMP3);         /* save LR        */
			M_BL(0);                                        /* get current PC */
			M_MFLR(REG_ITMP2_XPC);
			if (m->isleafmethod) M_MTLR(REG_ITMP3);         /* restore LR     */
			M_RTS;                                          /* jump to CTR    */

			ALIGNCODENOP;
			break;

		case ICMD_GOTO:         /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */
			M_BR(0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
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
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;
			
		case ICMD_RET:          /* ... ==> ...                                */
		                        /* op1 = local variable                       */

			var = &(rd->locals[iptr->op1][TYPE_ADR]);
			if (var->flags & INMEMORY) {
				M_ALD(REG_ITMP1, REG_SP, var->regoff * 4);
				M_MTCTR(REG_ITMP1);
			} else {
				M_MTCTR(var->regoff);
			}
			M_RTS;
			ALIGNCODENOP;
			break;

		case ICMD_IFNULL:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_TST(s1);
			M_BEQ(0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IFNONNULL:    /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_TST(s1);
			M_BNE(0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
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
			} else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_CMP(s1, REG_ITMP2);
			}
			switch (iptr->opc) {
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
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;


		case ICMD_IF_LEQ:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int_low(s1, src, REG_ITMP1);
			var_to_reg_int_high(s2, src, REG_ITMP2);
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
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;
			
		case ICMD_IF_LLT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */
			var_to_reg_int_low(s1, src, REG_ITMP1);
			var_to_reg_int_high(s2, src, REG_ITMP2);
/*  			if (iptr->val.l == 0) { */
/*  				M_OR(s1, s2, REG_ITMP3); */
/*  				M_CMPI(REG_ITMP3, 0); */

/*    			} else  */
			if ((iptr->val.l >= -32768) && (iptr->val.l <= 32767)) {
  				M_CMPI(s2, (u4) (iptr->val.l >> 32));
				M_BLT(0);
				codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
				M_BGT(2);
  				M_CMPI(s1, (u4) (iptr->val.l & 0xffffffff));
  			} else {
  				ICONST(REG_ITMP3, (u4) (iptr->val.l >> 32));
  				M_CMP(s2, REG_ITMP3);
				M_BLT(0);
				codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
				M_BGT(3);
  				ICONST(REG_ITMP3, (u4) (iptr->val.l & 0xffffffff));
				M_CMP(s1, REG_ITMP3)
			}
			M_BLT(0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;
			
		case ICMD_IF_LLE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int_low(s1, src, REG_ITMP1);
			var_to_reg_int_high(s2, src, REG_ITMP2);
/*  			if (iptr->val.l == 0) { */
/*  				M_OR(s1, s2, REG_ITMP3); */
/*  				M_CMPI(REG_ITMP3, 0); */

/*    			} else  */
			if ((iptr->val.l >= -32768) && (iptr->val.l <= 32767)) {
  				M_CMPI(s2, (u4) (iptr->val.l >> 32));
				M_BLT(0);
				codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
				M_BGT(2);
  				M_CMPI(s1, (u4) (iptr->val.l & 0xffffffff));
  			} else {
  				ICONST(REG_ITMP3, (u4) (iptr->val.l >> 32));
  				M_CMP(s2, REG_ITMP3);
				M_BLT(0);
				codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
				M_BGT(3);
  				ICONST(REG_ITMP3, (u4) (iptr->val.l & 0xffffffff));
				M_CMP(s1, REG_ITMP3)
			}
			M_BLE(0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;
			
		case ICMD_IF_LNE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int_low(s1, src, REG_ITMP1);
			var_to_reg_int_high(s2, src, REG_ITMP2);
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
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;
			
		case ICMD_IF_LGT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int_low(s1, src, REG_ITMP1);
			var_to_reg_int_high(s2, src, REG_ITMP2);
/*  			if (iptr->val.l == 0) { */
/*  				M_OR(s1, s2, REG_ITMP3); */
/*  				M_CMPI(REG_ITMP3, 0); */

/*    			} else  */
			if ((iptr->val.l >= -32768) && (iptr->val.l <= 32767)) {
  				M_CMPI(s2, (u4) (iptr->val.l >> 32));
				M_BGT(0);
				codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
				M_BLT(2);
  				M_CMPI(s1, (u4) (iptr->val.l & 0xffffffff));
  			} else {
  				ICONST(REG_ITMP3, (u4) (iptr->val.l >> 32));
  				M_CMP(s2, REG_ITMP3);
				M_BGT(0);
				codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
				M_BLT(3);
  				ICONST(REG_ITMP3, (u4) (iptr->val.l & 0xffffffff));
				M_CMP(s1, REG_ITMP3)
			}
			M_BGT(0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;
			
		case ICMD_IF_LGE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */
			var_to_reg_int_low(s1, src, REG_ITMP1);
			var_to_reg_int_high(s2, src, REG_ITMP2);
/*  			if (iptr->val.l == 0) { */
/*  				M_OR(s1, s2, REG_ITMP3); */
/*  				M_CMPI(REG_ITMP3, 0); */

/*    			} else  */
			if ((iptr->val.l >= -32768) && (iptr->val.l <= 32767)) {
  				M_CMPI(s2, (u4) (iptr->val.l >> 32));
				M_BGT(0);
				codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
				M_BLT(2);
  				M_CMPI(s1, (u4) (iptr->val.l & 0xffffffff));
  			} else {
  				ICONST(REG_ITMP3, (u4) (iptr->val.l >> 32));
  				M_CMP(s2, REG_ITMP3);
				M_BGT(0);
				codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
				M_BLT(3);
  				ICONST(REG_ITMP3, (u4) (iptr->val.l & 0xffffffff));
				M_CMP(s1, REG_ITMP3)
			}
			M_BGE(0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

			/* CUT: alle _L */
		case ICMD_IF_ICMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPEQ:    /* op1 = target JavaVM pc                     */
			/******************************************************************
            TODO: CMP UPPER 32 BIT OF LONGS, TOO!
			*******************************************************************/
		case ICMD_IF_ACMPEQ:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BEQ(0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IF_ICMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPNE:    /* op1 = target JavaVM pc                     */
			/******************************************************************
            TODO: CMP UPPER 32 BIT OF LONGS, TOO!
			*******************************************************************/
		case ICMD_IF_ACMPNE:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BNE(0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IF_ICMPLT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPLT:    /* op1 = target JavaVM pc                     */
			/******************************************************************
            TODO: CMP UPPER 32 BIT OF LONGS, TOO!
			*******************************************************************/

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BLT(0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IF_ICMPGT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPGT:    /* op1 = target JavaVM pc                     */
			/******************************************************************
            TODO: CMP UPPER 32 BIT OF LONGS, TOO!
			*******************************************************************/

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BGT(0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IF_ICMPLE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPLE:    /* op1 = target JavaVM pc                     */
			/******************************************************************
            TODO: CMP UPPER 32 BIT OF LONGS, TOO!
			*******************************************************************/

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BLE(0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IF_ICMPGE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPGE:    /* op1 = target JavaVM pc                     */
			/******************************************************************
            TODO: CMP UPPER 32 BIT OF LONGS, TOO!
			*******************************************************************/

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BGE(0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IRETURN:      /* ..., retvalue ==> ...                      */

			var_to_reg_int(s1, src, REG_RESULT);
			M_TINTMOVE(src->type, s1, REG_RESULT);
			goto nowperformreturn;

		case ICMD_ARETURN:      /* ..., retvalue ==> ...                      */

			var_to_reg_int(s1, src, REG_RESULT);
			M_TINTMOVE(src->type, s1, REG_RESULT);

			if (iptr->val.a) {
				codegen_addpatchref(cd, mcodeptr,
									PATCHER_athrow_areturn,
									(unresolved_class *) iptr->val.a, 0);

				if (opt_showdisassemble)
					M_NOP;
			}
			goto nowperformreturn;

		case ICMD_LRETURN:      /* ..., retvalue ==> ...                      */

			var_to_reg_int(s1, src, PACK_REGS(REG_RESULT2, REG_RESULT));
			M_TINTMOVE(src->type, s1, PACK_REGS(REG_RESULT2, REG_RESULT));
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
				M_MFLR(REG_ZERO);
				M_LDA(REG_SP, REG_SP, -10 * 8);
				M_DST(REG_FRESULT, REG_SP, 48+0);
				M_IST(REG_RESULT, REG_SP, 48+8);
				M_AST(REG_ZERO, REG_SP, 48+12);
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

				disp = dseg_addaddress(cd, m);
				M_ALD(rd->argintregs[0], REG_PV, disp);

				M_FLTMOVE(REG_FRESULT, rd->argfltregs[0]);
				M_FLTMOVE(REG_FRESULT, rd->argfltregs[1]);
				disp = dseg_addaddress(cd, builtin_displaymethodstop);
				M_ALD(REG_ITMP2, REG_PV, disp);
				M_MTCTR(REG_ITMP2);
				M_JSR;

				M_DLD(REG_FRESULT, REG_SP, 48+0);
				M_ILD(REG_RESULT, REG_SP, 48+8);
				M_ALD(REG_ZERO, REG_SP, 48+12);
				M_ILD(REG_RESULT2, REG_SP, 48+16);
				M_LDA(REG_SP, REG_SP, 10 * 8);
				M_MTLR(REG_ZERO);
			}
			
#if defined(USE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				/* we need to save the proper return value */
				switch (iptr->opc) {
				case ICMD_LRETURN:
					M_IST(REG_RESULT2, REG_SP, rd->memuse * 4 + 8);
					/* fall through */
				case ICMD_IRETURN:
				case ICMD_ARETURN:
					M_IST(REG_RESULT , REG_SP, rd->memuse * 4 + 4);
					break;
				case ICMD_FRETURN:
					M_FST(REG_FRESULT, REG_SP, rd->memuse * 4 + 4);
					break;
				case ICMD_DRETURN:
					M_DST(REG_FRESULT, REG_SP, rd->memuse * 4 + 4);
					break;
				}

				disp = dseg_addaddress(cd, BUILTIN_monitorexit);
				M_ALD(REG_ITMP3, REG_PV, disp);
				M_MTCTR(REG_ITMP3);
				M_ALD(rd->argintregs[0], REG_SP, rd->memuse * 4);
				M_JSR;

				/* and now restore the proper return value */
				switch (iptr->opc) {
				case ICMD_LRETURN:
					M_ILD(REG_RESULT2, REG_SP, rd->memuse * 4 + 8);
					/* fall through */
				case ICMD_IRETURN:
				case ICMD_ARETURN:
					M_ILD(REG_RESULT , REG_SP, rd->memuse * 4 + 4);
					break;
				case ICMD_FRETURN:
					M_FLD(REG_FRESULT, REG_SP, rd->memuse * 4 + 4);
					break;
				case ICMD_DRETURN:
					M_DLD(REG_FRESULT, REG_SP, rd->memuse * 4 + 4);
					break;
				}
			}
#endif

			/* restore return address                                         */

			if (!m->isleafmethod) {
				M_ALD(REG_ZERO, REG_SP, p * 4 + LA_LR_OFFSET);
				M_MTLR(REG_ZERO);
			}

			/* restore saved registers                                        */

			for (i = INT_SAV_CNT - 1; i >= rd->savintreguse; i--) {
				p--; M_ILD(rd->savintregs[i], REG_SP, p * 4);
			}
			for (i = FLT_SAV_CNT - 1; i >= rd->savfltreguse; i--) {
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
			codegen_addreference(cd, (basicblock *) tptr[0], mcodeptr);

			/* build jump table top down and use address of lowest entry */

			/* s4ptr += 3 + i; */
			tptr += i;

			while (--i >= 0) {
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
				} else {
					a = dseg_adds4(cd, val);
					M_ILD(REG_ITMP2, REG_PV, a);
					M_CMP(s1, REG_ITMP2);
				}
				M_BEQ(0);
				codegen_addreference(cd, (basicblock *) tptr[0], mcodeptr); 
			}

			M_BR(0);
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
			s3 = md->paramcount;

			MCODECHECK((s3 << 1) + 64);

			/* copy arguments to registers or stack location */

			for (s3 = s3 - 1; s3 >= 0; s3--, src = src->prev) {
				if (src->varkind == ARGVAR)
					continue;
				if (IS_INT_LNG_TYPE(src->type)) {
					if (!md->params[s3].inmemory) {
						if (IS_2_WORD_TYPE(src->type)) {
							s1 = PACK_REGS(
						   rd->argintregs[GET_LOW_REG(md->params[s3].regoff)],
						   rd->argintregs[GET_HIGH_REG(md->params[s3].regoff)]);
						} else {
							s1 = rd->argintregs[md->params[s3].regoff];
						}
						var_to_reg_int(d, src, s1);
						M_TINTMOVE(src->type, d, s1);
					} else {
						var_to_reg_int(d, src, PACK_REGS(REG_ITMP2, REG_ITMP1));
						M_IST(GET_HIGH_REG(d), REG_SP,
							  md->params[s3].regoff * 4);
						if (IS_2_WORD_TYPE(src->type)) {
							M_IST(GET_LOW_REG(d), 
								  REG_SP, md->params[s3].regoff * 4 + 4);
						}
					}
						
				} else {
					if (!md->params[s3].inmemory) {
						s1 = rd->argfltregs[md->params[s3].regoff];
						var_to_reg_flt(d, src, s1);
						M_FLTMOVE(d, s1);
					} else {
						var_to_reg_flt(d, src, REG_FTMP1);
						if (IS_2_WORD_TYPE(src->type)) {
							M_DST(d, REG_SP, md->params[s3].regoff * 4);
						} else {
							M_FST(d, REG_SP, md->params[s3].regoff * 4);
						}
					}
				}
			} /* end of for */

			switch (iptr->opc) {
			case ICMD_BUILTIN:
				if (iptr->target) {
					disp = dseg_addaddress(cd, NULL);

					codegen_addpatchref(cd, mcodeptr, bte->fp, iptr->target,
										disp);

					if (opt_showdisassemble)
						M_NOP;

				} else {
					disp = dseg_addaddress(cd, bte->fp);
				}

				d = md->returntype.type;

				M_ALD(REG_PV, REG_PV, disp);  /* pointer to built-in-function */
				M_MTCTR(REG_PV);
				M_JSR;
				disp = (s4) ((u1 *) mcodeptr - cd->mcodebase);
				M_MFLR(REG_ITMP1);
				M_LDA(REG_PV, REG_ITMP1, -disp);

				/* if op1 == true, we need to check for an exception */

				if (iptr->op1 == true) {
					M_CMPI(REG_RESULT, 0);
					M_BEQ(0);
					codegen_addxexceptionrefs(cd, mcodeptr);
				}
				break;

			case ICMD_INVOKESPECIAL:
				gen_nullptr_check(rd->argintregs[0]);
				M_ILD(REG_ITMP1, rd->argintregs[0], 0); /* hardware nullptr   */
				/* fall through */

			case ICMD_INVOKESTATIC:
				if (!lm) {
					unresolved_method *um = iptr->target;

					disp = dseg_addaddress(cd, NULL);

					codegen_addpatchref(cd, mcodeptr,
										PATCHER_invokestatic_special, um, disp);

					if (opt_showdisassemble)
						M_NOP;

					d = md->returntype.type;

				} else {
					disp = dseg_addaddress(cd, lm->stubroutine);
					d = md->returntype.type;
				}

				M_ALD(REG_PV, REG_PV, disp);
				M_MTCTR(REG_PV);
				M_JSR;
				disp = (s4) ((u1 *) mcodeptr - cd->mcodebase);
				M_MFLR(REG_ITMP1);
				M_LDA(REG_PV, REG_ITMP1, -disp);
				break;

			case ICMD_INVOKEVIRTUAL:
				gen_nullptr_check(rd->argintregs[0]);

				if (!lm) {
					unresolved_method *um = iptr->target;

					codegen_addpatchref(cd, mcodeptr,
										PATCHER_invokevirtual, um, 0);

					if (opt_showdisassemble)
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
				M_MTCTR(REG_PV);
				M_JSR;
				disp = (s4) ((u1 *) mcodeptr - cd->mcodebase);
				M_MFLR(REG_ITMP1);
				M_LDA(REG_PV, REG_ITMP1, -disp);
				break;

			case ICMD_INVOKEINTERFACE:
				gen_nullptr_check(rd->argintregs[0]);

				if (!lm) {
					unresolved_method *um = iptr->target;

					codegen_addpatchref(cd, mcodeptr,
										PATCHER_invokeinterface, um, 0);

					if (opt_showdisassemble)
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
				M_MTCTR(REG_PV);
				M_JSR;
				disp = (s4) ((u1 *) mcodeptr - cd->mcodebase);
				M_MFLR(REG_ITMP1);
				M_LDA(REG_PV, REG_ITMP1, -disp);
				break;
			}

			/* d contains return type */

			if (d != TYPE_VOID) {
				if (IS_INT_LNG_TYPE(iptr->dst->type)) {
					if (IS_2_WORD_TYPE(iptr->dst->type)) {
						s1 = reg_of_var(rd, iptr->dst,
										PACK_REGS(REG_RESULT2, REG_RESULT));
						M_TINTMOVE(iptr->dst->type,
								   PACK_REGS(REG_RESULT2, REG_RESULT), s1);
					} else {
						s1 = reg_of_var(rd, iptr->dst, REG_RESULT);
						M_TINTMOVE(iptr->dst->type, REG_RESULT, s1);
					}
					store_reg_to_var_int(iptr->dst, s1);

				} else {
					s1 = reg_of_var(rd, iptr->dst, REG_FRESULT);
					M_FLTMOVE(REG_FRESULT, s1);
					store_reg_to_var_flt(iptr->dst, s1);
				}
			}
			break;


		case ICMD_CHECKCAST:  /* ..., objectref ==> ..., objectref            */
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
				s2 += (opt_showdisassemble ? 1 : 0);

			/* calculate class checkcast code size */

			s3 = 8 + (s1 == REG_ITMP1);
			if (!super)
				s3 += (opt_showdisassemble ? 1 : 0);

			/* if class is not resolved, check which code to call */

			if (!super) {
				M_TST(s1);
				M_BEQ(3 + (opt_showdisassemble ? 1 : 0) + s2 + 1 + s3);

				disp = dseg_adds4(cd, 0);                     /* super->flags */

				codegen_addpatchref(cd, mcodeptr,
									PATCHER_checkcast_instanceof_flags,
									(constant_classref *) iptr->target, disp);

				if (opt_showdisassemble)
					M_NOP;

				M_ILD(REG_ITMP2, REG_PV, disp);
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
										(constant_classref *) iptr->target, 0);

					if (opt_showdisassemble)
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
				disp = dseg_addaddress(cd, supervftbl);

				if (super) {
					M_TST(s1);
					M_BEQ(s3);

				} else {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_checkcast_class,
										(constant_classref *) iptr->target,
										disp);

					if (opt_showdisassemble)
						M_NOP;
				}

				M_ALD(REG_ITMP2, s1, OFFSET(java_objectheader, vftbl));
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
				codegen_threadcritstart(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
				M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl_t, baseval));
				M_ALD(REG_ITMP2, REG_PV, disp);
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
					M_ALD(REG_ITMP2, REG_PV, disp);
					M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, diffval));
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
					codegen_threadcritstop(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
				}
				M_CMPU(REG_ITMP3, REG_ITMP2);
				M_BGT(0);
				codegen_addxcastrefs(cd, mcodeptr);
			}
			d = reg_of_var(rd, iptr->dst, s1);
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			}
			break;

		case ICMD_ARRAYCHECKCAST: /* ..., objectref ==> ..., objectref        */
		                          /* op1: 1... resolved, 0... not resolved    */

			var_to_reg_int(s1, src, rd->argintregs[0]);
			M_INTMOVE(s1, rd->argintregs[0]);

			bte = iptr->val.a;

			disp = dseg_addaddress(cd, iptr->target);

			if (!iptr->op1) {
				codegen_addpatchref(cd, mcodeptr, bte->fp, iptr->target, disp);

				if (opt_showdisassemble)
					M_NOP;

				a = 0;

			} else {
				a = (ptrint) bte->fp;
			}

			M_ALD(rd->argintregs[1], REG_PV, disp);
			disp = dseg_addaddress(cd, a);
			M_ALD(REG_ITMP2, REG_PV, disp);
			M_MTCTR(REG_ITMP2);
			M_JSR;
			M_TST(REG_RESULT);
			M_BEQ(0);
			codegen_addxcastrefs(cd, mcodeptr);

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, s1);
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_INSTANCEOF: /* ..., objectref ==> ..., intresult            */
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
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if (s1 == d) {
				M_MOV(s1, REG_ITMP1);
				s1 = REG_ITMP1;
			}

			/* calculate interface instanceof code size */

			s2 = 8;
			if (!super)
				s2 += (opt_showdisassemble ? 1 : 0);

			/* calculate class instanceof code size */

			s3 = 10;
			if (!super)
				s3 += (opt_showdisassemble ? 1 : 0);

			M_CLR(d);

			/* if class is not resolved, check which code to call */

			if (!super) {
				M_TST(s1);
				M_BEQ(3 + (opt_showdisassemble ? 1 : 0) + s2 + 1 + s3);

				disp = dseg_adds4(cd, 0);                     /* super->flags */

				codegen_addpatchref(cd, mcodeptr,
									PATCHER_checkcast_instanceof_flags,
									(constant_classref *) iptr->target, disp);

				if (opt_showdisassemble)
					M_NOP;

				M_ILD(REG_ITMP3, REG_PV, disp);
				M_AND_IMM(REG_ITMP3, ACC_INTERFACE, REG_ITMP3);
				M_BEQ(s2 + 1);
			}

			/* interface instanceof code */

			if (!super || (super->flags & ACC_INTERFACE)) {
				if (super) {
					M_TST(s1);
					M_BEQ(s2);

				} else {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_checkcast_instanceof_interface,
										(constant_classref *) iptr->target, 0);

					if (opt_showdisassemble)
						M_NOP;
				}

				M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
				M_ILD(REG_ITMP3, REG_ITMP1, OFFSET(vftbl_t, interfacetablelength));
				M_LDATST(REG_ITMP3, REG_ITMP3, -superindex);
				M_BLE(4);
				M_ALD(REG_ITMP1, REG_ITMP1,
					  OFFSET(vftbl_t, interfacetable[0]) -
					  superindex * sizeof(methodptr*));
				M_TST(REG_ITMP1);
				M_BEQ(1);
				M_IADD_IMM(REG_ZERO, 1, d);

				if (!super)
					M_BR(s3);
			}

			/* class instanceof code */

			if (!super || !(super->flags & ACC_INTERFACE)) {
				disp = dseg_addaddress(cd, supervftbl);

				if (super) {
					M_TST(s1);
					M_BEQ(s3);

				} else {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_instanceof_class,
										(constant_classref *) iptr->target,
										disp);

					if (opt_showdisassemble) {
						M_NOP;
					}
				}

				M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
				M_ALD(REG_ITMP2, REG_PV, disp);
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

		case ICMD_MULTIANEWARRAY:/* ..., cnt1, [cnt2, ...] ==> ..., arrayref  */
		                      /* op1 = dimension, val.a = array descriptor    */

			/* check for negative sizes and copy sizes to stack if necessary  */

			MCODECHECK((iptr->op1 << 1) + 64);

			for (s1 = iptr->op1; --s1 >= 0; src = src->prev) {
				/* copy SAVEDVAR sizes to stack */

				if (src->varkind != ARGVAR) {
					var_to_reg_int(s2, src, REG_ITMP1);
#if defined(__DARWIN__)
					M_IST(s2, REG_SP, LA_SIZE + (s1 + INT_ARG_CNT) * 4);
#else
					M_IST(s2, REG_SP, LA_SIZE + (s1 + 3) * 4);
#endif
				}
			}

			/* a0 = dimension count */

			ICONST(rd->argintregs[0], iptr->op1);

			/* is patcher function set? */

			if (iptr->target) {
				disp = dseg_addaddress(cd, NULL);

				codegen_addpatchref(cd, mcodeptr,
									(functionptr) (ptrint) iptr->target,
									iptr->val.a, disp);

				if (opt_showdisassemble)
					M_NOP;

			} else {
				disp = dseg_addaddress(cd, iptr->val.a);
			}

			/* a1 = arraydescriptor */

			M_ALD(rd->argintregs[1], REG_PV, disp);

			/* a2 = pointer to dimensions = stack pointer */

#if defined(__DARWIN__)
			M_LDA(rd->argintregs[2], REG_SP, LA_SIZE + INT_ARG_CNT * 4);
#else
			M_LDA(rd->argintregs[2], REG_SP, LA_SIZE + 3 * 4);
#endif

			disp = dseg_addaddress(cd, BUILTIN_multianewarray);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_MTCTR(REG_ITMP3);
			M_JSR;

			/* check for exception before result assignment */

			M_CMPI(REG_RESULT, 0);
			M_BEQ(0);
			codegen_addxexceptionrefs(cd, mcodeptr);

			d = reg_of_var(rd, iptr->dst, REG_RESULT);
			M_INTMOVE(REG_RESULT, d);
			store_reg_to_var_int(iptr->dst, d);
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
					if (IS_2_WORD_TYPE(s2)) {
						M_IST(GET_HIGH_REG(s1),
							  REG_SP, rd->interfaces[len][s2].regoff * 4);
						M_IST(GET_LOW_REG(s1), REG_SP,
							  rd->interfaces[len][s2].regoff * 4 + 4);
					} else {
						M_IST(s1, REG_SP, rd->interfaces[len][s2].regoff * 4);
					}

				}
			}
		}
		src = src->prev;
	}
	} /* if (bptr -> flags >= BBREACHED) */
	} /* for basic block */

	codegen_createlinenumbertable(cd);

	{

	s4        *xcodeptr;
	branchref *bref;

	/* generate ArithemticException check stubs */

	xcodeptr = NULL;

	for (bref = cd->xdivrefs; bref != NULL; bref = bref->next) {
		if ((m->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  (u1 *) xcodeptr - (u1 *) cd->mcodebase - 4);
			continue;
		}

		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(20);

		M_LDA(REG_ITMP2_XPC, REG_PV, bref->branchpos - 4);

		if (xcodeptr != NULL) {
			disp = xcodeptr - mcodeptr - 1;
			M_BR(disp);

		} else {
			xcodeptr = mcodeptr;

			if (m->isleafmethod) {
				M_MFLR(REG_ZERO);
				M_AST(REG_ZERO, REG_SP, parentargs_base * 4 + LA_LR_OFFSET);
			}

			M_MOV(REG_PV, rd->argintregs[0]);
			M_MOV(REG_SP, rd->argintregs[1]);

			if (m->isleafmethod)
				M_MOV(REG_ZERO, rd->argintregs[2]);
			else
				M_ALD(rd->argintregs[2],
					  REG_SP, parentargs_base * 4 + LA_LR_OFFSET);

			M_MOV(REG_ITMP2_XPC, rd->argintregs[3]);

			M_STWU(REG_SP, REG_SP, -(LA_SIZE + 5 * 4));
			M_AST(REG_ITMP2_XPC, REG_SP, LA_SIZE + 4 * 4);

			disp = dseg_addaddress(cd, stacktrace_inline_arithmeticexception);
			M_ALD(REG_ITMP1, REG_PV, disp);
			M_MTCTR(REG_ITMP1);
			M_JSR;
			M_MOV(REG_RESULT, REG_ITMP1_XPTR);

			M_ALD(REG_ITMP2_XPC, REG_SP, LA_SIZE + 4 * 4);
			M_IADD_IMM(REG_SP, LA_SIZE + 5 * 4, REG_SP);

			if (m->isleafmethod) {
				M_ALD(REG_ZERO, REG_SP, parentargs_base * 4 + LA_LR_OFFSET);
				M_MTLR(REG_ZERO);
			}

			disp = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_MTCTR(REG_ITMP3);
			M_RTS;
		}
	}

	/* generate ArrayIndexOutOfBoundsException stubs */

	xcodeptr = NULL;

	for (bref = cd->xboundrefs; bref != NULL; bref = bref->next) {
		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(21);

		/* move index register into REG_ITMP1 */

		M_MOV(bref->reg, REG_ITMP1);

		M_LDA(REG_ITMP2_XPC, REG_PV, bref->branchpos - 4);

		if (xcodeptr != NULL) {
			disp = xcodeptr - mcodeptr - 1;
			M_BR(disp);

		} else {
			xcodeptr = mcodeptr;

			if (m->isleafmethod) {
				M_MFLR(REG_ZERO);
				M_AST(REG_ZERO, REG_SP, parentargs_base * 4 + LA_LR_OFFSET);
			}

			M_MOV(REG_PV, rd->argintregs[0]);
			M_MOV(REG_SP, rd->argintregs[1]);

			if (m->isleafmethod)
				M_MOV(REG_ZERO, rd->argintregs[2]);
			else
				M_ALD(rd->argintregs[2],
					  REG_SP, parentargs_base * 4 + LA_LR_OFFSET);

			M_MOV(REG_ITMP2_XPC, rd->argintregs[3]);
			M_MOV(REG_ITMP1, rd->argintregs[4]);

			M_STWU(REG_SP, REG_SP, -(LA_SIZE + 6 * 4));
			M_AST(REG_ITMP2_XPC, REG_SP, LA_SIZE + 5 * 4);

			disp = dseg_addaddress(cd, stacktrace_inline_arrayindexoutofboundsexception);
			M_ALD(REG_ITMP1, REG_PV, disp);
			M_MTCTR(REG_ITMP1);
			M_JSR;
			M_MOV(REG_RESULT, REG_ITMP1_XPTR);

			M_ALD(REG_ITMP2_XPC, REG_SP, LA_SIZE + 5 * 4);
			M_IADD_IMM(REG_SP, LA_SIZE + 6 * 4, REG_SP);

			if (m->isleafmethod) {
				M_ALD(REG_ZERO, REG_SP, parentargs_base * 4 + LA_LR_OFFSET);
				M_MTLR(REG_ZERO);
			}

			disp = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_MTCTR(REG_ITMP3);
			M_RTS;
		}
	}

	/* generate ArrayStoreException check stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xstorerefs; bref != NULL; bref = bref->next) {
		if ((m->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  (u1 *) xcodeptr - (u1 *) cd->mcodebase - 4);
			continue;
		}

		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(15);

		M_LDA(REG_ITMP2_XPC, REG_PV, bref->branchpos - 4);

		if (xcodeptr != NULL) {
			disp = xcodeptr - mcodeptr - 1;
			M_BR(disp);

		} else {
			xcodeptr = mcodeptr;

			M_MOV(REG_PV, rd->argintregs[0]);
			M_MOV(REG_SP, rd->argintregs[1]);
			M_ALD(rd->argintregs[2],
				  REG_SP, parentargs_base * 4 + LA_LR_OFFSET);
			M_MOV(REG_ITMP2_XPC, rd->argintregs[3]);

			M_STWU(REG_SP, REG_SP, -(LA_SIZE + 5 * 4));
			M_AST(REG_ITMP2_XPC, REG_SP, LA_SIZE + 4 * 4);

			disp = dseg_addaddress(cd, stacktrace_inline_arraystoreexception);
			M_ALD(REG_ITMP1, REG_PV, disp);
			M_MTCTR(REG_ITMP1);
			M_JSR;
			M_MOV(REG_RESULT, REG_ITMP1_XPTR);

			M_ALD(REG_ITMP2_XPC, REG_SP, LA_SIZE + 4 * 4);
			M_IADD_IMM(REG_SP, LA_SIZE + 5 * 4, REG_SP);

			disp = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_MTCTR(REG_ITMP3);
			M_RTS;
		}
	}

	/* generate ClassCastException stubs */

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

		MCODECHECK(20);

		M_LDA(REG_ITMP2_XPC, REG_PV, bref->branchpos - 4);

		if (xcodeptr != NULL) {
			disp = xcodeptr - mcodeptr - 1;
			M_BR(disp);

		} else {
			xcodeptr = mcodeptr;

			if (m->isleafmethod) {
				M_MFLR(REG_ZERO);
				M_AST(REG_ZERO, REG_SP, parentargs_base * 4 + LA_LR_OFFSET);
			}

			M_MOV(REG_PV, rd->argintregs[0]);
			M_MOV(REG_SP, rd->argintregs[1]);

			if (m->isleafmethod)
				M_MOV(REG_ZERO, rd->argintregs[2]);
			else
				M_ALD(rd->argintregs[2],
					  REG_SP, parentargs_base * 4 + LA_LR_OFFSET);

			M_MOV(REG_ITMP2_XPC, rd->argintregs[3]);

			M_STWU(REG_SP, REG_SP, -(LA_SIZE + 5 * 4));
			M_AST(REG_ITMP2_XPC, REG_SP, LA_SIZE + 4 * 4);

			disp = dseg_addaddress(cd, stacktrace_inline_classcastexception);
			M_ALD(REG_ITMP1, REG_PV, disp);
			M_MTCTR(REG_ITMP1);
			M_JSR;
			M_MOV(REG_RESULT, REG_ITMP1_XPTR);

			M_ALD(REG_ITMP2_XPC, REG_SP, LA_SIZE + 4 * 4);
			M_IADD_IMM(REG_SP, LA_SIZE + 5 * 4, REG_SP);

			if (m->isleafmethod) {
				M_ALD(REG_ZERO, REG_SP, parentargs_base * 4 + LA_LR_OFFSET);
				M_MTLR(REG_ZERO);
			}

			disp = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_MTCTR(REG_ITMP3);
			M_RTS;
		}
	}

	/* generate NullPointerException stubs */

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

		MCODECHECK(20);

		M_LDA(REG_ITMP2_XPC, REG_PV, bref->branchpos - 4);

		if (xcodeptr != NULL) {
			disp = xcodeptr - mcodeptr - 1;
			M_BR(disp);

		} else {
			xcodeptr = mcodeptr;

			if (m->isleafmethod) {
				M_MFLR(REG_ZERO);
				M_AST(REG_ZERO, REG_SP, parentargs_base * 4 + LA_LR_OFFSET);
			}

			M_MOV(REG_PV, rd->argintregs[0]);
			M_MOV(REG_SP, rd->argintregs[1]);

			if (m->isleafmethod)
				M_MOV(REG_ZERO, rd->argintregs[2]);
			else
				M_ALD(rd->argintregs[2],
					  REG_SP, parentargs_base * 4 + LA_LR_OFFSET);

			M_MOV(REG_ITMP2_XPC, rd->argintregs[3]);

			M_STWU(REG_SP, REG_SP, -(LA_SIZE + 5 * 4));
			M_AST(REG_ITMP2_XPC, REG_SP, LA_SIZE + 4 * 4);

			disp = dseg_addaddress(cd, stacktrace_inline_nullpointerexception);
			M_ALD(REG_ITMP1, REG_PV, disp);
			M_MTCTR(REG_ITMP1);
			M_JSR;
			M_MOV(REG_RESULT, REG_ITMP1_XPTR);

			M_ALD(REG_ITMP2_XPC, REG_SP, LA_SIZE + 4 * 4);
			M_IADD_IMM(REG_SP, LA_SIZE + 5 * 4, REG_SP);

			if (m->isleafmethod) {
				M_ALD(REG_ZERO, REG_SP, parentargs_base * 4 + LA_LR_OFFSET);
				M_MTLR(REG_ZERO);
			}

			disp = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, disp);
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

		MCODECHECK(16);

		M_LDA(REG_ITMP2_XPC, REG_PV, bref->branchpos - 4);

		if (xcodeptr != NULL) {
			disp = xcodeptr - mcodeptr - 1;
			M_BR(disp);

		} else {
			xcodeptr = mcodeptr;

			M_MOV(REG_PV, rd->argintregs[0]);
			M_MOV(REG_SP, rd->argintregs[1]);
			M_ALD(rd->argintregs[2],
				  REG_SP, parentargs_base * 4 + LA_LR_OFFSET);
			M_MOV(REG_ITMP2_XPC, rd->argintregs[3]);

			M_STWU(REG_SP, REG_SP, -(LA_SIZE + 5 * 4));
			M_AST(REG_ITMP2_XPC, REG_SP, LA_SIZE + 4 * 4);

			disp = dseg_addaddress(cd, stacktrace_inline_fillInStackTrace);
			M_ALD(REG_ITMP1, REG_PV, disp);
			M_MTCTR(REG_ITMP1);
			M_JSR;
			M_MOV(REG_RESULT, REG_ITMP1_XPTR);

			M_ALD(REG_ITMP2_XPC, REG_SP, LA_SIZE + 4 * 4);
			M_IADD_IMM(REG_SP, LA_SIZE + 5 * 4, REG_SP);

			disp = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, disp);
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

			M_BR(tmpmcodeptr - (xcodeptr + 1));

			mcodeptr = tmpmcodeptr;         /* restore the current mcodeptr   */

			/* create stack frame - keep stack 16-byte aligned */

			M_AADD_IMM(REG_SP, -8 * 4, REG_SP);

			/* calculate return address and move it onto the stack */

			M_LDA(REG_ITMP3, REG_PV, pref->branchpos);
			M_AST_INTERN(REG_ITMP3, REG_SP, 5 * 4);

			/* move pointer to java_objectheader onto stack */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			/* order reversed because of data segment layout */

			(void) dseg_addaddress(cd, get_dummyLR());          /* monitorPtr */
			disp = dseg_addaddress(cd, NULL);                   /* vftbl      */

			M_LDA(REG_ITMP3, REG_PV, disp);
			M_AST_INTERN(REG_ITMP3, REG_SP, 4 * 4);
#else
			/* do nothing */
#endif

			/* move machine code onto stack */

			disp = dseg_adds4(cd, mcode);
			M_ILD(REG_ITMP3, REG_PV, disp);
			M_IST_INTERN(REG_ITMP3, REG_SP, 3 * 4);

			/* move class/method/field reference onto stack */

			disp = dseg_addaddress(cd, pref->ref);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_AST_INTERN(REG_ITMP3, REG_SP, 2 * 4);

			/* move data segment displacement onto stack */

			disp = dseg_addaddress(cd, pref->disp);
			M_ILD(REG_ITMP3, REG_PV, disp);
			M_IST_INTERN(REG_ITMP3, REG_SP, 1 * 4);

			/* move patcher function pointer onto stack */

			disp = dseg_addaddress(cd, pref->patcher);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_AST_INTERN(REG_ITMP3, REG_SP, 0 * 4);

			disp = dseg_addaddress(cd, asm_wrapper_patcher);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_MTCTR(REG_ITMP3);
			M_RTS;
		}
	}

	}

	codegen_finish(m, cd, (ptrint) ((u1 *) mcodeptr - cd->mcodebase));

	asm_cacheflush((void *) (ptrint) m->entrypoint, ((u1 *) mcodeptr - cd->mcodebase));
}


/* createcompilerstub **********************************************************

   Creates a stub routine which calls the compiler.
	
*******************************************************************************/

#define COMPSTUBSIZE 6

functionptr createcompilerstub(methodinfo *m)
{
	s4 *s = CNEW(s4, COMPSTUBSIZE);     /* memory to hold the stub            */
	s4 *mcodeptr = s;                   /* code generation pointer            */

	M_LDA(REG_ITMP1, REG_PV, 4 * 4);
	M_ALD_INTERN(REG_PV, REG_PV, 5 * 4);
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
							 registerdata *rd, methoddesc *nmd)
{
	s4         *mcodeptr;               /* code generation pointer            */
	s4          stackframesize;         /* size of stackframe if needed       */
	methoddesc *md;
	s4          nativeparams;
	s4          i, j;                   /* count variables                    */
	s4          t;
	s4          s1, s2, disp;
	s4          funcdisp;

	/* set some variables */

	md = m->parseddesc;
	nativeparams = (m->flags & ACC_STATIC) ? 2 : 1;

	/* calculate stackframe size */

	stackframesize =
		sizeof(stackframeinfo) / SIZEOF_VOID_P +
		sizeof(localref_table) / SIZEOF_VOID_P +
		4 +                             /* 4 stackframeinfo arguments (darwin)*/
		nmd->paramcount * 2 +           /* assume all arguments are doubles   */
		nmd->memuse;

	stackframesize = (stackframesize + 3) & ~3; /* keep stack 16-byte aligned */


	/* create method header */

	(void) dseg_addaddress(cd, m);                          /* MethodPointer  */
	(void) dseg_adds4(cd, stackframesize * 4);              /* FrameSize      */
	(void) dseg_adds4(cd, 0);                               /* IsSync         */
	(void) dseg_adds4(cd, 0);                               /* IsLeaf         */
	(void) dseg_adds4(cd, 0);                               /* IntSave        */
	(void) dseg_adds4(cd, 0);                               /* FltSave        */
	(void) dseg_addlinenumbertablesize(cd);
	(void) dseg_adds4(cd, 0);                               /* ExTableSize    */


	/* initialize mcode variables */
	
	mcodeptr = (s4 *) cd->mcodebase;
	cd->mcodeend = (s4 *) (cd->mcodebase + cd->mcodesize);


	/* generate code */

	M_MFLR(REG_ZERO);
	M_AST_INTERN(REG_ZERO, REG_SP, LA_LR_OFFSET);
	M_STWU(REG_SP, REG_SP, -(stackframesize * 4));


	if (runverbose) {
		/* parent_argbase == stackframesize * 4 */
		mcodeptr = codegen_trace_args(m, cd, rd, mcodeptr, stackframesize * 4 , 
									  true);
	}


	/* get function address (this must happen before the stackframeinfo) */

	funcdisp = dseg_addaddress(cd, f);

#if !defined(ENABLE_STATICVM)
	if (f == NULL) {
		codegen_addpatchref(cd, mcodeptr, PATCHER_resolve_native, m, funcdisp);

		if (opt_showdisassemble)
			M_NOP;
	}
#endif

	/* save integer and float argument registers */

	for (i = 0, j = 0; i < md->paramcount && i < INT_ARG_CNT; i++) {
		t = md->paramtypes[i].type;

		if (IS_INT_LNG_TYPE(t)) {
			s1 = md->params[i].regoff;
			if (IS_2_WORD_TYPE(t)) {
				M_IST(rd->argintregs[GET_HIGH_REG(s1)], REG_SP, LA_SIZE + 4 * 4 + j * 4);
				j++;
				M_IST(rd->argintregs[GET_LOW_REG(s1)], REG_SP, LA_SIZE + 4 * 4 + j * 4);
				j++;
			} else {
				M_IST(rd->argintregs[s1], REG_SP, LA_SIZE + 4 * 4 + j * 4);
				j++;
			}
		}
	}

	for (i = 0; i < md->paramcount && i < FLT_ARG_CNT; i++) {
		if (IS_FLT_DBL_TYPE(md->paramtypes[i].type)) {
			s1 = md->params[i].regoff;
			M_DST(rd->argfltregs[s1], REG_SP, LA_SIZE + 4 * 4 + j * 8);
			j++;
		}
	}

	/* create native stack info */

	M_AADD_IMM(REG_SP, stackframesize * 4, rd->argintregs[0]);
	M_MOV(REG_PV, rd->argintregs[1]);
	M_AADD_IMM(REG_SP, stackframesize * 4, rd->argintregs[2]);
	M_ALD(rd->argintregs[3], REG_SP, stackframesize * 4 + LA_LR_OFFSET);
	disp = dseg_addaddress(cd, codegen_start_native_call);
	M_ALD(REG_ITMP1, REG_PV, disp);
	M_MTCTR(REG_ITMP1);
	M_JSR;

	/* restore integer and float argument registers */

	for (i = 0, j = 0; i < md->paramcount && i < INT_ARG_CNT; i++) {
		t = md->paramtypes[i].type;

		if (IS_INT_LNG_TYPE(t)) {
			s1 = md->params[i].regoff;

			if (IS_2_WORD_TYPE(t)) {
				M_ILD(rd->argintregs[GET_HIGH_REG(s1)], REG_SP, LA_SIZE + 4 * 4 + j * 4);
				j++;
				M_ILD(rd->argintregs[GET_LOW_REG(s1)], REG_SP, LA_SIZE + 4 * 4 + j * 4);
				j++;
			} else {
				M_ILD(rd->argintregs[s1], REG_SP, LA_SIZE + 4 * 4 + j * 4);
				j++;
			}
		}
	}

	for (i = 0; i < md->paramcount && i < FLT_ARG_CNT; i++) {
		if (IS_FLT_DBL_TYPE(md->paramtypes[i].type)) {
			s1 = md->params[i].regoff;
			M_DLD(rd->argfltregs[i], REG_SP, LA_SIZE + 4 * 4 + j * 8);
			j++;
		}
	}
	
	/* copy or spill arguments to new locations */

	for (i = md->paramcount - 1, j = i + nativeparams; i >= 0; i--, j--) {
		t = md->paramtypes[i].type;

		if (IS_INT_LNG_TYPE(t)) {
			if (!md->params[i].inmemory) {
				if (IS_2_WORD_TYPE(t))
					s1 = PACK_REGS(
						rd->argintregs[GET_LOW_REG(md->params[i].regoff)],
					    rd->argintregs[GET_HIGH_REG(md->params[i].regoff)]);
				else
					s1 = rd->argintregs[md->params[i].regoff];

				if (!nmd->params[j].inmemory) {
					if (IS_2_WORD_TYPE(t))
						s2 = PACK_REGS(
						   rd->argintregs[GET_LOW_REG(nmd->params[j].regoff)],
						   rd->argintregs[GET_HIGH_REG(nmd->params[j].regoff)]);
					else
						s2 = rd->argintregs[nmd->params[j].regoff];
					M_TINTMOVE(t, s1, s2);

				} else {
					s2 = nmd->params[j].regoff;
					if (IS_2_WORD_TYPE(t)) {
						M_IST(GET_HIGH_REG(s1), REG_SP, s2 * 4);
						M_IST(GET_LOW_REG(s1), REG_SP, s2 * 4 + 4);
					} else {
						M_IST(s1, REG_SP, s2 * 4);
					}
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
		disp = dseg_addaddress(cd, m->class);
		M_ALD(rd->argintregs[1], REG_PV, disp);
	}

	/* put env into first argument register */

	disp = dseg_addaddress(cd, &env);
	M_ALD(rd->argintregs[0], REG_PV, disp);

	/* generate the actual native call */

	M_ALD(REG_ITMP3, REG_PV, funcdisp);
	M_MTCTR(REG_ITMP3);
	M_JSR;

	/* save return value */

	if (IS_INT_LNG_TYPE(md->returntype.type)) {
		if (IS_2_WORD_TYPE(md->returntype.type))
			M_IST(REG_RESULT2, REG_SP, LA_SIZE + 2 * 4);
		M_IST(REG_RESULT, REG_SP, LA_SIZE + 1 * 4);
	} else {
		if (IS_2_WORD_TYPE(md->returntype.type))
			M_DST(REG_FRESULT, REG_SP, LA_SIZE + 1 * 4);
		else
			M_FST(REG_FRESULT, REG_SP, LA_SIZE + 1 * 4);
	}

	/* remove native stackframe info */

	M_AADD_IMM(REG_SP, stackframesize * 4, rd->argintregs[0]);
	disp = dseg_addaddress(cd, codegen_finish_native_call);
	M_ALD(REG_ITMP1, REG_PV, disp);
	M_MTCTR(REG_ITMP1);
	M_JSR;

	/* print call trace */

	if (runverbose) {
		 /* just restore the value we need, don't care about the other */

		if (IS_INT_LNG_TYPE(md->returntype.type)) {
			if (IS_2_WORD_TYPE(md->returntype.type))
				M_ILD(REG_RESULT2, REG_SP, LA_SIZE + 2 * 4);
			M_ILD(REG_RESULT, REG_SP, LA_SIZE + 1 * 4);
		} else {
			if (IS_2_WORD_TYPE(md->returntype.type))
				M_DLD(REG_FRESULT, REG_SP, LA_SIZE + 1 * 4);
			else
				M_FLD(REG_FRESULT, REG_SP, LA_SIZE + 1 * 4);
		}

		M_LDA(REG_SP, REG_SP, -(LA_SIZE + (1 + 2 + 2 + 1) * 4));

		/* keep this order */
		switch (md->returntype.type) {
		case TYPE_INT:
		case TYPE_ADR:
#if defined(__DARWIN__)
			M_MOV(REG_RESULT, rd->argintregs[2]);
			M_CLR(rd->argintregs[1]);
#else
			M_MOV(REG_RESULT, rd->argintregs[3]);
			M_CLR(rd->argintregs[2]);
#endif
			break;

		case TYPE_LNG:
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
		disp = dseg_addaddress(cd, m);
		M_ALD(rd->argintregs[0], REG_PV, disp);

		disp = dseg_addaddress(cd, builtin_displaymethodstop);
		M_ALD(REG_ITMP2, REG_PV, disp);
		M_MTCTR(REG_ITMP2);
		M_JSR;

		M_LDA(REG_SP, REG_SP, LA_SIZE + (1 + 2 + 2 + 1) * 4);
	}

	/* check for exception */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	disp = dseg_addaddress(cd, builtin_get_exceptionptrptr);
	M_ALD(REG_ITMP1, REG_PV, disp);
	M_MTCTR(REG_ITMP1);
	M_JSR;
	M_MOV(REG_RESULT, REG_ITMP2);
#else
	disp = dseg_addaddress(cd, &_no_threads_exceptionptr);
	M_ALD(REG_ITMP2, REG_PV, disp);
#endif
	M_ALD(REG_ITMP1_XPTR, REG_ITMP2, 0);/* load exception into reg. itmp1     */

	/* restore return value */

	if (IS_INT_LNG_TYPE(md->returntype.type)) {
		if (IS_2_WORD_TYPE(md->returntype.type))
			M_ILD(REG_RESULT2, REG_SP, LA_SIZE + 2 * 4);
		M_ILD(REG_RESULT, REG_SP, LA_SIZE + 1 * 4);
	} else {
		if (IS_2_WORD_TYPE(md->returntype.type))
			M_DLD(REG_FRESULT, REG_SP, LA_SIZE + 1 * 4);
		else
			M_FLD(REG_FRESULT, REG_SP, LA_SIZE + 1 * 4);
	}

	M_TST(REG_ITMP1_XPTR);
	M_BNE(4);                           /* if no exception then return        */

	M_ALD(REG_ZERO, REG_SP, stackframesize * 4 + LA_LR_OFFSET); /* load ra   */
	M_MTLR(REG_ZERO);
	M_LDA(REG_SP, REG_SP, stackframesize * 4); /* remove stackframe           */

	M_RET;

	/* handle exception */

	M_CLR(REG_ITMP3);
	M_AST(REG_ITMP3, REG_ITMP2, 0);     /* store NULL into exceptionptr       */

	M_ALD(REG_ITMP2, REG_SP, stackframesize * 4 + LA_LR_OFFSET); /* load ra   */
	M_MTLR(REG_ITMP2);

	M_LDA(REG_SP, REG_SP, stackframesize * 4); /* remove stackframe           */

	M_IADD_IMM(REG_ITMP2, -4, REG_ITMP2_XPC);  /* fault address               */

	disp = dseg_addaddress(cd, asm_handle_nat_exception);
	M_ALD(REG_ITMP3, REG_PV, disp);
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

			M_MFLR(REG_ZERO);
			M_AST(REG_ZERO, REG_SP, 5 * 4);

			/* move pointer to java_objectheader onto stack */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			/* order reversed because of data segment layout */

			(void) dseg_addaddress(cd, get_dummyLR());          /* monitorPtr */
			disp = dseg_addaddress(cd, NULL);                   /* vftbl      */

			M_LDA(REG_ITMP3, REG_PV, disp);
			M_AST(REG_ITMP3, REG_SP, 4 * 4);
#else
			/* do nothing */
#endif

			/* move machine code onto stack */

			disp = dseg_adds4(cd, mcode);
			M_ILD(REG_ITMP3, REG_PV, disp);
			M_IST(REG_ITMP3, REG_SP, 3 * 4);

			/* move class/method/field reference onto stack */

			disp = dseg_addaddress(cd, pref->ref);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_AST(REG_ITMP3, REG_SP, 2 * 4);

			/* move data segment displacement onto stack */

			disp = dseg_addaddress(cd, pref->disp);
			M_ILD(REG_ITMP3, REG_PV, disp);
			M_IST(REG_ITMP3, REG_SP, 1 * 4);

			/* move patcher function pointer onto stack */

			disp = dseg_addaddress(cd, pref->patcher);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_AST(REG_ITMP3, REG_SP, 0 * 4);

			disp = dseg_addaddress(cd, asm_wrapper_patcher);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_MTCTR(REG_ITMP3);
			M_RTS;
		}
	}

	codegen_finish(m, cd, (s4) ((u1 *) mcodeptr - cd->mcodebase));

	asm_cacheflush((void *) (ptrint) m->entrypoint, ((u1 *) mcodeptr - cd->mcodebase));

	return m->entrypoint;
}


s4 *codegen_trace_args(methodinfo *m, codegendata *cd, registerdata *rd,
					   s4 *mcodeptr, s4 parentargs_base, bool nativestub)
{
	s4 s1, p, t, d;
	int stack_off;
	int stack_size;
	methoddesc *md;

	md = m->parseddesc;
	
	if (!nativestub)
		M_MFLR(REG_ITMP3);
	/* Build up Stackframe for builtin_trace_args call (a multiple of 16) */
	/* For Darwin:                                                        */
	/* LA + TRACE_ARGS_NUM u8 args + methodinfo + LR                      */
	/* LA_SIZE(=6*4) + 8*8         + 4          + 4  + 0(Padding)         */
	/* 6 * 4 + 8 * 8 + 2 * 4 = 12 * 8 = 6 * 16                            */
	/* For Linux:                                                         */
	/* LA + (TRACE_ARGS_NUM - INT_ARG_CNT/2) u8 args + methodinfo         */
	/* + INT_ARG_CNT * 4 ( save integer registers) + LR + 8 + 8 (Padding) */
	/* LA_SIZE(=2*4) + 4 * 8 + 4 + 8 * 4 + 4 + 8                          */
	/* 2 * 4 + 4 * 8 + 10 * 4 + 1 * 8 + 8= 12 * 8 = 6 * 16                */
	
	/* in nativestubs no Place to save the LR (Link Register) would be needed */
	/* but since the stack frame has to be aligned the 4 Bytes would have to  */
	/* be padded again */

#if defined(__DARWIN__)
	stack_size = LA_SIZE + (TRACE_ARGS_NUM + 1) * 8;
#else
	stack_size = 6 * 16;
#endif
	M_LDA(REG_SP, REG_SP, -stack_size);

	/* Save LR */
	if (!nativestub)
		M_IST(REG_ITMP3, REG_SP, LA_SIZE + TRACE_ARGS_NUM * 8 + 1 * 4);

	M_CLR(REG_ITMP1);    /* clear help register */

	/* save up to TRACE_ARGS_NUM arguments into the reserved stack space */
#if defined(__DARWIN__)
	/* Copy Params starting from first to Stack                          */
	/* since TRACE_ARGS == INT_ARG_CNT all used integer argument regs    */ 
	/* are saved                                                         */
	p = 0;
#else
	/* Copy Params starting from fifth to Stack (INT_ARG_CNT/2) are in   */
	/* integer argument regs                                             */
	/* all integer argument registers have to be saved                   */
	for (p = 0; p < 8; p++) {
		d = rd->argintregs[p];
		/* save integer argument registers */
		M_IST(d, REG_SP, LA_SIZE + 4 * 8 + 4 + p * 4);
	}
	p = 4;
#endif
	stack_off = LA_SIZE;
	for (; p < md->paramcount && p < TRACE_ARGS_NUM; p++, stack_off += 8) {
		t = md->paramtypes[p].type;
		if (IS_INT_LNG_TYPE(t)) {
			if (!md->params[p].inmemory) { /* Param in Arg Reg */
				if (IS_2_WORD_TYPE(t)) {
					M_IST(rd->argintregs[GET_HIGH_REG(md->params[p].regoff)]
						  , REG_SP, stack_off);
					M_IST(rd->argintregs[GET_LOW_REG(md->params[p].regoff)]
						  , REG_SP, stack_off + 4);
				} else {
					M_IST(REG_ITMP1, REG_SP, stack_off);
					M_IST(rd->argintregs[md->params[p].regoff]
						  , REG_SP, stack_off + 4);
				}
			} else { /* Param on Stack */
				s1 = (md->params[p].regoff + parentargs_base) * 4 
					+ stack_size;
				if (IS_2_WORD_TYPE(t)) {
					M_ILD(REG_ITMP2, REG_SP, s1);
					M_IST(REG_ITMP2, REG_SP, stack_off);
					M_ILD(REG_ITMP2, REG_SP, s1 + 4);
					M_IST(REG_ITMP2, REG_SP, stack_off + 4);
				} else {
					M_IST(REG_ITMP1, REG_SP, stack_off);
					M_ILD(REG_ITMP2, REG_SP, s1);
					M_IST(REG_ITMP2, REG_SP, stack_off + 4);
				}
			}
		} else { /* IS_FLT_DBL_TYPE(t) */
			if (!md->params[p].inmemory) { /* in Arg Reg */
				s1 = rd->argfltregs[md->params[p].regoff];
				if (!IS_2_WORD_TYPE(t)) {
					M_IST(REG_ITMP1, REG_SP, stack_off);
					M_FST(s1, REG_SP, stack_off + 4);
				} else {
					M_DST(s1, REG_SP, stack_off);
				}
			} else { /* on Stack */
				/* this should not happen */
			}
		}
	}

	/* load first 4 (==INT_ARG_CNT/2) arguments into integer registers */
#if defined(__DARWIN__)
	for (p = 0; p < 8; p++) {
		d = rd->argintregs[p];
		M_ILD(d, REG_SP, LA_SIZE + p * 4);
	}
#else
	/* LINUX */
	/* Set integer and float argument registers vor trace_args call */
	/* offset to saved integer argument registers                   */
	stack_off = LA_SIZE + 4 * 8 + 4;
	for (p = 0; (p < 4) && (p < md->paramcount); p++) {
		t = md->paramtypes[p].type;
		if (IS_INT_LNG_TYPE(t)) {
			/* "stretch" int types */
			if (!IS_2_WORD_TYPE(t)) {
				M_CLR(rd->argintregs[2 * p]);
				M_ILD(rd->argintregs[2 * p + 1], REG_SP,stack_off);
				stack_off += 4;
			} else {
				M_ILD(rd->argintregs[2 * p + 1], REG_SP,stack_off + 4);
				M_ILD(rd->argintregs[2 * p], REG_SP,stack_off);
				stack_off += 8;
			}
		} else { /* Float/Dbl */
			if (!md->params[p].inmemory) { /* Param in Arg Reg */
				/* use reserved Place on Stack (sp + 5 * 16) to copy  */
				/* float/double arg reg to int reg                    */
				s1 = rd->argfltregs[md->params[p].regoff];
				if (!IS_2_WORD_TYPE(t)) {
					M_FST(s1, REG_SP, 5 * 16);
					M_ILD(rd->argintregs[2 * p + 1], REG_SP, 5 * 16);
					M_CLR(rd->argintregs[2 * p]);
				} else {
					M_DST(s1, REG_SP, 5 * 16);
					M_ILD(rd->argintregs[2 * p + 1], REG_SP,  5 * 16 + 4);
					M_ILD(rd->argintregs[2 * p], REG_SP, 5 * 16);
				}
			}
		}
	}
#endif

	/* put methodinfo pointer on Stackframe */
	p = dseg_addaddress(cd, m);
	M_ALD(REG_ITMP1, REG_PV, p);
#if defined(__DARWIN__)
	M_AST(REG_ITMP1, REG_SP, LA_SIZE + TRACE_ARGS_NUM * 8); 
#else
	M_AST(REG_ITMP1, REG_SP, LA_SIZE + 4 * 8);
#endif
	p = dseg_addaddress(cd, builtin_trace_args);
	M_ALD(REG_ITMP2, REG_PV, p);
	M_MTCTR(REG_ITMP2);
	M_JSR;

#if defined(__DARWIN__)
	/* restore integer argument registers from the reserved stack space */

	stack_off = LA_SIZE;
	for (p = 0; p < md->paramcount && p < TRACE_ARGS_NUM; 
		 p++, stack_off += 8) {
		t = md->paramtypes[p].type;

		if (IS_INT_LNG_TYPE(t)) {
			if (!md->params[p].inmemory) {
				if (IS_2_WORD_TYPE(t)) {
					M_ILD(rd->argintregs[GET_HIGH_REG(md->params[p].regoff)]
						  , REG_SP, stack_off);
					M_ILD(rd->argintregs[GET_LOW_REG(md->params[p].regoff)]
						  , REG_SP, stack_off + 4);
				} else {
					M_ILD(rd->argintregs[md->params[p].regoff]
						  , REG_SP, stack_off + 4);
				}
			}
		}
	}
#else
	/* LINUX */
	for (p = 0; p < 8; p++) {
		d = rd->argintregs[p];
		/* save integer argument registers */
		M_ILD(d, REG_SP, LA_SIZE + 4 * 8 + 4 + p * 4);
	}
#endif

	if (!nativestub)
		M_ILD(REG_ITMP3, REG_SP, LA_SIZE + TRACE_ARGS_NUM * 8 + 1 * 4);

	M_LDA(REG_SP, REG_SP, stack_size);
	if (!nativestub)
		M_MTLR(REG_ITMP3);
	return mcodeptr;
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
