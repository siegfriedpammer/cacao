/* vm/jit/x86_64/emitfuncs.c - x86_64 code emitter functions

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

   $Id: emitfuncs.c 2103 2005-03-28 22:12:23Z christian $

*/


#include "vm/jit/jit.h"
#include "vm/jit/x86_64/codegen.h"
#include "vm/jit/x86_64/emitfuncs.h"
#include "vm/jit/x86_64/types.h"


/* code generation functions */

void x86_64_emit_ialu(codegendata *cd, s4 alu_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->prev->regoff;
	s4 s2 = src->regoff;
	s4 d = iptr->dst->regoff;

	if (iptr->dst->flags & INMEMORY) {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (s2 == d) {
				x86_64_movl_membase_reg(cd, REG_SP, s1 * 8, REG_ITMP1);
				x86_64_alul_reg_membase(cd, alu_op, REG_ITMP1, REG_SP, d * 8);

			} else if (s1 == d) {
				x86_64_movl_membase_reg(cd, REG_SP, s2 * 8, REG_ITMP1);
				x86_64_alul_reg_membase(cd, alu_op, REG_ITMP1, REG_SP, d * 8);

			} else {
				x86_64_movl_membase_reg(cd, REG_SP, s1 * 8, REG_ITMP1);
				x86_64_alul_membase_reg(cd, alu_op, REG_SP, s2 * 8, REG_ITMP1);
				x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, d * 8);
			}

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			if (s2 == d) {
				x86_64_alul_reg_membase(cd, alu_op, s1, REG_SP, d * 8);

			} else {
				x86_64_movl_membase_reg(cd, REG_SP, s2 * 8, REG_ITMP1);
				x86_64_alul_reg_reg(cd, alu_op, s1, REG_ITMP1);
				x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, d * 8);
			}

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (s1 == d) {
				x86_64_alul_reg_membase(cd, alu_op, s2, REG_SP, d * 8);
						
			} else {
				x86_64_movl_membase_reg(cd, REG_SP, s1 * 8, REG_ITMP1);
				x86_64_alul_reg_reg(cd, alu_op, s2, REG_ITMP1);
				x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, d * 8);
			}

		} else {
			x86_64_movl_reg_membase(cd, s1, REG_SP, d * 8);
			x86_64_alul_reg_membase(cd, alu_op, s2, REG_SP, d * 8);
		}

	} else {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			x86_64_movl_membase_reg(cd, REG_SP, s1 * 8, d);
			x86_64_alul_membase_reg(cd, alu_op, REG_SP, s2 * 8, d);

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			M_INTMOVE(s1, d);
			x86_64_alul_membase_reg(cd, alu_op, REG_SP, s2 * 8, d);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			M_INTMOVE(s2, d);
			x86_64_alul_membase_reg(cd, alu_op, REG_SP, s1 * 8, d);

		} else {
			if (s2 == d) {
				x86_64_alul_reg_reg(cd, alu_op, s1, d);

			} else {
				M_INTMOVE(s1, d);
				x86_64_alul_reg_reg(cd, alu_op, s2, d);
			}
		}
	}
}


void x86_64_emit_lalu(codegendata *cd, s4 alu_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->prev->regoff;
	s4 s2 = src->regoff;
	s4 d = iptr->dst->regoff;

	if (iptr->dst->flags & INMEMORY) {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (s2 == d) {
				x86_64_mov_membase_reg(cd, REG_SP, s1 * 8, REG_ITMP1);
				x86_64_alu_reg_membase(cd, alu_op, REG_ITMP1, REG_SP, d * 8);

			} else if (s1 == d) {
				x86_64_mov_membase_reg(cd, REG_SP, s2 * 8, REG_ITMP1);
				x86_64_alu_reg_membase(cd, alu_op, REG_ITMP1, REG_SP, d * 8);

			} else {
				x86_64_mov_membase_reg(cd, REG_SP, s1 * 8, REG_ITMP1);
				x86_64_alu_membase_reg(cd, alu_op, REG_SP, s2 * 8, REG_ITMP1);
				x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, d * 8);
			}

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			if (s2 == d) {
				x86_64_alu_reg_membase(cd, alu_op, s1, REG_SP, d * 8);

			} else {
				x86_64_mov_membase_reg(cd, REG_SP, s2 * 8, REG_ITMP1);
				x86_64_alu_reg_reg(cd, alu_op, s1, REG_ITMP1);
				x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, d * 8);
			}

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (s1 == d) {
				x86_64_alu_reg_membase(cd, alu_op, s2, REG_SP, d * 8);
						
			} else {
				x86_64_mov_membase_reg(cd, REG_SP, s1 * 8, REG_ITMP1);
				x86_64_alu_reg_reg(cd, alu_op, s2, REG_ITMP1);
				x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, d * 8);
			}

		} else {
			x86_64_mov_reg_membase(cd, s1, REG_SP, d * 8);
			x86_64_alu_reg_membase(cd, alu_op, s2, REG_SP, d * 8);
		}

	} else {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			x86_64_mov_membase_reg(cd, REG_SP, s1 * 8, d);
			x86_64_alu_membase_reg(cd, alu_op, REG_SP, s2 * 8, d);

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			M_INTMOVE(s1, d);
			x86_64_alu_membase_reg(cd, alu_op, REG_SP, s2 * 8, d);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			M_INTMOVE(s2, d);
			x86_64_alu_membase_reg(cd, alu_op, REG_SP, s1 * 8, d);

		} else {
			if (s2 == d) {
				x86_64_alu_reg_reg(cd, alu_op, s1, d);

			} else {
				M_INTMOVE(s1, d);
				x86_64_alu_reg_reg(cd, alu_op, s2, d);
			}
		}
	}
}


void x86_64_emit_ialuconst(codegendata *cd, s4 alu_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->regoff;
	s4 d = iptr->dst->regoff;

	if (iptr->dst->flags & INMEMORY) {
		if (src->flags & INMEMORY) {
			if (s1 == d) {
				x86_64_alul_imm_membase(cd, alu_op, iptr->val.i, REG_SP, d * 8);

			} else {
				x86_64_movl_membase_reg(cd, REG_SP, s1 * 8, REG_ITMP1);
				x86_64_alul_imm_reg(cd, alu_op, iptr->val.i, REG_ITMP1);
				x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, d * 8);
			}

		} else {
			x86_64_movl_reg_membase(cd, s1, REG_SP, d * 8);
			x86_64_alul_imm_membase(cd, alu_op, iptr->val.i, REG_SP, d * 8);
		}

	} else {
		if (src->flags & INMEMORY) {
			x86_64_movl_membase_reg(cd, REG_SP, s1 * 8, d);
			x86_64_alul_imm_reg(cd, alu_op, iptr->val.i, d);

		} else {
			M_INTMOVE(s1, d);
			x86_64_alul_imm_reg(cd, alu_op, iptr->val.i, d);
		}
	}
}


void x86_64_emit_laluconst(codegendata *cd, s4 alu_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->regoff;
	s4 d = iptr->dst->regoff;

	if (iptr->dst->flags & INMEMORY) {
		if (src->flags & INMEMORY) {
			if (s1 == d) {
				if (IS_IMM32(iptr->val.l)) {
					x86_64_alu_imm_membase(cd, alu_op, iptr->val.l, REG_SP, d * 8);

				} else {
					x86_64_mov_imm_reg(cd, iptr->val.l, REG_ITMP1);
					x86_64_alu_reg_membase(cd, alu_op, REG_ITMP1, REG_SP, d * 8);
				}

			} else {
				x86_64_mov_membase_reg(cd, REG_SP, s1 * 8, REG_ITMP1);

				if (IS_IMM32(iptr->val.l)) {
					x86_64_alu_imm_reg(cd, alu_op, iptr->val.l, REG_ITMP1);

				} else {
					x86_64_mov_imm_reg(cd, iptr->val.l, REG_ITMP2);
					x86_64_alu_reg_reg(cd, alu_op, REG_ITMP2, REG_ITMP1);
				}
				x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, d * 8);
			}

		} else {
			x86_64_mov_reg_membase(cd, s1, REG_SP, d * 8);

			if (IS_IMM32(iptr->val.l)) {
				x86_64_alu_imm_membase(cd, alu_op, iptr->val.l, REG_SP, d * 8);

			} else {
				x86_64_mov_imm_reg(cd, iptr->val.l, REG_ITMP1);
				x86_64_alu_reg_membase(cd, alu_op, REG_ITMP1, REG_SP, d * 8);
			}
		}

	} else {
		if (src->flags & INMEMORY) {
			x86_64_mov_membase_reg(cd, REG_SP, s1 * 8, d);

		} else {
			M_INTMOVE(s1, d);
		}

		if (IS_IMM32(iptr->val.l)) {
			x86_64_alu_imm_reg(cd, alu_op, iptr->val.l, d);

		} else {
			x86_64_mov_imm_reg(cd, iptr->val.l, REG_ITMP1);
			x86_64_alu_reg_reg(cd, alu_op, REG_ITMP1, d);
		}
	}
}


void x86_64_emit_ishift(codegendata *cd, s4 shift_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->prev->regoff;
	s4 s2 = src->regoff;
	s4 d = iptr->dst->regoff;
	s4 d_old;

	M_INTMOVE(RCX, REG_ITMP1);    /* save RCX */
	if (iptr->dst->flags & INMEMORY) {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (s1 == d) {
				x86_64_movl_membase_reg(cd, REG_SP, s2 * 8, RCX);
				x86_64_shiftl_membase(cd, shift_op, REG_SP, d * 8);

			} else {
				x86_64_movl_membase_reg(cd, REG_SP, s2 * 8, RCX);
				x86_64_movl_membase_reg(cd, REG_SP, s1 * 8, REG_ITMP2);
				x86_64_shiftl_reg(cd, shift_op, REG_ITMP2);
				x86_64_movl_reg_membase(cd, REG_ITMP2, REG_SP, d * 8);
			}

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			x86_64_movl_membase_reg(cd, REG_SP, s2 * 8, RCX);
			x86_64_movl_reg_membase(cd, s1, REG_SP, d * 8);
			x86_64_shiftl_membase(cd, shift_op, REG_SP, d * 8);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (s1 == d) {
				M_INTMOVE(s2, RCX);
				x86_64_shiftl_membase(cd, shift_op, REG_SP, d * 8);

			} else {
				M_INTMOVE(s2, RCX);
				x86_64_movl_membase_reg(cd, REG_SP, s1 * 8, REG_ITMP2);
				x86_64_shiftl_reg(cd, shift_op, REG_ITMP2);
				x86_64_movl_reg_membase(cd, REG_ITMP2, REG_SP, d * 8);
			}

		} else {
			M_INTMOVE(s2, RCX);
			x86_64_movl_reg_membase(cd, s1, REG_SP, d * 8);
			x86_64_shiftl_membase(cd, shift_op, REG_SP, d * 8);
		}
		M_INTMOVE(REG_ITMP1, RCX);    /* restore RCX */

	} else {
		if (d == RCX) {
			d_old = d;
			d = REG_ITMP3;
		}
					
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			x86_64_movl_membase_reg(cd, REG_SP, s2 * 8, RCX);
			x86_64_movl_membase_reg(cd, REG_SP, s1 * 8, d);
			x86_64_shiftl_reg(cd, shift_op, d);

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			M_INTMOVE(s1, d);    /* maybe src is RCX */
			x86_64_movl_membase_reg(cd, REG_SP, s2 * 8, RCX);
			x86_64_shiftl_reg(cd, shift_op, d);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			M_INTMOVE(s2, RCX);
			x86_64_movl_membase_reg(cd, REG_SP, s1 * 8, d);
			x86_64_shiftl_reg(cd, shift_op, d);

		} else {
			if (s1 == RCX) {
				M_INTMOVE(s1, d);
				M_INTMOVE(s2, RCX);

			} else {
				M_INTMOVE(s2, RCX);
				M_INTMOVE(s1, d);
			}
			x86_64_shiftl_reg(cd, shift_op, d);
		}

		if (d_old == RCX) {
			M_INTMOVE(REG_ITMP3, RCX);

		} else {
			M_INTMOVE(REG_ITMP1, RCX);    /* restore RCX */
		}
	}
}


void x86_64_emit_lshift(codegendata *cd, s4 shift_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->prev->regoff;
	s4 s2 = src->regoff;
	s4 d = iptr->dst->regoff;
	s4 d_old;
	
	d_old = -1;
	M_INTMOVE(RCX, REG_ITMP1);    /* save RCX */
	if (iptr->dst->flags & INMEMORY) {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (s1 == d) {
				x86_64_mov_membase_reg(cd, REG_SP, s2 * 8, RCX);
				x86_64_shift_membase(cd, shift_op, REG_SP, d * 8);

			} else {
				x86_64_mov_membase_reg(cd, REG_SP, s2 * 8, RCX);
				x86_64_mov_membase_reg(cd, REG_SP, s1 * 8, REG_ITMP2);
				x86_64_shift_reg(cd, shift_op, REG_ITMP2);
				x86_64_mov_reg_membase(cd, REG_ITMP2, REG_SP, d * 8);
			}

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			x86_64_mov_membase_reg(cd, REG_SP, s2 * 8, RCX);
			x86_64_mov_reg_membase(cd, s1, REG_SP, d * 8);
			x86_64_shift_membase(cd, shift_op, REG_SP, d * 8);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (s1 == d) {
				M_INTMOVE(s2, RCX);
				x86_64_shift_membase(cd, shift_op, REG_SP, d * 8);

			} else {
				M_INTMOVE(s2, RCX);
				x86_64_mov_membase_reg(cd, REG_SP, s1 * 8, REG_ITMP2);
				x86_64_shift_reg(cd, shift_op, REG_ITMP2);
				x86_64_mov_reg_membase(cd, REG_ITMP2, REG_SP, d * 8);
			}

		} else {
			M_INTMOVE(s2, RCX);
			x86_64_mov_reg_membase(cd, s1, REG_SP, d * 8);
			x86_64_shift_membase(cd, shift_op, REG_SP, d * 8);
		}
		M_INTMOVE(REG_ITMP1, RCX);    /* restore RCX */

	} else {
		if (d == RCX) {
			d_old = d;
			d = REG_ITMP3;
		}

		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			x86_64_mov_membase_reg(cd, REG_SP, s2 * 8, RCX);
			x86_64_mov_membase_reg(cd, REG_SP, s1 * 8, d);
			x86_64_shift_reg(cd, shift_op, d);

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			M_INTMOVE(s1, d);    /* maybe src is RCX */
			x86_64_mov_membase_reg(cd, REG_SP, s2 * 8, RCX);
			x86_64_shift_reg(cd, shift_op, d);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			M_INTMOVE(s2, RCX);
			x86_64_mov_membase_reg(cd, REG_SP, s1 * 8, d);
			x86_64_shift_reg(cd, shift_op, d);

		} else {
			if (s1 == RCX) {
				M_INTMOVE(s1, d);
				M_INTMOVE(s2, RCX);
			} else {
				M_INTMOVE(s2, RCX);
				M_INTMOVE(s1, d);
			}
			x86_64_shift_reg(cd, shift_op, d);
		}

		if (d_old == RCX) {
			M_INTMOVE(REG_ITMP3, RCX);

		} else {
			M_INTMOVE(REG_ITMP1, RCX);    /* restore RCX */
		}
	}
}


void x86_64_emit_ishiftconst(codegendata *cd, s4 shift_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->regoff;
	s4 d = iptr->dst->regoff;

	if ((src->flags & INMEMORY) && (iptr->dst->flags & INMEMORY)) {
		if (s1 == d) {
			x86_64_shiftl_imm_membase(cd, shift_op, iptr->val.i, REG_SP, d * 8);

		} else {
			x86_64_movl_membase_reg(cd, REG_SP, s1 * 8, REG_ITMP1);
			x86_64_shiftl_imm_reg(cd, shift_op, iptr->val.i, REG_ITMP1);
			x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, d * 8);
		}

	} else if ((src->flags & INMEMORY) && !(iptr->dst->flags & INMEMORY)) {
		x86_64_movl_membase_reg(cd, REG_SP, s1 * 8, d);
		x86_64_shiftl_imm_reg(cd, shift_op, iptr->val.i, d);
				
	} else if (!(src->flags & INMEMORY) && (iptr->dst->flags & INMEMORY)) {
		x86_64_movl_reg_membase(cd, s1, REG_SP, d * 8);
		x86_64_shiftl_imm_membase(cd, shift_op, iptr->val.i, REG_SP, d * 8);

	} else {
		M_INTMOVE(s1, d);
		x86_64_shiftl_imm_reg(cd, shift_op, iptr->val.i, d);
	}
}


void x86_64_emit_lshiftconst(codegendata *cd, s4 shift_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->regoff;
	s4 d = iptr->dst->regoff;

	if ((src->flags & INMEMORY) && (iptr->dst->flags & INMEMORY)) {
		if (s1 == d) {
			x86_64_shift_imm_membase(cd, shift_op, iptr->val.i, REG_SP, d * 8);

		} else {
			x86_64_mov_membase_reg(cd, REG_SP, s1 * 8, REG_ITMP1);
			x86_64_shift_imm_reg(cd, shift_op, iptr->val.i, REG_ITMP1);
			x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, d * 8);
		}

	} else if ((src->flags & INMEMORY) && !(iptr->dst->flags & INMEMORY)) {
		x86_64_mov_membase_reg(cd, REG_SP, s1 * 8, d);
		x86_64_shift_imm_reg(cd, shift_op, iptr->val.i, d);
				
	} else if (!(src->flags & INMEMORY) && (iptr->dst->flags & INMEMORY)) {
		x86_64_mov_reg_membase(cd, s1, REG_SP, d * 8);
		x86_64_shift_imm_membase(cd, shift_op, iptr->val.i, REG_SP, d * 8);

	} else {
		M_INTMOVE(s1, d);
		x86_64_shift_imm_reg(cd, shift_op, iptr->val.i, d);
	}
}


void x86_64_emit_ifcc(codegendata *cd, s4 if_op, stackptr src, instruction *iptr)
{
	if (src->flags & INMEMORY) {
		x86_64_alul_imm_membase(cd, X86_64_CMP, iptr->val.i, REG_SP, src->regoff * 8);

	} else {
		if (iptr->val.i == 0) {
			x86_64_testl_reg_reg(cd, src->regoff, src->regoff);

		} else {
			x86_64_alul_imm_reg(cd, X86_64_CMP, iptr->val.i, src->regoff);
		}
	}
	x86_64_jcc(cd, if_op, 0);
	codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
}


void x86_64_emit_if_lcc(codegendata *cd, s4 if_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->regoff;

	if (src->flags & INMEMORY) {
		if (IS_IMM32(iptr->val.l)) {
			x86_64_alu_imm_membase(cd, X86_64_CMP, iptr->val.l, REG_SP, s1 * 8);

		} else {
			x86_64_mov_imm_reg(cd, iptr->val.l, REG_ITMP1);
			x86_64_alu_reg_membase(cd, X86_64_CMP, REG_ITMP1, REG_SP, s1 * 8);
		}

	} else {
		if (iptr->val.l == 0) {
			x86_64_test_reg_reg(cd, s1, s1);

		} else {
			if (IS_IMM32(iptr->val.l)) {
				x86_64_alu_imm_reg(cd, X86_64_CMP, iptr->val.l, s1);

			} else {
				x86_64_mov_imm_reg(cd, iptr->val.l, REG_ITMP1);
				x86_64_alu_reg_reg(cd, X86_64_CMP, REG_ITMP1, s1);
			}
		}
	}
	x86_64_jcc(cd, if_op, 0);
	codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
}


void x86_64_emit_if_icmpcc(codegendata *cd, s4 if_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->prev->regoff;
	s4 s2 = src->regoff;

	if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
		x86_64_movl_membase_reg(cd, REG_SP, s2 * 8, REG_ITMP1);
		x86_64_alul_reg_membase(cd, X86_64_CMP, REG_ITMP1, REG_SP, s1 * 8);

	} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
		x86_64_alul_membase_reg(cd, X86_64_CMP, REG_SP, s2 * 8, s1);

	} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
		x86_64_alul_reg_membase(cd, X86_64_CMP, s2, REG_SP, s1 * 8);

	} else {
		x86_64_alul_reg_reg(cd, X86_64_CMP, s2, s1);
	}
	x86_64_jcc(cd, if_op, 0);
	codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
}


void x86_64_emit_if_lcmpcc(codegendata *cd, s4 if_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->prev->regoff;
	s4 s2 = src->regoff;

	if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
		x86_64_mov_membase_reg(cd, REG_SP, s2 * 8, REG_ITMP1);
		x86_64_alu_reg_membase(cd, X86_64_CMP, REG_ITMP1, REG_SP, s1 * 8);

	} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
		x86_64_alu_membase_reg(cd, X86_64_CMP, REG_SP, s2 * 8, s1);

	} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
		x86_64_alu_reg_membase(cd, X86_64_CMP, s2, REG_SP, s1 * 8);

	} else {
		x86_64_alu_reg_reg(cd, X86_64_CMP, s2, s1);
	}
	x86_64_jcc(cd, if_op, 0);
	codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
}


/*
 * mov ops
 */
void x86_64_mov_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	x86_64_emit_rex(1,(reg),0,(dreg));
	*(cd->mcodeptr++) = 0x89;
	x86_64_emit_reg((reg),(dreg));
}


void x86_64_mov_imm_reg(codegendata *cd, s8 imm, s8 reg) {
	x86_64_emit_rex(1,0,0,(reg));
	*(cd->mcodeptr++) = 0xb8 + ((reg) & 0x07);
	x86_64_emit_imm64((imm));
}


void x86_64_movl_imm_reg(codegendata *cd, s8 imm, s8 reg) {
	x86_64_emit_rex(0,0,0,(reg));
	*(cd->mcodeptr++) = 0xb8 + ((reg) & 0x07);
	x86_64_emit_imm32((imm));
}


void x86_64_mov_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 reg) {
	x86_64_emit_rex(1,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x8b;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_movl_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 reg) {
	x86_64_emit_rex(0,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x8b;
	x86_64_emit_membase((basereg),(disp),(reg));
}


/*
 * this one is for INVOKEVIRTUAL/INVOKEINTERFACE to have a
 * constant membase immediate length of 32bit
 */
void x86_64_mov_membase32_reg(codegendata *cd, s8 basereg, s8 disp, s8 reg) {
	x86_64_emit_rex(1,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x8b;
	x86_64_address_byte(2, (reg), (basereg));
	x86_64_emit_imm32((disp));
}


void x86_64_mov_reg_membase(codegendata *cd, s8 reg, s8 basereg, s8 disp) {
	x86_64_emit_rex(1,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x89;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_movl_reg_membase(codegendata *cd, s8 reg, s8 basereg, s8 disp) {
	x86_64_emit_rex(0,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x89;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_mov_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg) {
	x86_64_emit_rex(1,(reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x8b;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_movl_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg) {
	x86_64_emit_rex(0,(reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x8b;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_mov_reg_memindex(codegendata *cd, s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale) {
	x86_64_emit_rex(1,(reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x89;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_movl_reg_memindex(codegendata *cd, s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale) {
	x86_64_emit_rex(0,(reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x89;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_movw_reg_memindex(codegendata *cd, s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale) {
	*(cd->mcodeptr++) = 0x66;
	x86_64_emit_rex(0,(reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x89;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_movb_reg_memindex(codegendata *cd, s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale) {
	x86_64_emit_byte_rex((reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x88;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_mov_imm_membase(codegendata *cd, s8 imm, s8 basereg, s8 disp) {
	x86_64_emit_rex(1,0,0,(basereg));
	*(cd->mcodeptr++) = 0xc7;
	x86_64_emit_membase((basereg),(disp),0);
	x86_64_emit_imm32((imm));
}


void x86_64_movl_imm_membase(codegendata *cd, s8 imm, s8 basereg, s8 disp) {
	x86_64_emit_rex(0,0,0,(basereg));
	*(cd->mcodeptr++) = 0xc7;
	x86_64_emit_membase((basereg),(disp),0);
	x86_64_emit_imm32((imm));
}


void x86_64_movsbq_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	x86_64_emit_rex(1,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xbe;
	/* XXX: why do reg and dreg have to be exchanged */
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_movsbq_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	x86_64_emit_rex(1,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xbe;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_movswq_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	x86_64_emit_rex(1,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xbf;
	/* XXX: why do reg and dreg have to be exchanged */
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_movswq_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	x86_64_emit_rex(1,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xbf;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_movslq_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	x86_64_emit_rex(1,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x63;
	/* XXX: why do reg and dreg have to be exchanged */
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_movslq_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	x86_64_emit_rex(1,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x63;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_movzwq_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	x86_64_emit_rex(1,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xb7;
	/* XXX: why do reg and dreg have to be exchanged */
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_movzwq_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	x86_64_emit_rex(1,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xb7;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_movswq_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg) {
	x86_64_emit_rex(1,(reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xbf;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_movsbq_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg) {
	x86_64_emit_rex(1,(reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xbe;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_movzwq_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg) {
	x86_64_emit_rex(1,(reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xb7;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_mov_imm_memindex(codegendata *cd, s4 imm, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	x86_64_emit_rex(1,0,(indexreg),(basereg));
	*(cd->mcodeptr++) = 0xc7;
	x86_64_emit_memindex(0,(disp),(basereg),(indexreg),(scale));
	x86_64_emit_imm32((imm));
}


void x86_64_movl_imm_memindex(codegendata *cd, s4 imm, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	x86_64_emit_rex(0,0,(indexreg),(basereg));
	*(cd->mcodeptr++) = 0xc7;
	x86_64_emit_memindex(0,(disp),(basereg),(indexreg),(scale));
	x86_64_emit_imm32((imm));
}


void x86_64_movw_imm_memindex(codegendata *cd, s4 imm, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	*(cd->mcodeptr++) = 0x66;
	x86_64_emit_rex(0,0,(indexreg),(basereg));
	*(cd->mcodeptr++) = 0xc7;
	x86_64_emit_memindex(0,(disp),(basereg),(indexreg),(scale));
	x86_64_emit_imm16((imm));
}


void x86_64_movb_imm_memindex(codegendata *cd, s4 imm, s4 disp, s4 basereg, s4 indexreg, s4 scale)
{
	x86_64_emit_rex(0,0,(indexreg),(basereg));
	*(cd->mcodeptr++) = 0xc6;
	x86_64_emit_memindex(0,(disp),(basereg),(indexreg),(scale));
	x86_64_emit_imm8((imm));
}


/*
 * alu operations
 */
void x86_64_alu_reg_reg(codegendata *cd, s8 opc, s8 reg, s8 dreg) {
	x86_64_emit_rex(1,(reg),0,(dreg));
	*(cd->mcodeptr++) = (((opc)) << 3) + 1;
	x86_64_emit_reg((reg),(dreg));
}


void x86_64_alul_reg_reg(codegendata *cd, s8 opc, s8 reg, s8 dreg) {
	x86_64_emit_rex(0,(reg),0,(dreg));
	*(cd->mcodeptr++) = (((opc)) << 3) + 1;
	x86_64_emit_reg((reg),(dreg));
}


void x86_64_alu_reg_membase(codegendata *cd, s8 opc, s8 reg, s8 basereg, s8 disp) {
	x86_64_emit_rex(1,(reg),0,(basereg));
	*(cd->mcodeptr++) = (((opc)) << 3) + 1;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_alul_reg_membase(codegendata *cd, s8 opc, s8 reg, s8 basereg, s8 disp) {
	x86_64_emit_rex(0,(reg),0,(basereg));
	*(cd->mcodeptr++) = (((opc)) << 3) + 1;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_alu_membase_reg(codegendata *cd, s8 opc, s8 basereg, s8 disp, s8 reg) {
	x86_64_emit_rex(1,(reg),0,(basereg));
	*(cd->mcodeptr++) = (((opc)) << 3) + 3;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_alul_membase_reg(codegendata *cd, s8 opc, s8 basereg, s8 disp, s8 reg) {
	x86_64_emit_rex(0,(reg),0,(basereg));
	*(cd->mcodeptr++) = (((opc)) << 3) + 3;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_alu_imm_reg(codegendata *cd, s8 opc, s8 imm, s8 dreg) {
	if (IS_IMM8(imm)) {
		x86_64_emit_rex(1,0,0,(dreg));
		*(cd->mcodeptr++) = 0x83;
		x86_64_emit_reg((opc),(dreg));
		x86_64_emit_imm8((imm));
	} else {
		x86_64_emit_rex(1,0,0,(dreg));
		*(cd->mcodeptr++) = 0x81;
		x86_64_emit_reg((opc),(dreg));
		x86_64_emit_imm32((imm));
	}
}


void x86_64_alul_imm_reg(codegendata *cd, s8 opc, s8 imm, s8 dreg) {
	if (IS_IMM8(imm)) {
		x86_64_emit_rex(0,0,0,(dreg));
		*(cd->mcodeptr++) = 0x83;
		x86_64_emit_reg((opc),(dreg));
		x86_64_emit_imm8((imm));
	} else {
		x86_64_emit_rex(0,0,0,(dreg));
		*(cd->mcodeptr++) = 0x81;
		x86_64_emit_reg((opc),(dreg));
		x86_64_emit_imm32((imm));
	}
}


void x86_64_alu_imm_membase(codegendata *cd, s8 opc, s8 imm, s8 basereg, s8 disp) {
	if (IS_IMM8(imm)) {
		x86_64_emit_rex(1,(basereg),0,0);
		*(cd->mcodeptr++) = 0x83;
		x86_64_emit_membase((basereg),(disp),(opc));
		x86_64_emit_imm8((imm));
	} else {
		x86_64_emit_rex(1,(basereg),0,0);
		*(cd->mcodeptr++) = 0x81;
		x86_64_emit_membase((basereg),(disp),(opc));
		x86_64_emit_imm32((imm));
	}
}


void x86_64_alul_imm_membase(codegendata *cd, s8 opc, s8 imm, s8 basereg, s8 disp) {
	if (IS_IMM8(imm)) {
		x86_64_emit_rex(0,(basereg),0,0);
		*(cd->mcodeptr++) = 0x83;
		x86_64_emit_membase((basereg),(disp),(opc));
		x86_64_emit_imm8((imm));
	} else {
		x86_64_emit_rex(0,(basereg),0,0);
		*(cd->mcodeptr++) = 0x81;
		x86_64_emit_membase((basereg),(disp),(opc));
		x86_64_emit_imm32((imm));
	}
}


void x86_64_test_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	x86_64_emit_rex(1,(reg),0,(dreg));
	*(cd->mcodeptr++) = 0x85;
	x86_64_emit_reg((reg),(dreg));
}


void x86_64_testl_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	x86_64_emit_rex(0,(reg),0,(dreg));
	*(cd->mcodeptr++) = 0x85;
	x86_64_emit_reg((reg),(dreg));
}


void x86_64_test_imm_reg(codegendata *cd, s8 imm, s8 reg) {
	*(cd->mcodeptr++) = 0xf7;
	x86_64_emit_reg(0,(reg));
	x86_64_emit_imm32((imm));
}


void x86_64_testw_imm_reg(codegendata *cd, s8 imm, s8 reg) {
	*(cd->mcodeptr++) = 0x66;
	*(cd->mcodeptr++) = 0xf7;
	x86_64_emit_reg(0,(reg));
	x86_64_emit_imm16((imm));
}


void x86_64_testb_imm_reg(codegendata *cd, s8 imm, s8 reg) {
	*(cd->mcodeptr++) = 0xf6;
	x86_64_emit_reg(0,(reg));
	x86_64_emit_imm8((imm));
}


void x86_64_lea_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 reg) {
	x86_64_emit_rex(1,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x8d;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_leal_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 reg) {
	x86_64_emit_rex(0,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x8d;
	x86_64_emit_membase((basereg),(disp),(reg));
}



/*
 * inc, dec operations
 */
void x86_64_inc_reg(codegendata *cd, s8 reg) {
	x86_64_emit_rex(1,0,0,(reg));
	*(cd->mcodeptr++) = 0xff;
	x86_64_emit_reg(0,(reg));
}


void x86_64_incl_reg(codegendata *cd, s8 reg) {
	x86_64_emit_rex(0,0,0,(reg));
	*(cd->mcodeptr++) = 0xff;
	x86_64_emit_reg(0,(reg));
}


void x86_64_inc_membase(codegendata *cd, s8 basereg, s8 disp) {
	x86_64_emit_rex(1,(basereg),0,0);
	*(cd->mcodeptr++) = 0xff;
	x86_64_emit_membase((basereg),(disp),0);
}


void x86_64_incl_membase(codegendata *cd, s8 basereg, s8 disp) {
	x86_64_emit_rex(0,(basereg),0,0);
	*(cd->mcodeptr++) = 0xff;
	x86_64_emit_membase((basereg),(disp),0);
}


void x86_64_dec_reg(codegendata *cd, s8 reg) {
	x86_64_emit_rex(1,0,0,(reg));
	*(cd->mcodeptr++) = 0xff;
	x86_64_emit_reg(1,(reg));
}

        
void x86_64_decl_reg(codegendata *cd, s8 reg) {
	x86_64_emit_rex(0,0,0,(reg));
	*(cd->mcodeptr++) = 0xff;
	x86_64_emit_reg(1,(reg));
}

        
void x86_64_dec_membase(codegendata *cd, s8 basereg, s8 disp) {
	x86_64_emit_rex(1,(basereg),0,0);
	*(cd->mcodeptr++) = 0xff;
	x86_64_emit_membase((basereg),(disp),1);
}


void x86_64_decl_membase(codegendata *cd, s8 basereg, s8 disp) {
	x86_64_emit_rex(0,(basereg),0,0);
	*(cd->mcodeptr++) = 0xff;
	x86_64_emit_membase((basereg),(disp),1);
}




void x86_64_cltd(codegendata *cd) {
    *(cd->mcodeptr++) = 0x99;
}


void x86_64_cqto(codegendata *cd) {
	x86_64_emit_rex(1,0,0,0);
	*(cd->mcodeptr++) = 0x99;
}



void x86_64_imul_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	x86_64_emit_rex(1,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xaf;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_imull_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xaf;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_imul_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	x86_64_emit_rex(1,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xaf;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_imull_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	x86_64_emit_rex(0,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xaf;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_imul_imm_reg(codegendata *cd, s8 imm, s8 dreg) {
	if (IS_IMM8((imm))) {
		x86_64_emit_rex(1,0,0,(dreg));
		*(cd->mcodeptr++) = 0x6b;
		x86_64_emit_reg(0,(dreg));
		x86_64_emit_imm8((imm));
	} else {
		x86_64_emit_rex(1,0,0,(dreg));
		*(cd->mcodeptr++) = 0x69;
		x86_64_emit_reg(0,(dreg));
		x86_64_emit_imm32((imm));
	}
}


void x86_64_imul_imm_reg_reg(codegendata *cd, s8 imm, s8 reg, s8 dreg) {
	if (IS_IMM8((imm))) {
		x86_64_emit_rex(1,(dreg),0,(reg));
		*(cd->mcodeptr++) = 0x6b;
		x86_64_emit_reg((dreg),(reg));
		x86_64_emit_imm8((imm));
	} else {
		x86_64_emit_rex(1,(dreg),0,(reg));
		*(cd->mcodeptr++) = 0x69;
		x86_64_emit_reg((dreg),(reg));
		x86_64_emit_imm32((imm));
	}
}


void x86_64_imull_imm_reg_reg(codegendata *cd, s8 imm, s8 reg, s8 dreg) {
	if (IS_IMM8((imm))) {
		x86_64_emit_rex(0,(dreg),0,(reg));
		*(cd->mcodeptr++) = 0x6b;
		x86_64_emit_reg((dreg),(reg));
		x86_64_emit_imm8((imm));
	} else {
		x86_64_emit_rex(0,(dreg),0,(reg));
		*(cd->mcodeptr++) = 0x69;
		x86_64_emit_reg((dreg),(reg));
		x86_64_emit_imm32((imm));
	}
}


void x86_64_imul_imm_membase_reg(codegendata *cd, s8 imm, s8 basereg, s8 disp, s8 dreg) {
	if (IS_IMM8((imm))) {
		x86_64_emit_rex(1,(dreg),0,(basereg));
		*(cd->mcodeptr++) = 0x6b;
		x86_64_emit_membase((basereg),(disp),(dreg));
		x86_64_emit_imm8((imm));
	} else {
		x86_64_emit_rex(1,(dreg),0,(basereg));
		*(cd->mcodeptr++) = 0x69;
		x86_64_emit_membase((basereg),(disp),(dreg));
		x86_64_emit_imm32((imm));
	}
}


void x86_64_imull_imm_membase_reg(codegendata *cd, s8 imm, s8 basereg, s8 disp, s8 dreg) {
	if (IS_IMM8((imm))) {
		x86_64_emit_rex(0,(dreg),0,(basereg));
		*(cd->mcodeptr++) = 0x6b;
		x86_64_emit_membase((basereg),(disp),(dreg));
		x86_64_emit_imm8((imm));
	} else {
		x86_64_emit_rex(0,(dreg),0,(basereg));
		*(cd->mcodeptr++) = 0x69;
		x86_64_emit_membase((basereg),(disp),(dreg));
		x86_64_emit_imm32((imm));
	}
}


void x86_64_idiv_reg(codegendata *cd, s8 reg) {
	x86_64_emit_rex(1,0,0,(reg));
	*(cd->mcodeptr++) = 0xf7;
	x86_64_emit_reg(7,(reg));
}


void x86_64_idivl_reg(codegendata *cd, s8 reg) {
	x86_64_emit_rex(0,0,0,(reg));
	*(cd->mcodeptr++) = 0xf7;
	x86_64_emit_reg(7,(reg));
}



void x86_64_ret(codegendata *cd) {
    *(cd->mcodeptr++) = 0xc3;
}



/*
 * shift ops
 */
void x86_64_shift_reg(codegendata *cd, s8 opc, s8 reg) {
	x86_64_emit_rex(1,0,0,(reg));
	*(cd->mcodeptr++) = 0xd3;
	x86_64_emit_reg((opc),(reg));
}


void x86_64_shiftl_reg(codegendata *cd, s8 opc, s8 reg) {
	x86_64_emit_rex(0,0,0,(reg));
	*(cd->mcodeptr++) = 0xd3;
	x86_64_emit_reg((opc),(reg));
}


void x86_64_shift_membase(codegendata *cd, s8 opc, s8 basereg, s8 disp) {
	x86_64_emit_rex(1,0,0,(basereg));
	*(cd->mcodeptr++) = 0xd3;
	x86_64_emit_membase((basereg),(disp),(opc));
}


void x86_64_shiftl_membase(codegendata *cd, s8 opc, s8 basereg, s8 disp) {
	x86_64_emit_rex(0,0,0,(basereg));
	*(cd->mcodeptr++) = 0xd3;
	x86_64_emit_membase((basereg),(disp),(opc));
}


void x86_64_shift_imm_reg(codegendata *cd, s8 opc, s8 imm, s8 dreg) {
	if ((imm) == 1) {
		x86_64_emit_rex(1,0,0,(dreg));
		*(cd->mcodeptr++) = 0xd1;
		x86_64_emit_reg((opc),(dreg));
	} else {
		x86_64_emit_rex(1,0,0,(dreg));
		*(cd->mcodeptr++) = 0xc1;
		x86_64_emit_reg((opc),(dreg));
		x86_64_emit_imm8((imm));
	}
}


void x86_64_shiftl_imm_reg(codegendata *cd, s8 opc, s8 imm, s8 dreg) {
	if ((imm) == 1) {
		x86_64_emit_rex(0,0,0,(dreg));
		*(cd->mcodeptr++) = 0xd1;
		x86_64_emit_reg((opc),(dreg));
	} else {
		x86_64_emit_rex(0,0,0,(dreg));
		*(cd->mcodeptr++) = 0xc1;
		x86_64_emit_reg((opc),(dreg));
		x86_64_emit_imm8((imm));
	}
}


void x86_64_shift_imm_membase(codegendata *cd, s8 opc, s8 imm, s8 basereg, s8 disp) {
	if ((imm) == 1) {
		x86_64_emit_rex(1,0,0,(basereg));
		*(cd->mcodeptr++) = 0xd1;
		x86_64_emit_membase((basereg),(disp),(opc));
	} else {
		x86_64_emit_rex(1,0,0,(basereg));
		*(cd->mcodeptr++) = 0xc1;
		x86_64_emit_membase((basereg),(disp),(opc));
		x86_64_emit_imm8((imm));
	}
}


void x86_64_shiftl_imm_membase(codegendata *cd, s8 opc, s8 imm, s8 basereg, s8 disp) {
	if ((imm) == 1) {
		x86_64_emit_rex(0,0,0,(basereg));
		*(cd->mcodeptr++) = 0xd1;
		x86_64_emit_membase((basereg),(disp),(opc));
	} else {
		x86_64_emit_rex(0,0,0,(basereg));
		*(cd->mcodeptr++) = 0xc1;
		x86_64_emit_membase((basereg),(disp),(opc));
		x86_64_emit_imm8((imm));
	}
}



/*
 * jump operations
 */
void x86_64_jmp_imm(codegendata *cd, s8 imm) {
	*(cd->mcodeptr++) = 0xe9;
	x86_64_emit_imm32((imm));
}


void x86_64_jmp_reg(codegendata *cd, s8 reg) {
	x86_64_emit_rex(0,0,0,(reg));
	*(cd->mcodeptr++) = 0xff;
	x86_64_emit_reg(4,(reg));
}


void x86_64_jcc(codegendata *cd, s8 opc, s8 imm) {
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = (0x80 + (opc));
	x86_64_emit_imm32((imm));
}



/*
 * conditional set and move operations
 */

/* we need the rex byte to get all low bytes */
void x86_64_setcc_reg(codegendata *cd, s8 opc, s8 reg) {
	*(cd->mcodeptr++) = (0x40 | (((reg) >> 3) & 0x01));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = (0x90 + (opc));
	x86_64_emit_reg(0,(reg));
}


/* we need the rex byte to get all low bytes */
void x86_64_setcc_membase(codegendata *cd, s8 opc, s8 basereg, s8 disp) {
	*(cd->mcodeptr++) = (0x40 | (((basereg) >> 3) & 0x01));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = (0x90 + (opc));
	x86_64_emit_membase((basereg),(disp),0);
}


void x86_64_cmovcc_reg_reg(codegendata *cd, s8 opc, s8 reg, s8 dreg) {
	x86_64_emit_rex(1,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = (0x40 + (opc));
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_cmovccl_reg_reg(codegendata *cd, s8 opc, s8 reg, s8 dreg) {
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = (0x40 + (opc));
	x86_64_emit_reg((dreg),(reg));
}



void x86_64_neg_reg(codegendata *cd, s8 reg) {
	x86_64_emit_rex(1,0,0,(reg));
	*(cd->mcodeptr++) = 0xf7;
	x86_64_emit_reg(3,(reg));
}


void x86_64_negl_reg(codegendata *cd, s8 reg) {
	x86_64_emit_rex(0,0,0,(reg));
	*(cd->mcodeptr++) = 0xf7;
	x86_64_emit_reg(3,(reg));
}


void x86_64_neg_membase(codegendata *cd, s8 basereg, s8 disp) {
	x86_64_emit_rex(1,0,0,(basereg));
	*(cd->mcodeptr++) = 0xf7;
	x86_64_emit_membase((basereg),(disp),3);
}


void x86_64_negl_membase(codegendata *cd, s8 basereg, s8 disp) {
	x86_64_emit_rex(0,0,0,(basereg));
	*(cd->mcodeptr++) = 0xf7;
	x86_64_emit_membase((basereg),(disp),3);
}


void x86_64_push_reg(codegendata *cd, s8 reg) {
	x86_64_emit_rex(0,0,0,(reg));
	*(cd->mcodeptr++) = 0x50 + (0x07 & (reg));
}


void x86_64_push_imm(codegendata *cd, s8 imm) {
	*(cd->mcodeptr++) = 0x68;
	x86_64_emit_imm32((imm));
}


void x86_64_pop_reg(codegendata *cd, s8 reg) {
	x86_64_emit_rex(0,0,0,(reg));
	*(cd->mcodeptr++) = 0x58 + (0x07 & (reg));
}


void x86_64_xchg_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	x86_64_emit_rex(1,(reg),0,(dreg));
	*(cd->mcodeptr++) = 0x87;
	x86_64_emit_reg((reg),(dreg));
}


void x86_64_nop(codegendata *cd) {
    *(cd->mcodeptr++) = 0x90;
}



/*
 * call instructions
 */
void x86_64_call_reg(codegendata *cd, s8 reg) {
	x86_64_emit_rex(1,0,0,(reg));
	*(cd->mcodeptr++) = 0xff;
	x86_64_emit_reg(2,(reg));
}


void x86_64_call_imm(codegendata *cd, s8 imm) {
	*(cd->mcodeptr++) = 0xe8;
	x86_64_emit_imm32((imm));
}


void x86_64_call_mem(codegendata *cd, s8 mem) {
	*(cd->mcodeptr++) = 0xff;
	x86_64_emit_mem(2,(mem));
}



/*
 * floating point instructions (SSE2)
 */
void x86_64_addsd_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x58;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_addss_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x58;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_cvtsi2ssq_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	x86_64_emit_rex(1,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x2a;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_cvtsi2ss_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x2a;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_cvtsi2sdq_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	x86_64_emit_rex(1,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x2a;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_cvtsi2sd_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x2a;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_cvtss2sd_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x5a;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_cvtsd2ss_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x5a;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_cvttss2siq_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	x86_64_emit_rex(1,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x2c;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_cvttss2si_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x2c;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_cvttsd2siq_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	x86_64_emit_rex(1,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x2c;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_cvttsd2si_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x2c;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_divss_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x5e;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_divsd_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x5e;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_movd_reg_freg(codegendata *cd, s8 reg, s8 freg) {
	*(cd->mcodeptr++) = 0x66;
	x86_64_emit_rex(1,(freg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x6e;
	x86_64_emit_reg((freg),(reg));
}


void x86_64_movd_freg_reg(codegendata *cd, s8 freg, s8 reg) {
	*(cd->mcodeptr++) = 0x66;
	x86_64_emit_rex(1,(freg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x7e;
	x86_64_emit_reg((freg),(reg));
}


void x86_64_movd_reg_membase(codegendata *cd, s8 reg, s8 basereg, s8 disp) {
	*(cd->mcodeptr++) = 0x66;
	x86_64_emit_rex(0,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x7e;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_movd_reg_memindex(codegendata *cd, s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale) {
	*(cd->mcodeptr++) = 0x66;
	x86_64_emit_rex(0,(reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x7e;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_movd_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	*(cd->mcodeptr++) = 0x66;
	x86_64_emit_rex(1,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x6e;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_movdl_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	*(cd->mcodeptr++) = 0x66;
	x86_64_emit_rex(0,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x6e;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_movd_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 dreg) {
	*(cd->mcodeptr++) = 0x66;
	x86_64_emit_rex(0,(dreg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x6e;
	x86_64_emit_memindex((dreg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_movq_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x7e;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_movq_reg_membase(codegendata *cd, s8 reg, s8 basereg, s8 disp) {
	*(cd->mcodeptr++) = 0x66;
	x86_64_emit_rex(0,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0xd6;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_movq_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x7e;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_movss_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(reg),0,(dreg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x10;
	x86_64_emit_reg((reg),(dreg));
}


void x86_64_movsd_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(reg),0,(dreg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x10;
	x86_64_emit_reg((reg),(dreg));
}


void x86_64_movss_reg_membase(codegendata *cd, s8 reg, s8 basereg, s8 disp) {
	*(cd->mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x11;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_movsd_reg_membase(codegendata *cd, s8 reg, s8 basereg, s8 disp) {
	*(cd->mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(reg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x11;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_movss_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x10;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_movlps_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	x86_64_emit_rex(0,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x12;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_movsd_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x10;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_movlpd_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	*(cd->mcodeptr++) = 0x66;
	x86_64_emit_rex(0,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x12;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_movss_reg_memindex(codegendata *cd, s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale) {
	*(cd->mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x11;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_movsd_reg_memindex(codegendata *cd, s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale) {
	*(cd->mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(reg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x11;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_movss_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(dreg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x10;
	x86_64_emit_memindex((dreg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_movsd_memindex_reg(codegendata *cd, s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(dreg),(indexreg),(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x10;
	x86_64_emit_memindex((dreg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_mulss_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x59;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_mulsd_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x59;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_subss_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x5c;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_subsd_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x5c;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_ucomiss_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x2e;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_ucomisd_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0x66;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x2e;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_xorps_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x57;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_xorps_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	x86_64_emit_rex(0,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x57;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_xorpd_reg_reg(codegendata *cd, s8 reg, s8 dreg) {
	*(cd->mcodeptr++) = 0x66;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x57;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_xorpd_membase_reg(codegendata *cd, s8 basereg, s8 disp, s8 dreg) {
	*(cd->mcodeptr++) = 0x66;
	x86_64_emit_rex(0,(dreg),0,(basereg));
	*(cd->mcodeptr++) = 0x0f;
	*(cd->mcodeptr++) = 0x57;
	x86_64_emit_membase((basereg),(disp),(dreg));
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
