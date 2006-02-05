/* src/vm/jit/x86_64/asmpart.S - Java-C interface functions for x86_64

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Contact: cacao@cacaojvm.org

   Authors: Andreas Krall
            Reinhard Grafl
            Christian Thalinger

   Changes:

   $Id: asmpart.S 4440 2006-02-05 12:03:43Z twisti $

*/


#include "config.h"

#include "md-abi.h"
#include "md-asm.h"

#include "vm/jit/abi.h"
#include "vm/jit/x86_64/arch.h"
#include "vm/jit/x86_64/offsets.h"

#include "vm/jit/methodheader.h"


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
	.globl asm_handle_exception
	.globl asm_handle_nat_exception

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

	.align	8

	.quad   0                           /* catch type all                     */
	.quad   calljava_xhandler           /* handler pc                         */
	.quad   calljava_xhandler           /* end pc                             */
	.quad   asm_calljavafunction        /* start pc                           */
	.long   1                           /* extable size                       */
	.long   0						    									  
	.quad   0                           /* line number table start            */
	.quad   0                           /* line number table size             */
	.long   0						    									  
	.long   0                           /* fltsave                            */
	.long   0                           /* intsave                            */
	.long   0                           /* isleaf                             */
	.long   0                           /* IsSync                             */
	.long   8                           /* frame size                         */
	.quad   0                           /* method pointer (pointer to name)   */

asm_calljavafunction:
asm_calljavafunction_int:
	sub     $(3*8),sp                   /* keep stack 16-byte aligned         */
	mov     bp,0*8(sp)
	mov     %rbx,1*8(sp)                /* %rbx is not a callee saved in CACAO*/
	mov     a0,itmp1                    /* move function pointer to itmp1     */
	 								    /* compilerstub uses this             */

	mov     a1,a0                       /* pass remaining parameters          */
	mov     a2,a1															
	mov     a3,a2
	mov     a4,a3

	lea     asm_call_jit_compiler,itmp3
	call    *itmp3                      /* call JIT compiler                  */

L_asm_calljavafunction_return:
	mov     0*8(sp),bp
	mov		1*8(sp),%rbx                /* %rbx is not a callee saved in CACAO*/
	add     $(3*8),sp                   /* free stack space                   */
	ret

calljava_xhandler:
	mov     xptr,a0                     /* pass exception pointer             */
	call    builtin_throw_exception
	xor     v0,v0                       /* return NULL                        */
	jmp     L_asm_calljavafunction_return


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

	.align	8

	.quad   0                           /* catch type all                     */
	.quad   calljava_xhandler2          /* handler pc                         */
	.quad   calljava_xhandler2          /* end pc                             */
	.quad   asm_calljavafunction2       /* start pc                           */
	.long   1                           /* extable size                       */
	.long   0						    									 
	.quad   0                           /* line number table  start           */
	.quad   0                           /* line number table  size            */
	.long   0						    									 
	.long   0                           /* fltsave                            */
	.long   0                           /* intsave                            */
	.long   0                           /* isleaf                             */
	.long   0                           /* IsSync                             */
	.long   24                          /* frame size                         */
	.quad   0                           /* method pointer (pointer to name)   */

asm_calljavafunction2:
asm_calljavafunction2int:
asm_calljavafunction2long:
asm_calljavafunction2float:
asm_calljavafunction2double:
	sub     $(7*8),sp                   /* keep stack 16-byte aligned         */
	mov     %rbx,0*8(sp)                /* %rbx is not a callee saved in cacao*/
	mov     s0,1*8(sp)
	mov     s1,2*8(sp)
	mov     s2,3*8(sp)
	mov     s3,4*8(sp)
	mov     s4,5*8(sp)

	mov     a0,itmp1                    /* move method pointer for compiler   */
	xor     %rbp,%rbp                   /* set argument stack frame to zero   */

	test    a1,a1                       /* maybe we have no args...           */
	jle     L_copy_done

	mov     a1,itmp3                    /* arg count                          */
	mov     a3,itmp2                    /* pointer to arg block               */

	mov     itmp2,%r14                  /* save argument block pointer        */
	mov     itmp3,%r15                  /* save argument count                */

	sub     $sizejniblock,itmp2         /* initialize pointer (smaller code)  */
	add     $1,itmp3                    /* initialize argument count          */
	xor     %r12,%r12                   /* initialize integer argument counter*/
	xor     %r13,%r13                   /* initialize float argument counter  */

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

	and     $0xfffffffffffffffe,%rbp  /* keep stack 16-byte aligned           */
	shl     $3,%rbp                   /* calculate stack size                 */
	sub     %rbp,sp                 /* stack frame for arguments            */
	mov     sp,%rbx                 /* use %rbx as temp sp                  */

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
	lea     asm_call_jit_compiler,itmp3 /* %rax still contains method pointer */
	call    *itmp3                    /* call JIT compiler                    */

	add     bp,sp                     /* remove argument stack frame if any   */

L_asm_calljavafunction2_return:
	mov     0*8(sp),%rbx                /* restore callee saved registers     */
	mov     1*8(sp),s0
	mov     2*8(sp),s1
	mov     3*8(sp),s2
	mov     4*8(sp),s3
	mov     5*8(sp),s4
	add     $(7*8),sp                   /* free stack space                   */
	ret
		
calljava_xhandler2:
	mov     xptr,a0                     /* pass exception pointer             */
	call    builtin_throw_exception
	xor     v0,v0                       /* return NULL                        */
	jmp     L_asm_calljavafunction2_return


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
	                                    /* keep stack 16-byte aligned         */
	sub     $((3+ARG_CNT)*8+sizestackframeinfo),sp

	mov     t0,0*8(sp)                  /* save register                      */

	mov     (3+ARG_CNT)*8+sizestackframeinfo(sp),itmp3 /* get return address  */
	mov     -1(itmp3),%bl               /* get function code                  */
	cmp     $0xd2,%bl                   /* called with `call *REG_ITMP2'?     */
	jne     L_not_static_special

	sub     $11,itmp3                   /* calculate address of immediate     */
	jmp     L_call_jit_compile
		
L_not_static_special:
	cmp     $0xd0,%bl                   /* called with `call *REG_ITMP1'      */
	jne     L_not_virtual_interface
		
	sub     $7,itmp3                    /* calculate address of offset        */
	mov     (itmp3),itmp3l              /* get offset (32-bit)                */
	add     itmp2,itmp3                 /* add base address to get method addr*/
	jmp     L_call_jit_compile

L_not_virtual_interface:                /* a call from asm_calljavamethod     */
	xor     itmp3,itmp3
		
L_call_jit_compile:
	mov     0*8(sp),t0                  /* restore register                   */
	mov     itmp3,0*8(sp)               /* save address for method pointer    */
	mov     itmp1,1*8(sp)               /* save method pointer                */

	SAVE_ARGUMENT_REGISTERS(3)

	mov     sp,a0                       /* create stackframe info             */
	add     $((3+ARG_CNT)*8),a0         /* pass sfi                           */
	xor     a1,a1                       /* if pv is NULL, use findmethod      */
	mov     sp,a2                       /* pass java sp                       */
	add     $((1+3+ARG_CNT)*8+sizestackframeinfo),a2
	                                    /* pass ra to java function           */
	mov     ((3+ARG_CNT)*8+sizestackframeinfo)(sp),a3
	mov     a3,a4                       /* xpc is equal to ra                 */
	call    stacktrace_create_extern_stackframeinfo

	mov     1*8(sp),a0                  /* pass method pointer                */
	call	jit_compile
	mov     v0,1*8(sp)                  /* save return value                  */

	mov     sp,a0                       /* remove stackframe info             */
	add     $((3+ARG_CNT)*8),a0         /* pass sfi                           */
	call    stacktrace_remove_stackframeinfo

	mov     0*8(sp),itmp3               /* restore address for method pointer */
	mov     1*8(sp),v0                  /* restore return value               */

	RESTORE_ARGUMENT_REGISTERS(3)

	add     $((3+ARG_CNT)*8+sizestackframeinfo),sp /* remove stack frame      */

	test    v0,v0                       /* check for exception                */
	je      L_asm_call_jit_compiler_exception

	test    itmp3,itmp3                 /* is address == 0 (asm_calljavamethod*/
	je      L_call_method

	mov     v0,(itmp3)                  /* and now save the new pointer       */

L_call_method:
	jmp     *v0                         /* ...and now call the new method     */

L_asm_call_jit_compiler_exception:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	call    builtin_asm_get_exceptionptrptr
	mov     v0,itmp2                    /* v0 == xptr                         */
#else
	lea     _no_threads_exceptionptr,itmp2
#endif
	mov     (itmp2),xptr                /* get the exception pointer          */
	movl    $0,(itmp2)                  /* clear exception pointer            */

	pop     xpc                         /* delete return address              */
	sub     $5,xpc                      /* faulting address is ra - 5         */
	jmp     asm_handle_exception


/* asm_handle_exception ********************************************************
*                                                                              *
*   This function handles an exception. It does not use the usual calling      *
*   conventions. The exception pointer is passed in REG_ITMP1 and the          *
*   pc from the exception raising position is passed in REG_ITMP2. It searches *
*   the local exception table for a handler. If no one is found, it unwinds    *
*   stacks and continues searching the callers.                                *
*                                                                              *
*******************************************************************************/

asm_handle_nat_exception:
	add     $8,sp                       /* clear return address of native stub*/
		
asm_handle_exception:
	sub     $((ARG_CNT+TMP_CNT)*8),sp   /* create maybe-leaf stackframe       */

	SAVE_ARGUMENT_REGISTERS(0)          /* we save arg and temp registers in  */
	SAVE_TEMPORARY_REGISTERS(ARG_CNT)   /* case this is a leaf method         */

	mov     $((ARG_CNT+TMP_CNT)*8),a3   /* prepare a3 for handle_exception    */
	mov     $1,t0                       /* set maybe-leaf flag                */

L_asm_handle_exception_stack_loop:
	sub     $(6*8),sp
	mov     xptr,0*8(sp)                /* save exception pointer             */
	mov     xpc,1*8(sp)                 /* save exception pc                  */
	add     sp,a3                       /* calculate Java sp into a3...       */
	add     $(6*8),a3
	mov     a3,3*8(sp)                  /* ...and save it                     */
	mov     t0,4*8(sp)                  /* save maybe-leaf flag               */

	mov     xpc,a0                      /* exception pc                       */
	call    codegen_findmethod
	mov     v0,2*8(sp)                  /* save data segment pointer          */
        
	mov     0*8(sp),a0                  /* pass exception pointer             */
	mov     1*8(sp),a1                  /* pass exception pc                  */
	mov     v0,a2                       /* pass data segment pointer          */
	mov     3*8(sp),a3                  /* pass Java stack pointer            */
	call    exceptions_handle_exception

	test    v0,v0
	jz      L_asm_handle_exception_not_catched

	mov     v0,xpc                      /* move handlerpc into xpc            */
	mov     0*8(sp),xptr                /* restore exception pointer          */
	mov     4*8(sp),t0                  /* get maybe-leaf flag                */
	add     $(6*8),sp                   /* free stack frame                   */

	test    t0,t0                       /* test for maybe-leaf flag           */
	jz      L_asm_handle_exception_no_leaf

	RESTORE_ARGUMENT_REGISTERS(0)       /* if this is a leaf method, we have  */
	RESTORE_TEMPORARY_REGISTERS(ARG_CNT)/* to restore arg and temp registers  */

	add     $((ARG_CNT+TMP_CNT)*8),sp   /* remove maybe-leaf stackframe       */

L_asm_handle_exception_no_leaf:
	jmp     *xpc                        /* jump to the handler                */

L_asm_handle_exception_not_catched:
	mov     0*8(sp),xptr                /* restore exception pointer          */
	mov     2*8(sp),itmp3               /* restore data segment pointer       */
	mov     4*8(sp),t0                  /* get maybe-leaf flag                */
	add     $(6*8),sp

	test    t0,t0
	jz      L_asm_handle_exception_no_leaf_stack

	add     $((ARG_CNT+TMP_CNT)*8),sp   /* remove maybe-leaf stackframe       */
	xor     t0,t0                       /* clear the isleaf flags             */

L_asm_handle_exception_no_leaf_stack:
	mov     FrameSize(itmp3),itmp2l     /* get frame size                     */
	add     sp,itmp2                    /* pointer to save area               */
	
	mov     IntSave(itmp3),a0l          /* a0l = saved int register count     */
	test    a0l,a0l
	je      noint
	
	cmp     $1,a0l
	je      int1
	cmp     $2,a0l
	je      int2
	cmp     $3,a0l
	je      int3
	cmp     $4,a0l
	je      int4
	
	mov     -5*8(itmp2),s0
int4:   
	mov     -4*8(itmp2),s1
int3:   
	mov     -3*8(itmp2),s2
int2:   
	mov     -2*8(itmp2),s3
int1:   
	mov     -1*8(itmp2),s4

	shl     $3,a0l                      /* multiply by 8 bytes                */
	sub     a0,itmp2
		
noint:
#if 0
	mov     FltSave(itmp3),a0l          /* a0l = saved flt register count     */
	test	a0l,a0l
	je      noflt
	
	cmpl    $1,a0l
	je      flt1
	cmpl    $2,a0l
	je      flt2
	cmpl    $3,a0l
	je      flt3
	cmpl    $4,a0l
	je      flt4

	movq    -5*8(itmp2),%xmm11
flt4:   
	movq    -4*8(itmp2),%xmm12
flt3:   
	movq    -3*8(itmp2),%xmm13
flt2:   
	movq    -2*8(itmp2),%xmm14
flt1:   
	movq    -1*8(itmp2),%xmm15
                
noflt:
#endif
	mov     FrameSize(itmp3),itmp2l     /* get frame size                     */
	add     itmp2,sp                    /* unwind stack                       */

	                                    /* exception pointer is still set     */
	pop     xpc                         /* the new xpc is return address      */
	sub     $3,xpc                      /* subtract 3 bytes for call          */

	xor     a3,a3                       /* prepare a3 for handle_exception    */
	
	jmp		L_asm_handle_exception_stack_loop


/* asm_wrapper_patcher *********************************************************

   XXX

   Stack layout:
     40   return address
     32   pointer to virtual java_objectheader
     24   machine code (which is patched back later)
     16   unresolved class/method/field reference
      8   data segment displacement from load instructions
      0   pointer to patcher function
     -8   bp

*******************************************************************************/

asm_wrapper_patcher:
	push    bp                          /* save base pointer                  */
	mov     sp,bp                       /* move actual sp to bp               */
	sub     $((3+ARG_CNT+TMP_CNT)*8+sizestackframeinfo),sp
	and     $0xfffffffffffffff0,sp      /* align sp to 16-byte (this is for   */
	                                    /* leaf functions)                    */

	SAVE_ARGUMENT_REGISTERS(3)
	SAVE_TEMPORARY_REGISTERS(3+ARG_CNT)

	mov     itmp1,0*8(sp)               /* save itmp1 and itmp2               */
	mov     itmp2,1*8(sp)               /* can be used by some instructions   */

	mov     sp,a0                       /* create stackframe info             */
	add     $((3+ARG_CNT+TMP_CNT)*8),a0
	xor     a1,a1                       /* if pv is NULL, use findmethod      */
	mov     bp,a2                       /* pass java sp                       */
	add     $((6+1)*8),a2
	mov     ((5+1)*8)(bp),a3            /* pass ra to java function           */
	mov     a3,a4                       /* xpc is equal to ra                 */
	call    stacktrace_create_extern_stackframeinfo

	mov     bp,a0                       /* pass stack pointer                 */
	add     $((1+1)*8),a0               /* skip function pointer              */
	mov     1*8(bp),itmp3               /* get function pointer               */
	call    *itmp3                      /* call the patcher function          */
	mov     v0,2*8(sp)                  /* save return value                  */

	mov     sp,a0                       /* remove stackframe info             */
	add     $((3+ARG_CNT+TMP_CNT)*8),a0
	call    stacktrace_remove_stackframeinfo

	RESTORE_ARGUMENT_REGISTERS(3)
	RESTORE_TEMPORARY_REGISTERS(3+ARG_CNT)

	mov     0*8(sp),itmp1               /* restore itmp1 and itmp2            */
	mov     1*8(sp),itmp2               /* can be used by some instructions   */
	mov     2*8(sp),itmp3               /* restore return value               */

	mov     bp,sp                       /* restore original sp                */
	pop     bp                          /* restore bp                         */
	add     $(5*8),sp                   /* remove patcher stackframe, keep ra */

	test    itmp3,itmp3                 /* exception thrown?                  */
	jz      L_asm_wrapper_patcher_exception
	ret                                 /* call new patched code              */

L_asm_wrapper_patcher_exception:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	call    builtin_asm_get_exceptionptrptr
	mov     v0,itmp2                    /* v0 == xptr                         */
#else
	lea     _no_threads_exceptionptr,itmp2
#endif
	mov     (itmp2),xptr                /* get the exception pointer          */
	movl    $0,(itmp2)                  /* clear exception pointer            */

	pop     xpc                         /* get and remove return address      */
	jmp     asm_handle_exception


/* asm_builtin_x2x *************************************************************
*                                                                              *
*   Wrapper functions for float to int corner cases                            *
*                                                                              *
*******************************************************************************/

asm_builtin_f2i:
	sub     $(ARG_CNT*8),sp
	
	SAVE_ARGUMENT_REGISTERS(0)
	
	movq    ftmp1,fa0
	call    builtin_f2i
	
	RESTORE_ARGUMENT_REGISTERS(0)
	
	add     $(ARG_CNT*8),sp
	ret


asm_builtin_f2l:
	sub     $(ARG_CNT*8),sp
	
	SAVE_ARGUMENT_REGISTERS(0)
	
	movq    ftmp1,fa0
	call    builtin_f2l
	
	RESTORE_ARGUMENT_REGISTERS(0)
	
	add     $(ARG_CNT*8),sp
	ret


asm_builtin_d2i:
	sub     $(ARG_CNT*8),sp
	
	SAVE_ARGUMENT_REGISTERS(0)
	
	movq    ftmp1,fa0
	call    builtin_d2i
	
	RESTORE_ARGUMENT_REGISTERS(0)
	
	add     $(ARG_CNT*8),sp
	ret


asm_builtin_d2l:
	sub     $(ARG_CNT*8),sp
	
	SAVE_ARGUMENT_REGISTERS(0)
	
	movq    ftmp1,fa0
	call    builtin_d2l
	
	RESTORE_ARGUMENT_REGISTERS(0)
	
	add     $(ARG_CNT*8),sp
	ret


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
_crit_restart:
_crit_begin:
	movl    offbaseval(a0),itmp1l
	movl    offdiffval(a0),itmp2l
	movl    offbaseval(a1),itmp3l
_crit_end:
	movl    itmp1l,offcast_super_baseval(a2)
	movl    itmp2l,offcast_super_diffval(a2)
	movl    itmp3l,offcast_sub_baseval(a2)
	ret

	.data
		
asm_criticalsections:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	.quad   _crit_begin
	.quad   _crit_end
	.quad   _crit_restart
#endif
	.quad 0


/* Disable exec-stacks, required for Gentoo ***********************************/

#if defined(__GCC__) && defined(__ELF__)
	.section .note.GNU-stack,"",@progbits
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
