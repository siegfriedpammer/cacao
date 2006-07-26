/* src/vm/jit/powerpc/linux/md-asm.h - assembler defines for PowerPC Linux ABI

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

   $Id: md-asm.h 5176 2006-07-26 09:43:08Z twisti $

*/


#ifndef _MD_ASM_H
#define _MD_ASM_H

/* register defines ***********************************************************/

#define r0    0
#define sp    1

/* #define XXX   r2  -  system reserved register */

#define a0    3
#define a1    4
#define a2    5
#define a3    6
#define a4    7
#define a5    8
#define a6    9
#define a7    10

#define itmp1 11
#define itmp2 12
#define pv    13

#define s0    14
#define s1    15

#define itmp3 16
#define t0    17
#define t1    18
#define t2    19
#define t3    20
#define t4    21
#define t5    22
#define t6    23

#define s2    24
#define s3    25
#define s4    26
#define s5    27
#define s6    28
#define s7    29
#define s8    30
#define s9    31

#define v0    a0
#define v1    a1

#define xptr  itmp1
#define xpc   itmp2

#define mptr  itmp2
#define mptrn itmp2


#define ftmp3 0

#define fa0   1
#define fa1   2
#define fa2   3
#define fa3   4
#define fa4   5
#define fa5   6
#define fa6   7
#define fa7   8

#define fa8   9
#define fa9   10
#define fa10  11
#define fa11  12
#define fa12  13

#define fs0   14
#define fs1   15

#define ftmp1 16
#define ftmp2 17

#define ft0   18
#define ft1   19
#define ft2   20
#define ft3   21
#define ft4   22
#define ft5   23

#define fs2   24
#define fs3   25
#define fs4   26
#define fs5   27
#define fs6   28
#define fs7   29
#define fs8   30
#define fs9   31

#define fv0   fa0


/* save and restore macros ****************************************************/

#define SAVE_ARGUMENT_REGISTERS(off) \
	stw     a0,(0+(off))*4(sp); \
	stw     a1,(1+(off))*4(sp); \
	stw     a2,(2+(off))*4(sp); \
	stw     a3,(3+(off))*4(sp); \
	stw     a4,(4+(off))*4(sp); \
	stw     a5,(5+(off))*4(sp); \
	stw     a6,(6+(off))*4(sp); \
	stw     a7,(7+(off))*4(sp); \
	\
	stfd    fa0,(8+(off))*4(sp); \
	stfd    fa1,(10+(off))*4(sp); \
	stfd    fa2,(12+(off))*4(sp); \
	stfd    fa3,(14+(off))*4(sp); \
	stfd    fa4,(16+(off))*4(sp); \
	stfd    fa5,(18+(off))*4(sp); \
	stfd    fa6,(20+(off))*4(sp); \
	stfd    fa7,(22+(off))*4(sp);

#define RESTORE_ARGUMENT_REGISTERS(off) \
	lwz     a0,(0+(off))*4(sp); \
	lwz     a1,(1+(off))*4(sp); \
	lwz     a2,(2+(off))*4(sp); \
	lwz     a3,(3+(off))*4(sp); \
	lwz     a4,(4+(off))*4(sp); \
	lwz     a5,(5+(off))*4(sp); \
	lwz     a6,(6+(off))*4(sp); \
	lwz     a7,(7+(off))*4(sp); \
	\
	lfd     fa0,(8+(off))*4(sp); \
	lfd     fa1,(10+(off))*4(sp); \
	lfd     fa2,(12+(off))*4(sp); \
	lfd     fa3,(14+(off))*4(sp); \
	lfd     fa4,(16+(off))*4(sp); \
	lfd     fa5,(18+(off))*4(sp); \
	lfd     fa6,(20+(off))*4(sp); \
	lfd     fa7,(22+(off))*4(sp);


#define SAVE_TEMPORARY_REGISTERS(off) \
	stw     t0,(0+(off))*4(sp); \
	stw     t1,(1+(off))*4(sp); \
	stw     t2,(2+(off))*4(sp); \
	stw     t3,(3+(off))*4(sp); \
	stw     t4,(4+(off))*4(sp); \
	stw     t5,(5+(off))*4(sp); \
	stw     t6,(6+(off))*4(sp); \
	\
	stfd    ft0,(8+(off))*4(sp); \
	stfd    ft1,(10+(off))*4(sp); \
	stfd    ft2,(12+(off))*4(sp); \
	stfd    ft3,(14+(off))*4(sp); \
	stfd    ft4,(16+(off))*4(sp); \
	stfd    ft5,(18+(off))*4(sp);

#define RESTORE_TEMPORARY_REGISTERS(off) \
	lwz     t0,(0+(off))*4(sp); \
	lwz     t1,(1+(off))*4(sp); \
	lwz     t2,(2+(off))*4(sp); \
	lwz     t3,(3+(off))*4(sp); \
	lwz     t4,(4+(off))*4(sp); \
	lwz     t5,(5+(off))*4(sp); \
	lwz     t6,(6+(off))*4(sp); \
	\
	lfd     ft0,(8+(off))*4(sp); \
	lfd     ft1,(10+(off))*4(sp); \
	lfd     ft2,(12+(off))*4(sp); \
	lfd     ft3,(14+(off))*4(sp); \
	lfd     ft4,(16+(off))*4(sp); \
	lfd     ft5,(18+(off))*4(sp);

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
