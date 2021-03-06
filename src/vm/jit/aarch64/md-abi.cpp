/* src/vm/jit/alpha/md-abi.cpp - functions for Alpha ABI

   Copyright (C) 1996-2013
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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

*/


#include "config.h"
#include "vm/types.hpp"

#include "vm/jit/aarch64/md-abi.hpp"

#include "vm/descriptor.hpp"
#include "vm/global.hpp"
#include "vm/method.hpp"

#include "vm/jit/abi.hpp"
#include "vm/jit/code.hpp"


/* register descripton array **************************************************/

s4 nregdescint[] = {
	/*   a0,      a1,      a2,      a3,      a4,      a5,      a6,      a7,   */
	REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, 

	/*   r0,   itmp1,   itmp2,   itmp3,      t0,      t1,      t2,      t3,   */
	REG_RET, REG_RES, REG_RES, REG_RES, REG_TMP, REG_TMP, REG_TMP, REG_TMP, 

	/*   t4,      t5,      pv,      s0,      s1,      s2,      s3,      s4,   */
	REG_TMP, REG_TMP, REG_RES, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV,

	/*   s5,      s6,      s7,      s8,      s9,      fp,      lr, sp/zero,   */
	REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_RES, REG_RES, REG_RES,

	REG_END
};

const char *abi_registers_integer_name[] = {
	"a0", "a1",  "a2",  "a3",  "a4", "a5", "a6", "a7",
	"r0", "it1", "it2", "it3", "t0", "t1", "t2", "t3",
	"t4", "t5",  "pv",  "s0",  "s1", "s2", "s3", "s4",
	"s5", "s6",  "s7",  "s8",  "s9", "fp", "lr", "sp"
};

const s4 abi_registers_integer_argument[] = {
	0, /* a0  */
	1, /* a1  */
	2, /* a2  */
	3, /* a3  */
	4, /* a4  */
	5, /* a5  */
	6, /* a6  */
	7, /* a7  */
};

const s4 abi_registers_integer_saved[] = {
	19, /* s0  */
	20, /* s1  */
	21, /* s2  */
	22, /* s3  */
	23, /* s4  */
	24, /* s5  */
	25, /* s6  */
	26, /* s7  */
	27, /* s8  */
	28, /* s9  */
};

const s4 abi_registers_integer_temporary[] = {
	12, /* t0  */
	13, /* t1  */
	14, /* t2  */
	15, /* t3  */
	16, /* t4  */
	17, /* t5  */
};


s4 nregdescfloat[] = {
	REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG,
	REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, 
	REG_RES, REG_RES, REG_RES, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP,
	REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP,
	REG_END
};


const s4 abi_registers_float_argument[] = {
	0, /* fa0  */
	1, /* fa1  */
	2, /* fa2  */
	3, /* fa3  */
	4, /* fa4  */
	5, /* fa5  */
	6, /* fa6  */
	7, /* fa7  */
};

const s4 abi_registers_float_saved[] = {
	8,  /* fs0  */
	9,  /* fs1  */
	10, /* fs2  */
	11, /* fs3  */
	12, /* fs4  */
	13, /* fs5  */
	14, /* fs6  */
	15, /* fs7  */
};

const s4 abi_registers_float_temporary[] = {
	19, /* ft0  */
	20, /* ft1  */
	21, /* ft2  */
	22, /* ft3  */
	23, /* ft4  */
	24, /* ft5  */
	25, /* ft6  */
	26, /* ft7  */
	27, /* ft8  */
	28, /* ft9  */
	29, /* ft10 */
	30, /* ft11 */
	31, /* ft12 */
};


/* md_param_alloc **************************************************************

   Allocate the parameters of the given method descriptor according to the
   calling convention of the platform.

*******************************************************************************/

void md_param_alloc(methoddesc *md)
{
	paramdesc *pd;
	s4         i;
	s4         reguse, freguse;
	s4         stacksize;

	/* set default values */

	reguse    = 0;
	freguse   = 0;
	stacksize = 0;

	/* get params field of methoddesc */

	pd = md->params;

	for (i = 0; i < md->paramcount; i++, pd++) {
		switch (md->paramtypes[i].type) {
		case TYPE_INT:
		case TYPE_ADR:
		case TYPE_LNG:
			if (reguse < INT_ARG_CNT) {
				pd->inmemory = false;
				pd->index    = reguse;
				pd->regoff   = abi_registers_integer_argument[reguse];
				reguse++;
				md->argintreguse = reguse;
			}
			else {
				pd->inmemory = true;
				pd->index    = stacksize;
				pd->regoff   = stacksize * 8;
				stacksize++;
			}
			break;

		case TYPE_FLT:
		case TYPE_DBL:
			if (freguse < FLT_ARG_CNT) {
				pd->inmemory = false;
				pd->index    = freguse;
				pd->regoff   = abi_registers_float_argument[freguse];
				freguse++;
				md->argfltreguse = freguse;
			}
			else {
				pd->inmemory = true;
				pd->index    = stacksize;
				pd->regoff   = stacksize * 8;
				stacksize++;
			}
			break;
		default:
			assert(false);
			break;
		}
	}

	/* fill register and stack usage */

	md->memuse = stacksize;
}


/* md_param_alloc_native *******************************************************

   Pre-allocate arguments according to the native ABI.

*******************************************************************************/

void md_param_alloc_native(methoddesc *md)
{
	/* On Alpha we use the same ABI for JIT method calls as for native
	   method calls. */

	md_param_alloc(md);
}


/* md_return_alloc *************************************************************

   Precolor the Java Stackelement containing the Return Value, if possible. 

   --- in
   jd:                      jitdata of the current method
   stackslot:               Java Stackslot to contain the Return Value
   
   --- out
   if precoloring was possible:
   VAR(stackslot->varnum)->flags       = PREALLOC
   			             ->vv.regoff   = [REG_RESULT|REG_FRESULT]
   rd->arg[flt|int]reguse   set to a value according the register usage

   NOTE: Do not pass a LOCALVAR in stackslot->varnum.
*******************************************************************************/

void md_return_alloc(jitdata *jd, stackelement_t *stackslot)
{
	methodinfo   *m;
	codeinfo     *code;
	registerdata *rd;
	methoddesc   *md;

	/* get required compiler data */
	m    = jd->m;
	code = jd->code;
	rd   = jd->rd;

	md = m->parseddesc;

	/* In Leafmethods Local Vars holding parameters are precolored to
	   their argument register -> so leafmethods with paramcount > 0
	   could already use R0 == a00! */

	if (!code_is_leafmethod(code) || (md->paramcount == 0)) {
	/* Only precolor the stackslot, if it is not a SAVEDVAR <-> has
	   not to survive method invokations. */

		if (!(stackslot->flags & SAVEDVAR)) {

			VAR(stackslot->varnum)->flags = PREALLOC;

			if (IS_INT_LNG_TYPE(md->returntype.type)) {
				if (rd->argintreguse < 1)
					rd->argintreguse = 1;
				VAR(stackslot->varnum)->vv.regoff = REG_RESULT;
			} else {
				if (rd->argfltreguse < 1)
					rd->argfltreguse = 1;
				VAR(stackslot->varnum)->vv.regoff = REG_FRESULT;
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
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
