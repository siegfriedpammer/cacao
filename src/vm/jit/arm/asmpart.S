/* src/vm/jit/arm/asmpart.S - Java-C interface functions for ARM

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


#include "config.h"

#include <sys/syscall.h>

#include "md-asm.hpp"

#include "vm/jit/methodheader.hpp"


	.file "asmpart.S"
	.text
	.align 2

#if defined(__ARMHF__)
	.fpu vfp
#endif


/* export functions ***********************************************************/

	.globl asm_vm_call_method
	.type  asm_vm_call_method, %function
	.globl asm_vm_call_method_int
	.type  asm_vm_call_method_int, %function
	.globl asm_vm_call_method_long
	.type  asm_vm_call_method_long, %function
	.globl asm_vm_call_method_float
	.type  asm_vm_call_method_float, %function
	.globl asm_vm_call_method_double
	.type  asm_vm_call_method_double, %function
	.globl asm_vm_call_method_exception_handler
	.globl asm_vm_call_method_end

	.globl asm_cacheflush
	.type  asm_cacheflush, %function


/* asm_vm_call_method **********************************************************

   This function calls a Java-method (which possibly needs compilation)
   with up to 4 address parameters.

   This functions calls the JIT-compiler which eventually translates the
   method into machine code.

*******************************************************************************/

	.align  2

	.word   0                           /* FltSave                            */
	.word   0                           /* IntSave                            */
	.word   0                           /* IsLeaf                             */
	.word   0                           /* FrameSize                          */
	.word   0                           /* CodeinfoPointer                    */

asm_vm_call_method:
asm_vm_call_method_int:
asm_vm_call_method_long:
asm_vm_call_method_float:
asm_vm_call_method_double:
	SAVE_SCRATCH_REGISTERS            /* save our personal scratch regs   */
	stmfd sp!, {v1}                   /* V1 is used to remember SP        */
	str   a0, [sp, #-4]!              /* store methods entrypoint         */

	mov   v1, sp                      /* remember SP                      */

	mov   itmp1, a1                   /* address of data structure        */
	mov   itmp3, a2                   /* stack argument count             */

	ldr   a0, [itmp1], #8             /* load argument registers          */
	ldr   a1, [itmp1], #8
	ldr   a2, [itmp1], #8
	ldr   a3, [itmp1], #8

#if defined(__ARMHF__)
	vldm itmp1!, {d0,d1,d2,d3,d4,d5}
#endif

	cmp   itmp3, #0                   /* do we have stack arguments?      */
	ble   asm_calljava_copyfinish     /* no -> do not care :-)            */

	mov   itmp2, #0
	sub   sp, sp, itmp3, lsl #3       /* create stackframe for arguments  */
asm_calljava_copyloop:                /* reorder stack arguments!         */
	ldr   ip, [itmp1], #4             /* load argument                    */
	str   ip, [sp, itmp2]             /* store argument on stack          */
	add   itmp2, itmp2, #4            /* next stackslot                   */
	ldr   ip, [itmp1], #4             /* load argument                    */
	str   ip, [sp, itmp2]             /* store argument on stack          */
	add   itmp2, itmp2, #4            /* next stackslot                   */
	subs  itmp3, itmp3, #1            /* next argument                    */
	bgt   asm_calljava_copyloop

asm_calljava_copyfinish:
	mov   mptr, v1                    /* set method pointer               */

	/* REMEMBER: do the method call just like in java! */
	ldr   ip, [mptr]                  /* fake virtual function call       */
	mov   lr, pc
 	mov   pc, ip
fake:
	sub   ip, pc, #(fake - asm_vm_call_method)+8

	mov   sp, v1                      /* restore SP                       */
	add   sp, sp, #4                  /* free fake address                */
	ldmfd sp!, {v1}
	RESTORE_SCRATCH_REGS_AND_RETURN   /* return to caller, restore regs   */

asm_vm_call_method_exception_handler:
	mov   a0, xptr                    /* exception pointer is arg1        */
	bl    builtin_throw_exception     /* throw the exception              */
	mov   res1, #0                    /* return NULL                      */
	mov   res2, #0                    /* return NULL                      */
	mov   sp, v1                      /* restore SP                       */
	add   sp, sp, #4                  /* free fake address                */
	ldmfd sp!, {v1}
	RESTORE_SCRATCH_REGS_AND_RETURN   /* return to caller, restore regs   */

asm_vm_call_method_end:


/********************* function asm_cacheflush *********************************
*                                                                              *
*   TODO: document me                                                          *
*                                                                              *
*   void asm_cacheflush(void *p, s4 size);                                     *
*                                                                              *
*******************************************************************************/

L___ARM_NR_cacheflush:
	.align 2
	.word __ARM_NR_cacheflush

asm_cacheflush:
	add   a1, a0, a1
	mov   a2, #0

#if defined(__ARM_EABI__)
	/* According to EABI, the syscall number should be passed via R7,
	   see "http://wiki.debian.org/ArmEabiPort" for additional details. */

	stmfd sp!, {r7}
	ldr   r7, L___ARM_NR_cacheflush
	swi   0x0
	ldmfd sp!, {r7}
#else
# if 0
	/* TWISTI: required on iyonix, maybe a linux-2.4 bug */
	mov   a0, #0x0
	mov   a1, #0xff000000
# endif

	swi   __ARM_NR_cacheflush
#endif

#if defined(ARM_NO_THUMB_IW)
	mov   pc, lr
#else
	bx    lr
#endif


/* disable exec-stacks ********************************************************/

#if defined(__linux__) && defined(__ELF__)
	.section .note.GNU-stack,"",%progbits
#endif


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: asm
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
