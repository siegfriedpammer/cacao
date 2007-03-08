/* src/vm/jit/x86_64/md-asm.h - assembler defines for x86_64 Linux ABI

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

   Authors: Christian Thalinger

   Changes:

   $Id: md-asm.h 7486 2007-03-08 13:50:07Z twisti $

*/


#ifndef _MD_ASM_H
#define _MD_ASM_H

/* register defines ***********************************************************/

#if 0

#define v0       %rax
#define v0l      %eax
#define itmp1    v0

#define a3       %rcx
#define a2       %rdx

#define t0       %rbx
#define t0l      %ebx

#define sp       %rsp
#define s0       %rbp

#define a1       %rsi
#define a0       %rdi
#define a0l      %edi

#define a4       %r8
#define a5       %r9

#define itmp2    %r10
#define itmp3    %r14

#define s1       %r12
#define s2       %r13
#define s3       %r14
#define s4       %r15


#define bp       s0

#define itmp1l   %eax
#define itmp2l   %r10d
#define itmp3l   %r11d

#define xptr     itmp1
#define xpc      itmp2
#define mptr     itmp2


#define fa0      %xmm0
#define fa1      %xmm1
#define fa2      %xmm2
#define fa3      %xmm3
#define fa4      %xmm4
#define fa5      %xmm5
#define fa6      %xmm6
#define fa7      %xmm7

#define ftmp1    %xmm8
#define ftmp2    %xmm9
#define ftmp3    %xmm10

#define ft0      %xmm11
#define ft1      %xmm12
#define ft2      %xmm13
#define ft3      %xmm14
#define ft4      %xmm15

#endif

#define a0       %r2
#define a1       %r3
#define a2       %r4
#define a3       %r5
#define a4       %r6

#define sp       %r15
#define itmp1    %r1
#define itmp2    %r12
#define itmp3    %r14
#define v0       %r2
#define pv       %r13

#define mptr     itmp2
#define xptr     itmp1
#define xpc      itmp2

#define s0 %r7
#define s1 %r8
#define s2 %r9
#define s3 %r10
#define s4 %r11

#define fa0 %f0
#define fa1 %f2

/* save and restore macros ****************************************************/

#define SAVE_ARGUMENT_REGISTERS(off) \
	mov     a0,(0+(off))*8(sp)   ; \
	mov     a1,(1+(off))*8(sp)   ; \
	mov     a2,(2+(off))*8(sp)   ; \
	mov     a3,(3+(off))*8(sp)   ; \
	mov     a4,(4+(off))*8(sp)   ; \
	mov     a5,(5+(off))*8(sp)   ; \
	\
	movq    fa0,(6+(off))*8(sp)  ; \
	movq    fa1,(7+(off))*8(sp)  ; \
	movq    fa2,(8+(off))*8(sp)  ; \
	movq    fa3,(9+(off))*8(sp)  ; \
	movq    fa4,(10+(off))*8(sp) ; \
	movq    fa5,(11+(off))*8(sp) ; \
	movq    fa6,(12+(off))*8(sp) ; \
	movq    fa7,(13+(off))*8(sp) ;


#define RESTORE_ARGUMENT_REGISTERS(off) \
	mov     (0+(off))*8(sp),a0   ; \
	mov     (1+(off))*8(sp),a1   ; \
	mov     (2+(off))*8(sp),a2   ; \
	mov     (3+(off))*8(sp),a3   ; \
	mov     (4+(off))*8(sp),a4   ; \
	mov     (5+(off))*8(sp),a5   ; \
	\
	movq    (6+(off))*8(sp),fa0  ; \
	movq    (7+(off))*8(sp),fa1  ; \
	movq    (8+(off))*8(sp),fa2  ; \
	movq    (9+(off))*8(sp),fa3  ; \
	movq    (10+(off))*8(sp),fa4 ; \
	movq    (11+(off))*8(sp),fa5 ; \
	movq    (12+(off))*8(sp),fa6 ; \
	movq    (13+(off))*8(sp),fa7 ;


#define SAVE_TEMPORARY_REGISTERS(off) \
	mov     t0,(0+(off))*8(sp)   ; \
	movq    ft0,(1+(off))*8(sp)  ; \
	movq    ft1,(2+(off))*8(sp)  ; \
	movq    ft2,(3+(off))*8(sp)  ; \
	movq    ft3,(4+(off))*8(sp)  ;


#define RESTORE_TEMPORARY_REGISTERS(off) \
	mov     (0+(off))*8(sp),t0   ; \
	movq    (1+(off))*8(sp),ft0  ; \
	movq    (2+(off))*8(sp),ft1  ; \
	movq    (3+(off))*8(sp),ft2  ; \
	movq    (4+(off))*8(sp),ft3  ;

/* Volatile float registers (all volatile in terms of C abi) */

#define LOAD_STORE_VOLATILE_FLOAT_REGISTERS(inst, off) \
	inst    %f0, ((0 * 8) + (off))(sp); \
	inst    %f2, ((1 * 8) + (off))(sp); \
	inst    %f1, ((2 * 8) + (off))(sp); \
	inst    %f3, ((3 * 8) + (off))(sp); \
	inst    %f5, ((4 * 8) + (off))(sp); \
	inst    %f7, ((5 * 8) + (off))(sp); \
	inst    %f8, ((6 * 8) + (off))(sp); \
	inst    %f9, ((7 * 8) + (off))(sp); \
	inst    %f10, ((8 * 8) + (off))(sp); \
	inst    %f11, ((9 * 8) + (off))(sp); \
	inst    %f12, ((10 * 8) + (off))(sp); \
	inst    %f13, ((11 * 8) + (off))(sp); \
	inst    %f14, ((12 * 8) + (off))(sp); \
	inst    %f15, ((13 * 8) + (off))(sp); 

#define VOLATILE_FLOAT_REGISTERS_SIZE (14 * 8)

#define LOAD_VOLATILE_FLOAT_REGISTERS(off) LOAD_STORE_VOLATILE_FLOAT_REGISTERS(ld, off)
#define STORE_VOLATILE_FLOAT_REGISTERS(off) LOAD_STORE_VOLATILE_FLOAT_REGISTERS(std, off)

/* Volatile integer registers (all volatile in terms of C abi) */

#define LOAD_STORE_VOLATILE_INTEGER_REGISTERS(instm, inst, off) \
	instm   %r0, %r5, ((0 * 4) + (off))(sp); \
	inst    %r14, ((6 * 4) + (off))(sp);

#define VOLATILE_INTEGER_REGISTERS_SIZE (7 * 4)

#define LOAD_VOLATILE_INTEGER_REGISTERS(off) LOAD_STORE_VOLATILE_INTEGER_REGISTERS(lm, l, off)
#define STORE_VOLATILE_INTEGER_REGISTERS(off) LOAD_STORE_VOLATILE_INTEGER_REGISTERS(stm, st, off)

/* Argument registers (in terms of JAVA an C abi) */

#define ARGUMENT_REGISTERS_SIZE ((5 * 4) + (2 * 8))

#define LOAD_STORE_ARGUMENT_REGISTERS(iinst, finst, off) \
	iinst %r2, %r6, (off)(sp) ; \
	finst %f0, (off +  (5 * 4))(sp) ; \
	finst %f2, (off + (5 * 4) + 8)(sp)

#define STORE_ARGUMENT_REGISTERS(off) LOAD_STORE_ARGUMENT_REGISTERS(stm, std, off)
#define LOAD_ARGUMENT_REGISTERS(off) LOAD_STORE_ARGUMENT_REGISTERS(lm, ld, off)

/* Temporary registers (in terms of JAVA abi) */

#define TEMPORARY_REGISTERS_SIZE ((1 * 4) + (12 * 8))

#define LOAD_STORE_TEMPORARY_REGISTERS(iinst, finst, off) \
	finst    %f1, ((0 * 8) + (off))(sp); \
	finst    %f3, ((1 * 8) + (off))(sp); \
	finst    %f5, ((2 * 8) + (off))(sp); \
	finst    %f7, ((3 * 8) + (off))(sp); \
	finst    %f8, ((4 * 8) + (off))(sp); \
	finst    %f9, ((5 * 8) + (off))(sp); \
	finst    %f10, ((6 * 8) + (off))(sp); \
	finst    %f11, ((7 * 8) + (off))(sp); \
	finst    %f12, ((8 * 8) + (off))(sp); \
	finst    %f13, ((9 * 8) + (off))(sp); \
	finst    %f14, ((10 * 8) + (off))(sp); \
	finst    %f15, ((11 * 8) + (off))(sp); \
	iinst    %r0, ((12 * 8) + (off))(sp);

#define LOAD_TEMPORARY_REGISTERS(off) LOAD_STORE_TEMPORARY_REGISTERS(l, ld, off)
#define STORE_TEMPORARY_REGISTERS(off) LOAD_STORE_TEMPORARY_REGISTERS(st, std, off)

#endif /* _MD_ASM_H */


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
