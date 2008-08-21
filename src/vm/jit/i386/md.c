/* src/vm/jit/i386/md.c - machine dependent i386 functions

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

#include <assert.h>
#include <stdint.h>

#include "vm/types.h"

#include "vm/global.h"
#include "vm/vm.hpp"

#include "vm/jit/asmpart.h"
#include "vm/jit/jit.hpp"


/* md_init *********************************************************************

   Do some machine dependent initialization.

*******************************************************************************/

void md_init(void)
{
	(void) asm_md_init();
}


/* md_jit_method_patch_address *************************************************

   Gets the patch address of the currently compiled method. The offset
   is extracted from the load instruction(s) before the jump and added
   to the right base address (PV or REG_METHODPTR).

   INVOKESTATIC/SPECIAL:

   b9 30 00 49 b7             mov    $0xb7490030,%ecx
   ff d1                      call   *%ecx

   INVOKEVIRTUAL:

   8b 08                      mov    (%eax),%ecx
   8b 91 00 00 00 00          mov    0x0(%ecx),%edx
   ff d2                      call   *%edx

   INVOKEINTERFACE:

   8b 08                      mov    (%eax),%ecx
   8b 89 00 00 00 00          mov    0x0(%ecx),%ecx
   8b 91 00 00 00 00          mov    0x0(%ecx),%edx
   ff d2                      call   *%edx

*******************************************************************************/

void *md_jit_method_patch_address(void *pv, void *ra, void *mptr)
{
	uint8_t  *pc;
	uint16_t  opcode;
	int32_t   disp;
	void     *pa;                                            /* patch address */

	/* go back to the actual call instruction (2-bytes) */

	pc = ((uint8_t *) ra) - 2;

	/* Get the opcode of the call. */

	opcode = *((uint16_t *) pc);

	/* check for the different calls */

	switch (opcode) {
	case 0xd1ff:
		/* INVOKESTATIC/SPECIAL */

		/* Patch address is 4-bytes before the call instruction. */

		pa = pc - 4;
		break;

	case 0xd2ff:
		/* INVOKEVIRTUAL/INTERFACE */

		/* Return NULL if no mptr was specified (used for
		   replacement). */

		if (mptr == NULL)
			return NULL;

		/* Get the displacement from the instruction (the displacement
		   address is 4-bytes before the call instruction). */

		disp = *((int32_t *) (pc - 4));

		/* Add the displacement to the method pointer. */

		pa = ((uint8_t *) mptr) + disp;
		break;

	default:
		vm_abort_disassemble(pc, 1, "md_jit_method_patch_address: unknown instruction %x", opcode);
		return NULL;
	}

	return pa;
}


/* md_patch_replacement_point **************************************************

   Patch the given replacement point.

*******************************************************************************/

#if defined(ENABLE_REPLACEMENT)
void md_patch_replacement_point(u1 *pc, u1 *savedmcode, bool revert)
{
	u2 mcode;

	if (revert) {
		/* write saved machine code */
		*(u2*)(pc) = *(u2*)(savedmcode);
	}
	else {
		/* save the current machine code */
		*(u2*)(savedmcode) = *(u2*)(pc);

		/* build the machine code for the patch */
		mcode = 0x0b0f;

		/* write new machine code */
		*(u2*)(pc) = mcode;
	}

    /* XXX if required asm_cacheflush(pc,8); */
}
#endif /* defined(ENABLE_REPLACEMENT) */

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
