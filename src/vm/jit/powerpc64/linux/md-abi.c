/* src/vm/jit/powerpc64/linux/md-abi.c - functions for PowerPC64 Linux ABI

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

   Authors: Roland Lezuo

   Changes: 

   $Id: md-abi.c 5389 2006-09-06 23:18:27Z twisti $

*/


#include "config.h"
#include "vm/types.h"

#include "vm/jit/powerpc64/linux/md-abi.h"

#include "vm/descriptor.h"
#include "vm/global.h"
#include "vm/jit/abi.h"


#define _ALIGN(a)    do { if ((a) & 1) (a)++; } while (0)


/* register descripton array **************************************************/

s4 nregdescint[] = {
	/* zero,      sp,     TOC,   a0/v0,   a0/v1,      a2,      a3,      a4,   */
	REG_RES, REG_RES, REG_RES, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG,

	/*   a5,      a6,      a7,   itmp1,   itmp2, NO(SYS),      pv,      s0,   */
	REG_ARG, REG_ARG, REG_ARG, REG_RES, REG_RES, REG_RES, REG_RES, REG_SAV,

	/*itmp3,      t0,      t1,      t2,      t3,      t4,      t5,      t6,   */
	REG_RES, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP,

	/*   s1,      s2,      s3,      s4,      s5,      s6,      s7,      s8,   */
	REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV,

	REG_END
};

char *regs[] = {
	"r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
	"r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
	"r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
	"r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",
};


s4 nregdescfloat[] = {
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
	stacksize = LA_SIZE_IN_POINTERS + PA_SIZE_IN_POINTERS;

	/* get params field of methoddesc */

	pd = md->params;

	for (i = 0; i < md->paramcount; i++, pd++) {
		switch (md->paramtypes[i].type) {
		case TYPE_LNG:
		case TYPE_INT:
		case TYPE_ADR:
			if (iarg < INT_ARG_CNT) {
				pd->inmemory = false;
				pd->regoff = iarg;
				iarg++;
			} else {
				pd->inmemory = true;
				pd->regoff = stacksize;
				stacksize++;
			}
			break;
		case TYPE_FLT:
			if (farg < FLT_ARG_CNT) {
				pd->inmemory = false;
				pd->regoff = farg;
				farg++;
			} else {
				pd->inmemory = true;
				pd->regoff = stacksize;
				stacksize++;
			}
			break;
		case TYPE_DBL:
			if (farg < FLT_ARG_CNT) {
				pd->inmemory = false;
				pd->regoff = farg;
				farg++;
			} else {
				_ALIGN(stacksize);
				pd->inmemory = true;
				pd->regoff = stacksize;
				stacksize += 2;
			}
			break;
		}
	}

	/* Since R3/R4, F1 (==A0/A1, A0) are used for passing return values, this */
	/* argument register usage has to be regarded, too                        */
	if (IS_INT_LNG_TYPE(md->returntype.type)) {
		if (iarg < 1)
			iarg = 1;
	} else if (IS_FLT_DBL_TYPE(md->returntype.type)) {
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
   possible.  (R3==a00 for int/adr, R4/R3 == a01/a00 for long, F1==a00
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

void md_return_alloc(jitdata *jd, stackptr stackslot)
{
	methodinfo *m;
	codeinfo *code;
	registerdata *rd;
	methoddesc *md;

	/* get required compiler data */

	m = jd->m;
	code = jd->code;
	rd = jd->rd;

	md = m->parseddesc;
	
	/* In Leafmethods Local Vars holding parameters are precolored to
	   their argument register -> so leafmethods with paramcount > 0
	   could already use R3 == a00! */

	if (!jd->isleafmethod || (md->paramcount == 0)) {
		/* Only precolor the stackslot, if it is not a SAVEDVAR <->
		   has not to survive method invokations. */

		if (!(stackslot->flags & SAVEDVAR)) {
			stackslot->varkind = ARGVAR;
			stackslot->varnum = -1;
			stackslot->flags = 0;

			if (IS_INT_LNG_TYPE(md->returntype.type)) {
				if (rd->argintreguse < 1)
					rd->argintreguse = 1;

				stackslot->regoff = REG_RESULT;
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
