/* src/vm/jit/alpha/asmpart.S - Java-C interface functions for alpha

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

   Changes: Joseph Wenninger
            Christian Thalinger

   $Id: asmpart.S 4357 2006-01-22 23:33:38Z twisti $

*/


#include "config.h"

#include "vm/jit/alpha/md-abi.h"
#include "vm/jit/alpha/md-asm.h"
#include "vm/jit/alpha/offsets.h"

#include "vm/jit/abi.h"
#include "vm/jit/methodheader.h"


	.text
	.set    noat
	.set    noreorder


/********************* exported functions and variables ***********************/

	.globl asm_sync_instruction_cache
	.globl has_no_x_instr_set

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
	.globl asm_criticalsections
	.globl asm_getclassvalues_atomic


/* asm_sync_instruction_cache **************************************************

   XXX

*******************************************************************************/

	.ent    asm_sync_instruction_cache

asm_sync_instruction_cache:
	call_pal PAL_imb              /* synchronize instruction cache            */
	jmp     zero,(ra)

	.end    asm_sync_instruction_cache

/*********************** function has_no_x_instr_set ***************************
*                                                                              *
*   determines if the byte support instruction set (21164a and higher)         *
*   is available.                                                              *
*                                                                              *
*******************************************************************************/

	.ent    has_no_x_instr_set
has_no_x_instr_set:

	.long   0x47e03c20                /* amask   1,v0                         */
	jmp     zero,(ra)                 /* return                               */

	.end    has_no_x_instr_set


/********************* function asm_calljavafunction ***************************
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

	.ent    asm_calljavafunction

	.align  2 /*3*/

	.quad   0                           /* catch type all                     */
	.quad   calljava_xhandler           /* handler pc                         */
	.quad   calljava_xhandler           /* end pc                             */
	.quad   asm_calljavafunction        /* start pc                           */
	.long   1                           /* extable size                       */
	.long   0                           /* ALIGNMENT PADDING                  */
	.quad   0                           /* line number table start            */
	.quad   0                           /* line number table size             */
	.long   0                           /* ALIGNMENT PADDING                  */
	.long   0                           /* fltsave                            */
	.long   0                           /* intsave                            */
	.long   0                           /* isleaf                             */
	.long   0                           /* IsSync                             */
	.long   0                           /* frame size                         */
	.quad   0                           /* method pointer (pointer to name)   */

asm_calljavafunction:
asm_calljavafunction_int:
	ldgp    gp,0(pv)
	lda     sp,-4*8(sp)                 /* allocate stack space               */
	stq     ra,0*8(sp)                  /* save return address                */
	stq     gp,1*8(sp)                  /* save global pointer                */

	stq     a0,2*8(sp)                  /* save method pointer for compiler   */
	lda     v0,2*8(sp)                  /* pass pointer to method pointer     */

	mov     a1,a0                       /* pass the remaining parameters      */
	mov     a2,a1
	mov     a3,a2
	mov     a4,a3

	lda     itmp2,asm_call_jit_compiler /* fake virtual function call         */
	stq     itmp2,3*8(sp)               /* store function address             */
	mov     sp,itmp2                    /* set method pointer                 */

	ldq     pv,3*8(itmp2)               /* method call as in Java             */
	jmp     ra,(pv)                     /* call JIT compiler                  */
calljava_jit:
	lda     pv,(asm_calljavafunction - calljava_jit)(ra)

L_asm_calljavafunction_return:
	ldq     ra,0*8(sp)                  /* restore return address             */
	ldq     gp,1*8(sp)                  /* restore global pointer             */
	lda     sp,4*8(sp)                  /* free stack space                   */
	jmp     zero,(ra)

calljava_xhandler:
	ldq     gp,1*8(sp)                  /* restore global pointer             */
	mov     xptr,a0
	jsr     ra,builtin_throw_exception
	mov     zero,v0                     /* return NULL                        */
	br      L_asm_calljavafunction_return

	.end    asm_calljavafunction




	.ent    asm_calljavafunction2

	.align  3

	.quad   0                           /* catch type all                     */
	.quad   calljava_xhandler2          /* handler pc                         */
	.quad   calljava_xhandler2          /* end pc                             */
	.quad   asm_calljavafunction2       /* start pc                           */
	.long   1                           /* extable size                       */
	.long   0                           /* ALIGNMENT PADDING                  */
	.quad   0                           /* line number table start            */
	.quad   0                           /* line number table size             */
	.long   0                           /* ALIGNMENT PADDING                  */
	.long   0                           /* fltsave                            */
	.long   1                           /* intsave                            */
	.long   0                           /* isleaf                             */
	.long   0                           /* IsSync                             */
	.long   0                           /* frame size                         */
	.quad   0                           /* method pointer (pointer to name)   */

asm_calljavafunction2:
asm_calljavafunction2int:
asm_calljavafunction2long:
asm_calljavafunction2float:
asm_calljavafunction2double:
	ldgp    gp,0(pv)
	lda     sp,-5*8(sp)               /* allocate stack space                 */
	stq     ra,0*8(sp)                /* save return address                  */
	stq     gp,1*8(sp)                /* save global pointer                  */
	stq     s6,3*8(sp)

	stq     a0,4*8(sp)                /* save method pointer for compiler     */
	mov     a3,t0                     /* pointer to arg block                 */
	mov     a1,s6                     /* arg count                            */

	ble     s6,calljava_argsloaded
	lda     s6,-1(s6)
	ldq     a0,offjniitem(t0)
	ldt     $f16,offjniitem(t0)
	ble     s6,calljava_argsloaded

	lda     s6,-1(s6)
	ldq     a1,offjniitem+sizejniblock*1(t0)
	ldt     $f17,offjniitem+sizejniblock*1(t0)
	ble     s6,calljava_argsloaded

	lda     s6,-1(s6)
	ldq     a2,offjniitem+sizejniblock*2(t0)
	ldt     $f18,offjniitem+sizejniblock*2(t0)
	ble     s6,calljava_argsloaded

	lda     s6,-1(s6)
	ldq     a3,offjniitem+sizejniblock*3(t0)
	ldt     $f19,offjniitem+sizejniblock*3(t0)
	ble     s6,calljava_argsloaded

	lda     s6,-1(s6)
	ldq     a4,offjniitem+sizejniblock*4(t0)
	ldt     $f20,offjniitem+sizejniblock*4(t0)
	ble     s6,calljava_argsloaded

	lda     s6,-1(s6)
	ldq     a5,offjniitem+sizejniblock*5(t0)
	ldt     $f21,offjniitem+sizejniblock*5(t0)
calljava_argsloaded:
	mov     sp,t4
	ble     s6,calljava_nocopy
	negq    s6,t1
	s8addq  t1,sp,sp
	s8addq  t1,t4,t2

calljava_copyloop:
	ldq     t3,offjniitem+sizejniblock*6(t0)
	stq     t3,0(t2)
	lda     t1,1(t1)
	lda     t0,sizejniblock(t0)
	lda     t2,8(t2)
	bne     t1,calljava_copyloop

calljava_nocopy:
	lda     v0,4*8(t4)                /* pass pointer to method pointer via v0*/

	lda     itmp2,asm_call_jit_compiler/* fake virtual function call (2 instr)*/
	stq     itmp2,16(t4)              /* store function address               */
	lda     itmp2,8(t4)               /* set method pointer                   */

	ldq     pv,8(itmp2)               /* method call as in Java               */
	jmp     ra,(pv)                   /* call JIT compiler                    */
calljava_jit2:
	lda     pv,(asm_calljavafunction2 - calljava_jit2)(ra)

	s8addq  s6,sp,sp
calljava_return2:
	ldq     ra,0*8(sp)                /* restore return address               */
	ldq     gp,1*8(sp)                /* restore global pointer               */
	ldq     s6,3*8(sp)
	lda     sp,5*8(sp)                /* free stack space                     */

calljava_ret2:
	jmp     zero,(ra)

calljava_xhandler2:
	s8addq  s6,sp,sp
	ldq     gp,1*8(sp)                /* restore global pointer               */
	mov     itmp1,a0
	jsr     ra,builtin_throw_exception
	ldq     ra,0*8(sp)                /* restore return address               */
	ldq     s6,3*8(sp)
	lda     sp,5*8(sp)                /* free stack space                     */
	mov     zero,v0                   /* return NULL                          */
	jmp     zero,(ra)

	.end    asm_calljavafunction2


/****************** function asm_call_jit_compiler *****************************
*                                                                              *
*   invokes the compiler for untranslated JavaVM methods.                      *
*                                                                              *
*   Register R0 contains a pointer to the method info structure (prepared      *
*   by createcompilerstub). Using the return address in R26 and the            *
*   offset in the LDA instruction or using the value in methodptr R28 the      *
*   patching address for storing the method address can be computed:           *
*                                                                              *
*   method address was either loaded using                                     *
*   M_LDQ (REG_PV, REG_PV, a)        ; invokestatic/special    ($27)           *
*   M_LDA (REG_PV, REG_RA, low)                                                *
*   M_LDAH(REG_PV, REG_RA, high)     ; optional                                *
*   or                                                                         *
*   M_LDQ (REG_PV, REG_METHODPTR, m) ; invokevirtual/interface ($28)           *
*   in the static case the method pointer can be computed using the            *
*   return address and the lda function following the jmp instruction          *
*                                                                              *
*******************************************************************************/

	.ent    asm_call_jit_compiler

asm_call_jit_compiler:
	ldgp    gp,0(pv)
	ldl     t8,-8(ra)             /* load instruction LDQ PV,xxx($yy)         */
	srl     t8,16,t8              /* shift right register number $yy          */
	and     t8,31,t8              /* isolate register number                  */
	subl    t8,28,t8              /* test for REG_METHODPTR                   */
	beq     t8,noregchange       
	ldl     t8,0(ra)              /* load instruction LDA PV,xxx(RA)          */
	sll     t8,48,t8
	sra     t8,48,t8              /* isolate offset                           */
	addq    t8,ra,$28             /* compute update address                   */
	ldl     t8,4(ra)              /* load instruction LDAH PV,xxx(PV)         */
	srl     t8,16,t8              /* isolate instruction code                 */
	lda     t8,-0x177b(t8)        /* test for LDAH                            */
	bne     t8,noregchange       
	ldl     t8,4(ra)              /* load instruction LDAH PV,xxx(PV)         */
	sll     t8,16,t8              /* compute high offset                      */
	addl    t8,0,t8               /* sign extend high offset                  */
	addq    t8,$28,$28            /* compute update address                   */
noregchange:
	lda     sp,-(15*8+sizestackframeinfo)(sp) /* reserve stack space          */

	SAVE_ARGUMENT_REGISTERS(0)    /* save 6 int/6 float argument registers    */

	stq     $28,12*8(sp)          /* save method pointer                      */
	stq     ra,13*8(sp)           /* save return address                      */
	stq     v0,14*8(sp)           /* save methodinfo pointer                  */

	lda     a0,15*8(sp)           /* create stackframe info                   */
	mov     zero,a1               /* we don't have pv handy                   */
	lda     a2,15*8+sizestackframeinfo(sp) /* pass java sp                    */
	mov     ra,a3                 /* pass Java ra                             */
	mov     a3,a4                 /* xpc is equal to ra                       */
	jsr     ra,stacktrace_create_extern_stackframeinfo
	ldgp    gp,0(ra)

	ldq     v0,14*8(sp)           /* restore methodinfo pointer               */
	ldq     a0,0(v0)              /* pass methodinfo pointer                  */
	jsr     ra,jit_compile        /* call jit compiler                        */
	ldgp    gp,0(ra)
	stq     v0,14*8(sp)           /* save return value                        */

	lda     a0,15*8(sp)           /* remove stackframe info                   */
	jsr     ra,stacktrace_remove_stackframeinfo
	ldgp    gp,0(ra)

	RESTORE_ARGUMENT_REGISTERS(0) /* restore 6 int/6 float argument registers */

	ldq     $28,12*8(sp)          /* load method pointer                      */
	ldq     ra,13*8(sp)           /* load return address                      */
	ldq     v0,14*8(sp)           /* restore return value                     */
	lda     sp,15*8+sizestackframeinfo(sp) /* deallocate stack area           */

	beq     v0,L_asm_call_jit_compiler_exception

	ldl     t8,-8(ra)             /* load instruction LDQ PV,xxx($yy)         */
	sll     t8,48,t8
	sra     t8,48,t8              /* isolate offset                           */

	addq    t8,$28,t8             /* compute update address via method pointer*/
	stq     v0,0(t8)              /* save new method address there            */

	call_pal PAL_imb              /* synchronise instruction cache            */

	mov     v0,pv                 /* load method address into pv              */
	jmp     zero,(pv)             /* and call method. The method returns      */
	                              /* directly to the caller (ra).             */

L_asm_call_jit_compiler_exception:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	subq    sp,1*8,sp
	stq     ra,0*8(sp)            /* save return address (xpc)                */
	jsr     ra,builtin_asm_get_exceptionptrptr
	ldq     ra,0*8(sp)           /* restore return address (xpc)             */
	addq    sp,1*8,sp
#else
	lda     v0,_exceptionptr
#endif
	ldq     xptr,0(v0)            /* get the exception pointer                */
	stq     zero,0(v0)            /* clear the exception pointer              */

	subq    ra,4,xpc
	br      asm_handle_nat_exception

	.end    asm_call_jit_compiler


/* asm_handle_exception ********************************************************

   This function handles an exception. It does not use the usual calling
   conventions. The exception pointer is passed in REG_ITMP1 and the
   pc from the exception raising position is passed in REG_ITMP2. It searches
   the local exception table for a handler. If no one is found, it unwinds
   stacks and continues searching the callers.

   ATTENTION: itmp3 == gp!

*******************************************************************************/

	.ent    asm_handle_nat_exception

asm_handle_nat_exception:
	ldl     t0,0(ra)              /* load instruction LDA PV,xxx(RA)          */
	sll     t0,48,t0
	sra     t0,48,t0              /* isolate offset                           */
	addq    t0,ra,pv              /* compute update address                   */
	ldl     t0,4(ra)              /* load instruction LDAH PV,xxx(PV)         */
	srl     t0,16,t0              /* isolate instruction code                 */
	lda     t0,-0x177b(t0)        /* test for LDAH                            */
	bne     t0,asm_handle_exception
	ldl     t0,4(ra)              /* load instruction LDAH PV,xxx(PV)         */
	sll     t0,16,t0              /* compute high offset                      */
	addl    t0,0,t0               /* sign extend high offset                  */
	addq    t0,pv,pv              /* compute update address                   */

	.aent    asm_handle_exception

asm_handle_exception:
	lda     sp,-(ARG_CNT+TMP_CNT)*8(sp) /* create maybe-leaf stackframe       */

	SAVE_ARGUMENT_REGISTERS(0)          /* we save arg and temp registers in  */
	SAVE_TEMPORARY_REGISTERS(ARG_CNT)   /* case this is a leaf method         */

	lda     a3,(ARG_CNT+TMP_CNT)*8(zero)/* prepare a3 for handle_exception */
	lda     a4,1(zero)                  /* set maybe-leaf flag                */

L_asm_handle_exception_stack_loop:
	lda     sp,-5*8(sp)                 /* allocate stack                     */
	stq     xptr,0*8(sp)                /* save exception pointer             */
	stq     xpc,1*8(sp)                 /* save exception pc                  */
	stq     pv,2*8(sp)                  /* save data segment pointer          */
	stq     ra,3*8(sp)                  /* save return address                */
	addq    a3,sp,a3                    /* calculate Java sp into a3...       */
	addq    a3,5*8,a3
	stq     a4,4*8(sp)                  /* save maybe-leaf flag               */

	br      ra,L_asm_handle_exception_load_gp /* set ra for gp loading        */
L_asm_handle_exception_load_gp:
	ldgp    gp,0(ra)                    /* load gp                            */

	mov     xptr,a0                     /* pass exception pointer             */
	mov     xpc,a1                      /* pass exception pc                  */
	mov     pv,a2                       /* pass data segment pointer          */
	                                    /* a3 is still set                    */
	jsr     ra,exceptions_handle_exception

	beq     v0,L_asm_handle_exception_not_catched

	mov     v0,xpc                      /* move handlerpc into xpc            */
	ldq     xptr,0*8(sp)                /* restore exception pointer          */
	ldq     pv,2*8(sp)                  /* restore data segment pointer       */
	ldq     ra,3*8(sp)                  /* restore return address             */
	ldq     a4,4*8(sp)                  /* get maybe-leaf flag                */
	lda     sp,5*8(sp)                  /* free stack frame                   */

	beq     a4,L_asm_handle_exception_no_leaf

	RESTORE_ARGUMENT_REGISTERS(0)       /* if this is a leaf method, we have  */
	RESTORE_TEMPORARY_REGISTERS(ARG_CNT)/* to restore arg and temp registers  */
	
	lda     sp,(ARG_CNT+TMP_CNT)*8(sp)  /* remove maybe-leaf stackframe       */

L_asm_handle_exception_no_leaf:
	jmp     zero,(xpc)                  /* jump to the handler                */

L_asm_handle_exception_not_catched:
	ldq     xptr,0*8(sp)                /* restore exception pointer          */
	ldq     pv,2*8(sp)                  /* restore data segment pointer       */
	ldq     ra,3*8(sp)                  /* restore return address             */
	ldq     a4,4*8(sp)                  /* get maybe-leaf flag                */
	lda     sp,5*8(sp)

	beq     a4,L_asm_handle_exception_no_leaf_stack

	lda     sp,(ARG_CNT+TMP_CNT)*8(sp)  /* remove maybe-leaf stackframe       */
	mov     zero,a4                     /* clear the maybe-leaf flag          */

L_asm_handle_exception_no_leaf_stack:
	ldl     t0,FrameSize(pv)            /* get frame size                     */
	addq    t0,sp,t0                    /* pointer to save area               */

	ldl     t1,IsLeaf(pv)               /* is leaf procedure                  */
	bne     t1,L_asm_handle_exception_no_ra_restore

	ldq     ra,-1*8(t0)                 /* restore ra                         */
	subq    t0,8,t0                     /* t0--                               */

L_asm_handle_exception_no_ra_restore:
	mov     ra,xpc                      /* the new xpc is ra                  */
	ldl     t1,IntSave(pv)              /* t1 = saved int register count      */
	br      t2,ex_int1                  /* t2 = current pc                    */
ex_int1:
	lda     t2,(ex_int2-ex_int1)(t2)
	negl    t1,t1                       /* negate register count              */
	s4addq  t1,t2,t2                    /* t2 = IntSave - register count * 4  */
	jmp     zero,(t2)                   /* jump to save position              */

	ldq     s0,-7*8(t0)
	ldq     s1,-6*8(t0)
	ldq     s2,-5*8(t0)
	ldq     s3,-4*8(t0)
	ldq     s4,-3*8(t0)
	ldq     s5,-2*8(t0)
	ldq     s6,-1*8(t0)

ex_int2:
	s8addq  t1,t0,t0                    /* t0 = t0 - 8 * register count       */

	ldl     t1,FltSave(pv)              /* t1 = saved flt register count      */
	br      t2,ex_flt1                  /* t2 = current pc                    */
ex_flt1:
	lda     t2,(ex_flt2-ex_flt1)(t2)
	negl    t1,t1                       /* negate register count              */
	s4addq  t1,t2,t2                    /* t2 = FltSave - 4 * register count  */
	jmp     zero,(t2)                   /* jump to save position              */

	ldt     fs0,-8*8(t0)
	ldt     fs1,-7*8(t0)
	ldt     fs2,-6*8(t0)
	ldt     fs3,-5*8(t0)
	ldt     fs4,-4*8(t0)
	ldt     fs5,-3*8(t0)
	ldt     fs6,-2*8(t0)
	ldt     fs7,-1*8(t0)

ex_flt2:
	ldl     t0,FrameSize(pv)            /* get frame size                     */
	addq    sp,t0,sp                    /* unwind stack                       */
	mov     zero,a3                     /* prepare a3 for handle_exception    */

	ldl     t0,0(ra)              /* load instruction LDA PV,xxx(RA)          */
	sll     t0,48,t0
	sra     t0,48,t0              /* isolate offset                           */
	addq    t0,ra,pv              /* compute update address                   */
	ldl     t0,4(ra)              /* load instruction LDAH PV,xxx(PV)         */
	srl     t0,16,t0              /* isolate instruction code                 */
	lda     t0,-0x177b(t0)        /* test for LDAH                            */
	bne     t0,L_asm_handle_exception_stack_loop
	ldl     t0,4(ra)              /* load instruction LDAH PV,xxx(RA)         */
	sll     t0,16,t0              /* compute high offset                      */
	addl    t0,0,t0               /* sign extend high offset                  */
	addq    t0,pv,pv              /* compute update address                   */

	br      L_asm_handle_exception_stack_loop

	.end    asm_handle_nat_exception


/* asm_wrapper_patcher *********************************************************

   XXX

   Stack layout:
     40   return address into JIT code (patch position)
     32   pointer to virtual java_objectheader
     24   machine code (which is patched back later)
     16   unresolved class/method/field reference
      8   data segment displacement from load instructions
      0   patcher function pointer to call (pv afterwards)

   ATTENTION: itmp3 == gp! But we don't need gp do call the patcher function.

*******************************************************************************/
		
	.ent    asm_wrapper_patcher

asm_wrapper_patcher:
	lda     sp,-((2+12+27+4)*8+sizestackframeinfo)(sp) /* create stack frame  */

	SAVE_RETURN_REGISTERS(0)      /* save 1 int/1 float return registers      */
	SAVE_ARGUMENT_REGISTERS(2)    /* save 6 int/6 float argument registers    */
	SAVE_TEMPORARY_REGISTERS(14)  /* save 11 int/16 float temporary registers */

	stq     itmp1,(2+12+27+0)*8(sp) /* save itmp1                             */
	stq     itmp2,(2+12+27+1)*8(sp) /* save itmp2                             */
	stq     ra,(2+12+27+2)*8(sp)  /* save method return address (for leafs)   */
	stq     pv,(2+12+27+3)*8(sp)  /* save pv of calling java function         */

	br      ra,L_asm_wrapper_patcher_load_gp
L_asm_wrapper_patcher_load_gp:
	ldgp    gp,0(ra)              /* load gp (it's not set correctly in jit)  */

	lda     a0,(2+12+27+4)*8(sp)  /* create stackframe info                   */
	mov     pv,a1                 /* pass java pv                             */
	lda     a2,((6+2+12+27+4)*8+sizestackframeinfo)(sp) /* pass java sp       */
	ldq     a3,(2+12+27+2)*8(sp)  /* this is correct for leafs                */
	ldq     a4,((5+2+12+27+4)*8+sizestackframeinfo)(sp) /* pass xpc           */
	jsr     ra,stacktrace_create_extern_stackframeinfo
	ldgp    gp,0(ra)

	lda     a0,((0+2+12+27+4)*8+sizestackframeinfo)(sp) /* pass sp            */
	ldq     pv,((0+2+12+27+4)*8+sizestackframeinfo)(sp) /* get function       */
	ldq     itmp1,(2+12+27+3)*8(sp) /* save pv to the position of fp          */
	stq     itmp1,((0+2+12+27+4)*8+sizestackframeinfo)(sp)
	jmp     ra,(pv)               /* call the patcher function                */
	ldgp    gp,0(ra)
	stq     v0,((0+2+12+27+4)*8+sizestackframeinfo)(sp) /* save return value  */

	lda     a0,(2+12+27+4)*8(sp)  /* remove stackframe info                   */
	jsr     ra,stacktrace_remove_stackframeinfo
	ldgp    gp,0(ra)

	RESTORE_RETURN_REGISTERS(0)   /* restore 1 int/1 float return registers   */
	RESTORE_ARGUMENT_REGISTERS(2) /* restore 6 int/6 float argument registers */
	RESTORE_TEMPORARY_REGISTERS(14) /* restore 11 integer temporary registers */

	ldq     itmp1,(2+12+27+0)*8(sp) /* restore itmp1                          */
	ldq     itmp2,(2+12+27+1)*8(sp) /* restore itmp2                          */
	ldq     ra,(2+12+27+2)*8(sp)  /* restore method return address (for leafs)*/
	ldq     pv,(2+12+27+3)*8(sp)  /* restore pv of calling java function      */

	ldq     itmp3,((0+2+12+27+4)*8+sizestackframeinfo)(sp) /* get return value*/
	beq     itmp3,L_asm_wrapper_patcher_exception

	ldq     itmp3,((5+2+12+27+4)*8+sizestackframeinfo)(sp)/* get RA to JIT    */
	lda     sp,((6+2+12+27+4)*8+sizestackframeinfo)(sp) /* remove stack frame */

	jmp     zero,(itmp3)          /* jump to new patched code                 */

L_asm_wrapper_patcher_exception:
	ldq     xpc,((5+2+12+27+4)*8+sizestackframeinfo)(sp) /* RA is xpc         */
	lda     sp,((6+2+12+27+4)*8+sizestackframeinfo)(sp) /* remove stack frame */

	br      itmp1,L_asm_wrapper_patcher_exception_load_gp
L_asm_wrapper_patcher_exception_load_gp:
	ldgp    gp,0(itmp1)           /* itmp3 == gp, load the current gp         */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	subq    sp,3*8,sp
	stq     xpc,0*8(sp)           /* save return address (xpc)                */
	stq     ra,1*8(sp)
	stq     pv,2*8(sp)
	jsr     ra,builtin_asm_get_exceptionptrptr
	ldq     xpc,0*8(sp)           /* restore return address (xpc)             */
	ldq     ra,1*8(sp)
	ldq     pv,2*8(sp)
	addq    sp,3*8,sp
#else
	lda     v0,_exceptionptr
#endif
	ldq     xptr,0(v0)            /* get the exception pointer                */
	stq     zero,0(v0)            /* clear the exception pointer              */
	br      asm_handle_exception  /* we have the pv of the calling java func. */

	.end	asm_wrapper_patcher

		
/******************* function asm_initialize_thread_stack **********************
*                                                                              *
*   initialized a thread stack                                                 *
*                                                                              *
*******************************************************************************/

	.ent    asm_initialize_thread_stack

asm_initialize_thread_stack:
	lda     a1,-128(a1)
	stq     zero, 0(a1)
	stq     zero, 8(a1)
	stq     zero, 16(a1)
	stq     zero, 24(a1)
	stq     zero, 32(a1)
	stq     zero, 40(a1)
	stq     zero, 48(a1)
	stt     fzero, 56(a1)
	stt     fzero, 64(a1)
	stt     fzero, 72(a1)
	stt     fzero, 80(a1)
	stt     fzero, 88(a1)
	stt     fzero, 96(a1)
	stt     fzero, 104(a1)
	stt     fzero, 112(a1)
	stq     a0, 120(a1)
	mov     a1, v0
	jmp     zero,(ra)

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
	subq    sp,128,sp
	stq     s0, 0(sp)
	stq     s1, 8(sp)
	stq     s2, 16(sp)
	stq     s3, 24(sp)
	stq     s4, 32(sp)
	stq     s5, 40(sp)
	stq     s6, 48(sp)
	stt     fs0, 56(sp)
	stt     fs1, 64(sp)
	stt     fs2, 72(sp)
	stt     fs3, 80(sp)
	stt     fs4, 88(sp)
	stt     fs5, 96(sp)
	stt     fs6, 104(sp)
	stt     fs7, 112(sp)
	stq     ra, 120(sp)
	stq     sp, 0(a0)
	stq     sp, 0(a2)
	ldq     sp, 0(a1)
	ldq     s0, 0(sp)
	ldq     s1, 8(sp)
	ldq     s2, 16(sp)
	ldq     s3, 24(sp)
	ldq     s4, 32(sp)
	ldq     s5, 40(sp)
	ldq     s6, 48(sp)
	ldt     fs0, 56(sp)
	ldt     fs1, 64(sp)
	ldt     fs2, 72(sp)
	ldt     fs3, 80(sp)
	ldt     fs4, 88(sp)
	ldt     fs5, 96(sp)
	ldt     fs6, 104(sp)
	ldt     fs7, 112(sp)
	ldq     ra, 120(sp)
	mov     ra, pv
	addq    sp, 128, sp
	jmp	zero,(ra)

	.end    asm_perform_threadswitch


/********************* function asm_switchstackandcall *************************
*                                                                              *
*  void *asm_switchstackandcall (void *stack, void *func, void **stacktopsave, *
*                               void *p);                                      *
*                                                                              *
*   Switches to a new stack, calls a function and switches back.               *
*       a0      new stack pointer                                              *
*       a1      function pointer                                               *
*		a2		pointer to variable where stack top should be stored           *
*		a3      pointer to user data, is passed to the function                *
*                                                                              *
*******************************************************************************/


	.ent	asm_switchstackandcall

asm_switchstackandcall:
	lda	a0,-2*8(a0)     /* allocate new stack                                 */
	stq	ra,0(a0)        /* save return address on new stack                   */
	stq	sp,1*8(a0)      /* save old stack pointer on new stack                */
	stq sp,0(a2)        /* save old stack pointer to variable                 */
	mov	a0,sp           /* switch to new stack                                */
	
	mov	a1,pv           /* load function pointer                              */
	mov a3,a0           /* pass pointer */
	jmp	ra,(pv)         /* and call function                                  */

	ldq	ra,0(sp)        /* load return address                                */
	ldq	sp,1*8(sp)      /* switch to old stack                                */

	jmp	zero,(ra)       /* return                                             */

	.end	asm_switchstackandcall


	.ent    asm_getclassvalues_atomic

asm_getclassvalues_atomic:
_crit_restart:
_crit_begin:
	ldl     t0,offbaseval(a0)
	ldl     t1,offdiffval(a0)
	ldl     t2,offbaseval(a1)
_crit_end:
	stl     t0,offcast_super_baseval(a2)
	stl     t1,offcast_super_diffval(a2)
	stl     t2,offcast_sub_baseval(a2)
	jmp     zero,(ra)

	.end    asm_getclassvalues_atomic


    .data

asm_criticalsections:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
    .quad   _crit_begin
    .quad   _crit_end
    .quad   _crit_restart
#endif
    .quad   0


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
