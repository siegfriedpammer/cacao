/* src/vm/jit/x86_64/arch.hpp - architecture defines for x86_64

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


#ifndef ARCH_HPP_
#define ARCH_HPP_ 1

#define JIT_COMPILER_VIA_SIGNAL

#include "config.h"


/* define architecture features ***********************************************/

#define SUPPORT_DIVISION                 1

#define SUPPORT_I2F                      1
#define SUPPORT_I2D                      1
#define SUPPORT_L2F                      1
#define SUPPORT_L2D                      1

/* ATTENTION: x86_64 architectures support these conversions, but we
   need the builtin functions in corner cases */
#define SUPPORT_F2I                      0
#define SUPPORT_F2L                      0
#define SUPPORT_D2I                      0
#define SUPPORT_D2L                      0

#define SUPPORT_LONG_ADD                 1
#define SUPPORT_LONG_CMP                 1
#define SUPPORT_LONG_SHIFT               1
#define SUPPORT_LONG_MUL                 1
#define SUPPORT_LONG_DIV                 1

#define SUPPORT_LONG_DIV_POW2            1
#define SUPPORT_LONG_REM_POW2            1

#define SUPPORT_CONST_LOGICAL            1  /* AND, OR, XOR with immediates   */
#define SUPPORT_CONST_MUL                1  /* mutiply with immediate         */

#define SUPPORT_CONST_STORE              1  /* do we support const stores     */
#define SUPPORT_CONST_STORE_ZERO_ONLY    0  /* on some risc machines we can   */
                                            /* only store REG_ZERO            */


/* float **********************************************************************/

#define SUPPORT_FLOAT                    1

#if defined(ENABLE_SOFT_FLOAT_CMP)
# define SUPPORT_FLOAT_CMP               0
#else
# define SUPPORT_FLOAT_CMP               1
#endif


/* double *********************************************************************/

#define SUPPORT_DOUBLE                   1

#if defined(ENABLE_SOFT_FLOAT_CMP)
# define SUPPORT_DOUBLE_CMP              0
#else
# define SUPPORT_DOUBLE_CMP              1
#endif


#define CONSECUTIVE_INTEGER_ARGS
#define CONSECUTIVE_FLOAT_ARGS


/* branches *******************************************************************/

#define SUPPORT_BRANCH_CONDITIONAL_CONDITION_REGISTER       1
#define SUPPORT_BRANCH_CONDITIONAL_ONE_INTEGER_REGISTER     0
#define SUPPORT_BRANCH_CONDITIONAL_TWO_INTEGER_REGISTERS    0
#define SUPPORT_BRANCH_CONDITIONAL_UNSIGNED_CONDITIONS      1


/* exceptions *****************************************************************/

#define SUPPORT_HARDWARE_DIVIDE_BY_ZERO  1


/* stackframe *****************************************************************/

#define STACKFRAME_RA_BETWEEN_FRAMES              1
#define STACKFRAME_RA_TOP_OF_FRAME                0
#define STACKFRAME_RA_LINKAGE_AREA                0
#define STACKFRAME_LEAFMETHODS_RA_REGISTER        0
#define STACKFRAME_SYNC_NEEDS_TWO_SLOTS           0
#define STACKFRAME_PACKED_SAVED_REGISTERS         0


/* replacement ****************************************************************/

#define REPLACEMENT_PATCH_SIZE           2             /* bytes */

/* subtype ********************************************************************/

#define USES_NEW_SUBTYPE                 1

/* memory barriers ************************************************************/

#define CAS_PROVIDES_FULL_BARRIER        1

#define USES_PATCHABLE_MEMORY_BARRIER    1

#endif // ARCH_HPP_

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
 * vim:noexpandtab:sw=4:ts=4:
 */
