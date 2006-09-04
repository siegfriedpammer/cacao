/* src/vm/jit/x86_64/emit.c - x86_64 code emitter functions

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

   $Id: emit.c 5288 2006-09-04 15:48:16Z twisti $

*/

#include "config.h"

#include "vm/types.h"

#include "md-abi.h"

#include "vm/jit/x86_64/codegen.h"
#include "vm/jit/x86_64/md-emit.h"

#if defined(ENABLE_THREADS)
# include "threads/native/lock.h"
#endif

#include "vm/builtin.h"
#include "vm/jit/abi-asm.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/codegen-common.h"
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
	s4            disp;
	s4            reg;

	/* get required compiler data */

	cd = jd->cd;

	if (src->flags & INMEMORY) {
		COUNT_SPILLS;

		disp = src->regoff * 8;

		if (IS_FLT_DBL_TYPE(src->type)) {
			if (IS_2_WORD_TYPE(src->type))
				M_DLD(tempreg, REG_SP, disp);
			else
				M_FLD(tempreg, REG_SP, disp);

		} else {
			if (IS_INT_TYPE(src->type))
				M_ILD(tempreg, REG_SP, disp);
			else
				M_LLD(tempreg, REG_SP, disp);
		}

		reg = tempreg;
	} else
		reg = src->regoff;

	return reg;
}


/* emit_load_s1 ****************************************************************

   Emits a possible load of the first source operand.

*******************************************************************************/

s4 emit_load_s1(jitdata *jd, instruction *iptr, stackptr src, s4 tempreg)
{
	codegendata  *cd;
	s4            disp;
	s4            reg;

	/* get required compiler data */

	cd = jd->cd;

	if (src->flags & INMEMORY) {
		COUNT_SPILLS;

		disp = src->regoff * 8;

		if (IS_FLT_DBL_TYPE(src->type)) {
			if (IS_2_WORD_TYPE(src->type))
				M_DLD(tempreg, REG_SP, disp);
			else
				M_FLD(tempreg, REG_SP, disp);

		} else {
			if (IS_INT_TYPE(src->type))
				M_ILD(tempreg, REG_SP, disp);
			else
				M_LLD(tempreg, REG_SP, disp);
		}

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
	s4            disp;
	s4            reg;

	/* get required compiler data */

	cd = jd->cd;

	if (src->flags & INMEMORY) {
		COUNT_SPILLS;

		disp = src->regoff * 8;

		if (IS_FLT_DBL_TYPE(src->type)) {
			if (IS_2_WORD_TYPE(src->type))
				M_DLD(tempreg, REG_SP, disp);
			else
				M_FLD(tempreg, REG_SP, disp);

		} else {
			if (IS_INT_TYPE(src->type))
				M_ILD(tempreg, REG_SP, disp);
			else
				M_LLD(tempreg, REG_SP, disp);
		}

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
	s4            disp;
	s4            reg;

	/* get required compiler data */

	cd = jd->cd;

	if (src->flags & INMEMORY) {
		COUNT_SPILLS;

		disp = src->regoff * 8;

		if (IS_FLT_DBL_TYPE(src->type)) {
			if (IS_2_WORD_TYPE(src->type))
				M_DLD(tempreg, REG_SP, disp);
			else
				M_FLD(tempreg, REG_SP, disp);

		} else {
			if (IS_INT_TYPE(src->type))
				M_ILD(tempreg, REG_SP, disp);
			else
				M_LLD(tempreg, REG_SP, disp);
		}

		reg = tempreg;
	} else
		reg = src->regoff;

	return reg;
}


/* emit_store ******************************************************************

   This function generates the code to store the result of an
   operation back into a spilled pseudo-variable.  If the
   pseudo-variable has not been spilled in the first place, this
   function will generate nothing.
    
*******************************************************************************/

void emit_store(jitdata *jd, instruction *iptr, stackptr dst, s4 d)
{
	codegendata  *cd;
	registerdata *rd;
	s4            disp;
#if 0
	s4            s;
	u2            opcode;
#endif

	/* get required compiler data */

	cd = jd->cd;
	rd = jd->rd;

#if 0
	/* do we have to generate a conditional move? */

	if ((iptr != NULL) && (iptr->opc & ICMD_CONDITION_MASK)) {
		/* the passed register d is actually the source register */

		s = d;

		/* Only pass the opcode to codegen_reg_of_var to get the real
		   destination register. */

		opcode = iptr->opc & ICMD_OPCODE_MASK;

		/* get the real destination register */

		d = codegen_reg_of_var(rd, opcode, dst, REG_ITMP1);

		/* and emit the conditional move */

		emit_cmovxx(cd, iptr, s, d);
	}
#endif

	if (dst->flags & INMEMORY) {
		COUNT_SPILLS;

		disp = dst->regoff * 8;

		if (IS_FLT_DBL_TYPE(dst->type)) {
			if (IS_2_WORD_TYPE(dst->type))
				M_DST(d, REG_SP, disp);
			else
				M_FST(d, REG_SP, disp);

		} else
			M_LST(d, REG_SP, disp);
	}
}


/* emit_copy *******************************************************************

   XXX

*******************************************************************************/

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

		if (s1 != d) {
			if (IS_FLT_DBL_TYPE(src->type))
				M_FMOV(s1, d);
			else
				M_MOV(s1, d);
		}

		emit_store(jd, iptr, dst, d);
	}
}


void emit_cmovxx(codegendata *cd, instruction *iptr, s4 s, s4 d)
{
#if 0
	switch (iptr->flags.fields.condition) {
	case ICMD_IFEQ:
		M_CMOVEQ(s, d);
		break;
	case ICMD_IFNE:
		M_CMOVNE(s, d);
		break;
	case ICMD_IFLT:
		M_CMOVLT(s, d);
		break;
	case ICMD_IFGE:
		M_CMOVGE(s, d);
		break;
	case ICMD_IFGT:
		M_CMOVGT(s, d);
		break;
	case ICMD_IFLE:
		M_CMOVLE(s, d);
		break;
	}
#endif
}


/* emit_exception_stubs ********************************************************

   Generates the code for the exception stubs.

*******************************************************************************/

void emit_exception_stubs(jitdata *jd)
{
	codegendata  *cd;
	registerdata *rd;
	exceptionref *eref;
	s4           targetdisp;

	/* get required compiler data */

	cd = jd->cd;
	rd = jd->rd;

	/* generate exception stubs */

	targetdisp = 0;

	for (eref = cd->exceptionrefs; eref != NULL; eref = eref->next) {
		gen_resolvebranch(cd->mcodebase + eref->branchpos, 
						  eref->branchpos,
						  cd->mcodeptr - cd->mcodebase);

		MCODECHECK(512);

		/* Check if the exception is an
		   ArrayIndexOutOfBoundsException.  If so, move index register
		   into a4. */

		if (eref->reg != -1)
			M_MOV(eref->reg, rd->argintregs[4]);

		/* calcuate exception address */

		M_MOV_IMM(0, rd->argintregs[3]);
		dseg_adddata(cd);
		M_AADD_IMM32(eref->branchpos - 6, rd->argintregs[3]);

		/* move function to call into REG_ITMP3 */

		M_MOV_IMM(eref->function, REG_ITMP3);

		if (targetdisp == 0) {
			targetdisp = cd->mcodeptr - cd->mcodebase;

			emit_lea_membase_reg(cd, RIP, -((cd->mcodeptr + 7) - cd->mcodebase), rd->argintregs[0]);
			M_MOV(REG_SP, rd->argintregs[1]);
			M_ALD(rd->argintregs[2], REG_SP, cd->stackframesize * 8);

			M_ASUB_IMM(2 * 8, REG_SP);
			M_AST(rd->argintregs[3], REG_SP, 0 * 8);             /* store XPC */

			M_CALL(REG_ITMP3);

			M_ALD(REG_ITMP2_XPC, REG_SP, 0 * 8);
			M_AADD_IMM(2 * 8, REG_SP);

			M_MOV_IMM(asm_handle_exception, REG_ITMP3);
			M_JMP(REG_ITMP3);
		}
		else {
			M_JMP_IMM((cd->mcodebase + targetdisp) -
					  (cd->mcodeptr + PATCHER_CALL_SIZE));
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
	u8           mcode;
	u1          *savedmcodeptr;
	u1          *tmpmcodeptr;
	s4           targetdisp;
	s4           disp;

	/* get required compiler data */

	cd = jd->cd;

	/* generate code patching stub call code */

	targetdisp = 0;

	for (pref = cd->patchrefs; pref != NULL; pref = pref->next) {
		/* check size of code segment */

		MCODECHECK(512);

		/* Get machine code which is patched back in later. A
		   `call rel32' is 5 bytes long (but read 8 bytes). */

		savedmcodeptr = cd->mcodebase + pref->branchpos;
		mcode = *((u8 *) savedmcodeptr);

		/* patch in `call rel32' to call the following code */

		tmpmcodeptr  = cd->mcodeptr;    /* save current mcodeptr              */
		cd->mcodeptr = savedmcodeptr;   /* set mcodeptr to patch position     */

		M_CALL_IMM(tmpmcodeptr - (savedmcodeptr + PATCHER_CALL_SIZE));

		cd->mcodeptr = tmpmcodeptr;     /* restore the current mcodeptr       */

		/* move pointer to java_objectheader onto stack */

#if defined(ENABLE_THREADS)
		/* create a virtual java_objectheader */

		(void) dseg_addaddress(cd, NULL);                          /* flcword */
		(void) dseg_addaddress(cd, lock_get_initial_lock_word());
		disp = dseg_addaddress(cd, NULL);                          /* vftbl   */

		emit_lea_membase_reg(cd, RIP, -((cd->mcodeptr + 7) - cd->mcodebase) + disp, REG_ITMP3);
		M_PUSH(REG_ITMP3);
#else
		M_PUSH_IMM(0);
#endif

		/* move machine code bytes and classinfo pointer into registers */

		M_MOV_IMM(mcode, REG_ITMP3);
		M_PUSH(REG_ITMP3);

		M_MOV_IMM(pref->ref, REG_ITMP3);
		M_PUSH(REG_ITMP3);

		M_MOV_IMM(pref->disp, REG_ITMP3);
		M_PUSH(REG_ITMP3);

		M_MOV_IMM(pref->patcher, REG_ITMP3);
		M_PUSH(REG_ITMP3);

		if (targetdisp == 0) {
			targetdisp = cd->mcodeptr - cd->mcodebase;

			M_MOV_IMM(asm_patcher_wrapper, REG_ITMP3);
			M_JMP(REG_ITMP3);
		}
		else {
			M_JMP_IMM((cd->mcodebase + targetdisp) -
					  (cd->mcodeptr + PATCHER_CALL_SIZE));
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
	s4           disp;
	s4           i;

	/* get required compiler data */

	cd   = jd->cd;
	code = jd->code;

	rplp = code->rplpoints;

	for (i = 0; i < code->rplpointcount; ++i, ++rplp) {
		/* check code segment size */

		MCODECHECK(512);

		/* note start of stub code */

		rplp->outcode = (u1 *) (ptrint) (cd->mcodeptr - cd->mcodebase);

		/* make machine code for patching */

		disp = (ptrint) (rplp->outcode - rplp->pc) - 5;

		rplp->mcode = 0xe9 | ((u8) disp << 8);

		/* push address of `rplpoint` struct */
			
		M_MOV_IMM(rplp, REG_ITMP3);
		M_PUSH(REG_ITMP3);

		/* jump to replacement function */

		M_MOV_IMM(asm_replacement_out, REG_ITMP3);
		M_JMP(REG_ITMP3);
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
	s4            i, j, k;

	/* get required compiler data */

	m  = jd->m;
	cd = jd->cd;
	rd = jd->rd;

	md = m->parseddesc;

	/* mark trace code */

	M_NOP;

	/* additional +1 is for 16-byte stack alignment */

	M_LSUB_IMM((ARG_CNT + TMP_CNT + 1 + 1) * 8, REG_SP);

	/* save argument registers */

	for (i = 0; i < INT_ARG_CNT; i++)
		M_LST(rd->argintregs[i], REG_SP, (1 + i) * 8);

	for (i = 0; i < FLT_ARG_CNT; i++)
		M_DST(rd->argfltregs[i], REG_SP, (1 + INT_ARG_CNT + i) * 8);

	/* save temporary registers for leaf methods */

	if (jd->isleafmethod) {
		for (i = 0; i < INT_TMP_CNT; i++)
			M_LST(rd->tmpintregs[i], REG_SP, (1 + ARG_CNT + i) * 8);

		for (i = 0; i < FLT_TMP_CNT; i++)
			M_DST(rd->tmpfltregs[i], REG_SP, (1 + ARG_CNT + INT_TMP_CNT + i) * 8);
	}

	/* show integer hex code for float arguments */

	for (i = 0, j = 0; i < md->paramcount && i < INT_ARG_CNT; i++) {
		/* If the paramtype is a float, we have to right shift all
		   following integer registers. */
	
		if (IS_FLT_DBL_TYPE(md->paramtypes[i].type)) {
			for (k = INT_ARG_CNT - 2; k >= i; k--)
				M_MOV(rd->argintregs[k], rd->argintregs[k + 1]);

			emit_movd_freg_reg(cd, rd->argfltregs[j], rd->argintregs[i]);
			j++;
		}
	}

	M_MOV_IMM(m, REG_ITMP2);
	M_AST(REG_ITMP2, REG_SP, 0 * 8);
	M_MOV_IMM(builtin_trace_args, REG_ITMP1);
	M_CALL(REG_ITMP1);

	/* restore argument registers */

	for (i = 0; i < INT_ARG_CNT; i++)
		M_LLD(rd->argintregs[i], REG_SP, (1 + i) * 8);

	for (i = 0; i < FLT_ARG_CNT; i++)
		M_DLD(rd->argfltregs[i], REG_SP, (1 + INT_ARG_CNT + i) * 8);

	/* restore temporary registers for leaf methods */

	if (jd->isleafmethod) {
		for (i = 0; i < INT_TMP_CNT; i++)
			M_LLD(rd->tmpintregs[i], REG_SP, (1 + ARG_CNT + i) * 8);

		for (i = 0; i < FLT_TMP_CNT; i++)
			M_DLD(rd->tmpfltregs[i], REG_SP, (1 + ARG_CNT + INT_TMP_CNT + i) * 8);
	}

	M_LADD_IMM((ARG_CNT + TMP_CNT + 1 + 1) * 8, REG_SP);

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

	/* get required compiler data */

	m  = jd->m;
	cd = jd->cd;
	rd = jd->rd;

	/* mark trace code */

	M_NOP;

	M_ASUB_IMM(2 * 8, REG_SP);

	M_LST(REG_RESULT, REG_SP, 0 * 8);
	M_DST(REG_FRESULT, REG_SP, 1 * 8);

	M_MOV_IMM(m, rd->argintregs[0]);
	M_MOV(REG_RESULT, rd->argintregs[1]);
	M_FLTMOVE(REG_FRESULT, rd->argfltregs[0]);
	M_FLTMOVE(REG_FRESULT, rd->argfltregs[1]);

	M_MOV_IMM(builtin_displaymethodstop, REG_ITMP1);
	M_CALL(REG_ITMP1);

	M_LLD(REG_RESULT, REG_SP, 0 * 8);
	M_DLD(REG_FRESULT, REG_SP, 1 * 8);

	M_AADD_IMM(2 * 8, REG_SP);

	/* mark trace code */

	M_NOP;
}
#endif /* !defined(NDEBUG) */


/* code generation functions **************************************************/

static void emit_membase(codegendata *cd, s4 basereg, s4 disp, s4 dreg)
{
	if ((basereg == REG_SP) || (basereg == R12)) {
		if (disp == 0) {
			emit_address_byte(0, dreg, REG_SP);
			emit_address_byte(0, REG_SP, REG_SP);

		} else if (IS_IMM8(disp)) {
			emit_address_byte(1, dreg, REG_SP);
			emit_address_byte(0, REG_SP, REG_SP);
			emit_imm8(disp);

		} else {
			emit_address_byte(2, dreg, REG_SP);
			emit_address_byte(0, REG_SP, REG_SP);
			emit_imm32(disp);
		}

	} else if ((disp) == 0 && (basereg) != RBP && (basereg) != R13) {
		emit_address_byte(0,(dreg),(basereg));

	} else if ((basereg) == RIP) {
		emit_address_byte(0, dreg, RBP);
		emit_imm32(disp);

	} else {
		if (IS_IMM8(disp)) {
			emit_address_byte(1, dreg, basereg);
			emit_imm8(disp);

		} else {
			emit_address_byte(2, dreg, basereg);
			emit_imm32(disp);
		}
	}
}


static void emit_membase32(codegendata *cd, s4 basereg, s4 disp, s4 dreg)
{
	if ((basereg == REG_SP) || (basereg == R12)) {
		emit_address_byte(2, dreg, REG_SP);
		emit_address_byte(0, REG_SP, REG_SP);
		emit_imm32(disp);
	}
	else {
		emit_address_byte(2, dreg, basereg);
		emit_imm32(disp);
	}
}


static void emit_memindex(codegendata *cd, s4 reg, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	if (basereg == -1) {
		emit_address_byte(0, reg, 4);
		emit_address_byte(scale, indexreg, 5);
		emit_imm32(disp);
	}
	else if ((disp == 0) && (basereg != RBP) && (basereg != R13)) {
		emit_address_byte(0, reg, 4);
		emit_address_byte(scale, indexreg, basereg);
	}
	else if (IS_IMM8(disp)) {
		emit_address_byte(1, reg, 4);
		emit_address_byte(scale, indexreg, basereg);
		emit_imm8(disp);
	}
	else {
		emit_address_byte(2, reg, 4);
		emit_address_byte(scale, indexreg, basereg);
		emit_imm32(disp);
	}
}


void emit_ishift(codegendata *cd, s4 shift_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->prev->regoff;
	s4 s2 = src->regoff;
	s4 d = iptr->dst->regoff;
	s4 d_old;

	M_INTMOVE(RCX, REG_ITMP1);                                    /* save RCX */

	if (iptr->dst->flags & INMEMORY) {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (s1 == d) {
				M_ILD(RCX, REG_SP, s2 * 8);
				emit_shiftl_membase(cd, shift_op, REG_SP, d * 8);

			} else {
				M_ILD(RCX, REG_SP, s2 * 8);
				M_ILD(REG_ITMP2, REG_SP, s1 * 8);
				emit_shiftl_reg(cd, shift_op, REG_ITMP2);
				M_IST(REG_ITMP2, REG_SP, d * 8);
			}

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			/* s1 may be equal to RCX */
			if (s1 == RCX) {
				if (s2 == d) {
					M_ILD(REG_ITMP1, REG_SP, s2 * 8);
					M_IST(s1, REG_SP, d * 8);
					M_INTMOVE(REG_ITMP1, RCX);

				} else {
					M_IST(s1, REG_SP, d * 8);
					M_ILD(RCX, REG_SP, s2 * 8);
				}

			} else {
				M_ILD(RCX, REG_SP, s2 * 8);
				M_IST(s1, REG_SP, d * 8);
			}

			emit_shiftl_membase(cd, shift_op, REG_SP, d * 8);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (s1 == d) {
				M_INTMOVE(s2, RCX);
				emit_shiftl_membase(cd, shift_op, REG_SP, d * 8);

			} else {
				M_INTMOVE(s2, RCX);
				M_ILD(REG_ITMP2, REG_SP, s1 * 8);
				emit_shiftl_reg(cd, shift_op, REG_ITMP2);
				M_IST(REG_ITMP2, REG_SP, d * 8);
			}

		} else {
			/* s1 may be equal to RCX */
			M_IST(s1, REG_SP, d * 8);
			M_INTMOVE(s2, RCX);
			emit_shiftl_membase(cd, shift_op, REG_SP, d * 8);
		}

		M_INTMOVE(REG_ITMP1, RCX);                             /* restore RCX */

	} else {
		d_old = d;
		if (d == RCX) {
			d = REG_ITMP3;
		}
					
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			M_ILD(RCX, REG_SP, s2 * 8);
			M_ILD(d, REG_SP, s1 * 8);
			emit_shiftl_reg(cd, shift_op, d);

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			/* s1 may be equal to RCX */
			M_INTMOVE(s1, d);
			M_ILD(RCX, REG_SP, s2 * 8);
			emit_shiftl_reg(cd, shift_op, d);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			M_INTMOVE(s2, RCX);
			M_ILD(d, REG_SP, s1 * 8);
			emit_shiftl_reg(cd, shift_op, d);

		} else {
			/* s1 may be equal to RCX */
			if (s1 == RCX) {
				if (s2 == d) {
					/* d cannot be used to backup s1 since this would
					   overwrite s2. */
					M_INTMOVE(s1, REG_ITMP3);
					M_INTMOVE(s2, RCX);
					M_INTMOVE(REG_ITMP3, d);

				} else {
					M_INTMOVE(s1, d);
					M_INTMOVE(s2, RCX);
				}

			} else {
				/* d may be equal to s2 */
				M_INTMOVE(s2, RCX);
				M_INTMOVE(s1, d);
			}
			emit_shiftl_reg(cd, shift_op, d);
		}

		if (d_old == RCX)
			M_INTMOVE(REG_ITMP3, RCX);
		else
			M_INTMOVE(REG_ITMP1, RCX);                         /* restore RCX */
	}
}


void emit_lshift(codegendata *cd, s4 shift_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->prev->regoff;
	s4 s2 = src->regoff;
	s4 d = iptr->dst->regoff;
	s4 d_old;
	
	M_INTMOVE(RCX, REG_ITMP1);                                    /* save RCX */

	if (iptr->dst->flags & INMEMORY) {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (s1 == d) {
				M_ILD(RCX, REG_SP, s2 * 8);
				emit_shift_membase(cd, shift_op, REG_SP, d * 8);

			} else {
				M_ILD(RCX, REG_SP, s2 * 8);
				M_LLD(REG_ITMP2, REG_SP, s1 * 8);
				emit_shift_reg(cd, shift_op, REG_ITMP2);
				M_LST(REG_ITMP2, REG_SP, d * 8);
			}

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			/* s1 may be equal to RCX */
			if (s1 == RCX) {
				if (s2 == d) {
					M_ILD(REG_ITMP1, REG_SP, s2 * 8);
					M_LST(s1, REG_SP, d * 8);
					M_INTMOVE(REG_ITMP1, RCX);

				} else {
					M_LST(s1, REG_SP, d * 8);
					M_ILD(RCX, REG_SP, s2 * 8);
				}

			} else {
				M_ILD(RCX, REG_SP, s2 * 8);
				M_LST(s1, REG_SP, d * 8);
			}

			emit_shift_membase(cd, shift_op, REG_SP, d * 8);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (s1 == d) {
				M_INTMOVE(s2, RCX);
				emit_shift_membase(cd, shift_op, REG_SP, d * 8);

			} else {
				M_INTMOVE(s2, RCX);
				M_LLD(REG_ITMP2, REG_SP, s1 * 8);
				emit_shift_reg(cd, shift_op, REG_ITMP2);
				M_LST(REG_ITMP2, REG_SP, d * 8);
			}

		} else {
			/* s1 may be equal to RCX */
			M_LST(s1, REG_SP, d * 8);
			M_INTMOVE(s2, RCX);
			emit_shift_membase(cd, shift_op, REG_SP, d * 8);
		}

		M_INTMOVE(REG_ITMP1, RCX);                             /* restore RCX */

	} else {
		d_old = d;
		if (d == RCX) {
			d = REG_ITMP3;
		}

		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			M_ILD(RCX, REG_SP, s2 * 8);
			M_LLD(d, REG_SP, s1 * 8);
			emit_shift_reg(cd, shift_op, d);

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			/* s1 may be equal to RCX */
			M_INTMOVE(s1, d);
			M_ILD(RCX, REG_SP, s2 * 8);
			emit_shift_reg(cd, shift_op, d);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			M_INTMOVE(s2, RCX);
			M_LLD(d, REG_SP, s1 * 8);
			emit_shift_reg(cd, shift_op, d);

		} else {
			/* s1 may be equal to RCX */
			if (s1 == RCX) {
				if (s2 == d) {
					/* d cannot be used to backup s1 since this would
					   overwrite s2. */
					M_INTMOVE(s1, REG_ITMP3);
					M_INTMOVE(s2, RCX);
					M_INTMOVE(REG_ITMP3, d);

				} else {
					M_INTMOVE(s1, d);
					M_INTMOVE(s2, RCX);
				}

			} else {
				/* d may be equal to s2 */
				M_INTMOVE(s2, RCX);
				M_INTMOVE(s1, d);
			}
			emit_shift_reg(cd, shift_op, d);
		}

		if (d_old == RCX)
			M_INTMOVE(REG_ITMP3, RCX);
		else
			M_INTMOVE(REG_ITMP1, RCX);                         /* restore RCX */
	}
}


/* low-level code emitter functions *******************************************/

void emit_mov_reg_reg(codegendata *cd, s8 reg, s8 dreg)
{
	emit_rex(1,(reg),0,(dreg));
	*(cd->mcodeptr++) = 0x89;
	emit_reg((reg),(dreg));
}


void emit_mov_imm_reg(codegendata *cd, s8 imm, s8 reg)
{
	emit_rex(1,0,0,(reg));
	*(cd->mcodeptr++) = 0xb8 + ((reg) & 0x07);
	emit_imm64((imm));
}


void emit_movl_reg_reg(codegendata *cd, s8 reg, s8 dreg)
{
	emit_rex(0,(reg),0,(dreg));
	*(cd->mcodeptr++) = 0x89;
	emit_reg((reg),(dreg));
}


void emit_movl_imm_reg(codegendata *cd, s8 imm, s8 reg) {
	emit_rex(0,0,0,(reg));
	*(cd->mcodeptr++) = 0xb8 + ((reg) & 0x07);
	emit_imm32((imm));
}


void emit_mov_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 reg) {
	emit_rex(1,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x8b;
	emit_membase(cd, (basereg),(disp),(reg));
}


/*
 * this one is for INVOKEVIRTUAL/INVOKEINTERFACE to have a
 * constant membase immediate length of 32bit
 */
void emit_mov_membase32_reg(codegendata *cd, s8 basereg, s8 disp, s8 reg) {
	emit_rex(1,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x8b;
	emit_membase32(cd, (basereg),(disp),(reg));
}


void emit_movl_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 reg)
{
	emit_rex(0,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x8b;
	emit_membase(cd, (basereg),(disp),(reg));
}


/* ATTENTION: Always emit a REX byte, because the instruction size can
   be smaller when all register indexes are smaller than 7. */
void emit_movl_membase32_reg(codegendata *cd, s8 basereg, s8 disp, s8 reg)
{
	emit_byte_rex((reg),0,(basereg));
	*(cd->mcodeptr++) = 0x8b;
	emit_membase32(cd, (basereg),(disp),(reg));
}


void emit_mov_reg_membase(codegendata *cd, s8 reg, s8 basereg, s8 disp) {
	emit_rex(1,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x89;
	emit_membase(cd, (basereg),(disp),(reg));
}


void emit_mov_reg_membase32(codegendata *cd, s8 reg, s8 basereg, s8 disp) {
	emit_rex(1,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x89;
	emit_membase32(cd, (basereg),(disp),(reg));
}


void emit_movl_reg_membase(codegendata *cd, s8 reg, s8 basereg, s8 disp) {
	emit_rex(0,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x89;
	emit_membase(cd, (basereg),(disp),(reg));
}


/* Always emit a REX byte, because the instruction size can be smaller when   */
/* all register indexes are smaller than 7.                                   */
void emit_movl_reg_membase32(codegendata *cd, s8 reg, s8 basereg, s8 disp) {
	emit_byte_rex((reg),0,(basereg));
	*(cd->mcodeptr++) = 0x89;
	emit_membase32(cd, (basereg),(disp),(reg));
}


void emit_mov_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg) {
	emit_rex(1,(reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x8b;
	emit_memindex(cd, (reg),(disp),(basereg),(indexreg),(scale));
}


void emit_movl_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg) {
	emit_rex(0,(reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x8b;
	emit_memindex(cd, (reg),(disp),(basereg),(indexreg),(scale));
}


void emit_mov_reg_memindex(codegendata *cd, s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale) {
	emit_rex(1,(reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x89;
	emit_memindex(cd, (reg),(disp),(basereg),(indexreg),(scale));
}


void emit_movl_reg_memindex(codegendata *cd, s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale) {
	emit_rex(0,(reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x89;
	emit_memindex(cd, (reg),(disp),(basereg),(indexreg),(scale));
}


void emit_movw_reg_memindex(codegendata *cd, s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale) {
	*(cd->mcodeptr++) = 0x66;
	emit_rex(0,(reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x89;
	emit_memindex(cd, (reg),(disp),(basereg),(indexreg),(scale));
}


void emit_movb_reg_memindex(codegendata *cd, s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale) {
	emit_byte_rex((reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x88;
	emit_memindex(cd, (reg),(disp),(basereg),(indexreg),(scale));
}


void emit_mov_imm_membase(codegendata *cd, s8 imm, s8 basereg, s8 disp) {
	emit_rex(1,0,0,(basereg));
	*(cd->mcodeptr++) = 0xc7;
	emit_membase(cd, (basereg),(disp),0);
	emit_imm32((imm));
}


void emit_mov_imm_membase32(codegendata *cd, s8 imm, s8 basereg, s8 disp) {
	emit_rex(1,0,0,(basereg));
	*(cd->mcodeptr++) = 0xc7;
	emit_membase32(cd, (basereg),(disp),0);
	emit_imm32((imm));
}


void emit_movl_imm_membase(codegendata *cd, s8 imm, s8 basereg, s8 disp) {
	emit_rex(0,0,0,(basereg));
	*(cd->mcodeptr++) = 0xc7;
	emit_membase(cd, (basereg),(disp),0);
	emit_imm32((imm));
}


/* Always emit a REX byte, because the instruction size can be smaller when   */
/* all register indexes are smaller than 7.                                   */
void emit_movl_imm_membase32(codegendata *cd, s8 imm, s8 basereg, s8 disp) {
	emit_byte_rex(0,0,(basereg));
	*(cd->mcodeptr++) = 0xc7;
	emit_membase32(cd, (basereg),(disp),0);
	emit_imm32((imm));
}


void emit_movsbq_reg_reg(codegendata *cd, s8 reg, s8 dreg)
{
	emit_rex(1,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xbe;
	/* XXX: why do reg and dreg have to be exchanged */
	emit_reg((dreg),(reg));
}


void emit_movswq_reg_reg(codegendata *cd, s8 reg, s8 dreg)
{
	emit_rex(1,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xbf;
	/* XXX: why do reg and dreg have to be exchanged */
	emit_reg((dreg),(reg));
}


void emit_movslq_reg_reg(codegendata *cd, s8 reg, s8 dreg)
{
	emit_rex(1,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x63;
	/* XXX: why do reg and dreg have to be exchanged */
	emit_reg((dreg),(reg));
}


void emit_movzwq_reg_reg(codegendata *cd, s8 reg, s8 dreg)
{
	emit_rex(1,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xb7;
	/* XXX: why do reg and dreg have to be exchanged */
	emit_reg((dreg),(reg));
}


void emit_movswq_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg) {
	emit_rex(1,(reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xbf;
	emit_memindex(cd, (reg),(disp),(basereg),(indexreg),(scale));
}


void emit_movsbq_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg) {
	emit_rex(1,(reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xbe;
	emit_memindex(cd, (reg),(disp),(basereg),(indexreg),(scale));
}


void emit_movzwq_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg) {
	emit_rex(1,(reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xb7;
	emit_memindex(cd, (reg),(disp),(basereg),(indexreg),(scale));
}


void emit_mov_imm_memindex(codegendata *cd, s4 imm, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	emit_rex(1,0,(indexreg),(basereg));
	*(cd->mcodeptr++) = 0xc7;
	emit_memindex(cd, 0,(disp),(basereg),(indexreg),(scale));
	emit_imm32((imm));
}


void emit_movl_imm_memindex(codegendata *cd, s4 imm, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	emit_rex(0,0,(indexreg),(basereg));
	*(cd->mcodeptr++) = 0xc7;
	emit_memindex(cd, 0,(disp),(basereg),(indexreg),(scale));
	emit_imm32((imm));
}


void emit_movw_imm_memindex(codegendata *cd, s4 imm, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	*(cd->mcodeptr++) = 0x66;
	emit_rex(0,0,(indexreg),(basereg));
	*(cd->mcodeptr++) = 0xc7;
	emit_memindex(cd, 0,(disp),(basereg),(indexreg),(scale));
	emit_imm16((imm));
}


void emit_movb_imm_memindex(codegendata *cd, s4 imm, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	emit_rex(0,0,(indexreg),(basereg));
	*(cd->mcodeptr++) = 0xc6;
	emit_memindex(cd, 0,(disp),(basereg),(indexreg),(scale));
	emit_imm8((imm));
}


/*
 * alu operations
 */
void emit_alu_reg_reg(codegendata *cd, s8 opc, s8 reg, s8 dreg)
{
	emit_rex(1,(reg),0,(dreg));
	*(cd->mcodeptr++) = (((opc)) << 3) + 1;
	emit_reg((reg),(dreg));
}


void emit_alul_reg_reg(codegendata *cd, s8 opc, s8 reg, s8 dreg)
{
	emit_rex(0,(reg),0,(dreg));
	*(cd->mcodeptr++) = (((opc)) << 3) + 1;
	emit_reg((reg),(dreg));
}


void emit_alu_reg_membase(codegendata *cd, s8 opc, s8 reg, s8 basereg, s8 disp)
{
	emit_rex(1,(reg),0,(basereg));
	*(cd->mcodeptr++) = (((opc)) << 3) + 1;
	emit_membase(cd, (basereg),(disp),(reg));
}


void emit_alul_reg_membase(codegendata *cd, s8 opc, s8 reg, s8 basereg, s8 disp)
{
	emit_rex(0,(reg),0,(basereg));
	*(cd->mcodeptr++) = (((opc)) << 3) + 1;
	emit_membase(cd, (basereg),(disp),(reg));
}


void emit_alu_membase_reg(codegendata *cd, s8 opc, s8 basereg, s8 disp, s8 reg)
{
	emit_rex(1,(reg),0,(basereg));
	*(cd->mcodeptr++) = (((opc)) << 3) + 3;
	emit_membase(cd, (basereg),(disp),(reg));
}


void emit_alul_membase_reg(codegendata *cd, s8 opc, s8 basereg, s8 disp, s8 reg)
{
	emit_rex(0,(reg),0,(basereg));
	*(cd->mcodeptr++) = (((opc)) << 3) + 3;
	emit_membase(cd, (basereg),(disp),(reg));
}


void emit_alu_imm_reg(codegendata *cd, s8 opc, s8 imm, s8 dreg) {
	if (IS_IMM8(imm)) {
		emit_rex(1,0,0,(dreg));
		*(cd->mcodeptr++) = 0x83;
		emit_reg((opc),(dreg));
		emit_imm8((imm));
	} else {
		emit_rex(1,0,0,(dreg));
		*(cd->mcodeptr++) = 0x81;
		emit_reg((opc),(dreg));
		emit_imm32((imm));
	}
}


void emit_alu_imm32_reg(codegendata *cd, s8 opc, s8 imm, s8 dreg) {
	emit_rex(1,0,0,(dreg));
	*(cd->mcodeptr++) = 0x81;
	emit_reg((opc),(dreg));
	emit_imm32((imm));
}


void emit_alul_imm_reg(codegendata *cd, s8 opc, s8 imm, s8 dreg) {
	if (IS_IMM8(imm)) {
		emit_rex(0,0,0,(dreg));
		*(cd->mcodeptr++) = 0x83;
		emit_reg((opc),(dreg));
		emit_imm8((imm));
	} else {
		emit_rex(0,0,0,(dreg));
		*(cd->mcodeptr++) = 0x81;
		emit_reg((opc),(dreg));
		emit_imm32((imm));
	}
}


void emit_alu_imm_membase(codegendata *cd, s8 opc, s8 imm, s8 basereg, s8 disp) {
	if (IS_IMM8(imm)) {
		emit_rex(1,(basereg),0,0);
		*(cd->mcodeptr++) = 0x83;
		emit_membase(cd, (basereg),(disp),(opc));
		emit_imm8((imm));
	} else {
		emit_rex(1,(basereg),0,0);
		*(cd->mcodeptr++) = 0x81;
		emit_membase(cd, (basereg),(disp),(opc));
		emit_imm32((imm));
	}
}


void emit_alul_imm_membase(codegendata *cd, s8 opc, s8 imm, s8 basereg, s8 disp) {
	if (IS_IMM8(imm)) {
		emit_rex(0,(basereg),0,0);
		*(cd->mcodeptr++) = 0x83;
		emit_membase(cd, (basereg),(disp),(opc));
		emit_imm8((imm));
	} else {
		emit_rex(0,(basereg),0,0);
		*(cd->mcodeptr++) = 0x81;
		emit_membase(cd, (basereg),(disp),(opc));
		emit_imm32((imm));
	}
}


void emit_test_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	emit_rex(1,(reg),0,(dreg));
	*(cd->mcodeptr++) = 0x85;
	emit_reg((reg),(dreg));
}


void emit_testl_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	emit_rex(0,(reg),0,(dreg));
	*(cd->mcodeptr++) = 0x85;
	emit_reg((reg),(dreg));
}


void emit_test_imm_reg(codegendata *cd, s8 imm, s8 reg) {
	*(cd->mcodeptr++) = 0xf7;
	emit_reg(0,(reg));
	emit_imm32((imm));
}


void emit_testw_imm_reg(codegendata *cd, s8 imm, s8 reg) {
	*(cd->mcodeptr++) = 0x66;
	*(cd->mcodeptr++) = 0xf7;
	emit_reg(0,(reg));
	emit_imm16((imm));
}


void emit_testb_imm_reg(codegendata *cd, s8 imm, s8 reg) {
	*(cd->mcodeptr++) = 0xf6;
	emit_reg(0,(reg));
	emit_imm8((imm));
}


void emit_lea_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 reg) {
	emit_rex(1,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x8d;
	emit_membase(cd, (basereg),(disp),(reg));
}


void emit_leal_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 reg) {
	emit_rex(0,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x8d;
	emit_membase(cd, (basereg),(disp),(reg));
}



void emit_incl_membase(codegendata *cd, s8 basereg, s8 disp)
{
	emit_rex(0,0,0,(basereg));
	*(cd->mcodeptr++) = 0xff;
	emit_membase(cd, (basereg),(disp),0);
}



void emit_cltd(codegendata *cd) {
    *(cd->mcodeptr++) = 0x99;
}


void emit_cqto(codegendata *cd) {
	emit_rex(1,0,0,0);
	*(cd->mcodeptr++) = 0x99;
}



void emit_imul_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	emit_rex(1,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xaf;
	emit_reg((dreg),(reg));
}


void emit_imull_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xaf;
	emit_reg((dreg),(reg));
}


void emit_imul_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	emit_rex(1,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xaf;
	emit_membase(cd, (basereg),(disp),(dreg));
}


void emit_imull_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	emit_rex(0,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xaf;
	emit_membase(cd, (basereg),(disp),(dreg));
}


void emit_imul_imm_reg(codegendata *cd, s8 imm, s8 dreg) {
	if (IS_IMM8((imm))) {
		emit_rex(1,0,0,(dreg));
		*(cd->mcodeptr++) = 0x6b;
		emit_reg(0,(dreg));
		emit_imm8((imm));
	} else {
		emit_rex(1,0,0,(dreg));
		*(cd->mcodeptr++) = 0x69;
		emit_reg(0,(dreg));
		emit_imm32((imm));
	}
}


void emit_imul_imm_reg_reg(codegendata *cd, s8 imm, s8 reg, s8 dreg) {
	if (IS_IMM8((imm))) {
		emit_rex(1,(dreg),0,(reg));
		*(cd->mcodeptr++) = 0x6b;
		emit_reg((dreg),(reg));
		emit_imm8((imm));
	} else {
		emit_rex(1,(dreg),0,(reg));
		*(cd->mcodeptr++) = 0x69;
		emit_reg((dreg),(reg));
		emit_imm32((imm));
	}
}


void emit_imull_imm_reg_reg(codegendata *cd, s8 imm, s8 reg, s8 dreg) {
	if (IS_IMM8((imm))) {
		emit_rex(0,(dreg),0,(reg));
		*(cd->mcodeptr++) = 0x6b;
		emit_reg((dreg),(reg));
		emit_imm8((imm));
	} else {
		emit_rex(0,(dreg),0,(reg));
		*(cd->mcodeptr++) = 0x69;
		emit_reg((dreg),(reg));
		emit_imm32((imm));
	}
}


void emit_imul_imm_membase_reg(codegendata *cd, s8 imm, s8 basereg, s8 disp, s8 dreg) {
	if (IS_IMM8((imm))) {
		emit_rex(1,(dreg),0,(basereg));
		*(cd->mcodeptr++) = 0x6b;
		emit_membase(cd, (basereg),(disp),(dreg));
		emit_imm8((imm));
	} else {
		emit_rex(1,(dreg),0,(basereg));
		*(cd->mcodeptr++) = 0x69;
		emit_membase(cd, (basereg),(disp),(dreg));
		emit_imm32((imm));
	}
}


void emit_imull_imm_membase_reg(codegendata *cd, s8 imm, s8 basereg, s8 disp, s8 dreg) {
	if (IS_IMM8((imm))) {
		emit_rex(0,(dreg),0,(basereg));
		*(cd->mcodeptr++) = 0x6b;
		emit_membase(cd, (basereg),(disp),(dreg));
		emit_imm8((imm));
	} else {
		emit_rex(0,(dreg),0,(basereg));
		*(cd->mcodeptr++) = 0x69;
		emit_membase(cd, (basereg),(disp),(dreg));
		emit_imm32((imm));
	}
}


void emit_idiv_reg(codegendata *cd, s8 reg) {
	emit_rex(1,0,0,(reg));
	*(cd->mcodeptr++) = 0xf7;
	emit_reg(7,(reg));
}


void emit_idivl_reg(codegendata *cd, s8 reg) {
	emit_rex(0,0,0,(reg));
	*(cd->mcodeptr++) = 0xf7;
	emit_reg(7,(reg));
}



void emit_ret(codegendata *cd) {
    *(cd->mcodeptr++) = 0xc3;
}



/*
 * shift ops
 */
void emit_shift_reg(codegendata *cd, s8 opc, s8 reg) {
	emit_rex(1,0,0,(reg));
	*(cd->mcodeptr++) = 0xd3;
	emit_reg((opc),(reg));
}


void emit_shiftl_reg(codegendata *cd, s8 opc, s8 reg) {
	emit_rex(0,0,0,(reg));
	*(cd->mcodeptr++) = 0xd3;
	emit_reg((opc),(reg));
}


void emit_shift_membase(codegendata *cd, s8 opc, s8 basereg, s8 disp) {
	emit_rex(1,0,0,(basereg));
	*(cd->mcodeptr++) = 0xd3;
	emit_membase(cd, (basereg),(disp),(opc));
}


void emit_shiftl_membase(codegendata *cd, s8 opc, s8 basereg, s8 disp) {
	emit_rex(0,0,0,(basereg));
	*(cd->mcodeptr++) = 0xd3;
	emit_membase(cd, (basereg),(disp),(opc));
}


void emit_shift_imm_reg(codegendata *cd, s8 opc, s8 imm, s8 dreg) {
	if ((imm) == 1) {
		emit_rex(1,0,0,(dreg));
		*(cd->mcodeptr++) = 0xd1;
		emit_reg((opc),(dreg));
	} else {
		emit_rex(1,0,0,(dreg));
		*(cd->mcodeptr++) = 0xc1;
		emit_reg((opc),(dreg));
		emit_imm8((imm));
	}
}


void emit_shiftl_imm_reg(codegendata *cd, s8 opc, s8 imm, s8 dreg) {
	if ((imm) == 1) {
		emit_rex(0,0,0,(dreg));
		*(cd->mcodeptr++) = 0xd1;
		emit_reg((opc),(dreg));
	} else {
		emit_rex(0,0,0,(dreg));
		*(cd->mcodeptr++) = 0xc1;
		emit_reg((opc),(dreg));
		emit_imm8((imm));
	}
}


void emit_shift_imm_membase(codegendata *cd, s8 opc, s8 imm, s8 basereg, s8 disp) {
	if ((imm) == 1) {
		emit_rex(1,0,0,(basereg));
		*(cd->mcodeptr++) = 0xd1;
		emit_membase(cd, (basereg),(disp),(opc));
	} else {
		emit_rex(1,0,0,(basereg));
		*(cd->mcodeptr++) = 0xc1;
		emit_membase(cd, (basereg),(disp),(opc));
		emit_imm8((imm));
	}
}


void emit_shiftl_imm_membase(codegendata *cd, s8 opc, s8 imm, s8 basereg, s8 disp) {
	if ((imm) == 1) {
		emit_rex(0,0,0,(basereg));
		*(cd->mcodeptr++) = 0xd1;
		emit_membase(cd, (basereg),(disp),(opc));
	} else {
		emit_rex(0,0,0,(basereg));
		*(cd->mcodeptr++) = 0xc1;
		emit_membase(cd, (basereg),(disp),(opc));
		emit_imm8((imm));
	}
}



/*
 * jump operations
 */
void emit_jmp_imm(codegendata *cd, s8 imm) {
	*(cd->mcodeptr++) = 0xe9;
	emit_imm32((imm));
}


void emit_jmp_reg(codegendata *cd, s8 reg) {
	emit_rex(0,0,0,(reg));
	*(cd->mcodeptr++) = 0xff;
	emit_reg(4,(reg));
}


void emit_jcc(codegendata *cd, s8 opc, s8 imm) {
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = (0x80 + (opc));
	emit_imm32((imm));
}



/*
 * conditional set and move operations
 */

/* we need the rex byte to get all low bytes */
void emit_setcc_reg(codegendata *cd, s8 opc, s8 reg) {
	*(cd->mcodeptr++) = (0x40 | (((reg) >> 3) & 0x01));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = (0x90 + (opc));
	emit_reg(0,(reg));
}


/* we need the rex byte to get all low bytes */
void emit_setcc_membase(codegendata *cd, s8 opc, s8 basereg, s8 disp) {
	*(cd->mcodeptr++) = (0x40 | (((basereg) >> 3) & 0x01));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = (0x90 + (opc));
	emit_membase(cd, (basereg),(disp),0);
}


void emit_cmovcc_reg_reg(codegendata *cd, s8 opc, s8 reg, s8 dreg)
{
	emit_rex(1,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = (0x40 + (opc));
	emit_reg((dreg),(reg));
}


void emit_cmovccl_reg_reg(codegendata *cd, s8 opc, s8 reg, s8 dreg)
{
	emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = (0x40 + (opc));
	emit_reg((dreg),(reg));
}



void emit_neg_reg(codegendata *cd, s8 reg)
{
	emit_rex(1,0,0,(reg));
	*(cd->mcodeptr++) = 0xf7;
	emit_reg(3,(reg));
}


void emit_negl_reg(codegendata *cd, s8 reg)
{
	emit_rex(0,0,0,(reg));
	*(cd->mcodeptr++) = 0xf7;
	emit_reg(3,(reg));
}


void emit_push_reg(codegendata *cd, s8 reg) {
	emit_rex(0,0,0,(reg));
	*(cd->mcodeptr++) = 0x50 + (0x07 & (reg));
}


void emit_push_imm(codegendata *cd, s8 imm) {
	*(cd->mcodeptr++) = 0x68;
	emit_imm32((imm));
}


void emit_pop_reg(codegendata *cd, s8 reg) {
	emit_rex(0,0,0,(reg));
	*(cd->mcodeptr++) = 0x58 + (0x07 & (reg));
}


void emit_xchg_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	emit_rex(1,(reg),0,(dreg));
	*(cd->mcodeptr++) = 0x87;
	emit_reg((reg),(dreg));
}


void emit_nop(codegendata *cd) {
    *(cd->mcodeptr++) = 0x90;
}



/*
 * call instructions
 */
void emit_call_reg(codegendata *cd, s8 reg) {
	emit_rex(1,0,0,(reg));
	*(cd->mcodeptr++) = 0xff;
	emit_reg(2,(reg));
}


void emit_call_imm(codegendata *cd, s8 imm) {
	*(cd->mcodeptr++) = 0xe8;
	emit_imm32((imm));
}


void emit_call_mem(codegendata *cd, ptrint mem)
{
	*(cd->mcodeptr++) = 0xff;
	emit_mem(2,(mem));
}



/*
 * floating point instructions (SSE2)
 */
void emit_addsd_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x58;
	emit_reg((dreg),(reg));
}


void emit_addss_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x58;
	emit_reg((dreg),(reg));
}


void emit_cvtsi2ssq_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	emit_rex(1,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x2a;
	emit_reg((dreg),(reg));
}


void emit_cvtsi2ss_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x2a;
	emit_reg((dreg),(reg));
}


void emit_cvtsi2sdq_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	emit_rex(1,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x2a;
	emit_reg((dreg),(reg));
}


void emit_cvtsi2sd_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x2a;
	emit_reg((dreg),(reg));
}


void emit_cvtss2sd_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x5a;
	emit_reg((dreg),(reg));
}


void emit_cvtsd2ss_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x5a;
	emit_reg((dreg),(reg));
}


void emit_cvttss2siq_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	emit_rex(1,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x2c;
	emit_reg((dreg),(reg));
}


void emit_cvttss2si_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x2c;
	emit_reg((dreg),(reg));
}


void emit_cvttsd2siq_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	emit_rex(1,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x2c;
	emit_reg((dreg),(reg));
}


void emit_cvttsd2si_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x2c;
	emit_reg((dreg),(reg));
}


void emit_divss_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x5e;
	emit_reg((dreg),(reg));
}


void emit_divsd_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x5e;
	emit_reg((dreg),(reg));
}


void emit_movd_reg_freg(codegendata *cd, s8 reg, s8 freg) {
	*(cd->mcodeptr++) = 0x66;
	emit_rex(1,(freg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x6e;
	emit_reg((freg),(reg));
}


void emit_movd_freg_reg(codegendata *cd, s8 freg, s8 reg) {
	*(cd->mcodeptr++) = 0x66;
	emit_rex(1,(freg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x7e;
	emit_reg((freg),(reg));
}


void emit_movd_reg_membase(codegendata *cd, s8 reg, s8 basereg, s8 disp) {
	*(cd->mcodeptr++) = 0x66;
	emit_rex(0,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x7e;
	emit_membase(cd, (basereg),(disp),(reg));
}


void emit_movd_reg_memindex(codegendata *cd, s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale) {
	*(cd->mcodeptr++) = 0x66;
	emit_rex(0,(reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x7e;
	emit_memindex(cd, (reg),(disp),(basereg),(indexreg),(scale));
}


void emit_movd_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	*(cd->mcodeptr++) = 0x66;
	emit_rex(1,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x6e;
	emit_membase(cd, (basereg),(disp),(dreg));
}


void emit_movdl_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	*(cd->mcodeptr++) = 0x66;
	emit_rex(0,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x6e;
	emit_membase(cd, (basereg),(disp),(dreg));
}


void emit_movd_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 dreg) {
	*(cd->mcodeptr++) = 0x66;
	emit_rex(0,(dreg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x6e;
	emit_memindex(cd, (dreg),(disp),(basereg),(indexreg),(scale));
}


void emit_movq_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x7e;
	emit_reg((dreg),(reg));
}


void emit_movq_reg_membase(codegendata *cd, s8 reg, s8 basereg, s8 disp) {
	*(cd->mcodeptr++) = 0x66;
	emit_rex(0,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xd6;
	emit_membase(cd, (basereg),(disp),(reg));
}


void emit_movq_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	emit_rex(0,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x7e;
	emit_membase(cd, (basereg),(disp),(dreg));
}


void emit_movss_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	emit_rex(0,(reg),0,(dreg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x10;
	emit_reg((reg),(dreg));
}


void emit_movsd_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	emit_rex(0,(reg),0,(dreg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x10;
	emit_reg((reg),(dreg));
}


void emit_movss_reg_membase(codegendata *cd, s8 reg, s8 basereg, s8 disp) {
	*(cd->mcodeptr++) = 0xf3;
	emit_rex(0,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x11;
	emit_membase(cd, (basereg),(disp),(reg));
}


/* Always emit a REX byte, because the instruction size can be smaller when   */
/* all register indexes are smaller than 7.                                   */
void emit_movss_reg_membase32(codegendata *cd, s8 reg, s8 basereg, s8 disp) {
	*(cd->mcodeptr++) = 0xf3;
	emit_byte_rex((reg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x11;
	emit_membase32(cd, (basereg),(disp),(reg));
}


void emit_movsd_reg_membase(codegendata *cd, s8 reg, s8 basereg, s8 disp) {
	*(cd->mcodeptr++) = 0xf2;
	emit_rex(0,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x11;
	emit_membase(cd, (basereg),(disp),(reg));
}


/* Always emit a REX byte, because the instruction size can be smaller when   */
/* all register indexes are smaller than 7.                                   */
void emit_movsd_reg_membase32(codegendata *cd, s8 reg, s8 basereg, s8 disp) {
	*(cd->mcodeptr++) = 0xf2;
	emit_byte_rex((reg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x11;
	emit_membase32(cd, (basereg),(disp),(reg));
}


void emit_movss_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	emit_rex(0,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x10;
	emit_membase(cd, (basereg),(disp),(dreg));
}


/* Always emit a REX byte, because the instruction size can be smaller when   */
/* all register indexes are smaller than 7.                                   */
void emit_movss_membase32_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	emit_byte_rex((dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x10;
	emit_membase32(cd, (basereg),(disp),(dreg));
}


void emit_movlps_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg)
{
	emit_rex(0,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x12;
	emit_membase(cd, (basereg),(disp),(dreg));
}


void emit_movlps_reg_membase(codegendata *cd, s8 reg, s8 basereg, s8 disp)
{
	emit_rex(0,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x13;
	emit_membase(cd, (basereg),(disp),(reg));
}


void emit_movsd_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	emit_rex(0,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x10;
	emit_membase(cd, (basereg),(disp),(dreg));
}


/* Always emit a REX byte, because the instruction size can be smaller when   */
/* all register indexes are smaller than 7.                                   */
void emit_movsd_membase32_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	emit_byte_rex((dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x10;
	emit_membase32(cd, (basereg),(disp),(dreg));
}


void emit_movlpd_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg)
{
	*(cd->mcodeptr++) = 0x66;
	emit_rex(0,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x12;
	emit_membase(cd, (basereg),(disp),(dreg));
}


void emit_movlpd_reg_membase(codegendata *cd, s8 reg, s8 basereg, s8 disp)
{
	*(cd->mcodeptr++) = 0x66;
	emit_rex(0,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x13;
	emit_membase(cd, (basereg),(disp),(reg));
}


void emit_movss_reg_memindex(codegendata *cd, s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale) {
	*(cd->mcodeptr++) = 0xf3;
	emit_rex(0,(reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x11;
	emit_memindex(cd, (reg),(disp),(basereg),(indexreg),(scale));
}


void emit_movsd_reg_memindex(codegendata *cd, s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale) {
	*(cd->mcodeptr++) = 0xf2;
	emit_rex(0,(reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x11;
	emit_memindex(cd, (reg),(disp),(basereg),(indexreg),(scale));
}


void emit_movss_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	emit_rex(0,(dreg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x10;
	emit_memindex(cd, (dreg),(disp),(basereg),(indexreg),(scale));
}


void emit_movsd_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	emit_rex(0,(dreg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x10;
	emit_memindex(cd, (dreg),(disp),(basereg),(indexreg),(scale));
}


void emit_mulss_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x59;
	emit_reg((dreg),(reg));
}


void emit_mulsd_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x59;
	emit_reg((dreg),(reg));
}


void emit_subss_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x5c;
	emit_reg((dreg),(reg));
}


void emit_subsd_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x5c;
	emit_reg((dreg),(reg));
}


void emit_ucomiss_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x2e;
	emit_reg((dreg),(reg));
}


void emit_ucomisd_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0x66;
	emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x2e;
	emit_reg((dreg),(reg));
}


void emit_xorps_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x57;
	emit_reg((dreg),(reg));
}


void emit_xorps_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	emit_rex(0,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x57;
	emit_membase(cd, (basereg),(disp),(dreg));
}


void emit_xorpd_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0x66;
	emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x57;
	emit_reg((dreg),(reg));
}


void emit_xorpd_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	*(cd->mcodeptr++) = 0x66;
	emit_rex(0,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x57;
	emit_membase(cd, (basereg),(disp),(dreg));
}


/* system instructions ********************************************************/

void emit_rdtsc(codegendata *cd)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x31;
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
