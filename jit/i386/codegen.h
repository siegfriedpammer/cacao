/* jit/i386/codegen.h - code generation macros and definitions for i386

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser,
   M. Probst, S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck,
   P. Tomsich, J. Wenninger

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Andreas Krall
            Christian Thalinger

   $Id: codegen.h 588 2003-11-09 19:42:00Z twisti $

*/


#ifndef _CODEGEN_H
#define _CODEGEN_H

#include "jit.h"


/* define x86 register numbers */
#define I386_EAX    0
#define I386_ECX    1
#define I386_EDX    2
#define I386_EBX    3
#define I386_ESP    4
#define I386_EBP    5
#define I386_ESI    6
#define I386_EDI    7


/* preallocated registers *****************************************************/

/* integer registers */
  
#define REG_RESULT      I386_EAX /* to deliver method results                 */
#define REG_RESULT2     I386_EDX /* to deliver long method results            */

#define REG_ITMP1       I386_EAX /* temporary register                        */
#define REG_ITMP2       I386_EDX /* temporary register and method pointer     */
#define REG_ITMP3       I386_ECX /* temporary register                        */

#define REG_NULL        -1       /* used for reg_of_var where d is not needed */

#define REG_ITMP1_XPTR  I386_EAX /* exception pointer = temporary register 1  */
#define REG_ITMP2_XPC   I386_EDX /* exception pc = temporary register 2       */

#define REG_SP          I386_ESP /* stack pointer                             */

/* floating point registers */

#define REG_FRESULT     0    /* to deliver floating point method results      */
#define REG_FTMP1       6    /* temporary floating point register             */
#define REG_FTMP2       7    /* temporary floating point register             */
#define REG_FTMP3       7    /* temporary floating point register             */


static u1 fpu_in_24bit_mode = 0;

static u2 fpu_ctrlwrd_24bit = 0x007f;    /* Round to nearest, 24-bit mode, exceptions masked */
static u2 fpu_ctrlwrd_53bit = 0x027f;    /* Round to nearest, 53-bit mode, exceptions masked */

static u4 subnormal_bias1[3] = { 0x00000000, 0x80000000, 0x03ff };    /* 2^(-15360) */
static u4 subnormal_bias2[3] = { 0x00000000, 0x80000000, 0x7bff };    /* 2^(+15360) */


/* macros to create code ******************************************************/

/* immediate data union */

typedef union {
    s4 i;
    s8 l;
    float f;
    double d;
    void *a;
    u1 b[8];
} i386_imm_buf;


typedef enum {
    I386_AL = 0,
    I386_CL = 1,
    I386_DL = 2,
    I386_BL = 3,
    I386_AH = 4,
    I386_CH = 5,
    I386_DH = 6,
    I386_BH = 7,
    I386_NREGB
} I386_RegB_No;


/* opcodes for alu instructions */

typedef enum {
    I386_ADD = 0,
    I386_OR  = 1,
    I386_ADC = 2,
    I386_SBB = 3,
    I386_AND = 4,
    I386_SUB = 5,
    I386_XOR = 6,
    I386_CMP = 7,
    I386_NALU
} I386_ALU_Opcode;

typedef enum {
    I386_ROL = 0,
    I386_ROR = 1,
    I386_RCL = 2,
    I386_RCR = 3,
    I386_SHL = 4,
    I386_SHR = 5,
    I386_SAR = 7,
    I386_NSHIFT = 8
} I386_Shift_Opcode;

typedef enum {
    I386_CC_O = 0,
    I386_CC_NO = 1,
    I386_CC_B = 2, I386_CC_C = 2, I386_CC_NAE = 2,
    I386_CC_BE = 6, I386_CC_NA = 6,
    I386_CC_AE = 3, I386_CC_NB = 3, I386_CC_NC = 3,
    I386_CC_E = 4, I386_CC_Z = 4,
    I386_CC_NE = 5, I386_CC_NZ = 5,
    I386_CC_A = 7, I386_CC_NBE = 7,
    I386_CC_S = 8, I386_CC_LZ = 8,
    I386_CC_NS = 9, I386_CC_GEZ = 9,
    I386_CC_P = 0x0a, I386_CC_PE = 0x0a,
    I386_CC_NP = 0x0b, I386_CC_PO = 0x0b,
    I386_CC_L = 0x0c, I386_CC_NGE = 0x0c,
    I386_CC_GE = 0x0d, I386_CC_NL = 0x0d,
    I386_CC_LE = 0x0e, I386_CC_NG = 0x0e,
    I386_CC_G = 0x0f, I386_CC_NLE = 0x0f,
    I386_NCC
} I386_CC;


/* modrm and stuff */

#define i386_address_byte(mod, reg, rm) \
    *(mcodeptr++) = ((((mod) & 0x03) << 6) | (((reg) & 0x07) << 3) | (((rm) & 0x07)));


#define i386_emit_reg(reg,rm) \
    i386_address_byte(3,(reg),(rm));


#define i386_is_imm8(imm) \
    (((int)(imm) >= -128 && (int)(imm) <= 127))


#define i386_emit_imm8(imm) \
    *(mcodeptr++) = (u1) ((imm) & 0xff);


#define i386_emit_imm16(imm) \
    do { \
        i386_imm_buf imb; \
        imb.i = (int) (imm); \
        *(mcodeptr++) = imb.b[0]; \
        *(mcodeptr++) = imb.b[1]; \
    } while (0)


#define i386_emit_imm32(imm) \
    do { \
        i386_imm_buf imb; \
        imb.i = (int) (imm); \
        *(mcodeptr++) = imb.b[0]; \
        *(mcodeptr++) = imb.b[1]; \
        *(mcodeptr++) = imb.b[2]; \
        *(mcodeptr++) = imb.b[3]; \
    } while (0)


#define i386_emit_mem(r,disp) \
    do { \
        i386_address_byte(0,(r),5); \
        i386_emit_imm32((disp)); \
    } while (0)


#define i386_emit_membase(basereg,disp,dreg) \
    do { \
        if ((basereg) == I386_ESP) { \
            if ((disp) == 0) { \
                i386_address_byte(0, (dreg), I386_ESP); \
                i386_address_byte(0, I386_ESP, I386_ESP); \
            } else if (i386_is_imm8((disp))) { \
                i386_address_byte(1, (dreg), I386_ESP); \
                i386_address_byte(0, I386_ESP, I386_ESP); \
                i386_emit_imm8((disp)); \
            } else { \
                i386_address_byte(2, (dreg), I386_ESP); \
                i386_address_byte(0, I386_ESP, I386_ESP); \
                i386_emit_imm32((disp)); \
            } \
            break; \
        } \
        \
        if ((disp) == 0 && (basereg) != I386_EBP) { \
            i386_address_byte(0, (dreg), (basereg)); \
            break; \
        } \
        \
        if (i386_is_imm8((disp))) { \
            i386_address_byte(1, (dreg), (basereg)); \
            i386_emit_imm8((disp)); \
        } else { \
            i386_address_byte(2, (dreg), (basereg)); \
            i386_emit_imm32((disp)); \
        } \
    } while (0)


#define i386_emit_memindex(reg,disp,basereg,indexreg,scale) \
    do { \
        if ((basereg) == -1) { \
            i386_address_byte(0, (reg), 4); \
            i386_address_byte((scale), (indexreg), 5); \
            i386_emit_imm32((disp)); \
        \
        } else if ((disp) == 0 && (basereg) != I386_EBP) { \
            i386_address_byte(0, (reg), 4); \
            i386_address_byte((scale), (indexreg), (basereg)); \
        \
        } else if (i386_is_imm8((disp))) { \
            i386_address_byte(1, (reg), 4); \
            i386_address_byte((scale), (indexreg), (basereg)); \
            i386_emit_imm8 ((disp)); \
        \
        } else { \
            i386_address_byte(2, (reg), 4); \
            i386_address_byte((scale), (indexreg), (basereg)); \
            i386_emit_imm32((disp)); \
        }    \
     } while (0)


/* code generation prototypes */

void i386_emit_ialu(s4 alu_op, stackptr src, instruction *iptr);
void i386_emit_ialuconst(s4 alu_op, stackptr src, instruction *iptr);
void i386_emit_lalu(s4 alu_op, stackptr src, instruction *iptr);
void i386_emit_laluconst(s4 alu_op, stackptr src, instruction *iptr);
void i386_emit_ishift(s4 shift_op, stackptr src, instruction *iptr);
void i386_emit_ishiftconst(s4 shift_op, stackptr src, instruction *iptr);
void i386_emit_ifcc_iconst(s4 if_op, stackptr src, instruction *iptr);


/* integer instructions */

void i386_mov_reg_reg(s4 reg, s4 dreg);
void i386_mov_imm_reg(s4 imm, s4 dreg);
void i386_movb_imm_reg(s4 imm, s4 dreg);
void i386_mov_membase_reg(s4 basereg, s4 disp, s4 reg);
void i386_mov_membase32_reg(s4 basereg, s4 disp, s4 reg);
void i386_mov_reg_membase(s4 reg, s4 basereg, s4 disp);
void i386_mov_memindex_reg(s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg);
void i386_mov_reg_memindex(s4 reg, s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_movw_reg_memindex(s4 reg, s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_movb_reg_memindex(s4 reg, s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_mov_imm_membase(s4 imm, s4 basereg, s4 disp);
void i386_movsbl_memindex_reg(s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg);
void i386_movswl_memindex_reg(s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg);
void i386_movzwl_memindex_reg(s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg);
void i386_alu_reg_reg(s4 opc, s4 reg, s4 dreg);
void i386_alu_reg_membase(s4 opc, s4 reg, s4 basereg, s4 disp);
void i386_alu_membase_reg(s4 opc, s4 basereg, s4 disp, s4 reg);
void i386_alu_imm_reg(s4 opc, s4 imm, s4 reg);
void i386_alu_imm_membase(s4 opc, s4 imm, s4 basereg, s4 disp);
void i386_test_reg_reg(s4 reg, s4 dreg);
void i386_test_imm_reg(s4 imm, s4 dreg);
void i386_inc_reg(s4 reg);
void i386_inc_membase(s4 basereg, s4 disp);
void i386_dec_reg(s4 reg);
void i386_dec_membase(s4 basereg, s4 disp);
void i386_cltd();
void i386_imul_reg_reg(s4 reg, s4 dreg);
void i386_imul_membase_reg(s4 basereg, s4 disp, s4 dreg);
void i386_imul_imm_reg(s4 imm, s4 reg);
void i386_imul_imm_reg_reg(s4 imm, s4 reg, s4 dreg);
void i386_imul_imm_membase_reg(s4 imm, s4 basereg, s4 disp, s4 dreg);
void i386_mul_membase(s4 basereg, s4 disp);
void i386_idiv_reg(s4 reg);
void i386_ret();
void i386_shift_reg(s4 opc, s4 reg);
void i386_shift_membase(s4 opc, s4 basereg, s4 disp);
void i386_shift_imm_reg(s4 opc, s4 imm, s4 reg);
void i386_shift_imm_membase(s4 opc, s4 imm, s4 basereg, s4 disp);
void i386_shld_reg_reg(s4 reg, s4 dreg);
void i386_shld_imm_reg_reg(s4 imm, s4 reg, s4 dreg);
void i386_shld_reg_membase(s4 reg, s4 basereg, s4 disp);
void i386_shrd_reg_reg(s4 reg, s4 dreg);
void i386_shrd_imm_reg_reg(s4 imm, s4 reg, s4 dreg);
void i386_shrd_reg_membase(s4 reg, s4 basereg, s4 disp);
void i386_jmp_imm(s4 imm);
void i386_jmp_reg(s4 reg);
void i386_jcc(s4 opc, s4 imm);
void i386_setcc_reg(s4 opc, s4 reg);
void i386_setcc_membase(s4 opc, s4 basereg, s4 disp);
void i386_neg_reg(s4 reg);
void i386_neg_membase(s4 basereg, s4 disp);
void i386_push_imm(s4 imm);
void i386_pop_reg(s4 reg);
void i386_nop();
void i386_call_reg(s4 reg);
void i386_call_imm(s4 imm);


/* floating point instructions */

void i386_fld1();
void i386_fldz();
void i386_fld_reg(s4 reg);
void i386_flds_membase(s4 basereg, s4 disp);
void i386_fldl_membase(s4 basereg, s4 disp);
void i386_fldt_membase(s4 basereg, s4 disp);
void i386_flds_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_fldl_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_fildl_membase(s4 basereg, s4 disp);
void i386_fildll_membase(s4 basereg, s4 disp);
void i386_fst_reg(s4 reg);
void i386_fsts_membase(s4 basereg, s4 disp);
void i386_fstl_membase(s4 basereg, s4 disp);
void i386_fsts_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_fstl_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_fstp_reg(s4 reg);
void i386_fstps_membase(s4 basereg, s4 disp);
void i386_fstpl_membase(s4 basereg, s4 disp);
void i386_fstpt_membase(s4 basereg, s4 disp);
void i386_fstps_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_fstpl_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_fistl_membase(s4 basereg, s4 disp);
void i386_fistpl_membase(s4 basereg, s4 disp);
void i386_fistpll_membase(s4 basereg, s4 disp);
void i386_fchs();
void i386_faddp();
void i386_fadd_reg_st(s4 reg);
void i386_fadd_st_reg(s4 reg);
void i386_faddp_st_reg(s4 reg);
void i386_fadds_membase(s4 basereg, s4 disp);
void i386_faddl_membase(s4 basereg, s4 disp);
void i386_fsub_reg_st(s4 reg);
void i386_fsub_st_reg(s4 reg);
void i386_fsubp_st_reg(s4 reg);
void i386_fsubp();
void i386_fsubs_membase(s4 basereg, s4 disp);
void i386_fsubl_membase(s4 basereg, s4 disp);
void i386_fmul_reg_st(s4 reg);
void i386_fmul_st_reg(s4 reg);
void i386_fmulp();
void i386_fmulp_st_reg(s4 reg);
void i386_fmuls_membase(s4 basereg, s4 disp);
void i386_fmull_membase(s4 basereg, s4 disp);
void i386_fdiv_reg_st(s4 reg);
void i386_fdiv_st_reg(s4 reg);
void i386_fdivp();
void i386_fdivp_st_reg(s4 reg);
void i386_fxch();
void i386_fxch_reg(s4 reg);
void i386_fprem();
void i386_fprem1();
void i386_fucom();
void i386_fucom_reg(s4 reg);
void i386_fucomp_reg(s4 reg);
void i386_fucompp();
void i386_fnstsw();
void i386_sahf();
void i386_finit();
void i386_fldcw_mem(s4 mem);
void i386_fldcw_membase(s4 basereg, s4 disp);
void i386_wait();
void i386_ffree_reg(s4 reg);
void i386_fdecstp();
void i386_fincstp();


/* function gen_resolvebranch **************************************************

    backpatches a branch instruction

    parameters: ip ... pointer to instruction after branch (void*)
                so ... offset of instruction after branch  (s4)
                to ... offset of branch target             (s4)

*******************************************************************************/

#define gen_resolvebranch(ip,so,to) \
    *((void **) ((ip) - 4)) = (void **) ((to) - (so));


/* function prototypes */
void codegen_init();
void init_exceptions();
void codegen();
void codegen_close();
void dseg_display(s4 *s4ptr);


#endif /* _CODEGEN_H */


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
