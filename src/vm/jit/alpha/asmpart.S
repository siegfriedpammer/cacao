/* src/vm/jit/alpha/asmpart.S - Java-C interface functions for alpha

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Andreas Krall
            Reinhard Grafl

   Changes: Joseph Wenninger
            Christian Thalinger

   $Id: asmpart.S 2911 2005-07-05 08:49:55Z twisti $

*/


#include "config.h"        
#include "vm/jit/alpha/offsets.h"
#include "vm/jit/alpha/asmoffsets.h"
#include "vm/jit/alpha/md-asm.h"


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
	.globl asm_throw_and_handle_exception
	.globl asm_throw_and_handle_nat_exception
	.globl asm_throw_and_handle_arithmetic_exception
	.globl asm_throw_and_handle_arrayindexoutofbounds_exception
	.globl asm_handle_exception
	.globl asm_handle_nat_exception

	.globl asm_wrapper_patcher

	.globl asm_builtin_arraycheckcast
	.globl asm_builtin_aastore

	.globl asm_builtin_idiv
	.globl asm_builtin_irem
	.globl asm_builtin_ldiv
	.globl asm_builtin_lrem

	.globl asm_perform_threadswitch
	.globl asm_initialize_thread_stack
	.globl asm_switchstackandcall
	.globl asm_criticalsections
	.globl asm_getclassvalues_atomic
	.globl asm_prepare_native_stackinfo
	.globl asm_remove_native_stackinfo
	.globl asm_refillin_and_handle_exception


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

call_name:
	.ascii  "calljavafunction\0\0"

	.align  2 /*3*/
	.quad   0                         /* catch type all                       */
	.quad   calljava_xhandler         /* handler pc                           */
	.quad   calljava_xhandler         /* end pc                               */
	.quad   asm_calljavafunction      /* start pc                             */
	.long   1                         /* extable size                         */
	.long	0			  /* PADDING */
	.quad	0 			  /* line number table start		 */
	.quad	0 			  /* line number table size		*/
	.long	0			  /* PADDING */
	.long	0 			  /* fltsave */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   32                        /* frame size                           */
	.quad   0                         /* method pointer (pointer to name)     */

asm_calljavafunction:
asm_calljavafunction_int:
	ldgp    gp,0(pv)
	lda     sp,-32(sp)                /* allocate stack space                 */
	stq     gp,24(sp)                 /* save global pointer                  */
	stq     ra,0(sp)                  /* save return address                  */

	stq     a0,16(sp)                 /* save method pointer for compiler     */
	lda     v0,16(sp)                 /* pass pointer to method pointer via v0*/

	mov     a1,a0                     /* pass the remaining parameters        */
	mov     a2,a1
	mov     a3,a2
	mov     a4,a3

	lda     itmp2,asm_call_jit_compiler/* fake virtual function call (2 instr)*/
	stq     itmp2,8(sp)               /* store function address               */
	mov     sp,itmp2                  /* set method pointer                   */

	ldq     pv,8(itmp2)               /* method call as in Java               */
	jmp     ra,(pv)                   /* call JIT compiler                    */
calljava_jit:
	lda     pv,(asm_calljavafunction - calljava_jit)(ra)

calljava_return:
	ldq     ra,0(sp)                  /* restore return address               */
	ldq     gp,24(sp)                 /* restore global pointer               */
	lda     sp,32(sp)                 /* free stack space                     */

calljava_ret:
	jmp     zero,(ra)

calljava_xhandler:
	ldq     gp,24(sp)                 /* restore global pointer               */
	mov     xptr,a0
	jsr     ra,builtin_throw_exception
	ldq     ra,0(sp)                  /* restore return address               */
	lda     sp,32(sp)                 /* free stack space                     */
	mov     zero,v0                   /* return NULL                          */
	jmp     zero,(ra)
	.end    asm_calljavafunction




	.ent    asm_calljavafunction2

call_name2:
	.align  3

	.quad   0                         /* catch type all                       */
	.quad   calljava_xhandler2        /* handler pc                           */
	.quad   calljava_xhandler2        /* end pc                               */
	.quad   asm_calljavafunction2     /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* PADDING                              */
	.quad   0                         /* line number table start              */
	.quad   0                         /* line number table size               */
	.long   0                         /* PADDING                              */
	.long   0                         /* fltsave                              */
	.long   1                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   40                        /* frame size                           */
	.quad   0                         /* method pointer (pointer to name)     */

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
	lda     sp,-14*8(sp)          /* reserve stack space                      */

	SAVE_ARGUMENT_REGISTERS(0)    /* save 6 int/6 float argument registers    */

	stq     $28,12*8(sp)          /* save method pointer                      */
	stq     ra,13*8(sp)           /* save return address                      */

	ldq     a0,0(v0)              /* pass 'methodinfo' pointer to             */
	jsr     ra,jit_compile        /* jit compiler                             */
	ldgp    gp,0(ra)

	RESTORE_ARGUMENT_REGISTERS(0) /* restore 6 int/6 float argument registers */

	ldq     $28,12*8(sp)          /* load method pointer                      */
	ldq     ra,13*8(sp)           /* load return address                      */
	lda     sp,14*8(sp)           /* deallocate stack area                    */

	beq     v0,asm_call_jit_compiler_exception

	ldl     t8,-8(ra)             /* load instruction LDQ PV,xxx($yy)         */
	sll     t8,48,t8
	sra     t8,48,t8              /* isolate offset                           */

	addq    t8,$28,t8             /* compute update address via method pointer*/
	stq     v0,0(t8)              /* save new method address there            */

	call_pal PAL_imb              /* synchronise instruction cache            */

	mov     v0,pv                 /* load method address into pv              */
	jmp     zero,(pv)             /* and call method. The method returns      */
	                              /* directly to the caller (ra).             */

asm_call_jit_compiler_exception:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	subq    sp,1*8,sp
	stq     ra,0*8(sp)
	jsr     ra,builtin_asm_get_exceptionptrptr
	ldq     ra,0*8(sp)
	addq    sp,1*8,sp
#else
	lda     v0,_exceptionptr
#endif
	ldq     xptr,0(v0)            /* get the exception pointer                */
	stq     zero,0(v0)            /* clear the exception pointer              */

	subq    ra,4,xpc
	br      asm_handle_nat_exception

	.end    asm_call_jit_compiler


/**************** function asm_refillin_and_handle_exception *******************
*                                                                              *
*   This function handles an exception. It does not use the usual calling      *
*   conventions. The exception is passed in REG_ITMP1 and the                  *
*   pc from the exception raising position is passed in REG_ITMP2.             *
*   a0 contains the PV of the function causing the problem                     *
*                                                                              *
*   void asm_handle_arithmetic_exception (exceptionclass, exceptionpc);        *
*                                                                              *
*******************************************************************************/
	.ent asm_refillin_and_handle_exception
asm_refillin_and_handle_exception:
	ldgp	gp,0(pv)
        ldl     t0,0(ra)              /* load instruction LDA PV,xxx(RA)          */
        sll     t0,48,t0
        sra     t0,48,t0              /* isolate offset                           */
        addq    t0,ra,pv              /* compute update address                   */
        ldl     t0,4(ra)              /* load instruction LDAH PV,xxx(PV)         */
        srl     t0,16,t0              /* isolate instruction code                 */
        lda     t0,-0x177b(t0)        /* test for LDAH                            */
        bne     t0, asm_refillin_and_handle_exception_cont
        ldl     t0,4(ra)              /* load instruction LDAH PV,xxx(PV)         */
        sll     t0,16,t0              /* compute high offset                      */
        addl    t0,0,t0               /* sign extend high offset                  */
        addq    t0,pv,pv              /* compute update address                   */

asm_refillin_and_handle_exception_cont:

	mov	sp,t0
	lda	sp,-6*8(sp) /* prepare stackframe*/
	stq	pv,5*8(sp) /* store pv of caller */
	stq	xptr,4*8(sp) /*exception ptr*/
	stq	xpc,3*8(sp) /*address of failure*/
	stq	t0,2*8(sp) /*begin of java stack frame*/
	stq	pv,1*8(sp) /* store pv of caller */
	stq	zero,0*8(sp)	/*builtin (invisible) function */
	jsr     ra,asm_prepare_native_stackinfo /* puts 2 additional quadwords on stack */
	ldgp	gp,0(ra)

	ldq	a2,utf_void__java_lang_Throwable
	ldq	a1,utf_fillInStackTrace
	ldq	t0,6*8(sp)
	ldq	t1,offobjvftbl(t0)	
	ldq	a0,offclass(t1)
	jsr	ra,class_resolvemethod
	ldgp	gp,0(ra)
	/* now we have the method */

	/*refillin */
	mov	v0,a0
	ldq	a1,6*8(sp)
	jsr	ra,asm_calljavafunction
	ldgp	gp,0(ra)

	/*remove frame*/
	jsr ra,asm_remove_native_stackinfo
	ldgp	gp,0(ra)


	/*finish*/
	ldq	xpc,0(sp)	
	ldq	xptr,1*8(sp)
        ldq	pv,2*8(sp)
	lda sp,3*8(sp)
	br asm_handle_exception

	.end asm_refillin_and_handle_exception

/****** function asm_throw_and_handle_arrayindexoutofbounds_exception **********
*                                                                              *
*   This function handles an exception. It does not use the usual calling      *
*   conventions. The integer parameter is passed in REG_ITMP1 and the          *
*   pc from the exception raising position is passed in REG_ITMP2.             *
*                                                                              *
*   void asm_handle_arithmetic_exception (exceptionclass, exceptionpc);        *
*                                                                              *
*******************************************************************************/

	.ent asm_throw_and_handle_arrayindexoutofbounds_exception

asm_throw_and_handle_arrayindexoutofbounds_exception:
	ldgp	gp,0(pv)

	ldl     t0,0(ra)              /* load instruction LDA PV,xxx(RA)          */
	sll     t0,48,t0
	sra     t0,48,t0              /* isolate offset                           */
	addq    t0,ra,pv              /* compute update address                   */
	ldl     t0,4(ra)              /* load instruction LDAH PV,xxx(PV)         */
	srl     t0,16,t0              /* isolate instruction code                 */
	lda     t0,-0x177b(t0)        /* test for LDAH                            */
	bne     t0,asm_throw_and_handle_arrayindexoutofbounds_exception_cont
	ldl     t0,4(ra)              /* load instruction LDAH PV,xxx(PV)         */
	sll     t0,16,t0              /* compute high offset                      */
	addl    t0,0,t0               /* sign extend high offset                  */
	addq    t0,pv,pv              /* compute update address                   */

asm_throw_and_handle_arrayindexoutofbounds_exception_cont:
	mov     sp,t0
	lda     sp,-6*8(sp) /*prepare stackframe*/
	stq     pv,5*8(sp)  /*pv of failure*/
	stq     itmp1,4*8(sp) /*int parameter  of the exception*/
	stq     xpc,3*8(sp) /*address of failure */
	stq     t0,2*8(sp) /*store begin of java stack frame*/
	stq     pv,1*8(sp) /*store pv of caller in structure*/
	stq     zero,0*8(sp) /*builtin (invisible function)*/

	jsr     ra,asm_prepare_native_stackinfo /* puts 2 additional quadwords on stack */
	ldgp    gp,0(ra)

	ldq     a0,6*8(sp)  /*int  of exception*/
	jsr     ra,new_arrayindexoutofboundsexception
	ldgp	gp,0(ra)

	mov     v0,itmp1 /*itmp1 is not touched in asm_remove_native_stackinfo*/

	jsr     ra,asm_remove_native_stackinfo
	ldgp    gp,0(ra)

	ldq     itmp2,0(sp)	
	ldq     pv,2*8(sp)
	lda     sp,3*8(sp)
	br      asm_handle_exception

	.end asm_throw_and_handle_arrayindexoutofbounds_exception


/* asm_throw_and_handle_arithmetic_exception ***********************************

   DOCUMENT ME!

*******************************************************************************/

	.ent asm_throw_and_handle_arithmetic_exception

asm_throw_and_handle_arithmetic_exception:
	ldl     t0,0(ra)              /* load instruction LDA PV,xxx(RA)          */
	sll     t0,48,t0
	sra     t0,48,t0              /* isolate offset                           */
	addq    t0,ra,pv              /* compute update address                   */
	ldl     t0,4(ra)              /* load instruction LDAH PV,xxx(PV)         */
	srl     t0,16,t0              /* isolate instruction code                 */
	lda     t0,-0x177b(t0)        /* test for LDAH                            */
	bne     t0,asm_throw_and_handle_arithmetic_exception_cont
	ldl     t0,4(ra)              /* load instruction LDAH PV,xxx(PV)         */
	sll     t0,16,t0              /* compute high offset                      */
	addl    t0,0,t0               /* sign extend high offset                  */
	addq    t0,pv,pv              /* compute update address                   */

asm_throw_and_handle_arithmetic_exception_cont:
	mov	sp,t0
	lda	sp,-6*8(sp) /*prepare stackframe*/
        stq     pv,5*8(sp)  /*pv of failure*/
	stq 	itmp1,4*8(sp) /*exception string of the exception*/
	stq 	xpc,3*8(sp) /*address of failure */
	stq	t0,2*8(sp) /*store begin of java stack frame*/
	stq	pv,1*8(sp) /*store pv of caller in structure*/
	stq	zero,0*8(sp) /*builtin (invisible function)*/
	jsr	ra,asm_prepare_native_stackinfo /* puts 2 additional quadwords on stack */
	ldgp	gp,0(ra)

	jsr     ra,new_arithmeticexception
	ldgp	gp,0(ra)
	mov     v0,itmp1 /*itmp1 is not touched in asm_remove_native_stackinfo*/

	jsr ra,asm_remove_native_stackinfo
	ldgp	gp,0(ra)

	ldq itmp2,0(sp)	
        ldq    pv,2*8(sp)
	lda sp,3*8(sp)
	br asm_handle_exception

	.end asm_throw_and_handle_arithmetic_exception


/* asm_throw_and_handle_exception **********************************************

   DOCUMENT ME!!!

*******************************************************************************/

	.ent asm_throw_and_handle_nat_exception

asm_throw_and_handle_nat_exception:
	ldgp	gp,0(pv)
	ldl     t0,0(ra)              /* load instruction LDA PV,xxx(RA)          */
	sll     t0,48,t0
	sra     t0,48,t0              /* isolate offset                           */
	addq    t0,ra,pv              /* compute update address                   */
	ldl     t0,4(ra)              /* load instruction LDAH PV,xxx(PV)         */
	srl     t0,16,t0              /* isolate instruction code                 */
	lda     t0,-0x177b(t0)        /* test for LDAH                            */
	bne     t0,asm_throw_and_handle_exception
	ldl     t0,4(ra)              /* load instruction LDAH PV,xxx(PV)         */
	sll     t0,16,t0              /* compute high offset                      */
	addl    t0,0,t0               /* sign extend high offset                  */
	addq    t0,pv,pv              /* compute update address                   */

	.aent asm_throw_and_handle_exception

asm_throw_and_handle_exception:
	mov     sp,t0
	lda     sp,-6*8(sp)           /* prepare stackframe                       */
	stq     pv,5*8(sp)            /* pv of failure                            */
	stq     xptr,4*8(sp)          /* classname of the exception               */
	stq     xpc,3*8(sp)           /* address of failure                       */
	stq     t0,2*8(sp)            /* store begin of java stack frame          */
	stq     pv,1*8(sp)            /* store pv of caller in structure          */
	stq     zero,0*8(sp)          /* builtin (invisible function)             */
	                              /* puts 2 additional quadwords on stack     */

	br      ra,L_asm_throw_and_handle_exception_load_gp
L_asm_throw_and_handle_exception_load_gp:
	ldgp    gp,0(ra)              /* load gp (it's not set correctly in jit)  */

	jsr     ra,asm_prepare_native_stackinfo
	ldgp	gp,0(ra)

	ldq     a0,6*8(sp)            /* classname of exception                   */
	jsr     ra,new_exception
	ldgp    gp,0(ra)

	mov     v0,xptr               /* xptr (itmp1) is not touched in           */
	                              /* asm_remove_native_stackinfo              */

	jsr     ra,asm_remove_native_stackinfo
	ldgp	gp,0(ra)

	ldq     itmp2,0(sp)
	ldq     pv,2*8(sp)
	lda     sp,3*8(sp)
	br      asm_handle_exception

	.end asm_throw_and_handle_nat_exception


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
	lda     sp,-18*8(sp)          /* allocate stack                           */
	stq     t0,0*8(sp)            /* save possible used registers             */
	stq     t1,1*8(sp)            /* also registers used by trace_exception   */
	stq     t2,2*8(sp)
	stq     t3,3*8(sp)
	stq     t4,4*8(sp)
	stq     t5,5*8(sp)
	stq     t6,6*8(sp)
	stq     t7,7*8(sp)
	stq     t8,8*8(sp)
	stq     t9,9*8(sp)
	stq     t10,10*8(sp)
	stq     v0,11*8(sp)
	stq     a0,12*8(sp)
	stq     a1,13*8(sp)
	stq     a2,14*8(sp)
	stq     a3,15*8(sp)
	stq     a4,16*8(sp)
	stq     a5,17*8(sp)

	lda     t3,1(zero)            /* set no unwind flag                       */
ex_stack_loop:
	lda     sp,-5*8(sp)           /* allocate stack                           */
	stq     xptr,0*8(sp)          /* save used register                       */
	stq     xpc,1*8(sp)
	stq     pv,2*8(sp)
	stq     ra,3*8(sp)
	stq     t3,4*8(sp)

	mov     xptr,a0
	ldq     a1,MethodPointer(pv)
	mov     xpc,a2
/*	mov     t3,a3 */
	lda		a3,0(zero)
	lda		a4,1(zero)
	br      ra,ex_trace           /* set ra for gp loading                    */
ex_trace:
	ldgp    gp,0(ra)              /* load gp                                  */
	jsr     ra,builtin_trace_exception /* trace_exception(xptr,methodptr)     */
	
	ldq     xptr,0*8(sp)          /* restore used register                    */
	ldq     xpc,1*8(sp)
	ldq     pv,2*8(sp)
	ldq     ra,3*8(sp)
	ldq     t3,4*8(sp)
	lda     sp,5*8(sp)            /* deallocate stack                         */
	
	ldl     t0,ExTableSize(pv)    /* t0 = exception table size                */
	beq     t0,empty_table        /* if empty table skip                      */

	lda     t1,ExTableStart(pv)   /* t1 = start of exception table            */

ex_table_loop:
	ldq     t2,ExStartPC(t1)      /* t2 = exception start pc                  */
	cmple   t2,xpc,t2             /* t2 = (startpc <= xpc)                    */
	beq     t2,ex_table_cont      /* if (false) continue                      */
	ldq     t2,ExEndPC(t1)        /* t2 = exception end pc                    */
	cmplt   xpc,t2,t2             /* t2 = (xpc < endpc)                       */
	beq     t2,ex_table_cont      /* if (false) continue                      */
	ldq     a1,ExCatchType(t1)    /* arg1 = exception catch type              */
	beq     a1,ex_handle_it       /* NULL catches everything                  */

	ldl     itmp3,offclassloaded(a1)
	bne     itmp3,L_class_loaded

	subq    sp,8*8,sp             /* allocate stack                           */
	stq     t0,0*8(sp)            /* save used register                       */
	stq     t1,1*8(sp)
	stq     t3,2*8(sp)
	stq     xptr,3*8(sp)
	stq     xpc,4*8(sp)
	stq     pv,5*8(sp)
	stq     ra,6*8(sp)
	stq     a1,7*8(sp)

	mov     a1,a0

	br      ra,L_class_load_ra    /* set ra for gp loading                    */
L_class_load_ra:
	ldgp    gp,0(ra)              /* load gp                                  */
	jsr     ra,load_class_bootstrap

	ldq     t0,0*8(sp)            /* restore used register                    */
	ldq     t1,1*8(sp)
	ldq     t3,2*8(sp)
	ldq     xptr,3*8(sp)
	ldq     xpc,4*8(sp)
	ldq     pv,5*8(sp)
	ldq     ra,6*8(sp)
	ldq     a1,7*8(sp)
	addq    sp,8*8,sp             /* deallocate stack                         */

L_class_loaded:
	ldl     itmp3,offclasslinked(a1)
	subq    sp,8*8,sp             /* allocate stack                           */
	stq     a1,7*8(sp)
	bne     itmp3,L_class_linked

	stq     t0,0*8(sp)            /* save used register                       */
	stq     t1,1*8(sp)
	stq     t3,2*8(sp)
	stq     xptr,3*8(sp)
	stq     xpc,4*8(sp)
	stq     pv,5*8(sp)
	stq     ra,6*8(sp)

	mov     a1,a0

	br      ra,L_class_link_ra    /* set ra for gp loading                    */
L_class_link_ra:
	ldgp    gp,0(ra)              /* load gp                                  */
	jsr     ra,link_class

	ldq     t0,0*8(sp)            /* restore used register                    */
	ldq     t1,1*8(sp)
	ldq     t3,2*8(sp)
	ldq     xptr,3*8(sp)
	ldq     xpc,4*8(sp)
	ldq     pv,5*8(sp)
	ldq     ra,6*8(sp)

L_class_linked:
_crit_restart1:
	ldq     a1,7*8(sp)
_crit_begin1:
	ldq     a0,offobjvftbl(xptr)  /* a0 = vftblptr(xptr)                      */
	ldq     a1,offclassvftbl(a1)  /* a1 = vftblptr(catchtype) class (not obj) */
	ldl     a0,offbaseval(a0)     /* a0 = baseval(xptr)                       */
	ldl     v0,offbaseval(a1)     /* a2 = baseval(catchtype)                  */
	ldl     a1,offdiffval(a1)     /* a1 = diffval(catchtype)                  */
_crit_end1:
	subl    a0,v0,a0              /* a0 = baseval(xptr) - baseval(catchtype)  */
	cmpule  a0,a1,v0              /* v0 = xptr is instanceof catchtype        */
	addq    sp,8*8,sp             /* deallocate stack                         */
	beq     v0,ex_table_cont      /* if (false) continue                      */

ex_handle_it:
	ldq     xpc,ExHandlerPC(t1)   /* xpc = exception handler pc               */

	beq     t3,ex_jump            /* if (!(no stack unwinding) skip           */

	ldq     t0,0*8(sp)            /* restore possible used registers          */
	ldq     t1,1*8(sp)            /* also registers used by trace_exception   */
	ldq     t2,2*8(sp)
	ldq     t3,3*8(sp)
	ldq     t4,4*8(sp)
	ldq     t5,5*8(sp)
	ldq     t6,6*8(sp)
	ldq     t7,7*8(sp)
	ldq     t8,8*8(sp)
	ldq     t9,9*8(sp)
	ldq     t10,10*8(sp)
	ldq     v0,11*8(sp)
	ldq     a0,12*8(sp)
	ldq     a1,13*8(sp)
	ldq     a2,14*8(sp)
	ldq     a3,15*8(sp)
	ldq     a4,16*8(sp)
	ldq     a5,17*8(sp)
	lda     sp,18*8(sp)           /* deallocate stack                         */

ex_jump:
	jmp     zero,(xpc)            /* jump to the handler                      */

ex_table_cont:
	lda     t1,ExEntrySize(t1)    /* next exception table entry               */
	subl    t0,1,t0               /* decrement entry counter                  */
	bgt     t0,ex_table_loop      /* if (t0 > 0) next entry                   */

empty_table:
	beq     t3,ex_already_cleared /* if here the first time, then             */
	lda     sp,18*8(sp)           /* deallocate stack and                     */
	clr     t3                    /* clear the no unwind flag                 */
ex_already_cleared:
	ldl     t0,IsSync(pv)         /* t0 = SyncOffset                          */
	beq     t0,no_monitor_exit    /* if zero no monitorexit                   */

#if defined(USE_THREADS)
	addq    sp,t0,t0              /* add stackptr to Offset                   */
	ldq     a0,-8(t0)             /* load monitorexit pointer                 */

	lda     sp,-7*8(sp)           /* allocate stack                           */
	stq     t0,0*8(sp)            /* save used register                       */
	stq     t1,1*8(sp)
	stq     t3,2*8(sp)
	stq     xptr,3*8(sp)
	stq     xpc,4*8(sp)
	stq     pv,5*8(sp)
	stq     ra,6*8(sp)

	br      ra,ex_mon_load        /* set ra for gp loading                    */
ex_mon_load:
	ldgp    gp,0(ra)              /* load gp                                  */
	jsr     ra,builtin_monitorexit/* builtin_monitorexit(objectptr)           */
	
	ldq     t0,0*8(sp)            /* restore used register                    */
	ldq     t1,1*8(sp)
	ldq     t3,2*8(sp)
	ldq     xptr,3*8(sp)
	ldq     xpc,4*8(sp)
	ldq     pv,5*8(sp)
	ldq     ra,6*8(sp)
	lda     sp,7*8(sp)            /* deallocate stack                         */
#endif

no_monitor_exit:
	ldl     t0,FrameSize(pv)      /* t0 = frame size                          */
	addq    sp,t0,sp              /* unwind stack                             */
	mov     sp,t0                 /* t0 = pointer to save area                */
	ldl     t1,IsLeaf(pv)         /* t1 = is leaf procedure                   */
	bne     t1,ex_no_restore      /* if (leaf) skip                           */
	ldq     ra,-8(t0)             /* restore ra                               */
	lda     t0,-8(t0)             /* t0--                                     */
ex_no_restore:
	mov     ra,xpc                /* the new xpc is ra                        */
	ldl     t1,IntSave(pv)        /* t1 = saved int register count            */
	br      t2,ex_int1            /* t2 = current pc                          */
ex_int1:
	lda     t2,44(t2)             /* lda t2,ex_int2-ex_int1(t2) !!!!!!!!!!!!! */
	negl    t1,t1                 /* negate register count                    */
	s4addq  t1,t2,t2              /* t2 = ex_int_sav - 4 * register count     */
	jmp     zero,(t2)             /* jump to save position                    */
	ldq     s0,-56(t0)
	ldq     s1,-48(t0)
	ldq     s2,-40(t0)
	ldq     s3,-32(t0)
	ldq     s4,-24(t0)
	ldq     s5,-16(t0)
	ldq     s6,-8(t0)
ex_int2:
	s8addq  t1,t0,t0              /* t0 = t0 - 8 * register count             */

	ldl     t1,FltSave(pv)        /* t1 = saved flt register count            */
	br      t2,ex_flt1            /* t2 = current pc                          */
ex_flt1:
	lda     t2,48(t2)             /* lda t2,ex_flt2-ex_flt1(t2) !!!!!!!!!!!!! */
	negl    t1,t1                 /* negate register count                    */
	s4addq  t1,t2,t2              /* t2 = ex_flt_sav - 4 * register count     */
	jmp     zero,(t2)             /* jump to save position                    */
	ldt     fs0,-64(t0)
	ldt     fs1,-56(t0)
	ldt     fs2,-48(t0)
	ldt     fs3,-40(t0)
	ldt     fs4,-32(t0)
	ldt     fs5,-24(t0)
	ldt     fs6,-16(t0)
	ldt     fs7,-8(t0)
ex_flt2:
	ldl     t0,0(ra)              /* load instruction LDA PV,xxx(RA)          */
	sll     t0,48,t0
	sra     t0,48,t0              /* isolate offset                           */
	addq    t0,ra,pv              /* compute update address                   */
	ldl     t0,4(ra)              /* load instruction LDAH PV,xxx(PV)         */
	srl     t0,16,t0              /* isolate instruction code                 */
	lda     t0,-0x177b(t0)        /* test for LDAH                            */
	bne     t0,ex_stack_loop       
	ldl     t0,4(ra)              /* load instruction LDAH PV,xxx(RA)         */
	sll     t0,16,t0              /* compute high offset                      */
	addl    t0,0,t0               /* sign extend high offset                  */
	addq    t0,pv,pv              /* compute update address                   */
	br      ex_stack_loop

	.end    asm_handle_nat_exception


/* asm_wrapper_patcher *********************************************************

   XXX

   Stack layout:
	 32   return address into JIT code (patch position)
     24   pointer to virtual java_objectheader
     16   machine code (which is patched back later)
      8   unresolved class/method/field reference
      0   patcher function pointer to call

   ATTENTION: itmp3 == gp! But we don't need gp do call the patcher function.

*******************************************************************************/
		
	.ent    asm_wrapper_patcher

asm_wrapper_patcher:
	lda     sp,-(12+27+4)*8(sp)   /* create stack frame                       */

	SAVE_ARGUMENT_REGISTERS(0)    /* save 6 int/6 float argument registers    */
	SAVE_TEMPORARY_REGISTERS(12)  /* save 11 int/16 float temporary registers */

	stq     itmp1,(12+27+0)*8(sp) /* save itmp1                               */
	stq     itmp2,(12+27+1)*8(sp) /* save itmp2                               */
	stq     ra,(12+27+2)*8(sp)    /* save method return address (for leafs)   */
	stq     pv,(12+27+3)*8(sp)    /* save pv of calling java function         */

	lda     a0,(1+12+27+4)*8(sp)  /* pass sp, skip patcher function pointer   */
	ldq     pv,(0+12+27+4)*8(sp)  /* get function pointer                     */
	jmp     ra,(pv)               /* call the patcher function                */
	ldgp    gp,0(ra)
		
	RESTORE_ARGUMENT_REGISTERS(0) /* restore 6 int/6 float argument registers */
	RESTORE_TEMPORARY_REGISTERS(12)/* restore 11 integer temporary registers  */

	ldq     itmp1,(12+27+0)*8(sp) /* restore itmp1                            */
	ldq     itmp2,(12+27+1)*8(sp) /* restore itmp2                            */
	ldq     ra,(12+27+2)*8(sp)    /* restore method return address (for leafs)*/
	ldq     pv,(12+27+3)*8(sp)    /* restore pv of calling java function      */

	ldq     itmp3,(4+12+27+4)*8(sp)/* get return address (into JIT code)      */
	lda     sp,(5+12+27+4)*8(sp)  /* remove stack frame                       */

	beq     v0,L_asm_wrapper_patcher_exception

	jmp     zero,(itmp3)          /* jump to new patched code                 */

L_asm_wrapper_patcher_exception:
	mov     itmp3,xpc             /* return address into JIT code is xpc      */

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

		
/************************ function asm_builtin_idiv ****************************
*                                                                              *
*   Does null check and calls idiv or throws an exception                      *
*                                                                              *
*******************************************************************************/

	.ent    asm_builtin_idiv

asm_builtin_idiv:
	ldgp    gp,0(pv)
	lda     pv,builtin_idiv
	beq     a1,nb_idiv                /* if (null) throw exception            */
	jmp     zero,(pv)                 /* else call builtin_idiv               */

nb_idiv:
	ldq	xptr,string_java_lang_ArithmeticException_message
	lda     xpc,-4(ra)                /* faulting address is return adress - 4*/
	br	asm_throw_and_handle_arithmetic_exception

	.end    asm_builtin_idiv


/************************ function asm_builtin_ldiv ****************************
*                                                                              *
*   Does null check and calls ldiv or throws an exception                      *
*                                                                              *
*******************************************************************************/

	.ent    asm_builtin_ldiv

asm_builtin_ldiv:
	ldgp    gp,0(pv)
	lda     pv,builtin_ldiv
	beq     a1,nb_ldiv                /* if (null) throw exception            */
	jmp     zero,(pv)                 /* else call builtin_ldiv               */

nb_ldiv:
	ldq	xptr,string_java_lang_ArithmeticException_message
	lda     xpc,-4(ra)                /* faulting address is return adress - 4*/
	br	asm_throw_and_handle_arithmetic_exception

	.end    asm_builtin_ldiv


/************************ function asm_builtin_irem ****************************
*                                                                              *
*   Does null check and calls irem or throws an exception                      *
*                                                                              *
*******************************************************************************/

	.ent    asm_builtin_irem

asm_builtin_irem:
	ldgp    gp,0(pv)
	lda     pv,builtin_irem
	beq     a1,nb_irem                /* if (null) throw exception            */
	jmp     zero,(pv)                 /* else call builtin_irem               */

nb_irem:
	ldq	xptr,string_java_lang_ArithmeticException_message
	lda     xpc,-4(ra)                /* faulting address is return adress - 4*/
	br	asm_throw_and_handle_arithmetic_exception

	.end    asm_builtin_irem


/************************ function asm_builtin_lrem ****************************
*                                                                              *
*   Does null check and calls lrem or throws an exception                      *
*                                                                              *
*******************************************************************************/

	.ent    asm_builtin_lrem

asm_builtin_lrem:
	ldgp    gp,0(pv)
	lda     pv,builtin_lrem
	beq     a1,nb_lrem                /* if (null) throw exception            */
	jmp     zero,(pv)                 /* else call builtin_lrem               */

nb_lrem:
	ldq	xptr,string_java_lang_ArithmeticException_message
	lda     xpc,-4(ra)                /* faulting address is return adress - 4*/
	br	asm_throw_and_handle_arithmetic_exception

	.end    asm_builtin_lrem


/* asm_builtin_arraycheckcast **************************************************

   Does the cast check and eventually throws an exception.

*******************************************************************************/

	.ent    asm_builtin_arraycheckcast

asm_builtin_arraycheckcast:
	ldgp    gp,0(pv)
	lda     sp,-16(sp)                /* allocate stack space                 */
	stq     ra,0(sp)                  /* save return address                  */
	stq     a0,8(sp)                  /* save object pointer                  */
	jsr     ra,builtin_arraycheckcast
	ldgp    gp,0(ra)
	beq     v0,nb_carray_throw        /* if (false) throw exception           */
	ldq     ra,0(sp)                  /* restore return address               */
	ldq     v0,8(sp)                  /* return object pointer                */
	lda     sp,16(sp)                 /* free stack space                     */
	jmp     zero,(ra)

nb_carray_throw:
	ldq	ra,0(sp)
	lda	sp,16(sp)
	lda	xpc,-4(ra)
	ldq	xptr,string_java_lang_ClassCastException
	jmp	zero,asm_throw_and_handle_nat_exception
#if 0
	ldq     a0,string_java_lang_ClassCastException
    jsr     ra,new_exception
    ldgp    gp,0(ra)
	mov     v0,xptr

	ldq     ra,0(sp)                  /* restore return address               */
	lda     sp,16(sp)                 /* free stack space                     */
	lda     xpc,-4(ra)                /* faulting address is return adress - 4*/
	br      asm_handle_nat_exception
#endif

	.end    asm_builtin_arraycheckcast


/******************* function asm_builtin_aastore ******************************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/

	.ent    asm_builtin_aastore

asm_builtin_aastore:
	ldgp    gp,0(pv)
	beq     a0,nb_aastore_null        /* if null pointer throw exception      */
	ldl     t0,offarraysize(a0)       /* load size                            */
	lda     sp,-24(sp)                /* allocate stack space                 */
	stq     ra,0(sp)                  /* save return address                  */
	s8addq  a1,a0,t1                  /* add index*8 to arrayref              */
	cmpult  a1,t0,t0                  /* do bound check                       */
	beq     t0,nb_aastore_bound       /* if out of bounds throw exception     */
	mov     a2,a1                     /* object is second argument            */
	stq     t1,8(sp)                  /* save store position                  */
	stq     a1,16(sp)                 /* save object                          */
	jsr     ra,builtin_canstore       /* builtin_canstore(arrayref,object)    */
	ldgp    gp,0(ra)
	ldq     ra,0(sp)                  /* restore return address               */
	ldq     a0,8(sp)                  /* restore store position               */
	ldq     a1,16(sp)                 /* restore object                       */
	lda     sp,24(sp)                 /* free stack space                     */
	beq     v0,nb_aastore_throw       /* if (false) throw exception           */
	stq     a1,offobjarrdata(a0)      /* store objectptr in array             */
	jmp     zero,(ra)

nb_aastore_null:
	ldq	xptr,string_java_lang_NullPointerException
	mov	ra,xpc
	jmp	zero,asm_throw_and_handle_nat_exception
#if 0	
	subq    sp,8,sp                   /* allocate stack space                 */
	stq     ra,0(sp)                  /* save return address                  */
    jsr     ra,new_nullpointerexception
    ldgp    gp,0(ra)
	mov     v0,xptr
	ldq     ra,0(sp)
	addq    sp,8,sp

	mov     ra,xpc                    /* faulting address is return adress    */
	br      asm_handle_nat_exception
#endif
nb_aastore_bound:
	ldq	ra,0(sp)
	lda	sp,24(sp)
	mov	ra,xpc
	mov	a1,xptr
	jmp	zero,asm_throw_and_handle_arrayindexoutofbounds_exception
#if 0
	ldq     a0,string_java_lang_ArrayIndexOutOfBoundsException
    jsr     ra,new_exception_int      /* a1 already contains the index        */
    ldgp    gp,0(ra)
	mov     v0,xptr

	ldq     ra,0(sp)                  /* restore return address               */
	lda     sp,24(sp)                 /* free stack space                     */
	mov     ra,xpc                    /* faulting address is return adress    */
	br      asm_handle_nat_exception
#endif
nb_aastore_throw:
	mov	ra,xpc
	ldq	xptr,string_java_lang_ArrayStoreException
	jmp	zero,asm_throw_and_handle_nat_exception
#if 0
	subq    sp,8,sp                   /* allocate stack space                 */
	stq     ra,0(sp)                  /* save return address                  */
    jsr     ra,new_arraystoreexception
    ldgp    gp,0(ra)
	mov     v0,xptr
	ldq     ra,0(sp)
	addq    sp,8,sp

	mov     ra,xpc                    /* faulting address is return adress    */
	br      asm_handle_nat_exception
#endif
	.end    asm_builtin_aastore


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
_crit_restart2:
_crit_begin2:
	ldl     t0,offbaseval(a0)
	ldl     t1,offdiffval(a0)
	ldl     t2,offbaseval(a1)
_crit_end2:
	stl     t0,offcast_super_baseval(a2)
	stl     t1,offcast_super_diffval(a2)
	stl     t2,offcast_sub_baseval(a2)
	jmp      zero,(ra)

	.end    asm_getclassvalues_atomic

    .data

asm_criticalsections:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
    .quad   _crit_begin1
    .quad   _crit_end1
    .quad   _crit_restart1
    .quad   _crit_begin2
    .quad   _crit_end2
    .quad   _crit_restart2
#endif
    .quad   0


	.ent asm_prepare_native_stackinfo
asm_prepare_native_stackinfo:
	lda sp,-24(sp)
	stq ra,0(sp)
	jsr ra,builtin_asm_get_stackframeinfo
	stq v0,16(sp)
	ldq t0,0(v0)
	stq t0,8(sp)
	ldq ra,0(sp)
	lda sp,8(sp)
	stq sp,0(v0)
	ret
	.end asm_prepare_native_stackinfo

	.ent asm_remove_native_stackinfo
asm_remove_native_stackinfo:
	ldq t0,0(sp)
	ldq t1,8(sp)
	stq t0,0(t1)
	lda sp,40(sp)
	ret
	.end asm_remove_native_stackinfo

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
