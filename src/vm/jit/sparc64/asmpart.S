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

	.register %g2,#scratch                         /* define as scratch       */
	.register %g3,#scratch                    /* XXX  reserve for application */
	.text

/* export functions ***********************************************************/

	.global asm_vm_call_method        
	.global asm_vm_call_method_int    
	.global asm_vm_call_method_long   
	.global asm_vm_call_method_float  
	.global asm_vm_call_method_double 
	.global asm_vm_call_method_exception_handler

	.global asm_call_jit_compiler

	.global asm_handle_exception
	.global asm_handle_nat_exception


	.global asm_abstractmethoderror
	
	.global asm_criticalsections
	.global asm_getclassvalues_atomic


/* asm_vm_call_method ******************************************************
 *                                                                         *
 * This function calls a Java-method (which possibly needs compilation)    *
 *
 * C-prototype:
 *  java_objectheader *asm_vm_call_method(methodinfo *m, s4 vmargscount,
 *						 vm_arg *vmargs);
 **************************************************************************/

	.align 8	/* v9: All data types are aligned to their size */

	.xword 0                         /* catch type all */
	.xword 0                         /* handler pc */
	.xword 0                         /* end pc */
	.xword 0                         /* start pc */
	.word  1                         /* extable size */
	.word  0                         /* ALIGNMENT PADDING */
	.xword 0                         /* line number table start */
	.xword 0                         /* line number table size */
	.word  0                         /* ALIGNMENT PADDING */
	.word  0                         /* fltsave */
	.word  1                         /* intsave */
	.word  0                         /* isleaf */
	.word  0                         /* IsSync */
	.word  0                         /* frame size */
	.xword 0                         /* method pointer (pointer to name)*/ 

asm_vm_call_method:       
asm_vm_call_method_int:   
asm_vm_call_method_long:  
asm_vm_call_method_float: 
asm_vm_call_method_double:
	
	save %sp, -144, %sp		/* 16 reg-save + 2 */


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

	mov   %i0,itmp1		/* pass method info pointer via itmp1 */
	
	setx  asm_call_jit_compiler,%l0,mptr_itmp2  /* fake virtual function call (2 instr) */
	stx   mptr_itmp2,[%sp + 2047 + 17*8]        /* store function address               */
	add   %sp,2047 + 16*8,mptr_itmp2            /* set method pointer                   */
	
	ldx  [1*8 + mptr_itmp2], pv_caller          /* method call as in Java               */
	jmpl pv_caller,ra_caller                      /* call JIT compiler          */
	nop
calljava_jit2:
	/* pretend to restore pv */
	add  ra_caller,(asm_vm_call_method - calljava_jit2 + 8),pv_callee
	
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
	
	save    %sp,-160,%sp         /* regsave + 4 slots for float args          */

	SAVE_FLOAT_ARGUMENT_REGISTERS(16)

	mov     itmp1,%o0             /* pass methodinfo pointer                  */
	mov     mptr_itmp2,%o1        /* pass method pointer                      */
	mov     %fp,%o2				  /* pass java sp (==fp)             		  */
	mov     ra_callee,%o3         /* pass Java ra                             */
	mov     %o3,%o4               /* xpc is equal to ra                       */
	call    jit_asm_compile       /* call jit compiler                        */
	nop
	
	RESTORE_FLOAT_ARGUMENT_REGISTERS(16)
	
	restore %o0,%g0,pv_caller     /* restore the callers window               */
	                              /* the source o0 references the old window  */
	                              /* pv_caller references the new window      */



	brz     pv_caller,L_asm_call_jit_compiler_exception

	/* synchronise instruction cache moved somewhere else           */

	jmpl    pv_caller,zero        /* and call method, the method returns      */
	                              /* directly to the caller (ra).             */	
	
L_asm_call_jit_compiler_exception:
	/* no need to do a save, only ra needs to be preserved */

	/* we save ra in one of the application globals */
	mov     ra_caller,xpc_itmp3   /* save return address (xpc)                */
	
	call    exceptions_get_and_clear_exception

	mov     xpc_itmp3,ra_caller   /* restore return address (xpc)             */

	mov     %o0,xptr_itmp2        /* get exception                            */
	sub     ra_caller,4,xpc_itmp3 /* exception address is ra - 4              */
	ba      L_asm_handle_nat_exception



/* asm_handle_exception ********************************************************

   This function handles an exception. It does not use the usual calling
   conventions. The exception pointer is passed in REG_ITMP2 and the
   pc from the exception raising position is passed in REG_ITMP3. It searches
   the local exception table for a handler. If no one is found, it unwinds
   stacks and continues searching the callers.

*******************************************************************************/


asm_handle_nat_exception:
L_asm_handle_nat_exception:       /* required for PIC code                    */
asm_handle_exception:
	
	/* nothing here */

	restore zero,0,zero


/* asm_abstractmethoderror *****************************************************

   Creates and throws an AbstractMethodError.

*******************************************************************************/

asm_abstractmethoderror:
	/* do a window save */
	save %sp,-192,%sp

	mov     %fp,%o0                     /* pass java sp(==fp)                 */
	mov     ra_callee,%o1               /* pass exception address             */
	call    exceptions_asm_new_abstractmethoderror

	mov     %o0,xptr_itmp2              /* get exception pointer              */
	sub     ra_callee,4,xpc_itmp3       /* exception address is ra - 4        */
	ba      L_asm_handle_nat_exception

	/* XXX: leave the register window open for handle_exception ??? */

asm_getclassvalues_atomic:
_crit_restart:
_crit_begin:
/* not doing a window save, using the global temporary registers */
	ldsw    [offbaseval+%o0],itmp1
	ldsw    [offdiffval+%o0],itmp2
	ldsw    [offbaseval+%o1],itmp3
_crit_end:
	stw     itmp1,[offcast_super_baseval+%o2]
	stw     itmp2,[offcast_super_diffval+%o2]
	stw     itmp3,[offcast_sub_baseval+%o2]
	jmpl    ra_caller,zero  /* caller's ra, b/c no window save */

	.end    asm_getclassvalues_atomic


    .data

asm_criticalsections:
#if defined(ENABLE_THREADS)
    .xword   _crit_begin
    .xword   _crit_end
    .xword   _crit_restart
#endif
    .xword   0



