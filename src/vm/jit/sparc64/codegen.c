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

   Changes: Edwin Steiner

   $Id: codegen.c 4644 2006-03-16 18:44:46Z edwin $

*/


#include "config.h"

#include <stdio.h>
#include <assert.h>


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
#include "vm/jit/emit-common.h"
#include "vm/jit/jit.h"
#include "vm/jit/parse.h"
#include "vm/jit/patcher.h"
#include "vm/jit/reg.h"

/* XXX use something like this for window control ? 
 * #define REG_PV (own_window?REG_PV_CALLEE:REG_PV_CALLER)
 */
#define REG_PV REG_PV_CALLEE

static int fabort(char *x)
{
    fprintf(stderr, "sparc64 abort because: %s\n", x);
    exit(1);
    abort();
    return 0;
			    
}

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
	s4                  stackframesize;
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

	m  = jd->m;
	code = jd->code;
	cd = jd->cd;
	rd = jd->rd;
	
	/* prevent compiler warnings */

	d = 0;
	currentline = 0;
	lm = NULL;
	bte = NULL;

	{
	s4 i, p, t, l;
	s4 savedregs_num;

#if 0 /* no leaf optimization yet */
	savedregs_num = (jd->isleafmethod) ? 0 : 1;       /* space to save the RA */
#endif
	savedregs_num = 16;                          /* register-window save area */ 


	/* space to save used callee saved registers */

	savedregs_num += (INT_SAV_CNT - rd->savintreguse);
	savedregs_num += (FLT_SAV_CNT - rd->savfltreguse);

	stackframesize = rd->memuse + savedregs_num;

#if defined(ENABLE_THREADS)        /* space to save argument of monitor_enter */
	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		stackframesize++;
#endif

	/* create method header */
	
	(void) dseg_addaddress(cd, code);                      /* CodeinfoPointer */
	(void) dseg_adds4(cd, stackframesize * 8);              /* FrameSize      */

#if defined(ENABLE_THREADS)
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
	                                       
	(void) dseg_adds4(cd, jd->isleafmethod);                 /* IsLeaf         */
	(void) dseg_adds4(cd, INT_SAV_CNT - rd->savintreguse);  /* IntSave        */
	(void) dseg_adds4(cd, FLT_SAV_CNT - rd->savfltreguse);  /* FltSave        */
	
	dseg_addlinenumbertablesize(cd);

	(void) dseg_adds4(cd, cd->exceptiontablelength);        /* ExTableSize    */
	
	/* create exception table */

	for (ex = cd->exceptiontable; ex != NULL; ex = ex->down) {
		dseg_addtarget(cd, ex->start);
   		dseg_addtarget(cd, ex->end);
		dseg_addtarget(cd, ex->handler);
		(void) dseg_addaddress(cd, ex->catchtype.any);
	}

	/* save register window and create stack frame (if necessary) */

	if (stackframesize)
		M_SAVE(REG_SP, -stackframesize * 8, REG_SP);

	/* save return address and used callee saved registers */

	p = stackframesize;
	for (i = FLT_SAV_CNT - 1; i >= rd->savfltreguse; i--) {
		p--; M_DST(rd->savfltregs[i], REG_SP, (WINSAVE_REGS + p) * 8);
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
 					M_STX(s2, REG_SP, (WINSAVE_REGS + var->regoff) * 8);
 				}

			} else {                                 /* stack arguments       */
 				if (!(var->flags & INMEMORY)) {      /* stack arg -> register */
 					M_LDX(var->regoff, REG_SP, (stackframesize + s1) * 8);

 				} else {                             /* stack arg -> spilled  */
					var->regoff = stackframesize + s1;
				}
			}
		
		} else {                                     /* floating args         */
 			if (!md->params[p].inmemory) {           /* register arguments    */
				s2 = rd->argfltregs[s1];
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
 					M_FLTMOVE(s2, var->regoff);

 				} else {			                 /* reg arg -> spilled    */
 					M_DST(s2, REG_SP, (WINSAVE_REGS + var->regoff) * 8);
 				}

			} else {                                 /* stack arguments       */
 				if (!(var->flags & INMEMORY)) {      /* stack-arg -> register */
 					M_DLD(var->regoff, REG_SP, (stackframesize + s1) * 8);

 				} else {                             /* stack-arg -> spilled  */
					var->regoff = stackframesize + s1;
				}
			}
		}
	} /* end for */
	
	
	/* XXX monitor enter and tracing */
	
	}
	
	/* end of header generation */ 
	
	replacementpoint = jd->code->rplpoints;

	/* walk through all basic blocks */

	for (bptr = jd->new_basicblocks; bptr != NULL; bptr = bptr->next) {

		bptr->mpc = (s4) (cd->mcodeptr - cd->mcodebase);

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
		
		len = bptr->icount;

		for (iptr = bptr->iinstr; len > 0; len--, iptr++) {
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
		case ICMD_LLOAD:      /* op1 = local variable                         */
		case ICMD_ALOAD:

			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
			if ((iptr->dst.var->varkind == LOCALVAR) &&
			    (iptr->dst.var->varnum == iptr->s1.localindex))
				break;
			var = &(rd->locals[iptr->s1.localindex][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY) {
				M_ALD(REG_SP, var->regoff * 8, d);
			} else {
				M_INTMOVE(var->regoff, d);
			}
			emit_store_dst(jd, iptr, d);
			break;
	
		case ICMD_FLOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_DLOAD:      /* op1 = local variable                         */

			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
			if ((iptr->dst.var->varkind == LOCALVAR) &&
			    (iptr->dst.var->varnum == iptr->s1.localindex))
				break;
			var = &(rd->locals[iptr->s1.localindex][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY) {
				M_DLD(d, REG_SP, var->regoff * 8);
			} else {
				M_FLTMOVE(var->regoff, d);
			}
			emit_store_dst(jd, iptr, d);
			break;


		case ICMD_ISTORE:     /* ..., value  ==> ...                          */
		case ICMD_LSTORE:     /* op1 = local variable                         */
		case ICMD_ASTORE:

			if ((iptr->s1.var->varkind == LOCALVAR) &&
			    (iptr->s1.var->varnum == iptr->dst.localindex))
				break;
			var = &(rd->locals[iptr->dst.localindex][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				s1 = emit_load_s1(jd, iptr, REG_ITMP1);
				M_STX(s1, REG_SP, var->regoff * 8);
			} else {
				s1 = emit_load_s1(jd, iptr, var->regoff);
				M_INTMOVE(s1, var->regoff);
			}
			break;

		case ICMD_FSTORE:     /* ..., value  ==> ...                          */
		case ICMD_DSTORE:     /* op1 = local variable                         */

			if ((iptr->s1.var->varkind == LOCALVAR) &&
			    (iptr->s1.var->varnum == iptr->dst.localindex))
				break;
			var = &(rd->locals[iptr->dst.localindex][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				s1 = emit_load_s1(jd, iptr, REG_FTMP1);
				M_DST(s1, REG_SP, var->regoff * 8);
			} else {
				s1 = emit_load_s1(jd, iptr, var->regoff);
				M_FLTMOVE(s1, var->regoff);
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
		case ICMD_LNEG:

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_SUB(REG_ZERO, s1, d);
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
			M_SRA_IMM(s1, 0, d); /* sign extend upper 32 bits */
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_SLLX_IMM(s1, 56, d);
			M_SRAX_IMM( d, 56, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_INT2CHAR:   /* ..., value  ==> ..., value                   */
		case ICMD_INT2SHORT:

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_SLLX_IMM(s1, 48, d);
			M_SRAX_IMM( d, 48, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IADD:       /* ..., val1, val2  ==> ..., val1 + val2        */
		case ICMD_LADD:

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_ADD(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if ((iptr->sx.val.i >= -4096) && (iptr->sx.val.i <= 4095)) {
				M_ADD_IMM(s1, iptr->sx.val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->sx.val.i);
				M_ADD(s1, REG_ITMP2, d);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* sx.val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if ((iptr->sx.val.l >= -4096) && (iptr->sx.val.l <= 4095)) {
				M_ADD_IMM(s1, iptr->sx.val.l, d);
			} else {
				LCONST(REG_ITMP2, iptr->sx.val.l);
				M_ADD(s1, REG_ITMP2, d);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_ISUB:       /* ..., val1, val2  ==> ..., val1 - val2        */
		case ICMD_LSUB: 

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_SUB(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if ((iptr->sx.val.i >= -4096) && (iptr->sx.val.i <= 4095)) {
				M_SUB_IMM(s1, iptr->sx.val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->sx.val.i);
				M_SUB(s1, REG_ITMP2, d);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* sx.val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if ((iptr->sx.val.l >= -4096) && (iptr->sx.val.l <= 4095)) {
				M_SUB_IMM(s1, iptr->sx.val.l, d);
			} else {
				LCONST(REG_ITMP2, iptr->sx.val.l);
				M_SUB(s1, REG_ITMP2, d);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */
		case ICMD_LMUL:

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_MULX(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if ((iptr->sx.val.i >= -4096) && (iptr->sx.val.i <= 4095)) {
				M_MULX_IMM(s1, iptr->sx.val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->sx.val.i);
				M_MULX(s1, REG_ITMP2, d);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* sx.val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if ((iptr->sx.val.l >= -4096) && (iptr->sx.val.l <= 4095)) {
				M_MULX_IMM(s1, iptr->sx.val.l, d);
			} else {
				LCONST(REG_ITMP2, iptr->sx.val.l);
				M_MULX(s1, REG_ITMP2, d);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */
/* XXX could also clear Y and use 32bit div */
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			gen_div_check(s2);
			M_ISEXT(s1, s1);
			/* XXX trim s2 like s1 ? */
			M_DIVX(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			gen_div_check(s2);
			M_DIVX(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP3);
			gen_div_check(s2);
			M_ISEXT(s1, s1);
			/* XXX trim s2 like s1 ? */
			M_DIVX(s1, s2, d);
			M_MULX(s2, d, d);
			M_SUB(s1, d, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP3);
			gen_div_check(s2);
			M_DIVX(s1, s2, d);
			M_MULX(s2, d, d);
			M_SUB(s1, d, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IDIVPOW2:   /* ..., value  ==> ..., value << constant       */
		case ICMD_LDIVPOW2:   /* val.i = constant                             */
		                      
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_SRAX_IMM(s1, 63, REG_ITMP2);
			M_SRLX_IMM(REG_ITMP2, 64 - iptr->sx.val.i, REG_ITMP2);
			M_ADD(s1, REG_ITMP2, REG_ITMP2);
			M_SRAX_IMM(REG_ITMP2, iptr->sx.val.i, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_ISHL:       /* ..., val1, val2  ==> ..., val1 << val2       */
		case ICMD_LSHL:

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_SLLX(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		case ICMD_LSHLCONST:  /* val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_SLLX_IMM(s1, iptr->sx.val.i, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_SRA(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_SRA_IMM(s1, iptr->sx.val.i, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_SRL(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_SRL_IMM(s1, iptr->sx.val.i, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_SRAX(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_SRAX_IMM(s1, iptr->sx.val.i, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_SRLX(s1, s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_SRLX_IMM(s1, iptr->sx.val.i, d);
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
			if ((iptr->sx.val.i >= -4096) && (iptr->sx.val.i <= 4095)) {
				M_AND_IMM(s1, iptr->sx.val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->sx.val.i);
				M_AND(s1, REG_ITMP2, d);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_ISEXT(s1, s1); /* trim for 32-bit compare (BGEZ) */
			if (s1 == d) {
				M_MOV(s1, REG_ITMP1);
				s1 = REG_ITMP1;
			}
			if ((iptr->sx.val.i >= 0) && (iptr->sx.val.i <= 0xffff)) {
				M_AND_IMM(s1, iptr->sx.val.i, d);
				M_BGEZ(s1, 4);
				M_NOP;
				M_SUB(REG_ZERO, s1, d);
				M_AND_IMM(d, iptr->sx.val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->sx.val.i);
				M_AND(s1, REG_ITMP2, d);
				M_BGEZ(s1, 4);
				M_NOP;
				M_SUB(REG_ZERO, s1, d);
				M_AND(d, REG_ITMP2, d);
			}
			M_SUB(REG_ZERO, d, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* sx.val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if ((iptr->sx.val.l >= -4096) && (iptr->sx.val.l <= 4095)) {
				M_AND_IMM(s1, iptr->sx.val.l, d);
			} else {
				LCONST(REG_ITMP2, iptr->sx.val.l);
				M_AND(s1, REG_ITMP2, d);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* sx.val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if (s1 == d) {
				M_MOV(s1, REG_ITMP1);
				s1 = REG_ITMP1;
			}
			if ((iptr->sx.val.l >= -4096) && (iptr->sx.val.l <= 4095)) {
				M_AND_IMM(s1, iptr->sx.val.l, d);
				M_BGEZ(s1, 4);
				M_NOP;
				M_SUB(REG_ZERO, s1, d);
				M_AND_IMM(d, iptr->sx.val.l, d);
			} else {
				LCONST(REG_ITMP2, iptr->sx.val.l);
				M_AND(s1, REG_ITMP2, d);
				M_BGEZ(s1, 4);
				M_NOP;
				M_SUB(REG_ZERO, s1, d);
				M_AND(d, REG_ITMP2, d);
			}
			M_SUB(REG_ZERO, d, d);
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
			if ((iptr->sx.val.i >= -4096) && (iptr->sx.val.i <= 4095)) {
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
			if ((iptr->sx.val.l >= -4096) && (iptr->sx.val.l <= 4095)) {
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
			if ((iptr->sx.val.i >= -4096) && (iptr->sx.val.i <= 4095)) {
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
			if ((iptr->sx.val.l >= -4096) && (iptr->sx.val.l <= 4095)) {
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
			M_CMP(s1, s2);
			M_MOV(REG_ZERO, d);
			M_XCMOVLT_IMM(-1, d);
			M_XCMOVGT_IMM(1, d);
			emit_store_dst(jd, iptr, d);
			break;


		case ICMD_IINC:       /* ..., value  ==> ..., value + constant        */
		                      /* s1.localindex = variable, sx.val.i = constant             */

			var = &(rd->locals[iptr->s1.localindex][TYPE_INT]);
			if (var->flags & INMEMORY) {
				s1 = REG_ITMP1;
				M_LDX(s1, REG_SP, var->regoff * 8);
			} else
				s1 = var->regoff;
			if ((iptr->sx.val.i >= -4096) && (iptr->sx.val.i <= 4095)) {
				M_ADD_IMM(s1, iptr->sx.val.i, s1);
			} else {
				ICONST(REG_ITMP2, iptr->sx.val.i);
				M_ADD(s1, REG_ITMP2, s1);
			}
			if (var->flags & INMEMORY)
				M_STX(s1, REG_SP, var->regoff * 8);
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

		case ICMD_I2F:
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP3);
			disp = dseg_addfloat(cd, 0.0);
			M_IST (s1, REG_PV_CALLEE, disp);
			M_FLD (d, REG_PV_CALLEE, disp);
			M_CVTIF (d, d); /* rd gets translated to double target register */
			emit_store_dst(jd, iptr, d);
			break;
			
		case ICMD_I2D:
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP3);
			disp = dseg_adddouble(cd, 0.0);
			M_STX (s1, REG_PV_CALLEE, disp);
			M_DLD (REG_FTMP2, REG_PV_CALLEE, disp); /* REG_FTMP2 needs to be a double temp */
			M_CVTLF (REG_FTMP2, d); /* rd gets translated to double target register */
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_F2I:       /* ..., value  ==> ..., (int) value              */
			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP3);
			disp = dseg_addfloat(cd, 0.0);
			M_CVTFI(s1, REG_FTMP2);
			M_FST(REG_FTMP2, REG_PV_CALLEE, disp);
			M_ILD(d, REG_PV, disp);
			emit_store_dst(jd, iptr, d);
			break;
			
			       
		case ICMD_D2I:       /* ..., value  ==> ..., (int) value             */
			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP3);
			disp = dseg_addfloat(cd, 0.0);
			M_CVTDI(s1, REG_FTMP2);
			M_FST(REG_FTMP2, REG_PV, disp);
			M_ILD(d, REG_PV, disp);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_F2L:       /* ..., value  ==> ..., (long) value             */
			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP3);
			disp = dseg_adddouble(cd, 0.0);
			M_CVTFL(s1, REG_FTMP2); /* FTMP2 needs to be double reg */
			M_DST(REG_FTMP2, REG_PV, disp);
			M_LDX(d, REG_PV, disp);
			emit_store_dst(jd, iptr, d);
			break;
			
		case ICMD_D2L:       /* ..., value  ==> ..., (long) value             */
			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP3);
			disp = dseg_adddouble(cd, 0.0);
			M_CVTDL(s1, REG_FTMP2); /* FTMP2 needs to be double reg */
			M_DST(REG_FTMP2, REG_PV, disp);
			M_LDX(d, REG_PV, disp);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_F2D:       /* ..., value  ==> ..., (double) value           */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP3);
			M_CVTFD(s1, d);
			emit_store_dst(jd, iptr, d);
			break;
					
		case ICMD_D2F:       /* ..., value  ==> ..., (float) value            */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP3);
			M_CVTDF(s1, d);
			emit_store_dst(jd, iptr, d);
			break;
	
	/* XXX merge F/D versions? only compare instr. is different */
		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP3);
			M_FCMP(s1,s2);
			M_OR_IMM(REG_ZERO, -1, REG_ITMP3); /* less by default (less or unordered) */
			M_CMOVFEQ_IMM(0, REG_ITMP3); /* 0 if equal */
			M_CMOVFGT_IMM(1, REG_ITMP3); /* 1 if greater */
			emit_store_dst(jd, iptr, d);
			break;
			
		case ICMD_DCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP3);
			M_DCMP(s1,s2);
			M_OR_IMM(REG_ZERO, -1, REG_ITMP3); /* less by default (less or unordered) */
			M_CMOVFEQ_IMM(0, REG_ITMP3); /* 0 if equal */
			M_CMOVFGT_IMM(1, REG_ITMP3); /* 1 if greater */
			emit_store_dst(jd, iptr, d);
			break;
			
		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP3);	
			M_FCMP(s1,s2);
			M_OR_IMM(REG_ZERO, 1, REG_ITMP3); /* greater by default (greater or unordered) */
			M_CMOVFEQ_IMM(0, REG_ITMP3); /* 0 if equal */
			M_CMOVFLT_IMM(-1, REG_ITMP3); /* -1 if less */
			emit_store_dst(jd, iptr, d);
			break;
			
		case ICMD_DCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP3);	
			M_DCMP(s1,s2);
			M_OR_IMM(REG_ZERO, 1, REG_ITMP3); /* greater by default (greater or unordered) */
			M_CMOVFEQ_IMM(0, REG_ITMP3); /* 0 if equal */
			M_CMOVFLT_IMM(-1, REG_ITMP3); /* -1 if less */
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
			M_BLDS(d, REG_ITMP3, OFFSET(java_chararray, data[0]));
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
			M_SLDS(d, REG_ITMP3, OFFSET(java_chararray, data[0]));
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
			M_LDX(d, REG_ITMP3, OFFSET(java_longarray, data[0]));
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
			M_STX_INTERN(s3, REG_ITMP1, OFFSET(java_longarray, data[0]));
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

			M_MOV(s1, rd->argintregs[0]);
			M_MOV(s3, rd->argintregs[1]);
			disp = dseg_addaddress(cd, BUILTIN_canstore);
			M_ALD(REG_ITMP3, REG_PV, disp);
			M_JMP(REG_RA_CALLER, REG_ITMP3, REG_ZERO);
			M_NOP;

			M_BEQZ(REG_RESULT_CALLER, 0);
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
			M_STX_INTERN(REG_ZERO, REG_ITMP1, OFFSET(java_longarray, data[0]));
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

				codegen_addpatchref(cd, PATCHER_get_putstatic, uf, disp);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

			} else {
				fieldinfo *fi = iptr->sx.s23.s3.fmiref->p.field;

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
				M_LDX_INTERN(d, REG_ITMP1, 0);
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
				s2 = emit_load_s2(jd, iptr, REG_ITMP2);
				M_IST_INTERN(s2, REG_ITMP1, 0);
				break;
			case TYPE_LNG:
				s2 = emit_load_s2(jd, iptr, REG_ITMP2);
				M_STX_INTERN(s2, REG_ITMP1, 0);
				break;
			case TYPE_ADR:
				s2 = emit_load_s2(jd, iptr, REG_ITMP2);
				M_AST_INTERN(s2, REG_ITMP1, 0);
				break;
			case TYPE_FLT:
				s2 = emit_load_s2(jd, iptr, REG_FTMP2);
				M_FST_INTERN(s2, REG_ITMP1, 0);
				break;
			case TYPE_DBL:
				s2 = emit_load_s2(jd, iptr, REG_FTMP2);
				M_DST_INTERN(s2, REG_ITMP1, 0);
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

				disp = 0;

			} else {
				disp = iptr->sx.s23.s3.fmiref->p.field->offset;
			}

			switch (fieldtype) {
			case TYPE_INT:
				d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
				M_ILD(d, s1, disp);
				emit_store_dst(jd, iptr, d);
				break;
			case TYPE_LNG:
				d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
				M_LDX(d, s1, disp);
				emit_store_dst(jd, iptr, d);
				break;
			case TYPE_ADR:
				d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
				M_ALD(d, s1, disp);
				emit_store_dst(jd, iptr, d);
				break;
			case TYPE_FLT:
				d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
				M_FLD(d, s1, disp);
				emit_store_dst(jd, iptr, d);
				break;
			case TYPE_DBL:				
				d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
				M_DLD(d, s1, disp);
				emit_store_dst(jd, iptr, d);
				break;
			}
			break;

		case ICMD_PUTFIELD:   /* ..., objectref, value  ==> ...               */

			s1 = emit_load_s1(jd, iptr, REG_ITMP2);
			gen_nullptr_check(s1);

			/*if (!IS_FLT_DBL_TYPE(fieldtype)) {
				s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			} else {*/
				s2 = emit_load_s2(jd, iptr, REG_IFTMP);
			/*}*/

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				unresolved_field *uf = iptr->sx.s23.s3.uf;

				fieldtype = uf->fieldref->parseddesc.fd->type;

				codegen_addpatchref(cd, PATCHER_get_putfield,
									iptr->sx.s23.s3.uf, 0);

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}

				disp = 0;

			} else {
				disp = iptr->sx.s23.s3.fmiref->p.field->offset;
			}

			switch (fieldtype) {
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

				disp = 0;

			} else {
			{
				fieldinfo *fi = iptr->sx.s23.s3.fmiref->p.field;

				fieldtype = fi->type;
				disp = fi->offset;
			}

			}

			switch (fieldtype) {
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

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			M_INTMOVE(s1, REG_ITMP2_XPTR);

#ifdef ENABLE_VERIFIER
			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				codegen_addpatchref(cd, PATCHER_athrow_areturn,
									iptr->sx.s23.s2.uc, 0);

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
			M_BR(0);
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			ALIGNCODENOP;
			break;

		case ICMD_JSR:          /* ... ==> ...                                */

			dseg_addtarget(cd, iptr->sx.s23.s3.jsrtarget.block);
			M_ALD(REG_ITMP1, REG_PV, -(cd->dseglen));
			M_JMP(REG_ITMP1, REG_ITMP1, REG_ZERO);        /* REG_ITMP1 = return address */
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
				if ((iptr->sx.val.i >= -4096) && (iptr->sx.val.i <= 4095)) {
					M_CMP_IMM(s1, iptr->sx.val.i);
					}
				else {
					ICONST(REG_ITMP2, iptr->sx.val.i);
					M_CMP(s1, REG_ITMP2);
					}
				M_BEQ(0);
				}
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IFLT:         /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (iptr->sx.val.i == 0) {
				M_BLTZ(s1, 0);
			} else {
				if ((iptr->sx.val.i >= -4096) && (iptr->sx.val.i <= 4095)) {
					M_CMP_IMM(s1, iptr->sx.val.i);
				} else {
					ICONST(REG_ITMP2, iptr->sx.val.i);
					M_CMP(s1, REG_ITMP2);
				}
				M_BLT(0);
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
				if ((iptr->sx.val.i >= -4096) && (iptr->sx.val.i <= 4095)) {
					M_CMP_IMM(s1, iptr->sx.val.i);
					}
				else {
					ICONST(REG_ITMP2, iptr->sx.val.i);
					M_CMP(s1, REG_ITMP2);
				}
				M_BLE(0);
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
				if ((iptr->sx.val.i >= -4096) && (iptr->sx.val.i <= 4095)) {
					M_CMP_IMM(s1, iptr->sx.val.i);
				}
				else {
					ICONST(REG_ITMP2, iptr->sx.val.i);
					M_CMP(s1, REG_ITMP2);
				}
				M_BNE(0);
			}
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;
						
		case ICMD_IFGT:         /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (iptr->sx.val.i == 0) {
				M_BLTZ(s1, 0);
			} else {
				if ((iptr->sx.val.i >= -4096) && (iptr->sx.val.i <= 4095)) {
					M_CMP_IMM(s1, iptr->sx.val.i);
				} else {
					ICONST(REG_ITMP2, iptr->sx.val.i);
					M_CMP(s1, REG_ITMP2);
				}
				M_BGT(0);
			}
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IFGE:         /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (iptr->sx.val.i == 0) {
				M_BLEZ(s1, 0);
				}
			else {
				if ((iptr->sx.val.i >= -4096) && (iptr->sx.val.i <= 4095)) {
					M_CMP_IMM(s1, iptr->sx.val.i);
					}
				else {
					ICONST(REG_ITMP2, iptr->sx.val.i);
					M_CMP(s1, REG_ITMP2);
				}
				M_BGE(0);
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
				if ((iptr->sx.val.l >= -4096) && (iptr->sx.val.l <= 4095)) {
					M_CMP_IMM(s1, iptr->sx.val.l);
				}
				else {
					LCONST(REG_ITMP2, iptr->sx.val.l);
					M_CMP(s1, REG_ITMP2);
				}
				M_XBEQ(0);
			}
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;
			
		case ICMD_IF_LLT:       /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (iptr->sx.val.l == 0) {
				M_BLTZ(s1, 0);
			} else {
				if ((iptr->sx.val.l >= -4096) && (iptr->sx.val.l <= 4095)) {
					M_CMP_IMM(s1, iptr->sx.val.l);
				} else {
					ICONST(REG_ITMP2, iptr->sx.val.l);
					M_CMP(s1, REG_ITMP2);
				}
				M_XBLT(0);
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
				if ((iptr->sx.val.l >= -4096) && (iptr->sx.val.l <= 4095)) {
					M_CMP_IMM(s1, iptr->sx.val.l);
					}
				else {
					ICONST(REG_ITMP2, iptr->sx.val.l);
					M_CMP(s1, REG_ITMP2);
				}
				M_XBLE(0);
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
				if ((iptr->sx.val.l >= -4096) && (iptr->sx.val.l <= 4095)) {
					M_CMP_IMM(s1, iptr->sx.val.i);
				}
				else {
					ICONST(REG_ITMP2, iptr->sx.val.l);
					M_CMP(s1, REG_ITMP2);
				}
				M_XBNE(0);
			}
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;
						
		case ICMD_IF_LGT:       /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (iptr->sx.val.l == 0) {
				M_BLTZ(s1, 0);
			} else {
				if ((iptr->sx.val.l >= -4096) && (iptr->sx.val.l <= 4095)) {
					M_CMP_IMM(s1, iptr->sx.val.l);
				} else {
					ICONST(REG_ITMP2, iptr->sx.val.l);
					M_CMP(s1, REG_ITMP2);
				}
				M_XBGT(0);
			}
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IF_LGE:       /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (iptr->sx.val.l == 0) {
				M_BLEZ(s1, 0);
				}
			else {
				if ((iptr->sx.val.l >= -4096) && (iptr->sx.val.l <= 4095)) {
					M_CMP_IMM(s1, iptr->sx.val.l);
					}
				else {
					ICONST(REG_ITMP2, iptr->sx.val.l);
					M_CMP(s1, REG_ITMP2);
				}
				M_XBGE(0);
			}
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;			
			

		case ICMD_IF_ACMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPEQ:    /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_XBEQ(0);
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IF_ICMPEQ:    /* 32-bit compare                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_BEQ(0);
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IF_ACMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPNE:    /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_XBNE(0);
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;
			
		case ICMD_IF_ICMPNE:    /* 32-bit compare                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_BNE(0);
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IF_LCMPLT:    /* ..., value, value ==> ...                  */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_XBLT(0);
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;
			
		case ICMD_IF_ICMPLT:    /* 32-bit compare                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_BLT(0);
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IF_LCMPGT:    /* ..., value, value ==> ...                  */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_XBGT(0);
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;
			
		case ICMD_IF_ICMPGT:    /* 32-bit compare                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_BGT(0);
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IF_LCMPLE:    /* ..., value, value ==> ...                  */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_BLE(0);
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;
			
		case ICMD_IF_ICMPLE:    /* 32-bit compare                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_BLE(0);
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;			
	

		case ICMD_IF_LCMPGE:    /* ..., value, value ==> ...                  */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_BGE(0);
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;
			
		case ICMD_IF_ICMPGE:    /* 32-bit compare                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_BGE(0);
			codegen_addreference(cd, iptr->dst.block);
			M_NOP;
			break;


		case ICMD_IRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_LRETURN:

			s1 = emit_load_s1(jd, iptr, REG_RESULT_CALLEE);
			M_INTMOVE(s1, REG_RESULT_CALLEE);
			goto nowperformreturn;

		case ICMD_ARETURN:      /* ..., retvalue ==> ...                      */

			s1 = emit_load_s1(jd, iptr, REG_RESULT_CALLEE);
			M_INTMOVE(s1, REG_RESULT_CALLEE);

#ifdef ENABLE_VERIFIER
			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				codegen_addpatchref(cd, PATCHER_athrow_areturn,
									iptr->sx.s23.s2.uc, 0);

				if (opt_showdisassemble)
					M_NOP;
			}
#endif /* ENABLE_VERIFIER */
			goto nowperformreturn;

		case ICMD_FRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_DRETURN:

			s1 = emit_load_s1(jd, iptr, REG_FRESULT);
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

		case ICMD_TABLESWITCH:  /* ..., index ==> ...                         */
			{
			s4 i, l;
			branch_target_t *table;

			table = iptr->dst.table;

			l = iptr->sx.s23.s2.tablelow;
			i = iptr->sx.s23.s3.tablehigh;
			
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (l == 0) {
				M_INTMOVE(s1, REG_ITMP1);
			}
			else if (l <= 4095) {
				M_ADD_IMM(s1, -l, REG_ITMP1);
			}
			else {
				ICONST(REG_ITMP2, l);
				/* XXX: do I need to truncate s1 to 32-bit ? */
				M_SUB(s1, REG_ITMP2, REG_ITMP1);
			}
			i = i - l + 1;


			/* range check */
					
			if (i <= 4095) {
				M_CMP_IMM(REG_ITMP1, i);
			}
			else {
				ICONST(REG_ITMP2, i);
				M_CMP(REG_ITMP1, REG_ITMP2);
			}		
			M_XBULT(0);
			codegen_addreference(cd, table[0].block); /* default target */
			M_ASLL_IMM(REG_ITMP1, POINTERSHIFT, REG_ITMP1);      /* delay slot*/

			/* build jump table top down and use address of lowest entry */

			table += i;

			while (--i >= 0) {
				dseg_add_target(cd, table->block); 
				--table;
				}
			}

			/* length of dataseg after last dseg_addtarget is used by load */

			M_AADD(REG_ITMP1, REG_PV, REG_ITMP2);
			M_ALD(REG_ITMP2, REG_ITMP2, -(cd->dseglen));
			M_JMP(REG_ZERO, REG_ITMP2, REG_ZERO);
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
				if ((lookup->value >= -4096) && (lookup->value <= 4095)) {
					M_CMP_IMM(s1, lookup->value);
				} else {					
					ICONST(REG_ITMP2, lookup->value);
					M_CMP(s1, REG_ITMP2);
				}
				M_BEQ(0);
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


		case ICMD_BUILTIN:      /* ..., arg1, arg2, arg3 ==> ...              */

			bte = iptr->sx.s23.s3.bte;
			md = bte->md;
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

			for (s3 = s3 - 1; s3 >= 0; s3--, src = src->prev) {
				if (src->varkind == ARGVAR)
					continue;
				if (IS_INT_LNG_TYPE(src->type)) {
					if (!md->params[s3].inmemory) {
						s1 = rd->argintregs[md->params[s3].regoff];
						d = emit_load_s1(jd, iptr, s1);
						M_INTMOVE(d, s1);
					} else {
						d = emit_load_s1(jd, iptr, REG_ITMP1);
						M_STX(d, REG_SP, md->params[s3].regoff * 8);
					}

				} else {
					if (!md->params[s3].inmemory) {
						s1 = rd->argfltregs[md->params[s3].regoff];
						d = emit_load_s1(jd, iptr, s1);
						if (IS_2_WORD_TYPE(src->type))
							M_DMOV(d, s1);
						else
							M_FMOV(d, s1);

					} else {
						d = emit_load_s1(jd, iptr, REG_FTMP1);
						if (IS_2_WORD_TYPE(src->type))
							M_DST(d, REG_SP, md->params[s3].regoff * 8);
						else
							M_FST(d, REG_SP, md->params[s3].regoff * 8);
					}
				}
			}

			switch (iptr->opc) {
			case ICMD_BUILTIN:
		/* XXX needs manual attention! */
				disp = dseg_addaddress(cd, bte->fp);
				d = md->returntype.type;

				M_ALD(REG_ITMP3, REG_PV, disp);  /* built-in-function pointer */
				M_JMP(REG_RA_CALLER, REG_ITMP3, REG_ZERO);
				M_NOP;
/* XXX: how do builtins handle the register window? */
/*				disp = (s4) (cd->mcodeptr - cd->mcodebase);*/
/*				M_LDA(REG_PV, REG_RA, -disp);*/


				if (INSTRUCTION_MUST_CHECK(iptr)) {
					M_BEQZ(REG_RESULT_CALLER, 0);
					codegen_add_fillinstacktrace_ref(cd);
					M_NOP;
				}
				break;

			case ICMD_INVOKESPECIAL:
				M_BEQZ(REG_A0, 0);
				codegen_add_nullpointerexception_ref(cd);
				M_NOP;
				/* fall through */

			case ICMD_INVOKESTATIC:
		/* XXX needs manual attention! */
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

				M_ALD(REG_PV_CALLER, REG_PV, disp); /* method pointer in callee pv */
				M_JMP(REG_RA_CALLER, REG_PV_CALLER, REG_ZERO);
				M_NOP;
/* XXX no need to restore PV, when its in the regs  */
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
				M_ALD(REG_PV_CALLER, REG_METHODPTR, s1);
				M_JMP(REG_RA_CALLER, REG_PV_CALLER, REG_ZERO);
				M_NOP;
/* XXX no need to restore PV, when its in the regs  */
				break;

			case ICMD_INVOKEINTERFACE:
		/* XXX needs manual attention! */
				gen_nullptr_check(rd->argintregs[0]);

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
				M_ALD(REG_PV_CALLER, REG_METHODPTR, s2);
				M_JMP(REG_RA_CALLER, REG_PV_CALLER, REG_ZERO);
				M_NOP;
/* XXX no need to restore PV, when its in the regs  */
				break;
			}

			/* d contains return type */

			if (d != TYPE_VOID) {
				if (IS_INT_LNG_TYPE(d)) {
					s1 = codegen_reg_of_dst(jd, iptr, REG_RESULT_CALLER);
					M_INTMOVE(REG_RESULT_CALLER, s1);
				} 
				else {
					s1 = codegen_reg_of_dst(jd, iptr, REG_FRESULT);
					if (IS_2_WORD_TYPE(d)) {
						M_DBLMOVE(REG_FRESULT, s1);
					} else {
						M_FLTMOVE(REG_FRESULT, s1);
					}
				}
				emit_store_dst(jd, iptr, s1);
			}
			break;


		case ICMD_CHECKCAST:  /* ..., objectref ==> ..., objectref            */
		/* XXX needs manual attention! */
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

				super = iptr->sx.s23.s3.c.cls;

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

					disp = dseg_add_unique_s4(cd, 0);         /* super->flags */

					codegen_addpatchref(cd, PATCHER_checkcast_instanceof_flags,
										iptr->sx.s23.s3.c.ref,
										disp);

					if (opt_showdisassemble) {
						M_NOP; M_NOP;
					}

					M_ILD(REG_ITMP2, REG_PV, disp);
					M_AND_IMM(REG_ITMP2, ACC_INTERFACE, REG_ITMP2);
					M_BEQZ(REG_ITMP2, 1 + s2 + 2);
					M_NOP;
				}

				/* interface checkcast code */

				if ((super == NULL) || (super->flags & ACC_INTERFACE)) {
					if (super == NULL) {
						codegen_addpatchref(cd,
											PATCHER_checkcast_instanceof_interface,
											iptr->sx.s23.s3.c.ref,
											0);

						if (opt_showdisassemble) {
							M_NOP; M_NOP;
						}
					}
					else {
						M_BEQZ(s1, 1 + s2);
						M_NOP;
					}

					M_ALD(REG_ITMP2, s1, OFFSET(java_objectheader, vftbl));
					M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl_t, interfacetablelength));
					M_LDA(REG_ITMP3, REG_ITMP3, -superindex);
					M_BLEZ(REG_ITMP3, 0);
					codegen_add_classcastexception_ref(cd, s1);
					M_NOP;
					M_ALD(REG_ITMP3, REG_ITMP2,
						  (s4) (OFFSET(vftbl_t, interfacetable[0]) -
								superindex * sizeof(methodptr*)));
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
					if (super == NULL) {
						disp = dseg_add_unique_address(cd, NULL);

						codegen_addpatchref(cd,
											PATCHER_checkcast_instanceof_class,
											iptr->sx.s23.s3.c.ref,
											disp);

						if (opt_showdisassemble) {
							M_NOP; M_NOP;
						}
					}
					else {
						disp = dseg_add_address(cd, supervftbl);

						M_BEQZ(s1, 1 + s3);
						M_NOP;
					}

					M_ALD(REG_ITMP2, s1, OFFSET(java_objectheader, vftbl));
					M_ALD(REG_ITMP3, REG_PV, disp);
#if defined(ENABLE_THREADS)
					codegen_threadcritstart(cd, cd->mcodeptr - cd->mcodebase);
#endif
					M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, baseval));
					/*  				if (s1 != REG_ITMP1) { */
					/*  					M_ILD(REG_ITMP1, REG_ITMP3, OFFSET(vftbl_t, baseval)); */
					/*  					M_ILD(REG_ITMP3, REG_ITMP3, OFFSET(vftbl_t, diffval)); */
					/*  #if defined(ENABLE_THREADS) */
					/*  					codegen_threadcritstop(cd, (u1 *) mcodeptr - cd->mcodebase); */
					/*  #endif */
					/*  					M_ISUB(REG_ITMP2, REG_ITMP1, REG_ITMP2); */

					/*  				} else { */
					M_ILD(REG_ITMP3, REG_ITMP3, OFFSET(vftbl_t, baseval));
					M_SUB(REG_ITMP2, REG_ITMP3, REG_ITMP2);
					M_ALD(REG_ITMP3, REG_PV, disp);
					M_ILD(REG_ITMP3, REG_ITMP3, OFFSET(vftbl_t, diffval));
#if defined(ENABLE_THREADS)
					codegen_threadcritstop(cd, cd->mcodeptr - cd->mcodebase);
#endif
					/*  				} */
					M_CMP(REG_ITMP3, REG_ITMP2);
					M_BULT(0);                         /* branch if ITMP3 < ITMP2 */ 
					codegen_add_classcastexception_ref(cd, s1);
					M_NOP;
				}

				d = codegen_reg_of_dst(jd, iptr, s1);
			}
			else {
				/* array type cast-check */

				s1 = emit_load_s1(jd, iptr, rd->argintregs[0]);
				M_INTMOVE(s1, rd->argintregs[0]);

				disp = dseg_addaddress(cd, iptr->sx.s23.s3.c.cls);

				if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
					codegen_addpatchref(cd, PATCHER_builtin_arraycheckcast,
										iptr->sx.s23.s3.c.ref,
										disp);

					if (opt_showdisassemble) {
						M_NOP; M_NOP;
					}
				}

				M_ALD(rd->argintregs[1], REG_PV, disp);
				disp = dseg_addaddress(cd, BUILTIN_arraycheckcast);
				M_ALD(REG_ITMP3, REG_PV, disp);
				M_JMP(REG_RA_CALLER, REG_ITMP3, REG_ZERO);
				M_NOP;

				s1 = emit_load_s1(jd, iptr, REG_ITMP1);
				M_BEQZ(REG_RESULT_CALLER, 0);
				codegen_add_classcastexception_ref(cd, s1);
				M_NOP;

				d = codegen_reg_of_dst(jd, iptr, s1);
			}

			M_INTMOVE(s1, d);
			emit_store_dst(jd, iptr, d);
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
				s1 = emit_load(jd, NULL, src, REG_FTMP1);
				if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
					M_FLTMOVE(s1,rd->interfaces[len][s2].regoff);
					}
				else {
					M_DST(s1, REG_SP, 8 * rd->interfaces[len][s2].regoff);
					}
				}
			else {
				s1 = emit_load(jd, NULL, src, REG_ITMP1);
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
	
	dseg_createlinenumbertable(cd);

	/* generate stubs */

	emit_exception_stubs(jd);
	emit_patcher_stubs(jd);
	emit_replacement_stubs(jd);

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
	u1     *s;                          /* memory to hold the stub            */
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
	/* no window save yet, user caller's PV */
	M_ALD_INTERN(REG_ITMP1, REG_PV_CALLER, -2 * SIZEOF_VOID_P);  /* codeinfo pointer */
	M_ALD_INTERN(REG_PV_CALLER, REG_PV_CALLER, -3 * SIZEOF_VOID_P);  /* pointer to compiler */
	M_JMP(REG_ZERO, REG_PV_CALLER, REG_ZERO);  /* jump to the compiler, RA is wasted */
	M_NOP;

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
	/* fabort("help me!"); */
	printf("createnativestub not implemented\n");
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
