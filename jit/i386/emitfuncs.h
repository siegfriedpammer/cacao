/* jit/i386/emitfuncs.h - emit function prototypes

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

   Authors: Christian Thalinger

   $Id: emitfuncs.h 1132 2004-06-05 16:29:07Z twisti $

*/


#ifndef _EMITFUNCS_H
#define _EMITFUNCS_H


extern u1 *mcodeptr;


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
void i386_mov_mem_reg(s4 mem, s4 dreg);
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
void i386_dec_mem(s4 mem);
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
void i386_xadd_reg_mem(s4 reg, s4 mem);
void i386_neg_reg(s4 reg);
void i386_neg_membase(s4 basereg, s4 disp);
void i386_push_imm(s4 imm);
void i386_pop_reg(s4 reg);
void i386_push_reg(s4 reg);
void i386_nop();
void i386_lock();
void i386_call_reg(s4 reg);
void i386_call_imm(s4 imm);
void i386_call_mem(s4 mem);


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
