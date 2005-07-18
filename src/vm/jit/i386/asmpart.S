/* src/vm/jit/i386/asmpart.S - Java-C interface functions for i386

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

   $Id: asmpart.S 3046 2005-07-18 14:46:41Z twisti $

*/


#include "config.h"
#include "vm/jit/i386/offsets.h"
#include "vm/jit/i386/asmoffsets.h"


/* define it like the risc way */

#define v0       %eax

#define sp       %esp

#define itmp1    %eax
#define itmp2    %ecx
#define itmp3    %edx

#define itmp1b   %al
#define itmp2b   %cl
#define itmp3b   %dl

#define xptr     itmp1
#define xpc      itmp2


	.text


/********************* exported functions and variables ***********************/

	.globl asm_calljavafunction
	.globl asm_calljavafunction_int

	.globl asm_calljavafunction2
	.globl asm_calljavafunction2int
	.globl asm_calljavafunction2long
	.globl asm_calljavafunction2float
	.globl asm_calljavafunction2double

	.globl asm_call_jit_compiler
	.globl asm_handle_nat_exception
	.globl asm_handle_exception

	.globl asm_wrapper_patcher

	.globl asm_builtin_f2i
	.globl asm_builtin_f2l
	.globl asm_builtin_d2i
	.globl asm_builtin_d2l

	.globl asm_perform_threadswitch
	.globl asm_initialize_thread_stack
	.globl asm_switchstackandcall
	.globl asm_criticalsections
	.globl asm_getclassvalues_atomic


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
	.align	8

	.long   0                         /* catch type all                       */
	.long   calljava_xhandler         /* handler pc                           */
	.long   calljava_xhandler         /* end pc                               */
	.long   asm_calljavafunction      /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* line number table start              */
	.long   0                         /* line number table size               */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   32                        /* frame size                           */
	.long   0                         /* method pointer (pointer to name)     */

asm_calljavafunction:
asm_calljavafunction_int:
	push    %ebp                      /* allocate stack space                 */
	mov     %esp, %ebp

	push    %ebx                      /* save registers                       */
	push    %esi
	push    %edi


	sub		$16,%esp					/* 4 adress parameters * 4 Bytes */
	mov     24(%ebp),%eax				/* copy adress parameters to new block */
	mov     %eax,12(%esp)
		
	mov     20(%ebp),%eax
	mov     %eax,8(%esp)

	mov     16(%ebp),%eax
	mov     %eax,4(%esp)

	mov     12(%ebp),%eax
	mov     %eax,(%esp)
		
	mov     8(%ebp),%eax              /* move function pointer to %eax        */

	lea     asm_call_jit_compiler,%edx 
	call    *%edx                     /* call JIT compiler                    */

	add     $16,%esp
	pop     %edi                      /* restore registers                    */
	pop     %esi
	pop     %ebx
	leave
	ret

calljava_xhandler:
	push    %eax                      /* pass exception pointer               */
	call    builtin_throw_exception
	add     $4,%esp

	add     $16,%esp
	pop     %edi                      /* restore registers                    */
	pop     %esi
	pop     %ebx
	leave
	xor     %eax,%eax                 /* return NULL                          */
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
asm_calljavafunction2int:
asm_calljavafunction2long:
asm_calljavafunction2float:
asm_calljavafunction2double:
	push    %ebp
	mov     %esp,%ebp                 /* save stackptr                        */

	push    %ebx                      /* save registers                       */
	push    %esi
	push    %edi

	mov     20(%ebp),%eax             /* pointer to arg block (4(push)+4(return)+4+4+4)*/
	mov     12(%ebp),%ecx             /* arg count            (4(push)+4(return)+4     */
	test    %ecx,%ecx                 /* maybe we have no args                */
	jle     calljava_copydone

		mov		%ecx,%edx			 /* calculate stack size                 */
		xor		%esi,%esi
		mov		%eax,%edi			 /* save pointer to arg block */
calljava_calcstacksize:
		mov		offjniitemtype(%eax),%ebx
		test	$1,%ebx					/* Two Word Type? */
		jz		calljava_onewordtype
		add		$4,%esi
calljava_onewordtype:
		add		$4,%esi
		sub		$1,%edx
		test	%edx,%edx				/* any args left ?*/
		jz		calljava_setstack
		add     $sizejniblock,%eax      /* goto next argument block             */
		jmp		calljava_calcstacksize
		
calljava_setstack:				
		mov		%edi,%eax			 /* restore pointer to arg block */
	sub     %esi,%esp                 /* stack frame for arguments            */
	mov     %esp,%edi

calljava_copyloop:
	    mov     offjniitem(%eax),%edx   /* copy 4 Byte of Argument */
	    mov     %edx,(%edi)
		add		$4,%edi					/* increase sp to next argument         */
		mov		offjniitemtype(%eax),%ebx /* type -> ebx */
		test	$1,%ebx					/* Two Word Type? */
		jz		calljava_copynext
	    mov     offjniitem+4(%eax),%edx /* copy upper 4 Byte of 2 Word Type */  
		mov     %edx,(%edi)			
		add		$4,%edi					/* increase sp to next argument         */
calljava_copynext:		
	sub     $1,%ecx                   /* are there any args left?             */
	test    %ecx,%ecx
	jle     calljava_copydone

	add     $sizejniblock,%eax        /* goto next argument block             */
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
	xor     %eax,%eax                 /* return NULL                          */
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
	sub     $((4+2)*4+sizestackframeinfo),sp /* create stack frame            */
	mov     itmp1,(4+0)*4(sp)           /* save method pointer                */
			
	mov     (4+2)*4+sizestackframeinfo(sp),itmp3 /* get return address        */
	mov     -1(itmp3),itmp1b            /* get function code                  */
	cmp     $0xd1,itmp1b                /* called with `call *REG_ITMP2'?     */
	jne		L_not_static_special

	sub     $6,itmp3                    /* calculate address of immediate     */
	jmp		L_call_jit_compile
		
L_not_static_special:
	cmp     $0xd0,itmp1b                /* called with `call *REG_ITMP1'      */
	jne		L_not_virtual_interface
	
	sub     $6,itmp3                    /* calculate address of offset        */
	mov     (itmp3),itmp3               /* get offset                         */
	add     itmp2,itmp3                 /* add base address to get method adr */
	jmp		L_call_jit_compile

L_not_virtual_interface:
	xor     itmp3,itmp3                 /* a call from asm_calljavafunction   */
		
L_call_jit_compile:
	mov     itmp3,(4+1)*4(sp)           /* save address for method pointer    */

	mov     sp,itmp1                    /* create stackframe info             */
	add     $((4+2)*4),itmp1
	mov     itmp1,0*4(sp)               /* stackframeinfo pointer             */
	movl    $0,1*4(sp)                  /* if pv is NULL, use findmethod      */
	mov     sp,itmp2
	add     $((1+4+2)*4+sizestackframeinfo),itmp2 /* pass java sp             */
	mov     itmp2,2*4(sp)
	mov     ((0+4+2)*4+sizestackframeinfo)(sp),itmp3 /* pass java ra          */
	mov     itmp3,3*4(sp)
	call    stacktrace_create_inline_stackframeinfo

	mov     (4+0)*4(sp),itmp1           /* pass method pointer                */
	mov     itmp1,0*4(sp)
	call    jit_compile
	mov     v0,(4+0)*4(sp)              /* save return value                  */

	mov     sp,itmp1                    /* remove stackframe info             */
	add     $((4+2)*4),itmp1
	mov     itmp1,0*4(sp)               /* stackframeinfo pointer             */
	call    stacktrace_remove_stackframeinfo

	mov     (4+0)*4(sp),v0              /* restore return value               */
	mov     (4+1)*4(sp),itmp3           /* restore address for method pointer */

	add     $((4+2)*4+sizestackframeinfo),sp /* remove stack frame            */

	test    v0,v0                       /* check for exception                */
	je      L_asm_call_jit_compiler_exception

	test	itmp3,itmp3                 /* was this a JIT call?               */
	je		L_call_method
	
	mov     v0,(itmp3)                  /* save the new method pointer        */

L_call_method:
	jmp		*v0                         /* ...and now call the new method     */

L_asm_call_jit_compiler_exception:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	call    builtin_asm_get_exceptionptrptr
	mov     v0,itmp2                    /* v0 == itmp1                        */
#else
	lea     _exceptionptr,itmp2
#endif
	mov     (itmp2),xptr                /* get the exception pointer          */
	movl    $0,(itmp2)                  /* clear the exception pointer        */

	pop     xpc                         /* get return address                 */
	sub     $2,xpc                      /* faulting address is ra - 2         */
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
	add     $4,%esp                    /* clear return address of native stub */
		
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
	call    load_class_bootstrap

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
	call    link_class

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

#if defined(USE_THREADS)
        add     %esp,%eax
        mov     (%eax),%eax               /* we have the xptr on the stack (+4-4=0)  */
        push    %edx                        /* save regs                      */
        push    %eax
		call    builtin_monitorexit
		add     $4,%esp
        pop     %edx                        /* restore regs                   */
#endif

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
		mov     -16(%eax),%ebx

int3:	
		mov     -12(%eax),%ebp

int2:	
		mov     -8(%eax),%esi

int1:	
		mov     -4(%eax),%edi

   		shl     $2,%ecx						/* multiply by 4 bytes             */
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
		sub     $2,%ecx						/* -2 -> call */
		
		jmp		asm_handle_exception_loop
		

/* asm_wrapper_patcher *********************************************************

   XXX

   Stack layout:
     20   return address
     16   pointer to virtual java_objectheader
     12   last byte of machine code (xmcode)
      8   machine code (which is patched back later)
      4   unresolved field reference
      0   patcher function pointer to call

*******************************************************************************/

asm_wrapper_patcher:
	sub     $((2+4)*4+sizestackframeinfo),sp /* create stack frame            */

	mov     itmp1,(0+4)*4(sp)           /* save itmp1 and itmp2               */
	mov     itmp2,(1+4)*4(sp)           /* may be used by some instructions   */

	mov     sp,itmp1                    /* create stackframe info             */
	add     $((2+4)*4),itmp1
	mov     itmp1,0*4(sp)               /* stackframeinfo pointer             */
	movl    $0,1*4(sp)                  /* if pv is NULL, use findmethod      */
	mov     sp,itmp2
	add     $((6+2+4)*4+sizestackframeinfo),itmp2
	mov     itmp2,2*4(sp)
	mov     ((5+2+4)*4+sizestackframeinfo)(sp),itmp3
	mov     itmp3,3*4(sp)
	call    stacktrace_create_inline_stackframeinfo

	mov     sp,itmp1                    /* pass stack pointer                 */
	add     $((1+2+4)*4+sizestackframeinfo),itmp1  /* skip function pointer   */
	mov     itmp1,0*4(sp)
	mov     (0+2+4)*4+sizestackframeinfo(sp),itmp1 /* get function pointer    */
	call    *itmp1                      /* call the patcher function          */
	mov     v0,1*4(sp)                  /* save return value                  */

	mov     sp,itmp1                    /* remove stackframe info             */
	add     $((2+4)*4),itmp1
	mov     itmp1,0*4(sp)               /* stackframeinfo pointer             */
	call    stacktrace_remove_stackframeinfo

	mov     (0+4)*4(sp),itmp1           /* restore itmp1 and itmp2            */
	mov     (1+4)*4(sp),itmp2           /* may be used by some instructions   */
	mov     1*4(sp),itmp3               /* restore return value               */

	add     $((5+2+4)*4+sizestackframeinfo),sp /* remove stack frame, keep ra */
	test    itmp3,itmp3                 /* exception thrown?                  */
	jz      L_asm_wrapper_patcher_exception
	ret                                 /* call new patched code              */

L_asm_wrapper_patcher_exception:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	call    builtin_asm_get_exceptionptrptr
	mov     v0,itmp2
#else
	lea     _exceptionptr,itmp2
#endif
	mov     (itmp2),xptr                /* get the exception pointer          */
	movl    $0,(itmp2)                  /* clear the exception pointer        */

	pop     xpc                         /* get and remove return address      */
	jmp     asm_handle_exception


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
