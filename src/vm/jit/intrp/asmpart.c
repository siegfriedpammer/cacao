/* src/vm/jit/intrp/asmpart.S - Java-C interface functions for Interpreter

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

   Authors: Andreas Krall
            Reinhard Grafl

   Changes: Joseph Wenninger
            Christian Thalinger

   $Id: asmpart.c 3176 2005-09-14 08:51:23Z twisti $

*/


#include "config.h"

#include "vm/jit/intrp/arch.h"
#include "vm/jit/intrp/types.h"

#include "vm/jit/asmpart.h"


/* this is required to link cacao with intrp */
threadcritnode asm_criticalsections = { NULL, NULL, NULL };


void asm_getclassvalues_atomic(vftbl_t *super, vftbl_t *sub, castinfo *out)
{
	s4 sbv, sdv, sv;

#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
	compiler_lock();
#else
	intsDisable();
#endif
#endif

	sbv = super->baseval;
	sdv = super->diffval;
	sv  = sub->baseval;

#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
	compiler_unlock();
#else
	intsRestore();
#endif
#endif

	out->super_baseval = sbv;
	out->super_diffval = sdv;
	out->sub_baseval   = sv;
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
