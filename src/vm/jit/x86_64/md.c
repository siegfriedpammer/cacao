/* src/vm/jit/x86_64/md.c - machine dependent x86_64 Linux functions

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

   Authors: Christian Thalinger

   Changes:

   $Id: md.c 2991 2005-07-11 21:25:31Z twisti $

*/


#define _GNU_SOURCE

#include <stdlib.h>
#include <ucontext.h>

#include "config.h"
#include "vm/jit/x86_64/md-abi.h"

#include "vm/exceptions.h"
#include "vm/options.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"


/* md_init *********************************************************************

   Do some machine dependent initialization.

*******************************************************************************/

void md_init(void)
{
	/* nothing to do */
}


/* signal_handler_sigsegv ******************************************************

   NullPointerException signal handler for hardware null pointer check.

*******************************************************************************/

void signal_handler_sigsegv(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t  *_uc;
	mcontext_t  *_mc;
	u1          *sp;
	functionptr  ra;
	functionptr  xpc;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;

	/* ATTENTION: don't use CACAO internal REG_* defines as they are          */
	/* different to the ones in <ucontext.h>                                  */

	sp = (u1 *) _mc->gregs[REG_RSP];
	xpc = (functionptr) _mc->gregs[REG_RIP];
	ra = xpc;                           /* return address is equal to xpc     */

	_mc->gregs[REG_RAX] =
		(ptrint) stacktrace_hardware_nullpointerexception(NULL, sp, ra, xpc);

	_mc->gregs[REG_R10] = (ptrint) xpc;                      /* REG_ITMP2_XPC */
	_mc->gregs[REG_RIP] = (ptrint) asm_handle_exception;
}


/* signal_handler_sigfpe *******************************************************

   ArithmeticException signal handler for hardware divide by zero check.

*******************************************************************************/

void signal_handler_sigfpe(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t  *_uc;
	mcontext_t  *_mc;
	u1          *sp;
	functionptr  ra;
	functionptr  xpc;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;

	/* ATTENTION: don't use CACAO internal REG_* defines as they are          */
	/* different to the ones in <ucontext.h>                                  */

	sp = (u1 *) _mc->gregs[REG_RSP];
	xpc = (functionptr) _mc->gregs[REG_RIP];
	ra = xpc;                           /* return address is equal to xpc     */

	_mc->gregs[REG_RAX] =
		(ptrint) stacktrace_hardware_arithmeticexception(NULL, sp, ra, xpc);

	_mc->gregs[REG_R10] = (ptrint) xpc;                      /* REG_ITMP2_XPC */
	_mc->gregs[REG_RIP] = (ptrint) asm_handle_exception;
}


#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void thread_restartcriticalsection(ucontext_t *uc)
{
	void *critical;

	critical = thread_checkcritical((void *) uc->uc_mcontext.gregs[REG_RIP]);

	if (critical)
		uc->uc_mcontext.gregs[REG_RIP] = (ptrint) critical;
}
#endif


/* md_stacktrace_get_returnaddress *********************************************

   Returns the return address of the current stackframe, specified by
   the passed stack pointer and the stack frame size.

*******************************************************************************/

functionptr md_stacktrace_get_returnaddress(u1 *sp, u4 framesize)
{
	functionptr ra;

	/* on x86_64 the return address is above the current stack frame */

	ra = (functionptr) (ptrint) *((u1 **) (sp + framesize));

	return ra;
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
