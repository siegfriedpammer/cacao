/* vm/jit/i386/asmpart.S - Java-C interface functions for i386

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
            Christian Thalinger

   Changes: Joseph Wenninger

   $Id: asmpart.S 1735 2004-12-07 14:33:27Z twisti $

*/


#include "config.h"
#include "vm/jit/i386/offsets.h"
#include "vm/jit/i386/asmoffsets.h"


#define itmp1    %eax
#define itmp2    %ecx
#define itmp3    %edx

#define itmp1b   %al
#define itmp2b   %cl
#define itmp3b   %dl
		
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
	.globl asm_builtin_new
	.globl asm_criticalsections
	.globl asm_getclassvalues_atomic
	.globl asm_prepare_native_stackinfo
	.globl asm_remove_native_stackinfo
/*************************** imported functions *******************************/

	.globl jit_compile
	.globl builtin_monitorexit
	.globl builtin_throw_exception
	.globl builtin_trace_exception
	.globl class_java_lang_Object
	.globl codegen_findmethod
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

L_refillinStacktrace:               /*a compilation error should cause a stacktrace
                                    which starts at the method call, which caused
                                    the compilation of the new function. Until this
                                    point the trace is invalid anyways, since it is
                                    not complete. Compared to other runtimes it will
                                    not be correct either, since we report eg class
                                    not found errors too early, since we always
                                    compile methods completely*/
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
asm_handle_exception_loop:
		push    %ebp
		mov     %esp,%ebp
        
		push    %eax						/* save exception pointer         */
		push    %ecx                        /* save exception pc              */

		call    codegen_findmethod          /* get the data segment ptr       */
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

	mov     0*4(%esp),%ebx
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

	mov     0*4(%esp),%ebx
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
	mov     offobjvftbl(%eax),%esi		/* %esi = vftblptr(xptr)              */
	mov     offclassvftbl(%ebx),%ebx    /* %ebx = vftblptr(catchtype) class (not obj) */
	mov     offbaseval(%esi),%esi		/* %esi = baseval(xptr)               */
	mov     offbaseval(%ebx),%edx		/* %edx = baseval(catchtype)          */
	mov     offdiffval(%ebx),%ebx		/* %ebx = diffval(catchtype)          */
_crit_end1:
	sub     %edx,%esi					/* %esi = baseval(xptr) - baseval(catchtype) */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	add     $4,%esp
#endif
        
	cmp     %ebx,%esi					/* xptr is instanceof catchtype       */
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
		

/* asm_check_clinit ************************************************************

   DOCUMENT ME!!!

   Stack layout:

	16  ra      ; return address of patched call in java machine code
	12  xmcode  ; additional machine code (only for i386 and x86_64)
	8   mcode   ; machine code to patch back in
	4   class   ; pointer to class
	0   sp      ; stack pointer of java stack frame + return address

*******************************************************************************/

asm_check_clinit:
	mov     4(%esp),%eax                /* get fieldinfo's class pointer      */
	mov     offclassinit(%eax),%eax     /* get initialized flag               */
	test	%eax,%eax
	jnz     L_is_initialized

	/*3*4 bytes*/
	mov 16(%esp),itmp1
	push itmp1                        /*return adress into java machine code */
	mov 4(%esp),itmp1
	push itmp1                        /*begin of java stack frame*/
	pushl $0                           /*internal (invisible) method*/
	call asm_prepare_native_stackinfo /*puts additional 2 *4 bytes of
						data onto the stack */

	sub     $4,%esp
	mov     20+4+4(%esp),itmp1          /* get class pointer                  */
	mov     itmp1,(%esp)                /* store class pointer as a0          */
	call    class_init                  /* call class_init function           */
	add     $4,%esp

	call asm_remove_native_stackinfo  /* removes 4* 4 bytes and leaves ret
						 into java machine code on stack  */
	add	$4,%esp                   /* ret address no longer needed, is still
						on stack a few bytes above */

	test    %eax,%eax                   /* we had an exception                */
	je      L_initializererror

L_is_initialized:
	mov     16(%esp),itmp1              /* get return address                 */
	sub     $5,itmp1                    /* remove size of `call rel32'        */

	mov     12(%esp),itmp2              /* get xmcode machine code            */
	movb    itmp2b,(itmp1)              /* patch back in 1 byte               */
	mov     8(%esp),itmp2               /* get mcode machine code             */
	mov     itmp2,1(itmp1)              /* patch back in 4 bytes              */

	add     $(5*4),%esp                 /* remove stub stack frame incl. ra   */

	jmp     *itmp1                      /* jump to patched code an execute it */

L_initializererror:
	add     $(4*4),%esp                 /* remove stub stack frame            */

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

	pop     itmp2                       /* get and delete ra                  */
	sub     $5,itmp2                    /* faulting address is ra - 5         */

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
	add     $12,%esp
	mov	%ecx,%eax                        /* itmp2 contains array index         */
	pushl	$0  /*directly below return adress*/
	pushl   $0  /*internal (invisible) method*/
	call    asm_prepare_native_stackinfo /* puts 2*4 bytes onto stack*/

	push	%eax
	call    new_arrayindexoutofboundsexception
	add     $(1*4),%esp

	call	asm_remove_native_stackinfo /*return adress is the first on stack again*/

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

		
asm_throw_and_handle_exception:
	push %ecx
	pushl $0 /* the pushed XPC is directly below the java frame*/
	pushl $0
	call asm_prepare_native_stackinfo /* be aware of the stack effect and calling convention explained below*/
	
	push %eax
        call new_exception
	add $4,%esp   /*remove parameter*/

	call asm_remove_native_stackinfo /* be aware of the stack effect and calling convention explained below*/

	pop %ecx
	jmp asm_handle_exception
	ret /*should never be reached */

asm_throw_and_handle_hardware_arithmetic_exception:
	
	push %ecx
	pushl $0 /* the pushed XPC is directly below the java frame*/
	pushl $0
	call asm_prepare_native_stackinfo /* be aware of the stack effect and calling convention explained below*/
	
	mov string_java_lang_ArithmeticException_message,%eax
	push %eax
	mov string_java_lang_ArithmeticException,%eax
	push %eax

	call new_exception_message
	add $8,%esp /*remove parameters */

	call asm_remove_native_stackinfo /* be aware of the stack effect and calling convention explained below*/

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

		mov 4(%esp),%eax /* class pointer, is kept during the asm_prepare... calls */

		/* 2 *4 bytes, the return adress is used directy */
		pushl $0  /* the structure is placed directly below the java stackframe*/
		pushl $0  /* builtin (invisible) method */
		call  asm_prepare_native_stackinfo /*puts 2*4 additional bytes on stack*/
#if 0
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
#endif
		push	%eax
                call    builtin_new
		add	$4,%esp

		call	asm_remove_native_stackinfo /*first element on stack is return adress again*/
#if 0		
		call	
		mov	4(%esp),%ebx
		mov	8(%esp),%ecx
		mov	%ebx,(%ecx)

                add             $16,%esp
#endif
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






asm_getclassvalues_atomic:
_crit_restart2:
	mov     4(%esp),%ecx        /* super */
	mov     8(%esp),%edx        /* sub */
_crit_begin2:
	mov     offbaseval(%ecx),%eax
	mov     offdiffval(%ecx),%ecx
	mov     offbaseval(%edx),%edx
_crit_end2:
	push    %ebx
	mov     16(%esp),%ebx      /* out */
	mov     %eax,offcast_super_baseval(%ebx)
	mov     %ecx,offcast_super_diffval(%ebx)
	mov     %edx,offcast_sub_baseval(%ebx)
	pop     %ebx
	ret

	.data

asm_criticalsections:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	.long   _crit_begin1
	.long   _crit_end1
	.long   _crit_restart1
	.long   _crit_begin2
	.long   _crit_end2
	.long   _crit_restart2
#endif
	.long 0



/************************ function asm_prepare_native_stackinfo ****************************
*                                                                                          *
*    creates a stackfame for the begin of a native function (either builtin or not )       *
*    expected stack at begin of function                                                   *
*                                        ....                                              *
*                   address of the jit call which invokes the native                       *
*                   begin address of stack frame of the java method                        *
*                   method pointer or 0 (for built ins)                                    *
*                   return address                                                         *
*                                                                                          *
*    at end of function:                                                                   *
*                                          ...                                             *
*                   address of the jit call which invokes the native                       *
*                   begin address of stack frame of the java method                        *
*                   method pointer or 0 (for built ins)                                    *
*                   address of thread specific top of native list                          *
*                   old value of thread specific head                                      *
*                   return address                                                         *
*                                                                                          *
*                                        ....                                              *
* This thing is less efficient than the original #define (callerside)                      *
* destroyes REG_ITMP2, keeps REG_ITMP1                                                     *
********************************************************************************************/


asm_prepare_native_stackinfo:
        sub $8,%esp
        mov 8(%esp),%ecx
        mov %ecx,(%esp)
        push %eax
        lea     builtin_asm_get_stackframeinfo,%ecx
        call    *%ecx
        mov %eax, 12(%esp)
        mov (%eax),%ecx
        mov %ecx,8(%esp)
        mov %esp,%ecx
        add $8,%ecx
        mov %ecx,(%eax)
        pop %eax
        ret
#if 0
#define PREPARE_NATIVE_STACKINFO \
    i386_push_reg(cd, REG_ITMP1);       /*save itmp1, needed by some stubs */ \
    i386_alu_imm_reg(cd, I386_SUB, 2*4, REG_SP); /* build stack frame (2 * 4 bytes), together with previous =3*4 */ \
    i386_mov_imm_reg(cd, (s4) codegen_stubcalled,REG_ITMP1); \
    i386_call_reg(cd, REG_ITMP1);                /*call    codegen_stubcalled*/ \
    i386_mov_imm_reg(cd, (s4) builtin_asm_get_stackframeinfo,REG_ITMP1); \
    i386_call_reg(cd, REG_ITMP1);                /*call    builtin_asm_get_stackframeinfo*/ \
    i386_mov_reg_membase(cd, REG_RESULT,REG_SP,1*4); /* save thread pointer  to native call stack*/ \
    i386_mov_membase_reg(cd, REG_RESULT,0,REG_ITMP2); /* get old value of thread specific native call stack */ \
    i386_mov_reg_membase(cd, REG_ITMP2,REG_SP,0*4);     /* store value on stack */ \
    i386_mov_reg_membase(cd, REG_SP,REG_RESULT,0); /* store pointer to new stack frame information */ \
    i386_mov_membase_reg(cd, REG_SP,2*4,REG_ITMP1); /* restore ITMP1, need for some stubs*/ \
    i386_mov_imm_membase(cd, 0,REG_SP, 2*4);    /* builtin */
#endif


/************************ function asm_remove _native_stackinfo *******************************************
*                                                                                                         *
*    creates a stackfame for the begin of a native function (either builtin or not)                       *
*    expected stack at begin of function                                                                  *
*                   address of the jit call which invokes the native                                      *
*                   begin address of stack frame of the java method                                       *
*                   method pointer or 0 (for built ins)                                                   *
*                   address thread specific top of native list                                            *
*                   old value of thread specific head                                                     *
*                   return address                                                                        *
*                                                                                                         *
*    at end of function:                                                                                  *
*                             ....                                                                        *
*                   return adresss of the jit call which invokes the native                               *
*                   return address                                                                        *
*                                                                                                         *
*                   REG_ITMP2_XPC = address of the jit call which invokes the native                      *
*                                                                                                         *
*                                                                                                         *
* This thing is less efficient than the original #define (callerside), uses ITMP3,uses ITMP3,keeps ITMP1  *
***********************************************************************************************************/

asm_remove_native_stackinfo:
        mov 4(%esp),%ecx
        mov 8(%esp),%edx
        mov %ecx,(%edx)
        pop %edx
        add $16,%esp
        push %edx
	ret

#if 0
#define REMOVE_NATIVE_STACKINFO \
    i386_mov_membase_reg(cd, REG_SP,0,REG_ITMP2); \
    i386_mov_membase_reg(cd, REG_SP,4,REG_ITMP3); \
    i386_mov_reg_membase(cd, REG_ITMP2,REG_ITMP3,0); \
    i386_alu_imm_reg(cd, I386_ADD,3*4,REG_SP);
#endif




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
