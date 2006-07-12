/* src/vm/jit/x86_64/codegen.c - machine code generator for x86_64

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
            Christian Thalinger

   Changes: Christian Ullrich
            Edwin Steiner

   $Id: codegen.c 5123 2006-07-12 21:45:34Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <stdio.h>

#include "vm/types.h"

#include "md.h"
#include "md-abi.h"

#include "vm/jit/x86_64/arch.h"
#include "vm/jit/x86_64/codegen.h"
#include "vm/jit/x86_64/md-emit.h"

#include "mm/memory.h"
#include "native/jni.h"
#include "native/native.h"

#if defined(ENABLE_THREADS)
# include "threads/native/lock.h"
#endif

#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/statistics.h"
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
	u2                  currentline;
	ptrint              a;
	s4                  stackframesize;
	stackptr            src;
	varinfo            *var;
	basicblock         *bptr;
	instruction        *iptr;
	exceptiontable     *ex;
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
	savedregs_num += (FLT_SAV_CNT - rd->savfltreguse);

	stackframesize = rd->memuse + savedregs_num;

#if defined(ENABLE_THREADS)
	/* space to save argument of monitor_enter */

	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		stackframesize++;
#endif

	/* Keep stack of non-leaf functions 16-byte aligned for calls into
	   native code e.g. libc or jni (alignment problems with
	   movaps). */

	if (!jd->isleafmethod || opt_verbosecall)
		stackframesize |= 0x1;

	/* create method header */

	(void) dseg_addaddress(cd, code);                      /* CodeinfoPointer */
	(void) dseg_adds4(cd, stackframesize * 8);             /* FrameSize       */

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

	(void) dseg_addlinenumbertablesize(cd);

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

		M_MOV_IMM(code, REG_ITMP3);
		M_IINC_MEMBASE(REG_ITMP3, OFFSET(codeinfo, frequency));

		PROFILE_CYCLE_START;
	}

	/* create stack frame (if necessary) */

	if (stackframesize)
		M_ASUB_IMM(stackframesize * 8, REG_SP);

	/* save used callee saved registers */

  	p = stackframesize;
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
			s2 = rd->argintregs[s1];
 			if (!md->params[p].inmemory) {           /* register arguments    */
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
   					M_INTMOVE(s2, var->regoff);

				} else {                             /* reg arg -> spilled    */
   				    M_LST(s2, REG_SP, var->regoff * 8);
 				}

			} else {                                 /* stack arguments       */
 				if (!(var->flags & INMEMORY)) {      /* stack arg -> register */
					/* + 8 for return address */
 					M_LLD(var->regoff, REG_SP, (stackframesize + s1) * 8 + 8);

				} else {                             /* stack arg -> spilled  */
					var->regoff = stackframesize + s1 + 1;
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
					M_DLD(var->regoff, REG_SP, (stackframesize + s1) * 8 + 8);

				} else {
					var->regoff = stackframesize + s1 + 1;
				}
			}
		}
	}  /* end for */

	/* save monitorenter argument */

#if defined(ENABLE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		/* stack offset for monitor argument */

		s1 = rd->memuse;

		if (opt_verbosecall) {
			M_LSUB_IMM((INT_ARG_CNT + FLT_ARG_CNT) * 8, REG_SP);

			for (p = 0; p < INT_ARG_CNT; p++)
				M_LST(rd->argintregs[p], REG_SP, p * 8);

			for (p = 0; p < FLT_ARG_CNT; p++)
				M_DST(rd->argfltregs[p], REG_SP, (INT_ARG_CNT + p) * 8);

			s1 += INT_ARG_CNT + FLT_ARG_CNT;
		}

		/* decide which monitor enter function to call */

		if (m->flags & ACC_STATIC) {
			M_MOV_IMM(&m->class->object.header, rd->argintregs[0]);
		}
		else {
			M_TEST(rd->argintregs[0]);
			M_BEQ(0);
			codegen_add_nullpointerexception_ref(cd);
		}

		M_AST(rd->argintregs[0], REG_SP, s1 * 8);
		M_MOV_IMM(LOCK_monitor_enter, REG_ITMP1);
		M_CALL(REG_ITMP1);

		if (opt_verbosecall) {
			for (p = 0; p < INT_ARG_CNT; p++)
				M_LLD(rd->argintregs[p], REG_SP, p * 8);

			for (p = 0; p < FLT_ARG_CNT; p++)
				M_DLD(rd->argfltregs[p], REG_SP, (INT_ARG_CNT + p) * 8);

			M_LADD_IMM((INT_ARG_CNT + FLT_ARG_CNT) * 8, REG_SP);
		}
	}
#endif

#if !defined(NDEBUG)
	/* Copy argument registers to stack and call trace function with
	   pointer to arguments on stack. */

	if (opt_verbosecall) {
		M_LSUB_IMM((INT_ARG_CNT + FLT_ARG_CNT + INT_TMP_CNT + FLT_TMP_CNT + 1 + 1) * 8, REG_SP);

		/* save integer argument registers */

		for (p = 0; p < INT_ARG_CNT; p++)
			M_LST(rd->argintregs[p], REG_SP, (1 + p) * 8);

		/* save float argument registers */

		for (p = 0; p < FLT_ARG_CNT; p++)
			M_DST(rd->argfltregs[p], REG_SP, (1 + INT_ARG_CNT + p) * 8);

		/* save temporary registers for leaf methods */

		if (jd->isleafmethod) {
			for (p = 0; p < INT_TMP_CNT; p++)
				M_LST(rd->tmpintregs[p], REG_SP, (1 + INT_ARG_CNT + FLT_ARG_CNT + p) * 8);

			for (p = 0; p < FLT_TMP_CNT; p++)
				M_DST(rd->tmpfltregs[p], REG_SP, (1 + INT_ARG_CNT + FLT_ARG_CNT + INT_TMP_CNT + p) * 8);
		}

		/* show integer hex code for float arguments */

		for (p = 0, l = 0; p < md->paramcount && p < INT_ARG_CNT; p++) {
			/* If the paramtype is a float, we have to right shift all
			   following integer registers. */
	
			if (IS_FLT_DBL_TYPE(md->paramtypes[p].type)) {
				for (s1 = INT_ARG_CNT - 2; s1 >= p; s1--)
					M_MOV(rd->argintregs[s1], rd->argintregs[s1 + 1]);

				emit_movd_freg_reg(cd, rd->argfltregs[l], rd->argintregs[p]);
				l++;
			}
		}

		M_MOV_IMM(m, REG_ITMP2);
		M_AST(REG_ITMP2, REG_SP, 0 * 8);
		M_MOV_IMM(builtin_trace_args, REG_ITMP1);
		M_CALL(REG_ITMP1);

		/* restore integer argument registers */

		for (p = 0; p < INT_ARG_CNT; p++)
			M_LLD(rd->argintregs[p], REG_SP, (1 + p) * 8);

		/* restore float argument registers */

		for (p = 0; p < FLT_ARG_CNT; p++)
			M_DLD(rd->argfltregs[p], REG_SP, (1 + INT_ARG_CNT + p) * 8);

		/* restore temporary registers for leaf methods */

		if (jd->isleafmethod) {
			for (p = 0; p < INT_TMP_CNT; p++)
				M_LLD(rd->tmpintregs[p], REG_SP, (1 + INT_ARG_CNT + FLT_ARG_CNT + p) * 8);

			for (p = 0; p < FLT_TMP_CNT; p++)
				M_DLD(rd->tmpfltregs[p], REG_SP, (1 + INT_ARG_CNT + FLT_ARG_CNT + INT_TMP_CNT + p) * 8);
		}

		M_LADD_IMM((INT_ARG_CNT + FLT_ARG_CNT + INT_TMP_CNT + FLT_TMP_CNT + 1 + 1) * 8, REG_SP);
	}
#endif /* !defined(NDEBUG) */

	}

	/* end of header generation */

	replacementpoint = jd->code->rplpoints;

	/* walk through all basic blocks */

	for (bptr = m->basicblocks; bptr != NULL; bptr = bptr->next) {

		bptr->mpc = (u4) ((u1 *) cd->mcodeptr - cd->mcodebase);

		if (bptr->flags >= BBREACHED) {

			/* branch resolving */

			branchref *bref;
			for (bref = bptr->branchrefs; bref != NULL; bref = bref->next) {
				gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
								  bref->branchpos,
								  bptr->mpc);
			}

		/* handle replacement points */

		if (bptr->bitflags & BBFLAG_REPLACEMENT) {
			replacementpoint->pc = (u1*)(ptrint)bptr->mpc; /* will be resolved later */
			
			replacementpoint++;

			assert(cd->lastmcodeptr <= cd->mcodeptr);
			cd->lastmcodeptr = cd->mcodeptr + 5; /* 5 byte jmp patch */
		}

		/* copy interface registers to their destination */

		src = bptr->instack;
		len = bptr->indepth;
		MCODECHECK(512);

#if 0
		/* generate basicblock profiling code */

		if (JITDATA_HAS_FLAG_INSTRUMENT(jd)) {
			/* count frequency */

			M_MOV_IMM(code->bbfrequency, REG_ITMP3);
			M_IINC_MEMBASE(REG_ITMP3, bptr->debug_nr * 4);

			/* if this is an exception handler, start profiling again */

			if (bptr->type == BBTYPE_EXH)
				PROFILE_CYCLE_START;
		}
#endif

#if defined(ENABLE_LSRA)
		if (opt_lsra) {
			while (src != NULL) {
				len--;
				if ((len == 0) && (bptr->type != BBTYPE_STD)) {
					if (bptr->type == BBTYPE_SBR) {
/*  					d = reg_of_var(rd, src, REG_ITMP1); */
						if (!(src->flags & INMEMORY))
							d = src->regoff;
						else
							d = REG_ITMP1;
						M_POP(d);
						emit_store(jd, NULL, src, d);

					} else if (bptr->type == BBTYPE_EXH) {
/*  					d = reg_of_var(rd, src, REG_ITMP1); */
						if (!(src->flags & INMEMORY))
							d= src->regoff;
						else
							d=REG_ITMP1;
						M_INTMOVE(REG_ITMP1, d);
						emit_store(jd, NULL, src, d);
					}
				}
				src = src->prev;
			}
			
		} else {
#endif

		while (src != NULL) {
			len--;
  			if ((len == 0) && (bptr->type != BBTYPE_STD)) {
				if (bptr->type == BBTYPE_SBR) {
					d = codegen_reg_of_var(rd, 0, src, REG_ITMP1);
					M_POP(d);
					emit_store(jd, NULL, src, d);

				} else if (bptr->type == BBTYPE_EXH) {
					d = codegen_reg_of_var(rd, 0, src, REG_ITMP1);
					M_INTMOVE(REG_ITMP1, d);
					emit_store(jd, NULL, src, d);
				}

			} else {
				d = codegen_reg_of_var(rd, 0, src, REG_ITMP1);
				if ((src->varkind != STACKVAR)) {
					s2 = src->type;
					if (IS_FLT_DBL_TYPE(s2)) {
						s1 = rd->interfaces[len][s2].regoff;

						if (!(rd->interfaces[len][s2].flags & INMEMORY))
							M_FLTMOVE(s1, d);
						else
							M_DLD(d, REG_SP, s1 * 8);

						emit_store(jd, NULL, src, d);

					} else {
						s1 = rd->interfaces[len][s2].regoff;

						if (!(rd->interfaces[len][s2].flags & INMEMORY))
							M_INTMOVE(s1, d);
						else
							M_LLD(d, REG_SP, s1 * 8);

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

			MCODECHECK(1024);                         /* 1KB should be enough */

			switch (iptr->opc) {
			case ICMD_INLINE_START: /* internal ICMDs                         */
			case ICMD_INLINE_END:
				break;

			case ICMD_NOP:    /* ...  ==> ...                                 */
				break;

			case ICMD_CHECKNULL: /* ..., objectref  ==> ..., objectref        */

				s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
				M_TEST(s1);
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

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			LCONST(d, iptr->val.l);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_FCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.f = constant                    */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
			disp = dseg_addfloat(cd, iptr->val.f);
			emit_movdl_membase_reg(cd, RIP, -(((s8) cd->mcodeptr + ((d > 7) ? 9 : 8)) - (s8) cd->mcodebase) + disp, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;
		
		case ICMD_DCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.d = constant                    */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
			disp = dseg_adddouble(cd, iptr->val.d);
			emit_movd_membase_reg(cd, RIP, -(((s8) cd->mcodeptr + 9) - (s8) cd->mcodebase) + disp, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_ACONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.a = constant                    */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
/* 				PROFILE_CYCLE_STOP; */

				codegen_addpatchref(cd, PATCHER_aconst,
									ICMD_ACONST_UNRESOLVED_CLASSREF(iptr), 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

/* 				PROFILE_CYCLE_START; */

				M_MOV_IMM(NULL, d);

			} else {
				if (iptr->val.a == 0)
					M_CLR(d);
				else
					M_MOV_IMM(iptr->val.a, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;


		/* load/store operations **********************************************/

		case ICMD_ILOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
				(iptr->dst->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY)
				M_ILD(d, REG_SP, var->regoff * 8);
			else
				M_INTMOVE(var->regoff, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LLOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_ALOAD:      /* op1 = local variable                         */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
				(iptr->dst->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY)
				M_LLD(d, REG_SP, var->regoff * 8);
			else
				M_INTMOVE(var->regoff, d);
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
				M_FLD(d, REG_SP, var->regoff * 8);
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
				M_DLD(d, REG_SP, var->regoff * 8);
			else
				M_FLTMOVE(var->regoff, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_ISTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			if ((src->varkind == LOCALVAR) && (src->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
				M_IST(s1, REG_SP, var->regoff * 8);
			} else {
				s1 = emit_load_s1(jd, iptr, src, var->regoff);
				M_INTMOVE(s1, var->regoff);
			}
			break;

		case ICMD_LSTORE:     /* ..., value  ==> ...                          */
		case ICMD_ASTORE:     /* op1 = local variable                         */

			if ((src->varkind == LOCALVAR) && (src->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
				M_LST(s1, REG_SP, var->regoff * 8);
			} else {
				s1 = emit_load_s1(jd, iptr, src, var->regoff);
				M_INTMOVE(s1, var->regoff);
			}
			break;

		case ICMD_FSTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			if ((src->varkind == LOCALVAR) &&
			    (src->varnum == iptr->op1)) {
				break;
			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
				M_FST(s1, REG_SP, var->regoff * 8);
			} else {
				s1 = emit_load_s1(jd, iptr, src, var->regoff);
  				M_FLTMOVE(s1, var->regoff);
			}
			break;

		case ICMD_DSTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			if ((src->varkind == LOCALVAR) &&
			    (src->varnum == iptr->op1)) {
				break;
			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
				M_DST(s1, REG_SP, var->regoff * 8);
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
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			M_INTMOVE(s1, d);
			M_INEG(d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LNEG:       /* ..., value  ==> ..., - value                 */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1); 
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			M_INTMOVE(s1, d);
			M_LNEG(d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_I2L:        /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			M_ISEXT(s1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			M_IMOV(s1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			M_BSEXT(s1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_INT2CHAR:   /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			M_CZEXT(s1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_INT2SHORT:  /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			M_SSEXT(s1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;


		case ICMD_IADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if (s2 == d)
				M_IADD(s1, d);
			else {
				M_INTMOVE(s1, d);
				M_IADD(s2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			M_INTMOVE(s1, d);
			M_IADD_IMM(iptr->val.i, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if (s2 == d)
				M_LADD(s1, d);
			else {
				M_INTMOVE(s1, d);
				M_LADD(s2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			M_INTMOVE(s1, d);
			if (IS_IMM32(iptr->val.l))
				M_LADD_IMM(iptr->val.l, d);
			else {
				M_MOV_IMM(iptr->val.l, REG_ITMP2);
				M_LADD(REG_ITMP2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_ISUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if (s2 == d) {
				M_INTMOVE(s1, REG_ITMP1);
				M_ISUB(s2, REG_ITMP1);
				M_INTMOVE(REG_ITMP1, d);
			} else {
				M_INTMOVE(s1, d);
				M_ISUB(s2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			M_INTMOVE(s1, d);
			M_ISUB_IMM(iptr->val.i, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if (s2 == d) {
				M_INTMOVE(s1, REG_ITMP1);
				M_LSUB(s2, REG_ITMP1);
				M_INTMOVE(REG_ITMP1, d);
			} else {
				M_INTMOVE(s1, d);
				M_LSUB(s2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			M_INTMOVE(s1, d);
			if (IS_IMM32(iptr->val.l))
				M_LSUB_IMM(iptr->val.l, d);
			else {
				M_MOV_IMM(iptr->val.l, REG_ITMP2);
				M_LSUB(REG_ITMP2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if (s2 == d)
				M_IMUL(s1, d);
			else {
				M_INTMOVE(s1, d);
				M_IMUL(s2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			if (iptr->val.i == 2) {
				M_INTMOVE(s1, d);
				M_ISLL_IMM(1, d);
			} else
				M_IMUL_IMM(s1, iptr->val.i, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if (s2 == d)
				M_LMUL(s1, d);
			else {
				M_INTMOVE(s1, d);
				M_LMUL(s2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			if (IS_IMM32(iptr->val.l))
				M_LMUL_IMM(s1, iptr->val.l, d);
			else {
				M_MOV_IMM(iptr->val.l, REG_ITMP2);
				M_INTMOVE(s1, d);
				M_LMUL(REG_ITMP2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_NULL);
	        if (src->prev->flags & INMEMORY)
				M_ILD(RAX, REG_SP, src->prev->regoff * 8);
			else
				M_INTMOVE(src->prev->regoff, RAX);
			
			if (src->flags & INMEMORY)
				M_ILD(REG_ITMP3, REG_SP, src->regoff * 8);
			else
				M_INTMOVE(src->regoff, REG_ITMP3);

			if (checknull) {
				M_ITEST(REG_ITMP3);
				M_BEQ(0);
				codegen_add_arithmeticexception_ref(cd);
			}

			emit_alul_imm_reg(cd, ALU_CMP, 0x80000000, RAX); /* check as described in jvm spec */
			emit_jcc(cd, CC_NE, 4 + 6);
			emit_alul_imm_reg(cd, ALU_CMP, -1, REG_ITMP3);    /* 4 bytes */
			emit_jcc(cd, CC_E, 3 + 1 + 3);                /* 6 bytes */

			emit_mov_reg_reg(cd, RDX, REG_ITMP2); /* save %rdx, cause it's an argument register */
  			emit_cltd(cd);
			emit_idivl_reg(cd, REG_ITMP3);

			if (iptr->dst->flags & INMEMORY) {
				emit_mov_reg_membase(cd, RAX, REG_SP, iptr->dst->regoff * 8);
				emit_mov_reg_reg(cd, REG_ITMP2, RDX);       /* restore %rdx */

			} else {
				M_INTMOVE(RAX, iptr->dst->regoff);

				if (iptr->dst->regoff != RDX) {
					emit_mov_reg_reg(cd, REG_ITMP2, RDX);   /* restore %rdx */
				}
			}
			break;

		case ICMD_IREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_NULL);
			if (src->prev->flags & INMEMORY)
				M_ILD(RAX, REG_SP, src->prev->regoff * 8);
			else
				M_INTMOVE(src->prev->regoff, RAX);
			
			if (src->flags & INMEMORY)
				M_ILD(REG_ITMP3, REG_SP, src->regoff * 8);
			else
				M_INTMOVE(src->regoff, REG_ITMP3);

			if (checknull) {
				M_ITEST(REG_ITMP3);
				M_BEQ(0);
				codegen_add_arithmeticexception_ref(cd);
			}

			emit_mov_reg_reg(cd, RDX, REG_ITMP2); /* save %rdx, cause it's an argument register */

			emit_alul_imm_reg(cd, ALU_CMP, 0x80000000, RAX); /* check as described in jvm spec */
			emit_jcc(cd, CC_NE, 2 + 4 + 6);


			emit_alul_reg_reg(cd, ALU_XOR, RDX, RDX);         /* 2 bytes */
			emit_alul_imm_reg(cd, ALU_CMP, -1, REG_ITMP3);    /* 4 bytes */
			emit_jcc(cd, CC_E, 1 + 3);                    /* 6 bytes */

  			emit_cltd(cd);
			emit_idivl_reg(cd, REG_ITMP3);

			if (iptr->dst->flags & INMEMORY) {
				emit_mov_reg_membase(cd, RDX, REG_SP, iptr->dst->regoff * 8);
				emit_mov_reg_reg(cd, REG_ITMP2, RDX);       /* restore %rdx */

			} else {
				M_INTMOVE(RDX, iptr->dst->regoff);

				if (iptr->dst->regoff != RDX) {
					emit_mov_reg_reg(cd, REG_ITMP2, RDX);   /* restore %rdx */
				}
			}
			break;

		case ICMD_IDIVPOW2:   /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, REG_ITMP1);
			emit_alul_imm_reg(cd, ALU_CMP, -1, REG_ITMP1);
			emit_leal_membase_reg(cd, REG_ITMP1, (1 << iptr->val.i) - 1, REG_ITMP2);
			emit_cmovccl_reg_reg(cd, CC_LE, REG_ITMP2, REG_ITMP1);
			emit_shiftl_imm_reg(cd, SHIFT_SAR, iptr->val.i, REG_ITMP1);
			emit_mov_reg_reg(cd, REG_ITMP1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, REG_ITMP1);
			emit_alul_imm_reg(cd, ALU_CMP, -1, REG_ITMP1);
			emit_leal_membase_reg(cd, REG_ITMP1, iptr->val.i, REG_ITMP2);
			emit_cmovccl_reg_reg(cd, CC_G, REG_ITMP1, REG_ITMP2);
			emit_alul_imm_reg(cd, ALU_AND, -1 - (iptr->val.i), REG_ITMP2);
			emit_alul_reg_reg(cd, ALU_SUB, REG_ITMP2, REG_ITMP1);
			emit_mov_reg_reg(cd, REG_ITMP1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;


		case ICMD_LDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_NULL);

	        if (src->prev->flags & INMEMORY)
				M_LLD(RAX, REG_SP, src->prev->regoff * 8);
			else
				M_INTMOVE(src->prev->regoff, RAX);
			
			if (src->flags & INMEMORY)
				M_LLD(REG_ITMP3, REG_SP, src->regoff * 8);
			else
				M_INTMOVE(src->regoff, REG_ITMP3);

			if (checknull) {
				M_TEST(REG_ITMP3);
				M_BEQ(0);
				codegen_add_arithmeticexception_ref(cd);
			}

			/* check as described in jvm spec */
			disp = dseg_adds8(cd, 0x8000000000000000LL);
  			M_LCMP_MEMBASE(RIP, -(((ptrint) cd->mcodeptr + 7) - (ptrint) cd->mcodebase) + disp, RAX);
			M_BNE(4 + 6);
			M_LCMP_IMM(-1, REG_ITMP3);                              /* 4 bytes */
			M_BEQ(3 + 2 + 3);                                      /* 6 bytes */

			M_MOV(RDX, REG_ITMP2); /* save %rdx, cause it's an argument register */
  			emit_cqto(cd);
			emit_idiv_reg(cd, REG_ITMP3);

			if (iptr->dst->flags & INMEMORY) {
				M_LST(RAX, REG_SP, iptr->dst->regoff * 8);
				M_MOV(REG_ITMP2, RDX);                        /* restore %rdx */

			} else {
				M_INTMOVE(RAX, iptr->dst->regoff);

				if (iptr->dst->regoff != RDX) {
					M_MOV(REG_ITMP2, RDX);                    /* restore %rdx */
				}
			}
			break;

		case ICMD_LREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_NULL);
			if (src->prev->flags & INMEMORY)
				M_LLD(REG_ITMP1, REG_SP, src->prev->regoff * 8);
			else
				M_INTMOVE(src->prev->regoff, REG_ITMP1);
			
			if (src->flags & INMEMORY)
				M_LLD(REG_ITMP3, REG_SP, src->regoff * 8);
			else
				M_INTMOVE(src->regoff, REG_ITMP3);

			if (checknull) {
				M_ITEST(REG_ITMP3);
				M_BEQ(0);
				codegen_add_arithmeticexception_ref(cd);
			}

			M_MOV(RDX, REG_ITMP2); /* save %rdx, cause it's an argument register */

			/* check as described in jvm spec */
			disp = dseg_adds8(cd, 0x8000000000000000LL);
  			M_LCMP_MEMBASE(RIP, -(((ptrint) cd->mcodeptr + 7) - (ptrint) cd->mcodebase) + disp, REG_ITMP1);
			M_BNE(3 + 4 + 6);

#if 0
			emit_alul_reg_reg(cd, ALU_XOR, RDX, RDX);         /* 2 bytes */
#endif
			M_LXOR(RDX, RDX);                                      /* 3 bytes */
			M_LCMP_IMM(-1, REG_ITMP3);                              /* 4 bytes */
			M_BEQ(2 + 3);                                          /* 6 bytes */

  			emit_cqto(cd);
			emit_idiv_reg(cd, REG_ITMP3);

			if (iptr->dst->flags & INMEMORY) {
				M_LST(RDX, REG_SP, iptr->dst->regoff * 8);
				M_MOV(REG_ITMP2, RDX);                        /* restore %rdx */

			} else {
				M_INTMOVE(RDX, iptr->dst->regoff);

				if (iptr->dst->regoff != RDX) {
					M_MOV(REG_ITMP2, RDX);                    /* restore %rdx */
				}
			}
			break;

		case ICMD_LDIVPOW2:   /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, REG_ITMP1);
			emit_alu_imm_reg(cd, ALU_CMP, -1, REG_ITMP1);
			emit_lea_membase_reg(cd, REG_ITMP1, (1 << iptr->val.i) - 1, REG_ITMP2);
			emit_cmovcc_reg_reg(cd, CC_LE, REG_ITMP2, REG_ITMP1);
			emit_shift_imm_reg(cd, SHIFT_SAR, iptr->val.i, REG_ITMP1);
			emit_mov_reg_reg(cd, REG_ITMP1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, REG_ITMP1);
			emit_alu_imm_reg(cd, ALU_CMP, -1, REG_ITMP1);
			emit_lea_membase_reg(cd, REG_ITMP1, iptr->val.i, REG_ITMP2);
			emit_cmovcc_reg_reg(cd, CC_G, REG_ITMP1, REG_ITMP2);
			emit_alu_imm_reg(cd, ALU_AND, -1 - (iptr->val.i), REG_ITMP2);
			emit_alu_reg_reg(cd, ALU_SUB, REG_ITMP2, REG_ITMP1);
			emit_mov_reg_reg(cd, REG_ITMP1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_ISHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_NULL);
			emit_ishift(cd, SHIFT_SHL, src, iptr);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			M_INTMOVE(s1, d);
			M_ISLL_IMM(iptr->val.i, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_NULL);
			emit_ishift(cd, SHIFT_SAR, src, iptr);
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_INTMOVE(s1, d);
			M_ISRA_IMM(iptr->val.i, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_NULL);
			emit_ishift(cd, SHIFT_SHR, src, iptr);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_INTMOVE(s1, d);
			M_ISRL_IMM(iptr->val.i, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LSHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_NULL);
			emit_lshift(cd, SHIFT_SHL, src, iptr);
			break;

        case ICMD_LSHLCONST:  /* ..., value  ==> ..., value << constant       */
 			                  /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			M_INTMOVE(s1, d);
			M_LSLL_IMM(iptr->val.i, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_NULL);
			emit_lshift(cd, SHIFT_SAR, src, iptr);
			break;

		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_INTMOVE(s1, d);
			M_LSRA_IMM(iptr->val.i, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_NULL);
			emit_lshift(cd, SHIFT_SHR, src, iptr);
			break;

  		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
  		                      /* val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_INTMOVE(s1, d);
			M_LSRL_IMM(iptr->val.i, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if (s2 == d)
				M_IAND(s1, d);
			else {
				M_INTMOVE(s1, d);
				M_IAND(s2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			M_INTMOVE(s1, d);
			M_IAND_IMM(iptr->val.i, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if (s2 == d)
				M_LAND(s1, d);
			else {
				M_INTMOVE(s1, d);
				M_LAND(s2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			M_INTMOVE(s1, d);
			if (IS_IMM32(iptr->val.l))
				M_LAND_IMM(iptr->val.l, d);
			else {
				M_MOV_IMM(iptr->val.l, REG_ITMP2);
				M_LAND(REG_ITMP2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if (s2 == d)
				M_IOR(s1, d);
			else {
				M_INTMOVE(s1, d);
				M_IOR(s2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			M_INTMOVE(s1, d);
			M_IOR_IMM(iptr->val.i, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if (s2 == d)
				M_LOR(s1, d);
			else {
				M_INTMOVE(s1, d);
				M_LOR(s2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			M_INTMOVE(s1, d);
			if (IS_IMM32(iptr->val.l))
				M_LOR_IMM(iptr->val.l, d);
			else {
				M_MOV_IMM(iptr->val.l, REG_ITMP2);
				M_LOR(REG_ITMP2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if (s2 == d)
				M_IXOR(s1, d);
			else {
				M_INTMOVE(s1, d);
				M_IXOR(s2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			M_INTMOVE(s1, d);
			M_IXOR_IMM(iptr->val.i, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if (s2 == d)
				M_LXOR(s1, d);
			else {
				M_INTMOVE(s1, d);
				M_LXOR(s2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			M_INTMOVE(s1, d);
			if (IS_IMM32(iptr->val.l))
				M_LXOR_IMM(iptr->val.l, d);
			else {
				M_MOV_IMM(iptr->val.l, REG_ITMP2);
				M_LXOR(REG_ITMP2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;


		case ICMD_IINC:       /* ..., value  ==> ..., value + constant        */
		                      /* op1 = variable, val.i = constant             */

			var = &(rd->locals[iptr->op1][TYPE_INT]);
			if (var->flags & INMEMORY) {
				s1 = REG_ITMP1;
				M_ILD(s1, REG_SP, var->regoff * 8);
			} else
				s1 = var->regoff;

			/* Using inc and dec is not faster than add (tested with
			   sieve). */

			M_IADD_IMM(iptr->val.i, s1);

			if (var->flags & INMEMORY)
				M_IST(s1, REG_SP, var->regoff * 8);
			break;


		/* floating operations ************************************************/

		case ICMD_FNEG:       /* ..., value  ==> ..., - value                 */

			s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			disp = dseg_adds4(cd, 0x80000000);
			M_FLTMOVE(s1, d);
			emit_movss_membase_reg(cd, RIP, -(((s8) cd->mcodeptr + 9) - (s8) cd->mcodebase) + disp, REG_FTMP2);
			emit_xorps_reg_reg(cd, REG_FTMP2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */

			s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			disp = dseg_adds8(cd, 0x8000000000000000);
			M_FLTMOVE(s1, d);
			emit_movd_membase_reg(cd, RIP, -(((s8) cd->mcodeptr + 9) - (s8) cd->mcodebase) + disp, REG_FTMP2);
			emit_xorpd_reg_reg(cd, REG_FTMP2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			if (s2 == d)
				M_FADD(s1, d);
			else {
				M_FLTMOVE(s1, d);
				M_FADD(s2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_DADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			if (s2 == d)
				M_DADD(s1, d);
			else {
				M_FLTMOVE(s1, d);
				M_DADD(s2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_FSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			if (s2 == d) {
				M_FLTMOVE(s2, REG_FTMP2);
				s2 = REG_FTMP2;
			}
			M_FLTMOVE(s1, d);
			M_FSUB(s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_DSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			if (s2 == d) {
				M_FLTMOVE(s2, REG_FTMP2);
				s2 = REG_FTMP2;
			}
			M_FLTMOVE(s1, d);
			M_DSUB(s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_FMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			if (s2 == d)
				M_FMUL(s1, d);
			else {
				M_FLTMOVE(s1, d);
				M_FMUL(s2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_DMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			if (s2 == d)
				M_DMUL(s1, d);
			else {
				M_FLTMOVE(s1, d);
				M_DMUL(s2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_FDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			if (s2 == d) {
				M_FLTMOVE(s2, REG_FTMP2);
				s2 = REG_FTMP2;
			}
			M_FLTMOVE(s1, d);
			M_FDIV(s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_DDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			if (s2 == d) {
				M_FLTMOVE(s2, REG_FTMP2);
				s2 = REG_FTMP2;
			}
			M_FLTMOVE(s1, d);
			M_DDIV(s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_I2F:       /* ..., value  ==> ..., (float) value            */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
			M_CVTIF(s1, d);
  			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_I2D:       /* ..., value  ==> ..., (double) value           */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
			M_CVTID(s1, d);
  			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_L2F:       /* ..., value  ==> ..., (float) value            */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
			M_CVTLF(s1, d);
  			emit_store(jd, iptr, iptr->dst, d);
			break;
			
		case ICMD_L2D:       /* ..., value  ==> ..., (double) value           */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
			M_CVTLD(s1, d);
  			emit_store(jd, iptr, iptr->dst, d);
			break;
			
		case ICMD_F2I:       /* ..., value  ==> ..., (int) value              */

			s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			M_CVTFI(s1, d);
			M_ICMP_IMM(0x80000000, d);                        /* corner cases */
			disp = ((s1 == REG_FTMP1) ? 0 : 5) + 10 + 3 +
				((REG_RESULT == d) ? 0 : 3);
			M_BNE(disp);
			M_FLTMOVE(s1, REG_FTMP1);
			M_MOV_IMM(asm_builtin_f2i, REG_ITMP2);
			M_CALL(REG_ITMP2);
			M_INTMOVE(REG_RESULT, d);
  			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_D2I:       /* ..., value  ==> ..., (int) value              */

			s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			M_CVTDI(s1, d);
			M_ICMP_IMM(0x80000000, d);                        /* corner cases */
			disp = ((s1 == REG_FTMP1) ? 0 : 5) + 10 + 3 +
				((REG_RESULT == d) ? 0 : 3);
			M_BNE(disp);
			M_FLTMOVE(s1, REG_FTMP1);
			M_MOV_IMM(asm_builtin_d2i, REG_ITMP2);
			M_CALL(REG_ITMP2);
			M_INTMOVE(REG_RESULT, d);
  			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_F2L:       /* ..., value  ==> ..., (long) value             */

			s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			M_CVTFL(s1, d);
			M_MOV_IMM(0x8000000000000000, REG_ITMP2);
			M_LCMP(REG_ITMP2, d);                             /* corner cases */
			disp = ((s1 == REG_FTMP1) ? 0 : 5) + 10 + 3 +
				((REG_RESULT == d) ? 0 : 3);
			M_BNE(disp);
			M_FLTMOVE(s1, REG_FTMP1);
			M_MOV_IMM(asm_builtin_f2l, REG_ITMP2);
			M_CALL(REG_ITMP2);
			M_INTMOVE(REG_RESULT, d);
  			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_D2L:       /* ..., value  ==> ..., (long) value             */

			s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			M_CVTDL(s1, d);
			M_MOV_IMM(0x8000000000000000, REG_ITMP2);
			M_LCMP(REG_ITMP2, d);                             /* corner cases */
			disp = ((s1 == REG_FTMP1) ? 0 : 5) + 10 + 3 +
				((REG_RESULT == d) ? 0 : 3);
			M_BNE(disp);
			M_FLTMOVE(s1, REG_FTMP1);
			M_MOV_IMM(asm_builtin_d2l, REG_ITMP2);
			M_CALL(REG_ITMP2);
			M_INTMOVE(REG_RESULT, d);
  			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_F2D:       /* ..., value  ==> ..., (double) value           */

			s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			M_CVTFD(s1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_D2F:       /* ..., value  ==> ..., (float) value            */

			s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			M_CVTDF(s1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */
 			                  /* == => 0, < => 1, > => -1 */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			M_CLR(d);
			M_MOV_IMM(1, REG_ITMP1);
			M_MOV_IMM(-1, REG_ITMP2);
			emit_ucomiss_reg_reg(cd, s1, s2);
			M_CMOVB(REG_ITMP1, d);
			M_CMOVA(REG_ITMP2, d);
			M_CMOVP(REG_ITMP2, d);                   /* treat unordered as GT */
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */
 			                  /* == => 0, < => 1, > => -1 */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			M_CLR(d);
			M_MOV_IMM(1, REG_ITMP1);
			M_MOV_IMM(-1, REG_ITMP2);
			emit_ucomiss_reg_reg(cd, s1, s2);
			M_CMOVB(REG_ITMP1, d);
			M_CMOVA(REG_ITMP2, d);
			M_CMOVP(REG_ITMP1, d);                   /* treat unordered as LT */
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_DCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */
 			                  /* == => 0, < => 1, > => -1 */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			M_CLR(d);
			M_MOV_IMM(1, REG_ITMP1);
			M_MOV_IMM(-1, REG_ITMP2);
			emit_ucomisd_reg_reg(cd, s1, s2);
			M_CMOVB(REG_ITMP1, d);
			M_CMOVA(REG_ITMP2, d);
			M_CMOVP(REG_ITMP2, d);                   /* treat unordered as GT */
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_DCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */
 			                  /* == => 0, < => 1, > => -1 */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			M_CLR(d);
			M_MOV_IMM(1, REG_ITMP1);
			M_MOV_IMM(-1, REG_ITMP2);
			emit_ucomisd_reg_reg(cd, s1, s2);
			M_CMOVB(REG_ITMP1, d);
			M_CMOVA(REG_ITMP2, d);
			M_CMOVP(REG_ITMP1, d);                   /* treat unordered as LT */
			emit_store(jd, iptr, iptr->dst, d);
			break;


		/* memory operations **************************************************/

		case ICMD_ARRAYLENGTH: /* ..., arrayref  ==> ..., (int) length        */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			gen_nullptr_check(s1);
			M_ILD(d, s1, OFFSET(java_arrayheader, size));
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_BALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
   			emit_movsbq_memindex_reg(cd, OFFSET(java_bytearray, data[0]), s1, s2, 0, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_CALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			emit_movzwq_memindex_reg(cd, OFFSET(java_chararray, data[0]), s1, s2, 1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;			

		case ICMD_SALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			emit_movswq_memindex_reg(cd, OFFSET(java_shortarray, data[0]), s1, s2, 1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			emit_movl_memindex_reg(cd, OFFSET(java_intarray, data[0]), s1, s2, 2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			emit_mov_memindex_reg(cd, OFFSET(java_longarray, data[0]), s1, s2, 3, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_FALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			emit_movss_memindex_reg(cd, OFFSET(java_floatarray, data[0]), s1, s2, 2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_DALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			emit_movsd_memindex_reg(cd, OFFSET(java_doublearray, data[0]), s1, s2, 3, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_AALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			emit_mov_memindex_reg(cd, OFFSET(java_objectarray, data[0]), s1, s2, 3, d);
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
			emit_movb_reg_memindex(cd, s3, OFFSET(java_bytearray, data[0]), s1, s2, 0);
			break;

		case ICMD_CASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, src->prev->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			s3 = emit_load_s3(jd, iptr, src, REG_ITMP3);
			emit_movw_reg_memindex(cd, s3, OFFSET(java_chararray, data[0]), s1, s2, 1);
			break;

		case ICMD_SASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, src->prev->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			s3 = emit_load_s3(jd, iptr, src, REG_ITMP3);
			emit_movw_reg_memindex(cd, s3, OFFSET(java_shortarray, data[0]), s1, s2, 1);
			break;

		case ICMD_IASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, src->prev->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			s3 = emit_load_s3(jd, iptr, src, REG_ITMP3);
			emit_movl_reg_memindex(cd, s3, OFFSET(java_intarray, data[0]), s1, s2, 2);
			break;

		case ICMD_LASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, src->prev->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			s3 = emit_load_s3(jd, iptr, src, REG_ITMP3);
			emit_mov_reg_memindex(cd, s3, OFFSET(java_longarray, data[0]), s1, s2, 3);
			break;

		case ICMD_FASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, src->prev->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			s3 = emit_load_s3(jd, iptr, src, REG_FTMP3);
			emit_movss_reg_memindex(cd, s3, OFFSET(java_floatarray, data[0]), s1, s2, 2);
			break;

		case ICMD_DASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, src->prev->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			s3 = emit_load_s3(jd, iptr, src, REG_FTMP3);
			emit_movsd_reg_memindex(cd, s3, OFFSET(java_doublearray, data[0]), s1, s2, 3);
			break;

		case ICMD_AASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, src->prev->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			s3 = emit_load_s3(jd, iptr, src, REG_ITMP3);

			M_MOV(s1, rd->argintregs[0]);
			M_MOV(s3, rd->argintregs[1]);
			M_MOV_IMM(BUILTIN_canstore, REG_ITMP1);
			M_CALL(REG_ITMP1);
			M_TEST(REG_RESULT);
			M_BEQ(0);
			codegen_add_arraystoreexception_ref(cd);

			s1 = emit_load_s1(jd, iptr, src->prev->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			s3 = emit_load_s3(jd, iptr, src, REG_ITMP3);
			emit_mov_reg_memindex(cd, s3, OFFSET(java_objectarray, data[0]), s1, s2, 3);
			break;


		case ICMD_BASTORECONST: /* ..., arrayref, index  ==> ...              */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			emit_movb_imm_memindex(cd, iptr->val.i, OFFSET(java_bytearray, data[0]), s1, s2, 0);
			break;

		case ICMD_CASTORECONST:   /* ..., arrayref, index  ==> ...            */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			emit_movw_imm_memindex(cd, iptr->val.i, OFFSET(java_chararray, data[0]), s1, s2, 1);
			break;

		case ICMD_SASTORECONST:   /* ..., arrayref, index  ==> ...            */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			emit_movw_imm_memindex(cd, iptr->val.i, OFFSET(java_shortarray, data[0]), s1, s2, 1);
			break;

		case ICMD_IASTORECONST: /* ..., arrayref, index  ==> ...              */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			emit_movl_imm_memindex(cd, iptr->val.i, OFFSET(java_intarray, data[0]), s1, s2, 2);
			break;

		case ICMD_LASTORECONST: /* ..., arrayref, index  ==> ...              */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}

			if (IS_IMM32(iptr->val.l)) {
				emit_mov_imm_memindex(cd, (u4) (iptr->val.l & 0x00000000ffffffff), OFFSET(java_longarray, data[0]), s1, s2, 3);
			} else {
				emit_movl_imm_memindex(cd, (u4) (iptr->val.l & 0x00000000ffffffff), OFFSET(java_longarray, data[0]), s1, s2, 3);
				emit_movl_imm_memindex(cd, (u4) (iptr->val.l >> 32), OFFSET(java_longarray, data[0]) + 4, s1, s2, 3);
			}
			break;

		case ICMD_AASTORECONST: /* ..., arrayref, index  ==> ...              */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			emit_mov_imm_memindex(cd, 0, OFFSET(java_objectarray, data[0]), s1, s2, 3);
			break;


		case ICMD_GETSTATIC:  /* ...  ==> ..., value                          */
		                      /* op1 = type, val.a = field address            */

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				disp = dseg_addaddress(cd, NULL);

/* 				PROFILE_CYCLE_STOP; */

				codegen_addpatchref(cd, PATCHER_get_putstatic,
									INSTRUCTION_UNRESOLVED_FIELD(iptr), disp);

				if (opt_showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

/* 				PROFILE_CYCLE_START; */

			} else {
				fieldinfo *fi = INSTRUCTION_RESOLVED_FIELDINFO(iptr);

				disp = dseg_addaddress(cd, &(fi->value));

				if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					PROFILE_CYCLE_STOP;

					codegen_addpatchref(cd, PATCHER_clinit, fi->class, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}

					PROFILE_CYCLE_START;
				}
  			}

			/* This approach is much faster than moving the field
			   address inline into a register. */

  			M_ALD(REG_ITMP1, RIP, -(((ptrint) cd->mcodeptr + 7) -
									(ptrint) cd->mcodebase) + disp);

			switch (iptr->op1) {
			case TYPE_INT:
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
				M_ILD(d, REG_ITMP1, 0);
				break;
			case TYPE_LNG:
			case TYPE_ADR:
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
				M_LLD(d, REG_ITMP1, 0);
				break;
			case TYPE_FLT:
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
				M_FLD(d, REG_ITMP1, 0);
				break;
			case TYPE_DBL:				
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
				M_DLD(d, REG_ITMP1, 0);
				break;
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */
		                      /* op1 = type, val.a = field address            */

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				disp = dseg_addaddress(cd, NULL);

/* 				PROFILE_CYCLE_STOP; */

				codegen_addpatchref(cd, PATCHER_get_putstatic,
									INSTRUCTION_UNRESOLVED_FIELD(iptr), disp);

				if (opt_showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

/* 				PROFILE_CYCLE_START; */

			} else {
				fieldinfo *fi = INSTRUCTION_RESOLVED_FIELDINFO(iptr);

				disp = dseg_addaddress(cd, &(fi->value));

				if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					PROFILE_CYCLE_STOP;

					codegen_addpatchref(cd, PATCHER_clinit, fi->class, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}

					PROFILE_CYCLE_START;
				}
  			}

			/* This approach is much faster than moving the field
			   address inline into a register. */

  			M_ALD(REG_ITMP1, RIP, -(((ptrint) cd->mcodeptr + 7) -
									(ptrint) cd->mcodebase) + disp);

			switch (iptr->op1) {
			case TYPE_INT:
				s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
				M_IST(s2, REG_ITMP1, 0);
				break;
			case TYPE_LNG:
			case TYPE_ADR:
				s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
				M_LST(s2, REG_ITMP1, 0);
				break;
			case TYPE_FLT:
				s2 = emit_load_s2(jd, iptr, src, REG_FTMP1);
				M_FST(s2, REG_ITMP1, 0);
				break;
			case TYPE_DBL:
				s2 = emit_load_s2(jd, iptr, src, REG_FTMP1);
				M_DST(s2, REG_ITMP1, 0);
				break;
			}
			break;

		case ICMD_PUTSTATICCONST: /* ...  ==> ...                             */
		                          /* val = value (in current instruction)     */
		                          /* op1 = type, val.a = field address (in    */
		                          /* following NOP)                           */

			if (INSTRUCTION_IS_UNRESOLVED(iptr + 1)) {
				disp = dseg_addaddress(cd, NULL);

/* 				PROFILE_CYCLE_STOP; */

				codegen_addpatchref(cd, PATCHER_get_putstatic,
									INSTRUCTION_UNRESOLVED_FIELD(iptr + 1), disp);

				if (opt_showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

/* 				PROFILE_CYCLE_START; */

			} else {
				fieldinfo *fi = INSTRUCTION_RESOLVED_FIELDINFO(iptr + 1);

				disp = dseg_addaddress(cd, &(fi->value));

				if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					PROFILE_CYCLE_STOP;

					codegen_addpatchref(cd, PATCHER_clinit, fi->class, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}

					PROFILE_CYCLE_START;
				}
  			}

			/* This approach is much faster than moving the field
			   address inline into a register. */

  			M_ALD(REG_ITMP1, RIP, -(((ptrint) cd->mcodeptr + 7) -
									(ptrint) cd->mcodebase) + disp);

			switch (iptr->op1) {
			case TYPE_INT:
			case TYPE_FLT:
				M_IST_IMM(iptr->val.i, REG_ITMP1, 0);
				break;
			case TYPE_LNG:
			case TYPE_ADR:
			case TYPE_DBL:
				if (IS_IMM32(iptr->val.l))
					M_LST_IMM32(iptr->val.l, REG_ITMP1, 0);
				else {
					M_IST_IMM(iptr->val.l, REG_ITMP1, 0);
					M_IST_IMM(iptr->val.l >> 32, REG_ITMP1, 4);
				}
				break;
			}
			break;

		case ICMD_GETFIELD:   /* ...  ==> ..., value                          */
		                      /* op1 = type, val.i = field offset             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			gen_nullptr_check(s1);

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
/* 				PROFILE_CYCLE_STOP; */

				codegen_addpatchref(cd, PATCHER_get_putfield,
									INSTRUCTION_UNRESOLVED_FIELD(iptr), 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

/* 				PROFILE_CYCLE_START; */

				disp = 0;

			} else
				disp = INSTRUCTION_RESOLVED_FIELDINFO(iptr)->offset;

			switch (iptr->op1) {
			case TYPE_INT:
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
				M_ILD32(d, s1, disp);
				break;
			case TYPE_LNG:
			case TYPE_ADR:
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
				M_LLD32(d, s1, disp);
				break;
			case TYPE_FLT:
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
				M_FLD32(d, s1, disp);
				break;
			case TYPE_DBL:				
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
				M_DLD32(d, s1, disp);
				break;
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_PUTFIELD:   /* ..., objectref, value  ==> ...               */
		                      /* op1 = type, val.i = field offset             */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			gen_nullptr_check(s1);

			if (IS_INT_LNG_TYPE(iptr->op1))
				s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			else
				s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
/* 				PROFILE_CYCLE_STOP; */

				codegen_addpatchref(cd, PATCHER_get_putfield,
									INSTRUCTION_UNRESOLVED_FIELD(iptr), 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

/* 				PROFILE_CYCLE_START; */

				disp = 0;

			} else
				disp = INSTRUCTION_RESOLVED_FIELDINFO(iptr)->offset;

			switch (iptr->op1) {
			case TYPE_INT:
				M_IST32(s2, s1, disp);
				break;
			case TYPE_LNG:
			case TYPE_ADR:
				M_LST32(s2, s1, disp);
				break;
			case TYPE_FLT:
				M_FST32(s2, s1, disp);
				break;
			case TYPE_DBL:
				M_DST32(s2, s1, disp);
				break;
			}
			break;

		case ICMD_PUTFIELDCONST:  /* ..., objectref, value  ==> ...           */
		                          /* val = value (in current instruction)     */
		                          /* op1 = type, val.a = field address (in    */
		                          /* following NOP)                           */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			gen_nullptr_check(s1);

			if (INSTRUCTION_IS_UNRESOLVED(iptr + 1)) {
/* 				PROFILE_CYCLE_STOP; */

				codegen_addpatchref(cd, PATCHER_putfieldconst,
									INSTRUCTION_UNRESOLVED_FIELD(iptr + 1), 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

/* 				PROFILE_CYCLE_START; */

				disp = 0;

			} else
				disp = INSTRUCTION_RESOLVED_FIELDINFO(iptr + 1)->offset;

			switch (iptr->op1) {
			case TYPE_INT:
			case TYPE_FLT:
				M_IST32_IMM(iptr->val.i, s1, disp);
				break;
			case TYPE_LNG:
			case TYPE_ADR:
			case TYPE_DBL:
				M_IST32_IMM(iptr->val.l, s1, disp);
				M_IST32_IMM(iptr->val.l >> 32, s1, disp + 4);
				break;
			}
			break;


		/* branch operations **************************************************/

		case ICMD_ATHROW:       /* ..., objectref ==> ... (, objectref)       */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			M_INTMOVE(s1, REG_ITMP1_XPTR);

			PROFILE_CYCLE_STOP;

#ifdef ENABLE_VERIFIER
			if (iptr->val.a) {
				codegen_addpatchref(cd, PATCHER_athrow_areturn,
									(unresolved_class *) iptr->val.a, 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}
			}
#endif /* ENABLE_VERIFIER */

			M_CALL_IMM(0);                            /* passing exception pc */
			M_POP(REG_ITMP2_XPC);

  			M_MOV_IMM(asm_handle_exception, REG_ITMP3);
  			M_JMP(REG_ITMP3);
			break;

		case ICMD_GOTO:         /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */

			M_JMP_IMM(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_JSR:          /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */

  			M_CALL_IMM(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;
			
		case ICMD_RET:          /* ... ==> ...                                */
		                        /* op1 = local variable                       */

			var = &(rd->locals[iptr->op1][TYPE_ADR]);
			if (var->flags & INMEMORY) {
				M_ALD(REG_ITMP1, REG_SP, var->regoff * 8);
				M_JMP(REG_ITMP1);
			} else
				M_JMP(var->regoff);
			break;

		case ICMD_IFNULL:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			M_TEST(s1);
			M_BEQ(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IFNONNULL:    /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			M_TEST(s1);
			M_BNE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IFEQ:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			M_ICMP_IMM(iptr->val.i, s1);
			M_BEQ(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IFLT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			M_ICMP_IMM(iptr->val.i, s1);
			M_BLT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IFLE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			M_ICMP_IMM(iptr->val.i, s1);
			M_BLE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IFNE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			M_ICMP_IMM(iptr->val.i, s1);
			M_BNE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IFGT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			M_ICMP_IMM(iptr->val.i, s1);
			M_BGT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IFGE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			M_ICMP_IMM(iptr->val.i, s1);
			M_BGE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_LEQ:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			if (IS_IMM32(iptr->val.l))
				M_LCMP_IMM(iptr->val.l, s1);
			else {
				M_MOV_IMM(iptr->val.l, REG_ITMP2);
				M_LCMP(REG_ITMP2, s1);
			}
			M_BEQ(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_LLT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			if (IS_IMM32(iptr->val.l))
				M_LCMP_IMM(iptr->val.l, s1);
			else {
				M_MOV_IMM(iptr->val.l, REG_ITMP2);
				M_LCMP(REG_ITMP2, s1);
			}
			M_BLT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_LLE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			if (IS_IMM32(iptr->val.l))
				M_LCMP_IMM(iptr->val.l, s1);
			else {
				M_MOV_IMM(iptr->val.l, REG_ITMP2);
				M_LCMP(REG_ITMP2, s1);
			}
			M_BLE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_LNE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			if (IS_IMM32(iptr->val.l))
				M_LCMP_IMM(iptr->val.l, s1);
			else {
				M_MOV_IMM(iptr->val.l, REG_ITMP2);
				M_LCMP(REG_ITMP2, s1);
			}
			M_BNE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_LGT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			if (IS_IMM32(iptr->val.l))
				M_LCMP_IMM(iptr->val.l, s1);
			else {
				M_MOV_IMM(iptr->val.l, REG_ITMP2);
				M_LCMP(REG_ITMP2, s1);
			}
			M_BGT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_LGE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			if (IS_IMM32(iptr->val.l))
				M_LCMP_IMM(iptr->val.l, s1);
			else {
				M_MOV_IMM(iptr->val.l, REG_ITMP2);
				M_LCMP(REG_ITMP2, s1);
			}
			M_BGE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_ICMPEQ:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_ICMP(s2, s1);
			M_BEQ(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_LCMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_ACMPEQ:    /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_LCMP(s2, s1);
			M_BEQ(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_ICMPNE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_ICMP(s2, s1);
			M_BNE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_LCMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_ACMPNE:    /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_LCMP(s2, s1);
			M_BNE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_ICMPLT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_ICMP(s2, s1);
			M_BLT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_LCMPLT:    /* ..., value, value ==> ...                  */
	                            /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_LCMP(s2, s1);
			M_BLT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_ICMPGT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_ICMP(s2, s1);
			M_BGT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_LCMPGT:    /* ..., value, value ==> ...                  */
                                /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_LCMP(s2, s1);
			M_BGT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_ICMPLE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_ICMP(s2, s1);
			M_BLE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_LCMPLE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_LCMP(s2, s1);
			M_BLE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_ICMPGE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_ICMP(s2, s1);
			M_BGE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		case ICMD_IF_LCMPGE:    /* ..., value, value ==> ...                  */
	                            /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_LCMP(s2, s1);
			M_BGE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			break;

		/* (value xx 0) ? IFxx_ICONST : ELSE_ICONST                           */

		case ICMD_ELSE_ICONST:  /* handled by IFxx_ICONST                     */
			break;

		case ICMD_IFEQ_ICONST:  /* ..., value ==> ..., constant               */
		case ICMD_IFNE_ICONST:  /* val.i = constant                           */
		case ICMD_IFLT_ICONST:
		case ICMD_IFGE_ICONST:
		case ICMD_IFGT_ICONST:
		case ICMD_IFLE_ICONST:

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			if (iptr[1].opc == ICMD_ELSE_ICONST) {
				if (s1 == d) {
					M_INTMOVE(s1, REG_ITMP1);
					s1 = REG_ITMP1;
				}
				if (iptr[1].val.i == 0)
					M_CLR(d);
				else
					M_IMOV_IMM(iptr[1].val.i, d);
			}
			if (iptr->val.i == 0)
				M_CLR(REG_ITMP2);
			else
				M_IMOV_IMM(iptr->val.i, REG_ITMP2);
			M_ITEST(s1);

			switch (iptr->opc) {
			case ICMD_IFEQ_ICONST:
				M_CMOVEQ(REG_ITMP2, d);
				break;
			case ICMD_IFNE_ICONST:
				M_CMOVNE(REG_ITMP2, d);
				break;
			case ICMD_IFLT_ICONST:
				M_CMOVLT(REG_ITMP2, d);
				break;
			case ICMD_IFGE_ICONST:
				M_CMOVGE(REG_ITMP2, d);
				break;
			case ICMD_IFGT_ICONST:
				M_CMOVGT(REG_ITMP2, d);
				break;
			case ICMD_IFLE_ICONST:
				M_CMOVLE(REG_ITMP2, d);
				break;
			}

			emit_store(jd, iptr, iptr->dst, d);
			break;


		case ICMD_IRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_LRETURN:

			s1 = emit_load_s1(jd, iptr, src, REG_RESULT);
			M_INTMOVE(s1, REG_RESULT);
			goto nowperformreturn;

		case ICMD_ARETURN:      /* ..., retvalue ==> ...                      */

			s1 = emit_load_s1(jd, iptr, src, REG_RESULT);
			M_INTMOVE(s1, REG_RESULT);

#ifdef ENABLE_VERIFIER
			if (iptr->val.a) {
				PROFILE_CYCLE_STOP;

				codegen_addpatchref(cd, PATCHER_athrow_areturn,
									(unresolved_class *) iptr->val.a, 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

				PROFILE_CYCLE_START;
			}
#endif /* ENABLE_VERIFIER */
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

#if !defined(NDEBUG)
			/* generate call trace */

			if (opt_verbosecall) {
				emit_alu_imm_reg(cd, ALU_SUB, 2 * 8, REG_SP);

				emit_mov_reg_membase(cd, REG_RESULT, REG_SP, 0 * 8);
				emit_movq_reg_membase(cd, REG_FRESULT, REG_SP, 1 * 8);

  				emit_mov_imm_reg(cd, (u8) m, rd->argintregs[0]);
  				emit_mov_reg_reg(cd, REG_RESULT, rd->argintregs[1]);
				M_FLTMOVE(REG_FRESULT, rd->argfltregs[0]);
 				M_FLTMOVE(REG_FRESULT, rd->argfltregs[1]);

  				M_MOV_IMM(builtin_displaymethodstop, REG_ITMP1);
				M_CALL(REG_ITMP1);

				emit_mov_membase_reg(cd, REG_SP, 0 * 8, REG_RESULT);
				emit_movq_membase_reg(cd, REG_SP, 1 * 8, REG_FRESULT);

				emit_alu_imm_reg(cd, ALU_ADD, 2 * 8, REG_SP);
			}
#endif /* !defined(NDEBUG) */

#if defined(ENABLE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				M_ALD(rd->argintregs[0], REG_SP, rd->memuse * 8);
	
				/* we need to save the proper return value */
				switch (iptr->opc) {
				case ICMD_IRETURN:
				case ICMD_ARETURN:
				case ICMD_LRETURN:
					M_LST(REG_RESULT, REG_SP, rd->memuse * 8);
					break;
				case ICMD_FRETURN:
				case ICMD_DRETURN:
					M_DST(REG_FRESULT, REG_SP, rd->memuse * 8);
					break;
				}

				M_MOV_IMM(LOCK_monitor_exit, REG_ITMP1);
				M_CALL(REG_ITMP1);

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

			/* restore saved registers */

			for (i = INT_SAV_CNT - 1; i >= rd->savintreguse; i--) {
				p--; M_LLD(rd->savintregs[i], REG_SP, p * 8);
			}
			for (i = FLT_SAV_CNT - 1; i >= rd->savfltreguse; i--) {
  				p--; M_DLD(rd->savfltregs[i], REG_SP, p * 8);
			}

			/* deallocate stack */

			if (stackframesize)
				M_AADD_IMM(stackframesize * 8, REG_SP);

			/* generate method profiling code */

			PROFILE_CYCLE_STOP;

			M_RET;
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
				M_INTMOVE(s1, REG_ITMP1);

				if (l != 0)
					M_ISUB_IMM(l, REG_ITMP1);

				i = i - l + 1;

                /* range check */
				M_ICMP_IMM(i - 1, REG_ITMP1);
				M_BA(0);

				codegen_addreference(cd, (basicblock *) tptr[0]);

				/* build jump table top down and use address of lowest entry */

                /* s4ptr += 3 + i; */
				tptr += i;

				while (--i >= 0) {
					dseg_addtarget(cd, (basicblock *) tptr[0]); 
					--tptr;
				}

				/* length of dataseg after last dseg_addtarget is used
				   by load */

				M_MOV_IMM(0, REG_ITMP2);
				dseg_adddata(cd);
				emit_mov_memindex_reg(cd, -(cd->dseglen), REG_ITMP2, REG_ITMP1, 3, REG_ITMP1);
				M_JMP(REG_ITMP1);
			}
			break;


		case ICMD_LOOKUPSWITCH: /* ..., key ==> ...                           */
			{
				s4 i, l, val, *s4ptr;
				void **tptr;

				tptr = (void **) iptr->target;

				s4ptr = iptr->val.a;
				l = s4ptr[0];                          /* default  */
				i = s4ptr[1];                          /* count    */
			
				MCODECHECK(8 + ((7 + 6) * i) + 5);
				s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);

				while (--i >= 0) {
					s4ptr += 2;
					++tptr;

					val = s4ptr[0];
					M_ICMP_IMM(val, s1);
					M_BEQ(0);
					codegen_addreference(cd, (basicblock *) tptr[0]);
				}

				M_JMP_IMM(0);
			
				tptr = (void **) iptr->target;
				codegen_addreference(cd, (basicblock *) tptr[0]);
			}
			break;


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

			MCODECHECK((20 * s3) + 128);

			/* copy arguments to registers or stack location */

			for (s3 = s3 - 1; s3 >= 0; s3--, src = src->prev) {
				if (src->varkind == ARGVAR)
					continue;
				if (IS_INT_LNG_TYPE(src->type)) {
					if (!md->params[s3].inmemory) {
						s1 = rd->argintregs[md->params[s3].regoff];
						d = emit_load_s1(jd, iptr, src, s1);
						M_INTMOVE(d, s1);
					} else {
						d = emit_load_s1(jd, iptr, src, REG_ITMP1);
						M_LST(d, REG_SP, md->params[s3].regoff * 8);
					}
						
				} else {
					if (!md->params[s3].inmemory) {
						s1 = rd->argfltregs[md->params[s3].regoff];
						d = emit_load_s1(jd, iptr, src, s1);
						M_FLTMOVE(d, s1);
					} else {
						d = emit_load_s1(jd, iptr, src, REG_FTMP1);

						if (IS_2_WORD_TYPE(src->type))
							M_DST(d, REG_SP, md->params[s3].regoff * 8);
						else
							M_FST(d, REG_SP, md->params[s3].regoff * 8);
					}
				}
			}

			/* generate method profiling code */

			PROFILE_CYCLE_STOP;

			switch (iptr->opc) {
			case ICMD_BUILTIN:
				a = (ptrint) bte->fp;
				d = md->returntype.type;

				M_MOV_IMM(a, REG_ITMP1);
				M_CALL(REG_ITMP1);

				/* if op1 == true, we need to check for an exception */

				if (iptr->op1 == true) {
					M_TEST(REG_RESULT);
					M_BEQ(0);
					codegen_add_fillinstacktrace_ref(cd);
				}
				break;

			case ICMD_INVOKESPECIAL:
				M_TEST(rd->argintregs[0]);
				M_BEQ(0);
				codegen_add_nullpointerexception_ref(cd);

				/* first argument contains pointer */
/*  				gen_nullptr_check(rd->argintregs[0]); */

				/* access memory for hardware nullptr */
/*  				emit_mov_membase_reg(cd, rd->argintregs[0], 0, REG_ITMP2); */

				/* fall through */

			case ICMD_INVOKESTATIC:
				if (lm == NULL) {
					unresolved_method *um = INSTRUCTION_UNRESOLVED_METHOD(iptr);

					codegen_addpatchref(cd, PATCHER_invokestatic_special,
										um, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}

					a = 0;
					d = um->methodref->parseddesc.md->returntype.type;

				} else {
					a = (ptrint) lm->stubroutine;
					d = lm->parseddesc->returntype.type;
				}

				M_MOV_IMM(a, REG_ITMP2);
				M_CALL(REG_ITMP2);
				break;

			case ICMD_INVOKEVIRTUAL:
				gen_nullptr_check(rd->argintregs[0]);

				if (lm == NULL) {
					unresolved_method *um = INSTRUCTION_UNRESOLVED_METHOD(iptr);

					codegen_addpatchref(cd, PATCHER_invokevirtual, um, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
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
				M_ALD32(REG_ITMP3, REG_METHODPTR, s1);
				M_CALL(REG_ITMP3);
				break;

			case ICMD_INVOKEINTERFACE:
				gen_nullptr_check(rd->argintregs[0]);

				if (lm == NULL) {
					unresolved_method *um = INSTRUCTION_UNRESOLVED_METHOD(iptr);

					codegen_addpatchref(cd, PATCHER_invokeinterface, um, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}

					s1 = 0;
					s2 = 0;
					d = um->methodref->parseddesc.md->returntype.type;

				} else {
					s1 = OFFSET(vftbl_t, interfacetable[0]) -
						sizeof(methodptr) * lm->class->index;

					s2 = sizeof(methodptr) * (lm - lm->class->methods);

					d = lm->parseddesc->returntype.type;
				}

				M_ALD(REG_METHODPTR, rd->argintregs[0],
					  OFFSET(java_objectheader, vftbl));
				M_ALD32(REG_METHODPTR, REG_METHODPTR, s1);
				M_ALD32(REG_ITMP3, REG_METHODPTR, s2);
				M_CALL(REG_ITMP3);
				break;
			}

			/* generate method profiling code */

			PROFILE_CYCLE_START;

			/* d contains return type */

			if (d != TYPE_VOID) {
				if (IS_INT_LNG_TYPE(iptr->dst->type)) {
					s1 = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_RESULT);
					M_INTMOVE(REG_RESULT, s1);
					emit_store(jd, iptr, iptr->dst, s1);
				} else {
					s1 = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FRESULT);
					M_FLTMOVE(REG_FRESULT, s1);
					emit_store(jd, iptr, iptr->dst, s1);
				}
			}
			break;


		case ICMD_CHECKCAST:  /* ..., objectref ==> ..., objectref            */

		                      /* op1:   0 == array, 1 == class                */
		                      /* val.a: (classinfo *) superclass              */

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

			if (iptr->op1 == 1) {
				/* object type cast-check */

				classinfo *super;
				vftbl_t   *supervftbl;
				s4         superindex;

				super = (classinfo *) iptr->val.a;

				if (super == NULL) {
					superindex = 0;
					supervftbl = NULL;
				}
				else {
					superindex = super->index;
					supervftbl = super->vftbl;
				}

#if defined(ENABLE_THREADS)
				codegen_threadcritrestart(cd, cd->mcodeptr - cd->mcodebase);
#endif
				s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);

				/* calculate interface checkcast code size */

				s2 = 3; /* mov_membase_reg */
				CALCOFFSETBYTES(s2, s1, OFFSET(java_objectheader, vftbl));

				s2 += 3 + 4 /* movl_membase32_reg */ + 3 + 4 /* sub imm32 */ +
					3 /* test */ + 6 /* jcc */ + 3 + 4 /* mov_membase32_reg */ +
					3 /* test */ + 6 /* jcc */;

				if (super == NULL)
					s2 += (opt_showdisassemble ? 5 : 0);

				/* calculate class checkcast code size */

				s3 = 3; /* mov_membase_reg */
				CALCOFFSETBYTES(s3, s1, OFFSET(java_objectheader, vftbl));
				s3 += 10 /* mov_imm_reg */ + 3 + 4 /* movl_membase32_reg */;

#if 0
				if (s1 != REG_ITMP1) {
					a += 3;    /* movl_membase_reg - only if REG_ITMP3 == R11 */
					CALCOFFSETBYTES(a, REG_ITMP3, OFFSET(vftbl_t, baseval));
					a += 3;    /* movl_membase_reg - only if REG_ITMP3 == R11 */
					CALCOFFSETBYTES(a, REG_ITMP3, OFFSET(vftbl_t, diffval));
					a += 3;    /* sub */
				
				} else
#endif
					{
						s3 += 3 + 4 /* movl_membase32_reg */ + 3 /* sub */ +
							10 /* mov_imm_reg */ + 3 /* movl_membase_reg */;
						CALCOFFSETBYTES(s3, REG_ITMP3, OFFSET(vftbl_t, diffval));
					}
			
				s3 += 3 /* cmp */ + 6 /* jcc */;

				if (super == NULL)
					s3 += (opt_showdisassemble ? 5 : 0);

				/* if class is not resolved, check which code to call */

				if (super == NULL) {
					M_TEST(s1);
					M_BEQ(6 + (opt_showdisassemble ? 5 : 0) + 7 + 6 + s2 + 5 + s3);

					codegen_addpatchref(cd, PATCHER_checkcast_instanceof_flags,
										(constant_classref *) iptr->target, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}

					M_IMOV_IMM(0, REG_ITMP2);                 /* super->flags */
					M_IAND_IMM(ACC_INTERFACE, REG_ITMP2);
					M_BEQ(s2 + 5);
				}

				/* interface checkcast code */

				if ((super == NULL) || (super->flags & ACC_INTERFACE)) {
					if (super != NULL) {
						M_TEST(s1);
						M_BEQ(s2);
					}

					M_ALD(REG_ITMP2, s1, OFFSET(java_objectheader, vftbl));

					if (super == NULL) {
						codegen_addpatchref(cd,
											PATCHER_checkcast_instanceof_interface,
											(constant_classref *) iptr->target,
											0);

						if (opt_showdisassemble) {
							M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
						}
					}

					emit_movl_membase32_reg(cd, REG_ITMP2,
											  OFFSET(vftbl_t, interfacetablelength),
											  REG_ITMP3);
					/* XXX TWISTI: should this be int arithmetic? */
					M_LSUB_IMM32(superindex, REG_ITMP3);
					M_TEST(REG_ITMP3);
					M_BLE(0);
					codegen_add_classcastexception_ref(cd, s1);
					emit_mov_membase32_reg(cd, REG_ITMP2,
											 OFFSET(vftbl_t, interfacetable[0]) -
											 superindex * sizeof(methodptr*),
											 REG_ITMP3);
					M_TEST(REG_ITMP3);
					M_BEQ(0);
					codegen_add_classcastexception_ref(cd, s1);

					if (super == NULL)
						M_JMP_IMM(s3);
				}

				/* class checkcast code */

				if ((super == NULL) || !(super->flags & ACC_INTERFACE)) {
					if (super != NULL) {
						M_TEST(s1);
						M_BEQ(s3);
					}

					M_ALD(REG_ITMP2, s1, OFFSET(java_objectheader, vftbl));

					if (super == NULL) {
						codegen_addpatchref(cd, PATCHER_checkcast_class,
											(constant_classref *) iptr->target,
											0);

						if (opt_showdisassemble) {
							M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
						}
					}

					M_MOV_IMM(supervftbl, REG_ITMP3);
#if defined(ENABLE_THREADS)
					codegen_threadcritstart(cd, cd->mcodeptr - cd->mcodebase);
#endif
					emit_movl_membase32_reg(cd, REG_ITMP2,
											  OFFSET(vftbl_t, baseval),
											  REG_ITMP2);
					/*  					if (s1 != REG_ITMP1) { */
					/*  						emit_movl_membase_reg(cd, REG_ITMP3, */
					/*  												OFFSET(vftbl_t, baseval), */
					/*  												REG_ITMP1); */
					/*  						emit_movl_membase_reg(cd, REG_ITMP3, */
					/*  												OFFSET(vftbl_t, diffval), */
					/*  												REG_ITMP3); */
					/*  #if defined(ENABLE_THREADS) */
					/*  						codegen_threadcritstop(cd, cd->mcodeptr - cd->mcodebase); */
					/*  #endif */
					/*  						emit_alu_reg_reg(cd, ALU_SUB, REG_ITMP1, REG_ITMP2); */

					/*  					} else { */
					emit_movl_membase32_reg(cd, REG_ITMP3,
											  OFFSET(vftbl_t, baseval),
											  REG_ITMP3);
					M_LSUB(REG_ITMP3, REG_ITMP2);
					M_MOV_IMM(supervftbl, REG_ITMP3);
					M_ILD(REG_ITMP3, REG_ITMP3, OFFSET(vftbl_t, diffval));
					/*  					} */
#if defined(ENABLE_THREADS)
					codegen_threadcritstop(cd, cd->mcodeptr - cd->mcodebase);
#endif
					M_LCMP(REG_ITMP3, REG_ITMP2);
					M_BA(0);         /* (u) REG_ITMP1 > (u) REG_ITMP2 -> jump */
					codegen_add_classcastexception_ref(cd, s1);
				}

				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			}
			else {
				/* array type cast-check */

				s1 = emit_load_s1(jd, iptr, src, REG_ITMP2);
				M_INTMOVE(s1, rd->argintregs[0]);

				if (iptr->val.a == NULL) {
					codegen_addpatchref(cd, PATCHER_builtin_arraycheckcast,
										(constant_classref *) iptr->target, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}
				}

				M_MOV_IMM(iptr->val.a, rd->argintregs[1]);
				M_MOV_IMM(BUILTIN_arraycheckcast, REG_ITMP1);
				M_CALL(REG_ITMP1);

				/* s1 may have been destroyed over the function call */
				s1 = emit_load_s1(jd, iptr, src, REG_ITMP2);
				M_TEST(REG_RESULT);
				M_BEQ(0);
				codegen_add_classcastexception_ref(cd, s1);

				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			}

			M_INTMOVE(s1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_INSTANCEOF: /* ..., objectref ==> ..., intresult            */

		                      /* op1:   0 == array, 1 == class                */
		                      /* val.a: (classinfo *) superclass              */

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
				M_INTMOVE(s1, REG_ITMP1);
				s1 = REG_ITMP1;
			}

			/* calculate interface instanceof code size */

			s2 = 3; /* mov_membase_reg */
			CALCOFFSETBYTES(s2, s1, OFFSET(java_objectheader, vftbl));
			s2 += 3 + 4 /* movl_membase32_reg */ + 3 + 4 /* sub_imm32 */ +
				3 /* test */ + 6 /* jcc */ + 3 + 4 /* mov_membase32_reg */ +
				3 /* test */ + 4 /* setcc */;

			if (!super)
				s2 += (opt_showdisassemble ? 5 : 0);

			/* calculate class instanceof code size */
			
			s3 = 3; /* mov_membase_reg */
			CALCOFFSETBYTES(s3, s1, OFFSET(java_objectheader, vftbl));
			s3 += 10; /* mov_imm_reg */
			s3 += 2; /* movl_membase_reg - only if REG_ITMP1 == RAX */
			CALCOFFSETBYTES(s3, REG_ITMP1, OFFSET(vftbl_t, baseval));
			s3 += 3; /* movl_membase_reg - only if REG_ITMP2 == R10 */
			CALCOFFSETBYTES(s3, REG_ITMP2, OFFSET(vftbl_t, baseval));
			s3 += 3; /* movl_membase_reg - only if REG_ITMP2 == R10 */
			CALCOFFSETBYTES(s3, REG_ITMP2, OFFSET(vftbl_t, diffval));
			s3 += 3 /* sub */ + 3 /* xor */ + 3 /* cmp */ + 4 /* setcc */;

			if (!super)
				s3 += (opt_showdisassemble ? 5 : 0);

			emit_alu_reg_reg(cd, ALU_XOR, d, d);

			/* if class is not resolved, check which code to call */

			if (!super) {
				emit_test_reg_reg(cd, s1, s1);
				emit_jcc(cd, CC_Z, (6 + (opt_showdisassemble ? 5 : 0) +
											 7 + 6 + s2 + 5 + s3));

				codegen_addpatchref(cd, PATCHER_checkcast_instanceof_flags,
									(constant_classref *) iptr->target, 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

				emit_movl_imm_reg(cd, 0, REG_ITMP3); /* super->flags */
				emit_alul_imm_reg(cd, ALU_AND, ACC_INTERFACE, REG_ITMP3);
				emit_jcc(cd, CC_Z, s2 + 5);
			}

			/* interface instanceof code */

			if (!super || (super->flags & ACC_INTERFACE)) {
				if (super) {
					emit_test_reg_reg(cd, s1, s1);
					emit_jcc(cd, CC_Z, s2);
				}

				emit_mov_membase_reg(cd, s1,
									   OFFSET(java_objectheader, vftbl),
									   REG_ITMP1);
				if (!super) {
					codegen_addpatchref(cd,
										PATCHER_checkcast_instanceof_interface,
										(constant_classref *) iptr->target, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}
				}

				emit_movl_membase32_reg(cd, REG_ITMP1,
										  OFFSET(vftbl_t, interfacetablelength),
										  REG_ITMP3);
				emit_alu_imm32_reg(cd, ALU_SUB, superindex, REG_ITMP3);
				emit_test_reg_reg(cd, REG_ITMP3, REG_ITMP3);

				a = 3 + 4 /* mov_membase32_reg */ + 3 /* test */ + 4 /* setcc */;

				emit_jcc(cd, CC_LE, a);
				emit_mov_membase32_reg(cd, REG_ITMP1,
										 OFFSET(vftbl_t, interfacetable[0]) -
										 superindex * sizeof(methodptr*),
										 REG_ITMP1);
				emit_test_reg_reg(cd, REG_ITMP1, REG_ITMP1);
				emit_setcc_reg(cd, CC_NE, d);

				if (!super)
					emit_jmp_imm(cd, s3);
			}

			/* class instanceof code */

			if (!super || !(super->flags & ACC_INTERFACE)) {
				if (super) {
					emit_test_reg_reg(cd, s1, s1);
					emit_jcc(cd, CC_E, s3);
				}

				emit_mov_membase_reg(cd, s1,
									   OFFSET(java_objectheader, vftbl),
									   REG_ITMP1);

				if (!super) {
					codegen_addpatchref(cd, PATCHER_instanceof_class,
										(constant_classref *) iptr->target, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}
				}

				emit_mov_imm_reg(cd, (ptrint) supervftbl, REG_ITMP2);
#if defined(ENABLE_THREADS)
				codegen_threadcritstart(cd, cd->mcodeptr - cd->mcodebase);
#endif
				emit_movl_membase_reg(cd, REG_ITMP1,
										OFFSET(vftbl_t, baseval),
										REG_ITMP1);
				emit_movl_membase_reg(cd, REG_ITMP2,
										OFFSET(vftbl_t, diffval),
										REG_ITMP3);
				emit_movl_membase_reg(cd, REG_ITMP2,
										OFFSET(vftbl_t, baseval),
										REG_ITMP2);
#if defined(ENABLE_THREADS)
				codegen_threadcritstop(cd, cd->mcodeptr - cd->mcodebase);
#endif
				emit_alu_reg_reg(cd, ALU_SUB, REG_ITMP2, REG_ITMP1);
				emit_alu_reg_reg(cd, ALU_XOR, d, d); /* may be REG_ITMP2 */
				emit_alu_reg_reg(cd, ALU_CMP, REG_ITMP3, REG_ITMP1);
				emit_setcc_reg(cd, CC_BE, d);
			}
  			emit_store(jd, iptr, iptr->dst, d);
			}
			break;

		case ICMD_MULTIANEWARRAY:/* ..., cnt1, [cnt2, ...] ==> ..., arrayref  */
		                         /* op1 = dimension, val.a = class            */

			/* check for negative sizes and copy sizes to stack if necessary  */

  			MCODECHECK((10 * 4 * iptr->op1) + 5 + 10 * 8);

			for (s1 = iptr->op1; --s1 >= 0; src = src->prev) {
				/* copy SAVEDVAR sizes to stack */

				if (src->varkind != ARGVAR) {
					s2 = emit_load_s2(jd, iptr, src, REG_ITMP1);
					M_LST(s2, REG_SP, s1 * 8);
				}
			}

			/* is a patcher function set? */

			if (iptr->val.a == NULL) {
				codegen_addpatchref(cd, PATCHER_builtin_multianewarray,
									(constant_classref *) iptr->target, 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

				a = 0;

			} else {
				a = (ptrint) iptr->val.a;
			}

			/* a0 = dimension count */

			M_MOV_IMM(iptr->op1, rd->argintregs[0]);

			/* a1 = arrayvftbl */

			M_MOV_IMM(iptr->val.a, rd->argintregs[1]);

			/* a2 = pointer to dimensions = stack pointer */

			M_MOV(REG_SP, rd->argintregs[2]);

			M_MOV_IMM(BUILTIN_multianewarray, REG_ITMP1);
			M_CALL(REG_ITMP1);

			/* check for exception before result assignment */

			M_TEST(REG_RESULT);
			M_BEQ(0);
			codegen_add_fillinstacktrace_ref(cd);

			s1 = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_RESULT);
			M_INTMOVE(REG_RESULT, s1);
			emit_store(jd, iptr, iptr->dst, s1);
			break;

		default:
			*exceptionptr = new_internalerror("Unknown ICMD %d", iptr->opc);
			return false;
	} /* switch */

	} /* for instruction */
		
	/* copy values to interface registers */

	src = bptr->outstack;
	len = bptr->outdepth;
	MCODECHECK(512);
#if defined(ENABLE_LSRA)
	if (!opt_lsra)
#endif
	while (src) {
		len--;
		if ((src->varkind != STACKVAR)) {
			s2 = src->type;
			if (IS_FLT_DBL_TYPE(s2)) {
				s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
				if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
					M_FLTMOVE(s1, rd->interfaces[len][s2].regoff);

				} else {
					emit_movq_reg_membase(cd, s1, REG_SP, rd->interfaces[len][s2].regoff * 8);
				}

			} else {
				s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
				if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
					M_INTMOVE(s1, rd->interfaces[len][s2].regoff);

				} else {
					emit_mov_reg_membase(cd, s1, REG_SP, rd->interfaces[len][s2].regoff * 8);
				}
			}
		}
		src = src->prev;
	}

	/* At the end of a basic block we may have to append some nops,
	   because the patcher stub calling code might be longer than the
	   actual instruction. So codepatching does not change the
	   following block unintentionally. */

	if (cd->mcodeptr < cd->lastmcodeptr) {
		while (cd->mcodeptr < cd->lastmcodeptr) {
			M_NOP;
		}
	}

	} /* if (bptr -> flags >= BBREACHED) */
	} /* for basic block */

	dseg_createlinenumbertable(cd);


	/* generate exception and patcher stubs */

	{
		exceptionref *eref;
		patchref     *pref;
		ptrint        mcode;
		u1           *savedmcodeptr;
		u1           *tmpmcodeptr;

		savedmcodeptr = NULL;

		/* generate exception stubs */
	
		for (eref = cd->exceptionrefs; eref != NULL; eref = eref->next) {
			gen_resolvebranch(cd->mcodebase + eref->branchpos, 
							  eref->branchpos,
							  cd->mcodeptr - cd->mcodebase);

			MCODECHECK(512);

			/* Check if the exception is an
			   ArrayIndexOutOfBoundsException.  If so, move index register
			   into a4. */

			if (eref->reg != -1)
				M_MOV(eref->reg, rd->argintregs[4]);

			/* calcuate exception address */

			M_MOV_IMM(0, rd->argintregs[3]);
			dseg_adddata(cd);
			M_AADD_IMM32(eref->branchpos - 6, rd->argintregs[3]);

			/* move function to call into REG_ITMP3 */

			M_MOV_IMM(eref->function, REG_ITMP3);

			if (savedmcodeptr != NULL) {
				M_JMP_IMM(savedmcodeptr - cd->mcodeptr - 5);
		
			} else {
				savedmcodeptr = cd->mcodeptr;

				emit_lea_membase_reg(cd, RIP, -(((ptrint) cd->mcodeptr + 7) - (ptrint) cd->mcodebase), rd->argintregs[0]);
				M_MOV(REG_SP, rd->argintregs[1]);
				M_ALD(rd->argintregs[2], REG_SP, stackframesize * 8);

				M_ASUB_IMM(2 * 8, REG_SP);
				M_AST(rd->argintregs[3], REG_SP, 0 * 8);         /* store XPC */

				M_CALL(REG_ITMP3);

				M_ALD(REG_ITMP2_XPC, REG_SP, 0 * 8);
				M_AADD_IMM(2 * 8, REG_SP);

				M_MOV_IMM(asm_handle_exception, REG_ITMP3);
				M_JMP(REG_ITMP3);
			}
		}


		/* generate code patching stub call code */

		for (pref = cd->patchrefs; pref != NULL; pref = pref->next) {
			/* check size of code segment */

			MCODECHECK(512);

			/* Get machine code which is patched back in later. A
			   `call rel32' is 5 bytes long (but read 8 bytes). */

			savedmcodeptr = cd->mcodebase + pref->branchpos;
			mcode = *((ptrint *) savedmcodeptr);

			/* patch in `call rel32' to call the following code */

			tmpmcodeptr  = cd->mcodeptr;    /* save current mcodeptr          */
			cd->mcodeptr = savedmcodeptr;   /* set mcodeptr to patch position */

			M_CALL_IMM(tmpmcodeptr - (savedmcodeptr + PATCHER_CALL_SIZE));

			cd->mcodeptr = tmpmcodeptr;     /* restore the current mcodeptr   */

			/* move pointer to java_objectheader onto stack */

#if defined(ENABLE_THREADS)
			/* create a virtual java_objectheader */

			(void) dseg_addaddress(cd, NULL);                         /* flcword    */
			(void) dseg_addaddress(cd, lock_get_initial_lock_word()); /* monitorPtr */
			a = dseg_addaddress(cd, NULL);                            /* vftbl      */

  			emit_lea_membase_reg(cd, RIP, -(((ptrint) cd->mcodeptr + 7) - (ptrint) cd->mcodebase) + a, REG_ITMP3);
			M_PUSH(REG_ITMP3);
#else
			M_PUSH_IMM(0);
#endif

			/* move machine code bytes and classinfo pointer into registers */

			M_MOV_IMM(mcode, REG_ITMP3);
			M_PUSH(REG_ITMP3);
			M_MOV_IMM(pref->ref, REG_ITMP3);
			M_PUSH(REG_ITMP3);
			M_MOV_IMM(pref->disp, REG_ITMP3);
			M_PUSH(REG_ITMP3);

			M_MOV_IMM(pref->patcher, REG_ITMP3);
			M_PUSH(REG_ITMP3);

			M_MOV_IMM(asm_wrapper_patcher, REG_ITMP3);
			M_JMP(REG_ITMP3);
		}
	}

	/* generate replacement-out stubs */

	{
		int i;

		replacementpoint = jd->code->rplpoints;

		for (i = 0; i < jd->code->rplpointcount; ++i, ++replacementpoint) {
			/* check code segment size */

			MCODECHECK(512);

			/* note start of stub code */

			replacementpoint->outcode = (u1*) (ptrint)(cd->mcodeptr - cd->mcodebase);

			/* make machine code for patching */

			disp = (ptrint)(replacementpoint->outcode - replacementpoint->pc) - 5;
			replacementpoint->mcode = 0xe9 | ((u8)disp << 8);

			/* push address of `rplpoint` struct */
			
			M_MOV_IMM(replacementpoint, REG_ITMP3);
			M_PUSH(REG_ITMP3);

			/* jump to replacement function */

			M_MOV_IMM(asm_replacement_out, REG_ITMP3);
			M_JMP(REG_ITMP3);
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
#define COMPILERSTUB_CODESIZE    7 + 7 + 3

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

	/* code for the stub */

	M_ALD(REG_ITMP1, RIP, -(7 * 1 + 2 * SIZEOF_VOID_P));       /* methodinfo  */
	M_ALD(REG_ITMP3, RIP, -(7 * 2 + 3 * SIZEOF_VOID_P));  /* compiler pointer */
	M_JMP(REG_ITMP3);

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
	s4            stackframesize;       /* size of stackframe if needed       */
	s4            nativeparams;
	s4            i, j;                 /* count variables                    */
	s4            t;
	s4            s1, s2;

	/* get required compiler data */

	m    = jd->m;
	code = jd->code;
	cd   = jd->cd;
	rd   = jd->rd;

	/* initialize variables */

	md = m->parseddesc;
	nativeparams = (m->flags & ACC_STATIC) ? 2 : 1;

	/* calculate stack frame size */

	stackframesize =
		sizeof(stackframeinfo) / SIZEOF_VOID_P +
		sizeof(localref_table) / SIZEOF_VOID_P +
		INT_ARG_CNT + FLT_ARG_CNT + 1 +         /* + 1 for function address   */
		nmd->memuse;

	if (!(stackframesize & 0x1))                /* keep stack 16-byte aligned */
		stackframesize++;

	/* create method header */

	(void) dseg_addaddress(cd, code);                      /* CodeinfoPointer */
	(void) dseg_adds4(cd, stackframesize * 8);             /* FrameSize       */
	(void) dseg_adds4(cd, 0);                              /* IsSync          */
	(void) dseg_adds4(cd, 0);                              /* IsLeaf          */
	(void) dseg_adds4(cd, 0);                              /* IntSave         */
	(void) dseg_adds4(cd, 0);                              /* FltSave         */
	(void) dseg_addlinenumbertablesize(cd);
	(void) dseg_adds4(cd, 0);                              /* ExTableSize     */

	/* generate native method profiling code */

	if (JITDATA_HAS_FLAG_INSTRUMENT(jd)) {
		/* count frequency */

		M_MOV_IMM(code, REG_ITMP3);
		M_IINC_MEMBASE(REG_ITMP3, OFFSET(codeinfo, frequency));
	}

	/* generate stub code */

	M_ASUB_IMM(stackframesize * 8, REG_SP);

#if !defined(NDEBUG)
	/* generate call trace */

	if (opt_verbosecall) {
		/* save integer and float argument registers */

		for (i = 0, j = 1; i < md->paramcount; i++) {
			if (!md->params[i].inmemory) {
				s1 = md->params[i].regoff;

				if (IS_INT_LNG_TYPE(md->paramtypes[i].type))
					M_LST(rd->argintregs[s1], REG_SP, j * 8);
				else
					M_DST(rd->argfltregs[s1], REG_SP, j * 8);

				j++;
			}
		}

		/* show integer hex code for float arguments */

		for (i = 0, j = 0; i < md->paramcount && j < INT_ARG_CNT; i++) {
			/* if the paramtype is a float, we have to right shift all
			   following integer registers */

			if (IS_FLT_DBL_TYPE(md->paramtypes[i].type)) {
				for (s1 = INT_ARG_CNT - 2; s1 >= i; s1--)
					M_MOV(rd->argintregs[s1], rd->argintregs[s1 + 1]);

				emit_movd_freg_reg(cd, rd->argfltregs[j], rd->argintregs[i]);
				j++;
			}
		}

		M_MOV_IMM(m, REG_ITMP1);
		M_AST(REG_ITMP1, REG_SP, 0 * 8);
		M_MOV_IMM(builtin_trace_args, REG_ITMP1);
		M_CALL(REG_ITMP1);

		/* restore integer and float argument registers */

		for (i = 0, j = 1; i < md->paramcount; i++) {
			if (!md->params[i].inmemory) {
				s1 = md->params[i].regoff;

				if (IS_INT_LNG_TYPE(md->paramtypes[i].type))
					M_LLD(rd->argintregs[s1], REG_SP, j * 8);
				else
					M_DLD(rd->argfltregs[s1], REG_SP, j * 8);

				j++;
			}
		}
	}
#endif /* !defined(NDEBUG) */

	/* get function address (this must happen before the stackframeinfo) */

#if !defined(WITH_STATIC_CLASSPATH)
	if (f == NULL) {
		codegen_addpatchref(cd, PATCHER_resolve_native, m, 0);

		if (opt_showdisassemble) {
			M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
		}
	}
#endif

	M_MOV_IMM(f, REG_ITMP3);


	/* save integer and float argument registers */

	for (i = 0, j = 0; i < md->paramcount; i++) {
		if (!md->params[i].inmemory) {
			s1 = md->params[i].regoff;

			if (IS_INT_LNG_TYPE(md->paramtypes[i].type))
				M_LST(rd->argintregs[s1], REG_SP, j * 8);
			else
				M_DST(rd->argfltregs[s1], REG_SP, j * 8);

			j++;
		}
	}

	M_AST(REG_ITMP3, REG_SP, (INT_ARG_CNT + FLT_ARG_CNT) * 8);

	/* create dynamic stack info */

	M_ALEA(REG_SP, stackframesize * 8, rd->argintregs[0]);
	emit_lea_membase_reg(cd, RIP, -(((ptrint) cd->mcodeptr + 7) - (ptrint) cd->mcodebase), rd->argintregs[1]);
	M_ALEA(REG_SP, stackframesize * 8 + SIZEOF_VOID_P, rd->argintregs[2]);
	M_ALD(rd->argintregs[3], REG_SP, stackframesize * 8);
	M_MOV_IMM(codegen_start_native_call, REG_ITMP1);
	M_CALL(REG_ITMP1);

	/* restore integer and float argument registers */

	for (i = 0, j = 0; i < md->paramcount; i++) {
		if (!md->params[i].inmemory) {
			s1 = md->params[i].regoff;

			if (IS_INT_LNG_TYPE(md->paramtypes[i].type))
				M_LLD(rd->argintregs[s1], REG_SP, j * 8);
			else
				M_DLD(rd->argfltregs[s1], REG_SP, j * 8);

			j++;
		}
	}

	M_ALD(REG_ITMP3, REG_SP, (INT_ARG_CNT + FLT_ARG_CNT) * 8);


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
				s1 = md->params[i].regoff + stackframesize + 1;   /* + 1 (RA) */
				s2 = nmd->params[j].regoff;
				M_LLD(REG_ITMP1, REG_SP, s1 * 8);
				M_LST(REG_ITMP1, REG_SP, s2 * 8);
			}

		} else {
			/* We only copy spilled float arguments, as the float argument    */
			/* registers keep unchanged.                                      */

			if (md->params[i].inmemory) {
				s1 = md->params[i].regoff + stackframesize + 1;   /* + 1 (RA) */
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

	if (m->flags & ACC_STATIC)
		M_MOV_IMM(m->class, rd->argintregs[1]);

	/* put env into first argument register */

	M_MOV_IMM(_Jv_env, rd->argintregs[0]);

	/* do the native function call */

	M_CALL(REG_ITMP3);

	/* save return value */

	if (md->returntype.type != TYPE_VOID) {
		if (IS_INT_LNG_TYPE(md->returntype.type))
			M_LST(REG_RESULT, REG_SP, 0 * 8);
		else
			M_DST(REG_FRESULT, REG_SP, 0 * 8);
	}

#if !defined(NDEBUG)
	/* generate call trace */

	if (opt_verbosecall) {
		/* just restore the value we need, don't care about the other */

		if (md->returntype.type != TYPE_VOID) {
			if (IS_INT_LNG_TYPE(md->returntype.type))
				M_LLD(REG_RESULT, REG_SP, 0 * 8);
			else
				M_DLD(REG_FRESULT, REG_SP, 0 * 8);
		}

  		M_MOV_IMM(m, rd->argintregs[0]);
  		M_MOV(REG_RESULT, rd->argintregs[1]);
		M_FLTMOVE(REG_FRESULT, rd->argfltregs[0]);
  		M_FLTMOVE(REG_FRESULT, rd->argfltregs[1]);

		M_MOV_IMM(builtin_displaymethodstop, REG_ITMP1);
		M_CALL(REG_ITMP1);
	}
#endif /* !defined(NDEBUG) */

	/* remove native stackframe info */

	M_ALEA(REG_SP, stackframesize * 8, rd->argintregs[0]);
	M_MOV_IMM(codegen_finish_native_call, REG_ITMP1);
	M_CALL(REG_ITMP1);
	M_MOV(REG_RESULT, REG_ITMP3);

	/* restore return value */

	if (md->returntype.type != TYPE_VOID) {
		if (IS_INT_LNG_TYPE(md->returntype.type))
			M_LLD(REG_RESULT, REG_SP, 0 * 8);
		else
			M_DLD(REG_FRESULT, REG_SP, 0 * 8);
	}

	/* remove stackframe */

	M_AADD_IMM(stackframesize * 8, REG_SP);

	/* test for exception */

	M_TEST(REG_ITMP3);
	M_BNE(1);
	M_RET;

	/* handle exception */

	M_MOV(REG_ITMP3, REG_ITMP1_XPTR);
	M_ALD(REG_ITMP2_XPC, REG_SP, 0 * 8);     /* get return address from stack */
	M_ASUB_IMM(3, REG_ITMP2_XPC);                                    /* callq */

	M_MOV_IMM(asm_handle_nat_exception, REG_ITMP3);
	M_JMP(REG_ITMP3);


	/* process patcher calls **************************************************/

	{
		patchref *pref;
		ptrint    mcode;
		u1       *savedmcodeptr;
		u1       *tmpmcodeptr;
#if defined(ENABLE_THREADS)
		s4        disp;
#endif

		for (pref = cd->patchrefs; pref != NULL; pref = pref->next) {
			/* Get machine code which is patched back in later. A
			   `call rel32' is 5 bytes long (but read 8 bytes). */

			savedmcodeptr = cd->mcodebase + pref->branchpos;
			mcode = *((ptrint *) savedmcodeptr);

			/* patch in `call rel32' to call the following code               */

			tmpmcodeptr  = cd->mcodeptr;    /* save current mcodeptr          */
			cd->mcodeptr = savedmcodeptr;   /* set mcodeptr to patch position */

			M_CALL_IMM(tmpmcodeptr - (savedmcodeptr + PATCHER_CALL_SIZE));

			cd->mcodeptr = tmpmcodeptr;     /* restore the current mcodeptr   */

			/* move pointer to java_objectheader onto stack */

#if defined(ENABLE_THREADS)
			/* create a virtual java_objectheader */

			(void) dseg_addaddress(cd, NULL);                         /* flcword    */
			(void) dseg_addaddress(cd, lock_get_initial_lock_word()); /* monitorPtr */
			disp = dseg_addaddress(cd, NULL);                         /* vftbl      */

  			emit_lea_membase_reg(cd, RIP, -(((ptrint) cd->mcodeptr + 7) - (ptrint) cd->mcodebase) + disp, REG_ITMP3);
			M_PUSH(REG_ITMP3);
#else
			M_PUSH_IMM(0);
#endif

			/* move machine code bytes and classinfo pointer into registers */

			M_MOV_IMM(mcode, REG_ITMP3);
			M_PUSH(REG_ITMP3);
			M_MOV_IMM(pref->ref, REG_ITMP3);
			M_PUSH(REG_ITMP3);
			M_MOV_IMM(pref->disp, REG_ITMP3);
			M_PUSH(REG_ITMP3);

			M_MOV_IMM(pref->patcher, REG_ITMP3);
			M_PUSH(REG_ITMP3);

			M_MOV_IMM(asm_wrapper_patcher, REG_ITMP3);
			M_JMP(REG_ITMP3);
		}
	}

	codegen_finish(jd);

	return jd->code->entrypoint;
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
