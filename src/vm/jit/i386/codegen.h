/* src/vm/jit/i386/codegen.h - code generation macros and definitions for i386

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

   $Id: codegen.h 4849 2006-04-26 14:09:15Z twisti $

*/


#ifndef _CODEGEN_H
#define _CODEGEN_H

#include "config.h"
#include "vm/types.h"

#include "vm/jit/jit.h"


#if defined(ENABLE_LSRA)
/* let LSRA allocate reserved registers (REG_ITMP[1|2|3]) */
# define LSRA_USES_REG_RES
#endif

/* some defines ***************************************************************/

#define PATCHER_CALL_SIZE    5          /* size in bytes of a patcher call    */


/* additional functions and macros to generate code ***************************/

#define CALCOFFSETBYTES(var, reg, val) \
    if ((s4) (val) < -128 || (s4) (val) > 127) (var) += 4; \
    else if ((s4) (val) != 0) (var) += 1; \
    else if ((reg) == EBP) (var) += 1;


#define CALCIMMEDIATEBYTES(var, val) \
    if ((s4) (val) < -128 || (s4) (val) > 127) (var) += 4; \
    else (var) += 1;


#define ALIGNCODENOP \
    do { \
        for (s1 = 0; s1 < (s4) (((ptrint) cd->mcodeptr) & 7); s1++) \
            M_NOP; \
    } while (0)


/* gen_nullptr_check(objreg) */

#define gen_nullptr_check(objreg) \
    if (checknull) { \
        M_TEST(objreg); \
        M_BEQ(0); \
 	    codegen_add_nullpointerexception_ref(cd); \
    }

#define gen_bound_check \
    if (checkbounds) { \
        M_CMP_MEMBASE(s1, OFFSET(java_arrayheader, size), s2); \
        M_BAE(0); \
        codegen_add_arrayindexoutofboundsexception_ref(cd, s2); \
    }

#define gen_div_check(v) \
    if (checknull) { \
        if ((v)->flags & INMEMORY) \
            M_CMP_IMM_MEMBASE(0, REG_SP, src->regoff * 4); \
        else \
            M_TEST(src->regoff); \
        M_BEQ(0); \
        codegen_add_arithmeticexception_ref(cd); \
    }


/* MCODECHECK(icnt) */

#define MCODECHECK(icnt) \
    do { \
        if ((cd->mcodeptr + (icnt)) > (u1 *) cd->mcodeend) \
            codegen_increase(cd); \
    } while (0)


/* M_INTMOVE:
     generates an integer-move from register a to b.
     if a and b are the same int-register, no code will be generated.
*/ 

#define M_INTMOVE(a,b) \
    do { \
        if ((a) != (b)) \
            M_MOV(a, b); \
    } while (0)

#define M_LNGMOVE(a,b) \
    do { \
        if (GET_HIGH_REG(a) == GET_LOW_REG(b)) { \
            assert((GET_LOW_REG(a) != GET_HIGH_REG(b))); \
            M_INTMOVE(GET_HIGH_REG(a), GET_HIGH_REG(b)); \
            M_INTMOVE(GET_LOW_REG(a), GET_LOW_REG(b)); \
        } else { \
            M_INTMOVE(GET_LOW_REG(a), GET_LOW_REG(b)); \
            M_INTMOVE(GET_HIGH_REG(a), GET_HIGH_REG(b)); \
        } \
    } while (0)


/* M_FLTMOVE:
    generates a floating-point-move from register a to b.
    if a and b are the same float-register, no code will be generated
*/

#define M_FLTMOVE(reg,dreg) \
    do { \
        log_text("M_FLTMOVE"); \
        assert(0); \
    } while (0)


#define M_COPY(s,d)                     emit_copy(jd, iptr, (s), (d))


/* macros to create code ******************************************************/

typedef enum {
    REG_AL = 0,
    REG_CL = 1,
    REG_DL = 2,
    REG_BL = 3,
    REG_AH = 4,
    REG_CH = 5,
    REG_DH = 6,
    REG_BH = 7,
    REG_NREGB
} I386_RegB_No;


/* opcodes for alu instructions */

typedef enum {
    ALU_ADD = 0,
    ALU_OR  = 1,
    ALU_ADC = 2,
    ALU_SBB = 3,
    ALU_AND = 4,
    ALU_SUB = 5,
    ALU_XOR = 6,
    ALU_CMP = 7,
    ALU_NALU
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

#define i386_address_byte(mod,reg,rm) \
    *(cd->mcodeptr++) = ((((mod) & 0x03) << 6) | (((reg) & 0x07) << 3) | (((rm) & 0x07)));


#define i386_emit_reg(reg,rm) \
    i386_address_byte(3,(reg),(rm));


#define i386_is_imm8(imm) \
    (((int)(imm) >= -128 && (int)(imm) <= 127))


#define i386_emit_imm8(imm) \
    *(cd->mcodeptr++) = (u1) ((imm) & 0xff);


#define i386_emit_imm16(imm) \
    do { \
        imm_union imb; \
        imb.i = (int) (imm); \
        *(cd->mcodeptr++) = imb.b[0]; \
        *(cd->mcodeptr++) = imb.b[1]; \
    } while (0)


#define i386_emit_imm32(imm) \
    do { \
        imm_union imb; \
        imb.i = (int) (imm); \
        *(cd->mcodeptr++) = imb.b[0]; \
        *(cd->mcodeptr++) = imb.b[1]; \
        *(cd->mcodeptr++) = imb.b[2]; \
        *(cd->mcodeptr++) = imb.b[3]; \
    } while (0)


#define i386_emit_mem(r,mem) \
    do { \
        i386_address_byte(0,(r),5); \
        i386_emit_imm32((mem)); \
    } while (0)


#define i386_emit_membase(basereg,disp,dreg) \
    do { \
        if ((basereg) == ESP) { \
            if ((disp) == 0) { \
                i386_address_byte(0, (dreg), ESP); \
                i386_address_byte(0, ESP, ESP); \
            } else if (i386_is_imm8((disp))) { \
                i386_address_byte(1, (dreg), ESP); \
                i386_address_byte(0, ESP, ESP); \
                i386_emit_imm8((disp)); \
            } else { \
                i386_address_byte(2, (dreg), ESP); \
                i386_address_byte(0, ESP, ESP); \
                i386_emit_imm32((disp)); \
            } \
            break; \
        } \
        \
        if ((disp) == 0 && (basereg) != EBP) { \
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


#define i386_emit_membase32(basereg,disp,dreg) \
    do { \
        if ((basereg) == ESP) { \
            i386_address_byte(2, (dreg), ESP); \
            i386_address_byte(0, ESP, ESP); \
            i386_emit_imm32((disp)); \
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
        } else if ((disp) == 0 && (basereg) != EBP) { \
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


/* macros to create code ******************************************************/

#define M_ILD(a,b,disp)         i386_mov_membase_reg(cd, (b), (disp), (a))
#define M_ALD(a,b,disp)         M_ILD(a,b,disp)

#define M_ILD32(a,b,disp)       i386_mov_membase32_reg(cd, (b), (disp), (a))

#define M_LLD(a,b,disp) \
    do { \
        M_ILD(GET_LOW_REG(a),b,disp); \
        M_ILD(GET_HIGH_REG(a),b,disp + 4); \
    } while (0)

#define M_LLD32(a,b,disp) \
    do { \
        M_ILD32(GET_LOW_REG(a),b,disp); \
        M_ILD32(GET_HIGH_REG(a),b,disp + 4); \
    } while (0)

#define M_IST(a,b,disp)         i386_mov_reg_membase(cd, (a), (b), (disp))
#define M_IST_IMM(a,b,disp)     i386_mov_imm_membase(cd, (u4) (a), (b), (disp))
#define M_AST(a,b,disp)         M_IST(a,b,disp)
#define M_AST_IMM(a,b,disp)     M_IST_IMM(a,b,disp)

#define M_IST32(a,b,disp)       i386_mov_reg_membase32(cd, (a), (b), (disp))
#define M_IST32_IMM(a,b,disp)   i386_mov_imm_membase32(cd, (u4) (a), (b), (disp))

#define M_LST(a,b,disp) \
    do { \
        M_IST(GET_LOW_REG(a),b,disp); \
        M_IST(GET_HIGH_REG(a),b,disp + 4); \
    } while (0)

#define M_LST32(a,b,disp) \
    do { \
        M_IST32(GET_LOW_REG(a),b,disp); \
        M_IST32(GET_HIGH_REG(a),b,disp + 4); \
    } while (0)

#define M_LST_IMM(a,b,disp) \
    do { \
        M_IST_IMM(a,b,disp); \
        M_IST_IMM(a >> 32,b,disp + 4); \
    } while (0)

#define M_LST32_IMM(a,b,disp) \
    do { \
        M_IST32_IMM(a,b,disp); \
        M_IST32_IMM(a >> 32,b,disp + 4); \
    } while (0)

#define M_IADD_IMM(a,b)         i386_alu_imm_reg(cd, ALU_ADD, (a), (b))
#define M_IADD_IMM32(a,b)       i386_alu_imm32_reg(cd, ALU_ADD, (a), (b))
#define M_ISUB_IMM(a,b)         i386_alu_imm_reg(cd, ALU_SUB, (a), (b))

#define M_IADD_IMM_MEMBASE(a,b,c) i386_alu_imm_membase(cd, ALU_ADD, (a), (b), (c))

#define M_AADD_IMM(a,b)         M_IADD_IMM(a,b)
#define M_AADD_IMM32(a,b)       M_IADD_IMM32(a,b)
#define M_ASUB_IMM(a,b)         M_ISUB_IMM(a,b)

#define M_OR_MEMBASE(a,b,c)     i386_alu_membase_reg(cd, ALU_OR, (a), (b), (c))
#define M_XOR(a,b)              i386_alu_reg_reg(cd, ALU_XOR, (a), (b))
#define M_CLR(a)                M_XOR(a,a)

#define M_PUSH(a)               i386_push_reg(cd, (a))
#define M_PUSH_IMM(a)           i386_push_imm(cd, (s4) (a))
#define M_POP(a)                i386_pop_reg(cd, (a))

#define M_MOV(a,b)              i386_mov_reg_reg(cd, (a), (b))
#define M_MOV_IMM(a,b)          i386_mov_imm_reg(cd, (u4) (a), (b))

#define M_TEST(a)               i386_test_reg_reg(cd, (a), (a))

#define M_CMP(a,b)              i386_alu_reg_reg(cd, ALU_CMP, (a), (b))
#define M_CMP_MEMBASE(a,b,c)    i386_alu_membase_reg(cd, ALU_CMP, (a), (b), (c))

#define M_CMP_IMM_MEMBASE(a,b,c) i386_alu_imm_membase(cd, ALU_CMP, (a), (b), (c))

#define M_CALL(a)               i386_call_reg(cd, (a))
#define M_CALL_IMM(a)           i386_call_imm(cd, (a))
#define M_RET                   i386_ret(cd)

#define M_BEQ(a)                i386_jcc(cd, I386_CC_E, (a))
#define M_BNE(a)                i386_jcc(cd, I386_CC_NE, (a))
#define M_BLT(a)                i386_jcc(cd, I386_CC_L, (a))
#define M_BLE(a)                i386_jcc(cd, I386_CC_LE, (a))
#define M_BGE(a)                i386_jcc(cd, I386_CC_GE, (a))
#define M_BGT(a)                i386_jcc(cd, I386_CC_G, (a))

#define M_BBE(a)                i386_jcc(cd, I386_CC_BE, (a))
#define M_BAE(a)                i386_jcc(cd, I386_CC_AE, (a))

#define M_JMP(a)                i386_jmp_reg(cd, (a))
#define M_JMP_IMM(a)            i386_jmp_imm(cd, (a))

#define M_NOP                   i386_nop(cd)


#define M_FLD(a,b,disp)         i386_flds_membase(cd, (b), (disp))
#define M_DLD(a,b,disp)         i386_fldl_membase(cd, (b), (disp))

#define M_FLD32(a,b,disp)       i386_flds_membase32(cd, (b), (disp))
#define M_DLD32(a,b,disp)       i386_fldl_membase32(cd, (b), (disp))

#define M_FST(a,b,disp)         i386_fstps_membase(cd, (b), (disp))
#define M_DST(a,b,disp)         i386_fstpl_membase(cd, (b), (disp))


/* function gen_resolvebranch **************************************************

    backpatches a branch instruction

    parameters: ip ... pointer to instruction after branch (void*)
                so ... offset of instruction after branch  (s4)
                to ... offset of branch target             (s4)

*******************************************************************************/

#define gen_resolvebranch(ip,so,to) \
    *((void **) ((ip) - 4)) = (void **) ((to) - (so));


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
