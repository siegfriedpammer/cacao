/* jit/i386/asmpart.S - Java-C interface functions for i386

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
            Christian Thalinger

   $Id: asmpart.S 1140 2004-06-05 20:58:14Z twisti $

*/


#include "config.h"
#include "offsets.h"

/* data segment offsets */

#define MethodPointer   	-4
#define FrameSize       	-8
#define IsSync          	-12
#define IsLeaf          	-16
#define IntSave         	-20
#define FltSave         	-24
#define LineNumberTableSize	-28
#define LineNumberTableStart	-32
#define ExTableSize     	-36
#define ExTableStart    	-36

#define ExEntrySize     -16
#define ExStartPC       -4
#define ExEndPC         -8
#define ExHandlerPC     -12
#define ExCatchType     -16


#define LineEntrySize	-8
#define LinePC		0
#define LineLine	-4


#define itmp1    %eax
#define itmp2    %ecx
#define itmp3    %edx

		
	.text


/********************* exported functions and variables ***********************/

	.globl asm_calljavafunction
	.globl calljava_xhandler
	.globl asm_calljavafunction2
	.globl asm_calljavafunction2long
	.globl asm_calljavafunction2double
	.globl calljava_xhandler2

	.globl asm_call_jit_compiler
	.globl asm_handle_builtin_exception
	.globl asm_handle_nat_exception
	.globl asm_handle_exception
	.globl asm_check_clinit
	.globl asm_builtin_checkcast	
	.globl asm_builtin_checkarraycast
	.globl asm_builtin_newarray
	.globl asm_builtin_anewarray
	.globl asm_builtin_newarray_array
	.globl asm_builtin_aastore
	.globl asm_builtin_monitorenter
	.globl asm_builtin_monitorexit
	.globl asm_builtin_ldiv
	.globl asm_builtin_lrem
    .globl asm_builtin_f2i
    .globl asm_builtin_f2l
    .globl asm_builtin_d2i
    .globl asm_builtin_d2l
	.globl asm_builtin_arrayinstanceof
	.globl asm_perform_threadswitch
	.globl asm_initialize_thread_stack
	.globl asm_switchstackandcall
	.globl asm_getcallingmethod
	.globl Java_java_lang_VMSecurityManager_getClassContext
	.globl Java_java_lang_VMSecurityManager_currentClassLoader    
	.globl asm_builtin_new
	.globl asm_get_stackTrace
	.globl asm_criticalsections
	.globl asm_getclassvalues_atomic

/*************************** imported functions *******************************/

	.globl jit_compile
	.globl builtin_monitorexit
	.globl builtin_throw_exception
	.globl builtin_trace_exception
	.globl class_java_lang_Object
	.globl codegen_findmethod
/*	.globl codegen_findmethod1*/
	.globl builtin_asm_createclasscontextarray
	.globl builtin_asm_getclassloader
	.globl callgetexceptionptrptr
	.globl asm_throw_and_handle_exception
	.globl asm_throw_and_handle_hardware_arithmetic_exception


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

call_name:
	.ascii  "calljavafunction\0\0"

	.align	8
	.long   0                         /* catch type all                       */
	.long   calljava_xhandler         /* handler pc                           */
	.long   calljava_xhandler         /* end pc                               */
	.long   asm_calljavafunction      /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0			  /* line number table	start		  */
	.long   0			  /* line number table	size		  */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   32                        /* frame size                           */
	.long   0                         /* method pointer (pointer to name)     */

asm_calljavafunction:
	push    %ebp                      /* allocate stack space                 */
	mov     %esp, %ebp

	push    %ebx                      /* save registers                       */
	push    %esi
	push    %edi
        
	sub     $32,%esp                  /* pass the remaining parameters        */
	xor     %edx,%edx

	mov     %edx,28(%esp)             /* convert parms to 8 byte              */
	mov     24(%ebp),%eax
	mov     %eax,24(%esp)
		
	mov     %edx,20(%esp)
	mov     20(%ebp),%eax
	mov     %eax,16(%esp)

	mov     %edx,12(%esp)
	mov     16(%ebp),%eax
	mov     %eax,8(%esp)

	mov     %edx,4(%esp)
	mov     12(%ebp),%eax
	mov     %eax,(%esp)

	mov     8(%ebp),%eax              /* move function pointer to %eax        */

	lea     asm_call_jit_compiler,%edx 
	call    *%edx                     /* call JIT compiler                    */
	
calljava_return:
	add     $32,%esp
	pop     %edi                      /* restore registers                    */
	pop     %esi
	pop     %ebx
	leave
	ret

calljava_xhandler:
	push    %eax                      /* pass exception pointer               */
	call    builtin_throw_exception
	add     $4,%esp

	add     $32,%esp
	pop     %edi                      /* restore registers                    */
	pop     %esi
	pop     %ebx
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
*    javaobject_header *asm_calljavafunction2(methodinfo *m,                   *
*         u4 count, u4 size, void *callblock);                                 *
*                                                                              *
*******************************************************************************/

call_name2:
	.ascii  "calljavafunction2\0\0"

	.align	8
	.long   0                         /* catch type all                       */
	.long   calljava_xhandler2        /* handler pc                           */
	.long   calljava_xhandler2        /* end pc                               */
	.long   asm_calljavafunction2     /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* line number table start              */
	.long   0                         /* line number table size               */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   32                        /* frame size                           */
	.long   0                         /* method pointer (pointer to name)     */

asm_calljavafunction2:
asm_calljavafunction2double:
asm_calljavafunction2long:
	push    %ebp
	mov     %esp,%ebp                 /* save stackptr                        */

	push    %ebx                      /* save registers                       */
	push    %esi
	push    %edi

	mov     20(%ebp),%eax             /* pointer to arg block                 */
	mov     12(%ebp),%ecx             /* arg count                            */
	test    %ecx,%ecx                 /* maybe we have no args                */
	jle     calljava_copydone

	mov     %ecx,%edx                 /* calculate stack size                 */
	shl     $3,%edx
	mov     %edx,%esi                 /* save in callee saved register        */
	sub     %esi,%esp                 /* stack frame for arguments            */
	mov     %esp,%edi

calljava_copyloop:
    mov     offjniitem(%eax),%edx
    mov     %edx,0(%edi)
    mov     offjniitem+4(%eax),%edx
    mov     %edx,4(%edi)

	sub     $1,%ecx                   /* are there any args left?             */
	test    %ecx,%ecx
	jle     calljava_copydone

	add     $sizejniblock,%eax        /* goto next argument block             */
    add     $8,%edi                   /* increase sp to next argument         */
	jmp     calljava_copyloop

calljava_copydone:
	mov     8(%ebp),%eax              /* move function pointer to %eax        */

	lea     asm_call_jit_compiler,%edx 
	call    *%edx                     /* call JIT compiler                    */
	
calljava_return2:
	add     %esi,%esp                 /* remove arg stack frame               */
	pop     %edi                      /* restore registers                    */
	pop     %esi
	pop     %ebx
	leave
	ret

calljava_xhandler2:
	push    %eax                      /* pass exception pointer               */
	call    builtin_throw_exception
	add     $4,%esp
    
	add     %esi,%esp                 /* remove arg stack frame               */
	pop     %edi                      /* restore registers                    */
	pop     %esi
	pop     %ebx
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
*   i386_mov_membase_reg(REG_SP, 0, REG_ITMP1)    ; invokevirtual/interface    *
*   i386_mov_membase_reg(REG_ITMP1, OFFSET(, vftbl), REG_ITMP2)                *
*   i386_mov_membase_reg(REG_ITMP2, OFFSET(vftbl, table[0]) + \                *
*       sizeof(methodptr) * m->vftblindex, REG_ITMP1)                          *
*   i386_call_reg(REG_ITMP1)                                                   *
*                                                                              *
*   in the static case the method pointer can be computed using the            *
*   return address and the lda function following the jmp instruction          *
*                                                                              *
*******************************************************************************/

asm_call_jit_compiler:
	push	%ebx                /* save register                              */
    push    %ebp
			
	mov     8(%esp),%ebp        /* get return address (2 push)                */
	mov     -1(%ebp),%bl        /* get function code                          */
	cmp     $0xd1,%bl           /* called with `call *REG_ITMP2' (%ecx)?      */
	jne		L_not_static_special

	sub     $6,%ebp             /* calculate address of immediate             */
	jmp		L_call_jit_compile
		
L_not_static_special:
	cmp     $0xd0,%bl           /* called with `call *REG_ITMP1' (%eax)       */
	jne		L_not_virtual_interface
	
	sub     $6,%ebp             /* calculate address of offset                */
	mov     (%ebp),%ebp         /* get offset                                 */
	add     itmp2,%ebp          /* add base address to get method address     */
	jmp		L_call_jit_compile

L_not_virtual_interface:        /* a call from asm_calljavafunction           */
	xor     %ebp,%ebp
		
L_call_jit_compile:
	push    %ebp                /* save address for method pointer            */

	push    %eax                /* push methodpointer on stack                */
	call    jit_compile
	add     $4,%esp

	pop     %ebp                /* restore address for method pointer         */

	test    %eax,%eax           /* check for exception                        */
	je      L_exception

	test	%ebp,%ebp           /* is address == 0 (asm_calljavafunction)     */
	je		L_call_method
	
	mov     %eax,(%ebp)         /* and now save the new pointer               */

L_call_method:
	pop     %ebp                /* restore registers                          */
	pop     %ebx
		
	jmp		*%eax               /* ...and now call the new method             */

L_exception:
	pop     %ebp                /* restore registers                          */
	pop     %ebx

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	call    builtin_asm_get_exceptionptrptr
	mov     %eax,%ecx
	mov     (%ecx),%eax         /* get the exception pointer                  */
	movl    $0,(%ecx)           /* clear the exception pointer                */
#else
	lea     _exceptionptr,%ecx
	mov     (%ecx),%eax         /* get the exception pointer                  */
	movl    $0,(%ecx)           /* clear the exception pointer                */
#endif

	pop     %ecx                /* delete return address                      */
	sub     $2,%ecx             /* faulting address is return adress - 2      */

L_refillinStacktrace:
	push %ecx		/* store fault adress */
	push %eax		/* temporarily save exception pointer*/
	call builtin_asm_get_stackframeinfo
	push %eax	/* save location of thread specific stack info head pointer */
	mov (%eax),%ecx /* save old value of pointer*/
	push %ecx
	mov %esp,(%eax) /*store pointer to this structure*/
	mov 8(%esp),%eax  /* get the exception pointer again*/
	movl $0,8(%esp) /*mark this block as native*/
	push $0 /*used for the jni_callblock structure*/
	push %eax /*save eax for later */
	/* get fillInStackTrace method*/
	push utf_fillInStackTrace_desc
	push utf_fillInStackTrace_name
	mov offobjvftbl(%eax),%ecx
	mov offclass(%ecx),%eax
	push %eax
	call class_resolvemethod
	add $12,%esp
	push $0
	push $4 /*TYPE_ADR*/
	push %esp
	push $sizejniblock
	push $1
	push %eax
	call asm_calljavafunction2
	add $24,%esp

	/*remove native stack info */
	mov 8(%esp),%ecx
	mov 12(%esp),%eax
	mov %ecx,(%eax)
	mov (%esp),%eax
	add $20,%esp
	pop %ecx

	
	jmp     asm_handle_exception


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
		add     $4,%esp						/* clear return address of native stub */
		
asm_handle_exception:
#if 0
		push %ebp
		mov %esp,%ebp
		push %eax	/* exception pointer */
		push %ecx	/* excepiton pc */

		call asm_get_stackTrace

		pop %ecx
		pop %eax
		pop %ebp
#endif
asm_handle_exception_loop:
		push    %ebp
		mov     %esp,%ebp
        
		push    %eax						/* save exception pointer         */
		push    %ecx                        /* save exception pc              */

		call    codegen_findmethod          /* get the data segment ptr           */
		mov     %eax,%edx
		        
		mov     -4(%ebp),%eax
		mov     -8(%ebp),%ecx               /* could be changed in findmethod */

		push    %edx						/* save data segment pointer      */
		push    %ebx
		push    %esi
		push    %edi
		
ex_stack_loop:
		sub     $20,%esp
		mov     %eax,(%esp)					/* exception pointer              */
		mov     MethodPointer(%edx),%eax	/* method pointer                 */
		mov     %eax,4(%esp)
		mov     %ecx,8(%esp)				/* exception pc                   */
		movl	$0,12(%esp)                 /* line number                    */
		movl    $1,16(%esp)					/* set no unwind flag             */
		call	builtin_trace_exception
		add     $20,%esp
		mov     -12(%ebp),%esi				/* %esi = data segment pointer    */
		mov     ExTableSize(%esi),%ecx		/* %ecx = exception table size    */
		test    %ecx,%ecx					/* if empty table skip            */
		je      empty_table

		lea		ExTableStart(%esi),%edi		/* %edi = start of exception table*/
		mov     -4(%ebp),%eax               /* get xptr                       */
		
ex_table_loop:
		mov     -8(%ebp),%edx				/* get xpc                        */

		mov     ExStartPC(%edi),%ebx		/* %ebx = exception start pc      */
		cmp     %edx,%ebx					/* %ebx = (startpc <= xpc)        */
		jg      ex_table_cont				/* if (false) continue            */
		mov     ExEndPC(%edi),%ebx			/* %ebx = exception end pc        */
		cmp     %ebx,%edx					/* %ebx = (xpc < endpc)           */
		jge     ex_table_cont				/* if (false) continue            */
		mov     ExCatchType(%edi),%ebx		/* arg1 = exception catch type    */
		test    %ebx,%ebx					/* NULL catches everything        */
		je      ex_handle_it

	cmpl    $0,offclassloaded(%ebx)     /* check if class is loaded           */
	jne     L_class_loaded

	sub     $3*4,%esp
	mov     %eax,1*4(%esp)              /* save not callee saved regs         */
	mov     %ecx,2*4(%esp)

	mov     %ebx,0*4(%esp)              /* exception class is argument        */
	call    class_load

	mov     1*4(%esp),%eax
	mov     2*4(%esp),%ecx
	add     $3*4,%esp

L_class_loaded:
	cmpl    $0,offclasslinked(%ebx)
	jne     L_class_linked

	sub     $3*4,%esp
	mov     %eax,1*4(%esp)              /* save not callee saved regs         */
	mov     %ecx,2*4(%esp)

	mov     %ebx,0*4(%esp)              /* exception class is argument        */
	call    class_link

	mov     1*4(%esp),%eax
	mov     2*4(%esp),%ecx
	add     $3*4,%esp

L_class_linked:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
		push    %ebx

_crit_restart1:
		mov     0(%esp),%ebx
#endif
		
_crit_begin1:
		mov     offobjvftbl(%eax),%esi		/* %esi = vftblptr(xptr)          */
		mov     offclassvftbl(%ebx),%ebx    /* %ebx = vftblptr(catchtype) class (not obj) */
		mov     offbaseval(%esi),%esi		/* %esi = baseval(xptr)           */
		mov     offbaseval(%ebx),%edx		/* %edx = baseval(catchtype)      */
		mov     offdiffval(%ebx),%ebx		/* %ebx = diffval(catchtype)      */
_crit_end1:
		sub     %edx,%esi					/* %esi = baseval(xptr) - baseval(catchtype) */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
		add     $4,%esp
#endif
        
		cmp     %ebx,%esi					/* xptr is instanceof catchtype   */
		ja      ex_table_cont
		
ex_handle_it:
		mov     ExHandlerPC(%edi),%edx
		
		pop     %edi                        /* restore registers              */
		pop     %esi
		pop     %ebx
        add     $8,%esp                     /* suck %ecx, %edx                */
        pop     %eax                        /* restore xptr                   */

		leave
		jmp		*%edx                       /* jump to exception handler      */

ex_table_cont:
		lea     ExEntrySize(%edi),%edi
		dec     %ecx
		test    %ecx,%ecx
		jg      ex_table_loop
		
empty_table:
        pop     %edi
        pop     %esi
        pop     %ebx
        pop     %edx                        /* restore data segment pointer   */
        pop     %ecx
        pop     %eax
        pop     %ebp

        push    %eax                        /* save exception pointer         */
        
ex_already_cleared:
		mov     IsSync(%edx),%eax			/* %eax = SyncOffset              */
		test    %eax,%eax					/* if zero no monitorexit         */
		je      no_monitor_exit

        add     %esp,%eax
        mov     -4(%eax),%eax               /* we have the xptr on the stack  */
        push    %edx                        /* save regs                      */
        push    %eax
		call    builtin_monitorexit
		add     $4,%esp
        pop     %edx                        /* restore regs                   */
        
no_monitor_exit:
        mov     %esp,%eax
        add     FrameSize(%edx),%eax        /* %eax = frame size              */
        add     $4,%eax                     /* we have the xptr on the stack  */
        
   		mov     IntSave(%edx),%ecx          /* %ecx = saved int register count*/
		test    %ecx,%ecx
		je      noint
		cmp     $1,%ecx
		je      int1
		cmp     $2,%ecx
		je      int2
		cmp     $3,%ecx
		je      int3

int4:	
		mov     -32(%eax),%ebx

int3:	
		mov     -24(%eax),%ebp

int2:	
		mov     -16(%eax),%esi

int1:	
		mov     -8(%eax),%edi

   		shl     $3,%ecx						/* multiply by 8 bytes             */
		sub     %ecx,%eax
		
noint:
		mov     FltSave(%edx),%ecx			/* %ecx = saved flt register count */
		test    %ecx,%ecx
		je      noflt
		cmp     $1,%ecx
		je      flt1
		cmp     $2,%ecx
		je      flt2
		cmp     $3,%ecx
		je      flt3
		
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
        pop     %eax                        /* restore exception pointer      */
        
        mov     FrameSize(%edx),%ecx        /* %ecx = frame size              */
        add     %ecx,%esp                   /* unwind stack                   */
        
		pop     %ecx                        /* the new xpc is return address  */
		sub     $2,%ecx
		
		jmp		asm_handle_exception_loop
		

/********************* function asm_check_clinit *******************************
*                                                                              *
*   Does null check and calls monitorenter or throws an exception              *
*                                                                              *
*******************************************************************************/

asm_check_clinit:
	mov     offclassinit(%eax),%ecx     /* get initialized flag               */
	test	%ecx,%ecx
	jnz     L_is_initialized

	sub     $16,%esp                    /* build stack frame (4 * 4 bytes)    */
	mov     %eax,(%esp)
	call    builtin_asm_get_stackframeinfo

	movl    $0,12(%esp)
    mov     %eax,8(%esp)
	mov     (%eax),%ecx
	mov     %ecx,4(%esp)
	mov     %esp,%ecx
	add     $4,%ecx
	mov     %ecx,(%eax)

	call    class_init                  /* call class_init function           */

	mov     4(%esp),%ebx
	mov     8(%esp),%ecx
	mov     %ebx,(%ecx)

	add     $16,%esp

	test    %eax,%eax                   /* we had an exception                */
	je      L_initializererror

L_is_initialized:
	mov     (%esp),%eax                 /* get return address                 */
	sub     $12,%eax                    /* asm_putstatic call code size       */
	movb    $0xeb,(%eax)                /* jmp rel8                           */
	movb    $10,1(%eax)                 /* 8-bit offset                       */
	ret

L_initializererror:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	call    builtin_asm_get_exceptionptrptr
	mov     %eax,%ecx
	mov     (%ecx),%eax                 /* get the exception pointer          */
	movl    $0,(%ecx)                   /* clear the exception pointer        */
#else
	lea     _exceptionptr,%ecx
	mov     (%ecx),%eax                 /* get the exception pointer          */
	movl    $0,(%ecx)                   /* clear the exception pointer        */
#endif

	pop     %ecx                        /* delete return address              */
	sub     $2,%ecx                     /* faulting address is return adress - 2 */

	jmp     asm_handle_exception


/********************* function asm_builtin_monitorenter ***********************
*                                                                              *
*   Does null check and calls monitorenter or throws an exception              *
*                                                                              *
*******************************************************************************/

asm_builtin_monitorenter:
	cmpl    $0,4(%esp)
	je      nb_monitorenter             /* if (null) throw exception          */
	jmp		builtin_monitorenter        /* else call builtin_monitorenter     */

nb_monitorenter:
        mov string_java_lang_NullPointerException,%eax
        pop %ecx
        sub $2,%ecx
        jmp asm_throw_and_handle_exception

#if 0
	push    string_java_lang_NullPointerException
	call    new_exception
	add     $(1*4),%esp
	
	pop     %ecx                        /* delete return address              */
	sub     $2,%ecx                     /* faulting address is return adress - 2 */
	jmp     asm_handle_exception
#endif		

/********************* function asm_builtin_monitorexit ************************
*                                                                              *
*   Does null check and calls monitorexit or throws an exception               *
*                                                                              *
*******************************************************************************/

asm_builtin_monitorexit:
	mov     4(%esp),%eax
    test    %eax,%eax
	je      nb_monitorexit              /* if (null) throw exception          */
    push    %ecx                        /* save registers which could be used */
    push    %edx
    push    %eax
	call    builtin_monitorexit         /* else call builtin_monitorenter     */
    add     $4,%esp
    pop     %edx                        /* restore registers which could be used */
    pop     %ecx
    ret

nb_monitorexit:
        mov string_java_lang_NullPointerException,%eax
        pop %ecx
        sub $2,%ecx
        jmp asm_throw_and_handle_exception

#if 0
	push    string_java_lang_NullPointerException
	call    new_exception
	add     $(1*4),%esp
	
	pop     %ecx                    /* delete return address              */
	sub     $2,%ecx                 /* faulting address is return adress - 2 */
	jmp     asm_handle_exception
#endif

/************************ function asm_builtin_ldiv ****************************
*                                                                              *
*   Does null check and calls ldiv or throws an exception                      *
*                                                                              *
*******************************************************************************/

asm_builtin_ldiv:
	mov     12(%esp),%eax
	or      16(%esp),%eax
	test    %eax,%eax                   /* if (null) throw exception          */
	je      nb_ldiv

	jmp     builtin_ldiv

nb_ldiv:
	pop %ecx
	sub $2,%ecx
	jmp asm_throw_and_handle_hardware_arithmetic_exception
#if 0
	push    string_java_lang_ArithmeticException_message
	push    string_java_lang_ArithmeticException
	call    new_exception_message
	add     $(2*4),%esp
	
	pop     %ecx                        /* delete return address              */
	sub     $2,%ecx                     /* faulting address is return adress - 2 */
	jmp     asm_handle_exception
#endif				

/************************ function asm_builtin_lrem ****************************
*                                                                              *
*   Does null check and calls lrem or throws an exception                      *
*                                                                              *
*******************************************************************************/

asm_builtin_lrem:
	mov     12(%esp),%eax
	or      16(%esp),%eax
	test    %eax,%eax                   /* if (null) throw exception          */
	je      nb_lrem

	jmp     builtin_lrem

nb_lrem:
	pop %ecx
	sub $2,%ecx
	jmp asm_throw_and_handle_hardware_arithmetic_exception
#if 0
	push    string_java_lang_ArithmeticException_message
	push    string_java_lang_ArithmeticException
	call    new_exception_message
	add     $(2*4),%esp

	pop     %ecx                        /* delete return address              */
	sub     $2,%ecx                     /* faulting address is return adress - 2 */
	jmp     asm_handle_exception
#endif		

/************************ function asm_builtin_x2x *****************************
*                                                                              *
*   Wrapper functions for corner cases                                         *
*                                                                              *
*******************************************************************************/

asm_builtin_f2i:
	sub     $4,%esp
	fsts    (%esp)
	call    builtin_f2i
	add     $4,%esp
	ret

asm_builtin_d2i:
	sub     $8,%esp
	fstl    (%esp)
	call    builtin_d2i
	add     $8,%esp
	ret

asm_builtin_f2l:
	sub     $4,%esp
	fsts    (%esp)
	call    builtin_f2l
	add     $4,%esp
	ret

asm_builtin_d2l:
	sub     $8,%esp
	fstl    (%esp)
	call    builtin_d2l
	add     $8,%esp
	ret


/******************* function asm_builtin_checkarraycast ***********************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/

asm_builtin_checkarraycast:
	sub		$8,%esp                     /* build stack frame (2 * 4 bytes)    */

	mov		12(%esp),%eax               /* 8 (frame) + 4 (return)             */
	mov		%eax,(%esp)                 /* save object pointer                */

	mov		20(%esp),%eax
	mov		%eax,4(%esp)

	call	builtin_checkarraycast      /* builtin_checkarraycast             */

	test	%eax,%eax                   /* if (false) throw exception         */
	je		nb_carray_throw

	mov		12(%esp),%eax               /* return object pointer              */
	add		$8,%esp
	ret

nb_carray_throw:
	add $8,%esp
	mov string_java_lang_ClassCastException,%eax
	pop %ecx
	sub $2,%ecx
	jmp asm_throw_and_handle_exception
#if 0
	push    string_java_lang_ClassCastException
	call    new_exception
	add     $(1*4),%esp
	
	add		$8,%esp
	
	pop		%ecx                        /* delete return address              */
	sub		$2,%ecx                     /* faulting address is return adress - 2 */
	jmp		asm_handle_exception
#endif
		
/******************* function asm_builtin_newarray *****************************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/

asm_builtin_newarray:
	sub		$8,%esp                     /* build stack frame (2 * 4 bytes)    */

	mov		12(%esp),%eax
	mov		%eax,(%esp)

	mov		20(%esp),%eax
	mov		%eax,4(%esp)

	call	builtin_newarray

	add		$8,%esp
	ret

		
/******************* function asm_builtin_aastore ******************************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/

asm_builtin_aastore:
	sub     $12,%esp                    /* build stack frame (3 * 4 bytes)    */

	mov     16(%esp),%eax               /* 12 (frame) + 4 (return)            */
	test    %eax,%eax                   /* if null pointer throw exception    */
	je      nb_aastore_null

	mov     offarraysize(%eax),%edx /* load size                          */
	mov     24(%esp),%ecx               /* index                              */
	cmp     %edx,%ecx                   /* do bound check                     */
	jae     nb_aastore_bound            /* if out of bounds throw exception   */

	shl     $2,%ecx                     /* index * 4                          */
	add     %eax,%ecx                   /* add index * 4 to arrayref          */
	   
	mov     %ecx,8(%esp)                /* save store position                */
	   
	mov     16(%esp),%eax               /* 12 (frame) + 4 (return)            */
	mov     %eax,(%esp)
	   
	mov     32(%esp),%eax               /* object is second argument          */
	mov     %eax,4(%esp)
	
	call    builtin_canstore            /* builtin_canstore(arrayref,object)  */

	test    %eax,%eax                   /* if (false) throw exception         */
	je      nb_aastore_store

	mov     32(%esp),%eax
	mov     8(%esp),%ecx
	mov     %eax,offobjarrdata(%ecx)    /* store objectptr in array           */
	
	add     $12,%esp
	ret

nb_aastore_null:
        add $12,%esp
        mov string_java_lang_NullPointerException,%eax
        pop %ecx
        sub $2,%ecx
        jmp asm_throw_and_handle_exception

#if 0
	push    string_java_lang_NullPointerException
	call    new_exception
	add     $(1*4),%esp
	
	add     $12,%esp
	pop     %ecx                        /* delete return address              */
	sub     $2,%ecx                     /* faulting address is return adress - 2 */
	jmp		asm_handle_exception
#endif
nb_aastore_bound:
	push    %ecx                        /* itmp2 contains array index         */
	push    string_java_lang_ArrayIndexOutOfBoundsException
	call    new_exception_int
	add     $(2*4),%esp

	add     $12,%esp
	pop     %ecx                        /* delete return address              */
	sub     $2,%ecx                     /* faulting address is return adress - 2 */
	jmp     asm_handle_exception
		
nb_aastore_store:
	add     $12,%esp

        mov string_java_lang_ArrayStoreException,%eax
        pop %ecx
        sub $2,%ecx
        jmp asm_throw_and_handle_exception

#if 0
	push    string_java_lang_ArrayStoreException
	call    new_exception
	add     $(1*4),%esp
	
	add     $12,%esp
	pop     %ecx                        /* delete return address              */
	sub     $2,%ecx                     /* faulting address is return adress - 2 */
	jmp     asm_handle_exception
#endif
		
/******************* function asm_builtin_arrayinstanceof **********************
*                                                                              *
*   Does the instanceof check of arrays                                        *
*                                                                              *
*******************************************************************************/

asm_builtin_arrayinstanceof:
	sub		$8,%esp                     /* build stack frame (2 * 4 bytes)    */

	mov     12(%esp),%eax
	mov     %eax,(%esp)

	mov     20(%esp),%eax
	mov     %eax,4(%esp)

	call    builtin_arrayinstanceof

	add     $8,%esp
	ret

		
/******************* function asm_initialize_thread_stack **********************
*                                                                              *
* initialized a thread stack                                                   *
* (to)->restorePoint = asm_initialize_thread_stack((u1*)(func), (to)->stackEnd)*
*                                                                              *
*******************************************************************************/

asm_initialize_thread_stack:
		mov		8(%esp),%eax            /* (to)->stackEnd                     */
		sub		$36,%eax                /* 4 bytes * 8 regs + 4 bytes func    */
		   		
		xor		%edx,%edx
		mov		%edx,0(%eax)
		mov		%edx,4(%eax)
		mov		%edx,8(%eax)
		mov		%edx,12(%eax)
		mov		%edx,16(%eax)
		mov		%edx,20(%eax)
		mov     %edx,24(%eax)
		mov     %edx,28(%eax)
		   		
		mov     4(%esp),%edx            /* save (u1*) (func)                  */
		mov     %edx,32(%eax)

		ret                             /* return restorepoint in %eax        */


/******************* function asm_perform_threadswitch *************************
*                                                                              *
*   void asm_perform_threadswitch (u1 **from, u1 **to, u1 **stackTop);         *
*                                                                              *
*   performs a threadswitch                                                    *
*                                                                              *
*******************************************************************************/

asm_perform_threadswitch:
	sub     $36,%esp
	   
	mov     %eax,0(%esp)
	mov     %ecx,4(%esp)
	mov     %edx,8(%esp)
	mov     %ebx,12(%esp)
	mov     %esp,16(%esp)
	mov     %ebp,20(%esp)
	mov     %esi,24(%esp)
	mov     %edi,28(%esp)
	   
	mov     36(%esp),%eax         /* save current return address              */
	mov     %eax,32(%esp)
	   
	mov     40(%esp),%eax         /* first argument **from                    */
	mov     %esp,0(%eax)
	   
	mov     48(%esp),%eax         /* third argument **stackTop                */
	mov     %esp,0(%eax)
	   
	mov     44(%esp),%eax         /* second argument **to                     */
	mov     0(%eax),%esp          /* load new stack pointer                   */
	   
	mov     0(%esp),%eax
	mov     4(%esp),%ecx
	mov     8(%esp),%edx
	mov     12(%esp),%ebx
	                              /* skip stack pointer                       */
	mov     20(%esp),%ebp
	mov     24(%esp),%esi
	mov     28(%esp),%edi
	   
	add     $32,%esp              /* leave return address on stack            */
	ret
		

/********************* function asm_switchstackandcall *************************
*                                                                              *
*  int asm_switchstackandcall (void *stack, void *func, void **stacktopsave,   *
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
	mov     4(%esp),%edx          /* first argument *stack                    */
	sub     $8,%edx               /* allocate new stack                       */

	mov     (%esp),%eax           /* save return address on new stack         */
	mov     %eax,(%edx)

	mov     %esp,4(%edx)          /* save old stack pointer on new stack      */

	mov     12(%esp),%eax         /* third argument **stacktopsave            */
	mov     %esp,(%eax)           /* save old stack pointer to variable       */

	mov     8(%esp),%eax          /* load function pointer                    */
	mov     16(%esp),%ecx         /* fourth argument *p                       */
	
	mov     %edx,%esp             /* switch to new stack                      */

	sub     $4,%esp
	mov     %ecx,0(%esp)          /* pass pointer                             */
	call    *%eax                 /* and call function                        */
	add     $4,%esp

	mov     (%esp),%edx           /* load return address                      */
	mov     4(%esp),%esp          /* switch to old stack                      */
	mov     %edx,(%esp)
	ret

		
Java_java_lang_VMSecurityManager_currentClassLoader:
	lea  builtin_asm_getclassloader,%eax 
	push %eax	/*store collector function pointer*/
	jmp getClassContext_begin
Java_java_lang_VMSecurityManager_getClassContext:
	lea  builtin_asm_createclasscontextarray,%eax 
	push %eax /*store collector function pointer*/
getClassContext_begin:	/*start the real work*/

	mov %esp,%eax
	sub $4,%eax
	sub $68,%esp /*64 memory location without overwriting return adress and collector function adress*/
	mov %esp,%ebx	/*end of allocated memory block for classpointers is the adress of the working data block +4 */
	push $0 /*%esp+32 was native*/
	push %eax /*%esp+24 blkbegin*/
	push %eax /*%esp+20 currentpos*/
	push %ebx /*%esp+16 blkend*/

	call builtin_asm_get_threadrootmethod
	push %eax /*%esp+12*/
	movl 104(%esp),%eax /*(stack contains: threadRootMethod,blkend,blkpos,blkbegin,was native, data(64kB),collector,ret,env,class,frame stack info of stub, we want the frame stack info  of thestub*/
	movl %esp,%edx
	addl $116, %edx
	push %edx /*esp+8*/ /*position of return address of native stub*/
	call builtin_asm_get_stackframeinfo
	movl (%eax),%eax /*comment ??*/
	push 0(%eax) /*esp+4*/ /*address of frame info block*/
	movl 124(%esp),%edx

/*DEBUG*/
/*	mov %esp,%eax
	addl $116,%eax
	push %eax
	call i386_native_stub_debug
	pop %eax*/

	push %edx /*esp+0*/ /*return adress out of native stub*/
	call codegen_findmethod	/*find calling java method, this one is still to be skipped (==SecurityManager.getClassContext (or .currentClassLoader)*/

/*DEBUGGING*/
/*	push %eax
	movl MethodPointer(%eax),%eax
	push %eax
	call temporaryGetClassContextHelper
	pop %eax
	call traverseStackInfo
	pop %eax
*/

	movl 20(%esp),%edx
	movl MethodPointer(%eax),%ebx
	movl offclassmethodinfo(%ebx),%ecx
	movl %ecx,(%edx)
	subl $4,%edx
	movl %edx,20(%esp)

	mov 8(%esp),%ebx /*pos of return adress */
	add FrameSize(%eax),%ebx
	add $4,%ebx	/*adress of new return adress (out of Securitymanager.*/
	mov %ebx,8(%esp) 
	mov %eax,(%esp)

	/* by now we have skipped this method call*/

getClassContext_next:	
	movl 8(%esp),%eax
	movl (%eax),%eax
	movl %eax,(%esp) /*return adress*/

	call codegen_findmethod

	cmp $1,32(%esp)
	mov 8(%esp),%ebx
	add FrameSize(%eax),%ebx
	add $4,%ebx
	mov %ebx,8(%esp) /*store adress of next return adress*/
getClassContext_nextRetStored:

	mov MethodPointer(%eax),%ecx	/*get struct methodinfo*/

	cmp $0,%ecx
	je getClassContext_nativeCall
	/*save class pointer*/
	movl $0,32(%esp)
getClassContext_saveClassPointer:
	movl 20(%esp),%ebx	/*get temporary memory adress in stack*/
	movl offclassmethodinfo(%ecx),%edx /* get class pointer of method*/
	movl %edx,(%ebx) /*save */
	sub $4,%ebx	/*calculate next position */
	movl %ebx,20(%esp) /* check if the new adress would overwrite our working data */
	cmp %ebx,16(%esp)
	je getClassContext_incStack
getClassContext_checkLeave:
	
	cmp 12(%esp),%ecx	/*check if we reached the toplevel method of our thread*/
	je  getClassContext_leave /*yes ->leave*/

/*DEBUGING*/
/*	mov %ecx,(%esp)
	call temporaryGetClassContextHelper
*/
	
	
	jmp getClassContext_next /*continue*/


getClassContext_nativeCall:
	movl $1,32(%esp)
	movl 4(%esp),%eax	/*get top most element on stackframe help information stack*/
	movl 0(%eax),%ecx 
	movl %ecx,4(%esp)
	addl $8,%eax
	movl (%eax),%ecx
	addl $4,%eax
	movl %eax,8(%esp)
	
	cmp $0,%ecx
	je getClassContext_checkLeave
	jmp getClassContext_saveClassPointer

getClassContext_incStack:
	/*make another 64 in our temporary storage free and store the workingdata */
	movl %esp,%edx
	subl $40,%esp /*should be 32*/
	push 32(%edx)
	push 28(%edx)
	push 24(%edx)
	push 20(%edx)
	push 16(%edx)
	push 12(%edx)
	push 8(%edx)
	push 4(%edx)
	push 0(%edx)
	subl $64,16(%esp)

	jmp getClassContext_checkLeave /* continue */

getClassContext_leave:
/*DEBUGING*/
/*	mov %ecx,(%esp)
	call temporaryGetClassContextHelper*/

	/*call collector function with begin/end of temporary classarray*/
	push 24(%esp)
	push 24(%esp)
	
	movl 32(%esp),%eax
	add $4,%eax
	movl (%eax),%ebx
	call *%ebx

	/* free stack memory of this function*/
	mov 32(%esp),%esp
	add $8,%esp
	ret


asm_throw_and_handle_exception:
	sub $20,%esp /*build stack frame*/
	mov %ecx,16(%esp) /*save eip of problem */
	mov %eax,(%esp)
	movl $0,12(%esp) /*internal function -> no function description */
	call builtin_asm_get_stackframeinfo
	mov %eax,8(%esp)
	mov (%eax),%ecx
	mov %ecx,4(%esp)
	mov %esp,%ecx
	add $4,%ecx
	mov %ecx,(%eax)
	
/*	mov string_java_lang_NullPointerException,%eax
	mov %eax,(%esp)*/
        call new_exception
	push %eax
	mov 8(%esp),%eax
	mov 12(%esp),%ecx
	mov %eax,(%ecx)
	pop %eax
	add $16,%esp
	pop %ecx
	jmp asm_handle_exception
	ret /*should never be reached */

asm_throw_and_handle_hardware_arithmetic_exception:
	sub $24,%esp /*build stack frame*/
	mov %ecx,20(%esp) /*save eip of problem */
	
	movl $0,16(%esp) /*internal function -> no function description */
	call builtin_asm_get_stackframeinfo
	mov %eax,12(%esp)
	mov (%eax),%ecx
	mov %ecx,8(%esp)
	mov %esp,%ecx
	add $8,%ecx
	mov %ecx,(%eax)
	
	mov string_java_lang_ArithmeticException,%eax
	mov %eax,(%esp)
	mov string_java_lang_ArithmeticException_message,%eax
	mov %eax,4(%esp)

	call new_exception_message

	push %eax
	mov 12(%esp),%eax
	mov 16(%esp),%ecx
	mov %eax,(%ecx)
	pop %eax
	add $20,%esp
	pop %ecx
	jmp asm_handle_exception
	ret /*should never be reached */

asm_builtin_new:
/*optimize a littlebit */
		mov %esp,%eax
/*DEBUG*/
/*		push %eax
		call i386_native_stub_debug
		pop %eax */
		
		movl 4(%esp),%eax
		mov	offclassinit(%eax),%ecx     /* get initialized flag           */
		test	%ecx,%ecx
		jnz		L_builtin_new_noinit

                sub             $16,%esp                                 /* build stack frame (4 * 4 bytes) */

                mov             20(%esp),%eax
                mov             %eax,(%esp)

		call	builtin_asm_get_stackframeinfo
		movl	$0,12(%esp)
		mov	%eax,8(%esp)
		mov	(%eax),%ebx
		mov	%ebx,4(%esp)
		mov	%esp,%ecx
		add	$4,%ecx
		mov	%ecx,(%eax)

                call    builtin_new

		mov	4(%esp),%ebx
		mov	8(%esp),%ecx
		mov	%ebx,(%ecx)

                add             $16,%esp

		jmp L_builtin_new_patch


L_builtin_new_noinit:
		mov 4(%esp),%eax
		push %eax
		call builtin_new
		add $4,%esp
		/*jmp L_builtin_new_patch*/

L_builtin_new_patch:
/*add patching code here */
		lea builtin_new,%edx
		mov (%esp),%ecx
		mov %edx,-6(%ecx)	/*patch calling instruction, t directly call builtin_new the next time*/
		ret





asm_get_stackTrace:
	push %ebp /*(%ebp-4)*/
	mov %esp,%ebp
	add $4,%ebp
	push %edi /*(%ebp-8)*/
	push %esi /*(%ebp-12)*/
	push %ebx /*(%ebp-16)*/
	call builtin_asm_get_stackframeinfo
	movl (%eax),%eax
	pushl 0(%eax) /*(%ebp-20)*/
	lea 12(%eax),%edi
	call builtin_asm_get_threadrootmethod
	pushl %eax /*(%ebp-24)*/

	pushl (%edi)
asm_get_stackTraceLoop:
	call codegen_findmethod
	mov %eax,%esi
	add $4,%esp
	pushl $1 /*no indent*/

	mov (%edi),%edx
	sub $4,%edx

get_stackTrace_line:
	movl LineNumberTableSize(%esi),%ecx
	test    %ecx,%ecx /* skip if empty line table */
	je      get_stackTrace_noLineInfo

	movl LineNumberTableStart(%esi),%ebx
	
get_stackTrace_lineLoop:
	cmp %edx,LinePC(%ebx)
	jg get_stackTrace_nextLineInfo

	pushl LineLine(%ebx)
	jmp get_stackTrace_cont

get_stackTrace_nextLineInfo:	
	lea LineEntrySize(%ebx),%ebx
	dec %ecx
	test %ecx,%ecx

	jne get_stackTrace_lineLoop

get_stackTrace_noLineInfo:
	pushl $0


get_stackTrace_cont:
	pushl (%edi) /*4*/
	pushl MethodPointer(%esi)
	pushl $0 /*8(%ebp)*/ /*exception ptr*/
	call builtin_trace_exception
	add $12,%esp

	movl MethodPointer(%esi),%eax
	movl %eax,4(%esp)
	test %eax,%eax
	je get_stackTrace_nat

	cmp %eax,-24(%ebp)
	je get_stackTrace_leave

	mov FrameSize(%esi),%eax
	add $4,%edi
	add %eax,%edi
	pushl (%edi)
	jmp asm_get_stackTraceLoop

get_stackTrace_nat:
	add $8,%esp
	movl -20(%ebp),%eax
	cmp $0,%eax
	je get_stackTrace_leave
	movl  0(%eax),%ebx
	movl  %ebx,-20(%ebp)
	pushl 8(%eax)
	pushl $0
	lea 12(%eax),%edi
	pushl (%edi)
	jmp asm_get_stackTraceLoop

get_stackTrace_leave:
	mov %esp,%eax
	lea -24(%ebp),%ebx
	push %ebx
	push %eax
	push 4(%ebp)
	call builtin_stacktrace_copy

	lea -16(%ebp),%esp
	pop %ebx
	pop %esi
	pop %edi
	pop %ebp
	ret

asm_getclassvalues_atomic:
_crit_restart2:
	mov 4(%esp),%ecx        /* super */
	mov 8(%esp),%edx        /* sub */
_crit_begin2:
	mov offbaseval(%ecx),%eax
	mov offdiffval(%ecx),%ecx
	mov offbaseval(%edx),%edx
_crit_end2:
	push %ebx
	mov 16(%esp),%ebx      /* out */
	mov %eax,offcast_super_baseval(%ebx)
	mov %ecx,offcast_super_diffval(%ebx)
	mov %edx,offcast_sub_baseval(%ebx)
	pop %ebx
	ret

	.data

asm_criticalsections:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	.long _crit_begin1
	.long _crit_end1
	.long _crit_restart1
	.long _crit_begin2
	.long _crit_end2
	.long _crit_restart2
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
