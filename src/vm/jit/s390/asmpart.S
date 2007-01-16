/* src/vm/jit/x86_64/asmpart.S - Java-C interface functions for x86_64

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
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

   Contact: cacao@cacaojvm.org

   Authors: Andreas Krall
            Reinhard Grafl
            Christian Thalinger

   Changes: Edwin Steiner

   $Id: asmpart.S 7219 2007-01-16 22:18:57Z pm $

*/


#include "config.h"

#include "vm/jit/s390/arch.h"
#include "vm/jit/s390/md-abi.h"
#include "vm/jit/s390/md-asm.h"
#include "vm/jit/s390/offsets.h"

#include "vm/jit/abi-asm.h"
#include "vm/jit/methodheader.h"

/* Copy a call to a PIC function from gcc -S
 * We setup a temporary literal pool pointer.
 */

#define PIC_CALL(fun, magic)                                  \
	bras itmp3, L_##magic##_lp_end                          ; \
L_##magic##_lp:                                             ; \
L_##magic##_lp_5:                                           ; \
	.long fun@PLTOFF                                        ; \
L_##magic##_lp_4:                                           ; \
	.long _GLOBAL_OFFSET_TABLE_-L_##magic##_lp              ; \
L_##magic##_lp_end:                                         ; \
	l       itmp2,L_##magic##_lp_4-L_##magic##_lp(itmp3)    ; \
	la      itmp2,0(itmp2,itmp3)                            ; \
	l       itmp1,L_##magic##_lp_5-L_##magic##_lp(itmp3)    ; \
	bas     %r14,0(itmp1,itmp2)                             

	.text


/* export functions ***********************************************************/

	.globl asm_vm_call_method
	.globl asm_vm_call_method_int
	.globl asm_vm_call_method_long
	.globl asm_vm_call_method_float
	.globl asm_vm_call_method_double
	.globl asm_vm_call_method_exception_handler

	.globl asm_call_jit_compiler

	.globl asm_handle_exception
	.globl asm_handle_nat_exception

	.globl asm_abstractmethoderror

	.globl asm_patcher_wrapper

	.globl asm_replacement_out
	.globl asm_replacement_in

	.globl asm_builtin_f2i
	.globl asm_builtin_f2l
	.globl asm_builtin_d2i
	.globl asm_builtin_d2l

	.globl asm_criticalsections
	.globl asm_getclassvalues_atomic


#if 0
asm_vm_call_method:
asm_vm_call_method_int:
asm_vm_call_method_long:
asm_vm_call_method_float:
asm_vm_call_method_double:
#endif
asm_vm_call_method_exception_handler:
#if 0
asm_call_jit_compiler:
#endif
asm_handle_exception:
asm_handle_nat_exception:
asm_abstractmethoderror:
asm_patcher_wrapper:
asm_replacement_out:
asm_replacement_in:
asm_builtin_f2i:
asm_builtin_f2l:
asm_builtin_d2i:
asm_builtin_d2l:
asm_criticalsections:
asm_getclassvalues_atomic:
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
	.align	8

	.quad   0                           /* catch type all                     */
	.quad   0                           /* handler pc                         */
	.quad   0                           /* end pc                             */
	.quad   0                           /* start pc                           */
	.long   1                           /* extable size                       */
	.long   0                           /* ALIGNMENT PADDING                  */
	.quad   0                           /* line number table  start           */
	.quad   0                           /* line number table  size            */
	.long   0                           /* ALIGNMENT PADDING                  */
	.long   0                           /* fltsave                            */
	.long   0                           /* intsave                            */
	.long   0                           /* isleaf                             */
	.long   0                           /* IsSync                             */
	.long   0                           /* frame size                         */
	.quad   0                           /* codeinfo pointer                   */


asm_vm_call_method:
asm_vm_call_method_int:
asm_vm_call_method_long:
asm_vm_call_method_float:
asm_vm_call_method_double:

/*

a0:   methodinfo *m
a1:   s4 vmargscount   ---> v0: java_objectheader *
a2:   vm_arg *vmargs
r14:  return address

96 ...     on stack parameters (none)
0 - 96     register save area
-------------------------------------------------- <- SP on asm_vm_... entry
           arguments on stack
---------------------------------------------------- <- SP on JIT code entry
           saved return address (callee saved)

*/


/*
	Regiser usage:
	itmp1: argument block pointer
	itmp2: argument counter
	s0: integer argument counter
	s1: float argument counter
	s2: integer register counter
	s3: backup argument block pointer
	s4: backup argument count
*/

	stm   %r6, %r15, 24(sp)               /* save callers regiters */
	ahi   sp, -16                         /* allocate stack space for local variables */
	stm   a0, a2, 0(sp)                   /* save arguments */

	ltr   a1, a1                          /* maybe we have no args... */
	je    L_no_args

	lr    itmp2, a1                       /* load arg count */
	lr    itmp1, a2                       /* load arg pointer */

	ahi   itmp1, -sizevmarg               /* initialize arg pointer */
	ahi   itmp2, 1                        /* initialize arg count */ 
	lhi   s0, 0                           /* initialize integer arg counter */
	lhi   s2, 0                           /* initialize integer register counter */
	lhi   s1, 0                           /* initialize float arg counter */

	lr    s4, a1                          /* backup arg count */
	lr    s3, a2                          /* backup arg pointer */

L_register_copy:

	ahi   itmp1, sizevmarg                /* forward arg pointer */
	ahi   itmp2, -1                       /* decrement arg count */
	je    L_register_copy_done            /* no arguments left */

	tm    offvmargtype(itmp1), 0x02       /* is this a float/double type? */
	jne   L_register_handle_float

L_register_handle_int:

	chi   s2, INT_ARG_CNT	              /* are we out of integer arg registers ? */
	je    L_register_copy                 /* yes, next loop */

	tm    offvmargtype(itmp1), 0x01       /* is this a 2 word type ? */
	jne   L_register_handle_long

	ahi   s0, 1                           /* increment integer arg counter */
	ahi   s2, 1                           /* increment integer register counter */

	/* handle argument */

	chi   s2, 1
	je    L_handle_i0
	chi   s2, 2
	je    L_handle_i1
	chi   s2, 3
	je    L_handle_i2
	chi   s2, 4
	je    L_handle_i3
	chi   s2, 5
	je    L_handle_i4

L_register_handle_long:

	chi   s2, (INT_ARG_CNT - 1)           /* are there 2 integer arg registers left ? */
	jl    L_register_handle_long_continue /* yes */
	lhi   s2, INT_ARG_CNT                 /* no, drop last register */
	j     L_register_copy

L_register_handle_long_continue:

	ahi   s0, 1                           /* increment integer arg counter */
	ahi   s2, 2                           /* consume 2 integer arg registers */

	/* handle argument */

	chi   s2, 1
	je    L_handle_l0
	chi   s2, 2
	je    L_handle_l1
	chi   s2, 3
	je    L_handle_l2
	chi   s2, 4
	je    L_handle_l3

L_register_handle_float:

	chi   s1, FLT_ARG_CNT                 /* are we out of float arg registers */
	je    L_register_copy                 /* no arg regisers left */

	ahi   s1, 1                           /* increment float argument counter */

	tm    offvmargtype(itmp1), 0x01       /* is this a 2 word type ? */
	jne   L_register_handle_double

	/* handle argument */

	chi   s1, 1
	je    L_handle_f0
	chi   s1, 2
	je    L_handle_f1

L_register_handle_double:

	/* handle argument */

	chi   s1, 1
	je    L_handle_d0
	chi   s1, 2
	je    L_handle_d1

L_register_copy_done:

/*
	Regiser usage:
	itmp1: argument block pointer
	itmp2: argument counter
	s0: integer argument counter (initialized by previous code) 
	s1: float argument counter (initialized by previous code)
	s2: pointer to current argument on stack
	s3: backup argument block pointer (used to initialize itmp1)
	    after used as backup of original stack pointer
	s4: backup argument count (used to initialize itmp2)
	    after used as size of parameters on stack
*/

	lr    itmp2, s4                     /* restore argument counter */
	lr    itmp1, s3                     /* restore argument block pointer */

	                                    /* calculate remaining arguments */
	sr    s4, s0                        /* - integer arguments in registers */
	sr    s4, s1                        /* - float arguments in registers */

	je    L_copy_done                   /* no arguments left for stack */

	sll   s4, 3                         /* allocate 8 bytes per parameter on stack */	
	lr    s3, sp                        /* backup stack pointer */
	sr    sp, s4                        /* allocate stack space for arguments */ 

	lr    s2, sp                        /* points now to current argument on stack */

	ahi   itmp1, -sizevmarg             /* initialize argument block pointer */
	ahi   itmp2, 1                      /* initialize argument counter */

L_stack_copy_loop:

	ahi   itmp1, sizevmarg              /* forward argument block pointer */
	ahi   itmp2, -1                     /* decrement argument counter */
	je    L_copy_done                   /* all arguments done */

	tm    offvmargtype(itmp1), 0x0      /* is this a float/double type? */
	jne   L_stack_handle_float

L_stack_handle_int:

	ahi   s0, -1                         /* decrement number of integer arguments in regs */
	jhe   L_stack_copy_loop              /* argument is in register */

	tm    offvmargtype(itmp1), 0x01      /* is this a 2 word type ? */
	jne   L_stack_handle_long

	mvc   0(4, s2), offvmargdata(itmp1)  /* copy integer value */
	ahi   s2, 4
	j     L_stack_copy_loop

L_stack_handle_long:

	mvc   0(8, s2), offvmargdata(itmp1)  /* copy long value */
	ahi   s2, 8
	j     L_stack_copy_loop

L_stack_handle_float:

	ahi   s1, -1                         /* decrement number of float arguments in regs */
	jhe   L_stack_copy_loop              /* argument is in register */

	tm    offvmargtype(itmp1), 0x01      /* is this a 2 word type ? */
	jne   L_stack_handle_double

	mvc   0(4, s2), offvmargdata(itmp1)  /* copy float value */
	ahi   s2, 4
	j     L_stack_copy_loop

L_stack_handle_double:

	mvc   0(8, s2), offvmargdata(itmp1)  /* copy double value */
	ahi   s2, 8
	j     L_stack_copy_loop

L_copy_done:

	/* Now we call the compiler in a rather questionable way i needed
	 * some days to understand:
	 *
	 * We can't simply call asm_call_jit_compiler, but we have to call an 
	 * address loaded from memory like it is done in JIT code.
	 *
	 * This is because the compiler will intercept the instruction before 
	 * the call instruction, extract the address where the function pointer
	 * has been loaded from and overwrite it with the code entry.
	 *
	 * Arguments are passed in temporary registers.
	 */

	/* load address of L_asm_call_jit_compiler into memory */

	basr  mptr, 0                         /* store PC */
L_basr:
	la    mptr, L_asm_call_jit_compiler-L_basr(mptr) /* add offset to PC */
	st    mptr, 12(s3)                    /* store on stack */

	l     itmp1, 0(s3)                    /* load methodinfo for compiler */
	la    mptr, 12(s3)                    /* store **function in mptr for compiler */

	/* call L_asm_call_jit_compiler like JIT code would do */

	l     itmp3, 12(s3)                   /* load address of target from memory */
	basr  %r14, itmp3                     /* jump to target */

	/* todo will s4 survive the call? */
	ar    sp, s4                          /* remove stack space for arguments */

L_asm_vm_call_method_return:

	ahi   sp, 16                          /* remove stack space for local variables */
	lm    %r6, %r15, 24(sp)               /* restore callers registers */
	br    %r14                            /* return */

/* .... */

L_no_args:
	lr    s3, sp
	j     L_copy_done

L_handle_i0:
	l     a0, offvmargdata(itmp1)
	j     L_register_copy
L_handle_i1:
	l     a1, offvmargdata(itmp1)
	j     L_register_copy
L_handle_i2:
	l     a2, offvmargdata(itmp1)
	j     L_register_copy
L_handle_i3:
	l     a3, offvmargdata(itmp1)
	j     L_register_copy
L_handle_i4:
	l     a4, offvmargdata(itmp1)
	j     L_register_copy

L_handle_l0:
	lm    a0, a1, offvmargdata(itmp1)
	j     L_register_copy
L_handle_l1:
	lm    a1, a2, offvmargdata(itmp1)
	j     L_register_copy
L_handle_l2:
	lm    a2, a3, offvmargdata(itmp1)
	j     L_register_copy
L_handle_l3:
	lm    a3, a4, offvmargdata(itmp1)
	j     L_register_copy

L_handle_f0:
	le    fa0, offvmargdata(itmp1)
	j     L_register_copy
L_handle_f1:
	le    fa1, offvmargdata(itmp1)
	j     L_register_copy

L_handle_d0:
	ld    fa0, offvmargdata(itmp1)
	j     L_register_copy
L_handle_d1:
	ld    fa1, offvmargdata(itmp1)
	j     L_register_copy


/* .... */

#if 0

	.align	8

	.quad   0                           /* catch type all                     */
	.quad   0                           /* handler pc                         */
	.quad   0                           /* end pc                             */
	.quad   0                           /* start pc                           */
	.long   1                           /* extable size                       */
	.long   0                           /* ALIGNMENT PADDING                  */
	.quad   0                           /* line number table  start           */
	.quad   0                           /* line number table  size            */
	.long   0                           /* ALIGNMENT PADDING                  */
	.long   0                           /* fltsave                            */
	.long   0                           /* intsave                            */
	.long   0                           /* isleaf                             */
	.long   0                           /* IsSync                             */
	.long   0                           /* frame size                         */
	.quad   0                           /* codeinfo pointer                   */

asm_vm_call_method:
asm_vm_call_method_int:
asm_vm_call_method_long:
asm_vm_call_method_float:
asm_vm_call_method_double:

	stm %r6,%r15,24(sp)                /* save callers regiters */

	ahi sp, -12  /* allocate space on stack for local variables */

	st a0, 0(sp) /* store method info */
	st a1, 4(sp) /* store arg count */
	st a2, 8(sp) /* store args */


#	define r_methodinfo  itmp1
#	define r_vmargscount itmp2
#	define r_vmargs      s0

#	define r_arg_ctr     s1
#	define r_arg_ptr     s2

#	define r_int_ctr     s3
#	define r_float_ctr   s4
#   define r_int_extra_ctr    itmp3

	/* save arguments */

	lr      r_methodinfo,a0                    /* move method pointer for compiler   */
	lr      r_vmargscount,a1
	lr      r_vmargs,a2                    

	ltr     a1,a1                       /* maybe we have no args...           */
	je      L_copy_done

	lr      r_arg_ctr,r_vmargscount         /* arg count                          */
	lr      r_arg_ptr,r_vmargs              /* pointer to arg block               */

	ahi     r_arg_ptr,-sizevmarg            /* initialize pointer (smaller code)  */
	ahi     r_arg_ctr,1                     /* initialize argument count          */

	lhi     r_int_ctr,0                      /* initialize integer argument counter*/
	lhi     r_float_ctr,0                      /* initialize float argument counter  */
	lhi     r_int_extra_ctr,0

L_register_copy:
	ahi     r_arg_ptr,sizevmarg             /* goto next argument block           */
	ahi     r_arg_ctr,-1                    /* argument count - 1                 */
	je      L_register_copy_done

	tm      offvmargtype(r_arg_ptr),0x02    /* is this a float/double type?       */
	jne     L_register_handle_float     /* yes, handle it                     */

	cli     offvmargtype(r_arg_ptr),0x01
	je      L_register_handle_long

L_register_handle_int:

	chi     r_int_ctr,INT_ARG_CNT            /* are we out of integer argument     */
	je      L_register_copy             /* register? yes, next loop           */

	ahi     r_int_ctr,1

	chi     r_int_ctr,1
	je      L_handle_a0
	chi     r_int_ctr,2
	je      L_handle_a1
	chi     r_int_ctr,3
	je      L_handle_a2
	chi     r_int_ctr,4
	je      L_handle_a3
	chi     r_int_ctr,5
	je      L_handle_a4

L_register_handle_long:

	chi     r_int_ctr,INT_ARG_CNT-1 /* only one integer register left ? */
	je      L_register_handle_long_last_reg

	chi     r_int_ctr,INT_ARG_CNT /* no registers left */
	je     L_register_copy

	ahi     r_int_ctr,2
	ahi     r_int_extra_ctr,1

	chi     r_int_ctr,2
	je      L_handle_al0
	chi     r_int_ctr,3
	je      L_handle_al1
	chi     r_int_ctr,4
	je      L_handle_al2
	chi     r_int_ctr,5
	je      L_handle_al3

L_register_handle_long_last_reg:
	ahi     r_int_ctr,1 /* skip the integer register */
	ahi     r_int_extra_ctr,1
	j       L_register_copy

L_register_handle_float:
	chi     r_float_ctr,FLT_ARG_CNT         /* are we out of float argument         */
	je      L_register_copy           /* register? yes, next loop             */

	ahi     r_float_ctr,1

	chi     r_int_ctr,1
	je      L_handle_af0
	chi     r_int_ctr,2
	je      L_handle_af1

L_register_copy_done:

	lr      r_arg_ctr, r_vmargscount
	sr      r_arg_ctr, r_int_ctr
	ar      r_arg_ctr, r_int_extra_ctr
	sr      r_arg_ctr, r_float_ctr
	jle     L_copy_done 

	/* now allocate the parameter area on the stack
	 * the register save area will be allocated later
	 */

#	define r_arg_stack_ptr r_arg_ptr

 	sll     r_arg_ctr, 3 /* 8 bytes per parameter on stack */
	sr      sp,r_arg_ctr /* allocate stack space for parameters */
	lr      r_arg_stack_ptr,sp /* points now to arguments on stack */

	ahi     r_vmargs,-sizevmarg            /* initialize pointer (smaller code)  */
	ahi     r_vmargscount,1                     /* initialize argument count          */
		
L_stack_copy_loop:
	ahi     r_vmargs,sizevmarg             /* goto next argument block           */
	ahi     r_vmargscount,-1                    /* argument count - 1                 */
	jz      L_copy_done               /* no test needed after dec             */

	tm      offvmargtype(r_vmargs),0x02    /* is this a float/double type?       */
	jne     L_stack_handle_float     /* yes, handle it                     */

	cli     offvmargtype(r_vmargs),0x01
	je      L_stack_handle_long

L_stack_handle_int:

	ahi     r_int_ctr,-1                        /* arguments assigned to registers    */
	jhe     L_stack_copy_loop
	j       L_stack_copy

L_stack_handle_long:

	ahi     r_int_ctr,-2
	jhe     L_stack_copy_loop
	j       L_stack_copy

L_stack_handle_float:
	ahi     r_float_ctr,-1                        /* arguments assigned to registers    */
	jhe     L_stack_copy_loop

L_stack_copy:
	mvc     0(8,r_arg_stack_ptr),offvmargdata(r_vmargs) /* copy s8 argument onto stack        */

	ahi     r_arg_stack_ptr,8                     /* increase sp to next argument       */
	j       L_stack_copy_loop

L_copy_done:
#if 0
	                                    /* itmp1 still contains method pointer*/
	lea     L_asm_call_jit_compiler(%rip),mptr
	mov     sp,itmp3                    /* calculate the old stack pointer    */
	add     bp,itmp3
	mov     mptr,6*8(itmp3) /* store mptr on stack */
	lea     (6*8-256)(itmp3),mptr       /* We subtract 256 to force the next  */
	                                    /* move instruction to have a 32-bit  */
	                                    /* offset.                            */

	mov     (0*8+256)(mptr),itmp3       /* method call as in Java             */
	call    *itmp3                      /* call JIT compiler                  */

	add     bp,sp                       /* remove argument stack frame if any */
#endif

	                                    /* itmp1 still contains method pointer*/
	
	bras %r14, L_asm_call_jit_compiler
	ar      sp,r_arg_ctr                /* r_arg_ctr in callee saved regiter */

L_asm_vm_call_method_return:

	ahi sp, 12                        /* free stack space */
	lm %r6,%r15,24(sp)                /* restore registers */
	br %r14                           /* branch to return address */
		
#if 0
asm_vm_call_method_exception_handler:
	mov     xptr,a0                     /* pass exception pointer             */
	call    builtin_throw_exception@PLT
	jmp     L_asm_vm_call_method_return
#endif

L_handle_a0:
	l       a0,offvmargdata(r_arg_ptr)
	j       L_register_copy
L_handle_a1:
	l       a1,offvmargdata(r_arg_ptr)
	j       L_register_copy
L_handle_a2:
	l       a2,offvmargdata(r_arg_ptr)
	j       L_register_copy
L_handle_a3:
	l       a3,offvmargdata(r_arg_ptr)
	j       L_register_copy
L_handle_a4:
	l       a4,offvmargdata(r_arg_ptr)
	j       L_register_copy

L_handle_al0:
	l       a0,offvmargdata(r_arg_ptr)
	l       a1,offvmargdata+4(r_arg_ptr)
	j       L_register_copy
L_handle_al1:
	l       a1,offvmargdata(r_arg_ptr)
	l       a2,offvmargdata+4(r_arg_ptr)
	j       L_register_copy
L_handle_al2:
	l       a2,offvmargdata(r_arg_ptr)
	l       a3,offvmargdata+4(r_arg_ptr)
	j       L_register_copy
L_handle_al3:
	l       a3,offvmargdata(r_arg_ptr)
	l       a4,offvmargdata+4(r_arg_ptr)
	j       L_register_copy

L_handle_af0:
	ld    fa0,offvmargdata(r_arg_ptr)
	j     L_register_copy
L_handle_af1:
	ld    fa1,offvmargdata(r_arg_ptr)
	j     L_register_copy

#endif

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

	bras    itmp3, L_bras_jac
	.long   jit_asm_compile
L_bras_jac:
	l       itmp3, 0(itmp3)
	basr    %r14, itmp3

	lm     %r2,%r5,96(sp)             /* restore volatile int arg regs */
	ld     %f0,96+16(sp)              /* store volatile float arg regs */
	ld     %f2,96+24(sp)              /* store volatile float arg regs */
	ld     %r14,96+32(sp)             /* restore return address */

#if 0
	ltr     v0,v0
	je      L_asm_call_jit_compiler_exception
#endif

	b       0(v0)                      /* call the method, it will return to the caller */


L_asm_call_jit_compiler_exception:
#if 0
	call    exceptions_get_and_clear_exception@PLT
	pop     xpc                         /* delete return address              */
	sub     $3,xpc                      /* faulting address is ra - 3         */
	jmp     L_asm_handle_exception
#endif
	.long 0


#if 0
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
	add     $8,sp                       /* clear return address of native stub*/
		
asm_handle_exception:
L_asm_handle_exception:                 /* required for PIC code              */
	sub     $((ARG_CNT+TMP_CNT)*8),sp   /* create maybe-leaf stackframe       */

	SAVE_ARGUMENT_REGISTERS(0)          /* we save arg and temp registers in  */
	SAVE_TEMPORARY_REGISTERS(ARG_CNT)   /* case this is a leaf method         */

	mov     $((ARG_CNT+TMP_CNT)*8),a3   /* prepare a3 for handle_exception    */
	mov     $1,t0                       /* set maybe-leaf flag                */

L_asm_handle_exception_stack_loop:
	sub     $(6*8),sp
	mov     xptr,0*8(sp)                /* save exception pointer             */
	mov     xpc,1*8(sp)                 /* save exception pc                  */
	add     sp,a3                       /* calculate Java sp into a3...       */
	add     $(6*8),a3
	mov     a3,3*8(sp)                  /* ...and save it                     */
	mov     t0,4*8(sp)                  /* save maybe-leaf flag               */

	mov     xpc,a0                      /* exception pc                       */
	call    codegen_get_pv_from_pc@PLT
	mov     v0,2*8(sp)                  /* save data segment pointer          */
        
	mov     0*8(sp),a0                  /* pass exception pointer             */
	mov     1*8(sp),a1                  /* pass exception pc                  */
	mov     v0,a2                       /* pass data segment pointer          */
	mov     3*8(sp),a3                  /* pass Java stack pointer            */
	call    exceptions_handle_exception@PLT

	test    v0,v0
	jz      L_asm_handle_exception_not_catched

	mov     v0,xpc                      /* move handlerpc into xpc            */
	mov     0*8(sp),xptr                /* restore exception pointer          */
	mov     4*8(sp),t0                  /* get maybe-leaf flag                */
	add     $(6*8),sp                   /* free stack frame                   */

	test    t0,t0                       /* test for maybe-leaf flag           */
	jz      L_asm_handle_exception_no_leaf

	RESTORE_ARGUMENT_REGISTERS(0)       /* if this is a leaf method, we have  */
	RESTORE_TEMPORARY_REGISTERS(ARG_CNT)/* to restore arg and temp registers  */

	add     $((ARG_CNT+TMP_CNT)*8),sp   /* remove maybe-leaf stackframe       */

L_asm_handle_exception_no_leaf:
	jmp     *xpc                        /* jump to the handler                */

L_asm_handle_exception_not_catched:
	mov     0*8(sp),xptr                /* restore exception pointer          */
	mov     2*8(sp),itmp3               /* restore data segment pointer       */
	mov     4*8(sp),t0                  /* get maybe-leaf flag                */
	add     $(6*8),sp

	test    t0,t0
	jz      L_asm_handle_exception_no_leaf_stack

	add     $((ARG_CNT+TMP_CNT)*8),sp   /* remove maybe-leaf stackframe       */
	xor     t0,t0                       /* clear the isleaf flags             */

L_asm_handle_exception_no_leaf_stack:
	mov     FrameSize(itmp3),itmp2l     /* get frame size                     */
	add     sp,itmp2                    /* pointer to save area               */
	
	mov     IntSave(itmp3),a0l          /* a0l = saved int register count     */
	test    a0l,a0l
	je      noint
	
	cmp     $1,a0l
	je      int1
	cmp     $2,a0l
	je      int2
	cmp     $3,a0l
	je      int3
	cmp     $4,a0l
	je      int4
	
	mov     -5*8(itmp2),s0
int4:   
	mov     -4*8(itmp2),s1
int3:   
	mov     -3*8(itmp2),s2
int2:   
	mov     -2*8(itmp2),s3
int1:   
	mov     -1*8(itmp2),s4

	shl     $3,a0l                      /* multiply by 8 bytes                */
	sub     a0,itmp2
		
noint:
#if 0
	mov     FltSave(itmp3),a0l          /* a0l = saved flt register count     */
	test	a0l,a0l
	je      noflt
	
	cmpl    $1,a0l
	je      flt1
	cmpl    $2,a0l
	je      flt2
	cmpl    $3,a0l
	je      flt3
	cmpl    $4,a0l
	je      flt4

	movq    -5*8(itmp2),%xmm11
flt4:   
	movq    -4*8(itmp2),%xmm12
flt3:   
	movq    -3*8(itmp2),%xmm13
flt2:   
	movq    -2*8(itmp2),%xmm14
flt1:   
	movq    -1*8(itmp2),%xmm15
                
noflt:
#endif
	mov     FrameSize(itmp3),itmp2l     /* get frame size                     */
	add     itmp2,sp                    /* unwind stack                       */

	                                    /* exception pointer is still set     */
	pop     xpc                         /* the new xpc is return address      */
	sub     $3,xpc                      /* subtract 3 bytes for call          */

	xor     a3,a3                       /* prepare a3 for handle_exception    */
	
	jmp		L_asm_handle_exception_stack_loop


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


/* asm_patcher_wrapper *********************************************************

   XXX

   Stack layout:
     40   return address
     32   pointer to virtual java_objectheader
     24   machine code (which is patched back later)
     16   unresolved class/method/field reference
      8   data segment displacement from load instructions
      0   pointer to patcher function
     -8   bp

*******************************************************************************/

asm_patcher_wrapper:
	push    bp                          /* save base pointer                  */
	mov     sp,bp                       /* move actual sp to bp               */
	sub     $((3+ARG_CNT+TMP_CNT)*8+sizestackframeinfo),sp
	and     $0xfffffffffffffff0,sp      /* align sp to 16-byte (this is for   */
	                                    /* leaf functions)                    */

	SAVE_ARGUMENT_REGISTERS(3)
	SAVE_TEMPORARY_REGISTERS(3+ARG_CNT)

	mov     itmp1,0*8(sp)               /* save itmp1 and itmp2               */
	mov     itmp2,1*8(sp)               /* can be used by some instructions   */

	mov     bp,a0                       /* pass SP of patcher stub            */
	add     $(1*8),a0
	mov     $0,a1                       /* pass PV (if NULL, use findmethod)  */
	mov     $0,a2                       /* pass RA (it's on the stack)        */
	call    patcher_wrapper@PLT
	mov     v0,2*8(sp)                  /* save return value                  */

	RESTORE_ARGUMENT_REGISTERS(3)
	RESTORE_TEMPORARY_REGISTERS(3+ARG_CNT)

	mov     0*8(sp),itmp1               /* restore itmp1 and itmp2            */
	mov     1*8(sp),itmp2               /* can be used by some instructions   */
	mov     2*8(sp),itmp3               /* restore return value               */

	mov     bp,sp                       /* restore original sp                */
	pop     bp                          /* restore bp                         */
	add     $(5*8),sp                   /* remove patcher stackframe, keep RA */

	test    itmp3,itmp3                 /* exception thrown?                  */
	jne     L_asm_patcher_wrapper_exception
	ret                                 /* call new patched code              */

L_asm_patcher_wrapper_exception:
	mov     itmp3,xptr                  /* get exception                      */
	pop     xpc                         /* get and remove return address      */
	jmp     L_asm_handle_exception


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


/* asm_builtin_x2x *************************************************************
*                                                                              *
*   Wrapper functions for float to int corner cases                            *
*                                                                              *
*******************************************************************************/

asm_builtin_f2i:
	sub     $(ARG_CNT*8),sp
	
	SAVE_ARGUMENT_REGISTERS(0)
	
	movq    ftmp1,fa0
	call    builtin_f2i@PLT
	
	RESTORE_ARGUMENT_REGISTERS(0)
	
	add     $(ARG_CNT*8),sp
	ret


asm_builtin_f2l:
	sub     $(ARG_CNT*8),sp
	
	SAVE_ARGUMENT_REGISTERS(0)
	
	movq    ftmp1,fa0
	call    builtin_f2l@PLT
	
	RESTORE_ARGUMENT_REGISTERS(0)
	
	add     $(ARG_CNT*8),sp
	ret


asm_builtin_d2i:
	sub     $(ARG_CNT*8),sp
	
	SAVE_ARGUMENT_REGISTERS(0)
	
	movq    ftmp1,fa0
	call    builtin_d2i@PLT
	
	RESTORE_ARGUMENT_REGISTERS(0)
	
	add     $(ARG_CNT*8),sp
	ret


asm_builtin_d2l:
	sub     $(ARG_CNT*8),sp
	
	SAVE_ARGUMENT_REGISTERS(0)
	
	movq    ftmp1,fa0
	call    builtin_d2l@PLT
	
	RESTORE_ARGUMENT_REGISTERS(0)
	
	add     $(ARG_CNT*8),sp
	ret


asm_getclassvalues_atomic:
_crit_restart:
_crit_begin:
	movl    offbaseval(a0),itmp1l
	movl    offdiffval(a0),itmp2l
	movl    offbaseval(a1),itmp3l
_crit_end:
	movl    itmp1l,offcast_super_baseval(a2)
	movl    itmp2l,offcast_super_diffval(a2)
	movl    itmp3l,offcast_sub_baseval(a2)
	ret

	.data
		
asm_criticalsections:
#if defined(ENABLE_THREADS)
	.quad   _crit_begin
	.quad   _crit_end
	.quad   _crit_restart
#endif
	.quad 0


/* Disable exec-stacks, required for Gentoo ***********************************/

#if defined(__GCC__) && defined(__ELF__)
	.section .note.GNU-stack,"",@progbits
#endif


#endif /* if 0 */

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
