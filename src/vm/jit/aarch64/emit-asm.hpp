/* src/vm/jit/aarch64/emit-asm.hpp - emit asm instructions for Aarch64

   Copyright (C) 1996-2013
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO
   Copyright (C) 2009 Theobroma Systems Ltd.

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

#ifndef VM_JIT_AARCH64_EMIT_ASM_HPP_
#define VM_JIT_AARCH64_EMIT_ASM_HPP_

#include <cassert>
#include <cstdio>

#include "config.h"

#include "vm/types.hpp"
#include "vm/jit/codegen-common.hpp"

#define LSL(x,a) ((x) << a)
#define LSR(x,a) ((x) >> a)

/* Some instructions allow shifts of operands, these are for encoding them */
#define CODE_LSL	0
#define CODE_LSR	1
#define CODE_ASR	2

/* Inverts the least significatn bit of the passed value */
#define INVERT(x) ((x) ^ 1)

/* Condition codes, some instructions allow such a code to be passed as argument */
#define COND_EQ		0x0
#define COND_NE		0x1
#define COND_CS		0x2
#define COND_CC		0x3
#define COND_MI		0x4
#define COND_PL		0x5
#define COND_VS		0x6
#define COND_VC		0x7
#define COND_HI		0x8
#define COND_LS		0x9
#define COND_GE		0xa
#define COND_LT		0xb
#define COND_GT		0xc
#define COND_LE		0xd
#define COND_AL		0xe
#define COND_NV		0xf


/* Compare & branch (immediate) **********************************************/

inline void emit_cmp_branch_imm(codegendata *cd, u1 sf, u1 op, s4 imm, u1 Rt)
{
	*((u4 *) cd->mcodeptr) = LSL(sf, 31) | LSL(op, 24)  
						     | LSL(imm, 5) | Rt | 0x34000000;
	cd->mcodeptr += 4;
}

#define emit_cbnz(cd, Xt, imm)	emit_cmp_branch_imm(cd, 1, 1, imm, Xt)


/* Conditional branch (immediate) ********************************************/

inline void emit_cond_branch_imm(codegendata *cd, s4 imm19, u1 cond)
{
	*((u4 *) cd->mcodeptr) = LSL((imm19 & 0x7ffff), 5) | cond | 0x54000000;
	cd->mcodeptr += 4;
}

#define emit_br_eq(cd, imm)		emit_cond_branch_imm(cd, imm, COND_EQ)
#define emit_br_ne(cd, imm)		emit_cond_branch_imm(cd, imm, COND_NE)
#define emit_br_ge(cd, imm)		emit_cond_branch_imm(cd, imm, COND_GE)
#define emit_br_lt(cd, imm)		emit_cond_branch_imm(cd, imm, COND_LT)
#define emit_br_gt(cd, imm)		emit_cond_branch_imm(cd, imm, COND_GT)
#define emit_br_le(cd, imm)		emit_cond_branch_imm(cd, imm, COND_LE)
#define emit_br_vs(cd, imm)		emit_cond_branch_imm(cd, imm, COND_VS)
#define emit_br_vc(cd, imm)		emit_cond_branch_imm(cd, imm, COND_VC)


/* Unconditional branch (immediate) ******************************************/

inline void emit_unc_branch_imm(codegendata *cd, u1 op, s4 imm26)
{
	// TODO sanity check imm26
	*((u4 *) cd->mcodeptr) = LSL(op, 31) | (imm26 & 0x3ffffff) | 0x14000000;
	cd->mcodeptr += 4;
}

#define emit_br_imm(cd, imm)	emit_unc_branch_imm(cd, 0, imm)
#define emit_blr_imm(cd, imm)	emit_unc_branch_imm(cd, 1, imm)
	

/* Unconditional branch (register) *******************************************/

inline void emit_unc_branch_reg(codegendata *cd, u1 opc, u1 op2, u1 op3, u1 Rn, u1 op4)
{
	*((u4 *) cd->mcodeptr) = LSL(opc, 21) | LSL(op2, 16) | LSL(op3, 10) 
						     | LSL(Rn, 5) | op4 | 0xD6000000;
	cd->mcodeptr += 4;
}

#define emit_ret(cd)			emit_unc_branch_reg(cd, 2, 31, 0, 30, 0)
#define emit_blr_reg(cd, Xn)	emit_unc_branch_reg(cd, 1, 31, 0, Xn, 0)

#define emit_br_reg(cd, Xn)		emit_unc_branch_reg(cd, 0, 31, 0, Xn, 0)


/* Move wide (immediate) *****************************************************/

inline void emit_mov_wide_imm(codegendata *cd, u1 sf, u1 opc, u1 hw, u2 imm16, u1 Rd)
{
	*((u4 *) cd->mcodeptr) = LSL(sf, 31) | LSL(opc, 29) | LSL(hw, 21) 
						     | LSL(imm16, 5) | Rd | 0x12800000; 
	cd->mcodeptr += 4;
}

#define emit_mov_imm(cd, Xd, imm)	emit_mov_wide_imm(cd, 1, 2, 0, imm, Xd)
#define emit_movn_imm(cd, Xd, imm)	emit_mov_wide_imm(cd, 1, 0, 0, imm, Xd)


/* Load/Store Register (unscaled immediate) ***********************************/

inline void emit_ldstr_reg_usc(codegendata *cd, u1 size, u1 v, u1 opc, s2 imm9, u1 Rt, u1 Rn)
{
	*((u4 *) cd->mcodeptr) = LSL(size, 30) | LSL(v, 26) | LSL(opc, 22)  
							 | LSL(imm9 & 0x1ff, 12) | LSL(Rn, 5) | Rt | 0x38400000;
	cd->mcodeptr += 4;
}


/* Load/Store Register (register offset) *************************************/

inline void emit_ldstr_reg_reg(codegendata *cd, u1 size, u1 v, u1 opc, u1 Rm, u1 option, u1 S, u1 Rn, u1 Rt)
{
	*((u4 *) cd->mcodeptr) = LSL(size, 30) | LSL(v, 26) | LSL(opc, 22)  
							 | LSL(Rm, 16) | LSL(option, 13) | LSL(S, 12)
							 | LSL(Rn, 5) | Rt | 0x38200800;
	cd->mcodeptr += 4;
}

#define emit_ldr_reg(cd, Xt, Xn, Xm)	emit_ldstr_reg_reg(cd, 3, 0, 1, Xm, 3, 0, Xn, Xt)


/* Load/Store Register (unsigned immediate) **********************************/

inline void emit_ldstr_reg_us(codegendata *cd, u1 size, u1 v, u1 opc, u2 imm12, u1 Rt, u1 Rn)
{
	u2 imm = LSR(imm12, size); /* 64bit: imm = imm12/8, 32bit: imm = imm12/4 */

	*((u4 *) cd->mcodeptr) = LSL(size, 30) | LSL(v, 26) | LSL(opc, 22) 
							 | LSL(imm, 10) | LSL(Rn, 5) | Rt | 0x39000000;
	cd->mcodeptr += 4;
}


/* Handle ambigous Load/Store instructions ***********************************/

/* See Armv8 reference manual for the ambigous case rules */
inline void emit_ldstr_ambigous(codegendata *cd, u1 size, u1 v, u1 opc, s2 imm, u1 Rt, u1 Rn)
{
	u1 sz = LSL(1, size); /* 64bit: 8, 32bit: 4 */

	/* handle ambigous case first */
	if (imm >= 0 && imm <= 255 && (imm % sz == 0))
		emit_ldstr_reg_us(cd, size, v, opc, imm, Rt, Rn);
	else if (imm >= -256 && imm <= 255)
		emit_ldstr_reg_usc(cd, size, v, opc, imm, Rt, Rn);
	else if (imm < 0 && (-imm) < 0xffff) { /* this is for larger negative offsets */
		// Should only be the case for INVOKESTATIC, which does not use REG_ITMP3
		emit_movn_imm(cd, REG_ITMP3, (-imm) - 1);
		emit_ldstr_reg_reg(cd, size, v, opc, REG_ITMP3, 3, 0, Rn, Rt);
	}
	else {
		assert(imm >= 0);
		assert(imm % sz == 0);
		assert(imm / sz <= 0xfff);
		emit_ldstr_reg_us(cd, size, v, opc, imm, Rt, Rn);
	}
}

#define emit_ldrh_imm(cd, Xt, Xn, imm)		emit_ldstr_ambigous(cd, 1, 0, 1, imm, Xt, Xn)
#define emit_strh_imm(cd, Xt, Xn, imm)		emit_ldstr_ambigous(cd, 1, 0, 0, imm, Xt, Xn)

#define emit_ldr_imm(cd, Xt, Xn, imm)		emit_ldstr_ambigous(cd, 3, 0, 1, imm, Xt, Xn)
#define emit_str_imm(cd, Xt, Xn, imm)		emit_ldstr_ambigous(cd, 3, 0, 0, imm, Xt, Xn)

#define emit_fp_ldr_imm(cd, Xt, Xn, imm)	emit_ldstr_ambigous(cd, 3, 1, 1, imm, Xt, Xn)
#define emit_fp_str_imm(cd, Xt, Xn, imm)	emit_ldstr_ambigous(cd, 3, 1, 0, imm, Xt, Xn)

#define emit_ldr_imm32(cd, Xt, Xn, imm)		emit_ldstr_ambigous(cd, 2, 0, 1, imm, Xt, Xn)
#define emit_str_imm32(cd, Xt, Xn, imm)		emit_ldstr_ambigous(cd, 2, 0, 0, imm, Xt, Xn)

#define emit_fp_ldr_imm32(cd, Xt, Xn, imm)	emit_ldstr_ambigous(cd, 2, 1, 1, imm, Xt, Xn)
#define emit_fp_str_imm32(cd, Xt, Xn, imm)	emit_ldstr_ambigous(cd, 2, 1, 0, imm, Xt, Xn)


/* Add/subtract (immediate) **************************************************/

inline void emit_addsub_imm(codegendata *cd, u1 sf, u1 op, u1 S, u1 shift, u2 imm12, u1 Rn, u1 Rd)
{
	assert(imm12 >= 0 && imm12 <= 0xfff);
	*((u4 *) cd->mcodeptr) = LSL(sf, 31) | LSL(op, 30) | LSL(S, 29) 
						     | LSL(shift, 22) | LSL(imm12, 10) | LSL(Rn, 5) 
							 | Rd | 0x11000000;
	cd->mcodeptr += 4;
}

inline void emit_addsub_imm(codegendata *cd, u1 sf, u1 op, u1 Xd, u1 Xn, u4 imm)
{
	assert(imm >= 0 && imm <= 0xffffff);
	u2 lo = imm & 0xfff;
	u2 hi = (imm & 0xfff000) >> 12;
	if (hi == 0)
		emit_addsub_imm(cd, sf, op, 0, 0, lo, Xn, Xd);
	else {
		emit_addsub_imm(cd, sf, op, 0, 1, hi, Xn, Xd);
		emit_addsub_imm(cd, sf, op, 0, 0, lo, Xd, Xd);
	}
}

#define emit_add_imm(cd, Xd, Xn, imm)	emit_addsub_imm(cd, 1, 0, Xd, Xn, imm)
#define emit_add_imm32(cd, Xd, Xn, imm)	emit_addsub_imm(cd, 0, 0, Xd, Xn, imm)

#define emit_sub_imm(cd, Xd, Xn, imm)	emit_addsub_imm(cd, 1, 1, Xd, Xn, imm)
#define emit_sub_imm32(cd, Xd, Xn, imm)	emit_addsub_imm(cd, 0, 1, Xd, Xn, imm)

#define emit_subs_imm(cd, Xd, Xn, imm)		emit_addsub_imm(cd, 1, 1, 1, 0, imm, Xn, Xd)
#define emit_subs_imm32(cd, Xd, Xn, imm)	emit_addsub_imm(cd, 0, 1, 1, 0, imm, Xn, Xd)

#define emit_adds_imm(cd, Xd, Xn, imm)		emit_addsub_imm(cd, 1, 0, 1, 0, imm, Xn, Xd)
#define emit_adds_imm32(cd, Xd, Xn, imm)	emit_addsub_imm(cd, 0, 0, 1, 0, imm, Xn, Xd)

#define emit_mov_sp(cd, Xd, Xn)			emit_add_imm(cd, Xd, Xn, 0)

#define emit_cmp_imm(cd, Xn, imm)		emit_subs_imm(cd, 31, Xn, imm)
#define emit_cmp_imm32(cd, Xn, imm)		emit_subs_imm32(cd, 31, Xn, imm)

#define emit_cmn_imm(cd, Xn, imm)		emit_adds_imm(cd, 31, Xn, imm)
#define emit_cmn_imm32(cd, Wn, imm)		emit_adds_imm32(cd, 31, Wn, imm)


/* Bitfield ******************************************************************/

inline void emit_bitfield(codegendata *cd, u1 sf, u1 opc, u1 N, u1 immr, u1 imms, u1 Rn, u1 Rd)
{
	assert(immr >= 0 && immr <= 0x3f);
	assert(imms >= 0 && imms <= 0x3f);
	*((u4 *) cd->mcodeptr) = LSL(sf, 31) | LSL(opc, 29) | LSL(N, 22) 
						     | LSL(immr, 16) | LSL(imms, 10) | LSL(Rn, 5) 
							 | Rd | 0x13000000;
	cd->mcodeptr += 4;
}

#define emit_ubfm(cd, Xd, Xn, immr, imms)	emit_bitfield(cd, 1, 2, 1, immr, imms, Xn, Xd)
#define emit_ubfm32(cd, Wd, Wn, immr, imms)	emit_bitfield(cd, 0, 2, 0, immr, imms, Wn, Wd)

#define emit_lsl_imm(cd, Xd, Xn, shift)		emit_ubfm(cd, Xd, Xn, ((u1)(-(shift))) % 64, 63 - (shift))
#define emit_lsl_imm32(cd, Wd, Wn, shift)	emit_ubfm32(cd, Wd, Wn, ((u1)(-(shift))) % 32, 31 - (shift))

#define emit_uxth(cd, Wd, Wn)				emit_ubfm32(cd, Wd, Wn, 0, 15)


/* Add/subtract (shifted register) *******************************************/

inline void emit_addsub_reg(codegendata *cd, u1 sf, u1 op, u1 S, u1 shift, u1 Rm, u1 imm6, u1 Rn, u1 Rd)
{
	assert(imm6 >= 0 && imm6 <= 0x3f);
	*((u4 *) cd->mcodeptr) = LSL(sf, 31) | LSL(op, 30) | LSL(S, 29) 
						     | LSL(shift, 22) | LSL(Rm, 16) | LSL(imm6, 10) 
							 | LSL(Rn, 5) | Rd | 0xB000000;
	cd->mcodeptr += 4;
}

#define emit_add_reg(cd, Xd, Xn, Xm)	emit_addsub_reg(cd, 1, 0, 0, 0, Xm, 0, Xn, Xd)
#define emit_sub_reg(cd, Xd, Xn, Xm)	emit_addsub_reg(cd, 1, 1, 0, 0, Xm, 0, Xn, Xd)

#define emit_subs_reg(cd, Xd, Xn, Xm)	emit_addsub_reg(cd, 1, 1, 1, 0, Xm, 0, Xn, Xd)
#define emit_subs_reg32(cd, Xd, Xn, Xm)	emit_addsub_reg(cd, 0, 1, 1, 0, Xm, 0, Xn, Xd)

#define emit_cmp_reg(cd, Xn, Xm)		emit_subs_reg(cd, 31, Xn, Xm)
#define emit_cmp_reg32(cd, Xn, Xm)		emit_subs_reg32(cd, 31, Xn, Xm)

#define emit_add_reg_shift(cd, Xd, Xn, Xm, s, a)	emit_addsub_reg(cd, 1, 0, 0, s, Xm, a, Xn, Xd)
#define emit_sub_reg_shift(cd, Xd, Xn, Xm, s, a)	emit_addsub_reg(cd, 1, 1, 0, s, Xm, a, Xn, Xd)

#define emit_add_reg32(cd, Xd, Xn, Xm)	emit_addsub_reg(cd, 0, 0, 0, 0, Xm, 0, Xn, Xd)
#define emit_sub_reg32(cd, Xd, Xn, Xm)	emit_addsub_reg(cd, 0, 1, 0, 0, Xm, 0, Xn, Xd)


/* Alpha like LDA ************************************************************/

inline void emit_lda(codegendata *cd, u1 Xd, u1 Xn, s4 imm)
{
	if (imm >= 0)
		emit_add_imm(cd, Xd, Xn, imm);
	else
		emit_sub_imm(cd, Xd, Xn, -imm);
}


/* Conditional select ********************************************************/

inline void emit_cond_select(codegendata *cd, u1 sf, u1 op, u1 S, u1 Rm, u1 cond, u1 op2, u1 Rn, u1 Rd)
{
	*((u4 *) cd->mcodeptr) = LSL(sf, 31) | LSL(op, 30) | LSL(S, 29) 
						     | LSL(Rm, 16) | LSL(cond, 12) | LSL(op2, 10) | LSL(Rn, 5) 
							 | Rd | 0x1a800000;
	cd->mcodeptr += 4;
}

#define emit_csel(cd, Xd, Xn, Xm, cond)		emit_cond_select(cd, 1, 0, 0, Xm, cond, 0, Xn, Xd)
#define emit_csinc(cd, Xd, Xn, Xm, cond)	emit_cond_select(cd, 1, 0, 0, Xm, cond, 1, Xn, Xd)
#define emit_cset(cd, Xd, cond)				emit_csinc(cd, Xd, 31, 31, INVERT(cond))

#define emit_csinv(cd, Xd, Xn, Xm, cond)	emit_cond_select(cd, 1, 1, 0, Xm, cond, 0, Xn, Xd)
#define emit_csetm(cd, Xd, cond)			emit_csinv(cd, Xd, 31, 31, INVERT(cond))

/* Data-processing (2 source) ************************************************/

inline void emit_dp2(codegendata *cd, u1 sf, u1 S, u1 Rm, u1 opc, u1 Rn, u1 Rd)
{
	*((u4 *) cd->mcodeptr) = LSL(sf, 31) | LSL(S, 29) | LSL(Rm, 16)
							 | LSL(opc, 10) | LSL(Rn, 5) | Rd
							 | 0x1AC00000;
	cd->mcodeptr += 4;
}

#define emit_sdiv(cd, Xd, Xn, Xm)		emit_dp2(cd, 1, 0, Xm, 3, Xn, Xd)
#define emit_sdiv32(cd, Wd, Wn, Wm)		emit_dp2(cd, 0, 0, Wm, 3, Wn, Wd)


/* Data-processing (3 source) ************************************************/

inline void emit_dp3(codegendata *cd, u1 sf, u1 op31, u1 Rm, u1 o0, u1 Ra, u1 Rn, u1 Rd) 
{
	*((u4 *) cd->mcodeptr) = LSL(sf, 31) | LSL(op31, 21) | LSL(Rm, 16)
							 | LSL(o0, 15) | LSL(Ra, 10) | LSL(Rn, 5)
							 | Rd | 0x1B000000;
	cd->mcodeptr += 4;
}

#define emit_madd(cd, Xd, Xn, Xm, Xa)		emit_dp3(cd, 1, 0, Xm, 0, Xa, Xn, Xd)
#define emit_madd32(cd, Xd, Xn, Xm, Xa)		emit_dp3(cd, 0, 0, Xm, 0, Xa, Xn, Xd)

#define emit_msub(cd, Xd, Xn, Xm, Xa)		emit_dp3(cd, 1, 0, Xm, 1, Xa, Xn, Xd)
#define emit_msub32(cd, Xd, Xn, Xm, Xa)		emit_dp3(cd, 0, 0, Xm, 1, Xa, Xn, Xd)

#define emit_mul(cd, Xd, Xn, Xm)			emit_madd(cd, Xd, Xn, Xm, 31)
#define emit_mul32(cd, Xd, Xn, Xm)			emit_madd32(cd, Xd, Xn, Xm, 31)


/* Logical (shifted register) ************************************************/

inline void emit_logical_sreg(codegendata *cd, u1 sf, u1 opc, u1 shift, u1 N, u1 Rm, u1 imm6, u1 Rn, u1 Rd)
{
	assert(imm6 >= 0 && imm6 <= 0x3f);
	*((u4 *) cd->mcodeptr) = LSL(sf, 31) | LSL(opc, 29) | LSL(shift, 22) 
						     | LSL(N, 21) | LSL(Rm, 16) | LSL(imm6, 10) | LSL(Rn, 5) 
							 | Rd | 0x0a000000;
	cd->mcodeptr += 4;
}

#define emit_orr_sreg(cd, Xd, Xn, Xm)	emit_logical_sreg(cd, 1, 1, 0, 0, Xm, 0, Xn, Xd)
#define emit_mov_reg(cd, Xd, Xm)		emit_orr_sreg(cd, Xd, 31, Xm)

#define emit_ands_sreg(cd, Xd, Xn, Xm)	emit_logical_sreg(cd, 1, 3, 0, 0, Xm, 0, Xn, Xd)
#define emit_tst_sreg(cd, Xn, Xm)		emit_ands_sreg(cd, 31, Xn, Xm)

#define emit_eor_sreg(cd, Xd, Xn, Xm)	emit_logical_sreg(cd, 1, 2, 0, 0, Xm, 0, Xn, Xd)
#define emit_clr(cd, Xd)				emit_eor_sreg(cd, Xd, Xd, Xd)

inline void emit_mov(codegendata *cd, u1 Xd, u1 Xm)
{
	if (Xd == 31 || Xm == 31)
		emit_mov_sp(cd, Xd, Xm);
	else
		emit_mov_reg(cd, Xd, Xm);
}


/* Floating-point move (register) ********************************************/

inline void emit_fp_mov(codegendata *cd, u1 type, u1 Rn, u1 Rd)
{
	*((u4 *) cd->mcodeptr) = LSL(type, 22) | LSL(Rn, 5) | Rd | 0x1E204000;
	cd->mcodeptr += 4;
}


/* Floating-point comparison *************************************************/

inline void emit_fp_cmp(codegendata *cd, u1 type, u1 Rm, u1 Rn, u1 opc)
{
	*((u4 *) cd->mcodeptr) = LSL(type, 22) | LSL(Rm, 16) | LSL(Rn, 5)
							 | LSL(opc, 3) | 0x1E202000;
	cd->mcodeptr += 4;
}


/* Floating-point data-processing (2 source) *********************************/

inline void emit_fp_dp2(codegendata *cd, u1 M, u1 S, u1 type, u1 Rm, u1 opc, u1 Rn, u1 Rd)
{
	*((u4 *) cd->mcodeptr) = LSL(M, 31) | LSL(S, 29) | LSL(type, 22) 
							 | LSL(Rm, 16) | LSL(opc, 12) | LSL(Rn, 5) 
							 | Rd | 0x1E200800;
	cd->mcodeptr += 4; 
}

#define emit_fmul(cd, type, Rd, Rn, Rm)		emit_fp_dp2(cd, 0, 0, type, Rm, 0, Rn, Rd)


/* Conversion between floating-point and integer *****************************/

inline void emit_conversion_fp(codegendata *cd, u1 sf, u1 S, u1 type, u1 rmode, u1 opc, u1 Rn, u1 Rd)
{
	*((u4 *) cd->mcodeptr) = LSL(sf, 31) | LSL(S, 29) | LSL(type, 22) 
							 | LSL(rmode, 19) | LSL(opc, 16) | LSL(Rn, 5) 
							 | Rd | 0x1E200000;
	cd->mcodeptr += 4;
}

#define emit_scvtf(cd, sf, type, Rn, Rd)	emit_conversion_fp(cd, sf, 0, type, 0, 2, Rn, Rd)
#define emit_fcvtzs(cd, sf, type, Rn, Rd)	emit_conversion_fp(cd, sf, 0, type, 3, 0, Rn, Rd)


/* Traps *********************************************************************/

/**
 * - First byte is the register.
 * - Second byte is the type of the trap.
 * - Bit 27 and 28 have to be 0 to be in the unallocated encoding space so we
 *   use E7 as our mark byte.
 */
inline void emit_trap(codegendata *cd, u1 Xd, int type)
{
	*((u4 *) cd->mcodeptr) = Xd | LSL(type & 0xff, 8) | 0xE7000000;
	cd->mcodeptr += 4;
}


/* Barriers ******************************************************************/

inline void emit_sync(codegendata *cd, u1 option, u1 opc) 
{
	*((u4 *) cd->mcodeptr) = LSL(option & 0xf, 8) | LSL(opc & 0x3, 5) | 0xD503309F;
	cd->mcodeptr += 4;
}

#define emit_dmb(cd, option)	emit_sync(cd, option, 1);
#define emit_dsb(cd, option)	emit_sync(cd, option, 0);


/* Misc **********************************************************************/

inline void emit_nop(codegendata *cd)
{
    *((u4 *) cd->mcodeptr) = (0xd503201f);
    cd->mcodeptr += 4; 
}

#endif // VM_JIT_AARCH64_EMIT_ASM_HPP_

/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
