/* src/vm/jit/trap.cpp - hardware traps

   Copyright (C) 2008, 2009
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO
   Copyright (C) 2009 Theobroma Systems Ltd.

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
#include "vm/jit/trap.hpp"

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
	TRACESUBSYSTEMINITIALIZATION("trap_init");

	/* If requested we mmap a memory page at address 0x0,
	   so our hardware-exceptions work. */

	if (opt_AlwaysMmapFirstPage) {
		int pagesize = os::getpagesize();
		(void) os::mmap_anonymous(NULL, pagesize, PROT_NONE, MAP_PRIVATE | MAP_FIXED);
	}

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
 * @param sig signal number
 * @param xpc exception PC
 * @param context pointer to OS dependent machine context
 */
void trap_handle(int sig, void *xpc, void *context)
{
	executionstate_t es;
	stackframeinfo_t sfi;
	trapinfo_t       trp;

	// Prevent compiler warnings.
	java_handle_t* o = NULL;
	methodinfo*    m = NULL;

#if !defined(NDEBUG)
	// Sanity checking the XPC.
	if (xpc == NULL)
		vm_abort("trap_handle: The program counter is NULL!");
#endif

#if defined(__ALPHA__) || defined(__ARM__) || defined(__I386__) || defined(__MIPS__) || defined(__POWERPC__) || defined(__POWERPC64__) || defined(__X86_64__)
# if !defined(NDEBUG)
	/* Perform a sanity check on our execution state functions. */

	executionstate_sanity_check(context);
# endif

	/* Read execution state from current context. */

	es.code = NULL;
	md_executionstate_read(&es, context);

//# define TRAP_TRACE_VERBOSE
# if !defined(NDEBUG) && defined(TRAP_TRACE_VERBOSE)
	/* Dump contents of execution state */

	if (opt_TraceTraps) {
		log_println("[trap_handle: dumping execution state BEFORE ...]");
		executionstate_println(&es);
	}
# endif
#endif

	// Extract information from executionstate
	void* pv = es.pv;  // Maybe null, resolved during stackframeinfo creation.
	void* sp = es.sp;
#if defined(__I386__) || defined(__X86_64__)
	void* ra = xpc;  // Return address is equal to XPC.
#else
	void* ra = es.ra;  // This is correct for leafs.
#endif

	// Decode machine-dependent trap instruction.
	bool decode_result = md_trap_decode(&trp, sig, xpc, &es);

	// Check if the trap instruction is valid and was decoded
	// successfully.
	if (!decode_result) {
		// Check if the PC has been patched during our way to this
		// trap handler (see PR85).
		// NOTE: Some archs use SIGILL for other traps too, but it's OK to
		// do this check anyway because it will fail.
		if (patcher_is_patched_at(xpc) == true) {
			if (opt_PrintWarnings)
				log_println("trap_handle: Detected patcher race condition (PR85) at %p", xpc);
			return;
		}

		// We have a problem...
		vm_abort_disassemble(xpc, 1, "trap_handle: Unknown trap instruction at %p", xpc);
	}

	// For convenience only.
	int      type = trp.type;
	intptr_t val  = trp.value;

	/* Do some preparations before we enter the nativeworld. */
	/* BEFORE: creating stackframeinfo */

	switch (type) {
	case TRAP_ClassCastException:
		/* Wrap the value into a handle, as it is a reference. */

		o = LLNI_WRAP((java_object_t *) val);
		break;

	case TRAP_COMPILER:
		/* We need to fixup the XPC, SP and RA here because they
		   all might point into the compiler stub instead of the
		   calling method. */

		MD_TRAP_COMPILER_FIXUP(xpc, ra, sp, pv);

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

#if !defined(NDEBUG)
	// Trace this trap.
	if (opt_TraceTraps)
		log_println("[trap_handle: sig=%d, type=%d, val=%p, pv=%p, sp=%p, ra=%p, xpc=%p]", sig, type, val, pv, sp, ra, xpc);
#endif

#if defined(ENABLE_VMLOG)
	vmlog_cacao_signl_type(type);
#endif

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
		replace_me_wrapper((uint8_t*) xpc - 13, context);
# endif
		p = NULL;
		break;
#endif

	default:
		/* Let's try to get a backtrace. */

		(void) methodtree_find(xpc);

		/* If that does not work, print more debug info. */

		vm_abort_disassemble(xpc, 1, "trap_handle: Unknown hardware exception type %d", type);

		/* keep compiler happy */

		p = NULL;
	}

	/* Remove stackframeinfo. */

	stacktrace_stackframeinfo_remove(&sfi);

#if defined(__ALPHA__) || defined(__ARM__) || defined(__I386__) || defined(__MIPS__) || defined(__POWERPC__) || defined(__POWERPC64__) || defined(__X86_64__)
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

		// Remove RA from stack on some archs.

		es.sp = (uint8_t*) sp;

		// Get and set the PV from the parent Java method.

		es.pv = (uint8_t*) md_codegen_get_pv_from_pc(ra);

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

# if !defined(NDEBUG) && defined(TRAP_TRACE_VERBOSE)
	/* Dump contents of execution state */

	if (opt_TraceTraps) {
		log_println("[trap_handle: dumping execution state AFTER ...]");
		executionstate_println(&es);
	}
# endif
#endif
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
 * vim:noexpandtab:sw=4:ts=4:
 */