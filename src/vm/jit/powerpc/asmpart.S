/* src/vm/jit/powerpc/asmpart.S - Java-C interface functions for PowerPC
		
   Copyright (C) 1996-2013
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

   This file is part of CACAO.

   This program is free software.text;  you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation;  either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY	;  without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program;  if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/


#include "config.h"

#include "md-abi.hpp"
#include "md-asm.hpp"

#include "vm/jit/abi-asm.hpp"
#include "vm/jit/methodheader.hpp"


	.text

	.align 2


/* export functions ***********************************************************/

	.globl asm_vm_call_method
	.globl asm_vm_call_method_int
	.globl asm_vm_call_method_long
	.globl asm_vm_call_method_float
	.globl asm_vm_call_method_double

	.globl asm_vm_call_method_exception_handler
	.globl asm_vm_call_method_end

	.globl asm_cacheflush


/* asm_vm_call_method **********************************************************
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

	.align 2

	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* IsLeaf                               */
	.long   0                         /* frame size                           */
	.long   0                         /* codeinfo pointer                     */

asm_vm_call_method:
asm_vm_call_method_int:
asm_vm_call_method_long:
asm_vm_call_method_float:
asm_vm_call_method_double:
	mflr    r0
	stw     r0,LA_LR_OFFSET(sp)
	stwu    sp,-40*4(sp)              /* keep stack 16-byte aligned           */

	stw     s0,8*4(sp)                /* save used callee saved registers     */

#if defined(__DARWIN__)
	stw     itmp1,10*4(sp)            /* register r11 is callee saved         */
#endif
	stw     pv,11*4(sp)               /* save PV register                     */

	stw     itmp3,12*4(sp)            /* registers r14-r31 are callee saved   */
	stfd	ftmp1,14*4(sp)            /* registers f14-f31 are callee saved   */
	stfd	ftmp2,16*4(sp)

#if defined(__DARWIN__)
	stw     t1,18*4(sp)
	stw     t2,19*4(sp)
	stw     t3,20*4(sp)
	stw     t4,21*4(sp)
	stw     t5,22*4(sp)
	stw     t6,23*4(sp)
	stw     t7,24*4(sp)

	stfd	ft0,26*4(sp)
	stfd	ft1,28*4(sp)
	stfd	ft2,30*4(sp)
	stfd	ft3,32*4(sp)
	stfd	ft4,34*4(sp)
	stfd	ft5,36*4(sp)
#else
	SAVE_TEMPORARY_REGISTERS(18)        /* the offset has to be even          */
#endif

	mr      pv,a0                       /* move PV into PV register           */
	mtctr   pv                          /* move PV into branch register       */

	mr      t0,a1                       /* address of data structure          */
	mr      t1,a2                       /* stack argument count               */

	mr      s0,sp                       /* save SP                            */

	lwz     a0,0*8+4(t0)                /* we are on big-endian               */
	lwz     a1,1*8+4(t0)
	lwz     a2,2*8+4(t0)
	lwz     a3,3*8+4(t0)
	lwz     a4,4*8+4(t0)
	lwz     a5,5*8+4(t0)
	lwz     a6,6*8+4(t0)
	lwz     a7,7*8+4(t0)

	lfd     fa0,8*8(t0)
	lfd     fa1,9*8(t0)
	lfd     fa2,10*8(t0)
	lfd     fa3,11*8(t0)
	lfd     fa4,12*8(t0)
	lfd     fa5,13*8(t0)
	lfd     fa6,14*8(t0)
	lfd     fa7,15*8(t0)

#if defined(__DARWIN__)
	lfd     fa8,16*8(t0)
	lfd     fa9,17*8(t0)
	lfd     fa10,18*8(t0)
	lfd     fa11,19*8(t0)
	lfd     fa12,20*8(t0)
#endif

	mr.     t1,t1
	beq     L_asm_vm_call_method_stack_copy_done

	slwi    t2,t1,3                     /* calculate stackframe size (* 8)    */

	sub     sp,sp,t2                    /* create stackframe                  */
	mr      t2,sp                       /* temporary stack pointer            */

L_asm_vm_call_method_stack_copy_loop:
#if defined(__DARWIN__)
	lwz     t3,21*8+0(t0)               /* load argument                      */
	lwz     t4,21*8+4(t0)
#else
	lwz     t3,16*8+0(t0)               /* load argument                      */
	lwz     t4,16*8+4(t0)
#endif
	stw     t3,0(t2)                    /* store argument on stack            */
	stw     t4,4(t2)

	addi    t0,t0,8                     /* load address of next argument      */
	addi    t2,t2,8                     /* increase stack pointer             */
	addi    t1,t1,-1                    /* subtract 1 argument                */
	mr.     t1,t1
	bgt     L_asm_vm_call_method_stack_copy_loop

L_asm_vm_call_method_stack_copy_done:
	bctrl
1:
	mflr    itmp1
#if defined(__DARWIN__)
	addi    pv,itmp1,lo16(asm_vm_call_method - 1b)
#else
	addi    pv,itmp1,(asm_vm_call_method - 1b)@l
#endif

L_asm_vm_call_method_return:
	mr      sp,s0                       /* restore the SP                     */

	lwz     s0,8*4(sp)                  /* restore used callee saved registers*/

#if defined(__DARWIN__)
	lwz     itmp1,10*4(sp)              /* register r11 is callee saved       */
#endif
	lwz     pv,11*4(sp)                 /* save PV register                   */

	lwz     itmp3,12*4(sp)
	lfd     ftmp1,14*4(sp)              /* registers f14-f31 are callee saved */
	lfd     ftmp2,16*4(sp)

#if defined(__DARWIN__)
	lwz     t1,18*4(sp)
	lwz     t2,19*4(sp)
	lwz     t3,20*4(sp)
	lwz     t4,21*4(sp)
	lwz     t5,22*4(sp)
	lwz     t6,23*4(sp)
	lwz     t7,24*4(sp)

	lfd     ft0,26*4(sp)
	lfd     ft1,28*4(sp)
	lfd     ft2,30*4(sp)
	lfd     ft3,32*4(sp)
	lfd     ft4,34*4(sp)
	lfd     ft5,36*4(sp)
#else
	RESTORE_TEMPORARY_REGISTERS(18)   /* the offset has to be even            */
#endif

	lwz     r0,40*4+LA_LR_OFFSET(sp)
	mtlr    r0
	addi    sp,sp,40*4
	blr

asm_vm_call_method_exception_handler:
	mr      a0,itmp1
	bl      builtin_throw_exception
	b       L_asm_vm_call_method_return

asm_vm_call_method_end:
	nop


/*********************************************************************/

asm_cacheflush:
	add     a1,a0,a1
	rlwinm  a0,a0,0,0,26
	addi    a1,a1,31
	rlwinm  a1,a1,0,0,26
	mr      a2,a0
1:
	cmplw   a0,a1
	bge     0f
	dcbst   0,a0
	addi    a0,a0,32
	b       1b
0:
	sync
1:
	cmplw   a2,a1
	bge     0f
	icbi    0,a2
	addi    a2,a2,32
	b       1b
0:
	sync
	isync
	blr


#if defined(__DARWIN__)

.section __TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
	.align 2
L_builtin_throw_exception$stub:
	.indirect_symbol _builtin_throw_exception
	mflr r0
	bcl 20,31,L00$_builtin_throw_exception
L00$_builtin_throw_exception:
	mflr r11
	addis r11,r11,ha16(L_builtin_throw_exception$lazy_ptr - L00$_builtin_throw_exception)
	mtlr r0
	lwzu r12,lo16(L_builtin_throw_exception$lazy_ptr - L00$_builtin_throw_exception)(r11)
	mtctr r12
	bctr
.data
.lazy_symbol_pointer
L_builtin_throw_exception$lazy_ptr:
	.indirect_symbol _builtin_throw_exception
	.long dyld_stub_binding_helper


.section __TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
	.align 2
L_md_asm_codegen_get_pv_from_pc$stub:
	.indirect_symbol _md_asm_codegen_get_pv_from_pc
	mflr r0
	bcl 20,31,L00$_md_asm_codegen_get_pv_from_pc
L00$_md_asm_codegen_get_pv_from_pc:
	mflr r11
	addis r11,r11,ha16(L_md_asm_codegen_get_pv_from_pc$lazy_ptr - L00$_md_asm_codegen_get_pv_from_pc)
	mtlr r0
	lwzu r12,lo16(L_md_asm_codegen_get_pv_from_pc$lazy_ptr - L00$_md_asm_codegen_get_pv_from_pc)(r11)
	mtctr r12
	bctr
.data
.lazy_symbol_pointer
L_md_asm_codegen_get_pv_from_pc$lazy_ptr:
	.indirect_symbol _md_asm_codegen_get_pv_from_pc
	.long dyld_stub_binding_helper


.section __TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
	.align 2
L_exceptions_handle_exception$stub:
	.indirect_symbol _exceptions_handle_exception
	mflr r0
	bcl 20,31,L00$_exceptions_handle_exception
L00$_exceptions_handle_exception:
	mflr r11
	addis r11,r11,ha16(L_exceptions_handle_exception$lazy_ptr - L00$_exceptions_handle_exception)
	mtlr r0
	lwzu r12,lo16(L_exceptions_handle_exception$lazy_ptr - L00$_exceptions_handle_exception)(r11)
	mtctr r12
	bctr
.data
.lazy_symbol_pointer
L_exceptions_handle_exception$lazy_ptr:
	.indirect_symbol _exceptions_handle_exception
	.long dyld_stub_binding_helper


.section __TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
	.align 2
L_exceptions_asm_new_abstractmethoderror$stub:
	.indirect_symbol _exceptions_asm_new_abstractmethoderror
	mflr r0
	bcl 20,31,L00$_exceptions_asm_new_abstractmethoderror
L00$_exceptions_asm_new_abstractmethoderror:
	mflr r11
	addis r11,r11,ha16(L_exceptions_asm_new_abstractmethoderror$lazy_ptr - L00$_exceptions_asm_new_abstractmethoderror)
	mtlr r0
	lwzu r12,lo16(L_exceptions_asm_new_abstractmethoderror$lazy_ptr - L00$_exceptions_asm_new_abstractmethoderror)(r11)
	mtctr r12
	bctr
.data
.lazy_symbol_pointer
L_exceptions_asm_new_abstractmethoderror$lazy_ptr:
	.indirect_symbol _exceptions_asm_new_abstractmethoderror
	.long dyld_stub_binding_helper

#endif /* defined(__DARWIN__) */


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
