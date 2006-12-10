/* src/vm/jit/sparc64/emit.c - Sparc code emitter functions

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
            Alexander Jordan

   Changes: 

   $Id: emitfuncs.c 4398 2006-01-31 23:43:08Z twisti $

*/

#include "vm/types.h"

#include "md-abi.h"

#include "vm/jit/sparc64/codegen.h"

#include "vm/exceptions.h"
#include "vm/stringlocal.h" /* XXX for gen_resolvebranch */
#include "vm/jit/abi-asm.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/dseg.h"
#include "vm/jit/emit-common.h"
#include "vm/jit/jit.h"
#include "vm/jit/replace.h"

/* how to leaf optimization in the emitted stubs?? */
#define REG_PV REG_PV_CALLEE


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

		disp = USESTACK + src->vv.regoff * 8;

		if (IS_FLT_DBL_TYPE(src->type))
			M_DLD(tempreg, REG_SP, disp);
		else
			M_LDX(tempreg, REG_SP, disp);

		reg = tempreg;
	}
	else
		reg = src->vv.regoff;

	return reg;
}


/* emit_store ******************************************************************

   Emits a possible store to variable.

*******************************************************************************/

void emit_store(jitdata *jd, instruction *iptr, varinfo *dst, s4 d)
{
	codegendata  *cd;
	s4            disp;

	/* get required compiler data */

	cd = jd->cd;

	if (dst->flags & INMEMORY) {
		COUNT_SPILLS;

		disp = USESTACK + dst->vv.regoff * 8;

		if (IS_FLT_DBL_TYPE(dst->type))
			M_DST(d, REG_SP, disp);
		else
			M_STX(d, REG_SP, disp);
	}
}


/* emit_copy *******************************************************************

   Generates a register/memory to register/memory copy.

*******************************************************************************/

void emit_copy(jitdata *jd, instruction *iptr, varinfo *src, varinfo *dst)
{
	codegendata  *cd;
	registerdata *rd;
	s4            s1, d;

	/* get required compiler data */

	cd = jd->cd;
	rd = jd->rd;

	if ((src->vv.regoff != dst->vv.regoff) ||
		((src->flags ^ dst->flags) & INMEMORY)) {

		/* If one of the variables resides in memory, we can eliminate
		   the register move from/to the temporary register with the
		   order of getting the destination register and the load. */

		if (IS_INMEMORY(src->flags)) {
			d = codegen_reg_of_var(iptr->opc, dst, REG_IFTMP);
			s1 = emit_load(jd, iptr, src, d);
		}
		else {
			s1 = emit_load(jd, iptr, src, REG_IFTMP);
			d = codegen_reg_of_var(iptr->opc, dst, s1);
		}

		if (s1 != d) {
			if (IS_FLT_DBL_TYPE(src->type))
				M_DMOV(s1, d);
		else
				M_MOV(s1, d);
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

	if ((value >= -4096) && (value <= 4095)) {
		M_XOR_IMM(REG_ZERO, value, d);
	} else {
		disp = dseg_add_s4(cd, value);
		M_ILD(d, REG_PV_CALLEE, disp);
	}
}


/* emit_lconst *****************************************************************

   XXX

*******************************************************************************/

void emit_lconst(codegendata *cd, s4 d, s8 value)
{
	s4 disp;

	if ((value >= -4096) && (value <= 4095)) {
		M_XOR_IMM(REG_ZERO, value, d);	
	} else {
		disp = dseg_add_s8(cd, value);
		M_LDX(d, REG_PV_CALLEE, disp);
	}
}

/* emit_arrayindexoutofbounds_check ********************************************

   Emit an ArrayIndexOutOfBoundsException check.

*******************************************************************************/

void emit_arrayindexoutofbounds_check(codegendata *cd, s4 s1, s4 s2)
{
}

/* emit_nullpointer_check ******************************************************

   Emit a NullPointerException check.

*******************************************************************************/

void emit_nullpointer_check(codegendata *cd, s4 reg)
{
}

/* emit_exception_stubs ********************************************************

   Generates the code for the exception stubs.

*******************************************************************************/

void emit_exception_stubs(jitdata *jd)
{
}

/* emit_patcher_stubs **********************************************************

   Generates the code for the patcher stubs.

*******************************************************************************/

void emit_patcher_stubs(jitdata *jd)
{
	codegendata *cd;
	patchref    *pref;
	u4           mcode[2];
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
		   call is 2 instruction words long. */

		tmpmcodeptr = (u1 *) (cd->mcodebase + pref->branchpos);

		/* We use 2 loads here as an unaligned 8-byte read on 64-bit
		   SPARC causes a SIGSEGV */

		mcode[0] = ((u4 *) tmpmcodeptr)[0];
		mcode[1] = ((u4 *) tmpmcodeptr)[1];

		/* Patch in the call to call the following code (done at
		   compile time). */

		savedmcodeptr = cd->mcodeptr;   /* save current mcodeptr          */
		cd->mcodeptr  = tmpmcodeptr;    /* set mcodeptr to patch position */

		disp = ((u4 *) savedmcodeptr) - (((u4 *) tmpmcodeptr) );

		if ((disp < (s4) 0xfffc0000) || (disp > (s4) 0x003ffff)) {
			*exceptionptr =
				new_internalerror("Jump offset is out of range: %d > +/-%d",
								  disp, 0x003ffff);
			return;
		}

		M_BR(disp);
		M_NOP;

	cd->mcodeptr = savedmcodeptr;   /* restore the current mcodeptr   */

		/* extend stack frame for wrapper data */

		M_ASUB_IMM(REG_SP, 6 * 8, REG_SP);

		/* calculate return address and move it onto the stack */

		M_LDA(REG_ITMP3, REG_PV, pref->branchpos);
		M_AST(REG_ITMP3, REG_SP, USESTACK + 5 * 8);

		/* move pointer to java_objectheader onto stack */

#if defined(ENABLE_THREADS)
		/* create a virtual java_objectheader */

		(void) dseg_add_unique_address(cd, NULL);                  /* flcword */
		(void) dseg_add_unique_address(cd, lock_get_initial_lock_word());
		disp = dseg_add_unique_address(cd, NULL);                  /* vftbl   */

		M_LDA(REG_ITMP3, REG_PV, disp);
		M_AST(REG_ITMP3, REG_SP, USESTACK + 4 * 8);
#else
		/* do nothing */
#endif

		/* move machine code onto stack */

		disp = dseg_add_s4(cd, mcode[0]);
		M_ILD(REG_ITMP3, REG_PV, disp);
		M_IST(REG_ITMP3, REG_SP, USESTACK + 3 * 8);

		disp = dseg_add_s4(cd, mcode[1]);
		M_ILD(REG_ITMP3, REG_PV, disp);
		M_IST(REG_ITMP3, REG_SP, USESTACK + 3 * 8 + 4);

		/* move class/method/field reference onto stack */

		disp = dseg_add_address(cd, pref->ref);
		M_ALD(REG_ITMP3, REG_PV, disp);
		M_AST(REG_ITMP3, REG_SP, USESTACK + 2 * 8);

	/* move data segment displacement onto stack */

		disp = dseg_add_s4(cd, pref->disp);
		M_ILD(REG_ITMP3, REG_PV, disp);
		M_IST(REG_ITMP3, REG_SP, USESTACK + 1 * 8);

		/* move patcher function pointer onto stack */

		disp = dseg_add_address(cd, pref->patcher);
		M_ALD(REG_ITMP3, REG_PV, disp);
		M_AST(REG_ITMP3, REG_SP, USESTACK + 0 * 8);

		if (targetdisp == 0) {
			targetdisp = ((u4 *) cd->mcodeptr) - ((u4 *) cd->mcodebase);

			disp = dseg_add_functionptr(cd, asm_patcher_wrapper);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_JMP(REG_ZERO, REG_ITMP3, REG_ZERO);
			M_NOP;
		}
		else {
			disp = (((u4 *) cd->mcodebase) + targetdisp) -
				(((u4 *) cd->mcodeptr));

			M_BR(disp);
			M_NOP;
		}
	}
}


/* emit_replacement_stubs ******************************************************

   Generates the code for the replacement stubs.

*******************************************************************************/

void emit_replacement_stubs(jitdata *jd)
{
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

	/* we're calling a c function allocate paramter array */
	M_LDA(REG_SP, REG_SP, -(1 + FLT_ARG_CNT + ABI_PARAMARRAY_SLOTS) * 8);

	/* save float argument registers */

	for (i = 0; i < FLT_ARG_CNT; i++)
		M_DST(rd->argfltregs[i], REG_SP, USESTACK_PARAMS + (1 + i) * 8);

	/* save temporary registers for leaf methods */
/* XXX no leaf optimization yet
	if (jd->isleafmethod) {
		for (i = 0; i < INT_TMP_CNT; i++)
			M_LST(rd->tmpintregs[i], REG_SP, (2 + ARG_CNT + i) * 8);

		for (i = 0; i < FLT_TMP_CNT; i++)
			M_DST(rd->tmpfltregs[i], REG_SP, (2 + ARG_CNT + INT_TMP_CNT + i) * 8);
	}
*/
	/* load int/float arguments into integer argument registers */

	for (i = 0; i < md->paramcount && i < INT_ARG_CNT; i++) {
		t = md->paramtypes[i].type;

		if (IS_INT_LNG_TYPE(t)) {
			M_INTMOVE(REG_WINDOW_TRANSPOSE(rd->argintregs[i]), rd->argintregs[i]);
		}
		else {
			if (IS_2_WORD_TYPE(t)) {
				M_DST(rd->argfltregs[i], REG_SP, USESTACK_PARAMS);
				M_LDX(rd->argintregs[i], REG_SP, USESTACK_PARAMS);
			}
			else {
				M_FST(rd->argfltregs[i], REG_SP, USESTACK_PARAMS);
				M_ILD(rd->argintregs[i], REG_SP, USESTACK_PARAMS);
			}
		}
	}
	
	
	/* method info pointer is passed in argument register 5 */
	disp = dseg_add_address(cd, m);
	M_ALD(REG_OUT5, REG_PV_CALLEE, disp);
	disp = dseg_add_functionptr(cd, builtin_trace_args);
	M_ALD(REG_ITMP1, REG_PV_CALLEE, disp);
	M_JMP(REG_RA_CALLER, REG_ITMP1, REG_ZERO);
	M_NOP;

	/* restore float argument registers */

	for (i = 0; i < FLT_ARG_CNT; i++)
		M_DLD(rd->argfltregs[i], REG_SP, USESTACK_PARAMS + (1 + i) * 8);

	/* restore temporary registers for leaf methods */
/* XXX no leaf optimization yet
	if (jd->isleafmethod) {
		for (i = 0; i < INT_TMP_CNT; i++)
			M_LLD(rd->tmpintregs[i], REG_SP, (2 + ARG_CNT + i) * 8);

		for (i = 0; i < FLT_TMP_CNT; i++)
			M_DLD(rd->tmpfltregs[i], REG_SP, (2 + ARG_CNT + INT_TMP_CNT + i) * 8);
	}
*/
	M_LDA(REG_SP, REG_SP, (1 + FLT_ARG_CNT + ABI_PARAMARRAY_SLOTS) * 8);

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
	
	/* we're calling a c function allocate paramter array */
	M_LDA(REG_SP, REG_SP, -(1 + ABI_PARAMARRAY_SLOTS) * 8);

	M_DST(REG_FRESULT, REG_SP, USESTACK_PARAMS);

	disp = dseg_add_address(cd, m);
	M_ALD(rd->argintregs[0], REG_PV_CALLEE, disp);

	M_MOV(REG_RESULT_CALLEE, rd->argintregs[1]);
	M_DMOV(REG_FRESULT, 2); /* applies for flt and dbl values */

	disp = dseg_add_functionptr(cd, builtin_displaymethodstop);
	M_ALD(REG_ITMP3, REG_PV_CALLEE, disp);
	M_JMP(REG_RA_CALLER, REG_ITMP3, REG_ZERO);
	M_NOP;

	M_DLD(REG_FRESULT, REG_SP, USESTACK_PARAMS);

	M_LDA(REG_SP, REG_SP, (1 + ABI_PARAMARRAY_SLOTS) * 8);

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
