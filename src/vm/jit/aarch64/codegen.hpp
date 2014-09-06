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

#define gen_bound_check \
    os::abort("gen_bound_check not yet implemented!"); \
    if (checkbounds) { \
        M_ILD(REG_ITMP3, s1, OFFSET(java_arrayheader, size));\
        M_CMPULT(s2, REG_ITMP3, REG_ITMP3);\
        M_BEQZ(REG_ITMP3, 0);\
        codegen_add_arrayindexoutofboundsexception_ref(cd, s2); \
    }


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


/* macros to create code ******************************************************/

/* AARCH64 ===================================================================*/

class AsmEmitter {
	
	public:
		explicit AsmEmitter(codegendata *cd) : cd(cd) {}

        void imov(u1 wd, u1 wn) { emit_mov_reg32(cd, wd, wn); }

        void mov(u1 xt, u1 xn) { emit_mov(cd, xt, xn); }
        void mov_imm(u1 xt, u2 imm) {  emit_mov_imm(cd, xt, imm); }
        void movn_imm(u1 xt, u2 imm) { emit_movn_imm(cd, xt, imm); }

        void icmp_imm(u1 wd, u2 imm) { emit_cmp_imm32(cd, wd, imm); }
        void lcmp_imm(u1 xd, u2 imm) { emit_cmp_imm(cd, xd, imm); }
        void icmn_imm(u1 wd, u2 imm) { emit_cmn_imm32(cd, wd, imm); }

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

        void ist(u1 xt, u1 xn, s2 imm) { emit_str_imm32(cd, xt, xn, imm); }
        void lst(u1 xt, u1 xn, s2 imm) { emit_str_imm(cd, xt, xn, imm); }
        void ast(u1 xt, u1 xn, s2 imm) { emit_str_imm(cd, xt, xn, imm); }

        void fst(u1 xt, u1 xn, s2 imm) { emit_fp_str_imm32(cd, xt, xn, imm); }
        void dst(u1 xt, u1 xn, s2 imm) { emit_fp_str_imm(cd, xt, xn, imm); }

        void strh(u1 wt, u1 xn, s2 imm) { emit_strh_imm(cd, wt, xn, imm); }
        void strb(u1 wt, u1 xn, s2 imm) { emit_strb_imm(cd, wt, xn, imm); }

        // TODO: Implement these 2 using mov instructions
        void iconst(u1 xt, s4 value) {
            // if ((value >= -32768) && (value <= 32767))
            //    mov_imm(xt, value);
            // else {
                s4 disp = dseg_add_s4(cd, value);
                ild(xt, REG_PV, disp);
            // }
        }

        void lconst(u1 xt, s8 value) {
            // if ((value >= -32768) && (value <= 32767))
            //    mov_imm(xt, value);
            // else {
                s4 disp = dseg_add_s8(cd, value);
                lld(xt, REG_PV, disp);
            // }
        }

        void lda(u1 xd, u1 xn, s4 imm) { emit_lda(xd, xn, imm); }

        void adr(u1 xd, s4 imm) { emit_adr(cd, xd, imm); }

        /* Branch *************************************************************/
        void blr(u1 xn) { emit_blr_reg(cd, xn); }
        void br(u1 xn) { emit_br_reg(cd, xn); }

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

        void iasr(u1 wd, u1 wn, u1 wm) { emit_asr32(cd, wd, wn, wm); }
        void lasr(u1 xd, u1 xn, u1 xm) { emit_asr(cd, xd, xn, xm); }

        void uxth(u1 wd, u1 wn) { emit_uxth(cd, wd, wn); }
        void sxtb(u1 wd, u1 wn) { emit_sxtb(cd, wd, wn); }

        void test(u1 xn, u1 xm) { emit_tst_sreg(cd, xn, xm); }
        void test(u1 xn) { emit_tst_sreg(cd, xn, xn); }

        // TODO: implement these correctly
        // void iand_imm(u1 wd, u1 wn, u2 imm) { emit_and_imm32(cd, wd, wn, imm); }
        // void land_imm(u1 xd, u1 xn, u2 imm) { emit_and_imm(cd, xd, xn, imm); }

        void iand(u1 wd, u1 wn, u1 wm) { emit_and_sreg32(cd, wd, wn, wm); }
        void land(u1 xd, u1 xn, u1 xm) { emit_and_sreg(cd, xd, xn, xm); }

        void ixor(u1 wd, u1 wn, u1 wm) { emit_eor_sreg32(cd, wd, wn, wm); }
        void lxor(u1 xd, u1 xn, u1 xm) { emit_eor_sreg(cd, xd, xn, xm); }

        /* xt = if cond then xn else xm */
        void csel(u1 xt, u1 xn, u1 xm, u1 cond) { emit_csel(cd, xt, xn, xm, cond); }         
        void cset(u1 xt, u1 cond) { emit_cset(cd, xt, cond); } /* Xd = if cond then 1 else 0 */
        void csetm(u1 xt, u1 cond) { emit_csetm(cd, xt, cond); } /* Xd = if cond then -1 else 0 */

        /* Floating point *****************************************************/

        void fmov(u1 sd, u1 sn) { emit_fp_mov(cd, 0, sn, sd); }
        void dmov(u1 dd, u1 dn) { emit_fp_mov(cd, 1, dn, dd); }

        void fcmp(u1 sn, u1 sm) { emit_fp_cmp(cd, 0, sm, sn, 0); }
        void fcmp(u1 sn) { emit_fp_cmp(cd, 0, 0, sn, 1); } /* fcmp Sn, #0.0 */
        void dcmp(u1 xn, u1 xm) { emit_fp_cmp(cd, 1, xm, xn, 0); }

        void fmul(u1 st, u1 sn, u1 sm) { emit_fmul(cd, 0, st, sn, sm); }
        void dmul(u1 dt, u1 dn, u1 dm) { emit_fmul(cd, 1, dt, dn, dm); }

        void i2f(u1 st, u1 wn) { emit_scvtf(cd, 0, 0, wn, st); }
        void l2f(u1 st, u1 xn) { emit_scvtf(cd, 1, 0, xn, st); }
        void i2d(u1 dt, u1 wn) { emit_scvtf(cd, 0, 1, wn, dt); }
        void l2d(u1 dt, u1 xn) { emit_scvtf(cd, 1, 1, xn, dt); }

        void f2i(u1 wd, u1 sn) { emit_fcvtzs(cd, 0, 0, sn, wd); }
        void f2l(u1 xd, u1 sn) { emit_fcvtzs(cd, 1, 0, sn, xd); }
        void d2i(u1 wd, u1 dn) { emit_fcvtzs(cd, 0, 1, dn, wd); }
        void d2l(u1 xd, u1 dn) { emit_fcvtzs(cd, 1, 1, dn, xd); }

        void dmb() { emit_dmb(cd, 0xf); }
        void dsb() { emit_dsb(cd, 0xf); }

	private:
		codegendata *cd;

        void emit_lda(u1 xd, u1 xn, s4 imm) {
            if (imm >= 0)
                emit_add_imm(cd, xd, xn, imm);
            else
                emit_sub_imm(cd, xd, xn, -imm);
        }
};

#define M_LLD_INTERN(a,b,disp)      emit_ldr_imm(cd, a, b, disp)
#define M_LST_INTERN(a,b,disp)      emit_str_imm(cd, a, b, disp)

#define M_ILD_INTERN(a,b,disp)      emit_ldr_imm(cd, a, b, disp) // TODO: should i really only use 64 bits?
#define M_IST_INTERN(a,b,disp)      emit_str_imm(cd, a, b, disp)

#define M_FLD_INTERN(a,b,disp)      emit_fp_ldr_imm(cd, a, b, disp) // TODO: test wit 32 bit version
#define M_FST_INTERN(a,b,disp)      emit_fp_str_imm(cd, a, b, disp) // TODO: test wit 32 bit version

#define M_DLD_INTERN(a,b,disp)      emit_fp_ldr_imm(cd, a, b, disp) 
#define M_DST_INTERN(a,b,disp)      emit_fp_str_imm(cd, a, b, disp)

#define M_LDA_INTERN(a,b,disp)      emit_lda(cd, a, b, disp)

#define M_LDRH(Wt, Xn, imm)         emit_ldrh_imm(cd, Wt, Xn, imm)

#define M_RET(a,b)                  emit_ret(cd)
#define M_JSR(a,b)                  emit_blr_reg(cd, b)
#define M_JMP(a,b)                  emit_br_reg(cd, b)

#define M_BR(imm)                   emit_br_imm(cd, imm + 1) // TODO: again we are off by one

#define M_LADD_IMM(Xn,imm,Xd)       emit_add_imm(cd, Xd, Xn, imm)
#define M_LSUB_IMM(Xn,imm,Xd)       emit_sub_imm(cd, Xd, Xn, imm) /* Xd = Xn - imm */

#define M_IADD_IMM(Xn,imm,Xd)       M_LADD_IMM(Xn,imm,Xd)
#define M_ISUB_IMM(Xn,imm,Xd)       M_LSUB_IMM(Xn,imm,Xd) // TODO: test with 32 bit version instead of 64

#define M_LADD(Xn,Xm,Xd)            emit_add_reg(cd, Xd, Xn, Xm)
#define M_LSUB(Xn,Xm,Xd)            emit_sub_reg(cd, Xd, Xn, Xm)  /* Xd = Xn - Xm */

#define M_IADD(Xn,Xm,Xd)            M_LADD(Xn, Xm, Xd) // TODO: test with 32 bit version instead of 64
#define M_ISUB(Xn,Xm,Xd)            M_LSUB(Xn, Xm, Xd)

#define M_LMUL(Xn,Xm,Xd)            emit_mul(cd, Xd, Xn, Xm)    /* Xd = Xn * Xm */
#define M_IMUL(Xn,Xm,Xd)            M_LMUL(Xn, Xm, Xd) // TODO: test with 32 bit version

#define M_S8ADDQ(Xm,Xn,Xd)          emit_add_reg_shift(cd, Xd, Xn, Xm, CODE_LSL, 3); /* Xd = Xn + shift(Xm, 3) */
#define M_LADD_SHIFT(Xd,Xn,Xm,s,a)  emit_add_reg_shift(cd, Xd, Xn, Xm, s, a); /* Xd = Xn + lsl(Xm, a) */

#define M_MOV(a,b)                  emit_mov(cd, b, a)
#define M_MOV_IMM(a, imm)           emit_mov_imm(cd, a, imm)

#define M_CLR(Xd)                    emit_clr(cd, Xd)

#define M_CSET(Xd,cond)             emit_cset(cd, Xd, cond)  /* Xd = if cond then 1 else 0 */
#define M_CSETM(Xd,cond)            emit_csetm(cd, Xd, cond) /* Xd = if cond then -1 else 0 */

#define M_BNEZ(a,disp)              emit_cbnz(cd, a, disp + 1) // TODO: off by 1 compared to alpha

#define M_CMP_IMM(a, imm)           emit_cmp_imm(cd, a, imm)
#define M_CMN_IMM(a, imm)           emit_cmn_imm(cd, a, imm)

#define M_BR_EQ(imm)                emit_br_eq(cd, imm)
#define M_BR_NE(imm)                emit_br_ne(cd, imm)
#define M_BR_LT(imm)                emit_br_lt(cd, imm)
#define M_BR_LE(imm)                emit_br_le(cd, imm)
#define M_BR_GT(imm)                emit_br_gt(cd, imm)
#define M_BR_GE(imm)                emit_br_ge(cd, imm)
#define M_BR_VS(imm)                emit_br_vs(cd, imm)
#define M_BR_VC(imm)                emit_br_vc(cd, imm)

#define M_TEST(a)                   emit_tst_sreg(cd, a, a) 
#define M_TST(a,b)                  emit_tst_sreg(cd, a, b)
#define M_ACMP(a,b)                 emit_cmp_reg(cd, a, b) 
#define M_ICMP(a,b)                 emit_cmp_reg32(cd, a, b) 

#define M_FMOV(b,c)                 emit_fp_mov(cd, 1, b, c) // TODO: again, try with 32 bit version
#define M_DMOV(b,c)                 emit_fp_mov(cd, 1, b, c)

#define M_FCMP(a,b)                 emit_fp_cmp(cd, 1, a, b, 0)

#define M_FMUL(Dn,Dm,Dd)            emit_fmul(cd, 1, Dd, Dn, Dm)  /* Dd = Dn * Dm */
#define M_FMULS(Dn,Dm,Dd)           M_FMUL(Dn,Dm,Dd)           /* TODO: implement single precision */

#define M_CVTLD(b,c)                emit_scvtf(cd, 1, 0, c, b)       /* c = b */
#define M_CVTLF(b,c)                M_CVTLD(b, c) 
#define M_CVTDL(b,c)                emit_fcvtzs(cd, 1, 1, c, b)      /* c = b */


/* ===========================================================================*/

/* M_MEM - memory instruction format *******************************************

    Opcode ........ opcode
    Ra ............ source/target register for memory access
    Rb ............ base register
    Memory_disp ... memory displacement (16 bit signed) to be added to Rb

*******************************************************************************/

/*#define M_MEM(Opcode,Ra,Rb,Memory_disp) \
    do { \
        *((uint32_t *) cd->mcodeptr) = ((((Opcode)) << 26) | ((Ra) << 21) | ((Rb) << 16) | ((Memory_disp) & 0xffff)); \
        cd->mcodeptr += 4; \
    } while (0)*/
#define M_MEM(a,b,c,d) os::abort("M_MEM called with (%x, %x, %x, %x)", a, b, c, d) 

#define M_MEM_GET_Opcode(x)             (          (((x) >> 26) & 0x3f  ))
#define M_MEM_GET_Ra(x)                 (          (((x) >> 21) & 0x1f  ))
#define M_MEM_GET_Rb(x)                 (          (((x) >> 16) & 0x1f  ))
#define M_MEM_GET_Memory_disp(x)        ((int16_t) ( (x)        & 0xffff))


/* M_BRA - branch instruction format *******************************************

    Opcode ........ opcode
    Ra ............ register to be tested
    Branch_disp ... relative address to be jumped to (divided by 4)

*******************************************************************************/

/*#define M_BRA(Opcode,Ra,Branch_disp) \
    do { \
        *((uint32_t *) cd->mcodeptr) = ((((Opcode)) << 26) | ((Ra) << 21) | ((Branch_disp) & 0x1fffff)); \
        cd->mcodeptr += 4; \
    } while (0)*/
#define M_BRA(a,b,c) os::abort("M_BRA called with (%x, %x, %x)", a, b, c) 


#define REG   0
#define CONST 1

/* 3-address-operations: M_OP3
      op ..... opcode
      fu ..... function-number
      a  ..... register number source 1
      b  ..... register number or constant integer source 2
      c  ..... register number destination
      const .. switch to use b as constant integer 
                 (REG means: use b as register number)
                 (CONST means: use b as constant 8-bit-integer)
*/      

/*#define M_OP3(op,fu,a,b,c,const) \
    do { \
        *((u4 *) cd->mcodeptr) = ((((s4) (op)) << 26) | ((a) << 21) | ((b) << (16 - 3 * (const))) | ((const) << 12) | ((fu) << 5) | ((c))); \
        cd->mcodeptr += 4; \
    } while (0)*/
#define M_OP3(op,fu,a,b,c,const) os::abort("M_OP3 called with (%x, %x, %x, %x, %x, %x)", op, fu, a, b, c, const) 

#define M_OP3_GET_Opcode(x)             (          (((x) >> 26) & 0x3f  ))


/* 3-address-floating-point-operation: M_FOP3 
     op .... opcode
     fu .... function-number
     a,b ... source floating-point registers
     c ..... destination register
*/ 

/*#define M_FOP3(op,fu,a,b,c) \
    do { \
        *((u4 *) cd->mcodeptr) = ((((s4) (op)) << 26) | ((a) << 21) | ((b) << 16) | ((fu) << 5) | (c)); \
        cd->mcodeptr += 4; \
    } while (0)*/
#define M_FOP3(op,fu,a,b,c) os::abort("M_FOP3 called with (%x, %x, %x, %x, %x)", op, fu, a, b, c) 


/* macros for all used commands (see an Alpha-manual for description) *********/

//#define M_LDA_INTERN(a,b,disp)  M_MEM(0x08,a,b,disp)            /* low const  */

#define M_LDA(a,b,disp) \
    do { \
        s4 lo = (short) (disp); \
        s4 hi = (short) (((disp) - lo) >> 16); \
        if (hi == 0) { \
            M_LDA_INTERN(a,b,lo); \
        } else { \
            M_LDAH(a,b,hi); \
            M_LDA_INTERN(a,a,lo); \
        } \
    } while (0) 

// #define M_LDAH(a,b,disp)        M_MEM (0x09,a,b,disp)           /* high const */
#define M_LDAH(a,b,disp)        os::abort("LDA with hight not supported on aarch64!");

#define M_BLDU(a,b,disp)        M_MEM (0x0a,a,b,disp)           /*  8 load    */
#define M_SLDU(a,b,disp)        M_MEM (0x0c,a,b,disp)           /* 16 load    */

//#define M_ILD_INTERN(a,b,disp)  M_MEM(0x28,a,b,disp)            /* 32 load    */
//#define M_LLD_INTERN(a,b,disp)  M_MEM(0x29,a,b,disp)            /* 64 load    */

#define M_ILD(a,b,disp) \
    do { \
        s4 tmp = disp; \
        if (tmp < -256) { \
            u4 t2 = -tmp; \
            M_LSUB_IMM(b, t2, a); \
            M_ILD_INTERN(a, a, 0); \
        } else { \
            s4 lo = (short) (disp); \
            s4 hi = (short) (((disp) - lo) >> 16); \
            if (hi == 0) { \
                M_ILD_INTERN(a,b,lo); \
            } else { \
                M_LDAH(a,b,hi); \
                M_ILD_INTERN(a,a,lo); \
            } \
        } \
    } while (0)

#define M_LLD(a,b,disp) \
    do { \
        s4 tmp = disp; \
        if (tmp < -256) { \
            u4 t2 = -tmp; \
            M_LSUB_IMM(b, t2, a); \
            M_LLD_INTERN(a, a, 0); \
        } else { \
            s4 lo = (short) (disp); \
            s4 hi = (short) (((disp) - lo) >> 16); \
            if (hi == 0) { \
                M_LLD_INTERN(a,b,lo); \
            } else { \
                M_LDAH(a,b,hi); \
                M_LLD_INTERN(a,a,lo); \
            } \
        } \
    } while (0)

#define M_LLD_U(a,b,disp)           M_LLD(a,b,disp) /* TODO: only temp, see if it works on aarch64 */

#define M_ALD_INTERN(a,b,disp)  M_LLD_INTERN(a,b,disp)
#define M_ALD(a,b,disp)         M_LLD(a,b,disp)                 /* addr load  */
#define M_ALD_DSEG(a,disp)      M_LLD(a,REG_PV,disp)

#define M_BST(a,b,disp)         M_MEM(0x0e,a,b,disp)            /*  8 store   */
#define M_SST(a,b,disp)         M_MEM(0x0d,a,b,disp)            /* 16 store   */

// #define M_IST_INTERN(a,b,disp)  M_MEM(0x2c,a,b,disp)            /* 32 store   */
// #define M_LST_INTERN(a,b,disp)  M_MEM(0x2d,a,b,disp)            /* 64 store   */

/* Stores with displacement overflow should only happen with PUTFIELD or on   */
/* the stack. The PUTFIELD instruction does not use REG_ITMP3 and a           */
/* reg_of_var call should not use REG_ITMP3!!!                                */

#define M_IST(a,b,disp) \
    do { \
        s4 lo = (short) (disp); \
        s4 hi = (short) (((disp) - lo) >> 16); \
        if (hi == 0) { \
            M_IST_INTERN(a,b,lo); \
        } else { \
            M_LDAH(REG_ITMP3,b,hi); \
            M_IST_INTERN(a,REG_ITMP3,lo); \
        } \
    } while (0) 

#define M_LST(a,b,disp) \
    do { \
        s4 lo = (short) (disp); \
        s4 hi = (short) (((disp) - lo) >> 16); \
        if (hi == 0) { \
            M_LST_INTERN(a,b,lo); \
        } else { \
            M_LDAH(REG_ITMP3,b,hi); \
            M_LST_INTERN(a,REG_ITMP3,lo); \
        } \
    } while (0) 

#define M_AST(a,b,disp)         M_LST(a,b,disp)                 /* addr store */

#define M_BSEXT(b,c)            M_OP3 (0x1c,0x0,REG_ZERO,b,c,0) /*  8 signext */
#define M_SSEXT(b,c)            M_OP3 (0x1c,0x1,REG_ZERO,b,c,0) /* 16 signext */

// #define M_BR(disp)              M_BRA (0x31,REG_ZERO,disp)      /* branch     */
#define M_BSR(ra,disp)          M_BRA (0x34,ra,disp)            /* branch sbr */
#define M_BEQZ(a,disp)          M_BRA (0x39,a,disp)             /* br a == 0  */
#define M_BLTZ(a,disp)          M_BRA (0x3a,a,disp)             /* br a <  0  */
#define M_BLEZ(a,disp)          M_BRA (0x3b,a,disp)             /* br a <= 0  */
//#define M_BNEZ(a,disp)          M_BRA (0x3d,a,disp)             /* br a != 0  */
#define M_BGEZ(a,disp)          M_BRA (0x3e,a,disp)             /* br a >= 0  */
#define M_BGTZ(a,disp)          M_BRA (0x3f,a,disp)             /* br a >  0  */

// #define M_JMP(a,b)              M_MEM (0x1a,a,b,0x0000)         /* jump       */
// #define M_JSR(a,b)              M_MEM (0x1a,a,b,0x4000)         /* call sbr   */
// #define M_RET(a,b)              M_MEM (0x1a,a,b,0x8000)         /* return     */

// #define M_IADD(a,b,c)           M_OP3 (0x10,0x0,  a,b,c,0)      /* 32 add     */
// #define M_LADD(a,b,c)           M_OP3 (0x10,0x20, a,b,c,0)      /* 64 add     */
// #define M_ISUB(a,b,c)           M_OP3 (0x10,0x09, a,b,c,0)      /* 32 sub     */
// #define M_LSUB(a,b,c)           M_OP3 (0x10,0x29, a,b,c,0)      /* 64 sub     */
// #define M_IMUL(a,b,c)           M_OP3 (0x13,0x00, a,b,c,0)      /* 32 mul     */
// #define M_LMUL(a,b,c)           M_OP3 (0x13,0x20, a,b,c,0)      /* 64 mul     */

/* ============== aarch64 ================ */
#define M_NOP \
    do { \
        *((u4 *) cd->mcodeptr) = (0xd503201f); \
        cd->mcodeptr += 4; \
    } while (0) \
/* ============== aarch64 ================ */

// #define M_IADD_IMM(a,b,c)       M_OP3 (0x10,0x0,  a,b,c,1)      /* 32 add     */
// #define M_LADD_IMM(a,b,c)       M_OP3 (0x10,0x20, a,b,c,1)      /* 64 add     */
// #define M_ISUB_IMM(a,b,c)       M_OP3 (0x10,0x09, a,b,c,1)      /* 32 sub     */
// #define M_LSUB_IMM(a,b,c)       M_OP3 (0x10,0x29, a,b,c,1)      /* 64 sub     */
#define M_IMUL_IMM(a,b,c)       M_OP3 (0x13,0x00, a,b,c,1)      /* 32 mul     */
#define M_LMUL_IMM(a,b,c)       M_OP3 (0x13,0x20, a,b,c,1)      /* 64 mul     */

#define M_AADD_IMM(a,b,c)       M_LADD_IMM(a,b,c)
#define M_ASUB_IMM(a,b,c)       M_LSUB_IMM(a,b,c)

#define M_CMPEQ(a,b,c)          M_OP3 (0x10,0x2d, a,b,c,0)      /* c = a == b */
#define M_CMPLT(a,b,c)          M_OP3 (0x10,0x4d, a,b,c,0)      /* c = a <  b */
#define M_CMPLE(a,b,c)          M_OP3 (0x10,0x6d, a,b,c,0)      /* c = a <= b */

#define M_CMPULE(a,b,c)         M_OP3 (0x10,0x3d, a,b,c,0)      /* c = a <= b */
#define M_CMPULT(a,b,c)         M_OP3 (0x10,0x1d, a,b,c,0)      /* c = a <= b */

#define M_CMPEQ_IMM(a,b,c)      M_OP3 (0x10,0x2d, a,b,c,1)      /* c = a == b */
#define M_CMPLT_IMM(a,b,c)      M_OP3 (0x10,0x4d, a,b,c,1)      /* c = a <  b */
#define M_CMPLE_IMM(a,b,c)      M_OP3 (0x10,0x6d, a,b,c,1)      /* c = a <= b */

#define M_CMPULE_IMM(a,b,c)     M_OP3 (0x10,0x3d, a,b,c,1)      /* c = a <= b */
#define M_CMPULT_IMM(a,b,c)     M_OP3 (0x10,0x1d, a,b,c,1)      /* c = a <= b */

#define M_AND(a,b,c)            M_OP3 (0x11,0x00, a,b,c,0)      /* c = a &  b */
#define M_OR( a,b,c)            M_OP3 (0x11,0x20, a,b,c,0)      /* c = a |  b */
#define M_XOR(a,b,c)            M_OP3 (0x11,0x40, a,b,c,0)      /* c = a ^  b */

#define M_AND_IMM(a,b,c)        M_OP3 (0x11,0x00, a,b,c,1)      /* c = a &  b */
#define M_OR_IMM( a,b,c)        M_OP3 (0x11,0x20, a,b,c,1)      /* c = a |  b */
#define M_XOR_IMM(a,b,c)        M_OP3 (0x11,0x40, a,b,c,1)      /* c = a ^  b */

// #define M_MOV(a,c)              M_OR (a,a,c)                    /* c = a      */
// #define M_CLR(c)                M_OR (31,31,c)                  /* c = 0      */
// #define M_NOP                   M_OR (31,31,31)                 /* ;          */

#define M_SLL(a,b,c)            M_OP3 (0x12,0x39, a,b,c,0)      /* c = a << b */
#define M_SRA(a,b,c)            M_OP3 (0x12,0x3c, a,b,c,0)      /* c = a >> b */
#define M_SRL(a,b,c)            M_OP3 (0x12,0x34, a,b,c,0)      /* c = a >>>b */

#define M_SLL_IMM(a,b,c)        M_OP3 (0x12,0x39, a,b,c,1)      /* c = a << b */
#define M_SRA_IMM(a,b,c)        M_OP3 (0x12,0x3c, a,b,c,1)      /* c = a >> b */
#define M_SRL_IMM(a,b,c)        M_OP3 (0x12,0x34, a,b,c,1)      /* c = a >>>b */

// #define M_FLD_INTERN(a,b,disp)  M_MEM(0x22,a,b,disp)            /* load flt   */
// #define M_DLD_INTERN(a,b,disp)  M_MEM(0x23,a,b,disp)            /* load dbl   */

#define M_FLD(a,b,disp) \
    do { \
        s4 lo = (short) (disp); \
        s4 hi = (short) (((disp) - lo) >> 16); \
        if (hi == 0) { \
            M_FLD_INTERN(a,b,lo); \
        } else { \
            M_LDAH(REG_ITMP3,b,hi); \
            M_FLD_INTERN(a,REG_ITMP3,lo); \
        } \
    } while (0)

#define M_DLD(a,b,disp) \
    do { \
        s4 lo = (short) (disp); \
        s4 hi = (short) (((disp) - lo) >> 16); \
        if (hi == 0) { \
            M_DLD_INTERN(a,b,lo); \
        } else { \
            M_LDAH(REG_ITMP3,b,hi); \
            M_DLD_INTERN(a,REG_ITMP3,lo); \
        } \
    } while (0)

// #define M_FST_INTERN(a,b,disp)  M_MEM(0x26,a,b,disp)            /* store flt  */
// #define M_DST_INTERN(a,b,disp)  M_MEM(0x27,a,b,disp)            /* store dbl  */

/* Stores with displacement overflow should only happen with PUTFIELD or on   */
/* the stack. The PUTFIELD instruction does not use REG_ITMP3 and a           */
/* reg_of_var call should not use REG_ITMP3!!!                                */

#define M_FST(a,b,disp) \
    do { \
        s4 lo = (short) (disp); \
        s4 hi = (short) (((disp) - lo) >> 16); \
        if (hi == 0) { \
            M_FST_INTERN(a,b,lo); \
        } else { \
            M_LDAH(REG_ITMP3,b,hi); \
            M_FST_INTERN(a,REG_ITMP3,lo); \
        } \
    } while (0)

#define M_DST(a,b,disp) \
    do { \
        s4 lo = (short) (disp); \
        s4 hi = (short) (((disp) - lo) >> 16); \
        if (hi == 0) { \
            M_DST_INTERN(a,b,lo); \
        } else { \
            M_LDAH(REG_ITMP3,b,hi); \
            M_DST_INTERN(a,REG_ITMP3,lo); \
        } \
    } while (0)


#define M_FADD(a,b,c)           M_FOP3 (0x16, 0x080, a,b,c)     /* flt add    */
#define M_DADD(a,b,c)           M_FOP3 (0x16, 0x0a0, a,b,c)     /* dbl add    */
#define M_FSUB(a,b,c)           M_FOP3 (0x16, 0x081, a,b,c)     /* flt sub    */
#define M_DSUB(a,b,c)           M_FOP3 (0x16, 0x0a1, a,b,c)     /* dbl sub    */
// #define M_FMUL(a,b,c)           M_FOP3 (0x16, 0x082, a,b,c)     /* flt mul    */
#define M_DMUL(a,b,c)           M_FOP3 (0x16, 0x0a2, a,b,c)     /* dbl mul    */
#define M_FDIV(a,b,c)           M_FOP3 (0x16, 0x083, a,b,c)     /* flt div    */
#define M_DDIV(a,b,c)           M_FOP3 (0x16, 0x0a3, a,b,c)     /* dbl div    */

#define M_FADDS(a,b,c)          M_FOP3 (0x16, 0x580, a,b,c)     /* flt add    */
#define M_DADDS(a,b,c)          M_FOP3 (0x16, 0x5a0, a,b,c)     /* dbl add    */
#define M_FSUBS(a,b,c)          M_FOP3 (0x16, 0x581, a,b,c)     /* flt sub    */
#define M_DSUBS(a,b,c)          M_FOP3 (0x16, 0x5a1, a,b,c)     /* dbl sub    */
// #define M_FMULS(a,b,c)          M_FOP3 (0x16, 0x582, a,b,c)     /* flt mul    */
#define M_DMULS(a,b,c)          M_FOP3 (0x16, 0x5a2, a,b,c)     /* dbl mul    */
#define M_FDIVS(a,b,c)          M_FOP3 (0x16, 0x583, a,b,c)     /* flt div    */
#define M_DDIVS(a,b,c)          M_FOP3 (0x16, 0x5a3, a,b,c)     /* dbl div    */

#define M_CVTDF(b,c)            M_FOP3 (0x16, 0x0ac, 31,b,c)    /* dbl2flt    */
// #define M_CVTLF(b,c)            M_FOP3 (0x16, 0x0bc, 31,b,c)    /* long2flt   */
// #define M_CVTLD(b,c)            M_FOP3 (0x16, 0x0be, 31,b,c)    /* long2dbl   */
// #define M_CVTDL(b,c)            M_FOP3 (0x16, 0x1af, 31,b,c)    /* dbl2long   */
#define M_CVTDL_C(b,c)          M_FOP3 (0x16, 0x12f, 31,b,c)    /* dbl2long   */
#define M_CVTLI(b,c)            M_FOP3 (0x17, 0x130, 31,b,c)    /* long2int   */

#define M_CVTDFS(b,c)           M_FOP3 (0x16, 0x5ac, 31,b,c)    /* dbl2flt    */
#define M_CVTFDS(b,c)           M_FOP3 (0x16, 0x6ac, 31,b,c)    /* flt2dbl    */
#define M_CVTDLS(b,c)           M_FOP3 (0x16, 0x5af, 31,b,c)    /* dbl2long   */
#define M_CVTDL_CS(b,c)         M_FOP3 (0x16, 0x52f, 31,b,c)    /* dbl2long   */
#define M_CVTLIS(b,c)           M_FOP3 (0x17, 0x530, 31,b,c)    /* long2int   */

#define M_FCMPEQ(a,b,c)         M_FOP3 (0x16, 0x0a5, a,b,c)     /* c = a==b   */
#define M_FCMPLT(a,b,c)         M_FOP3 (0x16, 0x0a6, a,b,c)     /* c = a<b    */

// #define M_FCMPEQS(a,b,c)        M_FOP3 (0x16, 0x5a5, a,b,c)     /* c = a==b   */
#define M_FCMPLTS(a,b,c)        M_FOP3 (0x16, 0x5a6, a,b,c)     /* c = a<b    */

// #define M_FMOV(fa,fb)           M_FOP3 (0x17, 0x020, fa,fa,fb)  /* b = a      */
// #define M_DMOV(fa,fb)           M_FMOV (fa,fb)
#define M_FMOVN(fa,fb)          M_FOP3 (0x17, 0x021, fa,fa,fb)  /* b = -a     */

#define M_FNOP                  M_FMOV (31,31)

#define M_FBEQZ(fa,disp)        M_BRA (0x31,fa,disp)            /* br a == 0.0*/

/* macros for special commands (see an Alpha-manual for description) **********/

// #define M_TRAPB                 M_MEM (0x18,0,0,0x0000)        /* trap barrier*/
#define M_TRAPB                 M_NOP /* TODO: check where and how M_TRAPB is used */

#define M_S4ADDL(a,b,c)         M_OP3 (0x10,0x02, a,b,c,0)     /* c = a*4 + b */
#define M_S4ADDQ(a,b,c)         M_OP3 (0x10,0x22, a,b,c,0)     /* c = a*4 + b */
#define M_S4SUBL(a,b,c)         M_OP3 (0x10,0x0b, a,b,c,0)     /* c = a*4 - b */
#define M_S4SUBQ(a,b,c)         M_OP3 (0x10,0x2b, a,b,c,0)     /* c = a*4 - b */
#define M_S8ADDL(a,b,c)         M_OP3 (0x10,0x12, a,b,c,0)     /* c = a*8 + b */
// #define M_S8ADDQ(a,b,c)         M_OP3 (0x10,0x32, a,b,c,0)     /* c = a*8 + b */
#define M_S8SUBL(a,b,c)         M_OP3 (0x10,0x1b, a,b,c,0)     /* c = a*8 - b */
#define M_S8SUBQ(a,b,c)         M_OP3 (0x10,0x3b, a,b,c,0)     /* c = a*8 - b */
#define M_SAADDQ(a,b,c)         M_S8ADDQ(a,b,c)                /* c = a*8 + b */

#define M_S4ADDL_IMM(a,b,c)     M_OP3 (0x10,0x02, a,b,c,1)     /* c = a*4 + b */
#define M_S4ADDQ_IMM(a,b,c)     M_OP3 (0x10,0x22, a,b,c,1)     /* c = a*4 + b */
#define M_S4SUBL_IMM(a,b,c)     M_OP3 (0x10,0x0b, a,b,c,1)     /* c = a*4 - b */
#define M_S4SUBQ_IMM(a,b,c)     M_OP3 (0x10,0x2b, a,b,c,1)     /* c = a*4 - b */
#define M_S8ADDL_IMM(a,b,c)     M_OP3 (0x10,0x12, a,b,c,1)     /* c = a*8 + b */
#define M_S8ADDQ_IMM(a,b,c)     M_OP3 (0x10,0x32, a,b,c,1)     /* c = a*8 + b */
#define M_S8SUBL_IMM(a,b,c)     M_OP3 (0x10,0x1b, a,b,c,1)     /* c = a*8 - b */
#define M_S8SUBQ_IMM(a,b,c)     M_OP3 (0x10,0x3b, a,b,c,1)     /* c = a*8 - b */

// #define M_LLD_U(a,b,disp)       M_MEM (0x0b,a,b,disp)          /* unalign ld  */
#define M_LST_U(a,b,disp)       M_MEM (0x0f,a,b,disp)          /* unalign st  */

#define M_ZAP(a,b,c)            M_OP3 (0x12,0x30, a,b,c,0)
#define M_ZAPNOT(a,b,c)         M_OP3 (0x12,0x31, a,b,c,0)

#define M_ZAP_IMM(a,b,c)        M_OP3 (0x12,0x30, a,b,c,1)
#define M_ZAPNOT_IMM(a,b,c)     M_OP3 (0x12,0x31, a,b,c,1)

#define M_BZEXT(a,b)            M_ZAPNOT_IMM(a, 0x01, b)       /*  8 zeroext  */
#define M_CZEXT(a,b)            M_ZAPNOT_IMM(a, 0x03, b)       /* 16 zeroext  */
#define M_IZEXT(a,b)            M_ZAPNOT_IMM(a, 0x0f, b)       /* 32 zeroext  */

#define M_EXTBL(a,b,c)          M_OP3 (0x12,0x06, a,b,c,0)
#define M_EXTWL(a,b,c)          M_OP3 (0x12,0x16, a,b,c,0)
#define M_EXTLL(a,b,c)          M_OP3 (0x12,0x26, a,b,c,0)
#define M_EXTQL(a,b,c)          M_OP3 (0x12,0x36, a,b,c,0)
#define M_EXTWH(a,b,c)          M_OP3 (0x12,0x5a, a,b,c,0)
#define M_EXTLH(a,b,c)          M_OP3 (0x12,0x6a, a,b,c,0)
#define M_EXTQH(a,b,c)          M_OP3 (0x12,0x7a, a,b,c,0)
#define M_INSBL(a,b,c)          M_OP3 (0x12,0x0b, a,b,c,0)
#define M_INSWL(a,b,c)          M_OP3 (0x12,0x1b, a,b,c,0)
#define M_INSLL(a,b,c)          M_OP3 (0x12,0x2b, a,b,c,0)
#define M_INSQL(a,b,c)          M_OP3 (0x12,0x3b, a,b,c,0)
#define M_INSWH(a,b,c)          M_OP3 (0x12,0x57, a,b,c,0)
#define M_INSLH(a,b,c)          M_OP3 (0x12,0x67, a,b,c,0)
#define M_INSQH(a,b,c)          M_OP3 (0x12,0x77, a,b,c,0)
#define M_MSKBL(a,b,c)          M_OP3 (0x12,0x02, a,b,c,0)
#define M_MSKWL(a,b,c)          M_OP3 (0x12,0x12, a,b,c,0)
#define M_MSKLL(a,b,c)          M_OP3 (0x12,0x22, a,b,c,0)
#define M_MSKQL(a,b,c)          M_OP3 (0x12,0x32, a,b,c,0)
#define M_MSKWH(a,b,c)          M_OP3 (0x12,0x52, a,b,c,0)
#define M_MSKLH(a,b,c)          M_OP3 (0x12,0x62, a,b,c,0)
#define M_MSKQH(a,b,c)          M_OP3 (0x12,0x72, a,b,c,0)

#define M_EXTBL_IMM(a,b,c)      M_OP3 (0x12,0x06, a,b,c,1)
#define M_EXTWL_IMM(a,b,c)      M_OP3 (0x12,0x16, a,b,c,1)
#define M_EXTLL_IMM(a,b,c)      M_OP3 (0x12,0x26, a,b,c,1)
#define M_EXTQL_IMM(a,b,c)      M_OP3 (0x12,0x36, a,b,c,1)
#define M_EXTWH_IMM(a,b,c)      M_OP3 (0x12,0x5a, a,b,c,1)
#define M_EXTLH_IMM(a,b,c)      M_OP3 (0x12,0x6a, a,b,c,1)
#define M_EXTQH_IMM(a,b,c)      M_OP3 (0x12,0x7a, a,b,c,1)
#define M_INSBL_IMM(a,b,c)      M_OP3 (0x12,0x0b, a,b,c,1)
#define M_INSWL_IMM(a,b,c)      M_OP3 (0x12,0x1b, a,b,c,1)
#define M_INSLL_IMM(a,b,c)      M_OP3 (0x12,0x2b, a,b,c,1)
#define M_INSQL_IMM(a,b,c)      M_OP3 (0x12,0x3b, a,b,c,1)
#define M_INSWH_IMM(a,b,c)      M_OP3 (0x12,0x57, a,b,c,1)
#define M_INSLH_IMM(a,b,c)      M_OP3 (0x12,0x67, a,b,c,1)
#define M_INSQH_IMM(a,b,c)      M_OP3 (0x12,0x77, a,b,c,1)
#define M_MSKBL_IMM(a,b,c)      M_OP3 (0x12,0x02, a,b,c,1)
#define M_MSKWL_IMM(a,b,c)      M_OP3 (0x12,0x12, a,b,c,1)
#define M_MSKLL_IMM(a,b,c)      M_OP3 (0x12,0x22, a,b,c,1)
#define M_MSKQL_IMM(a,b,c)      M_OP3 (0x12,0x32, a,b,c,1)
#define M_MSKWH_IMM(a,b,c)      M_OP3 (0x12,0x52, a,b,c,1)
#define M_MSKLH_IMM(a,b,c)      M_OP3 (0x12,0x62, a,b,c,1)
#define M_MSKQH_IMM(a,b,c)      M_OP3 (0x12,0x72, a,b,c,1)

#define M_UMULH(a,b,c)          M_OP3 (0x13,0x30, a,b,c,0)     /* 64 umulh    */

#define M_UMULH_IMM(a,b,c)      M_OP3 (0x13,0x30, a,b,c,1)     /* 64 umulh    */

#define M_CMOVEQ(a,b,c)         M_OP3 (0x11,0x24, a,b,c,0)     /* a==0 ? c=b  */
#define M_CMOVNE(a,b,c)         M_OP3 (0x11,0x26, a,b,c,0)     /* a!=0 ? c=b  */
#define M_CMOVLT(a,b,c)         M_OP3 (0x11,0x44, a,b,c,0)     /* a< 0 ? c=b  */
#define M_CMOVGE(a,b,c)         M_OP3 (0x11,0x46, a,b,c,0)     /* a>=0 ? c=b  */
#define M_CMOVLE(a,b,c)         M_OP3 (0x11,0x64, a,b,c,0)     /* a<=0 ? c=b  */
#define M_CMOVGT(a,b,c)         M_OP3 (0x11,0x66, a,b,c,0)     /* a> 0 ? c=b  */

#define M_CMOVEQ_IMM(a,b,c)     M_OP3 (0x11,0x24, a,b,c,1)     /* a==0 ? c=b  */
#define M_CMOVNE_IMM(a,b,c)     M_OP3 (0x11,0x26, a,b,c,1)     /* a!=0 ? c=b  */
#define M_CMOVLT_IMM(a,b,c)     M_OP3 (0x11,0x44, a,b,c,1)     /* a< 0 ? c=b  */
#define M_CMOVGE_IMM(a,b,c)     M_OP3 (0x11,0x46, a,b,c,1)     /* a>=0 ? c=b  */
#define M_CMOVLE_IMM(a,b,c)     M_OP3 (0x11,0x64, a,b,c,1)     /* a<=0 ? c=b  */
#define M_CMOVGT_IMM(a,b,c)     M_OP3 (0x11,0x66, a,b,c,1)     /* a> 0 ? c=b  */

// 0x04 seems to be the first undefined instruction which does not
// call PALcode.
//#define M_UNDEFINED             M_OP3(0x04, 0, 0, 0, 0, 0)

/* macros for unused commands (see an Alpha-manual for description) ***********/

#define M_ANDNOT(a,b,c,const)   M_OP3 (0x11,0x08, a,b,c,const) /* c = a &~ b  */
#define M_ORNOT(a,b,c,const)    M_OP3 (0x11,0x28, a,b,c,const) /* c = a |~ b  */
#define M_XORNOT(a,b,c,const)   M_OP3 (0x11,0x48, a,b,c,const) /* c = a ^~ b  */

#define M_CMPBGE(a,b,c,const)   M_OP3 (0x10,0x0f, a,b,c,const)

#define M_FCMPUN(a,b,c)         M_FOP3 (0x16, 0x0a4, a,b,c)    /* unordered   */
#define M_FCMPLE(a,b,c)         M_FOP3 (0x16, 0x0a7, a,b,c)    /* c = a<=b    */

#define M_FCMPUNS(a,b,c)        M_FOP3 (0x16, 0x5a4, a,b,c)    /* unordered   */
#define M_FCMPLES(a,b,c)        M_FOP3 (0x16, 0x5a7, a,b,c)    /* c = a<=b    */

#define M_FBNEZ(fa,disp)        M_BRA (0x35,fa,disp)
#define M_FBLEZ(fa,disp)        M_BRA (0x33,fa,disp)

#define M_JMP_CO(a,b)           M_MEM (0x1a,a,b,0xc000)        /* call cosub  */

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
 */
