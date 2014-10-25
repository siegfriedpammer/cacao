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
#define xpc     x18 /* exception pc = itmp2 */

#define itmp1   x9
#define itmp2   x10
#define itmp3   x11

#define t3i     w12
#define t4i     w13

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

#define fs0     d8
#define fs1     d9
#define fs2     d10
#define fs3     d11
#define fs4     d12
#define fs5     d13
#define fs6     d14
#define fs7     d15

#define ft0     d16
#define ft1     d17
#define ft2     d18
#define ft3     d19
#define ft4     d20
#define ft5     d21
#define ft6     d22
#define ft7     d23
#define ft8     d24
#define ft9     d25
#define ft10    d26
#define ft11    d27
#define ft12    d28
#define ft13    d29
#define ft14    d30
#define ft15    d31

/* save and restore macros ***************************************************/

#define SAVE_ARGUMENT_REGISTERS \
    str     a0, [sp]        ; \
    str     a1, [sp, 1 * 8] ; \
    str     a2, [sp, 2 * 8] ; \
    str     a3, [sp, 3 * 8] ; \
    str     a4, [sp, 4 * 8] ; \
    str     a5, [sp, 5 * 8] ; \
    str     a6, [sp, 6 * 8] ; \
    str     a7, [sp, 7 * 8] ; \
    \
    str     fa0, [sp, 8 * 8]    ; \
    str     fa1, [sp, 9 * 8]    ; \
    str     fa2, [sp, 10 * 8]    ; \
    str     fa3, [sp, 11 * 8]    ; \
    str     fa4, [sp, 12 * 8]    ; \
    str     fa5, [sp, 13 * 8]    ; \
    str     fa6, [sp, 14 * 8]    ; \
    str     fa7, [sp, 15 * 8]    ; 

#define RESTORE_ARGUMENT_REGISTERS \
    ldr     a0, [sp]        ; \
    ldr     a1, [sp, 1 * 8] ; \
    ldr     a2, [sp, 2 * 8] ; \
    ldr     a3, [sp, 3 * 8] ; \
    ldr     a4, [sp, 4 * 8] ; \
    ldr     a5, [sp, 5 * 8] ; \
    ldr     a6, [sp, 6 * 8] ; \
    ldr     a7, [sp, 7 * 8] ; \
    \
    ldr     fa0, [sp, 8 * 8]    ; \
    ldr     fa1, [sp, 9 * 8]    ; \
    ldr     fa2, [sp, 10 * 8]    ; \
    ldr     fa3, [sp, 11 * 8]    ; \
    ldr     fa4, [sp, 12 * 8]    ; \
    ldr     fa5, [sp, 13 * 8]    ; \
    ldr     fa6, [sp, 14 * 8]    ; \
    ldr     fa7, [sp, 15 * 8]    ; 

#define SAVE_TEMPORARY_REGISTERS(off) \
    str     t0, [sp, (0 + (off)) * 8]   ; \
    str     t1, [sp, (1 + (off)) * 8]   ; \
    str     t2, [sp, (2 + (off)) * 8]   ; \
    str     t3, [sp, (3 + (off)) * 8]   ; \
    str     t4, [sp, (4 + (off)) * 8]   ; \
    str     t5, [sp, (5 + (off)) * 8]   ; \
    str     t6, [sp, (6 + (off)) * 8]   ; \
    \
    str     ft0, [sp, (7 + (off)) * 8]  ; \
    str     ft1, [sp, (8 + (off)) * 8]  ; \
    str     ft2, [sp, (9 + (off)) * 8]  ; \
    str     ft3, [sp, (10 + (off)) * 8]  ; \
    str     ft4, [sp, (11 + (off)) * 8]  ; \
    str     ft5, [sp, (12 + (off)) * 8]  ; \
    str     ft6, [sp, (13 + (off)) * 8]  ; \
    str     ft7, [sp, (14 + (off)) * 8]  ; \
    str     ft8, [sp, (15 + (off)) * 8]  ; \
    str     ft9, [sp, (16 + (off)) * 8]  ; \
    str     ft10, [sp, (17 + (off)) * 8]  ; \
    str     ft11, [sp, (18 + (off)) * 8]  ; \
    str     ft12, [sp, (19 + (off)) * 8]  ; \
    str     ft13, [sp, (20 + (off)) * 8]  ; \
    str     ft14, [sp, (21 + (off)) * 8]  ; \
    str     ft15, [sp, (22 + (off)) * 8]  ; 

#define RESTORE_TEMPORARY_REGISTERS(off) \
    ldr     t0, [sp, (0 + (off)) * 8]   ; \
    ldr     t1, [sp, (1 + (off)) * 8]   ; \
    ldr     t2, [sp, (2 + (off)) * 8]   ; \
    ldr     t3, [sp, (3 + (off)) * 8]   ; \
    ldr     t4, [sp, (4 + (off)) * 8]   ; \
    ldr     t5, [sp, (5 + (off)) * 8]   ; \
    ldr     t6, [sp, (6 + (off)) * 8]   ; \
    \
    ldr     ft0, [sp, (7 + (off)) * 8]  ; \
    ldr     ft1, [sp, (8 + (off)) * 8]  ; \
    ldr     ft2, [sp, (9 + (off)) * 8]  ; \
    ldr     ft3, [sp, (10 + (off)) * 8]  ; \
    ldr     ft4, [sp, (11 + (off)) * 8]  ; \
    ldr     ft5, [sp, (12 + (off)) * 8]  ; \
    ldr     ft6, [sp, (13 + (off)) * 8]  ; \
    ldr     ft7, [sp, (14 + (off)) * 8]  ; \
    ldr     ft8, [sp, (15 + (off)) * 8]  ; \
    ldr     ft9, [sp, (16 + (off)) * 8]  ; \
    ldr     ft10, [sp, (17 + (off)) * 8]  ; \
    ldr     ft11, [sp, (18 + (off)) * 8]  ; \
    ldr     ft12, [sp, (19 + (off)) * 8]  ; \
    ldr     ft13, [sp, (20 + (off)) * 8]  ; \
    ldr     ft14, [sp, (21 + (off)) * 8]  ; \
    ldr     ft15, [sp, (22 + (off)) * 8]  ; 

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
