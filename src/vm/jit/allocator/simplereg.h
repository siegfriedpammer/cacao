/* src/vm/jit/allocator/simplereg.h - register allocator header

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

   Changes: Christian Ullrich

   $Id: simplereg.h 4055 2006-01-02 12:59:54Z christian $

*/


#ifndef _SIMPLE_REG_H
#define _SIMPLE_REG_H

#include "config.h"
#include "vm/types.h"

#include "arch.h"

#include "vm/jit/codegen-common.h"
#include "vm/jit/jit.h"
#include "vm/jit/inline/inline.h"


/* function prototypes ********************************************************/

void regalloc(methodinfo *m, codegendata *cd, registerdata *rd);

#if defined(ENABLE_STATISTICS)
void reg_make_statistics( methodinfo *, codegendata *, registerdata *);
#endif

#endif /* _SIMPLE_REG_H */


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
