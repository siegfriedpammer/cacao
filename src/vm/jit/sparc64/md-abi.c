/* src/vm/jit/sparc64/md-abi.c - functions for Sparc ABI

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

   Authors: Alexander Jordan

   Changes:

   $Id: md-abi.h 4357 2006-01-22 23:33:38Z twisti $

*/


#include "config.h"
#include "vm/types.h"

#include "vm/jit/sparc64/md-abi.h"

#include "vm/descriptor.h"
#include "vm/global.h"
#include "vm/jit/abi.h"

/* temp */
#include "mm/memory.h"
#include <assert.h>


/* register descripton array **************************************************/

/* callee point-of-view, after SAVE has been called. */
s4 nregdescint[] = {
	/* zero  itmp1/g1 itmp2/g2 itmp3/g3 temp/g4  temp/g5  sys/g6   sys/g7 */  
	REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES,
	
	/* o0    o1       o2       o3       o4       pv/o5    sp/o6    o7/ra  */
	REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_RES, REG_RES, REG_RES,
	
	/* l0    l1       l2       l3       l4       l5       l6       l7     */
	REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV,
	
	/* i0/v0 i1       i2       i3       i4       pv/i5    fp/i6    ra/i7  */
	REG_RET, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_RES, REG_RES, REG_RES,
	REG_END
	

};

s4 nregdescfloat[] = {
	REG_RET, REG_RES, REG_RES, REG_RES, REG_TMP, REG_TMP, REG_TMP, REG_TMP,
	REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_TMP, REG_TMP, REG_TMP, REG_TMP,
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
	s4         i;
	s4         reguse;
	s4         stacksize;

	/* set default values */

	reguse = 0;
	stacksize = 0;

	/* get params field of methoddesc */

	pd = md->params;

	for (i = 0; i < md->paramcount; i++, pd++) {
		switch (md->paramtypes[i].type) {
		case TYPE_INT:
		case TYPE_ADR:
		case TYPE_LNG:
			if (i < INT_ARG_CNT) {
				pd->inmemory = false;
				pd->regoff = reguse;
				reguse++;
				md->argintreguse = reguse;

			} else {
				pd->inmemory = true;
				pd->regoff = stacksize;
				stacksize++;
			}
			break;
		case TYPE_FLT:
		case TYPE_DBL:
			if (i < FLT_ARG_CNT) {
				pd->inmemory = false;
				pd->regoff = reguse;
				reguse++;
				md->argfltreguse = reguse;
			} else {
				pd->inmemory = true;
				pd->regoff = stacksize;
				stacksize++;
			}
			break;
		}
	}
	
	/* Since O0 is used for passing return values, this */
	/* argument register usage has to be regarded, too                        */
	if (IS_INT_LNG_TYPE(md->returntype.type)) {
		if (reguse < 1)
			md->argintreguse = 1;
	}

	/* fill register and stack usage */

	md->memuse = stacksize;
}

/* md_native_param_alloc **************************************************************

   XXX

*******************************************************************************/

void md_native_param_alloc(methoddesc *md)
{
	paramdesc *pd;
	s4         i;
	s4         reguse;
	s4         stacksize;

	/* set default values */

	reguse = 0;
	stacksize = 6;

	/* get params field of methoddesc */

	pd = md->params;

	for (i = 0; i < md->paramcount; i++, pd++) {
		switch (md->paramtypes[i].type) {
		case TYPE_INT:
		case TYPE_ADR:
		case TYPE_LNG:
			if (i < INT_NATARG_CNT) {
				pd->inmemory = false;
				pd->regoff = reguse;
				reguse++;
				md->argintreguse = reguse;

			} else {
				pd->inmemory = true;
				pd->regoff = reguse;
				reguse++;
				stacksize++;
			}
			break;
		case TYPE_FLT:
		case TYPE_DBL:
			if (i < FLT_NATARG_CNT) {
				pd->inmemory = false;
				pd->regoff = reguse;
				reguse++;
				md->argfltreguse = reguse;
			} else {
				pd->inmemory = true;
				pd->regoff = reguse;
				reguse++;
				stacksize++;
			}
			break;
		}
	}
	
	/* Since O0 is used for passing return values, this */
	/* argument register usage has to be regarded, too                        */
	if (IS_INT_LNG_TYPE(md->returntype.type)) {
		if (reguse < 1)
			md->argintreguse = 1;
	}

	/* fill register and stack usage */

	md->memuse = stacksize;
}

/* reg_setup *******************************************************************

   TODO

*******************************************************************************/

void md_native_reg_setup(jitdata *jd)
{
	methodinfo   *m;
	registerdata *rd;
	s4            i;

	/* get required compiler data */

	m  = jd->m;
	rd = jd->rd;

	/* setup the integer register table */


	rd->argintregs = DMNEW(s4, INT_NATARG_CNT);
	rd->argintreguse = 0;

	for (rd->argintreguse = 0, i = 8; rd->argintreguse < INT_NATARG_CNT; i++) {
		rd->argintregs[rd->argintreguse++] = i;
	}
	
	assert(rd->argintreguse == INT_NATARG_CNT);
		
	/* setup the float register table */

	rd->argfltregs = DMNEW(s4, FLT_NATARG_CNT);

	rd->argfltreguse = 0;


	for (rd->argfltreguse = 0, i = 0; rd->argfltreguse < FLT_NATARG_CNT; i++) {
		rd->argfltregs[rd->argfltreguse++] = i;
	}
	assert(rd->argfltreguse == FLT_NATARG_CNT);

}

/* md_return_alloc *************************************************************

   Precolor the Java Stackelement containing the Return Value. Since
   alpha has a dedicated return register (not an reused arg or
   reserved reg), this is striaghtforward possible, as long, as this
   stackelement does not have to survive a method invokation
   (SAVEDVAR)

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
   			->regoff        =[REG_RESULT, REG_FRESULT]

*******************************************************************************/

void md_return_alloc(jitdata *jd, stackptr stackslot)
{
	methodinfo *m;
	methoddesc *md;
	
	assert(0);

	/* get required compiler data */

	m = jd->m;

	md = m->parseddesc;

	/* Only precolor the stackslot, if it is not a SAVEDVAR <-> has
	   not to survive method invokations. */


	if (!(stackslot->flags & SAVEDVAR)) {
		stackslot->varkind = ARGVAR;
		stackslot->varnum = -1;
		stackslot->flags = 0;

		if (IS_INT_LNG_TYPE(md->returntype.type)) {
			stackslot->regoff = REG_RESULT_CALLEE;
		} else { /* float/double */
			stackslot->regoff = REG_FRESULT;
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
