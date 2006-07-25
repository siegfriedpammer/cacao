/* vm/jit/i386/emit.c - i386 code emitter functions

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

   $Id: emit.c 5173 2006-07-25 15:57:11Z twisti $

*/


#include "config.h"

#include <assert.h>

#include "vm/types.h"

#include "vm/statistics.h"
#include "vm/jit/emit.h"
#include "vm/jit/jit.h"
#include "vm/jit/i386/md-abi.h"
#include "vm/jit/i386/md-emit.h"
#include "vm/jit/i386/codegen.h"


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

		disp = src->regoff * 4;

		if (IS_FLT_DBL_TYPE(src->type)) {
			if (IS_2_WORD_TYPE(src->type))
				M_DLD(tempreg, REG_SP, disp);
			else
				M_FLD(tempreg, REG_SP, disp);

		} else {
			if (IS_2_WORD_TYPE(src->type))
				M_LLD(tempreg, REG_SP, disp);
			else
				M_ILD(tempreg, REG_SP, disp);
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

		disp = src->regoff * 4;

		if (IS_FLT_DBL_TYPE(src->type)) {
			if (IS_2_WORD_TYPE(src->type))
				M_DLD(tempreg, REG_SP, disp);
			else
				M_FLD(tempreg, REG_SP, disp);

		} else {
			if (IS_2_WORD_TYPE(src->type))
				M_LLD(tempreg, REG_SP, disp);
			else
				M_ILD(tempreg, REG_SP, disp);
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

		disp = src->regoff * 4;

		if (IS_FLT_DBL_TYPE(src->type)) {
			if (IS_2_WORD_TYPE(src->type))
				M_DLD(tempreg, REG_SP, disp);
			else
				M_FLD(tempreg, REG_SP, disp);

		} else {
			if (IS_2_WORD_TYPE(src->type))
				M_LLD(tempreg, REG_SP, disp);
			else
				M_ILD(tempreg, REG_SP, disp);
		}

		reg = tempreg;
	} else
		reg = src->regoff;

	return reg;
}


/* emit_load_s1_low ************************************************************

   Emits a possible load of the low 32-bits of the first long source
   operand.

*******************************************************************************/

s4 emit_load_s1_low(jitdata *jd, instruction *iptr, stackptr src, s4 tempreg)
{
	codegendata  *cd;
	s4            disp;
	s4            reg;

	assert(src->type == TYPE_LNG);

	/* get required compiler data */

	cd = jd->cd;

	if (src->flags & INMEMORY) {
		COUNT_SPILLS;

		disp = src->regoff * 4;

		M_ILD(tempreg, REG_SP, disp);

		reg = tempreg;
	} else
		reg = GET_LOW_REG(src->regoff);

	return reg;
}


/* emit_load_s2_low ************************************************************

   Emits a possible load of the low 32-bits of the second long source
   operand.

*******************************************************************************/

s4 emit_load_s2_low(jitdata *jd, instruction *iptr, stackptr src, s4 tempreg)
{
	codegendata  *cd;
	s4            disp;
	s4            reg;

	assert(src->type == TYPE_LNG);

	/* get required compiler data */

	cd = jd->cd;

	if (src->flags & INMEMORY) {
		COUNT_SPILLS;

		disp = src->regoff * 4;

		M_ILD(tempreg, REG_SP, disp);

		reg = tempreg;
	} else
		reg = GET_LOW_REG(src->regoff);

	return reg;
}


/* emit_load_s1_high ***********************************************************

   Emits a possible load of the high 32-bits of the first long source
   operand.

*******************************************************************************/

s4 emit_load_s1_high(jitdata *jd, instruction *iptr, stackptr src, s4 tempreg)
{
	codegendata  *cd;
	s4            disp;
	s4            reg;

	assert(src->type == TYPE_LNG);

	/* get required compiler data */

	cd = jd->cd;

	if (src->flags & INMEMORY) {
		COUNT_SPILLS;

		disp = src->regoff * 4;

		M_ILD(tempreg, REG_SP, disp + 4);

		reg = tempreg;
	} else
		reg = GET_HIGH_REG(src->regoff);

	return reg;
}


/* emit_load_s2_high ***********************************************************

   Emits a possible load of the high 32-bits of the second long source
   operand.

*******************************************************************************/

s4 emit_load_s2_high(jitdata *jd, instruction *iptr, stackptr src, s4 tempreg)
{
	codegendata  *cd;
	s4            disp;
	s4            reg;

	assert(src->type == TYPE_LNG);

	/* get required compiler data */

	cd = jd->cd;

	if (src->flags & INMEMORY) {
		COUNT_SPILLS;

		disp = src->regoff * 4;

		M_ILD(tempreg, REG_SP, disp + 4);

		reg = tempreg;
	} else
		reg = GET_HIGH_REG(src->regoff);

	return reg;
}


/* emit_store ******************************************************************

   Emits a possible store of the destination operand.

*******************************************************************************/

void emit_store(jitdata *jd, instruction *iptr, stackptr dst, s4 d)
{
	codegendata  *cd;

	/* get required compiler data */

	cd = jd->cd;

	if (dst->flags & INMEMORY) {
		COUNT_SPILLS;

		if (IS_FLT_DBL_TYPE(dst->type)) {
			if (IS_2_WORD_TYPE(dst->type))
				M_DST(d, REG_SP, dst->regoff * 4);
			else
				M_FST(d, REG_SP, dst->regoff * 4);

		} else {
			if (IS_2_WORD_TYPE(dst->type))
				M_LST(d, REG_SP, dst->regoff * 4);
			else
				M_IST(d, REG_SP, dst->regoff * 4);
		}
	}
}


/* emit_store_low **************************************************************

   Emits a possible store of the low 32-bits of the destination
   operand.

*******************************************************************************/

void emit_store_low(jitdata *jd, instruction *iptr, stackptr dst, s4 d)
{
	codegendata  *cd;

	assert(dst->type == TYPE_LNG);

	/* get required compiler data */

	cd = jd->cd;

	if (dst->flags & INMEMORY) {
		COUNT_SPILLS;
		M_IST(GET_LOW_REG(d), REG_SP, dst->regoff * 4);
	}
}


/* emit_store_high *************************************************************

   Emits a possible store of the high 32-bits of the destination
   operand.

*******************************************************************************/

void emit_store_high(jitdata *jd, instruction *iptr, stackptr dst, s4 d)
{
	codegendata  *cd;

	assert(dst->type == TYPE_LNG);

	/* get required compiler data */

	cd = jd->cd;

	if (dst->flags & INMEMORY) {
		COUNT_SPILLS;
		M_IST(GET_HIGH_REG(d), REG_SP, dst->regoff * 4 + 4);
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
		if (IS_LNG_TYPE(src->type))
			d = codegen_reg_of_var(rd, iptr->opc, dst, REG_ITMP12_PACKED);
		else
			d = codegen_reg_of_var(rd, iptr->opc, dst, REG_ITMP1);

		s1 = emit_load_s1(jd, iptr, src, d);

		if (s1 != d) {
			if (IS_FLT_DBL_TYPE(src->type)) {
/* 				M_FMOV(s1, d); */
			} else {
				if (IS_2_WORD_TYPE(src->type))
					M_LNGMOVE(s1, d);
				else
                    M_MOV(s1, d);
			}
		}

		emit_store(jd, iptr, dst, d);
	}
}


/* code generation functions **************************************************/

static void emit_membase(codegendata *cd, s4 basereg, s4 disp, s4 dreg)
{
	if (basereg == ESP) {
		if (disp == 0) {
			emit_address_byte(0, dreg, ESP);
			emit_address_byte(0, ESP, ESP);
		}
		else if (IS_IMM8(disp)) {
			emit_address_byte(1, dreg, ESP);
			emit_address_byte(0, ESP, ESP);
			emit_imm8(disp);
		}
		else {
			emit_address_byte(2, dreg, ESP);
			emit_address_byte(0, ESP, ESP);
			emit_imm32(disp);
		}
	}
	else if ((disp == 0) && (basereg != EBP)) {
		emit_address_byte(0, dreg, basereg);
	}
	else if (IS_IMM8(disp)) {
		emit_address_byte(1, dreg, basereg);
		emit_imm8(disp);
	}
	else {
		emit_address_byte(2, dreg, basereg);
		emit_imm32(disp);
	}
}


static void emit_membase32(codegendata *cd, s4 basereg, s4 disp, s4 dreg)
{
	if (basereg == ESP) {
		emit_address_byte(2, dreg, ESP);
		emit_address_byte(0, ESP, ESP);
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
	else if ((disp == 0) && (basereg != EBP)) {
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


/* low-level code emitter functions *******************************************/

void emit_mov_reg_reg(codegendata *cd, s4 reg, s4 dreg)
{
	COUNT(count_mov_reg_reg);
	*(cd->mcodeptr++) = 0x89;
	emit_reg((reg),(dreg));
}


void emit_mov_imm_reg(codegendata *cd, s4 imm, s4 reg)
{
	*(cd->mcodeptr++) = 0xb8 + ((reg) & 0x07);
	emit_imm32((imm));
}


void emit_movb_imm_reg(codegendata *cd, s4 imm, s4 reg)
{
	*(cd->mcodeptr++) = 0xc6;
	emit_reg(0,(reg));
	emit_imm8((imm));
}


void emit_mov_membase_reg(codegendata *cd, s4 basereg, s4 disp, s4 reg)
{
	COUNT(count_mov_mem_reg);
	*(cd->mcodeptr++) = 0x8b;
	emit_membase(cd, (basereg),(disp),(reg));
}


/*
 * this one is for INVOKEVIRTUAL/INVOKEINTERFACE to have a
 * constant membase immediate length of 32bit
 */
void emit_mov_membase32_reg(codegendata *cd, s4 basereg, s4 disp, s4 reg)
{
	COUNT(count_mov_mem_reg);
	*(cd->mcodeptr++) = 0x8b;
	emit_membase32(cd, (basereg),(disp),(reg));
}


void emit_mov_reg_membase(codegendata *cd, s4 reg, s4 basereg, s4 disp)
{
	COUNT(count_mov_reg_mem);
	*(cd->mcodeptr++) = 0x89;
	emit_membase(cd, (basereg),(disp),(reg));
}


void emit_mov_reg_membase32(codegendata *cd, s4 reg, s4 basereg, s4 disp)
{
	COUNT(count_mov_reg_mem);
	*(cd->mcodeptr++) = 0x89;
	emit_membase32(cd, (basereg),(disp),(reg));
}


void emit_mov_memindex_reg(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg)
{
	COUNT(count_mov_mem_reg);
	*(cd->mcodeptr++) = 0x8b;
	emit_memindex(cd, (reg),(disp),(basereg),(indexreg),(scale));
}


void emit_mov_reg_memindex(codegendata *cd, s4 reg, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	COUNT(count_mov_reg_mem);
	*(cd->mcodeptr++) = 0x89;
	emit_memindex(cd, (reg),(disp),(basereg),(indexreg),(scale));
}


void emit_movw_reg_memindex(codegendata *cd, s4 reg, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	COUNT(count_mov_reg_mem);
	*(cd->mcodeptr++) = 0x66;
	*(cd->mcodeptr++) = 0x89;
	emit_memindex(cd, (reg),(disp),(basereg),(indexreg),(scale));
}


void emit_movb_reg_memindex(codegendata *cd, s4 reg, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	COUNT(count_mov_reg_mem);
	*(cd->mcodeptr++) = 0x88;
	emit_memindex(cd, (reg),(disp),(basereg),(indexreg),(scale));
}


void emit_mov_reg_mem(codegendata *cd, s4 reg, s4 mem)
{
	COUNT(count_mov_reg_mem);
	*(cd->mcodeptr++) = 0x89;
	emit_mem((reg),(mem));
}


void emit_mov_mem_reg(codegendata *cd, s4 mem, s4 dreg)
{
	COUNT(count_mov_mem_reg);
	*(cd->mcodeptr++) = 0x8b;
	emit_mem((dreg),(mem));
}


void emit_mov_imm_mem(codegendata *cd, s4 imm, s4 mem)
{
	*(cd->mcodeptr++) = 0xc7;
	emit_mem(0, mem);
	emit_imm32(imm);
}


void emit_mov_imm_membase(codegendata *cd, s4 imm, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xc7;
	emit_membase(cd, (basereg),(disp),0);
	emit_imm32((imm));
}


void emit_mov_imm_membase32(codegendata *cd, s4 imm, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xc7;
	emit_membase32(cd, (basereg),(disp),0);
	emit_imm32((imm));
}


void emit_movb_imm_membase(codegendata *cd, s4 imm, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xc6;
	emit_membase(cd, (basereg),(disp),0);
	emit_imm8((imm));
}


void emit_movsbl_memindex_reg(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg)
{
	COUNT(count_mov_mem_reg);
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xbe;
	emit_memindex(cd, (reg),(disp),(basereg),(indexreg),(scale));
}


void emit_movswl_reg_reg(codegendata *cd, s4 a, s4 b)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xbf;
	emit_reg((b),(a));
}


void emit_movswl_memindex_reg(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg)
{
	COUNT(count_mov_mem_reg);
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xbf;
	emit_memindex(cd, (reg),(disp),(basereg),(indexreg),(scale));
}


void emit_movzwl_reg_reg(codegendata *cd, s4 a, s4 b)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xb7;
	emit_reg((b),(a));
}


void emit_movzwl_memindex_reg(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg)
{
	COUNT(count_mov_mem_reg);
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xb7;
	emit_memindex(cd, (reg),(disp),(basereg),(indexreg),(scale));
}


void emit_mov_imm_memindex(codegendata *cd, s4 imm, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	*(cd->mcodeptr++) = 0xc7;
	emit_memindex(cd, 0,(disp),(basereg),(indexreg),(scale));
	emit_imm32((imm));
}


void emit_movw_imm_memindex(codegendata *cd, s4 imm, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	*(cd->mcodeptr++) = 0x66;
	*(cd->mcodeptr++) = 0xc7;
	emit_memindex(cd, 0,(disp),(basereg),(indexreg),(scale));
	emit_imm16((imm));
}


void emit_movb_imm_memindex(codegendata *cd, s4 imm, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	*(cd->mcodeptr++) = 0xc6;
	emit_memindex(cd, 0,(disp),(basereg),(indexreg),(scale));
	emit_imm8((imm));
}


/*
 * alu operations
 */
void emit_alu_reg_reg(codegendata *cd, s4 opc, s4 reg, s4 dreg)
{
	*(cd->mcodeptr++) = (((u1) (opc)) << 3) + 1;
	emit_reg((reg),(dreg));
}


void emit_alu_reg_membase(codegendata *cd, s4 opc, s4 reg, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = (((u1) (opc)) << 3) + 1;
	emit_membase(cd, (basereg),(disp),(reg));
}


void emit_alu_membase_reg(codegendata *cd, s4 opc, s4 basereg, s4 disp, s4 reg)
{
	*(cd->mcodeptr++) = (((u1) (opc)) << 3) + 3;
	emit_membase(cd, (basereg),(disp),(reg));
}


void emit_alu_imm_reg(codegendata *cd, s4 opc, s4 imm, s4 dreg)
{
	if (IS_IMM8(imm)) { 
		*(cd->mcodeptr++) = 0x83;
		emit_reg((opc),(dreg));
		emit_imm8((imm));
	} else { 
		*(cd->mcodeptr++) = 0x81;
		emit_reg((opc),(dreg));
		emit_imm32((imm));
	} 
}


void emit_alu_imm32_reg(codegendata *cd, s4 opc, s4 imm, s4 dreg)
{
	*(cd->mcodeptr++) = 0x81;
	emit_reg((opc),(dreg));
	emit_imm32((imm));
}


void emit_alu_imm_membase(codegendata *cd, s4 opc, s4 imm, s4 basereg, s4 disp)
{
	if (IS_IMM8(imm)) { 
		*(cd->mcodeptr++) = 0x83;
		emit_membase(cd, (basereg),(disp),(opc));
		emit_imm8((imm));
	} else { 
		*(cd->mcodeptr++) = 0x81;
		emit_membase(cd, (basereg),(disp),(opc));
		emit_imm32((imm));
	} 
}


void emit_test_reg_reg(codegendata *cd, s4 reg, s4 dreg)
{
	*(cd->mcodeptr++) = 0x85;
	emit_reg((reg),(dreg));
}


void emit_test_imm_reg(codegendata *cd, s4 imm, s4 reg)
{
	*(cd->mcodeptr++) = 0xf7;
	emit_reg(0,(reg));
	emit_imm32((imm));
}



/*
 * inc, dec operations
 */
void emit_dec_mem(codegendata *cd, s4 mem)
{
	*(cd->mcodeptr++) = 0xff;
	emit_mem(1,(mem));
}


void emit_cltd(codegendata *cd)
{
	*(cd->mcodeptr++) = 0x99;
}


void emit_imul_reg_reg(codegendata *cd, s4 reg, s4 dreg)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xaf;
	emit_reg((dreg),(reg));
}


void emit_imul_membase_reg(codegendata *cd, s4 basereg, s4 disp, s4 dreg)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xaf;
	emit_membase(cd, (basereg),(disp),(dreg));
}


void emit_imul_imm_reg(codegendata *cd, s4 imm, s4 dreg)
{
	if (IS_IMM8((imm))) { 
		*(cd->mcodeptr++) = 0x6b;
		emit_reg(0,(dreg));
		emit_imm8((imm));
	} else { 
		*(cd->mcodeptr++) = 0x69;
		emit_reg(0,(dreg));
		emit_imm32((imm));
	} 
}


void emit_imul_imm_reg_reg(codegendata *cd, s4 imm, s4 reg, s4 dreg)
{
	if (IS_IMM8((imm))) { 
		*(cd->mcodeptr++) = 0x6b;
		emit_reg((dreg),(reg));
		emit_imm8((imm));
	} else { 
		*(cd->mcodeptr++) = 0x69;
		emit_reg((dreg),(reg));
		emit_imm32((imm));
	} 
}


void emit_imul_imm_membase_reg(codegendata *cd, s4 imm, s4 basereg, s4 disp, s4 dreg)
{
	if (IS_IMM8((imm))) {
		*(cd->mcodeptr++) = 0x6b;
		emit_membase(cd, (basereg),(disp),(dreg));
		emit_imm8((imm));
	} else {
		*(cd->mcodeptr++) = 0x69;
		emit_membase(cd, (basereg),(disp),(dreg));
		emit_imm32((imm));
	}
}


void emit_mul_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xf7;
	emit_reg(4, reg);
}


void emit_mul_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xf7;
	emit_membase(cd, (basereg),(disp),4);
}


void emit_idiv_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xf7;
	emit_reg(7,(reg));
}


void emit_ret(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xc3;
}



/*
 * shift ops
 */
void emit_shift_reg(codegendata *cd, s4 opc, s4 reg)
{
	*(cd->mcodeptr++) = 0xd3;
	emit_reg((opc),(reg));
}


void emit_shift_imm_reg(codegendata *cd, s4 opc, s4 imm, s4 dreg)
{
	if ((imm) == 1) {
		*(cd->mcodeptr++) = 0xd1;
		emit_reg((opc),(dreg));
	} else {
		*(cd->mcodeptr++) = 0xc1;
		emit_reg((opc),(dreg));
		emit_imm8((imm));
	}
}


void emit_shld_reg_reg(codegendata *cd, s4 reg, s4 dreg)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xa5;
	emit_reg((reg),(dreg));
}


void emit_shld_imm_reg_reg(codegendata *cd, s4 imm, s4 reg, s4 dreg)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xa4;
	emit_reg((reg),(dreg));
	emit_imm8((imm));
}


void emit_shld_reg_membase(codegendata *cd, s4 reg, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xa5;
	emit_membase(cd, (basereg),(disp),(reg));
}


void emit_shrd_reg_reg(codegendata *cd, s4 reg, s4 dreg)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xad;
	emit_reg((reg),(dreg));
}


void emit_shrd_imm_reg_reg(codegendata *cd, s4 imm, s4 reg, s4 dreg)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xac;
	emit_reg((reg),(dreg));
	emit_imm8((imm));
}


void emit_shrd_reg_membase(codegendata *cd, s4 reg, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xad;
	emit_membase(cd, (basereg),(disp),(reg));
}



/*
 * jump operations
 */
void emit_jmp_imm(codegendata *cd, s4 imm)
{
	*(cd->mcodeptr++) = 0xe9;
	emit_imm32((imm));
}


void emit_jmp_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xff;
	emit_reg(4,(reg));
}


void emit_jcc(codegendata *cd, s4 opc, s4 imm)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) =  0x80 + (u1) (opc);
	emit_imm32((imm));
}



/*
 * conditional set operations
 */
void emit_setcc_reg(codegendata *cd, s4 opc, s4 reg)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x90 + (u1) (opc);
	emit_reg(0,(reg));
}


void emit_setcc_membase(codegendata *cd, s4 opc, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) =  0x90 + (u1) (opc);
	emit_membase(cd, (basereg),(disp),0);
}


void emit_xadd_reg_mem(codegendata *cd, s4 reg, s4 mem)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xc1;
	emit_mem((reg),(mem));
}


void emit_neg_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xf7;
	emit_reg(3,(reg));
}



void emit_push_imm(codegendata *cd, s4 imm)
{
	*(cd->mcodeptr++) = 0x68;
	emit_imm32((imm));
}


void emit_pop_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0x58 + (0x07 & (u1) (reg));
}


void emit_push_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0x50 + (0x07 & (u1) (reg));
}


void emit_nop(codegendata *cd)
{
	*(cd->mcodeptr++) = 0x90;
}


void emit_lock(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xf0;
}


/*
 * call instructions
 */
void emit_call_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xff;
	emit_reg(2,(reg));
}


void emit_call_imm(codegendata *cd, s4 imm)
{
	*(cd->mcodeptr++) = 0xe8;
	emit_imm32((imm));
}



/*
 * floating point instructions
 */
void emit_fld1(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xd9;
	*(cd->mcodeptr++) = 0xe8;
}


void emit_fldz(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xd9;
	*(cd->mcodeptr++) = 0xee;
}


void emit_fld_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xd9;
	*(cd->mcodeptr++) = 0xc0 + (0x07 & (u1) (reg));
}


void emit_flds_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xd9;
	emit_membase(cd, (basereg),(disp),0);
}


void emit_flds_membase32(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xd9;
	emit_membase32(cd, (basereg),(disp),0);
}


void emit_fldl_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdd;
	emit_membase(cd, (basereg),(disp),0);
}


void emit_fldl_membase32(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdd;
	emit_membase32(cd, (basereg),(disp),0);
}


void emit_fldt_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdb;
	emit_membase(cd, (basereg),(disp),5);
}


void emit_flds_memindex(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	*(cd->mcodeptr++) = 0xd9;
	emit_memindex(cd, 0,(disp),(basereg),(indexreg),(scale));
}


void emit_fldl_memindex(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	*(cd->mcodeptr++) = 0xdd;
	emit_memindex(cd, 0,(disp),(basereg),(indexreg),(scale));
}


void emit_flds_mem(codegendata *cd, s4 mem)
{
	*(cd->mcodeptr++) = 0xd9;
	emit_mem(0,(mem));
}


void emit_fldl_mem(codegendata *cd, s4 mem)
{
	*(cd->mcodeptr++) = 0xdd;
	emit_mem(0,(mem));
}


void emit_fildl_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdb;
	emit_membase(cd, (basereg),(disp),0);
}


void emit_fildll_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdf;
	emit_membase(cd, (basereg),(disp),5);
}


void emit_fst_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xdd;
	*(cd->mcodeptr++) = 0xd0 + (0x07 & (u1) (reg));
}


void emit_fsts_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xd9;
	emit_membase(cd, (basereg),(disp),2);
}


void emit_fstl_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdd;
	emit_membase(cd, (basereg),(disp),2);
}


void emit_fsts_memindex(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	*(cd->mcodeptr++) = 0xd9;
	emit_memindex(cd, 2,(disp),(basereg),(indexreg),(scale));
}


void emit_fstl_memindex(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	*(cd->mcodeptr++) = 0xdd;
	emit_memindex(cd, 2,(disp),(basereg),(indexreg),(scale));
}


void emit_fstp_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xdd;
	*(cd->mcodeptr++) = 0xd8 + (0x07 & (u1) (reg));
}


void emit_fstps_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xd9;
	emit_membase(cd, (basereg),(disp),3);
}


void emit_fstps_membase32(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xd9;
	emit_membase32(cd, (basereg),(disp),3);
}


void emit_fstpl_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdd;
	emit_membase(cd, (basereg),(disp),3);
}


void emit_fstpl_membase32(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdd;
	emit_membase32(cd, (basereg),(disp),3);
}


void emit_fstpt_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdb;
	emit_membase(cd, (basereg),(disp),7);
}


void emit_fstps_memindex(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	*(cd->mcodeptr++) = 0xd9;
	emit_memindex(cd, 3,(disp),(basereg),(indexreg),(scale));
}


void emit_fstpl_memindex(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	*(cd->mcodeptr++) = 0xdd;
	emit_memindex(cd, 3,(disp),(basereg),(indexreg),(scale));
}


void emit_fstps_mem(codegendata *cd, s4 mem)
{
	*(cd->mcodeptr++) = 0xd9;
	emit_mem(3,(mem));
}


void emit_fstpl_mem(codegendata *cd, s4 mem)
{
	*(cd->mcodeptr++) = 0xdd;
	emit_mem(3,(mem));
}


void emit_fistl_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdb;
	emit_membase(cd, (basereg),(disp),2);
}


void emit_fistpl_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdb;
	emit_membase(cd, (basereg),(disp),3);
}


void emit_fistpll_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdf;
	emit_membase(cd, (basereg),(disp),7);
}


void emit_fchs(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xd9;
	*(cd->mcodeptr++) = 0xe0;
}


void emit_faddp(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xde;
	*(cd->mcodeptr++) = 0xc1;
}


void emit_fadd_reg_st(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xd8;
	*(cd->mcodeptr++) = 0xc0 + (0x0f & (u1) (reg));
}


void emit_fadd_st_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xdc;
	*(cd->mcodeptr++) = 0xc0 + (0x0f & (u1) (reg));
}


void emit_faddp_st_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xde;
	*(cd->mcodeptr++) = 0xc0 + (0x0f & (u1) (reg));
}


void emit_fadds_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xd8;
	emit_membase(cd, (basereg),(disp),0);
}


void emit_faddl_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdc;
	emit_membase(cd, (basereg),(disp),0);
}


void emit_fsub_reg_st(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xd8;
	*(cd->mcodeptr++) = 0xe0 + (0x07 & (u1) (reg));
}


void emit_fsub_st_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xdc;
	*(cd->mcodeptr++) = 0xe8 + (0x07 & (u1) (reg));
}


void emit_fsubp_st_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xde;
	*(cd->mcodeptr++) = 0xe8 + (0x07 & (u1) (reg));
}


void emit_fsubp(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xde;
	*(cd->mcodeptr++) = 0xe9;
}


void emit_fsubs_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xd8;
	emit_membase(cd, (basereg),(disp),4);
}


void emit_fsubl_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdc;
	emit_membase(cd, (basereg),(disp),4);
}


void emit_fmul_reg_st(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xd8;
	*(cd->mcodeptr++) = 0xc8 + (0x07 & (u1) (reg));
}


void emit_fmul_st_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xdc;
	*(cd->mcodeptr++) = 0xc8 + (0x07 & (u1) (reg));
}


void emit_fmulp(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xde;
	*(cd->mcodeptr++) = 0xc9;
}


void emit_fmulp_st_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xde;
	*(cd->mcodeptr++) = 0xc8 + (0x07 & (u1) (reg));
}


void emit_fmuls_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xd8;
	emit_membase(cd, (basereg),(disp),1);
}


void emit_fmull_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdc;
	emit_membase(cd, (basereg),(disp),1);
}


void emit_fdiv_reg_st(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xd8;
	*(cd->mcodeptr++) = 0xf0 + (0x07 & (u1) (reg));
}


void emit_fdiv_st_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xdc;
	*(cd->mcodeptr++) = 0xf8 + (0x07 & (u1) (reg));
}


void emit_fdivp(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xde;
	*(cd->mcodeptr++) = 0xf9;
}


void emit_fdivp_st_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xde;
	*(cd->mcodeptr++) = 0xf8 + (0x07 & (u1) (reg));
}


void emit_fxch(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xd9;
	*(cd->mcodeptr++) = 0xc9;
}


void emit_fxch_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xd9;
	*(cd->mcodeptr++) = 0xc8 + (0x07 & (reg));
}


void emit_fprem(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xd9;
	*(cd->mcodeptr++) = 0xf8;
}


void emit_fprem1(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xd9;
	*(cd->mcodeptr++) = 0xf5;
}


void emit_fucom(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xdd;
	*(cd->mcodeptr++) = 0xe1;
}


void emit_fucom_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xdd;
	*(cd->mcodeptr++) = 0xe0 + (0x07 & (u1) (reg));
}


void emit_fucomp_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xdd;
	*(cd->mcodeptr++) = 0xe8 + (0x07 & (u1) (reg));
}


void emit_fucompp(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xda;
	*(cd->mcodeptr++) = 0xe9;
}


void emit_fnstsw(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xdf;
	*(cd->mcodeptr++) = 0xe0;
}


void emit_sahf(codegendata *cd)
{
	*(cd->mcodeptr++) = 0x9e;
}


void emit_finit(codegendata *cd)
{
	*(cd->mcodeptr++) = 0x9b;
	*(cd->mcodeptr++) = 0xdb;
	*(cd->mcodeptr++) = 0xe3;
}


void emit_fldcw_mem(codegendata *cd, s4 mem)
{
	*(cd->mcodeptr++) = 0xd9;
	emit_mem(5,(mem));
}


void emit_fldcw_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xd9;
	emit_membase(cd, (basereg),(disp),5);
}


void emit_wait(codegendata *cd)
{
	*(cd->mcodeptr++) = 0x9b;
}


void emit_ffree_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xdd;
	*(cd->mcodeptr++) = 0xc0 + (0x07 & (u1) (reg));
}


void emit_fdecstp(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xd9;
	*(cd->mcodeptr++) = 0xf6;
}


void emit_fincstp(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xd9;
	*(cd->mcodeptr++) = 0xf7;
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
