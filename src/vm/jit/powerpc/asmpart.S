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

   $Id: asmpart.S 4440 2006-02-05 12:03:43Z twisti $

*/


#include "config.h"

#include "md-abi.h"
#include "md-asm.h"

#include "vm/jit/abi.h"
#include "vm/jit/methodheader.h"
#include "vm/jit/powerpc/offsets.h"


	.text

	.align 2

	.globl asm_calljavafunction
	.globl asm_calljavafunction_int

	.globl asm_calljavafunction2
	.globl asm_calljavafunction2int
	.globl asm_calljavafunction2long
	.globl asm_calljavafunction2float
	.globl asm_calljavafunction2double

	.globl asm_call_jit_compiler

	.globl asm_handle_nat_exception
	.globl asm_handle_exception

	.globl asm_wrapper_patcher

	.globl asm_cacheflush
	.globl asm_initialize_thread_stack
	.globl asm_perform_threadswitch
	.globl asm_switchstackandcall
	.globl asm_criticalsections
	.globl asm_getclassvalues_atomic


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
		
	.align 2

	.long   0                         /* catch type all                       */
	.long   calljava_xhandler         /* handler pc                           */
	.long   calljava_xhandler         /* end pc                               */
	.long   asm_calljavafunction      /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* line number table start              */
	.long   0                         /* line number table size               */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   0                         /* frame size                           */
	.long   0                         /* method pointer (pointer to name)     */

asm_calljavafunction:
asm_calljavafunction_int:
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

	stw     a0,36(r1)
	addi    itmp1,r1,36
	mr      a0,a1
	mr      a1,a2
	mr      a2,a3
	mr      a3,a4

#if defined(__DARWIN__)
	lis     mptr,ha16(asm_call_jit_compiler)
	addi    mptr,mptr,lo16(asm_call_jit_compiler)
#else
	lis     mptr,asm_call_jit_compiler@ha
	addi    mptr,mptr,asm_call_jit_compiler@l
#endif
	stw     mptr,32(r1)
	addi    mptr,r1,28

	lwz     pv,4(mptr)
	mtctr   pv
	bctrl

1:
	mflr    itmp1
#if defined(__DARWIN__)
	addi    pv,itmp1,lo16(asm_calljavafunction-1b)
#else
	addi    pv,itmp1,(asm_calljavafunction-1b)@l
#endif

L_asm_calljavafunction_return:
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

calljava_xhandler:
	mr      a0,itmp1
	bl      builtin_throw_exception
	li      v0,0                      /* return NULL                          */
	b       L_asm_calljavafunction_return




	.align 2

	.long   0                         /* catch type all                       */
	.long   calljava_xhandler2        /* handler pc                           */
	.long   calljava_xhandler2        /* end pc                               */
	.long   asm_calljavafunction2     /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* line number table start              */
	.long   0                         /* line number table size               */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   0                         /* frame size                           */
	.long   0                         /* method pointer (pointer to name)     */

asm_calljavafunction2:
asm_calljavafunction2int:
asm_calljavafunction2long:
asm_calljavafunction2float:
asm_calljavafunction2double:
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

	stw     r3,36(r1)                 /* save method pointer for compiler     */
	mr      itmp1,r6                  /* pointer to arg block                 */
	mr      itmp2,r4                  /* arg count                            */

	addi    itmp1,itmp1,-sizejniblock /* initialize pointer (smaller code)    */
	addi    itmp2,itmp2,1             /* initialize argument count            */
	li      r17,0                     /* initialize integer argument counter  */
	li      r18,0                     /* initialize float argument counter    */

L_register_copy:
	addi    itmp1,itmp1,sizejniblock  /* goto next argument block             */
	addi    itmp2,itmp2,-1            /* argument count - 1                   */
	mr.     itmp2,itmp2
	beq     L_register_copy_done

	lwz     itmp3,offjniitemtype+4(itmp1)
	andi.   r0,itmp3,0x0002           /* is this a float/double type?         */
	bne     L_register_handle_float

	cmpwi   r17,INT_ARG_CNT           /* are we out of integer argument       */
	beq     L_register_copy           /* registers? yes, next loop            */

	andi.   r0,itmp3,0x0001           /* is this a long type?                 */
	bne     L_register_handle_long

L_register_handle_int:
#if defined(__DARWIN__)
	lis     itmp3,ha16(jumptable_int)
	addi    itmp3,itmp3,lo16(jumptable_int)
#else
	lis     itmp3,jumptable_int@ha
	addi    itmp3,itmp3,jumptable_int@l
#endif
	slwi    r19,r17,2                 /* multiple of 4-bytes                  */
	add     itmp3,itmp3,r19           /* calculate address of jumptable       */
	lwz     itmp3,0(itmp3)            /* load function address                */
	addi    r17,r17,1                 /* integer argument counter + 1         */
	mtctr   itmp3
	bctr

L_register_handle_long:
#if defined(__DARWIN__)
	lis     itmp3,ha16(jumptable_long)
	addi    itmp3,itmp3,lo16(jumptable_long)
#else
	lis     itmp3,jumptable_long@ha
	addi    itmp3,itmp3,jumptable_long@l
#endif
	addi    r19,r17,1                 /* align to even numbers                */
	srwi    r19,r19,1
	slwi    r19,r19,1
	slwi    r19,r19,2                 /* multiple of 4-bytes                  */
	add     itmp3,itmp3,r19           /* calculate address of jumptable       */
	lwz     itmp3,0(itmp3)            /* load function address                */
	addi    r17,r17,1                 /* integer argument counter + 1         */
	mtctr   itmp3
	bctr

L_register_handle_float:
L_register_copy_done:

L_stack_copy_done:
	addi    itmp1,r1,36
#if defined(__DARWIN__)
	lis     mptr,ha16(asm_call_jit_compiler)
	addi    mptr,mptr,lo16(asm_call_jit_compiler)
#else
	lis     mptr,asm_call_jit_compiler@ha
	addi    mptr,mptr,asm_call_jit_compiler@l
#endif
	stw     mptr,32(r1)
	addi    mptr,r1,28

	lwz     pv,4(mptr)
	mtctr   pv
	bctrl
1:
	mflr    itmp1
#if defined(__DARWIN__)
	addi    pv,itmp1,lo16(asm_calljavafunction2-1b)
#else
	addi    pv,itmp1,(asm_calljavafunction2-1b)@l
#endif

L_asm_calljavafunction2_return:
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

calljava_xhandler2:
	mr      r3,itmp1
	bl      builtin_throw_exception
	li      v0,0                      /* return NULL                          */
	b       L_asm_calljavafunction2_return


jumptable_int:
	.long   L_handle_a0
	.long   L_handle_a1
	.long   L_handle_a2
	.long   L_handle_a3
	.long   L_handle_a4
	.long   L_handle_a5
	.long   L_handle_a6
	.long   L_handle_a7

L_handle_a0:
	lwz     a0,offjniitem+4(itmp1)
	b       L_register_copy
L_handle_a1:
	lwz     a1,offjniitem+4(itmp1)
	b       L_register_copy
L_handle_a2:
	lwz     a2,offjniitem+4(itmp1)
	b       L_register_copy
L_handle_a3:
	lwz     a3,offjniitem+4(itmp1)
	b       L_register_copy
L_handle_a4:
	lwz     a4,offjniitem+4(itmp1)
	b       L_register_copy
L_handle_a5:
	lwz     a5,offjniitem+4(itmp1)
	b       L_register_copy
L_handle_a6:
	lwz     a6,offjniitem+4(itmp1)
	b       L_register_copy
L_handle_a7:
	lwz     a7,offjniitem+4(itmp1)
	b       L_register_copy


jumptable_long:
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

L_handle_a0_a1:
	lwz     a0,offjniitem+0(itmp1)
	lwz     a1,offjniitem+4(itmp1)
	b       L_register_copy
L_handle_a2_a3:
	lwz     a2,offjniitem+0(itmp1)
	lwz     a3,offjniitem+4(itmp1)
	b       L_register_copy
L_handle_a4_a5:
	lwz     a4,offjniitem+0(itmp1)
	lwz     a5,offjniitem+4(itmp1)
	b       L_register_copy
L_handle_a6_a7:
	lwz     a6,offjniitem+0(itmp1)
	lwz     a7,offjniitem+4(itmp1)
	b       L_register_copy


/* asm_call_jit_compiler *******************************************************

   Invokes the compiler for untranslated JavaVM methods.

*******************************************************************************/

asm_call_jit_compiler:
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

	lwz     itmp1,(LA_SIZE + 5*4 + INT_ARG_CNT*4 + FLT_ARG_CNT*8 + 1*4)(r1)
	lwz     a0,0(itmp1)
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
	lwz     v0,lo16(_no_threads_exceptionptr-0b)(pv)
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
	b       asm_handle_nat_exception


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
	mflr    r9
	lwz     itmp3,4(r9)
	extsh   itmp3,itmp3
	add     pv,itmp3,r9
	lwz     itmp3,8(r9)
	srwi    itmp3,itmp3,16
	cmpwi   itmp3,0x3dad
	bne     asm_handle_exception
	lwz     itmp3,8(r9)
	slwi    itmp3,itmp3,16
	add     pv,pv,itmp3

asm_handle_exception:
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
	lwz     v0,lo16(_no_threads_exceptionptr-0b)(pv)
# else
	lis     v0,_no_threads_exceptionptr@ha
	addi    v0,v0,_no_threads_exceptionptr@l
# endif
#endif

	lwz     xptr,0(v0)            /* get the exception pointer                */
	li      itmp3,0
	stw     itmp3,0(v0)           /* clear the exception pointer              */
	b       asm_handle_exception


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
 */
