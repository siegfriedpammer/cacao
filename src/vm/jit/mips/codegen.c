/* src/vm/jit/mips/codegen.c - machine code generator for MIPS

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

   Changes: Christian Thalinger
            Christian Ullrich

   Contains the codegenerator for an MIPS (R4000 or higher) processor.
   This module generates MIPS machine code for a sequence of
   intermediate code commands (ICMDs).

   $Id: codegen.c 3519 2005-10-28 14:48:18Z twisti $

*/


#include <assert.h>
#include <stdio.h>

#include "config.h"
#include "vm/types.h"

#include "md.h"
#include "md-abi.h"
#include "md-abi.inc"

#include "vm/jit/mips/arch.h"
#include "vm/jit/mips/codegen.h"

#include "cacao/cacao.h"
#include "native/native.h"
#include "vm/builtin.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/codegen.inc"
#include "vm/jit/jit.h"

#if defined(LSRA)
# include "vm/jit/lsra.h"
# include "vm/jit/lsra.inc"
#endif

#include "vm/jit/patcher.h"
#include "vm/jit/reg.h"
#include "vm/jit/reg.inc"


/* codegen *********************************************************************

   Generates machine code.

*******************************************************************************/

void codegen(methodinfo *m, codegendata *cd, registerdata *rd)
{
	s4                  len, s1, s2, s3, d, disp;
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

	/* adjust frame size for 16 byte alignment */

	if (parentargs_base & 1)
		parentargs_base++;

	/* create method header */

#if SIZEOF_VOID_P == 4
	(void) dseg_addaddress(cd, m);                          /* Filler         */
#endif
	(void) dseg_addaddress(cd, m);                          /* MethodPointer  */
	(void) dseg_adds4(cd, parentargs_base * 8);             /* FrameSize      */

#if defined(USE_THREADS)
	/* IsSync contains the offset relative to the stack pointer for the
	   argument of monitor_exit used in the exception handler. Since the
	   offset could be zero and give a wrong meaning of the flag it is
	   offset by one.
	*/

	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		(void) dseg_adds4(cd, (rd->memuse + 1) * 8);        /* IsSync         */
	else
#endif
		(void) dseg_adds4(cd, 0);                           /* IsSync         */
	                                       
	(void) dseg_adds4(cd, m->isleafmethod);                 /* IsLeaf         */
	(void) dseg_adds4(cd, INT_SAV_CNT - rd->savintreguse);  /* IntSave        */
	(void) dseg_adds4(cd, FLT_SAV_CNT - rd->savfltreguse);  /* FltSave        */
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

	/* initialize the last patcher pointer */

	cd->lastmcodeptr = (u1 *) mcodeptr;

	/* create stack frame (if necessary) */

	if (parentargs_base)
		M_LDA(REG_SP, REG_SP, -parentargs_base * 8);

	/* save return address and used callee saved registers */

	p = parentargs_base;
	if (!m->isleafmethod) {
		p--; M_AST(REG_RA, REG_SP, p * 8);
	}
	for (i = INT_SAV_CNT - 1; i >= rd->savintreguse; i--) {
		p--; M_AST(rd->savintregs[i], REG_SP, p * 8);
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
 					M_TFLTMOVE(var->type, s2, var->regoff);
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
			p = dseg_addaddress(cd, BUILTIN_staticmonitorenter);
			M_ALD(REG_ITMP3, REG_PV, p);
			M_JSR(REG_RA, REG_ITMP3);
			M_INTMOVE(REG_ITMP1, rd->argintregs[0]); /* branch delay */

		} else {
			M_BEQZ(rd->argintregs[0], 0);
			codegen_addxnullrefs(cd, mcodeptr);
			p = dseg_addaddress(cd, BUILTIN_monitorenter);
			M_ALD(REG_ITMP3, REG_PV, p);
			M_JSR(REG_RA, REG_ITMP3);
			M_AST(rd->argintregs[0], REG_SP, s1 * 8); /* br delay */
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

	/* copy argument registers to stack and call trace function */

	if (runverbose) {
		M_LDA(REG_SP, REG_SP, -(2 + INT_ARG_CNT + FLT_ARG_CNT + INT_TMP_CNT + FLT_TMP_CNT) * 8);
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
					M_LLD(rd->argintregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);

				} else {
					M_FST(rd->argfltregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);
					M_ILD(rd->argintregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);
				}

			} else {
				M_DST(rd->argfltregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);
			}
		}

		/* save temporary registers for leaf methods */

		if (m->isleafmethod) {
			for (p = 0; p < INT_TMP_CNT; p++)
				M_LST(rd->tmpintregs[p], REG_SP, (2 + INT_ARG_CNT + FLT_ARG_CNT + p) * 8);

			for (p = 0; p < FLT_TMP_CNT; p++)
				M_DST(rd->tmpfltregs[p], REG_SP, (2 + INT_ARG_CNT + FLT_ARG_CNT + INT_TMP_CNT + p) * 8);
		}

		p = dseg_addaddress(cd, m);
		M_ALD(REG_ITMP1, REG_PV, p);
		M_AST(REG_ITMP1, REG_SP, 0 * 8);
		disp = dseg_addaddress(cd, (void *) builtin_trace_args);
		M_ALD(REG_ITMP3, REG_PV, disp);
		M_JSR(REG_RA, REG_ITMP3);
		M_NOP;

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

		/* restore temporary registers for leaf methods */

		if (m->isleafmethod) {
			for (p = 0; p < INT_TMP_CNT; p++)
				M_LLD(rd->tmpintregs[p], REG_SP, (2 + INT_ARG_CNT + FLT_ARG_CNT + p) * 8);

			for (p = 0; p < FLT_TMP_CNT; p++)
				M_DLD(rd->tmpfltregs[p], REG_SP, (2 + INT_ARG_CNT + FLT_ARG_CNT + INT_TMP_CNT + p) * 8);
		}

		M_LDA(REG_SP, REG_SP, (2 + INT_ARG_CNT + FLT_ARG_CNT + INT_TMP_CNT + FLT_TMP_CNT) * 8);
	}

	}

	/* end of header generation */

	/* walk through all basic blocks */

	for (bptr = m->basicblocks; bptr != NULL; bptr = bptr->next) {

		bptr->mpc = (s4) ((u1 *) mcodeptr - cd->mcodebase);

		if (bptr->flags >= BBREACHED) {

			/* branch resolving */
			{
				branchref *bref;
				for (bref = bptr->branchrefs; bref != NULL; bref = bref->next) {
					gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
									  bref->branchpos,
									  bptr->mpc);
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

			} else {
				d = reg_of_var(rd, src, REG_IFTMP);
				if ((src->varkind != STACKVAR)) {
					s2 = src->type;
					if (IS_FLT_DBL_TYPE(s2)) {
						if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
							s1 = rd->interfaces[len][s2].regoff;
							M_TFLTMOVE(s2, s1, d);

						} else {
							M_DLD(d, REG_SP, rd->interfaces[len][s2].regoff * 8);
						}
						store_reg_to_var_flt(src, d);

					} else {
						if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
							s1 = rd->interfaces[len][s2].regoff;
							M_INTMOVE(s1,d);

						} else {
							M_LLD(d, REG_SP, rd->interfaces[len][s2].regoff * 8);
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
		currentline = 0;

		for (iptr = bptr->iinstr; len > 0; src = iptr->dst, len--, iptr++) {
			if (iptr->line != currentline) {
				dseg_addlinenumber(cd, iptr->line, (u1 *) mcodeptr);
				currentline = iptr->line;
			}

			MCODECHECK(64);   /* an instruction usually needs < 64 words      */

			switch (iptr->opc) {

		case ICMD_NOP:        /* ...  ==> ...                                 */
			break;

		case ICMD_CHECKNULL:  /* ..., objectref  ==> ..., objectref           */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_BEQZ(s1, 0);
			codegen_addxnullrefs(cd, mcodeptr);
			M_NOP;
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
			disp = dseg_addfloat(cd, iptr->val.f);
			M_FLD(d, REG_PV, disp);
			store_reg_to_var_flt(iptr->dst, d);
			break;
			
		case ICMD_DCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.d = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			disp = dseg_adddouble(cd, iptr->val.d);
			M_DLD(d, REG_PV, disp);
			store_reg_to_var_flt (iptr->dst, d);
			break;

		case ICMD_ACONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.a = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if (iptr->val.a) {
				disp = dseg_addaddress(cd, iptr->val.a);
				M_ALD(d, REG_PV, disp);
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
			if (var->flags & INMEMORY) {
				M_LLD(d, REG_SP, 8 * var->regoff);
			} else {
				M_INTMOVE(var->regoff,d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FLOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_DLOAD:      /* op1 = local variable                         */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			{
				int t2 = ((iptr->opc == ICMD_FLOAD) ? TYPE_FLT : TYPE_DBL);
				if (var->flags & INMEMORY) {
					M_CCFLD(var->type, t2, d, REG_SP, 8 * var->regoff);
				} else {
					M_CCFLTMOVE(var->type, t2, var->regoff, d);
				}
			}
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

			} else {
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
			{
				int t1 = ((iptr->opc == ICMD_FSTORE) ? TYPE_FLT : TYPE_DBL);
				if (var->flags & INMEMORY) {
					var_to_reg_flt(s1, src, REG_FTMP1);
					M_CCFST(t1, var->type, s1, REG_SP, 8 * var->regoff);

				} else {
					var_to_reg_flt(s1, src, var->regoff);
					M_CCFLTMOVE(t1, var->type, s1, var->regoff);
				}
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
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_ISUB(REG_ZERO, s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_LSUB(REG_ZERO, s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_I2L:        /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_ISLL_IMM(s1, 0, d );
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_LSLL_IMM(s1, 56, d);
			M_LSRA_IMM( d, 56, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2CHAR:   /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
            M_CZEXT(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2SHORT:  /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_LSLL_IMM(s1, 48, d);
			M_LSRA_IMM( d, 48, d);
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_IADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_IADD(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if ((iptr->val.i >= -32768) && (iptr->val.i <= 32767)) {
				M_IADD_IMM(s1, iptr->val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_IADD(s1, REG_ITMP2, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_LADD(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if ((iptr->val.l >= -32768) && (iptr->val.l <= 32767)) {
				M_LADD_IMM(s1, iptr->val.l, d);
			} else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_LADD(s1, REG_ITMP2, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_ISUB(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if ((iptr->val.i >= -32767) && (iptr->val.i <= 32768)) {
				M_IADD_IMM(s1, -iptr->val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_ISUB(s1, REG_ITMP2, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_LSUB(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if ((iptr->val.l >= -32767) && (iptr->val.l <= 32768)) {
				M_LADD_IMM(s1, -iptr->val.l, d);
			} else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_LSUB(s1, REG_ITMP2, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_IMUL(s1, s2);
			M_MFLO(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			ICONST(REG_ITMP2, iptr->val.i);
			M_IMUL(s1, REG_ITMP2);
			M_MFLO(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_LMUL(s1, s2);
			M_MFLO(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			LCONST(REG_ITMP2, iptr->val.l);
			M_LMUL(s1, REG_ITMP2);
			M_MFLO(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			gen_div_check(s2);
			M_IDIV(s1, s2);
			M_MFLO(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			gen_div_check(s2);
			M_LDIV(s1, s2);
			M_MFLO(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			gen_div_check(s2);
			M_IDIV(s1, s2);
			M_MFHI(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			gen_div_check(s2);
			M_LDIV(s1, s2);
			M_MFHI(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IDIVPOW2:   /* ..., value  ==> ..., value << constant       */
		case ICMD_LDIVPOW2:   /* val.i = constant                             */
		                      
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_LSRA_IMM(s1, 63, REG_ITMP2);
			M_LSRL_IMM(REG_ITMP2, 64 - iptr->val.i, REG_ITMP2);
			M_LADD(s1, REG_ITMP2, REG_ITMP2);
			M_LSRA_IMM(REG_ITMP2, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_ISLL(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_ISLL_IMM(s1, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_ISRA(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_ISRA_IMM(s1, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_ISRL(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_ISRL_IMM(s1, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_LSLL(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_LSLL_IMM(s1, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_LSRA(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_LSRA_IMM(s1, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_LSRL(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_LSRL_IMM(s1, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */
		case ICMD_LAND:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_AND(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 0xffff)) {
				M_AND_IMM(s1, iptr->val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_AND(s1, REG_ITMP2, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if (s1 == d) {
				M_MOV(s1, REG_ITMP1);
				s1 = REG_ITMP1;
			}
			if ((iptr->val.i >= 0) && (iptr->val.i <= 0xffff)) {
				M_AND_IMM(s1, iptr->val.i, d);
				M_BGEZ(s1, 4);
				M_NOP;
				M_ISUB(REG_ZERO, s1, d);
				M_AND_IMM(d, iptr->val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_AND(s1, REG_ITMP2, d);
				M_BGEZ(s1, 4);
				M_NOP;
				M_ISUB(REG_ZERO, s1, d);
				M_AND(d, REG_ITMP2, d);
			}
			M_ISUB(REG_ZERO, d, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if ((iptr->val.l >= 0) && (iptr->val.l <= 0xffff)) {
				M_AND_IMM(s1, iptr->val.l, d);
			} else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_AND(s1, REG_ITMP2, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if (s1 == d) {
				M_MOV(s1, REG_ITMP1);
				s1 = REG_ITMP1;
			}
			if ((iptr->val.l >= 0) && (iptr->val.l <= 0xffff)) {
				M_AND_IMM(s1, iptr->val.l, d);
				M_BGEZ(s1, 4);
				M_NOP;
				M_LSUB(REG_ZERO, s1, d);
				M_AND_IMM(d, iptr->val.l, d);
			} else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_AND(s1, REG_ITMP2, d);
				M_BGEZ(s1, 4);
				M_NOP;
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
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_OR(s1,s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 0xffff)) {
				M_OR_IMM(s1, iptr->val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_OR(s1, REG_ITMP2, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if ((iptr->val.l >= 0) && (iptr->val.l <= 0xffff)) {
				M_OR_IMM(s1, iptr->val.l, d);
			} else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_OR(s1, REG_ITMP2, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */
		case ICMD_LXOR:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_XOR(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 0xffff)) {
				M_XOR_IMM(s1, iptr->val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_XOR(s1, REG_ITMP2, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if ((iptr->val.l >= 0) && (iptr->val.l <= 0xffff)) {
				M_XOR_IMM(s1, iptr->val.l, d);
			} else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_XOR(s1, REG_ITMP2, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_LCMP:       /* ..., val1, val2  ==> ..., val1 cmp val2      */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_CMPLT(s1, s2, REG_ITMP3);
			M_CMPLT(s2, s1, REG_ITMP1);
			M_LSUB(REG_ITMP1, REG_ITMP3, d);
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_IINC:       /* ..., value  ==> ..., value + constant        */
		                      /* op1 = variable, val.i = constant             */

			var = &(rd->locals[iptr->op1][TYPE_INT]);
			if (var->flags & INMEMORY) {
				s1 = REG_ITMP1;
				M_LLD(s1, REG_SP, var->regoff * 8);
			} else
				s1 = var->regoff;
			M_IADD_IMM(s1, iptr->val.i, s1);
			if (var->flags & INMEMORY)
				M_LST(s1, REG_SP, var->regoff * 8);
			break;


		/* floating operations ************************************************/

		case ICMD_FNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP2);
			M_FNEG(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP2);
			M_DNEG(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP2);
			M_FADD(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP2);
			M_DADD(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP2);
			M_FSUB(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP2);
			M_DSUB(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP2);
			M_FMUL(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DMUL:       /* ..., val1, val2  ==> ..., val1 *** val2      */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP2);
			M_DMUL(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP2);
			M_FDIV(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP2);
			M_DDIV(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

#if 0		
		case ICMD_FREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_FDIV(s1,s2, REG_FTMP3);
			M_FLOORFL(REG_FTMP3, REG_FTMP3);
			M_CVTLF(REG_FTMP3, REG_FTMP3);
			M_FMUL(REG_FTMP3, s2, REG_FTMP3);
			M_FSUB(s1, REG_FTMP3, d);
			store_reg_to_var_flt(iptr->dst, d);
		    break;

		case ICMD_DREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_DDIV(s1,s2, REG_FTMP3);
			M_FLOORDL(REG_FTMP3, REG_FTMP3);
			M_CVTLD(REG_FTMP3, REG_FTMP3);
			M_DMUL(REG_FTMP3, s2, REG_FTMP3);
			M_DSUB(s1, REG_FTMP3, d);
			store_reg_to_var_flt(iptr->dst, d);
		    break;
#endif

		case ICMD_I2F:       /* ..., value  ==> ..., (float) value            */
		case ICMD_L2F:
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP2);
			M_MOVLD(s1, d);
			M_CVTLF(d, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_I2D:       /* ..., value  ==> ..., (double) value           */
		case ICMD_L2D:
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP2);
			M_MOVLD(s1, d);
			M_CVTLD(d, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
			
		case ICMD_F2I:       /* ..., (float) value  ==> ..., (int) value      */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			M_TRUNCFI(s1, REG_FTMP1);
			M_MOVDI(REG_FTMP1, d);
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;
		
		case ICMD_D2I:       /* ..., (double) value  ==> ..., (int) value     */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			M_TRUNCDI(s1, REG_FTMP1);
			M_MOVDI(REG_FTMP1, d);
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;
		
		case ICMD_F2L:       /* ..., (float) value  ==> ..., (long) value     */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			M_TRUNCFL(s1, REG_FTMP1);
			M_MOVDL(REG_FTMP1, d);
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_D2L:       /* ..., (double) value  ==> ..., (long) value    */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			M_TRUNCDL(s1, REG_FTMP1);
			M_MOVDL(REG_FTMP1, d);
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_F2D:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP2);
			M_CVTFD(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
					
		case ICMD_D2F:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP2);
			M_CVTDF(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
		
		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			M_FCMPULEF(s1, s2);
			M_FBT(3);
			M_LADD_IMM(REG_ZERO, 1, d);
			M_BR(4);
			M_NOP;
			M_FCMPEQF(s1, s2);
			M_LSUB_IMM(REG_ZERO, 1, d);
			M_CMOVT(REG_ZERO, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_DCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			M_FCMPULED(s1, s2);
			M_FBT(3);
			M_LADD_IMM(REG_ZERO, 1, d);
			M_BR(4);
			M_NOP;
			M_FCMPEQD(s1, s2);
			M_LSUB_IMM(REG_ZERO, 1, d);
			M_CMOVT(REG_ZERO, d);
			store_reg_to_var_int(iptr->dst, d);
			break;
			
		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			M_FCMPOLTF(s1, s2);
			M_FBF(3);
			M_LSUB_IMM(REG_ZERO, 1, d);
			M_BR(4);
			M_NOP;
			M_FCMPEQF(s1, s2);
			M_LADD_IMM(REG_ZERO, 1, d);
			M_CMOVT(REG_ZERO, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_DCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			M_FCMPOLTD(s1, s2);
			M_FBF(3);
			M_LSUB_IMM(REG_ZERO, 1, d);
			M_BR(4);
			M_NOP;
			M_FCMPEQD(s1, s2);
			M_LADD_IMM(REG_ZERO, 1, d);
			M_CMOVT(REG_ZERO, d);
			store_reg_to_var_int(iptr->dst, d);
			break;


		/* memory operations **************************************************/

		case ICMD_ARRAYLENGTH: /* ..., arrayref  ==> ..., length              */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			gen_nullptr_check(s1);
			M_ILD(d, s1, OFFSET(java_arrayheader, size));
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_BALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_AADD(s2, s1, REG_ITMP1);
			M_BLDS(d, REG_ITMP1, OFFSET(java_chararray, data[0]));
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_CALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_AADD(s2, s1, REG_ITMP1);
			M_AADD(s2, REG_ITMP1, REG_ITMP1);
			M_SLDU(d, REG_ITMP1, OFFSET(java_chararray, data[0]));
			store_reg_to_var_int(iptr->dst, d);
			break;			

		case ICMD_SALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_AADD(s2, s1, REG_ITMP1);
			M_AADD(s2, REG_ITMP1, REG_ITMP1);
			M_SLDS(d, REG_ITMP1, OFFSET(java_chararray, data[0]));
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 2, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_ILD(d, REG_ITMP1, OFFSET(java_intarray, data[0]));
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 3, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_LLD(d, REG_ITMP1, OFFSET(java_longarray, data[0]));
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 2, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_FLD(d, REG_ITMP1, OFFSET(java_floatarray, data[0]));
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 3, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_DLD(d, REG_ITMP1, OFFSET(java_doublearray, data[0]));
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_AALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, POINTERSHIFT, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_ALD(d, REG_ITMP1, OFFSET(java_objectarray, data[0]));
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_BASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			M_AADD(s2, s1, REG_ITMP1);
			M_BST(s3, REG_ITMP1, OFFSET(java_bytearray, data[0]));
			break;

		case ICMD_CASTORE:    /* ..., arrayref, index, value  ==> ...         */
		case ICMD_SASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			M_AADD(s2, s1, REG_ITMP1);
			M_AADD(s2, REG_ITMP1, REG_ITMP1);
			M_SST(s3, REG_ITMP1, OFFSET(java_chararray, data[0]));
			break;

		case ICMD_IASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			M_ASLL_IMM(s2, 2, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_IST(s3, REG_ITMP1, OFFSET(java_intarray, data[0]));
			break;

		case ICMD_LASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			M_ASLL_IMM(s2, 3, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_LST(s3, REG_ITMP1, OFFSET(java_longarray, data[0]));
			break;

		case ICMD_FASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_flt(s3, src, REG_FTMP1);
			M_ASLL_IMM(s2, 2, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_FST(s3, REG_ITMP1, OFFSET(java_floatarray, data[0]));
			break;

		case ICMD_DASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_flt(s3, src, REG_FTMP1);
			M_ASLL_IMM(s2, 3, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_DST(s3, REG_ITMP1, OFFSET(java_doublearray, data[0]));
			break;


		case ICMD_AASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
/* 			if (iptr->op1 == 0) { */
				gen_nullptr_check(s1);
				gen_bound_check;
/* 			} */
			var_to_reg_int(s3, src, REG_ITMP3);

			M_MOV(s1, rd->argintregs[0]);
			M_MOV(s3, rd->argintregs[1]);
			bte = iptr->val.a;
			disp = dseg_addaddress(cd, bte->fp);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_JSR(REG_RA, REG_ITMP3);
			M_NOP;

			M_BEQZ(REG_RESULT, 0);
			codegen_addxstorerefs(cd, mcodeptr);
			M_NOP;

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			var_to_reg_int(s3, src, REG_ITMP3);
			M_ASLL_IMM(s2, POINTERSHIFT, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_AST(s3, REG_ITMP1, OFFSET(java_objectarray, data[0]));
			break;


		case ICMD_IASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 2, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_IST(REG_ZERO, REG_ITMP1, OFFSET(java_intarray, data[0]));
			break;

		case ICMD_LASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 3, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_LST(REG_ZERO, REG_ITMP1, OFFSET(java_longarray, data[0]));
			break;

		case ICMD_AASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, POINTERSHIFT, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_AST(REG_ZERO, REG_ITMP1, OFFSET(java_objectarray, data[0]));
			break;

		case ICMD_BASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_AADD(s2, s1, REG_ITMP1);
			M_BST(REG_ZERO, REG_ITMP1, OFFSET(java_bytearray, data[0]));
			break;

		case ICMD_CASTORECONST:   /* ..., arrayref, index  ==> ...            */
		case ICMD_SASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_AADD(s2, s1, REG_ITMP1);
			M_AADD(s2, REG_ITMP1, REG_ITMP1);
			M_SST(REG_ZERO, REG_ITMP1, OFFSET(java_chararray, data[0]));
			break;


		case ICMD_GETSTATIC:  /* ...  ==> ..., value                          */
		                      /* op1 = type, val.a = field address            */

			if (!iptr->val.a) {
				disp = dseg_addaddress(cd, NULL);

				codegen_addpatchref(cd, mcodeptr,
									PATCHER_get_putstatic,
									(unresolved_field *) iptr->target, disp);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

			} else {
				fieldinfo *fi = iptr->val.a;

				disp = dseg_addaddress(cd, &(fi->value));

				if (!fi->class->initialized) {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_clinit, fi->class, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP;
					}
				}
  			}

			M_ALD(REG_ITMP1, REG_PV, disp);
			switch (iptr->op1) {
			case TYPE_INT:
				d = reg_of_var(rd, iptr->dst, REG_ITMP2);
				M_ILD_INTERN(d, REG_ITMP1, 0);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_LNG:
				d = reg_of_var(rd, iptr->dst, REG_ITMP2);
				M_LLD_INTERN(d, REG_ITMP1, 0);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_ADR:
				d = reg_of_var(rd, iptr->dst, REG_ITMP2);
				M_ALD_INTERN(d, REG_ITMP1, 0);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_FLT:
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				M_FLD_INTERN(d, REG_ITMP1, 0);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			case TYPE_DBL:				
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				M_DLD_INTERN(d, REG_ITMP1, 0);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			}
			break;

		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */
		                      /* op1 = type, val.a = field address            */

			if (!iptr->val.a) {
				disp = dseg_addaddress(cd, NULL);

				codegen_addpatchref(cd, mcodeptr,
									PATCHER_get_putstatic,
									(unresolved_field *) iptr->target, disp);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

			} else {
				fieldinfo *fi = iptr->val.a;

				disp = dseg_addaddress(cd, &(fi->value));

				if (!fi->class->initialized) {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_clinit, fi->class, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP;
					}
				}
  			}

			M_ALD(REG_ITMP1, REG_PV, disp);
			switch (iptr->op1) {
			case TYPE_INT:
				var_to_reg_int(s2, src, REG_ITMP2);
				M_IST_INTERN(s2, REG_ITMP1, 0);
				break;
			case TYPE_LNG:
				var_to_reg_int(s2, src, REG_ITMP2);
				M_LST_INTERN(s2, REG_ITMP1, 0);
				break;
			case TYPE_ADR:
				var_to_reg_int(s2, src, REG_ITMP2);
				M_AST_INTERN(s2, REG_ITMP1, 0);
				break;
			case TYPE_FLT:
				var_to_reg_flt(s2, src, REG_FTMP2);
				M_FST_INTERN(s2, REG_ITMP1, 0);
				break;
			case TYPE_DBL:
				var_to_reg_flt(s2, src, REG_FTMP2);
				M_DST_INTERN(s2, REG_ITMP1, 0);
				break;
			}
			break;

		case ICMD_PUTSTATICCONST: /* ...  ==> ...                             */
		                          /* val = value (in current instruction)     */
		                          /* op1 = type, val.a = field address (in    */
		                          /* following NOP)                           */

			if (!iptr[1].val.a) {
				disp = dseg_addaddress(cd, NULL);

				codegen_addpatchref(cd, mcodeptr,
									PATCHER_get_putstatic,
									(unresolved_field *) iptr[1].target, disp);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

			} else {
				fieldinfo *fi = iptr[1].val.a;

				disp = dseg_addaddress(cd, &(fi->value));

				if (!fi->class->initialized) {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_clinit, fi->class, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP;
					}
				}
  			}

			M_ALD(REG_ITMP1, REG_PV, disp);
			switch (iptr->op1) {
			case TYPE_INT:
				M_IST_INTERN(REG_ZERO, REG_ITMP1, 0);
				break;
			case TYPE_LNG:
				M_LST_INTERN(REG_ZERO, REG_ITMP1, 0);
				break;
			case TYPE_ADR:
				M_AST_INTERN(REG_ZERO, REG_ITMP1, 0);
				break;
			case TYPE_FLT:
				M_FST_INTERN(REG_ZERO, REG_ITMP1, 0);
				break;
			case TYPE_DBL:
				M_DST_INTERN(REG_ZERO, REG_ITMP1, 0);
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
									(unresolved_field *) iptr->target, 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

				a = 0;

			} else {
				a = ((fieldinfo *) (iptr->val.a))->offset;
			}

			switch (iptr->op1) {
			case TYPE_INT:
				d = reg_of_var(rd, iptr->dst, REG_ITMP2);
				M_ILD(d, s1, a);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_LNG:
				d = reg_of_var(rd, iptr->dst, REG_ITMP2);
				M_LLD(d, s1, a);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_ADR:
				d = reg_of_var(rd, iptr->dst, REG_ITMP2);
				M_ALD(d, s1, a);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_FLT:
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				M_FLD(d, s1, a);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			case TYPE_DBL:				
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
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
									(unresolved_field *) iptr->target, 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

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
									(unresolved_field *) iptr[1].target, 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

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

			if (iptr->val.a) {
				codegen_addpatchref(cd, mcodeptr,
									PATCHER_athrow_areturn,
									(unresolved_class *) iptr->val.a, 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}
			}

			disp = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP2, REG_PV, disp);
			M_JSR(REG_ITMP2_XPC, REG_ITMP2);
			M_NOP;
			M_NOP;              /* nop ensures that XPC is less than the end */
			                    /* of basic block                            */
			ALIGNCODENOP;
			break;

		case ICMD_GOTO:         /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */
			M_BR(0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			M_NOP;
			ALIGNCODENOP;
			break;

		case ICMD_JSR:          /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */

			dseg_addtarget(cd, (basicblock *) iptr->target);
			M_ALD(REG_ITMP1, REG_PV, -(cd->dseglen));
			M_JSR(REG_ITMP1, REG_ITMP1);        /* REG_ITMP1 = return address */
			M_NOP;
			break;
			
		case ICMD_RET:          /* ... ==> ...                                */
		                        /* op1 = local variable                       */
			var = &(rd->locals[iptr->op1][TYPE_ADR]);
			if (var->flags & INMEMORY) {
				M_ALD(REG_ITMP1, REG_SP, var->regoff * 8);
				M_RET(REG_ITMP1);
			} else
				M_RET(var->regoff);
			M_NOP;
			ALIGNCODENOP;
			break;

		case ICMD_IFNULL:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_BEQZ(s1, 0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			M_NOP;
			break;

		case ICMD_IFNONNULL:    /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_BNEZ(s1, 0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			M_NOP;
			break;

		case ICMD_IFEQ:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BEQZ(s1, 0);
			} else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_BEQ(s1, REG_ITMP2, 0);
			}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			M_NOP;
			break;

		case ICMD_IFLT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BLTZ(s1, 0);
			} else {
				if ((iptr->val.i >= -32768) && (iptr->val.i <= 32767)) {
					M_CMPLT_IMM(s1, iptr->val.i, REG_ITMP1);
				} else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMPLT(s1, REG_ITMP2, REG_ITMP1);
				}
				M_BNEZ(REG_ITMP1, 0);
			}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			M_NOP;
			break;

		case ICMD_IFLE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BLEZ(s1, 0);
				}
			else {
				if ((iptr->val.i >= -32769) && (iptr->val.i <= 32766)) {
					M_CMPLT_IMM(s1, iptr->val.i + 1, REG_ITMP1);
					M_BNEZ(REG_ITMP1, 0);
					}
				else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMPGT(s1, REG_ITMP2, REG_ITMP1);
					M_BEQZ(REG_ITMP1, 0);
					}
				}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			M_NOP;
			break;

		case ICMD_IFNE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BNEZ(s1, 0);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_BNE(s1, REG_ITMP2, 0);
				}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			M_NOP;
			break;

		case ICMD_IFGT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BGTZ(s1, 0);
				}
			else {
				if ((iptr->val.i >= -32769) && (iptr->val.i <= 32766)) {
					M_CMPLT_IMM(s1, iptr->val.i + 1, REG_ITMP1);
					M_BEQZ(REG_ITMP1, 0);
					}
				else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMPGT(s1, REG_ITMP2, REG_ITMP1);
					M_BNEZ(REG_ITMP1, 0);
					}
				}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			M_NOP;
			break;

		case ICMD_IFGE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BGEZ(s1, 0);
				}
			else {
				if ((iptr->val.i >= -32768) && (iptr->val.i <= 32767)) {
					M_CMPLT_IMM(s1, iptr->val.i, REG_ITMP1);
					}
				else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMPLT(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BEQZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_LEQ:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BEQZ(s1, 0);
				}
			else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_BEQ(s1, REG_ITMP2, 0);
				}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_LLT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BLTZ(s1, 0);
				}
			else {
				if ((iptr->val.l >= -32768) && (iptr->val.l <= 32767)) {
					M_CMPLT_IMM(s1, iptr->val.l, REG_ITMP1);
					}
				else {
					LCONST(REG_ITMP2, iptr->val.l);
					M_CMPLT(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BNEZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_LLE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BLEZ(s1, 0);
				}
			else {
				if ((iptr->val.l >= -32769) && (iptr->val.l <= 32766)) {
					M_CMPLT_IMM(s1, iptr->val.l + 1, REG_ITMP1);
					M_BNEZ(REG_ITMP1, 0);
					}
				else {
					LCONST(REG_ITMP2, iptr->val.l);
					M_CMPGT(s1, REG_ITMP2, REG_ITMP1);
					M_BEQZ(REG_ITMP1, 0);
					}
				}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_LNE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BNEZ(s1, 0);
				}
			else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_BNE(s1, REG_ITMP2, 0);
				}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_LGT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BGTZ(s1, 0);
				}
			else {
				if ((iptr->val.l >= -32769) && (iptr->val.l <= 32766)) {
					M_CMPLT_IMM(s1, iptr->val.l + 1, REG_ITMP1);
					M_BEQZ(REG_ITMP1, 0);
					}
				else {
					LCONST(REG_ITMP2, iptr->val.l);
					M_CMPGT(s1, REG_ITMP2, REG_ITMP1);
					M_BNEZ(REG_ITMP1, 0);
					}
				}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_LGE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BGEZ(s1, 0);
				}
			else {
				if ((iptr->val.l >= -32768) && (iptr->val.l <= 32767)) {
					M_CMPLT_IMM(s1, iptr->val.l, REG_ITMP1);
					}
				else {
					LCONST(REG_ITMP2, iptr->val.l);
					M_CMPLT(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BEQZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_ICMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPEQ:    /* op1 = target JavaVM pc                     */
		case ICMD_IF_ACMPEQ:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_BEQ(s1, s2, 0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_ICMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPNE:    /* op1 = target JavaVM pc                     */
		case ICMD_IF_ACMPNE:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_BNE(s1, s2, 0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_ICMPLT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPLT:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPLT(s1, s2, REG_ITMP1);
			M_BNEZ(REG_ITMP1, 0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_ICMPGT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPGT:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPGT(s1, s2, REG_ITMP1);
			M_BNEZ(REG_ITMP1, 0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_ICMPLE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPLE:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPGT(s1, s2, REG_ITMP1);
			M_BEQZ(REG_ITMP1, 0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_ICMPGE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPGE:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPLT(s1, s2, REG_ITMP1);
			M_BEQZ(REG_ITMP1, 0);
			codegen_addreference(cd, (basicblock *) iptr->target, mcodeptr);
			M_NOP;
			break;

#ifdef CONDITIONAL_LOADCONST
		/* (value xx 0) ? IFxx_ICONST : ELSE_ICONST                           */

		case ICMD_ELSE_ICONST:  /* handled by IFxx_ICONST                     */
			break;

		case ICMD_IFEQ_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
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
			} else {
				ICONST(REG_ITMP2, s3);
				M_CMOVEQ(s1, REG_ITMP2, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFNE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
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
			} else {
				ICONST(REG_ITMP2, s3);
				M_CMOVNE(s1, REG_ITMP2, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFLT_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
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
			} else {
				ICONST(REG_ITMP2, s3);
				M_CMOVLT(s1, REG_ITMP2, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFGE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
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
			} else {
				ICONST(REG_ITMP2, s3);
				M_CMOVGE(s1, REG_ITMP2, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFGT_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
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
			} else {
				ICONST(REG_ITMP2, s3);
				M_CMOVGT(s1, REG_ITMP2, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFLE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
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
			} else {
				ICONST(REG_ITMP2, s3);
				M_CMOVLE(s1, REG_ITMP2, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;
#endif


		case ICMD_IRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_LRETURN:

			var_to_reg_int(s1, src, REG_RESULT);
			M_INTMOVE(s1, REG_RESULT);
			goto nowperformreturn;

		case ICMD_ARETURN:      /* ..., retvalue ==> ...                      */

			var_to_reg_int(s1, src, REG_RESULT);
			M_INTMOVE(s1, REG_RESULT);

			if (iptr->val.a) {
				codegen_addpatchref(cd, mcodeptr,
									PATCHER_athrow_areturn,
									(unresolved_class *) iptr->val.a, 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}
			}
			goto nowperformreturn;

	    case ICMD_FRETURN:      /* ..., retvalue ==> ...                      */
	    case ICMD_DRETURN:
			var_to_reg_flt(s1, src, REG_FRESULT);
			{
				int t = ((iptr->opc == ICMD_FRETURN) ? TYPE_FLT : TYPE_DBL);
				M_TFLTMOVE(t, s1, REG_FRESULT);
			}
			goto nowperformreturn;

		case ICMD_RETURN:      /* ...  ==> ...                                */

nowperformreturn:
			{
			s4 i, p;
			
			p = parentargs_base;
			
			/* call trace function */

			if (runverbose) {
				M_LDA(REG_SP, REG_SP, -3 * 8);
				M_LST(REG_RA, REG_SP, 0 * 8);
				M_LST(REG_RESULT, REG_SP, 1 * 8);
				M_DST(REG_FRESULT, REG_SP, 2 * 8);

				disp = dseg_addaddress(cd, m);
				M_ALD(rd->argintregs[0], REG_PV, disp);
				M_MOV(REG_RESULT, rd->argintregs[1]);
				M_FLTMOVE(REG_FRESULT, rd->argfltregs[2]);
				M_FMOV(REG_FRESULT, rd->argfltregs[3]);

				disp = dseg_addaddress(cd, (void *) builtin_displaymethodstop);
				M_ALD(REG_ITMP3, REG_PV, disp);
				M_JSR(REG_RA, REG_ITMP3);
				M_NOP;

				M_DLD(REG_FRESULT, REG_SP, 2 * 8);
				M_LLD(REG_RESULT, REG_SP, 1 * 8);
				M_LLD(REG_RA, REG_SP, 0 * 8);
				M_LDA(REG_SP, REG_SP, 3 * 8);
			}

#if defined(USE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				disp = dseg_addaddress(cd, (void *) builtin_monitorexit);
				M_ALD(REG_ITMP3, REG_PV, disp);

				/* we need to save the proper return value */

				switch (iptr->opc) {
				case ICMD_IRETURN:
				case ICMD_ARETURN:
				case ICMD_LRETURN:
					M_ALD(rd->argintregs[0], REG_SP, rd->memuse * 8);
					M_JSR(REG_RA, REG_ITMP3);
					M_LST(REG_RESULT, REG_SP, rd->memuse * 8);  /* delay slot */
					break;
				case ICMD_FRETURN:
				case ICMD_DRETURN:
					M_ALD(rd->argintregs[0], REG_SP, rd->memuse * 8);
					M_JSR(REG_RA, REG_ITMP3);
					M_DST(REG_FRESULT, REG_SP, rd->memuse * 8); /* delay slot */
					break;
				case ICMD_RETURN:
					M_JSR(REG_RA, REG_ITMP3);
					M_ALD(rd->argintregs[0], REG_SP, rd->memuse * 8); /* delay*/
					break;
				}

				/* and now restore the proper return value */

				switch (iptr->opc) {
				case ICMD_IRETURN:
				case ICMD_ARETURN:
				case ICMD_LRETURN:
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
				p--; M_ALD(REG_RA, REG_SP, p * 8);
			}

			/* restore saved registers                                        */

			for (i = INT_SAV_CNT - 1; i >= rd->savintreguse; i--) {
				p--; M_LLD(rd->savintregs[i], REG_SP, p * 8);
			}
			for (i = FLT_SAV_CNT - 1; i >= rd->savfltreguse; i--) {
				p--; M_DLD(rd->savfltregs[i], REG_SP, p * 8);
			}

			/* deallocate stack and return                                    */

			if (parentargs_base) {
				s4 lo, hi;

				disp = parentargs_base * 8;
				lo = (short) (disp);
				hi = (short) (((disp) - lo) >> 16);

				if (hi == 0) {
					M_RET(REG_RA);
					M_LADD_IMM(REG_SP, lo, REG_SP);             /* delay slot */
				} else {
					M_LUI(REG_ITMP3,hi);
					M_LADD_IMM(REG_ITMP3,lo,REG_ITMP3);
					M_RET(REG_RA);
					M_LADD(REG_ITMP3,REG_SP,REG_SP);            /* delay slot */
				}

			} else {
				M_RET(REG_RA);
				M_NOP;
			}

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
				M_IADD_IMM(s1, -l, REG_ITMP1);
				}
			else {
				ICONST(REG_ITMP2, l);
				M_ISUB(s1, REG_ITMP2, REG_ITMP1);
				}
			i = i - l + 1;

			/* range check */

			M_CMPULT_IMM(REG_ITMP1, i, REG_ITMP2);
			M_BEQZ(REG_ITMP2, 0);
			codegen_addreference(cd, (basicblock *) tptr[0], mcodeptr);
			M_ASLL_IMM(REG_ITMP1, POINTERSHIFT, REG_ITMP1);      /* delay slot*/

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

			M_AADD(REG_ITMP1, REG_PV, REG_ITMP2);
			M_ALD(REG_ITMP2, REG_ITMP2, -(cd->dseglen));
			M_JMP(REG_ITMP2);
			M_NOP;
			ALIGNCODENOP;
			break;


		case ICMD_LOOKUPSWITCH: /* ..., key ==> ...                           */
			{
			s4 i, /*l, */val, *s4ptr;
			void **tptr;

			tptr = (void **) iptr->target;

			s4ptr = iptr->val.a;
			/*l = s4ptr[0];*/                          /* default  */
			i = s4ptr[1];                          /* count    */
			
			MCODECHECK((i<<2)+8);
			var_to_reg_int(s1, src, REG_ITMP1);
			while (--i >= 0) {
				s4ptr += 2;
				++tptr;

				val = s4ptr[0];
				ICONST(REG_ITMP2, val);
				M_BEQ(s1, REG_ITMP2, 0);
				codegen_addreference(cd, (basicblock *) tptr[0], mcodeptr); 
				M_NOP;
				}

			M_BR(0);
			tptr = (void **) iptr->target;
			codegen_addreference(cd, (basicblock *) tptr[0], mcodeptr);
			M_NOP;
			ALIGNCODENOP;
			break;
			}


		case ICMD_BUILTIN:      /* ..., arg1 ==> ...                          */
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
			s3 = md->paramcount;

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
					} else  {
						var_to_reg_int(d, src, REG_ITMP1);
						M_LST(d, REG_SP, md->params[s3].regoff * 8);
					}

				} else {
					if (!md->params[s3].inmemory) {
						s1 = rd->argfltregs[md->params[s3].regoff];
						var_to_reg_flt(d, src, s1);
						M_TFLTMOVE(src->type, d, s1);
					} else {
						var_to_reg_flt(d, src, REG_FTMP1);
						M_DST(d, REG_SP, md->params[s3].regoff * 8);
					}
				}
			}

			switch (iptr->opc) {
			case ICMD_BUILTIN:
				if (iptr->target) {
					disp = dseg_addaddress(cd, NULL);

					codegen_addpatchref(cd, mcodeptr, bte->fp, iptr->target,
										disp);

					if (opt_showdisassemble) {
						M_NOP; M_NOP;
					}

				} else {
					disp = dseg_addaddress(cd, bte->fp);
				}

				d = md->returntype.type;

				M_ALD(REG_ITMP3, REG_PV, disp);  /* built-in-function pointer */
				M_JSR(REG_RA, REG_ITMP3);
				M_NOP;
				disp = (s4) ((u1 *) mcodeptr - cd->mcodebase);
				M_LDA(REG_PV, REG_RA, -disp);

				/* if op1 == true, we need to check for an exception */

				if (iptr->op1 == true) {
					M_BEQZ(REG_RESULT, 0);
					codegen_addxexceptionrefs(cd, mcodeptr);
					M_NOP;
				}
				break;

			case ICMD_INVOKESPECIAL:
				M_BEQZ(rd->argintregs[0], 0);
				codegen_addxnullrefs(cd, mcodeptr);
				M_NOP;
				/* fall through */

			case ICMD_INVOKESTATIC:
				if (!lm) {
					unresolved_method *um = iptr->target;

					disp = dseg_addaddress(cd, NULL);

					codegen_addpatchref(cd, mcodeptr,
										PATCHER_invokestatic_special, um, disp);

					if (opt_showdisassemble) {
						M_NOP; M_NOP;
					}

					d = um->methodref->parseddesc.md->returntype.type;

				} else {
					disp = dseg_addaddress(cd, lm->stubroutine);
					d = lm->parseddesc->returntype.type;
				}

				M_ALD(REG_PV, REG_PV, disp);          /* method pointer in pv */
				M_JSR(REG_RA, REG_PV);
				M_NOP;
				disp = (s4) ((u1 *) mcodeptr - cd->mcodebase);
				M_LDA(REG_PV, REG_RA, -disp);
				break;

			case ICMD_INVOKEVIRTUAL:
				gen_nullptr_check(rd->argintregs[0]);

				if (!lm) {
					unresolved_method *um = iptr->target;

					codegen_addpatchref(cd, mcodeptr,
										PATCHER_invokevirtual, um, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP;
					}

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
				M_JSR(REG_RA, REG_PV);
				M_NOP;
				disp = (s4) ((u1 *) mcodeptr - cd->mcodebase);
				M_LDA(REG_PV, REG_RA, -disp);
				break;

			case ICMD_INVOKEINTERFACE:
				gen_nullptr_check(rd->argintregs[0]);

				if (!lm) {
					unresolved_method *um = iptr->target;

					codegen_addpatchref(cd, mcodeptr,
										PATCHER_invokeinterface, um, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP;
					}

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
				M_JSR(REG_RA, REG_PV);
				M_NOP;
				disp = (s4) ((u1 *) mcodeptr - cd->mcodebase);
				M_LDA(REG_PV, REG_RA, -disp);
				break;
			}

			/* d contains return type */

			if (d != TYPE_VOID) {
				if (IS_INT_LNG_TYPE(iptr->dst->type)) {
					s1 = reg_of_var(rd, iptr->dst, REG_RESULT);
					M_INTMOVE(REG_RESULT, s1);
					store_reg_to_var_int(iptr->dst, s1);
				} else {
					s1 = reg_of_var(rd, iptr->dst, REG_FRESULT);
					M_TFLTMOVE(iptr->dst->type, REG_FRESULT, s1);
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
			 *         super->vftbl->diffvall));
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

			s2 = 8;
			if (!super)
				s2 += (opt_showdisassemble ? 2 : 0);

			/* calculate class checkcast code size */

			s3 = 10 /* 10 + (s1 == REG_ITMP1) */;
			if (!super)
				s3 += (opt_showdisassemble ? 2 : 0);

			/* if class is not resolved, check which code to call */

			if (!super) {
				M_BEQZ(s1, 5 + (opt_showdisassemble ? 2 : 0) + s2 + 2 + s3);
				M_NOP;

				disp = dseg_adds4(cd, 0);                     /* super->flags */

				codegen_addpatchref(cd, mcodeptr,
									PATCHER_checkcast_instanceof_flags,
									(constant_classref *) iptr->target, disp);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

				/* XXX TWISTI M_ILD can be 2 instructions long (jump offset) */
				M_ILD(REG_ITMP2, REG_PV, disp);
				M_AND_IMM(REG_ITMP2, ACC_INTERFACE, REG_ITMP2);
				M_BEQZ(REG_ITMP2, 1 + s2 + 2);
				M_NOP;
			}

			/* interface checkcast code */

			if (!super || super->flags & ACC_INTERFACE) {
				if (super) {
					M_BEQZ(s1, 1 + s2);
					M_NOP;

				} else {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_checkcast_instanceof_interface,
										(constant_classref *) iptr->target, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP;
					}
				}

				M_ALD(REG_ITMP2, s1, OFFSET(java_objectheader, vftbl));
				M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl_t, interfacetablelength));
				M_IADD_IMM(REG_ITMP3, -superindex, REG_ITMP3);
				M_BLEZ(REG_ITMP3, 0);
				codegen_addxcastrefs(cd, mcodeptr);
				M_NOP;
				M_ALD(REG_ITMP3, REG_ITMP2,
					  OFFSET(vftbl_t, interfacetable[0]) -
					  superindex * sizeof(methodptr*));
				M_BEQZ(REG_ITMP3, 0);
				codegen_addxcastrefs(cd, mcodeptr);
				M_NOP;

				if (!super) {
					M_BR(1 + s3);
					M_NOP;
				}
			}

			/* class checkcast code */

			if (!super || !(super->flags & ACC_INTERFACE)) {
				disp = dseg_addaddress(cd, (void *) supervftbl);

				if (super) {
					M_BEQZ(s1, 1 + s3);
					M_NOP;

				} else {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_checkcast_instanceof_class,
										(constant_classref *) iptr->target,
										disp);

					if (opt_showdisassemble) {
						M_NOP; M_NOP;
					}
				}

				M_ALD(REG_ITMP2, s1, OFFSET(java_objectheader, vftbl));
				M_ALD(REG_ITMP3, REG_PV, disp);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
				codegen_threadcritstart(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
				M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, baseval));
/* 				if (s1 != REG_ITMP1) { */
/* 					M_ILD(REG_ITMP1, REG_ITMP3, OFFSET(vftbl_t, baseval)); */
/* 					M_ILD(REG_ITMP3, REG_ITMP3, OFFSET(vftbl_t, diffval)); */
/* #if defined(USE_THREADS) && defined(NATIVE_THREADS) */
/* 					codegen_threadcritstop(cd, (u1 *) mcodeptr - cd->mcodebase); */
/* #endif */
/* 					M_ISUB(REG_ITMP2, REG_ITMP1, REG_ITMP2); */
/* 				} else { */
					M_ILD(REG_ITMP3, REG_ITMP3, OFFSET(vftbl_t, baseval));
					M_ISUB(REG_ITMP2, REG_ITMP3, REG_ITMP2); 
					M_ALD(REG_ITMP3, REG_PV, disp);
					M_ILD(REG_ITMP3, REG_ITMP3, OFFSET(vftbl_t, diffval));
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
					codegen_threadcritstop(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
/* 				} */
				M_CMPULT(REG_ITMP3, REG_ITMP2, REG_ITMP3);
				M_BNEZ(REG_ITMP3, 0);
				codegen_addxcastrefs(cd, mcodeptr);
				M_NOP;
			}
			d = reg_of_var(rd, iptr->dst, s1);
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			}
			break;

		case ICMD_ARRAYCHECKCAST: /* ..., objectref ==> ..., objectref        */
		                          /* op1: 1... resolved, 0... not resolved    */

			var_to_reg_int(s1, src, rd->argintregs[0]);
			M_INTMOVE(s1, rd->argintregs[0]);

			bte = iptr->val.a;

			disp = dseg_addaddress(cd, iptr->target);

			if (!iptr->op1) {
				codegen_addpatchref(cd, mcodeptr, bte->fp, iptr->target, disp);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

				a = 0;

			} else {
				a = (ptrint) bte->fp;
			}

			M_ALD(rd->argintregs[1], REG_PV, disp);
			disp = dseg_addaddress(cd, a);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_JSR(REG_RA, REG_ITMP3);
			M_NOP;

			M_BEQZ(REG_RESULT, 0);
			codegen_addxcastrefs(cd, mcodeptr);
			M_NOP;

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, s1);
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
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

			s2 = 7;
			if (!super)
				s2 += (opt_showdisassemble ? 2 : 0);

			/* calculate class instanceof code size */

			s3 = 8;
			if (!super)
				s3 += (opt_showdisassemble ? 2 : 0);

			M_CLR(d);

			/* if class is not resolved, check which code to call */

			if (!super) {
				M_BEQZ(s1, 5 + (opt_showdisassemble ? 2 : 0) + s2 + 2 + s3);
				M_NOP;

				disp = dseg_adds4(cd, 0);                     /* super->flags */

				codegen_addpatchref(cd, mcodeptr,
									PATCHER_checkcast_instanceof_flags,
									(constant_classref *) iptr->target, disp);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

				/* XXX TWISTI M_ILD can be 2 instructions long (jump offset) */
				M_ILD(REG_ITMP3, REG_PV, disp);
				M_AND_IMM(REG_ITMP3, ACC_INTERFACE, REG_ITMP3);
				M_BEQZ(REG_ITMP3, 1 + s2 + 2);
				M_NOP;
			}

			/* interface instanceof code */

			if (!super || (super->flags & ACC_INTERFACE)) {
				if (super) {
					M_BEQZ(s1, 1 + s2);
					M_NOP;

				} else {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_checkcast_instanceof_interface,
										(constant_classref *) iptr->target, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP;
					}
				}

				M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
				M_ILD(REG_ITMP3, REG_ITMP1, OFFSET(vftbl_t, interfacetablelength));
				M_IADD_IMM(REG_ITMP3, -superindex, REG_ITMP3);
				M_BLEZ(REG_ITMP3, 3);
				M_NOP;
				M_ALD(REG_ITMP1, REG_ITMP1,
					  OFFSET(vftbl_t, interfacetable[0]) -
					  superindex * sizeof(methodptr*));
				M_CMPULT(REG_ZERO, REG_ITMP1, d);      /* REG_ITMP1 != 0  */

				if (!super) {
					M_BR(1 + s3);
					M_NOP;
				}
			}

			/* class instanceof code */

			if (!super || !(super->flags & ACC_INTERFACE)) {
				disp = dseg_addaddress(cd, supervftbl);

				if (super) {
					M_BEQZ(s1, 1 + s3);
					M_NOP;

				} else {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_checkcast_instanceof_class,
										(constant_classref *) iptr->target,
										disp);

					if (opt_showdisassemble) {
						M_NOP; M_NOP;
					}
				}

				M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
				M_ALD(REG_ITMP2, REG_PV, disp);
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
				M_CMPULT(REG_ITMP2, REG_ITMP1, d);
				M_XOR_IMM(d, 1, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			}
			break;

		case ICMD_MULTIANEWARRAY:/* ..., cnt1, [cnt2, ...] ==> ..., arrayref  */
		                      /* op1 = dimension, val.a = array descriptor    */

			/* check for negative sizes and copy sizes to stack if necessary  */

			MCODECHECK((iptr->op1 << 1) + 64);

			for (s1 = iptr->op1; --s1 >= 0; src = src->prev) {
				/* copy SAVEDVAR sizes to stack */

				if (src->varkind != ARGVAR) {
					var_to_reg_int(s2, src, REG_ITMP1);
					M_LST(s2, REG_SP, s1 * 8);
				}
			}

			/* a0 = dimension count */

			ICONST(rd->argintregs[0], iptr->op1);

			/* is patcher function set? */

			if (iptr->target) {
				disp = dseg_addaddress(cd, NULL);

				codegen_addpatchref(cd, mcodeptr,
									(functionptr) iptr->target, iptr->val.a,
									disp);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

			} else {
				disp = dseg_addaddress(cd, iptr->val.a);
			}

			/* a1 = arraydescriptor */

			M_ALD(rd->argintregs[1], REG_PV, disp);

			/* a2 = pointer to dimensions = stack pointer */

			M_INTMOVE(REG_SP, rd->argintregs[2]);

			disp = dseg_addaddress(cd, BUILTIN_multianewarray);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_JSR(REG_RA, REG_ITMP3);
			M_NOP;

			/* check for exception before result assignment */

			M_BEQZ(REG_RESULT, 0);
			codegen_addxexceptionrefs(cd, mcodeptr);
			M_NOP;

			d = reg_of_var(rd, iptr->dst, REG_RESULT);
			M_INTMOVE(REG_RESULT, d);
			store_reg_to_var_int(iptr->dst, d);
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
					M_TFLTMOVE(s2, s1, rd->interfaces[len][s2].regoff);

				} else {
					M_DST(s1, REG_SP, 8 * rd->interfaces[len][s2].regoff);
				}

			} else {
				var_to_reg_int(s1, src, REG_ITMP1);
				if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
					M_INTMOVE(s1, rd->interfaces[len][s2].regoff);

				} else {
					M_LST(s1, REG_SP, 8 * rd->interfaces[len][s2].regoff);
				}
			}
		}
		src = src->prev;
	}

	/* At the end of a basic block we may have to append some nops,
	   because the patcher stub calling code might be longer than the
	   actual instruction. So codepatching does not change the
	   following block unintentionally. */

	if ((u1 *) mcodeptr < cd->lastmcodeptr) {
		while ((u1 *) mcodeptr < cd->lastmcodeptr) {
			M_NOP;
		}
	}

	} /* if (bptr -> flags >= BBREACHED) */
	} /* for basic block */

	codegen_createlinenumbertable(cd);

	{
	s4        *xcodeptr;
	branchref *bref;

	/* generate ArithmeticException stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xdivrefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  (u1 *) xcodeptr - cd->mcodebase - 4);
			continue;
		}

		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(16);

		M_AADD_IMM(REG_PV, bref->branchpos - 4, REG_ITMP2_XPC);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr);
			M_NOP;

		} else {
			xcodeptr = mcodeptr;

			M_MOV(REG_PV, rd->argintregs[0]);
			M_MOV(REG_SP, rd->argintregs[1]);

			if (m->isleafmethod)
				M_MOV(REG_RA, rd->argintregs[2]);
			else
				M_ALD(rd->argintregs[2],
					  REG_SP, parentargs_base * 8 - SIZEOF_VOID_P);

			M_MOV(REG_ITMP2_XPC, rd->argintregs[3]);

			M_ASUB_IMM(REG_SP, 2 * 8, REG_SP);
			M_AST(REG_ITMP2_XPC, REG_SP, 0 * 8);

			if (m->isleafmethod)
				M_AST(REG_RA, REG_SP, 1 * 8);

			a = dseg_addaddress(cd, stacktrace_inline_arithmeticexception);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JSR(REG_RA, REG_ITMP3);
			M_NOP;
			M_MOV(REG_RESULT, REG_ITMP1_XPTR);

			if (m->isleafmethod)
				M_ALD(REG_RA, REG_SP, 1 * 8);

			M_ALD(REG_ITMP2_XPC, REG_SP, 0 * 8);
			M_AADD_IMM(REG_SP, 2 * 8, REG_SP);

			a = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JMP(REG_ITMP3);
			M_NOP;
		}
	}

	/* generate ArrayIndexOutOfBoundsException stubs */

	xcodeptr = NULL;

	for (bref = cd->xboundrefs; bref != NULL; bref = bref->next) {
		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(20);

		/* move index register into REG_ITMP1 */

		M_MOV(bref->reg, REG_ITMP1);
		M_AADD_IMM(REG_PV, bref->branchpos - 4, REG_ITMP2_XPC);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr);
			M_NOP;

		} else {
			xcodeptr = mcodeptr;

			M_MOV(REG_PV, rd->argintregs[0]);
			M_MOV(REG_SP, rd->argintregs[1]);

			if (m->isleafmethod)
				M_MOV(REG_RA, rd->argintregs[2]);
			else
				M_ALD(rd->argintregs[2],
					  REG_SP, parentargs_base * 8 - SIZEOF_VOID_P);

			M_MOV(REG_ITMP2_XPC, rd->argintregs[3]);
			M_MOV(REG_ITMP1, rd->argintregs[4]);

			M_ASUB_IMM(REG_SP, 2 * 8, REG_SP);
			M_AST(REG_ITMP2_XPC, REG_SP, 0 * 8);

			if (m->isleafmethod)
				M_AST(REG_RA, REG_SP, 1 * 8);

			a = dseg_addaddress(cd, stacktrace_inline_arrayindexoutofboundsexception);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JSR(REG_RA, REG_ITMP3);
			M_NOP;
			M_MOV(REG_RESULT, REG_ITMP1_XPTR);

			if (m->isleafmethod)
				M_ALD(REG_RA, REG_SP, 1 * 8);

			M_ALD(REG_ITMP2_XPC, REG_SP, 0 * 8);
			M_AADD_IMM(REG_SP, 2 * 8, REG_SP);

			a = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JMP(REG_ITMP3);
			M_NOP;
		}
	}

	/* generate ArrayStoreException stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xstorerefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  (u1 *) xcodeptr - cd->mcodebase - 4);
			continue;
		}

		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(16);

		M_AADD_IMM(REG_PV, bref->branchpos - 4, REG_ITMP2_XPC);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr);
			M_NOP;

		} else {
			xcodeptr = mcodeptr;

			M_MOV(REG_PV, rd->argintregs[0]);
			M_MOV(REG_SP, rd->argintregs[1]);
			M_ALD(rd->argintregs[2],
				  REG_SP, parentargs_base * 8 - SIZEOF_VOID_P);
			M_MOV(REG_ITMP2_XPC, rd->argintregs[3]);

			M_ASUB_IMM(REG_SP, 1 * 8, REG_SP);
			M_AST(REG_ITMP2_XPC, REG_SP, 0 * 8);

			a = dseg_addaddress(cd, stacktrace_inline_arraystoreexception);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JSR(REG_RA, REG_ITMP3);
			M_NOP;
			M_MOV(REG_RESULT, REG_ITMP1_XPTR);

			M_ALD(REG_ITMP2_XPC, REG_SP, 0 * 8);
			M_AADD_IMM(REG_SP, 1 * 8, REG_SP);

			a = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JMP(REG_ITMP3);
			M_NOP;
		}
	}

	/* generate ClassCastException stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xcastrefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  (u1 *) xcodeptr - cd->mcodebase - 4);
			continue;
		}

		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(16);

		M_AADD_IMM(REG_PV, bref->branchpos - 4, REG_ITMP2_XPC);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr);
			M_NOP;

		} else {
			xcodeptr = mcodeptr;

			M_MOV(REG_PV, rd->argintregs[0]);
			M_MOV(REG_SP, rd->argintregs[1]);

			if (m->isleafmethod)
				M_MOV(REG_RA, rd->argintregs[2]);
			else
				M_ALD(rd->argintregs[2],
					  REG_SP, parentargs_base * 8 - SIZEOF_VOID_P);

			M_MOV(REG_ITMP2_XPC, rd->argintregs[3]);

			M_ASUB_IMM(REG_SP, 2 * 8, REG_SP);
			M_AST(REG_ITMP2_XPC, REG_SP, 0 * 8);

			if (m->isleafmethod)
				M_AST(REG_RA, REG_SP, 1 * 8);

			a = dseg_addaddress(cd, stacktrace_inline_classcastexception);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JSR(REG_RA, REG_ITMP3);
			M_NOP;
			M_MOV(REG_RESULT, REG_ITMP1_XPTR);

			if (m->isleafmethod)
				M_ALD(REG_RA, REG_SP, 1 * 8);

			M_ALD(REG_ITMP2_XPC, REG_SP, 0 * 8);
			M_AADD_IMM(REG_SP, 2 * 8, REG_SP);

			a = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JMP(REG_ITMP3);
			M_NOP;
		}
	}

	/* generate NullPointerException stubs */

	xcodeptr = NULL;

	for (bref = cd->xnullrefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  (u1 *) xcodeptr - cd->mcodebase - 4);
			continue;
		}

		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(16);

		M_AADD_IMM(REG_PV, bref->branchpos - 4, REG_ITMP2_XPC);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr);
			M_NOP;

		} else {
			xcodeptr = mcodeptr;

			M_MOV(REG_PV, rd->argintregs[0]);
			M_MOV(REG_SP, rd->argintregs[1]);

			if (m->isleafmethod)
				M_MOV(REG_RA, rd->argintregs[2]);
			else
				M_ALD(rd->argintregs[2],
					  REG_SP, parentargs_base * 8 - SIZEOF_VOID_P);

			M_MOV(REG_ITMP2_XPC, rd->argintregs[3]);

			M_ASUB_IMM(REG_SP, 2 * 8, REG_SP);
			M_AST(REG_ITMP2_XPC, REG_SP, 0 * 8);

			if (m->isleafmethod)
				M_AST(REG_RA, REG_SP, 1 * 8);

			a = dseg_addaddress(cd, stacktrace_inline_nullpointerexception);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JSR(REG_RA, REG_ITMP3);
			M_NOP;
			M_MOV(REG_RESULT, REG_ITMP1_XPTR);

			if (m->isleafmethod)
				M_ALD(REG_RA, REG_SP, 1 * 8);

			M_ALD(REG_ITMP2_XPC, REG_SP, 0 * 8);
			M_AADD_IMM(REG_SP, 2 * 8, REG_SP);

			a = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JMP(REG_ITMP3);
			M_NOP;
		}
	}

	/* generate exception check stubs */

	xcodeptr = NULL;

	for (bref = cd->xexceptionrefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  (u1 *) xcodeptr - cd->mcodebase - 4);
			continue;
		}

		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(16);

		M_AADD_IMM(REG_PV, bref->branchpos - 4, REG_ITMP2_XPC);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr);
			M_NOP;

		} else {
			xcodeptr = mcodeptr;

			M_MOV(REG_PV, rd->argintregs[0]);
			M_MOV(REG_SP, rd->argintregs[1]);
			M_ALD(rd->argintregs[2],
				  REG_SP, parentargs_base * 8 - SIZEOF_VOID_P);
			M_MOV(REG_ITMP2_XPC, rd->argintregs[3]);

			M_ASUB_IMM(REG_SP, 1 * 8, REG_SP);
			M_AST(REG_ITMP2_XPC, REG_SP, 0 * 8);

			a = dseg_addaddress(cd, stacktrace_inline_fillInStackTrace);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JSR(REG_RA, REG_ITMP3);
			M_NOP;
			M_MOV(REG_RESULT, REG_ITMP1_XPTR);

			M_ALD(REG_ITMP2_XPC, REG_SP, 0 * 8);
			M_AADD_IMM(REG_SP, 1 * 8, REG_SP);

			a = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JMP(REG_ITMP3);
			M_NOP;
		}
	}

	/* generate patcher stub call code */

	{
		patchref *pref;
		u8        mcode;
		s4       *tmpmcodeptr;

		for (pref = cd->patchrefs; pref != NULL; pref = pref->next) {
			/* check code segment size */

			MCODECHECK(30);

			/* Get machine code which is patched back in later. The call is   */
			/* 2 instruction words long.                                      */

			xcodeptr = (s4 *) (cd->mcodebase + pref->branchpos);

			/* We need to split this, because an unaligned 8 byte read causes */
			/* a SIGSEGV.                                                     */

			mcode = ((u8) xcodeptr[1] << 32) + (u4) xcodeptr[0];

			/* patch in the call to call the following code (done at compile  */
			/* time)                                                          */

			tmpmcodeptr = mcodeptr;         /* save current mcodeptr          */
			mcodeptr = xcodeptr;            /* set mcodeptr to patch position */

			disp = (s4) (tmpmcodeptr - (xcodeptr + 1));

			if ((disp < (s4) 0xffff8000) || (disp > (s4) 0x00007fff)) {
				throw_cacao_exception_exit(string_java_lang_InternalError, \
										   "Jump offset is out of range: %d > +/-%d",
										   disp, 0x00007fff);
			}

			M_BR(disp);
			M_NOP;

			mcodeptr = tmpmcodeptr;         /* restore the current mcodeptr   */

			/* create stack frame */

			M_ASUB_IMM(REG_SP, 6 * 8, REG_SP);

			/* calculate return address and move it onto the stack */

			M_LDA(REG_ITMP3, REG_PV, pref->branchpos);
			M_AST(REG_ITMP3, REG_SP, 5 * 8);

			/* move pointer to java_objectheader onto stack */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			/* create a virtual java_objectheader */

			(void) dseg_addaddress(cd, get_dummyLR());          /* monitorPtr */
			disp = dseg_addaddress(cd, NULL);                   /* vftbl      */

			M_LDA(REG_ITMP3, REG_PV, disp);
			M_AST(REG_ITMP3, REG_SP, 4 * 8);
#else
			/* do nothing */
#endif

			/* move machine code onto stack */

			disp = dseg_adds8(cd, mcode);
			M_LLD(REG_ITMP3, REG_PV, disp);
			M_LST(REG_ITMP3, REG_SP, 3 * 8);

			/* move class/method/field reference onto stack */

			disp = dseg_addaddress(cd, pref->ref);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_AST(REG_ITMP3, REG_SP, 2 * 8);

			/* move data segment displacement onto stack */

			disp = dseg_adds4(cd, pref->disp);
			M_ILD(REG_ITMP3, REG_PV, disp);
			M_IST(REG_ITMP3, REG_SP, 1 * 8);

			/* move patcher function pointer onto stack */

			disp = dseg_addaddress(cd, pref->patcher);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_AST(REG_ITMP3, REG_SP, 0 * 8);

			disp = dseg_addaddress(cd, asm_wrapper_patcher);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_JMP(REG_ITMP3);
			M_NOP;
		}
	}
	}

	codegen_finish(m, cd, (s4) ((u1 *) mcodeptr - cd->mcodebase));

	docacheflush((void *) m->entrypoint, ((u1 *) mcodeptr - cd->mcodebase));
}


/* createcompilerstub **********************************************************

   Creates a stub routine which calls the compiler.
	
*******************************************************************************/

#define COMPILERSTUB_DATASIZE    2 * SIZEOF_VOID_P
#define COMPILERSTUB_CODESIZE    4 * 4

#define COMPILERSTUB_SIZE        COMPILERSTUB_DATASIZE + COMPILERSTUB_CODESIZE


functionptr createcompilerstub(methodinfo *m)
{
	ptrint *s;                          /* memory to hold the stub            */
	s4     *mcodeptr;                   /* code generation pointer            */

	s = (ptrint *) CNEW(u1, COMPILERSTUB_SIZE);

	s[0] = (ptrint) m;
	s[1] = (ptrint) asm_call_jit_compiler;

	mcodeptr = (s4 *) (s + 2);

	M_ALD(REG_ITMP1, REG_PV, -2 * SIZEOF_VOID_P); /* method pointer           */
	M_ALD(REG_PV, REG_PV, -1 * SIZEOF_VOID_P);    /* pointer to compiler      */
	M_JMP(REG_PV);
	M_NOP;

	(void) docacheflush((void *) s, (char *) mcodeptr - (char *) s);

#if defined(STATISTICS)
	if (opt_stat)
		count_cstub_len += COMPILERSTUB_SIZE;
#endif

	return (functionptr) (((u1 *) s) + COMPILERSTUB_DATASIZE);
}


/* createnativestub ************************************************************

   Creates a stub routine which calls a native method.

*******************************************************************************/

functionptr createnativestub(functionptr f, methodinfo *m, codegendata *cd,
							 registerdata *rd, methoddesc *nmd)
{
	s4         *mcodeptr;               /* code generation pointer            */
	s4          stackframesize;         /* size of stackframe if needed       */
	methoddesc *md;
	s4          nativeparams;
	s4          i, j;                   /* count variables                    */
	s4          t;
	s4          s1, s2, disp;
	s4          funcdisp;               /* displacement of the function       */


	/* initialize variables */

	md = m->parseddesc;
	nativeparams = (m->flags & ACC_STATIC) ? 2 : 1;


	/* calculate stack frame size */

	stackframesize =
		1 +                             /* return address                     */
		sizeof(stackframeinfo) / SIZEOF_VOID_P +
		sizeof(localref_table) / SIZEOF_VOID_P +
		md->paramcount +                /* for saving arguments over calls    */
		1 +                             /* for saving return address          */
		nmd->memuse;


	/* create method header */

#if SIZEOF_VOID_P == 4
	(void) dseg_addaddress(cd, m);                          /* MethodPointer  */
#endif
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

	M_LDA(REG_SP, REG_SP, -stackframesize * 8); /* build up stackframe        */
	M_AST(REG_RA, REG_SP, stackframesize * 8 - SIZEOF_VOID_P); /* store ra    */


	/* call trace function */

	if (runverbose) {
		M_LDA(REG_SP, REG_SP, -(1 + INT_ARG_CNT + FLT_ARG_CNT) * 8);

		/* save integer argument registers */

		for (i = 0; i < md->paramcount && i < INT_ARG_CNT; i++)
			if (IS_INT_LNG_TYPE(md->paramtypes[i].type))
				M_LST(rd->argintregs[i], REG_SP, (1 + i) * 8);

		/* save and copy float arguments into integer registers */

		for (i = 0; i < md->paramcount && i < FLT_ARG_CNT; i++) {
			t = md->paramtypes[i].type;

			if (IS_FLT_DBL_TYPE(t)) {
				if (IS_2_WORD_TYPE(t)) {
					M_DST(rd->argfltregs[i], REG_SP, (1 + INT_ARG_CNT + i) * 8);
					M_LLD(rd->argintregs[i], REG_SP, (1 + INT_ARG_CNT + i) * 8);
				} else {
					M_FST(rd->argfltregs[i], REG_SP, (1 + INT_ARG_CNT + i) * 8);
					M_ILD(rd->argintregs[i], REG_SP, (1 + INT_ARG_CNT + i) * 8);
				}
			}
		}

		disp = dseg_addaddress(cd, m);
		M_ALD(REG_ITMP1, REG_PV, disp);
		M_AST(REG_ITMP1, REG_SP, 0 * 8);
		disp = dseg_addaddress(cd, builtin_trace_args);
		M_ALD(REG_ITMP3, REG_PV, disp);
		M_JSR(REG_RA, REG_ITMP3);
		M_NOP;

		for (i = 0; i < md->paramcount && i < INT_ARG_CNT; i++)
			if (IS_INT_LNG_TYPE(md->paramtypes[i].type))
				M_LLD(rd->argintregs[i], REG_SP, (1 + i) * 8);

		for (i = 0; i < md->paramcount && i < FLT_ARG_CNT; i++) {
			t = md->paramtypes[i].type;

			if (IS_FLT_DBL_TYPE(t)) {
				if (IS_2_WORD_TYPE(t)) {
					M_DLD(rd->argfltregs[i], REG_SP, (1 + INT_ARG_CNT + i) * 8);
				} else {
					M_FLD(rd->argfltregs[i], REG_SP, (1 + INT_ARG_CNT + i) * 8);
				}
			}
		}

		M_LDA(REG_SP, REG_SP, (1 + INT_ARG_CNT + FLT_ARG_CNT) * 8);
	}


	/* get function address (this must happen before the stackframeinfo) */

	funcdisp = dseg_addaddress(cd, f);

#if !defined(ENABLE_STATICVM)
	if (f == NULL) {
		codegen_addpatchref(cd, mcodeptr, PATCHER_resolve_native, m, funcdisp);

		if (opt_showdisassemble) {
			M_NOP; M_NOP;
		}
	}
#endif

	/* save integer and float argument registers */

	for (i = 0, j = 0; i < md->paramcount && i < INT_ARG_CNT; i++) {
		if (IS_INT_LNG_TYPE(md->paramtypes[i].type)) {
			M_LST(rd->argintregs[i], REG_SP, j * 8);
			j++;
		}
	}

	for (i = 0; i < md->paramcount && i < FLT_ARG_CNT; i++) {
		if (IS_FLT_DBL_TYPE(md->paramtypes[i].type)) {
			M_DST(rd->argfltregs[i], REG_SP, j * 8);
			j++;
		}
	}

	/* prepare data structures for native function call */

	M_AADD_IMM(REG_SP, stackframesize * 8 - SIZEOF_VOID_P, rd->argintregs[0]);
	M_MOV(REG_PV, rd->argintregs[1]);
	M_AADD_IMM(REG_SP, stackframesize * 8, rd->argintregs[2]);
	M_ALD(rd->argintregs[3], REG_SP, stackframesize * 8 - SIZEOF_VOID_P);
	disp = dseg_addaddress(cd, codegen_start_native_call);
	M_ALD(REG_ITMP3, REG_PV, disp);
	M_JSR(REG_RA, REG_ITMP3);
	M_NOP; /* XXX fill me! */

	/* restore integer and float argument registers */

	for (i = 0, j = 0; i < md->paramcount && i < INT_ARG_CNT; i++) {
		if (IS_INT_LNG_TYPE(md->paramtypes[i].type)) {
			M_LLD(rd->argintregs[i], REG_SP, j * 8);
			j++;
		}
	}

	for (i = 0; i < md->paramcount && i < FLT_ARG_CNT; i++) {
		if (IS_FLT_DBL_TYPE(md->paramtypes[i].type)) {
			M_DLD(rd->argfltregs[i], REG_SP, j * 8);
			j++;
		}
	}

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
					M_AST(s1, REG_SP, s2 * 8);
				}

			} else {
				s1 = md->params[i].regoff + stackframesize;
				s2 = nmd->params[j].regoff;
				M_ALD(REG_ITMP1, REG_SP, s1 * 8);
				M_AST(REG_ITMP1, REG_SP, s2 * 8);
			}

		} else {
			if (!md->params[i].inmemory) {
				s1 = rd->argfltregs[md->params[i].regoff];

				if (!nmd->params[j].inmemory) {
					s2 = rd->argfltregs[nmd->params[j].regoff];
					M_TFLTMOVE(t, s1, s2);
				} else {
					s2 = nmd->params[j].regoff;
					if (IS_2_WORD_TYPE(t))
						M_DST(s1, REG_SP, s2 * 8);
					else
						M_FST(s1, REG_SP, s2 * 8);
				}

			} else {
				s1 = md->params[i].regoff + stackframesize;
				s2 = nmd->params[j].regoff;
				M_DLD(REG_FTMP1, REG_SP, s1 * 8);
				if (IS_2_WORD_TYPE(t))
					M_DST(REG_FTMP1, REG_SP, s2 * 8);
				else
					M_FST(REG_FTMP1, REG_SP, s2 * 8);
			}
		}
	}

	/* put class into second argument register */

	if (m->flags & ACC_STATIC) {
		disp = dseg_addaddress(cd, m->class);
		M_ALD(rd->argintregs[1], REG_PV, disp);
	}

	/* put env into first argument register */

	disp = dseg_addaddress(cd, &env);
	M_ALD(rd->argintregs[0], REG_PV, disp);

	/* do the native function call */

	M_ALD(REG_ITMP3, REG_PV, funcdisp); /* load adress of native method       */
	M_JSR(REG_RA, REG_ITMP3);           /* call native method                 */
	M_NOP;                              /* delay slot                         */

	/* save return value */

	if (IS_INT_LNG_TYPE(md->returntype.type))
		M_LST(REG_RESULT, REG_SP, 0 * 8);
	else
		M_DST(REG_FRESULT, REG_SP, 0 * 8);

	/* remove native stackframe info */

	M_AADD_IMM(REG_SP, stackframesize * 8 - SIZEOF_VOID_P, rd->argintregs[0]);
	disp = dseg_addaddress(cd, codegen_finish_native_call);
	M_ALD(REG_ITMP3, REG_PV, disp);
	M_JSR(REG_RA, REG_ITMP3);
	M_NOP; /* XXX fill me! */

	/* call finished trace function */

	if (runverbose) {
		if (IS_INT_LNG_TYPE(md->returntype.type))
			M_LLD(REG_RESULT, REG_SP, 0 * 8);
		else
			M_DLD(REG_FRESULT, REG_SP, 0 * 8);

		disp = dseg_addaddress(cd, m);
		M_ALD(rd->argintregs[0], REG_PV, disp);

		M_MOV(REG_RESULT, rd->argintregs[1]);
		M_DMFC1(REG_ITMP1, REG_FRESULT);
		M_DMTC1(REG_ITMP1, rd->argfltregs[2]);
		M_DMTC1(REG_ITMP1, rd->argfltregs[3]);

		disp = dseg_addaddress(cd, builtin_displaymethodstop);
		M_ALD(REG_ITMP3, REG_PV, disp);
		M_JSR(REG_RA, REG_ITMP3);
		M_NOP;
	}

	/* check for exception */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	disp = dseg_addaddress(cd, builtin_get_exceptionptrptr);
	M_ALD(REG_ITMP3, REG_PV, disp);
	M_JSR(REG_RA, REG_ITMP3);
	M_NOP;
	M_MOV(REG_RESULT, REG_ITMP3);
#else
	disp = dseg_addaddress(cd, &_exceptionptr);
	M_ALD(REG_ITMP3, REG_PV, disp);     /* get address of exceptionptr        */
#endif
	M_ALD(REG_ITMP1, REG_ITMP3, 0);     /* load exception into reg. itmp1     */

	M_ALD(REG_RA, REG_SP, stackframesize * 8 - SIZEOF_VOID_P); /* load ra     */

	/* restore return value */

	if (IS_INT_LNG_TYPE(md->returntype.type))
		M_LLD(REG_RESULT, REG_SP, 0 * 8);
	else
		M_DLD(REG_FRESULT, REG_SP, 0 * 8);

	M_BNEZ(REG_ITMP1, 2);               /* if no exception then return        */
	M_LDA(REG_SP, REG_SP, stackframesize * 8);/* remove stackframe, DELAY SLOT*/

	M_RET(REG_RA);                      /* return to caller                   */
	M_NOP;                              /* DELAY SLOT                         */

	/* handle exception */
	
	M_AST(REG_ZERO, REG_ITMP3, 0);      /* store NULL into exceptionptr       */

	disp = dseg_addaddress(cd, asm_handle_nat_exception);
	M_ALD(REG_ITMP3, REG_PV, disp);     /* load asm exception handler address */
	M_JMP(REG_ITMP3);                   /* jump to asm exception handler      */
	M_LDA(REG_ITMP2, REG_RA, -4);       /* move fault address into reg. itmp2 */
	                                    /* DELAY SLOT                         */

	/* generate static stub call code                                         */

	{
		patchref *pref;
		s4       *xcodeptr;
		s8        mcode;
		s4       *tmpmcodeptr;

		for (pref = cd->patchrefs; pref != NULL; pref = pref->next) {
			/* Get machine code which is patched back in later. The call is   */
			/* 2 instruction words long.                                      */

			xcodeptr = (s4 *) (cd->mcodebase + pref->branchpos);

			/* We need to split this, because an unaligned 8 byte read causes */
			/* a SIGSEGV.                                                     */

			mcode = ((u8) xcodeptr[1] << 32) + (u4) xcodeptr[0];

			/* patch in the call to call the following code (done at compile  */
			/* time)                                                          */

			tmpmcodeptr = mcodeptr;         /* save current mcodeptr          */
			mcodeptr = xcodeptr;            /* set mcodeptr to patch position */

			M_BRS(tmpmcodeptr - (xcodeptr + 1));
			M_NOP;                          /* branch delay slot              */

			mcodeptr = tmpmcodeptr;         /* restore the current mcodeptr   */

			/* create stack frame                                             */

			M_LSUB_IMM(REG_SP, 6 * 8, REG_SP);

			/* move return address onto stack */

			M_AST(REG_RA, REG_SP, 5 * 8);

			/* move pointer to java_objectheader onto stack */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			/* order reversed because of data segment layout */

			(void) dseg_addaddress(cd, get_dummyLR());          /* monitorPtr */
			disp = dseg_addaddress(cd, NULL);                   /* vftbl      */

			M_LDA(REG_ITMP3, REG_PV, disp);
			M_AST(REG_ITMP3, REG_SP, 4 * 8);
#else
			M_AST(REG_ZERO, REG_SP, 4 * 8);
#endif

			/* move machine code onto stack */

			disp = dseg_adds8(cd, mcode);
			M_LLD(REG_ITMP3, REG_PV, disp);
			M_LST(REG_ITMP3, REG_SP, 3 * 8);

			/* move class/method/field reference onto stack */

			disp = dseg_addaddress(cd, pref->ref);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_AST(REG_ITMP3, REG_SP, 2 * 8);

			/* move data segment displacement onto stack */

			disp = dseg_adds4(cd, pref->disp);
			M_ILD(REG_ITMP3, REG_PV, disp);
			M_IST(REG_ITMP3, REG_SP, 1 * 8);

			/* move patcher function pointer onto stack */

			disp = dseg_addaddress(cd, pref->patcher);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_AST(REG_ITMP3, REG_SP, 0 * 8);

			disp = dseg_addaddress(cd, asm_wrapper_patcher);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_JMP(REG_ITMP3);
			M_NOP;
		}
	}

	codegen_finish(m, cd, (s4) ((u1 *) mcodeptr - cd->mcodebase));

	docacheflush((void *) m->entrypoint, ((u1 *) mcodeptr - cd->mcodebase));

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
