/* src/vm/jit/i386/codegen.c - machine code generator for i386

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
            Christian Thalinger

   Changes: Joseph Wenninger

   $Id: codegen.c 2676 2005-06-13 16:20:32Z twisti $

*/


#include <assert.h>
#include <stdio.h>

#include "config.h"

#include "vm/jit/i386/md-abi.h"
#include "vm/jit/i386/md-abi.inc"

#include "cacao/cacao.h"
#include "native/jni.h"
#include "native/native.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/stringlocal.h"
#include "vm/tables.h"
#include "vm/utf8.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/jit.h"
#include "vm/jit/parse.h"
#include "vm/jit/patcher.h"
#include "vm/jit/reg.h"
#include "vm/jit/i386/codegen.h"
#include "vm/jit/i386/emitfuncs.h"
#include "vm/jit/i386/types.h"
#include "vm/jit/i386/asmoffsets.h"

/*******************************************************************************

    include independent code generation stuff -- include after register
    descriptions to avoid extern definitions

*******************************************************************************/

#include "vm/jit/codegen.inc"
#include "vm/jit/reg.inc"
#ifdef LSRA
#ifdef LSRA_USES_REG_RES
#include "vm/jit/i386/icmd_uses_reg_res.inc"
#endif
#include "vm/jit/lsra.inc"
#endif

void codegen_stubcalled() {
	log_text("Stub has been called");
}


/* codegen *********************************************************************

   Generates machine code.

*******************************************************************************/

void codegen(methodinfo *m, codegendata *cd, registerdata *rd)
{
	s4     len, s1, s2, s3, d;
	ptrint a;
	stackptr      src;
	varinfo      *var;
	basicblock   *bptr;
	instruction  *iptr;
	s4 parentargs_base;
	u2 currentline;
	s4 fpu_st_offset = 0;
	methodinfo     *lm;                 /* local methodinfo for ICMD_INVOKE*  */
	builtintable_entry *bte;
	methoddesc     *md;

	exceptiontable *ex;

	{
	s4 i, p, t, l;
  	s4 savedregs_num = 0;
	s4 stack_off = 0;

	/* space to save used callee saved registers */

	savedregs_num += (rd->savintregcnt - rd->maxsavintreguse);
	savedregs_num += 2 * (rd->savfltregcnt - rd->maxsavfltreguse); /* float register are saved on 2 4 Byte stackslots */

	parentargs_base = rd->maxmemuse + savedregs_num;

	   
#if defined(USE_THREADS)           /* space to save argument of monitor_enter */

	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		parentargs_base++;

#endif

/* align Stackframe to 8 Byte */
/* 	if ((parentargs_base+1) & 1) */
/* 		parentargs_base++;   */       

/* create method header */

	(void) dseg_addaddress(cd, m);                          /* MethodPointer  */
	(void) dseg_adds4(cd, parentargs_base * 4);             /* FrameSize      */

#if defined(USE_THREADS)

	/* IsSync contains the offset relative to the stack pointer for the
	   argument of monitor_exit used in the exception handler. Since the
	   offset could be zero and give a wrong meaning of the flag it is
	   offset by one.
	*/

	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		(void) dseg_adds4(cd, (rd->maxmemuse + 1) * 4);         /* IsSync     */
	else

#endif

	(void) dseg_adds4(cd, 0);                                   /* IsSync     */
	                                       
	(void) dseg_adds4(cd, m->isleafmethod);                     /* IsLeaf     */
	(void) dseg_adds4(cd, rd->savintregcnt - rd->maxsavintreguse); /* IntSave */
	(void) dseg_adds4(cd, rd->savfltregcnt - rd->maxsavfltreguse); /* FltSave */

	/* adds a reference for the length of the line number counter. We don't
	   know the size yet, since we evaluate the information during code
	   generation, to save one additional iteration over the whole
	   instructions. During code optimization the position could have changed
	   to the information gotten from the class file */
	(void) dseg_addlinenumbertablesize(cd);

	(void) dseg_adds4(cd, cd->exceptiontablelength);        /* ExTableSize    */
	
	/* create exception table */

	for (ex = cd->exceptiontable; ex != NULL; ex = ex->down) {
		dseg_addtarget(cd, ex->start);
   		dseg_addtarget(cd, ex->end);
		dseg_addtarget(cd, ex->handler);
		(void) dseg_addaddress(cd, ex->catchtype.cls);
	}

	
	/* initialize mcode variables */
	
	cd->mcodeptr = cd->mcodebase;
	cd->mcodeend = (s4 *) (cd->mcodebase + cd->mcodesize);
	MCODECHECK(128 + m->paramcount);

	/* create stack frame (if necessary) */

	if (parentargs_base) {
		i386_alu_imm_reg(cd, ALU_SUB, parentargs_base * 4, REG_SP);
	}

	/* save return address and used callee saved registers */

  	p = parentargs_base;
	for (i = rd->savintregcnt - 1; i >= rd->maxsavintreguse; i--) {
 		p--; i386_mov_reg_membase(cd, rd->savintregs[i], REG_SP, p * 4);
	}
	for (i = rd->savfltregcnt - 1; i >= rd->maxsavfltreguse; i--) {
		p-=2; i386_fld_reg(cd, rd->savfltregs[i]); i386_fstpl_membase(cd, REG_SP, p * 4);
	}

	/* take arguments out of register or stack frame */

	md = m->parseddesc;

	stack_off = 0;
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
				log_text("integer register argument");
				assert(0);
				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
					/* rd->argintregs[md->params[p].regoff -> var->regoff     */
				} else {                             /* reg arg -> spilled    */
					/* rd->argintregs[md->params[p].regoff -> var->regoff * 4 */
				}
			} else {                                 /* stack arguments       */
				if (!(var->flags & INMEMORY)) {      /* stack arg -> register */
					i386_mov_membase_reg(           /* + 4 for return address */
					   cd, REG_SP, (parentargs_base + s1) * 4 + 4, var->regoff);
					                                /* + 4 for return address */
				} else {                             /* stack arg -> spilled  */
					if (!IS_2_WORD_TYPE(t)) {
#if 0
						i386_mov_membase_reg(       /* + 4 for return address */
					         cd, REG_SP, (parentargs_base + s1) * 4 + 4,
							 REG_ITMP1);    
						i386_mov_reg_membase(
						    cd, REG_ITMP1, REG_SP, var->regoff * 4);
#else
						                  /* reuse Stackslotand avoid copying */
						var->regoff = parentargs_base + s1 + 1;
#endif

					} else {
#if 0
						i386_mov_membase_reg(       /* + 4 for return address */
						    cd, REG_SP, (parentargs_base + s1) * 4 + 4,
							REG_ITMP1);
						i386_mov_reg_membase(
						    cd, REG_ITMP1, REG_SP, var->regoff * 4);
						i386_mov_membase_reg(       /* + 4 for return address */
                            cd, REG_SP, (parentargs_base + s1) * 4 + 4 + 4,
                            REG_ITMP1);             
						i386_mov_reg_membase(
					        cd, REG_ITMP1, REG_SP, var->regoff * 4 + 4);
#else
						                  /* reuse Stackslotand avoid copying */
						var->regoff = parentargs_base + s1 + 1;
#endif
					}
				}
			}
		
		} else {                                     /* floating args         */
			if (!md->params[p].inmemory) {           /* register arguments    */
				log_text("There are no float argument registers!");
				assert(0);
				if (!(var->flags & INMEMORY)) {  /* reg arg -> register   */
					/* rd->argfltregs[md->params[p].regoff -> var->regoff     */
				} else {			             /* reg arg -> spilled    */
					/* rd->argfltregs[md->params[p].regoff -> var->regoff * 4 */
				}

			} else {                                 /* stack arguments       */
				if (!(var->flags & INMEMORY)) {      /* stack-arg -> register */
					if (t == TYPE_FLT) {
						i386_flds_membase(
                            cd, REG_SP, (parentargs_base + s1) * 4 + 4);
						fpu_st_offset++;
						i386_fstp_reg(cd, var->regoff + fpu_st_offset);
						fpu_st_offset--;

					} else {
						i386_fldl_membase(
                            cd, REG_SP, (parentargs_base + s1) * 4 + 4);
						fpu_st_offset++;
						i386_fstp_reg(cd, var->regoff + fpu_st_offset);
						fpu_st_offset--;
					}

				} else {                             /* stack-arg -> spilled  */
#if 0
					i386_mov_membase_reg(
                        cd, REG_SP, (parentargs_base + s1) * 4 + 4, REG_ITMP1);
					i386_mov_reg_membase(
					    cd, REG_ITMP1, REG_SP, var->regoff * 4);
					if (t == TYPE_FLT) {
						i386_flds_membase(
						    cd, REG_SP, (parentargs_base + s1) * 4 + 4);
						i386_fstps_membase(cd, REG_SP, var->regoff * 4);
					} else {
						i386_fldl_membase(
                            cd, REG_SP, (parentargs_base + s1) * 4 + 4);
						i386_fstpl_membase(cd, REG_SP, var->regoff * 4);
					}
#else
						                  /* reuse Stackslotand avoid copying */
						var->regoff = parentargs_base + s1 + 1;
#endif
				}
			}
		}
	}  /* end for */

	/* call monitorenter function */

#if defined(USE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		s1 = rd->maxmemuse;

		if (m->flags & ACC_STATIC) {
			i386_mov_imm_reg(cd, (ptrint) m->class, REG_ITMP1);
			i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, s1 * 4);
			i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, 0);
			i386_mov_imm_reg(cd, (ptrint) BUILTIN_staticmonitorenter, REG_ITMP1);
			i386_call_reg(cd, REG_ITMP1);

		} else {
			i386_mov_membase_reg(cd, REG_SP, parentargs_base * 4 + 4, REG_ITMP1);
			i386_test_reg_reg(cd, REG_ITMP1, REG_ITMP1);
			i386_jcc(cd, I386_CC_Z, 0);
			codegen_addxnullrefs(cd, cd->mcodeptr);
			i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, s1 * 4);
			i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, 0);
			i386_mov_imm_reg(cd, (ptrint) BUILTIN_monitorenter, REG_ITMP1);
			i386_call_reg(cd, REG_ITMP1);
		}
	}			
#endif

	/* copy argument registers to stack and call trace function with pointer
	   to arguments on stack.
	*/

	if (runverbose) {
		stack_off = 0;
		int pa = INT_TMP_CNT * 4 + TRACE_ARGS_NUM * 8 + 4 + 4 + parentargs_base * 4;

		M_ISUB_IMM(INT_TMP_CNT * 4 + TRACE_ARGS_NUM * 8 + 4, REG_SP);

		/* save temporary registers for leaf methods */

		for (p = 0; p < INT_TMP_CNT; p++)
			M_IST(rd->tmpintregs[p], REG_SP, TRACE_ARGS_NUM * 8 + 4 + p * 4);

		for (p = 0, l = 0; p < md->paramcount && p < TRACE_ARGS_NUM; p++) {
			t = md->paramtypes[p].type;

			if (IS_INT_LNG_TYPE(t)) {
				if (IS_2_WORD_TYPE(t)) {
					i386_mov_membase_reg(cd, REG_SP, pa + stack_off, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, p * 8);
					i386_mov_membase_reg(cd, REG_SP, pa + stack_off + 4, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, p * 8 + 4);

 				} else if (t == TYPE_ADR) {
/* 				} else { */
					i386_mov_membase_reg(cd, REG_SP, pa + stack_off, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, p * 8);
					i386_alu_reg_reg(cd, ALU_XOR, REG_ITMP1, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, p * 8 + 4);

 				} else {
 					i386_mov_membase_reg(cd, REG_SP, pa + stack_off, EAX);
 					i386_cltd(cd);
 					i386_mov_reg_membase(cd, EAX, REG_SP, p * 8);
 					i386_mov_reg_membase(cd, EDX, REG_SP, p * 8 + 4);
				}

			} else {
				if (!IS_2_WORD_TYPE(t)) {
					i386_flds_membase(cd, REG_SP, pa + stack_off);
					i386_fstps_membase(cd, REG_SP, p * 8);
					i386_alu_reg_reg(cd, ALU_XOR, REG_ITMP1, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, p * 8 + 4);

				} else {
					i386_fldl_membase(cd, REG_SP, pa + stack_off);
					i386_fstpl_membase(cd, REG_SP, p * 8);
				}
			}
			stack_off += (IS_2_WORD_TYPE(t)) ? 8 : 4;
		}

		/* fill up the remaining arguments */
		i386_alu_reg_reg(cd, ALU_XOR, REG_ITMP1, REG_ITMP1);
		for (p = md->paramcount; p < TRACE_ARGS_NUM; p++) {
			i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, p * 8);
			i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, p * 8 + 4);
		}

		i386_mov_imm_membase(cd, (ptrint) m, REG_SP, TRACE_ARGS_NUM * 8);
  		i386_mov_imm_reg(cd, (ptrint) builtin_trace_args, REG_ITMP1);
		i386_call_reg(cd, REG_ITMP1);

		/* restore temporary registers for leaf methods */

		for (p = 0; p < INT_TMP_CNT; p++)
			M_ILD(rd->tmpintregs[p], REG_SP, TRACE_ARGS_NUM * 8 + 4 + p * 4);

		M_IADD_IMM(INT_TMP_CNT * 4 + TRACE_ARGS_NUM * 8 + 4, REG_SP);
	}

	}

	/* end of header generation */

	/* walk through all basic blocks */
	for (bptr = m->basicblocks; bptr != NULL; bptr = bptr->next) {

		bptr->mpc = (s4) (cd->mcodeptr - cd->mcodebase);

		if (bptr->flags >= BBREACHED) {

		/* branch resolving */

		branchref *brefs;
		for (brefs = bptr->branchrefs; brefs != NULL; brefs = brefs->next) {
			gen_resolvebranch(cd->mcodebase + brefs->branchpos, 
			                  brefs->branchpos,
							  bptr->mpc);
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
					if (!IS_2_WORD_TYPE(src->type)) {
						if (bptr->type == BBTYPE_SBR) {
							/* 							d = reg_of_var(m, src, REG_ITMP1); */
							if (!(src->flags & INMEMORY))
								d = src->regoff;
							else
								d = REG_ITMP1;

							i386_pop_reg(cd, d);
							store_reg_to_var_int(src, d);

						} else if (bptr->type == BBTYPE_EXH) {
							/* 							d = reg_of_var(m, src, REG_ITMP1); */
							if (!(src->flags & INMEMORY))
								d = src->regoff;
							else
								d = REG_ITMP1;
							M_INTMOVE(REG_ITMP1, d);
							store_reg_to_var_int(src, d);
						}

					} else {
						log_text("copy interface registers(EXH, SBR): longs have to be in memory (begin 1)");
						assert(0);
					}
				}
				src = src->prev;
			}

		} else {
#endif
			while (src != NULL) {
				len--;
				if ((len == 0) && (bptr->type != BBTYPE_STD)) {
					if (!IS_2_WORD_TYPE(src->type)) {
						if (bptr->type == BBTYPE_SBR) {
							d = reg_of_var(rd, src, REG_ITMP1);
							i386_pop_reg(cd, d);
							store_reg_to_var_int(src, d);
					} else if (bptr->type == BBTYPE_EXH) {
						d = reg_of_var(rd, src, REG_ITMP1);
						M_INTMOVE(REG_ITMP1, d);
						store_reg_to_var_int(src, d);
					}

				} else {
					log_text("copy interface registers: longs have to be in memory (begin 1)");
					assert(0);
				}

			} else {
				d = reg_of_var(rd, src, REG_ITMP1);
				if ((src->varkind != STACKVAR)) {
					s2 = src->type;
					if (IS_FLT_DBL_TYPE(s2)) {
						s1 = rd->interfaces[len][s2].regoff;
						if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
							M_FLTMOVE(s1, d);

						} else {
							if (s2 == TYPE_FLT) {
								i386_flds_membase(cd, REG_SP, s1 * 4);

							} else {
								i386_fldl_membase(cd, REG_SP, s1 * 4);
							}
						}
						store_reg_to_var_flt(src, d);

					} else {
						s1 = rd->interfaces[len][s2].regoff;
						if (!IS_2_WORD_TYPE(rd->interfaces[len][s2].type)) {
							if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
								M_INTMOVE(s1, d);

							} else {
								i386_mov_membase_reg(cd, REG_SP, s1 * 4, d);
							}
							store_reg_to_var_int(src, d);

						} else {
							if (rd->interfaces[len][s2].flags & INMEMORY) {
  								M_LNGMEMMOVE(s1, src->regoff);

							} else {
								log_text("copy interface registers: longs have to be in memory (begin 2)");
								assert(0);
							}
						}
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
				dseg_addlinenumber(cd, iptr->line, cd->mcodeptr);
				currentline = iptr->line;
			}

			MCODECHECK(100);   /* XXX are 100 bytes enough? */

		switch (iptr->opc) {
		case ICMD_INLINE_START:
		case ICMD_INLINE_END:
			/* REG_RES Register usage: see lsra.inc icmd_uses_tmp */
			/* EAX: NO ECX: NO EDX: NO */
			break;

		case ICMD_NOP:        /* ...  ==> ...                                 */
			/* REG_RES Register usage: see lsra.inc icmd_uses_tmp */
			/* EAX: NO ECX: NO EDX: NO */
			break;

		case ICMD_CHECKNULL:  /* ..., objectref  ==> ..., objectref           */
			/* REG_RES Register usage: see lsra.inc icmd_uses_tmp */
			/* EAX: NO ECX: NO EDX: NO */
			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, ALU_CMP, 0, REG_SP, src->regoff * 4);

			} else {
				i386_test_reg_reg(cd, src->regoff, src->regoff);
			}
			i386_jcc(cd, I386_CC_Z, 0);
			codegen_addxnullrefs(cd, cd->mcodeptr);
			break;

		/* constant operations ************************************************/

		case ICMD_ICONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.i = constant                    */

			/* REG_RES Register usage: see lsra.inc icmd_uses_tmp */
			/* EAX: NO ECX: NO EDX: NO */
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				i386_mov_imm_membase(cd, iptr->val.i, REG_SP, iptr->dst->regoff * 4);

			} else {
				if (iptr->val.i == 0) {
					i386_alu_reg_reg(cd, ALU_XOR, d, d);

				} else {
					i386_mov_imm_reg(cd, iptr->val.i, d);
				}
			}
			break;

		case ICMD_LCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.l = constant                    */

			/* REG_RES Register usage: see lsra.inc icmd_uses_tmp */
			/* EAX: NO ECX: NO EDX: NO */
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				i386_mov_imm_membase(cd, iptr->val.l, REG_SP, iptr->dst->regoff * 4);
				i386_mov_imm_membase(cd, iptr->val.l >> 32, REG_SP, iptr->dst->regoff * 4 + 4);
				
			} else {
				log_text("LCONST: longs have to be in memory");
				assert(0);
			}
			break;

		case ICMD_FCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.f = constant                    */
			/* REG_RES Register usage: see lsra.inc icmd_uses_tmp */
			/* EAX: YES ECX: NO EDX: NO */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			if (iptr->val.f == 0.0) {
				i386_fldz(cd);
				fpu_st_offset++;

				/* -0.0 */
				if (iptr->val.i == 0x80000000) {
					i386_fchs(cd);
				}

			} else if (iptr->val.f == 1.0) {
				i386_fld1(cd);
				fpu_st_offset++;

			} else if (iptr->val.f == 2.0) {
				i386_fld1(cd);
				i386_fld1(cd);
				i386_faddp(cd);
				fpu_st_offset++;

			} else {
  				a = dseg_addfloat(cd, iptr->val.f);
				i386_mov_imm_reg(cd, 0, REG_ITMP1);
				dseg_adddata(cd, cd->mcodeptr);
				i386_flds_membase(cd, REG_ITMP1, a);
				fpu_st_offset++;
			}
			store_reg_to_var_flt(iptr->dst, d);
			break;
		
		case ICMD_DCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.d = constant                    */
			/* REG_RES Register usage: see lsra.inc icmd_uses_tmp */
			/* EAX: YES ECX: NO EDX: NO */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			if (iptr->val.d == 0.0) {
				i386_fldz(cd);
				fpu_st_offset++;

				/* -0.0 */
				if (iptr->val.l == 0x8000000000000000LL) {
					i386_fchs(cd);
				}

			} else if (iptr->val.d == 1.0) {
				i386_fld1(cd);
				fpu_st_offset++;

			} else if (iptr->val.d == 2.0) {
				i386_fld1(cd);
				i386_fld1(cd);
				i386_faddp(cd);
				fpu_st_offset++;

			} else {
				a = dseg_adddouble(cd, iptr->val.d);
				i386_mov_imm_reg(cd, 0, REG_ITMP1);
				dseg_adddata(cd, cd->mcodeptr);
				i386_fldl_membase(cd, REG_ITMP1, a);
				fpu_st_offset++;
			}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_ACONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.a = constant                    */
			/* REG_RES Register usage: see lsra.inc icmd_uses_tmp */
			/* EAX: YES ECX: NO EDX: NO */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				i386_mov_imm_membase(cd, (s4) iptr->val.a, REG_SP, iptr->dst->regoff * 4);

			} else {
				if ((s4) iptr->val.a == 0) {
					i386_alu_reg_reg(cd, ALU_XOR, d, d);

				} else {
					i386_mov_imm_reg(cd, (s4) iptr->val.a, d);
				}
			}
			break;


		/* load/store operations **********************************************/

		case ICMD_ILOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_ALOAD:      /* op1 = local variable                         */
			/* REG_RES Register usage: see lsra.inc icmd_uses_tmp */
			/* EAX: YES ECX: NO EDX: NO */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1)) {
				break;
			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (iptr->dst->flags & INMEMORY) {
				if (var->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, var->regoff * 4, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);

				} else {
					i386_mov_reg_membase(cd, var->regoff, REG_SP, iptr->dst->regoff * 4);
				}

			} else {
				if (var->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, var->regoff * 4, iptr->dst->regoff);

				} else {
					M_INTMOVE(var->regoff, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_LLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */
			/* REG_RES Register usage: see lsra.inc icmd_uses_tmp */
			/* EAX: NO ECX: NO EDX: NO */
  
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1)) {
				break;
			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (iptr->dst->flags & INMEMORY) {
				if (var->flags & INMEMORY) {
					M_LNGMEMMOVE(var->regoff, iptr->dst->regoff);

				} else {
					log_text("LLOAD: longs have to be in memory");
					assert(0);
				}

			} else {
				log_text("LLOAD: longs have to be in memory");
				assert(0);
			}
			break;

		case ICMD_FLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
  			if ((iptr->dst->varkind == LOCALVAR) &&
  			    (iptr->dst->varnum == iptr->op1)) {
    				break;
  			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY) {
				i386_flds_membase(cd, REG_SP, var->regoff * 4);
				fpu_st_offset++;
			} else {
				i386_fld_reg(cd, var->regoff + fpu_st_offset);
				fpu_st_offset++;
			}
  			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
  			if ((iptr->dst->varkind == LOCALVAR) &&
  			    (iptr->dst->varnum == iptr->op1)) {
    				break;
  			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY) {
				i386_fldl_membase(cd, REG_SP, var->regoff * 4);
				fpu_st_offset++;
			} else {
				i386_fld_reg(cd, var->regoff + fpu_st_offset);
				fpu_st_offset++;
			}
  			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_ISTORE:     /* ..., value  ==> ...                          */
		case ICMD_ASTORE:     /* op1 = local variable                         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO */

			if ((src->varkind == LOCALVAR) &&
			    (src->varnum == iptr->op1)) {
				break;
			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, var->regoff * 4);
					
				} else {
					i386_mov_reg_membase(cd, src->regoff, REG_SP, var->regoff * 4);
				}

			} else {
				var_to_reg_int(s1, src, var->regoff);
				M_INTMOVE(s1, var->regoff);
			}
			break;

		case ICMD_LSTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO */

			if ((src->varkind == LOCALVAR) &&
			    (src->varnum == iptr->op1)) {
				break;
			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					M_LNGMEMMOVE(src->regoff, var->regoff);

				} else {
					log_text("LSTORE: longs have to be in memory");
					assert(0);
				}

			} else {
				log_text("LSTORE: longs have to be in memory");
				assert(0);
			}
			break;

		case ICMD_FSTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO */

			if ((src->varkind == LOCALVAR) &&
			    (src->varnum == iptr->op1)) {
				break;
			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				var_to_reg_flt(s1, src, REG_FTMP1);
				i386_fstps_membase(cd, REG_SP, var->regoff * 4);
				fpu_st_offset--;
			} else {
				var_to_reg_flt(s1, src, var->regoff);
/*  				M_FLTMOVE(s1, var->regoff); */
				i386_fstp_reg(cd, var->regoff + fpu_st_offset);
				fpu_st_offset--;
			}
			break;

		case ICMD_DSTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO */

			if ((src->varkind == LOCALVAR) &&
			    (src->varnum == iptr->op1)) {
				break;
			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				var_to_reg_flt(s1, src, REG_FTMP1);
				i386_fstpl_membase(cd, REG_SP, var->regoff * 4);
				fpu_st_offset--;
			} else {
				var_to_reg_flt(s1, src, var->regoff);
/*  				M_FLTMOVE(s1, var->regoff); */
				i386_fstp_reg(cd, var->regoff + fpu_st_offset);
				fpu_st_offset--;
			}
			break;


		/* pop/dup/swap operations ********************************************/

		/* attention: double and longs are only one entry in CACAO ICMDs      */

		case ICMD_POP:        /* ..., value  ==> ...                          */
		case ICMD_POP2:       /* ..., value, value  ==> ...                   */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO */
			break;

		case ICMD_DUP:        /* ..., a ==> ..., a, a                         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO */
			M_COPY(src, iptr->dst);
			break;

		case ICMD_DUP2:       /* ..., a, b ==> ..., a, b, a, b                */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO */

			M_COPY(src,       iptr->dst);
			M_COPY(src->prev, iptr->dst->prev);
			break;

		case ICMD_DUP_X1:     /* ..., a, b ==> ..., b, a, b                   */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO */

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

		case ICMD_DUP2_X1:    /* ..., a, b, c ==> ..., b, c, a, b, c          */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO */

			M_COPY(src,             iptr->dst);
			M_COPY(src->prev,       iptr->dst->prev);
			M_COPY(src->prev->prev, iptr->dst->prev->prev);
			M_COPY(iptr->dst,       iptr->dst->prev->prev->prev);
			M_COPY(iptr->dst->prev, iptr->dst->prev->prev->prev);
			break;

		case ICMD_DUP2_X2:    /* ..., a, b, c, d ==> ..., c, d, a, b, c, d    */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO */

			M_COPY(src,                   iptr->dst);
			M_COPY(src->prev,             iptr->dst->prev);
			M_COPY(src->prev->prev,       iptr->dst->prev->prev);
			M_COPY(src->prev->prev->prev, iptr->dst->prev->prev->prev);
			M_COPY(iptr->dst,             iptr->dst->prev->prev->prev->prev);
			M_COPY(iptr->dst->prev,       iptr->dst->prev->prev->prev->prev->prev);
			break;

		case ICMD_SWAP:       /* ..., a, b ==> ..., b, a                      */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO */

			M_COPY(src,       iptr->dst->prev);
			M_COPY(src->prev, iptr->dst);
			break;


		/* integer operations *************************************************/

		case ICMD_INEG:       /* ..., value  ==> ..., - value                 */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_neg_membase(cd, REG_SP, iptr->dst->regoff * 4);

					} else {
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
						i386_neg_reg(cd, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
					}

				} else {
					i386_mov_reg_membase(cd, src->regoff, REG_SP, iptr->dst->regoff * 4);
					i386_neg_membase(cd, REG_SP, iptr->dst->regoff * 4);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, iptr->dst->regoff);
					i386_neg_reg(cd, iptr->dst->regoff);

				} else {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					i386_neg_reg(cd, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_LNEG:       /* ..., value  ==> ..., - value                 */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_neg_membase(cd, REG_SP, iptr->dst->regoff * 4);
						i386_alu_imm_membase(cd, ALU_ADC, 0, REG_SP, iptr->dst->regoff * 4 + 4);
						i386_neg_membase(cd, REG_SP, iptr->dst->regoff * 4 + 4);

					} else {
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
						i386_neg_reg(cd, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 4 + 4, REG_ITMP1);
						i386_alu_imm_reg(cd, ALU_ADC, 0, REG_ITMP1);
						i386_neg_reg(cd, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4 + 4);
					}
				}
			}
			break;

		case ICMD_I2L:        /* ..., value  ==> ..., value                   */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: YES */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, EAX);
					i386_cltd(cd);
					i386_mov_reg_membase(cd, EAX, REG_SP, iptr->dst->regoff * 4);
					i386_mov_reg_membase(cd, EDX, REG_SP, iptr->dst->regoff * 4 + 4);

				} else {
					M_INTMOVE(src->regoff, EAX);
					i386_cltd(cd);
					i386_mov_reg_membase(cd, EAX, REG_SP, iptr->dst->regoff * 4);
					i386_mov_reg_membase(cd, EDX, REG_SP, iptr->dst->regoff * 4 + 4);
				}
			}
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
					i386_shift_imm_reg(cd, I386_SHL, 24, REG_ITMP1);
					i386_shift_imm_reg(cd, I386_SAR, 24, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);

				} else {
					i386_mov_reg_membase(cd, src->regoff, REG_SP, iptr->dst->regoff * 4);
					i386_shift_imm_membase(cd, I386_SHL, 24, REG_SP, iptr->dst->regoff * 4);
					i386_shift_imm_membase(cd, I386_SAR, 24, REG_SP, iptr->dst->regoff * 4);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, iptr->dst->regoff);
					i386_shift_imm_reg(cd, I386_SHL, 24, iptr->dst->regoff);
					i386_shift_imm_reg(cd, I386_SAR, 24, iptr->dst->regoff);

				} else {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					i386_shift_imm_reg(cd, I386_SHL, 24, iptr->dst->regoff);
					i386_shift_imm_reg(cd, I386_SAR, 24, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_INT2CHAR:   /* ..., value  ==> ..., value                   */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_imm_membase(cd, ALU_AND, 0x0000ffff, REG_SP, iptr->dst->regoff * 4);

					} else {
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
						i386_alu_imm_reg(cd, ALU_AND, 0x0000ffff, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
					}

				} else {
					i386_mov_reg_membase(cd, src->regoff, REG_SP, iptr->dst->regoff * 4);
					i386_alu_imm_membase(cd, ALU_AND, 0x0000ffff, REG_SP, iptr->dst->regoff * 4);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, iptr->dst->regoff);
					i386_alu_imm_reg(cd, ALU_AND, 0x0000ffff, iptr->dst->regoff);

				} else {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					i386_alu_imm_reg(cd, ALU_AND, 0x0000ffff, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_INT2SHORT:  /* ..., value  ==> ..., value                   */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
					i386_shift_imm_reg(cd, I386_SHL, 16, REG_ITMP1);
					i386_shift_imm_reg(cd, I386_SAR, 16, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);

				} else {
					i386_mov_reg_membase(cd, src->regoff, REG_SP, iptr->dst->regoff * 4);
					i386_shift_imm_membase(cd, I386_SHL, 16, REG_SP, iptr->dst->regoff * 4);
					i386_shift_imm_membase(cd, I386_SAR, 16, REG_SP, iptr->dst->regoff * 4);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, iptr->dst->regoff);
					i386_shift_imm_reg(cd, I386_SHL, 16, iptr->dst->regoff);
					i386_shift_imm_reg(cd, I386_SAR, 16, iptr->dst->regoff);

				} else {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					i386_shift_imm_reg(cd, I386_SHL, 16, iptr->dst->regoff);
					i386_shift_imm_reg(cd, I386_SAR, 16, iptr->dst->regoff);
				}
			}
			break;


		case ICMD_IADD:       /* ..., val1, val2  ==> ..., val1 + val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: NO EDX: NO */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ialu(cd, ALU_ADD, src, iptr);
			break;

		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ialuconst(cd, ALU_ADD, src, iptr);
			break;

		case ICMD_LADD:       /* ..., val1, val2  ==> ..., val1 + val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: NO EDX: NO */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
						i386_alu_reg_membase(cd, ALU_ADD, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4 + 4, REG_ITMP1);
						i386_alu_reg_membase(cd, ALU_ADC, REG_ITMP1, REG_SP, iptr->dst->regoff * 4 + 4);

					} else if (src->prev->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
						i386_alu_reg_membase(cd, ALU_ADD, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 4 + 4, REG_ITMP1);
						i386_alu_reg_membase(cd, ALU_ADC, REG_ITMP1, REG_SP, iptr->dst->regoff * 4 + 4);

					} else {
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
						i386_alu_membase_reg(cd, ALU_ADD, REG_SP, src->regoff * 4, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4 + 4, REG_ITMP1);
						i386_alu_membase_reg(cd, ALU_ADC, REG_SP, src->regoff * 4 + 4, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4 + 4);
					}

				}
			}
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.l = constant                             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO */
			/* else path can never happen? longs stay in memory! */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_imm_membase(cd, ALU_ADD, iptr->val.l, REG_SP, iptr->dst->regoff * 4);
						i386_alu_imm_membase(cd, ALU_ADC, iptr->val.l >> 32, REG_SP, iptr->dst->regoff * 4 + 4);

					} else {
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
						i386_alu_imm_reg(cd, ALU_ADD, iptr->val.l, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 4 + 4, REG_ITMP1);
						i386_alu_imm_reg(cd, ALU_ADC, iptr->val.l >> 32, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4 + 4);
					}
				}
			}
			break;

		case ICMD_ISUB:       /* ..., val1, val2  ==> ..., val1 - val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: NO EDX: NO */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
						i386_alu_reg_membase(cd, ALU_SUB, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);

					} else {
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
						i386_alu_membase_reg(cd, ALU_SUB, REG_SP, src->regoff * 4, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
					}

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, REG_ITMP1);
					i386_alu_membase_reg(cd, ALU_SUB, REG_SP, src->regoff * 4, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						i386_alu_reg_membase(cd, ALU_SUB, src->regoff, REG_SP, iptr->dst->regoff * 4);

					} else {
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
						i386_alu_reg_reg(cd, ALU_SUB, src->regoff, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
					}

				} else {
					i386_mov_reg_membase(cd, src->prev->regoff, REG_SP, iptr->dst->regoff * 4);
					i386_alu_reg_membase(cd, ALU_SUB, src->regoff, REG_SP, iptr->dst->regoff * 4);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, d);
					i386_alu_membase_reg(cd, ALU_SUB, REG_SP, src->regoff * 4, d);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, d);
					i386_alu_membase_reg(cd, ALU_SUB, REG_SP, src->regoff * 4, d);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					/* workaround for reg alloc */
					if (src->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
						i386_alu_reg_reg(cd, ALU_SUB, src->regoff, REG_ITMP1);
						M_INTMOVE(REG_ITMP1, d);

					} else {
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, d);
						i386_alu_reg_reg(cd, ALU_SUB, src->regoff, d);
					}

				} else {
					/* workaround for reg alloc */
					if (src->regoff == iptr->dst->regoff) {
						M_INTMOVE(src->prev->regoff, REG_ITMP1);
						i386_alu_reg_reg(cd, ALU_SUB, src->regoff, REG_ITMP1);
						M_INTMOVE(REG_ITMP1, d);

					} else {
						M_INTMOVE(src->prev->regoff, d);
						i386_alu_reg_reg(cd, ALU_SUB, src->regoff, d);
					}
				}
			}
			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ialuconst(cd, ALU_SUB, src, iptr);
			break;

		case ICMD_LSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: NO EDX: NO */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
						i386_alu_reg_membase(cd, ALU_SUB, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 4 + 4, REG_ITMP1);
						i386_alu_reg_membase(cd, ALU_SBB, REG_ITMP1, REG_SP, iptr->dst->regoff * 4 + 4);

					} else {
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
						i386_alu_membase_reg(cd, ALU_SUB, REG_SP, src->regoff * 4, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4 + 4, REG_ITMP1);
						i386_alu_membase_reg(cd, ALU_SBB, REG_SP, src->regoff * 4 + 4, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4 + 4);
					}
				}
			}
			break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* val.l = constant                             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO */
			/* else path can never happen? longs stay in memory! */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_imm_membase(cd, ALU_SUB, iptr->val.l, REG_SP, iptr->dst->regoff * 4);
						i386_alu_imm_membase(cd, ALU_SBB, iptr->val.l >> 32, REG_SP, iptr->dst->regoff * 4 + 4);

					} else {
						/* TODO: could be size optimized with lea -- see gcc output */
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
						i386_alu_imm_reg(cd, ALU_SUB, iptr->val.l, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 4 + 4, REG_ITMP1);
						i386_alu_imm_reg(cd, ALU_SBB, iptr->val.l >> 32, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4 + 4);
					}
				}
			}
			break;

		case ICMD_IMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: NO EDX: NO OUTPUT: EAX*/ /* EDX really not destroyed by IMUL? */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
					i386_imul_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
					i386_imul_reg_reg(cd, src->prev->regoff, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
					i386_imul_reg_reg(cd, src->regoff, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);

				} else {
					i386_mov_reg_reg(cd, src->prev->regoff, REG_ITMP1);
					i386_imul_reg_reg(cd, src->regoff, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, iptr->dst->regoff);
					i386_imul_membase_reg(cd, REG_SP, src->regoff * 4, iptr->dst->regoff);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
					i386_imul_membase_reg(cd, REG_SP, src->regoff * 4, iptr->dst->regoff);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					i386_imul_membase_reg(cd, REG_SP, src->prev->regoff * 4, iptr->dst->regoff);

				} else {
					if (src->regoff == iptr->dst->regoff) {
						i386_imul_reg_reg(cd, src->prev->regoff, iptr->dst->regoff);

					} else {
						M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
						i386_imul_reg_reg(cd, src->regoff, iptr->dst->regoff);
					}
				}
			}
			break;

		case ICMD_IMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.i = constant                             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO OUTPUT: EAX*/ /* EDX really not destroyed by IMUL? */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_imul_imm_membase_reg(cd, iptr->val.i, REG_SP, src->regoff * 4, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);

				} else {
					i386_imul_imm_reg_reg(cd, iptr->val.i, src->regoff, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_imul_imm_membase_reg(cd, iptr->val.i, REG_SP, src->regoff * 4, iptr->dst->regoff);

				} else {
					i386_imul_imm_reg_reg(cd, iptr->val.i, src->regoff, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_LMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: S|YES OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, EAX);             /* mem -> EAX             */
					/* optimize move EAX -> REG_ITMP3 is slower??? */
/*    					i386_mov_reg_reg(cd, EAX, REG_ITMP3); */
					i386_mul_membase(cd, REG_SP, src->regoff * 4);                            /* mem * EAX -> EDX:EAX   */

					/* TODO: optimize move EAX -> REG_ITMP3 */
  					i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4 + 4, REG_ITMP2);   /* mem -> ITMP3           */
					i386_imul_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP2);            /* mem * ITMP3 -> ITMP3   */
					i386_alu_reg_reg(cd, ALU_ADD, REG_ITMP2, EDX);                      /* ITMP3 + EDX -> EDX     */

					i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP2);       /* mem -> ITMP3           */
					i386_imul_membase_reg(cd, REG_SP, src->regoff * 4 + 4, REG_ITMP2);        /* mem * ITMP3 -> ITMP3   */

					i386_alu_reg_reg(cd, ALU_ADD, REG_ITMP2, EDX);                      /* ITMP3 + EDX -> EDX     */
					i386_mov_reg_membase(cd, EAX, REG_SP, iptr->dst->regoff * 4);
					i386_mov_reg_membase(cd, EDX, REG_SP, iptr->dst->regoff * 4 + 4);
				}
			}
			break;

		case ICMD_LMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.l = constant                             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: S|YES OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_mov_imm_reg(cd, iptr->val.l, EAX);                                   /* imm -> EAX             */
					i386_mul_membase(cd, REG_SP, src->regoff * 4);                            /* mem * EAX -> EDX:EAX   */
					/* TODO: optimize move EAX -> REG_ITMP3 */
					i386_mov_imm_reg(cd, iptr->val.l >> 32, REG_ITMP2);                       /* imm -> ITMP3           */
					i386_imul_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP2);            /* mem * ITMP3 -> ITMP3   */

					i386_alu_reg_reg(cd, ALU_ADD, REG_ITMP2, EDX);                      /* ITMP3 + EDX -> EDX     */
					i386_mov_imm_reg(cd, iptr->val.l, REG_ITMP2);                             /* imm -> ITMP3           */
					i386_imul_membase_reg(cd, REG_SP, src->regoff * 4 + 4, REG_ITMP2);        /* mem * ITMP3 -> ITMP3   */

					i386_alu_reg_reg(cd, ALU_ADD, REG_ITMP2, EDX);                      /* ITMP3 + EDX -> EDX     */
					i386_mov_reg_membase(cd, EAX, REG_SP, iptr->dst->regoff * 4);
					i386_mov_reg_membase(cd, EDX, REG_SP, iptr->dst->regoff * 4 + 4);
				}
			}
			break;

		case ICMD_IDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: S|YES OUTPUT: EAX*/ /* Really uses EDX? */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			var_to_reg_int(s1, src, REG_ITMP2);
  			gen_div_check(src);
	        if (src->prev->flags & INMEMORY) {
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, EAX);

			} else {
				M_INTMOVE(src->prev->regoff, EAX);
			}
			
			i386_alu_imm_reg(cd, ALU_CMP, 0x80000000, EAX);    /* check as described in jvm spec */
			i386_jcc(cd, I386_CC_NE, 3 + 6);
			i386_alu_imm_reg(cd, ALU_CMP, -1, s1);
			i386_jcc(cd, I386_CC_E, 1 + 2);

  			i386_cltd(cd);
			i386_idiv_reg(cd, s1);

			if (iptr->dst->flags & INMEMORY) {
				i386_mov_reg_membase(cd, EAX, REG_SP, iptr->dst->regoff * 4);

			} else {
				M_INTMOVE(EAX, iptr->dst->regoff);
			}
			break;

		case ICMD_IREM:       /* ..., val1, val2  ==> ..., val1 % val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: S|YES OUTPUT: EDX*/ 


			d = reg_of_var(rd, iptr->dst, REG_NULL);
			var_to_reg_int(s1, src, REG_ITMP2);
  			gen_div_check(src);
			if (src->prev->flags & INMEMORY) {
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, EAX);

			} else {
				M_INTMOVE(src->prev->regoff, EAX);
			}
			
			i386_alu_imm_reg(cd, ALU_CMP, 0x80000000, EAX);    /* check as described in jvm spec */
			i386_jcc(cd, I386_CC_NE, 2 + 3 + 6);
			i386_alu_reg_reg(cd, ALU_XOR, EDX, EDX);
			i386_alu_imm_reg(cd, ALU_CMP, -1, s1);
			i386_jcc(cd, I386_CC_E, 1 + 2);

  			i386_cltd(cd);
			i386_idiv_reg(cd, s1);

			if (iptr->dst->flags & INMEMORY) {
				i386_mov_reg_membase(cd, EDX, REG_SP, iptr->dst->regoff * 4);

			} else {
				M_INTMOVE(EDX, iptr->dst->regoff);
			}
			break;

		case ICMD_IDIVPOW2:   /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			/* TODO: optimize for `/ 2' */
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);

			M_INTMOVE(s1, d);
			i386_test_reg_reg(cd, d, d);
			a = 2;
			CALCIMMEDIATEBYTES(a, (1 << iptr->val.i) - 1);
			i386_jcc(cd, I386_CC_NS, a);
			i386_alu_imm_reg(cd, ALU_ADD, (1 << iptr->val.i) - 1, d);
				
			i386_shift_imm_reg(cd, I386_SAR, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LDIVPOW2:   /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					a = 2;
					CALCIMMEDIATEBYTES(a, (1 << iptr->val.i) - 1);
					a += 3;
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4 + 4, REG_ITMP2);

					i386_test_reg_reg(cd, REG_ITMP2, REG_ITMP2);
					i386_jcc(cd, I386_CC_NS, a);
					i386_alu_imm_reg(cd, ALU_ADD, (1 << iptr->val.i) - 1, REG_ITMP1);
					i386_alu_imm_reg(cd, ALU_ADC, 0, REG_ITMP2);
					i386_shrd_imm_reg_reg(cd, iptr->val.i, REG_ITMP2, REG_ITMP1);
					i386_shift_imm_reg(cd, I386_SAR, iptr->val.i, REG_ITMP2);

					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
					i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, iptr->dst->regoff * 4 + 4);
				}
			}
			break;

		case ICMD_IREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.i = constant                             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: NO OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if (s1 == d) {
				M_INTMOVE(s1, REG_ITMP1);
				s1 = REG_ITMP1;
			} 

			a = 2;
			a += 2;
			a += 2;
			CALCIMMEDIATEBYTES(a, iptr->val.i);
			a += 2;

			/* TODO: optimize */
			M_INTMOVE(s1, d);
			i386_alu_imm_reg(cd, ALU_AND, iptr->val.i, d);
			i386_test_reg_reg(cd, s1, s1);
			i386_jcc(cd, I386_CC_GE, a);
 			i386_mov_reg_reg(cd, s1, d);
  			i386_neg_reg(cd, d);
  			i386_alu_imm_reg(cd, ALU_AND, iptr->val.i, d);
			i386_neg_reg(cd, d);

/*  			M_INTMOVE(s1, EAX); */
/*  			i386_cltd(cd); */
/*  			i386_alu_reg_reg(cd, ALU_XOR, EDX, EAX); */
/*  			i386_alu_reg_reg(cd, ALU_SUB, EDX, EAX); */
/*  			i386_alu_reg_reg(cd, ALU_AND, iptr->val.i, EAX); */
/*  			i386_alu_reg_reg(cd, ALU_XOR, EDX, EAX); */
/*  			i386_alu_reg_reg(cd, ALU_SUB, EDX, EAX); */
/*  			M_INTMOVE(EAX, d); */

/*  			i386_alu_reg_reg(cd, ALU_XOR, d, d); */
/*  			i386_mov_imm_reg(cd, iptr->val.i, ECX); */
/*  			i386_shrd_reg_reg(cd, s1, d); */
/*  			i386_shift_imm_reg(cd, I386_SHR, 32 - iptr->val.i, d); */

			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.l = constant                             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					/* Intel algorithm -- does not work, because constant is wrong */
/*  					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1); */
/*  					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4 + 4, REG_ITMP3); */

/*  					M_INTMOVE(REG_ITMP1, REG_ITMP2); */
/*  					i386_test_reg_reg(cd, REG_ITMP3, REG_ITMP3); */
/*  					i386_jcc(cd, I386_CC_NS, offset); */
/*  					i386_alu_imm_reg(cd, ALU_ADD, (1 << iptr->val.l) - 1, REG_ITMP2); */
/*  					i386_alu_imm_reg(cd, ALU_ADC, 0, REG_ITMP3); */
					
/*  					i386_shrd_imm_reg_reg(cd, iptr->val.l, REG_ITMP3, REG_ITMP2); */
/*  					i386_shift_imm_reg(cd, I386_SAR, iptr->val.l, REG_ITMP3); */
/*  					i386_shld_imm_reg_reg(cd, iptr->val.l, REG_ITMP2, REG_ITMP3); */

/*  					i386_shift_imm_reg(cd, I386_SHL, iptr->val.l, REG_ITMP2); */

/*  					i386_alu_reg_reg(cd, ALU_SUB, REG_ITMP2, REG_ITMP1); */
/*  					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4 + 4, REG_ITMP2); */
/*  					i386_alu_reg_reg(cd, ALU_SBB, REG_ITMP3, REG_ITMP2); */

/*  					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4); */
/*  					i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, iptr->dst->regoff * 4 + 4); */

					/* Alpha algorithm */
					a = 3;
					CALCOFFSETBYTES(a, REG_SP, src->regoff * 4);
					a += 3;
					CALCOFFSETBYTES(a, REG_SP, src->regoff * 4 + 4);

					a += 2;
					a += 3;
					a += 2;

					/* TODO: hmm, don't know if this is always correct */
					a += 2;
					CALCIMMEDIATEBYTES(a, iptr->val.l & 0x00000000ffffffff);
					a += 2;
					CALCIMMEDIATEBYTES(a, iptr->val.l >> 32);

					a += 2;
					a += 3;
					a += 2;

					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4 + 4, REG_ITMP2);
					
					i386_alu_imm_reg(cd, ALU_AND, iptr->val.l, REG_ITMP1);
					i386_alu_imm_reg(cd, ALU_AND, iptr->val.l >> 32, REG_ITMP2);
					i386_alu_imm_membase(cd, ALU_CMP, 0, REG_SP, src->regoff * 4 + 4);
					i386_jcc(cd, I386_CC_GE, a);

					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4 + 4, REG_ITMP2);
					
					i386_neg_reg(cd, REG_ITMP1);
					i386_alu_imm_reg(cd, ALU_ADC, 0, REG_ITMP2);
					i386_neg_reg(cd, REG_ITMP2);
					
					i386_alu_imm_reg(cd, ALU_AND, iptr->val.l, REG_ITMP1);
					i386_alu_imm_reg(cd, ALU_AND, iptr->val.l >> 32, REG_ITMP2);
					
					i386_neg_reg(cd, REG_ITMP1);
					i386_alu_imm_reg(cd, ALU_ADC, 0, REG_ITMP2);
					i386_neg_reg(cd, REG_ITMP2);

					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
					i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, iptr->dst->regoff * 4 + 4);
				}
			}
			break;

		case ICMD_ISHL:       /* ..., val1, val2  ==> ..., val1 << val2       */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: D|S|YES EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ishift(cd, I386_SHL, src, iptr);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ishiftconst(cd, I386_SHL, src, iptr);
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: D|S|YES EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ishift(cd, I386_SAR, src, iptr);
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ishiftconst(cd, I386_SAR, src, iptr);
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: D|S|YES EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ishift(cd, I386_SHR, src, iptr);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ishiftconst(cd, I386_SHR, src, iptr);
			break;

		case ICMD_LSHL:       /* ..., val1, val2  ==> ..., val1 << val2       */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: YES EDX: S|YES OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY ){
				if (src->prev->flags & INMEMORY) {
/*  					if (src->prev->regoff == iptr->dst->regoff) { */
/*  						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1); */

/*  						if (src->flags & INMEMORY) { */
/*  							i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, ECX); */
/*  						} else { */
/*  							M_INTMOVE(src->regoff, ECX); */
/*  						} */

/*  						i386_test_imm_reg(cd, 32, ECX); */
/*  						i386_jcc(cd, I386_CC_E, 2 + 2); */
/*  						i386_mov_reg_reg(cd, REG_ITMP1, REG_ITMP2); */
/*  						i386_alu_reg_reg(cd, ALU_XOR, REG_ITMP1, REG_ITMP1); */
						
/*  						i386_shld_reg_membase(cd, REG_ITMP1, REG_SP, src->prev->regoff * 4 + 4); */
/*  						i386_shift_membase(cd, I386_SHL, REG_SP, iptr->dst->regoff * 4); */

/*  					} else { */
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4 + 4, REG_ITMP3);
						
						if (src->flags & INMEMORY) {
							i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, ECX);
						} else {
							M_INTMOVE(src->regoff, ECX);
						}
						
						i386_test_imm_reg(cd, 32, ECX);
						i386_jcc(cd, I386_CC_E, 2 + 2);
						i386_mov_reg_reg(cd, REG_ITMP1, REG_ITMP3);
						i386_alu_reg_reg(cd, ALU_XOR, REG_ITMP1, REG_ITMP1);
						
						i386_shld_reg_reg(cd, REG_ITMP1, REG_ITMP3);
						i386_shift_reg(cd, I386_SHL, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
						i386_mov_reg_membase(cd, REG_ITMP3, REG_SP, iptr->dst->regoff * 4 + 4);
/*  					} */
				}
			}
			break;

        case ICMD_LSHLCONST:  /* ..., value  ==> ..., value << constant       */
 			                  /* val.i = constant                             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY ) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4 + 4, REG_ITMP2);

				if (iptr->val.i & 0x20) {
					i386_mov_reg_reg(cd, REG_ITMP1, REG_ITMP2);
					i386_alu_reg_reg(cd, ALU_XOR, REG_ITMP1, REG_ITMP1);
					i386_shld_imm_reg_reg(cd, iptr->val.i & 0x3f, REG_ITMP1, REG_ITMP2);

				} else {
					i386_shld_imm_reg_reg(cd, iptr->val.i & 0x3f, REG_ITMP1, REG_ITMP2);
					i386_shift_imm_reg(cd, I386_SHL, iptr->val.i & 0x3f, REG_ITMP1);
				}

				i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
				i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, iptr->dst->regoff * 4 + 4);
			}
			break;

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: YES S|EDX: YES OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY ){
				if (src->prev->flags & INMEMORY) {
/*  					if (src->prev->regoff == iptr->dst->regoff) { */
  						/* TODO: optimize */
/*  						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1); */
/*  						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4 + 4, REG_ITMP2); */

/*  						if (src->flags & INMEMORY) { */
/*  							i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, ECX); */
/*  						} else { */
/*  							M_INTMOVE(src->regoff, ECX); */
/*  						} */

/*  						i386_test_imm_reg(cd, 32, ECX); */
/*  						i386_jcc(cd, I386_CC_E, 2 + 3); */
/*  						i386_mov_reg_reg(cd, REG_ITMP2, REG_ITMP1); */
/*  						i386_shift_imm_reg(cd, I386_SAR, 31, REG_ITMP2); */
						
/*  						i386_shrd_reg_reg(cd, REG_ITMP2, REG_ITMP1); */
/*  						i386_shift_reg(cd, I386_SAR, REG_ITMP2); */
/*  						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4); */
/*  						i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, iptr->dst->regoff * 4 + 4); */

/*  					} else { */
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4 + 4, REG_ITMP3);

						if (src->flags & INMEMORY) {
							i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, ECX);
						} else {
							M_INTMOVE(src->regoff, ECX);
						}

						i386_test_imm_reg(cd, 32, ECX);
						i386_jcc(cd, I386_CC_E, 2 + 3);
						i386_mov_reg_reg(cd, REG_ITMP3, REG_ITMP1);
						i386_shift_imm_reg(cd, I386_SAR, 31, REG_ITMP3);
						
						i386_shrd_reg_reg(cd, REG_ITMP3, REG_ITMP1);
						i386_shift_reg(cd, I386_SAR, REG_ITMP3);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
						i386_mov_reg_membase(cd, REG_ITMP3, REG_SP, iptr->dst->regoff * 4 + 4);
/*  					} */
				}
			}
			break;

		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY ) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4 + 4, REG_ITMP2);

				if (iptr->val.i & 0x20) {
					i386_mov_reg_reg(cd, REG_ITMP2, REG_ITMP1);
					i386_shift_imm_reg(cd, I386_SAR, 31, REG_ITMP2);
					i386_shrd_imm_reg_reg(cd, iptr->val.i & 0x3f, REG_ITMP2, REG_ITMP1);

				} else {
					i386_shrd_imm_reg_reg(cd, iptr->val.i & 0x3f, REG_ITMP2, REG_ITMP1);
					i386_shift_imm_reg(cd, I386_SAR, iptr->val.i & 0x3f, REG_ITMP2);
				}

				i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
				i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, iptr->dst->regoff * 4 + 4);
			}
			break;

		case ICMD_LUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: YES EDX: S|YES OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY ){
				if (src->prev->flags & INMEMORY) {
/*  					if (src->prev->regoff == iptr->dst->regoff) { */
  						/* TODO: optimize */
/*  						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1); */
/*  						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4 + 4, REG_ITMP2); */

/*  						if (src->flags & INMEMORY) { */
/*  							i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, ECX); */
/*  						} else { */
/*  							M_INTMOVE(src->regoff, ECX); */
/*  						} */

/*  						i386_test_imm_reg(cd, 32, ECX); */
/*  						i386_jcc(cd, I386_CC_E, 2 + 2); */
/*  						i386_mov_reg_reg(cd, REG_ITMP2, REG_ITMP1); */
/*  						i386_alu_reg_reg(cd, ALU_XOR, REG_ITMP2, REG_ITMP2); */
						
/*  						i386_shrd_reg_reg(cd, REG_ITMP2, REG_ITMP1); */
/*  						i386_shift_reg(cd, I386_SHR, REG_ITMP2); */
/*  						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4); */
/*  						i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, iptr->dst->regoff * 4 + 4); */

/*  					} else { */
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4 + 4, REG_ITMP3);

						if (src->flags & INMEMORY) {
							i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, ECX);
						} else {
							M_INTMOVE(src->regoff, ECX);
						}

						i386_test_imm_reg(cd, 32, ECX);
						i386_jcc(cd, I386_CC_E, 2 + 2);
						i386_mov_reg_reg(cd, REG_ITMP3, REG_ITMP1);
						i386_alu_reg_reg(cd, ALU_XOR, REG_ITMP3, REG_ITMP3);
						
						i386_shrd_reg_reg(cd, REG_ITMP3, REG_ITMP1);
						i386_shift_reg(cd, I386_SHR, REG_ITMP3);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
						i386_mov_reg_membase(cd, REG_ITMP3, REG_SP, iptr->dst->regoff * 4 + 4);
/*  					} */
				}
			}
			break;

  		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
  		                      /* val.l = constant                             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY ) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4 + 4, REG_ITMP2);

				if (iptr->val.i & 0x20) {
					i386_mov_reg_reg(cd, REG_ITMP2, REG_ITMP1);
					i386_alu_reg_reg(cd, ALU_XOR, REG_ITMP2, REG_ITMP2);
					i386_shrd_imm_reg_reg(cd, iptr->val.i & 0x3f, REG_ITMP2, REG_ITMP1);

				} else {
					i386_shrd_imm_reg_reg(cd, iptr->val.i & 0x3f, REG_ITMP2, REG_ITMP1);
					i386_shift_imm_reg(cd, I386_SHR, iptr->val.i & 0x3f, REG_ITMP2);
				}

				i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
				i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, iptr->dst->regoff * 4 + 4);
			}
  			break;

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ialu(cd, ALU_AND, src, iptr);
			break;

		case ICMD_IANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.i = constant                             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ialuconst(cd, ALU_AND, src, iptr);
			break;

		case ICMD_LAND:       /* ..., val1, val2  ==> ..., val1 & val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_lalu(cd, ALU_AND, src, iptr);
			break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.l = constant                             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_laluconst(cd, ALU_AND, src, iptr);
			break;

		case ICMD_IOR:        /* ..., val1, val2  ==> ..., val1 | val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ialu(cd, ALU_OR, src, iptr);
			break;

		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.i = constant                             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ialuconst(cd, ALU_OR, src, iptr);
			break;

		case ICMD_LOR:        /* ..., val1, val2  ==> ..., val1 | val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_lalu(cd, ALU_OR, src, iptr);
			break;

		case ICMD_LORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.l = constant                             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_laluconst(cd, ALU_OR, src, iptr);
			break;

		case ICMD_IXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ialu(cd, ALU_XOR, src, iptr);
			break;

		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.i = constant                             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ialuconst(cd, ALU_XOR, src, iptr);
			break;

		case ICMD_LXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_lalu(cd, ALU_XOR, src, iptr);
			break;

		case ICMD_LXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.l = constant                             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_laluconst(cd, ALU_XOR, src, iptr);
			break;

		case ICMD_IINC:       /* ..., value  ==> ..., value + constant        */
		                      /* op1 = variable, val.i = constant             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			var = &(rd->locals[iptr->op1][TYPE_INT]);
			if (var->flags & INMEMORY) {
				i386_alu_imm_membase(cd, ALU_ADD, iptr->val.i, REG_SP, var->regoff * 4);

			} else {
				/* `inc reg' is slower on p4's (regarding to ia32             */
				/* optimization reference manual and benchmarks) and as fast  */
				/* on athlon's.                                               */
				i386_alu_imm_reg(cd, ALU_ADD, iptr->val.i, var->regoff);
			}
			break;


		/* floating operations ************************************************/
#if 0
#define ROUND_TO_SINGLE \
			i386_fstps_membase(cd, REG_SP, -8); \
			i386_flds_membase(cd, REG_SP, -8);

#define ROUND_TO_DOUBLE \
			i386_fstpl_membase(cd, REG_SP, -8); \
			i386_fldl_membase(cd, REG_SP, -8);

#define FPU_SET_24BIT_MODE \
			if (!fpu_in_24bit_mode) { \
				i386_fldcw_mem(cd, &fpu_ctrlwrd_24bit); \
				fpu_in_24bit_mode = 1; \
			}

#define FPU_SET_53BIT_MODE \
			if (fpu_in_24bit_mode) { \
				i386_fldcw_mem(cd, &fpu_ctrlwrd_53bit); \
				fpu_in_24bit_mode = 0; \
			}
#else
#define ROUND_TO_SINGLE
#define ROUND_TO_DOUBLE
#define FPU_SET_24BIT_MODE
#define FPU_SET_53BIT_MODE
#endif
		case ICMD_FNEG:       /* ..., value  ==> ..., - value                 */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			FPU_SET_24BIT_MODE;
			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			i386_fchs(cd);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			FPU_SET_53BIT_MODE;
			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			i386_fchs(cd);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			FPU_SET_24BIT_MODE;
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			i386_faddp(cd);
			fpu_st_offset--;
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DADD:       /* ..., val1, val2  ==> ..., val1 + val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			FPU_SET_53BIT_MODE;
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			i386_faddp(cd);
			fpu_st_offset--;
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			FPU_SET_24BIT_MODE;
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			i386_fsubp(cd);
			fpu_st_offset--;
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			FPU_SET_53BIT_MODE;
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			i386_fsubp(cd);
			fpu_st_offset--;
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			FPU_SET_24BIT_MODE;
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			i386_fmulp(cd);
			fpu_st_offset--;
			ROUND_TO_SINGLE;
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			FPU_SET_53BIT_MODE;
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);

/*  			i386_fldt_mem(cd, subnormal_bias1); */
/*  			i386_fmulp(cd); */

			var_to_reg_flt(s2, src, REG_FTMP2);

			i386_fmulp(cd);
			fpu_st_offset--;

/*  			i386_fldt_mem(cd, subnormal_bias2); */
/*  			i386_fmulp(cd); */

			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			FPU_SET_24BIT_MODE;
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			i386_fdivp(cd);
			fpu_st_offset--;
			ROUND_TO_SINGLE;
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			FPU_SET_53BIT_MODE;
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);

/*  			i386_fldt_mem(cd, subnormal_bias1); */
/*  			i386_fmulp(cd); */

			var_to_reg_flt(s2, src, REG_FTMP2);

			i386_fdivp(cd);
			fpu_st_offset--;

/*  			i386_fldt_mem(cd, subnormal_bias2); */
/*  			i386_fmulp(cd); */

			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FREM:       /* ..., val1, val2  ==> ..., val1 % val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			FPU_SET_24BIT_MODE;
			/* exchanged to skip fxch */
			var_to_reg_flt(s2, src, REG_FTMP2);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
/*  			i386_fxch(cd); */
			i386_fprem(cd);
			i386_wait(cd);
			i386_fnstsw(cd);
			i386_sahf(cd);
			i386_jcc(cd, I386_CC_P, -(2 + 1 + 2 + 1 + 6));
			store_reg_to_var_flt(iptr->dst, d);
			i386_ffree_reg(cd, 0);
			i386_fincstp(cd);
			fpu_st_offset--;
			break;

		case ICMD_DREM:       /* ..., val1, val2  ==> ..., val1 % val2        */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			FPU_SET_53BIT_MODE;
			/* exchanged to skip fxch */
			var_to_reg_flt(s2, src, REG_FTMP2);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
/*  			i386_fxch(cd); */
			i386_fprem(cd);
			i386_wait(cd);
			i386_fnstsw(cd);
			i386_sahf(cd);
			i386_jcc(cd, I386_CC_P, -(2 + 1 + 2 + 1 + 6));
			store_reg_to_var_flt(iptr->dst, d);
			i386_ffree_reg(cd, 0);
			i386_fincstp(cd);
			fpu_st_offset--;
			break;

		case ICMD_I2F:       /* ..., value  ==> ..., (float) value            */
		case ICMD_I2D:       /* ..., value  ==> ..., (double) value           */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			if (src->flags & INMEMORY) {
				i386_fildl_membase(cd, REG_SP, src->regoff * 4);
				fpu_st_offset++;

			} else {
				a = dseg_adds4(cd, 0);
				i386_mov_imm_reg(cd, 0, REG_ITMP1);
				dseg_adddata(cd, cd->mcodeptr);
				i386_mov_reg_membase(cd, src->regoff, REG_ITMP1, a);
				i386_fildl_membase(cd, REG_ITMP1, a);
				fpu_st_offset++;
			}
  			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_L2F:       /* ..., value  ==> ..., (float) value            */
		case ICMD_L2D:       /* ..., value  ==> ..., (double) value           */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			if (src->flags & INMEMORY) {
				i386_fildll_membase(cd, REG_SP, src->regoff * 4);
				fpu_st_offset++;

			} else {
				log_text("L2F: longs have to be in memory");
				assert(0);
			}
  			store_reg_to_var_flt(iptr->dst, d);
			break;
			
		case ICMD_F2I:       /* ..., value  ==> ..., (int) value              */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: D|YES ECX: NO EDX: NO OUTPUT: EAX*/ 

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_NULL);

			a = dseg_adds4(cd, 0x0e7f);    /* Round to zero, 53-bit mode, exception masked */
			i386_mov_imm_reg(cd, 0, REG_ITMP1);
			dseg_adddata(cd, cd->mcodeptr);
			i386_fldcw_membase(cd, REG_ITMP1, a);

			if (iptr->dst->flags & INMEMORY) {
				i386_fistpl_membase(cd, REG_SP, iptr->dst->regoff * 4);
				fpu_st_offset--;

				a = dseg_adds4(cd, 0x027f);    /* Round to nearest, 53-bit mode, exceptions masked */
				i386_fldcw_membase(cd, REG_ITMP1, a);

				i386_alu_imm_membase(cd, ALU_CMP, 0x80000000, REG_SP, iptr->dst->regoff * 4);

				a = 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 4);
				a += 5 + 2 + 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 4);

			} else {
				a = dseg_adds4(cd, 0);
				i386_fistpl_membase(cd, REG_ITMP1, a);
				fpu_st_offset--;
				i386_mov_membase_reg(cd, REG_ITMP1, a, iptr->dst->regoff);

				a = dseg_adds4(cd, 0x027f);    /* Round to nearest, 53-bit mode, exceptions masked */
				i386_fldcw_membase(cd, REG_ITMP1, a);

				i386_alu_imm_reg(cd, ALU_CMP, 0x80000000, iptr->dst->regoff);

				a = 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 4);
				a += 5 + 2 + ((REG_RESULT == iptr->dst->regoff) ? 0 : 2);
			}

			i386_jcc(cd, I386_CC_NE, a);

			/* XXX: change this when we use registers */
			i386_flds_membase(cd, REG_SP, src->regoff * 4);
			i386_mov_imm_reg(cd, (s4) asm_builtin_f2i, REG_ITMP1);
			i386_call_reg(cd, REG_ITMP1);

			if (iptr->dst->flags & INMEMORY) {
				i386_mov_reg_membase(cd, REG_RESULT, REG_SP, iptr->dst->regoff * 4);

			} else {
				M_INTMOVE(REG_RESULT, iptr->dst->regoff);
			}
			break;

		case ICMD_D2I:       /* ..., value  ==> ..., (int) value              */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: D|YES ECX: NO EDX: NO OUTPUT: EAX*/ 

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_NULL);

			a = dseg_adds4(cd, 0x0e7f);    /* Round to zero, 53-bit mode, exception masked */
			i386_mov_imm_reg(cd, 0, REG_ITMP1);
			dseg_adddata(cd, cd->mcodeptr);
			i386_fldcw_membase(cd, REG_ITMP1, a);

			if (iptr->dst->flags & INMEMORY) {
				i386_fistpl_membase(cd, REG_SP, iptr->dst->regoff * 4);
				fpu_st_offset--;

				a = dseg_adds4(cd, 0x027f);    /* Round to nearest, 53-bit mode, exceptions masked */
				i386_fldcw_membase(cd, REG_ITMP1, a);

  				i386_alu_imm_membase(cd, ALU_CMP, 0x80000000, REG_SP, iptr->dst->regoff * 4);

				a = 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 4);
				a += 5 + 2 + 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 4);

			} else {
				a = dseg_adds4(cd, 0);
				i386_fistpl_membase(cd, REG_ITMP1, a);
				fpu_st_offset--;
				i386_mov_membase_reg(cd, REG_ITMP1, a, iptr->dst->regoff);

				a = dseg_adds4(cd, 0x027f);    /* Round to nearest, 53-bit mode, exceptions masked */
				i386_fldcw_membase(cd, REG_ITMP1, a);

				i386_alu_imm_reg(cd, ALU_CMP, 0x80000000, iptr->dst->regoff);

				a = 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 4);
				a += 5 + 2 + ((REG_RESULT == iptr->dst->regoff) ? 0 : 2);
			}

			i386_jcc(cd, I386_CC_NE, a);

			/* XXX: change this when we use registers */
			i386_fldl_membase(cd, REG_SP, src->regoff * 4);
			i386_mov_imm_reg(cd, (s4) asm_builtin_d2i, REG_ITMP1);
			i386_call_reg(cd, REG_ITMP1);

			if (iptr->dst->flags & INMEMORY) {
				i386_mov_reg_membase(cd, REG_RESULT, REG_SP, iptr->dst->regoff * 4);
			} else {
				M_INTMOVE(REG_RESULT, iptr->dst->regoff);
			}
			break;

		case ICMD_F2L:       /* ..., value  ==> ..., (long) value             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: YES OUTPUT: REG_NULL*/ 

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_NULL);

			a = dseg_adds4(cd, 0x0e7f);    /* Round to zero, 53-bit mode, exception masked */
			i386_mov_imm_reg(cd, 0, REG_ITMP1);
			dseg_adddata(cd, cd->mcodeptr);
			i386_fldcw_membase(cd, REG_ITMP1, a);

			if (iptr->dst->flags & INMEMORY) {
				i386_fistpll_membase(cd, REG_SP, iptr->dst->regoff * 4);
				fpu_st_offset--;

				a = dseg_adds4(cd, 0x027f);    /* Round to nearest, 53-bit mode, exceptions masked */
				i386_fldcw_membase(cd, REG_ITMP1, a);

  				i386_alu_imm_membase(cd, ALU_CMP, 0x80000000, REG_SP, iptr->dst->regoff * 4 + 4);

				a = 6 + 4;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 4);
				a += 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 4);
				a += 5 + 2;
				a += 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 4);
				a += 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 4 + 4);

				i386_jcc(cd, I386_CC_NE, a);

  				i386_alu_imm_membase(cd, ALU_CMP, 0, REG_SP, iptr->dst->regoff * 4);

				a = 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 4);
				a += 5 + 2 + 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 4);

				i386_jcc(cd, I386_CC_NE, a);

				/* XXX: change this when we use registers */
				i386_flds_membase(cd, REG_SP, src->regoff * 4);
				i386_mov_imm_reg(cd, (s4) asm_builtin_f2l, REG_ITMP1);
				i386_call_reg(cd, REG_ITMP1);
				i386_mov_reg_membase(cd, REG_RESULT, REG_SP, iptr->dst->regoff * 4);
				i386_mov_reg_membase(cd, REG_RESULT2, REG_SP, iptr->dst->regoff * 4 + 4);

			} else {
				log_text("F2L: longs have to be in memory");
				assert(0);
			}
			break;

		case ICMD_D2L:       /* ..., value  ==> ..., (long) value             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: YES OUTPUT: REG_NULL*/ 

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_NULL);

			a = dseg_adds4(cd, 0x0e7f);    /* Round to zero, 53-bit mode, exception masked */
			i386_mov_imm_reg(cd, 0, REG_ITMP1);
			dseg_adddata(cd, cd->mcodeptr);
			i386_fldcw_membase(cd, REG_ITMP1, a);

			if (iptr->dst->flags & INMEMORY) {
				i386_fistpll_membase(cd, REG_SP, iptr->dst->regoff * 4);
				fpu_st_offset--;

				a = dseg_adds4(cd, 0x027f);    /* Round to nearest, 53-bit mode, exceptions masked */
				i386_fldcw_membase(cd, REG_ITMP1, a);

  				i386_alu_imm_membase(cd, ALU_CMP, 0x80000000, REG_SP, iptr->dst->regoff * 4 + 4);

				a = 6 + 4;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 4);
				a += 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 4);
				a += 5 + 2;
				a += 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 4);
				a += 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 4 + 4);

				i386_jcc(cd, I386_CC_NE, a);

  				i386_alu_imm_membase(cd, ALU_CMP, 0, REG_SP, iptr->dst->regoff * 4);

				a = 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 4);
				a += 5 + 2 + 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 4);

				i386_jcc(cd, I386_CC_NE, a);

				/* XXX: change this when we use registers */
				i386_fldl_membase(cd, REG_SP, src->regoff * 4);
				i386_mov_imm_reg(cd, (s4) asm_builtin_d2l, REG_ITMP1);
				i386_call_reg(cd, REG_ITMP1);
				i386_mov_reg_membase(cd, REG_RESULT, REG_SP, iptr->dst->regoff * 4);
				i386_mov_reg_membase(cd, REG_RESULT2, REG_SP, iptr->dst->regoff * 4 + 4);

			} else {
				log_text("D2L: longs have to be in memory");
				assert(0);
			}
			break;

		case ICMD_F2D:       /* ..., value  ==> ..., (double) value           */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			/* nothing to do */
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_D2F:       /* ..., value  ==> ..., (float) value            */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			/* nothing to do */
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */
		case ICMD_DCMPL:
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			/* exchanged to skip fxch */
			var_to_reg_flt(s2, src->prev, REG_FTMP1);
			var_to_reg_flt(s1, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
/*    			i386_fxch(cd); */
			i386_fucompp(cd);
			fpu_st_offset -= 2;
			i386_fnstsw(cd);
			i386_test_imm_reg(cd, 0x400, EAX);    /* unordered treat as GT */
			i386_jcc(cd, I386_CC_E, 6);
			i386_alu_imm_reg(cd, ALU_AND, 0x000000ff, EAX);
 			i386_sahf(cd);
			i386_mov_imm_reg(cd, 0, d);    /* does not affect flags */
  			i386_jcc(cd, I386_CC_E, 6 + 3 + 5 + 3);
  			i386_jcc(cd, I386_CC_B, 3 + 5);
			i386_alu_imm_reg(cd, ALU_SUB, 1, d);
			i386_jmp_imm(cd, 3);
			i386_alu_imm_reg(cd, ALU_ADD, 1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */
		case ICMD_DCMPG:
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			/* exchanged to skip fxch */
			var_to_reg_flt(s2, src->prev, REG_FTMP1);
			var_to_reg_flt(s1, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
/*    			i386_fxch(cd); */
			i386_fucompp(cd);
			fpu_st_offset -= 2;
			i386_fnstsw(cd);
			i386_test_imm_reg(cd, 0x400, EAX);    /* unordered treat as LT */
			i386_jcc(cd, I386_CC_E, 3);
			i386_movb_imm_reg(cd, 1, REG_AH);
 			i386_sahf(cd);
			i386_mov_imm_reg(cd, 0, d);    /* does not affect flags */
  			i386_jcc(cd, I386_CC_E, 6 + 3 + 5 + 3);
  			i386_jcc(cd, I386_CC_B, 3 + 5);
			i386_alu_imm_reg(cd, ALU_SUB, 1, d);
			i386_jmp_imm(cd, 3);
			i386_alu_imm_reg(cd, ALU_ADD, 1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;


		/* memory operations **************************************************/

		case ICMD_ARRAYLENGTH: /* ..., arrayref  ==> ..., length              */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			gen_nullptr_check(s1);
			i386_mov_membase_reg(cd, s1, OFFSET(java_arrayheader, size), d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_AALOAD:     /* ..., arrayref, index  ==> ..., value         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: NO OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			i386_mov_memindex_reg(cd, OFFSET(java_objectarray, data[0]), s1, s2, 2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LALOAD:     /* ..., arrayref, index  ==> ..., value         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: S|YES OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			
			if (iptr->dst->flags & INMEMORY) {
				i386_mov_memindex_reg(cd, OFFSET(java_longarray, data[0]), s1, s2, 3, REG_ITMP3);
				i386_mov_reg_membase(cd, REG_ITMP3, REG_SP, iptr->dst->regoff * 4);
				i386_mov_memindex_reg(cd, OFFSET(java_longarray, data[0]) + 4, s1, s2, 3, REG_ITMP3);
				i386_mov_reg_membase(cd, REG_ITMP3, REG_SP, iptr->dst->regoff * 4 + 4);
			}
			break;

		case ICMD_IALOAD:     /* ..., arrayref, index  ==> ..., value         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: NO OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			i386_mov_memindex_reg(cd, OFFSET(java_intarray, data[0]), s1, s2, 2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FALOAD:     /* ..., arrayref, index  ==> ..., value         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: NO OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			i386_flds_memindex(cd, OFFSET(java_floatarray, data[0]), s1, s2, 2);
			fpu_st_offset++;
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DALOAD:     /* ..., arrayref, index  ==> ..., value         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: NO OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			i386_fldl_memindex(cd, OFFSET(java_doublearray, data[0]), s1, s2, 3);
			fpu_st_offset++;
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_CALOAD:     /* ..., arrayref, index  ==> ..., value         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: NO OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			i386_movzwl_memindex_reg(cd, OFFSET(java_chararray, data[0]), s1, s2, 1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;			

		case ICMD_SALOAD:     /* ..., arrayref, index  ==> ..., value         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: NO OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			i386_movswl_memindex_reg(cd, OFFSET(java_shortarray, data[0]), s1, s2, 1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_BALOAD:     /* ..., arrayref, index  ==> ..., value         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: NO OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
   			i386_movsbl_memindex_reg(cd, OFFSET(java_bytearray, data[0]), s1, s2, 0, d);
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_AASTORE:    /* ..., arrayref, index, value  ==> ...         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: S|YES OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			i386_mov_reg_memindex(cd, s3, OFFSET(java_objectarray, data[0]), s1, s2, 2);
			break;

		case ICMD_LASTORE:    /* ..., arrayref, index, value  ==> ...         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: S|YES OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}

			if (src->flags & INMEMORY) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP3);
				i386_mov_reg_memindex(cd, REG_ITMP3, OFFSET(java_longarray, data[0]), s1, s2, 3);
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4 + 4, REG_ITMP3);
				i386_mov_reg_memindex(cd, REG_ITMP3, OFFSET(java_longarray, data[0]) + 4, s1, s2, 3);
			}
			break;

		case ICMD_IASTORE:    /* ..., arrayref, index, value  ==> ...         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: S|YES OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			i386_mov_reg_memindex(cd, s3, OFFSET(java_intarray, data[0]), s1, s2, 2);
			break;

		case ICMD_FASTORE:    /* ..., arrayref, index, value  ==> ...         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: NO OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_flt(s3, src, REG_FTMP1);
			i386_fstps_memindex(cd, OFFSET(java_floatarray, data[0]), s1, s2, 2);
			fpu_st_offset--;
			break;

		case ICMD_DASTORE:    /* ..., arrayref, index, value  ==> ...         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: NO OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_flt(s3, src, REG_FTMP1);
			i386_fstpl_memindex(cd, OFFSET(java_doublearray, data[0]), s1, s2, 3);
			fpu_st_offset--;
			break;

		case ICMD_CASTORE:    /* ..., arrayref, index, value  ==> ...         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: S|YES OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			i386_movw_reg_memindex(cd, s3, OFFSET(java_chararray, data[0]), s1, s2, 1);
			break;

		case ICMD_SASTORE:    /* ..., arrayref, index, value  ==> ...         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: S|YES OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			i386_movw_reg_memindex(cd, s3, OFFSET(java_shortarray, data[0]), s1, s2, 1);
			break;

		case ICMD_BASTORE:    /* ..., arrayref, index, value  ==> ...         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: S|YES OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			if (s3 >= EBP) {    /* because EBP, ESI, EDI have no xH and xL nibbles */
				M_INTMOVE(s3, REG_ITMP3);
				s3 = REG_ITMP3;
			}
			i386_movb_reg_memindex(cd, s3, OFFSET(java_bytearray, data[0]), s1, s2, 0);
			break;

		case ICMD_IASTORECONST: /* ..., arrayref, index  ==> ...              */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: NO OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			i386_mov_imm_memindex(cd, iptr->val.i, OFFSET(java_intarray, data[0]), s1, s2, 2);
			break;

		case ICMD_LASTORECONST: /* ..., arrayref, index  ==> ...              */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: NO OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}

			i386_mov_imm_memindex(cd, (u4) (iptr->val.l & 0x00000000ffffffff), OFFSET(java_longarray, data[0]), s1, s2, 3);
			i386_mov_imm_memindex(cd, (u4) (iptr->val.l >> 32), OFFSET(java_longarray, data[0]) + 4, s1, s2, 3);
			break;

		case ICMD_AASTORECONST: /* ..., arrayref, index  ==> ...              */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: NO OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			i386_mov_imm_memindex(cd, 0, OFFSET(java_objectarray, data[0]), s1, s2, 2);
			break;

		case ICMD_BASTORECONST: /* ..., arrayref, index  ==> ...              */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: NO OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			i386_movb_imm_memindex(cd, iptr->val.i, OFFSET(java_bytearray, data[0]), s1, s2, 0);
			break;

		case ICMD_CASTORECONST:   /* ..., arrayref, index  ==> ...            */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: NO OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			i386_movw_imm_memindex(cd, iptr->val.i, OFFSET(java_chararray, data[0]), s1, s2, 1);
			break;

		case ICMD_SASTORECONST:   /* ..., arrayref, index  ==> ...            */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: NO OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			i386_movw_imm_memindex(cd, iptr->val.i, OFFSET(java_shortarray, data[0]), s1, s2, 1);
			break;


		case ICMD_GETSTATIC:  /* ...  ==> ..., value                          */
		                      /* op1 = type, val.a = field address            */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: S|YES OUTPUT: EAX*/ 

			if (!iptr->val.a) {
				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_get_putstatic,
									(unresolved_field *) iptr->target);

				if (showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

				a = 0;

			} else {
				fieldinfo *fi = iptr->val.a;

				if (!fi->class->initialized) {
					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_clinit, fi->class);

					if (showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}
				}

				a = (ptrint) &(fi->value);
  			}

			i386_mov_imm_reg(cd, a, REG_ITMP1);
			switch (iptr->op1) {
			case TYPE_INT:
			case TYPE_ADR:
				d = reg_of_var(rd, iptr->dst, REG_ITMP2);
				i386_mov_membase_reg(cd, REG_ITMP1, 0, d);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_LNG:
				d = reg_of_var(rd, iptr->dst, REG_NULL);
				if (iptr->dst->flags & INMEMORY) {
					/* Using both REG_ITMP2 and REG_ITMP3 is faster than only */
					/* using REG_ITMP2 alternating.                           */
					i386_mov_membase_reg(cd, REG_ITMP1, 0, REG_ITMP2);
					i386_mov_membase_reg(cd, REG_ITMP1, 4, REG_ITMP3);
					i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, iptr->dst->regoff * 4);
					i386_mov_reg_membase(cd, REG_ITMP3, REG_SP, iptr->dst->regoff * 4 + 4);
				} else {
					log_text("GETSTATIC: longs have to be in memory");
					assert(0);
				}
				break;
			case TYPE_FLT:
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				i386_flds_membase(cd, REG_ITMP1, 0);
				fpu_st_offset++;
				store_reg_to_var_flt(iptr->dst, d);
				break;
			case TYPE_DBL:				
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				i386_fldl_membase(cd, REG_ITMP1, 0);
				fpu_st_offset++;
				store_reg_to_var_flt(iptr->dst, d);
				break;
			}
			break;

		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */
		                      /* op1 = type, val.a = field address            */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: S|YES OUTPUT: REG_NULL*/ 

			if (!iptr->val.a) {
				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_get_putstatic,
									(unresolved_field *) iptr->target);

				if (showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

				a = 0;

			} else {
				fieldinfo *fi = iptr->val.a;

				if (!fi->class->initialized) {
					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_clinit, fi->class);

					if (showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}
				}

				a = (ptrint) &(fi->value);
  			}

			i386_mov_imm_reg(cd, a, REG_ITMP1);
			switch (iptr->op1) {
			case TYPE_INT:
			case TYPE_ADR:
				var_to_reg_int(s2, src, REG_ITMP2);
				i386_mov_reg_membase(cd, s2, REG_ITMP1, 0);
				break;
			case TYPE_LNG:
				if (src->flags & INMEMORY) {
					/* Using both REG_ITMP2 and REG_ITMP3 is faster than only */
					/* using REG_ITMP2 alternating.                           */
					s2 = src->regoff;

					i386_mov_membase_reg(cd, REG_SP, s2 * 4, REG_ITMP2);
					i386_mov_membase_reg(cd, REG_SP, s2 * 4 + 4, REG_ITMP3);
					i386_mov_reg_membase(cd, REG_ITMP2, REG_ITMP1, 0);
					i386_mov_reg_membase(cd, REG_ITMP3, REG_ITMP1, 4);
				} else {
					log_text("PUTSTATIC: longs have to be in memory");
					assert(0);
				}
				break;
			case TYPE_FLT:
				var_to_reg_flt(s2, src, REG_FTMP1);
				i386_fstps_membase(cd, REG_ITMP1, 0);
				fpu_st_offset--;
				break;
			case TYPE_DBL:
				var_to_reg_flt(s2, src, REG_FTMP1);
				i386_fstpl_membase(cd, REG_ITMP1, 0);
				fpu_st_offset--;
				break;
			}
			break;

		case ICMD_PUTSTATICCONST: /* ...  ==> ...                             */
		                          /* val = value (in current instruction)     */
		                          /* op1 = type, val.a = field address (in    */
		                          /* following NOP)                           */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: S|YES OUTPUT: REG_NULL*/ 

			if (!iptr[1].val.a) {
				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_get_putstatic,
									(unresolved_field *) iptr[1].target);

				if (showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

				a = 0;

			} else {
				fieldinfo *fi = iptr[1].val.a;

				if (!fi->class->initialized) {
					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_clinit, fi->class);

					if (showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}
				}

				a = (ptrint) &(fi->value);
  			}

			i386_mov_imm_reg(cd, a, REG_ITMP1);
			switch (iptr[1].op1) {
			case TYPE_INT:
			case TYPE_FLT:
			case TYPE_ADR:
				i386_mov_imm_membase(cd, iptr->val.i, REG_ITMP1, 0);
				break;
			case TYPE_LNG:
			case TYPE_DBL:
				i386_mov_imm_membase(cd, iptr->val.l, REG_ITMP1, 0);
				i386_mov_imm_membase(cd, iptr->val.l >> 32, REG_ITMP1, 4);
				break;
			}
			break;

		case ICMD_GETFIELD:   /* .., objectref.  ==> ..., value               */
		                      /* op1 = type, val.i = field offset             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: S|YES EDX: S|YES OUTPUT: REG_NULL */ 

			var_to_reg_int(s1, src, REG_ITMP1);
			gen_nullptr_check(s1);

			if (!iptr->val.a) {
				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_getfield,
									(unresolved_field *) iptr->target);

				if (showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

				a = 0;

			} else {
				a = ((fieldinfo *) (iptr->val.a))->offset;
			}

			switch (iptr->op1) {
			case TYPE_INT:
			case TYPE_ADR:
				d = reg_of_var(rd, iptr->dst, REG_ITMP2);
				i386_mov_membase32_reg(cd, s1, a, d);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_LNG:
				d = reg_of_var(rd, iptr->dst, REG_NULL);
				i386_mov_membase32_reg(cd, s1, a, REG_ITMP2);
				i386_mov_membase32_reg(cd, s1, a + 4, REG_ITMP3);
				i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, iptr->dst->regoff * 4);
				i386_mov_reg_membase(cd, REG_ITMP3, REG_SP, iptr->dst->regoff * 4 + 4);
				break;
			case TYPE_FLT:
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				i386_flds_membase32(cd, s1, a);
				fpu_st_offset++;
				store_reg_to_var_flt(iptr->dst, d);
				break;
			case TYPE_DBL:				
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				i386_fldl_membase32(cd, s1, a);
				fpu_st_offset++;
				store_reg_to_var_flt(iptr->dst, d);
				break;
			}
			break;

		case ICMD_PUTFIELD:   /* ..., objectref, value  ==> ...               */
		                      /* op1 = type, val.a = field address            */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: S|YES OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			gen_nullptr_check(s1);
			if ((iptr->op1 == TYPE_INT) || IS_ADR_TYPE(iptr->op1)) {
				var_to_reg_int(s2, src, REG_ITMP2);
			} else if (IS_FLT_DBL_TYPE(iptr->op1)) {
				var_to_reg_flt(s2, src, REG_FTMP2);
			}

			if (!iptr->val.a) {
				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_putfield,
									(unresolved_field *) iptr->target);

				if (showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

				a = 0;

			} else {
				a = ((fieldinfo *) (iptr->val.a))->offset;
			}

			switch (iptr->op1) {
			case TYPE_INT:
			case TYPE_ADR:
				i386_mov_reg_membase32(cd, s2, s1, a);
				break;
			case TYPE_LNG:
				if (src->flags & INMEMORY) {
					i386_mov_membase32_reg(cd, REG_SP, src->regoff * 4, REG_ITMP2);
					i386_mov_membase32_reg(cd, REG_SP, src->regoff * 4 + 4, REG_ITMP3);
					i386_mov_reg_membase32(cd, REG_ITMP2, s1, a);
					i386_mov_reg_membase32(cd, REG_ITMP3, s1, a + 4);
				} else {
					log_text("PUTFIELD: longs have to be in memory");
					assert(0);
				}
				break;
			case TYPE_FLT:
				i386_fstps_membase32(cd, s1, a);
				fpu_st_offset--;
				break;
			case TYPE_DBL:
				i386_fstpl_membase32(cd, s1, a);
				fpu_st_offset--;
				break;
			}
			break;

		case ICMD_PUTFIELDCONST:  /* ..., objectref  ==> ...                  */
		                          /* val = value (in current instruction)     */
		                          /* op1 = type, val.a = field address (in    */
		                          /* following NOP)                           */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: S|YES EDX: S|YES OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src, REG_ITMP1);
			gen_nullptr_check(s1);

			if (!iptr[1].val.a) {
				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_putfieldconst,
									(unresolved_field *) iptr[1].target);

				if (showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

				a = 0;

			} else {
				a = ((fieldinfo *) (iptr[1].val.a))->offset;
			}

			switch (iptr[1].op1) {
			case TYPE_INT:
			case TYPE_FLT:
			case TYPE_ADR:
				i386_mov_imm_membase32(cd, iptr->val.i, s1, a);
				break;
			case TYPE_LNG:
			case TYPE_DBL:
				i386_mov_imm_membase32(cd, iptr->val.l, s1, a);
				i386_mov_imm_membase32(cd, iptr->val.l >> 32, s1, a + 4);
				break;
			}
			break;


		/* branch operations **************************************************/

		case ICMD_ATHROW:       /* ..., objectref ==> ... (, objectref)       */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src, REG_ITMP1);
			M_INTMOVE(s1, REG_ITMP1_XPTR);

			i386_call_imm(cd, 0);                /* passing exception pointer */
			i386_pop_reg(cd, REG_ITMP2_XPC);

  			i386_mov_imm_reg(cd, (s4) asm_handle_exception, REG_ITMP3);
  			i386_jmp_reg(cd, REG_ITMP3);
			break;

		case ICMD_GOTO:         /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			i386_jmp_imm(cd, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_JSR:          /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

  			i386_call_imm(cd, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;
			
		case ICMD_RET:          /* ... ==> ...                                */
		                        /* op1 = local variable                       */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			var = &(rd->locals[iptr->op1][TYPE_ADR]);
			var_to_reg_int(s1, var, REG_ITMP1);
			i386_jmp_reg(cd, s1);
			break;

		case ICMD_IFNULL:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, ALU_CMP, 0, REG_SP, src->regoff * 4);

			} else {
				i386_test_reg_reg(cd, src->regoff, src->regoff);
			}
			i386_jcc(cd, I386_CC_E, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IFNONNULL:    /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, ALU_CMP, 0, REG_SP, src->regoff * 4);

			} else {
				i386_test_reg_reg(cd, src->regoff, src->regoff);
			}
			i386_jcc(cd, I386_CC_NE, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IFEQ:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, ALU_CMP, iptr->val.i, REG_SP, src->regoff * 4);

			} else {
				i386_alu_imm_reg(cd, ALU_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(cd, I386_CC_E, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IFLT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, ALU_CMP, iptr->val.i, REG_SP, src->regoff * 4);

			} else {
				i386_alu_imm_reg(cd, ALU_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(cd, I386_CC_L, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IFLE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, ALU_CMP, iptr->val.i, REG_SP, src->regoff * 4);

			} else {
				i386_alu_imm_reg(cd, ALU_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(cd, I386_CC_LE, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IFNE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, ALU_CMP, iptr->val.i, REG_SP, src->regoff * 4);

			} else {
				i386_alu_imm_reg(cd, ALU_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(cd, I386_CC_NE, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IFGT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, ALU_CMP, iptr->val.i, REG_SP, src->regoff * 4);

			} else {
				i386_alu_imm_reg(cd, ALU_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(cd, I386_CC_G, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IFGE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, ALU_CMP, iptr->val.i, REG_SP, src->regoff * 4);

			} else {
				i386_alu_imm_reg(cd, ALU_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(cd, I386_CC_GE, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IF_LEQ:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if (src->flags & INMEMORY) {
				if (iptr->val.l == 0) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
					i386_alu_membase_reg(cd, ALU_OR, REG_SP, src->regoff * 4 + 4, REG_ITMP1);

				} else {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4 + 4, REG_ITMP2);
					i386_alu_imm_reg(cd, ALU_XOR, iptr->val.l >> 32, REG_ITMP2);
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
					i386_alu_imm_reg(cd, ALU_XOR, iptr->val.l, REG_ITMP1);
					i386_alu_reg_reg(cd, ALU_OR, REG_ITMP2, REG_ITMP1);
				}
			}
			i386_test_reg_reg(cd, REG_ITMP1, REG_ITMP1);
			i386_jcc(cd, I386_CC_E, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IF_LLT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, ALU_CMP, iptr->val.l >> 32, REG_SP, src->regoff * 4 + 4);
				i386_jcc(cd, I386_CC_L, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);

				a = 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 4);
				CALCIMMEDIATEBYTES(a, iptr->val.l);

				i386_jcc(cd, I386_CC_G, a);

				i386_alu_imm_membase(cd, ALU_CMP, iptr->val.l, REG_SP, src->regoff * 4);
				i386_jcc(cd, I386_CC_B, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			}			
			break;

		case ICMD_IF_LLE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, ALU_CMP, iptr->val.l >> 32, REG_SP, src->regoff * 4 + 4);
				i386_jcc(cd, I386_CC_L, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);

				a = 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 4);
				CALCIMMEDIATEBYTES(a, iptr->val.l);
				
				i386_jcc(cd, I386_CC_G, a);

				i386_alu_imm_membase(cd, ALU_CMP, iptr->val.l, REG_SP, src->regoff * 4);
				i386_jcc(cd, I386_CC_BE, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			}			
			break;

		case ICMD_IF_LNE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if (src->flags & INMEMORY) {
				if (iptr->val.l == 0) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
					i386_alu_membase_reg(cd, ALU_OR, REG_SP, src->regoff * 4 + 4, REG_ITMP1);

				} else {
					i386_mov_imm_reg(cd, iptr->val.l, REG_ITMP1);
					i386_alu_membase_reg(cd, ALU_XOR, REG_SP, src->regoff * 4, REG_ITMP1);
					i386_mov_imm_reg(cd, iptr->val.l >> 32, REG_ITMP2);
					i386_alu_membase_reg(cd, ALU_XOR, REG_SP, src->regoff * 4 + 4, REG_ITMP2);
					i386_alu_reg_reg(cd, ALU_OR, REG_ITMP2, REG_ITMP1);
				}
			}
			i386_test_reg_reg(cd, REG_ITMP1, REG_ITMP1);
			i386_jcc(cd, I386_CC_NE, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IF_LGT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, ALU_CMP, iptr->val.l >> 32, REG_SP, src->regoff * 4 + 4);
				i386_jcc(cd, I386_CC_G, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);

				a = 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 4);
				CALCIMMEDIATEBYTES(a, iptr->val.l);

				i386_jcc(cd, I386_CC_L, a);

				i386_alu_imm_membase(cd, ALU_CMP, iptr->val.l, REG_SP, src->regoff * 4);
				i386_jcc(cd, I386_CC_A, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			}			
			break;

		case ICMD_IF_LGE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, ALU_CMP, iptr->val.l >> 32, REG_SP, src->regoff * 4 + 4);
				i386_jcc(cd, I386_CC_G, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);

				a = 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 4);
				CALCIMMEDIATEBYTES(a, iptr->val.l);

				i386_jcc(cd, I386_CC_L, a);

				i386_alu_imm_membase(cd, ALU_CMP, iptr->val.l, REG_SP, src->regoff * 4);
				i386_jcc(cd, I386_CC_AE, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			}			
			break;

		case ICMD_IF_ICMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_ACMPEQ:    /* op1 = target JavaVM pc                     */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
				i386_alu_reg_membase(cd, ALU_CMP, REG_ITMP1, REG_SP, src->prev->regoff * 4);

			} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
				i386_alu_membase_reg(cd, ALU_CMP, REG_SP, src->regoff * 4, src->prev->regoff);

			} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_alu_reg_membase(cd, ALU_CMP, src->regoff, REG_SP, src->prev->regoff * 4);

			} else {
				i386_alu_reg_reg(cd, ALU_CMP, src->regoff, src->prev->regoff);
			}
			i386_jcc(cd, I386_CC_E, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IF_LCMPEQ:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4 + 4, REG_ITMP2);
				i386_alu_membase_reg(cd, ALU_XOR, REG_SP, src->regoff * 4, REG_ITMP1);
				i386_alu_membase_reg(cd, ALU_XOR, REG_SP, src->regoff * 4 + 4, REG_ITMP2);
				i386_alu_reg_reg(cd, ALU_OR, REG_ITMP2, REG_ITMP1);
				i386_test_reg_reg(cd, REG_ITMP1, REG_ITMP1);
			}			
			i386_jcc(cd, I386_CC_E, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IF_ICMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_ACMPNE:    /* op1 = target JavaVM pc                     */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
				i386_alu_reg_membase(cd, ALU_CMP, REG_ITMP1, REG_SP, src->prev->regoff * 4);

			} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
				i386_alu_membase_reg(cd, ALU_CMP, REG_SP, src->regoff * 4, src->prev->regoff);

			} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_alu_reg_membase(cd, ALU_CMP, src->regoff, REG_SP, src->prev->regoff * 4);

			} else {
				i386_alu_reg_reg(cd, ALU_CMP, src->regoff, src->prev->regoff);
			}
			i386_jcc(cd, I386_CC_NE, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IF_LCMPNE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4 + 4, REG_ITMP2);
				i386_alu_membase_reg(cd, ALU_XOR, REG_SP, src->regoff * 4, REG_ITMP1);
				i386_alu_membase_reg(cd, ALU_XOR, REG_SP, src->regoff * 4 + 4, REG_ITMP2);
				i386_alu_reg_reg(cd, ALU_OR, REG_ITMP2, REG_ITMP1);
				i386_test_reg_reg(cd, REG_ITMP1, REG_ITMP1);
			}			
			i386_jcc(cd, I386_CC_NE, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IF_ICMPLT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
				i386_alu_reg_membase(cd, ALU_CMP, REG_ITMP1, REG_SP, src->prev->regoff * 4);

			} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
				i386_alu_membase_reg(cd, ALU_CMP, REG_SP, src->regoff * 4, src->prev->regoff);

			} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_alu_reg_membase(cd, ALU_CMP, src->regoff, REG_SP, src->prev->regoff * 4);

			} else {
				i386_alu_reg_reg(cd, ALU_CMP, src->regoff, src->prev->regoff);
			}
			i386_jcc(cd, I386_CC_L, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IF_LCMPLT:    /* ..., value, value ==> ...                  */
	                            /* op1 = target JavaVM pc                     */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4 + 4, REG_ITMP1);
				i386_alu_membase_reg(cd, ALU_CMP, REG_SP, src->regoff * 4 + 4, REG_ITMP1);
				i386_jcc(cd, I386_CC_L, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);

				a = 3 + 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->prev->regoff * 4);
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 4);

				i386_jcc(cd, I386_CC_G, a);

				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
				i386_alu_membase_reg(cd, ALU_CMP, REG_SP, src->regoff * 4, REG_ITMP1);
				i386_jcc(cd, I386_CC_B, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			}			
			break;

		case ICMD_IF_ICMPGT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
				i386_alu_reg_membase(cd, ALU_CMP, REG_ITMP1, REG_SP, src->prev->regoff * 4);

			} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
				i386_alu_membase_reg(cd, ALU_CMP, REG_SP, src->regoff * 4, src->prev->regoff);

			} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_alu_reg_membase(cd, ALU_CMP, src->regoff, REG_SP, src->prev->regoff * 4);

			} else {
				i386_alu_reg_reg(cd, ALU_CMP, src->regoff, src->prev->regoff);
			}
			i386_jcc(cd, I386_CC_G, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IF_LCMPGT:    /* ..., value, value ==> ...                  */
                                /* op1 = target JavaVM pc                     */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4 + 4, REG_ITMP1);
				i386_alu_membase_reg(cd, ALU_CMP, REG_SP, src->regoff * 4 + 4, REG_ITMP1);
				i386_jcc(cd, I386_CC_G, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);

				a = 3 + 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->prev->regoff * 4);
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 4);

				i386_jcc(cd, I386_CC_L, a);

				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
				i386_alu_membase_reg(cd, ALU_CMP, REG_SP, src->regoff * 4, REG_ITMP1);
				i386_jcc(cd, I386_CC_A, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			}			
			break;

		case ICMD_IF_ICMPLE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
				i386_alu_reg_membase(cd, ALU_CMP, REG_ITMP1, REG_SP, src->prev->regoff * 4);

			} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
				i386_alu_membase_reg(cd, ALU_CMP, REG_SP, src->regoff * 4, src->prev->regoff);

			} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_alu_reg_membase(cd, ALU_CMP, src->regoff, REG_SP, src->prev->regoff * 4);

			} else {
				i386_alu_reg_reg(cd, ALU_CMP, src->regoff, src->prev->regoff);
			}
			i386_jcc(cd, I386_CC_LE, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IF_LCMPLE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4 + 4, REG_ITMP1);
				i386_alu_membase_reg(cd, ALU_CMP, REG_SP, src->regoff * 4 + 4, REG_ITMP1);
				i386_jcc(cd, I386_CC_L, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);

				a = 3 + 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->prev->regoff * 4);
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 4);

				i386_jcc(cd, I386_CC_G, a);

				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
				i386_alu_membase_reg(cd, ALU_CMP, REG_SP, src->regoff * 4, REG_ITMP1);
				i386_jcc(cd, I386_CC_BE, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			}			
			break;

		case ICMD_IF_ICMPGE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
				i386_alu_reg_membase(cd, ALU_CMP, REG_ITMP1, REG_SP, src->prev->regoff * 4);

			} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
				i386_alu_membase_reg(cd, ALU_CMP, REG_SP, src->regoff * 4, src->prev->regoff);

			} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_alu_reg_membase(cd, ALU_CMP, src->regoff, REG_SP, src->prev->regoff * 4);

			} else {
				i386_alu_reg_reg(cd, ALU_CMP, src->regoff, src->prev->regoff);
			}
			i386_jcc(cd, I386_CC_GE, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IF_LCMPGE:    /* ..., value, value ==> ...                  */
	                            /* op1 = target JavaVM pc                     */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4 + 4, REG_ITMP1);
				i386_alu_membase_reg(cd, ALU_CMP, REG_SP, src->regoff * 4 + 4, REG_ITMP1);
				i386_jcc(cd, I386_CC_G, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);

				a = 3 + 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->prev->regoff * 4);
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 4);

				i386_jcc(cd, I386_CC_L, a);

				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 4, REG_ITMP1);
				i386_alu_membase_reg(cd, ALU_CMP, REG_SP, src->regoff * 4, REG_ITMP1);
				i386_jcc(cd, I386_CC_AE, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			}			
			break;

		/* (value xx 0) ? IFxx_ICONST : ELSE_ICONST                           */

		case ICMD_ELSE_ICONST:  /* handled by IFxx_ICONST                     */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 
			break;

		case ICMD_IFEQ_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ifcc_iconst(cd, I386_CC_NE, src, iptr);
			break;

		case ICMD_IFNE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ifcc_iconst(cd, I386_CC_E, src, iptr);
			break;

		case ICMD_IFLT_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ifcc_iconst(cd, I386_CC_GE, src, iptr);
			break;

		case ICMD_IFGE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ifcc_iconst(cd, I386_CC_L, src, iptr);
			break;

		case ICMD_IFGT_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ifcc_iconst(cd, I386_CC_LE, src, iptr);
			break;

		case ICMD_IFLE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ifcc_iconst(cd, I386_CC_G, src, iptr);
			break;


		case ICMD_IRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_ARETURN:
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			var_to_reg_int(s1, src, REG_RESULT);
			M_INTMOVE(s1, REG_RESULT);

			goto nowperformreturn;

		case ICMD_LRETURN:      /* ..., retvalue ==> ...                      */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			if (src->flags & INMEMORY) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_RESULT);
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 4 + 4, REG_RESULT2);

			} else {
				log_text("LRETURN: longs have to be in memory");
				assert(0);
			}

			goto nowperformreturn;

		case ICMD_FRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_DRETURN:      /* ..., retvalue ==> ...                      */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

			var_to_reg_flt(s1, src, REG_FRESULT);
			/* this may be an early return -- keep the offset correct for the
			   remaining code */
			fpu_st_offset--;

			goto nowperformreturn;

		case ICMD_RETURN:      /* ...  ==> ...                                */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 

nowperformreturn:
			{
			s4 i, p;
			
  			p = parentargs_base;
			
			/* call trace function */
			if (runverbose) {
				i386_alu_imm_reg(cd, ALU_SUB, 4 + 8 + 8 + 4, REG_SP);

				i386_mov_imm_membase(cd, (s4) m, REG_SP, 0);

				i386_mov_reg_membase(cd, REG_RESULT, REG_SP, 4);
				i386_mov_reg_membase(cd, REG_RESULT2, REG_SP, 4 + 4);
				
				i386_fstl_membase(cd, REG_SP, 4 + 8);
				i386_fsts_membase(cd, REG_SP, 4 + 8 + 8);

  				i386_mov_imm_reg(cd, (s4) builtin_displaymethodstop, REG_ITMP1);
				i386_call_reg(cd, REG_ITMP1);

				i386_mov_membase_reg(cd, REG_SP, 4, REG_RESULT);
				i386_mov_membase_reg(cd, REG_SP, 4 + 4, REG_RESULT2);

				i386_alu_imm_reg(cd, ALU_ADD, 4 + 8 + 8 + 4, REG_SP);
			}

#if defined(USE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				i386_mov_membase_reg(cd, REG_SP, rd->maxmemuse * 4, REG_ITMP2);

				/* we need to save the proper return value */
				switch (iptr->opc) {
				case ICMD_IRETURN:
				case ICMD_ARETURN:
					i386_mov_reg_membase(cd, REG_RESULT, REG_SP, rd->maxmemuse * 4);
					break;

				case ICMD_LRETURN:
					i386_mov_reg_membase(cd, REG_RESULT, REG_SP, rd->maxmemuse * 4);
					i386_mov_reg_membase(cd, REG_RESULT2, REG_SP, rd->maxmemuse * 4 + 4);
					break;

				case ICMD_FRETURN:
					i386_fsts_membase(cd, REG_SP, rd->maxmemuse * 4);
					break;

				case ICMD_DRETURN:
					i386_fstl_membase(cd, REG_SP, rd->maxmemuse * 4);
					break;
				}

				i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, 0);
				i386_mov_imm_reg(cd, (ptrint) BUILTIN_monitorexit, REG_ITMP1);
				i386_call_reg(cd, REG_ITMP1);

				/* and now restore the proper return value */
				switch (iptr->opc) {
				case ICMD_IRETURN:
				case ICMD_ARETURN:
					i386_mov_membase_reg(cd, REG_SP, rd->maxmemuse * 4, REG_RESULT);
					break;

				case ICMD_LRETURN:
					i386_mov_membase_reg(cd, REG_SP, rd->maxmemuse * 4, REG_RESULT);
					i386_mov_membase_reg(cd, REG_SP, rd->maxmemuse * 4 + 4, REG_RESULT2);
					break;

				case ICMD_FRETURN:
					i386_flds_membase(cd, REG_SP, rd->maxmemuse * 4);
					break;

				case ICMD_DRETURN:
					i386_fldl_membase(cd, REG_SP, rd->maxmemuse * 4);
					break;
				}
			}
#endif

			/* restore saved registers */
			for (i = rd->savintregcnt - 1; i >= rd->maxsavintreguse; i--) {
				p--;
				i386_mov_membase_reg(cd, REG_SP, p * 4, rd->savintregs[i]);
			}
			for (i = rd->savfltregcnt - 1; i >= rd->maxsavfltreguse; i--) {
  				p--;
				i386_fldl_membase(cd, REG_SP, p * 4);
				fpu_st_offset++;
				if (iptr->opc == ICMD_FRETURN || iptr->opc == ICMD_DRETURN) {
					i386_fstp_reg(cd, rd->savfltregs[i] + fpu_st_offset + 1);
				} else {
					i386_fstp_reg(cd, rd->savfltregs[i] + fpu_st_offset);
				}
				fpu_st_offset--;
			}

			/* deallocate stack                                               */
			if (parentargs_base) {
				i386_alu_imm_reg(cd, ALU_ADD, parentargs_base * 4, REG_SP);
			}

			i386_ret(cd);
			}
			break;


		case ICMD_TABLESWITCH:  /* ..., index ==> ...                         */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 
			{
				s4 i, l, *s4ptr;
				void **tptr;

				tptr = (void **) iptr->target;

				s4ptr = iptr->val.a;
				l = s4ptr[1];                          /* low     */
				i = s4ptr[2];                          /* high    */

				var_to_reg_int(s1, src, REG_ITMP1);
				M_INTMOVE(s1, REG_ITMP1);
				if (l != 0) {
					i386_alu_imm_reg(cd, ALU_SUB, l, REG_ITMP1);
				}
				i = i - l + 1;

                /* range check */

				i386_alu_imm_reg(cd, ALU_CMP, i - 1, REG_ITMP1);
				i386_jcc(cd, I386_CC_A, 0);

                /* codegen_addreference(cd, BlockPtrOfPC(s4ptr[0]), cd->mcodeptr); */
				codegen_addreference(cd, (basicblock *) tptr[0], cd->mcodeptr);

				/* build jump table top down and use address of lowest entry */

                /* s4ptr += 3 + i; */
				tptr += i;

				while (--i >= 0) {
					/* dseg_addtarget(cd, BlockPtrOfPC(*--s4ptr)); */
					dseg_addtarget(cd, (basicblock *) tptr[0]); 
					--tptr;
				}

				/* length of dataseg after last dseg_addtarget is used by load */

				i386_mov_imm_reg(cd, 0, REG_ITMP2);
				dseg_adddata(cd, cd->mcodeptr);
				i386_mov_memindex_reg(cd, -(cd->dseglen), REG_ITMP2, REG_ITMP1, 2, REG_ITMP1);
				i386_jmp_reg(cd, REG_ITMP1);
			}
			break;


		case ICMD_LOOKUPSWITCH: /* ..., key ==> ...                           */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: YES EDX: YES OUTPUT: REG_NULL*/ 
 			{
				s4 i, l, val, *s4ptr;
				void **tptr;

				tptr = (void **) iptr->target;

				s4ptr = iptr->val.a;
				l = s4ptr[0];                          /* default  */
				i = s4ptr[1];                          /* count    */
			
				MCODECHECK((i<<2)+8);
				var_to_reg_int(s1, src, REG_ITMP1);    /* reg compare should always be faster */
				while (--i >= 0) {
					s4ptr += 2;
					++tptr;

					val = s4ptr[0];
					i386_alu_imm_reg(cd, ALU_CMP, val, s1);
					i386_jcc(cd, I386_CC_E, 0);
					/* codegen_addreference(cd, BlockPtrOfPC(s4ptr[1]), cd->mcodeptr); */
					codegen_addreference(cd, (basicblock *) tptr[0], cd->mcodeptr); 
				}

				i386_jmp_imm(cd, 0);
				/* codegen_addreference(cd, BlockPtrOfPC(l), cd->mcodeptr); */
			
				tptr = (void **) iptr->target;
				codegen_addreference(cd, (basicblock *) tptr[0], cd->mcodeptr);
			}
			break;

		case ICMD_BUILTIN:      /* ..., [arg1, [arg2 ...]] ==> ...            */
		                        /* op1 = arg count val.a = builtintable entry */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: YES EDX: YES OUTPUT: EAX*/ 
			bte = (builtintable_entry *) iptr->val.a;
			md = bte->md;
			goto gen_method;

		case ICMD_INVOKESTATIC: /* ..., [arg1, [arg2 ...]] ==> ...            */
		                        /* op1 = arg count, val.a = method pointer    */

		case ICMD_INVOKESPECIAL:/* ..., objectref, [arg1, [arg2 ...]] ==> ... */
		case ICMD_INVOKEVIRTUAL:/* op1 = arg count, val.a = method pointer    */
		case ICMD_INVOKEINTERFACE:
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: YES EDX: YES OUTPUT: EAX*/ 

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
						log_text("No integer argument registers available!");
						assert(0);
					} else {
						if (!IS_2_WORD_TYPE(src->type)) {
							if (src->flags & INMEMORY) {
								i386_mov_membase_reg(
                                    cd, REG_SP, src->regoff * 4, REG_ITMP1);
								i386_mov_reg_membase(
                                    cd, REG_ITMP1, REG_SP,
									md->params[s3].regoff * 4);
							} else {
								i386_mov_reg_membase(
							        cd, src->regoff, REG_SP,
									md->params[s3].regoff * 4);
							}

						} else {
							if (src->flags & INMEMORY) {
								M_LNGMEMMOVE(
								    src->regoff, md->params[s3].regoff);
							} else {
								log_text("copy arguments: longs have to be in memory");
								assert(0);
							}
						}
					}
				} else {
					if (!md->params[s3].inmemory) {
						log_text("No float argument registers available!");
						assert(0);
					} else {
						var_to_reg_flt(d, src, REG_FTMP1);
						if (src->type == TYPE_FLT) {
							i386_fstps_membase(
							    cd, REG_SP, md->params[s3].regoff * 4);

						} else {
							i386_fstpl_membase(
							    cd, REG_SP, md->params[s3].regoff * 4);
						}
					}
				}
			} /* end of for */

			switch (iptr->opc) {
			case ICMD_BUILTIN:
				d = md->returntype.type;

				if (iptr->target) {
					codegen_addpatchref(cd, cd->mcodeptr, bte->fp, iptr->target);

					if (showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}

					a = 0;

				} else {
					a = (ptrint) bte->fp;
				}

				i386_mov_imm_reg(cd, a, REG_ITMP1);
				i386_call_reg(cd, REG_ITMP1);
				break;

			case ICMD_INVOKESPECIAL:
				i386_mov_membase_reg(cd, REG_SP, 0, REG_ITMP1);
				gen_nullptr_check(REG_ITMP1);

				/* access memory for hardware nullptr */
				i386_mov_membase_reg(cd, REG_ITMP1, 0, REG_ITMP1);

				/* fall through */

			case ICMD_INVOKESTATIC:
				if (!lm) {
					unresolved_method *um = iptr->target;

					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_invokestatic_special, um);

					if (showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}

					a = 0;
					d = md->returntype.type;

				} else {
					a = (ptrint) lm->stubroutine;
					d = lm->parseddesc->returntype.type;
				}

				i386_mov_imm_reg(cd, a, REG_ITMP2);
				i386_call_reg(cd, REG_ITMP2);
				break;

			case ICMD_INVOKEVIRTUAL:
				i386_mov_membase_reg(cd, REG_SP, 0, REG_ITMP1);
				gen_nullptr_check(REG_ITMP1);

				if (!lm) {
					unresolved_method *um = iptr->target;

					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_invokevirtual, um);

					if (showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}

					s1 = 0;
					d = md->returntype.type;

				} else {
					s1 = OFFSET(vftbl_t, table[0]) +
						sizeof(methodptr) * lm->vftblindex;
					d = md->returntype.type;
				}

				i386_mov_membase_reg(cd, REG_ITMP1,
									 OFFSET(java_objectheader, vftbl),
									 REG_ITMP2);
				i386_mov_membase32_reg(cd, REG_ITMP2, s1, REG_ITMP1);
				i386_call_reg(cd, REG_ITMP1);
				break;

			case ICMD_INVOKEINTERFACE:
				i386_mov_membase_reg(cd, REG_SP, 0, REG_ITMP1);
				gen_nullptr_check(REG_ITMP1);

				if (!lm) {
					unresolved_method *um = iptr->target;

					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_invokeinterface, um);

					if (showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}

					s1 = 0;
					s2 = 0;
					d = md->returntype.type;

				} else {
					s1 = OFFSET(vftbl_t, interfacetable[0]) -
						sizeof(methodptr) * lm->class->index;

					s2 = sizeof(methodptr) * (lm - lm->class->methods);

					d = md->returntype.type;
				}

				i386_mov_membase_reg(cd, REG_ITMP1,
									 OFFSET(java_objectheader, vftbl),
									 REG_ITMP1);
				i386_mov_membase32_reg(cd, REG_ITMP1, s1, REG_ITMP2);
				i386_mov_membase32_reg(cd, REG_ITMP2, s2, REG_ITMP1);
				i386_call_reg(cd, REG_ITMP1);
				break;
			}

			/* d contains return type */

			if (d != TYPE_VOID) {
				d = reg_of_var(rd, iptr->dst, REG_NULL);

				if (IS_INT_LNG_TYPE(iptr->dst->type)) {
					if (IS_2_WORD_TYPE(iptr->dst->type)) {
						if (iptr->dst->flags & INMEMORY) {
							i386_mov_reg_membase(
                                cd, REG_RESULT, REG_SP, iptr->dst->regoff * 4);
							i386_mov_reg_membase(
                                cd, REG_RESULT2, REG_SP,
								iptr->dst->regoff * 4 + 4);
						} else {
  							log_text("RETURN: longs have to be in memory");
							assert(0);
						}

					} else {
						if (iptr->dst->flags & INMEMORY) {
							i386_mov_reg_membase(cd, REG_RESULT, REG_SP, iptr->dst->regoff * 4);

						} else {
							M_INTMOVE(REG_RESULT, iptr->dst->regoff);
						}
					}

				} else {
					/* fld from called function -- has other fpu_st_offset counter */
					fpu_st_offset++;
					store_reg_to_var_flt(iptr->dst, d);
				}
			}
			break;


		case ICMD_CHECKCAST:  /* ..., objectref ==> ..., objectref            */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: I|YES EDX: I|YES OUTPUT: REG_NULL*/ 

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
			codegen_threadcritrestart(cd, cd->mcodeptr - cd->mcodebase);
#endif
			var_to_reg_int(s1, src, REG_ITMP1);

			/* calculate interface checkcast code size */

			s2 = 2; /* mov_membase_reg */
			CALCOFFSETBYTES(s2, s1, OFFSET(java_objectheader, vftbl));

			s2 += (2 + 4 /* mov_membase32_reg */ + 2 + 4 /* sub imm32 */ +
				   2 /* test */ + 6 /* jcc */ + 2 + 4 /* mov_membase32_reg */ +
				   2 /* test */ + 6 /* jcc */);

			if (!super)
				s2 += (showdisassemble ? 5 : 0);

			/* calculate class checkcast code size */

			s3 = 2; /* mov_membase_reg */
			CALCOFFSETBYTES(s3, s1, OFFSET(java_objectheader, vftbl));

			s3 += 5 /* mov_imm_reg */ + 2 + 4 /* mov_membase32_reg */;

#if 0
			if (s1 != REG_ITMP1) {
				a += 2;
				CALCOFFSETBYTES(a, REG_ITMP3, OFFSET(vftbl_t, baseval));
				
				a += 2;
				CALCOFFSETBYTES(a, REG_ITMP3, OFFSET(vftbl_t, diffval));
				
				a += 2;
				
			} else
#endif
				{
					s3 += (2 + 4 /* mov_membase32_reg */ + 2 /* sub */ +
						   5 /* mov_imm_reg */ + 2 /* mov_membase_reg */);
					CALCOFFSETBYTES(s3, REG_ITMP3, OFFSET(vftbl_t, diffval));
				}

			s3 += 2 /* cmp */ + 6 /* jcc */;

			if (!super)
				s3 += (showdisassemble ? 5 : 0);

			/* if class is not resolved, check which code to call */

			if (!super) {
				i386_test_reg_reg(cd, s1, s1);
				i386_jcc(cd, I386_CC_Z, 5 + (showdisassemble ? 5 : 0) + 6 + 6 + s2 + 5 + s3);

				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_checkcast_instanceof_flags,
									(constant_classref *) iptr->target);

				if (showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

				i386_mov_imm_reg(cd, 0, REG_ITMP2); /* super->flags */
				i386_alu_imm_reg(cd, ALU_AND, ACC_INTERFACE, REG_ITMP2);
				i386_jcc(cd, I386_CC_Z, s2 + 5);
			}

			/* interface checkcast code */

			if (!super || (super->flags & ACC_INTERFACE)) {
				if (super) {
					i386_test_reg_reg(cd, s1, s1);
					i386_jcc(cd, I386_CC_Z, s2);
				}

				i386_mov_membase_reg(cd, s1,
									 OFFSET(java_objectheader, vftbl),
									 REG_ITMP2);

				if (!super) {
					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_checkcast_instanceof_interface,
										(constant_classref *) iptr->target);

					if (showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}
				}

				i386_mov_membase32_reg(cd, REG_ITMP2,
									   OFFSET(vftbl_t, interfacetablelength),
									   REG_ITMP3);
				i386_alu_imm32_reg(cd, ALU_SUB, superindex, REG_ITMP3);
				i386_test_reg_reg(cd, REG_ITMP3, REG_ITMP3);
				i386_jcc(cd, I386_CC_LE, 0);
				codegen_addxcastrefs(cd, cd->mcodeptr);
				i386_mov_membase32_reg(cd, REG_ITMP2,
									   OFFSET(vftbl_t, interfacetable[0]) -
									   superindex * sizeof(methodptr*),
									   REG_ITMP3);
				i386_test_reg_reg(cd, REG_ITMP3, REG_ITMP3);
				i386_jcc(cd, I386_CC_E, 0);
				codegen_addxcastrefs(cd, cd->mcodeptr);

				if (!super)
					i386_jmp_imm(cd, s3);
			}

			/* class checkcast code */

			if (!super || !(super->flags & ACC_INTERFACE)) {
				if (super) {
					i386_test_reg_reg(cd, s1, s1);
					i386_jcc(cd, I386_CC_Z, s3);
				}

				i386_mov_membase_reg(cd, s1,
									 OFFSET(java_objectheader, vftbl),
									 REG_ITMP2);

				if (!super) {
					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_checkcast_class,
										(constant_classref *) iptr->target);

					if (showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}
				}

				i386_mov_imm_reg(cd, (ptrint) supervftbl, REG_ITMP3);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
				codegen_threadcritstart(cd, cd->mcodeptr - cd->mcodebase);
#endif
				i386_mov_membase32_reg(cd, REG_ITMP2,
									   OFFSET(vftbl_t, baseval),
									   REG_ITMP2);

/* 				if (s1 != REG_ITMP1) { */
/* 					i386_mov_membase_reg(cd, REG_ITMP3, OFFSET(vftbl_t, baseval), REG_ITMP1); */
/* 					i386_mov_membase_reg(cd, REG_ITMP3, OFFSET(vftbl_t, diffval), REG_ITMP3); */
/* #if defined(USE_THREADS) && defined(NATIVE_THREADS) */
/* 					codegen_threadcritstop(cd, cd->mcodeptr - cd->mcodebase); */
/* #endif */
/* 					i386_alu_reg_reg(cd, ALU_SUB, REG_ITMP1, REG_ITMP2); */

/* 				} else { */
				i386_mov_membase32_reg(cd, REG_ITMP3,
									   OFFSET(vftbl_t, baseval),
									   REG_ITMP3);
				i386_alu_reg_reg(cd, ALU_SUB, REG_ITMP3, REG_ITMP2);
				i386_mov_imm_reg(cd, (ptrint) supervftbl, REG_ITMP3);
				i386_mov_membase_reg(cd, REG_ITMP3,
									 OFFSET(vftbl_t, diffval),
									 REG_ITMP3);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
				codegen_threadcritstop(cd, cd->mcodeptr - cd->mcodebase);
#endif
/* 				} */

				i386_alu_reg_reg(cd, ALU_CMP, REG_ITMP3, REG_ITMP2);
				i386_jcc(cd, I386_CC_A, 0);    /* (u) REG_ITMP2 > (u) REG_ITMP3 -> jump */
				codegen_addxcastrefs(cd, cd->mcodeptr);
			}
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			}
			break;

		case ICMD_INSTANCEOF: /* ..., objectref ==> ..., intresult            */

		                      /* op1:   0 == array, 1 == class                */
		                      /* val.a: (classinfo*) superclass               */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|D|YES ECX: YES S|D|EDX: S|D|YES OUTPUT: REG_NULL*/
			/* ????? Really necessary to block all ?????              */

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
			codegen_threadcritrestart(cd, cd->mcodeptr - cd->mcodebase);
#endif

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if (s1 == d) {
				M_INTMOVE(s1, REG_ITMP1);
				s1 = REG_ITMP1;
			}

			/* calculate interface instanceof code size */

			s2 = 2; /* mov_membase_reg */
			CALCOFFSETBYTES(s2, s1, OFFSET(java_objectheader, vftbl));

			s2 += (2 + 4 /* mov_membase32_reg */ + 2 + 4 /* alu_imm32_reg */ +
				   2 /* test */ + 6 /* jcc */ + 2 + 4 /* mov_membase32_reg */ +
				   2 /* test */ + 6 /* jcc */ + 5 /* mov_imm_reg */);

			if (!super)
				s2 += (showdisassemble ? 5 : 0);

			/* calculate class instanceof code size */

			s3 = 2; /* mov_membase_reg */
			CALCOFFSETBYTES(s3, s1, OFFSET(java_objectheader, vftbl));
			s3 += 5; /* mov_imm_reg */
			s3 += 2;
			CALCOFFSETBYTES(s3, REG_ITMP1, OFFSET(vftbl_t, baseval));
			s3 += 2;
			CALCOFFSETBYTES(s3, REG_ITMP2, OFFSET(vftbl_t, diffval));
			s3 += 2;
			CALCOFFSETBYTES(s3, REG_ITMP2, OFFSET(vftbl_t, baseval));

			s3 += (2 /* alu_reg_reg */ + 2 /* alu_reg_reg */ +
				   2 /* alu_reg_reg */ + 6 /* jcc */ + 5 /* mov_imm_reg */);

			if (!super)
				s3 += (showdisassemble ? 5 : 0);

			i386_alu_reg_reg(cd, ALU_XOR, d, d);

			/* if class is not resolved, check which code to call */

			if (!super) {
				i386_test_reg_reg(cd, s1, s1);
				i386_jcc(cd, I386_CC_Z, 5 + (showdisassemble ? 5 : 0) + 6 + 6 + s2 + 5 + s3);

				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_checkcast_instanceof_flags,
									(constant_classref *) iptr->target);

				if (showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

				i386_mov_imm_reg(cd, 0, REG_ITMP3); /* super->flags */
				i386_alu_imm32_reg(cd, ALU_AND, ACC_INTERFACE, REG_ITMP3);
				i386_jcc(cd, I386_CC_Z, s2 + 5);
			}

			/* interface instanceof code */

			if (!super || (super->flags & ACC_INTERFACE)) {
				if (super) {
					i386_test_reg_reg(cd, s1, s1);
					i386_jcc(cd, I386_CC_Z, s2);
				}

				i386_mov_membase_reg(cd, s1,
									 OFFSET(java_objectheader, vftbl),
									 REG_ITMP1);

				if (!super) {
					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_checkcast_instanceof_interface,
										(constant_classref *) iptr->target);

					if (showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}
				}

				i386_mov_membase32_reg(cd, REG_ITMP1,
									   OFFSET(vftbl_t, interfacetablelength),
									   REG_ITMP3);
				i386_alu_imm32_reg(cd, ALU_SUB, superindex, REG_ITMP3);
				i386_test_reg_reg(cd, REG_ITMP3, REG_ITMP3);

				a = (2 + 4 /* mov_membase32_reg */ + 2 /* test */ +
					 6 /* jcc */ + 5 /* mov_imm_reg */);

				i386_jcc(cd, I386_CC_LE, a);
				i386_mov_membase32_reg(cd, REG_ITMP1,
									   OFFSET(vftbl_t, interfacetable[0]) -
									   superindex * sizeof(methodptr*),
									   REG_ITMP1);
				i386_test_reg_reg(cd, REG_ITMP1, REG_ITMP1);
/*  					i386_setcc_reg(cd, I386_CC_A, d); */
/*  					i386_jcc(cd, I386_CC_BE, 5); */
				i386_jcc(cd, I386_CC_E, 5);
				i386_mov_imm_reg(cd, 1, d);

				if (!super)
					i386_jmp_imm(cd, s3);
			}

			/* class instanceof code */

			if (!super || !(super->flags & ACC_INTERFACE)) {
				if (super) {
					i386_test_reg_reg(cd, s1, s1);
					i386_jcc(cd, I386_CC_Z, s3);
				}

				i386_mov_membase_reg(cd, s1,
									 OFFSET(java_objectheader, vftbl),
									 REG_ITMP1);

				if (!super) {
					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_instanceof_class,
										(constant_classref *) iptr->target);

					if (showdisassemble) {
						M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
					}
				}

				i386_mov_imm_reg(cd, (ptrint) supervftbl, REG_ITMP2);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
				codegen_threadcritstart(cd, cd->mcodeptr - cd->mcodebase);
#endif
				i386_mov_membase_reg(cd, REG_ITMP1,
									 OFFSET(vftbl_t, baseval),
									 REG_ITMP1);
				i386_mov_membase_reg(cd, REG_ITMP2,
									 OFFSET(vftbl_t, diffval),
									 REG_ITMP3);
				i386_mov_membase_reg(cd, REG_ITMP2,
									 OFFSET(vftbl_t, baseval),
									 REG_ITMP2);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
				codegen_threadcritstop(cd, cd->mcodeptr - cd->mcodebase);
#endif
				i386_alu_reg_reg(cd, ALU_SUB, REG_ITMP2, REG_ITMP1);
				i386_alu_reg_reg(cd, ALU_XOR, d, d); /* may be REG_ITMP2 */
				i386_alu_reg_reg(cd, ALU_CMP, REG_ITMP3, REG_ITMP1);
				i386_jcc(cd, I386_CC_A, 5);
				i386_mov_imm_reg(cd, 1, d);
			}
  			store_reg_to_var_int(iptr->dst, d);
			}
			break;

		case ICMD_CHECKASIZE:  /* ..., size ==> ..., size                     */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: NO ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, ALU_CMP, 0, REG_SP, src->regoff * 4);
				
			} else {
				i386_test_reg_reg(cd, src->regoff, src->regoff);
			}
			i386_jcc(cd, I386_CC_L, 0);
			codegen_addxcheckarefs(cd, cd->mcodeptr);
			break;

		case ICMD_CHECKEXCEPTION:  /* ... ==> ...                             */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: YES ECX: NO EDX: NO OUTPUT: REG_NULL*/ 

			i386_test_reg_reg(cd, REG_RESULT, REG_RESULT);
			i386_jcc(cd, I386_CC_E, 0);
			codegen_addxexceptionrefs(cd, cd->mcodeptr);
			break;

		case ICMD_MULTIANEWARRAY:/* ..., cnt1, [cnt2, ...] ==> ..., arrayref  */
		                      /* op1 = dimension, val.a = array descriptor    */
			/* REG_RES Register usage: see icmd_uses_reg_res.inc */
			/* EAX: S|YES ECX: YES EDX: YES OUTPUT: EAX*/ 

			/* check for negative sizes and copy sizes to stack if necessary  */

  			MCODECHECK((iptr->op1 << 1) + 64);

			for (s1 = iptr->op1; --s1 >= 0; src = src->prev) {
				if (src->flags & INMEMORY) {
					i386_alu_imm_membase(cd, ALU_CMP, 0, REG_SP, src->regoff * 4);

				} else {
					i386_test_reg_reg(cd, src->regoff, src->regoff);
				}
				i386_jcc(cd, I386_CC_L, 0);
				codegen_addxcheckarefs(cd, cd->mcodeptr);

				/* 
				 * copy sizes to new stack location, because native function
				 * builtin_nmultianewarray access them as (int *)
				 */
				/*i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);*/
				/*i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, -(iptr->op1 - s1) * 4);*/

				/* copy SAVEDVAR sizes to stack */

				if (src->varkind != ARGVAR) {
					if (src->flags & INMEMORY) {
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 4, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, (s1 +3) * 4);

					} else {
						i386_mov_reg_membase(cd, src->regoff, REG_SP, (s1 + 3) * 4);
					}
				}
			}

			/* is a patcher function set? */

			if (iptr->target) {
				codegen_addpatchref(cd, cd->mcodeptr,
									(functionptr) iptr->target, iptr->val.a);

				if (showdisassemble) {
					M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
				}

				a = 0;

			} else {
				a = (ptrint) iptr->val.a;
			}

			/* a0 = dimension count */

			i386_mov_imm_membase(cd, iptr->op1, REG_SP, 0 * 4);

			/* a1 = arraydescriptor */

			i386_mov_imm_membase(cd, a, REG_SP, 1 * 4);

			/* a2 = pointer to dimensions = stack pointer */

			M_INTMOVE(REG_SP, REG_ITMP1);
			i386_alu_imm_reg(cd, ALU_ADD, 3 * 4, REG_ITMP1);
			i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, 2 * 4);

			i386_mov_imm_reg(cd, (ptrint) BUILTIN_multianewarray, REG_ITMP1);
			i386_call_reg(cd, REG_ITMP1);

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
					M_FLTMOVE(s1, rd->interfaces[len][s2].regoff);

				} else {
					log_text("double store");
					assert(0);
/*  					M_DST(s1, REG_SP, 4 * interfaces[len][s2].regoff); */
				}

			} else {
				var_to_reg_int(s1, src, REG_ITMP1);
				if (!IS_2_WORD_TYPE(rd->interfaces[len][s2].type)) {
					if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
						M_INTMOVE(s1, rd->interfaces[len][s2].regoff);

					} else {
						i386_mov_reg_membase(cd, s1, REG_SP, rd->interfaces[len][s2].regoff * 4);
					}

				} else {
					if (rd->interfaces[len][s2].flags & INMEMORY) {
						M_LNGMEMMOVE(s1, rd->interfaces[len][s2].regoff);

					} else {
						log_text("copy interface registers: longs have to be in memory (end)");
						assert(0);
					}
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

	u1 *xcodeptr = NULL;
	branchref *bref;

	for (bref = cd->xboundrefs; bref != NULL; bref = bref->next) {
		gen_resolvebranch(cd->mcodebase + bref->branchpos,
		                  bref->branchpos,
						  cd->mcodeptr - cd->mcodebase);

		MCODECHECK(100);

		/* move index register into REG_ITMP1 */
		i386_mov_reg_reg(cd, bref->reg, REG_ITMP1);                /* 2 bytes */

		i386_mov_imm_reg(cd, 0, REG_ITMP2_XPC);                    /* 5 bytes */
		dseg_adddata(cd, cd->mcodeptr);
		i386_mov_imm_reg(cd, bref->branchpos - 6, REG_ITMP3);      /* 5 bytes */
		i386_alu_reg_reg(cd, ALU_ADD, REG_ITMP3, REG_ITMP2_XPC);  /* 2 bytes */

		if (xcodeptr != NULL) {
			i386_jmp_imm(cd, (xcodeptr - cd->mcodeptr) - 5);

		} else {
			xcodeptr = cd->mcodeptr;

			i386_push_reg(cd, REG_ITMP2_XPC);

			/*PREPARE_NATIVE_STACKINFO;*/
			i386_push_imm(cd,0); /* the pushed XPC is directly below the java frame*/
			i386_push_imm(cd,0);
			i386_mov_imm_reg(cd,(s4)asm_prepare_native_stackinfo,REG_ITMP3);
			i386_call_reg(cd,REG_ITMP3);

			i386_alu_imm_reg(cd, ALU_SUB, 1 * 4, REG_SP);
			i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, 0 * 4);
			i386_mov_imm_reg(cd, (u4) new_arrayindexoutofboundsexception, REG_ITMP1);
			i386_call_reg(cd, REG_ITMP1);   /* return value is REG_ITMP1_XPTR */
			i386_alu_imm_reg(cd, ALU_ADD, 1 * 4, REG_SP);

			/*REMOVE_NATIVE_STACKINFO;*/
			i386_mov_imm_reg(cd,(s4)asm_remove_native_stackinfo,REG_ITMP3);
			i386_call_reg(cd,REG_ITMP3);

			i386_pop_reg(cd, REG_ITMP2_XPC);

			i386_mov_imm_reg(cd, (s4) asm_handle_exception, REG_ITMP3);
			i386_jmp_reg(cd, REG_ITMP3);
		}
	}

	/* generate negative array size check stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xcheckarefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch(cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  xcodeptr - cd->mcodebase - (5 + 5 + 2));
			continue;
		}

		gen_resolvebranch(cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  cd->mcodeptr - cd->mcodebase);

		MCODECHECK(100);

		i386_mov_imm_reg(cd, 0, REG_ITMP2_XPC);                    /* 5 bytes */
		dseg_adddata(cd, cd->mcodeptr);
		i386_mov_imm_reg(cd, bref->branchpos - 6, REG_ITMP1);      /* 5 bytes */
		i386_alu_reg_reg(cd, ALU_ADD, REG_ITMP1, REG_ITMP2_XPC);  /* 2 bytes */

		if (xcodeptr != NULL) {
			i386_jmp_imm(cd, (xcodeptr - cd->mcodeptr) - 5);

		} else {
			xcodeptr = cd->mcodeptr;

			i386_push_reg(cd, REG_ITMP2_XPC);

			/*PREPARE_NATIVE_STACKINFO;*/
			i386_push_imm(cd,0); /* the pushed XPC is directly below the java frame*/
			i386_push_imm(cd,0);
			i386_mov_imm_reg(cd,(s4)asm_prepare_native_stackinfo,REG_ITMP3);
			i386_call_reg(cd,REG_ITMP3);



			i386_mov_imm_reg(cd, (u4) new_negativearraysizeexception, REG_ITMP1);
			i386_call_reg(cd, REG_ITMP1);   /* return value is REG_ITMP1_XPTR */
			/*i386_alu_imm_reg(cd, ALU_ADD, 1 * 4, REG_SP);*/


			/*REMOVE_NATIVE_STACKINFO;*/
			i386_mov_imm_reg(cd,(s4)asm_remove_native_stackinfo,REG_ITMP3);
			i386_call_reg(cd,REG_ITMP3);


			i386_pop_reg(cd, REG_ITMP2_XPC);

			i386_mov_imm_reg(cd, (s4) asm_handle_exception, REG_ITMP3);
			i386_jmp_reg(cd, REG_ITMP3);
		}
	}

	/* generate cast check stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xcastrefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch(cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  xcodeptr - cd->mcodebase - (5 + 5 + 2));
			continue;
		}

		gen_resolvebranch(cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  cd->mcodeptr - cd->mcodebase);

		MCODECHECK(100);

		i386_mov_imm_reg(cd, 0, REG_ITMP2_XPC);                    /* 5 bytes */
		dseg_adddata(cd, cd->mcodeptr);
		i386_mov_imm_reg(cd, bref->branchpos - 6, REG_ITMP1);      /* 5 bytes */
		i386_alu_reg_reg(cd, ALU_ADD, REG_ITMP1, REG_ITMP2_XPC);  /* 2 bytes */

		if (xcodeptr != NULL) {
			i386_jmp_imm(cd, (xcodeptr - cd->mcodeptr) - 5);
		
		} else {
			xcodeptr = cd->mcodeptr;

			i386_push_reg(cd, REG_ITMP2_XPC);

			/*PREPARE_NATIVE_STACKINFO;*/
			i386_push_imm(cd,0); /* the pushed XPC is directly below the java frame*/
			i386_push_imm(cd,0);
			i386_mov_imm_reg(cd,(s4)asm_prepare_native_stackinfo,REG_ITMP3);
			i386_call_reg(cd,REG_ITMP3);


			i386_mov_imm_reg(cd, (u4) new_classcastexception, REG_ITMP1);
			i386_call_reg(cd, REG_ITMP1);   /* return value is REG_ITMP1_XPTR */
			/*i386_alu_imm_reg(cd, ALU_ADD, 1 * 4, REG_SP);*/


			/*REMOVE_NATIVE_STACKINFO;*/
			i386_mov_imm_reg(cd,(s4)asm_remove_native_stackinfo,REG_ITMP3);
			i386_call_reg(cd,REG_ITMP3);


			i386_pop_reg(cd, REG_ITMP2_XPC);

			i386_mov_imm_reg(cd, (u4) asm_handle_exception, REG_ITMP3);
			i386_jmp_reg(cd, REG_ITMP3);
		}
	}

	/* generate divide by zero check stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xdivrefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch(cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  xcodeptr - cd->mcodebase - (5 + 5 + 2));
			continue;
		}

		gen_resolvebranch(cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  cd->mcodeptr - cd->mcodebase);

		MCODECHECK(100);

		i386_mov_imm_reg(cd, 0, REG_ITMP2_XPC);                    /* 5 bytes */
		dseg_adddata(cd, cd->mcodeptr);
		i386_mov_imm_reg(cd, bref->branchpos - 6, REG_ITMP1);      /* 5 bytes */
		i386_alu_reg_reg(cd, ALU_ADD, REG_ITMP1, REG_ITMP2_XPC);  /* 2 bytes */

		if (xcodeptr != NULL) {
			i386_jmp_imm(cd, (xcodeptr - cd->mcodeptr) - 5);
		
		} else {
			xcodeptr = cd->mcodeptr;

			i386_push_reg(cd, REG_ITMP2_XPC);

			/*PREPARE_NATIVE_STACKINFO;*/
			i386_push_imm(cd,0); /* the pushed XPC is directly below the java frame*/
			i386_push_imm(cd,0);
			i386_mov_imm_reg(cd,(s4)asm_prepare_native_stackinfo,REG_ITMP3);
			i386_call_reg(cd,REG_ITMP3);



			i386_mov_imm_reg(cd, (u4) new_arithmeticexception, REG_ITMP1);
			i386_call_reg(cd, REG_ITMP1);   /* return value is REG_ITMP1_XPTR */

			/*REMOVE_NATIVE_STACKINFO;*/
			i386_mov_imm_reg(cd,(s4)asm_remove_native_stackinfo,REG_ITMP3);
			i386_call_reg(cd,REG_ITMP3);


			i386_pop_reg(cd, REG_ITMP2_XPC);

			i386_mov_imm_reg(cd, (s4) asm_handle_exception, REG_ITMP3);
			i386_jmp_reg(cd, REG_ITMP3);
		}
	}

	/* generate exception check stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xexceptionrefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch(cd->mcodebase + bref->branchpos,
							  bref->branchpos,
							  xcodeptr - cd->mcodebase - (5 + 5 + 2));
			continue;
		}

		gen_resolvebranch(cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  cd->mcodeptr - cd->mcodebase);

		MCODECHECK(200);

		i386_mov_imm_reg(cd, 0, REG_ITMP2_XPC);                    /* 5 bytes */
		dseg_adddata(cd, cd->mcodeptr);
		i386_mov_imm_reg(cd, bref->branchpos - 6, REG_ITMP1);      /* 5 bytes */
		i386_alu_reg_reg(cd, ALU_ADD, REG_ITMP1, REG_ITMP2_XPC);  /* 2 bytes */

		if (xcodeptr != NULL) {
			i386_jmp_imm(cd, (xcodeptr - cd->mcodeptr) - 5);
		
		} else {
			xcodeptr = cd->mcodeptr;

			i386_push_reg(cd, REG_ITMP2_XPC);

			/*PREPARE_NATIVE_STACKINFO;*/
			i386_push_imm(cd,0); /* the pushed XPC is directly below the java frame*/
			i386_push_imm(cd,0);
			i386_mov_imm_reg(cd,(s4)asm_prepare_native_stackinfo,REG_ITMP3);
			i386_call_reg(cd,REG_ITMP3);

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			i386_mov_imm_reg(cd, (s4) &builtin_get_exceptionptrptr, REG_ITMP1);
			i386_call_reg(cd, REG_ITMP1);
			i386_mov_membase_reg(cd, REG_RESULT, 0, REG_ITMP3);
			i386_mov_imm_membase(cd, 0, REG_RESULT, 0);
			i386_mov_reg_reg(cd, REG_ITMP3, REG_ITMP1_XPTR);
#else
			i386_mov_imm_reg(cd, (s4) &_exceptionptr, REG_ITMP3);
			i386_mov_membase_reg(cd, REG_ITMP3, 0, REG_ITMP1_XPTR);
			i386_mov_imm_membase(cd, 0, REG_ITMP3, 0);
#endif
			i386_push_imm(cd, 0);
			i386_push_reg(cd, REG_ITMP1_XPTR);

/*get the fillInStackTrace Method ID. I simulate a native call here, because I do not want to mess around with the
java stack at this point*/
			i386_mov_membase_reg(cd, REG_ITMP1_XPTR, OFFSET(java_objectheader, vftbl), REG_ITMP3);
			i386_mov_membase_reg(cd, REG_ITMP3, OFFSET(vftbl_t, class), REG_ITMP1);
			i386_push_imm(cd, (u4) utf_void__java_lang_Throwable);
			i386_push_imm(cd, (u4) utf_fillInStackTrace);
			i386_push_reg(cd, REG_ITMP1);
			i386_mov_imm_reg(cd, (s4) class_resolvemethod, REG_ITMP3);
			i386_call_reg(cd, REG_ITMP3);
/*cleanup parameters of class_resolvemethod*/
			i386_alu_imm_reg(cd, ALU_ADD,3*4 /*class reference + 2x string reference*/,REG_SP);
/*prepare call to asm_calljavafunction2 */			
			i386_push_imm(cd, 0);
			i386_push_imm(cd, TYPE_ADR); /* --> call block (TYPE,Exceptionptr), each 8 byte  (make this dynamic) (JOWENN)*/
			i386_push_reg(cd, REG_SP);
			i386_push_imm(cd, sizeof(jni_callblock));
			i386_push_imm(cd, 1);
			i386_push_reg(cd, REG_RESULT);
			
			i386_mov_imm_reg(cd, (s4) asm_calljavafunction2, REG_ITMP3);
			i386_call_reg(cd, REG_ITMP3);

			/* check exceptionptr + fail (JOWENN)*/			

			i386_alu_imm_reg(cd, ALU_ADD,6*4,REG_SP);

			i386_pop_reg(cd, REG_ITMP1_XPTR);
			i386_pop_reg(cd, REG_ITMP3); /* just remove the no longer needed 0 from the stack*/

			/*REMOVE_NATIVE_STACKINFO;*/
			i386_mov_imm_reg(cd,(s4)asm_remove_native_stackinfo,REG_ITMP3);
			i386_call_reg(cd,REG_ITMP3);


			i386_pop_reg(cd, REG_ITMP2_XPC);

			i386_mov_imm_reg(cd, (s4) asm_handle_exception, REG_ITMP3);
			i386_jmp_reg(cd, REG_ITMP3);
		}
	}

	/* generate null pointer check stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xnullrefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch(cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  xcodeptr - cd->mcodebase - (5 + 5 + 2));
			continue;
		}

		gen_resolvebranch(cd->mcodebase + bref->branchpos, 
						  bref->branchpos,
						  cd->mcodeptr - cd->mcodebase);
		
		MCODECHECK(100);

		i386_mov_imm_reg(cd, 0, REG_ITMP2_XPC);                    /* 5 bytes */
		dseg_adddata(cd, cd->mcodeptr);
		i386_mov_imm_reg(cd, bref->branchpos - 6, REG_ITMP1);      /* 5 bytes */
		i386_alu_reg_reg(cd, ALU_ADD, REG_ITMP1, REG_ITMP2_XPC);  /* 2 bytes */
		
		if (xcodeptr != NULL) {
			i386_jmp_imm(cd, (xcodeptr - cd->mcodeptr) - 5);
			
		} else {
			xcodeptr = cd->mcodeptr;
			
			i386_push_reg(cd, REG_ITMP2_XPC);

			/*PREPARE_NATIVE_STACKINFO;*/
			i386_push_imm(cd,0); /* the pushed XPC is directly below the java frame*/
			i386_push_imm(cd,0);
			i386_mov_imm_reg(cd,(s4)asm_prepare_native_stackinfo,REG_ITMP3);
			i386_call_reg(cd,REG_ITMP3);



#if 0
			/* create native call block*/
			i386_alu_imm_reg(cd, ALU_SUB, 3*4, REG_SP); /* build stack frame (4 * 4 bytes) */


			i386_mov_imm_reg(cd, (s4) codegen_stubcalled,REG_ITMP1);
			i386_call_reg(cd, REG_ITMP1);                /*call    codegen_stubcalled*/

			i386_mov_imm_reg(cd, (s4) builtin_asm_get_stackframeinfo,REG_ITMP1);
			i386_call_reg(cd, REG_ITMP1);                /*call    builtin_asm_get_stackframeinfo*/
			i386_mov_imm_membase(cd, 0,REG_SP, 2*4);	/* builtin */
			i386_mov_reg_membase(cd, REG_RESULT,REG_SP,1*4); /* save thread pointer  to native call stack*/
			i386_mov_membase_reg(cd, REG_RESULT,0,REG_ITMP2); /* get old value of thread specific native call stack */
			i386_mov_reg_membase(cd, REG_ITMP2,REG_SP,0*4);	/* store value on stack */
			i386_mov_reg_membase(cd, REG_SP,REG_RESULT,0); /* store pointer to new stack frame information */
#endif				

			i386_mov_imm_reg(cd, (u4) new_nullpointerexception, REG_ITMP1);
			i386_call_reg(cd, REG_ITMP1);   /* return value is REG_ITMP1_XPTR */

			/*REMOVE_NATIVE_STACKINFO;*/
			i386_mov_imm_reg(cd,(s4)asm_remove_native_stackinfo,REG_ITMP3);
			i386_call_reg(cd,REG_ITMP3);


#if 0
			/* restore native call stack */
			i386_mov_membase_reg(cd, REG_SP,0,REG_ITMP2);
			i386_mov_membase_reg(cd, REG_SP,4,REG_ITMP3);
			i386_mov_reg_membase(cd, REG_ITMP2,REG_ITMP3,0);
			i386_alu_imm_reg(cd, ALU_ADD,3*4,REG_SP);
#endif

			i386_pop_reg(cd, REG_ITMP2_XPC);

			i386_mov_imm_reg(cd, (s4) asm_handle_exception, REG_ITMP3);
			i386_jmp_reg(cd, REG_ITMP3);
		}
	}

	/* generate put/getstatic stub call code */

	{
		patchref    *pref;
		codegendata *tmpcd;
		u8           mcode;

		tmpcd = DNEW(codegendata);

		for (pref = cd->patchrefs; pref != NULL; pref = pref->next) {
			/* check code segment size */

			MCODECHECK(64);

			/* Get machine code which is patched back in later. A             */
			/* `call rel32' is 5 bytes long.                                  */

			xcodeptr = cd->mcodebase + pref->branchpos;
			mcode = *((u8 *) xcodeptr);

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			/* create a virtual java_objectheader */

			*((ptrint *) (cd->mcodeptr + 0)) = 0;                    /* vftbl */
			*((ptrint *) (cd->mcodeptr + 4)) = (ptrint) get_dummyLR(); /* monitorPtr */

			cd->mcodeptr += 2 * 4;
#endif

			/* patch in `call rel32' to call the following code               */

			tmpcd->mcodeptr = xcodeptr;     /* set dummy mcode pointer        */
			i386_call_imm(tmpcd, cd->mcodeptr - (xcodeptr + 5));

			/* move pointer to java_objectheader onto stack */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			i386_call_imm(cd, 0);
			i386_alu_imm_membase(cd, ALU_SUB, 5 + 2 * 4, REG_SP, 0);
#else
			i386_push_imm(cd, 0);
#endif

			/* move machine code bytes and classinfo pointer into registers */

			i386_push_imm(cd, (ptrint) (mcode >> 32));
			i386_push_imm(cd, (ptrint) mcode);
			i386_push_imm(cd, (ptrint) pref->ref);

			i386_push_imm(cd, (ptrint) pref->patcher);

			i386_mov_imm_reg(cd, (ptrint) asm_wrapper_patcher, REG_ITMP3);
			i386_jmp_reg(cd, REG_ITMP3);
		}
	}
	}
	
	codegen_finish(m, cd, (ptrint) (cd->mcodeptr - cd->mcodebase));
}


/* createcompilerstub **********************************************************

   Creates a stub routine which calls the compiler.
	
*******************************************************************************/

#define COMPILERSTUB_SIZE    12

functionptr createcompilerstub(methodinfo *m)
{
    u1          *s;                     /* memory to hold the stub            */
	codegendata *cd;
	s4           dumpsize;

	s = CNEW(u1, COMPILERSTUB_SIZE);

	/* mark start of dump memory area */

	dumpsize = dump_size();
	
	cd = DNEW(codegendata);
    cd->mcodeptr = s;

    i386_mov_imm_reg(cd, (ptrint) m, REG_ITMP1);

	/* we use REG_ITMP3 cause ECX (REG_ITMP2) is used for patching  */
    i386_mov_imm_reg(cd, (ptrint) asm_call_jit_compiler, REG_ITMP3);
    i386_jmp_reg(cd, REG_ITMP3);

#if defined(STATISTICS)
	if (opt_stat)
		count_cstub_len += COMPILERSTUB_SIZE;
#endif

	/* release dump area */

	dump_release(dumpsize);
	
    return (functionptr) (ptrint) s;
}


/* createnativestub ************************************************************

   Creates a stub routine which calls a native method.

*******************************************************************************/

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
/* this way we can call the function directly with a memory call */

static java_objectheader **(*callgetexceptionptrptr)() = builtin_get_exceptionptrptr;
#endif

functionptr createnativestub(functionptr f, methodinfo *m, codegendata *cd,
							 registerdata *rd)
{
	methoddesc *md;
	methoddesc *nmd;
	s4          nativeparams;
	s4          i, j;                   /* count variables                    */
	s4          t;
	s4          s1, s2, off;
	s4          stackframesize;
	s4          stackframeoffset;

	/* set some variables */

	nativeparams = (m->flags & ACC_STATIC) ? 2 : 1;

	/* initial 4 bytes is space for jni env, + 4 byte thread pointer + 4 byte */
	/* previous pointer + method info + 4 offset native                       */
	/* already handled by nmd->memuse */

	stackframesize = /* 4 + */ 16;
	stackframeoffset = 4;


	/* create new method descriptor with additional native parameters */

	md = m->parseddesc;
	
	nmd = (methoddesc *) DMNEW(u1, sizeof(methoddesc) - sizeof(typedesc) +
							   md->paramcount * sizeof(typedesc) +
							   nativeparams * sizeof(typedesc));

	nmd->paramcount = md->paramcount + nativeparams;

	nmd->params = DMNEW(paramdesc, nmd->paramcount);

	nmd->paramtypes[0].type = TYPE_ADR; /* add environment pointer            */

	if (m->flags & ACC_STATIC)
		nmd->paramtypes[1].type = TYPE_ADR; /* add class pointer              */

	MCOPY(nmd->paramtypes + nativeparams, md->paramtypes, typedesc,
		  md->paramcount);

	md_param_alloc(nmd);


	/* create method header */

	(void) dseg_addaddress(cd, m);                          /* MethodPointer  */
	(void) dseg_adds4(cd, 0 * 8);                           /* FrameSize      */
	(void) dseg_adds4(cd, 0);                               /* IsSync         */
	(void) dseg_adds4(cd, 0);                               /* IsLeaf         */
	(void) dseg_adds4(cd, 0);                               /* IntSave        */
	(void) dseg_adds4(cd, 0);                               /* FltSave        */
	(void) dseg_addlinenumbertablesize(cd);
	(void) dseg_adds4(cd, 0);                               /* ExTableSize    */


	/* initialize mcode variables */
	
	cd->mcodeptr = (u1 *) cd->mcodebase;
	cd->mcodeend = (s4 *) (cd->mcodebase + cd->mcodesize);


	/* if function is static, check for initialized */

	if (m->flags & ACC_STATIC) {
		/* already handled by nmd->memuse */
/*  		stackframesize += 4; */
		stackframeoffset += 4;

		/* if class isn't yet initialized, do it */

		if (!m->class->initialized) {
			codegen_addpatchref(cd, cd->mcodeptr, PATCHER_clinit, m->class);

			if (showdisassemble) {
				M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
			}
		}
	}

	if (runverbose) {
		s4 p, t;
		s4 stack_off = 0;

		i386_alu_imm_reg(cd, ALU_SUB, TRACE_ARGS_NUM * 8 + 4, REG_SP);
    
		for (p = 0; p < md->paramcount && p < TRACE_ARGS_NUM; p++) {
			t = md->paramtypes[p].type;
			if (IS_INT_LNG_TYPE(t)) {
				if (IS_2_WORD_TYPE(t)) {
					i386_mov_membase_reg(cd, REG_SP,
					    4 + TRACE_ARGS_NUM * 8 + 4 + stack_off, REG_ITMP1);
					i386_mov_membase_reg(cd, REG_SP,
				        4 + TRACE_ARGS_NUM * 8 + 4 + stack_off + 4, REG_ITMP2);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, p * 8);
					i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, p * 8 + 4);

				} else if (t == TYPE_ADR) {
					i386_mov_membase_reg(cd, REG_SP,
                        4 + TRACE_ARGS_NUM * 8 + 4 + stack_off, REG_ITMP1);
					i386_alu_reg_reg(cd, ALU_XOR, REG_ITMP2, REG_ITMP2);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, p * 8);
					i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, p * 8 + 4);

				} else {
					i386_mov_membase_reg(cd, REG_SP,
                        4 + TRACE_ARGS_NUM * 8 + 4 + stack_off, EAX);
					i386_cltd(cd);
					i386_mov_reg_membase(cd, EAX, REG_SP, p * 8);
					i386_mov_reg_membase(cd, EDX, REG_SP, p * 8 + 4);
				}

			} else {
				if (!IS_2_WORD_TYPE(t)) {
					i386_flds_membase(cd, REG_SP,
					    4 + TRACE_ARGS_NUM * 8 + 4 + stack_off);
					i386_fstps_membase(cd, REG_SP, p * 8);
					i386_alu_reg_reg(cd, ALU_XOR, REG_ITMP2, REG_ITMP2);
					i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, p * 8 + 4);

				} else {
					i386_fldl_membase(cd, REG_SP,
					    4 + TRACE_ARGS_NUM * 8 + 4 + stack_off);
					i386_fstpl_membase(cd, REG_SP, p * 8);
				}
			}
			stack_off += (IS_2_WORD_TYPE(t)) ? 8 : 4;
		}
	
		i386_alu_reg_reg(cd, ALU_XOR, REG_ITMP1, REG_ITMP1);
		for (p = md->paramcount; p < TRACE_ARGS_NUM; p++) {
			i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, p * 8);
			i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, p * 8 + 4);
		}

		i386_mov_imm_membase(cd, (ptrint) m, REG_SP, TRACE_ARGS_NUM * 8);

		i386_mov_imm_reg(cd, (ptrint) builtin_trace_args, REG_ITMP1);
		i386_call_reg(cd, REG_ITMP1);

		i386_alu_imm_reg(cd, ALU_ADD, TRACE_ARGS_NUM * 8 + 4, REG_SP);
	}

	/* Mark the whole fpu stack as free for native functions (only for saved  */
	/* register count == 0).                                                  */
	i386_ffree_reg(cd, 0);
	i386_ffree_reg(cd, 1);
	i386_ffree_reg(cd, 2);
	i386_ffree_reg(cd, 3);
	i386_ffree_reg(cd, 4);
	i386_ffree_reg(cd, 5);
	i386_ffree_reg(cd, 6);
	i386_ffree_reg(cd, 7);

	/* calculate stackframe size for native function */

	stackframesize += nmd->memuse * 4;

	i386_alu_imm_reg(cd, ALU_SUB, stackframesize, REG_SP);

	/* CREATE DYNAMIC STACK INFO -- BEGIN*/

	i386_mov_imm_membase(cd,0,REG_SP,stackframesize-4);
	i386_mov_imm_membase(cd, (ptrint) m, REG_SP,stackframesize-8);
	i386_mov_imm_reg(cd, (ptrint) builtin_asm_get_stackframeinfo, REG_ITMP1);
	i386_call_reg(cd, REG_ITMP1);
                                                /*save thread specific pointer*/
	i386_mov_reg_membase(cd, REG_RESULT,REG_SP,stackframesize-12); 
	i386_mov_membase_reg(cd, REG_RESULT,0,REG_ITMP2); 
  /*save previous value of memory adress pointed to by thread specific pointer*/
	i386_mov_reg_membase(cd, REG_ITMP2,REG_SP,stackframesize-16);
	i386_mov_reg_reg(cd, REG_SP,REG_ITMP2);
	i386_alu_imm_reg(cd, ALU_ADD,stackframesize-16,REG_ITMP2);
	i386_mov_reg_membase(cd, REG_ITMP2,REG_RESULT,0);

	/* CREATE DYNAMIC STACK INFO -- END*/

	/* copy arguments into new stackframe */

	for (i = md->paramcount - 1, j = i + nativeparams; i >= 0; i--, j--) {
		t = md->paramtypes[i].type;

		if (!md->params[i].inmemory) {
			/* no integer argument registers */
		} else {       /* float/double in memory can be copied like int/longs */
			s1 = md->params[i].regoff * 4 + stackframesize + (1 * 4);
			s2 = nmd->params[j].regoff * 4;

			i386_mov_membase_reg(cd, REG_SP, s1, REG_ITMP1);
			i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, s2);
			if (IS_2_WORD_TYPE(t)) {
				i386_mov_membase_reg(cd, REG_SP, s1 + 4, REG_ITMP1);
				i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, s2 + 4);
			}
		}
	}

	/* if function is static, put class into second argument */

	if (m->flags & ACC_STATIC)
		i386_mov_imm_membase(cd, (ptrint) m->class, REG_SP, 4);

	/* put env into first argument */

	i386_mov_imm_membase(cd, (ptrint) &env, REG_SP, 0);

	/* call the native function */

#if !defined(STATIC_CLASSPATH)
	if (f == NULL)
		codegen_addpatchref(cd, cd->mcodeptr, PATCHER_resolve_native, m);

		if (showdisassemble) {
			M_NOP; M_NOP; M_NOP; M_NOP; M_NOP;
		}
#endif

	i386_mov_imm_reg(cd, (ptrint) f, REG_ITMP1);
	i386_call_reg(cd, REG_ITMP1);


	/* REMOVE DYNAMIC STACK INFO -BEGIN */
    i386_push_reg(cd, REG_RESULT2);
    i386_mov_membase_reg(cd, REG_SP,stackframesize-12,REG_ITMP2); /*old value*/
    i386_mov_membase_reg(cd, REG_SP,stackframesize-8,REG_RESULT2); /*pointer*/
    i386_mov_reg_membase(cd, REG_ITMP2,REG_RESULT2,0);
    i386_pop_reg(cd, REG_RESULT2);
	/* REMOVE DYNAMIC STACK INFO -END */

    i386_alu_imm_reg(cd, ALU_ADD, stackframesize, REG_SP);


    if (runverbose) {
        i386_alu_imm_reg(cd, ALU_SUB, 4 + 8 + 8 + 4, REG_SP);
		
        i386_mov_imm_membase(cd, (ptrint) m, REG_SP, 0);
		
        i386_mov_reg_membase(cd, REG_RESULT, REG_SP, 4);
        i386_mov_reg_membase(cd, REG_RESULT2, REG_SP, 4 + 4);
		
        i386_fstl_membase(cd, REG_SP, 4 + 8);
        i386_fsts_membase(cd, REG_SP, 4 + 8 + 8);

        i386_mov_imm_reg(cd, (ptrint) builtin_displaymethodstop, REG_ITMP1);
        i386_call_reg(cd, REG_ITMP1);
		
        i386_mov_membase_reg(cd, REG_SP, 4, REG_RESULT);
        i386_mov_membase_reg(cd, REG_SP, 4 + 4, REG_RESULT2);
		
        i386_alu_imm_reg(cd, ALU_ADD, 4 + 8 + 8 + 4, REG_SP);
    }


	/* check for exception */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	/* we can't use REG_ITMP3 == REG_RESULT2 */

	i386_push_reg(cd, REG_RESULT);
	i386_push_reg(cd, REG_RESULT2);
/* 	i386_call_mem(cd, (ptrint) builtin_get_exceptionptrptr); */
	i386_call_mem(cd, (ptrint) &callgetexceptionptrptr);
	i386_mov_membase_reg(cd, REG_RESULT, 0, REG_ITMP2);
	i386_test_reg_reg(cd, REG_ITMP2, REG_ITMP2);
	i386_pop_reg(cd, REG_RESULT2);
	i386_pop_reg(cd, REG_RESULT);
#else
	i386_mov_imm_reg(cd, (ptrint) &_exceptionptr, REG_ITMP2);
	i386_mov_membase_reg(cd, REG_ITMP2, 0, REG_ITMP2);
	i386_test_reg_reg(cd, REG_ITMP2, REG_ITMP2);
#endif
	i386_jcc(cd, I386_CC_NE, 1);

	i386_ret(cd);


	/* handle exception */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	i386_push_reg(cd, REG_ITMP2);
/* 	i386_call_mem(cd, (ptrint) builtin_get_exceptionptrptr); */
	i386_call_mem(cd, (ptrint) &callgetexceptionptrptr);
	i386_mov_imm_membase(cd, 0, REG_RESULT, 0);
	i386_pop_reg(cd, REG_ITMP1_XPTR);
#else
	i386_mov_reg_reg(cd, REG_ITMP2, REG_ITMP1_XPTR);
	i386_mov_imm_reg(cd, (ptrint) &_exceptionptr, REG_ITMP2);
	i386_mov_imm_membase(cd, 0, REG_ITMP2, 0);
#endif
	i386_mov_membase_reg(cd, REG_SP, 0, REG_ITMP2_XPC);
	i386_alu_imm_reg(cd, ALU_SUB, 2, REG_ITMP2_XPC);

	i386_mov_imm_reg(cd, (ptrint) asm_handle_nat_exception, REG_ITMP3);
	i386_jmp_reg(cd, REG_ITMP3);


	/* process patcher calls **************************************************/

	{
		u1          *xcodeptr;
		patchref    *pref;
		codegendata *tmpcd;
		u8           mcode;

		tmpcd = DNEW(codegendata);

		for (pref = cd->patchrefs; pref != NULL; pref = pref->next) {
			/* Get machine code which is patched back in later. A             */
			/* `call rel32' is 5 bytes long.                                  */

			xcodeptr = cd->mcodebase + pref->branchpos;
			mcode =  *((u8 *) xcodeptr);

			/* patch in `call rel32' to call the following code               */

			tmpcd->mcodeptr = xcodeptr;     /* set dummy mcode pointer        */
			i386_call_imm(tmpcd, cd->mcodeptr - (xcodeptr + 5));

			/* move pointer to java_objectheader onto stack */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			/* create a virtual java_objectheader */

			(void) dseg_addaddress(cd, get_dummyLR());          /* monitorPtr */
			off = dseg_addaddress(cd, NULL);                    /* vftbl      */

			i386_mov_imm_reg(cd, 0, REG_ITMP3);
			dseg_adddata(cd, cd->mcodeptr);
			M_AADD_IMM(off, REG_ITMP3);
			M_PUSH(REG_ITMP3);
#else
			M_PUSH_IMM(0);
#endif

			/* move machine code bytes and classinfo pointer onto stack */

			i386_push_imm(cd, (ptrint) (mcode >> 32));
			i386_push_imm(cd, (ptrint) mcode);
			i386_push_imm(cd, (ptrint) pref->ref);

			i386_push_imm(cd, (ptrint) pref->patcher);

			i386_mov_imm_reg(cd, (ptrint) asm_wrapper_patcher, REG_ITMP3);
			i386_jmp_reg(cd, REG_ITMP3);
		}
	}

	codegen_finish(m, cd, (s4) ((u1 *) cd->mcodeptr - cd->mcodebase));

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
