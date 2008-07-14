/* src/vm/jit/m68k/linux/md-os.c - m68k linux specific functions

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

#include "vm/jit/m68k/md.h"
#include "vm/jit/m68k/linux/md-abi.h"

#include "vm/exceptions.h"
#include "vm/signallocal.h"
#include "vm/vm.hpp"

#include "vm/jit/asmpart.h"
#include "vm/jit/trap.h"

#include <assert.h>
#include <stdlib.h>
/*#include <signal.h>*/
#include <stdint.h>
#include <sys/ucontext.h> /* has br0ken ucontext_t*/

/*
 *  The glibc bundeled with my devels system ucontext.h differs with kernels ucontext.h. Not good.
 *  The following stuff is taken from 2.6.10 + freescale coldfire patches:
 */
typedef struct actual_fpregset {
	int f_fpcntl[3];
	int f_fpregs[8*3];
} actual_fpregset_t;

typedef struct actual_mcontext {
	int version;
	gregset_t gregs;	/* 0...7 = %d0-%d7, 8...15 = %a0-%a7 */
	actual_fpregset_t fpregs;
} actual_mcontext_t;

#define GREGS_ADRREG_OFF	8

typedef struct actual_ucontext {
	unsigned long     uc_flags;
	struct actual_ucontext  *uc_link;
	stack_t           uc_stack;
	struct actual_mcontext   uc_mcontext;
	unsigned long     uc_filler[80];
	sigset_t          uc_sigmask;   /* mask last for extensibility */
} actual_ucontext_t;



/* md_signal_handler_sigsegv ******************************************
 *
 * Invoked when a Nullpointerexception occured, or when the vm 
 * crashes, hard to tell the difference. 
 **********************************************************************/

void md_signal_handler_sigsegv(int sig, siginfo_t *siginfo, void *_p)
{ 	
	uint32_t	xpc, sp;
	uint16_t	opc;
	uint32_t 	val, regval, off;
	bool		adrreg;
	void        *p;
	actual_mcontext_t 	*_mc;
	actual_ucontext_t	*_uc;
	int type;

	_uc = (actual_ucontext_t*)_p;
	_mc = &_uc->uc_mcontext;
	sp = _mc->gregs[R_SP];
	xpc = _mc->gregs[R_PC];
	opc = *(uint16_t*)xpc;

	/* m68k uses a whole bunch of difficult to differ load instructions where null pointer check could occure */
	/* the first two are 2*32bit sized */
	adrreg = false;
	off = 0;
	if ((opc & ((2<<12) | (5<<3))) == ((2<<12) | (5<<3)))	{
		if (opc & (1<<6)) adrreg = true;		/* M_XLD */
		val = opc & 0x0007;
		off = *(uint16_t*)(xpc+1);
	} else if ((opc & ((2<<12) | (5<<6))) == ((2<<12) | (5<<6)))	{
		if (opc & (1<<3)) adrreg = true;		/* M_XST */
		val = (opc >> 9) & 0x0007;
		off = *(uint16_t*)(xpc+1);
	} else {
		
		/*fprintf(stderr, "SEGV: short instructions %x\n", opc);
		*/
		/* now check the 32 bit sized instructions */
		if ((opc & (2<<3)) == (2<<3))	{
			if (opc & (1<<6)) adrreg = true;		/* M_L*X */
			val = opc & 0x0007;
		} else if ((opc & (2<<6)) == (2<<6))	{
			if (opc & (1<<3)) adrreg = true;		/* M_S*X */
			val = (opc >> 9) & 0x0007;
		} else {
			vm_abort("md_signal_handler_sigsegv: unhandeled faulting opcode %x", opc);
		}
	}

	/* val is now register number, adreg == true if it is an address regsiter */
	regval = _mc->gregs[adrreg ? GREGS_ADRREG_OFF + val : val];
	type   = regval;

	/* Handle the type. */

	p = trap_handle(type, regval, NULL, (void*)sp, (void*)xpc, (void*)xpc, _p);

	_mc->gregs[GREGS_ADRREG_OFF + REG_ATMP1]     = (intptr_t) p;
	_mc->gregs[GREGS_ADRREG_OFF + REG_ATMP2_XPC] = (intptr_t) xpc;
	_mc->gregs[R_PC]                             = (intptr_t) asm_handle_exception;
}


/* md_signal_handler_sigill *******************************************
 *
 * This handler is used to generate hardware exceptions.
 * Type of exception derived from trap number.
 * If an object is needed a tst instruction (2 byte long) has
 * been created directly before the trap instruction (2 bytes long).
 * the last 3 bit of this tst instruction contain the register number.
 **********************************************************************/

void md_signal_handler_sigill(int sig, siginfo_t *siginfo, void *_p)
{
	uint32_t	xpc, sp, ra, pv;
	uint16_t	opc;
	uint32_t	type;
	uint32_t	regval;
	void        *p;
	actual_mcontext_t 	*_mc;
	actual_ucontext_t 	*_uc;

	_uc = (actual_ucontext_t*)_p;
	xpc = (uint32_t)siginfo->si_addr;

	pv = NULL;	/* we have no pv */
	ra = xpc;	/* that is ture for most cases */

	if (siginfo->si_code == ILL_ILLOPC)	{
		vm_abort("md_signal_handler_sigill: the illegal instruction @ 0x%x, aborting", xpc);
	}
	if (siginfo->si_code != ILL_ILLTRP)	{
		vm_abort("md_signal_handler_sigill: Caught something not a trap");
	}

	opc = *(uint16_t*)(xpc-2);

	assert( (opc&0xfff0) == 0x4e40 );
	type = opc & 0x000f;		/* filter trap number */

	_mc = &_uc->uc_mcontext;
	sp = _mc->gregs[R_SP];

	/* Figure out in which register the object causing the exception resides for appropiate exceptions
	 */
	switch (type)	{
		case TRAP_NullPointerException:
		case TRAP_ArithmeticException:
		case TRAP_CHECK_EXCEPTION:
			/* nothing */
			break;

		case TRAP_ClassCastException: 
			regval = *(uint16_t*)(xpc-4);
			assert( (regval&0xfff0) == 0x4a00 );
			/* was in a address register */
			regval = _mc->gregs[ GREGS_ADRREG_OFF + (regval & 0x7) ];
			break;

		case TRAP_ArrayIndexOutOfBoundsException:
			regval = *(uint16_t*)(xpc-4);
			assert( (regval&0xfff0) == 0x4a00 );
			/* was a data register */
			regval = _mc->gregs[regval & 0x7];
			break;

		case TRAP_COMPILER:
			regval = *(uint16_t*)(xpc-4);
			assert( (regval&0xfff0) == 0x4a00 );
			/* was in a address register */
			regval = _mc->gregs[ GREGS_ADRREG_OFF + (regval & 0x7) ];

			pv = xpc-4;	/* the compiler stub consists of 2 instructions */
			ra = md_stacktrace_get_returnaddress(sp, 0);
			sp = sp + SIZEOF_VOID_P;
			xpc = ra - 2;
			break;

		case TRAP_PATCHER:
			xpc -= 2;
			ra = xpc;
			break;
	}

	/* Handle the trap. */

	p = trap_handle(type, regval, pv, (void*)sp, (void*)ra, (void*)xpc, _p);

	switch (type)	{
	case TRAP_COMPILER:
		if (p == NULL) {
			/* exception when compiling the method */
			java_object_t *o = builtin_retrieve_exception();

			_mc->gregs[R_SP] = sp;	/* remove RA from stack */

			_mc->gregs[GREGS_ADRREG_OFF + REG_ATMP1]     = (uintptr_t) o;
			_mc->gregs[GREGS_ADRREG_OFF + REG_ATMP2_XPC] = (uintptr_t) xpc;
			_mc->gregs[R_PC]                             = (uintptr_t) asm_handle_exception;
		}
		else {
			/* compilation ok, execute */
			_mc->gregs[R_PC] = p;
		}
		break;

	case TRAP_PATCHER:
		if (p == NULL) { 
			/* No exception while patching, continue. */
			_mc->gregs[R_PC] = xpc;	
			return;	
		}
		/* fall-through in case of exception */

	default:
		/* a normal exception with normal expcetion handling */
		_mc->gregs[GREGS_ADRREG_OFF + REG_ATMP1]     = (uintptr_t) p;
		_mc->gregs[GREGS_ADRREG_OFF + REG_ATMP2_XPC] = (uintptr_t) xpc;
		_mc->gregs[R_PC]                             = (uintptr_t) asm_handle_exception;
	}
}


/* md_signal_handler_sigusr1 ***************************************************

   Signal handler for suspending threads.

*******************************************************************************/

#if defined(ENABLE_THREADS) && defined(ENABLE_GC_CACAO)
void md_signal_handler_sigusr1(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t *_uc;
	mcontext_t *_mc;
	u1         *pc;
	u1         *sp;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;

	/* get the PC and SP for this thread */
	pc = (u1 *) _mc->gregs[R_PC];
	sp = (u1 *) _mc->gregs[R_SP];

	/* now suspend the current thread */
	threads_suspend_ack(pc, sp);
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
