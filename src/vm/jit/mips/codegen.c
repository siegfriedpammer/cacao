/* src/vm/jit/mips/codegen.c - machine code generator for MIPS

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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

   Contact: cacao@cacaojvm.org

   Authors: Andreas Krall
            Reinhard Grafl

   Changes: Christian Thalinger
            Christian Ullrich
            Edwin Steiner

   Contains the codegenerator for an MIPS (R4000 or higher) processor.
   This module generates MIPS machine code for a sequence of
   intermediate code commands (ICMDs).

   $Id: codegen.c 5564 2006-09-28 20:17:03Z edwin $

*/


#include "config.h"

#include <assert.h>
#include <stdio.h>

#include "vm/types.h"

#include "md.h"
#include "md-abi.h"

#include "vm/jit/mips/arch.h"
#include "vm/jit/mips/codegen.h"

#include "native/native.h"

#if defined(ENABLE_THREADS)
# include "threads/native/lock.h"
#endif

#include "vm/builtin.h"
#include "vm/class.h"
#include "vm/exceptions.h"
#include "vm/options.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/codegen-common.h"
#include "vm/jit/dseg.h"
#include "vm/jit/emit.h"
#include "vm/jit/jit.h"
#include "vm/jit/patcher.h"
#include "vm/jit/reg.h"
#include "vm/jit/replace.h"

#if defined(ENABLE_LSRA)
# include "vm/jit/allocator/lsra.h"
#endif


/* codegen *********************************************************************

   Generates machine code.

*******************************************************************************/

bool codegen(jitdata *jd)
{
	methodinfo         *m;
	codeinfo           *code;
	codegendata        *cd;
	registerdata       *rd;
	s4                  len, s1, s2, s3, d, disp;
	ptrint              a;
	stackptr            src;
	varinfo            *var;
	basicblock         *bptr;
	instruction        *iptr;
	exceptiontable     *ex;
	u2                  currentline;
	methodinfo         *lm;             /* local methodinfo for ICMD_INVOKE*  */
	unresolved_method  *um;
	builtintable_entry *bte;
	methoddesc         *md;
	rplpoint           *replacementpoint;
	s4                  fieldtype;

	/* get required compiler data */

	m    = jd->m;
	code = jd->code;
	cd   = jd->cd;
	rd   = jd->rd;

	{
	s4 i, p, t, l;
	s4 savedregs_num;

	savedregs_num = (jd->isleafmethod) ? 0 : 1;       /* space to save the RA */

	/* space to save used callee saved registers */

	savedregs_num += (INT_SAV_CNT - rd->savintreguse);
	savedregs_num += (FLT_SAV_CNT - rd->savfltreguse);

	cd->stackframesize = rd->memuse + savedregs_num;

#if defined(ENABLE_THREADS)
	/* space to save argument of monitor_enter */

	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		cd->stackframesize++;
#endif

	/* keep stack 16-byte aligned */

	if (cd->stackframesize & 1)
		cd->stackframesize++;

	/* create method header */

#if SIZEOF_VOID_P == 4
	(void) dseg_addaddress(cd, code);                      /* Filler          */
#endif
	(void) dseg_addaddress(cd, code);                      /* CodeinfoPointer */
	(void) dseg_adds4(cd, cd->stackframesize * 8);         /* FrameSize       */

#if defined(ENABLE_THREADS)
	/* IsSync contains the offset relative to the stack pointer for the
	   argument of monitor_exit used in the exception handler. Since the
	   offset could be zero and give a wrong meaning of the flag it is
	   offset by one.
	*/

	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		(void) dseg_adds4(cd, (rd->memuse + 1) * 8);       /* IsSync          */
	else
#endif
		(void) dseg_adds4(cd, 0);                          /* IsSync          */
	                                       
	(void) dseg_adds4(cd, jd->isleafmethod);               /* IsLeaf          */
	(void) dseg_adds4(cd, INT_SAV_CNT - rd->savintreguse); /* IntSave         */
	(void) dseg_adds4(cd, FLT_SAV_CNT - rd->savfltreguse); /* FltSave         */
	dseg_addlinenumbertablesize(cd);
	(void) dseg_adds4(cd, cd->exceptiontablelength);       /* ExTableSize     */

	/* create exception table */

	for (ex = cd->exceptiontable; ex != NULL; ex = ex->down) {
		dseg_addtarget(cd, ex->start);
   		dseg_addtarget(cd, ex->end);
		dseg_addtarget(cd, ex->handler);
		(void) dseg_addaddress(cd, ex->catchtype.any);
	}
	
	/* create stack frame (if necessary) */

	if (cd->stackframesize)
		M_LDA(REG_SP, REG_SP, -cd->stackframesize * 8);

	/* save return address and used callee saved registers */

	p = cd->stackframesize;
	if (!jd->isleafmethod) {
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
 					M_LLD(var->regoff, REG_SP, (cd->stackframesize + s1) * 8);
 				} else {                             /* stack arg -> spilled  */
					var->regoff = cd->stackframesize + s1;
				}
			}

 		} else {                                     /* floating args         */
 			if (!md->params[p].inmemory) {           /* register arguments    */
				s2 = rd->argfltregs[s1];
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
					if (IS_2_WORD_TYPE(t))
						M_DMOV(s2, var->regoff);
					else
						M_FMOV(s2, var->regoff);
 				} else {			                 /* reg arg -> spilled    */
					if (IS_2_WORD_TYPE(t))
						M_DST(s2, REG_SP, var->regoff * 8);
					else
						M_FST(s2, REG_SP, var->regoff * 8);
 				}

 			} else {                                 /* stack arguments       */
 				if (!(var->flags & INMEMORY)) {      /* stack-arg -> register */
					if (IS_2_WORD_TYPE(t))
						M_DLD(var->regoff, REG_SP, (cd->stackframesize + s1) * 8);
					else
						M_FLD(var->regoff, REG_SP, (cd->stackframesize + s1) * 8);
				} else                               /* stack-arg -> spilled  */
					var->regoff = cd->stackframesize + s1;
			}
		}
	} /* end for */

	/* call monitorenter function */

#if defined(ENABLE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		/* stack offset for monitor argument */

		s1 = rd->memuse;

# if !defined(NDEBUG)
		if (opt_verbosecall) {
			M_LDA(REG_SP, REG_SP, -(INT_ARG_CNT + FLT_ARG_CNT) * 8);

			for (p = 0; p < INT_ARG_CNT; p++)
				M_LST(rd->argintregs[p], REG_SP, p * 8);

			for (p = 0; p < FLT_ARG_CNT; p++)
				M_DST(rd->argfltregs[p], REG_SP, (INT_ARG_CNT + p) * 8);

			s1 += INT_ARG_CNT + FLT_ARG_CNT;
		}
# endif

		/* get correct lock object */

		if (m->flags & ACC_STATIC) {
			p = dseg_addaddress(cd, &m->class->object.header);
			M_ALD(REG_A0, REG_PV, p);
		}
		else {
			M_BEQZ(REG_A0, 0);
			codegen_add_nullpointerexception_ref(cd);
		}

		p = dseg_addaddress(cd, LOCK_monitor_enter);
		M_ALD(REG_ITMP3, REG_PV, p);
		M_JSR(REG_RA, REG_ITMP3);
		M_AST(REG_A0, REG_SP, s1 * 8);         /* branch delay */

# if !defined(NDEBUG)
		if (opt_verbosecall) {
			for (p = 0; p < INT_ARG_CNT; p++)
				M_LLD(rd->argintregs[p], REG_SP, p * 8);

			for (p = 0; p < FLT_ARG_CNT; p++)
				M_DLD(rd->argfltregs[p], REG_SP, (INT_ARG_CNT + p) * 8);


			M_LDA(REG_SP, REG_SP, (INT_ARG_CNT + FLT_ARG_CNT) * 8);
		}
# endif
	}
#endif

#if !defined(NDEBUG)
	if (JITDATA_HAS_FLAG_VERBOSECALL(jd))
		emit_verbosecall_enter(jd);
#endif

	}

	/* end of header generation */

	replacementpoint = jd->code->rplpoints;

	/* walk through all basic blocks */

	for (bptr = jd->new_basicblocks; bptr != NULL; bptr = bptr->next) {

		/* handle replacement points */

#if 0
		if (bptr->bitflags & BBFLAG_REPLACEMENT && bptr->flags >= BBREACHED) {

			/* 8-byte align pc */
			if ((ptrint) cd->mcodeptr & 4) {
				M_NOP;
			}
			
			replacementpoint->pc = (u1*)(ptrint) (cd->mcodeptr - cd->mcodebase);
			replacementpoint++;

			assert(cd->lastmcodeptr <= cd->mcodeptr);
			cd->lastmcodeptr = cd->mcodeptr + 2 * 4;       /* br + delay slot */
		}
#endif

		/* store relative start of block */

		bptr->mpc = (s4) (cd->mcodeptr - cd->mcodebase);

		if (bptr->flags >= BBREACHED) {

			/* branch resolving */
			{
				branchref *bref;
				for (bref = bptr->branchrefs; bref != NULL; bref = bref->next) {
					gen_resolvebranch(cd->mcodebase + bref->branchpos, 
									  bref->branchpos,
									  bptr->mpc);
				}
			}

		/* copy interface registers to their destination */

		len = bptr->indepth;
		MCODECHECK(64+len);
#if defined(ENABLE_LSRA)
		if (opt_lsra) {
		while (len) {
			len--;
			src = bptr->invars[len];
			if ((len == 0) && (bptr->type != BBTYPE_STD)) {
					/* 				d = reg_of_var(m, src, REG_ITMP1); */
					if (!(src->flags & INMEMORY))
						d= src->regoff;
					else
						d=REG_ITMP1;
					M_INTMOVE(REG_ITMP1, d);
					emit_store(jd, NULL, src, d);
				}
			}
		} else {
#endif
		while (len) {
			len--;
			src = bptr->invars[len];
			if ((len == 0) && (bptr->type != BBTYPE_STD)) {
				d = codegen_reg_of_var(rd, 0, src, REG_ITMP1);
				M_INTMOVE(REG_ITMP1, d);
				emit_store(jd, NULL, src, d);

			} else {
				d = codegen_reg_of_var(rd, 0, src, REG_IFTMP);
				if ((src->varkind != STACKVAR)) {
					s2 = src->type;
					s1 = rd->interfaces[len][s2].regoff;
					if (IS_FLT_DBL_TYPE(s2)) {
						if (rd->interfaces[len][s2].flags & INMEMORY) {
							if (IS_2_WORD_TYPE(s2))
								M_DLD(d, REG_SP, s1 * 8);
							else
								M_FLD(d, REG_SP, s1 * 8);

						} else {
							if (IS_2_WORD_TYPE(s2))
								M_DMOV(s1, d);
							else
								M_FMOV(s1, d);
						}

					} else {
						if (rd->interfaces[len][s2].flags & INMEMORY)
							M_LLD(d, REG_SP, s1 * 8);
						else
							M_INTMOVE(s1,d);
					}

					emit_store(jd, NULL, src, d);
				}
			}
		}
#if defined(ENABLE_LSRA)
		}
#endif
		/* walk through all instructions */
		
		len = bptr->icount;
		currentline = 0;

		for (iptr = bptr->iinstr; len > 0; len--, iptr++) {
			if (iptr->line != currentline) {
				dseg_addlinenumber(cd, iptr->line);
				currentline = iptr->line;
			}

			MCODECHECK(64);   /* an instruction usually needs < 64 words      */

			switch (iptr->opc) {

		case ICMD_NOP:        /* ...  ==> ...                                 */
			break;

		case ICMD_CHECKNULL:  /* ..., objectref  ==> ..., objectref           */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			M_BEQZ(s1, 0);
			codegen_add_nullpointerexception_ref(cd);
			M_NOP;
			break;

		/* constant operations ************************************************/

		case ICMD_ICONST:     /* ...  ==> ..., constant                       */

			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
			ICONST(d, iptr->sx.val.i);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LCONST:     /* ...  ==> ..., constant                       */

			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
			LCONST(d, iptr->sx.val.l);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_FCONST:     /* ...  ==> ..., constant                       */

			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
			disp = dseg_addfloat(cd, iptr->sx.val.f);
			M_FLD(d, REG_PV, disp);
			emit_store_dst(jd, iptr, d);
			break;
			
		case ICMD_DCONST:     /* ...  ==> ..., constant                       */

			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
			disp = dseg_adddouble(cd, iptr->sx.val.d);
			M_DLD(d, REG_PV, disp);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_ACONST:     /* ...  ==> ..., constant                       */

			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				disp = dseg_addaddress(cd, NULL);

				codegen_addpatchref(cd, PATCHER_aconst,
									iptr->sx.val.c.ref,
								    disp);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

				M_ALD(d, REG_PV, disp);

			} else {
				if (iptr->sx.val.anyptr == NULL) {
					M_INTMOVE(REG_ZERO, d);
				} else {
					disp = dseg_addaddress(cd, iptr->sx.val.anyptr);
					M_ALD(d, REG_PV, disp);
				}
			}
			emit_store_dst(jd, iptr, d);
			break;


		/* load/store operations **********************************************/

		case ICMD_ILOAD:      /* ...  ==> ..., content of local variable      */
		                      /* s1.localindex = local variable                         */

			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
			if ((iptr->dst.var->varkind == LOCALVAR) &&
			    (iptr->dst.var->varnum == iptr->s1.localindex))
				break;
			var = &(rd->locals[iptr->s1.localindex][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY)
#if SIZEOF_VOID_P == 8
				M_LLD(d, REG_SP, var->regoff * 8);
#else
				M_ILD(d, REG_SP, var->regoff * 8);
#endif
			else
				M_INTMOVE(var->regoff,d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* s1.localindex = local variable                         */

			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
			if ((iptr->dst.var->varkind == LOCALVAR) &&
			    (iptr->dst.var->varnum == iptr->s1.localindex))
				break;
			var = &(rd->locals[iptr->s1.localindex][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY)
				M_LLD(d, REG_SP, var->regoff * 8);
			else
				M_INTMOVE(var->regoff,d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_ALOAD:      /* ...  ==> ..., content of local variable      */
		                      /* s1.localindex = local variable                         */

			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
			if ((iptr->dst.var->varkind == LOCALVAR) &&
			    (iptr->dst.var->varnum == iptr->s1.localindex))
				break;
			var = &(rd->locals[iptr->s1.localindex][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY)
				M_ALD(d, REG_SP, var->regoff * 8);
			else
				M_INTMOVE(var->regoff,d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_FLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* s1.localindex = local variable                         */

			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
			if ((iptr->dst.var->varkind == LOCALVAR) &&
			    (iptr->dst.var->varnum == iptr->s1.localindex))
				break;
			var = &(rd->locals[iptr->s1.localindex][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY)
				M_FLD(d, REG_SP, var->regoff * 8);
			else
				M_FMOV(var->regoff, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_DLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* s1.localindex = local variable                         */

			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
			if ((iptr->dst.var->varkind == LOCALVAR) &&
			    (iptr->dst.var->varnum == iptr->s1.localindex))
				break;
			var = &(rd->locals[iptr->s1.localindex][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY)
				M_DLD(d, REG_SP, var->regoff * 8);
			else
				M_DMOV(var->regoff, d);
			emit_store_dst(jd, iptr, d);
			break;


		case ICMD_ISTORE:     /* ..., value  ==> ...                          */
		                      /* dst.localindex = local variable                         */

			if ((iptr->s1.var->varkind == LOCALVAR) &&
			    (iptr->s1.var->varnum == iptr->dst.localindex))
				break;
			var = &(rd->locals[iptr->dst.localindex][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				s1 = emit_load_s1(jd, iptr, REG_ITMP1);
#if SIZEOF_VOID_P == 8
				M_LST(s1, REG_SP, var->regoff * 8);
#else
				M_IST(s1, REG_SP, var->regoff * 8);
#endif
			} else {
				s1 = emit_load_s1(jd, iptr, var->regoff);
				M_INTMOVE(s1, var->regoff);
			}
			break;

		case ICMD_LSTORE:     /* ..., value  ==> ...                          */
		                      /* dst.localindex = local variable                         */

			if ((iptr->s1.var->varkind == LOCALVAR) &&
			    (iptr->s1.var->varnum == iptr->dst.localindex))
				break;
			var = &(rd->locals[iptr->dst.localindex][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				s1 = emit_load_s1(jd, iptr, REG_ITMP1);
				M_LST(s1, REG_SP, var->regoff * 8);
			} else {
				s1 = emit_load_s1(jd, iptr, var->regoff);
				M_INTMOVE(s1, var->regoff);
			}
			break;

		case ICMD_ASTORE:     /* ..., value  ==> ...                          */
		                      /* dst.localindex = local variable                         */

			if ((iptr->s1.var->varkind == LOCALVAR) &&
			    (iptr->s1.var->varnum == iptr->dst.localindex))
				break;
			var = &(rd->locals[iptr->dst.localindex][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				s1 = emit_load_s1(jd, iptr, REG_ITMP1);
				M_AST(s1, REG_SP, var->regoff * 8);
			} else {
				s1 = emit_load_s1(jd, iptr, var->regoff);
				M_INTMOVE(s1, var->regoff);
			}
			break;

		case ICMD_FSTORE:     /* ..., value  ==> ...                          */
		                      /* dst.localindex = local variable                         */

			if ((iptr->s1.var->varkind == LOCALVAR) &&
			    (iptr->s1.var->varnum == iptr->dst.localindex))
				break;
			var = &(rd->locals[iptr->dst.localindex][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				s1 = emit_load_s1(jd, iptr, REG_FTMP1);
				M_FST(s1, REG_SP, var->regoff * 8);
			} else {
				s1 = emit_load_s1(jd, iptr, var->regoff);
				M_FMOV(s1, var->regoff);
			}
			break;

		case ICMD_DSTORE:     /* ..., value  ==> ...                          */
		                      /* dst.localindex = local variable                         */

			if ((iptr->s1.var->varkind == LOCALVAR) &&
			    (iptr->s1.var->varnum == iptr->dst.localindex))
				break;
			var = &(rd->locals[iptr->dst.localindex][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				s1 = emit_load_s1(jd, iptr, REG_FTMP1);
				M_DST(s1, REG_SP, var->regoff * 8);
			} else {
				s1 = emit_load_s1(jd, iptr, var->regoff);
				M_DMOV(s1, var->regoff);
			}
			break;


		/* pop/dup/swap operations ********************************************/

		/* attention: double and longs are only one entry in CACAO ICMDs      */

		case ICMD_POP:        /* ..., value  ==> ...                          */
		case ICMD_POP2:       /* ..., value, value  ==> ...                   */
			break;

		case ICMD_DUP:        /* ..., a ==> ..., a, a                         */

			M_COPY(iptr->s1.var, iptr->dst.var);
			break;

		case ICMD_DUP_X1:     /* ..., a, b ==> ..., b, a, b                   */

			M_COPY(iptr->dst.dupslots[  1], iptr->dst.dupslots[2+2]);
			M_COPY(iptr->dst.dupslots[  0], iptr->dst.dupslots[2+1]);
			M_COPY(iptr->dst.dupslots[2+2], iptr->dst.dupslots[2+0]);
			break;

		case ICMD_DUP_X2:     /* ..., a, b, c ==> ..., c, a, b, c             */

			M_COPY(iptr->dst.dupslots[  2], iptr->dst.dupslots[3+3]);
			M_COPY(iptr->dst.dupslots[  1], iptr->dst.dupslots[3+2]);
			M_COPY(iptr->dst.dupslots[  0], iptr->dst.dupslots[3+1]);
			M_COPY(iptr->dst.dupslots[3+3], iptr->dst.dupslots[3+0]);
			break;

		case ICMD_DUP2:       /* ..., a, b ==> ..., a, b, a, b                */

			M_COPY(iptr->dst.dupslots[  1], iptr->dst.dupslots[2+1]);
			M_COPY(iptr->dst.dupslots[  0], iptr->dst.dupslots[2+0]);
			break;

		case ICMD_DUP2_X1:    /* ..., a, b, c ==> ..., b, c, a, b, c          */

			M_COPY(iptr->dst.dupslots[  2], iptr->dst.dupslots[3+4]);
			M_COPY(iptr->dst.dupslots[  1], iptr->dst.dupslots[3+3]);
			M_COPY(iptr->dst.dupslots[  0], iptr->dst.dupslots[3+2]);
			M_COPY(iptr->dst.dupslots[3+4], iptr->dst.dupslots[3+1]);
			M_COPY(iptr->dst.dupslots[3+3], iptr->dst.dupslots[3+0]);
			break;

		case ICMD_DUP2_X2:    /* ..., a, b, c, d ==> ..., c, d, a, b, c, d    */

			M_COPY(iptr->dst.dupslots[  3], iptr->dst.dupslots[4+5]);
			M_COPY(iptr->dst.dupslots[  2], iptr->dst.dupslots[4+4]);
			M_COPY(iptr->dst.dupslots[  1], iptr->dst.dupslots[4+3]);
			M_COPY(iptr->dst.dupslots[  0], iptr->dst.dupslots[4+2]);
			M_COPY(iptr->dst.dupslots[4+5], iptr->dst.dupslots[4+1]);
			M_COPY(iptr->dst.dupslots[4+4], iptr->dst.dupslots[4+0]);
			break;

		case ICMD_SWAP:       /* ..., a, b ==> ..., b, a                      */

			M_COPY(iptr->dst.dupslots[  1], iptr->dst.dupslots[2+0]);
			M_COPY(iptr->dst.dupslots[  0], iptr->dst.dupslots[2+1]);
			break;


		/* integer operations *************************************************/

		case ICMD_INEG:       /* ..., value  ==> ..., - value                 */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1); 
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_ISUB(REG_ZERO, s1, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LNEG:       /* ..., value  ==> ..., - value                 */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_LSUB(REG_ZERO, s1, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_I2L:        /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_INTMOVE(s1, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_ISLL_IMM(s1, 0, d );
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_LSLL_IMM(s1, 56, d);
			M_LSRA_IMM( d, 56, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_INT2CHAR:   /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
            M_CZEXT(s1, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_INT2SHORT:  /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_LSLL_IMM(s1, 48, d);
			M_LSRA_IMM( d, 48, d);
			emit_store_dst(jd, iptr, d);
			break;


		case ICMD_IADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_IADD(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if ((iptr->sx.val.i >= -32768) && (iptr->sx.val.i <= 32767)) {
				M_IADD_IMM(s1, iptr->sx.val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->sx.val.i);
				M_IADD(s1, REG_ITMP2, d);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_LADD(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* sx.val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if ((iptr->sx.val.l >= -32768) && (iptr->sx.val.l <= 32767)) {
				M_LADD_IMM(s1, iptr->sx.val.l, d);
			} else {
				LCONST(REG_ITMP2, iptr->sx.val.l);
				M_LADD(s1, REG_ITMP2, d);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_ISUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_ISUB(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if ((iptr->sx.val.i >= -32767) && (iptr->sx.val.i <= 32768)) {
				M_IADD_IMM(s1, -iptr->sx.val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->sx.val.i);
				M_ISUB(s1, REG_ITMP2, d);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_LSUB(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* sx.val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if ((iptr->sx.val.l >= -32767) && (iptr->sx.val.l <= 32768)) {
				M_LADD_IMM(s1, -iptr->sx.val.l, d);
			} else {
				LCONST(REG_ITMP2, iptr->sx.val.l);
				M_LSUB(s1, REG_ITMP2, d);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_IMUL(s1, s2);
			M_MFLO(d);
			M_NOP;
			M_NOP;
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			ICONST(REG_ITMP2, iptr->sx.val.i);
			M_IMUL(s1, REG_ITMP2);
			M_MFLO(d);
			M_NOP;
			M_NOP;
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_LMUL(s1, s2);
			M_MFLO(d);
			M_NOP;
			M_NOP;
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* sx.val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			LCONST(REG_ITMP2, iptr->sx.val.l);
			M_LMUL(s1, REG_ITMP2);
			M_MFLO(d);
			M_NOP;
			M_NOP;
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			gen_div_check(s2);
			M_IDIV(s1, s2);
			M_MFLO(d);
			M_NOP;
			M_NOP;
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			gen_div_check(s2);
			M_LDIV(s1, s2);
			M_MFLO(d);
			M_NOP;
			M_NOP;
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			gen_div_check(s2);
			M_IDIV(s1, s2);
			M_MFHI(d);
			M_NOP;
			M_NOP;
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			gen_div_check(s2);
			M_LDIV(s1, s2);
			M_MFHI(d);
			M_NOP;
			M_NOP;
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IDIVPOW2:   /* ..., value  ==> ..., value << constant       */
		case ICMD_LDIVPOW2:   /* val.i = constant                             */
		                      
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_LSRA_IMM(s1, 63, REG_ITMP2);
			M_LSRL_IMM(REG_ITMP2, 64 - iptr->sx.val.i, REG_ITMP2);
			M_LADD(s1, REG_ITMP2, REG_ITMP2);
			M_LSRA_IMM(REG_ITMP2, iptr->sx.val.i, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* sx.val.i = constant                          */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if (s1 == d) {
				M_MOV(s1, REG_ITMP1);
				s1 = REG_ITMP1;
			}
			if ((iptr->sx.val.i >= 0) && (iptr->sx.val.i <= 0xffff)) {
				M_AND_IMM(s1, iptr->sx.val.i, d);
				M_BGEZ(s1, 4);
				M_NOP;
				M_ISUB(REG_ZERO, s1, d);
				M_AND_IMM(d, iptr->sx.val.i, d);
			}
			else {
				ICONST(REG_ITMP2, iptr->sx.val.i);
				M_AND(s1, REG_ITMP2, d);
				M_BGEZ(s1, 4);
				M_NOP;
				M_ISUB(REG_ZERO, s1, d);
				M_AND(d, REG_ITMP2, d);
			}
			M_ISUB(REG_ZERO, d, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* sx.val.l = constant                          */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if (s1 == d) {
				M_MOV(s1, REG_ITMP1);
				s1 = REG_ITMP1;
			}
			if ((iptr->sx.val.l >= 0) && (iptr->sx.val.l <= 0xffff)) {
				M_AND_IMM(s1, iptr->sx.val.l, d);
				M_BGEZ(s1, 4);
				M_NOP;
				M_LSUB(REG_ZERO, s1, d);
				M_AND_IMM(d, iptr->sx.val.l, d);
			}
			else {
				LCONST(REG_ITMP2, iptr->sx.val.l);
				M_AND(s1, REG_ITMP2, d);
				M_BGEZ(s1, 4);
				M_NOP;
				M_LSUB(REG_ZERO, s1, d);
				M_AND(d, REG_ITMP2, d);
			}
			M_LSUB(REG_ZERO, d, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_ISHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_ISLL(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_ISLL_IMM(s1, iptr->sx.val.i, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_ISRA(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_ISRA_IMM(s1, iptr->sx.val.i, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_ISRL(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_ISRL_IMM(s1, iptr->sx.val.i, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LSHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_LSLL(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LSHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_LSLL_IMM(s1, iptr->sx.val.i, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_LSRA(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_LSRA_IMM(s1, iptr->sx.val.i, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_LSRL(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_LSRL_IMM(s1, iptr->sx.val.i, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */
		case ICMD_LAND:

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_AND(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if ((iptr->sx.val.i >= 0) && (iptr->sx.val.i <= 0xffff)) {
				M_AND_IMM(s1, iptr->sx.val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->sx.val.i);
				M_AND(s1, REG_ITMP2, d);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* sx.val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if ((iptr->sx.val.l >= 0) && (iptr->sx.val.l <= 0xffff)) {
				M_AND_IMM(s1, iptr->sx.val.l, d);
			} else {
				LCONST(REG_ITMP2, iptr->sx.val.l);
				M_AND(s1, REG_ITMP2, d);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IOR:        /* ..., val1, val2  ==> ..., val1 | val2        */
		case ICMD_LOR:

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_OR(s1,s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if ((iptr->sx.val.i >= 0) && (iptr->sx.val.i <= 0xffff)) {
				M_OR_IMM(s1, iptr->sx.val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->sx.val.i);
				M_OR(s1, REG_ITMP2, d);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* sx.val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if ((iptr->sx.val.l >= 0) && (iptr->sx.val.l <= 0xffff)) {
				M_OR_IMM(s1, iptr->sx.val.l, d);
			} else {
				LCONST(REG_ITMP2, iptr->sx.val.l);
				M_OR(s1, REG_ITMP2, d);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */
		case ICMD_LXOR:

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_XOR(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if ((iptr->sx.val.i >= 0) && (iptr->sx.val.i <= 0xffff)) {
				M_XOR_IMM(s1, iptr->sx.val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->sx.val.i);
				M_XOR(s1, REG_ITMP2, d);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* sx.val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if ((iptr->sx.val.l >= 0) && (iptr->sx.val.l <= 0xffff)) {
				M_XOR_IMM(s1, iptr->sx.val.l, d);
			} else {
				LCONST(REG_ITMP2, iptr->sx.val.l);
				M_XOR(s1, REG_ITMP2, d);
			}
			emit_store_dst(jd, iptr, d);
			break;


		case ICMD_LCMP:       /* ..., val1, val2  ==> ..., val1 cmp val2      */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_CMPLT(s1, s2, REG_ITMP3);
			M_CMPLT(s2, s1, REG_ITMP1);
			M_LSUB(REG_ITMP1, REG_ITMP3, d);
			emit_store_dst(jd, iptr, d);
			break;


		case ICMD_IINC:       /* ..., value  ==> ..., value + constant        */
		                      /* s1.localindex = variable, sx.val.i = constant             */

			var = &(rd->locals[iptr->s1.localindex][TYPE_INT]);
			if (var->flags & INMEMORY) {
				s1 = REG_ITMP1;
				M_LLD(s1, REG_SP, var->regoff * 8);
			} else
				s1 = var->regoff;
			M_IADD_IMM(s1, iptr->sx.val.i, s1);
			if (var->flags & INMEMORY)
				M_LST(s1, REG_SP, var->regoff * 8);
			break;


		/* floating operations ************************************************/

		case ICMD_FNEG:       /* ..., value  ==> ..., - value                 */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP2);
			M_FNEG(s1, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP2);
			M_DNEG(s1, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP2);
			M_FADD(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_DADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP2);
			M_DADD(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_FSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP2);
			M_FSUB(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_DSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP2);
			M_DSUB(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_FMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP2);
			M_FMUL(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_DMUL:       /* ..., val1, val2  ==> ..., val1 *** val2      */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP2);
			M_DMUL(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_FDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP2);
			M_FDIV(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_DDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP2);
			M_DDIV(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

#if 0		
		case ICMD_FREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP3);
			M_FDIV(s1,s2, REG_FTMP3);
			M_FLOORFL(REG_FTMP3, REG_FTMP3);
			M_CVTLF(REG_FTMP3, REG_FTMP3);
			M_FMUL(REG_FTMP3, s2, REG_FTMP3);
			M_FSUB(s1, REG_FTMP3, d);
			emit_store_dst(jd, iptr, d);
		    break;

		case ICMD_DREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP3);
			M_DDIV(s1,s2, REG_FTMP3);
			M_FLOORDL(REG_FTMP3, REG_FTMP3);
			M_CVTLD(REG_FTMP3, REG_FTMP3);
			M_DMUL(REG_FTMP3, s2, REG_FTMP3);
			M_DSUB(s1, REG_FTMP3, d);
			emit_store_dst(jd, iptr, d);
		    break;
#endif

		case ICMD_I2F:       /* ..., value  ==> ..., (float) value            */
		case ICMD_L2F:
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP2);
			M_MOVLD(s1, d);
			M_CVTLF(d, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_I2D:       /* ..., value  ==> ..., (double) value           */
		case ICMD_L2D:
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP2);
			M_MOVLD(s1, d);
			M_CVTLD(d, d);
			emit_store_dst(jd, iptr, d);
			break;

#if 0
		/* XXX these do not work correctly */

		case ICMD_F2I:       /* ..., (float) value  ==> ..., (int) value      */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_TRUNCFI(s1, REG_FTMP1);
			M_MOVDI(REG_FTMP1, d);
			M_NOP;
			emit_store_dst(jd, iptr, d);
			break;
		
		case ICMD_D2I:       /* ..., (double) value  ==> ..., (int) value     */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
			M_TRUNCDI(s1, REG_FTMP1);
			M_MOVDI(REG_FTMP1, d);
			M_NOP;
			emit_store_dst(jd, iptr, d);
			break;
		
		case ICMD_F2L:       /* ..., (float) value  ==> ..., (long) value     */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
			M_TRUNCFL(s1, REG_FTMP1);
			M_MOVDL(REG_FTMP1, d);
			M_NOP;
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_D2L:       /* ..., (double) value  ==> ..., (long) value    */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
			M_TRUNCDL(s1, REG_FTMP1);
			M_MOVDL(REG_FTMP1, d);
			M_NOP;
			emit_store_dst(jd, iptr, d);
			break;
#endif

		case ICMD_F2D:       /* ..., value  ==> ..., (double) value           */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP2);
			M_CVTFD(s1, d);
			emit_store_dst(jd, iptr, d);
			break;
					
		case ICMD_D2F:       /* ..., value  ==> ..., (double) value           */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP2);
			M_CVTDF(s1, d);
			emit_store_dst(jd, iptr, d);
			break;
		
		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
			M_FCMPULEF(s1, s2);
			M_FBT(3);
			M_LADD_IMM(REG_ZERO, 1, d);
			M_BR(4);
			M_NOP;
			M_FCMPEQF(s1, s2);
			M_LSUB_IMM(REG_ZERO, 1, d);
			M_CMOVT(REG_ZERO, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_DCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
			M_FCMPULED(s1, s2);
			M_FBT(3);
			M_LADD_IMM(REG_ZERO, 1, d);
			M_BR(4);
			M_NOP;
			M_FCMPEQD(s1, s2);
			M_LSUB_IMM(REG_ZERO, 1, d);
			M_CMOVT(REG_ZERO, d);
			emit_store_dst(jd, iptr, d);
			break;
			
		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
			M_FCMPOLTF(s1, s2);
			M_FBF(3);
			M_LSUB_IMM(REG_ZERO, 1, d);
			M_BR(4);
			M_NOP;
			M_FCMPEQF(s1, s2);
			M_LADD_IMM(REG_ZERO, 1, d);
			M_CMOVT(REG_ZERO, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_DCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
			M_FCMPOLTD(s1, s2);
			M_FBF(3);
			M_LSUB_IMM(REG_ZERO, 1, d);
			M_BR(4);
			M_NOP;
			M_FCMPEQD(s1, s2);
			M_LADD_IMM(REG_ZERO, 1, d);
			M_CMOVT(REG_ZERO, d);
			emit_store_dst(jd, iptr, d);
			break;


		/* memory operations **************************************************/

		case ICMD_ARRAYLENGTH: /* ..., arrayref  ==> ..., length              */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			gen_nullptr_check(s1);
			M_ILD(d, s1, OFFSET(java_arrayheader, size));
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_BALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if (INSTRUCTION_MUST_CHECK(iptr)) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_AADD(s2, s1, REG_ITMP3);
			M_BLDS(d, REG_ITMP3, OFFSET(java_bytearray, data[0]));
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_CALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if (INSTRUCTION_MUST_CHECK(iptr)) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_AADD(s2, s1, REG_ITMP3);
			M_AADD(s2, REG_ITMP3, REG_ITMP3);
			M_SLDU(d, REG_ITMP3, OFFSET(java_chararray, data[0]));
			emit_store_dst(jd, iptr, d);
			break;			

		case ICMD_SALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if (INSTRUCTION_MUST_CHECK(iptr)) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_AADD(s2, s1, REG_ITMP3);
			M_AADD(s2, REG_ITMP3, REG_ITMP3);
			M_SLDS(d, REG_ITMP3, OFFSET(java_shortarray, data[0]));
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if (INSTRUCTION_MUST_CHECK(iptr)) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 2, REG_ITMP3);
			M_AADD(REG_ITMP3, s1, REG_ITMP3);
			M_ILD(d, REG_ITMP3, OFFSET(java_intarray, data[0]));
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if (INSTRUCTION_MUST_CHECK(iptr)) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 3, REG_ITMP3);
			M_AADD(REG_ITMP3, s1, REG_ITMP3);
			M_LLD(d, REG_ITMP3, OFFSET(java_longarray, data[0]));
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_FALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
			if (INSTRUCTION_MUST_CHECK(iptr)) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 2, REG_ITMP3);
			M_AADD(REG_ITMP3, s1, REG_ITMP3);
			M_FLD(d, REG_ITMP3, OFFSET(java_floatarray, data[0]));
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_DALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
			if (INSTRUCTION_MUST_CHECK(iptr)) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 3, REG_ITMP3);
			M_AADD(REG_ITMP3, s1, REG_ITMP3);
			M_DLD(d, REG_ITMP3, OFFSET(java_doublearray, data[0]));
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_AALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if (INSTRUCTION_MUST_CHECK(iptr)) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, POINTERSHIFT, REG_ITMP3);
			M_AADD(REG_ITMP3, s1, REG_ITMP3);
			M_ALD(d, REG_ITMP3, OFFSET(java_objectarray, data[0]));
			emit_store_dst(jd, iptr, d);
			break;


		case ICMD_BASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			if (INSTRUCTION_MUST_CHECK(iptr)) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_AADD(s2, s1, REG_ITMP1);
			s3 = emit_load_s3(jd, iptr, REG_ITMP3);
			M_BST(s3, REG_ITMP1, OFFSET(java_bytearray, data[0]));
			break;

		case ICMD_CASTORE:    /* ..., arrayref, index, value  ==> ...         */
		case ICMD_SASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			if (INSTRUCTION_MUST_CHECK(iptr)) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_AADD(s2, s1, REG_ITMP1);
			M_AADD(s2, REG_ITMP1, REG_ITMP1);
			s3 = emit_load_s3(jd, iptr, REG_ITMP3);
			M_SST(s3, REG_ITMP1, OFFSET(java_chararray, data[0]));
			break;

		case ICMD_IASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			if (INSTRUCTION_MUST_CHECK(iptr)) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 2, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			s3 = emit_load_s3(jd, iptr, REG_ITMP3);
			M_IST_INTERN(s3, REG_ITMP1, OFFSET(java_intarray, data[0]));
			break;

		case ICMD_LASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			if (INSTRUCTION_MUST_CHECK(iptr)) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 3, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			s3 = emit_load_s3(jd, iptr, REG_ITMP3);
			M_LST_INTERN(s3, REG_ITMP1, OFFSET(java_longarray, data[0]));
			break;

		case ICMD_FASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			if (INSTRUCTION_MUST_CHECK(iptr)) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 2, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			s3 = emit_load_s3(jd, iptr, REG_FTMP1);
			M_FST_INTERN(s3, REG_ITMP1, OFFSET(java_floatarray, data[0]));
			break;

		case ICMD_DASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			if (INSTRUCTION_MUST_CHECK(iptr)) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 3, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			s3 = emit_load_s3(jd, iptr, REG_FTMP1);
			M_DST_INTERN(s3, REG_ITMP1, OFFSET(java_doublearray, data[0]));
			break;


		case ICMD_AASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			if (INSTRUCTION_MUST_CHECK(iptr)) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			s3 = emit_load_s3(jd, iptr, REG_ITMP3);

			M_MOV(s1, REG_A0);
			M_MOV(s3, REG_A1);
			disp = dseg_addaddress(cd, BUILTIN_canstore);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_JSR(REG_RA, REG_ITMP3);
			M_NOP;

			M_BEQZ(REG_RESULT, 0);
			codegen_add_arraystoreexception_ref(cd);
			M_NOP;

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_ASLL_IMM(s2, POINTERSHIFT, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			s3 = emit_load_s3(jd, iptr, REG_ITMP3);
			M_AST_INTERN(s3, REG_ITMP1, OFFSET(java_objectarray, data[0]));
			break;


		case ICMD_BASTORECONST:   /* ..., arrayref, index  ==> ...            */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			if (INSTRUCTION_MUST_CHECK(iptr)) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_AADD(s2, s1, REG_ITMP1);
			M_BST(REG_ZERO, REG_ITMP1, OFFSET(java_bytearray, data[0]));
			break;

		case ICMD_CASTORECONST:   /* ..., arrayref, index  ==> ...            */
		case ICMD_SASTORECONST:   /* ..., arrayref, index  ==> ...            */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			if (INSTRUCTION_MUST_CHECK(iptr)) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_AADD(s2, s1, REG_ITMP1);
			M_AADD(s2, REG_ITMP1, REG_ITMP1);
			M_SST(REG_ZERO, REG_ITMP1, OFFSET(java_chararray, data[0]));
			break;

		case ICMD_IASTORECONST:   /* ..., arrayref, index  ==> ...            */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			if (INSTRUCTION_MUST_CHECK(iptr)) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 2, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_IST_INTERN(REG_ZERO, REG_ITMP1, OFFSET(java_intarray, data[0]));
			break;

		case ICMD_LASTORECONST:   /* ..., arrayref, index  ==> ...            */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			if (INSTRUCTION_MUST_CHECK(iptr)) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 3, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_LST_INTERN(REG_ZERO, REG_ITMP1, OFFSET(java_longarray, data[0]));
			break;

		case ICMD_AASTORECONST:   /* ..., arrayref, index  ==> ...            */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			if (INSTRUCTION_MUST_CHECK(iptr)) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, POINTERSHIFT, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_AST_INTERN(REG_ZERO, REG_ITMP1, OFFSET(java_objectarray, data[0]));
			break;


		case ICMD_GETSTATIC:  /* ...  ==> ..., value                          */

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				unresolved_field *uf = iptr->sx.s23.s3.uf;

				fieldtype = uf->fieldref->parseddesc.fd->type;
				disp = dseg_addaddress(cd, NULL);

				codegen_addpatchref(cd, PATCHER_get_putstatic,
									iptr->sx.s23.s3.uf, disp);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

			} else {
				fieldinfo *fi = iptr->sx.s23.s3.fmiref->p.field;

				fieldtype = fi->type;
				disp = dseg_addaddress(cd, &(fi->value));

				if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					codegen_addpatchref(cd, PATCHER_clinit, fi->class, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP;
					}
				}
  			}

			M_ALD(REG_ITMP1, REG_PV, disp);
			switch (fieldtype) {
			case TYPE_INT:
				d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
				M_ILD_INTERN(d, REG_ITMP1, 0);
				emit_store_dst(jd, iptr, d);
				break;
			case TYPE_LNG:
				d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
				M_LLD_INTERN(d, REG_ITMP1, 0);
				emit_store_dst(jd, iptr, d);
				break;
			case TYPE_ADR:
				d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
				M_ALD_INTERN(d, REG_ITMP1, 0);
				emit_store_dst(jd, iptr, d);
				break;
			case TYPE_FLT:
				d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
				M_FLD_INTERN(d, REG_ITMP1, 0);
				emit_store_dst(jd, iptr, d);
				break;
			case TYPE_DBL:				
				d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
				M_DLD_INTERN(d, REG_ITMP1, 0);
				emit_store_dst(jd, iptr, d);
				break;
			}
			break;

		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				unresolved_field *uf = iptr->sx.s23.s3.uf;

				fieldtype = uf->fieldref->parseddesc.fd->type;
				disp = dseg_addaddress(cd, NULL);

				codegen_addpatchref(cd, PATCHER_get_putstatic,
									iptr->sx.s23.s3.uf, disp);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

			} else {
				fieldinfo *fi = iptr->sx.s23.s3.fmiref->p.field;

				fieldtype = fi->type;
				disp = dseg_addaddress(cd, &(fi->value));

				if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					codegen_addpatchref(cd, PATCHER_clinit, fi->class, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP;
					}
				}
  			}

			M_ALD(REG_ITMP1, REG_PV, disp);
			switch (fieldtype) {
			case TYPE_INT:
				s1 = emit_load_s1(jd, iptr, REG_ITMP2);
				M_IST_INTERN(s1, REG_ITMP1, 0);
				break;
			case TYPE_LNG:
				s1 = emit_load_s1(jd, iptr, REG_ITMP2);
				M_LST_INTERN(s1, REG_ITMP1, 0);
				break;
			case TYPE_ADR:
				s1 = emit_load_s1(jd, iptr, REG_ITMP2);
				M_AST_INTERN(s1, REG_ITMP1, 0);
				break;
			case TYPE_FLT:
				s1 = emit_load_s1(jd, iptr, REG_FTMP2);
				M_FST_INTERN(s1, REG_ITMP1, 0);
				break;
			case TYPE_DBL:
				s1 = emit_load_s1(jd, iptr, REG_FTMP2);
				M_DST_INTERN(s1, REG_ITMP1, 0);
				break;
			}
			break;

		case ICMD_PUTSTATICCONST: /* ...  ==> ...                             */
		                          /* val = value (in current instruction)     */
		                          /* following NOP)                           */

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				unresolved_field *uf = iptr->sx.s23.s3.uf;

				fieldtype = uf->fieldref->parseddesc.fd->type;
				disp = dseg_addaddress(cd, NULL);

				codegen_addpatchref(cd, PATCHER_get_putstatic,
									uf, disp);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

			} else {
				fieldinfo *fi = iptr->sx.s23.s3.fmiref->p.field;

				fieldtype = fi->type;
				disp = dseg_addaddress(cd, &(fi->value));

				if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					codegen_addpatchref(cd, PATCHER_clinit, fi->class, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP;
					}
				}
  			}

			M_ALD(REG_ITMP1, REG_PV, disp);
			switch (fieldtype) {
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

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			gen_nullptr_check(s1);

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				unresolved_field *uf = iptr->sx.s23.s3.uf;

				fieldtype = uf->fieldref->parseddesc.fd->type;

				codegen_addpatchref(cd, PATCHER_get_putfield,
									iptr->sx.s23.s3.uf, 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

				a = 0;

			} else {
				fieldinfo *fi = iptr->sx.s23.s3.fmiref->p.field;
				fieldtype = fi->type;
				a = fi->offset;
			}

			switch (fieldtype) {
			case TYPE_INT:
				d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
				M_ILD(d, s1, a);
				emit_store_dst(jd, iptr, d);
				break;
			case TYPE_LNG:
				d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
				M_LLD(d, s1, a);
				emit_store_dst(jd, iptr, d);
				break;
			case TYPE_ADR:
				d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
				M_ALD(d, s1, a);
				emit_store_dst(jd, iptr, d);
				break;
			case TYPE_FLT:
				d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
				M_FLD(d, s1, a);
				emit_store_dst(jd, iptr, d);
				break;
			case TYPE_DBL:				
				d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
				M_DLD(d, s1, a);
				emit_store_dst(jd, iptr, d);
				break;
			}
			break;

		case ICMD_PUTFIELD:   /* ..., objectref, value  ==> ...               */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			gen_nullptr_check(s1);

			if (!IS_FLT_DBL_TYPE(fieldtype)) {
				s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			} else {
				s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			}

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				unresolved_field *uf = iptr->sx.s23.s3.uf;

				fieldtype = uf->fieldref->parseddesc.fd->type;

				codegen_addpatchref(cd, PATCHER_get_putfield,
									iptr->sx.s23.s3.uf, 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

				a = 0;

			} else {
				fieldinfo *fi = iptr->sx.s23.s3.fmiref->p.field;
				fieldtype = fi->type;
				a = fi->offset;
			}

			switch (fieldtype) {
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
		                          /* following NOP)                           */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			gen_nullptr_check(s1);

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				unresolved_field *uf = iptr->sx.s23.s3.uf;

				fieldtype = uf->fieldref->parseddesc.fd->type;

				codegen_addpatchref(cd, PATCHER_get_putfield,
									uf, 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

				a = 0;

			} else {
				fieldinfo *fi = iptr->sx.s23.s3.fmiref->p.field;
				fieldtype = fi->type;
				a = fi->offset;
			}

			switch (fieldtype) {
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

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			M_INTMOVE(s1, REG_ITMP1_XPTR);
			
#ifdef ENABLE_VERIFIER
			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				codegen_addpatchref(cd, PATCHER_athrow_areturn,
									iptr->sx.s23.s2.uc, 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}
			}
#endif /* ENABLE_VERIFIER */

			disp = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP2, REG_PV, disp);
			M_JSR(REG_ITMP2_XPC, REG_ITMP2);
			M_NOP;
			M_NOP;              /* nop ensures that XPC is less than the end */
			                    /* of basic block                            */
			ALIGNCODENOP;
			break;

		case ICMD_GOTO:         /* ... ==> ...                                */
			M_BR(0);
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			ALIGNCODENOP;
			break;

		case ICMD_JSR:          /* ... ==> ...                                */

			dseg_addtarget(cd, iptr->sx.s23.s3.jsrtarget.block);
			M_ALD(REG_ITMP1, REG_PV, -(cd->dseglen));
			M_JSR(REG_ITMP1, REG_ITMP1);        /* REG_ITMP1 = return address */
			M_NOP;
			break;
			
		case ICMD_RET:          /* ... ==> ...                                */
		                        /* s1.localindex = local variable                       */
			var = &(rd->locals[iptr->s1.localindex][TYPE_ADR]);
			if (var->flags & INMEMORY) {
				M_ALD(REG_ITMP1, REG_SP, var->regoff * 8);
				M_RET(REG_ITMP1);
			} else
				M_RET(var->regoff);
			M_NOP;
			ALIGNCODENOP;
			break;

		case ICMD_IFNULL:       /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			M_BEQZ(s1, 0);
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IFNONNULL:    /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			M_BNEZ(s1, 0);
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IFEQ:         /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (iptr->sx.val.i == 0) {
				M_BEQZ(s1, 0);
			} else {
				ICONST(REG_ITMP2, iptr->sx.val.i);
				M_BEQ(s1, REG_ITMP2, 0);
			}
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IFLT:         /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (iptr->sx.val.i == 0) {
				M_BLTZ(s1, 0);
			} else {
				if ((iptr->sx.val.i >= -32768) && (iptr->sx.val.i <= 32767)) {
					M_CMPLT_IMM(s1, iptr->sx.val.i, REG_ITMP1);
				} else {
					ICONST(REG_ITMP2, iptr->sx.val.i);
					M_CMPLT(s1, REG_ITMP2, REG_ITMP1);
				}
				M_BNEZ(REG_ITMP1, 0);
			}
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IFLE:         /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (iptr->sx.val.i == 0) {
				M_BLEZ(s1, 0);
				}
			else {
				if ((iptr->sx.val.i >= -32769) && (iptr->sx.val.i <= 32766)) {
					M_CMPLT_IMM(s1, iptr->sx.val.i + 1, REG_ITMP1);
					M_BNEZ(REG_ITMP1, 0);
					}
				else {
					ICONST(REG_ITMP2, iptr->sx.val.i);
					M_CMPGT(s1, REG_ITMP2, REG_ITMP1);
					M_BEQZ(REG_ITMP1, 0);
					}
				}
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IFNE:         /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (iptr->sx.val.i == 0) {
				M_BNEZ(s1, 0);
				}
			else {
				ICONST(REG_ITMP2, iptr->sx.val.i);
				M_BNE(s1, REG_ITMP2, 0);
				}
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IFGT:         /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (iptr->sx.val.i == 0) {
				M_BGTZ(s1, 0);
				}
			else {
				if ((iptr->sx.val.i >= -32769) && (iptr->sx.val.i <= 32766)) {
					M_CMPLT_IMM(s1, iptr->sx.val.i + 1, REG_ITMP1);
					M_BEQZ(REG_ITMP1, 0);
					}
				else {
					ICONST(REG_ITMP2, iptr->sx.val.i);
					M_CMPGT(s1, REG_ITMP2, REG_ITMP1);
					M_BNEZ(REG_ITMP1, 0);
					}
				}
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IFGE:         /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (iptr->sx.val.i == 0) {
				M_BGEZ(s1, 0);
				}
			else {
				if ((iptr->sx.val.i >= -32768) && (iptr->sx.val.i <= 32767)) {
					M_CMPLT_IMM(s1, iptr->sx.val.i, REG_ITMP1);
					}
				else {
					ICONST(REG_ITMP2, iptr->sx.val.i);
					M_CMPLT(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BEQZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IF_LEQ:       /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (iptr->sx.val.l == 0) {
				M_BEQZ(s1, 0);
				}
			else {
				LCONST(REG_ITMP2, iptr->sx.val.l);
				M_BEQ(s1, REG_ITMP2, 0);
				}
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IF_LLT:       /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (iptr->sx.val.l == 0) {
				M_BLTZ(s1, 0);
				}
			else {
				if ((iptr->sx.val.l >= -32768) && (iptr->sx.val.l <= 32767)) {
					M_CMPLT_IMM(s1, iptr->sx.val.l, REG_ITMP1);
					}
				else {
					LCONST(REG_ITMP2, iptr->sx.val.l);
					M_CMPLT(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BNEZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IF_LLE:       /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (iptr->sx.val.l == 0) {
				M_BLEZ(s1, 0);
				}
			else {
				if ((iptr->sx.val.l >= -32769) && (iptr->sx.val.l <= 32766)) {
					M_CMPLT_IMM(s1, iptr->sx.val.l + 1, REG_ITMP1);
					M_BNEZ(REG_ITMP1, 0);
					}
				else {
					LCONST(REG_ITMP2, iptr->sx.val.l);
					M_CMPGT(s1, REG_ITMP2, REG_ITMP1);
					M_BEQZ(REG_ITMP1, 0);
					}
				}
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IF_LNE:       /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (iptr->sx.val.l == 0) {
				M_BNEZ(s1, 0);
				}
			else {
				LCONST(REG_ITMP2, iptr->sx.val.l);
				M_BNE(s1, REG_ITMP2, 0);
				}
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IF_LGT:       /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (iptr->sx.val.l == 0) {
				M_BGTZ(s1, 0);
				}
			else {
				if ((iptr->sx.val.l >= -32769) && (iptr->sx.val.l <= 32766)) {
					M_CMPLT_IMM(s1, iptr->sx.val.l + 1, REG_ITMP1);
					M_BEQZ(REG_ITMP1, 0);
					}
				else {
					LCONST(REG_ITMP2, iptr->sx.val.l);
					M_CMPGT(s1, REG_ITMP2, REG_ITMP1);
					M_BNEZ(REG_ITMP1, 0);
					}
				}
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IF_LGE:       /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (iptr->sx.val.l == 0) {
				M_BGEZ(s1, 0);
				}
			else {
				if ((iptr->sx.val.l >= -32768) && (iptr->sx.val.l <= 32767)) {
					M_CMPLT_IMM(s1, iptr->sx.val.l, REG_ITMP1);
					}
				else {
					LCONST(REG_ITMP2, iptr->sx.val.l);
					M_CMPLT(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BEQZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IF_ICMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPEQ:    /* op1 = target JavaVM pc                     */
		case ICMD_IF_ACMPEQ:

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_BEQ(s1, s2, 0);
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IF_ICMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPNE:    /* op1 = target JavaVM pc                     */
		case ICMD_IF_ACMPNE:

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_BNE(s1, s2, 0);
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IF_ICMPLT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPLT:    /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMPLT(s1, s2, REG_ITMP1);
			M_BNEZ(REG_ITMP1, 0);
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IF_ICMPGT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPGT:    /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMPGT(s1, s2, REG_ITMP1);
			M_BNEZ(REG_ITMP1, 0);
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IF_ICMPLE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPLE:    /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMPGT(s1, s2, REG_ITMP1);
			M_BEQZ(REG_ITMP1, 0);
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IF_ICMPGE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPGE:    /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMPLT(s1, s2, REG_ITMP1);
			M_BEQZ(REG_ITMP1, 0);
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;


		case ICMD_IRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_LRETURN:

			s1 = emit_load_s1(jd, iptr, REG_RESULT);
			M_INTMOVE(s1, REG_RESULT);
			goto nowperformreturn;

		case ICMD_ARETURN:      /* ..., retvalue ==> ...                      */

			s1 = emit_load_s1(jd, iptr, REG_RESULT);
			M_INTMOVE(s1, REG_RESULT);

#ifdef ENABLE_VERIFIER
			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				codegen_addpatchref(cd, PATCHER_athrow_areturn,
									iptr->sx.s23.s2.uc, 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}
			}
#endif /* ENABLE_VERIFIER */
			goto nowperformreturn;

	    case ICMD_FRETURN:      /* ..., retvalue ==> ...                      */

			s1 = emit_load_s1(jd, iptr, REG_FRESULT);
			M_FLTMOVE(s1, REG_FRESULT);
			goto nowperformreturn;

	    case ICMD_DRETURN:      /* ..., retvalue ==> ...                      */

			s1 = emit_load_s1(jd, iptr, REG_FRESULT);
			M_DBLMOVE(s1, REG_FRESULT);
			goto nowperformreturn;

		case ICMD_RETURN:      /* ...  ==> ...                                */

nowperformreturn:
			{
			s4 i, p;
			
			p = cd->stackframesize;

#if !defined(NDEBUG)
			if (JITDATA_HAS_FLAG_VERBOSECALL(jd))
				emit_verbosecall_exit(jd);
#endif

#if defined(ENABLE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				disp = dseg_addaddress(cd, LOCK_monitor_exit);
				M_ALD(REG_ITMP3, REG_PV, disp);

				/* we need to save the proper return value */

				switch (iptr->opc) {
				case ICMD_IRETURN:
				case ICMD_ARETURN:
				case ICMD_LRETURN:
					M_ALD(REG_A0, REG_SP, rd->memuse * 8);
					M_JSR(REG_RA, REG_ITMP3);
					M_LST(REG_RESULT, REG_SP, rd->memuse * 8);  /* delay slot */
					break;
				case ICMD_FRETURN:
				case ICMD_DRETURN:
					M_ALD(REG_A0, REG_SP, rd->memuse * 8);
					M_JSR(REG_RA, REG_ITMP3);
					M_DST(REG_FRESULT, REG_SP, rd->memuse * 8); /* delay slot */
					break;
				case ICMD_RETURN:
					M_JSR(REG_RA, REG_ITMP3);
					M_ALD(REG_A0, REG_SP, rd->memuse * 8); /* delay*/
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

			if (!jd->isleafmethod) {
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

			if (cd->stackframesize) {
				s4 lo, hi;

				disp = cd->stackframesize * 8;
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
			s4 i, l;
			branch_target_t *table;

			table = iptr->dst.table;

			l = iptr->sx.s23.s2.tablelow;
			i = iptr->sx.s23.s3.tablehigh;

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (l == 0)
				{M_INTMOVE(s1, REG_ITMP1);}
			else if (l <= 32768) {
				M_IADD_IMM(s1, -l, REG_ITMP1);
				}
			else {
				ICONST(REG_ITMP2, l);
				M_ISUB(s1, REG_ITMP2, REG_ITMP1);
				}

			/* number of targets */
			i = i - l + 1;

			/* range check */

			M_CMPULT_IMM(REG_ITMP1, i, REG_ITMP2);
			M_BEQZ(REG_ITMP2, 0);
			codegen_addreference(cd, table[0].block); /* default target */
			M_ASLL_IMM(REG_ITMP1, POINTERSHIFT, REG_ITMP1);      /* delay slot*/

			/* build jump table top down and use address of lowest entry */

			table += i;

			while (--i >= 0) {
				dseg_addtarget(cd, table->block); 
				--table;
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
			s4 i;
			lookup_target_t *lookup;

			lookup = iptr->dst.lookup;

			i = iptr->sx.s23.s2.lookupcount;
			
			MCODECHECK((i<<2)+8);
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);

			while (--i >= 0) {
				ICONST(REG_ITMP2, lookup->value);
				M_BEQ(s1, REG_ITMP2, 0);
				codegen_addreference(cd, lookup->target.block); 
				M_NOP;
				++lookup;
			}

			M_BR(0);
			codegen_addreference(cd, iptr->sx.s23.s3.lookupdefault.block);
			M_NOP;
			ALIGNCODENOP;
			break;
			}


		case ICMD_BUILTIN:      /* ..., arg1 ==> ...                          */

			bte = iptr->sx.s23.s3.bte;
			md  = bte->md;
			goto gen_method;

		case ICMD_INVOKESTATIC: /* ..., [arg1, [arg2 ...]] ==> ...            */

		case ICMD_INVOKESPECIAL:/* ..., objectref, [arg1, [arg2 ...]] ==> ... */
		case ICMD_INVOKEVIRTUAL:/* op1 = arg count, val.a = method pointer    */
		case ICMD_INVOKEINTERFACE:

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				lm = NULL;
				um = iptr->sx.s23.s3.um;
				md = um->methodref->parseddesc.md;
			}
			else {
				lm = iptr->sx.s23.s3.fmiref->p.method;
				um = NULL;
				md = lm->parseddesc;
			}

gen_method:
			s3 = md->paramcount;

			MCODECHECK((s3 << 1) + 64);

			/* copy arguments to registers or stack location                  */

			for (s3 = s3 - 1; s3 >= 0; s3--) {
				src = iptr->sx.s23.s2.args[s3];

				if (src->varkind == ARGVAR)
					continue;

				if (IS_INT_LNG_TYPE(src->type)) {
					if (!md->params[s3].inmemory) {
						s1 = rd->argintregs[md->params[s3].regoff];
						d = emit_load(jd, iptr, src, s1);
						M_INTMOVE(d, s1);
					}
					else  {
						d = emit_load(jd, iptr, src, REG_ITMP1);
						M_LST(d, REG_SP, md->params[s3].regoff * 8);
					}
				}
				else {
					if (!md->params[s3].inmemory) {
						s1 = rd->argfltregs[md->params[s3].regoff];
						d = emit_load(jd, iptr, src, s1);
						if (IS_2_WORD_TYPE(src->type))
							M_DMOV(d, s1);
						else
							M_FMOV(d, s1);
					}
					else {
						d = emit_load(jd, iptr, src, REG_FTMP1);
						if (IS_2_WORD_TYPE(src->type))
							M_DST(d, REG_SP, md->params[s3].regoff * 8);
						else
							M_FST(d, REG_SP, md->params[s3].regoff * 8);
					}
				}
			}

			switch (iptr->opc) {
			case ICMD_BUILTIN:
				disp = dseg_addaddress(cd, bte->fp);

				M_ALD(REG_ITMP3, REG_PV, disp);  /* built-in-function pointer */

				/* TWISTI: i actually don't know the reason for using
				   REG_ITMP3 here instead of REG_PV. */
				s1 = REG_ITMP3;
				break;

			case ICMD_INVOKESPECIAL:
				M_BEQZ(REG_A0, 0);
				codegen_add_nullpointerexception_ref(cd);
				M_NOP;
				/* fall through */

			case ICMD_INVOKESTATIC:
				if (lm == NULL) {
					disp = dseg_addaddress(cd, NULL);

					codegen_addpatchref(cd, PATCHER_invokestatic_special,
										um, disp);

					if (opt_showdisassemble) {
						M_NOP; M_NOP;
					}
				}
				else
					disp = dseg_addaddress(cd, lm->stubroutine);

				M_ALD(REG_PV, REG_PV, disp);          /* method pointer in pv */
				s1 = REG_PV;
				break;

			case ICMD_INVOKEVIRTUAL:
				gen_nullptr_check(REG_A0);

				if (lm == NULL) {
					codegen_addpatchref(cd, PATCHER_invokevirtual, um, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP;
					}

					s1 = 0;
				}
				else
					s1 = OFFSET(vftbl_t, table[0]) +
						sizeof(methodptr) * lm->vftblindex;

				M_ALD(REG_METHODPTR, REG_A0,
					  OFFSET(java_objectheader, vftbl));
				M_ALD(REG_PV, REG_METHODPTR, s1);
				s1 = REG_PV;
				break;

			case ICMD_INVOKEINTERFACE:
				gen_nullptr_check(REG_A0);

				if (lm == NULL) {
					codegen_addpatchref(cd, PATCHER_invokeinterface, um, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP;
					}

					s1 = 0;
					s2 = 0;
				}
				else {
					s1 = OFFSET(vftbl_t, interfacetable[0]) -
						sizeof(methodptr*) * lm->class->index;

					s2 = sizeof(methodptr) * (lm - lm->class->methods);
				}

				M_ALD(REG_METHODPTR, REG_A0,
					  OFFSET(java_objectheader, vftbl));
				M_ALD(REG_METHODPTR, REG_METHODPTR, s1);
				M_ALD(REG_PV, REG_METHODPTR, s2);
				s1 = REG_PV;
				break;
			}

			/* generate the actual call */

			M_JSR(REG_RA, s1);
			M_NOP;
			disp = (s4) (cd->mcodeptr - cd->mcodebase);
			M_LDA(REG_PV, REG_RA, -disp);

			/* actually only used for ICMD_BUILTIN */

			if (INSTRUCTION_MUST_CHECK(iptr)) {
				M_BEQZ(REG_RESULT, 0);
				codegen_add_fillinstacktrace_ref(cd);
				M_NOP;
			}

			/* store return value */

			d = md->returntype.type;

			if (d != TYPE_VOID) {
				if (IS_INT_LNG_TYPE(d)) {
					s1 = codegen_reg_of_dst(jd, iptr, REG_RESULT);
					M_INTMOVE(REG_RESULT, s1);
				}
				else {
					s1 = codegen_reg_of_dst(jd, iptr, REG_FRESULT);
					if (IS_2_WORD_TYPE(d))
						M_DMOV(REG_FRESULT, s1);
					else
						M_FMOV(REG_FRESULT, s1);
				}
				emit_store_dst(jd, iptr, s1);
			}
			break;


		case ICMD_CHECKCAST:  /* ..., objectref ==> ..., objectref            */
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

			if (!(iptr->flags.bits & INS_FLAG_ARRAY)) {
				classinfo *super;
				vftbl_t   *supervftbl;
				s4         superindex;

				if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
					super = NULL;
					superindex = 0;
					supervftbl = NULL;
				}
				else {
					super = iptr->sx.s23.s3.c.cls;
					superindex = super->index;
					supervftbl = super->vftbl;
				}
			
#if defined(ENABLE_THREADS)
				codegen_threadcritrestart(cd, cd->mcodeptr - cd->mcodebase);
#endif

				s1 = emit_load_s1(jd, iptr, REG_ITMP1);

				/* calculate interface checkcast code size */

				s2 = 8;
				if (super == NULL)
					s2 += (opt_showdisassemble ? 2 : 0);

				/* calculate class checkcast code size */

				s3 = 10 /* 10 + (s1 == REG_ITMP1) */;
				if (super == NULL)
					s3 += (opt_showdisassemble ? 2 : 0);

				/* if class is not resolved, check which code to call */

				if (super == NULL) {
					M_BEQZ(s1, 5 + (opt_showdisassemble ? 2 : 0) + s2 + 2 + s3);
					M_NOP;

					disp = dseg_adds4(cd, 0);                 /* super->flags */

					codegen_addpatchref(cd, PATCHER_checkcast_instanceof_flags,
										iptr->sx.s23.s3.c.ref,
										disp);

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

				if ((super == NULL) || (super->flags & ACC_INTERFACE)) {
					if (super != NULL) {
						M_BEQZ(s1, 1 + s2);
						M_NOP;
					}
					else {
						codegen_addpatchref(cd,
											PATCHER_checkcast_instanceof_interface,
											iptr->sx.s23.s3.c.ref,
											0);

						if (opt_showdisassemble) {
							M_NOP; M_NOP;
						}
					}

					M_ALD(REG_ITMP2, s1, OFFSET(java_objectheader, vftbl));
					M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl_t, interfacetablelength));
					M_IADD_IMM(REG_ITMP3, -superindex, REG_ITMP3);
					M_BLEZ(REG_ITMP3, 0);
					codegen_add_classcastexception_ref(cd, s1);
					M_NOP;
					M_ALD(REG_ITMP3, REG_ITMP2,
						  OFFSET(vftbl_t, interfacetable[0]) -
						  superindex * sizeof(methodptr*));
					M_BEQZ(REG_ITMP3, 0);
					codegen_add_classcastexception_ref(cd, s1);
					M_NOP;

					if (super == NULL) {
						M_BR(1 + s3);
						M_NOP;
					}
				}

				/* class checkcast code */

				if ((super == NULL) || !(super->flags & ACC_INTERFACE)) {
					disp = dseg_addaddress(cd, (void *) supervftbl);

					if (super != NULL) {
						M_BEQZ(s1, 1 + s3);
						M_NOP;
					}
					else {
						codegen_addpatchref(cd,
											PATCHER_checkcast_instanceof_class,
											iptr->sx.s23.s3.c.ref,
											disp);

						if (opt_showdisassemble) {
							M_NOP; M_NOP;
						}
					}

					M_ALD(REG_ITMP2, s1, OFFSET(java_objectheader, vftbl));
					M_ALD(REG_ITMP3, REG_PV, disp);
#if defined(ENABLE_THREADS)
					codegen_threadcritstart(cd, cd->mcodeptr - cd->mcodebase);
#endif
					M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, baseval));
					/* 				if (s1 != REG_ITMP1) { */
					/* 					M_ILD(REG_ITMP1, REG_ITMP3, OFFSET(vftbl_t, baseval)); */
					/* 					M_ILD(REG_ITMP3, REG_ITMP3, OFFSET(vftbl_t, diffval)); */
					/* #if defined(ENABLE_THREADS) */
					/* 					codegen_threadcritstop(cd, cd->mcodeptr - cd->mcodebase); */
					/* #endif */
					/* 					M_ISUB(REG_ITMP2, REG_ITMP1, REG_ITMP2); */
					/* 				} else { */
					M_ILD(REG_ITMP3, REG_ITMP3, OFFSET(vftbl_t, baseval));
					M_ISUB(REG_ITMP2, REG_ITMP3, REG_ITMP2); 
					M_ALD(REG_ITMP3, REG_PV, disp);
					M_ILD(REG_ITMP3, REG_ITMP3, OFFSET(vftbl_t, diffval));
#if defined(ENABLE_THREADS)
					codegen_threadcritstop(cd, cd->mcodeptr - cd->mcodebase);
#endif
					/* 				} */
					M_CMPULT(REG_ITMP3, REG_ITMP2, REG_ITMP3);
					M_BNEZ(REG_ITMP3, 0);
					codegen_add_classcastexception_ref(cd, s1);
					M_NOP;
				}

				d = codegen_reg_of_dst(jd, iptr, s1);
			}
			else {
				s1 = emit_load_s1(jd, iptr, REG_A0);
				M_INTMOVE(s1, REG_A0);

				disp = dseg_addaddress(cd, iptr->sx.s23.s3.c.cls);

				if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
					codegen_addpatchref(cd, PATCHER_builtin_arraycheckcast,
										iptr->sx.s23.s3.c.ref,
										disp);

					if (opt_showdisassemble) {
						M_NOP; M_NOP;
					}
				}

				M_ALD(REG_A1, REG_PV, disp);
				disp = dseg_addaddress(cd, BUILTIN_arraycheckcast);
				M_ALD(REG_ITMP3, REG_PV, disp);
				M_JSR(REG_RA, REG_ITMP3);
				M_NOP;

				s1 = emit_load_s1(jd, iptr, REG_ITMP1);
				M_BEQZ(REG_RESULT, 0);
				codegen_add_classcastexception_ref(cd, s1);
				M_NOP;

				d = codegen_reg_of_dst(jd, iptr, s1);
			}

			M_INTMOVE(s1, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_INSTANCEOF: /* ..., objectref ==> ..., intresult            */
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

			super = iptr->sx.s23.s3.c.cls;

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				super = NULL;
				superindex = 0;
				supervftbl = NULL;
			}
			else {
				super = iptr->sx.s23.s3.c.cls;
				superindex = super->index;
				supervftbl = super->vftbl;
			}
			
#if defined(ENABLE_THREADS)
			codegen_threadcritrestart(cd, cd->mcodeptr - cd->mcodebase);
#endif

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if (s1 == d) {
				M_MOV(s1, REG_ITMP1);
				s1 = REG_ITMP1;
			}

			/* calculate interface instanceof code size */

			s2 = 7;
			if (super == NULL)
				s2 += (opt_showdisassemble ? 2 : 0);

			/* calculate class instanceof code size */

			s3 = 8;
			if (super == NULL)
				s3 += (opt_showdisassemble ? 2 : 0);

			M_CLR(d);

			/* if class is not resolved, check which code to call */

			if (super == NULL) {
				M_BEQZ(s1, 5 + (opt_showdisassemble ? 2 : 0) + s2 + 2 + s3);
				M_NOP;

				disp = dseg_adds4(cd, 0);                     /* super->flags */

				codegen_addpatchref(cd, PATCHER_checkcast_instanceof_flags,
									iptr->sx.s23.s3.c.ref, disp);

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

			if ((super == NULL) || (super->flags & ACC_INTERFACE)) {
				if (super != NULL) {
					M_BEQZ(s1, 1 + s2);
					M_NOP;
				}
				else {
					codegen_addpatchref(cd,
										PATCHER_checkcast_instanceof_interface,
										iptr->sx.s23.s3.c.ref, 0);

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

				if (super == NULL) {
					M_BR(1 + s3);
					M_NOP;
				}
			}

			/* class instanceof code */

			if ((super == NULL) || !(super->flags & ACC_INTERFACE)) {
				disp = dseg_addaddress(cd, supervftbl);

				if (super != NULL) {
					M_BEQZ(s1, 1 + s3);
					M_NOP;
				}
				else {
					codegen_addpatchref(cd, PATCHER_checkcast_instanceof_class,
										iptr->sx.s23.s3.c.ref,
										disp);

					if (opt_showdisassemble) {
						M_NOP; M_NOP;
					}
				}

				M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
				M_ALD(REG_ITMP2, REG_PV, disp);
#if defined(ENABLE_THREADS)
				codegen_threadcritstart(cd, cd->mcodeptr - cd->mcodebase);
#endif
				M_ILD(REG_ITMP1, REG_ITMP1, OFFSET(vftbl_t, baseval));
				M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl_t, baseval));
				M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, diffval));
#if defined(ENABLE_THREADS)
				codegen_threadcritstop(cd, cd->mcodeptr - cd->mcodebase);
#endif
				M_ISUB(REG_ITMP1, REG_ITMP3, REG_ITMP1); 
				M_CMPULT(REG_ITMP2, REG_ITMP1, d);
				M_XOR_IMM(d, 1, d);
			}
			emit_store_dst(jd, iptr, d);
			}
			break;

		case ICMD_MULTIANEWARRAY:/* ..., cnt1, [cnt2, ...] ==> ..., arrayref  */

			/* check for negative sizes and copy sizes to stack if necessary  */

			MCODECHECK((iptr->s1.argcount << 1) + 64);

			for (s1 = iptr->s1.argcount; --s1 >= 0; ) {

				src = iptr->sx.s23.s2.args[s1];

				/* copy SAVEDVAR sizes to stack */

				if (src->varkind != ARGVAR) {
					s2 = emit_load(jd, iptr, src, REG_ITMP1);
					M_LST(s2, REG_SP, s1 * 8);
				}
			}

			/* a0 = dimension count */

			ICONST(REG_A0, iptr->s1.argcount);

			/* is patcher function set? */

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				disp = dseg_addaddress(cd, NULL);

				codegen_addpatchref(cd, PATCHER_builtin_multianewarray,
									iptr->sx.s23.s3.c.ref, disp);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

			} else {
				disp = dseg_addaddress(cd, iptr->sx.s23.s3.c.cls);
			}

			/* a1 = arraydescriptor */

			M_ALD(REG_A1, REG_PV, disp);

			/* a2 = pointer to dimensions = stack pointer */

			M_INTMOVE(REG_SP, REG_A2);

			disp = dseg_addaddress(cd, BUILTIN_multianewarray);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_JSR(REG_RA, REG_ITMP3);
			M_NOP;

			/* check for exception before result assignment */

			M_BEQZ(REG_RESULT, 0);
			codegen_add_fillinstacktrace_ref(cd);
			M_NOP;

			d = codegen_reg_of_dst(jd, iptr, REG_RESULT);
			M_INTMOVE(REG_RESULT, d);
			emit_store_dst(jd, iptr, d);
			break;

		default:
			*exceptionptr = new_internalerror("Unknown ICMD %d", iptr->opc);
			return false;
	} /* switch */
		
	} /* for instruction */
		
	/* copy values to interface registers */

	len = bptr->outdepth;
	MCODECHECK(64+len);
#if defined(ENABLE_LSRA)
	if (!opt_lsra) 
#endif
	while (len) {
		len--;
		src = bptr->outvars[len];
		if ((src->varkind != STACKVAR)) {
			s2 = src->type;
			if (IS_FLT_DBL_TYPE(s2)) {
				s1 = emit_load(jd, iptr, src, REG_FTMP1);
				if (rd->interfaces[len][s2].flags & INMEMORY) {
					if (IS_2_WORD_TYPE(s2))
						M_DST(s1, REG_SP, rd->interfaces[len][s2].regoff * 8);
					else
						M_FST(s1, REG_SP, rd->interfaces[len][s2].regoff * 8);

				} else {
					if (IS_2_WORD_TYPE(s2))
						M_DMOV(s1, rd->interfaces[len][s2].regoff);
					else
						M_FMOV(s1, rd->interfaces[len][s2].regoff);
				}

			} else {
				s1 = emit_load(jd, iptr, src, REG_ITMP1);
				if (rd->interfaces[len][s2].flags & INMEMORY)
					M_LST(s1, REG_SP, rd->interfaces[len][s2].regoff * 8);
				else
					M_INTMOVE(s1, rd->interfaces[len][s2].regoff);
			}
		}
	}

	/* At the end of a basic block we may have to append some nops,
	   because the patcher stub calling code might be longer than the
	   actual instruction. So codepatching does not change the
	   following block unintentionally. */

	if (cd->mcodeptr < cd->lastmcodeptr) {
		while (cd->mcodeptr < cd->lastmcodeptr)
			M_NOP;
	}

	} /* if (bptr -> flags >= BBREACHED) */
	} /* for basic block */

	dseg_createlinenumbertable(cd);

	/* generate exception and patcher stubs */

	emit_exception_stubs(jd);
	emit_patcher_stubs(jd);

#if 0
	emit_replacement_stubs(jd);
#endif

	codegen_finish(jd);

	/* everything's ok */

	return true;
}


/* createcompilerstub **********************************************************

   Creates a stub routine which calls the compiler.
	
*******************************************************************************/

#define COMPILERSTUB_DATASIZE    3 * SIZEOF_VOID_P
#define COMPILERSTUB_CODESIZE    4 * 4

#define COMPILERSTUB_SIZE        COMPILERSTUB_DATASIZE + COMPILERSTUB_CODESIZE


u1 *createcompilerstub(methodinfo *m)
{
	u1          *s;                     /* memory to hold the stub            */
	ptrint      *d;
	codeinfo    *code;
	codegendata *cd;
	s4           dumpsize;

	s = CNEW(u1, COMPILERSTUB_SIZE);

	/* set data pointer and code pointer */

	d = (ptrint *) s;
	s = s + COMPILERSTUB_DATASIZE;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	cd = DNEW(codegendata);
	cd->mcodeptr = s;

	/* Store the codeinfo pointer in the same place as in the
	   methodheader for compiled methods. */

	code = code_codeinfo_new(m);

	d[0] = (ptrint) asm_call_jit_compiler;
	d[1] = (ptrint) m;
	d[2] = (ptrint) code;

	M_ALD_INTERN(REG_ITMP1, REG_PV, -2 * SIZEOF_VOID_P);  /* codeinfo pointer */
	M_ALD_INTERN(REG_PV, REG_PV, -3 * SIZEOF_VOID_P);  /* pointer to compiler */
	M_JMP(REG_PV);
	M_NOP;

	md_cacheflush(s, (s4) (cd->mcodeptr - (u1 *) d));

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		count_cstub_len += COMPILERSTUB_SIZE;
#endif

	/* release dump area */

	dump_release(dumpsize);

	return s;
}


/* createnativestub ************************************************************

   Creates a stub routine which calls a native method.

*******************************************************************************/

u1 *createnativestub(functionptr f, jitdata *jd, methoddesc *nmd)
{
	methodinfo   *m;
	codeinfo     *code;
	codegendata  *cd;
	registerdata *rd;
	methoddesc   *md;
	s4            nativeparams;
	s4            i, j;                 /* count variables                    */
	s4            t;
	s4            s1, s2, disp;
	s4            funcdisp;             /* displacement of the function       */

	/* get required compiler data */

	m    = jd->m;
	code = jd->code;
	cd   = jd->cd;
	rd   = jd->rd;

	/* initialize variables */

	md = m->parseddesc;
	nativeparams = (m->flags & ACC_STATIC) ? 2 : 1;

	/* calculate stack frame size */

	cd->stackframesize =
		1 +                             /* return address                     */
		sizeof(stackframeinfo) / SIZEOF_VOID_P +
		sizeof(localref_table) / SIZEOF_VOID_P +
		md->paramcount +                /* for saving arguments over calls    */
		1 +                             /* for saving return address          */
		nmd->memuse;

	/* create method header */

#if SIZEOF_VOID_P == 4
	(void) dseg_addaddress(cd, code);                      /* CodeinfoPointer */
#endif
	(void) dseg_addaddress(cd, code);                      /* MethodPointer   */
	(void) dseg_adds4(cd, cd->stackframesize * 8);         /* FrameSize       */
	(void) dseg_adds4(cd, 0);                              /* IsSync          */
	(void) dseg_adds4(cd, 0);                              /* IsLeaf          */
	(void) dseg_adds4(cd, 0);                              /* IntSave         */
	(void) dseg_adds4(cd, 0);                              /* FltSave         */
	(void) dseg_addlinenumbertablesize(cd);
	(void) dseg_adds4(cd, 0);                              /* ExTableSize     */

	/* generate stub code */

	M_LDA(REG_SP, REG_SP, -cd->stackframesize * 8); /* build up stackframe    */
	M_AST(REG_RA, REG_SP, cd->stackframesize * 8 - SIZEOF_VOID_P); /* store RA*/

#if !defined(NDEBUG)
	if (JITDATA_HAS_FLAG_VERBOSECALL(jd))
		emit_verbosecall_enter(jd);
#endif

	/* get function address (this must happen before the stackframeinfo) */

	funcdisp = dseg_addaddress(cd, f);

#if !defined(WITH_STATIC_CLASSPATH)
	if (f == NULL) {
		codegen_addpatchref(cd, PATCHER_resolve_native, m, funcdisp);

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

	M_AADD_IMM(REG_SP, cd->stackframesize * 8 - SIZEOF_VOID_P, REG_A0);
	M_MOV(REG_PV, REG_A1);
	M_AADD_IMM(REG_SP, cd->stackframesize * 8, REG_A2);
	M_ALD(REG_A3, REG_SP, cd->stackframesize * 8 - SIZEOF_VOID_P);
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
				s1 = md->params[i].regoff + cd->stackframesize;
				s2 = nmd->params[j].regoff;
				M_ALD(REG_ITMP1, REG_SP, s1 * 8);
				M_AST(REG_ITMP1, REG_SP, s2 * 8);
			}

		} else {
			if (!md->params[i].inmemory) {
				s1 = rd->argfltregs[md->params[i].regoff];

				if (!nmd->params[j].inmemory) {
					s2 = rd->argfltregs[nmd->params[j].regoff];
					if (IS_2_WORD_TYPE(t))
						M_DMOV(s1, s2);
					else
						M_FMOV(s1, s2);

				} else {
					s2 = nmd->params[j].regoff;
					if (IS_2_WORD_TYPE(t))
						M_DST(s1, REG_SP, s2 * 8);
					else
						M_FST(s1, REG_SP, s2 * 8);
				}

			} else {
				s1 = md->params[i].regoff + cd->stackframesize;
				s2 = nmd->params[j].regoff;
				if (IS_2_WORD_TYPE(t)) {
					M_DLD(REG_FTMP1, REG_SP, s1 * 8);
					M_DST(REG_FTMP1, REG_SP, s2 * 8);
				} else {
					M_FLD(REG_FTMP1, REG_SP, s1 * 8);
					M_FST(REG_FTMP1, REG_SP, s2 * 8);
				}
			}
		}
	}

	/* put class into second argument register */

	if (m->flags & ACC_STATIC) {
		disp = dseg_addaddress(cd, m->class);
		M_ALD(REG_A1, REG_PV, disp);
	}

	/* put env into first argument register */

	disp = dseg_addaddress(cd, _Jv_env);
	M_ALD(REG_A0, REG_PV, disp);

	/* do the native function call */

	M_ALD(REG_ITMP3, REG_PV, funcdisp); /* load adress of native method       */
	M_JSR(REG_RA, REG_ITMP3);           /* call native method                 */
	M_NOP;                              /* delay slot                         */

	/* save return value */

	if (md->returntype.type != TYPE_VOID) {
		if (IS_INT_LNG_TYPE(md->returntype.type))
			M_LST(REG_RESULT, REG_SP, 0 * 8);
		else
			M_DST(REG_FRESULT, REG_SP, 0 * 8);
	}

#if !defined(NDEBUG)
	if (JITDATA_HAS_FLAG_VERBOSECALL(jd))
		emit_verbosecall_exit(jd);
#endif

	/* remove native stackframe info */

	M_AADD_IMM(REG_SP, cd->stackframesize * 8 - SIZEOF_VOID_P, REG_A0);
	disp = dseg_addaddress(cd, codegen_finish_native_call);
	M_ALD(REG_ITMP3, REG_PV, disp);
	M_JSR(REG_RA, REG_ITMP3);
	M_NOP; /* XXX fill me! */
	M_MOV(REG_RESULT, REG_ITMP1_XPTR);

	/* restore return value */

	if (md->returntype.type != TYPE_VOID) {
		if (IS_INT_LNG_TYPE(md->returntype.type))
			M_LLD(REG_RESULT, REG_SP, 0 * 8);
		else
			M_DLD(REG_FRESULT, REG_SP, 0 * 8);
	}

	M_ALD(REG_RA, REG_SP, cd->stackframesize * 8 - SIZEOF_VOID_P); /* load RA */

	/* check for exception */

	M_BNEZ(REG_ITMP1_XPTR, 2);          /* if no exception then return        */
	M_LDA(REG_SP, REG_SP, cd->stackframesize * 8); /* DELAY SLOT              */

	M_RET(REG_RA);                      /* return to caller                   */
	M_NOP;                              /* DELAY SLOT                         */

	/* handle exception */
	
	disp = dseg_addaddress(cd, asm_handle_nat_exception);
	M_ALD(REG_ITMP3, REG_PV, disp);     /* load asm exception handler address */
	M_JMP(REG_ITMP3);                   /* jump to asm exception handler      */
	M_ASUB_IMM(REG_RA, 4, REG_ITMP2_XPC); /* get exception address (DELAY)    */

	/* generate patcher stubs */

	emit_patcher_stubs(jd);

	codegen_finish(jd);

	return code->entrypoint;
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
 * vim:noexpandtab:sw=4:ts=4:
 */
