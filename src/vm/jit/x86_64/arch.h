/* src/vm/jit/x86_64/arch.h - architecture defines for x86_64

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

   $Id: arch.h 2413 2005-04-29 18:54:31Z twisti $

*/


#ifndef _ARCH_H
#define _ARCH_H


/* define registers ***********************************************************/

#define RIP    -1
#define RAX    0
#define RCX    1
#define RDX    2
#define RBX    3
#define RSP    4
#define RBP    5
#define RSI    6
#define RDI    7
#define R8     8
#define R9     9
#define R10    10
#define R11    11
#define R12    12
#define R13    13
#define R14    14
#define R15    15


#define XMM0   0
#define XMM1   1
#define XMM2   2
#define XMM3   3
#define XMM4   4
#define XMM5   5
#define XMM6   6
#define XMM7   7
#define XMM8   8
#define XMM9   9
#define XMM10  10
#define XMM11  11
#define XMM12  12
#define XMM13  13
#define XMM14  14
#define XMM15  15


/* preallocated registers *****************************************************/

/* integer registers */
  
#define REG_RESULT      RAX      /* to deliver method results                 */

#define REG_ITMP1       RAX      /* temporary register                        */
#define REG_ITMP2       R10      /* temporary register and method pointer     */
#define REG_ITMP3       R11      /* temporary register                        */

#define REG_NULL        -1       /* used for reg_of_var where d is not needed */

#define REG_ITMP1_XPTR  RAX      /* exception pointer = temporary register 1  */
#define REG_ITMP2_XPC   R10      /* exception pc = temporary register 2       */

#define REG_SP          RSP      /* stack pointer                             */

/* floating point registers */

#define REG_FRESULT     XMM0     /* to deliver floating point method results  */

#define REG_FTMP1       XMM8     /* temporary floating point register         */
#define REG_FTMP2       XMM9     /* temporary floating point register         */
#define REG_FTMP3       XMM10    /* temporary floating point register         */


#define INT_REG_CNT     16       /* number of integer registers               */
#define INT_SAV_CNT     5        /* number of integer callee saved registers  */
#define INT_ARG_CNT     6        /* number of integer argument registers      */
#define INT_TMP_CNT     1        /* number of integer temporary registers     */
#define INT_RES_CNT     3        /* number of integer reserved registers      */

#define FLT_REG_CNT     16       /* number of float registers                 */
#define FLT_SAV_CNT     0        /* number of float callee saved registers    */
#define FLT_ARG_CNT     8        /* number of float argument registers        */
#define FLT_TMP_CNT     5        /* number of float temporary registers       */
#define FLT_RES_CNT     3        /* number of float reserved registers        */

#define TRACE_ARGS_NUM   6


/* define architecture features ***********************************************/

#define POINTERSIZE                      8
#define WORDS_BIGENDIAN                  0

#define U8_AVAILABLE                     1

#define USE_CODEMMAP                     1

/* #define USEBUILTINTABLE */

#define SUPPORT_DIVISION                 1
#define SUPPORT_LONG                     1
#define SUPPORT_FLOAT                    1
#define SUPPORT_DOUBLE                   1
/* #define SUPPORT_FMOD                    1 */
/* #define SUPPORT_IFCVT                   1 */
/* #define SUPPORT_FICVT                   1 */

#define SUPPORT_LONG_ADD                 1
#define SUPPORT_LONG_CMP                 1
#define SUPPORT_LONG_LOGICAL             1
#define SUPPORT_LONG_SHIFT               1
#define SUPPORT_LONG_MUL                 1
#define SUPPORT_LONG_DIV                 1
#define SUPPORT_LONG_ICVT                1
#define SUPPORT_LONG_FCVT                1

#define SUPPORT_CONST_LOGICAL            1  /* AND, OR, XOR with immediates   */
#define SUPPORT_CONST_MUL                1  /* mutiply with immediate         */

#define SUPPORT_CONST_STORE              0  /* do we support const stores     */
#define SUPPORT_CONST_STORE_ZERO_ONLY    0  /* on some risc machines we can   */
                                            /* only store REG_ZERO            */

#define CONDITIONAL_LOADCONST            1

#define CONSECUTIVE_INTEGER_ARGS
#define CONSECUTIVE_FLOAT_ARGS

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
