/* src/vm/jit/i386/md-trap.h - i386 hardware traps

   Copyright (C) 1996-2013
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


#ifndef MD_TRAP_HPP_
#define MD_TRAP_HPP_ 1

#include "config.h"


/**
 * Trap number defines.
 *
 * On this architecture (i386) the trap numbers are used as load
 * displacements and thus must not be 4- or 8-byte aligned.
 *
 * NOTE: In trap_init() we have a check whether the offset of
 * java_arrayheader.data[0] is greater than the largest displacement
 * defined below.  Otherwise normal array loads/stores could trigger
 * an exception.
 */

#define TRAP_INSTRUCTION_IS_LOAD    1

enum {
	TRAP_NullPointerException           = 0,
	TRAP_ArithmeticException            = 1,
	TRAP_ArrayIndexOutOfBoundsException = 2,
	TRAP_ArrayStoreException            = 3,
	TRAP_ClassCastException             = 4,
	TRAP_CHECK_EXCEPTION                = 5,
	TRAP_PATCHER                        = 6,
	TRAP_COMPILER                       = 7,
	TRAP_COUNTDOWN                      = 8,
	TRAP_AbstractMethodError            = 9,
	TRAP_THROW                          = 10,
	TRAP_NAT_EXCEPTION                  = 11,
	TRAP_END
};


/**
* Macro to fixup a compiler stub.
* PV : The PV from the compiler stub is equal to the XPC.
* RA : We use a framesize of zero here because the call pushed
*      the return addres onto the stack.
* SP : Skip the RA on the stack.
* XPC: The XPC is the RA minus 2, because the RA points to the
*      instruction after the call.
*/
#define MD_TRAP_COMPILER_FIXUP(xpc, ra, sp, pv) \
	do { \
		(pv)  = (xpc); \
		(ra)  = md_stacktrace_get_returnaddress((sp), 0); \
		(sp)  = (void*) (((uintptr_t) (sp)) + 1 * SIZEOF_VOID_P); \
		(xpc) = (void*) (((uintptr_t) (ra)) - 2); \
	} while(0)

/**
 * Align traps so they don't cross cache line boundaries.
 */
 
#define ALIGN_PATCHER_TRAP

void emit_patcher_alignment(codegendata *cd);

#endif // MD_TRAP_HPP_


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
