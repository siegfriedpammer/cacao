/* src/vm/jit/mips/asmpart.S - Java-C interface functions for MIPS

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

   Changes: Christian Thalinger

   $Id: asmpart.S 4357 2006-01-22 23:33:38Z twisti $

*/


#include "config.h"

#include "vm/jit/mips/md-abi.h"
#include "vm/jit/mips/md-asm.h"
#include "vm/jit/mips/offsets.h"

#include "vm/jit/abi.h"
#include "vm/jit/methodheader.h"


	.text
	.set    noat


/* exported functions and variables *******************************************/

	.globl asm_calljavafunction
	.globl asm_calljavafunction_int

	.globl asm_calljavafunction2
	.globl asm_calljavafunction2int
	.globl asm_calljavafunction2long
	.globl asm_calljavafunction2float
	.globl asm_calljavafunction2double

	.globl asm_call_jit_compiler
	.globl asm_handle_exception
	.globl asm_handle_nat_exception

	.globl asm_wrapper_patcher

	.globl asm_perform_threadswitch
	.globl asm_initialize_thread_stack
	.globl asm_switchstackandcall
	.globl asm_getclassvalues_atomic
	.globl asm_criticalsections

	.globl compare_and_swap


/********************* function asm_calljavafunction ***************************
*                                                                              *
*   This function calls a Java-method (which possibly needs compilation)       *
*   with up to 4 address parameters.                                           *
*                                                                              *
*   This functions calls the JIT-compiler which eventually translates the      *
*   method into machine code.                                                  *
*                                                                              *
*   A possibly throwed exception will be returned to the caller as function    *
*   return value, so the java method cannot return a fucntion value (this      *
*   function usually calls 'main' and '<clinit>' which do not return a         *
*   function value).                                                           *
*                                                                              *
*   C-prototype:                                                               *
*    javaobject_header *asm_calljavafunction (methodinfo *m,                   *
*         void *arg1, void *arg2, void *arg3, void *arg4);                     *
*                                                                              *
*******************************************************************************/

	.ent    asm_calljavafunction

	.align  3

	.dword  0                           /* catch type all                     */
	.dword  calljava_xhandler           /* handler pc                         */
	.dword  calljava_xhandler           /* end pc                             */
	.dword  asm_calljavafunction        /* start pc                           */
	.word   1                           /* extable size                       */
	.word   0                           /* 4-byte ALIGNMENT PADDING           */
	.dword  0                           /* line number table start            */
	.dword  0                           /* line number table size             */
	.word   0                           /* 4-byte ALIGNMENT PADDING           */
	.word   0                           /* fltsave                            */
	.word   0                           /* intsave                            */
	.word   0                           /* isleaf                             */
	.word   0                           /* IsSync                             */
	.word   0                           /* frame size                         */
	.dword  0                           /* method pointer (pointer to name)   */

asm_calljavafunction:
asm_calljavafunction_int:
	.set    noreorder                 /* XXX we need to recompute pv          */

	aaddiu  sp,sp,-10*8               /* allocate stack space                 */
	ast     ra,0(sp)                  /* save return address                  */

	bal     call_java_pc
	ast     pv,3*8(sp)                /* procedure vector (delay slot)        */
call_java_pc:
	aaddiu  pv,ra,-4*4

	sdc1    fss0,4*8(sp)              /* save non JavaABI saved flt registers */
	sdc1    fss1,5*8(sp)
	sdc1    fss2,6*8(sp)
	sdc1    fss3,7*8(sp)
	sdc1    fss4,8*8(sp)
	sdc1    fss5,9*8(sp)

	move    itmp1,a0                  /* pass method pointer via tmp1         */

	move    a0,a1                     /* pass the remaining parameters        */
	move    a1,a2
	move    a2,a3
	move    a3,a4

	ala     mptr,asm_call_jit_compiler/* fake virtual function call (2 instr) */
	ast     mptr,1*8(sp)              /* store function address               */
	move    mptr,sp                   /* set method pointer                   */

	ald     pv,1*8(mptr)              /* method call as in Java               */
	jalr    pv                        /* call JIT compiler                    */
	nop
L_asm_calljavafunction_recompute:
/*	aaddiu  pv,ra,(asm_calljavafunction - L_asm_calljavafunction_recompute)*/
	aaddiu  pv,ra,-22*4

	.set    reorder                   /* XXX we need to recompute pv          */

calljava_return:
	ald     ra,0(sp)                  /* restore return address               */
	ald     pv,3*8(sp)                /* restore procedure vector             */

	ldc1    fss0,4*8(sp)              /* restore non JavaABI saved flt regs   */
	ldc1    fss1,5*8(sp)
	ldc1    fss2,6*8(sp)
	ldc1    fss3,7*8(sp)
	ldc1    fss4,8*8(sp)
	ldc1    fss5,9*8(sp)

	aaddiu  sp,sp,10*8                /* free stack space                     */
	j       ra                        /* return                               */

calljava_xhandler:
	move    a0,itmp1                  
	jal     builtin_throw_exception
	move    v0,zero                   /* clear return value for exception     */
	b       calljava_return

	.end    asm_calljavafunction


	.ent    asm_calljavafunction2

	.align  3

	.dword  0                           /* catch type all                     */
	.dword  calljava_xhandler2          /* handler pc                         */
	.dword  calljava_xhandler2          /* end pc                             */
	.dword  asm_calljavafunction2       /* start pc                           */
	.word   1                           /* extable size                       */
	.word   0                           /* 4-byte ALIGNMENT PADDING           */
	.dword  0                           /* line number table start            */
	.dword  0                           /* line number table size             */
	.word   0                           /* 4-byte ALIGNMENT PADDING           */
	.word   0                           /* fltsave                            */
	.word   0                           /* intsave                            */
	.word   0                           /* isleaf                             */
	.word   0                           /* IsSync                             */
	.word   0                           /* frame size                         */
	.dword  0                           /* method pointer (pointer to name)   */

asm_calljavafunction2:
asm_calljavafunction2int:
asm_calljavafunction2long:
asm_calljavafunction2float:
asm_calljavafunction2double:
	.set    noreorder                 /* XXX we need to recompute pv          */

	aaddiu  sp,sp,-12*8               /* allocate stack space (only 11 needed)*/
	ast     ra,0(sp)                  /* save return address                  */

	bal     call_java_pc2
	ast     pv,1*8(sp)                /* procedure vector                     */
call_java_pc2:
	aaddiu  pv,ra,-4*4
	ast     s7,3*8(sp)

	sdc1    fss0,5*8(sp)              /* save non JavaABI saved flt registers */
	sdc1    fss1,6*8(sp)
	sdc1    fss2,7*8(sp)
	sdc1    fss3,8*8(sp)
	sdc1    fss4,9*8(sp)
	sdc1    fss5,10*8(sp)

	ast     a0,4*8(sp)                /* save method pointer for compiler     */

	move    t0,a3
	move    s7,a1
	blez    s7,calljava_argsloaded
	nop

	ald     a0,offjniitem(t0)
	ldc1    fa0,offjniitem(t0)
	daddi   s7,s7,-1
	blez    s7,calljava_argsloaded
	nop

	ald     a1,offjniitem+sizejniblock*1(t0)
	ldc1    fa1,offjniitem+sizejniblock*1(t0)
	daddi   s7,s7,-1
	blez    s7,calljava_argsloaded
	nop

	ald     a2,offjniitem+sizejniblock*2(t0)
	ldc1    fa2,offjniitem+sizejniblock*2(t0)
	daddi   s7,s7,-1
	blez    s7,calljava_argsloaded
	nop

	ald     a3,offjniitem+sizejniblock*3(t0)
	ldc1    fa3,offjniitem+sizejniblock*3(t0)
	daddi   s7,s7,-1
	blez    s7,calljava_argsloaded
	nop

	ald     a4,offjniitem+sizejniblock*4(t0)
	ldc1    fa4,offjniitem+sizejniblock*4(t0)
	daddi   s7,s7,-1
	blez    s7,calljava_argsloaded
	nop

	ald     a5,offjniitem+sizejniblock*5(t0)
	ldc1    fa5,offjniitem+sizejniblock*5(t0)
	daddi   s7,s7,-1
	blez    s7,calljava_argsloaded
	nop

	ald     a6,offjniitem+sizejniblock*6(t0)
	ldc1    fa6,offjniitem+sizejniblock*6(t0)
	daddi   s7,s7,-1
	blez    s7,calljava_argsloaded
	nop

	ald     a7,offjniitem+sizejniblock*7(t0)
	ldc1    fa7,offjniitem+sizejniblock*7(t0)
	daddi   s7,s7,-1
		
calljava_argsloaded:
	move    t8,sp                      /* save stack pointer                  */
	blez    s7,calljava_nocopy
	nop
	subu    t1,zero,s7
	sll     t2,t1,3
	aaddu   sp,sp,t2
	aaddu   t2,t2,t8

calljava_copyloop:
    ald     t3,offjniitem+sizejniblock*8(t0)
	ast     t3,0(t2)
	ala     t1,1(t1)
	ala     t0,sizejniblock(t0)
	ala     t2,8(t2)
	bnez    t1,calljava_copyloop
	nop

calljava_nocopy:
	ald     itmp1,4*8(t8)             /* pass method pointer via itmp1        */

	ala     mptr,asm_call_jit_compiler/* fake virtual function call (2 instr) */
	ast     mptr,2*8(t8)              /* store function address               */
	ala     mptr,1*8(t8)              /* set method pointer                   */

	ald     pv,1*8(mptr)              /* method call as in Java               */
	jalr    pv                        /* call JIT compiler                    */
	nop
L_asm_calljavafunction2_recompute:
/*	aaddiu  pv,ra,(asm_calljavafunction2 - L_asm_calljavafunction2_recompute)*/
	aaddiu  pv,ra,-76*4               /* recompute procedure vector           */

	.set    reorder                   /* XXX we need to recompute pv          */

calljava_return2:
	ald     ra,0(sp)                  /* restore return address               */
	ald     pv,8(sp)                  /* restore procedure vector             */
	ald     s7,3*8(sp)

	ldc1    fss0,5*8(sp)              /* restore non JavaABI saved flt regs   */
	ldc1    fss1,6*8(sp)
	ldc1    fss2,7*8(sp)
	ldc1    fss3,8*8(sp)
	ldc1    fss4,9*8(sp)
	ldc1    fss5,10*8(sp)

	aaddiu  sp,sp,12*8                /* free stack space                     */
	j       ra                        /* return                               */

calljava_xhandler2:
	asll    s7,s7,3
	aaddu   sp,s7,sp
	move    a0,itmp1                  
	jal     builtin_throw_exception
	b       calljava_return2

	.end    asm_calljavafunction2


/****************** function asm_call_jit_compiler *****************************
*                                                                              *
*   invokes the compiler for untranslated JavaVM methods.                      *
*                                                                              *
*   Register REG_ITEMP1 contains a pointer to the method info structure        *
*   (prepared by createcompilerstub). Using the return address in R31 and the  *
*   offset in the LDA instruction or using the value in methodptr R25 the      *
*   patching address for storing the method address can be computed:           *
*                                                                              *
*   method address was either loaded using                                     *
*   M_ALD (REG_PV, REG_PV, a)        ; invokestatic/special    ($28)           *
*   M_JSR (REG_RA, REG_PV);                                                    *
*   M_NOP                                                                      *
*   M_LDA (REG_PV, REG_RA, val)                                                *
*   or                                                                         *
*   M_ALD (REG_PV, REG_METHODPTR, m) ; invokevirtual/interface ($25)           *
*   M_JSR (REG_RA, REG_PV);                                                    *
*   M_NOP                                                                      *
*   in the static case the method pointer can be computed using the            *
*   return address and the lda function following the jmp instruction          *
*                                                                              *
*******************************************************************************/


	.ent    asm_call_jit_compiler

asm_call_jit_compiler:
	aaddiu  sp,sp,-(20*8+sizestackframeinfo) /* allocate stack space          */

	SAVE_ARGUMENT_REGISTERS(0)

	ast     mptr,16*8(sp)         /* save method pointer                      */
	ast     ra,17*8(sp)           /* save return address                      */
	ast     itmp1,18*8(sp)        /* save methodinfo pointer                  */

	aaddiu  a0,sp,20*8            /* create stackframe info                   */
	move    a1,zero               /* we don't have pv handy                   */
	aaddiu  a2,sp,(20*8+sizestackframeinfo) /* pass java sp                   */
	ald     a3,17*8(sp)           /* pass java ra                             */
	move    a4,a3                 /* xpc is equal to ra                       */
	jal     stacktrace_create_extern_stackframeinfo

	ald     a0,18*8(sp)           /* pass methodinfo pointer                  */
	jal     jit_compile           /* jit compiler                             */
	ast     v0,18*8(sp)           /* save return value                        */

	aaddiu  a0,sp,20*8            /* remove stackframe info                   */
	jal     stacktrace_remove_stackframeinfo

	ald     a0,17*8(sp)           /* pass return address                      */
	aaddiu  a1,sp,20*8            /* pass stackframeinfo (for PV)             */
	ald     a2,16*8(sp)           /* pass method pointer                      */
	jal     md_assembler_get_patch_address /* get address of patch position   */
	move    t0,v0                 /* move offset to t0 for later use          */

	RESTORE_ARGUMENT_REGISTERS(0)

	ald     ra,17*8(sp)           /* restore return address                   */
	ald     v0,18*8(sp)           /* restore return value                     */
	aaddiu  sp,sp,20*8+sizestackframeinfo /* deallocate stack area            */

	beqz    v0,L_asm_call_jit_compiler_exception

	ast     v0,0(t0)              /* store new method address                 */
	move    pv,v0                 /* move method address into pv              */
	jr      pv                    /* and call method. The method returns      */
	                              /* directly to the caller (ra).             */

L_asm_call_jit_compiler_exception:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	aaddiu  sp,sp,-2*8
	ast     ra,0*8(sp)
	jal     builtin_asm_get_exceptionptrptr
	ald     ra,0*8(sp)
	aaddiu  sp,sp,2*8
#else
	la      v0,_exceptionptr
#endif
	ald     xptr,0(v0)            /* get the exception pointer                */
	ast     zero,0(v0)            /* clear the exception pointer              */

	aaddiu  xpc,ra,-4             /* faulting address is return adress - 4    */
	b       asm_handle_nat_exception

	.end    asm_call_jit_compiler


/* asm_handle_exception ********************************************************

   This function handles an exception. It does not use the usual calling
   conventions. The exception pointer is passed in REG_ITMP1 and the
   pc from the exception raising position is passed in REG_ITMP2. It searches
   the local exception table for a handler. If no one is found, it unwinds
   stacks and continues searching the callers.

*******************************************************************************/

	.ent    asm_handle_nat_exception

asm_handle_nat_exception:
L_asm_handle_exception_stack_loop:
	aaddiu  sp,sp,-6*8                  /* allocate stack                     */
	ast     xptr,0*8(sp)                /* save exception pointer             */
	ast     xpc,1*8(sp)                 /* save exception pc                  */
	ast     ra,3*8(sp)                  /* save return address                */
	ast     zero,4*8(sp)                /* save maybe-leaf flag (cleared)     */

	move    a0,ra                       /* pass return address                */
	jal     md_codegen_findmethod       /* get PV from RA                     */
	ast     v0,2*8(sp)                  /* save data segment pointer          */

	ald     a0,0*8(sp)                  /* pass exception pointer             */
	ald     a1,1*8(sp)                  /* pass exception pc                  */
	move    a2,v0                       /* pass data segment pointer          */
	aaddiu  a3,sp,6*8                   /* pass Java stack pointer            */

	b       L_asm_handle_exception_continue

	.aent    asm_handle_exception

asm_handle_exception:
	aaddiu  sp,sp,-(ARG_CNT+TMP_CNT)*8  /* create maybe-leaf stackframe       */

	SAVE_ARGUMENT_REGISTERS(0)          /* we save arg and temp registers in  */
	SAVE_TEMPORARY_REGISTERS(ARG_CNT)   /* case this is a leaf method         */

	aaddiu  sp,sp,-6*8                  /* allocate stack                     */
	ast     xptr,0*8(sp)                /* save exception pointer             */
	ast     xpc,1*8(sp)                 /* save exception pc                  */
	ast     pv,2*8(sp)                  /* save data segment pointer          */
	ast     ra,3*8(sp)                  /* save return address                */
	addu    t0,zero,1                   /* set maybe-leaf flag                */
	ast     t0,4*8(sp)                  /* save maybe-leaf flag               */

	move    a0,xptr                     /* pass exception pointer             */
	move    a1,xpc                      /* pass exception pc                  */
	move    a2,pv                       /* pass data segment pointer          */
	aaddiu  a3,sp,(ARG_CNT+TMP_CNT+6)*8 /* pass Java stack pointer            */

L_asm_handle_exception_continue:
	jal     exceptions_handle_exception
	
	beqz    v0,L_asm_handle_exception_not_catched

	move    xpc,v0                      /* move handlerpc into xpc            */
	ald     xptr,0*8(sp)                /* restore exception pointer          */
	ald     pv,2*8(sp)                  /* restore data segment pointer       */
	ald     ra,3*8(sp)                  /* restore return address             */
	ald     t0,4*8(sp)                  /* get maybe-leaf flag                */
	aaddiu  sp,sp,6*8                   /* free stackframe                    */
	
	beqz    t0,L_asm_handle_exception_no_leaf

	RESTORE_ARGUMENT_REGISTERS(0)       /* if this is a leaf method, we have  */
	RESTORE_TEMPORARY_REGISTERS(ARG_CNT)/* to restore arg and temp registers  */
	
	aaddiu  sp,sp,(ARG_CNT+TMP_CNT)*8   /* remove maybe-leaf stackframe       */

L_asm_handle_exception_no_leaf:
	jr      xpc                         /* jump to the handler                */

L_asm_handle_exception_not_catched:
	ald     xptr,0*8(sp)                /* restore exception pointer          */
	ald     pv,2*8(sp)                  /* restore data segment pointer       */
	ald     ra,3*8(sp)                  /* restore return address             */
	ald     t0,4*8(sp)                  /* get maybe-leaf flag                */
	aaddiu  sp,sp,6*8                   /* free stackframe                    */
	
	beqz    t0,L_asm_handle_exception_no_leaf_stack

	aaddiu  sp,sp,(ARG_CNT+TMP_CNT)*8   /* remove maybe-leaf stackframe       */
	move    t0,zero                     /* clear the maybe-leaf flag          */

L_asm_handle_exception_no_leaf_stack:
	lw      t1,FrameSize(pv)            /* get frame size                     */
	aaddu   t1,sp,t1                    /* pointer to save area               */

	lw      t2,IsLeaf(pv)               /* is leaf procedure                  */
	bnez    t2,L_asm_handle_exception_no_ra_restore

	ald     ra,-1*8(t1)                 /* restore ra                         */
	aaddiu  t1,t1,-8                    /* t1--                               */

L_asm_handle_exception_no_ra_restore:
	move    xpc,ra                      /* the new xpc is ra                  */
	lw      t2,IntSave(pv)              /* t1 = saved int register count      */
	ala     t3,ex_int2                  /* t3 = current pc                    */
	sll     t2,t2,2                     /* t2 = register count * 4            */
	asubu   t3,t3,t2                    /* t3 = IntSave - 4 * register count  */
	jr      t3                          /* jump to save position              */

	ald     s0,-8*8(t1)
	ald     s1,-7*8(t1)
	ald     s2,-6*8(t1)
	ald     s3,-5*8(t1)
	ald     s4,-4*8(t1)
	ald     s5,-3*8(t1)
	ald     s6,-2*8(t1)
	ald     s7,-1*8(t1)
ex_int2:
	sll     t2,t2,1               /* t2 = register count * 4 * 2              */
	asubu   t1,t1,t2              /* t1 = t0 - 8 * register count             */

	lw      t2,FltSave(pv)        /* t2 = saved flt register count            */
	ala     t3,ex_flt2            /* t3 = current pc                          */
	sll     t2,t2,2               /* t2 = register count * 4                  */
	asubu   t3,t3,t2              /* t3 = ex_int_sav - 4 * register count     */
	jr      t3                          /* jump to save position              */

	ldc1    fs0,-4*8(t1)
	ldc1    fs1,-3*8(t1)
	ldc1    fs2,-2*8(t1)
	ldc1    fs3,-1*8(t1)

ex_flt2:
	lw      t1,FrameSize(pv)            /* get frame size                     */
	aaddu   sp,sp,t1                    /* unwind stack                       */
	b       L_asm_handle_exception_stack_loop

	.end    asm_handle_nat_exception


/* asm_wrapper_patcher *********************************************************

   XXX

   Stack layout:
     40   return address into JIT code (patch position)
     32   pointer to virtual java_objectheader
     24   machine code (which is patched back later)
     16   unresolved class/method/field reference
      8   data segment displacement from load instructions
      0   patcher function pointer to call

*******************************************************************************/
		
    .ent    asm_wrapper_patcher

asm_wrapper_patcher:
	aaddiu  sp,sp,-((2+16+22+4)*8+sizestackframeinfo) /* create stack frame   */

	SAVE_RETURN_REGISTERS(0)      /* save 1 int/1 float return registers      */
	SAVE_ARGUMENT_REGISTERS(2)    /* save 8 int/8 float argument registers    */
	SAVE_TEMPORARY_REGISTERS(18)  /* save 5 int/16 float temporary registers  */

	ast     itmp1,(2+16+22+0)*8(sp) /* save itmp1                             */
	ast     itmp2,(2+16+22+1)*8(sp) /* save itmp2                             */
	ast     ra,(2+16+22+2)*8(sp)  /* save method return address (for leafs)   */
	ast     pv,(2+16+22+3)*8(sp)  /* save pv of calling java function         */

	aaddiu  a0,sp,(2+16+22+4)*8   /* create stackframe info                   */
	move    a1,pv                 /* pass java pv                             */
	aaddiu  a2,sp,((6+2+16+22+4)*8+sizestackframeinfo) /* pass java sp        */
	move    a3,ra                 /* this is correct for leafs                */
	ald     a4,((5+2+16+22+4)*8+sizestackframeinfo)(sp) /* pass xpc           */
	jal     stacktrace_create_extern_stackframeinfo

	aaddiu  a0,sp,((0+2+16+22+4)*8+sizestackframeinfo) /* pass sp             */
	ald     itmp3,((0+2+16+22+4)*8+sizestackframeinfo)(sp) /* get function    */
	ald     itmp1,(2+16+22+3)*8(sp) /* save pv to the position of fp          */
	ast     itmp1,((0+2+16+22+4)*8+sizestackframeinfo)(sp)
	jalr    itmp3
	ast     v0,((0+2+16+22+4)*8+sizestackframeinfo)(sp) /* save return value  */

	aaddiu  a0,sp,(2+16+22+4)*8   /* remove stackframe info                   */
	jal     stacktrace_remove_stackframeinfo

	RESTORE_RETURN_REGISTERS(0)   /* restore 1 int/1 float return registers   */
	RESTORE_ARGUMENT_REGISTERS(2) /* restore 8 int/8 float argument registers */
	RESTORE_TEMPORARY_REGISTERS(18) /* restore 5 int/16 float temporary reg.  */

	ald     itmp1,(2+16+22+0)*8(sp) /* restore itmp1                          */
	ald     itmp2,(2+16+22+1)*8(sp) /* restore itmp2                          */
	ald     ra,(2+16+22+2)*8(sp)  /* restore method return address (for leafs)*/
	ald     pv,(2+16+22+3)*8(sp)  /* restore pv of calling java function      */

	ald     itmp3,((0+2+16+22+4)*8+sizestackframeinfo)(sp) /* get return value*/
	beqz    itmp3,L_asm_wrapper_patcher_exception

	ald     itmp3,((5+2+16+22+4)*8+sizestackframeinfo)(sp) /* get RA to JIT   */
	aaddiu  sp,sp,((6+2+16+22+4)*8+sizestackframeinfo) /* remove stack frame  */

	jr      itmp3                 /* jump to new patched code                 */

L_asm_wrapper_patcher_exception:
	ald     xpc,((5+2+16+22+4)*8+sizestackframeinfo)(sp) /* RA to JIT is xpc  */
	aaddiu  sp,sp,((6+2+16+22+4)*8+sizestackframeinfo) /* remove stack frame  */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	daddiu  sp,sp,-4*8
	sd      xpc,0*8(sp)
	sd      ra,1*8(sp)
	sd      pv,2*8(sp)
	jal     builtin_asm_get_exceptionptrptr
	ld      xpc,0*8(sp)
	ld      ra,1*8(sp)
	ld      pv,2*8(sp)
	daddiu  sp,sp,4*8
#else
	la      v0,_exceptionptr
#endif
	ld      xptr,0(v0)            /* get the exception pointer                */
	sd      zero,0(v0)            /* clear the exception pointer              */
	b       asm_handle_exception

	.end	asm_wrapper_patcher

		
/******************* function asm_initialize_thread_stack **********************
*                                                                              *
*   u1* asm_initialize_thread_stack (void *func, u1 *stack);                   *
*                                                                              *
*   initialize a thread stack                                                  *
*                                                                              *
*******************************************************************************/

	.ent    asm_initialize_thread_stack

asm_initialize_thread_stack:
	aaddiu  a1,a1,-14*8     /* allocate save area                             */
	sd      zero, 0*8(a1)   /* s0 initalize thread area                       */
	sd      zero, 1*8(a1)   /* s1                                             */
	sd      zero, 2*8(a1)   /* s2                                             */
	sd      zero, 3*8(a1)   /* s3                                             */
	sd      zero, 4*8(a1)   /* s4                                             */
	sd      zero, 5*8(a1)   /* s5                                             */
	sd      zero, 6*8(a1)   /* s6                                             */
	sd      zero, 7*8(a1)   /* s7                                             */
	sd      zero, 8*8(a1)   /* s8                                             */
	sd      zero, 9*8(a1)   /* fs0                                            */
	sd      zero,10*8(a1)   /* fs1                                            */
	sd      zero,11*8(a1)   /* fs2                                            */
	sd      zero,12*8(a1)   /* fs3                                            */
	sd      a0, 13*8(a1)
	move    v0,a1
	j       ra              /* return                                         */

	.end    asm_initialize_thread_stack


/******************* function asm_perform_threadswitch *************************
*                                                                              *
*   void asm_perform_threadswitch (u1 **from, u1 **to, u1 **stackTop);         *
*                                                                              *
*   performs a threadswitch                                                    *
*                                                                              *
*******************************************************************************/

	.ent    asm_perform_threadswitch

asm_perform_threadswitch:
	aaddiu  sp,sp,-14*8     /* allocate new stack                             */
	sd      s0,  0*8(sp)    /* save saved registers of old thread             */
	sd      s1,  1*8(sp)
	sd      s2,  2*8(sp)
	sd      s3,  3*8(sp)
	sd      s4,  4*8(sp)
	sd      s5,  5*8(sp)
	sd      s6,  6*8(sp)
	sd      s7,  7*8(sp)
	sd      s8,  8*8(sp)
	sdc1    fs0, 9*8(sp)
	sdc1    fs1,10*8(sp)
	sdc1    fs2,11*8(sp)
	sdc1    fs3,12*8(sp)
	sd      ra, 13*8(sp)
	ast     sp,0(a0)        /* save old stack pointer                         */
	ast     sp,0(a2)        /* stackTop = old stack pointer                   */
	ald     sp,0(a1)        /* load new stack pointer                         */
	ld      s0,  0*8(sp)    /* load saved registers of new thread             */
	ld      s1,  1*8(sp)
	ld      s2,  2*8(sp)
	ld      s3,  3*8(sp)
	ld      s4,  4*8(sp)
	ld      s5,  5*8(sp)
	ld      s6,  6*8(sp)
	ld      s7,  7*8(sp)
	ld      s8,  8*8(sp)
	ldc1    fs0, 9*8(sp)
	ldc1    fs1,10*8(sp)
	ldc1    fs2,11*8(sp)
	ldc1    fs3,12*8(sp)
	ld      ra, 13*8(sp)
	aaddiu  sp,sp,14*8      /* deallocate new stack                           */
	move    itmp3, ra
	j       ra              /* return                                         */

	.end    asm_perform_threadswitch


/********************* function asm_switchstackandcall *************************
*                                                                              *
*  void asm_switchstackandcall (void *stack, void *func, void **stacktopsave); *
*                                                                              *
*   Switches to a new stack, calls a function and switches back.               *
*       a0      new stack pointer                                              *
*       a1      function pointer                                               *
*		a2		pointer to variable where stack top should be stored           *
*                                                                              *
*******************************************************************************/

	.ent	asm_switchstackandcall

asm_switchstackandcall:
	aaddiu  a0,a0,-16       /* allocate new stack                             */
	sd      ra,0(a0)        /* save return address on new stack               */
	sd      sp,8(a0)        /* save old stack pointer on new stack            */
	sd      sp,0(a2)        /* save old stack pointer to variable             */
	move    sp,a0           /* switch to new stack                            */
	
	move    itmp3,a1
	move    a0,a3
	jalr    itmp3           /* and call function                              */

	ld      ra,0(sp)        /* load return address                            */
	ld      sp,8(sp)        /* switch to old stack                            */

	j       ra              /* return                                         */

	.end	asm_switchstackandcall


	.ent    asm_getclassvalues_atomic

asm_getclassvalues_atomic:
_crit_restart:
_crit_begin:
	lw      t0,offbaseval(a0)
	lw      t1,offdiffval(a0)
	lw      t2,offbaseval(a1)
_crit_end:
	sw      t0,offcast_super_baseval(a2)
	sw      t1,offcast_super_diffval(a2)
	sw      t2,offcast_sub_baseval(a2)
	j       ra

	.end    asm_getclassvalues_atomic

    .data

asm_criticalsections:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
    .dword  _crit_begin
    .dword  _crit_end
    .dword  _crit_restart
#endif
    .dword  0


	.text

	.ent    compare_and_swap

compare_and_swap:
1:
	all     v0,0(a0)
	bne     v0,a1,2f
	move    t0,a2
	asc     t0,0(a0)
	beqz    t0,1b
2:
	sync
	j       ra

	.end    compare_and_swap


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
