/* src/vm/jit/aarch64/codegen.hpp - code generation macros and definitions for Aarch64

   Copyright (C) 1996-2013
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO
   Copyright (C) 2008 Theobroma Systems Ltd.

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


#ifndef CODEGEN_HPP_
#define CODEGEN_HPP_ 1

#include "config.h"
#include "emit-asm.hpp"
#include "vm/types.hpp"

#include "vm/jit/jit.hpp"
#include "vm/jit/dseg.hpp"


/* additional functions and macros to generate code ***************************/

#define MCODECHECK(icnt) \
    do { \
        if ((cd->mcodeptr + (icnt) * 4) > cd->mcodeend) \
            codegen_increase(cd); \
    } while (0)


#define ALIGNCODENOP \
    if ((s4) ((ptrint) cd->mcodeptr & 7)) { \
        M_NOP; \
    }


#define ICONST(d,c)        emit_iconst(cd, (d), (c))
#define LCONST(d,c)        emit_lconst(cd, (d), (c))

#define SHIFT(x,s)         ((x) << s)

/* branch defines *************************************************************/

#define BRANCH_NOPS \
    do { \
        M_NOP; \
    } while (0)


/* patcher defines ************************************************************/

#define PATCHER_CALL_SIZE    1 * 4     /* an instruction is 4-bytes long      */

#define PATCHER_NOPS \
    do { \
        M_NOP; \
    } while (0)


/* AsmEmitter ******************************************************************
 * 
 * Small wrapper around codegendata.
 *
 * Prefix indicate whether the instruction operates on the first 32-bit (i)
 * or on the whole register (l). Same goes for FP operations.
 *
 * Provides also some convinient methods, mainly 'lda', 'iconst' and 'lconst'
 *
 ******************************************************************************/

class AsmEmitter {
	
	public:
		explicit AsmEmitter(codegendata *cd) : cd(cd) {}

        void imov(u1 wd, u1 wn) { emit_mov_reg32(cd, wd, wn); }
        void mov(u1 xt, u1 xn) { emit_mov(cd, xt, xn); }

        void icmp_imm(u1 wd, u2 imm) { emit_cmp_imm32(cd, wd, imm); }
        void lcmp_imm(u1 xd, u2 imm) { emit_cmp_imm(cd, xd, imm); }
        void icmn_imm(u1 wd, u2 imm) { emit_cmn_imm32(cd, wd, imm); }
        void lcmn_imm(u1 xd, u2 imm) { emit_cmn_imm(cd, xd, imm); }

        void icmp(u1 wn, u1 wm) { emit_cmp_reg32(cd, wn, wm); }
        void lcmp(u1 xn, u1 xm) { emit_cmp_reg(cd, xn, xm); }
        void acmp(u1 xn, u1 xm) { emit_cmp_reg(cd, xn, xm); }

        void nop() { emit_nop(cd); }

        /* Load / store ********************************************************/
        void ild(u1 xt, u1 xn, s2 imm) { emit_ldr_imm32(cd, xt, xn, imm); }
        void lld(u1 xt, u1 xn, s2 imm) { emit_ldr_imm(cd, xt, xn, imm); }
        void ald(u1 xt, u1 xn, s2 imm) { emit_ldr_imm(cd, xt, xn, imm); }

        void fld(u1 xt, u1 xn, s2 imm) { emit_fp_ldr_imm32(cd, xt, xn, imm); }
        void dld(u1 xt, u1 xn, s2 imm) { emit_fp_ldr_imm(cd, xt, xn, imm); }

        void ldrh(u1 wt, u1 xn, s2 imm) { emit_ldrh_imm(cd, wt, xn, imm); }
        void ldrb(u1 wt, u1 xn, s2 imm) { emit_ldrb_imm(cd, wt, xn, imm); }

        void ldrsb32(u1 wt, u1 xn, s2 imm) { emit_ldrsb_imm32(cd, wt, xn, imm); }
        void ldrsh32(u1 wt, u1 xn, s2 imm) { emit_ldrsh_imm32(cd, wt, xn, imm); }

        void ist(u1 xt, u1 xn, s2 imm) { emit_str_imm32(cd, xt, xn, imm); }
        void lst(u1 xt, u1 xn, s2 imm) { emit_str_imm(cd, xt, xn, imm); }
        void ast(u1 xt, u1 xn, s2 imm) { emit_str_imm(cd, xt, xn, imm); }

        void fst(u1 xt, u1 xn, s2 imm) { emit_fp_str_imm32(cd, xt, xn, imm); }
        void dst(u1 xt, u1 xn, s2 imm) { emit_fp_str_imm(cd, xt, xn, imm); }

        void strh(u1 wt, u1 xn, s2 imm) { emit_strh_imm(cd, wt, xn, imm); }
        void strb(u1 wt, u1 xn, s2 imm) { emit_strb_imm(cd, wt, xn, imm); }

        void iconst(u1 xt, s4 value) {
            // For small negative immediates, use MOVN
            if (value < 0 && -value-1 < 0xffff) {
                emit_movn_imm32(cd, xt, -value - 1);
                return;
            }

            emit_mov_imm32(cd, xt, value & 0xffff);

            u4 v = (u4) value;
            if (v > 0xffff) {
                u2 imm = (value >> 16) & 0xffff;
                emit_movk_imm32(cd, xt, imm, 1);
            }
        }

        void lconst(u1 xt, s8 value) {
            // For small negative immediates, use MOVN
            if (value < 0 && -value-1 < 0xffff) {
                emit_movn_imm(cd, xt, -value - 1);
                return;
            }

            emit_mov_imm(cd, xt, value & 0xffff);

            u8 v = (u8) value;
            if (v > 0xffff) {
                u2 imm = (value >> 16) & 0xffff;
                emit_movk_imm(cd, xt, imm, 1);
            }

            if (v > 0xffffffff) {
                u2 imm = (value >> 32) & 0xffff;
                emit_movk_imm(cd, xt, imm, 2);
            }

            if (v > 0xffffffffffff) {
                u2 imm = (value >> 48) & 0xffff;
                emit_movk_imm(cd, xt, imm, 3);
            }
        }

        void lda(u1 xd, u1 xn, s4 imm) { emit_lda(xd, xn, imm); }

        void adr(u1 xd, s4 imm) { emit_adr(cd, xd, imm); }

        /* Branch *************************************************************/
        void b(s4 imm) { emit_br_imm(cd, imm); }
        void b_eq(s4 imm) { emit_br_eq(cd, imm); }
        void b_ne(s4 imm) { emit_br_ne(cd, imm); }
        void b_cs(s4 imm) { emit_br_cs(cd, imm); }
        void b_cc(s4 imm) { emit_br_cc(cd, imm); }
        void b_mi(s4 imm) { emit_br_mi(cd, imm); }
        void b_pl(s4 imm) { emit_br_pl(cd, imm); }
        void b_vs(s4 imm) { emit_br_vs(cd, imm); }
        void b_vc(s4 imm) { emit_br_vc(cd, imm); }
        void b_hi(s4 imm) { emit_br_hi(cd, imm); }
        void b_ls(s4 imm) { emit_br_ls(cd, imm); }
        void b_ge(s4 imm) { emit_br_ge(cd, imm); }
        void b_lt(s4 imm) { emit_br_lt(cd, imm); }
        void b_gt(s4 imm) { emit_br_gt(cd, imm); }
        void b_le(s4 imm) { emit_br_le(cd, imm); }

        void blr(u1 xn) { emit_blr_reg(cd, xn); }
        void br(u1 xn) { emit_br_reg(cd, xn); }
        void ret() { emit_ret(cd); }

        void cbnz(u1 xn, s4 imm) { emit_cbnz(cd, xn, imm); }

        /* Integer arithemtic *************************************************/
        void iadd_imm(u1 xd, u1 xn, u4 imm) { emit_add_imm32(cd, xd, xn, imm); }
        void ladd_imm(u1 xd, u1 xn, u4 imm) { emit_add_imm(cd, xd, xn, imm); }

        void iadd(u1 xd, u1 xn, u1 xm) { emit_add_reg32(cd, xd, xn, xm); }
        void ladd(u1 xd, u1 xn, u1 xm) { emit_add_reg(cd, xd, xn, xm); }

        /** Xd = Xn + shift(Xm, amount); */
        void ladd_shift(u1 xd, u1 xn, u1 xm, u1 shift, u1 amount) {
            emit_add_reg_shift(cd, xd, xn, xm, shift, amount);
        }

        void isub_imm(u1 xd, u1 xn, u4 imm) { emit_sub_imm32(cd, xd, xn, imm); }
        void lsub_imm(u1 xd, u1 xn, u4 imm) { emit_sub_imm(cd, xd, xn, imm); }

        void isub(u1 xd, u1 xn, u1 xm) { emit_sub_reg32(cd, xd, xn, xm); }
        void lsub(u1 xd, u1 xn, u1 xm) { emit_sub_reg(cd, xd, xn, xm); }

        /* wd = wn / wm */
        void idiv(u1 wd, u1 wn, u1 wm) { emit_sdiv32(cd, wd, wn, wm); }
        void ldiv(u1 xd, u1 xn, u1 xm) { emit_sdiv(cd, xd, xn, xm); }

        void imul(u1 wd, u1 wn, u1 wm) { emit_mul32(cd, wd, wn, wm); }
        void lmul(u1 xd, u1 xn, u1 xm) { emit_mul(cd, xd, xn, xm); }

        /* wd = wa - wn * wm */
        void imsub(u1 wd, u1 wn, u1 wm, u1 wa) { emit_msub32(cd, wd, wn, wm, wa); }
        void lmsub(u1 xd, u1 xn, u1 xm, u1 xa) { emit_msub(cd, xd, xn, xm, xa); }

        /* wd = lsl(wn, shift) */
        void ilsl_imm(u1 wd, u1 wn, u1 shift) { emit_lsl_imm32(cd, wd, wn, shift); }
        void llsl_imm(u1 xd, u1 xn, u1 shift) { emit_lsl_imm(cd, xd, xn, shift); }

        void ilsr_imm(u1 wd, u1 wn, u1 shift) { emit_lsr_imm32(cd, wd, wn, shift); }
        void llsr_imm(u1 xd, u1 xn, u1 shift) { emit_lsr_imm(cd, xd, xn, shift); }

        void iasr_imm(u1 wd, u1 wn, u1 shift) { emit_asr_imm32(cd, wd, wn, shift); }
        void lasr_imm(u1 xd, u1 xn, u1 shift) { emit_asr_imm(cd, xd, xn, shift); }

        void iasr(u1 wd, u1 wn, u1 wm) { emit_asr32(cd, wd, wn, wm); }
        void lasr(u1 xd, u1 xn, u1 xm) { emit_asr(cd, xd, xn, xm); }

        void ilsr(u1 wd, u1 wn, u1 wm) { emit_lsr32(cd, wd, wn, wm); }
        void llsr(u1 xd, u1 xn, u1 xm) { emit_lsr(cd, xd, xn, xm); }

        void ilsl(u1 wd, u1 wn, u1 wm) { emit_lsl32(cd, wd, wn, wm); }
        void llsl(u1 xd, u1 xn, u1 xm) { emit_lsl(cd, xd, xn, xm); }

        void uxtb(u1 wd, u1 wn) { emit_uxtb(cd, wd, wn); }
        void uxth(u1 wd, u1 wn) { emit_uxth(cd, wd, wn); }
        void sxtb(u1 wd, u1 wn) { emit_sxtb(cd, wd, wn); }
        void sxth(u1 wd, u1 wn) { emit_sxth(cd, wd, wn); }
        void sxtw(u1 xd, u1 wn) { emit_sxtw(cd, xd, wn); }
        void ubfx(u1 wd, u1 xn) { emit_ubfm32(cd, wd, xn, 0, 31); }

        void ltst(u1 xn, u1 xm) { emit_tst_sreg(cd, xn, xm); }
        void itst(u1 wn, u1 wm) { emit_tst_sreg32(cd, wn, wm); }

        // TODO: implement these correctly
        // void iand_imm(u1 wd, u1 wn, u2 imm) { emit_and_imm32(cd, wd, wn, imm); }
        // void land_imm(u1 xd, u1 xn, u2 imm) { emit_and_imm(cd, xd, xn, imm); }

        void iand(u1 wd, u1 wn, u1 wm) { emit_and_sreg32(cd, wd, wn, wm); }
        void land(u1 xd, u1 xn, u1 xm) { emit_and_sreg(cd, xd, xn, xm); }

        void ior(u1 wd, u1 wn, u1 wm) { emit_orr_sreg32(cd, wd, wn, wm); }
        void lor(u1 xd, u1 xn, u1 xm) { emit_orr_sreg(cd, xd, xn, xm); }

        void ixor(u1 wd, u1 wn, u1 wm) { emit_eor_sreg32(cd, wd, wn, wm); }
        void lxor(u1 xd, u1 xn, u1 xm) { emit_eor_sreg(cd, xd, xn, xm); }

        void clr(u1 xd) { lxor(xd, xd, xd); }

        /* xt = if cond then xn else xm */
        void csel(u1 xt, u1 xn, u1 xm, u1 cond) { emit_csel(cd, xt, xn, xm, cond); }
        void icsel(u1 wt, u1 wn, u1 wm, u1 cond) { emit_csel32(cd, wt, wn, wm, cond); }

        void cset(u1 xt, u1 cond) { emit_cset(cd, xt, cond); } /* Xd = if cond then 1 else 0 */
        void csetm(u1 xt, u1 cond) { emit_csetm(cd, xt, cond); } /* Xd = if cond then -1 else 0 */

        /* xt = if cond then xn else -xm */
        void icsneg(u1 wd, u1 wn, u1 wm, u1 cond) { emit_csneg32(cd, wd, wn, wm, cond); }

        /* Floating point *****************************************************/

        void fmov(u1 sd, u1 sn) { emit_fmovs(cd, sd, sn); }
        void dmov(u1 dd, u1 dn) { emit_fmovd(cd, dd, dn); }

        void fneg(u1 sd, u1 sn) { emit_fnegs(cd, sd, sn); }
        void dneg(u1 dd, u1 dn) { emit_fnegd(cd, dd, dn); }

        void fcmp(u1 sn, u1 sm) { emit_fp_cmp(cd, 0, sm, sn, 0); }
        void fcmp(u1 sn) { emit_fp_cmp(cd, 0, 0, sn, 1); } /* fcmp Sn, #0.0 */
        void dcmp(u1 xn, u1 xm) { emit_fp_cmp(cd, 1, xm, xn, 0); }

        void fmul(u1 st, u1 sn, u1 sm) { emit_fmuls(cd, st, sn, sm); }
        void dmul(u1 dt, u1 dn, u1 dm) { emit_fmuld(cd, dt, dn, dm); }

        void fdiv(u1 st, u1 sn, u1 sm) { emit_fdivs(cd, st, sn, sm); }
        void ddiv(u1 dt, u1 dn, u1 dm) { emit_fdivd(cd, dt, dn, dm); }

        void fadd(u1 st, u1 sn, u1 sm) { emit_fadds(cd, st, sn, sm); }
        void dadd(u1 dt, u1 dn, u1 dm) { emit_faddd(cd, dt, dn, dm); }

        void fsub(u1 st, u1 sn, u1 sm) { emit_fsubs(cd, st, sn, sm); }
        void dsub(u1 dt, u1 dn, u1 dm) { emit_fsubd(cd, dt, dn, dm); }

        void i2f(u1 st, u1 wn) { emit_scvtf(cd, 0, 0, wn, st); }
        void l2f(u1 st, u1 xn) { emit_scvtf(cd, 1, 0, xn, st); }
        void i2d(u1 dt, u1 wn) { emit_scvtf(cd, 0, 1, wn, dt); }
        void l2d(u1 dt, u1 xn) { emit_scvtf(cd, 1, 1, xn, dt); }

        void f2i(u1 wd, u1 sn) { emit_fcvtzs(cd, 0, 0, sn, wd); }
        void f2l(u1 xd, u1 sn) { emit_fcvtzs(cd, 1, 0, sn, xd); }
        void d2i(u1 wd, u1 dn) { emit_fcvtzs(cd, 0, 1, dn, wd); }
        void d2l(u1 xd, u1 dn) { emit_fcvtzs(cd, 1, 1, dn, xd); }

        void f2d(u1 dd, u1 sn) { emit_fcvt(cd, 0, 1, sn, dd); }
        void d2f(u1 sd, u1 dn) { emit_fcvt(cd, 1, 0, dn, sd); }

        void dmb() { emit_dmb(cd, 0xf); }
        void dsb() { emit_dsb(cd, 0xf); }
        void dmb(u1 option) { emit_dmb(cd, option); }
        void dsb(u1 option) { emit_dsb(cd, option); }

	private:
		codegendata *cd;

        void emit_lda(u1 xd, u1 xn, s4 imm) {
            if (imm >= 0)
                emit_add_imm(cd, xd, xn, imm);
            else
                emit_sub_imm(cd, xd, xn, -imm);
        }
};


/* compatiblity macros ********************************************************/

#define M_NOP                       emit_nop(cd)

#define M_MOV(a,b)                  emit_mov(cd, b, a)
#define M_FMOV(b,c)                 emit_fmovs(cd, c, b)
#define M_DMOV(b,c)                 emit_fmovd(cd, c, b)

#define M_LLD(a, b, disp)           emit_ldr_imm(cd, a, b, disp)
#define M_ALD(a, b, disp)           emit_ldr_imm(cd, a, b, disp)
#define M_ALD_DSEG(a, disp)         M_LLD(a, REG_PV, disp)
#define M_ILD(a, b, disp)           emit_ldr_imm32(cd, a, b, disp)

#define M_LST(a, b, disp)           emit_str_imm(cd, a, b, disp)
#define M_AST(a, b, disp)           emit_str_imm(cd, a, b, disp)
#define M_IST(a, b, disp)           emit_str_imm32(cd, a, b, disp)

#define M_DLD(a, b, disp)           emit_fp_ldr_imm(cd, a, b, disp)
#define M_FLD(a, b, disp)           emit_fp_ldr_imm32(cd, a, b, disp)

#define M_DST(a, b, disp)           emit_fp_str_imm(cd, a, b, disp)
#define M_FST(a, b, disp)           emit_fp_str_imm32(cd, a, b, disp)

#define M_TEST(a)                   emit_tst_sreg(cd, a, a) 
#define M_ACMP(a, b)                emit_cmp_reg(cd, a, b) 
#define M_ICMP(a, b)                emit_cmp_reg32(cd, a, b) 

#endif // CODEGEN_HPP_


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
