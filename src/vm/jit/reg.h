/* jit/reg.h - register allocator header

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   Institut f. Computersprachen, TU Wien
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser, M. Probst,
   S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich,
   J. Wenninger

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

   $Id: reg.h 1621 2004-11-30 13:06:55Z twisti $

*/


#ifndef _REG_H
#define _REG_H

#include "types.h"
#include "vm/jit/codegen.inc.h"
#include "vm/jit/jit.h"
#include "vm/jit/inline/inline.h"


/************************* pseudo variable structure **************************/

typedef struct varinfo varinfo;

struct varinfo {
	int type;                   /* basic type of variable                     */
	int flags;                  /* flags (SAVED, INMEMORY)                    */
	int regoff;                 /* register number or memory offset           */
};

typedef struct varinfo varinfo5[5];


typedef struct registerdata registerdata;

struct registerdata {
	varinfo5 *locals;
	varinfo5 *interfaces;

	int intregsnum;                 /* absolute number of integer registers   */
	int floatregsnum;               /* absolute number of float registers     */

	int intreg_ret;                 /* register to return integer values      */
	int intreg_argnum;              /* number of integer argument registers   */

	int floatreg_ret;               /* register for return float values       */
	int fltreg_argnum;              /* number of float argument registers     */


	int *argintregs;                /* scratch integer registers              */
	int *tmpintregs;                /* scratch integer registers              */
	int *savintregs;                /* saved integer registers                */
	int *argfltregs;                /* scratch float registers                */
	int *tmpfltregs;                /* scratch float registers                */
	int *savfltregs;                /* saved float registers                  */
	int *freeargintregs;            /* free argument integer registers        */
	int *freetmpintregs;            /* free scratch integer registers         */
	int *freesavintregs;            /* free saved integer registers           */
	int *freeargfltregs;            /* free argument float registers          */
	int *freetmpfltregs;            /* free scratch float registers           */
	int *freesavfltregs;            /* free saved float registers             */

#ifdef USETWOREGS
	int *secondregs;                /* used for longs in 2 32 bit registers   */
#endif

	int *freemem;                   /* free scratch memory                    */
	int memuse;                     /* used memory count                      */
	int ifmemuse;                   /* interface used memory count            */
	int maxmemuse;                  /* maximal used memory count (spills)     */
	int freememtop;                 /* free memory count                      */

	int tmpintregcnt;               /* scratch integer register count         */
	int savintregcnt;               /* saved integer register count           */
	int tmpfltregcnt;               /* scratch float register count           */
	int savfltregcnt;               /* saved float register count             */

	int iftmpintregcnt;             /* iface scratch integer register count   */
	int ifsavintregcnt;             /* iface saved integer register count     */
	int iftmpfltregcnt;             /* iface scratch float register count     */
	int ifsavfltregcnt;             /* iface saved float register count       */

	int argintreguse;               /* used argument integer register count   */
	int tmpintreguse;               /* used scratch integer register count    */
	int savintreguse;               /* used saved integer register count      */
	int argfltreguse;               /* used argument float register count     */
	int tmpfltreguse;               /* used scratch float register count      */
	int savfltreguse;               /* used saved float register count        */

	int maxargintreguse;            /* max used argument int register count   */
	int maxtmpintreguse;            /* max used scratch int register count    */
	int maxsavintreguse;            /* max used saved int register count      */
	int maxargfltreguse;            /* max used argument float register count */
	int maxtmpfltreguse;            /* max used scratch float register count  */
	int maxsavfltreguse;            /* max used saved float register count    */

	int freetmpinttop;              /* free scratch integer register count    */
	int freesavinttop;              /* free saved integer register count      */
	int freetmpflttop;              /* free scratch float register count      */
	int freesavflttop;              /* free saved float register count        */

	int arguments_num;              /* size of parameter field in the stackframe  */
};


/* function prototypes */

void reg_init();
void reg_setup(methodinfo *m, registerdata *rd, t_inlining_globals *id);
void reg_free(methodinfo *m, registerdata *rd);
void reg_close();
void regalloc(methodinfo *m, codegendata *cd, registerdata *rd);

#endif /* _REG_H */


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
