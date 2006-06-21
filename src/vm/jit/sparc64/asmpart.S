/* src/vm/jit/sparc64/asmpart.S - Java-C interface functions for Sparc

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
            Alexander Jordan

   Changes: 

   $Id: asmpart.S 4749 2006-04-11 10:20:18Z twisti $

*/


#include "config.h"
#include "offsets.h"
#include "md-asm.h"

	.text

/* export functions ***********************************************************/

	.global asm_vm_call_method        
	.global asm_vm_call_method_int    
	.global asm_vm_call_method_long   
	.global asm_vm_call_method_float  
	.global asm_vm_call_method_double 
	.global asm_vm_call_method_exception_handler


/* asm_vm_call_method ******************************************************
 *                                                                         *
 * This function calls a Java-method (which possibly needs compilation)    *
 *
 * C-prototype:
 *  java_objectheader *asm_vm_call_method(methodinfo *m, s4 vmargscount,
 *						 vm_arg *vmargs);
 **************************************************************************/

	.align 8	/* v9: All data types are aligned to their size */

/*x*/	.xword 0                         /* catch type all */
/*x*/	.word calljava_xhandler2        /* handler pc */
/*x*/	.word calljava_xhandler2        /* end pc */
/*x*/	.word asm_vm_call_method        /* start pc */
	.word  1                         /* extable size */
	.word  0                         /* ALIGNMENT PADDING */
/*x*/	.xword 0                         /* line number table start */
/*x*/	.xword 0                         /* line number table size */
	.word  0                         /* ALIGNMENT PADDING */
	.word  0                         /* fltsave */
	.word  1                         /* intsave */
	.word  0                         /* isleaf */
	.word  0                         /* IsSync */
	.word  0                         /* frame size */
/*x*/	.xword 0                         /* method pointer (pointer to name)*/ 

asm_vm_call_method:       
asm_vm_call_method_int:   
asm_vm_call_method_long:  
asm_vm_call_method_float: 
asm_vm_call_method_double:
	
	save %sp, -128, %sp		/* minimal (reg-window) frame */


	/* todo: copy fp registers */

	brlez %i1, calljava_argsloaded
	dec %i1
	ldx [%i2 + offvmargdata], %o0
	brlez %i1, calljava_argsloaded

	dec %i1
	ldx [%i2 + (offvmargdata+sizevmarg*1)], %o1
	brlez %i1, calljava_argsloaded

	dec %i1
	ldx [%i2 + (offvmargdata+sizevmarg*2)], %o2
	brlez %i1, calljava_argsloaded

	dec %i1
	ldx [%i2 + (offvmargdata+sizevmarg*3)], %o3
	brlez %i1, calljava_argsloaded

	dec %i1
	ldx [%i2 + (offvmargdata+sizevmarg*4)], %o4

	/* todo: use more out registers ? */

calljava_argsloaded:
	/* todo: stack frame layout!! */
	
	brlez %i1, calljava_nocopy
	sllx %i1, 3, %l0	/* remaining args * 8           */
	mov %sp, %l1		/* right above window save area */
	sub %sp, %l0, %sp	/* allocate more stack space    */
	
calljava_copyloop:
	ldx [%i2 + (offvmargdata+sizevmarg*5)], %l0
	stx %l0, [%l1]
	inc sizevmarg, %i2 	/* src++                        */
	inc 8, %l1		/* dst++                        */
	dec %i1			/* arg_count--			*/
	bnz %xcc, calljava_copyloop

calljava_nocopy:

	mov		%i0,itmp1		/* pass method pointer via itmp1 */
	
	addx    %sp,1*8,mptr_itmp2    /* set method pointer      ???          */

	/* setx asm_call_jit_compiler, %l1, %l0 */
	call 	asm_call_jit_compiler	/* call JIT compiler                         */
	nop
calljava_jit2:
	/* no need to restore pv */
	
calljava_return:
	mov %o0, %i0		/* pass on the return value	*/
	return %i7 + 8		/* implicit window restore */
	nop


asm_vm_call_method_exception_handler:
	mov		itmp1,%o0
	call	builtin_throw_exception
	return 	%i7 + 8         		 /* implicit window restore */
	nop
	
	
	
	

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
*   M_LDQ (REG_PV, REG_PV, a)        ; invokestatic/special    ($27)           *
*   M_LDA (REG_PV, REG_RA, low)                                                *
*   M_LDAH(REG_PV, REG_RA, high)     ; optional                                *
*   or                                                                         *
*   M_LDQ (REG_PV, REG_METHODPTR, m) ; invokevirtual/interface ($28)           *
*   in the static case the method pointer can be computed using the            *
*   return address and the lda function following the jmp instruction          *
*                                                                              *
*******************************************************************************/

asm_call_jit_compiler:
	
	/* XXX save + reserve stack space          */
	/* XXX save float arg registers */

	mov     mptr_itmp2,%l0        /* save method pointer                      */
	stq     itmp1,14*8(sp)        /* save methodinfo pointer                  */

	addx    15*8,%sp,%o0          /* create stackframe info                   */
	mov     zero,%o1              /* we don't have pv handy                   */
	mov     %fp,%o2				  /* pass java sp (==fp)             		  */
	mov     %ra,%o3               /* pass Java ra                             */
	mov     %o3,%o4               /* xpc is equal to ra                       */
	call    stacktrace_create_extern_stackframeinfo

	mov     itmp1,%o0,            /* pass methodinfo pointer                  */
	call    jit_compile           /* call jit compiler                        */
	mov     %o0,pv_callee         /* save return value into callee pv         */
	                              /* the in's we have are also the in's of the*/
	                              /* method that will be called               */

	mov     %ra,%o0               /* pass return address                      */
	addx    15*8,sp,%o1           /* pass stackframeinfo (for PV)             */
	mov     %l0,%o2               /* pass method pointer                      */
	jsr     ra,md_assembler_get_patch_address /* get address of patch position*/
	mov     %o0,%l2               /* store patch address for later use        */

	addx    15*8,sp,%o0           /* remove stackframe info                   */
	call    stacktrace_remove_stackframeinfo
	
	
	mov 	%l2, itmp3            /* move patch address out of window         */
	/* XXX do a window restore here ???  */
	/* XXX restore float argument registers */

	beq     pv_callee,L_asm_call_jit_compiler_exception

	stx     pv_callee,[itmp3]     /* patch method entry point                 */

	/* XXX synchronise instruction cache            */

	jmpl    pv,zero               /* and call method, the method returns      */
	                              /* directly to the caller (ra).             */	
	
L_asm_call_jit_compiler_exception:
	/* XXX how to handle stack & window here? */
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	/* itmpx shall be one of the application globals */
	mov     %ra,itmpx             /* save return address (xpc)                */
	
	call    builtin_asm_get_exceptionptrptr
#else
	setx    v0,_exceptionptr
#endif
	ldq     xptr,0(v0)            /* get the exception pointer                */
	stq     zero,0(v0)            /* clear the exception pointer              */

	subq    ra,4,xpc
	br      L_asm_handle_nat_exception







