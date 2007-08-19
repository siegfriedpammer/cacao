/* src/vm/jit/s390/emit.c - s390 code emitter functions

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

   $Id: emit.c 8352 2007-08-19 18:32:59Z pm $

*/

#include "config.h"

#include <assert.h>
#include <stdint.h>

#include "mm/memory.h"
#if defined(ENABLE_THREADS)
# include "threads/native/lock.h"
#endif
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/jit/abi.h"
#include "vm/jit/abi-asm.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/codegen-common.h"
#include "vm/jit/emit-common.h"
#include "vm/jit/jit.h"
#include "vm/jit/patcher-common.h"
#include "vm/jit/replace.h"
#include "vm/jit/s390/codegen.h"
#include "vm/jit/s390/emit.h"
#include "vm/jit/s390/md-abi.h"
#include "vm/types.h"
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


/* emit_store ******************************************************************

   This function generates the code to store the result of an
   operation back into a spilled pseudo-variable.  If the
   pseudo-variable has not been spilled in the first place, this
   function will generate nothing.
    
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
				M_DST(d, REG_SP, dst->vv.regoff);
			else
				M_FST(d, REG_SP, dst->vv.regoff);
		}
		else {
			if (IS_2_WORD_TYPE(dst->type))
				M_LST(d, REG_SP, dst->vv.regoff);
			else
				M_IST(d, REG_SP, dst->vv.regoff);
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
		((src->flags ^ dst->flags) & INMEMORY)) {

		if ((src->type == TYPE_RET) || (dst->type == TYPE_RET)) {
			/* emit nothing, as the value won't be used anyway */
			return;
		}

		/* If one of the variables resides in memory, we can eliminate
		   the register move from/to the temporary register with the
		   order of getting the destination register and the load. */

		if (IS_INMEMORY(src->flags)) {
			if (IS_FLT_DBL_TYPE(dst->type)) {
				d = codegen_reg_of_var(iptr->opc, dst, REG_FTMP1);
			} else {
				if (IS_2_WORD_TYPE(dst->type)) {
					d = codegen_reg_of_var(iptr->opc, dst, REG_ITMP12_PACKED);
				} else {
					d = codegen_reg_of_var(iptr->opc, dst, REG_ITMP1);
				}
			}
			s1 = emit_load(jd, iptr, src, d);
		}
		else {
			if (IS_FLT_DBL_TYPE(src->type)) {
				s1 = emit_load(jd, iptr, src, REG_FTMP1);
			} else {
				if (IS_2_WORD_TYPE(src->type)) {
					s1 = emit_load(jd, iptr, src, REG_ITMP12_PACKED);
				} else {
					s1 = emit_load(jd, iptr, src, REG_ITMP1);
				}
			}
			d = codegen_reg_of_var(iptr->opc, dst, s1);
		}

		if (s1 != d) {
			if (IS_FLT_DBL_TYPE(src->type)) {
				M_FMOV(s1, d);
			} else {
				if (IS_2_WORD_TYPE(src->type)) {
					M_LNGMOVE(s1, d);
				} else {
					M_MOV(s1, d);
				}
			}
		}

		emit_store(jd, iptr, dst, d);
	}
}

/* emit_trap *******************************************************************

   Emit a trap instruction and return the original machine code.

*******************************************************************************/

uint32_t emit_trap(codegendata *cd)
{
	uint32_t mcode;

	/* Get machine code which is patched back in later. The
	   trap is 2 bytes long. */

	mcode = *((u2 *) cd->mcodeptr);

	M_ILL(EXCEPTION_HARDWARE_PATCHER);

	return mcode;
}


/* emit_verbosecall_enter ******************************************************

   Generates the code for the call trace.

*******************************************************************************/

#if !defined(NDEBUG)
#include "vm/jit/trace.h"
void emit_verbosecall_enter(jitdata *jd)
{
#if 1
	methodinfo   *m;
	codegendata  *cd;
	s4            stackframesize;
	s4            i, off, disp;

	m  = jd->m;
	cd = jd->cd;

	/* mark trace code */

	M_NOP;

	/* allocate stack frame */

	stackframesize = 96 + (ARG_CNT * 8) + (TMP_CNT * 8);
	M_ASUB_IMM(stackframesize, REG_SP);

	/* store argument registers in array */

	off = 96;

	for (i = 0; i < INT_ARG_CNT; ++i, off += 8) {
		M_IST(abi_registers_integer_argument[i], REG_SP, off + 4);
		/* high bytes are sign extension */
		M_SRA_IMM(31, abi_registers_integer_argument[i]);
		M_IST(abi_registers_integer_argument[i], REG_SP, off);
	}

	for (i = 0; i < FLT_ARG_CNT; ++i, off += 8) {
		M_DST(abi_registers_float_argument[i], REG_SP, off);
	}
	
	/* save temporary registers for leaf methods */

	if (jd->isleafmethod) {
		for (i = 0; i < INT_TMP_CNT; ++i, off += 8) {
			M_IST(abi_registers_integer_temporary[i], REG_SP, off);
		}

		for (i = 0; i < FLT_TMP_CNT; ++i, off += 8) {
			M_DST(abi_registers_float_temporary[i], REG_SP, off);
		}
	}

	/* load arguments for trace_java_call_enter */

	/* methodinfo */
	disp = dseg_add_address(cd, m);
	M_ALD_DSEG(REG_A0, disp);	
	/* pointer to argument registers array */
	M_LDA(REG_A1, REG_SP, 96);
	/* pointer to on stack arguments */
	M_LDA(REG_A2, REG_SP, stackframesize + (cd->stackframesize * 8));

	/* call trace_java_call_enter */

	disp = dseg_add_functionptr(cd, trace_java_call_enter);
	M_ALD_DSEG(REG_ITMP3, disp);
	M_CALL(REG_ITMP3);

	/* restore argument registers */

	off = 96;

	for (i = 0; i < INT_ARG_CNT; ++i, off += 8) {
		M_ILD(abi_registers_integer_argument[i], REG_SP, off + 4);
	}

	for (i = 0; i < FLT_ARG_CNT; ++i, off += 8) {
		M_DLD(abi_registers_float_argument[i], REG_SP, off);
	}

	/* restore temporary registers for leaf methods */

	if (jd->isleafmethod) {
		for (i = 0; i < INT_TMP_CNT; ++i, off += 8) {
			M_ILD(abi_registers_integer_temporary[i], REG_SP, off);
		}

		for (i = 0; i < FLT_TMP_CNT; ++i, off += 8) {
			M_DLD(abi_registers_float_temporary[i], REG_SP, off);
		}
	}

	/* remove stack frame */

	M_AADD_IMM(stackframesize, REG_SP);

	/* mark trace code */

	M_NOP;

#else
	methodinfo   *m;
	codegendata  *cd;
	methoddesc   *md;
	s4            i, j, k;
	s4            stackframesize, off, foff, aoff, doff, t, iargctr, fargctr, disp;

	/* get required compiler data */

	m  = jd->m;
	cd = jd->cd;

	md = m->parseddesc;

	/* mark trace code */

	M_NOP;

	stackframesize = 
		(6 * 8) + /* s8 on stack parameters x 6 */
		(1 * 4) + /* methodinfo on stack parameter */
		(ARG_CNT * 8) +
		(TMP_CNT * 8) 
		;

	M_ASUB_IMM(stackframesize, REG_SP); /* allocate stackframe */

	/* save argument registers */

	off = (6 * 8) + (1 * 4);

	for (i = 0; i < INT_ARG_CNT; i++, off += 8)
		M_IST(abi_registers_integer_argument[i], REG_SP, off);

	for (i = 0; i < FLT_ARG_CNT; i++, off += 8)
		M_DST(abi_registers_float_argument[i], REG_SP, off);

	/* save temporary registers for leaf methods */

	if (jd->isleafmethod) {
		for (i = 0; i < INT_TMP_CNT; i++, off += 8)
			M_LST(abi_registers_integer_temporary[i], REG_SP, off);

		for (i = 0; i < FLT_TMP_CNT; i++, off += 8)
			M_DST(abi_registers_float_temporary[i], REG_SP, off);
	}

	/* Load arguments to new locations */

	/* First move all arguments to stack
	 *
	 * (s8) a7
	 * (s8) a2
	 *   ...
	 * (s8) a1 \ Auxilliary stack frame
	 * (s8) a0 /
	 * ------- <---- SP
	 */

	M_ASUB_IMM(2 * 8, REG_SP);
	
	/* offset to where first integer arg is saved on stack */
	off = (2 * 8) + (6 * 8) + (1 * 4); 
	/* offset to where first float arg is saved on stack */
	foff = off + (INT_ARG_CNT * 8); 
	/* offset to where first argument is passed on stack */
	aoff = (2 * 8) + stackframesize + (cd->stackframesize * 8);
	/* offset to destination on stack */
	doff = 0; 

	iargctr = fargctr = 0;

	ICONST(REG_ITMP1, 0);

	for (i = 0; i < md->paramcount && i < 8; i++) {
		t = md->paramtypes[i].type;

		M_IST(REG_ITMP1, REG_SP, doff);
		M_IST(REG_ITMP1, REG_SP, doff + 4);

		if (IS_FLT_DBL_TYPE(t)) {
			if (fargctr < 2) { /* passed in register */
				N_STD(abi_registers_float_argument[fargctr], doff, RN, REG_SP);
				fargctr += 1;
			} else { /* passed on stack */
				/*
				if (IS_2_WORD_TYPE(t)) {
					N_MVC(doff, 8, REG_SP, aoff, REG_SP);
				} else {
					N_MVC(doff + 4, 4, REG_SP, aoff, REG_SP);
				}
				*/
				N_MVC(doff, 8, REG_SP, aoff, REG_SP);
				aoff += 8;
			}
		} else {
			if (IS_2_WORD_TYPE(t)) {
				if (iargctr < 4) { /* passed in 2 registers */
					N_STM(REG_A0 + iargctr, REG_A0 + iargctr + 1, doff, REG_SP);
					iargctr += 2;
				} else { /* passed on stack */
					N_MVC(doff, 8, REG_SP, aoff, REG_SP);
					aoff += 8;
				}
			} else {
				if (iargctr < 5) { /* passed in register */
					N_ST(REG_A0 + iargctr, doff + 4, RN, REG_SP);
					iargctr += 1;
				} else { /* passed on stack */
					N_MVC(doff + 4, 4, REG_SP, aoff, REG_SP);
					aoff += 8;
				}
			}
		}

		doff += 8;
	}

	/* Now move a0 and a1 to registers
	 *
	 * (s8) a7
	 *   ...
	 * (s8) a2
	 * ------- <- SP
	 * (s8) a0 ==> a0, a1
	 * (s8) a1 ==> a2, a3
	 */

	N_LM(REG_A0, REG_A1, 0, REG_SP);
	N_LM(REG_A2, REG_A3, 8, REG_SP);

	M_AADD_IMM(2 * 8, REG_SP);

	/* Finally load methodinfo argument */

	disp = dseg_add_address(cd, m);
	M_ALD_DSEG(REG_ITMP2, disp);	
	M_AST(REG_ITMP2, REG_SP, 6 * 8);

	/* Call builtin_verbosecall_enter */

	disp = dseg_add_address(cd, builtin_verbosecall_enter);
	M_ALD_DSEG(REG_ITMP2, disp);
	M_ASUB_IMM(96, REG_SP);
	M_CALL(REG_ITMP2);
	M_AADD_IMM(96, REG_SP);

	/* restore argument registers */

	off = (6 * 8) + (1 * 4);

	for (i = 0; i < INT_ARG_CNT; i++, off += 8)
		M_ILD(abi_registers_integer_argument[i], REG_SP, off);

	for (i = 0; i < FLT_ARG_CNT; i++, off += 8)
		M_DLD(abi_registers_float_argument[i], REG_SP, off);

	/* restore temporary registers for leaf methods */

	if (jd->isleafmethod) {
		for (i = 0; i < INT_TMP_CNT; i++, off += 8)
			M_ILD(abi_registers_integer_temporary[i], REG_SP, off);

		for (i = 0; i < FLT_TMP_CNT; i++, off += 8)
			M_DLD(abi_registers_float_temporary[i], REG_SP, off);
	}

	/* remove stackframe */

	M_AADD_IMM(stackframesize, REG_SP);

	/* mark trace code */

	M_NOP;
#endif
}
#endif /* !defined(NDEBUG) */


/* emit_verbosecall_exit *******************************************************

   Generates the code for the call trace.

*******************************************************************************/

#if !defined(NDEBUG)
void emit_verbosecall_exit(jitdata *jd)
{
#if 1
	methodinfo   *m;
	codegendata  *cd;
	s4            disp;
	s4            stackframesize;

	m  = jd->m;
	cd = jd->cd;

	/* mark trace code */

	M_NOP;

	/* allocate stackframe */

	stackframesize = 96 + (3 * 8);
	M_ASUB_IMM(stackframesize, REG_SP);

	/* store return values in array and sign extend them */

	M_IST(REG_RESULT, REG_SP, 96 + (0 * 8) + 4);
	M_SRA_IMM(31, REG_RESULT);
	M_IST(REG_RESULT, REG_SP, 96 + (0 * 8));

	M_IST(REG_RESULT2, REG_SP, 96 + (1 * 8) + 4);
	M_SRA_IMM(31, REG_RESULT2);
	M_IST(REG_RESULT2, REG_SP, 96 + (1 * 8));

	M_DST(REG_FRESULT, REG_SP, 96 + (2 * 8));

	/* call trace_java_call_exit */

	disp = dseg_add_address(cd, m);
	M_ALD_DSEG(REG_A0, disp);
	M_LDA(REG_A1, REG_SP, 96);
	disp = dseg_add_functionptr(cd, trace_java_call_exit);
	M_ALD_DSEG(REG_ITMP3, disp);
	M_CALL(REG_ITMP3);

	/* restore return values */

	M_ILD(REG_RESULT, REG_SP, 96 + (0 * 8) + 4);
	M_ILD(REG_RESULT2, REG_SP, 96 + (1 * 8) + 4);
	M_DLD(REG_FRESULT, REG_SP, 96 + (2 * 8));

	/* remove stackframe */

	M_AADD_IMM(stackframesize, REG_SP);

	/* mark trace code */

	M_NOP;

#else

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

	M_ASUB_IMM(2 * 8, REG_SP);

	N_STM(REG_RESULT, REG_RESULT2, 0 * 8, REG_SP);
	M_DST(REG_FRESULT, REG_SP, 1 * 8);

	if (IS_2_WORD_TYPE(m->parseddesc->returntype.type)) {
		/* (REG_A0, REG_A1) == (REG_RESULT, REG_RESULT2), se no need to move */
	} else {
		M_INTMOVE(REG_RESULT, REG_A1);
		ICONST(REG_A0, 0);
	}

	disp = dseg_add_address(cd, m);
	M_ALD_DSEG(REG_A2, disp);

	/* REG_FRESULT is REG_FA0, so no need to move */
	M_FLTMOVE(REG_FRESULT, REG_FA1);

	disp = dseg_add_address(cd, builtin_verbosecall_exit);
	M_ALD_DSEG(REG_ITMP1, disp);
	M_ASUB_IMM(96, REG_SP);
	M_CALL(REG_ITMP1);
	M_AADD_IMM(96, REG_SP);

	N_LM(REG_RESULT, REG_RESULT2, 0 * 8, REG_SP);
	M_DLD(REG_FRESULT, REG_SP, 1 * 8);

	M_AADD_IMM(2 * 8, REG_SP);

	/* mark trace code */

	M_NOP;
#endif
}
#endif /* !defined(NDEBUG) */


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

s4 emit_load_s1_notzero(jitdata *jd, instruction *iptr, s4 tempreg) {
	codegendata *cd = jd->cd;
	s4 reg = emit_load_s1(jd, iptr, tempreg);
	if (reg == 0) {
		M_MOV(reg, tempreg);
		return tempreg;
	} else {
		return reg;
	}
}

s4 emit_load_s2_notzero(jitdata *jd, instruction *iptr, s4 tempreg) {
	codegendata *cd = jd->cd;
	s4 reg = emit_load_s2(jd, iptr, tempreg);
	if (reg == 0) {
		if (IS_FLT_DBL_TYPE(VAROP(iptr->sx.s23.s2)->type)) {
			M_FMOV(reg, tempreg);
		} else {
			M_MOV(reg, tempreg);
		}
		return tempreg;
	} else {
		return reg;
	}
}

s4 emit_load_s1_but(jitdata *jd, instruction *iptr, s4 tempreg, s4 notreg) {
	codegendata *cd = jd->cd;
	s4 reg = emit_load_s1(jd, iptr, tempreg);
	if (reg == notreg) {
		if (IS_FLT_DBL_TYPE(VAROP(iptr->s1)->type)) {
			M_FMOV(reg, tempreg);
		} else {
			M_MOV(reg, tempreg);
		}
		return tempreg;
	} else {
		return reg;
	}
}

s4 emit_load_s2_but(jitdata *jd, instruction *iptr, s4 tempreg, s4 notreg) {
	codegendata *cd = jd->cd;
	s4 reg = emit_load_s2(jd, iptr, tempreg);
	if (reg == notreg) {
		if (IS_FLT_DBL_TYPE(VAROP(iptr->sx.s23.s2)->type)) {
			M_FMOV(reg, tempreg);
		} else {
			M_MOV(reg, tempreg);
		}
		return tempreg;
	} else {
		return reg;
	}
}

s4 emit_alloc_dst_even_odd(jitdata *jd, instruction *iptr, s4 htmpreg, s4 ltmpreg, s4 breg) {
	codegendata *cd;
	s4           hr, lr;
	varinfo     *dst;

	/* (r0, r1)    
	 * (r2, r3)
	 * (r4, r5)
	 * (r6, r7)
	 * (r8, r9)
	 * (r10, r11)
	 * (r12, r13) Illegal, because r13 is PV
	 * (r14, r15) Illegal, because r15 is SP
	 */

	cd = jd->cd;
	dst = VAROP(iptr->dst);

	if (IS_INMEMORY(dst->flags)) {
		if (! IS_REG_ITMP(ltmpreg)) {
			M_INTMOVE(ltmpreg, breg);
		}
		if (! IS_REG_ITMP(htmpreg)) {
			M_INTMOVE(htmpreg, breg);
		}
		return PACK_REGS(ltmpreg, htmpreg);
	} else {
		hr = GET_HIGH_REG(dst->vv.regoff);
		lr = GET_LOW_REG(dst->vv.regoff);
		if (((hr % 2) == 0) && lr == (hr + 1)) {
			/* the result is already in a even-odd pair */
			return dst->vv.regoff;			
		} else if (((hr % 2) == 0) && (hr < R12)) {
			/* the high register is at a even position */
			M_INTMOVE(hr + 1, breg);
			return PACK_REGS(hr + 1, hr);
		} else if (((lr % 2) == 1) && (lr < R12)) {
			/* the low register is at a odd position */
			M_INTMOVE(lr - 1, breg);
			return PACK_REGS(lr, lr - 1);
		} else {
			/* no way to create an even-odd pair by 1 copy operation,
			 * Use the temporary register pair.
			 */
			if (! IS_REG_ITMP(ltmpreg)) {
				M_INTMOVE(ltmpreg, breg);
			}
			if (! IS_REG_ITMP(htmpreg)) {
				M_INTMOVE(htmpreg, breg);
			}
			return PACK_REGS(ltmpreg, htmpreg);
		}
	}
}

void emit_restore_dst_even_odd(jitdata *jd, instruction *iptr, s4 htmpreg, s4 ltmpreg, s4 breg) {
	codegendata *cd;
	s4           hr, lr;
	varinfo     *dst;

	cd = jd->cd;
	dst = VAROP(iptr->dst);

	if (IS_INMEMORY(dst->flags)) {
		if (! IS_REG_ITMP(ltmpreg)) {
			M_INTMOVE(breg, ltmpreg);
		}
		if (! IS_REG_ITMP(htmpreg)) {
			M_INTMOVE(breg, htmpreg);
		}
	} else {
		hr = GET_HIGH_REG(dst->vv.regoff);
		lr = GET_LOW_REG(dst->vv.regoff);
		if (((hr % 2) == 0) && lr == (hr + 1)) {
			return;
		} else if (((hr % 2) == 0) && (hr < R12)) {
			M_INTMOVE(breg, hr + 1);
		} else if (((lr % 2) == 1) && (lr < R12)) {
			M_INTMOVE(breg, lr - 1);
		} else {
			if (! IS_REG_ITMP(ltmpreg)) {
				M_INTMOVE(breg, ltmpreg);
			}
			if (! IS_REG_ITMP(htmpreg)) {
				M_INTMOVE(breg, htmpreg);
			}
		}
	}
}

void emit_copy_dst(jitdata *jd, instruction *iptr, s4 dtmpreg) {
	codegendata *cd;
	varinfo *dst;
	cd = jd->cd;
	dst = VAROP(iptr->dst);
	if (! IS_INMEMORY(dst->flags)) {
		if (dst->vv.regoff != dtmpreg) {
			if (IS_FLT_DBL_TYPE(dst->type)) {
				M_FLTMOVE(dtmpreg, dst->vv.regoff);
			} else if (IS_2_WORD_TYPE(dst->type)) {
				M_LNGMOVE(dtmpreg, dst->vv.regoff);
			} else {
				M_INTMOVE(dtmpreg, dst->vv.regoff);
			}
		}
	}
}

void emit_branch(codegendata *cd, s4 disp, s4 condition, s4 reg, u4 opt) {

	s4 branchdisp = disp;
	s4 branchmpc;
	u1 *ref;

	if (N_VALID_BRANCH(branchdisp)) {

		/* valid displacement */

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
			case BRANCH_UNCONDITIONAL:
				M_BR(branchdisp);
				break;
			default:
				vm_abort("emit_branch: unknown condition %d", condition);
		}
	} else {

		/* If LONGBRANCHES is not set, the flag and the error flag */

		if (!CODEGENDATA_HAS_FLAG_LONGBRANCHES(cd)) {
			cd->flags |= (CODEGENDATA_FLAG_ERROR |
				CODEGENDATA_FLAG_LONGBRANCHES);
		}

		/* If error flag is set, do nothing. The method has to be recompiled. */

		if (CODEGENDATA_HAS_FLAG_LONGBRANCHES(cd) && CODEGENDATA_HAS_FLAG_ERROR(cd)) {
			return;
		}

		/* Patch the displacement to branch over the actual branch manually
		 * to not get yet more nops.
		 */

		branchmpc = cd->mcodeptr - cd->mcodebase;
		ref = cd->mcodeptr;

		switch (condition) {
			case BRANCH_EQ:
				M_BNE(0);
				break;
			case BRANCH_NE:
				M_BEQ(0);
				break;
			case BRANCH_LT:
				M_BGE(0);
				break;
			case BRANCH_GE:
				M_BLT(0);
				break;
			case BRANCH_GT:
				M_BLE(0);
				break;
			case BRANCH_LE:
				M_BGT(0);
				break;
			case BRANCH_UNCONDITIONAL:
				/* fall through, no displacement to patch */
				ref = NULL;
				break;
			default:
				vm_abort("emit_branch: unknown condition %d", condition);
		}

		/* The actual long branch */

		disp = dseg_add_s4(cd, branchmpc + disp - N_PV_OFFSET);
		M_ILD_DSEG(REG_ITMP3, disp);
		M_AADD(REG_PV, REG_ITMP3);
		M_JMP(RN, REG_ITMP3);

		/* Patch back the displacement */

		if (ref != NULL) {
			*(u4 *)ref |= (u4)((cd->mcodeptr - ref) / 2);
		}
	}
}

void emit_arithmetic_check(codegendata *cd, instruction *iptr, s4 reg) {
	if (INSTRUCTION_MUST_CHECK(iptr)) {
		M_TEST(reg);
		M_BNE(SZ_BRC + SZ_ILL);
		M_ILL(EXCEPTION_HARDWARE_ARITHMETIC);
	}
}

/* emit_arrayindexoutofbounds_check ********************************************

   Emit a ArrayIndexOutOfBoundsException check.

*******************************************************************************/

void emit_arrayindexoutofbounds_check(codegendata *cd, instruction *iptr, s4 s1, s4 s2)
{
	if (INSTRUCTION_MUST_CHECK(iptr)) {
		/* Size is s4, >= 0
		 * Do unsigned comparison to catch negative indexes.
		 */
		N_CL(s2, OFFSET(java_array_t, size), RN, s1);
        M_BLT(SZ_BRC + SZ_ILL);
		M_ILL2(s2, EXCEPTION_HARDWARE_ARRAYINDEXOUTOFBOUNDS);
	}
}

void emit_classcast_check(codegendata *cd, instruction *iptr, s4 condition, s4 reg, s4 s1) {
	if (INSTRUCTION_MUST_CHECK(iptr)) {
		if (reg != RN) {
			M_TEST(reg);
		}
		switch (condition) {
			case BRANCH_LE:
				M_BGT(SZ_BRC + SZ_ILL);
				break;
			case BRANCH_EQ:
				M_BNE(SZ_BRC + SZ_ILL);
				break;
			case BRANCH_GT:
				M_BLE(SZ_BRC + SZ_ILL);
				break;
			default:
				vm_abort("emit_classcast_check: unknown condition %d", condition);
		}
		M_ILL2(s1, EXCEPTION_HARDWARE_CLASSCAST);
	}
}

void emit_nullpointer_check(codegendata *cd, instruction *iptr, s4 reg) {
	if (INSTRUCTION_MUST_CHECK(iptr)) {
		M_TEST(reg);
		M_BNE(SZ_BRC + SZ_ILL);
		M_ILL(EXCEPTION_HARDWARE_NULLPOINTER);
	}
}

void emit_exception_check(codegendata *cd, instruction *iptr) {
	if (INSTRUCTION_MUST_CHECK(iptr)) {
		M_TEST(REG_RESULT);
		M_BNE(SZ_BRC + SZ_ILL);
		M_ILL(EXCEPTION_HARDWARE_EXCEPTION);
	}
}

void emit_restore_pv(codegendata *cd) {
	s4 offset, offset_imm;

	/*
	N_BASR(REG_PV, RN);
	disp = (s4) (cd->mcodeptr - cd->mcodebase);
	M_ASUB_IMM32(disp, REG_ITMP1, REG_PV);
	*/

	/* If the offset from the method start does not fit into an immediate
	 * value, we can't put it into the data segment!
	 */

	/* Displacement from start of method to here */

	offset = (s4) (cd->mcodeptr - cd->mcodebase);
	offset_imm = -offset - SZ_BASR + N_PV_OFFSET;

	if (N_VALID_IMM(offset_imm)) {
		/* Get program counter */
		N_BASR(REG_PV, RN);
		/* Substract displacement */
		M_AADD_IMM(offset_imm, REG_PV);
	} else {
		/* Save program counter and jump over displacement in instruction flow */
		N_BRAS(REG_PV, SZ_BRAS + SZ_LONG);
		/* Place displacement here */
		/* REG_PV points now exactly to this position */
		N_LONG(-offset - SZ_BRAS + N_PV_OFFSET);
		/* Substract *(REG_PV) from REG_PV */
		N_A(REG_PV, 0, RN, REG_PV);
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
 */
