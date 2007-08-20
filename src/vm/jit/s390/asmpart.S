/* src/vm/jit/s390/asmpart.S - Java-C interface functions for s390

   Copyright (C) 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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

   $Id: asmpart.S 8299 2007-08-13 08:41:18Z michi $

*/


#include "config.h"

#include "vm/jit/s390/arch.h"
#include "vm/jit/s390/md-abi.h"
#include "vm/jit/s390/md-asm.h"

#include "vm/jit/abi-asm.h"
#include "vm/jit/methodheader.h"

	.text


/* export functions ***********************************************************/

	.globl asm_vm_call_method
	.globl asm_vm_call_method_int
	.globl asm_vm_call_method_long
	.globl asm_vm_call_method_float
	.globl asm_vm_call_method_double
	.globl asm_vm_call_method_exception_handler
	.globl asm_vm_call_method_end

	.globl asm_call_jit_compiler

	.globl asm_handle_exception
	.globl asm_handle_nat_exception

	.globl asm_abstractmethoderror

	.globl asm_replacement_out
	.globl asm_replacement_in

	.globl asm_builtin_f2i
	.globl asm_builtin_f2l
	.globl asm_builtin_d2i
	.globl asm_builtin_d2l


asm_abstractmethoderror:
	.long 0
asm_replacement_out:
	.long 0
asm_replacement_in:
	.long 0
asm_builtin_f2i:
	.long 0
asm_builtin_f2l:
	.long 0
asm_builtin_d2i:
	.long 0
asm_builtin_d2l:
	.long 0

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

	.long   0                         /* catch type all                       */
	.long   0                         /* exception handler pc                 */
	.long   0                         /* end pc                               */
	.long   0                         /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* line number table start              */
	.long   0                         /* line number table size               */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* IsLeaf                               */
	.long   0                         /* IsSync                               */
	.long   0                         /* frame size                           */
	.long   0                         /* codeinfo pointer                     */

asm_vm_call_method:
asm_vm_call_method_int:
asm_vm_call_method_long:
asm_vm_call_method_float:
asm_vm_call_method_double:

	ahi sp, -8*4                /* allocate stack frame */

	/* a0: PV */
	/* a1: data structure */
	/* a2: number of stack arguments */

	st    s0, 0*4(sp)           /* store used calle saved registers */
	st    s1, 1*4(sp)
	st    a0, 2*4(sp)
	st    mptr, 3*4(sp)         /* mptr/itmp2 is callee saved in terms of C abi */
	st    pv, 4*4(sp)
	st    a4, 5*4(sp)           /* a4 is callee saved in terms of C abi */
	st    ra, 6*4(sp)

	lr    s0, a1                /* data structure */
	lr    %r0, a2               /* number of stack arguments */

	l     a0, 0*8+4(s0)         /* big endian */
	l     a1, 1*8+4(s0)
	l     a2, 2*8+4(s0)
	l     a3, 3*8+4(s0)
	l     a4, 4*8+4(s0)         

	ld    fa0, 5*8(s0)
	ld    fa1, 6*8(s0)

	lr    s1, sp                /* backup stack pointer */

	ltr   %r0, %r0              /* are there any stack arguments ? */
	je    L_asm_vm_call_method_stack_copy_done
	lr    %r1, %r0              /* copy number of stack arguments */
	sll   %r1, 3                /* calculate stackframe size */ 
	sr    sp, %r1               /* allocate stack frame */
	lr    %r1, sp               /* temporary stack pointer */

L_asm_vm_call_method_stack_copy_loop:

	mvc   0(8, %r1), 7*8(s0)    /* copy argument */
	ahi   %r1, 8                /* increase sp */
	ahi   s0, 8                 /* set address of next argument */
	ahi   %r0, -1               /* substract 1 argument */
	jh    L_asm_vm_call_method_stack_copy_loop

L_asm_vm_call_method_stack_copy_done:

	la    mptr, 2*4(s1)         /* load method pointer */
	l     pv, 0(mptr)           /* load procedure vector from method pointer */
	basr  ra, pv                /* call method */
	lr    sp, s1                /* restore stack pointer */

L_asm_vm_call_method_return:

	l     s0, 0*4(sp)           /* restore used callee saved registers */
	l     s1, 1*4(sp)
	l     mptr, 3*4(sp)
	l     pv, 4*4(sp)
	l     a4, 5*4(sp)
	l     ra, 6*4(sp)

	ahi   sp, 8*4               /* remove stackframe */
	br    ra                    /* return */


asm_vm_call_method_exception_handler:
	lr    a0, xptr

	bras  %r14, L_avcmeh_bras
	.long builtin_throw_exception
L_avcmeh_bras:
	l     %r14, 0(%r14)
	ahi   sp, -96
	basr  %r14, %r14
	ahi   sp, 96
	j     L_asm_vm_call_method_return

asm_vm_call_method_end:
	brc   0,0	

/****************** function asm_call_jit_compiler *****************************
*                                                                              *
*   invokes the compiler for untranslated JavaVM methods.                      *
*                                                                              *
*   itmp1: methodinfo pointer                                                  *
*   itmp2: method pointer                                                      *
*                                                                              *
*******************************************************************************/

/*

argument registers: arguments (like in JIT)

        arguments on stack (like in JIT)
------------------------------------------------------------- <- SP on entry

        saved return address                                           \
        stored volatile (in terms of C ABI) floag argument registers   | 
96      stored volatile (in terms of C ABI) integer argument registers | ACJC_STACKFRAME
0 - 96  register save area (C ABI)                                     /
-------------------------------------------------- <- SP for jit_asm_compile
*/

/* This is called from a compiler stub.
 * Arguments are already in registers and the stack is setup like in CACAO.
 */

asm_call_jit_compiler:
L_asm_call_jit_compiler:

#	define ACJC_STACKFRAME (4 + (4 * 4) + (2 * 8) + 96)

	ahi     sp,-ACJC_STACKFRAME        /* allocate stack space */

	stm	    %r2,%r5,96(sp)             /* store volatile int arg regs */
	std     %f0,96+16(sp)              /* store volatile float arg regs */
	std     %f2,96+24(sp)              
	st      %r14,96+32(sp)             /* store return address */

	/* load arguments */

	lr      a0,itmp1                   /* pass methodinfo pointer            */
	lr      a1,itmp2                   /* pass method pointer                */
	la      a2,ACJC_STACKFRAME(sp)     /* pass java sp                       */
	la      a3,0(%r14)                 /* pass return address, make sure bit 32 is 0 */

	/* call jit_asm_compile in a PIC way */

	bras    itmp2, L_bras_jac
	.long   jit_asm_compile
L_bras_jac:
	l       itmp2, 0(itmp2)
	basr    %r14, itmp2

	lr      pv, v0                     /* save return value */

	lm      %r2,%r5,96(sp)             /* restore volatile int arg regs */
	ld      %f0,96+16(sp)              /* restore volatile float arg regs */
	ld      %f2,96+24(sp)              /* restore volatile float arg regs */

	ltr     pv,pv
	je      L_asm_call_jit_compiler_exception

	l       %r14,96+32(sp)             /* restore return address */
	ahi     sp, ACJC_STACKFRAME        /* remove stack frame */

jit_code_entry:                        /* label to set breakpoint on */
	br      pv                         /* call the method, it will return to the caller */


L_asm_call_jit_compiler_exception:
	bras    itmp2, L_bras_acjce
	.long exceptions_get_and_clear_exception
L_bras_acjce:
	l       itmp2, 0(itmp2)
	basr    %r14, itmp2
	lr      xptr, %r2
	l       xpc,96+32(sp)              /* restore return address */
	ahi     sp, ACJC_STACKFRAME        /* remove stack frame */
	j       L_asm_handle_nat_exception


/* asm_handle_exception ********************************************************
*                                                                              *
*   This function handles an exception. It does not use the usual calling      *
*   conventions. The exception pointer is passed in REG_ITMP1 and the          *
*   pc from the exception raising position is passed in REG_ITMP2. It searches *
*   the local exception table for a handler. If no one is found, it unwinds    *
*   stacks and continues searching the callers.                                *
*                                                                              *
*******************************************************************************/

asm_handle_nat_exception:
L_asm_handle_nat_exception:
	/* TODO really nothing here ? */
asm_handle_exception:
L_asm_handle_exception:

	/* a wrapper for md_handle_exception */

#	define STACKFRAMESIZE (96 + (16 * 4) + (16 * 8) + (4 * 4))
#	define REGS 96
#	define FREGS (96 + (16 * 4))
#	define OUT (96 + (16 * 4) + (16 * 8))

	ahi   sp, -STACKFRAMESIZE   /* allocate stack frame containing the arrays */

	/* store special registers to array */

	st    xptr, REGS+(1*4)(sp)
	st    xpc, REGS+(12*4)(sp)
	st    pv, REGS+(13*4)(sp)
	la    itmp3, STACKFRAMESIZE(sp)
	st    itmp3, REGS+(15*4)(sp)

	/* store temporary and argument registers */

	stm   a0, a4, REGS+(2*4)(sp)
	std   %f0, FREGS+(0*8)(sp)
	std   %f1, FREGS+(1*8)(sp)
	std   %f2, FREGS+(2*8)(sp)
	std   %f3, FREGS+(3*8)(sp)
	std   %f5, FREGS+(5*8)(sp)
	std   %f7, FREGS+(7*8)(sp)
	std   %f8, FREGS+(8*8)(sp)
	std   %f9, FREGS+(9*8)(sp)
	std   %f10, FREGS+(10*8)(sp)
	std   %f11, FREGS+(11*8)(sp)
	std   %f12, FREGS+(12*8)(sp)
	std   %f13, FREGS+(13*8)(sp)
	std   %f14, FREGS+(14*8)(sp)
	std   %f15, FREGS+(15*8)(sp)

	/* call md_handle_exception */

	la    a0, REGS(sp)
	la    a1, FREGS(sp)
	la    a2, OUT(sp)

	bras    %r14,L_ahe_bras
	.long   md_handle_exception
L_ahe_bras:
	l       %r14,0(%r14)
	basr    %r14,%r14

	l     itmp3, OUT+(2*4)(sp)  /* out[2] contains maybe leaf flag */
	ltr   itmp3, itmp3           
	je    L_restore_saved

L_restore_temporary_and_argument:

	/* if we are maybe leaf,
	 * we have to restore argument and temporary registers
	 */

	lm    a0, a4, REGS+(2*4)(sp)
	ld    %f0, FREGS+(0*8)(sp)
	ld    %f1, FREGS+(1*8)(sp)
	ld    %f2, FREGS+(2*8)(sp)
	ld    %f3, FREGS+(3*8)(sp)
	ld    %f5, FREGS+(5*8)(sp)
	ld    %f7, FREGS+(7*8)(sp)
	ld    %f8, FREGS+(8*8)(sp)
	ld    %f9, FREGS+(9*8)(sp)
	ld    %f10, FREGS+(10*8)(sp)
	ld    %f11, FREGS+(11*8)(sp)
	ld    %f12, FREGS+(12*8)(sp)
	ld    %f13, FREGS+(13*8)(sp)
	ld    %f14, FREGS+(14*8)(sp)
	ld    %f15, FREGS+(15*8)(sp)

	j     L_restore_done

L_restore_saved:

	/* if we are not a maybe leaf,
	 * we have to restore callee saved registers of the callee
	 */

	l     itmp3, OUT+(0*4)(sp)  /* out[0] contains IntSav */

	ahi   itmp3, -1
	jl    L_int_done
	l     s4, REGS+(11*4)(sp)

	ahi   itmp3, -1
	jl    L_int_done
	l     s3, REGS+(10*4)(sp)

	ahi   itmp3, -1
	jl    L_int_done
	l     s2, REGS+(9*4)(sp)

	ahi   itmp3, -1
	jl    L_int_done
	l     s1, REGS+(8*4)(sp)

	ahi   itmp3, -1
	jl    L_int_done
	l     s0, REGS+(7*4)(sp)

L_int_done:

	/* restore callee saved float registers */

	l     itmp3, OUT+(1*4)(sp)   /* out[1] contains FltSav */

	ahi   itmp3, -1
	jl    L_flt_done
	ld    %f6, FREGS+(6*8)(sp)

	ahi   itmp3, -1
	jl    L_flt_done
	ld    %f4, FREGS+(4*8)(sp)

L_flt_done:

L_restore_done:

	/* write new values for special registers */

	l     xptr, REGS+(1*4)(sp)
	l     xpc, REGS+(12*4)(sp)
	l     pv, REGS+(13*4)(sp)  
	l     sp, REGS+(15*4)(sp)   

	br    xpc                   /* jump to handler */

#	undef STACKFRAMESIZE
#	undef REGS
#	undef FREGS
#	undef OUT

#if 0

/* asm_abstractmethoderror *****************************************************

   Creates and throws an AbstractMethodError.

*******************************************************************************/

asm_abstractmethoderror:
	mov     sp,a0                       /* pass java sp                       */
	add     $1*8,a0
	mov     0*8(sp),a1                  /* pass exception address             */
	sub     $3,a1
	call	exceptions_asm_new_abstractmethoderror@PLT
	                                    /* exception pointer is return value  */
	pop     xpc                         /* get exception address              */
	sub     $3,xpc                      /* exception address is ra - 3        */
	jmp     L_asm_handle_exception

#endif

#if 0

/* asm_replacement_out *********************************************************

   This code is jumped to from the replacement-out stubs that are executed
   when a thread reaches an activated replacement point.

   The purpose of asm_replacement_out is to read out the parts of the
   execution state that cannot be accessed from C code, store this state,
   and then call the C function replace_me.

   Stack layout:
      8                 start of stack inside method to replace
      0   rplpoint *    info on the replacement point that was reached

*******************************************************************************/

/* some room to accomodate changes of the stack frame size during replacement */
	/* XXX we should find a cleaner solution here */
#define REPLACEMENT_ROOM  512

asm_replacement_out:
    /* create stack frame */
	sub     $(sizeexecutionstate + REPLACEMENT_ROOM),sp

	/* save registers in execution state */
	mov     %rax,(RAX*8+offes_intregs)(sp)
	mov     %rbx,(RBX*8+offes_intregs)(sp)
	mov     %rcx,(RCX*8+offes_intregs)(sp)
	mov     %rdx,(RDX*8+offes_intregs)(sp)
	mov     %rsi,(RSI*8+offes_intregs)(sp)
	mov     %rdi,(RDI*8+offes_intregs)(sp)
	mov     %rbp,(RBP*8+offes_intregs)(sp)
	movq    $0  ,(RSP*8+offes_intregs)(sp) /* not used */
	mov     %r8 ,(R8 *8+offes_intregs)(sp)
	mov     %r9 ,(R9 *8+offes_intregs)(sp)
	mov     %r10,(R10*8+offes_intregs)(sp)
	mov     %r11,(R11*8+offes_intregs)(sp)
	mov     %r12,(R12*8+offes_intregs)(sp)
	mov     %r13,(R13*8+offes_intregs)(sp)
	mov     %r14,(R14*8+offes_intregs)(sp)
	mov     %r15,(R15*8+offes_intregs)(sp)

	movq    %xmm0 ,(XMM0 *8+offes_fltregs)(sp)
	movq    %xmm1 ,(XMM1 *8+offes_fltregs)(sp)
	movq    %xmm2 ,(XMM2 *8+offes_fltregs)(sp)
	movq    %xmm3 ,(XMM3 *8+offes_fltregs)(sp)
	movq    %xmm4 ,(XMM4 *8+offes_fltregs)(sp)
	movq    %xmm5 ,(XMM5 *8+offes_fltregs)(sp)
	movq    %xmm6 ,(XMM6 *8+offes_fltregs)(sp)
	movq    %xmm7 ,(XMM7 *8+offes_fltregs)(sp)
	movq    %xmm8 ,(XMM8 *8+offes_fltregs)(sp)
	movq    %xmm9 ,(XMM9 *8+offes_fltregs)(sp)
	movq    %xmm10,(XMM10*8+offes_fltregs)(sp)
	movq    %xmm11,(XMM11*8+offes_fltregs)(sp)
	movq    %xmm12,(XMM12*8+offes_fltregs)(sp)
	movq    %xmm13,(XMM13*8+offes_fltregs)(sp)
	movq    %xmm14,(XMM14*8+offes_fltregs)(sp)
	movq    %xmm15,(XMM15*8+offes_fltregs)(sp)

	/* calculate sp of method */
	mov     sp,itmp1
	add     $(sizeexecutionstate + REPLACEMENT_ROOM + 8),itmp1
	mov     itmp1,(offes_sp)(sp)

	/* pv must be looked up via AVL tree */
	movq    $0,(offes_pv)(sp)

	/* call replace_me */
	mov     -8(itmp1),a0                /* rplpoint *                         */
    mov     sp,a1                       /* arg1: execution state              */
    call    replace_me@PLT              /* call C function replace_me         */
    call    abort@PLT                   /* NEVER REACHED                      */

/* asm_replacement_in **********************************************************

   This code writes the given execution state and jumps to the replacement
   code.

   This function never returns!

   C prototype:
      void asm_replacement_in(executionstate *es);

*******************************************************************************/

asm_replacement_in:
	mov     a0,%rbp                     /* executionstate *es                 */

	/* set new sp */
	mov     (offes_sp)(%rbp),%rsp
	
	/* store address of new code */
	push    (offes_pc)(%rbp)
	
	/* copy registers from execution state */
	movq    (XMM0 *8+offes_fltregs)(%rbp),%xmm0
	movq    (XMM1 *8+offes_fltregs)(%rbp),%xmm1
	movq    (XMM2 *8+offes_fltregs)(%rbp),%xmm2
	movq    (XMM3 *8+offes_fltregs)(%rbp),%xmm3
	movq    (XMM4 *8+offes_fltregs)(%rbp),%xmm4
	movq    (XMM5 *8+offes_fltregs)(%rbp),%xmm5
	movq    (XMM6 *8+offes_fltregs)(%rbp),%xmm6
	movq    (XMM7 *8+offes_fltregs)(%rbp),%xmm7
	movq    (XMM8 *8+offes_fltregs)(%rbp),%xmm8
	movq    (XMM9 *8+offes_fltregs)(%rbp),%xmm9
	movq    (XMM10*8+offes_fltregs)(%rbp),%xmm10
	movq    (XMM11*8+offes_fltregs)(%rbp),%xmm11
	movq    (XMM12*8+offes_fltregs)(%rbp),%xmm12
	movq    (XMM13*8+offes_fltregs)(%rbp),%xmm13
	movq    (XMM14*8+offes_fltregs)(%rbp),%xmm14
	movq    (XMM15*8+offes_fltregs)(%rbp),%xmm15

	mov     (RAX*8+offes_intregs)(%rbp),%rax
	mov     (RBX*8+offes_intregs)(%rbp),%rbx
	mov     (RCX*8+offes_intregs)(%rbp),%rcx
	mov     (RDX*8+offes_intregs)(%rbp),%rdx
	mov     (RSI*8+offes_intregs)(%rbp),%rsi
	mov     (RDI*8+offes_intregs)(%rbp),%rdi
	mov     (R8 *8+offes_intregs)(%rbp),%r8
	mov     (R9 *8+offes_intregs)(%rbp),%r9
	mov     (R10*8+offes_intregs)(%rbp),%r10
	mov     (R11*8+offes_intregs)(%rbp),%r11
	mov     (R12*8+offes_intregs)(%rbp),%r12
	mov     (R13*8+offes_intregs)(%rbp),%r13
	mov     (R14*8+offes_intregs)(%rbp),%r14
	mov     (R15*8+offes_intregs)(%rbp),%r15

	mov     (RBP*8+offes_intregs)(%rbp),%rbp

	/* jump to new code */
	ret
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
