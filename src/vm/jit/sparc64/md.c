
#include "config.h"

#include "vm/types.h"

#include "vm/jit/sparc64/md-abi.h"

#include "vm/exceptions.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/stacktrace.h"


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
	/* where's it gonna be ? */

	return 0;
}


/* md_codegen_findmethod *******************************************************


   This reconstructs and returns the PV of a method given a return address
   pointer. (basically, same was as the generated code following the jump does)
   
   Machine code:

   6b5b4000    jsr     (pv)
   277afffe    ldah    pv,-2(ra)
   237ba61c    lda     pv,-23012(pv)

*******************************************************************************/

u1 *md_codegen_findmethod(u1 *ra)
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
