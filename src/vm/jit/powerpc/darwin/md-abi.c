/* src/vm/jit/powerpc/darwin/md-abi.c - functions for PowerPC Darwin ABI

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

   $Id: md-abi.c 3487 2005-10-21 13:45:45Z twisti $

*/


#include "config.h"
#include "vm/types.h"

#include "vm/jit/powerpc/darwin/md-abi.h"

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
	s4         i;
	s4         iarg;
	s4         farg;
	s4         stacksize;

	/* set default values */

	iarg = 0;
	farg = 0;
	stacksize = LA_WORD_SIZE;

	/* get params field of methoddesc */

	pd = md->params;

	for (i = 0; i < md->paramcount; i++, pd++) {
		switch (md->paramtypes[i].type) {
		case TYPE_INT:
		case TYPE_ADR:
			if (iarg < INT_ARG_CNT) {
				pd->inmemory = false;
				pd->regoff = iarg;           /* rd->arg[int|flt]regs index !! */
				iarg++;
			} else {
				pd->inmemory = true;
				pd->regoff = stacksize;
			}
			stacksize++;
			break;
		case TYPE_LNG:
			if (iarg < INT_ARG_CNT - 1) {
				pd->inmemory = false;
				                             /* rd->arg[int|flt]regs index !! */
				pd->regoff = PACK_REGS(iarg + 1, iarg); 
				iarg += 2;
			} else {
				pd->inmemory = true;
				pd->regoff = stacksize;
				iarg = INT_ARG_CNT;
			}
			stacksize += 2;
			break;
		case TYPE_FLT:
			if (farg < FLT_ARG_CNT) {
				pd->inmemory = false;
				pd->regoff = farg;           /* rd->arg[int|flt]regs index !! */
				iarg++;     /* skip 1 integer argument register */
				farg++;
			} else {
				pd->inmemory = true;
				pd->regoff = stacksize;
			}
			stacksize++;
			break;
		case TYPE_DBL:
			if (farg < FLT_ARG_CNT) {
				pd->inmemory = false;
				pd->regoff = farg;           /* rd->arg[int|flt]regs index !! */
				iarg += 2;  /* skip 2 integer argument registers */
				farg++;
			} else {
				pd->inmemory = true;
				pd->regoff = stacksize;
			}
			stacksize += 2;
			break;
		}
	}


	/* Since R3/R4, F1 (==A0/A1, A0) are used for passing return values, this */
	/* argument register usage has to be regarded, too                        */
	if (IS_INT_LNG_TYPE(md->returntype.type)) {
		if (iarg < (IS_2_WORD_TYPE(md->returntype.type) ? 2 : 1))
			iarg = IS_2_WORD_TYPE(md->returntype.type) ? 2 : 1;
	} else {
		if (IS_FLT_DBL_TYPE(md->returntype.type))
			if (farg < 1)
				farg = 1;
	}

	/* fill register and stack usage */
	md->argintreguse = iarg;
	md->argfltreguse = farg;
	md->memuse = stacksize;
}


/* md_return_alloc *************************************************************

   Precolor the Java Stackelement containing the Return Value, if
   possible. (R3==a00 for int/adr, R4/R3 == a01/a00 for long, F1==a00
   for float/double)

   --- in
   m:                       Methodinfo of current method
   return_type:             Return Type of the Method (TYPE_INT.. TYPE_ADR)
                            TYPE_VOID is not allowed!
   stackslot:               Java Stackslot to contain the Return Value
   
   --- out
   if precoloring was possible:
   stackslot->varkind       =ARGVAR
            ->varnum        =-1
            ->flags         =0
            ->regoff        =[REG_RESULT, (REG_RESULT2/REG_RESULT), REG_FRESULT]
   rd->arg[flt|int]reguse   set to a value according the register usage

*******************************************************************************/

void md_return_alloc(methodinfo *m, registerdata *rd, s4 return_type,
					 stackptr stackslot)
{
	/* In Leafmethods Local Vars holding parameters are precolored to their   */
	/* argument register -> so leafmethods with paramcount > 0 could already  */
	/* use  R3 == a00! */

	if (!m->isleafmethod || (m->paramcount == 0)) {
		/* Only precolor the stackslot, if it is not a SAVEDVAR <-> has not   */
		/* to survive method invokations */

		if (!(stackslot->flags & SAVEDVAR)) {
			stackslot->varkind = ARGVAR;
			stackslot->varnum = -1;
			stackslot->flags = 0;

			if (IS_INT_LNG_TYPE(return_type)) {
				if (!IS_2_WORD_TYPE(return_type)) {
					if (rd->argintreguse < 1)
						rd->argintreguse = 1;

					stackslot->regoff = REG_RESULT;

				} else {
					if (rd->argintreguse < 2)
						rd->argintreguse = 2;

					stackslot->regoff = PACK_REGS(REG_RESULT2, REG_RESULT);
				}

			} else { /* float/double */
				if (rd->argfltreguse < 1)
					rd->argfltreguse = 1;

				stackslot->regoff = REG_FRESULT;
			}
		}
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
 */
