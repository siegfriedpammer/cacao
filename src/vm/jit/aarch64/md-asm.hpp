/* src/vm/jit/alpha/md-asm.hpp - assembler defines for Alpha ABI

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

#define ip0     x16 /* intra-procedure-call temporary registers */
#define ip1     x17

#define platf   x18 /* platform register (for platf-specific ABI) */

#define v0      x19 /* variable registers, all following registers are callee saved reg */
#define v1      x20
#define v2      x21
#define v3      x22
#define v4      x23
#define v5      x24
#define v6      x25
#define v7      x26
#define v8      x27
#define v9      x28

#define fp      x29 /* frame pointer */
#define lr      x30 /* link register */
#define sp      sp  /* stack pointer */

#define zero    xzr /* zero register */

/* save and restore macros ****************************************************/

#define SAVE_TEMPORARY_REGISTERS \
    mov     t0, sp; \
    stp     v0, v1, [t0], 16; \
    stp     v2, v3, [t0], 16

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
