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

#include "vm/jit/asmpart.h"
#include "vm/jit/dseg.h"
#include "vm/jit/emit.h"
#include "vm/jit/jit.h"


/* code generation functions **************************************************/

/* emit_load_s1 ****************************************************************

   Emits a possible load of the first source operand.

*******************************************************************************/

s4 emit_load_s1(jitdata *jd, instruction *iptr, stackptr src, s4 tempreg)
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


/* emit_load_s2 ****************************************************************

   Emits a possible load of the second source operand.

*******************************************************************************/

s4 emit_load_s2(jitdata *jd, instruction *iptr, stackptr src, s4 tempreg)
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


/* emit_load_s3 ****************************************************************

   Emits a possible load of the third source operand.

*******************************************************************************/

s4 emit_load_s3(jitdata *jd, instruction *iptr, stackptr src, s4 tempreg)
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


/* emit_store ******************************************************************

   XXX

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
		s1 = emit_load_s1(jd, iptr, src, d);

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
	u1           *savedmcodeptr;
	s4            disp;

	/* get required compiler data */

	cd = jd->cd;
	rd = jd->rd;

	savedmcodeptr = NULL;

	/* generate exception stubs */

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

		if (savedmcodeptr != NULL) {
			disp = ((u4 *) savedmcodeptr) - (((u4 *) cd->mcodeptr) + 1);
			M_BR(disp);
		}
		else {
			savedmcodeptr = cd->mcodeptr;

			M_MOV(REG_PV, rd->argintregs[0]);
			M_MOV(REG_SP, rd->argintregs[1]);

			if (jd->isleafmethod)
				M_MOV(REG_RA, rd->argintregs[2]);
			else
				M_ALD(rd->argintregs[2],
					  REG_SP, jd->stackframesize * 8 - SIZEOF_VOID_P);

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
	s4           disp;

	/* get required compiler data */

	cd = jd->cd;

	/* generate code patching stub call code */

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
		M_AST(REG_ZERO, REG_SP, 4 * 8);
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

		disp = dseg_add_functionptr(cd, asm_patcher_wrapper);
		M_ALD(REG_ITMP3, REG_PV, disp);
		M_JMP(REG_ZERO, REG_ITMP3);
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
