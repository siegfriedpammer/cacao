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
	.globl asm_builtin_anewarray
	.globl asm_builtin_aastore
	.globl asm_builtin_monitorenter
	.globl asm_builtin_monitorexit
	.globl asm_builtin_idiv
	.globl asm_builtin_irem
	.globl asm_builtin_ldiv
	.globl asm_builtin_lrem
	.globl asm_builtin_frem
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

has_no_x_instr_set:

		xor		%eax,%eax
		ret


/********************* function synchronize_caches ****************************/

synchronize_caches:

		ret


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

#define    	MethodPointer   -4
#define    	FrameSize       -8
#define     IsSync          -12
#define     IsLeaf          -16
#define     IntSave         -20
#define     FltSave         -24
#define     ExTableSize     -28
#define     ExTableStart    -28

#define     ExEntrySize     -16
#define     ExStartPC       -4
#define     ExEndPC         -8
#define     ExHandlerPC     -12
#define     ExCatchType     -16

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

		pushl	%ebp                  /* allocate stack space                 */
		movl	%esp, %ebp

		finit                         /* intitialize the fpu                  */

		pushl	$0x027f   /* Round to nearest, 53-bit mode, exceptions masked */
		fldcw	(%esp)
		addl	$4,%esp
		
		subl	$32,%esp              /* pass the remaining parameters        */
		xorl	%edx,%edx

		movl	%edx,28(%esp)         /* convert parms to 8 byte              */
		movl	24(%ebp),%eax
		movl	%eax,24(%esp)
		
		movl	%edx,20(%esp)
		movl	20(%ebp),%eax
		movl	%eax,16(%esp)

		movl	%edx,12(%esp)
		movl	16(%ebp),%eax
		movl	%eax,8(%esp)

		movl	%edx,4(%esp)
		movl	12(%ebp),%eax
		movl	%eax,(%esp)

		movl	8(%ebp),%eax          /* move function pointer to %eax        */

		lea		asm_call_jit_compiler,%ecx
		call	*%ecx                 /* call JIT compiler                    */
		
calljava_jit:
/*	lda     pv,-64(ra)                /* asm_calljavamethod-calljava_jit !!!!!*/

calljava_return:

		addl	$32,%esp              /* free stack space                     */
		
/*	ldl     v0,newcompiler            /* load newcompiler flag                */
		movl    newcompiler, %eax     /* load newcompiler flag                */
/*	subq    v0,1,v0                   /* negate for clearing v0               */
		subl    $1,%eax               /* negate for clearing v0               */
/*	beq     v0,calljava_ret           /* if newcompiler skip ex copying       */
		cmpl    $0,%eax               /* if newcompiler skip ex copying       */
		je      calljava_ret
/*	mov     $1,v0                     /* pass exception to caller (C)         */
		movl    $1, %eax              /* pass exception to caller (C)         */
calljava_ret:
		leave
		ret

calljava_xhandler:

/*	ldq     gp,24(sp)                 /* restore global pointer               */
/*	mov     itmp1,a0 */
		movl	$0,%eax
/*	jsr     ra,builtin_throw_exception */
		call    builtin_throw_exception
/*	ldq     ra,0(sp)                  /* restore return address               */
/*	lda     sp,32(sp)                 /* free stack space                     */
/*	jmp     zero,(ra) */
		leave
		ret


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
		pushl	%ebp                  /* allocate stack space                 */
		movl	%esp, %ebp

		subl	$32,%esp              /* pass the remaining parameters        */
		xorl	%edx,%edx

		movl	%edx,28(%esp)         /* convert parms to 8 byte              */
		movl	24(%ebp),%eax
		movl	%eax,24(%esp)
		
		movl	%edx,20(%esp)
		movl	20(%ebp),%eax
		movl	%eax,16(%esp)

		movl	%edx,12(%esp)
		movl	16(%ebp),%eax
		movl	%eax,8(%esp)

		movl	%edx,4(%esp)
		movl	12(%ebp),%eax
		movl	%eax,(%esp)

		movl	8(%ebp),%eax          /* move function pointer to %eax        */

		lea		asm_call_jit_compiler,%ecx 
		call	*%ecx                 /* call JIT compiler                    */
	
calljava_jit2:
/*	lda     pv,-64(ra)                /* asm_calljavamethod-calljava_jit !!!!!*/

calljava_return2:
		addl	$32,%esp              /* free stack space                     */
		
calljava_ret2:
		leave
		ret

calljava_xhandler2:

/*	ldq     gp,24(sp)                 /* restore global pointer               */
/*	mov     itmp1,a0 */
		movl	$0,%eax
/*	jsr     ra,builtin_throw_exception */
		call    builtin_throw_exception
/*	ldq     ra,0(sp)                  /* restore return address               */
/*	lda     sp,32(sp)                 /* free stack space                     */
/*	jmp     zero,(ra) */
		leave
		ret
						

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

		pushl	%ebx            /* save register                              */
		pushl	%esi
		pushl	%edi
				
		movl	4(%ebp),%edx	/* get return address                         */
		movb	-1(%edx),%bl	/* get function code                          */
		cmpb	$0xd2,%bl		/* called with `call *REG_ITMP2' (%edx)?      */
		jne		L_not_static_special

		subl	$6,%edx			/* calculate address of immediate             */
		jmp		L_call_jit_compile
		
L_not_static_special:
		cmpb	$0xd0,%bl		/* called with `call *REG_ITMP1' (%eax)       */
		jne		L_not_virtual_interface
		
		subl	$6,%edx         /* calculate address of offset                */
		movl	(%edx),%edx     /* get offset                                 */
		addl	%ecx,%edx       /* add base address to get method address     */
		jmp		L_call_jit_compile

L_not_virtual_interface:        /* a call from asm_calljavamethod             */
		xorl	%edx,%edx
		
L_call_jit_compile:
		pushl	%edx            /* save address for method pointer            */

		pushl	%eax			/* push methodpointer on stack                */
		call	jit_compile
		addl	$4,%esp

		popl	%edx            /* restore address for method pointer         */
		testl	%edx,%edx		/* is address == 0 (asm_calljavamethod)       */
		je		L_call_method
		
		movl	%eax,(%edx)		/* and now save the new pointer               */

L_call_method:
		popl	%edi            /* restore registers                          */
		popl	%esi
		popl	%ebx
		
		leave
		jmp		*%eax			/* ...and now call the new method             */



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
		pushl	%ebp				/* build stack frame */
		movl	%esp,%ebp

		push	%ebx
		push	%esi
		push	%edi
				
		movl	8(%ebp),%eax		/* load function pointer */
		call	*%eax				/* call function */

		pop		%edi
		pop		%esi
		pop		%ebx
		
		leave
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
		pushl	%ebp
		movl	%esp,%ebp

		pushl	%eax						/* save exception pointer */
		pushl	%edx						/* save exception pc */
		pushl	%ecx						/* save data segment pointer */
		pushl	%ebx
		pushl	%esi
		pushl	%edi
		
ex_trace:
		subl	$16,%esp

		movl	%eax,(%esp)					/* exception pointer */

		movl	MethodPointer(%ecx),%eax	/* method pointer */
		movl	%eax,4(%esp)
		
		movl	%edx,8(%esp)				/* exception pc */
		movl	$1,12(%esp)					/* set no unwind flag */
		call	builtin_trace_exception

		addl	$16,%esp

		movl	-12(%ebp),%esi				/* %esi = data segment pointer */
		movl	ExTableSize(%esi),%ecx		/* %ecx = exception table size */
		test	%ecx,%ecx					/* if empty table skip */
		je		empty_table

		lea		ExTableStart(%esi),%edi		/* %edi = start of exception table */
		movl	-4(%ebp),%eax				/* get xptr */
		
ex_table_loop:
		movl	-8(%ebp),%edx				/* get xpc */
		
		movl	ExStartPC(%edi),%ebx		/* %ebx = exception start pc */
		cmpl	%edx,%ebx					/* %ebx = (startpc <= xpc) */
		jg		ex_table_cont				/* if (false) continue */
		movl	ExEndPC(%edi),%ebx			/* %ebx = exception end pc */
		cmpl	%ebx,%edx					/* %ebx = (xpc < endpc) */
		jge		ex_table_cont				/* if (false) continue */
		movl	ExCatchType(%edi),%ebx		/* arg1 = exception catch type */
		test	%ebx,%ebx					/* NULL catches everything */
		je		ex_handle_it

		movl	offobjvftbl(%eax),%esi		/* %esi = vftblptr(xptr) */
		movl	offobjvftbl(%ebx),%ebx		/* %ebx = vftblptr(catchtype) class (not obj) */
		movl	offbaseval(%esi),%esi		/* %esi = baseval(xptr) */
		movl	offbaseval(%ebx),%edx		/* %edx = baseval(catchtype) */
		movl	offdiffval(%ebx),%ebx		/* %ebx = diffval(catchtype) */
		subl	%edx,%esi					/* %esi = baseval(xptr) - baseval(catchtype) */
		cmpl	%ebx,%esi					/* xptr is instanceof catchtype */
		ja		ex_table_cont
		
ex_handle_it:
		movl	ExHandlerPC(%edi),%edx

		popl	%edi
		popl	%esi
		popl	%ebx

ex_jump:
/*		popl	%edx						/* restore exception pc */
/*		popl	%eax						/* restore exception pointer */
		leave
		
		jmp		*%edx

ex_table_cont:
		lea		ExEntrySize(%edi),%edi
		decl	%ecx
		test	%ecx,%ecx
		jg		ex_table_loop
		
empty_table:
		xorl	%eax,%eax
		movl	$0,(%eax)
		
		popl	%edi
		popl	%esi
		popl	%ebx
		
		leave
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
		cmpl	$0,12(%esp)             /* if (null) throw exception */
		je		nb_idiv

		movl	4(%esp),%eax            /* 4 (return) = 4 */
		cltd
		idivl	12(%esp)

		ret

nb_idiv:
		popl	%edx					/* delete return address */
		
		movl	proto_java_lang_ArithmeticException,%eax
		subl	$2,%edx					/* faulting address is return adress - 2 */
		jmp		asm_handle_nat_exception
		

/************************ function asm_builtin_ldiv ****************************
*                                                                              *
*   Does null check and calls ldiv or throws an exception                      *
*                                                                              *
*******************************************************************************/

asm_builtin_ldiv:
		movl	12(%esp),%eax
		orl		16(%esp),%eax
		test	%eax,%eax				/* if (null) throw exception */
		je		nb_ldiv

		jmp		builtin_ldiv

nb_ldiv:
		popl	%edx					/* delete return address */
		
		movl	proto_java_lang_ArithmeticException,%eax
		subl	$2,%edx					/* faulting address is return adress - 2 */
		jmp		asm_handle_nat_exception
				

/************************ function asm_builtin_irem ****************************
*                                                                              *
*   Does null check and calls irem or throws an exception                      *
*                                                                              *
*******************************************************************************/

asm_builtin_irem:
		cmpl	$0,12(%esp)             /* if (null) throw exception */
		je		nb_irem

		movl	4(%esp),%eax            /* 4 (return) = 4 */
		cltd
		idivl	12(%esp)
		
		movl	%edx,%eax
		ret

nb_irem:
		popl	%edx					/* delete return address */
		
		movl	proto_java_lang_ArithmeticException,%eax
		subl	$2,%edx					/* faulting address is return adress - 2 */
		jmp		asm_handle_nat_exception


/************************ function asm_builtin_lrem ****************************
*                                                                              *
*   Does null check and calls lrem or throws an exception                      *
*                                                                              *
*******************************************************************************/

asm_builtin_lrem:
		movl	12(%esp),%eax
		orl		16(%esp),%eax
		test	%eax,%eax				/* if (null) throw exception */
		je		nb_lrem

		jmp		builtin_lrem

nb_lrem:
		popl	%edx					/* delete return address */
		
		movl	proto_java_lang_ArithmeticException,%eax
		subl	$2,%edx					/* faulting address is return adress - 2 */
		jmp		asm_handle_nat_exception
		

/************************ function asm_builtin_frem ****************************
*                                                                              *
*   Does null check and calls frem or throws an exception                      *
*                                                                              *
*******************************************************************************/

asm_builtin_frem:
		subl	$8,%esp					/* build stack frame (2 * 4 bytes) */

		flds	12(%esp)                /* 8 (frame) + 4 (return) = 12 */
		fstps	(%esp)

		flds	20(%esp)
		fstps	4(%esp)

		call	builtin_frem

		addl	$8,%esp
		ret

nb_frem:
		popl	%edx					/* delete return address */
		
		movl	proto_java_lang_ArithmeticException,%eax
		subl	$2,%edx					/* faulting address is return adress - 2 */
		jmp		asm_handle_nat_exception
		

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
		pushl	%ecx					/* save data segment address */
		
		subl	$8,%esp					/* build stack frame (2 * 4 bytes) */

		movl	16(%esp),%eax           /* 8 (frame) + 4 (return) + 4 (data) = 16 */
		movl	%eax,(%esp)

		movl	24(%esp),%eax
		movl	%eax,4(%esp)

		call	builtin_checkarraycast  /* builtin_checkarraycast               */
	
		addl	$8,%esp

		test	%eax,%eax               /* if (false) throw exception           */
		je		nb_carray_throw

		addl	$4,%esp
		ret

nb_carray_throw:
		popl	%ecx					/* restore data segment address */
		popl	%edx					/* delete return address */
		
		movl	proto_java_lang_ClassCastException,%eax
		subl	$2,%edx					/* faulting address is return adress - 2 */
		jmp		asm_handle_nat_exception

		
/******************* function asm_builtin_anewarray ****************************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/

asm_builtin_anewarray:
		subl	$8,%esp					/* build stack frame (2 * 4 bytes) */

		movl	12(%esp),%eax
		movl	%eax,(%esp)

		movl	20(%esp),%eax
		movl	%eax,4(%esp)

		call	builtin_anewarray
	
		addl	$8,%esp
		ret

		
/******************* function asm_builtin_aastore ******************************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/

asm_builtin_aastore:
		pushl	%ecx					/* save data segment address */
		
		subl	$12,%esp				/* build stack frame (3 * 4 bytes) */

		movl	20(%esp),%eax           /* 12 (frame) + 4 (return) + 4 (data) */
		test	%eax,%eax				/* if null pointer throw exception */
		je		nb_aastore_null

		movl	offarraysize(%eax),%edx /* load size */
		movl	28(%esp),%ecx			/* index */
		cmpl	%edx,%ecx				/* do bound check */
		ja		nb_aastore_bound		/* if out of bounds throw exception */
		
/*		shll	$3,%ecx					/* index * 4 */
/*		addl	%eax,%ecx				/* add index * 4 to arrayref */
	
/*		movl	20(%esp),%eax           /* 12 (frame) + 4 (return) + 4 (data) */
/*		movl	%eax,(%esp)

/*		movl	36(%esp),%eax			/* object is second argument */
/*		movl	%eax,4(%esp)
		
/*		call	builtin_canstore		/* builtin_canstore(arrayref,object) */

/*		test	%eax,%eax */
/*		je		nb_aastore_throw */
				
		movl	20(%esp),%eax           /* 12 (frame) + 4 (return) + 4 (data) */
		movl	%eax,(%esp)

		movl	28(%esp),%eax
		movl	%eax,4(%esp)

		movl	36(%esp),%eax
		movl	%eax,8(%esp)
		
		call	builtin_aastore

		test	%eax,%eax
		je		nb_aastore_throw
			
		addl	$12,%esp

		addl	$4,%esp
		ret

nb_aastore_null:
		addl	$12,%esp
		popl	%ecx					/* restore data segment address */
		popl	%edx					/* delete return address */
		
		movl	proto_java_lang_NullPointerException,%eax
		subl	$2,%edx					/* faulting address is return adress - 2 */
		jmp		asm_handle_nat_exception

nb_aastore_bound:
		addl	$12,%esp
		popl	%ecx					/* restore data segment address */
		popl	%edx					/* delete return address */
		
		movl	proto_java_lang_ArrayIndexOutOfBoundsException,%eax
		subl	$2,%edx					/* faulting address is return adress - 2 */
		jmp		asm_handle_nat_exception
		
nb_aastore_throw:
		addl	$12,%esp
		popl	%ecx					/* restore data segment address */
		popl	%edx					/* delete return address */
		
		movl	proto_java_lang_ArrayStoreException,%eax
		subl	$2,%edx					/* faulting address is return adress - 2 */
		jmp		asm_handle_nat_exception

		
/******************* function asm_initialize_thread_stack **********************
*                                                                              *
*   initialized a thread stack                                                 *
*                                                                              *
*******************************************************************************/

asm_initialize_thread_stack:
		movl	8(%esp),%eax
		subl	$36,%eax				/* 4 bytes * 8 regs + 4 bytes func */

		xorl	%edx,%edx
		movl	%edx,0(%eax)
		movl	%edx,4(%eax)
		movl	%edx,8(%eax)
		movl	%edx,12(%eax)
		movl	%edx,16(%eax)
		movl	%edx,20(%eax)
		movl	%edx,24(%eax)
		movl	%edx,28(%eax)

		movl	4(%esp),%edx
		movl	%edx,32(%eax)
		
		ret


/******************* function asm_perform_threadswitch *************************
*                                                                              *
*   void asm_perform_threadswitch (u1 **from, u1 **to, u1 **stackTop);         *
*                                                                              *
*   performs a threadswitch                                                    *
*                                                                              *
*******************************************************************************/

asm_perform_threadswitch:
		subl	$36,%esp

		movl	%eax,0(%esp)
		movl	%edx,4(%esp)
		movl	%ecx,8(%esp)
		movl	%ebx,12(%esp)
		movl	%esp,16(%esp)
		movl	%ebp,20(%esp)
		movl	%esi,24(%esp)
		movl	%edi,28(%esp)

		movl	36(%esp),%eax			/* save return address */
		movl	%eax,32(%esp)

		movl	40(%esp),%eax			/* first argument **from */
		movl	%esp,0(%eax)

		movl	48(%esp),%eax			/* third argument **stackTop */
		movl	%esp,0(%eax)

		movl	44(%esp),%eax			/* second argument **to */
		movl	0(%eax),%esp			/* load stack pointer */

		movl	32(%esp),%eax			/* restore return address */
		movl	%eax,36(%esp)
		
		movl	0(%esp),%eax
		movl	4(%esp),%edx
		movl	8(%esp),%ecx
		movl	12(%esp),%ebx
										/* skip stack pointer */
		movl	20(%esp),%ebp
		movl	24(%esp),%esi
		movl	28(%esp),%edi

		addl	$36,%esp
		ret
		

/********************* function asm_switchstackandcall *************************
*                                                                              *
*  void asm_switchstackandcall (void *stack, void *func, void **stacktopsave,  *
*		                       void *p);                                       *
*                                                                              *
*   Switches to a new stack, calls a function and switches back.               *
*       a0      new stack pointer                                              *
*       a1      function pointer                                               *
*		a2		pointer to variable where stack top should be stored           *
*       a3      pointer to user data, is passed to the function                *
*                                                                              *
*******************************************************************************/

asm_switchstackandcall:
		movl	4(%esp),%edx			/* first argument *stack */
		subl	$8,%edx					/* allocate new stack */

		movl	0(%esp),%eax			/* save return address on new stack */
		movl	%eax,0(%edx)

		movl	%esp,4(%edx)			/* save old stack pointer on new stack */

		movl	12(%esp),%eax			/* third argument **stacktopsave */
		movl	%esp,0(%eax)			/* save old stack pointer to variable */

		movl	8(%esp),%eax			/* load function pointer */
		movl	16(%esp),%ecx			/* fourth argument *p */
		
		movl	%edx,%esp				/* switch to new stack */

		subl	$4,%esp
		movl	%ecx,0(%esp)			/* pass pointer */
		call	*%eax					/* and call function */
		addl	$4,%esp

		movl	0(%esp),%eax			/* load return address */
		movl	4(%esp),%esp			/* switch to old stack */
		movl	%eax,0(%esp)
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
