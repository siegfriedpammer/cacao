/* src/vm/jit/aarch64/md.cpp - machine dependent Aarch64 functions

   Copyright (C) 1996-2013
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
#include <ucontext.h>

#include "vm/jit/aarch64/codegen.hpp"
#include "vm/jit/aarch64/md.hpp"

#include "vm/jit/asmpart.hpp"
#include "vm/jit/disass.hpp"
#include "vm/jit/jit.hpp"
#include "vm/jit/trap.hpp"

bool has_ext_instr_set = false;

/* md_init *********************************************************************

   Do some machine dependent initialization.

*******************************************************************************/

void md_init(void)
{
	/* do nothing on aarch64 */
}


/* md_jit_method_patch_address *************************************************

   Gets the patch address of the currently compiled method. The offset
   is extracted from the load instruction(s) before the jump and added
   to the right base address (PV or REG_METHODPTR).

   INVOKESTATIC/SPECIAL:

   a77bffb8    ldq     pv,-72(pv)
   6b5b4000    jsr     (pv)

   INVOKEVIRTUAL:

   a7900000    ldq     at,0(a0)
   a77c0000    ldq     pv,0(at)
   6b5b4000    jsr     (pv)

   INVOKEINTERFACE:

   a7900000    ldq     at,0(a0)
   a79cff98    ldq     at,-104(at)
   a77c0018    ldq     pv,24(at)
   6b5b4000    jsr     (pv)

*******************************************************************************/

void *md_jit_method_patch_address(void *pv, void *ra, void *mptr)
{
	log_println("md_jit_method_patch_address called");
	uint32_t *pc;
	uint32_t  mcode;
	int       opcode;
	int       base;
	int32_t   disp;
	void     *pa;                       /* patch address                      */

	/* Go back to the load instruction (2 instructions). */

	pc = ((uint32_t *) ra) - 2;

	/* Get first instruction word. */

	mcode = pc[0];

	/* Get opcode, base register and displacement. */

	opcode = M_MEM_GET_Opcode(mcode);
	base   = M_MEM_GET_Rb(mcode);
	disp   = M_MEM_GET_Memory_disp(mcode);

	/* Check for short or long load (2 instructions). */

	switch (opcode) {
	case 0x29: /* LDQ: TODO use define */
		switch (base) {
		case REG_PV:
			/* Calculate the data segment address. */

			pa = ((uint8_t *) pv) + disp;
			break;

		case REG_METHODPTR:
			/* Return NULL if no mptr was specified (used for
			   replacement). */

			if (mptr == NULL)
				return NULL;

			/* Calculate the address in the vftbl. */

			pa = ((uint8_t *) mptr) + disp;
			break;

		default:
			vm_abort_disassemble(pc, 2, "md_jit_method_patch_address: unknown instruction %x", mcode);
			return NULL;
		}
		break;

	case 0x09: /* LDAH: TODO use define */
		/* XXX write a regression for this */

		vm_abort("md_jit_method_patch_address: IMPLEMENT ME!");

		pa = NULL;
		break;

	default:
		vm_abort_disassemble(pc, 2, "md_jit_method_patch_address: unknown instruction %x", mcode);
		return NULL;
	}

	return pa;
}


/**
 * Decode the trap instruction at the given PC.
 *
 * @param trp information about trap to be filled
 * @param sig signal number
 * @param xpc exception PC
 * @param es execution state of the machine
 * @return true if trap was decoded successfully, false otherwise.
 */
bool md_trap_decode(trapinfo_t* trp, int sig, void* xpc, executionstate_t* es)
{
	// Get the exception-throwing instruction.
	uint32_t mcode = *((uint32_t*) xpc);

	switch (sig) {
	case TRAP_SIGILL:
		// Check for the mark
		if ((mcode & 0xE7000000)) {
			trp->type = (mcode >> 8) & 0xff;
			u1 reg = mcode & 0xff;
			trp->value = es->intregs[reg];
			return true;
		}

		return false;

	case TRAP_SIGSEGV:
	{
		// Retrieve base address of instruction.
		u2 reg = (mcode >> 5) & 0x1f;
		uintptr_t addr = es->intregs[reg];

		// Check for implicit NullPointerException.
		if (addr == 0) {
			trp->type  = TRAP_NullPointerException;
			trp->value = 0;
			return true;
		}

		return false;
	}

	default:
		return false;
	}
}


/* md_patch_replacement_point **************************************************

   Patch the given replacement point.

*******************************************************************************/

#if defined(ENABLE_REPLACEMENT)
void md_patch_replacement_point(u1 *pc, u1 *savedmcode, bool revert)
{
	os::abort("md_patch_replacement_point: NOT IMPLEMENTED!");
}
#endif /* defined(ENABLE_REPLACEMENT) */


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
