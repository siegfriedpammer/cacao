/* src/vm/jit/i386/md-abi.c - functions for i386 Linux ABI

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

   Authors: Christian Ullrich

   Changes: Christian Thalinger

   $Id: md-abi.c 5080 2006-07-06 12:42:23Z twisti $

*/


#include "config.h"
#include "vm/types.h"

#include "vm/jit/i386/md-abi.h"

#include "vm/descriptor.h"
#include "vm/global.h"


/* register descripton - array ************************************************/

s4 nregdescint[] = {
    REG_RET, REG_RES, REG_RES, REG_TMP, REG_RES, REG_SAV, REG_SAV, REG_SAV,
    REG_END
};


s4 nregdescfloat[] = {
 /* rounding problems with callee saved registers */
 /* REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_TMP, REG_TMP, REG_RES, REG_RES, */
 /* REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_RES, REG_RES, */
    REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES,
    REG_END
};


/* md_param_alloc **************************************************************

   Allocate Arguments to Stackslots according the Calling Conventions

   --- in
   md->paramcount:           Number of arguments for this method
   md->paramtypes[].type:    Argument types
   
   --- out
   md->params[].inmemory:    Argument spilled on stack
   md->params[].regoff:      Stack offset or rd->arg[int|flt]regs index
   md->memuse:               Stackslots needed for argument spilling
   md->argintreguse:         max number of integer arguments used
   md->argfltreguse:         max number of float arguments used

*******************************************************************************/

void md_param_alloc(methoddesc *md)
{
	paramdesc *pd;
	s4        stacksize;
	s4        i;

	pd = md->params;
	stacksize = 0;

	for (i = 0; i < md->paramcount; i++, pd++) {
		pd->inmemory = true;
		pd->regoff = stacksize;
		stacksize += IS_2_WORD_TYPE(md->paramtypes[i].type) ? 2 : 1;
	}

	md->memuse = stacksize;
	md->argintreguse = 0;
	md->argfltreguse = 0;
}


/* md_return_alloc *************************************************************

   No straight forward precoloring of the Java Stackelement containing
   the return value possible for i386, since it uses "reserved"
   registers for return values

*******************************************************************************/

void md_return_alloc(jitdata *jd, stackptr stackslot)
{
	/* nothing */
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
