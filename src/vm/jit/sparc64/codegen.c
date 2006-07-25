/* src/vm/jit/sparc64/codegen.c - machine code generator for Sparc

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
            Alexander Jordan

   Changes:

   $Id: codegen.c 4644 2006-03-16 18:44:46Z edwin $

*/

#include <stdio.h>

#include "config.h"
#include "vm/types.h"

#include "md-abi.h"

/* #include "vm/jit/sparc64/arch.h" */
#include "vm/jit/sparc64/codegen.h"

#include "mm/memory.h"

#include "native/jni.h"
#include "native/native.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/codegen-common.h"
#include "vm/jit/dseg.h"
#include "vm/jit/emit.h"
#include "vm/jit/jit.h"
#include "vm/jit/parse.h"
#include "vm/jit/patcher.h"
#include "vm/jit/reg.h"


#define REG_PV (own_window?REG_PV_CALLEE:REG_PV_CALLER)

static int fabort(char *x)
{
    fprintf(stderr, "sparc64 abort because: %s\n", x);
    exit(1);
    abort();
    return 0;
			    
}


bool codegen(jitdata *jd)
{
	methodinfo         *m;
	codegendata        *cd;
	registerdata       *rd;
	s4                  len, s1, s2, s3, d, disp;
	s4                  stackframesize;
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
	rplpoint           *replacementpoint;

	bool				own_window = true; /* currently assumes immediate save*/
	
	/* get required compiler data */

	m  = jd->m;
	cd = jd->cd;
	rd = jd->rd;
	
	
	(void) dseg_addaddress(cd, m);                          /* MethodPointer  */
	(void) dseg_adds4(cd, stackframesize * 8);              /* FrameSize      */

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
	
	
	
	
	/* XXX create exception table */
	
	/* initialize mcode variables */
	mcodeptr = (s4 *) cd->mcodeptr;
	
	
	/* XXX stack setup */
	
	
	/* XXX copy arguments */
	
	
	/* XXX monitor enter and tracing */
	
	
	/* end of header generation */ 
	
	replacementpoint = jd->code->rplpoints;

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
		
		/* handle replacement points */

		if (bptr->bitflags & BBFLAG_REPLACEMENT) {
			replacementpoint->pc = (u1*)(ptrint)bptr->mpc; /* will be resolved later */
			
			replacementpoint++;
		}

		/* copy interface registers to their destination */

		src = bptr->instack;
		len = bptr->indepth;
		MCODECHECK(64+len);
		
#if defined(ENABLE_LSRA)
		if (opt_lsra) {
		while (src != NULL) {
			len--;
			if ((len == 0) && (bptr->type != BBTYPE_STD)) {
					/* 				d = reg_of_var(m, src, REG_ITMP1); */
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

				} else {
					d = codegen_reg_of_var(rd, 0, src, REG_IFTMP);
					if ((src->varkind != STACKVAR)) {
						s2 = src->type;
						if (IS_FLT_DBL_TYPE(s2)) {
							if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
								s1 = rd->interfaces[len][s2].regoff;
								M_DBLMOVE(s1, d);
							} else {
								M_DLD(d, REG_SP, rd->interfaces[len][s2].regoff * 8);
							}
							emit_store(jd, NULL, src, d);

						} else {
							if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
								s1 = rd->interfaces[len][s2].regoff;
								M_INTMOVE(s1, d);
							} else {
								M_LDX(d, REG_SP, rd->interfaces[len][s2].regoff * 8);
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

		for (iptr = bptr->iinstr; len > 0; src = iptr->dst, len--, iptr++) {
			if (iptr->line != currentline) {
				dseg_addlinenumber(cd, iptr->line);
				currentline = iptr->line;
			}

		MCODECHECK(64);       /* an instruction usually needs < 64 words      */
		switch (iptr->opc) {

		case ICMD_INLINE_START:
		case ICMD_INLINE_END:
			break;

		case ICMD_NOP:        /* ...  ==> ...                                 */
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
			M_FLD(d, REG_PV, disp);
			emit_store(jd, iptr, iptr->dst, d);
			break;
			
		case ICMD_DCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.d = constant                    */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
			disp = dseg_adddouble(cd, iptr->val.d);
			M_DLD(d, REG_PV, disp);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_ACONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.a = constant                    */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);

			if ((iptr->target != NULL) && (iptr->val.a == NULL)) {
				disp = dseg_addaddress(cd, iptr->val.a);

				codegen_addpatchref(cd, PATCHER_aconst,
									(unresolved_class *) iptr->target, disp);

				if (opt_showdisassemble) {
					M_NOP;
				}

				M_ALD(REG_PV, disp, d);

			} else {
				if (iptr->val.a == NULL) {
					M_INTMOVE(REG_ZERO, d);
				} else {
					disp = dseg_addaddress(cd, iptr->val.a);
					M_ALD(d, REG_PV, disp);
				}
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;


		/* load/store operations **********************************************/

		case ICMD_ILOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_LLOAD:      /* op1 = local variable                         */
		case ICMD_ALOAD:

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY) {
				M_ALD(REG_SP, var->regoff * 8, d);
			} else {
				M_INTMOVE(var->regoff, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;
	
		case ICMD_FLOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_DLOAD:      /* op1 = local variable                         */

			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY) {
				M_DLD(d, REG_SP, var->regoff * 8);
			} else {
				M_FLTMOVE(var->regoff, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;


		case ICMD_ISTORE:     /* ..., value  ==> ...                          */
		case ICMD_LSTORE:     /* op1 = local variable                         */
		case ICMD_ASTORE:

			if ((src->varkind == LOCALVAR) &&
			    (src->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
				M_STX(s1, REG_SP, var->regoff * 8);
			} else {
				s1 = emit_load_s1(jd, iptr, src, var->regoff);
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
		case ICMD_LNEG:

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_SUB(REG_ZERO, s1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_I2L:        /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_INTMOVE(s1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_SRA_IMM(s1, 0, d); /* sign extend upper 32 bits */
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_SLLX_IMM(s1, 56, d);
			M_SRAX_IMM( d, 56, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_INT2CHAR:   /* ..., value  ==> ..., value                   */
		case ICMD_INT2SHORT:

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_SLLX_IMM(s1, 48, d);
			M_SRAX_IMM( d, 48, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IADD:       /* ..., val1, val2  ==> ..., val1 + val2        */
		case ICMD_LADD:

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_ADD(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if ((iptr->val.i >= -4096) && (iptr->val.i <= 4095)) {
				M_ADD_IMM(s1, iptr->val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_ADD(s1, REG_ITMP2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if ((iptr->val.l >= -4096) && (iptr->val.l <= 4095)) {
				M_ADD_IMM(s1, iptr->val.l, d);
			} else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_ADD(s1, REG_ITMP2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_ISUB:       /* ..., val1, val2  ==> ..., val1 - val2        */
		case ICMD_LSUB: 

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_SUB(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if ((iptr->val.i >= -4096) && (iptr->val.i <= 4095)) {
				M_SUB_IMM(s1, iptr->val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_SUB(s1, REG_ITMP2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if ((iptr->val.l >= -4096) && (iptr->val.l <= 4095)) {
				M_SUB_IMM(s1, iptr->val.l, d);
			} else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_SUB(s1, REG_ITMP2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */
		case ICMD_LMUL:

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_MULX(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if ((iptr->val.i >= -4096) && (iptr->val.i <= 4095)) {
				M_MULX_IMM(s1, iptr->val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_MULX(s1, REG_ITMP2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if ((iptr->val.l >= -4096) && (iptr->val.l <= 4095)) {
				M_MULX_IMM(s1, iptr->val.l, d);
			} else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_MULX(s1, REG_ITMP2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */
/* XXX could also clear Y and use 32bit div */
			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			gen_div_check(s2);
			M_ISEXT(s1, s1);
			/* XXX trim s2 like s1 ? */
			M_DIVX(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			gen_div_check(s2);
			M_DIVX(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			gen_div_check(s2);
			M_ISEXT(s1, s1);
			/* XXX trim s2 like s1 ? */
			M_DIVX(s1, s2, d);
			M_MULX(s2, d, d);
			M_SUB(s1, d, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			gen_div_check(s2);
			M_DIVX(s1, s2, d);
			M_MULX(s2, d, d);
			M_SUB(s1, d, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IDIVPOW2:   /* ..., value  ==> ..., value << constant       */
		case ICMD_LDIVPOW2:   /* val.i = constant                             */
		                      
			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_SRAX_IMM(s1, 63, REG_ITMP2);
			M_SRLX_IMM(REG_ITMP2, 64 - iptr->val.i, REG_ITMP2);
			M_ADD(s1, REG_ITMP2, REG_ITMP2);
			M_SRAX_IMM(REG_ITMP2, iptr->val.i, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_ISHL:       /* ..., val1, val2  ==> ..., val1 << val2       */
		case ICMD_LSHL:

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_SLLX(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		case ICMD_LSHLCONST:  /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_SLLX_IMM(s1, iptr->val.i, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_SRA(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_SRA_IMM(s1, iptr->val.i, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_SRL(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_SRL_IMM(s1, iptr->val.i, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_SRAX(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_SRAX_IMM(s1, iptr->val.i, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_SRLX(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_SRLX_IMM(s1, iptr->val.i, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */
		case ICMD_LAND:

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
			if ((iptr->val.i >= -4096) && (iptr->val.i <= 4095)) {
				M_AND_IMM(s1, iptr->val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_AND(s1, REG_ITMP2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_ISEXT(s1, s1); /* trim for 32-bit compare (BGEZ) */
			if (s1 == d) {
				M_MOV(s1, REG_ITMP1);
				s1 = REG_ITMP1;
			}
			if ((iptr->val.i >= 0) && (iptr->val.i <= 0xffff)) {
				M_AND_IMM(s1, iptr->val.i, d);
				M_BGEZ(s1, 4);
				M_NOP;
				M_SUB(REG_ZERO, s1, d);
				M_AND_IMM(d, iptr->val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_AND(s1, REG_ITMP2, d);
				M_BGEZ(s1, 4);
				M_NOP;
				M_SUB(REG_ZERO, s1, d);
				M_AND(d, REG_ITMP2, d);
			}
			M_SUB(REG_ZERO, d, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if ((iptr->val.l >= -4096) && (iptr->val.l <= 4095)) {
				M_AND_IMM(s1, iptr->val.l, d);
			} else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_AND(s1, REG_ITMP2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if (s1 == d) {
				M_MOV(s1, REG_ITMP1);
				s1 = REG_ITMP1;
			}
			if ((iptr->val.l >= -4096) && (iptr->val.l <= 4095)) {
				M_AND_IMM(s1, iptr->val.l, d);
				M_BGEZ(s1, 4);
				M_NOP;
				M_SUB(REG_ZERO, s1, d);
				M_AND_IMM(d, iptr->val.l, d);
			} else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_AND(s1, REG_ITMP2, d);
				M_BGEZ(s1, 4);
				M_NOP;
				M_SUB(REG_ZERO, s1, d);
				M_AND(d, REG_ITMP2, d);
			}
			M_SUB(REG_ZERO, d, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IOR:        /* ..., val1, val2  ==> ..., val1 | val2        */
		case ICMD_LOR:

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_OR(s1,s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if ((iptr->val.i >= -4096) && (iptr->val.i <= 4095)) {
				M_OR_IMM(s1, iptr->val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_OR(s1, REG_ITMP2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if ((iptr->val.l >= -4096) && (iptr->val.l <= 4095)) {
				M_OR_IMM(s1, iptr->val.l, d);
			} else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_OR(s1, REG_ITMP2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_IXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */
		case ICMD_LXOR:

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
			if ((iptr->val.i >= -4096) && (iptr->val.i <= 4095)) {
				M_XOR_IMM(s1, iptr->val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_XOR(s1, REG_ITMP2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if ((iptr->val.l >= -4096) && (iptr->val.l <= 4095)) {
				M_XOR_IMM(s1, iptr->val.l, d);
			} else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_XOR(s1, REG_ITMP2, d);
			}
			emit_store(jd, iptr, iptr->dst, d);
			break;


		case ICMD_LCMP:       /* ..., val1, val2  ==> ..., val1 cmp val2      */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			M_CMP(s1, s2);
			M_MOV(REG_ZERO, d);
			M_XCMOVLT_IMM(-1, d);
			M_XCMOVGT_IMM(1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;


		case ICMD_IINC:       /* ..., value  ==> ..., value + constant        */
		                      /* op1 = variable, val.i = constant             */

			var = &(rd->locals[iptr->op1][TYPE_INT]);
			if (var->flags & INMEMORY) {
				s1 = REG_ITMP1;
				M_LDX(s1, REG_SP, var->regoff * 8);
			} else
				s1 = var->regoff;
			if ((iptr->val.i >= -4096) && (iptr->val.i <= 4095)) {
				M_ADD_IMM(s1, iptr->val.i, s1);
			} else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_ADD(s1, REG_ITMP2, s1);
			}
			if (var->flags & INMEMORY)
				M_STX(s1, REG_SP, var->regoff * 8);
			break;


		/* floating operations ************************************************/

		case ICMD_FNEG:       /* ..., value  ==> ..., - value                 */

			s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP2);
			M_FNEG(s1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */

			s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP2);
			M_DNEG(s1, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP2);
			M_FADD(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_DADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP2);
			M_DADD(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_FSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP2);
			M_FSUB(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_DSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP2);
			M_DSUB(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_FMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP2);
			M_FMUL(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_DMUL:       /* ..., val1, val2  ==> ..., val1 *** val2      */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP2);
			M_DMUL(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_FDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP2);
			M_FDIV(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_DDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP2);
			M_DDIV(s1, s2, d);
			emit_store(jd, iptr, iptr->dst, d);
			break;	

		case ICMD_I2F:
			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			disp = dseg_addfloat(cd, 0.0);
			M_IST (s1, REG_PV_CALLEE, disp);
			M_FLD (d, REG_PV_CALLEE, disp);
			M_CVTIF (d, d); /* rd gets translated to double target register */
			emit_store(jd, iptr, iptr->dst, d);
			break;
			
		case ICMD_I2D:
			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP3);
			disp = dseg_adddouble(cd, 0.0);
			M_STX (s1, REG_PV_CALLEE, disp);
			M_DLD (REG_FTMP2, REG_PV_CALLEE, disp); /* REG_FTMP2 needs to be a double temp */
			M_CVTLF (REG_FTMP2, d); /* rd gets translated to double target register */
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_F2I:       /* ..., value  ==> ..., (int) value              */
			s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			disp = dseg_addfloat(cd, 0.0);
			M_CVTFI(s1, REG_FTMP2);
			M_FST(REG_FTMP2, REG_PV_CALLEE, disp);
			M_ILD(d, REG_PV, disp);
			emit_store(jd, iptr, iptr->dst, d);
			break;
			
			       
		case ICMD_D2I:       /* ..., value  ==> ..., (int) value             */
			s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			disp = dseg_addfloat(cd, 0.0);
			M_CVTDI(s1, REG_FTMP2);
			M_FST(REG_FTMP2, REG_PV, disp);
			M_ILD(d, REG_PV, disp);
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_F2L:       /* ..., value  ==> ..., (long) value             */
			s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			disp = dseg_adddouble(cd, 0.0);
			M_CVTFL(s1, REG_FTMP2); /* FTMP2 needs to be double reg */
			M_DST(REG_FTMP2, REG_PV, disp);
			M_LDX(d, REG_PV, disp);
			emit_store(jd, iptr, iptr->dst, d);
			break;
			
		case ICMD_D2L:       /* ..., value  ==> ..., (long) value             */
			s1 = emit_load_s1(jd, iptr, src, REG_FTMP1);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			disp = dseg_adddouble(cd, 0.0);
			M_CVTDL(s1, REG_FTMP2); /* FTMP2 needs to be double reg */
			M_DST(REG_FTMP2, REG_PV, disp);
			M_LDX(d, REG_PV, disp);
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
	
	/* XXX merge F/D versions? only compare instr. is different */
		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			M_FCMP(s1,s2);
			M_OR_IMM(REG_ZERO, -1, REG_ITMP3); /* less by default (less or unordered) */
			M_CMOVFEQ_IMM(0, REG_ITMP3); /* 0 if equal */
			M_CMOVFGT_IMM(1, REG_ITMP3); /* 1 if greater */
			emit_store(jd, iptr, iptr->dst, d);
			break;
			
		case ICMD_DCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);
			M_DCMP(s1,s2);
			M_OR_IMM(REG_ZERO, -1, REG_ITMP3); /* less by default (less or unordered) */
			M_CMOVFEQ_IMM(0, REG_ITMP3); /* 0 if equal */
			M_CMOVFGT_IMM(1, REG_ITMP3); /* 1 if greater */
			emit_store(jd, iptr, iptr->dst, d);
			break;
			
		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);	
			M_FCMP(s1,s2);
			M_OR_IMM(REG_ZERO, 1, REG_ITMP3); /* greater by default (greater or unordered) */
			M_CMOVFEQ_IMM(0, REG_ITMP3); /* 0 if equal */
			M_CMOVFLT_IMM(-1, REG_ITMP3); /* -1 if less */
			emit_store(jd, iptr, iptr->dst, d);
			break;
			
		case ICMD_DCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_FTMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP3);	
			M_DCMP(s1,s2);
			M_OR_IMM(REG_ZERO, 1, REG_ITMP3); /* greater by default (greater or unordered) */
			M_CMOVFEQ_IMM(0, REG_ITMP3); /* 0 if equal */
			M_CMOVFLT_IMM(-1, REG_ITMP3); /* -1 if less */
			emit_store(jd, iptr, iptr->dst, d);
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
			M_AADD(s2, s1, REG_ITMP3);
			M_BLDS(d, REG_ITMP3, OFFSET(java_chararray, data[0]));
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
			M_AADD(s2, s1, REG_ITMP3);
			M_AADD(s2, REG_ITMP3, REG_ITMP3);
			M_SLDU(d, REG_ITMP3, OFFSET(java_chararray, data[0]));
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
			M_AADD(s2, s1, REG_ITMP3);
			M_AADD(s2, REG_ITMP3, REG_ITMP3);
			M_SLDS(d, REG_ITMP3, OFFSET(java_chararray, data[0]));
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
			M_ASLL_IMM(s2, 2, REG_ITMP3);
			M_AADD(REG_ITMP3, s1, REG_ITMP3);
			M_ILD(d, REG_ITMP3, OFFSET(java_intarray, data[0]));
			emit_store(jd, iptr, iptr->dst, d);
			break;

		case ICMD_LALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 3, REG_ITMP3);
			M_AADD(REG_ITMP3, s1, REG_ITMP3);
			M_LDX(d, REG_ITMP3, OFFSET(java_longarray, data[0]));
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
			M_ASLL_IMM(s2, 2, REG_ITMP3);
			M_AADD(REG_ITMP3, s1, REG_ITMP3);
			M_FLD(d, REG_ITMP3, OFFSET(java_floatarray, data[0]));
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
			M_ASLL_IMM(s2, 3, REG_ITMP3);
			M_AADD(REG_ITMP3, s1, REG_ITMP3);
			M_DLD(d, REG_ITMP3, OFFSET(java_doublearray, data[0]));
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
			M_ASLL_IMM(s2, POINTERSHIFT, REG_ITMP3);
			M_AADD(REG_ITMP3, s1, REG_ITMP3);
			M_ALD(d, REG_ITMP3, OFFSET(java_objectarray, data[0]));
			emit_store(jd, iptr, iptr->dst, d);
			break;

	
		case ICMD_BASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, src->prev->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_AADD(s2, s1, REG_ITMP1);
			s3 = emit_load_s3(jd, iptr, src, REG_ITMP3);
			M_BST(s3, REG_ITMP1, OFFSET(java_bytearray, data[0]));
			break;

		case ICMD_CASTORE:    /* ..., arrayref, index, value  ==> ...         */
		case ICMD_SASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, src->prev->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_AADD(s2, s1, REG_ITMP1);
			M_AADD(s2, REG_ITMP1, REG_ITMP1);
			s3 = emit_load_s3(jd, iptr, src, REG_ITMP3);
			M_SST(s3, REG_ITMP1, OFFSET(java_chararray, data[0]));
			break;

		case ICMD_IASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, src->prev->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 2, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			s3 = emit_load_s3(jd, iptr, src, REG_ITMP3);
			M_IST_INTERN(s3, REG_ITMP1, OFFSET(java_intarray, data[0]));
			break;

		case ICMD_LASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, src->prev->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 3, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			s3 = emit_load_s3(jd, iptr, src, REG_ITMP3);
			M_STX_INTERN(s3, REG_ITMP1, OFFSET(java_longarray, data[0]));
			break;

		case ICMD_FASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, src->prev->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 2, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			s3 = emit_load_s3(jd, iptr, src, REG_FTMP1);
			M_FST_INTERN(s3, REG_ITMP1, OFFSET(java_floatarray, data[0]));
			break;

		case ICMD_DASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, src->prev->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 3, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			s3 = emit_load_s3(jd, iptr, src, REG_FTMP1);
			M_DST_INTERN(s3, REG_ITMP1, OFFSET(java_doublearray, data[0]));
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
			disp = dseg_addaddress(cd, BUILTIN_canstore);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_JMP(REG_RA_CALLER, REG_ITMP3, REG_ZERO);
			M_NOP;

			M_BEQZ(REG_RESULT_CALLER, 0);
			codegen_add_arraystoreexception_ref(cd);
			M_NOP;

			s1 = emit_load_s1(jd, iptr, src->prev->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src->prev, REG_ITMP2);
			M_ASLL_IMM(s2, POINTERSHIFT, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			s3 = emit_load_s3(jd, iptr, src, REG_ITMP3);
			M_AST_INTERN(s3, REG_ITMP1, OFFSET(java_objectarray, data[0]));
			break;


		case ICMD_BASTORECONST:   /* ..., arrayref, index  ==> ...            */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_AADD(s2, s1, REG_ITMP1);
			M_BST(REG_ZERO, REG_ITMP1, OFFSET(java_bytearray, data[0]));
			break;

		case ICMD_CASTORECONST:   /* ..., arrayref, index  ==> ...            */
		case ICMD_SASTORECONST:   /* ..., arrayref, index  ==> ...            */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_AADD(s2, s1, REG_ITMP1);
			M_AADD(s2, REG_ITMP1, REG_ITMP1);
			M_SST(REG_ZERO, REG_ITMP1, OFFSET(java_chararray, data[0]));
			break;

		case ICMD_IASTORECONST:   /* ..., arrayref, index  ==> ...            */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 2, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_IST_INTERN(REG_ZERO, REG_ITMP1, OFFSET(java_intarray, data[0]));
			break;

		case ICMD_LASTORECONST:   /* ..., arrayref, index  ==> ...            */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 3, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_STX_INTERN(REG_ZERO, REG_ITMP1, OFFSET(java_longarray, data[0]));
			break;

		case ICMD_AASTORECONST:   /* ..., arrayref, index  ==> ...            */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, POINTERSHIFT, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_AST_INTERN(REG_ZERO, REG_ITMP1, OFFSET(java_objectarray, data[0]));
			break;
		

		case ICMD_GETSTATIC:  /* ...  ==> ..., value                          */
		                      /* op1 = type, val.a = field address            */

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				disp = dseg_addaddress(cd, NULL);

				codegen_addpatchref(cd, PATCHER_get_putstatic,
									INSTRUCTION_UNRESOLVED_FIELD(iptr), disp);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

			} else {
				fieldinfo *fi = INSTRUCTION_RESOLVED_FIELDINFO(iptr);

				disp = dseg_addaddress(cd, &(fi->value));

				if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					codegen_addpatchref(cd, PATCHER_clinit, fi->class, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP;
					}
				}
  			}

			M_ALD(REG_ITMP1, REG_PV, disp);
			switch (iptr->op1) {
			case TYPE_INT:
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
				M_ILD_INTERN(d, REG_ITMP1, 0);
				emit_store(jd, iptr, iptr->dst, d);
				break;
			case TYPE_LNG:
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
				M_LDX_INTERN(d, REG_ITMP1, 0);
				emit_store(jd, iptr, iptr->dst, d);
				break;
			case TYPE_ADR:
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
				M_ALD_INTERN(d, REG_ITMP1, 0);
				emit_store(jd, iptr, iptr->dst, d);
				break;
			case TYPE_FLT:
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
				M_FLD_INTERN(d, REG_ITMP1, 0);
				emit_store(jd, iptr, iptr->dst, d);
				break;
			case TYPE_DBL:				
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
				M_DLD_INTERN(d, REG_ITMP1, 0);
				emit_store(jd, iptr, iptr->dst, d);
				break;
			}
			break;

		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */
		                      /* op1 = type, val.a = field address            */

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				disp = dseg_addaddress(cd, NULL);

				codegen_addpatchref(cd, PATCHER_get_putstatic,
									INSTRUCTION_UNRESOLVED_FIELD(iptr), disp);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

			} else {
				fieldinfo *fi = INSTRUCTION_RESOLVED_FIELDINFO(iptr);

				disp = dseg_addaddress(cd, &(fi->value));

				if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					codegen_addpatchref(cd, PATCHER_clinit, fi->class, 0);

					if (opt_showdisassemble) {
						M_NOP; M_NOP;
					}
				}
  			}

			M_ALD(REG_ITMP1, REG_PV, disp);
			switch (iptr->op1) {
			case TYPE_INT:
				s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
				M_IST_INTERN(s2, REG_ITMP1, 0);
				break;
			case TYPE_LNG:
				s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
				M_STX_INTERN(s2, REG_ITMP1, 0);
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

		case ICMD_PUTSTATICCONST: /* ...  ==> ...                             */
		                          /* val = value (in current instruction)     */
		                          /* op1 = type, val.a = field address (in    */
		                          /* following NOP)                           */

			if (INSTRUCTION_IS_UNRESOLVED(iptr + 1)) {
				disp = dseg_addaddress(cd, NULL);

				codegen_addpatchref(cd, PATCHER_get_putstatic,
									INSTRUCTION_UNRESOLVED_FIELD(iptr + 1), disp);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

			} else {
				fieldinfo *fi = INSTRUCTION_RESOLVED_FIELDINFO(iptr + 1);

				disp = dseg_addaddress(cd, &(fi->value));

				if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					codegen_addpatchref(cd, PATCHER_clinit, fi->class, 0);

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
				M_STX_INTERN(REG_ZERO, REG_ITMP1, 0);
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

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			gen_nullptr_check(s1);

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				codegen_addpatchref(cd, PATCHER_get_putfield,
									INSTRUCTION_UNRESOLVED_FIELD(iptr), 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

				disp = 0;

			} else {
				disp = INSTRUCTION_RESOLVED_FIELDINFO(iptr)->offset;
			}

			switch (iptr->op1) {
			case TYPE_INT:
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
				M_ILD(d, s1, disp);
				emit_store(jd, iptr, iptr->dst, d);
				break;
			case TYPE_LNG:
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
				M_LDX(d, s1, disp);
				emit_store(jd, iptr, iptr->dst, d);
				break;
			case TYPE_ADR:
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_ITMP2);
				M_ALD(d, s1, disp);
				emit_store(jd, iptr, iptr->dst, d);
				break;
			case TYPE_FLT:
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
				M_FLD(d, s1, disp);
				emit_store(jd, iptr, iptr->dst, d);
				break;
			case TYPE_DBL:				
				d = codegen_reg_of_var(rd, iptr->opc, iptr->dst, REG_FTMP1);
				M_DLD(d, s1, disp);
				emit_store(jd, iptr, iptr->dst, d);
				break;
			}
			break;

		case ICMD_PUTFIELD:   /* ..., objectref, value  ==> ...               */
		                      /* op1 = type, val.a = field address            */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP2);
			gen_nullptr_check(s1);

			/*if (!IS_FLT_DBL_TYPE(iptr->op1)) {
				s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			} else {*/
				s2 = emit_load_s2(jd, iptr, src, REG_IFTMP);
			/*}*/

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				codegen_addpatchref(cd, PATCHER_get_putfield,
									INSTRUCTION_UNRESOLVED_FIELD(iptr), 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

				disp = 0;

			} else {
				disp = INSTRUCTION_RESOLVED_FIELDINFO(iptr)->offset;
			}

			switch (iptr->op1) {
			case TYPE_INT:
				M_IST(s2, s1, disp);
				break;
			case TYPE_LNG:
				M_STX(s2, s1, disp);
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

		case ICMD_PUTFIELDCONST:  /* ..., objectref  ==> ...                  */
		                          /* val = value (in current instruction)     */
		                          /* op1 = type, val.a = field address (in    */
		                          /* following NOP)                           */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			gen_nullptr_check(s1);

			if (INSTRUCTION_IS_UNRESOLVED(iptr + 1)) {
				codegen_addpatchref(cd, PATCHER_get_putfield,
									INSTRUCTION_UNRESOLVED_FIELD(iptr + 1), 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

				disp = 0;

			} else {
				disp = INSTRUCTION_RESOLVED_FIELDINFO(iptr + 1)->offset;
			}

			switch (iptr[1].op1) {
			case TYPE_INT:
				M_IST(REG_ZERO, s1, disp);
				break;
			case TYPE_LNG:
				M_STX(REG_ZERO, s1, disp);
				break;
			case TYPE_ADR:
				M_AST(REG_ZERO, s1, disp);
				break;
			case TYPE_FLT:
				M_FST(REG_ZERO, s1, disp);
				break;
			case TYPE_DBL:
				M_DST(REG_ZERO, s1, disp);
				break;
			}
			break;


		/* branch operations **************************************************/

		case ICMD_ATHROW:       /* ..., objectref ==> ... (, objectref)       */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			M_INTMOVE(s1, REG_ITMP2_XPTR);

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
			M_JMP(REG_ITMP3_XPC, REG_ITMP2, REG_ZERO);
			M_NOP;
			M_NOP;              /* nop ensures that XPC is less than the end */
			                    /* of basic block                            */
			ALIGNCODENOP;
			break;

		case ICMD_GOTO:         /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */
			M_BR(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_NOP;
			ALIGNCODENOP;
			break;

		case ICMD_JSR:          /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */

			dseg_addtarget(cd, (basicblock *) iptr->target);
			M_ALD(REG_ITMP1, REG_PV, -(cd->dseglen));
			M_JMP(REG_ITMP1, REG_ITMP1, REG_ZERO);        /* REG_ITMP1 = return address */
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

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			M_BEQZ(s1, 0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_NOP;
			break;

		case ICMD_IFNONNULL:    /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			M_BNEZ(s1, 0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_NOP;
			break;
/* XXX: CMP_IMM */
		case ICMD_IFEQ:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BEQZ(s1, 0);
			} else {
				if ((iptr->val.i >= -4096) && (iptr->val.i <= 4095)) {
					M_CMP_IMM(s1, iptr->val.i);
					}
				else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMP(s1, REG_ITMP2);
					}
				M_BEQ(0);
				}
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_NOP;
			break;

		case ICMD_IFLT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BLTZ(s1, 0);
			} else {
				if ((iptr->val.i >= -4096) && (iptr->val.i <= 4095)) {
					M_CMP_IMM(s1, iptr->val.i);
				} else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMP(s1, REG_ITMP2);
				}
				M_BLT(0);
			}
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_NOP;
			break;

		case ICMD_IFLE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BLEZ(s1, 0);
				}
			else {
				if ((iptr->val.i >= -4096) && (iptr->val.i <= 4095)) {
					M_CMP_IMM(s1, iptr->val.i);
					}
				else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMP(s1, REG_ITMP2);
				}
				M_BLE(0);
			}
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_NOP;
			break;

		case ICMD_IFNE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			s1 = emit_load_s1(jd, iptr, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BNEZ(s1, 0);
				}
			else {
				if ((iptr->val.i >= -4096) && (iptr->val.i <= 4095)) {
					M_CMP_IMM(s1, iptr->val.i);
				}
				else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMP(s1, REG_ITMP2);
				}
				M_BNE(0);
			}
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_NOP;
			break;

		case ICMD_IF_ACMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPEQ:    /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_XBEQ(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_NOP;
			break;

		case ICMD_IF_ICMPEQ:    /* 32-bit compare                             */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BEQ(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_NOP;
			break;

		case ICMD_IF_ACMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPNE:    /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_XBNE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_NOP;
			break;
			
		case ICMD_IF_ICMPNE:    /* 32-bit compare                             */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BNE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_NOP;
			break;

		case ICMD_IF_LCMPLT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_XBLT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_NOP;
			break;
			
		case ICMD_IF_ICMPLT:    /* 32-bit compare                             */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BLT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_NOP;
			break;

		case ICMD_IF_LCMPGT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_XBGT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_NOP;
			break;
			
		case ICMD_IF_ICMPGT:    /* 32-bit compare                             */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BGT(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_NOP;
			break;

		case ICMD_IF_LCMPLE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BLE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_NOP;
			break;
			
		case ICMD_IF_ICMPLE:    /* 32-bit compare                             */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BLE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_NOP;
			break;			
	

		case ICMD_IF_LCMPGE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BGE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_NOP;
			break;
			
		case ICMD_IF_ICMPGE:    /* 32-bit compare                             */

			s1 = emit_load_s1(jd, iptr, src->prev, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BGE(0);
			codegen_addreference(cd, (basicblock *) iptr->target);
			M_NOP;
			break;


		case ICMD_IRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_LRETURN:

			s1 = emit_load_s1(jd, iptr, src, REG_RESULT_CALLEE);
			M_INTMOVE(s1, REG_RESULT_CALLEE);
			goto nowperformreturn;

		case ICMD_ARETURN:      /* ..., retvalue ==> ...                      */

			s1 = emit_load_s1(jd, iptr, src, REG_RESULT_CALLEE);
			M_INTMOVE(s1, REG_RESULT_CALLEE);

#ifdef ENABLE_VERIFIER
			if (iptr->val.a) {
				codegen_addpatchref(cd, PATCHER_athrow_areturn,
									(unresolved_class *) iptr->val.a, 0);

				if (opt_showdisassemble)
					M_NOP;
			}
#endif /* ENABLE_VERIFIER */
			goto nowperformreturn;

		case ICMD_FRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_DRETURN:

			s1 = emit_load_s1(jd, iptr, src, REG_FRESULT);
			M_FLTMOVE(s1, REG_FRESULT);
			goto nowperformreturn;

		case ICMD_RETURN:       /* ...  ==> ...                               */

nowperformreturn:
			{
			s4 i, p;
			
			p = stackframesize;
			
			/* call trace function */

#if !defined(NDEBUG)
			if (opt_verbosecall) {
				M_LDA(REG_SP, REG_SP, -3 * 8);
				M_AST(REG_RA_CALLEE, REG_SP, 0 * 8); /* XXX: no need to save anything but FRES ? */
		/*		M_STX(REG_RESULT, REG_SP, 1 * 8); */
				M_DST(REG_FRESULT, REG_SP, 2 * 8);

				disp = dseg_addaddress(cd, m);
				M_ALD(rd->argintregs[0], REG_PV, disp);
				M_MOV(REG_RESULT_CALLEE, rd->argintregs[1]);
				M_FLTMOVE(REG_FRESULT, rd->argfltregs[2]);
				M_FLTMOVE(REG_FRESULT, rd->argfltregs[3]);

				disp = dseg_addaddress(cd, (void *) builtin_displaymethodstop);
				M_ALD(REG_ITMP3, REG_PV, disp);
				M_JMP(REG_RA_CALLER, REG_ITMP3, REG_ZERO);
				M_NOP;

				M_DLD(REG_FRESULT, REG_SP, 2 * 8);
		/*		M_LDX(REG_RESULT, REG_SP, 1 * 8); */
				M_ALD(REG_RA_CALLEE, REG_SP, 0 * 8);
				M_LDA(REG_SP, REG_SP, 3 * 8);
			}
#endif

#if defined(ENABLE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
/* XXX: REG_RESULT is save, but what about FRESULT? */
				M_ALD(rd->argintregs[0], REG_SP, rd->memuse * 8); /* XXX: what for ? */

				switch (iptr->opc) {
				case ICMD_FRETURN:
				case ICMD_DRETURN:
					M_DST(REG_FRESULT, REG_SP, rd->memuse * 8);
					break;
				}

				disp = dseg_addaddress(cd, BUILTIN_monitorexit);
				M_ALD(REG_ITMP3, REG_PV, disp);
				M_JMP(REG_RA_CALLER, REG_ITMP3, REG_ZERO); /*REG_RA_CALLER */

				switch (iptr->opc) {
				case ICMD_FRETURN:
				case ICMD_DRETURN:
					M_DLD(REG_FRESULT, REG_SP, rd->memuse * 8);
					break;
				}
			}
#endif



			M_RETURN(REG_RA_CALLEE); /* implicit window restore */
			M_NOP;
			ALIGNCODENOP;
			}
			break;


		default:
			*exceptionptr = new_internalerror("Unknown ICMD %d", iptr->opc);
			return false;
			
	} /* switch */
		
	} /* for instruction */
	
	
	/* copy values to interface registers */

	src = bptr->outstack;
	len = bptr->outdepth;
	MCODECHECK(64+len);
#if defined(ENABLE_LSRA)
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
					M_STX(s1, REG_SP, 8 * rd->interfaces[len][s2].regoff);
					}
				}
			}
		src = src->prev;
		}
	} /* if (bptr -> flags >= BBREACHED) */
	} /* for basic block */
	
	
	
}





/* createcompilerstub **********************************************************

   Creates a stub routine which calls the compiler.
	
*******************************************************************************/

#define COMPILERSTUB_DATASIZE    2 * SIZEOF_VOID_P
#define COMPILERSTUB_CODESIZE    3 * 4

#define COMPILERSTUB_SIZE        COMPILERSTUB_DATASIZE + COMPILERSTUB_CODESIZE


u1 *createcompilerstub(methodinfo *m)
{
	u1     *s;                          /* memory to hold the stub            */
	ptrint *d;
	s4     *mcodeptr;                   /* code generation pointer            */

	s = CNEW(u1, COMPILERSTUB_SIZE);

	/* set data pointer and code pointer */

	d = (ptrint *) s;
	s = s + COMPILERSTUB_DATASIZE;

	mcodeptr = (s4 *) s;
	
	/* Store the methodinfo* in the same place as in the methodheader
	   for compiled methods. */

	d[0] = (ptrint) asm_call_jit_compiler;
	d[1] = (ptrint) m;

	/* code for the stub */

	M_LDX(REG_ITMP1, REG_PV_CALLER, -1 * 8);   /* load methodinfo pointer            */
	/* XXX CALLER PV ??? */
	M_LDX(REG_PV_CALLER ,REG_PV_CALLER, -2 * 8);      /* load pointer to the compiler       */
	M_JMP(REG_ZERO, REG_PV_CALLER, REG_ZERO);  /* jump to the compiler, RA is wasted */

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		count_cstub_len += COMPILERSTUB_SIZE;
#endif

	return s;
}



/* createnativestub ************************************************************

   Creates a stub routine which calls a native method.

*******************************************************************************/

u1 *createnativestub(functionptr f, jitdata *jd, methoddesc *nmd)
{
	fabort("help me!");
	return NULL;
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
