/* src/vm/jit/powerpc/linux/md-abi.h - defines for PowerPC Linux ABI

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

   $Id: md-abi.h 2539 2005-05-31 15:55:54Z twisti $

*/


#ifndef _MD_ABI_H
#define _MD_ABI_H

/* preallocated registers *****************************************************/

/* integer registers */
  
#define REG_RESULT       3   /* to deliver method results                     */
#define REG_RESULT2      4   /* to deliver long method results                */

#define REG_PV          13   /* procedure vector, must be provided by caller  */
#define REG_METHODPTR   12   /* pointer to the place from where the procedure */
                             /* vector has been fetched                       */
#define REG_ITMP1       11   /* temporary register                            */
#define REG_ITMP2       12   /* temporary register and method pointer         */
#define REG_ITMP3        0   /* temporary register                            */

#define REG_ITMP1_XPTR  11   /* exception pointer = temporary register 1      */
#define REG_ITMP2_XPC   12   /* exception pc = temporary register 2           */

#define REG_SP           1   /* stack pointer                                 */
#define REG_ZERO         0   /* almost always zero: only in address calc.     */

/* floating point registers */

#define REG_FRESULT      1   /* to deliver floating point method results      */
#define REG_FTMP1       16   /* temporary floating point register             */
#define REG_FTMP2       17   /* temporary floating point register             */
#define REG_FTMP3        0   /* temporary floating point register             */

#define REG_IFTMP        0   /* temporary integer and floating point register */


#define INT_REG_CNT     32   /* number of integer registers                   */
#define INT_SAV_CNT     10   /* number of int callee saved registers          */
#define INT_ARG_CNT      8   /* number of int argument registers              */
#define INT_TMP_CNT      8   /* number of integer temporary registers         */
#define INT_RES_CNT      3   /* number of integer reserved registers          */

#define FLT_REG_CNT     32   /* number of float registers                     */
#define FLT_SAV_CNT     10   /* number of float callee saved registers        */
#define FLT_ARG_CNT      8   /* number of float argument registers            */
#define FLT_TMP_CNT     11   /* number of float temporary registers           */
#define FLT_RES_CNT      3   /* number of float reserved registers            */

#define TRACE_ARGS_NUM   8


/* ABI defines ****************************************************************/

#define LA_SIZE         20   /* linkage area size                             */
#define LA_SIZE_ALIGNED 32   /* linkage area size aligned to 16-byte          */
#define LA_WORD_SIZE     5   /* linkage area size in words: 5 * 4 = 20        */

#define LA_LR_OFFSET     4   /* link register offset in linkage area          */

/* #define ALIGN_FRAME_SIZE(sp)       (sp) */


/* SET_ARG_STACKSLOTS **********************************************************

Macro for stack.c to set Argument Stackslots

Sets the first call_argcount stackslots of curstack to varkind ARGVAR, if
they to not have the SAVEDVAR flag set. According to the calling
conventions these stackslots are assigned argument registers or memory
locations

--- in
i,call_argcount:  Number of arguments for this method
curstack:         instack of the method invokation
call_returntype:  return type

--- uses
i, copy

--- out
copy:             Points to first stackslot after the parameters
rd->argintreguse: max. number of used integer argument register so far
rd->argfltreguse: max. number of used float argument register so far
rd->ifmemuse:     max. number of stackslots used for spilling parameters
                  so far

*******************************************************************************/

#define SET_ARG_STACKSLOTS \
	do { \
		s4 iarg = 0; \
		s4 farg = 0; \
		s4 iarg_max = 0; \
		s4 stacksize; \
		\
		stacksize = 6; \
		\
		copy = curstack; \
		for (;i > 0; i--) { \
			stacksize += (IS_2_WORD_TYPE(copy->type)) ? 2 : 1; \
			if (IS_FLT_DBL_TYPE(copy->type)) \
				farg++; \
			copy = copy->prev; \
		} \
		if (rd->argfltreguse < farg) \
			rd->argfltreguse = farg; \
		\
		/* REG_FRESULT == FLOAT ARGUMENT REGISTER 0 */					\
		if (IS_FLT_DBL_TYPE(call_returntype))							\
			if (rd->argfltreguse < 1)									\
				rd->argfltreguse = 1;									\
																		\
		if (stacksize > rd->ifmemuse)									\
			rd->ifmemuse = stacksize;									\
																		\
		i = call_argcount;												\
		copy = curstack;												\
		while (--i >= 0) {												\
			stacksize -= (IS_2_WORD_TYPE(copy->type)) ? 2 : 1;			\
			if (IS_FLT_DBL_TYPE(copy->type)) {							\
				farg--;													\
				if (!(copy->flags & SAVEDVAR)) {						\
					copy->varnum = i;									\
					copy->varkind = ARGVAR;								\
					if (farg < rd->fltreg_argnum) {						\
						copy->flags = 0;								\
						copy->regoff = rd->argfltregs[farg];			\
					} else {											\
						copy->flags = INMEMORY;							\
						copy->regoff = stacksize;						\
					}													\
				}														\
			} else {													\
				iarg = stacksize - 6;									\
				if (iarg+(IS_2_WORD_TYPE(copy->type) ? 2 : 1) > iarg_max) \
					iarg_max = iarg+(IS_2_WORD_TYPE(copy->type) ? 2 : 1); \
																		\
				if (!(copy->flags & SAVEDVAR)) {						\
					copy->varnum = i;									\
					copy->varkind = ARGVAR;								\
					if ((iarg+((IS_2_WORD_TYPE(copy->type)) ? 1 : 0)) < rd->intreg_argnum) { \
						copy->flags = 0;								\
						copy->regoff = rd->argintregs[iarg];			\
					} else {											\
						copy->flags = INMEMORY;							\
						copy->regoff = stacksize;						\
					} \
				} \
			} \
			copy = copy->prev; \
		} \
		if (rd->argintreguse < iarg_max) \
			rd->argintreguse = iarg_max; \
		if (IS_INT_LNG_TYPE(call_returntype)) { \
			/* REG_RESULT  == INTEGER ARGUMENT REGISTER 0 */ \
			/* REG_RESULT2 == INTEGER ARGUMENT REGISTER 1 */ \
			if (IS_2_WORD_TYPE(call_returntype)) { \
				if (rd->argintreguse < 2) \
					rd->argintreguse = 2; \
			} else { \
				if (rd->argintreguse < 1) \
					rd->argintreguse = 1; \
			} \
		} \
	} while (0)


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
