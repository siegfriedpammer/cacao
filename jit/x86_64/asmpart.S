/* jit/x86_64/asmpart.S - Java-C interface functions for x86_64

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

   $Id: asmpart.S 713 2003-12-07 20:40:43Z twisti $

*/


#include "offsets.h"

	.text


/********************* exported functions and variables ***********************/

	.globl asm_calljavamethod
	.globl asm_calljavafunction
	.globl asm_calljavafunction2
	.globl asm_calljavafunction2long
	.globl asm_calljavafunction2double
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
    .globl asm_builtin_f2i
    .globl asm_builtin_f2l
    .globl asm_builtin_d2i
    .globl asm_builtin_d2l
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
	.globl findmethod

        
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

	.align	8
	.quad   0                         /* catch type all                       */
	.quad   calljava_xhandler         /* handler pc                           */
	.quad   calljava_xhandler         /* end pc                               */
	.quad   asm_calljavamethod        /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   8                         /* frame size                           */
	.quad   0                         /* method pointer (pointer to name)     */

asm_calljavamethod:
        sub     $8,%rsp               /* keep %rsp 16-byte aligned            */
        mov     %rbp,(%rsp)
        
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
        mov     (%rsp),%rbp
        add     $8,%rsp               /* keep %rsp 16-byte aligned            */
        ret

calljava_xhandler:
        mov     %rax,%rdi			  /* pass exception pointer               */
        call    builtin_throw_exception
        mov     (%rsp),%rbp
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

	.align	8
	.quad   0                         /* catch type all                       */
	.quad   calljava_xhandler2        /* handler pc                           */
	.quad   calljava_xhandler2        /* end pc                               */
	.quad   asm_calljavafunction      /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   8                         /* frame size                           */
	.quad   0                         /* method pointer (pointer to name)     */

asm_calljavafunction:
        sub     $8,%rsp               /* keep stack 16-byte aligned           */
        mov     %rbp,(%rsp)
        mov     %rdi,%rax             /* move function pointer to %rax        */
                                      /* compilerstub uses this               */

        mov     %rsi,%rdi             /* pass remaining parameters            */
        mov     %rdx,%rsi
        mov     %rcx,%rdx
        mov     %r8,%rcx
        
        lea     asm_call_jit_compiler,%r11
        call    *%r11                 /* call JIT compiler                    */
	
calljava_jit2:
calljava_return2:
calljava_ret2:
        mov     (%rsp),%rbp
        add     $8,%rsp               /* free stack space                     */
        ret

calljava_xhandler2:
        mov     %rax,%rdi       	  /* pass exception pointer               */
        call    builtin_throw_exception
        mov     (%rsp),%rbp
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

call_name3:
	.ascii  "calljavafunction2\0\0"

	.align	8
	.quad   0                         /* catch type all                       */
	.quad   calljava_xhandler3        /* handler pc                           */
	.quad   calljava_xhandler3        /* end pc                               */
	.quad   asm_calljavafunction2     /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   8                         /* frame size                           */
	.quad   0                         /* method pointer (pointer to name)     */

asm_calljavafunction2:
asm_calljavafunction2double:
asm_calljavafunction2long:
        sub     $8,%rsp               /* keep stack 16-byte aligned           */

        mov     %rdi,%rax             /* save method pointer for compiler     */
        mov     %rcx,%r10             /* pointer to arg block                 */
        
        mov     offjniitem(%r10),%rdi /* move args into registers             */
        mov     offjniitem + sizejniblock * 1(%r10),%rsi
        mov     offjniitem + sizejniblock * 2(%r10),%rdx
        mov     offjniitem + sizejniblock * 3(%r10),%rcx
        
        lea     asm_call_jit_compiler,%r11
        call    *%r11                 /* call JIT compiler                    */
	
calljava_jit3:
calljava_return3:
calljava_ret3:
        add     $8,%rsp               /* free stack space                     */
        ret

calljava_xhandler3:
        mov     %rax,%rdi       	  /* pass exception pointer               */
        call    builtin_throw_exception
        add     $8,%rsp
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
        sub     $8,%rsp         /* keep stack 16-byte aligned                 */

        mov     %rbx,(%rsp)     /* save register                              */
        
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

        movq    %xmm8,15*8(%rsp) /* we use them as callee saved registers      */
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

        movq    15*8(%rsp),%xmm8
        movq    16*8(%rsp),%xmm9
        movq    17*8(%rsp),%xmm10
        movq    18*8(%rsp),%xmm11
        movq    19*8(%rsp),%xmm12
        movq    20*8(%rsp),%xmm13
        movq    21*8(%rsp),%xmm14
        movq    22*8(%rsp),%xmm15

        add     $(24*8),%rsp

		test	%r11,%r11		/* is address == 0 (asm_calljavamethod)       */
		je		L_call_method
		
		mov     %rax,(%r11)		/* and now save the new pointer               */

L_call_method:
        add     $8,%rsp         /* keep stack 16-byte aligned                 */
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
        sub     $(7*8),%rsp             /* allocate stack space               */

        mov     %rbx,0*8(%rsp)          /* save all callee saved registers    */
        mov     %rsp,1*8(%rsp)
        mov     %rbp,2*8(%rsp)
        mov     %r12,3*8(%rsp)
        mov     %r13,4*8(%rsp)
        mov     %r14,5*8(%rsp)
        mov     %r15,6*8(%rsp)

        xor     %rax,%rax               /* intialize the remaining registers  */
        xor     %rcx,%rcx
        xor     %rdx,%rdx
        xor     %rsi,%rsi
        xor     %r8,%r8
        xor     %r9,%r9
        xor     %r10,%r10
        xor     %r11,%r11
        
        call    *%rdi                   /* call function                      */

        mov     0*8(%rsp),%rbx
        mov     1*8(%rsp),%rsp
        mov     2*8(%rsp),%rbp
        mov     3*8(%rsp),%r12
        mov     4*8(%rsp),%r13
        mov     5*8(%rsp),%r14
        mov     6*8(%rsp),%r15

        add     $(7*8),%rsp        
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
        add     $8,%rsp						/* clear return address of native stub */
		
asm_handle_exception:
        sub     $(4*8),%rsp
        mov     %rax,0*8(%rsp)              /* save exception pointer         */
        mov     %r10,1*8(%rsp)              /* save exception pc              */
        
        mov     %r10,%rdi                   /* exception pc                   */
        call	findmethod
        mov     %rax,%r11
        mov     %rax,2*8(%rsp)				/* save data segment pointer      */
        
        mov     0*8(%rsp),%rax              /* restore exception pointer      */
        mov     1*8(%rsp),%r10              /* restore exception pc           */
        
ex_stack_loop:
        mov     %rax,%rdi					/* exception pointer              */
        mov     MethodPointer(%r11),%rsi	/* method pointer                 */
        mov     %r10,%rdx                   /* exception pc                   */
        mov     $1,%rcx 					/* set no unwind flag             */
        call	builtin_trace_exception

		mov     2*8(%rsp),%r11				/* %r11 = data segment pointer    */
		mov     ExTableSize(%r11),%rcx		/* %rcx = exception table size    */
		test	%rcx,%rcx					/* if empty table skip            */
		je		empty_table

		lea		ExTableStart(%r11),%rdi		/* %rdi = start of exception table */
		mov     0*8(%rsp),%rax				/* get xptr                       */
		
ex_table_loop:
		mov     1*8(%rsp),%r10              /* get xpc                        */
		
		mov     ExStartPC(%rdi),%rdx        /* %rdx = exception start pc      */
		cmp     %r10,%rdx                   /* %rdx = (startpc <= xpc)        */
		jg		ex_table_cont               /* if (false) continue            */
		mov     ExEndPC(%rdi),%rdx          /* %rdx = exception end pc        */
		cmp     %rdx,%r10                   /* %rdx = (xpc < endpc)           */
		jge		ex_table_cont               /* if (false) continue            */
		mov     ExCatchType(%rdi),%rdx      /* %rdx = exception catch type    */
		test	%rdx,%rdx                   /* NULL catches everything        */
		je		ex_handle_it

		mov     offobjvftbl(%rax),%rsi      /* %rsi = vftblptr(xptr)          */
		mov     offclassvftbl(%rdx),%rdx    /* %rdx = vftblptr(catchtype) class (not obj) */
		mov     offbaseval(%rsi),%esi       /* %esi = baseval(xptr)           */
		mov     offbaseval(%rdx),%r10d      /* %r10d = baseval(catchtype)     */
		mov     offdiffval(%rdx),%edx       /* %edx = diffval(catchtype)      */
		sub     %r10d,%esi                  /* %esi = baseval(xptr) - baseval(catchtype) */
		cmp     %edx,%esi                   /* xptr is instanceof catchtype   */
		ja		ex_table_cont
		
ex_handle_it:
		mov     ExHandlerPC(%rdi),%r10      /* xpc = exception handler pc     */

        mov     0*8(%rsp),%rax              /* restore exception pointer      */
        add     $(4*8),%rsp                 /* free stack frame               */

        jmp		*%r10                       /* jump to the handler            */

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
                
no_monitor_exit:
		mov		FrameSize(%r11),%eax        /* %eax = frame size              */
		add		%rax,%rsp                   /* unwind stack                   */
		mov		%rsp,%rax                   /* %rax = pointer to save area    */

		mov		IntSave(%r11),%edx          /* %edx = saved int register count */
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

		shl		$3,%edx                     /* multiply by 8 bytes             */
		sub     %rdx,%rax
		
noint:
		mov     FltSave(%r11),%edx          /* %edx = saved flt register count */
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
        call	findmethod      			/* get the new data segment ptr   */
        mov     %rax,%r11
        
        mov     0*8(%rsp),%rcx
        mov     1*8(%rsp),%r10
        add     $(2*8),%rsp

        mov     %rcx,%rax					/* restore saved exception pointer */

        sub     $(4*8),%rsp
        				
        mov     %rax,0*8(%rsp)              /* save exception pointer         */
        mov     %r10,1*8(%rsp)              /* save exception pc              */
		mov     %r11,2*8(%rsp)				/* save data segment pointer      */
		
		jmp		ex_stack_loop


/********************* function asm_builtin_monitorenter ***********************
*                                                                              *
*   Does null check and calls monitorenter or throws an exception              *
*                                                                              *
*******************************************************************************/

asm_builtin_monitorenter:
        test    %rdi,%rdi
        je      nb_monitorenter			/* if (null) throw exception          */
        jmp     builtin_monitorenter	/* else call builtin_monitorenter     */

nb_monitorenter:
        pop     %r10					/* delete return address */
        sub     $3,%r10					/* faulting address is return adress - 3 */
        mov     proto_java_lang_NullPointerException,%rax
        jmp     asm_handle_exception
		

/********************* function asm_builtin_monitorexit ************************
*                                                                              *
*   Does null check and calls monitorexit or throws an exception               *
*                                                                              *
*******************************************************************************/

asm_builtin_monitorexit:
        test    %rdi,%rdi
        je      nb_monitorexit			/* if (null) throw exception          */
        jmp     builtin_monitorexit		/* else call builtin_monitorenter     */

nb_monitorexit:
        pop     %r10					/* delete return address */
        sub     $3,%r10					/* faulting address is return adress - 3 */
        mov     proto_java_lang_NullPointerException,%rax
        jmp     asm_handle_exception


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

        
/*********************** function new_builtin_checkcast ************************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/

asm_builtin_checkcast:
        xor     %rax,%rax
        mov     %rax,(%rax)
        ret

		
/******************* function asm_builtin_checkarraycast ***********************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/

asm_builtin_checkarraycast:
        sub     $24,%rsp				/* keep stack 16-byte aligned         */
        mov     %rdi,(%rsp)				/* save object pointer                */
        call    builtin_checkarraycast  /* builtin_checkarraycast             */
        test    %rax,%rax               /* if (false) throw exception         */
        je      nb_carray_throw
        mov     (%rsp),%rax             /* return object pointer              */
        add     $24,%rsp                /* free stack space                   */
        ret

nb_carray_throw:
        add     $24,%rsp
        pop     %r10					/* delete return address              */
        sub     $3,%r10					/* faulting address is return adress - 3 */
        mov     proto_java_lang_ClassCastException,%rax
        jmp     asm_handle_exception

		
/******************* function asm_builtin_aastore ******************************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/

asm_builtin_aastore:
        sub     $24,%rsp         		/* allocate stack space               */
        test    %rdi,%rdi				/* if null pointer throw exception    */
        je      nb_aastore_null

        movl    offarraysize(%rdi),%eax /* load size                          */
        cmpl    %eax,%esi				/* do bound check                     */
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
        sub     $3,%r10					/* faulting address is return adress - 3 */
		
        mov     proto_java_lang_NullPointerException,%rax
        jmp     asm_handle_exception

nb_aastore_bound:
        add     $24,%rsp
        pop     %r10					/* delete return address */
        sub     $3,%r10					/* faulting address is return adress - 3 */
		
        mov     proto_java_lang_ArrayIndexOutOfBoundsException,%rax
        jmp     asm_handle_exception
		
nb_aastore_throw:
        add     $24,%rsp
        pop     %r10					/* delete return address */
        sub     $3,%r10					/* faulting address is return adress - 3 */
		
        mov     proto_java_lang_ArrayStoreException,%rax
        jmp		asm_handle_exception

		
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
        sub     $8,%rsp                 /* keep stack 16-byte aligned         */
        sub     $16,%rdi				/* allocate new stack                 */

        mov     8(%rsp),%rax			/* save return address on new stack   */
        mov     %rax,(%rdi)
        mov     %rsp,8(%rdi)			/* save old stack pointer on new stack */
        mov     %rsp,(%rdx)				/* save old stack pointer to variable */

        mov     %rdi,%rsp				/* switch to new stack                */

        mov     %rcx,%rdi       		/* pass pointer                       */
        call	*%rsi					/* and call function                  */

        mov     (%rsp),%r10				/* load return address                */
        mov     8(%rsp),%rsp			/* switch to old stack                */
        add     $8,%rsp                 /* free stack space                   */
        mov     %r10,(%rsp)             /* write return adress                */
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
*   Stack:                                                                     *
*       java function                                                          *
*       native stub                                                            *
*       Java_java_lang_System_getCallerClass                                   *
*                                                                              *
*******************************************************************************/

asm_getcallingmethod:
        mov     %rbp,%rax               /* return address of native function  */
        add     $(2*8),%rax             /* %rsp, return address               */
        add     $(7*8),%rax             /* native stub stackframe             */
        mov     (%rax),%rdi             /* return address to java function    */
        call    findmethod
        mov     MethodPointer(%rax),%rax
        ret


/*********************** function asm_builtin_trace ****************************
*                                                                              *
*   Intended to be called from the native stub. Saves all argument registers   *
*   and calls builtin_trace_args.                                              *
*                                                                              *
*******************************************************************************/

asm_builtin_trace:
        sub     $(15*8),%rsp       /* 14 + 1 */
        
        mov     %rdi,0*8(%rsp)  /* save arguments                             */
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

        call	builtin_trace_args

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

        add     $(15*8),%rsp
        ret


/********************* function asm_builtin_exittrace **************************
*                                                                              *
*   Intended to be called from the native stub. Saves return value and calls   *
*   builtin_displaymethodstop.                                                 *
*                                                                              *
*******************************************************************************/

asm_builtin_exittrace:
        sub     $(3*8),%rsp
        mov     %rax,0*8(%rsp)
        movq    %xmm0,1*8(%rsp)
        movq    %xmm0,%xmm1
        call    builtin_displaymethodstop
        mov     0*8(%rsp),%rax
        movq    1*8(%rsp),%xmm0
        add     $(3*8),%rsp
        ret


asm_printf:
        push    %rbp
        mov     %rsp,%rbp
        
        push    %rax
        push    %rcx
        push    %rdx
        push    %rbx
        push    %rsi
        push    %rdi
        push    %r8
        push    %r9
        push    %r10
        push    %r11
        push    %r12
        push    %r13
        push    %r14
        push    %r15

        mov     16(%rbp),%rdi
        call    asmprintf

        pop     %r15
        pop     %r14
        pop     %r13
        pop     %r12
        pop     %r11
        pop     %r10
        pop     %r9
        pop     %r8
        pop     %rdi
        pop     %rsi
        pop     %rbx
        pop     %rdx
        pop     %rcx
        pop     %rax
        
        leave
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
 */
