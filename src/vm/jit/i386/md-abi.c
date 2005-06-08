/* src/vm/jit/i386/md-abi.c - functions for i386 Linux ABI

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

   $Id: md-abi.c 2605 2005-06-08 14:41:35Z christian $

*/


#include "vm/jit/i386/types.h"
#include "vm/jit/i386/md-abi.h"

#include "vm/descriptor.h"
#include "vm/global.h"


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
  int       i;

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
