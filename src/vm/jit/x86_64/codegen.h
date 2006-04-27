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

   $Id: codegen.h 4853 2006-04-27 12:33:20Z twisti $

*/


#ifndef _CODEGEN_H
#define _CODEGEN_H

#include "config.h"

#include <ucontext.h>

#include "vm/types.h"

#include "vm/jit/jit.h"


/* some defines ***************************************************************/

#define PATCHER_CALL_SIZE    5          /* size in bytes of a patcher call    */


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
        if ((v)->flags & INMEMORY) { \
            M_CMP_IMM_MEMBASE(0, REG_SP, src->regoff * 8); \
        } else { \
            M_TEST(src->regoff); \
        } \
        M_BEQ(0); \
        codegen_add_arithmeticexception_ref(cd); \
    }


/* MCODECHECK(icnt) */

#define MCODECHECK(icnt) \
    do { \
        if ((cd->mcodeptr + (icnt)) > cd->mcodeend) \
            codegen_increase(cd); \
    } while (0)


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

#define M_MOV(a,b)              emit_mov_reg_reg(cd, (a), (b))
#define M_MOV_IMM(a,b)          emit_mov_imm_reg(cd, (u8) (a), (b))

#define M_FMOV(a,b)             emit_movq_reg_reg(cd, (a), (b))

#define M_IMOV_IMM(a,b)         emit_movl_imm_reg(cd, (u4) (a), (b))

#define M_ILD(a,b,disp)         emit_movl_membase_reg(cd, (b), (disp), (a))
#define M_LLD(a,b,disp)         emit_mov_membase_reg(cd, (b), (disp), (a))

#define M_ILD32(a,b,disp)       emit_movl_membase32_reg(cd, (b), (disp), (a))
#define M_LLD32(a,b,disp)       emit_mov_membase32_reg(cd, (b), (disp), (a))

#define M_IST(a,b,disp)         emit_movl_reg_membase(cd, (a), (b), (disp))
#define M_LST(a,b,disp)         emit_mov_reg_membase(cd, (a), (b), (disp))

#define M_IST_IMM(a,b,disp)     emit_movl_imm_membase(cd, (a), (b), (disp))
#define M_LST_IMM32(a,b,disp)   emit_mov_imm_membase(cd, (a), (b), (disp))

#define M_IST32(a,b,disp)       emit_movl_reg_membase32(cd, (a), (b), (disp))
#define M_LST32(a,b,disp)       emit_mov_reg_membase32(cd, (a), (b), (disp))

#define M_IST32_IMM(a,b,disp)   emit_movl_imm_membase32(cd, (a), (b), (disp))
#define M_LST32_IMM32(a,b,disp) emit_mov_imm_membase32(cd, (a), (b), (disp))

#define M_IADD(a,b)             emit_alul_reg_reg(cd, ALU_ADD, (a), (b))
#define M_IADD_IMM(a,b)         emit_alul_reg_reg(cd, ALU_ADD, (a), (b))

#define M_LADD(a,b)             emit_alu_reg_reg(cd, ALU_ADD, (a), (b))
#define M_LADD_IMM(a,b)         emit_alu_imm_reg(cd, ALU_ADD, (a), (b))
#define M_LSUB(a,b)             emit_alu_reg_reg(cd, ALU_SUB, (a), (b))
#define M_LSUB_IMM(a,b)         emit_alu_imm_reg(cd, ALU_SUB, (a), (b))

#define M_IINC_MEMBASE(a,b)     emit_incl_membase(cd, (a), (b))

#define M_IADD_MEMBASE(a,b,c)   emit_alul_reg_membase(cd, ALU_ADD, (a), (b), (c))
#define M_IADC_MEMBASE(a,b,c)   emit_alul_reg_membase(cd, ALU_ADC, (a), (b), (c))
#define M_ISUB_MEMBASE(a,b,c)   emit_alul_reg_membase(cd, ALU_SUB, (a), (b), (c))
#define M_ISBB_MEMBASE(a,b,c)   emit_alul_reg_membase(cd, ALU_SBB, (a), (b), (c))

#define M_ALD(a,b,disp)         M_LLD(a,b,disp)
#define M_ALD32(a,b,disp)       M_LLD32(a,b,disp)

#define M_AST(a,b,c)            M_LST(a,b,c)
#define M_AST_IMM32(a,b,c)      M_LST_IMM32(a,b,c)

#define M_AADD(a,b)             M_LADD(a,b)
#define M_AADD_IMM(a,b)         M_LADD_IMM(a,b)
#define M_ASUB_IMM(a,b)         M_LSUB_IMM(a,b)

#define M_LADD_IMM32(a,b)       emit_alu_imm32_reg(cd, ALU_ADD, (a), (b))
#define M_AADD_IMM32(a,b)       M_LADD_IMM32(a,b)
#define M_LSUB_IMM32(a,b)       emit_alu_imm32_reg(cd, ALU_SUB, (a), (b))

#define M_ILEA(a,b,c)           emit_leal_membase_reg(cd, (a), (b), (c))
#define M_LLEA(a,b,c)           emit_lea_membase_reg(cd, (a), (b), (c))
#define M_ALEA(a,b,c)           M_LLEA(a,b,c)

#define M_INEG(a)               emit_negl_reg(cd, (a))
#define M_LNEG(a)               emit_neg_reg(cd, (a))

#define M_INEG_MEMBASE(a,b)     emit_negl_membase(cd, (a), (b))
#define M_LNEG_MEMBASE(a,b)     emit_neg_membase(cd, (a), (b))

#define M_AND(a,b)              emit_alu_reg_reg(cd, ALU_AND, (a), (b))
#define M_XOR(a,b)              emit_alu_reg_reg(cd, ALU_XOR, (a), (b))

#define M_IAND(a,b)             emit_alul_reg_reg(cd, ALU_AND, (a), (b))
#define M_IAND_IMM(a,b)         emit_alul_imm_reg(cd, ALU_AND, (a), (b))
#define M_IXOR(a,b)             emit_alul_reg_reg(cd, ALU_XOR, (a), (b))

#define M_BSEXT(a,b)            emit_movsbq_reg_reg(cd, (a), (b))
#define M_SSEXT(a,b)            emit_movswq_reg_reg(cd, (a), (b))
#define M_ISEXT(a,b)            emit_movslq_reg_reg(cd, (a), (b))

#define M_CZEXT(a,b)            emit_movzwq_reg_reg(cd, (a), (b))

#define M_BSEXT_MEMBASE(a,disp,b) emit_movsbq_membase_reg(cd, (a), (disp), (b))
#define M_SSEXT_MEMBASE(a,disp,b) emit_movswq_membase_reg(cd, (a), (disp), (b))
#define M_ISEXT_MEMBASE(a,disp,b) emit_movslq_membase_reg(cd, (a), (disp), (b))

#define M_CZEXT_MEMBASE(a,disp,b) emit_movzwq_membase_reg(cd, (a), (disp), (b))

#define M_TEST(a)               emit_test_reg_reg(cd, (a), (a))
#define M_ITEST(a)              emit_testl_reg_reg(cd, (a), (a))

#define M_CMP(a,b)              emit_alu_reg_reg(cd, ALU_CMP, (a), (b))
#define M_CMP_IMM(a,b)          emit_alu_imm_reg(cd, ALU_CMP, (a), (b))
#define M_CMP_IMM_MEMBASE(a,b,c) emit_alu_imm_membase(cd, ALU_CMP, (a), (b), (c))
#define M_CMP_MEMBASE(a,b,c)    emit_alu_membase_reg(cd, ALU_CMP, (a), (b), (c))

#define M_ICMP(a,b)             emit_alul_reg_reg(cd, ALU_CMP, (a), (b))
#define M_ICMP_IMM(a,b)         emit_alul_imm_reg(cd, ALU_CMP, (a), (b))
#define M_ICMP_IMM_MEMBASE(a,b,c) emit_alul_imm_membase(cd, ALU_CMP, (a), (b), (c))

#define M_BEQ(disp)             emit_jcc(cd, CC_E, (disp))
#define M_BNE(disp)             emit_jcc(cd, CC_NE, (disp))
#define M_BLT(disp)             emit_jcc(cd, CC_L, (disp))
#define M_BLE(disp)             emit_jcc(cd, CC_LE, (disp))
#define M_BAE(disp)             emit_jcc(cd, CC_AE, (disp))
#define M_BA(disp)              emit_jcc(cd, CC_A, (disp))

#define M_CMOVEQ(a,b)           emit_cmovcc_reg_reg(cd, CC_E, (a), (b))
#define M_CMOVNE(a,b)           emit_cmovcc_reg_reg(cd, CC_NE, (a), (b))
#define M_CMOVLT(a,b)           emit_cmovcc_reg_reg(cd, CC_L, (a), (b))
#define M_CMOVLE(a,b)           emit_cmovcc_reg_reg(cd, CC_LE, (a), (b))
#define M_CMOVGE(a,b)           emit_cmovcc_reg_reg(cd, CC_GE, (a), (b))
#define M_CMOVGT(a,b)           emit_cmovcc_reg_reg(cd, CC_G, (a), (b))

#define M_CMOVEQ_MEMBASE(a,b,c) emit_cmovcc_reg_membase(cd, CC_E, (a), (b))
#define M_CMOVNE_MEMBASE(a,b,c) emit_cmovcc_reg_membase(cd, CC_NE, (a), (b))
#define M_CMOVLT_MEMBASE(a,b,c) emit_cmovcc_reg_membase(cd, CC_L, (a), (b))
#define M_CMOVLE_MEMBASE(a,b,c) emit_cmovcc_reg_membase(cd, CC_LE, (a), (b))
#define M_CMOVGE_MEMBASE(a,b,c) emit_cmovcc_reg_membase(cd, CC_GE, (a), (b))
#define M_CMOVGT_MEMBASE(a,b,c) emit_cmovcc_reg_membase(cd, CC_G, (a), (b))

#define M_CMOVB(a,b)            emit_cmovcc_reg_reg(cd, CC_B, (a), (b))
#define M_CMOVA(a,b)            emit_cmovcc_reg_reg(cd, CC_A, (a), (b))
#define M_CMOVP(a,b)            emit_cmovcc_reg_reg(cd, CC_P, (a), (b))

#define M_PUSH(a)               emit_push_reg(cd, (a))
#define M_PUSH_IMM(a)           emit_push_imm(cd, (a))
#define M_POP(a)                emit_pop_reg(cd, (a))

#define M_JMP(a)                emit_jmp_reg(cd, (a))
#define M_JMP_IMM(a)            emit_jmp_imm(cd, (a))
#define M_CALL(a)               emit_call_reg(cd, (a))
#define M_CALL_IMM(a)           emit_call_imm(cd, (a))
#define M_RET                   emit_ret(cd)

#define M_NOP                   emit_nop(cd)

#define M_CLR(a)                M_XOR(a,a)


#if 0
#define M_FLD(a,b,c)            emit_movlps_membase_reg(cd, (a), (b), (c))
#define M_DLD(a,b,c)            emit_movlpd_membase_reg(cd, (a), (b), (c))

#define M_FST(a,b,c)            emit_movlps_reg_membase(cd, (a), (b), (c))
#define M_DST(a,b,c)            emit_movlpd_reg_membase(cd, (a), (b), (c))
#endif

#define M_DLD(a,b,disp)         emit_movq_membase_reg(cd, (b), (disp), (a))
#define M_DST(a,b,disp)         emit_movq_reg_membase(cd, (a), (b), (disp))


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
