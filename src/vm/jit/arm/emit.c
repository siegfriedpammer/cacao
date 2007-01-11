/* src/vm/jit/arm/emit.c - Arm code emitter functions

   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
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

   $Id: emit.c 4398 2006-01-31 23:43:08Z twisti $

*/


#include "config.h"

#include <assert.h>

#include "vm/types.h"

#include "md-abi.h"

#include "vm/jit/arm/codegen.h"

#if defined(ENABLE_THREADS)
# include "threads/native/lock.h"
#endif

#include "vm/builtin.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/emit-common.h"
#include "vm/jit/jit.h"
#include "vm/jit/replace.h"

#include "toolbox/logging.h" /* XXX for debugging only */


/* emit_load *******************************************************************

   Emits a possible load of an operand.

*******************************************************************************/

s4 emit_load(jitdata *jd, instruction *iptr, varinfo *src, s4 tempreg)
{
	codegendata  *cd;
	s4            disp;
	s4            reg;

	/* get required compiler data */

	cd = jd->cd;

	if (src->flags & INMEMORY) {
		COUNT_SPILLS;

		disp = src->vv.regoff * 4;

		if (IS_FLT_DBL_TYPE(src->type)) {
#if !defined(ENABLE_SOFTFLOAT)
			if (IS_2_WORD_TYPE(src->type))
				M_DLD(tempreg, REG_SP, disp);
			else
				M_FLD(tempreg, REG_SP, disp);
#else
			assert(0);
#endif
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

   Emits a possible load of the low 32-bits of a long source operand.

*******************************************************************************/

s4 emit_load_low(jitdata *jd, instruction *iptr, varinfo *src, s4 tempreg)
{
	codegendata  *cd;
	s4            disp;
	s4            reg;

	assert(src->type == TYPE_LNG);

	/* get required compiler data */

	cd = jd->cd;

	if (src->flags & INMEMORY) {
		COUNT_SPILLS;

		disp = src->vv.regoff * 4;

#if defined(__ARMEL__)
		M_ILD(tempreg, REG_SP, disp);
#else
		M_ILD(tempreg, REG_SP, disp + 4);
#endif

		reg = tempreg;
	}
	else
		reg = GET_LOW_REG(src->vv.regoff);

	return reg;
}


/* emit_load_high **************************************************************

   Emits a possible load of the high 32-bits of a long source operand.

*******************************************************************************/

s4 emit_load_high(jitdata *jd, instruction *iptr, varinfo *src, s4 tempreg)
{
	codegendata  *cd;
	s4            disp;
	s4            reg;

	assert(src->type == TYPE_LNG);

	/* get required compiler data */

	cd = jd->cd;

	if (src->flags & INMEMORY) {
		COUNT_SPILLS;

		disp = src->vv.regoff * 4;

#if defined(__ARMEL__)
		M_ILD(tempreg, REG_SP, disp + 4);
#else
		M_ILD(tempreg, REG_SP, disp);
#endif

		reg = tempreg;
	}
	else
		reg = GET_HIGH_REG(src->vv.regoff);

	return reg;
}


/* emit_store ******************************************************************

   Emits a possible store to a variable.

*******************************************************************************/

void emit_store(jitdata *jd, instruction *iptr, varinfo *dst, s4 d)
{
	codegendata  *cd;
	s4            disp;

	/* get required compiler data */

	cd = jd->cd;

	if (dst->flags & INMEMORY) {
		COUNT_SPILLS;

		disp = dst->vv.regoff * 4;

#if !defined(ENABLE_SOFTFLOAT)
		if (IS_FLT_DBL_TYPE(dst->type)) {
			if (IS_2_WORD_TYPE(dst->type))
				M_DST(d, REG_SP, disp);
			else
				M_FST(d, REG_SP, disp);
		}
		else {
			if (IS_2_WORD_TYPE(dst->type))
				M_LST(d, REG_SP, disp);
			else
				M_IST(d, REG_SP, disp);
		}
#else
		if (IS_2_WORD_TYPE(dst->type))
			M_LST(d, REG_SP, disp);
		else
			M_IST(d, REG_SP, disp);
#endif
	}
	else if (IS_LNG_TYPE(dst->type)) {
#if defined(__ARMEL__)
		if (GET_HIGH_REG(dst->vv.regoff) == REG_SPLIT)
			M_IST_INTERN(GET_HIGH_REG(d), REG_SP, 0 * 4);
#else
		if (GET_LOW_REG(dst->vv.regoff) == REG_SPLIT)
			M_IST_INTERN(GET_LOW_REG(d), REG_SP, 0 * 4);
#endif
	}
}


/* emit_copy *******************************************************************

   XXX

*******************************************************************************/

void emit_copy(jitdata *jd, instruction *iptr, varinfo *src, varinfo *dst)
{
	codegendata  *cd;
	registerdata *rd;
	s4            s1, d;

	/* get required compiler data */

	cd = jd->cd;
	rd = jd->rd;

	/* XXX dummy call, removed me!!! */
	d = codegen_reg_of_var(iptr->opc, dst, REG_ITMP1);

	if ((src->vv.regoff != dst->vv.regoff) ||
		((src->flags ^ dst->flags) & INMEMORY)) {

		/* If one of the variables resides in memory, we can eliminate
		   the register move from/to the temporary register with the
		   order of getting the destination register and the load. */

		if (IS_INMEMORY(src->flags)) {
#if !defined(ENABLE_SOFTFLOAT)
			if (IS_FLT_DBL_TYPE(src->type))
				d = codegen_reg_of_var(iptr->opc, dst, REG_FTMP1);
			else
#endif
			{
				if (IS_2_WORD_TYPE(src->type))
					d = codegen_reg_of_var(iptr->opc, dst, REG_ITMP12_PACKED);
				else
					d = codegen_reg_of_var(iptr->opc, dst, REG_ITMP1);
			}

			s1 = emit_load(jd, iptr, src, d);
		}
		else {
#if !defined(ENABLE_SOFTFLOAT)
			if (IS_FLT_DBL_TYPE(src->type))
				s1 = emit_load(jd, iptr, src, REG_FTMP1);
			else
#endif
			{
				if (IS_2_WORD_TYPE(src->type))
					s1 = emit_load(jd, iptr, src, REG_ITMP12_PACKED);
				else
					s1 = emit_load(jd, iptr, src, REG_ITMP1);
			}

			d = codegen_reg_of_var(iptr->opc, dst, s1);
		}

		if (s1 != d) {
#if !defined(ENABLE_SOFTFLOAT)
			if (IS_FLT_DBL_TYPE(src->type)) {
				if (IS_2_WORD_TYPE(src->type))
					M_DMOV(s1, d);
				else
					M_FMOV(s1, d);
			}
			else {
				if (IS_2_WORD_TYPE(src->type))
					M_LNGMOVE(s1, d);
				else
					/* XXX grrrr, wrong direction! */
					M_MOV(d, s1);
			}
#else
			if (IS_2_WORD_TYPE(src->type))
				M_LNGMOVE(s1, d);
			else
				/* XXX grrrr, wrong direction! */
				M_MOV(d, s1);
#endif
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

	if (IS_IMM(value))
		M_MOV_IMM(d, value);
	else {
		disp = dseg_add_s4(cd, value);
		M_DSEG_LOAD(d, disp);
	}
}


/* emit_nullpointer_check ******************************************************

   Emit a NullPointerException check.

*******************************************************************************/

void emit_nullpointer_check(codegendata *cd, instruction *iptr, s4 reg)
{
	if (INSTRUCTION_MUST_CHECK(iptr)) {
		M_TST(reg, reg);
		M_BEQ(0);
		codegen_add_nullpointerexception_ref(cd);
	}
}


/* emit_arrayindexoutofbounds_check ********************************************

   Emit a ArrayIndexOutOfBoundsException check.

*******************************************************************************/

void emit_arrayindexoutofbounds_check(codegendata *cd, instruction *iptr, s4 s1, s4 s2)
{
	if (INSTRUCTION_MUST_CHECK(iptr)) {
		M_ILD_INTERN(REG_ITMP3, s1, OFFSET(java_arrayheader, size));
		M_CMP(s2, REG_ITMP3);
		M_BHS(0);
		codegen_add_arrayindexoutofboundsexception_ref(cd, s2);
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

		/* Check if the exception is an
		   ArrayIndexOutOfBoundsException.  If so, move index register
		   into REG_ITMP1. */

		if (eref->reg != -1)
			M_MOV(REG_ITMP1, eref->reg);

		/* calcuate exception address */

		assert((eref->branchpos - 4) % 4 == 0);
		M_ADD_IMM_EXT_MUL4(REG_ITMP2_XPC, REG_IP, (eref->branchpos - 4) / 4);

		/* move function to call into REG_ITMP3 */

		disp = dseg_add_functionptr(cd, eref->function);
		M_DSEG_LOAD(REG_ITMP3, disp);

		if (targetdisp == 0) {
			targetdisp = ((u4 *) cd->mcodeptr) - ((u4 *) cd->mcodebase);

			M_MOV(rd->argintregs[0], REG_IP);
			M_MOV(rd->argintregs[1], REG_SP);

			if (jd->isleafmethod)
				M_MOV(rd->argintregs[2], REG_LR);
			else
				M_LDR(rd->argintregs[2], REG_SP,
					  cd->stackframesize * 4 - SIZEOF_VOID_P);

			M_MOV(rd->argintregs[3], REG_ITMP2_XPC);

			/* save registers */
			/* TODO: we only need to save LR in leaf methods */

			M_STMFD(BITMASK_ARGS | 1<<REG_IP | 1<<REG_LR, REG_SP);

			/* move a3 to stack */

			M_STR_UPDATE(REG_ITMP1, REG_SP, -4);

			/* do the exception call */

			M_MOV(REG_LR, REG_PC);
			M_MOV(REG_PC, REG_ITMP3);

			M_ADD_IMM(REG_SP, REG_SP, 4);

			/* result of stacktrace is our XPTR */

			M_MOV(REG_ITMP1_XPTR, REG_RESULT);

			/* restore registers */

			M_LDMFD(BITMASK_ARGS | 1<<REG_IP | 1<<REG_LR, REG_SP);

			disp = dseg_add_functionptr(cd, asm_handle_exception);
			M_DSEG_LOAD(REG_ITMP3, disp);
			M_MOV(REG_PC, REG_ITMP3);
		}
		else {
			disp = (((u4 *) cd->mcodebase) + targetdisp) -
				(((u4 *) cd->mcodeptr) + 2);

			M_B(disp);
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

	/* generate patcher stub call code */

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

		savedmcodeptr = cd->mcodeptr;   /* save current mcodeptr              */
		cd->mcodeptr  = tmpmcodeptr;    /* set mcodeptr to patch position     */

		disp = ((u4 *) savedmcodeptr) - (((u4 *) tmpmcodeptr) + 2);
		M_B(disp);

		cd->mcodeptr = savedmcodeptr;   /* restore the current mcodeptr       */

		/* create stack frame */

		M_SUB_IMM(REG_SP, REG_SP, 7 * 4);

		/* save itmp3 onto stack */

		M_STR_INTERN(REG_ITMP3, REG_SP, 6 * 4);

		/* calculate return address and move it onto stack */
		/* ATTENTION: we can not use BL to branch to patcher stub,        */
		/* ATTENTION: because we need to preserve LR for leaf methods     */

		disp = (s4) (((u4 *) cd->mcodeptr) - (((u4 *) tmpmcodeptr) + 1) + 2);

		M_SUB_IMM_EXT_MUL4(REG_ITMP3, REG_PC, disp);
		M_STR_INTERN(REG_ITMP3, REG_SP, 4 * 4);

		/* move pointer to java_objectheader onto stack */

#if defined(ENABLE_THREADS)
		/* order reversed because of data segment layout */

		(void) dseg_add_unique_address(cd, NULL);           /* flcword    */
		(void) dseg_add_unique_address(cd, lock_get_initial_lock_word());
		disp = dseg_add_unique_address(cd, NULL);           /* vftbl      */

		M_SUB_IMM_EXT_MUL4(REG_ITMP3, REG_IP, -disp / 4);
		M_STR_INTERN(REG_ITMP3, REG_SP, 3 * 4);
#else
		M_EOR(REG_ITMP3, REG_ITMP3, REG_ITMP3);
		M_STR_INTERN(REG_ITMP3, REG_SP, 3 * 4);
#endif

		/* move machine code onto stack */

		disp = dseg_add_unique_s4(cd, mcode);
		M_DSEG_LOAD(REG_ITMP3, disp);
		M_STR_INTERN(REG_ITMP3, REG_SP, 2 * 4);

		/* move class/method/field reference onto stack */

		disp = dseg_add_unique_address(cd, pref->ref);
		M_DSEG_LOAD(REG_ITMP3, disp);
		M_STR_INTERN(REG_ITMP3, REG_SP, 1 * 4);

		/* move data segment displacement onto stack */

		disp = dseg_add_unique_s4(cd, pref->disp);
		M_DSEG_LOAD(REG_ITMP3, disp);
		M_STR_INTERN(REG_ITMP3, REG_SP, 5 * 4);

		/* move patcher function pointer onto stack */

		disp = dseg_add_functionptr(cd, pref->patcher);
		M_DSEG_LOAD(REG_ITMP3, disp);
		M_STR_INTERN(REG_ITMP3, REG_SP, 0 * 4);

		/* finally call the patcher via asm_patcher_wrapper */
		/* ATTENTION: don't use REG_IP here, because some patchers need it */

		if (targetdisp == 0) {
			targetdisp = ((u4 *) cd->mcodeptr) - ((u4 *) cd->mcodebase);

			disp = dseg_add_functionptr(cd, asm_patcher_wrapper);
			/*M_DSEG_BRANCH_NOLINK(REG_PC, REG_IP, a);*/
			/* TODO: this is only a hack */
			M_DSEG_LOAD(REG_ITMP3, disp);
			M_MOV(REG_PC, REG_ITMP3);
		}
		else {
			disp = (((u4 *) cd->mcodebase) + targetdisp) -
				(((u4 *) cd->mcodeptr) + 2);

			M_B(disp);
		}
	}
}


/* emit_replacement_stubs ******************************************************

   Generates the code for the replacement stubs.

*******************************************************************************/

#if defined(ENABLE_REPLACEMENT)
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
}
#endif /* defined(ENABLE_REPLACEMENT) */


/* emit_verbosecall_enter ******************************************************

   Generates the code for the call trace.

*******************************************************************************/

#if !defined(NDEBUG)
void emit_verbosecall_enter(jitdata *jd)
{
	methodinfo   *m;
	codegendata  *cd;
	registerdata *rd;
	methoddesc   *md;
	s4            stackframesize;
	s4            disp;
	s4            i, t, s1, s2;

	/* get required compiler data */

	m  = jd->m;
	cd = jd->cd;
	rd = jd->rd;

	md = m->parseddesc;

	/* stackframesize is changed below */

	stackframesize = cd->stackframesize;

	/* mark trace code */

	M_NOP;

	/* save argument registers to stack (including LR and IP) */
	M_STMFD(BITMASK_ARGS | (1<<REG_LR) | (1<<REG_IP), REG_SP);
	M_SUB_IMM(REG_SP, REG_SP, (2 + 2 + 1) * 4);     /* space for a3, a4 and m */

	stackframesize += 6 + 2 + 2 + 1;

	/* prepare args for tracer */

	i = md->paramcount - 1;

	if (i > 3)
		i = 3;

	for (; i >= 0; i--) {
		t = md->paramtypes[i].type;

		/* load argument into register (s1) and make it of TYPE_LNG */

		if (!md->params[i].inmemory) {
			s1 = md->params[i].regoff;

			if (!IS_2_WORD_TYPE(t)) {
				M_MOV_IMM(REG_ITMP1, 0);
				s1 = PACK_REGS(s1, REG_ITMP1);
			}
			else {
				SPLIT_OPEN(t, s1, REG_ITMP1);
				SPLIT_LOAD(t, s1, stackframesize);
			}
		}
		else {
			s1 = md->params[i].regoff + stackframesize;

			if (IS_2_WORD_TYPE(t))
				M_LLD(REG_ITMP12_PACKED, REG_SP, s1 * 4);
			else
				M_ILD(REG_ITMP1, REG_SP, s1 * 4);
		}

		/* place argument for tracer */

		if (i < 2) {
#if defined(__ARMEL__)
			s2 = PACK_REGS(rd->argintregs[i * 2], rd->argintregs[i * 2 + 1]);
#else /* defined(__ARMEB__) */
			s2 = PACK_REGS(rd->argintregs[i * 2 + 1], rd->argintregs[i * 2]);
#endif          
			M_LNGMOVE(s1, s2);
		}
		else {
			s2 = (i - 2) * 2;
			M_LST(s1, REG_SP, s2 * 4);
		}
	}

	/* prepare methodinfo pointer for tracer */

	disp = dseg_add_address(cd, m);
	M_DSEG_LOAD(REG_ITMP1, disp);
	M_STR_INTERN(REG_ITMP1, REG_SP, 16);

	/* call tracer here (we use a long branch) */

	M_LONGBRANCH(builtin_trace_args);

	/* restore argument registers from stack */

	M_ADD_IMM(REG_SP, REG_SP, (2 + 2 + 1) * 4);        /* free argument stack */
	M_LDMFD(BITMASK_ARGS | (1<<REG_LR) | (1<<REG_IP), REG_SP);

	/* mark trace code */

	M_NOP;
}
#endif /* !defined(NDEBUG) */


/* emit_verbosecall_exit *******************************************************

   Generates the code for the call trace.

*******************************************************************************/

#if !defined(NDEBUG)
void emit_verbosecall_exit(jitdata *jd)
{
	methodinfo   *m;
	codegendata  *cd;
	registerdata *rd;
	methoddesc   *md;
	s4            disp;
	s4            s1;

	/* get required compiler data */

	m  = jd->m;
	cd = jd->cd;
	rd = jd->rd;

	md = m->parseddesc;

	/* mark trace code */

	M_NOP;

	M_STMFD(BITMASK_RESULT | (1<<REG_LR) | (1<<REG_IP), REG_SP);
	M_SUB_IMM(REG_SP, REG_SP, (1 + 1) * 4);    /* space for d[high reg] and f */

#if defined(__ARMEL__)
	s1 = PACK_REGS(rd->argintregs[1], rd->argintregs[2]);
#else /* defined(__ARMEB__) */
	s1 = PACK_REGS(rd->argintregs[2], rd->argintregs[1]);
#endif

	switch (md->returntype.type) {
	case TYPE_ADR:
	case TYPE_INT:
		M_INTMOVE(REG_RESULT, GET_LOW_REG(s1));
		M_MOV_IMM(GET_HIGH_REG(s1), 0);
		break;

	case TYPE_LNG:
		M_LNGMOVE(REG_RESULT_PACKED, s1);
		break;

	case TYPE_FLT:
		M_IST(REG_RESULT, REG_SP, 1 * 4);
		break;

	case TYPE_DBL:
		s1 = rd->argintregs[3];
		M_INTMOVE(REG_RESULT, s1);
		M_IST(REG_RESULT2, REG_SP, 0 * 4);
		break;
	}

	disp = dseg_add_address(cd, m);
	M_DSEG_LOAD(rd->argintregs[0], disp);
	M_LONGBRANCH(builtin_displaymethodstop);

	M_ADD_IMM(REG_SP, REG_SP, (1 + 1) * 4);            /* free argument stack */
	M_LDMFD(BITMASK_RESULT | (1<<REG_LR) | (1<<REG_IP), REG_SP);

	/* mark trace code */

	M_NOP;
}
#endif /* !defined(NDEBUG) */


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
