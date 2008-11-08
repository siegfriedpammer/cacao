/* src/vm/jit/m68k/emit.c

   Copyright (C) 1996-2005, 2006, 2007, 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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

#include "vm/jit/m68k/codegen.h"
#include "vm/jit/m68k/emit.h"

#include "mm/memory.hpp"

#include "vm/jit/builtin.hpp"

#include "vm/jit/asmpart.h"
#include "vm/jit/emit-common.hpp"
#include "vm/jit/trace.hpp"
#include "vm/jit/trap.h"


/* emit_mov_imm_reg **************************************************************************
 *
 *	Loads an immededat operand into an integer data register
 *
 ********************************************************************************************/
void emit_mov_imm_reg (codegendata *cd, s4 imm, s4 dreg)
{
	/* FIXME: -1 can be used as byte form 0xff, but this ifs cascade is plain wrong it seems */

	if ( (imm & 0x0000007F) == imm)	{
		/* use byte form */
		*((s2*)cd->mcodeptr) = 0x7000 | (dreg << 9) | imm;	/* MOVEQ.L */
		cd->mcodeptr += 2;
	} else if ((imm  & 0x00007FFF) == imm)	{
		/* use word form */
		OPWORD( ((7<<6) | (dreg << 3) | 5), 7, 4);			/* MVS.W */
		*((s2*)cd->mcodeptr) = (s2)imm;
		cd->mcodeptr += 2;
	} else {
		/* use long form */
		OPWORD( ((2<<6) | (dreg << 3) | 0), 7, 4);
		*((s4*)cd->mcodeptr) = (s4)imm;
		cd->mcodeptr += 4;

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
			switch(src->type)	{
			case TYPE_INT: M_INTMOVE(s1, d); break;
			case TYPE_ADR: M_ADRMOVE(s1, d); break;
			case TYPE_LNG: M_LNGMOVE(s1, d); break;
#if !defined(ENABLE_SOFTFLOAT)
			case TYPE_FLT: M_FLTMOVE(s1, d); break;
			case TYPE_DBL: M_DBLMOVE(s1, d); break;
#else
			case TYPE_FLT: M_INTMOVE(s1, d); break;
			case TYPE_DBL: M_LNGMOVE(s1, d); break;
#endif
			default:
				vm_abort("emit_copy: unknown type %d", src->type);
			}
		}

		emit_store(jd, iptr, dst, d);
	}
}


/* emit_store ******************************************************************

   Emits a possible store of the destination operand.

*******************************************************************************/

void emit_store(jitdata *jd, instruction *iptr, varinfo *dst, s4 d)
{
	codegendata  *cd;

	/* get required compiler data */

	cd = jd->cd;

	if (IS_INMEMORY(dst->flags)) {
		COUNT_SPILLS;
	
		switch(dst->type)	{
#if defined(ENABLE_SOFTFLOAT)
			case TYPE_DBL:
#endif
			case TYPE_LNG:
				M_LST(d, REG_SP, dst->vv.regoff);
				break;
#if defined(ENABLE_SOFTFLOAT)
			case TYPE_FLT:
#endif
			case TYPE_INT:
				M_IST(d, REG_SP, dst->vv.regoff);
				break;
			case TYPE_ADR:
				M_AST(d, REG_SP, dst->vv.regoff);
				break;
#if !defined(ENABLE_SOFTFLOAT)
			case TYPE_DBL:
				M_DST(d, REG_SP, dst->vv.regoff);
				break;
			case TYPE_FLT:
				M_FST(d, REG_SP, dst->vv.regoff);
				break;
#endif
			default:
				vm_abort("emit_store: unknown type %d", dst->type);
		}
	}
}


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
	
		switch (src->type)	{
#if defined(ENABLE_SOFTFLOAT)
			case TYPE_FLT:
#endif
			case TYPE_INT: 
				M_ILD(tempreg, REG_SP, disp);
				break;
#if defined(ENABLE_SOFTFLOAT)
			case TYPE_DBL:
#endif
			case TYPE_LNG:
				M_LLD(tempreg, REG_SP, disp);
				break;
			case TYPE_ADR:
				M_ALD(tempreg, REG_SP, disp);
				break;
#if !defined(ENABLE_SOFTFLOAT)
			case TYPE_FLT:
				M_FLD(tempreg, REG_SP, disp);
				break;
			case TYPE_DBL:
				M_DLD(tempreg, REG_SP, disp);
				break;
#endif
			default:
				vm_abort("emit_load: unknown type %d", src->type);
		}
		#if 0
		if (IS_FLT_DBL_TYPE(src->type)) {
			if (IS_2_WORD_TYPE(src->type)) {
				M_DLD(tempreg, REG_SP, disp);
			 } else {
				M_FLD(tempreg, REG_SP, disp);
			}
		} else {
			if (IS_2_WORD_TYPE(src->type)) {
				M_LLD(tempreg, REG_SP, disp);
			} else {
				M_ILD(tempreg, REG_SP, disp);
			}
		}
		#endif

		reg = tempreg;
	}
	else
		reg = src->vv.regoff;

	return reg;
}

s4 emit_load_low(jitdata *jd, instruction *iptr, varinfo *src, s4 tempreg) 
{
	codegendata  *cd;
	s4            disp;
	s4            reg;

#if !defined(ENABLE_SOFTFLOAT)
	assert(src->type == TYPE_LNG);
#else
	assert(src->type == TYPE_LNG || src->type == TYPE_DBL);
#endif

	/* get required compiler data */
	cd = jd->cd;

	if (IS_INMEMORY(src->flags)) {
		COUNT_SPILLS;

		disp = src->vv.regoff;
		M_ILD(tempreg, REG_SP, disp + 4);
		reg = tempreg;
	} else {
		reg = GET_LOW_REG(src->vv.regoff);
	}
	return reg;
}
s4 emit_load_high(jitdata *jd, instruction *iptr, varinfo *src, s4 tempreg)
{
	codegendata  *cd;
	s4            disp;
	s4            reg;

#if !defined(ENABLE_SOFTFLOAT)
	assert(src->type == TYPE_LNG);
#else
	assert(src->type == TYPE_LNG || src->type == TYPE_DBL);
#endif
	/* get required compiler data */
	cd = jd->cd;

	if (IS_INMEMORY(src->flags)) {
		COUNT_SPILLS;
		disp = src->vv.regoff;
		M_ILD(tempreg, REG_SP, disp);
		reg = tempreg;
	} else {
		reg = GET_HIGH_REG(src->vv.regoff);
	}
	return reg;
}
/* emit_branch *****************************************************************

   Emits the code for conditional and unconditional branchs.

*******************************************************************************/
void emit_branch(codegendata *cd, s4 disp, s4 condition, s4 reg, u4 opt) 
{ 
	/* calculate the different displacements */
	/* PC is a at branch instruction + 2 */
	/* coditional and uncondition branching work the same way */
	/* short branches have signed 16 bit offset */
	/* long branches are signed 32 bit */
	/* the 8 bit offset branching instructions are not used */

	disp  =  disp - 2;

	/* check displacement for overflow */
	if ((disp & 0x0000FFFF) != disp)	{
		if (!CODEGENDATA_HAS_FLAG_LONGBRANCHES(cd)) {
			cd->flags |= (CODEGENDATA_FLAG_ERROR | CODEGENDATA_FLAG_LONGBRANCHES);
		}
	}

	/* check which branch to generate */

	if (condition == BRANCH_UNCONDITIONAL) {
		if (CODEGENDATA_HAS_FLAG_LONGBRANCHES(cd))	{
			M_BR_32(disp);
		} else	{
			M_BR_16(disp);
		}
	} else {
		if (CODEGENDATA_HAS_FLAG_LONGBRANCHES(cd)) {
			switch (condition) {
			case BRANCH_EQ:
				M_BEQ_32(disp);
				break;
			case BRANCH_NE:
				M_BNE_32(disp);
				break;
			case BRANCH_LT:
				M_BLT_32(disp);
				break;
			case BRANCH_GE:
				M_BGE_32(disp);
				break;
			case BRANCH_GT:
				M_BGT_32(disp);
				break;
			case BRANCH_LE:
				M_BLE_32(disp);
				break;
			case BRANCH_NAN:
				M_BNAN_32(disp);
				break;
			case BRANCH_UGT:
				M_BHI_32(disp);
				break;

			default:
				vm_abort("emit_branch: unknown condition %d", condition);
			}
		} else {
			switch (condition) {
			case BRANCH_EQ:
				M_BEQ_16(disp);
				break;
			case BRANCH_NE:
				M_BNE_16(disp);
				break;
			case BRANCH_LT:
				M_BLT_16(disp);
				break;
			case BRANCH_GE:
				M_BGE_16(disp);
				break;
			case BRANCH_GT:
				M_BGT_16(disp);
				break;
			case BRANCH_LE:
				M_BLE_16(disp);
				break;
			case BRANCH_NAN:
				M_BNAN_16(disp);
				break;
			case BRANCH_UGT:
				M_BHI_16(disp);
				break;
			default:
				vm_abort("emit_branch: unknown condition %d", condition);
			}
		}
	}
}


#if !defined(NDEBUG)
/*
 *	Trace functions. Implement -verbose:call flag
 *	code marked by real NOP, but performance is no matter when using -verbose:call :)
 */
void emit_verbosecall_enter(jitdata* jd) 
{ 
	methodinfo   *m;
	codegendata  *cd;
	registerdata *rd;
	methoddesc   *md;

	if (!JITDATA_HAS_FLAG_VERBOSECALL(jd))
		return;
	
	/* get required compiler data */
	m  = jd->m;
	cd = jd->cd;
	rd = jd->rd;
	md = m->parseddesc;

	/* mark trace code */
	M_NOP;

	M_IPUSH(REG_D0);
	M_IPUSH(REG_D1);
	M_APUSH(REG_A0);
	M_APUSH(REG_A1);
	M_AMOV(REG_SP, REG_A0);	/* simpyfy stack offset calculation */

#if !defined(ENABLE_SOFTFLOAT)
	M_AADD_IMM(-8*2, REG_SP);
	M_FSTORE(REG_F0, REG_SP, 8);
	M_FSTORE(REG_F1, REG_SP, 0);
#endif
	
	M_AADD_IMM(4*4 + 4, REG_A0);
	M_APUSH(REG_A0);		/* third arg is original argument stack */
	M_IPUSH_IMM(0);			/* second arg is number of argument registers (=0) */
	M_IPUSH_IMM(m);			/* first arg is methodpointer */
	
	M_JSR_IMM(trace_java_call_enter);
	/* pop arguments off stack */
	M_AADD_IMM(3*4, REG_SP);

#if !defined(ENABLE_SOFTFLOAT)
	M_FSTORE(REG_F1, REG_SP, 0);
	M_FSTORE(REG_F0, REG_SP, 8);
	M_AADD_IMM(8*2, REG_SP);
#endif

	M_APOP(REG_A1);
	M_APOP(REG_A0);
	M_IPOP(REG_D1);
	M_IPOP(REG_D0);

	M_NOP;
}
void emit_verbosecall_exit(jitdata* jd) 
{ 
	methodinfo   *m;
	codegendata  *cd;
	registerdata *rd;
	methoddesc   *md;

	if (!JITDATA_HAS_FLAG_VERBOSECALL(jd))
		return;

	/* get required compiler data */
	m  = jd->m;
	cd = jd->cd;
	rd = jd->rd;
	md = m->parseddesc;

	/* void builtin_verbosecall_exit(s8 l, double d, float f, methodinfo *m); */

    /* void trace_java_call_exit(methodinfo *m, uint64_t *return_regs) */

	/* mark trace code */
	M_NOP;

	/* to store result on stack */
	M_AADD_IMM(-8, REG_SP);			/* create space for array */

	switch (md->returntype.type)	{
		case TYPE_ADR:
		case TYPE_INT:
	#if defined(ENABLE_SOFTFLOAT)
		case TYPE_FLT:
	#endif
			M_IST(REG_D0, REG_SP, 0);
			break;

		case TYPE_LNG:
	#if defined(ENABLE_SOFTFLOAT)
		case TYPE_DBL:
	#endif
			M_LST(REG_D1, REG_SP, 0);
			break;

	#if !defined(ENABLE_SOFTFLOAT)
		case TYPE_FLT:	/* FIXME */
		case TYPE_DBL:  /* FIXME */
	#endif

		case TYPE_VOID: /* nothing */
			break;

		default:
			assert(0);
	}

	M_APUSH(REG_SP);				/* push address of array */
	M_IPUSH_IMM(m);					/* push methodinfo */
	M_JSR_IMM(trace_java_call_exit);

	M_AADD_IMM(8, REG_SP);			/* remove args */
	/* poping result registers from stack */
	switch (md->returntype.type)	{
		case TYPE_ADR:
		case TYPE_INT:
	#if defined(ENABLE_SOFTFLOAT)
		case TYPE_FLT:
	#endif
			M_ILD(REG_D0, REG_SP, 0);
			break;

		case TYPE_LNG:
	#if defined(ENABLE_SOFTFLOAT)
		case TYPE_DBL:
	#endif
			M_LLD(REG_D1, REG_SP, 0);
			break;

	#if !defined(ENABLE_SOFTFLOAT)
		case TYPE_FLT:	/* FIXME */
		case TYPE_DBL:  /* FIXME */
	#endif

		case TYPE_VOID: /* nothing */
			break;

		default:
			assert(0);
	}

	M_AADD_IMM(8, REG_SP);			/* remove space for array */

	M_NOP;
}
#endif

/* emit_classcast_check ********************************************************

   Emit a ClassCastException check.

*******************************************************************************/

void emit_classcast_check(codegendata *cd, instruction *iptr, s4 condition, s4 reg, s4 s1)
{
	if (INSTRUCTION_MUST_CHECK(iptr)) {
		switch (condition) {
		case BRANCH_LE:
			M_BGT(4);
			break;
		case BRANCH_EQ:
			M_BNE(4);
			break;
		case BRANCH_GT:
			M_BLE(4);
			break;
		case BRANCH_UGT:
			M_BLS(4);
			break;
		default:
			vm_abort("emit_classcast_check: unknown condition %d", condition);
		}
		M_TRAP_SETREGISTER(s1);
		M_TRAP(TRAP_ClassCastException);
	}
}

/* emit_arrayindexoutofbounds_check ********************************************

   Emit a ArrayIndexOutOfBoundsException check.

*******************************************************************************/
void emit_arrayindexoutofbounds_check(codegendata *cd, instruction *iptr, s4 s1, s4 s2)
{
	if (INSTRUCTION_MUST_CHECK(iptr)) {
		M_ILD(REG_ITMP3, s1, OFFSET(java_array_t, size));
		M_ICMP(s2, REG_ITMP3);
		M_BHI(4);
		M_TRAP_SETREGISTER(s2);
		M_TRAP(TRAP_ArrayIndexOutOfBoundsException);
	}
}


/* emit_arraystore_check *******************************************************

   Emit an ArrayStoreException check.

*******************************************************************************/

void emit_arraystore_check(codegendata *cd, instruction *iptr)
{
	if (INSTRUCTION_MUST_CHECK(iptr)) {
		M_ITST(REG_RESULT);
		M_BNE(2);
		M_TRAP(TRAP_ArrayStoreException);
	}
}


/* emit_nullpointer_check ******************************************************

   Emit a NullPointerException check.

*******************************************************************************/

void emit_nullpointer_check(codegendata *cd, instruction *iptr, s4 reg)
{
	if (INSTRUCTION_MUST_CHECK(iptr)) {
		/* XXX: this check is copied to call monitor_enter 
		 * invocation at the beginning of codegen.c */
		M_ATST(reg);
		M_BNE(2);
		M_TRAP(TRAP_NullPointerException);
	}
}

/* emit_arithmetic_check *******************************************************

   Emit an ArithmeticException check.

*******************************************************************************/

void emit_arithmetic_check(codegendata *cd, instruction *iptr, s4 reg)
{
	if (INSTRUCTION_MUST_CHECK(iptr)) {
		M_ITST(reg);
		M_BNE(2);
		M_TRAP(TRAP_ArithmeticException);
	}
}

/* emit_exception_check_ireg **************************************************

   Emit an Exception check. Teste register is integer REG_RESULT

*******************************************************************************/
void emit_exception_check(codegendata *cd, instruction *iptr)
{
	if (INSTRUCTION_MUST_CHECK(iptr)) {
		M_ITST(REG_RESULT);
		M_BNE(2);
		M_TRAP(TRAP_CHECK_EXCEPTION);
	}
}

/* emit_trap_compiler **********************************************************

   Emit a trap instruction which calls the JIT compiler.

*******************************************************************************/

void emit_trap_compiler(codegendata *cd)
{
	M_TRAP_SETREGISTER(REG_METHODPTR);
	M_TRAP(TRAP_COMPILER);
}


/* emit_trap *******************************************************************

   Emit a trap instruction and return the original machine code.

*******************************************************************************/

uint32_t emit_trap(codegendata *cd)
{
	uint32_t mcode;

	/* Get machine code which is patched back in later. The
	   trap is 2 bytes long. */

	mcode = *((uint32_t *) cd->mcodeptr);

	M_TRAP(TRAP_PATCHER);

	return mcode;
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
