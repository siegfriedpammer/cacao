/* src/vm/jit/mips/asmpart.S - Java-C interface functions for mips

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

   Changes: Christian Thalinger

   $Id: asmpart.S 2205 2005-04-03 22:51:43Z twisti $

*/


#include "config.h"
#include "vm/jit/mips/offsets.h"
#include "vm/jit/mips/asmoffsets.h"


#define zero    $0
#define itmp1   $1
#define v0      $2
#define itmp2   $3
#define a0      $4
#define a1      $5
#define a2      $6
#define a3      $7

#define a4      $8
#define	a5      $9
#define	a6      $10
#define	a7      $11
#define	t0      $12
#define	t1      $13
#define	t2      $14
#define	t3      $15

#define s0      $16
#define s1      $17
#define s2      $18
#define s3      $19
#define s4      $20
#define s5      $21
#define s6      $22
#define s7      $23

#define t8      $24
#define itmp3   $25
#define k0      $26
#define k1      $27

#define gp      $28
#define sp      $29
#define s8      $30
#define ra      $31

#define pv      s8
#define t9      itmp3

#define xptr    itmp1
#define xpc     itmp2
#define mptr    itmp3
#define mptrreg 25

#define fv0     $f0
#define ft0     $f1
#define ft1     $f2
#define ft2     $f3
#define ft3     $f4
#define ft4     $f5
#define ft5     $f6
#define ft6     $f7

#define ft7     $f8
#define	ft8     $f9
#define	ft9     $f10
#define	ft10    $f11
#define	fa0     $f12
#define	fa1     $f13
#define	fa2     $f14
#define	fa3     $f15

#define fa4     $f16
#define fa5     $f17
#define fa6     $f18
#define fa7     $f19
#define ft11    $f20
#define ft12    $f21
#define ft13    $f22
#define ft14    $f23

#define fs0     $f24
#define ft15    $f25
#define fs1     $f26
#define ft16    $f27
#define fs2     $f28
#define ft17    $f29
#define fs3     $f30
#define ft18    $f31

#define fss0    $f20
#define fss1    $f22
#define fss2    $f25
#define fss3    $f27
#define fss4    $f29
#define fss5    $f31

#define aaddu   daddu
#define asubu   dsubu
#define aaddiu  daddiu
#define ald     ld
#define ast     sd
#define ala     dla
#define asll    dsll
#define ashift  3


	.text
	.set    noat


/********************* exported functions and variables ***********************/

	.globl asm_calljavafunction
	.globl asm_calljavafunction_int

	.globl asm_calljavafunction2
	.globl asm_calljavafunction2int
	.globl asm_calljavafunction2long
	.globl asm_calljavafunction2float
	.globl asm_calljavafunction2double

	.globl asm_call_jit_compiler
	.globl asm_dumpregistersandcall
	.globl asm_handle_exception
	.globl asm_handle_nat_exception
	.globl asm_check_clinit
	.globl asm_builtin_checkarraycast
	.globl asm_builtin_checkcast
	.globl asm_builtin_aastore

#if defined(USE_THREADS)
	.globl asm_builtin_monitorenter
	.globl asm_builtin_monitorexit
#endif

	.globl asm_builtin_idiv
	.globl asm_builtin_irem
	.globl asm_builtin_ldiv
	.globl asm_builtin_lrem
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

	.dword  0                         /* catch type all                       */
	.dword  calljava_xhandler         /* handler pc                           */
	.dword  calljava_xhandler         /* end pc                               */
	.dword  asm_calljavafunction      /* start pc                             */
	.word   1                         /* extable size                         */
	.word   0                         /* fltsave                              */
	.word   0                         /* intsave                              */
	.word   0                         /* isleaf                               */
	.word   0                         /* IsSync                               */
	.word   10*8                      /* frame size                           */
	.dword  0                         /* method pointer (pointer to name)     */

asm_calljavafunction:
asm_calljavafunction_int:
	aaddiu  sp,sp,-10*8               /* allocate stack space                 */
	sd      ra,0(sp)                  /* save return address                  */

	.set    noreorder
	bal     call_java_pc
	sd      pv,3*8(sp)                /* procedure vector                     */
call_java_pc:
	aaddiu  pv,ra,-4*4

	.set    reorder
	
	sdc1    fss0,4*8(sp)              /* save non JavaABI saved flt registers */
	sdc1    fss1,5*8(sp)
	sdc1    fss2,6*8(sp)
	sdc1    fss3,7*8(sp)
	sdc1    fss4,8*8(sp)
	sdc1    fss5,9*8(sp)
	sd      a0,2*8(sp)                /* save method pointer for compiler     */
	aaddiu  itmp1,sp,16               /* pass pointer to methodptr via itmp1  */

	move    a0,a1                     /* pass the remaining parameters        */
	move    a1,a2
	move    a2,a3
	move    a3,a4

	ala     mptr,asm_call_jit_compiler/* fake virtual function call (2 instr) */
	ast     mptr,1*8(sp)              /* store function address               */
	move    mptr,sp                   /* set method pointer                   */

	.set    noreorder

	ald     pv,1*8(mptr)              /* method call as in Java               */
	jalr    pv                        /* call JIT compiler                    */
	nop
	aaddiu  pv,ra,-23*4               /* recompute procedure vector           */

	.set    reorder

calljava_return:
	ld      ra,0(sp)                  /* restore return address               */
	ld      pv,3*8(sp)                /* restore procedure vector             */

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

	.dword  0                         /* catch type all                       */
	.dword  calljava_xhandler2        /* handler pc                           */
	.dword  calljava_xhandler2        /* end pc                               */
	.dword  asm_calljavafunction2     /* start pc                             */
	.word   1                         /* extable size                         */
	.word   0                         /* fltsave                              */
	.word   1                         /* intsave                              */
	.word   0                         /* isleaf                               */
	.word   0                         /* IsSync                               */
	.word   12*8                      /* frame size                           */
	.dword  0                         /* method pointer (pointer to name)     */

asm_calljavafunction2:
asm_calljavafunction2int:
asm_calljavafunction2long:
asm_calljavafunction2float:
asm_calljavafunction2double:
	aaddiu  sp,sp,-12*8               /* allocate stack space (only 11 needed)*/
	sd      ra,0(sp)                  /* save return address                  */

	.set    noreorder
	bal     call_java_pc2
	sd      pv,1*8(sp)                /* procedure vector                     */
call_java_pc2:
	aaddiu  pv,ra,-4*4
	sd      s7,3*8(sp)

	.set    reorder
	
	sdc1    fss0,5*8(sp)              /* save non JavaABI saved flt registers */
	sdc1    fss1,6*8(sp)
	sdc1    fss2,7*8(sp)
	sdc1    fss3,8*8(sp)
	sdc1    fss4,9*8(sp)
	sdc1    fss5,10*8(sp)
	sd      a0,4*8(sp)                /* save method pointer for compiler     */
	move    t0,a3
	move    s7,a1

	blez    s7,calljava_argsloaded
	ald     a0,offjniitem(t0)
	ldc1    fa0,offjniitem(t0)
	daddi   s7,s7,-1
	blez    s7,calljava_argsloaded

	ald     a1,offjniitem+sizejniblock*1(t0)
	ldc1    fa1,offjniitem+sizejniblock*1(t0)
	daddi   s7,s7,-1
	blez    s7,calljava_argsloaded

	ald     a2,offjniitem+sizejniblock*2(t0)
	ldc1    fa2,offjniitem+sizejniblock*2(t0)
	daddi   s7,s7,-1
	blez    s7,calljava_argsloaded

	ald     a3,offjniitem+sizejniblock*3(t0)
	ldc1    fa3,offjniitem+sizejniblock*3(t0)
	daddi   s7,s7,-1
	blez    s7,calljava_argsloaded

	ald     a4,offjniitem+sizejniblock*4(t0)
	ldc1    fa4,offjniitem+sizejniblock*4(t0)
	daddi   s7,s7,-1
	blez    s7,calljava_argsloaded

	ald     a5,offjniitem+sizejniblock*5(t0)
	ldc1    fa5,offjniitem+sizejniblock*5(t0)
	daddi   s7,s7,-1
	blez    s7,calljava_argsloaded

	ald     a6,offjniitem+sizejniblock*6(t0)
	ldc1    fa6,offjniitem+sizejniblock*6(t0)
	daddi   s7,s7,-1
	blez    s7,calljava_argsloaded

	ald     a7,offjniitem+sizejniblock*7(t0)
	ldc1    fa7,offjniitem+sizejniblock*7(t0)
	daddi   s7,s7,-1
		
calljava_argsloaded:
    move    t8,sp
	blez    s7,calljava_nocopy
	subu    t1,zero,s7
	sll     t2,t1,3
	daddu   sp,sp,t2
	daddu   t2,t2,t8

calljava_copyloop:
    ald     t3,offjniitem+sizejniblock*8(t0)
	ast     t3,0(t2)
	ala     t1,1(t1)
	ala     t0,sizejniblock(t0)
	ala     t2,8(t2)
	bnez    t1,calljava_copyloop

calljava_nocopy:
	ala     itmp1,32(t8)              /* pass pointer to methodptr via itmp1  */

	ala     mptr,asm_call_jit_compiler/* fake virtual function call (2 instr) */
	ast     mptr,16(sp)               /* store function address               */
	ala     mptr,8(t8)                /* set method pointer                   */

	.set    noreorder

	ald     pv,8(mptr)                /* method call as in Java               */
	jalr    pv                        /* call JIT compiler                    */
	nop
	aaddiu  pv,ra,-76*4               /* recompute procedure vector           */

	.set    reorder

calljava_return2:
	ld      ra,0(sp)                  /* restore return address               */
	ld      pv,8(sp)                  /* restore procedure vector             */
	ld      s7,24(sp)

	ldc1    fss0,5*8(sp)              /* restore non JavaABI saved flt regs   */
	ldc1    fss1,6*8(sp)
	ldc1    fss2,7*8(sp)
	ldc1    fss3,8*8(sp)
	ldc1    fss4,9*8(sp)
	ldc1    fss5,10*8(sp)
	aaddiu  sp,sp,12*8                /* free stack space                     */
	j       ra                        /* return                               */

calljava_xhandler2:
    sll     s7,s7,3
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
	lw      t0,-12(ra)            /* load instruction LD PV,xxx($y)           */
	srl     t0,t0,21              /* shift right register number $y           */
	and     t0,t0,31              /* isolate register number                  */
	addiu   t0,t0,-mptrreg        /* test for REG_METHODPTR                   */
	beqz    t0,noregchange       

	lw      t0,0(ra)              /* load instruction LDA PV,xxx(RA)          */
	sll     t0,t0,16
	sra     t0,t0,16              /* isolate offset                           */
	aaddu   mptr,t0,ra            /* compute update address                   */

noregchange:
	aaddiu  sp,sp,-18*8           /* allocate stack space                     */
	sd      a0,0*8(sp)            /* save all argument registers              */
	sd      a1,1*8(sp)            /* they could be used by method             */
	sd      a2,2*8(sp)
	sd      a3,3*8(sp)
	sd      a4,4*8(sp)
	sd      a5,5*8(sp)
	sd      a6,6*8(sp)
	sd      a7,7*8(sp)
	sdc1    fa0,8*8(sp)
	sdc1    fa1,9*8(sp)
	sdc1    fa2,10*8(sp)
	sdc1    fa3,11*8(sp)
	sdc1    fa4,12*8(sp)
	sdc1    fa5,13*8(sp)
	sdc1    fa6,14*8(sp)
	sdc1    fa7,15*8(sp)
	sd      mptr,16*8(sp)         /* save method pointer                      */
	sd      ra,17*8(sp)           /* save return address                      */

	ald     a0,0(itmp1)           /* pass 'methodinfo' pointer to             */
	jal     jit_compile           /* jit compiler                             */

	ld      a0,0*8(sp)            /* restore argument registers               */
	ld      a1,1*8(sp)            /* they could be used by method             */
	ld      a2,2*8(sp)
	ld      a3,3*8(sp)
	ld      a4,4*8(sp)
	ld      a5,5*8(sp)
	ld      a6,6*8(sp)
	ld      a7,7*8(sp)
	ldc1    fa0,8*8(sp)
	ldc1    fa1,9*8(sp)
	ldc1    fa2,10*8(sp)
	ldc1    fa3,11*8(sp)
	ldc1    fa4,12*8(sp)
	ldc1    fa5,13*8(sp)
	ldc1    fa6,14*8(sp)
	ldc1    fa7,15*8(sp)
	ld      mptr,16*8(sp)         /* restore method pointer                   */
	ld      ra,17*8(sp)           /* restore return address                   */
	aaddiu  sp,sp,18*8            /* deallocate stack area                    */

	lw      t0,-12(ra)            /* load instruction LDQ PV,xxx($yy)         */
	sll     t0,t0,16
	sra     t0,t0,16              /* isolate offset                           */

	aaddu   t0,t0,mptr            /* compute update address via method pointer*/
	ast     v0,0(t0)              /* save new method address there            */

	move    pv,v0                 /* move method address into pv              */

	jr      pv                    /* and call method. The method returns      */
	                              /* directly to the caller (ra).             */

	.end    asm_call_jit_compiler


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
	lw      t0,0(ra)              /* load instruction LDA PV,xxx(RA)          */
	sll     t0,t0,16
	sra     t0,t0,16              /* isolate offset                           */
	aaddu   pv,t0,ra              /* compute update address                   */

	.aent    asm_handle_exception

asm_handle_exception:
	aaddiu  sp,sp,-14*8           /* allocate stack                           */
	sd      v0,0*8(sp)            /* save possible used registers             */
	sd      t0,1*8(sp)            /* also registers used by trace_exception   */
	sd      t1,2*8(sp)
	sd      t2,3*8(sp)
	sd      t3,4*8(sp)
	sd      t8,5*8(sp)
	sd      a0,6*8(sp)
	sd      a1,7*8(sp)
	sd      a2,8*8(sp)
	sd      a3,9*8(sp)
	sd      a4,10*8(sp)
	sd      a5,11*8(sp)
	sd      a6,12*8(sp)
	sd      a7,13*8(sp)

	addu    t3,zero,1             /* set no unwind flag                       */
ex_stack_loop:
	aaddiu  sp,sp,-6*8            /* allocate stack                           */
	sd      xptr,0*8(sp)          /* save used registers                      */
	sd      xpc,1*8(sp)
	sd      pv,2*8(sp)
	sd      ra,3*8(sp)
	sd      t3,4*8(sp)

	move    a0,xptr
	ald     a1,MethodPointer(pv)
	move    a2,xpc
/*	move    a3,t3 */
	move    a3,zero
	addu    a4,zero,1
	jal     builtin_trace_exception /* trace_exception(xptr,methodptr)        */
	
	ld      xptr,0*8(sp)          /* restore used register                    */
	ld      xpc,1*8(sp)
	ld      pv,2*8(sp)
	ld      ra,3*8(sp)
	ld      t3,4*8(sp)
	aaddiu  sp,sp,6*8             /* deallocate stack                         */
	
	lw      t0,ExTableSize(pv)    /* t0 = exception table size                */
	beqz    t0,empty_table        /* if empty table skip                      */
	aaddiu  t1,pv,ExTableStart    /* t1 = start of exception table            */

ex_table_loop:
	ald     t2,ExStartPC(t1)      /* t2 = exception start pc                  */
	sle     t2,t2,xpc             /* t2 = (startpc <= xpc)                    */
	beqz    t2,ex_table_cont      /* if (false) continue                      */
	ald     t2,ExEndPC(t1)        /* t2 = exception end pc                    */
	slt     t2,xpc,t2             /* t2 = (xpc < endpc)                       */
	beqz    t2,ex_table_cont      /* if (false) continue                      */
	ald     a1,ExCatchType(t1)    /* arg1 = exception catch type              */
	beqz    a1,ex_handle_it       /* NULL catches everything                  */

	lw      itmp3,offclassloaded(a1)
	bnez    itmp3,L_class_loaded

	aaddiu  sp,sp,-8*8            /* allocate stack                           */
	sd      t0,0*8(sp)            /* save used register                       */
	sd      t1,1*8(sp)
	sd      t3,2*8(sp)
	sd      xptr,3*8(sp)
	sd      xpc,4*8(sp)
	sd      pv,5*8(sp)
	sd      ra,6*8(sp)
	sd      a1,7*8(sp)
		
	move    a0,a1
	jal     load_class_bootstrap
		
	ld      t0,0*8(sp)            /* restore used register                    */
	ld      t1,1*8(sp)
	ld      t3,2*8(sp)
	ld      xptr,3*8(sp)
	ld      xpc,4*8(sp)
	ld      pv,5*8(sp)
	ld      ra,6*8(sp)
	ld      a1,7*8(sp)
	aaddiu  sp,sp,8*8             /* deallocate stack                         */
	
L_class_loaded:
	lw      itmp3,offclasslinked(a1)
	aaddiu  sp,sp,-8*8            /* allocate stack                           */
	sd      a1,7*8(sp)
	bnez    itmp3,L_class_linked

	sd      t0,0*8(sp)            /* save used register                       */
	sd      t1,1*8(sp)
	sd      t3,2*8(sp)
	sd      xptr,3*8(sp)
	sd      xpc,4*8(sp)
	sd      pv,5*8(sp)
	sd      ra,6*8(sp)
		
	move    a0,a1
	jal     link_class
		
	ld      t0,0*8(sp)            /* restore used register                    */
	ld      t1,1*8(sp)
	ld      t3,2*8(sp)
	ld      xptr,3*8(sp)
	ld      xpc,4*8(sp)
	ld      pv,5*8(sp)
	ld      ra,6*8(sp)

L_class_linked:
_crit_restart1:
	ld      a1,7*8(sp)
_crit_begin1:
	ald     a0,offobjvftbl(xptr)  /* a0 = vftblptr(xptr)                      */
	ald     a1,offclassvftbl(a1)  /* a1 = vftblptr(catchtype) class (not obj) */
	lw      a0,offbaseval(a0)     /* a0 = baseval(xptr)                       */
	lw      v0,offbaseval(a1)     /* a2 = baseval(catchtype)                  */
	lw      a1,offdiffval(a1)     /* a1 = diffval(catchtype)                  */
_crit_end1:
	subu    a0,a0,v0              /* a0 = baseval(xptr) - baseval(catchtype)  */
	sltu    v0,a1,a0              /* v0 = xptr is instanceof catchtype        */
	aaddiu  sp,sp,8*8             /* deallocate stack                         */
	bnez    v0,ex_table_cont      /* if (false) continue                      */

ex_handle_it:
	ald     xpc,ExHandlerPC(t1)   /* xpc = exception handler pc               */

	beqz    t3,ex_jump            /* if (!(no stack unwinding) skip           */

	ld      v0,0*8(sp)            /* restore possible used registers          */
	ld      t0,1*8(sp)            /* also registers used by trace_exception   */
	ld      t1,2*8(sp)
	ld      t2,3*8(sp)
	ld      t3,4*8(sp)
	ld      t8,5*8(sp)
	ld      a0,6*8(sp)
	ld      a1,7*8(sp)
	ld      a2,8*8(sp)
	ld      a3,9*8(sp)
	ld      a4,10*8(sp)
	ld      a5,11*8(sp)
	ld      a6,12*8(sp)
	ld      a7,13*8(sp)
	
	aaddiu  sp,sp,14*8            /* deallocate stack                         */

ex_jump:
	jr      xpc                   /* jump to the handler                      */

ex_table_cont:
	aaddiu  t1,t1,ExEntrySize     /* next exception table entry               */
	addiu   t0,t0,-1              /* decrement entry counter                  */
	bgtz    t0,ex_table_loop      /* if (t0 > 0) next entry                   */

empty_table:
	beqz    t3,ex_already_cleared /* if here the first time, then             */
	aaddiu  sp,sp,14*8            /* deallocate stack and                     */
	move    t3,zero               /* clear the no unwind flag                 */
ex_already_cleared:
	lw      t0,IsSync(pv)         /* t0 = SyncOffset                          */
	beqz    t0,no_monitor_exit    /* if zero no monitorexit                   */

#if defined(USE_THREADS)
	aaddu   t0,sp,t0              /* add stackptr to Offset                   */
	ald     a0,-8(t0)             /* load monitorexit pointer                 */

	aaddiu  sp,sp,-8*8            /* allocate stack                           */
	sd      t0,0*8(sp)            /* save used register                       */
	sd      t1,1*8(sp)
	sd      t3,2*8(sp)
	sd      xptr,3*8(sp)
	sd      xpc,4*8(sp)
	sd      pv,5*8(sp)
	sd      ra,6*8(sp)

	jal     builtin_monitorexit   /* builtin_monitorexit(objectptr)           */
	
	ld      t0,0*8(sp)            /* restore used register                    */
	ld      t1,1*8(sp)
	ld      t3,2*8(sp)
	ld      xptr,3*8(sp)
	ld      xpc,4*8(sp)
	ld      pv,5*8(sp)
	ld      ra,6*8(sp)
	aaddiu  sp,sp,8*8             /* deallocate stack                         */
#endif

no_monitor_exit:
	lw      t0,FrameSize(pv)      /* t0 = frame size                          */
	aaddu   sp,sp,t0              /* unwind stack                             */
	move    t0,sp                 /* t0 = pointer to save area                */
	lw      t1,IsLeaf(pv)         /* t1 = is leaf procedure                   */
	bnez    t1,ex_no_restore      /* if (leaf) skip                           */
	ld      ra,-8(t0)             /* restore ra                               */
	aaddiu  t0,t0,-8              /* t0--                                     */
ex_no_restore:
	move    xpc,ra                /* the new xpc is ra                        */
	lw      t1,IntSave(pv)        /* t1 = saved int register count            */
	ala     t2,ex_int2            /* t2 = current pc                          */
	sll     t1,t1,2               /* t1 = register count * 4                  */
	asubu   t2,t2,t1              /* t2 = ex_int_sav - 4 * register count     */
	jr      t2                    /* jump to save position                    */
	ld      s0,-8*8(t0)
	ld      s1,-7*8(t0)
	ld      s2,-6*8(t0)
	ld      s3,-5*8(t0)
	ld      s4,-4*8(t0)
	ld      s5,-3*8(t0)
	ld      s6,-2*8(t0)
	ld      s7,-1*8(t0)
ex_int2:
	sll     t1,t1,1               /* t1 = register count * 4 * 2              */
	asubu   t0,t0,t1              /* t0 = t0 - 8 * register count             */

	lw      t1,FltSave(pv)        /* t1 = saved flt register count            */
	ala     t2,ex_flt2            /* t2 = current pc                          */
	sll     t1,t1,2               /* t1 = register count * 4                  */
	asubu   t2,t2,t1              /* t2 = ex_int_sav - 4 * register count     */
	jr      t2                    /* jump to save position                    */
	ldc1    fs0,-4*8(t0)
	ldc1    fs1,-3*8(t0)
	ldc1    fs2,-2*8(t0)
	ldc1    fs3,-1*8(t0)
ex_flt2:
	lw      t0,0(ra)              /* load instruction LDA PV,xxx(RA)          */
	sll     t0,t0,16
	sra     t0,t0,16              /* isolate offset                           */
	aaddu   pv,t0,ra              /* compute update address                   */
	b       ex_stack_loop

	.end    asm_handle_nat_exception


/********************* asm_check_clinit ****************************************
*                                                                              *
*   Checks if class is initialized. If not, do it right now.                   *
*                                                                              *
*******************************************************************************/
		
    .ent    asm_check_clinit

asm_check_clinit:
	daddiu  sp,sp,-24*8

	sd      ra,0*8(sp)            /* save return address                      */

	sd      a0,1*8(sp)            /* save argument registers for leaf funcs   */
	sd      a1,2*8(sp)
	sd      a2,3*8(sp)
	sd      a3,4*8(sp)
	sd      a4,5*8(sp)
	sd      a5,6*8(sp)
	sd      a6,7*8(sp)
	sd      a7,8*8(sp)

	sd      t0,9*8(sp)
	sd      t1,10*8(sp)
	sd      t2,11*8(sp)
	sd      t3,12*8(sp)
	sd      t8,13*8(sp)

	sdc1    fa0,14*8(sp)
	sdc1    fa1,15*8(sp)
	sdc1    fa2,16*8(sp)
	sdc1    fa3,17*8(sp)
	sdc1    fa4,18*8(sp)
	sdc1    fa5,19*8(sp)
	sdc1    fa6,20*8(sp)
	sdc1    fa7,21*8(sp)

	sd      itmp2,22*8(sp)        /* save machine code                        */

	                              /* check if class is initialized            */
	lw      itmp3,offclassinit(itmp1)
	bnez    itmp3,L_is_initialized
	
	move    a0,itmp1              /* move class pointer to a0                 */
	jal     initialize_class
		
	beqz    v0,L_initializererror

L_is_initialized:
	ld      ra,0*8(sp)            /* get return address                       */
	ld      itmp1,22*8(sp)        /* get machine code                         */

	daddiu  ra,ra,-2*4            /* go back 2 instructions (jal + nop delay) */
	sw      itmp1,0(ra)           /* patch first instruction                  */
	dsrl32  itmp1,itmp1,0         /* get high 32 bit                          */
	sw      itmp1,4(ra)           /* patch second instruction                 */

	move    a0,ra                 /* start of flush area                      */
	addiu   a1,zero,2*4           /* 2 instruction words long                 */
	jal     docacheflush          /* flush!                                   */
	
	ld      ra,0*8(sp)            /* restore return address                   */

	ld      a0,1*8(sp)            /* restore argument registers               */
	ld      a1,2*8(sp)
	ld      a2,3*8(sp)
	ld      a3,4*8(sp)
	ld      a4,5*8(sp)
	ld      a5,6*8(sp)
	ld      a6,7*8(sp)
	ld      a7,8*8(sp)

	ld      t0,9*8(sp)
	ld      t1,10*8(sp)
	ld      t2,11*8(sp)
	ld      t3,12*8(sp)
	ld      t8,13*8(sp)

	ldc1    fa0,14*8(sp)
	ldc1    fa1,15*8(sp)
	ldc1    fa2,16*8(sp)
	ldc1    fa3,17*8(sp)
	ldc1    fa4,18*8(sp)
	ldc1    fa5,19*8(sp)
	ldc1    fa6,20*8(sp)
	ldc1    fa7,21*8(sp)

	daddiu  sp,sp,24*8

	daddiu  ra,ra,-2*4            /* go back 2 instructions (jal + nop delay) */
	jr      ra

L_initializererror:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	jal     builtin_asm_get_exceptionptrptr
#else
	la      v0,_exceptionptr
#endif
	ld      xptr,0(v0)            /* get the exception pointer                */
	sd      zero,0(v0)            /* clear the exception pointer              */

	ld      ra,0*8(sp)            /* restore return address                   */

	ld      a0,1*8(sp)            /* restore argument registers               */
	ld      a1,2*8(sp)
	ld      a2,3*8(sp)
	ld      a3,4*8(sp)
	ld      a4,5*8(sp)
	ld      a5,6*8(sp)
	ld      a6,7*8(sp)
	ld      a7,8*8(sp)

	ld      t0,9*8(sp)
	ld      t1,10*8(sp)
	ld      t2,11*8(sp)
	ld      t3,12*8(sp)
	ld      t8,13*8(sp)

	ldc1    fa0,14*8(sp)
	ldc1    fa1,15*8(sp)
	ldc1    fa2,16*8(sp)
	ldc1    fa3,17*8(sp)
	ldc1    fa4,18*8(sp)
	ldc1    fa5,19*8(sp)
	ldc1    fa6,20*8(sp)
	ldc1    fa7,21*8(sp)

	daddiu  sp,sp,24*8

	aaddiu  xpc,ra,-4             /* faulting address is return adress - 4    */
	b       asm_handle_exception

	.end	asm_check_clinit

		
/********************* function asm_builtin_monitorenter ***********************
*                                                                              *
*   Does null check and calls monitorenter or throws an exception              *
*                                                                              *
*******************************************************************************/

#if defined(USE_THREADS)
	.ent    asm_builtin_monitorenter

asm_builtin_monitorenter:
	beqz    a0,nb_monitorenter        /* if (null) throw exception            */
	ala     t9,builtin_monitorenter   /* else call builtin_monitorenter       */
	j       t9

nb_monitorenter:
	daddiu  sp,sp,-2*8
	sd      ra,0*8(sp)
	ald     a0,string_java_lang_NullPointerException
	jal     new_exception
	move    xptr,v0
	ld      ra,0*8(sp)
	daddiu  sp,sp,2*8

	aaddiu  xpc,ra,-4                 /* faulting address is return adress - 4*/
	b       asm_handle_nat_exception

	.end    asm_builtin_monitorenter
#endif


/********************* function asm_builtin_monitorexit ************************
*                                                                              *
*   Does null check and calls monitorexit or throws an exception               *
*                                                                              *
*******************************************************************************/

#if defined(USE_THREADS)
	.ent    asm_builtin_monitorexit

asm_builtin_monitorexit:
	beqz    a0,nb_monitorexit         /* if (null) throw exception            */
	ala     t9,builtin_monitorexit    /* else call builtin_monitorexit        */
	j       t9

nb_monitorexit:
	daddiu  sp,sp,-2*8
	sd      ra,0*8(sp)
	jal     new_nullpointerexception
	move    xptr,v0
	ld      ra,0*8(sp)
	daddiu  sp,sp,2*8

	aaddiu  xpc,ra,-4                 /* faulting address is return adress - 4*/
	b       asm_handle_nat_exception

	.end    asm_builtin_monitorexit
#endif


/************************ function asm_builtin_idiv ****************************
*                                                                              *
*   Does null check and calls idiv or throws an exception                      *
*                                                                              *
*******************************************************************************/

	.ent    asm_builtin_idiv

asm_builtin_idiv:
	beqz    a1,nb_idiv                /* if (null) throw exception            */
	ala     itmp3,builtin_idiv        /* else call builtin_idiv               */
	j       itmp3

nb_idiv:
	daddiu  sp,sp,-2*8
	sd      ra,0*8(sp)
	jal     new_arithmeticexception
	move    xptr,v0
	ld      ra,0*8(sp)
	daddiu  sp,sp,2*8

	aaddiu  xpc,ra,-4                 /* faulting address is return adress - 4*/
	b       asm_handle_nat_exception

	.end    asm_builtin_idiv


/************************ function asm_builtin_ldiv ****************************
*                                                                              *
*   Does null check and calls ldiv or throws an exception                      *
*                                                                              *
*******************************************************************************/

	.ent    asm_builtin_ldiv

asm_builtin_ldiv:
	beqz    a1,nb_ldiv                /* if (null) throw exception            */
	ala     itmp3,builtin_ldiv        /* else call builtin_ldiv               */
	j       itmp3

nb_ldiv:
	daddiu  sp,sp,-2*8
	sd      ra,0*8(sp)
	jal     new_arithmeticexception
	move    xptr,v0
	ld      ra,0*8(sp)
	daddiu  sp,sp,2*8

	aaddiu  xpc,ra,-4                 /* faulting address is return adress - 4*/
	b       asm_handle_nat_exception

	.end    asm_builtin_ldiv


/************************ function asm_builtin_irem ****************************
*                                                                              *
*   Does null check and calls irem or throws an exception                      *
*                                                                              *
*******************************************************************************/

	.ent    asm_builtin_irem

asm_builtin_irem:
	beqz    a1,nb_irem                /* if (null) throw exception            */
	ala     t9,builtin_irem           /* else call builtin_irem               */
	j       t9

nb_irem:
	daddiu  sp,sp,-2*8
	sd      ra,0*8(sp)
	jal     new_arithmeticexception
	move    xptr,v0
	ld      ra,0*8(sp)
	daddiu  sp,sp,2*8

	aaddiu  xpc,ra,-4                 /* faulting address is return adress - 4*/
	b       asm_handle_nat_exception

	.end    asm_builtin_irem


/************************ function asm_builtin_lrem ****************************
*                                                                              *
*   Does null check and calls lrem or throws an exception                      *
*                                                                              *
*******************************************************************************/

	.ent    asm_builtin_lrem

asm_builtin_lrem:
	beqz    a1,nb_lrem                /* if (null) throw exception            */
	ala     t9,builtin_lrem           /* else call builtin_lrem               */
	j       t9

nb_lrem:
	daddiu  sp,sp,-2*8
	sd      ra,0*8(sp)
	jal     new_arithmeticexception
	move    xptr,v0
	ld      ra,0*8(sp)
	daddiu  sp,sp,2*8

	aaddiu  xpc,ra,-4                 /* faulting address is return adress - 4*/
	b       asm_handle_nat_exception

	.end    asm_builtin_lrem


/******************* function asm_builtin_checkarraycast ***********************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/

	.ent    asm_builtin_checkarraycast

asm_builtin_checkarraycast:
	aaddiu  sp,sp,-16                 /* allocate stack space                 */
	sd      ra,0(sp)                  /* save return address                  */
	sd      a0,8(sp)                  /* save object pointer                  */
	jal     builtin_checkarraycast    /* builtin_checkarraycast               */
	beqz    v0,nb_carray_throw        /* if (false) throw exception           */
	ld      ra,0(sp)                  /* restore return address               */
	ld      v0,8(sp)                  /* return object pointer                */
	aaddiu  sp,sp,16                  /* deallocate stack                     */
	j       ra                        /* return                               */

nb_carray_throw:
	jal     new_classcastexception
	move    xptr,v0

	ld      ra,0(sp)                  /* restore return address               */
	aaddiu  sp,sp,16                  /* free stack space                     */
	aaddiu  xpc,ra,-4                 /* faulting address is return adress - 4*/
	b       asm_handle_nat_exception

	.end    asm_builtin_checkarraycast


/********************* function asm_builtin_checkcast **************************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/

	.ent    asm_builtin_checkcast

asm_builtin_checkcast:
	aaddiu  sp,sp,-16                 /* allocate stack space                 */
	sd      ra,0(sp)                  /* save return address                  */
	sd      a0,8(sp)                  /* save object pointer                  */
	jal     builtin_checkcast         /* builtin_checkcast                    */
	beqz    v0,nb_ccast_throw         /* if (false) throw exception           */
	ld      ra,0(sp)                  /* restore return address               */
	ld      v0,8(sp)                  /* return object pointer                */
	aaddiu  sp,sp,16                  /* deallocate stack                     */
	j       ra                        /* return                               */

nb_ccast_throw:
	jal     new_classcastexception
	move    xptr,v0

	ld      ra,0(sp)                  /* restore return address               */
	aaddiu  sp,sp,16                  /* free stack space                     */
	aaddiu  xpc,ra,-4                 /* faulting address is return adress - 4*/
	b       asm_handle_nat_exception

	.end    asm_builtin_checkcast


/******************* function asm_builtin_aastore ******************************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*   a0 = arrayref, a1 = index, a2 = value                                      *
*                                                                              *
*******************************************************************************/

	.ent    asm_builtin_aastore

asm_builtin_aastore:
	beqz    a0,nb_aastore_null        /* if null pointer throw exception      */
	lw      t0,offarraysize(a0)       /* load size                            */
	aaddiu  sp,sp,-32                 /* allocate stack space                 */
	sd      ra,0(sp)                  /* save return address                  */
	asll    t1,a1,ashift              /* add index*8 to arrayref              */
	aaddu   t1,a0,t1                  /* add index * ashift to arrayref       */
	sltu    t0,a1,t0                  /* do bound check                       */
	beqz    t0,nb_aastore_bound       /* if out of bounds throw exception     */
	move    a1,a2                     /* object is second argument            */
	sd      t1,8(sp)                  /* save store position                  */
	sd      a1,16(sp)                 /* save object                          */
	jal     builtin_canstore          /* builtin_canstore(arrayref,object)    */
	ld      ra,0(sp)                  /* restore return address               */
	ld      a0,8(sp)                  /* restore store position               */
	ld      a1,16(sp)                 /* restore object                       */
	aaddiu  sp,sp,32                  /* free stack space                     */
	beqz    v0,nb_aastore_store       /* if (false) throw exception           */
	ast     a1,offobjarrdata(a0)      /* store objectptr in array             */
	j       ra                        /* return                               */

nb_aastore_null:
	daddiu  sp,sp,-2*8
	sd      ra,0*8(sp)
	jal     new_nullpointerexception
	move    xptr,v0
	ld      ra,0*8(sp)
	daddiu  sp,sp,2*8
	
	move    xpc,ra                    /* faulting address is return adress    */
	b       asm_handle_nat_exception

nb_aastore_bound:
	daddiu  sp,sp,-2*8
	sd      ra,0*8(sp)
	move    a0,a1                     /* move index into a0                   */
	jal     new_arrayindexoutofboundsexception
	move    xptr,v0
	ld      ra,0*8(sp)
	daddiu  sp,sp,2*8

	aaddiu  sp,sp,32                  /* free stack space                     */
	move    xpc,ra                    /* faulting address is return adress    */
	b       asm_handle_nat_exception

nb_aastore_store:
	daddiu  sp,sp,-2*8
	sd      ra,0*8(sp)
	jal     new_arraystoreexception
	move    xptr,v0
	ld      ra,0*8(sp)
	daddiu  sp,sp,2*8

	move    xpc,ra                    /* faulting address is return adress    */
	b       asm_handle_nat_exception

	.end    asm_builtin_aastore


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
_crit_restart2:
_crit_begin2:
	lw      t0,offbaseval(a0)
	lw      t1,offdiffval(a0)
	lw      t2,offbaseval(a1)
_crit_end2:
	sw      t0,offcast_super_baseval(a2)
	sw      t1,offcast_super_diffval(a2)
	sw      t2,offcast_sub_baseval(a2)
	j       ra

	.end    asm_getclassvalues_atomic

    .data

asm_criticalsections:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
    .dword  _crit_begin1
    .dword  _crit_end1
    .dword  _crit_restart1
    .dword  _crit_begin2
    .dword  _crit_end2
    .dword  _crit_restart2
#endif
    .dword  0


	.text

	.ent    compare_and_swap
compare_and_swap:
1:
	lld		v0,0(a0)
	bne		v0,a1,2f
	move    t0,a2
	scd		t0,0(a0)
	beqz	t0,1b
2:
	sync
	j		ra
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
