/* src/vm/jit/x86_64/emitfuncs.h - emit function prototypes

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

   Changes:

   $Id: emitfuncs.h 4357 2006-01-22 23:33:38Z twisti $

*/


#ifndef _EMITFUNCS_H
#define _EMITFUNCS_H

#include "vm/types.h"


/* code generation prototypes */

void x86_64_emit_ialu(codegendata *cd, s4 alu_op, stackptr src, instruction *iptr);
void x86_64_emit_lalu(codegendata *cd, s4 alu_op, stackptr src, instruction *iptr);
void x86_64_emit_ialuconst(codegendata *cd, s4 alu_op, stackptr src, instruction *iptr);
void x86_64_emit_laluconst(codegendata *cd, s4 alu_op, stackptr src, instruction *iptr);
void x86_64_emit_ishift(codegendata *cd, s4 shift_op, stackptr src, instruction *iptr);
void x86_64_emit_lshift(codegendata *cd, s4 shift_op, stackptr src, instruction *iptr);
void x86_64_emit_ishiftconst(codegendata *cd, s4 shift_op, stackptr src, instruction *iptr);
void x86_64_emit_lshiftconst(codegendata *cd, s4 shift_op, stackptr src, instruction *iptr);
void x86_64_emit_ifcc(codegendata *cd, s4 if_op, stackptr src, instruction *iptr);
void x86_64_emit_if_lcc(codegendata *cd, s4 if_op, stackptr src, instruction *iptr);
void x86_64_emit_if_icmpcc(codegendata *cd, s4 if_op, stackptr src, instruction *iptr);
void x86_64_emit_if_lcmpcc(codegendata *cd, s4 if_op, stackptr src, instruction *iptr);


/* integer instructions */

void x86_64_mov_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_mov_imm_reg(codegendata *cd, s8 imm, s8 reg);
void x86_64_movl_imm_reg(codegendata *cd, s8 imm, s8 reg);
void x86_64_mov_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 reg);
void x86_64_mov_membase32_reg(codegendata *cd, s8 basereg, s8 disp, s8 reg);
void x86_64_movl_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 reg);
void x86_64_movl_membase32_reg(codegendata *cd, s8 basereg, s8 disp, s8 reg);
void x86_64_mov_reg_membase(codegendata *cd, s8 reg, s8 basereg, s8 disp);
void x86_64_mov_reg_membase32(codegendata *cd, s8 reg, s8 basereg, s8 disp);
void x86_64_movl_reg_membase(codegendata *cd, s8 reg, s8 basereg, s8 disp);
void x86_64_movl_reg_membase32(codegendata *cd, s8 reg, s8 basereg, s8 disp);
void x86_64_mov_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg);
void x86_64_movl_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg);
void x86_64_mov_reg_memindex(codegendata *cd, s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale);
void x86_64_movl_reg_memindex(codegendata *cd, s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale);
void x86_64_movw_reg_memindex(codegendata *cd, s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale);
void x86_64_movb_reg_memindex(codegendata *cd, s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale);
void x86_64_mov_imm_membase(codegendata *cd, s8 imm, s8 basereg, s8 disp);
void x86_64_mov_imm_membase32(codegendata *cd, s8 imm, s8 basereg, s8 disp);
void x86_64_movl_imm_membase(codegendata *cd, s8 imm, s8 basereg, s8 disp);
void x86_64_movl_imm_membase32(codegendata *cd, s8 imm, s8 basereg, s8 disp);
void x86_64_movsbq_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_movsbq_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg);
void x86_64_movswq_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_movswq_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg);
void x86_64_movslq_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_movslq_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg);
void x86_64_movzwq_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_movzwq_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg);
void x86_64_movswq_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg);
void x86_64_movsbq_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg);
void x86_64_movzwq_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg);
void x86_64_mov_imm_memindex(codegendata *cd, s4 imm, s4 disp, s4 basereg, s4 indexreg, s4 scale);
void x86_64_movl_imm_memindex(codegendata *cd, s4 imm, s4 disp, s4 basereg, s4 indexreg, s4 scale);
void x86_64_movw_imm_memindex(codegendata *cd, s4 imm, s4 disp, s4 basereg, s4 indexreg, s4 scale);
void x86_64_movb_imm_memindex(codegendata *cd, s4 imm, s4 disp, s4 basereg, s4 indexreg, s4 scale);
void x86_64_alu_reg_reg(codegendata *cd, s8 opc, s8 reg, s8 dreg);
void x86_64_alul_reg_reg(codegendata *cd, s8 opc, s8 reg, s8 dreg);
void x86_64_alu_reg_membase(codegendata *cd, s8 opc, s8 reg, s8 basereg, s8 disp);
void x86_64_alul_reg_membase(codegendata *cd, s8 opc, s8 reg, s8 basereg, s8 disp);
void x86_64_alu_membase_reg(codegendata *cd, s8 opc, s8 basereg, s8 disp, s8 reg);
void x86_64_alul_membase_reg(codegendata *cd, s8 opc, s8 basereg, s8 disp, s8 reg);
void x86_64_alu_imm_reg(codegendata *cd, s8 opc, s8 imm, s8 dreg);
void x86_64_alu_imm32_reg(codegendata *cd, s8 opc, s8 imm, s8 dreg);
void x86_64_alul_imm_reg(codegendata *cd, s8 opc, s8 imm, s8 dreg);
void x86_64_alu_imm_membase(codegendata *cd, s8 opc, s8 imm, s8 basereg, s8 disp);
void x86_64_alul_imm_membase(codegendata *cd, s8 opc, s8 imm, s8 basereg, s8 disp);
void x86_64_test_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_testl_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_test_imm_reg(codegendata *cd, s8 imm, s8 reg);
void x86_64_testw_imm_reg(codegendata *cd, s8 imm, s8 reg);
void x86_64_testb_imm_reg(codegendata *cd, s8 imm, s8 reg);
void x86_64_lea_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 reg);
void x86_64_leal_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 reg);
void x86_64_inc_reg(codegendata *cd, s8 reg);
void x86_64_incl_reg(codegendata *cd, s8 reg);
void x86_64_inc_membase(codegendata *cd, s8 basereg, s8 disp);
void x86_64_incl_membase(codegendata *cd, s8 basereg, s8 disp);
void x86_64_dec_reg(codegendata *cd, s8 reg);
void x86_64_decl_reg(codegendata *cd, s8 reg);
void x86_64_dec_membase(codegendata *cd, s8 basereg, s8 disp);
void x86_64_decl_membase(codegendata *cd, s8 basereg, s8 disp);
void x86_64_cltd(codegendata *cd);
void x86_64_cqto(codegendata *cd);
void x86_64_imul_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_imull_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_imul_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg);
void x86_64_imull_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg);
void x86_64_imul_imm_reg(codegendata *cd, s8 imm, s8 dreg);
void x86_64_imul_imm_reg_reg(codegendata *cd, s8 imm,s8 reg, s8 dreg);
void x86_64_imull_imm_reg_reg(codegendata *cd, s8 imm, s8 reg, s8 dreg);
void x86_64_imul_imm_membase_reg(codegendata *cd, s8 imm, s8 basereg, s8 disp, s8 dreg);
void x86_64_imull_imm_membase_reg(codegendata *cd, s8 imm, s8 basereg, s8 disp, s8 dreg);
void x86_64_idiv_reg(codegendata *cd, s8 reg);
void x86_64_idivl_reg(codegendata *cd, s8 reg);
void x86_64_ret(codegendata *cd);
void x86_64_shift_reg(codegendata *cd, s8 opc, s8 reg);
void x86_64_shiftl_reg(codegendata *cd, s8 opc, s8 reg);
void x86_64_shift_membase(codegendata *cd, s8 opc, s8 basereg, s8 disp);
void x86_64_shiftl_membase(codegendata *cd, s8 opc, s8 basereg, s8 disp);
void x86_64_shift_imm_reg(codegendata *cd, s8 opc, s8 imm, s8 dreg);
void x86_64_shiftl_imm_reg(codegendata *cd, s8 opc, s8 imm, s8 dreg);
void x86_64_shift_imm_membase(codegendata *cd, s8 opc, s8 imm, s8 basereg, s8 disp);
void x86_64_shiftl_imm_membase(codegendata *cd, s8 opc, s8 imm, s8 basereg, s8 disp);
void x86_64_jmp_imm(codegendata *cd, s8 imm);
void x86_64_jmp_reg(codegendata *cd, s8 reg);
void x86_64_jcc(codegendata *cd, s8 opc, s8 imm);
void x86_64_setcc_reg(codegendata *cd, s8 opc, s8 reg);
void x86_64_setcc_membase(codegendata *cd, s8 opc, s8 basereg, s8 disp);
void x86_64_cmovcc_reg_reg(codegendata *cd, s8 opc, s8 reg, s8 dreg);
void x86_64_cmovccl_reg_reg(codegendata *cd, s8 opc, s8 reg, s8 dreg);
void x86_64_neg_reg(codegendata *cd, s8 reg);
void x86_64_negl_reg(codegendata *cd, s8 reg);
void x86_64_neg_membase(codegendata *cd, s8 basereg, s8 disp);
void x86_64_negl_membase(codegendata *cd, s8 basereg, s8 disp);
void x86_64_push_reg(codegendata *cd, s8 reg);
void x86_64_push_imm(codegendata *cd, s8 imm);
void x86_64_pop_reg(codegendata *cd, s8 reg);
void x86_64_xchg_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_nop(codegendata *cd);
void x86_64_call_reg(codegendata *cd, s8 reg);
void x86_64_call_imm(codegendata *cd, s8 imm);
void x86_64_call_mem(codegendata *cd, s8 mem);


/* floating point instructions (SSE2) */

void x86_64_addsd_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_addss_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_cvtsi2ssq_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_cvtsi2ss_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_cvtsi2sdq_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_cvtsi2sd_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_cvtss2sd_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_cvtsd2ss_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_cvttss2siq_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_cvttss2si_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_cvttsd2siq_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_cvttsd2si_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_divss_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_divsd_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_movd_reg_freg(codegendata *cd, s8 reg, s8 freg);
void x86_64_movd_freg_reg(codegendata *cd, s8 freg, s8 reg);
void x86_64_movd_reg_membase(codegendata *cd, s8 reg, s8 basereg, s8 disp);
void x86_64_movd_reg_memindex(codegendata *cd, s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale);
void x86_64_movd_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg);
void x86_64_movdl_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg);
void x86_64_movd_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 dreg);
void x86_64_movq_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_movq_reg_membase(codegendata *cd, s8 reg, s8 basereg, s8 disp);
void x86_64_movq_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg);
void x86_64_movss_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_movsd_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_movss_reg_membase(codegendata *cd, s8 reg, s8 basereg, s8 disp);
void x86_64_movss_reg_membase32(codegendata *cd, s8 reg, s8 basereg, s8 disp);
void x86_64_movsd_reg_membase(codegendata *cd, s8 reg, s8 basereg, s8 disp);
void x86_64_movsd_reg_membase32(codegendata *cd, s8 reg, s8 basereg, s8 disp);
void x86_64_movss_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg);
void x86_64_movss_membase32_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg);
void x86_64_movlps_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg);
void x86_64_movsd_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg);
void x86_64_movsd_membase32_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg);
void x86_64_movlpd_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg);
void x86_64_movss_reg_memindex(codegendata *cd, s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale);
void x86_64_movsd_reg_memindex(codegendata *cd, s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale);
void x86_64_movss_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 dreg);
void x86_64_movsd_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 dreg);
void x86_64_mulss_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_mulsd_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_subss_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_subsd_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_ucomiss_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_ucomisd_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_xorps_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_xorps_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg);
void x86_64_xorpd_reg_reg(codegendata *cd, s8 reg, s8 dreg);
void x86_64_xorpd_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg);

#endif /* _EMITFUNCS_H */


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
