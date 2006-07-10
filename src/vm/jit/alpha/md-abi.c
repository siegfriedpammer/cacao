/* src/vm/jit/alpha/md-abi.c - functions for Alpha ABI

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

   Authors: Christian Thalinger

   Changes: Christian Ullrich

   $Id: md-abi.c 5094 2006-07-10 13:51:38Z twisti $

*/


#include "config.h"
#include "vm/types.h"

#include "vm/jit/alpha/md-abi.h"

#include "vm/descriptor.h"
#include "vm/global.h"


/* register descripton array **************************************************/

s4 nregdescint[] = {
	/*   v0,      t0,      t1,      t2,      t3,      t4,      t5,      t6,   */
	REG_RET, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, 

	/*   t7,      s0,      s1,      s2,      s3,      s4,      s5,      s6,   */
	REG_TMP, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, 

	/*   a0,      a1,      a2,      a3,      a4,      a5,      t8,      t9,   */
	REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_TMP, REG_TMP,

	/*  t10,   itmp1,      ra,      pv,      at,   itmp3,      sp,    zero,   */
	REG_TMP, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES,

	REG_END
};

s4 nregdescfloat[] = {
	REG_RET, REG_TMP, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV,
	REG_SAV, REG_SAV, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, 
	REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_TMP, REG_TMP,
	REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_RES, REG_RES, REG_RES, REG_RES,
	REG_END
};


/* md_param_alloc **************************************************************

   Allocate the parameters of the given method descriptor according to the
   calling convention of the platform.

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
			}
			else {
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
			}
			else {
				pd->inmemory = true;
				pd->regoff = stacksize;
				stacksize++;
			}
			break;
		}
	}

	/* fill register and stack usage */

	md->memuse = stacksize;
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

	/* get required compiler data */

	m = jd->m;

	md = m->parseddesc;

	/* Only precolor the stackslot, if it is not a SAVEDVAR <-> has
	   not to survive method invokations. */

	if (!(stackslot->flags & SAVEDVAR)) {
		stackslot->varkind = ARGVAR;
		stackslot->varnum  = -1;
		stackslot->flags   = 0;

		if (IS_INT_LNG_TYPE(md->returntype.type))
			stackslot->regoff = REG_RESULT;
		else
			stackslot->regoff = REG_FRESULT;
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
