/* -*- mode: asm; tab-width: 4 -*- */
/****************************** asmpart.c **************************************
*                                                                              *
*   is an assembly language file, but called .c to fake the preprocessor.      *
*   It contains the Java-C interface functions for Alpha processors.           *
*                                                                              *
*   Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst              *
*                                                                              *
*   See file COPYRIGHT for information on usage and disclaimer of warranties   *
*                                                                              *
*   Authors: Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at            *
*            Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at            *
*                                                                              *
*   Last Change: 1998/11/01                                                    *
*                                                                              *
*******************************************************************************/

#include "offsets.h"

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
	.globl asm_calljavamethod
	.globl asm_call_jit_compiler
	.globl asm_dumpregistersandcall
	.globl asm_handle_exception
	.globl asm_handle_nat_exception
	.globl asm_signal_exception
	.globl asm_builtin_checkarraycast
	.globl asm_builtin_aastore
	.globl asm_builtin_monitorenter
	.globl asm_builtin_monitorexit
	.globl asm_builtin_idiv
	.globl asm_builtin_irem
	.globl asm_builtin_ldiv
	.globl asm_builtin_lrem
	.globl perform_alpha_threadswitch
	.globl initialize_thread_stack
	.globl asm_switchstackandcall


/*************************** imported variables *******************************/

	.globl newcompiler


/*************************** imported functions *******************************/

	.globl jit_compile
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


/********************* function synchronize_caches ****************************/

	.ent    synchronize_caches
synchronize_caches:

	call_pal PAL_imb                  /* synchronise instruction cache        */
	jmp     zero,(ra)                 /* return                               */

	.end    synchronize_caches


/********************* function asm_calljavamethod *****************************
*                                                                              *
*   This function calls a Java-method (which possibly needs compilation)       *
*   with up to 4 parameters.                                                   *
*                                                                              *
*   This functions calls the JIT-compiler which eventually translates the      *
*   method into machine code.                                                  *
*                                                                              *
*   An possibly throwed exception will be returned to the caller as function   *
*   return value, so the java method cannot return a fucntion value (this      *
*   function usually calls 'main' and '<clinit>' which do not return a         *
*   function value).                                                           *
*                                                                              *
*   C-prototype:                                                               *
*    javaobject_header *asm_calljavamethod (methodinfo *m,                     *
*         void *arg1, void *arg2, void *arg3, void *arg4);                     *
*                                                                              *
*******************************************************************************/

#define    	MethodPointer   -8
#define    	FrameSize       -12
#define     IsSync          -16
#define     IsLeaf          -20
#define     IntSave         -24
#define     FltSave         -28
#define     ExTableSize     -32
#define     ExTableStart    -32

#define     ExEntrySize     -32
#define     ExStartPC       -8
#define     ExEndPC         -16
#define     ExHandlerPC     -24
#define     ExCatchType     -32

	.ent    asm_calljavamethod

call_name:
	.ascii  "calljavamethod\0\0"

	.align  3
	.quad   0                         /* catch type all                       */
	.quad   calljava_xhandler         /* end pc                               */
	.quad   calljava_xhandler         /* end pc                               */
	.quad   asm_calljavamethod        /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   32                        /* frame size                           */
	.quad   0                         /* method pointer (pointer to name)     */

asm_calljavamethod:

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
	lda     pv,-64(ra)                /* asm_calljavamethod-calljava_jit !!!!!*/

calljava_return:

	ldq     ra,0(sp)                  /* restore return address               */
	ldq     gp,24(sp)                 /* restore global pointer               */
	lda     sp,32(sp)                 /* free stack space                     */
	ldl     v0,newcompiler            /* load newcompiler flag                */
	subq    v0,1,v0                   /* negate for clearing v0               */
	beq     v0,calljava_ret           /* if newcompiler skip ex copying       */
	mov     $1,v0                     /* pass exception to caller (C)         */
calljava_ret:
	jmp     zero,(ra)

calljava_xhandler:

	ldq     gp,24(sp)                 /* restore global pointer               */
	mov     itmp1,a0
	jsr     ra,builtin_throw_exception
	ldq     ra,0(sp)                  /* restore return address               */
	lda     sp,32(sp)                 /* free stack space                     */
	jmp     zero,(ra)
	.end    asm_calljavamethod


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
	ldl     t8,0(ra)              /* load instruction LDA PV,xxx(RA)          */
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


/****************** function asm_dumpregistersandcall **************************
*                                                                              *
*   This funtion saves all callee saved registers and calls the function       *
*   which is passed as parameter.                                              *
*                                                                              *
*   This function is needed by the garbage collector, which needs to access    *
*   all registers which are stored on the stack. Unused registers are          *
*   cleared to avoid interferances with the GC.                                *
*                                                                              *
*   void asm_dumpregistersandcall (functionptr f);                             *
*                                                                              *
*******************************************************************************/

	.ent    asm_dumpregistersandcall
asm_dumpregistersandcall:
	lda     sp,-16*8(sp)          /* allocate stack                           */
	stq     ra,0(sp)              /* save return address                      */

	stq     s0,1*8(sp)            /* save all callee saved registers          */
	stq     s1,2*8(sp)            /* intialize the remaining registers        */
	stq     s2,3*8(sp)
	stq     s3,4*8(sp)
	stq     s4,5*8(sp)
	stq     s5,6*8(sp)
	stq     s6,7*8(sp)
	stt     $f2,8*8(sp)
	stt     $f3,9*8(sp)
	stt     $f4,10*8(sp)
	stt     $f5,11*8(sp)
	stt     $f6,12*8(sp)
	stt     $f7,13*8(sp)
	stt     $f8,14*8(sp)
	stt     $f9,15*8(sp)

	clr     v0                   /* intialize the remaining registers         */
	clr     t0
	clr     t1
	clr     t2
	clr     t3
	clr     t4
	clr     t5
	clr     t6
	clr     t7
	clr     a1
	clr     a2
	clr     a3
	clr     a4
	clr     a5
	clr     t8
	clr     t9
	clr     t10
	clr     t11
	clr     t12
	clr     $28
	clr     $29
	cpys    $f31,$f31,$f0
	cpys    $f31,$f31,$f1
	cpys    $f31,$f31,$f10
	cpys    $f31,$f31,$f11
	cpys    $f31,$f31,$f12
	cpys    $f31,$f31,$f13
	cpys    $f31,$f31,$f14
	cpys    $f31,$f31,$f15
	cpys    $f31,$f31,$f16
	cpys    $f31,$f31,$f17
	cpys    $f31,$f31,$f18
	cpys    $f31,$f31,$f19
	cpys    $f31,$f31,$f20
	cpys    $f31,$f31,$f21
	cpys    $f31,$f31,$f22
	cpys    $f31,$f31,$f23
	cpys    $f31,$f31,$f24
	cpys    $f31,$f31,$f25
	cpys    $f31,$f31,$f26
	cpys    $f31,$f31,$f27
	cpys    $f31,$f31,$f28
	cpys    $f31,$f31,$f29
	cpys    $f31,$f31,$f30

	mov     a0,pv                 /* load function pointer                    */
	jmp     ra,(pv)               /* and call function                        */

	ldq     ra,0(sp)              /* load return address                      */
	lda     sp,16*8(sp)           /* deallocate stack                         */
	jmp     zero,(ra)             /* return                                   */

	.end    asm_dumpregistersandcall


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
	ldl     t0,0(ra)              /* load instruction LDA PV,xxx(RA)          */
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
	mov     t3,a3
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

	ldq     a0,offobjvftbl(xptr)  /* a0 = vftblptr(xptr)                      */
	ldq     a1,offobjvftbl(a1)    /* a1 = vftblptr(catchtype) class (not obj) */
	ldl     a0,offbaseval(a0)     /* a0 = baseval(xptr)                       */
	ldl     v0,offbaseval(a1)     /* a2 = baseval(catchtype)                  */
	ldl     a1,offdiffval(a1)     /* a1 = diffval(catchtype)                  */
	subl    a0,v0,a0              /* a0 = baseval(xptr) - baseval(catchtype)  */
	cmpule  a0,a1,v0              /* v0 = xptr is instanceof catchtype        */
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
	addq    sp,t0,t0              /* add Offset to stackptr                   */
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
	ldl     t0,0(ra)              /* load instruction LDA PV,xxx(RA)          */
	sll     t0,16,t0              /* compute high offset                      */
	addl    t0,0,t0               /* sign extend high offset                  */
	addq    t0,pv,pv              /* compute update address                   */
	br      ex_stack_loop

	.end    asm_handle_exception


/********************* function asm_signal_exception ***************************
*                                                                              *
*   This function handles an exception which was catched by a signal.          *
*                                                                              *
*   void asm_signal_exception (exceptionptr, signalcontext);                   *
*                                                                              *
*******************************************************************************/

#define     sigctxstack  0*8   /* sigstack state to restore                   */
#define     sigctxmask   1*8   /* signal mask to restore                      */
#define     sigctxpc     2*8   /* pc at time of signal                        */
#define     sigctxpsl    3*8   /* psl to retore                               */
#define     sigctxr00    4*8   /* processor regs 0 to 31                      */
#define     sigctxr01    5*8
#define     sigctxr02    6*8
#define     sigctxr03    7*8
#define     sigctxr04    8*8
#define     sigctxr05    9*8
#define     sigctxr06   10*8
#define     sigctxr07   11*8
#define     sigctxr08   12*8
#define     sigctxr09   13*8
#define     sigctxr10   14*8
#define     sigctxr11   15*8
#define     sigctxr12   16*8
#define     sigctxr13   17*8
#define     sigctxr14   18*8
#define     sigctxr15   19*8
#define     sigctxr16   20*8
#define     sigctxr17   21*8
#define     sigctxr18   22*8
#define     sigctxr19   23*8
#define     sigctxr20   24*8
#define     sigctxr21   25*8
#define     sigctxr22   26*8
#define     sigctxr23   27*8
#define     sigctxr24   28*8
#define     sigctxr25   29*8
#define     sigctxr26   30*8
#define     sigctxr27   31*8
#define     sigctxr28   32*8
#define     sigctxr29   33*8
#define     sigctxr30   34*8
#define     sigctxr31   35*8

#define     sigctxfpuse 36*8   /* fp has been used                            */
#define     sigctxf00   37*8   /* fp regs 0 to 31                             */
#define     sigctxf01   38*8
#define     sigctxf02   39*8
#define     sigctxf03   40*8
#define     sigctxf04   41*8
#define     sigctxf05   42*8
#define     sigctxf06   43*8
#define     sigctxf07   44*8
#define     sigctxf08   45*8
#define     sigctxf09   46*8
#define     sigctxf10   47*8
#define     sigctxf11   48*8
#define     sigctxf12   49*8
#define     sigctxf13   50*8
#define     sigctxf14   51*8
#define     sigctxf15   52*8
#define     sigctxf16   53*8
#define     sigctxf17   54*8
#define     sigctxf18   55*8
#define     sigctxf19   56*8
#define     sigctxf20   57*8
#define     sigctxf21   58*8
#define     sigctxf22   59*8
#define     sigctxf23   60*8
#define     sigctxf24   61*8
#define     sigctxf25   62*8
#define     sigctxf26   63*8
#define     sigctxf27   64*8
#define     sigctxf28   65*8
#define     sigctxf29   66*8
#define     sigctxf30   67*8
#define     sigctxf31   68*8

#define     sigctxhfpcr 69*8   /* floating point control register             */
#define     sigctxsfpcr 70*8   /* software fpcr                               */

	.ent    asm_signal_exception
asm_signal_exception:

	mov     a0,xptr
	mov     a1,sp
	ldq     xpc,sigctxpc(sp)
	ldq     v0,sigctxr00(sp)   /* restore possible used registers             */
	ldq     t0,sigctxr01(sp)
	ldq     t1,sigctxr02(sp)
	ldq     t2,sigctxr03(sp)
	ldq     t3,sigctxr04(sp)
	ldq     t4,sigctxr05(sp)
	ldq     t5,sigctxr06(sp)
	ldq     t6,sigctxr07(sp)
	ldq     t7,sigctxr08(sp)
	ldq     s0,sigctxr09(sp)
	ldq     s1,sigctxr10(sp)
	ldq     s2,sigctxr11(sp)
	ldq     s3,sigctxr12(sp)
	ldq     s4,sigctxr13(sp)
	ldq     s5,sigctxr14(sp)
	ldq     s6,sigctxr15(sp)
	ldq     a0,sigctxr16(sp)
	ldq     a1,sigctxr17(sp)
	ldq     a2,sigctxr18(sp)
	ldq     a3,sigctxr19(sp)
	ldq     a4,sigctxr20(sp)
	ldq     a5,sigctxr21(sp)
	ldq     t8,sigctxr22(sp)
	ldq     t9,sigctxr23(sp)
	ldq     t10,sigctxr24(sp)
	ldq     ra,sigctxr26(sp)
	ldq     pv,sigctxr27(sp)
	ldq     sp,sigctxr30(sp)
	br      asm_handle_nat_exception
	.end    asm_signal_exception


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
	ldq     xptr,proto_java_lang_NullPointerException
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
	ldq     xptr,proto_java_lang_NullPointerException
	lda     xpc,-4(ra)                /* faulting address is return adress - 4*/
	br      asm_handle_nat_exception
	.end    asm_builtin_monitorenter


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
	ldq     xptr,proto_java_lang_ArithmeticException
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
	ldq     xptr,proto_java_lang_ArithmeticException
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
	ldq     xptr,proto_java_lang_ArithmeticException
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
	ldq     xptr,proto_java_lang_ArithmeticException
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
	ldq     xptr,proto_java_lang_ClassCastException
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
	ldq     xptr,proto_java_lang_NullPointerException
	mov     ra,xpc                    /* faulting address is return adress    */
	br      asm_handle_nat_exception

nb_aastore_bound:
	ldq     xptr,proto_java_lang_ArrayIndexOutOfBoundsException
	lda     sp,24(sp)                 /* free stack space                     */
	mov     ra,xpc                    /* faulting address is return adress    */
	br      asm_handle_nat_exception

nb_aastore_throw:
	ldq     xptr,proto_java_lang_ArrayStoreException
	mov     ra,xpc                    /* faulting address is return adress    */
	br      asm_handle_nat_exception

	.end    asm_builtin_aastore


/********************** function initialize_thread_stack ***********************
*                                                                              *
*   initialized a thread stack                                                 *
*                                                                              *
*******************************************************************************/

	.ent    initialize_thread_stack
initialize_thread_stack:

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
	.end    initialize_thread_stack


/******************* function perform_alpha_threadswitch ***********************
*                                                                              *
*   void perform_alpha_threadswitch (u1 **from, u1 **to, u1 **stackTop);       *
*                                                                              *
*   performs a threadswitch                                                    *
*                                                                              *
*******************************************************************************/

	.ent    perform_alpha_threadswitch
perform_alpha_threadswitch:

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
	mov     ra, t12
	addq    sp, 128, sp
	jmp	zero,(ra)
	.end    perform_alpha_threadswitch


/********************* function asm_switchstackandcall *************************
*                                                                              *
*   void asm_switchstackandcall (void *stack, void *func);                     *
*                                                                              *
*   Switches to a new stack, calls a function and switches back.               *
*       a0      new stack pointer                                              *
*       a1      function pointer                                               *
*                                                                              *
*******************************************************************************/


	.ent	asm_switchstackandcall
asm_switchstackandcall:
	lda	a0,-2*8(a0)		/* allocate new stack                                 */
	stq	ra,0(a0)		/* save return address                                */
	stq	sp,1*8(a0)		/* save old stack pointer                             */
	mov	a0,sp			/* switch to new stack                                */
	
	mov	a1,pv			/* load function pointer                              */
	jmp	ra,(pv)			/* and call funciton                                  */

	ldq	ra,0(sp)		/* load return address                                */
	ldq	sp,1*8(sp)		/* switch to old stack                                */

	jmp	zero,(ra)		/* return                                             */

	.end	asm_switchstackandcall
