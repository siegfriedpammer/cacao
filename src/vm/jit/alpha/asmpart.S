/* jit/alpha/asmpart.S - Java-C interface functions for alpha

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   Institut f. Computersprachen, TU Wien
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser, M. Probst,
   S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich,
   J. Wenninger

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

   $Id: asmpart.S 1072 2004-05-19 17:20:12Z stefan $

*/


#include "config.h"        
#include "offsets.h"


#define    	MethodPointer   -8
#define    	FrameSize       -12
#define     IsSync          -16
#define     IsLeaf          -20
#define     IntSave         -24
#define     FltSave         -28
/* DEFINE LINE NUMBER STUFF HERE */
#define     ExTableSize     -56
#define     ExTableStart    -56

#define     ExEntrySize     -32
#define     ExStartPC       -8
#define     ExEndPC         -16
#define     ExHandlerPC     -24
#define     ExCatchType     -32


#define v0      $0

#define t0      $1
#define t1      $2
#define t2      $3
#define t3      $4
#define t4      $5
#define t5      $6
#define t6      $7
#define t7      $8

#define	s0      $9
#define	s1      $10
#define	s2      $11
#define	s3      $12
#define	s4      $13
#define	s5      $14
#define	s6      $15

#define a0      $16
#define a1      $17
#define a2      $18
#define a3      $19
#define a4      $20
#define a5      $21

#define t8      $22
#define t9      $23
#define t10     $24
#define t11     $25
#define ra      $26
#define t12     $27

#define pv      t12
#define AT      $at
#define gp      $29
#define sp      $30
#define zero    $31

#define itmp1   $25
#define itmp2   $28
#define itmp3   $29

#define xptr    itmp1
#define xpc     itmp2

#define sf0     $f2
#define sf1     $f3
#define sf2     $f4
#define sf3     $f5
#define sf4     $f6
#define sf5     $f7
#define sf6     $f8
#define sf7     $f9

#define fzero   $f31


#define PAL_imb 134

	.text
	.set    noat
	.set    noreorder


/********************* exported functions and variables ***********************/

	.globl has_no_x_instr_set
	.globl synchronize_caches
	.globl asm_calljavafunction
	.globl asm_calljavafunction2
	.globl asm_calljavafunction2double
	.globl asm_calljavafunction2long
	.globl asm_call_jit_compiler
	.globl asm_handle_exception
	.globl asm_handle_nat_exception
	.globl asm_check_clinit
	.globl asm_builtin_checkcast	
	.globl asm_builtin_checkarraycast
	.globl asm_builtin_aastore
	.globl asm_builtin_monitorenter
	.globl asm_builtin_monitorexit
	.globl asm_builtin_idiv
	.globl asm_builtin_irem
	.globl asm_builtin_ldiv
	.globl asm_builtin_lrem
	.globl asm_perform_threadswitch
	.globl asm_initialize_thread_stack
	.globl asm_switchstackandcall
	.globl asm_getcallingmethod
	.globl asm_builtin_trace
	.globl asm_builtin_exittrace
	.globl asm_criticalsections
	.globl asm_getclassvalues_atomic

/*************************** imported functions *******************************/

	.globl jit_compile
	.globl _exceptionptr
	.globl builtin_monitorexit
	.globl builtin_throw_exception
	.globl builtin_trace_exception
	.globl class_java_lang_Object


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

	lda     $28,asm_call_jit_compiler /* fake virtual function call (2 instr) */
	stq     $28,8(sp)                 /* store function address               */
	mov     sp,$28                    /* set method pointer                   */

	ldq     pv,8($28)                 /* method call as in Java               */
	jmp     ra,(pv)                   /* call JIT compiler                    */
calljava_jit:
	lda     pv,-64(ra)                /* asm_calljavafunction-calljava_jit !!!!!*/

calljava_return:
	ldq     ra,0(sp)                  /* restore return address               */
	ldq     gp,24(sp)                 /* restore global pointer               */
	lda     sp,32(sp)                 /* free stack space                     */

calljava_ret:
	jmp     zero,(ra)

calljava_xhandler:
	ldq     gp,24(sp)                 /* restore global pointer               */
	mov     itmp1,a0
	jsr     ra,builtin_throw_exception
	ldq     ra,0(sp)                  /* restore return address               */
	lda     sp,32(sp)                 /* free stack space                     */
	jmp     zero,(ra)
	.end    asm_calljavafunction




	.ent    asm_calljavafunction2

call_name2:
	.ascii  "calljavafunction2\0\0"

	.align  3
	.quad   0                         /* catch type all                       */
	.quad   calljava_xhandler2        /* handler pc                           */
	.quad   calljava_xhandler2        /* end pc                               */
	.quad   asm_calljavafunction2     /* start pc                             */
	.long   1                         /* extable size                         */
	.long	0			  /* PADDING */
	.quad	0 			  /* line number table start		 */
	.quad	0 			  /* line number table size		*/
	.long	0			  /* PADDING */
	.long   0                         /* fltsave                              */
	.long   1                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   40                        /* frame size                           */
	.quad   0                         /* method pointer (pointer to name)     */

asm_calljavafunction2:
asm_calljavafunction2double:
asm_calljavafunction2long:
	ldgp    gp,0(pv)
	lda     sp,-40(sp)                /* allocate stack space                 */
	stq     ra,0(sp)                  /* save return address                  */
	stq     s6,24(sp)
	stq     gp,8(sp)                  /* save global pointer                  */

	stq     a0,32(sp)                 /* save method pointer for compiler     */
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
	lda     v0,32(t4)                 /* pass pointer to method pointer via v0*/

	lda     $28,asm_call_jit_compiler /* fake virtual function call (2 instr) */
	stq     $28,16(t4)                /* store function address               */
	lda     $28,8(t4)                 /* set method pointer                   */

	ldq     pv,8($28)                 /* method call as in Java               */
	jmp     ra,(pv)                   /* call JIT compiler                    */
calljava_jit2:
	lda     pv,-200(ra)               /* asm_calljavafunction-calljava_jit !!!*/

	s8addq  s6,sp,sp
calljava_return2:
	ldq     ra,0(sp)                  /* restore return address               */
	ldq     gp,8(sp)                  /* restore global pointer               */
	ldq     s6,24(sp)
	lda     sp,40(sp)                 /* free stack space                     */

calljava_ret2:
	jmp     zero,(ra)

calljava_xhandler2:
	s8addq  s6,sp,sp
	ldq     gp,8(sp)                  /* restore global pointer               */
	mov     itmp1,a0
	jsr     ra,builtin_throw_exception
	ldq     ra,0(sp)                  /* restore return address               */
	ldq     s6,24(sp)
	lda     sp,40(sp)                 /* free stack space                     */
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
	stq     a0,0*8(sp)            /* save all argument registers              */
	stq     a1,1*8(sp)            /* they could be used by method             */
	stq     a2,2*8(sp)
	stq     a3,3*8(sp)
	stq     a4,4*8(sp)
	stq     a5,5*8(sp)
	stt     $f16,6*8(sp)
	stt     $f17,7*8(sp)
	stt     $f18,8*8(sp)
	stt     $f19,9*8(sp)
	stt     $f20,10*8(sp)
	stt     $f21,11*8(sp)
	stq     $28,12*8(sp)          /* save method pointer                      */
	stq     ra,13*8(sp)           /* save return address                      */

	ldq     a0,0(v0)              /* pass 'methodinfo' pointer to             */
	jsr     ra,jit_compile        /* jit compiler                             */
	ldgp    gp,0(ra)

	call_pal PAL_imb              /* synchronise instruction cache            */

	ldq     a0,0*8(sp)            /* load argument registers                  */
	ldq     a1,1*8(sp)
	ldq     a2,2*8(sp)
	ldq     a3,3*8(sp)
	ldq     a4,4*8(sp)
	ldq     a5,5*8(sp)
	ldt     $f16,6*8(sp)
	ldt     $f17,7*8(sp)
	ldt     $f18,8*8(sp)
	ldt     $f19,9*8(sp)
	ldt     $f20,10*8(sp)
	ldt     $f21,11*8(sp)
	ldq     $28,12*8(sp)          /* load method pointer                      */
	ldq     ra,13*8(sp)           /* load return address                      */
	lda     sp,14*8(sp)           /* deallocate stack area                    */

	ldl     t8,-8(ra)             /* load instruction LDQ PV,xxx($yy)         */
	sll     t8,48,t8
	sra     t8,48,t8              /* isolate offset                           */

	addq    t8,$28,t8             /* compute update address via method pointer*/
	stq     v0,0(t8)              /* save new method address there            */

	mov     v0,pv                 /* load method address into pv              */

	jmp     zero,(pv)             /* and call method. The method returns      */
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
	jsr     ra,class_load         /* class_load(exceptionclass)               */

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
	jsr     ra,class_link         /* class_load(exceptionclass)               */

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
	ldt     $f2,-64(t0)
	ldt     $f3,-56(t0)
	ldt     $f4,-48(t0)
	ldt     $f5,-40(t0)
	ldt     $f6,-32(t0)
	ldt     $f7,-24(t0)
	ldt     $f8,-16(t0)
	ldt     $f9,-8(t0)
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


/********************* asm_check_clinit ****************************************
*                                                                              *
*   Does null check and calls monitorenter or throws an exception              *
*                                                                              *
*******************************************************************************/
		
    .ent    asm_check_clinit

asm_check_clinit:
	ldgp	gp,0(pv)

	ldl		itmp2,offclassinit(itmp1)
	bne		itmp2,L_is_initialized
	
	subq	sp,7*8,sp
	stq		ra,0*8(sp)
	stq		a0,1*8(sp)            /* save argument registers for leaf         */
	stq		a1,2*8(sp)            /* functions and native stub                */
	stq		a2,3*8(sp)
	stq		a3,4*8(sp)
	stq		a4,5*8(sp)
	stq		a5,6*8(sp)
		
	mov		itmp1,a0
	jsr		ra,class_init
		
	ldgp	gp,0(ra)
	ldq		ra,0*8(sp)
	ldq		a0,1*8(sp)            /* restore argument registers               */
	ldq		a1,2*8(sp)
	ldq		a2,3*8(sp)
	ldq		a3,4*8(sp)
	ldq		a4,5*8(sp)
	ldq		a5,6*8(sp)
	addq	sp,7*8,sp

	beq     v0,L_initializererror

L_is_initialized:
	mov     ra,itmp1              /* now patch the calling code               */
	subq    itmp1,(3*4),itmp1     /* go back 3 instructions                   */
	ldah    itmp2,-15392(zero)    /* br -- 0xc3e0 == -15392                   */
	lda     itmp2,4(itmp2)        /* jump over 4 instructions                 */
	stl     itmp2,0(itmp1)        /* store the new branch: br +4              */

	jmp		zero,(ra)

L_initializererror:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	subq    sp,1*8,sp
	stq     ra,0*8(sp)
	jsr     ra,builtin_asm_get_exceptionptrptr
	ldq     ra,0*8(sp)
	addq    sp,1*8,sp
	ldq     xptr,0(v0)            /* get the exception pointer                */
	stq     zero,0(v0)            /* clear the exception pointer              */
#else
	lda     itmp3,_exceptionptr
	ldq     xptr,0(itmp3)
	stq     zero,0(itmp3)
#endif

	subq    ra,4,xpc
	br      asm_handle_nat_exception

	.end	asm_check_clinit

		
/********************* function asm_builtin_monitorenter ***********************
*                                                                              *
*   Does null check and calls monitorenter or throws an exception              *
*                                                                              *
*******************************************************************************/

	.ent    asm_builtin_monitorenter
asm_builtin_monitorenter:
	ldgp    gp,0(pv)
	lda     pv,builtin_monitorenter
	beq     a0,nb_monitorenter        /* if (null) throw exception            */
	jmp     zero,(pv)                 /* else call builtin_monitorenter       */

nb_monitorenter:
	subq    sp,8,sp
    stq     ra,0(sp)
	ldq     a0,string_java_lang_NullPointerException
    jsr     ra,new_exception_message
    ldgp    gp,0(ra)
	mov     v0,xptr
    ldq     ra,0(sp)
	addq    sp,8,sp
	
	lda     xpc,-4(ra)                /* faulting address is return adress - 4*/
	br      asm_handle_nat_exception
	.end    asm_builtin_monitorenter


/********************* function asm_builtin_monitorexit ************************
*                                                                              *
*   Does null check and calls monitorexit or throws an exception               *
*                                                                              *
*******************************************************************************/

	.ent    asm_builtin_monitorexit
asm_builtin_monitorexit:
	ldgp    gp,0(pv)
	lda     pv,builtin_monitorexit
	beq     a0,nb_monitorexit         /* if (null) throw exception            */
	jmp     zero,(pv)                 /* else call builtin_monitorexit        */

nb_monitorexit:
	subq    sp,8,sp
    stq     ra,0(sp)
	ldq     a0,string_java_lang_NullPointerException
    jsr     ra,new_exception_message
    ldgp    gp,0(ra)
	mov     v0,xptr
    ldq     ra,0(sp)
	addq    sp,8,sp
	
	lda     xpc,-4(ra)                /* faulting address is return adress - 4*/
	br      asm_handle_nat_exception
	.end    asm_builtin_monitorexit


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
	subq    sp,8,sp
	stq     ra,0(sp)
	ldq     a0,string_java_lang_ArithmeticException
	ldq     a1,string_java_lang_ArithmeticException_message
	jsr     ra,new_exception_message
	ldgp    gp,0(ra)
	mov     v0,xptr
	ldq     ra,0(sp)
	addq    sp,8,sp
	
	lda     xpc,-4(ra)                /* faulting address is return adress - 4*/
	br      asm_handle_nat_exception

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
	subq    sp,8,sp
	stq     ra,0(sp)
	ldq     a0,string_java_lang_ArithmeticException
	ldq     a1,string_java_lang_ArithmeticException_message
	jsr     ra,new_exception_message
	ldgp    gp,0(ra)
	mov     v0,xptr
	ldq     ra,0(sp)
	addq    sp,8,sp

	lda     xpc,-4(ra)                /* faulting address is return adress - 4*/
	br      asm_handle_nat_exception

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
	subq    sp,8,sp
	stq     ra,0(sp)
	ldq     a0,string_java_lang_ArithmeticException
	ldq     a1,string_java_lang_ArithmeticException_message
	jsr     ra,new_exception_message
	ldgp    gp,0(ra)
	mov     v0,xptr
	ldq     ra,0(sp)
	addq    sp,8,sp

	lda     xpc,-4(ra)                /* faulting address is return adress - 4*/
	br      asm_handle_nat_exception

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
	subq    sp,8,sp
	stq     ra,0(sp)
	ldq     a0,string_java_lang_ArithmeticException
	ldq     a1,string_java_lang_ArithmeticException_message
	jsr     ra,new_exception_message
	ldgp    gp,0(ra)
	mov     v0,xptr
	ldq     ra,0(sp)
	addq    sp,8,sp

	lda     xpc,-4(ra)                /* faulting address is return adress - 4*/
	br      asm_handle_nat_exception

	.end    asm_builtin_lrem


/******************* function asm_builtin_checkarraycast ***********************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/

	.ent    asm_builtin_checkarraycast

asm_builtin_checkarraycast:
	ldgp    gp,0(pv)
	lda     sp,-16(sp)                /* allocate stack space                 */
	stq     ra,0(sp)                  /* save return address                  */
	stq     a0,8(sp)                  /* save object pointer                  */
	jsr     ra,builtin_checkarraycast /* builtin_checkarraycast               */
	ldgp    gp,0(ra)
	beq     v0,nb_carray_throw        /* if (false) throw exception           */
	ldq     ra,0(sp)                  /* restore return address               */
	ldq     v0,8(sp)                  /* return object pointer                */
	lda     sp,16(sp)                 /* free stack space                     */
	jmp     zero,(ra)

nb_carray_throw:
	ldq     a0,string_java_lang_ClassCastException
    jsr     ra,new_exception
    ldgp    gp,0(ra)
	mov     v0,xptr

	ldq     ra,0(sp)                  /* restore return address               */
	lda     sp,16(sp)                 /* free stack space                     */
	lda     xpc,-4(ra)                /* faulting address is return adress - 4*/
	br      asm_handle_nat_exception

	.end    asm_builtin_checkarraycast


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
	subq    sp,8,sp                   /* allocate stack space                 */
	stq     ra,0(sp)                  /* save return address                  */
	ldq     a0,string_java_lang_NullPointerException
    jsr     ra,new_exception
    ldgp    gp,0(ra)
	mov     v0,xptr
	ldq     ra,0(sp)
	addq    sp,8,sp

	mov     ra,xpc                    /* faulting address is return adress    */
	br      asm_handle_nat_exception

nb_aastore_bound:
	ldq     a0,string_java_lang_ArrayIndexOutOfBoundsException
    jsr     ra,new_exception_int      /* a1 already contains the index        */
    ldgp    gp,0(ra)
	mov     v0,xptr

	ldq     ra,0(sp)                  /* restore return address               */
	lda     sp,24(sp)                 /* free stack space                     */
	mov     ra,xpc                    /* faulting address is return adress    */
	br      asm_handle_nat_exception

nb_aastore_throw:
	subq    sp,8,sp                   /* allocate stack space                 */
	stq     ra,0(sp)                  /* save return address                  */
	ldq     a0,string_java_lang_ArrayStoreException
    jsr     ra,new_exception
    ldgp    gp,0(ra)
	mov     v0,xptr
	ldq     ra,0(sp)
	addq    sp,8,sp

	mov     ra,xpc                    /* faulting address is return adress    */
	br      asm_handle_nat_exception

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
	stt     sf0, 56(sp)
	stt     sf1, 64(sp)
	stt     sf2, 72(sp)
	stt     sf3, 80(sp)
	stt     sf4, 88(sp)
	stt     sf5, 96(sp)
	stt     sf6, 104(sp)
	stt     sf7, 112(sp)
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
	ldt     sf0, 56(sp)
	ldt     sf1, 64(sp)
	ldt     sf2, 72(sp)
	ldt     sf3, 80(sp)
	ldt     sf4, 88(sp)
	ldt     sf5, 96(sp)
	ldt     sf6, 104(sp)
	ldt     sf7, 112(sp)
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
	ldq t0,offbaseval(a0)
	ldq t1,offdiffval(a0)
	ldq t2,offbaseval(a1)
_crit_end2:
	stq t0,offcast_super_baseval(a2)
	stq t1,offcast_super_diffval(a2)
	stq t2,offcast_sub_baseval(a2)
	jmp zero,(ra)

	.end    asm_getclassvalues_atomic

    .data

asm_criticalsections:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
    .quad _crit_begin1
    .quad _crit_end1
    .quad _crit_restart1
    .quad _crit_begin2
    .quad _crit_end2
    .quad _crit_restart2
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
