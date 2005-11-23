/* vm/jit/intrp/codegen.h - code generation definitions for Interpreter

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

   Changes: Christian Thalinger

   $Id: codegen.h 3784 2005-11-23 22:39:49Z twisti $

*/


#ifndef _CODEGEN_H
#define _CODEGEN_H

#include "config.h"
#include "vm/types.h"


/* some defines ***************************************************************/

#define PATCHER_CALL_SIZE    0          /* dummy define (for codegen.inc)     */


/* additional functions and macros to generate code ***************************/

/* MCODECHECK(icnt) */

#define MCODECHECK(icnt) \
	if ((cd->mcodeptr + (icnt)) > (u1 *) cd->mcodeend) { \
        s4 lcoffset = cd->lastmcodeptr - cd->mcodebase; \
        cd->mcodeptr = (u1 *) codegen_increase(cd, cd->mcodeptr); \
        if (cd->lastmcodeptr != NULL) \
		  	cd->lastmcodeptr = lcoffset + cd->mcodebase; \
    }


/* gen_resolvebranch ***********************************************************

   backpatches a branch instruction; Alpha branch instructions are very
   regular, so it is only necessary to overwrite some fixed bits in the
   instruction.

   parameters: ip ... pointer to instruction after branch (void*)
               target ... offset of branch target         (void*)

*******************************************************************************/

#define gen_resolveanybranch(ip, target) \
		(((void **)(ip))[-1] = (target))

#endif /* _CODEGEN_H */


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
