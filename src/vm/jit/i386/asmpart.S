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

   $Id: asmpart.S 771 2003-12-13 23:11:08Z stefan $

*/


#include "offsets.h"

/* data segment offsets */

#define MethodPointer   -4
#define FrameSize       -8
#define IsSync          -12
#define IsLeaf          -16
#define IntSave         -20
#define FltSave         -24
#define ExTableSize     -28
#define ExTableStart    -28

#define ExEntrySize     -16
#define ExStartPC       -4
#define ExEndPC         -8
#define ExHandlerPC     -12
#define ExCatchType     -16

		
	.text


/********************* exported functions and variables ***********************/

	.globl has_no_x_instr_set
	.globl asm_calljavafunction
	.globl asm_calljavafunction2
	.globl asm_calljavafunction2long
	.globl asm_calljavafunction2double

	.globl asm_call_jit_compiler
	.globl asm_dumpregistersandcall
	.globl asm_handle_builtin_exception
	.globl asm_handle_nat_exception
	.globl asm_handle_exception
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
    .globl asm_builtin_trace
    .globl asm_builtin_exittrace
	.globl asm_xadd
    
/*************************** imported functions *******************************/

	.globl jit_compile
	.globl builtin_monitorexit
	.globl builtin_throw_exception
	.globl builtin_trace_exception
	.globl class_java_lang_Object
	.globl findmethod
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	.globl cast_lock
	.globl cast_unlock
#endif

/*********************** function has_no_x_instr_set ***************************
*                                                                              *
*   determines if the byte support instruction set (21164a and higher)         *
*   is available.                                                              *
*                                                                              *
*   Use it on i386 architecture to init the fpu.                               *
*                                                                              *
*******************************************************************************/

has_no_x_instr_set:
		finit                         /* intitialize the fpu                  */

		pushl	$0x027f   /* Round to nearest, 53-bit mode, exceptions masked */
		fldcw	(%esp)
		addl	$4,%esp
		
		xor		%eax,%eax				/* result code 0 (not used for i386)  */
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
	.long   0                         /* catch type all                       */
	.long   calljava_xhandler2        /* handler pc                           */
	.long   calljava_xhandler2        /* end pc                               */
	.long   asm_calljavafunction      /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   32                        /* frame size                           */
	.long   0                         /* method pointer (pointer to name)     */

asm_calljavafunction:
		push	%ebp                  /* allocate stack space                 */
		mov		%esp, %ebp

        push    %ebx                  /* save registers                       */
        push    %esi
        push    %edi
        
		sub		$32,%esp              /* pass the remaining parameters        */
		xor		%edx,%edx

		mov		%edx,28(%esp)         /* convert parms to 8 byte              */
		mov		24(%ebp),%eax
		mov		%eax,24(%esp)
		
		mov		%edx,20(%esp)
		mov		20(%ebp),%eax
		mov		%eax,16(%esp)

		mov		%edx,12(%esp)
		mov		16(%ebp),%eax
		mov		%eax,8(%esp)

		mov		%edx,4(%esp)
		mov		12(%ebp),%eax
		mov		%eax,(%esp)

		mov		8(%ebp),%eax          /* move function pointer to %eax        */

		lea		asm_call_jit_compiler,%edx 
		call	*%edx                 /* call JIT compiler                    */
	
calljava_jit2:
calljava_return2:
calljava_ret2:
        add     $32,%esp
        pop     %edi                 /* restore registers                     */
        pop     %esi
        pop     %ebx
		leave
		ret

calljava_xhandler2:
		push    %eax				  /* pass exception pointer               */
		call    builtin_throw_exception
		add     $4,%esp

   		add     $32,%esp
        pop     %edi                 /* restore registers                     */
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
*    javaobject_header *asm_calljavamethod (methodinfo *m,                     *
*         void *arg1, void *arg2, void *arg3, void *arg4);                     *
*                                                                              *
*******************************************************************************/

call_name3:
	.ascii  "calljavafunction2\0\0"

	.align	8
	.long   0                         /* catch type all                       */
	.long   calljava_xhandler3        /* handler pc                           */
	.long   calljava_xhandler3        /* end pc                               */
	.long   asm_calljavafunction2     /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   32                        /* frame size                           */
	.long   0                         /* method pointer (pointer to name)     */

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

call_name4:
	.ascii  "calljavafunction2double\0\0"

	.align	8
	.long   0                         /* catch type all                       */
	.long   calljava_xhandler3        /* handler pc                           */
	.long   calljava_xhandler3        /* end pc                               */
	.long   asm_calljavafunction2double      /* start pc                      */
	.long   1                         /* extable size                         */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   32                        /* frame size                           */
	.long   0                         /* method pointer (pointer to name)     */

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

call_name5:
	.ascii  "calljavafunction2long\0\0"

	.align	8
	.long   0                         /* catch type all                       */
	.long   calljava_xhandler3        /* handler pc                           */
	.long   calljava_xhandler3        /* end pc                               */
	.long   asm_calljavafunction2long /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   32                        /* frame size                           */
	.long   0                         /* method pointer (pointer to name)     */


asm_calljavafunction2:
asm_calljavafunction2double:
asm_calljavafunction2long:
		push    %ebp                  /* save ebp                             */

		mov     %esp,%eax             /* save stackptr                        */
		mov     20(%esp),%ebp

        push    %ebx                  /* save registers                       */
        push    %esi
        push    %edi

   		sub     $32,%esp

        mov     sizejniblock*3+offjniitem+4(%ebp),%ebx
        mov     %ebx,28(%esp)
        mov     sizejniblock*3+offjniitem(%ebp),%ebx
        mov     %ebx,24(%esp)

        mov     sizejniblock*2+offjniitem+4(%ebp),%ebx
        mov     %ebx,20(%esp)
        mov     sizejniblock*2+offjniitem(%ebp),%ebx
        mov     %ebx,16(%esp)


        mov     sizejniblock+offjniitem+4(%ebp),%ebx
        mov     %ebx,12(%esp)
        mov     sizejniblock+offjniitem(%ebp),%ebx
        mov     %ebx,8(%esp)

        mov     offjniitem+4(%ebp),%ebx
        mov     %ebx,4(%esp)
        mov     offjniitem(%ebp),%ebx
        mov     %ebx,0(%esp)

		mov     %eax,%ebp	
		mov     8(%ebp),%eax          /* move function pointer to %eax        */

		lea     asm_call_jit_compiler,%edx 
		call    *%edx                 /* call JIT compiler                    */
	
calljava_jit3:
calljava_return3:
calljava_ret3:
        add     $32,%esp
        pop     %edi                  /* restore registers                    */
        pop     %esi
        pop     %ebx
		leave
		ret

calljava_xhandler3:
		push    %eax				  /* pass exception pointer               */
		call    builtin_throw_exception
		add     $4,%esp
        
		add     $32,%esp
        pop     %edi                  /* restore registers                    */
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
		push	%ebx            /* save register                              */
        push    %ebp
				
		mov     8(%esp),%ebp	/* get return address (2 push)                */
		mov     -1(%ebp),%bl	/* get function code                          */
		cmp     $0xd1,%bl		/* called with `call *REG_ITMP2' (%ecx)?      */
		jne		L_not_static_special

		sub     $6,%ebp			/* calculate address of immediate             */
		jmp		L_call_jit_compile
		
L_not_static_special:
		cmp     $0xd0,%bl		/* called with `call *REG_ITMP1' (%eax)       */
		jne		L_not_virtual_interface
		
		sub     $6,%ebp         /* calculate address of offset                */
		mov     (%ebp),%ebp     /* get offset                                 */
		add     %ecx,%ebp       /* add base address to get method address     */
		jmp		L_call_jit_compile

L_not_virtual_interface:        /* a call from asm_calljavamethod             */
		xor     %ebp,%ebp
		
L_call_jit_compile:
		push    %ebp            /* save address for method pointer            */

		push    %eax			/* push methodpointer on stack                */
		call    jit_compile
		add     $4,%esp

		pop     %ebp            /* restore address for method pointer         */
		test	%ebp,%ebp		/* is address == 0 (asm_calljavamethod)       */
		je		L_call_method
		
		mov     %eax,(%ebp)		/* and now save the new pointer               */

L_call_method:
        pop     %ebp            /* restore registers                          */
		pop     %ebx
        		
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
        xor     %eax,%eax
        mov     %eax,(%eax)

		push	%ebp
        push    %ecx
        push    %edx
		push	%ebx
		push	%esi
		push	%edi
				
		mov     4(%ebp),%eax		/* load function pointer */
		call	*%eax				/* call function */

		pop		%edi
		pop		%esi
		pop		%ebx
        pop     %edx
		pop     %ecx
		pop		%ebp
        
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

asm_handle_builtin_exception:
        add     $4,%esp						/* clear return address of this call */
        mov     (%esp),%eax                 /* get exceptionptr               */
        leave                               /* leave builtin function         */
        mov     (%esp),%edx                 /* get exceptionpc                */
        sub     $2,%edx                     /* size of builtin call           */
        jmp     asm_handle_exception
        
asm_handle_nat_exception:
		add     $4,%esp						/* clear return address of native stub */
		
asm_handle_exception:
		push    %ebp
		mov     %esp,%ebp
        
		push    %eax						/* save exception pointer         */
		push    %ecx                        /* save exception pc              */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
		call    cast_lock
#endif
		
		call    findmethod                  /* get the data segment ptr       */
		mov     %eax,%edx
		        
		mov     -4(%ebp),%eax
		mov     -8(%ebp),%ecx               /* could be changed in findmethod */

		push    %edx						/* save data segment pointer      */
		push    %ebx
		push    %esi
		push    %edi
		
ex_stack_loop:
		sub     $16,%esp
		mov     %eax,(%esp)					/* exception pointer              */
		mov     MethodPointer(%edx),%eax	/* method pointer                 */
		mov     %eax,4(%esp)
		mov     %ecx,8(%esp)				/* exception pc                   */
		movl    $1,12(%esp)					/* set no unwind flag             */
		call	builtin_trace_exception
		add     $16,%esp

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

		mov     offobjvftbl(%eax),%esi		/* %esi = vftblptr(xptr)          */
		mov     offclassvftbl(%ebx),%ebx    /* %ebx = vftblptr(catchtype) class (not obj) */
		mov     offbaseval(%esi),%esi		/* %esi = baseval(xptr)           */
		mov     offbaseval(%ebx),%edx		/* %edx = baseval(catchtype)      */
		mov     offdiffval(%ebx),%ebx		/* %ebx = diffval(catchtype)      */
		sub     %edx,%esi					/* %esi = baseval(xptr) - baseval(catchtype) */
		cmp     %ebx,%esi					/* xptr is instanceof catchtype   */
		ja      ex_table_cont
		
ex_handle_it:
		mov     ExHandlerPC(%edi),%edx
		
		pop     %edi                        /* restore registers              */
		pop     %esi
		pop     %ebx
        add     $8,%esp                     /* suck %ecx, %edx                */
        pop     %eax                        /* restore xptr                   */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
		call cast_unlock
#endif
        
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
		
		jmp		asm_handle_exception
		

/********************* function asm_builtin_monitorenter ***********************
*                                                                              *
*   Does null check and calls monitorenter or throws an exception              *
*                                                                              *
*******************************************************************************/

asm_builtin_monitorenter:
		cmpl	$0,4(%esp)
		je		nb_monitorenter			/* if (null) throw exception          */
		jmp		builtin_monitorenter	/* else call builtin_monitorenter     */

nb_monitorenter:
		popl	%ecx					/* delete return address */
		subl	$2,%ecx					/* faulting address is return adress - 2 */
		movl	proto_java_lang_NullPointerException,%eax
		jmp		asm_handle_exception
		

/********************* function asm_builtin_monitorexit ************************
*                                                                              *
*   Does null check and calls monitorexit or throws an exception               *
*                                                                              *
*******************************************************************************/

asm_builtin_monitorexit:
		mov     4(%esp),%eax
        test    %eax,%eax
		je      nb_monitorexit			/* if (null) throw exception          */
        push    %ecx                    /* save registers which could be used */
        push    %edx
        push    %eax
		call    builtin_monitorexit		/* else call builtin_monitorenter     */
        add     $4,%esp
        pop     %edx                    /* restore registers which could be used */
        pop     %ecx
        ret

nb_monitorexit:
		popl	%ecx					/* delete return address */
		subl	$2,%ecx					/* faulting address is return adress - 2 */
		movl	proto_java_lang_NullPointerException,%eax
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
		popl	%ecx					/* delete return address */
		subl	$2,%ecx					/* faulting address is return adress - 2 */
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
		popl	%ecx					/* delete return address */
		subl	$2,%ecx					/* faulting address is return adress - 2 */
		movl	proto_java_lang_ArithmeticException,%eax
		jmp		asm_handle_exception
		

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
		
		popl	%ecx					/* delete return address              */
		subl	$2,%ecx					/* faulting address is return adress - 2 */
		movl	proto_java_lang_ClassCastException,%eax
		jmp		asm_handle_exception

		
/******************* function asm_builtin_newarray *****************************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/

asm_builtin_newarray:
		subl	$8,%esp					/* build stack frame (2 * 4 bytes) */

		movl	12(%esp),%eax
		movl	%eax,(%esp)

		movl	20(%esp),%eax
		movl	%eax,4(%esp)

		call	builtin_newarray
	
		addl	$8,%esp
		ret

		
/******************* function asm_builtin_aastore ******************************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/

asm_builtin_aastore:
		subl	$12,%esp				/* build stack frame (3 * 4 bytes)    */

		movl	16(%esp),%eax           /* 12 (frame) + 4 (return)            */
		test	%eax,%eax				/* if null pointer throw exception    */
		je		nb_aastore_null

		movl	offarraysize(%eax),%edx /* load size                          */
		movl	24(%esp),%ecx			/* index                              */
		cmpl	%edx,%ecx				/* do bound check                     */
		ja		nb_aastore_bound		/* if out of bounds throw exception   */

		shll	$2,%ecx					/* index * 4                          */
		addl	%eax,%ecx				/* add index * 4 to arrayref          */
		
		movl	%ecx,8(%esp)			/* save store position                */
		
		movl	16(%esp),%eax           /* 12 (frame) + 4 (return)            */
		movl	%eax,(%esp)

		movl	32(%esp),%eax			/* object is second argument          */
		movl	%eax,4(%esp)
		
		call	builtin_canstore		/* builtin_canstore(arrayref,object)  */

		test	%eax,%eax				/* if (false) throw exception         */
		je		nb_aastore_throw

		movl	32(%esp),%eax
		movl	8(%esp),%ecx
		movl	%eax,offobjarrdata(%ecx)/* store objectptr in array           */
		
		addl	$12,%esp
		ret

nb_aastore_null:
		addl	$12,%esp
		popl	%ecx					/* delete return address */
		subl	$2,%ecx					/* faulting address is return adress - 2 */
		movl	proto_java_lang_NullPointerException,%eax
		jmp		asm_handle_exception

nb_aastore_bound:
		addl	$12,%esp
		popl	%ecx					/* delete return address */
		subl	$2,%ecx					/* faulting address is return adress - 2 */
		movl	proto_java_lang_ArrayIndexOutOfBoundsException,%eax
		jmp		asm_handle_exception
		
nb_aastore_throw:
		addl	$12,%esp
		popl	%ecx					/* delete return address */
		subl	$2,%ecx					/* faulting address is return adress - 2 */
		movl	proto_java_lang_ArrayStoreException,%eax
		jmp		asm_handle_exception

		
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

asm_initialize_thread_stack:
		movl	8(%esp),%eax            /* (to)->stackEnd                     */
		subl	$36,%eax				/* 4 bytes * 8 regs + 4 bytes func    */

		xorl	%edx,%edx
		movl	%edx,0(%eax)
		movl	%edx,4(%eax)
		movl	%edx,8(%eax)
		movl	%edx,12(%eax)
		movl	%edx,16(%eax)
		movl	%edx,20(%eax)
		movl	%edx,24(%eax)
		movl	%edx,28(%eax)

		movl	4(%esp),%edx            /* save (u1*) (func)                  */
		movl	%edx,32(%eax)

		ret                             /* return restorepoint in %eax        */


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
		movl	%ecx,4(%esp)
		movl	%edx,8(%esp)
		movl	%ebx,12(%esp)
		movl	%esp,16(%esp)
		movl	%ebp,20(%esp)
		movl	%esi,24(%esp)
		movl	%edi,28(%esp)

		movl	36(%esp),%eax			/* save current return address */
		movl	%eax,32(%esp)

		movl	40(%esp),%eax			/* first argument **from */
		movl	%esp,0(%eax)

		movl	48(%esp),%eax			/* third argument **stackTop */
		movl	%esp,0(%eax)

		movl	44(%esp),%eax			/* second argument **to */
		movl	0(%eax),%esp			/* load new stack pointer */

		movl	0(%esp),%eax
		movl	4(%esp),%ecx
		movl	8(%esp),%edx
		movl	12(%esp),%ebx
										/* skip stack pointer */
		movl	20(%esp),%ebp
		movl	24(%esp),%esi
		movl	28(%esp),%edi

		addl	$32,%esp                /* leave return address on stack      */
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
		movl	4(%esp),%edx			/* first argument *stack              */
		subl	$8,%edx					/* allocate new stack                 */

		movl	(%esp),%eax				/* save return address on new stack   */
		movl	%eax,(%edx)

		movl	%esp,4(%edx)			/* save old stack pointer on new stack*/

		movl	12(%esp),%eax			/* third argument **stacktopsave      */
		movl	%esp,(%eax)				/* save old stack pointer to variable */

		movl	8(%esp),%eax			/* load function pointer              */
		movl	16(%esp),%ecx			/* fourth argument *p                 */
		
		movl	%edx,%esp				/* switch to new stack                */

		subl	$4,%esp
		movl	%ecx,0(%esp)			/* pass pointer                       */
		call	*%eax					/* and call function                  */
		addl	$4,%esp

		movl	(%esp),%edx				/* load return address                */
		movl	4(%esp),%esp			/* switch to old stack                */
		movl	%edx,(%esp)
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
		xorl	%eax,%eax
/*		movl	$0,(%eax) */
		ret


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
