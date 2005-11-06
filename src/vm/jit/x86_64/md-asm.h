/* src/vm/jit/x86_64/md-asm.h - assembler defines for x86_64 Linux ABI

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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

   Authors: Christian Thalinger

   Changes:

   $Id: md-asm.h 3589 2005-11-06 14:44:42Z twisti $

*/


#ifndef _MD_ASM_H
#define _MD_ASM_H


/* register defines ***********************************************************/

#define v0       %rax
#define v0l      %eax

#define a0       %rdi
#define a1       %rsi
#define a2       %rdx
#define a3       %rcx
#define a4       %r8
#define a5       %r9

#define fa0      %xmm0
#define fa1      %xmm1
#define fa2      %xmm2
#define fa3      %xmm3
#define fa4      %xmm4
#define fa5      %xmm5
#define fa6      %xmm6
#define fa7      %xmm7

#define sp       %rsp
#define bp       %rbp

#define itmp1    %rax
#define itmp2    %r10
#define itmp3    %r11

#define itmp1l   %eax
#define itmp2l   %r10d
#define itmp3l   %r11d

#define itmp1b   %al
#define itmp2b   %r10b
#define itmp3b   %r11b

#define xptr     itmp1
#define xpc      itmp2


/* save and restore macros ****************************************************/

#define SAVE_ARGUMENT_REGISTERS(off) \
	mov     a0,(0+(off))*8(%rsp)		; \
	mov     a1,(1+(off))*8(%rsp)		; \
	mov     a2,(2+(off))*8(%rsp)		; \
	mov     a3,(3+(off))*8(%rsp)		; \
	mov     a4,(4+(off))*8(%rsp)		; \
	mov     a5,(5+(off))*8(%rsp)		; \
	\
	movq    fa0,(6+(off))*8(%rsp)		; \
	movq    fa1,(7+(off))*8(%rsp)		; \
	movq    fa2,(8+(off))*8(%rsp)		; \
	movq    fa3,(9+(off))*8(%rsp)		; \
	movq    fa4,(10+(off))*8(%rsp)		; \
	movq    fa5,(11+(off))*8(%rsp)		; \
	movq    fa6,(12+(off))*8(%rsp)		; \
	movq    fa7,(13+(off))*8(%rsp)		;


#define RESTORE_ARGUMENT_REGISTERS(off) \
	mov     (0+(off))*8(%rsp),a0		; \
	mov     (1+(off))*8(%rsp),a1		; \
	mov     (2+(off))*8(%rsp),a2		; \
	mov     (3+(off))*8(%rsp),a3		; \
	mov     (4+(off))*8(%rsp),a4		; \
	mov     (5+(off))*8(%rsp),a5		; \
	\
	movq    (6+(off))*8(%rsp),fa0		; \
	movq    (7+(off))*8(%rsp),fa1		; \
	movq    (8+(off))*8(%rsp),fa2		; \
	movq    (9+(off))*8(%rsp),fa3		; \
	movq    (10+(off))*8(%rsp),fa4		; \
	movq    (11+(off))*8(%rsp),fa5		; \
	movq    (12+(off))*8(%rsp),fa6		; \
	movq    (13+(off))*8(%rsp),fa7		; 


#define SAVE_TEMPORARY_REGISTERS(off) \
	mov     %rbx,(0+(off))*8(%rsp)


#define RESTORE_TEMPORARY_REGISTERS(off) \
	mov     (0+(off))*8(%rsp),%rbx

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
