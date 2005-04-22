/* jit/reg.h - register allocator header

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

   $Id: reg.h 2356 2005-04-22 17:33:35Z christian $

*/


#ifndef _REG_H
#define _REG_H

/* #define INVOKE_NEW_DEBUG */
/* #define NEW_MEMORY */


/* preliminary define for testing of the new creation of ARGVAR Stackslots in stack.c */
/* Changes affect handling of ARGVAR Stackslots in reg_of_var in codegen.inc          */
/* and calculation of rd->ifmemuse in reg.inc                                         */

/* We typedef these structures before #includes to resolve circular           */
/* dependencies.                                                              */

typedef struct varinfo varinfo;
typedef struct registerdata registerdata;


#include "types.h"
#include "vm/jit/codegen.inc.h"
#include "vm/jit/jit.h"
#include "vm/jit/inline/inline.h"


/************************* pseudo variable structure **************************/

struct varinfo {
	int type;                   /* basic type of variable                     */
	int flags;                  /* flags (SAVED, INMEMORY)                    */
	int regoff;                 /* register number or memory offset           */
};

typedef struct varinfo varinfo5[5];


struct registerdata {
	varinfo5 *locals;
	varinfo5 *interfaces;

	int intregsnum;                 /* absolute number of integer registers   */
	int fltregsnum;                 /* absolute number of float registers     */

	int intreg_ret;                 /* register to return integer values      */
	int intreg_argnum;              /* number of integer argument registers   */

	int fltreg_ret;                 /* register for return float values       */
	int fltreg_argnum;              /* number of float argument registers     */


	int *argintregs;                /* argument integer registers             */
	int *tmpintregs;                /* scratch integer registers              */
	int *savintregs;                /* saved integer registers                */
	int *argfltregs;                /* argument float registers               */
	int *tmpfltregs;                /* scratch float registers                */
	int *savfltregs;                /* saved float registers                  */
	int *freeargintregs;            /* free argument integer registers        */
	int *freetmpintregs;            /* free scratch integer registers         */
	int *freesavintregs;            /* free saved integer registers           */
	int *freeargfltregs;            /* free argument float registers          */
	int *freetmpfltregs;            /* free scratch float registers           */
	int *freesavfltregs;            /* free saved float registers             */

#ifdef HAS_ADDRESS_REGISTER_FILE
	int adrregsnum;                 /* absolute number of address registers   */
	int adrreg_ret;                 /* register to return address values      */
	int adrreg_argnum;              /* number of address argument registers   */
	int *argadrregs;                /* argument address registers             */
	int *tmpadrregs;                /* scratch address registers              */
	int *savadrregs;               /* saved address registers                */
	int *freeargadrregs;            /* free argument address registers        */
	int *freetmpadrregs;            /* free scratch address registers         */
	int *freesavadrregs;            /* free saved address registers           */

	int tmpadrregcnt;               /* scratch address register count         */
	int savadrregcnt;               /* saved address register count           */
	int iftmpadrregcnt;             /* iface scratch address register count   */
	int ifsavadrregcnt;             /* iface saved address register count     */
	int argadrreguse;               /* used argument address register count   */
	int tmpadrreguse;               /* used scratch address register count    */
	int savadrreguse;               /* used saved address register count      */
	int maxargadrreguse;            /* max used argument address register count */
	int maxtmpadrreguse;            /* max used scratch address register count  */
	int maxsavadrreguse;            /* max used saved address register count  */
	int freetmpadrtop;              /* free scratch address register count    */
	int freesavadrtop;              /* free saved address register count      */
	int ifargadrregcnt;             /* iface argument address register count     */
	int freeargadrtop;              /* free argument address register count      */
#endif

#ifdef SUPPORT_COMBINE_INTEGER_REGISTERS
	int *secondregs;                /* used for longs in 2 32 bit registers   */
#endif

#if defined(NEW_MEMORY) && defined(HAS_4BYTE_STACKSLOT)
	int *freemem_2;
	int freememtop_2;
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
	int ifargintregcnt;             /* iface argument float register count     */
	int ifargfltregcnt;             /* iface argument float register count       */
	int freearginttop;              /* free argument integer register count      */
	int freeargflttop;              /* free argument float register count      */

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
#ifdef STATISTICS
void reg_make_statistics( methodinfo *, codegendata *, registerdata *);
#endif

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
