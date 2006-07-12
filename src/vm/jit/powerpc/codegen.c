/* src/vm/jit/powerpc/codegen.c - machine code generator for 32-bit PowerPC

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
            Stefan Ring

   Changes: Christian Thalinger
            Christian Ullrich
            Edwin Steiner

   $Id: codegen.c 5117 2006-07-12 20:14:00Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <signal.h>

#include "vm/types.h"

#include "md-abi.h"

#include "vm/jit/powerpc/arch.h"
#include "vm/jit/powerpc/codegen.h"

#include "mm/memory.h"
#include "native/native.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/codegen-common.h"
#include "vm/jit/dseg.h"
#include "vm/jit/emit.h"
#include "vm/jit/jit.h"
#include "vm/jit/methodheader.h"
#include "vm/jit/parse.h"
#include "vm/jit/patcher.h"
#include "vm/jit/reg.h"
#include "vm/jit/replace.h"

#if defined(ENABLE_LSRA)
# include "vm/jit/allocator/lsra.h"
#endif


void codegen_trace_args(jitdata *jd, s4 stackframesize, bool nativestub);

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
	s4                  stackframesize;
	stackptr            src;
	varinfo            *var;
	basicblock         *bptr;
	instruction        *iptr;
	exceptiontable     *ex;
	u2                  currentline;
	methodinfo         *lm;             /* local methodinfo for ICMD_INVOKE*  */
	builtintable_entry *bte;
	methoddesc         *md;
	rplpoint           *replacementpoint;

	/* get required compiler data */

	m    = jd->m;
	code = jd->code;
	cd   = jd->cd;
	rd   = jd->rd;

	/* prevent compiler warnings */

	d = 0;
	lm = NULL;
	bte = NULL;

	{
	s4 i, p, t, l;
	s4 savedregs_num;

	savedregs_num = 0;

	/* space to save used callee saved registers */

	savedregs_num += (INT_SAV_CNT - rd->savintreguse);
	savedregs_num += (FLT_SAV_CNT - rd->savfltreguse) * 2;

	stackframesize = rd->memuse + savedregs_num;

#if defined(ENABLE_THREADS)
	/* space to save argument of monitor_enter and Return Values to survive */
    /* monitor_exit. The stack position for the argument can not be shared  */
	/* with place to save the return register on PPC, since both values     */
	/* reside in R3 */
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		/* reserve 2 slots for long/double return values for monitorexit */

		if (IS_2_WORD_TYPE(m->parseddesc->returntype.type))
			stackframesize += 3;
		else
			stackframesize += 2;
	}

#endif

	/* create method header */

	/* align stack to 16-bytes */

/* 	if (!jd->isleafmethod || opt_verbosecall) */
		stackframesize = (stackframesize + 3) & ~3;

/* 	else if (jd->isleafmethod && (stackframesize == LA_WORD_SIZE)) */
/* 		stackframesize = 0; */

	(void) dseg_addaddress(cd, code);                      /* CodeinfoPointer */
	(void) dseg_adds4(cd, stackframesize * 4);             /* FrameSize       */

#if defined(ENABLE_THREADS)
	/* IsSync contains the offset relative to the stack pointer for the
	   argument of monitor_exit used in the exception handler. Since the
	   offset could be zero and give a wrong meaning of the flag it is
	   offset by one.
	*/

	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		(void) dseg_adds4(cd, (rd->memuse + 1) * 4);       /* IsSync          */
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
		(void) dseg_addaddress(cd, ex->catchtype.cls);
	}
	
	/* generate method profiling code */

	if (JITDATA_HAS_FLAG_INSTRUMENT(jd)) {
		/* count frequency */

		M_ALD(REG_ITMP1, REG_PV, CodeinfoPointer);
		M_ALD(REG_ITMP2, REG_ITMP1, OFFSET(codeinfo, frequency));
		M_IADD_IMM(REG_ITMP2, 1, REG_ITMP2);
		M_AST(REG_ITMP2, REG_ITMP1, OFFSET(codeinfo, frequency));

/* 		PROFILE_CYCLE_START; */
	}

	/* create stack frame (if necessary) */

	if (!jd->isleafmethod) {
		M_MFLR(REG_ZERO);
		M_AST(REG_ZERO, REG_SP, LA_LR_OFFSET);
	}

	if (stackframesize)
		M_STWU(REG_SP, REG_SP, -stackframesize * 4);

	/* save return address and used callee saved registers */

	p = stackframesize;
	for (i = INT_SAV_CNT - 1; i >= rd->savintreguse; i--) {
		p--; M_IST(rd->savintregs[i], REG_SP, p * 4);
	}
	for (i = FLT_SAV_CNT - 1; i >= rd->savfltreguse; i--) {
		p -= 2; M_DST(rd->savfltregs[i], REG_SP, p * 4);
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
			if (IS_2_WORD_TYPE(t))
				s2 = PACK_REGS(rd->argintregs[GET_LOW_REG(s1)],
							   rd->argintregs[GET_HIGH_REG(s1)]);
			else
				s2 = rd->argintregs[s1];
 			if (!md->params[p].inmemory) {           /* register arguments    */
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
					if (IS_2_WORD_TYPE(t))
						M_LNGMOVE(s2, var->regoff);
					else
						M_INTMOVE(s2, var->regoff);

				} else {                             /* reg arg -> spilled    */
					if (IS_2_WORD_TYPE(t))
						M_LST(s2, REG_SP, var->regoff * 4);
					else
						M_IST(s2, REG_SP, var->regoff * 4);
				}

			} else {                                 /* stack arguments       */
 				if (!(var->flags & INMEMORY)) {      /* stack arg -> register */
					if (IS_2_WORD_TYPE(t))
						M_LLD(var->regoff, REG_SP, (stackframesize + s1) * 4);
					else
						M_ILD(var->regoff, REG_SP, (stackframesize + s1) * 4);

				} else {                             /* stack arg -> spilled  */
#if 1
 					M_ILD(REG_ITMP1, REG_SP, (stackframesize + s1) * 4);
 					M_IST(REG_ITMP1, REG_SP, var->regoff * 4);
					if (IS_2_WORD_TYPE(t)) {
						M_ILD(REG_ITMP1, REG_SP, (stackframesize + s1) * 4 +4);
						M_IST(REG_ITMP1, REG_SP, var->regoff * 4 + 4);
					}
#else
					/* Reuse Memory Position on Caller Stack */
					var->regoff = stackframesize + s1;
#endif
				}
			}

		} else {                                     /* floating args         */
 			if (!md->params[p].inmemory) {           /* register arguments    */
				s2 = rd->argfltregs[s1];
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
 					M_FLTMOVE(s2, var->regoff);

 				} else {			                 /* reg arg -> spilled    */
					if (IS_2_WORD_TYPE(t))
						M_DST(s2, REG_SP, var->regoff * 4);
					else
						M_FST(s2, REG_SP, var->regoff * 4);
 				}

 			} else {                                 /* stack arguments       */
 				if (!(var->flags & INMEMORY)) {      /* stack-arg -> register */
					if (IS_2_WORD_TYPE(t))
						M_DLD(var->regoff, REG_SP, (stackframesize + s1) * 4);

					else
						M_FLD(var->regoff, REG_SP, (stackframesize + s1) * 4);

 				} else {                             /* stack-arg -> spilled  */
#if 1
					if (IS_2_WORD_TYPE(t)) {
						M_DLD(REG_FTMP1, REG_SP, (stackframesize + s1) * 4);
						M_DST(REG_FTMP1, REG_SP, var->regoff * 4);
						var->regoff = stackframesize + s1;

					} else {
						M_FLD(REG_FTMP1, REG_SP, (stackframesize + s1) * 4);
						M_FST(REG_FTMP1, REG_SP, var->regoff * 4);
					}
#else
					/* Reuse Memory Position on Caller Stack */
					var->regoff = stackframesize + s1;
#endif
				}
			}
		}
	} /* end for */

	/* save monitorenter argument */

#if defined(ENABLE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		p = dseg_addaddress(cd, BUILTIN_monitorenter);
		M_ALD(REG_ITMP3, REG_PV, p);
		M_MTCTR(REG_ITMP3);

		/* get or test the lock object */

		if (m->flags & ACC_STATIC) {
			p = dseg_addaddress(cd, &m->class->object.header);
			M_ALD(rd->argintregs[0], REG_PV, p);
		}
		else {
			M_TST(rd->argintregs[0]);
			M_BEQ(0);
			codegen_add_nullpointerexception_ref(cd);
		}

		M_AST(rd->argintregs[0], REG_SP, rd->memuse * 4);
		M_JSR;
	}
#endif

	/* call trace function */

	if (JITDATA_HAS_FLAG_VERBOSECALL(jd))
		codegen_trace_args(jd, stackframesize, false);
	}

	/* end of header generation */

	replacementpoint = code->rplpoints;

	/* walk through all basic blocks */

	for (bptr = m->basicblocks; bptr != NULL; bptr = bptr->next) {

		bptr->mpc = (s4) (cd->mcodeptr - cd->mcodebase);

		if (bptr->flags >= BBREACHED) {

		/* branch resolving */

		{
		branchref *brefs;
		for (brefs = bptr->branchrefs; brefs != NULL; brefs = brefs->next) {
			gen_resolvebranch((u1*) cd->mcodebase + brefs->branchpos, 
			                  brefs->branchpos,
							  bptr->mpc);
			}
		}

		/* handle replacement points */

		if (bptr->bitflags & BBFLAG_REPLACEMENT) {
			replacementpoint->pc = (u1*)(ptrint)bptr->mpc; /* will be resolved later */
			
			replacementpoint++;
		}

#if 0
		/* generate basicblock profiling code */

		if (JITDATA_HAS_FLAG_INSTRUMENT(jd)) {
			/* count frequency */

			disp = dseg_addaddress(cd, code->bbfrequency);
			M_ALD(REG_ITMP2, REG_PV, disp);
			M_ALD(REG_ITMP3, REG_ITMP2, bptr->debug_nr * 4);
			M_IADD_IMM(REG_ITMP3, 1, REG_ITMP3);
			M_AST(REG_ITMP3, REG_ITMP2, bptr->debug_nr * 4);

			/* if this is an exception handler, start profiling again */

/* 			if (bptr->type == BBTYPE_EXH) */
/* 				PROFILE_CYCLE_START; */
		}
#endif

		/* copy interface registers to their destination */

		src = bptr->instack;
		len = bptr->indepth;
		MCODECHECK(64+len);

#if defined(ENABLE_LSRA)
		if (opt_lsra) {
			while (src != NULL) {
				len--;
				if ((len == 0) && (bptr->type != BBTYPE_STD)) {
					/* d = reg_of_var(m, src, REG_ITMP1); */
					if (!(src->flags & INMEMORY))
						d = src->regoff;
					else
						d = REG_ITMP1;
					M_INTMOVE(REG_ITMP1, d);
					emit_store(jd, NULL, src, d);
				}
				src = src->prev;
			}
		} else {
#endif
		while (src != NULL) {
			len--;
			if ((len == 0) && (bptr->type != BBTYPE_STD)) {
				d = codegen_reg_of_var(rd, 0, src, REG_ITMP1);
				M_INTMOVE(REG_ITMP1, d);
				emit_store(jd, NULL, src, d);
			}
			else {
				if (src->type == TYPE_LNG)
					d = codegen_reg_of_var(rd, 0, src, REG_ITMP12_PACKED);
				else
					d = codegen_reg_of_var(rd, 0, src, REG_IFTMP);
				if ((src->varkind != STACKVAR)) {
					s2 = src->type;
					if (IS_FLT_DBL_TYPE(s2)) {
						if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
							s1 = rd->interfaces[len][s2].regoff;
							M_FLTMOVE(s1, d);
						}
						else {
							if (IS_2_WORD_TYPE(s2)) {
								M_DLD(d, REG_SP,
									  rd->interfaces[len][s2].regoff * 4);
							}
							else {
								M_FLD(d, REG_SP,
									  rd->interfaces[len][s2].regoff * 4);
							}	
						}
						emit_store(jd, NULL, src, d);
					}
					else {
						if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
							s1 = rd->interfaces[len][s2].regoff;
							if (IS_2_WORD_TYPE(s2))
								M_LNGMOVE(s1, d);
							else
								M_INTMOVE(s1, d);
						} 
						else {
							if (IS_2_WORD_TYPE(s2))
								M_LLD(d, REG_SP,
									  rd->interfaces[len][s2].regoff * 4);
							else
								M_ILD(d, REG_SP,
									  rd->interfaces[len][s2].regoff * 4);
						}
						emit_store(jd, NULL, src, d);
					}
				}
			}
			src = src->prev;
		}

#if defined(ENABLE_LSRA)
		}
#endif
		/* walk through all instructions */
		
		src = bptr->instack;
		len = bptr->icount;
		currentline = 0;

		for (iptr = bptr->iinstr; len > 0; src = iptr->dst, len--, iptr++) {
			if (iptr->line != currentline) {
				dseg_addlinenumber(cd, iptr->line);
				currentline = iptr->line;
			}

			MCODECHECK(64);   /* an instruction usually needs < 64 words      */

			switch (iptr->opc) {
			case ICMD_NOP:    /* ...  ==> ...                                 */
			case ICMD_INLINE_START:
			case ICMD_INLINE_END:
				break;

		case ICMD_CHECKNULL:  /* ..., objectref  ==> ..., objectref           */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			M_TST(s1);
			M_BEQ(0);
			codegen_add_nullpointerexception_ref(cd);
			break;

		/* constant operations ************************************************/

		case ICMD_ICONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.i = constant                    */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			ICONST(d, iptr->val.i);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.l = constant                    */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP12_PACKED);
			LCONST(d, iptr->val.l);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_FCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.f = constant                    */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
			a = dseg_addfloat(cd, iptr->val.f);
			M_FLD(d, REG_PV, a);
			emit_store(jd, iptr, iptr->dst, d);
			break;
			
		case ICMD_DCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.d = constant                    */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
			a = dseg_adddouble(cd, iptr->val.d);
			M_DLD(d, REG_PV, a);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_ACONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.a = constant                    */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			disp = dseg_addaddress(cd, iptr->val.a);

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				codegen_addpatchref(cd, PATCHER_aconst,
									ICMD_ACONST_UNRESOLVED_CLASSREF(iptr),
								    disp);

				if (opt_showdisassemble)
					M_NOP;
			}

			M_ALD(d, REG_PV, disp);
			emit_store(jd, iptr, iptr->dst, d);
			break;


		/* load/store operations **********************************************/

		case ICMD_ILOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_ALOAD:      /* op1 = local variable                         */

			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1))
				break;
			if (var->flags & INMEMORY)
				M_ILD(d, REG_SP, var->regoff * 4);
			else
				M_INTMOVE(var->regoff, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP12_PACKED);
			if ((iptr->dst->varkind == LOCALVAR) &&
				(iptr->dst->varnum == iptr->op1))
				break;
			if (var->flags & INMEMORY)
				M_LLD(d, REG_SP, var->regoff * 4);
			else
				M_LNGMOVE(var->regoff, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_FLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
				(iptr->dst->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY)
				M_FLD(d, REG_SP, var->regoff * 4);
			else
				M_FLTMOVE(var->regoff, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_DLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
				(iptr->dst->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY)
				M_DLD(d, REG_SP, var->regoff * 4);
			else
				M_FLTMOVE(var->regoff, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;


		case ICMD_ISTORE:     /* ..., value  ==> ...                          */
		case ICMD_ASTORE:     /* op1 = local variable                         */

			if ((src->varkind == LOCALVAR) && (src->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
				M_IST(s1, REG_SP, var->regoff * 4);
			} else {
				s1 = emit_load_s1(jd, iptr, src, var->regoff);
				M_INTMOVE(s1, var->regoff);
			}
			break;

		case ICMD_LSTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			if ((src->varkind == LOCALVAR) && (src->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				s1 = emit_load_s1(jd, iptr, src, REG_ITMP12_PACKED);
				M_LST(s1, REG_SP, var->regoff * 4);
			} else {
				s1 = emit_load_s1(jd, iptr, src, var->regoff);
				M_LNGMOVE(s1, var->regoff);
			}
			break;

		case ICMD_FSTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			if ((src->varkind == LOCALVAR) && (src->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
				M_FST(s1, REG_SP, var->regoff * 4);
			} else {
				s1 = emit_load_s1(jd, iptr, src, var->regoff);
				M_FLTMOVE(s1, var->regoff);
			}
			break;

		case ICMD_DSTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			if ((src->varkind == LOCALVAR) && (src->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
				M_DST(s1, REG_SP, var->regoff * 4);
			} else {
				s1 = emit_load_s1(jd, iptr, src, var->regoff);
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

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1); 
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_NEG(s1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LNEG:       /* ..., value  ==> ..., - value                 */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP12_PACKED);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP12_PACKED);
			M_SUBFIC(GET_LOW_REG(s1), 0, GET_LOW_REG(d));
			M_SUBFZE(GET_HIGH_REG(s1), GET_HIGH_REG(d));
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_I2L:        /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP12_PACKED);
			M_INTMOVE(s1, GET_LOW_REG(d));
			M_SRA_IMM(GET_LOW_REG(d), 31, GET_HIGH_REG(d));
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1_low(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_INTMOVE(s1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_BSEXT(s1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_INT2CHAR:   /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_CZEXT(s1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_INT2SHORT:  /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_SSEXT(s1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;


		case ICMD_IADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_IADD(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if ((iptr->val.i >= -32768) && (iptr->val.i <= 32767)) {
				M_IADD_IMM(s1, iptr->val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_IADD(s1, REG_ITMP2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			s1 = emit_load_s1_low(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2_low(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			M_ADDC(s1, s2, GET_LOW_REG(d));
			s1 = emit_load_s1_high(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2_high(jd, iptr, src, REG_ITMP3);   /* don't use REG_ITMP2 */
			M_ADDE(s1, s2, GET_HIGH_REG(d));
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.l = constant                             */

			s3 = iptr->val.l & 0xffffffff;
			s1 = emit_load_s1_low(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			if ((s3 >= -32768) && (s3 <= 32767)) {
				M_ADDIC(s1, s3, GET_LOW_REG(d));
			} else {
				ICONST(REG_ITMP2, s3);
				M_ADDC(s1, REG_ITMP2, GET_LOW_REG(d));
			}
			s1 = emit_load_s1_high(jd, iptr, src, REG_ITMP1);
			s3 = iptr->val.l >> 32;
			if (s3 == -1) {
				M_ADDME(s1, GET_HIGH_REG(d));
			} else if (s3 == 0) {
				M_ADDZE(s1, GET_HIGH_REG(d));
			} else {
				ICONST(REG_ITMP3, s3);                 /* don't use REG_ITMP2 */
				M_ADDE(s1, REG_ITMP3, GET_HIGH_REG(d));
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_ISUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_ISUB(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if ((iptr->val.i >= -32767) && (iptr->val.i <= 32768)) {
				M_IADD_IMM(s1, -iptr->val.i, d);
			} else {
				ICONST(REG_ITMP2, -iptr->val.i);
				M_IADD(s1, REG_ITMP2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			s1 = emit_load_s1_low(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2_low(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			M_SUBC(s1, s2, GET_LOW_REG(d));
			s1 = emit_load_s1_high(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2_high(jd, iptr, src, REG_ITMP3);   /* don't use REG_ITMP2 */
			M_SUBE(s1, s2, GET_HIGH_REG(d));
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* val.l = constant                             */

			s3 = (-iptr->val.l) & 0xffffffff;
			s1 = emit_load_s1_low(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			if ((s3 >= -32768) && (s3 <= 32767)) {
				M_ADDIC(s1, s3, GET_LOW_REG(d));
			} else {
				ICONST(REG_ITMP2, s3);
				M_ADDC(s1, REG_ITMP2, GET_LOW_REG(d));
			}
			s1 = emit_load_s1_high(jd, iptr, src, REG_ITMP1);
			s3 = (-iptr->val.l) >> 32;
			if (s3 == -1)
				M_ADDME(s1, GET_HIGH_REG(d));
			else if (s3 == 0)
				M_ADDZE(s1, GET_HIGH_REG(d));
			else {
				ICONST(REG_ITMP3, s3);                 /* don't use REG_ITMP2 */
				M_ADDE(s1, REG_ITMP3, GET_HIGH_REG(d));
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			M_TST(s2);
			M_BEQ(0);
			codegen_add_arithmeticexception_ref(cd);
			M_LDAH(REG_ITMP3, REG_ZERO, 0x8000);
			M_CMP(REG_ITMP3, s1);
			M_BNE(3 + (s1 != d));
			M_CMPI(s2, -1);
			M_BNE(1 + (s1 != d));
			M_INTMOVE(s1, d);
			M_BR(1);
			M_IDIV(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_TST(s2);
			M_BEQ(0);
			codegen_add_arithmeticexception_ref(cd);
			M_LDAH(REG_ITMP3, REG_ZERO, 0x8000);
			M_CMP(REG_ITMP3, s1);
			M_BNE(4);
			M_CMPI(s2, -1);
			M_BNE(2);
			M_CLR(d);
			M_BR(3);
			M_IDIV(s1, s2, REG_ITMP3);
			M_IMUL(REG_ITMP3, s2, REG_ITMP3);
			M_ISUB(s1, REG_ITMP3, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */
		case ICMD_LREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			bte = iptr->val.a;
			md = bte->md;

			s2 = emit_load_s2(jd, iptr, src, PACK_REGS(REG_ITMP2, REG_ITMP1));
			M_OR_TST(GET_HIGH_REG(s2), GET_LOW_REG(s2), REG_ITMP3);
			M_BEQ(0);
			codegen_add_arithmeticexception_ref(cd);

			disp = dseg_addaddress(cd, bte->fp);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_MTCTR(REG_ITMP3);

			s3 = PACK_REGS(rd->argintregs[GET_LOW_REG(md->params[1].regoff)],
						   rd->argintregs[GET_HIGH_REG(md->params[1].regoff)]);
			M_LNGMOVE(s2, s3);

			s1 = emit_load_s1(jd, iptr, src->prev, PACK_REGS(REG_ITMP2, REG_ITMP1));
			s3 = PACK_REGS(rd->argintregs[GET_LOW_REG(md->params[0].regoff)],
						   rd->argintregs[GET_HIGH_REG(md->params[0].regoff)]);
			M_LNGMOVE(s1, s3);

			M_JSR;

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, PACK_REGS(REG_RESULT2, REG_RESULT));
			M_LNGMOVE(PACK_REGS(REG_RESULT2, REG_RESULT), d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_IMUL(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if ((iptr->val.i >= -32768) && (iptr->val.i <= 32767)) {
				M_IMUL_IMM(s1, iptr->val.i, d);
			} else {
				ICONST(REG_ITMP3, iptr->val.i);
				M_IMUL(s1, REG_ITMP3, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IDIVPOW2:   /* ..., value  ==> ..., value << constant       */
		                      
			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			M_SRA_IMM(s1, iptr->val.i, d);
			M_ADDZE(d, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_ISHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_AND_IMM(s2, 0x1f, REG_ITMP3);
			M_SLL(s1, REG_ITMP3, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_SLL_IMM(s1, iptr->val.i & 0x1f, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_AND_IMM(s2, 0x1f, REG_ITMP3);
			M_SRA(s1, REG_ITMP3, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_SRA_IMM(s1, iptr->val.i & 0x1f, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_AND_IMM(s2, 0x1f, REG_ITMP2);
			M_SRL(s1, REG_ITMP2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if (iptr->val.i & 0x1f) {
				M_SRL_IMM(s1, iptr->val.i & 0x1f, d);
			} else {
				M_INTMOVE(s1, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_AND(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 65535)) {
				M_AND_IMM(s1, iptr->val.i, d);
				}
			/*
			else if (iptr->val.i == 0xffffff) {
				M_RLWINM(s1, 0, 8, 31, d);
				}
			*/
			else {
				ICONST(REG_ITMP3, iptr->val.i);
				M_AND(s1, REG_ITMP3, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			s1 = emit_load_s1_low(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2_low(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			M_AND(s1, s2, GET_LOW_REG(d));
			s1 = emit_load_s1_high(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2_high(jd, iptr, src, REG_ITMP3);   /* don't use REG_ITMP2 */
			M_AND(s1, s2, GET_HIGH_REG(d));
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.l = constant                             */

			s3 = iptr->val.l & 0xffffffff;
			s1 = emit_load_s1_low(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			if ((s3 >= 0) && (s3 <= 65535)) {
				M_AND_IMM(s1, s3, GET_LOW_REG(d));
			} else {
				ICONST(REG_ITMP3, s3);
				M_AND(s1, REG_ITMP3, GET_LOW_REG(d));
			}
			s1 = emit_load_s1_high(jd, iptr, src, REG_ITMP1);
			s3 = iptr->val.l >> 32;
			if ((s3 >= 0) && (s3 <= 65535)) {
				M_AND_IMM(s1, s3, GET_HIGH_REG(d));
			} else {
				ICONST(REG_ITMP3, s3);                 /* don't use REG_ITMP2 */
				M_AND(s1, REG_ITMP3, GET_HIGH_REG(d));
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_MOV(s1, REG_ITMP2);
			M_CMPI(s1, 0);
			M_BGE(1 + 2*(iptr->val.i >= 32768));
			if (iptr->val.i >= 32768) {
				M_ADDIS(REG_ZERO, iptr->val.i >> 16, REG_ITMP2);
				M_OR_IMM(REG_ITMP2, iptr->val.i, REG_ITMP2);
				M_IADD(s1, REG_ITMP2, REG_ITMP2);
			} else {
				M_IADD_IMM(s1, iptr->val.i, REG_ITMP2);
			}
			{
				int b=0, m = iptr->val.i;
				while (m >>= 1)
					++b;
				M_RLWINM(REG_ITMP2, 0, 0, 30-b, REG_ITMP2);
			}
			M_ISUB(s1, REG_ITMP2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_OR(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 65535)) {
				M_OR_IMM(s1, iptr->val.i, d);
			} else {
				ICONST(REG_ITMP3, iptr->val.i);
				M_OR(s1, REG_ITMP3, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LOR:       /* ..., val1, val2  ==> ..., val1 | val2        */

			s1 = emit_load_s1_low(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2_low(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			M_OR(s1, s2, GET_LOW_REG(d));
			s1 = emit_load_s1_high(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2_high(jd, iptr, src, REG_ITMP3);   /* don't use REG_ITMP2 */
			M_OR(s1, s2, GET_HIGH_REG(d));
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.l = constant                             */

			s3 = iptr->val.l & 0xffffffff;
			s1 = emit_load_s1_low(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			if ((s3 >= 0) && (s3 <= 65535)) {
				M_OR_IMM(s1, s3, GET_LOW_REG(d));
			} else {
				ICONST(REG_ITMP3, s3);
				M_OR(s1, REG_ITMP3, GET_LOW_REG(d));
			}
			s1 = emit_load_s1_high(jd, iptr, src, REG_ITMP1);
			s3 = iptr->val.l >> 32;
			if ((s3 >= 0) && (s3 <= 65535)) {
				M_OR_IMM(s1, s3, GET_HIGH_REG(d));
			} else {
				ICONST(REG_ITMP3, s3);                 /* don't use REG_ITMP2 */
				M_OR(s1, REG_ITMP3, GET_HIGH_REG(d));
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_XOR(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 65535)) {
				M_XOR_IMM(s1, iptr->val.i, d);
			} else {
				ICONST(REG_ITMP3, iptr->val.i);
				M_XOR(s1, REG_ITMP3, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			s1 = emit_load_s1_low(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2_low(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			M_XOR(s1, s2, GET_LOW_REG(d));
			s1 = emit_load_s1_high(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2_high(jd, iptr, src, REG_ITMP3);   /* don't use REG_ITMP2 */
			M_XOR(s1, s2, GET_HIGH_REG(d));
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.l = constant                             */

			s3 = iptr->val.l & 0xffffffff;
			s1 = emit_load_s1_low(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			if ((s3 >= 0) && (s3 <= 65535)) {
				M_XOR_IMM(s1, s3, GET_LOW_REG(d));
			} else {
				ICONST(REG_ITMP3, s3);
				M_XOR(s1, REG_ITMP3, GET_LOW_REG(d));
			}
			s1 = emit_load_s1_high(jd, iptr, src, REG_ITMP1);
			s3 = iptr->val.l >> 32;
			if ((s3 >= 0) && (s3 <= 65535)) {
				M_XOR_IMM(s1, s3, GET_HIGH_REG(d));
			} else {
				ICONST(REG_ITMP3, s3);                 /* don't use REG_ITMP2 */
				M_XOR(s1, REG_ITMP3, GET_HIGH_REG(d));
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LCMP:       /* ..., val1, val2  ==> ..., val1 cmp val2      */
			/*******************************************************************
                TODO: CHANGE THIS TO A VERSION THAT WORKS !!!
			*******************************************************************/
			s1 = emit_load_s1_high(jd, iptr, src->prev, REG_ITMP3);
			s2 = emit_load_s2_high(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			{
				int tempreg = false;
				int dreg;
				u1  *br1;

				if (src->prev->flags & INMEMORY) {
					tempreg = tempreg || (d == REG_ITMP3) || (d == REG_ITMP2);
				} else {
					tempreg = tempreg || (d == GET_HIGH_REG(src->prev->regoff))
						        || (d == GET_LOW_REG(src->prev->regoff));
				}
				if (src->flags & INMEMORY) {
					tempreg = tempreg || (d == REG_ITMP3) || (d == REG_ITMP2);
				} else {
					tempreg = tempreg || (d == GET_HIGH_REG(src->regoff))
                                 || (d == GET_LOW_REG(src->regoff));
				}

				dreg = tempreg ? REG_ITMP1 : d;
				M_IADD_IMM(REG_ZERO, 1, dreg);
				M_CMP(s1, s2);
				M_BGT(0);
				br1 = cd->mcodeptr;
				M_BLT(0);
				s1 = emit_load_s1_low(jd, iptr, src->prev, REG_ITMP3);
				s2 = emit_load_s2_low(jd, iptr, src, REG_ITMP2);
				M_CMPU(s1, s2);
				M_BGT(3);
				M_BEQ(1);
				M_IADD_IMM(dreg, -1, dreg);
				M_IADD_IMM(dreg, -1, dreg);
				gen_resolvebranch(br1, br1, cd->mcodeptr);
				gen_resolvebranch(br1 + 1 * 4, br1 + 1 * 4, cd->mcodeptr - 2 * 4);
				M_INTMOVE(dreg, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IINC:       /* ..., value  ==> ..., value + constant        */
		                      /* op1 = variable, val.i = constant             */

			var = &(rd->locals[iptr->op1][TYPE_INT]);
			if (var->flags & INMEMORY) {
				s1 = REG_ITMP1;
				M_ILD(s1, REG_SP, var->regoff * 4);
			} else
				s1 = var->regoff;
			{
				u4 m = iptr->val.i;
				if (m & 0x8000)
					m += 65536;
				if (m & 0xffff0000)
					M_ADDIS(s1, m >> 16, s1);
				if (m & 0xffff)
					M_IADD_IMM(s1, m & 0xffff, s1);
			}
			if (var->flags & INMEMORY)
				M_IST(s1, REG_SP, var->regoff * 4);
			break;


		/* floating operations ************************************************/

		case ICMD_FNEG:       /* ..., value  ==> ..., - value                 */

			s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			M_FMOVN(s1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */

			s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			M_FMOVN(s1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			M_FADD(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_DADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			M_DADD(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_FSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			M_FSUB(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_DSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			M_DSUB(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_FMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			M_FMUL(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_DMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			M_DMUL(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_FDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			M_FDIV(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_DDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			M_DDIV(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;
		
		case ICMD_F2I:       /* ..., value  ==> ..., (int) value              */
		case ICMD_D2I:

			s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_CLR(d);
			disp = dseg_addfloat(cd, 0.0);
			M_FLD(REG_FTMP2, REG_PV, disp);
			M_FCMPU(s1, REG_FTMP2);
			M_BNAN(4);
			disp = dseg_adds4(cd, 0);
			M_CVTDL_C(s1, REG_FTMP1);
			M_LDA(REG_ITMP1, REG_PV, disp);
			M_STFIWX(REG_FTMP1, 0, REG_ITMP1);
			M_ILD(d, REG_PV, disp);
			emit_store(jd, iptr, iptr->dst, d);
			break;
		
		case ICMD_F2D:       /* ..., value  ==> ..., (double) value           */

			s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			M_FLTMOVE(s1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;
					
		case ICMD_D2F:       /* ..., value  ==> ..., (double) value           */

			s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			M_CVTDF(s1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;
		
		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */
		case ICMD_DCMPL:      /* == => 0, < => 1, > => -1                     */


			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			M_FCMPU(s2, s1);
			M_IADD_IMM(REG_ZERO, -1, d);
			M_BNAN(4);
			M_BGT(3);
			M_IADD_IMM(REG_ZERO, 0, d);
			M_BGE(1);
			M_IADD_IMM(REG_ZERO, 1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */
		case ICMD_DCMPG:      /* == => 0, < => 1, > => -1                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			M_FCMPU(s1, s2);
			M_IADD_IMM(REG_ZERO, 1, d);
			M_BNAN(4);
			M_BGT(3);
			M_IADD_IMM(REG_ZERO, 0, d);
			M_BGE(1);
			M_IADD_IMM(REG_ZERO, -1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;
			
		case ICMD_IF_FCMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_DCMPEQ:

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			M_FCMPU(s1, s2);
			M_BNAN(1);
			M_BEQ(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_FCMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_DCMPNE:

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			M_FCMPU(s1, s2);
			M_BNAN(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_BNE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;


		case ICMD_IF_FCMPL_LT:  /* ..., value, value ==> ...                  */
		case ICMD_IF_DCMPL_LT:

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			M_FCMPU(s1, s2);
			M_BNAN(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_BLT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_FCMPL_GT:  /* ..., value, value ==> ...                  */
		case ICMD_IF_DCMPL_GT:

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			M_FCMPU(s1, s2);
			M_BNAN(1);
			M_BGT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_FCMPL_LE:  /* ..., value, value ==> ...                  */
		case ICMD_IF_DCMPL_LE:

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			M_FCMPU(s1, s2);
			M_BNAN(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_BLE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_FCMPL_GE:  /* ..., value, value ==> ...                  */
		case ICMD_IF_DCMPL_GE:

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			M_FCMPU(s1, s2);
			M_BNAN(1);
			M_BGE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_FCMPG_LT:  /* ..., value, value ==> ...                  */
		case ICMD_IF_DCMPG_LT:

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			M_FCMPU(s1, s2);
			M_BNAN(1);
			M_BLT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_FCMPG_GT:  /* ..., value, value ==> ...                  */
		case ICMD_IF_DCMPG_GT:

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			M_FCMPU(s1, s2);
			M_BNAN(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_BGT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_FCMPG_LE:  /* ..., value, value ==> ...                  */
		case ICMD_IF_DCMPG_LE:

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			M_FCMPU(s1, s2);
			M_BNAN(1);
			M_BLE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_FCMPG_GE:  /* ..., value, value ==> ...                  */
		case ICMD_IF_DCMPG_GE:

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			M_FCMPU(s1, s2);
			M_BNAN(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_BGE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;


		/* memory operations **************************************************/

		case ICMD_ARRAYLENGTH: /* ..., arrayref  ==> ..., length              */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			gen_nullptr_check(s1);
			M_ILD(d, s1, OFFSET(java_arrayheader, size));
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_BALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_IADD_IMM(s2, OFFSET(java_chararray, data[0]), REG_ITMP2);
			M_LBZX(d, s1, REG_ITMP2);
			M_BSEXT(d, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;			

		case ICMD_CALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_SLL_IMM(s2, 1, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_chararray, data[0]), REG_ITMP2);
			M_LHZX(d, s1, REG_ITMP2);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_SALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_SLL_IMM(s2, 1, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_shortarray, data[0]), REG_ITMP2);
			M_LHAX(d, s1, REG_ITMP2);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_SLL_IMM(s2, 2, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_intarray, data[0]), REG_ITMP2);
			M_LWZX(d, s1, REG_ITMP2);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_SLL_IMM(s2, 3, REG_ITMP2);
			M_IADD(s1, REG_ITMP2, REG_ITMP2);
			M_LLD_INTERN(d, REG_ITMP2, OFFSET(java_longarray, data[0]));
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_FALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_SLL_IMM(s2, 2, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_floatarray, data[0]), REG_ITMP2);
			M_LFSX(d, s1, REG_ITMP2);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_DALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_SLL_IMM(s2, 3, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_doublearray, data[0]), REG_ITMP2);
			M_LFDX(d, s1, REG_ITMP2);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_AALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_SLL_IMM(s2, 2, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_objectarray, data[0]), REG_ITMP2);
			M_LWZX(d, s1, REG_ITMP2);
			emit_store(jd, iptr, iptr->dst, d);
			break;


		case ICMD_BASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, src->prev->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			s3 = emit_load_s3(jd, iptr, src, REG_ITMP3);
			M_IADD_IMM(s2, OFFSET(java_bytearray, data[0]), REG_ITMP2);
			M_STBX(s3, s1, REG_ITMP2);
			break;

		case ICMD_CASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, src->prev->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			s3 = emit_load_s3(jd, iptr, src, REG_ITMP3);
			M_SLL_IMM(s2, 1, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_chararray, data[0]), REG_ITMP2);
			M_STHX(s3, s1, REG_ITMP2);
			break;

		case ICMD_SASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, src->prev->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			s3 = emit_load_s3(jd, iptr, src, REG_ITMP3);
			M_SLL_IMM(s2, 1, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_shortarray, data[0]), REG_ITMP2);
			M_STHX(s3, s1, REG_ITMP2);
			break;

		case ICMD_IASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, src->prev->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			s3 = emit_load_s3(jd, iptr, src, REG_ITMP3);
			M_SLL_IMM(s2, 2, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_intarray, data[0]), REG_ITMP2);
			M_STWX(s3, s1, REG_ITMP2);
			break;

		case ICMD_LASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, src->prev->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			s3 = emit_load_s3_high(jd, iptr, src, REG_ITMP3);
			M_SLL_IMM(s2, 3, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_longarray, data[0]), REG_ITMP2);
			M_STWX(s3, s1, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, 4, REG_ITMP2);
			s3 = emit_load_s3_low(jd, iptr, src, REG_ITMP3);
			M_STWX(s3, s1, REG_ITMP2);
			break;

		case ICMD_FASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, src->prev->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			s3 = emit_load_s3(jd, iptr, src, REG_FTMP3);
			M_SLL_IMM(s2, 2, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_floatarray, data[0]), REG_ITMP2);
			M_STFSX(s3, s1, REG_ITMP2);
			break;

		case ICMD_DASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, src->prev->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			s3 = emit_load_s3(jd, iptr, src, REG_FTMP3);
			M_SLL_IMM(s2, 3, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_doublearray, data[0]), REG_ITMP2);
			M_STFDX(s3, s1, REG_ITMP2);
			break;

		case ICMD_AASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, src->prev->prev, rd->argintregs[0]);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			s3 = emit_load_s3(jd, iptr, src, rd->argintregs[1]);

			disp = dseg_addaddress(cd, BUILTIN_canstore);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_MTCTR(REG_ITMP3);

			M_INTMOVE(s1, rd->argintregs[0]);
			M_INTMOVE(s3, rd->argintregs[1]);

			M_JSR;
			M_TST(REG_RESULT);
			M_BEQ(0);
			codegen_add_arraystoreexception_ref(cd);

			s1 = emit_load_s1(jd, iptr, src->prev->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			s3 = emit_load_s3(jd, iptr, src, REG_ITMP3);
			M_SLL_IMM(s2, 2, REG_ITMP2);
			M_IADD_IMM(REG_ITMP2, OFFSET(java_objectarray, data[0]), REG_ITMP2);
			M_STWX(s3, s1, REG_ITMP2);
			break;


		case ICMD_GETSTATIC:  /* ...  ==> ..., value                          */
		                      /* op1 = type, val.a = field address            */

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				disp = dseg_addaddress(cd, NULL);

				codegen_addpatchref(cd, PATCHER_get_putstatic,
									INSTRUCTION_UNRESOLVED_FIELD(iptr), disp);

				if (opt_showdisassemble)
					M_NOP;

			} else {
				fieldinfo *fi = INSTRUCTION_RESOLVED_FIELDINFO(iptr);

				disp = dseg_addaddress(cd, &(fi->value));

				if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					codegen_addpatchref(cd, PATCHER_clinit, fi->class, disp);

					if (opt_showdisassemble)
						M_NOP;
				}
  			}

			M_ALD(REG_ITMP1, REG_PV, disp);
			switch (iptr->op1) {
			case TYPE_INT:
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
				M_ILD_INTERN(d, REG_ITMP1, 0);
				break;
			case TYPE_LNG:
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
				M_ILD_INTERN(GET_LOW_REG(d), REG_ITMP1, 4);/* keep this order */
				M_ILD_INTERN(GET_HIGH_REG(d), REG_ITMP1, 0);/*keep this order */
				break;
			case TYPE_ADR:
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
				M_ALD_INTERN(d, REG_ITMP1, 0);
				break;
			case TYPE_FLT:
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
				M_FLD_INTERN(d, REG_ITMP1, 0);
				break;
			case TYPE_DBL:				
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
				M_DLD_INTERN(d, REG_ITMP1, 0);
				break;
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */
		                      /* op1 = type, val.a = field address            */


			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				disp = dseg_addaddress(cd, NULL);

				codegen_addpatchref(cd, PATCHER_get_putstatic,
									INSTRUCTION_UNRESOLVED_FIELD(iptr), disp);

				if (opt_showdisassemble)
					M_NOP;

			} else {
				fieldinfo *fi = INSTRUCTION_RESOLVED_FIELDINFO(iptr);

				disp = dseg_addaddress(cd, &(fi->value));

				if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					codegen_addpatchref(cd, PATCHER_clinit, fi->class, disp);

					if (opt_showdisassemble)
						M_NOP;
				}
  			}

			M_ALD(REG_ITMP1, REG_PV, disp);
			switch (iptr->op1) {
			case TYPE_INT:
				s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
				M_IST_INTERN(s2, REG_ITMP1, 0);
				break;
			case TYPE_LNG:
				s2 = emit_load_s2(jd, iptr, src, PACK_REGS(REG_ITMP2, REG_ITMP3));
				M_LST_INTERN(s2, REG_ITMP1, 0);
				break;
			case TYPE_ADR:
				s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
				M_AST_INTERN(s2, REG_ITMP1, 0);
				break;
			case TYPE_FLT:
				s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
				M_FST_INTERN(s2, REG_ITMP1, 0);
				break;
			case TYPE_DBL:
				s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
				M_DST_INTERN(s2, REG_ITMP1, 0);
				break;
			}
			break;


		case ICMD_GETFIELD:   /* ...  ==> ..., value                          */
		                      /* op1 = type, val.i = field offset             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			gen_nullptr_check(s1);

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				codegen_addpatchref(cd, PATCHER_get_putfield,
									INSTRUCTION_UNRESOLVED_FIELD(iptr), 0);

				if (opt_showdisassemble)
					M_NOP;

				disp = 0;

			} else {
				disp = INSTRUCTION_RESOLVED_FIELDINFO(iptr)->offset;
			}

			switch (iptr->op1) {
			case TYPE_INT:
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
				M_ILD(d, s1, disp);
				break;
			case TYPE_LNG:
   				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, PACK_REGS(REG_ITMP2, REG_ITMP1));
				if (GET_HIGH_REG(d) == s1) {
					M_ILD(GET_LOW_REG(d), s1, disp + 4);
					M_ILD(GET_HIGH_REG(d), s1, disp);
				} else {
					M_ILD(GET_HIGH_REG(d), s1, disp);
					M_ILD(GET_LOW_REG(d), s1, disp + 4);
				}
				break;
			case TYPE_ADR:
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
				M_ALD(d, s1, disp);
				break;
			case TYPE_FLT:
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
				M_FLD(d, s1, disp);
				break;
			case TYPE_DBL:				
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
				M_DLD(d, s1, disp);
				break;
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_PUTFIELD:   /* ..., value  ==> ...                          */
		                      /* op1 = type, val.i = field offset             */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			gen_nullptr_check(s1);

			if (!IS_FLT_DBL_TYPE(iptr->op1)) {
				if (IS_2_WORD_TYPE(iptr->op1)) {
					s2 = emit_load_s2(jd, iptr, src, PACK_REGS(REG_ITMP2, REG_ITMP3));
				} else {
					s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
				}
			} else {
				s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			}

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				codegen_addpatchref(cd, PATCHER_get_putfield,
									INSTRUCTION_UNRESOLVED_FIELD(iptr), 0);

				if (opt_showdisassemble)
					M_NOP;

				disp = 0;

			} else {
				disp = INSTRUCTION_RESOLVED_FIELDINFO(iptr)->offset;
			}

			switch (iptr->op1) {
			case TYPE_INT:
				M_IST(s2, s1, disp);
				break;
			case TYPE_LNG:
				M_IST(GET_LOW_REG(s2), s1, disp + 4);      /* keep this order */
				M_IST(GET_HIGH_REG(s2), s1, disp);         /* keep this order */
				break;
			case TYPE_ADR:
				M_AST(s2, s1, disp);
				break;
			case TYPE_FLT:
				M_FST(s2, s1, disp);
				break;
			case TYPE_DBL:
				M_DST(s2, s1, disp);
				break;
			}
			break;


		/* branch operations **************************************************/

		case ICMD_ATHROW:       /* ..., objectref ==> ... (, objectref)       */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			M_INTMOVE(s1, REG_ITMP1_XPTR);

#ifdef ENABLE_VERIFIER
			if (iptr->val.a) {
				codegen_addpatchref(cd, PATCHER_athrow_areturn,
									(unresolved_class *) iptr->val.a, 0);

				if (opt_showdisassemble)
					M_NOP;
			}
#endif /* ENABLE_VERIFIER */

			disp = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP2, REG_PV, disp);
			M_MTCTR(REG_ITMP2);

			if (jd->isleafmethod)
				M_MFLR(REG_ITMP3);                          /* save LR        */

			M_BL(0);                                        /* get current PC */
			M_MFLR(REG_ITMP2_XPC);

			if (jd->isleafmethod)
				M_MTLR(REG_ITMP3);                          /* restore LR     */

			M_RTS;                                          /* jump to CTR    */
			ALIGNCODENOP;
			break;

		case ICMD_GOTO:         /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */
			M_BR(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			ALIGNCODENOP;
			break;

		case ICMD_JSR:          /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */

			if (jd->isleafmethod)
				M_MFLR(REG_ITMP2);

			M_BL(0);
			M_MFLR(REG_ITMP1);
			M_IADD_IMM(REG_ITMP1, jd->isleafmethod ? 4*4 : 3*4, REG_ITMP1);

			if (jd->isleafmethod)
				M_MTLR(REG_ITMP2);

			M_BR(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;
			
		case ICMD_RET:          /* ... ==> ...                                */
		                        /* op1 = local variable                       */

			var = &(rd->locals[iptr->op1][TYPE_ADR]);
			if (var->flags & INMEMORY) {
				M_ALD(REG_ITMP1, REG_SP, var->regoff * 4);
				M_MTCTR(REG_ITMP1);
			} else {
				M_MTCTR(var->regoff);
			}
			M_RTS;
			ALIGNCODENOP;
			break;

		case ICMD_IFNULL:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			M_TST(s1);
			M_BEQ(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IFNONNULL:    /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			M_TST(s1);
			M_BNE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IFLT:
		case ICMD_IFLE:
		case ICMD_IFNE:
		case ICMD_IFGT:
		case ICMD_IFGE:
		case ICMD_IFEQ:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			if ((iptr->val.i >= -32768) && (iptr->val.i <= 32767))
				M_CMPI(s1, iptr->val.i);
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_CMP(s1, REG_ITMP2);
			}
			switch (iptr->opc) {
			case ICMD_IFLT:
				M_BLT(0);
				break;
			case ICMD_IFLE:
				M_BLE(0);
				break;
			case ICMD_IFNE:
				M_BNE(0);
				break;
			case ICMD_IFGT:
				M_BGT(0);
				break;
			case ICMD_IFGE:
				M_BGE(0);
				break;
			case ICMD_IFEQ:
				M_BEQ(0);
				break;
			}
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;


		case ICMD_IF_LEQ:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			s1 = emit_load_s1_low(jd, iptr, src, REG_ITMP1);
			s2 = emit_load_s2_high(jd, iptr, src, REG_ITMP2);
			if (iptr->val.l == 0) {
				M_OR_TST(s1, s2, REG_ITMP3);
  			} else if ((iptr->val.l >= 0) && (iptr->val.l <= 0xffff)) {
				M_XOR_IMM(s2, 0, REG_ITMP2);
				M_XOR_IMM(s1, iptr->val.l & 0xffff, REG_ITMP1);
				M_OR_TST(REG_ITMP1, REG_ITMP2, REG_ITMP3);
  			} else {
				ICONST(REG_ITMP3, iptr->val.l & 0xffffffff);
				M_XOR(s1, REG_ITMP3, REG_ITMP1);
				ICONST(REG_ITMP3, iptr->val.l >> 32);
				M_XOR(s2, REG_ITMP3, REG_ITMP2);
				M_OR_TST(REG_ITMP1, REG_ITMP2, REG_ITMP3);
			}
			M_BEQ(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;
			
		case ICMD_IF_LLT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */
			s1 = emit_load_s1_low(jd, iptr, src, REG_ITMP1);
			s2 = emit_load_s2_high(jd, iptr, src, REG_ITMP2);
			if (iptr->val.l == 0) {
				/* if high word is less than zero, the whole long is too */
				M_CMPI(s2, 0);
			} else if ((iptr->val.l >= 0) && (iptr->val.l <= 0xffff)) {
  				M_CMPI(s2, 0);
				M_BLT(0);
				codegen_addreference(cd, (basicblock *) iptr->target);
				M_BGT(2);
  				M_CMPUI(s1, iptr->val.l & 0xffff);
  			} else {
  				ICONST(REG_ITMP3, iptr->val.l >> 32);
  				M_CMP(s2, REG_ITMP3);
				M_BLT(0);
				codegen_addreference(cd, (basicblock *) iptr->target);
				M_BGT(3);
  				ICONST(REG_ITMP3, iptr->val.l & 0xffffffff);
				M_CMPU(s1, REG_ITMP3);
			}
			M_BLT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;
			
		case ICMD_IF_LLE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			s1 = emit_load_s1_low(jd, iptr, src, REG_ITMP1);
			s2 = emit_load_s2_high(jd, iptr, src, REG_ITMP2);
/*  			if (iptr->val.l == 0) { */
/*  				M_OR(s1, s2, REG_ITMP3); */
/*  				M_CMPI(REG_ITMP3, 0); */

/*    			} else  */
			if ((iptr->val.l >= 0) && (iptr->val.l <= 0xffff)) {
  				M_CMPI(s2, 0);
				M_BLT(0);
				codegen_addreference(cd, (basicblock *) iptr->target);
				M_BGT(2);
  				M_CMPUI(s1, iptr->val.l & 0xffff);
  			} else {
  				ICONST(REG_ITMP3, iptr->val.l >> 32);
  				M_CMP(s2, REG_ITMP3);
				M_BLT(0);
				codegen_addreference(cd, (basicblock *) iptr->target);
				M_BGT(3);
  				ICONST(REG_ITMP3, iptr->val.l & 0xffffffff);
				M_CMPU(s1, REG_ITMP3);
			}
			M_BLE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;
			
		case ICMD_IF_LNE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			s1 = emit_load_s1_low(jd, iptr, src, REG_ITMP1);
			s2 = emit_load_s2_high(jd, iptr, src, REG_ITMP2);
			if (iptr->val.l == 0) {
				M_OR_TST(s1, s2, REG_ITMP3);
			} else if ((iptr->val.l >= 0) && (iptr->val.l <= 0xffff)) {
				M_XOR_IMM(s2, 0, REG_ITMP2);
				M_XOR_IMM(s1, iptr->val.l & 0xffff, REG_ITMP1);
				M_OR_TST(REG_ITMP1, REG_ITMP2, REG_ITMP3);
  			} else {
				ICONST(REG_ITMP3, iptr->val.l & 0xffffffff);
				M_XOR(s1, REG_ITMP3, REG_ITMP1);
				ICONST(REG_ITMP3, iptr->val.l >> 32);
				M_XOR(s2, REG_ITMP3, REG_ITMP2);
				M_OR_TST(REG_ITMP1, REG_ITMP2, REG_ITMP3);
			}
			M_BNE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;
			
		case ICMD_IF_LGT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			s1 = emit_load_s1_low(jd, iptr, src, REG_ITMP1);
			s2 = emit_load_s2_high(jd, iptr, src, REG_ITMP2);
/*  			if (iptr->val.l == 0) { */
/*  				M_OR(s1, s2, REG_ITMP3); */
/*  				M_CMPI(REG_ITMP3, 0); */

/*    			} else  */
			if ((iptr->val.l >= 0) && (iptr->val.l <= 0xffff)) {
  				M_CMPI(s2, 0);
				M_BGT(0);
				codegen_addreference(cd, (basicblock *) iptr->target);
				M_BLT(2);
  				M_CMPUI(s1, iptr->val.l & 0xffff);
  			} else {
  				ICONST(REG_ITMP3, iptr->val.l >> 32);
  				M_CMP(s2, REG_ITMP3);
				M_BGT(0);
				codegen_addreference(cd, (basicblock *) iptr->target);
				M_BLT(3);
  				ICONST(REG_ITMP3, iptr->val.l & 0xffffffff);
				M_CMPU(s1, REG_ITMP3);
			}
			M_BGT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;
			
		case ICMD_IF_LGE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			s1 = emit_load_s1_low(jd, iptr, src, REG_ITMP1);
			s2 = emit_load_s2_high(jd, iptr, src, REG_ITMP2);
			if (iptr->val.l == 0) {
				/* if high word is greater equal zero, the whole long is too */
				M_CMPI(s2, 0);
			} else if ((iptr->val.l >= 0) && (iptr->val.l <= 0xffff)) {
  				M_CMPI(s2, 0);
				M_BGT(0);
				codegen_addreference(cd, (basicblock *) iptr->target);
				M_BLT(2);
  				M_CMPUI(s1, iptr->val.l & 0xffff);
  			} else {
  				ICONST(REG_ITMP3, iptr->val.l >> 32);
  				M_CMP(s2, REG_ITMP3);
				M_BGT(0);
				codegen_addreference(cd, (basicblock *) iptr->target);
				M_BLT(3);
  				ICONST(REG_ITMP3, iptr->val.l & 0xffffffff);
				M_CMPU(s1, REG_ITMP3);
			}
			M_BGE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_ICMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_ACMPEQ:    /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BEQ(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_LCMPEQ:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1_high(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2_high(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			/* load low-bits before the branch, so we know the distance */
			s1 = emit_load_s1_low(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2_low(jd, iptr, src, REG_ITMP2);
			M_BNE(2);
			M_CMP(s1, s2);
			M_BEQ(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_ICMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_ACMPNE:    /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BNE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_LCMPNE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1_high(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2_high(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BNE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			s1 = emit_load_s1_low(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2_low(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BNE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_ICMPLT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BLT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_LCMPLT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1_high(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2_high(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BLT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			/* load low-bits before the branch, so we know the distance */
			s1 = emit_load_s1_low(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2_low(jd, iptr, src, REG_ITMP2);
			M_BGT(2);
			M_CMPU(s1, s2);
			M_BLT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_ICMPGT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BGT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_LCMPGT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1_high(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2_high(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BGT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			/* load low-bits before the branch, so we know the distance */	
			s1 = emit_load_s1_low(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2_low(jd, iptr, src, REG_ITMP2);
			M_BLT(2);
			M_CMPU(s1, s2);
			M_BGT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_ICMPLE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BLE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_LCMPLE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1_high(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2_high(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BLT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			/* load low-bits before the branch, so we know the distance */
			s1 = emit_load_s1_low(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2_low(jd, iptr, src, REG_ITMP2);
			M_BGT(2);
			M_CMPU(s1, s2);
			M_BLE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_ICMPGE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BGE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_LCMPGE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1_high(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2_high(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BGT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			/* load low-bits before the branch, so we know the distance */
			s1 = emit_load_s1_low(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2_low(jd, iptr, src, REG_ITMP2);
			M_BLT(2);
			M_CMPU(s1, s2);
			M_BGE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IRETURN:      /* ..., retvalue ==> ...                      */

			s1 = emit_load_s1(jd, iptr, src, REG_RESULT);
			M_INTMOVE(s1, REG_RESULT);
			goto nowperformreturn;

		case ICMD_ARETURN:      /* ..., retvalue ==> ...                      */

			s1 = emit_load_s1(jd, iptr, src, REG_RESULT);
			M_INTMOVE(s1, REG_RESULT);

#ifdef ENABLE_VERIFIER
			if (iptr->val.a) {
				codegen_addpatchref(cd, PATCHER_athrow_areturn,
									(unresolved_class *) iptr->val.a, 0);

				if (opt_showdisassemble)
					M_NOP;
			}
#endif /* ENABLE_VERIFIER */
			goto nowperformreturn;

		case ICMD_LRETURN:      /* ..., retvalue ==> ...                      */

			s1 = emit_load_s1(jd, iptr, src, PACK_REGS(REG_RESULT2, REG_RESULT));
			M_LNGMOVE(s1, PACK_REGS(REG_RESULT2, REG_RESULT));
			goto nowperformreturn;

		case ICMD_FRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_DRETURN:

			s1 = emit_load_s1(jd, iptr, src, REG_FRESULT);
			M_FLTMOVE(s1, REG_FRESULT);
			goto nowperformreturn;

		case ICMD_RETURN:      /* ...  ==> ...                                */

nowperformreturn:
			{
			s4 i, p;
			
			p = stackframesize;

			/* call trace function */

			if (JITDATA_HAS_FLAG_VERBOSECALL(jd)) {
				M_MFLR(REG_ZERO);
				M_LDA(REG_SP, REG_SP, -10 * 8);
				M_DST(REG_FRESULT, REG_SP, 48+0);
				M_IST(REG_RESULT, REG_SP, 48+8);
				M_AST(REG_ZERO, REG_SP, 48+12);
				M_IST(REG_RESULT2, REG_SP, 48+16);

				/* keep this order */
				switch (iptr->opc) {
				case ICMD_IRETURN:
				case ICMD_ARETURN:
#if defined(__DARWIN__)
					M_MOV(REG_RESULT, rd->argintregs[2]);
					M_CLR(rd->argintregs[1]);
#else
					M_MOV(REG_RESULT, rd->argintregs[3]);
					M_CLR(rd->argintregs[2]);
#endif
					break;

				case ICMD_LRETURN:
#if defined(__DARWIN__)
					M_MOV(REG_RESULT2, rd->argintregs[2]);
					M_MOV(REG_RESULT, rd->argintregs[1]);
#else
					M_MOV(REG_RESULT2, rd->argintregs[3]);
					M_MOV(REG_RESULT, rd->argintregs[2]);
#endif
					break;
				}

				disp = dseg_addaddress(cd, m);
				M_ALD(rd->argintregs[0], REG_PV, disp);

				M_FLTMOVE(REG_FRESULT, rd->argfltregs[0]);
				M_FLTMOVE(REG_FRESULT, rd->argfltregs[1]);
				disp = dseg_addaddress(cd, builtin_displaymethodstop);
				M_ALD(REG_ITMP2, REG_PV, disp);
				M_MTCTR(REG_ITMP2);
				M_JSR;

				M_DLD(REG_FRESULT, REG_SP, 48+0);
				M_ILD(REG_RESULT, REG_SP, 48+8);
				M_ALD(REG_ZERO, REG_SP, 48+12);
				M_ILD(REG_RESULT2, REG_SP, 48+16);
				M_LDA(REG_SP, REG_SP, 10 * 8);
				M_MTLR(REG_ZERO);
			}
			
#if defined(ENABLE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				disp = dseg_addaddress(cd, BUILTIN_monitorexit);
				M_ALD(REG_ITMP3, REG_PV, disp);
				M_MTCTR(REG_ITMP3);

				/* we need to save the proper return value */

				switch (iptr->opc) {
				case ICMD_LRETURN:
					M_IST(REG_RESULT2, REG_SP, rd->memuse * 4 + 8);
					/* fall through */
				case ICMD_IRETURN:
				case ICMD_ARETURN:
					M_IST(REG_RESULT , REG_SP, rd->memuse * 4 + 4);
					break;
				case ICMD_FRETURN:
					M_FST(REG_FRESULT, REG_SP, rd->memuse * 4 + 4);
					break;
				case ICMD_DRETURN:
					M_DST(REG_FRESULT, REG_SP, rd->memuse * 4 + 4);
					break;
				}

				M_ALD(rd->argintregs[0], REG_SP, rd->memuse * 4);
				M_JSR;

				/* and now restore the proper return value */

				switch (iptr->opc) {
				case ICMD_LRETURN:
					M_ILD(REG_RESULT2, REG_SP, rd->memuse * 4 + 8);
					/* fall through */
				case ICMD_IRETURN:
				case ICMD_ARETURN:
					M_ILD(REG_RESULT , REG_SP, rd->memuse * 4 + 4);
					break;
				case ICMD_FRETURN:
					M_FLD(REG_FRESULT, REG_SP, rd->memuse * 4 + 4);
					break;
				case ICMD_DRETURN:
					M_DLD(REG_FRESULT, REG_SP, rd->memuse * 4 + 4);
					break;
				}
			}
#endif

			/* restore return address                                         */

			if (!jd->isleafmethod) {
				/* ATTENTION: Don't use REG_ZERO (r0) here, as M_ALD
				   may have a displacement overflow. */

				M_ALD(REG_ITMP1, REG_SP, p * 4 + LA_LR_OFFSET);
				M_MTLR(REG_ITMP1);
			}

			/* restore saved registers                                        */

			for (i = INT_SAV_CNT - 1; i >= rd->savintreguse; i--) {
				p--; M_ILD(rd->savintregs[i], REG_SP, p * 4);
			}
			for (i = FLT_SAV_CNT - 1; i >= rd->savfltreguse; i--) {
				p -= 2; M_DLD(rd->savfltregs[i], REG_SP, p * 4);
			}

			/* deallocate stack                                               */

			if (stackframesize)
				M_LDA(REG_SP, REG_SP, stackframesize * 4);

			M_RET;
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
			
			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			if (l == 0) {
				M_INTMOVE(s1, REG_ITMP1);
			} else if (l <= 32768) {
				M_LDA(REG_ITMP1, s1, -l);
			} else {
				ICONST(REG_ITMP2, l);
				M_ISUB(s1, REG_ITMP2, REG_ITMP1);
			}
			i = i - l + 1;

			/* range check */

			M_CMPUI(REG_ITMP1, i - 1);
			M_BGT(0);
			codegen_addreference(cd, (basicblock *) tptr[0]);

			/* build jump table top down and use address of lowest entry */

			/* s4ptr += 3 + i; */
			tptr += i;

			while (--i >= 0) {
				dseg_addtarget(cd, (basicblock *) tptr[0]); 
				--tptr;
			}
			}

			/* length of dataseg after last dseg_addtarget is used by load */

			M_SLL_IMM(REG_ITMP1, 2, REG_ITMP1);
			M_IADD(REG_ITMP1, REG_PV, REG_ITMP2);
			M_ALD(REG_ITMP2, REG_ITMP2, -(cd->dseglen));
			M_MTCTR(REG_ITMP2);
			M_RTS;
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
			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			while (--i >= 0) {
				s4ptr += 2;
				++tptr;

				val = s4ptr[0];
				if ((val >= -32768) && (val <= 32767)) {
					M_CMPI(s1, val);
				} else {
					a = dseg_adds4(cd, val);
					M_ILD(REG_ITMP2, REG_PV, a);
					M_CMP(s1, REG_ITMP2);
				}
				M_BEQ(0);
				codegen_addreference(cd, (basicblock *) tptr[0]); 
			}

			M_BR(0);
			tptr = (void **) iptr->target;
			codegen_addreference(cd, (basicblock *) tptr[0]);

			ALIGNCODENOP;
			break;
			}


		case ICMD_BUILTIN:      /* ..., [arg1, [arg2 ...]] ==> ...            */
		                        /* op1 = arg count val.a = builtintable entry */

			bte = iptr->val.a;
			md = bte->md;
			goto gen_method;

		case ICMD_INVOKESTATIC: /* ..., [arg1, [arg2 ...]] ==> ...            */
		                        /* op1 = arg count, val.a = method pointer    */

		case ICMD_INVOKESPECIAL:/* ..., objectref, [arg1, [arg2 ...]] ==> ... */
		case ICMD_INVOKEVIRTUAL:/* op1 = arg count, val.a = method pointer    */
		case ICMD_INVOKEINTERFACE:

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				md = INSTRUCTION_UNRESOLVED_METHOD(iptr)->methodref->parseddesc.md;
				lm = NULL;
			}
			else {
				lm = INSTRUCTION_RESOLVED_METHODINFO(iptr);
				md = lm->parseddesc;
			}

gen_method:
			s3 = md->paramcount;

			MCODECHECK((s3 << 1) + 64);

			/* copy arguments to registers or stack location */

			for (s3 = s3 - 1; s3 >= 0; s3--, src = src->prev) {
				if (src->varkind == ARGVAR)
					continue;
				if (IS_INT_LNG_TYPE(src->type)) {
					if (!md->params[s3].inmemory) {
						if (IS_2_WORD_TYPE(src->type)) {
							s1 = PACK_REGS(
						   rd->argintregs[GET_LOW_REG(md->params[s3].regoff)],
						   rd->argintregs[GET_HIGH_REG(md->params[s3].regoff)]);
							d = emit_load_s1(jd, iptr, src, s1);
							M_LNGMOVE(d, s1);
						} else {
							s1 = rd->argintregs[md->params[s3].regoff];
							d = emit_load_s1(jd, iptr, src, s1);
							M_INTMOVE(d, s1);
						}

					} else {
						if (IS_2_WORD_TYPE(src->type)) {
							d = emit_load_s1(jd, iptr, src, PACK_REGS(REG_ITMP2, REG_ITMP1));
							M_LST(d, REG_SP, md->params[s3].regoff * 4);
						} else {
							d = emit_load_s1(jd, iptr, src, REG_ITMP1);
							M_IST(d, REG_SP, md->params[s3].regoff * 4);
						}
					}
						
				} else {
					if (!md->params[s3].inmemory) {
						s1 = rd->argfltregs[md->params[s3].regoff];
						d = emit_load_s1(jd, iptr, src, s1);
						M_FLTMOVE(d, s1);

					} else {
						d = emit_load_s1(jd, iptr, src, REG_FTMP1);
						if (IS_2_WORD_TYPE(src->type))
							M_DST(d, REG_SP, md->params[s3].regoff * 4);
						else
							M_FST(d, REG_SP, md->params[s3].regoff * 4);
					}
				}
			} /* end of for */

			switch (iptr->opc) {
			case ICMD_BUILTIN:
				disp = dseg_addaddress(cd, bte->fp);
				d = md->returntype.type;

				M_ALD(REG_PV, REG_PV, disp);  /* pointer to built-in-function */
				M_MTCTR(REG_PV);
				M_JSR;
				disp = (s4) (cd->mcodeptr - cd->mcodebase);
				M_MFLR(REG_ITMP1);
				M_LDA(REG_PV, REG_ITMP1, -disp);

				/* if op1 == true, we need to check for an exception */

				if (iptr->op1 == true) {
					M_CMPI(REG_RESULT, 0);
					M_BEQ(0);
					codegen_add_fillinstacktrace_ref(cd);
				}
				break;

			case ICMD_INVOKESPECIAL:
				gen_nullptr_check(rd->argintregs[0]);
				M_ILD(REG_ITMP1, rd->argintregs[0], 0); /* hardware nullptr   */
				/* fall through */

			case ICMD_INVOKESTATIC:
				if (lm == NULL) {
					unresolved_method *um = INSTRUCTION_UNRESOLVED_METHOD(iptr);

					disp = dseg_addaddress(cd, NULL);

					codegen_addpatchref(cd, PATCHER_invokestatic_special,
										um, disp);

					if (opt_showdisassemble)
						M_NOP;

					d = md->returntype.type;

				} else {
					disp = dseg_addaddress(cd, lm->stubroutine);
					d = md->returntype.type;
				}

				M_ALD(REG_PV, REG_PV, disp);
				M_MTCTR(REG_PV);
				M_JSR;
				disp = (s4) (cd->mcodeptr - cd->mcodebase);
				M_MFLR(REG_ITMP1);
				M_LDA(REG_PV, REG_ITMP1, -disp);
				break;

			case ICMD_INVOKEVIRTUAL:
				gen_nullptr_check(rd->argintregs[0]);

				if (lm == NULL) {
					unresolved_method *um = INSTRUCTION_UNRESOLVED_METHOD(iptr);

					codegen_addpatchref(cd, PATCHER_invokevirtual, um, 0);

					if (opt_showdisassemble)
						M_NOP;

					s1 = 0;
					d = md->returntype.type;

				} else {
					s1 = OFFSET(vftbl_t, table[0]) +
						sizeof(methodptr) * lm->vftblindex;
					d = md->returntype.type;
				}

				M_ALD(REG_METHODPTR, rd->argintregs[0],
					  OFFSET(java_objectheader, vftbl));
				M_ALD(REG_PV, REG_METHODPTR, s1);
				M_MTCTR(REG_PV);
				M_JSR;
				disp = (s4) (cd->mcodeptr - cd->mcodebase);
				M_MFLR(REG_ITMP1);
				M_LDA(REG_PV, REG_ITMP1, -disp);
				break;

			case ICMD_INVOKEINTERFACE:
				gen_nullptr_check(rd->argintregs[0]);

				if (lm == NULL) {
					unresolved_method *um = INSTRUCTION_UNRESOLVED_METHOD(iptr);

					codegen_addpatchref(cd, PATCHER_invokeinterface, um, 0);

					if (opt_showdisassemble)
						M_NOP;

					s1 = 0;
					s2 = 0;
					d = md->returntype.type;

				} else {
					s1 = OFFSET(vftbl_t, interfacetable[0]) -
						sizeof(methodptr*) * lm->class->index;

					s2 = sizeof(methodptr) * (lm - lm->class->methods);

					d = md->returntype.type;
				}

				M_ALD(REG_METHODPTR, rd->argintregs[0],
					  OFFSET(java_objectheader, vftbl));    
				M_ALD(REG_METHODPTR, REG_METHODPTR, s1);
				M_ALD(REG_PV, REG_METHODPTR, s2);
				M_MTCTR(REG_PV);
				M_JSR;
				disp = (s4) (cd->mcodeptr - cd->mcodebase);
				M_MFLR(REG_ITMP1);
				M_LDA(REG_PV, REG_ITMP1, -disp);
				break;
			}

			/* d contains return type */

			if (d != TYPE_VOID) {
				if (IS_INT_LNG_TYPE(iptr->dst->type)) {
					if (IS_2_WORD_TYPE(iptr->dst->type)) {
						s1 = codegen_reg_of_var(rd, iptr->opc, iptr->dst,
										PACK_REGS(REG_RESULT2, REG_RESULT));
						M_LNGMOVE(PACK_REGS(REG_RESULT2, REG_RESULT), s1);
					} else {
						s1 = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_RESULT);
						M_INTMOVE(REG_RESULT, s1);
					}
				} else {
					s1 = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FRESULT);
					M_FLTMOVE(REG_FRESULT, s1);
				}
				emit_store(jd, iptr, iptr->dst, s1);
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

			if (iptr->op1 == 1) {
				/* object type cast-check */

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
			
#if defined(ENABLE_THREADS)
				codegen_threadcritrestart(cd, cd->mcodeptr - cd->mcodebase);
#endif
				s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);

				/* calculate interface checkcast code size */

				s2 = 7;
				if (!super)
					s2 += (opt_showdisassemble ? 1 : 0);

				/* calculate class checkcast code size */

				s3 = 8 + (s1 == REG_ITMP1);
				if (!super)
					s3 += (opt_showdisassemble ? 1 : 0);

				/* if class is not resolved, check which code to call */

				if (!super) {
					M_TST(s1);
					M_BEQ(3 + (opt_showdisassemble ? 1 : 0) + s2 + 1 + s3);

					disp = dseg_adds4(cd, 0);                     /* super->flags */

					codegen_addpatchref(cd,
										PATCHER_checkcast_instanceof_flags,
										(constant_classref *) iptr->target,
										disp);

					if (opt_showdisassemble)
						M_NOP;

					M_ILD(REG_ITMP2, REG_PV, disp);
					M_AND_IMM(REG_ITMP2, ACC_INTERFACE, REG_ITMP2);
					M_BEQ(s2 + 1);
				}

				/* interface checkcast code */

				if ((super == NULL) || (super->flags & ACC_INTERFACE)) {
					if (super != NULL) {
						M_TST(s1);
						M_BEQ(s2);
					}
					else {
						codegen_addpatchref(cd,
											PATCHER_checkcast_instanceof_interface,
											(constant_classref *) iptr->target,
											0);

						if (opt_showdisassemble)
							M_NOP;
					}

					M_ALD(REG_ITMP2, s1, OFFSET(java_objectheader, vftbl));
					M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl_t, interfacetablelength));
					M_LDATST(REG_ITMP3, REG_ITMP3, -superindex);
					M_BLE(0);
					codegen_add_classcastexception_ref(cd, s1);
					M_ALD(REG_ITMP3, REG_ITMP2,
						  OFFSET(vftbl_t, interfacetable[0]) -
						  superindex * sizeof(methodptr*));
					M_TST(REG_ITMP3);
					M_BEQ(0);
					codegen_add_classcastexception_ref(cd, s1);

					if (super == NULL)
						M_BR(s3);
				}

				/* class checkcast code */

				if ((super == NULL) || !(super->flags & ACC_INTERFACE)) {
					disp = dseg_addaddress(cd, supervftbl);

					if (super != NULL) {
						M_TST(s1);
						M_BEQ(s3);
					}
					else {
						codegen_addpatchref(cd, PATCHER_checkcast_class,
											(constant_classref *) iptr->target,
											disp);

						if (opt_showdisassemble)
							M_NOP;
					}

					M_ALD(REG_ITMP2, s1, OFFSET(java_objectheader, vftbl));
#if defined(ENABLE_THREADS)
					codegen_threadcritstart(cd, cd->mcodeptr - cd->mcodebase);
#endif
					M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl_t, baseval));
					M_ALD(REG_ITMP2, REG_PV, disp);
					if (s1 != REG_ITMP1) {
						M_ILD(REG_ITMP1, REG_ITMP2, OFFSET(vftbl_t, baseval));
						M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, diffval));
#if defined(ENABLE_THREADS)
						codegen_threadcritstop(cd, cd->mcodeptr - cd->mcodebase);
#endif
						M_ISUB(REG_ITMP3, REG_ITMP1, REG_ITMP3);
					} else {
						M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, baseval));
						M_ISUB(REG_ITMP3, REG_ITMP2, REG_ITMP3);
						M_ALD(REG_ITMP2, REG_PV, disp);
						M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, diffval));
#if defined(ENABLE_THREADS)
						codegen_threadcritstop(cd, cd->mcodeptr - cd->mcodebase);
#endif
					}
					M_CMPU(REG_ITMP3, REG_ITMP2);
					M_BGT(0);
					codegen_add_classcastexception_ref(cd, s1);
				}
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, s1);
			}
			else {
				/* array type cast-check */

				s1 = emit_load_s1(jd, iptr, src, rd->argintregs[0]);
				M_INTMOVE(s1, rd->argintregs[0]);

				disp = dseg_addaddress(cd, iptr->val.a);

				if (iptr->val.a == NULL) {
					codegen_addpatchref(cd, PATCHER_builtin_arraycheckcast,
										(constant_classref *) iptr->target,
										disp);

					if (opt_showdisassemble)
						M_NOP;
				}

				M_ALD(rd->argintregs[1], REG_PV, disp);
				disp = dseg_addaddress(cd, BUILTIN_arraycheckcast);
				M_ALD(REG_ITMP2, REG_PV, disp);
				M_MTCTR(REG_ITMP2);
				M_JSR;
				M_TST(REG_RESULT);
				M_BEQ(0);
				codegen_add_classcastexception_ref(cd, s1);

				s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, s1);
			}
			M_INTMOVE(s1, d);
			emit_store(jd, iptr, iptr->dst, d);
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

			super = (classinfo *) iptr->val.a;

			if (!super) {
				superindex = 0;
				supervftbl = NULL;

			} else {
				superindex = super->index;
				supervftbl = super->vftbl;
			}
			
#if defined(ENABLE_THREADS)
            codegen_threadcritrestart(cd, cd->mcodeptr - cd->mcodebase);
#endif
			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if (s1 == d) {
				M_MOV(s1, REG_ITMP1);
				s1 = REG_ITMP1;
			}

			/* calculate interface instanceof code size */

			s2 = 8;
			if (!super)
				s2 += (opt_showdisassemble ? 1 : 0);

			/* calculate class instanceof code size */

			s3 = 10;
			if (!super)
				s3 += (opt_showdisassemble ? 1 : 0);

			M_CLR(d);

			/* if class is not resolved, check which code to call */

			if (!super) {
				M_TST(s1);
				M_BEQ(3 + (opt_showdisassemble ? 1 : 0) + s2 + 1 + s3);

				disp = dseg_adds4(cd, 0);                     /* super->flags */

				codegen_addpatchref(cd, PATCHER_checkcast_instanceof_flags,
									(constant_classref *) iptr->target, disp);

				if (opt_showdisassemble)
					M_NOP;

				M_ILD(REG_ITMP3, REG_PV, disp);
				M_AND_IMM(REG_ITMP3, ACC_INTERFACE, REG_ITMP3);
				M_BEQ(s2 + 1);
			}

			/* interface instanceof code */

			if (!super || (super->flags & ACC_INTERFACE)) {
				if (super) {
					M_TST(s1);
					M_BEQ(s2);

				} else {
					codegen_addpatchref(cd,
										PATCHER_checkcast_instanceof_interface,
										(constant_classref *) iptr->target, 0);

					if (opt_showdisassemble)
						M_NOP;
				}

				M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
				M_ILD(REG_ITMP3, REG_ITMP1, OFFSET(vftbl_t, interfacetablelength));
				M_LDATST(REG_ITMP3, REG_ITMP3, -superindex);
				M_BLE(4);
				M_ALD(REG_ITMP1, REG_ITMP1,
					  OFFSET(vftbl_t, interfacetable[0]) -
					  superindex * sizeof(methodptr*));
				M_TST(REG_ITMP1);
				M_BEQ(1);
				M_IADD_IMM(REG_ZERO, 1, d);

				if (!super)
					M_BR(s3);
			}

			/* class instanceof code */

			if (!super || !(super->flags & ACC_INTERFACE)) {
				disp = dseg_addaddress(cd, supervftbl);

				if (super) {
					M_TST(s1);
					M_BEQ(s3);

				} else {
					codegen_addpatchref(cd, PATCHER_instanceof_class,
										(constant_classref *) iptr->target,
										disp);

					if (opt_showdisassemble) {
						M_NOP;
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
				M_CMPU(REG_ITMP1, REG_ITMP2);
				M_CLR(d);
				M_BGT(1);
				M_IADD_IMM(REG_ZERO, 1, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			}
			break;

		case ICMD_MULTIANEWARRAY:/* ..., cnt1, [cnt2, ...] ==> ..., arrayref  */
		                      /* op1 = dimension, val.a = class               */

			/* check for negative sizes and copy sizes to stack if necessary  */

			MCODECHECK((iptr->op1 << 1) + 64);

			for (s1 = iptr->op1; --s1 >= 0; src = src->prev) {
				/* copy SAVEDVAR sizes to stack */

				if (src->varkind != ARGVAR) {
					s2 = emit_load_s2(jd, iptr, src, REG_ITMP1);
#if defined(__DARWIN__)
					M_IST(s2, REG_SP, LA_SIZE + (s1 + INT_ARG_CNT) * 4);
#else
					M_IST(s2, REG_SP, LA_SIZE + (s1 + 3) * 4);
#endif
				}
			}

			/* a0 = dimension count */

			ICONST(rd->argintregs[0], iptr->op1);

			/* is patcher function set? */

			if (iptr->val.a == NULL) {
				disp = dseg_addaddress(cd, NULL);

				codegen_addpatchref(cd, PATCHER_builtin_multianewarray,
									(constant_classref *) iptr->target, disp);

				if (opt_showdisassemble)
					M_NOP;

			} else {
				disp = dseg_addaddress(cd, iptr->val.a);
			}

			/* a1 = arraydescriptor */

			M_ALD(rd->argintregs[1], REG_PV, disp);

			/* a2 = pointer to dimensions = stack pointer */

#if defined(__DARWIN__)
			M_LDA(rd->argintregs[2], REG_SP, LA_SIZE + INT_ARG_CNT * 4);
#else
			M_LDA(rd->argintregs[2], REG_SP, LA_SIZE + 3 * 4);
#endif

			disp = dseg_addaddress(cd, BUILTIN_multianewarray);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_MTCTR(REG_ITMP3);
			M_JSR;

			/* check for exception before result assignment */

			M_CMPI(REG_RESULT, 0);
			M_BEQ(0);
			codegen_add_fillinstacktrace_ref(cd);

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_RESULT);
			M_INTMOVE(REG_RESULT, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		default:
			*exceptionptr =
				new_internalerror("Unknown ICMD %d during code generation",
								  iptr->opc);
			return false;
	} /* switch */
		
	} /* for instruction */
		
	/* copy values to interface registers */

	src = bptr->outstack;
	len = bptr->outdepth;
	MCODECHECK(64 + len);
#if defined(ENABLE_LSRA)
	if (!opt_lsra)
#endif
	while (src) {
		len--;
		if ((src->varkind != STACKVAR)) {
			s2 = src->type;
			if (IS_FLT_DBL_TYPE(s2)) {
				s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
				if (!(rd->interfaces[len][s2].flags & INMEMORY))
					M_FLTMOVE(s1, rd->interfaces[len][s2].regoff);
				else
					M_DST(s1, REG_SP, rd->interfaces[len][s2].regoff * 4);

			} else {
				s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
				if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
					if (IS_2_WORD_TYPE(s2))
						M_LNGMOVE(s1, rd->interfaces[len][s2].regoff);
					else
						M_INTMOVE(s1, rd->interfaces[len][s2].regoff);

				} else {
					if (IS_2_WORD_TYPE(s2))
						M_LST(s1, REG_SP, rd->interfaces[len][s2].regoff * 4);
					else
						M_IST(s1, REG_SP, rd->interfaces[len][s2].regoff * 4);
				}
			}
		}
		src = src->prev;
	}
	} /* if (bptr -> flags >= BBREACHED) */
	} /* for basic block */

	dseg_createlinenumbertable(cd);


	/* generate exception and patcher stubs */

	{
		exceptionref *eref;
		patchref     *pref;
		u4            mcode;
		u1           *savedmcodeptr;
		u1           *tmpmcodeptr;

		savedmcodeptr = NULL;

		/* generate exception stubs */

		for (eref = cd->exceptionrefs; eref != NULL; eref = eref->next) {
			gen_resolvebranch(cd->mcodebase + eref->branchpos, 
							  eref->branchpos, cd->mcodeptr - cd->mcodebase);

			MCODECHECK(100);

			/* Move the value register to a temporary register, if
			   there is the need for it. */

			if (eref->reg != -1)
				M_MOV(eref->reg, REG_ITMP1);

			/* calcuate exception address */

			M_LDA(REG_ITMP2_XPC, REG_PV, eref->branchpos - 4);

			/* move function to call into REG_ITMP3 */

			disp = dseg_addaddress(cd, eref->function);
			M_ALD(REG_ITMP3, REG_PV, disp);

			if (savedmcodeptr != NULL) {
				disp = ((u4 *) savedmcodeptr) - (((u4 *) cd->mcodeptr) + 1);
				M_BR(disp);
			}
			else {
				savedmcodeptr = cd->mcodeptr;

				if (jd->isleafmethod) {
					M_MFLR(REG_ZERO);
					M_AST(REG_ZERO, REG_SP, stackframesize * 4 + LA_LR_OFFSET);
				}

				M_MOV(REG_PV, rd->argintregs[0]);
				M_MOV(REG_SP, rd->argintregs[1]);

				if (jd->isleafmethod)
					M_MOV(REG_ZERO, rd->argintregs[2]);
				else
					M_ALD(rd->argintregs[2],
						  REG_SP, stackframesize * 4 + LA_LR_OFFSET);

				M_MOV(REG_ITMP2_XPC, rd->argintregs[3]);
				M_MOV(REG_ITMP1, rd->argintregs[4]);

				M_STWU(REG_SP, REG_SP, -(LA_SIZE + 6 * 4));
				M_AST(REG_ITMP2_XPC, REG_SP, LA_SIZE + 5 * 4);

				M_MTCTR(REG_ITMP3);
				M_JSR;
				M_MOV(REG_RESULT, REG_ITMP1_XPTR);

				M_ALD(REG_ITMP2_XPC, REG_SP, LA_SIZE + 5 * 4);
				M_IADD_IMM(REG_SP, LA_SIZE + 6 * 4, REG_SP);

				if (jd->isleafmethod) {
					/* XXX FIXME: REG_ZERO can cause problems here! */
					assert(stackframesize * 4 <= 32767);

					M_ALD(REG_ZERO, REG_SP, stackframesize * 4 + LA_LR_OFFSET);
					M_MTLR(REG_ZERO);
				}

				disp = dseg_addaddress(cd, asm_handle_exception);
				M_ALD(REG_ITMP3, REG_PV, disp);
				M_MTCTR(REG_ITMP3);
				M_RTS;
			}
		}


		/* generate code patching stub call code */

		for (pref = cd->patchrefs; pref != NULL; pref = pref->next) {
			/* check code segment size */

			MCODECHECK(16);

			/* Get machine code which is patched back in later. The
			   call is 1 instruction word long. */

			tmpmcodeptr = (u1 *) (cd->mcodebase + pref->branchpos);

			mcode = *((u4 *) tmpmcodeptr);

			/* Patch in the call to call the following code (done at
			   compile time). */

			savedmcodeptr = cd->mcodeptr;   /* save current mcodeptr          */
			cd->mcodeptr  = tmpmcodeptr;    /* set mcodeptr to patch position */

			disp = ((u4 *) savedmcodeptr) - (((u4 *) tmpmcodeptr) + 1);
			M_BR(disp);

			cd->mcodeptr = savedmcodeptr;   /* restore the current mcodeptr   */

			/* create stack frame - keep stack 16-byte aligned */

			M_AADD_IMM(REG_SP, -8 * 4, REG_SP);

			/* calculate return address and move it onto the stack */

			M_LDA(REG_ITMP3, REG_PV, pref->branchpos);
			M_AST_INTERN(REG_ITMP3, REG_SP, 5 * 4);

			/* move pointer to java_objectheader onto stack */

#if defined(ENABLE_THREADS)
			/* order reversed because of data segment layout */

			(void) dseg_addaddress(cd, NULL);                         /* flcword    */
			(void) dseg_addaddress(cd, lock_get_initial_lock_word()); /* monitorPtr */
			disp = dseg_addaddress(cd, NULL);                         /* vftbl      */

			M_LDA(REG_ITMP3, REG_PV, disp);
			M_AST_INTERN(REG_ITMP3, REG_SP, 4 * 4);
#else
			/* do nothing */
#endif

			/* move machine code onto stack */

			disp = dseg_adds4(cd, mcode);
			M_ILD(REG_ITMP3, REG_PV, disp);
			M_IST_INTERN(REG_ITMP3, REG_SP, 3 * 4);

			/* move class/method/field reference onto stack */

			disp = dseg_addaddress(cd, pref->ref);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_AST_INTERN(REG_ITMP3, REG_SP, 2 * 4);

			/* move data segment displacement onto stack */

			disp = dseg_addaddress(cd, pref->disp);
			M_ILD(REG_ITMP3, REG_PV, disp);
			M_IST_INTERN(REG_ITMP3, REG_SP, 1 * 4);

			/* move patcher function pointer onto stack */

			disp = dseg_addaddress(cd, pref->patcher);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_AST_INTERN(REG_ITMP3, REG_SP, 0 * 4);

			disp = dseg_addaddress(cd, asm_wrapper_patcher);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_MTCTR(REG_ITMP3);
			M_RTS;
		}

		/* generate replacement-out stubs */

		{
			int i;

			replacementpoint = jd->code->rplpoints;

			for (i = 0; i < jd->code->rplpointcount; ++i, ++replacementpoint) {
				/* check code segment size */

				MCODECHECK(100);

				/* note start of stub code */

				replacementpoint->outcode = (u1 *) (cd->mcodeptr - cd->mcodebase);

				/* make machine code for patching */

				tmpmcodeptr  = cd->mcodeptr;
				cd->mcodeptr = (u1 *) &(replacementpoint->mcode) + 1 /* big-endian */;

				disp = (ptrint)((s4*)replacementpoint->outcode - (s4*)replacementpoint->pc) - 1;
				M_BR(disp);

				cd->mcodeptr = tmpmcodeptr;

				/* create stack frame - keep 16-byte aligned */

				M_AADD_IMM(REG_SP, -4 * 4, REG_SP);

				/* push address of `rplpoint` struct */

				disp = dseg_addaddress(cd, replacementpoint);
				M_ALD(REG_ITMP3, REG_PV, disp);
				M_AST_INTERN(REG_ITMP3, REG_SP, 0 * 4);

				/* jump to replacement function */

				disp = dseg_addaddress(cd, asm_replacement_out);
				M_ALD(REG_ITMP3, REG_PV, disp);
				M_MTCTR(REG_ITMP3);
				M_RTS;
			}
		}
	}

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

	M_ALD_INTERN(REG_ITMP1, REG_PV, -2 * SIZEOF_VOID_P);
	M_ALD_INTERN(REG_PV, REG_PV, -3 * SIZEOF_VOID_P);
	M_MTCTR(REG_PV);
	M_RTS;

	md_cacheflush((u1 *) d, COMPILERSTUB_SIZE);

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
	s4            stackframesize;       /* size of stackframe if needed       */
	methoddesc   *md;
	s4            nativeparams;
	s4            i, j;                 /* count variables                    */
	s4            t;
	s4            s1, s2, disp;
	s4            funcdisp;

	/* get required compiler data */

	m    = jd->m;
	code = jd->code;
	cd   = jd->cd;
	rd   = jd->rd;

	/* set some variables */

	md = m->parseddesc;
	nativeparams = (m->flags & ACC_STATIC) ? 2 : 1;

	/* calculate stackframe size */

	stackframesize =
		sizeof(stackframeinfo) / SIZEOF_VOID_P +
		sizeof(localref_table) / SIZEOF_VOID_P +
		4 +                             /* 4 stackframeinfo arguments (darwin)*/
		nmd->paramcount * 2 +           /* assume all arguments are doubles   */
		nmd->memuse;

	stackframesize = (stackframesize + 3) & ~3; /* keep stack 16-byte aligned */

	/* create method header */

	(void) dseg_addaddress(cd, code);                      /* CodeinfoPointer */
	(void) dseg_adds4(cd, stackframesize * 4);             /* FrameSize       */
	(void) dseg_adds4(cd, 0);                              /* IsSync          */
	(void) dseg_adds4(cd, 0);                              /* IsLeaf          */
	(void) dseg_adds4(cd, 0);                              /* IntSave         */
	(void) dseg_adds4(cd, 0);                              /* FltSave         */
	(void) dseg_addlinenumbertablesize(cd);
	(void) dseg_adds4(cd, 0);                              /* ExTableSize     */

	/* generate code */

	M_MFLR(REG_ZERO);
	M_AST_INTERN(REG_ZERO, REG_SP, LA_LR_OFFSET);
	M_STWU(REG_SP, REG_SP, -(stackframesize * 4));

	if (JITDATA_HAS_FLAG_VERBOSECALL(jd))
		/* parent_argbase == stackframesize * 4 */
		codegen_trace_args(jd, stackframesize * 4 , true);

	/* get function address (this must happen before the stackframeinfo) */

	funcdisp = dseg_addaddress(cd, f);

#if !defined(WITH_STATIC_CLASSPATH)
	if (f == NULL) {
		codegen_addpatchref(cd, PATCHER_resolve_native, m, funcdisp);

		if (opt_showdisassemble)
			M_NOP;
	}
#endif

	/* save integer and float argument registers */

	j = 0;

	for (i = 0; i < md->paramcount; i++) {
		t = md->paramtypes[i].type;

		if (IS_INT_LNG_TYPE(t)) {
			if (!md->params[i].inmemory) {
				s1 = md->params[i].regoff;
				if (IS_2_WORD_TYPE(t)) {
					M_IST(rd->argintregs[GET_HIGH_REG(s1)], REG_SP, LA_SIZE + 4 * 4 + j * 4);
					j++;
					M_IST(rd->argintregs[GET_LOW_REG(s1)], REG_SP, LA_SIZE + 4 * 4 + j * 4);
				} else {
					M_IST(rd->argintregs[s1], REG_SP, LA_SIZE + 4 * 4 + j * 4);
				}
				j++;
			}
		}
	}

	for (i = 0; i < md->paramcount; i++) {
		if (IS_FLT_DBL_TYPE(md->paramtypes[i].type)) {
			if (!md->params[i].inmemory) {
				s1 = md->params[i].regoff;
				M_DST(rd->argfltregs[s1], REG_SP, LA_SIZE + 4 * 4 + j * 8);
				j++;
			}
		}
	}

	/* create native stack info */

	M_AADD_IMM(REG_SP, stackframesize * 4, rd->argintregs[0]);
	M_MOV(REG_PV, rd->argintregs[1]);
	M_AADD_IMM(REG_SP, stackframesize * 4, rd->argintregs[2]);
	M_ALD(rd->argintregs[3], REG_SP, stackframesize * 4 + LA_LR_OFFSET);
	disp = dseg_addaddress(cd, codegen_start_native_call);
	M_ALD(REG_ITMP1, REG_PV, disp);
	M_MTCTR(REG_ITMP1);
	M_JSR;

	/* restore integer and float argument registers */

	j = 0;

	for (i = 0; i < md->paramcount; i++) {
		t = md->paramtypes[i].type;

		if (IS_INT_LNG_TYPE(t)) {
			if (!md->params[i].inmemory) {
				s1 = md->params[i].regoff;

				if (IS_2_WORD_TYPE(t)) {
					M_ILD(rd->argintregs[GET_HIGH_REG(s1)], REG_SP, LA_SIZE + 4 * 4 + j * 4);
					j++;
					M_ILD(rd->argintregs[GET_LOW_REG(s1)], REG_SP, LA_SIZE + 4 * 4 + j * 4);
				} else {
					M_ILD(rd->argintregs[s1], REG_SP, LA_SIZE + 4 * 4 + j * 4);
				}
				j++;
			}
		}
	}

	for (i = 0; i < md->paramcount; i++) {
		if (IS_FLT_DBL_TYPE(md->paramtypes[i].type)) {
			if (!md->params[i].inmemory) {
				s1 = md->params[i].regoff;
				M_DLD(rd->argfltregs[s1], REG_SP, LA_SIZE + 4 * 4 + j * 8);
				j++;
			}
		}
	}
	
	/* copy or spill arguments to new locations */

	for (i = md->paramcount - 1, j = i + nativeparams; i >= 0; i--, j--) {
		t = md->paramtypes[i].type;

		if (IS_INT_LNG_TYPE(t)) {
			if (!md->params[i].inmemory) {
				if (IS_2_WORD_TYPE(t))
					s1 = PACK_REGS(
						rd->argintregs[GET_LOW_REG(md->params[i].regoff)],
					    rd->argintregs[GET_HIGH_REG(md->params[i].regoff)]);
				else
					s1 = rd->argintregs[md->params[i].regoff];

				if (!nmd->params[j].inmemory) {
					if (IS_2_WORD_TYPE(t)) {
						s2 = PACK_REGS(
						   rd->argintregs[GET_LOW_REG(nmd->params[j].regoff)],
						   rd->argintregs[GET_HIGH_REG(nmd->params[j].regoff)]);
						M_LNGMOVE(s1, s2);
					} else {
						s2 = rd->argintregs[nmd->params[j].regoff];
						M_INTMOVE(s1, s2);
					}

				} else {
					s2 = nmd->params[j].regoff;
					if (IS_2_WORD_TYPE(t))
						M_LST(s1, REG_SP, s2 * 4);
					else
						M_IST(s1, REG_SP, s2 * 4);
				}

			} else {
				s1 = md->params[i].regoff + stackframesize;
				s2 = nmd->params[j].regoff;

				M_ILD(REG_ITMP1, REG_SP, s1 * 4);
				if (IS_2_WORD_TYPE(t))
					M_ILD(REG_ITMP2, REG_SP, s1 * 4 + 4);

				M_IST(REG_ITMP1, REG_SP, s2 * 4);
				if (IS_2_WORD_TYPE(t))
					M_IST(REG_ITMP2, REG_SP, s2 * 4 + 4);
			}

		} else {
			/* We only copy spilled float arguments, as the float
			   argument registers keep unchanged. */

			if (md->params[i].inmemory) {
				s1 = md->params[i].regoff + stackframesize;
				s2 = nmd->params[j].regoff;

				if (IS_2_WORD_TYPE(t)) {
					M_DLD(REG_FTMP1, REG_SP, s1 * 4);
					M_DST(REG_FTMP1, REG_SP, s2 * 4);

				} else {
					M_FLD(REG_FTMP1, REG_SP, s1 * 4);
					M_FST(REG_FTMP1, REG_SP, s2 * 4);
				}
			}
		}
	}

	/* put class into second argument register */

	if (m->flags & ACC_STATIC) {
		disp = dseg_addaddress(cd, m->class);
		M_ALD(rd->argintregs[1], REG_PV, disp);
	}

	/* put env into first argument register */

	disp = dseg_addaddress(cd, _Jv_env);
	M_ALD(rd->argintregs[0], REG_PV, disp);

	/* generate the actual native call */

	M_ALD(REG_ITMP3, REG_PV, funcdisp);
	M_MTCTR(REG_ITMP3);
	M_JSR;

	/* save return value */

	if (md->returntype.type != TYPE_VOID) {
		if (IS_INT_LNG_TYPE(md->returntype.type)) {
			if (IS_2_WORD_TYPE(md->returntype.type))
				M_IST(REG_RESULT2, REG_SP, LA_SIZE + 2 * 4);
			M_IST(REG_RESULT, REG_SP, LA_SIZE + 1 * 4);
		}
		else {
			if (IS_2_WORD_TYPE(md->returntype.type))
				M_DST(REG_FRESULT, REG_SP, LA_SIZE + 1 * 4);
			else
				M_FST(REG_FRESULT, REG_SP, LA_SIZE + 1 * 4);
		}
	}

	/* print call trace */

	if (JITDATA_HAS_FLAG_VERBOSECALL(jd)) {
		 /* just restore the value we need, don't care about the other */

		if (md->returntype.type != TYPE_VOID) {
			if (IS_INT_LNG_TYPE(md->returntype.type)) {
				if (IS_2_WORD_TYPE(md->returntype.type))
					M_ILD(REG_RESULT2, REG_SP, LA_SIZE + 2 * 4);
				M_ILD(REG_RESULT, REG_SP, LA_SIZE + 1 * 4);
			}
			else {
				if (IS_2_WORD_TYPE(md->returntype.type))
					M_DLD(REG_FRESULT, REG_SP, LA_SIZE + 1 * 4);
				else
					M_FLD(REG_FRESULT, REG_SP, LA_SIZE + 1 * 4);
			}
		}

		M_LDA(REG_SP, REG_SP, -(LA_SIZE + (1 + 2 + 2 + 1) * 4));

		/* keep this order */
		switch (md->returntype.type) {
		case TYPE_INT:
		case TYPE_ADR:
#if defined(__DARWIN__)
			M_MOV(REG_RESULT, rd->argintregs[2]);
			M_CLR(rd->argintregs[1]);
#else
			M_MOV(REG_RESULT, rd->argintregs[3]);
			M_CLR(rd->argintregs[2]);
#endif
			break;

		case TYPE_LNG:
#if defined(__DARWIN__)
			M_MOV(REG_RESULT2, rd->argintregs[2]);
			M_MOV(REG_RESULT, rd->argintregs[1]);
#else
			M_MOV(REG_RESULT2, rd->argintregs[3]);
			M_MOV(REG_RESULT, rd->argintregs[2]);
#endif
			break;
		}

		M_FLTMOVE(REG_FRESULT, rd->argfltregs[0]);
		M_FLTMOVE(REG_FRESULT, rd->argfltregs[1]);
		disp = dseg_addaddress(cd, m);
		M_ALD(rd->argintregs[0], REG_PV, disp);

		disp = dseg_addaddress(cd, builtin_displaymethodstop);
		M_ALD(REG_ITMP2, REG_PV, disp);
		M_MTCTR(REG_ITMP2);
		M_JSR;

		M_LDA(REG_SP, REG_SP, LA_SIZE + (1 + 2 + 2 + 1) * 4);
	}

	/* remove native stackframe info */

	M_AADD_IMM(REG_SP, stackframesize * 4, rd->argintregs[0]);
	disp = dseg_addaddress(cd, codegen_finish_native_call);
	M_ALD(REG_ITMP1, REG_PV, disp);
	M_MTCTR(REG_ITMP1);
	M_JSR;
	M_MOV(REG_RESULT, REG_ITMP1_XPTR);

	/* restore return value */

	if (md->returntype.type != TYPE_VOID) {
		if (IS_INT_LNG_TYPE(md->returntype.type)) {
			if (IS_2_WORD_TYPE(md->returntype.type))
				M_ILD(REG_RESULT2, REG_SP, LA_SIZE + 2 * 4);
			M_ILD(REG_RESULT, REG_SP, LA_SIZE + 1 * 4);
		}
		else {
			if (IS_2_WORD_TYPE(md->returntype.type))
				M_DLD(REG_FRESULT, REG_SP, LA_SIZE + 1 * 4);
			else
				M_FLD(REG_FRESULT, REG_SP, LA_SIZE + 1 * 4);
		}
	}

	M_ALD(REG_ITMP2_XPC, REG_SP, stackframesize * 4 + LA_LR_OFFSET);
	M_MTLR(REG_ITMP2_XPC);
	M_LDA(REG_SP, REG_SP, stackframesize * 4); /* remove stackframe           */

	/* check for exception */

	M_TST(REG_ITMP1_XPTR);
	M_BNE(1);                           /* if no exception then return        */

	M_RET;

	/* handle exception */

	M_IADD_IMM(REG_ITMP2_XPC, -4, REG_ITMP2_XPC);  /* exception address       */

	disp = dseg_addaddress(cd, asm_handle_nat_exception);
	M_ALD(REG_ITMP3, REG_PV, disp);
	M_MTCTR(REG_ITMP3);
	M_RTS;

	/* generate patcher stub call code */

	{
		patchref *pref;
		u4        mcode;
		u1       *savedmcodeptr;
		u1       *tmpmcodeptr;

		for (pref = cd->patchrefs; pref != NULL; pref = pref->next) {
			/* Get machine code which is patched back in later. The
			   call is 1 instruction word long. */

			tmpmcodeptr = cd->mcodebase + pref->branchpos;

			mcode = *((u4 *) tmpmcodeptr);

			/* Patch in the call to call the following code (done at
			   compile time). */

			savedmcodeptr = cd->mcodeptr;   /* save current mcodeptr          */
			cd->mcodeptr  = tmpmcodeptr;    /* set mcodeptr to patch position */

			disp = ((u4 *) savedmcodeptr) - (((u4 *) tmpmcodeptr) + 1);
			M_BL(disp);

			cd->mcodeptr = savedmcodeptr;   /* restore the current mcodeptr   */

			/* create stack frame - keep stack 16-byte aligned */

			M_AADD_IMM(REG_SP, -8 * 4, REG_SP);

			/* move return address onto stack */

			M_MFLR(REG_ZERO);
			M_AST(REG_ZERO, REG_SP, 5 * 4);

			/* move pointer to java_objectheader onto stack */

#if defined(ENABLE_THREADS)
			/* order reversed because of data segment layout */

			(void) dseg_addaddress(cd, NULL);                         /* flcword    */
			(void) dseg_addaddress(cd, lock_get_initial_lock_word()); /* monitorPtr */
			disp = dseg_addaddress(cd, NULL);                         /* vftbl      */

			M_LDA(REG_ITMP3, REG_PV, disp);
			M_AST(REG_ITMP3, REG_SP, 4 * 4);
#else
			/* do nothing */
#endif

			/* move machine code onto stack */

			disp = dseg_adds4(cd, mcode);
			M_ILD(REG_ITMP3, REG_PV, disp);
			M_IST(REG_ITMP3, REG_SP, 3 * 4);

			/* move class/method/field reference onto stack */

			disp = dseg_addaddress(cd, pref->ref);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_AST(REG_ITMP3, REG_SP, 2 * 4);

			/* move data segment displacement onto stack */

			disp = dseg_addaddress(cd, pref->disp);
			M_ILD(REG_ITMP3, REG_PV, disp);
			M_IST(REG_ITMP3, REG_SP, 1 * 4);

			/* move patcher function pointer onto stack */

			disp = dseg_addaddress(cd, pref->patcher);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_AST(REG_ITMP3, REG_SP, 0 * 4);

			disp = dseg_addaddress(cd, asm_wrapper_patcher);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_MTCTR(REG_ITMP3);
			M_RTS;
		}
	}

	codegen_finish(jd);

	return jd->code->entrypoint;
}


void codegen_trace_args(jitdata *jd, s4 stackframesize, bool nativestub)
{
	methodinfo   *m;
	codegendata  *cd;
	registerdata *rd;
	s4 s1, p, t, d;
	int stack_off;
	int stack_size;
	methoddesc *md;

	/* get required compiler data */

	m  = jd->m;
	cd = jd->cd;
	rd = jd->rd;

	md = m->parseddesc;
	
	if (!nativestub)
		M_MFLR(REG_ITMP3);
	/* Build up Stackframe for builtin_trace_args call (a multiple of 16) */
	/* For Darwin:                                                        */
	/* LA + TRACE_ARGS_NUM u8 args + methodinfo + LR                      */
	/* LA_SIZE(=6*4) + 8*8         + 4          + 4  + 0(Padding)         */
	/* 6 * 4 + 8 * 8 + 2 * 4 = 12 * 8 = 6 * 16                            */
	/* For Linux:                                                         */
	/* LA + (TRACE_ARGS_NUM - INT_ARG_CNT/2) u8 args + methodinfo         */
	/* + INT_ARG_CNT * 4 ( save integer registers) + LR + 8 + 8 (Padding) */
	/* LA_SIZE(=2*4) + 4 * 8 + 4 + 8 * 4 + 4 + 8                          */
	/* 2 * 4 + 4 * 8 + 10 * 4 + 1 * 8 + 8= 12 * 8 = 6 * 16                */
	
	/* in nativestubs no Place to save the LR (Link Register) would be needed */
	/* but since the stack frame has to be aligned the 4 Bytes would have to  */
	/* be padded again */

#if defined(__DARWIN__)
	stack_size = LA_SIZE + (TRACE_ARGS_NUM + 1) * 8;
#else
	stack_size = 6 * 16;
#endif
	M_LDA(REG_SP, REG_SP, -stack_size);

	/* Save LR */
	if (!nativestub)
		M_IST(REG_ITMP3, REG_SP, LA_SIZE + TRACE_ARGS_NUM * 8 + 1 * 4);

	M_CLR(REG_ITMP1);    /* clear help register */

	/* save up to TRACE_ARGS_NUM arguments into the reserved stack space */
#if defined(__DARWIN__)
	/* Copy Params starting from first to Stack                          */
	/* since TRACE_ARGS == INT_ARG_CNT all used integer argument regs    */ 
	/* are saved                                                         */
	p = 0;
#else
	/* Copy Params starting from fifth to Stack (INT_ARG_CNT/2) are in   */
	/* integer argument regs                                             */
	/* all integer argument registers have to be saved                   */
	for (p = 0; p < 8; p++) {
		d = rd->argintregs[p];
		/* save integer argument registers */
		M_IST(d, REG_SP, LA_SIZE + 4 * 8 + 4 + p * 4);
	}
	p = 4;
#endif
	stack_off = LA_SIZE;
	for (; p < md->paramcount && p < TRACE_ARGS_NUM; p++, stack_off += 8) {
		t = md->paramtypes[p].type;
		if (IS_INT_LNG_TYPE(t)) {
			if (!md->params[p].inmemory) { /* Param in Arg Reg */
				if (IS_2_WORD_TYPE(t)) {
					M_IST(rd->argintregs[GET_HIGH_REG(md->params[p].regoff)]
						  , REG_SP, stack_off);
					M_IST(rd->argintregs[GET_LOW_REG(md->params[p].regoff)]
						  , REG_SP, stack_off + 4);
				} else {
					M_IST(REG_ITMP1, REG_SP, stack_off);
					M_IST(rd->argintregs[md->params[p].regoff]
						  , REG_SP, stack_off + 4);
				}
			} else { /* Param on Stack */
				s1 = (md->params[p].regoff + stackframesize) * 4 
					+ stack_size;
				if (IS_2_WORD_TYPE(t)) {
					M_ILD(REG_ITMP2, REG_SP, s1);
					M_IST(REG_ITMP2, REG_SP, stack_off);
					M_ILD(REG_ITMP2, REG_SP, s1 + 4);
					M_IST(REG_ITMP2, REG_SP, stack_off + 4);
				} else {
					M_IST(REG_ITMP1, REG_SP, stack_off);
					M_ILD(REG_ITMP2, REG_SP, s1);
					M_IST(REG_ITMP2, REG_SP, stack_off + 4);
				}
			}
		} else { /* IS_FLT_DBL_TYPE(t) */
			if (!md->params[p].inmemory) { /* in Arg Reg */
				s1 = rd->argfltregs[md->params[p].regoff];
				if (!IS_2_WORD_TYPE(t)) {
					M_IST(REG_ITMP1, REG_SP, stack_off);
					M_FST(s1, REG_SP, stack_off + 4);
				} else {
					M_DST(s1, REG_SP, stack_off);
				}
			} else { /* on Stack */
				/* this should not happen */
			}
		}
	}

	/* load first 4 (==INT_ARG_CNT/2) arguments into integer registers */
#if defined(__DARWIN__)
	for (p = 0; p < 8; p++) {
		d = rd->argintregs[p];
		M_ILD(d, REG_SP, LA_SIZE + p * 4);
	}
#else
	/* LINUX */
	/* Set integer and float argument registers vor trace_args call */
	/* offset to saved integer argument registers                   */
	stack_off = LA_SIZE + 4 * 8 + 4;
	for (p = 0; (p < 4) && (p < md->paramcount); p++) {
		t = md->paramtypes[p].type;
		if (IS_INT_LNG_TYPE(t)) {
			/* "stretch" int types */
			if (!IS_2_WORD_TYPE(t)) {
				M_CLR(rd->argintregs[2 * p]);
				M_ILD(rd->argintregs[2 * p + 1], REG_SP,stack_off);
				stack_off += 4;
			} else {
				M_ILD(rd->argintregs[2 * p + 1], REG_SP,stack_off + 4);
				M_ILD(rd->argintregs[2 * p], REG_SP,stack_off);
				stack_off += 8;
			}
		} else { /* Float/Dbl */
			if (!md->params[p].inmemory) { /* Param in Arg Reg */
				/* use reserved Place on Stack (sp + 5 * 16) to copy  */
				/* float/double arg reg to int reg                    */
				s1 = rd->argfltregs[md->params[p].regoff];
				if (!IS_2_WORD_TYPE(t)) {
					M_FST(s1, REG_SP, 5 * 16);
					M_ILD(rd->argintregs[2 * p + 1], REG_SP, 5 * 16);
					M_CLR(rd->argintregs[2 * p]);
				} else {
					M_DST(s1, REG_SP, 5 * 16);
					M_ILD(rd->argintregs[2 * p + 1], REG_SP,  5 * 16 + 4);
					M_ILD(rd->argintregs[2 * p], REG_SP, 5 * 16);
				}
			}
		}
	}
#endif

	/* put methodinfo pointer on Stackframe */
	p = dseg_addaddress(cd, m);
	M_ALD(REG_ITMP1, REG_PV, p);
#if defined(__DARWIN__)
	M_AST(REG_ITMP1, REG_SP, LA_SIZE + TRACE_ARGS_NUM * 8); 
#else
	M_AST(REG_ITMP1, REG_SP, LA_SIZE + 4 * 8);
#endif
	p = dseg_addaddress(cd, builtin_trace_args);
	M_ALD(REG_ITMP2, REG_PV, p);
	M_MTCTR(REG_ITMP2);
	M_JSR;

#if defined(__DARWIN__)
	/* restore integer argument registers from the reserved stack space */

	stack_off = LA_SIZE;
	for (p = 0; p < md->paramcount && p < TRACE_ARGS_NUM; 
		 p++, stack_off += 8) {
		t = md->paramtypes[p].type;

		if (IS_INT_LNG_TYPE(t)) {
			if (!md->params[p].inmemory) {
				if (IS_2_WORD_TYPE(t)) {
					M_ILD(rd->argintregs[GET_HIGH_REG(md->params[p].regoff)]
						  , REG_SP, stack_off);
					M_ILD(rd->argintregs[GET_LOW_REG(md->params[p].regoff)]
						  , REG_SP, stack_off + 4);
				} else {
					M_ILD(rd->argintregs[md->params[p].regoff]
						  , REG_SP, stack_off + 4);
				}
			}
		}
	}
#else
	/* LINUX */
	for (p = 0; p < 8; p++) {
		d = rd->argintregs[p];
		/* save integer argument registers */
		M_ILD(d, REG_SP, LA_SIZE + 4 * 8 + 4 + p * 4);
	}
#endif

	if (!nativestub)
		M_ILD(REG_ITMP3, REG_SP, LA_SIZE + TRACE_ARGS_NUM * 8 + 1 * 4);

	M_LDA(REG_SP, REG_SP, stack_size);

	if (!nativestub)
		M_MTLR(REG_ITMP3);
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
