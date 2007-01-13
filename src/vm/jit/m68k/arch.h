/* src/vm/jit/m68k/arch.h - architecture defines for m68k

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

   Authors: Roland Lezuo

   Changes:

   $Id: arch.h 5330 2006-09-05 18:43:12Z edwin $

*/


#ifndef _ARCH_H
#define _ARCH_H

/*#define USE_FAKE_ATOMIC_INSTRUCTIONS 	0 */

/* define architecture features ***********************************************/

#define U8_AVAILABLE                     1

#define USEBUILTINTABLE

#define SUPPORT_DIVISION                 0
#define SUPPORT_LONG                     0
#define SUPPORT_FLOAT                    0
#define SUPPORT_DOUBLE                   0

#define SUPPORT_FMOD                     0
#define SUPPORT_FICVT                    0
#define SUPPORT_IFCVT                    0

#define SUPPORT_LONG_ADD                 0
#define SUPPORT_LONG_CMP                 0
#define SUPPORT_LONG_CMP_CONST           0
#define SUPPORT_LONG_LOGICAL             0
#define SUPPORT_LONG_SHIFT               0	
#define SUPPORT_LONG_MUL                 0
#define SUPPORT_LONG_DIV                 0
#define SUPPORT_LONG_ICVT                0
#define SUPPORT_LONG_FCVT                0

#define SUPPORT_CONST_LOGICAL            0  /* AND, OR, XOR with immediates   */
#define SUPPORT_CONST_MUL                0  /* mutiply with immediate         */

#define SUPPORT_CONST_STORE              0  /* do we support const stores     */
#define SUPPORT_CONST_STORE_ZERO_ONLY    0  /* on some risc machines we can   */
                                            /* only store REG_ZERO            */

#define SPECIALMEMUSE
/* #define HAS_4BYTE_STACKSLOT */
/* #define SUPPORT_COMBINE_INTEGER_REGISTERS */

#endif /* _ARCH_H */


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