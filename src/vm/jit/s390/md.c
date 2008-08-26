/* src/vm/jit/s390/md.c - machine dependent s390 Linux functions

   Copyright (C) 2006, 2007, 2008
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


#define _GNU_SOURCE

#include "config.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <ucontext.h>

#include "vm/jit/s390/md-abi.h"

#include "threads/thread.hpp"

#include "vm/exceptions.hpp"
#include "vm/signallocal.h"

#include "vm/jit/asmpart.h"
#include "vm/jit/abi.h"
#include "vm/jit/executionstate.h"
#include "vm/jit/methodheader.h"
#include "vm/jit/methodtree.h"
#include "vm/jit/stacktrace.hpp"
#include "vm/jit/trap.h"

#if !defined(NDEBUG) && defined(ENABLE_DISASSEMBLER)
#include "vm/options.h" /* XXX debug */
#include "vm/jit/disass.h" /* XXX debug */
#endif

#include "vm/jit/codegen-common.hpp"
#include "vm/jit/s390/codegen.h"
#include "vm/jit/s390/md.h"


/* prototypes *****************************************************************/

u1 *exceptions_handle_exception(java_object_t *xptro, u1 *xpc, u1 *pv, u1 *sp);

void md_signal_handler_sigill(int sig, siginfo_t *siginfo, void *_p);

void md_dump_context(u1 *pc, mcontext_t *mc);

/* md_init *********************************************************************

   Do some machine dependent initialization.

*******************************************************************************/

void md_init(void)
{
}

/* md_dump_context ************************************************************
 
   Logs the machine context
  
*******************************************************************************/

void md_dump_context(u1 *pc, mcontext_t *mc) {
	int i;
	u1 *pv;
	methodinfo *m;

	union {
		u8 l;
		fpreg_t fr;
	} freg;

	log_println("Dumping context.");

	log_println("Program counter: 0x%08X", pc);

	pv = methodtree_find_nocheck(pc);

	if (pv == NULL) {
		log_println("No java method found at location.");
	} else {
		m = (*(codeinfo **)(pv + CodeinfoPointer))->m;
		log_println(
			"Java method: class %s, method %s, descriptor %s.",
			m->clazz->name->text, m->name->text, m->descriptor->text
		);
	}

#if defined(ENABLE_DISASSEMBLER)
	log_println("Printing instruction at program counter:");
	disassinstr(pc);
#endif

	log_println("General purpose registers:");

	for (i = 0; i < 16; i++) {
		log_println("\tr%d:\t0x%08X\t%d", i, mc->gregs[i], mc->gregs[i]);
	}

	log_println("Floating point registers:");

	for (i = 0; i < 16; i++) {
		freg.fr.d = mc->fpregs.fprs[i].d;
		log_println("\tf%d\t0x%016llX\t(double)%e\t(float)%f", i, freg.l, freg.fr.d, freg.fr.f);
	}

	log_println("Dumping the current stacktrace:");
	stacktrace_print_current();
}

/* md_signal_handler_sigsegv ***************************************************

   NullPointerException signal handler for hardware null pointer
   check.

*******************************************************************************/

void md_signal_handler_sigsegv(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t     *_uc;
	mcontext_t     *_mc;
	u1             *pv;
	u1             *sp;
	u1             *ra;
	u1             *xpc;
	int             type;
	intptr_t        val;
	void           *p;
	s4              base;
	s4              is_null;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;

	xpc = (u1 *)_mc->psw.addr;

	/* Check opcodes and verify it is a null pointer exception */

	switch (N_RX_GET_OPC(xpc)) {
		case OPC_L:
		case OPC_ST:
		case OPC_CL: /* array size check on NULL array */
			base = N_RX_GET_BASE(xpc);
			if (base == 0) {
				is_null = 1;
			} else if (_mc->gregs[base] == 0) {
				is_null = 1;
			} else {
				is_null = 0;
			}
			break;
		default:
			is_null = 0;
			break;
	}

	if (! is_null) {
#if !defined(NDEBUG)
		md_dump_context(xpc, _mc);
#endif
		vm_abort("%s: segmentation fault at %p, aborting.", __FUNCTION__, xpc);
	}

	pv = (u1 *)_mc->gregs[REG_PV] - N_PV_OFFSET;
	sp = (u1 *)_mc->gregs[REG_SP];
	ra = xpc;
	type = TRAP_NullPointerException;
	val = 0;

	/* Handle the trap. */

	p = trap_handle(type, val, pv, sp, ra, xpc, _p);

	if (p != NULL) {
		_mc->gregs[REG_ITMP3_XPTR] = (uintptr_t) p;
		_mc->gregs[REG_ITMP1_XPC]  = (uintptr_t) xpc;
		_mc->psw.addr              = (uintptr_t) asm_handle_exception;
	}
	else {
		_mc->psw.addr              = (uintptr_t) xpc;
	}
}

void md_signal_handler_sigill(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t     *_uc;
	mcontext_t     *_mc;
	u1             *xpc;
	u1             *ra;
	u1             *pv;
	u1             *sp;
	int             type;
	intptr_t        val;
	void           *p;
	s4              reg;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;
	xpc = ra = siginfo->si_addr;

	/* Our trap instruction has the format: { 0x02, one_byte_of_data }. */

	if ((siginfo->si_code == ILL_ILLOPC) && (N_RR_GET_OPC(xpc) == OPC_ILL)) {

		/* bits 7-4 contain a register holding a value */
		reg = N_ILL_GET_REG(xpc);

		/* bits 3-0 designate the exception type */
		type = N_ILL_GET_TYPE(xpc);

		pv = (u1 *)_mc->gregs[REG_PV] - N_PV_OFFSET;
		sp = (u1 *)_mc->gregs[REG_SP];
		val = (ptrint)_mc->gregs[reg];

		if (TRAP_COMPILER == type) {
			/* The PV from the compiler stub is equal to the XPC. */

			pv = xpc;

			/* The return address in is REG_RA */

			ra = (u1 *)_mc->gregs[REG_RA];

			xpc = ra - 2;
		}

		/* Handle the trap. */

		p = trap_handle(type, val, pv, sp, ra, xpc, _p);

		if (TRAP_COMPILER == type) {
			if (NULL == p) {
				_mc->gregs[REG_ITMP3_XPTR] = (uintptr_t) builtin_retrieve_exception();
				_mc->gregs[REG_ITMP1_XPC]  = (uintptr_t) ra - 2;
				_mc->gregs[REG_PV]         = (uintptr_t) md_codegen_get_pv_from_pc(ra);
				_mc->psw.addr              = (uintptr_t) asm_handle_exception;
			} else {
				_mc->gregs[REG_PV]         = (uintptr_t) p;
				_mc->psw.addr              = (uintptr_t) p;
			}
		} else {
			if (p != NULL) {
				_mc->gregs[REG_ITMP3_XPTR] = (uintptr_t) p;
				_mc->gregs[REG_ITMP1_XPC]  = (uintptr_t) xpc;
				_mc->psw.addr              = (uintptr_t) asm_handle_exception;
			}
			else {
				_mc->psw.addr              = (uintptr_t) xpc;
			}
		}
	} else {
#if !defined(NDEBUG)
		md_dump_context(xpc, _mc);
#endif
		vm_abort("%s: illegal instruction at %p, aborting.", __FUNCTION__, xpc);
	}
}

/* md_signal_handler_sigfpe ****************************************************

   ArithmeticException signal handler for hardware divide by zero
   check.

*******************************************************************************/

void md_signal_handler_sigfpe(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t     *_uc;
	mcontext_t     *_mc;
	u1             *pv;
	u1             *sp;
	u1             *ra;
	u1             *xpc;
	u1             *pc;
	int             r1, r2;
	int             type;
	intptr_t        val;
	void           *p;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;

	/* Instruction that raised signal */
	xpc = siginfo->si_addr;

	/* Check opcodes */

	if (N_RR_GET_OPC(xpc) == OPC_DR) { /* DR */

		r1 = N_RR_GET_REG1(xpc);
		r2 = N_RR_GET_REG2(xpc);

		if (
			(_mc->gregs[r1] == 0xFFFFFFFF) &&
			(_mc->gregs[r1 + 1] == 0x80000000) && 
			(_mc->gregs[r2] == 0xFFFFFFFF)
		) {
			/* handle special case 0x80000000 / 0xFFFFFFFF that fails on hardware */
			/* next instruction */
			pc = (u1 *)_mc->psw.addr;
			/* reminder */
			_mc->gregs[r1] = 0;
			/* quotient */
			_mc->gregs[r1 + 1] = 0x80000000;
			/* continue at next instruction */
			_mc->psw.addr = (ptrint) pc;

			return;
		}
		else if (_mc->gregs[r2] == 0) {
			/* division by 0 */

			pv = (u1 *)_mc->gregs[REG_PV] - N_PV_OFFSET;
			sp = (u1 *)_mc->gregs[REG_SP];
			ra = xpc;

			type = TRAP_ArithmeticException;
			val = 0;

			/* Handle the trap. */

			p = trap_handle(type, val, pv, sp, ra, xpc, _p);

			_mc->gregs[REG_ITMP3_XPTR] = (uintptr_t) p;
			_mc->gregs[REG_ITMP1_XPC]  = (uintptr_t) xpc;
			_mc->psw.addr              = (uintptr_t) asm_handle_exception;

			return;
		}
	}

	/* Could not handle signal */

#if !defined(NDEBUG)
	md_dump_context(xpc, _mc);
#endif
	vm_abort("%s: floating point exception at %p, aborting.", __FUNCTION__, xpc);
}


/* md_signal_handler_sigusr2 ***************************************************

   Signal handler for profiling sampling.

*******************************************************************************/

#if defined(ENABLE_THREADS)
void md_signal_handler_sigusr2(int sig, siginfo_t *siginfo, void *_p)
{
	threadobject *t;
	ucontext_t   *_uc;
	mcontext_t   *_mc;
	u1           *pc;

	t = THREADOBJECT;

	_uc = (ucontext_t *) _p;
	_mc = &_uc->uc_mcontext;

	/* ATTENTION: Don't use CACAO's internal REG_* defines as they are
	   different to the ones in <ucontext.h>. */

	pc = (u1 *) _mc->psw.addr;

	t->pc = pc;
}
#endif


/**
 * Read the given context into an executionstate.
 *
 * @param es      execution state
 * @param context machine context
 */
void md_executionstate_read(executionstate_t* es, void* context)
{
	vm_abort("md_executionstate_read: IMPLEMENT ME!");
}


/**
 * Write the given executionstate back to the context.
 *
 * @param es      execution state
 * @param context machine context
 */
void md_executionstate_write(executionstate_t* es, void* context)
{
	vm_abort("md_executionstate_write: IMPLEMENT ME!");
}


/* md_jit_method_patch_address *************************************************

   Gets the patch address of the currently compiled method. The offset
   is extracted from the load instruction(s) before the jump and added
   to the right base address (PV or REG_METHODPTR).

   INVOKESTATIC/SPECIAL:

0x7748d7b2:   a7 18 ff d4                      lhi      %r1,-44  
(load dseg offset)
0x7748d7b6:   58 d1 d0 00                      l        %r13,0(%r1,%r13)
(load pv)
0x7748d7ba:   0d ed                            basr     %r14,%r13
(jump to pv)

   INVOKEVIRTUAL:

0x7748d82a:   58 c0 20 00                      l        %r12,0(%r2)
(load mptr)
0x7748d82e:   58 d0 c0 00                      l        %r13,0(%r12)
(load pv from mptr)
0x7748d832:   0d ed                            basr     %r14,%r13
(jump to pv)


   INVOKEINTERFACE:

last 2 instructions the same as in invokevirtual

*******************************************************************************/

void *md_jit_method_patch_address(void* pv, void *ra, void *mptr)
{
	uint8_t *pc;
	uint8_t  base, index;
	int32_t  offset;
	void    *pa;                        /* patch address                      */

	/* go back to the load before the call instruction */

	pc = ((uint8_t *) ra) - SZ_BCR - SZ_L;

	/* get the base register of the load */

	base  = N_RX_GET_BASE(pc);
	index = N_RX_GET_INDEX(pc);

	/* check for the different calls */

	switch (base) {
		case REG_PV:
			/* INVOKESTATIC/SPECIAL */

			switch (index) {
				case R0:
					/* the offset is in the load instruction */
					offset = N_RX_GET_DISP(pc) + N_PV_OFFSET;
					break;
				case REG_ITMP1:
					/* the offset is in the immediate load before the load */
					offset = N_RI_GET_IMM(pc - SZ_L);
					break;
				default:
					assert(0);
			}

			/* add the offset to the procedure vector */

			pa = ((uint8_t *) pv) + offset;
			break;

		case REG_METHODPTR:
			/* mptr relative */
			/* INVOKEVIRTUAL/INTERFACE */

			offset = N_RX_GET_DISP(pc);

			/* return NULL if no mptr was specified (used for replacement) */

			if (mptr == NULL)
				return NULL;

			/* add offset to method pointer */
			
			pa = (uint8_t *)mptr + offset;
			break;

		default:
			/* catch any problems */
			vm_abort("md_jit_method_patch_address");
			break;
	}

	return pa;
}


/* md_patch_replacement_point **************************************************

   Patch the given replacement point.

*******************************************************************************/
#if defined(ENABLE_REPLACEMENT)
void md_patch_replacement_point(u1 *pc, u1 *savedmcode, bool revert)
{
	assert(0);
}
#endif

void md_handle_exception(int32_t *regs, int64_t *fregs, int32_t *out) {

	uint8_t *xptr;
	uint8_t *xpc;
	uint8_t *sp;
	uint8_t *pv;
	uint8_t *ra;
	uint8_t *handler;
	int32_t framesize;
	int32_t intsave;
	int32_t fltsave;
	int64_t *savearea;
	int i;
	int reg;
	int loops = 0;

	/* get registers */

	xptr = *(uint8_t **)(regs + REG_ITMP3_XPTR);
	xpc = *(uint8_t **)(regs + REG_ITMP1_XPC);
	sp = *(uint8_t **)(regs + REG_SP);


	/* initialize number of calle saved int regs to restore to 0 */
	out[0] = 0;

	/* initialize number of calle saved flt regs to restore to 0 */
	out[1] = 0;

	do {

		++loops;

		pv = methodtree_find(xpc);

		handler = exceptions_handle_exception((java_object_t *)xptr, xpc, pv, sp);

		if (handler == NULL) {

			/* exception was not handled
			 * get values of calee saved registers and remove stack frame 
			 */

			/* read stuff from data segment */

			framesize = *(int32_t *)(pv + FrameSize);

			intsave = *(int32_t *)(pv + IntSave);
			if (intsave > out[0]) {
				out[0] = intsave;
			}

			fltsave = *(int32_t *)(pv + FltSave);
			if (fltsave > out[1]) {
				out[1] = fltsave;
			}

			/* pointer to register save area */

			savearea = (int64_t *)(sp + framesize - 8);

			/* return address */

			ra = *(uint8_t **)(sp + framesize - 8);

			/* restore saved registers */

			for (i = 0; i < intsave; ++i) {
				--savearea;
				reg = abi_registers_integer_saved[INT_SAV_CNT - 1 - i];
				regs[reg] = *(int32_t *)(savearea);
			}

			for (i = 0; i < fltsave; ++i) {
				--savearea;
				reg = abi_registers_float_saved[FLT_SAV_CNT - 1 - i];
				fregs[reg] = *savearea;
			}

			/* remove stack frame */

			sp += framesize;

			/* new xpc is call before return address */

			xpc = ra - 2;

		} else {
			xpc = handler;
		}
	} while (handler == NULL);

	/* write new values for registers */

	*(uint8_t **)(regs + REG_ITMP3_XPTR) = xptr;
	*(uint8_t **)(regs + REG_ITMP1_XPC) = xpc;
	*(uint8_t **)(regs + REG_SP) = sp;
	*(uint8_t **)(regs + REG_PV) = pv - 0XFFC;

	/* maybe leaf flag */

	out[2] = (loops == 1);
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
 * vim:noexpandtab:sw=4:ts=4:
 */
