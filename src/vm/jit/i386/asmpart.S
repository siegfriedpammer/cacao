/* src/vm/jit/i386/asmpart.S - Java-C interface functions for i386

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

   Changes: Joseph Wenninger
            Edwin Steiner

   $Id: asmpart.S 4921 2006-05-15 14:24:36Z twisti $

*/


#include "config.h"

#include "vm/jit/i386/arch.h"
#include "vm/jit/i386/md-abi.h"
#include "vm/jit/i386/md-asm.h"
#include "vm/jit/i386/offsets.h"

#include "vm/jit/abi-asm.h"
#include "vm/jit/methodheader.h"


	.text


/* export functions ***********************************************************/

	.globl asm_md_init

	.globl asm_vm_call_method
	.globl asm_vm_call_method_int
	.globl asm_vm_call_method_long
	.globl asm_vm_call_method_float
	.globl asm_vm_call_method_double
	.globl asm_vm_call_method_exception_handler

	.globl asm_call_jit_compiler
	.globl asm_handle_nat_exception
	.globl asm_handle_exception

	.globl asm_wrapper_patcher

	.globl asm_replacement_out
	.globl asm_replacement_in

	.globl asm_builtin_f2i
	.globl asm_builtin_f2l
	.globl asm_builtin_d2i
	.globl asm_builtin_d2l

	.globl asm_perform_threadswitch
	.globl asm_initialize_thread_stack
	.globl asm_switchstackandcall
	.globl asm_criticalsections
	.globl asm_getclassvalues_atomic

	.globl asm_get_cycle_count


/* asm_md_init *****************************************************************

   Initialize machine dependent stuff.

   See: http://www.srware.com/linux_numerics.txt

   This puts the X86 FPU in 64-bit precision mode.  The default under
   Linux is to use 80-bit mode, which produces subtle differences from
   FreeBSD and other systems, eg, (int)(1000*atof("0.3")) is 300 in
   64-bit mode, 299 in 80-bit mode.

   Fixes: http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=350729

*******************************************************************************/

asm_md_init:
	sub     $4,sp                       /* allocate space for the FPU state   */
	fnstcw  (sp)                        /* get the FPU state                  */
	mov     (sp),%eax
	and     $0xfcff,%ax                 /* remove the extended mode flag      */
	or      $0x0200,%ax                 /* put the double mode flag           */
	mov     %eax,(sp)                   /* store new FPU state                */
	fldcw   (sp)                        /* setup new FPU state                */
	add     $4,sp
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
*    javaobject_header *asm_vm_call_method(methodinfo *m,                      *
*         u4 count, u4 size, void *callblock);                                 *
*                                                                              *
*******************************************************************************/

	.align	8

	.long   0                           /* catch type all                     */
	.long   0                           /* handler pc                         */
	.long   0                           /* end pc                             */
	.long   0                           /* start pc                           */
	.long   1                           /* extable size                       */
	.long   0                           /* line number table start            */
	.long   0                           /* line number table size             */
	.long   0                           /* fltsave                            */
	.long   0                           /* intsave                            */
	.long   0                           /* isleaf                             */
	.long   0                           /* IsSync                             */
	.long   0                           /* frame size                         */
	.long   0                           /* method pointer (pointer to name)   */

asm_vm_call_method:
asm_vm_call_method_int:
asm_vm_call_method_long:
asm_vm_call_method_float:
asm_vm_call_method_double:
	push    bp
	mov     sp,bp                       /* save stackptr                      */
	sub     $(4*4),sp                   /* create stackframe                  */
	and     $0xfffffff0,sp              /* align stack to 16-byte             */

	mov     t0,0*4(sp)                  /* save registers                     */
	mov     s1,1*4(sp)
	mov     s2,2*4(sp)

	mov     4*4(bp),itmp1               /* pointer to arg block (4(push)+4(return)+4+4)*/
	mov     3*4(bp),itmp2               /* arg count            (4(push)+4(return)+4 */

	mov     sp,s1                       /* save the stackpointer              */

	test    itmp2,itmp2                 /* maybe we have no args              */
	jle     calljava_copydone

	mov     itmp2,itmp3                 /* calculate stack size               */
	mov     itmp1,%edi                  /* save pointer to arg block          */

calljava_calcstacksize:
	mov     offvmargtype(itmp1),t0
	test    $1,t0                       /* two word type?                     */
	jz      calljava_onewordtype

	sub     $4,sp                       /* add 1 slot to stackframe size      */

calljava_onewordtype:
	sub     $4,sp                       /* add 1 slot to stackframe size      */
	sub     $1,itmp3
	test	itmp3,itmp3                 /* any args left?                     */
	jz      calljava_setstack

	add     $sizevmarg,itmp1            /* goto next argument block           */
	jmp     calljava_calcstacksize
		
calljava_setstack:				
	mov     %edi,itmp1                  /* restore pointer to arg block       */
	and     $0xfffffff0,sp              /* align stack to 16-byte             */
	mov     sp,itmp3                    /* initialize pointer for copying     */

calljava_copyloop:
	mov     offvmargdata(itmp1),t0      /* get 4-bytes of argument            */
	mov     t0,(itmp3)                  /* and store them on the stack        */
	add     $4,itmp3                    /* increase sp to next argument       */
	mov     offvmargtype(itmp1),t0      /* get the argument type              */
	test    $1,t0                       /* two word type?                     */
	jz      calljava_copynext

	mov     offvmargdata+4(itmp1),t0    /* get upper 4-bytes of 2 word type   */
	mov     t0,(itmp3)			
	add     $4,itmp3                    /* increase sp to next argument       */

calljava_copynext:		
	sub     $1,itmp2                    /* are there any args left?           */
	test    itmp2,itmp2
	jle     calljava_copydone

	add     $sizevmarg,itmp1            /* goto next argument block           */
	jmp     calljava_copyloop

calljava_copydone:
	mov     2*4(bp),itmp1               /* move function pointer to itmp1     */

	lea     L_asm_call_jit_compiler,itmp3
	call    *itmp3                      /* call JIT compiler                  */

L_asm_vm_call_method_return:
	mov     s1,sp                       /* restore stackpointer               */

	mov     0*4(sp),t0                  /* restore registers                  */
	mov     1*4(sp),s1
	mov     2*4(sp),s2

	leave
	ret

asm_vm_call_method_exception_handler:
	push    xptr                        /* pass exception pointer             */
	call    builtin_throw_exception
	add     $4,sp
	jmp     L_asm_vm_call_method_return


/* asm_call_jit_compiler *******************************************************

   Invokes the compiler for untranslated JavaVM methods.

   Register R0 contains a pointer to the method info structure (prepared
   by createcompilerstub). Using the return address in R26 and the
   offset in the LDA instruction or using the value in methodptr R28 the
   patching address for storing the method address can be computed:

   Method address was either loaded using

   i386_mov_imm_reg(a, REG_ITMP2)                ; invokestatic/special
   i386_call_reg(REG_ITMP2)

   or

   i386_mov_membase_reg(REG_SP, 0, REG_ITMP1)    ; invokevirtual/interface
   i386_mov_membase_reg(REG_ITMP1, OFFSET(, vftbl), REG_ITMP2)
   i386_mov_membase_reg(REG_ITMP2, OFFSET(vftbl, table[0]) + \
       sizeof(methodptr) * m->vftblindex, REG_ITMP1)
   i386_call_reg(REG_ITMP1)

   In the static case the method pointer can be computed using the
   return address and the lda function following the jmp instruction.

*******************************************************************************/

asm_call_jit_compiler:
L_asm_call_jit_compiler:                /* required for PIC code              */
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
#if defined(ENABLE_THREADS)
	call    builtin_asm_get_exceptionptrptr
	mov     v0,itmp2                    /* v0 == itmp1                        */
#else
	lea     _exceptionptr,itmp2
#endif
	mov     (itmp2),xptr                /* get the exception pointer          */
	movl    $0,(itmp2)                  /* clear the exception pointer        */

	pop     xpc                         /* get return address                 */
	sub     $2,xpc                      /* faulting address is ra - 2         */
	jmp     L_asm_handle_exception


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
	add     $4,sp                       /* clear return address of native stub*/
		
asm_handle_exception:
L_asm_handle_exception:                 /* required for PIC code              */
	sub     $((ARG_CNT+TMP_CNT)*4),sp   /* create maybe-leaf stackframe       */

	SAVE_ARGUMENT_REGISTERS(0)          /* we save arg and temp registers in  */
	SAVE_TEMPORARY_REGISTERS(ARG_CNT)   /* case this is a leaf method         */

	mov     $((ARG_CNT+TMP_CNT)*4),itmp3/* prepare a3 for handle_exception    */
	mov     $1,t0                       /* set maybe-leaf flag                */

L_asm_handle_exception_stack_loop:
	sub     $(10*4),sp                  /* create stackframe                  */
	mov     xptr,4*4(sp)                /* save exception pointer             */
	mov     xpc,5*4(sp)                 /* save exception pc                  */
	add     sp,itmp3                    /* calculate Java sp into a3...       */
	add     $(10*4),itmp3
	mov     itmp3,7*4(sp)               /* ...and save it                     */
	mov     t0,8*4(sp)                  /* save maybe-leaf flag               */

	mov     xpc,0*4(sp)                 /* pass exception pc                  */
	call    codegen_findmethod
	mov     v0,6*4(sp)                  /* save data segment pointer          */

	mov     4*4(sp),itmp3               /* pass exception pointer             */
	mov     itmp3,0*4(sp)
	mov     5*4(sp),itmp3               /* pass exception pc                  */
	mov     itmp3,1*4(sp)
	mov     v0,2*4(sp)                  /* pass data segment pointer          */
	mov     7*4(sp),itmp3               /* pass Java stack pointer            */
	mov     itmp3,3*4(sp)
	call    exceptions_handle_exception

	test    v0,v0
	jz      L_asm_handle_exception_not_catched

	mov     v0,xpc                      /* move handlerpc into xpc            */
	mov     4*4(sp),xptr                /* restore exception pointer          */
	mov     8*4(sp),t0                  /* get maybe-leaf flag                */
	add     $(10*4),sp                  /* free stackframe                    */

	test    t0,t0                       /* test for maybe-leaf flag           */
	jz      L_asm_handle_exception_no_leaf

	RESTORE_ARGUMENT_REGISTERS(0)       /* if this is a leaf method, we have  */
	RESTORE_TEMPORARY_REGISTERS(ARG_CNT)/* to restore arg and temp registers  */

	add     $((ARG_CNT+TMP_CNT)*4),sp   /* remove maybe-leaf stackframe       */

L_asm_handle_exception_no_leaf:
	jmp     *xpc                        /* jump to exception handler          */

L_asm_handle_exception_not_catched:
	mov     4*4(sp),xptr                /* restore exception pointer          */
	mov     6*4(sp),itmp3               /* restore data segment pointer       */
	mov     8*4(sp),t0                  /* get maybe-leaf flag                */
	add     $(10*4),sp                  /* free stackframe                    */

	test    t0,t0
	jz      L_asm_handle_exception_no_leaf_stack

	add     $((ARG_CNT+TMP_CNT)*4),sp   /* remove maybe-leaf stackframe       */
	xor     t0,t0                       /* clear the maybe-leaf flag          */

L_asm_handle_exception_no_leaf_stack:
	mov     FrameSize(itmp3),itmp2      /* get frame size                     */
	add     sp,itmp2                    /* pointer to save area               */

	push    xptr                        /* we are out of registers            */

	mov     IntSave(itmp3),itmp1        /* itmp1 = saved int register count   */
	test    itmp1,itmp1
	je      noint

	cmp     $1,itmp1
	je      int1
	cmp     $2,itmp1
	je      int2

	mov     -3*4(itmp2),s0
int2:	
	mov     -2*4(itmp2),s1
int1:	
	mov     -1*4(itmp2),s2

	shl     $2,itmp1                    /* multiply by 4 bytes                */
	sub     itmp1,itmp2
		
noint:
#if 0
	mov     FltSave(itmp3),itmp1        /* itmp1 = saved flt register count   */
	test    itmp1,itmp1
	je      noflt

	cmp     $1,itmp1
	je      flt1
	cmp     $2,itmp1
	je      flt2
	cmp     $3,itmp1
	je      flt3
		
	fldl    -4*8(itmp2)
	fstp    %st(1)
flt3:
	fldl    -3*8(itmp2)
	fstp    %st(2)
flt2:
	fldl    -2*8(itmp2)
	fstp    %st(3)
flt1:
	fldl    -1*8(itmp2)
	fstp    %st(4)
		
noflt:
#endif
	pop     xptr                        /* restore exception pointer          */
	mov     FrameSize(itmp3),itmp2      /* get frame size                     */
	add     itmp2,sp                    /* unwind stack                       */

	pop     xpc                         /* the new xpc is return address      */
	sub     $2,xpc                      /* subtract 2-bytes for call          */

	xor     itmp3,itmp3                 /* prepare a3 for handle_exception    */

	jmp     L_asm_handle_exception_stack_loop
		

/* asm_wrapper_patcher *********************************************************

   XXX

   Stack layout:
     24   return address
     20   REG_ITMP3
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
	add     $((7+2+4)*4+sizestackframeinfo),itmp2
	mov     itmp2,2*4(sp)               /* pass Java sp                       */
	mov     ((6+2+4)*4+sizestackframeinfo)(sp),itmp3
	mov     itmp3,3*4(sp)               /* pass ra to java function           */
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

	mov     1*4(sp),itmp3               /* restore return value               */
	test    itmp3,itmp3                 /* exception thrown?                  */
	jz      L_asm_wrapper_patcher_exception

	mov     (0+4)*4(sp),itmp1           /* restore itmp1 and itmp2            */
	mov     (1+4)*4(sp),itmp2           /* may be used by some instructions   */
	mov     ((5+2+4)*4+sizestackframeinfo)(sp),itmp3
	add     $((6+2+4)*4+sizestackframeinfo),sp /* remove stack frame, keep ra */

	ret                                 /* call new patched code              */

L_asm_wrapper_patcher_exception:
	add     $((6+2+4)*4+sizestackframeinfo),sp /* remove stack frame, keep ra */

#if defined(ENABLE_THREADS)
	call    builtin_asm_get_exceptionptrptr
	mov     v0,itmp2
#else
	lea     _exceptionptr,itmp2
#endif
	mov     (itmp2),xptr                /* get the exception pointer          */
	movl    $0,(itmp2)                  /* clear the exception pointer        */

	pop     xpc                         /* get and remove return address      */
	jmp     L_asm_handle_exception


/* asm_replacement_out *********************************************************

   This code is jumped to from the replacement-out stubs that are executed
   when a thread reaches an activated replacement point.

   The purpose of asm_replacement_out is to read out the parts of the
   execution state that cannot be accessed from C code, store this state,
   and then call the C function replace_me.

   Stack layout:
      4                 start of stack inside method to replace
      0   rplpoint *    info on the replacement point that was reached

*******************************************************************************/

/* some room to accomodate changes of the stack frame size during replacement */
	/* XXX we should find a cleaner solution here */
#define REPLACEMENT_ROOM  512

asm_replacement_out:
    /* create stack frame */
	sub     $(sizeexecutionstate + REPLACEMENT_ROOM),sp

	/* save registers in execution state */
	mov     %eax,(EAX*8+offes_intregs)(sp)
	mov     %ebx,(EBX*8+offes_intregs)(sp)
	mov     %ecx,(ECX*8+offes_intregs)(sp)
	mov     %edx,(EDX*8+offes_intregs)(sp)
	mov     %esi,(ESI*8+offes_intregs)(sp)
	mov     %edi,(EDI*8+offes_intregs)(sp)
	mov     %ebp,(EBP*8+offes_intregs)(sp)
	movl    $0  ,(ESP*8+offes_intregs)(sp) /* not used */

#ifndef NDEBUG
	/* clear high 32bit */
	movl    $0,(4+0*8+offes_intregs)(sp)
	movl    $0,(4+1*8+offes_intregs)(sp)
	movl    $0,(4+2*8+offes_intregs)(sp)
	movl    $0,(4+3*8+offes_intregs)(sp)
	movl    $0,(4+4*8+offes_intregs)(sp)
	movl    $0,(4+5*8+offes_intregs)(sp)
	movl    $0,(4+6*8+offes_intregs)(sp)
	movl    $0,(4+7*8+offes_intregs)(sp)
#endif

	/* calculate sp of method */
	mov     sp,itmp1
	add     $(sizeexecutionstate + REPLACEMENT_ROOM + 4),itmp1
	mov     itmp1,(offes_sp)(sp)

	/* pv must be looked up via AVL tree */
	movl    $0,(offes_pv)(sp)

	/* call replace_me */
	mov     -4(itmp1),itmp1             /* rplpoint *                         */
    push    sp                          /* arg1: execution state              */
    push    itmp1                       /* arg0: replacement point            */
    call    replace_me                  /* call C function replace_me         */
    call    abort                       /* NEVER REACHED                      */

/* asm_replacement_in **********************************************************

   This code writes the given execution state and jumps to the replacement
   code.

   This function never returns!

   C prototype:
      void asm_replacement_in(executionstate *es);

*******************************************************************************/

asm_replacement_in:
	mov     4(sp),%ebp                  /* executionstate *es                 */

	/* set new sp */
	mov     (offes_sp)(%ebp),%esp
	
	/* store address of new code */
	push    (offes_pc)(%ebp)
	
	/* copy registers from execution state */
	mov     (EAX*8+offes_intregs)(%ebp),%eax
	mov     (EBX*8+offes_intregs)(%ebp),%ebx
	mov     (ECX*8+offes_intregs)(%ebp),%ecx
	mov     (EDX*8+offes_intregs)(%ebp),%edx
	mov     (ESI*8+offes_intregs)(%ebp),%esi
	mov     (EDI*8+offes_intregs)(%ebp),%edi

	mov     (EBP*8+offes_intregs)(%ebp),%ebp

	/* jump to new code */
	ret

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
#if defined(ENABLE_THREADS)
#if 0
	.long   _crit_begin1
	.long   _crit_end1
	.long   _crit_restart1
#endif
	.long   _crit_begin2
	.long   _crit_end2
	.long   _crit_restart2
#endif
	.long 0


/* Disable exec-stacks, required for Gentoo ***********************************/

#if defined(__GCC__) && defined(__ELF__)
	.section .note.GNU-stack,"",@progbits
#endif


/* asm_get_cycle_count *********************************************************

   Get the current time-stamp counter from the CPU.

*******************************************************************************/

asm_get_cycle_count:
	rdtsc
	ret


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
 * vim:noexpandtab:sw=4:ts=4:
 */
