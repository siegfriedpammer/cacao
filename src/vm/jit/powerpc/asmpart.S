/* src/vm/jit/powerpc/asmpart.S - Java-C interface functions for PowerPC
		
   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Andreas Krall
            Reinhard Grafl
            Stefan Ring

   Changes: Christian Thalinger

   $Id: asmpart.S 2176 2005-04-01 13:00:34Z twisti $

*/


#if defined(__LINUX__)
# include <asm/ppc_asm.h>
#endif

#include "config.h"
#include "vm/jit/powerpc/offsets.h"
#include "vm/jit/powerpc/asmoffsets.h"


#define itmp1 r11
#define itmp2 r12
#define itmp3 r0

#define xptr itmp1
#define xpc itmp2

#define pv r13
#define mptr r12
#define mptrn 12


#if defined(__DARWIN__)

/* Defines for darwin's old gnu assembler *************************************/

/* internal defines ***********************************************************/

#define asm_calljavafunction                  _asm_calljavafunction
#define asm_calljavafunction_int              _asm_calljavafunction_int

#define asm_calljavafunction2                 _asm_calljavafunction2
#define asm_calljavafunction2int              _asm_calljavafunction2int
#define asm_calljavafunction2long             _asm_calljavafunction2long
#define asm_calljavafunction2float            _asm_calljavafunction2float
#define asm_calljavafunction2double           _asm_calljavafunction2double

#define asm_call_jit_compiler                 _asm_call_jit_compiler

#define asm_handle_nat_exception              _asm_handle_nat_exception
#define asm_handle_exception                  _asm_handle_exception
#define asm_handle_nullptr_exception          _asm_handle_nullptr_exception

#define asm_check_clinit                      _asm_check_clinit

#define asm_builtin_checkarraycast            _asm_builtin_checkarraycast
#define asm_builtin_aastore                   _asm_builtin_aastore

#define asm_builtin_monitorenter              _asm_builtin_monitorenter
#define asm_builtin_monitorexit               _asm_builtin_monitorexit

#define asm_builtin_idiv                      _asm_builtin_idiv
#define asm_builtin_irem                      _asm_builtin_irem
#define asm_builtin_ldiv                      _asm_builtin_ldiv
#define asm_builtin_lrem                      _asm_builtin_lrem

#define asm_cacheflush                        _asm_cacheflush
#define asm_initialize_thread_stack           _asm_initialize_thread_stack
#define asm_perform_threadswitch              _asm_perform_threadswitch
#define asm_switchstackandcall                _asm_switchstackandcall
#define asm_criticalsections                  _asm_criticalsections
#define asm_getclassvalues_atomic             _asm_getclassvalues_atomic


/* external defines ***********************************************************/

#define builtin_asm_get_exceptionptrptr       _builtin_asm_get_exceptionptrptr
#define builtin_canstore                      _builtin_canstore
#define builtin_checkarraycast                _builtin_checkarraycast
#define builtin_ldiv                          _builtin_ldiv
#define builtin_lrem                          _builtin_lrem

#if defined(USE_THREADS)
#define builtin_monitorenter                  _builtin_monitorenter
#define builtin_monitorexit                   _builtin_monitorexit
#endif

#define builtin_throw_exception               _builtin_throw_exception
#define builtin_trace_exception               _builtin_trace_exception
#define class_init                            _class_init
#define link_class                            _link_class
#define load_class_bootstrap                  _load_class_bootstrap
#define jit_compile                           _jit_compile
#define new_arithmeticexception               _new_arithmeticexception
#define new_arrayindexoutofboundsexception    _new_arrayindexoutofboundsexception
#define new_arraystoreexception               _new_arraystoreexception
#define new_classcastexception                _new_classcastexception
#define new_nullpointerexception              _new_nullpointerexception

#if !defined(USE_THREADS) || !defined(NATIVE_THREADS)
#if 0
#define _exceptionptr                         __exceptionptr
#endif

_exceptionptr:
	.globl		__exceptionptr
#endif

#endif /* defined(__DARWIN__) */

#if defined(__DARWIN__)

#define fr0     f0
#define fr1     f1
#define fr2     f2
#define fr3     f3
#define fr4     f4
#define fr5     f5
#define fr6     f6
#define fr7     f7
#define fr8     f8
#define fr9     f9
#define fr10    f10
#define fr11    f11
#define fr12    f12
#define fr13    f13
#define fr14    f14
#define fr15    f15
#define fr16    f16
#define fr17    f17
#define fr18    f18
#define fr19    f19
#define fr20    f20
#define fr21    f21
#define fr22    f22
#define fr23    f23
#define fr24    f24
#define fr25    f25
#define fr26    f26
#define fr27    f27
#define fr28    f28
#define fr29    f29
#define fr30    f30
#define fr31    f31

#endif /* defined(__DARWIN__) */


	.text

	.align 2

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
	.globl asm_handle_nullptr_exception

	.globl asm_check_clinit

	.globl asm_builtin_checkarraycast
	.globl asm_builtin_aastore

#if defined(USE_THREADS)
	.globl asm_builtin_monitorenter
	.globl asm_builtin_monitorexit
#endif

	.globl asm_builtin_idiv
	.globl asm_builtin_irem
	.globl asm_builtin_ldiv
	.globl asm_builtin_lrem

	.globl asm_cacheflush
	.globl asm_initialize_thread_stack
	.globl asm_perform_threadswitch
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
*   C-prototype:		                                                               *
*    javaobject_header *asm_calljavamethod (methodinfo *m,                     *
*         void *arg1, void *arg2, void *arg3, void *arg4);                      *
*                                                                              *
*******************************************************************************/
		
	.align 2

    .long   0                         /* catch type all                       */
	.long   calljava_xhandler         /* handler pc                           */
	.long   calljava_xhandler         /* end pc                               */
	.long   asm_calljavafunction      /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   24                        /* frame size                           */
	.long   0                         /* method pointer (pointer to name)     */
	.long   0                         /* padding                              */

asm_calljavafunction:
asm_calljavafunction_int:
	mflr    r0
	stw     r31,-4(r1)
/*	stw     r30,-8(r1)*/
	stw     pv,-12(r1)
	stw     r0,8(r1)
	stwu    r1,-148(r1)

	bl      0f
0:
	mflr    r31

	stw		r16,40(r1)
	stw		r17,44(r1)
	stw		r18,48(r1)
	stw		r19,52(r1)
	stw		r20,56(r1)
	stw		r21,60(r1)
	stw		r22,64(r1)
	stw		r23,68(r1)
	stfd	fr16,72(r1)
	stfd	fr17,80(r1)
	stfd	fr18,88(r1)
	stfd	fr19,96(r1)
	stfd	fr20,104(r1)
	stfd	fr21,112(r1)
	stfd	fr22,120(r1)
	stfd	fr23,128(r1)

	stw     r3,36(r1)
	addi    itmp1,r1,36
	mr      r3,r4
	mr      r4,r5
	mr      r5,r6
	mr      r6,r7

#if defined(__DARWIN__)
/*	addis   mptr,r31,ha16(_asm_call_jit_compiler-0b)*/
	addi    mptr,r31,lo16(asm_call_jit_compiler-0b)
#else
	addi    mptr,r31,(asm_call_jit_compiler-0b)@l
#endif
	stw     mptr,32(r1)
	addi    mptr,r1,28

	lwz     pv,4(mptr)
	mtctr   pv
	bctrl

1:
	mflr    itmp1
#if defined(__DARWIN__)
	addi    pv,itmp1,lo16(asm_calljavafunction-1b)
#else
	addi    pv,itmp1,(asm_calljavafunction-1b)@l
#endif

calljava_regrestore:
	lwz     r16,40(r1)
	lwz		r17,44(r1)
	lwz		r18,48(r1)
	lwz		r19,52(r1)
	lwz		r20,56(r1)
	lwz		r21,60(r1)
	lwz		r22,64(r1)
	lwz		r23,68(r1)
	lfd		fr16,72(r1)
	lfd		fr17,80(r1)
	lfd		fr18,88(r1)
	lfd		fr19,96(r1)
	lfd		fr20,104(r1)
	lfd		fr21,112(r1)
	lfd		fr22,120(r1)
	lfd		fr23,128(r1)

	lwz     r0,148+8(r1)
	mtlr    r0
	addi    r1,r1,148
	lwz     pv,-12(r1)
/*	lwz     r30,-8(r1)*/
	lwz     r31,-4(r1)
	blr

calljava_xhandler:
	mr      r3,itmp1
	bl      builtin_throw_exception
	b       calljava_regrestore




	.align 2

	.long   0                         /* catch type all                       */
	.long   calljava_xhandler2        /* handler pc                           */
	.long   calljava_xhandler2        /* end pc                               */
	.long   asm_calljavafunction2     /* start pc                             */
	.long   1                         /* extable size                         */
	.long   0                         /* fltsave                              */
	.long   0                         /* intsave                              */
	.long   0                         /* isleaf                               */
	.long   0                         /* IsSync                               */
	.long   24                        /* frame size                           */
	.long   0                         /* method pointer (pointer to name)     */
	.long   0                         /* padding                              */

asm_calljavafunction2:
asm_calljavafunction2int:
asm_calljavafunction2long:
asm_calljavafunction2float:
asm_calljavafunction2double:
	mflr    r0
	stw     r31,-4(r1)
/*	stw     r30,-8(r1)*/
	stw     pv,-12(r1)
	stw     r0,8(r1)
	stwu    r1,-148(r1)
	bl      0f
0:
	mflr    r31

	stw     r16,40(r1)
	stw	    r17,44(r1)
	stw     r18,48(r1)
	stw	    r19,52(r1)
	stw     r20,56(r1)
	stw     r21,60(r1)
	stw     r22,64(r1)
	stw     r23,68(r1)
	stfd	fr16,72(r1)
	stfd	fr17,80(r1)
	stfd	fr18,88(r1)
	stfd	fr19,96(r1)
	stfd	fr20,104(r1)
	stfd	fr21,112(r1)
	stfd	fr22,120(r1)
	stfd	fr23,128(r1)

	stw     r3,36(r1)                 /* save method pointer for compiler     */
	mr      itmp1,r6                  /* pointer to arg block                 */
	mr      itmp2,r4                  /* arg count                            */

	mr.     itmp2,itmp2
	ble     calljava_argsloaded

	addi    itmp2,itmp2,-1
	lwz     r3,offjniitem+4(itmp1)
	mr.     itmp2,itmp2
	ble     calljava_argsloaded

	addi    itmp2,itmp2,-1
	lwz     r4,offjniitem+sizejniblock*1+4(itmp1)
	mr.     itmp2,itmp2
	ble     calljava_argsloaded
    addi    itmp2,itmp2,-1

    addi    itmp2,itmp2,-1
    lwz     r5,offjniitem+sizejniblock*2+4(itmp1)
    mr.     itmp2,itmp2
    ble     calljava_argsloaded
    addi    itmp2,itmp2,-1

    addi    itmp2,itmp2,-1
    lwz     r6,offjniitem+sizejniblock*3+4(itmp1)
    mr.     itmp2,itmp2
    ble     calljava_argsloaded
    addi    itmp2,itmp2,-1

calljava_argsloaded:
	addi    itmp1,r1,36
#if defined(__DARWIN__)
/*	addis   mptr,r31,ha16(_asm_call_jit_compiler-0b)*/
	addi    mptr,r31,lo16(asm_call_jit_compiler-0b)
#else
	addi    mptr,r31,(asm_call_jit_compiler-0b)@l
#endif
	stw     mptr,32(r1)
	addi    mptr,r1,28

	lwz     pv,4(mptr)
	mtctr   pv
	bctrl
1:
	mflr    itmp1
#if defined(__DARWIN__)
	addi    pv,itmp1,lo16(asm_calljavafunction2-1b)
#else
	addi    pv,itmp1,(asm_calljavafunction2-1b)@l
#endif

calljava_regrestore2:
	lwz		r16,40(r1)
	lwz		r17,44(r1)
	lwz		r18,48(r1)
	lwz		r19,52(r1)
	lwz		r20,56(r1)
	lwz		r21,60(r1)
	lwz		r22,64(r1)
	lwz		r23,68(r1)
	lfd		fr16,72(r1)
	lfd		fr17,80(r1)
	lfd		fr18,88(r1)
	lfd		fr19,96(r1)
	lfd		fr20,104(r1)
	lfd		fr21,112(r1)
	lfd		fr22,120(r1)
	lfd		fr23,128(r1)

	lwz     r0,148+8(r1)
	mtlr    r0
	addi    r1,r1,148
	lwz     pv,-12(r1)
/*	lwz     r30,-8(r1)*/
	lwz     r31,-4(r1)
	blr

calljava_xhandler2:
	mr      r3,itmp1
	bl      builtin_throw_exception
	b       calljava_regrestore2


asm_call_jit_compiler:
	stw     itmp1,-8(r1)
	mflr    itmp1
	stw     r31,-4(r1)
	stw     r29,-12(r1)
	stw     itmp1,8(r1)
	stwu    r1,-176(r1)
	mr      r31,pv

	lwz     itmp3,-12(itmp1)
	srwi    itmp3,itmp3,16
	andi.   itmp3,itmp3,31
	cmpwi   itmp3,mptrn
	beq     noregchange
	lwz     itmp3,4(itmp1)
	extsh   itmp3,itmp3
	add     mptr,itmp3,itmp1
	lwz     itmp3,8(itmp1)
	srwi    itmp3,itmp3,16
	cmpwi   itmp3,0x3dad
	bne     noregchange
	lwz     itmp3,8(itmp1)
	slwi    itmp3,itmp3,16
	add     mptr,mptr,itmp3
		
noregchange:
	lwz     itmp1,168(r1)
	mr      r29,mptr
	stw     r3,28(r1)
	stw     r4,32(r1)
	stw     r5,36(r1)
	stw     r6,40(r1)
	stw     r7,44(r1)
	stw     r8,48(r1)
	stw     r9,52(r1)
	stfd    fr1,56(r1)
	stfd    fr2,64(r1)
	stfd    fr3,72(r1)
	stfd    fr4,80(r1)
	stfd    fr5,88(r1)
	stfd    fr6,96(r1)
	stfd    fr7,104(r1)
	stfd    fr8,112(r1)
	stfd    fr9,120(r1)
	stfd    fr10,128(r1)
	stfd    fr11,136(r1)
	stfd    fr12,144(r1)
	stfd    fr13,152(r1)
	stw     r10,160(r1)

	lwz     r3,0(itmp1)
	bl      jit_compile
	mr      pv,r3

	mr      mptr,r29
	lwz     r3,28(r1)
	lwz	  	r4,32(r1)
	lwz	  	r5,36(r1)
	lwz	  	r6,40(r1)
	lwz	  	r7,44(r1)
	lwz	  	r8,48(r1)
	lwz	  	r9,52(r1)
	lfd	  	fr1,56(r1)
	lfd	  	fr2,64(r1)
	lfd	  	fr3,72(r1)
	lfd	  	fr4,80(r1)
	lfd	  	fr5,88(r1)
	lfd	  	fr6,96(r1)
	lfd	  	fr7,104(r1)
	lfd	  	fr8,112(r1)
	lfd	  	fr9,120(r1)
	lfd	  	fr10,128(r1)
	lfd	  	fr11,136(r1)
	lfd	  	fr12,144(r1)
	lfd	  	fr13,152(r1)
	lwz	  	r10,160(r1)

	lwz     itmp1,176+8(r1)
	lwz     itmp3,-12(itmp1)
	extsh   itmp3,itmp3
	add     mptr,mptr,itmp3
	stw     pv,0(mptr)

	mtctr   pv

	lwz     r0,176+8(r1)
	mtlr    r0
	addi    r1,r1,176
	lwz     r29,-12(r1)
/*	lwz     pv,-8(r1)*/
	lwz     r31,-4(r1)
	bctr


/***************** function asm_handle_nullptr_exception ***********************
*                                                                              *
*   This behaves the same as asm_handle_exception but calls                    *
*   new_nullpointerexception first                                             *
*                                                                              *
*******************************************************************************/
		
asm_handle_nullptr_exception:
	stwu	r1,-32(r1)
	stw		xpc,24(r1)
	bl		new_nullpointerexception
	lwz		xpc,24(r1)
	addi	r1,r1,32
	mr      xptr,r3
	b		asm_handle_exception

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
	mflr    r9
	lwz     itmp3,4(r9)
	extsh   itmp3,itmp3
	add     pv,itmp3,r9
	lwz     itmp3,8(r9)
	srwi    itmp3,itmp3,16
	cmpwi   itmp3,0x3dad
	bne     asm_handle_exception
	lwz     itmp3,8(r9)
	slwi    itmp3,itmp3,16
	add     pv,pv,itmp3

asm_handle_exception:
	addi    r1,r1,-18*4
	stw     r0,0*4(r1)
	stw     r2,1*4(r1)
	stw     r3,2*4(r1)
	stw     r4,3*4(r1)
	stw     r5,4*4(r1)
	stw     r6,5*4(r1)
	stw     r7,6*4(r1)
	stw     r8,7*4(r1)
	stw     r9,8*4(r1)
	stw     r10,9*4(r1)
	stw     r16,10*4(r1)
	stw     r17,11*4(r1)
	stw     r18,12*4(r1)
	stw     r19,13*4(r1)
	stw     r20,14*4(r1)
	stw     r21,15*4(r1)
	stw     r22,16*4(r1)
	stw     r23,17*4(r1)

	li      r9,1
ex_stack_loop:
	addi    r1,r1,-4*4
	stw     xptr,0*4(r1)
	stw     xpc,1*4(r1)
	mflr    xptr
	stw     xptr,2*4(r1)
	stw     r9,3*4(r1)

	lwz     r3,0*4(r1)            /* exception pointer                        */
	lwz     r4,MethodPointer(pv)  /* method pointer                           */
	mr      r5,xpc                /* exception pc                             */
/*	mr      r6,r9 */
	li      r6,0                  /* line number                              */
	li      r7,4                  /* set no unwind flag                       */

	/* XXX no valid stack frame chaining here */
	addi    r1,r1,-(24+5*4)       /* 24 linkage area + 5 argument * 4         */
	bl      builtin_trace_exception
	addi    r1,r1,(24+5*4)

	lwz     xptr,2*4(r1)
	mtlr    xptr
	lwz     xptr,0*4(r1)          /* restore xptr                             */
	lwz     xpc,1*4(r1)
	lwz     r9,3*4(r1)
	addi    r1,r1,4*4

	lwz     r3,ExTableSize(pv)    /* r3 = exception table size                */
	mr.     r3,r3                 /* if empty table skip                      */
	beq     empty_table

	addi    r4,pv,ExTableStart    /* r4 = start of exception table            */

ex_table_loop:
	lwz     r5,ExStartPC(r4)      /* r5 = exception start pc                  */
	cmplw   r5,xpc                /* (startpc <= xpc)                         */
	bgt     ex_table_cont
	lwz     r5,ExEndPC(r4)        /* r5 = exception end pc                    */
	cmplw   xpc,r5                /* (xpc < endpc)                            */
	bge     ex_table_cont
	lwz     r7,ExCatchType(r4)    /* r7 = exception catch type                */
	mr.     r7,r7
	beq     ex_handle_it

	lwz     itmp3,offclassloaded(r7)
	mr.     itmp3,itmp3
	bne     L_class_loaded

	/* XXX no valid stack frame chaining here */
	addi    r1,r1,-16*4           /* allocate stack                           */
	stw     r3,7*4(r1)            /* save used registers                      */
	stw     r4,8*4(r1)            /* 6*4 (linkage) + 1*4 (arg1) + 7*4 (save)  */
	stw     r9,9*4(r1)
	stw     xptr,10*4(r1)
	stw     xpc,11*4(r1)
	mflr    xptr
	stw     xptr,12*4(r1)
	stw     r7,13*4(r1)

	mr      r3,r7                 /* arg1 = exceptionclass                    */
	bl      load_class_bootstrap

	lwz     r3,7*4(r1)
	lwz     r4,8*4(r1)
	lwz     r9,9*4(r1)
	lwz     xptr,10*4(r1)
	lwz     xpc,11*4(r1)
	lwz     itmp3,12*4(r1)
	mtlr    itmp3
	lwz     r7,13*4(r1)	
	addi    r1,r1,16*4

L_class_loaded:
	lwz     itmp3,offclasslinked(r7)
	mr.     itmp3,itmp3
	/* XXX no valid stack frame chaining here */
	addi    r1,r1,-16*4           /* allocate stack                           */
	stw     r7,13*4(r1)
	bne     L_class_linked

	stw     r3,7*4(r1)            /* save used registers                      */
	stw     r4,8*4(r1)            /* 6*4 (linkage) + 1*4 (arg1) + 7*4 (save)  */
	stw     r9,9*4(r1)
	stw     xptr,10*4(r1)
	stw     xpc,11*4(r1)
	mflr    xptr
	stw     xptr,12*4(r1)

	mr      r3,r7                 /* arg1 = exceptionclass                    */
	bl      link_class

	lwz     r3,7*4(r1)
	lwz     r4,8*4(r1)
	lwz     r9,9*4(r1)
	lwz     xptr,10*4(r1)
	lwz     xpc,11*4(r1)
	lwz     itmp3,12*4(r1)
	mtlr    itmp3

L_class_linked:
_crit_restart1:
	lwz     r7,13*4(r1)	
_crit_begin1:
	lwz     r6,offobjvftbl(xptr)  /* r6 = vftblptr(xptr)                      */
	lwz     r7,offclassvftbl(r7)  /* r7 = vftblptr(catchtype) class (not obj) */
	lwz     r6,offbaseval(r6)     /* r6 = baseval(xptr)                       */
	lwz     r8,offbaseval(r7)     /* r8 = baseval(catchtype)                  */
	lwz     r7,offdiffval(r7)     /* r7 = diffval(catchtype)                  */
_crit_end1:
	subf    r6,r8,r6              /* r6 = baseval(xptr) - baseval(catchtype)  */
	cmplw   r6,r7                 /* xptr is instanceof catchtype             */
	addi    r1,r1,16*4
	bgt     ex_table_cont         /* if (false) continue                      */

ex_handle_it:
	lwz     xpc,ExHandlerPC(r4)   /* xpc = exception handler pc               */
	mr.     r9,r9
	beq     ex_jump

	lwz     r0,0*4(r1)
	lwz     r2,1*4(r1)
	lwz     r3,2*4(r1)
	lwz     r4,3*4(r1)
	lwz     r5,4*4(r1)
	lwz     r6,5*4(r1)
	lwz     r7,6*4(r1)
	lwz     r8,7*4(r1)
	lwz     r9,8*4(r1)
	lwz     r10,9*4(r1)
	lwz     r16,10*4(r1)
	lwz     r17,11*4(r1)
	lwz     r18,12*4(r1)
	lwz     r19,13*4(r1)
	lwz     r20,14*4(r1)
	lwz     r21,15*4(r1)
	lwz     r22,16*4(r1)
	lwz     r23,17*4(r1)
	addi    r1,r1,18*4

ex_jump:
	mtctr   xpc
	bctr

ex_table_cont:
	addi    r4,r4,ExEntrySize
	addic.  r3,r3,-1
	bgt     ex_table_loop

empty_table:
	mr.     r9,r9                 /* if here the first time, then             */
	beq     ex_already_cleared
	addi    r1,r1,18*4            /* deallocate stack and                     */
	li      r9,0                  /* clear the no unwind flag                 */
ex_already_cleared:
	lwz     r3,IsSync(pv)
	mr.     r3,r3
	beq     no_monitor_exit

#if defined(USE_THREADS)
	add     r3,r1,r3
	lwz     r6,-4(r3)

	addi    r1,r1,-6*4
	stw     r3,0*4(r1)
	stw     r4,1*4(r1)
	stw     r9,2*4(r1)
	stw     xptr,3*4(r1)
	stw     xpc,4*4(r1)
	mflr    xptr
	stw     xptr,5*4(r1)

	mr      r3,r6
	/* XXX no valid stack frame chaining here */
	addi    r1,r1,-40
	bl      builtin_monitorexit
	addi    r1,r1,40

	lwz     xptr,5*4(r1)
	mtlr    xptr
	lwz     r3,0*4(r1)
	lwz     r4,1*4(r1)
	lwz     r9,2*4(r1)
	lwz     xptr,3*4(r1)
	lwz     xpc,4*4(r1)
	addi    r1,r1,6*4
#endif

no_monitor_exit:
	lwz     r3,FrameSize(pv)      /* r3 = frame size                          */
	add     r1,r1,r3              /* unwind stack                             */
	mr      r3,r1                 /* r3 = pointer to save area                */
	lwz     r4,IsLeaf(pv)         /* r4 = is leaf procedure                   */
	mr.     r4,r4
	bne     ex_no_restore         /* if (leaf) skip                           */
	lwz     r4,8(r3)              /* restore ra                               */
	mtlr    r4                    /* t0--                                     */
ex_no_restore:
	mflr    r4                    /* the new xpc is ra                        */
	mr      xpc,r4
	lwz     r4,IntSave(pv)        /* r4 = saved int register count            */
	bl      ex_int1
ex_int1:
	mflr    r5
#if defined(__DARWIN__)
	addi    r5,r5,lo16(ex_int2-ex_int1)
#else
	addi    r5,r5,(ex_int2-ex_int1)@l
#endif
	slwi    r4,r4,2
	subf    r5,r4,r5
	mtctr   r5
	bctr
	lwz     r14,-40(r3)
	lwz     r15,-36(r3)
	lwz     r24,-32(r3)
	lwz     r25,-28(r3)
	lwz     r26,-24(r3)
	lwz     r27,-20(r3)
	lwz     r28,-16(r3)
	lwz     r29,-12(r3)
	lwz     r30,-8(r3)
	lwz     r31,-4(r3)
ex_int2:
	subf    r3,r4,r3

	lwz     r4,FltSave(pv)
	bl      ex_flt1
ex_flt1:
	mflr    r5
#if defined(__DARWIN__)
	addi    r5,r5,lo16(ex_flt2-ex_flt1)
#else
	addi    r5,r5,(ex_flt2-ex_flt1)@l
#endif
	slwi    r4,r4,2
	subf    r5,r4,r5
	mtctr   r5
	bctr
	lfd     fr14,-80(r3)
	lfd     fr15,-72(r3)
	lfd     fr24,-64(r3)
	lfd     fr25,-56(r3)
	lfd     fr26,-48(r3)
	lfd     fr27,-40(r3)
	lfd     fr28,-32(r3)
	lfd     fr29,-24(r3)
	lfd     fr30,-16(r3)
	lfd     fr31,-8(r3)
ex_flt2:
	mtlr    xpc
	lwz     itmp3,4(xpc)
	extsh   itmp3,itmp3
	add     pv,itmp3,xpc
	lwz     itmp3,8(xpc)
	srwi    itmp3,itmp3,16
	cmpwi   itmp3,0x3dad
	bne     ex_stack_loop
	lwz     itmp3,8(xpc)
	slwi    itmp3,itmp3,16
	add     pv,pv,itmp3
	b       ex_stack_loop


/********************* asm_check_clinit ****************************************
*                                                                              *
*   call static class initializer for PUT/GETSTATIC instructions               *
*                                                                              *
*******************************************************************************/

asm_check_clinit:
	lwz     itmp2,offclassinit(itmp1)
	mr.     itmp2,itmp2
	bne     L_is_initialized

	mflr	r0
	stw     r0,8(r1)
	stwu    r1,-(26*8)(r1)        /* keep stack 16-bytes aligned              */

    stw     r3,4*8(r1)            /* save argument registers                  */
    stw     r4,5*8(r1)            /* preserve linkage area (24 bytes)         */
    stw     r5,6*8(r1)            /* and 4 bytes (better 8) for 1 argument    */
    stw     r6,7*8(r1)
    stw     r7,8*8(r1)
    stw     r8,9*8(r1)
    stw     r9,10*8(r1)
	stw     r10,11*8(r1)

	stfd    fr1,12*8(r1)
    stfd    fr2,13*8(r1)
    stfd    fr3,14*8(r1)
    stfd    fr4,15*8(r1)
    stfd    fr5,16*8(r1)
    stfd    fr6,17*8(r1)
    stfd    fr7,18*8(r1)
    stfd    fr8,19*8(r1)
    stfd    fr9,20*8(r1)
    stfd    fr10,21*8(r1)
    stfd    fr11,22*8(r1)
    stfd    fr12,23*8(r1)
    stfd    fr13,24*8(r1)
		
	mr      r3,itmp1
	bl      class_init
	mr      itmp1,r3              /* save return value in temp register       */

	lwz     r3,4*8(r1)
	lwz     r4,5*8(r1)
	lwz     r5,6*8(r1)
	lwz     r6,7*8(r1)
	lwz     r7,8*8(r1)
	lwz     r8,9*8(r1)
	lwz     r9,10*8(r1)
	lwz     r10,11*8(r1)

    lfd     fr1,12*8(r1)
    lfd     fr2,13*8(r1)
    lfd     fr3,14*8(r1)
    lfd   	fr4,15*8(r1)
    lfd   	fr5,16*8(r1)
    lfd   	fr6,17*8(r1)
    lfd   	fr7,18*8(r1)
    lfd   	fr8,19*8(r1)
    lfd   	fr9,20*8(r1)
    lfd   	fr10,21*8(r1)
    lfd   	fr11,22*8(r1)
    lfd   	fr12,23*8(r1)
    lfd   	fr13,24*8(r1)
		
	lwz     r0,(26*8)+8(r1)
	mtlr    r0
	addi    r1,r1,(26*8)

	mr.	    itmp1,itmp1           /* check for an exception                   */
	beq     L_initializererror

L_is_initialized:
	blr

L_initializererror:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	mflr	r0
	stw     r0,8(r1)
	stwu    r1,-4*8(r1)           /* preserve linkage area (24 bytes)         */
	bl      builtin_asm_get_exceptionptrptr
	lwz     r0,(4*8)+8(r1)
	mtlr    r0	
	addi    r1,r1,4*8
#else
# if defined(__DARWIN__)
	lwz     r3,lo16(_exceptionptr-0b)(pv)
# else
	lis     r3,_exceptionptr@ha
	addi    r3,r3,_exceptionptr@l
# endif
#endif

	lwz     xptr,0(r3)            /* get the exception pointer                */
	li      r0,0
	stw     r0,0(r3)              /* clear the exception pointer              */

	mflr    xpc
	b       asm_handle_nat_exception


/******************* function asm_builtin_checkarraycast ***********************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/
		
asm_builtin_checkarraycast:
	mflr    r0
	stw     r0,8(r1)
	stwu    r1,-48(r1)

	stw     r3,32(r1)
	bl      builtin_checkarraycast

	lwz     r0,48+8(r1)
	mtlr    r0
	mr.     r3,r3
	beq     nb_carray_throw
	lwz     r3,32(r1)
	addi    r1,r1,48
	blr

nb_carray_throw:
	addi    r1,r1,48
	mflr    r0
	stw     r0,8(r1)
	stwu    r1,-24(r1)
	bl      new_classcastexception
	mr      xptr,r3
	addi    r1,r1,24
	lwz     r0,8(r1)
	mr      xpc,r0
	mtlr    r0
	b       asm_handle_nat_exception


/******************* function asm_builtin_aastore ******************************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/
	
asm_builtin_aastore:
	mr.     r3,r3
	beq     nb_aastore_null
	mflr    r0
	stw     r0,8(r1)
	stwu    r1,-48(r1)

	lwz     itmp1,offarraysize(r3)
	slwi    itmp3,r4,2
	add     itmp2,r3,itmp3
	cmplw   r4,itmp1
	bge     nb_aastore_bound
	mr      r4,r5
	stw     itmp2,32(r1)
	stw     r4,36(r1)
	bl      builtin_canstore

	lwz     r0,48+8(r1)
	mtlr    r0
	lwz     itmp1,32(r1)
	lwz     itmp2,36(r1)
	addi    r1,r1,48
	mr.     r3,r3
	beq     nb_aastore_store
	stw     itmp2,offobjarrdata(itmp1)
	blr

nb_aastore_null:
	mflr    r0
	stw     r0,8(r1)
	stwu    r1,-24(r1)
	bl      new_nullpointerexception
	mr      xptr,r3
	addi    r1,r1,24
	lwz     r0,8(r1)
	mr      xpc,r0
	mtlr    r0
	b       asm_handle_nat_exception

nb_aastore_bound:
	addi    r1,r1,48
	mflr    r0
	stw     r0,8(r1)
	stwu    r1,-(24+1*4)(r1)
	mr      r3,r4                 /* move index into a0                       */
	bl      new_arrayindexoutofboundsexception
	mr      xptr,r3
	addi    r1,r1,(24+1*4)
	lwz     r0,8(r1)
	mr      xpc,r0
	mtlr    r0
	b       asm_handle_nat_exception

nb_aastore_store:
	mflr    r0
	stw     r0,8(r1)
	stwu    r1,-24(r1)
	bl      new_arraystoreexception
	mr      xptr,r3
	addi    r1,r1,24
	lwz     r0,8(r1)
	mr      xpc,r0
	mtlr    r0
	b       asm_handle_nat_exception


#if defined(USE_THREADS)
asm_builtin_monitorenter:
	mr.     r3,r3
	beq     nb_monitorenter
	b       builtin_monitorenter

nb_monitorenter:
	mflr    r0
	stw     r0,8(r1)
	stwu    r1,-24(r1)
	bl      new_nullpointerexception
	mr      xptr,r3
	addi    r1,r1,24
	lwz     r0,8(r1)
/*	addi    xpc,r0,-4 */
	mr      xpc,r0
	addi    xpc,xpc,-4
	mtlr    r0
	b       asm_handle_nat_exception


asm_builtin_monitorexit:
	mr.     r3,r3
	beq     nb_monitorexit
	b       builtin_monitorexit

nb_monitorexit:
	mflr    r0
	stw     r0,8(r1)
	stwu    r1,-24(r1)
	bl      new_nullpointerexception
	mr      xptr,r3
	addi    r1,r1,24
	lwz     r0,8(r1)
/*	addi    xpc,r0,-4 */
	mr      xpc,r0
	addi    xpc,xpc,-4
	mtlr    r0
	b       asm_handle_nat_exception
#endif

	
asm_builtin_idiv:
	mr.     r4,r4
	beq     nb_idiv
	lis     itmp3,0x8000
	cmpw    r3,itmp3
	bne     normal_idiv
	cmpwi   r4,-1
	bne     normal_idiv
	blr

normal_idiv:
	divw    r3,r3,r4
	blr

nb_idiv:
	mflr    r0
	stw     r0,8(r1)
	stwu    r1,-24(r1)
	bl      new_arithmeticexception
	mr      xptr,r3
	addi    r1,r1,24
	lwz     r0,8(r1)
	mr      xpc,r0
	mtlr    r0
	b       asm_handle_nat_exception


asm_builtin_irem:
	mr      itmp2,r3
	mr.     r4,r4
	beq     nb_irem
	lis     itmp3,0x8000
	cmpw    itmp2,itmp3
	bne     normal_irem
	cmpwi   r4,-1
	bne     normal_irem
	li      r3,0
	beqlr

normal_irem:
	divw    itmp3,itmp2,r4
	mullw   itmp3,itmp3,r4
	subf    r3,itmp3,itmp2
	blr

nb_irem:
	mflr    r0
	stw     r0,8(r1)
	stwu    r1,-24(r1)
	bl      new_arithmeticexception
	mr      xptr,r3
	addi    r1,r1,24
	lwz     r0,8(r1)
	mr      xpc,r0
	mtlr    r0
	b       asm_handle_nat_exception


asm_builtin_ldiv:
	or.     r0,r5,r6
	beq     nb_ldiv
	b       builtin_ldiv

nb_ldiv:
	mflr    r0
	stw     r0,8(r1)
	stwu    r1,-24(r1)
	bl      new_arithmeticexception
	mr      xptr,r3
	addi    r1,r1,24
	lwz     r0,8(r1)
	mr      xpc,r0
	mtlr    r0
	b       asm_handle_nat_exception


asm_builtin_lrem:
	or.     r0,r5,r6
	beq     nb_lrem
	b       builtin_lrem

nb_lrem:
	mflr    r0
	stw     r0,8(r1)
	stwu    r1,-24(r1)
	bl      new_arithmeticexception
	mr      xptr,r3
	addi    r1,r1,24
	lwz     r0,8(r1)
	mr      xpc,r0
	mtlr    r0
	b       asm_handle_nat_exception


asm_cacheflush:
	add     r4,r3,r4
	rlwinm  r3,r3,0,0,26
	addi    r4,r4,31
	rlwinm  r4,r4,0,0,26
	mr      r5,r3
1:
	cmplw   r3,r4
	bge     0f
	dcbst   0,r3
	addi    r3,r3,32
	b       1b
0:
	sync
1:
	cmplw   r5,r4
	bge     0f
	icbi    0,r5
	addi    r5,r5,32
	b       1b
0:
	sync
	isync
	blr


	.align 3
doublezero:
	.double 0.0

asm_initialize_thread_stack:
	addi r4,r4,-256
	stw r3,120(r4)
	li r3,0
	stw r3,124(r4)
	stw r3,0(r4)
	stw r3,4(r4)
	stw r3,8(r4)
	stw r3,12(r4)
	stw r3,16(r4)
	stw r3,20(r4)
	stw r3,24(r4)
	stw r3,28(r4)
	stw r3,32(r4)
	stw r3,36(r4)

	stw r3,128(r4)
	stw r3,132(r4)
	stw r3,136(r4)
	stw r3,140(r4)
	stw r3,144(r4)
	stw r3,148(r4)
	stw r3,152(r4)
	stw r3,156(r4)

	mflr r0
	bl 0f
0:
	mflr r3
	mtlr r0
#if defined(__DARWIN__)
	lfd fr0,lo16(doublezero-0b)(r3)
#else
	lfd fr0,(doublezero-0b)@l(r3)
#endif

	stfd fr0,40(r4)
	stfd fr0,48(r4)
	stfd fr0,56(r4)
	stfd fr0,64(r4)
	stfd fr0,72(r4)
	stfd fr0,80(r4)
	stfd fr0,88(r4)
	stfd fr0,96(r4)
	stfd fr0,104(r4)
	stfd fr0,112(r4)

	stfd fr0,160(r4)
	stfd fr0,168(r4)
	stfd fr0,176(r4)
	stfd fr0,184(r4)
	stfd fr0,192(r4)
	stfd fr0,200(r4)
	stfd fr0,208(r4)
	stfd fr0,216(r4)

	mr r3,r4
	blr


asm_perform_threadswitch:
	mflr r0
	addi r1,r1,-224
	stw r0,120(r1)
	stw pv,124(r1)
	stw r14,0(r1)
	stw r15,4(r1)
	stw r24,8(r1)
	stw r25,12(r1)
	stw r26,16(r1)
	stw r27,20(r1)
	stw r28,24(r1)
	stw r29,28(r1)
	stw r30,32(r1)
	stw r31,36(r1)
	stfd fr14,40(r1)
	stfd fr15,48(r1)
	stfd fr24,56(r1)
	stfd fr25,64(r1)
	stfd fr26,72(r1)
	stfd fr27,80(r1)
	stfd fr28,88(r1)
	stfd fr29,96(r1)
	stfd fr30,104(r1)
	stfd fr31,112(r1)

	stw r16,128(r1)
	stw r17,132(r1)
	stw r18,136(r1)
	stw r19,140(r1)
	stw r20,144(r1)
	stw r21,148(r1)
	stw r22,152(r1)
	stw r23,156(r1)
	stfd fr16,160(r1)
	stfd fr17,168(r1)
	stfd fr18,176(r1)
	stfd fr19,184(r1)
	stfd fr20,192(r1)
	stfd fr21,200(r1)
	stfd fr22,208(r1)
	stfd fr23,216(r1)

	stw r1,0(r3)
	stw r1,0(r5)
	lwz r1,0(r4)

	lwz r0,120(r1)
	lwz pv,124(r1)
	lwz r14,0(r1)
	lwz r15,4(r1)
	lwz r24,8(r1)
	lwz r25,12(r1)
	lwz r26,16(r1)
	lwz r27,20(r1)
	lwz r28,24(r1)
	lwz r29,28(r1)
	lwz r30,32(r1)
	lwz r31,36(r1)
	lfd fr14,40(r1)
	lfd fr15,48(r1)
	lfd fr24,56(r1)
	lfd fr25,64(r1)
	lfd fr26,72(r1)
	lfd fr27,80(r1)
	lfd fr28,88(r1)
	lfd fr29,96(r1)
	lfd fr30,104(r1)
	lfd fr31,112(r1)

	lwz r16,128(r1)
	lwz r17,132(r1)
	lwz r18,136(r1)
	lwz r19,140(r1)
	lwz r20,144(r1)
	lwz r21,148(r1)
	lwz r22,152(r1)
	lwz r23,156(r1)
	lfd fr16,160(r1)
	lfd fr17,168(r1)
	lfd fr18,176(r1)
	lfd fr19,184(r1)
	lfd fr20,192(r1)
	lfd fr21,200(r1)
	lfd fr22,208(r1)
	lfd fr23,216(r1)

	mtlr r0
	addi r1,r1,224
	blr


asm_switchstackandcall:
	mflr r0
	stwu r3,-48(r3)
	stw r0,40(r3)
	stw r1,44(r3)
	stw r1,0(r5)
	mr r1,r3

	mtctr r4
	mr r3,r6
	bctrl

	lwz r0,40(r1)
	mtlr r0
	lwz r1,44(r1)
	blr


asm_getclassvalues_atomic:
_crit_restart2:
_crit_begin2:
	lwz     r6,offbaseval(r3)
	lwz     r7,offdiffval(r3)
	lwz     r8,offbaseval(r4)
_crit_end2:
	stw     r6,offcast_super_baseval(r5)
	stw     r7,offcast_super_diffval(r5)
	stw     r8,offcast_sub_baseval(r5)
	blr

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
