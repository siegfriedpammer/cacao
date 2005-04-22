/* vm/jit/i386/emitfuncs.c - i386 code emitter functions

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

   $Id: emitfuncs.c 2356 2005-04-22 17:33:35Z christian $

*/

#include "vm/statistics.h"
#include "vm/jit/jit.h"
#include "vm/jit/i386/emitfuncs.h"
#include "vm/jit/i386/codegen.h"
#include "vm/jit/i386/types.h"

#ifdef STATISTICS
#define COUNT(a) (a)++
#else
#define COUNT(a)
#endif


void i386_emit_ialu(codegendata *cd, s4 alu_op, stackptr src, instruction *iptr)
{
	if (iptr->dst->flags & INMEMORY) {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (src->regoff == iptr->dst->regoff) {
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
				i386_alu_reg_membase(cd, alu_op, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);

			} else if (src->prev->regoff == iptr->dst->regoff) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
				i386_alu_reg_membase(cd, alu_op, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);

			} else {
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
				i386_alu_membase_reg(cd, alu_op, REG_SP, src->regoff * 4, REG_ITMP1);
				i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
			}

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			if (src->regoff == iptr->dst->regoff) {
				i386_alu_reg_membase(cd, alu_op, src->prev->regoff, REG_SP, iptr->dst->regoff * 4);

			} else {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
				i386_alu_reg_reg(cd, alu_op, src->prev->regoff, REG_ITMP1);
				i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
			}

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (src->prev->regoff == iptr->dst->regoff) {
				i386_alu_reg_membase(cd, alu_op, src->regoff, REG_SP, iptr->dst->regoff * 4);
						
			} else {
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
				i386_alu_reg_reg(cd, alu_op, src->regoff, REG_ITMP1);
				i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
			}

		} else {
			i386_mov_reg_membase(cd, src->prev->regoff, REG_SP, iptr->dst->regoff * 4);
			i386_alu_reg_membase(cd, alu_op, src->regoff, REG_SP, iptr->dst->regoff * 4);
		}

	} else {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, iptr->dst->regoff);
			i386_alu_membase_reg(cd, alu_op, REG_SP, src->regoff * 4, iptr->dst->regoff);

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			if (src->prev->regoff != iptr->dst->regoff)
				i386_mov_reg_reg(cd, src->prev->regoff, iptr->dst->regoff);

			i386_alu_membase_reg(cd, alu_op, REG_SP, src->regoff * 4, iptr->dst->regoff);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (src->regoff != iptr->dst->regoff)
				i386_mov_reg_reg(cd, src->regoff, iptr->dst->regoff);

			i386_alu_membase_reg(cd, alu_op, REG_SP, src->prev->regoff * 4, iptr->dst->regoff);

		} else {
			if (src->regoff == iptr->dst->regoff) {
				i386_alu_reg_reg(cd, alu_op, src->prev->regoff, iptr->dst->regoff);

			} else {
				if (src->prev->regoff != iptr->dst->regoff)
					i386_mov_reg_reg(cd, src->prev->regoff, iptr->dst->regoff);

				i386_alu_reg_reg(cd, alu_op, src->regoff, iptr->dst->regoff);
			}
		}
	}
}


void i386_emit_ialuconst(codegendata *cd, s4 alu_op, stackptr src, instruction *iptr)
{
	if (iptr->dst->flags & INMEMORY) {
		if (src->flags & INMEMORY) {
			if (src->regoff == iptr->dst->regoff) {
				i386_alu_imm_membase(cd, alu_op, iptr->val.i, REG_SP, iptr->dst->regoff * 4);

			} else {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
				i386_alu_imm_reg(cd, alu_op, iptr->val.i, REG_ITMP1);
				i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
			}

		} else {
			i386_mov_reg_membase(cd, src->regoff, REG_SP, iptr->dst->regoff * 4);
			i386_alu_imm_membase(cd, alu_op, iptr->val.i, REG_SP, iptr->dst->regoff * 4);
		}

	} else {
		if (src->flags & INMEMORY) {
			i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, iptr->dst->regoff);
			i386_alu_imm_reg(cd, alu_op, iptr->val.i, iptr->dst->regoff);

		} else {
			if (src->regoff != iptr->dst->regoff)
				i386_mov_reg_reg(cd, src->regoff, iptr->dst->regoff);

			/* `inc reg' is slower on p4's (regarding to ia32 optimization    */
			/* reference manual and benchmarks) and as fast on athlon's.      */
			i386_alu_imm_reg(cd, alu_op, iptr->val.i, iptr->dst->regoff);
		}
	}
}


void i386_emit_lalu(codegendata *cd, s4 alu_op, stackptr src, instruction *iptr)
{
	if (iptr->dst->flags & INMEMORY) {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (src->regoff == iptr->dst->regoff) {
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
				i386_alu_reg_membase(cd, alu_op, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4 + 4, REG_ITMP1);
				i386_alu_reg_membase(cd, alu_op, REG_ITMP1, REG_SP, iptr->dst->regoff * 4 + 4);

			} else if (src->prev->regoff == iptr->dst->regoff) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
				i386_alu_reg_membase(cd, alu_op, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4 + 4, REG_ITMP1);
				i386_alu_reg_membase(cd, alu_op, REG_ITMP1, REG_SP, iptr->dst->regoff * 4 + 4);

			} else {
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
				i386_alu_membase_reg(cd, alu_op, REG_SP, src->regoff * 4, REG_ITMP1);
				i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4 + 4, REG_ITMP1);
				i386_alu_membase_reg(cd, alu_op, REG_SP, src->regoff * 4 + 4, REG_ITMP1);
				i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4 + 4);
			}
		}
	}
}


void i386_emit_laluconst(codegendata *cd, s4 alu_op, stackptr src, instruction *iptr)
{
	if (iptr->dst->flags & INMEMORY) {
		if (src->flags & INMEMORY) {
			if (src->regoff == iptr->dst->regoff) {
				i386_alu_imm_membase(cd, alu_op, iptr->val.l, REG_SP, iptr->dst->regoff * 4);
				i386_alu_imm_membase(cd, alu_op, iptr->val.l >> 32, REG_SP, iptr->dst->regoff * 4 + 4);

			} else {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
				i386_alu_imm_reg(cd, alu_op, iptr->val.l, REG_ITMP1);
				i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4 + 4, REG_ITMP1);
				i386_alu_imm_reg(cd, alu_op, iptr->val.l >> 32, REG_ITMP1);
				i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4 + 4);
			}
		}
	}
}


void i386_emit_ishift(codegendata *cd, s4 shift_op, stackptr src, instruction *iptr)
{
	if (iptr->dst->flags & INMEMORY) {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (src->prev->regoff == iptr->dst->regoff) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, ECX);
				i386_shift_membase(cd, shift_op, REG_SP, iptr->dst->regoff * 4);

			} else {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, ECX);
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
				i386_shift_reg(cd, shift_op, REG_ITMP1);
				i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
			}

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, ECX);
			i386_mov_reg_membase(cd, src->prev->regoff, REG_SP, iptr->dst->regoff * 4);
			i386_shift_membase(cd, shift_op, REG_SP, iptr->dst->regoff * 4);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (src->prev->regoff == iptr->dst->regoff) {
				if (src->regoff != ECX)
					i386_mov_reg_reg(cd, src->regoff, ECX);

				i386_shift_membase(cd, shift_op, REG_SP, iptr->dst->regoff * 4);

			} else {	
				if (src->regoff != ECX)
					i386_mov_reg_reg(cd, src->regoff, ECX);

				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
				i386_shift_reg(cd, shift_op, REG_ITMP1);
				i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
			}

		} else {
			if (src->regoff != ECX)
				i386_mov_reg_reg(cd, src->regoff, ECX);

			i386_mov_reg_membase(cd, src->prev->regoff, REG_SP, iptr->dst->regoff * 4);
			i386_shift_membase(cd, shift_op, REG_SP, iptr->dst->regoff * 4);
		}

	} else {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, ECX);
			i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, iptr->dst->regoff);
			i386_shift_reg(cd, shift_op, iptr->dst->regoff);

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, ECX);

			if (src->prev->regoff != iptr->dst->regoff)
				i386_mov_reg_reg(cd, src->prev->regoff, iptr->dst->regoff);

			i386_shift_reg(cd, shift_op, iptr->dst->regoff);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (src->regoff != ECX)
				i386_mov_reg_reg(cd, src->regoff, ECX);

			i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, iptr->dst->regoff);
			i386_shift_reg(cd, shift_op, iptr->dst->regoff);

		} else {
			if (src->regoff != ECX)
				i386_mov_reg_reg(cd, src->regoff, ECX);

			if (src->prev->regoff != iptr->dst->regoff)
				i386_mov_reg_reg(cd, src->prev->regoff, iptr->dst->regoff);

			i386_shift_reg(cd, shift_op, iptr->dst->regoff);
		}
	}
}


void i386_emit_ishiftconst(codegendata *cd, s4 shift_op, stackptr src, instruction *iptr)
{
	if ((src->flags & INMEMORY) && (iptr->dst->flags & INMEMORY)) {
		if (src->regoff == iptr->dst->regoff) {
			i386_shift_imm_membase(cd, shift_op, iptr->val.i, REG_SP, iptr->dst->regoff * 4);

		} else {
			i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
			i386_shift_imm_reg(cd, shift_op, iptr->val.i, REG_ITMP1);
			i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
		}

	} else if ((src->flags & INMEMORY) && !(iptr->dst->flags & INMEMORY)) {
		i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, iptr->dst->regoff);
		i386_shift_imm_reg(cd, shift_op, iptr->val.i, iptr->dst->regoff);
				
	} else if (!(src->flags & INMEMORY) && (iptr->dst->flags & INMEMORY)) {
		i386_mov_reg_membase(cd, src->regoff, REG_SP, iptr->dst->regoff * 4);
		i386_shift_imm_membase(cd, shift_op, iptr->val.i, REG_SP, iptr->dst->regoff * 4);

	} else {
		if (src->regoff != iptr->dst->regoff)
			i386_mov_reg_reg(cd, src->regoff, iptr->dst->regoff);

		i386_shift_imm_reg(cd, shift_op, iptr->val.i, iptr->dst->regoff);
	}
}


void i386_emit_ifcc_iconst(codegendata *cd, s4 if_op, stackptr src, instruction *iptr)
{
	if (iptr->dst->flags & INMEMORY) {
		s4 offset = 0;

		if (src->flags & INMEMORY) {
			i386_alu_imm_membase(cd, I386_CMP, 0, REG_SP, src->regoff * 4);

		} else {
			i386_test_reg_reg(cd, src->regoff, src->regoff);
		}

		offset += 7;
		CALCOFFSETBYTES(offset, REG_SP, iptr->dst->regoff * 4);
	
		i386_jcc(cd, if_op, offset + (iptr[1].opc == ICMD_ELSE_ICONST) ? 5 + offset : 0);
		i386_mov_imm_membase(cd, iptr->val.i, REG_SP, iptr->dst->regoff * 4);

		if (iptr[1].opc == ICMD_ELSE_ICONST) {
			i386_jmp_imm(cd, offset);
			i386_mov_imm_membase(cd, iptr[1].val.i, REG_SP, iptr->dst->regoff * 4);
		}

	} else {
		if (src->flags & INMEMORY) {
			i386_alu_imm_membase(cd, I386_CMP, 0, REG_SP, src->regoff * 4);

		} else {
			i386_test_reg_reg(cd, src->regoff, src->regoff);
		}

		i386_jcc(cd, if_op, (iptr[1].opc == ICMD_ELSE_ICONST) ? 10 : 5);
		i386_mov_imm_reg(cd, iptr->val.i, iptr->dst->regoff);

		if (iptr[1].opc == ICMD_ELSE_ICONST) {
			i386_jmp_imm(cd, 5);
			i386_mov_imm_reg(cd, iptr[1].val.i, iptr->dst->regoff);
		}
	}
}



/*
 * mov ops
 */
void i386_mov_reg_reg(codegendata *cd, s4 reg, s4 dreg)
{
	COUNT(count_mov_reg_reg);
	*(cd->mcodeptr++) = 0x89;
	i386_emit_reg((reg),(dreg));
}


void i386_mov_imm_reg(codegendata *cd, s4 imm, s4 reg)
{
	*(cd->mcodeptr++) = 0xb8 + ((reg) & 0x07);
	i386_emit_imm32((imm));
}


void i386_movb_imm_reg(codegendata *cd, s4 imm, s4 reg)
{
	*(cd->mcodeptr++) = 0xc6;
	i386_emit_reg(0,(reg));
	i386_emit_imm8((imm));
}


void i386_mov_membase_reg(codegendata *cd, s4 basereg, s4 disp, s4 reg)
{
	COUNT(count_mov_mem_reg);
	*(cd->mcodeptr++) = 0x8b;
	i386_emit_membase((basereg),(disp),(reg));
}


/*
 * this one is for INVOKEVIRTUAL/INVOKEINTERFACE to have a
 * constant membase immediate length of 32bit
 */
void i386_mov_membase32_reg(codegendata *cd, s4 basereg, s4 disp, s4 reg)
{
	COUNT(count_mov_mem_reg);
	*(cd->mcodeptr++) = 0x8b;
	i386_emit_membase32((basereg),(disp),(reg));
}


void i386_mov_reg_membase(codegendata *cd, s4 reg, s4 basereg, s4 disp)
{
	COUNT(count_mov_reg_mem);
	*(cd->mcodeptr++) = 0x89;
	i386_emit_membase((basereg),(disp),(reg));
}


void i386_mov_reg_membase32(codegendata *cd, s4 reg, s4 basereg, s4 disp)
{
	COUNT(count_mov_reg_mem);
	*(cd->mcodeptr++) = 0x89;
	i386_emit_membase32((basereg),(disp),(reg));
}


void i386_mov_memindex_reg(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg)
{
	COUNT(count_mov_mem_reg);
	*(cd->mcodeptr++) = 0x8b;
	i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void i386_mov_reg_memindex(codegendata *cd, s4 reg, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	COUNT(count_mov_reg_mem);
	*(cd->mcodeptr++) = 0x89;
	i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void i386_movw_reg_memindex(codegendata *cd, s4 reg, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	COUNT(count_mov_reg_mem);
	*(cd->mcodeptr++) = 0x66;
	*(cd->mcodeptr++) = 0x89;
	i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void i386_movb_reg_memindex(codegendata *cd, s4 reg, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	COUNT(count_mov_reg_mem);
	*(cd->mcodeptr++) = 0x88;
	i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void i386_mov_reg_mem(codegendata *cd, s4 reg, s4 mem)
{
	COUNT(count_mov_reg_mem);
	*(cd->mcodeptr++) = 0x89;
	i386_emit_mem((reg),(mem));
}


void i386_mov_mem_reg(codegendata *cd, s4 mem, s4 dreg)
{
	COUNT(count_mov_mem_reg);
	*(cd->mcodeptr++) = 0x8b;
	i386_emit_mem((dreg),(mem));
}


void i386_mov_imm_mem(codegendata *cd, s4 imm, s4 mem)
{
	*(cd->mcodeptr++) = 0xc7;
	i386_emit_mem(0, mem);
	i386_emit_imm32(imm);
}


void i386_mov_imm_membase(codegendata *cd, s4 imm, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xc7;
	i386_emit_membase((basereg),(disp),0);
	i386_emit_imm32((imm));
}


void i386_mov_imm_membase32(codegendata *cd, s4 imm, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xc7;
	i386_emit_membase32((basereg),(disp),0);
	i386_emit_imm32((imm));
}


void i386_movb_imm_membase(codegendata *cd, s4 imm, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xc6;
	i386_emit_membase((basereg),(disp),0);
	i386_emit_imm8((imm));
}


void i386_movsbl_memindex_reg(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg)
{
	COUNT(count_mov_mem_reg);
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xbe;
	i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void i386_movswl_memindex_reg(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg)
{
	COUNT(count_mov_mem_reg);
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xbf;
	i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void i386_movzwl_memindex_reg(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg)
{
	COUNT(count_mov_mem_reg);
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xb7;
	i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void i386_mov_imm_memindex(codegendata *cd, s4 imm, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	*(cd->mcodeptr++) = 0xc7;
	i386_emit_memindex(0,(disp),(basereg),(indexreg),(scale));
	i386_emit_imm32((imm));
}


void i386_movw_imm_memindex(codegendata *cd, s4 imm, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	*(cd->mcodeptr++) = 0x66;
	*(cd->mcodeptr++) = 0xc7;
	i386_emit_memindex(0,(disp),(basereg),(indexreg),(scale));
	i386_emit_imm16((imm));
}


void i386_movb_imm_memindex(codegendata *cd, s4 imm, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	*(cd->mcodeptr++) = 0xc6;
	i386_emit_memindex(0,(disp),(basereg),(indexreg),(scale));
	i386_emit_imm8((imm));
}


/*
 * alu operations
 */
void i386_alu_reg_reg(codegendata *cd, s4 opc, s4 reg, s4 dreg)
{
	*(cd->mcodeptr++) = (((u1) (opc)) << 3) + 1;
	i386_emit_reg((reg),(dreg));
}


void i386_alu_reg_membase(codegendata *cd, s4 opc, s4 reg, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = (((u1) (opc)) << 3) + 1;
	i386_emit_membase((basereg),(disp),(reg));
}


void i386_alu_membase_reg(codegendata *cd, s4 opc, s4 basereg, s4 disp, s4 reg)
{
	*(cd->mcodeptr++) = (((u1) (opc)) << 3) + 3;
	i386_emit_membase((basereg),(disp),(reg));
}


void i386_alu_imm_reg(codegendata *cd, s4 opc, s4 imm, s4 dreg)
{
	if (i386_is_imm8(imm)) { 
		*(cd->mcodeptr++) = 0x83;
		i386_emit_reg((opc),(dreg));
		i386_emit_imm8((imm));
	} else { 
		*(cd->mcodeptr++) = 0x81;
		i386_emit_reg((opc),(dreg));
		i386_emit_imm32((imm));
	} 
}


void i386_alu_imm32_reg(codegendata *cd, s4 opc, s4 imm, s4 dreg)
{
	*(cd->mcodeptr++) = 0x81;
	i386_emit_reg((opc),(dreg));
	i386_emit_imm32((imm));
}


void i386_alu_imm_membase(codegendata *cd, s4 opc, s4 imm, s4 basereg, s4 disp)
{
	if (i386_is_imm8(imm)) { 
		*(cd->mcodeptr++) = 0x83;
		i386_emit_membase((basereg),(disp),(opc));
		i386_emit_imm8((imm));
	} else { 
		*(cd->mcodeptr++) = 0x81;
		i386_emit_membase((basereg),(disp),(opc));
		i386_emit_imm32((imm));
	} 
}


void i386_test_reg_reg(codegendata *cd, s4 reg, s4 dreg)
{
	*(cd->mcodeptr++) = 0x85;
	i386_emit_reg((reg),(dreg));
}


void i386_test_imm_reg(codegendata *cd, s4 imm, s4 reg)
{
	*(cd->mcodeptr++) = 0xf7;
	i386_emit_reg(0,(reg));
	i386_emit_imm32((imm));
}



/*
 * inc, dec operations
 */
void i386_dec_mem(codegendata *cd, s4 mem)
{
	*(cd->mcodeptr++) = 0xff;
	i386_emit_mem(1,(mem));
}


void i386_cltd(codegendata *cd)
{
	*(cd->mcodeptr++) = 0x99;
}


void i386_imul_reg_reg(codegendata *cd, s4 reg, s4 dreg)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xaf;
	i386_emit_reg((dreg),(reg));
}


void i386_imul_membase_reg(codegendata *cd, s4 basereg, s4 disp, s4 dreg)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xaf;
	i386_emit_membase((basereg),(disp),(dreg));
}


void i386_imul_imm_reg(codegendata *cd, s4 imm, s4 dreg)
{
	if (i386_is_imm8((imm))) { 
		*(cd->mcodeptr++) = 0x6b;
		i386_emit_reg(0,(dreg));
		i386_emit_imm8((imm));
	} else { 
		*(cd->mcodeptr++) = 0x69;
		i386_emit_reg(0,(dreg));
		i386_emit_imm32((imm));
	} 
}


void i386_imul_imm_reg_reg(codegendata *cd, s4 imm, s4 reg, s4 dreg)
{
	if (i386_is_imm8((imm))) { 
		*(cd->mcodeptr++) = 0x6b;
		i386_emit_reg((dreg),(reg));
		i386_emit_imm8((imm));
	} else { 
		*(cd->mcodeptr++) = 0x69;
		i386_emit_reg((dreg),(reg));
		i386_emit_imm32((imm));
	} 
}


void i386_imul_imm_membase_reg(codegendata *cd, s4 imm, s4 basereg, s4 disp, s4 dreg)
{
	if (i386_is_imm8((imm))) {
		*(cd->mcodeptr++) = 0x6b;
		i386_emit_membase((basereg),(disp),(dreg));
		i386_emit_imm8((imm));
	} else {
		*(cd->mcodeptr++) = 0x69;
		i386_emit_membase((basereg),(disp),(dreg));
		i386_emit_imm32((imm));
	}
}


void i386_mul_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xf7;
	i386_emit_membase((basereg),(disp),4);
}


void i386_idiv_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xf7;
	i386_emit_reg(7,(reg));
}


void i386_ret(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xc3;
}



/*
 * shift ops
 */
void i386_shift_reg(codegendata *cd, s4 opc, s4 reg)
{
	*(cd->mcodeptr++) = 0xd3;
	i386_emit_reg((opc),(reg));
}


void i386_shift_membase(codegendata *cd, s4 opc, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xd3;
	i386_emit_membase((basereg),(disp),(opc));
}


void i386_shift_imm_reg(codegendata *cd, s4 opc, s4 imm, s4 dreg)
{
	if ((imm) == 1) {
		*(cd->mcodeptr++) = 0xd1;
		i386_emit_reg((opc),(dreg));
	} else {
		*(cd->mcodeptr++) = 0xc1;
		i386_emit_reg((opc),(dreg));
		i386_emit_imm8((imm));
	}
}


void i386_shift_imm_membase(codegendata *cd, s4 opc, s4 imm, s4 basereg, s4 disp)
{
	if ((imm) == 1) {
		*(cd->mcodeptr++) = 0xd1;
		i386_emit_membase((basereg),(disp),(opc));
	} else {
		*(cd->mcodeptr++) = 0xc1;
		i386_emit_membase((basereg),(disp),(opc));
		i386_emit_imm8((imm));
	}
}


void i386_shld_reg_reg(codegendata *cd, s4 reg, s4 dreg)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xa5;
	i386_emit_reg((reg),(dreg));
}


void i386_shld_imm_reg_reg(codegendata *cd, s4 imm, s4 reg, s4 dreg)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xa4;
	i386_emit_reg((reg),(dreg));
	i386_emit_imm8((imm));
}


void i386_shld_reg_membase(codegendata *cd, s4 reg, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xa5;
	i386_emit_membase((basereg),(disp),(reg));
}


void i386_shrd_reg_reg(codegendata *cd, s4 reg, s4 dreg)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xad;
	i386_emit_reg((reg),(dreg));
}


void i386_shrd_imm_reg_reg(codegendata *cd, s4 imm, s4 reg, s4 dreg)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xac;
	i386_emit_reg((reg),(dreg));
	i386_emit_imm8((imm));
}


void i386_shrd_reg_membase(codegendata *cd, s4 reg, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xad;
	i386_emit_membase((basereg),(disp),(reg));
}



/*
 * jump operations
 */
void i386_jmp_imm(codegendata *cd, s4 imm)
{
	*(cd->mcodeptr++) = 0xe9;
	i386_emit_imm32((imm));
}


void i386_jmp_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xff;
	i386_emit_reg(4,(reg));
}


void i386_jcc(codegendata *cd, s4 opc, s4 imm)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) =  0x80 + (u1) (opc);
	i386_emit_imm32((imm));
}



/*
 * conditional set operations
 */
void i386_setcc_reg(codegendata *cd, s4 opc, s4 reg)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x90 + (u1) (opc);
	i386_emit_reg(0,(reg));
}


void i386_setcc_membase(codegendata *cd, s4 opc, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) =  0x90 + (u1) (opc);
	i386_emit_membase((basereg),(disp),0);
}


void i386_xadd_reg_mem(codegendata *cd, s4 reg, s4 mem)
{
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xc1;
	i386_emit_mem((reg),(mem));
}


void i386_neg_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xf7;
	i386_emit_reg(3,(reg));
}


void i386_neg_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xf7;
	i386_emit_membase((basereg),(disp),3);
}



void i386_push_imm(codegendata *cd, s4 imm)
{
	*(cd->mcodeptr++) = 0x68;
	i386_emit_imm32((imm));
}


void i386_pop_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0x58 + (0x07 & (u1) (reg));
}


void i386_push_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0x50 + (0x07 & (u1) (reg));
}


void i386_nop(codegendata *cd)
{
	*(cd->mcodeptr++) = 0x90;
}


void i386_lock(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xf0;
}


/*
 * call instructions
 */
void i386_call_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xff;
	i386_emit_reg(2,(reg));
}


void i386_call_imm(codegendata *cd, s4 imm)
{
	*(cd->mcodeptr++) = 0xe8;
	i386_emit_imm32((imm));
}


void i386_call_mem(codegendata *cd, s4 mem)
{
	*(cd->mcodeptr++) = 0xff;
	i386_emit_mem(2,(mem));
}



/*
 * floating point instructions
 */
void i386_fld1(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xd9;
	*(cd->mcodeptr++) = 0xe8;
}


void i386_fldz(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xd9;
	*(cd->mcodeptr++) = 0xee;
}


void i386_fld_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xd9;
	*(cd->mcodeptr++) = 0xc0 + (0x07 & (u1) (reg));
}


void i386_flds_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xd9;
	i386_emit_membase((basereg),(disp),0);
}


void i386_flds_membase32(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xd9;
	i386_emit_membase32((basereg),(disp),0);
}


void i386_fldl_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdd;
	i386_emit_membase((basereg),(disp),0);
}


void i386_fldl_membase32(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdd;
	i386_emit_membase32((basereg),(disp),0);
}


void i386_fldt_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdb;
	i386_emit_membase((basereg),(disp),5);
}


void i386_flds_memindex(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	*(cd->mcodeptr++) = 0xd9;
	i386_emit_memindex(0,(disp),(basereg),(indexreg),(scale));
}


void i386_fldl_memindex(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	*(cd->mcodeptr++) = 0xdd;
	i386_emit_memindex(0,(disp),(basereg),(indexreg),(scale));
}


void i386_flds_mem(codegendata *cd, s4 mem)
{
	*(cd->mcodeptr++) = 0xd9;
	i386_emit_mem(0,(mem));
}


void i386_fldl_mem(codegendata *cd, s4 mem)
{
	*(cd->mcodeptr++) = 0xdd;
	i386_emit_mem(0,(mem));
}


void i386_fildl_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdb;
	i386_emit_membase((basereg),(disp),0);
}


void i386_fildll_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdf;
	i386_emit_membase((basereg),(disp),5);
}


void i386_fst_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xdd;
	*(cd->mcodeptr++) = 0xd0 + (0x07 & (u1) (reg));
}


void i386_fsts_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xd9;
	i386_emit_membase((basereg),(disp),2);
}


void i386_fstl_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdd;
	i386_emit_membase((basereg),(disp),2);
}


void i386_fsts_memindex(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	*(cd->mcodeptr++) = 0xd9;
	i386_emit_memindex(2,(disp),(basereg),(indexreg),(scale));
}


void i386_fstl_memindex(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	*(cd->mcodeptr++) = 0xdd;
	i386_emit_memindex(2,(disp),(basereg),(indexreg),(scale));
}


void i386_fstp_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xdd;
	*(cd->mcodeptr++) = 0xd8 + (0x07 & (u1) (reg));
}


void i386_fstps_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xd9;
	i386_emit_membase((basereg),(disp),3);
}


void i386_fstps_membase32(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xd9;
	i386_emit_membase32((basereg),(disp),3);
}


void i386_fstpl_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdd;
	i386_emit_membase((basereg),(disp),3);
}


void i386_fstpl_membase32(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdd;
	i386_emit_membase32((basereg),(disp),3);
}


void i386_fstpt_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdb;
	i386_emit_membase((basereg),(disp),7);
}


void i386_fstps_memindex(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	*(cd->mcodeptr++) = 0xd9;
	i386_emit_memindex(3,(disp),(basereg),(indexreg),(scale));
}


void i386_fstpl_memindex(codegendata *cd, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	*(cd->mcodeptr++) = 0xdd;
	i386_emit_memindex(3,(disp),(basereg),(indexreg),(scale));
}


void i386_fstps_mem(codegendata *cd, s4 mem)
{
	*(cd->mcodeptr++) = 0xd9;
	i386_emit_mem(3,(mem));
}


void i386_fstpl_mem(codegendata *cd, s4 mem)
{
	*(cd->mcodeptr++) = 0xdd;
	i386_emit_mem(3,(mem));
}


void i386_fistl_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdb;
	i386_emit_membase((basereg),(disp),2);
}


void i386_fistpl_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdb;
	i386_emit_membase((basereg),(disp),3);
}


void i386_fistpll_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdf;
	i386_emit_membase((basereg),(disp),7);
}


void i386_fchs(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xd9;
	*(cd->mcodeptr++) = 0xe0;
}


void i386_faddp(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xde;
	*(cd->mcodeptr++) = 0xc1;
}


void i386_fadd_reg_st(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xd8;
	*(cd->mcodeptr++) = 0xc0 + (0x0f & (u1) (reg));
}


void i386_fadd_st_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xdc;
	*(cd->mcodeptr++) = 0xc0 + (0x0f & (u1) (reg));
}


void i386_faddp_st_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xde;
	*(cd->mcodeptr++) = 0xc0 + (0x0f & (u1) (reg));
}


void i386_fadds_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xd8;
	i386_emit_membase((basereg),(disp),0);
}


void i386_faddl_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdc;
	i386_emit_membase((basereg),(disp),0);
}


void i386_fsub_reg_st(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xd8;
	*(cd->mcodeptr++) = 0xe0 + (0x07 & (u1) (reg));
}


void i386_fsub_st_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xdc;
	*(cd->mcodeptr++) = 0xe8 + (0x07 & (u1) (reg));
}


void i386_fsubp_st_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xde;
	*(cd->mcodeptr++) = 0xe8 + (0x07 & (u1) (reg));
}


void i386_fsubp(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xde;
	*(cd->mcodeptr++) = 0xe9;
}


void i386_fsubs_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xd8;
	i386_emit_membase((basereg),(disp),4);
}


void i386_fsubl_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdc;
	i386_emit_membase((basereg),(disp),4);
}


void i386_fmul_reg_st(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xd8;
	*(cd->mcodeptr++) = 0xc8 + (0x07 & (u1) (reg));
}


void i386_fmul_st_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xdc;
	*(cd->mcodeptr++) = 0xc8 + (0x07 & (u1) (reg));
}


void i386_fmulp(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xde;
	*(cd->mcodeptr++) = 0xc9;
}


void i386_fmulp_st_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xde;
	*(cd->mcodeptr++) = 0xc8 + (0x07 & (u1) (reg));
}


void i386_fmuls_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xd8;
	i386_emit_membase((basereg),(disp),1);
}


void i386_fmull_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xdc;
	i386_emit_membase((basereg),(disp),1);
}


void i386_fdiv_reg_st(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xd8;
	*(cd->mcodeptr++) = 0xf0 + (0x07 & (u1) (reg));
}


void i386_fdiv_st_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xdc;
	*(cd->mcodeptr++) = 0xf8 + (0x07 & (u1) (reg));
}


void i386_fdivp(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xde;
	*(cd->mcodeptr++) = 0xf9;
}


void i386_fdivp_st_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xde;
	*(cd->mcodeptr++) = 0xf8 + (0x07 & (u1) (reg));
}


void i386_fxch(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xd9;
	*(cd->mcodeptr++) = 0xc9;
}


void i386_fxch_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xd9;
	*(cd->mcodeptr++) = 0xc8 + (0x07 & (reg));
}


void i386_fprem(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xd9;
	*(cd->mcodeptr++) = 0xf8;
}


void i386_fprem1(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xd9;
	*(cd->mcodeptr++) = 0xf5;
}


void i386_fucom(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xdd;
	*(cd->mcodeptr++) = 0xe1;
}


void i386_fucom_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xdd;
	*(cd->mcodeptr++) = 0xe0 + (0x07 & (u1) (reg));
}


void i386_fucomp_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xdd;
	*(cd->mcodeptr++) = 0xe8 + (0x07 & (u1) (reg));
}


void i386_fucompp(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xda;
	*(cd->mcodeptr++) = 0xe9;
}


void i386_fnstsw(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xdf;
	*(cd->mcodeptr++) = 0xe0;
}


void i386_sahf(codegendata *cd)
{
	*(cd->mcodeptr++) = 0x9e;
}


void i386_finit(codegendata *cd)
{
	*(cd->mcodeptr++) = 0x9b;
	*(cd->mcodeptr++) = 0xdb;
	*(cd->mcodeptr++) = 0xe3;
}


void i386_fldcw_mem(codegendata *cd, s4 mem)
{
	*(cd->mcodeptr++) = 0xd9;
	i386_emit_mem(5,(mem));
}


void i386_fldcw_membase(codegendata *cd, s4 basereg, s4 disp)
{
	*(cd->mcodeptr++) = 0xd9;
	i386_emit_membase((basereg),(disp),5);
}


void i386_wait(codegendata *cd)
{
	*(cd->mcodeptr++) = 0x9b;
}


void i386_ffree_reg(codegendata *cd, s4 reg)
{
	*(cd->mcodeptr++) = 0xdd;
	*(cd->mcodeptr++) = 0xc0 + (0x07 & (u1) (reg));
}


void i386_fdecstp(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xd9;
	*(cd->mcodeptr++) = 0xf6;
}


void i386_fincstp(codegendata *cd)
{
	*(cd->mcodeptr++) = 0xd9;
	*(cd->mcodeptr++) = 0xf7;
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
