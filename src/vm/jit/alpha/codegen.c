/* src/vm/jit/alpha/codegen.c - machine code generator for Alpha

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
            Reinhard Grafl

   Changes: Joseph Wenninger
            Christian Thalinger

   $Id: codegen.c 2458 2005-05-12 23:02:07Z twisti $

*/


#include <stdio.h>
#include <signal.h>

#include "config.h"
#include "cacao/cacao.h"
#include "native/native.h"
#include "vm/builtin.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/stringlocal.h"
#include "vm/tables.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/jit.h"
#ifdef LSRA
#include "vm/jit/lsra.h"
#endif
#include "vm/jit/parse.h"
#include "vm/jit/patcher.h"
#include "vm/jit/reg.h"
#include "vm/jit/alpha/arch.h"
#include "vm/jit/alpha/codegen.h"
#include "vm/jit/alpha/types.h"
#include "vm/jit/alpha/asmoffsets.h"


/* *****************************************************************************

Datatypes and Register Allocations:
----------------------------------- 

On 64-bit-machines (like the Alpha) all operands are stored in the
registers in a 64-bit form, even when the correspondig JavaVM  operands
only need 32 bits. This is done by a canonical representation:

32-bit integers are allways stored as sign-extended 64-bit values (this
approach is directly supported by the Alpha architecture and is very easy
to implement).

32-bit-floats are stored in a 64-bit doubleprecision register by simply
expanding the exponent and mantissa with zeroes. (also supported by the
architecture)


Stackframes:

The calling conventions and the layout of the stack is  explained in detail
in the documention file: calling.doc

*******************************************************************************/


/* register descripton - array ************************************************/

/* #define REG_RES   0         reserved register for OS or code generator     */
/* #define REG_RET   1         return value register                          */
/* #define REG_EXC   2         exception value register (only old jit)        */
/* #define REG_SAV   3         (callee) saved register                        */
/* #define REG_TMP   4         scratch temporary register (caller saved)      */
/* #define REG_ARG   5         argument register (caller saved)               */

/* #define REG_END   -1        last entry in tables */
 
int nregdescint[] = {
	REG_RET, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, 
	REG_TMP, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, 
	REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_TMP, REG_TMP,
	REG_TMP, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES,
	REG_END };

/* for use of reserved registers, see comment above */
	
int nregdescfloat[] = {
	REG_RET, REG_TMP, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV,
	REG_SAV, REG_SAV, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, 
	REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_TMP, REG_TMP,
	REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_RES, REG_RES, REG_RES, REG_RES,
	REG_END };


/* Include independent code generation stuff -- include after register        */
/* descriptions to avoid extern definitions.                                  */

#include "vm/jit/codegen.inc"
#include "vm/jit/reg.inc"
#ifdef LSRA
#include "vm/jit/lsra.inc"
#endif


/* NullPointerException handlers and exception handling initialisation        */

typedef struct sigctx_struct {
	long          sc_onstack;           /* sigstack state to restore          */
	long          sc_mask;              /* signal mask to restore             */
	long          sc_pc;                /* pc at time of signal               */
	long          sc_ps;                /* psl to retore                      */
	long          sc_regs[32];          /* processor regs 0 to 31             */
	long          sc_ownedfp;           /* fp has been used                   */
	long          sc_fpregs[32];        /* fp regs 0 to 31                    */
	unsigned long sc_fpcr;              /* floating point control register    */
	unsigned long sc_fp_control;        /* software fpcr                      */
	                                    /* rest is unused                     */
	unsigned long sc_reserved1, sc_reserved2;
	unsigned long sc_ssize;
	char          *sc_sbase;
	unsigned long sc_traparg_a0;
	unsigned long sc_traparg_a1;
	unsigned long sc_traparg_a2;
	unsigned long sc_fp_trap_pc;
	unsigned long sc_fp_trigger_sum;
	unsigned long sc_fp_trigger_inst;
	unsigned long sc_retcode[2];
} sigctx_struct;


#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void thread_restartcriticalsection(ucontext_t *uc)
{
	void *critical;
	if ((critical = thread_checkcritical((void*) uc->uc_mcontext.sc_pc)) != NULL)
		uc->uc_mcontext.sc_pc = (u8) critical;
}
#endif


/* NullPointerException signal handler for hardware null pointer check */

void catch_NullPointerException(int sig, siginfo_t *siginfo, void *_p)
{
	struct sigaction act;
	sigset_t         nsig;
	int              instr;
	long             faultaddr;

	ucontext_t *_uc = (ucontext_t *) _p;
	mcontext_t *sigctx = &_uc->uc_mcontext;

	instr = *((s4 *) (sigctx->sc_pc));
	faultaddr = sigctx->sc_regs[(instr >> 16) & 0x1f];

	if (faultaddr == 0) {
		/* Reset signal handler - necessary for SysV, does no harm for BSD */
		act.sa_sigaction = catch_NullPointerException;
		act.sa_flags = SA_SIGINFO;
		sigaction(sig, &act, NULL);

		sigemptyset(&nsig);
		sigaddset(&nsig, sig);
		sigprocmask(SIG_UNBLOCK, &nsig, NULL);           /* unblock signal    */

		sigctx->sc_regs[REG_ITMP1_XPTR] = (u8) string_java_lang_NullPointerException;
		sigctx->sc_regs[REG_ITMP2_XPC] = sigctx->sc_pc;
		sigctx->sc_pc = (u8) asm_throw_and_handle_exception;
		return;

	} else {
		faultaddr += (long) ((instr << 16) >> 16);
		fprintf(stderr, "faulting address: 0x%016lx\n", faultaddr);
		panic("Stack overflow");
	}
}


#ifdef __osf__

void init_exceptions(void)
{

#else /* Linux */

/* Linux on Digital Alpha needs an initialisation of the ieee floating point
	control for IEEE compliant arithmetic (option -mieee of GCC). Under
	Digital Unix this is done automatically.
*/

#include <asm/fpu.h>

extern unsigned long ieee_get_fp_control();
extern void ieee_set_fp_control(unsigned long fp_control);

void init_exceptions(void)
{
	struct sigaction act;

	/* initialize floating point control */

	ieee_set_fp_control(ieee_get_fp_control()
						& ~IEEE_TRAP_ENABLE_INV
						& ~IEEE_TRAP_ENABLE_DZE
/*  						& ~IEEE_TRAP_ENABLE_UNF   we dont want underflow */
						& ~IEEE_TRAP_ENABLE_OVF);
#endif

	/* install signal handlers we need to convert to exceptions */

	if (!checknull) {
		act.sa_sigaction = catch_NullPointerException;
		act.sa_flags = SA_SIGINFO;

#if defined(SIGSEGV)
		sigaction(SIGSEGV, &act, NULL);
#endif

#if defined(SIGBUS)
		sigaction(SIGBUS, &act, NULL);
#endif
	}
}


/* function gen_mcode **********************************************************

	generates machine code

*******************************************************************************/

void codegen(methodinfo *m, codegendata *cd, registerdata *rd)
{
	s4     len, s1, s2, s3, d;
	ptrint a;
	s4 parentargs_base;
	s4             *mcodeptr;
	stackptr        src;
	varinfo        *var;
	basicblock     *bptr;
	instruction    *iptr;
	exceptiontable *ex;
	u2 currentline=0;
	{
	s4 i, p, pa, t, l;
	s4 savedregs_num;

	savedregs_num = (m->isleafmethod) ? 0 : 1;        /* space to save the RA */

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
		(void) dseg_adds4(cd, (rd->maxmemuse + 1) * 8);     /* IsSync         */
	else

#endif

	(void) dseg_adds4(cd, 0);                               /* IsSync         */
	                                       
	(void) dseg_adds4(cd, m->isleafmethod);                 /* IsLeaf         */
	(void) dseg_adds4(cd, rd->savintregcnt - rd->maxsavintreguse);/* IntSave  */
	(void) dseg_adds4(cd, rd->savfltregcnt - rd->maxsavfltreguse);/* FltSave  */

	dseg_addlinenumbertablesize(cd);

	(void) dseg_adds4(cd, cd->exceptiontablelength);        /* ExTableSize    */

	/* create exception table */

	for (ex = cd->exceptiontable; ex != NULL; ex = ex->down) {
		dseg_addtarget(cd, ex->start);
   		dseg_addtarget(cd, ex->end);
		dseg_addtarget(cd, ex->handler);
		(void) dseg_addaddress(cd, ex->catchtype.cls);
	}
	
	/* initialize mcode variables */
	
	mcodeptr = (s4 *) cd->mcodebase;
	cd->mcodeend = (s4 *) (cd->mcodebase + cd->mcodesize);
	MCODECHECK(128 + m->paramcount);

	/* create stack frame (if necessary) */

	if (parentargs_base) {
		M_LDA(REG_SP, REG_SP, -parentargs_base * 8);
	}

	/* save return address and used callee saved registers */

	p = parentargs_base;
	if (!m->isleafmethod) {
		p--; M_AST(REG_RA, REG_SP, p * 8);
	}
	for (i = rd->savintregcnt - 1; i >= rd->maxsavintreguse; i--) {
		p--; M_LST(rd->savintregs[i], REG_SP, p * 8);
	}
	for (i = rd->savfltregcnt - 1; i >= rd->maxsavfltreguse; i--) {
		p--; M_DST(rd->savfltregs[i], REG_SP, p * 8);
	}

	/* copy argument registers to stack and call trace function with pointer
	   to arguments on stack.
	*/

	if (runverbose) {
		s4 disp;
		M_LDA(REG_SP, REG_SP, -((INT_ARG_CNT + FLT_ARG_CNT + 2) * 8));
		M_AST(REG_RA, REG_SP, 1 * 8);

		/* save integer argument registers */
		for (p = 0; p < m->paramcount && p < INT_ARG_CNT; p++) {
			M_LST(rd->argintregs[p], REG_SP,  (2 + p) * 8);
		}

		/* save and copy float arguments into integer registers */
		for (p = 0; p < m->paramcount && p < FLT_ARG_CNT; p++) {
			t = m->paramtypes[p];

			if (IS_FLT_DBL_TYPE(t)) {
				if (IS_2_WORD_TYPE(t)) {
					M_DST(rd->argfltregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);

				} else {
					M_FST(rd->argfltregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);
				}

				M_LLD(rd->argintregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);
				
			} else {
				M_DST(rd->argfltregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);
			}
		}

		p = dseg_addaddress(cd, m);
		M_ALD(REG_ITMP1, REG_PV, p);
		M_AST(REG_ITMP1, REG_SP, 0 * 8);
		p = dseg_addaddress(cd, (void *) builtin_trace_args);
		M_ALD(REG_PV, REG_PV, p);
		M_JSR(REG_RA, REG_PV);
		disp = -(s4) ((u1 *) mcodeptr - cd->mcodebase);
		M_LDA(REG_PV, REG_RA, disp);
		M_ALD(REG_RA, REG_SP, 1 * 8);

		for (p = 0; p < m->paramcount && p < INT_ARG_CNT; p++) {
			M_LLD(rd->argintregs[p], REG_SP,  (2 + p) * 8);
		}

		for (p = 0; p < m->paramcount && p < FLT_ARG_CNT; p++) {
			t = m->paramtypes[p];

			if (IS_FLT_DBL_TYPE(t)) {
				if (IS_2_WORD_TYPE(t)) {
					M_DLD(rd->argfltregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);

				} else {
					M_FLD(rd->argfltregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);
				}

			} else {
				M_DLD(rd->argfltregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);
			}
		}

		M_LDA(REG_SP, REG_SP, (INT_ARG_CNT + FLT_ARG_CNT + 2) * 8);
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
 			if (p < INT_ARG_CNT) {                   /* register arguments    */
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
 					M_INTMOVE(rd->argintregs[p], var->regoff);
				} else {                             /* reg arg -> spilled    */
 					M_LST(rd->argintregs[p], REG_SP, 8 * var->regoff);
 				}

			} else {                                 /* stack arguments       */
 				pa = p - INT_ARG_CNT;
 				if (!(var->flags & INMEMORY)) {      /* stack arg -> register */
 					M_LLD(var->regoff, REG_SP, 8 * (parentargs_base + pa));

 				} else {                             /* stack arg -> spilled  */
/*  					M_LLD(REG_ITMP1, REG_SP, 8 * (parentargs_base + pa)); */
/*  					M_LST(REG_ITMP1, REG_SP, 8 * var->regoff); */
					var->regoff = parentargs_base + pa;
				}
			}

		} else {                                     /* floating args         */
 			if (p < FLT_ARG_CNT) {                   /* register arguments    */
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
 					M_FLTMOVE(rd->argfltregs[p], var->regoff);

 				} else {			                 /* reg arg -> spilled    */
 					M_DST(rd->argfltregs[p], REG_SP, 8 * var->regoff);
 				}

			} else {                                 /* stack arguments       */
 				pa = p - FLT_ARG_CNT;
 				if (!(var->flags & INMEMORY)) {      /* stack-arg -> register */
 					M_DLD(var->regoff, REG_SP, 8 * (parentargs_base + pa) );

 				} else {                             /* stack-arg -> spilled  */
/*  					M_DLD(REG_FTMP1, REG_SP, 8 * (parentargs_base + pa)); */
/*  					M_DST(REG_FTMP1, REG_SP, 8 * var->regoff); */
					var->regoff = parentargs_base + pa;
				}
			}
		}
	} /* end for */

	/* call monitorenter function */

#if defined(USE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		s4 disp;

		if (m->flags & ACC_STATIC) {
			p = dseg_addaddress(cd, m->class);
			M_ALD(REG_ITMP1, REG_PV, p);
			M_AST(REG_ITMP1, REG_SP, rd->maxmemuse * 8);
			M_INTMOVE(REG_ITMP1, rd->argintregs[0]);
			p = dseg_addaddress(cd, BUILTIN_staticmonitorenter);
			M_ALD(REG_PV, REG_PV, p);
			M_JSR(REG_RA, REG_PV);
			disp = -(s4) ((u1 *) mcodeptr - cd->mcodebase);
			M_LDA(REG_PV, REG_RA, disp);

		} else {
			M_BEQZ(rd->argintregs[0], 0);
			codegen_addxnullrefs(cd, mcodeptr);
			M_AST(rd->argintregs[0], REG_SP, rd->maxmemuse * 8);
			p = dseg_addaddress(cd, BUILTIN_monitorenter);
			M_ALD(REG_PV, REG_PV, p);
			M_JSR(REG_RA, REG_PV);
			disp = -(s4) ((u1 *) mcodeptr - cd->mcodebase);
			M_LDA(REG_PV, REG_RA, disp);
		}
	}			
#endif

	}

	/* end of header generation */

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

		/* copy interface registers to their destination */

		src = bptr->instack;
		len = bptr->indepth;
		MCODECHECK(64+len);
#ifdef LSRA
		if (opt_lsra) {
		while (src != NULL) {
			len--;
			if ((len == 0) && (bptr->type != BBTYPE_STD)) {
					/* 				d = reg_of_var(m, src, REG_ITMP1); */
					if (!(src->flags & INMEMORY))
						d= src->regoff;
					else
						d=REG_ITMP1;
					M_INTMOVE(REG_ITMP1, d);
					store_reg_to_var_int(src, d);
				}
				src = src->prev;
			}
		} else {
#endif
			while (src != NULL) {
				len--;
				if ((len == 0) && (bptr->type != BBTYPE_STD)) {
				d = reg_of_var(rd, src, REG_ITMP1);
				M_INTMOVE(REG_ITMP1, d);
				store_reg_to_var_int(src, d);
				}
			else {
				d = reg_of_var(rd, src, REG_IFTMP);
				if ((src->varkind != STACKVAR)) {
					s2 = src->type;
					if (IS_FLT_DBL_TYPE(s2)) {
						if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
							s1 = rd->interfaces[len][s2].regoff;
							M_FLTMOVE(s1,d);
							}
						else {
							M_DLD(d, REG_SP, 8 * rd->interfaces[len][s2].regoff);
							}
						store_reg_to_var_flt(src, d);
						}
					else {
						if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
							s1 = rd->interfaces[len][s2].regoff;
							M_INTMOVE(s1,d);
							}
						else {
							M_LLD(d, REG_SP, 8 * rd->interfaces[len][s2].regoff);
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

		for (iptr = bptr->iinstr; len > 0; src = iptr->dst, len--, iptr++) {
			if (iptr->line != currentline) {
				dseg_addlinenumber(cd, iptr->line, mcodeptr);
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

			var_to_reg_int(s1, src, REG_ITMP1);
			M_BEQZ(s1, 0);
			codegen_addxnullrefs(cd, mcodeptr);
			break;

		/* constant operations ************************************************/

		case ICMD_ICONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.i = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			ICONST(d, iptr->val.i);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.l = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			LCONST(d, iptr->val.l);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.f = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			a = dseg_addfloat(cd, iptr->val.f);
			M_FLD(d, REG_PV, a);
			store_reg_to_var_flt(iptr->dst, d);
			break;
			
		case ICMD_DCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.d = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			a = dseg_adddouble(cd, iptr->val.d);
			M_DLD(d, REG_PV, a);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_ACONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.a = constant                    */

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if (iptr->val.a) {
				a = dseg_addaddress(cd, iptr->val.a);
				M_ALD(d, REG_PV, a);
			} else {
				M_INTMOVE(REG_ZERO, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;


		/* load/store operations **********************************************/

		case ICMD_ILOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_LLOAD:      /* op1 = local variable                         */
		case ICMD_ALOAD:

			d = reg_of_var(rd, iptr->dst, REG_ITMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY)
				M_LLD(d, REG_SP, 8 * var->regoff);
			else
				{M_INTMOVE(var->regoff,d);}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FLOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_DLOAD:      /* op1 = local variable                         */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY)
				M_DLD(d, REG_SP, 8 * var->regoff);
			else
				{M_FLTMOVE(var->regoff,d);}
			store_reg_to_var_flt(iptr->dst, d);
			break;


		case ICMD_ISTORE:     /* ..., value  ==> ...                          */
		case ICMD_LSTORE:     /* op1 = local variable                         */
		case ICMD_ASTORE:

			if ((src->varkind == LOCALVAR) &&
			    (src->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				var_to_reg_int(s1, src, REG_ITMP1);
				M_LST(s1, REG_SP, 8 * var->regoff);
				}
			else {
				var_to_reg_int(s1, src, var->regoff);
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
				var_to_reg_flt(s1, src, REG_FTMP1);
				M_DST(s1, REG_SP, 8 * var->regoff);
				}
			else {
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

			var_to_reg_int(s1, src, REG_ITMP1); 
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_ISUB(REG_ZERO, s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_LSUB(REG_ZERO, s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_I2L:        /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_IADD(s1, REG_ZERO, d );
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (has_ext_instr_set) {
				M_BSEXT(s1, d);
				}
			else {
				M_SLL_IMM(s1, 56, d);
				M_SRA_IMM( d, 56, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2CHAR:   /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
            M_CZEXT(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2SHORT:  /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (has_ext_instr_set) {
				M_SSEXT(s1, d);
				}
			else {
				M_SLL_IMM(s1, 48, d);
				M_SRA_IMM( d, 48, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_IADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_IADD(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 255)) {
				M_IADD_IMM(s1, iptr->val.i, d);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_IADD(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_LADD(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.l >= 0) && (iptr->val.l <= 255)) {
				M_LADD_IMM(s1, iptr->val.l, d);
				}
			else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_LADD(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_ISUB(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 255)) {
				M_ISUB_IMM(s1, iptr->val.i, d);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_ISUB(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_LSUB(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.l >= 0) && (iptr->val.l <= 255)) {
				M_LSUB_IMM(s1, iptr->val.l, d);
				}
			else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_LSUB(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_IMUL(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 255)) {
				M_IMUL_IMM(s1, iptr->val.i, d);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_IMUL(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_LMUL (s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.l >= 0) && (iptr->val.l <= 255)) {
				M_LMUL_IMM(s1, iptr->val.l, d);
				}
			else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_LMUL(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IDIVPOW2:   /* ..., value  ==> ..., value << constant       */
		case ICMD_LDIVPOW2:   /* val.i = constant                             */
		                      
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (iptr->val.i <= 15) {
				M_LDA(REG_ITMP2, s1, (1 << iptr->val.i) -1);
				M_CMOVGE(s1, s1, REG_ITMP2);
				}
			else {
				M_SRA_IMM(s1, 63, REG_ITMP2);
				M_SRL_IMM(REG_ITMP2, 64 - iptr->val.i, REG_ITMP2);
				M_LADD(s1, REG_ITMP2, REG_ITMP2);
				}
			M_SRA_IMM(REG_ITMP2, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_AND_IMM(s2, 0x1f, REG_ITMP3);
			M_SLL(s1, REG_ITMP3, d);
			M_IADD(d, REG_ZERO, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_SLL_IMM(s1, iptr->val.i & 0x1f, d);
			M_IADD(d, REG_ZERO, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_AND_IMM(s2, 0x1f, REG_ITMP3);
			M_SRA(s1, REG_ITMP3, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_SRA_IMM(s1, iptr->val.i & 0x1f, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_AND_IMM(s2, 0x1f, REG_ITMP2);
            M_IZEXT(s1, d);
			M_SRL(d, REG_ITMP2, d);
			M_IADD(d, REG_ZERO, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
            M_IZEXT(s1, d);
			M_SRL_IMM(d, iptr->val.i & 0x1f, d);
			M_IADD(d, REG_ZERO, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_SLL(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_SLL_IMM(s1, iptr->val.i & 0x3f, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_SRA(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_SRA_IMM(s1, iptr->val.i & 0x3f, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_SRL(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_SRL_IMM(s1, iptr->val.i & 0x3f, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */
		case ICMD_LAND:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_AND(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 255)) {
				M_AND_IMM(s1, iptr->val.i, d);
				}
			else if (iptr->val.i == 0xffff) {
				M_CZEXT(s1, d);
				}
			else if (iptr->val.i == 0xffffff) {
				M_ZAPNOT_IMM(s1, 0x07, d);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_AND(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (s1 == d) {
				M_MOV(s1, REG_ITMP1);
				s1 = REG_ITMP1;
				}
			if ((iptr->val.i >= 0) && (iptr->val.i <= 255)) {
				M_AND_IMM(s1, iptr->val.i, d);
				M_BGEZ(s1, 3);
				M_ISUB(REG_ZERO, s1, d);
				M_AND_IMM(d, iptr->val.i, d);
				}
			else if (iptr->val.i == 0xffff) {
				M_CZEXT(s1, d);
				M_BGEZ(s1, 3);
				M_ISUB(REG_ZERO, s1, d);
				M_CZEXT(d, d);
				}
			else if (iptr->val.i == 0xffffff) {
				M_ZAPNOT_IMM(s1, 0x07, d);
				M_BGEZ(s1, 3);
				M_ISUB(REG_ZERO, s1, d);
				M_ZAPNOT_IMM(d, 0x07, d);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_AND(s1, REG_ITMP2, d);
				M_BGEZ(s1, 3);
				M_ISUB(REG_ZERO, s1, d);
				M_AND(d, REG_ITMP2, d);
				}
			M_ISUB(REG_ZERO, d, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.l >= 0) && (iptr->val.l <= 255)) {
				M_AND_IMM(s1, iptr->val.l, d);
				}
			else if (iptr->val.l == 0xffffL) {
				M_CZEXT(s1, d);
				}
			else if (iptr->val.l == 0xffffffL) {
				M_ZAPNOT_IMM(s1, 0x07, d);
				}
			else if (iptr->val.l == 0xffffffffL) {
				M_IZEXT(s1, d);
				}
			else if (iptr->val.l == 0xffffffffffL) {
				M_ZAPNOT_IMM(s1, 0x1f, d);
				}
			else if (iptr->val.l == 0xffffffffffffL) {
				M_ZAPNOT_IMM(s1, 0x3f, d);
				}
			else if (iptr->val.l == 0xffffffffffffffL) {
				M_ZAPNOT_IMM(s1, 0x7f, d);
				}
			else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_AND(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (s1 == d) {
				M_MOV(s1, REG_ITMP1);
				s1 = REG_ITMP1;
				}
			if ((iptr->val.l >= 0) && (iptr->val.l <= 255)) {
				M_AND_IMM(s1, iptr->val.l, d);
				M_BGEZ(s1, 3);
				M_LSUB(REG_ZERO, s1, d);
				M_AND_IMM(d, iptr->val.l, d);
				}
			else if (iptr->val.l == 0xffffL) {
				M_CZEXT(s1, d);
				M_BGEZ(s1, 3);
				M_LSUB(REG_ZERO, s1, d);
				M_CZEXT(d, d);
				}
			else if (iptr->val.l == 0xffffffL) {
				M_ZAPNOT_IMM(s1, 0x07, d);
				M_BGEZ(s1, 3);
				M_LSUB(REG_ZERO, s1, d);
				M_ZAPNOT_IMM(d, 0x07, d);
				}
			else if (iptr->val.l == 0xffffffffL) {
				M_IZEXT(s1, d);
				M_BGEZ(s1, 3);
				M_LSUB(REG_ZERO, s1, d);
				M_IZEXT(d, d);
				}
			else if (iptr->val.l == 0xffffffffffL) {
 				M_ZAPNOT_IMM(s1, 0x1f, d);
				M_BGEZ(s1, 3);
				M_LSUB(REG_ZERO, s1, d);
				M_ZAPNOT_IMM(d, 0x1f, d);
				}
			else if (iptr->val.l == 0xffffffffffffL) {
				M_ZAPNOT_IMM(s1, 0x3f, d);
				M_BGEZ(s1, 3);
				M_LSUB(REG_ZERO, s1, d);
				M_ZAPNOT_IMM(d, 0x3f, d);
				}
			else if (iptr->val.l == 0xffffffffffffffL) {
				M_ZAPNOT_IMM(s1, 0x7f, d);
				M_BGEZ(s1, 3);
				M_LSUB(REG_ZERO, s1, d);
				M_ZAPNOT_IMM(d, 0x7f, d);
				}
			else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_AND(s1, REG_ITMP2, d);
				M_BGEZ(s1, 3);
				M_LSUB(REG_ZERO, s1, d);
				M_AND(d, REG_ITMP2, d);
				}
			M_LSUB(REG_ZERO, d, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IOR:        /* ..., val1, val2  ==> ..., val1 | val2        */
		case ICMD_LOR:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_OR( s1,s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 255)) {
				M_OR_IMM(s1, iptr->val.i, d);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_OR(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.l >= 0) && (iptr->val.l <= 255)) {
				M_OR_IMM(s1, iptr->val.l, d);
				}
			else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_OR(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */
		case ICMD_LXOR:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_XOR(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 255)) {
				M_XOR_IMM(s1, iptr->val.i, d);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_XOR(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if ((iptr->val.l >= 0) && (iptr->val.l <= 255)) {
				M_XOR_IMM(s1, iptr->val.l, d);
				}
			else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_XOR(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_LCMP:       /* ..., val1, val2  ==> ..., val1 cmp val2      */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_CMPLT(s1, s2, REG_ITMP3);
			M_CMPLT(s2, s1, REG_ITMP1);
			M_LSUB (REG_ITMP1, REG_ITMP3, d);
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_IINC:       /* ..., value  ==> ..., value + constant        */
		                      /* op1 = variable, val.i = constant             */

			var = &(rd->locals[iptr->op1][TYPE_INT]);
			if (var->flags & INMEMORY) {
				s1 = REG_ITMP1;
				M_LLD(s1, REG_SP, 8 * var->regoff);
				}
			else
				s1 = var->regoff;
			if ((iptr->val.i >= 0) && (iptr->val.i <= 255)) {
				M_IADD_IMM(s1, iptr->val.i, s1);
				}
			else if ((iptr->val.i > -256) && (iptr->val.i < 0)) {
				M_ISUB_IMM(s1, (-iptr->val.i), s1);
				}
			else {
				M_LDA (s1, s1, iptr->val.i);
				M_IADD(s1, REG_ZERO, s1);
				}
			if (var->flags & INMEMORY)
				M_LST(s1, REG_SP, 8 * var->regoff);
			break;


		/* floating operations ************************************************/

		case ICMD_FNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_FMOVN(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_FMOVN(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (opt_noieee) {
				M_FADD(s1, s2, d);
				}
			else {
				if (d == s1 || d == s2) {
					M_FADDS(s1, s2, REG_FTMP3);
					M_TRAPB;
					M_FMOV(REG_FTMP3, d);
					}
				else {
					M_FADDS(s1, s2, d);
					M_TRAPB;
					}
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (opt_noieee) {
				M_DADD(s1, s2, d);
				}
			else {
				if (d == s1 || d == s2) {
					M_DADDS(s1, s2, REG_FTMP3);
					M_TRAPB;
					M_FMOV(REG_FTMP3, d);
					}
				else {
					M_DADDS(s1, s2, d);
					M_TRAPB;
					}
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (opt_noieee) {
				M_FSUB(s1, s2, d);
				}
			else {
				if (d == s1 || d == s2) {
					M_FSUBS(s1, s2, REG_FTMP3);
					M_TRAPB;
					M_FMOV(REG_FTMP3, d);
					}
				else {
					M_FSUBS(s1, s2, d);
					M_TRAPB;
					}
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (opt_noieee) {
				M_DSUB(s1, s2, d);
				}
			else {
				if (d == s1 || d == s2) {
					M_DSUBS(s1, s2, REG_FTMP3);
					M_TRAPB;
					M_FMOV(REG_FTMP3, d);
					}
				else {
					M_DSUBS(s1, s2, d);
					M_TRAPB;
					}
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (opt_noieee) {
				M_FMUL(s1, s2, d);
				}
			else {
				if (d == s1 || d == s2) {
					M_FMULS(s1, s2, REG_FTMP3);
					M_TRAPB;
					M_FMOV(REG_FTMP3, d);
					}
				else {
					M_FMULS(s1, s2, d);
					M_TRAPB;
					}
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DMUL:       /* ..., val1, val2  ==> ..., val1 *** val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (opt_noieee) {
				M_DMUL(s1, s2, d);
				}
			else {
				if (d == s1 || d == s2) {
					M_DMULS(s1, s2, REG_FTMP3);
					M_TRAPB;
					M_FMOV(REG_FTMP3, d);
					}
				else {
					M_DMULS(s1, s2, d);
					M_TRAPB;
					}
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (opt_noieee) {
				M_FDIV(s1, s2, d);
				}
			else {
				if (d == s1 || d == s2) {
					M_FDIVS(s1, s2, REG_FTMP3);
					M_TRAPB;
					M_FMOV(REG_FTMP3, d);
					}
				else {
					M_FDIVS(s1, s2, d);
					M_TRAPB;
					}
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (opt_noieee) {
				M_DDIV(s1, s2, d);
				}
			else {
				if (d == s1 || d == s2) {
					M_DDIVS(s1, s2, REG_FTMP3);
					M_TRAPB;
					M_FMOV(REG_FTMP3, d);
					}
				else {
					M_DDIVS(s1, s2, d);
					M_TRAPB;
					}
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;
		
		case ICMD_I2F:       /* ..., value  ==> ..., (float) value            */
		case ICMD_L2F:
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			a = dseg_adddouble(cd, 0.0);
			M_LST (s1, REG_PV, a);
			M_DLD (d, REG_PV, a);
			M_CVTLF(d, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_I2D:       /* ..., value  ==> ..., (double) value           */
		case ICMD_L2D:
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			a = dseg_adddouble(cd, 0.0);
			M_LST (s1, REG_PV, a);
			M_DLD (d, REG_PV, a);
			M_CVTLD(d, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
			
		case ICMD_F2I:       /* ..., value  ==> ..., (int) value              */
		case ICMD_D2I:
			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			a = dseg_adddouble(cd, 0.0);
			M_CVTDL_C(s1, REG_FTMP2);
			M_CVTLI(REG_FTMP2, REG_FTMP3);
			M_DST (REG_FTMP3, REG_PV, a);
			M_ILD (d, REG_PV, a);
			store_reg_to_var_int(iptr->dst, d);
			break;
		
		case ICMD_F2L:       /* ..., value  ==> ..., (long) value             */
		case ICMD_D2L:
			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			a = dseg_adddouble(cd, 0.0);
			M_CVTDL_C(s1, REG_FTMP2);
			M_DST (REG_FTMP2, REG_PV, a);
			M_LLD (d, REG_PV, a);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_F2D:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_CVTFDS(s1, d);
			M_TRAPB;
			store_reg_to_var_flt(iptr->dst, d);
			break;
					
		case ICMD_D2F:       /* ..., value  ==> ..., (float) value            */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			if (opt_noieee) {
				M_CVTDF(s1, d);
				}
			else {
				M_CVTDFS(s1, d);
				M_TRAPB;
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;
		
		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */
		case ICMD_DCMPL:
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (opt_noieee) {
				M_LSUB_IMM(REG_ZERO, 1, d);
				M_FCMPEQ(s1, s2, REG_FTMP3);
				M_FBEQZ (REG_FTMP3, 1);        /* jump over next instructions */
				M_CLR   (d);
				M_FCMPLT(s2, s1, REG_FTMP3);
				M_FBEQZ (REG_FTMP3, 1);        /* jump over next instruction  */
				M_LADD_IMM(REG_ZERO, 1, d);
				}
			else {
				M_LSUB_IMM(REG_ZERO, 1, d);
				M_FCMPEQS(s1, s2, REG_FTMP3);
				M_TRAPB;
				M_FBEQZ (REG_FTMP3, 1);        /* jump over next instructions */
				M_CLR   (d);
				M_FCMPLTS(s2, s1, REG_FTMP3);
				M_TRAPB;
				M_FBEQZ (REG_FTMP3, 1);        /* jump over next instruction  */
				M_LADD_IMM(REG_ZERO, 1, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;
			
		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */
		case ICMD_DCMPG:
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (opt_noieee) {
				M_LADD_IMM(REG_ZERO, 1, d);
				M_FCMPEQ(s1, s2, REG_FTMP3);
				M_FBEQZ (REG_FTMP3, 1);        /* jump over next instruction  */
				M_CLR   (d);
				M_FCMPLT(s1, s2, REG_FTMP3);
				M_FBEQZ (REG_FTMP3, 1);        /* jump over next instruction  */
				M_LSUB_IMM(REG_ZERO, 1, d);
				}
			else {
				M_LADD_IMM(REG_ZERO, 1, d);
				M_FCMPEQS(s1, s2, REG_FTMP3);
				M_TRAPB;
				M_FBEQZ (REG_FTMP3, 1);        /* jump over next instruction  */
				M_CLR   (d);
				M_FCMPLTS(s1, s2, REG_FTMP3);
				M_TRAPB;
				M_FBEQZ (REG_FTMP3, 1);        /* jump over next instruction  */
				M_LSUB_IMM(REG_ZERO, 1, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;


		/* memory operations **************************************************/

		case ICMD_ARRAYLENGTH: /* ..., arrayref  ==> ..., length              */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			gen_nullptr_check(s1);
			M_ILD(d, s1, OFFSET(java_arrayheader, size));
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
			M_SAADDQ(s2, s1, REG_ITMP1);
			M_ALD( d, REG_ITMP1, OFFSET(java_objectarray, data[0]));
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
			M_S8ADDQ(s2, s1, REG_ITMP1);
			M_LLD(d, REG_ITMP1, OFFSET(java_longarray, data[0]));
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
		  
			M_S4ADDQ(s2, s1, REG_ITMP1);
			M_ILD(d, REG_ITMP1, OFFSET(java_intarray, data[0]));
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
			M_S4ADDQ(s2, s1, REG_ITMP1);
			M_FLD(d, REG_ITMP1, OFFSET(java_floatarray, data[0]));
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
			M_S8ADDQ(s2, s1, REG_ITMP1);
			M_DLD(d, REG_ITMP1, OFFSET(java_doublearray, data[0]));
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
			if (has_ext_instr_set) {
				M_LADD(s2, s1, REG_ITMP1);
				M_LADD(s2, REG_ITMP1, REG_ITMP1);
				M_SLDU(d, REG_ITMP1, OFFSET(java_chararray, data[0]));
				}
			else {
				M_LADD (s2, s1, REG_ITMP1);
				M_LADD (s2, REG_ITMP1, REG_ITMP1);
				M_LLD_U(REG_ITMP2, REG_ITMP1, OFFSET(java_chararray, data[0]));
				M_LDA  (REG_ITMP1, REG_ITMP1, OFFSET(java_chararray, data[0]));
				M_EXTWL(REG_ITMP2, REG_ITMP1, d);
				}
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
			if (has_ext_instr_set) {
				M_LADD(s2, s1, REG_ITMP1);
				M_LADD(s2, REG_ITMP1, REG_ITMP1);
				M_SLDU( d, REG_ITMP1, OFFSET (java_shortarray, data[0]));
				M_SSEXT(d, d);
				}
			else {
				M_LADD(s2, s1, REG_ITMP1);
				M_LADD(s2, REG_ITMP1, REG_ITMP1);
				M_LLD_U(REG_ITMP2, REG_ITMP1, OFFSET(java_shortarray, data[0]));
				M_LDA(REG_ITMP1, REG_ITMP1, OFFSET(java_shortarray, data[0])+2);
				M_EXTQH(REG_ITMP2, REG_ITMP1, d);
				M_SRA_IMM(d, 48, d);
				}
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
			if (has_ext_instr_set) {
				M_LADD   (s2, s1, REG_ITMP1);
				M_BLDU   (d, REG_ITMP1, OFFSET (java_bytearray, data[0]));
				M_BSEXT  (d, d);
				}
			else {
				M_LADD(s2, s1, REG_ITMP1);
				M_LLD_U(REG_ITMP2, REG_ITMP1, OFFSET(java_bytearray, data[0]));
				M_LDA(REG_ITMP1, REG_ITMP1, OFFSET(java_bytearray, data[0])+1);
				M_EXTQH(REG_ITMP2, REG_ITMP1, d);
				M_SRA_IMM(d, 56, d);
				}
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
			M_SAADDQ(s2, s1, REG_ITMP1);
			M_AST   (s3, REG_ITMP1, OFFSET(java_objectarray, data[0]));
			break;

		case ICMD_LASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_int(s3, src, REG_ITMP3);
			M_S8ADDQ(s2, s1, REG_ITMP1);
			M_LST   (s3, REG_ITMP1, OFFSET(java_longarray, data[0]));
			break;

		case ICMD_IASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}

			var_to_reg_int(s3, src, REG_ITMP3);
			M_S4ADDQ(s2, s1, REG_ITMP1);
			M_IST   (s3, REG_ITMP1, OFFSET(java_intarray, data[0]));
			break;

		case ICMD_FASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_flt(s3, src, REG_FTMP3);
			M_S4ADDQ(s2, s1, REG_ITMP1);
			M_FST   (s3, REG_ITMP1, OFFSET(java_floatarray, data[0]));
			break;

		case ICMD_DASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_flt(s3, src, REG_FTMP3);
			M_S8ADDQ(s2, s1, REG_ITMP1);
			M_DST   (s3, REG_ITMP1, OFFSET(java_doublearray, data[0]));
			break;

		case ICMD_CASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_int(s3, src, REG_ITMP3);
			if (has_ext_instr_set) {
				M_LADD(s2, s1, REG_ITMP1);
				M_LADD(s2, REG_ITMP1, REG_ITMP1);
				M_SST (s3, REG_ITMP1, OFFSET(java_chararray, data[0]));
				}
			else {
				M_LADD (s2, s1, REG_ITMP1);
				M_LADD (s2, REG_ITMP1, REG_ITMP1);
				M_LLD_U(REG_ITMP2, REG_ITMP1, OFFSET(java_chararray, data[0]));
				M_LDA  (REG_ITMP1, REG_ITMP1, OFFSET(java_chararray, data[0]));
				M_INSWL(s3, REG_ITMP1, REG_ITMP3);
				M_MSKWL(REG_ITMP2, REG_ITMP1, REG_ITMP2);
				M_OR   (REG_ITMP2, REG_ITMP3, REG_ITMP2);
				M_LST_U(REG_ITMP2, REG_ITMP1, 0);
				}
			break;

		case ICMD_SASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_int(s3, src, REG_ITMP3);
			if (has_ext_instr_set) {
				M_LADD(s2, s1, REG_ITMP1);
				M_LADD(s2, REG_ITMP1, REG_ITMP1);
				M_SST (s3, REG_ITMP1, OFFSET(java_shortarray, data[0]));
				}
			else {
				M_LADD (s2, s1, REG_ITMP1);
				M_LADD (s2, REG_ITMP1, REG_ITMP1);
				M_LLD_U(REG_ITMP2, REG_ITMP1, OFFSET(java_shortarray, data[0]));
				M_LDA  (REG_ITMP1, REG_ITMP1, OFFSET(java_shortarray, data[0]));
				M_INSWL(s3, REG_ITMP1, REG_ITMP3);
				M_MSKWL(REG_ITMP2, REG_ITMP1, REG_ITMP2);
				M_OR   (REG_ITMP2, REG_ITMP3, REG_ITMP2);
				M_LST_U(REG_ITMP2, REG_ITMP1, 0);
				}
			break;

		case ICMD_BASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_int(s3, src, REG_ITMP3);
			if (has_ext_instr_set) {
				M_LADD(s2, s1, REG_ITMP1);
				M_BST (s3, REG_ITMP1, OFFSET(java_bytearray, data[0]));
				}
			else {
				M_LADD (s2, s1, REG_ITMP1);
				M_LLD_U(REG_ITMP2, REG_ITMP1, OFFSET(java_bytearray, data[0]));
				M_LDA  (REG_ITMP1, REG_ITMP1, OFFSET(java_bytearray, data[0]));
				M_INSBL(s3, REG_ITMP1, REG_ITMP3);
				M_MSKBL(REG_ITMP2, REG_ITMP1, REG_ITMP2);
				M_OR   (REG_ITMP2, REG_ITMP3, REG_ITMP2);
				M_LST_U(REG_ITMP2, REG_ITMP1, 0);
				}
			break;


		case ICMD_IASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_S4ADDQ(s2, s1, REG_ITMP1);
			M_IST(REG_ZERO, REG_ITMP1, OFFSET(java_intarray, data[0]));
			break;

		case ICMD_LASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_S8ADDQ(s2, s1, REG_ITMP1);
			M_LST(REG_ZERO, REG_ITMP1, OFFSET(java_longarray, data[0]));
			break;

		case ICMD_AASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_SAADDQ(s2, s1, REG_ITMP1);
			M_AST(REG_ZERO, REG_ITMP1, OFFSET(java_objectarray, data[0]));
			break;

		case ICMD_BASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			if (has_ext_instr_set) {
				M_LADD(s2, s1, REG_ITMP1);
				M_BST(REG_ZERO, REG_ITMP1, OFFSET(java_bytearray, data[0]));

			} else {
				M_LADD(s2, s1, REG_ITMP1);
				M_LLD_U(REG_ITMP2, REG_ITMP1, OFFSET(java_bytearray, data[0]));
				M_LDA(REG_ITMP1, REG_ITMP1, OFFSET(java_bytearray, data[0]));
				M_INSBL(REG_ZERO, REG_ITMP1, REG_ITMP3);
				M_MSKBL(REG_ITMP2, REG_ITMP1, REG_ITMP2);
				M_OR(REG_ITMP2, REG_ITMP3, REG_ITMP2);
				M_LST_U(REG_ITMP2, REG_ITMP1, 0);
			}
			break;

		case ICMD_CASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			if (has_ext_instr_set) {
				M_LADD(s2, s1, REG_ITMP1);
				M_LADD(s2, REG_ITMP1, REG_ITMP1);
				M_SST(REG_ZERO, REG_ITMP1, OFFSET(java_chararray, data[0]));

			} else {
				M_LADD(s2, s1, REG_ITMP1);
				M_LADD(s2, REG_ITMP1, REG_ITMP1);
				M_LLD_U(REG_ITMP2, REG_ITMP1, OFFSET(java_chararray, data[0]));
				M_LDA(REG_ITMP1, REG_ITMP1, OFFSET(java_chararray, data[0]));
				M_INSWL(REG_ZERO, REG_ITMP1, REG_ITMP3);
				M_MSKWL(REG_ITMP2, REG_ITMP1, REG_ITMP2);
				M_OR(REG_ITMP2, REG_ITMP3, REG_ITMP2);
				M_LST_U(REG_ITMP2, REG_ITMP1, 0);
			}
			break;

		case ICMD_SASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			if (has_ext_instr_set) {
				M_LADD(s2, s1, REG_ITMP1);
				M_LADD(s2, REG_ITMP1, REG_ITMP1);
				M_SST(REG_ZERO, REG_ITMP1, OFFSET(java_shortarray, data[0]));

			} else {
				M_LADD(s2, s1, REG_ITMP1);
				M_LADD(s2, REG_ITMP1, REG_ITMP1);
				M_LLD_U(REG_ITMP2, REG_ITMP1, OFFSET(java_shortarray, data[0]));
				M_LDA(REG_ITMP1, REG_ITMP1, OFFSET(java_shortarray, data[0]));
				M_INSWL(REG_ZERO, REG_ITMP1, REG_ITMP3);
				M_MSKWL(REG_ITMP2, REG_ITMP1, REG_ITMP2);
				M_OR(REG_ITMP2, REG_ITMP3, REG_ITMP2);
				M_LST_U(REG_ITMP2, REG_ITMP1, 0);
			}
			break;


		case ICMD_GETSTATIC:  /* ...  ==> ..., value                          */
		                      /* op1 = type, val.a = field address            */

			if (!iptr->val.a) {
				codegen_addpatchref(cd, mcodeptr,
									PATCHER_get_putstatic,
									(unresolved_field *) iptr->target);

				if (showdisassemble)
					M_NOP;

				a = 0;

			} else {
				fieldinfo *fi = iptr->val.a;

				if (!fi->class->initialized) {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_clinit, fi->class);

					if (showdisassemble)
						M_NOP;
				}

				a = (ptrint) &(fi->value);
  			}

			a = dseg_addaddress(cd, a);
			M_ALD(REG_ITMP1, REG_PV, a);
			switch (iptr->op1) {
			case TYPE_INT:
				d = reg_of_var(rd, iptr->dst, REG_ITMP3);
				M_ILD(d, REG_ITMP1, 0);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_LNG:
				d = reg_of_var(rd, iptr->dst, REG_ITMP3);
				M_LLD(d, REG_ITMP1, 0);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_ADR:
				d = reg_of_var(rd, iptr->dst, REG_ITMP3);
				M_ALD(d, REG_ITMP1, 0);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_FLT:
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				M_FLD(d, REG_ITMP1, 0);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			case TYPE_DBL:				
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				M_DLD(d, REG_ITMP1, 0);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			}
			break;

		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */
		                      /* op1 = type, val.a = field address            */

			if (!iptr->val.a) {
				codegen_addpatchref(cd, mcodeptr,
									PATCHER_get_putstatic,
									(unresolved_field *) iptr->target);

				if (showdisassemble)
					M_NOP;

				a = 0;

			} else {
				fieldinfo *fi = iptr->val.a;

				if (!fi->class->initialized) {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_clinit, fi->class);

					if (showdisassemble)
						M_NOP;
				}

				a = (ptrint) &(fi->value);
  			}

			a = dseg_addaddress(cd, a);
			M_ALD(REG_ITMP1, REG_PV, a);
			switch (iptr->op1) {
			case TYPE_INT:
				var_to_reg_int(s2, src, REG_ITMP2);
				M_IST(s2, REG_ITMP1, 0);
				break;
			case TYPE_LNG:
				var_to_reg_int(s2, src, REG_ITMP2);
				M_LST(s2, REG_ITMP1, 0);
				break;
			case TYPE_ADR:
				var_to_reg_int(s2, src, REG_ITMP2);
				M_AST(s2, REG_ITMP1, 0);
				break;
			case TYPE_FLT:
				var_to_reg_flt(s2, src, REG_FTMP2);
				M_FST(s2, REG_ITMP1, 0);
				break;
			case TYPE_DBL:
				var_to_reg_flt(s2, src, REG_FTMP2);
				M_DST(s2, REG_ITMP1, 0);
				break;
			}
			break;

		case ICMD_PUTSTATICCONST: /* ...  ==> ...                             */
		                          /* val = value (in current instruction)     */
		                          /* op1 = type, val.a = field address (in    */
		                          /* following NOP)                           */

			if (!iptr[1].val.a) {
				codegen_addpatchref(cd, mcodeptr,
									PATCHER_get_putstatic,
									(unresolved_field *) iptr[1].target);

				if (showdisassemble)
					M_NOP;

				a = 0;

			} else {
				fieldinfo *fi = iptr[1].val.a;

				if (!fi->class->initialized) {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_clinit, fi->class);

					if (showdisassemble)
						M_NOP;
				}

				a = (ptrint) &(fi->value);
  			}
			
			a = dseg_addaddress(cd, a);
			M_ALD(REG_ITMP1, REG_PV, a);
			switch (iptr->op1) {
			case TYPE_INT:
				M_IST(REG_ZERO, REG_ITMP1, 0);
				break;
			case TYPE_LNG:
				M_LST(REG_ZERO, REG_ITMP1, 0);
				break;
			case TYPE_ADR:
				M_AST(REG_ZERO, REG_ITMP1, 0);
				break;
			case TYPE_FLT:
				M_FST(REG_ZERO, REG_ITMP1, 0);
				break;
			case TYPE_DBL:
				M_DST(REG_ZERO, REG_ITMP1, 0);
				break;
			}
			break;


		case ICMD_GETFIELD:   /* ...  ==> ..., value                          */
		                      /* op1 = type, val.i = field offset             */

			var_to_reg_int(s1, src, REG_ITMP1);
			gen_nullptr_check(s1);

			if (!iptr->val.a) {
				codegen_addpatchref(cd, mcodeptr,
									PATCHER_get_putfield,
									(unresolved_field *) iptr->target);

				if (showdisassemble)
					M_NOP;

				a = 0;

			} else {
				a = ((fieldinfo *) (iptr->val.a))->offset;
			}

			switch (iptr->op1) {
			case TYPE_INT:
				d = reg_of_var(rd, iptr->dst, REG_ITMP3);
				M_ILD(d, s1, a);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_LNG:
				d = reg_of_var(rd, iptr->dst, REG_ITMP3);
				M_LLD(d, s1, a);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_ADR:
				d = reg_of_var(rd, iptr->dst, REG_ITMP3);
				M_ALD(d, s1, a);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_FLT:
				d = reg_of_var(rd, iptr->dst, REG_FTMP3);
				M_FLD(d, s1, a);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			case TYPE_DBL:				
				d = reg_of_var(rd, iptr->dst, REG_FTMP3);
				M_DLD(d, s1, a);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			}
			break;

		case ICMD_PUTFIELD:   /* ..., objectref, value  ==> ...               */
		                      /* op1 = type, val.a = field address            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			gen_nullptr_check(s1);

			if (!IS_FLT_DBL_TYPE(iptr->op1)) {
				var_to_reg_int(s2, src, REG_ITMP2);
			} else {
				var_to_reg_flt(s2, src, REG_FTMP2);
			}

			if (!iptr->val.a) {
				codegen_addpatchref(cd, mcodeptr,
									PATCHER_get_putfield,
									(unresolved_field *) iptr->target);

				if (showdisassemble)
					M_NOP;

				a = 0;

			} else {
				a = ((fieldinfo *) (iptr->val.a))->offset;
			}

			switch (iptr->op1) {
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
		                          /* op1 = type, val.a = field address (in    */
		                          /* following NOP)                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			gen_nullptr_check(s1);

			if (!iptr[1].val.a) {
				codegen_addpatchref(cd, mcodeptr,
									PATCHER_get_putfield,
									(unresolved_field *) iptr[1].target);

				if (showdisassemble)
					M_NOP;

				a = 0;

			} else {
				a = ((fieldinfo *) (iptr[1].val.a))->offset;
			}

			switch (iptr[1].op1) {
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

			var_to_reg_int(s1, src, REG_ITMP1);
			M_INTMOVE(s1, REG_ITMP1_XPTR);
			a = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP2, REG_PV, a);
			M_JMP(REG_ITMP2_XPC, REG_ITMP2);
			M_NOP;              /* nop ensures that XPC is less than the end */
			                    /* of basic block                            */
			ALIGNCODENOP;
			break;

		case ICMD_GOTO:         /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */
			M_BR(0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			ALIGNCODENOP;
			break;

		case ICMD_JSR:          /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */

			M_BSR(REG_ITMP1, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;
			
		case ICMD_RET:          /* ... ==> ...                                */
		                        /* op1 = local variable                       */

			var = &(rd->locals[iptr->op1][TYPE_ADR]);
			if (var->flags & INMEMORY) {
				M_ALD(REG_ITMP1, REG_SP, 8 * var->regoff);
				M_RET(REG_ZERO, REG_ITMP1);
				}
			else
				M_RET(REG_ZERO, var->regoff);
			ALIGNCODENOP;
			break;

		case ICMD_IFNULL:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_BEQZ(s1, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFNONNULL:    /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_BNEZ(s1, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFEQ:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BEQZ(s1, 0);
				}
			else {
				if ((iptr->val.i > 0) && (iptr->val.i <= 255)) {
					M_CMPEQ_IMM(s1, iptr->val.i, REG_ITMP1);
					}
				else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMPEQ(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BNEZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFLT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BLTZ(s1, 0);
				}
			else {
				if ((iptr->val.i > 0) && (iptr->val.i <= 255)) {
					M_CMPLT_IMM(s1, iptr->val.i, REG_ITMP1);
					}
				else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMPLT(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BNEZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFLE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BLEZ(s1, 0);
				}
			else {
				if ((iptr->val.i > 0) && (iptr->val.i <= 255)) {
					M_CMPLE_IMM(s1, iptr->val.i, REG_ITMP1);
					}
				else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMPLE(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BNEZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFNE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BNEZ(s1, 0);
				}
			else {
				if ((iptr->val.i > 0) && (iptr->val.i <= 255)) {
					M_CMPEQ_IMM(s1, iptr->val.i, REG_ITMP1);
					}
				else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMPEQ(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BEQZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFGT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BGTZ(s1, 0);
				}
			else {
				if ((iptr->val.i > 0) && (iptr->val.i <= 255)) {
					M_CMPLE_IMM(s1, iptr->val.i, REG_ITMP1);
					}
				else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMPLE(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BEQZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFGE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BGEZ(s1, 0);
				}
			else {
				if ((iptr->val.i > 0) && (iptr->val.i <= 255)) {
					M_CMPLT_IMM(s1, iptr->val.i, REG_ITMP1);
					}
				else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMPLT(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BEQZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_LEQ:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BEQZ(s1, 0);
				}
			else {
				if ((iptr->val.l > 0) && (iptr->val.l <= 255)) {
					M_CMPEQ_IMM(s1, iptr->val.l, REG_ITMP1);
					}
				else {
					LCONST(REG_ITMP2, iptr->val.l);
					M_CMPEQ(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BNEZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_LLT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BLTZ(s1, 0);
				}
			else {
				if ((iptr->val.l > 0) && (iptr->val.l <= 255)) {
					M_CMPLT_IMM(s1, iptr->val.l, REG_ITMP1);
					}
				else {
					LCONST(REG_ITMP2, iptr->val.l);
					M_CMPLT(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BNEZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_LLE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BLEZ(s1, 0);
				}
			else {
				if ((iptr->val.l > 0) && (iptr->val.l <= 255)) {
					M_CMPLE_IMM(s1, iptr->val.l, REG_ITMP1);
					}
				else {
					LCONST(REG_ITMP2, iptr->val.l);
					M_CMPLE(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BNEZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_LNE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BNEZ(s1, 0);
				}
			else {
				if ((iptr->val.l > 0) && (iptr->val.l <= 255)) {
					M_CMPEQ_IMM(s1, iptr->val.l, REG_ITMP1);
					}
				else {
					LCONST(REG_ITMP2, iptr->val.l);
					M_CMPEQ(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BEQZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_LGT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BGTZ(s1, 0);
				}
			else {
				if ((iptr->val.l > 0) && (iptr->val.l <= 255)) {
					M_CMPLE_IMM(s1, iptr->val.l, REG_ITMP1);
					}
				else {
					LCONST(REG_ITMP2, iptr->val.l);
					M_CMPLE(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BEQZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_LGE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BGEZ(s1, 0);
				}
			else {
				if ((iptr->val.l > 0) && (iptr->val.l <= 255)) {
					M_CMPLT_IMM(s1, iptr->val.l, REG_ITMP1);
					}
				else {
					LCONST(REG_ITMP2, iptr->val.l);
					M_CMPLT(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BEQZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPEQ:    /* op1 = target JavaVM pc                     */
		case ICMD_IF_ACMPEQ:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPEQ(s1, s2, REG_ITMP1);
			M_BNEZ(REG_ITMP1, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPNE:    /* op1 = target JavaVM pc                     */
		case ICMD_IF_ACMPNE:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPEQ(s1, s2, REG_ITMP1);
			M_BEQZ(REG_ITMP1, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPLT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPLT:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPLT(s1, s2, REG_ITMP1);
			M_BNEZ(REG_ITMP1, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPGT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPGT:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPLE(s1, s2, REG_ITMP1);
			M_BEQZ(REG_ITMP1, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPLE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPLE:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPLE(s1, s2, REG_ITMP1);
			M_BNEZ(REG_ITMP1, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPGE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPGE:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPLT(s1, s2, REG_ITMP1);
			M_BEQZ(REG_ITMP1, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		/* (value xx 0) ? IFxx_ICONST : ELSE_ICONST                           */

		case ICMD_ELSE_ICONST:  /* handled by IFxx_ICONST                     */
			break;

		case ICMD_IFEQ_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			s3 = iptr->val.i;
			if (iptr[1].opc == ICMD_ELSE_ICONST) {
				if ((s3 == 1) && (iptr[1].val.i == 0)) {
					M_CMPEQ(s1, REG_ZERO, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if ((s3 == 0) && (iptr[1].val.i == 1)) {
					M_CMPEQ(s1, REG_ZERO, d);
					M_XOR_IMM(d, 1, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if (s1 == d) {
					M_MOV(s1, REG_ITMP1);
					s1 = REG_ITMP1;
					}
				ICONST(d, iptr[1].val.i);
				}
			if ((s3 >= 0) && (s3 <= 255)) {
				M_CMOVEQ_IMM(s1, s3, d);
				}
			else {
				ICONST(REG_ITMP2, s3);
				M_CMOVEQ(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFNE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			s3 = iptr->val.i;
			if (iptr[1].opc == ICMD_ELSE_ICONST) {
				if ((s3 == 0) && (iptr[1].val.i == 1)) {
					M_CMPEQ(s1, REG_ZERO, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if ((s3 == 1) && (iptr[1].val.i == 0)) {
					M_CMPEQ(s1, REG_ZERO, d);
					M_XOR_IMM(d, 1, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if (s1 == d) {
					M_MOV(s1, REG_ITMP1);
					s1 = REG_ITMP1;
					}
				ICONST(d, iptr[1].val.i);
				}
			if ((s3 >= 0) && (s3 <= 255)) {
				M_CMOVNE_IMM(s1, s3, d);
				}
			else {
				ICONST(REG_ITMP2, s3);
				M_CMOVNE(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFLT_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			s3 = iptr->val.i;
			if ((iptr[1].opc == ICMD_ELSE_ICONST)) {
				if ((s3 == 1) && (iptr[1].val.i == 0)) {
					M_CMPLT(s1, REG_ZERO, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if ((s3 == 0) && (iptr[1].val.i == 1)) {
					M_CMPLE(REG_ZERO, s1, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if (s1 == d) {
					M_MOV(s1, REG_ITMP1);
					s1 = REG_ITMP1;
					}
				ICONST(d, iptr[1].val.i);
				}
			if ((s3 >= 0) && (s3 <= 255)) {
				M_CMOVLT_IMM(s1, s3, d);
				}
			else {
				ICONST(REG_ITMP2, s3);
				M_CMOVLT(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFGE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			s3 = iptr->val.i;
			if ((iptr[1].opc == ICMD_ELSE_ICONST)) {
				if ((s3 == 1) && (iptr[1].val.i == 0)) {
					M_CMPLE(REG_ZERO, s1, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if ((s3 == 0) && (iptr[1].val.i == 1)) {
					M_CMPLT(s1, REG_ZERO, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if (s1 == d) {
					M_MOV(s1, REG_ITMP1);
					s1 = REG_ITMP1;
					}
				ICONST(d, iptr[1].val.i);
				}
			if ((s3 >= 0) && (s3 <= 255)) {
				M_CMOVGE_IMM(s1, s3, d);
				}
			else {
				ICONST(REG_ITMP2, s3);
				M_CMOVGE(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFGT_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			s3 = iptr->val.i;
			if ((iptr[1].opc == ICMD_ELSE_ICONST)) {
				if ((s3 == 1) && (iptr[1].val.i == 0)) {
					M_CMPLT(REG_ZERO, s1, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if ((s3 == 0) && (iptr[1].val.i == 1)) {
					M_CMPLE(s1, REG_ZERO, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if (s1 == d) {
					M_MOV(s1, REG_ITMP1);
					s1 = REG_ITMP1;
					}
				ICONST(d, iptr[1].val.i);
				}
			if ((s3 >= 0) && (s3 <= 255)) {
				M_CMOVGT_IMM(s1, s3, d);
				}
			else {
				ICONST(REG_ITMP2, s3);
				M_CMOVGT(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFLE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			s3 = iptr->val.i;
			if ((iptr[1].opc == ICMD_ELSE_ICONST)) {
				if ((s3 == 1) && (iptr[1].val.i == 0)) {
					M_CMPLE(s1, REG_ZERO, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if ((s3 == 0) && (iptr[1].val.i == 1)) {
					M_CMPLT(REG_ZERO, s1, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
					}
				if (s1 == d) {
					M_MOV(s1, REG_ITMP1);
					s1 = REG_ITMP1;
					}
				ICONST(d, iptr[1].val.i);
				}
			if ((s3 >= 0) && (s3 <= 255)) {
				M_CMOVLE_IMM(s1, s3, d);
				}
			else {
				ICONST(REG_ITMP2, s3);
				M_CMOVLE(s1, REG_ITMP2, d);
				}
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

		case ICMD_RETURN:       /* ...  ==> ...                               */

nowperformreturn:
			{
			s4 i, p;
			
			p = parentargs_base;
			
			/* call trace function */

			if (runverbose) {
				M_LDA(REG_SP, REG_SP, -3 * 8);
				M_AST(REG_RA, REG_SP, 0 * 8);
				M_LST(REG_RESULT, REG_SP, 1 * 8);
				M_DST(REG_FRESULT, REG_SP, 2 * 8);
				a = dseg_addaddress(cd, m);
				M_ALD(rd->argintregs[0], REG_PV, a);
				M_MOV(REG_RESULT, rd->argintregs[1]);
				M_FLTMOVE(REG_FRESULT, rd->argfltregs[2]);
				M_FLTMOVE(REG_FRESULT, rd->argfltregs[3]);
				a = dseg_addaddress(cd, (void *) builtin_displaymethodstop);
				M_ALD(REG_PV, REG_PV, a);
				M_JSR(REG_RA, REG_PV);
				s1 = (s4) ((u1 *) mcodeptr - cd->mcodebase);
				if (s1 <= 32768) M_LDA(REG_PV, REG_RA, -s1);
				else {
					s4 ml = -s1, mh = 0;
					while (ml < -32768) { ml += 65536; mh--; }
					M_LDA(REG_PV, REG_RA, ml);
					M_LDAH(REG_PV, REG_PV, mh);
				}
				M_DLD(REG_FRESULT, REG_SP, 2 * 8);
				M_LLD(REG_RESULT, REG_SP, 1 * 8);
				M_ALD(REG_RA, REG_SP, 0 * 8);
				M_LDA(REG_SP, REG_SP, 3 * 8);
			}

#if defined(USE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				s4 disp;

				M_ALD(rd->argintregs[0], REG_SP, rd->maxmemuse * 8);

				switch (iptr->opc) {
				case ICMD_IRETURN:
				case ICMD_LRETURN:
				case ICMD_ARETURN:
					M_LST(REG_RESULT, REG_SP, rd->maxmemuse * 8);
					break;
				case ICMD_FRETURN:
				case ICMD_DRETURN:
					M_DST(REG_FRESULT, REG_SP, rd->maxmemuse * 8);
					break;
				}

				a = dseg_addaddress(cd, BUILTIN_monitorexit);
				M_ALD(REG_PV, REG_PV, a);
				M_JSR(REG_RA, REG_PV);
				disp = -(s4) ((u1 *) mcodeptr - cd->mcodebase);
				M_LDA(REG_PV, REG_RA, disp);

				switch (iptr->opc) {
				case ICMD_IRETURN:
				case ICMD_LRETURN:
				case ICMD_ARETURN:
					M_LLD(REG_RESULT, REG_SP, rd->maxmemuse * 8);
					break;
				case ICMD_FRETURN:
				case ICMD_DRETURN:
					M_DLD(REG_FRESULT, REG_SP, rd->maxmemuse * 8);
					break;
				}
			}
#endif

			/* restore return address                                         */

			if (!m->isleafmethod) {
				p--; M_LLD(REG_RA, REG_SP, p * 8);
			}

			/* restore saved registers                                        */

			for (i = rd->savintregcnt - 1; i >= rd->maxsavintreguse; i--) {
				p--; M_LLD(rd->savintregs[i], REG_SP, p * 8);
			}
			for (i = rd->savfltregcnt - 1; i >= rd->maxsavfltreguse; i--) {
				p--; M_DLD(rd->savfltregs[i], REG_SP, p * 8);
			}

			/* deallocate stack                                               */

			if (parentargs_base) {
				M_LDA(REG_SP, REG_SP, parentargs_base * 8);
			}

			M_RET(REG_ZERO, REG_RA);
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
			if (l == 0)
				{M_INTMOVE(s1, REG_ITMP1);}
			else if (l <= 32768) {
				M_LDA(REG_ITMP1, s1, -l);
				}
			else {
				ICONST(REG_ITMP2, l);
				M_ISUB(s1, REG_ITMP2, REG_ITMP1);
				}
			i = i - l + 1;

			/* range check */

			if (i <= 256)
				M_CMPULE_IMM(REG_ITMP1, i - 1, REG_ITMP2);
			else {
				M_LDA(REG_ITMP2, REG_ZERO, i - 1);
				M_CMPULE(REG_ITMP1, REG_ITMP2, REG_ITMP2);
				}
			M_BEQZ(REG_ITMP2, 0);


			/* codegen_addreference(cd, BlockPtrOfPC(s4ptr[0]), mcodeptr); */
			codegen_addreference(cd, (basicblock *) tptr[0], mcodeptr);

			/* build jump table top down and use address of lowest entry */

			/* s4ptr += 3 + i; */
			tptr += i;

			while (--i >= 0) {
				/* dseg_addtarget(cd, BlockPtrOfPC(*--s4ptr)); */
				dseg_addtarget(cd, (basicblock *) tptr[0]); 
				--tptr;
				}
			}

			/* length of dataseg after last dseg_addtarget is used by load */

			M_SAADDQ(REG_ITMP1, REG_PV, REG_ITMP2);
			M_ALD(REG_ITMP2, REG_ITMP2, -(cd->dseglen));
			M_JMP(REG_ZERO, REG_ITMP2);
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
			var_to_reg_int(s1, src, REG_ITMP1);
			while (--i >= 0) {
				s4ptr += 2;
				++tptr;

				val = s4ptr[0];
				if ((val >= 0) && (val <= 255)) {
					M_CMPEQ_IMM(s1, val, REG_ITMP2);
					}
				else {
					if ((val >= -32768) && (val <= 32767)) {
						M_LDA(REG_ITMP2, REG_ZERO, val);
						} 
					else {
						a = dseg_adds4(cd, val);
						M_ILD(REG_ITMP2, REG_PV, a);
						}
					M_CMPEQ(s1, REG_ITMP2, REG_ITMP2);
					}
				M_BNEZ(REG_ITMP2, 0);
				/* codegen_addreference(cd, BlockPtrOfPC(s4ptr[1]), mcodeptr); */
				codegen_addreference(cd, (basicblock *) tptr[0], mcodeptr); 
				}

			M_BR(0);
			/* codegen_addreference(cd, BlockPtrOfPC(l), mcodeptr); */
			
			tptr = (void **) iptr->target;
			codegen_addreference(cd, (basicblock *) tptr[0], mcodeptr);

			ALIGNCODENOP;
			break;
			}


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
				if (src->varkind == ARGVAR)
					continue;
				if (IS_INT_LNG_TYPE(src->type)) {
					if (s3 < INT_ARG_CNT) {
						s1 = rd->argintregs[s3];
						var_to_reg_int(d, src, s1);
						M_INTMOVE(d, s1);

					} else {
						var_to_reg_int(d, src, REG_ITMP1);
						M_LST(d, REG_SP, 8 * (s3 - INT_ARG_CNT));
					}

				} else {
					if (s3 < FLT_ARG_CNT) {
						s1 = rd->argfltregs[s3];
						var_to_reg_flt(d, src, s1);
						M_FLTMOVE(d, s1);

					} else {
						var_to_reg_flt(d, src, REG_FTMP1);
						M_DST(d, REG_SP, 8 * (s3 - FLT_ARG_CNT));
					}
				}
			} /* end of for */

			lm = iptr->val.a;
			switch (iptr->opc) {
			case ICMD_BUILTIN3:
			case ICMD_BUILTIN2:
			case ICMD_BUILTIN1:
				if (iptr->target) {
					codegen_addpatchref(cd, mcodeptr,
										(functionptr) lm, iptr->target);

					if (showdisassemble)
						M_NOP;

					a = 0;

				} else {
					a = (ptrint) lm;
				}

				a = dseg_addaddress(cd, a);
				d = iptr->op1;

				M_ALD(REG_PV, REG_PV, a);     /* Pointer to built-in-function */
				break;

			case ICMD_INVOKESPECIAL:
				gen_nullptr_check(rd->argintregs[0]);
				M_ILD(REG_ITMP1, rd->argintregs[0], 0); /* hardware nullptr   */
				/* fall through */

			case ICMD_INVOKESTATIC:
				if (!lm) {
					unresolved_method *um = iptr->target;

					codegen_addpatchref(cd, mcodeptr,
										PATCHER_invokestatic_special, um);

					if (showdisassemble)
						M_NOP;

					a = 0;
					d = um->methodref->parseddesc.md->returntype.type;

				} else {
					a = (ptrint) lm->stubroutine;
					d = lm->parseddesc->returntype.type;
				}

				a = dseg_addaddress(cd, a);
				M_ALD(REG_PV, REG_PV, a);            /* method pointer in r27 */
				break;

			case ICMD_INVOKEVIRTUAL:
				gen_nullptr_check(rd->argintregs[0]);

				if (!lm) {
					unresolved_method *um = iptr->target;

					codegen_addpatchref(cd, mcodeptr,
										PATCHER_invokevirtual, um);

					if (showdisassemble)
						M_NOP;

					s1 = 0;
					d = um->methodref->parseddesc.md->returntype.type;

				} else {
					s1 = OFFSET(vftbl_t, table[0]) +
						sizeof(methodptr) * lm->vftblindex;
					d = lm->parseddesc->returntype.type;
				}

				M_ALD(REG_METHODPTR, rd->argintregs[0],
					  OFFSET(java_objectheader, vftbl));
				M_ALD(REG_PV, REG_METHODPTR, s1);
				break;

			case ICMD_INVOKEINTERFACE:
				gen_nullptr_check(rd->argintregs[0]);

				if (!lm) {
					unresolved_method *um = iptr->target;

					codegen_addpatchref(cd, mcodeptr,
										PATCHER_invokeinterface, um);

					if (showdisassemble)
						M_NOP;

					s1 = 0;
					s2 = 0;
					d = um->methodref->parseddesc.md->returntype.type;

				} else {
					s1 = OFFSET(vftbl_t, interfacetable[0]) -
						sizeof(methodptr*) * lm->class->index;

					s2 = sizeof(methodptr) * (lm - lm->class->methods);

					d = lm->parseddesc->returntype.type;
				}
					
				M_ALD(REG_METHODPTR, rd->argintregs[0],
					  OFFSET(java_objectheader, vftbl));    
				M_ALD(REG_METHODPTR, REG_METHODPTR, s1);
				M_ALD(REG_PV, REG_METHODPTR, s2);
				break;
			}

			M_JSR(REG_RA, REG_PV);

			/* recompute pv */

			s1 = (s4) ((u1 *) mcodeptr - cd->mcodebase);
			if (s1 <= 32768) M_LDA(REG_PV, REG_RA, -s1);
			else {
				s4 ml = -s1, mh = 0;
				while (ml < -32768) { ml += 65536; mh--; }
				M_LDA(REG_PV, REG_RA, ml);
				M_LDAH(REG_PV, REG_PV, mh);
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
			codegen_threadcritrestart(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
			var_to_reg_int(s1, src, REG_ITMP1);

			/* calculate interface checkcast code size */

			s2 = 6;
			if (!super)
				s2 += showdisassemble ? 1 : 0;

			/* calculate class checkcast code size */

			s3 = 9 /* 8 + (s1 == REG_ITMP1) */;
			if (!super)
				s3 += showdisassemble ? 1 : 0;

			/* if class is not resolved, check which code to call */

			if (!super) {
				M_BEQZ(s1, 4 + (showdisassemble ? 1 : 0) + s2 + 1 + s3);

				codegen_addpatchref(cd, mcodeptr,
									PATCHER_checkcast_instanceof_flags,
									(constant_classref *) iptr->target);

				if (showdisassemble)
					M_NOP;

				a = dseg_adds4(cd, 0); /* super->flags */
				M_ILD(REG_ITMP2, REG_PV, a);
				a = dseg_adds4(cd, ACC_INTERFACE);
				M_ILD(REG_ITMP3, REG_PV, a);
				M_AND(REG_ITMP2, REG_ITMP3, REG_ITMP2);
				M_BEQZ(REG_ITMP2, s2 + 1);
			}

			/* interface checkcast code */

			if (!super || (super->flags & ACC_INTERFACE)) {
				if (super) {
					M_BEQZ(s1, s2);

				} else {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_checkcast_instanceof_interface,
										(constant_classref *) iptr->target);

					if (showdisassemble)
						M_NOP;
				}

				M_ALD(REG_ITMP2, s1, OFFSET(java_objectheader, vftbl));
				M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl_t, interfacetablelength));
				M_LDA(REG_ITMP3, REG_ITMP3, -superindex);
				M_BLEZ(REG_ITMP3, 0);
				codegen_addxcastrefs(cd, mcodeptr);
				M_ALD(REG_ITMP3, REG_ITMP2,
					  OFFSET(vftbl_t, interfacetable[0]) -
					  superindex * sizeof(methodptr*));
				M_BEQZ(REG_ITMP3, 0);
				codegen_addxcastrefs(cd, mcodeptr);

				if (!super)
					M_BR(s3);
			}

			/* class checkcast code */

			if (!super || !(super->flags & ACC_INTERFACE)) {
				if (super) {
					M_BEQZ(s1, s3);

				} else {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_checkcast_instanceof_class,
										(constant_classref *) iptr->target);

					if (showdisassemble)
						M_NOP;
				}

				M_ALD(REG_ITMP2, s1, OFFSET(java_objectheader, vftbl));
				a = dseg_addaddress(cd, supervftbl);
				M_ALD(REG_ITMP3, REG_PV, a);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
				codegen_threadcritstart(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
				M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, baseval));
/*  				if (s1 != REG_ITMP1) { */
/*  					M_ILD(REG_ITMP1, REG_ITMP3, OFFSET(vftbl_t, baseval)); */
/*  					M_ILD(REG_ITMP3, REG_ITMP3, OFFSET(vftbl_t, diffval)); */
/*  #if defined(USE_THREADS) && defined(NATIVE_THREADS) */
/*  					codegen_threadcritstop(cd, (u1 *) mcodeptr - cd->mcodebase); */
/*  #endif */
/*  					M_ISUB(REG_ITMP2, REG_ITMP1, REG_ITMP2); */

/*  				} else { */
					M_ILD(REG_ITMP3, REG_ITMP3, OFFSET(vftbl_t, baseval));
					M_ISUB(REG_ITMP2, REG_ITMP3, REG_ITMP2);
					M_ALD(REG_ITMP3, REG_PV, a);
					M_ILD(REG_ITMP3, REG_ITMP3, OFFSET(vftbl_t, diffval));
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
					codegen_threadcritstop(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
/*  				} */
				M_CMPULE(REG_ITMP2, REG_ITMP3, REG_ITMP3);
				M_BEQZ(REG_ITMP3, 0);
				codegen_addxcastrefs(cd, mcodeptr);
			}
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			}
			break;

		case ICMD_INSTANCEOF: /* ..., objectref ==> ..., intresult            */

		                      /* op1:   0 == array, 1 == class                */
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
			
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			codegen_threadcritrestart(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP2);
			if (s1 == d) {
				M_MOV(s1, REG_ITMP1);
				s1 = REG_ITMP1;
			}

			/* calculate interface instanceof code size */

			s2 = 6;
			if (!super)
				s2 += (d == REG_ITMP2 ? 1 : 0) + (showdisassemble ? 1 : 0);

			/* calculate class instanceof code size */

			s3 = 7;
			if (!super)
				s3 += (showdisassemble ? 1 : 0);

			/* if class is not resolved, check which code to call */

			if (!super) {
				M_CLR(d);
				M_BEQZ(s1, 4 + (showdisassemble ? 1 : 0) + s2 + 1 + s3);

				codegen_addpatchref(cd, mcodeptr,
									PATCHER_checkcast_instanceof_flags,
									(constant_classref *) iptr->target);

				if (showdisassemble)
					M_NOP;

				a = dseg_adds4(cd, 0); /* super->flags */
				M_ILD(REG_ITMP3, REG_PV, a);
				a = dseg_adds4(cd, ACC_INTERFACE);
				M_ILD(REG_ITMP2, REG_PV, a);
				M_AND(REG_ITMP3, REG_ITMP2, REG_ITMP3);
				M_BEQZ(REG_ITMP3, s2 + 1);
			}

			/* interface instanceof code */

			if (!super || (super->flags & ACC_INTERFACE)) {
				if (super) {
					M_CLR(d);
					M_BEQZ(s1, s2);

				} else {
					/* If d == REG_ITMP2, then it's destroyed in check code   */
					/* above.                                                 */
					if (d == REG_ITMP2)
						M_CLR(d);

					codegen_addpatchref(cd, mcodeptr,
										PATCHER_checkcast_instanceof_interface,
										(constant_classref *) iptr->target);

					if (showdisassemble)
						M_NOP;
				}

				M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
				M_ILD(REG_ITMP3, REG_ITMP1, OFFSET(vftbl_t, interfacetablelength));
				M_LDA(REG_ITMP3, REG_ITMP3, -superindex);
				M_BLEZ(REG_ITMP3, 2);
				M_ALD(REG_ITMP1, REG_ITMP1,
					  OFFSET(vftbl_t, interfacetable[0]) -
					  superindex * sizeof(methodptr*));
				M_CMPULT(REG_ZERO, REG_ITMP1, d);      /* REG_ITMP1 != 0  */

				if (!super)
					M_BR(s3);
			}

			/* class instanceof code */

			if (!super || !(super->flags & ACC_INTERFACE)) {
				if (super) {
					M_CLR(d);
					M_BEQZ(s1, s3);

				} else {
					codegen_addpatchref(cd, mcodeptr,
										PATCHER_checkcast_instanceof_class,
										(constant_classref *) iptr->target);

					if (showdisassemble)
						M_NOP;
				}

				M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
				a = dseg_addaddress(cd, supervftbl);
				M_ALD(REG_ITMP2, REG_PV, a);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
				codegen_threadcritstart(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
				M_ILD(REG_ITMP1, REG_ITMP1, OFFSET(vftbl_t, baseval));
				M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl_t, baseval));
				M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, diffval));
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
				codegen_threadcritstop(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
				M_ISUB(REG_ITMP1, REG_ITMP3, REG_ITMP1);
				M_CMPULE(REG_ITMP1, REG_ITMP2, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			}
			break;


		case ICMD_CHECKASIZE:  /* ..., size ==> ..., size                     */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_BLTZ(s1, 0);
			codegen_addxcheckarefs(cd, mcodeptr);
			break;

		case ICMD_CHECKEXCEPTION:    /* ... ==> ...                           */

			M_BEQZ(REG_RESULT, 0);
			codegen_addxexceptionrefs(cd, mcodeptr);
			break;

		case ICMD_MULTIANEWARRAY:/* ..., cnt1, [cnt2, ...] ==> ..., arrayref  */
		                      /* op1 = dimension, val.a = array descriptor    */

			/* check for negative sizes and copy sizes to stack if necessary  */

			MCODECHECK((iptr->op1 << 1) + 64);

			for (s1 = iptr->op1; --s1 >= 0; src = src->prev) {
				var_to_reg_int(s2, src, REG_ITMP1);
				M_BLTZ(s2, 0);
				codegen_addxcheckarefs(cd, mcodeptr);

				/* copy SAVEDVAR sizes to stack */

				if (src->varkind != ARGVAR) {
					M_LST(s2, REG_SP, s1 * 8);
				}
			}

			/* is patcher function set? */

			if (iptr->target) {
				codegen_addpatchref(cd, mcodeptr,
									(functionptr) iptr->target, iptr->val.a);

				if (showdisassemble)
					M_NOP;

				a = 0;

			} else {
				a = (ptrint) iptr->val.a;
			}

			/* a0 = dimension count */

			ICONST(rd->argintregs[0], iptr->op1);

			/* a1 = arraydescriptor */

			a = dseg_addaddress(cd, a);
			M_ALD(rd->argintregs[1], REG_PV, a);

			/* a2 = pointer to dimensions = stack pointer */

			M_INTMOVE(REG_SP, rd->argintregs[2]);

			a = dseg_addaddress(cd, (void *) BUILTIN_multianewarray);
			M_ALD(REG_PV, REG_PV, a);
			M_JSR(REG_RA, REG_PV);
			s1 = (s4) ((u1 *) mcodeptr - cd->mcodebase);
			if (s1 <= 32768)
				M_LDA(REG_PV, REG_RA, -s1);
			else {
				s4 ml = -s1, mh = 0;
				while (ml < -32768) { ml += 65536; mh--; }
				M_LDA(REG_PV, REG_RA, ml);
				M_LDAH(REG_PV, REG_PV, mh);
			}
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
					M_LST(s1, REG_SP, 8 * rd->interfaces[len][s2].regoff);
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

	s4 *xcodeptr = NULL;
	branchref *bref;

	for (bref = cd->xboundrefs; bref != NULL; bref = bref->next) {
		gen_resolvebranch((u1*) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1*) mcodeptr - cd->mcodebase);

		MCODECHECK(8);

		/* move index register into REG_ITMP1 */
		M_MOV(bref->reg, REG_ITMP1);
		M_LDA(REG_ITMP2_XPC, REG_PV, bref->branchpos - 4);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr - 1);

		} else {
			xcodeptr = mcodeptr;

                        a = dseg_addaddress(cd, asm_throw_and_handle_arrayindexoutofbounds_exception);
                        M_ALD(REG_PV, REG_PV, a);

                        M_JSR(REG_RA, REG_PV);

                        /* recompute pv */
                        s1 = (s4) ((u1 *) mcodeptr - cd->mcodebase);
                        if (s1 <= 32768) M_LDA(REG_PV, REG_RA, -s1);
                        else {
                                s4 ml = -s1, mh = 0;
                                while (ml < -32768) { ml += 65536; mh--; }
                                M_LDA(REG_PV, REG_RA, ml);
                                M_LDAH(REG_PV, REG_PV, mh);
                        }
		}
	}

	/* generate negative array size check stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xcheckarefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  (u1 *) xcodeptr - (u1 *) cd->mcodebase - 4);
			continue;
		}

		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(8);

		M_LDA(REG_ITMP2_XPC, REG_PV, bref->branchpos - 4);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr - 1);

		} else {
			xcodeptr = mcodeptr;

			
			a = dseg_addaddress(cd, string_java_lang_NegativeArraySizeException);
			M_ALD(REG_ITMP1_XPTR,REG_PV,a);

			a = dseg_addaddress(cd, asm_throw_and_handle_nat_exception);
			M_ALD(REG_PV, REG_PV, a);

			M_JSR(REG_RA, REG_PV);
		
			/* recompute pv */
			s1 = (s4) ((u1 *) mcodeptr - cd->mcodebase);
			if (s1 <= 32768) M_LDA(REG_PV, REG_RA, -s1);
			else {
				s4 ml = -s1, mh = 0;
				while (ml < -32768) { ml += 65536; mh--; }
				M_LDA(REG_PV, REG_RA, ml);
				M_LDAH(REG_PV, REG_PV, mh);
			}


		}
	}

	/* generate cast check stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xcastrefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  (u1 *) xcodeptr - (u1 *) cd->mcodebase - 4);
			continue;
		}

		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(8);

		M_LDA(REG_ITMP2_XPC, REG_PV, bref->branchpos - 4);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr - 1);

		} else {
			xcodeptr = mcodeptr;

			a = dseg_addaddress(cd, string_java_lang_ClassCastException);
                        M_ALD(REG_ITMP1_XPTR,REG_PV,a);

                        a = dseg_addaddress(cd, asm_throw_and_handle_nat_exception);
                        M_ALD(REG_PV, REG_PV, a);

                        M_JSR(REG_RA, REG_PV);

                        /* recompute pv */
                        s1 = (s4) ((u1 *) mcodeptr - cd->mcodebase);
                        if (s1 <= 32768) M_LDA(REG_PV, REG_RA, -s1);
                        else {
                                s4 ml = -s1, mh = 0;
                                while (ml < -32768) { ml += 65536; mh--; }
                                M_LDA(REG_PV, REG_RA, ml);
                                M_LDAH(REG_PV, REG_PV, mh);
                        }

		}
	}

	/* generate exception check stubs */

	xcodeptr = NULL;

	for (bref = cd->xexceptionrefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos,
							  bref->branchpos,
							  (u1 *) xcodeptr - (u1 *) cd->mcodebase - 4);
			continue;
		}

		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(8);

		M_LDA(REG_ITMP2_XPC, REG_PV, bref->branchpos - 4);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr - 1);

		} else {
			xcodeptr = mcodeptr;

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			M_LSUB_IMM(REG_SP, 1 * 8, REG_SP);
			M_LST(REG_ITMP2_XPC, REG_SP, 0 * 8);

			a = dseg_addaddress(cd, &builtin_get_exceptionptrptr);
			M_ALD(REG_PV, REG_PV, a);
			M_JSR(REG_RA, REG_PV);

			/* recompute pv */
			s1 = (s4) ((u1 *) mcodeptr - cd->mcodebase);
			if (s1 <= 32768) M_LDA(REG_PV, REG_RA, -s1);
			else {
				s4 ml = -s1, mh = 0;
				while (ml < -32768) { ml += 65536; mh--; }
				M_LDA(REG_PV, REG_RA, ml);
				M_LDAH(REG_PV, REG_PV, mh);
			}

			M_ALD(REG_ITMP1_XPTR, REG_RESULT, 0);
			M_AST(REG_ZERO, REG_RESULT, 0);

			M_LLD(REG_ITMP2_XPC, REG_SP, 0 * 8);
			M_LADD_IMM(REG_SP, 1 * 8, REG_SP);
#else
			a = dseg_addaddress(cd, &_exceptionptr);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_ALD(REG_ITMP1_XPTR, REG_ITMP3, 0);
			M_AST(REG_ZERO, REG_ITMP3, 0);
#endif

			a = dseg_addaddress(cd, asm_refillin_and_handle_exception);
			M_ALD(REG_PV, REG_PV, a);

			M_JMP(REG_RA, REG_PV);
		
			/* recompute pv */
			s1 = (s4) ((u1 *) mcodeptr - cd->mcodebase);
			if (s1 <= 32768) M_LDA(REG_PV, REG_RA, -s1);
			else {
				s4 ml = -s1, mh = 0;
				while (ml < -32768) { ml += 65536; mh--; }
				M_LDA(REG_PV, REG_RA, ml);
				M_LDAH(REG_PV, REG_PV, mh);
			}

		}
	}

	/* generate null pointer check stubs */

	xcodeptr = NULL;

	for (bref = cd->xnullrefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  (u1 *) xcodeptr - (u1 *) cd->mcodebase - 4);
			continue;
		}

		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(8);

		M_LDA(REG_ITMP2_XPC, REG_PV, bref->branchpos - 4);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr - 1);

		} else {
			xcodeptr = mcodeptr;

			a = dseg_addaddress(cd, string_java_lang_NullPointerException);
                        M_ALD(REG_ITMP1_XPTR,REG_PV,a);

                        a = dseg_addaddress(cd, asm_throw_and_handle_nat_exception);
                        M_ALD(REG_PV, REG_PV, a);

                        M_JSR(REG_RA, REG_PV);

                        /* recompute pv */
                        s1 = (s4) ((u1 *) mcodeptr - cd->mcodebase);
                        if (s1 <= 32768) M_LDA(REG_PV, REG_RA, -s1);
                        else {
                                s4 ml = -s1, mh = 0;
                                while (ml < -32768) { ml += 65536; mh--; }
                                M_LDA(REG_PV, REG_RA, ml);
                                M_LDAH(REG_PV, REG_PV, mh);
                        }

		}
	}

	/* generate put/getstatic stub call code */

	{
		patchref *pref;
		u4        mcode;
		s4       *tmpmcodeptr;

		for (pref = cd->patchrefs; pref != NULL; pref = pref->next) {
			/* check code segment size */

			MCODECHECK(13 + 4 + 1);

			/* Get machine code which is patched back in later. The call is   */
			/* 1 instruction word long.                                       */

			xcodeptr = (s4 *) (cd->mcodebase + pref->branchpos);
			mcode = *xcodeptr;

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			/* create a virtual java_objectheader */

			/* align data structure to 8-byte */

			ALIGNCODENOP;

			*((ptrint *) (mcodeptr + 0)) = 0;                        /* vftbl */
			*((ptrint *) (mcodeptr + 2)) = (ptrint) get_dummyLR(); /* monitorPtr */

			mcodeptr += 2 * 2;                 /* mcodeptr is a `u4*' pointer */
#endif

			/* patch in the call to call the following code (done at compile  */
			/* time)                                                          */

			tmpmcodeptr = mcodeptr;         /* save current mcodeptr          */
			mcodeptr = xcodeptr;            /* set mcodeptr to patch position */

			M_BSR(REG_ITMP3, tmpmcodeptr - (xcodeptr + 1));

			mcodeptr = tmpmcodeptr;         /* restore the current mcodeptr   */

			/* create stack frame */

			M_LSUB_IMM(REG_SP, 5 * 8, REG_SP);

			/* move return address onto stack */

			M_AST(REG_ITMP3, REG_SP, 4 * 8);

			/* move pointer to java_objectheader onto stack */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			M_BSR(REG_ITMP3, 0);
			M_LSUB_IMM(REG_ITMP3, 3 * 4 + 2 * 8, REG_ITMP3);
			M_AST(REG_ITMP3, REG_SP, 3 * 8);
#else
			M_AST(REG_ZERO, REG_SP, 3 * 8);
#endif

			/* move machine code onto stack */

			a = dseg_adds4(cd, mcode);
			M_ILD(REG_ITMP3, REG_PV, a);
			M_IST(REG_ITMP3, REG_SP, 2 * 8);

			/* move class/method/field reference onto stack */

			a = dseg_addaddress(cd, pref->ref);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_AST(REG_ITMP3, REG_SP, 1 * 8);

			/* move patcher function pointer onto stack */

			a = dseg_addaddress(cd, pref->patcher);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_AST(REG_ITMP3, REG_SP, 0 * 8);

			a = dseg_addaddress(cd, asm_wrapper_patcher);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JMP(REG_ZERO, REG_ITMP3);
		}
	}
	}

	codegen_finish(m, cd, (s4) ((u1 *) mcodeptr - cd->mcodebase));
}


/* function createcompilerstub *************************************************

	creates a stub routine which calls the compiler
	
*******************************************************************************/

#define COMPSTUBSIZE    3

u1 *createcompilerstub(methodinfo *m)
{
	u8 *s = CNEW(u8, COMPSTUBSIZE);     /* memory to hold the stub            */
	s4 *mcodeptr = (s4 *) s;            /* code generation pointer            */
	
	                                    /* code for the stub                  */
	M_ALD(REG_PV, REG_PV, 16);          /* load pointer to the compiler       */
	M_JMP(0, REG_PV);                   /* jump to the compiler, return address
	                                       in reg 0 is used as method pointer */
	s[1] = (u8) m;                      /* literals to be adressed            */  
	s[2] = (u8) asm_call_jit_compiler;  /* jump directly via PV from above    */

#if defined(STATISTICS)
	if (opt_stat)
		count_cstub_len += COMPSTUBSIZE * 8;
#endif

	return (u1 *) s;
}


/* function removecompilerstub *************************************************

     deletes a compilerstub from memory  (simply by freeing it)

*******************************************************************************/

void removecompilerstub(u1 *stub)
{
	CFREE(stub, COMPSTUBSIZE * 8);
}


/* function: createnativestub **************************************************

	creates a stub routine which calls a native method

*******************************************************************************/


#if defined(USE_THREADS) && defined(NATIVE_THREADS)
#define NATIVESTUB_STACK          8/*ra,native result, oldThreadspecificHeadValue, addressOfThreadspecificHead, method, 0,0,ra*/
#define NATIVESTUB_THREAD_EXTRA    (6 + 20) /*20 for additional frame creation*/
#define NATIVESTUB_STACKTRACE_OFFSET   1
#else
#define NATIVESTUB_STACK          7/*ra,oldThreadspecificHeadValue, addressOfThreadspecificHead, method, 0,0,ra*/
#define NATIVESTUB_THREAD_EXTRA    (1 + 20) /*20 for additional frame creation*/
#define NATIVESTUB_STACKTRACE_OFFSET   0
#endif

#define NATIVESTUB_SIZE            (44 + NATIVESTUB_THREAD_EXTRA - 1)
#define NATIVESTUB_STATIC_SIZE     5
#define NATIVESTUB_VERBOSE_SIZE    (39 + 13)
#define NATIVESTUB_OFFSET          12

#if 0
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
#define NATIVESTUB_STACK           2
#define NATIVESTUB_THREAD_EXTRA    6
#else
#define NATIVESTUB_STACK           1
#define NATIVESTUB_THREAD_EXTRA    1
#endif

#define NATIVESTUB_SIZE            (44 + NATIVESTUB_THREAD_EXTRA - 1)
#define NATIVESTUB_STATIC_SIZE     4
#define NATIVESTUB_VERBOSE_SIZE    (39 + 13)
#define NATIVESTUB_OFFSET          10
#endif

u1 *createnativestub(functionptr f, methodinfo *m)
{
	u8 *s;                              /* memory pointer to hold the stub    */
	u8 *cs;
	s4 *mcodeptr;                       /* code generation pointer            */
	s4 stackframesize = 0;              /* size of stackframe if needed       */
	s4 disp;
	s4 stubsize;
	codegendata  *cd;
	registerdata *rd;
	t_inlining_globals *id;
	s4 dumpsize;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* setup registers before using it */

	cd = DNEW(codegendata);
	rd = DNEW(registerdata);
	id = DNEW(t_inlining_globals);

	inlining_setup(m, id);
	reg_setup(m, rd, id);

	method_descriptor2types(m);                /* set paramcount and paramtypes      */

	stubsize = NATIVESTUB_SIZE;         /* calculate nativestub size          */

	if ((m->flags & ACC_STATIC) && !m->class->initialized)
		stubsize += NATIVESTUB_STATIC_SIZE;

	if (runverbose)
		stubsize += NATIVESTUB_VERBOSE_SIZE;

	s = CNEW(u8, stubsize);             /* memory to hold the stub            */
	cs = s + NATIVESTUB_OFFSET;
	mcodeptr = (s4 *) cs;               /* code generation pointer            */

	/* set some required varibles which are normally set by codegen_setup     */
	cd->mcodebase = (u1 *) mcodeptr;
	cd->patchrefs = NULL;

	*(cs-1)  = (u8) f;                  /* address of native method           */
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	*(cs-2)  = (u8) &builtin_get_exceptionptrptr;
#else
	*(cs-2)  = (u8) (&_exceptionptr);   /* address of exceptionptr            */
#endif
	*(cs-3)  = (u8) asm_handle_nat_exception; /* addr of asm exception handler*/
	*(cs-4)  = (u8) (&env);             /* addr of jni_environement           */
	*(cs-5)  = (u8) builtin_trace_args;
	*(cs-6)  = (u8) m;
	*(cs-7)  = (u8) builtin_displaymethodstop;
	*(cs-8)  = (u8) m->class;
	*(cs-9)  = (u8) asm_wrapper_patcher;
	*(cs-10) = (u8) &builtin_asm_get_stackframeinfo;
	*(cs-11) = (u8) NULL;               /* filled with machine code           */
	*(cs-12) = (u8) PATCHER_clinit;

	M_LDA(REG_SP, REG_SP, -NATIVESTUB_STACK * 8);     /* build up stackframe  */
	M_AST(REG_RA, REG_SP, 0 * 8);       /* store return address               */

	M_AST(REG_RA, REG_SP, (6+NATIVESTUB_STACKTRACE_OFFSET) * 8);       /* store return address  in stackinfo helper*/

	/* if function is static, check for initialized */

	if (m->flags & ACC_STATIC) {
	/* if class isn't yet initialized, do it */
		if (!m->class->initialized) {
			codegen_addpatchref(cd, mcodeptr, NULL, NULL);
		}
	}

	/* max. 39 +9 instructions */
	{
		s4 p;
		s4 t;
		M_LDA(REG_SP, REG_SP, -((INT_ARG_CNT + FLT_ARG_CNT + 2) * 8));
		M_AST(REG_RA, REG_SP, 1 * 8);

		/* save integer argument registers */
		for (p = 0; p < m->paramcount && p < INT_ARG_CNT; p++) {
			M_LST(rd->argintregs[p], REG_SP, (2 + p) * 8);
		}

		/* save and copy float arguments into integer registers */
		for (p = 0; p < m->paramcount && p < FLT_ARG_CNT; p++) {
			t = m->paramtypes[p];

			if (IS_FLT_DBL_TYPE(t)) {
				if (IS_2_WORD_TYPE(t)) {
					M_DST(rd->argfltregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);
					M_LLD(rd->argintregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);

				} else {
					M_FST(rd->argfltregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);
					M_ILD(rd->argintregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);
				}
				
			} else {
				M_DST(rd->argfltregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);
			}
		}

		if (runverbose) {
			M_ALD(REG_ITMP1, REG_PV, -6 * 8);
			M_AST(REG_ITMP1, REG_SP, 0 * 8);
			M_ALD(REG_PV, REG_PV, -5 * 8);
			M_JSR(REG_RA, REG_PV);
			disp = -(s4) (mcodeptr - (s4 *) cs) * 4;
			M_LDA(REG_PV, REG_RA, disp);
		}


/*stack info */
                M_ALD(REG_PV, REG_PV, -10 * 8);      /* builtin_asm_get_stackframeinfo        */
                M_JSR(REG_RA, REG_PV);
                disp = -(s4) (mcodeptr - (s4 *) cs) * 4;
                M_LDA(REG_PV, REG_RA, disp);

#if 0
                M_MOV(REG_RESULT,REG_ITMP3);
                M_LST(REG_RESULT,REG_ITMP3,0);
#endif
                M_LST(REG_RESULT,REG_SP, ((INT_ARG_CNT + FLT_ARG_CNT + 2) * 8)+(2+NATIVESTUB_STACKTRACE_OFFSET)*8);/*save adress of pointer*/
                M_LLD(REG_ITMP3,REG_RESULT,0); /* get pointer*/
                M_LST(REG_ITMP3,REG_SP,(1+NATIVESTUB_STACKTRACE_OFFSET)*8+((INT_ARG_CNT + FLT_ARG_CNT + 2) * 8)); /*save old value*/
                M_LDA(REG_ITMP3,REG_SP,(1+NATIVESTUB_STACKTRACE_OFFSET)*8+((INT_ARG_CNT + FLT_ARG_CNT + 2) * 8)); /*calculate new value*/
                M_LLD(REG_ITMP2,REG_ITMP3,8);
                M_LST(REG_ITMP3,REG_ITMP2,0); /*store new value*/
                M_LLD(REG_ITMP2,REG_PV,-6*8);
                M_LST(REG_ITMP2,REG_SP,(3+NATIVESTUB_STACKTRACE_OFFSET)*8+((INT_ARG_CNT + FLT_ARG_CNT + 2) * 8));
                M_LST(REG_ZERO,REG_SP,(4+NATIVESTUB_STACKTRACE_OFFSET)*8+((INT_ARG_CNT + FLT_ARG_CNT + 2) * 8));
                M_LST(REG_ZERO,REG_SP,(5+NATIVESTUB_STACKTRACE_OFFSET)*8+((INT_ARG_CNT + FLT_ARG_CNT + 2) * 8));
/*stack info -end */


		for (p = 0; p < m->paramcount && p < INT_ARG_CNT; p++) {
			M_LLD(rd->argintregs[p], REG_SP, (2 + p) * 8);
		}

		for (p = 0; p < m->paramcount && p < FLT_ARG_CNT; p++) {
			t = m->paramtypes[p];

			if (IS_FLT_DBL_TYPE(t)) {
				if (IS_2_WORD_TYPE(t)) {
					M_DLD(rd->argfltregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);

				} else {
					M_FLD(rd->argfltregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);
				}

			} else {
				M_DLD(rd->argfltregs[p], REG_SP, (2 + INT_ARG_CNT + p) * 8);
			}
		}

		M_ALD(REG_RA, REG_SP, 1 * 8);
		M_LDA(REG_SP, REG_SP, (INT_ARG_CNT + FLT_ARG_CNT + 2) * 8);
	}

	/* save argument registers on stack -- if we have to */
	if ((m->flags & ACC_STATIC && m->paramcount > (INT_ARG_CNT - 2)) || m->paramcount > (INT_ARG_CNT - 1)) {
		s4 i;
		s4 paramshiftcnt = (m->flags & ACC_STATIC) ? 2 : 1;
		s4 stackparamcnt = (m->paramcount > INT_ARG_CNT) ? m->paramcount - INT_ARG_CNT : 0;

  		stackframesize = stackparamcnt + paramshiftcnt;

		M_LDA(REG_SP, REG_SP, -stackframesize * 8);

		/* copy stack arguments into new stack frame -- if any */
		for (i = 0; i < stackparamcnt; i++) {
			M_LLD(REG_ITMP1, REG_SP, (stackparamcnt + 1 + i) * 8);
			M_LST(REG_ITMP1, REG_SP, (paramshiftcnt + i) * 8);
		}

		if (m->flags & ACC_STATIC) {
			if (IS_FLT_DBL_TYPE(m->paramtypes[5])) {
				M_DST(rd->argfltregs[5], REG_SP, 1 * 8);
			} else {
				M_LST(rd->argintregs[5], REG_SP, 1 * 8);
			}

			if (IS_FLT_DBL_TYPE(m->paramtypes[4])) {
				M_DST(rd->argfltregs[4], REG_SP, 0 * 8);
			} else {
				M_LST(rd->argintregs[4], REG_SP, 0 * 8);
			}

		} else {
			if (IS_FLT_DBL_TYPE(m->paramtypes[5])) {
				M_DST(rd->argfltregs[5], REG_SP, 0 * 8);
			} else {
				M_LST(rd->argintregs[5], REG_SP, 0 * 8);
			}
		}
	}
#if 1
	{
		s4 i;

		if (m->flags & ACC_STATIC) {
			/* shift iargs count if less than INT_ARG_CNT, or all */
			for (i = (m->paramcount < (INT_ARG_CNT - 2)) ? m->paramcount : (INT_ARG_CNT - 2); i >= 0; i--) {
				M_MOV(rd->argintregs[i], rd->argintregs[i + 2]);
				M_FMOV(rd->argfltregs[i], rd->argfltregs[i + 2]);
			}

			/* put class into second argument register */
			M_ALD(rd->argintregs[1], REG_PV, -8 * 8);

		} else {
			/* shift iargs count if less than INT_ARG_CNT, or all */
			for (i = (m->paramcount < (INT_ARG_CNT - 1)) ? m->paramcount : (INT_ARG_CNT - 1); i >= 0; i--) {
				M_MOV(rd->argintregs[i], rd->argintregs[i + 1]);
				M_FMOV(rd->argfltregs[i], rd->argfltregs[i + 1]);
			}
		}
	}
#else
	if (m->flags & ACC_STATIC) {
		M_MOV(rd->argintregs[3], rd->argintregs[5]);
		M_MOV(rd->argintregs[2], rd->argintregs[4]);
		M_MOV(rd->argintregs[1], rd->argintregs[3]);
		M_MOV(rd->argintregs[0], rd->argintregs[2]);
		M_FMOV(rd->argfltregs[3], rd->argfltregs[5]);
		M_FMOV(rd->argfltregs[2], rd->argfltregs[4]);
		M_FMOV(rd->argfltregs[1], rd->argfltregs[3]);
		M_FMOV(rd->argfltregs[0], rd->argfltregs[2]);

		/* put class into second argument register */
		M_ALD(rd->argintregs[1], REG_PV, -8 * 8);

	} else {
		M_MOV(rd->argintregs[4], rd->argintregs[5]);
		M_MOV(rd->argintregs[3], rd->argintregs[4]);
		M_MOV(rd->argintregs[2], rd->argintregs[3]);
		M_MOV(rd->argintregs[1], rd->argintregs[2]);
		M_MOV(rd->argintregs[0], rd->argintregs[1]);
		M_FMOV(rd->argfltregs[4], rd->argfltregs[5]);
		M_FMOV(rd->argfltregs[3], rd->argfltregs[4]);
		M_FMOV(rd->argfltregs[2], rd->argfltregs[3]);
		M_FMOV(rd->argfltregs[1], rd->argfltregs[2]);
		M_FMOV(rd->argfltregs[0], rd->argfltregs[1]);
	}
#endif

	/* put env into first argument register */
	M_ALD(rd->argintregs[0], REG_PV, -4 * 8);

	M_ALD(REG_PV, REG_PV, -1 * 8);      /* load adress of native method       */
	M_JSR(REG_RA, REG_PV);              /* call native method                 */
	disp = -(s4) (mcodeptr - (s4 *) cs) * 4;
	M_LDA(REG_PV, REG_RA, disp);        /* recompute pv from ra               */

	/* remove stackframe if there is one */
	if (stackframesize) {
		M_LDA(REG_SP, REG_SP, stackframesize * 8);
	}

	/* 13 instructions */
	if (runverbose) {
		M_LDA(REG_SP, REG_SP, -2 * 8);
		M_ALD(rd->argintregs[0], REG_PV, -6 * 8); /* load method adress       */
		M_LST(REG_RESULT, REG_SP, 0 * 8);
		M_DST(REG_FRESULT, REG_SP, 1 * 8);
		M_MOV(REG_RESULT, rd->argintregs[1]);
		M_FMOV(REG_FRESULT, rd->argfltregs[2]);
		M_FMOV(REG_FRESULT, rd->argfltregs[3]);
		M_ALD(REG_PV, REG_PV, -7 * 8);  /* builtin_displaymethodstop          */
		M_JSR(REG_RA, REG_PV);
		disp = -(s4) (mcodeptr - (s4 *) cs) * 4;
		M_LDA(REG_PV, REG_RA, disp);
		M_LLD(REG_RESULT, REG_SP, 0 * 8);
		M_DLD(REG_FRESULT, REG_SP, 1 * 8);
		M_LDA(REG_SP, REG_SP, 2 * 8);
	}

        M_LLD(REG_ITMP3,REG_SP,(2+NATIVESTUB_STACKTRACE_OFFSET)*8); /*get address of stacktrace helper pointer*/
        M_LLD(REG_ITMP1,REG_SP,(1+NATIVESTUB_STACKTRACE_OFFSET)*8); /*get old value*/
        M_LST(REG_ITMP1,REG_ITMP3,0); /*set old value*/

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	if (IS_FLT_DBL_TYPE(m->returntype))
		M_DST(REG_FRESULT, REG_SP, 1 * 8);
	else
		M_AST(REG_RESULT, REG_SP, 1 * 8);
	M_ALD(REG_PV, REG_PV, -2 * 8);      /* builtin_get_exceptionptrptr        */
	M_JSR(REG_RA, REG_PV);
	disp = -(s4) (mcodeptr - (s4 *) cs) * 4;
	M_LDA(REG_PV, REG_RA, disp);
	M_MOV(REG_RESULT, REG_ITMP3);
	if (IS_FLT_DBL_TYPE(m->returntype))
		M_DLD(REG_FRESULT, REG_SP, 1 * 8);
	else
		M_ALD(REG_RESULT, REG_SP, 1 * 8);
#else
	M_ALD(REG_ITMP3, REG_PV, -2 * 8);   /* get address of exceptionptr        */
#endif
	M_ALD(REG_ITMP1, REG_ITMP3, 0);     /* load exception into reg. itmp1     */
	M_BNEZ(REG_ITMP1, 3);               /* if no exception then return        */

	M_ALD(REG_RA, REG_SP, 0 * 8);       /* load return address                */
	M_LDA(REG_SP, REG_SP, NATIVESTUB_STACK * 8); /* remove stackframe         */
	M_RET(REG_ZERO, REG_RA);            /* return to caller                   */

	M_AST(REG_ZERO, REG_ITMP3, 0);      /* store NULL into exceptionptr       */

	M_ALD(REG_RA, REG_SP, 0 * 8);       /* load return address                */
	M_LDA(REG_SP, REG_SP, NATIVESTUB_STACK * 8); /* remove stackframe         */
	M_LDA(REG_ITMP2, REG_RA, -4);       /* move fault address into reg. itmp2 */
	M_ALD(REG_ITMP3, REG_PV, -3 * 8);   /* load asm exception handler address */
	M_JMP(REG_ZERO, REG_ITMP3);         /* jump to asm exception handler      */
	
	/* generate put/getstatic stub call code */

	{
		patchref *pref;
		s4       *xcodeptr;
		s4       *tmpmcodeptr;

		/* there can only be one <clinit> ref entry                           */
		pref = cd->patchrefs;

		if (pref) {
			/* Get machine code which is patched back in later. The call is   */
			/* 1 instruction word long.                                       */

			xcodeptr = (s4 *) (cd->mcodebase + pref->branchpos);
			*(cs-11) = (u4) *xcodeptr;

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			/* create a virtual java_objectheader */

			/* align data structure to 8-byte */

			ALIGNCODENOP;

			*((ptrint *) (mcodeptr + 0)) = 0;                        /* vftbl */
			*((ptrint *) (mcodeptr + 2)) = (ptrint) get_dummyLR(); /* monitorPtr */

			mcodeptr += 2 * 2;                 /* mcodeptr is a `u4*' pointer */
#endif

			/* patch in the call to call the following code (done at compile  */
			/* time)                                                          */

			tmpmcodeptr = mcodeptr;         /* save current mcodeptr          */
			mcodeptr = xcodeptr;            /* set mcodeptr to patch position */

			M_BSR(REG_ITMP3, tmpmcodeptr - (xcodeptr + 1));

			mcodeptr = tmpmcodeptr;         /* restore the current mcodeptr   */

			/* create stack frame                                             */

			M_LSUB_IMM(REG_SP, 5 * 8, REG_SP);

			/* move return address onto stack */

			M_AST(REG_ITMP3, REG_SP, 4 * 8);

			/* move pointer to java_objectheader onto stack */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			M_BSR(REG_ITMP3, 0);
			M_LSUB_IMM(REG_ITMP3, 3 * 4 + 2 * 8, REG_ITMP3);
			M_AST(REG_ITMP3, REG_SP, 3 * 8);
#else
			M_AST(REG_ZERO, REG_SP, 3 * 8);
#endif

			/* move machine code onto stack                                   */

			M_ILD(REG_ITMP3, REG_PV, -11 * 8);    /* machine code             */
			M_IST(REG_ITMP3, REG_SP, 2 * 8);

			/* move class reference onto stack                                */

			M_ALD(REG_ITMP3, REG_PV, -8 * 8);     /* class                    */
			M_AST(REG_ITMP3, REG_SP, 1 * 8);

			/* move patcher function pointer onto stack                       */

			M_ALD(REG_ITMP3, REG_PV, -12 * 8);    /* patcher function         */
			M_AST(REG_ITMP3, REG_SP, 0 * 8);

			M_ALD(REG_ITMP3, REG_PV, -9 * 8);     /* asm_wrapper_patcher      */
			M_JMP(REG_ZERO, REG_ITMP3);
		}
	}

	/* Check if the stub size is big enough to hold the whole stub generated. */
	/* If not, this can lead into unpredictable crashes, because of heap      */
	/* corruption.                                                            */
	if ((s4) ((ptrint) mcodeptr - (ptrint) s) > stubsize * sizeof(u8)) {
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Native stub size %d is to small for current stub size %d",
								   stubsize, (s4) ((ptrint) mcodeptr - (ptrint) s));
	}

#if defined(STATISTICS)
	if (opt_stat)
		count_nstub_len += NATIVESTUB_SIZE * 8;
#endif

	/* release dump area */

	dump_release(dumpsize);

	return (u1 *) (s + NATIVESTUB_OFFSET);
}


/* function: removenativestub **************************************************

    removes a previously created native-stub from memory
    
*******************************************************************************/

void removenativestub(u1 *stub)
{
	CFREE((u8 *) stub - NATIVESTUBOFFSET, NATIVESTUBSIZE * 8);
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
