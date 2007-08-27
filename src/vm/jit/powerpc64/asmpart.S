/* src/vm/jit/powerpc64/asmpart.S - Java-C interface functions for PowerPC64
		
   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

   This file is part of CACAO.

   This program is free software.text;  you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation;  either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY	;  without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program;  if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/


#include "config.h"

#define __ASSEMBLY__

#include "md-abi.h"
#include "md-asm.h"

#include "vm/jit/abi-asm.h"
#include "vm/jit/methodheader.h"


/* export functions ***********************************************************/

	.globl asm_vm_call_method_exception_handler
	.globl asm_vm_call_method_end

	.globl asm_call_jit_compiler

	.globl asm_handle_nat_exception
	.globl asm_handle_exception

	.globl asm_abstractmethoderror

	.globl asm_patcher_wrapper

#if defined(ENABLE_REPLACEMENT)
	.globl asm_replacement_out
	.globl .asm_replacement_in
#endif

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
*    javaobject_header *asm_calljavamethod (methodinfo *m,                     *
*         void *arg1, void *arg2, void *arg3, void *arg4);                     *
*                                                                              *
*******************************************************************************/
	/* this is the method header see src/vm/jit/methodheader.h */

	.align	3

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

#ifdef ENABLE_LIBJVM
	
	.globl asm_vm_call_method
	.globl asm_vm_call_method_int
	.globl asm_vm_call_method_long
	.globl asm_vm_call_method_float
	.globl asm_vm_call_method_double
	.section ".opd","aw"
	.align 3

	asm_vm_call_method:
	asm_vm_call_method_int:
	asm_vm_call_method_long:
	asm_vm_call_method_float:
	asm_vm_call_method_double:
		.quad	.asm_vm_call_method,.TOC.@tocbase,0
		.previous
		.size asm_vm_call_method, 24
		.type .asm_vm_call_method,@function
		.globl .asm_vm_call_method
#else
	asm_vm_call_method:
	.globl asm_vm_call_method
	asm_vm_call_method_int:
	.globl asm_vm_call_method_int
	asm_vm_call_method_long:
	.globl asm_vm_call_method_long
	asm_vm_call_method_float:
	.globl asm_vm_call_method_float
	asm_vm_call_method_double:
	.globl asm_vm_call_method_double
#endif

.asm_vm_call_method:
.asm_vm_call_method_int:
.asm_vm_call_method_long:
.asm_vm_call_method_float:
.asm_vm_call_method_double:
	mflr    r0
	std     r0,LA_LR_OFFSET(sp)
	stdu    sp,-40*8(sp)
	
	std     s0,8*8(sp)                /* save used callee saved registers     */
	std     a0,9*8(sp)                /* save method pointer for compiler     */

	std     pv,11*8(sp)               /* save PV register                     */

	std     itmp3,12*8(sp)            /* registers r14-r31 are callee saved   */
	stfd	ftmp1,13*8(sp)            /* registers f14-f31 are callee saved   */
	stfd	ftmp2,14*8(sp)


	SAVE_TEMPORARY_REGISTERS(15)     
	mr	s0, r1			/* save stack pointer */

	/* a1 contains a pointer to a unit64_t structure filled with all INT_ARG_REG,
	followed by ADR_ARG_CNT and FLT_ARG_CNT, afterwards what else needs to be copied onto
	the stack 
	a2 contains the number of additional stack slots to be copied
	*/

L_register_copy:
	mr	t1, a1
	mr	t2, a2

	ld	a0 ,  0*8(t1)
	ld	a1 ,  1*8(t1)
	ld	a2 ,  2*8(t1)
	ld	a3 ,  3*8(t1)
	ld	a4 ,  4*8(t1)
	ld	a5 ,  5*8(t1)
	ld	a6 ,  6*8(t1)
	ld	a7 ,  7*8(t1)

	lfd	fa0 , 8*8(t1)
	lfd	fa1 , 9*8(t1)
	lfd	fa2 ,10*8(t1)
	lfd	fa3 ,11*8(t1)
	lfd	fa4 ,12*8(t1)
	lfd	fa5 ,13*8(t1)
	lfd	fa6 ,14*8(t1)
	lfd	fa7 ,15*8(t1)
	lfd	fa8 ,16*8(t1)
	lfd	fa9 ,17*8(t1)
	lfd	fa10,18*8(t1)
	lfd	fa11,19*8(t1)
	lfd	fa12,20*8(t1)

	mr.	t2,t2
	beq L_stack_copy_done

L_stack_copy:
	addi	t1,t1,20*8		/* before first possible stack slot arg */
	mr	t3,t2			/* argument counter */
	sldi	t2,t2,8			/* calculate size of stack */
	sub	sp,sp,t2		/* increase the stack */
	mr	t2,sp			/* t2 points to bottom of stack now */

L_stack_copy_loop:
	addi	t1,t1,8			/* next possible stack slot to copy */
	mr.	t3,t3			/* more stack slots to copy ? */
	beq	L_stack_copy_done
	ld	itmp3, 0(t1)
	std	itmp3, 0(t2)
	addi	t2,t2,8
	addi	t3,t3,-1
	b L_stack_copy_loop

L_stack_copy_done:
	mr	itmp1, s0		/* fake invokevirtual invocation */
	addi	itmp1, itmp1, 9*8	/* address of methods pv */
	ld 	pv,0*8(itmp1)
	mtctr   pv
	bctrl
1:
	mflr    itmp1
	addi    pv,itmp1,(.asm_vm_call_method - 1b)@l

L_asm_vm_call_method_return:
	mr      sp,s0                     /* restore the function's sp            */

	ld      s0,8*8(sp)                /* restore used callee saved registers  */

	ld      pv,11*8(sp)               /* save PV register                     */

	ld      itmp3,12*8(sp)
	lfd     ftmp1,13*8(sp)            /* registers f14-f31 are callee saved   */
	lfd     ftmp2,14*8(sp)

	RESTORE_TEMPORARY_REGISTERS(15) 

	ld     r0,40*8+LA_LR_OFFSET(r1)
	mtlr   r0
	addi   r1,r1,40*8
	blr

asm_vm_call_method_exception_handler:
	mr      r3,itmp1
	bl      builtin_throw_exception
	b       L_asm_vm_call_method_return

asm_vm_call_method_end:
	nop

/* asm_call_jit_compiler *******************************************************

   Invokes the compiler for untranslated JavaVM methods.

*******************************************************************************/

asm_call_jit_compiler:
L_asm_call_jit_compiler:                /* required for PIC code              */
	mflr    r0
	std     r0,LA_LR_OFFSET(sp)         /* save return address                */
	stdu    r1,-(LA_SIZE+PA_SIZE+ARG_CNT*8)(sp)

	SAVE_ARGUMENT_REGISTERS(LA_SIZE_IN_POINTERS+PA_SIZE_IN_POINTERS)

	mr      a0,itmp1
	mr      a1,mptr
	addi    a2,sp,(LA_SIZE + PA_SIZE+ ARG_CNT*8)
	ld      a3,(LA_SIZE + PA_SIZE + ARG_CNT*8)+LA_LR_OFFSET(sp)
	bl      jit_asm_compile
	ori	r0,r0,0			    /* nop needed after jump to function desc. */
	mr      pv,v0                       /* move address to pv register        */

	RESTORE_ARGUMENT_REGISTERS(LA_SIZE_IN_POINTERS+PA_SIZE_IN_POINTERS)

	ld     itmp1,(LA_SIZE + PA_SIZE + ARG_CNT*8)+LA_LR_OFFSET(sp)
	mtlr   itmp1

	addi    sp,sp,(LA_SIZE + PA_SIZE + ARG_CNT*8)

	mr.     pv,pv                       /* test for exception                 */
	beq     L_asm_call_jit_compiler_exception

	mtctr   pv                          /* move method address to control reg */
	bctr                                /* and call the Java method           */

L_asm_call_jit_compiler_exception:
	mflr	r0
	std     r0,LA_LR_OFFSET(sp)
	stdu    sp,-LA_SIZE_ALIGNED(sp)     /* preserve linkage area              */
	bl      exceptions_get_and_clear_exception
	ld      xpc,LA_SIZE_ALIGNED+LA_LR_OFFSET(sp)
	mtlr    xpc	
	addi    sp,sp,LA_SIZE_ALIGNED

	mr      xptr,v0                     /* get exception                      */
	addi    xpc,xpc,-4                  /* exception address is ra - 4        */
	b       L_asm_handle_nat_exception


/********************* function asm_handle_exception ***************************
*                                                                              *
*   This function handles an exception. It does not use the usual calling      *
*   conventions. The exception pointer is passed in REG_ITMP1 and the          *
*   pc from the exception raising position is passed in REG_ITMP2. It searches *
*   the local exception table for a handler. If no one is found, it unwinds    *
*   stacks and continues searching the callers.                                *
*                                                                              *
*   void asm_handle_exception (exceptionptr, exceptionpc);                     *
*                                                                              *
*******************************************************************************/
		
asm_handle_nat_exception:
L_asm_handle_nat_exception:             /* required for PIC code              */
L_asm_handle_exception_stack_loop:
	mflr    r0
	addi    sp,sp,-(LA_SIZE+PA_SIZE+((4+6)*8))  /* allocate stack (+4 for darwin)     */
	std     xptr,LA_SIZE+PA_SIZE+(4+0)*8(sp)    /* save exception pointer             */
	std     xpc,LA_SIZE+PA_SIZE+(4+1)*8(sp)     /* save exception pc                  */
	std     r0,LA_SIZE+PA_SIZE+(4+3)*8(sp)      /* save return address                */
	li      itmp3,0
	std     itmp3,LA_SIZE+PA_SIZE+(4+4)*8(sp)   /* save maybe-leaf flag (cleared)     */

	mr      a0,r0                       /* pass return address                */
	bl      md_codegen_get_pv_from_pc   /* get PV from RA                     */
	std     v0,LA_SIZE+PA_SIZE+(4+2)*8(sp)      /* save data segment pointer          */

	ld      a0,LA_SIZE+PA_SIZE+(4+0)*8(sp)      /* pass xptr                          */
	ld      a1,LA_SIZE+PA_SIZE+(4+1)*8(sp)      /* pass xpc                           */
	ld      a2,LA_SIZE+PA_SIZE+(4+2)*8(sp)      /* pass PV (v0 == a0)                 */
	addi    a3,sp,LA_SIZE+PA_SIZE+((4+6)*8)     /* pass Java SP                       */

	b       L_asm_handle_exception_continue


asm_handle_exception:
L_asm_handle_exception:                 /* required for PIC code              */
	addi    sp,sp,-(ARG_CNT+TMP_CNT)*8  /* create maybe-leaf stackframe       */

	SAVE_ARGUMENT_REGISTERS(0)          /* we save arg and temp registers in  */
	SAVE_TEMPORARY_REGISTERS(ARG_CNT)   /* case this is a leaf method         */

	addi    sp,sp,-(LA_SIZE+PA_SIZE+(4+6)*8)        /* allocate stack                     */
	std     xptr,LA_SIZE+PA_SIZE+(4+0)*8(sp)        /* save exception pointer             */
	std     pv,LA_SIZE+PA_SIZE+(4+2)*8(sp)          /* save data segment pointer          */
	mflr    r0           		                /* save return address                */
	std     r0,LA_SIZE+PA_SIZE+(4+3)*8(sp)		
	li      t0, 1
	std     t0, LA_SIZE+PA_SIZE+(4+4)*8(sp)         /* maybe-leaf flag */
	
	mr      a0,xptr                     /* pass exception pointer             */
	mr      a1,xpc                      /* pass exception pc                  */
	mr      a2,pv                       /* pass data segment pointer          */
	addi    a3,sp,LA_SIZE+PA_SIZE+(ARG_CNT+TMP_CNT)*8+(4+6)*8


L_asm_handle_exception_continue:
	bl      exceptions_handle_exception

	mr.     v0,v0
	beq     L_asm_handle_exception_not_catched

	mr      xpc,v0                              /* move handlerpc into xpc            */
	ld      xptr,LA_SIZE+PA_SIZE+(4+0)*8(sp)    /* restore exception pointer          */
	ld      pv,LA_SIZE+PA_SIZE+(4+2)*8(sp)      /* restore data segment pointer       */
	ld      r0,LA_SIZE+PA_SIZE+(4+3)*8(sp)      /* restore return address             */
	mtlr    r0
	ld      t0,LA_SIZE+PA_SIZE+(4+4)*8(sp)      /* get maybe-leaf flag                */
	addi    sp,sp,LA_SIZE+PA_SIZE+(4+6)*8       /* free stack frame                   */

	mr.     t0,t0
	beq     L_asm_handle_exception_no_leaf

	RESTORE_ARGUMENT_REGISTERS(0)       /* if this is a leaf method, we have  */
	RESTORE_TEMPORARY_REGISTERS(ARG_CNT)/* to restore arg and temp registers  */

	addi    sp,sp,(ARG_CNT+TMP_CNT)*8   /* remove maybe-leaf stackframe       */

L_asm_handle_exception_no_leaf:
	mtctr   xpc                         /* jump to the handler                */
	bctr

L_asm_handle_exception_not_catched:
	ld      xptr,LA_SIZE+PA_SIZE+(4+0)*8(sp)        /* restore exception pointer          */
	ld      pv,LA_SIZE+PA_SIZE+(4+2)*8(sp)          /* restore data segment pointer       */
	ld      r0,LA_SIZE+PA_SIZE+(4+3)*8(sp)          /* restore return address             */
	mtlr    r0
	ld      t0,LA_SIZE+PA_SIZE+(4+4)*8(sp)          /* get maybe-leaf flag                */
	addi    sp,sp,LA_SIZE+PA_SIZE+(4+6)*8           /* free stack frame                   */

	mr.     t0,t0
	beq     L_asm_handle_exception_no_leaf_stack

	addi    sp,sp,(ARG_CNT+TMP_CNT)*8   /* remove maybe-leaf stackframe       */
	li      t0,0                        /* clear the maybe-leaf flag          */

L_asm_handle_exception_no_leaf_stack:
	lwz     t1,FrameSize(pv)            /* get frame size                     */
	add     t1,sp,t1                    /* pointer to save area               */

	lwz     t2,IsLeaf(pv)               /* is leaf procedure                  */
	mr.     t2,t2
	bne     L_asm_handle_exception_no_ra_restore

	ld      r0,LA_LR_OFFSET(t1)         /* restore ra                         */
	mtlr    r0

L_asm_handle_exception_no_ra_restore:
	mflr    xpc                         /* the new xpc is ra                  */
	mr      t4,xpc                      /* save RA */
	lwz     t2,IntSave(pv)              /* t1 = saved int register count      */
	bl      ex_int1
ex_int1:
	mflr    t3                          /* t3 = current pc                    */
	addi    t3,t3,(ex_int2-ex_int1)@l
	slwi    t2,t2,2                     /* t2 = register count * 4            */
	subf    t3,t2,t3                    /* t3 = IntSave - t2                  */
	mtctr   t3
	bctr

	ld      s0,-9*8(t1)
	ld      s1,-8*8(t1)
	ld      s2,-7*8(t1)
	ld      s3,-6*8(t1)
	ld      s4,-5*8(t1)
	ld      s5,-4*8(t1)
	ld      s6,-3*8(t1)
	ld      s7,-2*8(t1)
	ld      s8,-1*8(t1)

ex_int2:
	subf    t1,t2,t1                    /* t1 = t1 - register count * 4       */
	lwz     t2,FltSave(pv)
	bl      ex_flt1
ex_flt1:
	mflr    t3
	addi    t3,t3,(ex_flt2-ex_flt1)@l
	slwi    t2,t2,2                     /* t2 = register count * 4            */
	subf    t3,t2,t3                    /* t3 = FltSave - t2                  */
	mtctr   t3
	bctr

	lfd     fs0,-10*8(t1)
	lfd     fs1,-9*8(t1)
	lfd     fs2,-8*8(t1)
	lfd     fs3,-7*8(t1)
	lfd     fs4,-6*8(t1)
	lfd     fs5,-5*8(t1)
	lfd     fs6,-4*8(t1)
	lfd     fs7,-3*8(t1)
	lfd     fs8,-2*8(t1)
	lfd     fs9,-1*8(t1)

ex_flt2:
	mtlr    t4                                   /* restore RA */
	lwz     t1,FrameSize(pv)                     
	add     sp,sp,t1                             /* unwind stack */
	b       L_asm_handle_exception_stack_loop


/* asm_abstractmethoderror *****************************************************

   Creates and throws an AbstractMethodError.

*******************************************************************************/

asm_abstractmethoderror:
	mflr	r0
	std     r0,LA_LR_OFFSET(sp)
	stdu    sp,-LA_SIZE_ALIGNED(sp)     /* preserve linkage area              */
	addi    a0,sp,LA_SIZE_ALIGNED       /* pass java sp                       */
	mr      a1,r0                       /* pass exception address             */
	bl      exceptions_asm_new_abstractmethoderror
	ld      r0,LA_SIZE_ALIGNED+LA_LR_OFFSET(sp)
	mtlr    r0                          /* restore return address             */
	addi    sp,sp,LA_SIZE_ALIGNED

	mr      xptr,v0                     /* get exception pointer              */
	mr      xpc,r0                      /* we can't use r0 directly in addi   */
	addi    xpc,xpc,-4                  /* exception address is ra - 4        */
	b       L_asm_handle_nat_exception


/* asm_patcher_wrapper *********************************************************

   XXX

   Stack layout:
     40   return address into JIT code (patch position)
     32   pointer to virtual java_objectheader
     24   machine code (which is patched back later)
     16   unresolved class/method/field reference
      8   data segment displacement from load instructions
      0   patcher function pointer to call (pv is saved here afterwards)

*******************************************************************************/

asm_patcher_wrapper:
	mflr	r0                    /* get Java return address (leaf)           */
	std     r0,6*8(sp)            /* store it in the stub stackframe          */
	                              /* keep stack 16-bytes aligned: 6+1+37 = 44 */
	stdu    sp,-(LA_SIZE+PA_SIZE+ARG_CNT*8+TMP_CNT*8+4*8)(sp)

	SAVE_ARGUMENT_REGISTERS(LA_SIZE_IN_POINTERS+PA_SIZE_IN_POINTERS) /* save 8 int/8 float arguments   */
	SAVE_TEMPORARY_REGISTERS(LA_SIZE_IN_POINTERS+PA_SIZE_IN_POINTERS+ARG_CNT)

	std     itmp1,LA_SIZE+PA_SIZE+(ARG_CNT+TMP_CNT)*8+1*8(sp)
	std     itmp2,LA_SIZE+PA_SIZE+(ARG_CNT+TMP_CNT)*8+2*8(sp)
	std     pv,LA_SIZE+PA_SIZE+(ARG_CNT+TMP_CNT)*8+3*8(sp)

	addi    a0,sp,LA_SIZE+PA_SIZE+ARG_CNT*8+TMP_CNT*8+4*8      /* pass SP of patcher stub            */
	mr      a1,pv                       /* pass PV                            */
	mr      a2,r0                       /* pass RA (correct for leafs)        */
	bl      patcher_wrapper
	std     v0,LA_SIZE+PA_SIZE+(ARG_CNT+TMP_CNT)*8+4*8(sp)     /* save return value                  */


	RESTORE_ARGUMENT_REGISTERS(LA_SIZE_IN_POINTERS+PA_SIZE_IN_POINTERS) /* restore 8 int/8 float args  */
	RESTORE_TEMPORARY_REGISTERS(LA_SIZE_IN_POINTERS+PA_SIZE_IN_POINTERS+ARG_CNT)

	ld     itmp1,LA_SIZE+PA_SIZE+(ARG_CNT+TMP_CNT)*8+1*8(sp)
	ld     itmp2,LA_SIZE+PA_SIZE+(ARG_CNT+TMP_CNT)*8+2*8(sp)
	ld     pv,LA_SIZE+PA_SIZE+(ARG_CNT+TMP_CNT)*8+3*8(sp)
	ld     itmp3,LA_SIZE+PA_SIZE+(ARG_CNT+TMP_CNT)*8+4*8(sp)

	ld    	r0,LA_SIZE+PA_SIZE+ARG_CNT*8+TMP_CNT*8+4*8+6*8(sp) /* restore RA                      */
	mtlr	r0

	mr.     itmp3,itmp3           /* check for an exception                   */
	bne     L_asm_patcher_wrapper_exception

	                              /* get return address (into JIT code)       */
	ld     itmp3,LA_SIZE+PA_SIZE+ARG_CNT*8+TMP_CNT*8+4*8+5*8(sp)

	                              /* remove stack frame + patcher stub stack  */
	addi    sp,sp,LA_SIZE+PA_SIZE+ARG_CNT*8+TMP_CNT*8+4*8+8*8

	mtctr   itmp3
	bctr                          /* jump to new patched code                 */

L_asm_patcher_wrapper_exception:
	mr      xptr,itmp3                  /* get exception                      */
	ld      xpc,LA_SIZE+PA_SIZE+ARG_CNT*8+TMP_CNT*8+4*8+5*8(sp)
	addi    sp,sp,LA_SIZE+PA_SIZE+ARG_CNT*8+TMP_CNT*8+4*8+8*8
	b       L_asm_handle_exception

#if defined(ENABLE_REPLACEMENT)

/* asm_replacement_out *********************************************************

   This code is jumped to from the replacement-out stubs that are executed
   when a thread reaches an activated replacement point.

   The purpose of asm_replacement_out is to read out the parts of the
   execution state that cannot be accessed from C code, store this state,
   and then call the C function replace_me.

   Stack layout:
      16                start of stack inside method to replace
      0   rplpoint *    info on the replacement point that was reached

   NOTE: itmp3 has been clobbered by the replacement-out stub!

*******************************************************************************/

/* some room to accomodate changes of the stack frame size during replacement */
	/* XXX we should find a cleaner solution here */
#define REPLACEMENT_ROOM  512

asm_replacement_out:
    /* create stack frame */
	addi    sp,sp,-(sizeexecutionstate + REPLACEMENT_ROOM) /* XXX align */

	/* save link register */
	mflr    r16

	/* save registers in execution state */
	std     r0 ,( 0*8+offes_intregs)(sp)
	std     r1 ,( 1*8+offes_intregs)(sp)
	std     r2 ,( 2*8+offes_intregs)(sp)
	std     r3 ,( 3*8+offes_intregs)(sp)
	std     r4 ,( 4*8+offes_intregs)(sp)
	std     r5 ,( 5*8+offes_intregs)(sp)
	std     r6 ,( 6*8+offes_intregs)(sp)
	std     r7 ,( 7*8+offes_intregs)(sp)
	std     r8 ,( 8*8+offes_intregs)(sp)
	std     r9 ,( 9*8+offes_intregs)(sp)
	std     r10,(10*8+offes_intregs)(sp)
	std     r11,(11*8+offes_intregs)(sp)
	std     r12,(12*8+offes_intregs)(sp)
	std     r13,(13*8+offes_intregs)(sp)
	std     r14,(14*8+offes_intregs)(sp)
	std     r15,(15*8+offes_intregs)(sp)
	std     r16,(16*8+offes_intregs)(sp) /* link register */
	std     r17,(17*8+offes_intregs)(sp)
	std     r18,(18*8+offes_intregs)(sp)
	std     r19,(19*8+offes_intregs)(sp)
	std     r20,(20*8+offes_intregs)(sp)
	std     r21,(21*8+offes_intregs)(sp)
	std     r22,(22*8+offes_intregs)(sp)
	std     r23,(23*8+offes_intregs)(sp)
	std     r24,(24*8+offes_intregs)(sp)
	std     r25,(25*8+offes_intregs)(sp)
	std     r26,(26*8+offes_intregs)(sp)
	std     r27,(27*8+offes_intregs)(sp)
	std     r28,(28*8+offes_intregs)(sp)
	std     r29,(29*8+offes_intregs)(sp)
	std     r30,(30*8+offes_intregs)(sp)
	std     r31,(31*8+offes_intregs)(sp)
	
	stfd    fr0 ,( 0*8+offes_fltregs)(sp)
	stfd    fr1 ,( 1*8+offes_fltregs)(sp)
	stfd    fr2 ,( 2*8+offes_fltregs)(sp)
	stfd    fr3 ,( 3*8+offes_fltregs)(sp)
	stfd    fr4 ,( 4*8+offes_fltregs)(sp)
	stfd    fr5 ,( 5*8+offes_fltregs)(sp)
	stfd    fr6 ,( 6*8+offes_fltregs)(sp)
	stfd    fr7 ,( 7*8+offes_fltregs)(sp)
	stfd    fr8 ,( 8*8+offes_fltregs)(sp)
	stfd    fr9 ,( 9*8+offes_fltregs)(sp)
	stfd    fr10,(10*8+offes_fltregs)(sp)
	stfd    fr11,(11*8+offes_fltregs)(sp)
	stfd    fr12,(12*8+offes_fltregs)(sp)
	stfd    fr13,(13*8+offes_fltregs)(sp)
	stfd    fr14,(14*8+offes_fltregs)(sp)
	stfd    fr15,(15*8+offes_fltregs)(sp)
	stfd    fr16,(16*8+offes_fltregs)(sp)
	stfd    fr17,(17*8+offes_fltregs)(sp)
	stfd    fr18,(18*8+offes_fltregs)(sp)
	stfd    fr19,(19*8+offes_fltregs)(sp)
	stfd    fr20,(20*8+offes_fltregs)(sp)
	stfd    fr21,(21*8+offes_fltregs)(sp)
	stfd    fr22,(22*8+offes_fltregs)(sp)
	stfd    fr23,(23*8+offes_fltregs)(sp)
	stfd    fr24,(24*8+offes_fltregs)(sp)
	stfd    fr25,(25*8+offes_fltregs)(sp)
	stfd    fr26,(26*8+offes_fltregs)(sp)
	stfd    fr27,(27*8+offes_fltregs)(sp)
	stfd    fr28,(28*8+offes_fltregs)(sp)
	stfd    fr29,(29*8+offes_fltregs)(sp)
	stfd    fr30,(30*8+offes_fltregs)(sp)
	stfd    fr31,(31*8+offes_fltregs)(sp)
	
	/* calculate sp of method */
	addi    itmp1,sp,(sizeexecutionstate + REPLACEMENT_ROOM + 4*4)
	stw     itmp1,(offes_sp)(sp)

	/* store pv */
	stw     pv,(offes_pv)(sp)

	/* call replace_me */
	lwz     a0,-(4*4)(itmp1)            /* arg0: rplpoint *                   */
	mr      a1,sp                       /* arg1: execution state              */
	addi    sp,sp,-(LA_SIZE_ALIGNED)
	b       replace_me                  /* call C function replace_me         */

/* asm_replacement_in **********************************************************

   This code writes the given execution state and jumps to the replacement
   code.

   This function never returns!

   NOTE: itmp3 is not restored!

   C prototype:
      void asm_replacement_in(executionstate *es);

*******************************************************************************/

.asm_replacement_in:
	/* a0 == executionstate *es */

	/* set new sp and pv */
	ld     sp,(offes_sp)(a0)
	ld     pv,(offes_pv)(a0)
	
	/* copy registers from execution state */
	ld     r0 ,( 0*8+offes_intregs)(a0)
	/* r1 is sp                       */
	/* r2 is reserved                 */
	/* a0 is loaded below             */
	ld     r4 ,( 4*8+offes_intregs)(a0)
	ld     r5 ,( 5*8+offes_intregs)(a0)
	ld     r6 ,( 6*8+offes_intregs)(a0)
	ld     r7 ,( 7*8+offes_intregs)(a0)
	ld     r8 ,( 8*8+offes_intregs)(a0)
	ld     r9 ,( 9*8+offes_intregs)(a0)
	ld     r10,(10*8+offes_intregs)(a0)
	ld     r11,(11*8+offes_intregs)(a0)
	ld     r12,(12*8+offes_intregs)(a0)
	/* r13 is pv                      */
	ld     r14,(14*8+offes_intregs)(a0)
	ld     r15,(15*8+offes_intregs)(a0)
	ld     r16,(16*8+offes_intregs)(a0) /* link register */
	ld     r17,(17*8+offes_intregs)(a0)
	ld     r18,(18*8+offes_intregs)(a0)
	ld     r19,(19*8+offes_intregs)(a0)
	ld     r20,(20*8+offes_intregs)(a0)
	ld     r21,(21*8+offes_intregs)(a0)
	ld     r22,(22*8+offes_intregs)(a0)
	ld     r23,(23*8+offes_intregs)(a0)
	ld     r24,(24*8+offes_intregs)(a0)
	ld     r25,(25*8+offes_intregs)(a0)
	ld     r26,(26*8+offes_intregs)(a0)
	ld     r27,(27*8+offes_intregs)(a0)
	ld     r28,(28*8+offes_intregs)(a0)
	ld     r29,(29*8+offes_intregs)(a0)
	ld     r30,(30*8+offes_intregs)(a0)
	ld     r31,(31*8+offes_intregs)(a0)
	
	lfd     fr0 ,( 0*8+offes_fltregs)(a0)
	lfd     fr1 ,( 1*8+offes_fltregs)(a0)
	lfd     fr2 ,( 2*8+offes_fltregs)(a0)
	lfd     fr3 ,( 3*8+offes_fltregs)(a0)
	lfd     fr4 ,( 4*8+offes_fltregs)(a0)
	lfd     fr5 ,( 5*8+offes_fltregs)(a0)
	lfd     fr6 ,( 6*8+offes_fltregs)(a0)
	lfd     fr7 ,( 7*8+offes_fltregs)(a0)
	lfd     fr8 ,( 8*8+offes_fltregs)(a0)
	lfd     fr9 ,( 9*8+offes_fltregs)(a0)
	lfd     fr10,(10*8+offes_fltregs)(a0)
	lfd     fr11,(11*8+offes_fltregs)(a0)
	lfd     fr12,(12*8+offes_fltregs)(a0)
	lfd     fr13,(13*8+offes_fltregs)(a0)
	lfd     fr14,(14*8+offes_fltregs)(a0)
	lfd     fr15,(15*8+offes_fltregs)(a0)
	lfd     fr16,(16*8+offes_fltregs)(a0)
	lfd     fr17,(17*8+offes_fltregs)(a0)
	lfd     fr18,(18*8+offes_fltregs)(a0)
	lfd     fr19,(19*8+offes_fltregs)(a0)
	lfd     fr20,(20*8+offes_fltregs)(a0)
	lfd     fr21,(21*8+offes_fltregs)(a0)
	lfd     fr22,(22*8+offes_fltregs)(a0)
	lfd     fr23,(23*8+offes_fltregs)(a0)
	lfd     fr24,(24*8+offes_fltregs)(a0)
	lfd     fr25,(25*8+offes_fltregs)(a0)
	lfd     fr26,(26*8+offes_fltregs)(a0)
	lfd     fr27,(27*8+offes_fltregs)(a0)
	lfd     fr28,(28*8+offes_fltregs)(a0)
	lfd     fr29,(29*8+offes_fltregs)(a0)
	lfd     fr30,(30*8+offes_fltregs)(a0)
	lfd     fr31,(31*8+offes_fltregs)(a0)

	/* restore link register */

	mtlr    r16
	
	/* load new pc */

	ld     itmp3,offes_pc(a0)

	/* load a0 */
	
	ld     a0,(3*8+offes_intregs)(a0)

	/* jump to new code */

	mtctr   itmp3
	bctr

#endif /* defined(ENABLE_REPLACEMENT) */

/* asm_cacheflush **************************************************************
	copied from linux/arch/ppc64/kernel/vdso64/cacheflush.S
	assumes 128 byte cache line size.
	All registers used may be trashed for fun and profit.
*******************************************************************************/

	.section ".opd","aw"
	.align 3
asm_cacheflush:
		.quad	.asm_cacheflush,.TOC.@tocbase,0
		.previous
		.size asm_cacheflush, 24
		.type .asm_cacheflush,@function
		.globl .asm_cacheflush 
.asm_cacheflush:
	/* construct the AND mask */
	li	r6,   0xffffffffffff8000
	ori	r6,r6,0x000000000000ff80

	add     r4,r3,r4
	and.	r3,r3,r6
	addi    r4,r4,127
	and.	r4,r4,r6
	mr      r5,r3
1:
	cmpld   r3,r4
	bge     0f
	dcbst   0,r3
	addi    r3,r3,128
	b       1b
0:
	sync
1:
	cmpld   r5,r4
	bge     0f
	icbi    0,r5
	addi    r5,r5,128
	b       1b
0:
	sync
	isync
	blr


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
