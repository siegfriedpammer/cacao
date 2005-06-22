/* src/vm/jit/mips/md-abi.h - defines for MIPS ABI

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

   Changes: Christian Ullrich

   $Id: md-abi.h 2777 2005-06-22 09:51:11Z christian $

*/


#ifndef _MD_ABI_H
#define _MD_ABI_H

#include "config.h"


/* preallocated registers *****************************************************/

/* integer registers */
  
#define REG_ZERO        0    /* always zero                                   */

#define REG_RESULT      2    /* to deliver method results                     */

#define REG_ITMP1       1    /* temporary register                            */
#define REG_ITMP2       3    /* temporary register and method pointer         */
#define REG_ITMP3       25   /* temporary register                            */

#define REG_RA          31   /* return address                                */
#define REG_SP          29   /* stack pointer                                 */
#define REG_GP          28   /* global pointer                                */

#define REG_PV          30   /* procedure vector, must be provided by caller  */
#define REG_METHODPTR   25   /* pointer to the place from where the procedure */
                             /* vector has been fetched                       */
#define REG_ITMP1_XPTR  1    /* exception pointer = temporary register 1      */
#define REG_ITMP2_XPC   3    /* exception pc = temporary register 2           */


/* floating point registers */

#define REG_FRESULT     0    /* to deliver floating point method results      */

#define REG_FTMP1       1    /* temporary floating point register             */
#define REG_FTMP2       2    /* temporary floating point register             */
#define REG_FTMP3       3    /* temporary floating point register             */

#define REG_IFTMP       1    /* temporary integer and floating point register */


#if SIZEOF_VOID_P == 8

/* MIPS64 defines */

#define INT_REG_CNT     32   /* number of integer registers                   */
#define INT_SAV_CNT     8    /* number of int callee saved registers          */
#define INT_ARG_CNT     8    /* number of int argument registers              */
#define INT_TMP_CNT     5    /* number of integer temporary registers         */
#define INT_RES_CNT    10    /* number of integer reserved registers          */
                             /* + 1 REG_RET totals to 32                      */

#define FLT_REG_CNT     32   /* number of float registers                     */
#define FLT_SAV_CNT     4    /* number of flt callee saved registers          */
#define FLT_ARG_CNT     8    /* number of flt argument registers              */
#define FLT_TMP_CNT     16   /* number of float temporary registers           */
#define FLT_RES_CNT     3    /* number of float reserved registers            */
                             /* + 1 REG_RET totals to 32                      */

#define TRACE_ARGS_NUM  8

#else /* SIZEOF_VOID_P == 8 */

/* MIPS32 defines */

#define INT_REG_CNT     32   /* number of integer registers                   */
#define INT_SAV_CNT     8    /* number of int callee saved registers          */
#define INT_ARG_CNT     4    /* number of int argument registers              */
#define INT_TMP_CNT     9    /* number of integer temporary registers         */
#define INT_RES_CNT    10    /* number of integer reserved registers          */
                             /* + 1 REG_RET totals to 32                      */

#if 1

#define FLT_REG_CNT     32   /* number of float registers                     */
#define FLT_SAV_CNT     4    /* number of flt callee saved registers          */
#define FLT_ARG_CNT     8    /* number of flt argument registers              */
#define FLT_TMP_CNT     16   /* number of float temporary registers           */
#define FLT_RES_CNT     3    /* number of float reserved registers            */
                             /* + 1 REG_RET totals to 32                      */

#else

#define FLT_REG_CNT     0    /* number of float registers                     */
#define FLT_SAV_CNT     0    /* number of flt callee saved registers          */
#define FLT_ARG_CNT     0    /* number of flt argument registers              */
#define FLT_TMP_CNT     0    /* number of float temporary registers           */
#define FLT_RES_CNT     0    /* number of float reserved registers            */

#endif

#define TRACE_ARGS_NUM  4

#endif /* SIZEOF_VOID_P == 8 */

#endif /* _MD_ABI_H */


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
