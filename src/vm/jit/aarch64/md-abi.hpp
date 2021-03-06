/* src/vm/jit/aarch64/md-abi.hpp - defines for aarch64 ABI

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


#ifndef MD_ABI_HPP_
#define MD_ABI_HPP_ 1

/* preallocated registers *****************************************************/

/* integer registers */
  
#define REG_RESULT      0    /* to deliver method results                     */

#define REG_A0          0    /* define some argument registers                */
#define REG_A1          1
#define REG_A2          2
#define REG_A3          3

#define REG_LR          30   /* link register                                 */
#define REG_RA          30   /* return address = link register (bw compat)    */
#define REG_PV          18   /* procedure vector, must be provided by caller  */
#define REG_METHODPTR   10   /* pointer to the place from where the procedure */
                             /* vector has been fetched                       */

#define REG_ITMP1       9    /* temporary register                            */
#define REG_ITMP2       10   /* temporary register                            */
#define REG_ITMP3       11   /* temporary register                            */

#define REG_ITMP1_XPTR  9    /* exception pointer = temporary register 1      */
#define REG_ITMP2_XPC   10   /* exception pc = temporary register 2           */

#define REG_FP          29   /* frame pointer, needed in compiler2            */

#define REG_SP          31   /* stack pointer                                 */
#define REG_ZERO        31   /* always zero                                   */


/* floating point registers */

#define REG_FRESULT     0    /* to deliver floating point method results      */

#define REG_FA0         0    /* define some argument registers                */
#define REG_FA1         1
#define REG_FA2         2

#define REG_FTMP1       16   /* temporary floating point register             */
#define REG_FTMP2       17   /* temporary floating point register             */
#define REG_FTMP3       18   /* temporary floating point register             */

#define REG_IFTMP       28   /* temporary integer and floating point register */


#define INT_REG_CNT     32   /* number of integer registers                   */
#define INT_SAV_CNT     10   /* number of int callee saved registers          */
#define INT_ARG_CNT      8   /* number of int argument registers              */
#define INT_TMP_CNT      6   /* number of int temp registers                  */
#define INT_RES_CNT      7   /* number of reserved integer registers          */
                             /* the one "missing" register is the return reg  */

#define FLT_REG_CNT     32   /* number of float registers                     */
#define FLT_SAV_CNT      8   /* number of flt callee saved registers          */
#define FLT_ARG_CNT      8   /* number of flt argument registers              */
#define FLT_TMP_CNT     13   /* number of flt temp registers                  */
#define FLT_RES_CNT      3   /* number of reserved float registers            */
                             /* the one "missing" register is the return reg  */

#endif // MD_ABI_HPP_


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
