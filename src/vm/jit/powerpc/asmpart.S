/* jit/powerpc/asmpart.S - Java-C interface functions for powerpc
		
   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   Institut f. Computersprachen, TU Wien
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser, M. Probst,
   S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich,
   J. Wenninger

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

   $Id: asmpart.S 1073 2004-05-19 23:21:15Z stefan $

*/

#include "config.h"
#include "offsets.h"


#define     MethodPointer   -8
#define     FrameSize       -12
#define     IsSync          -16
#define     IsLeaf          -20
#define     IntSave         -24
#define     FltSave         -28
#define     ExTableSize     -32
#define     ExTableStart    -32

#define     ExEntrySize     -16
#define     ExStartPC       -4
#define     ExEndPC         -8
#define     ExHandlerPC     -12
#define     ExCatchType     -16

#define itmp1 r11
#define itmp2 r12
#define itmp3 r0

#define xptr itmp1
#define xpc itmp2

#define pv r13
#define mptr r12
#define mptrn 12


	.text

	.align 2

	.globl _asm_docalljavamethod
	.globl _asm_calljavafunction2
	.globl _asm_calljavafunction2long
	.globl _asm_calljavafunction2double
	.globl _asm_call_jit_compiler

	.globl _jit_compile
	.globl _asm_handle_nat_exception
	.globl _asm_handle_exception
	.globl _asm_check_clinit
	.globl _asm_builtin_checkarraycast
	.globl _asm_builtin_aastore
	.globl _builtin_canstore
	.globl _builtin_trace_exception
	.globl _builtin_monitorenter
	.globl _builtin_monitorexit
	.globl _builtin_ldiv
	.globl _builtin_lrem
	.globl _builtin_checkarraycast
	.globl _asm_builtin_monitorenter
	.globl _asm_builtin_monitorexit
	.globl _asm_builtin_idiv
	.globl _asm_builtin_irem
	.globl _asm_builtin_ldiv
	.globl _asm_builtin_lrem
	.globl _asm_cacheflush
	.globl _asm_initialize_thread_stack
	.globl _asm_perform_threadswitch
	.globl _asm_switchstackandcall
	.globl _asm_criticalsections
	.globl _asm_getclassvalues_atomic

	.globl _string_java_lang_NullPointerException
	.globl _string_java_lang_ArrayIndexOutOfBoundsException
	.globl _string_java_lang_ArrayStoreException
	.globl _string_java_lang_ArithmeticException
	.globl _string_java_lang_ArithmeticException_message
	.globl _string_java_lang_ClassCastException

	.globl _builtin_asm_get_exceptionptrptr
#if !defined(USE_THREADS) || !defined(NATIVE_THREADS)
	.globl __exceptionptr
#endif

	.globl _builtin_throw_exception
	.globl _class_load
	.globl _class_link
	.globl _class_init
	.globl _new_exception
	.globl _new_exception_message
	.globl _new_exception_int

jitcompile:
	.long _jit_compile

class_load:
	.long _class_load
class_link:
	.long _class_link
class_init:
	.long _class_init
		
builtinthrow:
	.long _builtin_throw_exception

builtin_traceexception:
	.long _builtin_trace_exception
builtin_monitorenter:
	.long _builtin_monitorenter
builtin_monitorexit:
	.long _builtin_monitorexit
builtin_ldiv:
	.long _builtin_ldiv
builtin_lrem:
	.long _builtin_lrem

builtincanstore:
	.long _builtin_canstore
builtincheckarraycast:
	.long _builtin_checkarraycast
	
new_exception:
	.long _new_exception
new_exception_message:
	.long _new_exception_message
new_exception_int:
	.long _new_exception_int

string_java_lang_NullPointerException:
	.long _string_java_lang_NullPointerException
string_java_lang_ArrayIndexOutOfBoundsException:
	.long _string_java_lang_ArrayIndexOutOfBoundsException
string_java_lang_ArrayStoreException:
	.long _string_java_lang_ArrayStoreException
string_java_lang_ArithmeticException:
	.long _string_java_lang_ArithmeticException
string_java_lang_ArithmeticException_message:
	.long _string_java_lang_ArithmeticException_message
string_java_lang_ClassCastException:
	.long _string_java_lang_ClassCastException

builtin_asm_get_exceptionptrptr:
	.long _builtin_asm_get_exceptionptrptr
#if !defined(USE_THREADS) || !defined(NATIVE_THREADS)
_exceptionptr:
	.long __exceptionptr
#endif

		
#define     MethodPointer   -8
#define     FrameSize       -12
#define     IsSync          -16
#define     IsLeaf          -20
#define     IntSave         -24
#define     FltSave         -28
#define     ExTableSize     -32
#define     ExTableStart    -32

#define     ExEntrySize     -16
#define     ExStartPC       -4
#define     ExEndPC         -8
#define     ExHandlerPC     -12
#define     ExCatchType     -16


	.align 2
    .long   0                         /* catch type all                       */
    .long   calljava_xhandler         /* handler pc                           */
    .long   calljava_xhandler         /* end pc                               */
    .long   _asm_calljavafunction     /* start pc                             */
    .long   1                         /* extable size                         */
    .long   0                         /* fltsave                              */
    .long   0                         /* intsave                              */
    .long   0                         /* isleaf                               */
    .long   0                         /* IsSync                               */
    .long   24                        /* frame size                           */
    .long   0                         /* method pointer (pointer to name)     */
	.long   0                         /* padding                              */


_asm_docalljavamethod:
	mflr    r0
	stw     r31,-4(r1)
//	stw     r30,-8(r1)
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
	stfd	f16,72(r1)
	stfd	f17,80(r1)
	stfd	f18,88(r1)
	stfd	f19,96(r1)
	stfd	f20,104(r1)
	stfd	f21,112(r1)
	stfd	f22,120(r1)
	stfd	f23,128(r1)

	stw     r3,36(r1)
	addi    r2,r1,36
	mr      r3,r4
	mr      r4,r5
	mr      r5,r6
	mr      r6,r7

//	addis   mptr,r31,ha16(_asm_call_jit_compiler-0b)
	addi    mptr,r31,lo16(_asm_call_jit_compiler-0b)
	stw     mptr,32(r1)
	addi    mptr,r1,28

	lwz     pv,4(mptr)
	mtctr   pv
	bctrl
1:
	mflr    itmp1
	addi    pv,itmp1,lo16(_asm_docalljavamethod-1b)

calljava_regrestore:
	lwz     r16,40(r1)
	lwz		r17,44(r1)
	lwz		r18,48(r1)
	lwz		r19,52(r1)
	lwz		r20,56(r1)
	lwz		r21,60(r1)
	lwz		r22,64(r1)
	lwz		r23,68(r1)
	lfd		f16,72(r1)
	lfd		f17,80(r1)
	lfd		f18,88(r1)
	lfd		f19,96(r1)
	lfd		f20,104(r1)
	lfd		f21,112(r1)
	lfd		f22,120(r1)
	lfd		f23,128(r1)

	lwz     r0,148+8(r1)
	mtlr    r0
	addi    r1,r1,148
	lwz     pv,-12(r1)
//	lwz     r30,-8(r1)
	lwz     r31,-4(r1)
	blr

calljava_xhandler:
	mr      r3,itmp1
//	addis   pv,r31,ha16(builtinthrow-0b)
	lwz     itmp1,lo16(builtinthrow-0b)(r31)
	mtctr   itmp1
	bctrl
	b       calljava_regrestore




	.align 2
    .long   0                         /* catch type all                       */
    .long   calljava_xhandler2        /* handler pc                           */
    .long   calljava_xhandler2        /* end pc                               */
    .long   _asm_calljavafunction2    /* start pc                             */
    .long   1                         /* extable size                         */
    .long   0                         /* fltsave                              */
    .long   0                         /* intsave                              */
    .long   0                         /* isleaf                               */
    .long   0                         /* IsSync                               */
    .long   24                        /* frame size                           */
    .long   0                         /* method pointer (pointer to name)     */
	.long   0                         /* padding                              */

_asm_calljavafunction2:
_asm_calljavafunction2long:
_asm_calljavafunction2double:
	mflr    r0
	stw     r31,-4(r1)
//	stw     r30,-8(r1)
	stw     pv,-12(r1)
	stw     r0,8(r1)
	addi    r1,r1,-148
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
	stfd	f16,72(r1)
	stfd	f17,80(r1)
	stfd	f18,88(r1)
	stfd	f19,96(r1)
	stfd	f20,104(r1)
	stfd	f21,112(r1)
	stfd	f22,120(r1)
	stfd	f23,128(r1)

	stw     r3,36(r1)                 /* save method pointer for compiler     */
	addi    r2,r1,36
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
//	addis   mptr,r31,ha16(_asm_call_jit_compiler-0b)
	addi    mptr,r31,lo16(_asm_call_jit_compiler-0b)
	stw     mptr,32(r1)
	addi    mptr,r1,28

	lwz     pv,4(mptr)
	mtctr   pv
	bctrl
1:
	mflr    itmp1
	addi    pv,itmp1,lo16(_asm_docalljavamethod-1b)
	
calljava_regrestore2:
	lwz		r16,40(r1)
	lwz		r17,44(r1)
	lwz		r18,48(r1)
	lwz		r19,52(r1)
	lwz		r20,56(r1)
	lwz		r21,60(r1)
	lwz		r22,64(r1)
	lwz		r23,68(r1)
	lfd		f16,72(r1)
	lfd		f17,80(r1)
	lfd		f18,88(r1)
	lfd		f19,96(r1)
	lfd		f20,104(r1)
	lfd		f21,112(r1)
	lfd		f22,120(r1)
	lfd		f23,128(r1)

	lwz     r0,148+8(r1)
	mtlr    r0
	addi    r1,r1,148
	lwz     pv,-12(r1)
//	lwz     r30,-8(r1)
	lwz     r31,-4(r1)
	blr

calljava_xhandler2:
	mr      r3,itmp1
//	addis   pv,r31,ha16(builtinthrow-0b)
	lwz     itmp1,lo16(builtinthrow-0b)(r31)
	mtctr   itmp1
	bctrl
	b       calljava_regrestore2


_asm_call_jit_compiler:
0:
	mflr    itmp1
	stw     r31,-4(r1)
//	stw     pv,-8(r1)
	stw     r29,-12(r1)
	stw     itmp1,8(r1)
	addi    r1,r1,-176
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
	mr      r29,mptr
	stw     r3,28(r1)
	stw     r4,32(r1)
	stw     r5,36(r1)
	stw     r6,40(r1)
	stw     r7,44(r1)
	stw     r8,48(r1)
	stw     r9,52(r1)
	stfd    f1,56(r1)
	stfd    f2,64(r1)
	stfd    f3,72(r1)
	stfd    f4,80(r1)
	stfd    f5,88(r1)
	stfd    f6,96(r1)
	stfd    f7,104(r1)
	stfd    f8,112(r1)
	stfd    f9,120(r1)
	stfd    f10,128(r1)
	stfd    f11,136(r1)
	stfd    f12,144(r1)
	stfd    f13,152(r1)
	stw     r10,160(r1)

	lwz     r3,0(r2)
//	addis   pv,r31,ha16(jitcompile-0b)
	lwz     itmp1,lo16(jitcompile-0b)(r31)
	mtctr   itmp1
	bctrl

	mr      pv,r3
	mr      mptr,r29
	lwz     r3,28(r1)
	lwz	  	r4,32(r1)
	lwz	  	r5,36(r1)
	lwz	  	r6,40(r1)
	lwz	  	r7,44(r1)
	lwz	  	r8,48(r1)
	lwz	  	r9,52(r1)
	lfd	  	f1,56(r1)
	lfd	  	f2,64(r1)
	lfd	  	f3,72(r1)
	lfd	  	f4,80(r1)
	lfd	  	f5,88(r1)
	lfd	  	f6,96(r1)
	lfd	  	f7,104(r1)
	lfd	  	f8,112(r1)
	lfd	  	f9,120(r1)
	lfd	  	f10,128(r1)
	lfd	  	f11,136(r1)
	lfd	  	f12,144(r1)
	lfd	  	f13,152(r1)
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
//	lwz     pv,-8(r1)
	lwz     r31,-4(r1)
	bctr



_asm_handle_nat_exception:
	mflr    r2
	lwz     itmp3,4(r2)
	extsh   itmp3,itmp3
	add     pv,itmp3,r2
	lwz     itmp3,8(r2)
	srwi    itmp3,itmp3,16
	cmpwi   itmp3,0x3dad
	bne     _asm_handle_exception
	lwz     itmp3,8(r2)
	slwi    itmp3,itmp3,16
	add     pv,pv,itmp3

_asm_handle_exception:
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

	li      r2,1
ex_stack_loop:
	addi    r1,r1,-4*4
	stw     xptr,0*4(r1)
	stw     xpc,1*4(r1)
	mflr    xptr
	stw     xptr,2*4(r1)
	stw     r2,3*4(r1)

	lwz     r3,0*4(r1)            /* exception pointer                        */
	lwz     r4,MethodPointer(pv)  /* method pointer                           */
	mr      r5,xpc                /* exception pc                             */
/*	mr      r6,r2 */
	li      r6,0                  /* line number                              */
	li      r7,4                  /* set no unwind flag                       */

	bl      0f
0:
	mflr    itmp1
	lwz     itmp1,lo16(builtin_traceexception-0b)(itmp1)
	mtctr   itmp1
	addi    r1,r1,-(24+5*4)       /* 24 linkage area + 5 argument * 4         */
	bctrl
	addi    r1,r1,(24+5*4)

	lwz     xptr,2*4(r1)
	mtlr    xptr
	lwz     xptr,0*4(r1)          /* restore xptr                             */
	lwz     xpc,1*4(r1)
	lwz     r2,3*4(r1)
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

	addi    r1,r1,-16*4           /* allocate stack                           */
	stw     r3,7*4(r1)            /* save used registers                      */
	stw     r4,8*4(r1)            /* 6*4 (linkage) + 1*4 (arg1) + 7*4 (save)  */
	stw     r2,9*4(r1)
	stw     xptr,10*4(r1)
	stw     xpc,11*4(r1)
	mflr    xptr
	stw     xptr,12*4(r1)
	stw     r7,13*4(r1)

	mr      r3,r7                 /* arg1 = exceptionclass                    */
	bl      0f
0:
	mflr    itmp1
	lwz     itmp1,lo16(class_load-0b)(itmp1)
	mtctr   itmp1
	bctrl

	lwz     r3,7*4(r1)
	lwz     r4,8*4(r1)
	lwz     r2,9*4(r1)
	lwz     xptr,10*4(r1)
	lwz     xpc,11*4(r1)
	lwz     itmp3,12*4(r1)
	mtlr    itmp3
	lwz     r7,13*4(r1)	
	addi    r1,r1,16*4

L_class_loaded:
	lwz     itmp3,offclasslinked(r7)
	mr.     itmp3,itmp3
	addi    r1,r1,-16*4           /* allocate stack                           */
	stw     r7,13*4(r1)
	bne     L_class_linked

	stw     r3,7*4(r1)            /* save used registers                      */
	stw     r4,8*4(r1)            /* 6*4 (linkage) + 1*4 (arg1) + 7*4 (save)  */
	stw     r2,9*4(r1)
	stw     xptr,10*4(r1)
	stw     xpc,11*4(r1)
	mflr    xptr
	stw     xptr,12*4(r1)

	mr      r3,r7                 /* arg1 = exceptionclass                    */
	bl      0f
0:
	mflr    itmp1
	lwz     itmp1,lo16(class_link-0b)(itmp1)
	mtctr   itmp1
	bctrl

	lwz     r3,7*4(r1)
	lwz     r4,8*4(r1)
	lwz     r2,9*4(r1)
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
	mr.     r2,r2
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
	mr.     r2,r2                 /* if here the first time, then             */
	beq     ex_already_cleared
	addi    r1,r1,18*4            /* deallocate stack and                     */
	li      r2,0                  /* clear the no unwind flag                 */
ex_already_cleared:
	lwz     r3,IsSync(pv)
	mr.     r3,r3
	beq     no_monitor_exit
	add     r3,r1,r3
	lwz     r6,-4(r3)

	addi    r1,r1,-6*4
	stw     r3,0*4(r1)
	stw     r4,1*4(r1)
	stw     r2,2*4(r1)
	stw     xptr,3*4(r1)
	stw     xpc,4*4(r1)
	mflr    xptr
	stw     xptr,5*4(r1)

	mr      r3,r6
	bl      0f
0:
	mflr    itmp1
	lwz     itmp1,lo16(builtin_monitorexit-0b)(itmp1)
	mtctr   itmp1
	addi    r1,r1,-40
	bctrl
	addi    r1,r1,40

	lwz     xptr,5*4(r1)
	mtlr    xptr
	lwz     r3,0*4(r1)
	lwz     r4,1*4(r1)
	lwz     r2,2*4(r1)
	lwz     xptr,3*4(r1)
	lwz     xpc,4*4(r1)
	addi    r1,r1,6*4

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
	addi    r5,r5,lo16(ex_int2-ex_int1)
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
	addi    r5,r5,lo16(ex_flt2-ex_flt1)
	slwi    r4,r4,2
	subf    r5,r4,r5
	mtctr   r5
	bctr
	lfd     f14,-80(r3)
	lfd     f15,-72(r3)
	lfd     f24,-64(r3)
	lfd     f25,-56(r3)
	lfd     f26,-48(r3)
	lfd     f27,-40(r3)
	lfd     f28,-32(r3)
	lfd     f29,-24(r3)
	lfd     f30,-16(r3)
	lfd     f31,-8(r3)
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

_asm_check_clinit:
0:
	lwz     itmp2,offclassinit(itmp1)
	mr.     itmp2,itmp2
	bne     L_is_initialized

	mflr	r0
	stw     r0,8(r1)
	addi    r1,r1,-(26*8)         /* keep stack 16-bytes aligned              */

    stw     r3,4*8(r1)            /* save argument registers                  */
    stw     r4,5*8(r1)            /* preserve linkage area (24 bytes)         */
    stw     r5,6*8(r1)            /* and 4 bytes (better 8) for 1 argument    */
    stw     r6,7*8(r1)
    stw     r7,8*8(r1)
    stw     r8,9*8(r1)
    stw     r9,10*8(r1)
	stw     r10,11*8(r1)

	stfd    f1,12*8(r1)
    stfd    f2,13*8(r1)
    stfd    f3,14*8(r1)
    stfd    f4,15*8(r1)
    stfd    f5,16*8(r1)
    stfd    f6,17*8(r1)
    stfd    f7,18*8(r1)
    stfd    f8,19*8(r1)
    stfd    f9,20*8(r1)
    stfd    f10,21*8(r1)
    stfd    f11,22*8(r1)
    stfd    f12,23*8(r1)
    stfd    f13,24*8(r1)
		
	mr      r3,itmp1
	lwz     itmp1,lo16(class_init-0b)(pv)
	mtctr   itmp1
	bctrl
	mr      itmp1,r3              /* save return value in temp register       */

	lwz     r3,4*8(r1)
	lwz     r4,5*8(r1)
	lwz     r5,6*8(r1)
	lwz     r6,7*8(r1)
	lwz     r7,8*8(r1)
	lwz     r8,9*8(r1)
	lwz     r9,10*8(r1)
	lwz     r10,11*8(r1)

    lfd     f1,12*8(r1)
    lfd     f2,13*8(r1)
    lfd     f3,14*8(r1)
    lfd   	f4,15*8(r1)
    lfd   	f5,16*8(r1)
    lfd   	f6,17*8(r1)
    lfd   	f7,18*8(r1)
    lfd   	f8,19*8(r1)
    lfd   	f9,20*8(r1)
    lfd   	f10,21*8(r1)
    lfd   	f11,22*8(r1)
    lfd   	f12,23*8(r1)
    lfd   	f13,24*8(r1)
		
	lwz     r0,(26*8)+8(r1)
	mtlr    r0
	addi    r1,r1,(26*8)

	mr.	    itmp1,itmp1           /* check for an exception                   */
	beq     L_initializererror

L_is_initialized:
	blr

L_initializererror:
0:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	mflr	r0
	stw     r0,8(r1)
	addi    r1,r1,-4*8            /* preserve linkage area (24 bytes)         */
	lwz     itmp1,lo16(builtin_asm_get_exceptionptrptr-0b)(pv)
	mtctr   itmp1
	bctrl
	lwz     r0,(4*8)+8(r1)
	mtlr    r0	
	addi    r1,r1,4*8
#else
	lwz     r3,lo16(_exceptionptr-0b)(pv)
#endif
	lwz     xptr,0(r3)         /* get the exception pointer                */
	li      r0,0
	stw     r0,0(r3)           /* clear the exception pointer              */

	mflr    xpc
	b       _asm_handle_nat_exception


/******************* function asm_builtin_checkarraycast ***********************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/
		
_asm_builtin_checkarraycast:
0:
	mflr    r0
	stw     r0,8(r1)
	stwu    r1,-48(r1)

	stw     r3,32(r1)
	lwz     itmp1,lo16(builtincheckarraycast-0b)(pv)
	mtctr   itmp1
	bctrl

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
	addi    r1,r1,-(24+4)
	lwz     r3,lo16(string_java_lang_ClassCastException-0b)(pv)
	lwz     r3,0(r3)
	lwz     itmp1,lo16(new_exception-0b)(pv)
	mtctr   itmp1
	bctrl
	mr      xptr,r3
	addi    r1,r1,24+4
	lwz     r0,8(r1)
	mr      xpc,r0
	mtlr    r0
	b       _asm_handle_nat_exception


/******************* function asm_builtin_aastore ******************************
*                                                                              *
*   Does the cast check and eventually throws an exception                     *
*                                                                              *
*******************************************************************************/
	
_asm_builtin_aastore:
0:
	mr.     r3,r3
	beq     nb_aastore_null
	mflr    r0
	stw     r0,8(r1)
	addi    r1,r1,-48

	lwz     itmp1,offarraysize(r3)
	slwi    itmp3,r4,2
	add     itmp2,r3,itmp3
	cmplw   r4,itmp1
	bge     nb_aastore_bound
	mr      r4,r5
	stw     itmp2,32(r1)
	stw     r4,36(r1)
//	addis   pv,r31,ha16(builtincanstore-0b)
	lwz     itmp1,lo16(builtincanstore-0b)(pv)
	mtctr   itmp1
	bctrl

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
	addi    r1,r1,-(24+4)
	lwz     r3,lo16(string_java_lang_NullPointerException-0b)(pv)
	lwz     r3,0(r3)
	lwz     itmp1,lo16(new_exception-0b)(pv)
	mtctr   itmp1
	bctrl
	mr      xptr,r3
	addi    r1,r1,(24+4)
	lwz     r0,8(r1)
	mr      xpc,r0
	mtlr    r0
	b       _asm_handle_nat_exception

nb_aastore_bound:
	addi    r1,r1,48
	mflr    r0
	stw     r0,8(r1)
	addi    r1,r1,-(24+2*4)
	lwz     r3,lo16(string_java_lang_ArrayIndexOutOfBoundsException-0b)(pv)
	lwz     r3,0(r3)
	lwz     itmp1,lo16(new_exception_int-0b)(pv)
	mtctr   itmp1
	bctrl                         /* r4 already contains the index            */
	mr      xptr,r3
	addi    r1,r1,(24+2*4)
	lwz     r0,8(r1)
	mr      xpc,r0
	mtlr    r0
	b       _asm_handle_nat_exception

nb_aastore_store:
	mflr    r0
	stw     r0,8(r1)
	addi    r1,r1,-(24+4)
	lwz     r3,lo16(string_java_lang_ArrayStoreException-0b)(pv)
	lwz     r3,0(r3)
	lwz     itmp1,lo16(new_exception-0b)(pv)
	mtctr   itmp1
	bctrl
	mr      xptr,r3
	addi    r1,r1,(24+4)
	lwz     r0,8(r1)
	mr      xpc,r0
	mtlr    r0
	b       _asm_handle_nat_exception


_asm_builtin_monitorenter:
0:
	lwz     itmp2,lo16(builtin_monitorenter-0b)(pv)
	mr.     r3,r3
	beq     nb_monitorenter
	mtctr   itmp2
	bctr

nb_monitorenter:
	mflr    r0
	stw     r0,8(r1)
	addi    r1,r1,-(24+4)
	lwz     r3,lo16(string_java_lang_NullPointerException-0b)(pv)
	lwz     r3,0(r3)
	lwz     itmp1,lo16(new_exception-0b)(pv)
	mtctr   itmp1
	bctrl
	mr      xptr,r3
	addi    r1,r1,24+4
	lwz     r0,8(r1)
/*	addi    xpc,r0,-4 */
	mr      xpc,r0
	addi    xpc,xpc,-4

	mtlr    r0
	b       _asm_handle_nat_exception


_asm_builtin_monitorexit:
0:
	lwz     itmp2,lo16(builtin_monitorexit-0b)(pv)
	mr.     r3,r3
	beq     nb_monitorexit
	mtctr   itmp2
	bctr

nb_monitorexit:
	mflr    r0
	stw     r0,8(r1)
	addi    r1,r1,-(24+4)
	lwz     r3,lo16(string_java_lang_NullPointerException-0b)(pv)
	lwz     r3,0(r3)
	lwz     itmp1,lo16(new_exception-0b)(pv)
	mtctr   itmp1
	bctrl
	mr      xptr,r3
	addi    r1,r1,24+4
	lwz     r0,8(r1)
/*	addi    xpc,r0,-4 */
	mr      xpc,r0
	addi    xpc,xpc,-4

	mtlr    r0
	b       _asm_handle_nat_exception
	
	
_asm_builtin_idiv:
0:
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
	addi    r1,r1,-(24+2*4)
	lwz     r3,lo16(string_java_lang_ArithmeticException-0b)(pv)
	lwz     r3,0(r3)
	lwz     r4,lo16(string_java_lang_ArithmeticException_message-0b)(pv)
	lwz     r4,0(r4)
	lwz     itmp1,lo16(new_exception_message-0b)(pv)
	mtctr   itmp1
	bctrl
	mr      xptr,r3
	addi    r1,r1,(24+2*4)
	lwz     r0,8(r1)
	mr      xpc,r0
	mtlr    r0
	b       _asm_handle_nat_exception


_asm_builtin_irem:
0:
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
	addi    r1,r1,-(24+2*4)
	lwz     r3,lo16(string_java_lang_ArithmeticException-0b)(pv)
	lwz     r3,0(r3)
	lwz     r4,lo16(string_java_lang_ArithmeticException_message-0b)(pv)
	lwz     r4,0(r4)
	lwz     itmp1,lo16(new_exception_message-0b)(pv)
	mtctr   itmp1
	bctrl
	mr      xptr,r3
	addi    r1,r1,(24+2*4)
	lwz     r0,8(r1)
	mr      xpc,r0
	mtlr    r0
	b       _asm_handle_nat_exception


_asm_builtin_ldiv:
0:
	or.     r0,r5,r6
	beq     nb_ldiv
	lwz     itmp1,lo16(builtin_ldiv-0b)(pv)
	mtctr   itmp1
	bctr

nb_ldiv:
	mflr    r0
	stw     r0,8(r1)
	addi    r1,r1,-(24+2*4)
	lwz     r3,lo16(string_java_lang_ArithmeticException-0b)(pv)
	lwz     r3,0(r3)
	lwz     r4,lo16(string_java_lang_ArithmeticException_message-0b)(pv)
	lwz     r4,0(r4)
	lwz     itmp1,lo16(new_exception_message-0b)(pv)
	mtctr   itmp1
	bctrl
	mr      xptr,r3
	addi    r1,r1,(24+2*4)
	lwz     r0,8(r1)
	mr      xpc,r0
	mtlr    r0
	b       _asm_handle_nat_exception


 _asm_builtin_lrem:
0:
	or.     r0,r5,r6
	beq     nb_lrem
	lwz     itmp1,lo16(builtin_lrem-0b)(pv)
	mtctr   itmp1
	bctr

nb_lrem:
	mflr    r0
	stw     r0,8(r1)
	addi    r1,r1,-(24+2*4)
	lwz     r3,lo16(string_java_lang_ArithmeticException-0b)(pv)
	lwz     r3,0(r3)
	lwz     r4,lo16(string_java_lang_ArithmeticException_message-0b)(pv)
	lwz     r4,0(r4)
	lwz     itmp1,lo16(new_exception_message-0b)(pv)
	mtctr   itmp1
	bctrl
	mr      xptr,r3
	addi    r1,r1,(24+2*4)
	lwz     r0,8(r1)
	mr      xpc,r0
	mtlr    r0
	b       _asm_handle_nat_exception


_asm_cacheflush:
	addi    r4,r4,31
	mr      r5,r3
	add     r4,r3,r4
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

_asm_initialize_thread_stack:
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
	lfd f0,lo16(doublezero-0b)(r3)

	stfd f0,40(r4)
	stfd f0,48(r4)
	stfd f0,56(r4)
	stfd f0,64(r4)
	stfd f0,72(r4)
	stfd f0,80(r4)
	stfd f0,88(r4)
	stfd f0,96(r4)
	stfd f0,104(r4)
	stfd f0,112(r4)

	stfd f0,160(r4)
	stfd f0,168(r4)
	stfd f0,176(r4)
	stfd f0,184(r4)
	stfd f0,192(r4)
	stfd f0,200(r4)
	stfd f0,208(r4)
	stfd f0,216(r4)

	mr r3,r4
	blr


_asm_perform_threadswitch:
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
	stfd f14,40(r1)
	stfd f15,48(r1)
	stfd f24,56(r1)
	stfd f25,64(r1)
	stfd f26,72(r1)
	stfd f27,80(r1)
	stfd f28,88(r1)
	stfd f29,96(r1)
	stfd f30,104(r1)
	stfd f31,112(r1)

	stw r16,128(r1)
	stw r17,132(r1)
	stw r18,136(r1)
	stw r19,140(r1)
	stw r20,144(r1)
	stw r21,148(r1)
	stw r22,152(r1)
	stw r23,156(r1)
	stfd f16,160(r1)
	stfd f17,168(r1)
	stfd f18,176(r1)
	stfd f19,184(r1)
	stfd f20,192(r1)
	stfd f21,200(r1)
	stfd f22,208(r1)
	stfd f23,216(r1)

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
	lfd f14,40(r1)
	lfd f15,48(r1)
	lfd f24,56(r1)
	lfd f25,64(r1)
	lfd f26,72(r1)
	lfd f27,80(r1)
	lfd f28,88(r1)
	lfd f29,96(r1)
	lfd f30,104(r1)
	lfd f31,112(r1)

	lwz r16,128(r1)
	lwz r17,132(r1)
	lwz r18,136(r1)
	lwz r19,140(r1)
	lwz r20,144(r1)
	lwz r21,148(r1)
	lwz r22,152(r1)
	lwz r23,156(r1)
	lfd f16,160(r1)
	lfd f17,168(r1)
	lfd f18,176(r1)
	lfd f19,184(r1)
	lfd f20,192(r1)
	lfd f21,200(r1)
	lfd f22,208(r1)
	lfd f23,216(r1)

	mtlr r0
	addi r1,r1,224
	blr


_asm_switchstackandcall:
	mflr r0
	addi r3,r3,-48
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


_asm_getclassvalues_atomic:
_crit_restart2:
_crit_begin2:
	lwz r6,offbaseval(r3)
	lwz r7,offdiffval(r3)
	lwz r8,offbaseval(r4)
_crit_end2:
	stw r6,offcast_super_baseval(r5)
	stw r7,offcast_super_diffval(r5)
	stw r8,offcast_sub_baseval(r5)
	blr

	.data

_asm_criticalsections:
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	.long _crit_begin1
	.long _crit_end1
	.long _crit_restart1
	.long _crit_begin2
	.long _crit_end2
	.long _crit_restart2
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
