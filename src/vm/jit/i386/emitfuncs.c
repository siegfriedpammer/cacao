/* jit/i386/emitfuncs.c - i386 code emitter functions

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   Institut f. Computersprachen, TU Wien
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser, M. Probst,
   S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich,
   J. Wenninger

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

   $Id: emitfuncs.c 1132 2004-06-05 16:29:07Z twisti $

*/


#include "jit/jit.h"
#include "jit/i386/emitfuncs.h"
#include "jit/i386/codegen.h"
#include "jit/i386/types.h"


void i386_emit_ialu(s4 alu_op, stackptr src, instruction *iptr)
{
	if (iptr->dst->flags & INMEMORY) {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (src->regoff == iptr->dst->regoff) {
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_alu_reg_membase(alu_op, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

			} else if (src->prev->regoff == iptr->dst->regoff) {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_reg_membase(alu_op, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

			} else {
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_alu_membase_reg(alu_op, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
			}

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			if (src->regoff == iptr->dst->regoff) {
				i386_alu_reg_membase(alu_op, src->prev->regoff, REG_SP, iptr->dst->regoff * 8);

			} else {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_reg_reg(alu_op, src->prev->regoff, REG_ITMP1);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
			}

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (src->prev->regoff == iptr->dst->regoff) {
				i386_alu_reg_membase(alu_op, src->regoff, REG_SP, iptr->dst->regoff * 8);
						
			} else {
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_alu_reg_reg(alu_op, src->regoff, REG_ITMP1);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
			}

		} else {
			i386_mov_reg_membase(src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
			i386_alu_reg_membase(alu_op, src->regoff, REG_SP, iptr->dst->regoff * 8);
		}

	} else {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
			i386_alu_membase_reg(alu_op, REG_SP, src->regoff * 8, iptr->dst->regoff);

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
			i386_alu_membase_reg(alu_op, REG_SP, src->regoff * 8, iptr->dst->regoff);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			M_INTMOVE(src->regoff, iptr->dst->regoff);
			i386_alu_membase_reg(alu_op, REG_SP, src->prev->regoff * 8, iptr->dst->regoff);

		} else {
			if (src->regoff == iptr->dst->regoff) {
				i386_alu_reg_reg(alu_op, src->prev->regoff, iptr->dst->regoff);

			} else {
				M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
				i386_alu_reg_reg(alu_op, src->regoff, iptr->dst->regoff);
			}
		}
	}
}



void i386_emit_ialuconst(s4 alu_op, stackptr src, instruction *iptr)
{
	if (iptr->dst->flags & INMEMORY) {
		if (src->flags & INMEMORY) {
			if (src->regoff == iptr->dst->regoff) {
				i386_alu_imm_membase(alu_op, iptr->val.i, REG_SP, iptr->dst->regoff * 8);

			} else {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_imm_reg(alu_op, iptr->val.i, REG_ITMP1);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
			}

		} else {
			i386_mov_reg_membase(src->regoff, REG_SP, iptr->dst->regoff * 8);
			i386_alu_imm_membase(alu_op, iptr->val.i, REG_SP, iptr->dst->regoff * 8);
		}

	} else {
		if (src->flags & INMEMORY) {
			i386_mov_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);
			i386_alu_imm_reg(alu_op, iptr->val.i, iptr->dst->regoff);

		} else {
			M_INTMOVE(src->regoff, iptr->dst->regoff);
			i386_alu_imm_reg(alu_op, iptr->val.i, iptr->dst->regoff);
		}
	}
}



void i386_emit_lalu(s4 alu_op, stackptr src, instruction *iptr)
{
	if (iptr->dst->flags & INMEMORY) {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (src->regoff == iptr->dst->regoff) {
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_alu_reg_membase(alu_op, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP1);
				i386_alu_reg_membase(alu_op, REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);

			} else if (src->prev->regoff == iptr->dst->regoff) {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_reg_membase(alu_op, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP1);
				i386_alu_reg_membase(alu_op, REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);

			} else {
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_alu_membase_reg(alu_op, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP1);
				i386_alu_membase_reg(alu_op, REG_SP, src->regoff * 8 + 4, REG_ITMP1);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);
			}
		}
	}
}



void i386_emit_laluconst(s4 alu_op, stackptr src, instruction *iptr)
{
	if (iptr->dst->flags & INMEMORY) {
		if (src->flags & INMEMORY) {
			if (src->regoff == iptr->dst->regoff) {
				i386_alu_imm_membase(alu_op, iptr->val.l, REG_SP, iptr->dst->regoff * 8);
				i386_alu_imm_membase(alu_op, iptr->val.l >> 32, REG_SP, iptr->dst->regoff * 8 + 4);

			} else {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_imm_reg(alu_op, iptr->val.l, REG_ITMP1);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP1);
				i386_alu_imm_reg(alu_op, iptr->val.l >> 32, REG_ITMP1);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);
			}
		}
	}
}



void i386_emit_ishift(s4 shift_op, stackptr src, instruction *iptr)
{
	if (iptr->dst->flags & INMEMORY) {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (src->prev->regoff == iptr->dst->regoff) {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, ECX);
				i386_shift_membase(shift_op, REG_SP, iptr->dst->regoff * 8);

			} else {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, ECX);
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_shift_reg(shift_op, REG_ITMP1);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
			}

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			i386_mov_membase_reg(REG_SP, src->regoff * 8, ECX);
			i386_mov_reg_membase(src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
			i386_shift_membase(shift_op, REG_SP, iptr->dst->regoff * 8);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (src->prev->regoff == iptr->dst->regoff) {
				M_INTMOVE(src->regoff, ECX);
				i386_shift_membase(shift_op, REG_SP, iptr->dst->regoff * 8);

			} else {
				M_INTMOVE(src->regoff, ECX);
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_shift_reg(shift_op, REG_ITMP1);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
			}

		} else {
			M_INTMOVE(src->regoff, ECX);
			i386_mov_reg_membase(src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
			i386_shift_membase(shift_op, REG_SP, iptr->dst->regoff * 8);
		}

	} else {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			i386_mov_membase_reg(REG_SP, src->regoff * 8, ECX);
			i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
			i386_shift_reg(shift_op, iptr->dst->regoff);

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			i386_mov_membase_reg(REG_SP, src->regoff * 8, ECX);
			M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
			i386_shift_reg(shift_op, iptr->dst->regoff);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			M_INTMOVE(src->regoff, ECX);
			i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
			i386_shift_reg(shift_op, iptr->dst->regoff);

		} else {
			M_INTMOVE(src->regoff, ECX);
			M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
			i386_shift_reg(shift_op, iptr->dst->regoff);
		}
	}
}



void i386_emit_ishiftconst(s4 shift_op, stackptr src, instruction *iptr)
{
	if ((src->flags & INMEMORY) && (iptr->dst->flags & INMEMORY)) {
		if (src->regoff == iptr->dst->regoff) {
			i386_shift_imm_membase(shift_op, iptr->val.i, REG_SP, iptr->dst->regoff * 8);

		} else {
			i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
			i386_shift_imm_reg(shift_op, iptr->val.i, REG_ITMP1);
			i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
		}

	} else if ((src->flags & INMEMORY) && !(iptr->dst->flags & INMEMORY)) {
		i386_mov_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);
		i386_shift_imm_reg(shift_op, iptr->val.i, iptr->dst->regoff);
				
	} else if (!(src->flags & INMEMORY) && (iptr->dst->flags & INMEMORY)) {
		i386_mov_reg_membase(src->regoff, REG_SP, iptr->dst->regoff * 8);
		i386_shift_imm_membase(shift_op, iptr->val.i, REG_SP, iptr->dst->regoff * 8);

	} else {
		M_INTMOVE(src->regoff, iptr->dst->regoff);
		i386_shift_imm_reg(shift_op, iptr->val.i, iptr->dst->regoff);
	}
}



void i386_emit_ifcc_iconst(s4 if_op, stackptr src, instruction *iptr)
{
	if (iptr->dst->flags & INMEMORY) {
		int offset = 0;

		if (src->flags & INMEMORY) {
			i386_alu_imm_membase(I386_CMP, 0, REG_SP, src->regoff * 8);

		} else {
			i386_test_reg_reg(src->regoff, src->regoff);
		}

		offset += 7;
		CALCOFFSETBYTES(offset, REG_SP, iptr->dst->regoff * 8);
	
		i386_jcc(if_op, offset + (iptr[1].opc == ICMD_ELSE_ICONST) ? 5 + offset : 0);
		i386_mov_imm_membase(iptr->val.i, REG_SP, iptr->dst->regoff * 8);

		if ((iptr[1].opc == ICMD_ELSE_ICONST)) {
			i386_jmp_imm(offset);
			i386_mov_imm_membase(iptr[1].val.i, REG_SP, iptr->dst->regoff * 8);
		}

	} else {
		if (src->flags & INMEMORY) {
			i386_alu_imm_membase(I386_CMP, 0, REG_SP, src->regoff * 8);

		} else {
			i386_test_reg_reg(src->regoff, src->regoff);
		}

		i386_jcc(if_op, (iptr[1].opc == ICMD_ELSE_ICONST) ? 10 : 5);
		i386_mov_imm_reg(iptr->val.i, iptr->dst->regoff);

		if ((iptr[1].opc == ICMD_ELSE_ICONST)) {
			i386_jmp_imm(5);
			i386_mov_imm_reg(iptr[1].val.i, iptr->dst->regoff);
		}
	}
}



/*
 * mov ops
 */
void i386_mov_reg_reg(s4 reg, s4 dreg) {
	*(mcodeptr++) = (u1) 0x89;
	i386_emit_reg((reg),(dreg));
}


void i386_mov_imm_reg(s4 imm, s4 reg) {
	*(mcodeptr++) = (u1) 0xb8 + ((reg) & 0x07);
	i386_emit_imm32((imm));
}


void i386_movb_imm_reg(s4 imm, s4 reg) {
	*(mcodeptr++) = (u1) 0xc6;
	i386_emit_reg(0,(reg));
	i386_emit_imm8((imm));
}


void i386_mov_membase_reg(s4 basereg, s4 disp, s4 reg) {
	*(mcodeptr++) = (u1) 0x8b;
	i386_emit_membase((basereg),(disp),(reg));
}


/*
 * this one is for INVOKEVIRTUAL/INVOKEINTERFACE to have a
 * constant membase immediate length of 32bit
 */
void i386_mov_membase32_reg(s4 basereg, s4 disp, s4 reg) {
	*(mcodeptr++) = (u1) 0x8b;
	i386_address_byte(2, (reg), (basereg));
	i386_emit_imm32((disp));
}


void i386_mov_reg_membase(s4 reg, s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0x89;
	i386_emit_membase((basereg),(disp),(reg));
}


void i386_mov_memindex_reg(s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg) {
	*(mcodeptr++) = (u1) 0x8b;
	i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void i386_mov_reg_memindex(s4 reg, s4 disp, s4 basereg, s4 indexreg, s4 scale) {
	*(mcodeptr++) = (u1) 0x89;
	i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void i386_mov_mem_reg(s4 mem, s4 dreg) {
	*(mcodeptr++) = (u1) 0x8b;
	i386_emit_mem((dreg),(mem));
}


void i386_movw_reg_memindex(s4 reg, s4 disp, s4 basereg, s4 indexreg, s4 scale) {
	*(mcodeptr++) = (u1) 0x66;
	*(mcodeptr++) = (u1) 0x89;
	i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void i386_movb_reg_memindex(s4 reg, s4 disp, s4 basereg, s4 indexreg, s4 scale) {
	*(mcodeptr++) = (u1) 0x88;
	i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void i386_mov_imm_membase(s4 imm, s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xc7;
	i386_emit_membase((basereg),(disp),0);
	i386_emit_imm32((imm));
}


void i386_movsbl_memindex_reg(s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xbe;
	i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void i386_movswl_memindex_reg(s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xbf;
	i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void i386_movzwl_memindex_reg(s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xb7;
	i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}



/*
 * alu operations
 */
void i386_alu_reg_reg(s4 opc, s4 reg, s4 dreg) {
	*(mcodeptr++) = (((u1) (opc)) << 3) + 1;
	i386_emit_reg((reg),(dreg));
}


void i386_alu_reg_membase(s4 opc, s4 reg, s4 basereg, s4 disp) {
	*(mcodeptr++) = (((u1) (opc)) << 3) + 1;
	i386_emit_membase((basereg),(disp),(reg));
}


void i386_alu_membase_reg(s4 opc, s4 basereg, s4 disp, s4 reg) {
	*(mcodeptr++) = (((u1) (opc)) << 3) + 3;
	i386_emit_membase((basereg),(disp),(reg));
}


void i386_alu_imm_reg(s4 opc, s4 imm, s4 dreg) {
	if (i386_is_imm8(imm)) { 
		*(mcodeptr++) = (u1) 0x83;
		i386_emit_reg((opc),(dreg));
		i386_emit_imm8((imm));
	} else { 
		*(mcodeptr++) = (u1) 0x81;
		i386_emit_reg((opc),(dreg));
		i386_emit_imm32((imm));
	} 
}


void i386_alu_imm_membase(s4 opc, s4 imm, s4 basereg, s4 disp) {
	if (i386_is_imm8(imm)) { 
		*(mcodeptr++) = (u1) 0x83;
		i386_emit_membase((basereg),(disp),(opc));
		i386_emit_imm8((imm));
	} else { 
		*(mcodeptr++) = (u1) 0x81;
		i386_emit_membase((basereg),(disp),(opc));
		i386_emit_imm32((imm));
	} 
}


void i386_test_reg_reg(s4 reg, s4 dreg) {
	*(mcodeptr++) = (u1) 0x85;
	i386_emit_reg((reg),(dreg));
}


void i386_test_imm_reg(s4 imm, s4 reg) {
	*(mcodeptr++) = (u1) 0xf7;
	i386_emit_reg(0,(reg));
	i386_emit_imm32((imm));
}



/*
 * inc, dec operations
 */
void i386_dec_mem(s4 mem) {
	*(mcodeptr++) = (u1) 0xff;
	i386_emit_mem(1,(mem));
}



void i386_cltd() {
	*(mcodeptr++) = (u1) 0x99;
}



void i386_imul_reg_reg(s4 reg, s4 dreg) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xaf;
	i386_emit_reg((dreg),(reg));
}


void i386_imul_membase_reg(s4 basereg, s4 disp, s4 dreg) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xaf;
	i386_emit_membase((basereg),(disp),(dreg));
}


void i386_imul_imm_reg(s4 imm, s4 dreg) {
	if (i386_is_imm8((imm))) { 
		*(mcodeptr++) = (u1) 0x6b;
		i386_emit_reg(0,(dreg));
		i386_emit_imm8((imm));
	} else { 
		*(mcodeptr++) = (u1) 0x69;
		i386_emit_reg(0,(dreg));
		i386_emit_imm32((imm));
	} 
}


void i386_imul_imm_reg_reg(s4 imm, s4 reg, s4 dreg) {
	if (i386_is_imm8((imm))) { 
		*(mcodeptr++) = (u1) 0x6b;
		i386_emit_reg((dreg),(reg));
		i386_emit_imm8((imm));
	} else { 
		*(mcodeptr++) = (u1) 0x69;
		i386_emit_reg((dreg),(reg));
		i386_emit_imm32((imm));
	} 
}


void i386_imul_imm_membase_reg(s4 imm, s4 basereg, s4 disp, s4 dreg) {
	if (i386_is_imm8((imm))) { 
		*(mcodeptr++) = (u1) 0x6b;
		i386_emit_membase((basereg),(disp),(dreg));
		i386_emit_imm8((imm));
	} else { 
		*(mcodeptr++) = (u1) 0x69;
		i386_emit_membase((basereg),(disp),(dreg));
		i386_emit_imm32((imm));
	} 
}


void i386_mul_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xf7;
	i386_emit_membase((basereg),(disp),4);
}


void i386_idiv_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xf7;
	i386_emit_reg(7,(reg));
}



void i386_ret() {
	*(mcodeptr++) = (u1) 0xc3;
}



/*
 * shift ops
 */
void i386_shift_reg(s4 opc, s4 reg) {
	*(mcodeptr++) = (u1) 0xd3;
	i386_emit_reg((opc),(reg));
}


void i386_shift_membase(s4 opc, s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xd3;
	i386_emit_membase((basereg),(disp),(opc));
}


void i386_shift_imm_reg(s4 opc, s4 imm, s4 dreg) {
	if ((imm) == 1) { 
		*(mcodeptr++) = (u1) 0xd1;
		i386_emit_reg((opc),(dreg));
	} else { 
		*(mcodeptr++) = (u1) 0xc1;
		i386_emit_reg((opc),(dreg));
		i386_emit_imm8((imm));
	} 
}


void i386_shift_imm_membase(s4 opc, s4 imm, s4 basereg, s4 disp) {
	if ((imm) == 1) { 
		*(mcodeptr++) = (u1) 0xd1;
		i386_emit_membase((basereg),(disp),(opc));
	} else { 
		*(mcodeptr++) = (u1) 0xc1;
		i386_emit_membase((basereg),(disp),(opc));
		i386_emit_imm8((imm));
	} 
}


void i386_shld_reg_reg(s4 reg, s4 dreg) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xa5;
	i386_emit_reg((reg),(dreg));
}


void i386_shld_imm_reg_reg(s4 imm, s4 reg, s4 dreg) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xa4;
	i386_emit_reg((reg),(dreg));
	i386_emit_imm8((imm));
}


void i386_shld_reg_membase(s4 reg, s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xa5;
	i386_emit_membase((basereg),(disp),(reg));
}


void i386_shrd_reg_reg(s4 reg, s4 dreg) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xad;
	i386_emit_reg((reg),(dreg));
}


void i386_shrd_imm_reg_reg(s4 imm, s4 reg, s4 dreg) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xac;
	i386_emit_reg((reg),(dreg));
	i386_emit_imm8((imm));
}


void i386_shrd_reg_membase(s4 reg, s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xad;
	i386_emit_membase((basereg),(disp),(reg));
}



/*
 * jump operations
 */
void i386_jmp_imm(s4 imm) {
	*(mcodeptr++) = (u1) 0xe9;
	i386_emit_imm32((imm));
}


void i386_jmp_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xff;
	i386_emit_reg(4,(reg));
}


void i386_jcc(s4 opc, s4 imm) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) (0x80 + (opc));
	i386_emit_imm32((imm));
}



/*
 * conditional set operations
 */
void i386_setcc_reg(s4 opc, s4 reg) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) (0x90 + (opc));
	i386_emit_reg(0,(reg));
}


void i386_setcc_membase(s4 opc, s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) (0x90 + (opc));
	i386_emit_membase((basereg),(disp),0);
}


void i386_xadd_reg_mem(s4 reg, s4 mem) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xc1;
	i386_emit_mem((reg),(mem));
}


void i386_neg_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xf7;
	i386_emit_reg(3,(reg));
}


void i386_neg_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xf7;
	i386_emit_membase((basereg),(disp),3);
}



void i386_push_imm(s4 imm) {
	*(mcodeptr++) = (u1) 0x68;
	i386_emit_imm32((imm));
}


void i386_pop_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0x58 + (0x07 & (reg));
}


void i386_push_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0x50 + (0x07 & (reg));
}


void i386_nop() {
	*(mcodeptr++) = (u1) 0x90;
}


void i386_lock() {
	*(mcodeptr++) = (u1) 0xf0;
}


/*
 * call instructions
 */
void i386_call_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xff;
	i386_emit_reg(2,(reg));
}


void i386_call_imm(s4 imm) {
	*(mcodeptr++) = (u1) 0xe8;
	i386_emit_imm32((imm));
}


void i386_call_mem(s4 mem) {
	*(mcodeptr++) = (u1) 0xff;
	i386_emit_mem(2, (mem));
}



/*
 * floating point instructions
 */
void i386_fld1() {
	*(mcodeptr++) = (u1) 0xd9;
	*(mcodeptr++) = (u1) 0xe8;
}


void i386_fldz() {
	*(mcodeptr++) = (u1) 0xd9;
	*(mcodeptr++) = (u1) 0xee;
}


void i386_fld_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xd9;
	*(mcodeptr++) = (u1) 0xc0 + (0x07 & (reg));
}


void i386_flds_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xd9;
	i386_emit_membase((basereg),(disp),0);
}


void i386_fldl_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdd;
	i386_emit_membase((basereg),(disp),0);
}


void i386_fldt_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdb;
	i386_emit_membase((basereg),(disp),5);
}


void i386_flds_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale) {
	*(mcodeptr++) = (u1) 0xd9;
	i386_emit_memindex(0,(disp),(basereg),(indexreg),(scale));
}


void i386_fldl_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale) {
	*(mcodeptr++) = (u1) 0xdd;
	i386_emit_memindex(0,(disp),(basereg),(indexreg),(scale));
}




void i386_fildl_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdb;
	i386_emit_membase((basereg),(disp),0);
}


void i386_fildll_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdf;
	i386_emit_membase((basereg),(disp),5);
}




void i386_fst_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xdd;
	*(mcodeptr++) = (u1) 0xd0 + (0x07 & (reg));
}


void i386_fsts_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xd9;
	i386_emit_membase((basereg),(disp),2);
}


void i386_fstl_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdd;
	i386_emit_membase((basereg),(disp),2);
}


void i386_fsts_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale) {
	*(mcodeptr++) = (u1) 0xd9;
	i386_emit_memindex(2,(disp),(basereg),(indexreg),(scale));
}


void i386_fstl_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale) {
	*(mcodeptr++) = (u1) 0xdd;
	i386_emit_memindex(2,(disp),(basereg),(indexreg),(scale));
}


void i386_fstp_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xdd;
	*(mcodeptr++) = (u1) 0xd8 + (0x07 & (reg));
}


void i386_fstps_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xd9;
	i386_emit_membase((basereg),(disp),3);
}


void i386_fstpl_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdd;
	i386_emit_membase((basereg),(disp),3);
}


void i386_fstpt_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdb;
	i386_emit_membase((basereg),(disp),7);
}


void i386_fstps_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale) {
	*(mcodeptr++) = (u1) 0xd9;
	i386_emit_memindex(3,(disp),(basereg),(indexreg),(scale));
}


void i386_fstpl_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale) {
	*(mcodeptr++) = (u1) 0xdd;
	i386_emit_memindex(3,(disp),(basereg),(indexreg),(scale));
}


void i386_fistl_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdb;
	i386_emit_membase((basereg),(disp),2);
}


void i386_fistpl_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdb;
	i386_emit_membase((basereg),(disp),3);
}


void i386_fistpll_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdf;
	i386_emit_membase((basereg),(disp),7);
}


void i386_fchs() {
	*(mcodeptr++) = (u1) 0xd9;
	*(mcodeptr++) = (u1) 0xe0;
}


void i386_faddp() {
	*(mcodeptr++) = (u1) 0xde;
	*(mcodeptr++) = (u1) 0xc1;
}


void i386_fadd_reg_st(s4 reg) {
	*(mcodeptr++) = (u1) 0xd8;
	*(mcodeptr++) = (u1) 0xc0 + (0x0f & (reg));
}


void i386_fadd_st_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xdc;
	*(mcodeptr++) = (u1) 0xc0 + (0x0f & (reg));
}


void i386_faddp_st_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xde;
	*(mcodeptr++) = (u1) 0xc0 + (0x0f & (reg));
}


void i386_fadds_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xd8;
	i386_emit_membase((basereg),(disp),0);
}


void i386_faddl_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdc;
	i386_emit_membase((basereg),(disp),0);
}


void i386_fsub_reg_st(s4 reg) {
	*(mcodeptr++) = (u1) 0xd8;
	*(mcodeptr++) = (u1) 0xe0 + (0x07 & (reg));
}


void i386_fsub_st_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xdc;
	*(mcodeptr++) = (u1) 0xe8 + (0x07 & (reg));
}


void i386_fsubp_st_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xde;
	*(mcodeptr++) = (u1) 0xe8 + (0x07 & (reg));
}


void i386_fsubp() {
	*(mcodeptr++) = (u1) 0xde;
	*(mcodeptr++) = (u1) 0xe9;
}


void i386_fsubs_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xd8;
	i386_emit_membase((basereg),(disp),4);
}


void i386_fsubl_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdc;
	i386_emit_membase((basereg),(disp),4);
}


void i386_fmul_reg_st(s4 reg) {
	*(mcodeptr++) = (u1) 0xd8;
	*(mcodeptr++) = (u1) 0xc8 + (0x07 & (reg));
}


void i386_fmul_st_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xdc;
	*(mcodeptr++) = (u1) 0xc8 + (0x07 & (reg));
}


void i386_fmulp() {
	*(mcodeptr++) = (u1) 0xde;
	*(mcodeptr++) = (u1) 0xc9;
}


void i386_fmulp_st_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xde;
	*(mcodeptr++) = (u1) 0xc8 + (0x07 & (reg));
}


void i386_fmuls_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xd8;
	i386_emit_membase((basereg),(disp),1);
}


void i386_fmull_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdc;
	i386_emit_membase((basereg),(disp),1);
}


void i386_fdiv_reg_st(s4 reg) {
	*(mcodeptr++) = (u1) 0xd8;
	*(mcodeptr++) = (u1) 0xf0 + (0x07 & (reg));
}


void i386_fdiv_st_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xdc;
	*(mcodeptr++) = (u1) 0xf8 + (0x07 & (reg));
}


void i386_fdivp() {
	*(mcodeptr++) = (u1) 0xde;
	*(mcodeptr++) = (u1) 0xf9;
}


void i386_fdivp_st_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xde;
	*(mcodeptr++) = (u1) 0xf8 + (0x07 & (reg));
}


void i386_fxch() {
	*(mcodeptr++) = (u1) 0xd9;
	*(mcodeptr++) = (u1) 0xc9;
}


void i386_fxch_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xd9;
	*(mcodeptr++) = (u1) 0xc8 + (0x07 & (reg));
}


void i386_fprem() {
	*(mcodeptr++) = (u1) 0xd9;
	*(mcodeptr++) = (u1) 0xf8;
}


void i386_fprem1() {
	*(mcodeptr++) = (u1) 0xd9;
	*(mcodeptr++) = (u1) 0xf5;
}


void i386_fucom() {
	*(mcodeptr++) = (u1) 0xdd;
	*(mcodeptr++) = (u1) 0xe1;
}


void i386_fucom_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xdd;
	*(mcodeptr++) = (u1) 0xe0 + (0x07 & (reg));
}


void i386_fucomp_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xdd;
	*(mcodeptr++) = (u1) 0xe8 + (0x07 & (reg));
}


void i386_fucompp() {
	*(mcodeptr++) = (u1) 0xda;
	*(mcodeptr++) = (u1) 0xe9;
}


void i386_fnstsw() {
	*(mcodeptr++) = (u1) 0xdf;
	*(mcodeptr++) = (u1) 0xe0;
}


void i386_sahf() {
	*(mcodeptr++) = (u1) 0x9e;
}


void i386_finit() {
	*(mcodeptr++) = (u1) 0x9b;
	*(mcodeptr++) = (u1) 0xdb;
	*(mcodeptr++) = (u1) 0xe3;
}


void i386_fldcw_mem(s4 mem) {
	*(mcodeptr++) = (u1) 0xd9;
	i386_emit_mem(5,(mem));
}


void i386_fldcw_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xd9;
	i386_emit_membase((basereg),(disp),5);
}


void i386_wait() {
	*(mcodeptr++) = (u1) 0x9b;
}


void i386_ffree_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xdd;
	*(mcodeptr++) = (u1) 0xc0 + (0x07 & (reg));
}


void i386_fdecstp() {
	*(mcodeptr++) = (u1) 0xd9;
	*(mcodeptr++) = (u1) 0xf6;
}


void i386_fincstp() {
	*(mcodeptr++) = (u1) 0xd9;
	*(mcodeptr++) = (u1) 0xf7;
}


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
