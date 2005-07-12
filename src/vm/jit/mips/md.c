/* src/vm/jit/mips/md.c - machine dependent MIPS functions

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

   Authors: Andreas Krall
            Reinhard Grafl

   Changes: Christian Thalinger

   $Id: md.c 3023 2005-07-12 23:49:49Z twisti $

*/


#include <assert.h>
#include <signal.h>
#include <sys/fpu.h>
#include <sys/mman.h>
#include <unistd.h>

#include "config.h"

#include "vm/jit/mips/md-abi.h"
#include "vm/jit/mips/types.h"

#include "vm/exceptions.h"
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

	heap_allocate(1, 0, NULL);


	/* Turn off flush-to-zero */

	{
		union fpc_csr n;
		n.fc_word = get_fpc_csr();
		n.fc_struct.flush = 0;
		set_fpc_csr(n.fc_word);
	}
}


/* signal_handler_sigsegv ******************************************************

   NullPointerException signal handler for hardware null pointer check.

*******************************************************************************/

void signal_handler_sigsegv(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t  *_uc;
	mcontext_t  *_mc;
	u4           instr;
	ptrint       addr;
	u1          *pv;
	u1          *sp;
	functionptr  ra;
	functionptr  xpc;

	_uc = (struct ucontext *) _p;
	_mc = &_uc->uc_mcontext;

	instr = *((u4 *) (_mc->gregs[CTX_EPC]));
	addr = _mc->gregs[(instr >> 21) & 0x1f];

	if (addr == 0) {
		pv  = (u1 *) _mc->gregs[REG_PV];
		sp  = (u1 *) _mc->gregs[REG_SP];
		ra  = (functionptr) _mc->gregs[REG_RA]; /* this is correct for leafs*/
		xpc = (functionptr) _mc->gregs[CTX_EPC];

		_mc->gregs[REG_ITMP1_XPTR] =
			(ptrint) stacktrace_hardware_nullpointerexception(pv, sp, ra, xpc);

		_mc->gregs[REG_ITMP2_XPC] = (ptrint) xpc;
		_mc->gregs[CTX_EPC] = (ptrint) asm_handle_exception;

	} else {
        addr += (long) ((instr << 16) >> 16);

		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "faulting address: 0x%lx at 0x%lx\n",
								   addr, _mc->gregs[CTX_EPC]);
	}
}


#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void thread_restartcriticalsection(ucontext_t *uc)
{
	void *critical;

	critical = thread_checkcritical((void*) uc->uc_mcontext.gregs[CTX_EPC]);

	if (critical)
		uc->uc_mcontext.gregs[CTX_EPC] = (ptrint) critical;
}
#endif


void docacheflush(u1 *p, long bytelen)
{
	u1 *e = p + bytelen;
	long psize = sysconf(_SC_PAGESIZE);
	p -= (long) p & (psize - 1);
	e += psize - ((((long) e - 1) & (psize - 1)) + 1);
	bytelen = e-p;
	mprotect(p, bytelen, PROT_READ | PROT_WRITE | PROT_EXEC);
}


/* md_stacktrace_get_returnaddress *********************************************

   Returns the return address of the current stackframe, specified by
   the passed stack pointer and the stack frame size.

*******************************************************************************/

functionptr md_stacktrace_get_returnaddress(u1 *sp, u4 framesize)
{
	functionptr ra;

	/* on MIPS the return address is located on the top of the stackframe */

	ra = (functionptr) *((u1 **) (sp + framesize - SIZEOF_VOID_P));

	return ra;
}


/* codegen_findmethod **********************************************************

   Machine code:

   6b5b4000    jsr     (pv)
   237affe8    lda     pv,-24(ra)

*******************************************************************************/

functionptr codegen_findmethod(functionptr pc)
{
	u1 *ra;
	u1 *pv;
	u4  mcode;
	s2  offset;

	ra = (u1 *) pc;
	pv = ra;

	/* get offset of first instruction (lda) */

	mcode = *((u4 *) ra);

	if ((mcode >> 16) != 0x67fe) {
		log_text("No `daddiu s8,ra,x' instruction found on return address!");
		assert(0);
	}

	offset = (s2) (mcode & 0x0000ffff);
	pv += offset;

#if 0
	/* XXX TWISTI: implement this! */

	/* check for second instruction (ldah) */

	mcode = *((u4 *) (ra + 1 * 4));

	if ((mcode >> 16) == 0x177b) {
		offset = (s2) (mcode << 16);
		pv += offset;
	}
#endif

	return (functionptr) pv;
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
