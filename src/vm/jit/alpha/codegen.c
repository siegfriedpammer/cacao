/* src/vm/jit/alpha/codegen.c - machine code generator for Alpha

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

   Authors: Andreas Krall
            Reinhard Grafl

   Changes: Joseph Wenninger
            Christian Thalinger
	    Christian Ullrich

   $Id: codegen.c 2774 2005-06-22 09:47:44Z christian $

*/


#include <stdio.h>

#include "config.h"

#include "md.h"
#include "md-abi.h"
#include "md-abi.inc"

#include "vm/jit/alpha/arch.h"
#include "vm/jit/alpha/codegen.h"
#include "vm/jit/alpha/types.h"
#include "vm/jit/alpha/asmoffsets.h"

#include "cacao/cacao.h"
#include "native/native.h"
#include "vm/builtin.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/stringlocal.h"
#include "vm/tables.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/codegen.inc"
#include "vm/jit/jit.h"

#if defined(LSRA)
# include "vm/jit/lsra.h"
# include "vm/jit/lsra.inc"
#endif

#include "vm/jit/parse.h"
#include "vm/jit/patcher.h"
#include "vm/jit/reg.h"
#include "vm/jit/reg.inc"


/* codegen *********************************************************************

   Generates machine code.

*******************************************************************************/

void codegen(methodinfo *m, codegendata *cd, registerdata *rd)
{
	s4                  len, s1, s2, s3, d;
	ptrint              a;
	s4                  parentargs_base;
	s4                 *mcodeptr;
	stackptr            src;
	varinfo            *var;
	basicblock         *bptr;
	instruction        *iptr;
	exceptiontable     *ex;
	u2                  currentline;
	methodinfo         *lm;             /* local methodinfo for ICMD_INVOKE*  */
	builtintable_entry *bte;
	methoddesc         *md;

	{
	s4 i, p, t, l;
	s4 savedregs_num;

	savedregs_num = (m->isleafmethod) ? 0 : 1;        /* space to save the RA */

	/* space to save used callee saved registers */

	savedregs_num += (INT_SAV_CNT - rd->savintreguse);
	savedregs_num += (FLT_SAV_CNT - rd->savfltreguse);

	parentargs_base = rd->memuse + savedregs_num;

#if defined(USE_THREADS)           /* space to save argument of monitor_enter */

	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		parentargs_base++;

#endif

	/* create method header */

	(void) dseg_addaddress(cd, m);                          /* MethodPointer  */
	(void) dseg_adds4(cd, parentargs_base * 8);             /* FrameSize      */

#if defined(USE_THREADS)

	/* IsSync contains the offset relative to the stack pointer for the
	   argument of monitor_exit used in the exception handler. Since the
	   offset could be zero and give a wrong meaning of the flag it is
	   offset by one.
	*/

	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		(void) dseg_adds4(cd, (rd->memuse + 1) * 8);     /* IsSync         */
	else

#endif

	(void) dseg_adds4(cd, 0);                               /* IsSync         */
	                                       
	(void) dseg_adds4(cd, m->isleafmethod);                 /* IsLeaf         */
	(void) dseg_adds4(cd, INT_SAV_CNT - rd->savintreguse);/* IntSave  */
	(void) dseg_adds4(cd, FLT_SAV_CNT - rd->savfltreguse);/* FltSave  */

	dseg_addlinenumbertablesize(cd);

	(void) dseg_adds4(cd, cd->exceptiontablelength);        /* ExTableSize    */

	/* create exception table */

	for (ex = cd->exceptiontable; ex != NULL; ex = ex->down) {
		dseg_addtarget(cd, ex->start);
   		dseg_addtarget(cd, ex->end);
		dseg_addtarget(cd, ex->handler);
		(void) dseg_addaddress(cd, ex->catchtype.cls);
	}
	
	/* initialize mcode variables */
	
	mcodeptr = (s4 *) cd->mcodebase;
	cd->mcodeend = (s4 *) (cd->mcodebase + cd->mcodesize);
	MCODECHECK(128 + m->paramcount);

	/* create stack frame (if necessary) */

	if (parentargs_base) {
		M_LDA(REG_SP, REG_SP, -parentargs_base * 8);
	}

	/* save return address and used callee saved registers */

	p = parentargs_base;
	if (!m->isleafmethod) {
		p--; M_AST(REG_RA, REG_SP, p * 8);
	}
	for (i = INT_SAV_CNT - 1; i >= rd->savintreguse; i--) {
		p--; M_LST(rd->savintregs[i], REG_SP, p * 8);
	}
	for (i = FLT_SAV_CNT - 1; i >= rd->savfltreguse; i--) {
		p--; M_DST(rd->savfltregs[i], REG_SP, p * 8);
	}

	/* take arguments out of register or stack frame */

	md = m->parseddesc;

 	for (p = 0, l = 0; p < md->paramcount; p++) {
 		t = md->paramtypes[p].type;
 		var = &(rd->locals[l][t]);
 		l++;
 		if (IS_2_WORD_TYPE(t))    /* increment local counter for 2 word types */
 			l++;
 		if (var->type < 0)
 			continue;
		s1 = md->params[p].regoff;
		if (IS_INT_LNG_TYPE(t)) {                    /* integer args          */
 			if (!md->params[p].inmemory) {           /* register arguments    */
				s2 = rd->argintregs[s1];
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
 					M_INTMOVE(s2, var->regoff);

				} else {                             /* reg arg -> spilled    */
 					M_LST(s2, REG_SP, var->regoff * 8);
 				}

			} else {                                 /* stack arguments       */
 				if (!(var->flags & INMEMORY)) {      /* stack arg -> register */
 					M_LLD(var->regoff, REG_SP, (parentargs_base + s1) * 8);

 				} else {                             /* stack arg -> spilled  */
					var->regoff = parentargs_base + s1;
				}
			}

		} else {                                     /* floating args         */
 			if (!md->params[p].inmemory) {           /* register arguments    */
				s2 = rd->argfltregs[s1];
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
 					M_FLTMOVE(s2, var->regoff);

 				} else {			                 /* reg arg -> spilled    */
 					M_DST(s2, REG_SP, var->regoff * 8);
 				}

			} else {                                 /* stack arguments       */
 				if (!(var->flags & INMEMORY)) {      /* stack-arg -> register */
 					M_DLD(var->regoff, REG_SP, (parentargs_base + s1) * 8);

 				} else {                             /* stack-arg -> spilled  */
					var->regoff = parentargs_base + s1;
				}
			}
		}
	} /* end for */

	/* call monitorenter function */

#if defined(USE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		/* stack offset for monitor argument */

		s1 = rd->memuse;

		if (runverbose) {
			M_LDA(REG_SP, REG_SP, -(INT_ARG_CNT + FLT_ARG_CNT) * 8);

			for (p = 0; p < INT_ARG_CNT; p++)
				M_LST(rd->argintregs[p], REG_SP, p * 8);

			for (p = 0; p < FLT_ARG_CNT; p++)
				M_DST(rd->argfltregs[p], REG_SP, (INT_ARG_CNT + p) * 8);

			s1 += INT_ARG_CNT + FLT_ARG_CNT;
		}

		/* decide which monitor enter function to call */

		if (m->flags & ACC_STATIC) {
			p = dseg_addaddress(cd, m->class);
			M_ALD(REG_ITMP1, REG_PV, p);
			M_AST(REG_ITMP1, REG_SP, s1 * 8);
			M_INTMOVE(REG_ITMP1, rd->argintregs[0]);
			p = dseg_addaddress(cd, BUILTIN_staticmonitorenter);
			M_ALD(REG_PV, REG_PV, p);
			M_JSR(REG_RA, REG_PV);
			d = -(s4) ((u1 *) mcodeptr - cd->mcodebase);
			M_LDA(REG_PV, REG_RA, d);

		} else {
			M_BEQZ(rd->argintregs[0], 0);
			codegen_addxnullrefs(cd, mcodeptr);
			M_AST(rd->argintregs[0], REG_SP, s1 * 8);
			p = dseg_addaddress(cd, BUILTIN_monitorenter);
			M_ALD(REG_PV, REG_PV, p);
			M_JSR(REG_RA, REG_PV);
			d = -(s4) ((u1 *) mcodeptr - cd->mcodebase);
			M_LDA(REG_PV, REG_RA, d);
		}

		if (runverbose) {
			for (p = 0; p < INT_ARG_CNT; p++)
				M_LLD(rd->argintregs[p], REG_SP, p * 8);

			for (p = 0; p < FLT_ARG_CNT; p++)
				M_DLD(rd->argfltregs[p], REG_SP, (INT_ARG_CNT + p) * 8);

			M_LDA(REG_SP, REG_SP, (INT_ARG_CNT + FLT_ARG_CNT) * 8);
		}
	}			
#endif

	/* call trace function */

	if (runverbose) {
		M_LDA(REG_SP, REG_SP, -((INT_ARG_CNT + FLT_ARG_CNT + 2) * 8));
		M_AST(REG_RA, REG_SP, 1 * 8);

		/* save integer argument registers */

		for (p = 0; p < md->paramcount && p < INT_ARG_CNT; p++)
			M_LST(rd->argintregs[p], REG_SP, (2 + p) * 8);

		/* save and copy float arguments into integer registers */

		for (p = 0; p < md->paramcount && p < FLT_ARG_CNT; p++) {
			t = md->paramtypes[p].type;

			if (IS_FLT_DBL_TYPE(t)) {
				if (IS_2_WORD_TYPE(t)) {
					M_DST(rd->argfltregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);

				} else {
					M_FST(rd->argfltregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);
				}

				M_LLD(rd->argintregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);
				
			} else {
				M_DST(rd->argfltregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);
			}
		}

		p = dseg_addaddress(cd, m);
		M_ALD(REG_ITMP1, REG_PV, p);
		M_AST(REG_ITMP1, REG_SP, 0 * 8);
		p = dseg_addaddress(cd, (void *) builtin_trace_args);
		M_ALD(REG_PV, REG_PV, p);
		M_JSR(REG_RA, REG_PV);
		d = -(s4) ((u1 *) mcodeptr - cd->mcodebase);
		M_LDA(REG_PV, REG_RA, d);
		M_ALD(REG_RA, REG_SP, 1 * 8);

		/* restore integer argument registers */

		for (p = 0; p < md->paramcount && p < INT_ARG_CNT; p++)
			M_LLD(rd->argintregs[p], REG_SP, (2 + p) * 8);

		/* restore float argument registers */

		for (p = 0; p < md->paramcount && p < FLT_ARG_CNT; p++) {
			t = md->paramtypes[p].type;

			if (IS_FLT_DBL_TYPE(t)) {
				if (IS_2_WORD_TYPE(t)) {
					M_DLD(rd->argfltregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);

				} else {
					M_FLD(rd->argfltregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);
				}

			} else {
				M_DLD(rd->argfltregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);
			}
		}

		M_LDA(REG_SP, REG_SP, (INT_ARG_CNT + FLT_ARG_CNT + 2) * 8);
	}

	}

	/* end of header generation */

	/* walk through all basic blocks */

	for (bptr = m->basicblocks; bptr != NULL; bptr = bptr->next) {

		bptr->mpc = (s4) ((u1 *) mcodeptr - cd->mcodebase);

		if (bptr->flags >= BBREACHED) {

		/* branch resolving */

		{
		branchref *brefs;
		for (brefs = bptr->branchrefs; brefs != NULL; brefs = brefs->next) {
			gen_resolvebranch((u1*) cd->mcodebase + brefs->branchpos, 
			                  brefs->branchpos, bptr->mpc);
			}
		}

		/* copy interface registers to their destination */

		src = bptr->instack;
		len = bptr->indepth;
		MCODECHECK(64+len);
#ifdef LSRA
		if (opt_lsra) {
		while (src != NULL) {
			len--;
			if ((len == 0) && (bptr->type != BBTYPE_STD)) {
					/* 				d = reg_of_var(m, src, REG_ITMP1); */
					if (!(src->flags & INMEMORY))
						d= src->regoff;
					else
						d=REG_ITMP1;
					M_INTMOVE(REG_ITMP1, d);
					store_reg_to_var_int(src, d);
				}
				src = src->prev;
			}
		} else {
#endif
			while (src != NULL) {
				len--;
				if ((len == 0) && (bptr->type != BBTYPE_STD)) {
				d = reg_of_var(rd, src, REG_ITMP1);
				M_INTMOVE(REG_ITMP1, d);
				store_reg_to_var_int(src, d);
				}
			else {
				d = reg_of_var(rd, src, REG_IFTMP);
				if ((src->varkind != STACKVAR)) {
					s2 = src->type;
					if (IS_FLT_DBL_TYPE(s2)) {
						if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
							s1 = rd->interfaces[len][s2].regoff;
							M_FLTMOVE(s1,d);
							}
						else {
							M_DLD(d, REG_SP, 8 * rd->interfaces[len][s2].regoff);
							}
						store_reg_to_var_flt(src, d);
						}
					else {
						if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
							s1 = rd->interfaces[len][s2].regoff;
							M_INTMOVE(s1,d);
							}
						else {
							M_LLD(d, REG_SP, 8 * rd->interfaces[len][s2].regoff);
							}
						store_reg_to_var_int(src, d);
						}
					}
				}
			src = src->prev;
			}
#ifdef LSRA
		}
#endif

		/* walk through all instructions */
		
		src = bptr->instack;
		len = bptr->icount;

		for (iptr = bptr->iinstr; len > 0; src = iptr->dst, len--, iptr++) {
			if (iptr->line != currentline) {
				dseg_addlinenumber(cd, iptr->line, mcodeptr);
				currentline = iptr->line;
			}

		MCODECHECK(64);       /* an instruction usually needs < 64 words      */
		switch (iptr->opc) {

		case ICMD_INLINE_START:
		case ICMD_INLINE_END:
			break;

		case ICMD_NOP:        /* ...  ==> ...                                 */
			break;

		case ICMD_CHECKNULL:  /* ..., objectref  ==> ..., objectref           */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_BEQZ(s1, 0);
			codegen_addxnullrefs(cd, mcodeptr);
			break;

		/* constant operations ************************************************/

		case ICMD_ICONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.i = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			ICONST(d, iptr->val.i);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.l = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			LCONST(d, iptr->val.l);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.f = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			a = dseg_addfloat(cd, iptr->val.f);
			M_FLD(d, REG_PV, a);
			store_reg_to_var_flt(iptr->dst, d);
			break;
			
		case ICMD_DCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.d = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			a = dseg_adddouble(cd, iptr->val.d);
			M_DLD(d, REG_PV, a);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_ACONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.a = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if (iptr->val.a) {
				a = dseg_addaddress(cd, iptr->val.a);
				M_ALD(d, REG_PV, a);
			} else {
				M_INTMOVE(REG_ZERO, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;


		/* load/store operations **********************************************/

		case ICMD_ILOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_LLOAD:      /* op1 = local variable                         */
		case ICMD_ALOAD:

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY)
				M_LLD(d, REG_SP, 8 * var->regoff);
			else
				{M_INTMOVE(var->regoff,d);}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FLOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_DLOAD:      /* op1 = local variable                         */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY)
				M_DLD(d, REG_SP, 8 * var->regoff);
			else
				{M_FLTMOVE(var->regoff,d);}
			store_reg_to_var_flt(iptr->dst, d);
			break;


		case ICMD_ISTORE:     /* ..., value  ==> ...                          */
		case ICMD_LSTORE:     /* op1 = local variable                         */
		case ICMD_ASTORE:

			if ((src->varkind == LOCALVAR) &&
			    (src->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				var_to_reg_int(s1, src, REG_ITMP1);
				M_LST(s1, REG_SP, 8 * var->regoff);
				}
			else {
				var_to_reg_int(s1, src, var->regoff);
				M_INTMOVE(s1, var->regoff);
				}
			break;

		case ICMD_FSTORE:     /* ..., value  ==> ...                          */
		case ICMD_DSTORE:     /* op1 = local variable                         */

			if ((src->varkind == LOCALVAR) &&
			    (src->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				var_to_reg_flt(s1, src, REG_FTMP1);
				M_DST(s1, REG_SP, 8 * var->regoff);
				}
			else {
				var_to_reg_flt(s1, src, var->regoff);
				M_FLTMOVE(s1, var->regoff);
				}
			break;


		/* pop/dup/swap operations ********************************************/

		/* attention: double and longs are only one entry in CACAO ICMDs      */

		case ICMD_POP:        /* ..., value  ==> ...                          */
		case ICMD_POP2:       /* ..., value, value  ==> ...                   */
			break;

		case ICMD_DUP:        /* ..., a ==> ..., a, a                         */
			M_COPY(src, iptr->dst);
			break;

		case ICMD_DUP_X1:     /* ..., a, b ==> ..., b, a, b                   */

			M_COPY(src,       iptr->dst);
			M_COPY(src->prev, iptr->dst->prev);
			M_COPY(iptr->dst, iptr->dst->prev->prev);
			break;

		case ICMD_DUP_X2:     /* ..., a, b, c ==> ..., c, a, b, c             */

			M_COPY(src,             iptr->dst);
			M_COPY(src->prev,       iptr->dst->prev);
			M_COPY(src->prev->prev, iptr->dst->prev->prev);
			M_COPY(iptr->dst,       iptr->dst->prev->prev->prev);
			break;

		case ICMD_DUP2:       /* ..., a, b ==> ..., a, b, a, b                */

			M_COPY(src,       iptr->dst);
			M_COPY(src->prev, iptr->dst->prev);
			break;

		case ICMD_DUP2_X1:    /* ..., a, b, c ==> ..., b, c, a, b, c          */

			M_COPY(src,             iptr->dst);
			M_COPY(src->prev,       iptr->dst->prev);
			M_COPY(src->prev->prev, iptr->dst->prev->prev);
			M_COPY(iptr->dst,       iptr->dst->prev->prev->prev);
			M_COPY(iptr->dst->prev, iptr->dst->prev->prev->prev->prev);
			break;

		case ICMD_DUP2_X2:    /* ..., a, b, c, d ==> ..., c, d, a, b, c, d    */

			M_COPY(src,                   iptr->dst);
			M_COPY(src->prev,             iptr->dst->prev);
			M_COPY(src->prev->prev,       iptr->dst->prev->prev);
			M_COPY(src->prev->prev->prev, iptr->dst->prev->prev->prev);
			M_COPY(iptr->dst,             iptr->dst->prev->prev->prev->prev);
			M_COPY(iptr->dst->prev,       iptr->dst->prev->prev->prev->prev->prev);
			break;

		case ICMD_SWAP:       /* ..., a, b ==> ..., b, a                      */

			M_COPY(src,       iptr->dst->prev);
			M_COPY(src->prev, iptr->dst);
			break;


		/* integer operations *************************************************/

		case ICMD_INEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_int(s1, src, REG_ITMP1); 
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_ISUB(REG_ZERO, s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_LSUB(REG_ZERO, s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_I2L:        /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_IADD(s1, REG_ZERO, d );
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (has_ext_instr_set) {
				M_BSEXT(s1, d);
				}
			else {
				M_SLL_IMM(s1, 56, d);
				M_SRA_IMM( d, 56, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2CHAR:   /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
            M_CZEXT(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2SHORT:  /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (has_ext_instr_set) {
				M_SSEXT(s1, d);
				}
			else {
				M_SLL_IMM(s1, 48, d);
				M_SRA_IMM( d, 48, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_IADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_IADD(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 255)) {
				M_IADD_IMM(s1, iptr->val.i, d);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_IADD(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_LADD(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.l >= 0) && (iptr->val.l <= 255)) {
				M_LADD_IMM(s1, iptr->val.l, d);
				}
			else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_LADD(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_ISUB(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 255)) {
				M_ISUB_IMM(s1, iptr->val.i, d);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_ISUB(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_LSUB(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.l >= 0) && (iptr->val.l <= 255)) {
				M_LSUB_IMM(s1, iptr->val.l, d);
				}
			else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_LSUB(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_IMUL(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 255)) {
				M_IMUL_IMM(s1, iptr->val.i, d);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_IMUL(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_LMUL (s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.l >= 0) && (iptr->val.l <= 255)) {
				M_LMUL_IMM(s1, iptr->val.l, d);
				}
			else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_LMUL(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IDIVPOW2:   /* ..., value  ==> ..., value << constant       */
		case ICMD_LDIVPOW2:   /* val.i = constant                             */
		                      
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr->val.i <= 15) {
				M_LDA(REG_ITMP2, s1, (1 << iptr->val.i) -1);
				M_CMOVGE(s1, s1, REG_ITMP2);
				}
			else {
				M_SRA_IMM(s1, 63, REG_ITMP2);
				M_SRL_IMM(REG_ITMP2, 64 - iptr->val.i, REG_ITMP2);
				M_LADD(s1, REG_ITMP2, REG_ITMP2);
				}
			M_SRA_IMM(REG_ITMP2, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_AND_IMM(s2, 0x1f, REG_ITMP3);
			M_SLL(s1, REG_ITMP3, d);
			M_IADD(d, REG_ZERO, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_SLL_IMM(s1, iptr->val.i & 0x1f, d);
			M_IADD(d, REG_ZERO, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_AND_IMM(s2, 0x1f, REG_ITMP3);
			M_SRA(s1, REG_ITMP3, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_SRA_IMM(s1, iptr->val.i & 0x1f, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_AND_IMM(s2, 0x1f, REG_ITMP2);
            M_IZEXT(s1, d);
			M_SRL(d, REG_ITMP2, d);
			M_IADD(d, REG_ZERO, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
            M_IZEXT(s1, d);
			M_SRL_IMM(d, iptr->val.i & 0x1f, d);
			M_IADD(d, REG_ZERO, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_SLL(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_SLL_IMM(s1, iptr->val.i & 0x3f, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_SRA(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_SRA_IMM(s1, iptr->val.i & 0x3f, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_SRL(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_SRL_IMM(s1, iptr->val.i & 0x3f, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */
		case ICMD_LAND:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_AND(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 255)) {
				M_AND_IMM(s1, iptr->val.i, d);
				}
			else if (iptr->val.i == 0xffff) {
				M_CZEXT(s1, d);
				}
			else if (iptr->val.i == 0xffffff) {
				M_ZAPNOT_IMM(s1, 0x07, d);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_AND(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (s1 == d) {
				M_MOV(s1, REG_ITMP1);
				s1 = REG_ITMP1;
				}
			if ((iptr->val.i >= 0) && (iptr->val.i <= 255)) {
				M_AND_IMM(s1, iptr->val.i, d);
				M_BGEZ(s1, 3);
				M_ISUB(REG_ZERO, s1, d);
				M_AND_IMM(d, iptr->val.i, d);
				}
			else if (iptr->val.i == 0xffff) {
				M_CZEXT(s1, d);
				M_BGEZ(s1, 3);
				M_ISUB(REG_ZERO, s1, d);
				M_CZEXT(d, d);
				}
			else if (iptr->val.i == 0xffffff) {
				M_ZAPNOT_IMM(s1, 0x07, d);
				M_BGEZ(s1, 3);
				M_ISUB(REG_ZERO, s1, d);
				M_ZAPNOT_IMM(d, 0x07, d);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_AND(s1, REG_ITMP2, d);
				M_BGEZ(s1, 3);
				M_ISUB(REG_ZERO, s1, d);
				M_AND(d, REG_ITMP2, d);
				}
			M_ISUB(REG_ZERO, d, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.l >= 0) && (iptr->val.l <= 255)) {
				M_AND_IMM(s1, iptr->val.l, d);
				}
			else if (iptr->val.l == 0xffffL) {
				M_CZEXT(s1, d);
				}
			else if (iptr->val.l == 0xffffffL) {
				M_ZAPNOT_IMM(s1, 0x07, d);
				}
			else if (iptr->val.l == 0xffffffffL) {
				M_IZEXT(s1, d);
				}
			else if (iptr->val.l == 0xffffffffffL) {
				M_ZAPNOT_IMM(s1, 0x1f, d);
				}
			else if (iptr->val.l == 0xffffffffffffL) {
				M_ZAPNOT_IMM(s1, 0x3f, d);
				}
			else if (iptr->val.l == 0xffffffffffffffL) {
				M_ZAPNOT_IMM(s1, 0x7f, d);
				}
			else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_AND(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (s1 == d) {
				M_MOV(s1, REG_ITMP1);
				s1 = REG_ITMP1;
				}
			if ((iptr->val.l >= 0) && (iptr->val.l <= 255)) {
				M_AND_IMM(s1, iptr->val.l, d);
				M_BGEZ(s1, 3);
				M_LSUB(REG_ZERO, s1, d);
				M_AND_IMM(d, iptr->val.l, d);
				}
			else if (iptr->val.l == 0xffffL) {
				M_CZEXT(s1, d);
				M_BGEZ(s1, 3);
				M_LSUB(REG_ZERO, s1, d);
				M_CZEXT(d, d);
				}
			else if (iptr->val.l == 0xffffffL) {
				M_ZAPNOT_IMM(s1, 0x07, d);
				M_BGEZ(s1, 3);
				M_LSUB(REG_ZERO, s1, d);
				M_ZAPNOT_IMM(d, 0x07, d);
				}
			else if (iptr->val.l == 0xffffffffL) {
				M_IZEXT(s1, d);
				M_BGEZ(s1, 3);
				M_LSUB(REG_ZERO, s1, d);
				M_IZEXT(d, d);
				}
			else if (iptr->val.l == 0xffffffffffL) {
 				M_ZAPNOT_IMM(s1, 0x1f, d);
				M_BGEZ(s1, 3);
				M_LSUB(REG_ZERO, s1, d);
				M_ZAPNOT_IMM(d, 0x1f, d);
				}
			else if (iptr->val.l == 0xffffffffffffL) {
				M_ZAPNOT_IMM(s1, 0x3f, d);
				M_BGEZ(s1, 3);
				M_LSUB(REG_ZERO, s1, d);
				M_ZAPNOT_IMM(d, 0x3f, d);
				}
			else if (iptr->val.l == 0xffffffffffffffL) {
				M_ZAPNOT_IMM(s1, 0x7f, d);
				M_BGEZ(s1, 3);
				M_LSUB(REG_ZERO, s1, d);
				M_ZAPNOT_IMM(d, 0x7f, d);
				}
			else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_AND(s1, REG_ITMP2, d);
				M_BGEZ(s1, 3);
				M_LSUB(REG_ZERO, s1, d);
				M_AND(d, REG_ITMP2, d);
				}
			M_LSUB(REG_ZERO, d, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IOR:        /* ..., val1, val2  ==> ..., val1 | val2        */
		case ICMD_LOR:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_OR( s1,s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 255)) {
				M_OR_IMM(s1, iptr->val.i, d);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_OR(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.l >= 0) && (iptr->val.l <= 255)) {
				M_OR_IMM(s1, iptr->val.l, d);
				}
			else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_OR(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */
		case ICMD_LXOR:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_XOR(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 255)) {
				M_XOR_IMM(s1, iptr->val.i, d);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_XOR(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.l >= 0) && (iptr->val.l <= 255)) {
				M_XOR_IMM(s1, iptr->val.l, d);
				}
			else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_XOR(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_LCMP:       /* ..., val1, val2  ==> ..., val1 cmp val2      */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_CMPLT(s1, s2, REG_ITMP3);
			M_CMPLT(s2, s1, REG_ITMP1);
			M_LSUB (REG_ITMP1, REG_ITMP3, d);
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_IINC:       /* ..., value  ==> ..., value + constant        */
		                      /* op1 = variable, val.i = constant             */

			var = &(rd->locals[iptr->op1][TYPE_INT]);
			if (var->flags & INMEMORY) {
				s1 = REG_ITMP1;
				M_LLD(s1, REG_SP, 8 * var->regoff);
				}
			else
				s1 = var->regoff;
			if ((iptr->val.i >= 0) && (iptr->val.i <= 255)) {
				M_IADD_IMM(s1, iptr->val.i, s1);
				}
			else if ((iptr->val.i > -256) && (iptr->val.i < 0)) {
				M_ISUB_IMM(s1, (-iptr->val.i), s1);
				}
			else {
				M_LDA (s1, s1, iptr->val.i);
				M_IADD(s1, REG_ZERO, s1);
				}
			if (var->flags & INMEMORY)
				M_LST(s1, REG_SP, 8 * var->regoff);
			break;


		/* floating operations ************************************************/

		case ICMD_FNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_FMOVN(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_FMOVN(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (opt_noieee) {
				M_FADD(s1, s2, d);
				}
			else {
				if (d == s1 || d == s2) {
					M_FADDS(s1, s2, REG_FTMP3);
					M_TRAPB;
					M_FMOV(REG_FTMP3, d);
					}
				else {
					M_FADDS(s1, s2, d);
					M_TRAPB;
					}
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (opt_noieee) {
				M_DADD(s1, s2, d);
				}
			else {
				if (d == s1 || d == s2) {
					M_DADDS(s1, s2, REG_FTMP3);
					M_TRAPB;
					M_FMOV(REG_FTMP3, d);
					}
				else {
					M_DADDS(s1, s2, d);
					M_TRAPB;
					}
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (opt_noieee) {
				M_FSUB(s1, s2, d);
				}
			else {
				if (d == s1 || d == s2) {
					M_FSUBS(s1, s2, REG_FTMP3);
					M_TRAPB;
					M_FMOV(REG_FTMP3, d);
					}
				else {
					M_FSUBS(s1, s2, d);
					M_TRAPB;
					}
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (opt_noieee) {
				M_DSUB(s1, s2, d);
				}
			else {
				if (d == s1 || d == s2) {
					M_DSUBS(s1, s2, REG_FTMP3);
					M_TRAPB;
					M_FMOV(REG_FTMP3, d);
					}
				else {
					M_DSUBS(s1, s2, d);
					M_TRAPB;
					}
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (opt_noieee) {
				M_FMUL(s1, s2, d);
				}
			else {
				if (d == s1 || d == s2) {
					M_FMULS(s1, s2, REG_FTMP3);
					M_TRAPB;
					M_FMOV(REG_FTMP3, d);
					}
				else {
					M_FMULS(s1, s2, d);
					M_TRAPB;
					}
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DMUL:       /* ..., val1, val2  ==> ..., val1 *** val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (opt_noieee) {
				M_DMUL(s1, s2, d);
				}
			else {
				if (d == s1 || d == s2) {
					M_DMULS(s1, s2, REG_FTMP3);
					M_TRAPB;
					M_FMOV(REG_FTMP3, d);
					}
				else {
					M_DMULS(s1, s2, d);
					M_TRAPB;
					}
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (opt_noieee) {
				M_FDIV(s1, s2, d);
				}
			else {
				if (d == s1 || d == s2) {
					M_FDIVS(s1, s2, REG_FTMP3);
					M_TRAPB;
					M_FMOV(REG_FTMP3, d);
					}
				else {
					M_FDIVS(s1, s2, d);
					M_TRAPB;
					}
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (opt_noieee) {
				M_DDIV(s1, s2, d);
				}
			else {
				if (d == s1 || d == s2) {
					M_DDIVS(s1, s2, REG_FTMP3);
					M_TRAPB;
					M_FMOV(REG_FTMP3, d);
					}
				else {
					M_DDIVS(s1, s2, d);
					M_TRAPB;
					}
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;
		
		case ICMD_I2F:       /* ..., value  ==> ..., (float) value            */
		case ICMD_L2F:
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			a = dseg_adddouble(cd, 0.0);
			M_LST (s1, REG_PV, a);
			M_DLD (d, REG_PV, a);
			M_CVTLF(d, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_I2D:       /* ..., value  ==> ..., (double) value           */
		case ICMD_L2D:
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			a = dseg_adddouble(cd, 0.0);
			M_LST (s1, REG_PV, a);
			M_DLD (d, REG_PV, a);
			M_CVTLD(d, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
			
		case ICMD_F2I:       /* ..., value  ==> ..., (int) value              */
		case ICMD_D2I:
			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			a = dseg_adddouble(cd, 0.0);
			M_CVTDL_C(s1, REG_FTMP2);
			M_CVTLI(REG_FTMP2, REG_FTMP3);
			M_DST (REG_FTMP3, REG_PV, a);
			M_ILD (d, REG_PV, a);
			store_reg_to_var_int(iptr->dst, d);
			break;
		
		case ICMD_F2L:       /* ..., value  ==> ..., (long) value             */
		case ICMD_D2L:
			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			a = dseg_adddouble(cd, 0.0);
			M_CVTDL_C(s1, REG_FTMP2);
			M_DST (REG_FTMP2, REG_PV, a);
			M_LLD (d, REG_PV, a);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_F2D:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_CVTFDS(s1, d);
			M_TRAPB;
			store_reg_to_var_flt(iptr->dst, d);
			break;
					
		case ICMD_D2F:       /* ..., value  ==> ..., (float) value            */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (opt_noieee) {
				M_CVTDF(s1, d);
				}
			else {
				M_CVTDFS(s1, d);
				M_TRAPB;
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;
		
		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */
		case ICMD_DCMPL:
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (opt_noieee) {
				M_LSUB_IMM(REG_ZERO, 1, d);
				M_FCMPEQ(s1, s2, REG_FTMP3);
				M_FBEQZ (REG_FTMP3, 1);        /* jump over next instructions */
				M_CLR   (d);
				M_FCMPLT(s2, s1, REG_FTMP3);
				M_FBEQZ (REG_FTMP3, 1);        /* jump over next instruction  */
				M_LADD_IMM(REG_ZERO, 1, d);
				}
			else {
				M_LSUB_IMM(REG_ZERO, 1, d);
				M_FCMPEQS(s1, s2, REG_FTMP3);
				M_TRAPB;
				M_FBEQZ (REG_FTMP3, 1);        /* jump over next instructions */
				M_CLR   (d);
				M_FCMPLTS(s2, s1, REG_FTMP3);
				M_TRAPB;
				M_FBEQZ (REG_FTMP3, 1);        /* jump over next instruction  */
				M_LADD_IMM(REG_ZERO, 1, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;
			
		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */
		case ICMD_DCMPG:
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (opt_noieee) {
				M_LADD_IMM(REG_ZERO, 1, d);
				M_FCMPEQ(s1, s2, REG_FTMP3);
				M_FBEQZ (REG_FTMP3, 1);        /* jump over next instruction  */
				M_CLR   (d);
				M_FCMPLT(s1, s2, REG_FTMP3);
				M_FBEQZ (REG_FTMP3, 1);        /* jump over next instruction  */
				M_LSUB_IMM(REG_ZERO, 1, d);
				}
			else {
				M_LADD_IMM(REG_ZERO, 1, d);
				M_FCMPEQS(s1, s2, REG_FTMP3);
				M_TRAPB;
				M_FBEQZ (REG_FTMP3, 1);        /* jump over next instruction  */
				M_CLR   (d);
				M_FCMPLTS(s1, s2, REG_FTMP3);
				M_TRAPB;
				M_FBEQZ (REG_FTMP3, 1);        /* jump over next instruction  */
				M_LSUB_IMM(REG_ZERO, 1, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;


		/* memory operations **************************************************/

		case ICMD_ARRAYLENGTH: /* ..., arrayref  ==> ..., length              */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			gen_nullptr_check(s1);
			M_ILD(d, s1, OFFSET(java_arrayheader, size));
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_AALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			M_SAADDQ(s2, s1, REG_ITMP1);
			M_ALD( d, REG_ITMP1, OFFSET(java_objectarray, data[0]));
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			M_S8ADDQ(s2, s1, REG_ITMP1);
			M_LLD(d, REG_ITMP1, OFFSET(java_longarray, data[0]));
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
		  
			M_S4ADDQ(s2, s1, REG_ITMP1);
			M_ILD(d, REG_ITMP1, OFFSET(java_intarray, data[0]));
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			M_S4ADDQ(s2, s1, REG_ITMP1);
			M_FLD(d, REG_ITMP1, OFFSET(java_floatarray, data[0]));
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			M_S8ADDQ(s2, s1, REG_ITMP1);
			M_DLD(d, REG_ITMP1, OFFSET(java_doublearray, data[0]));
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_CALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			if (has_ext_instr_set) {
				M_LADD(s2, s1, REG_ITMP1);
				M_LADD(s2, REG_ITMP1, REG_ITMP1);
				M_SLDU(d, REG_ITMP1, OFFSET(java_chararray, data[0]));
				}
			else {
				M_LADD (s2, s1, REG_ITMP1);
				M_LADD (s2, REG_ITMP1, REG_ITMP1);
				M_LLD_U(REG_ITMP2, REG_ITMP1, OFFSET(java_chararray, data[0]));
				M_LDA  (REG_ITMP1, REG_ITMP1, OFFSET(java_chararray, data[0]));
				M_EXTWL(REG_ITMP2, REG_ITMP1, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;			

		case ICMD_SALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			if (has_ext_instr_set) {
				M_LADD(s2, s1, REG_ITMP1);
				M_LADD(s2, REG_ITMP1, REG_ITMP1);
				M_SLDU( d, REG_ITMP1, OFFSET (java_shortarray, data[0]));
				M_SSEXT(d, d);
				}
			else {
				M_LADD(s2, s1, REG_ITMP1);
				M_LADD(s2, REG_ITMP1, REG_ITMP1);
				M_LLD_U(REG_ITMP2, REG_ITMP1, OFFSET(java_shortarray, data[0]));
				M_LDA(REG_ITMP1, REG_ITMP1, OFFSET(java_shortarray, data[0])+2);
				M_EXTQH(REG_ITMP2, REG_ITMP1, d);
				M_SRA_IMM(d, 48, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_BALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			if (has_ext_instr_set) {
				M_LADD   (s2, s1, REG_ITMP1);
				M_BLDU   (d, REG_ITMP1, OFFSET (java_bytearray, data[0]));
				M_BSEXT  (d, d);
				}
			else {
				M_LADD(s2, s1, REG_ITMP1);
				M_LLD_U(REG_ITMP2, REG_ITMP1, OFFSET(java_bytearray, data[0]));
				M_LDA(REG_ITMP1, REG_ITMP1, OFFSET(java_bytearray, data[0])+1);
				M_EXTQH(REG_ITMP2, REG_ITMP1, d);
				M_SRA_IMM(d, 56, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_AASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_int(s3, src, REG_ITMP3);
			M_SAADDQ(s2, s1, REG_ITMP1);
			M_AST   (s3, REG_ITMP1, OFFSET(java_objectarray, data[0]));
			break;

		case ICMD_LASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_int(s3, src, REG_ITMP3);
			M_S8ADDQ(s2, s1, REG_ITMP1);
			M_LST   (s3, REG_ITMP1, OFFSET(java_longarray, data[0]));
			break;

		case ICMD_IASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}

			var_to_reg_int(s3, src, REG_ITMP3);
			M_S4ADDQ(s2, s1, REG_ITMP1);
			M_IST   (s3, REG_ITMP1, OFFSET(java_intarray, data[0]));
			break;

		case ICMD_FASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_flt(s3, src, REG_FTMP3);
			M_S4ADDQ(s2, s1, REG_ITMP1);
			M_FST   (s3, REG_ITMP1, OFFSET(java_floatarray, data[0]));
			break;

		case ICMD_DASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_flt(s3, src, REG_FTMP3);
			M_S8ADDQ(s2, s1, REG_ITMP1);
			M_DST   (s3, REG_ITMP1, OFFSET(java_doublearray, data[0]));
			break;

		case ICMD_CASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_int(s3, src, REG_ITMP3);
			if (has_ext_instr_set) {
				M_LADD(s2, s1, REG_ITMP1);
				M_LADD(s2, REG_ITMP1, REG_ITMP1);
				M_SST (s3, REG_ITMP1, OFFSET(java_chararray, data[0]));
				}
			else {
				M_LADD (s2, s1, REG_ITMP1);
				M_LADD (s2, REG_ITMP1, REG_ITMP1);
				M_LLD_U(REG_ITMP2, REG_ITMP1, OFFSET(java_chararray, data[0]));
				M_LDA  (REG_ITMP1, REG_ITMP1, OFFSET(java_chararray, data[0]));
				M_INSWL(s3, REG_ITMP1, REG_ITMP3);
				M_MSKWL(REG_ITMP2, REG_ITMP1, REG_ITMP2);
				M_OR   (REG_ITMP2, REG_ITMP3, REG_ITMP2);
				M_LST_U(REG_ITMP2, REG_ITMP1, 0);
				}
			break;

		case ICMD_SASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_int(s3, src, REG_ITMP3);
			if (has_ext_instr_set) {
				M_LADD(s2, s1, REG_ITMP1);
				M_LADD(s2, REG_ITMP1, REG_ITMP1);
				M_SST (s3, REG_ITMP1, OFFSET(java_shortarray, data[0]));
				}
			else {
				M_LADD (s2, s1, REG_ITMP1);
				M_LADD (s2, REG_ITMP1, REG_ITMP1);
				M_LLD_U(REG_ITMP2, REG_ITMP1, OFFSET(java_shortarray, data[0]));
				M_LDA  (REG_ITMP1, REG_ITMP1, OFFSET(java_shortarray, data[0]));
				M_INSWL(s3, REG_ITMP1, REG_ITMP3);
				M_MSKWL(REG_ITMP2, REG_ITMP1, REG_ITMP2);
				M_OR   (REG_ITMP2, REG_ITMP3, REG_ITMP2);
				M_LST_U(REG_ITMP2, REG_ITMP1, 0);
				}
			break;

		case ICMD_BASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_int(s3, src, REG_ITMP3);
			if (has_ext_instr_set) {
				M_LADD(s2, s1, REG_ITMP1);
				M_BST (s3, REG_ITMP1, OFFSET(java_bytearray, data[0]));
				}
			else {
				M_LADD (s2, s1, REG_ITMP1);
				M_LLD_U(REG_ITMP2, REG_ITMP1, OFFSET(java_bytearray, data[0]));
				M_LDA  (REG_ITMP1, REG_ITMP1, OFFSET(java_bytearray, data[0]));
				M_INSBL(s3, REG_ITMP1, REG_ITMP3);
				M_MSKBL(REG_ITMP2, REG_ITMP1, REG_ITMP2);
				M_OR   (REG_ITMP2, REG_ITMP3, REG_ITMP2);
				M_LST_U(REG_ITMP2, REG_ITMP1, 0);
				}
			break;


		case ICMD_IASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_S4ADDQ(s2, s1, REG_ITMP1);
			M_IST(REG_ZERO, REG_ITMP1, OFFSET(java_intarray, data[0]));
			break;

		case ICMD_LASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_S8ADDQ(s2, s1, REG_ITMP1);
			M_LST(REG_ZERO, REG_ITMP1, OFFSET(java_longarray, data[0]));
			break;

		case ICMD_AASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_SAADDQ(s2, s1, REG_ITMP1);
			M_AST(REG_ZERO, REG_ITMP1, OFFSET(java_objectarray, data[0]));
			break;

		case ICMD_BASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			if (has_ext_instr_set) {
				M_LADD(s2, s1, REG_ITMP1);
				M_BST(REG_ZERO, REG_ITMP1, OFFSET(java_bytearray, data[0]));

			} else {
				M_LADD(s2, s1, REG_ITMP1);
				M_LLD_U(REG_ITMP2, REG_ITMP1, OFFSET(java_bytearray, data[0]));
				M_LDA(REG_ITMP1, REG_ITMP1, OFFSET(java_bytearray, data[0]));
				M_INSBL(REG_ZERO, REG_ITMP1, REG_ITMP3);
				M_MSKBL(REG_ITMP2, REG_ITMP1, REG_ITMP2);
				M_OR(REG_ITMP2, REG_ITMP3, REG_ITMP2);
				M_LST_U(REG_ITMP2, REG_ITMP1, 0);
			}
			break;

		case ICMD_CASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			if (has_ext_instr_set) {
				M_LADD(s2, s1, REG_ITMP1);
				M_LADD(s2, REG_ITMP1, REG_ITMP1);
				M_SST(REG_ZERO, REG_ITMP1, OFFSET(java_chararray, data[0]));

			} else {
				M_LADD(s2, s1, REG_ITMP1);
				M_LADD(s2, REG_ITMP1, REG_ITMP1);
				M_LLD_U(REG_ITMP2, REG_ITMP1, OFFSET(java_chararray, data[0]));
				M_LDA(REG_ITMP1, REG_ITMP1, OFFSET(java_chararray, data[0]));
				M_INSWL(REG_ZERO, REG_ITMP1, REG_ITMP3);
				M_MSKWL(REG_ITMP2, REG_ITMP1, REG_ITMP2);
				M_OR(REG_ITMP2, REG_ITMP3, REG_ITMP2);
				M_LST_U(REG_ITMP2, REG_ITMP1, 0);
			}
			break;

		case ICMD_SASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			if (has_ext_instr_set) {
				M_LADD(s2, s1, REG_ITMP1);
				M_LADD(s2, REG_ITMP1, REG_ITMP1);
				M_SST(REG_ZERO, REG_ITMP1, OFFSET(java_shortarray, data[0]));

			} else {
				M_LADD(s2, s1, REG_ITMP1);
				M_LADD(s2, REG_ITMP1, REG_ITMP1);
				M_LLD_U(REG_ITMP2, REG_ITMP1, OFFSET(java_shortarray, data[0]));
				M_LDA(REG_ITMP1, REG_ITMP1, OFFSET(java_shortarray, data[0]));
				M_INSWL(REG_ZERO, REG_ITMP1, REG_ITMP3);
				M_MSKWL(REG_ITMP2, REG_ITMP1, REG_ITMP2);
				M_OR(REG_ITMP2, REG_ITMP3, REG_ITMP2);
				M_LST_U(REG_ITMP2, REG_ITMP1, 0);
			}
			break;


		case ICMD_GETSTATIC:  /* ...  ==> ..., value                          */
		                      /* op1 = type, val.a = field address            */

			if (!iptr->val.a) {
				codegen_addpatchref(cd, mcodeptr,
									PATCHER_get_putstatic,
									(unresolved_field *) iptr->target);

				if (showdisassemble)
					M_NOP;

				a = 0;

			} else {
				fieldinfo *fi = iptr->val.a;

				if (!fi->class->initialized) {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_clinit, fi->class);

					if (showdisassemble)
						M_NOP;
				}

				a = (ptrint) &(fi->value);
  			}

			a = dseg_addaddress(cd, a);
			M_ALD(REG_ITMP1, REG_PV, a);
			switch (iptr->op1) {
			case TYPE_INT:
				d = reg_of_var(rd, iptr->dst, REG_ITMP3);
				M_ILD(d, REG_ITMP1, 0);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_LNG:
				d = reg_of_var(rd, iptr->dst, REG_ITMP3);
				M_LLD(d, REG_ITMP1, 0);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_ADR:
				d = reg_of_var(rd, iptr->dst, REG_ITMP3);
				M_ALD(d, REG_ITMP1, 0);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_FLT:
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				M_FLD(d, REG_ITMP1, 0);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			case TYPE_DBL:				
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				M_DLD(d, REG_ITMP1, 0);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			}
			break;

		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */
		                      /* op1 = type, val.a = field address            */

			if (!iptr->val.a) {
				codegen_addpatchref(cd, mcodeptr,
									PATCHER_get_putstatic,
									(unresolved_field *) iptr->target);

				if (showdisassemble)
					M_NOP;

				a = 0;

			} else {
				fieldinfo *fi = iptr->val.a;

				if (!fi->class->initialized) {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_clinit, fi->class);

					if (showdisassemble)
						M_NOP;
				}

				a = (ptrint) &(fi->value);
  			}

			a = dseg_addaddress(cd, a);
			M_ALD(REG_ITMP1, REG_PV, a);
			switch (iptr->op1) {
			case TYPE_INT:
				var_to_reg_int(s2, src, REG_ITMP2);
				M_IST(s2, REG_ITMP1, 0);
				break;
			case TYPE_LNG:
				var_to_reg_int(s2, src, REG_ITMP2);
				M_LST(s2, REG_ITMP1, 0);
				break;
			case TYPE_ADR:
				var_to_reg_int(s2, src, REG_ITMP2);
				M_AST(s2, REG_ITMP1, 0);
				break;
			case TYPE_FLT:
				var_to_reg_flt(s2, src, REG_FTMP2);
				M_FST(s2, REG_ITMP1, 0);
				break;
			case TYPE_DBL:
				var_to_reg_flt(s2, src, REG_FTMP2);
				M_DST(s2, REG_ITMP1, 0);
				break;
			}
			break;

		case ICMD_PUTSTATICCONST: /* ...  ==> ...                             */
		                          /* val = value (in current instruction)     */
		                          /* op1 = type, val.a = field address (in    */
		                          /* following NOP)                           */

			if (!iptr[1].val.a) {
				codegen_addpatchref(cd, mcodeptr,
									PATCHER_get_putstatic,
									(unresolved_field *) iptr[1].target);

				if (showdisassemble)
					M_NOP;

				a = 0;

			} else {
				fieldinfo *fi = iptr[1].val.a;

				if (!fi->class->initialized) {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_clinit, fi->class);

					if (showdisassemble)
						M_NOP;
				}

				a = (ptrint) &(fi->value);
  			}
			
			a = dseg_addaddress(cd, a);
			M_ALD(REG_ITMP1, REG_PV, a);
			switch (iptr->op1) {
			case TYPE_INT:
				M_IST(REG_ZERO, REG_ITMP1, 0);
				break;
			case TYPE_LNG:
				M_LST(REG_ZERO, REG_ITMP1, 0);
				break;
			case TYPE_ADR:
				M_AST(REG_ZERO, REG_ITMP1, 0);
				break;
			case TYPE_FLT:
				M_FST(REG_ZERO, REG_ITMP1, 0);
				break;
			case TYPE_DBL:
				M_DST(REG_ZERO, REG_ITMP1, 0);
				break;
			}
			break;


		case ICMD_GETFIELD:   /* ...  ==> ..., value                          */
		                      /* op1 = type, val.i = field offset             */

			var_to_reg_int(s1, src, REG_ITMP1);
			gen_nullptr_check(s1);

			if (!iptr->val.a) {
				codegen_addpatchref(cd, mcodeptr,
									PATCHER_get_putfield,
									(unresolved_field *) iptr->target);

				if (showdisassemble)
					M_NOP;

				a = 0;

			} else {
				a = ((fieldinfo *) (iptr->val.a))->offset;
			}

			switch (iptr->op1) {
			case TYPE_INT:
				d = reg_of_var(rd, iptr->dst, REG_ITMP3);
				M_ILD(d, s1, a);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_LNG:
				d = reg_of_var(rd, iptr->dst, REG_ITMP3);
				M_LLD(d, s1, a);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_ADR:
				d = reg_of_var(rd, iptr->dst, REG_ITMP3);
				M_ALD(d, s1, a);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_FLT:
				d = reg_of_var(rd, iptr->dst, REG_FTMP3);
				M_FLD(d, s1, a);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			case TYPE_DBL:				
				d = reg_of_var(rd, iptr->dst, REG_FTMP3);
				M_DLD(d, s1, a);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			}
			break;

		case ICMD_PUTFIELD:   /* ..., objectref, value  ==> ...               */
		                      /* op1 = type, val.a = field address            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			gen_nullptr_check(s1);

			if (!IS_FLT_DBL_TYPE(iptr->op1)) {
				var_to_reg_int(s2, src, REG_ITMP2);
			} else {
				var_to_reg_flt(s2, src, REG_FTMP2);
			}

			if (!iptr->val.a) {
				codegen_addpatchref(cd, mcodeptr,
									PATCHER_get_putfield,
									(unresolved_field *) iptr->target);

				if (showdisassemble)
					M_NOP;

				a = 0;

			} else {
				a = ((fieldinfo *) (iptr->val.a))->offset;
			}

			switch (iptr->op1) {
			case TYPE_INT:
				M_IST(s2, s1, a);
				break;
			case TYPE_LNG:
				M_LST(s2, s1, a);
				break;
			case TYPE_ADR:
				M_AST(s2, s1, a);
				break;
			case TYPE_FLT:
				M_FST(s2, s1, a);
				break;
			case TYPE_DBL:
				M_DST(s2, s1, a);
				break;
			}
			break;

		case ICMD_PUTFIELDCONST:  /* ..., objectref  ==> ...                  */
		                          /* val = value (in current instruction)     */
		                          /* op1 = type, val.a = field address (in    */
		                          /* following NOP)                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			gen_nullptr_check(s1);

			if (!iptr[1].val.a) {
				codegen_addpatchref(cd, mcodeptr,
									PATCHER_get_putfield,
									(unresolved_field *) iptr[1].target);

				if (showdisassemble)
					M_NOP;

				a = 0;

			} else {
				a = ((fieldinfo *) (iptr[1].val.a))->offset;
			}

			switch (iptr[1].op1) {
			case TYPE_INT:
				M_IST(REG_ZERO, s1, a);
				break;
			case TYPE_LNG:
				M_LST(REG_ZERO, s1, a);
				break;
			case TYPE_ADR:
				M_AST(REG_ZERO, s1, a);
				break;
			case TYPE_FLT:
				M_FST(REG_ZERO, s1, a);
				break;
			case TYPE_DBL:
				M_DST(REG_ZERO, s1, a);
				break;
			}
			break;


		/* branch operations **************************************************/

		case ICMD_ATHROW:       /* ..., objectref ==> ... (, objectref)       */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_INTMOVE(s1, REG_ITMP1_XPTR);
			a = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP2, REG_PV, a);
			M_JMP(REG_ITMP2_XPC, REG_ITMP2);
			M_NOP;              /* nop ensures that XPC is less than the end */
			                    /* of basic block                            */
			ALIGNCODENOP;
			break;

		case ICMD_GOTO:         /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */
			M_BR(0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			ALIGNCODENOP;
			break;

		case ICMD_JSR:          /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */

			M_BSR(REG_ITMP1, 0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;
			
		case ICMD_RET:          /* ... ==> ...                                */
		                        /* op1 = local variable                       */

			var = &(rd->locals[iptr->op1][TYPE_ADR]);
			if (var->flags & INMEMORY) {
				M_ALD(REG_ITMP1, REG_SP, 8 * var->regoff);
				M_RET(REG_ZERO, REG_ITMP1);
				}
			else
				M_RET(REG_ZERO, var->regoff);
			ALIGNCODENOP;
			break;

		case ICMD_IFNULL:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_BEQZ(s1, 0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IFNONNULL:    /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_BNEZ(s1, 0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IFEQ:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BEQZ(s1, 0);
				}
			else {
				if ((iptr->val.i > 0) && (iptr->val.i <= 255)) {
					M_CMPEQ_IMM(s1, iptr->val.i, REG_ITMP1);
					}
				else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMPEQ(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BNEZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IFLT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BLTZ(s1, 0);
				}
			else {
				if ((iptr->val.i > 0) && (iptr->val.i <= 255)) {
					M_CMPLT_IMM(s1, iptr->val.i, REG_ITMP1);
					}
				else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMPLT(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BNEZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IFLE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BLEZ(s1, 0);
				}
			else {
				if ((iptr->val.i > 0) && (iptr->val.i <= 255)) {
					M_CMPLE_IMM(s1, iptr->val.i, REG_ITMP1);
					}
				else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMPLE(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BNEZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IFNE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BNEZ(s1, 0);
				}
			else {
				if ((iptr->val.i > 0) && (iptr->val.i <= 255)) {
					M_CMPEQ_IMM(s1, iptr->val.i, REG_ITMP1);
					}
				else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMPEQ(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BEQZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IFGT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BGTZ(s1, 0);
				}
			else {
				if ((iptr->val.i > 0) && (iptr->val.i <= 255)) {
					M_CMPLE_IMM(s1, iptr->val.i, REG_ITMP1);
					}
				else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMPLE(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BEQZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IFGE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BGEZ(s1, 0);
				}
			else {
				if ((iptr->val.i > 0) && (iptr->val.i <= 255)) {
					M_CMPLT_IMM(s1, iptr->val.i, REG_ITMP1);
					}
				else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMPLT(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BEQZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IF_LEQ:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BEQZ(s1, 0);
				}
			else {
				if ((iptr->val.l > 0) && (iptr->val.l <= 255)) {
					M_CMPEQ_IMM(s1, iptr->val.l, REG_ITMP1);
					}
				else {
					LCONST(REG_ITMP2, iptr->val.l);
					M_CMPEQ(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BNEZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IF_LLT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BLTZ(s1, 0);
				}
			else {
				if ((iptr->val.l > 0) && (iptr->val.l <= 255)) {
					M_CMPLT_IMM(s1, iptr->val.l, REG_ITMP1);
					}
				else {
					LCONST(REG_ITMP2, iptr->val.l);
					M_CMPLT(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BNEZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IF_LLE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BLEZ(s1, 0);
				}
			else {
				if ((iptr->val.l > 0) && (iptr->val.l <= 255)) {
					M_CMPLE_IMM(s1, iptr->val.l, REG_ITMP1);
					}
				else {
					LCONST(REG_ITMP2, iptr->val.l);
					M_CMPLE(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BNEZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IF_LNE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BNEZ(s1, 0);
				}
			else {
				if ((iptr->val.l > 0) && (iptr->val.l <= 255)) {
					M_CMPEQ_IMM(s1, iptr->val.l, REG_ITMP1);
					}
				else {
					LCONST(REG_ITMP2, iptr->val.l);
					M_CMPEQ(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BEQZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IF_LGT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BGTZ(s1, 0);
				}
			else {
				if ((iptr->val.l > 0) && (iptr->val.l <= 255)) {
					M_CMPLE_IMM(s1, iptr->val.l, REG_ITMP1);
					}
				else {
					LCONST(REG_ITMP2, iptr->val.l);
					M_CMPLE(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BEQZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IF_LGE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BGEZ(s1, 0);
				}
			else {
				if ((iptr->val.l > 0) && (iptr->val.l <= 255)) {
					M_CMPLT_IMM(s1, iptr->val.l, REG_ITMP1);
					}
				else {
					LCONST(REG_ITMP2, iptr->val.l);
					M_CMPLT(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BEQZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IF_ICMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPEQ:    /* op1 = target JavaVM pc                     */
		case ICMD_IF_ACMPEQ:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPEQ(s1, s2, REG_ITMP1);
			M_BNEZ(REG_ITMP1, 0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IF_ICMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPNE:    /* op1 = target JavaVM pc                     */
		case ICMD_IF_ACMPNE:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPEQ(s1, s2, REG_ITMP1);
			M_BEQZ(REG_ITMP1, 0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IF_ICMPLT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPLT:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPLT(s1, s2, REG_ITMP1);
			M_BNEZ(REG_ITMP1, 0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IF_ICMPGT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPGT:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPLE(s1, s2, REG_ITMP1);
			M_BEQZ(REG_ITMP1, 0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IF_ICMPLE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPLE:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPLE(s1, s2, REG_ITMP1);
			M_BNEZ(REG_ITMP1, 0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		case ICMD_IF_ICMPGE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPGE:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPLT(s1, s2, REG_ITMP1);
			M_BEQZ(REG_ITMP1, 0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			break;

		/* (value xx 0) ? IFxx_ICONST : ELSE_ICONST                           */

		case ICMD_ELSE_ICONST:  /* handled by IFxx_ICONST                     */
			break;

		case ICMD_IFEQ_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			s3 = iptr->val.i;
			if (iptr[1].opc == ICMD_ELSE_ICONST) {
				if ((s3 == 1) && (iptr[1].val.i == 0)) {
					M_CMPEQ(s1, REG_ZERO, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if ((s3 == 0) && (iptr[1].val.i == 1)) {
					M_CMPEQ(s1, REG_ZERO, d);
					M_XOR_IMM(d, 1, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if (s1 == d) {
					M_MOV(s1, REG_ITMP1);
					s1 = REG_ITMP1;
					}
				ICONST(d, iptr[1].val.i);
				}
			if ((s3 >= 0) && (s3 <= 255)) {
				M_CMOVEQ_IMM(s1, s3, d);
				}
			else {
				ICONST(REG_ITMP2, s3);
				M_CMOVEQ(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFNE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			s3 = iptr->val.i;
			if (iptr[1].opc == ICMD_ELSE_ICONST) {
				if ((s3 == 0) && (iptr[1].val.i == 1)) {
					M_CMPEQ(s1, REG_ZERO, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if ((s3 == 1) && (iptr[1].val.i == 0)) {
					M_CMPEQ(s1, REG_ZERO, d);
					M_XOR_IMM(d, 1, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if (s1 == d) {
					M_MOV(s1, REG_ITMP1);
					s1 = REG_ITMP1;
					}
				ICONST(d, iptr[1].val.i);
				}
			if ((s3 >= 0) && (s3 <= 255)) {
				M_CMOVNE_IMM(s1, s3, d);
				}
			else {
				ICONST(REG_ITMP2, s3);
				M_CMOVNE(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFLT_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			s3 = iptr->val.i;
			if ((iptr[1].opc == ICMD_ELSE_ICONST)) {
				if ((s3 == 1) && (iptr[1].val.i == 0)) {
					M_CMPLT(s1, REG_ZERO, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if ((s3 == 0) && (iptr[1].val.i == 1)) {
					M_CMPLE(REG_ZERO, s1, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if (s1 == d) {
					M_MOV(s1, REG_ITMP1);
					s1 = REG_ITMP1;
					}
				ICONST(d, iptr[1].val.i);
				}
			if ((s3 >= 0) && (s3 <= 255)) {
				M_CMOVLT_IMM(s1, s3, d);
				}
			else {
				ICONST(REG_ITMP2, s3);
				M_CMOVLT(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFGE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			s3 = iptr->val.i;
			if ((iptr[1].opc == ICMD_ELSE_ICONST)) {
				if ((s3 == 1) && (iptr[1].val.i == 0)) {
					M_CMPLE(REG_ZERO, s1, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if ((s3 == 0) && (iptr[1].val.i == 1)) {
					M_CMPLT(s1, REG_ZERO, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if (s1 == d) {
					M_MOV(s1, REG_ITMP1);
					s1 = REG_ITMP1;
					}
				ICONST(d, iptr[1].val.i);
				}
			if ((s3 >= 0) && (s3 <= 255)) {
				M_CMOVGE_IMM(s1, s3, d);
				}
			else {
				ICONST(REG_ITMP2, s3);
				M_CMOVGE(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFGT_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			s3 = iptr->val.i;
			if ((iptr[1].opc == ICMD_ELSE_ICONST)) {
				if ((s3 == 1) && (iptr[1].val.i == 0)) {
					M_CMPLT(REG_ZERO, s1, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if ((s3 == 0) && (iptr[1].val.i == 1)) {
					M_CMPLE(s1, REG_ZERO, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if (s1 == d) {
					M_MOV(s1, REG_ITMP1);
					s1 = REG_ITMP1;
					}
				ICONST(d, iptr[1].val.i);
				}
			if ((s3 >= 0) && (s3 <= 255)) {
				M_CMOVGT_IMM(s1, s3, d);
				}
			else {
				ICONST(REG_ITMP2, s3);
				M_CMOVGT(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFLE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			s3 = iptr->val.i;
			if ((iptr[1].opc == ICMD_ELSE_ICONST)) {
				if ((s3 == 1) && (iptr[1].val.i == 0)) {
					M_CMPLE(s1, REG_ZERO, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if ((s3 == 0) && (iptr[1].val.i == 1)) {
					M_CMPLT(REG_ZERO, s1, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if (s1 == d) {
					M_MOV(s1, REG_ITMP1);
					s1 = REG_ITMP1;
					}
				ICONST(d, iptr[1].val.i);
				}
			if ((s3 >= 0) && (s3 <= 255)) {
				M_CMOVLE_IMM(s1, s3, d);
				}
			else {
				ICONST(REG_ITMP2, s3);
				M_CMOVLE(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_IRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_LRETURN:
		case ICMD_ARETURN:

			var_to_reg_int(s1, src, REG_RESULT);
			M_INTMOVE(s1, REG_RESULT);

			goto nowperformreturn;

		case ICMD_FRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_DRETURN:

			var_to_reg_flt(s1, src, REG_FRESULT);
			M_FLTMOVE(s1, REG_FRESULT);

			goto nowperformreturn;

		case ICMD_RETURN:       /* ...  ==> ...                               */

nowperformreturn:
			{
			s4 i, p;
			
			p = parentargs_base;
			
			/* call trace function */

			if (runverbose) {
				M_LDA(REG_SP, REG_SP, -3 * 8);
				M_AST(REG_RA, REG_SP, 0 * 8);
				M_LST(REG_RESULT, REG_SP, 1 * 8);
				M_DST(REG_FRESULT, REG_SP, 2 * 8);
				a = dseg_addaddress(cd, m);
				M_ALD(rd->argintregs[0], REG_PV, a);
				M_MOV(REG_RESULT, rd->argintregs[1]);
				M_FLTMOVE(REG_FRESULT, rd->argfltregs[2]);
				M_FLTMOVE(REG_FRESULT, rd->argfltregs[3]);
				a = dseg_addaddress(cd, (void *) builtin_displaymethodstop);
				M_ALD(REG_PV, REG_PV, a);
				M_JSR(REG_RA, REG_PV);
				s1 = (s4) ((u1 *) mcodeptr - cd->mcodebase);
				if (s1 <= 32768) M_LDA(REG_PV, REG_RA, -s1);
				else {
					s4 ml = -s1, mh = 0;
					while (ml < -32768) { ml += 65536; mh--; }
					M_LDA(REG_PV, REG_RA, ml);
					M_LDAH(REG_PV, REG_PV, mh);
				}
				M_DLD(REG_FRESULT, REG_SP, 2 * 8);
				M_LLD(REG_RESULT, REG_SP, 1 * 8);
				M_ALD(REG_RA, REG_SP, 0 * 8);
				M_LDA(REG_SP, REG_SP, 3 * 8);
			}

#if defined(USE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				s4 disp;

				M_ALD(rd->argintregs[0], REG_SP, rd->memuse * 8);

				switch (iptr->opc) {
				case ICMD_IRETURN:
				case ICMD_LRETURN:
				case ICMD_ARETURN:
					M_LST(REG_RESULT, REG_SP, rd->memuse * 8);
					break;
				case ICMD_FRETURN:
				case ICMD_DRETURN:
					M_DST(REG_FRESULT, REG_SP, rd->memuse * 8);
					break;
				}

				a = dseg_addaddress(cd, BUILTIN_monitorexit);
				M_ALD(REG_PV, REG_PV, a);
				M_JSR(REG_RA, REG_PV);
				disp = -(s4) ((u1 *) mcodeptr - cd->mcodebase);
				M_LDA(REG_PV, REG_RA, disp);

				switch (iptr->opc) {
				case ICMD_IRETURN:
				case ICMD_LRETURN:
				case ICMD_ARETURN:
					M_LLD(REG_RESULT, REG_SP, rd->memuse * 8);
					break;
				case ICMD_FRETURN:
				case ICMD_DRETURN:
					M_DLD(REG_FRESULT, REG_SP, rd->memuse * 8);
					break;
				}
			}
#endif

			/* restore return address                                         */

			if (!m->isleafmethod) {
				p--; M_LLD(REG_RA, REG_SP, p * 8);
			}

			/* restore saved registers                                        */

			for (i = INT_SAV_CNT - 1; i >= rd->savintreguse; i--) {
				p--; M_LLD(rd->savintregs[i], REG_SP, p * 8);
			}
			for (i = FLT_SAV_CNT - 1; i >= rd->savfltreguse; i--) {
				p--; M_DLD(rd->savfltregs[i], REG_SP, p * 8);
			}

			/* deallocate stack                                               */

			if (parentargs_base) {
				M_LDA(REG_SP, REG_SP, parentargs_base * 8);
			}

			M_RET(REG_ZERO, REG_RA);
			ALIGNCODENOP;
			}
			break;


		case ICMD_TABLESWITCH:  /* ..., index ==> ...                         */
			{
			s4 i, l, *s4ptr;
			void **tptr;

			tptr = (void **) iptr->target;

			s4ptr = iptr->val.a;
			l = s4ptr[1];                          /* low     */
			i = s4ptr[2];                          /* high    */
			
			var_to_reg_int(s1, src, REG_ITMP1);
			if (l == 0)
				{M_INTMOVE(s1, REG_ITMP1);}
			else if (l <= 32768) {
				M_LDA(REG_ITMP1, s1, -l);
				}
			else {
				ICONST(REG_ITMP2, l);
				M_ISUB(s1, REG_ITMP2, REG_ITMP1);
				}
			i = i - l + 1;

			/* range check */

			if (i <= 256)
				M_CMPULE_IMM(REG_ITMP1, i - 1, REG_ITMP2);
			else {
				M_LDA(REG_ITMP2, REG_ZERO, i - 1);
				M_CMPULE(REG_ITMP1, REG_ITMP2, REG_ITMP2);
				}
			M_BEQZ(REG_ITMP2, 0);


			/* codegen_addreference(cd, BlockPtrOfPC(s4ptr[0]), mcodeptr); */
			codegen_addreference(cd, (basicblock *) tptr[0], mcodeptr);

			/* build jump table top down and use address of lowest entry */

			/* s4ptr += 3 + i; */
			tptr += i;

			while (--i >= 0) {
				/* dseg_addtarget(cd, BlockPtrOfPC(*--s4ptr)); */
				dseg_addtarget(cd, (basicblock *) tptr[0]); 
				--tptr;
				}
			}

			/* length of dataseg after last dseg_addtarget is used by load */

			M_SAADDQ(REG_ITMP1, REG_PV, REG_ITMP2);
			M_ALD(REG_ITMP2, REG_ITMP2, -(cd->dseglen));
			M_JMP(REG_ZERO, REG_ITMP2);
			ALIGNCODENOP;
			break;


		case ICMD_LOOKUPSWITCH: /* ..., key ==> ...                           */
			{
			s4 i, l, val, *s4ptr;
			void **tptr;

			tptr = (void **) iptr->target;

			s4ptr = iptr->val.a;
			l = s4ptr[0];                          /* default  */
			i = s4ptr[1];                          /* count    */
			
			MCODECHECK((i<<2)+8);
			var_to_reg_int(s1, src, REG_ITMP1);
			while (--i >= 0) {
				s4ptr += 2;
				++tptr;

				val = s4ptr[0];
				if ((val >= 0) && (val <= 255)) {
					M_CMPEQ_IMM(s1, val, REG_ITMP2);
					}
				else {
					if ((val >= -32768) && (val <= 32767)) {
						M_LDA(REG_ITMP2, REG_ZERO, val);
						} 
					else {
						a = dseg_adds4(cd, val);
						M_ILD(REG_ITMP2, REG_PV, a);
						}
					M_CMPEQ(s1, REG_ITMP2, REG_ITMP2);
					}
				M_BNEZ(REG_ITMP2, 0);
				/* codegen_addreference(cd, BlockPtrOfPC(s4ptr[1]), mcodeptr); */
				codegen_addreference(cd, (basicblock *) tptr[0], mcodeptr); 
				}

			M_BR(0);
			/* codegen_addreference(cd, BlockPtrOfPC(l), mcodeptr); */
			
			tptr = (void **) iptr->target;
			codegen_addreference(cd, (basicblock *) tptr[0], mcodeptr);

			ALIGNCODENOP;
			break;
			}


		case ICMD_BUILTIN:      /* ..., arg1, arg2, arg3 ==> ...              */
		                        /* op1 = arg count val.a = builtintable entry */

			bte = iptr->val.a;
			md = bte->md;
			goto gen_method;

		case ICMD_INVOKESTATIC: /* ..., [arg1, [arg2 ...]] ==> ...            */
		                        /* op1 = arg count, val.a = method pointer    */

		case ICMD_INVOKESPECIAL:/* ..., objectref, [arg1, [arg2 ...]] ==> ... */
		case ICMD_INVOKEVIRTUAL:/* op1 = arg count, val.a = method pointer    */
		case ICMD_INVOKEINTERFACE:

			lm = iptr->val.a;

			if (lm)
				md = lm->parseddesc;
			else {
				unresolved_method *um = iptr->target;
				md = um->methodref->parseddesc.md;
			}

gen_method:
			s3 = iptr->op1;

			MCODECHECK((s3 << 1) + 64);

			/* copy arguments to registers or stack location                  */

			for (s3 = s3 - 1; s3 >= 0; s3--, src = src->prev) {
				if (src->varkind == ARGVAR)
					continue;
				if (IS_INT_LNG_TYPE(src->type)) {
					if (!md->params[s3].inmemory) {
						s1 = rd->argintregs[md->params[s3].regoff];
						var_to_reg_int(d, src, s1);
						M_INTMOVE(d, s1);
					} else {
						var_to_reg_int(d, src, REG_ITMP1);
						M_LST(d, REG_SP, md->params[s3].regoff * 8);
					}

				} else {
					if (!md->params[s3].inmemory) {
						s1 = rd->argfltregs[md->params[s3].regoff];
						var_to_reg_flt(d, src, s1);
						M_FLTMOVE(d, s1);
					} else {
						var_to_reg_flt(d, src, REG_FTMP1);
						M_DST(d, REG_SP, md->params[s3].regoff * 8);
					}
				}
			}

			switch (iptr->opc) {
			case ICMD_BUILTIN:
				if (iptr->target) {
					codegen_addpatchref(cd, mcodeptr, bte->fp, iptr->target);

					if (showdisassemble)
						M_NOP;

					a = 0;

				} else {
					a = (ptrint) bte->fp;
				}

				a = dseg_addaddress(cd, a);
				d = md->returntype.type;

				M_ALD(REG_PV, REG_PV, a);     /* Pointer to built-in-function */
				break;

			case ICMD_INVOKESPECIAL:
				gen_nullptr_check(rd->argintregs[0]);
				M_ILD(REG_ITMP1, rd->argintregs[0], 0); /* hardware nullptr   */
				/* fall through */

			case ICMD_INVOKESTATIC:
				if (!lm) {
					unresolved_method *um = iptr->target;

					codegen_addpatchref(cd, mcodeptr,
										PATCHER_invokestatic_special, um);

					if (showdisassemble)
						M_NOP;

					a = 0;
					d = um->methodref->parseddesc.md->returntype.type;

				} else {
					a = (ptrint) lm->stubroutine;
					d = lm->parseddesc->returntype.type;
				}

				a = dseg_addaddress(cd, a);
				M_ALD(REG_PV, REG_PV, a);            /* method pointer in r27 */
				break;

			case ICMD_INVOKEVIRTUAL:
				gen_nullptr_check(rd->argintregs[0]);

				if (!lm) {
					unresolved_method *um = iptr->target;

					codegen_addpatchref(cd, mcodeptr,
										PATCHER_invokevirtual, um);

					if (showdisassemble)
						M_NOP;

					s1 = 0;
					d = um->methodref->parseddesc.md->returntype.type;

				} else {
					s1 = OFFSET(vftbl_t, table[0]) +
						sizeof(methodptr) * lm->vftblindex;
					d = lm->parseddesc->returntype.type;
				}

				M_ALD(REG_METHODPTR, rd->argintregs[0],
					  OFFSET(java_objectheader, vftbl));
				M_ALD(REG_PV, REG_METHODPTR, s1);
				break;

			case ICMD_INVOKEINTERFACE:
				gen_nullptr_check(rd->argintregs[0]);

				if (!lm) {
					unresolved_method *um = iptr->target;

					codegen_addpatchref(cd, mcodeptr,
										PATCHER_invokeinterface, um);

					if (showdisassemble)
						M_NOP;

					s1 = 0;
					s2 = 0;
					d = um->methodref->parseddesc.md->returntype.type;

				} else {
					s1 = OFFSET(vftbl_t, interfacetable[0]) -
						sizeof(methodptr*) * lm->class->index;

					s2 = sizeof(methodptr) * (lm - lm->class->methods);

					d = lm->parseddesc->returntype.type;
				}
					
				M_ALD(REG_METHODPTR, rd->argintregs[0],
					  OFFSET(java_objectheader, vftbl));    
				M_ALD(REG_METHODPTR, REG_METHODPTR, s1);
				M_ALD(REG_PV, REG_METHODPTR, s2);
				break;
			}

			M_JSR(REG_RA, REG_PV);

			/* recompute pv */

			s1 = (s4) ((u1 *) mcodeptr - cd->mcodebase);
			if (s1 <= 32768) M_LDA(REG_PV, REG_RA, -s1);
			else {
				s4 ml = -s1, mh = 0;
				while (ml < -32768) { ml += 65536; mh--; }
				M_LDA(REG_PV, REG_RA, ml);
				M_LDAH(REG_PV, REG_PV, mh);
			}

			/* d contains return type */

			if (d != TYPE_VOID) {
				if (IS_INT_LNG_TYPE(iptr->dst->type)) {
					s1 = reg_of_var(rd, iptr->dst, REG_RESULT);
					M_INTMOVE(REG_RESULT, s1);
					store_reg_to_var_int(iptr->dst, s1);
				} else {
					s1 = reg_of_var(rd, iptr->dst, REG_FRESULT);
					M_FLTMOVE(REG_FRESULT, s1);
					store_reg_to_var_flt(iptr->dst, s1);
				}
			}
			break;


		case ICMD_CHECKCAST:  /* ..., objectref ==> ..., objectref            */

		                      /* op1:   0 == array, 1 == class                */
		                      /* val.a: (classinfo*) superclass               */

			/*  superclass is an interface:
			 *	
			 *  OK if ((sub == NULL) ||
			 *         (sub->vftbl->interfacetablelength > super->index) &&
			 *         (sub->vftbl->interfacetable[-super->index] != NULL));
			 *	
			 *  superclass is a class:
			 *	
			 *  OK if ((sub == NULL) || (0
			 *         <= (sub->vftbl->baseval - super->vftbl->baseval) <=
			 *         super->vftbl->diffval));
			 */

			{
			classinfo *super;
			vftbl_t   *supervftbl;
			s4         superindex;

			super = (classinfo *) iptr->val.a;

			if (!super) {
				superindex = 0;
				supervftbl = NULL;

			} else {
				superindex = super->index;
				supervftbl = super->vftbl;
			}
			
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			codegen_threadcritrestart(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
			var_to_reg_int(s1, src, REG_ITMP1);

			/* calculate interface checkcast code size */

			s2 = 6;
			if (!super)
				s2 += showdisassemble ? 1 : 0;

			/* calculate class checkcast code size */

			s3 = 9 /* 8 + (s1 == REG_ITMP1) */;
			if (!super)
				s3 += showdisassemble ? 1 : 0;

			/* if class is not resolved, check which code to call */

			if (!super) {
				M_BEQZ(s1, 4 + (showdisassemble ? 1 : 0) + s2 + 1 + s3);

				codegen_addpatchref(cd, mcodeptr,
									PATCHER_checkcast_instanceof_flags,
									(constant_classref *) iptr->target);

				if (showdisassemble)
					M_NOP;

				a = dseg_adds4(cd, 0); /* super->flags */
				M_ILD(REG_ITMP2, REG_PV, a);
				a = dseg_adds4(cd, ACC_INTERFACE);
				M_ILD(REG_ITMP3, REG_PV, a);
				M_AND(REG_ITMP2, REG_ITMP3, REG_ITMP2);
				M_BEQZ(REG_ITMP2, s2 + 1);
			}

			/* interface checkcast code */

			if (!super || (super->flags & ACC_INTERFACE)) {
				if (super) {
					M_BEQZ(s1, s2);

				} else {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_checkcast_instanceof_interface,
										(constant_classref *) iptr->target);

					if (showdisassemble)
						M_NOP;
				}

				M_ALD(REG_ITMP2, s1, OFFSET(java_objectheader, vftbl));
				M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl_t, interfacetablelength));
				M_LDA(REG_ITMP3, REG_ITMP3, -superindex);
				M_BLEZ(REG_ITMP3, 0);
				codegen_addxcastrefs(cd, mcodeptr);
				M_ALD(REG_ITMP3, REG_ITMP2,
					  OFFSET(vftbl_t, interfacetable[0]) -
					  superindex * sizeof(methodptr*));
				M_BEQZ(REG_ITMP3, 0);
				codegen_addxcastrefs(cd, mcodeptr);

				if (!super)
					M_BR(s3);
			}

			/* class checkcast code */

			if (!super || !(super->flags & ACC_INTERFACE)) {
				if (super) {
					M_BEQZ(s1, s3);

				} else {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_checkcast_instanceof_class,
										(constant_classref *) iptr->target);

					if (showdisassemble)
						M_NOP;
				}

				M_ALD(REG_ITMP2, s1, OFFSET(java_objectheader, vftbl));
				a = dseg_addaddress(cd, supervftbl);
				M_ALD(REG_ITMP3, REG_PV, a);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
				codegen_threadcritstart(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
				M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, baseval));
/*  				if (s1 != REG_ITMP1) { */
/*  					M_ILD(REG_ITMP1, REG_ITMP3, OFFSET(vftbl_t, baseval)); */
/*  					M_ILD(REG_ITMP3, REG_ITMP3, OFFSET(vftbl_t, diffval)); */
/*  #if defined(USE_THREADS) && defined(NATIVE_THREADS) */
/*  					codegen_threadcritstop(cd, (u1 *) mcodeptr - cd->mcodebase); */
/*  #endif */
/*  					M_ISUB(REG_ITMP2, REG_ITMP1, REG_ITMP2); */

/*  				} else { */
					M_ILD(REG_ITMP3, REG_ITMP3, OFFSET(vftbl_t, baseval));
					M_ISUB(REG_ITMP2, REG_ITMP3, REG_ITMP2);
					M_ALD(REG_ITMP3, REG_PV, a);
					M_ILD(REG_ITMP3, REG_ITMP3, OFFSET(vftbl_t, diffval));
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
					codegen_threadcritstop(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
/*  				} */
				M_CMPULE(REG_ITMP2, REG_ITMP3, REG_ITMP3);
				M_BEQZ(REG_ITMP3, 0);
				codegen_addxcastrefs(cd, mcodeptr);
			}
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			}
			break;

		case ICMD_INSTANCEOF: /* ..., objectref ==> ..., intresult            */

		                      /* op1:   0 == array, 1 == class                */
		                      /* val.a: (classinfo*) superclass               */

			/*  superclass is an interface:
			 *	
			 *  return (sub != NULL) &&
			 *         (sub->vftbl->interfacetablelength > super->index) &&
			 *         (sub->vftbl->interfacetable[-super->index] != NULL);
			 *	
			 *  superclass is a class:
			 *	
			 *  return ((sub != NULL) && (0
			 *          <= (sub->vftbl->baseval - super->vftbl->baseval) <=
			 *          super->vftbl->diffvall));
			 */

			{
			classinfo *super;
			vftbl_t   *supervftbl;
			s4         superindex;

			super = (classinfo *) iptr->val.a;

			if (!super) {
				superindex = 0;
				supervftbl = NULL;

			} else {
				superindex = super->index;
				supervftbl = super->vftbl;
			}
			
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			codegen_threadcritrestart(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if (s1 == d) {
				M_MOV(s1, REG_ITMP1);
				s1 = REG_ITMP1;
			}

			/* calculate interface instanceof code size */

			s2 = 6;
			if (!super)
				s2 += (d == REG_ITMP2 ? 1 : 0) + (showdisassemble ? 1 : 0);

			/* calculate class instanceof code size */

			s3 = 7;
			if (!super)
				s3 += (showdisassemble ? 1 : 0);

			/* if class is not resolved, check which code to call */

			if (!super) {
				M_CLR(d);
				M_BEQZ(s1, 4 + (showdisassemble ? 1 : 0) + s2 + 1 + s3);

				codegen_addpatchref(cd, mcodeptr,
									PATCHER_checkcast_instanceof_flags,
									(constant_classref *) iptr->target);

				if (showdisassemble)
					M_NOP;

				a = dseg_adds4(cd, 0); /* super->flags */
				M_ILD(REG_ITMP3, REG_PV, a);
				a = dseg_adds4(cd, ACC_INTERFACE);
				M_ILD(REG_ITMP2, REG_PV, a);
				M_AND(REG_ITMP3, REG_ITMP2, REG_ITMP3);
				M_BEQZ(REG_ITMP3, s2 + 1);
			}

			/* interface instanceof code */

			if (!super || (super->flags & ACC_INTERFACE)) {
				if (super) {
					M_CLR(d);
					M_BEQZ(s1, s2);

				} else {
					/* If d == REG_ITMP2, then it's destroyed in check code   */
					/* above.                                                 */
					if (d == REG_ITMP2)
						M_CLR(d);

					codegen_addpatchref(cd, mcodeptr,
										PATCHER_checkcast_instanceof_interface,
										(constant_classref *) iptr->target);

					if (showdisassemble)
						M_NOP;
				}

				M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
				M_ILD(REG_ITMP3, REG_ITMP1, OFFSET(vftbl_t, interfacetablelength));
				M_LDA(REG_ITMP3, REG_ITMP3, -superindex);
				M_BLEZ(REG_ITMP3, 2);
				M_ALD(REG_ITMP1, REG_ITMP1,
					  OFFSET(vftbl_t, interfacetable[0]) -
					  superindex * sizeof(methodptr*));
				M_CMPULT(REG_ZERO, REG_ITMP1, d);      /* REG_ITMP1 != 0  */

				if (!super)
					M_BR(s3);
			}

			/* class instanceof code */

			if (!super || !(super->flags & ACC_INTERFACE)) {
				if (super) {
					M_CLR(d);
					M_BEQZ(s1, s3);

				} else {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_checkcast_instanceof_class,
										(constant_classref *) iptr->target);

					if (showdisassemble)
						M_NOP;
				}

				M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
				a = dseg_addaddress(cd, supervftbl);
				M_ALD(REG_ITMP2, REG_PV, a);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
				codegen_threadcritstart(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
				M_ILD(REG_ITMP1, REG_ITMP1, OFFSET(vftbl_t, baseval));
				M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl_t, baseval));
				M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, diffval));
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
				codegen_threadcritstop(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
				M_ISUB(REG_ITMP1, REG_ITMP3, REG_ITMP1);
				M_CMPULE(REG_ITMP1, REG_ITMP2, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			}
			break;


		case ICMD_CHECKASIZE:  /* ..., size ==> ..., size                     */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_BLTZ(s1, 0);
			codegen_addxcheckarefs(cd, mcodeptr);
			break;

		case ICMD_CHECKEXCEPTION:    /* ... ==> ...                           */

			M_BEQZ(REG_RESULT, 0);
			codegen_addxexceptionrefs(cd, mcodeptr);
			break;

		case ICMD_MULTIANEWARRAY:/* ..., cnt1, [cnt2, ...] ==> ..., arrayref  */
		                      /* op1 = dimension, val.a = array descriptor    */

			/* check for negative sizes and copy sizes to stack if necessary  */

			MCODECHECK((iptr->op1 << 1) + 64);

			for (s1 = iptr->op1; --s1 >= 0; src = src->prev) {
				var_to_reg_int(s2, src, REG_ITMP1);
				M_BLTZ(s2, 0);
				codegen_addxcheckarefs(cd, mcodeptr);

				/* copy SAVEDVAR sizes to stack */

				if (src->varkind != ARGVAR) {
					M_LST(s2, REG_SP, s1 * 8);
				}
			}

			/* is patcher function set? */

			if (iptr->target) {
				codegen_addpatchref(cd, mcodeptr,
									(functionptr) iptr->target, iptr->val.a);

				if (showdisassemble)
					M_NOP;

				a = 0;

			} else {
				a = (ptrint) iptr->val.a;
			}

			/* a0 = dimension count */

			ICONST(rd->argintregs[0], iptr->op1);

			/* a1 = arraydescriptor */

			a = dseg_addaddress(cd, a);
			M_ALD(rd->argintregs[1], REG_PV, a);

			/* a2 = pointer to dimensions = stack pointer */

			M_INTMOVE(REG_SP, rd->argintregs[2]);

			a = dseg_addaddress(cd, (void *) BUILTIN_multianewarray);
			M_ALD(REG_PV, REG_PV, a);
			M_JSR(REG_RA, REG_PV);
			s1 = (s4) ((u1 *) mcodeptr - cd->mcodebase);
			if (s1 <= 32768)
				M_LDA(REG_PV, REG_RA, -s1);
			else {
				s4 ml = -s1, mh = 0;
				while (ml < -32768) { ml += 65536; mh--; }
				M_LDA(REG_PV, REG_RA, ml);
				M_LDAH(REG_PV, REG_PV, mh);
			}
			s1 = reg_of_var(rd, iptr->dst, REG_RESULT);
			M_INTMOVE(REG_RESULT, s1);
			store_reg_to_var_int(iptr->dst, s1);
			break;

		default:
			throw_cacao_exception_exit(string_java_lang_InternalError,
									   "Unknown ICMD %d", iptr->opc);
	} /* switch */
		
	} /* for instruction */
		
	/* copy values to interface registers */

	src = bptr->outstack;
	len = bptr->outdepth;
	MCODECHECK(64+len);
#ifdef LSRA
	if (!opt_lsra) 
#endif
	while (src) {
		len--;
		if ((src->varkind != STACKVAR)) {
			s2 = src->type;
			if (IS_FLT_DBL_TYPE(s2)) {
				var_to_reg_flt(s1, src, REG_FTMP1);
				if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
					M_FLTMOVE(s1,rd->interfaces[len][s2].regoff);
					}
				else {
					M_DST(s1, REG_SP, 8 * rd->interfaces[len][s2].regoff);
					}
				}
			else {
				var_to_reg_int(s1, src, REG_ITMP1);
				if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
					M_INTMOVE(s1,rd->interfaces[len][s2].regoff);
					}
				else {
					M_LST(s1, REG_SP, 8 * rd->interfaces[len][s2].regoff);
					}
				}
			}
		src = src->prev;
		}
	} /* if (bptr -> flags >= BBREACHED) */
	} /* for basic block */

	codegen_createlinenumbertable(cd);

	{
	/* generate bound check stubs */

	s4 *xcodeptr = NULL;
	branchref *bref;

	for (bref = cd->xboundrefs; bref != NULL; bref = bref->next) {
		gen_resolvebranch((u1*) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1*) mcodeptr - cd->mcodebase);

		MCODECHECK(8);

		/* move index register into REG_ITMP1 */
		M_MOV(bref->reg, REG_ITMP1);
		M_LDA(REG_ITMP2_XPC, REG_PV, bref->branchpos - 4);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr - 1);

		} else {
			xcodeptr = mcodeptr;

			a = dseg_addaddress(cd, asm_throw_and_handle_arrayindexoutofbounds_exception);
			M_ALD(REG_PV, REG_PV, a);

			M_JSR(REG_RA, REG_PV);

			/* recompute pv */
			s1 = (s4) ((u1 *) mcodeptr - cd->mcodebase);
			if (s1 <= 32768) M_LDA(REG_PV, REG_RA, -s1);
			else {
				s4 ml = -s1, mh = 0;
				while (ml < -32768) { ml += 65536; mh--; }
				M_LDA(REG_PV, REG_RA, ml);
				M_LDAH(REG_PV, REG_PV, mh);
			}
		}
	}

	/* generate negative array size check stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xcheckarefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  (u1 *) xcodeptr - (u1 *) cd->mcodebase - 4);
			continue;
		}

		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(8);

		M_LDA(REG_ITMP2_XPC, REG_PV, bref->branchpos - 4);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr - 1);

		} else {
			xcodeptr = mcodeptr;

			a = dseg_addaddress(cd, string_java_lang_NegativeArraySizeException);
			M_ALD(REG_ITMP1_XPTR,REG_PV,a);

			a = dseg_addaddress(cd, asm_throw_and_handle_nat_exception);
			M_ALD(REG_PV, REG_PV, a);

			M_JSR(REG_RA, REG_PV);
		
			/* recompute pv */
			s1 = (s4) ((u1 *) mcodeptr - cd->mcodebase);
			if (s1 <= 32768) M_LDA(REG_PV, REG_RA, -s1);
			else {
				s4 ml = -s1, mh = 0;
				while (ml < -32768) { ml += 65536; mh--; }
				M_LDA(REG_PV, REG_RA, ml);
				M_LDAH(REG_PV, REG_PV, mh);
			}
		}
	}

	/* generate cast check stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xcastrefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  (u1 *) xcodeptr - (u1 *) cd->mcodebase - 4);
			continue;
		}

		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(8);

		M_LDA(REG_ITMP2_XPC, REG_PV, bref->branchpos - 4);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr - 1);

		} else {
			xcodeptr = mcodeptr;

			a = dseg_addaddress(cd, string_java_lang_ClassCastException);
			M_ALD(REG_ITMP1_XPTR,REG_PV,a);

			a = dseg_addaddress(cd, asm_throw_and_handle_nat_exception);
			M_ALD(REG_PV, REG_PV, a);

			M_JSR(REG_RA, REG_PV);

			/* recompute pv */
			s1 = (s4) ((u1 *) mcodeptr - cd->mcodebase);
			if (s1 <= 32768) M_LDA(REG_PV, REG_RA, -s1);
			else {
				s4 ml = -s1, mh = 0;
				while (ml < -32768) { ml += 65536; mh--; }
				M_LDA(REG_PV, REG_RA, ml);
				M_LDAH(REG_PV, REG_PV, mh);
			}
		}
	}

	/* generate exception check stubs */

	xcodeptr = NULL;

	for (bref = cd->xexceptionrefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos,
							  bref->branchpos,
							  (u1 *) xcodeptr - (u1 *) cd->mcodebase - 4);
			continue;
		}

		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(8);

		M_LDA(REG_ITMP2_XPC, REG_PV, bref->branchpos - 4);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr - 1);

		} else {
			xcodeptr = mcodeptr;

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			M_LSUB_IMM(REG_SP, 1 * 8, REG_SP);
			M_LST(REG_ITMP2_XPC, REG_SP, 0 * 8);

			a = dseg_addaddress(cd, &builtin_get_exceptionptrptr);
			M_ALD(REG_PV, REG_PV, a);
			M_JSR(REG_RA, REG_PV);

			/* recompute pv */
			s1 = (s4) ((u1 *) mcodeptr - cd->mcodebase);
			if (s1 <= 32768) M_LDA(REG_PV, REG_RA, -s1);
			else {
				s4 ml = -s1, mh = 0;
				while (ml < -32768) { ml += 65536; mh--; }
				M_LDA(REG_PV, REG_RA, ml);
				M_LDAH(REG_PV, REG_PV, mh);
			}

			M_ALD(REG_ITMP1_XPTR, REG_RESULT, 0);
			M_AST(REG_ZERO, REG_RESULT, 0);

			M_LLD(REG_ITMP2_XPC, REG_SP, 0 * 8);
			M_LADD_IMM(REG_SP, 1 * 8, REG_SP);
#else
			a = dseg_addaddress(cd, &_exceptionptr);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_ALD(REG_ITMP1_XPTR, REG_ITMP3, 0);
			M_AST(REG_ZERO, REG_ITMP3, 0);
#endif

			a = dseg_addaddress(cd, asm_refillin_and_handle_exception);
			M_ALD(REG_PV, REG_PV, a);

			M_JMP(REG_RA, REG_PV);
		
			/* recompute pv */
			s1 = (s4) ((u1 *) mcodeptr - cd->mcodebase);
			if (s1 <= 32768) M_LDA(REG_PV, REG_RA, -s1);
			else {
				s4 ml = -s1, mh = 0;
				while (ml < -32768) { ml += 65536; mh--; }
				M_LDA(REG_PV, REG_RA, ml);
				M_LDAH(REG_PV, REG_PV, mh);
			}

		}
	}

	/* generate null pointer check stubs */

	xcodeptr = NULL;

	for (bref = cd->xnullrefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  (u1 *) xcodeptr - (u1 *) cd->mcodebase - 4);
			continue;
		}

		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(8);

		M_LDA(REG_ITMP2_XPC, REG_PV, bref->branchpos - 4);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr - 1);

		} else {
			xcodeptr = mcodeptr;

			a = dseg_addaddress(cd, string_java_lang_NullPointerException);
			M_ALD(REG_ITMP1_XPTR,REG_PV,a);

			a = dseg_addaddress(cd, asm_throw_and_handle_nat_exception);
			M_ALD(REG_PV, REG_PV, a);

			M_JSR(REG_RA, REG_PV);

			/* recompute pv */
			s1 = (s4) ((u1 *) mcodeptr - cd->mcodebase);
			if (s1 <= 32768) M_LDA(REG_PV, REG_RA, -s1);
			else {
				s4 ml = -s1, mh = 0;
				while (ml < -32768) { ml += 65536; mh--; }
				M_LDA(REG_PV, REG_RA, ml);
				M_LDAH(REG_PV, REG_PV, mh);
			}
		}
	}

	/* generate put/getstatic stub call code */

	{
		patchref *pref;
		u4        mcode;
		s4       *tmpmcodeptr;

		for (pref = cd->patchrefs; pref != NULL; pref = pref->next) {
			/* check code segment size */

			MCODECHECK(13 + 4 + 1);

			/* Get machine code which is patched back in later. The call is   */
			/* 1 instruction word long.                                       */

			xcodeptr = (s4 *) (cd->mcodebase + pref->branchpos);
			mcode = *xcodeptr;

			/* patch in the call to call the following code (done at compile  */
			/* time)                                                          */

			tmpmcodeptr = mcodeptr;         /* save current mcodeptr          */
			mcodeptr = xcodeptr;            /* set mcodeptr to patch position */

			M_BSR(REG_ITMP3, tmpmcodeptr - (xcodeptr + 1));

			mcodeptr = tmpmcodeptr;         /* restore the current mcodeptr   */

			/* create stack frame */

			M_LSUB_IMM(REG_SP, 5 * 8, REG_SP);

			/* move return address onto stack */

			M_AST(REG_ITMP3, REG_SP, 4 * 8);

			/* move pointer to java_objectheader onto stack */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			/* create a virtual java_objectheader */

			(void) dseg_addaddress(cd, get_dummyLR());          /* monitorPtr */
			a = dseg_addaddress(cd, NULL);                      /* vftbl      */

			if (a >= -32768) {
				M_LDA(REG_ITMP3, REG_PV, a);
			} else {
				M_LDAH(REG_ITMP3, REG_PV, (a >> 16) & 0x0000ffff);
				M_LDA(REG_ITMP3, REG_ITMP3, a & 0x0000ffff);
			}
			M_AST(REG_ITMP3, REG_SP, 3 * 8);
#else
			M_AST(REG_ZERO, REG_SP, 3 * 8);
#endif

			/* move machine code onto stack */

			a = dseg_adds4(cd, mcode);
			if (a >= -32768) {
				M_ILD(REG_ITMP3, REG_PV, a);
			} else {
				M_LDAH(REG_ITMP3, REG_PV, (a >> 16) & 0x0000ffff);
				M_ILD(REG_ITMP3, REG_ITMP3, a & 0x0000ffff);
			}
			M_IST(REG_ITMP3, REG_SP, 2 * 8);

			/* move class/method/field reference onto stack */

			a = dseg_addaddress(cd, pref->ref);
			if (a >= -32768) {
				M_ALD(REG_ITMP3, REG_PV, a);
			} else {
				M_LDAH(REG_ITMP3, REG_PV, (a >> 16) & 0x0000ffff);
				M_ALD(REG_ITMP3, REG_ITMP3, a & 0x0000ffff);
			}
			M_AST(REG_ITMP3, REG_SP, 1 * 8);

			/* move patcher function pointer onto stack */

			a = dseg_addaddress(cd, pref->patcher);
			if (a >= -32768) {
				M_ALD(REG_ITMP3, REG_PV, a);
			} else {
				M_LDAH(REG_ITMP3, REG_PV, (a >> 16) & 0x0000ffff);
				M_ALD(REG_ITMP3, REG_ITMP3, a & 0x0000ffff);
			}
			M_AST(REG_ITMP3, REG_SP, 0 * 8);

			a = dseg_addaddress(cd, asm_wrapper_patcher);
			if (a >= -32768) {
				M_ALD(REG_ITMP3, REG_PV, a);
			} else {
				M_LDAH(REG_ITMP3, REG_PV, (a >> 16) & 0x0000ffff);
				M_ALD(REG_ITMP3, REG_ITMP3, a & 0x0000ffff);
			}
			M_JMP(REG_ZERO, REG_ITMP3);
		}
	}
	}

	codegen_finish(m, cd, (s4) ((u1 *) mcodeptr - cd->mcodebase));
}


/* createcompilerstub **********************************************************

   Creates a stub routine which calls the compiler.
	
*******************************************************************************/

#define COMPSTUBSIZE    3

functionptr createcompilerstub(methodinfo *m)
{
	u8 *s = CNEW(u8, COMPSTUBSIZE);     /* memory to hold the stub            */
	s4 *mcodeptr = (s4 *) s;            /* code generation pointer            */
	
	                                    /* code for the stub                  */
	M_ALD(REG_PV, REG_PV, 16);          /* load pointer to the compiler       */
	M_JMP(0, REG_PV);                   /* jump to the compiler, return address
	                                       in reg 0 is used as method pointer */
	s[1] = (ptrint) m;                  /* literals to be adressed            */
	s[2] = (ptrint) asm_call_jit_compiler; /* jump directly via PV from above */

#if defined(STATISTICS)
	if (opt_stat)
		count_cstub_len += COMPSTUBSIZE * 8;
#endif

	return (functionptr) s;
}


/* createnativestub ************************************************************

   Creates a stub routine which calls a native method.

*******************************************************************************/

functionptr createnativestub(functionptr f, methodinfo *m, codegendata *cd,
							 registerdata *rd, methoddesc *nmd)
{
	s4         *mcodeptr;               /* code generation pointer            */
	s4          stackframesize;         /* size of stackframe if needed       */
	s4          disp;
	methoddesc *md;
	s4          nativeparams;
	s4          i, j;                   /* count variables                    */
	s4          t;
	s4          s1, s2, off;

	/* initialize variables */

	md = m->parseddesc;
	nativeparams = (m->flags & ACC_STATIC) ? 2 : 1;


	/* calculate stack frame size */

	stackframesize =
		1 +                             /* return address                     */
		6 +                             /* dynamic stack info                 */
		1 +                             /* methodinfo for call trace          */
		(md->paramcount > INT_ARG_CNT ? INT_ARG_CNT : md->paramcount) +
		nmd->memuse;


	/* create method header */

	(void) dseg_addaddress(cd, m);                          /* MethodPointer  */
	(void) dseg_adds4(cd, stackframesize * 8);              /* FrameSize      */
	(void) dseg_adds4(cd, 0);                               /* IsSync         */
	(void) dseg_adds4(cd, 0);                               /* IsLeaf         */
	(void) dseg_adds4(cd, 0);                               /* IntSave        */
	(void) dseg_adds4(cd, 0);                               /* FltSave        */
	(void) dseg_addlinenumbertablesize(cd);
	(void) dseg_adds4(cd, 0);                               /* ExTableSize    */


	/* initialize mcode variables */
	
	mcodeptr = (s4 *) cd->mcodebase;
	cd->mcodeend = (s4 *) (cd->mcodebase + cd->mcodesize);


	/* generate stub code */

	M_LDA(REG_SP, REG_SP, -stackframesize * 8);
	M_AST(REG_RA, REG_SP, (stackframesize - 1) * 8);


	/* if function is static, check for initialized */

	if ((m->flags & ACC_STATIC) && !m->class->initialized) {
		codegen_addpatchref(cd, mcodeptr, PATCHER_clinit, m->class);

		if (showdisassemble)
			M_NOP;
	}

	/* call trace function */

	if (runverbose) {
		/* save integer argument registers */

		for (i = 0, j = 1; i < md->paramcount && i < INT_ARG_CNT; i++)
			if (IS_INT_LNG_TYPE(md->paramtypes[i].type))
				M_LST(rd->argintregs[i], REG_SP, j++ * 8);

		/* save and copy float arguments into integer registers */

		for (i = 0; i < md->paramcount && i < FLT_ARG_CNT; i++) {
			t = md->paramtypes[i].type;

			if (IS_FLT_DBL_TYPE(t)) {
				if (IS_2_WORD_TYPE(t)) {
					M_DST(rd->argfltregs[i], REG_SP, j * 8);
					M_LLD(rd->argintregs[i], REG_SP, j * 8);
				} else {
					M_FST(rd->argfltregs[i], REG_SP, j * 8);
					M_ILD(rd->argintregs[i], REG_SP, j * 8);
				}
				j++;
			}
		}

		off = dseg_addaddress(cd, m);
		M_ALD(REG_ITMP1, REG_PV, off);
		M_AST(REG_ITMP1, REG_SP, 0 * 8);
		off = dseg_addaddress(cd, builtin_trace_args);
		M_ALD(REG_PV, REG_PV, off);
		M_JSR(REG_RA, REG_PV);
		disp = (s4) ((u1 *) mcodeptr - cd->mcodebase);
		M_LDA(REG_PV, REG_RA, -disp);

		for (i = 0, j = 1; i < md->paramcount && i < INT_ARG_CNT; i++)
			if (IS_INT_LNG_TYPE(md->paramtypes[i].type))
				M_LLD(rd->argintregs[i], REG_SP, j++ * 8);

		for (i = 0; i < md->paramcount && i < FLT_ARG_CNT; i++) {
			t = md->paramtypes[i].type;

			if (IS_FLT_DBL_TYPE(t)) {
				if (IS_2_WORD_TYPE(t)) {
					M_DLD(rd->argfltregs[i], REG_SP, j * 8);
				} else {
					M_FLD(rd->argfltregs[i], REG_SP, j * 8);
				}
				j++;
			}
		}
	}


	/* save integer and float argument registers */

	for (i = 0, j = 0; i < md->paramcount && i < INT_ARG_CNT; i++)
		if (IS_INT_LNG_TYPE(md->paramtypes[i].type))
			M_LST(rd->argintregs[i], REG_SP, j++ * 8);

	for (i = 0; i < md->paramcount && i < FLT_ARG_CNT; i++)
		if (IS_FLT_DBL_TYPE(md->paramtypes[i].type))
			M_DST(rd->argfltregs[i], REG_SP, j++ * 8);

	/* create native stack info */

	off = dseg_addaddress(cd, builtin_asm_get_stackframeinfo);
	M_ALD(REG_PV, REG_PV, off);
	M_JSR(REG_RA, REG_PV);
	disp = (s4) ((u1 *) mcodeptr - cd->mcodebase);
	M_LDA(REG_PV, REG_RA, -disp);

	M_LST(REG_RESULT, REG_SP, (stackframesize - 5) * 8); /* save adress of pointer */
	M_LLD(REG_ITMP2, REG_RESULT, 0); /* get pointer */
	M_LST(REG_ITMP2, REG_SP, (stackframesize - 6) * 8); /* save old value */
	M_LDA(REG_ITMP3, REG_SP, (stackframesize - 6) * 8); /* calculate new value */
	M_LST(REG_ITMP3, REG_RESULT, 0); /* store new value */
	off = dseg_addaddress(cd, m);
	M_LLD(REG_ITMP2, REG_PV, off);
	M_LST(REG_ITMP2, REG_SP, (stackframesize - 4) * 8);
	M_LST(REG_ZERO, REG_SP, (stackframesize - 3) * 8);
	M_LST(REG_ZERO, REG_SP, (stackframesize - 2) * 8);

	/* restore integer and float argument registers */

	for (i = 0, j = 0; i < md->paramcount && i < INT_ARG_CNT; i++)
		if (IS_INT_LNG_TYPE(md->paramtypes[i].type))
			M_LLD(rd->argintregs[i], REG_SP, j++ * 8);

	for (i = 0; i < md->paramcount && i < FLT_ARG_CNT; i++)
		if (IS_FLT_DBL_TYPE(md->paramtypes[i].type))
			M_DLD(rd->argfltregs[i], REG_SP, j++ * 8);


	/* copy or spill arguments to new locations */

	for (i = md->paramcount - 1, j = i + nativeparams; i >= 0; i--, j--) {
		t = md->paramtypes[i].type;

		if (IS_INT_LNG_TYPE(t)) {
			if (!md->params[i].inmemory) {
				s1 = rd->argintregs[md->params[i].regoff];

				if (!nmd->params[j].inmemory) {
					s2 = rd->argintregs[nmd->params[j].regoff];
					M_INTMOVE(s1, s2);

				} else {
					s2 = nmd->params[j].regoff;
					M_LST(s1, REG_SP, s2 * 8);
				}

			} else {
				s1 = md->params[i].regoff + stackframesize;
				s2 = nmd->params[j].regoff;
				M_LLD(REG_ITMP1, REG_SP, s1 * 8);
				M_LST(REG_ITMP1, REG_SP, s2 * 8);
			}

		} else {
			if (!md->params[i].inmemory) {
				s1 = rd->argfltregs[md->params[i].regoff];

				if (!nmd->params[j].inmemory) {
					s2 = rd->argfltregs[nmd->params[j].regoff];
					M_FLTMOVE(s1, s2);

				} else {
					s2 = nmd->params[j].regoff;
					M_DST(s1, REG_SP, s2 * 8);
				}

			} else {
				s1 = md->params[i].regoff + stackframesize;
				s2 = nmd->params[j].regoff;
				M_DLD(REG_FTMP1, REG_SP, s1 * 8);
				M_DST(REG_FTMP1, REG_SP, s2 * 8);
			}
		}
	}

	/* put class into second argument register */

	if (m->flags & ACC_STATIC) {
		off = dseg_addaddress(cd, m->class);
		M_ALD(rd->argintregs[1], REG_PV, off);
	}

	/* put env into first argument register */

	off = dseg_addaddress(cd, &env);
	M_ALD(rd->argintregs[0], REG_PV, off);

	/* do the native function call */

#if !defined(STATIC_CLASSPATH)
	if (f == NULL) {
		codegen_addpatchref(cd, mcodeptr, PATCHER_resolve_native, m);

		if (showdisassemble)
			M_NOP;
	}
#endif

	off = dseg_addaddress(cd, f);
	M_ALD(REG_PV, REG_PV, off);         /* load adress of native method       */
	M_JSR(REG_RA, REG_PV);              /* call native method                 */
	disp = (s4) ((u1 *) mcodeptr - cd->mcodebase);
	M_LDA(REG_PV, REG_RA, -disp);       /* recompute pv from ra               */


	/* remove native stack info */

	M_LLD(REG_ITMP3, REG_SP, (stackframesize - 5) * 8); /* get address of stacktrace helper pointer */
	M_LLD(REG_ITMP1, REG_SP, (stackframesize - 6) * 8); /* get old value */
	M_LST(REG_ITMP1, REG_ITMP3, 0); /* set old value */


	/* call finished trace */

	if (runverbose) {
		M_LST(REG_RESULT, REG_SP, 0 * 8);
		M_DST(REG_FRESULT, REG_SP, 1 * 8);

		off = dseg_addaddress(cd, m);
		M_ALD(rd->argintregs[0], REG_PV, off);

		M_MOV(REG_RESULT, rd->argintregs[1]);
		M_FMOV(REG_FRESULT, rd->argfltregs[2]);
		M_FMOV(REG_FRESULT, rd->argfltregs[3]);

		off = dseg_addaddress(cd, builtin_displaymethodstop);
		M_ALD(REG_PV, REG_PV, off);
		M_JSR(REG_RA, REG_PV);
		disp = (s4) ((u1 *) mcodeptr - cd->mcodebase);
		M_LDA(REG_PV, REG_RA, -disp);

		M_LLD(REG_RESULT, REG_SP, 0 * 8);
		M_DLD(REG_FRESULT, REG_SP, 1 * 8);
	}


	/* check for exception */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	if (IS_FLT_DBL_TYPE(md->returntype.type))
		M_DST(REG_FRESULT, REG_SP, 0 * 8);
	else
		M_AST(REG_RESULT, REG_SP, 0 * 8);

	off = dseg_addaddress(cd, builtin_get_exceptionptrptr);
	M_ALD(REG_PV, REG_PV, off);
	M_JSR(REG_RA, REG_PV);
	disp = (s4) ((u1 *) mcodeptr - cd->mcodebase);
	M_LDA(REG_PV, REG_RA, -disp);
	M_MOV(REG_RESULT, REG_ITMP3);

	if (IS_FLT_DBL_TYPE(md->returntype.type))
		M_DLD(REG_FRESULT, REG_SP, 0 * 8);
	else
		M_ALD(REG_RESULT, REG_SP, 0 * 8);
#else
	off = dseg_addaddress(cd, &_exceptionptr);
	M_ALD(REG_ITMP3, REG_PV, off);      /* get address of exceptionptr        */
#endif

	M_ALD(REG_ITMP1, REG_ITMP3, 0);     /* load exception into reg. itmp1     */
	M_BNEZ(REG_ITMP1, 3);               /* if no exception then return        */

	M_ALD(REG_RA, REG_SP, (stackframesize - 1) * 8); /* load return address   */

	M_LDA(REG_SP, REG_SP, stackframesize * 8);

	M_RET(REG_ZERO, REG_RA);            /* return to caller                   */

	/* handle exception */

	M_AST(REG_ZERO, REG_ITMP3, 0);      /* store NULL into exceptionptr       */

	M_ALD(REG_RA, REG_SP, (stackframesize - 1) * 8); /* load return address   */
	M_LDA(REG_ITMP2, REG_RA, -4);       /* move fault address into reg. itmp2 */

	M_LDA(REG_SP, REG_SP, stackframesize * 8);

	off = dseg_addaddress(cd, asm_handle_nat_exception);
	M_ALD(REG_ITMP3, REG_PV, off);      /* load asm exception handler address */
	M_JMP(REG_ZERO, REG_ITMP3);         /* jump to asm exception handler      */
	

	/* process patcher calls **************************************************/

	{
		patchref *pref;
		s4       *xcodeptr;
		u4        mcode;
		s4       *tmpmcodeptr;

		/* there can only be one <clinit> ref entry                           */
		pref = cd->patchrefs;

		for (pref = cd->patchrefs; pref != NULL; pref = pref->next) {
			/* Get machine code which is patched back in later. The call is   */
			/* 1 instruction word long.                                       */

			xcodeptr = (s4 *) (cd->mcodebase + pref->branchpos);
			mcode = (u4) *xcodeptr;

			/* patch in the call to call the following code (done at compile  */
			/* time)                                                          */

			tmpmcodeptr = mcodeptr;         /* save current mcodeptr          */
			mcodeptr = xcodeptr;            /* set mcodeptr to patch position */

			M_BSR(REG_ITMP3, tmpmcodeptr - (xcodeptr + 1));

			mcodeptr = tmpmcodeptr;         /* restore the current mcodeptr   */

			/* create stack frame                                             */

			M_LSUB_IMM(REG_SP, 5 * 8, REG_SP);

			/* move return address onto stack */

			M_AST(REG_ITMP3, REG_SP, 4 * 8);

			/* move pointer to java_objectheader onto stack */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			/* create a virtual java_objectheader */

			(void) dseg_addaddress(cd, get_dummyLR());          /* monitorPtr */
			off = dseg_addaddress(cd, NULL);                    /* vftbl      */

			M_LDA(REG_ITMP3, REG_PV, off);
			M_AST(REG_ITMP3, REG_SP, 3 * 8);
#else
			M_AST(REG_ZERO, REG_SP, 3 * 8);
#endif

			/* move machine code onto stack */

			off = dseg_adds4(cd, mcode);
			M_ILD(REG_ITMP3, REG_PV, off);
			M_IST(REG_ITMP3, REG_SP, 2 * 8);

			/* move class/method/field reference onto stack */

			off = dseg_addaddress(cd, pref->ref);
			M_ALD(REG_ITMP3, REG_PV, off);
			M_AST(REG_ITMP3, REG_SP, 1 * 8);

			/* move patcher function pointer onto stack */

			off = dseg_addaddress(cd, pref->patcher);
			M_ALD(REG_ITMP3, REG_PV, off);
			M_AST(REG_ITMP3, REG_SP, 0 * 8);

			off = dseg_addaddress(cd, asm_wrapper_patcher);
			M_ALD(REG_ITMP3, REG_PV, off);
			M_JMP(REG_ZERO, REG_ITMP3);
		}
	}

	codegen_finish(m, cd, (s4) ((u1 *) mcodeptr - cd->mcodebase));

	return m->entrypoint;
}


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
