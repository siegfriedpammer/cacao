/* src/vm/jit/i386/emitfuncs.h - emit function prototypes

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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

   Authors: Christian Thalinger

   Changes:

   $Id: emitfuncs.h 2338 2005-04-22 13:30:40Z twisti $

*/


#ifndef _EMITFUNCS_H
#define _EMITFUNCS_H

#include "vm/jit/i386/types.h"


/* code generation prototypes *************************************************/

void i386_emit_ialu(codegendata *cd, s4 alu_op, stackptr src, instruction *iptr);
void i386_emit_ialuconst(codegendata *cd, s4 alu_op, stackptr src, instruction *iptr);
void i386_emit_lalu(codegendata *cd, s4 alu_op, stackptr src, instruction *iptr);
void i386_emit_laluconst(codegendata *cd, s4 alu_op, stackptr src, instruction *iptr);
void i386_emit_ishift(codegendata *cd, s4 shift_op, stackptr src, instruction *iptr);
void i386_emit_ishiftconst(codegendata *cd, s4 shift_op, stackptr src, instruction *iptr);
void i386_emit_ifcc_iconst(codegendata *cd, s4 if_op, stackptr src, instruction *iptr);


/* integer instructions */

void i386_mov_reg_reg(codegendata *cd, s4 reg, s4 dreg);
void i386_mov_imm_reg(codegendata *cd, s4 imm, s4 dreg);
void i386_movb_imm_reg(codegendata *cd, s4 imm, s4 dreg);
void i386_mov_membase_reg(codegendata *cd, s4 basereg, s4 disp, s4 reg);
void i386_mov_membase32_reg(codegendata *cd, s4 basereg, s4 disp, s4 reg);
void i386_mov_reg_membase(codegendata *cd, s4 reg, s4 basereg, s4 disp);
void i386_mov_reg_membase32(codegendata *cd, s4 reg, s4 basereg, s4 disp);
void i386_mov_memindex_reg(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg);
void i386_mov_reg_memindex(codegendata *cd, s4 reg, s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_movw_reg_memindex(codegendata *cd, s4 reg, s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_movb_reg_memindex(codegendata *cd, s4 reg, s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_mov_reg_mem(codegendata *cd, s4 reg, s4 mem);
void i386_mov_mem_reg(codegendata *cd, s4 mem, s4 dreg);
void i386_mov_imm_mem(codegendata *cd, s4 imm, s4 mem);
void i386_mov_imm_membase(codegendata *cd, s4 imm, s4 basereg, s4 disp);
void i386_mov_imm_membase32(codegendata *cd, s4 imm, s4 basereg, s4 disp);
void i386_movb_imm_membase(codegendata *cd, s4 imm, s4 basereg, s4 disp);
void i386_movsbl_memindex_reg(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg);
void i386_movswl_memindex_reg(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg);
void i386_movzwl_memindex_reg(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg);
void i386_mov_imm_memindex(codegendata *cd, s4 imm, s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_movw_imm_memindex(codegendata *cd, s4 imm, s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_movb_imm_memindex(codegendata *cd, s4 imm, s4 disp, s4 basereg, s4 indexreg, s4 scale);

void i386_alu_reg_reg(codegendata *cd, s4 opc, s4 reg, s4 dreg);
void i386_alu_reg_membase(codegendata *cd, s4 opc, s4 reg, s4 basereg, s4 disp);
void i386_alu_membase_reg(codegendata *cd, s4 opc, s4 basereg, s4 disp, s4 reg);
void i386_alu_imm_reg(codegendata *cd, s4 opc, s4 imm, s4 reg);
void i386_alu_imm32_reg(codegendata *cd, s4 opc, s4 imm, s4 reg);
void i386_alu_imm_membase(codegendata *cd, s4 opc, s4 imm, s4 basereg, s4 disp);
void i386_test_reg_reg(codegendata *cd, s4 reg, s4 dreg);
void i386_test_imm_reg(codegendata *cd, s4 imm, s4 dreg);
void i386_dec_mem(codegendata *cd, s4 mem);
void i386_cltd(codegendata *cd);
void i386_imul_reg_reg(codegendata *cd, s4 reg, s4 dreg);
void i386_imul_membase_reg(codegendata *cd, s4 basereg, s4 disp, s4 dreg);
void i386_imul_imm_reg(codegendata *cd, s4 imm, s4 reg);
void i386_imul_imm_reg_reg(codegendata *cd, s4 imm, s4 reg, s4 dreg);
void i386_imul_imm_membase_reg(codegendata *cd, s4 imm, s4 basereg, s4 disp, s4 dreg);
void i386_mul_membase(codegendata *cd, s4 basereg, s4 disp);
void i386_idiv_reg(codegendata *cd, s4 reg);
void i386_ret(codegendata *cd);
void i386_shift_reg(codegendata *cd, s4 opc, s4 reg);
void i386_shift_membase(codegendata *cd, s4 opc, s4 basereg, s4 disp);
void i386_shift_imm_reg(codegendata *cd, s4 opc, s4 imm, s4 reg);
void i386_shift_imm_membase(codegendata *cd, s4 opc, s4 imm, s4 basereg, s4 disp);
void i386_shld_reg_reg(codegendata *cd, s4 reg, s4 dreg);
void i386_shld_imm_reg_reg(codegendata *cd, s4 imm, s4 reg, s4 dreg);
void i386_shld_reg_membase(codegendata *cd, s4 reg, s4 basereg, s4 disp);
void i386_shrd_reg_reg(codegendata *cd, s4 reg, s4 dreg);
void i386_shrd_imm_reg_reg(codegendata *cd, s4 imm, s4 reg, s4 dreg);
void i386_shrd_reg_membase(codegendata *cd, s4 reg, s4 basereg, s4 disp);
void i386_jmp_imm(codegendata *cd, s4 imm);
void i386_jmp_reg(codegendata *cd, s4 reg);
void i386_jcc(codegendata *cd, s4 opc, s4 imm);
void i386_setcc_reg(codegendata *cd, s4 opc, s4 reg);
void i386_setcc_membase(codegendata *cd, s4 opc, s4 basereg, s4 disp);
void i386_xadd_reg_mem(codegendata *cd, s4 reg, s4 mem);
void i386_neg_reg(codegendata *cd, s4 reg);
void i386_neg_membase(codegendata *cd, s4 basereg, s4 disp);
void i386_push_imm(codegendata *cd, s4 imm);
void i386_pop_reg(codegendata *cd, s4 reg);
void i386_push_reg(codegendata *cd, s4 reg);
void i386_nop(codegendata *cd);
void i386_lock(codegendata *cd);
void i386_call_reg(codegendata *cd, s4 reg);
void i386_call_imm(codegendata *cd, s4 imm);
void i386_call_mem(codegendata *cd, s4 mem);


/* floating point instructions */

void i386_fld1(codegendata *cd);
void i386_fldz(codegendata *cd);
void i386_fld_reg(codegendata *cd, s4 reg);
void i386_flds_membase(codegendata *cd, s4 basereg, s4 disp);
void i386_flds_membase32(codegendata *cd, s4 basereg, s4 disp);
void i386_fldl_membase(codegendata *cd, s4 basereg, s4 disp);
void i386_fldl_membase32(codegendata *cd, s4 basereg, s4 disp);
void i386_fldt_membase(codegendata *cd, s4 basereg, s4 disp);
void i386_flds_memindex(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_fldl_memindex(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_flds_mem(codegendata *cd, s4 mem);
void i386_fldl_mem(codegendata *cd, s4 mem);
void i386_fildl_membase(codegendata *cd, s4 basereg, s4 disp);
void i386_fildll_membase(codegendata *cd, s4 basereg, s4 disp);
void i386_fst_reg(codegendata *cd, s4 reg);
void i386_fsts_membase(codegendata *cd, s4 basereg, s4 disp);
void i386_fstl_membase(codegendata *cd, s4 basereg, s4 disp);
void i386_fsts_memindex(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_fstl_memindex(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_fstp_reg(codegendata *cd, s4 reg);
void i386_fstps_membase(codegendata *cd, s4 basereg, s4 disp);
void i386_fstps_membase32(codegendata *cd, s4 basereg, s4 disp);
void i386_fstpl_membase(codegendata *cd, s4 basereg, s4 disp);
void i386_fstpl_membase32(codegendata *cd, s4 basereg, s4 disp);
void i386_fstpt_membase(codegendata *cd, s4 basereg, s4 disp);
void i386_fstps_memindex(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_fstpl_memindex(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_fstps_mem(codegendata *cd, s4 mem);
void i386_fstpl_mem(codegendata *cd, s4 mem);
void i386_fistl_membase(codegendata *cd, s4 basereg, s4 disp);
void i386_fistpl_membase(codegendata *cd, s4 basereg, s4 disp);
void i386_fistpll_membase(codegendata *cd, s4 basereg, s4 disp);
void i386_fchs(codegendata *cd);
void i386_faddp(codegendata *cd);
void i386_fadd_reg_st(codegendata *cd, s4 reg);
void i386_fadd_st_reg(codegendata *cd, s4 reg);
void i386_faddp_st_reg(codegendata *cd, s4 reg);
void i386_fadds_membase(codegendata *cd, s4 basereg, s4 disp);
void i386_faddl_membase(codegendata *cd, s4 basereg, s4 disp);
void i386_fsub_reg_st(codegendata *cd, s4 reg);
void i386_fsub_st_reg(codegendata *cd, s4 reg);
void i386_fsubp_st_reg(codegendata *cd, s4 reg);
void i386_fsubp(codegendata *cd);
void i386_fsubs_membase(codegendata *cd, s4 basereg, s4 disp);
void i386_fsubl_membase(codegendata *cd, s4 basereg, s4 disp);
void i386_fmul_reg_st(codegendata *cd, s4 reg);
void i386_fmul_st_reg(codegendata *cd, s4 reg);
void i386_fmulp(codegendata *cd);
void i386_fmulp_st_reg(codegendata *cd, s4 reg);
void i386_fmuls_membase(codegendata *cd, s4 basereg, s4 disp);
void i386_fmull_membase(codegendata *cd, s4 basereg, s4 disp);
void i386_fdiv_reg_st(codegendata *cd, s4 reg);
void i386_fdiv_st_reg(codegendata *cd, s4 reg);
void i386_fdivp(codegendata *cd);
void i386_fdivp_st_reg(codegendata *cd, s4 reg);
void i386_fxch(codegendata *cd);
void i386_fxch_reg(codegendata *cd, s4 reg);
void i386_fprem(codegendata *cd);
void i386_fprem1(codegendata *cd);
void i386_fucom(codegendata *cd);
void i386_fucom_reg(codegendata *cd, s4 reg);
void i386_fucomp_reg(codegendata *cd, s4 reg);
void i386_fucompp(codegendata *cd);
void i386_fnstsw(codegendata *cd);
void i386_sahf(codegendata *cd);
void i386_finit(codegendata *cd);
void i386_fldcw_mem(codegendata *cd, s4 mem);
void i386_fldcw_membase(codegendata *cd, s4 basereg, s4 disp);
void i386_wait(codegendata *cd);
void i386_ffree_reg(codegendata *cd, s4 reg);
void i386_fdecstp(codegendata *cd);
void i386_fincstp(codegendata *cd);

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
