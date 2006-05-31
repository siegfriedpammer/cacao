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

   $Id: emit.c 4994 2006-05-31 12:33:40Z twisti $

*/


#include "vm/types.h"

#include "md-abi.h"

#include "vm/jit/codegen-common.h"
#include "vm/jit/emit.h"
#include "vm/jit/jit.h"
#include "vm/jit/x86_64/codegen.h"
#include "vm/jit/x86_64/md-emit.h"


/* code generation functions **************************************************/

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
	s4            s;
	u2            opcode;

	/* get required compiler data */

	cd = jd->cd;
	rd = jd->rd;

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
	switch ((iptr->opc & ICMD_CONDITION_MASK) >> 8) {
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
}


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
