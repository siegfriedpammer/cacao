/* jit/i386/codegen.h - code generation macros and definitions for x86_64

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

   $Id: codegen.h 573 2003-11-07 08:53:55Z twisti $

*/


#ifndef _CODEGEN_H
#define _CODEGEN_H

#include "jit.h"


/* x86_64 register numbers */
#define RIP    -1
#define RAX    0
#define RCX    1
#define RDX    2
#define RBX    3
#define RSP    4
#define RBP    5
#define RSI    6
#define RDI    7
#define R8     8
#define R9     9
#define R10    10
#define R11    11
#define R12    12
#define R13    13
#define R14    14
#define R15    15


#define XMM0   0
#define XMM1   1
#define XMM2   2
#define XMM3   3
#define XMM4   4
#define XMM5   5
#define XMM6   6
#define XMM7   7
#define XMM8   8
#define XMM9   9
#define XMM10  10
#define XMM11  11
#define XMM12  12
#define XMM13  13
#define XMM14  14
#define XMM15  15


/* preallocated registers *****************************************************/

/* integer registers */
  
#define REG_RESULT      RAX      /* to deliver method results                 */

#define REG_ITMP1       RAX      /* temporary register                        */
#define REG_ITMP2       R10      /* temporary register and method pointer     */
#define REG_ITMP3       R11      /* temporary register                        */

#define REG_NULL        -1       /* used for reg_of_var where d is not needed */

#define REG_ITMP1_XPTR  RAX      /* exception pointer = temporary register 1  */
#define REG_ITMP2_XPC   R10      /* exception pc = temporary register 2       */

#define REG_SP          RSP      /* stack pointer                             */

/* floating point registers */

#define REG_FRESULT     XMM0     /* to deliver floating point method results  */

#define REG_FTMP1       XMM8     /* temporary floating point register         */
#define REG_FTMP2       XMM9     /* temporary floating point register         */
#define REG_FTMP3       XMM10    /* temporary floating point register         */


/* stackframe-infos ***********************************************************/

int parentargs_base; /* offset in stackframe for the parameter from the caller*/


/* macros to create code ******************************************************/

/* immediate data union */

typedef union {
    s4 i;
    s8 l;
    float f;
    double d;
    void *a;
    u1 b[8];
} x86_64_imm_buf;


/* opcodes for alu instructions */

typedef enum {
    X86_64_ADD = 0,
    X86_64_OR  = 1,
    X86_64_ADC = 2,
    X86_64_SBB = 3,
    X86_64_AND = 4,
    X86_64_SUB = 5,
    X86_64_XOR = 6,
    X86_64_CMP = 7,
    X86_64_NALU
} X86_64_ALU_Opcode;


typedef enum {
    X86_64_ROL = 0,
    X86_64_ROR = 1,
    X86_64_RCL = 2,
    X86_64_RCR = 3,
    X86_64_SHL = 4,
    X86_64_SHR = 5,
    X86_64_SAR = 7,
    X86_64_NSHIFT = 8
} X86_64_Shift_Opcode;


typedef enum {
    X86_64_CC_O = 0,
    X86_64_CC_NO = 1,
    X86_64_CC_B = 2, X86_64_CC_C = 2, X86_64_CC_NAE = 2,
    X86_64_CC_BE = 6, X86_64_CC_NA = 6,
    X86_64_CC_AE = 3, X86_64_CC_NB = 3, X86_64_CC_NC = 3,
    X86_64_CC_E = 4, X86_64_CC_Z = 4,
    X86_64_CC_NE = 5, X86_64_CC_NZ = 5,
    X86_64_CC_A = 7, X86_64_CC_NBE = 7,
    X86_64_CC_S = 8, X86_64_CC_LZ = 8,
    X86_64_CC_NS = 9, X86_64_CC_GEZ = 9,
    X86_64_CC_P = 0x0a, X86_64_CC_PE = 0x0a,
    X86_64_CC_NP = 0x0b, X86_64_CC_PO = 0x0b,
    X86_64_CC_L = 0x0c, X86_64_CC_NGE = 0x0c,
    X86_64_CC_GE = 0x0d, X86_64_CC_NL = 0x0d,
    X86_64_CC_LE = 0x0e, X86_64_CC_NG = 0x0e,
    X86_64_CC_G = 0x0f, X86_64_CC_NLE = 0x0f,
    X86_64_NCC
} X86_64_CC;


/* modrm and stuff */

#define x86_64_address_byte(mod,reg,rm) \
    *(mcodeptr++) = ((((mod) & 0x03) << 6) | (((reg) & 0x07) << 3) | ((rm) & 0x07));


#define x86_64_emit_reg(reg,rm) \
    x86_64_address_byte(3,(reg),(rm));


#define x86_64_emit_rex(size,reg,index,rm) \
    if ((size) == 1 || (reg) > 7 || (index) > 7 || (rm) > 7) { \
        *(mcodeptr++) = (0x40 | (((size) & 0x01) << 3) | ((((reg) >> 3) & 0x01) << 2) | ((((index) >> 3) & 0x01) << 1) | (((rm) >> 3) & 0x01)); \
    }


#define x86_64_emit_mem(r,disp) \
    do { \
        x86_64_address_byte(0,(r),5); \
        x86_64_emit_imm32((disp)); \
    } while (0)


#define x86_64_emit_membase(basereg,disp,dreg) \
    do { \
        if ((basereg) == REG_SP || (basereg) == R12) { \
            if ((disp) == 0) { \
                x86_64_address_byte(0,(dreg),REG_SP); \
                x86_64_address_byte(0,REG_SP,REG_SP); \
            } else if (x86_64_is_imm8((disp))) { \
                x86_64_address_byte(1,(dreg),REG_SP); \
                x86_64_address_byte(0,REG_SP,REG_SP); \
                x86_64_emit_imm8((disp)); \
            } else { \
                x86_64_address_byte(2,(dreg),REG_SP); \
                x86_64_address_byte(0,REG_SP,REG_SP); \
                x86_64_emit_imm32((disp)); \
            } \
            break; \
        } \
        if ((disp) == 0 && (basereg) != RBP && (basereg) != R13) { \
            x86_64_address_byte(0,(dreg),(basereg)); \
            break; \
        } \
        \
        if ((basereg) == RIP) { \
            x86_64_address_byte(0,(dreg),RBP); \
            x86_64_emit_imm32((disp)); \
            break; \
        } \
        \
        if (x86_64_is_imm8((disp))) { \
            x86_64_address_byte(1,(dreg),(basereg)); \
            x86_64_emit_imm8((disp)); \
        } else { \
            x86_64_address_byte(2,(dreg),(basereg)); \
            x86_64_emit_imm32((disp)); \
        } \
    } while (0)


#define x86_64_emit_memindex(reg,disp,basereg,indexreg,scale) \
    do { \
        if ((basereg) == -1) { \
            x86_64_address_byte(0,(reg),4); \
            x86_64_address_byte((scale),(indexreg),5); \
            x86_64_emit_imm32((disp)); \
        \
        } else if ((disp) == 0 && (basereg) != RBP && (basereg) != R13) { \
            x86_64_address_byte(0,(reg),4); \
            x86_64_address_byte((scale),(indexreg),(basereg)); \
        \
        } else if (x86_64_is_imm8((disp))) { \
            x86_64_address_byte(1,(reg),4); \
            x86_64_address_byte((scale),(indexreg),(basereg)); \
            x86_64_emit_imm8 ((disp)); \
        \
        } else { \
            x86_64_address_byte(2,(reg),4); \
            x86_64_address_byte((scale),(indexreg),(basereg)); \
            x86_64_emit_imm32((disp)); \
        }    \
     } while (0)


#define x86_64_is_imm8(imm) \
    (((long)(imm) >= -128 && (long)(imm) <= 127))


#define x86_64_is_imm32(imm) \
    ((long)(imm) >= (-2147483647-1) && (long)(imm) <= 2147483647)


#define x86_64_emit_imm8(imm) \
    *(mcodeptr++) = (u1) ((imm) & 0xff);


#define x86_64_emit_imm16(imm) \
    do { \
        x86_64_imm_buf imb; \
        imb.i = (s4) (imm); \
        *(mcodeptr++) = imb.b[0]; \
        *(mcodeptr++) = imb.b[1]; \
    } while (0)


#define x86_64_emit_imm32(imm) \
    do { \
        x86_64_imm_buf imb; \
        imb.i = (s4) (imm); \
        *(mcodeptr++) = imb.b[0]; \
        *(mcodeptr++) = imb.b[1]; \
        *(mcodeptr++) = imb.b[2]; \
        *(mcodeptr++) = imb.b[3]; \
    } while (0)


#define x86_64_emit_imm64(imm) \
    do { \
        x86_64_imm_buf imb; \
        imb.l = (s8) (imm); \
        *(mcodeptr++) = imb.b[0]; \
        *(mcodeptr++) = imb.b[1]; \
        *(mcodeptr++) = imb.b[2]; \
        *(mcodeptr++) = imb.b[3]; \
        *(mcodeptr++) = imb.b[4]; \
        *(mcodeptr++) = imb.b[5]; \
        *(mcodeptr++) = imb.b[6]; \
        *(mcodeptr++) = imb.b[7]; \
    } while (0)


/* code generation prototypes */

void x86_64_emit_ialu(s4 alu_op, stackptr src, instruction *iptr);
void x86_64_emit_lalu(s4 alu_op, stackptr src, instruction *iptr);
void x86_64_emit_ialuconst(s4 alu_op, stackptr src, instruction *iptr);
void x86_64_emit_laluconst(s4 alu_op, stackptr src, instruction *iptr);
void x86_64_emit_ishift(s4 shift_op, stackptr src, instruction *iptr);
void x86_64_emit_lshift(s4 shift_op, stackptr src, instruction *iptr);
void x86_64_emit_ishiftconst(s4 shift_op, stackptr src, instruction *iptr);
void x86_64_emit_lshiftconst(s4 shift_op, stackptr src, instruction *iptr);
void x86_64_emit_ifcc(s4 if_op, stackptr src, instruction *iptr);
void x86_64_emit_if_lcc(s4 if_op, stackptr src, instruction *iptr);
void x86_64_emit_if_icmpcc(s4 if_op, stackptr src, instruction *iptr);
void x86_64_emit_if_lcmpcc(s4 if_op, stackptr src, instruction *iptr);


/* integer instructions */

void x86_64_mov_reg_reg(s8 reg, s8 dreg);
void x86_64_mov_imm_reg(s8 imm, s8 reg);
void x86_64_movl_imm_reg(s8 imm, s8 reg);
void x86_64_mov_membase_reg(s8 basereg, s8 disp, s8 reg);
void x86_64_movl_membase_reg(s8 basereg, s8 disp, s8 reg);
void x86_64_mov_membase32_reg(s8 basereg, s8 disp, s8 reg);
void x86_64_mov_reg_membase(s8 reg, s8 basereg, s8 disp);
void x86_64_movl_reg_membase(s8 reg, s8 basereg, s8 disp);
void x86_64_mov_memindex_reg(s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg);
void x86_64_movl_memindex_reg(s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg);
void x86_64_mov_reg_memindex(s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale);
void x86_64_movl_reg_memindex(s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale);
void x86_64_movw_reg_memindex(s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale);
void x86_64_movb_reg_memindex(s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale);
void x86_64_mov_imm_membase(s8 imm, s8 basereg, s8 disp);
void x86_64_movl_imm_membase(s8 imm, s8 basereg, s8 disp);
void x86_64_movsbq_reg_reg(s8 reg, s8 dreg);
void x86_64_movsbq_membase_reg(s8 basereg, s8 disp, s8 dreg);
void x86_64_movswq_reg_reg(s8 reg, s8 dreg);
void x86_64_movswq_membase_reg(s8 basereg, s8 disp, s8 dreg);
void x86_64_movslq_reg_reg(s8 reg, s8 dreg);
void x86_64_movslq_membase_reg(s8 basereg, s8 disp, s8 dreg);
void x86_64_movzwq_reg_reg(s8 reg, s8 dreg);
void x86_64_movzwq_membase_reg(s8 basereg, s8 disp, s8 dreg);
void x86_64_movswq_memindex_reg(s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg);
void x86_64_movsbq_memindex_reg(s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg);
void x86_64_movzwq_memindex_reg(s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg);
void x86_64_alu_reg_reg(s8 opc, s8 reg, s8 dreg);
void x86_64_alul_reg_reg(s8 opc, s8 reg, s8 dreg);
void x86_64_alu_reg_membase(s8 opc, s8 reg, s8 basereg, s8 disp);
void x86_64_alul_reg_membase(s8 opc, s8 reg, s8 basereg, s8 disp);
void x86_64_alu_membase_reg(s8 opc, s8 basereg, s8 disp, s8 reg);
void x86_64_alul_membase_reg(s8 opc, s8 basereg, s8 disp, s8 reg);
void x86_64_alu_imm_reg(s8 opc, s8 imm, s8 dreg);
void x86_64_alul_imm_reg(s8 opc, s8 imm, s8 dreg);
void x86_64_alu_imm_membase(s8 opc, s8 imm, s8 basereg, s8 disp);
void x86_64_alul_imm_membase(s8 opc, s8 imm, s8 basereg, s8 disp);
void x86_64_test_reg_reg(s8 reg, s8 dreg);
void x86_64_testl_reg_reg(s8 reg, s8 dreg);
void x86_64_test_imm_reg(s8 imm, s8 reg);
void x86_64_testw_imm_reg(s8 imm, s8 reg);
void x86_64_testb_imm_reg(s8 imm, s8 reg);
void x86_64_lea_membase_reg(s8 basereg, s8 disp, s8 reg);
void x86_64_leal_membase_reg(s8 basereg, s8 disp, s8 reg);
void x86_64_inc_reg(s8 reg);
void x86_64_incl_reg(s8 reg);
void x86_64_inc_membase(s8 basereg, s8 disp);
void x86_64_incl_membase(s8 basereg, s8 disp);
void x86_64_dec_reg(s8 reg);
void x86_64_decl_reg(s8 reg);
void x86_64_dec_membase(s8 basereg, s8 disp);
void x86_64_decl_membase(s8 basereg, s8 disp);
void x86_64_cltd();
void x86_64_cqto();
void x86_64_imul_reg_reg(s8 reg, s8 dreg);
void x86_64_imull_reg_reg(s8 reg, s8 dreg);
void x86_64_imul_membase_reg(s8 basereg, s8 disp, s8 dreg);
void x86_64_imull_membase_reg(s8 basereg, s8 disp, s8 dreg);
void x86_64_imul_imm_reg(s8 imm, s8 dreg);
void x86_64_imul_imm_reg_reg(s8 imm,s8 reg, s8 dreg);
void x86_64_imull_imm_reg_reg(s8 imm, s8 reg, s8 dreg);
void x86_64_imul_imm_membase_reg(s8 imm, s8 basereg, s8 disp, s8 dreg);
void x86_64_imull_imm_membase_reg(s8 imm, s8 basereg, s8 disp, s8 dreg);
void x86_64_idiv_reg(s8 reg);
void x86_64_idivl_reg(s8 reg);
void x86_64_ret();
void x86_64_shift_reg(s8 opc, s8 reg);
void x86_64_shiftl_reg(s8 opc, s8 reg);
void x86_64_shift_membase(s8 opc, s8 basereg, s8 disp);
void x86_64_shiftl_membase(s8 opc, s8 basereg, s8 disp);
void x86_64_shift_imm_reg(s8 opc, s8 imm, s8 dreg);
void x86_64_shiftl_imm_reg(s8 opc, s8 imm, s8 dreg);
void x86_64_shift_imm_membase(s8 opc, s8 imm, s8 basereg, s8 disp);
void x86_64_shiftl_imm_membase(s8 opc, s8 imm, s8 basereg, s8 disp);
void x86_64_jmp_imm(s8 imm);
void x86_64_jmp_reg(s8 reg);
void x86_64_jcc(s8 opc, s8 imm);
void x86_64_setcc_reg(s8 opc, s8 reg);
void x86_64_setcc_membase(s8 opc, s8 basereg, s8 disp);
void x86_64_cmovcc_reg_reg(s8 opc, s8 reg, s8 dreg);
void x86_64_cmovccl_reg_reg(s8 opc, s8 reg, s8 dreg);
void x86_64_neg_reg(s8 reg);
void x86_64_negl_reg(s8 reg);
void x86_64_neg_membase(s8 basereg, s8 disp);
void x86_64_negl_membase(s8 basereg, s8 disp);
void x86_64_push_imm(s8 imm);
void x86_64_pop_reg(s8 reg);
void x86_64_xchg_reg_reg(s8 reg, s8 dreg);
void x86_64_nop();
void x86_64_call_reg(s8 reg);
void x86_64_call_imm(s8 imm);


/* floating point instructions (SSE2) */

void x86_64_addsd_reg_reg(s8 reg, s8 dreg);
void x86_64_addss_reg_reg(s8 reg, s8 dreg);
void x86_64_cvtsi2ssq_reg_reg(s8 reg, s8 dreg);
void x86_64_cvtsi2ss_reg_reg(s8 reg, s8 dreg);
void x86_64_cvtsi2sdq_reg_reg(s8 reg, s8 dreg);
void x86_64_cvtsi2sd_reg_reg(s8 reg, s8 dreg);
void x86_64_cvtss2sd_reg_reg(s8 reg, s8 dreg);
void x86_64_cvtsd2ss_reg_reg(s8 reg, s8 dreg);
void x86_64_cvttss2siq_reg_reg(s8 reg, s8 dreg);
void x86_64_cvttss2si_reg_reg(s8 reg, s8 dreg);
void x86_64_cvttsd2siq_reg_reg(s8 reg, s8 dreg);
void x86_64_cvttsd2si_reg_reg(s8 reg, s8 dreg);
void x86_64_divss_reg_reg(s8 reg, s8 dreg);
void x86_64_divsd_reg_reg(s8 reg, s8 dreg);
void x86_64_movd_reg_freg(s8 reg, s8 freg);
void x86_64_movd_freg_reg(s8 freg, s8 reg);
void x86_64_movd_reg_membase(s8 reg, s8 basereg, s8 disp);
void x86_64_movd_reg_memindex(s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale);
void x86_64_movd_membase_reg(s8 basereg, s8 disp, s8 dreg);
void x86_64_movdl_membase_reg(s8 basereg, s8 disp, s8 dreg);
void x86_64_movd_memindex_reg(s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 dreg);
void x86_64_movq_reg_reg(s8 reg, s8 dreg);
void x86_64_movq_reg_membase(s8 reg, s8 basereg, s8 disp);
void x86_64_movq_membase_reg(s8 basereg, s8 disp, s8 dreg);
void x86_64_movss_reg_reg(s8 reg, s8 dreg);
void x86_64_movsd_reg_reg(s8 reg, s8 dreg);
void x86_64_movss_reg_membase(s8 reg, s8 basereg, s8 disp);
void x86_64_movsd_reg_membase(s8 reg, s8 basereg, s8 disp);
void x86_64_movss_membase_reg(s8 basereg, s8 disp, s8 dreg);
void x86_64_movlps_membase_reg(s8 basereg, s8 disp, s8 dreg);
void x86_64_movsd_membase_reg(s8 basereg, s8 disp, s8 dreg);
void x86_64_movlpd_membase_reg(s8 basereg, s8 disp, s8 dreg);
void x86_64_movss_reg_memindex(s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale);
void x86_64_movsd_reg_memindex(s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale);
void x86_64_movss_memindex_reg(s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 dreg);
void x86_64_movsd_memindex_reg(s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 dreg);
void x86_64_mulss_reg_reg(s8 reg, s8 dreg);
void x86_64_mulsd_reg_reg(s8 reg, s8 dreg);
void x86_64_subss_reg_reg(s8 reg, s8 dreg);
void x86_64_subsd_reg_reg(s8 reg, s8 dreg);
void x86_64_ucomiss_reg_reg(s8 reg, s8 dreg);
void x86_64_ucomisd_reg_reg(s8 reg, s8 dreg);
void x86_64_xorps_reg_reg(s8 reg, s8 dreg);
void x86_64_xorps_membase_reg(s8 basereg, s8 disp, s8 dreg);
void x86_64_xorpd_reg_reg(s8 reg, s8 dreg);
void x86_64_xorpd_membase_reg(s8 basereg, s8 disp, s8 dreg);


/* function gen_resolvebranch **************************************************

    backpatches a branch instruction

    parameters: ip ... pointer to instruction after branch (void*)
                so ... offset of instruction after branch  (s8)
                to ... offset of branch target             (s8)

*******************************************************************************/

#define gen_resolvebranch(ip,so,to) \
    *((s4*) ((ip) - 4)) = (s4) ((to) - (so));

#define SOFTNULLPTRCHECK       /* soft null pointer check supportet as option */

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

