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

   <patched call position>
   mov      x9, <interfacetable index>
   mov      x10, <method offset>
   ldr		x16, [x0, _]
   ldr      x16, [x16, x9]
   ldr      x17, [x16, x10]
   blr      x17

*******************************************************************************/

void *md_jit_method_patch_address(void *pv, void *ra, void *mptr)
{
	uint32_t *pc;
	uint32_t  mcode;
	int       base;
	int32_t   disp;
	void     *pa;                       /* patch address                      */
	u2		  msb10;                    /* most significant 10 bits           */
	u2		  msb11;                    /* most significant 11 bits           */
	bool      invokeinterface = false;  

	/* Go back to the load instruction (2 instructions). */
	pc = ((uint32_t *) ra) - 3;

	/* Get first instruction word. */
	mcode = pc[1];

	if (mcode == 0xd503201f)
		mcode = pc[0];

	/* Get base register */
	base = (mcode >> 5) & 0x1f;
	

	/* Check for LDR or LDUR       */
	msb10 = (mcode >> 22) & 0x3ff;
	msb11 = (mcode >> 21) & 0x7ff;

	if (msb10 == 0x3e5) { /* unsigned offset (LDR) */
		disp = (mcode >> 10) & 0xfff;
		disp *= 8; /* imm is encoded as imm/8 */
	} else if (msb11 == 0x7c2) { /* unscaled immediate (LDUR) */
		disp = (mcode >> 12) & 0xff;
		if ((mcode >> 12) & 0x100) /* negative offset */
			disp |= 0xffffff00; 
	} else if (msb11 == 0x7c3) { /* LDR from register --> previous instruction was a MOV */
		/* if the instruction beforehand is a ldr as well, we have a invokeinterface */
		if (((pc[0] >> 21) & 0x7ff) == 0x7c3)
			invokeinterface = true;

		/* do nothing, offset is encoded in the previous MOVN instruction */
		disp = 0;
	} else {
		vm_abort_disassemble(pc, 2, "md_jit_method_patch_address: unknown instruction %x", mcode);
		return NULL;
	}

	/* if negative offset is too big, we had to use a SUB or MOV beforehand */
	if (disp == 0 && (msb11 == 0x7c2 || msb11 == 0x7c3)) {
		uint32_t subcode = pc[0];
		u1 high = (subcode >> 24) & 0xff;
		if (high == 0xD1 && msb11 == 0x7c2) { /* Check for SUB */
			// TODO: check that the target register is the same, as the one used by the LDR
			int32_t offset = (subcode >> 10) & 0xfff;
			disp = -offset;
		} else if (high == 0x92 && msb11 == 0x7c3) { /* Check for MOVN */
			int32_t offset = (subcode >> 5) & 0xffff;
			disp = -offset - 1;
		} else if (invokeinterface) { 
			uint32_t *movncode = ((uint32_t *) ra) - 4;
			mcode = movncode[0];
			high = (mcode >> 24) & 0xff;
			if (high == 0x92) { /* check for MOVN */
				int32_t offset = (mcode >> 5) & 0xffff;
				disp = -offset - 1;
			} else if (high == 0xd2) { /* check foir MOVZ */
				int32_t offset = (mcode >> 5) & 0xffff;
				disp = offset;
			}
		}
	} 


	switch(base) {
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
		vm_abort_disassemble(pc, 2, "md_jit_method_patch_address: unsupported base register %x", mcode);
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
		if ((mcode & 0xFF000000) == 0xE7000000) {
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
		vm_abort_disassemble(xpc, 1, "md_trap_decode: Unexpected signal: %d", sig);
		return false;
	}
}


/* md_patch_replacement_point **************************************************

   Patch the given replacement point.

*******************************************************************************/

#if defined(ENABLE_REPLACEMENT)
void md_patch_replacement_point(u1 *pc, u1 *savedmcode, bool revert)
{
	u4 mcode;

	if (revert) {
		/* restore the patched-over instruction */
		*(u4*)(pc) = *(u4*)(savedmcode);
	}
	else {
		/* save the current machine code */
		*(u4*)(savedmcode) = *(u4*)(pc);

		/* build the machine code for the patch */
		mcode =  0xE7000000 | (TRAP_PATCHER << 8);

		/* write the new machine code */
		*(u4*)(pc) = (u4) mcode;
	}

	/* flush instruction cache */
    md_icacheflush(pc,4);
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
