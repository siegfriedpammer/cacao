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

	.text
/*	.set    noat */
/*	.set    noreorder */


/********************* exported functions and variables ***********************/

	.globl has_no_x_instr_set
	.globl synchronize_caches
	.globl asm_calljavamethod
	.globl asm_calljavafunction
	.globl asm_call_jit_compiler
	.globl asm_dumpregistersandcall
	.globl asm_handle_exception
	.globl asm_handle_nat_exception
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

/*	.ent    has_no_x_instr_set */
has_no_x_instr_set:

/*	.long   0x47e03c20                /* amask   1,v0                         */
		xorl	%eax, %eax
/*	jmp     zero,(ra)                 /* return                               */
		ret

/*	.end    has_no_x_instr_set */


/********************* function synchronize_caches ****************************/

/*	.ent    synchronize_caches */
synchronize_caches:

/*	call_pal PAL_imb                  /* synchronise instruction cache        */
/*	jmp     zero,(ra)                 /* return                               */
		ret

/*	.end    synchronize_caches */


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

/*	.ent    asm_calljavamethod */

call_name:
	.ascii  "calljavamethod\0\0"

/*	.align  3 */
	.align	8
	.quad   0                         /* catch type all                       */
/*	.quad   calljava_xhandler         /* handler pc                           */
/*	.quad   calljava_xhandler         /* end pc                               */
/*	.quad   asm_calljavamethod        /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   32                        /* frame size                           */
	.quad   0                         /* method pointer (pointer to name)     */

asm_calljavamethod:

/*	ldgp    gp,0(pv) */
/*	lda     sp,-32(sp)                /* allocate stack space                 */
		pushl	%ebp                  /* allocate stack space                 */
		movl	%esp, %ebp
/*	stq     gp,24(sp)                 /* save global pointer                  */
/*	stq     ra,0(sp)                  /* save return address                  */

/*	stq     a0,16(sp)                 /* save method pointer for compiler     */
/*	lda     v0,16(sp)                 /* pass pointer to method pointer via v0*/

/*	mov     a1,a0                     /* pass the remaining parameters        */
/*	mov     a2,a1 */
/*	mov     a3,a2 */
/*	mov     a4,a3 */
		pushl	24(%ebp)              /* pass the remaining parameters        */
		pushl   20(%ebp)
		pushl	16(%ebp)
		pushl	12(%ebp)
		pushl	8(%ebp)
				
/*	lda     $28,asm_call_jit_compiler /* fake virtual function call (2 instr) */
/*	stq     $28,8(sp)                 /* store function address               */
/*	mov     sp,$28                    /* set method pointer                   */

/*	ldq     pv,8($28)                 /* method call as in Java               */
/*	jmp     ra,(pv)                   /* call JIT compiler                    */
		pushl	%ebp                  /* save %ebp, cause it's a tmp reg      */
		call	asm_call_jit_compiler /* call JIT compiler                    */
		popl	%ebp                  /* restore %ebp, changed in java code   */
calljava_jit:
/*	lda     pv,-64(ra)                /* asm_calljavamethod-calljava_jit !!!!!*/

calljava_return:

/*	ldq     ra,0(sp)                  /* restore return address               */
/*	ldq     gp,24(sp)                 /* restore global pointer               */
/*	lda     sp,32(sp)                 /* free stack space                     */
		addl	$20, %esp             /* free stack space                     */
/*	ldl     v0,newcompiler            /* load newcompiler flag                */
		movl    newcompiler, %eax     /* load newcompiler flag                */
/*	subq    v0,1,v0                   /* negate for clearing v0               */
		subl    $1, %eax              /* negate for clearing v0               */
/*	beq     v0,calljava_ret           /* if newcompiler skip ex copying       */
		cmpl    $0, %eax              /* if newcompiler skip ex copying       */
		je      calljava_ret
/*	mov     $1,v0                     /* pass exception to caller (C)         */
		movl    $1, %eax              /* pass exception to caller (C)         */
calljava_ret:
/*	jmp     zero,(ra) */
		leave
		ret

calljava_xhandler:

/*	ldq     gp,24(sp)                 /* restore global pointer               */
/*	mov     itmp1,a0 */
		movl	$0, %eax
/*	jsr     ra,builtin_throw_exception */
		call    builtin_throw_exception
/*	ldq     ra,0(sp)                  /* restore return address               */
/*	lda     sp,32(sp)                 /* free stack space                     */
/*	jmp     zero,(ra) */
		leave
		ret
/*	.end    asm_calljavamethod */


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

/*	.ent    asm_calljavafunction */

call_name2:
	.ascii  "calljavafunction\0\0"

/*	.align  3 */
	.align	8
	.quad   0                         /* catch type all                       */
/*	.quad   calljava_xhandler2        /* handler pc                           */
/*	.quad   calljava_xhandler2        /* end pc                               */
/*	.quad   asm_calljavafunction      /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   32                        /* frame size                           */
	.quad   0                         /* method pointer (pointer to name)     */

asm_calljavafunction:
/*	ldgp    gp,0(pv) */
/*	lda     sp,-32(sp)                /* allocate stack space                 */
		pushl	%ebp                  /* allocate stack space                 */
		movl	%esp, %ebp
		subl	$8, %esp
/*	stq     gp,24(sp)                 /* save global pointer                  */
/*	stq     ra,0(sp)                  /* save return address                  */

/*	stq     a0,16(sp)                 /* save method pointer for compiler     */
/*	lda     v0,16(sp)                 /* pass pointer to method pointer via v0*/

/*	mov     a1,a0                     /* pass the remaining parameters        */
/*	mov     a2,a1 */
/*	mov     a3,a2 */
/*	mov     a4,a3 */
		pushl	24(%ebp)              /* pass the remaining parameters        */
		pushl	20(%ebp)
		pushl	16(%ebp)
		pushl	12(%ebp)
		movl	8(%ebp),%eax          /* method pointer is passed in %eax     */

/*	lda     $28,asm_call_jit_compiler /* fake virtual function call (2 instr) */
/*	stq     $28,8(sp)                 /* store function address               */
/*	mov     sp,$28                    /* set method pointer                   */

/*	ldq     pv,8($28)                 /* method call as in Java               */
/*	jmp     ra,(pv)                   /* call JIT compiler                    */
		call	asm_call_jit_compiler /* call JIT compiler                    */
calljava_jit2:
/*	lda     pv,-64(ra)                /* asm_calljavafunction-calljava_jit !!!!!*/

calljava_return2:

/*	ldq     ra,0(sp)                  /* restore return address               */
/*	ldq     gp,24(sp)                 /* restore global pointer               */
/*	lda     sp,32(sp)                 /* free stack space                     */
		addl	$0x10, %esp           /* free stack space                     */

/*	ldl     v0,newcompiler */         /* load newcompiler flag                */
/*	subq    v0,1,v0        */         /* negate for clearing v0               */
/*	beq     v0,calljava_ret*/         /* if newcompiler skip ex copying       */
/*	mov     $1,v0 */                  /* pass exception to caller (C)         */
calljava_ret2:
/*	jmp     zero,(ra) */
		leave
		ret

calljava_xhandler2:

/*	ldq     gp,24(sp)                 /* restore global pointer               */
/*	mov     itmp1,a0 */
		movl	$0, %eax
/*	jsr     ra,builtin_throw_exception */
		call builtin_throw_exception
/*	ldq     ra,0(sp)                  /* restore return address               */
/*	lda     sp,32(sp)                 /* free stack space                     */
/*	jmp     zero,(ra) */
		leave
		ret
/*	.end    asm_calljavafunction */
						

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
*                                                                              *
*   i386_mov_imm_reg(a, REG_ITMP2)                ; invokestatic/special       *
*   i386_call_reg(REG_ITMP2)                                                   *
*                                                                              *
*   or                                                                         *
*                                                                              *
*   i386_mov_membase_reg(REG_SP, 0, REG_ITMP2)    ; invokevirtual/interface    *
*   i386_mov_membase_reg(REG_ITMP2, OFFSET(, vftbl), REG_ITMP3)                *
*   i386_mov_membase_reg(REG_ITMP3, OFFSET(vftbl, table[0]) + \                *
*       sizeof(methodptr) * m->vftblindex, REG_ITMP1)                          *
*   i386_call_reg(REG_ITMP1)                                                   *
*                                                                              *
*   in the static case the method pointer can be computed using the            *
*   return address and the lda function following the jmp instruction          *
*                                                                              *
*******************************************************************************/


asm_call_jit_compiler:

		pushl	%ebp
		movl	%esp, %ebp

		pushl	%eax			/* push methodpointer on stack */
		call	jit_compile
		addl	$4,%esp

		leave
		jmp		*%eax
		
		movl	4(%ebp),%edx	/* see how we have been called */
		movb	-1(%edx),%bl
		cmpb	$0xd2,%bl		/* called with `call *REG_ITMP2' (%edx)? */
		jne		L01

		subl	$6,%edx			/* calculate immediate address */
		movl	%eax,(%edx)		/* and now save the pointer */

L01:	leave
		jmp		*%eax			/* ...and now call the new method */



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

asm_dumpregistersandcall:
	xorl		%eax,%eax
	movl		$0,(%eax)
	ret


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
asm_handle_exception:
	xorl		%eax,%eax
	movl		$0,(%eax)
	ret


/********************* function asm_builtin_monitorenter ***********************
*                                                                              *
*   Does null check and calls monitorenter or throws an exception              *
*                                                                              *
*******************************************************************************/

asm_builtin_monitorenter:
	jmp		builtin_monitorenter


/********************* function asm_builtin_monitorexit ************************
*                                                                              *
*   Does null check and calls monitorexit or throws an exception               *
*                                                                              *
*******************************************************************************/

asm_builtin_monitorexit:
	jmp		builtin_monitorexit


/************************ function asm_builtin_idiv ****************************
*                                                                              *
*   Does null check and calls idiv or throws an exception                      *
*                                                                              *
*******************************************************************************/

asm_builtin_idiv:
	ret
		

/************************ function asm_builtin_ldiv ****************************
*                                                                              *
*   Does null check and calls ldiv or throws an exception                      *
*                                                                              *
*******************************************************************************/

asm_builtin_ldiv:
	jmp		builtin_ldiv
				

/************************ function asm_builtin_irem ****************************
*                                                                              *
*   Does null check and calls irem or throws an exception                      *
*                                                                              *
*******************************************************************************/

asm_builtin_irem:
	ret


/************************ function asm_builtin_lrem ****************************
*                                                                              *
*   Does null check and calls lrem or throws an exception                      *
*                                                                              *
*******************************************************************************/

asm_builtin_lrem:
	jmp		builtin_lrem
		

/*********************** function new_builtin_checkcast ************************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/

asm_builtin_checkcast:
	xorl		%eax,%eax
	movl		$0,(%eax)
	ret

		
/******************* function asm_builtin_checkarraycast ***********************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/

asm_builtin_checkarraycast:
	xorl		%eax,%eax
	movl		$0,(%eax)
	ret
		

/******************* function asm_builtin_aastore ******************************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/

asm_builtin_aastore:
	xorl		%eax,%eax
	movl		$0,(%eax)
	ret


/******************* function asm_initialize_thread_stack **********************
*                                                                              *
*   initialized a thread stack                                                 *
*                                                                              *
*******************************************************************************/

asm_initialize_thread_stack:
	xorl		%eax,%eax
	movl		$0,(%eax)
	ret


/******************* function asm_perform_threadswitch *************************
*                                                                              *
*   void asm_perform_threadswitch (u1 **from, u1 **to, u1 **stackTop);         *
*                                                                              *
*   performs a threadswitch                                                    *
*                                                                              *
*******************************************************************************/

asm_perform_threadswitch:
	xorl		%eax,%eax
	movl		$0,(%eax)
	ret
		

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

asm_switchstackandcall:
	xorl		%eax,%eax
	movl		$0,(%eax)
	ret

		
/********************* function asm_getcallingmethod ***************************
*                                                                              *
*   classinfo *asm_getcallingmethodclass ();								   *
*																			   *	
*   goes back stack frames to get the calling method						   *	   
*																			   *	
*				t2 .. sp												       *
*				t3 .. ra												       *
*				t4 .. pv												       *
*                                                                              *
*******************************************************************************/

asm_getcallingmethod:
	xorl		%eax,%eax
	movl		$0,(%eax)
	ret
