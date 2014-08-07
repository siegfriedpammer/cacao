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

#include "config.h"

#include "vm/types.hpp"
#include "vm/jit/codegen-common.hpp"

#define LSL(x,a) ((x) << a)
#define LSR(x,a) ((x) >> a)

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
	*((u4 *) cd->mcodeptr) = LSL(imm19, 5) | cond | 0x54000000;
	cd->mcodeptr += 4;
}

#define emit_br_eq(cd, imm)		emit_cond_branch_imm(cd, imm, 0)
#define emit_br_ne(cd, imm)		emit_cond_branch_imm(cd, imm, 1)
#define emit_br_ge(cd, imm)		emit_cond_branch_imm(cd, imm, 10)
#define emit_br_lt(cd, imm)		emit_cond_branch_imm(cd, imm, 11)
#define emit_br_gt(cd, imm)		emit_cond_branch_imm(cd, imm, 12)
#define emit_br_le(cd, imm)		emit_cond_branch_imm(cd, imm, 13)


/* Unconditional branch (register) *******************************************/

inline void emit_unc_branch_reg(codegendata *cd, u1 opc, u1 op2, u1 op3, u1 Rn, u1 op4)
{
	*((u4 *) cd->mcodeptr) = LSL(opc, 21) | LSL(op2, 16) | LSL(op3, 10) 
						     | LSL(Rn, 5) | op4 | 0xD6000000;
	cd->mcodeptr += 4;
}

#define emit_ret(cd)		emit_unc_branch_reg(cd, 2, 31, 0, 30, 0)
#define emit_blr(cd, Xn)	emit_unc_branch_reg(cd, 1, 31, 0, Xn, 0)

#define emit_br_aa(cd, Xn)	emit_unc_branch_reg(cd, 0, 31, 0, Xn, 0)


/* Load/Store Register (unscaled immediate) ***********************************/

inline void emit_ldstr_reg_usc(codegendata *cd, u1 size, u1 v, u1 opc, s2 imm9, u1 Rt, u1 Rn)
{
	*((u4 *) cd->mcodeptr) = LSL(size, 30) | LSL(v, 26) | LSL(opc, 22)  
							 | LSL(imm9 & 0x1ff, 12) | LSL(Rn, 5) | Rt | 0x38400000;
	cd->mcodeptr += 4;
}

#define emit_ldur(cd, Xt, Xn, imm9)		emit_ldstr_reg_usc(cd, 3, 0, 1, imm9, Xt, Xn)
#define emit_stur(cd, Xt, Xn, imm9)     emit_ldstr_reg_usc(cd, 3, 0, 0, imm9, Xt, Xn)

#define emit_ldur32(cd, Xt, Xn, imm9) 	emit_ldstr_reg_usc(cd, 2, 0, 1, imm9, Xt, Xn)
#define emit_stur32(cd, Xt, Xn, imm9)   emit_ldstr_reg_usc(cd, 2, 0, 0, imm9, Xt, Xn)

/* Load/Store Register (unsigned immediate) **********************************/

inline void emit_ldstr_reg_us(codegendata *cd, u1 size, u1 v, u1 opc, u2 imm12, u1 Rt, u1 Rn)
{
	*((u4 *) cd->mcodeptr) = LSL(size, 30) | LSL(v, 26) | LSL(opc, 22) 
							 | LSL(imm12/8, 10) | LSL(Rn, 5) | Rt | 0x39000000;
	cd->mcodeptr += 4;
}

#define emit_str_uo(cd, Xt, Xn, imm12)		emit_ldstr_reg_us(cd, 3, 0, 0, imm12, Xt, Xn)
#define emit_ldr_uo(cd, Xt, Xn, imm12)		emit_ldstr_reg_us(cd, 3, 0, 1, imm12, Xt, Xn)

#define emit_str_uo32(cd, Xt, Xn, imm12)	emit_ldstr_reg_us(cd, 2, 0, 0, imm12, Xt, Xn)
#define emit_ldr_uo32(cd, Xt, Xn, imm12)	emit_ldstr_reg_us(cd, 2, 0, 1, imm12, Xt, Xn)

/* Handle ambigous Load/Store instructions ***********************************/
/* TODO: Generalize this */

inline void emit_ldr_imm(codegendata *cd, u1 Xt, u1 Xn, s2 imm)
{
	/* handle ambigous case first */
	if (imm >= 0 && imm <= 255 && (imm % 8 == 0)) 
		emit_ldr_uo(cd, Xt, Xn, imm);
	else if (imm >= -256 && imm <= 255)
		emit_ldur(cd, Xt, Xn, imm);
	else {
		assert(imm >= 0);
		assert(imm % 8 == 0);
		emit_ldr_uo(cd, Xt, Xn, imm);
	}
}

inline void emit_str_imm(codegendata *cd, u1 Xt, u1 Xn, s2 imm)
{
	/* handle ambigous case first */
	if (imm >= 0 && imm <= 255 && (imm % 8 == 0)) 
		emit_str_uo(cd, Xt, Xn, imm);
	else if (imm >= -256 && imm <= 255)
		emit_stur(cd, Xt, Xn, imm);
	else {
		assert(imm >= 0);
		assert(imm % 8 == 0);
		emit_str_uo(cd, Xt, Xn, imm);
	}
}

inline void emit_ldr_imm32(codegendata *cd, u1 Xt, u1 Xn, s2 imm)
{
	/* handle ambigous case first */
	if (imm >= 0 && imm <= 255 && (imm % 8 == 0)) 
		emit_ldr_uo32(cd, Xt, Xn, imm);
	else if (imm >= -256 && imm <= 255)
		emit_ldur32(cd, Xt, Xn, imm);
	else {
		assert(imm >= 0);
		assert(imm % 8 == 0);
		emit_ldr_uo32(cd, Xt, Xn, imm);
	}
}

inline void emit_str_imm32(codegendata *cd, u1 Xt, u1 Xn, s2 imm)
{
	/* handle ambigous case first */
	if (imm >= 0 && imm <= 255 && (imm % 8 == 0)) 
		emit_str_uo32(cd, Xt, Xn, imm);
	else if (imm >= -256 && imm <= 255)
		emit_stur32(cd, Xt, Xn, imm);
	else {
		assert(imm >= 0);
		assert(imm % 8 == 0);
		emit_str_uo32(cd, Xt, Xn, imm);
	}
}


/* Add/subtract (immediate) **************************************************/

inline void emit_addsub_imm(codegendata *cd, u1 sf, u1 op, u1 S, u1 shift, u2 imm12, u1 Rn, u1 Rd)
{
	assert(imm12 >= 0 && imm12 <= 0xfff);
	*((u4 *) cd->mcodeptr) = LSL(sf, 31) | LSL(op, 30) | LSL(S, 29) 
						     | LSL(shift, 22) | LSL(imm12, 10) | LSL(Rn, 5) 
							 | Rd | 0x11000000;
	cd->mcodeptr += 4;
}

inline void emit_addsub_imm(codegendata *cd, u1 op, u1 Xd, u1 Xn, u4 imm)
{
	assert(imm >= 0 && imm <= 0xffffff);
	u2 lo = imm & 0xfff;
	u2 hi = (imm & 0xfff000) >> 12;
	if (hi == 0)
		emit_addsub_imm(cd, 1, op, 0, 0, lo, Xn, Xd);
	else {
		emit_addsub_imm(cd, 1, op, 0, 1, hi, Xn, Xd);
		emit_addsub_imm(cd, 1, op, 0, 0, lo, Xd, Xd);
	}
}

#define emit_add_imm(cd, Xd, Xn, imm)	emit_addsub_imm(cd, 0, Xd, Xn, imm)
#define emit_sub_imm(cd, Xd, Xn, imm)	emit_addsub_imm(cd, 1, Xd, Xn, imm)
#define emit_mov_sp(cd, Xd, Xn)			emit_add_imm(cd, Xd, Xn, 0)

#define emit_subs_imm(cd, Xd, Xn, imm)	emit_addsub_imm(cd, 1, 1, 1, 0, imm, Xn, Xd)
#define emit_cmp_imm(cd, Xn, imm)		emit_subs_imm(cd, 31, Xn, imm)


/* Move wide (immediate) *****************************************************/

inline void emit_mov_wide_imm(codegendata *cd, u1 sf, u1 opc, u1 hw, u2 imm16, u1 Rd)
{
	*((u4 *) cd->mcodeptr) = LSL(sf, 31) | LSL(opc, 29) | LSL(hw, 21) 
						     | LSL(imm16, 5) | Rd | 0x12800000; 
	cd->mcodeptr += 4;
}

#define emit_mov_imm(cd, Xd, imm)	emit_mov_wide_imm(cd, 1, 2, 0, imm, Xd)


/* Alpha like LDA ************************************************************/

inline void emit_lda(codegendata *cd, u1 Xd, u1 Xn, s4 imm)
{
	if (imm >= 0)
		emit_add_imm(cd, Xd, Xn, imm);
	else
		emit_sub_imm(cd, Xd, Xn, -imm);
}


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

inline void emit_mov(codegendata *cd, u1 Xd, u1 Xm)
{
	if (Xd == 31 || Xm == 31)
		emit_mov_sp(cd, Xd, Xm);
	else
		emit_mov_reg(cd, Xd, Xm);
}


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
