/* jit/x86_64/codegen.c - machine code generator for x86_64

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   Institut f. Computersprachen, TU Wien
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser, M. Probst,
   S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich,
   J. Wenninger

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

   $Id: codegen.c 1284 2004-07-07 15:56:17Z twisti $

*/


#include <stdio.h>
#include <signal.h>
#include <sys/ucontext.h>
#include "builtin.h"
#include "asmpart.h"
#include "jni.h"
#include "loader.h"
#include "tables.h"
#include "native.h"
#include "jit/jit.h"
#include "jit/reg.h"
#include "jit/parse.h"
#include "jit/x86_64/codegen.h"
#include "jit/x86_64/emitfuncs.h"
#include "jit/x86_64/types.h"

/* include independent code generation stuff */
#include "jit/codegen.inc"
#include "jit/reg.inc"


/* register descripton - array ************************************************/

/* #define REG_RES   0         reserved register for OS or code generator     */
/* #define REG_RET   1         return value register                          */
/* #define REG_EXC   2         exception value register (only old jit)        */
/* #define REG_SAV   3         (callee) saved register                        */
/* #define REG_TMP   4         scratch temporary register (caller saved)      */
/* #define REG_ARG   5         argument register (caller saved)               */

/* #define REG_END   -1        last entry in tables                           */

int nregdescint[] = {
    REG_RET, REG_ARG, REG_ARG, REG_TMP, REG_RES, REG_SAV, REG_ARG, REG_ARG,
    REG_ARG, REG_ARG, REG_RES, REG_RES, REG_SAV, REG_SAV, REG_SAV, REG_SAV,
    REG_END
};


int nregdescfloat[] = {
	/*      REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_TMP, REG_TMP, REG_TMP, REG_TMP, */
	/*      REG_RES, REG_RES, REG_RES, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, */
    REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_TMP, REG_TMP, REG_TMP, REG_TMP,
    REG_RES, REG_RES, REG_RES, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP,
    REG_END
};


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
	/*  	int      instr; */
	/*  	long     faultaddr; */

	struct ucontext *_uc = (struct ucontext *) _p;
	struct sigcontext *sigctx = (struct sigcontext *) &_uc->uc_mcontext;
	struct sigaction act;
	java_objectheader *xptr;

	/* Reset signal handler - necessary for SysV, does no harm for BSD */

	
/*	instr = *((int*)(sigctx->rip)); */
/*    	faultaddr = sigctx->sc_regs[(instr >> 16) & 0x1f]; */

/*  	if (faultaddr == 0) { */
	act.sa_sigaction = (void *) catch_NullPointerException; /* reinstall handler */
	act.sa_flags = SA_SIGINFO;
	sigaction(sig, &act, NULL);
	
	sigemptyset(&nsig);
	sigaddset(&nsig, sig);
	sigprocmask(SIG_UNBLOCK, &nsig, NULL);               /* unblock signal    */

	xptr = new_exception(string_java_lang_NullPointerException);

	sigctx->rax = (u8) xptr;                             /* REG_ITMP1_XPTR    */
	sigctx->r10 = sigctx->rip;                           /* REG_ITMP2_XPC     */
	sigctx->rip = (u8) asm_handle_exception;

	return;

/*  	} else { */
/*  		faultaddr += (long) ((instr << 16) >> 16); */
/*  		fprintf(stderr, "faulting address: 0x%08x\n", faultaddr); */
/*  		panic("Stack overflow"); */
/*  	} */
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

	act.sa_sigaction = (void *) catch_ArithmeticException; /* reinstall handler */
	act.sa_flags = SA_SIGINFO;
	sigaction(sig, &act, NULL);

	sigemptyset(&nsig);
	sigaddset(&nsig, sig);
	sigprocmask(SIG_UNBLOCK, &nsig, NULL);               /* unblock signal    */

	xptr = new_exception_message(string_java_lang_ArithmeticException,
								 string_java_lang_ArithmeticException_message);

	sigctx->rax = (u8) xptr;                             /* REG_ITMP1_XPTR    */
	sigctx->r10 = sigctx->rip;                           /* REG_ITMP2_XPC     */
	sigctx->rip = (u8) asm_handle_exception;

	return;
}


void init_exceptions(void)
{
	struct sigaction act;

	/* install signal handlers we need to convert to exceptions */

	if (!checknull) {
#if defined(SIGSEGV)
		act.sa_sigaction = (void *) catch_NullPointerException;
		act.sa_flags = SA_SIGINFO;
		sigaction(SIGSEGV, &act, NULL);
#endif

#if defined(SIGBUS)
		act.sa_sigaction = (void *) catch_NullPointerException;
		act.sa_flags = SA_SIGINFO;
		sigaction(SIGBUS, &act, NULL);
#endif
	}

	act.sa_sigaction = (void *) catch_ArithmeticException;
	act.sa_flags = SA_SIGINFO;
	sigaction(SIGFPE, &act, NULL);
}


/* function gen_mcode **********************************************************

	generates machine code

*******************************************************************************/

/* global code generation pointer */

u1 *mcodeptr;


void codegen(methodinfo *m)
{
	s4 len, s1, s2, s3, d;
	s8 a;
	stackptr        src;
	varinfo        *var;
	basicblock     *bptr;
	instruction    *iptr;
	exceptiontable *ex;
	registerdata   *r;

	{
	s4 i, p, pa, t, l;
	s4 savedregs_num;

	/* keep code size smaller */
	r = m->registerdata;

  	savedregs_num = 0;

	/* space to save used callee saved registers */

	savedregs_num += (r->savintregcnt - r->maxsavintreguse);
	savedregs_num += (r->savfltregcnt - r->maxsavfltreguse);

	parentargs_base = r->maxmemuse + savedregs_num;

#if defined(USE_THREADS)           /* space to save argument of monitor_enter */

	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		parentargs_base++;

#endif

    /* keep stack 16-byte aligned for calls into libc */

	if (!m->isleafmethod || runverbose) {
		if ((parentargs_base % 2) == 0) {
			parentargs_base++;
		}
	}

	/* create method header */

	(void) dseg_addaddress(m);                              /* MethodPointer  */
	(void) dseg_adds4(parentargs_base * 8);                 /* FrameSize      */

#if defined(USE_THREADS)

	/* IsSync contains the offset relative to the stack pointer for the
	   argument of monitor_exit used in the exception handler. Since the
	   offset could be zero and give a wrong meaning of the flag it is
	   offset by one.
	*/

	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		(void) dseg_adds4((r->maxmemuse + 1) * 8);          /* IsSync         */
	else

#endif

		(void) dseg_adds4(0);                               /* IsSync         */
	                                       
	(void) dseg_adds4(m->isleafmethod);                     /* IsLeaf         */
	(void) dseg_adds4(r->savintregcnt - r->maxsavintreguse);/* IntSave        */
	(void) dseg_adds4(r->savfltregcnt - r->maxsavfltreguse);/* FltSave        */
	(void) dseg_adds4(m->exceptiontablelength);             /* ExTableSize    */

	/* create exception table */

	for (ex = m->exceptiontable; ex != NULL; ex = ex->down) {
		dseg_addtarget(ex->start);
   		dseg_addtarget(ex->end);
		dseg_addtarget(ex->handler);
		(void) dseg_addaddress(ex->catchtype);
	}
	
	/* initialize mcode variables */
	
	mcodeptr = (u1 *) mcodebase;
	mcodeend = (s4 *) (mcodebase + mcodesize);
	MCODECHECK(128 + m->paramcount);

	/* create stack frame (if necessary) */

	if (parentargs_base) {
		x86_64_alu_imm_reg(X86_64_SUB, parentargs_base * 8, REG_SP);
	}

	/* save return address and used callee saved registers */

  	p = parentargs_base;
	for (i = r->savintregcnt - 1; i >= r->maxsavintreguse; i--) {
 		p--; x86_64_mov_reg_membase(r->savintregs[i], REG_SP, p * 8);
	}
	for (i = r->savfltregcnt - 1; i >= r->maxsavfltreguse; i--) {
		p--; x86_64_movq_reg_membase(r->savfltregs[i], REG_SP, p * 8);
	}

	/* save monitorenter argument */

#if defined(USE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		if (m->flags & ACC_STATIC) {
			x86_64_mov_imm_reg((s8) m->class, REG_ITMP1);
			x86_64_mov_reg_membase(REG_ITMP1, REG_SP, r->maxmemuse * 8);

		} else {
			x86_64_mov_reg_membase(r->argintregs[0], REG_SP, r->maxmemuse * 8);
		}
	}			
#endif

	/* copy argument registers to stack and call trace function with pointer
	   to arguments on stack.
	*/
	if (runverbose) {
		x86_64_alu_imm_reg(X86_64_SUB, (6 + 8 + 1 + 1) * 8, REG_SP);

		x86_64_mov_reg_membase(r->argintregs[0], REG_SP, 1 * 8);
		x86_64_mov_reg_membase(r->argintregs[1], REG_SP, 2 * 8);
		x86_64_mov_reg_membase(r->argintregs[2], REG_SP, 3 * 8);
		x86_64_mov_reg_membase(r->argintregs[3], REG_SP, 4 * 8);
		x86_64_mov_reg_membase(r->argintregs[4], REG_SP, 5 * 8);
		x86_64_mov_reg_membase(r->argintregs[5], REG_SP, 6 * 8);

		x86_64_movq_reg_membase(r->argfltregs[0], REG_SP, 7 * 8);
		x86_64_movq_reg_membase(r->argfltregs[1], REG_SP, 8 * 8);
		x86_64_movq_reg_membase(r->argfltregs[2], REG_SP, 9 * 8);
		x86_64_movq_reg_membase(r->argfltregs[3], REG_SP, 10 * 8);
/*  		x86_64_movq_reg_membase(r->argfltregs[4], REG_SP, 11 * 8); */
/*  		x86_64_movq_reg_membase(r->argfltregs[5], REG_SP, 12 * 8); */
/*  		x86_64_movq_reg_membase(r->argfltregs[6], REG_SP, 13 * 8); */
/*  		x86_64_movq_reg_membase(r->argfltregs[7], REG_SP, 14 * 8); */

		for (p = 0, l = 0; p < m->paramcount; p++) {
			t = m->paramtypes[p];

			if (IS_FLT_DBL_TYPE(t)) {
				for (s1 = (m->paramcount > INT_ARG_CNT) ? INT_ARG_CNT - 2 : m->paramcount - 2; s1 >= p; s1--) {
					x86_64_mov_reg_reg(r->argintregs[s1], r->argintregs[s1 + 1]);
				}

				x86_64_movd_freg_reg(r->argfltregs[l], r->argintregs[p]);
				l++;
			}
		}

		x86_64_mov_imm_reg((s8) m, REG_ITMP2);
		x86_64_mov_reg_membase(REG_ITMP2, REG_SP, 0 * 8);
		x86_64_mov_imm_reg((s8) builtin_trace_args, REG_ITMP1);
		x86_64_call_reg(REG_ITMP1);

		x86_64_mov_membase_reg(REG_SP, 1 * 8, r->argintregs[0]);
		x86_64_mov_membase_reg(REG_SP, 2 * 8, r->argintregs[1]);
		x86_64_mov_membase_reg(REG_SP, 3 * 8, r->argintregs[2]);
		x86_64_mov_membase_reg(REG_SP, 4 * 8, r->argintregs[3]);
		x86_64_mov_membase_reg(REG_SP, 5 * 8, r->argintregs[4]);
		x86_64_mov_membase_reg(REG_SP, 6 * 8, r->argintregs[5]);

		x86_64_movq_membase_reg(REG_SP, 7 * 8, r->argfltregs[0]);
		x86_64_movq_membase_reg(REG_SP, 8 * 8, r->argfltregs[1]);
		x86_64_movq_membase_reg(REG_SP, 9 * 8, r->argfltregs[2]);
		x86_64_movq_membase_reg(REG_SP, 10 * 8, r->argfltregs[3]);
/*  		x86_64_movq_membase_reg(REG_SP, 11 * 8, r->argfltregs[4]); */
/*  		x86_64_movq_membase_reg(REG_SP, 12 * 8, r->argfltregs[5]); */
/*  		x86_64_movq_membase_reg(REG_SP, 13 * 8, r->argfltregs[6]); */
/*  		x86_64_movq_membase_reg(REG_SP, 14 * 8, r->argfltregs[7]); */

		x86_64_alu_imm_reg(X86_64_ADD, (6 + 8 + 1 + 1) * 8, REG_SP);
	}

	/* take arguments out of register or stack frame */

 	for (p = 0, l = 0, s1 = 0, s2 = 0; p < m->paramcount; p++) {
 		t = m->paramtypes[p];
 		var = &(r->locals[l][t]);
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
   					M_INTMOVE(r->argintregs[s1], var->regoff);

				} else {                             /* reg arg -> spilled    */
   				    x86_64_mov_reg_membase(r->argintregs[s1], REG_SP, var->regoff * 8);
 				}

			} else {                                 /* stack arguments       */
 				pa = s1 - INT_ARG_CNT;
				if (s2 >= FLT_ARG_CNT) {
					pa += s2 - FLT_ARG_CNT;
				}
 				if (!(var->flags & INMEMORY)) {      /* stack arg -> register */ 
 					x86_64_mov_membase_reg(REG_SP, (parentargs_base + pa) * 8 + 8, var->regoff);    /* + 8 for return address */
				} else {                             /* stack arg -> spilled  */
					x86_64_mov_membase_reg(REG_SP, (parentargs_base + pa) * 8 + 8, REG_ITMP1);    /* + 8 for return address */
					x86_64_mov_reg_membase(REG_ITMP1, REG_SP, var->regoff * 8);
				}
			}
			s1++;

		} else {                                     /* floating args         */   
 			if (s2 < FLT_ARG_CNT) {                /* register arguments    */
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
					M_FLTMOVE(r->argfltregs[s2], var->regoff);

 				} else {			                 /* reg arg -> spilled    */
					x86_64_movq_reg_membase(r->argfltregs[s2], REG_SP, var->regoff * 8);
 				}

 			} else {                                 /* stack arguments       */
 				pa = s2 - FLT_ARG_CNT;
				if (s1 >= INT_ARG_CNT) {
					pa += s1 - INT_ARG_CNT;
				}
 				if (!(var->flags & INMEMORY)) {      /* stack-arg -> register */
					x86_64_movq_membase_reg(REG_SP, (parentargs_base + pa) * 8 + 8, var->regoff);

				} else {
					x86_64_movq_membase_reg(REG_SP, (parentargs_base + pa) * 8 + 8, REG_FTMP1);
					x86_64_movq_reg_membase(REG_FTMP1, REG_SP, var->regoff * 8);
				}
			}
			s2++;
		}
	}  /* end for */

	/* call monitorenter function */

#if defined(USE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		s8 func_enter = (m->flags & ACC_STATIC) ?
			(s8) builtin_staticmonitorenter : (s8) builtin_monitorenter;
		x86_64_mov_membase_reg(REG_SP, r->maxmemuse * 8, r->argintregs[0]);
		x86_64_mov_imm_reg(func_enter, REG_ITMP1);
		x86_64_call_reg(REG_ITMP1);
	}			
#endif
	}

	/* end of header generation */

	/* walk through all basic blocks */
	for (bptr = m->basicblocks; bptr != NULL; bptr = bptr->next) {

		bptr->mpc = (u4) ((u1 *) mcodeptr - mcodebase);

		if (bptr->flags >= BBREACHED) {

		/* branch resolving */

		branchref *brefs;
		for (brefs = bptr->branchrefs; brefs != NULL; brefs = brefs->next) {
			gen_resolvebranch((u1*) mcodebase + brefs->branchpos, 
			                  brefs->branchpos,
							  bptr->mpc);
		}

		/* copy interface registers to their destination */

		src = bptr->instack;
		len = bptr->indepth;
		MCODECHECK(64 + len);
		while (src != NULL) {
			len--;
  			if ((len == 0) && (bptr->type != BBTYPE_STD)) {
				if (bptr->type == BBTYPE_SBR) {
					d = reg_of_var(m, src, REG_ITMP1);
					x86_64_pop_reg(d);
					store_reg_to_var_int(src, d);

				} else if (bptr->type == BBTYPE_EXH) {
					d = reg_of_var(m, src, REG_ITMP1);
					M_INTMOVE(REG_ITMP1, d);
					store_reg_to_var_int(src, d);
				}

			} else {
				d = reg_of_var(m, src, REG_ITMP1);
				if ((src->varkind != STACKVAR)) {
					s2 = src->type;
					if (IS_FLT_DBL_TYPE(s2)) {
						s1 = r->interfaces[len][s2].regoff;
						if (!(r->interfaces[len][s2].flags & INMEMORY)) {
							M_FLTMOVE(s1, d);

						} else {
							x86_64_movq_membase_reg(REG_SP, s1 * 8, d);
						}
						store_reg_to_var_flt(src, d);

					} else {
						s1 = r->interfaces[len][s2].regoff;
						if (!(r->interfaces[len][s2].flags & INMEMORY)) {
							M_INTMOVE(s1, d);

						} else {
							x86_64_mov_membase_reg(REG_SP, s1 * 8, d);
						}
						store_reg_to_var_int(src, d);
					}
				}
			}
			src = src->prev;
		}

		/* walk through all instructions */
		
		src = bptr->instack;
		len = bptr->icount;
		for (iptr = bptr->iinstr; len > 0; src = iptr->dst, len--, iptr++) {

			MCODECHECK(64);   /* an instruction usually needs < 64 words      */
			switch (iptr->opc) {

			case ICMD_NOP:    /* ...  ==> ...                                 */
				break;

			case ICMD_NULLCHECKPOP: /* ..., objectref  ==> ...                */
				if (src->flags & INMEMORY) {
					x86_64_alu_imm_membase(X86_64_CMP, 0, REG_SP, src->regoff * 8);

				} else {
					x86_64_test_reg_reg(src->regoff, src->regoff);
				}
				x86_64_jcc(X86_64_CC_E, 0);
				codegen_addxnullrefs(mcodeptr);
				break;

		/* constant operations ************************************************/

		case ICMD_ICONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.i = constant                    */

			d = reg_of_var(m, iptr->dst, REG_ITMP1);
			if (iptr->val.i == 0) {
				x86_64_alu_reg_reg(X86_64_XOR, d, d);
			} else {
				x86_64_movl_imm_reg(iptr->val.i, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ACONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.a = constant                    */

			d = reg_of_var(m, iptr->dst, REG_ITMP1);
			if (iptr->val.a == 0) {
				x86_64_alu_reg_reg(X86_64_XOR, d, d);
			} else {
				x86_64_mov_imm_reg((s8) iptr->val.a, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.l = constant                    */

			d = reg_of_var(m, iptr->dst, REG_ITMP1);
			if (iptr->val.l == 0) {
				x86_64_alu_reg_reg(X86_64_XOR, d, d);
			} else {
				x86_64_mov_imm_reg(iptr->val.l, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.f = constant                    */

			d = reg_of_var(m, iptr->dst, REG_FTMP1);
			a = dseg_addfloat(iptr->val.f);
			x86_64_movdl_membase_reg(RIP, -(((s8) mcodeptr + ((d > 7) ? 9 : 8)) - (s8) mcodebase) + a, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
		
		case ICMD_DCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.d = constant                    */

			d = reg_of_var(m, iptr->dst, REG_FTMP1);
			a = dseg_adddouble(iptr->val.d);
			x86_64_movd_membase_reg(RIP, -(((s8) mcodeptr + 9) - (s8) mcodebase) + a, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;


		/* load/store operations **********************************************/

		case ICMD_ILOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			d = reg_of_var(m, iptr->dst, REG_ITMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1)) {
				break;
			}
			var = &(r->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY) {
				x86_64_movl_membase_reg(REG_SP, var->regoff * 8, d);
				store_reg_to_var_int(iptr->dst, d);

			} else {
				if (iptr->dst->flags & INMEMORY) {
					x86_64_mov_reg_membase(var->regoff, REG_SP, iptr->dst->regoff * 8);

				} else {
					M_INTMOVE(var->regoff, d);
				}
			}
			break;

		case ICMD_LLOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_ALOAD:      /* op1 = local variable                         */

			d = reg_of_var(m, iptr->dst, REG_ITMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1)) {
				break;
			}
			var = &(r->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY) {
				x86_64_mov_membase_reg(REG_SP, var->regoff * 8, d);
				store_reg_to_var_int(iptr->dst, d);

			} else {
				if (iptr->dst->flags & INMEMORY) {
					x86_64_mov_reg_membase(var->regoff, REG_SP, iptr->dst->regoff * 8);

				} else {
					M_INTMOVE(var->regoff, d);
				}
			}
			break;

		case ICMD_FLOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_DLOAD:      /* op1 = local variable                         */

			d = reg_of_var(m, iptr->dst, REG_FTMP1);
  			if ((iptr->dst->varkind == LOCALVAR) &&
  			    (iptr->dst->varnum == iptr->op1)) {
    				break;
  			}
			var = &(r->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY) {
				x86_64_movq_membase_reg(REG_SP, var->regoff * 8, d);
				store_reg_to_var_flt(iptr->dst, d);

			} else {
				if (iptr->dst->flags & INMEMORY) {
					x86_64_movq_reg_membase(var->regoff, REG_SP, iptr->dst->regoff * 8);

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
			var = &(r->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				var_to_reg_int(s1, src, REG_ITMP1);
				x86_64_mov_reg_membase(s1, REG_SP, var->regoff * 8);

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
			var = &(r->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				var_to_reg_flt(s1, src, REG_FTMP1);
				x86_64_movq_reg_membase(s1, REG_SP, var->regoff * 8);

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

#define M_COPY(from,to) \
	        d = reg_of_var(m, to, REG_ITMP1); \
			if ((from->regoff != to->regoff) || \
			    ((from->flags ^ to->flags) & INMEMORY)) { \
				if (IS_FLT_DBL_TYPE(from->type)) { \
					var_to_reg_flt(s1, from, d); \
					M_FLTMOVE(s1, d); \
					store_reg_to_var_flt(to, d); \
				} else { \
					var_to_reg_int(s1, from, d); \
					M_INTMOVE(s1, d); \
					store_reg_to_var_int(to, d); \
				} \
			}

		case ICMD_DUP:        /* ..., a ==> ..., a, a                         */
			M_COPY(src, iptr->dst);
			break;

		case ICMD_DUP_X1:     /* ..., a, b ==> ..., b, a, b                   */

			M_COPY(src,       iptr->dst->prev->prev);

		case ICMD_DUP2:       /* ..., a, b ==> ..., a, b, a, b                */

			M_COPY(src,       iptr->dst);
			M_COPY(src->prev, iptr->dst->prev);
			break;

		case ICMD_DUP2_X1:    /* ..., a, b, c ==> ..., b, c, a, b, c          */

			M_COPY(src->prev,       iptr->dst->prev->prev->prev);

		case ICMD_DUP_X2:     /* ..., a, b, c ==> ..., c, a, b, c             */

			M_COPY(src,             iptr->dst);
			M_COPY(src->prev,       iptr->dst->prev);
			M_COPY(src->prev->prev, iptr->dst->prev->prev);
			M_COPY(src, iptr->dst->prev->prev->prev);
			break;

		case ICMD_DUP2_X2:    /* ..., a, b, c, d ==> ..., c, d, a, b, c, d    */

			M_COPY(src,                   iptr->dst);
			M_COPY(src->prev,             iptr->dst->prev);
			M_COPY(src->prev->prev,       iptr->dst->prev->prev);
			M_COPY(src->prev->prev->prev, iptr->dst->prev->prev->prev);
			M_COPY(src,       iptr->dst->prev->prev->prev->prev);
			M_COPY(src->prev, iptr->dst->prev->prev->prev->prev->prev);
			break;

		case ICMD_SWAP:       /* ..., a, b ==> ..., b, a                      */

			M_COPY(src, iptr->dst->prev);
			M_COPY(src->prev, iptr->dst);
			break;


		/* integer operations *************************************************/

		case ICMD_INEG:       /* ..., value  ==> ..., - value                 */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						x86_64_negl_membase(REG_SP, iptr->dst->regoff * 8);

					} else {
						x86_64_movl_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						x86_64_negl_reg(REG_ITMP1);
						x86_64_movl_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					x86_64_movl_reg_membase(src->regoff, REG_SP, iptr->dst->regoff * 8);
					x86_64_negl_membase(REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					x86_64_movl_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);
					x86_64_negl_reg(d);

				} else {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					x86_64_negl_reg(iptr->dst->regoff);
				}
			}
			break;

		case ICMD_LNEG:       /* ..., value  ==> ..., - value                 */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						x86_64_neg_membase(REG_SP, iptr->dst->regoff * 8);

					} else {
						x86_64_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						x86_64_neg_reg(REG_ITMP1);
						x86_64_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					x86_64_mov_reg_membase(src->regoff, REG_SP, iptr->dst->regoff * 8);
					x86_64_neg_membase(REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					x86_64_mov_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);
					x86_64_neg_reg(iptr->dst->regoff);

				} else {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					x86_64_neg_reg(iptr->dst->regoff);
				}
			}
			break;

		case ICMD_I2L:        /* ..., value  ==> ..., value                   */

			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			if (src->flags & INMEMORY) {
				x86_64_movslq_membase_reg(REG_SP, src->regoff * 8, d);

			} else {
				x86_64_movslq_reg_reg(src->regoff, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			if (src->flags & INMEMORY) {
				x86_64_movsbq_membase_reg(REG_SP, src->regoff * 8, d);

			} else {
				x86_64_movsbq_reg_reg(src->regoff, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2CHAR:   /* ..., value  ==> ..., value                   */

			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			if (src->flags & INMEMORY) {
				x86_64_movzwq_membase_reg(REG_SP, src->regoff * 8, d);

			} else {
				x86_64_movzwq_reg_reg(src->regoff, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2SHORT:  /* ..., value  ==> ..., value                   */

			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			if (src->flags & INMEMORY) {
				x86_64_movswq_membase_reg(REG_SP, src->regoff * 8, d);

			} else {
				x86_64_movswq_reg_reg(src->regoff, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_IADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_ialu(X86_64_ADD, src, iptr);
			break;

		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_ialuconst(X86_64_ADD, src, iptr);
			break;

		case ICMD_LADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_lalu(X86_64_ADD, src, iptr);
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_laluconst(X86_64_ADD, src, iptr);
			break;

		case ICMD_ISUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						x86_64_movl_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						x86_64_alul_reg_membase(X86_64_SUB, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

					} else {
						x86_64_movl_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						x86_64_alul_membase_reg(X86_64_SUB, REG_SP, src->regoff * 8, REG_ITMP1);
						x86_64_movl_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, REG_ITMP1);
					x86_64_alul_membase_reg(X86_64_SUB, REG_SP, src->regoff * 8, REG_ITMP1);
					x86_64_movl_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						x86_64_alul_reg_membase(X86_64_SUB, src->regoff, REG_SP, iptr->dst->regoff * 8);

					} else {
						x86_64_movl_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						x86_64_alul_reg_reg(X86_64_SUB, src->regoff, REG_ITMP1);
						x86_64_movl_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					x86_64_movl_reg_membase(src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
					x86_64_alul_reg_membase(X86_64_SUB, src->regoff, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_movl_membase_reg(REG_SP, src->prev->regoff * 8, d);
					x86_64_alul_membase_reg(X86_64_SUB, REG_SP, src->regoff * 8, d);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, d);
					x86_64_alul_membase_reg(X86_64_SUB, REG_SP, src->regoff * 8, d);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					/* workaround for reg alloc */
					if (src->regoff == iptr->dst->regoff) {
						x86_64_movl_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						x86_64_alul_reg_reg(X86_64_SUB, src->regoff, REG_ITMP1);
						M_INTMOVE(REG_ITMP1, d);

					} else {
						x86_64_movl_membase_reg(REG_SP, src->prev->regoff * 8, d);
						x86_64_alul_reg_reg(X86_64_SUB, src->regoff, d);
					}

				} else {
					/* workaround for reg alloc */
					if (src->regoff == iptr->dst->regoff) {
						M_INTMOVE(src->prev->regoff, REG_ITMP1);
						x86_64_alul_reg_reg(X86_64_SUB, src->regoff, REG_ITMP1);
						M_INTMOVE(REG_ITMP1, d);

					} else {
						M_INTMOVE(src->prev->regoff, d);
						x86_64_alul_reg_reg(X86_64_SUB, src->regoff, d);
					}
				}
			}
			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_ialuconst(X86_64_SUB, src, iptr);
			break;

		case ICMD_LSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						x86_64_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						x86_64_alu_reg_membase(X86_64_SUB, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

					} else {
						x86_64_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						x86_64_alu_membase_reg(X86_64_SUB, REG_SP, src->regoff * 8, REG_ITMP1);
						x86_64_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, REG_ITMP1);
					x86_64_alu_membase_reg(X86_64_SUB, REG_SP, src->regoff * 8, REG_ITMP1);
					x86_64_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						x86_64_alu_reg_membase(X86_64_SUB, src->regoff, REG_SP, iptr->dst->regoff * 8);

					} else {
						x86_64_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						x86_64_alu_reg_reg(X86_64_SUB, src->regoff, REG_ITMP1);
						x86_64_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					x86_64_mov_reg_membase(src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
					x86_64_alu_reg_membase(X86_64_SUB, src->regoff, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_mov_membase_reg(REG_SP, src->prev->regoff * 8, d);
					x86_64_alu_membase_reg(X86_64_SUB, REG_SP, src->regoff * 8, d);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, d);
					x86_64_alu_membase_reg(X86_64_SUB, REG_SP, src->regoff * 8, d);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					/* workaround for reg alloc */
					if (src->regoff == iptr->dst->regoff) {
						x86_64_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						x86_64_alu_reg_reg(X86_64_SUB, src->regoff, REG_ITMP1);
						M_INTMOVE(REG_ITMP1, d);

					} else {
						x86_64_mov_membase_reg(REG_SP, src->prev->regoff * 8, d);
						x86_64_alu_reg_reg(X86_64_SUB, src->regoff, d);
					}

				} else {
					/* workaround for reg alloc */
					if (src->regoff == iptr->dst->regoff) {
						M_INTMOVE(src->prev->regoff, REG_ITMP1);
						x86_64_alu_reg_reg(X86_64_SUB, src->regoff, REG_ITMP1);
						M_INTMOVE(REG_ITMP1, d);

					} else {
						M_INTMOVE(src->prev->regoff, d);
						x86_64_alu_reg_reg(X86_64_SUB, src->regoff, d);
					}
				}
			}
			break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_laluconst(X86_64_SUB, src, iptr);
			break;

		case ICMD_IMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_movl_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
					x86_64_imull_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					x86_64_movl_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					x86_64_movl_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					x86_64_imull_reg_reg(src->prev->regoff, REG_ITMP1);
					x86_64_movl_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_movl_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
					x86_64_imull_reg_reg(src->regoff, REG_ITMP1);
					x86_64_movl_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else {
					M_INTMOVE(src->prev->regoff, REG_ITMP1);
					x86_64_imull_reg_reg(src->regoff, REG_ITMP1);
					x86_64_movl_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_movl_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
					x86_64_imull_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
					x86_64_imull_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					x86_64_imull_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);

				} else {
					if (src->regoff == iptr->dst->regoff) {
						x86_64_imull_reg_reg(src->prev->regoff, iptr->dst->regoff);

					} else {
						M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
						x86_64_imull_reg_reg(src->regoff, iptr->dst->regoff);
					}
				}
			}
			break;

		case ICMD_IMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					x86_64_imull_imm_membase_reg(iptr->val.i, REG_SP, src->regoff * 8, REG_ITMP1);
					x86_64_movl_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else {
					x86_64_imull_imm_reg_reg(iptr->val.i, src->regoff, REG_ITMP1);
					x86_64_movl_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					x86_64_imull_imm_membase_reg(iptr->val.i, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else {
					if (iptr->val.i == 2) {
						M_INTMOVE(src->regoff, iptr->dst->regoff);
						x86_64_alul_reg_reg(X86_64_ADD, iptr->dst->regoff, iptr->dst->regoff);

					} else {
						x86_64_imull_imm_reg_reg(iptr->val.i, src->regoff, iptr->dst->regoff);    /* 3 cycles */
					}
				}
			}
			break;

		case ICMD_LMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
					x86_64_imul_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					x86_64_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					x86_64_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					x86_64_imul_reg_reg(src->prev->regoff, REG_ITMP1);
					x86_64_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
					x86_64_imul_reg_reg(src->regoff, REG_ITMP1);
					x86_64_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else {
					x86_64_mov_reg_reg(src->prev->regoff, REG_ITMP1);
					x86_64_imul_reg_reg(src->regoff, REG_ITMP1);
					x86_64_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					x86_64_mov_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
					x86_64_imul_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
					x86_64_imul_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					x86_64_imul_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);

				} else {
					if (src->regoff == iptr->dst->regoff) {
						x86_64_imul_reg_reg(src->prev->regoff, iptr->dst->regoff);

					} else {
						M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
						x86_64_imul_reg_reg(src->regoff, iptr->dst->regoff);
					}
				}
			}
			break;

		case ICMD_LMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (x86_64_is_imm32(iptr->val.l)) {
						x86_64_imul_imm_membase_reg(iptr->val.l, REG_SP, src->regoff * 8, REG_ITMP1);

					} else {
						x86_64_mov_imm_reg(iptr->val.l, REG_ITMP1);
						x86_64_imul_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					}
					x86_64_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					
				} else {
					if (x86_64_is_imm32(iptr->val.l)) {
						x86_64_imul_imm_reg_reg(iptr->val.l, src->regoff, REG_ITMP1);

					} else {
						x86_64_mov_imm_reg(iptr->val.l, REG_ITMP1);
						x86_64_imul_reg_reg(src->regoff, REG_ITMP1);
					}
					x86_64_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					if (x86_64_is_imm32(iptr->val.l)) {
						x86_64_imul_imm_membase_reg(iptr->val.l, REG_SP, src->regoff * 8, iptr->dst->regoff);

					} else {
						x86_64_mov_imm_reg(iptr->val.l, iptr->dst->regoff);
						x86_64_imul_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);
					}

				} else {
					/* should match in many cases */
					if (iptr->val.l == 2) {
						M_INTMOVE(src->regoff, iptr->dst->regoff);
						x86_64_alul_reg_reg(X86_64_ADD, iptr->dst->regoff, iptr->dst->regoff);

					} else {
						if (x86_64_is_imm32(iptr->val.l)) {
							x86_64_imul_imm_reg_reg(iptr->val.l, src->regoff, iptr->dst->regoff);    /* 4 cycles */

						} else {
							x86_64_mov_imm_reg(iptr->val.l, REG_ITMP1);
							M_INTMOVE(src->regoff, iptr->dst->regoff);
							x86_64_imul_reg_reg(REG_ITMP1, iptr->dst->regoff);
						}
					}
				}
			}
			break;

		case ICMD_IDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			d = reg_of_var(m, iptr->dst, REG_NULL);
	        if (src->prev->flags & INMEMORY) {
				x86_64_movl_membase_reg(REG_SP, src->prev->regoff * 8, RAX);

			} else {
				M_INTMOVE(src->prev->regoff, RAX);
			}
			
			if (src->flags & INMEMORY) {
				x86_64_movl_membase_reg(REG_SP, src->regoff * 8, REG_ITMP3);

			} else {
				M_INTMOVE(src->regoff, REG_ITMP3);
			}
			gen_div_check(src);

			x86_64_alul_imm_reg(X86_64_CMP, 0x80000000, RAX);    /* check as described in jvm spec */
			x86_64_jcc(X86_64_CC_NE, 4 + 6);
			x86_64_alul_imm_reg(X86_64_CMP, -1, REG_ITMP3);      /* 4 bytes */
			x86_64_jcc(X86_64_CC_E, 3 + 1 + 3);                  /* 6 bytes */

			x86_64_mov_reg_reg(RDX, REG_ITMP2);    /* save %rdx, cause it's an argument register */
  			x86_64_cltd();
			x86_64_idivl_reg(REG_ITMP3);

			if (iptr->dst->flags & INMEMORY) {
				x86_64_mov_reg_membase(RAX, REG_SP, iptr->dst->regoff * 8);
				x86_64_mov_reg_reg(REG_ITMP2, RDX);    /* restore %rdx */

			} else {
				M_INTMOVE(RAX, iptr->dst->regoff);

				if (iptr->dst->regoff != RDX) {
					x86_64_mov_reg_reg(REG_ITMP2, RDX);    /* restore %rdx */
				}
			}
			break;

		case ICMD_IREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			if (src->prev->flags & INMEMORY) {
				x86_64_movl_membase_reg(REG_SP, src->prev->regoff * 8, RAX);

			} else {
				M_INTMOVE(src->prev->regoff, RAX);
			}
			
			if (src->flags & INMEMORY) {
				x86_64_movl_membase_reg(REG_SP, src->regoff * 8, REG_ITMP3);

			} else {
				M_INTMOVE(src->regoff, REG_ITMP3);
			}
			gen_div_check(src);

			x86_64_alul_imm_reg(X86_64_CMP, 0x80000000, RAX);    /* check as described in jvm spec */
			x86_64_jcc(X86_64_CC_NE, 2 + 4 + 6);
			x86_64_alul_reg_reg(X86_64_XOR, RDX, RDX);           /* 2 bytes */
			x86_64_alul_imm_reg(X86_64_CMP, -1, REG_ITMP3);      /* 4 bytes */
			x86_64_jcc(X86_64_CC_E, 3 + 1 + 3);                  /* 6 bytes */

			x86_64_mov_reg_reg(RDX, REG_ITMP2);    /* save %rdx, cause it's an argument register */
  			x86_64_cltd();
			x86_64_idivl_reg(REG_ITMP3);

			if (iptr->dst->flags & INMEMORY) {
				x86_64_mov_reg_membase(RDX, REG_SP, iptr->dst->regoff * 8);
				x86_64_mov_reg_reg(REG_ITMP2, RDX);    /* restore %rdx */

			} else {
				M_INTMOVE(RDX, iptr->dst->regoff);

				if (iptr->dst->regoff != RDX) {
					x86_64_mov_reg_reg(REG_ITMP2, RDX);    /* restore %rdx */
				}
			}
			break;

		case ICMD_IDIVPOW2:   /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, REG_ITMP1);
			x86_64_alul_imm_reg(X86_64_CMP, -1, REG_ITMP1);
			x86_64_leal_membase_reg(REG_ITMP1, (1 << iptr->val.i) - 1, REG_ITMP2);
			x86_64_cmovccl_reg_reg(X86_64_CC_LE, REG_ITMP2, REG_ITMP1);
			x86_64_shiftl_imm_reg(X86_64_SAR, iptr->val.i, REG_ITMP1);
			x86_64_mov_reg_reg(REG_ITMP1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, REG_ITMP1);
			x86_64_alul_imm_reg(X86_64_CMP, -1, REG_ITMP1);
			x86_64_leal_membase_reg(REG_ITMP1, iptr->val.i, REG_ITMP2);
			x86_64_cmovccl_reg_reg(X86_64_CC_G, REG_ITMP1, REG_ITMP2);
			x86_64_alul_imm_reg(X86_64_AND, -1 - (iptr->val.i), REG_ITMP2);
			x86_64_alul_reg_reg(X86_64_SUB, REG_ITMP2, REG_ITMP1);
			x86_64_mov_reg_reg(REG_ITMP1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_LDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			d = reg_of_var(m, iptr->dst, REG_NULL);
	        if (src->prev->flags & INMEMORY) {
				x86_64_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);

			} else {
				M_INTMOVE(src->prev->regoff, REG_ITMP1);
			}
			
			if (src->flags & INMEMORY) {
				x86_64_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP3);

			} else {
				M_INTMOVE(src->regoff, REG_ITMP3);
			}
			gen_div_check(src);

			x86_64_mov_imm_reg(0x8000000000000000LL, REG_ITMP2);    /* check as described in jvm spec */
			x86_64_alu_reg_reg(X86_64_CMP, REG_ITMP2, REG_ITMP1);
			x86_64_jcc(X86_64_CC_NE, 4 + 6);
			x86_64_alu_imm_reg(X86_64_CMP, -1, REG_ITMP3);          /* 4 bytes */
			x86_64_jcc(X86_64_CC_E, 3 + 2 + 3);                     /* 6 bytes */

			x86_64_mov_reg_reg(RDX, REG_ITMP2);    /* save %rdx, cause it's an argument register */
  			x86_64_cqto();
			x86_64_idiv_reg(REG_ITMP3);

			if (iptr->dst->flags & INMEMORY) {
				x86_64_mov_reg_membase(RAX, REG_SP, iptr->dst->regoff * 8);
				x86_64_mov_reg_reg(REG_ITMP2, RDX);    /* restore %rdx */

			} else {
				M_INTMOVE(RAX, iptr->dst->regoff);

				if (iptr->dst->regoff != RDX) {
					x86_64_mov_reg_reg(REG_ITMP2, RDX);    /* restore %rdx */
				}
			}
			break;

		case ICMD_LREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			if (src->prev->flags & INMEMORY) {
				x86_64_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);

			} else {
				M_INTMOVE(src->prev->regoff, REG_ITMP1);
			}
			
			if (src->flags & INMEMORY) {
				x86_64_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP3);

			} else {
				M_INTMOVE(src->regoff, REG_ITMP3);
			}
			gen_div_check(src);

			x86_64_mov_imm_reg(0x8000000000000000LL, REG_ITMP2);    /* check as described in jvm spec */
			x86_64_alu_reg_reg(X86_64_CMP, REG_ITMP2, REG_ITMP1);
			x86_64_jcc(X86_64_CC_NE, 2 + 4 + 6);
			x86_64_alul_reg_reg(X86_64_XOR, RDX, RDX);              /* 2 bytes */
			x86_64_alu_imm_reg(X86_64_CMP, -1, REG_ITMP3);          /* 4 bytes */
			x86_64_jcc(X86_64_CC_E, 3 + 2 + 3);                     /* 6 bytes */

			x86_64_mov_reg_reg(RDX, REG_ITMP2);    /* save %rdx, cause it's an argument register */
  			x86_64_cqto();
			x86_64_idiv_reg(REG_ITMP3);

			if (iptr->dst->flags & INMEMORY) {
				x86_64_mov_reg_membase(RDX, REG_SP, iptr->dst->regoff * 8);
				x86_64_mov_reg_reg(REG_ITMP2, RDX);    /* restore %rdx */

			} else {
				M_INTMOVE(RDX, iptr->dst->regoff);

				if (iptr->dst->regoff != RDX) {
					x86_64_mov_reg_reg(REG_ITMP2, RDX);    /* restore %rdx */
				}
			}
			break;

		case ICMD_LDIVPOW2:   /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, REG_ITMP1);
			x86_64_alu_imm_reg(X86_64_CMP, -1, REG_ITMP1);
			x86_64_lea_membase_reg(REG_ITMP1, (1 << iptr->val.i) - 1, REG_ITMP2);
			x86_64_cmovcc_reg_reg(X86_64_CC_LE, REG_ITMP2, REG_ITMP1);
			x86_64_shift_imm_reg(X86_64_SAR, iptr->val.i, REG_ITMP1);
			x86_64_mov_reg_reg(REG_ITMP1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, REG_ITMP1);
			x86_64_alu_imm_reg(X86_64_CMP, -1, REG_ITMP1);
			x86_64_lea_membase_reg(REG_ITMP1, iptr->val.i, REG_ITMP2);
			x86_64_cmovcc_reg_reg(X86_64_CC_G, REG_ITMP1, REG_ITMP2);
			x86_64_alu_imm_reg(X86_64_AND, -1 - (iptr->val.i), REG_ITMP2);
			x86_64_alu_reg_reg(X86_64_SUB, REG_ITMP2, REG_ITMP1);
			x86_64_mov_reg_reg(REG_ITMP1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_ishift(X86_64_SHL, src, iptr);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_ishiftconst(X86_64_SHL, src, iptr);
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_ishift(X86_64_SAR, src, iptr);
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_ishiftconst(X86_64_SAR, src, iptr);
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_ishift(X86_64_SHR, src, iptr);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_ishiftconst(X86_64_SHR, src, iptr);
			break;

		case ICMD_LSHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_lshift(X86_64_SHL, src, iptr);
			break;

        case ICMD_LSHLCONST:  /* ..., value  ==> ..., value << constant       */
 			                  /* val.i = constant                             */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_lshiftconst(X86_64_SHL, src, iptr);
			break;

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_lshift(X86_64_SAR, src, iptr);
			break;

		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_lshiftconst(X86_64_SAR, src, iptr);
			break;

		case ICMD_LUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_lshift(X86_64_SHR, src, iptr);
			break;

  		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
  		                      /* val.l = constant                             */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_lshiftconst(X86_64_SHR, src, iptr);
			break;

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_ialu(X86_64_AND, src, iptr);
			break;

		case ICMD_IANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_ialuconst(X86_64_AND, src, iptr);
			break;

		case ICMD_LAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_lalu(X86_64_AND, src, iptr);
			break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_laluconst(X86_64_AND, src, iptr);
			break;

		case ICMD_IOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_ialu(X86_64_OR, src, iptr);
			break;

		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_ialuconst(X86_64_OR, src, iptr);
			break;

		case ICMD_LOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_lalu(X86_64_OR, src, iptr);
			break;

		case ICMD_LORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_laluconst(X86_64_OR, src, iptr);
			break;

		case ICMD_IXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_ialu(X86_64_XOR, src, iptr);
			break;

		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_ialuconst(X86_64_XOR, src, iptr);
			break;

		case ICMD_LXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_lalu(X86_64_XOR, src, iptr);
			break;

		case ICMD_LXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(m, iptr->dst, REG_NULL);
			x86_64_emit_laluconst(X86_64_XOR, src, iptr);
			break;


		case ICMD_IINC:       /* ..., value  ==> ..., value + constant        */
		                      /* op1 = variable, val.i = constant             */

			var = &(r->locals[iptr->op1][TYPE_INT]);
			d = var->regoff;
			if (var->flags & INMEMORY) {
				if (iptr->val.i == 1) {
					x86_64_incl_membase(REG_SP, d * 8);
 
				} else if (iptr->val.i == -1) {
					x86_64_decl_membase(REG_SP, d * 8);

				} else {
					x86_64_alul_imm_membase(X86_64_ADD, iptr->val.i, REG_SP, d * 8);
				}

			} else {
				if (iptr->val.i == 1) {
					x86_64_incl_reg(d);
 
				} else if (iptr->val.i == -1) {
					x86_64_decl_reg(d);

				} else {
					x86_64_alul_imm_reg(X86_64_ADD, iptr->val.i, d);
				}
			}
			break;


		/* floating operations ************************************************/

		case ICMD_FNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(m, iptr->dst, REG_FTMP3);
			a = dseg_adds4(0x80000000);
			M_FLTMOVE(s1, d);
			x86_64_movss_membase_reg(RIP, -(((s8) mcodeptr + 9) - (s8) mcodebase) + a, REG_FTMP2);
			x86_64_xorps_reg_reg(REG_FTMP2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(m, iptr->dst, REG_FTMP3);
			a = dseg_adds8(0x8000000000000000);
			M_FLTMOVE(s1, d);
			x86_64_movd_membase_reg(RIP, -(((s8) mcodeptr + 9) - (s8) mcodebase) + a, REG_FTMP2);
			x86_64_xorpd_reg_reg(REG_FTMP2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(m, iptr->dst, REG_FTMP3);
			if (s1 == d) {
				x86_64_addss_reg_reg(s2, d);
			} else if (s2 == d) {
				x86_64_addss_reg_reg(s1, d);
			} else {
				M_FLTMOVE(s1, d);
				x86_64_addss_reg_reg(s2, d);
			}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(m, iptr->dst, REG_FTMP3);
			if (s1 == d) {
				x86_64_addsd_reg_reg(s2, d);
			} else if (s2 == d) {
				x86_64_addsd_reg_reg(s1, d);
			} else {
				M_FLTMOVE(s1, d);
				x86_64_addsd_reg_reg(s2, d);
			}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(m, iptr->dst, REG_FTMP3);
			if (s2 == d) {
				M_FLTMOVE(s2, REG_FTMP2);
				s2 = REG_FTMP2;
			}
			M_FLTMOVE(s1, d);
			x86_64_subss_reg_reg(s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(m, iptr->dst, REG_FTMP3);
			if (s2 == d) {
				M_FLTMOVE(s2, REG_FTMP2);
				s2 = REG_FTMP2;
			}
			M_FLTMOVE(s1, d);
			x86_64_subsd_reg_reg(s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(m, iptr->dst, REG_FTMP3);
			if (s1 == d) {
				x86_64_mulss_reg_reg(s2, d);
			} else if (s2 == d) {
				x86_64_mulss_reg_reg(s1, d);
			} else {
				M_FLTMOVE(s1, d);
				x86_64_mulss_reg_reg(s2, d);
			}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(m, iptr->dst, REG_FTMP3);
			if (s1 == d) {
				x86_64_mulsd_reg_reg(s2, d);
			} else if (s2 == d) {
				x86_64_mulsd_reg_reg(s1, d);
			} else {
				M_FLTMOVE(s1, d);
				x86_64_mulsd_reg_reg(s2, d);
			}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(m, iptr->dst, REG_FTMP3);
			if (s2 == d) {
				M_FLTMOVE(s2, REG_FTMP2);
				s2 = REG_FTMP2;
			}
			M_FLTMOVE(s1, d);
			x86_64_divss_reg_reg(s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(m, iptr->dst, REG_FTMP3);
			if (s2 == d) {
				M_FLTMOVE(s2, REG_FTMP2);
				s2 = REG_FTMP2;
			}
			M_FLTMOVE(s1, d);
			x86_64_divsd_reg_reg(s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_I2F:       /* ..., value  ==> ..., (float) value            */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(m, iptr->dst, REG_FTMP1);
			x86_64_cvtsi2ss_reg_reg(s1, d);
  			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_I2D:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(m, iptr->dst, REG_FTMP1);
			x86_64_cvtsi2sd_reg_reg(s1, d);
  			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_L2F:       /* ..., value  ==> ..., (float) value            */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(m, iptr->dst, REG_FTMP1);
			x86_64_cvtsi2ssq_reg_reg(s1, d);
  			store_reg_to_var_flt(iptr->dst, d);
			break;
			
		case ICMD_L2D:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(m, iptr->dst, REG_FTMP1);
			x86_64_cvtsi2sdq_reg_reg(s1, d);
  			store_reg_to_var_flt(iptr->dst, d);
			break;
			
		case ICMD_F2I:       /* ..., value  ==> ..., (int) value              */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(m, iptr->dst, REG_ITMP1);
			x86_64_cvttss2si_reg_reg(s1, d);
			x86_64_alul_imm_reg(X86_64_CMP, 0x80000000, d);    /* corner cases */
			a = ((s1 == REG_FTMP1) ? 0 : 5) + 10 + 3 + ((REG_RESULT == d) ? 0 : 3);
			x86_64_jcc(X86_64_CC_NE, a);
			M_FLTMOVE(s1, REG_FTMP1);
			x86_64_mov_imm_reg((s8) asm_builtin_f2i, REG_ITMP2);
			x86_64_call_reg(REG_ITMP2);
			M_INTMOVE(REG_RESULT, d);
  			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_D2I:       /* ..., value  ==> ..., (int) value              */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(m, iptr->dst, REG_ITMP1);
			x86_64_cvttsd2si_reg_reg(s1, d);
			x86_64_alul_imm_reg(X86_64_CMP, 0x80000000, d);    /* corner cases */
			a = ((s1 == REG_FTMP1) ? 0 : 5) + 10 + 3 + ((REG_RESULT == d) ? 0 : 3);
			x86_64_jcc(X86_64_CC_NE, a);
			M_FLTMOVE(s1, REG_FTMP1);
			x86_64_mov_imm_reg((s8) asm_builtin_d2i, REG_ITMP2);
			x86_64_call_reg(REG_ITMP2);
			M_INTMOVE(REG_RESULT, d);
  			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_F2L:       /* ..., value  ==> ..., (long) value             */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(m, iptr->dst, REG_ITMP1);
			x86_64_cvttss2siq_reg_reg(s1, d);
			x86_64_mov_imm_reg(0x8000000000000000, REG_ITMP2);
			x86_64_alu_reg_reg(X86_64_CMP, REG_ITMP2, d);     /* corner cases */
			a = ((s1 == REG_FTMP1) ? 0 : 5) + 10 + 3 + ((REG_RESULT == d) ? 0 : 3);
			x86_64_jcc(X86_64_CC_NE, a);
			M_FLTMOVE(s1, REG_FTMP1);
			x86_64_mov_imm_reg((s8) asm_builtin_f2l, REG_ITMP2);
			x86_64_call_reg(REG_ITMP2);
			M_INTMOVE(REG_RESULT, d);
  			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_D2L:       /* ..., value  ==> ..., (long) value             */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(m, iptr->dst, REG_ITMP1);
			x86_64_cvttsd2siq_reg_reg(s1, d);
			x86_64_mov_imm_reg(0x8000000000000000, REG_ITMP2);
			x86_64_alu_reg_reg(X86_64_CMP, REG_ITMP2, d);     /* corner cases */
			a = ((s1 == REG_FTMP1) ? 0 : 5) + 10 + 3 + ((REG_RESULT == d) ? 0 : 3);
			x86_64_jcc(X86_64_CC_NE, a);
			M_FLTMOVE(s1, REG_FTMP1);
			x86_64_mov_imm_reg((s8) asm_builtin_d2l, REG_ITMP2);
			x86_64_call_reg(REG_ITMP2);
			M_INTMOVE(REG_RESULT, d);
  			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_F2D:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(m, iptr->dst, REG_FTMP3);
			x86_64_cvtss2sd_reg_reg(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_D2F:       /* ..., value  ==> ..., (float) value            */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(m, iptr->dst, REG_FTMP3);
			x86_64_cvtsd2ss_reg_reg(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */
 			                  /* == => 0, < => 1, > => -1 */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			x86_64_alu_reg_reg(X86_64_XOR, d, d);
			x86_64_mov_imm_reg(1, REG_ITMP1);
			x86_64_mov_imm_reg(-1, REG_ITMP2);
			x86_64_ucomiss_reg_reg(s1, s2);
			x86_64_cmovcc_reg_reg(X86_64_CC_B, REG_ITMP1, d);
			x86_64_cmovcc_reg_reg(X86_64_CC_A, REG_ITMP2, d);
			x86_64_cmovcc_reg_reg(X86_64_CC_P, REG_ITMP2, d);    /* treat unordered as GT */
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */
 			                  /* == => 0, < => 1, > => -1 */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			x86_64_alu_reg_reg(X86_64_XOR, d, d);
			x86_64_mov_imm_reg(1, REG_ITMP1);
			x86_64_mov_imm_reg(-1, REG_ITMP2);
			x86_64_ucomiss_reg_reg(s1, s2);
			x86_64_cmovcc_reg_reg(X86_64_CC_B, REG_ITMP1, d);
			x86_64_cmovcc_reg_reg(X86_64_CC_A, REG_ITMP2, d);
			x86_64_cmovcc_reg_reg(X86_64_CC_P, REG_ITMP1, d);    /* treat unordered as LT */
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_DCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */
 			                  /* == => 0, < => 1, > => -1 */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			x86_64_alu_reg_reg(X86_64_XOR, d, d);
			x86_64_mov_imm_reg(1, REG_ITMP1);
			x86_64_mov_imm_reg(-1, REG_ITMP2);
			x86_64_ucomisd_reg_reg(s1, s2);
			x86_64_cmovcc_reg_reg(X86_64_CC_B, REG_ITMP1, d);
			x86_64_cmovcc_reg_reg(X86_64_CC_A, REG_ITMP2, d);
			x86_64_cmovcc_reg_reg(X86_64_CC_P, REG_ITMP2, d);    /* treat unordered as GT */
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_DCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */
 			                  /* == => 0, < => 1, > => -1 */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			x86_64_alu_reg_reg(X86_64_XOR, d, d);
			x86_64_mov_imm_reg(1, REG_ITMP1);
			x86_64_mov_imm_reg(-1, REG_ITMP2);
			x86_64_ucomisd_reg_reg(s1, s2);
			x86_64_cmovcc_reg_reg(X86_64_CC_B, REG_ITMP1, d);
			x86_64_cmovcc_reg_reg(X86_64_CC_A, REG_ITMP2, d);
			x86_64_cmovcc_reg_reg(X86_64_CC_P, REG_ITMP1, d);    /* treat unordered as LT */
			store_reg_to_var_int(iptr->dst, d);
			break;


		/* memory operations **************************************************/

		case ICMD_ARRAYLENGTH: /* ..., arrayref  ==> ..., (int) length        */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			gen_nullptr_check(s1);
			x86_64_movl_membase_reg(s1, OFFSET(java_arrayheader, size), d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_AALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			x86_64_mov_memindex_reg(OFFSET(java_objectarray, data[0]), s1, s2, 3, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			x86_64_mov_memindex_reg(OFFSET(java_longarray, data[0]), s1, s2, 3, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			x86_64_movl_memindex_reg(OFFSET(java_intarray, data[0]), s1, s2, 2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(m, iptr->dst, REG_FTMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			x86_64_movss_memindex_reg(OFFSET(java_floatarray, data[0]), s1, s2, 2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(m, iptr->dst, REG_FTMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			x86_64_movsd_memindex_reg(OFFSET(java_doublearray, data[0]), s1, s2, 3, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_CALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			x86_64_movzwq_memindex_reg(OFFSET(java_chararray, data[0]), s1, s2, 1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;			

		case ICMD_SALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			x86_64_movswq_memindex_reg(OFFSET(java_shortarray, data[0]), s1, s2, 1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_BALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
   			x86_64_movsbq_memindex_reg(OFFSET(java_bytearray, data[0]), s1, s2, 0, d);
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
			x86_64_mov_reg_memindex(s3, OFFSET(java_objectarray, data[0]), s1, s2, 3);
			break;

		case ICMD_LASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			x86_64_mov_reg_memindex(s3, OFFSET(java_longarray, data[0]), s1, s2, 3);
			break;

		case ICMD_IASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			x86_64_movl_reg_memindex(s3, OFFSET(java_intarray, data[0]), s1, s2, 2);
			break;

		case ICMD_FASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_flt(s3, src, REG_FTMP3);
			x86_64_movss_reg_memindex(s3, OFFSET(java_floatarray, data[0]), s1, s2, 2);
			break;

		case ICMD_DASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_flt(s3, src, REG_FTMP3);
			x86_64_movsd_reg_memindex(s3, OFFSET(java_doublearray, data[0]), s1, s2, 3);
			break;

		case ICMD_CASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			x86_64_movw_reg_memindex(s3, OFFSET(java_chararray, data[0]), s1, s2, 1);
			break;

		case ICMD_SASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			x86_64_movw_reg_memindex(s3, OFFSET(java_shortarray, data[0]), s1, s2, 1);
			break;

		case ICMD_BASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			x86_64_movb_reg_memindex(s3, OFFSET(java_bytearray, data[0]), s1, s2, 0);
			break;


		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */
		                      /* op1 = type, val.a = field address            */

			/* if class isn't yet initialized, do it */
  			if (!((fieldinfo *) iptr->val.a)->class->initialized) {
				/* call helper function which patches this code */
				x86_64_mov_imm_reg((s8) ((fieldinfo *) iptr->val.a)->class, REG_ITMP1);
				x86_64_mov_imm_reg((s8) asm_check_clinit, REG_ITMP2);
				x86_64_call_reg(REG_ITMP2);
  			}

			a = dseg_addaddress(&(((fieldinfo *) iptr->val.a)->value));
/*    			x86_64_mov_imm_reg(0, REG_ITMP2); */
/*    			dseg_adddata(mcodeptr); */
/*    			x86_64_mov_membase_reg(REG_ITMP2, a, REG_ITMP2); */
  			x86_64_mov_membase_reg(RIP, -(((s8) mcodeptr + 7) - (s8) mcodebase) + a, REG_ITMP2);
			switch (iptr->op1) {
			case TYPE_INT:
				var_to_reg_int(s2, src, REG_ITMP1);
				x86_64_movl_reg_membase(s2, REG_ITMP2, 0);
				break;
			case TYPE_LNG:
			case TYPE_ADR:
				var_to_reg_int(s2, src, REG_ITMP1);
				x86_64_mov_reg_membase(s2, REG_ITMP2, 0);
				break;
			case TYPE_FLT:
				var_to_reg_flt(s2, src, REG_FTMP1);
				x86_64_movss_reg_membase(s2, REG_ITMP2, 0);
				break;
			case TYPE_DBL:
				var_to_reg_flt(s2, src, REG_FTMP1);
				x86_64_movsd_reg_membase(s2, REG_ITMP2, 0);
				break;
			default: panic("internal error");
			}
			break;

		case ICMD_GETSTATIC:  /* ...  ==> ..., value                          */
		                      /* op1 = type, val.a = field address            */

			/* if class isn't yet initialized, do it */
  			if (!((fieldinfo *) iptr->val.a)->class->initialized) {
				/* call helper function which patches this code */
				x86_64_mov_imm_reg((s8) ((fieldinfo *) iptr->val.a)->class, REG_ITMP1);
				x86_64_mov_imm_reg((s8) asm_check_clinit, REG_ITMP2);
				x86_64_call_reg(REG_ITMP2);
  			}

			a = dseg_addaddress(&(((fieldinfo *) iptr->val.a)->value));
/*  			x86_64_mov_imm_reg(0, REG_ITMP2); */
/*  			dseg_adddata(mcodeptr); */
/*  			x86_64_mov_membase_reg(REG_ITMP2, a, REG_ITMP2); */
  			x86_64_mov_membase_reg(RIP, -(((s8) mcodeptr + 7) - (s8) mcodebase) + a, REG_ITMP2);
			switch (iptr->op1) {
			case TYPE_INT:
				d = reg_of_var(m, iptr->dst, REG_ITMP1);
				x86_64_movl_membase_reg(REG_ITMP2, 0, d);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_LNG:
			case TYPE_ADR:
				d = reg_of_var(m, iptr->dst, REG_ITMP1);
				x86_64_mov_membase_reg(REG_ITMP2, 0, d);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_FLT:
				d = reg_of_var(m, iptr->dst, REG_ITMP1);
				x86_64_movss_membase_reg(REG_ITMP2, 0, d);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			case TYPE_DBL:				
				d = reg_of_var(m, iptr->dst, REG_ITMP1);
				x86_64_movsd_membase_reg(REG_ITMP2, 0, d);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			default: panic("internal error");
			}
			break;

		case ICMD_PUTFIELD:   /* ..., value  ==> ...                          */
		                      /* op1 = type, val.i = field offset             */

			/* if class isn't yet initialized, do it */
  			if (!((fieldinfo *) iptr->val.a)->class->initialized) {
				/* call helper function which patches this code */
				x86_64_mov_imm_reg((s8) ((fieldinfo *) iptr->val.a)->class, REG_ITMP1);
				x86_64_mov_imm_reg((s8) asm_check_clinit, REG_ITMP2);
				x86_64_call_reg(REG_ITMP2);
  			}

			a = ((fieldinfo *)(iptr->val.a))->offset;
			var_to_reg_int(s1, src->prev, REG_ITMP1);
			switch (iptr->op1) {
				case TYPE_INT:
					var_to_reg_int(s2, src, REG_ITMP2);
					gen_nullptr_check(s1);
					x86_64_movl_reg_membase(s2, s1, a);
					break;
				case TYPE_LNG:
				case TYPE_ADR:
					var_to_reg_int(s2, src, REG_ITMP2);
					gen_nullptr_check(s1);
					x86_64_mov_reg_membase(s2, s1, a);
					break;
				case TYPE_FLT:
					var_to_reg_flt(s2, src, REG_FTMP2);
					gen_nullptr_check(s1);
					x86_64_movss_reg_membase(s2, s1, a);
					break;
				case TYPE_DBL:
					var_to_reg_flt(s2, src, REG_FTMP2);
					gen_nullptr_check(s1);
					x86_64_movsd_reg_membase(s2, s1, a);
					break;
				default: panic ("internal error");
				}
			break;

		case ICMD_GETFIELD:   /* ...  ==> ..., value                          */
		                      /* op1 = type, val.i = field offset             */

			a = ((fieldinfo *)(iptr->val.a))->offset;
			var_to_reg_int(s1, src, REG_ITMP1);
			switch (iptr->op1) {
				case TYPE_INT:
					d = reg_of_var(m, iptr->dst, REG_ITMP1);
					gen_nullptr_check(s1);
					x86_64_movl_membase_reg(s1, a, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
				case TYPE_LNG:
				case TYPE_ADR:
					d = reg_of_var(m, iptr->dst, REG_ITMP1);
					gen_nullptr_check(s1);
					x86_64_mov_membase_reg(s1, a, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
				case TYPE_FLT:
					d = reg_of_var(m, iptr->dst, REG_FTMP1);
					gen_nullptr_check(s1);
					x86_64_movss_membase_reg(s1, a, d);
   					store_reg_to_var_flt(iptr->dst, d);
					break;
				case TYPE_DBL:				
					d = reg_of_var(m, iptr->dst, REG_FTMP1);
					gen_nullptr_check(s1);
					x86_64_movsd_membase_reg(s1, a, d);
  					store_reg_to_var_flt(iptr->dst, d);
					break;
				default: panic ("internal error");
				}
			break;


		/* branch operations **************************************************/

/*  #define ALIGNCODENOP {if((int)((long)mcodeptr&7)){M_NOP;}} */
#define ALIGNCODENOP do {} while (0)

		case ICMD_ATHROW:       /* ..., objectref ==> ... (, objectref)       */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_INTMOVE(s1, REG_ITMP1_XPTR);

			x86_64_call_imm(0); /* passing exception pointer                  */
			x86_64_pop_reg(REG_ITMP2_XPC);

  			x86_64_mov_imm_reg((s8) asm_handle_exception, REG_ITMP3);
  			x86_64_jmp_reg(REG_ITMP3);
			ALIGNCODENOP;
			break;

		case ICMD_GOTO:         /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */

			x86_64_jmp_imm(0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
  			ALIGNCODENOP;
			break;

		case ICMD_JSR:          /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */

  			x86_64_call_imm(0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;
			
		case ICMD_RET:          /* ... ==> ...                                */
		                        /* op1 = local variable                       */

			var = &(r->locals[iptr->op1][TYPE_ADR]);
			var_to_reg_int(s1, var, REG_ITMP1);
			x86_64_jmp_reg(s1);
			break;

		case ICMD_IFNULL:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			if (src->flags & INMEMORY) {
				x86_64_alu_imm_membase(X86_64_CMP, 0, REG_SP, src->regoff * 8);

			} else {
				x86_64_test_reg_reg(src->regoff, src->regoff);
			}
			x86_64_jcc(X86_64_CC_E, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFNONNULL:    /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			if (src->flags & INMEMORY) {
				x86_64_alu_imm_membase(X86_64_CMP, 0, REG_SP, src->regoff * 8);

			} else {
				x86_64_test_reg_reg(src->regoff, src->regoff);
			}
			x86_64_jcc(X86_64_CC_NE, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFEQ:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			x86_64_emit_ifcc(X86_64_CC_E, src, iptr);
			break;

		case ICMD_IFLT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			x86_64_emit_ifcc(X86_64_CC_L, src, iptr);
			break;

		case ICMD_IFLE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			x86_64_emit_ifcc(X86_64_CC_LE, src, iptr);
			break;

		case ICMD_IFNE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			x86_64_emit_ifcc(X86_64_CC_NE, src, iptr);
			break;

		case ICMD_IFGT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			x86_64_emit_ifcc(X86_64_CC_G, src, iptr);
			break;

		case ICMD_IFGE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			x86_64_emit_ifcc(X86_64_CC_GE, src, iptr);
			break;

		case ICMD_IF_LEQ:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			x86_64_emit_if_lcc(X86_64_CC_E, src, iptr);
			break;

		case ICMD_IF_LLT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			x86_64_emit_if_lcc(X86_64_CC_L, src, iptr);
			break;

		case ICMD_IF_LLE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			x86_64_emit_if_lcc(X86_64_CC_LE, src, iptr);
			break;

		case ICMD_IF_LNE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			x86_64_emit_if_lcc(X86_64_CC_NE, src, iptr);
			break;

		case ICMD_IF_LGT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			x86_64_emit_if_lcc(X86_64_CC_G, src, iptr);
			break;

		case ICMD_IF_LGE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			x86_64_emit_if_lcc(X86_64_CC_GE, src, iptr);
			break;

		case ICMD_IF_ICMPEQ:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			x86_64_emit_if_icmpcc(X86_64_CC_E, src, iptr);
			break;

		case ICMD_IF_LCMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_ACMPEQ:    /* op1 = target JavaVM pc                     */

			x86_64_emit_if_lcmpcc(X86_64_CC_E, src, iptr);
			break;

		case ICMD_IF_ICMPNE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			x86_64_emit_if_icmpcc(X86_64_CC_NE, src, iptr);
			break;

		case ICMD_IF_LCMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_ACMPNE:    /* op1 = target JavaVM pc                     */

			x86_64_emit_if_lcmpcc(X86_64_CC_NE, src, iptr);
			break;

		case ICMD_IF_ICMPLT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			x86_64_emit_if_icmpcc(X86_64_CC_L, src, iptr);
			break;

		case ICMD_IF_LCMPLT:    /* ..., value, value ==> ...                  */
	                            /* op1 = target JavaVM pc                     */

			x86_64_emit_if_lcmpcc(X86_64_CC_L, src, iptr);
			break;

		case ICMD_IF_ICMPGT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			x86_64_emit_if_icmpcc(X86_64_CC_G, src, iptr);
			break;

		case ICMD_IF_LCMPGT:    /* ..., value, value ==> ...                  */
                                /* op1 = target JavaVM pc                     */

			x86_64_emit_if_lcmpcc(X86_64_CC_G, src, iptr);
			break;

		case ICMD_IF_ICMPLE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			x86_64_emit_if_icmpcc(X86_64_CC_LE, src, iptr);
			break;

		case ICMD_IF_LCMPLE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			x86_64_emit_if_lcmpcc(X86_64_CC_LE, src, iptr);
			break;

		case ICMD_IF_ICMPGE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			x86_64_emit_if_icmpcc(X86_64_CC_GE, src, iptr);
			break;

		case ICMD_IF_LCMPGE:    /* ..., value, value ==> ...                  */
	                            /* op1 = target JavaVM pc                     */

			x86_64_emit_if_lcmpcc(X86_64_CC_GE, src, iptr);
			break;

		/* (value xx 0) ? IFxx_ICONST : ELSE_ICONST                           */

		case ICMD_ELSE_ICONST:  /* handled by IFxx_ICONST                     */
			break;

		case ICMD_IFEQ_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			s3 = iptr->val.i;
			if (iptr[1].opc == ICMD_ELSE_ICONST) {
				if (s1 == d) {
					M_INTMOVE(s1, REG_ITMP1);
					s1 = REG_ITMP1;
				}
				x86_64_movl_imm_reg(iptr[1].val.i, d);
			}
			x86_64_movl_imm_reg(s3, REG_ITMP2);
			x86_64_testl_reg_reg(s1, s1);
			x86_64_cmovccl_reg_reg(X86_64_CC_E, REG_ITMP2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFNE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			s3 = iptr->val.i;
			if (iptr[1].opc == ICMD_ELSE_ICONST) {
				if (s1 == d) {
					M_INTMOVE(s1, REG_ITMP1);
					s1 = REG_ITMP1;
				}
				x86_64_movl_imm_reg(iptr[1].val.i, d);
			}
			x86_64_movl_imm_reg(s3, REG_ITMP2);
			x86_64_testl_reg_reg(s1, s1);
			x86_64_cmovccl_reg_reg(X86_64_CC_NE, REG_ITMP2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFLT_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			s3 = iptr->val.i;
			if (iptr[1].opc == ICMD_ELSE_ICONST) {
				if (s1 == d) {
					M_INTMOVE(s1, REG_ITMP1);
					s1 = REG_ITMP1;
				}
				x86_64_movl_imm_reg(iptr[1].val.i, d);
			}
			x86_64_movl_imm_reg(s3, REG_ITMP2);
			x86_64_testl_reg_reg(s1, s1);
			x86_64_cmovccl_reg_reg(X86_64_CC_L, REG_ITMP2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFGE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			s3 = iptr->val.i;
			if (iptr[1].opc == ICMD_ELSE_ICONST) {
				if (s1 == d) {
					M_INTMOVE(s1, REG_ITMP1);
					s1 = REG_ITMP1;
				}
				x86_64_movl_imm_reg(iptr[1].val.i, d);
			}
			x86_64_movl_imm_reg(s3, REG_ITMP2);
			x86_64_testl_reg_reg(s1, s1);
			x86_64_cmovccl_reg_reg(X86_64_CC_GE, REG_ITMP2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFGT_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			s3 = iptr->val.i;
			if (iptr[1].opc == ICMD_ELSE_ICONST) {
				if (s1 == d) {
					M_INTMOVE(s1, REG_ITMP1);
					s1 = REG_ITMP1;
				}
				x86_64_movl_imm_reg(iptr[1].val.i, d);
			}
			x86_64_movl_imm_reg(s3, REG_ITMP2);
			x86_64_testl_reg_reg(s1, s1);
			x86_64_cmovccl_reg_reg(X86_64_CC_G, REG_ITMP2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFLE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			s3 = iptr->val.i;
			if (iptr[1].opc == ICMD_ELSE_ICONST) {
				if (s1 == d) {
					M_INTMOVE(s1, REG_ITMP1);
					s1 = REG_ITMP1;
				}
				x86_64_movl_imm_reg(iptr[1].val.i, d);
			}
			x86_64_movl_imm_reg(s3, REG_ITMP2);
			x86_64_testl_reg_reg(s1, s1);
			x86_64_cmovccl_reg_reg(X86_64_CC_LE, REG_ITMP2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_IRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_LRETURN:
		case ICMD_ARETURN:

			var_to_reg_int(s1, src, REG_RESULT);
			M_INTMOVE(s1, REG_RESULT);

#if defined(USE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				x86_64_mov_membase_reg(REG_SP, r->maxmemuse * 8, r->argintregs[0]);
				x86_64_mov_reg_membase(REG_RESULT, REG_SP, r->maxmemuse * 8);
				x86_64_mov_imm_reg((u8) builtin_monitorexit, REG_ITMP1);
				x86_64_call_reg(REG_ITMP1);
				x86_64_mov_membase_reg(REG_SP, r->maxmemuse * 8, REG_RESULT);
			}
#endif

			goto nowperformreturn;

		case ICMD_FRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_DRETURN:

			var_to_reg_flt(s1, src, REG_FRESULT);
			M_FLTMOVE(s1, REG_FRESULT);

#if defined(USE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				x86_64_mov_membase_reg(REG_SP, r->maxmemuse * 8, r->argintregs[0]);
				x86_64_movq_reg_membase(REG_FRESULT, REG_SP, r->maxmemuse * 8);
				x86_64_mov_imm_reg((u8) builtin_monitorexit, REG_ITMP1);
				x86_64_call_reg(REG_ITMP1);
				x86_64_movq_membase_reg(REG_SP, r->maxmemuse * 8, REG_FRESULT);
			}
#endif

			goto nowperformreturn;

		case ICMD_RETURN:      /* ...  ==> ...                                */

#if defined(USE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				x86_64_mov_membase_reg(REG_SP, r->maxmemuse * 8, r->argintregs[0]);
				x86_64_mov_imm_reg((u8) builtin_monitorexit, REG_ITMP1);
				x86_64_call_reg(REG_ITMP1);
			}
#endif

nowperformreturn:
			{
			s4 i, p;
			
  			p = parentargs_base;
			
			/* call trace function */
			if (runverbose) {
				x86_64_alu_imm_reg(X86_64_SUB, 2 * 8, REG_SP);

				x86_64_mov_reg_membase(REG_RESULT, REG_SP, 0 * 8);
				x86_64_movq_reg_membase(REG_FRESULT, REG_SP, 1 * 8);

  				x86_64_mov_imm_reg((s8) m, r->argintregs[0]);
  				x86_64_mov_reg_reg(REG_RESULT, r->argintregs[1]);
				M_FLTMOVE(REG_FRESULT, r->argfltregs[0]);
 				M_FLTMOVE(REG_FRESULT, r->argfltregs[1]);

  				x86_64_mov_imm_reg((s8) builtin_displaymethodstop, REG_ITMP1);
				x86_64_call_reg(REG_ITMP1);

				x86_64_mov_membase_reg(REG_SP, 0 * 8, REG_RESULT);
				x86_64_movq_membase_reg(REG_SP, 1 * 8, REG_FRESULT);

				x86_64_alu_imm_reg(X86_64_ADD, 2 * 8, REG_SP);
			}

			/* restore saved registers                                        */
			for (i = r->savintregcnt - 1; i >= r->maxsavintreguse; i--) {
				p--; x86_64_mov_membase_reg(REG_SP, p * 8, r->savintregs[i]);
			}
			for (i = r->savfltregcnt - 1; i >= r->maxsavfltreguse; i--) {
  				p--; x86_64_movq_membase_reg(REG_SP, p * 8, r->savfltregs[i]);
			}

			/* deallocate stack                                               */
			if (parentargs_base) {
				x86_64_alu_imm_reg(X86_64_ADD, parentargs_base * 8, REG_SP);
			}

			x86_64_ret();
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
				M_INTMOVE(s1, REG_ITMP1);
				if (l != 0) {
					x86_64_alul_imm_reg(X86_64_SUB, l, REG_ITMP1);
				}
				i = i - l + 1;

                /* range check */
				x86_64_alul_imm_reg(X86_64_CMP, i - 1, REG_ITMP1);
				x86_64_jcc(X86_64_CC_A, 0);

                /* codegen_addreference(BlockPtrOfPC(s4ptr[0]), mcodeptr); */
				codegen_addreference((basicblock *) tptr[0], mcodeptr);

				/* build jump table top down and use address of lowest entry */

                /* s4ptr += 3 + i; */
				tptr += i;

				while (--i >= 0) {
					/* dseg_addtarget(BlockPtrOfPC(*--s4ptr)); */
					dseg_addtarget((basicblock *) tptr[0]); 
					--tptr;
				}

				/* length of dataseg after last dseg_addtarget is used by load */

				x86_64_mov_imm_reg(0, REG_ITMP2);
				dseg_adddata(mcodeptr);
				x86_64_mov_memindex_reg(-dseglen, REG_ITMP2, REG_ITMP1, 3, REG_ITMP1);
				x86_64_jmp_reg(REG_ITMP1);
				ALIGNCODENOP;
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
					x86_64_alul_imm_reg(X86_64_CMP, val, s1);
					x86_64_jcc(X86_64_CC_E, 0);
					/* codegen_addreference(BlockPtrOfPC(s4ptr[1]), mcodeptr); */
					codegen_addreference((basicblock *) tptr[0], mcodeptr); 
				}

				x86_64_jmp_imm(0);
				/* codegen_addreference(BlockPtrOfPC(l), mcodeptr); */
			
				tptr = (void **) iptr->target;
				codegen_addreference((basicblock *) tptr[0], mcodeptr);

				ALIGNCODENOP;
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
		                        /* op1 = arg count, val.a = method pointer    */

		case ICMD_INVOKEVIRTUAL:/* ..., objectref, [arg1, [arg2 ...]] ==> ... */
		                        /* op1 = arg count, val.a = method pointer    */

		case ICMD_INVOKEINTERFACE:/*.., objectref, [arg1, [arg2 ...]] ==> ... */
		                        /* op1 = arg count, val.a = method pointer    */

			s3 = iptr->op1;

gen_method: {
			methodinfo   *lm;
			classinfo    *ci;
			stackptr      tmpsrc;
			s4 iarg, farg;

			MCODECHECK((s3 << 1) + 64);

			tmpsrc = src;
			s2 = s3;
			iarg = 0;
			farg = 0;

			/* copy arguments to registers or stack location                  */
			for (; --s3 >= 0; src = src->prev) {
				IS_INT_LNG_TYPE(src->type) ? iarg++ : farg++;
			}

			src = tmpsrc;
			s3 = s2;

			s2 = (iarg > INT_ARG_CNT) ? iarg - INT_ARG_CNT : 0 + (farg > FLT_ARG_CNT) ? farg - FLT_ARG_CNT : 0;

			for (; --s3 >= 0; src = src->prev) {
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
						s1 = r->argintregs[iarg];
						var_to_reg_int(d, src, s1);
						M_INTMOVE(d, s1);

					} else {
						var_to_reg_int(d, src, REG_ITMP1);
						s2--;
						x86_64_mov_reg_membase(d, REG_SP, s2 * 8);
					}

				} else {
					if (farg < FLT_ARG_CNT) {
						s1 = r->argfltregs[farg];
						var_to_reg_flt(d, src, s1);
						M_FLTMOVE(d, s1);

					} else {
						var_to_reg_flt(d, src, REG_FTMP1);
						s2--;
						x86_64_movq_reg_membase(d, REG_SP, s2 * 8);
					}
				}
			} /* end of for */

			lm = iptr->val.a;
			switch (iptr->opc) {
				case ICMD_BUILTIN3:
				case ICMD_BUILTIN2:
				case ICMD_BUILTIN1:

					a = (s8) lm;
					d = iptr->op1;

					x86_64_mov_imm_reg(a, REG_ITMP1);
					x86_64_call_reg(REG_ITMP1);
					break;

				case ICMD_INVOKESTATIC:

					a = (s8) lm->stubroutine;
					d = lm->returntype;

					x86_64_mov_imm_reg(a, REG_ITMP2);
					x86_64_call_reg(REG_ITMP2);
					break;

				case ICMD_INVOKESPECIAL:

					a = (s8) lm->stubroutine;
					d = lm->returntype;

					gen_nullptr_check(r->argintregs[0]);    /* first argument contains pointer */
					x86_64_mov_membase_reg(r->argintregs[0], 0, REG_ITMP2); /* access memory for hardware nullptr */
					x86_64_mov_imm_reg(a, REG_ITMP2);
					x86_64_call_reg(REG_ITMP2);
					break;

				case ICMD_INVOKEVIRTUAL:

					d = lm->returntype;

					gen_nullptr_check(r->argintregs[0]);
					x86_64_mov_membase_reg(r->argintregs[0], OFFSET(java_objectheader, vftbl), REG_ITMP2);
					x86_64_mov_membase32_reg(REG_ITMP2, OFFSET(vftbl, table[0]) + sizeof(methodptr) * lm->vftblindex, REG_ITMP1);
					x86_64_call_reg(REG_ITMP1);
					break;

				case ICMD_INVOKEINTERFACE:

					ci = lm->class;
					d = lm->returntype;

					gen_nullptr_check(r->argintregs[0]);
					x86_64_mov_membase_reg(r->argintregs[0], OFFSET(java_objectheader, vftbl), REG_ITMP2);
					x86_64_mov_membase_reg(REG_ITMP2, OFFSET(vftbl, interfacetable[0]) - sizeof(methodptr) * ci->index, REG_ITMP2);
					x86_64_mov_membase32_reg(REG_ITMP2, sizeof(methodptr) * (lm - ci->methods), REG_ITMP1);
					x86_64_call_reg(REG_ITMP1);
					break;

				default:
					d = 0;
					error("Unkown ICMD-Command: %d", iptr->opc);
				}

			/* d contains return type */

			if (d != TYPE_VOID) {
				if (IS_INT_LNG_TYPE(iptr->dst->type)) {
					s1 = reg_of_var(m, iptr->dst, REG_RESULT);
					M_INTMOVE(REG_RESULT, s1);
					store_reg_to_var_int(iptr->dst, s1);

				} else {
					s1 = reg_of_var(m, iptr->dst, REG_FRESULT);
					M_FLTMOVE(REG_FRESULT, s1);
					store_reg_to_var_flt(iptr->dst, s1);
				}
			}
			}
			break;


		case ICMD_INSTANCEOF: /* ..., objectref ==> ..., intresult            */

		                      /* op1:   0 == array, 1 == class                */
		                      /* val.a: (classinfo*) superclass               */

/*          superclass is an interface:
 *
 *          return (sub != NULL) &&
 *                 (sub->vftbl->interfacetablelength > super->index) &&
 *                 (sub->vftbl->interfacetable[-super->index] != NULL);
 *
 *          superclass is a class:
 *
 *          return ((sub != NULL) && (0
 *                  <= (sub->vftbl->baseval - super->vftbl->baseval) <=
 *                  super->vftbl->diffvall));
 */

			{
			classinfo *super = (classinfo*) iptr->val.a;
			
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			if (s1 == d) {
				M_INTMOVE(s1, REG_ITMP1);
				s1 = REG_ITMP1;
			}
			x86_64_alu_reg_reg(X86_64_XOR, d, d);
			if (iptr->op1) {                               /* class/interface */
				if (super->flags & ACC_INTERFACE) {        /* interface       */
					x86_64_test_reg_reg(s1, s1);

					/* TODO: clean up this calculation */
					a = 3;    /* mov_membase_reg */
					CALCOFFSETBYTES(a, s1, OFFSET(java_objectheader, vftbl));

					a += 3;    /* movl_membase_reg - only if REG_ITMP2 == R10 */
					CALCOFFSETBYTES(a, REG_ITMP1, OFFSET(vftbl, interfacetablelength));
					
					a += 3;    /* sub */
					CALCIMMEDIATEBYTES(a, super->index);
					
					a += 3;    /* test */

					a += 6;    /* jcc */
					a += 3;    /* mov_membase_reg */
					CALCOFFSETBYTES(a, REG_ITMP1, OFFSET(vftbl, interfacetable[0]) - super->index * sizeof(methodptr*));

					a += 3;    /* test */
					a += 4;    /* setcc */

					x86_64_jcc(X86_64_CC_E, a);

					x86_64_mov_membase_reg(s1, OFFSET(java_objectheader, vftbl), REG_ITMP1);
					x86_64_movl_membase_reg(REG_ITMP1, OFFSET(vftbl, interfacetablelength), REG_ITMP2);
					x86_64_alu_imm_reg(X86_64_SUB, super->index, REG_ITMP2);
					x86_64_test_reg_reg(REG_ITMP2, REG_ITMP2);

					/* TODO: clean up this calculation */
					a = 0;
					a += 3;    /* mov_membase_reg */
					CALCOFFSETBYTES(a, REG_ITMP1, OFFSET(vftbl, interfacetable[0]) - super->index * sizeof(methodptr*));

					a += 3;    /* test */
					a += 4;    /* setcc */

					x86_64_jcc(X86_64_CC_LE, a);
					x86_64_mov_membase_reg(REG_ITMP1, OFFSET(vftbl, interfacetable[0]) - super->index * sizeof(methodptr*), REG_ITMP1);
					x86_64_test_reg_reg(REG_ITMP1, REG_ITMP1);
  					x86_64_setcc_reg(X86_64_CC_NE, d);

				} else {                                   /* class           */
					x86_64_test_reg_reg(s1, s1);

					/* TODO: clean up this calculation */
					a = 3;    /* mov_membase_reg */
					CALCOFFSETBYTES(a, s1, OFFSET(java_objectheader, vftbl));

					a += 10;   /* mov_imm_reg */

					a += 2;    /* movl_membase_reg - only if REG_ITMP1 == RAX */
					CALCOFFSETBYTES(a, REG_ITMP1, OFFSET(vftbl, baseval));
					
					a += 3;    /* movl_membase_reg - only if REG_ITMP2 == R10 */
					CALCOFFSETBYTES(a, REG_ITMP2, OFFSET(vftbl, baseval));
					
					a += 3;    /* movl_membase_reg - only if REG_ITMP2 == R10 */
					CALCOFFSETBYTES(a, REG_ITMP2, OFFSET(vftbl, diffval));
					
					a += 3;    /* sub */
					a += 3;    /* xor */
					a += 3;    /* cmp */
					a += 4;    /* setcc */

					x86_64_jcc(X86_64_CC_E, a);

					x86_64_mov_membase_reg(s1, OFFSET(java_objectheader, vftbl), REG_ITMP1);
					x86_64_mov_imm_reg((s8) super->vftbl, REG_ITMP2);
					x86_64_movl_membase_reg(REG_ITMP1, OFFSET(vftbl, baseval), REG_ITMP1);
					x86_64_movl_membase_reg(REG_ITMP2, OFFSET(vftbl, baseval), REG_ITMP3);
					x86_64_movl_membase_reg(REG_ITMP2, OFFSET(vftbl, diffval), REG_ITMP2);
					x86_64_alu_reg_reg(X86_64_SUB, REG_ITMP3, REG_ITMP1);
					x86_64_alu_reg_reg(X86_64_XOR, d, d);
					x86_64_alu_reg_reg(X86_64_CMP, REG_ITMP2, REG_ITMP1);
  					x86_64_setcc_reg(X86_64_CC_BE, d);
				}
			}
			else
				panic("internal error: no inlined array instanceof");
			}
  			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_CHECKCAST:  /* ..., objectref ==> ..., objectref            */

		                      /* op1:   0 == array, 1 == class                */
		                      /* val.a: (classinfo*) superclass               */

/*          superclass is an interface:
 *
 *          OK if ((sub == NULL) ||
 *                 (sub->vftbl->interfacetablelength > super->index) &&
 *                 (sub->vftbl->interfacetable[-super->index] != NULL));
 *
 *          superclass is a class:
 *
 *          OK if ((sub == NULL) || (0
 *                 <= (sub->vftbl->baseval - super->vftbl->baseval) <=
 *                 super->vftbl->diffvall));
 */

			{
			classinfo *super = (classinfo*) iptr->val.a;
			
			d = reg_of_var(m, iptr->dst, REG_ITMP3);
			var_to_reg_int(s1, src, d);
			if (iptr->op1) {                               /* class/interface */
				if (super->flags & ACC_INTERFACE) {        /* interface       */
					x86_64_test_reg_reg(s1, s1);

					/* TODO: clean up this calculation */
					a = 3;    /* mov_membase_reg */
					CALCOFFSETBYTES(a, s1, OFFSET(java_objectheader, vftbl));

					a += 3;    /* movl_membase_reg - only if REG_ITMP2 == R10 */
					CALCOFFSETBYTES(a, REG_ITMP1, OFFSET(vftbl, interfacetablelength));

					a += 3;    /* sub */
					CALCIMMEDIATEBYTES(a, super->index);

					a += 3;    /* test */
					a += 6;    /* jcc */

					a += 3;    /* mov_membase_reg */
					CALCOFFSETBYTES(a, REG_ITMP1, OFFSET(vftbl, interfacetable[0]) - super->index * sizeof(methodptr*));

					a += 3;    /* test */
					a += 6;    /* jcc */

					x86_64_jcc(X86_64_CC_E, a);

					x86_64_mov_membase_reg(s1, OFFSET(java_objectheader, vftbl), REG_ITMP1);
					x86_64_movl_membase_reg(REG_ITMP1, OFFSET(vftbl, interfacetablelength), REG_ITMP2);
					x86_64_alu_imm_reg(X86_64_SUB, super->index, REG_ITMP2);
					x86_64_test_reg_reg(REG_ITMP2, REG_ITMP2);
					x86_64_jcc(X86_64_CC_LE, 0);
					codegen_addxcastrefs(mcodeptr);
					x86_64_mov_membase_reg(REG_ITMP1, OFFSET(vftbl, interfacetable[0]) - super->index * sizeof(methodptr*), REG_ITMP2);
					x86_64_test_reg_reg(REG_ITMP2, REG_ITMP2);
					x86_64_jcc(X86_64_CC_E, 0);
					codegen_addxcastrefs(mcodeptr);

				} else {                                     /* class           */
					x86_64_test_reg_reg(s1, s1);

					/* TODO: clean up this calculation */
					a = 3;    /* mov_membase_reg */
					CALCOFFSETBYTES(a, s1, OFFSET(java_objectheader, vftbl));
					a += 10;   /* mov_imm_reg */
					a += 2;    /* movl_membase_reg - only if REG_ITMP1 == RAX */
					CALCOFFSETBYTES(a, REG_ITMP1, OFFSET(vftbl, baseval));

					if (d != REG_ITMP3) {
						a += 3;    /* movl_membase_reg - only if REG_ITMP2 == R10 */
						CALCOFFSETBYTES(a, REG_ITMP2, OFFSET(vftbl, baseval));
						a += 3;    /* movl_membase_reg - only if REG_ITMP2 == R10 */
						CALCOFFSETBYTES(a, REG_ITMP2, OFFSET(vftbl, diffval));
						a += 3;    /* sub */
						
					} else {
						a += 3;    /* movl_membase_reg - only if REG_ITMP2 == R10 */
						CALCOFFSETBYTES(a, REG_ITMP2, OFFSET(vftbl, baseval));
						a += 3;    /* sub */
						a += 10;   /* mov_imm_reg */
						a += 3;    /* movl_membase_reg - only if REG_ITMP2 == R10 */
						CALCOFFSETBYTES(a, REG_ITMP2, OFFSET(vftbl, diffval));
					}

					a += 3;    /* cmp */
					a += 6;    /* jcc */

					x86_64_jcc(X86_64_CC_E, a);

					x86_64_mov_membase_reg(s1, OFFSET(java_objectheader, vftbl), REG_ITMP1);
					x86_64_mov_imm_reg((s8) super->vftbl, REG_ITMP2);
					x86_64_movl_membase_reg(REG_ITMP1, OFFSET(vftbl, baseval), REG_ITMP1);
					if (d != REG_ITMP3) {
						x86_64_movl_membase_reg(REG_ITMP2, OFFSET(vftbl, baseval), REG_ITMP3);
						x86_64_movl_membase_reg(REG_ITMP2, OFFSET(vftbl, diffval), REG_ITMP2);
						x86_64_alu_reg_reg(X86_64_SUB, REG_ITMP3, REG_ITMP1);

					} else {
						x86_64_movl_membase_reg(REG_ITMP2, OFFSET(vftbl, baseval), REG_ITMP2);
						x86_64_alu_reg_reg(X86_64_SUB, REG_ITMP2, REG_ITMP1);
						x86_64_mov_imm_reg((s8) super->vftbl, REG_ITMP2);
						x86_64_movl_membase_reg(REG_ITMP2, OFFSET(vftbl, diffval), REG_ITMP2);
					}
					x86_64_alu_reg_reg(X86_64_CMP, REG_ITMP2, REG_ITMP1);
					x86_64_jcc(X86_64_CC_A, 0);    /* (u) REG_ITMP1 > (u) REG_ITMP2 -> jump */
					codegen_addxcastrefs(mcodeptr);
				}

			} else
				panic("internal error: no inlined array checkcast");
			}
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_CHECKASIZE:  /* ..., size ==> ..., size                     */

			if (src->flags & INMEMORY) {
				x86_64_alul_imm_membase(X86_64_CMP, 0, REG_SP, src->regoff * 8);
				
			} else {
				x86_64_testl_reg_reg(src->regoff, src->regoff);
			}
			x86_64_jcc(X86_64_CC_L, 0);
			codegen_addxcheckarefs(mcodeptr);
			break;

		case ICMD_CHECKEXCEPTION:    /* ... ==> ...                           */

			x86_64_test_reg_reg(REG_RESULT, REG_RESULT);
			x86_64_jcc(X86_64_CC_E, 0);
			codegen_addxexceptionrefs(mcodeptr);
			break;

		case ICMD_MULTIANEWARRAY:/* ..., cnt1, [cnt2, ...] ==> ..., arrayref  */
		                         /* op1 = dimension, val.a = array descriptor */

			/* check for negative sizes and copy sizes to stack if necessary  */

  			MCODECHECK((iptr->op1 << 1) + 64);

			for (s1 = iptr->op1; --s1 >= 0; src = src->prev) {
				var_to_reg_int(s2, src, REG_ITMP1);
				x86_64_testl_reg_reg(s2, s2);
				x86_64_jcc(X86_64_CC_L, 0);
				codegen_addxcheckarefs(mcodeptr);

				/* copy sizes to stack (argument numbers >= INT_ARG_CNT)      */

				if (src->varkind != ARGVAR) {
					x86_64_mov_reg_membase(s2, REG_SP, (s1 + INT_ARG_CNT) * 8);
				}
			}

			/* a0 = dimension count */
			x86_64_mov_imm_reg(iptr->op1, r->argintregs[0]);

			/* a1 = arraydescriptor */
			x86_64_mov_imm_reg((s8) iptr->val.a, r->argintregs[1]);

			/* a2 = pointer to dimensions = stack pointer */
			x86_64_mov_reg_reg(REG_SP, r->argintregs[2]);

			x86_64_mov_imm_reg((s8) builtin_nmultianewarray, REG_ITMP1);
			x86_64_call_reg(REG_ITMP1);

			s1 = reg_of_var(m, iptr->dst, REG_RESULT);
			M_INTMOVE(REG_RESULT, s1);
			store_reg_to_var_int(iptr->dst, s1);
			break;

		default: error("Unknown pseudo command: %d", iptr->opc);
	} /* switch */
		
	} /* for instruction */
		
	/* copy values to interface registers */

	src = bptr->outstack;
	len = bptr->outdepth;
	MCODECHECK(64 + len);
	while (src) {
		len--;
		if ((src->varkind != STACKVAR)) {
			s2 = src->type;
			if (IS_FLT_DBL_TYPE(s2)) {
				var_to_reg_flt(s1, src, REG_FTMP1);
				if (!(r->interfaces[len][s2].flags & INMEMORY)) {
					M_FLTMOVE(s1, r->interfaces[len][s2].regoff);

				} else {
					x86_64_movq_reg_membase(s1, REG_SP, r->interfaces[len][s2].regoff * 8);
				}

			} else {
				var_to_reg_int(s1, src, REG_ITMP1);
				if (!(r->interfaces[len][s2].flags & INMEMORY)) {
					M_INTMOVE(s1, r->interfaces[len][s2].regoff);

				} else {
					x86_64_mov_reg_membase(s1, REG_SP, r->interfaces[len][s2].regoff * 8);
				}
			}
		}
		src = src->prev;
	}
	} /* if (bptr -> flags >= BBREACHED) */
	} /* for basic block */

	/* bptr -> mpc = (int)((u1*) mcodeptr - mcodebase); */

	{

	/* generate bound check stubs */

	u1 *xcodeptr = NULL;
	
	for (; xboundrefs != NULL; xboundrefs = xboundrefs->next) {
		gen_resolvebranch(mcodebase + xboundrefs->branchpos, 
		                  xboundrefs->branchpos,
						  mcodeptr - mcodebase);

		MCODECHECK(50);

		/* move index register into REG_ITMP1 */
		x86_64_mov_reg_reg(xboundrefs->reg, REG_ITMP1);              /* 3 bytes  */

		x86_64_mov_imm_reg(0, REG_ITMP2_XPC);                        /* 10 bytes */
		dseg_adddata(mcodeptr);
		x86_64_mov_imm_reg(xboundrefs->branchpos - 6, REG_ITMP3);    /* 10 bytes */
		x86_64_alu_reg_reg(X86_64_ADD, REG_ITMP3, REG_ITMP2_XPC);    /* 3 bytes  */

		if (xcodeptr != NULL) {
			x86_64_jmp_imm(xcodeptr - mcodeptr - 5);

		} else {
			xcodeptr = mcodeptr;

			x86_64_alu_imm_reg(X86_64_SUB, 2 * 8, REG_SP);
			x86_64_mov_reg_membase(REG_ITMP2_XPC, REG_SP, 0 * 8);
			x86_64_mov_imm_reg((s8) string_java_lang_ArrayIndexOutOfBoundsException, r->argintregs[0]);
			x86_64_mov_reg_reg(REG_ITMP1, r->argintregs[1]);
			x86_64_mov_imm_reg((s8) new_exception_int, REG_ITMP3);
			x86_64_call_reg(REG_ITMP3);
			x86_64_mov_membase_reg(REG_SP, 0 * 8, REG_ITMP2_XPC);
			x86_64_alu_imm_reg(X86_64_ADD, 2 * 8, REG_SP);

			x86_64_mov_imm_reg((s8) asm_handle_exception, REG_ITMP3);
			x86_64_jmp_reg(REG_ITMP3);
		}
	}

	/* generate negative array size check stubs */

	xcodeptr = NULL;
	
	for (; xcheckarefs != NULL; xcheckarefs = xcheckarefs->next) {
		if ((m->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch(mcodebase + xcheckarefs->branchpos, 
							  xcheckarefs->branchpos,
							  xcodeptr - mcodebase - (10 + 10 + 3));
			continue;
		}

		gen_resolvebranch(mcodebase + xcheckarefs->branchpos, 
		                  xcheckarefs->branchpos,
						  mcodeptr - mcodebase);

		MCODECHECK(50);

		x86_64_mov_imm_reg(0, REG_ITMP2_XPC);                         /* 10 bytes */
		dseg_adddata(mcodeptr);
		x86_64_mov_imm_reg(xcheckarefs->branchpos - 6, REG_ITMP3);    /* 10 bytes */
		x86_64_alu_reg_reg(X86_64_ADD, REG_ITMP3, REG_ITMP2_XPC);     /* 3 bytes  */

		if (xcodeptr != NULL) {
			x86_64_jmp_imm(xcodeptr - mcodeptr - 5);

		} else {
			xcodeptr = mcodeptr;

			x86_64_alu_imm_reg(X86_64_SUB, 2 * 8, REG_SP);
			x86_64_mov_reg_membase(REG_ITMP2_XPC, REG_SP, 0 * 8);
			x86_64_mov_imm_reg((s8) string_java_lang_NegativeArraySizeException, r->argintregs[0]);
			x86_64_mov_imm_reg((s8) new_exception, REG_ITMP3);
			x86_64_call_reg(REG_ITMP3);
			x86_64_mov_membase_reg(REG_SP, 0 * 8, REG_ITMP2_XPC);
			x86_64_alu_imm_reg(X86_64_ADD, 2 * 8, REG_SP);

			x86_64_mov_imm_reg((s8) asm_handle_exception, REG_ITMP3);
			x86_64_jmp_reg(REG_ITMP3);
		}
	}

	/* generate cast check stubs */

	xcodeptr = NULL;
	
	for (; xcastrefs != NULL; xcastrefs = xcastrefs->next) {
		if ((m->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch(mcodebase + xcastrefs->branchpos, 
							  xcastrefs->branchpos,
							  xcodeptr - mcodebase - (10 + 10 + 3));
			continue;
		}

		gen_resolvebranch(mcodebase + xcastrefs->branchpos, 
		                  xcastrefs->branchpos,
						  mcodeptr - mcodebase);

		MCODECHECK(50);

		x86_64_mov_imm_reg(0, REG_ITMP2_XPC);                        /* 10 bytes */
		dseg_adddata(mcodeptr);
		x86_64_mov_imm_reg(xcastrefs->branchpos - 6, REG_ITMP3);     /* 10 bytes */
		x86_64_alu_reg_reg(X86_64_ADD, REG_ITMP3, REG_ITMP2_XPC);    /* 3 bytes  */

		if (xcodeptr != NULL) {
			x86_64_jmp_imm(xcodeptr - mcodeptr - 5);
		
		} else {
			xcodeptr = mcodeptr;

			x86_64_alu_imm_reg(X86_64_SUB, 2 * 8, REG_SP);
			x86_64_mov_reg_membase(REG_ITMP2_XPC, REG_SP, 0 * 8);
			x86_64_mov_imm_reg((s8) string_java_lang_ClassCastException, r->argintregs[0]);
			x86_64_mov_imm_reg((s8) new_exception, REG_ITMP3);
			x86_64_call_reg(REG_ITMP3);
			x86_64_mov_membase_reg(REG_SP, 0 * 8, REG_ITMP2_XPC);
			x86_64_alu_imm_reg(X86_64_ADD, 2 * 8, REG_SP);

			x86_64_mov_imm_reg((s8) asm_handle_exception, REG_ITMP3);
			x86_64_jmp_reg(REG_ITMP3);
		}
	}

	/* generate divide by zero check stubs */

	xcodeptr = NULL;
	
	for (; xdivrefs != NULL; xdivrefs = xdivrefs->next) {
		if ((m->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch(mcodebase + xdivrefs->branchpos, 
							  xdivrefs->branchpos,
							  xcodeptr - mcodebase - (10 + 10 + 3));
			continue;
		}

		gen_resolvebranch(mcodebase + xdivrefs->branchpos, 
		                  xdivrefs->branchpos,
						  mcodeptr - mcodebase);

		MCODECHECK(50);

		x86_64_mov_imm_reg(0, REG_ITMP2_XPC);                        /* 10 bytes */
		dseg_adddata(mcodeptr);
		x86_64_mov_imm_reg(xdivrefs->branchpos - 6, REG_ITMP3);      /* 10 bytes */
		x86_64_alu_reg_reg(X86_64_ADD, REG_ITMP3, REG_ITMP2_XPC);    /* 3 bytes  */

		if (xcodeptr != NULL) {
			x86_64_jmp_imm(xcodeptr - mcodeptr - 5);
		
		} else {
			xcodeptr = mcodeptr;

			x86_64_alu_imm_reg(X86_64_SUB, 2 * 8, REG_SP);
			x86_64_mov_reg_membase(REG_ITMP2_XPC, REG_SP, 0 * 8);
			x86_64_mov_imm_reg((u8) string_java_lang_ArithmeticException, r->argintregs[0]);
			x86_64_mov_imm_reg((u8) string_java_lang_ArithmeticException_message, r->argintregs[1]);
			x86_64_mov_imm_reg((u8) new_exception, REG_ITMP3);
			x86_64_call_reg(REG_ITMP3);
			x86_64_mov_membase_reg(REG_SP, 0 * 8, REG_ITMP2_XPC);
			x86_64_alu_imm_reg(X86_64_ADD, 2 * 8, REG_SP);

			x86_64_mov_imm_reg((u8) asm_handle_exception, REG_ITMP3);
			x86_64_jmp_reg(REG_ITMP3);
		}
	}

	/* generate exception check stubs */

	xcodeptr = NULL;
	
	for (; xexceptionrefs != NULL; xexceptionrefs = xexceptionrefs->next) {
		if ((m->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch(mcodebase + xexceptionrefs->branchpos, 
							  xexceptionrefs->branchpos,
							  xcodeptr - mcodebase - (10 + 10 + 3));
			continue;
		}

		gen_resolvebranch(mcodebase + xexceptionrefs->branchpos, 
		                  xexceptionrefs->branchpos,
						  mcodeptr - mcodebase);

		MCODECHECK(50);

		x86_64_mov_imm_reg(0, REG_ITMP2_XPC);                        /* 10 bytes */
		dseg_adddata(mcodeptr);
		x86_64_mov_imm_reg(xexceptionrefs->branchpos - 6, REG_ITMP1);     /* 10 bytes */
		x86_64_alu_reg_reg(X86_64_ADD, REG_ITMP1, REG_ITMP2_XPC);    /* 3 bytes  */

		if (xcodeptr != NULL) {
			x86_64_jmp_imm(xcodeptr - mcodeptr - 5);
		
		} else {
			xcodeptr = mcodeptr;

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			x86_64_alu_imm_reg(X86_64_SUB, 8, REG_SP);
			x86_64_mov_reg_membase(REG_ITMP2_XPC, REG_SP, 0);
			x86_64_mov_imm_reg((u8) &builtin_get_exceptionptrptr, REG_ITMP1);
			x86_64_call_reg(REG_ITMP1);
			x86_64_mov_membase_reg(REG_RESULT, 0, REG_ITMP3);
			x86_64_mov_imm_membase(0, REG_RESULT, 0);
			x86_64_mov_reg_reg(REG_ITMP3, REG_ITMP1_XPTR);
			x86_64_mov_membase_reg(REG_SP, 0, REG_ITMP2_XPC);
			x86_64_alu_imm_reg(X86_64_ADD, 8, REG_SP);
#else
			x86_64_mov_imm_reg((u8) &_exceptionptr, REG_ITMP3);
			x86_64_mov_membase_reg(REG_ITMP3, 0, REG_ITMP1_XPTR);
			x86_64_mov_imm_membase(0, REG_ITMP3, 0);
#endif

			x86_64_mov_imm_reg((u8) asm_handle_exception, REG_ITMP3);
			x86_64_jmp_reg(REG_ITMP3);
		}
	}

	/* generate null pointer check stubs */

	xcodeptr = NULL;
	
	for (; xnullrefs != NULL; xnullrefs = xnullrefs->next) {
		if ((m->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch(mcodebase + xnullrefs->branchpos, 
							  xnullrefs->branchpos,
							  xcodeptr - mcodebase - (10 + 10 + 3));
			continue;
		}

		gen_resolvebranch(mcodebase + xnullrefs->branchpos, 
		                  xnullrefs->branchpos,
						  mcodeptr - mcodebase);

		MCODECHECK(50);

		x86_64_mov_imm_reg(0, REG_ITMP2_XPC);                        /* 10 bytes */
		dseg_adddata(mcodeptr);
		x86_64_mov_imm_reg(xnullrefs->branchpos - 6, REG_ITMP1);     /* 10 bytes */
		x86_64_alu_reg_reg(X86_64_ADD, REG_ITMP1, REG_ITMP2_XPC);    /* 3 bytes  */

		if (xcodeptr != NULL) {
			x86_64_jmp_imm(xcodeptr - mcodeptr - 5);
		
		} else {
			xcodeptr = mcodeptr;

			x86_64_alu_imm_reg(X86_64_SUB, 2 * 8, REG_SP);
			x86_64_mov_reg_membase(REG_ITMP2_XPC, REG_SP, 0 * 8);
			x86_64_mov_imm_reg((s8) string_java_lang_NullPointerException, r->argintregs[0]);
			x86_64_mov_imm_reg((s8) new_exception, REG_ITMP3);
			x86_64_call_reg(REG_ITMP3);
			x86_64_mov_membase_reg(REG_SP, 0 * 8, REG_ITMP2_XPC);
			x86_64_alu_imm_reg(X86_64_ADD, 2 * 8, REG_SP);

			x86_64_mov_imm_reg((s8) asm_handle_exception, REG_ITMP3);
			x86_64_jmp_reg(REG_ITMP3);
		}
	}
	}

	codegen_finish(m, (s4) ((u1 *) mcodeptr - mcodebase));
}


/* function createcompilerstub *************************************************

	creates a stub routine which calls the compiler
	
*******************************************************************************/

#define COMPSTUBSIZE 23

u1 *createcompilerstub(methodinfo *m)
{
	u1 *s = CNEW(u1, COMPSTUBSIZE);     /* memory to hold the stub            */
	mcodeptr = s;                       /* code generation pointer            */

	                                    /* code for the stub                  */
	x86_64_mov_imm_reg((s8) m, REG_ITMP1); /* pass method pointer to compiler */
	x86_64_mov_imm_reg((s8) asm_call_jit_compiler, REG_ITMP3);/* load address */
	x86_64_jmp_reg(REG_ITMP3);          /* jump to compiler                   */

#if defined(STATISTICS)
	if (opt_stat)
		count_cstub_len += COMPSTUBSIZE;
#endif

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

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
static java_objectheader **(*callgetexceptionptrptr)() = builtin_get_exceptionptrptr;
#endif

#define NATIVESTUBSIZE 420

u1 *createnativestub(functionptr f, methodinfo *m)
{
	u1 *s = CNEW(u1, NATIVESTUBSIZE);   /* memory to hold the stub            */
	s4 stackframesize;                  /* size of stackframe if needed       */
	registerdata *r;
	mcodeptr = s;                       /* make macros work                   */

	/* initialize registers before using it */
	reg_init(m);

	/* keep code size smaller */
	r = m->registerdata;

    descriptor2types(m);                /* set paramcount and paramtypes      */

	/* if function is static, check for initialized */

	if (m->flags & ACC_STATIC) {
		/* if class isn't yet initialized, do it */
		if (!m->class->initialized) {
			/* call helper function which patches this code */
			x86_64_mov_imm_reg((u8) m->class, REG_ITMP1);
			x86_64_mov_imm_reg((u8) asm_check_clinit, REG_ITMP2);
			x86_64_call_reg(REG_ITMP2);
		}
	}

	if (runverbose) {
		s4 p, l, s1;

		x86_64_alu_imm_reg(X86_64_SUB, (INT_ARG_CNT + FLT_ARG_CNT + 1) * 8, REG_SP);

		x86_64_mov_reg_membase(r->argintregs[0], REG_SP, 1 * 8);
		x86_64_mov_reg_membase(r->argintregs[1], REG_SP, 2 * 8);
		x86_64_mov_reg_membase(r->argintregs[2], REG_SP, 3 * 8);
		x86_64_mov_reg_membase(r->argintregs[3], REG_SP, 4 * 8);
		x86_64_mov_reg_membase(r->argintregs[4], REG_SP, 5 * 8);
		x86_64_mov_reg_membase(r->argintregs[5], REG_SP, 6 * 8);

		x86_64_movq_reg_membase(r->argfltregs[0], REG_SP, 7 * 8);
		x86_64_movq_reg_membase(r->argfltregs[1], REG_SP, 8 * 8);
		x86_64_movq_reg_membase(r->argfltregs[2], REG_SP, 9 * 8);
		x86_64_movq_reg_membase(r->argfltregs[3], REG_SP, 10 * 8);
/*  		x86_64_movq_reg_membase(r->argfltregs[4], REG_SP, 11 * 8); */
/*  		x86_64_movq_reg_membase(r->argfltregs[5], REG_SP, 12 * 8); */
/*  		x86_64_movq_reg_membase(r->argfltregs[6], REG_SP, 13 * 8); */
/*  		x86_64_movq_reg_membase(r->argfltregs[7], REG_SP, 14 * 8); */

		/* show integer hex code for float arguments */
		for (p = 0, l = 0; p < m->paramcount; p++) {
			if (IS_FLT_DBL_TYPE(m->paramtypes[p])) {
				for (s1 = (m->paramcount > INT_ARG_CNT) ? INT_ARG_CNT - 2 : m->paramcount - 2; s1 >= p; s1--) {
					x86_64_mov_reg_reg(r->argintregs[s1], r->argintregs[s1 + 1]);
				}

				x86_64_movd_freg_reg(r->argfltregs[l], r->argintregs[p]);
				l++;
			}
		}

		x86_64_mov_imm_reg((s8) m, REG_ITMP1);
		x86_64_mov_reg_membase(REG_ITMP1, REG_SP, 0 * 8);
  		x86_64_mov_imm_reg((s8) builtin_trace_args, REG_ITMP1);
		x86_64_call_reg(REG_ITMP1);

		x86_64_mov_membase_reg(REG_SP, 1 * 8, r->argintregs[0]);
		x86_64_mov_membase_reg(REG_SP, 2 * 8, r->argintregs[1]);
		x86_64_mov_membase_reg(REG_SP, 3 * 8, r->argintregs[2]);
		x86_64_mov_membase_reg(REG_SP, 4 * 8, r->argintregs[3]);
		x86_64_mov_membase_reg(REG_SP, 5 * 8, r->argintregs[4]);
		x86_64_mov_membase_reg(REG_SP, 6 * 8, r->argintregs[5]);

		x86_64_movq_membase_reg(REG_SP, 7 * 8, r->argfltregs[0]);
		x86_64_movq_membase_reg(REG_SP, 8 * 8, r->argfltregs[1]);
		x86_64_movq_membase_reg(REG_SP, 9 * 8, r->argfltregs[2]);
		x86_64_movq_membase_reg(REG_SP, 10 * 8, r->argfltregs[3]);
/*  		x86_64_movq_membase_reg(REG_SP, 11 * 8, r->argfltregs[4]); */
/*  		x86_64_movq_membase_reg(REG_SP, 12 * 8, r->argfltregs[5]); */
/*  		x86_64_movq_membase_reg(REG_SP, 13 * 8, r->argfltregs[6]); */
/*  		x86_64_movq_membase_reg(REG_SP, 14 * 8, r->argfltregs[7]); */

		x86_64_alu_imm_reg(X86_64_ADD, (INT_ARG_CNT + FLT_ARG_CNT + 1) * 8, REG_SP);
	}

#if 0
	x86_64_alu_imm_reg(X86_64_SUB, 7 * 8, REG_SP);    /* keep stack 16-byte aligned */

	/* save callee saved float registers */
	x86_64_movq_reg_membase(XMM15, REG_SP, 0 * 8);
	x86_64_movq_reg_membase(XMM14, REG_SP, 1 * 8);
	x86_64_movq_reg_membase(XMM13, REG_SP, 2 * 8);
	x86_64_movq_reg_membase(XMM12, REG_SP, 3 * 8);
	x86_64_movq_reg_membase(XMM11, REG_SP, 4 * 8);
	x86_64_movq_reg_membase(XMM10, REG_SP, 5 * 8);
#endif

	/* save argument registers on stack -- if we have to */
	if ((m->flags & ACC_STATIC && m->paramcount > (INT_ARG_CNT - 2)) || m->paramcount > (INT_ARG_CNT - 1)) {
		s4 i;
		s4 paramshiftcnt = (m->flags & ACC_STATIC) ? 2 : 1;
		s4 stackparamcnt = (m->paramcount > INT_ARG_CNT) ? m->paramcount - INT_ARG_CNT : 0;

		stackframesize = stackparamcnt + paramshiftcnt;

		/* keep stack 16-byte aligned */
		if ((stackframesize % 2) == 0) stackframesize++;

		x86_64_alu_imm_reg(X86_64_SUB, stackframesize * 8, REG_SP);

		/* copy stack arguments into new stack frame -- if any */
		for (i = 0; i < stackparamcnt; i++) {
			x86_64_mov_membase_reg(REG_SP, (stackparamcnt + 1 + i) * 8, REG_ITMP1);
			x86_64_mov_reg_membase(REG_ITMP1, REG_SP, (paramshiftcnt + i) * 8);
		}

		if (m->flags & ACC_STATIC) {
			x86_64_mov_reg_membase(r->argintregs[5], REG_SP, 1 * 8);
			x86_64_mov_reg_membase(r->argintregs[4], REG_SP, 0 * 8);

		} else {
			x86_64_mov_reg_membase(r->argintregs[5], REG_SP, 0 * 8);
		}

	} else {
		/* keep stack 16-byte aligned -- this is essential for x86_64 */
		x86_64_alu_imm_reg(X86_64_SUB, 8, REG_SP);
		stackframesize = 1;
	}

	if (m->flags & ACC_STATIC) {
		x86_64_mov_reg_reg(r->argintregs[3], r->argintregs[5]);
		x86_64_mov_reg_reg(r->argintregs[2], r->argintregs[4]);
		x86_64_mov_reg_reg(r->argintregs[1], r->argintregs[3]);
		x86_64_mov_reg_reg(r->argintregs[0], r->argintregs[2]);

		/* put class into second argument register */
		x86_64_mov_imm_reg((u8) m->class, r->argintregs[1]);

	} else {
		x86_64_mov_reg_reg(r->argintregs[4], r->argintregs[5]);
		x86_64_mov_reg_reg(r->argintregs[3], r->argintregs[4]);
		x86_64_mov_reg_reg(r->argintregs[2], r->argintregs[3]);
		x86_64_mov_reg_reg(r->argintregs[1], r->argintregs[2]);
		x86_64_mov_reg_reg(r->argintregs[0], r->argintregs[1]);
	}

	/* put env into first argument register */
	x86_64_mov_imm_reg((u8) &env, r->argintregs[0]);

	x86_64_mov_imm_reg((u8) f, REG_ITMP1);
	x86_64_call_reg(REG_ITMP1);

	/* remove stackframe if there is one */
	if (stackframesize) {
		x86_64_alu_imm_reg(X86_64_ADD, stackframesize * 8, REG_SP);
	}

	if (runverbose) {
		x86_64_alu_imm_reg(X86_64_SUB, 3 * 8, REG_SP);    /* keep stack 16-byte aligned */

		x86_64_mov_reg_membase(REG_RESULT, REG_SP, 0 * 8);
		x86_64_movq_reg_membase(REG_FRESULT, REG_SP, 1 * 8);

  		x86_64_mov_imm_reg((u8) m, r->argintregs[0]);
  		x86_64_mov_reg_reg(REG_RESULT, r->argintregs[1]);
		M_FLTMOVE(REG_FRESULT, r->argfltregs[0]);
  		M_FLTMOVE(REG_FRESULT, r->argfltregs[1]);

  		x86_64_mov_imm_reg((u8) builtin_displaymethodstop, REG_ITMP1);
		x86_64_call_reg(REG_ITMP1);

		x86_64_mov_membase_reg(REG_SP, 0 * 8, REG_RESULT);
		x86_64_movq_membase_reg(REG_SP, 1 * 8, REG_FRESULT);

		x86_64_alu_imm_reg(X86_64_ADD, 3 * 8, REG_SP);    /* keep stack 16-byte aligned */
	}

#if 0
	/* restore callee saved registers */
	x86_64_movq_membase_reg(REG_SP, 0 * 8, XMM15);
	x86_64_movq_membase_reg(REG_SP, 1 * 8, XMM14);
	x86_64_movq_membase_reg(REG_SP, 2 * 8, XMM13);
	x86_64_movq_membase_reg(REG_SP, 3 * 8, XMM12);
	x86_64_movq_membase_reg(REG_SP, 4 * 8, XMM11);
	x86_64_movq_membase_reg(REG_SP, 5 * 8, XMM10);

	x86_64_alu_imm_reg(X86_64_ADD, 7 * 8, REG_SP);    /* keep stack 16-byte aligned */
#endif

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	x86_64_push_reg(REG_RESULT);
/* 	x86_64_call_mem((u8) &callgetexceptionptrptr); */
	x86_64_mov_imm_reg((u8) builtin_get_exceptionptrptr, REG_ITMP3);
	x86_64_call_reg(REG_ITMP3);
	x86_64_mov_membase_reg(REG_RESULT, 0, REG_ITMP3);
	x86_64_pop_reg(REG_RESULT);
#else
	x86_64_mov_imm_reg((s8) &_exceptionptr, REG_ITMP3);
	x86_64_mov_membase_reg(REG_ITMP3, 0, REG_ITMP3);
#endif
	x86_64_test_reg_reg(REG_ITMP3, REG_ITMP3);
	x86_64_jcc(X86_64_CC_NE, 1);

	x86_64_ret();

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	x86_64_push_reg(REG_ITMP3);
/* 	x86_64_call_mem((u8) &callgetexceptionptrptr); */
	x86_64_mov_imm_reg((u8) builtin_get_exceptionptrptr, REG_ITMP3);
	x86_64_call_reg(REG_ITMP3);
	x86_64_mov_imm_membase(0, REG_RESULT, 0);
	x86_64_pop_reg(REG_ITMP1_XPTR);
#else
	x86_64_mov_reg_reg(REG_ITMP3, REG_ITMP1_XPTR);
	x86_64_mov_imm_reg((s8) &_exceptionptr, REG_ITMP3);
	x86_64_alu_reg_reg(X86_64_XOR, REG_ITMP2, REG_ITMP2);
	x86_64_mov_reg_membase(REG_ITMP2, REG_ITMP3, 0);    /* clear exception pointer */
#endif

	x86_64_mov_membase_reg(REG_SP, 0, REG_ITMP2_XPC);    /* get return address from stack */
	x86_64_alu_imm_reg(X86_64_SUB, 3, REG_ITMP2_XPC);    /* callq */

	x86_64_mov_imm_reg((s8) asm_handle_nat_exception, REG_ITMP3);
	x86_64_jmp_reg(REG_ITMP3);

#if 0
	{
		static int stubprinted;
		if (!stubprinted)
			printf("stubsize: %d\n", ((long)mcodeptr - (long) s));
		stubprinted = 1;
	}
#endif

#if defined(STATISTICS)
	if (opt_stat)
		count_nstub_len += NATIVESTUBSIZE;
#endif

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
