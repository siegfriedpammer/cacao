/* src/vm/jit/x86_64/asmpart.S - Java-C interface functions for x86_64

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

#include "vm/jit/x86_64/arch.hpp"
#include "vm/jit/x86_64/md-abi.hpp"
#include "md-asm.hpp"

#include "vm/jit/abi-asm.hpp"
#include "vm/jit/methodheader.hpp"


	.text


/* export functions ***********************************************************/

	.globl asm_vm_call_method
	.globl asm_vm_call_method_int
	.globl asm_vm_call_method_long
	.globl asm_vm_call_method_float
	.globl asm_vm_call_method_double
	.globl asm_vm_call_method_exception_handler
	.globl asm_vm_call_method_end

	.globl asm_builtin_f2i
	.globl asm_builtin_f2l
	.globl asm_builtin_d2i
	.globl asm_builtin_d2l


/********************* function asm_calljavafunction ***************************
*                                                                              *
*   This function calls a Java-method (which possibly needs compilation)       *
*   with up to 4 address parameters.                                           *
*                                                                              *
*   This functions calls the JIT-compiler which eventually translates the      *
*   method into machine code.                                                  *
*                                                                              *
*   C-prototype:                                                               *
*    javaobject_header *asm_calljavamethod (methodinfo *m,                     *
*         void *arg1, void *arg2, void *arg3, void *arg4);                     *
*                                                                              *
*******************************************************************************/

	.align	8

	.long   0                           /* fltsave                            */
	.long   0                           /* intsave                            */
	.long   0                           /* isleaf                             */
	.long   0                           /* frame size                         */
	.quad   0                           /* codeinfo pointer                   */

asm_vm_call_method:
asm_vm_call_method_int:
asm_vm_call_method_long:
asm_vm_call_method_float:
asm_vm_call_method_double:
	sub     $(7*8),sp                   /* keep stack 16-byte aligned         */
	mov     %rbx,0*8(sp)                /* %rbx is not a callee saved in cacao*/
	mov     s0,1*8(sp)
	mov     s1,2*8(sp)
	mov     s2,3*8(sp)
	mov     s3,4*8(sp)
	mov     s4,5*8(sp)

	mov     a0,6*8(sp)                  /* store method PV                    */

	mov     sp,s0                       /* save stack pointer                 */

	mov     a1,t0                       /* address of data structure          */
	mov     a2,itmp1                    /* number of stack arguments          */

	mov     0*8(t0),a0
	mov     1*8(t0),a1
	mov     2*8(t0),a2
	mov     3*8(t0),a3
	mov     4*8(t0),a4
	mov     5*8(t0),a5

	movq    6*8(t0),fa0
	movq    7*8(t0),fa1
	movq    8*8(t0),fa2
	movq    9*8(t0),fa3
	movq    10*8(t0),fa4
	movq    11*8(t0),fa5
	movq    12*8(t0),fa6
	movq    13*8(t0),fa7

	cmp     $0,itmp1l
	je      L_asm_vm_call_method_stack_copy_done

	mov     itmp1,itmp2
	add     $1,itmp2                    /* keep stack 16-byte aligned         */
	and     $0xfffffffffffffffe,itmp2
	shl     $3,itmp2                    /* calculate stack size               */
	sub     itmp2,sp                    /* create stack frame                 */
	mov     sp,itmp2                    /* temporary stack pointer            */

L_asm_vm_call_method_stack_copy_loop:
	mov     14*8(t0),itmp3              /* load argument                      */
	mov     itmp3,0(itmp2)              /* store argument on stack            */

	sub     $1,itmp1l                   /* subtract 1 argument                */
	add     $8,t0                       /* set address of next argument       */
	add     $8,itmp2                    /* increase SP                        */

	cmp     $0,itmp1l
	jg      L_asm_vm_call_method_stack_copy_loop

L_asm_vm_call_method_stack_copy_done:
	lea     (6*8-256)(s0),mptr          /* We subtract 256 to force the next  */
	                                    /* move instruction to have a 32-bit  */
	                                    /* offset.                            */

	mov     (0*8+256)(mptr),itmp3       /* load PV                            */
	call    *itmp3

L_asm_vm_call_method_return:
	mov     s0,sp                       /* restore SP                         */

	mov     0*8(sp),%rbx                /* restore callee saved registers     */
	mov     1*8(sp),s0
	mov     2*8(sp),s1
	mov     3*8(sp),s2
	mov     4*8(sp),s3
	mov     5*8(sp),s4
	add     $(7*8),sp                   /* free stack space                   */
	ret
		
asm_vm_call_method_exception_handler:
	mov     xptr,a0                     /* pass exception pointer             */
	call    PIC_SYMBOL(builtin_throw_exception)
	jmp     L_asm_vm_call_method_return

asm_vm_call_method_end:
	nop


/* asm_builtin_x2x *************************************************************
*                                                                              *
*   Wrapper functions for float to int corner cases                            *
*                                                                              *
*******************************************************************************/

asm_builtin_f2i:
	sub     $(ARG_CNT*8),sp
	
	SAVE_ARGUMENT_REGISTERS(0)
	
	movq    ftmp1,fa0
	call    PIC_SYMBOL(builtin_f2i)
	
	RESTORE_ARGUMENT_REGISTERS(0)
	
	add     $(ARG_CNT*8),sp
	ret


asm_builtin_f2l:
	sub     $(ARG_CNT*8),sp
	
	SAVE_ARGUMENT_REGISTERS(0)
	
	movq    ftmp1,fa0
	call    PIC_SYMBOL(builtin_f2l)
	
	RESTORE_ARGUMENT_REGISTERS(0)
	
	add     $(ARG_CNT*8),sp
	ret


asm_builtin_d2i:
	sub     $(ARG_CNT*8),sp
	
	SAVE_ARGUMENT_REGISTERS(0)
	
	movq    ftmp1,fa0
	call    PIC_SYMBOL(builtin_d2i)
	
	RESTORE_ARGUMENT_REGISTERS(0)
	
	add     $(ARG_CNT*8),sp
	ret


asm_builtin_d2l:
	sub     $(ARG_CNT*8),sp
	
	SAVE_ARGUMENT_REGISTERS(0)
	
	movq    ftmp1,fa0
	call    PIC_SYMBOL(builtin_d2l)
	
	RESTORE_ARGUMENT_REGISTERS(0)
	
	add     $(ARG_CNT*8),sp
	ret


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
