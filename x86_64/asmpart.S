/* -*- mode: asm; tab-width: 4 -*- */
/* x86_64/asmpart.S ************************************************************
*                                                                              *
*   It contains the Java-C interface functions for x86_64 processors.          *
*                                                                              *
*   Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst              *
*                                                                              *
*   See file COPYRIGHT for information on usage and disclaimer of warranties   *
*                                                                              *
*   Authors: Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at            *
*            Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at            *
*            Christian Thalinger EMAIL: cacao@complang.tuwien.ac.at            *
*                                                                              *
*   Last Change: $Id: asmpart.S 413 2003-08-22 17:46:18Z twisti $         *
*                                                                              *
*******************************************************************************/

#include "offsets.h"

	.text


/********************* exported functions and variables ***********************/

	.globl has_no_x_instr_set
	.globl asm_calljavamethod
	.globl asm_calljavafunction
	.globl asm_call_jit_compiler
	.globl asm_dumpregistersandcall
	.globl asm_handle_exception
	.globl asm_handle_nat_exception
	.globl asm_builtin_checkcast	
	.globl asm_builtin_checkarraycast
	.globl asm_builtin_anewarray
	.globl asm_builtin_newarray_array
	.globl asm_builtin_aastore
	.globl asm_builtin_monitorenter
	.globl asm_builtin_monitorexit
	.globl asm_builtin_idiv
	.globl asm_builtin_irem
	.globl asm_builtin_ldiv
	.globl asm_builtin_lrem
	.globl asm_builtin_frem
	.globl asm_builtin_arrayinstanceof
	.globl asm_perform_threadswitch
	.globl asm_initialize_thread_stack
	.globl asm_switchstackandcall
	.globl asm_getcallingmethod
    .globl asm_builtin_trace
    .globl asm_builtin_exittrace
    
/*************************** imported functions *******************************/

	.globl jit_compile
	.globl builtin_monitorexit
	.globl builtin_throw_exception
	.globl builtin_trace_exception
	.globl class_java_lang_Object
	.globl findmethod

/*********************** function has_no_x_instr_set ***************************
*                                                                              *
*   determines if the byte support instruction set (21164a and higher)         *
*   is available.                                                              *
*                                                                              *
*   Use it on i386 architecture to init the fpu.                               *
*                                                                              *
*******************************************************************************/

has_no_x_instr_set:
		xor		%rax,%rax				/* result code 0 (not used for i386)  */
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

call_name:
	.ascii  "calljavamethod\0\0"

/*	.align  3 */
	.align	8
	.long   0                         /* catch type all                       */
	.long   calljava_xhandler         /* handler pc                           */
	.long   calljava_xhandler         /* end pc                               */
	.long   asm_calljavamethod        /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   8                         /* frame size                           */
	.long   0                         /* method pointer (pointer to name)     */

asm_calljavamethod:
        sub     $8,%rsp               /* keep %rsp 16-byte aligned            */
        
        mov     %rdi,%rax             /* move function pointer to %rax        */
                                      /* compilerstub uses this               */
        
        mov     %rsi,%rdi             /* pass remaining parameters            */
        mov     %rdx,%rsi
        mov     %rcx,%rdx
        mov     %r8,%rcx
        
        lea		asm_call_jit_compiler,%r11
        call	*%r11                 /* call JIT compiler                    */
		
calljava_jit:
calljava_return:
calljava_ret:
        xor     %rax,%rax
        add     $8,%rsp               /* keep %rsp 16-byte aligned            */
        ret

calljava_xhandler:
        push	%rax				  /* pass exception pointer               */
        call    builtin_throw_exception
        add     $8,%rsp
        add     $8,%rsp
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
	.long   0                         /* catch type all                       */
	.long   calljava_xhandler2        /* handler pc                           */
	.long   calljava_xhandler2        /* end pc                               */
	.long   asm_calljavafunction      /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   8                         /* frame size                           */
	.long   0                         /* method pointer (pointer to name)     */

asm_calljavafunction:
		mov     %rdi,%rax             /* move function pointer to %rax        */
                                      /* compilerstub uses this               */

        mov     %rsi,%rdi             /* pass remaining parameters            */
        mov     %rdx,%rsi
        mov     %rcx,%rdx
        mov     %r8,%rcx
        
		lea		asm_call_jit_compiler,%r11
		call	*%r11                 /* call JIT compiler                    */
	
calljava_jit2:
calljava_return2:
calljava_ret2:
		ret

calljava_xhandler2:
		push    %rax				  /* pass exception pointer               */
		call    builtin_throw_exception
		add     $8,%esp
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
        sub     $8,%rsp         /* keep %rsp 16-byte aligned                  */
        
        mov     8(%rsp),%r10    /* get return address                         */
        movb	-1(%r10),%r11b	/* get function code                          */
        cmpb	$0xd2,%r11b		/* called with `call *REG_ITMP2' (%rdx)?      */
        jne		L_not_static_special

        sub     $6,%r10			/* calculate address of immediate             */
        jmp		L_call_jit_compile
		
L_not_static_special:
		cmpb	$0xd0,%r11b		/* called with `call *REG_ITMP1' (%rax)       */
		jne		L_not_virtual_interface
		
		sub     $6,%r10         /* calculate address of offset                */
		mov     (%r10),%r10     /* get offset                                 */
		add     %rcx,%r10       /* add base address to get method address     */
		jmp		L_call_jit_compile

L_not_virtual_interface:        /* a call from asm_calljavamethod             */
		xor     %r10,%r10
		
L_call_jit_compile:
		xor     %r10,%r10       /* HACK: don't write address */

        sub     $(16*8),%rsp       /* 8 + 48 + 64 */
        
  		mov     %r10,0*8(%rsp)  /* save address for method pointer            */

        mov     %rdi,1*8(%rsp)  /* save arguments                             */
        mov     %rsi,2*8(%rsp)
        mov     %rdx,3*8(%rsp)
        mov     %rcx,4*8(%rsp)
        mov     %r8,5*8(%rsp)
        mov     %r9,6*8(%rsp)

        movq    %xmm0,7*8(%rsp)
        movq    %xmm1,8*8(%rsp)
        movq    %xmm2,9*8(%rsp)
        movq    %xmm3,10*8(%rsp)
        movq    %xmm4,11*8(%rsp)
        movq    %xmm5,12*8(%rsp)
        movq    %xmm6,13*8(%rsp)
        movq    %xmm7,14*8(%rsp)

        mov     %rax,%rdi       /* pass method pointer                        */
		call	jit_compile

        mov     0*8(%rsp),%r10
        
        mov     1*8(%rsp),%rdi
        mov     2*8(%rsp),%rsi
        mov     3*8(%rsp),%rdx
        mov     4*8(%rsp),%rcx
        mov     5*8(%rsp),%r8
        mov     6*8(%rsp),%r9

        movq    7*8(%rsp),%xmm0
        movq    8*8(%rsp),%xmm1
        movq    9*8(%rsp),%xmm2
        movq    10*8(%rsp),%xmm3
        movq    11*8(%rsp),%xmm4
        movq    12*8(%rsp),%xmm5
        movq    13*8(%rsp),%xmm6
        movq    14*8(%rsp),%xmm7

        add     $(16*8),%rsp
        
		test	%r10,%r10		/* is address == 0 (asm_calljavamethod)       */
		je		L_call_method
		
		mov     %rax,(%r10)		/* and now save the new pointer               */

L_call_method:
        add     $8,%rsp         /* keep %rsp 16-byte aligned                  */
		jmp		*%rax			/* ...and now call the new method             */



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
        xor     %rax,%rax
        mov     %rax,(%rax)
asm_builtin_checkcast:
        xor     %rax,%rax
        mov     %rax,(%rax)
asm_builtin_checkarraycast:     
        xor     %rax,%rax
        mov     %rax,(%rax)
asm_builtin_anewarray:  
        xor     %rax,%rax
        mov     %rax,(%rax)
asm_builtin_newarray_array:     
        xor     %rax,%rax
        mov     %rax,(%rax)
asm_builtin_monitorenter:       
        xor     %rax,%rax
        mov     %rax,(%rax)
asm_builtin_monitorexit:        
        xor     %rax,%rax
        mov     %rax,(%rax)
asm_builtin_idiv:       
        xor     %rax,%rax
        mov     %rax,(%rax)
asm_builtin_irem:       
        xor     %rax,%rax
        mov     %rax,(%rax)
asm_builtin_ldiv:       
        xor     %rax,%rax
        mov     %rax,(%rax)
asm_builtin_lrem:       
        xor     %rax,%rax
        mov     %rax,(%rax)
asm_builtin_frem:       
        xor     %rax,%rax
        mov     %rax,(%rax)
asm_builtin_arrayinstanceof:    
        xor     %rax,%rax
        mov     %rax,(%rax)
asm_builtin_trace:      
        xor     %rax,%rax
        mov     %rax,(%rax)
asm_builtin_exittrace:
        xor     %rax,%rax
        mov     %rax,(%rax)

#if 0
asm_dumpregistersandcall:
		push	%ebx
		push	%ebp
		push	%esi
		push	%edi
				
		mov     8(%ebp),%eax		/* load function pointer */
		call	*%eax				/* call function */

		pop		%edi
		pop		%esi
		pop		%ebp
		pop		%ebx
		
		ret
#endif

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
		add     $8,%rsp						/* clear return address of native stub */
		
asm_handle_exception:
        mov     %rdx,%rdi
        mov     %rax,%rsi
		call	findmethod
		mov     %rax,%rcx

		push    %rbp
		mov     %rsp,%rbp

		push    %rax						/* save exception pointer         */
/*		sub     $2,%edx */
		push    %rdx						/* save exception pc              */
		push	%rcx						/* save data segment pointer      */
        
		push	%rbx
		push	%rsi
		push	%rdi
		
ex_stack_loop:
		mov     %rax,%rdi					/* exception pointer */
		mov     MethodPointer(%rcx),%rsi	/* method pointer */
		mov     $1,%rcx 					/* set no unwind flag */
		call	builtin_trace_exception

        xor     %rax,%rax
        mov     %rax,(%rax)
        
#if 0
		mov     -12(%ebp),%esi				/* %esi = data segment pointer */
		mov     ExTableSize(%esi),%ecx		/* %ecx = exception table size */
		test	%ecx,%ecx					/* if empty table skip */
		je		empty_table

		lea		ExTableStart(%esi),%edi		/* %edi = start of exception table */
		mov     -4(%ebp),%eax				/* get xptr */
		
ex_table_loop:
		mov     -8(%ebp),%edx				/* get xpc */
		
		mov     ExStartPC(%edi),%ebx		/* %ebx = exception start pc */
		cmp     %edx,%ebx					/* %ebx = (startpc <= xpc) */
		jg		ex_table_cont				/* if (false) continue */
		mov     ExEndPC(%edi),%ebx			/* %ebx = exception end pc */
		cmp     %ebx,%edx					/* %ebx = (xpc < endpc) */
		jge		ex_table_cont				/* if (false) continue */
		mov     ExCatchType(%edi),%ebx		/* arg1 = exception catch type */
		test	%ebx,%ebx					/* NULL catches everything */
		je		ex_handle_it

		mov     offobjvftbl(%eax),%esi		/* %esi = vftblptr(xptr) */
		mov     offobjvftbl(%ebx),%ebx		/* %ebx = vftblptr(catchtype) class (not obj) */
		mov     offbaseval(%esi),%esi		/* %esi = baseval(xptr) */
		mov     offbaseval(%ebx),%edx		/* %edx = baseval(catchtype) */
		mov     offdiffval(%ebx),%ebx		/* %ebx = diffval(catchtype) */
		sub     %edx,%esi					/* %esi = baseval(xptr) - baseval(catchtype) */
		cmp     %ebx,%esi					/* xptr is instanceof catchtype */
		ja		ex_table_cont
		
ex_handle_it:
		mov     ExHandlerPC(%edi),%edx
		
		pop     %edi
		pop     %esi
		pop     %ebx
        
        pop     %eax                        /* pop %ecx (dummy) */
        pop     %eax                        /* pop %edx (dummy) */
        pop     %eax                        /* pop %eax */
        
		leave

		jmp		*%edx

ex_table_cont:
		lea		ExEntrySize(%edi),%edi
		dec     %ecx
		test	%ecx,%ecx
		jg		ex_table_loop
		
empty_table:
		pop     %edi
		pop    	%esi
		pop     %ebx
		pop     %ecx						/* restore data segment pointer   */
		pop     %edx
		pop     %eax						/* restore exception pointer      */
		pop     %ebp

		mov     %eax,%edi					/* save exception pointer         */
				
ex_already_cleared:		
		mov     IsSync(%ecx),%eax			/* %eax = SyncOffset              */
		test	%eax,%eax					/* if zero no monitorexit         */
		je		no_monitor_exit
		
		add     %esp,%eax
		mov     -8(%eax),%eax
		push    %eax
		call	builtin_monitorexit
		add     $4,%esp
        
no_monitor_exit:
		mov     FrameSize(%ecx),%eax		/* %eax = frame size              */
		add     %eax,%esp					/* unwind stack                   */
		mov     %esp,%eax					/* %eax = pointer to save area    */

		mov     IntSave(%ecx),%edx			/* %edx = saved int register count */
		test	%edx,%edx
		je		noint
		cmp     $1,%edx
		je		int1
		
int2:	
		mov     -16(%eax),%ebx

int1:	
		mov     -8(%eax),%ebp

		shl     $3,%edx						/* multiply by 8 bytes             */
		sub     %edx,%eax
		
noint:
		mov     FltSave(%ecx),%edx			/* %edx = saved flt register count */
		test	%edx,%edx
		je		noflt
		cmp     $1,%edx
		je		flt1
		cmp     $2,%edx
		je		flt2
		cmp     $3,%edx
		je		flt3
		
flt4:	
		fldl	-32(%eax)
		fstp	%st(1)

flt3:	
		fldl	-24(%eax)
		fstp	%st(2)
		
flt2:	
		fldl	-16(%eax)
		fstp	%st(3)
		
flt1:	
		fldl	-8(%eax)
		fstp	%st(4)
		
noflt:					
		pop     %edx						/* the new xpc is return address  */
		sub     $2,%edx
		
		push	%edx
		push	%ebx
		push	%ebp
		push	%esi
		push	%edi
								
		push	%edx						/* get the new data segment ptr   */
		call	findmethod
		mov     %eax,%ecx
		add     $4,%esp

		pop     %edi
		pop     %esi
		pop     %ebp
		pop     %ebx
		pop     %edx
		
		mov     %edi,%eax					/* restore saved exception pointer */
				
		push    %ebp
		mov     %esp,%ebp

		push    %eax						/* save exception pointer         */
		push    %edx						/* save exception pc              */
		push    %ecx						/* save data segment pointer      */
		push    %ebx
		push    %esi
		push    %edi
		
		jmp		ex_stack_loop
#endif		

/********************* function asm_builtin_monitorenter ***********************
*                                                                              *
*   Does null check and calls monitorenter or throws an exception              *
*                                                                              *
*******************************************************************************/
#if 0
asm_builtin_monitorenter:
		cmpl	$0,4(%esp)
		je		nb_monitorenter			/* if (null) throw exception          */
		jmp		builtin_monitorenter	/* else call builtin_monitorenter     */

nb_monitorenter:
		popl	%edx					/* delete return address */
		subl	$2,%edx					/* faulting address is return adress - 2 */
		movl	proto_java_lang_NullPointerException,%eax
		jmp		asm_handle_exception
		

/********************* function asm_builtin_monitorexit ************************
*                                                                              *
*   Does null check and calls monitorexit or throws an exception               *
*                                                                              *
*******************************************************************************/

asm_builtin_monitorexit:
		cmpl	$0,4(%esp)
		je		nb_monitorexit			/* if (null) throw exception          */
		jmp		builtin_monitorexit		/* else call builtin_monitorenter     */

nb_monitorexit:
		popl	%edx					/* delete return address */
		subl	$2,%edx					/* faulting address is return adress - 2 */
		movl	proto_java_lang_NullPointerException,%eax
		jmp		asm_handle_exception


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
		subl	$2,%edx					/* faulting address is return adress - 2 */
		
		movl	proto_java_lang_ArithmeticException,%eax
		jmp		asm_handle_exception
		

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
		subl	$2,%edx					/* faulting address is return adress - 2 */
		
		movl	proto_java_lang_ArithmeticException,%eax
		jmp		asm_handle_exception
				

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
		subl	$2,%edx					/* faulting address is return adress - 2 */
		
		movl	proto_java_lang_ArithmeticException,%eax
		jmp		asm_handle_exception


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
		subl	$2,%edx					/* faulting address is return adress - 2 */
		
		movl	proto_java_lang_ArithmeticException,%eax
		jmp		asm_handle_exception
		

/*********************** function new_builtin_checkcast ************************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/

asm_builtin_checkcast:
		xorl	%eax,%eax
		movl	$0,(%eax)
		ret

		
/******************* function asm_builtin_checkarraycast ***********************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/

asm_builtin_checkarraycast:
		subl	$8,%esp					/* build stack frame (2 * 4 bytes)    */

		movl	12(%esp),%eax           /* 8 (frame) + 4 (return)             */
		movl	%eax,(%esp)				/* save object pointer                */

		movl	20(%esp),%eax
		movl	%eax,4(%esp)

		call	builtin_checkarraycast  /* builtin_checkarraycast             */
	
		test	%eax,%eax               /* if (false) throw exception         */
		je		nb_carray_throw

		movl	12(%esp),%eax			/* return object pointer              */
		addl	$8,%esp
		ret

nb_carray_throw:
		addl	$8,%esp
		
		popl	%edx					/* delete return address              */
		subl	$2,%edx					/* faulting address is return adress - 2 */
		
		movl	proto_java_lang_ClassCastException,%eax
		jmp		asm_handle_exception

		
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

		
/******************* function asm_builtin_newarray_array ***********************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/

asm_builtin_newarray_array:
		subl	$8,%esp					/* build stack frame (2 * 4 bytes) */

		movl	12(%esp),%eax
		movl	%eax,(%esp)

		movl	20(%esp),%eax
		movl	%eax,4(%esp)

		call	builtin_newarray_array
	
		addl	$8,%esp
		ret

		
/******************* function asm_builtin_aastore ******************************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/
#endif
asm_builtin_aastore:
        sub     $24,%rsp         		/* allocate stack space               */

        test    %rdi,%rdi				/* if null pointer throw exception    */
        je      nb_aastore_null

        mov     offarraysize(%rdi),%rax /* load size                          */
        cmp     %rax,%rsi				/* do bound check                     */
        ja      nb_aastore_bound		/* if out of bounds throw exception   */

        shl     $3,%rsi					/* index * 8                          */
        mov     %rdi,%r10
        add     %rsi,%r10				/* add index * 8 to arrayref          */
		
        mov     %r10,(%rsp)     		/* save store position                */
        mov     %rdx,8(%rsp)            /* save object                        */
        
        mov     %rdx,%rsi               /* object is second argument          */
        call    builtin_canstore		/* builtin_canstore(arrayref,object)  */
        test    %rax,%rax				/* if (false) throw exception         */
        je      nb_aastore_throw

        mov     (%rsp),%r10             /* restore store position             */
        mov     8(%rsp),%rdx            /* restore object                     */
        mov     %rdx,offobjarrdata(%r10)/* store objectptr in array           */
        add     $24,%rsp                /* free stack space                   */
        ret

nb_aastore_null:
        add     $24,%rsp
        pop     %r10					/* delete return address */
        sub     $2,%r10					/* faulting address is return adress - 2 */
		
        mov     proto_java_lang_NullPointerException,%rax
        jmp     asm_handle_exception

nb_aastore_bound:
        add     $24,%rsp
        pop     %r10					/* delete return address */
        sub     $2,%r10					/* faulting address is return adress - 2 */
		
        mov     proto_java_lang_ArrayIndexOutOfBoundsException,%rax
        jmp     asm_handle_exception
		
nb_aastore_throw:
        add     $24,%rsp
        pop     %r10					/* delete return address */
        sub     $2,%r10					/* faulting address is return adress - 2 */
		
        mov     proto_java_lang_ArrayStoreException,%rax
        jmp		asm_handle_exception
#if 0
		
/******************* function asm_builtin_arrayinstanceof **********************
*                                                                              *
*   Does the instanceof check of arrays                                        *
*                                                                              *
*******************************************************************************/

asm_builtin_arrayinstanceof:
		subl	$8,%esp					/* build stack frame (2 * 4 bytes) */

		movl	12(%esp),%eax
		movl	%eax,(%esp)

		movl	20(%esp),%eax
		movl	%eax,4(%esp)

		call	builtin_arrayinstanceof
	
		addl	$8,%esp
		ret

		
/******************* function asm_initialize_thread_stack **********************
*                                                                              *
* initialized a thread stack                                                   *
* (to)->restorePoint = asm_initialize_thread_stack((u1*)(func), (to)->stackEnd)*
*                                                                              *
*******************************************************************************/
#endif
asm_initialize_thread_stack:
        sub     $56,%rsi

        xor     %r10,%r10
        mov     %r10,0*8(%rsi)
        mov     %r10,1*8(%rsi)
        mov     %r10,2*8(%rsi)
        mov     %r10,3*8(%rsi)
        mov     %r10,4*8(%rsi)
        mov     %r10,5*8(%rsi)

        mov     %rdi,6*8(%rsi)          /* save (u1*) (func)                  */
        ret                             /* return restorepoint in %eax        */


/******************* function asm_perform_threadswitch *************************
*                                                                              *
*   void asm_perform_threadswitch (u1 **from, u1 **to, u1 **stackTop);         *
*                                                                              *
*   performs a threadswitch                                                    *
*                                                                              *
*******************************************************************************/

asm_perform_threadswitch:
        sub     $56,%rsp

        mov     %rbx,0*8(%rsp)
        mov     %rbp,1*8(%rsp)
        mov     %r12,2*8(%rsp)
        mov     %r13,3*8(%rsp)
        mov     %r14,4*8(%rsp)
        mov     %r15,5*8(%rsp)

        mov     64(%rsp),%rax			/* save current return address        */
        mov     %rax,6*8(%rsp)

        mov     %rsp,(%rdi)     		/* first argument **from              */
        mov     %rsp,(%rdx)     		/* third argument **stackTop          */

        mov     (%rsi),%rsp     		/* load new stack pointer             */

        mov     0*8(%rsp),%rbx
        mov     1*8(%rsp),%rbp
        mov     2*8(%rsp),%r12
        mov     3*8(%rsp),%r13
        mov     4*8(%rsp),%r14
        mov     5*8(%rsp),%r15
      
        add     $48,%rsp                /* leave return address on stack      */
        ret
		

/********************* function asm_switchstackandcall *************************
*                                                                              *
*  int asm_switchstackandcall (void *stack, void *func, void **stacktopsave,   *
*		                       void *p);                                       *
*                                                                              *
*   Switches to a new stack, calls a function and switches back.               *
*       a0 (%rdi)     new stack pointer                                        *
*       a1 (%rsi)     function pointer                                         *
*		a2 (%rdx)     pointer to variable where stack top should be stored     *
*       a3 (%rcx)     pointer to user data, is passed to the function          *
*                                                                              *
*******************************************************************************/

asm_switchstackandcall:
        sub     $8,%rsp                 /* keep stack 16-byte aligned         */
        sub     $16,%rdi				/* allocate new stack                 */

        mov     8(%rsp),%rax			/* save return address on new stack */
        mov     %rax,(%rdi)
        mov     %rsp,8(%rdi)			/* save old stack pointer on new stack */
        mov     %rsp,(%rdx)				/* save old stack pointer to variable */

        mov     %rdi,%rsp				/* switch to new stack */

        mov     %rcx,%rdi       		/* pass pointer */
        call	*%rsi					/* and call function */

        mov     (%rsp),%r10				/* load return address */
        mov     8(%rsp),%rsp			/* switch to old stack */
        add     $8,%rsp
        mov     %r10,(%rsp)
        ret

		
/********************* function asm_getcallingmethod ***************************
*                                                                              *
*   classinfo *asm_getcallingmethod ();         							   *
*																			   *	
*   goes back stack frames to get the calling method						   *	   
*																			   *	
*				t2 .. sp												       *
*				t3 .. ra												       *
*				t4 .. pv												       *
*                                                                              *
*******************************************************************************/

asm_getcallingmethod:
		xor     %rax,%rax
/*		mov     $0,(%rax) */
		ret
#if 0

/*********************** function asm_builtin_trace ****************************
*                                                                              *
*   Intended to be called from the native stub. Saves all argument registers   *
*   and calls builtin_trace_args.                                              *
*                                                                              *
*******************************************************************************/

asm_builtin_trace:
        pusha
        subl    $68,%esp                /* 4 + 8 * 4 + 68 = 104 */

        movl    104(%esp),%eax
        movl    108(%esp),%edx
        movl    %eax,(%esp)
        movl    %edx,4(%esp)

        movl    112(%esp),%eax
        movl    116(%esp),%edx
        movl    %eax,8(%esp)
        movl    %edx,12(%esp)

        movl    120(%esp),%eax
        movl    124(%esp),%edx
        movl    %eax,16(%esp)
        movl    %edx,20(%esp)

        movl    128(%esp),%eax
        movl    132(%esp),%edx
        movl    %eax,24(%esp)
        movl    %edx,28(%esp)

        movl    136(%esp),%eax
        movl    140(%esp),%edx
        movl    %eax,32(%esp)
        movl    %edx,36(%esp)

        movl    144(%esp),%eax
        movl    148(%esp),%edx
        movl    %eax,40(%esp)
        movl    %edx,44(%esp)

        movl    152(%esp),%eax
        movl    156(%esp),%edx
        movl    %eax,48(%esp)
        movl    %edx,52(%esp)

        movl    160(%esp),%eax
        movl    164(%esp),%edx
        movl    %eax,56(%esp)
        movl    %edx,60(%esp)

        movl    168(%esp),%eax
        movl    %eax,64(%esp)
        
        call    builtin_trace_args
        addl    $68,%esp

        popa
        ret


/********************* function asm_builtin_exittrace **************************
*                                                                              *
*   Intended to be called from the native stub. Saves return value and calls   *
*   builtin_displaymethodstop.                                                 *
*                                                                              *
*******************************************************************************/

asm_builtin_exittrace:
        pusha
        subl    $24,%esp
        
        movl    60(%esp),%eax           /* 4 + 8 * 4 + 24 = 60 */
        movl    %eax,(%esp)

        movl    64(%esp),%eax
        movl    68(%esp),%edx
        movl    %eax,4(%esp)
        movl    %edx,8(%esp)

        movl    72(%esp),%eax
        movl    76(%esp),%edx
        movl    %eax,12(%esp)
        movl    %edx,16(%esp)

        movl    80(%esp),%eax
        movl    %eax,20(%esp)

        call    builtin_displaymethodstop
        addl    $24,%esp

        popa
        ret
#endif
