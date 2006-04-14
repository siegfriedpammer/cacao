/* src/vm/jit/mips/emit.c - MIPS code emitter functions

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

   $Id: emitfuncs.c 4398 2006-01-31 23:43:08Z twisti $

*/


#include "vm/types.h"

#include "md-abi.h"

#include "vm/jit/emit.h"
#include "vm/jit/jit.h"
#include "vm/jit/mips/codegen.h"


/* code generation functions **************************************************/

/* emit_load_s1 ****************************************************************

   Emits a possible load of the first source operand.

*******************************************************************************/

s4 emit_load_s1(jitdata *jd, instruction *iptr, stackptr src, s4 tempreg)
{
	codegendata  *cd;
	s4            disp;
	s4            reg;

	/* get required compiler data */

	cd = jd->cd;

	if (src->flags & INMEMORY) {
		COUNT_SPILLS;

		disp = src->regoff * 8;

		if (IS_FLT_DBL_TYPE(src->type))
			M_DLD(tempreg, REG_SP, disp);
		else
			M_LLD(tempreg, REG_SP, disp);

		reg = tempreg;
	} else
		reg = src->regoff;

	return reg;
}


/* emit_load_s2 ****************************************************************

   Emits a possible load of the second source operand.

*******************************************************************************/

s4 emit_load_s2(jitdata *jd, instruction *iptr, stackptr src, s4 tempreg)
{
	codegendata  *cd;
	s4            disp;
	s4            reg;

	/* get required compiler data */

	cd = jd->cd;

	if (src->flags & INMEMORY) {
		COUNT_SPILLS;

		disp = src->regoff * 8;

		if (IS_FLT_DBL_TYPE(src->type))
			M_DLD(tempreg, REG_SP, disp);
		else
			M_LLD(tempreg, REG_SP, disp);

		reg = tempreg;
	} else
		reg = src->regoff;

	return reg;
}


/* emit_load_s3 ****************************************************************

   Emits a possible load of the third source operand.

*******************************************************************************/

s4 emit_load_s3(jitdata *jd, instruction *iptr, stackptr src, s4 tempreg)
{
	codegendata  *cd;
	s4            disp;
	s4            reg;

	/* get required compiler data */

	cd = jd->cd;

	if (src->flags & INMEMORY) {
		COUNT_SPILLS;

		disp = src->regoff * 8;

		if (IS_FLT_DBL_TYPE(src->type))
			M_DLD(tempreg, REG_SP, disp);
		else
			M_LLD(tempreg, REG_SP, disp);

		reg = tempreg;
	} else
		reg = src->regoff;

	return reg;
}


/* emit_store ******************************************************************

   XXX

*******************************************************************************/

void emit_store(jitdata *jd, instruction *iptr, stackptr dst, s4 d)
{
	codegendata  *cd;
	s4            disp;

	/* get required compiler data */

	cd = jd->cd;

	if (dst->flags & INMEMORY) {
		COUNT_SPILLS;

		disp = dst->regoff * 8;

		if (IS_FLT_DBL_TYPE(dst->type))
			M_DST(d, REG_SP, disp);
		else
			M_LST(d, REG_SP, disp);
	}
}


/* emit_copy *******************************************************************

   XXX

*******************************************************************************/

void emit_copy(jitdata *jd, instruction *iptr, stackptr src, stackptr dst)
{
	codegendata  *cd;
	registerdata *rd;
	s4            s1, d;

	/* get required compiler data */

	cd = jd->cd;
	rd = jd->rd;

	if ((src->regoff != dst->regoff) ||
		((src->flags ^ dst->flags) & INMEMORY)) {
		d = codegen_reg_of_var(rd, iptr->opc, dst, REG_IFTMP);
		s1 = emit_load_s1(jd, iptr, src, d);

		if (s1 != d) {
			if (IS_FLT_DBL_TYPE(src->type)) {
				if (IS_2_WORD_TYPE(src->type))
					M_DMOV(s1, d);
				else
					M_FMOV(s1, d);
			} else
				M_MOV(s1, d);
		}

		emit_store(jd, iptr, dst, d);
	}
}


/* emit_iconst *****************************************************************

   XXX

*******************************************************************************/

void emit_iconst(codegendata *cd, s4 d, s4 value)
{
	s4 disp;

    if ((value >= -32768) && (value <= 32767))
        M_IADD_IMM(REG_ZERO, value, d);
	else if ((value >= 0) && (value <= 0xffff))
        M_OR_IMM(REG_ZERO, value, d);
	else {
        disp = dseg_adds4(cd, value);
        M_ILD(d, REG_PV, disp);
    }
}


/* emit_lconst *****************************************************************

   XXX

*******************************************************************************/

void emit_lconst(codegendata *cd, s4 d, s8 value)
{
	s4 disp;

	if ((value >= -32768) && (value <= 32767))
		M_LADD_IMM(REG_ZERO, value, d);
	else if ((value >= 0) && (value <= 0xffff))
		M_OR_IMM(REG_ZERO, value, d);
	else {
		disp = dseg_adds8(cd, value);
		M_LLD(d, REG_PV, disp);
	}
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
 * vim:noexpandtab:sw=4:ts=4:
 */
