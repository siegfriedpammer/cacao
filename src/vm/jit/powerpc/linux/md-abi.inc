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

   $Id: md-abi.inc 2539 2005-05-31 15:55:54Z twisti $

*/


#ifndef _MD_ABI_INC
#define _MD_ABI_INC

#include "vm/jit/jit.h"


/* register descripton array **************************************************/

static int nregdescint[] = {
	REG_RES, REG_RES, REG_TMP, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG,
	REG_ARG, REG_ARG, REG_ARG, REG_RES, REG_RES, REG_RES, REG_SAV, REG_SAV,
	REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP,
	REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV,
	REG_END
};


static int nregdescfloat[] = {
	REG_RES, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG,
	REG_ARG, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_SAV, REG_SAV,
	REG_RES, REG_RES, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP,
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
