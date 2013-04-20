/* src/vm/jit/reg.hpp - register allocator header

   Copyright (C) 1996-2005, 2006, 2007, 2008
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


#ifndef REG_HPP_
#define REG_HPP_ 1

/* forward typedefs ***********************************************************/

#include "config.h"
#include "vm/types.hpp"

#include "arch.hpp"

#include "vm/jit/verify/typeinfo.hpp"

struct basicblock;

/************************* pseudo variable structure **************************/

struct varinfo {
	s4 type;                   /* basic type of variable                     */
	s4 flags;                  /* flags (SAVED, INMEMORY)                    */
	union {
		s4 regoff;             /* register number or memory offset           */
		s4 i;
		s8 l;
		float f;
		double d;
		basicblock *retaddr;
		s4 ii[2];
	} vv;
#if defined(ENABLE_VERIFIER)
	typeinfo_t typeinfo;       /* type info for reference types              */
#endif
};

typedef struct varinfo varinfo5[5];


struct registerdata {
	int intreg_ret;                 /* register to return integer values      */
	int fltreg_ret;                 /* register for return float values       */

	int *tmpintregs;                /* scratch integer registers              */
	int *savintregs;                /* saved integer registers                */
	int *tmpfltregs;                /* scratch float registers                */
	int *savfltregs;                /* saved float registers                  */
	int *freeargintregs;            /* free argument integer registers        */
	int *freetmpintregs;            /* free scratch integer registers         */
	int *freesavintregs;            /* free saved integer registers           */
	int *freeargfltregs;            /* free argument float registers          */
	int *freetmpfltregs;            /* free scratch float registers           */
	int *freesavfltregs;            /* free saved float registers             */

#ifdef HAS_ADDRESS_REGISTER_FILE
	int adrreg_ret;                 /* register to return address values      */

	int *argadrregs;                /* argument address registers             */
	int *tmpadrregs;                /* scratch address registers              */
	int *savadrregs;                /* saved address registers                */
	int *freeargadrregs;            /* free argument address registers        */
	int *freetmpadrregs;            /* free scratch address registers         */
	int *freesavadrregs;            /* free saved address registers           */

	int *adrusedinout;              /* is this adr register uses as INOUT?    */

	int argadrreguse;               /* used argument address register count   */
	int tmpadrreguse;               /* used scratch address register count    */
	int savadrreguse;               /* used saved address register count      */

	int freetmpadrtop;              /* free scratch address register count    */
	int freesavadrtop;              /* free saved address register count      */
	int freeargadrtop;              /* free argument address register count   */
#endif

	int *freemem;                   /* free scratch memory                    */
	int freememtop;                 /* free memory count                      */

	int memuse;                     /* used memory count                      */

	int argintreguse;               /* used argument integer register count   */
	int tmpintreguse;               /* used scratch integer register count    */
	int savintreguse;               /* used saved integer register count      */
	int argfltreguse;               /* used argument float register count     */
	int tmpfltreguse;               /* used scratch float register count      */
	int savfltreguse;               /* used saved float register count        */

	int freearginttop;              /* free argument integer register count   */
	int freeargflttop;              /* free argument float register count     */
	int freetmpinttop;              /* free scratch integer register count    */
	int freesavinttop;              /* free saved integer register count      */
	int freetmpflttop;              /* free scratch float register count      */
	int freesavflttop;              /* free saved float register count        */

	int *intusedinout;              /* is this int register uses as INOUT?    */
	int *fltusedinout;              /* is this flt register uses as INOUT?    */
	int *regisoutvar;               /* true if reg. is outvar of this block   */
	int *regcopycount;              /* counts copies of each register regoff  */
	int *memcopycount;              /* counts copies of each INMEMORY regoff  */
	int memcopycountsize;           /* size of memcopycount buffer            */
};


/* function prototypes ********************************************************/

#ifdef __cplusplus
extern "C" {
#endif

void reg_setup(jitdata *jd);

#ifdef __cplusplus
}
#endif

#endif // REG_HPP_


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
