/* src/vm/jit/powerpc/asmpart.S - Java-C interface functions for PowerPC
		
   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Andreas Krall
            Reinhard Grafl
            Stefan Ring

   Changes: Christian Thalinger

   $Id: asmpart.S 2806 2005-06-23 13:51:06Z twisti $

*/


#include "config.h"
#include "md-abi.h"
#include "md-asm.h"

#include "vm/jit/powerpc/offsets.h"
#include "vm/jit/powerpc/asmoffsets.h"


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

	.globl asm_builtin_arraycheckcast
	.globl asm_builtin_aastore

	.globl asm_builtin_idiv
	.globl asm_builtin_irem
	.globl asm_builtin_ldiv
	.globl asm_builtin_lrem

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
*   C-prototype:		                                                               *
*    javaobject_header *asm_calljavamethod (methodinfo *m,                     *
*         void *arg1, void *arg2, void *arg3, void *arg4);                      *
*                                                                              *
*******************************************************************************/
		
	.align 2

	.long   0                         /* catch type all                       */
	.long   calljava_xhandler         /* handler pc                           */
	.long   calljava_xhandler         /* end pc                               */
	.long   asm_calljavafunction      /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   24                        /* frame size                           */
	.long   0                         /* method pointer (pointer to name)     */
	.long   0                         /* padding                              */

asm_calljavafunction:
asm_calljavafunction_int:
	mflr    r0
	stw     r31,-4(r1)
/*	stw     r30,-8(r1)*/
	stw     pv,-12(r1)
	stw     r0,LA_LR_OFFSET(r1)
	stwu    r1,-148(r1)

#if defined(__DARWIN__)
	bl      0f
0:
	mflr    r31
#endif

	stw     t0,40(r1)
	stw     t1,44(r1)
	stw     t2,48(r1)
	stw     t3,52(r1)
	stw     t4,56(r1)
	stw     t5,60(r1)
	stw     t6,64(r1)
	stw     t7,68(r1)

	stfd	ftmp1,72(r1)
	stfd	ftmp2,80(r1)
	stfd	ft0,88(r1)
	stfd	ft1,96(r1)
	stfd	ft2,104(r1)
	stfd	ft3,112(r1)
	stfd	ft4,120(r1)
	stfd	ft5,128(r1)

	stw     a0,36(r1)
	addi    itmp1,r1,36
	mr      a0,a1
	mr      a1,a2
	mr      a2,a3
	mr      a3,a4

#if defined(__DARWIN__)
/*	addis   mptr,r31,ha16(_asm_call_jit_compiler-0b)*/
	addi    mptr,r31,lo16(asm_call_jit_compiler-0b)
#else
/*	addi    mptr,r31,(asm_call_jit_compiler-0b)@l*/
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

calljava_regrestore:
	lwz     t0,40(r1)
	lwz     t1,44(r1)
	lwz     t2,48(r1)
	lwz     t3,52(r1)
	lwz     t4,56(r1)
	lwz     t5,60(r1)
	lwz     t6,64(r1)
	lwz     t7,68(r1)

	lfd     ftmp1,72(r1)
	lfd     ftmp2,80(r1)
	lfd     ft0,88(r1)
	lfd     ft1,96(r1)
	lfd     ft2,104(r1)
	lfd     ft3,112(r1)
	lfd     ft4,120(r1)
	lfd     ft5,128(r1)

	lwz     r0,148+LA_LR_OFFSET(r1)
	mtlr    r0
	addi    r1,r1,148
	lwz     pv,-12(r1)
/*	lwz     r30,-8(r1)*/
	lwz     r31,-4(r1)
	blr

calljava_xhandler:
	mr      r3,itmp1
	bl      builtin_throw_exception
	li      v0,0                      /* return NULL                          */
	b       calljava_regrestore




	.align 2

	.long   0                         /* catch type all                       */
	.long   calljava_xhandler2        /* handler pc                           */
	.long   calljava_xhandler2        /* end pc                               */
	.long   asm_calljavafunction2     /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   24                        /* frame size                           */
	.long   0                         /* method pointer (pointer to name)     */
	.long   0                         /* padding                              */

asm_calljavafunction2:
asm_calljavafunction2int:
asm_calljavafunction2long:
asm_calljavafunction2float:
asm_calljavafunction2double:
	mflr    r0
	stw     r31,-4(r1)
/*	stw     r30,-8(r1)*/
	stw     pv,-12(r1)
	stw     r0,LA_LR_OFFSET(r1)
	stwu    r1,-148(r1)
	bl      0f
0:
	mflr    r31

	stw     r16,40(r1)
	stw     r17,44(r1)
	stw     r18,48(r1)
	stw     r19,52(r1)
	stw     r20,56(r1)
	stw     r21,60(r1)
	stw     r22,64(r1)
	stw     r23,68(r1)
	stfd	fr16,72(r1)
	stfd	fr17,80(r1)
	stfd	fr18,88(r1)
	stfd	fr19,96(r1)
	stfd	fr20,104(r1)
	stfd	fr21,112(r1)
	stfd	fr22,120(r1)
	stfd	fr23,128(r1)

	stw     r3,36(r1)                 /* save method pointer for compiler     */
	mr      itmp1,r6                  /* pointer to arg block                 */
	mr      itmp2,r4                  /* arg count                            */

	mr.     itmp2,itmp2
	ble     calljava_argsloaded

	addi    itmp2,itmp2,-1
	lwz     r3,offjniitem+4(itmp1)
	mr.     itmp2,itmp2
	ble     calljava_argsloaded

	addi    itmp2,itmp2,-1
	lwz     r4,offjniitem+sizejniblock*1+4(itmp1)
	mr.     itmp2,itmp2
	ble     calljava_argsloaded
	addi    itmp2,itmp2,-1

	addi    itmp2,itmp2,-1
	lwz     r5,offjniitem+sizejniblock*2+4(itmp1)
	mr.     itmp2,itmp2
	ble     calljava_argsloaded
	addi    itmp2,itmp2,-1

	addi    itmp2,itmp2,-1
	lwz     r6,offjniitem+sizejniblock*3+4(itmp1)
	mr.     itmp2,itmp2
	ble     calljava_argsloaded
	addi    itmp2,itmp2,-1

calljava_argsloaded:
	addi    itmp1,r1,36
#if defined(__DARWIN__)
/*	addis   mptr,r31,ha16(_asm_call_jit_compiler-0b)*/
	addi    mptr,r31,lo16(asm_call_jit_compiler-0b)
#else
	addi    mptr,r31,(asm_call_jit_compiler-0b)@l
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

calljava_regrestore2:
	lwz     r16,40(r1)
	lwz     r17,44(r1)
	lwz     r18,48(r1)
	lwz     r19,52(r1)
	lwz     r20,56(r1)
	lwz     r21,60(r1)
	lwz     r22,64(r1)
	lwz     r23,68(r1)
	lfd     fr16,72(r1)
	lfd     fr17,80(r1)
	lfd     fr18,88(r1)
	lfd     fr19,96(r1)
	lfd     fr20,104(r1)
	lfd     fr21,112(r1)
	lfd     fr22,120(r1)
	lfd     fr23,128(r1)

	lwz     r0,148+LA_LR_OFFSET(r1)
	mtlr    r0
	addi    r1,r1,148
	lwz     pv,-12(r1)
/*	lwz     r30,-8(r1)*/
	lwz     r31,-4(r1)
	blr

calljava_xhandler2:
	mr      r3,itmp1
	bl      builtin_throw_exception
	li      v0,0                      /* return NULL                          */
	b       calljava_regrestore2


/* asm_call_jit_compiler *******************************************************

   Invokes the compiler for untranslated JavaVM methods.

*******************************************************************************/

asm_call_jit_compiler:
	stw     itmp1,-8(r1)
	mflr    itmp1
	stw     r31,-4(r1)
	stw     r29,-12(r1)
	stw     itmp1,LA_LR_OFFSET(r1)
	stwu    r1,-(LA_SIZE + 1*4 + INT_ARG_CNT*4 + FLT_ARG_CNT*8 + 3*4)(r1)
	mr      r31,pv

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
	lwz     itmp1,(LA_SIZE + 1*4 + INT_ARG_CNT*4 + FLT_ARG_CNT*8 + 1*4)(r1)
	mr      r29,mptr

#if 1
	stw     a0,28(r1)
	stw     a1,32(r1)
	stw     a2,36(r1)
	stw     a3,40(r1)
	stw     a4,44(r1)
	stw     a5,48(r1)
	stw     a6,52(r1)

	stfd    fa0,56(r1)
	stfd    fa1,64(r1)
	stfd    fa2,72(r1)
	stfd    fa3,80(r1)
	stfd    fa4,88(r1)
	stfd    fa5,96(r1)
	stfd    fa6,104(r1)
	stfd    fa7,112(r1)
	stfd    fa8,120(r1)
	stfd    fa9,128(r1)
	stfd    fa10,136(r1)
	stfd    fa11,144(r1)
	stfd    fa12,152(r1)

	stw     r10,160(r1)
#else
	SAVE_ARGUMENT_REGISTERS(LA_WORD_SIZE + 1)
#endif

	lwz     r3,0(itmp1)
	bl      jit_compile
	mr      pv,r3

	mr      mptr,r29

#if 1
	lwz     a0,28(r1)
	lwz     a1,32(r1)
	lwz     a2,36(r1)
	lwz     a3,40(r1)
	lwz     a4,44(r1)
	lwz     a5,48(r1)
	lwz     a6,52(r1)

	lfd     fa0,56(r1)
	lfd     fa1,64(r1)
	lfd     fa2,72(r1)
	lfd     fa3,80(r1)
	lfd     fa4,88(r1)
	lfd     fa5,96(r1)
	lfd     fa6,104(r1)
	lfd     fa7,112(r1)
	lfd     fa8,120(r1)
	lfd     fa9,128(r1)
	lfd     fa10,136(r1)
	lfd     fa11,144(r1)
	lfd     fa12,152(r1)

	lwz     r10,160(r1)
#else
	RESTORE_ARGUMENT_REGISTERS(LA_WORD_SIZE + 1)
#endif

	lwz     itmp1,(LA_SIZE + 1*4 + INT_ARG_CNT*4 + FLT_ARG_CNT*8 + 3*4)+LA_LR_OFFSET(r1)
	lwz     itmp3,-12(itmp1)
	extsh   itmp3,itmp3
	add     mptr,mptr,itmp3
	stw     pv,0(mptr)

	mtctr   pv

	lwz     r0,(LA_SIZE + 1*4 + INT_ARG_CNT*4 + FLT_ARG_CNT*8 + 3*4)+LA_LR_OFFSET(r1)
	mtlr    r0
	addi    r1,r1,(LA_SIZE + 1*4 + INT_ARG_CNT*4 + FLT_ARG_CNT*8 + 3*4)
	lwz     r29,-12(r1)
/*	lwz     pv,-8(r1)*/
	lwz     r31,-4(r1)
	bctr


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
	addi    r1,r1,-18*4
	stw     r0,0*4(r1)
	stw     r2,1*4(r1)
	stw     r3,2*4(r1)
	stw     r4,3*4(r1)
	stw     r5,4*4(r1)
	stw     r6,5*4(r1)
	stw     r7,6*4(r1)
	stw     r8,7*4(r1)
	stw     r9,8*4(r1)
	stw     r10,9*4(r1)
	stw     r16,10*4(r1)
	stw     r17,11*4(r1)
	stw     r18,12*4(r1)
	stw     r19,13*4(r1)
	stw     r20,14*4(r1)
	stw     r21,15*4(r1)
	stw     r22,16*4(r1)
	stw     r23,17*4(r1)

	li      r9,1
ex_stack_loop:
	addi    r1,r1,-4*4            /* allocate stack                           */
	stw     xptr,0*4(r1)          /* save used register                       */
	stw     xpc,1*4(r1)
	mflr    xptr
	stw     xptr,2*4(r1)
	stw     r9,3*4(r1)

	lwz     r3,0*4(r1)            /* exception pointer                        */
	lwz     r4,MethodPointer(pv)  /* method pointer                           */
	mr      r5,xpc                /* exception pc                             */
/*	mr      r6,r9 */
	li      r6,0                  /* line number                              */
	li      r7,4                  /* set no unwind flag                       */

	/* XXX no valid stack frame chaining here */
	addi    r1,r1,-(24+5*4)       /* 24 linkage area + 5 argument * 4         */
	bl      builtin_trace_exception
	addi    r1,r1,(24+5*4)

	lwz     xptr,2*4(r1)
	mtlr    xptr
	lwz     xptr,0*4(r1)          /* restore xptr                             */
	lwz     xpc,1*4(r1)
	lwz     r9,3*4(r1)
	addi    r1,r1,4*4

	lwz     r3,ExTableSize(pv)    /* r3 = exception table size                */
	mr.     r3,r3                 /* if empty table skip                      */
	beq     empty_table

	addi    r4,pv,ExTableStart    /* r4 = start of exception table            */

ex_table_loop:
	lwz     r5,ExStartPC(r4)      /* r5 = exception start pc                  */
	cmplw   r5,xpc                /* (startpc <= xpc)                         */
	bgt     ex_table_cont
	lwz     r5,ExEndPC(r4)        /* r5 = exception end pc                    */
	cmplw   xpc,r5                /* (xpc < endpc)                            */
	bge     ex_table_cont
	lwz     r7,ExCatchType(r4)    /* r7 = exception catch type                */
	mr.     r7,r7
	beq     ex_handle_it

	lwz     itmp3,offclassloaded(r7)
	mr.     itmp3,itmp3
	bne     L_class_loaded

	/* XXX no valid stack frame chaining here */
	addi    r1,r1,-16*4           /* allocate stack                           */
	stw     r3,7*4(r1)            /* save used registers                      */
	stw     r4,8*4(r1)            /* 6*4 (linkage) + 1*4 (arg1) + 7*4 (save)  */
	stw     r9,9*4(r1)
	stw     xptr,10*4(r1)
	stw     xpc,11*4(r1)
	mflr    xptr
	stw     xptr,12*4(r1)
	stw     r7,13*4(r1)

	mr      r3,r7                 /* arg1 = exceptionclass                    */
	bl      load_class_bootstrap

	lwz     r3,7*4(r1)
	lwz     r4,8*4(r1)
	lwz     r9,9*4(r1)
	lwz     xptr,10*4(r1)
	lwz     xpc,11*4(r1)
	lwz     itmp3,12*4(r1)
	mtlr    itmp3
	lwz     r7,13*4(r1)	
	addi    r1,r1,16*4

L_class_loaded:
	lwz     itmp3,offclasslinked(r7)
	mr.     itmp3,itmp3
	/* XXX no valid stack frame chaining here */
	addi    r1,r1,-16*4           /* allocate stack                           */
	stw     r7,13*4(r1)
	bne     L_class_linked

	stw     r3,7*4(r1)            /* save used registers                      */
	stw     r4,8*4(r1)            /* 6*4 (linkage) + 1*4 (arg1) + 7*4 (save)  */
	stw     r9,9*4(r1)
	stw     xptr,10*4(r1)
	stw     xpc,11*4(r1)
	mflr    xptr
	stw     xptr,12*4(r1)

	mr      r3,r7                 /* arg1 = exceptionclass                    */
	bl      link_class

	lwz     r3,7*4(r1)
	lwz     r4,8*4(r1)
	lwz     r9,9*4(r1)
	lwz     xptr,10*4(r1)
	lwz     xpc,11*4(r1)
	lwz     itmp3,12*4(r1)
	mtlr    itmp3

L_class_linked:
_crit_restart1:
	lwz     r7,13*4(r1)
_crit_begin1:
	lwz     r6,offobjvftbl(xptr)  /* r6 = vftblptr(xptr)                      */
	lwz     r7,offclassvftbl(r7)  /* r7 = vftblptr(catchtype) class (not obj) */
	lwz     r6,offbaseval(r6)     /* r6 = baseval(xptr)                       */
	lwz     r8,offbaseval(r7)     /* r8 = baseval(catchtype)                  */
	lwz     r7,offdiffval(r7)     /* r7 = diffval(catchtype)                  */
_crit_end1:
	subf    r6,r8,r6              /* r6 = baseval(xptr) - baseval(catchtype)  */
	cmplw   r6,r7                 /* xptr is instanceof catchtype             */
	addi    r1,r1,16*4
	bgt     ex_table_cont         /* if (false) continue                      */

ex_handle_it:
	lwz     xpc,ExHandlerPC(r4)   /* xpc = exception handler pc               */
	mr.     r9,r9
	beq     ex_jump

	lwz     r0,0*4(r1)
	lwz     r2,1*4(r1)
	lwz     r3,2*4(r1)
	lwz     r4,3*4(r1)
	lwz     r5,4*4(r1)
	lwz     r6,5*4(r1)
	lwz     r7,6*4(r1)
	lwz     r8,7*4(r1)
	lwz     r9,8*4(r1)
	lwz     r10,9*4(r1)
	lwz     r16,10*4(r1)
	lwz     r17,11*4(r1)
	lwz     r18,12*4(r1)
	lwz     r19,13*4(r1)
	lwz     r20,14*4(r1)
	lwz     r21,15*4(r1)
	lwz     r22,16*4(r1)
	lwz     r23,17*4(r1)
	addi    r1,r1,18*4

ex_jump:
	mtctr   xpc
	bctr

ex_table_cont:
	addi    r4,r4,ExEntrySize     /* next exception table entry               */
	addic.  r3,r3,-1              /* decrement entry counter                  */
	bgt     ex_table_loop         /* if (t0 > 0) next entry                   */

empty_table:
	mr.     r9,r9                 /* if here the first time, then             */
	beq     ex_already_cleared
	addi    r1,r1,18*4            /* deallocate stack and                     */
	li      r9,0                  /* clear the no unwind flag                 */
ex_already_cleared:
	lwz     r3,IsSync(pv)         /* t0 = SyncOffset                          */
	mr.     r3,r3
	beq     no_monitor_exit       /* if zero no monitorexit                   */

#if defined(USE_THREADS)
	add     r3,r1,r3
	lwz     r6,-4(r3)

	addi    r1,r1,-6*4
	stw     r3,0*4(r1)
	stw     r4,1*4(r1)
	stw     r9,2*4(r1)
	stw     xptr,3*4(r1)
	stw     xpc,4*4(r1)
	mflr    xptr
	stw     xptr,5*4(r1)

	mr      r3,r6
	/* XXX no valid stack frame chaining here */
	addi    r1,r1,-40
	bl      builtin_monitorexit
	addi    r1,r1,40

	lwz     xptr,5*4(r1)
	mtlr    xptr
	lwz     r3,0*4(r1)
	lwz     r4,1*4(r1)
	lwz     r9,2*4(r1)
	lwz     xptr,3*4(r1)
	lwz     xpc,4*4(r1)
	addi    r1,r1,6*4
#endif

no_monitor_exit:
	lwz     r3,FrameSize(pv)      /* r3 = frame size                          */
	add     r1,r1,r3              /* unwind stack                             */
	mr      r3,r1                 /* r3 = pointer to save area                */
	lwz     r4,IsLeaf(pv)         /* r4 = is leaf procedure                   */
	mr.     r4,r4
	bne     ex_no_restore         /* if (leaf) skip                           */
	lwz     r4,LA_LR_OFFSET(r3)   /* restore ra                               */
	mtlr    r4                    /* t0--                                     */
ex_no_restore:
	mflr    r4                    /* the new xpc is ra                        */
	mr      xpc,r4
	lwz     r4,IntSave(pv)        /* r4 = saved int register count            */
	bl      ex_int1
ex_int1:
	mflr    r5
#if defined(__DARWIN__)
	addi    r5,r5,lo16(ex_int2-ex_int1)
#else
	addi    r5,r5,(ex_int2-ex_int1)@l
#endif
	slwi    r4,r4,2
	subf    r5,r4,r5
	mtctr   r5
	bctr
	lwz     s0,-40(r3)
	lwz     s1,-36(r3)
	lwz     s2,-32(r3)
	lwz     s3,-28(r3)
	lwz     s4,-24(r3)
	lwz     s5,-20(r3)
	lwz     s6,-16(r3)
	lwz     s7,-12(r3)
	lwz     s8,-8(r3)
	lwz     s9,-4(r3)

ex_int2:
	subf    r3,r4,r3

	lwz     r4,FltSave(pv)
	bl      ex_flt1
ex_flt1:
	mflr    r5
#if defined(__DARWIN__)
	addi    r5,r5,lo16(ex_flt2-ex_flt1)
#else
	addi    r5,r5,(ex_flt2-ex_flt1)@l
#endif
	slwi    r4,r4,2
	subf    r5,r4,r5
	mtctr   r5
	bctr
	lfd     fs0,-80(r3)
	lfd     fs1,-72(r3)
	lfd     fs2,-64(r3)
	lfd     fs3,-56(r3)
	lfd     fs4,-48(r3)
	lfd     fs5,-40(r3)
	lfd     fs6,-32(r3)
	lfd     fs7,-24(r3)
	lfd     fs8,-16(r3)
	lfd     fs9,-8(r3)

ex_flt2:
	mtlr    xpc
	lwz     itmp3,4(xpc)
	extsh   itmp3,itmp3
	add     pv,itmp3,xpc
	lwz     itmp3,8(xpc)
	srwi    itmp3,itmp3,16
	cmpwi   itmp3,0x3dad
	bne     ex_stack_loop
	lwz     itmp3,8(xpc)
	slwi    itmp3,itmp3,16
	add     pv,pv,itmp3
	b       ex_stack_loop


/* asm_wrapper_patcher *********************************************************

   XXX

   Stack layout:
     16   return address into JIT code (patch position)
     12   pointer to virtual java_objectheader
      8   machine code (which is patched back later)
      4   unresolved class/method/field reference
      0   patcher function pointer to call

*******************************************************************************/

asm_wrapper_patcher:
	mflr	r0
	stw     r0,8*4+LA_LR_OFFSET(r1) /* skip stack frame of patcher stub       */
	stwu    r1,-((6+1+37)*4)(r1)  /* keep stack 16-bytes aligned: 6+1+37 = 44 */

#if 1
	stw     a0,(6+1+0)*4(r1)      /* save argument registers                  */
	stw     a1,(6+1+1)*4(r1)      /* preserve linkage area (24 bytes)         */
	stw     a2,(6+1+2)*4(r1)      /* and 4 bytes for 1 argument               */
	stw     a3,(6+1+3)*4(r1)
	stw     a4,(6+1+4)*4(r1)
	stw     a5,(6+1+5)*4(r1)
	stw     a6,(6+1+6)*4(r1)
	stw     a7,(6+1+7)*4(r1)

	stfd    fa0,(6+1+8)*4(sp)
	stfd    fa1,(6+1+10)*4(sp)
	stfd    fa2,(6+1+12)*4(sp)
	stfd    fa3,(6+1+14)*4(sp)
	stfd    fa4,(6+1+16)*4(sp)
	stfd    fa5,(6+1+18)*4(sp)
	stfd    fa6,(6+1+20)*4(sp)
	stfd    fa7,(6+1+22)*4(sp)
	stfd    fa8,(6+1+24)*4(sp)
	stfd    fa9,(6+1+26)*4(sp)
	stfd    fa10,(6+1+28)*4(sp)
	stfd    fa11,(6+1+30)*4(sp)
	stfd    fa12,(6+1+32)*4(sp)
#else
	SAVE_ARGUMENT_REGISTERS(6+1)  /* save 8 int/13 float argument registers   */
#endif

#if 0
	stw     r2,25*8(r1)
	stw     r16,26*8(r1)
	stw     r17,27*8(r1)
	stw     r18,28*8(r1)
	stw     r19,29*8(r1)
	stw     r20,30*8(r1)
	stw     r21,31*8(r1)
	stw     r22,32*8(r1)
	stw     r23,33*8(r1)
#endif

	stw     itmp1,(6+1+34)*4(sp)
	stw     itmp2,(6+1+35)*4(sp)
	stw     pv,(6+1+36)*4(sp)

	addi    a0,sp,(1+6+1+37)*4    /* pass sp, skip patcher function pointer   */
	lwz     pv,(0+6+1+37)*4(sp)   /* get function pointer                     */
	mtctr   pv                    /* call the patcher function                */
	bctrl
	mr      itmp3,r3              /* save return value in temp register       */

#if 1
	lwz     a0,(6+1+0)*4(r1)
	lwz     a1,(6+1+1)*4(r1)
	lwz     a2,(6+1+2)*4(r1)
	lwz     a3,(6+1+3)*4(r1)
	lwz     a4,(6+1+4)*4(r1)
	lwz     a5,(6+1+5)*4(r1)
	lwz     a6,(6+1+6)*4(r1)
	lwz     a7,(6+1+7)*4(r1)

	lfd     fa0,(6+1+8)*4(sp)
	lfd     fa1,(6+1+10)*4(sp)
	lfd     fa2,(6+1+12)*4(sp)
	lfd     fa3,(6+1+14)*4(sp)
	lfd     fa4,(6+1+16)*4(sp)
	lfd     fa5,(6+1+18)*4(sp)
	lfd     fa6,(6+1+20)*4(sp)
	lfd     fa7,(6+1+22)*4(sp)
	lfd     fa8,(6+1+24)*4(sp)
	lfd     fa9,(6+1+26)*4(sp)
	lfd     fa10,(6+1+28)*4(sp)
	lfd     fa11,(6+1+30)*4(sp)
	lfd     fa12,(6+1+32)*4(sp)
#else
	RESTORE_ARGUMENT_REGISTERS(6+1)/* restore 8 int/13 float argument reg.    */
#endif

#if 0
	lwz     r2,25*8(r1)
	lwz     r16,26*8(r1)
	lwz     r17,27*8(r1)
	lwz     r18,28*8(r1)
	lwz     r19,29*8(r1)
	lwz     r20,30*8(r1)
	lwz     r21,31*8(r1)
	lwz     r22,32*8(r1)
	lwz     r23,33*8(r1)		
#endif

	lwz     itmp1,(4+6+1+37)*4(sp)/* get return address (into JIT code)       */
	mtlr    itmp1

	lwz     itmp1,(6+1+34)*4(sp)
	lwz     itmp2,(6+1+35)*4(sp)
	lwz     pv,(6+1+36)*4(sp)

	addi    r1,r1,(8+6+1+37)*4    /* remove stack frame + patcher stub stack  */

	mr.     itmp3,itmp3           /* check for an exception                   */
	beq     L_asm_wrapper_patcher_exception

	blr                           /* jump to new patched code                 */

L_asm_wrapper_patcher_exception:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	mflr	r0
	stw     r0,LA_LR_OFFSET(r1)
	stwu    r1,-LA_SIZE_ALIGNED(r1) /* preserve linkage area                  */
	bl      builtin_asm_get_exceptionptrptr
	lwz     r0,LA_SIZE_ALIGNED+LA_LR_OFFSET(r1)
	mtlr    r0	
	addi    r1,r1,LA_SIZE_ALIGNED
#else
# if defined(__DARWIN__)
	lwz     v0,lo16(_exceptionptr-0b)(pv)
# else
	lis     v0,_exceptionptr@ha
	addi    v0,v0,_exceptionptr@l
# endif
#endif

	lwz     xptr,0(v0)            /* get the exception pointer                */
	li      itmp3,0
	stw     itmp3,0(v0)           /* clear the exception pointer              */

	mflr    xpc
	b       asm_handle_exception


/* asm_builtin_arraycheckcast **************************************************

   Does the cast check and eventually throws an exception.

*******************************************************************************/
		
asm_builtin_arraycheckcast:
	mflr    r0
	stw     r0,LA_LR_OFFSET(r1)
	stwu    r1,-48(r1)

	stw     r3,32(r1)
	bl      builtin_arraycheckcast

	lwz     r0,48+LA_LR_OFFSET(r1)
	mtlr    r0
	mr.     r3,r3
	beq     nb_carray_throw
	lwz     r3,32(r1)
	addi    r1,r1,48
	blr

nb_carray_throw:
	addi    r1,r1,48
	mflr    r0
	stw     r0,LA_LR_OFFSET(r1)
	stwu    r1,-24(r1)
	bl      new_classcastexception
	mr      xptr,r3
	addi    r1,r1,24
	lwz     r0,LA_LR_OFFSET(r1)
	mr      xpc,r0
	mtlr    r0
	b       asm_handle_nat_exception


/******************* function asm_builtin_aastore ******************************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/
	
asm_builtin_aastore:
	mr.     r3,r3
	beq     nb_aastore_null
	mflr    r0
	stw     r0,LA_LR_OFFSET(r1)
	stwu    r1,-48(r1)

	lwz     itmp1,offarraysize(r3)
	slwi    itmp3,r4,2
	add     itmp2,r3,itmp3
	cmplw   r4,itmp1
	bge     nb_aastore_bound
	mr      r4,r5
	stw     itmp2,32(r1)
	stw     r4,36(r1)
	bl      builtin_canstore

	lwz     r0,48+LA_LR_OFFSET(r1)
	mtlr    r0
	lwz     itmp1,32(r1)
	lwz     itmp2,36(r1)
	addi    r1,r1,48
	mr.     r3,r3
	beq     nb_aastore_store
	stw     itmp2,offobjarrdata(itmp1)
	blr

nb_aastore_null:
	mflr    r0
	stw     r0,LA_LR_OFFSET(r1)
	stwu    r1,-24(r1)
	bl      new_nullpointerexception
	mr      xptr,r3
	addi    r1,r1,24
	lwz     r0,LA_LR_OFFSET(r1)
	mr      xpc,r0
	mtlr    r0
	b       asm_handle_nat_exception

nb_aastore_bound:
	addi    r1,r1,48
	mflr    r0
	stw     r0,LA_LR_OFFSET(r1)
	stwu    r1,-(24+1*4)(r1)
	mr      r3,r4                 /* move index into a0                       */
	bl      new_arrayindexoutofboundsexception
	mr      xptr,r3
	addi    r1,r1,(24+1*4)
	lwz     r0,LA_LR_OFFSET(r1)
	mr      xpc,r0
	mtlr    r0
	b       asm_handle_nat_exception

nb_aastore_store:
	mflr    r0
	stw     r0,LA_LR_OFFSET(r1)
	stwu    r1,-24(r1)
	bl      new_arraystoreexception
	mr      xptr,r3
	addi    r1,r1,24
	lwz     r0,LA_LR_OFFSET(r1)
	mr      xpc,r0
	mtlr    r0
	b       asm_handle_nat_exception


asm_builtin_idiv:
	mr.     r4,r4
	beq     nb_idiv
	lis     itmp3,0x8000
	cmpw    r3,itmp3
	bne     normal_idiv
	cmpwi   r4,-1
	bne     normal_idiv
	blr

normal_idiv:
	divw    r3,r3,r4
	blr

nb_idiv:
	mflr    r0
	stw     r0,LA_LR_OFFSET(r1)
	stwu    r1,-24(r1)
	bl      new_arithmeticexception
	mr      xptr,r3
	addi    r1,r1,24
	lwz     r0,LA_LR_OFFSET(r1)
	mr      xpc,r0
	mtlr    r0
	b       asm_handle_nat_exception


asm_builtin_irem:
	mr      itmp2,r3
	mr.     r4,r4
	beq     nb_irem
	lis     itmp3,0x8000
	cmpw    itmp2,itmp3
	bne     normal_irem
	cmpwi   r4,-1
	bne     normal_irem
	li      r3,0
	beqlr

normal_irem:
	divw    itmp3,itmp2,r4
	mullw   itmp3,itmp3,r4
	subf    r3,itmp3,itmp2
	blr

nb_irem:
	mflr    r0
	stw     r0,LA_LR_OFFSET(r1)
	stwu    r1,-24(r1)
	bl      new_arithmeticexception
	mr      xptr,r3
	addi    r1,r1,24
	lwz     r0,LA_LR_OFFSET(r1)
	mr      xpc,r0
	mtlr    r0
	b       asm_handle_nat_exception


asm_builtin_ldiv:
	or.     r0,r5,r6
	beq     nb_ldiv
	b       builtin_ldiv

nb_ldiv:
	mflr    r0
	stw     r0,LA_LR_OFFSET(r1)
	stwu    r1,-24(r1)
	bl      new_arithmeticexception
	mr      xptr,r3
	addi    r1,r1,24
	lwz     r0,LA_LR_OFFSET(r1)
	mr      xpc,r0
	mtlr    r0
	b       asm_handle_nat_exception


asm_builtin_lrem:
	or.     r0,r5,r6
	beq     nb_lrem
	b       builtin_lrem

nb_lrem:
	mflr    r0
	stw     r0,LA_LR_OFFSET(r1)
	stwu    r1,-24(r1)
	bl      new_arithmeticexception
	mr      xptr,r3
	addi    r1,r1,24
	lwz     r0,LA_LR_OFFSET(r1)
	mr      xpc,r0
	mtlr    r0
	b       asm_handle_nat_exception


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
_crit_restart2:
_crit_begin2:
	lwz     r6,offbaseval(r3)
	lwz     r7,offdiffval(r3)
	lwz     r8,offbaseval(r4)
_crit_end2:
	stw     r6,offcast_super_baseval(r5)
	stw     r7,offcast_super_diffval(r5)
	stw     r8,offcast_sub_baseval(r5)
	blr

	.data

asm_criticalsections:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	.long   _crit_begin1
	.long   _crit_end1
	.long   _crit_restart1
	.long   _crit_begin2
	.long   _crit_end2
	.long   _crit_restart2
#endif
	.long 0


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
