/* vm/jit/powerpc/arch.h - architecture defines for PowerPC

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser,
   M. Probst, S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck,
   P. Tomsich, J. Wenninger

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

   $Id: arch.h 1648 2004-12-01 16:44:22Z twisti $

*/


#ifndef _ARCH_H
#define _ARCH_H


/* preallocated registers *****************************************************/

/* integer registers */
  
#define REG_RESULT       3   /* to deliver method results                     */
#define REG_RESULT2      4   /* to deliver long method results                */

/*#define REG_RA          26*/  /* return address                                */
#define REG_PV          13   /* procedure vector, must be provided by caller  */
#define REG_METHODPTR   12   /* pointer to the place from where the procedure */
                             /* vector has been fetched                       */
#define REG_ITMP1       11   /* temporary register                            */
#define REG_ITMP2       12   /* temporary register and method pointer         */
#define REG_ITMP3        0   /* temporary register                            */

#define REG_ITMP1_XPTR  11   /* exception pointer = temporary register 1      */
#define REG_ITMP2_XPC   12   /* exception pc = temporary register 2           */

#define REG_SP           1   /* stack pointer                                 */
#define REG_ZERO         0   /* allways zero                                  */

/* floating point registers */

#define REG_FRESULT      1   /* to deliver floating point method results      */
#define REG_FTMP1       16   /* temporary floating point register             */
#define REG_FTMP2       17   /* temporary floating point register             */
#define REG_FTMP3        0   /* temporary floating point register             */

#define REG_IFTMP        0   /* temporary integer and floating point register */


/*#define INT_SAV_CNT     19*/   /* number of int callee saved registers          */
#define INT_ARG_CNT      8   /* number of int argument registers              */

/*#define FLT_SAV_CNT     18*/   /* number of flt callee saved registers          */
#define FLT_ARG_CNT     13   /* number of flt argument registers              */

#define TRACE_ARGS_NUM   8


/* define architecture features ***********************************************/

#define POINTERSIZE         4
#define WORDS_BIGENDIAN     1

#define U8_AVAILABLE        1

#define USE_CODEMMAP        1

#define SUPPORT_DIVISION    0
#define SUPPORT_LONG        1
#define SUPPORT_FLOAT       1
#define SUPPORT_DOUBLE      1
#define SUPPORT_FMOD        0
#define SUPPORT_FICVT       1
#define SUPPORT_IFCVT       0

#define SUPPORT_LONG_ADD    1
#define SUPPORT_LONG_CMP    1
#define SUPPORT_LONG_LOG    1
#define SUPPORT_LONG_SHIFT  0
#define SUPPORT_LONG_MUL    0
#define SUPPORT_LONG_DIV    0
#define SUPPORT_LONG_ICVT   0
#define SUPPORT_LONG_FCVT   0

#define USEBUILTINTABLE

/* #define CONDITIONAL_LOADCONST */
#define NOLONG_CONDITIONAL

/* #define CONSECUTIVE_INTARGS */
#define CONSECUTIVE_FLOATARGS

#define SPECIALMEMUSE
#define USETWOREGS

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
