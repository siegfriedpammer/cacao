
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

/* shift away 13-bit immediate,  mask rd and rs1    */
#define SHIFT_AND_MASK(instr) \
	((instr >> 13) & 0x60fc1)

#define IS_SETHI(instr) \
	((instr & 0xc1c00000)  == 0x00800000)

inline s2 decode_13bit_imm(u4 instr) {
	s2 imm;

	/* mask everything else in the instruction */
	imm = instr & 0x00001fff;

	/* sign extend 13-bit to 16-bit */
	imm <<= 3;
	imm >>= 3;

	return imm;
}

/* md_init *********************************************************************

   Do some machine dependent initialization.

*******************************************************************************/

void md_init(void)
{
	/* do nothing */
}


/* md_codegen_patch_branch *****************************************************

   Back-patches a branch instruction.

*******************************************************************************/

void md_codegen_patch_branch(codegendata *cd, s4 branchmpc, s4 targetmpc)
{
	s4 *mcodeptr;
	s4  mcode;
	s4  disp;                           /* branch displacement                */

	/* calculate the patch position */

	mcodeptr = (s4 *) (cd->mcodebase + branchmpc);

	/* get the instruction before the exception point */

	mcode = mcodeptr[-1];
	
	/* Calculate the branch displacement.  SPARC displacements regard current
	   PC as base => (branchmpc - 4 */
	
	disp = (targetmpc - (branchmpc - 4)) >> 2;
	

	/* check for BPcc or FBPfcc instruction */
	if (((mcode >> 16) & 0xc1c0) == 0x0040) {
	
		/* check branch displacement (19-bit)*/
	
		if ((disp < (s4) 0xfffc0000) || (disp > (s4) 0x003ffff))
			vm_abort("branch displacement is out of range: %d > +/-%d", disp, 0x003ffff);
	
		/* patch the branch instruction before the mcodeptr */
	
		mcodeptr[-1] |= (disp & 0x007ffff);
	}
	/* check for BPr instruction */
	else if (((mcode >> 16) & 0xd1c0) == 0x00c0) {

		/* check branch displacement (16-bit)*/
	
		if ((disp < (s4) 0xffff8000) || (disp > (s4) 0x0007fff))
			vm_abort("branch displacement is out of range: %d > +/-%d", disp, 0x0007fff);
			
		/* patch the upper 2-bit of the branch displacement */
		mcodeptr[-1] |= ((disp & 0xc000) << 6);
			
		/* patch the lower 14-bit of the branch displacement */
		mcodeptr[-1] |= (disp & 0x003fff);
		
	}
	else
		assert(0);
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
	ra = *((u1 **) (sp + 120 + BIAS));
	
	/* NOTE: on SPARC ra is the address of the call instruction */

	return ra;
}


/* md_codegen_get_pv_from_pc ***************************************************

   This reconstructs and returns the PV of a method given a return address
   pointer. (basically, same was as the generated code following the jump does)
   
   Machine code:

   6b5b4000    jmpl    (pv)
   10000000    nop
   277afffe    ldah    pv,-2(ra)
   237ba61c    lda     pv,-23012(pv)

*******************************************************************************/

u1 *md_codegen_get_pv_from_pc(u1 *ra)
{
	u1 *pv;
	u8  mcode;
	u4  mcode_masked;
	s2  offset;

	pv = ra;

	/* get the instruction word after jump and nop */
	mcode = *((u4 *) (ra+8) );

	/* check if we have 2 instructions (ldah, lda) */

	mcode_masked = SHIFT_AND_MASK(mcode);

	if (mcode_masked == 0x40001) {
#if 0
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
#endif
		/* mask and extend the negative sign for the 13 bit immediate */
		offset = decode_13bit_imm(mcode);

		pv += offset;
	}
	else
	{
		assert(0);
	}

	return pv;
}

/* md_get_method_patch_address *************************************************

   Gets the patch address of the currently compiled method. The offset
   is extracted from the load instruction(s) before the jump and added
   to the right base address (PV or REG_METHODPTR).

   INVOKESTATIC/SPECIAL:

   dfdeffb8    ldx      [i5 - 72],o5
   03c0f809    jmp      o5
   00000000    nop

   INVOKEVIRTUAL:

   dc990000    ld       t9,0(a0)
   df3e0000    ld       [g2 + 0],o5
   03c0f809    jmp      o5
   00000000    nop

   INVOKEINTERFACE:

   dc990000    ld       t9,0(a0)
   df39ff90    ld       [g2 - 112],g2
   df3e0018    ld       [g2 + 24],o5
   03c0f809    jmp      o5
   00000000    nop

*******************************************************************************/

u1 *md_get_method_patch_address(u1 *ra, stackframeinfo *sfi, u1 *mptr)
{
	u4  mcode, mcode_masked;
	s4  offset;
	u1 *pa;

	/* go back to the actual load instruction (1 instruction before jump) */
	/* ra is the address of the jump instruction on SPARC                 */
	ra -= 1 * 4;

	/* get first instruction word on current PC */

	mcode = *((u4 *) ra);


	/* check if we have 2 instructions (lui) */

	if (IS_SETHI(mcode)) {
		/* XXX write a regression for this */
		assert(0);

		/* get displacement of first instruction (lui) */

		offset = (s4) (mcode << 16);

		/* get displacement of second instruction (daddiu) */

		mcode = *((u4 *) (ra + 1 * 4));

		assert((mcode >> 16) != 0x6739);

		offset += (s2) (mcode & 0x0000ffff);

	} else {

		/* shift and maks rd */

		mcode_masked = (mcode >> 13) & 0x060fff;
		
		/* get the offset from the instruction */

		offset = decode_13bit_imm(mcode);

		/* check for call with rs1 == REG_METHODPTR: ldx [g2+x],pv_caller */

		if (mcode_masked == 0x0602c5) {
			/* in this case we use the passed method pointer */

			/* return NULL if no mptr was specified (used for replacement) */

			if (mptr == NULL)
				return NULL;

			pa = mptr + offset;

		} else {
			/* in the normal case we check for a `ldx [i5+x],pv_caller' instruction */

			assert(mcode_masked  == 0x0602fb);

			printf("data segment: pv=0x%08x, offset=%d\n", sfi->pv, offset);

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


/* md_dcacheflush **************************************************************

   Calls the system's function to flush the data cache.

*******************************************************************************/

void md_dcacheflush(u1 *addr, s4 nbytes)
{
	/* XXX don't know yet */	
}


/* md_patch_replacement_point **************************************************

   Patch the given replacement point.

*******************************************************************************/

#if defined(ENABLE_REPLACEMENT)
void md_patch_replacement_point(codeinfo *code, s4 index, rplpoint *rp, u1 *savedmcode)
{
	s4 disp;
	u4 mcode;
	
	assert(0);

	if (index < 0) {
		/* restore the patched-over instruction */
		*(u4*)(rp->pc) = *(u4*)(savedmcode);
	}
	else {
		/* save the current machine code */
		*(u4*)(savedmcode) = *(u4*)(rp->pc);

		/* build the machine code for the patch */
		disp = ((u4*)code->replacementstubs - (u4*)rp->pc)
			   + index * REPLACEMENT_STUB_SIZE
			   - 1;

		mcode = (((s4)(0x00))<<30) | ((0)<<29) | ((0x8)<<25) | (0x1<<22) | (0<<20)
			  | (1 << 19 ) | ((disp) & 0x007ffff);

		/* write the new machine code */
		*(u4*)(rp->pc) = (u4) mcode;
	}
	
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
