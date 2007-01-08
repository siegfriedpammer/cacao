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
	varinfo            *var;
	basicblock         *bptr;
	instruction        *iptr;
	exception_entry    *ex;
	u2                  currentline;
	constant_classref  *cr;
	methodinfo         *lm;             /* local methodinfo for ICMD_INVOKE*  */
	unresolved_method  *um;
	builtintable_entry *bte;
	methoddesc         *md;
	fieldinfo          *fi;
	unresolved_field   *uf;
	rplpoint           *replacementpoint;
	s4                  fieldtype;
	s4                  varindex;

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
	s4 savedregs_num, localbase;

#if 0 /* no leaf optimization yet */
	savedregs_num = (jd->isleafmethod) ? 0 : 1;       /* space to save the RA */
#endif
	savedregs_num = 16;                          /* register-window save area */ 


	/* space to save used callee saved registers */

	savedregs_num += (INT_SAV_CNT - rd->savintreguse);
	savedregs_num += (FLT_SAV_CNT - rd->savfltreguse);

	cd->stackframesize = rd->memuse + savedregs_num;

#if defined(ENABLE_THREADS)        /* space to save argument of monitor_enter */
	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		cd->stackframesize++;
#endif

	/* create method header */

	(void) dseg_add_unique_address(cd, code);              /* CodeinfoPointer */
	(void) dseg_add_unique_s4(cd, cd->stackframesize * 8); /* FrameSize       */

#if defined(ENABLE_THREADS)
	/* IsSync contains the offset relative to the stack pointer for the
	   argument of monitor_exit used in the exception handler. Since the
	   offset could be zero and give a wrong meaning of the flag it is
	   offset by one.
	*/

	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		(void) dseg_add_unique_s4(cd, (rd->memuse + 1) * 8); /* IsSync        */
	else
#endif
		(void) dseg_add_unique_s4(cd, 0);                  /* IsSync          */
	                                       
	(void) dseg_add_unique_s4(cd, jd->isleafmethod);       /* IsLeaf          */
	(void) dseg_add_unique_s4(cd, INT_SAV_CNT - rd->savintreguse); /* IntSave */
	(void) dseg_add_unique_s4(cd, FLT_SAV_CNT - rd->savfltreguse); /* FltSave */
	dseg_addlinenumbertablesize(cd);
	(void) dseg_add_unique_s4(cd, jd->exceptiontablelength); /* ExTableSize   */

	/* create exception table */

	for (ex = jd->exceptiontable; ex != NULL; ex = ex->down) {
		dseg_add_target(cd, ex->start);
   		dseg_add_target(cd, ex->end);
		dseg_add_target(cd, ex->handler);
		(void) dseg_add_unique_address(cd, ex->catchtype.any);
	}

	/* save register window and create stack frame (if necessary) */

	if (cd->stackframesize)
		M_SAVE(REG_SP, -cd->stackframesize * 8, REG_SP);


	/* save callee saved float registers (none right now) */
#if 0
	p = cd->stackframesize;
	for (i = FLT_SAV_CNT - 1; i >= rd->savfltreguse; i--) {
		p--; M_DST(rd->savfltregs[i], REG_SP, USESTACK + (p * 8));
	}
#endif

#if !defined(NDEBUG)
	if (JITDATA_HAS_FLAG_VERBOSECALL(jd))
		emit_verbosecall_enter(jd);
#endif
	
	
	
	/* take arguments out of register or stack frame */
	
	md = m->parseddesc;

	/* when storing locals, use this as base */
	localbase = USESTACK;
	
	/* since the register allocator does not know about the shifting window
	 * arg regs need to be copied via the stack
	 */
	if (md->argintreguse > 0) {
		/* allocate scratch space for copying in to save(i&l) regs */
		M_SUB_IMM(REG_SP, INT_ARG_CNT * 8, REG_SP);
		
		localbase += INT_ARG_CNT * 8;
		
		for (p = 0; p < INT_ARG_CNT; p++)
			M_STX(REG_WINDOW_TRANSPOSE(rd->argintregs[p]), REG_SP, USESTACK + (p * 8));
	}
	

 	for (p = 0, l = 0; p < md->paramcount; p++) {
 		t = md->paramtypes[p].type;

  		varindex = jd->local_map[l * 5 + t];

 		l++;
 		if (IS_2_WORD_TYPE(t))    /* increment local counter for 2 word types */
 			l++;

		if (varindex == UNUSED)
 			continue;

 		var = VAR(varindex);

		s1 = md->params[p].regoff;
		if (IS_INT_LNG_TYPE(t)) {                    /* integer args          */
 			if (!md->params[p].inmemory) {           /* register arguments    */
				/*s2 = rd->argintregs[s1];*/
				/*s2 = REG_WINDOW_TRANSPOSE(s2);*/
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
 					/*M_INTMOVE(s2, var->vv.regoff);*/
 					M_LDX(var->vv.regoff, REG_SP, USESTACK + (s1 * 8));

				} else {                             /* reg arg -> spilled    */
					/*M_STX(s2, REG_SP, (WINSAVE_CNT + var->vv.regoff) * 8);*/
					
					M_LDX(REG_ITMP1, REG_SP, USESTACK + (s1 * 8));
					M_STX(REG_ITMP1, REG_SP, localbase + (var->vv.regoff * 8));
 				}

			} else {                                 /* stack arguments       */
 				if (!(var->flags & INMEMORY)) {      /* stack arg -> register */
 					M_LDX(var->vv.regoff, REG_FP, USESTACK + (s1 * 8));

 				} else {                             /* stack arg -> spilled  */
					/* add the callers window save registers */
					var->vv.regoff = cd->stackframesize + WINSAVE_CNT + s1;
				}
			}
		
		} else {                                     /* floating args         */
 			if (!md->params[p].inmemory) {           /* register arguments    */
				s2 = rd->argfltregs[s1];
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
 					M_FLTMOVE(s2, var->vv.regoff);

 				} else {			                 /* reg arg -> spilled    */
 					M_DST(s2, REG_SP, localbase + (var->vv.regoff) * 8);
 				}

			} else {                                 /* stack arguments       */
 				if (!(var->flags & INMEMORY)) {      /* stack-arg -> register */
 					M_DLD(var->vv.regoff, REG_FP, USESTACK + (s1 * 8));

 				} else {                             /* stack-arg -> spilled  */
					var->vv.regoff = cd->stackframesize + WINSAVE_CNT + s1;
				}
			}
		}
	} /* end for */
	
	if (md->argintreguse > 0) {
		/* release scratch space */
		M_ADD_IMM(REG_SP, INT_ARG_CNT * 8, REG_SP);
	}
	
	
	/* XXX monitor enter */



	
	}
	
	/* end of header generation */ 
	
	replacementpoint = jd->code->rplpoints;

	/* walk through all basic blocks */

	for (bptr = jd->basicblocks; bptr != NULL; bptr = bptr->next) {

		bptr->mpc = (s4) (cd->mcodeptr - cd->mcodebase);

		if (bptr->flags >= BBREACHED) {

		/* branch resolving */

		codegen_resolve_branchrefs(cd, bptr);
		
		/* handle replacement points */

#if 0
		if (bptr->bitflags & BBFLAG_REPLACEMENT) {
			replacementpoint->pc = (u1*)(ptrint)bptr->mpc; /* will be resolved later */
			
			replacementpoint++;
		}
#endif

		/* copy interface registers to their destination */

		len = bptr->indepth;
		MCODECHECK(64+len);
		
#if defined(ENABLE_LSRA)
		if (opt_lsra) {
		while (len) {
			len--;
			src = bptr->invars[len];
			if ((len == bptr->indepth-1) && (bptr->type == BBTYPE_EXH)) {
					/* 				d = reg_of_var(m, src, REG_ITMP1); */
					if (!(src->flags & INMEMORY))
						d = src->vv.regoff;
					else
						d = REG_ITMP1;
					M_INTMOVE(REG_ITMP1, d);
					emit_store(jd, NULL, src, d);
				}
			}
		} else {
#endif
		while (len) {
			len--;
			var = VAR(bptr->invars[len]);
			if ((len == bptr->indepth-1) && (bptr->type == BBTYPE_EXH)) {
				d = codegen_reg_of_var(0, var, REG_ITMP1);
				M_INTMOVE(REG_ITMP1, d);
				emit_store(jd, NULL, var, d);
			}
			else {
				assert((var->flags & INOUT));
			}
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

		case ICMD_CHECKNULL:  /* ..., objectref  ==> ..., objectref           */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			emit_nullpointer_check(cd, s1);
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
			disp = dseg_add_float(cd, iptr->sx.val.f);
			M_FLD(d, REG_PV, disp);
			emit_store_dst(jd, iptr, d);
			break;
			
		case ICMD_DCONST:     /* ...  ==> ..., constant                       */

			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
			disp = dseg_add_double(cd, iptr->sx.val.d);
			M_DLD(d, REG_PV, disp);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_ACONST:     /* ...  ==> ..., constant                       */

			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				cr   = iptr->sx.val.c.ref;
				disp = dseg_add_unique_address(cd, cr);

				codegen_add_patch_ref(cd, PATCHER_aconst, cr, disp);

				M_ALD(d, REG_PV, disp);

			} 
			else {
				if (iptr->sx.val.anyptr == NULL) {
					M_INTMOVE(REG_ZERO, d);
				} 
				else {
					disp = dseg_add_address(cd, iptr->sx.val.anyptr);
					M_ALD(d, REG_PV, disp);
				}
			}
			emit_store_dst(jd, iptr, d);
			break;


		/* load/store/copy/move operations ************************************/

		case ICMD_ILOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_LLOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_ALOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_FLOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_DLOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_ISTORE:     /* ..., value  ==> ...                          */
		case ICMD_LSTORE:     /* ..., value  ==> ...                          */
		case ICMD_FSTORE:     /* ..., value  ==> ...                          */
		case ICMD_DSTORE:     /* ..., value  ==> ...                          */
		case ICMD_COPY:
		case ICMD_MOVE:

			emit_copy(jd, iptr, VAROP(iptr->s1), VAROP(iptr->dst));
			break;
	
		case ICMD_ASTORE:
			if (!(iptr->flags.bits & INS_FLAG_RETADDR))
				emit_copy(jd, iptr, VAROP(iptr->s1), VAROP(iptr->dst));
			break;


		/* pop/dup/swap operations ********************************************/

		/* attention: double and longs are only one entry in CACAO ICMDs      */

		case ICMD_POP:        /* ..., value  ==> ...                          */
		case ICMD_POP2:       /* ..., value, value  ==> ...                   */
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

		case ICMD_IINC:
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
			disp = dseg_add_float(cd, 0.0);
			M_IST (s1, REG_PV_CALLEE, disp);
			M_FLD (d, REG_PV_CALLEE, disp);
			M_CVTIF (d, d); /* rd gets translated to double target register */
			emit_store_dst(jd, iptr, d);
			break;
			
		case ICMD_I2D:
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP3);
			disp = dseg_add_float(cd, 0.0);
			M_IST (s1, REG_PV_CALLEE, disp);
			M_FLD (REG_FTMP2, REG_PV_CALLEE, disp); /* REG_FTMP2 needs to be a double temp */
			M_CVTID (REG_FTMP2, d); /* rd gets translated to double target register */
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_F2I:       /* ..., value  ==> ..., (int) value              */
			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP3);
			disp = dseg_add_float(cd, 0.0);
			M_CVTFI(s1, REG_FTMP2);
			M_FST(REG_FTMP2, REG_PV_CALLEE, disp);
			M_ILD(d, REG_PV, disp);
			emit_store_dst(jd, iptr, d);
			break;
			
			       
		case ICMD_D2I:       /* ..., value  ==> ..., (int) value             */
			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP3);
			disp = dseg_add_float(cd, 0.0);
			M_CVTDI(s1, REG_FTMP2);
			M_FST(REG_FTMP2, REG_PV, disp);
			M_ILD(d, REG_PV, disp);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_F2L:       /* ..., value  ==> ..., (long) value             */
			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP3);
			disp = dseg_add_double(cd, 0.0);
			M_CVTFL(s1, REG_FTMP2); /* FTMP2 needs to be double reg */
			M_DST(REG_FTMP2, REG_PV, disp);
			M_LDX(d, REG_PV, disp);
			emit_store_dst(jd, iptr, d);
			break;
			
		case ICMD_D2L:       /* ..., value  ==> ..., (long) value             */
			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP3);
			disp = dseg_add_double(cd, 0.0);
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
			disp = dseg_add_functionptr(cd, BUILTIN_canstore);
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
				uf = iptr->sx.s23.s3.uf;
				fieldtype = uf->fieldref->parseddesc.fd->type;
				disp      = dseg_add_unique_address(cd, uf);

				codegen_add_patch_ref(cd, PATCHER_get_putstatic, uf, disp);
			} 
			else {
				fi = iptr->sx.s23.s3.fmiref->p.field;
				fieldtype = fi->type;
				disp = dseg_add_address(cd, &(fi->value));

				if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class))
					codegen_add_patch_ref(cd, PATCHER_clinit, fi->class, disp);
  			}

			M_ALD(REG_ITMP1, REG_PV, disp);

			switch (fieldtype) {
			case TYPE_INT:
				d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
				M_ILD_INTERN(d, REG_ITMP1, 0);
				break;
			case TYPE_LNG:
				d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
				M_LDX_INTERN(d, REG_ITMP1, 0);
				break;
			case TYPE_ADR:
				d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
				M_ALD_INTERN(d, REG_ITMP1, 0);
				break;
			case TYPE_FLT:
				d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
				M_FLD_INTERN(d, REG_ITMP1, 0);
				break;
			case TYPE_DBL:				
				d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
				M_DLD_INTERN(d, REG_ITMP1, 0);
				break;
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				uf = iptr->sx.s23.s3.uf;
				fieldtype = uf->fieldref->parseddesc.fd->type;
				disp      = dseg_add_unique_address(cd, uf);

				codegen_add_patch_ref(cd, PATCHER_get_putstatic, uf, disp);
			} 
			else {
				fi = iptr->sx.s23.s3.fmiref->p.field;
				fieldtype = fi->type;
				disp = dseg_add_address(cd, &(fi->value));

				if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class))
					codegen_add_patch_ref(cd, PATCHER_clinit, fi->class, disp);
  			}

			M_ALD(REG_ITMP1, REG_PV, disp);

			switch (fieldtype) {
			case TYPE_INT:
				s1 = emit_load_s1(jd, iptr, REG_ITMP2);
				M_IST_INTERN(s1, REG_ITMP1, 0);
				break;
			case TYPE_LNG:
				s1 = emit_load_s1(jd, iptr, REG_ITMP2);
				M_STX_INTERN(s1, REG_ITMP1, 0);
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
				uf        = iptr->sx.s23.s3.uf;
				fieldtype = uf->fieldref->parseddesc.fd->type;
				disp = dseg_add_unique_address(cd, uf);

				codegen_add_patch_ref(cd, PATCHER_get_putstatic, uf, disp);
			} 
			else {
				fi        = iptr->sx.s23.s3.fmiref->p.field;
				fieldtype = fi->type;
				disp      = dseg_add_address(cd, &(fi->value));

				if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class))
					codegen_add_patch_ref(cd, PATCHER_clinit, fi->class, disp);
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
				uf = iptr->sx.s23.s3.uf;

				fieldtype = uf->fieldref->parseddesc.fd->type;
				disp      = 0;

				codegen_add_patch_ref(cd, PATCHER_get_putfield, uf, 0);
			} 
			else {
				fi        = iptr->sx.s23.s3.fmiref->p.field;
				fieldtype = fi->type;
				disp      = fi->offset;
			}

			switch (fieldtype) {
			case TYPE_INT:
				d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
				M_ILD(d, s1, disp);
				break;
			case TYPE_LNG:
				d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
				M_LDX(d, s1, disp);
				break;
			case TYPE_ADR:
				d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
				M_ALD(d, s1, disp);
				break;
			case TYPE_FLT:
				d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
				M_FLD(d, s1, disp);
				break;
			case TYPE_DBL:				
				d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
				M_DLD(d, s1, disp);
				break;
			default:
				assert(0);
				break;
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_PUTFIELD:   /* ..., objectref, value  ==> ...               */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			gen_nullptr_check(s1);

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				uf = iptr->sx.s23.s3.uf;
				fieldtype = uf->fieldref->parseddesc.fd->type;
				disp      = 0;
			}
			else {
				uf        = NULL;
				fi        = iptr->sx.s23.s3.fmiref->p.field;
				fieldtype = fi->type;
				disp      = fi->offset;
				}

			if (IS_INT_LNG_TYPE(fieldtype))
				s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			else
				s2 = emit_load_s2(jd, iptr, REG_FTMP2);

			if (INSTRUCTION_IS_UNRESOLVED(iptr))
				codegen_add_patch_ref(cd, PATCHER_get_putfield, uf, 0);

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
			default:
				assert(0);
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
				uc = iptr->sx.s23.s2.uc;

				codegen_add_patch_ref(cd, PATCHER_athrow_areturn, uc, 0);
			}
#endif /* ENABLE_VERIFIER */

			disp = dseg_add_functionptr(cd, asm_handle_exception);
			M_ALD(REG_ITMP1, REG_PV, disp);
			M_JMP(REG_ITMP3_XPC, REG_ITMP1, REG_ZERO);
			M_NOP;
			M_NOP;              /* nop ensures that XPC is less than the end */
			                    /* of basic block                            */
			ALIGNCODENOP;
			break;

		case ICMD_GOTO:         /* ... ==> ...                                */
		case ICMD_RET:          /* ... ==> ...                                */

			M_BR(0);
			codegen_add_branch_ref(cd, iptr->dst.block);
			M_NOP;
			ALIGNCODENOP;
			break;

		case ICMD_JSR:          /* ... ==> ...                                */

			M_BR(0);
			codegen_add_branch_ref(cd, iptr->sx.s23.s3.jsrtarget.block);
			M_NOP;
			ALIGNCODENOP;
			break;

		case ICMD_IFNULL:       /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			M_BEQZ(s1, 0);
			codegen_add_branch_ref(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IFNONNULL:    /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			M_BNEZ(s1, 0);
			codegen_add_branch_ref(cd, iptr->dst.block);
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
			codegen_add_branch_ref(cd, iptr->dst.block);
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
			codegen_add_branch_ref(cd, iptr->dst.block);
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
			codegen_add_branch_ref(cd, iptr->dst.block);
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
			codegen_add_branch_ref(cd, iptr->dst.block);
			M_NOP;
			break;
						
		case ICMD_IFGT:         /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (iptr->sx.val.i == 0) {
				M_BGTZ(s1, 0);
			} 
			else {
				if ((iptr->sx.val.i >= -4096) && (iptr->sx.val.i <= 4095)) {
					M_CMP_IMM(s1, iptr->sx.val.i);
				} else {
					ICONST(REG_ITMP2, iptr->sx.val.i);
					M_CMP(s1, REG_ITMP2);
				}
				M_BGT(0);
			}
			codegen_add_branch_ref(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IFGE:         /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (iptr->sx.val.i == 0) {
				M_BGEZ(s1, 0);
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
			codegen_add_branch_ref(cd, iptr->dst.block);
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
			codegen_add_branch_ref(cd, iptr->dst.block);
			M_NOP;
			break;
			
		case ICMD_IF_LLT:       /* ..., value ==> ...                         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (iptr->sx.val.l == 0) {
				M_BLTZ(s1, 0);
			} 
			else {
				if ((iptr->sx.val.l >= -4096) && (iptr->sx.val.l <= 4095)) {
					M_CMP_IMM(s1, iptr->sx.val.l);
				} 
				else {
					ICONST(REG_ITMP2, iptr->sx.val.l);
					M_CMP(s1, REG_ITMP2);
				}
				M_XBLT(0);
			}
			codegen_add_branch_ref(cd, iptr->dst.block);
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
			codegen_add_branch_ref(cd, iptr->dst.block);
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
			codegen_add_branch_ref(cd, iptr->dst.block);
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
			codegen_add_branch_ref(cd, iptr->dst.block);
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
			codegen_add_branch_ref(cd, iptr->dst.block);
			M_NOP;
			break;			
			

		case ICMD_IF_ACMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPEQ:    /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_XBEQ(0);
			codegen_add_branch_ref(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IF_ICMPEQ:    /* 32-bit compare                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_BEQ(0);
			codegen_add_branch_ref(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IF_ACMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPNE:    /* op1 = target JavaVM pc                     */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_XBNE(0);
			codegen_add_branch_ref(cd, iptr->dst.block);
			M_NOP;
			break;
			
		case ICMD_IF_ICMPNE:    /* 32-bit compare                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_BNE(0);
			codegen_add_branch_ref(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IF_LCMPLT:    /* ..., value, value ==> ...                  */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_XBLT(0);
			codegen_add_branch_ref(cd, iptr->dst.block);
			M_NOP;
			break;
			
		case ICMD_IF_ICMPLT:    /* 32-bit compare                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_BLT(0);
			codegen_add_branch_ref(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IF_LCMPGT:    /* ..., value, value ==> ...                  */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_XBGT(0);
			codegen_add_branch_ref(cd, iptr->dst.block);
			M_NOP;
			break;
			
		case ICMD_IF_ICMPGT:    /* 32-bit compare                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_BGT(0);
			codegen_add_branch_ref(cd, iptr->dst.block);
			M_NOP;
			break;

		case ICMD_IF_LCMPLE:    /* ..., value, value ==> ...                  */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_BLE(0);
			codegen_add_branch_ref(cd, iptr->dst.block);
			M_NOP;
			break;
			
		case ICMD_IF_ICMPLE:    /* 32-bit compare                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_BLE(0);
			codegen_add_branch_ref(cd, iptr->dst.block);
			M_NOP;
			break;			
	

		case ICMD_IF_LCMPGE:    /* ..., value, value ==> ...                  */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_BGE(0);
			codegen_add_branch_ref(cd, iptr->dst.block);
			M_NOP;
			break;
			
		case ICMD_IF_ICMPGE:    /* 32-bit compare                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			M_BGE(0);
			codegen_add_branch_ref(cd, iptr->dst.block);
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

				if (opt_showdisassemble) {
					M_NOP; M_NOP;
				}
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
			
			p = cd->stackframesize;

#if !defined(NDEBUG)
			if (JITDATA_HAS_FLAG_VERBOSECALL(jd))
				emit_verbosecall_exit(jd);
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

				disp = dseg_add_functionptr(cd, BUILTIN_monitorexit);
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



			M_RETURN(REG_RA_CALLEE, 8); /* implicit window restore */
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
			codegen_add_branch_ref(cd, table[0].block); /* default target */
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
				codegen_add_branch_ref(cd, lookup->target.block); 
				M_NOP;
				++lookup;
			}

			M_BR(0);
			codegen_add_branch_ref(cd, iptr->sx.s23.s3.lookupdefault.block);
			M_NOP;
			ALIGNCODENOP;
			break;
			}


		case ICMD_BUILTIN:      /* ..., arg1, arg2, arg3 ==> ...              */

			bte = iptr->sx.s23.s3.bte;
			md = bte->md;
			
			/* XXX: proper builtin calling and float args are so not implemented */
			assert(md->paramcount <= 5 && md->argfltreguse < 1);
			
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
				var = VAR(iptr->sx.s23.s2.args[s3]);

				if (var->flags & PREALLOC)
					continue;

				if (IS_INT_LNG_TYPE(var->type)) {
					if (!md->params[s3].inmemory) {
						s1 = rd->argintregs[md->params[s3].regoff];
						d = emit_load(jd, iptr, var, s1);
						M_INTMOVE(d, s1);
					} 
					else {
						d = emit_load(jd, iptr, var, REG_ITMP1);
						M_STX(d, REG_SP, USESTACK + md->params[s3].regoff * 8);
					}
				}
				else {
					if (!md->params[s3].inmemory) {
						s1 = rd->argfltregs[md->params[s3].regoff];
						d = emit_load(jd, iptr, var, s1);
						if (IS_2_WORD_TYPE(var->type))
							M_DMOV(d, s1);
						else
							M_FMOV(d, s1);
					}
					else {
						d = emit_load(jd, iptr, var, REG_FTMP1);
						if (IS_2_WORD_TYPE(var->type))
							M_DST(d, REG_SP, USESTACK + md->params[s3].regoff * 8);
						else
							M_FST(d, REG_SP, USESTACK + md->params[s3].regoff * 8);
					}
				}
			}

			switch (iptr->opc) {
			case ICMD_BUILTIN:
				disp = dseg_add_functionptr(cd, bte->fp);

				M_ALD(REG_PV_CALLER, REG_PV, disp);  /* built-in-function pointer */
				s1 = REG_PV_CALLER;

				/* c call, allocate parameter array */
				M_LDA(REG_SP, REG_SP, -(ABI_PARAMARRAY_SLOTS) * 8);

				break;

			case ICMD_INVOKESPECIAL:
				M_BEQZ(REG_OUT0, 0);
				codegen_add_nullpointerexception_ref(cd);
				M_NOP;
				/* fall through */

			case ICMD_INVOKESTATIC:
				if (lm == NULL) {
					disp = dseg_add_unique_address(cd, NULL);

					codegen_add_patch_ref(cd, PATCHER_invokestatic_special,
										um, disp);
				}
				else
					disp = dseg_add_address(cd, lm->stubroutine);

				M_ALD(REG_PV_CALLER, REG_PV, disp);          /* method pointer in pv */
				s1 = REG_PV_CALLER;
				break;

			case ICMD_INVOKEVIRTUAL:
				gen_nullptr_check(REG_OUT0);

				if (lm == NULL) {
					codegen_add_patch_ref(cd, PATCHER_invokevirtual, um, 0);

					s1 = 0;
				}
				else
					s1 = OFFSET(vftbl_t, table[0]) +
						sizeof(methodptr) * lm->vftblindex;

				M_ALD(REG_METHODPTR, REG_OUT0,
					  OFFSET(java_objectheader, vftbl));
				M_ALD(REG_PV_CALLER, REG_METHODPTR, s1);
				s1 = REG_PV_CALLER;
				break;

			case ICMD_INVOKEINTERFACE:
				gen_nullptr_check(rd->argintregs[0]);

				if (lm == NULL) {
					codegen_add_patch_ref(cd, PATCHER_invokeinterface, um, 0);

					s1 = 0;
					s2 = 0;
				} 
				else {
					s1 = OFFSET(vftbl_t, interfacetable[0]) -
						sizeof(methodptr*) * lm->class->index;

					s2 = sizeof(methodptr) * (lm - lm->class->methods);
				}

				M_ALD(REG_METHODPTR, REG_OUT0,
					  OFFSET(java_objectheader, vftbl));
				M_ALD(REG_METHODPTR, REG_METHODPTR, s1);
				M_ALD(REG_PV_CALLER, REG_METHODPTR, s2);
				s1 = REG_PV_CALLER;
				break;
			}

			/* generate the actual call */

			M_JMP(REG_RA_CALLER, s1, REG_ZERO);
			M_NOP;
			disp = (s4) (cd->mcodeptr - cd->mcodebase);
			/* REG_RA holds the value of the jmp instruction, therefore +8 */
			M_LDA(REG_ZERO, REG_RA_CALLER, -disp + 8); 

			if (iptr->opc == ICMD_BUILTIN) {
				/* remove param slots */
				M_LDA(REG_SP, REG_SP, (ABI_PARAMARRAY_SLOTS) * 8);
			}


			/* actually only used for ICMD_BUILTIN */

			if (INSTRUCTION_MUST_CHECK(iptr)) {
				M_BEQZ(REG_RESULT_CALLER, 0);
				codegen_add_fillinstacktrace_ref(cd);
				M_NOP;
			}

			/* store return value */

			d = md->returntype.type;

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
				s4         superindex;

				if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
					super      = NULL;
					superindex = 0;
				}
				else {
					super = iptr->sx.s23.s3.c.cls;
					superindex = super->index;
				}

#if defined(ENABLE_THREADS)
				codegen_threadcritrestart(cd, cd->mcodeptr - cd->mcodebase);
#endif

				s1 = emit_load_s1(jd, iptr, REG_ITMP1);

				/* calculate interface checkcast code size */

				s2 = 8;
				if (super == NULL)
					s2 += (opt_shownops ? PATCHER_CALL_INSTRUCTIONS : 0);

				/* calculate class checkcast code size */

				s3 = 10;
				if (super == NULL)
					s3 += (opt_shownops ? PATCHER_CALL_INSTRUCTIONS : 0);

				/* if class is not resolved, check which code to call */

				if (super == NULL) {
					M_BEQZ(s1, 5 + (opt_shownops ? PATCHER_CALL_INSTRUCTIONS : 0) + s2 + 2 + s3 + 1);
					M_NOP;

					cr   = iptr->sx.s23.s3.c.ref;
					disp = dseg_add_unique_s4(cd, 0);         /* super->flags */

					codegen_add_patch_ref(cd, PATCHER_checkcast_instanceof_flags,
										  cr, disp);

					M_ILD(REG_ITMP2, REG_PV, disp);
					M_AND_IMM(REG_ITMP2, ACC_INTERFACE, REG_ITMP2);
					M_BEQZ(REG_ITMP2, s2 + 2 + 2);
					M_NOP;
				}

				/* interface checkcast code */

				if ((super == NULL) || (super->flags & ACC_INTERFACE)) {
					if (super == NULL) {
						cr = iptr->sx.s23.s3.c.ref;

						codegen_add_patch_ref(cd, PATCHER_checkcast_interface,
											  cr, 0);
					}
					else {
						M_BEQZ(s1, s2 + 2);
						M_NOP;
					}

					M_ALD(REG_ITMP2, s1, OFFSET(java_objectheader, vftbl));
					M_ILD(REG_ITMP3, REG_ITMP2,
 							OFFSET(vftbl_t, interfacetablelength));
					M_ADD_IMM(REG_ITMP3, -superindex, REG_ITMP3);
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
			            /* on sparc we always add 2 to the size of the code we want  */
			            /* branch over. (1 for branch delay nop, 1 since the base is */
			            /* the address of the branch instruction */
						M_BR(s3 + 2);
						M_NOP;
					}
				}

				/* class checkcast code */

				if ((super == NULL) || !(super->flags & ACC_INTERFACE)) {
					if (super == NULL) {
						cr   = iptr->sx.s23.s3.c.ref;
						disp = dseg_add_unique_address(cd, NULL);

						codegen_add_patch_ref(cd,
											PATCHER_checkcast_instanceof_class,
											  cr, disp);
					}
					else {
						disp = dseg_add_address(cd, super->vftbl);

						M_BEQZ(s1, s3 + 2);
						M_NOP;
					}

					M_ALD(REG_ITMP2, s1, OFFSET(java_objectheader, vftbl));
					M_ALD(REG_ITMP3, REG_PV, disp);
#if defined(ENABLE_THREADS)
					codegen_threadcritstart(cd, cd->mcodeptr - cd->mcodebase);
#endif
					M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, baseval));
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

				disp = dseg_add_address(cd, iptr->sx.s23.s3.c.cls);

				if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
					cr   = iptr->sx.s23.s3.c.ref;
					disp = dseg_add_unique_address(cd, NULL);

					codegen_add_patch_ref(cd, PATCHER_builtin_arraycheckcast,
										  cr, disp);
				}
				else
					disp = dseg_add_address(cd, iptr->sx.s23.s3.c.cls);

				M_ALD(rd->argintregs[1], REG_PV, disp);
				disp = dseg_add_functionptr(cd, BUILTIN_arraycheckcast);
				M_ALD(REG_ITMP3, REG_PV, disp);
				M_LDA(REG_SP, -6*8, REG_SP); /* PARAMARRAY SLOTS */
				M_JMP(REG_RA_CALLER, REG_ITMP3, REG_ZERO);
				M_NOP;
				M_LDA(REG_SP, 6*8, REG_SP);

				s1 = emit_load_s1(jd, iptr, REG_ITMP1);
				M_BEQZ(REG_RESULT_CALLER, 0);
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

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				super = NULL;
				superindex = 0;
				supervftbl = NULL;

			} else {
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
				s2 += (opt_shownops ? PATCHER_CALL_INSTRUCTIONS : 0);

			/* calculate class instanceof code size */

			s3 = 8;
			if (super == NULL)
				s3 += (opt_shownops ? PATCHER_CALL_INSTRUCTIONS : 0);

			M_CLR(d);

			/* if class is not resolved, check which code to call */

			if (super == NULL) {
				M_BEQZ(s1, 5 + (opt_shownops ? PATCHER_CALL_INSTRUCTIONS : 0) + s2 + 2 + s3);
				M_NOP;

				cr   = iptr->sx.s23.s3.c.ref;
				disp = dseg_add_unique_s4(cd, 0);             /* super->flags */

				codegen_add_patch_ref(cd, PATCHER_checkcast_instanceof_flags,
									  cr, disp);

				M_ILD(REG_ITMP3, REG_PV, disp);
				M_AND_IMM(REG_ITMP3, ACC_INTERFACE, REG_ITMP3);
				M_BEQZ(REG_ITMP3, s2 + 2 + 2);
				M_NOP;
			}

			/* interface instanceof code */

			if ((super == NULL) || (super->flags & ACC_INTERFACE)) {
				if (super == NULL) {
					cr = iptr->sx.s23.s3.c.ref;

					codegen_add_patch_ref(cd, PATCHER_instanceof_interface,
										  cr, 0);
				}
				else {
					M_BEQZ(s1, s2 + 2);
					M_NOP;
				}

				M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
				M_ILD(REG_ITMP3, REG_ITMP1, OFFSET(vftbl_t, interfacetablelength));
				M_CMP_IMM(REG_ITMP3, superindex);
				M_BLE(4);
				M_NOP;
				M_ALD(REG_ITMP1, REG_ITMP1,
					  (s4) (OFFSET(vftbl_t, interfacetable[0]) -
							superindex * sizeof(methodptr*)));
				M_CMOVRNE_IMM(REG_ITMP1, 1, d);      /* REG_ITMP1 != 0  */

				if (super == NULL) {
					M_BR(s3 + 2);
					M_NOP;
				}
			}

			/* class instanceof code */

			if ((super == NULL) || !(super->flags & ACC_INTERFACE)) {
				if (super == NULL) {
					cr   = iptr->sx.s23.s3.c.ref;
					disp = dseg_add_unique_address(cd, NULL);

					codegen_add_patch_ref(cd, PATCHER_checkcast_instanceof_class,
										  cr, disp);
				}
				else {
					disp = dseg_add_address(cd, supervftbl);

					M_BEQZ(s1, s3 + 2);
					M_NOP;
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
				M_SUB(REG_ITMP1, REG_ITMP3, REG_ITMP1);
				M_CMP(REG_ITMP1, REG_ITMP2);
				M_XCMOVULE_IMM(1, d);
			}
			emit_store_dst(jd, iptr, d);
			}
			break;


		default:
			*exceptionptr = new_internalerror("Unknown ICMD %d", iptr->opc);
			return false;
			
	} /* switch */
		
	} /* for instruction */
	

		
	} /* if (bptr -> flags >= BBREACHED) */
	} /* for basic block */
	
	dseg_createlinenumbertable(cd);

	/* generate exception and patcher stubs */

	emit_exception_stubs(jd);
	emit_patcher_stubs(jd);
#if defined(ENABLE_REPLACEMENT)
	emit_replacement_stubs(jd);
#endif /* defined(ENABLE_REPLACEMENT) */

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

	/* rewrite registers and params */
	md_native_reg_setup(jd);
	md_native_param_alloc(nmd);

	/* initialize variables */

	md = m->parseddesc;
	nativeparams = (m->flags & ACC_STATIC) ? 2 : 1;

	/* calculate stack frame size */

	cd->stackframesize =
		sizeof(stackframeinfo) / SIZEOF_VOID_P +
		sizeof(localref_table) / SIZEOF_VOID_P +
		md->paramcount +                /* for saving arguments over calls    */
		nmd->memuse +  /* nmd knows about the native stackframe layout */
		WINSAVE_CNT;

	/* create method header */

	(void) dseg_add_unique_address(cd, code);              /* CodeinfoPointer */
	(void) dseg_add_unique_s4(cd, cd->stackframesize * 8); /* FrameSize       */
	(void) dseg_add_unique_s4(cd, 0);                      /* IsSync          */
	(void) dseg_add_unique_s4(cd, 0);                      /* IsLeaf          */
	(void) dseg_add_unique_s4(cd, 0);                      /* IntSave         */
	(void) dseg_add_unique_s4(cd, 0);                      /* FltSave         */
	(void) dseg_addlinenumbertablesize(cd);
	(void) dseg_add_unique_s4(cd, 0);                      /* ExTableSize     */

	/* generate stub code */

	M_SAVE(REG_SP, -cd->stackframesize * 8, REG_SP); /* build up stackframe    */

#if !defined(NDEBUG)
	if (JITDATA_HAS_FLAG_VERBOSECALL(jd))
		emit_verbosecall_enter(jd);
#endif

	/* get function address (this must happen before the stackframeinfo) */

	funcdisp = dseg_add_functionptr(cd, f);

#if !defined(WITH_STATIC_CLASSPATH)
	if (f == NULL) {
		codegen_addpatchref(cd, PATCHER_resolve_native, m, funcdisp);

		if (opt_showdisassemble) {
			M_NOP; M_NOP;
		}
	}
#endif

	/* save float argument registers */

	for (i = 0, j = 0; i < md->paramcount && i < FLT_ARG_CNT; i++) {
		if (IS_FLT_DBL_TYPE(md->paramtypes[i].type)) {
			M_DST(rd->argfltregs[i], REG_SP, j * 8);
			j++;
		}
	}

	/* prepare data structures for native function call */

	M_ADD_IMM(REG_FP, BIAS, REG_OUT0); /* datasp == top of the stack frame (absolute == +BIAS) */
	M_MOV(REG_PV_CALLEE, REG_OUT1);
	M_MOV(REG_FP, REG_OUT2); /* java sp */
	M_MOV(REG_RA_CALLEE, REG_OUT3);
	disp = dseg_add_functionptr(cd, codegen_start_native_call);
	M_ALD(REG_ITMP3, REG_PV_CALLEE, disp);
	M_JMP(REG_RA_CALLER, REG_ITMP3, REG_ZERO);
	M_NOP; /* XXX fill me! */

	/* restore float argument registers */

	for (i = 0, j = 0; i < md->paramcount && i < FLT_ARG_CNT; i++) {
		if (IS_FLT_DBL_TYPE(md->paramtypes[i].type)) {
			M_DLD(rd->argfltregs[i], REG_SP, j * 8);
			j++;
		}
	}

	/* copy or spill arguments to new locations */
	int num_fltregargs = 0;
	int fltregarg_inswap[16];
	for (i = md->paramcount - 1, j = i + nativeparams; i >= 0; i--, j--) {
		t = md->paramtypes[i].type;

		if (IS_INT_LNG_TYPE(t)) {
			if (!md->params[i].inmemory) {
				s1 = rd->argintregs[md->params[i].regoff];
				/* s1 refers to the old window, transpose */
				s1 = REG_WINDOW_TRANSPOSE(s1);

				if (!nmd->params[j].inmemory) {
					s2 = nat_argintregs[nmd->params[j].regoff];
					M_INTMOVE(s1, s2);
				} else {
					s2 = nmd->params[j].regoff;
					M_AST(s1, REG_SP, USESTACK_PARAMS + s2 * 8);
				}

			} else {
				s1 = md->params[i].regoff + cd->stackframesize;
				s2 = nmd->params[j].regoff;
				M_ALD(REG_ITMP1, REG_SP, USESTACK_PARAMS + s1 * 8);
				M_AST(REG_ITMP1, REG_SP, USESTACK_PARAMS + s2 * 8);
			}

		} else {
			if (!md->params[i].inmemory) {
				s1 = rd->argfltregs[md->params[i].regoff];

				if (!nmd->params[j].inmemory) {
					/* no mapping to regs needed, native flt args use regoff */
					s2 = nmd->params[j].regoff;
					
					/* we cannot move flt regs to their native arg locations directly */
					M_DMOV(s1, s2 + 16);
					fltregarg_inswap[num_fltregargs] = s2;
					num_fltregargs++;
					printf("flt arg swap to %d\n", s2 + 16);

				} else {
					s2 = nmd->params[j].regoff;
					if (IS_2_WORD_TYPE(t))
						M_DST(s1, REG_SP, USESTACK_PARAMS + s2 * 8);
					else
						M_FST(s1, REG_SP, USESTACK_PARAMS + s2 * 8);
				}

			} else {
				s1 = md->params[i].regoff + cd->stackframesize;
				s2 = nmd->params[j].regoff;
				if (IS_2_WORD_TYPE(t)) {
					M_DLD(REG_FTMP1, REG_SP, USESTACK_PARAMS + s1 * 8);
					M_DST(REG_FTMP1, REG_SP, USESTACK_PARAMS + s2 * 8);
				} else {
					M_FLD(REG_FTMP1, REG_SP, USESTACK_PARAMS + s1 * 8);
					M_FST(REG_FTMP1, REG_SP, USESTACK_PARAMS + s2 * 8);
				}
			}
		}
	}
	
	/* move swapped float args to target regs */
	for (i = 0; i < num_fltregargs; i++) {
		s1 = fltregarg_inswap[i];
		M_DMOV(s1 + 16, s1);
		printf("float arg to target reg: %d ==> %d\n", s1+16, s1);
	}


	/* put class into second argument register */

	if (m->flags & ACC_STATIC) {
		disp = dseg_add_address(cd, m->class);
		M_ALD(REG_OUT1, REG_PV_CALLEE, disp);
	}

	/* put env into first argument register */

	disp = dseg_add_address(cd, _Jv_env);
	M_ALD(REG_OUT0, REG_PV_CALLEE, disp);

	/* do the native function call */

	M_ALD(REG_ITMP3, REG_PV_CALLEE, funcdisp); /* load adress of native method       */
	M_JMP(REG_RA_CALLER, REG_ITMP3, REG_ZERO); /* call native method                 */
	M_NOP;                              /* delay slot                         */

	/* save return value */

	if (md->returntype.type != TYPE_VOID) {
		if (IS_INT_LNG_TYPE(md->returntype.type))
			M_MOV(REG_RESULT_CALLER, REG_RESULT_CALLEE);
		else
			M_DST(REG_FRESULT, REG_SP, USESTACK_PARAMS);
	}
	
	/* Note: native functions return float values in %f0 (see ABI) */
	/* we handle this by doing M_FLD below. (which will load the lower word into %f1) */

#if !defined(NDEBUG)
	/* But for the trace function we need to put a flt result into %f1 */
	if (JITDATA_HAS_FLAG_VERBOSECALL(jd)) {
		if (!IS_2_WORD_TYPE(md->returntype.type))
			M_FLD(REG_FRESULT, REG_SP, USESTACK_PARAMS);
		emit_verbosecall_exit(jd);
	}
#endif

	/* remove native stackframe info */

	M_ADD_IMM(REG_FP, BIAS, REG_OUT0); /* datasp, like above */
	disp = dseg_add_functionptr(cd, codegen_finish_native_call);
	M_ALD(REG_ITMP3, REG_PV_CALLEE, disp);
	M_JMP(REG_RA_CALLER, REG_ITMP3, REG_ZERO);
	M_NOP; /* XXX fill me! */
	M_MOV(REG_RESULT_CALLER, REG_ITMP2_XPTR);

	/* restore float return value, int return value already in our return reg */

	if (md->returntype.type != TYPE_VOID) {
		if (IS_FLT_DBL_TYPE(md->returntype.type)) {
			if (IS_2_WORD_TYPE(md->returntype.type))
				M_DLD(REG_FRESULT, REG_SP, USESTACK_PARAMS);
			else
				M_FLD(REG_FRESULT, REG_SP, USESTACK_PARAMS);
		}
	}

	/* check for exception */

	M_BNEZ(REG_ITMP2_XPTR, 4);          /* if no exception then return        */
	M_NOP;

	M_RETURN(REG_RA_CALLEE, 8); /* implicit window restore */
	M_NOP;
#if 0	
	M_RESTORE(REG_ZERO, 0, REG_ZERO);   /* restore callers window (DELAY)     */

	M_RET(REG_RA_CALLER, 8);            /* return to caller                   */
	M_NOP;                              /* DELAY SLOT                         */
#endif

	/* handle exception */
	
	disp = dseg_add_functionptr(cd, asm_handle_nat_exception);
	M_ALD(REG_ITMP3, REG_PV, disp);     /* load asm exception handler address */
	M_JMP(REG_ZERO, REG_ITMP3, REG_ZERO);/* jump to asm exception handler     */
	M_MOV(REG_RA_CALLEE, REG_ITMP3_XPC); /* get exception address (DELAY)    */

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
