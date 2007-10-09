/* src/vm/jit/powerpc/emit.c - PowerPC code emitter functions

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

*/


#include "config.h"

#include <assert.h>
#include <stdint.h>

#include "vm/types.h"

#include "md-abi.h"

#include "vm/jit/powerpc/codegen.h"

#include "mm/memory.h"

#include "threads/lock-common.h"

#include "vm/exceptions.h"

#include "vm/jit/abi.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/codegen-common.h"
#include "vm/jit/dseg.h"
#include "vm/jit/emit-common.h"
#include "vm/jit/jit.h"
#include "vm/jit/replace.h"
#include "vm/jit/trace.h"

#include "vmcore/options.h"


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

		disp = src->vv.regoff;

		switch (src->type) {
		case TYPE_INT:
		case TYPE_ADR:
			M_ILD(tempreg, REG_SP, disp);
			break;
		case TYPE_LNG:
			M_LLD(tempreg, REG_SP, disp);
			break;
		case TYPE_FLT:
		case TYPE_DBL:
			M_DLD(tempreg, REG_SP, disp);
			break;
		default:
			vm_abort("emit_load: unknown type %d", src->type);
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

		disp = src->vv.regoff;

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

		disp = src->vv.regoff;

		M_ILD(tempreg, REG_SP, disp);

		reg = tempreg;
	}
	else
		reg = GET_HIGH_REG(src->vv.regoff);

	return reg;
}


/* emit_store ******************************************************************

   Emit a possible store for the given variable.

*******************************************************************************/

void emit_store(jitdata *jd, instruction *iptr, varinfo *dst, s4 d)
{
	codegendata *cd;
	s4           disp;

	/* get required compiler data */

	cd = jd->cd;

	if (IS_INMEMORY(dst->flags)) {
		COUNT_SPILLS;

		disp = dst->vv.regoff;

		switch (dst->type) {
		case TYPE_INT:
		case TYPE_ADR:
			M_IST(d, REG_SP, disp);
			break;
		case TYPE_LNG:
			M_LST(d, REG_SP, disp);
			break;
		case TYPE_FLT:
		case TYPE_DBL:
			M_DST(d, REG_SP, disp);
			break;
		default:
			vm_abort("emit_store: unknown type %d", dst->type);
		}
	}
}


/* emit_copy *******************************************************************

   Generates a register/memory to register/memory copy.

*******************************************************************************/

void emit_copy(jitdata *jd, instruction *iptr)
{
	codegendata *cd;
	varinfo     *src;
	varinfo     *dst;
	s4           s1, d;

	/* get required compiler data */

	cd = jd->cd;

	/* get source and destination variables */

	src = VAROP(iptr->s1);
	dst = VAROP(iptr->dst);

	if ((src->vv.regoff != dst->vv.regoff) ||
		(IS_INMEMORY(src->flags ^ dst->flags))) {

		if ((src->type == TYPE_RET) || (dst->type == TYPE_RET)) {
			/* emit nothing, as the value won't be used anyway */
			return;
		}

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
			switch (src->type) {
			case TYPE_INT:
			case TYPE_ADR:
				M_MOV(s1, d);
				break;
			case TYPE_LNG:
				M_MOV(GET_LOW_REG(s1), GET_LOW_REG(d));
				M_MOV(GET_HIGH_REG(s1), GET_HIGH_REG(d));
				break;
			case TYPE_FLT:
			case TYPE_DBL:
				M_FMOV(s1, d);
				break;
			default:
				vm_abort("emit_copy: unknown type %d", src->type);
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


/* emit_branch *****************************************************************

   Emits the code for conditional and unconditional branchs.

*******************************************************************************/

void emit_branch(codegendata *cd, s4 disp, s4 condition, s4 reg, u4 opt)
{
	s4 checkdisp;
	s4 branchdisp;

	/* calculate the different displacements */

	checkdisp  =  disp + 4;
	branchdisp = (disp - 4) >> 2;

	/* check which branch to generate */

	if (condition == BRANCH_UNCONDITIONAL) {
		/* check displacement for overflow */

		if ((checkdisp < (s4) 0xfe000000) || (checkdisp > (s4) 0x01fffffc)) {
			/* if the long-branches flag isn't set yet, do it */

			if (!CODEGENDATA_HAS_FLAG_LONGBRANCHES(cd)) {
				cd->flags |= (CODEGENDATA_FLAG_ERROR |
							  CODEGENDATA_FLAG_LONGBRANCHES);
			}

			vm_abort("emit_branch: emit unconditional long-branch code");
		}
		else {
			M_BR(branchdisp);
		}
	}
	else {
		/* and displacement for overflow */

		if ((checkdisp < (s4) 0xffff8000) || (checkdisp > (s4) 0x00007fff)) {
			/* if the long-branches flag isn't set yet, do it */

			if (!CODEGENDATA_HAS_FLAG_LONGBRANCHES(cd)) {
				cd->flags |= (CODEGENDATA_FLAG_ERROR |
							  CODEGENDATA_FLAG_LONGBRANCHES);
			}

			switch (condition) {
			case BRANCH_EQ:
				M_BNE(1);
				M_BR(branchdisp);
				break;
			case BRANCH_NE:
				M_BEQ(1);
				M_BR(branchdisp);
				break;
			case BRANCH_LT:
				M_BGE(1);
				M_BR(branchdisp);
				break;
			case BRANCH_GE:
				M_BLT(1);
				M_BR(branchdisp);
				break;
			case BRANCH_GT:
				M_BLE(1);
				M_BR(branchdisp);
				break;
			case BRANCH_LE:
				M_BGT(1);
				M_BR(branchdisp);
				break;
			case BRANCH_NAN:
				vm_abort("emit_branch: long BRANCH_NAN");
				break;
			default:
				vm_abort("emit_branch: unknown condition %d", condition);
			}
		}
		else {
			switch (condition) {
			case BRANCH_EQ:
				M_BEQ(branchdisp);
				break;
			case BRANCH_NE:
				M_BNE(branchdisp);
				break;
			case BRANCH_LT:
				M_BLT(branchdisp);
				break;
			case BRANCH_GE:
				M_BGE(branchdisp);
				break;
			case BRANCH_GT:
				M_BGT(branchdisp);
				break;
			case BRANCH_LE:
				M_BLE(branchdisp);
				break;
			case BRANCH_NAN:
				M_BNAN(branchdisp);
				break;
			default:
				vm_abort("emit_branch: unknown condition %d", condition);
			}
		}
	}
}


/* emit_arithmetic_check *******************************************************

   Emit an ArithmeticException check.

*******************************************************************************/

void emit_arithmetic_check(codegendata *cd, instruction *iptr, s4 reg)
{
	if (INSTRUCTION_MUST_CHECK(iptr)) {
		M_TST(reg);
		M_BNE(1);
		M_ALD_INTERN(REG_ZERO, REG_ZERO, EXCEPTION_HARDWARE_ARITHMETIC);
	}
}


/* emit_arrayindexoutofbounds_check ********************************************

   Emit a ArrayIndexOutOfBoundsException check.

*******************************************************************************/

void emit_arrayindexoutofbounds_check(codegendata *cd, instruction *iptr, s4 s1, s4 s2)
{
	if (INSTRUCTION_MUST_CHECK(iptr)) {
		M_ILD(REG_ITMP3, s1, OFFSET(java_array_t, size));
		M_TRAPGEU(s2, REG_ITMP3);
	}
}


/* emit_arraystore_check *******************************************************

   Emit an ArrayStoreException check.

*******************************************************************************/

void emit_arraystore_check(codegendata *cd, instruction *iptr)
{
	if (INSTRUCTION_MUST_CHECK(iptr)) {
		M_TST(REG_RESULT);
		M_BNE(1);
		M_ALD_INTERN(REG_ZERO, REG_ZERO, EXCEPTION_HARDWARE_ARRAYSTORE);
	}
}


/* emit_classcast_check ********************************************************

   Emit a ClassCastException check.

*******************************************************************************/

void emit_classcast_check(codegendata *cd, instruction *iptr, s4 condition, s4 reg, s4 s1)
{
	if (INSTRUCTION_MUST_CHECK(iptr)) {
		switch (condition) {
		case BRANCH_LE:
			M_BGT(1);
			break;
		case BRANCH_EQ:
			M_BNE(1);
			break;
		case BRANCH_GT:
			M_BLE(1);
			break;
		default:
			vm_abort("emit_classcast_check: unknown condition %d", condition);
		}
		M_ALD_INTERN(s1, REG_ZERO, EXCEPTION_HARDWARE_CLASSCAST);
	}
}


/* emit_nullpointer_check ******************************************************

   Emit a NullPointerException check.

*******************************************************************************/

void emit_nullpointer_check(codegendata *cd, instruction *iptr, s4 reg)
{
	if (INSTRUCTION_MUST_CHECK(iptr)) {
		M_TST(reg);
		M_BNE(1);
		M_ALD_INTERN(REG_ZERO, REG_ZERO, EXCEPTION_HARDWARE_NULLPOINTER);
	}
}


/* emit_exception_check ********************************************************

   Emit an Exception check.

*******************************************************************************/

void emit_exception_check(codegendata *cd, instruction *iptr)
{
	if (INSTRUCTION_MUST_CHECK(iptr)) {
		M_TST(REG_RESULT);
		M_BNE(1);
		M_ALD_INTERN(REG_ZERO, REG_ZERO, EXCEPTION_HARDWARE_EXCEPTION);
	}
}


/* emit_trap_compiler **********************************************************

   Emit a trap instruction which calls the JIT compiler.

*******************************************************************************/

void emit_trap_compiler(codegendata *cd)
{
	M_ALD_INTERN(REG_METHODPTR, REG_ZERO, EXCEPTION_HARDWARE_COMPILER);
}


/* emit_trap *******************************************************************

   Emit a trap instruction and return the original machine code.

*******************************************************************************/

uint32_t emit_trap(codegendata *cd)
{
	uint32_t mcode;

	/* Get machine code which is patched back in later. The
	   trap is 1 instruction word long. */

	mcode = *((u4 *) cd->mcodeptr);

	M_ALD_INTERN(REG_ZERO, REG_ZERO, EXCEPTION_HARDWARE_PATCHER);

	return mcode;
}


/* emit_verbosecall_enter ******************************************************

   Generates the code for the call trace.

*******************************************************************************/

void emit_verbosecall_enter(jitdata *jd)
{
#if !defined(NDEBUG)
	methodinfo   *m;
	codegendata  *cd;
	registerdata *rd;
	methoddesc   *md;
	int32_t       disp;
	int32_t       i;
	int32_t       s, d;

	if (!JITDATA_HAS_FLAG_VERBOSECALL(jd))
		return;

	/* get required compiler data */

	m  = jd->m;
	cd = jd->cd;
	rd = jd->rd;

	md = m->parseddesc;

	/* mark trace code */

	M_NOP;

	/* On Darwin we need to allocate an additional 3*4 bytes of stack
	   for the arguments to trace_java_call_enter, we make it 2*8. */

	M_MFLR(REG_ZERO);
	M_AST(REG_ZERO, REG_SP, LA_LR_OFFSET);
	M_STWU(REG_SP, REG_SP, -(LA_SIZE + (2 + ARG_CNT + TMP_CNT) * 8));

	/* save argument registers */

	for (i = 0; i < md->paramcount; i++) {
		if (!md->params[i].inmemory) {
			s = md->params[i].regoff;
			d = LA_SIZE + (i + 2) * 8;

			switch (md->paramtypes[i].type) {
			case TYPE_INT:
			case TYPE_ADR:
				M_IST(s, REG_SP, d);
				break;
			case TYPE_LNG:
				M_LST(s, REG_SP, d);
				break;
			case TYPE_FLT:
				M_FST(s, REG_SP, d);
				break;
			case TYPE_DBL:
				M_DST(s, REG_SP, d);
				break;
			}
		}
	}

	/* pass methodinfo and pointers to the tracer function */

	disp = dseg_add_address(cd, m);
	M_ALD(REG_A0, REG_PV, disp);
	M_AADD_IMM(REG_SP, LA_SIZE + 2 * 8, REG_A1);
	M_AADD_IMM(REG_SP, LA_SIZE + (2 + ARG_CNT + TMP_CNT + cd->stackframesize) * 8, REG_A2);
	
	disp = dseg_add_functionptr(cd, trace_java_call_enter);
	M_ALD(REG_ITMP2, REG_PV, disp);
	M_MTCTR(REG_ITMP2);
	M_JSR;

	/* restore argument registers */

	for (i = 0; i < md->paramcount; i++) {
		if (!md->params[i].inmemory) {
			s = LA_SIZE + (i + 2) * 8;
			d = md->params[i].regoff;

			switch (md->paramtypes[i].type) {
			case TYPE_INT:
			case TYPE_ADR:
				M_ILD(d, REG_SP, s);
				break;
			case TYPE_LNG:
				M_LLD(d, REG_SP, s);
				break;
			case TYPE_FLT:
				M_FLD(d, REG_SP, s);
				break;
			case TYPE_DBL:
				M_DLD(d, REG_SP, s);
				break;
			}
		}
	}

	M_ALD(REG_ZERO, REG_SP, LA_SIZE + (2 + ARG_CNT + TMP_CNT) * 8 + LA_LR_OFFSET);
	M_MTLR(REG_ZERO);
	M_LDA(REG_SP, REG_SP, LA_SIZE + (2 + ARG_CNT + TMP_CNT) * 8);

	/* mark trace code */

	M_NOP;
#endif /* !defined(NDEBUG) */
}


/* emit_verbosecall_exit *******************************************************

   Generates the code for the call trace.

*******************************************************************************/

void emit_verbosecall_exit(jitdata *jd)
{
#if !defined(NDEBUG)
	methodinfo   *m;
	codegendata  *cd;
	registerdata *rd;
	methoddesc   *md;
	s4            disp;

	if (!JITDATA_HAS_FLAG_VERBOSECALL(jd))
		return;

	/* get required compiler data */

	m  = jd->m;
	cd = jd->cd;
	rd = jd->rd;

	md = m->parseddesc;
	
	/* mark trace code */

	M_NOP;

	/* On Darwin we need to allocate an additional 2*4 bytes of stack
	   for the arguments to trace_java_call_exit, we make it 1*8. */

	M_MFLR(REG_ZERO);
	M_AST(REG_ZERO, REG_SP, LA_LR_OFFSET);
	M_STWU(REG_SP, REG_SP, -(LA_SIZE + (1 + 1) * 8));

	/* save return value */

	switch (md->returntype.type) {
	case TYPE_INT:
	case TYPE_ADR:
		M_IST(REG_RESULT, REG_SP, LA_SIZE + 1 * 8);
		break;
	case TYPE_LNG:
		M_LST(REG_RESULT_PACKED, REG_SP, LA_SIZE + 1 * 8);
		break;
	case TYPE_FLT:
		M_FST(REG_FRESULT, REG_SP, LA_SIZE + 1 * 8);
		break;
	case TYPE_DBL:
		M_DST(REG_FRESULT, REG_SP, LA_SIZE + 1 * 8);
		break;
	case TYPE_VOID:
		break;
	}

	disp = dseg_add_address(cd, m);
	M_ALD(REG_A0, REG_PV, disp);
	M_AADD_IMM(REG_SP, LA_SIZE + 1 * 8, REG_A1);

	disp = dseg_add_functionptr(cd, trace_java_call_exit);
	M_ALD(REG_ITMP2, REG_PV, disp);
	M_MTCTR(REG_ITMP2);
	M_JSR;

	/* restore return value */

	switch (md->returntype.type) {
	case TYPE_INT:
	case TYPE_ADR:
		M_ILD(REG_RESULT, REG_SP, LA_SIZE + 1 * 8);
		break;
	case TYPE_LNG:
		M_LLD(REG_RESULT_PACKED, REG_SP, LA_SIZE + 1 * 8);
		break;
	case TYPE_FLT:
		M_FLD(REG_FRESULT, REG_SP, LA_SIZE + 1 * 8);
		break;
	case TYPE_DBL:
		M_DLD(REG_FRESULT, REG_SP, LA_SIZE + 1 * 8);
		break;
	case TYPE_VOID:
		break;
	}

	M_ALD(REG_ZERO, REG_SP, LA_SIZE + (1 + 1) * 8 + LA_LR_OFFSET);
	M_MTLR(REG_ZERO);
	M_LDA(REG_SP, REG_SP, LA_SIZE + (1 + 1) * 8);

	/* mark trace code */

	M_NOP;
#endif /* !defined(NDEBUG) */
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
