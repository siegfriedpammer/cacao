/* src/vm/jit/alpha/asmpart.S - Java-C interface functions for alpha

   Copyright (C) 1996-2012
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

#include "vm/jit/alpha/md-abi.hpp"
#include "md-asm.hpp"

#include "vm/jit/abi-asm.hpp"
#include "vm/jit/methodheader.hpp"


	.text
	.set    noat
	.set    noreorder


/* export functions ***********************************************************/

	.globl asm_vm_call_method
	.globl asm_vm_call_method_int
	.globl asm_vm_call_method_long
	.globl asm_vm_call_method_float
	.globl asm_vm_call_method_double
	.globl asm_vm_call_method_exception_handler
	.globl asm_vm_call_method_end

	.globl asm_md_init
	.globl asm_cacheflush


/* asm_vm_call_method **********************************************************
*                                                                              *
*   This function calls a Java-method (which possibly needs compilation)       *
*   with up to 4 address parameters.                                           *
*                                                                              *
*   This functions calls the JIT-compiler which eventually translates the      *
*   method into machine code.                                                  *
*                                                                              *
*   C-prototype:                                                               *
*    javaobject_header *asm_calljavafunction (methodinfo *m,                   *
*         void *arg1, void *arg2, void *arg3, void *arg4);                     *
*                                                                              *
*******************************************************************************/

	.ent    asm_vm_call_method

	.align  3

	.long   0                           /* fltsave                            */
	.long   1                           /* intsave                            */
	.long   0                           /* isleaf                             */
	.long   0                           /* frame size                         */
	.quad   0                           /* codeinfo pointer                   */

asm_vm_call_method:
asm_vm_call_method_int:
asm_vm_call_method_long:
asm_vm_call_method_float:
asm_vm_call_method_double:
	ldgp    gp,0(pv)
	lda     sp,-5*8(sp)                 /* allocate stack space               */
	stq     ra,0*8(sp)                  /* save return address                */
	stq     gp,1*8(sp)                  /* save global pointer                */

	stq     s0,3*8(sp)
	stq     a0,4*8(sp)                  /* save method PV                     */

	mov     a1,t0                       /* address of argument array          */
	mov     a2,t1                       /* stack argument count               */
	mov     sp,s0                       /* save stack pointer                 */

	ldq     a0,0*8(t0)
	ldq     a1,1*8(t0)
	ldq     a2,2*8(t0)
	ldq     a3,3*8(t0)
	ldq     a4,4*8(t0)
	ldq     a5,5*8(t0)

	ldt     fa0,6*8(t0)
	ldt     fa1,7*8(t0)
	ldt     fa2,8*8(t0)
	ldt     fa3,9*8(t0)
	ldt     fa4,10*8(t0)
	ldt     fa5,11*8(t0)

	beq     t1,L_asm_vm_call_method_stack_copy_done

	negq    t1,t2                       /* calculate stackframe size (* 8)    */
	s8addq  t2,sp,sp                    /* create stackframe                  */
	mov     sp,t2                       /* temporary stack pointer            */

L_asm_vm_call_method_stack_copy_loop:
	ldq     t3,12*8(t0)                 /* load argument                      */
	stq     t3,0(t2)                    /* store argument on stack            */

	subq    t1,1,t1                     /* subtract 1 argument                */
	addq    t0,8,t0                     /* load address of next argument      */
	addq    t2,8,t2                     /* increase stack pointer             */

	bgt     t1,L_asm_vm_call_method_stack_copy_loop

L_asm_vm_call_method_stack_copy_done:
	lda     mptr,4*8(s0)                /* get address of PV                  */
	ldq     pv,0*8(mptr)                /* load PV                            */
	jmp     ra,(pv)
L_asm_vm_call_method_recompute_pv:
	lda     pv,(asm_vm_call_method - L_asm_vm_call_method_recompute_pv)(ra)

L_asm_vm_call_method_recompute_return:
	mov     s0,sp                       /* restore stack pointer              */

	ldq     ra,0*8(sp)                  /* restore RA                         */
	ldq     gp,1*8(sp)                  /* restore global pointer             */
	ldq     s0,3*8(sp)

	lda     sp,5*8(sp)                  /* free stack space                   */
	jmp     zero,(ra)

asm_vm_call_method_exception_handler:
	mov     s0,sp                       /* restore stack pointer              */
	ldq     gp,1*8(sp)                  /* restore global pointer             */
	mov     itmp1,a0
	jsr     ra,builtin_throw_exception

asm_vm_call_method_end:					
	br      L_asm_vm_call_method_recompute_return

	.end    asm_vm_call_method


/* asm_md_init *****************************************************************

   Initialize machine dependent stuff.

   Determines if the byte support instruction set (21164a and higher)
   is available.

*******************************************************************************/

	.ent    asm_md_init

asm_md_init:
	.long   0x47e03c20                  /* amask   1,v0                       */
	jmp     zero,(ra)                   /* return                             */

	.end    asm_md_init


/* asm_cacheflush **************************************************************

   XXX

*******************************************************************************/

	.ent    asm_cacheflush

asm_cacheflush:
	call_pal PAL_imb              /* synchronize instruction cache            */
	jmp     zero,(ra)

	.end    asm_cacheflush


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
