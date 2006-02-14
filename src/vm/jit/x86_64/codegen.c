/* src/vm/jit/x86_64/codegen.c - machine code generator for x86_64

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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

   Contact: cacao@cacaojvm.org

   Authors: Andreas Krall
            Christian Thalinger

   Changes: Christian Ullrich

   $Id: codegen.c 4508 2006-02-14 00:41:57Z twisti $

*/


#include "config.h"

#include <stdio.h>

#include "vm/types.h"

#include "md.h"
#include "md-abi.h"

#include "vm/jit/x86_64/arch.h"
#include "vm/jit/x86_64/codegen.h"
#include "vm/jit/x86_64/emitfuncs.h"

#include "cacao/cacao.h"
#include "native/native.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/statistics.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/codegen-common.h"
#include "vm/jit/dseg.h"
#include "vm/jit/jit.h"
#include "vm/jit/methodheader.h"
#include "vm/jit/parse.h"
#include "vm/jit/patcher.h"
#include "vm/jit/reg.h"

#if defined(ENABLE_LSRA)
# include "vm/jit/allocator/lsra.h"
#endif




/* codegen *********************************************************************

   Generates machine code.

*******************************************************************************/

bool codegen(methodinfo *m, codegendata *cd, registerdata *rd)
{
	s4                  len, s1, s2, s3, d, disp;
	u2                  currentline;
	ptrint              a;
	s4                  parentargs_base;
	stackptr            src;
	varinfo            *var;
	basicblock         *bptr;
	instruction        *iptr;
	exceptiontable     *ex;
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
	savedregs_num += (FLT_SAV_CNT - rd->savfltreguse);

	parentargs_base = rd->memuse + savedregs_num;

#if defined(USE_THREADS)
	/* space to save argument of monitor_enter */

	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		parentargs_base++;
#endif

    /* Keep stack of non-leaf functions 16-byte aligned for calls into native */
	/* code e.g. libc or jni (alignment problems with movaps).                */

	if (!m->isleafmethod || runverbose)
		parentargs_base |= 0x1;

	/* create method header */

	(void) dseg_addaddress(cd, m);                          /* MethodPointer  */
	(void) dseg_adds4(cd, parentargs_base * 8);             /* FrameSize      */

#if defined(USE_THREADS)
	/* IsSync contains the offset relative to the stack pointer for the
	   argument of monitor_exit used in the exception handler. Since the
	   offset could be zero and give a wrong meaning of the flag it is
	   offset by one.
	*/

	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		(void) dseg_adds4(cd, (rd->memuse + 1) * 8);        /* IsSync         */
	else
#endif
		(void) dseg_adds4(cd, 0);                           /* IsSync         */
	                                       
	(void) dseg_adds4(cd, m->isleafmethod);                 /* IsLeaf         */
	(void) dseg_adds4(cd, INT_SAV_CNT - rd->savintreguse);  /* IntSave        */
	(void) dseg_adds4(cd, FLT_SAV_CNT - rd->savfltreguse);  /* FltSave        */

	(void) dseg_addlinenumbertablesize(cd);

	(void) dseg_adds4(cd, cd->exceptiontablelength);        /* ExTableSize    */

	/* create exception table */

	for (ex = cd->exceptiontable; ex != NULL; ex = ex->down) {
		dseg_addtarget(cd, ex->start);
   		dseg_addtarget(cd, ex->end);
		dseg_addtarget(cd, ex->handler);
		(void) dseg_addaddress(cd, ex->catchtype.cls);
	}
	
	/* initialize mcode variables */
	
	cd->mcodeptr = (u1 *) cd->mcodebase;
	cd->mcodeend = (s4 *) (cd->mcodebase + cd->mcodesize);

	/* initialize the last patcher pointer */

	cd->lastmcodeptr = cd->mcodeptr;

	/* generate method profiling code */

	if (opt_prof) {
		/* count frequency */

		M_MOV_IMM((ptrint) m, REG_ITMP3);
		M_IINC_MEMBASE(REG_ITMP3, OFFSET(methodinfo, frequency));

		PROFILE_CYCLE_START;
	}

	/* create stack frame (if necessary) */

	if (parentargs_base)
		M_ASUB_IMM(parentargs_base * 8, REG_SP);

	/* save used callee saved registers */

  	p = parentargs_base;
	for (i = INT_SAV_CNT - 1; i >= rd->savintreguse; i--) {
 		p--; M_LST(rd->savintregs[i], REG_SP, p * 8);
	}
	for (i = FLT_SAV_CNT - 1; i >= rd->savfltreguse; i--) {
		p--; M_DST(rd->savfltregs[i], REG_SP, p * 8);
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
   					M_INTMOVE(s2, var->regoff);

				} else {                             /* reg arg -> spilled    */
   				    M_LST(s2, REG_SP, var->regoff * 8);
 				}

			} else {                                 /* stack arguments       */
 				if (!(var->flags & INMEMORY)) {      /* stack arg -> register */
					/* + 8 for return address */
 					M_LLD(var->regoff, REG_SP, (parentargs_base + s1) * 8 + 8);

				} else {                             /* stack arg -> spilled  */
					var->regoff = parentargs_base + s1 + 1;
				}
			}

		} else {                                     /* floating args         */
 			if (!md->params[p].inmemory) {           /* register arguments    */
				s2 = rd->argfltregs[s1];
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
					M_FLTMOVE(s2, var->regoff);

 				} else {			                 /* reg arg -> spilled    */
					M_DST(s2, REG_SP, var->regoff * 8);
 				}

 			} else {                                 /* stack arguments       */
 				if (!(var->flags & INMEMORY)) {      /* stack-arg -> register */
					M_DLD(var->regoff, REG_SP, (parentargs_base + s1) * 8 + 8);

				} else {
					var->regoff = parentargs_base + s1 + 1;
				}
			}
		}
	}  /* end for */

	/* save monitorenter argument */

#if defined(USE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		/* stack offset for monitor argument */

		s1 = rd->memuse;

		if (runverbose) {
			M_LSUB_IMM((INT_ARG_CNT + FLT_ARG_CNT) * 8, REG_SP);

			for (p = 0; p < INT_ARG_CNT; p++)
				M_LST(rd->argintregs[p], REG_SP, p * 8);

			for (p = 0; p < FLT_ARG_CNT; p++)
				M_DST(rd->argfltregs[p], REG_SP, (INT_ARG_CNT + p) * 8);

			s1 += INT_ARG_CNT + FLT_ARG_CNT;
		}

		/* decide which monitor enter function to call */

		if (m->flags & ACC_STATIC) {
			M_MOV_IMM((ptrint) m->class, REG_ITMP1);
			M_AST(REG_ITMP1, REG_SP, s1 * 8);
			M_INTMOVE(REG_ITMP1, rd->argintregs[0]);
			M_MOV_IMM((ptrint) BUILTIN_staticmonitorenter, REG_ITMP1);
			M_CALL(REG_ITMP1);

		} else {
			M_TEST(rd->argintregs[0]);
			x86_64_jcc(cd, X86_64_CC_Z, 0);
			codegen_addxnullrefs(cd, cd->mcodeptr);
			M_AST(rd->argintregs[0], REG_SP, s1 * 8);
			M_MOV_IMM((ptrint) BUILTIN_monitorenter, REG_ITMP1);
			M_CALL(REG_ITMP1);
		}

		if (runverbose) {
			for (p = 0; p < INT_ARG_CNT; p++)
				M_LLD(rd->argintregs[p], REG_SP, p * 8);

			for (p = 0; p < FLT_ARG_CNT; p++)
				M_DLD(rd->argfltregs[p], REG_SP, (INT_ARG_CNT + p) * 8);

			M_LADD_IMM((INT_ARG_CNT + FLT_ARG_CNT) * 8, REG_SP);
		}
	}
#endif

	/* Copy argument registers to stack and call trace function with
	   pointer to arguments on stack. */

	if (runverbose || opt_stat) {
		M_LSUB_IMM((INT_ARG_CNT + FLT_ARG_CNT + INT_TMP_CNT + FLT_TMP_CNT + 1 + 1) * 8, REG_SP);

		/* save integer argument registers */

		for (p = 0; p < INT_ARG_CNT; p++)
			M_LST(rd->argintregs[p], REG_SP, (1 + p) * 8);

		/* save float argument registers */

		for (p = 0; p < FLT_ARG_CNT; p++)
			M_DST(rd->argfltregs[p], REG_SP, (1 + INT_ARG_CNT + p) * 8);

		/* save temporary registers for leaf methods */

		if (m->isleafmethod) {
			for (p = 0; p < INT_TMP_CNT; p++)
				M_LST(rd->tmpintregs[p], REG_SP, (1 + INT_ARG_CNT + FLT_ARG_CNT + p) * 8);

			for (p = 0; p < FLT_TMP_CNT; p++)
				M_DST(rd->tmpfltregs[p], REG_SP, (1 + INT_ARG_CNT + FLT_ARG_CNT + INT_TMP_CNT + p) * 8);
		}

		if (runverbose) {
			/* show integer hex code for float arguments */

			for (p = 0, l = 0; p < md->paramcount && p < INT_ARG_CNT; p++) {
				/* if the paramtype is a float, we have to right shift all    */
				/* following integer registers                                */
	
				if (IS_FLT_DBL_TYPE(md->paramtypes[p].type)) {
					for (s1 = INT_ARG_CNT - 2; s1 >= p; s1--) {
						M_MOV(rd->argintregs[s1], rd->argintregs[s1 + 1]);
					}

					x86_64_movd_freg_reg(cd, rd->argfltregs[l], rd->argintregs[p]);
					l++;
				}
			}

			x86_64_mov_imm_reg(cd, (ptrint) m, REG_ITMP2);
			x86_64_mov_reg_membase(cd, REG_ITMP2, REG_SP, 0 * 8);
			x86_64_mov_imm_reg(cd, (ptrint) builtin_trace_args, REG_ITMP1);
			x86_64_call_reg(cd, REG_ITMP1);
		}

		/* restore integer argument registers */

		for (p = 0; p < INT_ARG_CNT; p++)
			M_LLD(rd->argintregs[p], REG_SP, (1 + p) * 8);

		/* restore float argument registers */

		for (p = 0; p < FLT_ARG_CNT; p++)
			M_DLD(rd->argfltregs[p], REG_SP, (1 + INT_ARG_CNT + p) * 8);

		/* restore temporary registers for leaf methods */

		if (m->isleafmethod) {
			for (p = 0; p < INT_TMP_CNT; p++)
				M_LLD(rd->tmpintregs[p], REG_SP, (1 + INT_ARG_CNT + FLT_ARG_CNT + p) * 8);

			for (p = 0; p < FLT_TMP_CNT; p++)
				M_DLD(rd->tmpfltregs[p], REG_SP, (1 + INT_ARG_CNT + FLT_ARG_CNT + INT_TMP_CNT + p) * 8);
		}

		M_LADD_IMM((INT_ARG_CNT + FLT_ARG_CNT + INT_TMP_CNT + FLT_TMP_CNT + 1 + 1) * 8, REG_SP);
	}

	}

	/* end of header generation */

	/* walk through all basic blocks */

	for (bptr = m->basicblocks; bptr != NULL; bptr = bptr->next) {

		bptr->mpc = (u4) ((u1 *) cd->mcodeptr - cd->mcodebase);

		if (bptr->flags >= BBREACHED) {

			/* branch resolving */

			branchref *bref;
			for (bref = bptr->branchrefs; bref != NULL; bref = bref->next) {
				gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
								  bref->branchpos,
								  bptr->mpc);
			}

		/* copy interface registers to their destination */

		src = bptr->instack;
		len = bptr->indepth;
		MCODECHECK(512);

		/* generate basicblock profiling code */

		if (opt_prof_bb) {
			/* count frequency */

			M_MOV_IMM((ptrint) m->bbfrequency, REG_ITMP2);
			M_IINC_MEMBASE(REG_ITMP2, bptr->debug_nr * 4);

			/* if this is an exception handler, start profiling again */

			if (bptr->type == BBTYPE_EXH)
				PROFILE_CYCLE_START;
		}

#if defined(ENABLE_LSRA)
		if (opt_lsra) {
			while (src != NULL) {
				len--;
				if ((len == 0) && (bptr->type != BBTYPE_STD)) {
					if (bptr->type == BBTYPE_SBR) {
						/*  					d = reg_of_var(rd, src, REG_ITMP1); */
						if (!(src->flags & INMEMORY))
							d= src->regoff;
						else
							d=REG_ITMP1;
						x86_64_pop_reg(cd, d);
						store_reg_to_var_int(src, d);

					} else if (bptr->type == BBTYPE_EXH) {
						/*  					d = reg_of_var(rd, src, REG_ITMP1); */
						if (!(src->flags & INMEMORY))
							d= src->regoff;
						else
							d=REG_ITMP1;
						M_INTMOVE(REG_ITMP1, d);
						store_reg_to_var_int(src, d);
					}
				}
				src = src->prev;
			}
			
		} else {
#endif

		while (src != NULL) {
			len--;
  			if ((len == 0) && (bptr->type != BBTYPE_STD)) {
				if (bptr->type == BBTYPE_SBR) {
					d = reg_of_var(rd, src, REG_ITMP1);
					M_POP(d);
					store_reg_to_var_int(src, d);

				} else if (bptr->type == BBTYPE_EXH) {
					d = reg_of_var(rd, src, REG_ITMP1);
					M_INTMOVE(REG_ITMP1, d);
					store_reg_to_var_int(src, d);
				}

			} else {
				d = reg_of_var(rd, src, REG_ITMP1);
				if ((src->varkind != STACKVAR)) {
					s2 = src->type;
					if (IS_FLT_DBL_TYPE(s2)) {
						s1 = rd->interfaces[len][s2].regoff;

						if (!(rd->interfaces[len][s2].flags & INMEMORY))
							M_FLTMOVE(s1, d);
						else
							M_DLD(d, REG_SP, s1 * 8);

						store_reg_to_var_flt(src, d);

					} else {
						s1 = rd->interfaces[len][s2].regoff;

						if (!(rd->interfaces[len][s2].flags & INMEMORY))
							M_INTMOVE(s1, d);
						else
							M_LLD(d, REG_SP, s1 * 8);

						store_reg_to_var_int(src, d);
					}
				}
			}
			src = src->prev;
		}
#if defined(ENABLE_LSRA)
		}
#endif
		/* walk through all instructions */
		
		src = bptr->instack;
		len = bptr->icount;
		currentline = 0;

		for (iptr = bptr->iinstr; len > 0; src = iptr->dst, len--, iptr++) {
			if (iptr->line != currentline) {
				dseg_addlinenumber(cd, iptr->line, cd->mcodeptr);
				currentline = iptr->line;
			}

			MCODECHECK(1024);                         /* 1KB should be enough */

			switch (iptr->opc) {
			case ICMD_INLINE_START: /* internal ICMDs                         */
			case ICMD_INLINE_END:
				break;

			case ICMD_NOP:    /* ...  ==> ...                                 */
				break;

			case ICMD_CHECKNULL: /* ..., objectref  ==> ..., objectref        */

				if (src->flags & INMEMORY)
					M_CMP_IMM_MEMBASE(0, REG_SP, src->regoff * 8);
				else
					M_TEST(src->regoff);
				M_BEQ(0);
				codegen_addxnullrefs(cd, cd->mcodeptr);
				break;

		/* constant operations ************************************************/

		case ICMD_ICONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.i = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if (iptr->val.i == 0)
				M_CLR(d);
			else
				M_IMOV_IMM(iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.l = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if (iptr->val.l == 0)
				M_CLR(d);
			else
				M_MOV_IMM(iptr->val.l, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.f = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			disp = dseg_addfloat(cd, iptr->val.f);
			x86_64_movdl_membase_reg(cd, RIP, -(((s8) cd->mcodeptr + ((d > 7) ? 9 : 8)) - (s8) cd->mcodebase) + disp, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
		
		case ICMD_DCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.d = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			disp = dseg_adddouble(cd, iptr->val.d);
			x86_64_movd_membase_reg(cd, RIP, -(((s8) cd->mcodeptr + 9) - (s8) cd->mcodebase) + disp, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_ACONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.a = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);

			if ((iptr->target != NULL) && (iptr->val.a == NULL)) {
/* 				PROFILE_CYCLE_STOP; */

				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_aconst,
									(unresolved_class *) iptr->target, 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

/* 				PROFILE_CYCLE_START; */

				M_MOV_IMM((ptrint) iptr->val.a, d);

			} else {
				if (iptr->val.a == 0)
					M_CLR(d);
				else
					M_MOV_IMM((ptrint) iptr->val.a, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;


		/* load/store operations **********************************************/

		case ICMD_ILOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1)) {
				break;
			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY) {
				x86_64_movl_membase_reg(cd, REG_SP, var->regoff * 8, d);
				store_reg_to_var_int(iptr->dst, d);

			} else {
				if (iptr->dst->flags & INMEMORY) {
					x86_64_mov_reg_membase(cd, var->regoff, REG_SP, iptr->dst->regoff * 8);

				} else {
					M_INTMOVE(var->regoff, d);
				}
			}
			break;

		case ICMD_LLOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_ALOAD:      /* op1 = local variable                         */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1)) {
				break;
			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY) {
				x86_64_mov_membase_reg(cd, REG_SP, var->regoff * 8, d);
				store_reg_to_var_int(iptr->dst, d);

			} else {
				if (iptr->dst->flags & INMEMORY) {
					x86_64_mov_reg_membase(cd, var->regoff, REG_SP, iptr->dst->regoff * 8);

				} else {
					M_INTMOVE(var->regoff, d);
				}
			}
			break;

		case ICMD_FLOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_DLOAD:      /* op1 = local variable                         */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
  			if ((iptr->dst->varkind == LOCALVAR) &&
  			    (iptr->dst->varnum == iptr->op1)) {
    				break;
  			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY) {
				x86_64_movq_membase_reg(cd, REG_SP, var->regoff * 8, d);
				store_reg_to_var_flt(iptr->dst, d);

			} else {
				if (iptr->dst->flags & INMEMORY) {
					x86_64_movq_reg_membase(cd, var->regoff, REG_SP, iptr->dst->regoff * 8);

				} else {
					M_FLTMOVE(var->regoff, d);
				}
			}
			break;

		case ICMD_ISTORE:     /* ..., value  ==> ...                          */
		case ICMD_LSTORE:     /* op1 = local variable                         */
		case ICMD_ASTORE:

			if ((src->varkind == LOCALVAR) &&
			    (src->varnum == iptr->op1)) {
				break;
			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				var_to_reg_int(s1, src, REG_ITMP1);
				x86_64_mov_reg_membase(cd, s1, REG_SP, var->regoff * 8);

			} else {
				var_to_reg_int(s1, src, var->regoff);
				M_INTMOVE(s1, var->regoff);
			}
			break;

		case ICMD_FSTORE:     /* ..., value  ==> ...                          */
		case ICMD_DSTORE:     /* op1 = local variable                         */

			if ((src->varkind == LOCALVAR) &&
			    (src->varnum == iptr->op1)) {
				break;
			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				var_to_reg_flt(s1, src, REG_FTMP1);
				x86_64_movq_reg_membase(cd, s1, REG_SP, var->regoff * 8);

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

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						x86_64_negl_membase(cd, REG_SP, iptr->dst->regoff * 8);

					} else {
						x86_64_movl_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
						x86_64_negl_reg(cd, REG_ITMP1);
						x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					x86_64_movl_reg_membase(cd, src->regoff, REG_SP, iptr->dst->regoff * 8);
					x86_64_negl_membase(cd, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					x86_64_movl_membase_reg(cd, REG_SP, src->regoff * 8, iptr->dst->regoff);
					x86_64_negl_reg(cd, d);

				} else {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					x86_64_negl_reg(cd, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_LNEG:       /* ..., value  ==> ..., - value                 */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						x86_64_neg_membase(cd, REG_SP, iptr->dst->regoff * 8);

					} else {
						x86_64_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
						x86_64_neg_reg(cd, REG_ITMP1);
						x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					x86_64_mov_reg_membase(cd, src->regoff, REG_SP, iptr->dst->regoff * 8);
					x86_64_neg_membase(cd, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					x86_64_mov_membase_reg(cd, REG_SP, src->regoff * 8, iptr->dst->regoff);
					x86_64_neg_reg(cd, iptr->dst->regoff);

				} else {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					x86_64_neg_reg(cd, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_I2L:        /* ..., value  ==> ..., value                   */

			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (src->flags & INMEMORY) {
				x86_64_movslq_membase_reg(cd, REG_SP, src->regoff * 8, d);

			} else {
				x86_64_movslq_reg_reg(cd, src->regoff, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (src->flags & INMEMORY) {
				x86_64_movsbq_membase_reg(cd, REG_SP, src->regoff * 8, d);

			} else {
				x86_64_movsbq_reg_reg(cd, src->regoff, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2CHAR:   /* ..., value  ==> ..., value                   */

			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (src->flags & INMEMORY) {
				x86_64_movzwq_membase_reg(cd, REG_SP, src->regoff * 8, d);

			} else {
				x86_64_movzwq_reg_reg(cd, src->regoff, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2SHORT:  /* ..., value  ==> ..., value                   */

			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (src->flags & INMEMORY) {
				x86_64_movswq_membase_reg(cd, REG_SP, src->regoff * 8, d);

			} else {
				x86_64_movswq_reg_reg(cd, src->regoff, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_IADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ialu(cd, X86_64_ADD, src, iptr);
			break;

		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ialuconst(cd, X86_64_ADD, src, iptr);
			break;

		case ICMD_LADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_lalu(cd, X86_64_ADD, src, iptr);
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_laluconst(cd, X86_64_ADD, src, iptr);
			break;

		case ICMD_ISUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						x86_64_movl_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
						x86_64_alul_reg_membase(cd, X86_64_SUB, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

					} else {
						x86_64_movl_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
						x86_64_alul_membase_reg(cd, X86_64_SUB, REG_SP, src->regoff * 8, REG_ITMP1);
						x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, REG_ITMP1);
					x86_64_alul_membase_reg(cd, X86_64_SUB, REG_SP, src->regoff * 8, REG_ITMP1);
					x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						x86_64_alul_reg_membase(cd, X86_64_SUB, src->regoff, REG_SP, iptr->dst->regoff * 8);

					} else {
						x86_64_movl_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
						x86_64_alul_reg_reg(cd, X86_64_SUB, src->regoff, REG_ITMP1);
						x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					x86_64_movl_reg_membase(cd, src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
					x86_64_alul_reg_membase(cd, X86_64_SUB, src->regoff, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_movl_membase_reg(cd, REG_SP, src->prev->regoff * 8, d);
					x86_64_alul_membase_reg(cd, X86_64_SUB, REG_SP, src->regoff * 8, d);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, d);
					x86_64_alul_membase_reg(cd, X86_64_SUB, REG_SP, src->regoff * 8, d);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					/* workaround for reg alloc */
					if (src->regoff == iptr->dst->regoff) {
						x86_64_movl_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
						x86_64_alul_reg_reg(cd, X86_64_SUB, src->regoff, REG_ITMP1);
						M_INTMOVE(REG_ITMP1, d);

					} else {
						x86_64_movl_membase_reg(cd, REG_SP, src->prev->regoff * 8, d);
						x86_64_alul_reg_reg(cd, X86_64_SUB, src->regoff, d);
					}

				} else {
					/* workaround for reg alloc */
					if (src->regoff == iptr->dst->regoff) {
						M_INTMOVE(src->prev->regoff, REG_ITMP1);
						x86_64_alul_reg_reg(cd, X86_64_SUB, src->regoff, REG_ITMP1);
						M_INTMOVE(REG_ITMP1, d);

					} else {
						M_INTMOVE(src->prev->regoff, d);
						x86_64_alul_reg_reg(cd, X86_64_SUB, src->regoff, d);
					}
				}
			}
			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ialuconst(cd, X86_64_SUB, src, iptr);
			break;

		case ICMD_LSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						x86_64_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
						x86_64_alu_reg_membase(cd, X86_64_SUB, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

					} else {
						x86_64_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
						x86_64_alu_membase_reg(cd, X86_64_SUB, REG_SP, src->regoff * 8, REG_ITMP1);
						x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, REG_ITMP1);
					x86_64_alu_membase_reg(cd, X86_64_SUB, REG_SP, src->regoff * 8, REG_ITMP1);
					x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						x86_64_alu_reg_membase(cd, X86_64_SUB, src->regoff, REG_SP, iptr->dst->regoff * 8);

					} else {
						x86_64_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
						x86_64_alu_reg_reg(cd, X86_64_SUB, src->regoff, REG_ITMP1);
						x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					x86_64_mov_reg_membase(cd, src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
					x86_64_alu_reg_membase(cd, X86_64_SUB, src->regoff, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, d);
					x86_64_alu_membase_reg(cd, X86_64_SUB, REG_SP, src->regoff * 8, d);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, d);
					x86_64_alu_membase_reg(cd, X86_64_SUB, REG_SP, src->regoff * 8, d);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					/* workaround for reg alloc */
					if (src->regoff == iptr->dst->regoff) {
						x86_64_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
						x86_64_alu_reg_reg(cd, X86_64_SUB, src->regoff, REG_ITMP1);
						M_INTMOVE(REG_ITMP1, d);

					} else {
						x86_64_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, d);
						x86_64_alu_reg_reg(cd, X86_64_SUB, src->regoff, d);
					}

				} else {
					/* workaround for reg alloc */
					if (src->regoff == iptr->dst->regoff) {
						M_INTMOVE(src->prev->regoff, REG_ITMP1);
						x86_64_alu_reg_reg(cd, X86_64_SUB, src->regoff, REG_ITMP1);
						M_INTMOVE(REG_ITMP1, d);

					} else {
						M_INTMOVE(src->prev->regoff, d);
						x86_64_alu_reg_reg(cd, X86_64_SUB, src->regoff, d);
					}
				}
			}
			break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_laluconst(cd, X86_64_SUB, src, iptr);
			break;

		case ICMD_IMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_movl_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
					x86_64_imull_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
					x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					x86_64_movl_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
					x86_64_imull_reg_reg(cd, src->prev->regoff, REG_ITMP1);
					x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_movl_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
					x86_64_imull_reg_reg(cd, src->regoff, REG_ITMP1);
					x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else {
					M_INTMOVE(src->prev->regoff, REG_ITMP1);
					x86_64_imull_reg_reg(cd, src->regoff, REG_ITMP1);
					x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_movl_membase_reg(cd, REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
					x86_64_imull_membase_reg(cd, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
					x86_64_imull_membase_reg(cd, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					x86_64_imull_membase_reg(cd, REG_SP, src->prev->regoff * 8, iptr->dst->regoff);

				} else {
					if (src->regoff == iptr->dst->regoff) {
						x86_64_imull_reg_reg(cd, src->prev->regoff, iptr->dst->regoff);

					} else {
						M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
						x86_64_imull_reg_reg(cd, src->regoff, iptr->dst->regoff);
					}
				}
			}
			break;

		case ICMD_IMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					x86_64_imull_imm_membase_reg(cd, iptr->val.i, REG_SP, src->regoff * 8, REG_ITMP1);
					x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else {
					x86_64_imull_imm_reg_reg(cd, iptr->val.i, src->regoff, REG_ITMP1);
					x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					x86_64_imull_imm_membase_reg(cd, iptr->val.i, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else {
					if (iptr->val.i == 2) {
						M_INTMOVE(src->regoff, iptr->dst->regoff);
						x86_64_alul_reg_reg(cd, X86_64_ADD, iptr->dst->regoff, iptr->dst->regoff);

					} else {
						x86_64_imull_imm_reg_reg(cd, iptr->val.i, src->regoff, iptr->dst->regoff);    /* 3 cycles */
					}
				}
			}
			break;

		case ICMD_LMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
					x86_64_imul_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
					x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					x86_64_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
					x86_64_imul_reg_reg(cd, src->prev->regoff, REG_ITMP1);
					x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
					x86_64_imul_reg_reg(cd, src->regoff, REG_ITMP1);
					x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else {
					x86_64_mov_reg_reg(cd, src->prev->regoff, REG_ITMP1);
					x86_64_imul_reg_reg(cd, src->regoff, REG_ITMP1);
					x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
					x86_64_imul_membase_reg(cd, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
					x86_64_imul_membase_reg(cd, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					x86_64_imul_membase_reg(cd, REG_SP, src->prev->regoff * 8, iptr->dst->regoff);

				} else {
					if (src->regoff == iptr->dst->regoff) {
						x86_64_imul_reg_reg(cd, src->prev->regoff, iptr->dst->regoff);

					} else {
						M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
						x86_64_imul_reg_reg(cd, src->regoff, iptr->dst->regoff);
					}
				}
			}
			break;

		case ICMD_LMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (IS_IMM32(iptr->val.l)) {
						x86_64_imul_imm_membase_reg(cd, iptr->val.l, REG_SP, src->regoff * 8, REG_ITMP1);

					} else {
						x86_64_mov_imm_reg(cd, iptr->val.l, REG_ITMP1);
						x86_64_imul_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
					}
					x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					
				} else {
					if (IS_IMM32(iptr->val.l)) {
						x86_64_imul_imm_reg_reg(cd, iptr->val.l, src->regoff, REG_ITMP1);

					} else {
						x86_64_mov_imm_reg(cd, iptr->val.l, REG_ITMP1);
						x86_64_imul_reg_reg(cd, src->regoff, REG_ITMP1);
					}
					x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					if (IS_IMM32(iptr->val.l)) {
						x86_64_imul_imm_membase_reg(cd, iptr->val.l, REG_SP, src->regoff * 8, iptr->dst->regoff);

					} else {
						x86_64_mov_imm_reg(cd, iptr->val.l, iptr->dst->regoff);
						x86_64_imul_membase_reg(cd, REG_SP, src->regoff * 8, iptr->dst->regoff);
					}

				} else {
					/* should match in many cases */
					if (iptr->val.l == 2) {
						M_INTMOVE(src->regoff, iptr->dst->regoff);
						x86_64_alul_reg_reg(cd, X86_64_ADD, iptr->dst->regoff, iptr->dst->regoff);

					} else {
						if (IS_IMM32(iptr->val.l)) {
							x86_64_imul_imm_reg_reg(cd, iptr->val.l, src->regoff, iptr->dst->regoff);    /* 4 cycles */

						} else {
							x86_64_mov_imm_reg(cd, iptr->val.l, REG_ITMP1);
							M_INTMOVE(src->regoff, iptr->dst->regoff);
							x86_64_imul_reg_reg(cd, REG_ITMP1, iptr->dst->regoff);
						}
					}
				}
			}
			break;

		case ICMD_IDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
	        if (src->prev->flags & INMEMORY) {
				x86_64_movl_membase_reg(cd, REG_SP, src->prev->regoff * 8, RAX);

			} else {
				M_INTMOVE(src->prev->regoff, RAX);
			}
			
			if (src->flags & INMEMORY) {
				x86_64_movl_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP3);

			} else {
				M_INTMOVE(src->regoff, REG_ITMP3);
			}
			gen_div_check(src);

			x86_64_alul_imm_reg(cd, X86_64_CMP, 0x80000000, RAX); /* check as described in jvm spec */
			x86_64_jcc(cd, X86_64_CC_NE, 4 + 6);
			x86_64_alul_imm_reg(cd, X86_64_CMP, -1, REG_ITMP3);    /* 4 bytes */
			x86_64_jcc(cd, X86_64_CC_E, 3 + 1 + 3);                /* 6 bytes */

			x86_64_mov_reg_reg(cd, RDX, REG_ITMP2); /* save %rdx, cause it's an argument register */
  			x86_64_cltd(cd);
			x86_64_idivl_reg(cd, REG_ITMP3);

			if (iptr->dst->flags & INMEMORY) {
				x86_64_mov_reg_membase(cd, RAX, REG_SP, iptr->dst->regoff * 8);
				x86_64_mov_reg_reg(cd, REG_ITMP2, RDX);       /* restore %rdx */

			} else {
				M_INTMOVE(RAX, iptr->dst->regoff);

				if (iptr->dst->regoff != RDX) {
					x86_64_mov_reg_reg(cd, REG_ITMP2, RDX);   /* restore %rdx */
				}
			}
			break;

		case ICMD_IREM:       /* ..., val1, val2  ==> ..., val1 % val2        */
			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (src->prev->flags & INMEMORY) {
				x86_64_movl_membase_reg(cd, REG_SP, src->prev->regoff * 8, RAX);

			} else {
				M_INTMOVE(src->prev->regoff, RAX);
			}
			
			if (src->flags & INMEMORY) {
				x86_64_movl_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP3);

			} else {
				M_INTMOVE(src->regoff, REG_ITMP3);
			}
			gen_div_check(src);

			x86_64_mov_reg_reg(cd, RDX, REG_ITMP2); /* save %rdx, cause it's an argument register */

			x86_64_alul_imm_reg(cd, X86_64_CMP, 0x80000000, RAX); /* check as described in jvm spec */
			x86_64_jcc(cd, X86_64_CC_NE, 2 + 4 + 6);


			x86_64_alul_reg_reg(cd, X86_64_XOR, RDX, RDX);         /* 2 bytes */
			x86_64_alul_imm_reg(cd, X86_64_CMP, -1, REG_ITMP3);    /* 4 bytes */
			x86_64_jcc(cd, X86_64_CC_E, 1 + 3);                    /* 6 bytes */

  			x86_64_cltd(cd);
			x86_64_idivl_reg(cd, REG_ITMP3);

			if (iptr->dst->flags & INMEMORY) {
				x86_64_mov_reg_membase(cd, RDX, REG_SP, iptr->dst->regoff * 8);
				x86_64_mov_reg_reg(cd, REG_ITMP2, RDX);       /* restore %rdx */

			} else {
				M_INTMOVE(RDX, iptr->dst->regoff);

				if (iptr->dst->regoff != RDX) {
					x86_64_mov_reg_reg(cd, REG_ITMP2, RDX);   /* restore %rdx */
				}
			}
			break;

		case ICMD_IDIVPOW2:   /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, REG_ITMP1);
			x86_64_alul_imm_reg(cd, X86_64_CMP, -1, REG_ITMP1);
			x86_64_leal_membase_reg(cd, REG_ITMP1, (1 << iptr->val.i) - 1, REG_ITMP2);
			x86_64_cmovccl_reg_reg(cd, X86_64_CC_LE, REG_ITMP2, REG_ITMP1);
			x86_64_shiftl_imm_reg(cd, X86_64_SAR, iptr->val.i, REG_ITMP1);
			x86_64_mov_reg_reg(cd, REG_ITMP1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, REG_ITMP1);
			x86_64_alul_imm_reg(cd, X86_64_CMP, -1, REG_ITMP1);
			x86_64_leal_membase_reg(cd, REG_ITMP1, iptr->val.i, REG_ITMP2);
			x86_64_cmovccl_reg_reg(cd, X86_64_CC_G, REG_ITMP1, REG_ITMP2);
			x86_64_alul_imm_reg(cd, X86_64_AND, -1 - (iptr->val.i), REG_ITMP2);
			x86_64_alul_reg_reg(cd, X86_64_SUB, REG_ITMP2, REG_ITMP1);
			x86_64_mov_reg_reg(cd, REG_ITMP1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_LDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);

	        if (src->prev->flags & INMEMORY) {
				M_LLD(RAX, REG_SP, src->prev->regoff * 8);

			} else {
				M_INTMOVE(src->prev->regoff, RAX);
			}
			
			if (src->flags & INMEMORY) {
				M_LLD(REG_ITMP3, REG_SP, src->regoff * 8);

			} else {
				M_INTMOVE(src->regoff, REG_ITMP3);
			}
			gen_div_check(src);

			/* check as described in jvm spec */
			disp = dseg_adds8(cd, 0x8000000000000000LL);
  			M_CMP_MEMBASE(RIP, -(((ptrint) cd->mcodeptr + 7) - (ptrint) cd->mcodebase) + disp, RAX);
			M_BNE(4 + 6);
			M_CMP_IMM(-1, REG_ITMP3);                              /* 4 bytes */
			M_BEQ(3 + 2 + 3);                                      /* 6 bytes */

			M_MOV(RDX, REG_ITMP2); /* save %rdx, cause it's an argument register */
  			x86_64_cqto(cd);
			x86_64_idiv_reg(cd, REG_ITMP3);

			if (iptr->dst->flags & INMEMORY) {
				M_LST(RAX, REG_SP, iptr->dst->regoff * 8);
				M_MOV(REG_ITMP2, RDX);                        /* restore %rdx */

			} else {
				M_INTMOVE(RAX, iptr->dst->regoff);

				if (iptr->dst->regoff != RDX) {
					M_MOV(REG_ITMP2, RDX);                    /* restore %rdx */
				}
			}
			break;

		case ICMD_LREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (src->prev->flags & INMEMORY) {
				M_LLD(REG_ITMP1, REG_SP, src->prev->regoff * 8);

			} else {
				M_INTMOVE(src->prev->regoff, REG_ITMP1);
			}
			
			if (src->flags & INMEMORY) {
				M_LLD(REG_ITMP3, REG_SP, src->regoff * 8);

			} else {
				M_INTMOVE(src->regoff, REG_ITMP3);
			}
			gen_div_check(src);

			M_MOV(RDX, REG_ITMP2); /* save %rdx, cause it's an argument register */

			/* check as described in jvm spec */
			disp = dseg_adds8(cd, 0x8000000000000000LL);
  			M_CMP_MEMBASE(RIP, -(((ptrint) cd->mcodeptr + 7) - (ptrint) cd->mcodebase) + disp, REG_ITMP1);
			M_BNE(3 + 4 + 6);

#if 0
			x86_64_alul_reg_reg(cd, X86_64_XOR, RDX, RDX);         /* 2 bytes */
#endif
			M_XOR(RDX, RDX);                                       /* 3 bytes */
			M_CMP_IMM(-1, REG_ITMP3);                              /* 4 bytes */
			M_BEQ(2 + 3);                                          /* 6 bytes */

  			x86_64_cqto(cd);
			x86_64_idiv_reg(cd, REG_ITMP3);

			if (iptr->dst->flags & INMEMORY) {
				M_LST(RDX, REG_SP, iptr->dst->regoff * 8);
				M_MOV(REG_ITMP2, RDX);                        /* restore %rdx */

			} else {
				M_INTMOVE(RDX, iptr->dst->regoff);

				if (iptr->dst->regoff != RDX) {
					M_MOV(REG_ITMP2, RDX);                    /* restore %rdx */
				}
			}
			break;

		case ICMD_LDIVPOW2:   /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, REG_ITMP1);
			x86_64_alu_imm_reg(cd, X86_64_CMP, -1, REG_ITMP1);
			x86_64_lea_membase_reg(cd, REG_ITMP1, (1 << iptr->val.i) - 1, REG_ITMP2);
			x86_64_cmovcc_reg_reg(cd, X86_64_CC_LE, REG_ITMP2, REG_ITMP1);
			x86_64_shift_imm_reg(cd, X86_64_SAR, iptr->val.i, REG_ITMP1);
			x86_64_mov_reg_reg(cd, REG_ITMP1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, REG_ITMP1);
			x86_64_alu_imm_reg(cd, X86_64_CMP, -1, REG_ITMP1);
			x86_64_lea_membase_reg(cd, REG_ITMP1, iptr->val.i, REG_ITMP2);
			x86_64_cmovcc_reg_reg(cd, X86_64_CC_G, REG_ITMP1, REG_ITMP2);
			x86_64_alu_imm_reg(cd, X86_64_AND, -1 - (iptr->val.i), REG_ITMP2);
			x86_64_alu_reg_reg(cd, X86_64_SUB, REG_ITMP2, REG_ITMP1);
			x86_64_mov_reg_reg(cd, REG_ITMP1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ishift(cd, X86_64_SHL, src, iptr);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ishiftconst(cd, X86_64_SHL, src, iptr);
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ishift(cd, X86_64_SAR, src, iptr);
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ishiftconst(cd, X86_64_SAR, src, iptr);
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ishift(cd, X86_64_SHR, src, iptr);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ishiftconst(cd, X86_64_SHR, src, iptr);
			break;

		case ICMD_LSHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_lshift(cd, X86_64_SHL, src, iptr);
			break;

        case ICMD_LSHLCONST:  /* ..., value  ==> ..., value << constant       */
 			                  /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_lshiftconst(cd, X86_64_SHL, src, iptr);
			break;

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_lshift(cd, X86_64_SAR, src, iptr);
			break;

		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_lshiftconst(cd, X86_64_SAR, src, iptr);
			break;

		case ICMD_LUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_lshift(cd, X86_64_SHR, src, iptr);
			break;

  		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
  		                      /* val.l = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_lshiftconst(cd, X86_64_SHR, src, iptr);
			break;

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ialu(cd, X86_64_AND, src, iptr);
			break;

		case ICMD_IANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ialuconst(cd, X86_64_AND, src, iptr);
			break;

		case ICMD_LAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_lalu(cd, X86_64_AND, src, iptr);
			break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_laluconst(cd, X86_64_AND, src, iptr);
			break;

		case ICMD_IOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ialu(cd, X86_64_OR, src, iptr);
			break;

		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ialuconst(cd, X86_64_OR, src, iptr);
			break;

		case ICMD_LOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_lalu(cd, X86_64_OR, src, iptr);
			break;

		case ICMD_LORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_laluconst(cd, X86_64_OR, src, iptr);
			break;

		case ICMD_IXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ialu(cd, X86_64_XOR, src, iptr);
			break;

		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ialuconst(cd, X86_64_XOR, src, iptr);
			break;

		case ICMD_LXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_lalu(cd, X86_64_XOR, src, iptr);
			break;

		case ICMD_LXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_laluconst(cd, X86_64_XOR, src, iptr);
			break;


		case ICMD_IINC:       /* ..., value  ==> ..., value + constant        */
		                      /* op1 = variable, val.i = constant             */

			/* using inc and dec is definitely faster than add -- tested      */
			/* with sieve                                                     */

			var = &(rd->locals[iptr->op1][TYPE_INT]);
			d = var->regoff;
			if (var->flags & INMEMORY) {
				if (iptr->val.i == 1) {
					x86_64_incl_membase(cd, REG_SP, d * 8);
 
				} else if (iptr->val.i == -1) {
					x86_64_decl_membase(cd, REG_SP, d * 8);

				} else {
					x86_64_alul_imm_membase(cd, X86_64_ADD, iptr->val.i, REG_SP, d * 8);
				}

			} else {
				if (iptr->val.i == 1) {
					x86_64_incl_reg(cd, d);
 
				} else if (iptr->val.i == -1) {
					x86_64_decl_reg(cd, d);

				} else {
					x86_64_alul_imm_reg(cd, X86_64_ADD, iptr->val.i, d);
				}
			}
			break;


		/* floating operations ************************************************/

		case ICMD_FNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			disp = dseg_adds4(cd, 0x80000000);
			M_FLTMOVE(s1, d);
			x86_64_movss_membase_reg(cd, RIP, -(((s8) cd->mcodeptr + 9) - (s8) cd->mcodebase) + disp, REG_FTMP2);
			x86_64_xorps_reg_reg(cd, REG_FTMP2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			disp = dseg_adds8(cd, 0x8000000000000000);
			M_FLTMOVE(s1, d);
			x86_64_movd_membase_reg(cd, RIP, -(((s8) cd->mcodeptr + 9) - (s8) cd->mcodebase) + disp, REG_FTMP2);
			x86_64_xorpd_reg_reg(cd, REG_FTMP2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (s1 == d) {
				x86_64_addss_reg_reg(cd, s2, d);
			} else if (s2 == d) {
				x86_64_addss_reg_reg(cd, s1, d);
			} else {
				M_FLTMOVE(s1, d);
				x86_64_addss_reg_reg(cd, s2, d);
			}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (s1 == d) {
				x86_64_addsd_reg_reg(cd, s2, d);
			} else if (s2 == d) {
				x86_64_addsd_reg_reg(cd, s1, d);
			} else {
				M_FLTMOVE(s1, d);
				x86_64_addsd_reg_reg(cd, s2, d);
			}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (s2 == d) {
				M_FLTMOVE(s2, REG_FTMP2);
				s2 = REG_FTMP2;
			}
			M_FLTMOVE(s1, d);
			x86_64_subss_reg_reg(cd, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (s2 == d) {
				M_FLTMOVE(s2, REG_FTMP2);
				s2 = REG_FTMP2;
			}
			M_FLTMOVE(s1, d);
			x86_64_subsd_reg_reg(cd, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (s1 == d) {
				x86_64_mulss_reg_reg(cd, s2, d);
			} else if (s2 == d) {
				x86_64_mulss_reg_reg(cd, s1, d);
			} else {
				M_FLTMOVE(s1, d);
				x86_64_mulss_reg_reg(cd, s2, d);
			}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (s1 == d) {
				x86_64_mulsd_reg_reg(cd, s2, d);
			} else if (s2 == d) {
				x86_64_mulsd_reg_reg(cd, s1, d);
			} else {
				M_FLTMOVE(s1, d);
				x86_64_mulsd_reg_reg(cd, s2, d);
			}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (s2 == d) {
				M_FLTMOVE(s2, REG_FTMP2);
				s2 = REG_FTMP2;
			}
			M_FLTMOVE(s1, d);
			x86_64_divss_reg_reg(cd, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (s2 == d) {
				M_FLTMOVE(s2, REG_FTMP2);
				s2 = REG_FTMP2;
			}
			M_FLTMOVE(s1, d);
			x86_64_divsd_reg_reg(cd, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_I2F:       /* ..., value  ==> ..., (float) value            */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			x86_64_cvtsi2ss_reg_reg(cd, s1, d);
  			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_I2D:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			x86_64_cvtsi2sd_reg_reg(cd, s1, d);
  			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_L2F:       /* ..., value  ==> ..., (float) value            */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			x86_64_cvtsi2ssq_reg_reg(cd, s1, d);
  			store_reg_to_var_flt(iptr->dst, d);
			break;
			
		case ICMD_L2D:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			x86_64_cvtsi2sdq_reg_reg(cd, s1, d);
  			store_reg_to_var_flt(iptr->dst, d);
			break;
			
		case ICMD_F2I:       /* ..., value  ==> ..., (int) value              */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			x86_64_cvttss2si_reg_reg(cd, s1, d);
			x86_64_alul_imm_reg(cd, X86_64_CMP, 0x80000000, d);    /* corner cases */
			a = ((s1 == REG_FTMP1) ? 0 : 5) + 10 + 3 + ((REG_RESULT == d) ? 0 : 3);
			x86_64_jcc(cd, X86_64_CC_NE, a);
			M_FLTMOVE(s1, REG_FTMP1);
			x86_64_mov_imm_reg(cd, (ptrint) asm_builtin_f2i, REG_ITMP2);
			x86_64_call_reg(cd, REG_ITMP2);
			M_INTMOVE(REG_RESULT, d);
  			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_D2I:       /* ..., value  ==> ..., (int) value              */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			x86_64_cvttsd2si_reg_reg(cd, s1, d);
			x86_64_alul_imm_reg(cd, X86_64_CMP, 0x80000000, d);    /* corner cases */
			a = ((s1 == REG_FTMP1) ? 0 : 5) + 10 + 3 + ((REG_RESULT == d) ? 0 : 3);
			x86_64_jcc(cd, X86_64_CC_NE, a);
			M_FLTMOVE(s1, REG_FTMP1);
			x86_64_mov_imm_reg(cd, (ptrint) asm_builtin_d2i, REG_ITMP2);
			x86_64_call_reg(cd, REG_ITMP2);
			M_INTMOVE(REG_RESULT, d);
  			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_F2L:       /* ..., value  ==> ..., (long) value             */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			x86_64_cvttss2siq_reg_reg(cd, s1, d);
			x86_64_mov_imm_reg(cd, 0x8000000000000000, REG_ITMP2);
			x86_64_alu_reg_reg(cd, X86_64_CMP, REG_ITMP2, d);     /* corner cases */
			a = ((s1 == REG_FTMP1) ? 0 : 5) + 10 + 3 + ((REG_RESULT == d) ? 0 : 3);
			x86_64_jcc(cd, X86_64_CC_NE, a);
			M_FLTMOVE(s1, REG_FTMP1);
			x86_64_mov_imm_reg(cd, (ptrint) asm_builtin_f2l, REG_ITMP2);
			x86_64_call_reg(cd, REG_ITMP2);
			M_INTMOVE(REG_RESULT, d);
  			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_D2L:       /* ..., value  ==> ..., (long) value             */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			x86_64_cvttsd2siq_reg_reg(cd, s1, d);
			x86_64_mov_imm_reg(cd, 0x8000000000000000, REG_ITMP2);
			x86_64_alu_reg_reg(cd, X86_64_CMP, REG_ITMP2, d);     /* corner cases */
			a = ((s1 == REG_FTMP1) ? 0 : 5) + 10 + 3 + ((REG_RESULT == d) ? 0 : 3);
			x86_64_jcc(cd, X86_64_CC_NE, a);
			M_FLTMOVE(s1, REG_FTMP1);
			x86_64_mov_imm_reg(cd, (ptrint) asm_builtin_d2l, REG_ITMP2);
			x86_64_call_reg(cd, REG_ITMP2);
			M_INTMOVE(REG_RESULT, d);
  			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_F2D:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			x86_64_cvtss2sd_reg_reg(cd, s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_D2F:       /* ..., value  ==> ..., (float) value            */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			x86_64_cvtsd2ss_reg_reg(cd, s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */
 			                  /* == => 0, < => 1, > => -1 */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_CLR(d);
			M_MOV_IMM(1, REG_ITMP1);
			M_MOV_IMM(-1, REG_ITMP2);
			x86_64_ucomiss_reg_reg(cd, s1, s2);
			M_CMOVB(REG_ITMP1, d);
			M_CMOVA(REG_ITMP2, d);
			M_CMOVP(REG_ITMP2, d);                   /* treat unordered as GT */
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */
 			                  /* == => 0, < => 1, > => -1 */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_CLR(d);
			M_MOV_IMM(1, REG_ITMP1);
			M_MOV_IMM(-1, REG_ITMP2);
			x86_64_ucomiss_reg_reg(cd, s1, s2);
			M_CMOVB(REG_ITMP1, d);
			M_CMOVA(REG_ITMP2, d);
			M_CMOVP(REG_ITMP1, d);                   /* treat unordered as LT */
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_DCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */
 			                  /* == => 0, < => 1, > => -1 */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_CLR(d);
			M_MOV_IMM(1, REG_ITMP1);
			M_MOV_IMM(-1, REG_ITMP2);
			x86_64_ucomisd_reg_reg(cd, s1, s2);
			M_CMOVB(REG_ITMP1, d);
			M_CMOVA(REG_ITMP2, d);
			M_CMOVP(REG_ITMP2, d);                   /* treat unordered as GT */
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_DCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */
 			                  /* == => 0, < => 1, > => -1 */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_CLR(d);
			M_MOV_IMM(1, REG_ITMP1);
			M_MOV_IMM(-1, REG_ITMP2);
			x86_64_ucomisd_reg_reg(cd, s1, s2);
			M_CMOVB(REG_ITMP1, d);
			M_CMOVA(REG_ITMP2, d);
			M_CMOVP(REG_ITMP1, d);                   /* treat unordered as LT */
			store_reg_to_var_int(iptr->dst, d);
			break;


		/* memory operations **************************************************/

		case ICMD_ARRAYLENGTH: /* ..., arrayref  ==> ..., (int) length        */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			gen_nullptr_check(s1);
			M_ILD(d, s1, OFFSET(java_arrayheader, size));
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_BALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
   			x86_64_movsbq_memindex_reg(cd, OFFSET(java_bytearray, data[0]), s1, s2, 0, d);
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
			x86_64_movzwq_memindex_reg(cd, OFFSET(java_chararray, data[0]), s1, s2, 1, d);
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
			x86_64_movswq_memindex_reg(cd, OFFSET(java_shortarray, data[0]), s1, s2, 1, d);
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
			x86_64_movl_memindex_reg(cd, OFFSET(java_intarray, data[0]), s1, s2, 2, d);
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
			x86_64_mov_memindex_reg(cd, OFFSET(java_longarray, data[0]), s1, s2, 3, d);
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
			x86_64_movss_memindex_reg(cd, OFFSET(java_floatarray, data[0]), s1, s2, 2, d);
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
			x86_64_movsd_memindex_reg(cd, OFFSET(java_doublearray, data[0]), s1, s2, 3, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_AALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			x86_64_mov_memindex_reg(cd, OFFSET(java_objectarray, data[0]), s1, s2, 3, d);
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
			x86_64_movb_reg_memindex(cd, s3, OFFSET(java_bytearray, data[0]), s1, s2, 0);
			break;

		case ICMD_CASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			x86_64_movw_reg_memindex(cd, s3, OFFSET(java_chararray, data[0]), s1, s2, 1);
			break;

		case ICMD_SASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			x86_64_movw_reg_memindex(cd, s3, OFFSET(java_shortarray, data[0]), s1, s2, 1);
			break;

		case ICMD_IASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			x86_64_movl_reg_memindex(cd, s3, OFFSET(java_intarray, data[0]), s1, s2, 2);
			break;

		case ICMD_LASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			x86_64_mov_reg_memindex(cd, s3, OFFSET(java_longarray, data[0]), s1, s2, 3);
			break;

		case ICMD_FASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_flt(s3, src, REG_FTMP3);
			x86_64_movss_reg_memindex(cd, s3, OFFSET(java_floatarray, data[0]), s1, s2, 2);
			break;

		case ICMD_DASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_flt(s3, src, REG_FTMP3);
			x86_64_movsd_reg_memindex(cd, s3, OFFSET(java_doublearray, data[0]), s1, s2, 3);
			break;

		case ICMD_AASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);

			M_MOV(s1, rd->argintregs[0]);
			M_MOV(s3, rd->argintregs[1]);
			M_MOV_IMM((ptrint) BUILTIN_canstore, REG_ITMP1);
			M_CALL(REG_ITMP1);
			M_TEST(REG_RESULT);
			M_BEQ(0);
			codegen_addxstorerefs(cd, cd->mcodeptr);

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			var_to_reg_int(s3, src, REG_ITMP3);
			x86_64_mov_reg_memindex(cd, s3, OFFSET(java_objectarray, data[0]), s1, s2, 3);
			break;


		case ICMD_BASTORECONST: /* ..., arrayref, index  ==> ...              */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			x86_64_movb_imm_memindex(cd, iptr->val.i, OFFSET(java_bytearray, data[0]), s1, s2, 0);
			break;

		case ICMD_CASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			x86_64_movw_imm_memindex(cd, iptr->val.i, OFFSET(java_chararray, data[0]), s1, s2, 1);
			break;

		case ICMD_SASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			x86_64_movw_imm_memindex(cd, iptr->val.i, OFFSET(java_shortarray, data[0]), s1, s2, 1);
			break;

		case ICMD_IASTORECONST: /* ..., arrayref, index  ==> ...              */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			x86_64_movl_imm_memindex(cd, iptr->val.i, OFFSET(java_intarray, data[0]), s1, s2, 2);
			break;

		case ICMD_LASTORECONST: /* ..., arrayref, index  ==> ...              */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}

			if (IS_IMM32(iptr->val.l)) {
				x86_64_mov_imm_memindex(cd, (u4) (iptr->val.l & 0x00000000ffffffff), OFFSET(java_longarray, data[0]), s1, s2, 3);
			} else {
				x86_64_movl_imm_memindex(cd, (u4) (iptr->val.l & 0x00000000ffffffff), OFFSET(java_longarray, data[0]), s1, s2, 3);
				x86_64_movl_imm_memindex(cd, (u4) (iptr->val.l >> 32), OFFSET(java_longarray, data[0]) + 4, s1, s2, 3);
			}
			break;

		case ICMD_AASTORECONST: /* ..., arrayref, index  ==> ...              */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			x86_64_mov_imm_memindex(cd, 0, OFFSET(java_objectarray, data[0]), s1, s2, 3);
			break;


		case ICMD_GETSTATIC:  /* ...  ==> ..., value                          */
		                      /* op1 = type, val.a = field address            */

			if (iptr->val.a == NULL) {
				disp = dseg_addaddress(cd, NULL);

/* 				PROFILE_CYCLE_STOP; */

				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_get_putstatic,
									(unresolved_field *) iptr->target, disp);

				if (opt_showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

/* 				PROFILE_CYCLE_START; */

			} else {
				fieldinfo *fi = iptr->val.a;

				disp = dseg_addaddress(cd, &(fi->value));

				if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					PROFILE_CYCLE_STOP;

					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_clinit, fi->class, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}

					PROFILE_CYCLE_START;
				}
  			}

			/* This approach is much faster than moving the field
			   address inline into a register. */

  			M_ALD(REG_ITMP2, RIP, -(((ptrint) cd->mcodeptr + 7) -
									(ptrint) cd->mcodebase) + disp);

			switch (iptr->op1) {
			case TYPE_INT:
				d = reg_of_var(rd, iptr->dst, REG_ITMP1);
				M_ILD(d, REG_ITMP2, 0);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_LNG:
			case TYPE_ADR:
				d = reg_of_var(rd, iptr->dst, REG_ITMP1);
				M_LLD(d, REG_ITMP2, 0);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_FLT:
				d = reg_of_var(rd, iptr->dst, REG_ITMP1);
				x86_64_movss_membase_reg(cd, REG_ITMP2, 0, d);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			case TYPE_DBL:				
				d = reg_of_var(rd, iptr->dst, REG_ITMP1);
				x86_64_movsd_membase_reg(cd, REG_ITMP2, 0, d);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			}
			break;

		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */
		                      /* op1 = type, val.a = field address            */

			if (iptr->val.a == NULL) {
				disp = dseg_addaddress(cd, NULL);

/* 				PROFILE_CYCLE_STOP; */

				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_get_putstatic,
									(unresolved_field *) iptr->target, disp);

				if (opt_showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

/* 				PROFILE_CYCLE_START; */

			} else {
				fieldinfo *fi = iptr->val.a;

				disp = dseg_addaddress(cd, &(fi->value));

				if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					PROFILE_CYCLE_STOP;

					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_clinit, fi->class, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}

					PROFILE_CYCLE_START;
				}
  			}

			/* This approach is much faster than moving the field
			   address inline into a register. */

  			M_ALD(REG_ITMP2, RIP, -(((ptrint) cd->mcodeptr + 7) -
									(ptrint) cd->mcodebase) + disp);

			switch (iptr->op1) {
			case TYPE_INT:
				var_to_reg_int(s2, src, REG_ITMP1);
				M_IST(s2, REG_ITMP2, 0);
				break;
			case TYPE_LNG:
			case TYPE_ADR:
				var_to_reg_int(s2, src, REG_ITMP1);
				M_LST(s2, REG_ITMP2, 0);
				break;
			case TYPE_FLT:
				var_to_reg_flt(s2, src, REG_FTMP1);
				x86_64_movss_reg_membase(cd, s2, REG_ITMP2, 0);
				break;
			case TYPE_DBL:
				var_to_reg_flt(s2, src, REG_FTMP1);
				x86_64_movsd_reg_membase(cd, s2, REG_ITMP2, 0);
				break;
			}
			break;

		case ICMD_PUTSTATICCONST: /* ...  ==> ...                             */
		                          /* val = value (in current instruction)     */
		                          /* op1 = type, val.a = field address (in    */
		                          /* following NOP)                           */

			if (iptr[1].val.a == NULL) {
				disp = dseg_addaddress(cd, NULL);

/* 				PROFILE_CYCLE_STOP; */

				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_get_putstatic,
									(unresolved_field *) iptr[1].target, disp);

				if (opt_showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

/* 				PROFILE_CYCLE_START; */

			} else {
				fieldinfo *fi = iptr[1].val.a;

				disp = dseg_addaddress(cd, &(fi->value));

				if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					PROFILE_CYCLE_STOP;

					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_clinit, fi->class, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}

					PROFILE_CYCLE_START;
				}
  			}

			/* This approach is much faster than moving the field
			   address inline into a register. */

  			M_ALD(REG_ITMP1, RIP, -(((ptrint) cd->mcodeptr + 7) -
									(ptrint) cd->mcodebase) + disp);

			switch (iptr->op1) {
			case TYPE_INT:
			case TYPE_FLT:
				M_IST_IMM(iptr->val.i, REG_ITMP1, 0);
				break;
			case TYPE_LNG:
			case TYPE_ADR:
			case TYPE_DBL:
				if (IS_IMM32(iptr->val.l)) {
					M_LST_IMM32(iptr->val.l, REG_ITMP1, 0);
				} else {
					M_IST_IMM(iptr->val.l, REG_ITMP1, 0);
					M_IST_IMM(iptr->val.l >> 32, REG_ITMP1, 4);
				}
				break;
			}
			break;

		case ICMD_GETFIELD:   /* ...  ==> ..., value                          */
		                      /* op1 = type, val.i = field offset             */

			var_to_reg_int(s1, src, REG_ITMP1);
			gen_nullptr_check(s1);

			if (iptr->val.a == NULL) {
/* 				PROFILE_CYCLE_STOP; */

				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_get_putfield,
									(unresolved_field *) iptr->target, 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

/* 				PROFILE_CYCLE_START; */

				disp = 0;

			} else {
				disp = ((fieldinfo *) (iptr->val.a))->offset;
			}

			switch (iptr->op1) {
			case TYPE_INT:
				d = reg_of_var(rd, iptr->dst, REG_ITMP1);
				if (iptr->val.a == NULL)
					M_ILD32(d, s1, disp);
				else
					M_ILD(d, s1, disp);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_LNG:
			case TYPE_ADR:
				d = reg_of_var(rd, iptr->dst, REG_ITMP1);
				if (iptr->val.a == NULL)
					M_LLD32(d, s1, disp);
				else
					M_LLD(d, s1, disp);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_FLT:
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				x86_64_movss_membase32_reg(cd, s1, disp, d);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			case TYPE_DBL:				
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				x86_64_movsd_membase32_reg(cd, s1, disp, d);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			}
			break;

		case ICMD_PUTFIELD:   /* ..., objectref, value  ==> ...               */
		                      /* op1 = type, val.i = field offset             */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			gen_nullptr_check(s1);

			if (IS_INT_LNG_TYPE(iptr->op1)) {
				var_to_reg_int(s2, src, REG_ITMP2);
			} else {
				var_to_reg_flt(s2, src, REG_FTMP2);
			}

			if (iptr->val.a == NULL) {
/* 				PROFILE_CYCLE_STOP; */

				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_get_putfield,
									(unresolved_field *) iptr->target, 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

/* 				PROFILE_CYCLE_START; */

				disp = 0;

			} else {
				disp = ((fieldinfo *) (iptr->val.a))->offset;
			}

			switch (iptr->op1) {
			case TYPE_INT:
				if (iptr->val.a == NULL)
					M_IST32(s2, s1, disp);
				else
					M_IST(s2, s1, disp);
				break;
			case TYPE_LNG:
			case TYPE_ADR:
				if (iptr->val.a == NULL)
					M_LST32(s2, s1, disp);
				else
					M_LST(s2, s1, disp);
				break;
			case TYPE_FLT:
				x86_64_movss_reg_membase32(cd, s2, s1, disp);
				break;
			case TYPE_DBL:
				x86_64_movsd_reg_membase32(cd, s2, s1, disp);
				break;
			}
			break;

		case ICMD_PUTFIELDCONST:  /* ..., objectref, value  ==> ...           */
		                          /* val = value (in current instruction)     */
		                          /* op1 = type, val.a = field address (in    */
		                          /* following NOP)                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			gen_nullptr_check(s1);

			if (iptr[1].val.a == NULL) {
/* 				PROFILE_CYCLE_STOP; */

				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_putfieldconst,
									(unresolved_field *) iptr[1].target, 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

/* 				PROFILE_CYCLE_START; */

				disp = 0;

			} else {
				disp = ((fieldinfo *) (iptr[1].val.a))->offset;
			}

			switch (iptr->op1) {
			case TYPE_INT:
			case TYPE_FLT:
				if (iptr[1].val.a == NULL)
					M_IST32_IMM(iptr->val.i, s1, disp);
				else
					M_IST_IMM(iptr->val.i, s1, disp);
				break;
			case TYPE_LNG:
			case TYPE_ADR:
			case TYPE_DBL:
				/* We can only optimize the move, if the class is
				   resolved. Otherwise we don't know what to patch. */
				if (iptr[1].val.a == NULL) {
					M_IST32_IMM(iptr->val.l, s1, disp);
					M_IST32_IMM(iptr->val.l >> 32, s1, disp + 4);
				} else {
					if (IS_IMM32(iptr->val.l)) {
						M_LST_IMM32(iptr->val.l, s1, disp);
					} else {
						M_IST_IMM(iptr->val.l, s1, disp);
						M_IST_IMM(iptr->val.l >> 32, s1, disp + 4);
					}
				}
				break;
			}
			break;


		/* branch operations **************************************************/

		case ICMD_ATHROW:       /* ..., objectref ==> ... (, objectref)       */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_INTMOVE(s1, REG_ITMP1_XPTR);

			PROFILE_CYCLE_STOP;

#ifdef ENABLE_VERIFIER
			if (iptr->val.a) {
				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_athrow_areturn,
									(unresolved_class *) iptr->val.a, 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}
			}
#endif /* ENABLE_VERIFIER */

			M_CALL_IMM(0);                            /* passing exception pc */
			M_POP(REG_ITMP2_XPC);

  			M_MOV_IMM((ptrint) asm_handle_exception, REG_ITMP3);
  			M_JMP(REG_ITMP3);
			break;

		case ICMD_GOTO:         /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */

			M_JMP_IMM(0);
			codegen_addreference(cd, (basicblock *) iptr->target, cd->mcodeptr);
			break;

		case ICMD_JSR:          /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */

  			M_CALL_IMM(0);
			codegen_addreference(cd, (basicblock *) iptr->target, cd->mcodeptr);
			break;
			
		case ICMD_RET:          /* ... ==> ...                                */
		                        /* op1 = local variable                       */

			var = &(rd->locals[iptr->op1][TYPE_ADR]);
			var_to_reg_int(s1, var, REG_ITMP1);
			M_JMP(s1);
			break;

		case ICMD_IFNULL:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			if (src->flags & INMEMORY)
				M_CMP_IMM_MEMBASE(0, REG_SP, src->regoff * 8);
			else
				M_TEST(src->regoff);
			M_BEQ(0);
			codegen_addreference(cd, (basicblock *) iptr->target, cd->mcodeptr);
			break;

		case ICMD_IFNONNULL:    /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			if (src->flags & INMEMORY)
				M_CMP_IMM_MEMBASE(0, REG_SP, src->regoff * 8);
			else
				M_TEST(src->regoff);
			M_BNE(0);
			codegen_addreference(cd, (basicblock *) iptr->target, cd->mcodeptr);
			break;

		case ICMD_IFEQ:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			x86_64_emit_ifcc(cd, X86_64_CC_E, src, iptr);
			break;

		case ICMD_IFLT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			x86_64_emit_ifcc(cd, X86_64_CC_L, src, iptr);
			break;

		case ICMD_IFLE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			x86_64_emit_ifcc(cd, X86_64_CC_LE, src, iptr);
			break;

		case ICMD_IFNE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			x86_64_emit_ifcc(cd, X86_64_CC_NE, src, iptr);
			break;

		case ICMD_IFGT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			x86_64_emit_ifcc(cd, X86_64_CC_G, src, iptr);
			break;

		case ICMD_IFGE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			x86_64_emit_ifcc(cd, X86_64_CC_GE, src, iptr);
			break;

		case ICMD_IF_LEQ:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			x86_64_emit_if_lcc(cd, X86_64_CC_E, src, iptr);
			break;

		case ICMD_IF_LLT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			x86_64_emit_if_lcc(cd, X86_64_CC_L, src, iptr);
			break;

		case ICMD_IF_LLE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			x86_64_emit_if_lcc(cd, X86_64_CC_LE, src, iptr);
			break;

		case ICMD_IF_LNE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			x86_64_emit_if_lcc(cd, X86_64_CC_NE, src, iptr);
			break;

		case ICMD_IF_LGT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			x86_64_emit_if_lcc(cd, X86_64_CC_G, src, iptr);
			break;

		case ICMD_IF_LGE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			x86_64_emit_if_lcc(cd, X86_64_CC_GE, src, iptr);
			break;

		case ICMD_IF_ICMPEQ:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			x86_64_emit_if_icmpcc(cd, X86_64_CC_E, src, iptr);
			break;

		case ICMD_IF_LCMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_ACMPEQ:    /* op1 = target JavaVM pc                     */

			x86_64_emit_if_lcmpcc(cd, X86_64_CC_E, src, iptr);
			break;

		case ICMD_IF_ICMPNE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			x86_64_emit_if_icmpcc(cd, X86_64_CC_NE, src, iptr);
			break;

		case ICMD_IF_LCMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_ACMPNE:    /* op1 = target JavaVM pc                     */

			x86_64_emit_if_lcmpcc(cd, X86_64_CC_NE, src, iptr);
			break;

		case ICMD_IF_ICMPLT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			x86_64_emit_if_icmpcc(cd, X86_64_CC_L, src, iptr);
			break;

		case ICMD_IF_LCMPLT:    /* ..., value, value ==> ...                  */
	                            /* op1 = target JavaVM pc                     */

			x86_64_emit_if_lcmpcc(cd, X86_64_CC_L, src, iptr);
			break;

		case ICMD_IF_ICMPGT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			x86_64_emit_if_icmpcc(cd, X86_64_CC_G, src, iptr);
			break;

		case ICMD_IF_LCMPGT:    /* ..., value, value ==> ...                  */
                                /* op1 = target JavaVM pc                     */

			x86_64_emit_if_lcmpcc(cd, X86_64_CC_G, src, iptr);
			break;

		case ICMD_IF_ICMPLE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			x86_64_emit_if_icmpcc(cd, X86_64_CC_LE, src, iptr);
			break;

		case ICMD_IF_LCMPLE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			x86_64_emit_if_lcmpcc(cd, X86_64_CC_LE, src, iptr);
			break;

		case ICMD_IF_ICMPGE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			x86_64_emit_if_icmpcc(cd, X86_64_CC_GE, src, iptr);
			break;

		case ICMD_IF_LCMPGE:    /* ..., value, value ==> ...                  */
	                            /* op1 = target JavaVM pc                     */

			x86_64_emit_if_lcmpcc(cd, X86_64_CC_GE, src, iptr);
			break;

		/* (value xx 0) ? IFxx_ICONST : ELSE_ICONST                           */

		case ICMD_ELSE_ICONST:  /* handled by IFxx_ICONST                     */
			break;

		case ICMD_IFEQ_ICONST:  /* ..., value ==> ..., constant               */
		case ICMD_IFNE_ICONST:  /* val.i = constant                           */
		case ICMD_IFLT_ICONST:
		case ICMD_IFGE_ICONST:
		case ICMD_IFGT_ICONST:
		case ICMD_IFLE_ICONST:

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr[1].opc == ICMD_ELSE_ICONST) {
				if (s1 == d) {
					M_INTMOVE(s1, REG_ITMP1);
					s1 = REG_ITMP1;
				}
				if (iptr[1].val.i == 0)
					M_CLR(d);
				else
					M_IMOV_IMM(iptr[1].val.i, d);
			}
			if (iptr->val.i == 0)
				M_CLR(REG_ITMP2);
			else
				M_IMOV_IMM(iptr->val.i, REG_ITMP2);
			M_ITEST(s1);

			switch (iptr->opc) {
			case ICMD_IFEQ_ICONST:
				M_CMOVEQ(REG_ITMP2, d);
				break;
			case ICMD_IFNE_ICONST:
				M_CMOVNE(REG_ITMP2, d);
				break;
			case ICMD_IFLT_ICONST:
				M_CMOVLT(REG_ITMP2, d);
				break;
			case ICMD_IFGE_ICONST:
				M_CMOVGE(REG_ITMP2, d);
				break;
			case ICMD_IFGT_ICONST:
				M_CMOVGT(REG_ITMP2, d);
				break;
			case ICMD_IFLE_ICONST:
				M_CMOVLE(REG_ITMP2, d);
				break;
			}

			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_IRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_LRETURN:

			var_to_reg_int(s1, src, REG_RESULT);
			M_INTMOVE(s1, REG_RESULT);
			goto nowperformreturn;

		case ICMD_ARETURN:      /* ..., retvalue ==> ...                      */

			var_to_reg_int(s1, src, REG_RESULT);
			M_INTMOVE(s1, REG_RESULT);

#ifdef ENABLE_VERIFIER
			if (iptr->val.a) {
				PROFILE_CYCLE_STOP;

				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_athrow_areturn,
									(unresolved_class *) iptr->val.a, 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

				PROFILE_CYCLE_START;
			}
#endif /* ENABLE_VERIFIER */
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
				x86_64_alu_imm_reg(cd, X86_64_SUB, 2 * 8, REG_SP);

				x86_64_mov_reg_membase(cd, REG_RESULT, REG_SP, 0 * 8);
				x86_64_movq_reg_membase(cd, REG_FRESULT, REG_SP, 1 * 8);

  				x86_64_mov_imm_reg(cd, (u8) m, rd->argintregs[0]);
  				x86_64_mov_reg_reg(cd, REG_RESULT, rd->argintregs[1]);
				M_FLTMOVE(REG_FRESULT, rd->argfltregs[0]);
 				M_FLTMOVE(REG_FRESULT, rd->argfltregs[1]);

  				x86_64_mov_imm_reg(cd, (u8) builtin_displaymethodstop, REG_ITMP1);
				x86_64_call_reg(cd, REG_ITMP1);

				x86_64_mov_membase_reg(cd, REG_SP, 0 * 8, REG_RESULT);
				x86_64_movq_membase_reg(cd, REG_SP, 1 * 8, REG_FRESULT);

				x86_64_alu_imm_reg(cd, X86_64_ADD, 2 * 8, REG_SP);
			}

#if defined(USE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				M_ALD(rd->argintregs[0], REG_SP, rd->memuse * 8);
	
				/* we need to save the proper return value */
				switch (iptr->opc) {
				case ICMD_IRETURN:
				case ICMD_ARETURN:
				case ICMD_LRETURN:
					M_LST(REG_RESULT, REG_SP, rd->memuse * 8);
					break;
				case ICMD_FRETURN:
				case ICMD_DRETURN:
					M_DST(REG_FRESULT, REG_SP, rd->memuse * 8);
					break;
				}

				M_MOV_IMM((ptrint) builtin_monitorexit, REG_ITMP1);
				M_CALL(REG_ITMP1);

				/* and now restore the proper return value */
				switch (iptr->opc) {
				case ICMD_IRETURN:
				case ICMD_ARETURN:
				case ICMD_LRETURN:
					M_LLD(REG_RESULT, REG_SP, rd->memuse * 8);
					break;
				case ICMD_FRETURN:
				case ICMD_DRETURN:
					M_DLD(REG_FRESULT, REG_SP, rd->memuse * 8);
					break;
				}
			}
#endif

			/* restore saved registers */

			for (i = INT_SAV_CNT - 1; i >= rd->savintreguse; i--) {
				p--; M_LLD(rd->savintregs[i], REG_SP, p * 8);
			}
			for (i = FLT_SAV_CNT - 1; i >= rd->savfltreguse; i--) {
  				p--; M_DLD(rd->savfltregs[i], REG_SP, p * 8);
			}

			/* deallocate stack */

			if (parentargs_base)
				M_AADD_IMM(parentargs_base * 8, REG_SP);

			/* generate method profiling code */

			PROFILE_CYCLE_STOP;

			M_RET;
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
				M_INTMOVE(s1, REG_ITMP1);
				if (l != 0) {
					x86_64_alul_imm_reg(cd, X86_64_SUB, l, REG_ITMP1);
				}
				i = i - l + 1;

                /* range check */
				x86_64_alul_imm_reg(cd, X86_64_CMP, i - 1, REG_ITMP1);
				x86_64_jcc(cd, X86_64_CC_A, 0);

                /* codegen_addreference(cd, BlockPtrOfPC(s4ptr[0]), cd->mcodeptr); */
				codegen_addreference(cd, (basicblock *) tptr[0], cd->mcodeptr);

				/* build jump table top down and use address of lowest entry */

                /* s4ptr += 3 + i; */
				tptr += i;

				while (--i >= 0) {
					dseg_addtarget(cd, (basicblock *) tptr[0]); 
					--tptr;
				}

				/* length of dataseg after last dseg_addtarget is used by load */

				x86_64_mov_imm_reg(cd, 0, REG_ITMP2);
				dseg_adddata(cd, cd->mcodeptr);
				x86_64_mov_memindex_reg(cd, -(cd->dseglen), REG_ITMP2, REG_ITMP1, 3, REG_ITMP1);
				x86_64_jmp_reg(cd, REG_ITMP1);
			}
			break;


		case ICMD_LOOKUPSWITCH: /* ..., key ==> ...                           */
			{
				s4 i, l, val, *s4ptr;
				void **tptr;

				tptr = (void **) iptr->target;

				s4ptr = iptr->val.a;
				l = s4ptr[0];                          /* default  */
				i = s4ptr[1];                          /* count    */
			
				MCODECHECK(8 + ((7 + 6) * i) + 5);
				var_to_reg_int(s1, src, REG_ITMP1);    /* reg compare should always be faster */
				while (--i >= 0) {
					s4ptr += 2;
					++tptr;

					val = s4ptr[0];
					x86_64_alul_imm_reg(cd, X86_64_CMP, val, s1);
					x86_64_jcc(cd, X86_64_CC_E, 0);
					codegen_addreference(cd, (basicblock *) tptr[0], cd->mcodeptr); 
				}

				x86_64_jmp_imm(cd, 0);
			
				tptr = (void **) iptr->target;
				codegen_addreference(cd, (basicblock *) tptr[0], cd->mcodeptr);
			}
			break;


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

			if (lm == NULL) {
				unresolved_method *um = iptr->target;
				md = um->methodref->parseddesc.md;
			} else {
				md = lm->parseddesc;
			}

gen_method:
			s3 = md->paramcount;

			MCODECHECK((20 * s3) + 128);

			/* copy arguments to registers or stack location */

			for (s3 = s3 - 1; s3 >= 0; s3--, src = src->prev) {
				if (src->varkind == ARGVAR)
					continue;
				if (IS_INT_LNG_TYPE(src->type)) {
					if (!md->params[s3].inmemory) {
						s1 = rd->argintregs[md->params[s3].regoff];
						var_to_reg_int(d, src, s1);
						M_INTMOVE(d, s1);
					} else {
						var_to_reg_int(d, src, REG_ITMP1);
						M_LST(d, REG_SP, md->params[s3].regoff * 8);
					}
						
				} else {
					if (!md->params[s3].inmemory) {
						s1 = rd->argfltregs[md->params[s3].regoff];
						var_to_reg_flt(d, src, s1);
						M_FLTMOVE(d, s1);
					} else {
						var_to_reg_flt(d, src, REG_FTMP1);
						M_DST(d, REG_SP, md->params[s3].regoff * 8);
					}
				}
			}

			/* generate method profiling code */

			PROFILE_CYCLE_STOP;

			switch (iptr->opc) {
			case ICMD_BUILTIN:
				a = (ptrint) bte->fp;
				d = md->returntype.type;

				M_MOV_IMM(a, REG_ITMP1);
				M_CALL(REG_ITMP1);

				/* if op1 == true, we need to check for an exception */

				if (iptr->op1 == true) {
					M_TEST(REG_RESULT);
					M_BEQ(0);
					codegen_addxexceptionrefs(cd, cd->mcodeptr);
				}
				break;

			case ICMD_INVOKESPECIAL:
				M_TEST(rd->argintregs[0]);
				M_BEQ(0);
				codegen_addxnullrefs(cd, cd->mcodeptr);

				/* first argument contains pointer */
/*  				gen_nullptr_check(rd->argintregs[0]); */

				/* access memory for hardware nullptr */
/*  				x86_64_mov_membase_reg(cd, rd->argintregs[0], 0, REG_ITMP2); */

				/* fall through */

			case ICMD_INVOKESTATIC:
				if (lm == NULL) {
					unresolved_method *um = iptr->target;

					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_invokestatic_special, um, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}

					a = 0;
					d = um->methodref->parseddesc.md->returntype.type;

				} else {
					a = (ptrint) lm->stubroutine;
					d = lm->parseddesc->returntype.type;
				}

				M_MOV_IMM(a, REG_ITMP2);
				M_CALL(REG_ITMP2);
				break;

			case ICMD_INVOKEVIRTUAL:
				gen_nullptr_check(rd->argintregs[0]);

				if (lm == NULL) {
					unresolved_method *um = iptr->target;

					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_invokevirtual, um, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}

					s1 = 0;
					d = um->methodref->parseddesc.md->returntype.type;

				} else {
					s1 = OFFSET(vftbl_t, table[0]) +
						sizeof(methodptr) * lm->vftblindex;
					d = lm->parseddesc->returntype.type;
				}

				x86_64_mov_membase_reg(cd, rd->argintregs[0],
									   OFFSET(java_objectheader, vftbl),
									   REG_ITMP2);
				x86_64_mov_membase32_reg(cd, REG_ITMP2, s1, REG_ITMP1);
				M_CALL(REG_ITMP1);
				break;

			case ICMD_INVOKEINTERFACE:
				gen_nullptr_check(rd->argintregs[0]);

				if (lm == NULL) {
					unresolved_method *um = iptr->target;

					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_invokeinterface, um, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}

					s1 = 0;
					s2 = 0;
					d = um->methodref->parseddesc.md->returntype.type;

				} else {
					s1 = OFFSET(vftbl_t, interfacetable[0]) -
						sizeof(methodptr) * lm->class->index;

					s2 = sizeof(methodptr) * (lm - lm->class->methods);

					d = lm->parseddesc->returntype.type;
				}

				M_ALD(REG_ITMP2, rd->argintregs[0],
					  OFFSET(java_objectheader, vftbl));
				x86_64_mov_membase32_reg(cd, REG_ITMP2, s1, REG_ITMP2);
				x86_64_mov_membase32_reg(cd, REG_ITMP2, s2, REG_ITMP1);
				M_CALL(REG_ITMP1);
				break;
			}

			/* generate method profiling code */

			PROFILE_CYCLE_START;

			/* d contains return type */

			if (d != TYPE_VOID) {
				if (IS_INT_LNG_TYPE(iptr->dst->type)) {
					s1 = reg_of_var(rd, iptr->dst, REG_RESULT);
					M_INTMOVE(REG_RESULT, s1);
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
		                      /* val.a: (classinfo *) superclass              */

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
			 *         super->vftbl->diffval));
			 */

			if (iptr->op1 == 1) {
				/* object type cast-check */

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
				codegen_threadcritrestart(cd, cd->mcodeptr - cd->mcodebase);
#endif
				var_to_reg_int(s1, src, REG_ITMP1);

				/* calculate interface checkcast code size */

				s2 = 3; /* mov_membase_reg */
				CALCOFFSETBYTES(s2, s1, OFFSET(java_objectheader, vftbl));

				s2 += 3 + 4 /* movl_membase32_reg */ + 3 + 4 /* sub imm32 */ +
					3 /* test */ + 6 /* jcc */ + 3 + 4 /* mov_membase32_reg */ +
					3 /* test */ + 6 /* jcc */;

				if (!super)
					s2 += (opt_showdisassemble ? 5 : 0);

				/* calculate class checkcast code size */

				s3 = 3; /* mov_membase_reg */
				CALCOFFSETBYTES(s3, s1, OFFSET(java_objectheader, vftbl));
				s3 += 10 /* mov_imm_reg */ + 3 + 4 /* movl_membase32_reg */;

#if 0
				if (s1 != REG_ITMP1) {
					a += 3;    /* movl_membase_reg - only if REG_ITMP3 == R11 */
					CALCOFFSETBYTES(a, REG_ITMP3, OFFSET(vftbl_t, baseval));
					a += 3;    /* movl_membase_reg - only if REG_ITMP3 == R11 */
					CALCOFFSETBYTES(a, REG_ITMP3, OFFSET(vftbl_t, diffval));
					a += 3;    /* sub */
				
				} else
#endif
					{
						s3 += 3 + 4 /* movl_membase32_reg */ + 3 /* sub */ +
							10 /* mov_imm_reg */ + 3 /* movl_membase_reg */;
						CALCOFFSETBYTES(s3, REG_ITMP3, OFFSET(vftbl_t, diffval));
					}
			
				s3 += 3 /* cmp */ + 6 /* jcc */;

				if (!super)
					s3 += (opt_showdisassemble ? 5 : 0);

				/* if class is not resolved, check which code to call */

				if (!super) {
					M_TEST(s1);
					M_BEQ(6 + (opt_showdisassemble ? 5 : 0) + 7 + 6 + s2 + 5 + s3);

					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_checkcast_instanceof_flags,
										(constant_classref *) iptr->target, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}

					M_IMOV_IMM(0, REG_ITMP2);                 /* super->flags */
					M_IAND_IMM(ACC_INTERFACE, REG_ITMP2);
					M_BEQ(s2 + 5);
				}

				/* interface checkcast code */

				if (!super || (super->flags & ACC_INTERFACE)) {
					if (super) {
						M_TEST(s1);
						M_BEQ(s2);
					}

					M_ALD(REG_ITMP2, s1, OFFSET(java_objectheader, vftbl));

					if (!super) {
						codegen_addpatchref(cd, cd->mcodeptr,
											PATCHER_checkcast_instanceof_interface,
											(constant_classref *) iptr->target, 0);

						if (opt_showdisassemble) {
							M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
						}
					}

					x86_64_movl_membase32_reg(cd, REG_ITMP2,
											  OFFSET(vftbl_t, interfacetablelength),
											  REG_ITMP3);
					/* XXX TWISTI: should this be int arithmetic? */
					M_LSUB_IMM32(superindex, REG_ITMP3);
					M_TEST(REG_ITMP3);
					M_BLE(0);
					codegen_addxcastrefs(cd, cd->mcodeptr);
					x86_64_mov_membase32_reg(cd, REG_ITMP2,
											 OFFSET(vftbl_t, interfacetable[0]) -
											 superindex * sizeof(methodptr*),
											 REG_ITMP3);
					M_TEST(REG_ITMP3);
					M_BEQ(0);
					codegen_addxcastrefs(cd, cd->mcodeptr);

					if (!super)
						M_JMP_IMM(s3);
				}

				/* class checkcast code */

				if (!super || !(super->flags & ACC_INTERFACE)) {
					if (super) {
						M_TEST(s1);
						M_BEQ(s3);
					}

					M_ALD(REG_ITMP2, s1, OFFSET(java_objectheader, vftbl));

					if (!super) {
						codegen_addpatchref(cd, cd->mcodeptr,
											PATCHER_checkcast_class,
											(constant_classref *) iptr->target,
											0);

						if (opt_showdisassemble) {
							M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
						}
					}

					M_MOV_IMM((ptrint) supervftbl, REG_ITMP3);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
					codegen_threadcritstart(cd, cd->mcodeptr - cd->mcodebase);
#endif
					x86_64_movl_membase32_reg(cd, REG_ITMP2,
											  OFFSET(vftbl_t, baseval),
											  REG_ITMP2);
					/*  					if (s1 != REG_ITMP1) { */
					/*  						x86_64_movl_membase_reg(cd, REG_ITMP3, */
					/*  												OFFSET(vftbl_t, baseval), */
					/*  												REG_ITMP1); */
					/*  						x86_64_movl_membase_reg(cd, REG_ITMP3, */
					/*  												OFFSET(vftbl_t, diffval), */
					/*  												REG_ITMP3); */
					/*  #if defined(USE_THREADS) && defined(NATIVE_THREADS) */
					/*  						codegen_threadcritstop(cd, cd->mcodeptr - cd->mcodebase); */
					/*  #endif */
					/*  						x86_64_alu_reg_reg(cd, X86_64_SUB, REG_ITMP1, REG_ITMP2); */

					/*  					} else { */
					x86_64_movl_membase32_reg(cd, REG_ITMP3,
											  OFFSET(vftbl_t, baseval),
											  REG_ITMP3);
					M_LSUB(REG_ITMP3, REG_ITMP2);
					M_MOV_IMM((ptrint) supervftbl, REG_ITMP3);
					M_ILD(REG_ITMP3, REG_ITMP3, OFFSET(vftbl_t, diffval));
					/*  					} */
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
					codegen_threadcritstop(cd, cd->mcodeptr - cd->mcodebase);
#endif
					M_CMP(REG_ITMP3, REG_ITMP2);
					M_BA(0);         /* (u) REG_ITMP1 > (u) REG_ITMP2 -> jump */
					codegen_addxcastrefs(cd, cd->mcodeptr);
				}
				d = reg_of_var(rd, iptr->dst, REG_ITMP3);

			} else {
				/* array type cast-check */

				var_to_reg_int(s1, src, REG_ITMP1);
				M_INTMOVE(s1, rd->argintregs[0]);

				if (iptr->val.a == NULL) {
					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_builtin_arraycheckcast,
										(constant_classref *) iptr->target, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}
				}

				M_MOV_IMM((ptrint) iptr->val.a, rd->argintregs[1]);
				M_MOV_IMM((ptrint) BUILTIN_arraycheckcast, REG_ITMP1);
				M_CALL(REG_ITMP1);
				M_TEST(REG_RESULT);
				M_BEQ(0);
				codegen_addxcastrefs(cd, cd->mcodeptr);

				var_to_reg_int(s1, src, REG_ITMP1);
				d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			}
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INSTANCEOF: /* ..., objectref ==> ..., intresult            */

		                      /* op1:   0 == array, 1 == class                */
		                      /* val.a: (classinfo *) superclass              */

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
            codegen_threadcritrestart(cd, cd->mcodeptr - cd->mcodebase);
#endif

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if (s1 == d) {
				M_INTMOVE(s1, REG_ITMP1);
				s1 = REG_ITMP1;
			}

			/* calculate interface instanceof code size */

			s2 = 3; /* mov_membase_reg */
			CALCOFFSETBYTES(s2, s1, OFFSET(java_objectheader, vftbl));
			s2 += 3 + 4 /* movl_membase32_reg */ + 3 + 4 /* sub_imm32 */ +
				3 /* test */ + 6 /* jcc */ + 3 + 4 /* mov_membase32_reg */ +
				3 /* test */ + 4 /* setcc */;

			if (!super)
				s2 += (opt_showdisassemble ? 5 : 0);

			/* calculate class instanceof code size */
			
			s3 = 3; /* mov_membase_reg */
			CALCOFFSETBYTES(s3, s1, OFFSET(java_objectheader, vftbl));
			s3 += 10; /* mov_imm_reg */
			s3 += 2; /* movl_membase_reg - only if REG_ITMP1 == RAX */
			CALCOFFSETBYTES(s3, REG_ITMP1, OFFSET(vftbl_t, baseval));
			s3 += 3; /* movl_membase_reg - only if REG_ITMP2 == R10 */
			CALCOFFSETBYTES(s3, REG_ITMP2, OFFSET(vftbl_t, baseval));
			s3 += 3; /* movl_membase_reg - only if REG_ITMP2 == R10 */
			CALCOFFSETBYTES(s3, REG_ITMP2, OFFSET(vftbl_t, diffval));
			s3 += 3 /* sub */ + 3 /* xor */ + 3 /* cmp */ + 4 /* setcc */;

			if (!super)
				s3 += (opt_showdisassemble ? 5 : 0);

			x86_64_alu_reg_reg(cd, X86_64_XOR, d, d);

			/* if class is not resolved, check which code to call */

			if (!super) {
				x86_64_test_reg_reg(cd, s1, s1);
				x86_64_jcc(cd, X86_64_CC_Z, (6 + (opt_showdisassemble ? 5 : 0) +
											 7 + 6 + s2 + 5 + s3));

				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_checkcast_instanceof_flags,
									(constant_classref *) iptr->target, 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

				x86_64_movl_imm_reg(cd, 0, REG_ITMP3); /* super->flags */
				x86_64_alul_imm_reg(cd, X86_64_AND, ACC_INTERFACE, REG_ITMP3);
				x86_64_jcc(cd, X86_64_CC_Z, s2 + 5);
			}

			/* interface instanceof code */

			if (!super || (super->flags & ACC_INTERFACE)) {
				if (super) {
					x86_64_test_reg_reg(cd, s1, s1);
					x86_64_jcc(cd, X86_64_CC_Z, s2);
				}

				x86_64_mov_membase_reg(cd, s1,
									   OFFSET(java_objectheader, vftbl),
									   REG_ITMP1);
				if (!super) {
					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_checkcast_instanceof_interface,
										(constant_classref *) iptr->target, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}
				}

				x86_64_movl_membase32_reg(cd, REG_ITMP1,
										  OFFSET(vftbl_t, interfacetablelength),
										  REG_ITMP3);
				x86_64_alu_imm32_reg(cd, X86_64_SUB, superindex, REG_ITMP3);
				x86_64_test_reg_reg(cd, REG_ITMP3, REG_ITMP3);

				a = 3 + 4 /* mov_membase32_reg */ + 3 /* test */ + 4 /* setcc */;

				x86_64_jcc(cd, X86_64_CC_LE, a);
				x86_64_mov_membase32_reg(cd, REG_ITMP1,
										 OFFSET(vftbl_t, interfacetable[0]) -
										 superindex * sizeof(methodptr*),
										 REG_ITMP1);
				x86_64_test_reg_reg(cd, REG_ITMP1, REG_ITMP1);
				x86_64_setcc_reg(cd, X86_64_CC_NE, d);

				if (!super)
					x86_64_jmp_imm(cd, s3);
			}

			/* class instanceof code */

			if (!super || !(super->flags & ACC_INTERFACE)) {
				if (super) {
					x86_64_test_reg_reg(cd, s1, s1);
					x86_64_jcc(cd, X86_64_CC_E, s3);
				}

				x86_64_mov_membase_reg(cd, s1,
									   OFFSET(java_objectheader, vftbl),
									   REG_ITMP1);

				if (!super) {
					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_instanceof_class,
										(constant_classref *) iptr->target, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}
				}

				x86_64_mov_imm_reg(cd, (ptrint) supervftbl, REG_ITMP2);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
				codegen_threadcritstart(cd, cd->mcodeptr - cd->mcodebase);
#endif
				x86_64_movl_membase_reg(cd, REG_ITMP1,
										OFFSET(vftbl_t, baseval),
										REG_ITMP1);
				x86_64_movl_membase_reg(cd, REG_ITMP2,
										OFFSET(vftbl_t, diffval),
										REG_ITMP3);
				x86_64_movl_membase_reg(cd, REG_ITMP2,
										OFFSET(vftbl_t, baseval),
										REG_ITMP2);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
				codegen_threadcritstop(cd, cd->mcodeptr - cd->mcodebase);
#endif
				x86_64_alu_reg_reg(cd, X86_64_SUB, REG_ITMP2, REG_ITMP1);
				x86_64_alu_reg_reg(cd, X86_64_XOR, d, d); /* may be REG_ITMP2 */
				x86_64_alu_reg_reg(cd, X86_64_CMP, REG_ITMP3, REG_ITMP1);
				x86_64_setcc_reg(cd, X86_64_CC_BE, d);
			}
  			store_reg_to_var_int(iptr->dst, d);
			}
			break;

		case ICMD_MULTIANEWARRAY:/* ..., cnt1, [cnt2, ...] ==> ..., arrayref  */
		                         /* op1 = dimension, val.a = class            */

			/* check for negative sizes and copy sizes to stack if necessary  */

  			MCODECHECK((10 * 4 * iptr->op1) + 5 + 10 * 8);

			for (s1 = iptr->op1; --s1 >= 0; src = src->prev) {
				/* copy SAVEDVAR sizes to stack */

				if (src->varkind != ARGVAR) {
					var_to_reg_int(s2, src, REG_ITMP1);
					M_LST(s2, REG_SP, s1 * 8);
				}
			}

			/* is a patcher function set? */

			if (iptr->val.a == NULL) {
				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_builtin_multianewarray,
									(constant_classref *) iptr->target, 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

				a = 0;

			} else {
				a = (ptrint) iptr->val.a;
			}

			/* a0 = dimension count */

			M_MOV_IMM(iptr->op1, rd->argintregs[0]);

			/* a1 = arrayvftbl */

			M_MOV_IMM((ptrint) iptr->val.a, rd->argintregs[1]);

			/* a2 = pointer to dimensions = stack pointer */

			M_MOV(REG_SP, rd->argintregs[2]);

			M_MOV_IMM((ptrint) BUILTIN_multianewarray, REG_ITMP1);
			M_CALL(REG_ITMP1);

			/* check for exception before result assignment */

			M_TEST(REG_RESULT);
			M_BEQ(0);
			codegen_addxexceptionrefs(cd, cd->mcodeptr);

			s1 = reg_of_var(rd, iptr->dst, REG_RESULT);
			M_INTMOVE(REG_RESULT, s1);
			store_reg_to_var_int(iptr->dst, s1);
			break;

		default:
			*exceptionptr = new_internalerror("Unknown ICMD %d", iptr->opc);
			return false;
	} /* switch */

	} /* for instruction */
		
	/* copy values to interface registers */

	src = bptr->outstack;
	len = bptr->outdepth;
	MCODECHECK(512);
#if defined(ENABLE_LSRA)
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
					x86_64_movq_reg_membase(cd, s1, REG_SP, rd->interfaces[len][s2].regoff * 8);
				}

			} else {
				var_to_reg_int(s1, src, REG_ITMP1);
				if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
					M_INTMOVE(s1, rd->interfaces[len][s2].regoff);

				} else {
					x86_64_mov_reg_membase(cd, s1, REG_SP, rd->interfaces[len][s2].regoff * 8);
				}
			}
		}
		src = src->prev;
	}

	/* At the end of a basic block we may have to append some nops,
	   because the patcher stub calling code might be longer than the
	   actual instruction. So codepatching does not change the
	   following block unintentionally. */

	if (cd->mcodeptr < cd->lastmcodeptr) {
		while (cd->mcodeptr < cd->lastmcodeptr) {
			M_NOP;
		}
	}

	} /* if (bptr -> flags >= BBREACHED) */
	} /* for basic block */

	dseg_createlinenumbertable(cd);

	{

	u1        *xcodeptr;
	branchref *bref;

	/* generate ArithmeticException stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xdivrefs; bref != NULL; bref = bref->next) {
		gen_resolvebranch(cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  cd->mcodeptr - cd->mcodebase);

		MCODECHECK(512);

		M_MOV_IMM(0, REG_ITMP2_XPC);                              /* 10 bytes */
		dseg_adddata(cd, cd->mcodeptr);
		M_AADD_IMM32(bref->branchpos - 6, REG_ITMP2_XPC);         /* 7 bytes  */

		if (xcodeptr != NULL) {
			M_JMP_IMM(xcodeptr - cd->mcodeptr - 5);
		
		} else {
			xcodeptr = cd->mcodeptr;

			x86_64_lea_membase_reg(cd, RIP, -(((ptrint) cd->mcodeptr + 7) - (ptrint) cd->mcodebase), rd->argintregs[0]);
			M_MOV(REG_SP, rd->argintregs[1]);
			M_ALD(rd->argintregs[2], REG_SP, parentargs_base * 8);
			M_MOV(REG_ITMP2_XPC, rd->argintregs[3]);

			M_ASUB_IMM(2 * 8, REG_SP);
			M_AST(REG_ITMP2_XPC, REG_SP, 0 * 8);

			M_MOV_IMM((ptrint) stacktrace_inline_arithmeticexception,
					  REG_ITMP3);
			M_CALL(REG_ITMP3);

			M_ALD(REG_ITMP2_XPC, REG_SP, 0 * 8);
			M_AADD_IMM(2 * 8, REG_SP);

			M_MOV_IMM((ptrint) asm_handle_exception, REG_ITMP3);
			M_JMP(REG_ITMP3);
		}
	}

	/* generate ArrayIndexOutOfBoundsException stubs */

	xcodeptr = NULL;

	for (bref = cd->xboundrefs; bref != NULL; bref = bref->next) {
		gen_resolvebranch(cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  cd->mcodeptr - cd->mcodebase);

		MCODECHECK(512);

		/* move index register into REG_ITMP1 */

		M_MOV(bref->reg, REG_ITMP1);                              /* 3 bytes  */

		M_MOV_IMM(0, REG_ITMP2_XPC);                              /* 10 bytes */
		dseg_adddata(cd, cd->mcodeptr);
		M_AADD_IMM32(bref->branchpos - 6, REG_ITMP2_XPC);         /* 7 bytes  */

		if (xcodeptr != NULL) {
			M_JMP_IMM(xcodeptr - cd->mcodeptr - 5);

		} else {
			xcodeptr = cd->mcodeptr;

			x86_64_lea_membase_reg(cd, RIP, -(((ptrint) cd->mcodeptr + 7) - (ptrint) cd->mcodebase), rd->argintregs[0]);
			M_MOV(REG_SP, rd->argintregs[1]);
			M_ALD(rd->argintregs[2], REG_SP, parentargs_base * 8);
			M_MOV(REG_ITMP2_XPC, rd->argintregs[3]);
			M_MOV(REG_ITMP1, rd->argintregs[4]);

			M_ASUB_IMM(2 * 8, REG_SP);
			M_AST(REG_ITMP2_XPC, REG_SP, 0 * 8);

			M_MOV_IMM((ptrint) stacktrace_inline_arrayindexoutofboundsexception,
					  REG_ITMP3);
			M_CALL(REG_ITMP3);

			M_ALD(REG_ITMP2_XPC, REG_SP, 0 * 8);
			M_AADD_IMM(2 * 8, REG_SP);

			M_MOV_IMM((ptrint) asm_handle_exception, REG_ITMP3);
			M_JMP(REG_ITMP3);
		}
	}

	/* generate ArrayStoreException stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xstorerefs; bref != NULL; bref = bref->next) {
		gen_resolvebranch(cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  cd->mcodeptr - cd->mcodebase);

		MCODECHECK(512);

		M_MOV_IMM(0, REG_ITMP2_XPC);                              /* 10 bytes */
		dseg_adddata(cd, cd->mcodeptr);
		M_AADD_IMM32(bref->branchpos - 6, REG_ITMP2_XPC);         /* 7 bytes  */

		if (xcodeptr != NULL) {
			M_JMP_IMM(xcodeptr - cd->mcodeptr - 5);

		} else {
			xcodeptr = cd->mcodeptr;

			x86_64_lea_membase_reg(cd, RIP, -(((ptrint) cd->mcodeptr + 7) - (ptrint) cd->mcodebase), rd->argintregs[0]);
			M_MOV(REG_SP, rd->argintregs[1]);
			M_ALD(rd->argintregs[2], REG_SP, parentargs_base * 8);
			M_MOV(REG_ITMP2_XPC, rd->argintregs[3]);

			M_ASUB_IMM(2 * 8, REG_SP);
			M_AST(REG_ITMP2_XPC, REG_SP, 0 * 8);

			M_MOV_IMM((ptrint) stacktrace_inline_arraystoreexception,
					  REG_ITMP3);
			M_CALL(REG_ITMP3);

			M_ALD(REG_ITMP2_XPC, REG_SP, 0 * 8);
			M_AADD_IMM(2 * 8, REG_SP);

			M_MOV_IMM((ptrint) asm_handle_exception, REG_ITMP3);
			M_JMP(REG_ITMP3);
		}
	}

	/* generate ClassCastException stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xcastrefs; bref != NULL; bref = bref->next) {
		gen_resolvebranch(cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  cd->mcodeptr - cd->mcodebase);

		MCODECHECK(512);

		M_MOV_IMM(0, REG_ITMP2_XPC);                              /* 10 bytes */
		dseg_adddata(cd, cd->mcodeptr);
		M_AADD_IMM32(bref->branchpos - 6, REG_ITMP2_XPC);         /* 7 bytes  */

		if (xcodeptr != NULL) {
			M_JMP_IMM(xcodeptr - cd->mcodeptr - 5);
		
		} else {
			xcodeptr = cd->mcodeptr;

			x86_64_lea_membase_reg(cd, RIP, -(((ptrint) cd->mcodeptr + 7) - (ptrint) cd->mcodebase), rd->argintregs[0]);
			M_MOV(REG_SP, rd->argintregs[1]);
			M_ALD(rd->argintregs[2], REG_SP, parentargs_base * 8);
			M_MOV(REG_ITMP2_XPC, rd->argintregs[3]);

			M_ASUB_IMM(2 * 8, REG_SP);
			M_AST(REG_ITMP2_XPC, REG_SP, 0 * 8);

			M_MOV_IMM((ptrint) stacktrace_inline_classcastexception, REG_ITMP3);
			M_CALL(REG_ITMP3);

			M_ALD(REG_ITMP2_XPC, REG_SP, 0 * 8);
			M_AADD_IMM(2 * 8, REG_SP);

			M_MOV_IMM((ptrint) asm_handle_exception, REG_ITMP3);
			M_JMP(REG_ITMP3);
		}
	}

	/* generate NullpointerException stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xnullrefs; bref != NULL; bref = bref->next) {
		gen_resolvebranch(cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  cd->mcodeptr - cd->mcodebase);

		MCODECHECK(512);

		M_MOV_IMM(0, REG_ITMP2_XPC);                              /* 10 bytes */
		dseg_adddata(cd, cd->mcodeptr);
		M_AADD_IMM32(bref->branchpos - 6, REG_ITMP2_XPC);         /* 7 bytes  */

		if (xcodeptr != NULL) {
			M_JMP_IMM(xcodeptr - cd->mcodeptr - 5);
		
		} else {
			xcodeptr = cd->mcodeptr;

			x86_64_lea_membase_reg(cd, RIP, -(((ptrint) cd->mcodeptr + 7) - (ptrint) cd->mcodebase), rd->argintregs[0]);
			M_MOV(REG_SP, rd->argintregs[1]);
			M_ALD(rd->argintregs[2], REG_SP, parentargs_base * 8);
			M_MOV(REG_ITMP2_XPC, rd->argintregs[3]);

			M_ASUB_IMM(2 * 8, REG_SP);
			M_AST(REG_ITMP2_XPC, REG_SP, 0 * 8);

			M_MOV_IMM((ptrint) stacktrace_inline_nullpointerexception,
					  REG_ITMP3);
			M_CALL(REG_ITMP3);

			M_ALD(REG_ITMP2_XPC, REG_SP, 0 * 8);
			M_AADD_IMM(2 * 8, REG_SP);

			M_MOV_IMM((ptrint) asm_handle_exception, REG_ITMP3);
			M_JMP(REG_ITMP3);
		}
	}

	/* generate exception check stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xexceptionrefs; bref != NULL; bref = bref->next) {
		gen_resolvebranch(cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  cd->mcodeptr - cd->mcodebase);

		MCODECHECK(512);

		M_MOV_IMM(0, REG_ITMP2_XPC);                              /* 10 bytes */
		dseg_adddata(cd, cd->mcodeptr);
		M_AADD_IMM32(bref->branchpos - 6, REG_ITMP2_XPC);         /* 7 bytes  */

		if (xcodeptr != NULL) {
			M_JMP_IMM(xcodeptr - cd->mcodeptr - 5);
		
		} else {
			xcodeptr = cd->mcodeptr;

			x86_64_lea_membase_reg(cd, RIP, -(((ptrint) cd->mcodeptr + 7) - (ptrint) cd->mcodebase), rd->argintregs[0]);
			M_MOV(REG_SP, rd->argintregs[1]);
			M_ALD(rd->argintregs[2], REG_SP, parentargs_base * 8);
			M_MOV(REG_ITMP2_XPC, rd->argintregs[3]);

			M_ASUB_IMM(2 * 8, REG_SP);
			M_AST(REG_ITMP2_XPC, REG_SP, 0 * 8);

			M_MOV_IMM((ptrint) stacktrace_inline_fillInStackTrace, REG_ITMP3);
			M_CALL(REG_ITMP3);

			M_ALD(REG_ITMP2_XPC, REG_SP, 0 * 8);
			M_AADD_IMM(2 * 8, REG_SP);

			M_MOV_IMM((ptrint) asm_handle_exception, REG_ITMP3);
			M_JMP(REG_ITMP3);
		}
	}

	/* generate code patching stub call code */

	{
		patchref    *pref;
		codegendata *tmpcd;
		ptrint       mcode;

		tmpcd = DNEW(codegendata);

		for (pref = cd->patchrefs; pref != NULL; pref = pref->next) {
			/* check size of code segment */

			MCODECHECK(512);

			/* Get machine code which is patched back in later. A             */
			/* `call rel32' is 5 bytes long (but read 8 bytes).               */

			xcodeptr = cd->mcodebase + pref->branchpos;
			mcode = *((ptrint *) xcodeptr);

			/* patch in `call rel32' to call the following code               */

			tmpcd->mcodeptr = xcodeptr;     /* set dummy mcode pointer        */
			x86_64_call_imm(tmpcd, cd->mcodeptr - (xcodeptr + 5));

			/* move pointer to java_objectheader onto stack */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			/* create a virtual java_objectheader */

			(void) dseg_addaddress(cd, get_dummyLR());          /* monitorPtr */
			a = dseg_addaddress(cd, NULL);                      /* vftbl      */

  			x86_64_lea_membase_reg(cd, RIP, -(((ptrint) cd->mcodeptr + 7) - (ptrint) cd->mcodebase) + a, REG_ITMP3);
			M_PUSH(REG_ITMP3);
#else
			M_PUSH_IMM(0);
#endif

			/* move machine code bytes and classinfo pointer into registers */

			M_MOV_IMM((ptrint) mcode, REG_ITMP3);
			M_PUSH(REG_ITMP3);
			M_MOV_IMM((ptrint) pref->ref, REG_ITMP3);
			M_PUSH(REG_ITMP3);
			M_MOV_IMM((ptrint) pref->disp, REG_ITMP3);
			M_PUSH(REG_ITMP3);

			M_MOV_IMM((ptrint) pref->patcher, REG_ITMP3);
			M_PUSH(REG_ITMP3);

			M_MOV_IMM((ptrint) asm_wrapper_patcher, REG_ITMP3);
			M_JMP(REG_ITMP3);
		}
	}
	}

	codegen_finish(m, cd, (s4) ((u1 *) cd->mcodeptr - cd->mcodebase));

	/* everything's ok */

	return true;
}


/* createcompilerstub **********************************************************

   Creates a stub routine which calls the compiler.
	
*******************************************************************************/

#define COMPILERSTUB_DATASIZE    2 * SIZEOF_VOID_P
#define COMPILERSTUB_CODESIZE    7 + 7 + 3

#define COMPILERSTUB_SIZE        COMPILERSTUB_DATASIZE + COMPILERSTUB_CODESIZE


u1 *createcompilerstub(methodinfo *m)
{
	u1          *s;                     /* memory to hold the stub            */
	ptrint      *d;
	codegendata *cd;
	s4           dumpsize;

	s = CNEW(u1, COMPILERSTUB_SIZE);

	/* set data pointer and code pointer */

	d = (ptrint *) s;
	s = s + COMPILERSTUB_DATASIZE;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	cd = DNEW(codegendata);
	cd->mcodeptr = s;

	/* Store the methodinfo* in the same place as in the methodheader
	   for compiled methods. */

	d[0] = (ptrint) asm_call_jit_compiler;
	d[1] = (ptrint) m;

	/* code for the stub */

	M_ALD(REG_ITMP1, RIP, -(7 * 1 + 1 * SIZEOF_VOID_P));       /* methodinfo  */
	M_ALD(REG_ITMP3, RIP, -(7 * 2 + 2 * SIZEOF_VOID_P));  /* compiler pointer */
	M_JMP(REG_ITMP3);

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		count_cstub_len += COMPILERSTUB_SIZE;
#endif

	/* release dump area */

	dump_release(dumpsize);

	return s;
}


/* createnativestub ************************************************************

   Creates a stub routine which calls a native method.

*******************************************************************************/

u1 *createnativestub(functionptr f, methodinfo *m, codegendata *cd,
					 registerdata *rd, methoddesc *nmd)
{
	methoddesc *md;
	s4          stackframesize;         /* size of stackframe if needed       */
	s4          nativeparams;
	s4          i, j;                   /* count variables                    */
	s4          t;
	s4          s1, s2;

	/* initialize variables */

	md = m->parseddesc;
	nativeparams = (m->flags & ACC_STATIC) ? 2 : 1;

	/* calculate stack frame size */

	stackframesize =
		sizeof(stackframeinfo) / SIZEOF_VOID_P +
		sizeof(localref_table) / SIZEOF_VOID_P +
		INT_ARG_CNT + FLT_ARG_CNT + 1 +         /* + 1 for function address   */
		nmd->memuse;

	if (!(stackframesize & 0x1))                /* keep stack 16-byte aligned */
		stackframesize++;

	/* create method header */

	(void) dseg_addaddress(cd, m);                          /* MethodPointer  */
	(void) dseg_adds4(cd, stackframesize * 8);              /* FrameSize      */
	(void) dseg_adds4(cd, 0);                               /* IsSync         */
	(void) dseg_adds4(cd, 0);                               /* IsLeaf         */
	(void) dseg_adds4(cd, 0);                               /* IntSave        */
	(void) dseg_adds4(cd, 0);                               /* FltSave        */
	(void) dseg_addlinenumbertablesize(cd);
	(void) dseg_adds4(cd, 0);                               /* ExTableSize    */

	/* initialize mcode variables */
	
	cd->mcodeptr = (u1 *) cd->mcodebase;
	cd->mcodeend = (s4 *) (cd->mcodebase + cd->mcodesize);

	/* generate native method profiling code */

	if (opt_prof) {
		/* count frequency */

		M_MOV_IMM((ptrint) m, REG_ITMP2);
		M_IINC_MEMBASE(REG_ITMP2, OFFSET(methodinfo, frequency));
	}

	/* generate stub code */

	M_ASUB_IMM(stackframesize * 8, REG_SP);

	if (runverbose) {
		/* save integer and float argument registers */

		for (i = 0, j = 0; i < md->paramcount && j < INT_ARG_CNT; i++)
			if (IS_INT_LNG_TYPE(md->paramtypes[i].type))
				M_LST(rd->argintregs[j++], REG_SP, (1 + i) * 8);

		for (i = 0, j = 0; i < md->paramcount && j < FLT_ARG_CNT; i++)
			if (IS_FLT_DBL_TYPE(md->paramtypes[i].type))
				M_DST(rd->argfltregs[j++], REG_SP, (1 + INT_ARG_CNT + i) * 8);

		/* show integer hex code for float arguments */

		for (i = 0, j = 0; i < md->paramcount && j < INT_ARG_CNT; i++) {
			/* if the paramtype is a float, we have to right shift all
			   following integer registers */

			if (IS_FLT_DBL_TYPE(md->paramtypes[i].type)) {
				for (s1 = INT_ARG_CNT - 2; s1 >= i; s1--)
					M_MOV(rd->argintregs[s1], rd->argintregs[s1 + 1]);

				x86_64_movd_freg_reg(cd, rd->argfltregs[j], rd->argintregs[i]);
				j++;
			}
		}

		M_MOV_IMM((ptrint) m, REG_ITMP1);
		M_AST(REG_ITMP1, REG_SP, 0 * 8);
		M_MOV_IMM((ptrint) builtin_trace_args, REG_ITMP1);
		M_CALL(REG_ITMP1);

		/* restore integer and float argument registers */

		for (i = 0, j = 0; i < md->paramcount && j < INT_ARG_CNT; i++)
			if (IS_INT_LNG_TYPE(md->paramtypes[i].type))
				M_LLD(rd->argintregs[j++], REG_SP, (1 + i) * 8);

		for (i = 0, j = 0; i < md->paramcount && j < FLT_ARG_CNT; i++)
			if (IS_FLT_DBL_TYPE(md->paramtypes[i].type))
				M_DLD(rd->argfltregs[j++], REG_SP, (1 + INT_ARG_CNT + i) * 8);
	}


	/* get function address (this must happen before the stackframeinfo) */

#if !defined(ENABLE_STATICVM)
	if (f == NULL) {
		codegen_addpatchref(cd, cd->mcodeptr, PATCHER_resolve_native, m, 0);

		if (opt_showdisassemble) {
			M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
		}
	}
#endif

	M_MOV_IMM((ptrint) f, REG_ITMP3);


	/* save integer and float argument registers */

	for (i = 0, j = 0; i < md->paramcount && j < INT_ARG_CNT; i++)
		if (IS_INT_LNG_TYPE(md->paramtypes[i].type))
			M_LST(rd->argintregs[j++], REG_SP, i * 8);

	for (i = 0, j = 0; i < md->paramcount && j < FLT_ARG_CNT; i++)
		if (IS_FLT_DBL_TYPE(md->paramtypes[i].type))
			M_DST(rd->argfltregs[j++], REG_SP, (INT_ARG_CNT + i) * 8);

	M_AST(REG_ITMP3, REG_SP, (INT_ARG_CNT + FLT_ARG_CNT) * 8);

	/* create dynamic stack info */

	M_ALEA(REG_SP, stackframesize * 8, rd->argintregs[0]);
	x86_64_lea_membase_reg(cd, RIP, -(((ptrint) cd->mcodeptr + 7) - (ptrint) cd->mcodebase), rd->argintregs[1]);
	M_ALEA(REG_SP, stackframesize * 8 + SIZEOF_VOID_P, rd->argintregs[2]);
	M_ALD(rd->argintregs[3], REG_SP, stackframesize * 8);
	M_MOV_IMM((ptrint) codegen_start_native_call, REG_ITMP1);
	M_CALL(REG_ITMP1);

	/* restore integer and float argument registers */

	for (i = 0, j = 0; i < md->paramcount && j < INT_ARG_CNT; i++)
		if (IS_INT_LNG_TYPE(md->paramtypes[i].type))
			M_LLD(rd->argintregs[j++], REG_SP, i * 8);

	for (i = 0, j = 0; i < md->paramcount && j < FLT_ARG_CNT; i++)
		if (IS_FLT_DBL_TYPE(md->paramtypes[i].type))
			M_DLD(rd->argfltregs[j++], REG_SP, (INT_ARG_CNT + i) * 8);

	M_ALD(REG_ITMP3, REG_SP, (INT_ARG_CNT + FLT_ARG_CNT) * 8);


	/* copy or spill arguments to new locations */

	for (i = md->paramcount - 1, j = i + nativeparams; i >= 0; i--, j--) {
		t = md->paramtypes[i].type;

		if (IS_INT_LNG_TYPE(t)) {
			if (!md->params[i].inmemory) {
				s1 = rd->argintregs[md->params[i].regoff];

				if (!nmd->params[j].inmemory) {
					s2 = rd->argintregs[nmd->params[j].regoff];
					M_INTMOVE(s1, s2);

				} else {
					s2 = nmd->params[j].regoff;
					M_LST(s1, REG_SP, s2 * 8);
				}

			} else {
				s1 = md->params[i].regoff + stackframesize + 1;   /* + 1 (RA) */
				s2 = nmd->params[j].regoff;
				M_LLD(REG_ITMP1, REG_SP, s1 * 8);
				M_LST(REG_ITMP1, REG_SP, s2 * 8);
			}

		} else {
			/* We only copy spilled float arguments, as the float argument    */
			/* registers keep unchanged.                                      */

			if (md->params[i].inmemory) {
				s1 = md->params[i].regoff + stackframesize + 1;   /* + 1 (RA) */
				s2 = nmd->params[j].regoff;
				M_DLD(REG_FTMP1, REG_SP, s1 * 8);
				M_DST(REG_FTMP1, REG_SP, s2 * 8);
			}
		}
	}

	/* put class into second argument register */

	if (m->flags & ACC_STATIC)
		M_MOV_IMM((ptrint) m->class, rd->argintregs[1]);

	/* put env into first argument register */

	M_MOV_IMM((ptrint) &env, rd->argintregs[0]);

	/* do the native function call */

	M_CALL(REG_ITMP3);

	/* save return value */

	if (md->returntype.type != TYPE_VOID) {
		if (IS_INT_LNG_TYPE(md->returntype.type))
			M_LST(REG_RESULT, REG_SP, 0 * 8);
		else
			M_DST(REG_FRESULT, REG_SP, 0 * 8);
	}

	/* remove native stackframe info */

	M_ALEA(REG_SP, stackframesize * 8, rd->argintregs[0]);
	M_MOV_IMM((ptrint) codegen_finish_native_call, REG_ITMP1);
	M_CALL(REG_ITMP1);

	/* generate call trace */

	if (runverbose) {
		/* just restore the value we need, don't care about the other */

		if (md->returntype.type != TYPE_VOID) {
			if (IS_INT_LNG_TYPE(md->returntype.type))
				M_LLD(REG_RESULT, REG_SP, 0 * 8);
			else
				M_DLD(REG_FRESULT, REG_SP, 0 * 8);
		}

  		M_MOV_IMM((ptrint) m, rd->argintregs[0]);
  		M_MOV(REG_RESULT, rd->argintregs[1]);
		M_FLTMOVE(REG_FRESULT, rd->argfltregs[0]);
  		M_FLTMOVE(REG_FRESULT, rd->argfltregs[1]);

		M_MOV_IMM((ptrint) builtin_displaymethodstop, REG_ITMP1);
		M_CALL(REG_ITMP1);
	}

	/* check for exception */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	M_MOV_IMM((ptrint) builtin_get_exceptionptrptr, REG_ITMP3);
	M_CALL(REG_ITMP3);
#else
	M_MOV_IMM((ptrint) &_no_threads_exceptionptr, REG_RESULT);
#endif
	M_ALD(REG_ITMP2, REG_RESULT, 0);

	/* restore return value */

	if (md->returntype.type != TYPE_VOID) {
		if (IS_INT_LNG_TYPE(md->returntype.type))
			M_LLD(REG_RESULT, REG_SP, 0 * 8);
		else
			M_DLD(REG_FRESULT, REG_SP, 0 * 8);
	}

	/* test for exception */

	M_TEST(REG_ITMP2);
	M_BNE(7 + 1);

	/* remove stackframe */

	M_AADD_IMM(stackframesize * 8, REG_SP);
	M_RET;


	/* handle exception */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	M_LST(REG_ITMP2, REG_SP, 0 * 8);
	M_MOV_IMM((ptrint) builtin_get_exceptionptrptr, REG_ITMP3);
	M_CALL(REG_ITMP3);
	M_AST_IMM32(0, REG_RESULT, 0);                 /* clear exception pointer */
	M_LLD(REG_ITMP1_XPTR, REG_SP, 0 * 8);
#else
	M_MOV(REG_ITMP3, REG_ITMP1_XPTR);
	M_MOV_IMM((ptrint) &_no_threads_exceptionptr, REG_ITMP3);
	M_AST_IMM32(0, REG_ITMP3, 0);                  /* clear exception pointer */
#endif

	/* remove stackframe */

	M_AADD_IMM(stackframesize * 8, REG_SP);

	M_LLD(REG_ITMP2_XPC, REG_SP, 0 * 8);     /* get return address from stack */
	M_ASUB_IMM(3, REG_ITMP2_XPC);                                    /* callq */

	M_MOV_IMM((ptrint) asm_handle_nat_exception, REG_ITMP3);
	M_JMP(REG_ITMP3);


	/* process patcher calls **************************************************/

	{
		u1          *xcodeptr;
		patchref    *pref;
		codegendata *tmpcd;
		ptrint       mcode;
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
		s4           disp;
#endif

		tmpcd = DNEW(codegendata);

		for (pref = cd->patchrefs; pref != NULL; pref = pref->next) {
			/* Get machine code which is patched back in later. A             */
			/* `call rel32' is 5 bytes long (but read 8 bytes).               */

			xcodeptr = cd->mcodebase + pref->branchpos;
			mcode = *((ptrint *) xcodeptr);

			/* patch in `call rel32' to call the following code               */

			tmpcd->mcodeptr = xcodeptr;     /* set dummy mcode pointer        */
			x86_64_call_imm(tmpcd, cd->mcodeptr - (xcodeptr + 5));

			/* move pointer to java_objectheader onto stack */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			/* create a virtual java_objectheader */

			(void) dseg_addaddress(cd, get_dummyLR());          /* monitorPtr */
			disp = dseg_addaddress(cd, NULL);                   /* vftbl      */

  			x86_64_lea_membase_reg(cd, RIP, -(((ptrint) cd->mcodeptr + 7) - (ptrint) cd->mcodebase) + disp, REG_ITMP3);
			M_PUSH(REG_ITMP3);
#else
			M_PUSH_IMM(0);
#endif

			/* move machine code bytes and classinfo pointer into registers */

			M_MOV_IMM((ptrint) mcode, REG_ITMP3);
			M_PUSH(REG_ITMP3);
			M_MOV_IMM((ptrint) pref->ref, REG_ITMP3);
			M_PUSH(REG_ITMP3);
			M_MOV_IMM((ptrint) pref->disp, REG_ITMP3);
			M_PUSH(REG_ITMP3);

			M_MOV_IMM((ptrint) pref->patcher, REG_ITMP3);
			M_PUSH(REG_ITMP3);

			M_MOV_IMM((ptrint) asm_wrapper_patcher, REG_ITMP3);
			M_JMP(REG_ITMP3);
		}
	}

	codegen_finish(m, cd, (s4) ((u1 *) cd->mcodeptr - cd->mcodebase));

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
