/* src/vm/jit/mips/linux/md-os.c - machine dependent MIPS Linux functions

   Copyright (C) 1996-2005, 2006, 2007, 2008
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

#include <assert.h>
#include <sgidefs.h> /* required for _MIPS_SIM_ABI* defines (before signal.h) */
#include <signal.h>
#include <stdint.h>
#include <ucontext.h>

#include "vm/types.h"

#include "vm/jit/mips/codegen.h"
#include "vm/jit/mips/md.h"
#include "vm/jit/mips/md-abi.h"

#include "mm/gc-common.h"
#include "mm/memory.h"

#include "vm/exceptions.h"
#include "vm/signallocal.h"

#include "vm/jit/asmpart.h"
#include "vm/jit/stacktrace.h"
#include "vm/jit/trap.h"


/* md_init *********************************************************************

   Do some machine dependent initialization.

*******************************************************************************/

void md_init(void)
{
	/* The Boehm GC initialization blocks the SIGSEGV signal. So we do
	   a dummy allocation here to ensure that the GC is
	   initialized. */

#if defined(ENABLE_GC_BOEHM)
	(void) GCNEW(int);
#endif

#if 0
	/* Turn off flush-to-zero */

	{
		union fpc_csr n;
		n.fc_word = get_fpc_csr();
		n.fc_struct.flush = 0;
		set_fpc_csr(n.fc_word);
	}
#endif
}


/* md_signal_handler_sigsegv ***************************************************

   NullPointerException signal handler for hardware null pointer
   check.

*******************************************************************************/

void md_signal_handler_sigsegv(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t     *_uc;
	mcontext_t     *_mc;
	greg_t         *_gregs;
	u1             *pv;
	u1             *sp;
	u1             *ra;
	u1             *xpc;
	unsigned int    cause;
	u4              mcode;
	int             d;
	int             s1;
	int16_t         disp;
	intptr_t        val;
	intptr_t        addr;
	int             type;
	void           *p;

	_uc    = (struct ucontext *) _p;
	_mc    = &_uc->uc_mcontext;

#if defined(__UCLIBC__)
	_gregs = _mc->gpregs;
#else	
	_gregs = _mc->gregs;
#endif

	/* In glibc's ucontext.h the registers are defined as long long,
	   even for MIPS32, so we cast them.  This is not the case for
	   uClibc. */

	pv  = (u1 *) (ptrint) _gregs[REG_PV];
	sp  = (u1 *) (ptrint) _gregs[REG_SP];
	ra  = (u1 *) (ptrint) _gregs[REG_RA];        /* this is correct for leafs */

#if !defined(__UCLIBC__) && ((__GLIBC__ == 2) && (__GLIBC_MINOR__ < 5))
	/* NOTE: We only need this for pre glibc-2.5. */

	xpc = (u1 *) (ptrint) _mc->pc;

	/* get the cause of this exception */

	cause = _mc->cause;

	/* check the cause to find the faulting instruction */

	/* TODO: use defines for that stuff */

	switch (cause & 0x0000003c) {
	case 0x00000008:
		/* TLBL: XPC is ok */
		break;

	case 0x00000010:
		/* AdEL: XPC is of the following instruction */
		xpc = xpc - 4;
		break;
	}
#else
	xpc = (u1 *) (ptrint) _gregs[CTX_EPC];
#endif

	/* get exception-throwing instruction */

	mcode = *((u4 *) xpc);

	d    = M_ITYPE_GET_RT(mcode);
	s1   = M_ITYPE_GET_RS(mcode);
	disp = M_ITYPE_GET_IMM(mcode);

	/* check for special-load */

	if (s1 == REG_ZERO) {
		/* we use the exception type as load displacement */

		type = disp;
		val  = _gregs[d];

		if (type == TRAP_COMPILER) {
			/* The XPC is the RA minus 4, because the RA points to the
			   instruction after the call. */

			xpc = ra - 4;
		}
	}
	else {
		/* This is a normal NPE: addr must be NULL and the NPE-type
		   define is 0. */

		addr = _gregs[s1];
		type = (int) addr;
		val  = 0;
	}

	/* Handle the trap. */

	p = trap_handle(type, val, pv, sp, ra, xpc, _p);

	/* Set registers. */

	switch (type) {
	case TRAP_COMPILER:
		if (p != NULL) {
			_gregs[REG_PV]  = (uintptr_t) p;
#if defined(__UCLIBC__)
			_gregs[CTX_EPC] = (uintptr_t) p;
#else
			_mc->pc         = (uintptr_t) p;
#endif
			break;
		}

		/* Get and set the PV from the parent Java method. */

		pv = md_codegen_get_pv_from_pc(ra);

		_gregs[REG_PV] = (uintptr_t) pv;

		/* Get the exception object. */

		p = builtin_retrieve_exception();

		assert(p != NULL);

		/* fall-through */

	case TRAP_PATCHER:
		if (p == NULL) {
			/* We set the PC again because the cause may have changed
			   the XPC. */

#if defined(__UCLIBC__)
			_gregs[CTX_EPC] = (uintptr_t) xpc;
#else
			_mc->pc         = (uintptr_t) xpc;
#endif
			break;
		}

		/* fall-through */
		
	default:
		_gregs[REG_ITMP1_XPTR] = (uintptr_t) p;
		_gregs[REG_ITMP2_XPC]  = (uintptr_t) xpc;
#if defined(__UCLIBC__)
		_gregs[CTX_EPC]        = (uintptr_t) asm_handle_exception;
#else
		_mc->pc                = (uintptr_t) asm_handle_exception;
#endif
	}
}


/* md_signal_handler_sigusr2 ***************************************************

   DOCUMENT ME

*******************************************************************************/

void md_signal_handler_sigusr2(int sig, siginfo_t *siginfo, void *_p)
{
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

#if defined(__UCLIBC__)
	pc = (u1 *) (ptrint) _mc->gpregs[CTX_EPC];
#else
	pc = (u1 *) (ptrint) _mc->pc;
#endif

	npc = critical_find_restart_point(pc);

	if (npc != NULL) {
#if defined(__UCLIBC__)
		_mc->gpregs[CTX_EPC] = (ptrint) npc;
#else
		_mc->pc              = (ptrint) npc;
#endif
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
 * vim:noexpandtab:sw=4:ts=4:
 */
