/* vm/jit/i386/codegen.c - machine code generator for i386

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

   $Id: codegen.c 1735 2004-12-07 14:33:27Z twisti $

*/


#define _GNU_SOURCE

#include <stdio.h>
#include <ucontext.h>
#ifdef __FreeBSD__
#include <machine/signal.h>
#endif

#include "config.h"
#include "native/jni.h"
#include "native/native.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/tables.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/jit.h"
#include "vm/jit/parse.h"
#include "vm/jit/reg.h"
#include "vm/jit/i386/codegen.h"
#include "vm/jit/i386/emitfuncs.h"
#include "vm/jit/i386/types.h"
#include "vm/jit/i386/asmoffsets.h"


/* register descripton - array ************************************************/

/* #define REG_RES   0         reserved register for OS or code generator     */
/* #define REG_RET   1         return value register                          */
/* #define REG_EXC   2         exception value register (only old jit)        */
/* #define REG_SAV   3         (callee) saved register                        */
/* #define REG_TMP   4         scratch temporary register (caller saved)      */
/* #define REG_ARG   5         argument register (caller saved)               */

/* #define REG_END   -1        last entry in tables */

static int nregdescint[] = {
    REG_RET, REG_RES, REG_RES, REG_TMP, REG_RES, REG_SAV, REG_SAV, REG_SAV,
    REG_END
};


static int nregdescfloat[] = {
  /* rounding problems with callee saved registers */
/*      REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_TMP, REG_TMP, REG_RES, REG_RES, */
/*      REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_RES, REG_RES, */
    REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES,
    REG_END
};


/*******************************************************************************

    include independent code generation stuff -- include after register
    descriptions to avoid extern definitions

*******************************************************************************/

#include "vm/jit/codegen.inc"
#include "vm/jit/reg.inc"
#ifdef LSRA
#include "vm/jit/lsra.inc"
#endif

void codegen_stubcalled() {
	log_text("Stub has been called");
}

void codegen_general_stubcalled() {
	log_text("general exception stub  has been called");
}


#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void thread_restartcriticalsection(ucontext_t *uc)
{
	void *critical;
#ifdef __FreeBSD__
	if ((critical = thread_checkcritical((void*) uc->uc_mcontext.mc_eip)) != NULL)
		uc->uc_mcontext.mc_eip = (u4) critical;
#else
	if ((critical = thread_checkcritical((void*) uc->uc_mcontext.gregs[REG_EIP])) != NULL)
		uc->uc_mcontext.gregs[REG_EIP] = (u4) critical;

#endif
}
#endif



/* NullPointerException signal handler for hardware null pointer check */

void catch_NullPointerException(int sig, siginfo_t *siginfo, void *_p)
{
	sigset_t nsig;
/*  	long     faultaddr; */

#ifdef __FreeBSD__
	ucontext_t *_uc = (ucontext_t *) _p;
	mcontext_t *sigctx = (mcontext_t *) &_uc->uc_mcontext;
#else
	struct ucontext *_uc = (struct ucontext *) _p;
	struct sigcontext *sigctx = (struct sigcontext *) &_uc->uc_mcontext;
#endif
	struct sigaction act;

	/* Reset signal handler - necessary for SysV, does no harm for BSD */

/*  	instr = *((int*)(sigctx->eip)); */
/*    	faultaddr = sigctx->sc_regs[(instr >> 16) & 0x1f]; */

/*  	fprintf(stderr, "null=%d %p addr=%p\n", sig, sigctx, sigctx->eip);*/

/*  	if (faultaddr == 0) { */
/*  		signal(sig, (void *) catch_NullPointerException); */
	act.sa_sigaction = (functionptr) catch_NullPointerException;
	act.sa_flags = SA_SIGINFO;
	sigaction(sig, &act, NULL);                          /* reinstall handler */

	sigemptyset(&nsig);
	sigaddset(&nsig, sig);
	sigprocmask(SIG_UNBLOCK, &nsig, NULL);           /* unblock signal    */

#ifdef __FreeBSD__
	sigctx->mc_ecx = sigctx->mc_eip;     /* REG_ITMP2_XPC*/
	sigctx->mc_eax = (u4) string_java_lang_NullPointerException;
	sigctx->mc_eip = (u4) asm_throw_and_handle_exception;
#else
	sigctx->ecx = sigctx->eip;             /* REG_ITMP2_XPC     */
	sigctx->eax = (u4) string_java_lang_NullPointerException;
	sigctx->eip = (u4) asm_throw_and_handle_exception;
#endif
	return;

/*  	} else { */
/*  		faultaddr += (long) ((instr << 16) >> 16); */
/*  		fprintf(stderr, "faulting address: 0x%08x\n", faultaddr); */
/*  		panic("Stack overflow"); */
/*  	} */
}


/* ArithmeticException signal handler for hardware divide by zero check       */

void catch_ArithmeticException(int sig, siginfo_t *siginfo, void *_p)
{
	sigset_t nsig;

/*  	void **_p = (void **) &sig; */
/*  	struct sigcontext *sigctx = (struct sigcontext *) ++_p; */

#ifdef __FreeBSD__
	ucontext_t *_uc = (ucontext_t *) _p;
	mcontext_t *sigctx = (mcontext_t *) &_uc->uc_mcontext;
#else
	struct ucontext *_uc = (struct ucontext *) _p;
	struct sigcontext *sigctx = (struct sigcontext *) &_uc->uc_mcontext;
#endif

	struct sigaction act;

	/* Reset signal handler - necessary for SysV, does no harm for BSD        */

/*  	signal(sig, (void *) catch_ArithmeticException); */
	act.sa_sigaction = (functionptr) catch_ArithmeticException;
	act.sa_flags = SA_SIGINFO;
	sigaction(sig, &act, NULL);                          /* reinstall handler */

	sigemptyset(&nsig);
	sigaddset(&nsig, sig);
	sigprocmask(SIG_UNBLOCK, &nsig, NULL);               /* unblock signal    */

#ifdef __FreeBSD__
	sigctx->mc_ecx = sigctx->mc_eip;                 /* REG_ITMP2_XPC     */
	sigctx->mc_eip = (u4) asm_throw_and_handle_hardware_arithmetic_exception;
#else
	sigctx->ecx = sigctx->eip;                     /* REG_ITMP2_XPC     */
	sigctx->eip = (u4) asm_throw_and_handle_hardware_arithmetic_exception;
#endif
	return;
}


void init_exceptions(void)
{
	struct sigaction act;

	/* install signal handlers we need to convert to exceptions */
	sigemptyset(&act.sa_mask);

	if (!checknull) {
#if defined(SIGSEGV)
/*  		signal(SIGSEGV, (void *) catch_NullPointerException); */
		act.sa_sigaction = (functionptr) catch_NullPointerException;
		act.sa_flags = SA_SIGINFO;
		sigaction(SIGSEGV, &act, NULL);
#endif

#if defined(SIGBUS)
/*  		signal(SIGBUS, (void *) catch_NullPointerException); */
		act.sa_sigaction = (functionptr) catch_NullPointerException;
		act.sa_flags = SA_SIGINFO;
		sigaction(SIGBUS, &act, NULL);
#endif
	}

/*  	signal(SIGFPE, (void *) catch_ArithmeticException); */
	act.sa_sigaction = (functionptr) catch_ArithmeticException;
	act.sa_flags = SA_SIGINFO;
	sigaction(SIGFPE, &act, NULL);
}


/* function codegen ************************************************************

	generates machine code

*******************************************************************************/

void codegen(methodinfo *m, codegendata *cd, registerdata *rd)
{
	s4 len, s1, s2, s3, d;
	s4 a;
	stackptr      src;
	varinfo      *var;
	basicblock   *bptr;
	instruction  *iptr;
	s4 parentargs_base;
	u2 currentline;
	s4 fpu_st_offset = 0;

	exceptiontable *ex;

	{
	s4 i, p, pa, t, l;
  	s4 savedregs_num = 0;

	/* space to save used callee saved registers */

	savedregs_num += (rd->savintregcnt - rd->maxsavintreguse);
	savedregs_num += (rd->savfltregcnt - rd->maxsavfltreguse);

	parentargs_base = rd->maxmemuse + savedregs_num;

	   
#if defined(USE_THREADS)           /* space to save argument of monitor_enter */

	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		parentargs_base++;

#endif

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
		(void) dseg_adds4(cd, (rd->maxmemuse + 1) * 8);         /* IsSync     */
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
		(void) dseg_addaddress(cd, ex->catchtype);
	}

	
	/* initialize mcode variables */
	
	cd->mcodeptr = cd->mcodebase;
	cd->mcodeend = (s4 *) (cd->mcodebase + cd->mcodesize);
	MCODECHECK(128 + m->paramcount);

	/* create stack frame (if necessary) */

	if (parentargs_base) {
		i386_alu_imm_reg(cd, I386_SUB, parentargs_base * 8, REG_SP);
	}

	/* save return address and used callee saved registers */

  	p = parentargs_base;
	for (i = rd->savintregcnt - 1; i >= rd->maxsavintreguse; i--) {
 		p--; i386_mov_reg_membase(cd, rd->savintregs[i], REG_SP, p * 8);
	}
	for (i = rd->savfltregcnt - 1; i >= rd->maxsavfltreguse; i--) {
		p--; i386_fld_reg(cd, rd->savfltregs[i]); i386_fstpl_membase(cd, REG_SP, p * 8);
	}

	/* save monitorenter argument */

#if defined(USE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		s4 func_enter = (m->flags & ACC_STATIC) ?
			(s4) builtin_staticmonitorenter : (s4) builtin_monitorenter;

		if (m->flags & ACC_STATIC) {
			i386_mov_imm_reg(cd, (s4) m->class, REG_ITMP1);
			i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, rd->maxmemuse * 8);

		} else {
			i386_mov_membase_reg(cd, REG_SP, parentargs_base * 8 + 4, REG_ITMP1);
			i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, rd->maxmemuse * 8);
		}

		/* call monitorenter function */

		i386_alu_imm_reg(cd, I386_SUB, 4, REG_SP);
		i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, 0);
		i386_mov_imm_reg(cd, func_enter, REG_ITMP1);
		i386_call_reg(cd, REG_ITMP1);
		i386_alu_imm_reg(cd, I386_ADD, 4, REG_SP);
	}			
#endif

	/* copy argument registers to stack and call trace function with pointer
	   to arguments on stack.
	*/

	if (runverbose) {
		i386_alu_imm_reg(cd, I386_SUB, TRACE_ARGS_NUM * 8 + 4, REG_SP);

		for (p = 0; p < m->paramcount && p < TRACE_ARGS_NUM; p++) {
			t = m->paramtypes[p];

			if (IS_INT_LNG_TYPE(t)) {
				if (IS_2_WORD_TYPE(t)) {
					i386_mov_membase_reg(cd, REG_SP, 4 + (parentargs_base + TRACE_ARGS_NUM + p) * 8 + 4, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, p * 8);
					i386_mov_membase_reg(cd, REG_SP, 4 + (parentargs_base + TRACE_ARGS_NUM + p) * 8 + 4 + 4, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, p * 8 + 4);

/*  				} else if (t == TYPE_ADR) { */
				} else {
					i386_mov_membase_reg(cd, REG_SP, 4 + (parentargs_base + TRACE_ARGS_NUM + p) * 8 + 4, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, p * 8);
					i386_alu_reg_reg(cd, I386_XOR, REG_ITMP1, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, p * 8 + 4);

/*  				} else { */
/*  					i386_mov_membase_reg(cd, REG_SP, 4 + (parentargs_base + TRACE_ARGS_NUM + p) * 8 + 4, EAX); */
/*  					i386_cltd(cd); */
/*  					i386_mov_reg_membase(cd, EAX, REG_SP, p * 8); */
/*  					i386_mov_reg_membase(cd, EDX, REG_SP, p * 8 + 4); */
				}

			} else {
				if (!IS_2_WORD_TYPE(t)) {
					i386_flds_membase(cd, REG_SP, 4 + (parentargs_base + TRACE_ARGS_NUM + p) * 8 + 4);
					i386_fstps_membase(cd, REG_SP, p * 8);
					i386_alu_reg_reg(cd, I386_XOR, REG_ITMP1, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, p * 8 + 4);

				} else {
					i386_fldl_membase(cd, REG_SP, 4 + (parentargs_base + TRACE_ARGS_NUM + p) * 8 + 4);
					i386_fstpl_membase(cd, REG_SP, p * 8);
				}
			}
		}

		/* fill up the remaining arguments */
		i386_alu_reg_reg(cd, I386_XOR, REG_ITMP1, REG_ITMP1);
		for (p = m->paramcount; p < TRACE_ARGS_NUM; p++) {
			i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, p * 8);
			i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, p * 8 + 4);
		}

		i386_mov_imm_membase(cd, (s4) m, REG_SP, TRACE_ARGS_NUM * 8);
  		i386_mov_imm_reg(cd, (s4) builtin_trace_args, REG_ITMP1);
		i386_call_reg(cd, REG_ITMP1);

		i386_alu_imm_reg(cd, I386_ADD, TRACE_ARGS_NUM * 8 + 4, REG_SP);
	}

	/* take arguments out of register or stack frame */

 	for (p = 0, l = 0; p < m->paramcount; p++) {
 		t = m->paramtypes[p];
 		var = &(rd->locals[l][t]);
 		l++;
 		if (IS_2_WORD_TYPE(t))    /* increment local counter for 2 word types */
 			l++;
 		if (var->type < 0)
 			continue;
		if (IS_INT_LNG_TYPE(t)) {                    /* integer args          */
 			if (p < rd->intreg_argnum) {              /* register arguments    */
				panic("integer register argument");
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
/*   					M_INTMOVE (argintregs[p], r); */

				} else {                             /* reg arg -> spilled    */
/*   					M_LST (argintregs[p], REG_SP, 8 * r); */
 				}
			} else {                                 /* stack arguments       */
 				pa = p - rd->intreg_argnum;
 				if (!(var->flags & INMEMORY)) {      /* stack arg -> register */ 
 					i386_mov_membase_reg(cd, REG_SP, (parentargs_base + pa) * 8 + 4, var->regoff);            /* + 4 for return address */
				} else {                             /* stack arg -> spilled  */
					if (!IS_2_WORD_TYPE(t)) {
						i386_mov_membase_reg(cd, REG_SP, (parentargs_base + pa) * 8 + 4, REG_ITMP1);    /* + 4 for return address */
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, var->regoff * 8);

					} else {
						i386_mov_membase_reg(cd, REG_SP, (parentargs_base + pa) * 8 + 4, REG_ITMP1);    /* + 4 for return address */
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, var->regoff * 8);
						i386_mov_membase_reg(cd, REG_SP, (parentargs_base + pa) * 8 + 4 + 4, REG_ITMP1);    /* + 4 for return address */
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, var->regoff * 8 + 4);
					}
				}
			}
		
		} else {                                     /* floating args         */   
 			if (p < rd->fltreg_argnum) {              /* register arguments    */
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
					panic("There are no float argument registers!");

 				} else {			                 /* reg arg -> spilled    */
					panic("There are no float argument registers!");
 				}

 			} else {                                 /* stack arguments       */
 				pa = p - rd->fltreg_argnum;
 				if (!(var->flags & INMEMORY)) {      /* stack-arg -> register */
					if (t == TYPE_FLT) {
						i386_flds_membase(cd, REG_SP, (parentargs_base + pa) * 8 + 4);
						fpu_st_offset++;
						i386_fstp_reg(cd, var->regoff + fpu_st_offset);
						fpu_st_offset--;

					} else {
						i386_fldl_membase(cd, REG_SP, (parentargs_base + pa) * 8 + 4);
						fpu_st_offset++;
						i386_fstp_reg(cd, var->regoff + fpu_st_offset);
						fpu_st_offset--;
					}

 				} else {                              /* stack-arg -> spilled  */
/*   					i386_mov_membase_reg(cd, REG_SP, (parentargs_base + pa) * 8 + 4, REG_ITMP1); */
/*   					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, r * 8); */
					if (t == TYPE_FLT) {
						i386_flds_membase(cd, REG_SP, (parentargs_base + pa) * 8 + 4);
						i386_fstps_membase(cd, REG_SP, var->regoff * 8);

					} else {
						i386_fldl_membase(cd, REG_SP, (parentargs_base + pa) * 8 + 4);
						i386_fstpl_membase(cd, REG_SP, var->regoff * 8);
					}
				}
			}
		}
	}  /* end for */

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
								d= src->regoff;
							else
								d=REG_ITMP1;
						i386_pop_reg(cd, d);
						store_reg_to_var_int(src, d);
						} else if (bptr->type == BBTYPE_EXH) {
							/* 							d = reg_of_var(m, src, REG_ITMP1); */
							if (!(src->flags & INMEMORY))
								d= src->regoff;
							else
								d=REG_ITMP1;
							M_INTMOVE(REG_ITMP1, d);
							store_reg_to_var_int(src, d);
						}

					} else {
						panic("copy interface registers(EXH, SBR): longs have to me in memory (begin 1)");
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
					panic("copy interface registers: longs have to me in memory (begin 1)");
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
								i386_flds_membase(cd, REG_SP, s1 * 8);

							} else {
								i386_fldl_membase(cd, REG_SP, s1 * 8);
							}
						}
						store_reg_to_var_flt(src, d);

					} else {
						s1 = rd->interfaces[len][s2].regoff;
						if (!IS_2_WORD_TYPE(rd->interfaces[len][s2].type)) {
							if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
								M_INTMOVE(s1, d);

							} else {
								i386_mov_membase_reg(cd, REG_SP, s1 * 8, d);
							}
							store_reg_to_var_int(src, d);

						} else {
							if (rd->interfaces[len][s2].flags & INMEMORY) {
  								M_LNGMEMMOVE(s1, src->regoff);

							} else {
								panic("copy interface registers: longs have to be in memory (begin 2)");
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

			MCODECHECK(64);   /* an instruction usually needs < 64 words      */

		switch (iptr->opc) {
		case ICMD_NOP:        /* ...  ==> ...                                 */
			break;

		case ICMD_NULLCHECKPOP: /* ..., objectref  ==> ...                    */
			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, I386_CMP, 0, REG_SP, src->regoff * 8);

			} else {
				i386_test_reg_reg(cd, src->regoff, src->regoff);
			}
			i386_jcc(cd, I386_CC_E, 0);
			codegen_addxnullrefs(cd, cd->mcodeptr);
			break;

		/* constant operations ************************************************/

		case ICMD_ICONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.i = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				i386_mov_imm_membase(cd, iptr->val.i, REG_SP, iptr->dst->regoff * 8);

			} else {
				if (iptr->val.i == 0) {
					i386_alu_reg_reg(cd, I386_XOR, d, d);

				} else {
					i386_mov_imm_reg(cd, iptr->val.i, d);
				}
			}
			break;

		case ICMD_LCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.l = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				i386_mov_imm_membase(cd, iptr->val.l, REG_SP, iptr->dst->regoff * 8);
				i386_mov_imm_membase(cd, iptr->val.l >> 32, REG_SP, iptr->dst->regoff * 8 + 4);
				
			} else {
				panic("LCONST: longs have to be in memory");
			}
			break;

		case ICMD_FCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.f = constant                    */

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

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				i386_mov_imm_membase(cd, (s4) iptr->val.a, REG_SP, iptr->dst->regoff * 8);

			} else {
				if ((s4) iptr->val.a == 0) {
					i386_alu_reg_reg(cd, I386_XOR, d, d);

				} else {
					i386_mov_imm_reg(cd, (s4) iptr->val.a, d);
				}
			}
			break;


		/* load/store operations **********************************************/

		case ICMD_ILOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_ALOAD:      /* op1 = local variable                         */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1)) {
				break;
			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (iptr->dst->flags & INMEMORY) {
				if (var->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, var->regoff * 8, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else {
					i386_mov_reg_membase(cd, var->regoff, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (var->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, var->regoff * 8, iptr->dst->regoff);

				} else {
					M_INTMOVE(var->regoff, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_LLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

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
					panic("LLOAD: longs have to be in memory");
				}

			} else {
				panic("LLOAD: longs have to be in memory");
			}
			break;

		case ICMD_FLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
  			if ((iptr->dst->varkind == LOCALVAR) &&
  			    (iptr->dst->varnum == iptr->op1)) {
    				break;
  			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY) {
				i386_flds_membase(cd, REG_SP, var->regoff * 8);
				fpu_st_offset++;
			} else {
				i386_fld_reg(cd, var->regoff + fpu_st_offset);
				fpu_st_offset++;
			}
  			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
  			if ((iptr->dst->varkind == LOCALVAR) &&
  			    (iptr->dst->varnum == iptr->op1)) {
    				break;
  			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY) {
				i386_fldl_membase(cd, REG_SP, var->regoff * 8);
				fpu_st_offset++;
			} else {
				i386_fld_reg(cd, var->regoff + fpu_st_offset);
				fpu_st_offset++;
			}
  			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_ISTORE:     /* ..., value  ==> ...                          */
		case ICMD_ASTORE:     /* op1 = local variable                         */

			if ((src->varkind == LOCALVAR) &&
			    (src->varnum == iptr->op1)) {
				break;
			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, var->regoff * 8);
					
				} else {
					i386_mov_reg_membase(cd, src->regoff, REG_SP, var->regoff * 8);
				}

			} else {
				var_to_reg_int(s1, src, var->regoff);
				M_INTMOVE(s1, var->regoff);
			}
			break;

		case ICMD_LSTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			if ((src->varkind == LOCALVAR) &&
			    (src->varnum == iptr->op1)) {
				break;
			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					M_LNGMEMMOVE(src->regoff, var->regoff);

				} else {
					panic("LSTORE: longs have to be in memory");
				}

			} else {
				panic("LSTORE: longs have to be in memory");
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
				var_to_reg_flt(s1, src, REG_FTMP1);
				i386_fstps_membase(cd, REG_SP, var->regoff * 8);
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

			if ((src->varkind == LOCALVAR) &&
			    (src->varnum == iptr->op1)) {
				break;
			}
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				var_to_reg_flt(s1, src, REG_FTMP1);
				i386_fstpl_membase(cd, REG_SP, var->regoff * 8);
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
			break;

		case ICMD_DUP:        /* ..., a ==> ..., a, a                         */
			M_COPY(src, iptr->dst);
			break;

		case ICMD_DUP2:       /* ..., a, b ==> ..., a, b, a, b                */

			M_COPY(src,       iptr->dst);
			M_COPY(src->prev, iptr->dst->prev);
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

		case ICMD_DUP2_X1:    /* ..., a, b, c ==> ..., b, c, a, b, c          */

			M_COPY(src,             iptr->dst);
			M_COPY(src->prev,       iptr->dst->prev);
			M_COPY(src->prev->prev, iptr->dst->prev->prev);
			M_COPY(iptr->dst,       iptr->dst->prev->prev->prev);
			M_COPY(iptr->dst->prev, iptr->dst->prev->prev->prev);
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
						i386_neg_membase(cd, REG_SP, iptr->dst->regoff * 8);

					} else {
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
						i386_neg_reg(cd, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					i386_mov_reg_membase(cd, src->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_neg_membase(cd, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, iptr->dst->regoff);
					i386_neg_reg(cd, iptr->dst->regoff);

				} else {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					i386_neg_reg(cd, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_LNEG:       /* ..., value  ==> ..., - value                 */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_neg_membase(cd, REG_SP, iptr->dst->regoff * 8);
						i386_alu_imm_membase(cd, I386_ADC, 0, REG_SP, iptr->dst->regoff * 8 + 4);
						i386_neg_membase(cd, REG_SP, iptr->dst->regoff * 8 + 4);

					} else {
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
						i386_neg_reg(cd, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 8 + 4, REG_ITMP1);
						i386_alu_imm_reg(cd, I386_ADC, 0, REG_ITMP1);
						i386_neg_reg(cd, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);
					}
				}
			}
			break;

		case ICMD_I2L:        /* ..., value  ==> ..., value                   */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, EAX);
					i386_cltd(cd);
					i386_mov_reg_membase(cd, EAX, REG_SP, iptr->dst->regoff * 8);
					i386_mov_reg_membase(cd, EDX, REG_SP, iptr->dst->regoff * 8 + 4);

				} else {
					M_INTMOVE(src->regoff, EAX);
					i386_cltd(cd);
					i386_mov_reg_membase(cd, EAX, REG_SP, iptr->dst->regoff * 8);
					i386_mov_reg_membase(cd, EDX, REG_SP, iptr->dst->regoff * 8 + 4);
				}
			}
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
					i386_shift_imm_reg(cd, I386_SHL, 24, REG_ITMP1);
					i386_shift_imm_reg(cd, I386_SAR, 24, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else {
					i386_mov_reg_membase(cd, src->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_shift_imm_membase(cd, I386_SHL, 24, REG_SP, iptr->dst->regoff * 8);
					i386_shift_imm_membase(cd, I386_SAR, 24, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, iptr->dst->regoff);
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

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_imm_membase(cd, I386_AND, 0x0000ffff, REG_SP, iptr->dst->regoff * 8);

					} else {
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_imm_reg(cd, I386_AND, 0x0000ffff, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					i386_mov_reg_membase(cd, src->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_alu_imm_membase(cd, I386_AND, 0x0000ffff, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, iptr->dst->regoff);
					i386_alu_imm_reg(cd, I386_AND, 0x0000ffff, iptr->dst->regoff);

				} else {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					i386_alu_imm_reg(cd, I386_AND, 0x0000ffff, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_INT2SHORT:  /* ..., value  ==> ..., value                   */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
					i386_shift_imm_reg(cd, I386_SHL, 16, REG_ITMP1);
					i386_shift_imm_reg(cd, I386_SAR, 16, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else {
					i386_mov_reg_membase(cd, src->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_shift_imm_membase(cd, I386_SHL, 16, REG_SP, iptr->dst->regoff * 8);
					i386_shift_imm_membase(cd, I386_SAR, 16, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, iptr->dst->regoff);
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

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ialu(cd, I386_ADD, src, iptr);
			break;

		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ialuconst(cd, I386_ADD, src, iptr);
			break;

		case ICMD_LADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_alu_reg_membase(cd, I386_ADD, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8 + 4, REG_ITMP1);
						i386_alu_reg_membase(cd, I386_ADC, REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);

					} else if (src->prev->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_reg_membase(cd, I386_ADD, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 8 + 4, REG_ITMP1);
						i386_alu_reg_membase(cd, I386_ADC, REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);

					} else {
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_alu_membase_reg(cd, I386_ADD, REG_SP, src->regoff * 8, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8 + 4, REG_ITMP1);
						i386_alu_membase_reg(cd, I386_ADC, REG_SP, src->regoff * 8 + 4, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);
					}

				}
			}
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_imm_membase(cd, I386_ADD, iptr->val.l, REG_SP, iptr->dst->regoff * 8);
						i386_alu_imm_membase(cd, I386_ADC, iptr->val.l >> 32, REG_SP, iptr->dst->regoff * 8 + 4);

					} else {
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_imm_reg(cd, I386_ADD, iptr->val.l, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 8 + 4, REG_ITMP1);
						i386_alu_imm_reg(cd, I386_ADC, iptr->val.l >> 32, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);
					}
				}
			}
			break;

		case ICMD_ISUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_reg_membase(cd, I386_SUB, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

					} else {
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_alu_membase_reg(cd, I386_SUB, REG_SP, src->regoff * 8, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, REG_ITMP1);
					i386_alu_membase_reg(cd, I386_SUB, REG_SP, src->regoff * 8, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						i386_alu_reg_membase(cd, I386_SUB, src->regoff, REG_SP, iptr->dst->regoff * 8);

					} else {
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_alu_reg_reg(cd, I386_SUB, src->regoff, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					i386_mov_reg_membase(cd, src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_alu_reg_membase(cd, I386_SUB, src->regoff, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, d);
					i386_alu_membase_reg(cd, I386_SUB, REG_SP, src->regoff * 8, d);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, d);
					i386_alu_membase_reg(cd, I386_SUB, REG_SP, src->regoff * 8, d);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					/* workaround for reg alloc */
					if (src->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_alu_reg_reg(cd, I386_SUB, src->regoff, REG_ITMP1);
						M_INTMOVE(REG_ITMP1, d);

					} else {
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, d);
						i386_alu_reg_reg(cd, I386_SUB, src->regoff, d);
					}

				} else {
					/* workaround for reg alloc */
					if (src->regoff == iptr->dst->regoff) {
						M_INTMOVE(src->prev->regoff, REG_ITMP1);
						i386_alu_reg_reg(cd, I386_SUB, src->regoff, REG_ITMP1);
						M_INTMOVE(REG_ITMP1, d);

					} else {
						M_INTMOVE(src->prev->regoff, d);
						i386_alu_reg_reg(cd, I386_SUB, src->regoff, d);
					}
				}
			}
			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ialuconst(cd, I386_SUB, src, iptr);
			break;

		case ICMD_LSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_reg_membase(cd, I386_SUB, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 8 + 4, REG_ITMP1);
						i386_alu_reg_membase(cd, I386_SBB, REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);

					} else {
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_alu_membase_reg(cd, I386_SUB, REG_SP, src->regoff * 8, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8 + 4, REG_ITMP1);
						i386_alu_membase_reg(cd, I386_SBB, REG_SP, src->regoff * 8 + 4, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);
					}
				}
			}
			break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_imm_membase(cd, I386_SUB, iptr->val.l, REG_SP, iptr->dst->regoff * 8);
						i386_alu_imm_membase(cd, I386_SBB, iptr->val.l >> 32, REG_SP, iptr->dst->regoff * 8 + 4);

					} else {
						/* TODO: could be size optimized with lea -- see gcc output */
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_imm_reg(cd, I386_SUB, iptr->val.l, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_membase_reg(cd, REG_SP, src->regoff * 8 + 4, REG_ITMP1);
						i386_alu_imm_reg(cd, I386_SBB, iptr->val.l >> 32, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);
					}
				}
			}
			break;

		case ICMD_IMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
					i386_imul_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
					i386_imul_reg_reg(cd, src->prev->regoff, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
					i386_imul_reg_reg(cd, src->regoff, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else {
					i386_mov_reg_reg(cd, src->prev->regoff, REG_ITMP1);
					i386_imul_reg_reg(cd, src->regoff, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
					i386_imul_membase_reg(cd, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
					i386_imul_membase_reg(cd, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					i386_imul_membase_reg(cd, REG_SP, src->prev->regoff * 8, iptr->dst->regoff);

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

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_imul_imm_membase_reg(cd, iptr->val.i, REG_SP, src->regoff * 8, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else {
					i386_imul_imm_reg_reg(cd, iptr->val.i, src->regoff, REG_ITMP1);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_imul_imm_membase_reg(cd, iptr->val.i, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else {
					i386_imul_imm_reg_reg(cd, iptr->val.i, src->regoff, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_LMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, EAX);             /* mem -> EAX             */
					/* optimize move EAX -> REG_ITMP3 is slower??? */
/*    					i386_mov_reg_reg(cd, EAX, REG_ITMP3); */
					i386_mul_membase(cd, REG_SP, src->regoff * 8);                            /* mem * EAX -> EDX:EAX   */

					/* TODO: optimize move EAX -> REG_ITMP3 */
  					i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2);   /* mem -> ITMP3           */
					i386_imul_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP2);            /* mem * ITMP3 -> ITMP3   */
					i386_alu_reg_reg(cd, I386_ADD, REG_ITMP2, EDX);                      /* ITMP3 + EDX -> EDX     */

					i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP2);       /* mem -> ITMP3           */
					i386_imul_membase_reg(cd, REG_SP, src->regoff * 8 + 4, REG_ITMP2);        /* mem * ITMP3 -> ITMP3   */

					i386_alu_reg_reg(cd, I386_ADD, REG_ITMP2, EDX);                      /* ITMP3 + EDX -> EDX     */
					i386_mov_reg_membase(cd, EAX, REG_SP, iptr->dst->regoff * 8);
					i386_mov_reg_membase(cd, EDX, REG_SP, iptr->dst->regoff * 8 + 4);
				}
			}
			break;

		case ICMD_LMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_mov_imm_reg(cd, iptr->val.l, EAX);                                   /* imm -> EAX             */
					i386_mul_membase(cd, REG_SP, src->regoff * 8);                            /* mem * EAX -> EDX:EAX   */
					/* TODO: optimize move EAX -> REG_ITMP3 */
					i386_mov_imm_reg(cd, iptr->val.l >> 32, REG_ITMP2);                       /* imm -> ITMP3           */
					i386_imul_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP2);            /* mem * ITMP3 -> ITMP3   */

					i386_alu_reg_reg(cd, I386_ADD, REG_ITMP2, EDX);                      /* ITMP3 + EDX -> EDX     */
					i386_mov_imm_reg(cd, iptr->val.l, REG_ITMP2);                             /* imm -> ITMP3           */
					i386_imul_membase_reg(cd, REG_SP, src->regoff * 8 + 4, REG_ITMP2);        /* mem * ITMP3 -> ITMP3   */

					i386_alu_reg_reg(cd, I386_ADD, REG_ITMP2, EDX);                      /* ITMP3 + EDX -> EDX     */
					i386_mov_reg_membase(cd, EAX, REG_SP, iptr->dst->regoff * 8);
					i386_mov_reg_membase(cd, EDX, REG_SP, iptr->dst->regoff * 8 + 4);
				}
			}
			break;

		case ICMD_IDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			var_to_reg_int(s1, src, REG_ITMP2);
  			gen_div_check(src);
	        if (src->prev->flags & INMEMORY) {
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, EAX);

			} else {
				M_INTMOVE(src->prev->regoff, EAX);
			}
			
			i386_alu_imm_reg(cd, I386_CMP, 0x80000000, EAX);    /* check as described in jvm spec */
			i386_jcc(cd, I386_CC_NE, 3 + 6);
			i386_alu_imm_reg(cd, I386_CMP, -1, s1);
			i386_jcc(cd, I386_CC_E, 1 + 2);

  			i386_cltd(cd);
			i386_idiv_reg(cd, s1);

			if (iptr->dst->flags & INMEMORY) {
				i386_mov_reg_membase(cd, EAX, REG_SP, iptr->dst->regoff * 8);

			} else {
				M_INTMOVE(EAX, iptr->dst->regoff);
			}
			break;

		case ICMD_IREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			var_to_reg_int(s1, src, REG_ITMP2);
  			gen_div_check(src);
			if (src->prev->flags & INMEMORY) {
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, EAX);

			} else {
				M_INTMOVE(src->prev->regoff, EAX);
			}
			
			i386_alu_imm_reg(cd, I386_CMP, 0x80000000, EAX);    /* check as described in jvm spec */
			i386_jcc(cd, I386_CC_NE, 2 + 3 + 6);
			i386_alu_reg_reg(cd, I386_XOR, EDX, EDX);
			i386_alu_imm_reg(cd, I386_CMP, -1, s1);
			i386_jcc(cd, I386_CC_E, 1 + 2);

  			i386_cltd(cd);
			i386_idiv_reg(cd, s1);

			if (iptr->dst->flags & INMEMORY) {
				i386_mov_reg_membase(cd, EDX, REG_SP, iptr->dst->regoff * 8);

			} else {
				M_INTMOVE(EDX, iptr->dst->regoff);
			}
			break;

		case ICMD_IDIVPOW2:   /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			/* TODO: optimize for `/ 2' */
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);

			M_INTMOVE(s1, d);
			i386_test_reg_reg(cd, d, d);
			a = 2;
			CALCIMMEDIATEBYTES(a, (1 << iptr->val.i) - 1);
			i386_jcc(cd, I386_CC_NS, a);
			i386_alu_imm_reg(cd, I386_ADD, (1 << iptr->val.i) - 1, d);
				
			i386_shift_imm_reg(cd, I386_SAR, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LDIVPOW2:   /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					a = 2;
					CALCIMMEDIATEBYTES(a, (1 << iptr->val.i) - 1);
					a += 3;
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8 + 4, REG_ITMP2);

					i386_test_reg_reg(cd, REG_ITMP2, REG_ITMP2);
					i386_jcc(cd, I386_CC_NS, a);
					i386_alu_imm_reg(cd, I386_ADD, (1 << iptr->val.i) - 1, REG_ITMP1);
					i386_alu_imm_reg(cd, I386_ADC, 0, REG_ITMP2);
					i386_shrd_imm_reg_reg(cd, iptr->val.i, REG_ITMP2, REG_ITMP1);
					i386_shift_imm_reg(cd, I386_SAR, iptr->val.i, REG_ITMP2);

					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
				}
			}
			break;

		case ICMD_IREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.i = constant                             */

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
			i386_alu_imm_reg(cd, I386_AND, iptr->val.i, d);
			i386_test_reg_reg(cd, s1, s1);
			i386_jcc(cd, I386_CC_GE, a);
 			i386_mov_reg_reg(cd, s1, d);
  			i386_neg_reg(cd, d);
  			i386_alu_imm_reg(cd, I386_AND, iptr->val.i, d);
			i386_neg_reg(cd, d);

/*  			M_INTMOVE(s1, EAX); */
/*  			i386_cltd(cd); */
/*  			i386_alu_reg_reg(cd, I386_XOR, EDX, EAX); */
/*  			i386_alu_reg_reg(cd, I386_SUB, EDX, EAX); */
/*  			i386_alu_reg_reg(cd, I386_AND, iptr->val.i, EAX); */
/*  			i386_alu_reg_reg(cd, I386_XOR, EDX, EAX); */
/*  			i386_alu_reg_reg(cd, I386_SUB, EDX, EAX); */
/*  			M_INTMOVE(EAX, d); */

/*  			i386_alu_reg_reg(cd, I386_XOR, d, d); */
/*  			i386_mov_imm_reg(cd, iptr->val.i, ECX); */
/*  			i386_shrd_reg_reg(cd, s1, d); */
/*  			i386_shift_imm_reg(cd, I386_SHR, 32 - iptr->val.i, d); */

			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					/* Intel algorithm -- does not work, because constant is wrong */
/*  					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1); */
/*  					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8 + 4, REG_ITMP3); */

/*  					M_INTMOVE(REG_ITMP1, REG_ITMP2); */
/*  					i386_test_reg_reg(cd, REG_ITMP3, REG_ITMP3); */
/*  					i386_jcc(cd, I386_CC_NS, offset); */
/*  					i386_alu_imm_reg(cd, I386_ADD, (1 << iptr->val.l) - 1, REG_ITMP2); */
/*  					i386_alu_imm_reg(cd, I386_ADC, 0, REG_ITMP3); */
					
/*  					i386_shrd_imm_reg_reg(cd, iptr->val.l, REG_ITMP3, REG_ITMP2); */
/*  					i386_shift_imm_reg(cd, I386_SAR, iptr->val.l, REG_ITMP3); */
/*  					i386_shld_imm_reg_reg(cd, iptr->val.l, REG_ITMP2, REG_ITMP3); */

/*  					i386_shift_imm_reg(cd, I386_SHL, iptr->val.l, REG_ITMP2); */

/*  					i386_alu_reg_reg(cd, I386_SUB, REG_ITMP2, REG_ITMP1); */
/*  					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8 + 4, REG_ITMP2); */
/*  					i386_alu_reg_reg(cd, I386_SBB, REG_ITMP3, REG_ITMP2); */

/*  					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8); */
/*  					i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4); */

					/* Alpha algorithm */
					a = 3;
					CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
					a += 3;
					CALCOFFSETBYTES(a, REG_SP, src->regoff * 8 + 4);

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

					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8 + 4, REG_ITMP2);
					
					i386_alu_imm_reg(cd, I386_AND, iptr->val.l, REG_ITMP1);
					i386_alu_imm_reg(cd, I386_AND, iptr->val.l >> 32, REG_ITMP2);
					i386_alu_imm_membase(cd, I386_CMP, 0, REG_SP, src->regoff * 8 + 4);
					i386_jcc(cd, I386_CC_GE, a);

					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8 + 4, REG_ITMP2);
					
					i386_neg_reg(cd, REG_ITMP1);
					i386_alu_imm_reg(cd, I386_ADC, 0, REG_ITMP2);
					i386_neg_reg(cd, REG_ITMP2);
					
					i386_alu_imm_reg(cd, I386_AND, iptr->val.l, REG_ITMP1);
					i386_alu_imm_reg(cd, I386_AND, iptr->val.l >> 32, REG_ITMP2);
					
					i386_neg_reg(cd, REG_ITMP1);
					i386_alu_imm_reg(cd, I386_ADC, 0, REG_ITMP2);
					i386_neg_reg(cd, REG_ITMP2);

					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
				}
			}
			break;

		case ICMD_ISHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ishift(cd, I386_SHL, src, iptr);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ishiftconst(cd, I386_SHL, src, iptr);
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ishift(cd, I386_SAR, src, iptr);
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ishiftconst(cd, I386_SAR, src, iptr);
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ishift(cd, I386_SHR, src, iptr);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ishiftconst(cd, I386_SHR, src, iptr);
			break;

		case ICMD_LSHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY ){
				if (src->prev->flags & INMEMORY) {
/*  					if (src->prev->regoff == iptr->dst->regoff) { */
/*  						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1); */

/*  						if (src->flags & INMEMORY) { */
/*  							i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, ECX); */
/*  						} else { */
/*  							M_INTMOVE(src->regoff, ECX); */
/*  						} */

/*  						i386_test_imm_reg(cd, 32, ECX); */
/*  						i386_jcc(cd, I386_CC_E, 2 + 2); */
/*  						i386_mov_reg_reg(cd, REG_ITMP1, REG_ITMP2); */
/*  						i386_alu_reg_reg(cd, I386_XOR, REG_ITMP1, REG_ITMP1); */
						
/*  						i386_shld_reg_membase(cd, REG_ITMP1, REG_SP, src->prev->regoff * 8 + 4); */
/*  						i386_shift_membase(cd, I386_SHL, REG_SP, iptr->dst->regoff * 8); */

/*  					} else { */
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8 + 4, REG_ITMP3);
						
						if (src->flags & INMEMORY) {
							i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, ECX);
						} else {
							M_INTMOVE(src->regoff, ECX);
						}
						
						i386_test_imm_reg(cd, 32, ECX);
						i386_jcc(cd, I386_CC_E, 2 + 2);
						i386_mov_reg_reg(cd, REG_ITMP1, REG_ITMP3);
						i386_alu_reg_reg(cd, I386_XOR, REG_ITMP1, REG_ITMP1);
						
						i386_shld_reg_reg(cd, REG_ITMP1, REG_ITMP3);
						i386_shift_reg(cd, I386_SHL, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_reg_membase(cd, REG_ITMP3, REG_SP, iptr->dst->regoff * 8 + 4);
/*  					} */
				}
			}
			break;

        case ICMD_LSHLCONST:  /* ..., value  ==> ..., value << constant       */
 			                  /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY ) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 8 + 4, REG_ITMP2);

				if (iptr->val.i & 0x20) {
					i386_mov_reg_reg(cd, REG_ITMP1, REG_ITMP2);
					i386_alu_reg_reg(cd, I386_XOR, REG_ITMP1, REG_ITMP1);
					i386_shld_imm_reg_reg(cd, iptr->val.i & 0x3f, REG_ITMP1, REG_ITMP2);

				} else {
					i386_shld_imm_reg_reg(cd, iptr->val.i & 0x3f, REG_ITMP1, REG_ITMP2);
					i386_shift_imm_reg(cd, I386_SHL, iptr->val.i & 0x3f, REG_ITMP1);
				}

				i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
			}
			break;

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY ){
				if (src->prev->flags & INMEMORY) {
/*  					if (src->prev->regoff == iptr->dst->regoff) { */
  						/* TODO: optimize */
/*  						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1); */
/*  						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2); */

/*  						if (src->flags & INMEMORY) { */
/*  							i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, ECX); */
/*  						} else { */
/*  							M_INTMOVE(src->regoff, ECX); */
/*  						} */

/*  						i386_test_imm_reg(cd, 32, ECX); */
/*  						i386_jcc(cd, I386_CC_E, 2 + 3); */
/*  						i386_mov_reg_reg(cd, REG_ITMP2, REG_ITMP1); */
/*  						i386_shift_imm_reg(cd, I386_SAR, 31, REG_ITMP2); */
						
/*  						i386_shrd_reg_reg(cd, REG_ITMP2, REG_ITMP1); */
/*  						i386_shift_reg(cd, I386_SAR, REG_ITMP2); */
/*  						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8); */
/*  						i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4); */

/*  					} else { */
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8 + 4, REG_ITMP3);

						if (src->flags & INMEMORY) {
							i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, ECX);
						} else {
							M_INTMOVE(src->regoff, ECX);
						}

						i386_test_imm_reg(cd, 32, ECX);
						i386_jcc(cd, I386_CC_E, 2 + 3);
						i386_mov_reg_reg(cd, REG_ITMP3, REG_ITMP1);
						i386_shift_imm_reg(cd, I386_SAR, 31, REG_ITMP3);
						
						i386_shrd_reg_reg(cd, REG_ITMP3, REG_ITMP1);
						i386_shift_reg(cd, I386_SAR, REG_ITMP3);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_reg_membase(cd, REG_ITMP3, REG_SP, iptr->dst->regoff * 8 + 4);
/*  					} */
				}
			}
			break;

		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY ) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 8 + 4, REG_ITMP2);

				if (iptr->val.i & 0x20) {
					i386_mov_reg_reg(cd, REG_ITMP2, REG_ITMP1);
					i386_shift_imm_reg(cd, I386_SAR, 31, REG_ITMP2);
					i386_shrd_imm_reg_reg(cd, iptr->val.i & 0x3f, REG_ITMP2, REG_ITMP1);

				} else {
					i386_shrd_imm_reg_reg(cd, iptr->val.i & 0x3f, REG_ITMP2, REG_ITMP1);
					i386_shift_imm_reg(cd, I386_SAR, iptr->val.i & 0x3f, REG_ITMP2);
				}

				i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
			}
			break;

		case ICMD_LUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY ){
				if (src->prev->flags & INMEMORY) {
/*  					if (src->prev->regoff == iptr->dst->regoff) { */
  						/* TODO: optimize */
/*  						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1); */
/*  						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2); */

/*  						if (src->flags & INMEMORY) { */
/*  							i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, ECX); */
/*  						} else { */
/*  							M_INTMOVE(src->regoff, ECX); */
/*  						} */

/*  						i386_test_imm_reg(cd, 32, ECX); */
/*  						i386_jcc(cd, I386_CC_E, 2 + 2); */
/*  						i386_mov_reg_reg(cd, REG_ITMP2, REG_ITMP1); */
/*  						i386_alu_reg_reg(cd, I386_XOR, REG_ITMP2, REG_ITMP2); */
						
/*  						i386_shrd_reg_reg(cd, REG_ITMP2, REG_ITMP1); */
/*  						i386_shift_reg(cd, I386_SHR, REG_ITMP2); */
/*  						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8); */
/*  						i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4); */

/*  					} else { */
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8 + 4, REG_ITMP3);

						if (src->flags & INMEMORY) {
							i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, ECX);
						} else {
							M_INTMOVE(src->regoff, ECX);
						}

						i386_test_imm_reg(cd, 32, ECX);
						i386_jcc(cd, I386_CC_E, 2 + 2);
						i386_mov_reg_reg(cd, REG_ITMP3, REG_ITMP1);
						i386_alu_reg_reg(cd, I386_XOR, REG_ITMP3, REG_ITMP3);
						
						i386_shrd_reg_reg(cd, REG_ITMP3, REG_ITMP1);
						i386_shift_reg(cd, I386_SHR, REG_ITMP3);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_reg_membase(cd, REG_ITMP3, REG_SP, iptr->dst->regoff * 8 + 4);
/*  					} */
				}
			}
			break;

  		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
  		                      /* val.l = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY ) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 8 + 4, REG_ITMP2);

				if (iptr->val.i & 0x20) {
					i386_mov_reg_reg(cd, REG_ITMP2, REG_ITMP1);
					i386_alu_reg_reg(cd, I386_XOR, REG_ITMP2, REG_ITMP2);
					i386_shrd_imm_reg_reg(cd, iptr->val.i & 0x3f, REG_ITMP2, REG_ITMP1);

				} else {
					i386_shrd_imm_reg_reg(cd, iptr->val.i & 0x3f, REG_ITMP2, REG_ITMP1);
					i386_shift_imm_reg(cd, I386_SHR, iptr->val.i & 0x3f, REG_ITMP2);
				}

				i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
			}
  			break;

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ialu(cd, I386_AND, src, iptr);
			break;

		case ICMD_IANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ialuconst(cd, I386_AND, src, iptr);
			break;

		case ICMD_LAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_lalu(cd, I386_AND, src, iptr);
			break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_laluconst(cd, I386_AND, src, iptr);
			break;

		case ICMD_IOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ialu(cd, I386_OR, src, iptr);
			break;

		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ialuconst(cd, I386_OR, src, iptr);
			break;

		case ICMD_LOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_lalu(cd, I386_OR, src, iptr);
			break;

		case ICMD_LORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_laluconst(cd, I386_OR, src, iptr);
			break;

		case ICMD_IXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ialu(cd, I386_XOR, src, iptr);
			break;

		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ialuconst(cd, I386_XOR, src, iptr);
			break;

		case ICMD_LXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_lalu(cd, I386_XOR, src, iptr);
			break;

		case ICMD_LXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_laluconst(cd, I386_XOR, src, iptr);
			break;

		case ICMD_IINC:       /* ..., value  ==> ..., value + constant        */
		                      /* op1 = variable, val.i = constant             */

			var = &(rd->locals[iptr->op1][TYPE_INT]);
			if (var->flags & INMEMORY) {
				i386_alu_imm_membase(cd, I386_ADD, iptr->val.i, REG_SP, var->regoff * 8);

			} else {
				/* `inc reg' is slower on p4's (regarding to ia32             */
				/* optimization reference manual and benchmarks) and as fast  */
				/* on athlon's.                                               */
				i386_alu_imm_reg(cd, I386_ADD, iptr->val.i, var->regoff);
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

			FPU_SET_24BIT_MODE;
			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			i386_fchs(cd);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */

			FPU_SET_53BIT_MODE;
			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			i386_fchs(cd);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			FPU_SET_24BIT_MODE;
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			i386_faddp(cd);
			fpu_st_offset--;
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			FPU_SET_53BIT_MODE;
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			i386_faddp(cd);
			fpu_st_offset--;
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			FPU_SET_24BIT_MODE;
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			i386_fsubp(cd);
			fpu_st_offset--;
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			FPU_SET_53BIT_MODE;
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			i386_fsubp(cd);
			fpu_st_offset--;
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

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

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			if (src->flags & INMEMORY) {
				i386_fildl_membase(cd, REG_SP, src->regoff * 8);
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

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			if (src->flags & INMEMORY) {
				i386_fildll_membase(cd, REG_SP, src->regoff * 8);
				fpu_st_offset++;

			} else {
				panic("L2F: longs have to be in memory");
			}
  			store_reg_to_var_flt(iptr->dst, d);
			break;
			
		case ICMD_F2I:       /* ..., value  ==> ..., (int) value              */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_NULL);

			a = dseg_adds4(cd, 0x0e7f);    /* Round to zero, 53-bit mode, exception masked */
			i386_mov_imm_reg(cd, 0, REG_ITMP1);
			dseg_adddata(cd, cd->mcodeptr);
			i386_fldcw_membase(cd, REG_ITMP1, a);

			if (iptr->dst->flags & INMEMORY) {
				i386_fistpl_membase(cd, REG_SP, iptr->dst->regoff * 8);
				fpu_st_offset--;

				a = dseg_adds4(cd, 0x027f);    /* Round to nearest, 53-bit mode, exceptions masked */
				i386_fldcw_membase(cd, REG_ITMP1, a);

				i386_alu_imm_membase(cd, I386_CMP, 0x80000000, REG_SP, iptr->dst->regoff * 8);

				a = 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				a += 5 + 2 + 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 8);

			} else {
				a = dseg_adds4(cd, 0);
				i386_fistpl_membase(cd, REG_ITMP1, a);
				fpu_st_offset--;
				i386_mov_membase_reg(cd, REG_ITMP1, a, iptr->dst->regoff);

				a = dseg_adds4(cd, 0x027f);    /* Round to nearest, 53-bit mode, exceptions masked */
				i386_fldcw_membase(cd, REG_ITMP1, a);

				i386_alu_imm_reg(cd, I386_CMP, 0x80000000, iptr->dst->regoff);

				a = 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				a += 5 + 2 + ((REG_RESULT == iptr->dst->regoff) ? 0 : 2);
			}

			i386_jcc(cd, I386_CC_NE, a);

			/* XXX: change this when we use registers */
			i386_flds_membase(cd, REG_SP, src->regoff * 8);
			i386_mov_imm_reg(cd, (s4) asm_builtin_f2i, REG_ITMP1);
			i386_call_reg(cd, REG_ITMP1);

			if (iptr->dst->flags & INMEMORY) {
				i386_mov_reg_membase(cd, REG_RESULT, REG_SP, iptr->dst->regoff * 8);

			} else {
				M_INTMOVE(REG_RESULT, iptr->dst->regoff);
			}
			break;

		case ICMD_D2I:       /* ..., value  ==> ..., (int) value              */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_NULL);

			a = dseg_adds4(cd, 0x0e7f);    /* Round to zero, 53-bit mode, exception masked */
			i386_mov_imm_reg(cd, 0, REG_ITMP1);
			dseg_adddata(cd, cd->mcodeptr);
			i386_fldcw_membase(cd, REG_ITMP1, a);

			if (iptr->dst->flags & INMEMORY) {
				i386_fistpl_membase(cd, REG_SP, iptr->dst->regoff * 8);
				fpu_st_offset--;

				a = dseg_adds4(cd, 0x027f);    /* Round to nearest, 53-bit mode, exceptions masked */
				i386_fldcw_membase(cd, REG_ITMP1, a);

  				i386_alu_imm_membase(cd, I386_CMP, 0x80000000, REG_SP, iptr->dst->regoff * 8);

				a = 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				a += 5 + 2 + 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 8);

			} else {
				a = dseg_adds4(cd, 0);
				i386_fistpl_membase(cd, REG_ITMP1, a);
				fpu_st_offset--;
				i386_mov_membase_reg(cd, REG_ITMP1, a, iptr->dst->regoff);

				a = dseg_adds4(cd, 0x027f);    /* Round to nearest, 53-bit mode, exceptions masked */
				i386_fldcw_membase(cd, REG_ITMP1, a);

				i386_alu_imm_reg(cd, I386_CMP, 0x80000000, iptr->dst->regoff);

				a = 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				a += 5 + 2 + ((REG_RESULT == iptr->dst->regoff) ? 0 : 2);
			}

			i386_jcc(cd, I386_CC_NE, a);

			/* XXX: change this when we use registers */
			i386_fldl_membase(cd, REG_SP, src->regoff * 8);
			i386_mov_imm_reg(cd, (s4) asm_builtin_d2i, REG_ITMP1);
			i386_call_reg(cd, REG_ITMP1);

			if (iptr->dst->flags & INMEMORY) {
				i386_mov_reg_membase(cd, REG_RESULT, REG_SP, iptr->dst->regoff * 8);
			} else {
				M_INTMOVE(REG_RESULT, iptr->dst->regoff);
			}
			break;

		case ICMD_F2L:       /* ..., value  ==> ..., (long) value             */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_NULL);

			a = dseg_adds4(cd, 0x0e7f);    /* Round to zero, 53-bit mode, exception masked */
			i386_mov_imm_reg(cd, 0, REG_ITMP1);
			dseg_adddata(cd, cd->mcodeptr);
			i386_fldcw_membase(cd, REG_ITMP1, a);

			if (iptr->dst->flags & INMEMORY) {
				i386_fistpll_membase(cd, REG_SP, iptr->dst->regoff * 8);
				fpu_st_offset--;

				a = dseg_adds4(cd, 0x027f);    /* Round to nearest, 53-bit mode, exceptions masked */
				i386_fldcw_membase(cd, REG_ITMP1, a);

  				i386_alu_imm_membase(cd, I386_CMP, 0x80000000, REG_SP, iptr->dst->regoff * 8 + 4);

				a = 6 + 4;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 8);
				a += 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				a += 5 + 2;
				a += 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 8);
				a += 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 8 + 4);

				i386_jcc(cd, I386_CC_NE, a);

  				i386_alu_imm_membase(cd, I386_CMP, 0, REG_SP, iptr->dst->regoff * 8);

				a = 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				a += 5 + 2 + 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 8);

				i386_jcc(cd, I386_CC_NE, a);

				/* XXX: change this when we use registers */
				i386_flds_membase(cd, REG_SP, src->regoff * 8);
				i386_mov_imm_reg(cd, (s4) asm_builtin_f2l, REG_ITMP1);
				i386_call_reg(cd, REG_ITMP1);
				i386_mov_reg_membase(cd, REG_RESULT, REG_SP, iptr->dst->regoff * 8);
				i386_mov_reg_membase(cd, REG_RESULT2, REG_SP, iptr->dst->regoff * 8 + 4);

			} else {
				panic("F2L: longs have to be in memory");
			}
			break;

		case ICMD_D2L:       /* ..., value  ==> ..., (long) value             */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_NULL);

			a = dseg_adds4(cd, 0x0e7f);    /* Round to zero, 53-bit mode, exception masked */
			i386_mov_imm_reg(cd, 0, REG_ITMP1);
			dseg_adddata(cd, cd->mcodeptr);
			i386_fldcw_membase(cd, REG_ITMP1, a);

			if (iptr->dst->flags & INMEMORY) {
				i386_fistpll_membase(cd, REG_SP, iptr->dst->regoff * 8);
				fpu_st_offset--;

				a = dseg_adds4(cd, 0x027f);    /* Round to nearest, 53-bit mode, exceptions masked */
				i386_fldcw_membase(cd, REG_ITMP1, a);

  				i386_alu_imm_membase(cd, I386_CMP, 0x80000000, REG_SP, iptr->dst->regoff * 8 + 4);

				a = 6 + 4;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 8);
				a += 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				a += 5 + 2;
				a += 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 8);
				a += 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 8 + 4);

				i386_jcc(cd, I386_CC_NE, a);

  				i386_alu_imm_membase(cd, I386_CMP, 0, REG_SP, iptr->dst->regoff * 8);

				a = 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				a += 5 + 2 + 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 8);

				i386_jcc(cd, I386_CC_NE, a);

				/* XXX: change this when we use registers */
				i386_fldl_membase(cd, REG_SP, src->regoff * 8);
				i386_mov_imm_reg(cd, (s4) asm_builtin_d2l, REG_ITMP1);
				i386_call_reg(cd, REG_ITMP1);
				i386_mov_reg_membase(cd, REG_RESULT, REG_SP, iptr->dst->regoff * 8);
				i386_mov_reg_membase(cd, REG_RESULT2, REG_SP, iptr->dst->regoff * 8 + 4);

			} else {
				panic("D2L: longs have to be in memory");
			}
			break;

		case ICMD_F2D:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			/* nothing to do */
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_D2F:       /* ..., value  ==> ..., (float) value            */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			/* nothing to do */
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */
		case ICMD_DCMPL:

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
			i386_alu_imm_reg(cd, I386_AND, 0x000000ff, EAX);
 			i386_sahf(cd);
			i386_mov_imm_reg(cd, 0, d);    /* does not affect flags */
  			i386_jcc(cd, I386_CC_E, 6 + 3 + 5 + 3);
  			i386_jcc(cd, I386_CC_B, 3 + 5);
			i386_alu_imm_reg(cd, I386_SUB, 1, d);
			i386_jmp_imm(cd, 3);
			i386_alu_imm_reg(cd, I386_ADD, 1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */
		case ICMD_DCMPG:

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
			i386_movb_imm_reg(cd, 1, I386_AH);
 			i386_sahf(cd);
			i386_mov_imm_reg(cd, 0, d);    /* does not affect flags */
  			i386_jcc(cd, I386_CC_E, 6 + 3 + 5 + 3);
  			i386_jcc(cd, I386_CC_B, 3 + 5);
			i386_alu_imm_reg(cd, I386_SUB, 1, d);
			i386_jmp_imm(cd, 3);
			i386_alu_imm_reg(cd, I386_ADD, 1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;


		/* memory operations **************************************************/

		case ICMD_ARRAYLENGTH: /* ..., arrayref  ==> ..., length              */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			gen_nullptr_check(s1);
			i386_mov_membase_reg(cd, s1, OFFSET(java_arrayheader, size), d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_AALOAD:     /* ..., arrayref, index  ==> ..., value         */

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

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			
			if (iptr->dst->flags & INMEMORY) {
				i386_mov_memindex_reg(cd, OFFSET(java_longarray, data[0]), s1, s2, 3, REG_ITMP3);
				i386_mov_reg_membase(cd, REG_ITMP3, REG_SP, iptr->dst->regoff * 8);
				i386_mov_memindex_reg(cd, OFFSET(java_longarray, data[0]) + 4, s1, s2, 3, REG_ITMP3);
				i386_mov_reg_membase(cd, REG_ITMP3, REG_SP, iptr->dst->regoff * 8 + 4);
			}
			break;

		case ICMD_IALOAD:     /* ..., arrayref, index  ==> ..., value         */

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

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}

			if (src->flags & INMEMORY) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP3);
				i386_mov_reg_memindex(cd, REG_ITMP3, OFFSET(java_longarray, data[0]), s1, s2, 3);
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 8 + 4, REG_ITMP3);
				i386_mov_reg_memindex(cd, REG_ITMP3, OFFSET(java_longarray, data[0]) + 4, s1, s2, 3);
			}
			break;

		case ICMD_IASTORE:    /* ..., arrayref, index, value  ==> ...         */

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

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			i386_mov_imm_memindex(cd, iptr->val.i, OFFSET(java_intarray, data[0]), s1, s2, 2);
			break;

		case ICMD_LASTORECONST: /* ..., arrayref, index  ==> ...              */

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

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			i386_mov_imm_memindex(cd, 0, OFFSET(java_objectarray, data[0]), s1, s2, 2);
			break;

		case ICMD_BASTORECONST: /* ..., arrayref, index  ==> ...              */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			i386_movb_imm_memindex(cd, iptr->val.i, OFFSET(java_bytearray, data[0]), s1, s2, 0);
			break;

		case ICMD_CASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			i386_movw_imm_memindex(cd, iptr->val.i, OFFSET(java_chararray, data[0]), s1, s2, 1);
			break;

		case ICMD_SASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			i386_movw_imm_memindex(cd, iptr->val.i, OFFSET(java_shortarray, data[0]), s1, s2, 1);
			break;


		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */
		                      /* op1 = type, val.a = field address            */

			/* If the static fields' class is not yet initialized, we do it   */
			/* now. The call code is generated later.                         */
  			if (!((fieldinfo *) iptr->val.a)->class->initialized) {
				codegen_addclinitref(cd, cd->mcodeptr, ((fieldinfo *) iptr->val.a)->class);

				/* This is just for debugging purposes. Is very difficult to  */
				/* read patched code. Here we patch the following 5 nop's     */
				/* so that the real code keeps untouched.                     */
				if (showdisassemble) {
					i386_nop(cd);
					i386_nop(cd);
					i386_nop(cd);
					i386_nop(cd);
					i386_nop(cd);
				}
  			}

			a = (u4) &(((fieldinfo *) iptr->val.a)->value);
			switch (iptr->op1) {
			case TYPE_INT:
			case TYPE_ADR:
				var_to_reg_int(s2, src, REG_ITMP1);
				i386_mov_reg_mem(cd, s2, a);
				break;
			case TYPE_LNG:
				if (src->flags & INMEMORY) {
					/* Using both REG_ITMP1 and REG_ITMP2 is faster than only */
					/* using REG_ITMP1 alternating.                           */
					s2 = src->regoff;
					i386_mov_membase_reg(cd, REG_SP, s2 * 8, REG_ITMP1);
					i386_mov_membase_reg(cd, REG_SP, s2 * 8 + 4, REG_ITMP2);
					i386_mov_reg_mem(cd, REG_ITMP1, a);
					i386_mov_reg_mem(cd, REG_ITMP2, a + 4);
				} else {
					panic("PUTSTATIC: longs have to be in memory");
				}
				break;
			case TYPE_FLT:
				var_to_reg_flt(s2, src, REG_FTMP1);
				i386_fstps_mem(cd, a);
				fpu_st_offset--;
				break;
			case TYPE_DBL:
				var_to_reg_flt(s2, src, REG_FTMP1);
				i386_fstpl_mem(cd, a);
				fpu_st_offset--;
				break;
			default:
				throw_cacao_exception_exit(string_java_lang_InternalError,
										   "Unknown PUTSTATIC operand type %d",
										   iptr->op1);
			}
			break;

		case ICMD_GETSTATIC:  /* ...  ==> ..., value                          */
		                      /* op1 = type, val.a = field address            */

			/* If the static fields' class is not yet initialized, we do it   */
			/* now. The call code is generated later.                         */
  			if (!((fieldinfo *) iptr->val.a)->class->initialized) {
				codegen_addclinitref(cd, cd->mcodeptr, ((fieldinfo *) iptr->val.a)->class);

				/* This is just for debugging purposes. Is very difficult to  */
				/* read patched code. Here we patch the following 5 nop's     */
				/* so that the real code keeps untouched.                     */
				if (showdisassemble) {
					i386_nop(cd);
					i386_nop(cd);
					i386_nop(cd);
					i386_nop(cd);
					i386_nop(cd);
				}
  			}

			a = (u4) &(((fieldinfo *) iptr->val.a)->value);
			switch (iptr->op1) {
			case TYPE_INT:
			case TYPE_ADR:
				d = reg_of_var(rd, iptr->dst, REG_ITMP1);
				i386_mov_mem_reg(cd, a, d);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_LNG:
				d = reg_of_var(rd, iptr->dst, REG_NULL);
				if (iptr->dst->flags & INMEMORY) {
					/* Using both REG_ITMP1 and REG_ITMP2 is faster than only */
					/* using REG_ITMP1 alternating.                           */
					i386_mov_mem_reg(cd, a, REG_ITMP1);
					i386_mov_mem_reg(cd, a + 4, REG_ITMP2);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
				} else {
					panic("GETSTATIC: longs have to be in memory");
				}
				break;
			case TYPE_FLT:
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				i386_flds_mem(cd, a);
				fpu_st_offset++;
				store_reg_to_var_flt(iptr->dst, d);
				break;
			case TYPE_DBL:				
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				i386_fldl_mem(cd, a);
				fpu_st_offset++;
				store_reg_to_var_flt(iptr->dst, d);
				break;
			default:
				throw_cacao_exception_exit(string_java_lang_InternalError,
										   "Unknown GETSTATIC operand type %d",
										   iptr->op1);
			}
			break;

		case ICMD_PUTFIELD:   /* ..., value  ==> ...                          */
		                      /* op1 = type, val.i = field offset             */

			a = ((fieldinfo *) (iptr->val.a))->offset;
			switch (iptr->op1) {
			case TYPE_INT:
			case TYPE_ADR:
				var_to_reg_int(s1, src->prev, REG_ITMP1);
				var_to_reg_int(s2, src, REG_ITMP2);
				gen_nullptr_check(s1);
				i386_mov_reg_membase(cd, s2, s1, a);
				break;
			case TYPE_LNG:
				var_to_reg_int(s1, src->prev, REG_ITMP1);
				gen_nullptr_check(s1);
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP2);
					i386_mov_reg_membase(cd, REG_ITMP2, s1, a);
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8 + 4, REG_ITMP2);
					i386_mov_reg_membase(cd, REG_ITMP2, s1, a + 4);
				} else {
					panic("PUTFIELD: longs have to be in memory");
				}
				break;
			case TYPE_FLT:
				var_to_reg_int(s1, src->prev, REG_ITMP1);
				var_to_reg_flt(s2, src, REG_FTMP1);
				gen_nullptr_check(s1);
				i386_fstps_membase(cd, s1, a);
				fpu_st_offset--;
				break;
			case TYPE_DBL:
				var_to_reg_int(s1, src->prev, REG_ITMP1);
				var_to_reg_flt(s2, src, REG_FTMP1);
				gen_nullptr_check(s1);
				i386_fstpl_membase(cd, s1, a);
				fpu_st_offset--;
				break;
			default:
				throw_cacao_exception_exit(string_java_lang_InternalError,
										   "Unknown PUTFIELD operand type %d",
										   iptr->op1);
			}
			break;

		case ICMD_GETFIELD:   /* ...  ==> ..., value                          */
		                      /* op1 = type, val.i = field offset             */

			a = ((fieldinfo *) (iptr->val.a))->offset;
			switch (iptr->op1) {
			case TYPE_INT:
			case TYPE_ADR:
				var_to_reg_int(s1, src, REG_ITMP1);
				d = reg_of_var(rd, iptr->dst, REG_ITMP2);
				gen_nullptr_check(s1);
				i386_mov_membase_reg(cd, s1, a, d);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_LNG:
				var_to_reg_int(s1, src, REG_ITMP1);
				d = reg_of_var(rd, iptr->dst, REG_NULL);
				gen_nullptr_check(s1);
				i386_mov_membase_reg(cd, s1, a, REG_ITMP2);
				i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, iptr->dst->regoff * 8);
				i386_mov_membase_reg(cd, s1, a + 4, REG_ITMP2);
				i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
				break;
			case TYPE_FLT:
				var_to_reg_int(s1, src, REG_ITMP1);
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				gen_nullptr_check(s1);
				i386_flds_membase(cd, s1, a);
				fpu_st_offset++;
				store_reg_to_var_flt(iptr->dst, d);
				break;
			case TYPE_DBL:				
				var_to_reg_int(s1, src, REG_ITMP1);
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				gen_nullptr_check(s1);
				i386_fldl_membase(cd, s1, a);
				fpu_st_offset++;
				store_reg_to_var_flt(iptr->dst, d);
				break;
			default:
				throw_cacao_exception_exit(string_java_lang_InternalError,
										   "Unknown GETFIELD operand type %d",
										   iptr->op1);
			}
			break;


		/* branch operations **************************************************/

		case ICMD_ATHROW:       /* ..., objectref ==> ... (, objectref)       */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_INTMOVE(s1, REG_ITMP1_XPTR);

			i386_call_imm(cd, 0);                /* passing exception pointer */
			i386_pop_reg(cd, REG_ITMP2_XPC);

  			i386_mov_imm_reg(cd, (s4) asm_handle_exception, REG_ITMP3);
  			i386_jmp_reg(cd, REG_ITMP3);
			ALIGNCODENOP;
			break;

		case ICMD_GOTO:         /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */

			i386_jmp_imm(cd, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
  			ALIGNCODENOP;
			break;

		case ICMD_JSR:          /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */

  			i386_call_imm(cd, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;
			
		case ICMD_RET:          /* ... ==> ...                                */
		                        /* op1 = local variable                       */

			var = &(rd->locals[iptr->op1][TYPE_ADR]);
			var_to_reg_int(s1, var, REG_ITMP1);
			i386_jmp_reg(cd, s1);
			break;

		case ICMD_IFNULL:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, I386_CMP, 0, REG_SP, src->regoff * 8);

			} else {
				i386_test_reg_reg(cd, src->regoff, src->regoff);
			}
			i386_jcc(cd, I386_CC_E, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IFNONNULL:    /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, I386_CMP, 0, REG_SP, src->regoff * 8);

			} else {
				i386_test_reg_reg(cd, src->regoff, src->regoff);
			}
			i386_jcc(cd, I386_CC_NE, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IFEQ:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, I386_CMP, iptr->val.i, REG_SP, src->regoff * 8);

			} else {
				i386_alu_imm_reg(cd, I386_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(cd, I386_CC_E, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IFLT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, I386_CMP, iptr->val.i, REG_SP, src->regoff * 8);

			} else {
				i386_alu_imm_reg(cd, I386_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(cd, I386_CC_L, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IFLE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, I386_CMP, iptr->val.i, REG_SP, src->regoff * 8);

			} else {
				i386_alu_imm_reg(cd, I386_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(cd, I386_CC_LE, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IFNE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, I386_CMP, iptr->val.i, REG_SP, src->regoff * 8);

			} else {
				i386_alu_imm_reg(cd, I386_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(cd, I386_CC_NE, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IFGT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, I386_CMP, iptr->val.i, REG_SP, src->regoff * 8);

			} else {
				i386_alu_imm_reg(cd, I386_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(cd, I386_CC_G, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IFGE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, I386_CMP, iptr->val.i, REG_SP, src->regoff * 8);

			} else {
				i386_alu_imm_reg(cd, I386_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(cd, I386_CC_GE, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IF_LEQ:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			if (src->flags & INMEMORY) {
				if (iptr->val.l == 0) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
					i386_alu_membase_reg(cd, I386_OR, REG_SP, src->regoff * 8 + 4, REG_ITMP1);

				} else {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8 + 4, REG_ITMP2);
					i386_alu_imm_reg(cd, I386_XOR, iptr->val.l >> 32, REG_ITMP2);
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
					i386_alu_imm_reg(cd, I386_XOR, iptr->val.l, REG_ITMP1);
					i386_alu_reg_reg(cd, I386_OR, REG_ITMP2, REG_ITMP1);
				}
			}
			i386_test_reg_reg(cd, REG_ITMP1, REG_ITMP1);
			i386_jcc(cd, I386_CC_E, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IF_LLT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, I386_CMP, iptr->val.l >> 32, REG_SP, src->regoff * 8 + 4);
				i386_jcc(cd, I386_CC_L, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);

				a = 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				CALCIMMEDIATEBYTES(a, iptr->val.l);

				i386_jcc(cd, I386_CC_G, a);

				i386_alu_imm_membase(cd, I386_CMP, iptr->val.l, REG_SP, src->regoff * 8);
				i386_jcc(cd, I386_CC_B, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			}			
			break;

		case ICMD_IF_LLE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, I386_CMP, iptr->val.l >> 32, REG_SP, src->regoff * 8 + 4);
				i386_jcc(cd, I386_CC_L, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);

				a = 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				CALCIMMEDIATEBYTES(a, iptr->val.l);
				
				i386_jcc(cd, I386_CC_G, a);

				i386_alu_imm_membase(cd, I386_CMP, iptr->val.l, REG_SP, src->regoff * 8);
				i386_jcc(cd, I386_CC_BE, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			}			
			break;

		case ICMD_IF_LNE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			if (src->flags & INMEMORY) {
				if (iptr->val.l == 0) {
					i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
					i386_alu_membase_reg(cd, I386_OR, REG_SP, src->regoff * 8 + 4, REG_ITMP1);

				} else {
					i386_mov_imm_reg(cd, iptr->val.l, REG_ITMP1);
					i386_alu_membase_reg(cd, I386_XOR, REG_SP, src->regoff * 8, REG_ITMP1);
					i386_mov_imm_reg(cd, iptr->val.l >> 32, REG_ITMP2);
					i386_alu_membase_reg(cd, I386_XOR, REG_SP, src->regoff * 8 + 4, REG_ITMP2);
					i386_alu_reg_reg(cd, I386_OR, REG_ITMP2, REG_ITMP1);
				}
			}
			i386_test_reg_reg(cd, REG_ITMP1, REG_ITMP1);
			i386_jcc(cd, I386_CC_NE, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IF_LGT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, I386_CMP, iptr->val.l >> 32, REG_SP, src->regoff * 8 + 4);
				i386_jcc(cd, I386_CC_G, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);

				a = 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				CALCIMMEDIATEBYTES(a, iptr->val.l);

				i386_jcc(cd, I386_CC_L, a);

				i386_alu_imm_membase(cd, I386_CMP, iptr->val.l, REG_SP, src->regoff * 8);
				i386_jcc(cd, I386_CC_A, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			}			
			break;

		case ICMD_IF_LGE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, I386_CMP, iptr->val.l >> 32, REG_SP, src->regoff * 8 + 4);
				i386_jcc(cd, I386_CC_G, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);

				a = 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				CALCIMMEDIATEBYTES(a, iptr->val.l);

				i386_jcc(cd, I386_CC_L, a);

				i386_alu_imm_membase(cd, I386_CMP, iptr->val.l, REG_SP, src->regoff * 8);
				i386_jcc(cd, I386_CC_AE, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			}			
			break;

		case ICMD_IF_ICMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_ACMPEQ:    /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_reg_membase(cd, I386_CMP, REG_ITMP1, REG_SP, src->prev->regoff * 8);

			} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
				i386_alu_membase_reg(cd, I386_CMP, REG_SP, src->regoff * 8, src->prev->regoff);

			} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_alu_reg_membase(cd, I386_CMP, src->regoff, REG_SP, src->prev->regoff * 8);

			} else {
				i386_alu_reg_reg(cd, I386_CMP, src->regoff, src->prev->regoff);
			}
			i386_jcc(cd, I386_CC_E, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IF_LCMPEQ:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2);
				i386_alu_membase_reg(cd, I386_XOR, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_membase_reg(cd, I386_XOR, REG_SP, src->regoff * 8 + 4, REG_ITMP2);
				i386_alu_reg_reg(cd, I386_OR, REG_ITMP2, REG_ITMP1);
				i386_test_reg_reg(cd, REG_ITMP1, REG_ITMP1);
			}			
			i386_jcc(cd, I386_CC_E, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IF_ICMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_ACMPNE:    /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_reg_membase(cd, I386_CMP, REG_ITMP1, REG_SP, src->prev->regoff * 8);

			} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
				i386_alu_membase_reg(cd, I386_CMP, REG_SP, src->regoff * 8, src->prev->regoff);

			} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_alu_reg_membase(cd, I386_CMP, src->regoff, REG_SP, src->prev->regoff * 8);

			} else {
				i386_alu_reg_reg(cd, I386_CMP, src->regoff, src->prev->regoff);
			}
			i386_jcc(cd, I386_CC_NE, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IF_LCMPNE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2);
				i386_alu_membase_reg(cd, I386_XOR, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_membase_reg(cd, I386_XOR, REG_SP, src->regoff * 8 + 4, REG_ITMP2);
				i386_alu_reg_reg(cd, I386_OR, REG_ITMP2, REG_ITMP1);
				i386_test_reg_reg(cd, REG_ITMP1, REG_ITMP1);
			}			
			i386_jcc(cd, I386_CC_NE, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IF_ICMPLT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_reg_membase(cd, I386_CMP, REG_ITMP1, REG_SP, src->prev->regoff * 8);

			} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
				i386_alu_membase_reg(cd, I386_CMP, REG_SP, src->regoff * 8, src->prev->regoff);

			} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_alu_reg_membase(cd, I386_CMP, src->regoff, REG_SP, src->prev->regoff * 8);

			} else {
				i386_alu_reg_reg(cd, I386_CMP, src->regoff, src->prev->regoff);
			}
			i386_jcc(cd, I386_CC_L, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IF_LCMPLT:    /* ..., value, value ==> ...                  */
	                            /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8 + 4, REG_ITMP1);
				i386_alu_membase_reg(cd, I386_CMP, REG_SP, src->regoff * 8 + 4, REG_ITMP1);
				i386_jcc(cd, I386_CC_L, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);

				a = 3 + 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->prev->regoff * 8);
				CALCOFFSETBYTES(a, REG_SP, src->regoff);

				i386_jcc(cd, I386_CC_G, a);

				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_alu_membase_reg(cd, I386_CMP, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_jcc(cd, I386_CC_B, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			}			
			break;

		case ICMD_IF_ICMPGT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_reg_membase(cd, I386_CMP, REG_ITMP1, REG_SP, src->prev->regoff * 8);

			} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
				i386_alu_membase_reg(cd, I386_CMP, REG_SP, src->regoff * 8, src->prev->regoff);

			} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_alu_reg_membase(cd, I386_CMP, src->regoff, REG_SP, src->prev->regoff * 8);

			} else {
				i386_alu_reg_reg(cd, I386_CMP, src->regoff, src->prev->regoff);
			}
			i386_jcc(cd, I386_CC_G, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IF_LCMPGT:    /* ..., value, value ==> ...                  */
                                /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8 + 4, REG_ITMP1);
				i386_alu_membase_reg(cd, I386_CMP, REG_SP, src->regoff * 8 + 4, REG_ITMP1);
				i386_jcc(cd, I386_CC_G, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);

				a = 3 + 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->prev->regoff * 8);
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);

				i386_jcc(cd, I386_CC_L, a);

				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_alu_membase_reg(cd, I386_CMP, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_jcc(cd, I386_CC_A, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			}			
			break;

		case ICMD_IF_ICMPLE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_reg_membase(cd, I386_CMP, REG_ITMP1, REG_SP, src->prev->regoff * 8);

			} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
				i386_alu_membase_reg(cd, I386_CMP, REG_SP, src->regoff * 8, src->prev->regoff);

			} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_alu_reg_membase(cd, I386_CMP, src->regoff, REG_SP, src->prev->regoff * 8);

			} else {
				i386_alu_reg_reg(cd, I386_CMP, src->regoff, src->prev->regoff);
			}
			i386_jcc(cd, I386_CC_LE, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IF_LCMPLE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8 + 4, REG_ITMP1);
				i386_alu_membase_reg(cd, I386_CMP, REG_SP, src->regoff * 8 + 4, REG_ITMP1);
				i386_jcc(cd, I386_CC_L, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);

				a = 3 + 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->prev->regoff * 8);
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);

				i386_jcc(cd, I386_CC_G, a);

				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_alu_membase_reg(cd, I386_CMP, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_jcc(cd, I386_CC_BE, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			}			
			break;

		case ICMD_IF_ICMPGE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_reg_membase(cd, I386_CMP, REG_ITMP1, REG_SP, src->prev->regoff * 8);

			} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
				i386_alu_membase_reg(cd, I386_CMP, REG_SP, src->regoff * 8, src->prev->regoff);

			} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_alu_reg_membase(cd, I386_CMP, src->regoff, REG_SP, src->prev->regoff * 8);

			} else {
				i386_alu_reg_reg(cd, I386_CMP, src->regoff, src->prev->regoff);
			}
			i386_jcc(cd, I386_CC_GE, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			break;

		case ICMD_IF_LCMPGE:    /* ..., value, value ==> ...                  */
	                            /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8 + 4, REG_ITMP1);
				i386_alu_membase_reg(cd, I386_CMP, REG_SP, src->regoff * 8 + 4, REG_ITMP1);
				i386_jcc(cd, I386_CC_G, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);

				a = 3 + 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->prev->regoff * 8);
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);

				i386_jcc(cd, I386_CC_L, a);

				i386_mov_membase_reg(cd, REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_alu_membase_reg(cd, I386_CMP, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_jcc(cd, I386_CC_AE, 0);
				codegen_addreference(cd, BlockPtrOfPC(iptr->op1), cd->mcodeptr);
			}			
			break;

		/* (value xx 0) ? IFxx_ICONST : ELSE_ICONST                           */

		case ICMD_ELSE_ICONST:  /* handled by IFxx_ICONST                     */
			break;

		case ICMD_IFEQ_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ifcc_iconst(cd, I386_CC_NE, src, iptr);
			break;

		case ICMD_IFNE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ifcc_iconst(cd, I386_CC_E, src, iptr);
			break;

		case ICMD_IFLT_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ifcc_iconst(cd, I386_CC_GE, src, iptr);
			break;

		case ICMD_IFGE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ifcc_iconst(cd, I386_CC_L, src, iptr);
			break;

		case ICMD_IFGT_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ifcc_iconst(cd, I386_CC_LE, src, iptr);
			break;

		case ICMD_IFLE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			d = reg_of_var(rd, iptr->dst, REG_NULL);
			i386_emit_ifcc_iconst(cd, I386_CC_G, src, iptr);
			break;


		case ICMD_IRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_ARETURN:

			var_to_reg_int(s1, src, REG_RESULT);
			M_INTMOVE(s1, REG_RESULT);

			goto nowperformreturn;

		case ICMD_LRETURN:      /* ..., retvalue ==> ...                      */

			if (src->flags & INMEMORY) {
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_RESULT);
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 8 + 4, REG_RESULT2);

			} else {
				panic("LRETURN: longs have to be in memory");
			}

			goto nowperformreturn;

		case ICMD_FRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_DRETURN:      /* ..., retvalue ==> ...                      */

			var_to_reg_flt(s1, src, REG_FRESULT);
			/* this may be an early return -- keep the offset correct for the
			   remaining code */
			fpu_st_offset--;

			goto nowperformreturn;

		case ICMD_RETURN:      /* ...  ==> ...                                */

nowperformreturn:
			{
			s4 i, p;
			
  			p = parentargs_base;
			
			/* call trace function */
			if (runverbose) {
				i386_alu_imm_reg(cd, I386_SUB, 4 + 8 + 8 + 4, REG_SP);

				i386_mov_imm_membase(cd, (s4) m, REG_SP, 0);

				i386_mov_reg_membase(cd, REG_RESULT, REG_SP, 4);
				i386_mov_reg_membase(cd, REG_RESULT2, REG_SP, 4 + 4);
				
				i386_fstl_membase(cd, REG_SP, 4 + 8);
				i386_fsts_membase(cd, REG_SP, 4 + 8 + 8);

  				i386_mov_imm_reg(cd, (s4) builtin_displaymethodstop, REG_ITMP1);
				i386_call_reg(cd, REG_ITMP1);

				i386_mov_membase_reg(cd, REG_SP, 4, REG_RESULT);
				i386_mov_membase_reg(cd, REG_SP, 4 + 4, REG_RESULT2);

				i386_alu_imm_reg(cd, I386_ADD, 4 + 8 + 8 + 4, REG_SP);
			}

#if defined(USE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				i386_mov_membase_reg(cd, REG_SP, 8 * rd->maxmemuse, REG_ITMP2);

				/* we need to save the proper return value */
				switch (iptr->opc) {
				case ICMD_IRETURN:
				case ICMD_ARETURN:
					i386_mov_reg_membase(cd, REG_RESULT, REG_SP, rd->maxmemuse * 8);
					break;

				case ICMD_LRETURN:
					i386_mov_reg_membase(cd, REG_RESULT, REG_SP, rd->maxmemuse * 8);
					i386_mov_reg_membase(cd, REG_RESULT2, REG_SP, rd->maxmemuse * 8 + 4);
					break;

				case ICMD_FRETURN:
					i386_fsts_membase(cd, REG_SP, rd->maxmemuse * 8);
					break;

				case ICMD_DRETURN:
					i386_fstl_membase(cd, REG_SP, rd->maxmemuse * 8);
					break;
				}

				i386_alu_imm_reg(cd, I386_SUB, 4, REG_SP);
				i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, 0);
				i386_mov_imm_reg(cd, (s4) builtin_monitorexit, REG_ITMP1);
				i386_call_reg(cd, REG_ITMP1);
				i386_alu_imm_reg(cd, I386_ADD, 4, REG_SP);

				/* and now restore the proper return value */
				switch (iptr->opc) {
				case ICMD_IRETURN:
				case ICMD_ARETURN:
					i386_mov_membase_reg(cd, REG_SP, rd->maxmemuse * 8, REG_RESULT);
					break;

				case ICMD_LRETURN:
					i386_mov_membase_reg(cd, REG_SP, rd->maxmemuse * 8, REG_RESULT);
					i386_mov_membase_reg(cd, REG_SP, rd->maxmemuse * 8 + 4, REG_RESULT2);
					break;

				case ICMD_FRETURN:
					i386_flds_membase(cd, REG_SP, rd->maxmemuse * 8);
					break;

				case ICMD_DRETURN:
					i386_fldl_membase(cd, REG_SP, rd->maxmemuse * 8);
					break;
				}
			}
#endif

			/* restore saved registers */
			for (i = rd->savintregcnt - 1; i >= rd->maxsavintreguse; i--) {
				p--;
				i386_mov_membase_reg(cd, REG_SP, p * 8, rd->savintregs[i]);
			}
			for (i = rd->savfltregcnt - 1; i >= rd->maxsavfltreguse; i--) {
  				p--;
				i386_fldl_membase(cd, REG_SP, p * 8);
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
				i386_alu_imm_reg(cd, I386_ADD, parentargs_base * 8, REG_SP);
			}

			i386_ret(cd);
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
					i386_alu_imm_reg(cd, I386_SUB, l, REG_ITMP1);
				}
				i = i - l + 1;

                /* range check */

				i386_alu_imm_reg(cd, I386_CMP, i - 1, REG_ITMP1);
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
					i386_alu_imm_reg(cd, I386_CMP, val, s1);
					i386_jcc(cd, I386_CC_E, 0);
					/* codegen_addreference(cd, BlockPtrOfPC(s4ptr[1]), cd->mcodeptr); */
					codegen_addreference(cd, (basicblock *) tptr[0], cd->mcodeptr); 
				}

				i386_jmp_imm(cd, 0);
				/* codegen_addreference(cd, BlockPtrOfPC(l), cd->mcodeptr); */
			
				tptr = (void **) iptr->target;
				codegen_addreference(cd, (basicblock *) tptr[0], cd->mcodeptr);

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
			methodinfo *lm;

			MCODECHECK((s3 << 1) + 64);

			/* copy arguments to registers or stack location                  */

			for (; --s3 >= 0; src = src->prev) {
				if (src->varkind == ARGVAR) {
					continue;
				}

				if (IS_INT_LNG_TYPE(src->type)) {
					if (s3 < rd->intreg_argnum) {
						panic("No integer argument registers available!");

					} else {
						if (!IS_2_WORD_TYPE(src->type)) {
							if (src->flags & INMEMORY) {
								i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
								i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, s3 * 8);

							} else {
								i386_mov_reg_membase(cd, src->regoff, REG_SP, s3 * 8);
							}

						} else {
							if (src->flags & INMEMORY) {
								M_LNGMEMMOVE(src->regoff, s3);

							} else {
								panic("copy arguments: longs have to be in memory");
							}
						}
					}

				} else {
					if (s3 < rd->fltreg_argnum) {
						panic("No float argument registers available!");

					} else {
						var_to_reg_flt(d, src, REG_FTMP1);
						if (src->type == TYPE_FLT) {
							i386_fstps_membase(cd, REG_SP, s3 * 8);

						} else {
							i386_fstpl_membase(cd, REG_SP, s3 * 8);
						}
					}
				}
			} /* end of for */

			lm = iptr->val.a;
			switch (iptr->opc) {
			case ICMD_BUILTIN3:
			case ICMD_BUILTIN2:
			case ICMD_BUILTIN1:
				a = (u4) lm;
				d = iptr->op1;

				i386_mov_imm_reg(cd, a, REG_ITMP1);
				i386_call_reg(cd, REG_ITMP1);
				break;

			case ICMD_INVOKESTATIC:
				a = (u4) lm->stubroutine;
				d = lm->returntype;

				i386_mov_imm_reg(cd, a, REG_ITMP2);
				i386_call_reg(cd, REG_ITMP2);
				break;

			case ICMD_INVOKESPECIAL:
				a = (u4) lm->stubroutine;
				d = lm->returntype;

				i386_mov_membase_reg(cd, REG_SP, 0, REG_ITMP1);
				gen_nullptr_check(REG_ITMP1);
				i386_mov_membase_reg(cd, REG_ITMP1, 0, REG_ITMP1);    /* access memory for hardware nullptr */

				i386_mov_imm_reg(cd, a, REG_ITMP2);
				i386_call_reg(cd, REG_ITMP2);
				break;

			case ICMD_INVOKEVIRTUAL:
				d = lm->returntype;

				i386_mov_membase_reg(cd, REG_SP, 0, REG_ITMP1);
				gen_nullptr_check(REG_ITMP1);
				i386_mov_membase_reg(cd, REG_ITMP1, OFFSET(java_objectheader, vftbl), REG_ITMP2);
				i386_mov_membase32_reg(cd, REG_ITMP2, OFFSET(vftbl_t, table[0]) + sizeof(methodptr) * lm->vftblindex, REG_ITMP1);

				i386_call_reg(cd, REG_ITMP1);
				break;

			case ICMD_INVOKEINTERFACE:
				d = lm->returntype;

				i386_mov_membase_reg(cd, REG_SP, 0, REG_ITMP1);
				gen_nullptr_check(REG_ITMP1);
				i386_mov_membase_reg(cd, REG_ITMP1, OFFSET(java_objectheader, vftbl), REG_ITMP1);
				i386_mov_membase_reg(cd, REG_ITMP1, OFFSET(vftbl_t, interfacetable[0]) - sizeof(methodptr) * lm->class->index, REG_ITMP2);
				i386_mov_membase32_reg(cd, REG_ITMP2, sizeof(methodptr) * (lm - lm->class->methods), REG_ITMP1);

				i386_call_reg(cd, REG_ITMP1);
				break;
			}

			/* d contains return type */

			if (d != TYPE_VOID) {
				d = reg_of_var(rd, iptr->dst, REG_NULL);

				if (IS_INT_LNG_TYPE(iptr->dst->type)) {
					if (IS_2_WORD_TYPE(iptr->dst->type)) {
						if (iptr->dst->flags & INMEMORY) {
							i386_mov_reg_membase(cd, REG_RESULT, REG_SP, iptr->dst->regoff * 8);
							i386_mov_reg_membase(cd, REG_RESULT2, REG_SP, iptr->dst->regoff * 8 + 4);

						} else {
  							panic("RETURN: longs have to be in memory");
						}

					} else {
						if (iptr->dst->flags & INMEMORY) {
							i386_mov_reg_membase(cd, REG_RESULT, REG_SP, iptr->dst->regoff * 8);

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
			
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			codegen_threadcritrestart(cd, cd->mcodeptr - cd->mcodebase);
#endif
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (s1 == d) {
				M_INTMOVE(s1, REG_ITMP1);
				s1 = REG_ITMP1;
			}
			i386_alu_reg_reg(cd, I386_XOR, d, d);
			if (iptr->op1) {                               /* class/interface */
				if (super->flags & ACC_INTERFACE) {        /* interface       */
					i386_test_reg_reg(cd, s1, s1);

					/* TODO: clean up this calculation */
					a = 2;
					CALCOFFSETBYTES(a, s1, OFFSET(java_objectheader, vftbl));

					a += 2;
					CALCOFFSETBYTES(a, REG_ITMP1, OFFSET(vftbl_t, interfacetablelength));
					
					a += 2;
/*  					CALCOFFSETBYTES(a, super->index); */
					CALCIMMEDIATEBYTES(a, super->index);
					
					a += 3;
					a += 6;

					a += 2;
					CALCOFFSETBYTES(a, REG_ITMP1, OFFSET(vftbl_t, interfacetable[0]) - super->index * sizeof(methodptr*));

					a += 3;

					a += 6;    /* jcc */
					a += 5;

					i386_jcc(cd, I386_CC_E, a);

					i386_mov_membase_reg(cd, s1, OFFSET(java_objectheader, vftbl), REG_ITMP1);
					i386_mov_membase_reg(cd, REG_ITMP1, OFFSET(vftbl_t, interfacetablelength), REG_ITMP2);
					i386_alu_imm_reg(cd, I386_SUB, super->index, REG_ITMP2);
					/* TODO: test */
					i386_alu_imm_reg(cd, I386_CMP, 0, REG_ITMP2);

					/* TODO: clean up this calculation */
					a = 0;
					a += 2;
					CALCOFFSETBYTES(a, REG_ITMP1, OFFSET(vftbl_t, interfacetable[0]) - super->index * sizeof(methodptr*));

					a += 3;

					a += 6;    /* jcc */
					a += 5;

					i386_jcc(cd, I386_CC_LE, a);
					i386_mov_membase_reg(cd, REG_ITMP1, OFFSET(vftbl_t, interfacetable[0]) - super->index * sizeof(methodptr*), REG_ITMP1);
					/* TODO: test */
					i386_alu_imm_reg(cd, I386_CMP, 0, REG_ITMP1);
/*  					i386_setcc_reg(cd, I386_CC_A, d); */
/*  					i386_jcc(cd, I386_CC_BE, 5); */
					i386_jcc(cd, I386_CC_E, 5);
					i386_mov_imm_reg(cd, 1, d);
					

				} else {                                   /* class           */
					i386_test_reg_reg(cd, s1, s1);

					/* TODO: clean up this calculation */
					a = 2;
					CALCOFFSETBYTES(a, s1, OFFSET(java_objectheader, vftbl));
					a += 5;
					a += 2;
					CALCOFFSETBYTES(a, REG_ITMP1, OFFSET(vftbl_t, baseval));
					a += 2;
					CALCOFFSETBYTES(a, REG_ITMP2, OFFSET(vftbl_t, baseval));
					
					a += 2;
					CALCOFFSETBYTES(a, REG_ITMP2, OFFSET(vftbl_t, diffval));
					
					a += 2;
					a += 2;    /* xor */

					a += 2;

					a += 6;    /* jcc */
					a += 5;

					i386_jcc(cd, I386_CC_E, a);

					i386_mov_membase_reg(cd, s1, OFFSET(java_objectheader, vftbl), REG_ITMP1);
					i386_mov_imm_reg(cd, (s4) super->vftbl, REG_ITMP2);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
					codegen_threadcritstart(cd, cd->mcodeptr - cd->mcodebase);
#endif
					i386_mov_membase_reg(cd, REG_ITMP1, OFFSET(vftbl_t, baseval), REG_ITMP1);
					i386_mov_membase_reg(cd, REG_ITMP2, OFFSET(vftbl_t, baseval), REG_ITMP3);
					i386_mov_membase_reg(cd, REG_ITMP2, OFFSET(vftbl_t, diffval), REG_ITMP2);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
					codegen_threadcritstop(cd, cd->mcodeptr - cd->mcodebase);
#endif
					i386_alu_reg_reg(cd, I386_SUB, REG_ITMP3, REG_ITMP1);
					i386_alu_reg_reg(cd, I386_XOR, d, d);

					i386_alu_reg_reg(cd, I386_CMP, REG_ITMP2, REG_ITMP1);
					i386_jcc(cd, I386_CC_A, 5);
					i386_mov_imm_reg(cd, 1, d);
				}
			}
			else
				panic ("internal error: no inlined array instanceof");
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
			
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			codegen_threadcritrestart(cd, cd->mcodeptr - cd->mcodebase);
#endif
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			var_to_reg_int(s1, src, d);
			if (iptr->op1) {                               /* class/interface */
				if (super->flags & ACC_INTERFACE) {        /* interface       */
					i386_test_reg_reg(cd, s1, s1);

					/* TODO: clean up this calculation */
					a = 2;
					CALCOFFSETBYTES(a, s1, OFFSET(java_objectheader, vftbl));

					a += 2;
					CALCOFFSETBYTES(a, REG_ITMP1, OFFSET(vftbl_t, interfacetablelength));

					a += 2;
/*  					CALCOFFSETBYTES(a, super->index); */
					CALCIMMEDIATEBYTES(a, super->index);

					a += 3;
					a += 6;

					a += 2;
					CALCOFFSETBYTES(a, REG_ITMP1, OFFSET(vftbl_t, interfacetable[0]) - super->index * sizeof(methodptr*));

					a += 3;
					a += 6;

					i386_jcc(cd, I386_CC_E, a);

					i386_mov_membase_reg(cd, s1, OFFSET(java_objectheader, vftbl), REG_ITMP1);
					i386_mov_membase_reg(cd, REG_ITMP1, OFFSET(vftbl_t, interfacetablelength), REG_ITMP2);
					i386_alu_imm_reg(cd, I386_SUB, super->index, REG_ITMP2);
					/* TODO: test */
					i386_alu_imm_reg(cd, I386_CMP, 0, REG_ITMP2);
					i386_jcc(cd, I386_CC_LE, 0);
					codegen_addxcastrefs(cd, cd->mcodeptr);
					i386_mov_membase_reg(cd, REG_ITMP1, OFFSET(vftbl_t, interfacetable[0]) - super->index * sizeof(methodptr*), REG_ITMP2);
					/* TODO: test */
					i386_alu_imm_reg(cd, I386_CMP, 0, REG_ITMP2);
					i386_jcc(cd, I386_CC_E, 0);
					codegen_addxcastrefs(cd, cd->mcodeptr);

				} else {                                     /* class           */
					i386_test_reg_reg(cd, s1, s1);

					/* TODO: clean up this calculation */
					a = 2;
					CALCOFFSETBYTES(a, s1, OFFSET(java_objectheader, vftbl));

					a += 5;

					a += 2;
					CALCOFFSETBYTES(a, REG_ITMP1, OFFSET(vftbl_t, baseval));

					if (d != REG_ITMP3) {
						a += 2;
						CALCOFFSETBYTES(a, REG_ITMP2, OFFSET(vftbl_t, baseval));
						
						a += 2;
						CALCOFFSETBYTES(a, REG_ITMP2, OFFSET(vftbl_t, diffval));

						a += 2;
						
					} else {
						a += 2;
						CALCOFFSETBYTES(a, REG_ITMP2, OFFSET(vftbl_t, baseval));

						a += 2;

						a += 5;

						a += 2;
						CALCOFFSETBYTES(a, REG_ITMP2, OFFSET(vftbl_t, diffval));
					}

					a += 2;

					a += 6;

					i386_jcc(cd, I386_CC_E, a);

					i386_mov_membase_reg(cd, s1, OFFSET(java_objectheader, vftbl), REG_ITMP1);
					i386_mov_imm_reg(cd, (s4) super->vftbl, REG_ITMP2);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
					codegen_threadcritstart(cd, cd->mcodeptr - cd->mcodebase);
#endif
					i386_mov_membase_reg(cd, REG_ITMP1, OFFSET(vftbl_t, baseval), REG_ITMP1);
					if (d != REG_ITMP3) {
						i386_mov_membase_reg(cd, REG_ITMP2, OFFSET(vftbl_t, baseval), REG_ITMP3);
						i386_mov_membase_reg(cd, REG_ITMP2, OFFSET(vftbl_t, diffval), REG_ITMP2);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
						codegen_threadcritstop(cd, cd->mcodeptr - cd->mcodebase);
#endif
						i386_alu_reg_reg(cd, I386_SUB, REG_ITMP3, REG_ITMP1);

					} else {
						i386_mov_membase_reg(cd, REG_ITMP2, OFFSET(vftbl_t, baseval), REG_ITMP2);
						i386_alu_reg_reg(cd, I386_SUB, REG_ITMP2, REG_ITMP1);
						i386_mov_imm_reg(cd, (s4) super->vftbl, REG_ITMP2);
						i386_mov_membase_reg(cd, REG_ITMP2, OFFSET(vftbl_t, diffval), REG_ITMP2);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
						codegen_threadcritstop(cd, cd->mcodeptr - cd->mcodebase);
#endif
					}

					i386_alu_reg_reg(cd, I386_CMP, REG_ITMP2, REG_ITMP1);
					i386_jcc(cd, I386_CC_A, 0);    /* (u) REG_ITMP1 > (u) REG_ITMP2 -> jump */
					codegen_addxcastrefs(cd, cd->mcodeptr);
				}

			} else
				panic ("internal error: no inlined array checkcast");
			}
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_CHECKASIZE:  /* ..., size ==> ..., size                     */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(cd, I386_CMP, 0, REG_SP, src->regoff * 8);
				
			} else {
				i386_test_reg_reg(cd, src->regoff, src->regoff);
			}
			i386_jcc(cd, I386_CC_L, 0);
			codegen_addxcheckarefs(cd, cd->mcodeptr);
			break;

		case ICMD_CHECKEXCEPTION:  /* ... ==> ...                             */

			i386_test_reg_reg(cd, REG_RESULT, REG_RESULT);
			i386_jcc(cd, I386_CC_E, 0);
			codegen_addxexceptionrefs(cd, cd->mcodeptr);
			break;

		case ICMD_MULTIANEWARRAY:/* ..., cnt1, [cnt2, ...] ==> ..., arrayref  */
		                      /* op1 = dimension, val.a = array descriptor    */

			/* check for negative sizes and copy sizes to stack if necessary  */

  			MCODECHECK((iptr->op1 << 1) + 64);

			for (s1 = iptr->op1; --s1 >= 0; src = src->prev) {
				if (src->flags & INMEMORY) {
					i386_alu_imm_membase(cd, I386_CMP, 0, REG_SP, src->regoff * 8);

				} else {
					i386_test_reg_reg(cd, src->regoff, src->regoff);
				}
				i386_jcc(cd, I386_CC_L, 0);
				codegen_addxcheckarefs(cd, cd->mcodeptr);

				/* 
				 * copy sizes to new stack location, be cause native function
				 * builtin_nmultianewarray access them as (int *)
				 */
				i386_mov_membase_reg(cd, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, -(iptr->op1 - s1) * 4);

				/* copy sizes to stack (argument numbers >= INT_ARG_CNT)      */

				if (src->varkind != ARGVAR) {
					if (src->flags & INMEMORY) {
						i386_mov_membase_reg(cd, REG_SP, (src->regoff + INT_ARG_CNT) * 8, REG_ITMP1);
						i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, (s1 + INT_ARG_CNT) * 8);

					} else {
						i386_mov_reg_membase(cd, src->regoff, REG_SP, (s1 + INT_ARG_CNT) * 8);
					}
				}
			}
			i386_alu_imm_reg(cd, I386_SUB, iptr->op1 * 4, REG_SP);

			/* a0 = dimension count */

			/* save stack pointer */
			M_INTMOVE(REG_SP, REG_ITMP1);

			i386_alu_imm_reg(cd, I386_SUB, 12, REG_SP);
			i386_mov_imm_membase(cd, iptr->op1, REG_SP, 0);

			/* a1 = arraydescriptor */

			i386_mov_imm_membase(cd, (s4) iptr->val.a, REG_SP, 4);

			/* a2 = pointer to dimensions = stack pointer */

			i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, 8);

			i386_mov_imm_reg(cd, (s4) (builtin_nmultianewarray), REG_ITMP1);
			i386_call_reg(cd, REG_ITMP1);
			i386_alu_imm_reg(cd, I386_ADD, 12 + iptr->op1 * 4, REG_SP);

			s1 = reg_of_var(rd, iptr->dst, REG_RESULT);
			M_INTMOVE(REG_RESULT, s1);
			store_reg_to_var_int(iptr->dst, s1);
			break;

		case ICMD_INLINE_START:
		case ICMD_INLINE_END:
			break;
		default:
			error ("Unknown pseudo command: %d", iptr->opc);
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
					panic("double store");
/*  					M_DST(s1, REG_SP, 8 * interfaces[len][s2].regoff); */
				}

			} else {
				var_to_reg_int(s1, src, REG_ITMP1);
				if (!IS_2_WORD_TYPE(rd->interfaces[len][s2].type)) {
					if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
						M_INTMOVE(s1, rd->interfaces[len][s2].regoff);

					} else {
						i386_mov_reg_membase(cd, s1, REG_SP, rd->interfaces[len][s2].regoff * 8);
					}

				} else {
					if (rd->interfaces[len][s2].flags & INMEMORY) {
						M_LNGMEMMOVE(s1, rd->interfaces[len][s2].regoff);

					} else {
						panic("copy interface registers: longs have to be in memory (end)");
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
		i386_alu_reg_reg(cd, I386_ADD, REG_ITMP3, REG_ITMP2_XPC);  /* 2 bytes */

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

			i386_alu_imm_reg(cd, I386_SUB, 1 * 4, REG_SP);
			i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, 0 * 4);
			i386_mov_imm_reg(cd, (u4) new_arrayindexoutofboundsexception, REG_ITMP1);
			i386_call_reg(cd, REG_ITMP1);   /* return value is REG_ITMP1_XPTR */
			i386_alu_imm_reg(cd, I386_ADD, 1 * 4, REG_SP);

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
		i386_alu_reg_reg(cd, I386_ADD, REG_ITMP1, REG_ITMP2_XPC);  /* 2 bytes */

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
			/*i386_alu_imm_reg(cd, I386_ADD, 1 * 4, REG_SP);*/


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
		i386_alu_reg_reg(cd, I386_ADD, REG_ITMP1, REG_ITMP2_XPC);  /* 2 bytes */

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
			/*i386_alu_imm_reg(cd, I386_ADD, 1 * 4, REG_SP);*/


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
		i386_alu_reg_reg(cd, I386_ADD, REG_ITMP1, REG_ITMP2_XPC);  /* 2 bytes */

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
		i386_alu_reg_reg(cd, I386_ADD, REG_ITMP1, REG_ITMP2_XPC);  /* 2 bytes */

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


			i386_mov_imm_reg(cd, (s4) codegen_general_stubcalled, REG_ITMP1);
			i386_call_reg(cd, REG_ITMP1);                

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
			i386_push_imm(cd, (u4) utf_fillInStackTrace_desc);
			i386_push_imm(cd, (u4) utf_fillInStackTrace_name);
			i386_push_reg(cd, REG_ITMP1);
			i386_mov_imm_reg(cd, (s4) class_resolvemethod, REG_ITMP3);
			i386_call_reg(cd, REG_ITMP3);
/*cleanup parameters of class_resolvemethod*/
			i386_alu_imm_reg(cd, I386_ADD,3*4 /*class reference + 2x string reference*/,REG_SP);
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

			i386_alu_imm_reg(cd, I386_ADD,6*4,REG_SP);

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
		i386_alu_reg_reg(cd, I386_ADD, REG_ITMP1, REG_ITMP2_XPC);  /* 2 bytes */
		
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
			i386_alu_imm_reg(cd, I386_SUB, 3*4, REG_SP); /* build stack frame (4 * 4 bytes) */


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
			i386_alu_imm_reg(cd, I386_ADD,3*4,REG_SP);
#endif

			i386_pop_reg(cd, REG_ITMP2_XPC);

			i386_mov_imm_reg(cd, (s4) asm_handle_exception, REG_ITMP3);
			i386_jmp_reg(cd, REG_ITMP3);
		}
	}

	/* generate put/getstatic stub call code */

	{
		clinitref   *cref;
		codegendata *tmpcd;
		u1           xmcode;
		u4           mcode;

		tmpcd = DNEW(codegendata);

		for (cref = cd->clinitrefs; cref != NULL; cref = cref->next) {
			/* Get machine code which is patched back in later. A             */
			/* `call rel32' is 5 bytes long.                                  */
			xcodeptr = cd->mcodebase + cref->branchpos;
			xmcode = *xcodeptr;
			mcode = *((u4 *) (xcodeptr + 1));

			MCODECHECK(50);

			/* patch in `call rel32' to call the following code               */
			tmpcd->mcodeptr = xcodeptr;     /* set dummy mcode pointer        */
			i386_call_imm(tmpcd, cd->mcodeptr - (xcodeptr + 5));

			/* Save current stack pointer into a temporary register.          */
			i386_mov_reg_reg(cd, REG_SP, REG_ITMP1);

			/* Push machine code bytes to patch onto the stack.               */
			i386_push_imm(cd, (u4) xmcode);
			i386_push_imm(cd, (u4) mcode);

			i386_push_imm(cd, (u4) cref->class);

			/* Push previously saved stack pointer onto stack.                */
			i386_push_reg(cd, REG_ITMP1);

			i386_mov_imm_reg(cd, (u4) asm_check_clinit, REG_ITMP1);
			i386_jmp_reg(cd, REG_ITMP1);
		}
	}
	}
	
	codegen_finish(m, cd, (u4) (cd->mcodeptr - cd->mcodebase));
}


/* function createcompilerstub *************************************************

   creates a stub routine which calls the compiler
	
*******************************************************************************/

#define COMPSTUBSIZE 12

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
    i386_mov_imm_reg(cd, (u4) m, REG_ITMP1);/* pass method pointer to compiler*/

	/* we use REG_ITMP3 cause ECX (REG_ITMP2) is used for patching  */
    i386_mov_imm_reg(cd, (u4) asm_call_jit_compiler, REG_ITMP3);
    i386_jmp_reg(cd, REG_ITMP3);        /* jump to compiler                   */

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

#define NATIVESTUBSIZE    370 + 36


#if defined(USE_THREADS) && defined(NATIVE_THREADS)
static java_objectheader **(*callgetexceptionptrptr)() = builtin_get_exceptionptrptr;
#endif

void i386_native_stub_debug(void **p) {
	printf("Pos on stack: %p\n",p);
	printf("Return adress should be: %p\n",*p);
}

void i386_native_stub_debug2(void **p) {
	printf("Pos on stack: %p\n",p);
	printf("Return for lookup is: %p\n",*p);
}

void traverseStackInfo() {
	void **p=builtin_asm_get_stackframeinfo();
	
	while ((*p)!=0) {
		methodinfo *m;
		printf("base addr:%p, methodinfo:%p\n",*p,(methodinfo*)((*p)+8));
		m=*((methodinfo**)((*p)+8));
		utf_display(m->name);
		printf("\n");
		p=*p;
	}
	

}


u1 *createnativestub(functionptr f, methodinfo *m)
{
    u1 *s = CNEW(u1, NATIVESTUBSIZE);   /* memory to hold the stub            */
	codegendata *cd;
	registerdata *rd;
	t_inlining_globals *id;
	s4 dumpsize;

    int addmethod=0;
    u1 *tptr;
    int i;
    int stackframesize = 4+16;           /* initial 4 bytes is space for jni env,
					 	+ 4 byte thread pointer + 4 byte previous pointer + method info + 4 offset native*/
    int stackframeoffset = 4;

    int p, t;

    void**  callAddrPatchPos=0;
    u1* jmpInstrPos=0;
    void** jmpInstrPatchPos=0;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* allocate required dump memory */

	cd = DNEW(codegendata);
	rd = DNEW(registerdata);
	id = DNEW(t_inlining_globals);

	/* setup registers before using it */

	inlining_setup(m, id);
	reg_setup(m, rd, id);

	/* set some required varibles which are normally set by codegen_setup */
	cd->mcodebase = s;
	cd->mcodeptr = s;
	cd->clinitrefs = NULL;

	if (m->flags & ACC_STATIC) {
		stackframesize += 4;
		stackframeoffset += 4;
	}

    descriptor2types(m);                     /* set paramcount and paramtypes */
  
/*DEBUG*/
/* 	i386_push_reg(cd, REG_SP);
        i386_mov_imm_reg(cd, (s4) i386_native_stub_debug, REG_ITMP1);
        i386_call_reg(cd, REG_ITMP1);
 	i386_pop_reg(cd, REG_ITMP1);*/


	/* if function is static, check for initialized */

	if (m->flags & ACC_STATIC) {
		/* if class isn't yet initialized, do it */
		if (!m->class->initialized) {
			s4 *header = (s4 *) s;
			*header = 0;/*extablesize*/
			header++;
			*header = 0;/*line number table start*/
			header++;
			*header = 0;/*line number table size*/
			header++;
			*header = 0;/*fltsave*/
			header++;
			*header = 0;/*intsave*/
			header++;
			*header = 0;/*isleaf*/
			header++;
			*header = 0;/*issync*/
			header++;
			*header = 0;/*framesize*/
			header++;
			*header = (u4) m;/*methodpointer*/
			header++;

			s = (u1 *) header;

			cd->mcodebase = s;
			cd->mcodeptr = s;
			addmethod = 1;

			codegen_addclinitref(cd, cd->mcodeptr, m->class);
		}
	}

    if (runverbose) {
        i386_alu_imm_reg(cd, I386_SUB, TRACE_ARGS_NUM * 8 + 4, REG_SP);
        
        for (p = 0; p < m->paramcount && p < TRACE_ARGS_NUM; p++) {
            t = m->paramtypes[p];
            if (IS_INT_LNG_TYPE(t)) {
                if (IS_2_WORD_TYPE(t)) {
                    i386_mov_membase_reg(cd, REG_SP, 4 + (TRACE_ARGS_NUM + p) * 8 + 4, REG_ITMP1);
                    i386_mov_membase_reg(cd, REG_SP, 4 + (TRACE_ARGS_NUM + p) * 8 + 4 + 4, REG_ITMP2);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, p * 8);
					i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, p * 8 + 4);

                } else if (t == TYPE_ADR) {
                    i386_mov_membase_reg(cd, REG_SP, 4 + (TRACE_ARGS_NUM + p) * 8 + 4, REG_ITMP1);
                    i386_alu_reg_reg(cd, I386_XOR, REG_ITMP2, REG_ITMP2);
					i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, p * 8);
					i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, p * 8 + 4);

                } else {
                    i386_mov_membase_reg(cd, REG_SP, 4 + (TRACE_ARGS_NUM + p) * 8 + 4, EAX);
                    i386_cltd(cd);
					i386_mov_reg_membase(cd, EAX, REG_SP, p * 8);
					i386_mov_reg_membase(cd, EDX, REG_SP, p * 8 + 4);
                }

            } else {
                if (!IS_2_WORD_TYPE(t)) {
                    i386_flds_membase(cd, REG_SP, 4 + (TRACE_ARGS_NUM + p) * 8 + 4);
                    i386_fstps_membase(cd, REG_SP, p * 8);
                    i386_alu_reg_reg(cd, I386_XOR, REG_ITMP2, REG_ITMP2);
                    i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, p * 8 + 4);

                } else {
                    i386_fldl_membase(cd, REG_SP, 4 + (TRACE_ARGS_NUM + p) * 8 + 4);
                    i386_fstpl_membase(cd, REG_SP, p * 8);
                }
            }
        }
		
        i386_alu_reg_reg(cd, I386_XOR, REG_ITMP1, REG_ITMP1);
        for (p = m->paramcount; p < TRACE_ARGS_NUM; p++) {
            i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, p * 8);
            i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, p * 8 + 4);
        }

        i386_mov_imm_membase(cd, (s4) m, REG_SP, TRACE_ARGS_NUM * 8);

        i386_mov_imm_reg(cd, (s4) builtin_trace_args, REG_ITMP1);
        i386_call_reg(cd, REG_ITMP1);

        i386_alu_imm_reg(cd, I386_ADD, TRACE_ARGS_NUM * 8 + 4, REG_SP);
    }

    /*
	 * mark the whole fpu stack as free for native functions
	 * (only for saved register count == 0)
	 */
    i386_ffree_reg(cd, 0);
    i386_ffree_reg(cd, 1);
    i386_ffree_reg(cd, 2);
    i386_ffree_reg(cd, 3);
    i386_ffree_reg(cd, 4);
    i386_ffree_reg(cd, 5);
    i386_ffree_reg(cd, 6);
    i386_ffree_reg(cd, 7);

	/* calculate stackframe size for native function */
    tptr = m->paramtypes;
    for (i = 0; i < m->paramcount; i++) {
        switch (*tptr++) {
        case TYPE_INT:
        case TYPE_FLT:
        case TYPE_ADR:
            stackframesize += 4;
            break;

        case TYPE_LNG:
        case TYPE_DBL:
            stackframesize += 8;
            break;

        default:
            panic("unknown parameter type in native function");
        }
    }

	i386_alu_imm_reg(cd, I386_SUB, stackframesize, REG_SP);

/* CREATE DYNAMIC STACK INFO -- BEGIN*/
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

/*TESTING ONLY */
/*   i386_mov_imm_membase(cd, (s4) m, REG_SP,stackframesize-4);
   i386_mov_imm_membase(cd, (s4) m, REG_SP,stackframesize-8);
   i386_mov_imm_membase(cd, (s4) m, REG_SP,stackframesize-12);*/

/* CREATE DYNAMIC STACK INFO -- END*/

/* RESOLVE NATIVE METHOD -- BEGIN*/
#ifndef STATIC_CLASSPATH
   if (f==0) {
     /*log_text("Dynamic classpath: preparing for delayed native function resolving");*/
     i386_jmp_imm(cd,0);
     jmpInstrPos=cd->mcodeptr-4;
     /*patchposition*/
     i386_mov_imm_reg(cd,jmpInstrPos,REG_ITMP1);
     i386_push_reg(cd,REG_ITMP1);
     /*jmp offset*/
     i386_mov_imm_reg(cd,0,REG_ITMP1);
     jmpInstrPatchPos=cd->mcodeptr-4;
     i386_push_reg(cd,REG_ITMP1);
     /*position of call address to patch*/
     i386_mov_imm_reg(cd,0,REG_ITMP1);
     callAddrPatchPos=(cd->mcodeptr-4);
     i386_push_reg(cd,REG_ITMP1);
     /*method info structure*/
     i386_mov_imm_reg(cd,(s4) m, REG_ITMP1);
     i386_push_reg(cd,REG_ITMP1);
     /*call resolve functions*/
     i386_mov_imm_reg(cd, (s4)codegen_resolve_native,REG_ITMP1);
     i386_call_reg(cd,REG_ITMP1);
     /*cleanup*/
     i386_pop_reg(cd,REG_ITMP1);
     i386_pop_reg(cd,REG_ITMP1);
     i386_pop_reg(cd,REG_ITMP1);
     i386_pop_reg(cd,REG_ITMP1);
     /*fix jmp offset replacement*/
     (*jmpInstrPatchPos)=cd->mcodeptr-jmpInstrPos-4;
   } /*else log_text("Dynamic classpath: immediate native function resolution possible");*/
#endif
/* RESOLVE NATIVE METHOD -- END*/



    tptr = m->paramtypes;
    for (i = 0; i < m->paramcount; i++) {
        switch (*tptr++) {
        case TYPE_INT:
        case TYPE_FLT:
        case TYPE_ADR:
            i386_mov_membase_reg(cd, REG_SP, stackframesize + (1 * 4) + i * 8, REG_ITMP1);
            i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, stackframeoffset);
            stackframeoffset += 4;
            break;

        case TYPE_LNG:
        case TYPE_DBL:
            i386_mov_membase_reg(cd, REG_SP, stackframesize + (1 * 4) + i * 8, REG_ITMP1);
            i386_mov_membase_reg(cd, REG_SP, stackframesize + (1 * 4) + i * 8 + 4, REG_ITMP2);
            i386_mov_reg_membase(cd, REG_ITMP1, REG_SP, stackframeoffset);
            i386_mov_reg_membase(cd, REG_ITMP2, REG_SP, stackframeoffset + 4);
            stackframeoffset += 8;
            break;

        default:
            panic("unknown parameter type in native function");
        }
    }

	if (m->flags & ACC_STATIC) {
		/* put class into second argument */
		i386_mov_imm_membase(cd, (s4) m->class, REG_SP, 4);
	}

	/* put env into first argument */
	i386_mov_imm_membase(cd, (s4) &env, REG_SP, 0);

    i386_mov_imm_reg(cd, (s4) f, REG_ITMP1);
#ifndef STATIC_CLASSPATH
    if (f==0)
      (*callAddrPatchPos)=(cd->mcodeptr-4);
#endif
    i386_call_reg(cd, REG_ITMP1);

/*REMOVE DYNAMIC STACK INFO -BEGIN */
    i386_push_reg(cd, REG_RESULT2);
    i386_mov_membase_reg(cd, REG_SP,stackframesize-12,REG_ITMP2); /*old value*/
    i386_mov_membase_reg(cd, REG_SP,stackframesize-8,REG_RESULT2); /*pointer*/
    i386_mov_reg_membase(cd, REG_ITMP2,REG_RESULT2,0);
    i386_pop_reg(cd, REG_RESULT2);
/*REMOVE DYNAMIC STACK INFO -END */

    i386_alu_imm_reg(cd, I386_ADD, stackframesize, REG_SP);


    if (runverbose) {
        i386_alu_imm_reg(cd, I386_SUB, 4 + 8 + 8 + 4, REG_SP);
		
        i386_mov_imm_membase(cd, (u4) m, REG_SP, 0);
		
        i386_mov_reg_membase(cd, REG_RESULT, REG_SP, 4);
        i386_mov_reg_membase(cd, REG_RESULT2, REG_SP, 4 + 4);
		
        i386_fstl_membase(cd, REG_SP, 4 + 8);
        i386_fsts_membase(cd, REG_SP, 4 + 8 + 8);

        i386_mov_imm_reg(cd, (u4) builtin_displaymethodstop, REG_ITMP1);
        i386_call_reg(cd, REG_ITMP1);
		
        i386_mov_membase_reg(cd, REG_SP, 4, REG_RESULT);
        i386_mov_membase_reg(cd, REG_SP, 4 + 4, REG_RESULT2);
		
        i386_alu_imm_reg(cd, I386_ADD, 4 + 8 + 8 + 4, REG_SP);
    }

	/* we can't use REG_ITMP3 == REG_RESULT2 */
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	i386_push_reg(cd, REG_RESULT);
	i386_push_reg(cd, REG_RESULT2);
	i386_call_mem(cd, (s4) &callgetexceptionptrptr);
	i386_mov_membase_reg(cd, REG_RESULT, 0, REG_ITMP2);
	i386_test_reg_reg(cd, REG_ITMP2, REG_ITMP2);
	i386_pop_reg(cd, REG_RESULT2);
	i386_pop_reg(cd, REG_RESULT);
#else
	i386_mov_imm_reg(cd, (s4) &_exceptionptr, REG_ITMP2);
	i386_mov_membase_reg(cd, REG_ITMP2, 0, REG_ITMP2);
	i386_test_reg_reg(cd, REG_ITMP2, REG_ITMP2);
#endif
	i386_jcc(cd, I386_CC_NE, 1);

	i386_ret(cd);

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	i386_push_reg(cd, REG_ITMP2);
	i386_call_mem(cd, (s4) &callgetexceptionptrptr);
	i386_mov_imm_membase(cd, 0, REG_RESULT, 0);
	i386_pop_reg(cd, REG_ITMP1_XPTR);
#else
	i386_mov_reg_reg(cd, REG_ITMP2, REG_ITMP1_XPTR);
	i386_mov_imm_reg(cd, (s4) &_exceptionptr, REG_ITMP2);
	i386_mov_imm_membase(cd, 0, REG_ITMP2, 0);
#endif
	i386_mov_membase_reg(cd, REG_SP, 0, REG_ITMP2_XPC);
	i386_alu_imm_reg(cd, I386_SUB, 2, REG_ITMP2_XPC);

	i386_mov_imm_reg(cd, (s4) asm_handle_nat_exception, REG_ITMP3);
	i386_jmp_reg(cd, REG_ITMP3);

	if (addmethod) {
		codegen_insertmethod(s, cd->mcodeptr);
	}

	{
		u1          *xcodeptr;
		clinitref   *cref;
		codegendata *tmpcd;
		u1           xmcode;
		u4           mcode;

		tmpcd = DNEW(codegendata);

		/* there can only be one clinit ref entry                             */
		cref = cd->clinitrefs;

		if (cref) {
			/* Get machine code which is patched back in later. A             */
			/* `call rel32' is 5 bytes long.                                  */
			xcodeptr = cd->mcodebase + cref->branchpos;
			xmcode = *xcodeptr;
			mcode =  *((u4 *) (xcodeptr + 1));

			/* patch in `call rel32' to call the following code               */
			tmpcd->mcodeptr = xcodeptr;     /* set dummy mcode pointer        */
			i386_call_imm(tmpcd, cd->mcodeptr - (xcodeptr + 5));

			/* Save current stack pointer into a temporary register.          */
			i386_mov_reg_reg(cd, REG_SP, REG_ITMP1);

			/* Push machine code bytes to patch onto the stack.               */
			i386_push_imm(cd, (u4) xmcode);
			i386_push_imm(cd, (u4) mcode);

			i386_push_imm(cd, (u4) cref->class);

			/* Push previously saved stack pointer onto stack.                */
			i386_push_reg(cd, REG_ITMP1);

			i386_mov_imm_reg(cd, (u4) asm_check_clinit, REG_ITMP1);
			i386_jmp_reg(cd, REG_ITMP1);
		}
	}

#if 0
	dolog_plain("native stubentry: %p, stubsize: %d (for %d params) --", (s4)s,(s4) (cd->mcodeptr - s), m->paramcount);
	utf_display(m->name);
	dolog_plain("\n");
#endif

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
