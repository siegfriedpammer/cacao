/* src/vm/jit/x86_64/md-abi.c - functions for x86_64 Linux ABI

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

   $Id: md-abi.c 7468 2007-03-06 11:12:03Z twisti $

*/


#include "config.h"
#include "vm/types.h"

#include "vm/jit/x86_64/md-abi.h"

#include "vm/global.h"

#include "vm/jit/jit.h" /* for REG_* (maybe can be removed) */

#include "vmcore/descriptor.h"


/* register descripton array **************************************************/

s4 nregdescint[] = {
    REG_RET, REG_ARG, REG_ARG, REG_TMP, REG_RES, REG_SAV, REG_ARG, REG_ARG,
    REG_ARG, REG_ARG, REG_RES, REG_RES, REG_SAV, REG_SAV, REG_SAV, REG_SAV,
    REG_END
};

const char *abi_registers_integer_name[] = {
	"rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
	"r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15"
};


s4 nregdescfloat[] = {
    REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG,
    REG_RES, REG_RES, REG_RES, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP,
    REG_END
};


/* md_param_alloc **************************************************************

   XXX

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
	stacksize = 0;

	/* get params field of methoddesc */

	pd = md->params;

	for (i = 0; i < md->paramcount; i++, pd++) {
		switch (md->paramtypes[i].type) {
		case TYPE_INT:
		case TYPE_ADR:
		case TYPE_LNG:
			if (iarg < INT_ARG_CNT) {
				pd->inmemory = false;
				pd->regoff   = iarg;
				iarg++;
			}
			else {
				pd->inmemory = true;
				pd->regoff   = stacksize;
				stacksize++;
			}
			break;

		case TYPE_FLT:
		case TYPE_DBL:
			if (farg < FLT_ARG_CNT) {
				pd->inmemory = false;
				pd->regoff   = farg;
				farg++;
			}
			else {
				pd->inmemory = true;
				pd->regoff   = stacksize;
				stacksize++;
			}
			break;
		}
	}

	/* Since XMM0 (==A0) is used for passing return values, this
	   argument register usage has to be regarded, too. */

	if (IS_FLT_DBL_TYPE(md->returntype.type))
		if (farg < 1)
			farg = 1;

	/* fill register and stack usage */

	md->argintreguse = iarg;
	md->argfltreguse = farg;
	md->memuse       = stacksize;
}


/* md_param_alloc_native *******************************************************

   Pre-allocate arguments according the native ABI.

*******************************************************************************/

void md_param_alloc_native(methoddesc *md)
{
	/* On x86_64 we use the same ABI for JIT method calls as for
	   native method calls. */

	md_param_alloc(md);
}


/* md_return_alloc *************************************************************

   Precolor the Java Stackelement containing the Return Value. Only
   for float/ double types straight forward possible, since INT_LNG
   types use "reserved" registers Float/Double values use a00 as
   return register.

   --- in
   jd:                      jitdata of the current method
   stackslot:               Java Stackslot to contain the Return Value

   --- out
   if precoloring was possible:
   VAR(stackslot->varnum)->flags     = PREALLOC
   			             ->vv.regoff = [REG_RESULT|REG_FRESULT]
   rd->arg[flt|int]reguse   set to a value according the register usage

   NOTE: Do not pass a LOCALVAR in stackslot->varnum.

*******************************************************************************/

void md_return_alloc(jitdata *jd, stackptr stackslot)
{
	methodinfo   *m;
	registerdata *rd;
	methoddesc   *md;

	/* get required compiler data */

	m  = jd->m;
	rd = jd->rd;

	md = m->parseddesc;

	/* precoloring only straightforward possible with flt/dbl types
	   For Address/Integer/Long REG_RESULT == rax == REG_ITMP1 and so
	   could be destroyed if the return value Stack Slot "lives too
	   long" */

	if (IS_FLT_DBL_TYPE(md->returntype.type)) {
		/* In Leafmethods Local Vars holding parameters are precolored
		   to their argument register -> so leafmethods with
		   paramcount > 0 could already use a00! */

		if (!jd->isleafmethod || (md->paramcount == 0)) {
			/* Only precolor the stackslot, if it is not a SAVEDVAR
			   <-> has not to survive method invokations */

			if (!(stackslot->flags & SAVEDVAR)) {

				VAR(stackslot->varnum)->flags = PREALLOC;

			    /* float/double */
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
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
