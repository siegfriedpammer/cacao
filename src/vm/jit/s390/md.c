/* src/vm/jit/s390/md.c - machine dependent s390 Linux functions

   Copyright (C) 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
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

*/


#define _GNU_SOURCE

#include "config.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <ucontext.h>

#include "vm/jit/s390/md-abi.h"

#if defined(ENABLE_THREADS)
# include "threads/threads-common.h"
# include "threads/native/threads.h"
#endif

#include "vm/exceptions.h"
#include "vm/signallocal.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/abi.h"
#include "vm/jit/methodheader.h"
#include "vm/jit/stacktrace.h"

#if !defined(NDEBUG) && defined(ENABLE_DISASSEMBLER)
#include "vmcore/options.h" /* XXX debug */
#include "vm/jit/disass.h" /* XXX debug */
#endif

#include "vm/jit/codegen-common.h"
#include "vm/jit/s390/codegen.h"

#include <assert.h>
#define OOPS() assert(0);

/* prototypes *****************************************************************/

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

	pv = codegen_get_pv_from_pc_nocheck(pc);
	if (pv == NULL) {
		log_println("No java method found at location.");
	} else {
		m = (*(codeinfo **)(pv + CodeinfoPointer))->m;
		log_println(
			"Java method: class %s, method %s, descriptor %s.",
			m->class->name->text, m->name->text, m->descriptor->text
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

#if defined(ENABLE_THREADS)
	log_println("Dumping the current stacktrace:");
	threads_print_stacktrace();
#endif

}

/* md_signal_handler_sigsegv ***************************************************

   NullPointerException signal handler for hardware null pointer
   check.

*******************************************************************************/

void md_signal_handler_sigsegv(int sig, siginfo_t *siginfo, void *_p)
{
	stackframeinfo  sfi;
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

	switch (xpc[0]) {
		case 0x58: /* L */
		case 0x50: /* ST */
		case 0x55: /* CL (array size check on NULL array) */
			base = (xpc[2] >> 4) & 0xF;
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
	type = EXCEPTION_HARDWARE_NULLPOINTER;
	val = 0;

	/* create stackframeinfo */

	stacktrace_create_extern_stackframeinfo(&sfi, pv, sp, ra, xpc);

	/* Handle the type. */

	p = signal_handle(xpc, type, val);

	/* remove stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	if (p != NULL) {
		_mc->gregs[REG_ITMP1_XPTR] = (intptr_t) p;
		_mc->gregs[REG_ITMP2_XPC]  = (intptr_t) xpc;
		_mc->psw.addr              = (intptr_t) asm_handle_exception;
	}
	else {
		_mc->psw.addr              = (intptr_t) xpc;
	}
}

void md_signal_handler_sigill(int sig, siginfo_t *siginfo, void *_p)
{
	stackframeinfo  sfi;
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

	if ((siginfo->si_code == ILL_ILLOPC) && (xpc[0] == 0x02)) {

		/* bits 7-4 contain a register holding a value */
		reg = (xpc[1] >> 4) & 0xF;

		/* bits 3-0 designate the exception type */
		type = xpc[1] & 0xF;  

		pv = (u1 *)_mc->gregs[REG_PV] - N_PV_OFFSET;
		sp = (u1 *)_mc->gregs[REG_SP];
		val = (ptrint)_mc->gregs[reg];

		/* create stackframeinfo */

		stacktrace_create_extern_stackframeinfo(&sfi, pv, sp, ra, xpc);

		/* Handle the type. */

		p = signal_handle(xpc, type, val);

		/* remove stackframeinfo */

		stacktrace_remove_stackframeinfo(&sfi);

		if (p != NULL) {
			_mc->gregs[REG_ITMP1_XPTR] = (intptr_t) p;
			_mc->gregs[REG_ITMP2_XPC]  = (intptr_t) xpc;
			_mc->psw.addr              = (intptr_t) asm_handle_exception;
		}
		else {
			_mc->psw.addr              = (intptr_t) xpc;
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
	stackframeinfo  sfi;
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

	if (xpc[0] == 0x1D) { /* DR */

		r1 = (xpc[1] >> 4) & 0xF;
		r2 = xpc[1] & 0xF;

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

			type = EXCEPTION_HARDWARE_ARITHMETIC;
			val = 0;

			/* create stackframeinfo */

			stacktrace_create_extern_stackframeinfo(&sfi, pv, sp, ra, xpc);

			/* Handle the type. */

			p = signal_handle(xpc, type, val);

			/* remove stackframeinfo */

			stacktrace_remove_stackframeinfo(&sfi);

			_mc->gregs[REG_ITMP1_XPTR] = (intptr_t) p;
			_mc->gregs[REG_ITMP2_XPC]  = (intptr_t) xpc;
			_mc->psw.addr              = (intptr_t) asm_handle_exception;

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


#if defined(ENABLE_THREADS)
void md_critical_section_restart(ucontext_t *_uc)
{
	mcontext_t *_mc;
	u1         *pc;
	void       *npc;

	_mc = &_uc->uc_mcontext;

	pc = (u1 *)_mc->psw.addr;

	npc = critical_find_restart_point(pc);

	if (npc != NULL) {
		log_println("%s: pc=%p, npc=%p", __FUNCTION__, pc, npc);
		_mc->psw.addr = (ptrint) npc;
	}
}
#endif


/* md_codegen_patch_branch *****************************************************

   Back-patches a branch instruction.

*******************************************************************************/

void md_codegen_patch_branch(codegendata *cd, s4 branchmpc, s4 targetmpc)
{

	s4 *mcodeptr;
	s4  disp;                           /* branch displacement                */

	/* calculate the patch position */

	mcodeptr = (s4 *) (cd->mcodebase + branchmpc);

	/* Calculate the branch displacement. */

	disp = targetmpc - branchmpc;
	disp += 4; /* size of branch */
	disp /= 2; /* specified in halfwords */

	ASSERT_VALID_BRANCH(disp);	

	/* patch the branch instruction before the mcodeptr */

	mcodeptr[-1] |= (disp & 0xFFFF);
}


/* md_stacktrace_get_returnaddress *********************************************

   Returns the return address of the current stackframe, specified by
   the passed stack pointer and the stack frame size.

*******************************************************************************/

u1 *md_stacktrace_get_returnaddress(u1 *sp, u4 framesize)
{
	u1 *ra;

	/* on S390 the return address is located on the top of the stackframe */

	ra = *((u1 **) (sp + framesize - 8));

	return ra;
}


/* md_get_method_patch_address *************************************************

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

u1 *md_get_method_patch_address(u1 *ra, stackframeinfo *sfi, u1 *mptr)
{
	u1  base, index;
	s4  offset;
	u1 *pa;                             /* patch address                      */

	/* go back to the load before the call instruction */

	ra = ra - 2 /* sizeof bcr */ - 4 /* sizeof l */;

	/* get the base register of the load */

	base = ra[2] >> 4;
	index = ra[1] & 0xF;

	/* check for the different calls */

	switch (base) {
		case 0xd:
			/* INVOKESTATIC/SPECIAL */

		
			switch (index) {
				case 0x0:
					/* the offset is in the load instruction */
					offset = ((*(u2 *)(ra + 2)) & 0xFFF) + N_PV_OFFSET;
					break;
				case 0x1:
					/* the offset is in the immediate load before the load */
					offset = *((s2 *) (ra - 2));
					break;
				default:
					assert(0);
			}

			/* add the offset to the procedure vector */

			pa = sfi->pv + offset;

			break;

		case 0xc:
			/* mptr relative */
			/* INVOKEVIRTUAL/INTERFACE */

			offset = *((u2 *)(ra + 2)) & 0xFFF;

			/* return NULL if no mptr was specified (used for replacement) */

			if (mptr == NULL)
				return NULL;

			/* add offset to method pointer */
			
			pa = mptr + offset;
			break;
		default:
			/* catch any problems */
			assert(0); 
			break;
	}

	return pa;
}


/* md_codegen_get_pv_from_pc ***************************************************

   On this architecture just a wrapper function to
   codegen_get_pv_from_pc.

*******************************************************************************/

u1 *md_codegen_get_pv_from_pc(u1 *ra)
{
	u1 *pv;

	/* Get the start address of the function which contains this
       address from the method table. */

	pv = codegen_get_pv_from_pc(ra);

	return pv;
}


/* md_cacheflush ***************************************************************

   Calls the system's function to flush the instruction and data
   cache.

*******************************************************************************/

void md_cacheflush(u1 *addr, s4 nbytes)
{
	/* do nothing */
}


/* md_icacheflush **************************************************************

   Calls the system's function to flush the instruction cache.

*******************************************************************************/

void md_icacheflush(u1 *addr, s4 nbytes)
{
	/* do nothing */
}


/* md_dcacheflush **************************************************************

   Calls the system's function to flush the data cache.

*******************************************************************************/

void md_dcacheflush(u1 *addr, s4 nbytes)
{
	/* do nothing */
}


/* md_patch_replacement_point **************************************************

   Patch the given replacement point.

*******************************************************************************/
#if defined(ENABLE_REPLACEMENT)
void md_patch_replacement_point(codeinfo *code, s4 index, rplpoint *rp, u1 *savedmcode)
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

	xptr = *(uint8_t **)(regs + REG_ITMP1_XPTR);
	xpc = *(uint8_t **)(regs + REG_ITMP2_XPC);
	sp = *(uint8_t **)(regs + REG_SP);


	/* initialize number of calle saved int regs to restore to 0 */
	out[0] = 0;

	/* initialize number of calle saved flt regs to restore to 0 */
	out[1] = 0;

	do {

		++loops;

		pv = codegen_get_pv_from_pc(xpc);

		handler = exceptions_handle_exception(xptr, xpc, pv, sp);

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

			xpc = ra;

		} else {
			xpc = handler;
		}
	} while (handler == NULL);

	/* write new values for registers */

	*(uint8_t **)(regs + REG_ITMP1_XPTR) = xptr;
	*(uint8_t **)(regs + REG_ITMP2_XPC) = xpc;
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
