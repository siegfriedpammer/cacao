/* src/vm/jit/mips/linux/md-os.c - machine dependent MIPS Linux functions

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

   Contact: cacao@cacaojvm.org

   Authors: Andreas Krall
            Reinhard Grafl
            Christian Thalinger

   $Id: md-os.c 7206 2007-01-11 22:39:52Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <sgidefs.h> /* required for _MIPS_SIM_ABI* defines (before signal.h) */
#include <signal.h>
#include <ucontext.h>

#include "vm/types.h"

#include "vm/jit/mips/md-abi.h"

#include "mm/gc-common.h"
#include "vm/exceptions.h"
#include "vm/signallocal.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/stacktrace.h"


/* md_init *********************************************************************

   Do some machine dependent initialization.

*******************************************************************************/

void md_init(void)
{
	/* The Boehm GC initialization blocks the SIGSEGV signal. So we do a      */
	/* dummy allocation here to ensure that the GC is initialized.            */

#if defined(ENABLE_GC_BOEHM)
	heap_allocate(1, 0, NULL);
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
	ucontext_t  *_uc;
	mcontext_t  *_mc;
	greg_t      *_gregs;
	u4           instr;
	ptrint       addr;
	u1          *pv;
	u1          *sp;
	u1          *ra;
	u1          *xpc;

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

#if defined(__UCLIBC__)
	xpc = (u1 *) (ptrint) _gregs[CTX_EPC];
#else
	xpc = (u1 *) (ptrint) _mc->pc;
#endif

	instr = *((u4 *) xpc);
	addr  = _gregs[(instr >> 21) & 0x1f];

	if (addr == 0) {
		_gregs[REG_ITMP1_XPTR] =
			(ptrint) stacktrace_hardware_nullpointerexception(pv, sp, ra, xpc);

		_gregs[REG_ITMP2_XPC] = (ptrint) xpc;

#if defined(__UCLIBC__)
		_gregs[CTX_EPC] = (ptrint) asm_handle_exception;
#else
		_mc->pc         = (ptrint) asm_handle_exception;
#endif
	}
	else {
		addr += (long) ((instr << 16) >> 16);

		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "faulting address: 0x%lx at 0x%lx\n",
								   addr, xpc);
	}
}


/* md_signal_handler_sigusr2 ***************************************************

   DOCUMENT ME

*******************************************************************************/

void md_signal_handler_sigusr2(int sig, siginfo_t *siginfo, void *_p)
{
}


#if defined(ENABLE_THREADS)
void thread_restartcriticalsection(ucontext_t *_uc)
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
