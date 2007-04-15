/* src/vm/jit/mips/md-abi.c - functions for MIPS ABI

   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
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

   $Id: md-abi.c 7699 2007-04-13 10:42:05Z twisti $

*/


#include "config.h"
#include "vm/types.h"

#include "vm/jit/mips/md-abi.h"

#include "vm/global.h"

#include "vm/jit/abi.h"

#include "vmcore/descriptor.h"


/* register descripton array **************************************************/

#if SIZEOF_VOID_P == 8

/* MIPS64 */

s4 nregdescint[] = {
	REG_RES, REG_RES, REG_RET, REG_RES, REG_ARG, REG_ARG, REG_ARG, REG_ARG,
	REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_TMP, REG_TMP, REG_TMP, REG_TMP,
	REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV,
	REG_TMP, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES,
	REG_END
};

const char *abi_registers_integer_name[] = {
	"zero",  "at",    "v0",    "v1",    "a0",    "a1",    "a2",    "a3",
	"a4",    "a5",    "a6",    "a7",    "t0",    "t1",    "t2",    "t3",
	"s0",    "s1",    "s2",    "s3",    "s4",    "s5",    "s6",    "s7",
	"t8",    "t9",    "k0",    "k1",    "gp",    "sp",    "s8",    "ra"
};


s4 nregdescfloat[] = {
	/*  fv0,   ftmp1,   ftmp2,   ftmp3,     ft0,     ft1,     ft2,     ft3,   */
	REG_RET, REG_RES, REG_RES, REG_RES, REG_TMP, REG_TMP, REG_TMP, REG_TMP,

	/*  ft4,     ft5,     ft6,     ft7,     fa0,     fa1,     fa2,     fa3,   */
	REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_ARG, REG_ARG, REG_ARG, REG_ARG,

	/*  fa4,     fa5,     fa6,     fa7,     ft8,     ft9,    ft10,    ft11,   */
	REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_TMP, REG_TMP, REG_TMP, REG_TMP,

	/*  fs0,    ft12,     fs1,    ft13,     fs2,    ft14,     fs3     ft15    */
	REG_SAV, REG_TMP, REG_SAV, REG_TMP, REG_SAV, REG_TMP, REG_SAV, REG_TMP,

	REG_END
};

#else /* SIZEOF_VOID_P == 8 */

/* MIPS32 */

s4 nregdescint[] = {
	/* zero,   itmp1,      v0,      v1,      a0,      a1,      a2,      a3,   */
	REG_RES, REG_RES, REG_RET, REG_RES, REG_ARG, REG_ARG, REG_ARG, REG_ARG,

	/*   t0,      t1,      t2,      t3,      t4,      t5,      t6,      t7,   */
	REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP,

	/*   s0,      s1,      s2,      s3,      s4,      s5,      s6,      s7,   */
	REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV,

	/*itmp2,   itmp3, k0(sys), k1(sys),      gp,      sp,      pv,      ra    */
	REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES,

	REG_END
};

const char *abi_registers_integer_name[] = {
	"zero",  "at",    "v0",    "v1",    "a0",    "a1",    "a2",    "a3",
	"a4",    "a5",    "a6",    "a7",    "t0",    "t1",    "t2",    "t3",
	"s0",    "s1",    "s2",    "s3",    "s4",    "s5",    "s6",    "s7",
	"t8",    "t9",    "k0",    "k1",    "gp",    "sp",    "s8",    "ra"
};


#if !defined(ENABLE_SOFT_FLOAT)

s4 nregdescfloat[] = {
	/*  fv0,            ftmp1,            ftmp2,            ftmp3,            */
	REG_RET, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES,

	/*  ft0,              ft1,              fa0,              fa1,            */
	REG_TMP, REG_RES, REG_TMP, REG_RES, REG_ARG, REG_RES, REG_ARG, REG_RES,

	/*  ft2,              ft3,              fs0,              fs1,            */
	REG_TMP, REG_RES, REG_TMP, REG_RES, REG_SAV, REG_RES, REG_SAV, REG_RES,

	/*  fs2,              fs3,              fs4,              fs5             */
	REG_SAV, REG_RES, REG_SAV, REG_RES, REG_SAV, REG_RES, REG_SAV, REG_RES,

	REG_END
};

#else /* !defined(ENABLE_SOFT_FLOAT) */

s4 nregdescfloat[] = {
	REG_END
};

#endif /* !defined(ENABLE_SOFT_FLOAT) */

#endif /* SIZEOF_VOID_P == 8 */


/* md_param_alloc **************************************************************

   Pre-allocate arguments according to the internal JIT ABI.

*******************************************************************************/

void md_param_alloc(methoddesc *md)
{
	paramdesc *pd;
	s4         i;
	s4         reguse;
	s4         stacksize;
#if SIZEOF_VOID_P == 4 && !defined(ENABLE_SOFT_FLOAT)
	s4         t;
	bool       a0_is_float;
#endif

	/* set default values */

	reguse = 0;
	stacksize = 0;
#if SIZEOF_VOID_P == 4 && !defined(ENABLE_SOFT_FLOAT)
	a0_is_float = false;
#endif

	/* get params field of methoddesc */

	pd = md->params;

	for (i = 0; i < md->paramcount; i++, pd++) {
#if SIZEOF_VOID_P == 8

		switch (md->paramtypes[i].type) {
		case TYPE_INT:
		case TYPE_ADR:
		case TYPE_LNG:
			if (i < INT_ARG_CNT) {
				pd->inmemory = false;
				pd->regoff   = reguse;
				reguse++;
				md->argintreguse = reguse;
			}
			else {
				pd->inmemory = true;
				pd->regoff   = stacksize;
				stacksize++;
			}
			break;
		case TYPE_FLT:
		case TYPE_DBL:
			if (i < FLT_ARG_CNT) {
				pd->inmemory = false;
				pd->regoff   = reguse;
				reguse++;
				md->argfltreguse = reguse;
			}
			else {
				pd->inmemory = true;
				pd->regoff   = stacksize;
				stacksize++;
			}
			break;
		}

		/* register type is the same as java type */

		pd->type = md->paramtypes[i].type;

#else /* SIZEOF_VOID_P == 8 */

#if !defined(ENABLE_SOFT_FLOAT)

#define ALIGN_2_WORD(s)    ((s) & 1) ? ++(s) : (s)

		t = md->paramtypes[i].type;

		if (IS_FLT_DBL_TYPE(t) &&
			((i == 0) ||
			 ((i == 1) && IS_FLT_DBL_TYPE(md->paramtypes[0].type)))) {
			if (IS_2_WORD_TYPE(t)) {
				pd->type = TYPE_DBL;
				pd->regoff = reguse;
				reguse++;
				stacksize += 2;
			}
			else {
				pd->type = TYPE_FLT;
				pd->regoff = reguse;
				reguse++;
				stacksize++;
			}
			md->argfltreguse = reguse;
			a0_is_float = true;
		}
		else {
			if (IS_2_WORD_TYPE(t)) {
				ALIGN_2_WORD(reguse);
				pd->type = TYPE_LNG;

				if (reguse < INT_ARG_CNT) {
					pd->inmemory = false;
# if WORDS_BIGENDIAN == 1
					pd->regoff   = PACK_REGS(reguse + 1, reguse);
# else
					pd->regoff   = PACK_REGS(reguse, reguse + 1);
# endif
					reguse += 2;
					md->argintreguse = reguse;
				}
				else {
					pd->inmemory = true;
					pd->regoff   = ALIGN_2_WORD(stacksize);
				}
				stacksize += 2;
			}
			else {
				pd->type = TYPE_INT;

				if (reguse < INT_ARG_CNT) {
					pd->inmemory = false;
					pd->regoff = reguse;
					reguse++;
					md->argintreguse = reguse;
				}
				else {
					pd->inmemory = true;
					pd->regoff = stacksize;
				}
				stacksize++;
			}
		}

#else /* !defined(ENABLE_SOFT_FLOAT) */
#error never actually tested!

		switch (md->paramtypes[i].type) {
		case TYPE_INT:
		case TYPE_ADR:
		case TYPE_FLT:
			pd->type = TYPE_INT;

			if (i < INT_ARG_CNT) {
				pd->inmemory = false;
				pd->regoff   = reguse;
				reguse++;
				md->argintreguse = reguse;
			}
			else {
				pd->inmemory = true;
				pd->regoff   = stacksize;
			}
			stacksize++;
			break;
		case TYPE_LNG:
		case TYPE_DBL:
			pd->type = TYPE_LNG;

			if (i < INT_ARG_CNT) {
				pd->inmemory = false;
#if WORDS_BIGENDIAN == 1
				pd->regoff   = PACK_REGS(reguse + 1, reguse);
#else
				pd->regoff   = PACK_REGS(reguse, reguse + 1);
#endif
				reguse += 2;
				md->argintreguse = reguse;
			}
			else {
				pd->inmemory = true;
				pd->regoff   = stacksize;
			}
			stacksize += 2;
			break;
		}


#endif /* !defined(ENABLE_SOFT_FLOAT) */

#endif /* SIZEOF_VOID_P == 8 */
	}

	/* fill register and stack usage */

	md->memuse = stacksize;
}


/* md_param_alloc_native *******************************************************

   Pre-allocate arguments according the native ABI.

*******************************************************************************/

void md_param_alloc_native(methoddesc *md)
{
	/* On MIPS we use the same ABI for JIT method calls as for native
	   method calls. */

	md_param_alloc(md);
}


/* md_return_alloc *************************************************************

   Precolor the Java Stackelement containing the Return Value. Since
   mips has a dedicated return register (not an reused arg or reserved
   reg), this is striaghtforward possible, as long, as this
   stackelement does not have to survive a method invokation
   (SAVEDVAR)

   --- in
   jd:                      jitdata of the current method
   stackslot:               Java Stackslot to contain the Return Value
   
   --- out
   if precoloring was possible:
   VAR(stackslot->varnum)->flags       = PREALLOC
   			             ->regoff      = [REG_RESULT|REG_FRESULT]

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
		VAR(stackslot->varnum)->flags = PREALLOC;

		if (IS_INT_LNG_TYPE(md->returntype.type)) {
#if SIZEOF_VOID_P == 4
			if (IS_2_WORD_TYPE(md->returntype.type))
				VAR(stackslot->varnum)->vv.regoff = REG_RESULT_PACKED;
			else
#endif
				VAR(stackslot->varnum)->vv.regoff = REG_RESULT;
		}
		else
			VAR(stackslot->varnum)->vv.regoff = REG_FRESULT;
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
