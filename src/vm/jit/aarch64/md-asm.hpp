/* src/vm/jit/aarch64/md-asm.hpp - assembler defines for Aarch64 ABI

   Copyright (C) 1996-2013
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


#ifndef MD_ASM_HPP_
#define MD_ASM_HPP_ 1


/* register defines ***********************************************************/

#define a0      x0  /* argument & result registers */
#define a1      x1
#define a2      x2
#define a3      x3
#define a4      x4
#define a5      x5
#define a6      x6
#define a7      x7

#define ires    x8  /* indirect result location (structs) */

#define t0      x9  /* temporary registers */
#define t1      x10
#define t2      x11
#define t3      x12
#define t4      x13
#define t5      x14
#define t6      x15

#define xptr    x9  /* exception pointer = itmp1 */
#define xpc     x10 /* exception pc = itmp2 */

#define ip0     x16 /* intra-procedure-call temporary registers */
#define ip1     x17

#define platf   x18 /* platform register (for platf-specific ABI) */

#define pv      x17 /* using ip1 for procedure vector */
#define mptr    x16 /* using ip0 for method pointer */

#define s0      x19 /* variable registers, all following registers are callee saved reg */
#define s1      x20
#define s2      x21
#define s3      x22
#define s4      x23
#define s5      x24
#define s6      x25
#define s7      x26
#define s8      x27
#define s9      x28

#define fp      x29 /* frame pointer */
#define lr      x30 /* link register */
#define sp      sp  /* stack pointer */

#define zero    xzr /* zero register */

#define fa0     d0  /*argument and result register for SMID & FP */
#define fa1     d1
#define fa2     d2
#define fa3     d3
#define fa4     d4
#define fa5     d5
#define fa6     d6
#define fa7     d7

#endif // MD_ASM_HPP_

/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
