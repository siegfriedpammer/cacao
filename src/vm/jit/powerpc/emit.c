/* src/vm/jit/powerpc/emit.c - PowerPC code emitter functions

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

   Authors: Christian Thalinger

   Changes:

   $Id: emitfuncs.c 4398 2006-01-31 23:43:08Z twisti $

*/


#include "config.h"

#include <assert.h>

#include "vm/types.h"

#include "md-abi.h"

#include "vm/jit/powerpc/codegen.h"

#include "vm/builtin.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/dseg.h"
#include "vm/jit/emit-common.h"
#include "vm/jit/jit.h"
#include "vm/jit/replace.h"


/* emit_load *******************************************************************

   Emits a possible load of an operand.

*******************************************************************************/

s4 emit_load(jitdata *jd, instruction *iptr, varinfo *src, s4 tempreg)
{
	codegendata *cd;
	s4           disp;
	s4           reg;

	/* get required compiler data */

	cd = jd->cd;

	if (IS_INMEMORY(src->flags)) {
		COUNT_SPILLS;

		disp = src->vv.regoff * 4;

		if (IS_FLT_DBL_TYPE(src->type)) {
			if (IS_2_WORD_TYPE(src->type))
				M_DLD(tempreg, REG_SP, disp);
			else
				M_FLD(tempreg, REG_SP, disp);
		}
		else {
			if (IS_2_WORD_TYPE(src->type))
				M_LLD(tempreg, REG_SP, disp);
			else
				M_ILD(tempreg, REG_SP, disp);
		}

		reg = tempreg;
	}
	else
		reg = src->vv.regoff;

	return reg;
}


/* emit_load_low ***************************************************************

   Emits a possible load of the low 32-bits of an operand.

*******************************************************************************/

s4 emit_load_low(jitdata *jd, instruction *iptr, varinfo *src, s4 tempreg)
{
	codegendata  *cd;
	s4            disp;
	s4            reg;

	assert(src->type == TYPE_LNG);

	/* get required compiler data */

	cd = jd->cd;

	if (IS_INMEMORY(src->flags)) {
		COUNT_SPILLS;

		disp = src->vv.regoff * 4;

		M_ILD(tempreg, REG_SP, disp + 4);

		reg = tempreg;
	}
	else
		reg = GET_LOW_REG(src->vv.regoff);

	return reg;
}


/* emit_load_high **************************************************************

   Emits a possible load of the high 32-bits of an operand.

*******************************************************************************/

s4 emit_load_high(jitdata *jd, instruction *iptr, varinfo *src, s4 tempreg)
{
	codegendata  *cd;
	s4            disp;
	s4            reg;

	assert(src->type == TYPE_LNG);

	/* get required compiler data */

	cd = jd->cd;

	if (IS_INMEMORY(src->flags)) {
		COUNT_SPILLS;

		disp = src->vv.regoff * 4;

		M_ILD(tempreg, REG_SP, disp);

		reg = tempreg;
	}
	else
		reg = GET_HIGH_REG(src->vv.regoff);

	return reg;
}


/* emit_store ******************************************************************

   XXX

*******************************************************************************/

void emit_store(jitdata *jd, instruction *iptr, varinfo *dst, s4 d)
{
	codegendata *cd;

	/* get required compiler data */

	cd = jd->cd;

	if (IS_INMEMORY(dst->flags)) {
		COUNT_SPILLS;

		if (IS_FLT_DBL_TYPE(dst->type)) {
			if (IS_2_WORD_TYPE(dst->type))
				M_DST(d, REG_SP, dst->vv.regoff * 4);
			else
				M_FST(d, REG_SP, dst->vv.regoff * 4);
		}
		else {
			if (IS_2_WORD_TYPE(dst->type))
				M_LST(d, REG_SP, dst->vv.regoff * 4);
			else
				M_IST(d, REG_SP, dst->vv.regoff * 4);
		}
	}
}


/* emit_copy *******************************************************************

   Generates a register/memory to register/memory copy.

*******************************************************************************/

void emit_copy(jitdata *jd, instruction *iptr, varinfo *src, varinfo *dst)
{
	codegendata  *cd;
	s4            s1, d;

	/* get required compiler data */

	cd = jd->cd;

	if ((src->vv.regoff != dst->vv.regoff) ||
		(IS_INMEMORY(src->flags ^ dst->flags))) {

		/* If one of the variables resides in memory, we can eliminate
		   the register move from/to the temporary register with the
		   order of getting the destination register and the load. */

		if (IS_INMEMORY(src->flags)) {
			if (IS_LNG_TYPE(src->type))
				d = codegen_reg_of_var(iptr->opc, dst, REG_ITMP12_PACKED);
			else
				d = codegen_reg_of_var(iptr->opc, dst, REG_IFTMP);

			s1 = emit_load(jd, iptr, src, d);
		}
		else {
			if (IS_LNG_TYPE(src->type))
				s1 = emit_load(jd, iptr, src, REG_ITMP12_PACKED);
			else
				s1 = emit_load(jd, iptr, src, REG_IFTMP);

			d = codegen_reg_of_var(iptr->opc, dst, s1);
		}

		if (s1 != d) {
			if (IS_FLT_DBL_TYPE(src->type))
				M_FMOV(s1, d);
			else {
				if (IS_2_WORD_TYPE(src->type)) {
					M_MOV(GET_LOW_REG(s1), GET_LOW_REG(d));
					M_MOV(GET_HIGH_REG(s1), GET_HIGH_REG(d));
                }
				else
                    M_MOV(s1, d);
			}
		}

		emit_store(jd, iptr, dst, d);
	}
}


/* emit_iconst *****************************************************************

   XXX

*******************************************************************************/

void emit_iconst(codegendata *cd, s4 d, s4 value)
{
	s4 disp;

	if ((value >= -32768) && (value <= 32767))
		M_LDA_INTERN(d, REG_ZERO, value);
	else {
		disp = dseg_add_s4(cd, value);
		M_ILD(d, REG_PV, disp);
	}
}


/* emit_exception_stubs ********************************************************

   Generates the code for the exception stubs.

*******************************************************************************/

void emit_exception_stubs(jitdata *jd)
{
	codegendata  *cd;
	registerdata *rd;
	exceptionref *eref;
	s4            targetdisp;
	s4            disp;

	/* get required compiler data */

	cd = jd->cd;
	rd = jd->rd;

	/* generate exception stubs */

	targetdisp = 0;

	for (eref = cd->exceptionrefs; eref != NULL; eref = eref->next) {
		gen_resolvebranch(cd->mcodebase + eref->branchpos, 
						  eref->branchpos, cd->mcodeptr - cd->mcodebase);

		MCODECHECK(100);

		/* Move the value register to a temporary register, if
		   there is the need for it. */

		if (eref->reg != -1)
			M_MOV(eref->reg, REG_ITMP1);

		/* calcuate exception address */

		M_LDA(REG_ITMP2_XPC, REG_PV, eref->branchpos - 4);

		/* move function to call into REG_ITMP3 */

		disp = dseg_add_functionptr(cd, eref->function);
		M_ALD(REG_ITMP3, REG_PV, disp);

		if (targetdisp == 0) {
		    targetdisp = ((u4 *) cd->mcodeptr) - ((u4 *) cd->mcodebase);

			if (jd->isleafmethod) {
				M_MFLR(REG_ZERO);
				M_AST(REG_ZERO, REG_SP, cd->stackframesize * 4 + LA_LR_OFFSET);
			}

			M_MOV(REG_PV, rd->argintregs[0]);
			M_MOV(REG_SP, rd->argintregs[1]);

			if (jd->isleafmethod)
				M_MOV(REG_ZERO, rd->argintregs[2]);
			else
				M_ALD(rd->argintregs[2],
					  REG_SP, cd->stackframesize * 4 + LA_LR_OFFSET);

			M_MOV(REG_ITMP2_XPC, rd->argintregs[3]);
			M_MOV(REG_ITMP1, rd->argintregs[4]);

			M_STWU(REG_SP, REG_SP, -(LA_SIZE + 6 * 4));
			M_AST(REG_ITMP2_XPC, REG_SP, LA_SIZE + 5 * 4);

			M_MTCTR(REG_ITMP3);
			M_JSR;
			M_MOV(REG_RESULT, REG_ITMP1_XPTR);

			M_ALD(REG_ITMP2_XPC, REG_SP, LA_SIZE + 5 * 4);
			M_IADD_IMM(REG_SP, LA_SIZE + 6 * 4, REG_SP);

			if (jd->isleafmethod) {
				/* XXX FIXME: REG_ZERO can cause problems here! */
				assert(cd->stackframesize * 4 <= 32767);

				M_ALD(REG_ZERO, REG_SP, cd->stackframesize * 4 + LA_LR_OFFSET);
				M_MTLR(REG_ZERO);
			}

			disp = dseg_add_functionptr(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_MTCTR(REG_ITMP3);
			M_RTS;
		}
		else {
			disp = (((u4 *) cd->mcodebase) + targetdisp) -
				(((u4 *) cd->mcodeptr) + 1);
			M_BR(disp);
		}
	}
}


/* emit_patcher_stubs **********************************************************

   Generates the code for the patcher stubs.

*******************************************************************************/

void emit_patcher_stubs(jitdata *jd)
{
	codegendata *cd;
	patchref    *pref;
	u4           mcode;
	u1          *savedmcodeptr;
	u1          *tmpmcodeptr;
	s4           targetdisp;
	s4           disp;

	/* get required compiler data */

	cd = jd->cd;

	/* generate code patching stub call code */

	targetdisp = 0;

	for (pref = cd->patchrefs; pref != NULL; pref = pref->next) {
		/* check code segment size */

		MCODECHECK(100);

		/* Get machine code which is patched back in later. The
		   call is 1 instruction word long. */

		tmpmcodeptr = (u1 *) (cd->mcodebase + pref->branchpos);

		mcode = *((u4 *) tmpmcodeptr);

		/* Patch in the call to call the following code (done at
		   compile time). */

		savedmcodeptr = cd->mcodeptr;   /* save current mcodeptr          */
		cd->mcodeptr  = tmpmcodeptr;    /* set mcodeptr to patch position */

		disp = ((u4 *) savedmcodeptr) - (((u4 *) tmpmcodeptr) + 1);
		M_BR(disp);

		cd->mcodeptr = savedmcodeptr;   /* restore the current mcodeptr   */

		/* create stack frame - keep stack 16-byte aligned */

		M_AADD_IMM(REG_SP, -8 * 4, REG_SP);

		/* calculate return address and move it onto the stack */

		M_LDA(REG_ITMP3, REG_PV, pref->branchpos);
		M_AST_INTERN(REG_ITMP3, REG_SP, 5 * 4);

		/* move pointer to java_objectheader onto stack */

#if defined(ENABLE_THREADS)
		/* order reversed because of data segment layout */

		(void) dseg_add_unique_address(cd, NULL);                  /* flcword */
		(void) dseg_add_unique_address(cd, lock_get_initial_lock_word());
		disp = dseg_add_unique_address(cd, NULL);                  /* vftbl   */

		M_LDA(REG_ITMP3, REG_PV, disp);
		M_AST_INTERN(REG_ITMP3, REG_SP, 4 * 4);
#else
		/* do nothing */
#endif

		/* move machine code onto stack */

		disp = dseg_add_s4(cd, mcode);
		M_ILD(REG_ITMP3, REG_PV, disp);
		M_IST_INTERN(REG_ITMP3, REG_SP, 3 * 4);

		/* move class/method/field reference onto stack */

		disp = dseg_add_address(cd, pref->ref);
		M_ALD(REG_ITMP3, REG_PV, disp);
		M_AST_INTERN(REG_ITMP3, REG_SP, 2 * 4);

		/* move data segment displacement onto stack */

		disp = dseg_add_s4(cd, pref->disp);
		M_ILD(REG_ITMP3, REG_PV, disp);
		M_IST_INTERN(REG_ITMP3, REG_SP, 1 * 4);

		/* move patcher function pointer onto stack */

		disp = dseg_add_functionptr(cd, pref->patcher);
		M_ALD(REG_ITMP3, REG_PV, disp);
		M_AST_INTERN(REG_ITMP3, REG_SP, 0 * 4);

		if (targetdisp == 0) {
			targetdisp = ((u4 *) cd->mcodeptr) - ((u4 *) cd->mcodebase);

			disp = dseg_add_functionptr(cd, asm_patcher_wrapper);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_MTCTR(REG_ITMP3);
			M_RTS;
		}
		else {
			disp = (((u4 *) cd->mcodebase) + targetdisp) -
				(((u4 *) cd->mcodeptr) + 1);
			M_BR(disp);
		}
	}
}


/* emit_replacement_stubs ******************************************************

   Generates the code for the replacement stubs.

*******************************************************************************/

void emit_replacement_stubs(jitdata *jd)
{
	codegendata *cd;
	codeinfo    *code;
	rplpoint    *rplp;
	u1          *savedmcodeptr;
	s4           disp;
	s4           i;

	/* get required compiler data */

	cd   = jd->cd;
	code = jd->code;

	rplp = code->rplpoints;

	for (i = 0; i < code->rplpointcount; ++i, ++rplp) {
		/* check code segment size */

		MCODECHECK(100);

		/* note start of stub code */

		rplp->outcode = (u1 *) (cd->mcodeptr - cd->mcodebase);

		/* make machine code for patching */

		savedmcodeptr = cd->mcodeptr;
		cd->mcodeptr  = (u1 *) &(rplp->mcode) + 1;              /* big-endian */

		disp = (ptrint) ((s4 *) rplp->outcode - (s4 *) rplp->pc) - 1;
		M_BR(disp);

		cd->mcodeptr = savedmcodeptr;

		/* create stack frame - keep 16-byte aligned */

		M_AADD_IMM(REG_SP, -4 * 4, REG_SP);

		/* push address of `rplpoint` struct */

		disp = dseg_add_address(cd, rplp);
		M_ALD(REG_ITMP3, REG_PV, disp);
		M_AST_INTERN(REG_ITMP3, REG_SP, 0 * 4);

		/* jump to replacement function */

		disp = dseg_add_functionptr(cd, asm_replacement_out);
		M_ALD(REG_ITMP3, REG_PV, disp);
		M_MTCTR(REG_ITMP3);
		M_RTS;
	}
}


/* emit_verbosecall_enter ******************************************************

   Generates the code for the call trace.

*******************************************************************************/

void emit_verbosecall_enter(jitdata *jd)
{
	methodinfo   *m;
	codegendata  *cd;
	registerdata *rd;
	s4 s1, p, t, d;
	int stack_off;
	int stack_size;
	methoddesc *md;

	/* get required compiler data */

	m  = jd->m;
	cd = jd->cd;
	rd = jd->rd;

	md = m->parseddesc;
	
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

	/* mark trace code */

	M_NOP;

	M_MFLR(REG_ZERO);
	M_AST(REG_ZERO, REG_SP, LA_LR_OFFSET);
	M_STWU(REG_SP, REG_SP, -stack_size);

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
				s1 = (md->params[p].regoff + cd->stackframesize) * 4 
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
	p = dseg_add_address(cd, m);
	M_ALD(REG_ITMP1, REG_PV, p);
#if defined(__DARWIN__)
	M_AST(REG_ITMP1, REG_SP, LA_SIZE + TRACE_ARGS_NUM * 8); 
#else
	M_AST(REG_ITMP1, REG_SP, LA_SIZE + 4 * 8);
#endif
	p = dseg_add_functionptr(cd, builtin_trace_args);
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
					M_ILD(rd->argintregs[GET_HIGH_REG(md->params[p].vv.regoff)]
						  , REG_SP, stack_off);
					M_ILD(rd->argintregs[GET_LOW_REG(md->params[p].vv.regoff)]
						  , REG_SP, stack_off + 4);
				} else {
					M_ILD(rd->argintregs[md->params[p].vv.regoff]
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

	M_ALD(REG_ZERO, REG_SP, stack_size + LA_LR_OFFSET);
	M_MTLR(REG_ZERO);
	M_LDA(REG_SP, REG_SP, stack_size);

	/* mark trace code */

	M_NOP;
}


/* emit_verbosecall_exit *******************************************************

   Generates the code for the call trace.

*******************************************************************************/

void emit_verbosecall_exit(jitdata *jd)
{
	methodinfo   *m;
	codegendata  *cd;
	registerdata *rd;
	methoddesc   *md;
	s4            disp;

	/* get required compiler data */

	m  = jd->m;
	cd = jd->cd;
	rd = jd->rd;

	md = m->parseddesc;
	
	/* mark trace code */

	M_NOP;

	M_MFLR(REG_ZERO);
	M_AST(REG_ZERO, REG_SP, LA_LR_OFFSET);
	M_STWU(REG_SP, REG_SP, -(LA_SIZE + (1 + 2 + 2 + 1 + 4) * 4));

	/* save return registers */

	M_LST(REG_RESULT_PACKED, REG_SP, LA_SIZE + (1 + 2 + 2 + 1 + 0) * 4);
	M_DST(REG_FRESULT, REG_SP, LA_SIZE + (1 + 2 + 2 + 1 + 2) * 4);

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

	disp = dseg_add_address(cd, m);
	M_ALD(rd->argintregs[0], REG_PV, disp);

	disp = dseg_add_functionptr(cd, builtin_displaymethodstop);
	M_ALD(REG_ITMP2, REG_PV, disp);
	M_MTCTR(REG_ITMP2);
	M_JSR;

	/* restore return registers */

	M_LLD(REG_RESULT_PACKED, REG_SP, LA_SIZE + (1 + 2 + 2 + 1 + 0) * 4);
	M_DLD(REG_FRESULT, REG_SP, LA_SIZE + (1 + 2 + 2 + 1 + 2) * 4);

	M_ALD(REG_ZERO, REG_SP, LA_SIZE + (1 + 2 + 2 + 1 + 4) * 4 + LA_LR_OFFSET);
	M_MTLR(REG_ZERO);
	M_LDA(REG_SP, REG_SP, LA_SIZE + (1 + 2 + 2 + 1 + 4) * 4);

	/* mark trace code */

	M_NOP;
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
