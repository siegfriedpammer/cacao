/* src/vm/jit/alpha/emit.c - Alpha code emitter functions

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

   $Id: emit.c 4398 2006-01-31 23:43:08Z twisti $

*/


#include "config.h"
#include "vm/types.h"

#include "md-abi.h"

#include "vm/jit/alpha/codegen.h"

#if defined(ENABLE_THREADS)
# include "threads/native/lock.h"
#endif

#include "vm/builtin.h"
#include "vm/jit/abi-asm.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/dseg.h"
#include "vm/jit/emit.h"
#include "vm/jit/jit.h"
#include "vm/jit/replace.h"


/* code generation functions **************************************************/

/* emit_load *******************************************************************

   Emits a possible load of an operand.

*******************************************************************************/

s4 emit_load(jitdata *jd, instruction *iptr, stackptr src, s4 tempreg)
{
	codegendata  *cd;
	s4            reg;

	/* get required compiler data */

	cd = jd->cd;

	if (src->flags & INMEMORY) {
		COUNT_SPILLS;

		if (IS_FLT_DBL_TYPE(src->type))
			M_DLD(tempreg, REG_SP, src->regoff * 8);
		else
			M_LLD(tempreg, REG_SP, src->regoff * 8);

		reg = tempreg;
	} else
		reg = src->regoff;

	return reg;
}


/* emit_load_s1 ****************************************************************

   Emits a possible load of the first source operand.

*******************************************************************************/

s4 emit_load_s1(jitdata *jd, instruction *iptr, s4 tempreg)
{
	s4 r;
	
	r = emit_load(jd, iptr, iptr->s1.var, tempreg);

	return r;
}


/* emit_load_s2 ****************************************************************

   Emits a possible load of the second source operand.

*******************************************************************************/

s4 emit_load_s2(jitdata *jd, instruction *iptr, s4 tempreg)
{
	s4 r;

	r = emit_load(jd, iptr, iptr->sx.s23.s2.var, tempreg);

	return r;
}


/* emit_load_s3 ****************************************************************

   Emits a possible load of the third source operand.

*******************************************************************************/

s4 emit_load_s3(jitdata *jd, instruction *iptr, s4 tempreg)
{
	s4 r;

	r = emit_load(jd, iptr, iptr->sx.s23.s3.var, tempreg);

	return r;
}


/* emit_store ******************************************************************

   Emit a possible store for the given variable.

*******************************************************************************/

void emit_store(jitdata *jd, instruction *iptr, stackptr dst, s4 d)
{
	codegendata  *cd;

	/* get required compiler data */

	cd = jd->cd;

	if (dst->flags & INMEMORY) {
		COUNT_SPILLS;

		if (IS_FLT_DBL_TYPE(dst->type))
			M_DST(d, REG_SP, dst->regoff * 8);
		else
			M_LST(d, REG_SP, dst->regoff * 8);
	}
}


/* emit_store_dst **************************************************************

   Emit a possible store for the destination operand.

*******************************************************************************/

void emit_store_dst(jitdata *jd, instruction *iptr, s4 d)
{
	emit_store(jd, iptr, iptr->dst.var, d);
}


void emit_copy(jitdata *jd, instruction *iptr, stackptr src, stackptr dst)
{
	codegendata  *cd;
	registerdata *rd;
	s4            s1, d;

	/* get required compiler data */

	cd = jd->cd;
	rd = jd->rd;

	if ((src->regoff != dst->regoff) ||
		((src->flags ^ dst->flags) & INMEMORY)) {
		d = codegen_reg_of_var(rd, iptr->opc, dst, REG_IFTMP);
		s1 = emit_load(jd, iptr, src, d);

		if (IS_FLT_DBL_TYPE(src->type))
			M_FLTMOVE(s1, d);
		else
			M_INTMOVE(s1, d);

		emit_store(jd, iptr, dst, d);
	}
}


void emit_iconst(codegendata *cd, s4 d, s4 value)
{
	s4 disp;

	if ((value >= -32768) && (value <= 32767)) {
		M_LDA_INTERN(d, REG_ZERO, value);
	} else {
		disp = dseg_adds4(cd, value);
		M_ILD(d, REG_PV, disp);
	}
}


void emit_lconst(codegendata *cd, s4 d, s8 value)
{
	s4 disp;

	if ((value >= -32768) && (value <= 32767)) {
		M_LDA_INTERN(d, REG_ZERO, value);
	} else {
		disp = dseg_adds8(cd, value);
		M_LLD(d, REG_PV, disp);
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

		/* move index register into REG_ITMP1 */

		/* Check if the exception is an
		   ArrayIndexOutOfBoundsException.  If so, move index register
		   into a4. */

		if (eref->reg != -1)
			M_MOV(eref->reg, rd->argintregs[4]);

		/* calcuate exception address */

		M_LDA(rd->argintregs[3], REG_PV, eref->branchpos - 4);

		/* move function to call into REG_ITMP3 */

		disp = dseg_add_functionptr(cd, eref->function);
		M_ALD(REG_ITMP3, REG_PV, disp);

		if (targetdisp == 0) {
			targetdisp = ((u4 *) cd->mcodeptr) - ((u4 *) cd->mcodebase);

			M_MOV(REG_PV, rd->argintregs[0]);
			M_MOV(REG_SP, rd->argintregs[1]);

			if (jd->isleafmethod)
				M_MOV(REG_RA, rd->argintregs[2]);
			else
				M_ALD(rd->argintregs[2],
					  REG_SP, cd->stackframesize * 8 - SIZEOF_VOID_P);

			M_LDA(REG_SP, REG_SP, -2 * 8);
			M_AST(rd->argintregs[3], REG_SP, 0 * 8);             /* store XPC */

			if (jd->isleafmethod)
				M_AST(REG_RA, REG_SP, 1 * 8);

			M_MOV(REG_ITMP3, REG_PV);
			M_JSR(REG_RA, REG_PV);
			disp = (s4) (cd->mcodeptr - cd->mcodebase);
			M_LDA(REG_PV, REG_RA, -disp);

			M_MOV(REG_RESULT, REG_ITMP1_XPTR);

			if (jd->isleafmethod)
				M_ALD(REG_RA, REG_SP, 1 * 8);

			M_ALD(REG_ITMP2_XPC, REG_SP, 0 * 8);
			M_LDA(REG_SP, REG_SP, 2 * 8);

			disp = dseg_add_functionptr(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_JMP(REG_ZERO, REG_ITMP3);
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

		savedmcodeptr = cd->mcodeptr;   /* save current mcodeptr              */
		cd->mcodeptr  = tmpmcodeptr;    /* set mcodeptr to patch position     */

		disp = ((u4 *) savedmcodeptr) - (((u4 *) tmpmcodeptr) + 1);
		M_BSR(REG_ITMP3, disp);

		cd->mcodeptr = savedmcodeptr;   /* restore the current mcodeptr       */

		/* create stack frame */

		M_LSUB_IMM(REG_SP, 6 * 8, REG_SP);

		/* move return address onto stack */

		M_AST(REG_ITMP3, REG_SP, 5 * 8);

		/* move pointer to java_objectheader onto stack */

#if defined(ENABLE_THREADS)
		/* create a virtual java_objectheader */

		(void) dseg_add_unique_address(cd, NULL);                  /* flcword */
		(void) dseg_add_unique_address(cd, lock_get_initial_lock_word());
		disp = dseg_add_unique_address(cd, NULL);                  /* vftbl   */

		M_LDA(REG_ITMP3, REG_PV, disp);
		M_AST(REG_ITMP3, REG_SP, 4 * 8);
#else
		/* nothing to do */
#endif

		/* move machine code onto stack */

		disp = dseg_add_s4(cd, mcode);
		M_ILD(REG_ITMP3, REG_PV, disp);
		M_IST(REG_ITMP3, REG_SP, 3 * 8);

		/* move class/method/field reference onto stack */

		disp = dseg_add_address(cd, pref->ref);
		M_ALD(REG_ITMP3, REG_PV, disp);
		M_AST(REG_ITMP3, REG_SP, 2 * 8);

		/* move data segment displacement onto stack */

		disp = dseg_add_s4(cd, pref->disp);
		M_ILD(REG_ITMP3, REG_PV, disp);
		M_IST(REG_ITMP3, REG_SP, 1 * 8);

		/* move patcher function pointer onto stack */

		disp = dseg_add_functionptr(cd, pref->patcher);
		M_ALD(REG_ITMP3, REG_PV, disp);
		M_AST(REG_ITMP3, REG_SP, 0 * 8);

		if (targetdisp == 0) {
			targetdisp = ((u4 *) cd->mcodeptr) - ((u4 *) cd->mcodebase);

			disp = dseg_add_functionptr(cd, asm_patcher_wrapper);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_JMP(REG_ZERO, REG_ITMP3);
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

		rplp->outcode = (u1 *) (ptrint) (cd->mcodeptr - cd->mcodebase);

		/* make machine code for patching */

		savedmcodeptr = cd->mcodeptr;
		cd->mcodeptr  = (u1 *) &(rplp->mcode);

		disp = (ptrint) ((s4 *) rplp->outcode - (s4 *) rplp->pc) - 1;
		M_BR(disp);

		cd->mcodeptr = savedmcodeptr;

		/* create stack frame - 16-byte aligned */

		M_LSUB_IMM(REG_SP, 2 * 8, REG_SP);

		/* push address of `rplpoint` struct */

		disp = dseg_add_address(cd, rplp);
		M_ALD(REG_ITMP3, REG_PV, disp);
		M_AST(REG_ITMP3, REG_SP, 0 * 8);

		/* jump to replacement function */

		disp = dseg_add_functionptr(cd, asm_replacement_out);
		M_ALD(REG_ITMP3, REG_PV, disp);
		M_JMP(REG_ZERO, REG_ITMP3);
	}
}


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
	s4            disp;
	s4            i, t;

	/* get required compiler data */

	m  = jd->m;
	cd = jd->cd;
	rd = jd->rd;

	md = m->parseddesc;

	/* mark trace code */

	M_NOP;

	M_LDA(REG_SP, REG_SP, -((ARG_CNT + TMP_CNT + 2) * 8));
	M_AST(REG_RA, REG_SP, 1 * 8);

	/* save argument registers */

	for (i = 0; i < INT_ARG_CNT; i++)
		M_LST(rd->argintregs[i], REG_SP, (2 + i) * 8);

	for (i = 0; i < FLT_ARG_CNT; i++)
		M_DST(rd->argintregs[i], REG_SP, (2 + INT_ARG_CNT + i) * 8);

	/* save temporary registers for leaf methods */

	if (jd->isleafmethod) {
		for (i = 0; i < INT_TMP_CNT; i++)
			M_LST(rd->tmpintregs[i], REG_SP, (2 + ARG_CNT + i) * 8);

		for (i = 0; i < FLT_TMP_CNT; i++)
			M_DST(rd->tmpfltregs[i], REG_SP, (2 + ARG_CNT + INT_TMP_CNT + i) * 8);
	}

	/* load float arguments into integer registers */

	for (i = 0; i < md->paramcount && i < INT_ARG_CNT; i++) {
		t = md->paramtypes[i].type;

		if (IS_FLT_DBL_TYPE(t)) {
			if (IS_2_WORD_TYPE(t)) {
				M_DST(rd->argfltregs[i], REG_SP, 0 * 8);
				M_LLD(rd->argintregs[i], REG_SP, 0 * 8);
			}
			else {
				M_FST(rd->argfltregs[i], REG_SP, 0 * 8);
				M_ILD(rd->argintregs[i], REG_SP, 0 * 8);
			}
		}
	}

	disp = dseg_add_address(cd, m);
	M_ALD(REG_ITMP1, REG_PV, disp);
	M_AST(REG_ITMP1, REG_SP, 0 * 8);
	disp = dseg_add_functionptr(cd, builtin_trace_args);
	M_ALD(REG_PV, REG_PV, disp);
	M_JSR(REG_RA, REG_PV);
	disp = (s4) (cd->mcodeptr - cd->mcodebase);
	M_LDA(REG_PV, REG_RA, -disp);
	M_ALD(REG_RA, REG_SP, 1 * 8);

	/* restore argument registers */

	for (i = 0; i < INT_ARG_CNT; i++)
		M_LLD(rd->argintregs[i], REG_SP, (2 + i) * 8);

	for (i = 0; i < FLT_ARG_CNT; i++)
		M_DLD(rd->argintregs[i], REG_SP, (2 + INT_ARG_CNT + i) * 8);

	/* restore temporary registers for leaf methods */

	if (jd->isleafmethod) {
		for (i = 0; i < INT_TMP_CNT; i++)
			M_LLD(rd->tmpintregs[i], REG_SP, (2 + ARG_CNT + i) * 8);

		for (i = 0; i < FLT_TMP_CNT; i++)
			M_DLD(rd->tmpfltregs[i], REG_SP, (2 + ARG_CNT + INT_TMP_CNT + i) * 8);
	}

	M_LDA(REG_SP, REG_SP, (ARG_CNT + TMP_CNT + 2) * 8);

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
	s4            disp;

	/* get required compiler data */

	m  = jd->m;
	cd = jd->cd;
	rd = jd->rd;

	/* mark trace code */

	M_NOP;

	M_LDA(REG_SP, REG_SP, -4 * 8);
	M_AST(REG_RA, REG_SP, 0 * 8);

	M_LST(REG_RESULT, REG_SP, 1 * 8);
	M_DST(REG_FRESULT, REG_SP, 2 * 8);

	disp = dseg_add_address(cd, m);
	M_ALD(rd->argintregs[0], REG_PV, disp);

	M_MOV(REG_RESULT, rd->argintregs[1]);
	M_FLTMOVE(REG_FRESULT, rd->argfltregs[2]);
	M_FLTMOVE(REG_FRESULT, rd->argfltregs[3]);

	disp = dseg_add_functionptr(cd, builtin_displaymethodstop);
	M_ALD(REG_PV, REG_PV, disp);
	M_JSR(REG_RA, REG_PV);
	disp = (cd->mcodeptr - cd->mcodebase);
	M_LDA(REG_PV, REG_RA, -disp);

	M_DLD(REG_FRESULT, REG_SP, 2 * 8);
	M_LLD(REG_RESULT, REG_SP, 1 * 8);

	M_ALD(REG_RA, REG_SP, 0 * 8);
	M_LDA(REG_SP, REG_SP, 4 * 8);

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
