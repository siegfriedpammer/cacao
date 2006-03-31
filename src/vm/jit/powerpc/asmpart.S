/* src/vm/jit/powerpc/asmpart.S - Java-C interface functions for PowerPC
		
   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
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

   Contact: cacao@cacaojvm.org

   Authors: Andreas Krall
            Reinhard Grafl
            Stefan Ring

   Changes: Christian Thalinger
            Edwin Steiner

   $Id: asmpart.S 4715 2006-03-31 07:50:19Z twisti $

*/


#include "config.h"

#include "md-abi.h"
#include "md-asm.h"

#include "vm/jit/abi-asm.h"
#include "vm/jit/methodheader.h"
#include "vm/jit/powerpc/offsets.h"


	.text

	.align 2


/* exported functions and variables *******************************************/

	.globl asm_vm_call_method
	.globl asm_vm_call_method_int
	.globl asm_vm_call_method_long
	.globl asm_vm_call_method_float
	.globl asm_vm_call_method_double

	.globl asm_vm_call_method_exception_handler

	.globl asm_call_jit_compiler

	.globl asm_handle_nat_exception
	.globl asm_handle_exception

	.globl asm_wrapper_patcher

	.globl asm_replacement_out
	.globl asm_replacement_in

	.globl asm_cacheflush
	.globl asm_initialize_thread_stack
	.globl asm_perform_threadswitch
	.globl asm_switchstackandcall
	.globl asm_criticalsections
	.globl asm_getclassvalues_atomic


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

	.align 2

	.long   0                         /* catch type all                       */
	.long   0                         /* exception handler pc                 */
	.long   0                         /* end pc                               */
	.long   0                         /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* line number table start              */
	.long   0                         /* line number table size               */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   0                         /* frame size                           */
	.long   0                         /* method pointer (pointer to name)     */

asm_vm_call_method:
asm_vm_call_method_int:
asm_vm_call_method_long:
asm_vm_call_method_float:
asm_vm_call_method_double:
	mflr    r0
	stw     r0,LA_LR_OFFSET(r1)
	stwu    r1,-40*4(r1)

#if defined(__DARWIN__)
	stw     itmp1,10*4(sp)            /* register r11 is callee saved         */
#endif
	stw     pv,11*4(sp)               /* save PV register                     */

	stw     itmp3,12*4(sp)            /* registers r14-r31 are callee saved   */
	stfd	ftmp1,14*4(sp)            /* registers f14-f31 are callee saved   */
	stfd	ftmp2,16*4(sp)

#if defined(__DARWIN__)
	stw     t1,18*4(r1)
	stw     t2,19*4(r1)
	stw     t3,20*4(r1)
	stw     t4,21*4(r1)
	stw     t5,22*4(r1)
	stw     t6,23*4(r1)
	stw     t7,24*4(r1)

	stfd	ft0,26*4(r1)
	stfd	ft1,28*4(r1)
	stfd	ft2,30*4(r1)
	stfd	ft3,32*4(r1)
	stfd	ft4,34*4(r1)
	stfd	ft5,36*4(r1)
#else
	SAVE_TEMPORARY_REGISTERS(18)      /* the offset has to be even            */
#endif

	stw     a0,9*4(r1)                /* save method pointer for compiler     */

	mr      itmp1,r5                  /* pointer to arg block                 */
	mr      itmp2,r4                  /* arg count                            */

	addi    itmp1,itmp1,-sizevmarg    /* initialize pointer (smaller code)    */
	addi    itmp2,itmp2,1             /* initialize argument count            */
	li      t0,0                      /* initialize integer argument counter  */
	li      t1,0                      /* initialize float argument counter    */

	mflr    r0                        /* save link register (PIC code)        */
	bl      L_asm_vm_call_method_get_pc
L_asm_vm_call_method_get_pc:
	mflr    t3                        /* t3 contains the current pc           */
	mtlr    r0

L_register_copy:
	addi    itmp1,itmp1,sizevmarg     /* goto next argument block             */
	addi    itmp2,itmp2,-1            /* argument count - 1                   */
	mr.     itmp2,itmp2
	beq     L_register_copy_done

#if WORDS_BIGENDIAN == 1
	lwz     itmp3,offvmargtype+4(itmp1)
#else
#error XXX
#endif
	andi.   r0,itmp3,0x0002           /* is this a float/double type?         */
	bne     L_register_handle_float

	cmpwi   t0,INT_ARG_CNT            /* are we out of integer argument       */
	beq     L_register_copy           /* registers? yes, next loop            */

	andi.   r0,itmp3,0x0001           /* is this a long type?                 */
	bne     L_register_handle_long

L_register_handle_int:
#if defined(__DARWIN__)
	addis   itmp3,t3,ha16(L_jumptable_int - L_asm_vm_call_method_get_pc)
	la      itmp3,lo16(L_jumptable_int - L_asm_vm_call_method_get_pc)(itmp3)
#else
	lis     itmp3,L_jumptable_int@ha
	addi    itmp3,itmp3,L_jumptable_int@l
#endif
	slwi    t2,t0,2                   /* multiple of 4-bytes                  */
	add     itmp3,itmp3,t2            /* calculate address of jumptable       */
	lwz     itmp3,0(itmp3)            /* load function address                */
	addi    t0,t0,1                   /* integer argument counter + 1         */
	mtctr   itmp3
	bctr

L_register_handle_long:
#if defined(__DARWIN__)
	addis   itmp3,t3,ha16(L_jumptable_long - L_asm_vm_call_method_get_pc)
	la      itmp3,lo16(L_jumptable_long - L_asm_vm_call_method_get_pc)(itmp3)
#else
	lis     itmp3,L_jumptable_long@ha
	addi    itmp3,itmp3,L_jumptable_long@l
#endif
	addi    t2,t0,1                   /* align to even numbers                */
	srwi    t2,t2,1
	slwi    t2,t2,1
	slwi    t2,t2,2                   /* multiple of 4-bytes                  */
	add     itmp3,itmp3,t2            /* calculate address of jumptable       */
	lwz     itmp3,0(itmp3)            /* load function address                */
	addi    t0,t0,1                   /* integer argument counter + 1         */
	mtctr   itmp3
	bctr

L_register_handle_float:
L_register_copy_done:

L_stack_copy_done:
	lwz     itmp1,9*4(sp)             /* pass method pointer via tmp1         */

#if defined(__DARWIN__)
	addis   mptr,t3,ha16(L_asm_call_jit_compiler - L_asm_vm_call_method_get_pc)
	la      mptr,lo16(L_asm_call_jit_compiler - L_asm_vm_call_method_get_pc)(mptr)
#else
	lis     mptr,L_asm_call_jit_compiler@ha
	addi    mptr,mptr,L_asm_call_jit_compiler@l
#endif
	stw     mptr,8*4(r1)
	addi    mptr,r1,7*4

	lwz     pv,1*4(mptr)
	mtctr   pv
	bctrl
1:
	mflr    itmp1
#if defined(__DARWIN__)
	addi    pv,itmp1,lo16(asm_vm_call_method - 1b)
#else
	addi    pv,itmp1,(asm_vm_call_method - 1b)@l
#endif

L_asm_vm_call_method_return:
#if defined(__DARWIN__)
	lwz     itmp1,10*4(sp)            /* register r11 is callee saved         */
#endif
	lwz     pv,11*4(sp)               /* save PV register                     */

	lwz     itmp3,12*4(sp)
	lfd     ftmp1,14*4(sp)            /* registers f14-f31 are callee saved   */
	lfd     ftmp2,16*4(sp)

#if defined(__DARWIN__)
	lwz     t1,18*4(r1)
	lwz     t2,19*4(r1)
	lwz     t3,20*4(r1)
	lwz     t4,21*4(r1)
	lwz     t5,22*4(r1)
	lwz     t6,23*4(r1)
	lwz     t7,24*4(r1)

	lfd     ft0,26*4(r1)
	lfd     ft1,28*4(r1)
	lfd     ft2,30*4(r1)
	lfd     ft3,32*4(r1)
	lfd     ft4,34*4(r1)
	lfd     ft5,36*4(r1)
#else
	RESTORE_TEMPORARY_REGISTERS(18)   /* the offset has to be even            */
#endif

	lwz     r0,40*4+LA_LR_OFFSET(r1)
	mtlr    r0
	addi    r1,r1,40*4
	blr

asm_vm_call_method_exception_handler:
#if !defined(NDEBUG)
	mr      r3,itmp1
	bl      builtin_throw_exception
#endif
	li      v0,0                      /* return NULL                          */
	b       L_asm_vm_call_method_return


	.data
	.align  2

L_jumptable_int:
	.long   L_handle_a0
	.long   L_handle_a1
	.long   L_handle_a2
	.long   L_handle_a3
	.long   L_handle_a4
	.long   L_handle_a5
	.long   L_handle_a6
	.long   L_handle_a7

	.text
	.align  2

L_handle_a0:
	lwz     a0,offvmargdata+4(itmp1)
	b       L_register_copy
L_handle_a1:
	lwz     a1,offvmargdata+4(itmp1)
	b       L_register_copy
L_handle_a2:
	lwz     a2,offvmargdata+4(itmp1)
	b       L_register_copy
L_handle_a3:
	lwz     a3,offvmargdata+4(itmp1)
	b       L_register_copy
L_handle_a4:
	lwz     a4,offvmargdata+4(itmp1)
	b       L_register_copy
L_handle_a5:
	lwz     a5,offvmargdata+4(itmp1)
	b       L_register_copy
L_handle_a6:
	lwz     a6,offvmargdata+4(itmp1)
	b       L_register_copy
L_handle_a7:
	lwz     a7,offvmargdata+4(itmp1)
	b       L_register_copy


	.data
	.align  2

L_jumptable_long:
#if defined(__DARWIN__)
#else
	/* we have two entries here, so we get the even argument register
	alignment for linux */

	.long   L_handle_a0_a1
	.long   0
	.long   L_handle_a2_a3
	.long   0
	.long   L_handle_a4_a5
	.long   0
	.long   L_handle_a6_a7
	.long   0
#endif

	.text
	.align  2

L_handle_a0_a1:
	lwz     a0,offvmargdata+0(itmp1)
	lwz     a1,offvmargdata+4(itmp1)
	b       L_register_copy
L_handle_a2_a3:
	lwz     a2,offvmargdata+0(itmp1)
	lwz     a3,offvmargdata+4(itmp1)
	b       L_register_copy
L_handle_a4_a5:
	lwz     a4,offvmargdata+0(itmp1)
	lwz     a5,offvmargdata+4(itmp1)
	b       L_register_copy
L_handle_a6_a7:
	lwz     a6,offvmargdata+0(itmp1)
	lwz     a7,offvmargdata+4(itmp1)
	b       L_register_copy


/* asm_call_jit_compiler *******************************************************

   Invokes the compiler for untranslated JavaVM methods.

*******************************************************************************/

asm_call_jit_compiler:
L_asm_call_jit_compiler:                /* required for PIC code              */
	mflr    r0
	stw     r0,LA_LR_OFFSET(r1)         /* save return address                */
	stwu    r1,-((LA_SIZE + 5*4 + INT_ARG_CNT*4 + FLT_ARG_CNT*8 + 3*4)+sizestackframeinfo)(r1)
	stw     itmp1,(LA_SIZE + 5*4 + INT_ARG_CNT*4 + FLT_ARG_CNT*8 + 1*4)(r1)

	mr      itmp1,r0                    /* save return address to other reg.  */
	lwz     itmp3,-12(itmp1)
	srwi    itmp3,itmp3,16
	andi.   itmp3,itmp3,31
	cmpwi   itmp3,mptrn
	beq     noregchange
	lwz     itmp3,4(itmp1)
	extsh   itmp3,itmp3
	add     mptr,itmp3,itmp1
	lwz     itmp3,8(itmp1)
	srwi    itmp3,itmp3,16
	cmpwi   itmp3,0x3dad
	bne     noregchange
	lwz     itmp3,8(itmp1)
	slwi    itmp3,itmp3,16
	add     mptr,mptr,itmp3
		
noregchange:
	stw     mptr,(LA_SIZE + 5*4 + INT_ARG_CNT*4 + FLT_ARG_CNT*8 + 2*4)(r1)

#if defined(__DARWIN__)
	stw     a0,(LA_WORD_SIZE+5+0)*4(r1)
	stw     a1,(LA_WORD_SIZE+5+1)*4(r1)
	stw     a2,(LA_WORD_SIZE+5+2)*4(r1)
	stw     a3,(LA_WORD_SIZE+5+3)*4(r1)
	stw     a4,(LA_WORD_SIZE+5+4)*4(r1)
	stw     a5,(LA_WORD_SIZE+5+5)*4(r1)
	stw     a6,(LA_WORD_SIZE+5+6)*4(r1)
	stw     a7,(LA_WORD_SIZE+5+7)*4(r1)

	stfd    fa0,(LA_WORD_SIZE+5+8)*4(r1)
	stfd    fa1,(LA_WORD_SIZE+5+10)*4(r1)
	stfd    fa2,(LA_WORD_SIZE+5+12)*4(r1)
	stfd    fa3,(LA_WORD_SIZE+5+14)*4(r1)
	stfd    fa4,(LA_WORD_SIZE+5+16)*4(r1)
	stfd    fa5,(LA_WORD_SIZE+5+18)*4(r1)
	stfd    fa6,(LA_WORD_SIZE+5+20)*4(r1)
	stfd    fa7,(LA_WORD_SIZE+5+22)*4(r1)
	stfd    fa8,(LA_WORD_SIZE+5+24)*4(r1)
	stfd    fa9,(LA_WORD_SIZE+5+26)*4(r1)
	stfd    fa10,(LA_WORD_SIZE+5+28)*4(r1)
	stfd    fa11,(LA_WORD_SIZE+5+30)*4(r1)
	stfd    fa12,(LA_WORD_SIZE+5+32)*4(r1)
#else
	SAVE_ARGUMENT_REGISTERS(LA_WORD_SIZE+1)
#endif

	addi    a0,sp,(LA_SIZE + 5*4 + INT_ARG_CNT*4 + FLT_ARG_CNT*8 + 3*4)
	li      a1,0                        /* we don't have pv handy             */
	addi    a2,sp,(LA_SIZE + 5*4 + INT_ARG_CNT*4 + FLT_ARG_CNT*8 + 3*4)+sizestackframeinfo
	lwz     a3,(LA_SIZE + 5*4 + INT_ARG_CNT*4 + FLT_ARG_CNT*8 + 3*4)+sizestackframeinfo+LA_LR_OFFSET(sp)
	mr      a4,a3                       /* xpc is equal to ra                 */
	bl      stacktrace_create_extern_stackframeinfo

	lwz     a0,(LA_SIZE + 5*4 + INT_ARG_CNT*4 + FLT_ARG_CNT*8 + 1*4)(r1)
	bl      jit_compile                 /* compile the Java method            */
	mr      pv,r3                       /* move address to pv register        */

	addi    a0,sp,(LA_SIZE + 5*4 + INT_ARG_CNT*4 + FLT_ARG_CNT*8 + 3*4)
	bl      stacktrace_remove_stackframeinfo

#if defined(__DARWIN__)
	lwz     a0,(LA_WORD_SIZE+5+0)*4(r1)
	lwz     a1,(LA_WORD_SIZE+5+1)*4(r1)
	lwz     a2,(LA_WORD_SIZE+5+2)*4(r1)
	lwz     a3,(LA_WORD_SIZE+5+3)*4(r1)
	lwz     a4,(LA_WORD_SIZE+5+4)*4(r1)
	lwz     a5,(LA_WORD_SIZE+5+5)*4(r1)
	lwz     a6,(LA_WORD_SIZE+5+6)*4(r1)
	lwz     a7,(LA_WORD_SIZE+5+7)*4(r1)

	lfd     fa0,(LA_WORD_SIZE+5+8)*4(r1)
	lfd     fa1,(LA_WORD_SIZE+5+10)*4(r1)
	lfd     fa2,(LA_WORD_SIZE+5+12)*4(r1)
	lfd     fa3,(LA_WORD_SIZE+5+14)*4(r1)
	lfd     fa4,(LA_WORD_SIZE+5+16)*4(r1)
	lfd     fa5,(LA_WORD_SIZE+5+18)*4(r1)
	lfd     fa6,(LA_WORD_SIZE+5+20)*4(r1)
	lfd     fa7,(LA_WORD_SIZE+5+22)*4(r1)
	lfd     fa8,(LA_WORD_SIZE+5+24)*4(r1)
	lfd     fa9,(LA_WORD_SIZE+5+26)*4(r1)
	lfd     fa10,(LA_WORD_SIZE+5+28)*4(r1)
	lfd     fa11,(LA_WORD_SIZE+5+30)*4(r1)
	lfd     fa12,(LA_WORD_SIZE+5+32)*4(r1)
#else
	RESTORE_ARGUMENT_REGISTERS(LA_WORD_SIZE+1)
#endif

	lwz     mptr,(LA_SIZE + 5*4 + INT_ARG_CNT*4 + FLT_ARG_CNT*8 + 2*4)(r1)

	lwz     itmp1,(LA_SIZE + 5*4 + INT_ARG_CNT*4 + FLT_ARG_CNT*8 + 3*4)+sizestackframeinfo+LA_LR_OFFSET(r1)
	mtlr    itmp1
	addi    r1,r1,(LA_SIZE + 5*4 + INT_ARG_CNT*4 + FLT_ARG_CNT*8 + 3*4)+sizestackframeinfo

	mr.     pv,pv                       /* test for exception                 */
	beq     L_asm_call_jit_compiler_exception

	lwz     itmp3,-12(itmp1)
	extsh   itmp3,itmp3
	add     mptr,mptr,itmp3
	stw     pv,0(mptr)                  /* store method address               */

	mtctr   pv                          /* move method address to control reg */
	bctr                                /* and call the Java method           */

L_asm_call_jit_compiler_exception:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	mflr	r0
	stw     r0,LA_LR_OFFSET(sp)
	stwu    sp,-LA_SIZE_ALIGNED(sp)     /* preserve linkage area              */
	bl      builtin_asm_get_exceptionptrptr
	lwz     r0,LA_SIZE_ALIGNED+LA_LR_OFFSET(sp)
	mtlr    r0	
	addi    sp,sp,LA_SIZE_ALIGNED
#else
# if defined(__DARWIN__)
	lis     v0,ha16(_no_threads_exceptionptr)
	addi    v0,v0,lo16(_no_threads_exceptionptr)
# else
	lis     v0,_no_threads_exceptionptr@ha
	addi    v0,v0,_no_threads_exceptionptr@l
# endif
#endif
	lwz     xptr,0(v0)                  /* get the exception pointer          */
	li      itmp3,0
	stw     itmp3,0(v0)                 /* clear the exception pointer        */

	mflr    xpc
	addi    xpc,xpc,-4
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
	mflr    r9
	lwz     itmp3,4(r9)
	extsh   itmp3,itmp3
	add     pv,itmp3,r9
	lwz     itmp3,8(r9)
	srwi    itmp3,itmp3,16
	cmpwi   itmp3,0x3dad
	bne     L_asm_handle_exception
	lwz     itmp3,8(r9)
	slwi    itmp3,itmp3,16
	add     pv,pv,itmp3

asm_handle_exception:
L_asm_handle_exception:                 /* required for PIC code              */
	addi    sp,sp,-(ARG_CNT+TMP_CNT)*8  /* create maybe-leaf stackframe       */

#if defined(__DARWIN__)
#else
	SAVE_ARGUMENT_REGISTERS(0)          /* we save arg and temp registers in  */
	SAVE_TEMPORARY_REGISTERS(ARG_CNT)   /* case this is a leaf method         */
#endif

	li      a3,(ARG_CNT+TMP_CNT)*8      /* prepare a3 for handle_exception    */
	li      a4,1                        /* set maybe-leaf flag                */

L_asm_handle_exception_stack_loop:
	addi    sp,sp,-(LA_WORD_SIZE+4+5)*4 /* allocate stack                     */
	stw     xptr,LA_SIZE+4*4(sp)        /* save exception pointer             */
	stw     xpc,LA_SIZE+5*4(sp)         /* save exception pc                  */
	stw     pv,LA_SIZE+6*4(sp)          /* save data segment pointer          */
	mflr    r0                          /* save return address                */
	stw     r0,LA_SIZE+5*4(sp)
	add     a3,a3,sp                    /* calculate Java sp into a3...       */
	addi    a3,a3,(LA_WORD_SIZE+4+5)*4
	stw     a4,LA_SIZE+8*4(sp)          /* save maybe-leaf flag               */

	mr      a0,xptr                     /* pass exception pointer             */
	mr      a1,xpc                      /* pass exception pc                  */
	mr      a2,pv                       /* pass data segment pointer          */
	                                    /* a3 is still set                    */
	bl      exceptions_handle_exception

	mr.     v0,v0
	beq     L_asm_handle_exception_not_catched

	mr      xpc,v0                      /* move handlerpc into xpc            */
	lwz     xptr,LA_SIZE+4*4(sp)        /* restore exception pointer          */
	lwz     pv,LA_SIZE+6*4(sp)          /* restore data segment pointer       */
	lwz     r0,LA_SIZE+5*4(sp)          /* restore return address             */
	mtlr    r0
	lwz     a4,LA_SIZE+8*4(sp)          /* get maybe-leaf flag                */
	addi    sp,sp,(LA_WORD_SIZE+4+5)*4  /* free stack frame                   */

	mr.     a4,a4
	beq     L_asm_handle_exception_no_leaf

#if defined(__DARWIN__)
#else
	RESTORE_ARGUMENT_REGISTERS(0)       /* if this is a leaf method, we have  */
	RESTORE_TEMPORARY_REGISTERS(ARG_CNT)/* to restore arg and temp registers  */
#endif

	addi    sp,sp,(ARG_CNT+TMP_CNT)*8   /* remove maybe-leaf stackframe       */

L_asm_handle_exception_no_leaf:
	mtctr   xpc                         /* jump to the handler                */
	bctr

L_asm_handle_exception_not_catched:
	lwz     xptr,LA_SIZE+4*4(sp)        /* restore exception pointer          */
	lwz     pv,LA_SIZE+6*4(sp)          /* restore data segment pointer       */
	lwz     r0,LA_SIZE+5*4(sp)          /* restore return address             */
	mtlr    r0
	lwz     a4,LA_SIZE+8*4(sp)          /* get maybe-leaf flag                */
	addi    sp,sp,(LA_WORD_SIZE+4+5)*4  /* free stack frame                   */

	mr.     a4,a4
	beq     L_asm_handle_exception_no_leaf_stack

	addi    sp,sp,(ARG_CNT+TMP_CNT)*8   /* remove maybe-leaf stackframe       */
	li      a4,0                        /* clear the maybe-leaf flag          */

L_asm_handle_exception_no_leaf_stack:
	lwz     t0,FrameSize(pv)            /* get frame size                     */
	add     t0,sp,t0                    /* pointer to save area               */

	lwz     t1,IsLeaf(pv)               /* is leaf procedure                  */
	mr.     t1,t1
	bne     L_asm_handle_exception_no_ra_restore

	lwz     r0,LA_LR_OFFSET(t0)         /* restore ra                         */
	mtlr    r0

L_asm_handle_exception_no_ra_restore:
	mflr    xpc                         /* the new xpc is ra                  */
	lwz     t1,IntSave(pv)              /* t1 = saved int register count      */
	bl      ex_int1
ex_int1:
	mflr    t2                          /* t2 = current pc                    */
#if defined(__DARWIN__)
	addi    t2,t2,lo16(ex_int2-ex_int1)
#else
	addi    t2,t2,(ex_int2-ex_int1)@l
#endif
	slwi    t1,t1,2                     /* t1 = register count * 4            */
	subf    t2,t1,t2                    /* t2 = IntSave - t1                  */
	mtctr   t2
	bctr

	lwz     s0,-10*4(t0)
	lwz     s1,-9*4(t0)
	lwz     s2,-8*4(t0)
	lwz     s3,-7*4(t0)
	lwz     s4,-6*4(t0)
	lwz     s5,-5*4(t0)
	lwz     s6,-4*4(t0)
	lwz     s7,-3*4(t0)
	lwz     s8,-2*4(t0)
	lwz     s9,-1*4(t0)

ex_int2:
	subf    t0,t1,t0                    /* t0 = t0 - register count * 4       */

	lwz     t1,FltSave(pv)
	bl      ex_flt1
ex_flt1:
	mflr    t2
#if defined(__DARWIN__)
	addi    t2,t2,lo16(ex_flt2-ex_flt1)
#else
	addi    t2,t2,(ex_flt2-ex_flt1)@l
#endif
	slwi    t1,t1,2                     /* t1 = register count * 4            */
	subf    t2,t1,t2                    /* t2 = FltSave - t1                  */
	mtctr   t2
	bctr

	lfd     fs0,-10*8(t0)
	lfd     fs1,-9*8(t0)
	lfd     fs2,-8*8(t0)
	lfd     fs3,-7*8(t0)
	lfd     fs4,-6*8(t0)
	lfd     fs5,-5*8(t0)
	lfd     fs6,-4*8(t0)
	lfd     fs7,-3*8(t0)
	lfd     fs8,-2*8(t0)
	lfd     fs9,-1*8(t0)

ex_flt2:
	lwz     t0,FrameSize(pv)            /* get frame size                     */
	add     sp,sp,t0                    /* unwind stack                       */
	li      a3,0                        /* prepare a3 for handle_exception    */

	mtlr    xpc
	lwz     itmp3,4(xpc)
	extsh   itmp3,itmp3
	add     pv,itmp3,xpc
	lwz     itmp3,8(xpc)
	srwi    itmp3,itmp3,16
	cmpwi   itmp3,0x3dad
	bne     L_asm_handle_exception_stack_loop
	lwz     itmp3,8(xpc)
	slwi    itmp3,itmp3,16
	add     pv,pv,itmp3

	b       L_asm_handle_exception_stack_loop


/* asm_wrapper_patcher *********************************************************

   XXX

   Stack layout:
     20   return address into JIT code (patch position)
     16   pointer to virtual java_objectheader
     12   machine code (which is patched back later)
      8   unresolved class/method/field reference
      4   data segment displacement from load instructions
      0   patcher function pointer to call (pv is saved here afterwards)

*******************************************************************************/

asm_wrapper_patcher:
	mflr	r0                    /* get Java return address (leaf)           */
	stw     r0,6*4(sp)            /* store it in the stub stackframe          */
	                              /* keep stack 16-bytes aligned: 6+1+37 = 44 */
	stwu    sp,-(LA_SIZE+(5+58)*4+sizestackframeinfo)(sp)

#if defined(__DARWIN__)
	stw     a0,LA_SIZE+(5+0)*4(r1)      /* save argument registers            */
	stw     a1,LA_SIZE+(5+1)*4(r1)      /* preserve linkage area (24 bytes)   */
	stw     a2,LA_SIZE+(5+2)*4(r1)      /* and 4 bytes for 4 argument         */
	stw     a3,LA_SIZE+(5+3)*4(r1)
	stw     a4,LA_SIZE+(5+4)*4(r1)
	stw     a5,LA_SIZE+(5+5)*4(r1)
	stw     a6,LA_SIZE+(5+6)*4(r1)
	stw     a7,LA_SIZE+(5+7)*4(r1)

	stfd    fa0,LA_SIZE+(5+8)*4(sp)
	stfd    fa1,LA_SIZE+(5+10)*4(sp)
	stfd    fa2,LA_SIZE+(5+12)*4(sp)
	stfd    fa3,LA_SIZE+(5+14)*4(sp)
	stfd    fa4,LA_SIZE+(5+16)*4(sp)
	stfd    fa5,LA_SIZE+(5+18)*4(sp)
	stfd    fa6,LA_SIZE+(5+20)*4(sp)
	stfd    fa7,LA_SIZE+(5+22)*4(sp)
	stfd    fa8,LA_SIZE+(5+24)*4(sp)
	stfd    fa9,LA_SIZE+(5+26)*4(sp)
	stfd    fa10,LA_SIZE+(5+28)*4(sp)
	stfd    fa11,LA_SIZE+(5+30)*4(sp)
	stfd    fa12,LA_SIZE+(5+32)*4(sp)

	stw     t0,(LA_WORD_SIZE+5+33)*4(r1)
	stw     t1,(LA_WORD_SIZE+5+34)*4(r1)
	stw     t2,(LA_WORD_SIZE+5+35)*4(r1)
	stw     t3,(LA_WORD_SIZE+5+36)*4(r1)
	stw     t4,(LA_WORD_SIZE+5+37)*4(r1)
	stw     t5,(LA_WORD_SIZE+5+38)*4(r1)
	stw     t6,(LA_WORD_SIZE+5+39)*4(r1)
	stw     t7,(LA_WORD_SIZE+5+40)*4(r1)

	stfd    ft0,(LA_WORD_SIZE+5+42)*4(r1)
	stfd    ft1,(LA_WORD_SIZE+5+44)*4(r1)
	stfd    ft2,(LA_WORD_SIZE+5+46)*4(r1)
	stfd    ft3,(LA_WORD_SIZE+5+48)*4(r1)
	stfd    ft4,(LA_WORD_SIZE+5+50)*4(r1)
	stfd    ft5,(LA_WORD_SIZE+5+52)*4(r1)
#else
	SAVE_ARGUMENT_REGISTERS(LA_WORD_SIZE+1) /* save 8 int/8 float arguments   */
	SAVE_TEMPORARY_REGISTERS(LA_WORD_SIZE+1+24)
#endif

	stw     itmp1,LA_SIZE+(5+54)*4(sp)
	stw     itmp2,LA_SIZE+(5+55)*4(sp)
	stw     pv,LA_SIZE+(5+56)*4(sp)

	addi    a0,sp,LA_SIZE+(5+58)*4      /* create stackframe info             */
	mr      a1,pv
	addi    a2,sp,(8+LA_WORD_SIZE+5+58)*4+sizestackframeinfo
	mr      a3,r0                       /* this is correct for leafs          */
	lwz     a4,((5+LA_WORD_SIZE+5+58)*4+sizestackframeinfo)(sp) /* pass xpc   */
	bl      stacktrace_create_extern_stackframeinfo

	addi    a0,sp,(0+LA_WORD_SIZE+5+58)*4+sizestackframeinfo  /* pass sp      */
	lwz     pv,(0+LA_WORD_SIZE+5+58)*4+sizestackframeinfo(sp) /* get function */
	lwz     itmp1,LA_SIZE+(5+56)*4(sp)  /* move pv to position of fp          */
	stw     itmp1,(0+LA_WORD_SIZE+5+58)*4+sizestackframeinfo(sp)
	mtctr   pv                          /* call the patcher function          */
	bctrl
	stw     v0,LA_SIZE+(5+57)*4(sp)     /* save return value                  */

	addi    a0,sp,LA_SIZE+(5+58)*4
	bl      stacktrace_remove_stackframeinfo /* remove stackframe info        */

#if defined(__DARWIN__)
	lwz     a0,LA_SIZE+(5+0)*4(r1)
	lwz     a1,LA_SIZE+(5+1)*4(r1)
	lwz     a2,LA_SIZE+(5+2)*4(r1)
	lwz     a3,LA_SIZE+(5+3)*4(r1)
	lwz     a4,LA_SIZE+(5+4)*4(r1)
	lwz     a5,LA_SIZE+(5+5)*4(r1)
	lwz     a6,LA_SIZE+(5+6)*4(r1)
	lwz     a7,LA_SIZE+(5+7)*4(r1)

	lfd     fa0,LA_SIZE+(5+8)*4(sp)
	lfd     fa1,LA_SIZE+(5+10)*4(sp)
	lfd     fa2,LA_SIZE+(5+12)*4(sp)
	lfd     fa3,LA_SIZE+(5+14)*4(sp)
	lfd     fa4,LA_SIZE+(5+16)*4(sp)
	lfd     fa5,LA_SIZE+(5+18)*4(sp)
	lfd     fa6,LA_SIZE+(5+20)*4(sp)
	lfd     fa7,LA_SIZE+(5+22)*4(sp)
	lfd     fa8,LA_SIZE+(5+24)*4(sp)
	lfd     fa9,LA_SIZE+(5+26)*4(sp)
	lfd     fa10,LA_SIZE+(5+28)*4(sp)
	lfd     fa11,LA_SIZE+(5+30)*4(sp)
	lfd     fa12,LA_SIZE+(5+32)*4(sp)

	lwz     t0,(LA_WORD_SIZE+5+33)*4(r1)
	lwz     t1,(LA_WORD_SIZE+5+34)*4(r1)
	lwz     t2,(LA_WORD_SIZE+5+35)*4(r1)
	lwz     t3,(LA_WORD_SIZE+5+36)*4(r1)
	lwz     t4,(LA_WORD_SIZE+5+37)*4(r1)
	lwz     t5,(LA_WORD_SIZE+5+38)*4(r1)
	lwz     t6,(LA_WORD_SIZE+5+39)*4(r1)
	lwz     t7,(LA_WORD_SIZE+5+40)*4(r1)

	lfd     ft0,(LA_WORD_SIZE+5+42)*4(r1)
	lfd     ft1,(LA_WORD_SIZE+5+44)*4(r1)
	lfd     ft2,(LA_WORD_SIZE+5+46)*4(r1)
	lfd     ft3,(LA_WORD_SIZE+5+48)*4(r1)
	lfd     ft4,(LA_WORD_SIZE+5+50)*4(r1)
	lfd     ft5,(LA_WORD_SIZE+5+52)*4(r1)
#else
	RESTORE_ARGUMENT_REGISTERS(LA_WORD_SIZE+1) /* restore 8 int/8 float args  */
	RESTORE_TEMPORARY_REGISTERS(LA_WORD_SIZE+1+24)
#endif

	lwz     itmp1,LA_SIZE+(5+54)*4(sp)
	lwz     itmp2,LA_SIZE+(5+55)*4(sp)
	lwz     pv,LA_SIZE+(5+56)*4(sp)
	lwz     itmp3,LA_SIZE+(5+57)*4(sp)  /* restore return value into temp reg.*/

	lwz     r0,(6+LA_WORD_SIZE+5+58)*4+sizestackframeinfo(sp) /* restore RA   */
	mtlr	r0

	mr.     itmp3,itmp3           /* check for an exception                   */
	beq     L_asm_wrapper_patcher_exception

	                              /* get return address (into JIT code)       */
	lwz     itmp3,(5+LA_WORD_SIZE+5+58)*4+sizestackframeinfo(sp)

	                              /* remove stack frame + patcher stub stack  */
	addi    sp,sp,(8+LA_WORD_SIZE+5+58)*4+sizestackframeinfo

	mtctr   itmp3
	bctr                          /* jump to new patched code                 */

L_asm_wrapper_patcher_exception:
	lwz     xpc,(5+LA_WORD_SIZE+5+58)*4+sizestackframeinfo(sp)
	addi    sp,sp,(8+LA_WORD_SIZE+5+58)*4+sizestackframeinfo

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	mflr	r0
	stw     r0,LA_LR_OFFSET(sp)
	stwu    sp,-(LA_SIZE+1*4)(sp) /* preserve linkage area                    */
	stw     xpc,LA_SIZE+0*4(sp)
	bl      builtin_asm_get_exceptionptrptr
	lwz     xpc,LA_SIZE+0*4(sp)
	lwz     r0,LA_SIZE+1*4+LA_LR_OFFSET(sp)
	mtlr    r0
	addi    sp,sp,LA_SIZE+1*4
#else
# if defined(__DARWIN__)
	lis     v0,ha16(_no_threads_exceptionptr)
	addi    v0,v0,lo16(_no_threads_exceptionptr)
# else
	lis     v0,_no_threads_exceptionptr@ha
	addi    v0,v0,_no_threads_exceptionptr@l
# endif
#endif

	lwz     xptr,0(v0)            /* get the exception pointer                */
	li      itmp3,0
	stw     itmp3,0(v0)           /* clear the exception pointer              */
	b       L_asm_handle_exception


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
	stw     r0 ,( 0*8+offes_intregs)(sp)
	stw     r1 ,( 1*8+offes_intregs)(sp)
	stw     r2 ,( 2*8+offes_intregs)(sp)
	stw     r3 ,( 3*8+offes_intregs)(sp)
	stw     r4 ,( 4*8+offes_intregs)(sp)
	stw     r5 ,( 5*8+offes_intregs)(sp)
	stw     r6 ,( 6*8+offes_intregs)(sp)
	stw     r7 ,( 7*8+offes_intregs)(sp)
	stw     r8 ,( 8*8+offes_intregs)(sp)
	stw     r9 ,( 9*8+offes_intregs)(sp)
	stw     r10,(10*8+offes_intregs)(sp)
	stw     r11,(11*8+offes_intregs)(sp)
	stw     r12,(12*8+offes_intregs)(sp)
	stw     r13,(13*8+offes_intregs)(sp)
	stw     r14,(14*8+offes_intregs)(sp)
	stw     r15,(15*8+offes_intregs)(sp)
	stw     r16,(16*8+offes_intregs)(sp) /* link register */
	stw     r17,(17*8+offes_intregs)(sp)
	stw     r18,(18*8+offes_intregs)(sp)
	stw     r19,(19*8+offes_intregs)(sp)
	stw     r20,(20*8+offes_intregs)(sp)
	stw     r21,(21*8+offes_intregs)(sp)
	stw     r22,(22*8+offes_intregs)(sp)
	stw     r23,(23*8+offes_intregs)(sp)
	stw     r24,(24*8+offes_intregs)(sp)
	stw     r25,(25*8+offes_intregs)(sp)
	stw     r26,(26*8+offes_intregs)(sp)
	stw     r27,(27*8+offes_intregs)(sp)
	stw     r28,(28*8+offes_intregs)(sp)
	stw     r29,(29*8+offes_intregs)(sp)
	stw     r30,(30*8+offes_intregs)(sp)
	stw     r31,(31*8+offes_intregs)(sp)
	
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

asm_replacement_in:
	/* a0 == executionstate *es */

	/* set new sp and pv */
	lwz     sp,(offes_sp)(a0)
	lwz     pv,(offes_pv)(a0)
	
	/* copy registers from execution state */
	lwz     r0 ,( 0*8+offes_intregs)(a0)
	/* r1 is sp                       */
	/* r2 is reserved                 */
	/* a0 is loaded below             */
	lwz     r4 ,( 4*8+offes_intregs)(a0)
	lwz     r5 ,( 5*8+offes_intregs)(a0)
	lwz     r6 ,( 6*8+offes_intregs)(a0)
	lwz     r7 ,( 7*8+offes_intregs)(a0)
	lwz     r8 ,( 8*8+offes_intregs)(a0)
	lwz     r9 ,( 9*8+offes_intregs)(a0)
	lwz     r10,(10*8+offes_intregs)(a0)
	lwz     r11,(11*8+offes_intregs)(a0)
	lwz     r12,(12*8+offes_intregs)(a0)
	/* r13 is pv                      */
	lwz     r14,(14*8+offes_intregs)(a0)
	lwz     r15,(15*8+offes_intregs)(a0)
	lwz     r16,(16*8+offes_intregs)(a0) /* link register */
	lwz     r17,(17*8+offes_intregs)(a0)
	lwz     r18,(18*8+offes_intregs)(a0)
	lwz     r19,(19*8+offes_intregs)(a0)
	lwz     r20,(20*8+offes_intregs)(a0)
	lwz     r21,(21*8+offes_intregs)(a0)
	lwz     r22,(22*8+offes_intregs)(a0)
	lwz     r23,(23*8+offes_intregs)(a0)
	lwz     r24,(24*8+offes_intregs)(a0)
	lwz     r25,(25*8+offes_intregs)(a0)
	lwz     r26,(26*8+offes_intregs)(a0)
	lwz     r27,(27*8+offes_intregs)(a0)
	lwz     r28,(28*8+offes_intregs)(a0)
	lwz     r29,(29*8+offes_intregs)(a0)
	lwz     r30,(30*8+offes_intregs)(a0)
	lwz     r31,(31*8+offes_intregs)(a0)
	
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

	lwz     itmp3,offes_pc(a0)

	/* load a0 */
	
	lwz     a0,(3*8+offes_intregs)(a0)

	/* jump to new code */

	mtctr   itmp3
	bctr

/*********************************************************************/

asm_cacheflush:
	add     r4,r3,r4
	rlwinm  r3,r3,0,0,26
	addi    r4,r4,31
	rlwinm  r4,r4,0,0,26
	mr      r5,r3
1:
	cmplw   r3,r4
	bge     0f
	dcbst   0,r3
	addi    r3,r3,32
	b       1b
0:
	sync
1:
	cmplw   r5,r4
	bge     0f
	icbi    0,r5
	addi    r5,r5,32
	b       1b
0:
	sync
	isync
	blr


	.align 3
doublezero:
	.double 0.0

asm_initialize_thread_stack:
	addi r4,r4,-256
	stw r3,120(r4)
	li r3,0
	stw r3,124(r4)
	stw r3,0(r4)
	stw r3,4(r4)
	stw r3,8(r4)
	stw r3,12(r4)
	stw r3,16(r4)
	stw r3,20(r4)
	stw r3,24(r4)
	stw r3,28(r4)
	stw r3,32(r4)
	stw r3,36(r4)

	stw r3,128(r4)
	stw r3,132(r4)
	stw r3,136(r4)
	stw r3,140(r4)
	stw r3,144(r4)
	stw r3,148(r4)
	stw r3,152(r4)
	stw r3,156(r4)

	mflr r0
	bl 0f
0:
	mflr r3
	mtlr r0
#if defined(__DARWIN__)
	lfd fr0,lo16(doublezero-0b)(r3)
#else
	lfd fr0,(doublezero-0b)@l(r3)
#endif

	stfd fr0,40(r4)
	stfd fr0,48(r4)
	stfd fr0,56(r4)
	stfd fr0,64(r4)
	stfd fr0,72(r4)
	stfd fr0,80(r4)
	stfd fr0,88(r4)
	stfd fr0,96(r4)
	stfd fr0,104(r4)
	stfd fr0,112(r4)

	stfd fr0,160(r4)
	stfd fr0,168(r4)
	stfd fr0,176(r4)
	stfd fr0,184(r4)
	stfd fr0,192(r4)
	stfd fr0,200(r4)
	stfd fr0,208(r4)
	stfd fr0,216(r4)

	mr r3,r4
	blr


asm_perform_threadswitch:
	mflr r0
	addi r1,r1,-224
	stw r0,120(r1)
	stw pv,124(r1)
	stw r14,0(r1)
	stw r15,4(r1)
	stw r24,8(r1)
	stw r25,12(r1)
	stw r26,16(r1)
	stw r27,20(r1)
	stw r28,24(r1)
	stw r29,28(r1)
	stw r30,32(r1)
	stw r31,36(r1)
	stfd fr14,40(r1)
	stfd fr15,48(r1)
	stfd fr24,56(r1)
	stfd fr25,64(r1)
	stfd fr26,72(r1)
	stfd fr27,80(r1)
	stfd fr28,88(r1)
	stfd fr29,96(r1)
	stfd fr30,104(r1)
	stfd fr31,112(r1)

	stw r16,128(r1)
	stw r17,132(r1)
	stw r18,136(r1)
	stw r19,140(r1)
	stw r20,144(r1)
	stw r21,148(r1)
	stw r22,152(r1)
	stw r23,156(r1)
	stfd fr16,160(r1)
	stfd fr17,168(r1)
	stfd fr18,176(r1)
	stfd fr19,184(r1)
	stfd fr20,192(r1)
	stfd fr21,200(r1)
	stfd fr22,208(r1)
	stfd fr23,216(r1)

	stw r1,0(r3)
	stw r1,0(r5)
	lwz r1,0(r4)

	lwz r0,120(r1)
	lwz pv,124(r1)
	lwz r14,0(r1)
	lwz r15,4(r1)
	lwz r24,8(r1)
	lwz r25,12(r1)
	lwz r26,16(r1)
	lwz r27,20(r1)
	lwz r28,24(r1)
	lwz r29,28(r1)
	lwz r30,32(r1)
	lwz r31,36(r1)
	lfd fr14,40(r1)
	lfd fr15,48(r1)
	lfd fr24,56(r1)
	lfd fr25,64(r1)
	lfd fr26,72(r1)
	lfd fr27,80(r1)
	lfd fr28,88(r1)
	lfd fr29,96(r1)
	lfd fr30,104(r1)
	lfd fr31,112(r1)

	lwz r16,128(r1)
	lwz r17,132(r1)
	lwz r18,136(r1)
	lwz r19,140(r1)
	lwz r20,144(r1)
	lwz r21,148(r1)
	lwz r22,152(r1)
	lwz r23,156(r1)
	lfd fr16,160(r1)
	lfd fr17,168(r1)
	lfd fr18,176(r1)
	lfd fr19,184(r1)
	lfd fr20,192(r1)
	lfd fr21,200(r1)
	lfd fr22,208(r1)
	lfd fr23,216(r1)

	mtlr r0
	addi r1,r1,224
	blr


asm_switchstackandcall:
	mflr r0
	stwu r3,-48(r3)
	stw r0,40(r3)
	stw r1,44(r3)
	stw r1,0(r5)
	mr r1,r3

	mtctr r4
	mr r3,r6
	bctrl

	lwz r0,40(r1)
	mtlr r0
	lwz r1,44(r1)
	blr


asm_getclassvalues_atomic:
_crit_restart:
_crit_begin:
	lwz     r6,offbaseval(r3)
	lwz     r7,offdiffval(r3)
	lwz     r8,offbaseval(r4)
_crit_end:
	stw     r6,offcast_super_baseval(r5)
	stw     r7,offcast_super_diffval(r5)
	stw     r8,offcast_sub_baseval(r5)
	blr

	.data

asm_criticalsections:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	.long   _crit_begin
	.long   _crit_end
	.long   _crit_restart
#endif
	.long 0


#if defined(__DARWIN__)

.section __TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
	.align 2
L_builtin_throw_exception$stub:
	.indirect_symbol _builtin_throw_exception
	mflr r0
	bcl 20,31,L00$_builtin_throw_exception
L00$_builtin_throw_exception:
	mflr r11
	addis r11,r11,ha16(L_builtin_throw_exception$lazy_ptr - L00$_builtin_throw_exception)
	mtlr r0
	lwzu r12,lo16(L_builtin_throw_exception$lazy_ptr - L00$_builtin_throw_exception)(r11)
	mtctr r12
	bctr
.data
.lazy_symbol_pointer
L_builtin_throw_exception$lazy_ptr:
	.indirect_symbol _builtin_throw_exception
	.long dyld_stub_binding_helper


.section __TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
	.align 2
L_exceptions_handle_exception$stub:
	.indirect_symbol _exceptions_handle_exception
	mflr r0
	bcl 20,31,L00$_exceptions_handle_exception
L00$_exceptions_handle_exception:
	mflr r11
	addis r11,r11,ha16(L_exceptions_handle_exception$lazy_ptr - L00$_exceptions_handle_exception)
	mtlr r0
	lwzu r12,lo16(L_exceptions_handle_exception$lazy_ptr - L00$_exceptions_handle_exception)(r11)
	mtctr r12
	bctr
.data
.lazy_symbol_pointer
L_exceptions_handle_exception$lazy_ptr:
	.indirect_symbol _exceptions_handle_exception
	.long dyld_stub_binding_helper


.section __TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
	.align 2
L_stacktrace_create_extern_stackframeinfo$stub:
	.indirect_symbol _stacktrace_create_extern_stackframeinfo
	mflr r0
	bcl 20,31,L00$_stacktrace_create_extern_stackframeinfo
L00$_stacktrace_create_extern_stackframeinfo:
	mflr r11
	addis r11,r11,ha16(L_stacktrace_create_extern_stackframeinfo$lazy_ptr - L00$_stacktrace_create_extern_stackframeinfo)
	mtlr r0
	lwzu r12,lo16(L_stacktrace_create_extern_stackframeinfo$lazy_ptr - L00$_stacktrace_create_extern_stackframeinfo)(r11)
	mtctr r12
	bctr
.data
.lazy_symbol_pointer
L_stacktrace_create_extern_stackframeinfo$lazy_ptr:
	.indirect_symbol _stacktrace_create_extern_stackframeinfo
	.long dyld_stub_binding_helper


.section __TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
	.align 2
L_jit_compile$stub:
	.indirect_symbol _jit_compile
	mflr r0
	bcl 20,31,L00$_jit_compile
L00$_jit_compile:
	mflr r11
	addis r11,r11,ha16(L_jit_compile$lazy_ptr - L00$_jit_compile)
	mtlr r0
	lwzu r12,lo16(L_jit_compile$lazy_ptr - L00$_jit_compile)(r11)
	mtctr r12
	bctr
.data
.lazy_symbol_pointer
L_jit_compile$lazy_ptr:
	.indirect_symbol _jit_compile
	.long dyld_stub_binding_helper


.section __TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
	.align 2
L_stacktrace_remove_stackframeinfo$stub:
	.indirect_symbol _stacktrace_remove_stackframeinfo
	mflr r0
	bcl 20,31,L00$_stacktrace_remove_stackframeinfo
L00$_stacktrace_remove_stackframeinfo:
	mflr r11
	addis r11,r11,ha16(L_stacktrace_remove_stackframeinfo$lazy_ptr - L00$_stacktrace_remove_stackframeinfo)
	mtlr r0
	lwzu r12,lo16(L_stacktrace_remove_stackframeinfo$lazy_ptr - L00$_stacktrace_remove_stackframeinfo)(r11)
	mtctr r12
	bctr
.data
.lazy_symbol_pointer
L_stacktrace_remove_stackframeinfo$lazy_ptr:
	.indirect_symbol _stacktrace_remove_stackframeinfo
	.long dyld_stub_binding_helper


.section __TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
	.align 2
L_builtin_asm_get_exceptionptrptr$stub:
	.indirect_symbol _builtin_asm_get_exceptionptrptr
	mflr r0
	bcl 20,31,L00$_builtin_asm_get_exceptionptrptr
L00$_builtin_asm_get_exceptionptrptr:
	mflr r11
	addis r11,r11,ha16(L_builtin_asm_get_exceptionptrptr$lazy_ptr - L00$_builtin_asm_get_exceptionptrptr)
	mtlr r0
	lwzu r12,lo16(L_builtin_asm_get_exceptionptrptr$lazy_ptr - L00$_builtin_asm_get_exceptionptrptr)(r11)
	mtctr r12
	bctr
.data
.lazy_symbol_pointer
L_builtin_asm_get_exceptionptrptr$lazy_ptr:
	.indirect_symbol _builtin_asm_get_exceptionptrptr
	.long dyld_stub_binding_helper


.section __TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
	.align 2
L_replace_me$stub:
	.indirect_symbol _replace_me
	mflr r0
	bcl 20,31,L00$_replace_me
L00$_replace_me:
	mflr r11
	addis r11,r11,ha16(L_replace_me$lazy_ptr - L00$_replace_me)
	mtlr r0
	lwzu r12,lo16(L_replace_me$lazy_ptr - L00$_replace_me)(r11)
	mtctr r12
	bctr
.data
.lazy_symbol_pointer
L_replace_me$lazy_ptr:
	.indirect_symbol _replace_me
	.long dyld_stub_binding_helper

#endif /* defined(__DARWIN__) */


/* Disable exec-stacks, required for Gentoo ***********************************/

#if defined(__GCC__) && defined(__ELF__)
	.section .note.GNU-stack,"",@progbits
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
