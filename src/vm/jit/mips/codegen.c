/* src/vm/jit/mips/codegen.c - machine code generator for MIPS

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

   Changes: Christian Thalinger

   Contains the codegenerator for an MIPS (R4000 or higher) processor.
   This module generates MIPS machine code for a sequence of
   intermediate code commands (ICMDs).

   $Id: codegen.c 2037 2005-03-19 15:57:50Z twisti $

*/


#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>

#include "config.h"
#include "cacao/cacao.h"
#include "native/native.h"
#include "vm/builtin.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/jit.h"
#include "vm/jit/reg.h"
#include "vm/jit/mips/codegen.h"
#include "vm/jit/mips/types.h"


/* *****************************************************************************

Datatypes and Register Allocations:
----------------------------------- 

On 64-bit-machines (like the MIPS) all operands are stored in the
registers in a 64-bit form, even when the correspondig JavaVM operands
only need 32 bits. This is done by a canonical representation:

32-bit integers are allways stored as sign-extended 64-bit values (this
approach is directly supported by the MIPS architecture and is very easy
to implement).

32-bit-floats are stored in a 64-bit double precision register by simply
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
 
static int nregdescint[] = {
	REG_RES, REG_RES, REG_RET, REG_RES, REG_ARG, REG_ARG, REG_ARG, REG_ARG, 
	REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_TMP, REG_TMP, REG_TMP, REG_TMP, 
	REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV,
	REG_TMP, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES,
	REG_END };

/* for use of reserved registers, see comment above */
	
static int nregdescfloat[] = {
	REG_RET, REG_RES, REG_RES, REG_RES, REG_TMP, REG_TMP, REG_TMP, REG_TMP,
	REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_ARG, REG_ARG, REG_ARG, REG_ARG, 
	REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_TMP, REG_TMP, REG_TMP, REG_TMP,
	REG_SAV, REG_TMP, REG_SAV, REG_TMP, REG_SAV, REG_TMP, REG_SAV, REG_TMP,
	REG_END };

/* for use of reserved registers, see comment above */


/* Include independent code generation stuff -- include after register        */
/* descriptions to avoid extern definitions.                                  */

#include "vm/jit/codegen.inc"
#include "vm/jit/reg.inc"
#ifdef LSRA
#include "vm/jit/lsra.inc"
#endif


/* NullPointerException handlers and exception handling initialisation        */

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void thread_restartcriticalsection(ucontext_t *uc)
{
	void *critical;
	if ((critical = thread_checkcritical((void*) uc->uc_mcontext.gregs[CTX_EPC])) != NULL)
		uc->uc_mcontext.gregs[CTX_EPC] = (u8) critical;
}
#endif

/* NullPointerException signal handler for hardware null pointer check */

void catch_NullPointerException(int sig, siginfo_t *siginfo, void *_p)
{
	sigset_t nsig;
	int      instr;
	long     faultaddr;
	java_objectheader *xptr;

	struct ucontext *_uc = (struct ucontext *) _p;
	mcontext_t *sigctx = &_uc->uc_mcontext;
	struct sigaction act;

	instr = *((s4 *) (sigctx->gregs[CTX_EPC]));
	faultaddr = sigctx->gregs[(instr >> 21) & 0x1f];

	if (faultaddr == 0) {
		/* Reset signal handler - necessary for SysV, does no harm for BSD */

		act.sa_sigaction = catch_NullPointerException;
		act.sa_flags = SA_SIGINFO;
		sigaction(sig, &act, NULL);

		sigemptyset(&nsig);
		sigaddset(&nsig, sig);
		sigprocmask(SIG_UNBLOCK, &nsig, NULL);           /* unblock signal    */
		
		xptr = new_nullpointerexception();

		sigctx->gregs[REG_ITMP1_XPTR] = (u8) xptr;
		sigctx->gregs[REG_ITMP2_XPC] = sigctx->gregs[CTX_EPC];
		sigctx->gregs[CTX_EPC] = (u8) asm_handle_exception;

	} else {
        faultaddr += (long) ((instr << 16) >> 16);
		fprintf(stderr, "faulting address: 0x%lx at 0x%lx\n", (long) faultaddr, (long) sigctx->gregs[CTX_EPC]);
		panic("Stack overflow");
	}
}


#include <sys/fpu.h>

void init_exceptions(void)
{
	struct sigaction act;

	/* The Boehm GC initialization blocks the SIGSEGV signal. So we do a
	   dummy allocation here to ensure that the GC is initialized.
	*/
	heap_allocate(1, 0, NULL);

	/* install signal handlers we need to convert to exceptions */

	sigemptyset(&act.sa_mask);

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

	/* Turn off flush-to-zero */
	{
		union fpc_csr n;
		n.fc_word = get_fpc_csr();
		n.fc_struct.flush = 0;
		set_fpc_csr(n.fc_word);
	}
}


/* function gen_mcode **********************************************************

	generates machine code

*******************************************************************************/

void codegen(methodinfo *m, codegendata *cd, registerdata *rd)
{
	s4 len, s1, s2, s3, d;
	s4 a;
	s4 parentargs_base;
	s4             *mcodeptr;
	stackptr        src;
	varinfo        *var;
	basicblock     *bptr;
	instruction    *iptr;
	exceptiontable *ex;

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

	/* adjust frame size for 16 byte alignment */

	if (parentargs_base & 1)
		parentargs_base++;

	/* create method header */

#if POINTERSIZE == 4
	(void) dseg_addaddress(cd, m);                          /* Filler         */
#endif
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
	(void) dseg_adds4(cd, cd->exceptiontablelength);        /* ExTableSize    */

	/* create exception table */

	for (ex = cd->exceptiontable; ex != NULL; ex = ex->down) {
		dseg_addtarget(cd, ex->start);
   		dseg_addtarget(cd, ex->end);
		dseg_addtarget(cd, ex->handler);
		(void) dseg_addaddress(cd, ex->catchtype);
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
		p--; M_LST(REG_RA, REG_SP, 8 * p);
	}
	for (i = rd->savintregcnt - 1; i >= rd->maxsavintreguse; i--) {
		p--; M_LST(rd->savintregs[i], REG_SP, 8 * p);
	}
	for (i = rd->savfltregcnt - 1; i >= rd->maxsavfltreguse; i--) {
		p--; M_DST(rd->savfltregs[i], REG_SP, 8 * p);
	}

	/* save monitorenter argument */

#if defined(USE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		if (m->flags & ACC_STATIC) {
			p = dseg_addaddress(cd, m->class);
			M_ALD(REG_ITMP1, REG_PV, p);
			M_AST(REG_ITMP1, REG_SP, rd->maxmemuse * 8);

		} else {
			M_AST(rd->argintregs[0], REG_SP, rd->maxmemuse * 8);
		}
	}			
#endif

	/* copy argument registers to stack and call trace function with pointer
	   to arguments on stack. ToDo: save floating point registers !!!!!!!!!
	*/

	if (runverbose) {
		M_LDA(REG_SP, REG_SP, -(18 * 8));
		M_LST(REG_RA, REG_SP, 1 * 8);

		/* save integer argument registers */
		for (p = 0; p < m->paramcount && p < INT_ARG_CNT; p++) {
			M_LST(rd->argintregs[p], REG_SP,  (2 + p) * 8);
		}

		/* save and copy float arguments into integer registers */
		for (p = 0; p < m->paramcount && p < FLT_ARG_CNT; p++) {
			t = m->paramtypes[p];

			if (IS_FLT_DBL_TYPE(t)) {
				if (IS_2_WORD_TYPE(t)) {
					M_DST(rd->argfltregs[p], REG_SP, (10 + p) * 8);
					M_LLD(rd->argintregs[p], REG_SP, (10 + p) * 8);

				} else {
					M_FST(rd->argfltregs[p], REG_SP, (10 + p) * 8);
					M_ILD(rd->argintregs[p], REG_SP, (10 + p) * 8);
				}

			} else {
				M_DST(rd->argfltregs[p], REG_SP, (10 + p) * 8);
			}
		}

		p = dseg_addaddress(cd, m);
		M_ALD(REG_ITMP1, REG_PV, p);
		M_LST(REG_ITMP1, REG_SP, 0);
		p = dseg_addaddress(cd, (void *) builtin_trace_args);
		M_ALD(REG_ITMP3, REG_PV, p);
		M_JSR(REG_RA, REG_ITMP3);
		M_NOP;

		M_LLD(REG_RA, REG_SP, 1 * 8);

		for (p = 0; p < m->paramcount && p < INT_ARG_CNT; p++) {
			M_LLD(rd->argintregs[p], REG_SP, (2 + p) * 8);
		}

		for (p = 0; p < m->paramcount && p < FLT_ARG_CNT; p++) {
			t = m->paramtypes[p];

			if (IS_FLT_DBL_TYPE(t)) {
				if (IS_2_WORD_TYPE(t)) {
					M_DLD(rd->argfltregs[p], REG_SP, (10 + p) * 8);

				} else {
					M_FLD(rd->argfltregs[p], REG_SP, (10 + p) * 8);
				}

			} else {
				M_DLD(rd->argfltregs[p], REG_SP, (10 + p) * 8);
			}
		}

		M_LDA(REG_SP, REG_SP, 18 * 8);
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
 					M_LLD(REG_ITMP1, REG_SP, 8 * (parentargs_base + pa));
 					M_LST(REG_ITMP1, REG_SP, 8 * var->regoff);
				}
			}

 		} else {                                     /* floating args         */
 			if (p < FLT_ARG_CNT) {                   /* register arguments    */
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
 					M_TFLTMOVE(var->type, rd->argfltregs[p], var->regoff);

 				} else {			                 /* reg arg -> spilled    */
 					M_DST(rd->argfltregs[p], REG_SP, var->regoff * 8);
 				}

 			} else {                                 /* stack arguments       */
 				pa = p - FLT_ARG_CNT;
 				if (!(var->flags & INMEMORY)) {      /* stack-arg -> register */
 					M_DLD(var->regoff, REG_SP, 8 * (parentargs_base + pa));

				} else {                             /* stack-arg -> spilled  */
 					M_DLD(REG_FTMP1, REG_SP, 8 * (parentargs_base + pa));
 					M_DST(REG_FTMP1, REG_SP, 8 * var->regoff);
				}
			}
		}
	} /* end for */

	/* call monitorenter function */

#if defined(USE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		s4 disp;
		s8 func_enter = (m->flags & ACC_STATIC) ?
			(s8) builtin_staticmonitorenter : (s8) builtin_monitorenter;
		p = dseg_addaddress(cd, (void *) func_enter);
		M_ALD(REG_ITMP3, REG_PV, p);
		M_JSR(REG_RA, REG_ITMP3);
		M_ALD(rd->argintregs[0], REG_SP, rd->maxmemuse * 8);
		disp = -(s4) ((u1 *) mcodeptr - cd->mcodebase);
		M_LDA(REG_PV, REG_RA, disp);
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
				branchref *bref;
				for (bref = bptr->branchrefs; bref != NULL; bref = bref->next) {
					gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
									  bref->branchpos,
									  bptr->mpc);
				}
			}

		/* copy interface registers to their destination */

		src = bptr->instack;
		len = bptr->indepth;
		MCODECHECK(64+len);
		while (src != NULL) {
			len--;
			if ((len == 0) && (bptr->type != BBTYPE_STD)) {
				d = reg_of_var(rd, src, REG_ITMP1);
				M_INTMOVE(REG_ITMP1, d);
				store_reg_to_var_int(src, d);

			} else {
				d = reg_of_var(rd, src, REG_IFTMP);
				if ((src->varkind != STACKVAR)) {
					s2 = src->type;
					if (IS_FLT_DBL_TYPE(s2)) {
						if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
							s1 = rd->interfaces[len][s2].regoff;
							M_TFLTMOVE(s2, s1, d);

						} else {
							M_DLD(d, REG_SP, 8 * rd->interfaces[len][s2].regoff);
						}
						store_reg_to_var_flt(src, d);

					} else {
						if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
							s1 = rd->interfaces[len][s2].regoff;
							M_INTMOVE(s1,d);

						} else {
							M_LLD(d, REG_SP, 8 * rd->interfaces[len][s2].regoff);
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

	MCODECHECK(64);           /* an instruction usually needs < 64 words      */
	switch (iptr->opc) {

		case ICMD_NOP:        /* ...  ==> ...                                 */
			break;

		case ICMD_NULLCHECKPOP: /* ..., objectref  ==> ...                    */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_BEQZ(s1, 0);
			codegen_addxnullrefs(cd, mcodeptr);
			M_NOP;
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
			store_reg_to_var_flt (iptr->dst, d);
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
			if (var->flags & INMEMORY) {
				M_LLD(d, REG_SP, 8 * var->regoff);
			} else {
				M_INTMOVE(var->regoff,d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FLOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_DLOAD:      /* op1 = local variable                         */

			d = reg_of_var(rd, iptr->dst, REG_FTMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1))
				break;
			var = &(rd->locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			{
				int t2 = ((iptr->opc == ICMD_FLOAD) ? TYPE_FLT : TYPE_DBL);
				if (var->flags & INMEMORY) {
					M_CCFLD(var->type, t2, d, REG_SP, 8 * var->regoff);
				} else {
					M_CCFLTMOVE(var->type, t2, var->regoff, d);
				}
			}
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

			} else {
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
			{
				int t1 = ((iptr->opc == ICMD_FSTORE) ? TYPE_FLT : TYPE_DBL);
				if (var->flags & INMEMORY) {
					var_to_reg_flt(s1, src, REG_FTMP1);
					M_CCFST(t1, var->type, s1, REG_SP, 8 * var->regoff);

				} else {
					var_to_reg_flt(s1, src, var->regoff);
					M_CCFLTMOVE(t1, var->type, s1, var->regoff);
				}
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
			M_ISLL_IMM(s1, 0, d );
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_LSLL_IMM(s1, 56, d);
			M_LSRA_IMM( d, 56, d);
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
			M_LSLL_IMM(s1, 48, d);
			M_LSRA_IMM( d, 48, d);
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
			if ((iptr->val.i >= -32768) && (iptr->val.i <= 32767)) {
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
			if ((iptr->val.l >= -32768) && (iptr->val.l <= 32767)) {
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
			if ((iptr->val.i >= -32767) && (iptr->val.i <= 32768)) {
				M_IADD_IMM(s1, -iptr->val.i, d);
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
			if ((iptr->val.l >= -32767) && (iptr->val.l <= 32768)) {
				M_LADD_IMM(s1, -iptr->val.l, d);
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
			M_IMUL(s1, s2);
			M_MFLO(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			ICONST(REG_ITMP2, iptr->val.i);
			M_IMUL(s1, REG_ITMP2);
			M_MFLO(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_LMUL(s1, s2);
			M_MFLO(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			LCONST(REG_ITMP2, iptr->val.l);
			M_LMUL(s1, REG_ITMP2);
			M_MFLO(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_IDIV(s1, s2);
			M_MFLO(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;
#if 0
		case ICMD_IDIVCONST:  /* ..., value  ==> ..., value / constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			ICONST(REG_ITMP2, iptr->val.i);
			M_IDIV(s1, REG_ITMP2);
			M_MFLO(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;
#endif
		case ICMD_LDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_LDIV(s1, s2);
			M_MFLO(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;
#if 0
		case ICMD_LDIVCONST:  /* ..., value  ==> ..., value / constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			LCONST(REG_ITMP2, iptr->val.l);
			M_LDIV(s1, REG_ITMP2);
			M_MFLO(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;
#endif
		case ICMD_IREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_IDIV(s1, s2);
			M_MFHI(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;
#if 0
		case ICMD_IREMCONST:  /* ..., value  ==> ..., value % constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			ICONST(REG_ITMP2, iptr->val.i);
			M_IDIV(s1, REG_ITMP2);
			M_MFHI(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;
#endif
		case ICMD_LREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_LDIV(s1, s2);
			M_MFHI(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;
#if 0
		case ICMD_LREMCONST:  /* ..., value  ==> ..., value % constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			LCONST(REG_ITMP2, iptr->val.l);
			M_LDIV(s1, REG_ITMP2);
			M_MFHI(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;
#endif
		case ICMD_IDIVPOW2:   /* ..., value  ==> ..., value << constant       */
		case ICMD_LDIVPOW2:   /* val.i = constant                             */
		                      
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_LSRA_IMM(s1, 63, REG_ITMP2);
			M_LSRL_IMM(REG_ITMP2, 64 - iptr->val.i, REG_ITMP2);
			M_LADD(s1, REG_ITMP2, REG_ITMP2);
			M_LSRA_IMM(REG_ITMP2, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_ISLL(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_ISLL_IMM(s1, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_ISRA(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_ISRA_IMM(s1, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_ISRL(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_ISRL_IMM(s1, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_LSLL(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_LSLL_IMM(s1, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_LSRA(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_LSRA_IMM(s1, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_LSRL(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_LSRL_IMM(s1, iptr->val.i, d);
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
			if ((iptr->val.i >= 0) && (iptr->val.i <= 0xffff)) {
				M_AND_IMM(s1, iptr->val.i, d);
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
			if ((iptr->val.i >= 0) && (iptr->val.i <= 0xffff)) {
				M_AND_IMM(s1, iptr->val.i, d);
				M_BGEZ(s1, 4);
				M_NOP;
				M_ISUB(REG_ZERO, s1, d);
				M_AND_IMM(d, iptr->val.i, d);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_AND(s1, REG_ITMP2, d);
				M_BGEZ(s1, 4);
				M_NOP;
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
			if ((iptr->val.l >= 0) && (iptr->val.l <= 0xffff)) {
				M_AND_IMM(s1, iptr->val.l, d);
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
			if ((iptr->val.l >= 0) && (iptr->val.l <= 0xffff)) {
				M_AND_IMM(s1, iptr->val.l, d);
				M_BGEZ(s1, 4);
				M_NOP;
				M_LSUB(REG_ZERO, s1, d);
				M_AND_IMM(d, iptr->val.l, d);
				}
			else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_AND(s1, REG_ITMP2, d);
				M_BGEZ(s1, 4);
				M_NOP;
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
			if ((iptr->val.i >= 0) && (iptr->val.i <= 0xffff)) {
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
			if ((iptr->val.l >= 0) && (iptr->val.l <= 0xffff)) {
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
			if ((iptr->val.i >= 0) && (iptr->val.i <= 0xffff)) {
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
			if ((iptr->val.l >= 0) && (iptr->val.l <= 0xffff)) {
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

			} else
				s1 = var->regoff;
			M_IADD_IMM(s1, iptr->val.i, s1);
			if (var->flags & INMEMORY)
				M_LST(s1, REG_SP, 8 * var->regoff);
			break;


		/* floating operations ************************************************/

		case ICMD_FNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_FNEG(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_DNEG(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_FADD(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_DADD(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_FSUB(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_DSUB(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_FMUL(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DMUL:       /* ..., val1, val2  ==> ..., val1 *** val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_DMUL(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_FDIV(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_DDIV(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
		
		case ICMD_FREM:       /* ..., val1, val2  ==> ..., val1 % val2        */
			panic("FREM");

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_FDIV(s1,s2, REG_FTMP3);
			M_FLOORFL(REG_FTMP3, REG_FTMP3);
			M_CVTLF(REG_FTMP3, REG_FTMP3);
			M_FMUL(REG_FTMP3, s2, REG_FTMP3);
			M_FSUB(s1, REG_FTMP3, d);
			store_reg_to_var_flt(iptr->dst, d);
		    break;

		case ICMD_DREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_DDIV(s1,s2, REG_FTMP3);
			M_FLOORDL(REG_FTMP3, REG_FTMP3);
			M_CVTLD(REG_FTMP3, REG_FTMP3);
			M_DMUL(REG_FTMP3, s2, REG_FTMP3);
			M_DSUB(s1, REG_FTMP3, d);
			store_reg_to_var_flt(iptr->dst, d);
		    break;

		case ICMD_I2F:       /* ..., value  ==> ..., (float) value            */
		case ICMD_L2F:
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_MOVLD(s1, d);
			M_CVTLF(d, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_I2D:       /* ..., value  ==> ..., (double) value           */
		case ICMD_L2D:
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_MOVLD(s1, d);
			M_CVTLD(d, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
			
		case ICMD_F2I:       /* ..., (float) value  ==> ..., (int) value      */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_TRUNCFI(s1, REG_FTMP1);
			M_MOVDI(REG_FTMP1, d);
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;
		
		case ICMD_D2I:       /* ..., (double) value  ==> ..., (int) value     */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_TRUNCDI(s1, REG_FTMP1);
			M_MOVDI(REG_FTMP1, d);
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;
		
		case ICMD_F2L:       /* ..., (float) value  ==> ..., (long) value     */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_TRUNCFL(s1, REG_FTMP1);
			M_MOVDL(REG_FTMP1, d);
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_D2L:       /* ..., (double) value  ==> ..., (long) value    */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_TRUNCDL(s1, REG_FTMP1);
			M_MOVDL(REG_FTMP1, d);
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_F2D:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_CVTFD(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
					
		case ICMD_D2F:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(rd, iptr->dst, REG_FTMP3);
			M_CVTDF(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
		
		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_FCMPULEF(s1, s2);
			M_FBT(3);
			M_LADD_IMM(REG_ZERO, 1, d);
			M_BR(4);
			M_NOP;
			M_FCMPEQF(s1, s2);
			M_LSUB_IMM(REG_ZERO, 1, d);
			M_CMOVT(REG_ZERO, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_DCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_FCMPULED(s1, s2);
			M_FBT(3);
			M_LADD_IMM(REG_ZERO, 1, d);
			M_BR(4);
			M_NOP;
			M_FCMPEQD(s1, s2);
			M_LSUB_IMM(REG_ZERO, 1, d);
			M_CMOVT(REG_ZERO, d);
			store_reg_to_var_int(iptr->dst, d);
			break;
			
		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_FCMPOLTF(s1, s2);
			M_FBF(3);
			M_LSUB_IMM(REG_ZERO, 1, d);
			M_BR(4);
			M_NOP;
			M_FCMPEQF(s1, s2);
			M_LADD_IMM(REG_ZERO, 1, d);
			M_CMOVT(REG_ZERO, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_DCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			M_FCMPOLTD(s1, s2);
			M_FBF(3);
			M_LSUB_IMM(REG_ZERO, 1, d);
			M_BR(4);
			M_NOP;
			M_FCMPEQD(s1, s2);
			M_LADD_IMM(REG_ZERO, 1, d);
			M_CMOVT(REG_ZERO, d);
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
			M_ASLL_IMM(s2, POINTERSHIFT, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_ALD(d, REG_ITMP1, OFFSET(java_objectarray, data[0]));
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
			M_ASLL_IMM(s2, 2, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_ILD(d, REG_ITMP1, OFFSET(java_intarray, data[0]));
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
			M_ASLL_IMM(s2, 3, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_LLD(d, REG_ITMP1, OFFSET(java_longarray, data[0]));
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
			M_ASLL_IMM(s2, 2, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
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
			M_ASLL_IMM(s2, 3, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
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
			M_AADD(s2, s1, REG_ITMP1);
			M_AADD(s2, REG_ITMP1, REG_ITMP1);
			M_SLDU(d, REG_ITMP1, OFFSET(java_chararray, data[0]));
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
			M_AADD(s2, s1, REG_ITMP1);
			M_AADD(s2, REG_ITMP1, REG_ITMP1);
			M_SLDS(d, REG_ITMP1, OFFSET(java_chararray, data[0]));
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
			M_AADD(s2, s1, REG_ITMP1);
			M_BLDS(d, REG_ITMP1, OFFSET(java_chararray, data[0]));
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
			M_ASLL_IMM(s2, POINTERSHIFT, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_AST(s3, REG_ITMP1, OFFSET(java_objectarray, data[0]));
			break;

		case ICMD_IASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_int(s3, src, REG_ITMP3);
			M_ASLL_IMM(s2, 2, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_IST(s3, REG_ITMP1, OFFSET(java_intarray, data[0]));
			break;

		case ICMD_LASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_int(s3, src, REG_ITMP3);
			M_ASLL_IMM(s2, 3, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_LST(s3, REG_ITMP1, OFFSET(java_longarray, data[0]));
			break;

		case ICMD_FASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_flt(s3, src, REG_FTMP3);
			M_ASLL_IMM(s2, 2, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_FST(s3, REG_ITMP1, OFFSET(java_floatarray, data[0]));
			break;

		case ICMD_DASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_flt(s3, src, REG_FTMP3);
			M_ASLL_IMM(s2, 3, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_DST(s3, REG_ITMP1, OFFSET(java_doublearray, data[0]));
			break;

		case ICMD_CASTORE:    /* ..., arrayref, index, value  ==> ...         */
		case ICMD_SASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_int(s3, src, REG_ITMP3);
			M_AADD(s2, s1, REG_ITMP1);
			M_AADD(s2, REG_ITMP1, REG_ITMP1);
			M_SST(s3, REG_ITMP1, OFFSET(java_chararray, data[0]));
			break;

		case ICMD_BASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_int(s3, src, REG_ITMP3);
			M_AADD(s2, s1, REG_ITMP1);
			M_BST(s3, REG_ITMP1, OFFSET(java_bytearray, data[0]));
			break;


		case ICMD_IASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 2, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_IST(REG_ZERO, REG_ITMP1, OFFSET(java_intarray, data[0]));
			break;

		case ICMD_LASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, 3, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_LST(REG_ZERO, REG_ITMP1, OFFSET(java_longarray, data[0]));
			break;

		case ICMD_AASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_ASLL_IMM(s2, POINTERSHIFT, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_AST(REG_ZERO, REG_ITMP1, OFFSET(java_objectarray, data[0]));
			break;

		case ICMD_BASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_AADD(s2, s1, REG_ITMP1);
			M_BST(REG_ZERO, REG_ITMP1, OFFSET(java_bytearray, data[0]));
			break;

		case ICMD_CASTORECONST:   /* ..., arrayref, index  ==> ...            */
		case ICMD_SASTORECONST:   /* ..., arrayref, index  ==> ...            */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			M_AADD(s2, s1, REG_ITMP1);
			M_AADD(s2, REG_ITMP1, REG_ITMP1);
			M_SST(REG_ZERO, REG_ITMP1, OFFSET(java_chararray, data[0]));
			break;


		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */
		                      /* op1 = type, val.a = field address            */

			/* If the static fields' class is not yet initialized, we do it   */
			/* now. The call code is generated later.                         */
  			if (!((fieldinfo *) iptr->val.a)->class->initialized) {
				codegen_addclinitref(cd, mcodeptr, ((fieldinfo *) iptr->val.a)->class);

				/* This is just for debugging purposes. Is very difficult to  */
				/* read patched code. Here we patch the following 2 nop's     */
				/* so that the real code keeps untouched.                     */
				if (showdisassemble) {
					M_NOP;
					M_NOP;
				}
  			}

			a = dseg_addaddress(cd, &(((fieldinfo *) iptr->val.a)->value));
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
				codegen_addclinitref(cd, mcodeptr, ((fieldinfo *) iptr->val.a)->class);

				/* This is just for debugging purposes. Is very difficult to  */
				/* read patched code. Here we patch the following 2 nop's     */
				/* so that the real code keeps untouched.                     */
				if (showdisassemble) {
					M_NOP;
					M_NOP;
				}
  			}

			a = dseg_addaddress(cd, &(((fieldinfo *) iptr->val.a)->value));
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
			default:
				throw_cacao_exception_exit(string_java_lang_InternalError,
										   "Unknown GETSTATIC operand type %d",
										   iptr->op1);
			}
			break;


		case ICMD_PUTFIELD:   /* ..., value  ==> ...                          */
		                      /* op1 = type, val.i = field offset             */

			a = ((fieldinfo *)(iptr->val.a))->offset;
			switch (iptr->op1) {
			case TYPE_INT:
				var_to_reg_int(s1, src->prev, REG_ITMP1);
				var_to_reg_int(s2, src, REG_ITMP2);
				gen_nullptr_check(s1);
				M_IST(s2, s1, a);
				break;
			case TYPE_LNG:
				var_to_reg_int(s1, src->prev, REG_ITMP1);
				var_to_reg_int(s2, src, REG_ITMP2);
				gen_nullptr_check(s1);
				M_LST(s2, s1, a);
				break;
			case TYPE_ADR:
				var_to_reg_int(s1, src->prev, REG_ITMP1);
				var_to_reg_int(s2, src, REG_ITMP2);
				gen_nullptr_check(s1);
				M_AST(s2, s1, a);
				break;
			case TYPE_FLT:
				var_to_reg_int(s1, src->prev, REG_ITMP1);
				var_to_reg_flt(s2, src, REG_FTMP2);
				gen_nullptr_check(s1);
				M_FST(s2, s1, a);
				break;
			case TYPE_DBL:
				var_to_reg_int(s1, src->prev, REG_ITMP1);
				var_to_reg_flt(s2, src, REG_FTMP2);
				gen_nullptr_check(s1);
				M_DST(s2, s1, a);
				break;
			default:
				throw_cacao_exception_exit(string_java_lang_InternalError,
										   "Unknown PUTFIELD operand type %d",
										   iptr->op1);
			}
			break;

		case ICMD_GETFIELD:   /* ...  ==> ..., value                          */
		                      /* op1 = type, val.i = field offset             */

			a = ((fieldinfo *)(iptr->val.a))->offset;
			switch (iptr->op1) {
			case TYPE_INT:
				var_to_reg_int(s1, src, REG_ITMP1);
				d = reg_of_var(rd, iptr->dst, REG_ITMP3);
				gen_nullptr_check(s1);
				M_ILD(d, s1, a);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_LNG:
				var_to_reg_int(s1, src, REG_ITMP1);
				d = reg_of_var(rd, iptr->dst, REG_ITMP3);
				gen_nullptr_check(s1);
				M_LLD(d, s1, a);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_ADR:
				var_to_reg_int(s1, src, REG_ITMP1);
				d = reg_of_var(rd, iptr->dst, REG_ITMP3);
				gen_nullptr_check(s1);
				M_ALD(d, s1, a);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_FLT:
				var_to_reg_int(s1, src, REG_ITMP1);
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				gen_nullptr_check(s1);
				M_FLD(d, s1, a);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			case TYPE_DBL:				
				var_to_reg_int(s1, src, REG_ITMP1);
				d = reg_of_var(rd, iptr->dst, REG_FTMP1);
				gen_nullptr_check(s1);
				M_DLD(d, s1, a);
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
			a = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP2, REG_PV, a);
			M_JSR(REG_ITMP2_XPC, REG_ITMP2);
			M_NOP;
			M_NOP;              /* nop ensures that XPC is less than the end */
			                    /* of basic block                            */
			ALIGNCODENOP;
			break;

		case ICMD_GOTO:         /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */
			M_BR(0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			ALIGNCODENOP;
			break;

		case ICMD_JSR:          /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */

			dseg_addtarget(cd, BlockPtrOfPC(iptr->op1));
			M_ALD(REG_ITMP1, REG_PV, -(cd->dseglen));
			M_JSR(REG_ITMP1, REG_ITMP1);        /* REG_ITMP1 = return address */
			M_NOP;
			break;
			
		case ICMD_RET:          /* ... ==> ...                                */
		                        /* op1 = local variable                       */
			var = &(rd->locals[iptr->op1][TYPE_ADR]);
			if (var->flags & INMEMORY) {
				M_ALD(REG_ITMP1, REG_SP, 8 * var->regoff);
				M_RET(REG_ITMP1);
				}
			else
				M_RET(var->regoff);
			M_NOP;
			ALIGNCODENOP;
			break;

		case ICMD_IFNULL:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_BEQZ(s1, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IFNONNULL:    /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_BNEZ(s1, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IFEQ:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BEQZ(s1, 0);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_BEQ(s1, REG_ITMP2, 0);
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IFLT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BLTZ(s1, 0);
				}
			else {
				if ((iptr->val.i >= -32768) && (iptr->val.i <= 32767)) {
					M_CMPLT_IMM(s1, iptr->val.i, REG_ITMP1);
					}
				else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMPLT(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BNEZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IFLE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BLEZ(s1, 0);
				}
			else {
				if ((iptr->val.i >= -32769) && (iptr->val.i <= 32766)) {
					M_CMPLT_IMM(s1, iptr->val.i + 1, REG_ITMP1);
					M_BNEZ(REG_ITMP1, 0);
					}
				else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMPGT(s1, REG_ITMP2, REG_ITMP1);
					M_BEQZ(REG_ITMP1, 0);
					}
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IFNE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BNEZ(s1, 0);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_BNE(s1, REG_ITMP2, 0);
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IFGT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BGTZ(s1, 0);
				}
			else {
				if ((iptr->val.i >= -32769) && (iptr->val.i <= 32766)) {
					M_CMPLT_IMM(s1, iptr->val.i + 1, REG_ITMP1);
					M_BEQZ(REG_ITMP1, 0);
					}
				else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMPGT(s1, REG_ITMP2, REG_ITMP1);
					M_BNEZ(REG_ITMP1, 0);
					}
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IFGE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.i == 0) {
				M_BGEZ(s1, 0);
				}
			else {
				if ((iptr->val.i >= -32768) && (iptr->val.i <= 32767)) {
					M_CMPLT_IMM(s1, iptr->val.i, REG_ITMP1);
					}
				else {
					ICONST(REG_ITMP2, iptr->val.i);
					M_CMPLT(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BEQZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_LEQ:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BEQZ(s1, 0);
				}
			else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_BEQ(s1, REG_ITMP2, 0);
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_LLT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BLTZ(s1, 0);
				}
			else {
				if ((iptr->val.l >= -32768) && (iptr->val.l <= 32767)) {
					M_CMPLT_IMM(s1, iptr->val.l, REG_ITMP1);
					}
				else {
					LCONST(REG_ITMP2, iptr->val.l);
					M_CMPLT(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BNEZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_LLE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BLEZ(s1, 0);
				}
			else {
				if ((iptr->val.l >= -32769) && (iptr->val.l <= 32766)) {
					M_CMPLT_IMM(s1, iptr->val.l + 1, REG_ITMP1);
					M_BNEZ(REG_ITMP1, 0);
					}
				else {
					LCONST(REG_ITMP2, iptr->val.l);
					M_CMPGT(s1, REG_ITMP2, REG_ITMP1);
					M_BEQZ(REG_ITMP1, 0);
					}
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_LNE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BNEZ(s1, 0);
				}
			else {
				LCONST(REG_ITMP2, iptr->val.l);
				M_BNE(s1, REG_ITMP2, 0);
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_LGT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BGTZ(s1, 0);
				}
			else {
				if ((iptr->val.l >= -32769) && (iptr->val.l <= 32766)) {
					M_CMPLT_IMM(s1, iptr->val.l + 1, REG_ITMP1);
					M_BEQZ(REG_ITMP1, 0);
					}
				else {
					LCONST(REG_ITMP2, iptr->val.l);
					M_CMPGT(s1, REG_ITMP2, REG_ITMP1);
					M_BNEZ(REG_ITMP1, 0);
					}
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_LGE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if (iptr->val.l == 0) {
				M_BGEZ(s1, 0);
				}
			else {
				if ((iptr->val.l >= -32768) && (iptr->val.l <= 32767)) {
					M_CMPLT_IMM(s1, iptr->val.l, REG_ITMP1);
					}
				else {
					LCONST(REG_ITMP2, iptr->val.l);
					M_CMPLT(s1, REG_ITMP2, REG_ITMP1);
					}
				M_BEQZ(REG_ITMP1, 0);
				}
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_ICMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPEQ:    /* op1 = target JavaVM pc                     */
		case ICMD_IF_ACMPEQ:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_BEQ(s1, s2, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_ICMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPNE:    /* op1 = target JavaVM pc                     */
		case ICMD_IF_ACMPNE:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_BNE(s1, s2, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_ICMPLT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPLT:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPLT(s1, s2, REG_ITMP1);
			M_BNEZ(REG_ITMP1, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_ICMPGT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPGT:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPGT(s1, s2, REG_ITMP1);
			M_BNEZ(REG_ITMP1, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_ICMPLE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPLE:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPGT(s1, s2, REG_ITMP1);
			M_BEQZ(REG_ITMP1, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_ICMPGE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPGE:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPLT(s1, s2, REG_ITMP1);
			M_BEQZ(REG_ITMP1, 0);
			codegen_addreference(cd, BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

#ifdef CONDITIONAL_LOADCONST
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
#endif


		case ICMD_IRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_LRETURN:
		case ICMD_ARETURN:

#if defined(USE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				s4 disp;
				a = dseg_addaddress(cd, (void *) builtin_monitorexit);
				M_ALD(REG_ITMP3, REG_PV, a);
				M_JSR(REG_RA, REG_ITMP3);
				M_ALD(rd->argintregs[0], REG_SP, rd->maxmemuse * 8); /* delay slot */
				disp = -(s4) ((u1 *) mcodeptr - cd->mcodebase);
				M_LDA(REG_PV, REG_RA, disp);
			}
#endif
			var_to_reg_int(s1, src, REG_RESULT);
			M_INTMOVE(s1, REG_RESULT);
			goto nowperformreturn;

		case ICMD_FRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_DRETURN:

#if defined(USE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				s4 disp;
				a = dseg_addaddress(cd, (void *) builtin_monitorexit);
				M_ALD(REG_ITMP3, REG_PV, a);
				M_JSR(REG_RA, REG_ITMP3);
				M_ALD(rd->argintregs[0], REG_SP, rd->maxmemuse * 8); /* delay slot */
				disp = -(s4) ((u1 *) mcodeptr - cd->mcodebase);
				M_LDA(REG_PV, REG_RA, disp);
			}
#endif
			var_to_reg_flt(s1, src, REG_FRESULT);
			{
				int t = ((iptr->opc == ICMD_FRETURN) ? TYPE_FLT : TYPE_DBL);
				M_TFLTMOVE(t, s1, REG_FRESULT);
			}
			goto nowperformreturn;

		case ICMD_RETURN:      /* ...  ==> ...                                */

#if defined(USE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				s4 disp;
				a = dseg_addaddress(cd, (void *) builtin_monitorexit);
				M_ALD(REG_ITMP3, REG_PV, a);
				M_JSR(REG_RA, REG_ITMP3);
				M_ALD(rd->argintregs[0], REG_SP, rd->maxmemuse * 8); /* delay slot */
				disp = -(s4) ((u1 *) mcodeptr - cd->mcodebase);
				M_LDA(REG_PV, REG_RA, disp);
			}
#endif

nowperformreturn:
			{
			s4 i, p;
			
			p = parentargs_base;
			
			/* restore return address                                         */

			if (!m->isleafmethod) {
				p--; M_LLD(REG_RA, REG_SP, 8 * p);
			}

			/* restore saved registers                                        */

			for (i = rd->savintregcnt - 1; i >= rd->maxsavintreguse; i--) {
				p--; M_LLD(rd->savintregs[i], REG_SP, 8 * p);
			}
			for (i = rd->savfltregcnt - 1; i >= rd->maxsavfltreguse; i--) {
				p--; M_DLD(rd->savfltregs[i], REG_SP, 8 * p);
			}

			/* call trace function */

			if (runverbose) {
				M_LDA (REG_SP, REG_SP, -24);
				M_LST(REG_RA, REG_SP, 0);
				M_LST(REG_RESULT, REG_SP, 8);
				M_DST(REG_FRESULT, REG_SP,16);
				a = dseg_addaddress(cd, m);
				M_ALD(rd->argintregs[0], REG_PV, a);
				M_MOV(REG_RESULT, rd->argintregs[1]);
				M_FLTMOVE(REG_FRESULT, rd->argfltregs[2]);
				M_FMOV(REG_FRESULT, rd->argfltregs[3]);
				a = dseg_addaddress(cd, (void *) builtin_displaymethodstop);
				M_ALD(REG_ITMP3, REG_PV, a);
				M_JSR (REG_RA, REG_ITMP3);
				M_NOP;
				M_DLD(REG_FRESULT, REG_SP,16);
				M_LLD(REG_RESULT, REG_SP, 8);
				M_LLD(REG_RA, REG_SP, 0);
				M_LDA (REG_SP, REG_SP, 24);
			}

			M_RET(REG_RA);

			/* deallocate stack                                               */

			if (parentargs_base) {
				M_LDA(REG_SP, REG_SP, parentargs_base * 8);

			} else {
				M_NOP;
			}

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
				M_IADD_IMM(s1, -l, REG_ITMP1);
				}
			else {
				ICONST(REG_ITMP2, l);
				M_ISUB(s1, REG_ITMP2, REG_ITMP1);
				}
			i = i - l + 1;

			/* range check */

			M_CMPULT_IMM(REG_ITMP1, i, REG_ITMP2);
			M_BEQZ(REG_ITMP2, 0);
			codegen_addreference(cd, (basicblock *) tptr[0], mcodeptr);
			M_ASLL_IMM(REG_ITMP1, POINTERSHIFT, REG_ITMP1);      /* delay slot*/

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

			M_AADD(REG_ITMP1, REG_PV, REG_ITMP2);
			M_ALD(REG_ITMP2, REG_ITMP2, -(cd->dseglen));
			M_JMP(REG_ITMP2);
			M_NOP;
			ALIGNCODENOP;
			break;


		case ICMD_LOOKUPSWITCH: /* ..., key ==> ...                           */
			{
			s4 i, /*l, */val, *s4ptr;
			void **tptr;

			tptr = (void **) iptr->target;

			s4ptr = iptr->val.a;
			/*l = s4ptr[0];*/                          /* default  */
			i = s4ptr[1];                          /* count    */
			
			MCODECHECK((i<<2)+8);
			var_to_reg_int(s1, src, REG_ITMP1);
			while (--i >= 0) {
				s4ptr += 2;
				++tptr;

				val = s4ptr[0];
				ICONST(REG_ITMP2, val);
				M_BEQ(s1, REG_ITMP2, 0);
				codegen_addreference(cd, (basicblock *) tptr[0], mcodeptr); 
				M_NOP;
				}

			M_BR(0);
			tptr = (void **) iptr->target;
			codegen_addreference(cd, (basicblock *) tptr[0], mcodeptr);
			M_NOP;
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

					} else  {
						var_to_reg_int(d, src, REG_ITMP1);
						M_LST(d, REG_SP, 8 * (s3 - INT_ARG_CNT));
					}

				} else {
					if (s3 < FLT_ARG_CNT) {
						s1 = rd->argfltregs[s3];
						var_to_reg_flt(d, src, s1);
						M_TFLTMOVE(src->type, d, s1);

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
				a = dseg_addaddress(cd, (void *) lm);
				d = iptr->op1;                                 /* return type */

				M_ALD(REG_ITMP3, REG_PV, a);     /* built-in-function pointer */
				M_JSR(REG_RA, REG_ITMP3);
				M_NOP;
				goto afteractualcall;

			case ICMD_INVOKESTATIC:
			case ICMD_INVOKESPECIAL:
				a = dseg_addaddress(cd, lm->stubroutine);
				d = lm->returntype;

				M_ALD(REG_PV, REG_PV, a);             /* method pointer in pv */
				break;

			case ICMD_INVOKEVIRTUAL:
				d = lm->returntype;

				gen_nullptr_check(rd->argintregs[0]);
				M_ALD(REG_METHODPTR, rd->argintregs[0], OFFSET(java_objectheader, vftbl));
				M_ALD(REG_PV, REG_METHODPTR, OFFSET(vftbl_t, table[0]) + sizeof(methodptr) * lm->vftblindex);
				break;

			case ICMD_INVOKEINTERFACE:
				d = lm->returntype;
					
				gen_nullptr_check(rd->argintregs[0]);
				M_ALD(REG_METHODPTR, rd->argintregs[0], OFFSET(java_objectheader, vftbl));
				M_ALD(REG_METHODPTR, REG_METHODPTR, OFFSET(vftbl_t, interfacetable[0]) - sizeof(methodptr*) * lm->class->index);
				M_ALD(REG_PV, REG_METHODPTR, sizeof(methodptr) * (lm - lm->class->methods));
				break;
			}

			M_JSR(REG_RA, REG_PV);
			M_NOP;

			/* recompute pv */

afteractualcall:

			s1 = (s4) ((u1 *) mcodeptr - cd->mcodebase);
			if (s1 <= 32768) M_LDA(REG_PV, REG_RA, -s1);
			else {
				s4 ml = -s1, mh = 0;
				while (ml < -32768) { ml += 65536; mh--; }
				M_LUI(REG_PV, mh);
				M_IADD_IMM(REG_PV, ml, REG_PV);
				M_LADD(REG_PV, REG_RA, REG_PV);
			}

			/* d contains return type */

			if (d != TYPE_VOID) {
				if (IS_INT_LNG_TYPE(iptr->dst->type)) {
					s1 = reg_of_var(rd, iptr->dst, REG_RESULT);
					M_INTMOVE(REG_RESULT, s1);
					store_reg_to_var_int(iptr->dst, s1);

				} else {
					s1 = reg_of_var(rd, iptr->dst, REG_FRESULT);
					M_TFLTMOVE(iptr->dst->type, REG_FRESULT, s1);
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
			
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			codegen_threadcritrestart(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			if (s1 == d) {
				M_MOV(s1, REG_ITMP1);
				s1 = REG_ITMP1;
				}
			M_CLR(d);
			if (iptr->op1) {                               /* class/interface */
				if (super->flags & ACC_INTERFACE) {        /* interface       */
					M_BEQZ(s1, 8);
					M_NOP;
					M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
					M_ILD(REG_ITMP2, REG_ITMP1, OFFSET(vftbl_t, interfacetablelength));
					M_IADD_IMM(REG_ITMP2, - super->index, REG_ITMP2);
					M_BLEZ(REG_ITMP2, 3);
					M_NOP;
					M_ALD(REG_ITMP1, REG_ITMP1,
					      OFFSET(vftbl_t, interfacetable[0]) -
					      super->index * sizeof(methodptr*));
					M_CMPULT(REG_ZERO, REG_ITMP1, d);      /* REG_ITMP1 != 0  */
					}
				else {                                     /* class           */
					/*
					s2 = super->vftbl->diffval;
					M_BEQZ(s1, 5);
					M_NOP;
					M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
					M_ILD(REG_ITMP1, REG_ITMP1, OFFSET(vftbl_t, baseval));
					M_IADD_IMM(REG_ITMP1, - super->vftbl->baseval, REG_ITMP1);
					M_CMPULT_IMM(REG_ITMP1, s2 + 1, d);
					*/

					M_BEQZ(s1, 9);
					M_NOP;
					M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
                    a = dseg_addaddress(cd, (void *) super->vftbl);
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
                    M_CMPULT(REG_ITMP2, REG_ITMP1, d);
					M_XOR_IMM(d, 1, d);

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
			codegen_threadcritrestart(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif

			d = reg_of_var(rd, iptr->dst, REG_ITMP3);
			var_to_reg_int(s1, src, d);
			if (iptr->op1) {                               /* class/interface */
				if (super->flags & ACC_INTERFACE) {        /* interface       */
					M_BEQZ(s1, 9);
					M_NOP;
					M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
					M_ILD(REG_ITMP2, REG_ITMP1, OFFSET(vftbl_t, interfacetablelength));
					M_IADD_IMM(REG_ITMP2, - super->index, REG_ITMP2);
					M_BLEZ(REG_ITMP2, 0);
					codegen_addxcastrefs(cd, mcodeptr);
					M_NOP;
					M_ALD(REG_ITMP2, REG_ITMP1,
					      OFFSET(vftbl_t, interfacetable[0]) -
					      super->index * sizeof(methodptr*));
					M_BEQZ(REG_ITMP2, 0);
					codegen_addxcastrefs(cd, mcodeptr);
					M_NOP;
					}
				else {                                     /* class           */

					/*
					s2 = super->vftbl->diffval;
					M_BEQZ(s1, 6 + (s2 != 0));
					M_NOP;
					M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
					M_ILD(REG_ITMP1, REG_ITMP1, OFFSET(vftbl_t, baseval));
					M_IADD_IMM(REG_ITMP1, - super->vftbl->baseval, REG_ITMP1);
					if (s2 == 0) {
						M_BNEZ(REG_ITMP1, 0);
						}
					else{
						M_CMPULT_IMM(REG_ITMP1, s2 + 1, REG_ITMP2);
						M_BEQZ(REG_ITMP2, 0);
						}
					*/

					M_BEQZ(s1, 10 + (d == REG_ITMP3));
					M_NOP;
					M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
                    a = dseg_addaddress(cd, (void *) super->vftbl);
                    M_ALD(REG_ITMP2, REG_PV, a);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
					codegen_threadcritstart(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
                    M_ILD(REG_ITMP1, REG_ITMP1, OFFSET(vftbl_t, baseval));
					if (d != REG_ITMP3) {
						M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl_t, baseval));
						M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, diffval));
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
						codegen_threadcritstop(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
						M_ISUB(REG_ITMP1, REG_ITMP3, REG_ITMP1); 
					} else {
						M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, baseval));
						M_ISUB(REG_ITMP1, REG_ITMP2, REG_ITMP1); 
						M_ALD(REG_ITMP2, REG_PV, a);
						M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, diffval));
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
						codegen_threadcritstop(cd, (u1 *) mcodeptr - cd->mcodebase);
#endif
					}
                    M_CMPULT(REG_ITMP2, REG_ITMP1, REG_ITMP2);
					M_BNEZ(REG_ITMP2, 0);

					codegen_addxcastrefs(cd, mcodeptr);
					M_NOP;
					}
				}
			else
				panic ("internal error: no inlined array checkcast");
			}
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_CHECKASIZE:  /* ..., size ==> ..., size                     */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_BLTZ(s1, 0);
			codegen_addxcheckarefs(cd, mcodeptr);
			M_NOP;
			break;

		case ICMD_CHECKEXCEPTION:  /* ... ==> ...                             */

			M_BEQZ(REG_RESULT, 0);
			codegen_addxexceptionrefs(cd, mcodeptr);
			M_NOP;
			break;

		case ICMD_MULTIANEWARRAY:/* ..., cnt1, [cnt2, ...] ==> ..., arrayref  */
		                      /* op1 = dimension, val.a = array descriptor    */

			/* check for negative sizes and copy sizes to stack if necessary  */

			MCODECHECK((iptr->op1 << 1) + 64);

			for (s1 = iptr->op1; --s1 >= 0; src = src->prev) {
				var_to_reg_int(s2, src, REG_ITMP1);
				M_BLTZ(s2, 0);
				codegen_addxcheckarefs(cd, mcodeptr);
				M_NOP;

				/* copy SAVEDVAR sizes to stack */

				if (src->varkind != ARGVAR) {
					M_LST(s2, REG_SP, s1 * 8);
				}
			}

			/* a0 = dimension count */

			ICONST(rd->argintregs[0], iptr->op1);

			/* a1 = arraydescriptor */

			a = dseg_addaddress(cd, iptr->val.a);
			M_ALD(rd->argintregs[1], REG_PV, a);

			/* a2 = pointer to dimensions = stack pointer */

			M_INTMOVE(REG_SP, rd->argintregs[2]);

			a = dseg_addaddress(cd, (void*) builtin_nmultianewarray);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JSR(REG_RA, REG_ITMP3);
			M_NOP;
			s1 = (int)((u1*) mcodeptr - cd->mcodebase);
			if (s1 <= 32768)
				M_LDA (REG_PV, REG_RA, -s1);
			else {
				panic("To big");
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
	while (src) {
		len--;
		if ((src->varkind != STACKVAR)) {
			s2 = src->type;
			if (IS_FLT_DBL_TYPE(s2)) {
				var_to_reg_flt(s1, src, REG_FTMP1);
				if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
					M_TFLTMOVE(s2, s1, rd->interfaces[len][s2].regoff);

				} else {
					M_DST(s1, REG_SP, 8 * rd->interfaces[len][s2].regoff);
				}

			} else {
				var_to_reg_int(s1, src, REG_ITMP1);
				if (!(rd->interfaces[len][s2].flags & INMEMORY)) {
					M_INTMOVE(s1, rd->interfaces[len][s2].regoff);

				} else {
					M_LST(s1, REG_SP, 8 * rd->interfaces[len][s2].regoff);
				}
			}
		}
		src = src->prev;
	}
	} /* if (bptr -> flags >= BBREACHED) */
	} /* for basic block */

	{
	/* generate bound check stubs */

	s4        *xcodeptr = NULL;
	branchref *bref;

	for (bref = cd->xboundrefs; bref != NULL; bref = bref->next) {
		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(14);

		M_MOV(bref->reg, REG_ITMP1);
		M_LADD_IMM(REG_PV, bref->branchpos - 4, REG_ITMP2_XPC);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr);
			M_NOP;

		} else {
			xcodeptr = mcodeptr;

			M_LSUB_IMM(REG_SP, 1 * 8, REG_SP);
			M_LST(REG_ITMP2_XPC, REG_SP, 0 * 8);

			M_MOV(REG_ITMP1, rd->argintregs[0]);
			a = dseg_addaddress(cd, new_arrayindexoutofboundsexception);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JSR(REG_RA, REG_ITMP3);
			M_NOP;
			M_MOV(REG_RESULT, REG_ITMP1_XPTR);

			M_LLD(REG_ITMP2_XPC, REG_SP, 0 * 8);
			M_LADD_IMM(REG_SP, 1 * 8, REG_SP);

			a = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JMP(REG_ITMP3);
			M_NOP;
		}
	}

	/* generate negative array size check stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xcheckarefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  (u1 *) xcodeptr - cd->mcodebase - 4);
			continue;
		}

		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(12);

		M_LADD_IMM(REG_PV, bref->branchpos - 4, REG_ITMP2_XPC);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr);
			M_NOP;

		} else {
			xcodeptr = mcodeptr;

			M_LSUB_IMM(REG_SP, 1 * 8, REG_SP);
			M_LST(REG_ITMP2_XPC, REG_SP, 0 * 8);

			a = dseg_addaddress(cd, new_negativearraysizeexception);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JSR(REG_RA, REG_ITMP3);
			M_NOP;
			M_MOV(REG_RESULT, REG_ITMP1_XPTR);

			M_LLD(REG_ITMP2_XPC, REG_SP, 0 * 8);
			M_LADD_IMM(REG_SP, 1 * 8, REG_SP);

			a = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JMP(REG_ITMP3);
			M_NOP;
		}
	}

	/* generate cast check stubs */

	xcodeptr = NULL;
	
	for (bref = cd->xcastrefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  (u1 *) xcodeptr - cd->mcodebase - 4);
			continue;
		}

		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(12);

		M_LADD_IMM(REG_PV, bref->branchpos - 4, REG_ITMP2_XPC);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr);
			M_NOP;

		} else {
			xcodeptr = mcodeptr;

			M_LSUB_IMM(REG_SP, 1 * 8, REG_SP);
			M_LST(REG_ITMP2_XPC, REG_SP, 0 * 8);

			a = dseg_addaddress(cd, new_classcastexception);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JSR(REG_RA, REG_ITMP3);
			M_NOP;
			M_MOV(REG_RESULT, REG_ITMP1_XPTR);

			M_LLD(REG_ITMP2_XPC, REG_SP, 0 * 8);
			M_LADD_IMM(REG_SP, 1 * 8, REG_SP);

			a = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JMP(REG_ITMP3);
			M_NOP;
		}
	}

	/* generate exception check stubs */

	xcodeptr = NULL;

	for (bref = cd->xexceptionrefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  (u1 *) xcodeptr - cd->mcodebase - 4);
			continue;
		}

		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(13);

		M_LADD_IMM(REG_PV, bref->branchpos - 4, REG_ITMP2_XPC);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr);
			M_NOP;

		} else {
			xcodeptr = mcodeptr;

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			M_LSUB_IMM(REG_SP, 1 * 8, REG_SP);
			M_LST(REG_ITMP2_XPC, REG_SP, 0 * 8);

			a = dseg_addaddress(cd, builtin_get_exceptionptrptr);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JSR(REG_RA, REG_ITMP3);
			M_NOP;

			/* get the exceptionptr from the ptrprt and clear it */
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

			a = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JMP(REG_ITMP3);
			M_NOP;
		}
	}

	/* generate null pointer check stubs */

	xcodeptr = NULL;

	for (bref = cd->xnullrefs; bref != NULL; bref = bref->next) {
		if ((cd->exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
							  bref->branchpos,
							  (u1 *) xcodeptr - cd->mcodebase - 4);
			continue;
		}

		gen_resolvebranch((u1 *) cd->mcodebase + bref->branchpos, 
		                  bref->branchpos,
						  (u1 *) mcodeptr - cd->mcodebase);

		MCODECHECK(12);

		M_LADD_IMM(REG_PV, bref->branchpos - 4, REG_ITMP2_XPC);

		if (xcodeptr != NULL) {
			M_BR(xcodeptr - mcodeptr);
			M_NOP;

		} else {
			xcodeptr = mcodeptr;

			M_LSUB_IMM(REG_SP, 1 * 8, REG_SP);
			M_LST(REG_ITMP2_XPC, REG_SP, 0 * 8);

			a = dseg_addaddress(cd, new_nullpointerexception);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JSR(REG_RA, REG_ITMP3);
			M_NOP;
			M_MOV(REG_RESULT, REG_ITMP1_XPTR);

			M_LLD(REG_ITMP2_XPC, REG_SP, 0 * 8);
			M_LADD_IMM(REG_SP, 1 * 8, REG_SP);

			a = dseg_addaddress(cd, asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JMP(REG_ITMP3);
			M_NOP;
		}
	}


	/* generate put/getstatic stub call code */

	{
		clinitref   *cref;
		u8           mcode;
		s4          *tmpmcodeptr;

		for (cref = cd->clinitrefs; cref != NULL; cref = cref->next) {
			/* Get machine code which is patched back in later. The call is   */
			/* 2 instruction words long.                                      */
			xcodeptr = (s4 *) (cd->mcodebase + cref->branchpos);

			/* We need to split this, because an unaligned 8 byte read causes */
			/* a SIGSEGV.                                                     */
			mcode = ((u8) xcodeptr[1] << 32) + (u4) xcodeptr[0];

			/* patch in the call to call the following code (done at compile  */
			/* time)                                                          */

			tmpmcodeptr = mcodeptr;         /* save current mcodeptr          */
			mcodeptr = xcodeptr;            /* set mcodeptr to patch position */

			M_BRS(tmpmcodeptr - (xcodeptr + 1));
			M_NOP;

			mcodeptr = tmpmcodeptr;         /* restore the current mcodeptr   */

			MCODECHECK(5);

			/* move class pointer into REG_ITMP1                              */
			a = dseg_addaddress(cd, cref->class);
			M_ALD(REG_ITMP1, REG_PV, a);

			/* move machine code into REG_ITMP2                               */
			a = dseg_adds8(cd, mcode);
			M_LLD(REG_ITMP2, REG_PV, a);

			a = dseg_addaddress(cd, asm_check_clinit);
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JMP(REG_ITMP3);
			M_NOP;
		}
	}
	}

	codegen_finish(m, cd, (s4) ((u1 *) mcodeptr - cd->mcodebase));

	docacheflush((void*) m->entrypoint, ((u1*) mcodeptr - cd->mcodebase));
}


/* function createcompilerstub *************************************************

	creates a stub routine which calls the compiler
	
*******************************************************************************/

#define COMPSTUB_SIZE    4

u1 *createcompilerstub(methodinfo *m)
{
	u8 *s = CNEW(u8, COMPSTUB_SIZE);    /* memory to hold the stub            */
	s4 *mcodeptr = (s4 *) s;            /* code generation pointer            */
	
	                                    /* code for the stub                  */
	M_ALD(REG_PV, REG_PV, 24);          /* load pointer to the compiler       */
	M_NOP;
	M_JSR(REG_ITMP1, REG_PV);           /* jump to the compiler, return address
	                                       in itmp1 is used as method pointer */
	M_NOP;

	s[2] = (u8) m;                      /* literals to be adressed            */
	s[3] = (u8) asm_call_jit_compiler;  /* jump directly via PV from above    */

	(void) docacheflush((void*) s, (char*) mcodeptr - (char*) s);

#if defined(STATISTICS)
	if (opt_stat)
		count_cstub_len += COMPSTUB_SIZE * 8;
#endif

	return (u1 *) s;
}


/* function removecompilerstub *************************************************

     deletes a compilerstub from memory  (simply by freeing it)

*******************************************************************************/

void removecompilerstub(u1 *stub)
{
	CFREE(stub, COMPSTUB_SIZE * 8);
}


/* function: createnativestub **************************************************

	creates a stub routine which calls a native method

*******************************************************************************/

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
#define NATIVESTUB_STACK           2
#define NATIVESTUB_THREAD_EXTRA    5
#else
#define NATIVESTUB_STACK           1
#define NATIVESTUB_THREAD_EXTRA    1
#endif

#define NATIVESTUB_SIZE            (54 + 4 + NATIVESTUB_THREAD_EXTRA - 1)
#define NATIVESTUB_STATIC_SIZE     5
#define NATIVESTUB_VERBOSE_SIZE    (50 + 17)
#define NATIVESTUB_OFFSET          10


u1 *createnativestub(functionptr f, methodinfo *m)
{
	u8 *s;                              /* memory to hold the stub            */
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

	descriptor2types(m);                /* set paramcount and paramtypes      */

	stubsize = NATIVESTUB_SIZE;         /* calculate nativestub size          */

	if ((m->flags & ACC_STATIC) && !m->class->initialized)
		stubsize += NATIVESTUB_STATIC_SIZE;

	if (runverbose)
		stubsize += NATIVESTUB_VERBOSE_SIZE;

	s = CNEW(u8, stubsize);             /* memory to hold the stub            */
	cs = s + NATIVESTUB_OFFSET;
	mcodeptr = (s4 *) (cs);             /* code generation pointer            */

	/* set some required varibles which are normally set by codegen_setup     */
	cd->mcodebase = (u1 *) mcodeptr;
	cd->clinitrefs = NULL;

	*(cs-1)  = (u8) f;                  /* address of native method           */
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	*(cs-2)  = (u8) &builtin_get_exceptionptrptr;
#else
	*(cs-2)  = (u8) (&_exceptionptr);   /* address of exceptionptr            */
#endif
	*(cs-3)  = (u8) asm_handle_nat_exception;/* addr of asm exception handler */
	*(cs-4)  = (u8) (&env);             /* addr of jni_environement           */
 	*(cs-5)  = (u8) builtin_trace_args;
	*(cs-6)  = (u8) m;
 	*(cs-7)  = (u8) builtin_displaymethodstop;
	*(cs-8)  = (u8) m->class;
	*(cs-9)  = (u8) asm_check_clinit;
	*(cs-10) = (u8) NULL;               /* filled with machine code           */

	M_LDA(REG_SP, REG_SP, -NATIVESTUB_STACK * 8); /* build up stackframe      */
	M_LST(REG_RA, REG_SP, 0);           /* store return address               */

	/* if function is static, check for initialized */

	if (m->flags & ACC_STATIC && !m->class->initialized) {
		codegen_addclinitref(cd, mcodeptr, m->class);
	}

	/* max. 50 instructions */
	if (runverbose) {
		s4 p;
		s4 t;

		M_LDA(REG_SP, REG_SP, -(18 * 8));
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
					M_DST(rd->argfltregs[p], REG_SP, (10 + p) * 8);
					M_LLD(rd->argintregs[p], REG_SP, (10 + p) * 8);

				} else {
					M_FST(rd->argfltregs[p], REG_SP, (10 + p) * 8);
					M_ILD(rd->argintregs[p], REG_SP, (10 + p) * 8);
				}

			} else {
				M_DST(rd->argfltregs[p], REG_SP, (10 + p) * 8);
			}
		}

		M_ALD(REG_ITMP1, REG_PV, -6 * 8); /* method address                   */
		M_AST(REG_ITMP1, REG_SP, 0);
		M_ALD(REG_ITMP3, REG_PV, -5 * 8); /* builtin_trace_args               */
		M_JSR(REG_RA, REG_ITMP3);
		M_NOP;
		disp = -(s4) (mcodeptr - (s4 *) cs) * 4;
		M_LDA(REG_PV, REG_RA, disp);

		for (p = 0; p < m->paramcount && p < INT_ARG_CNT; p++) {
			M_LLD(rd->argintregs[p], REG_SP,  (2 + p) * 8);
		}

		for (p = 0; p < m->paramcount && p < FLT_ARG_CNT; p++) {
			t = m->paramtypes[p];

			if (IS_FLT_DBL_TYPE(t)) {
				if (IS_2_WORD_TYPE(t)) {
					M_DLD(rd->argfltregs[p], REG_SP, (10 + p) * 8);

				} else {
					M_FLD(rd->argfltregs[p], REG_SP, (10 + p) * 8);
				}

			} else {
				M_DLD(rd->argfltregs[p], REG_SP, (10 + p) * 8);
			}
		}

		M_ALD(REG_RA, REG_SP, 1 * 8);
		M_LDA(REG_SP, REG_SP, 18 * 8);
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

	if (m->flags & ACC_STATIC) {
		M_MOV(rd->argintregs[5], rd->argintregs[7]);

		M_DMFC1(REG_ITMP1, rd->argfltregs[5]);
		M_DMTC1(REG_ITMP1, rd->argfltregs[7]);

		M_MOV(rd->argintregs[4], rd->argintregs[6]);

		M_DMFC1(REG_ITMP1, rd->argfltregs[4]);
		M_DMTC1(REG_ITMP1, rd->argfltregs[6]);

		M_MOV(rd->argintregs[3], rd->argintregs[5]);
		M_DMFC1(REG_ITMP1, rd->argfltregs[3]);

		M_MOV(rd->argintregs[2], rd->argintregs[4]);
		M_DMTC1(REG_ITMP1, rd->argfltregs[5]);

		M_MOV(rd->argintregs[1], rd->argintregs[3]);
		M_DMFC1(REG_ITMP1, rd->argfltregs[2]);

		M_MOV(rd->argintregs[0], rd->argintregs[2]);
		M_DMTC1(REG_ITMP1, rd->argfltregs[4]);

		M_DMFC1(REG_ITMP1, rd->argfltregs[1]);
		M_DMTC1(REG_ITMP1, rd->argfltregs[3]);

		M_DMFC1(REG_ITMP1, rd->argfltregs[0]);
		M_DMTC1(REG_ITMP1, rd->argfltregs[2]);

		M_ALD(rd->argintregs[1], REG_PV, -8 * 8);

	} else {
		M_MOV(rd->argintregs[6], rd->argintregs[7]);

		M_DMFC1(REG_ITMP1, rd->argfltregs[6]);
		M_DMTC1(REG_ITMP1, rd->argfltregs[7]);

		M_MOV(rd->argintregs[5], rd->argintregs[6]);

		M_DMFC1(REG_ITMP1, rd->argfltregs[5]);
		M_DMTC1(REG_ITMP1, rd->argfltregs[6]);

		M_MOV(rd->argintregs[4], rd->argintregs[5]);
		M_DMFC1(REG_ITMP1, rd->argfltregs[4]);

		M_MOV(rd->argintregs[3], rd->argintregs[4]);
		M_DMTC1(REG_ITMP1, rd->argfltregs[5]);

		M_MOV(rd->argintregs[2], rd->argintregs[3]);
		M_DMFC1(REG_ITMP1, rd->argfltregs[3]);

		M_MOV(rd->argintregs[1], rd->argintregs[2]);
		M_DMTC1(REG_ITMP1, rd->argfltregs[4]);

		M_MOV(rd->argintregs[0], rd->argintregs[1]);
		M_DMFC1(REG_ITMP1, rd->argfltregs[2]);

		M_DMTC1(REG_ITMP1, rd->argfltregs[3]);

		M_DMFC1(REG_ITMP1, rd->argfltregs[1]);
		M_DMFC1(REG_ITMP2, rd->argfltregs[0]);

		M_DMTC1(REG_ITMP1, rd->argfltregs[2]);
		M_DMTC1(REG_ITMP2, rd->argfltregs[1]);
	}

	M_ALD(rd->argintregs[0], REG_PV, -4 * 8); /* jni environement              */
	M_ALD(REG_ITMP3, REG_PV, -1 * 8);   /* load adress of native method       */
	M_JSR(REG_RA, REG_ITMP3);           /* call native method                 */
	M_NOP;                              /* delay slot                         */

	/* remove stackframe if there is one */
	if (stackframesize) {
		M_LDA(REG_SP, REG_SP, stackframesize * 8);
	}

	/* 17 instructions */
	if (runverbose) {
		M_LDA(REG_SP, REG_SP, -(3 * 8));
		M_AST(REG_RA, REG_SP, 0 * 8);
		M_LST(REG_RESULT, REG_SP, 1 * 8);
		M_DST(REG_FRESULT, REG_SP, 2 * 8);
		M_ALD(rd->argintregs[0], REG_PV, -6 * 8);
		M_MOV(REG_RESULT, rd->argintregs[1]);
		M_DMFC1(REG_ITMP1, REG_FRESULT);
		M_DMTC1(REG_ITMP1, rd->argfltregs[2]);
		M_DMTC1(REG_ITMP1, rd->argfltregs[3]);
		M_ALD(REG_ITMP3, REG_PV, -7 * 8);/* builtin_displaymethodstop         */
		M_JSR(REG_RA, REG_ITMP3);
		M_NOP;
		disp = -(s4) (mcodeptr - (s4 *) cs) * 4;
		M_LDA(REG_PV, REG_RA, disp);
		M_ALD(REG_RA, REG_SP, 0 * 8);
		M_LLD(REG_RESULT, REG_SP, 1 * 8);
		M_DLD(REG_FRESULT, REG_SP, 2 * 8);
		M_LDA(REG_SP, REG_SP, 3 * 8);
	}

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	M_ALD(REG_ITMP3, REG_PV, -2 * 8);   /* builtin_get_exceptionptrptr        */
	M_JSR(REG_RA, REG_ITMP3);

	/* delay slot */
	if (IS_FLT_DBL_TYPE(m->returntype)) {
		M_DST(REG_FRESULT, REG_SP, 1 * 8);

	} else {
		M_AST(REG_RESULT, REG_SP, 1 * 8);
	}

	M_MOV(REG_RESULT, REG_ITMP3);

	if (IS_FLT_DBL_TYPE(m->returntype)) {
		M_DLD(REG_FRESULT, REG_SP, 1 * 8);

	} else {
		M_ALD(REG_RESULT, REG_SP, 1 * 8);
	}
#else
	M_ALD(REG_ITMP3, REG_PV, -2 * 8);   /* get address of exceptionptr        */
#endif

	M_LLD(REG_RA, REG_SP, 0);           /* load return address                */
	M_ALD(REG_ITMP1, REG_ITMP3, 0);     /* load exception into reg. itmp1     */

	M_BNEZ(REG_ITMP1, 2);               /* if no exception then return        */
	M_LDA(REG_SP, REG_SP, NATIVESTUB_STACK * 8);/*remove stackframe, delay slot*/

	M_RET(REG_RA);                      /* return to caller                   */
	M_NOP;                              /* delay slot                         */
	
	M_AST(REG_ZERO, REG_ITMP3, 0);      /* store NULL into exceptionptr       */
	M_ALD(REG_ITMP3, REG_PV, -3 * 8);   /* load asm exception handler address */

	M_JMP(REG_ITMP3);                   /* jump to asm exception handler      */
	M_LDA(REG_ITMP2, REG_RA, -4);       /* move fault address into reg. itmp2 */
	                                    /* delay slot                         */

	/* generate static stub call code                                         */

	{
		s4          *xcodeptr;
		clinitref   *cref;
		s4          *tmpmcodeptr;

		/* there can only be one clinit ref entry                             */
		cref = cd->clinitrefs;

		if (cref) {
			/* Get machine code which is patched back in later. The call is   */
			/* 2 instruction words long.                                      */
			xcodeptr = (s4 *) (cd->mcodebase + cref->branchpos);

			/* We need to split this, because an unaligned 8 byte read causes */
			/* a SIGSEGV.                                                     */
			*(cs-10) = ((u8) xcodeptr[1] << 32) + (u4) xcodeptr[0];

			/* patch in the call to call the following code (done at compile  */
			/* time)                                                          */

			tmpmcodeptr = mcodeptr;         /* save current mcodeptr          */
			mcodeptr = xcodeptr;            /* set mcodeptr to patch position */

			M_BRS(tmpmcodeptr - (xcodeptr + 1));
			M_NOP;

			mcodeptr = tmpmcodeptr;         /* restore the current mcodeptr   */

			/* move class pointer into REG_ITMP1                              */
			M_ALD(REG_ITMP1, REG_PV, -8 * 8);     /* class                    */

			/* move machine code into REG_ITMP2                               */
			M_LLD(REG_ITMP2, REG_PV, -10 * 8);    /* machine code             */

			M_ALD(REG_ITMP3, REG_PV, -9 * 8);     /* asm_check_clinit         */
			M_JMP(REG_ITMP3);
			M_NOP;
		}
	}

	(void) docacheflush((void*) cs, (char*) mcodeptr - (char*) cs);

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
	CFREE((u8 *) stub - NATIVESTUB_OFFSET, NATIVESTUB_SIZE * 8);
}


void docacheflush(u1 *p, long bytelen)
{
	u1 *e = p + bytelen;
	long psize = sysconf(_SC_PAGESIZE);
	p -= (long) p & (psize-1);
	e += psize - ((((long) e - 1) & (psize-1)) + 1);
	bytelen = e-p;
	mprotect(p, bytelen, PROT_READ|PROT_WRITE|PROT_EXEC);
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
