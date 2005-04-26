/* vm/jit/x86_64/codegen.c - machine code generator for x86_64

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

   $Id: codegen.c 2383 2005-04-26 16:11:58Z twisti $

*/


#define _GNU_SOURCE

#include <stdio.h>
#include <ucontext.h>

#include "cacao/cacao.h"
#include "native/native.h"
#include "vm/global.h"
#include "vm/builtin.h"
#include "vm/loader.h"
#include "vm/tables.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/jit.h"
#include "vm/jit/reg.h"
#include "vm/jit/parse.h"
#include "vm/jit/patcher.h"
#include "vm/jit/x86_64/arch.h"
#include "vm/jit/x86_64/codegen.h"
#include "vm/jit/x86_64/emitfuncs.h"
#include "vm/jit/x86_64/types.h"
#include "vm/jit/x86_64/asmoffsets.h"
#include "vm/jit/helper.h"

/* register descripton - array ************************************************/

/* #define REG_RES   0         reserved register for OS or code generator     */
/* #define REG_RET   1         return value register                          */
/* #define REG_EXC   2         exception value register (only old jit)        */
/* #define REG_SAV   3         (callee) saved register                        */
/* #define REG_TMP   4         scratch temporary register (caller saved)      */
/* #define REG_ARG   5         argument register (caller saved)               */

/* #define REG_END   -1        last entry in tables                           */

static int nregdescint[] = {
    REG_RET, REG_ARG, REG_ARG, REG_TMP, REG_RES, REG_SAV, REG_ARG, REG_ARG,
    REG_ARG, REG_ARG, REG_RES, REG_RES, REG_SAV, REG_SAV, REG_SAV, REG_SAV,
    REG_END
};


static int nregdescfloat[] = {
    REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG,
    REG_RES, REG_RES, REG_RES, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP,
    REG_END
};


/* Include independent code generation stuff -- include after register        */
/* descriptions to avoid extern definitions.                                  */

#include "vm/jit/codegen.inc"
#include "vm/jit/reg.inc"
#ifdef LSRA
#include "vm/jit/lsra.inc"
#endif


void codegen_dummy_func() { log_text("codegen_dummy_func"); }

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void thread_restartcriticalsection(ucontext_t *uc)
{
	void *critical;

	critical = thread_checkcritical((void *) uc->uc_mcontext.gregs[REG_RIP]);

	if (critical)
		uc->uc_mcontext.gregs[REG_RIP] = (u8) critical;
}
#endif


/* NullPointerException signal handler for hardware null pointer check */

void catch_NullPointerException(int sig, siginfo_t *siginfo, void *_p)
{
	sigset_t nsig;

	struct ucontext *_uc = (struct ucontext *) _p;
	struct sigcontext *sigctx = (struct sigcontext *) &_uc->uc_mcontext;
	struct sigaction act;
	java_objectheader *xptr;

	/* Reset signal handler - necessary for SysV, does no harm for BSD */
	
	act.sa_sigaction = catch_NullPointerException;       /* reinstall handler */
	act.sa_flags = SA_SIGINFO;
	sigaction(sig, &act, NULL);
	
	sigemptyset(&nsig);
	sigaddset(&nsig, sig);
	sigprocmask(SIG_UNBLOCK, &nsig, NULL);               /* unblock signal    */

#if 0
	xptr = new_nullpointerexception();

	sigctx->rax = (u8) xptr;                             /* REG_ITMP1_XPTR    */
	sigctx->r10 = sigctx->rip;                           /* REG_ITMP2_XPC     */
	sigctx->rip = (u8) asm_handle_exception;
#endif

	sigctx->rax = (u8) string_java_lang_NullPointerException;
	sigctx->r10 = sigctx->rip;                           /* REG_ITMP2_XPC     */
	sigctx->rip = (u8) asm_throw_and_handle_exception;

	return;
}


/* ArithmeticException signal handler for hardware divide by zero check */

void catch_ArithmeticException(int sig, siginfo_t *siginfo, void *_p)
{
	sigset_t nsig;

	struct ucontext *_uc = (struct ucontext *) _p;
	struct sigcontext *sigctx = (struct sigcontext *) &_uc->uc_mcontext;
	struct sigaction act;
	java_objectheader *xptr;

	/* Reset signal handler - necessary for SysV, does no harm for BSD */

	act.sa_sigaction = catch_ArithmeticException;        /* reinstall handler */
	act.sa_flags = SA_SIGINFO;
	sigaction(sig, &act, NULL);

	sigemptyset(&nsig);
	sigaddset(&nsig, sig);
	sigprocmask(SIG_UNBLOCK, &nsig, NULL);               /* unblock signal    */

#if 0
	xptr = new_arithmeticexception();

	sigctx->rax = (u8) xptr;                             /* REG_ITMP1_XPTR    */
#endif
	sigctx->r10 = sigctx->rip;                           /* REG_ITMP2_XPC     */
	sigctx->rip = (u8) asm_throw_and_handle_hardware_arithmetic_exception;

	return;
}


void init_exceptions(void)
{
	struct sigaction act;

	/* install signal handlers we need to convert to exceptions */
	sigemptyset(&act.sa_mask);

	if (!checknull) {
#if defined(SIGSEGV)
		act.sa_sigaction = catch_NullPointerException;
		act.sa_flags = SA_SIGINFO;
		sigaction(SIGSEGV, &act, NULL);
#endif

#if defined(SIGBUS)
		act.sa_sigaction = catch_NullPointerException;
		act.sa_flags = SA_SIGINFO;
		sigaction(SIGBUS, &act, NULL);
#endif
	}

	act.sa_sigaction = catch_ArithmeticException;
	act.sa_flags = SA_SIGINFO;
	sigaction(SIGFPE, &act, NULL);
}


/* function gen_mcode **********************************************************

	generates machine code

*******************************************************************************/

void codegen(methodinfo *m, codegendata *cd, registerdata *rd)
{
	s4              len, s1, s2, s3, d;
	u2              currentline;
	ptrint          a;
	s4              parentargs_base;
	stackptr        src;
	varinfo        *var;
	basicblock     *bptr;
	instruction    *iptr;
	exceptiontable *ex;

	{
	s4 i, p, pa, t, l;
	s4 savedregs_num;

  	savedregs_num = 0;

	/* space to save used callee saved registers */

	savedregs_num += (rd->savintregcnt - rd->maxsavintreguse);
	savedregs_num += (rd->savfltregcnt - rd->maxsavfltreguse);

	parentargs_base = rd->maxmemuse + savedregs_num;

#if defined(USE_THREADS)           /* space to save argument of monitor_enter */

	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		parentargs_base++;

#endif

    /* Keep stack of non-leaf functions 16-byte aligned for calls into native */
	/* code e.g. libc or jni (alignment problems with movaps).                */

	if (!m->isleafmethod || runverbose)
		parentargs_base |= 0x1;

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
		(void) dseg_adds4(cd, (rd->maxmemuse + 1) * 8);     /* IsSync         */
	else

#endif

		(void) dseg_adds4(cd, 0);                           /* IsSync         */
	                                       
	(void) dseg_adds4(cd, m->isleafmethod);                 /* IsLeaf         */
	(void) dseg_adds4(cd, rd->savintregcnt - rd->maxsavintreguse);/* IntSave  */
	(void) dseg_adds4(cd, rd->savfltregcnt - rd->maxsavfltreguse);/* FltSave  */

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
	
	cd->mcodeptr = (u1 *) cd->mcodebase;
	cd->mcodeend = (s4 *) (cd->mcodebase + cd->mcodesize);
	MCODECHECK(128 + m->paramcount);

	/* create stack frame (if necessary) */

	if (parentargs_base) {
		x86_64_alu_imm_reg(cd, X86_64_SUB, parentargs_base * 8, REG_SP);
	}

	/* save used callee saved registers */

  	p = parentargs_base;
	for (i = rd->savintregcnt - 1; i >= rd->maxsavintreguse; i--) {
 		p--; x86_64_mov_reg_membase(cd, rd->savintregs[i], REG_SP, p * 8);
	}
	for (i = rd->savfltregcnt - 1; i >= rd->maxsavfltreguse; i--) {
		p--; x86_64_movq_reg_membase(cd, rd->savfltregs[i], REG_SP, p * 8);
	}

	/* take arguments out of register or stack frame */

 	for (p = 0, l = 0, s1 = 0, s2 = 0; p < m->paramcount; p++) {
 		t = m->paramtypes[p];
 		var = &(rd->locals[l][t]);
 		l++;
 		if (IS_2_WORD_TYPE(t))    /* increment local counter for 2 word types */
 			l++;
 		if (var->type < 0) {
			if (IS_INT_LNG_TYPE(t)) {
				s1++;
			} else {
				s2++;
			}
 			continue;
		}
		if (IS_INT_LNG_TYPE(t)) {                    /* integer args          */
 			if (s1 < INT_ARG_CNT) {                  /* register arguments    */
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
   					M_INTMOVE(rd->argintregs[s1], var->regoff);

				} else {                             /* reg arg -> spilled    */
   				    x86_64_mov_reg_membase(cd, rd->argintregs[s1], REG_SP, var->regoff * 8);
 				}

			} else {                                 /* stack arguments       */
 				pa = s1 - INT_ARG_CNT;
				if (s2 >= FLT_ARG_CNT) {
					pa += s2 - FLT_ARG_CNT;
				}
 				if (!(var->flags & INMEMORY)) {      /* stack arg -> register */
 					x86_64_mov_membase_reg(cd, REG_SP, (parentargs_base + pa) * 8 + 8, var->regoff);    /* + 8 for return address */
				} else {                             /* stack arg -> spilled  */
					/* x86_64_mov_membase_reg(cd, REG_SP, (parentargs_base + pa) * 8 + 8, REG_ITMP1); */    /* + 8 for return address */
/* 					x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, var->regoff * 8); */
					var->regoff = parentargs_base + pa + 1;
				}
			}
			s1++;

		} else {                                     /* floating args         */
 			if (s2 < FLT_ARG_CNT) {                  /* register arguments    */
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
					M_FLTMOVE(rd->argfltregs[s2], var->regoff);

 				} else {			                 /* reg arg -> spilled    */
					x86_64_movq_reg_membase(cd, rd->argfltregs[s2], REG_SP, var->regoff * 8);
 				}

 			} else {                                 /* stack arguments       */
 				pa = s2 - FLT_ARG_CNT;
				if (s1 >= INT_ARG_CNT) {
					pa += s1 - INT_ARG_CNT;
				}
 				if (!(var->flags & INMEMORY)) {      /* stack-arg -> register */
					x86_64_movq_membase_reg(cd, REG_SP, (parentargs_base + pa) * 8 + 8, var->regoff);

				} else {
/* 					x86_64_movq_membase_reg(cd, REG_SP, (parentargs_base + pa) * 8 + 8, REG_FTMP1); */
/* 					x86_64_movq_reg_membase(cd, REG_FTMP1, REG_SP, var->regoff * 8); */
					var->regoff = parentargs_base + pa + 1;
				}
			}
			s2++;
		}
	}  /* end for */

	/* save monitorenter argument */

#if defined(USE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		u8 func_enter;

		if (m->flags & ACC_STATIC) {
			func_enter = (u8) builtin_staticmonitorenter;
			x86_64_mov_imm_reg(cd, (s8) m->class, REG_ITMP1);
			x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, rd->maxmemuse * 8);

		} else {
			func_enter = (u8) builtin_monitorenter;
			x86_64_mov_reg_membase(cd, rd->argintregs[0], REG_SP, rd->maxmemuse * 8);
		}

		/* call monitorenter function */

		x86_64_mov_membase_reg(cd, REG_SP, rd->maxmemuse * 8, rd->argintregs[0]);
		x86_64_mov_imm_reg(cd, func_enter, REG_ITMP1);
		x86_64_call_reg(cd, REG_ITMP1);
	}
#endif

	/* Copy argument registers to stack and call trace function with pointer  */
	/* to arguments on stack.                                                 */

	if (runverbose) {
		x86_64_alu_imm_reg(cd, X86_64_SUB, (INT_ARG_CNT + FLT_ARG_CNT + 1 + 1) * 8, REG_SP);

		/* save integer argument registers */

		for (p = 0; p < INT_ARG_CNT; p++) {
			x86_64_mov_reg_membase(cd, rd->argintregs[p], REG_SP, (1 + p) * 8);
		}

		/* save float argument registers */

		for (p = 0; p < FLT_ARG_CNT; p++) {
			x86_64_movq_reg_membase(cd, rd->argfltregs[p], REG_SP, (1 + INT_ARG_CNT + p) * 8);
		}

		/* show integer hex code for float arguments */

		for (p = 0, l = 0; p < m->paramcount && p < INT_ARG_CNT; p++) {
			t = m->paramtypes[p];

			/* if the paramtype is a float, we have to right shift all        */
			/* following integer registers                                    */

			if (IS_FLT_DBL_TYPE(t)) {
				for (s1 = INT_ARG_CNT - 2; s1 >= p; s1--) {
					x86_64_mov_reg_reg(cd, rd->argintregs[s1], rd->argintregs[s1 + 1]);
				}

				x86_64_movd_freg_reg(cd, rd->argfltregs[l], rd->argintregs[p]);
				l++;
			}
		}

		x86_64_mov_imm_reg(cd, (u8) m, REG_ITMP2);
		x86_64_mov_reg_membase(cd, REG_ITMP2, REG_SP, 0 * 8);
		x86_64_mov_imm_reg(cd, (u8) builtin_trace_args, REG_ITMP1);
		x86_64_call_reg(cd, REG_ITMP1);

		/* restore integer argument registers */

		for (p = 0; p < INT_ARG_CNT; p++) {
			x86_64_mov_membase_reg(cd, REG_SP, (1 + p) * 8, rd->argintregs[p]);
		}

		/* restore float argument registers */

		for (p = 0; p < FLT_ARG_CNT; p++) {
			x86_64_movq_membase_reg(cd, REG_SP, (1 + INT_ARG_CNT + p) * 8, rd->argfltregs[p]);
		}

		x86_64_alu_imm_reg(cd, X86_64_ADD, (6 + 8 + 1 + 1) * 8, REG_SP);
	}

	}

	/* end of header generation */

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

		/* copy interface registers to their destination */

		src = bptr->instack;
		len = bptr->indepth;
		MCODECHECK(64 + len);

#ifdef LSRA
		if (opt_lsra) {
			while (src != NULL) {
				len--;
				if ((len == 0) && (bptr->type != BBTYPE_STD)) {
					if (bptr->type == BBTYPE_SBR) {
						/*  					d = reg_of_var(rd, src, REG_ITMP1); */
						if (!(src->flags & INMEMORY))
							d= src->regoff;
						else
							d=REG_ITMP1;
						x86_64_pop_reg(cd, d);
						store_reg_to_var_int(src, d);

					} else if (bptr->type == BBTYPE_EXH) {
						/*  					d = reg_of_var(rd, src, REG_ITMP1); */
						if (!(src->flags & INMEMORY))
							d= src->regoff;
						else
							d=REG_ITMP1;
						M_INTMOVE(REG_ITMP1, d);
						store_reg_to_var_int(src, d);
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
					d = reg_of_var(rd, src, REG_ITMP1);
					x86_64_pop_reg(cd, d);
					store_reg_to_var_int(src, d);

				} else if (bptr->type == BBTYPE_EXH) {
					d = reg_of_var(rd, src, REG_ITMP1);
					M_INTMOVE(REG_ITMP1, d);
					store_reg_to_var_int(src, d);
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
							x86_64_movq_membase_reg(cd, REG_SP, s1 * 8, d);
						}
						store_reg_to_var_flt(src, d);

					} else {
						s1 = rd->interfaces[len][s2].regoff;
						if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
							M_INTMOVE(s1, d);

						} else {
							x86_64_mov_membase_reg(cd, REG_SP, s1 * 8, d);
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
		currentline=0;
		for (iptr = bptr->iinstr; len > 0; src = iptr->dst, len--, iptr++) {
			if (iptr->line != currentline) {
				dseg_addlinenumber(cd, iptr->line, cd->mcodeptr);
				/*printf("%s : %d\n",m->name->text,iptr->line);*/
				currentline = iptr->line;
			}

			MCODECHECK(128);    /* XXX are 128 bytes enough? */

			switch (iptr->opc) {
			case ICMD_INLINE_START: /* internal ICMDs                         */
			case ICMD_INLINE_END:
				break;

			case ICMD_NOP:    /* ...  ==> ...                                 */
				break;

			case ICMD_NULLCHECKPOP: /* ..., objectref  ==> ...                */
				if (src->flags & INMEMORY) {
					x86_64_alu_imm_membase(cd, X86_64_CMP, 0, REG_SP, src->regoff * 8);

				} else {
					x86_64_test_reg_reg(cd, src->regoff, src->regoff);
				}
				x86_64_jcc(cd, X86_64_CC_E, 0);
				codegen_addxnullrefs(cd, cd->mcodeptr);
				break;

		/* constant operations ************************************************/

		case ICMD_ICONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.i = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if (iptr->val.i == 0) {
				x86_64_alu_reg_reg(cd, X86_64_XOR, d, d);
			} else {
				x86_64_movl_imm_reg(cd, iptr->val.i, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ACONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.a = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if (iptr->val.a == 0) {
				x86_64_alu_reg_reg(cd, X86_64_XOR, d, d);
			} else {
				x86_64_mov_imm_reg(cd, (s8) iptr->val.a, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.l = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if (iptr->val.l == 0) {
				x86_64_alu_reg_reg(cd, X86_64_XOR, d, d);
			} else {
				x86_64_mov_imm_reg(cd, iptr->val.l, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.f = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			a = dseg_addfloat(cd, iptr->val.f);
			x86_64_movdl_membase_reg(cd, RIP, -(((s8) cd->mcodeptr + ((d > 7) ? 9 : 8)) - (s8) cd->mcodebase) + a, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
		
		case ICMD_DCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.d = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			a = dseg_adddouble(cd, iptr->val.d);
			x86_64_movd_membase_reg(cd, RIP, -(((s8) cd->mcodeptr + 9) - (s8) cd->mcodebase) + a, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;


		/* load/store operations **********************************************/

		case ICMD_ILOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1)) {
				break;
			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY) {
				x86_64_movl_membase_reg(cd, REG_SP, var->regoff * 8, d);
				store_reg_to_var_int(iptr->dst, d);

			} else {
				if (iptr->dst->flags & INMEMORY) {
					x86_64_mov_reg_membase(cd, var->regoff, REG_SP, iptr->dst->regoff * 8);

				} else {
					M_INTMOVE(var->regoff, d);
				}
			}
			break;

		case ICMD_LLOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_ALOAD:      /* op1 = local variable                         */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1)) {
				break;
			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY) {
				x86_64_mov_membase_reg(cd, REG_SP, var->regoff * 8, d);
				store_reg_to_var_int(iptr->dst, d);

			} else {
				if (iptr->dst->flags & INMEMORY) {
					x86_64_mov_reg_membase(cd, var->regoff, REG_SP, iptr->dst->regoff * 8);

				} else {
					M_INTMOVE(var->regoff, d);
				}
			}
			break;

		case ICMD_FLOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_DLOAD:      /* op1 = local variable                         */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
  			if ((iptr->dst->varkind == LOCALVAR) &&
  			    (iptr->dst->varnum == iptr->op1)) {
    				break;
  			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY) {
				x86_64_movq_membase_reg(cd, REG_SP, var->regoff * 8, d);
				store_reg_to_var_flt(iptr->dst, d);

			} else {
				if (iptr->dst->flags & INMEMORY) {
					x86_64_movq_reg_membase(cd, var->regoff, REG_SP, iptr->dst->regoff * 8);

				} else {
					M_FLTMOVE(var->regoff, d);
				}
			}
			break;

		case ICMD_ISTORE:     /* ..., value  ==> ...                          */
		case ICMD_LSTORE:     /* op1 = local variable                         */
		case ICMD_ASTORE:

			if ((src->varkind == LOCALVAR) &&
			    (src->varnum == iptr->op1)) {
				break;
			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				var_to_reg_int(s1, src, REG_ITMP1);
				x86_64_mov_reg_membase(cd, s1, REG_SP, var->regoff * 8);

			} else {
				var_to_reg_int(s1, src, var->regoff);
				M_INTMOVE(s1, var->regoff);
			}
			break;

		case ICMD_FSTORE:     /* ..., value  ==> ...                          */
		case ICMD_DSTORE:     /* op1 = local variable                         */

			if ((src->varkind == LOCALVAR) &&
			    (src->varnum == iptr->op1)) {
				break;
			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				var_to_reg_flt(s1, src, REG_FTMP1);
				x86_64_movq_reg_membase(cd, s1, REG_SP, var->regoff * 8);

			} else {
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

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						x86_64_negl_membase(cd, REG_SP, iptr->dst->regoff * 8);

					} else {
						x86_64_movl_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
						x86_64_negl_reg(cd, REG_ITMP1);
						x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					x86_64_movl_reg_membase(cd, src->regoff, REG_SP, iptr->dst->regoff * 8);
					x86_64_negl_membase(cd, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					x86_64_movl_membase_reg(cd, REG_SP, src->regoff * 8, iptr->dst->regoff);
					x86_64_negl_reg(cd, d);

				} else {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					x86_64_negl_reg(cd, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_LNEG:       /* ..., value  ==> ..., - value                 */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						x86_64_neg_membase(cd, REG_SP, iptr->dst->regoff * 8);

					} else {
						x86_64_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
						x86_64_neg_reg(cd, REG_ITMP1);
						x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					x86_64_mov_reg_membase(cd, src->regoff, REG_SP, iptr->dst->regoff * 8);
					x86_64_neg_membase(cd, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					x86_64_mov_membase_reg(cd, REG_SP, src->regoff * 8, iptr->dst->regoff);
					x86_64_neg_reg(cd, iptr->dst->regoff);

				} else {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					x86_64_neg_reg(cd, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_I2L:        /* ..., value  ==> ..., value                   */

			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (src->flags & INMEMORY) {
				x86_64_movslq_membase_reg(cd, REG_SP, src->regoff * 8, d);

			} else {
				x86_64_movslq_reg_reg(cd, src->regoff, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (src->flags & INMEMORY) {
				x86_64_movsbq_membase_reg(cd, REG_SP, src->regoff * 8, d);

			} else {
				x86_64_movsbq_reg_reg(cd, src->regoff, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2CHAR:   /* ..., value  ==> ..., value                   */

			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (src->flags & INMEMORY) {
				x86_64_movzwq_membase_reg(cd, REG_SP, src->regoff * 8, d);

			} else {
				x86_64_movzwq_reg_reg(cd, src->regoff, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2SHORT:  /* ..., value  ==> ..., value                   */

			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (src->flags & INMEMORY) {
				x86_64_movswq_membase_reg(cd, REG_SP, src->regoff * 8, d);

			} else {
				x86_64_movswq_reg_reg(cd, src->regoff, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_IADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ialu(cd, X86_64_ADD, src, iptr);
			break;

		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ialuconst(cd, X86_64_ADD, src, iptr);
			break;

		case ICMD_LADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_lalu(cd, X86_64_ADD, src, iptr);
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_laluconst(cd, X86_64_ADD, src, iptr);
			break;

		case ICMD_ISUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						x86_64_movl_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
						x86_64_alul_reg_membase(cd, X86_64_SUB, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

					} else {
						x86_64_movl_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
						x86_64_alul_membase_reg(cd, X86_64_SUB, REG_SP, src->regoff * 8, REG_ITMP1);
						x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, REG_ITMP1);
					x86_64_alul_membase_reg(cd, X86_64_SUB, REG_SP, src->regoff * 8, REG_ITMP1);
					x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						x86_64_alul_reg_membase(cd, X86_64_SUB, src->regoff, REG_SP, iptr->dst->regoff * 8);

					} else {
						x86_64_movl_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
						x86_64_alul_reg_reg(cd, X86_64_SUB, src->regoff, REG_ITMP1);
						x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					x86_64_movl_reg_membase(cd, src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
					x86_64_alul_reg_membase(cd, X86_64_SUB, src->regoff, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_movl_membase_reg(cd, REG_SP, src->prev->regoff * 8, d);
					x86_64_alul_membase_reg(cd, X86_64_SUB, REG_SP, src->regoff * 8, d);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, d);
					x86_64_alul_membase_reg(cd, X86_64_SUB, REG_SP, src->regoff * 8, d);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					/* workaround for reg alloc */
					if (src->regoff == iptr->dst->regoff) {
						x86_64_movl_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
						x86_64_alul_reg_reg(cd, X86_64_SUB, src->regoff, REG_ITMP1);
						M_INTMOVE(REG_ITMP1, d);

					} else {
						x86_64_movl_membase_reg(cd, REG_SP, src->prev->regoff * 8, d);
						x86_64_alul_reg_reg(cd, X86_64_SUB, src->regoff, d);
					}

				} else {
					/* workaround for reg alloc */
					if (src->regoff == iptr->dst->regoff) {
						M_INTMOVE(src->prev->regoff, REG_ITMP1);
						x86_64_alul_reg_reg(cd, X86_64_SUB, src->regoff, REG_ITMP1);
						M_INTMOVE(REG_ITMP1, d);

					} else {
						M_INTMOVE(src->prev->regoff, d);
						x86_64_alul_reg_reg(cd, X86_64_SUB, src->regoff, d);
					}
				}
			}
			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ialuconst(cd, X86_64_SUB, src, iptr);
			break;

		case ICMD_LSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						x86_64_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
						x86_64_alu_reg_membase(cd, X86_64_SUB, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

					} else {
						x86_64_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
						x86_64_alu_membase_reg(cd, X86_64_SUB, REG_SP, src->regoff * 8, REG_ITMP1);
						x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, REG_ITMP1);
					x86_64_alu_membase_reg(cd, X86_64_SUB, REG_SP, src->regoff * 8, REG_ITMP1);
					x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						x86_64_alu_reg_membase(cd, X86_64_SUB, src->regoff, REG_SP, iptr->dst->regoff * 8);

					} else {
						x86_64_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
						x86_64_alu_reg_reg(cd, X86_64_SUB, src->regoff, REG_ITMP1);
						x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					x86_64_mov_reg_membase(cd, src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
					x86_64_alu_reg_membase(cd, X86_64_SUB, src->regoff, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, d);
					x86_64_alu_membase_reg(cd, X86_64_SUB, REG_SP, src->regoff * 8, d);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, d);
					x86_64_alu_membase_reg(cd, X86_64_SUB, REG_SP, src->regoff * 8, d);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					/* workaround for reg alloc */
					if (src->regoff == iptr->dst->regoff) {
						x86_64_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
						x86_64_alu_reg_reg(cd, X86_64_SUB, src->regoff, REG_ITMP1);
						M_INTMOVE(REG_ITMP1, d);

					} else {
						x86_64_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, d);
						x86_64_alu_reg_reg(cd, X86_64_SUB, src->regoff, d);
					}

				} else {
					/* workaround for reg alloc */
					if (src->regoff == iptr->dst->regoff) {
						M_INTMOVE(src->prev->regoff, REG_ITMP1);
						x86_64_alu_reg_reg(cd, X86_64_SUB, src->regoff, REG_ITMP1);
						M_INTMOVE(REG_ITMP1, d);

					} else {
						M_INTMOVE(src->prev->regoff, d);
						x86_64_alu_reg_reg(cd, X86_64_SUB, src->regoff, d);
					}
				}
			}
			break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_laluconst(cd, X86_64_SUB, src, iptr);
			break;

		case ICMD_IMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_movl_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
					x86_64_imull_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
					x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					x86_64_movl_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
					x86_64_imull_reg_reg(cd, src->prev->regoff, REG_ITMP1);
					x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_movl_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
					x86_64_imull_reg_reg(cd, src->regoff, REG_ITMP1);
					x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else {
					M_INTMOVE(src->prev->regoff, REG_ITMP1);
					x86_64_imull_reg_reg(cd, src->regoff, REG_ITMP1);
					x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_movl_membase_reg(cd, REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
					x86_64_imull_membase_reg(cd, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
					x86_64_imull_membase_reg(cd, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					x86_64_imull_membase_reg(cd, REG_SP, src->prev->regoff * 8, iptr->dst->regoff);

				} else {
					if (src->regoff == iptr->dst->regoff) {
						x86_64_imull_reg_reg(cd, src->prev->regoff, iptr->dst->regoff);

					} else {
						M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
						x86_64_imull_reg_reg(cd, src->regoff, iptr->dst->regoff);
					}
				}
			}
			break;

		case ICMD_IMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					x86_64_imull_imm_membase_reg(cd, iptr->val.i, REG_SP, src->regoff * 8, REG_ITMP1);
					x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else {
					x86_64_imull_imm_reg_reg(cd, iptr->val.i, src->regoff, REG_ITMP1);
					x86_64_movl_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					x86_64_imull_imm_membase_reg(cd, iptr->val.i, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else {
					if (iptr->val.i == 2) {
						M_INTMOVE(src->regoff, iptr->dst->regoff);
						x86_64_alul_reg_reg(cd, X86_64_ADD, iptr->dst->regoff, iptr->dst->regoff);

					} else {
						x86_64_imull_imm_reg_reg(cd, iptr->val.i, src->regoff, iptr->dst->regoff);    /* 3 cycles */
					}
				}
			}
			break;

		case ICMD_LMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
					x86_64_imul_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
					x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					x86_64_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
					x86_64_imul_reg_reg(cd, src->prev->regoff, REG_ITMP1);
					x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
					x86_64_imul_reg_reg(cd, src->regoff, REG_ITMP1);
					x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else {
					x86_64_mov_reg_reg(cd, src->prev->regoff, REG_ITMP1);
					x86_64_imul_reg_reg(cd, src->regoff, REG_ITMP1);
					x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
					x86_64_imul_membase_reg(cd, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
					x86_64_imul_membase_reg(cd, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					x86_64_imul_membase_reg(cd, REG_SP, src->prev->regoff * 8, iptr->dst->regoff);

				} else {
					if (src->regoff == iptr->dst->regoff) {
						x86_64_imul_reg_reg(cd, src->prev->regoff, iptr->dst->regoff);

					} else {
						M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
						x86_64_imul_reg_reg(cd, src->regoff, iptr->dst->regoff);
					}
				}
			}
			break;

		case ICMD_LMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (IS_IMM32(iptr->val.l)) {
						x86_64_imul_imm_membase_reg(cd, iptr->val.l, REG_SP, src->regoff * 8, REG_ITMP1);

					} else {
						x86_64_mov_imm_reg(cd, iptr->val.l, REG_ITMP1);
						x86_64_imul_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
					}
					x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					
				} else {
					if (IS_IMM32(iptr->val.l)) {
						x86_64_imul_imm_reg_reg(cd, iptr->val.l, src->regoff, REG_ITMP1);

					} else {
						x86_64_mov_imm_reg(cd, iptr->val.l, REG_ITMP1);
						x86_64_imul_reg_reg(cd, src->regoff, REG_ITMP1);
					}
					x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					if (IS_IMM32(iptr->val.l)) {
						x86_64_imul_imm_membase_reg(cd, iptr->val.l, REG_SP, src->regoff * 8, iptr->dst->regoff);

					} else {
						x86_64_mov_imm_reg(cd, iptr->val.l, iptr->dst->regoff);
						x86_64_imul_membase_reg(cd, REG_SP, src->regoff * 8, iptr->dst->regoff);
					}

				} else {
					/* should match in many cases */
					if (iptr->val.l == 2) {
						M_INTMOVE(src->regoff, iptr->dst->regoff);
						x86_64_alul_reg_reg(cd, X86_64_ADD, iptr->dst->regoff, iptr->dst->regoff);

					} else {
						if (IS_IMM32(iptr->val.l)) {
							x86_64_imul_imm_reg_reg(cd, iptr->val.l, src->regoff, iptr->dst->regoff);    /* 4 cycles */

						} else {
							x86_64_mov_imm_reg(cd, iptr->val.l, REG_ITMP1);
							M_INTMOVE(src->regoff, iptr->dst->regoff);
							x86_64_imul_reg_reg(cd, REG_ITMP1, iptr->dst->regoff);
						}
					}
				}
			}
			break;

		case ICMD_IDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
	        if (src->prev->flags & INMEMORY) {
				x86_64_movl_membase_reg(cd, REG_SP, src->prev->regoff * 8, RAX);

			} else {
				M_INTMOVE(src->prev->regoff, RAX);
			}
			
			if (src->flags & INMEMORY) {
				x86_64_movl_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP3);

			} else {
				M_INTMOVE(src->regoff, REG_ITMP3);
			}
			gen_div_check(src);

			x86_64_alul_imm_reg(cd, X86_64_CMP, 0x80000000, RAX);    /* check as described in jvm spec */
			x86_64_jcc(cd, X86_64_CC_NE, 4 + 6);
			x86_64_alul_imm_reg(cd, X86_64_CMP, -1, REG_ITMP3);      /* 4 bytes */
			x86_64_jcc(cd, X86_64_CC_E, 3 + 1 + 3);                  /* 6 bytes */

			x86_64_mov_reg_reg(cd, RDX, REG_ITMP2);    /* save %rdx, cause it's an argument register */
  			x86_64_cltd(cd);
			x86_64_idivl_reg(cd, REG_ITMP3);

			if (iptr->dst->flags & INMEMORY) {
				x86_64_mov_reg_membase(cd, RAX, REG_SP, iptr->dst->regoff * 8);
				x86_64_mov_reg_reg(cd, REG_ITMP2, RDX);    /* restore %rdx */

			} else {
				M_INTMOVE(RAX, iptr->dst->regoff);

				if (iptr->dst->regoff != RDX) {
					x86_64_mov_reg_reg(cd, REG_ITMP2, RDX);    /* restore %rdx */
				}
			}
			break;

		case ICMD_IREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (src->prev->flags & INMEMORY) {
				x86_64_movl_membase_reg(cd, REG_SP, src->prev->regoff * 8, RAX);

			} else {
				M_INTMOVE(src->prev->regoff, RAX);
			}
			
			if (src->flags & INMEMORY) {
				x86_64_movl_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP3);

			} else {
				M_INTMOVE(src->regoff, REG_ITMP3);
			}
			gen_div_check(src);

			x86_64_mov_reg_reg(cd, RDX, REG_ITMP2);    /* save %rdx, cause it's an argument register */

			x86_64_alul_imm_reg(cd, X86_64_CMP, 0x80000000, RAX);    /* check as described in jvm spec */
			x86_64_jcc(cd, X86_64_CC_NE, 2 + 4 + 6);


			x86_64_alul_reg_reg(cd, X86_64_XOR, RDX, RDX);           /* 2 bytes */
			x86_64_alul_imm_reg(cd, X86_64_CMP, -1, REG_ITMP3);      /* 4 bytes */
			x86_64_jcc(cd, X86_64_CC_E, 1 + 3);                      /* 6 bytes */

  			x86_64_cltd(cd);
			x86_64_idivl_reg(cd, REG_ITMP3);

			if (iptr->dst->flags & INMEMORY) {
				x86_64_mov_reg_membase(cd, RDX, REG_SP, iptr->dst->regoff * 8);
				x86_64_mov_reg_reg(cd, REG_ITMP2, RDX);    /* restore %rdx */

			} else {
				M_INTMOVE(RDX, iptr->dst->regoff);

				if (iptr->dst->regoff != RDX) {
					x86_64_mov_reg_reg(cd, REG_ITMP2, RDX);    /* restore %rdx */
				}
			}
			break;

		case ICMD_IDIVPOW2:   /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, REG_ITMP1);
			x86_64_alul_imm_reg(cd, X86_64_CMP, -1, REG_ITMP1);
			x86_64_leal_membase_reg(cd, REG_ITMP1, (1 << iptr->val.i) - 1, REG_ITMP2);
			x86_64_cmovccl_reg_reg(cd, X86_64_CC_LE, REG_ITMP2, REG_ITMP1);
			x86_64_shiftl_imm_reg(cd, X86_64_SAR, iptr->val.i, REG_ITMP1);
			x86_64_mov_reg_reg(cd, REG_ITMP1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, REG_ITMP1);
			x86_64_alul_imm_reg(cd, X86_64_CMP, -1, REG_ITMP1);
			x86_64_leal_membase_reg(cd, REG_ITMP1, iptr->val.i, REG_ITMP2);
			x86_64_cmovccl_reg_reg(cd, X86_64_CC_G, REG_ITMP1, REG_ITMP2);
			x86_64_alul_imm_reg(cd, X86_64_AND, -1 - (iptr->val.i), REG_ITMP2);
			x86_64_alul_reg_reg(cd, X86_64_SUB, REG_ITMP2, REG_ITMP1);
			x86_64_mov_reg_reg(cd, REG_ITMP1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_LDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
	        if (src->prev->flags & INMEMORY) {
				x86_64_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);

			} else {
				M_INTMOVE(src->prev->regoff, REG_ITMP1);
			}
			
			if (src->flags & INMEMORY) {
				x86_64_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP3);

			} else {
				M_INTMOVE(src->regoff, REG_ITMP3);
			}
			gen_div_check(src);

			x86_64_mov_imm_reg(cd, 0x8000000000000000LL, REG_ITMP2);    /* check as described in jvm spec */
			x86_64_alu_reg_reg(cd, X86_64_CMP, REG_ITMP2, REG_ITMP1);
			x86_64_jcc(cd, X86_64_CC_NE, 4 + 6);
			x86_64_alu_imm_reg(cd, X86_64_CMP, -1, REG_ITMP3);          /* 4 bytes */
			x86_64_jcc(cd, X86_64_CC_E, 3 + 2 + 3);                     /* 6 bytes */

			x86_64_mov_reg_reg(cd, RDX, REG_ITMP2);    /* save %rdx, cause it's an argument register */
  			x86_64_cqto(cd);
			x86_64_idiv_reg(cd, REG_ITMP3);

			if (iptr->dst->flags & INMEMORY) {
				x86_64_mov_reg_membase(cd, RAX, REG_SP, iptr->dst->regoff * 8);
				x86_64_mov_reg_reg(cd, REG_ITMP2, RDX);    /* restore %rdx */

			} else {
				M_INTMOVE(RAX, iptr->dst->regoff);

				if (iptr->dst->regoff != RDX) {
					x86_64_mov_reg_reg(cd, REG_ITMP2, RDX);    /* restore %rdx */
				}
			}
			break;

		case ICMD_LREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (src->prev->flags & INMEMORY) {
				x86_64_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);

			} else {
				M_INTMOVE(src->prev->regoff, REG_ITMP1);
			}
			
			if (src->flags & INMEMORY) {
				x86_64_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP3);

			} else {
				M_INTMOVE(src->regoff, REG_ITMP3);
			}
			gen_div_check(src);

			x86_64_mov_reg_reg(cd, RDX, REG_ITMP2);    /* save %rdx, cause it's an argument register */

			x86_64_mov_imm_reg(cd, 0x8000000000000000LL, REG_ITMP2);    /* check as described in jvm spec */
			x86_64_alu_reg_reg(cd, X86_64_CMP, REG_ITMP2, REG_ITMP1);
			x86_64_jcc(cd, X86_64_CC_NE, 2 + 4 + 6);


			x86_64_alul_reg_reg(cd, X86_64_XOR, RDX, RDX);              /* 2 bytes */
			x86_64_alu_imm_reg(cd, X86_64_CMP, -1, REG_ITMP3);          /* 4 bytes */
			x86_64_jcc(cd, X86_64_CC_E, 2 + 3);                         /* 6 bytes */

  			x86_64_cqto(cd);
			x86_64_idiv_reg(cd, REG_ITMP3);

			if (iptr->dst->flags & INMEMORY) {
				x86_64_mov_reg_membase(cd, RDX, REG_SP, iptr->dst->regoff * 8);
				x86_64_mov_reg_reg(cd, REG_ITMP2, RDX);    /* restore %rdx */

			} else {
				M_INTMOVE(RDX, iptr->dst->regoff);

				if (iptr->dst->regoff != RDX) {
					x86_64_mov_reg_reg(cd, REG_ITMP2, RDX);    /* restore %rdx */
				}
			}
			break;

		case ICMD_LDIVPOW2:   /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, REG_ITMP1);
			x86_64_alu_imm_reg(cd, X86_64_CMP, -1, REG_ITMP1);
			x86_64_lea_membase_reg(cd, REG_ITMP1, (1 << iptr->val.i) - 1, REG_ITMP2);
			x86_64_cmovcc_reg_reg(cd, X86_64_CC_LE, REG_ITMP2, REG_ITMP1);
			x86_64_shift_imm_reg(cd, X86_64_SAR, iptr->val.i, REG_ITMP1);
			x86_64_mov_reg_reg(cd, REG_ITMP1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, REG_ITMP1);
			x86_64_alu_imm_reg(cd, X86_64_CMP, -1, REG_ITMP1);
			x86_64_lea_membase_reg(cd, REG_ITMP1, iptr->val.i, REG_ITMP2);
			x86_64_cmovcc_reg_reg(cd, X86_64_CC_G, REG_ITMP1, REG_ITMP2);
			x86_64_alu_imm_reg(cd, X86_64_AND, -1 - (iptr->val.i), REG_ITMP2);
			x86_64_alu_reg_reg(cd, X86_64_SUB, REG_ITMP2, REG_ITMP1);
			x86_64_mov_reg_reg(cd, REG_ITMP1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ishift(cd, X86_64_SHL, src, iptr);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ishiftconst(cd, X86_64_SHL, src, iptr);
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ishift(cd, X86_64_SAR, src, iptr);
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ishiftconst(cd, X86_64_SAR, src, iptr);
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ishift(cd, X86_64_SHR, src, iptr);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ishiftconst(cd, X86_64_SHR, src, iptr);
			break;

		case ICMD_LSHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_lshift(cd, X86_64_SHL, src, iptr);
			break;

        case ICMD_LSHLCONST:  /* ..., value  ==> ..., value << constant       */
 			                  /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_lshiftconst(cd, X86_64_SHL, src, iptr);
			break;

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_lshift(cd, X86_64_SAR, src, iptr);
			break;

		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_lshiftconst(cd, X86_64_SAR, src, iptr);
			break;

		case ICMD_LUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_lshift(cd, X86_64_SHR, src, iptr);
			break;

  		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
  		                      /* val.l = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_lshiftconst(cd, X86_64_SHR, src, iptr);
			break;

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ialu(cd, X86_64_AND, src, iptr);
			break;

		case ICMD_IANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ialuconst(cd, X86_64_AND, src, iptr);
			break;

		case ICMD_LAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_lalu(cd, X86_64_AND, src, iptr);
			break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_laluconst(cd, X86_64_AND, src, iptr);
			break;

		case ICMD_IOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ialu(cd, X86_64_OR, src, iptr);
			break;

		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ialuconst(cd, X86_64_OR, src, iptr);
			break;

		case ICMD_LOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_lalu(cd, X86_64_OR, src, iptr);
			break;

		case ICMD_LORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_laluconst(cd, X86_64_OR, src, iptr);
			break;

		case ICMD_IXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ialu(cd, X86_64_XOR, src, iptr);
			break;

		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_ialuconst(cd, X86_64_XOR, src, iptr);
			break;

		case ICMD_LXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_lalu(cd, X86_64_XOR, src, iptr);
			break;

		case ICMD_LXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			x86_64_emit_laluconst(cd, X86_64_XOR, src, iptr);
			break;


		case ICMD_IINC:       /* ..., value  ==> ..., value + constant        */
		                      /* op1 = variable, val.i = constant             */

			/* using inc and dec is definitely faster than add -- tested      */
			/* with sieve                                                     */

			var = &(rd->locals[iptr->op1][TYPE_INT]);
			d = var->regoff;
			if (var->flags & INMEMORY) {
				if (iptr->val.i == 1) {
					x86_64_incl_membase(cd, REG_SP, d * 8);
 
				} else if (iptr->val.i == -1) {
					x86_64_decl_membase(cd, REG_SP, d * 8);

				} else {
					x86_64_alul_imm_membase(cd, X86_64_ADD, iptr->val.i, REG_SP, d * 8);
				}

			} else {
				if (iptr->val.i == 1) {
					x86_64_incl_reg(cd, d);
 
				} else if (iptr->val.i == -1) {
					x86_64_decl_reg(cd, d);

				} else {
					x86_64_alul_imm_reg(cd, X86_64_ADD, iptr->val.i, d);
				}
			}
			break;


		/* floating operations ************************************************/

		case ICMD_FNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			a = dseg_adds4(cd, 0x80000000);
			M_FLTMOVE(s1, d);
			x86_64_movss_membase_reg(cd, RIP, -(((s8) cd->mcodeptr + 9) - (s8) cd->mcodebase) + a, REG_FTMP2);
			x86_64_xorps_reg_reg(cd, REG_FTMP2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			a = dseg_adds8(cd, 0x8000000000000000);
			M_FLTMOVE(s1, d);
			x86_64_movd_membase_reg(cd, RIP, -(((s8) cd->mcodeptr + 9) - (s8) cd->mcodebase) + a, REG_FTMP2);
			x86_64_xorpd_reg_reg(cd, REG_FTMP2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (s1 == d) {
				x86_64_addss_reg_reg(cd, s2, d);
			} else if (s2 == d) {
				x86_64_addss_reg_reg(cd, s1, d);
			} else {
				M_FLTMOVE(s1, d);
				x86_64_addss_reg_reg(cd, s2, d);
			}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (s1 == d) {
				x86_64_addsd_reg_reg(cd, s2, d);
			} else if (s2 == d) {
				x86_64_addsd_reg_reg(cd, s1, d);
			} else {
				M_FLTMOVE(s1, d);
				x86_64_addsd_reg_reg(cd, s2, d);
			}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (s2 == d) {
				M_FLTMOVE(s2, REG_FTMP2);
				s2 = REG_FTMP2;
			}
			M_FLTMOVE(s1, d);
			x86_64_subss_reg_reg(cd, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (s2 == d) {
				M_FLTMOVE(s2, REG_FTMP2);
				s2 = REG_FTMP2;
			}
			M_FLTMOVE(s1, d);
			x86_64_subsd_reg_reg(cd, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (s1 == d) {
				x86_64_mulss_reg_reg(cd, s2, d);
			} else if (s2 == d) {
				x86_64_mulss_reg_reg(cd, s1, d);
			} else {
				M_FLTMOVE(s1, d);
				x86_64_mulss_reg_reg(cd, s2, d);
			}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (s1 == d) {
				x86_64_mulsd_reg_reg(cd, s2, d);
			} else if (s2 == d) {
				x86_64_mulsd_reg_reg(cd, s1, d);
			} else {
				M_FLTMOVE(s1, d);
				x86_64_mulsd_reg_reg(cd, s2, d);
			}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (s2 == d) {
				M_FLTMOVE(s2, REG_FTMP2);
				s2 = REG_FTMP2;
			}
			M_FLTMOVE(s1, d);
			x86_64_divss_reg_reg(cd, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (s2 == d) {
				M_FLTMOVE(s2, REG_FTMP2);
				s2 = REG_FTMP2;
			}
			M_FLTMOVE(s1, d);
			x86_64_divsd_reg_reg(cd, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_I2F:       /* ..., value  ==> ..., (float) value            */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			x86_64_cvtsi2ss_reg_reg(cd, s1, d);
  			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_I2D:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			x86_64_cvtsi2sd_reg_reg(cd, s1, d);
  			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_L2F:       /* ..., value  ==> ..., (float) value            */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			x86_64_cvtsi2ssq_reg_reg(cd, s1, d);
  			store_reg_to_var_flt(iptr->dst, d);
			break;
			
		case ICMD_L2D:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			x86_64_cvtsi2sdq_reg_reg(cd, s1, d);
  			store_reg_to_var_flt(iptr->dst, d);
			break;
			
		case ICMD_F2I:       /* ..., value  ==> ..., (int) value              */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			x86_64_cvttss2si_reg_reg(cd, s1, d);
			x86_64_alul_imm_reg(cd, X86_64_CMP, 0x80000000, d);    /* corner cases */
			a = ((s1 == REG_FTMP1) ? 0 : 5) + 10 + 3 + ((REG_RESULT == d) ? 0 : 3);
			x86_64_jcc(cd, X86_64_CC_NE, a);
			M_FLTMOVE(s1, REG_FTMP1);
			x86_64_mov_imm_reg(cd, (s8) asm_builtin_f2i, REG_ITMP2);
			x86_64_call_reg(cd, REG_ITMP2);
			M_INTMOVE(REG_RESULT, d);
  			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_D2I:       /* ..., value  ==> ..., (int) value              */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			x86_64_cvttsd2si_reg_reg(cd, s1, d);
			x86_64_alul_imm_reg(cd, X86_64_CMP, 0x80000000, d);    /* corner cases */
			a = ((s1 == REG_FTMP1) ? 0 : 5) + 10 + 3 + ((REG_RESULT == d) ? 0 : 3);
			x86_64_jcc(cd, X86_64_CC_NE, a);
			M_FLTMOVE(s1, REG_FTMP1);
			x86_64_mov_imm_reg(cd, (s8) asm_builtin_d2i, REG_ITMP2);
			x86_64_call_reg(cd, REG_ITMP2);
			M_INTMOVE(REG_RESULT, d);
  			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_F2L:       /* ..., value  ==> ..., (long) value             */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			x86_64_cvttss2siq_reg_reg(cd, s1, d);
			x86_64_mov_imm_reg(cd, 0x8000000000000000, REG_ITMP2);
			x86_64_alu_reg_reg(cd, X86_64_CMP, REG_ITMP2, d);     /* corner cases */
			a = ((s1 == REG_FTMP1) ? 0 : 5) + 10 + 3 + ((REG_RESULT == d) ? 0 : 3);
			x86_64_jcc(cd, X86_64_CC_NE, a);
			M_FLTMOVE(s1, REG_FTMP1);
			x86_64_mov_imm_reg(cd, (s8) asm_builtin_f2l, REG_ITMP2);
			x86_64_call_reg(cd, REG_ITMP2);
			M_INTMOVE(REG_RESULT, d);
  			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_D2L:       /* ..., value  ==> ..., (long) value             */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			x86_64_cvttsd2siq_reg_reg(cd, s1, d);
			x86_64_mov_imm_reg(cd, 0x8000000000000000, REG_ITMP2);
			x86_64_alu_reg_reg(cd, X86_64_CMP, REG_ITMP2, d);     /* corner cases */
			a = ((s1 == REG_FTMP1) ? 0 : 5) + 10 + 3 + ((REG_RESULT == d) ? 0 : 3);
			x86_64_jcc(cd, X86_64_CC_NE, a);
			M_FLTMOVE(s1, REG_FTMP1);
			x86_64_mov_imm_reg(cd, (s8) asm_builtin_d2l, REG_ITMP2);
			x86_64_call_reg(cd, REG_ITMP2);
			M_INTMOVE(REG_RESULT, d);
  			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_F2D:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			x86_64_cvtss2sd_reg_reg(cd, s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_D2F:       /* ..., value  ==> ..., (float) value            */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			x86_64_cvtsd2ss_reg_reg(cd, s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */
 			                  /* == => 0, < => 1, > => -1 */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			x86_64_alu_reg_reg(cd, X86_64_XOR, d, d);
			x86_64_mov_imm_reg(cd, 1, REG_ITMP1);
			x86_64_mov_imm_reg(cd, -1, REG_ITMP2);
			x86_64_ucomiss_reg_reg(cd, s1, s2);
			x86_64_cmovcc_reg_reg(cd, X86_64_CC_B, REG_ITMP1, d);
			x86_64_cmovcc_reg_reg(cd, X86_64_CC_A, REG_ITMP2, d);
			x86_64_cmovcc_reg_reg(cd, X86_64_CC_P, REG_ITMP2, d);    /* treat unordered as GT */
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */
 			                  /* == => 0, < => 1, > => -1 */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			x86_64_alu_reg_reg(cd, X86_64_XOR, d, d);
			x86_64_mov_imm_reg(cd, 1, REG_ITMP1);
			x86_64_mov_imm_reg(cd, -1, REG_ITMP2);
			x86_64_ucomiss_reg_reg(cd, s1, s2);
			x86_64_cmovcc_reg_reg(cd, X86_64_CC_B, REG_ITMP1, d);
			x86_64_cmovcc_reg_reg(cd, X86_64_CC_A, REG_ITMP2, d);
			x86_64_cmovcc_reg_reg(cd, X86_64_CC_P, REG_ITMP1, d);    /* treat unordered as LT */
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_DCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */
 			                  /* == => 0, < => 1, > => -1 */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			x86_64_alu_reg_reg(cd, X86_64_XOR, d, d);
			x86_64_mov_imm_reg(cd, 1, REG_ITMP1);
			x86_64_mov_imm_reg(cd, -1, REG_ITMP2);
			x86_64_ucomisd_reg_reg(cd, s1, s2);
			x86_64_cmovcc_reg_reg(cd, X86_64_CC_B, REG_ITMP1, d);
			x86_64_cmovcc_reg_reg(cd, X86_64_CC_A, REG_ITMP2, d);
			x86_64_cmovcc_reg_reg(cd, X86_64_CC_P, REG_ITMP2, d);    /* treat unordered as GT */
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_DCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */
 			                  /* == => 0, < => 1, > => -1 */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			x86_64_alu_reg_reg(cd, X86_64_XOR, d, d);
			x86_64_mov_imm_reg(cd, 1, REG_ITMP1);
			x86_64_mov_imm_reg(cd, -1, REG_ITMP2);
			x86_64_ucomisd_reg_reg(cd, s1, s2);
			x86_64_cmovcc_reg_reg(cd, X86_64_CC_B, REG_ITMP1, d);
			x86_64_cmovcc_reg_reg(cd, X86_64_CC_A, REG_ITMP2, d);
			x86_64_cmovcc_reg_reg(cd, X86_64_CC_P, REG_ITMP1, d);    /* treat unordered as LT */
			store_reg_to_var_int(iptr->dst, d);
			break;


		/* memory operations **************************************************/

		case ICMD_ARRAYLENGTH: /* ..., arrayref  ==> ..., (int) length        */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			gen_nullptr_check(s1);
			x86_64_movl_membase_reg(cd, s1, OFFSET(java_arrayheader, size), d);
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
			x86_64_mov_memindex_reg(cd, OFFSET(java_objectarray, data[0]), s1, s2, 3, d);
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
			x86_64_mov_memindex_reg(cd, OFFSET(java_longarray, data[0]), s1, s2, 3, d);
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
			x86_64_movl_memindex_reg(cd, OFFSET(java_intarray, data[0]), s1, s2, 2, d);
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
			x86_64_movss_memindex_reg(cd, OFFSET(java_floatarray, data[0]), s1, s2, 2, d);
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
			x86_64_movsd_memindex_reg(cd, OFFSET(java_doublearray, data[0]), s1, s2, 3, d);
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
			x86_64_movzwq_memindex_reg(cd, OFFSET(java_chararray, data[0]), s1, s2, 1, d);
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
			x86_64_movswq_memindex_reg(cd, OFFSET(java_shortarray, data[0]), s1, s2, 1, d);
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
   			x86_64_movsbq_memindex_reg(cd, OFFSET(java_bytearray, data[0]), s1, s2, 0, d);
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
			x86_64_mov_reg_memindex(cd, s3, OFFSET(java_objectarray, data[0]), s1, s2, 3);
			break;

		case ICMD_LASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			x86_64_mov_reg_memindex(cd, s3, OFFSET(java_longarray, data[0]), s1, s2, 3);
			break;

		case ICMD_IASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			x86_64_movl_reg_memindex(cd, s3, OFFSET(java_intarray, data[0]), s1, s2, 2);
			break;

		case ICMD_FASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_flt(s3, src, REG_FTMP3);
			x86_64_movss_reg_memindex(cd, s3, OFFSET(java_floatarray, data[0]), s1, s2, 2);
			break;

		case ICMD_DASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_flt(s3, src, REG_FTMP3);
			x86_64_movsd_reg_memindex(cd, s3, OFFSET(java_doublearray, data[0]), s1, s2, 3);
			break;

		case ICMD_CASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			x86_64_movw_reg_memindex(cd, s3, OFFSET(java_chararray, data[0]), s1, s2, 1);
			break;

		case ICMD_SASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			x86_64_movw_reg_memindex(cd, s3, OFFSET(java_shortarray, data[0]), s1, s2, 1);
			break;

		case ICMD_BASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			x86_64_movb_reg_memindex(cd, s3, OFFSET(java_bytearray, data[0]), s1, s2, 0);
			break;

		case ICMD_IASTORECONST: /* ..., arrayref, index  ==> ...              */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			x86_64_movl_imm_memindex(cd, iptr->val.i, OFFSET(java_intarray, data[0]), s1, s2, 2);
			break;

		case ICMD_LASTORECONST: /* ..., arrayref, index  ==> ...              */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}

			if (IS_IMM32(iptr->val.l)) {
				x86_64_mov_imm_memindex(cd, (u4) (iptr->val.l & 0x00000000ffffffff), OFFSET(java_longarray, data[0]), s1, s2, 3);

			} else {
				x86_64_movl_imm_memindex(cd, (u4) (iptr->val.l & 0x00000000ffffffff), OFFSET(java_longarray, data[0]), s1, s2, 3);
				x86_64_movl_imm_memindex(cd, (u4) (iptr->val.l >> 32), OFFSET(java_longarray, data[0]) + 4, s1, s2, 3);
			}
			break;

		case ICMD_AASTORECONST: /* ..., arrayref, index  ==> ...              */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			x86_64_mov_imm_memindex(cd, 0, OFFSET(java_objectarray, data[0]), s1, s2, 3);
			break;

		case ICMD_BASTORECONST: /* ..., arrayref, index  ==> ...              */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			x86_64_movb_imm_memindex(cd, iptr->val.i, OFFSET(java_bytearray, data[0]), s1, s2, 0);
			break;

		case ICMD_CASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			x86_64_movw_imm_memindex(cd, iptr->val.i, OFFSET(java_chararray, data[0]), s1, s2, 1);
			break;

		case ICMD_SASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			x86_64_movw_imm_memindex(cd, iptr->val.i, OFFSET(java_shortarray, data[0]), s1, s2, 1);
			break;


		case ICMD_GETSTATIC:  /* ...  ==> ..., value                          */
		                      /* op1 = type, val.a = field address            */

			if (!iptr->val.a) {
				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_get_putstatic,
									(unresolved_field *) iptr->target);
				a = 0;

			} else {
				fieldinfo *fi = iptr->val.a;

				if (!fi->class->initialized) {
					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_clinit, fi->class);
				}

				a = (ptrint) &(fi->value);
  			}

			/* This approach is much faster than moving the field address     */
			/* inline into a register. */
			a = dseg_addaddress(cd, a);
  			x86_64_mov_membase_reg(cd, RIP, -(((ptrint) cd->mcodeptr + 7) - (ptrint) cd->mcodebase) + a, REG_ITMP2);
			switch (iptr->op1) {
			case TYPE_INT:
				d = reg_of_var(rd, iptr->dst, REG_ITMP1);
				x86_64_movl_membase_reg(cd, REG_ITMP2, 0, d);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_LNG:
			case TYPE_ADR:
				d = reg_of_var(rd, iptr->dst, REG_ITMP1);
				x86_64_mov_membase_reg(cd, REG_ITMP2, 0, d);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_FLT:
				d = reg_of_var(rd, iptr->dst, REG_ITMP1);
				x86_64_movss_membase_reg(cd, REG_ITMP2, 0, d);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			case TYPE_DBL:				
				d = reg_of_var(rd, iptr->dst, REG_ITMP1);
				x86_64_movsd_membase_reg(cd, REG_ITMP2, 0, d);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			}
			break;

		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */
		                      /* op1 = type, val.a = field address            */

			if (!iptr->val.a) {
				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_get_putstatic,
									(unresolved_field *) iptr->target);
				a = 0;

			} else {
				fieldinfo *fi = iptr->val.a;

				if (!fi->class->initialized) {
					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_clinit, fi->class);
				}

				a = (ptrint) &(fi->value);
  			}

			/* This approach is much faster than moving the field address     */
			/* inline into a register. */
			a = dseg_addaddress(cd, a);
  			x86_64_mov_membase_reg(cd, RIP, -(((ptrint) cd->mcodeptr + 7) - (ptrint) cd->mcodebase) + a, REG_ITMP2);
			switch (iptr->op1) {
			case TYPE_INT:
				var_to_reg_int(s2, src, REG_ITMP1);
				x86_64_movl_reg_membase(cd, s2, REG_ITMP2, 0);
				break;
			case TYPE_LNG:
			case TYPE_ADR:
				var_to_reg_int(s2, src, REG_ITMP1);
				x86_64_mov_reg_membase(cd, s2, REG_ITMP2, 0);
				break;
			case TYPE_FLT:
				var_to_reg_flt(s2, src, REG_FTMP1);
				x86_64_movss_reg_membase(cd, s2, REG_ITMP2, 0);
				break;
			case TYPE_DBL:
				var_to_reg_flt(s2, src, REG_FTMP1);
				x86_64_movsd_reg_membase(cd, s2, REG_ITMP2, 0);
				break;
			}
			break;

		case ICMD_PUTSTATICCONST: /* ...  ==> ...                             */
		                          /* val = value (in current instruction)     */
		                          /* op1 = type, val.a = field address (in    */
		                          /* following NOP)                           */

			if (!iptr[1].val.a) {
				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_get_putstatic,
									(unresolved_field *) iptr[1].target);
				a = 0;

			} else {
				fieldinfo *fi = iptr[1].val.a;

				if (!fi->class->initialized) {
					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_clinit, fi->class);
				}

				a = (ptrint) &(fi->value);
  			}

			/* This approach is much faster than moving the field address     */
			/* inline into a register. */
			a = dseg_addaddress(cd, a);
  			x86_64_mov_membase_reg(cd, RIP, -(((ptrint) cd->mcodeptr + 7) - (ptrint) cd->mcodebase) + a, REG_ITMP1);
			switch (iptr->op1) {
			case TYPE_INT:
			case TYPE_FLT:
				x86_64_movl_imm_membase(cd, iptr->val.i, REG_ITMP1, 0);
				break;
			case TYPE_LNG:
			case TYPE_ADR:
			case TYPE_DBL:
				if (IS_IMM32(iptr->val.l)) {
					x86_64_mov_imm_membase(cd, iptr->val.l, REG_ITMP1, 0);
				} else {
					x86_64_movl_imm_membase(cd, iptr->val.l, REG_ITMP1, 0);
					x86_64_movl_imm_membase(cd, iptr->val.l >> 32, REG_ITMP1, 4);
				}
				break;
			}
			break;

		case ICMD_GETFIELD:   /* ...  ==> ..., value                          */
		                      /* op1 = type, val.i = field offset             */

			var_to_reg_int(s1, src, REG_ITMP1);
			gen_nullptr_check(s1);

			if (!iptr->val.a) {
				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_get_putfield,
									(unresolved_field *) iptr->target);
				a = 0;
			} else
				a = ((fieldinfo *) (iptr->val.a))->offset;

			switch (iptr->op1) {
			case TYPE_INT:
				d = reg_of_var(rd, iptr->dst, REG_ITMP1);
				x86_64_movl_membase32_reg(cd, s1, a, d);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_LNG:
			case TYPE_ADR:
				d = reg_of_var(rd, iptr->dst, REG_ITMP1);
				x86_64_mov_membase32_reg(cd, s1, a, d);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_FLT:
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				x86_64_movss_membase32_reg(cd, s1, a, d);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			case TYPE_DBL:				
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				x86_64_movsd_membase32_reg(cd, s1, a, d);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			}
			break;

		case ICMD_PUTFIELD:   /* ..., objectref, value  ==> ...               */
		                      /* op1 = type, val.i = field offset             */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			gen_nullptr_check(s1);
			if (IS_INT_LNG_TYPE(iptr->op1)) {
				var_to_reg_int(s2, src, REG_ITMP2);
			} else {
				var_to_reg_flt(s2, src, REG_FTMP2);
			}

			if (!iptr->val.a) {
				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_get_putfield,
									(unresolved_field *) iptr->target);
				a = 0;
			} else
				a = ((fieldinfo *) (iptr->val.a))->offset;

			switch (iptr->op1) {
			case TYPE_INT:
				x86_64_movl_reg_membase32(cd, s2, s1, a);
				break;
			case TYPE_LNG:
			case TYPE_ADR:
				x86_64_mov_reg_membase32(cd, s2, s1, a);
				break;
			case TYPE_FLT:
				x86_64_movss_reg_membase32(cd, s2, s1, a);
				break;
			case TYPE_DBL:
				x86_64_movsd_reg_membase32(cd, s2, s1, a);
				break;
			}
			break;

		case ICMD_PUTFIELDCONST:  /* ..., objectref, value  ==> ...           */
		                          /* val = value (in current instruction)     */
		                          /* op1 = type, val.a = field address (in    */
		                          /* following NOP)                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			gen_nullptr_check(s1);

			if (!iptr[1].val.a) {
				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_get_putfield,
									(unresolved_field *) iptr[1].target);
				a = 0;
			} else
				a = ((fieldinfo *) (iptr[1].val.a))->offset;

			switch (iptr->op1) {
			case TYPE_INT:
			case TYPE_FLT:
				x86_64_movl_imm_membase32(cd, iptr->val.i, s1, a);
				break;
			case TYPE_LNG:
			case TYPE_ADR:
			case TYPE_DBL:
				if (IS_IMM32(iptr->val.l)) {
					x86_64_mov_imm_membase32(cd, iptr->val.l, s1, a);
				} else {
					x86_64_movl_imm_membase32(cd, iptr->val.l, s1, a);
					x86_64_movl_imm_membase32(cd, iptr->val.l >> 32, s1, a + 4);
				}
				break;
			}
			break;


		/* branch operations **************************************************/

		case ICMD_ATHROW:       /* ..., objectref ==> ... (, objectref)       */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_INTMOVE(s1, REG_ITMP1_XPTR);

			x86_64_call_imm(cd, 0); /* passing exception pointer              */
			x86_64_pop_reg(cd, REG_ITMP2_XPC);

  			x86_64_mov_imm_reg(cd, (ptrint) asm_handle_exception, REG_ITMP3);
  			x86_64_jmp_reg(cd, REG_ITMP3);
			break;

		case ICMD_GOTO:         /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */

			x86_64_jmp_imm(cd, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_JSR:          /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */

  			x86_64_call_imm(cd, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;
			
		case ICMD_RET:          /* ... ==> ...                                */
		                        /* op1 = local variable                       */

			var = &(rd->locals[iptr->op1][TYPE_ADR]);
			var_to_reg_int(s1, var, REG_ITMP1);
			x86_64_jmp_reg(cd, s1);
			break;

		case ICMD_IFNULL:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			if (src->flags & INMEMORY) {
				x86_64_alu_imm_membase(cd, X86_64_CMP, 0, REG_SP, src->regoff * 8);

			} else {
				x86_64_test_reg_reg(cd, src->regoff, src->regoff);
			}
			x86_64_jcc(cd, X86_64_CC_E, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IFNONNULL:    /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			if (src->flags & INMEMORY) {
				x86_64_alu_imm_membase(cd, X86_64_CMP, 0, REG_SP, src->regoff * 8);

			} else {
				x86_64_test_reg_reg(cd, src->regoff, src->regoff);
			}
			x86_64_jcc(cd, X86_64_CC_NE, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IFEQ:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			x86_64_emit_ifcc(cd, X86_64_CC_E, src, iptr);
			break;

		case ICMD_IFLT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			x86_64_emit_ifcc(cd, X86_64_CC_L, src, iptr);
			break;

		case ICMD_IFLE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			x86_64_emit_ifcc(cd, X86_64_CC_LE, src, iptr);
			break;

		case ICMD_IFNE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			x86_64_emit_ifcc(cd, X86_64_CC_NE, src, iptr);
			break;

		case ICMD_IFGT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			x86_64_emit_ifcc(cd, X86_64_CC_G, src, iptr);
			break;

		case ICMD_IFGE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			x86_64_emit_ifcc(cd, X86_64_CC_GE, src, iptr);
			break;

		case ICMD_IF_LEQ:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			x86_64_emit_if_lcc(cd, X86_64_CC_E, src, iptr);
			break;

		case ICMD_IF_LLT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			x86_64_emit_if_lcc(cd, X86_64_CC_L, src, iptr);
			break;

		case ICMD_IF_LLE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			x86_64_emit_if_lcc(cd, X86_64_CC_LE, src, iptr);
			break;

		case ICMD_IF_LNE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			x86_64_emit_if_lcc(cd, X86_64_CC_NE, src, iptr);
			break;

		case ICMD_IF_LGT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			x86_64_emit_if_lcc(cd, X86_64_CC_G, src, iptr);
			break;

		case ICMD_IF_LGE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			x86_64_emit_if_lcc(cd, X86_64_CC_GE, src, iptr);
			break;

		case ICMD_IF_ICMPEQ:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			x86_64_emit_if_icmpcc(cd, X86_64_CC_E, src, iptr);
			break;

		case ICMD_IF_LCMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_ACMPEQ:    /* op1 = target JavaVM pc                     */

			x86_64_emit_if_lcmpcc(cd, X86_64_CC_E, src, iptr);
			break;

		case ICMD_IF_ICMPNE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			x86_64_emit_if_icmpcc(cd, X86_64_CC_NE, src, iptr);
			break;

		case ICMD_IF_LCMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_ACMPNE:    /* op1 = target JavaVM pc                     */

			x86_64_emit_if_lcmpcc(cd, X86_64_CC_NE, src, iptr);
			break;

		case ICMD_IF_ICMPLT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			x86_64_emit_if_icmpcc(cd, X86_64_CC_L, src, iptr);
			break;

		case ICMD_IF_LCMPLT:    /* ..., value, value ==> ...                  */
	                            /* op1 = target JavaVM pc                     */

			x86_64_emit_if_lcmpcc(cd, X86_64_CC_L, src, iptr);
			break;

		case ICMD_IF_ICMPGT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			x86_64_emit_if_icmpcc(cd, X86_64_CC_G, src, iptr);
			break;

		case ICMD_IF_LCMPGT:    /* ..., value, value ==> ...                  */
                                /* op1 = target JavaVM pc                     */

			x86_64_emit_if_lcmpcc(cd, X86_64_CC_G, src, iptr);
			break;

		case ICMD_IF_ICMPLE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			x86_64_emit_if_icmpcc(cd, X86_64_CC_LE, src, iptr);
			break;

		case ICMD_IF_LCMPLE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			x86_64_emit_if_lcmpcc(cd, X86_64_CC_LE, src, iptr);
			break;

		case ICMD_IF_ICMPGE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			x86_64_emit_if_icmpcc(cd, X86_64_CC_GE, src, iptr);
			break;

		case ICMD_IF_LCMPGE:    /* ..., value, value ==> ...                  */
	                            /* op1 = target JavaVM pc                     */

			x86_64_emit_if_lcmpcc(cd, X86_64_CC_GE, src, iptr);
			break;

		/* (value xx 0) ? IFxx_ICONST : ELSE_ICONST                           */

		case ICMD_ELSE_ICONST:  /* handled by IFxx_ICONST                     */
			break;

		case ICMD_IFEQ_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr[1].opc == ICMD_ELSE_ICONST) {
				if (s1 == d) {
					M_INTMOVE(s1, REG_ITMP1);
					s1 = REG_ITMP1;
				}
				x86_64_movl_imm_reg(cd, iptr[1].val.i, d);
			}
			x86_64_movl_imm_reg(cd, iptr->val.i, REG_ITMP2);
			x86_64_testl_reg_reg(cd, s1, s1);
			x86_64_cmovccl_reg_reg(cd, X86_64_CC_E, REG_ITMP2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFNE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr[1].opc == ICMD_ELSE_ICONST) {
				if (s1 == d) {
					M_INTMOVE(s1, REG_ITMP1);
					s1 = REG_ITMP1;
				}
				x86_64_movl_imm_reg(cd, iptr[1].val.i, d);
			}
			x86_64_movl_imm_reg(cd, iptr->val.i, REG_ITMP2);
			x86_64_testl_reg_reg(cd, s1, s1);
			x86_64_cmovccl_reg_reg(cd, X86_64_CC_NE, REG_ITMP2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFLT_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr[1].opc == ICMD_ELSE_ICONST) {
				if (s1 == d) {
					M_INTMOVE(s1, REG_ITMP1);
					s1 = REG_ITMP1;
				}
				x86_64_movl_imm_reg(cd, iptr[1].val.i, d);
			}
			x86_64_movl_imm_reg(cd, iptr->val.i, REG_ITMP2);
			x86_64_testl_reg_reg(cd, s1, s1);
			x86_64_cmovccl_reg_reg(cd, X86_64_CC_L, REG_ITMP2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFGE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr[1].opc == ICMD_ELSE_ICONST) {
				if (s1 == d) {
					M_INTMOVE(s1, REG_ITMP1);
					s1 = REG_ITMP1;
				}
				x86_64_movl_imm_reg(cd, iptr[1].val.i, d);
			}
			x86_64_movl_imm_reg(cd, iptr->val.i, REG_ITMP2);
			x86_64_testl_reg_reg(cd, s1, s1);
			x86_64_cmovccl_reg_reg(cd, X86_64_CC_GE, REG_ITMP2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFGT_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr[1].opc == ICMD_ELSE_ICONST) {
				if (s1 == d) {
					M_INTMOVE(s1, REG_ITMP1);
					s1 = REG_ITMP1;
				}
				x86_64_movl_imm_reg(cd, iptr[1].val.i, d);
			}
			x86_64_movl_imm_reg(cd, iptr->val.i, REG_ITMP2);
			x86_64_testl_reg_reg(cd, s1, s1);
			x86_64_cmovccl_reg_reg(cd, X86_64_CC_G, REG_ITMP2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFLE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr[1].opc == ICMD_ELSE_ICONST) {
				if (s1 == d) {
					M_INTMOVE(s1, REG_ITMP1);
					s1 = REG_ITMP1;
				}
				x86_64_movl_imm_reg(cd, iptr[1].val.i, d);
			}
			x86_64_movl_imm_reg(cd, iptr->val.i, REG_ITMP2);
			x86_64_testl_reg_reg(cd, s1, s1);
			x86_64_cmovccl_reg_reg(cd, X86_64_CC_LE, REG_ITMP2, d);
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

		case ICMD_RETURN:      /* ...  ==> ...                                */

nowperformreturn:
			{
			s4 i, p;
			
  			p = parentargs_base;
			
			/* call trace function */
			if (runverbose) {
				x86_64_alu_imm_reg(cd, X86_64_SUB, 2 * 8, REG_SP);

				x86_64_mov_reg_membase(cd, REG_RESULT, REG_SP, 0 * 8);
				x86_64_movq_reg_membase(cd, REG_FRESULT, REG_SP, 1 * 8);

  				x86_64_mov_imm_reg(cd, (u8) m, rd->argintregs[0]);
  				x86_64_mov_reg_reg(cd, REG_RESULT, rd->argintregs[1]);
				M_FLTMOVE(REG_FRESULT, rd->argfltregs[0]);
 				M_FLTMOVE(REG_FRESULT, rd->argfltregs[1]);

  				x86_64_mov_imm_reg(cd, (u8) builtin_displaymethodstop, REG_ITMP1);
				x86_64_call_reg(cd, REG_ITMP1);

				x86_64_mov_membase_reg(cd, REG_SP, 0 * 8, REG_RESULT);
				x86_64_movq_membase_reg(cd, REG_SP, 1 * 8, REG_FRESULT);

				x86_64_alu_imm_reg(cd, X86_64_ADD, 2 * 8, REG_SP);
			}

#if defined(USE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				x86_64_mov_membase_reg(cd, REG_SP, rd->maxmemuse * 8, rd->argintregs[0]);
	
				/* we need to save the proper return value */
				switch (iptr->opc) {
				case ICMD_IRETURN:
				case ICMD_ARETURN:
				case ICMD_LRETURN:
					x86_64_mov_reg_membase(cd, REG_RESULT, REG_SP, rd->maxmemuse * 8);
					break;
				case ICMD_FRETURN:
				case ICMD_DRETURN:
					x86_64_movq_reg_membase(cd, REG_FRESULT, REG_SP, rd->maxmemuse * 8);
					break;
				}

				x86_64_mov_imm_reg(cd, (u8) builtin_monitorexit, REG_ITMP1);
				x86_64_call_reg(cd, REG_ITMP1);

				/* and now restore the proper return value */
				switch (iptr->opc) {
				case ICMD_IRETURN:
				case ICMD_ARETURN:
				case ICMD_LRETURN:
					x86_64_mov_membase_reg(cd, REG_SP, rd->maxmemuse * 8, REG_RESULT);
					break;
				case ICMD_FRETURN:
				case ICMD_DRETURN:
					x86_64_movq_membase_reg(cd, REG_SP, rd->maxmemuse * 8, REG_FRESULT);
					break;
				}
			}
#endif

			/* restore saved registers                                        */
			for (i = rd->savintregcnt - 1; i >= rd->maxsavintreguse; i--) {
				p--; x86_64_mov_membase_reg(cd, REG_SP, p * 8, rd->savintregs[i]);
			}
			for (i = rd->savfltregcnt - 1; i >= rd->maxsavfltreguse; i--) {
  				p--; x86_64_movq_membase_reg(cd, REG_SP, p * 8, rd->savfltregs[i]);
			}

			/* deallocate stack                                               */
			if (parentargs_base) {
				x86_64_alu_imm_reg(cd, X86_64_ADD, parentargs_base * 8, REG_SP);
			}

			x86_64_ret(cd);
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
				M_INTMOVE(s1, REG_ITMP1);
				if (l != 0) {
					x86_64_alul_imm_reg(cd, X86_64_SUB, l, REG_ITMP1);
				}
				i = i - l + 1;

                /* range check */
				x86_64_alul_imm_reg(cd, X86_64_CMP, i - 1, REG_ITMP1);
				x86_64_jcc(cd, X86_64_CC_A, 0);

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

				x86_64_mov_imm_reg(cd, 0, REG_ITMP2);
				dseg_adddata(cd, cd->mcodeptr);
				x86_64_mov_memindex_reg(cd, -(cd->dseglen), REG_ITMP2, REG_ITMP1, 3, REG_ITMP1);
				x86_64_jmp_reg(cd, REG_ITMP1);
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
			
				MCODECHECK((i<<2)+8);
				var_to_reg_int(s1, src, REG_ITMP1);    /* reg compare should always be faster */
				while (--i >= 0) {
					s4ptr += 2;
					++tptr;

					val = s4ptr[0];
					x86_64_alul_imm_reg(cd, X86_64_CMP, val, s1);
					x86_64_jcc(cd, X86_64_CC_E, 0);
					/* codegen_addreference(cd, BlockPtrOfPC(s4ptr[1]), cd->mcodeptr); */
					codegen_addreference(cd, (basicblock *) tptr[0], cd->mcodeptr); 
				}

				x86_64_jmp_imm(cd, 0);
				/* codegen_addreference(cd, BlockPtrOfPC(l), cd->mcodeptr); */
			
				tptr = (void **) iptr->target;
				codegen_addreference(cd, (basicblock *) tptr[0], cd->mcodeptr);
			}
			break;


		case ICMD_BUILTIN3:     /* ..., arg1, arg2, arg3 ==> ...              */
		                        /* op1 = return type, val.a = function pointer*/
			s3 = 3;
			goto gen_method;

		case ICMD_BUILTIN2:     /* ..., arg1, arg2 ==> ...                    */
		                        /* op1 = return type, val.a = function pointer*/
			s3 = 2;
			goto gen_method;

		case ICMD_BUILTIN1:     /* ..., arg1 ==> ...                          */
		                        /* op1 = return type, val.a = function pointer*/
			s3 = 1;
			goto gen_method;

		case ICMD_INVOKESTATIC: /* ..., [arg1, [arg2 ...]] ==> ...            */
		                        /* op1 = arg count, val.a = method pointer    */

		case ICMD_INVOKESPECIAL:/* ..., objectref, [arg1, [arg2 ...]] ==> ... */
		case ICMD_INVOKEVIRTUAL:/* op1 = arg count, val.a = method pointer    */
		case ICMD_INVOKEINTERFACE:

			s3 = iptr->op1;

gen_method: {
			methodinfo *lm;
			stackptr    tmpsrc;
			s4          iarg;
			s4          farg;

			MCODECHECK((s3 << 1) + 64);

			/* copy arguments to registers or stack location ******************/

			/* count integer and float arguments */

			iarg = 0;
			farg = 0;

			for (s2 = s3, tmpsrc = src; --s2 >= 0; tmpsrc = tmpsrc->prev) {
				IS_INT_LNG_TYPE(tmpsrc->type) ? iarg++ : farg++;
			}

			/* calculate amount of arguments to be on stack */

			s2 = (iarg > INT_ARG_CNT) ? iarg - INT_ARG_CNT : 0 +
				(farg > FLT_ARG_CNT) ? farg - FLT_ARG_CNT : 0;

			for (; --s3 >= 0; src = src->prev) {
				/* decrement the current argument type */
				IS_INT_LNG_TYPE(src->type) ? iarg-- : farg--;

				if (src->varkind == ARGVAR) {
					if (IS_INT_LNG_TYPE(src->type)) {
						if (iarg >= INT_ARG_CNT) {
							s2--;
						}
					} else {
						if (farg >= FLT_ARG_CNT) {
							s2--;
						}
					}
					continue;
				}

				if (IS_INT_LNG_TYPE(src->type)) {
					if (iarg < INT_ARG_CNT) {
						s1 = rd->argintregs[iarg];
						var_to_reg_int(d, src, s1);
						M_INTMOVE(d, s1);

					} else {
						var_to_reg_int(d, src, REG_ITMP1);
						s2--;
						x86_64_mov_reg_membase(cd, d, REG_SP, s2 * 8);
					}

				} else {
					if (farg < FLT_ARG_CNT) {
						s1 = rd->argfltregs[farg];
						var_to_reg_flt(d, src, s1);
						M_FLTMOVE(d, s1);

					} else {
						var_to_reg_flt(d, src, REG_FTMP1);
						s2--;
						x86_64_movq_reg_membase(cd, d, REG_SP, s2 * 8);
					}
				}
			} /* end of for */

			lm = iptr->val.a;
			switch (iptr->opc) {
			case ICMD_BUILTIN3:
			case ICMD_BUILTIN2:
			case ICMD_BUILTIN1:
				a = (ptrint) lm;
				d = iptr->op1;

				x86_64_mov_imm_reg(cd, a, REG_ITMP1);
				x86_64_call_reg(cd, REG_ITMP1);
				break;

			case ICMD_INVOKESPECIAL:
				/* first argument contains pointer */
				gen_nullptr_check(rd->argintregs[0]);

				/* access memory for hardware nullptr */
				x86_64_mov_membase_reg(cd, rd->argintregs[0], 0, REG_ITMP2);

				/* fall through */

			case ICMD_INVOKESTATIC:
				if (!lm) {
					unresolved_method *um = iptr->target;

					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_invokestatic_special, um);

					a = 0;
					d = um->methodref->parseddesc.md->returntype.type;

				} else {
					a = (ptrint) lm->stubroutine;
					d = lm->parseddesc->returntype.type;
				}

				x86_64_mov_imm_reg(cd, a, REG_ITMP2);
				x86_64_call_reg(cd, REG_ITMP2);
				break;

			case ICMD_INVOKEVIRTUAL:
				gen_nullptr_check(rd->argintregs[0]);

				if (!lm) {
					unresolved_method *um = iptr->target;

					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_invokevirtual, um);

					s1 = 0;
					d = um->methodref->parseddesc.md->returntype.type;

				} else {
					s1 = OFFSET(vftbl_t, table[0]) +
						sizeof(methodptr) * lm->vftblindex;
					d = lm->parseddesc->returntype.type;
				}

				x86_64_mov_membase_reg(cd, rd->argintregs[0],
									   OFFSET(java_objectheader, vftbl),
									   REG_ITMP2);
				x86_64_mov_membase32_reg(cd, REG_ITMP2, s1, REG_ITMP1);
				x86_64_call_reg(cd, REG_ITMP1);
				break;

			case ICMD_INVOKEINTERFACE:
				gen_nullptr_check(rd->argintregs[0]);

				if (!lm) {
					unresolved_method *um = iptr->target;

					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_invokeinterface, um);

					s1 = 0;
					s2 = 0;
					d = um->methodref->parseddesc.md->returntype.type;

				} else {
					classinfo * ci = lm->class;

					s1 = OFFSET(vftbl_t, interfacetable[0]) -
						sizeof(methodptr) * ci->index;

					s2 = sizeof(methodptr) * (lm - ci->methods);

					d = lm->parseddesc->returntype.type;
				}

				x86_64_mov_membase_reg(cd, rd->argintregs[0],
									   OFFSET(java_objectheader, vftbl),
									   REG_ITMP2);
				x86_64_mov_membase32_reg(cd, REG_ITMP2, s1, REG_ITMP2);
				x86_64_mov_membase32_reg(cd, REG_ITMP2, s2, REG_ITMP1);
				x86_64_call_reg(cd, REG_ITMP1);
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
					M_FLTMOVE(REG_FRESULT, s1);
					store_reg_to_var_flt(iptr->dst, s1);
				}
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

			s2 = 3; /* mov_membase_reg */
			CALCOFFSETBYTES(s2, s1, OFFSET(java_objectheader, vftbl));

			s2 += 3 + 4 /* movl_membase32_reg */ + 3 + 4 /* sub imm32 */ +
				3 /* test */ + 6 /* jcc */ + 3 + 4 /* mov_membase32_reg */ +
				3 /* test */ + 6 /* jcc */;

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

			/* if class is not resolved, check which code to call */

			if (!super) {
				x86_64_test_reg_reg(cd, s1, s1);
				x86_64_jcc(cd, X86_64_CC_Z, 6 + 7 + 6 + s2 + 5 + s3);

				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_checkcast_instanceof_flags,
									(constant_classref *) iptr->target);

				x86_64_movl_imm_reg(cd, 0, REG_ITMP2); /* super->flags */
				x86_64_alul_imm_reg(cd, X86_64_AND, ACC_INTERFACE, REG_ITMP2);
				x86_64_jcc(cd, X86_64_CC_Z, s2 + 5);
			}

			/* interface checkcast code */

			if (!super || (super->flags & ACC_INTERFACE)) {
				if (super) {
					x86_64_test_reg_reg(cd, s1, s1);
					x86_64_jcc(cd, X86_64_CC_Z, s2);
				}

				x86_64_mov_membase_reg(cd, s1,
									   OFFSET(java_objectheader, vftbl),
									   REG_ITMP2);

				if (!super)
					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_checkcast_instanceof_interface,
										(constant_classref *) iptr->target);

				x86_64_movl_membase32_reg(cd, REG_ITMP2,
										  OFFSET(vftbl_t, interfacetablelength),
										  REG_ITMP3);
				x86_64_alu_imm32_reg(cd, X86_64_SUB, superindex, REG_ITMP3);
				x86_64_test_reg_reg(cd, REG_ITMP3, REG_ITMP3);
				x86_64_jcc(cd, X86_64_CC_LE, 0);
				codegen_addxcastrefs(cd, cd->mcodeptr);
				x86_64_mov_membase32_reg(cd, REG_ITMP2,
										 OFFSET(vftbl_t, interfacetable[0]) -
										 superindex * sizeof(methodptr*),
										 REG_ITMP3);
				x86_64_test_reg_reg(cd, REG_ITMP3, REG_ITMP3);
				x86_64_jcc(cd, X86_64_CC_E, 0);
				codegen_addxcastrefs(cd, cd->mcodeptr);

				if (!super)
					x86_64_jmp_imm(cd, s3);
			}

			/* class checkcast code */

			if (!super || !(super->flags & ACC_INTERFACE)) {
				if (super) {
					x86_64_test_reg_reg(cd, s1, s1);
					x86_64_jcc(cd, X86_64_CC_Z, s3);
				}

				x86_64_mov_membase_reg(cd, s1,
									   OFFSET(java_objectheader, vftbl),
									   REG_ITMP2);

				if (!super)
					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_checkcast_class,
										(constant_classref *) iptr->target);

				x86_64_mov_imm_reg(cd, (ptrint) supervftbl, REG_ITMP3);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
				codegen_threadcritstart(cd, cd->mcodeptr - cd->mcodebase);
#endif
				x86_64_movl_membase32_reg(cd, REG_ITMP2,
										  OFFSET(vftbl_t, baseval),
										  REG_ITMP2);
/*  					if (s1 != REG_ITMP1) { */
/*  						x86_64_movl_membase_reg(cd, REG_ITMP3, */
/*  												OFFSET(vftbl_t, baseval), */
/*  												REG_ITMP1); */
/*  						x86_64_movl_membase_reg(cd, REG_ITMP3, */
/*  												OFFSET(vftbl_t, diffval), */
/*  												REG_ITMP3); */
/*  #if defined(USE_THREADS) && defined(NATIVE_THREADS) */
/*  						codegen_threadcritstop(cd, cd->mcodeptr - cd->mcodebase); */
/*  #endif */
/*  						x86_64_alu_reg_reg(cd, X86_64_SUB, REG_ITMP1, REG_ITMP2); */

/*  					} else { */
				x86_64_movl_membase32_reg(cd, REG_ITMP3,
										  OFFSET(vftbl_t, baseval),
										  REG_ITMP3);
				x86_64_alu_reg_reg(cd, X86_64_SUB, REG_ITMP3, REG_ITMP2);
				x86_64_mov_imm_reg(cd, (ptrint) supervftbl, REG_ITMP3);
				x86_64_movl_membase_reg(cd, REG_ITMP3,
										OFFSET(vftbl_t, diffval),
										REG_ITMP3);
/*  					} */
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
				codegen_threadcritstop(cd, cd->mcodeptr - cd->mcodebase);
#endif
				x86_64_alu_reg_reg(cd, X86_64_CMP, REG_ITMP3, REG_ITMP2);
				x86_64_jcc(cd, X86_64_CC_A, 0);    /* (u) REG_ITMP1 > (u) REG_ITMP2 -> jump */
				codegen_addxcastrefs(cd, cd->mcodeptr);
			}
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
/*  			if (iptr->dst->flags & INMEMORY) { */
/*  				x86_64_mov_reg_membase(cd, s1, REG_SP, iptr->dst->regoff * 8); */
/*  			} else { */
/*  				M_INTMOVE(s1, iptr->dst->regoff); */
/*  			} */
			}
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

			s2 = 3; /* mov_membase_reg */
			CALCOFFSETBYTES(s2, s1, OFFSET(java_objectheader, vftbl));
			s2 += 3 + 4 /* movl_membase32_reg */ + 3 + 4 /* sub_imm32 */ +
				3 /* test */ + 6 /* jcc */ + 3 + 4 /* mov_membase32_reg */ +
				3 /* test */ + 4 /* setcc */;

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

			x86_64_alu_reg_reg(cd, X86_64_XOR, d, d);

			/* if class is not resolved, check which code to call */

			if (!super) {
				x86_64_test_reg_reg(cd, s1, s1);
				x86_64_jcc(cd, X86_64_CC_Z, 6 + 7 + 6 + s2 + 5 + s3);

				codegen_addpatchref(cd, cd->mcodeptr,
									PATCHER_checkcast_instanceof_flags,
									(constant_classref *) iptr->target);

				x86_64_movl_imm_reg(cd, 0, REG_ITMP3); /* super->flags */
				x86_64_alul_imm_reg(cd, X86_64_AND, ACC_INTERFACE, REG_ITMP3);
				x86_64_jcc(cd, X86_64_CC_Z, s2 + 5);
			}

			/* interface instanceof code */

			if (!super || (super->flags & ACC_INTERFACE)) {
				if (super) {
					x86_64_test_reg_reg(cd, s1, s1);
					x86_64_jcc(cd, X86_64_CC_Z, s2);
				}

				x86_64_mov_membase_reg(cd, s1,
									   OFFSET(java_objectheader, vftbl),
									   REG_ITMP1);
				if (!super)
					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_checkcast_instanceof_interface,
										(constant_classref *) iptr->target);

				x86_64_movl_membase32_reg(cd, REG_ITMP1,
										  OFFSET(vftbl_t, interfacetablelength),
										  REG_ITMP3);
				x86_64_alu_imm32_reg(cd, X86_64_SUB, superindex, REG_ITMP3);
				x86_64_test_reg_reg(cd, REG_ITMP3, REG_ITMP3);

				a = 3 + 4 /* mov_membase32_reg */ + 3 /* test */ + 4 /* setcc */;

				x86_64_jcc(cd, X86_64_CC_LE, a);
				x86_64_mov_membase32_reg(cd, REG_ITMP1,
										 OFFSET(vftbl_t, interfacetable[0]) -
										 superindex * sizeof(methodptr*),
										 REG_ITMP1);
				x86_64_test_reg_reg(cd, REG_ITMP1, REG_ITMP1);
				x86_64_setcc_reg(cd, X86_64_CC_NE, d);

				if (!super)
					x86_64_jmp_imm(cd, s3);
			}

			/* class instanceof code */

			if (!super || !(super->flags & ACC_INTERFACE)) {
				if (super) {
					x86_64_test_reg_reg(cd, s1, s1);
					x86_64_jcc(cd, X86_64_CC_E, s3);
				}

				x86_64_mov_membase_reg(cd, s1,
									   OFFSET(java_objectheader, vftbl),
									   REG_ITMP1);

				if (!super)
					codegen_addpatchref(cd, cd->mcodeptr,
										PATCHER_instanceof_class,
										(constant_classref *) iptr->target);

				x86_64_mov_imm_reg(cd, (ptrint) supervftbl, REG_ITMP2);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
				codegen_threadcritstart(cd, cd->mcodeptr - cd->mcodebase);
#endif
				x86_64_movl_membase_reg(cd, REG_ITMP1,
										OFFSET(vftbl_t, baseval),
										REG_ITMP1);
				x86_64_movl_membase_reg(cd, REG_ITMP2,
										OFFSET(vftbl_t, diffval),
										REG_ITMP3);
				x86_64_movl_membase_reg(cd, REG_ITMP2,
										OFFSET(vftbl_t, baseval),
										REG_ITMP2);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
				codegen_threadcritstop(cd, cd->mcodeptr - cd->mcodebase);
#endif
				x86_64_alu_reg_reg(cd, X86_64_SUB, REG_ITMP2, REG_ITMP1);
				x86_64_alu_reg_reg(cd, X86_64_XOR, d, d); /* may be REG_ITMP2 */
				x86_64_alu_reg_reg(cd, X86_64_CMP, REG_ITMP3, REG_ITMP1);
				x86_64_setcc_reg(cd, X86_64_CC_BE, d);
			}
  			store_reg_to_var_int(iptr->dst, d);
			}
			break;

		case ICMD_CHECKASIZE:  /* ..., size ==> ..., size                     */

			if (src->flags & INMEMORY) {
				x86_64_alul_imm_membase(cd, X86_64_CMP, 0, REG_SP, src->regoff * 8);
				
			} else {
				x86_64_testl_reg_reg(cd, src->regoff, src->regoff);
			}
			x86_64_jcc(cd, X86_64_CC_L, 0);
			codegen_addxcheckarefs(cd, cd->mcodeptr);
			break;

		case ICMD_CHECKEXCEPTION:    /* ... ==> ...                           */

			x86_64_test_reg_reg(cd, REG_RESULT, REG_RESULT);
			x86_64_jcc(cd, X86_64_CC_E, 0);
			codegen_addxexceptionrefs(cd, cd->mcodeptr);
			break;

		case ICMD_MULTIANEWARRAY:/* ..., cnt1, [cnt2, ...] ==> ..., arrayref  */
		                         /* op1 = dimension, val.a = array descriptor */

			/* check for negative sizes and copy sizes to stack if necessary  */

  			MCODECHECK((iptr->op1 << 1) + 64);

			for (s1 = iptr->op1; --s1 >= 0; src = src->prev) {
				var_to_reg_int(s2, src, REG_ITMP1);
				x86_64_testl_reg_reg(cd, s2, s2);
				x86_64_jcc(cd, X86_64_CC_L, 0);
				codegen_addxcheckarefs(cd, cd->mcodeptr);

				/* copy SAVEDVAR sizes to stack */

				if (src->varkind != ARGVAR) {
					x86_64_mov_reg_membase(cd, s2, REG_SP, s1 * 8);
				}
			}

			/* a0 = dimension count */
			x86_64_mov_imm_reg(cd, iptr->op1, rd->argintregs[0]);

			/* a1 = arrayvftbl */
			x86_64_mov_imm_reg(cd, (ptrint) iptr->val.a, rd->argintregs[1]);

			/* a2 = pointer to dimensions = stack pointer */
			x86_64_mov_reg_reg(cd, REG_SP, rd->argintregs[2]);

			/* contains the correct function to call (from parse.c) */
			x86_64_mov_imm_reg(cd, (ptrint) iptr->target, REG_ITMP1);
			x86_64_call_reg(cd, REG_ITMP1);

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
	MCODECHECK(64 + len);
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
					x86_64_movq_reg_membase(cd, s1, REG_SP, rd->interfaces[len][s2].regoff * 8);
				}

			} else {
				var_to_reg_int(s1, src, REG_ITMP1);
				if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
					M_INTMOVE(s1, rd->interfaces[len][s2].regoff);

				} else {
					x86_64_mov_reg_membase(cd, s1, REG_SP, rd->interfaces[len][s2].regoff * 8);
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
		x86_64_mov_reg_reg(cd, bref->reg, REG_ITMP1);             /* 3 bytes  */

		x86_64_mov_imm_reg(cd, 0, REG_ITMP2_XPC);                 /* 10 bytes */
		dseg_adddata(cd, cd->mcodeptr);
		x86_64_mov_imm_reg(cd, bref->branchpos - 6, REG_ITMP3);   /* 10 bytes */
		x86_64_alu_reg_reg(cd, X86_64_ADD, REG_ITMP3, REG_ITMP2_XPC); /* 3 bytes  */

		if (xcodeptr != NULL) {
			x86_64_jmp_imm(cd, xcodeptr - cd->mcodeptr - 5);

		} else {
			xcodeptr = cd->mcodeptr;


			/*create stackinfo -- begin*/
			x86_64_alu_imm_reg(cd, X86_64_SUB, 4 * 8, REG_SP);
			x86_64_mov_reg_membase(cd, REG_ITMP2_XPC, REG_SP, 3 * 8);
			x86_64_mov_imm_membase(cd,0,REG_SP,2*8);
			x86_64_mov_imm_membase(cd,0,REG_SP,1*8);
			x86_64_mov_imm_reg(cd,(ptrint)asm_prepare_native_stackinfo,REG_ITMP3);
			x86_64_call_reg(cd,REG_ITMP3);
			/*create stackinfo -- end*/

			x86_64_mov_reg_reg(cd, REG_ITMP1, rd->argintregs[0]);
			x86_64_mov_imm_reg(cd, (ptrint) new_arrayindexoutofboundsexception, REG_ITMP3);
			x86_64_call_reg(cd, REG_ITMP3);

			/*remove stackinfo -- begin*/
			x86_64_mov_imm_reg(cd,(ptrint)asm_remove_native_stackinfo,REG_ITMP3);
			x86_64_call_reg(cd,REG_ITMP3);
			/*remove stackinfo -- end*/

			x86_64_mov_membase_reg(cd, REG_SP, 1 * 8, REG_ITMP2_XPC);
			x86_64_alu_imm_reg(cd, X86_64_ADD, 2 * 8, REG_SP);

			x86_64_mov_imm_reg(cd, (ptrint) asm_handle_exception, REG_ITMP3);
			x86_64_jmp_reg(cd, REG_ITMP3);
		}
	}

	/* generate negative array size check stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xcheckarefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch(cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  xcodeptr - cd->mcodebase - (10 + 10 + 3));
			continue;
		}

		gen_resolvebranch(cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  cd->mcodeptr - cd->mcodebase);

		MCODECHECK(100);

		x86_64_mov_imm_reg(cd, 0, REG_ITMP2_XPC);                 /* 10 bytes */
		dseg_adddata(cd, cd->mcodeptr);
		x86_64_mov_imm_reg(cd, bref->branchpos - 6, REG_ITMP3);   /* 10 bytes */
		x86_64_alu_reg_reg(cd, X86_64_ADD, REG_ITMP3, REG_ITMP2_XPC); /* 3 bytes  */

		if (xcodeptr != NULL) {
			x86_64_jmp_imm(cd, xcodeptr - cd->mcodeptr - 5);

		} else {
			xcodeptr = cd->mcodeptr;


			/*create stackinfo -- begin*/
			x86_64_alu_imm_reg(cd, X86_64_SUB, 4 * 8, REG_SP);
			x86_64_mov_reg_membase(cd, REG_ITMP2_XPC, REG_SP, 3 * 8);
			x86_64_mov_imm_membase(cd,0,REG_SP,2*8);
			x86_64_mov_imm_membase(cd,0,REG_SP,1*8);
			x86_64_mov_imm_reg(cd,(ptrint)asm_prepare_native_stackinfo,REG_ITMP3);
			x86_64_call_reg(cd,REG_ITMP3);
			/*create stackinfo -- end*/

			x86_64_mov_imm_reg(cd, (u8) new_negativearraysizeexception, REG_ITMP3);
			x86_64_call_reg(cd, REG_ITMP3);

			/*remove stackinfo -- begin*/
			x86_64_mov_imm_reg(cd,(ptrint)asm_remove_native_stackinfo,REG_ITMP3);
			x86_64_call_reg(cd,REG_ITMP3);
			/*remove stackinfo -- end*/

			x86_64_mov_membase_reg(cd, REG_SP, 1 * 8, REG_ITMP2_XPC);
			x86_64_alu_imm_reg(cd, X86_64_ADD, 2 * 8, REG_SP);

			x86_64_mov_imm_reg(cd, (u8) asm_handle_exception, REG_ITMP3);
			x86_64_jmp_reg(cd, REG_ITMP3);
		}
	}

	/* generate cast check stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xcastrefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch(cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  xcodeptr - cd->mcodebase - (10 + 10 + 3));
			continue;
		}

		gen_resolvebranch(cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  cd->mcodeptr - cd->mcodebase);

		MCODECHECK(100);

		x86_64_mov_imm_reg(cd, 0, REG_ITMP2_XPC);                 /* 10 bytes */
		dseg_adddata(cd, cd->mcodeptr);
		x86_64_mov_imm_reg(cd, bref->branchpos - 6, REG_ITMP3);   /* 10 bytes */
		x86_64_alu_reg_reg(cd, X86_64_ADD, REG_ITMP3, REG_ITMP2_XPC); /* 3 bytes  */

		if (xcodeptr != NULL) {
			x86_64_jmp_imm(cd, xcodeptr - cd->mcodeptr - 5);
		
		} else {
			xcodeptr = cd->mcodeptr;

			/*create stackinfo -- begin*/
			x86_64_alu_imm_reg(cd, X86_64_SUB, 4 * 8, REG_SP);
			x86_64_mov_reg_membase(cd, REG_ITMP2_XPC, REG_SP, 3 * 8);
			x86_64_mov_imm_membase(cd,0,REG_SP,2*8);
			x86_64_mov_imm_membase(cd,0,REG_SP,1*8);
			x86_64_mov_imm_reg(cd,(ptrint)asm_prepare_native_stackinfo,REG_ITMP3);
			x86_64_call_reg(cd,REG_ITMP3);
			/*create stackinfo -- end*/


			x86_64_mov_imm_reg(cd, (u8) new_classcastexception, REG_ITMP3);
			x86_64_call_reg(cd, REG_ITMP3);

			/*remove stackinfo -- begin*/
			x86_64_mov_imm_reg(cd,(ptrint)asm_remove_native_stackinfo,REG_ITMP3);
			x86_64_call_reg(cd,REG_ITMP3);
			/*remove stackinfo -- end*/

			x86_64_mov_membase_reg(cd, REG_SP, 1 * 8, REG_ITMP2_XPC);
			x86_64_alu_imm_reg(cd, X86_64_ADD, 2 * 8, REG_SP);

			x86_64_mov_imm_reg(cd, (u8) asm_handle_exception, REG_ITMP3);
			x86_64_jmp_reg(cd, REG_ITMP3);
		}
	}

	/* generate divide by zero check stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xdivrefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch(cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  xcodeptr - cd->mcodebase - (10 + 10 + 3));
			continue;
		}

		gen_resolvebranch(cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  cd->mcodeptr - cd->mcodebase);

		MCODECHECK(100);

		x86_64_mov_imm_reg(cd, 0, REG_ITMP2_XPC);                 /* 10 bytes */
		dseg_adddata(cd, cd->mcodeptr);
		x86_64_mov_imm_reg(cd, bref->branchpos - 6, REG_ITMP3);   /* 10 bytes */
		x86_64_alu_reg_reg(cd, X86_64_ADD, REG_ITMP3, REG_ITMP2_XPC); /* 3 bytes  */

		if (xcodeptr != NULL) {
			x86_64_jmp_imm(cd, xcodeptr - cd->mcodeptr - 5);
		
		} else {
			xcodeptr = cd->mcodeptr;

			/*create stackinfo -- begin*/
			x86_64_alu_imm_reg(cd, X86_64_SUB, 4 * 8, REG_SP);
			x86_64_mov_reg_membase(cd, REG_ITMP2_XPC, REG_SP, 3 * 8);
			x86_64_mov_imm_membase(cd,0,REG_SP,2*8);
			x86_64_mov_imm_membase(cd,0,REG_SP,1*8);
			x86_64_mov_imm_reg(cd,(ptrint)asm_prepare_native_stackinfo,REG_ITMP3);
			x86_64_call_reg(cd,REG_ITMP3);
			/*create stackinfo -- end*/

			x86_64_mov_imm_reg(cd, (u8) new_arithmeticexception, REG_ITMP3);
			x86_64_call_reg(cd, REG_ITMP3);

			/*remove stackinfo -- begin*/
			x86_64_mov_imm_reg(cd,(ptrint)asm_remove_native_stackinfo,REG_ITMP3);
			x86_64_call_reg(cd,REG_ITMP3);
			/*remove stackinfo -- end*/

			x86_64_mov_membase_reg(cd, REG_SP, 1 * 8, REG_ITMP2_XPC);
			x86_64_alu_imm_reg(cd, X86_64_ADD, 2 * 8, REG_SP);

			x86_64_mov_imm_reg(cd, (u8) asm_handle_exception, REG_ITMP3);
			x86_64_jmp_reg(cd, REG_ITMP3);
		}
	}

	/* generate exception check stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xexceptionrefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch(cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  xcodeptr - cd->mcodebase - (10 + 10 + 3));
			continue;
		}

		gen_resolvebranch(cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  cd->mcodeptr - cd->mcodebase);

		MCODECHECK(100);

		x86_64_mov_imm_reg(cd, 0, REG_ITMP2_XPC);                 /* 10 bytes */
		dseg_adddata(cd, cd->mcodeptr);
		x86_64_mov_imm_reg(cd, bref->branchpos - 6, REG_ITMP1);   /* 10 bytes */
		x86_64_alu_reg_reg(cd, X86_64_ADD, REG_ITMP1, REG_ITMP2_XPC); /* 3 bytes  */

		if (xcodeptr != NULL) {
			x86_64_jmp_imm(cd, xcodeptr - cd->mcodeptr - 5);
		
		} else {
			xcodeptr = cd->mcodeptr;



			x86_64_alu_imm_reg(cd, X86_64_SUB, 4*8, REG_SP);
			x86_64_mov_reg_membase(cd, REG_ITMP2_XPC, REG_SP, 3*8);
			x86_64_mov_imm_membase(cd, 0, REG_SP, 2*8);
			x86_64_mov_imm_membase(cd, 0, REG_SP, 1*8);
			x86_64_mov_imm_membase(cd, 0, REG_SP, 0*8);
			x86_64_mov_imm_reg(cd,(u8) asm_prepare_native_stackinfo,REG_ITMP1);
			x86_64_call_reg(cd,REG_ITMP1);
			

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			x86_64_mov_imm_reg(cd, (u8) &builtin_get_exceptionptrptr, REG_ITMP1);
			x86_64_call_reg(cd, REG_ITMP1);
			x86_64_mov_membase_reg(cd, REG_RESULT, 0, REG_ITMP3);
			x86_64_mov_imm_membase(cd, 0, REG_RESULT, 0);
			x86_64_mov_reg_reg(cd, REG_ITMP3, REG_ITMP1_XPTR);
#else
			x86_64_mov_imm_reg(cd, (u8) &_exceptionptr, REG_ITMP3);
			x86_64_mov_membase_reg(cd, REG_ITMP3, 0, REG_ITMP1_XPTR);
			x86_64_mov_imm_membase(cd, 0, REG_ITMP3, 0);
#endif
			x86_64_mov_reg_reg(cd,REG_ITMP1_XPTR,RDI);
			x86_64_mov_imm_reg(cd,(u8) helper_fillin_stacktrace_always,REG_ITMP1);
			x86_64_call_reg(cd,REG_ITMP1);
			x86_64_mov_reg_reg(cd,REG_RESULT,REG_ITMP1_XPTR);

			x86_64_mov_imm_reg(cd,(u8) asm_remove_native_stackinfo,REG_ITMP2);
			x86_64_call_reg(cd,REG_ITMP2);
			
			x86_64_alu_imm_reg(cd, X86_64_ADD, 8, REG_SP);
			x86_64_mov_membase_reg(cd, REG_SP, 0, REG_ITMP2_XPC);
			x86_64_alu_imm_reg(cd, X86_64_ADD, 8, REG_SP);


			x86_64_mov_imm_reg(cd, (u8) asm_handle_exception, REG_ITMP3);
			x86_64_jmp_reg(cd, REG_ITMP3);
		}
	}

	/* generate null pointer check stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xnullrefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch(cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  xcodeptr - cd->mcodebase - (10 + 10 + 3));
			continue;
		}

		gen_resolvebranch(cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  cd->mcodeptr - cd->mcodebase);

		MCODECHECK(100);

		x86_64_mov_imm_reg(cd, 0, REG_ITMP2_XPC);                 /* 10 bytes */
		dseg_adddata(cd, cd->mcodeptr);
		x86_64_mov_imm_reg(cd, bref->branchpos - 6, REG_ITMP1);   /* 10 bytes */
		x86_64_alu_reg_reg(cd, X86_64_ADD, REG_ITMP1, REG_ITMP2_XPC); /* 3 bytes  */

		if (xcodeptr != NULL) {
			x86_64_jmp_imm(cd, xcodeptr - cd->mcodeptr - 5);
		
		} else {
			xcodeptr = cd->mcodeptr;

			/*create stackinfo -- begin*/
			x86_64_alu_imm_reg(cd, X86_64_SUB, 4 * 8, REG_SP);
			x86_64_mov_reg_membase(cd, REG_ITMP2_XPC, REG_SP, 3 * 8);
			x86_64_mov_imm_membase(cd,0,REG_SP,2*8);
			x86_64_mov_imm_membase(cd,0,REG_SP,1*8);
			x86_64_mov_imm_reg(cd,(ptrint)asm_prepare_native_stackinfo,REG_ITMP3);
			x86_64_call_reg(cd,REG_ITMP3);
			/*create stackinfo -- end*/


			x86_64_mov_imm_reg(cd, (ptrint) new_nullpointerexception, REG_ITMP3);
			x86_64_call_reg(cd, REG_ITMP3);

			/*remove stackinfo -- begin*/
			x86_64_mov_imm_reg(cd,(ptrint)asm_remove_native_stackinfo,REG_ITMP3);
			x86_64_call_reg(cd,REG_ITMP3);
			/*remove stackinfo -- end*/

			x86_64_mov_membase_reg(cd, REG_SP, 1 * 8, REG_ITMP2_XPC);
			x86_64_alu_imm_reg(cd, X86_64_ADD, 2 * 8, REG_SP);

			x86_64_mov_imm_reg(cd, (ptrint) asm_handle_exception, REG_ITMP3);
			x86_64_jmp_reg(cd, REG_ITMP3);
		}
	}

	/* generate code patching stub call code */

	{
		patchref    *pref;
		codegendata *tmpcd;
		ptrint       mcode;

		tmpcd = DNEW(codegendata);

		for (pref = cd->patchrefs; pref != NULL; pref = pref->next) {
			MCODECHECK(64);

			/* Get machine code which is patched back in later. A             */
			/* `call rel32' is 5 bytes long (but read 8 bytes).               */
			xcodeptr = cd->mcodebase + pref->branchpos;
			mcode = *((ptrint *) xcodeptr);

			/* patch in `call rel32' to call the following code               */
			tmpcd->mcodeptr = xcodeptr;     /* set dummy mcode pointer        */
			x86_64_call_imm(tmpcd, cd->mcodeptr - (xcodeptr + 5));

			/* move machine code bytes and classinfo pointer into registers */
			x86_64_mov_imm_reg(cd, (ptrint) mcode, REG_ITMP3);
			x86_64_push_reg(cd, REG_ITMP3);
			x86_64_mov_imm_reg(cd, (ptrint) pref->ref, REG_ITMP3);
			x86_64_push_reg(cd, REG_ITMP3);

			x86_64_mov_imm_reg(cd, (ptrint) pref->patcher, REG_ITMP3);
			x86_64_push_reg(cd, REG_ITMP3);

			x86_64_mov_imm_reg(cd, (ptrint) asm_wrapper_patcher, REG_ITMP3);
			x86_64_jmp_reg(cd, REG_ITMP3);
		}
	}
	}

	codegen_finish(m, cd, (s4) ((u1 *) cd->mcodeptr - cd->mcodebase));
}


/* function createcompilerstub *************************************************

	creates a stub routine which calls the compiler
	
*******************************************************************************/

#define COMPSTUBSIZE    23

u1 *createcompilerstub(methodinfo *m)
{
	u1 *s = CNEW(u1, COMPSTUBSIZE);     /* memory to hold the stub            */
	codegendata *cd;
	s4 dumpsize;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	cd = DNEW(codegendata);
	cd->mcodeptr = s;

	/* code for the stub */
	x86_64_mov_imm_reg(cd, (u8) m, REG_ITMP1); /* pass method to compiler     */
	x86_64_mov_imm_reg(cd, (u8) asm_call_jit_compiler, REG_ITMP3);/* load address */
	x86_64_jmp_reg(cd, REG_ITMP3);      /* jump to compiler                   */

#if defined(STATISTICS)
	if (opt_stat)
		count_cstub_len += COMPSTUBSIZE;
#endif

	/* release dump area */

	dump_release(dumpsize);

	return s;
}


/* function removecompilerstub *************************************************

     deletes a compilerstub from memory  (simply by freeing it)

*******************************************************************************/

void removecompilerstub(u1 *stub) 
{
	CFREE(stub, COMPSTUBSIZE);
}


/* function: createnativestub **************************************************

	creates a stub routine which calls a native method

*******************************************************************************/

/* #if defined(USE_THREADS) && defined(NATIVE_THREADS) */
/* static java_objectheader **(*callgetexceptionptrptr)() = builtin_get_exceptionptrptr; */
/* #endif */

#define NATIVESTUBSIZE    800           /* keep this size high enough!        */

u1 *createnativestub(functionptr f, methodinfo *m)
{
	u1                 *s;              /* pointer to stub memory             */
	codegendata        *cd;
	registerdata       *rd;
	t_inlining_globals *id;
	s4                  dumpsize;
	s4                  stackframesize; /* size of stackframe if needed       */
	u1                 *tptr;
	s4                  iargs;          /* count of integer arguments         */
	s4                  fargs;          /* count of float arguments           */
	s4                  i;              /* counter                            */

	void **callAddrPatchPos=0;
	u1 *jmpInstrPos=0;
	void **jmpInstrPatchPos=0;

	/* initialize variables */

	iargs = 0;
	fargs = 0;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	cd = DNEW(codegendata);
	rd = DNEW(registerdata);
	id = DNEW(t_inlining_globals);

	/* setup registers before using it */

	inlining_setup(m, id);
	reg_setup(m, rd, id);

	/* set paramcount and paramtypes      */

	method_descriptor2types(m);

	/* count integer and float arguments */

	tptr = m->paramtypes;
	for (i = 0; i < m->paramcount; i++) {
		IS_INT_LNG_TYPE(*tptr++) ? iargs++ : fargs++;
	}

	s = CNEW(u1, NATIVESTUBSIZE);       /* memory to hold the stub            */

	/* set some required varibles which are normally set by codegen_setup */
	cd->mcodebase = s;
	cd->mcodeptr = s;
	cd->patchrefs = NULL;

	/* if function is static, check for initialized */

	if ((m->flags & ACC_STATIC) && !m->class->initialized) {
		codegen_addpatchref(cd, cd->mcodeptr, PATCHER_clinit, m->class);
	}

	if (runverbose) {
		s4 l, s1;

		x86_64_alu_imm_reg(cd, X86_64_SUB, (INT_ARG_CNT + FLT_ARG_CNT + 1) * 8, REG_SP);

		/* save integer and float argument registers */

		for (i = 0; i < INT_ARG_CNT; i++) {
			x86_64_mov_reg_membase(cd, rd->argintregs[i], REG_SP, (1 + i) * 8);
		}

		for (i = 0; i < FLT_ARG_CNT; i++) {
			x86_64_movq_reg_membase(cd, rd->argfltregs[i], REG_SP, (1 + INT_ARG_CNT + i) * 8);
		}

		/* show integer hex code for float arguments */

		for (i = 0, l = 0; i < m->paramcount && i < INT_ARG_CNT; i++) {
			/* if the paramtype is a float, we have to right shift all        */
			/* following integer registers                                    */

			if (IS_FLT_DBL_TYPE(m->paramtypes[i])) {
				for (s1 = INT_ARG_CNT - 2; s1 >= i; s1--) {
					x86_64_mov_reg_reg(cd, rd->argintregs[s1], rd->argintregs[s1 + 1]);
				}

				x86_64_movd_freg_reg(cd, rd->argfltregs[l], rd->argintregs[i]);
				l++;
			}
		}

		x86_64_mov_imm_reg(cd, (u8) m, REG_ITMP1);
		x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, 0 * 8);
		x86_64_mov_imm_reg(cd, (u8) builtin_trace_args, REG_ITMP1);
		x86_64_call_reg(cd, REG_ITMP1);

		/* restore integer and float argument registers */

		for (i = 0; i < INT_ARG_CNT; i++) {
			x86_64_mov_membase_reg(cd, REG_SP, (1 + i) * 8, rd->argintregs[i]);
		}

		for (i = 0; i < FLT_ARG_CNT; i++) {
			x86_64_movq_membase_reg(cd, REG_SP, (1 + INT_ARG_CNT + i) * 8, rd->argfltregs[i]);
		}

		x86_64_alu_imm_reg(cd, X86_64_ADD, (INT_ARG_CNT + FLT_ARG_CNT + 1) * 8, REG_SP);
	}

	/* 4 == additional size needed for native stack frame information*/
	x86_64_alu_imm_reg(cd, X86_64_SUB, (4+INT_ARG_CNT + FLT_ARG_CNT+1) * 8, REG_SP);

	x86_64_mov_reg_membase(cd, rd->argintregs[0], REG_SP, 0 * 8);
	x86_64_mov_reg_membase(cd, rd->argintregs[1], REG_SP, 1 * 8);
	x86_64_mov_reg_membase(cd, rd->argintregs[2], REG_SP, 2 * 8);
	x86_64_mov_reg_membase(cd, rd->argintregs[3], REG_SP, 3 * 8);
	x86_64_mov_reg_membase(cd, rd->argintregs[4], REG_SP, 4 * 8);
	x86_64_mov_reg_membase(cd, rd->argintregs[5], REG_SP, 5 * 8);

	x86_64_movq_reg_membase(cd, rd->argfltregs[0], REG_SP, 6 * 8);
	x86_64_movq_reg_membase(cd, rd->argfltregs[1], REG_SP, 7 * 8);
	x86_64_movq_reg_membase(cd, rd->argfltregs[2], REG_SP, 8 * 8);
	x86_64_movq_reg_membase(cd, rd->argfltregs[3], REG_SP, 9 * 8);
	x86_64_movq_reg_membase(cd, rd->argfltregs[4], REG_SP, 10 * 8);
	x86_64_movq_reg_membase(cd, rd->argfltregs[5], REG_SP, 11 * 8);
	x86_64_movq_reg_membase(cd, rd->argfltregs[6], REG_SP, 12 * 8);
	x86_64_movq_reg_membase(cd, rd->argfltregs[7], REG_SP, 13 * 8);

/*
15+
0*8        void *oldThreadspecificHeadValue;
1*8        void **addressOfThreadspecificHead;
2*8        methodinfo *method;
3*8        void *beginOfJavaStackframe; only used if != 0
4*8        void *returnToFromNative;
*/

	/* CREATE DYNAMIC STACK INFO -- BEGIN   offsets:15,16,17,18*/
	x86_64_mov_imm_membase(cd, 0, REG_SP, 18*8);
	x86_64_mov_imm_membase(cd, (u8)m, REG_SP, 17*8);

	x86_64_mov_imm_reg(cd, (u8) builtin_asm_get_stackframeinfo,REG_ITMP1);
	x86_64_call_reg(cd,REG_ITMP1);

	x86_64_mov_reg_membase(cd,REG_RESULT,REG_SP,16*8);
	x86_64_mov_membase_reg(cd,REG_RESULT,0,REG_ITMP2);
	x86_64_mov_reg_membase(cd,REG_ITMP2,REG_SP,15*8);
	x86_64_mov_reg_reg(cd,REG_SP,REG_ITMP2);
	x86_64_alu_imm_reg(cd, X86_64_ADD, (1+INT_ARG_CNT + FLT_ARG_CNT) * 8, REG_ITMP2);
	x86_64_mov_reg_membase(cd,REG_ITMP2,REG_RESULT,0);

#if 0

        i386_mov_imm_membase(cd,0,REG_SP,stackframesize-4);
        i386_mov_imm_membase(cd, (s4) m, REG_SP,stackframesize-8);
        i386_mov_imm_reg(cd, (s4) builtin_asm_get_stackframeinfo, REG_ITMP1);
        i386_call_reg(cd, REG_ITMP1);
        i386_mov_reg_membase(cd, REG_RESULT,REG_SP,stackframesize-12); /*save thread specific pointer*/
        i386_mov_membase_reg(cd, REG_RESULT,0,REG_ITMP2);
        i386_mov_reg_membase(cd, REG_ITMP2,REG_SP,stackframesize-16); /*save previous value of memory adress pointed to by thread specific pointer*/
        i386_mov_reg_reg(cd, REG_SP,REG_ITMP2);
        i386_alu_imm_reg(cd, I386_ADD,stackframesize-16,REG_ITMP2);
        i386_mov_reg_membase(cd, REG_ITMP2,REG_RESULT,0);
#endif
	/* CREATE DYNAMIC STACK INFO -- END*/


#if !defined(STATIC_CLASSPATH)
	/* call method to resolve native function if needed */
	if (f == NULL) {
		/* needed to patch a jump over this block */
		x86_64_jmp_imm(cd, 0);
		jmpInstrPos = cd->mcodeptr - 4;


		x86_64_mov_imm_reg(cd, (u8) m, rd->argintregs[0]);

		x86_64_mov_imm_reg(cd, 0, rd->argintregs[1]);
		callAddrPatchPos = cd->mcodeptr - 8; /* at this position the place is specified where the native function adress should be patched into*/

		x86_64_mov_imm_reg(cd, 0, rd->argintregs[2]);
		jmpInstrPatchPos = cd->mcodeptr - 8;

		x86_64_mov_imm_reg(cd, jmpInstrPos, rd->argintregs[3]);

		x86_64_mov_imm_reg(cd, (u8) codegen_resolve_native, REG_ITMP1);
		x86_64_call_reg(cd, REG_ITMP1);

		*(jmpInstrPatchPos) = cd->mcodeptr - jmpInstrPos - 1-3; /*=opcode jmp_imm size*/

	}

#endif


	x86_64_mov_membase_reg(cd, REG_SP, 0 * 8, rd->argintregs[0]);
	x86_64_mov_membase_reg(cd, REG_SP, 1 * 8, rd->argintregs[1]);
	x86_64_mov_membase_reg(cd, REG_SP, 2 * 8, rd->argintregs[2]);
	x86_64_mov_membase_reg(cd, REG_SP, 3 * 8, rd->argintregs[3]);
	x86_64_mov_membase_reg(cd, REG_SP, 4 * 8, rd->argintregs[4]);
	x86_64_mov_membase_reg(cd, REG_SP, 5 * 8, rd->argintregs[5]);

	x86_64_movq_membase_reg(cd, REG_SP, 6 * 8, rd->argfltregs[0]);
	x86_64_movq_membase_reg(cd, REG_SP, 7 * 8, rd->argfltregs[1]);
	x86_64_movq_membase_reg(cd, REG_SP, 8 * 8, rd->argfltregs[2]);
	x86_64_movq_membase_reg(cd, REG_SP, 9 * 8, rd->argfltregs[3]);
	x86_64_movq_membase_reg(cd, REG_SP, 10 * 8, rd->argfltregs[4]);
	x86_64_movq_membase_reg(cd, REG_SP, 11 * 8, rd->argfltregs[5]);
	x86_64_movq_membase_reg(cd, REG_SP, 12 * 8, rd->argfltregs[6]);
	x86_64_movq_membase_reg(cd, REG_SP, 13 * 8, rd->argfltregs[7]);

	x86_64_alu_imm_reg(cd, X86_64_ADD, (INT_ARG_CNT + FLT_ARG_CNT+1) * 8, REG_SP);

	/* save argument registers on stack -- if we have to */

	if ((((m->flags & ACC_STATIC) && iargs > (INT_ARG_CNT - 2)) || iargs > (INT_ARG_CNT - 1)) ||
		(fargs > FLT_ARG_CNT)) {
		s4 paramshiftcnt;
		s4 stackparamcnt;

		paramshiftcnt = 0;
		stackparamcnt = 0;

		/* do we need to shift integer argument register onto stack? */

		if ((m->flags & ACC_STATIC) && iargs > (INT_ARG_CNT - 2)) {
			/* do we need to shift 2 arguments? */
			if (iargs > (INT_ARG_CNT - 1)) {
				paramshiftcnt = 2;

			} else {
				paramshiftcnt = 1;
			}

		} else if (iargs > (INT_ARG_CNT - 1)) {
			paramshiftcnt = 1;
		}

		/* calculate required stack space */

		stackparamcnt += (iargs > INT_ARG_CNT) ? iargs - INT_ARG_CNT : 0;
		stackparamcnt += (fargs > FLT_ARG_CNT) ? fargs - FLT_ARG_CNT : 0;


		stackframesize = stackparamcnt + paramshiftcnt;

		/* keep stack 16-byte aligned */
		if (!(stackframesize & 0x1))
			stackframesize++;

		x86_64_alu_imm_reg(cd, X86_64_SUB, stackframesize * 8, REG_SP);

		/* shift integer arguments if required */

		if ((m->flags & ACC_STATIC) && iargs > (INT_ARG_CNT - 2)) {
			/* do we need to shift 2 arguments? */
			if (iargs > (INT_ARG_CNT - 1))
				x86_64_mov_reg_membase(cd, rd->argintregs[5], REG_SP, 1 * 8);

			x86_64_mov_reg_membase(cd, rd->argintregs[4], REG_SP, 0 * 8);

		} else if (iargs > (INT_ARG_CNT - 1)) {
			x86_64_mov_reg_membase(cd, rd->argintregs[5], REG_SP, 0 * 8);
		}

		/* copy stack arguments into new stack frame -- if any */
		for (i = 0; i < stackparamcnt; i++) {
			x86_64_mov_membase_reg(cd, REG_SP, (stackframesize + 1 + i+4) * 8, REG_ITMP1);  /* 4==additional size for stackrace data*/
			x86_64_mov_reg_membase(cd, REG_ITMP1, REG_SP, (paramshiftcnt + i) * 8);
		}

	} else {
		/* keep stack 16-byte aligned */
		x86_64_alu_imm_reg(cd, X86_64_SUB, 1 * 8, REG_SP);
		stackframesize = 1;
	}

	/* shift integer arguments for `env' and `class' arguments */

	if (m->flags & ACC_STATIC) {
		/* shift iargs count if less than INT_ARG_CNT, or all */
		for (i = (iargs < (INT_ARG_CNT - 2)) ? iargs : (INT_ARG_CNT - 2); i >= 0; i--) {
			x86_64_mov_reg_reg(cd, rd->argintregs[i], rd->argintregs[i + 2]);
		}

		/* put class into second argument register */
		x86_64_mov_imm_reg(cd, (u8) m->class, rd->argintregs[1]);

	} else {
		/* shift iargs count if less than INT_ARG_CNT, or all */
		for (i = (iargs < (INT_ARG_CNT - 1)) ? iargs : (INT_ARG_CNT - 1); i >= 0; i--) {
			x86_64_mov_reg_reg(cd, rd->argintregs[i], rd->argintregs[i + 1]);
		}
	}

	/* put env into first argument register */
	x86_64_mov_imm_reg(cd, (u8) &env, rd->argintregs[0]);

	/* do the native function call */
	x86_64_mov_imm_reg(cd, (u8) f, REG_ITMP1);
#if !defined(STATIC_CLASSPATH)
	if (f == NULL)
		(*callAddrPatchPos) = cd->mcodeptr - 8;
#endif
	x86_64_call_reg(cd, REG_ITMP1);

	/* remove stackframe if there is one */
	if (stackframesize) {
		x86_64_alu_imm_reg(cd, X86_64_ADD, stackframesize * 8, REG_SP);
	}

	/*REMOVE DYNAMIC STACK INFO -BEGIN */
	x86_64_mov_reg_membase(cd,REG_RESULT,REG_SP,2*8);
	x86_64_mov_membase_reg(cd,REG_SP,0*8,REG_ITMP2);
	x86_64_mov_membase_reg(cd,REG_SP,1*8,REG_RESULT);
	x86_64_mov_reg_membase(cd,REG_ITMP2,REG_RESULT,0);
	x86_64_mov_membase_reg(cd,REG_SP,2*8,REG_RESULT);
#if 0
	i386_push_reg(cd, REG_RESULT2);
	i386_mov_membase_reg(cd, REG_SP,stackframesize-12,REG_ITMP2); /*old value*/
	i386_mov_membase_reg(cd, REG_SP,stackframesize-8,REG_RESULT2); /*pointer*/
	i386_mov_reg_membase(cd, REG_ITMP2,REG_RESULT2,0);
	i386_pop_reg(cd, REG_RESULT2);
#endif
	/*REMOVE DYNAMIC STACK INFO -END */

	x86_64_alu_imm_reg(cd, X86_64_ADD, 4 * 8, REG_SP);

	if (runverbose) {
		x86_64_alu_imm_reg(cd, X86_64_SUB, 3 * 8, REG_SP);    /* keep stack 16-byte aligned */

		x86_64_mov_reg_membase(cd, REG_RESULT, REG_SP, 0 * 8);
		x86_64_movq_reg_membase(cd, REG_FRESULT, REG_SP, 1 * 8);

  		x86_64_mov_imm_reg(cd, (u8) m, rd->argintregs[0]);
  		x86_64_mov_reg_reg(cd, REG_RESULT, rd->argintregs[1]);
		M_FLTMOVE(REG_FRESULT, rd->argfltregs[0]);
  		M_FLTMOVE(REG_FRESULT, rd->argfltregs[1]);

  		x86_64_mov_imm_reg(cd, (u8) builtin_displaymethodstop, REG_ITMP1);
		x86_64_call_reg(cd, REG_ITMP1);

		x86_64_mov_membase_reg(cd, REG_SP, 0 * 8, REG_RESULT);
		x86_64_movq_membase_reg(cd, REG_SP, 1 * 8, REG_FRESULT);

		x86_64_alu_imm_reg(cd, X86_64_ADD, 3 * 8, REG_SP);    /* keep stack 16-byte aligned */
	}

	/* check for exception */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	x86_64_push_reg(cd, REG_RESULT);
/* 	x86_64_call_mem(cd, (u8) &callgetexceptionptrptr); */
	x86_64_mov_imm_reg(cd, (u8) builtin_get_exceptionptrptr, REG_ITMP3);
	x86_64_call_reg(cd, REG_ITMP3);
	x86_64_mov_membase_reg(cd, REG_RESULT, 0, REG_ITMP3);
	x86_64_pop_reg(cd, REG_RESULT);
#else
	x86_64_mov_imm_reg(cd, (u8) &_exceptionptr, REG_ITMP3);
	x86_64_mov_membase_reg(cd, REG_ITMP3, 0, REG_ITMP3);
#endif
	x86_64_test_reg_reg(cd, REG_ITMP3, REG_ITMP3);
	x86_64_jcc(cd, X86_64_CC_NE, 1);

	x86_64_ret(cd);

	/* handle exception */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	x86_64_push_reg(cd, REG_ITMP3);
/* 	x86_64_call_mem(cd, (u8) &callgetexceptionptrptr); */
	x86_64_mov_imm_reg(cd, (u8) builtin_get_exceptionptrptr, REG_ITMP3);
	x86_64_call_reg(cd, REG_ITMP3);
	x86_64_mov_imm_membase(cd, 0, REG_RESULT, 0);
	x86_64_pop_reg(cd, REG_ITMP1_XPTR);
#else
	x86_64_mov_reg_reg(cd, REG_ITMP3, REG_ITMP1_XPTR);
	x86_64_mov_imm_reg(cd, (u8) &_exceptionptr, REG_ITMP3);
	x86_64_alu_reg_reg(cd, X86_64_XOR, REG_ITMP2, REG_ITMP2);
	x86_64_mov_reg_membase(cd, REG_ITMP2, REG_ITMP3, 0);    /* clear exception pointer */
#endif

	x86_64_mov_membase_reg(cd, REG_SP, 0, REG_ITMP2_XPC);    /* get return address from stack */
	x86_64_alu_imm_reg(cd, X86_64_SUB, 3, REG_ITMP2_XPC);    /* callq */

	x86_64_mov_imm_reg(cd, (u8) asm_handle_nat_exception, REG_ITMP3);
	x86_64_jmp_reg(cd, REG_ITMP3);


	/* patch in a <clinit> call if required ***********************************/

	{
		u1          *xcodeptr;
		patchref    *pref;
		codegendata *tmpcd;
		ptrint       mcode;

		tmpcd = DNEW(codegendata);

		/* there can only be one patch ref entry                              */
		pref = cd->patchrefs;

		if (pref) {
			/* Get machine code which is patched back in later. A             */
			/* `call rel32' is 5 bytes long (but read 8 bytes).               */
			xcodeptr = cd->mcodebase + pref->branchpos;
			mcode = *((ptrint *) xcodeptr);

			/* patch in `call rel32' to call the following code               */
			tmpcd->mcodeptr = xcodeptr;     /* set dummy mcode pointer        */
			x86_64_call_imm(tmpcd, cd->mcodeptr - (xcodeptr + 5));

			/* move machine code bytes and classinfo pointer into registers */
			x86_64_mov_imm_reg(cd, (ptrint) mcode, REG_ITMP3);
			x86_64_push_reg(cd, REG_ITMP3);
			x86_64_mov_imm_reg(cd, (ptrint) pref->ref, REG_ITMP3);
			x86_64_push_reg(cd, REG_ITMP3);

			x86_64_mov_imm_reg(cd, (ptrint) pref->patcher, REG_ITMP3);
			x86_64_push_reg(cd, REG_ITMP3);

			x86_64_mov_imm_reg(cd, (ptrint) asm_wrapper_patcher, REG_ITMP3);
			x86_64_jmp_reg(cd, REG_ITMP3);
		}
	}

	/* Check if the stub size is big enough to hold the whole stub generated. */
	/* If not, this can lead into unpredictable crashes, because of heap      */
	/* corruption.                                                            */
	if ((s4) (cd->mcodeptr - s) > NATIVESTUBSIZE) {
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Native stub size %d is to small for current stub size %d",
								   NATIVESTUBSIZE, (s4) (cd->mcodeptr - s));
	}


#if defined(STATISTICS)
	if (opt_stat)
		count_nstub_len += NATIVESTUBSIZE;
#endif

	/* release dump area */

	dump_release(dumpsize);

	return s;
}


/* function: removenativestub **************************************************

    removes a previously created native-stub from memory
    
*******************************************************************************/

void removenativestub(u1 *stub)
{
	CFREE(stub, NATIVESTUBSIZE);
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
