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

		disp = src->vv.regoff * 8;

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

		disp = dst->vv.regoff * 8;

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
				M_FMOV(s1, d);
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
		M_XOR(REG_ZERO, value, d);
	} else {
		disp = dseg_adds4(cd, value);
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
		M_XOR(REG_ZERO, value, d);	
	} else {
		disp = dseg_adds8(cd, value);
		M_LDX(d, REG_PV_CALLEE, disp);
	}
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

	M_LDA(REG_SP, REG_SP, -(1 + WINSAVE_CNT + FLT_ARG_CNT) * 8);

	/* save float argument registers */

	for (i = 0; i < FLT_ARG_CNT; i++)
		M_DST(rd->argfltregs[i], REG_SP, BIAS + (WINSAVE_CNT + 1 + i) * 8);

	/* save temporary registers for leaf methods */
/* XXX no leaf optimization yet
	if (jd->isleafmethod) {
		for (i = 0; i < INT_TMP_CNT; i++)
			M_LST(rd->tmpintregs[i], REG_SP, (2 + ARG_CNT + i) * 8);

		for (i = 0; i < FLT_TMP_CNT; i++)
			M_DST(rd->tmpfltregs[i], REG_SP, (2 + ARG_CNT + INT_TMP_CNT + i) * 8);
	}
*/
	/* load float arguments into integer registers */

	for (i = 0; i < md->paramcount && i < INT_ARG_CNT; i++) {
		t = md->paramtypes[i].type;

		if (IS_FLT_DBL_TYPE(t)) {
			if (IS_2_WORD_TYPE(t)) {
				M_DST(rd->argfltregs[i], REG_SP, USESTACK);
				M_LDX(rd->argintregs[i], REG_SP, USESTACK);
			}
			else {
				M_DST(rd->argfltregs[i], REG_SP, USESTACK);
				M_LDX(rd->argintregs[i], REG_SP, USESTACK);
			}
		}
	}

	disp = dseg_add_address(cd, m);
	M_ALD(REG_ITMP1, REG_PV_CALLEE, disp);
	M_AST(REG_ITMP1, REG_SP, USESTACK);
	disp = dseg_add_functionptr(cd, builtin_trace_args);
	M_ALD(REG_PV_CALLER, REG_PV_CALLEE, disp);
	M_JMP(REG_RA_CALLER, REG_PV_CALLER, REG_ZERO);
	M_NOP;

	/* restore float argument registers */

	for (i = 0; i < FLT_ARG_CNT; i++)
		M_DLD(rd->argfltregs[i], REG_SP, BIAS + (WINSAVE_CNT + 1 + i) * 8);

	/* restore temporary registers for leaf methods */
/* XXX no leaf optimization yet
	if (jd->isleafmethod) {
		for (i = 0; i < INT_TMP_CNT; i++)
			M_LLD(rd->tmpintregs[i], REG_SP, (2 + ARG_CNT + i) * 8);

		for (i = 0; i < FLT_TMP_CNT; i++)
			M_DLD(rd->tmpfltregs[i], REG_SP, (2 + ARG_CNT + INT_TMP_CNT + i) * 8);
	}
*/
	M_LDA(REG_SP, REG_SP, (1 + WINSAVE_CNT + FLT_ARG_CNT) * 8);

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

	M_LDA(REG_SP, REG_SP, -2 * 8);           /* keep stack 16-byte aligned ???*/

	M_DST(REG_FRESULT, REG_SP, USESTACK + (1 * 8));

	disp = dseg_add_address(cd, m);
	M_ALD(rd->argintregs[0], REG_PV_CALLEE, disp);

	M_MOV(REG_RESULT_CALLEE, rd->argintregs[1]);
	M_DMOV(REG_FRESULT, rd->argfltregs[2]);
	M_FMOV(REG_FRESULT, rd->argfltregs[3]);

	disp = dseg_add_functionptr(cd, builtin_displaymethodstop);
	M_ALD(REG_ITMP3, REG_PV_CALLEE, disp);
	M_JMP(REG_RA_CALLER, REG_ITMP3, REG_ZERO);
	M_NOP;

	M_DLD(REG_FRESULT, REG_SP, USESTACK + (1 * 8));

	M_LDA(REG_SP, REG_SP, 2 * 8);

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
