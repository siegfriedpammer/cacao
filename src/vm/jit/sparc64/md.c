
#include <assert.h>

#include "config.h"

#include "vm/types.h"

#include "vm/jit/sparc64/md-abi.h"

#include "vm/exceptions.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/stacktrace.h"

#if !defined(NDEBUG) && defined(ENABLE_DISASSEMBLER)
#include "vm/options.h" /* XXX debug */
#include "vm/jit/disass.h" /* XXX debug */
#endif


/* md_init *********************************************************************

   Do some machine dependent initialization.

*******************************************************************************/

void md_init(void)
{
	/* do nothing */
}


/* md_stacktrace_get_returnaddress *********************************************

   Returns the return address of the current stackframe, specified by
   the passed stack pointer and the stack frame size.

*******************************************************************************/

u1 *md_stacktrace_get_returnaddress(u1 *sp, u4 framesize)
{
	u1 *ra;
	/* flush register windows to the stack */
	__asm__ ("flushw");

	/* the return address resides in register i7, the last register in the
	 * 16-extended-word save area
	 */
	ra = *((u1 **) (sp + 120));
	
	/* ra is the address of the call instr, advance to the real return address  */
	ra += 8;

	return ra;
}


/* md_codegen_get_pv_from_pc ***************************************************

   This reconstructs and returns the PV of a method given a return address
   pointer. (basically, same was as the generated code following the jump does)
   
   Machine code:

   6b5b4000    jsr     (pv)
   277afffe    ldah    pv,-2(ra)
   237ba61c    lda     pv,-23012(pv)

*******************************************************************************/

u1 *md_codegen_get_pv_from_pc(u1 *ra)
{
	u1 *pv;
	u4  mcode;
	s4  offset;

	pv = ra;

	/* get first instruction word after jump */

	mcode = *((u4 *) ra);

	/* check if we have 2 instructions (ldah, lda) */

	if ((mcode >> 16) == 0x277a) {
		/* get displacement of first instruction (ldah) */

		offset = (s4) (mcode << 16);
		pv += offset;

		/* get displacement of second instruction (lda) */

		mcode = *((u4 *) (ra + 1 * 4));

		assert((mcode >> 16) == 0x237b);

		offset = (s2) (mcode & 0x0000ffff);
		pv += offset;

	} else {
		/* get displacement of first instruction (lda) */

		assert((mcode >> 16) == 0x237a);

		offset = (s2) (mcode & 0x0000ffff);
		pv += offset;
	}

	return pv;
}

/* md_get_method_patch_address *************************************************

   Gets the patch address of the currently compiled method. The offset
   is extracted from the load instruction(s) before the jump and added
   to the right base address (PV or REG_METHODPTR).

   INVOKESTATIC/SPECIAL:

   dfdeffb8    ld       s8,-72(s8)
   03c0f809    jalr     s8
   00000000    nop

   INVOKEVIRTUAL:

   dc990000    ld       t9,0(a0)
   df3e0000    ld       s8,0(t9)
   03c0f809    jalr     s8
   00000000    nop

   INVOKEINTERFACE:

   dc990000    ld       t9,0(a0)
   df39ff90    ld       t9,-112(t9)
   df3e0018    ld       s8,24(t9)
   03c0f809    jalr     s8
   00000000    nop

*******************************************************************************/

u1 *md_get_method_patch_address(u1 *ra, stackframeinfo *sfi, u1 *mptr)
{
	u4  mcode;
	s4  offset;
	u1 *pa;

	/* go back to the actual load instruction (3 instructions on MIPS) */

	ra -= 3 * 4;

	/* get first instruction word on current PC */

	mcode = *((u4 *) ra);

	/* check if we have 2 instructions (lui) */

	if ((mcode >> 16) == 0x3c19) {
		/* XXX write a regression for this */
		assert(0);

		/* get displacement of first instruction (lui) */

		offset = (s4) (mcode << 16);

		/* get displacement of second instruction (daddiu) */

		mcode = *((u4 *) (ra + 1 * 4));

		assert((mcode >> 16) != 0x6739);

		offset += (s2) (mcode & 0x0000ffff);

	} else {
		/* get first instruction (ld) */

		mcode = *((u4 *) ra);

		/* get the offset from the instruction */

		offset = (s2) (mcode & 0x0000ffff);

		/* check for call with REG_METHODPTR: ld s8,x(t9) */

#if SIZEOF_VOID_P == 8
		if ((mcode >> 16) == 0xdf3e) {
#else
		if ((mcode >> 16) == 0x8f3e) {
#endif
			/* in this case we use the passed method pointer */

			pa = mptr + offset;

		} else {
			/* in the normal case we check for a `ld s8,x(s8)' instruction */

#if SIZEOF_VOID_P == 8
			assert((mcode >> 16) == 0xdfde);
#else
			assert((mcode >> 16) == 0x8fde);
#endif

			/* and get the final data segment address */

			pa = sfi->pv + offset;
		}
	}

	return pa;
}


/* md_cacheflush ***************************************************************

   Calls the system's function to flush the instruction and data
   cache.

*******************************************************************************/

void md_cacheflush(u1 *addr, s4 nbytes)
{
	/* don't know yet */	
}


/* md_icacheflush **************************************************************

   Calls the system's function to flush the instruction cache.

*******************************************************************************/

void md_icacheflush(u1 *addr, s4 nbytes)
{
	/* don't know yet */	
}


/* md_patch_replacement_point **************************************************

   Patch the given replacement point.

*******************************************************************************/

void md_patch_replacement_point(rplpoint *rp)
{
    u8 mcode;

	/* save the current machine code */
	mcode = *(u4*)rp->pc;

	/* write the new machine code */
    *(u4*)(rp->pc) = (u4) rp->mcode;

	/* store saved mcode */
	rp->mcode = mcode;
	
#if !defined(NDEBUG) && defined(ENABLE_DISASSEMBLER)
	{
		u1* u1ptr = rp->pc;
		DISASSINSTR(u1ptr);
		fflush(stdout);
	}
#endif
			
	/* flush instruction cache */
    /* md_icacheflush(rp->pc,4); */
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
