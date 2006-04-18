/* src/vm/jit/x86_64/codegen.h - code generation macros for x86_64

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

   Authors: Andreas Krall
            Christian Thalinger

   Changes:

   $Id: codegen.h 4789 2006-04-18 20:34:52Z twisti $

*/


#ifndef _CODEGEN_H
#define _CODEGEN_H

#include "config.h"

#include <ucontext.h>

#include "vm/types.h"

#include "vm/jit/jit.h"


/* some defines ***************************************************************/

#define PATCHER_CALL_SIZE    5          /* size in bytes of a patcher call    */


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


#define IS_IMM8(imm) \
    (((long) (imm) >= -128) && ((long) (imm) <= 127))


#define IS_IMM32(imm) \
    (((long) (imm) >= (-2147483647-1)) && ((long) (imm) <= 2147483647))


/* modrm and stuff */

#define x86_64_address_byte(mod,reg,rm) \
    *(cd->mcodeptr++) = ((((mod) & 0x03) << 6) | (((reg) & 0x07) << 3) | ((rm) & 0x07));


#define x86_64_emit_reg(reg,rm) \
    x86_64_address_byte(3,(reg),(rm));


#define x86_64_emit_rex(size,reg,index,rm) \
    if (((size) == 1) || ((reg) > 7) || ((index) > 7) || ((rm) > 7)) { \
        *(cd->mcodeptr++) = (0x40 | (((size) & 0x01) << 3) | ((((reg) >> 3) & 0x01) << 2) | ((((index) >> 3) & 0x01) << 1) | (((rm) >> 3) & 0x01)); \
    }


#define x86_64_emit_byte_rex(reg,index,rm) \
    *(cd->mcodeptr++) = (0x40 | ((((reg) >> 3) & 0x01) << 2) | ((((index) >> 3) & 0x01) << 1) | (((rm) >> 3) & 0x01));


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
            } else if (IS_IMM8((disp))) { \
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
        if (IS_IMM8((disp))) { \
            x86_64_address_byte(1,(dreg),(basereg)); \
            x86_64_emit_imm8((disp)); \
        } else { \
            x86_64_address_byte(2,(dreg),(basereg)); \
            x86_64_emit_imm32((disp)); \
        } \
    } while (0)


#define x86_64_emit_membase32(basereg,disp,dreg) \
    do { \
        if ((basereg) == REG_SP || (basereg) == R12) { \
            x86_64_address_byte(2,(dreg),REG_SP); \
            x86_64_address_byte(0,REG_SP,REG_SP); \
            x86_64_emit_imm32((disp)); \
        } else {\
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
        } else if (IS_IMM8((disp))) { \
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


#define x86_64_emit_imm8(imm) \
    *(cd->mcodeptr++) = (u1) ((imm) & 0xff);


#define x86_64_emit_imm16(imm) \
    do { \
        x86_64_imm_buf imb; \
        imb.i = (s4) (imm); \
        *(cd->mcodeptr++) = imb.b[0]; \
        *(cd->mcodeptr++) = imb.b[1]; \
    } while (0)


#define x86_64_emit_imm32(imm) \
    do { \
        x86_64_imm_buf imb; \
        imb.i = (s4) (imm); \
        *(cd->mcodeptr++) = imb.b[0]; \
        *(cd->mcodeptr++) = imb.b[1]; \
        *(cd->mcodeptr++) = imb.b[2]; \
        *(cd->mcodeptr++) = imb.b[3]; \
    } while (0)


#define x86_64_emit_imm64(imm) \
    do { \
        x86_64_imm_buf imb; \
        imb.l = (s8) (imm); \
        *(cd->mcodeptr++) = imb.b[0]; \
        *(cd->mcodeptr++) = imb.b[1]; \
        *(cd->mcodeptr++) = imb.b[2]; \
        *(cd->mcodeptr++) = imb.b[3]; \
        *(cd->mcodeptr++) = imb.b[4]; \
        *(cd->mcodeptr++) = imb.b[5]; \
        *(cd->mcodeptr++) = imb.b[6]; \
        *(cd->mcodeptr++) = imb.b[7]; \
    } while (0)


/* additional functions and macros to generate code ***************************/

#define CALCOFFSETBYTES(var, reg, val) \
    if ((s4) (val) < -128 || (s4) (val) > 127) (var) += 4; \
    else if ((s4) (val) != 0) (var) += 1; \
    else if ((reg) == RBP || (reg) == RSP || (reg) == R12 || (reg) == R13) (var) += 1;


#define CALCIMMEDIATEBYTES(var, val) \
    if ((s4) (val) < -128 || (s4) (val) > 127) (var) += 4; \
    else (var) += 1;


/* gen_nullptr_check(objreg) */

#define gen_nullptr_check(objreg) \
	if (checknull) { \
        M_TEST(objreg); \
        M_BEQ(0); \
 	    codegen_add_nullpointerexception_ref(cd, cd->mcodeptr); \
	}


#define gen_bound_check \
    if (checkbounds) { \
        M_CMP_MEMBASE(s1, OFFSET(java_arrayheader, size), s2); \
        M_BAE(0); \
        codegen_add_arrayindexoutofboundsexception_ref(cd, cd->mcodeptr, s2); \
    }


#define gen_div_check(v) \
    if (checknull) { \
        if ((v)->flags & INMEMORY) { \
            M_CMP_IMM_MEMBASE(0, REG_SP, src->regoff * 8); \
        } else { \
            M_TEST(src->regoff); \
        } \
        M_BEQ(0); \
        codegen_add_arithmeticexception_ref(cd, cd->mcodeptr); \
    }


/* MCODECHECK(icnt) */

#define MCODECHECK(icnt) \
	if ((cd->mcodeptr + (icnt)) > (u1 *) cd->mcodeend) \
        cd->mcodeptr = (u1 *) codegen_increase(cd, cd->mcodeptr)


#define ALIGNCODENOP \
    if ((s4) (((ptrint) cd->mcodeptr) & 7)) { \
        M_NOP; \
    }


/* M_INTMOVE:
    generates an integer-move from register a to b.
    if a and b are the same int-register, no code will be generated.
*/ 

#define M_INTMOVE(reg,dreg) \
    do { \
        if ((reg) != (dreg)) { \
            M_MOV(reg, dreg); \
        } \
    } while (0)


/* M_FLTMOVE:
    generates a floating-point-move from register a to b.
    if a and b are the same float-register, no code will be generated
*/ 

#define M_FLTMOVE(reg,dreg) \
    do { \
        if ((reg) != (dreg)) { \
            M_FMOV(reg, dreg); \
        } \
    } while (0)


#define M_COPY(s,d)                     emit_copy(jd, iptr, (s), (d))

#define ICONST(r,c) \
    do { \
        if (iptr->val.i == 0) \
            M_CLR(d); \
        else \
            M_IMOV_IMM(iptr->val.i, d); \
    } while (0)
/*     do { \ */
/*        M_IMOV_IMM(iptr->val.i, d); \ */
/*     } while (0) */


#define LCONST(r,c) \
    do { \
        if (iptr->val.l == 0) \
            M_CLR(d); \
        else \
            M_MOV_IMM(iptr->val.l, d); \
    } while (0)


/* macros to create code ******************************************************/

#define M_MOV(a,b)              x86_64_mov_reg_reg(cd, (a), (b))
#define M_MOV_IMM(a,b)          x86_64_mov_imm_reg(cd, (u8) (a), (b))

#define M_FMOV(a,b)             x86_64_movq_reg_reg(cd, (a), (b))

#define M_IMOV_IMM(a,b)         x86_64_movl_imm_reg(cd, (u4) (a), (b))

#define M_ILD(a,b,disp)         x86_64_movl_membase_reg(cd, (b), (disp), (a))
#define M_LLD(a,b,disp)         x86_64_mov_membase_reg(cd, (b), (disp), (a))

#define M_ILD32(a,b,disp)       x86_64_movl_membase32_reg(cd, (b), (disp), (a))
#define M_LLD32(a,b,disp)       x86_64_mov_membase32_reg(cd, (b), (disp), (a))

#define M_IST(a,b,disp)         x86_64_movl_reg_membase(cd, (a), (b), (disp))
#define M_LST(a,b,disp)         x86_64_mov_reg_membase(cd, (a), (b), (disp))

#define M_IST_IMM(a,b,disp)     x86_64_movl_imm_membase(cd, (a), (b), (disp))
#define M_LST_IMM32(a,b,disp)   x86_64_mov_imm_membase(cd, (a), (b), (disp))

#define M_IST32(a,b,disp)       x86_64_movl_reg_membase32(cd, (a), (b), (disp))
#define M_LST32(a,b,disp)       x86_64_mov_reg_membase32(cd, (a), (b), (disp))

#define M_IST32_IMM(a,b,disp)   x86_64_movl_imm_membase32(cd, (a), (b), (disp))
#define M_LST32_IMM32(a,b,disp) x86_64_mov_imm_membase32(cd, (a), (b), (disp))

#define M_LADD(a,b)             x86_64_alu_reg_reg(cd, X86_64_ADD, (a), (b))
#define M_LADD_IMM(a,b)         x86_64_alu_imm_reg(cd, X86_64_ADD, (a), (b))
#define M_LSUB(a,b)             x86_64_alu_reg_reg(cd, X86_64_SUB, (a), (b))
#define M_LSUB_IMM(a,b)         x86_64_alu_imm_reg(cd, X86_64_SUB, (a), (b))

#define M_IINC_MEMBASE(a,b)     x86_64_incl_membase(cd, (a), (b))

#define M_IADD_MEMBASE(a,b,c)   x86_64_alul_reg_membase(cd, X86_64_ADD, (a), (b), (c))
#define M_IADC_MEMBASE(a,b,c)   x86_64_alul_reg_membase(cd, X86_64_ADC, (a), (b), (c))
#define M_ISUB_MEMBASE(a,b,c)   x86_64_alul_reg_membase(cd, X86_64_SUB, (a), (b), (c))
#define M_ISBB_MEMBASE(a,b,c)   x86_64_alul_reg_membase(cd, X86_64_SBB, (a), (b), (c))

#define M_ALD(a,b,c)            M_LLD(a,b,c)
#define M_AST(a,b,c)            M_LST(a,b,c)
#define M_AST_IMM32(a,b,c)      M_LST_IMM32(a,b,c)
#define M_AADD(a,b)             M_LADD(a,b)
#define M_AADD_IMM(a,b)         M_LADD_IMM(a,b)
#define M_ASUB_IMM(a,b)         M_LSUB_IMM(a,b)

#define M_LADD_IMM32(a,b)       x86_64_alu_imm32_reg(cd, X86_64_ADD, (a), (b))
#define M_AADD_IMM32(a,b)       M_LADD_IMM32(a,b)
#define M_LSUB_IMM32(a,b)       x86_64_alu_imm32_reg(cd, X86_64_SUB, (a), (b))

#define M_ILEA(a,b,c)           x86_64_leal_membase_reg(cd, (a), (b), (c))
#define M_LLEA(a,b,c)           x86_64_lea_membase_reg(cd, (a), (b), (c))
#define M_ALEA(a,b,c)           M_LLEA(a,b,c)

#define M_INEG(a)               x86_64_negl_reg(cd, (a))
#define M_LNEG(a)               x86_64_neg_reg(cd, (a))

#define M_INEG_MEMBASE(a,b)     x86_64_negl_membase(cd, (a), (b))
#define M_LNEG_MEMBASE(a,b)     x86_64_neg_membase(cd, (a), (b))

#define M_AND(a,b)              x86_64_alu_reg_reg(cd, X86_64_AND, (a), (b))
#define M_XOR(a,b)              x86_64_alu_reg_reg(cd, X86_64_XOR, (a), (b))

#define M_IAND(a,b)             x86_64_alul_reg_reg(cd, X86_64_AND, (a), (b))
#define M_IAND_IMM(a,b)         x86_64_alul_imm_reg(cd, X86_64_AND, (a), (b))
#define M_IXOR(a,b)             x86_64_alul_reg_reg(cd, X86_64_XOR, (a), (b))

#define M_TEST(a)               x86_64_test_reg_reg(cd, (a), (a))
#define M_ITEST(a)              x86_64_testl_reg_reg(cd, (a), (a))

#define M_CMP(a,b)              x86_64_alu_reg_reg(cd, X86_64_CMP, (a), (b))
#define M_CMP_IMM(a,b)          x86_64_alu_imm_reg(cd, X86_64_CMP, (a), (b))
#define M_CMP_IMM_MEMBASE(a,b,c) x86_64_alu_imm_membase(cd, X86_64_CMP, (a), (b), (c))
#define M_CMP_MEMBASE(a,b,c)    x86_64_alu_membase_reg(cd, X86_64_CMP, (a), (b), (c))

#define M_ICMP(a,b)             x86_64_alul_reg_reg(cd, X86_64_CMP, (a), (b))
#define M_ICMP_IMM(a,b)         x86_64_alul_imm_reg(cd, X86_64_CMP, (a), (b))
#define M_ICMP_IMM_MEMBASE(a,b,c) x86_64_alul_imm_membase(cd, X86_64_CMP, (a), (b), (c))

#define M_BEQ(disp)             x86_64_jcc(cd, X86_64_CC_E, (disp))
#define M_BNE(disp)             x86_64_jcc(cd, X86_64_CC_NE, (disp))
#define M_BLE(disp)             x86_64_jcc(cd, X86_64_CC_LE, (disp))
#define M_BAE(disp)             x86_64_jcc(cd, X86_64_CC_AE, (disp))
#define M_BA(disp)              x86_64_jcc(cd, X86_64_CC_A, (disp))

#define M_CMOVEQ(a,b)           x86_64_cmovcc_reg_reg(cd, X86_64_CC_E, (a), (b))
#define M_CMOVNE(a,b)           x86_64_cmovcc_reg_reg(cd, X86_64_CC_NE, (a), (b))
#define M_CMOVLT(a,b)           x86_64_cmovcc_reg_reg(cd, X86_64_CC_L, (a), (b))
#define M_CMOVLE(a,b)           x86_64_cmovcc_reg_reg(cd, X86_64_CC_LE, (a), (b))
#define M_CMOVGE(a,b)           x86_64_cmovcc_reg_reg(cd, X86_64_CC_GE, (a), (b))
#define M_CMOVGT(a,b)           x86_64_cmovcc_reg_reg(cd, X86_64_CC_G, (a), (b))

#define M_CMOVEQ_MEMBASE(a,b,c) x86_64_cmovcc_reg_membase(cd, X86_64_CC_E, (a), (b))
#define M_CMOVNE_MEMBASE(a,b,c) x86_64_cmovcc_reg_membase(cd, X86_64_CC_NE, (a), (b))
#define M_CMOVLT_MEMBASE(a,b,c) x86_64_cmovcc_reg_membase(cd, X86_64_CC_L, (a), (b))
#define M_CMOVLE_MEMBASE(a,b,c) x86_64_cmovcc_reg_membase(cd, X86_64_CC_LE, (a), (b))
#define M_CMOVGE_MEMBASE(a,b,c) x86_64_cmovcc_reg_membase(cd, X86_64_CC_GE, (a), (b))
#define M_CMOVGT_MEMBASE(a,b,c) x86_64_cmovcc_reg_membase(cd, X86_64_CC_G, (a), (b))

#define M_CMOVB(a,b)            x86_64_cmovcc_reg_reg(cd, X86_64_CC_B, (a), (b))
#define M_CMOVA(a,b)            x86_64_cmovcc_reg_reg(cd, X86_64_CC_A, (a), (b))
#define M_CMOVP(a,b)            x86_64_cmovcc_reg_reg(cd, X86_64_CC_P, (a), (b))

#define M_PUSH(a)               x86_64_push_reg(cd, (a))
#define M_PUSH_IMM(a)           x86_64_push_imm(cd, (a))
#define M_POP(a)                x86_64_pop_reg(cd, (a))

#define M_JMP(a)                x86_64_jmp_reg(cd, (a))
#define M_JMP_IMM(a)            x86_64_jmp_imm(cd, (a))
#define M_CALL(a)               x86_64_call_reg(cd, (a))
#define M_CALL_IMM(a)           x86_64_call_imm(cd, (a))
#define M_RET                   x86_64_ret(cd)

#define M_NOP                   x86_64_nop(cd)

#define M_CLR(a)                M_XOR(a,a)


#if 0
#define M_FLD(a,b,c)            x86_64_movlps_membase_reg(cd, (a), (b), (c))
#define M_DLD(a,b,c)            x86_64_movlpd_membase_reg(cd, (a), (b), (c))

#define M_FST(a,b,c)            x86_64_movlps_reg_membase(cd, (a), (b), (c))
#define M_DST(a,b,c)            x86_64_movlpd_reg_membase(cd, (a), (b), (c))
#endif

#define M_DLD(a,b,disp)         x86_64_movq_membase_reg(cd, (b), (disp), (a))
#define M_DST(a,b,disp)         x86_64_movq_reg_membase(cd, (a), (b), (disp))


/* system instructions ********************************************************/

#define M_RDTSC                 emit_rdtsc(cd)

#define PROFILE_CYCLE_START \
    do { \
        if (opt_prof) { \
            M_PUSH(RAX); \
            M_PUSH(RDX); \
            \
            M_MOV_IMM((ptrint) m, REG_ITMP3); \
            M_RDTSC; \
            M_ISUB_MEMBASE(RAX, REG_ITMP3, OFFSET(methodinfo, cycles)); \
            M_ISBB_MEMBASE(RDX, REG_ITMP3, OFFSET(methodinfo, cycles) + 4); \
            \
            M_POP(RDX); \
            M_POP(RAX); \
        } \
    } while (0)

#define PROFILE_CYCLE_STOP \
    do { \
        if (opt_prof) { \
            M_PUSH(RAX); \
            M_PUSH(RDX); \
            \
            M_MOV_IMM((ptrint) m, REG_ITMP3); \
            M_RDTSC; \
            M_IADD_MEMBASE(RAX, REG_ITMP3, OFFSET(methodinfo, cycles)); \
            M_IADC_MEMBASE(RDX, REG_ITMP3, OFFSET(methodinfo, cycles) + 4); \
            \
            M_POP(RDX); \
            M_POP(RAX); \
        } \
    } while (0)


/* function gen_resolvebranch **************************************************

    backpatches a branch instruction

    parameters: ip ... pointer to instruction after branch (void*)
                so ... offset of instruction after branch  (s8)
                to ... offset of branch target             (s8)

*******************************************************************************/

#define gen_resolvebranch(ip,so,to) \
    *((s4*) ((ip) - 4)) = (s4) ((to) - (so));

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
