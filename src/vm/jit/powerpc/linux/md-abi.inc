/* src/vm/jit/powerpc/linux/md-abi.inc - stuff for PowerPC Linux ABI

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

   $Id: md-abi.inc 3167 2005-09-10 20:30:43Z twisti $

*/


#ifndef _MD_ABI_INC
#define _MD_ABI_INC

#include "vm/jit/jit.h"


/* register descripton array **************************************************/

static int nregdescint[] = {
	/* zero,      sp, NO(sys),   a0/v0,   a0/v1,      a2,      a3,      a4,   */
	REG_RES, REG_RES, REG_RES, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG,

	/*   a5,      a6,      a7,   itmp1,   itmp2,      pv,      s0,      s1,   */
	REG_ARG, REG_ARG, REG_ARG, REG_RES, REG_RES, REG_RES, REG_SAV, REG_SAV,

	/*itmp3,      t0,      t1,      t2,      t3,      t4,      t5,      t6,   */
	REG_RES, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP,

	/*   s2,      s3,      s4,      s5,      s6,      s7,      s8,      s9,   */
	REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV,

	REG_END
};


static int nregdescfloat[] = {
	/*ftmp3,  fa0/v0,     fa1,     fa2,     fa3,     fa4,     fa5,     fa6,   */
	REG_RES, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG,

	/*  fa7,     ft0,     ft1,     ft2,     ft3,     ft4,     fs0,     fs1,   */
	REG_ARG, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_SAV, REG_SAV,

	/*ftmp1,   ftmp2,     ft5,     ft6,     ft7,     ft8,     ft9,    ft10,   */
	REG_RES, REG_RES, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP,

	/*  fs2,     fs3,     fs4,     fs5,     fs6,     fs7,     fs8,     fs9    */
	REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV,

	REG_END
};

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
