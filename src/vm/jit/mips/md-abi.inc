/* src/vm/jit/mips/md-abi.inc - stuff for MIPS ABI

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

   $Id: md-abi.inc 3927 2005-12-09 11:32:47Z twisti $

*/


#ifndef _MD_ABI_INC
#define _MD_ABI_INC

#include "config.h"

#include "vm/jit/jit.h"


/* register descripton array **************************************************/

#if SIZEOF_VOID_P == 8

/* MIPS64 */

static int nregdescint[] = {
	REG_RES, REG_RES, REG_RET, REG_RES, REG_ARG, REG_ARG, REG_ARG, REG_ARG,
	REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_TMP, REG_TMP, REG_TMP, REG_TMP,
	REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV,
	REG_TMP, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES,
	REG_END
};

static int nregdescfloat[] = {
	/*  fv0,   ftmp1,   ftmp2,   ftmp3,     ft0,     ft1,     ft2,     ft3,   */
	REG_RET, REG_RES, REG_RES, REG_RES, REG_TMP, REG_TMP, REG_TMP, REG_TMP,

	/*  ft4,     ft5,     ft6,     ft7,     fa0,     fa1,     fa2,     fa3,   */
	REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_ARG, REG_ARG, REG_ARG, REG_ARG,

	/*  fa4,     fa5,     fa6,     fa7,     ft8,     ft9,    ft10,    ft11,   */
	REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_TMP, REG_TMP, REG_TMP, REG_TMP,

	/*  fs0,    ft12,     fs1,    ft13,     fs2,    ft14,     fs3     ft15    */
	REG_SAV, REG_TMP, REG_SAV, REG_TMP, REG_SAV, REG_TMP, REG_SAV, REG_TMP,

	REG_END
};

#else /* SIZEOF_VOID_P == 8 */

/* MIPS32 */

static int nregdescint[] = {
	/* zero,   itmp1,      v0,      v1,      a0,      a1,      a2,      a3,   */
	REG_RES, REG_RES, REG_RET, REG_RES, REG_ARG, REG_ARG, REG_ARG, REG_ARG,

	/*   t0,      t1,      t2,      t3,      t4,      t5,      t6,      t7,   */
	REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP,

	/*   s0,      s1,      s2,      s3,      s4,      s5,      s6,      s7,   */
	REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV,

	/*itmp2,   itmp3, k0(sys), k1(sys),      gp,      sp,      pv,      ra    */
	REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES,

	REG_END
};

#if !defined(ENABLE_SOFT_FLOAT)

static int nregdescfloat[] = {
	/*  fv0,            ftmp1,            ftmp2,            ftmp3,            */
	REG_RET, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES,

	/*  ft0,              ft1,              fa0,              fa1,            */
	REG_TMP, REG_RES, REG_TMP, REG_RES, REG_ARG, REG_RES, REG_ARG, REG_RES,

	/*  ft2,              ft3,              fs0,              fs1,            */
	REG_TMP, REG_RES, REG_TMP, REG_RES, REG_SAV, REG_RES, REG_SAV, REG_RES,

	/*  fs2,              fs3,              fs4,              fs5             */
	REG_SAV, REG_RES, REG_SAV, REG_RES, REG_SAV, REG_RES, REG_SAV, REG_RES,

	REG_END
};

#else /* !defined(ENABLE_SOFT_FLOAT) */

static int nregdescfloat[] = {
	REG_END
};

#endif /* !defined(ENABLE_SOFT_FLOAT) */

#endif /* SIZEOF_VOID_P == 8 */

#endif /* _MD_ABI_INC */


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
