/* src/vm/jit/mips/irix/md-os.c - machine dependent MIPS IRIX functions

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

   $Id: md-os.c 8179 2007-07-05 11:21:08Z michi $

*/


#include "config.h"

#include <assert.h>
#include <signal.h>
#include <sys/fpu.h>

#include "vm/types.h"

#include "vm/jit/mips/codegen.h"
#include "vm/jit/mips/md-abi.h"

#include "mm/gc-common.h"

#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/signallocal.h"
#include "vm/stringlocal.h"

#include "vm/jit/asmpart.h"
#include "vm/jit/codegen-common.h"
#include "vm/jit/stacktrace.h"


/* md_init *********************************************************************

   Do some machine dependent initialization.

*******************************************************************************/

void md_init(void)
{
	/* The Boehm GC initialization blocks the SIGSEGV signal. So we do a      */
	/* dummy allocation here to ensure that the GC is initialized.            */

#if defined(ENABLE_GC_BOEHM)
	(void) GCNEW(u1);
#endif


	/* Turn off flush-to-zero */

	{
		union fpc_csr n;
		n.fc_word = get_fpc_csr();
		n.fc_struct.flush = 0;
		set_fpc_csr(n.fc_word);
	}
}


/* md_signal_handler_sigsegv ***************************************************

   Signal handler for hardware-exceptions.

*******************************************************************************/

void md_signal_handler_sigsegv(int sig, siginfo_t *siginfo, void *_p)
{
	stackframeinfo     sfi;
	ucontext_t        *_uc;
	mcontext_t        *_mc;
	u1                *pv;
	u1                *sp;
	u1                *ra;
	u1                *xpc;
	u4                 mcode;
	s4                 d;
	s4                 s1;
	s4                 disp;
	ptrint             val;
	ptrint             addr;
	s4                 type;
	java_objectheader *o;

	_uc = (struct ucontext *) _p;
	_mc = &_uc->uc_mcontext;

	pv  = (u1 *) _mc->gregs[REG_PV];
	sp  = (u1 *) _mc->gregs[REG_SP];
	ra  = (u1 *) _mc->gregs[REG_RA];             /* this is correct for leafs */
	xpc = (u1 *) _mc->gregs[CTX_EPC];

	/* get exception-throwing instruction */

	mcode = *((u4 *) xpc);

	d    = M_ITYPE_GET_RT(mcode);
	s1   = M_ITYPE_GET_RS(mcode);
	disp = M_ITYPE_GET_IMM(mcode);

	/* check for special-load */

	if (s1 == REG_ZERO) {
		/* we use the exception type as load displacement */

		type = disp;
		val  = _mc->gregs[d];
	}
	else {
		/* This is a normal NPE: addr must be NULL and the NPE-type
		   define is 0. */

		addr = _mc->gregs[s1];
		type = (s4) addr;
		val  = 0;
	}

	/* generate appropriate exception */

	o = exceptions_new_hardware_exception(pv, sp, ra, xpc, type, val, &sfi);

	/* set registers */

	_mc->gregs[REG_ITMP1_XPTR] = (ptrint) o;
	_mc->gregs[REG_ITMP2_XPC]  = (ptrint) xpc;
	_mc->gregs[CTX_EPC]        = (ptrint) asm_handle_exception;
}


/* md_critical_section_restart *************************************************

   Search the critical sections tree for a matching section and set
   the PC to the restart point, if necessary.

*******************************************************************************/

#if defined(ENABLE_THREADS)
void md_critical_section_restart(ucontext_t *_uc)
{
	mcontext_t *_mc;
	u1         *pc;
	u1         *npc;

	_mc = &_uc->uc_mcontext;

	pc = (u1 *) _mc->gregs[CTX_EPC];

	npc = critical_find_restart_point(pc);

	if (npc != NULL) {
		log_println("md_critical_section_restart: pc=%p, npc=%p", pc, npc);
		_mc->gregs[CTX_EPC] = (ptrint) npc;
	}
}
#endif


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
