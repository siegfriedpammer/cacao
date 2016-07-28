/* src/vm/jit/mips/asmpart.S - Java-C interface functions for MIPS

   Copyright (C) 1996-2005, 2006, 2007, 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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

*/


#include "config.h"

#include "vm/jit/mips/md-abi.hpp"
#include "vm/jit/mips/md-asm.hpp"

#include "vm/jit/abi-asm.hpp"
#include "vm/jit/methodheader.hpp"


	.text
	.set    noat


/* export functions ***********************************************************/

	.globl asm_vm_call_method
	.globl asm_vm_call_method_int
	.globl asm_vm_call_method_long
	.globl asm_vm_call_method_float
	.globl asm_vm_call_method_double
	.globl asm_vm_call_method_exception_handler
	.globl asm_vm_call_method_end

/* asm_vm_call_method **********************************************************
*                                                                              *
*   This function calls a Java-method (which possibly needs compilation)       *
*   with up to 4 address parameters.                                           *
*                                                                              *
*   This functions calls the JIT-compiler which eventually translates the      *
*   method into machine code.                                                  *
*                                                                              *
*   A possibly throwed exception will be returned to the caller as function    *
*   return value, so the java method cannot return a fucntion value (this      *
*   function usually calls 'main' and '<clinit>' which do not return a         *
*   function value).                                                           *
*                                                                              *
*   C-prototype:                                                               *
*    javaobject_header *asm_calljavafunction (methodinfo *m,                   *
*         void *arg1, void *arg2, void *arg3, void *arg4);                     *
*                                                                              *
*******************************************************************************/

	.ent    asm_vm_call_method

	.align  3

#if SIZEOF_VOID_P == 8

	.word   0                           /* fltsave                            */
	.word   0                           /* intsave                            */
	.word   0                           /* isleaf                             */
	.word   0                           /* frame size                         */
	.dword  0                           /* codeinfo pointer                   */

#else /* SIZEOF_VOID_P == 8 */

	.word   0                           /* fltsave                            */
	.word   0                           /* intsave                            */
	.word   0                           /* isleaf                             */
	.word   0                           /* frame size                         */
	.word   0                           /* method pointer (pointer to name)   */

#endif /* SIZEOF_VOID_P == 8 */

asm_vm_call_method:
asm_vm_call_method_int:
asm_vm_call_method_long:
asm_vm_call_method_float:
asm_vm_call_method_double:
	.set    noreorder                 /* XXX we need to recompute pv          */

	aaddiu  sp,sp,-12*8               /* allocate stack space (only 11 needed)*/
	ast     ra,0*8(sp)                /* save return address                  */

	bal     L_asm_vm_call_method_compute_pv
	ast     pv,1*8(sp)                /* procedure vector                     */
L_asm_vm_call_method_compute_pv:
	aaddiu  pv,ra,-4*4

	ast     s0,3*8(sp)                /* save callee saved register           */
	ast     a0,4*8(sp)                /* save method PV                       */

#if SIZEOF_VOID_P == 8
	s.d     fss0,5*8(sp)              /* save non JavaABI saved flt registers */
	s.d     fss1,6*8(sp)
	s.d     fss2,7*8(sp)
	s.d     fss3,8*8(sp)
	s.d     fss4,9*8(sp)
	s.d     fss5,10*8(sp)
#endif

	move    t0,a1                     /* address of data structure            */
	move    t1,a2                     /* stack argument count                 */
	move    s0,sp                     /* save stack pointer                   */

#if SIZEOF_VOID_P == 8

	ld      a0,0*8(t0)
	ld      a1,1*8(t0)
	ld      a2,2*8(t0)
	ld      a3,3*8(t0)
	ld      a4,4*8(t0)
	ld      a5,5*8(t0)
	ld      a6,6*8(t0)
	ld      a7,7*8(t0)

	l.d     fa0,8*8(t0)
	l.d     fa1,9*8(t0)
	l.d     fa2,10*8(t0)
	l.d     fa3,11*8(t0)
	l.d     fa4,12*8(t0)
	l.d     fa5,13*8(t0)
	l.d     fa6,14*8(t0)
	l.d     fa7,15*8(t0)

#else /* SIZEOF_VOID_P == 8 */

# if WORDS_BIGENDIAN == 1
	lw      a0,0*8+4(t0)
	lw      a1,1*8+4(t0)
	lw      a2,2*8+4(t0)
	lw      a3,3*8+4(t0)
# else
	lw      a0,0*8(t0)
	lw      a1,1*8(t0)
	lw      a2,2*8(t0)
	lw      a3,3*8(t0)
# endif

# if !defined(ENABLE_SOFT_FLOAT)
	l.d     fa0,4*8(t0)
	l.d     fa1,5*8(t0)
# endif

#endif /* SIZEOF_VOID_P == 8 */

	beqz    t1,L_asm_vm_call_method_stack_copy_done
	nop

	sll     t2,t1,3                   /* calculate stackframe size (* 8)      */
	asubu   sp,sp,t2                  /* create stackframe                    */
	move    t2,sp                     /* temporary stack pointer              */

L_asm_vm_call_method_stack_copy_loop:
#if SIZEOF_VOID_P == 8
	ld      t3,16*8(t0)               /* load argument                        */
	sd      t3,0(t2)                  /* store argument on stack              */
#else
# if !defined(ENABLE_SOFT_FLOAT)
	lw      t3,6*8+0(t0)              /* load argument                        */
	lw      t4,6*8+4(t0)
	sw      t3,0(t2)                  /* store argument on stack              */
	sw      t4,4(t2)
# else
#  error implement me
# endif
#endif

	aaddi   t1,t1,-1                  /* subtract 1 argument                  */
	aaddi   t0,t0,8                   /* load address of next argument        */
	aaddi   t2,t2,8                   /* increase stack pointer               */

	bgtz    t1,L_asm_vm_call_method_stack_copy_loop
	nop

L_asm_vm_call_method_stack_copy_done:
	ala     mptr,4*8(s0)              /* get address of PV                    */
	ald     pv,0*8(mptr)              /* load PV                              */
	jalr    pv
	nop
L_asm_vm_call_method_recompute_pv:
#if SIZEOF_VOID_P == 8
	aaddiu  pv,ra,-76*4               /* recompute procedure vector           */
#else
	aaddiu  pv,ra,(asm_vm_call_method - L_asm_vm_call_method_recompute_pv)
#endif

	.set    reorder                   /* XXX we need to recompute pv          */

calljava_return2:
	move    sp,s0                     /* restore stack pointer                */

	ald     ra,0*8(sp)                /* restore return address               */
	ald     pv,1*8(sp)                /* restore procedure vector             */
	ald     s0,3*8(sp)

#if SIZEOF_VOID_P == 8
	l.d     fss0,5*8(sp)              /* restore non JavaABI saved flt regs   */
	l.d     fss1,6*8(sp)
	l.d     fss2,7*8(sp)
	l.d     fss3,8*8(sp)
	l.d     fss4,9*8(sp)
	l.d     fss5,10*8(sp)
#endif

	aaddiu  sp,sp,12*8                /* free stack space                     */
	j       ra                        /* return                               */

asm_vm_call_method_exception_handler:
	move    sp,s0                     /* restore stack pointer                */
#if SIZEOF_VOID_P == 4
	aaddiu  sp,sp,-4*4                /* reserve space for 1 argument         */
#endif

	move    a0,itmp1                  
	jal     builtin_throw_exception
#if SIZEOF_VOID_P == 4
	aaddiu  sp,sp,4*4
#endif
asm_vm_call_method_end:
	b       calljava_return2

	.end    asm_vm_call_method


/* disable exec-stacks ********************************************************/

#if defined(__linux__) && defined(__ELF__)
	.section .note.GNU-stack,"",%progbits
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
 * vim:noexpandtab:sw=4:ts=4:
 */
