/* src/vm/jit/x86_64/asmpart.S - Java-C interface functions for x86_64

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

   $Id: asmpart.S 2051 2005-03-20 16:26:57Z twisti $

*/


#include "config.h"
#include "vm/jit/x86_64/arch.h"
#include "vm/jit/x86_64/offsets.h"
#include "vm/jit/x86_64/asmoffsets.h"


/* define it like the risc way */

#define v0       %rax

#define a0       %rdi
#define a1       %rsi
#define a2       %rdx
#define a3       %rcx
#define a4       %r8
#define a5       %r9

#define fa0      %xmm0
#define fa1      %xmm1
#define fa2      %xmm2
#define fa3      %xmm3
#define fa4      %xmm4
#define fa5      %xmm5
#define fa6      %xmm6
#define fa7      %xmm7

#define itmp1    %rax
#define itmp2    %r10
#define itmp3    %r11

#define itmp1l   %eax
#define itmp2l   %r10d
#define itmp3l   %r11d

#define itmp1b   %al
#define itmp2b   %r10b
#define itmp3b   %r11b

#define xptr     itmp1
#define xpc      itmp2


	.text


/********************* exported functions and variables ***********************/

	.globl asm_calljavafunction

	.globl asm_calljavafunction2
	.globl asm_calljavafunction2int
	.globl asm_calljavafunction2long
	.globl asm_calljavafunction2float
	.globl asm_calljavafunction2double

	.globl asm_call_jit_compiler
	.globl asm_handle_exception
	.globl asm_handle_nat_exception
	.globl asm_check_clinit
	.globl asm_builtin_checkarraycast
	.globl asm_builtin_anewarray
	.globl asm_builtin_newarray_array
	.globl asm_builtin_aastore

#if defined(USE_THREADS)
	.globl asm_builtin_monitorenter
	.globl asm_builtin_monitorexit
#endif

	.globl asm_builtin_f2i
	.globl asm_builtin_f2l
	.globl asm_builtin_d2i
	.globl asm_builtin_d2l

	.globl asm_builtin_arrayinstanceof
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

	.quad   0                         /* catch type all                       */
	.quad   calljava_xhandler         /* handler pc                           */
	.quad   calljava_xhandler         /* end pc                               */
	.quad   asm_calljavafunction      /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   8                         /* frame size                           */
	.quad   0                         /* method pointer (pointer to name)     */

asm_calljavafunction:
	sub     $(3*8),%rsp               /* keep stack 16-byte aligned           */
	mov     %rbp,0*8(%rsp)
	mov     %rbx,1*8(%rsp)            /* %rbx is not a callee saved in cacao  */
	mov     %rdi,%rax                 /* move function pointer to %rax        */
	 								  /* compilerstub uses this               */

	mov     %rsi,%rdi                 /* pass remaining parameters            */
	mov     %rdx,%rsi
	mov     %rcx,%rdx
	mov     %r8,%rcx
	
	lea     asm_call_jit_compiler,%r11
	call    *%r11                     /* call JIT compiler                    */

	mov     0*8(%rsp),%rbp
	mov		1*8(%rsp),%rbx
	add     $(3*8),%rsp              /* free stack space                     */
	ret

calljava_xhandler:
	mov     %rax,%rdi                /* pass exception pointer               */
	call    builtin_throw_exception
	mov     0*8(%rsp),%rbp
	mov		1*8(%rsp),%rbx
	add     $(3*8),%rsp
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
	.align	8

	.quad   0                         /* catch type all                       */
	.quad   calljava_xhandler2        /* handler pc                           */
	.quad   calljava_xhandler2        /* end pc                               */
	.quad   asm_calljavafunction2     /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   24                        /* frame size                           */
	.quad   0                         /* method pointer (pointer to name)     */

asm_calljavafunction2:
asm_calljavafunction2int:
asm_calljavafunction2long:
asm_calljavafunction2float:
asm_calljavafunction2double:
	sub     $(7*8),%rsp               /* keep stack 16-byte aligned           */
	mov     %rbx,0*8(%rsp)            /* %rbx is not a callee saved in cacao  */
	mov     %rbp,1*8(%rsp)
	mov     %r12,2*8(%rsp)
	mov     %r13,3*8(%rsp)
	mov     %r14,4*8(%rsp)
	mov     %r15,5*8(%rsp)

	mov     %rdi,%rax                 /* move method pointer for compiler     */
	xor     %rbp,%rbp                 /* set argument stack frame to zero     */

	test    %rsi,%rsi                 /* maybe we have no args...             */
	jle     L_copy_done

	mov     %rsi,itmp3                /* arg count                            */
	mov     %rcx,itmp2                /* pointer to arg block                 */

	mov     itmp2,%r14                /* save argument block pointer          */
	mov     itmp3,%r15                /* save argument count                  */

	sub     $sizejniblock,itmp2       /* initialize pointer (smaller code)    */
	add     $1,itmp3                  /* initialize argument count            */
	xor     %r12,%r12                 /* initialize integer argument counter  */
	xor     %r13,%r13                 /* initialize float argument counter    */

L_register_copy:
	add     $sizejniblock,itmp2       /* goto next argument block             */
	dec     itmp3                     /* argument count - 1                   */
	jz      L_register_copy_done
	andb    $0x02,offjniitemtype(itmp2) /* is this a float/double type?       */
	jnz     L_register_handle_float   /* yes, handle it                       */

	cmp     $INT_ARG_CNT,%r12         /* are we out of integer argument       */
	je      L_register_copy           /* register? yes, next loop             */

	lea     jumptable_integer,%rbp
	mov     0(%rbp,%r12,8),%rbx
	inc     %r12                      /* integer argument counter + 1         */
	jmp     *%rbx

L_register_handle_float:
	cmp     $FLT_ARG_CNT,%r13         /* are we out of float argument         */
	je      L_register_copy           /* register? yes, next loop             */

	lea     jumptable_float,%rbp
	mov     0(%rbp,%r13,8),%rbx
	inc     %r13                      /* float argument counter + 1           */
	jmp     *%rbx
	
L_register_copy_done:
	mov     %r15,%rbp                 /* calculate remaining arguments        */
	sub     %r12,%rbp                 /* - integer arguments in registers     */
	sub     %r13,%rbp                 /* - float arguments in registers       */
	jle     L_copy_done               /* are all assigned to registers?       */

	shl     $3,%rbp                   /* calculate stack size                 */
	sub     %rbp,%rsp                 /* stack frame for arguments            */
	mov     %rsp,%rbx                 /* use %rbx as temp sp                  */

	sub     $sizejniblock,%r14        /* initialize pointer (smaller code)    */
	add     $1,%r15                   /* initialize argument count            */
		
L_stack_copy_loop:
	add     $sizejniblock,%r14        /* goto next argument block             */
	dec     %r15                      /* are there any arguments left?        */
	jz      L_copy_done               /* no test needed after dec             */

	andb    $0x02,offjniitemtype(%r14) /* is this a float/double type?        */
	jnz     L_stack_handle_float
	dec     %r12                      /* arguments assigned to registers      */
	jge     L_stack_copy_loop
	jmp     L_stack_copy

L_stack_handle_float:
	dec     %r13                      /* arguments assigned to registers      */
	jge     L_stack_copy_loop

L_stack_copy:
    mov     offjniitem(%r14),itmp3    /* copy s8 argument onto stack          */
    mov     itmp3,0(%rbx)
    add     $8,%rbx                   /* increase sp to next argument         */
	jmp     L_stack_copy_loop

L_copy_done:
	lea     asm_call_jit_compiler,%r11/* %rax still contains method pointer   */
	call    *%r11                     /* call JIT compiler                    */

	add     %rbp,%rsp                 /* remove argument stack frame if any   */

	mov     5*8(%rsp),%r15            /* restore callee saved registers       */
	mov     4*8(%rsp),%r14
	mov     3*8(%rsp),%r13
	mov     2*8(%rsp),%r12
	mov     1*8(%rsp),%rbp
	mov     0*8(%rsp),%rbx
	add     $(7*8),%rsp               /* free stack space                     */
	ret
		
calljava_xhandler2:
	mov     %rax,%rdi                 /* pass exception pointer               */
	call    builtin_throw_exception

	mov     5*8(%rsp),%r15            /* restore callee saved registers       */
	mov     4*8(%rsp),%r14
	mov     3*8(%rsp),%r13
	mov     2*8(%rsp),%r12
	mov     1*8(%rsp),%rbp
	mov     0*8(%rsp),%rbx
	add     $(7*8),%rsp               /* free stack space                     */
	ret


jumptable_integer:
	.quad   handle_a0
	.quad   handle_a1
	.quad   handle_a2
	.quad   handle_a3
	.quad   handle_a4
	.quad   handle_a5

handle_a0:
	mov     offjniitem(itmp2),a0
	jmp     L_register_copy
handle_a1:
	mov     offjniitem(itmp2),a1
	jmp     L_register_copy
handle_a2:
	mov     offjniitem(itmp2),a2
	jmp     L_register_copy
handle_a3:
	mov     offjniitem(itmp2),a3
	jmp     L_register_copy
handle_a4:
	mov     offjniitem(itmp2),a4
	jmp     L_register_copy
handle_a5:
	mov     offjniitem(itmp2),a5
	jmp     L_register_copy


jumptable_float:
	.quad   handle_fa0
	.quad   handle_fa1
	.quad   handle_fa2
	.quad   handle_fa3
	.quad   handle_fa4
	.quad   handle_fa5
	.quad   handle_fa6
	.quad   handle_fa7

handle_fa0:
	movq    offjniitem(itmp2),fa0
	jmp     L_register_copy
handle_fa1:
	movq    offjniitem(itmp2),fa1
	jmp     L_register_copy
handle_fa2:
	movq    offjniitem(itmp2),fa2
	jmp     L_register_copy
handle_fa3:
	movq    offjniitem(itmp2),fa3
	jmp     L_register_copy
handle_fa4:
	movq    offjniitem(itmp2),fa4
	jmp     L_register_copy
handle_fa5:
	movq    offjniitem(itmp2),fa5
	jmp     L_register_copy
handle_fa6:
	movq    offjniitem(itmp2),fa6
	jmp     L_register_copy
handle_fa7:
	movq    offjniitem(itmp2),fa7
	jmp     L_register_copy


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
	sub     $8,%rsp                 /* keep stack 16-byte aligned             */

	mov     %rbx,(%rsp)             /* save register                          */
        
        mov     8(%rsp),%r11    /* get return address                         */
        mov     -1(%r11),%bl	/* get function code                          */
        cmp     $0xd2,%bl		/* called with `call *REG_ITMP2' (%r10)?      */
        jne     L_not_static_special

        sub     $11,%r11        /* calculate address of immediate             */
        jmp		L_call_jit_compile
		
L_not_static_special:
		cmp     $0xd0,%bl		/* called with `call *REG_ITMP1' (%rax)       */
		jne     L_not_virtual_interface
		
		sub     $7,%r11         /* calculate address of offset                */
		mov     (%r11),%r11d    /* get offset (32-bit)                        */
		add     %r10,%r11       /* add base address to get method address     */
		jmp     L_call_jit_compile

L_not_virtual_interface:        /* a call from asm_calljavamethod             */
		xor     %r11,%r11
		
L_call_jit_compile:
        mov     (%rsp),%rbx     /* restore register                           */
        
        sub     $(24*8),%rsp    /* 8 + 48 + 64 + 64                           */
        
  		mov     %r11,0*8(%rsp)  /* save address for method pointer            */

        mov     a0,1*8(%rsp)    /* save arguments                             */
        mov     a1,2*8(%rsp)
        mov     a2,3*8(%rsp)
        mov     a3,4*8(%rsp)
        mov     a4,5*8(%rsp)
        mov     a5,6*8(%rsp)

        movq    fa0,7*8(%rsp)
        movq    fa1,8*8(%rsp)
        movq    fa2,9*8(%rsp)
        movq    fa3,10*8(%rsp)
        movq    fa4,11*8(%rsp)
        movq    fa5,12*8(%rsp)
        movq    fa6,13*8(%rsp)
        movq    fa7,14*8(%rsp)

        movq    %xmm8,15*8(%rsp)/* we use them as callee saved registers      */
        movq    %xmm9,16*8(%rsp)
        movq    %xmm10,17*8(%rsp)
        movq    %xmm11,18*8(%rsp)
        movq    %xmm12,19*8(%rsp)
        movq    %xmm13,20*8(%rsp)
        movq    %xmm14,21*8(%rsp)
        movq    %xmm15,22*8(%rsp)

        mov     %rax,%rdi       /* pass method pointer                        */
		call	jit_compile

        mov     0*8(%rsp),%r11
        
        mov     1*8(%rsp),a0
        mov     2*8(%rsp),a1
        mov     3*8(%rsp),a2
        mov     4*8(%rsp),a3
        mov     5*8(%rsp),a4
        mov     6*8(%rsp),a5

	movq    7*8(%rsp),fa0
	movq    8*8(%rsp),fa1
	movq    9*8(%rsp),fa2
	movq    10*8(%rsp),fa3
	movq    11*8(%rsp),fa4
	movq    12*8(%rsp),fa5
	movq    13*8(%rsp),fa6
	movq    14*8(%rsp),fa7

	movq    15*8(%rsp),%xmm8
	movq    16*8(%rsp),%xmm9
	movq    17*8(%rsp),%xmm10
	movq    18*8(%rsp),%xmm11
	movq    19*8(%rsp),%xmm12
	movq    20*8(%rsp),%xmm13
	movq    21*8(%rsp),%xmm14
	movq    22*8(%rsp),%xmm15

	add     $(24*8),%rsp
	add     $8,%rsp                 /* keep stack 16-byte aligned             */

	test    %rax,%rax               /* check for exception                    */
	je      L_asm_call_jit_compiler_exception

	test    %r11,%r11               /* is address == 0 (asm_calljavamethod)   */
	je      L_call_method

	mov     %rax,(%r11)             /* and now save the new pointer           */

L_call_method:
	jmp     *%rax                   /* ...and now call the new method         */

L_asm_call_jit_compiler_exception:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	call    builtin_asm_get_exceptionptrptr
	mov     %rax,itmp2
#else
	lea     _exceptionptr,itmp2
#endif
	mov     (itmp2),xptr                /* get the exception pointer          */
	movl    $0,(itmp2)                  /* clear the exception pointer        */

	pop     xpc                         /* delete return address              */
	sub     $5,xpc                      /* faulting address is ra - 5         */

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
	add     $8,%rsp                     /* clear return address of native stub*/
		
asm_handle_exception:
	sub     $(4*8),%rsp
	mov     xptr,0*8(%rsp)              /* save exception pointer             */
	mov     xpc,1*8(%rsp)               /* save exception pc                  */
        
	mov     xpc,%rdi                    /* exception pc                       */
	call    codegen_findmethod
	mov     %rax,itmp3
	mov     %rax,2*8(%rsp)              /* save data segment pointer          */
        
	mov     0*8(%rsp),%rax              /* restore exception pointer          */
	mov     1*8(%rsp),%r10              /* restore exception pc               */
        
ex_stack_loop:
	mov     %rax,%rdi                   /* exception pointer                  */
	mov     MethodPointer(itmp3),%rsi   /* method pointer                     */
	mov     xpc,%rdx                    /* exception pc                       */
	mov     $0,%rcx
	mov     $1,%r8                      /* set noindent flag                  */
	call    builtin_trace_exception

	mov     2*8(%rsp),itmp3             /* %r11 = data segment pointer        */
	mov     ExTableSize(itmp3),%rcx     /* %rcx = exception table size        */
	test    %rcx,%rcx					/* if empty table skip                */
	je      empty_table

	lea     ExTableStart(itmp3),%rdi    /* %rdi = start of exception table    */
	mov     0*8(%rsp),xptr              /* get xptr                           */
		
ex_table_loop:
	mov     1*8(%rsp),xpc               /* get xpc                            */
		
	mov     ExStartPC(%rdi),%rdx        /* %rdx = exception start pc          */
	cmp     xpc,%rdx                    /* %rdx = (startpc <= xpc)            */
	jg      ex_table_cont               /* if (false) continue                */
	mov     ExEndPC(%rdi),%rdx          /* %rdx = exception end pc            */
	cmp     %rdx,xpc                    /* %rdx = (xpc < endpc)               */
	jge     ex_table_cont               /* if (false) continue                */
	mov     ExCatchType(%rdi),%rdx      /* %rdx = exception catch type        */
	test    %rdx,%rdx                   /* NULL catches everything            */
	je      ex_handle_it

	cmpl    $0,offclassloaded(%rdx)     /* check if class is loaded           */
	jne     L_class_loaded

	sub     $(4*8),%rsp
	mov     %rax,0*8(%rsp)
	mov     %rcx,1*8(%rsp)
	mov     %rdx,2*8(%rsp)
	mov     %rdi,3*8(%rsp)

	mov     %rdx,%rdi
	call    class_load

	mov     0*8(%rsp),%rax
	mov     1*8(%rsp),%rcx
	mov     2*8(%rsp),%rdx
	mov     3*8(%rsp),%rdi
	add     $(4*8),%rsp

L_class_loaded:
	cmpl    $0,offclasslinked(%rdx)     /* check if class is linked           */
	jne     L_class_linked

	sub     $(4*8),%rsp
	mov     %rax,0*8(%rsp)
	mov     %rcx,1*8(%rsp)
	mov     %rdx,2*8(%rsp)
	mov     %rdi,3*8(%rsp)

	mov     %rdx,%rdi
	call    class_link

	mov     0*8(%rsp),%rax
	mov     1*8(%rsp),%rcx
	mov     2*8(%rsp),%rdx
	mov     3*8(%rsp),%rdi
	add     $(4*8),%rsp

L_class_linked:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	push    %rdx

_crit_restart1:
	mov     0(%rsp),%rdx
#endif

_crit_begin1:
	mov     offobjvftbl(%rax),%rsi      /* %rsi = vftblptr(xptr)              */
	mov     offclassvftbl(%rdx),%rdx    /* %rdx = vftblptr(catchtype) class (not obj) */
	mov     offbaseval(%rsi),%esi       /* %esi = baseval(xptr)               */
	mov     offbaseval(%rdx),%r10d      /* %r10d = baseval(catchtype)         */
	mov     offdiffval(%rdx),%edx       /* %edx = diffval(catchtype)          */
_crit_end1:
	sub     %r10d,%esi                  /* %esi = baseval(xptr) - baseval(catchtype) */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	add     $8,%rsp
#endif

	cmp     %edx,%esi                   /* xptr is instanceof catchtype       */
	ja      ex_table_cont
		
ex_handle_it:
		mov     ExHandlerPC(%rdi),xpc       /* xpc = exception handler pc     */

        mov     0*8(%rsp),%rax              /* restore exception pointer      */
        add     $(4*8),%rsp                 /* free stack frame               */

        jmp		*xpc                        /* jump to the handler            */

ex_table_cont:
        lea		ExEntrySize(%rdi),%rdi      /* next exception table entry     */
        dec     %rcx                        /* decrement entry counter        */
        test	%rcx,%rcx                   /* if (t0 > 0) next entry         */
        jg		ex_table_loop
		
empty_table:
        mov     0*8(%rsp),%rax              /* restore exception pointer      */
        mov     1*8(%rsp),%r10              /* restore exception pc           */
        mov     2*8(%rsp),%r11				/* restore data segment pointer   */
        add     $(4*8),%rsp
        
		mov     %rax,%rcx					/* save exception pointer         */
				
ex_already_cleared:		
		movl    IsSync(%r11),%eax			/* %rax = SyncOffset              */
		test	%rax,%rax					/* if zero no monitorexit         */
		je		no_monitor_exit

#if defined(USE_THREADS)		
		add     %rsp,%rax
		mov     -8(%rax),%rdi

        sub     $(4*8),%rsp
        mov     %rcx,0*8(%rsp)
        mov     %r10,1*8(%rsp)
        mov     %r11,2*8(%rsp)
        
		call	builtin_monitorexit

        mov     0*8(%rsp),%rcx
        mov     1*8(%rsp),%r10
        mov     2*8(%rsp),%r11
        add     $(4*8),%rsp
#endif

no_monitor_exit:
		mov		FrameSize(%r11),%eax        /* %eax = frame size              */
		add		%rax,%rsp                   /* unwind stack                   */
		mov		%rsp,%rax                   /* %rax = pointer to save area    */

		mov		IntSave(%r11),%edx          /* %edx = saved int register count*/
        test	%edx,%edx
        je		noint
        
   		cmp     $1,%edx
		je		int1
		cmp		$2,%edx
		je		int2
		cmp		$3,%edx
		je		int3
		cmp		$4,%edx
		je		int4
		cmp		$5,%edx
		je		int5
        
        mov     -48(%rax),%rbx
int5:   
        mov     -40(%rax),%rbp
int4:   
        mov     -32(%rax),%r12
int3:   
        mov     -24(%rax),%r13
int2:   
		mov     -16(%rax),%r14
int1:   
		mov     -8(%rax),%r15

		shl		$3,%edx                     /* multiply by 8 bytes            */
		sub     %rdx,%rax
		
noint:
		mov     FltSave(%r11),%edx          /* %edx = saved flt register count*/
		test	%edx,%edx
		je      noflt

        cmpl    $1,%edx
        je      flt1
        cmpl    $2,%edx
        je      flt2
        cmpl    $3,%edx
        je      flt3
        cmpl    $4,%edx
        je      flt4
        cmpl    $5,%edx
        je      flt5
        cmpl    $6,%edx
        je      flt7
        cmpl    $7,%edx
        je      flt7

        movq    -64(%rax),%xmm8
flt7:   
        movq    -56(%rax),%xmm9
flt6:   
        movq    -48(%rax),%xmm10
flt5:   
        movq    -40(%rax),%xmm11
flt4:   
        movq    -32(%rax),%xmm12
flt3:   
        movq    -24(%rax),%xmm13
flt2:   
        movq    -16(%rax),%xmm14
flt1:   
        movq    -8(%rax),%xmm15
                
noflt:					
		pop     %r10						/* the new xpc is return address  */
		sub     $3,%r10                     /* subtract 3 bytes for call      */

        sub     $(2*8),%rsp
        mov     %rcx,0*8(%rsp)
        mov     %r10,1*8(%rsp)

        mov     %r10,%rdi
        call	codegen_findmethod          /* get the new data segment ptr   */
        mov     %rax,%r11
        
        mov     0*8(%rsp),%rcx
        mov     1*8(%rsp),%r10
        add     $(2*8),%rsp

        mov     %rcx,%rax					/* restore saved exception pointer*/

        sub     $(4*8),%rsp
        				
        mov     %rax,0*8(%rsp)              /* save exception pointer         */
        mov     %r10,1*8(%rsp)              /* save exception pc              */
		mov     %r11,2*8(%rsp)				/* save data segment pointer      */
		
		jmp		ex_stack_loop


/* asm_check_clinit ************************************************************

   DOCUMENT ME!!!

   Stack layout:

	24  ra      ; return address of patched call in java machine code
	16  xmcode  ; additional machine code (only for i386 and x86_64)
	8   mcode   ; machine code to patch back in
	0   class   ; pointer to class

*******************************************************************************/

asm_check_clinit:
	mov     0(%rsp),itmp1
	mov		offclassinit(itmp1),itmp1l  /* get initialized flag (int)         */
	test	itmp1,itmp1
	jnz		L_is_initialized

	sub		$(15*8),%rsp                /* keep stack 16-byte aligned         */

	mov		a0,0*8(%rsp)                /* save argument registers            */
	mov		a1,1*8(%rsp)
	mov		a2,2*8(%rsp)
	mov		a3,3*8(%rsp)
	mov		a4,4*8(%rsp)
	mov		a5,5*8(%rsp)

	movq	fa0,6*8(%rsp)               /* maybe cacao does not use all 8     */
	movq	fa1,7*8(%rsp)               /* argument register, but who knows   */
	movq	fa2,8*8(%rsp)
	movq	fa3,9*8(%rsp)
	movq	fa4,10*8(%rsp)
	movq	fa5,11*8(%rsp)
	movq	fa6,12*8(%rsp)
	movq	fa7,13*8(%rsp)

	mov		15*8(%rsp),%rdi             /* pass classinfo pointer             */
	call	class_init                  /* call class_init function           */
	
    mov     0*8(%rsp),a0                /* restore argument registers         */
    mov     1*8(%rsp),a1
    mov     2*8(%rsp),a2
    mov     3*8(%rsp),a3
    mov     4*8(%rsp),a4
    mov     5*8(%rsp),a5

	movq	6*8(%rsp),fa0
	movq	7*8(%rsp),fa1
	movq	8*8(%rsp),fa2
	movq	9*8(%rsp),fa3
	movq	10*8(%rsp),fa4
	movq	11*8(%rsp),fa5
	movq	12*8(%rsp),fa6
	movq	13*8(%rsp),fa7

	add		$(15*8),%rsp                /* remove stack frame                 */

	test    v0,v0                       /* we had an exception                */
	je      L_initializererror
				
L_is_initialized:
	mov     24(%rsp),itmp1              /* get return address                 */
	sub     $5,itmp1                    /* remove size of `call rel32'        */

	mov     16(%rsp),itmp2              /* get xmcode machine code            */
	movb    itmp2b,(itmp1)              /* patch back in 1 byte               */
	mov     8(%rsp),itmp2               /* get mcode machine code             */
	movl    itmp2l,1(itmp1)             /* patch back in 4 bytes              */

	add     $((3+1)*8),%rsp             /* remove stub stack frame incl. ra   */

	jmp     *itmp1                      /* jump to patched code an execute it */

L_initializererror:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	call    builtin_asm_get_exceptionptrptr
	mov     %rax,itmp2
#else
	lea     _exceptionptr,itmp2
#endif
	mov     (itmp2),xptr                /* get the exception pointer          */
	movl    $0,(itmp2)                  /* clear the exception pointer        */

	add     $(3*8),%rsp                 /* remove stub stack frame            */

	pop     xpc                         /* delete return address              */
	sub     $5,xpc                      /* faulting address is ra - 5         */

	jmp     asm_handle_exception
		

/********************* function asm_builtin_monitorenter ***********************
*                                                                              *
*   Does null check and calls monitorenter or throws an exception              *
*                                                                              *
*******************************************************************************/

#if defined(USE_THREADS)
asm_builtin_monitorenter:
	test    %rdi,%rdi
	je      nb_monitorenter             /* if (null) throw exception          */
	jmp     builtin_monitorenter        /* else call builtin_monitorenter     */

nb_monitorenter:
	call    new_nullpointerexception
	pop     %r10                        /* delete return address              */
	sub     $3,%r10                     /* faulting address is ra - 3         */
	jmp     asm_handle_exception
#endif
		

/********************* function asm_builtin_monitorexit ************************
*                                                                              *
*   Does null check and calls monitorexit or throws an exception               *
*                                                                              *
*******************************************************************************/

#if defined(USE_THREADS)
asm_builtin_monitorexit:
	test    %rdi,%rdi
	je      nb_monitorexit              /* if (null) throw exception          */
	jmp     builtin_monitorexit         /* else call builtin_monitorenter     */

nb_monitorexit:
	call    new_nullpointerexception
	pop     %r10                        /* delete return address              */
	sub     $3,%r10                     /* faulting address is ra - 3         */
	jmp     asm_handle_exception
#endif


/********************* function asm_builtin_x2x ********************************
*                                                                              *
*   Wrapper functions for float to int corner cases                            *
*                                                                              *
*******************************************************************************/

asm_builtin_f2i:
	sub     $(14*8),%rsp
	
	mov     %rdi,0*8(%rsp)
	mov     %rsi,1*8(%rsp)
	mov     %rdx,2*8(%rsp)
	mov     %rcx,3*8(%rsp)
	mov     %r8,4*8(%rsp)
	mov     %r9,5*8(%rsp)
	
	movq    %xmm0,6*8(%rsp)
	movq    %xmm1,7*8(%rsp)
	movq    %xmm2,8*8(%rsp)
	movq    %xmm3,9*8(%rsp)
	movq    %xmm4,10*8(%rsp)
	movq    %xmm5,11*8(%rsp)
	movq    %xmm6,12*8(%rsp)
	movq    %xmm7,13*8(%rsp)
	
	movq    %xmm8,%xmm0
	call    builtin_f2i
	
	mov     0*8(%rsp),%rdi
	mov     1*8(%rsp),%rsi
	mov     2*8(%rsp),%rdx
	mov     3*8(%rsp),%rcx
	mov     4*8(%rsp),%r8
	mov     5*8(%rsp),%r9
	
	movq    6*8(%rsp),%xmm0
	movq    7*8(%rsp),%xmm1
	movq    8*8(%rsp),%xmm2
	movq    9*8(%rsp),%xmm3
	movq    10*8(%rsp),%xmm4
	movq    11*8(%rsp),%xmm5
	movq    12*8(%rsp),%xmm6
	movq    13*8(%rsp),%xmm7
	
	add     $(14*8),%rsp
	ret


asm_builtin_f2l:
	sub     $(14*8),%rsp
	
	mov     %rdi,0*8(%rsp)
	mov     %rsi,1*8(%rsp)
	mov     %rdx,2*8(%rsp)
	mov     %rcx,3*8(%rsp)
	mov     %r8,4*8(%rsp)
	mov     %r9,5*8(%rsp)
	
	movq    %xmm0,6*8(%rsp)
	movq    %xmm1,7*8(%rsp)
	movq    %xmm2,8*8(%rsp)
	movq    %xmm3,9*8(%rsp)
	movq    %xmm4,10*8(%rsp)
	movq    %xmm5,11*8(%rsp)
	movq    %xmm6,12*8(%rsp)
	movq    %xmm7,13*8(%rsp)
	
	movq    %xmm8,%xmm0
	call    builtin_f2l
	
	mov     0*8(%rsp),%rdi
	mov     1*8(%rsp),%rsi
	mov     2*8(%rsp),%rdx
	mov     3*8(%rsp),%rcx
	mov     4*8(%rsp),%r8
	mov     5*8(%rsp),%r9
	
	movq    6*8(%rsp),%xmm0
	movq    7*8(%rsp),%xmm1
	movq    8*8(%rsp),%xmm2
	movq    9*8(%rsp),%xmm3
	movq    10*8(%rsp),%xmm4
	movq    11*8(%rsp),%xmm5
	movq    12*8(%rsp),%xmm6
	movq    13*8(%rsp),%xmm7
	
	add     $(14*8),%rsp
	ret


asm_builtin_d2i:
	sub     $(14*8),%rsp
	
	mov     %rdi,0*8(%rsp)
	mov     %rsi,1*8(%rsp)
	mov     %rdx,2*8(%rsp)
	mov     %rcx,3*8(%rsp)
	mov     %r8,4*8(%rsp)
	mov     %r9,5*8(%rsp)
	
	movq    %xmm0,6*8(%rsp)
	movq    %xmm1,7*8(%rsp)
	movq    %xmm2,8*8(%rsp)
	movq    %xmm3,9*8(%rsp)
	movq    %xmm4,10*8(%rsp)
	movq    %xmm5,11*8(%rsp)
	movq    %xmm6,12*8(%rsp)
	movq    %xmm7,13*8(%rsp)
	
	movq    %xmm8,%xmm0
	call    builtin_d2i
	
	mov     0*8(%rsp),%rdi
	mov     1*8(%rsp),%rsi
	mov     2*8(%rsp),%rdx
	mov     3*8(%rsp),%rcx
	mov     4*8(%rsp),%r8
	mov     5*8(%rsp),%r9
	
	movq    6*8(%rsp),%xmm0
	movq    7*8(%rsp),%xmm1
	movq    8*8(%rsp),%xmm2
	movq    9*8(%rsp),%xmm3
	movq    10*8(%rsp),%xmm4
	movq    11*8(%rsp),%xmm5
	movq    12*8(%rsp),%xmm6
	movq    13*8(%rsp),%xmm7
	
	add     $(14*8),%rsp
	ret


asm_builtin_d2l:
	sub     $(14*8),%rsp
	
	mov     %rdi,0*8(%rsp)
	mov     %rsi,1*8(%rsp)
	mov     %rdx,2*8(%rsp)
	mov     %rcx,3*8(%rsp)
	mov     %r8,4*8(%rsp)
	mov     %r9,5*8(%rsp)
	
	movq    %xmm0,6*8(%rsp)
	movq    %xmm1,7*8(%rsp)
	movq    %xmm2,8*8(%rsp)
	movq    %xmm3,9*8(%rsp)
	movq    %xmm4,10*8(%rsp)
	movq    %xmm5,11*8(%rsp)
	movq    %xmm6,12*8(%rsp)
	movq    %xmm7,13*8(%rsp)
	
	movq    %xmm8,%xmm0
	call    builtin_d2l
	
	mov     0*8(%rsp),%rdi
	mov     1*8(%rsp),%rsi
	mov     2*8(%rsp),%rdx
	mov     3*8(%rsp),%rcx
	mov     4*8(%rsp),%r8
	mov     5*8(%rsp),%r9
	
	movq    6*8(%rsp),%xmm0
	movq    7*8(%rsp),%xmm1
	movq    8*8(%rsp),%xmm2
	movq    9*8(%rsp),%xmm3
	movq    10*8(%rsp),%xmm4
	movq    11*8(%rsp),%xmm5
	movq    12*8(%rsp),%xmm6
	movq    13*8(%rsp),%xmm7
	
	add     $(14*8),%rsp
	ret


/* asm_builtin_checkarraycast **************************************************

   Does the cast check and eventually throws an exception.

*******************************************************************************/

asm_builtin_checkarraycast:
	sub     $24,%rsp                    /* keep stack 16-byte aligned         */
	mov     %rdi,(%rsp)                 /* save object pointer                */
	call    builtin_checkarraycast      /* builtin_checkarraycast             */
	test    %rax,%rax                   /* if (false) throw exception         */
	je      nb_carray_throw
	mov     (%rsp),%rax                 /* return object pointer              */
	add     $24,%rsp                    /* free stack space                   */
	ret

nb_carray_throw:
	call    new_classcastexception
	add     $24,%rsp
	pop     %r10                        /* delete return address              */
	sub     $3,%r10                     /* faulting address is ra - 3         */
	jmp     asm_handle_exception

		
/* asm_builtin_aastore *********************************************************

   Checks if the object can be stored in the given array and stores the
   address if it's possible. This function can also throw some exceptions.

*******************************************************************************/

asm_builtin_aastore:
	sub     $(3*8),%rsp             /* allocate stack space                   */
	test    %rdi,%rdi               /* if null pointer throw exception        */
	je      nb_aastore_null

	movl    offarraysize(%rdi),%eax /* load size                              */
	cmpl    %eax,%esi               /* do bound check                         */
	jae     nb_aastore_bound        /* if out of bounds throw exception       */

	shl     $3,%rsi                 /* index * 8                              */
	mov     %rdi,%r10
	add     %rsi,%r10               /* add index * 8 to arrayref              */

	mov     %r10,(%rsp)             /* save store position                    */
	mov     %rdx,8(%rsp)            /* save object                            */

	mov     %rdx,%rsi               /* object is second argument              */
	call    builtin_canstore        /* builtin_canstore(arrayref,object)      */
	test    %rax,%rax               /* if (false) throw exception             */
	je      nb_aastore_throw

	mov     (%rsp),%r10             /* restore store position                 */
	mov     8(%rsp),%rdx            /* restore object                         */
	mov     %rdx,offobjarrdata(%r10)/* store objectptr in array               */
	add     $(3*8),%rsp             /* free stack space                       */
	ret

nb_aastore_null:
	call    new_nullpointerexception
	add     $24,%rsp
	pop     %r10                    /* delete return address                  */
	sub     $3,%r10                 /* faulting address is return adress - 3  */
	jmp     asm_handle_exception

nb_aastore_bound:
	mov     %rsi,%rdi               /* move index into a0                     */
	call    new_arrayindexoutofboundsexception
	add     $24,%rsp
	pop     %r10                    /* delete return address                  */
	sub     $3,%r10                 /* faulting address is return adress - 3  */
	jmp     asm_handle_exception
		
nb_aastore_throw:
	call    new_arraystoreexception
	add     $24,%rsp
	pop     %r10                    /* delete return address                  */
	sub     $3,%r10                 /* faulting address is return adress - 3  */
	jmp     asm_handle_exception

		
/******************* function asm_initialize_thread_stack **********************
*                                                                              *
* initialized a thread stack                                                   *
* (to)->restorePoint = asm_initialize_thread_stack((u1*)(func), (to)->stackEnd)*
*                                                                              *
*******************************************************************************/

asm_initialize_thread_stack:
        sub     $(7*8),%rsi

        xor     %r10,%r10
        mov     %r10,0*8(%rsi)
        mov     %r10,1*8(%rsi)
        mov     %r10,2*8(%rsi)
        mov     %r10,3*8(%rsi)
        mov     %r10,4*8(%rsi)
        mov     %r10,5*8(%rsi)

        mov     %rdi,6*8(%rsi)          /* save (u1*) (func)                  */
        mov     %rsi,%rax               /* return restorepoint in %rax        */
        ret


/******************* function asm_perform_threadswitch *************************
*                                                                              *
*   void asm_perform_threadswitch (u1 **from, u1 **to, u1 **stackTop);         *
*                                                                              *
*   performs a threadswitch                                                    *
*                                                                              *
*******************************************************************************/

asm_perform_threadswitch:
        sub     $(7*8),%rsp             /* allocate stack frame               */

        mov     %rbx,0*8(%rsp)
        mov     %rbp,1*8(%rsp)
        mov     %r12,2*8(%rsp)
        mov     %r13,3*8(%rsp)
        mov     %r14,4*8(%rsp)
        mov     %r15,5*8(%rsp)

        mov     7*8(%rsp),%rax			/* save current return address        */
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

        mov     6*8(%rsp),%rax          /* restore return address             */
        add     $(7*8),%rsp             /* free stack frame                   */
        mov     %rax,(%rsp)
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
        sub     $(1*8),%rsp             /* keep stack 16-byte aligned         */
        sub     $16,%rdi				/* allocate new stack                 */

        mov     8(%rsp),%rax			/* save return address on new stack   */
        mov     %rax,(%rdi)
        mov     %rsp,8(%rdi)			/* save old stack pointer on new stack*/
        mov     %rsp,(%rdx)				/* save old stack pointer to variable */

        mov     %rdi,%rsp				/* switch to new stack                */

        mov     %rcx,%rdi       		/* pass pointer                       */
        call	*%rsi					/* and call function                  */

        mov     (%rsp),%r10				/* load return address                */
        mov     8(%rsp),%rsp			/* switch to old stack                */
        add     $(1*8),%rsp             /* free stack space                   */
        mov     %r10,(%rsp)             /* write return adress                */
        ret


asm_getclassvalues_atomic:
_crit_restart2:
_crit_begin2:
	movl    offbaseval(a0),itmp1l
	movl    offdiffval(a0),itmp2l
	movl    offbaseval(a1),itmp3l
_crit_end2:
	movl    itmp1l,offcast_super_baseval(a2)
	movl    itmp2l,offcast_super_diffval(a2)
	movl    itmp3l,offcast_sub_baseval(a2)
	ret

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
	.quad 0

				
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
