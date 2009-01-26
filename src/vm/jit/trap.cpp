/* src/vm/jit/trap.cpp - hardware traps

   Copyright (C) 2008
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

#include <stdint.h>

/* Include machine dependent trap stuff. */

#include "md.h"
#include "md-trap.h"

#include "mm/memory.hpp"

#include "native/llni.h"

#include "toolbox/logging.hpp"

#include "vm/exceptions.hpp"
#include "vm/options.h"
#include "vm/os.hpp"
#include "vm/vm.hpp"

#include "vm/jit/asmpart.h"
#include "vm/jit/code.hpp"
#include "vm/jit/disass.h"
#include "vm/jit/executionstate.h"
#include "vm/jit/jit.hpp"
#include "vm/jit/methodtree.h"
#include "vm/jit/patcher-common.hpp"
#include "vm/jit/replace.hpp"
#include "vm/jit/stacktrace.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Mmap the first memory page to support hardware exceptions and check
 * the maximum hardware trap displacement on the architectures where
 * it is required (TRAP_INSTRUCTION_IS_LOAD defined to 1).
 */
void trap_init(void)
{
#if !(defined(__ARM__) && defined(__LINUX__))
	/* On arm-linux the first memory page can't be mmap'ed, as it
	   contains the exception vectors. */

	int pagesize;

	/* mmap a memory page at address 0x0, so our hardware-exceptions
	   work. */

	pagesize = os::getpagesize();

	(void) os::mmap_anonymous(NULL, pagesize, PROT_NONE, MAP_PRIVATE | MAP_FIXED);
#endif

	TRACESUBSYSTEMINITIALIZATION("trap_init");

#if !defined(TRAP_INSTRUCTION_IS_LOAD)
# error TRAP_INSTRUCTION_IS_LOAD is not defined in your md-trap.h
#endif

#if TRAP_INSTRUCTION_IS_LOAD == 1
	/* Check if we get into trouble with our hardware-exceptions. */

	if (TRAP_END > OFFSET(java_bytearray_t, data))
		vm_abort("trap_init: maximum hardware trap displacement is greater than the array-data offset: %d > %d", TRAP_END, OFFSET(java_bytearray_t, data));
#endif
}


/**
 * Handles the signal which is generated by trap instructions, caught
 * by a signal handler and calls the correct function.
 *
 * @param type trap number
 * @param 
 */
void* trap_handle(int type, intptr_t val, void *pv, void *sp, void *ra, void *xpc, void *context)
{
	executionstate_t es;
	stackframeinfo_t sfi;

#if !defined(NDEBUG)
	if (opt_TraceTraps)
		log_println("[trap_handle: type=%d, val=%p, pv=%p, sp=%p, ra=%p, xpc=%p]", type, val, pv, sp, ra, xpc);
#endif
	
#if defined(ENABLE_VMLOG)
	vmlog_cacao_signl_type(type);
#endif

	/* Prevent compiler warnings. */

	java_handle_t* o = NULL;
	methodinfo*    m = NULL;

#if defined(__ALPHA__) || defined(__ARM__) || defined(__I386__) || defined(__POWERPC__) || defined(__POWERPC64__) || defined(__X86_64__)
# if !defined(NDEBUG)
	/* Perform a sanity check on our execution state functions. */

	executionstate_sanity_check(context);
# endif

	/* Read execution state from current context. */

	es.code = NULL;
	md_executionstate_read(&es, context);

//# define TRAPS_VERBOSE
# if !defined(NDEBUG) && defined(TRAPS_VERBOSE)
	/* Dump contents of execution state */

	if (opt_TraceTraps) {
		log_println("[trap_handle: dumping execution state BEFORE ...]");
		executionstate_println(&es);
	}
# endif
#endif

	/* Do some preparations before we enter the nativeworld. */
	/* BEFORE: creating stackframeinfo */

	switch (type) {
	case TRAP_ClassCastException:
		/* Wrap the value into a handle, as it is a reference. */

		o = LLNI_WRAP((java_object_t *) val);
		break;

	case TRAP_COMPILER:
		/* In this case the passed PV points to the compiler stub.  We
		   get the methodinfo pointer here and set PV to NULL so
		   stacktrace_stackframeinfo_add determines the PV for the
		   parent Java method. */

		m  = code_get_methodinfo_for_pv(pv);
		pv = NULL;
		break;

	default:
		/* do nothing */
		break;
	}

	/* Fill and add a stackframeinfo. */

	stacktrace_stackframeinfo_add(&sfi, pv, sp, ra, xpc);

	/* Get resulting exception (or pointer to compiled method). */

	int32_t        index;
	java_handle_t* p;
	void*          entry;

	switch (type) {
	case TRAP_NullPointerException:
		p = exceptions_new_nullpointerexception();
		break;

	case TRAP_ArithmeticException:
		p = exceptions_new_arithmeticexception();
		break;

	case TRAP_ArrayIndexOutOfBoundsException:
		index = (int32_t) val;
		p = exceptions_new_arrayindexoutofboundsexception(index);
		break;

	case TRAP_ArrayStoreException:
		p = exceptions_new_arraystoreexception();
		break;

	case TRAP_ClassCastException:
		p = exceptions_new_classcastexception(o);
		break;

	case TRAP_CHECK_EXCEPTION:
		p = exceptions_fillinstacktrace();
		break;

	case TRAP_PATCHER:
#if defined(ENABLE_REPLACEMENT)
		if (replace_me_wrapper((uint8_t*) xpc, context)) {
			p = NULL;
			break;
		}
#endif
		p = patcher_handler((uint8_t*) xpc);
		break;

	case TRAP_COMPILER:
		entry = jit_compile_handle(m, sfi.pv, ra, (void*) val);
		p = NULL;
		break;

#if defined(ENABLE_REPLACEMENT)
	case TRAP_COUNTDOWN:
# if defined(__I386__)
		replace_me_wrapper((char*)xpc - 13, context);
# endif
		p = NULL;
		break;
#endif

	default:
		/* Let's try to get a backtrace. */

		(void) methodtree_find(xpc);

		/* If that does not work, print more debug info. */

		log_println("signal_handle: unknown hardware exception type %d", type);

#if SIZEOF_VOID_P == 8
		log_println("PC=0x%016lx", xpc);
#else
		log_println("PC=0x%08x", xpc);
#endif

#if defined(ENABLE_DISASSEMBLER)
		log_println("machine instruction at PC:");
		disassinstr((uint8_t*) xpc);
#endif

		vm_abort("Exiting...");

		/* keep compiler happy */

		p = NULL;
	}

	/* Remove stackframeinfo. */

	stacktrace_stackframeinfo_remove(&sfi);

#if defined(__ALPHA__) || defined(__ARM__) || defined(__I386__) || defined(__POWERPC__) || defined(__POWERPC64__) || defined(__X86_64__)
	/* Update execution state and set registers. */
	/* AFTER: removing stackframeinfo */

	switch (type) {
	case TRAP_COMPILER:
		// The normal case for a compiler trap is to jump directly to
		// the newly compiled method.

		if (entry != NULL) {
			es.pc = (uint8_t *) (uintptr_t) entry;
			es.pv = (uint8_t *) (uintptr_t) entry;
			break;
		}

		// In case of an exception during JIT compilation, we fetch
		// the exception here and proceed with exception handling.

		p = exceptions_get_and_clear_exception();
		assert(p != NULL);

		// Get and set the PV from the parent Java method.

		es.pv = (uint8_t *) md_codegen_get_pv_from_pc(ra);

		// Now fall-through to default exception handling.

		goto trap_handle_exception;

	case TRAP_PATCHER:
		// The normal case for a patcher trap is to continue execution at
		// the trap instruction. On some archs the PC may point after the
		// trap instruction, so we reset it here.

		if (p == NULL) {
			es.pc = (uint8_t *) (uintptr_t) xpc;
			break;
		}

		// Fall-through to default exception handling.

	trap_handle_exception:
	default:
		if (p != NULL) {
			es.intregs[REG_ITMP1_XPTR] = (uintptr_t) LLNI_DIRECT(p);
			es.intregs[REG_ITMP2_XPC]  = (uintptr_t) xpc;
			es.pc                      = (uint8_t *) (uintptr_t) asm_handle_exception;
		}
	}

	/* Write back execution state to current context. */

	md_executionstate_write(&es, context);

# if !defined(NDEBUG) && defined(TRAPS_VERBOSE)
	/* Dump contents of execution state */

	if (opt_TraceTraps) {
		log_println("[trap_handle: dumping execution state AFTER ...]");
		executionstate_println(&es);
	}
# endif
#endif

	/* Unwrap and return the exception object. */
	/* AFTER: removing stackframeinfo */

	if (type == TRAP_COMPILER)
		return entry;
	else
		return LLNI_UNWRAP(p);
}

#ifdef __cplusplus
}
#endif


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
