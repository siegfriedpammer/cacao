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

   $Id: codegen.c 559 2003-11-02 23:20:06Z twisti $

*/


#include <stdio.h>
#include <signal.h>
#include "types.h"
#include "codegen.h"
#include "jit.h"
#include "reg.h"
#include "builtin.h"
#include "asmpart.h"
#include "jni.h"
#include "loader.h"
#include "tables.h"
#include "native.h"
#include "methodtable.h"

/* include independent code generation stuff */
#include "codegen.inc"
#include "reg.inc"


/* additional functions and macros to generate code ***************************/

#define BlockPtrOfPC(pc)  ((basicblock *) iptr->target)


#ifdef STATISTICS
#define COUNT_SPILLS count_spills++
#else
#define COUNT_SPILLS
#endif


#define CALCOFFSETBYTES(var, reg, val) \
    if ((s4) (val) < -128 || (s4) (val) > 127) (var) += 4; \
    else if ((s4) (val) != 0) (var) += 1; \
    else if ((reg) == RBP || (reg) == RSP || (reg) == R12 || (reg) == R13) (var) += 1;


#define CALCREGOFFBYTES(var, val) \
    if ((val) > 15) (var) += 4; \
    else if ((val) != 0) (var) += 1;


#define CALCIMMEDIATEBYTES(var, val) \
    if ((s4) (val) < -128 || (s4) (val) > 127) (var) += 4; \
    else (var) += 1;


/* gen_nullptr_check(objreg) */

#ifdef SOFTNULLPTRCHECK
#define gen_nullptr_check(objreg) \
	if (checknull) { \
        x86_64_test_reg_reg((objreg), (objreg)); \
        x86_64_jcc(X86_64_CC_E, 0); \
 	    codegen_addxnullrefs(mcodeptr); \
	}
#else
#define gen_nullptr_check(objreg)
#endif


/* MCODECHECK(icnt) */

#define MCODECHECK(icnt) \
	if ((mcodeptr + (icnt)) > (u1*) mcodeend) mcodeptr = (u1*) codegen_increase((u1*) mcodeptr)

/* M_INTMOVE:
    generates an integer-move from register a to b.
    if a and b are the same int-register, no code will be generated.
*/ 

#define M_INTMOVE(reg,dreg) \
    if ((reg) != (dreg)) { \
        x86_64_mov_reg_reg((reg),(dreg)); \
    }


/* M_FLTMOVE:
    generates a floating-point-move from register a to b.
    if a and b are the same float-register, no code will be generated
*/ 

#define M_FLTMOVE(reg,dreg) \
    if ((reg) != (dreg)) { \
        x86_64_movq_reg_reg((reg),(dreg)); \
    }


/* var_to_reg_xxx:
    this function generates code to fetch data from a pseudo-register
    into a real register. 
    If the pseudo-register has actually been assigned to a real 
    register, no code will be emitted, since following operations
    can use this register directly.
    
    v: pseudoregister to be fetched from
    tempregnum: temporary register to be used if v is actually spilled to ram

    return: the register number, where the operand can be found after 
            fetching (this wil be either tempregnum or the register
            number allready given to v)
*/

#define var_to_reg_int(regnr,v,tempnr) \
    if ((v)->flags & INMEMORY) { \
        COUNT_SPILLS; \
        if ((v)->type == TYPE_INT) { \
            x86_64_movl_membase_reg(REG_SP, (v)->regoff * 8, tempnr); \
        } else { \
            x86_64_mov_membase_reg(REG_SP, (v)->regoff * 8, tempnr); \
        } \
        regnr = tempnr; \
    } else { \
        regnr = (v)->regoff; \
    }



#define var_to_reg_flt(regnr,v,tempnr) \
    if ((v)->flags & INMEMORY) { \
        COUNT_SPILLS; \
        if ((v)->type == TYPE_FLT) { \
            x86_64_movlps_membase_reg(REG_SP, (v)->regoff * 8, tempnr); \
        } else { \
            x86_64_movlpd_membase_reg(REG_SP, (v)->regoff * 8, tempnr); \
        } \
/*        x86_64_movq_membase_reg(REG_SP, (v)->regoff * 8, tempnr);*/ \
        regnr = tempnr; \
    } else { \
        regnr = (v)->regoff; \
    }


/* reg_of_var:
    This function determines a register, to which the result of an operation
    should go, when it is ultimatively intended to store the result in
    pseudoregister v.
    If v is assigned to an actual register, this register will be returned.
    Otherwise (when v is spilled) this function returns tempregnum.
    If not already done, regoff and flags are set in the stack location.
*/        

static int reg_of_var(stackptr v, int tempregnum)
{
	varinfo      *var;

	switch (v->varkind) {
		case TEMPVAR:
			if (!(v->flags & INMEMORY))
				return(v->regoff);
			break;
		case STACKVAR:
			var = &(interfaces[v->varnum][v->type]);
			v->regoff = var->regoff;
			if (!(var->flags & INMEMORY))
				return(var->regoff);
			break;
		case LOCALVAR:
			var = &(locals[v->varnum][v->type]);
			v->regoff = var->regoff;
			if (!(var->flags & INMEMORY))
				return(var->regoff);
			break;
		case ARGVAR:
			v->regoff = v->varnum;
			if (IS_FLT_DBL_TYPE(v->type)) {
				if (v->varnum < fltreg_argnum) {
					v->regoff = argfltregs[v->varnum];
					return(argfltregs[v->varnum]);
				}
			} else {
				if (v->varnum < intreg_argnum) {
					v->regoff = argintregs[v->varnum];
					return(argintregs[v->varnum]);
				}
			}
			v->regoff -= intreg_argnum;
			break;
		}
	v->flags |= INMEMORY;
	return tempregnum;
}


/* store_reg_to_var_xxx:
    This function generates the code to store the result of an operation
    back into a spilled pseudo-variable.
    If the pseudo-variable has not been spilled in the first place, this 
    function will generate nothing.
    
    v ............ Pseudovariable
    tempregnum ... Number of the temporary registers as returned by
                   reg_of_var.
*/	

#define store_reg_to_var_int(sptr, tempregnum) \
    if ((sptr)->flags & INMEMORY) { \
        COUNT_SPILLS; \
        x86_64_mov_reg_membase(tempregnum, REG_SP, (sptr)->regoff * 8); \
    }


#define store_reg_to_var_flt(sptr, tempregnum) \
    if ((sptr)->flags & INMEMORY) { \
         COUNT_SPILLS; \
         x86_64_movq_reg_membase(tempregnum, REG_SP, (sptr)->regoff * 8); \
    }


/* NullPointerException signal handler for hardware null pointer check */

void catch_NullPointerException(int sig, siginfo_t *siginfo, void *_p)
{
	sigset_t nsig;
/*  	int      instr; */
/*  	long     faultaddr; */

	struct ucontext *_uc = (struct ucontext *) _p;
	struct sigcontext *sigctx = (struct sigcontext *) &_uc->uc_mcontext;

	/* Reset signal handler - necessary for SysV, does no harm for BSD */

	
/*	instr = *((int*)(sigctx->rip)); */
/*    	faultaddr = sigctx->sc_regs[(instr >> 16) & 0x1f]; */

/*  	if (faultaddr == 0) { */
		signal(sig, (void *) catch_NullPointerException);          /* reinstall handler */
		sigemptyset(&nsig);
		sigaddset(&nsig, sig);
		sigprocmask(SIG_UNBLOCK, &nsig, NULL);                     /* unblock signal    */
		sigctx->rax = (long) proto_java_lang_NullPointerException; /* REG_ITMP1_XPTR    */
		sigctx->r10 = sigctx->rip;                                 /* REG_ITMP2_XPC     */
		sigctx->rip = (long) asm_handle_exception;

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

	classinfo *c;
	java_objectheader *p;
	methodinfo *m;

	/* Reset signal handler - necessary for SysV, does no harm for BSD */

	signal(sig, (void *) catch_ArithmeticException);     /* reinstall handler */
	sigemptyset(&nsig);
	sigaddset(&nsig, sig);
	sigprocmask(SIG_UNBLOCK, &nsig, NULL);               /* unblock signal    */

	c = loader_load(utf_new_char("java/lang/ArithmeticException"));
	p = builtin_new(c);
	m = class_findmethod(c, 
						 utf_new_char("<init>"), 
						 utf_new_char("(Ljava/lang/String;)V"));

	asm_calljavamethod(m, p, javastring_new_char("/ by zero"), NULL, NULL);

	sigctx->rax = (long) p;                              /* REG_ITMP1_XPTR    */
	sigctx->r10 = sigctx->rip;                           /* REG_ITMP2_XPC     */
	sigctx->rip = (long) asm_handle_exception;

	return;
}


void init_exceptions(void)
{
	/* install signal handlers we need to convert to exceptions */

	if (!checknull) {
#if defined(SIGSEGV)
		signal(SIGSEGV, (void *) catch_NullPointerException);
#endif

#if defined(SIGBUS)
		signal(SIGBUS, (void *) catch_NullPointerException);
#endif
	}

	signal(SIGFPE, (void *) catch_ArithmeticException);
}


/* function gen_mcode **********************************************************

	generates machine code

*******************************************************************************/

u1          *mcodeptr;

static void codegen()
{
	int  len, s1, s2, s3, d /*, bbs */;
	s8   a;
	stackptr    src;
	varinfo     *var;
/*  	varinfo     *dst; */
	basicblock  *bptr;
	instruction *iptr;

	xtable *ex;

	{
	int p, pa, t, l, r;

  	savedregs_num = 0;

	/* space to save used callee saved registers */

	savedregs_num += (savintregcnt - maxsavintreguse);
	savedregs_num += (savfltregcnt - maxsavfltreguse);

	parentargs_base = maxmemuse + savedregs_num;

#ifdef USE_THREADS                 /* space to save argument of monitor_enter */

	if (checksync && (method->flags & ACC_SYNCHRONIZED))
		parentargs_base++;

#endif

    /* keep stack 16-byte aligned for calls into libc */

	if (!isleafmethod || runverbose) {
		if ((parentargs_base % 2) == 0) {
			parentargs_base++;
		}
	}

	/* create method header */

	(void) dseg_addaddress(method);                         /* MethodPointer  */
	(void) dseg_adds4(parentargs_base * 8);                 /* FrameSize      */

#ifdef USE_THREADS

	/* IsSync contains the offset relative to the stack pointer for the
	   argument of monitor_exit used in the exception handler. Since the
	   offset could be zero and give a wrong meaning of the flag it is
	   offset by one.
	*/

	if (checksync && (method->flags & ACC_SYNCHRONIZED))
		(void) dseg_adds4((maxmemuse + 1) * 8);             /* IsSync         */
	else

#endif

	(void) dseg_adds4(0);                                   /* IsSync         */
	                                       
	(void) dseg_adds4(isleafmethod);                        /* IsLeaf         */
	(void) dseg_adds4(savintregcnt - maxsavintreguse);      /* IntSave        */
	(void) dseg_adds4(savfltregcnt - maxsavfltreguse);      /* FltSave        */
	(void) dseg_adds4(exceptiontablelength);                /* ExTableSize    */

	/* create exception table */

	for (ex = extable; ex != NULL; ex = ex->down) {

#ifdef LOOP_DEBUG	
		if (ex->start != NULL)
			printf("adding start - %d - ", ex->start->debug_nr);
		else {
			printf("PANIC - start is NULL");
			exit(-1);
		}
#endif

		dseg_addtarget(ex->start);

#ifdef LOOP_DEBUG			
		if (ex->end != NULL)
			printf("adding end - %d - ", ex->end->debug_nr);
		else {
			printf("PANIC - end is NULL");
			exit(-1);
		}
#endif

   		dseg_addtarget(ex->end);

#ifdef LOOP_DEBUG		
		if (ex->handler != NULL)
			printf("adding handler - %d\n", ex->handler->debug_nr);
		else {
			printf("PANIC - handler is NULL");
			exit(-1);
		}
#endif

		dseg_addtarget(ex->handler);
	   
		(void) dseg_addaddress(ex->catchtype);
	}
	
	/* initialize mcode variables */
	
	mcodeptr = (u1*) mcodebase;
	mcodeend = (s4*) (mcodebase + mcodesize);
	MCODECHECK(128 + mparamcount);

	/* create stack frame (if necessary) */

	if (parentargs_base) {
		x86_64_alu_imm_reg(X86_64_SUB, parentargs_base * 8, REG_SP);
	}

	/* save return address and used callee saved registers */

  	p = parentargs_base;
	for (r = savintregcnt - 1; r >= maxsavintreguse; r--) {
 		p--; x86_64_mov_reg_membase(savintregs[r], REG_SP, p * 8);
	}
	for (r = savfltregcnt - 1; r >= maxsavfltreguse; r--) {
		p--; x86_64_movq_reg_membase(savfltregs[r], REG_SP, p * 8);
	}

	/* save monitorenter argument */

#ifdef USE_THREADS
	if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
		if (method->flags & ACC_STATIC) {
			x86_64_mov_imm_reg((s8) class, REG_ITMP1);
			x86_64_mov_reg_membase(REG_ITMP1, REG_SP, maxmemuse * 8);

		} else {
			x86_64_mov_reg_membase(argintregs[0], REG_SP, maxmemuse * 8);
		}
	}			
#endif

	/* copy argument registers to stack and call trace function with pointer
	   to arguments on stack.
	*/
	if (runverbose) {
		x86_64_alu_imm_reg(X86_64_SUB, (6 + 8 + 1 + 1) * 8, REG_SP);

		x86_64_mov_reg_membase(argintregs[0], REG_SP, 1 * 8);
		x86_64_mov_reg_membase(argintregs[1], REG_SP, 2 * 8);
		x86_64_mov_reg_membase(argintregs[2], REG_SP, 3 * 8);
		x86_64_mov_reg_membase(argintregs[3], REG_SP, 4 * 8);
		x86_64_mov_reg_membase(argintregs[4], REG_SP, 5 * 8);
		x86_64_mov_reg_membase(argintregs[5], REG_SP, 6 * 8);

		x86_64_movq_reg_membase(argfltregs[0], REG_SP, 7 * 8);
		x86_64_movq_reg_membase(argfltregs[1], REG_SP, 8 * 8);
		x86_64_movq_reg_membase(argfltregs[2], REG_SP, 9 * 8);
		x86_64_movq_reg_membase(argfltregs[3], REG_SP, 10 * 8);
/*  		x86_64_movq_reg_membase(argfltregs[4], REG_SP, 11 * 8); */
/*  		x86_64_movq_reg_membase(argfltregs[5], REG_SP, 12 * 8); */
/*  		x86_64_movq_reg_membase(argfltregs[6], REG_SP, 13 * 8); */
/*  		x86_64_movq_reg_membase(argfltregs[7], REG_SP, 14 * 8); */

		for (p = 0, l = 0; p < mparamcount; p++) {
			t = mparamtypes[p];

			if (IS_FLT_DBL_TYPE(t)) {
				for (s1 = (mparamcount > intreg_argnum) ? intreg_argnum - 2 : mparamcount - 2; s1 >= p; s1--) {
					x86_64_mov_reg_reg(argintregs[s1], argintregs[s1 + 1]);
				}

				x86_64_movd_freg_reg(argfltregs[l], argintregs[p]);
				l++;
			}
		}

		x86_64_mov_imm_reg((s8) method, REG_ITMP2);
		x86_64_mov_reg_membase(REG_ITMP2, REG_SP, 0 * 8);
		x86_64_mov_imm_reg((s8) builtin_trace_args, REG_ITMP1);
		x86_64_call_reg(REG_ITMP1);

		x86_64_mov_membase_reg(REG_SP, 1 * 8, argintregs[0]);
		x86_64_mov_membase_reg(REG_SP, 2 * 8, argintregs[1]);
		x86_64_mov_membase_reg(REG_SP, 3 * 8, argintregs[2]);
		x86_64_mov_membase_reg(REG_SP, 4 * 8, argintregs[3]);
		x86_64_mov_membase_reg(REG_SP, 5 * 8, argintregs[4]);
		x86_64_mov_membase_reg(REG_SP, 6 * 8, argintregs[5]);

		x86_64_movq_membase_reg(REG_SP, 7 * 8, argfltregs[0]);
		x86_64_movq_membase_reg(REG_SP, 8 * 8, argfltregs[1]);
		x86_64_movq_membase_reg(REG_SP, 9 * 8, argfltregs[2]);
		x86_64_movq_membase_reg(REG_SP, 10 * 8, argfltregs[3]);
/*  		x86_64_movq_membase_reg(REG_SP, 11 * 8, argfltregs[4]); */
/*  		x86_64_movq_membase_reg(REG_SP, 12 * 8, argfltregs[5]); */
/*  		x86_64_movq_membase_reg(REG_SP, 13 * 8, argfltregs[6]); */
/*  		x86_64_movq_membase_reg(REG_SP, 14 * 8, argfltregs[7]); */

		x86_64_alu_imm_reg(X86_64_ADD, (6 + 8 + 1 + 1) * 8, REG_SP);
	}

	/* take arguments out of register or stack frame */

 	for (p = 0, l = 0, s1 = 0, s2 = 0; p < mparamcount; p++) {
 		t = mparamtypes[p];
 		var = &(locals[l][t]);
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
 		r = var->regoff; 
		if (IS_INT_LNG_TYPE(t)) {                    /* integer args          */
 			if (s1 < intreg_argnum) {                /* register arguments    */
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
   					M_INTMOVE(argintregs[s1], r);

				} else {                             /* reg arg -> spilled    */
   				    x86_64_mov_reg_membase(argintregs[s1], REG_SP, r * 8);
 				}
			} else {                                 /* stack arguments       */
 				pa = s1 - intreg_argnum;
				if (s2 >= fltreg_argnum) {
					pa += s2 - fltreg_argnum;
				}
 				if (!(var->flags & INMEMORY)) {      /* stack arg -> register */ 
 					x86_64_mov_membase_reg(REG_SP, (parentargs_base + pa) * 8 + 8, r);    /* + 8 for return address */
				} else {                             /* stack arg -> spilled  */
					x86_64_mov_membase_reg(REG_SP, (parentargs_base + pa) * 8 + 8, REG_ITMP1);    /* + 8 for return address */
					x86_64_mov_reg_membase(REG_ITMP1, REG_SP, r * 8);
				}
			}
			s1++;

		} else {                                     /* floating args         */   
 			if (s2 < fltreg_argnum) {                /* register arguments    */
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
					M_FLTMOVE(argfltregs[s2], r);

 				} else {			                 /* reg arg -> spilled    */
					x86_64_movq_reg_membase(argfltregs[s2], REG_SP, r * 8);
 				}

 			} else {                                 /* stack arguments       */
 				pa = s2 - fltreg_argnum;
				if (s1 >= intreg_argnum) {
					pa += s1 - intreg_argnum;
				}
 				if (!(var->flags & INMEMORY)) {      /* stack-arg -> register */
					x86_64_movq_membase_reg(REG_SP, (parentargs_base + pa) * 8 + 8, r);

				} else {
					x86_64_movq_membase_reg(REG_SP, (parentargs_base + pa) * 8 + 8, REG_FTMP1);
					x86_64_movq_reg_membase(REG_FTMP1, REG_SP, r * 8);
				}
			}
			s2++;
		}
	}  /* end for */

	/* call monitorenter function */

#ifdef USE_THREADS
	if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
		x86_64_mov_membase_reg(REG_SP, 8 * maxmemuse, argintregs[0]);
		x86_64_mov_imm_reg((s8) builtin_monitorenter, REG_ITMP1);
		x86_64_call_reg(REG_ITMP1);
	}			
#endif
	}

	/* end of header generation */

	/* walk through all basic blocks */
	for (/* bbs = block_count, */ bptr = block; /* --bbs >= 0 */ bptr != NULL; bptr = bptr->next) {

		bptr->mpc = (int)((u1*) mcodeptr - mcodebase);

		if (bptr->flags >= BBREACHED) {

		/* branch resolving */

		branchref *brefs;
		for (brefs = bptr->branchrefs; brefs != NULL; brefs = brefs->next) {
			gen_resolvebranch((u1*) mcodebase + brefs->branchpos, 
			                  brefs->branchpos, bptr->mpc);
		}

		/* copy interface registers to their destination */

		src = bptr->instack;
		len = bptr->indepth;
		MCODECHECK(64+len);
		while (src != NULL) {
			len--;
  			if ((len == 0) && (bptr->type != BBTYPE_STD)) {
				if (bptr->type == BBTYPE_SBR) {
					d = reg_of_var(src, REG_ITMP1);
					x86_64_pop_reg(d);
					store_reg_to_var_int(src, d);

				} else if (bptr->type == BBTYPE_EXH) {
					d = reg_of_var(src, REG_ITMP1);
					M_INTMOVE(REG_ITMP1, d);
					store_reg_to_var_int(src, d);
				}

			} else {
				d = reg_of_var(src, REG_ITMP1);
				if ((src->varkind != STACKVAR)) {
					s2 = src->type;
					if (IS_FLT_DBL_TYPE(s2)) {
						s1 = interfaces[len][s2].regoff;
						if (!(interfaces[len][s2].flags & INMEMORY)) {
							M_FLTMOVE(s1, d);

						} else {
							x86_64_movq_membase_reg(REG_SP, s1 * 8, d);
						}
						store_reg_to_var_flt(src, d);

					} else {
						s1 = interfaces[len][s2].regoff;
						if (!(interfaces[len][s2].flags & INMEMORY)) {
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
		for (iptr = bptr->iinstr;
		    len > 0;
		    src = iptr->dst, len--, iptr++) {

	MCODECHECK(64);           /* an instruction usually needs < 64 words      */
	switch (iptr->opc) {

		case ICMD_NOP:        /* ...  ==> ...                                 */
			break;

		case ICMD_NULLCHECKPOP: /* ..., objectref  ==> ...                    */
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

/*  			d = reg_of_var(iptr->dst, REG_ITMP1); */
/*  			if (iptr->dst->flags & INMEMORY) { */
/*  				x86_64_movl_imm_membase(iptr->val.i, REG_SP, iptr->dst->regoff * 8); */

/*  			} else { */
/*  				x86_64_movl_imm_reg(iptr->val.i, d); */
/*  			} */
			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->val.i == 0) {
				x86_64_alu_reg_reg(X86_64_XOR, d, d);
			} else {
				x86_64_movl_imm_reg(iptr->val.i, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ACONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.a = constant                    */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->val.a == 0) {
				x86_64_alu_reg_reg(X86_64_XOR, d, d);
			} else {
				x86_64_mov_imm_reg((s8) iptr->val.a, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.l = constant                    */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->val.l == 0) {
				x86_64_alu_reg_reg(X86_64_XOR, d, d);
			} else {
				x86_64_mov_imm_reg(iptr->val.l, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.f = constant                    */

			d = reg_of_var(iptr->dst, REG_FTMP1);
			a = dseg_addfloat(iptr->val.f);
			x86_64_movdl_membase_reg(RIP, -(((s8) mcodeptr + ((d > 7) ? 9 : 8)) - (s8) mcodebase) + a, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
		
		case ICMD_DCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.d = constant                    */

			d = reg_of_var(iptr->dst, REG_FTMP1);
			a = dseg_adddouble(iptr->val.d);
			x86_64_movd_membase_reg(RIP, -(((s8) mcodeptr + 9) - (s8) mcodebase) + a, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;


		/* load/store operations **********************************************/

		case ICMD_ILOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1)) {
				break;
			}
			var = &(locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
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

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1)) {
				break;
			}
			var = &(locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
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

			d = reg_of_var(iptr->dst, REG_FTMP1);
  			if ((iptr->dst->varkind == LOCALVAR) &&
  			    (iptr->dst->varnum == iptr->op1)) {
    				break;
  			}
			var = &(locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
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
			var = &(locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
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
			var = &(locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
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
	        d = reg_of_var(to, REG_ITMP1); \
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

			d = reg_of_var(iptr->dst, REG_NULL);
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

			d = reg_of_var(iptr->dst, REG_NULL);
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

			d = reg_of_var(iptr->dst, REG_ITMP3);
			if (src->flags & INMEMORY) {
				x86_64_movslq_membase_reg(REG_SP, src->regoff * 8, d);

			} else {
				x86_64_movslq_reg_reg(src->regoff, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			d = reg_of_var(iptr->dst, REG_ITMP3);
			if (src->flags & INMEMORY) {
				x86_64_movsbq_membase_reg(REG_SP, src->regoff * 8, d);

			} else {
				x86_64_movsbq_reg_reg(src->regoff, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2CHAR:   /* ..., value  ==> ..., value                   */

			d = reg_of_var(iptr->dst, REG_ITMP3);
			if (src->flags & INMEMORY) {
				x86_64_movzwq_membase_reg(REG_SP, src->regoff * 8, d);

			} else {
				x86_64_movzwq_reg_reg(src->regoff, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2SHORT:  /* ..., value  ==> ..., value                   */

			d = reg_of_var(iptr->dst, REG_ITMP3);
			if (src->flags & INMEMORY) {
				x86_64_movswq_membase_reg(REG_SP, src->regoff * 8, d);

			} else {
				x86_64_movswq_reg_reg(src->regoff, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_IADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_ialu(X86_64_ADD, src, iptr);
			break;

		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_ialuconst(X86_64_ADD, src, iptr);
			break;

		case ICMD_LADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_lalu(X86_64_ADD, src, iptr);
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_laluconst(X86_64_ADD, src, iptr);
			break;

		case ICMD_ISUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
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

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_ialuconst(X86_64_SUB, src, iptr);
			break;

		case ICMD_LSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
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

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_laluconst(X86_64_SUB, src, iptr);
			break;

		case ICMD_IMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
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

			d = reg_of_var(iptr->dst, REG_NULL);
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

			d = reg_of_var(iptr->dst, REG_NULL);
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

			d = reg_of_var(iptr->dst, REG_NULL);
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
					x86_64_imul_imm_membase_reg(iptr->val.l, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else {
					if (iptr->val.l == 2) {
						M_INTMOVE(src->regoff, iptr->dst->regoff);
						x86_64_alul_reg_reg(X86_64_ADD, iptr->dst->regoff, iptr->dst->regoff);

					} else {
						x86_64_imul_imm_reg_reg(iptr->val.l, src->regoff, iptr->dst->regoff);    /* 4 cycles */
					}
				}
			}
			break;

		case ICMD_IDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
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

			d = reg_of_var(iptr->dst, REG_NULL);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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

			d = reg_of_var(iptr->dst, REG_NULL);
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

			d = reg_of_var(iptr->dst, REG_NULL);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_ishift(X86_64_SHL, src, iptr);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_ishiftconst(X86_64_SHL, src, iptr);
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_ishift(X86_64_SAR, src, iptr);
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_ishiftconst(X86_64_SAR, src, iptr);
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_ishift(X86_64_SHR, src, iptr);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_ishiftconst(X86_64_SHR, src, iptr);
			break;

		case ICMD_LSHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_lshift(X86_64_SHL, src, iptr);
			break;

        case ICMD_LSHLCONST:  /* ..., value  ==> ..., value << constant       */
 			                  /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_lshiftconst(X86_64_SHL, src, iptr);
			break;

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_lshift(X86_64_SAR, src, iptr);
			break;

		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_lshiftconst(X86_64_SAR, src, iptr);
			break;

		case ICMD_LUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_lshift(X86_64_SHR, src, iptr);
			break;

  		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
  		                      /* val.l = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_lshiftconst(X86_64_SHR, src, iptr);
			break;

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_ialu(X86_64_AND, src, iptr);
			break;

		case ICMD_IANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_ialuconst(X86_64_AND, src, iptr);
			break;

		case ICMD_LAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_lalu(X86_64_AND, src, iptr);
			break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_laluconst(X86_64_AND, src, iptr);
			break;

		case ICMD_IOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_ialu(X86_64_OR, src, iptr);
			break;

		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_ialuconst(X86_64_OR, src, iptr);
			break;

		case ICMD_LOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_lalu(X86_64_OR, src, iptr);
			break;

		case ICMD_LORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_laluconst(X86_64_OR, src, iptr);
			break;

		case ICMD_IXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_ialu(X86_64_XOR, src, iptr);
			break;

		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_ialuconst(X86_64_XOR, src, iptr);
			break;

		case ICMD_LXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_lalu(X86_64_XOR, src, iptr);
			break;

		case ICMD_LXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			x86_64_emit_laluconst(X86_64_XOR, src, iptr);
			break;


		case ICMD_IINC:       /* ..., value  ==> ..., value + constant        */
		                      /* op1 = variable, val.i = constant             */

			var = &(locals[iptr->op1][TYPE_INT]);
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
			d = reg_of_var(iptr->dst, REG_FTMP3);
			a = dseg_adds4(0x80000000);
			M_FLTMOVE(s1, d);
			x86_64_movss_membase_reg(RIP, -(((s8) mcodeptr + 9) - (s8) mcodebase) + a, REG_FTMP2);
			x86_64_xorps_reg_reg(REG_FTMP2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			a = dseg_adds8(0x8000000000000000);
			M_FLTMOVE(s1, d);
			x86_64_movd_membase_reg(RIP, -(((s8) mcodeptr + 9) - (s8) mcodebase) + a, REG_FTMP2);
			x86_64_xorpd_reg_reg(REG_FTMP2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_FTMP3);
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
			d = reg_of_var(iptr->dst, REG_FTMP3);
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
			d = reg_of_var(iptr->dst, REG_FTMP3);
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
			d = reg_of_var(iptr->dst, REG_FTMP3);
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
			d = reg_of_var(iptr->dst, REG_FTMP3);
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
			d = reg_of_var(iptr->dst, REG_FTMP3);
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
			d = reg_of_var(iptr->dst, REG_FTMP3);
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
			d = reg_of_var(iptr->dst, REG_FTMP3);
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
			d = reg_of_var(iptr->dst, REG_FTMP1);
			x86_64_cvtsi2ss_reg_reg(s1, d);
  			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_I2D:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_FTMP1);
			x86_64_cvtsi2sd_reg_reg(s1, d);
  			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_L2F:       /* ..., value  ==> ..., (float) value            */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_FTMP1);
			x86_64_cvtsi2ssq_reg_reg(s1, d);
  			store_reg_to_var_flt(iptr->dst, d);
			break;
			
		case ICMD_L2D:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_FTMP1);
			x86_64_cvtsi2sdq_reg_reg(s1, d);
  			store_reg_to_var_flt(iptr->dst, d);
			break;
			
		case ICMD_F2I:       /* ..., value  ==> ..., (int) value              */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_ITMP1);
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
			d = reg_of_var(iptr->dst, REG_ITMP1);
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
			d = reg_of_var(iptr->dst, REG_ITMP1);
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
			d = reg_of_var(iptr->dst, REG_ITMP1);
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
			d = reg_of_var(iptr->dst, REG_FTMP3);
			x86_64_cvtss2sd_reg_reg(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_D2F:       /* ..., value  ==> ..., (float) value            */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			x86_64_cvtsd2ss_reg_reg(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */
 			                  /* == => 0, < => 1, > => -1 */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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

#define gen_bound_check \
    if (checkbounds) { \
        x86_64_alul_membase_reg(X86_64_CMP, s1, OFFSET(java_arrayheader, size), s2); \
        x86_64_jcc(X86_64_CC_AE, 0); \
        codegen_addxboundrefs(mcodeptr); \
    }

		case ICMD_ARRAYLENGTH: /* ..., arrayref  ==> ..., (int) length        */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			gen_nullptr_check(s1);
			x86_64_movl_membase_reg(s1, OFFSET(java_arrayheader, size), d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_AALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_FTMP3);
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
			d = reg_of_var(iptr->dst, REG_FTMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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

			a = dseg_addaddress(&(((fieldinfo *)(iptr->val.a))->value));
/*    			x86_64_mov_imm_reg(0, REG_ITMP2); */
/*    			dseg_adddata(mcodeptr); */
/*    			x86_64_mov_membase_reg(REG_ITMP2, a, REG_ITMP3); */
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

			a = dseg_addaddress(&(((fieldinfo *)(iptr->val.a))->value));
/*  			x86_64_mov_imm_reg(0, REG_ITMP2); */
/*  			dseg_adddata(mcodeptr); */
/*  			x86_64_mov_membase_reg(REG_ITMP2, a, REG_ITMP3); */
			x86_64_mov_membase_reg(RIP, -(((s8) mcodeptr + 7) - (s8) mcodebase) + a, REG_ITMP2);
			switch (iptr->op1) {
				case TYPE_INT:
					d = reg_of_var(iptr->dst, REG_ITMP1);
					x86_64_movl_membase_reg(REG_ITMP2, 0, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
				case TYPE_LNG:
				case TYPE_ADR:
					d = reg_of_var(iptr->dst, REG_ITMP1);
					x86_64_mov_membase_reg(REG_ITMP2, 0, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
				case TYPE_FLT:
					d = reg_of_var(iptr->dst, REG_ITMP1);
					x86_64_movss_membase_reg(REG_ITMP2, 0, d);
					store_reg_to_var_flt(iptr->dst, d);
					break;
				case TYPE_DBL:				
					d = reg_of_var(iptr->dst, REG_ITMP1);
					x86_64_movsd_membase_reg(REG_ITMP2, 0, d);
					store_reg_to_var_flt(iptr->dst, d);
					break;
				default: panic("internal error");
				}
			break;

		case ICMD_PUTFIELD:   /* ..., value  ==> ...                          */
		                      /* op1 = type, val.i = field offset             */

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
					d = reg_of_var(iptr->dst, REG_ITMP1);
					gen_nullptr_check(s1);
					x86_64_movl_membase_reg(s1, a, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
				case TYPE_LNG:
				case TYPE_ADR:
					d = reg_of_var(iptr->dst, REG_ITMP1);
					gen_nullptr_check(s1);
					x86_64_mov_membase_reg(s1, a, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
				case TYPE_FLT:
					d = reg_of_var(iptr->dst, REG_FTMP1);
					gen_nullptr_check(s1);
					x86_64_movss_membase_reg(s1, a, d);
   					store_reg_to_var_flt(iptr->dst, d);
					break;
				case TYPE_DBL:				
					d = reg_of_var(iptr->dst, REG_FTMP1);
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

			var = &(locals[iptr->op1][TYPE_ADR]);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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

#ifdef USE_THREADS
			if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
				x86_64_mov_membase_reg(REG_SP, 8 * maxmemuse, argintregs[0]);
				x86_64_mov_imm_reg((s8) builtin_monitorexit, REG_ITMP1);
				x86_64_call_reg(REG_ITMP1);
			}
#endif
			var_to_reg_int(s1, src, REG_RESULT);
			M_INTMOVE(s1, REG_RESULT);
			goto nowperformreturn;

		case ICMD_FRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_DRETURN:

#ifdef USE_THREADS
			if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
				x86_64_mov_membase_reg(REG_SP, 8 * maxmemuse, argintregs[0]);
				x86_64_mov_imm_reg((s8) builtin_monitorexit, REG_ITMP1);
				x86_64_call_reg(REG_ITMP1);
			}
#endif
			var_to_reg_flt(s1, src, REG_FRESULT);
			M_FLTMOVE(s1, REG_FRESULT);
			goto nowperformreturn;

		case ICMD_RETURN:      /* ...  ==> ...                                */

#ifdef USE_THREADS
			if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
				x86_64_mov_membase_reg(REG_SP, 8 * maxmemuse, argintregs[0]);
				x86_64_mov_imm_reg((s8) builtin_monitorexit, REG_ITMP1);
				x86_64_call_reg(REG_ITMP1);
			}
#endif

nowperformreturn:
			{
			int r, p;
			
  			p = parentargs_base;
			
			/* call trace function */
			if (runverbose) {
				x86_64_alu_imm_reg(X86_64_SUB, 2 * 8, REG_SP);

				x86_64_mov_reg_membase(REG_RESULT, REG_SP, 0 * 8);
				x86_64_movq_reg_membase(REG_FRESULT, REG_SP, 1 * 8);

  				x86_64_mov_imm_reg((s8) method, argintregs[0]);
  				x86_64_mov_reg_reg(REG_RESULT, argintregs[1]);
				M_FLTMOVE(REG_FRESULT, argfltregs[0]);
 				M_FLTMOVE(REG_FRESULT, argfltregs[1]);

  				x86_64_mov_imm_reg((s8) builtin_displaymethodstop, REG_ITMP1);
				x86_64_call_reg(REG_ITMP1);

				x86_64_mov_membase_reg(REG_SP, 0 * 8, REG_RESULT);
				x86_64_movq_membase_reg(REG_SP, 1 * 8, REG_FRESULT);

				x86_64_alu_imm_reg(X86_64_ADD, 2 * 8, REG_SP);
			}

			/* restore saved registers                                        */
			for (r = savintregcnt - 1; r >= maxsavintreguse; r--) {
				p--; x86_64_mov_membase_reg(REG_SP, p * 8, savintregs[r]);
			}
			for (r = savfltregcnt - 1; r >= maxsavfltreguse; r--) {
  				p--; x86_64_movq_membase_reg(REG_SP, p * 8, savfltregs[r]);
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
			methodinfo   *m;
			classinfo    *ci;
			stackptr     tmpsrc;
			int iarg = 0;
			int farg = 0;

			MCODECHECK((s3 << 1) + 64);

			tmpsrc = src;
			s2 = s3;

			/* copy arguments to registers or stack location                  */
			for (; --s3 >= 0; src = src->prev) {
				IS_INT_LNG_TYPE(src->type) ? iarg++ : farg++;
			}

			src = tmpsrc;
			s3 = s2;

			s2 = (iarg > intreg_argnum) ? iarg - intreg_argnum : 0 + (farg > fltreg_argnum) ? farg - fltreg_argnum : 0;

			for (; --s3 >= 0; src = src->prev) {
				IS_INT_LNG_TYPE(src->type) ? iarg-- : farg--;
				if (src->varkind == ARGVAR) {
					if (IS_INT_LNG_TYPE(src->type)) {
						if (iarg >= intreg_argnum) {
							s2--;
						}
					} else {
						if (farg >= fltreg_argnum) {
							s2--;
						}
					}
					continue;
				}

				if (IS_INT_LNG_TYPE(src->type)) {
					if (iarg < intreg_argnum) {
						s1 = argintregs[iarg];
						var_to_reg_int(d, src, s1);
						M_INTMOVE(d, s1);

					} else {
						var_to_reg_int(d, src, REG_ITMP1);
						s2--;
						x86_64_mov_reg_membase(d, REG_SP, s2 * 8);
					}

				} else {
					if (farg < fltreg_argnum) {
						s1 = argfltregs[farg];
						var_to_reg_flt(d, src, s1);
						M_FLTMOVE(d, s1);

					} else {
						var_to_reg_flt(d, src, REG_FTMP1);
						s2--;
						x86_64_movq_reg_membase(d, REG_SP, s2 * 8);
					}
				}
			} /* end of for */

			m = iptr->val.a;
			switch (iptr->opc) {
				case ICMD_BUILTIN3:
				case ICMD_BUILTIN2:
				case ICMD_BUILTIN1:

					a = (s8) m;
					d = iptr->op1;

					x86_64_mov_imm_reg(a, REG_ITMP1);
					x86_64_call_reg(REG_ITMP1);
					break;

				case ICMD_INVOKESTATIC:

					a = (s8) m->stubroutine;
					d = m->returntype;

					x86_64_mov_imm_reg(a, REG_ITMP2);
					x86_64_call_reg(REG_ITMP2);
					break;

				case ICMD_INVOKESPECIAL:

					a = (s8) m->stubroutine;
					d = m->returntype;

					gen_nullptr_check(argintregs[0]);    /* first argument contains pointer */
					x86_64_mov_membase_reg(argintregs[0], 0, REG_ITMP2);    /* access memory for hardware nullptr */
					x86_64_mov_imm_reg(a, REG_ITMP2);
					x86_64_call_reg(REG_ITMP2);
					break;

				case ICMD_INVOKEVIRTUAL:

					d = m->returntype;

					gen_nullptr_check(argintregs[0]);
					x86_64_mov_membase_reg(argintregs[0], OFFSET(java_objectheader, vftbl), REG_ITMP2);
					x86_64_mov_membase32_reg(REG_ITMP2, OFFSET(vftbl, table[0]) + sizeof(methodptr) * m->vftblindex, REG_ITMP1);
					x86_64_call_reg(REG_ITMP1);
					break;

				case ICMD_INVOKEINTERFACE:

					ci = m->class;
					d = m->returntype;

					gen_nullptr_check(argintregs[0]);
					x86_64_mov_membase_reg(argintregs[0], OFFSET(java_objectheader, vftbl), REG_ITMP2);
					x86_64_mov_membase_reg(REG_ITMP2, OFFSET(vftbl, interfacetable[0]) - sizeof(methodptr) * ci->index, REG_ITMP2);
					x86_64_mov_membase32_reg(REG_ITMP2, sizeof(methodptr) * (m - ci->methods), REG_ITMP1);
					x86_64_call_reg(REG_ITMP1);
					break;

				default:
					d = 0;
					sprintf(logtext, "Unkown ICMD-Command: %d", iptr->opc);
					error();
				}

			/* d contains return type */

			if (d != TYPE_VOID) {
				if (IS_INT_LNG_TYPE(iptr->dst->type)) {
					s1 = reg_of_var(iptr->dst, REG_RESULT);
					M_INTMOVE(REG_RESULT, s1);
					store_reg_to_var_int(iptr->dst, s1);

				} else {
					s1 = reg_of_var(iptr->dst, REG_FRESULT);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
					CALCOFFSETBYTES(a, 0, super->index);
					
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
			
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
					CALCOFFSETBYTES(a, 0, super->index);

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
					x86_64_mov_reg_membase(s2, REG_SP, (s1 + intreg_argnum) * 8);
				}
			}

			/* a0 = dimension count */
			x86_64_mov_imm_reg(iptr->op1, argintregs[0]);

			/* a1 = arraydescriptor */
			x86_64_mov_imm_reg((s8) iptr->val.a, argintregs[1]);

			/* a2 = pointer to dimensions = stack pointer */
			x86_64_mov_reg_reg(REG_SP, argintregs[2]);

			x86_64_mov_imm_reg((s8) (builtin_nmultianewarray), REG_ITMP1);
			x86_64_call_reg(REG_ITMP1);

			s1 = reg_of_var(iptr->dst, REG_RESULT);
			M_INTMOVE(REG_RESULT, s1);
			store_reg_to_var_int(iptr->dst, s1);
			break;

		default: sprintf(logtext, "Unknown pseudo command: %d", iptr->opc);
		         error();
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
				if (!(interfaces[len][s2].flags & INMEMORY)) {
					M_FLTMOVE(s1, interfaces[len][s2].regoff);

				} else {
					x86_64_movq_reg_membase(s1, REG_SP, 8 * interfaces[len][s2].regoff);
				}

			} else {
				var_to_reg_int(s1, src, REG_ITMP1);
				if (!(interfaces[len][s2].flags & INMEMORY)) {
					M_INTMOVE(s1, interfaces[len][s2].regoff);

				} else {
					x86_64_mov_reg_membase(s1, REG_SP, interfaces[len][s2].regoff * 8);
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
		if ((exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch(mcodebase + xboundrefs->branchpos, 
				xboundrefs->branchpos, xcodeptr - mcodebase - (10 + 10 + 3));
			continue;
		}

		gen_resolvebranch(mcodebase + xboundrefs->branchpos, 
		                  xboundrefs->branchpos, mcodeptr - mcodebase);

		MCODECHECK(8);

		x86_64_mov_imm_reg(0, REG_ITMP2_XPC);    /* 10 bytes */
		dseg_adddata(mcodeptr);
		x86_64_mov_imm_reg(xboundrefs->branchpos - 6, REG_ITMP1);    /* 10 bytes */
		x86_64_alu_reg_reg(X86_64_ADD, REG_ITMP1, REG_ITMP2_XPC);    /* 3 bytes */

		if (xcodeptr != NULL) {
			x86_64_jmp_imm((xcodeptr - mcodeptr) - 5);

		} else {
			xcodeptr = mcodeptr;

			x86_64_mov_imm_reg((s8) proto_java_lang_ArrayIndexOutOfBoundsException, REG_ITMP1_XPTR);
			x86_64_mov_imm_reg((s8) asm_handle_exception, REG_ITMP3);
			x86_64_jmp_reg(REG_ITMP3);
		}
	}

	/* generate negative array size check stubs */
	xcodeptr = NULL;
	
	for (; xcheckarefs != NULL; xcheckarefs = xcheckarefs->next) {
		if ((exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch(mcodebase + xcheckarefs->branchpos, 
				xcheckarefs->branchpos, xcodeptr - mcodebase - (10 + 10 + 3));
			continue;
		}

		gen_resolvebranch(mcodebase + xcheckarefs->branchpos, 
		                  xcheckarefs->branchpos, mcodeptr - mcodebase);

		MCODECHECK(8);

		x86_64_mov_imm_reg(0, REG_ITMP2_XPC);    /* 10 bytes */
		dseg_adddata(mcodeptr);
		x86_64_mov_imm_reg(xcheckarefs->branchpos - 6, REG_ITMP1);    /* 10 bytes */
		x86_64_alu_reg_reg(X86_64_ADD, REG_ITMP1, REG_ITMP2_XPC);    /* 3 bytes */

		if (xcodeptr != NULL) {
			x86_64_jmp_imm((xcodeptr - mcodeptr) - 5);

		} else {
			xcodeptr = mcodeptr;

			x86_64_mov_imm_reg((s8) proto_java_lang_NegativeArraySizeException, REG_ITMP1_XPTR);
			x86_64_mov_imm_reg((s8) asm_handle_exception, REG_ITMP3);
			x86_64_jmp_reg(REG_ITMP3);
		}
	}

	/* generate cast check stubs */
	xcodeptr = NULL;
	
	for (; xcastrefs != NULL; xcastrefs = xcastrefs->next) {
		if ((exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch(mcodebase + xcastrefs->branchpos, 
				xcastrefs->branchpos, xcodeptr - mcodebase - (10 + 10 + 3));
			continue;
		}

		gen_resolvebranch(mcodebase + xcastrefs->branchpos, 
		                  xcastrefs->branchpos, mcodeptr - mcodebase);

		MCODECHECK(8);

		x86_64_mov_imm_reg(0, REG_ITMP2_XPC);    /* 10 bytes */
		dseg_adddata(mcodeptr);
		x86_64_mov_imm_reg(xcastrefs->branchpos - 6, REG_ITMP1);    /* 10 bytes */
		x86_64_alu_reg_reg(X86_64_ADD, REG_ITMP1, REG_ITMP2_XPC);    /* 3 bytes */

		if (xcodeptr != NULL) {
			x86_64_jmp_imm((xcodeptr - mcodeptr) - 5);
		
		} else {
			xcodeptr = mcodeptr;

			x86_64_mov_imm_reg((s8) proto_java_lang_ClassCastException, REG_ITMP1_XPTR);
			x86_64_mov_imm_reg((s8) asm_handle_exception, REG_ITMP3);
			x86_64_jmp_reg(REG_ITMP3);
		}
	}

#ifdef SOFTNULLPTRCHECK
	/* generate null pointer check stubs */
	xcodeptr = NULL;
	
	for (; xnullrefs != NULL; xnullrefs = xnullrefs->next) {
		if ((exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch(mcodebase + xnullrefs->branchpos, 
				xnullrefs->branchpos, xcodeptr - mcodebase - (10 + 10 + 3));
			continue;
		}

		gen_resolvebranch(mcodebase + xnullrefs->branchpos, 
		                  xnullrefs->branchpos, mcodeptr - mcodebase);

		MCODECHECK(8);

		x86_64_mov_imm_reg(0, REG_ITMP2_XPC);    /* 10 bytes */
		dseg_adddata(mcodeptr);
		x86_64_mov_imm_reg(xnullrefs->branchpos - 6, REG_ITMP1);    /* 10 bytes */
		x86_64_alu_reg_reg(X86_64_ADD, REG_ITMP1, REG_ITMP2_XPC);    /* 3 bytes */

		if (xcodeptr != NULL) {
			x86_64_jmp_imm((xcodeptr - mcodeptr) - 5);
		
		} else {
			xcodeptr = mcodeptr;

			x86_64_mov_imm_reg((s8) proto_java_lang_NullPointerException, REG_ITMP1_XPTR);
			x86_64_mov_imm_reg((s8) asm_handle_exception, REG_ITMP3);
			x86_64_jmp_reg(REG_ITMP3);
		}
	}

#endif
	}

	codegen_finish((int)((u1*) mcodeptr - mcodebase));
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

#ifdef STATISTICS
	count_cstub_len += COMPSTUBSIZE;
#endif

	return (u1*) s;
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

#define NATIVESTUBSIZE 420

u1 *createnativestub(functionptr f, methodinfo *m)
{
	u1 *s = CNEW(u1, NATIVESTUBSIZE);   /* memory to hold the stub            */
	mcodeptr = s;                       /* make macros work                   */

	reg_init();

	if (runverbose) {
		int p, l, s1;

		x86_64_alu_imm_reg(X86_64_SUB, (6 + 8 + 1) * 8, REG_SP);

		x86_64_mov_reg_membase(argintregs[0], REG_SP, 1 * 8);
		x86_64_mov_reg_membase(argintregs[1], REG_SP, 2 * 8);
		x86_64_mov_reg_membase(argintregs[2], REG_SP, 3 * 8);
		x86_64_mov_reg_membase(argintregs[3], REG_SP, 4 * 8);
		x86_64_mov_reg_membase(argintregs[4], REG_SP, 5 * 8);
		x86_64_mov_reg_membase(argintregs[5], REG_SP, 6 * 8);

		x86_64_movq_reg_membase(argfltregs[0], REG_SP, 7 * 8);
		x86_64_movq_reg_membase(argfltregs[1], REG_SP, 8 * 8);
		x86_64_movq_reg_membase(argfltregs[2], REG_SP, 9 * 8);
		x86_64_movq_reg_membase(argfltregs[3], REG_SP, 10 * 8);
/*  		x86_64_movq_reg_membase(argfltregs[4], REG_SP, 11 * 8); */
/*  		x86_64_movq_reg_membase(argfltregs[5], REG_SP, 12 * 8); */
/*  		x86_64_movq_reg_membase(argfltregs[6], REG_SP, 13 * 8); */
/*  		x86_64_movq_reg_membase(argfltregs[7], REG_SP, 14 * 8); */

		descriptor2types(m);                     /* set paramcount and paramtypes */

		/* also show the hex code for floats passed */
		for (p = 0, l = 0; p < m->paramcount; p++) {
			if (IS_FLT_DBL_TYPE(m->paramtypes[p])) {
				for (s1 = (m->paramcount > intreg_argnum) ? intreg_argnum - 2 : m->paramcount - 2; s1 >= p; s1--) {
					x86_64_mov_reg_reg(argintregs[s1], argintregs[s1 + 1]);
				}

				x86_64_movd_freg_reg(argfltregs[l], argintregs[p]);
				l++;
			}
		}

		x86_64_mov_imm_reg((s8) m, REG_ITMP2);
		x86_64_mov_reg_membase(REG_ITMP2, REG_SP, 0 * 8);
/*  		x86_64_mov_imm_reg(asm_builtin_trace, REG_ITMP1); */
		x86_64_mov_imm_reg((s8) builtin_trace_args, REG_ITMP1);
		x86_64_call_reg(REG_ITMP1);

		x86_64_mov_membase_reg(REG_SP, 1 * 8, argintregs[0]);
		x86_64_mov_membase_reg(REG_SP, 2 * 8, argintregs[1]);
		x86_64_mov_membase_reg(REG_SP, 3 * 8, argintregs[2]);
		x86_64_mov_membase_reg(REG_SP, 4 * 8, argintregs[3]);
		x86_64_mov_membase_reg(REG_SP, 5 * 8, argintregs[4]);
		x86_64_mov_membase_reg(REG_SP, 6 * 8, argintregs[5]);

		x86_64_movq_membase_reg(REG_SP, 7 * 8, argfltregs[0]);
		x86_64_movq_membase_reg(REG_SP, 8 * 8, argfltregs[1]);
		x86_64_movq_membase_reg(REG_SP, 9 * 8, argfltregs[2]);
		x86_64_movq_membase_reg(REG_SP, 10 * 8, argfltregs[3]);
/*  		x86_64_movq_membase_reg(REG_SP, 11 * 8, argfltregs[4]); */
/*  		x86_64_movq_membase_reg(REG_SP, 12 * 8, argfltregs[5]); */
/*  		x86_64_movq_membase_reg(REG_SP, 13 * 8, argfltregs[6]); */
/*  		x86_64_movq_membase_reg(REG_SP, 14 * 8, argfltregs[7]); */

		x86_64_alu_imm_reg(X86_64_ADD, (6 + 8 + 1) * 8, REG_SP);
	}

	x86_64_alu_imm_reg(X86_64_SUB, 7 * 8, REG_SP);    /* keep stack 16-byte aligned */

	/* save callee saved float registers */
	x86_64_movq_reg_membase(XMM15, REG_SP, 0 * 8);
	x86_64_movq_reg_membase(XMM14, REG_SP, 1 * 8);
	x86_64_movq_reg_membase(XMM13, REG_SP, 2 * 8);
	x86_64_movq_reg_membase(XMM12, REG_SP, 3 * 8);
	x86_64_movq_reg_membase(XMM11, REG_SP, 4 * 8);
	x86_64_movq_reg_membase(XMM10, REG_SP, 5 * 8);

	x86_64_mov_reg_reg(argintregs[4], argintregs[5]);
	x86_64_mov_reg_reg(argintregs[3], argintregs[4]);
	x86_64_mov_reg_reg(argintregs[2], argintregs[3]);
	x86_64_mov_reg_reg(argintregs[1], argintregs[2]);
	x86_64_mov_reg_reg(argintregs[0], argintregs[1]);

	x86_64_mov_imm_reg((s8) &env, argintregs[0]);

	x86_64_mov_imm_reg((s8) f, REG_ITMP1);
	x86_64_call_reg(REG_ITMP1);

	/* restore callee saved registers */
	x86_64_movq_membase_reg(REG_SP, 0 * 8, XMM15);
	x86_64_movq_membase_reg(REG_SP, 1 * 8, XMM14);
	x86_64_movq_membase_reg(REG_SP, 2 * 8, XMM13);
	x86_64_movq_membase_reg(REG_SP, 3 * 8, XMM12);
	x86_64_movq_membase_reg(REG_SP, 4 * 8, XMM11);
	x86_64_movq_membase_reg(REG_SP, 5 * 8, XMM10);

	x86_64_alu_imm_reg(X86_64_ADD, 7 * 8, REG_SP);    /* keep stack 16-byte aligned */

	if (runverbose) {
		x86_64_alu_imm_reg(X86_64_SUB, 3 * 8, REG_SP);    /* keep stack 16-byte aligned */

		x86_64_mov_reg_membase(REG_RESULT, REG_SP, 0 * 8);
		x86_64_movq_reg_membase(REG_FRESULT, REG_SP, 1 * 8);

  		x86_64_mov_imm_reg((s8) m, argintregs[0]);
  		x86_64_mov_reg_reg(REG_RESULT, argintregs[1]);
		M_FLTMOVE(REG_FRESULT, argfltregs[0]);
  		M_FLTMOVE(REG_FRESULT, argfltregs[1]);

/*  		x86_64_mov_imm_reg(asm_builtin_exittrace, REG_ITMP1); */
		x86_64_mov_imm_reg((s8) builtin_displaymethodstop, REG_ITMP1);
		x86_64_call_reg(REG_ITMP1);

		x86_64_mov_membase_reg(REG_SP, 0 * 8, REG_RESULT);
		x86_64_movq_membase_reg(REG_SP, 1 * 8, REG_FRESULT);

		x86_64_alu_imm_reg(X86_64_ADD, 3 * 8, REG_SP);    /* keep stack 16-byte aligned */
	}

	x86_64_mov_imm_reg((s8) &exceptionptr, REG_ITMP3);
	x86_64_mov_membase_reg(REG_ITMP3, 0, REG_ITMP3);
	x86_64_test_reg_reg(REG_ITMP3, REG_ITMP3);
	x86_64_jcc(X86_64_CC_NE, 1);

	x86_64_ret();

	x86_64_mov_reg_reg(REG_ITMP3, REG_ITMP1_XPTR);
	x86_64_mov_imm_reg((s8) &exceptionptr, REG_ITMP3);
	x86_64_alu_reg_reg(X86_64_XOR, REG_ITMP2, REG_ITMP2);
	x86_64_mov_reg_membase(REG_ITMP2, REG_ITMP3, 0);    /* clear exception pointer */

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

#ifdef STATISTICS
	count_nstub_len += NATIVESTUBSIZE;
#endif

	return (u1*) s;
}



/* function: removenativestub **************************************************

    removes a previously created native-stub from memory
    
*******************************************************************************/

void removenativestub(u1 *stub)
{
	CFREE(stub, NATIVESTUBSIZE);
}



void x86_64_emit_ialu(s4 alu_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->prev->regoff;
	s4 s2 = src->regoff;
	s4 d = iptr->dst->regoff;

	if (iptr->dst->flags & INMEMORY) {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (s2 == d) {
				x86_64_movl_membase_reg(REG_SP, s1 * 8, REG_ITMP1);
				x86_64_alul_reg_membase(alu_op, REG_ITMP1, REG_SP, d * 8);

			} else if (s1 == d) {
				x86_64_movl_membase_reg(REG_SP, s2 * 8, REG_ITMP1);
				x86_64_alul_reg_membase(alu_op, REG_ITMP1, REG_SP, d * 8);

			} else {
				x86_64_movl_membase_reg(REG_SP, s1 * 8, REG_ITMP1);
				x86_64_alul_membase_reg(alu_op, REG_SP, s2 * 8, REG_ITMP1);
				x86_64_movl_reg_membase(REG_ITMP1, REG_SP, d * 8);
			}

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			if (s2 == d) {
				x86_64_alul_reg_membase(alu_op, s1, REG_SP, d * 8);

			} else {
				x86_64_movl_membase_reg(REG_SP, s2 * 8, REG_ITMP1);
				x86_64_alul_reg_reg(alu_op, s1, REG_ITMP1);
				x86_64_movl_reg_membase(REG_ITMP1, REG_SP, d * 8);
			}

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (s1 == d) {
				x86_64_alul_reg_membase(alu_op, s2, REG_SP, d * 8);
						
			} else {
				x86_64_movl_membase_reg(REG_SP, s1 * 8, REG_ITMP1);
				x86_64_alul_reg_reg(alu_op, s2, REG_ITMP1);
				x86_64_movl_reg_membase(REG_ITMP1, REG_SP, d * 8);
			}

		} else {
			x86_64_movl_reg_membase(s1, REG_SP, d * 8);
			x86_64_alul_reg_membase(alu_op, s2, REG_SP, d * 8);
		}

	} else {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			x86_64_movl_membase_reg(REG_SP, s1 * 8, d);
			x86_64_alul_membase_reg(alu_op, REG_SP, s2 * 8, d);

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			M_INTMOVE(s1, d);
			x86_64_alul_membase_reg(alu_op, REG_SP, s2 * 8, d);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			M_INTMOVE(s2, d);
			x86_64_alul_membase_reg(alu_op, REG_SP, s1 * 8, d);

		} else {
			if (s2 == d) {
				x86_64_alul_reg_reg(alu_op, s1, d);

			} else {
				M_INTMOVE(s1, d);
				x86_64_alul_reg_reg(alu_op, s2, d);
			}
		}
	}
}



void x86_64_emit_lalu(s4 alu_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->prev->regoff;
	s4 s2 = src->regoff;
	s4 d = iptr->dst->regoff;

	if (iptr->dst->flags & INMEMORY) {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (s2 == d) {
				x86_64_mov_membase_reg(REG_SP, s1 * 8, REG_ITMP1);
				x86_64_alu_reg_membase(alu_op, REG_ITMP1, REG_SP, d * 8);

			} else if (s1 == d) {
				x86_64_mov_membase_reg(REG_SP, s2 * 8, REG_ITMP1);
				x86_64_alu_reg_membase(alu_op, REG_ITMP1, REG_SP, d * 8);

			} else {
				x86_64_mov_membase_reg(REG_SP, s1 * 8, REG_ITMP1);
				x86_64_alu_membase_reg(alu_op, REG_SP, s2 * 8, REG_ITMP1);
				x86_64_mov_reg_membase(REG_ITMP1, REG_SP, d * 8);
			}

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			if (s2 == d) {
				x86_64_alu_reg_membase(alu_op, s1, REG_SP, d * 8);

			} else {
				x86_64_mov_membase_reg(REG_SP, s2 * 8, REG_ITMP1);
				x86_64_alu_reg_reg(alu_op, s1, REG_ITMP1);
				x86_64_mov_reg_membase(REG_ITMP1, REG_SP, d * 8);
			}

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (s1 == d) {
				x86_64_alu_reg_membase(alu_op, s2, REG_SP, d * 8);
						
			} else {
				x86_64_mov_membase_reg(REG_SP, s1 * 8, REG_ITMP1);
				x86_64_alu_reg_reg(alu_op, s2, REG_ITMP1);
				x86_64_mov_reg_membase(REG_ITMP1, REG_SP, d * 8);
			}

		} else {
			x86_64_mov_reg_membase(s1, REG_SP, d * 8);
			x86_64_alu_reg_membase(alu_op, s2, REG_SP, d * 8);
		}

	} else {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			x86_64_mov_membase_reg(REG_SP, s1 * 8, d);
			x86_64_alu_membase_reg(alu_op, REG_SP, s2 * 8, d);

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			M_INTMOVE(s1, d);
			x86_64_alu_membase_reg(alu_op, REG_SP, s2 * 8, d);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			M_INTMOVE(s2, d);
			x86_64_alu_membase_reg(alu_op, REG_SP, s1 * 8, d);

		} else {
			if (s2 == d) {
				x86_64_alu_reg_reg(alu_op, s1, d);

			} else {
				M_INTMOVE(s1, d);
				x86_64_alu_reg_reg(alu_op, s2, d);
			}
		}
	}
}



void x86_64_emit_ialuconst(s4 alu_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->regoff;
	s4 d = iptr->dst->regoff;

	if (iptr->dst->flags & INMEMORY) {
		if (src->flags & INMEMORY) {
			if (s1 == d) {
				x86_64_alul_imm_membase(alu_op, iptr->val.i, REG_SP, d * 8);

			} else {
				x86_64_movl_membase_reg(REG_SP, s1 * 8, REG_ITMP1);
				x86_64_alul_imm_reg(alu_op, iptr->val.i, REG_ITMP1);
				x86_64_movl_reg_membase(REG_ITMP1, REG_SP, d * 8);
			}

		} else {
			x86_64_movl_reg_membase(s1, REG_SP, d * 8);
			x86_64_alul_imm_membase(alu_op, iptr->val.i, REG_SP, d * 8);
		}

	} else {
		if (src->flags & INMEMORY) {
			x86_64_movl_membase_reg(REG_SP, s1 * 8, d);
			x86_64_alul_imm_reg(alu_op, iptr->val.i, d);

		} else {
			M_INTMOVE(s1, d);
			x86_64_alul_imm_reg(alu_op, iptr->val.i, d);
		}
	}
}



void x86_64_emit_laluconst(s4 alu_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->regoff;
	s4 d = iptr->dst->regoff;

	if (iptr->dst->flags & INMEMORY) {
		if (src->flags & INMEMORY) {
			if (s1 == d) {
				if (x86_64_is_imm32(iptr->val.l)) {
					x86_64_alu_imm_membase(alu_op, iptr->val.l, REG_SP, d * 8);

				} else {
					x86_64_mov_imm_reg(iptr->val.l, REG_ITMP1);
					x86_64_alu_reg_membase(alu_op, REG_ITMP1, REG_SP, d * 8);
				}

			} else {
				x86_64_mov_membase_reg(REG_SP, s1 * 8, REG_ITMP1);

				if (x86_64_is_imm32(iptr->val.l)) {
					x86_64_alu_imm_reg(alu_op, iptr->val.l, REG_ITMP1);

				} else {
					x86_64_mov_imm_reg(iptr->val.l, REG_ITMP2);
					x86_64_alu_reg_reg(alu_op, REG_ITMP2, REG_ITMP1);
				}
				x86_64_mov_reg_membase(REG_ITMP1, REG_SP, d * 8);
			}

		} else {
			x86_64_mov_reg_membase(s1, REG_SP, d * 8);

			if (x86_64_is_imm32(iptr->val.l)) {
				x86_64_alu_imm_membase(alu_op, iptr->val.l, REG_SP, d * 8);

			} else {
				x86_64_mov_imm_reg(iptr->val.l, REG_ITMP1);
				x86_64_alu_reg_membase(alu_op, REG_ITMP1, REG_SP, d * 8);
			}
		}

	} else {
		if (src->flags & INMEMORY) {
			x86_64_mov_membase_reg(REG_SP, s1 * 8, d);

		} else {
			M_INTMOVE(s1, d);
		}

		if (x86_64_is_imm32(iptr->val.l)) {
			x86_64_alu_imm_reg(alu_op, iptr->val.l, d);

		} else {
			x86_64_mov_imm_reg(iptr->val.l, REG_ITMP1);
			x86_64_alu_reg_reg(alu_op, REG_ITMP1, d);
		}
	}
}



void x86_64_emit_ishift(s4 shift_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->prev->regoff;
	s4 s2 = src->regoff;
	s4 d = iptr->dst->regoff;

	M_INTMOVE(RCX, REG_ITMP1);    /* save RCX */
	if (iptr->dst->flags & INMEMORY) {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (s1 == d) {
				x86_64_movl_membase_reg(REG_SP, s2 * 8, RCX);
				x86_64_shiftl_membase(shift_op, REG_SP, d * 8);

			} else {
				x86_64_movl_membase_reg(REG_SP, s2 * 8, RCX);
				x86_64_movl_membase_reg(REG_SP, s1 * 8, REG_ITMP2);
				x86_64_shiftl_reg(shift_op, REG_ITMP2);
				x86_64_movl_reg_membase(REG_ITMP2, REG_SP, d * 8);
			}

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			x86_64_movl_membase_reg(REG_SP, s2 * 8, RCX);
			x86_64_movl_reg_membase(s1, REG_SP, d * 8);
			x86_64_shiftl_membase(shift_op, REG_SP, d * 8);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (s1 == d) {
				M_INTMOVE(s2, RCX);
				x86_64_shiftl_membase(shift_op, REG_SP, d * 8);

			} else {
				M_INTMOVE(s2, RCX);
				x86_64_movl_membase_reg(REG_SP, s1 * 8, REG_ITMP2);
				x86_64_shiftl_reg(shift_op, REG_ITMP2);
				x86_64_movl_reg_membase(REG_ITMP2, REG_SP, d * 8);
			}

		} else {
			M_INTMOVE(s2, RCX);
			x86_64_movl_reg_membase(s1, REG_SP, d * 8);
			x86_64_shiftl_membase(shift_op, REG_SP, d * 8);
		}
		M_INTMOVE(REG_ITMP1, RCX);    /* restore RCX */

	} else {
		if (d == RCX) {
			d = REG_ITMP3;
		}
					
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			x86_64_movl_membase_reg(REG_SP, s2 * 8, RCX);
			x86_64_movl_membase_reg(REG_SP, s1 * 8, d);
			x86_64_shiftl_reg(shift_op, d);

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			M_INTMOVE(s1, d);    /* maybe src is RCX */
			x86_64_movl_membase_reg(REG_SP, s2 * 8, RCX);
			x86_64_shiftl_reg(shift_op, d);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			M_INTMOVE(s2, RCX);
			x86_64_movl_membase_reg(REG_SP, s1 * 8, d);
			x86_64_shiftl_reg(shift_op, d);

		} else {
			if (s1 == RCX) {
				M_INTMOVE(s1, d);
				M_INTMOVE(s2, RCX);

			} else {
				M_INTMOVE(s2, RCX);
				M_INTMOVE(s1, d);
			}
			x86_64_shiftl_reg(shift_op, d);
		}

		if (d == RCX) {
			M_INTMOVE(REG_ITMP3, RCX);

		} else {
			M_INTMOVE(REG_ITMP1, RCX);    /* restore RCX */
		}
	}
}



void x86_64_emit_lshift(s4 shift_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->prev->regoff;
	s4 s2 = src->regoff;
	s4 d = iptr->dst->regoff;

	M_INTMOVE(RCX, REG_ITMP1);    /* save RCX */
	if (iptr->dst->flags & INMEMORY) {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (s1 == d) {
				x86_64_mov_membase_reg(REG_SP, s2 * 8, RCX);
				x86_64_shift_membase(shift_op, REG_SP, d * 8);

			} else {
				x86_64_mov_membase_reg(REG_SP, s2 * 8, RCX);
				x86_64_mov_membase_reg(REG_SP, s1 * 8, REG_ITMP2);
				x86_64_shift_reg(shift_op, REG_ITMP2);
				x86_64_mov_reg_membase(REG_ITMP2, REG_SP, d * 8);
			}

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			x86_64_mov_membase_reg(REG_SP, s2 * 8, RCX);
			x86_64_mov_reg_membase(s1, REG_SP, d * 8);
			x86_64_shift_membase(shift_op, REG_SP, d * 8);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (s1 == d) {
				M_INTMOVE(s2, RCX);
				x86_64_shift_membase(shift_op, REG_SP, d * 8);

			} else {
				M_INTMOVE(s2, RCX);
				x86_64_mov_membase_reg(REG_SP, s1 * 8, REG_ITMP2);
				x86_64_shift_reg(shift_op, REG_ITMP2);
				x86_64_mov_reg_membase(REG_ITMP2, REG_SP, d * 8);
			}

		} else {
			M_INTMOVE(s2, RCX);
			x86_64_mov_reg_membase(s1, REG_SP, d * 8);
			x86_64_shift_membase(shift_op, REG_SP, d * 8);
		}
		M_INTMOVE(REG_ITMP1, RCX);    /* restore RCX */

	} else {
		if (d == RCX) {
			d = REG_ITMP3;
		}

		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			x86_64_mov_membase_reg(REG_SP, s2 * 8, RCX);
			x86_64_mov_membase_reg(REG_SP, s1 * 8, d);
			x86_64_shift_reg(shift_op, d);

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			M_INTMOVE(s1, d);    /* maybe src is RCX */
			x86_64_mov_membase_reg(REG_SP, s2 * 8, RCX);
			x86_64_shift_reg(shift_op, d);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			M_INTMOVE(s2, RCX);
			x86_64_mov_membase_reg(REG_SP, s1 * 8, d);
			x86_64_shift_reg(shift_op, d);

		} else {
			if (s1 == RCX) {
				M_INTMOVE(s1, d);
				M_INTMOVE(s2, RCX);
			} else {
				M_INTMOVE(s2, RCX);
				M_INTMOVE(s1, d);
			}
			x86_64_shift_reg(shift_op, d);
		}

		if (d == RCX) {
			M_INTMOVE(REG_ITMP3, RCX);

		} else {
			M_INTMOVE(REG_ITMP1, RCX);    /* restore RCX */
		}
	}
}



void x86_64_emit_ishiftconst(s4 shift_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->regoff;
	s4 d = iptr->dst->regoff;

	if ((src->flags & INMEMORY) && (iptr->dst->flags & INMEMORY)) {
		if (s1 == d) {
			x86_64_shiftl_imm_membase(shift_op, iptr->val.i, REG_SP, d * 8);

		} else {
			x86_64_movl_membase_reg(REG_SP, s1 * 8, REG_ITMP1);
			x86_64_shiftl_imm_reg(shift_op, iptr->val.i, REG_ITMP1);
			x86_64_movl_reg_membase(REG_ITMP1, REG_SP, d * 8);
		}

	} else if ((src->flags & INMEMORY) && !(iptr->dst->flags & INMEMORY)) {
		x86_64_movl_membase_reg(REG_SP, s1 * 8, d);
		x86_64_shiftl_imm_reg(shift_op, iptr->val.i, d);
				
	} else if (!(src->flags & INMEMORY) && (iptr->dst->flags & INMEMORY)) {
		x86_64_movl_reg_membase(s1, REG_SP, d * 8);
		x86_64_shiftl_imm_membase(shift_op, iptr->val.i, REG_SP, d * 8);

	} else {
		M_INTMOVE(s1, d);
		x86_64_shiftl_imm_reg(shift_op, iptr->val.i, d);
	}
}



void x86_64_emit_lshiftconst(s4 shift_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->regoff;
	s4 d = iptr->dst->regoff;

	if ((src->flags & INMEMORY) && (iptr->dst->flags & INMEMORY)) {
		if (s1 == d) {
			x86_64_shift_imm_membase(shift_op, iptr->val.i, REG_SP, d * 8);

		} else {
			x86_64_mov_membase_reg(REG_SP, s1 * 8, REG_ITMP1);
			x86_64_shift_imm_reg(shift_op, iptr->val.i, REG_ITMP1);
			x86_64_mov_reg_membase(REG_ITMP1, REG_SP, d * 8);
		}

	} else if ((src->flags & INMEMORY) && !(iptr->dst->flags & INMEMORY)) {
		x86_64_mov_membase_reg(REG_SP, s1 * 8, d);
		x86_64_shift_imm_reg(shift_op, iptr->val.i, d);
				
	} else if (!(src->flags & INMEMORY) && (iptr->dst->flags & INMEMORY)) {
		x86_64_mov_reg_membase(s1, REG_SP, d * 8);
		x86_64_shift_imm_membase(shift_op, iptr->val.i, REG_SP, d * 8);

	} else {
		M_INTMOVE(s1, d);
		x86_64_shift_imm_reg(shift_op, iptr->val.i, d);
	}
}



void x86_64_emit_ifcc(s4 if_op, stackptr src, instruction *iptr)
{
	if (src->flags & INMEMORY) {
		x86_64_alul_imm_membase(X86_64_CMP, iptr->val.i, REG_SP, src->regoff * 8);

	} else {
		x86_64_alul_imm_reg(X86_64_CMP, iptr->val.i, src->regoff);
	}
	x86_64_jcc(if_op, 0);
	codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
}



void x86_64_emit_if_lcc(s4 if_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->regoff;

	if (src->flags & INMEMORY) {
		if (x86_64_is_imm32(iptr->val.l)) {
			x86_64_alu_imm_membase(X86_64_CMP, iptr->val.l, REG_SP, s1 * 8);

		} else {
			x86_64_mov_imm_reg(iptr->val.l, REG_ITMP1);
			x86_64_alu_reg_membase(X86_64_CMP, REG_ITMP1, REG_SP, s1 * 8);
		}

	} else {
		if (x86_64_is_imm32(iptr->val.l)) {
			x86_64_alu_imm_reg(X86_64_CMP, iptr->val.l, s1);

		} else {
			x86_64_mov_imm_reg(iptr->val.l, REG_ITMP1);
			x86_64_alu_reg_reg(X86_64_CMP, REG_ITMP1, s1);
		}
	}
	x86_64_jcc(if_op, 0);
	codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
}



void x86_64_emit_if_icmpcc(s4 if_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->prev->regoff;
	s4 s2 = src->regoff;

	if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
		x86_64_movl_membase_reg(REG_SP, s2 * 8, REG_ITMP1);
		x86_64_alul_reg_membase(X86_64_CMP, REG_ITMP1, REG_SP, s1 * 8);

	} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
		x86_64_alul_membase_reg(X86_64_CMP, REG_SP, s2 * 8, s1);

	} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
		x86_64_alul_reg_membase(X86_64_CMP, s2, REG_SP, s1 * 8);

	} else {
		x86_64_alul_reg_reg(X86_64_CMP, s2, s1);
	}
	x86_64_jcc(if_op, 0);
	codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
}



void x86_64_emit_if_lcmpcc(s4 if_op, stackptr src, instruction *iptr)
{
	s4 s1 = src->prev->regoff;
	s4 s2 = src->regoff;

	if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
		x86_64_mov_membase_reg(REG_SP, s2 * 8, REG_ITMP1);
		x86_64_alu_reg_membase(X86_64_CMP, REG_ITMP1, REG_SP, s1 * 8);

	} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
		x86_64_alu_membase_reg(X86_64_CMP, REG_SP, s2 * 8, s1);

	} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
		x86_64_alu_reg_membase(X86_64_CMP, s2, REG_SP, s1 * 8);

	} else {
		x86_64_alu_reg_reg(X86_64_CMP, s2, s1);
	}
	x86_64_jcc(if_op, 0);
	codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
}



#if 1

/*
 * mov ops
 */
void x86_64_mov_reg_reg(s8 reg, s8 dreg) {
	x86_64_emit_rex(1,(reg),0,(dreg));
	*(mcodeptr++) = 0x89;
	x86_64_emit_reg((reg),(dreg));
}


void x86_64_mov_imm_reg(s8 imm, s8 reg) {
	x86_64_emit_rex(1,0,0,(reg));
	*(mcodeptr++) = 0xb8 + ((reg) & 0x07);
	x86_64_emit_imm64((imm));
}


void x86_64_movl_imm_reg(s8 imm, s8 reg) {
	x86_64_emit_rex(0,0,0,(reg));
	*(mcodeptr++) = 0xb8 + ((reg) & 0x07);
	x86_64_emit_imm32((imm));
}


void x86_64_mov_membase_reg(s8 basereg, s8 disp, s8 reg) {
	x86_64_emit_rex(1,(reg),0,(basereg));
	*(mcodeptr++) = 0x8b;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_movl_membase_reg(s8 basereg, s8 disp, s8 reg) {
	x86_64_emit_rex(0,(reg),0,(basereg));
	*(mcodeptr++) = 0x8b;
	x86_64_emit_membase((basereg),(disp),(reg));
}


/*
 * this one is for INVOKEVIRTUAL/INVOKEINTERFACE to have a
 * constant membase immediate length of 32bit
 */
void x86_64_mov_membase32_reg(s8 basereg, s8 disp, s8 reg) {
	x86_64_emit_rex(1,(reg),0,(basereg));
	*(mcodeptr++) = 0x8b;
	x86_64_address_byte(2, (reg), (basereg));
	x86_64_emit_imm32((disp));
}


void x86_64_mov_reg_membase(s8 reg, s8 basereg, s8 disp) {
	x86_64_emit_rex(1,(reg),0,(basereg));
	*(mcodeptr++) = 0x89;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_movl_reg_membase(s8 reg, s8 basereg, s8 disp) {
	x86_64_emit_rex(0,(reg),0,(basereg));
	*(mcodeptr++) = 0x89;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_mov_memindex_reg(s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg) {
	x86_64_emit_rex(1,(reg),(indexreg),(basereg));
	*(mcodeptr++) = 0x8b;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_movl_memindex_reg(s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg) {
	x86_64_emit_rex(0,(reg),(indexreg),(basereg));
	*(mcodeptr++) = 0x8b;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_mov_reg_memindex(s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale) {
	x86_64_emit_rex(1,(reg),(indexreg),(basereg));
	*(mcodeptr++) = 0x89;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_movl_reg_memindex(s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale) {
	x86_64_emit_rex(0,(reg),(indexreg),(basereg));
	*(mcodeptr++) = 0x89;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_movw_reg_memindex(s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale) {
	*(mcodeptr++) = 0x66;
	x86_64_emit_rex(0,(reg),(indexreg),(basereg));
	*(mcodeptr++) = 0x89;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_movb_reg_memindex(s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale) {
	x86_64_emit_rex(0,(reg),(indexreg),(basereg));
	*(mcodeptr++) = 0x88;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_mov_imm_membase(s8 imm, s8 basereg, s8 disp) {
	x86_64_emit_rex(1,0,0,(basereg));
	*(mcodeptr++) = 0xc7;
	x86_64_emit_membase((basereg),(disp),0);
	x86_64_emit_imm32((imm));
}


void x86_64_movl_imm_membase(s8 imm, s8 basereg, s8 disp) {
	x86_64_emit_rex(0,0,0,(basereg));
	*(mcodeptr++) = 0xc7;
	x86_64_emit_membase((basereg),(disp),0);
	x86_64_emit_imm32((imm));
}


void x86_64_movsbq_reg_reg(s8 reg, s8 dreg) {
	x86_64_emit_rex(1,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0xbe;
	/* XXX: why do reg and dreg have to be exchanged */
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_movsbq_membase_reg(s8 basereg, s8 disp, s8 dreg) {
	x86_64_emit_rex(1,(dreg),0,(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0xbe;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_movswq_reg_reg(s8 reg, s8 dreg) {
	x86_64_emit_rex(1,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0xbf;
	/* XXX: why do reg and dreg have to be exchanged */
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_movswq_membase_reg(s8 basereg, s8 disp, s8 dreg) {
	x86_64_emit_rex(1,(dreg),0,(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0xbf;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_movslq_reg_reg(s8 reg, s8 dreg) {
	x86_64_emit_rex(1,(dreg),0,(reg));
	*(mcodeptr++) = 0x63;
	/* XXX: why do reg and dreg have to be exchanged */
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_movslq_membase_reg(s8 basereg, s8 disp, s8 dreg) {
	x86_64_emit_rex(1,(dreg),0,(basereg));
	*(mcodeptr++) = 0x63;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_movzwq_reg_reg(s8 reg, s8 dreg) {
	x86_64_emit_rex(1,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0xb7;
	/* XXX: why do reg and dreg have to be exchanged */
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_movzwq_membase_reg(s8 basereg, s8 disp, s8 dreg) {
	x86_64_emit_rex(1,(dreg),0,(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0xb7;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_movswq_memindex_reg(s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg) {
	x86_64_emit_rex(1,(reg),(indexreg),(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0xbf;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_movsbq_memindex_reg(s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg) {
	x86_64_emit_rex(1,(reg),(indexreg),(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0xbe;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_movzwq_memindex_reg(s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 reg) {
	x86_64_emit_rex(1,(reg),(indexreg),(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0xb7;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}



/*
 * alu operations
 */
void x86_64_alu_reg_reg(s8 opc, s8 reg, s8 dreg) {
	x86_64_emit_rex(1,(reg),0,(dreg));
	*(mcodeptr++) = (((opc)) << 3) + 1;
	x86_64_emit_reg((reg),(dreg));
}


void x86_64_alul_reg_reg(s8 opc, s8 reg, s8 dreg) {
	x86_64_emit_rex(0,(reg),0,(dreg));
	*(mcodeptr++) = (((opc)) << 3) + 1;
	x86_64_emit_reg((reg),(dreg));
}


void x86_64_alu_reg_membase(s8 opc, s8 reg, s8 basereg, s8 disp) {
	x86_64_emit_rex(1,(reg),0,(basereg));
	*(mcodeptr++) = (((opc)) << 3) + 1;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_alul_reg_membase(s8 opc, s8 reg, s8 basereg, s8 disp) {
	x86_64_emit_rex(0,(reg),0,(basereg));
	*(mcodeptr++) = (((opc)) << 3) + 1;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_alu_membase_reg(s8 opc, s8 basereg, s8 disp, s8 reg) {
	x86_64_emit_rex(1,(reg),0,(basereg));
	*(mcodeptr++) = (((opc)) << 3) + 3;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_alul_membase_reg(s8 opc, s8 basereg, s8 disp, s8 reg) {
	x86_64_emit_rex(0,(reg),0,(basereg));
	*(mcodeptr++) = (((opc)) << 3) + 3;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_alu_imm_reg(s8 opc, s8 imm, s8 dreg) {
	if (x86_64_is_imm8(imm)) {
		x86_64_emit_rex(1,0,0,(dreg));
		*(mcodeptr++) = 0x83;
		x86_64_emit_reg((opc),(dreg));
		x86_64_emit_imm8((imm));
	} else {
		x86_64_emit_rex(1,0,0,(dreg));
		*(mcodeptr++) = 0x81;
		x86_64_emit_reg((opc),(dreg));
		x86_64_emit_imm32((imm));
	}
}


void x86_64_alul_imm_reg(s8 opc, s8 imm, s8 dreg) {
	if (x86_64_is_imm8(imm)) {
		x86_64_emit_rex(0,0,0,(dreg));
		*(mcodeptr++) = 0x83;
		x86_64_emit_reg((opc),(dreg));
		x86_64_emit_imm8((imm));
	} else {
		x86_64_emit_rex(0,0,0,(dreg));
		*(mcodeptr++) = 0x81;
		x86_64_emit_reg((opc),(dreg));
		x86_64_emit_imm32((imm));
	}
}


void x86_64_alu_imm_membase(s8 opc, s8 imm, s8 basereg, s8 disp) {
	if (x86_64_is_imm8(imm)) {
		x86_64_emit_rex(1,(basereg),0,0);
		*(mcodeptr++) = 0x83;
		x86_64_emit_membase((basereg),(disp),(opc));
		x86_64_emit_imm8((imm));
	} else {
		x86_64_emit_rex(1,(basereg),0,0);
		*(mcodeptr++) = 0x81;
		x86_64_emit_membase((basereg),(disp),(opc));
		x86_64_emit_imm32((imm));
	}
}


void x86_64_alul_imm_membase(s8 opc, s8 imm, s8 basereg, s8 disp) {
	if (x86_64_is_imm8(imm)) {
		x86_64_emit_rex(0,(basereg),0,0);
		*(mcodeptr++) = 0x83;
		x86_64_emit_membase((basereg),(disp),(opc));
		x86_64_emit_imm8((imm));
	} else {
		x86_64_emit_rex(0,(basereg),0,0);
		*(mcodeptr++) = 0x81;
		x86_64_emit_membase((basereg),(disp),(opc));
		x86_64_emit_imm32((imm));
	}
}


void x86_64_test_reg_reg(s8 reg, s8 dreg) {
	x86_64_emit_rex(1,(reg),0,(dreg));
	*(mcodeptr++) = 0x85;
	x86_64_emit_reg((reg),(dreg));
}


void x86_64_testl_reg_reg(s8 reg, s8 dreg) {
	x86_64_emit_rex(0,(reg),0,(dreg));
	*(mcodeptr++) = 0x85;
	x86_64_emit_reg((reg),(dreg));
}


void x86_64_test_imm_reg(s8 imm, s8 reg) {
	*(mcodeptr++) = 0xf7;
	x86_64_emit_reg(0,(reg));
	x86_64_emit_imm32((imm));
}


void x86_64_testw_imm_reg(s8 imm, s8 reg) {
	*(mcodeptr++) = 0x66;
	*(mcodeptr++) = 0xf7;
	x86_64_emit_reg(0,(reg));
	x86_64_emit_imm16((imm));
}


void x86_64_testb_imm_reg(s8 imm, s8 reg) {
	*(mcodeptr++) = 0xf6;
	x86_64_emit_reg(0,(reg));
	x86_64_emit_imm8((imm));
}


void x86_64_lea_membase_reg(s8 basereg, s8 disp, s8 reg) {
	x86_64_emit_rex(1,(reg),0,(basereg));
	*(mcodeptr++) = 0x8d;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_leal_membase_reg(s8 basereg, s8 disp, s8 reg) {
	x86_64_emit_rex(0,(reg),0,(basereg));
	*(mcodeptr++) = 0x8d;
	x86_64_emit_membase((basereg),(disp),(reg));
}



/*
 * inc, dec operations
 */
void x86_64_inc_reg(s8 reg) {
	x86_64_emit_rex(1,0,0,(reg));
	*(mcodeptr++) = 0xff;
	x86_64_emit_reg(0,(reg));
}


void x86_64_incl_reg(s8 reg) {
	x86_64_emit_rex(0,0,0,(reg));
	*(mcodeptr++) = 0xff;
	x86_64_emit_reg(0,(reg));
}


void x86_64_inc_membase(s8 basereg, s8 disp) {
	x86_64_emit_rex(1,(basereg),0,0);
	*(mcodeptr++) = 0xff;
	x86_64_emit_membase((basereg),(disp),0);
}


void x86_64_incl_membase(s8 basereg, s8 disp) {
	x86_64_emit_rex(0,(basereg),0,0);
	*(mcodeptr++) = 0xff;
	x86_64_emit_membase((basereg),(disp),0);
}


void x86_64_dec_reg(s8 reg) {
	x86_64_emit_rex(1,0,0,(reg));
	*(mcodeptr++) = 0xff;
	x86_64_emit_reg(1,(reg));
}

        
void x86_64_decl_reg(s8 reg) {
	x86_64_emit_rex(0,0,0,(reg));
	*(mcodeptr++) = 0xff;
	x86_64_emit_reg(1,(reg));
}

        
void x86_64_dec_membase(s8 basereg, s8 disp) {
	x86_64_emit_rex(1,(basereg),0,0);
	*(mcodeptr++) = 0xff;
	x86_64_emit_membase((basereg),(disp),1);
}


void x86_64_decl_membase(s8 basereg, s8 disp) {
	x86_64_emit_rex(0,(basereg),0,0);
	*(mcodeptr++) = 0xff;
	x86_64_emit_membase((basereg),(disp),1);
}




void x86_64_cltd() {
    *(mcodeptr++) = 0x99;
}


void x86_64_cqto() {
	x86_64_emit_rex(1,0,0,0);
	*(mcodeptr++) = 0x99;
}



void x86_64_imul_reg_reg(s8 reg, s8 dreg) {
	x86_64_emit_rex(1,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0xaf;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_imull_reg_reg(s8 reg, s8 dreg) {
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0xaf;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_imul_membase_reg(s8 basereg, s8 disp, s8 dreg) {
	x86_64_emit_rex(1,(dreg),0,(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0xaf;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_imull_membase_reg(s8 basereg, s8 disp, s8 dreg) {
	x86_64_emit_rex(0,(dreg),0,(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0xaf;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_imul_imm_reg(s8 imm, s8 dreg) {
	if (x86_64_is_imm8((imm))) {
		x86_64_emit_rex(1,0,0,(dreg));
		*(mcodeptr++) = 0x6b;
		x86_64_emit_reg(0,(dreg));
		x86_64_emit_imm8((imm));
	} else {
		x86_64_emit_rex(1,0,0,(dreg));
		*(mcodeptr++) = 0x69;
		x86_64_emit_reg(0,(dreg));
		x86_64_emit_imm32((imm));
	}
}


void x86_64_imul_imm_reg_reg(s8 imm, s8 reg, s8 dreg) {
	if (x86_64_is_imm8((imm))) {
		x86_64_emit_rex(1,(dreg),0,(reg));
		*(mcodeptr++) = 0x6b;
		x86_64_emit_reg((dreg),(reg));
		x86_64_emit_imm8((imm));
	} else {
		x86_64_emit_rex(1,(dreg),0,(reg));
		*(mcodeptr++) = 0x69;
		x86_64_emit_reg((dreg),(reg));
		x86_64_emit_imm32((imm));
	}
}


void x86_64_imull_imm_reg_reg(s8 imm, s8 reg, s8 dreg) {
	if (x86_64_is_imm8((imm))) {
		x86_64_emit_rex(0,(dreg),0,(reg));
		*(mcodeptr++) = 0x6b;
		x86_64_emit_reg((dreg),(reg));
		x86_64_emit_imm8((imm));
	} else {
		x86_64_emit_rex(0,(dreg),0,(reg));
		*(mcodeptr++) = 0x69;
		x86_64_emit_reg((dreg),(reg));
		x86_64_emit_imm32((imm));
	}
}


void x86_64_imul_imm_membase_reg(s8 imm, s8 basereg, s8 disp, s8 dreg) {
	if (x86_64_is_imm8((imm))) {
		x86_64_emit_rex(1,(dreg),0,(basereg));
		*(mcodeptr++) = 0x6b;
		x86_64_emit_membase((basereg),(disp),(dreg));
		x86_64_emit_imm8((imm));
	} else {
		x86_64_emit_rex(1,(dreg),0,(basereg));
		*(mcodeptr++) = 0x69;
		x86_64_emit_membase((basereg),(disp),(dreg));
		x86_64_emit_imm32((imm));
	}
}


void x86_64_imull_imm_membase_reg(s8 imm, s8 basereg, s8 disp, s8 dreg) {
	if (x86_64_is_imm8((imm))) {
		x86_64_emit_rex(0,(dreg),0,(basereg));
		*(mcodeptr++) = 0x6b;
		x86_64_emit_membase((basereg),(disp),(dreg));
		x86_64_emit_imm8((imm));
	} else {
		x86_64_emit_rex(0,(dreg),0,(basereg));
		*(mcodeptr++) = 0x69;
		x86_64_emit_membase((basereg),(disp),(dreg));
		x86_64_emit_imm32((imm));
	}
}


void x86_64_idiv_reg(s8 reg) {
	x86_64_emit_rex(1,0,0,(reg));
	*(mcodeptr++) = 0xf7;
	x86_64_emit_reg(7,(reg));
}


void x86_64_idivl_reg(s8 reg) {
	x86_64_emit_rex(0,0,0,(reg));
	*(mcodeptr++) = 0xf7;
	x86_64_emit_reg(7,(reg));
}



void x86_64_ret() {
    *(mcodeptr++) = 0xc3;
}



/*
 * shift ops
 */
void x86_64_shift_reg(s8 opc, s8 reg) {
	x86_64_emit_rex(1,0,0,(reg));
	*(mcodeptr++) = 0xd3;
	x86_64_emit_reg((opc),(reg));
}


void x86_64_shiftl_reg(s8 opc, s8 reg) {
	x86_64_emit_rex(0,0,0,(reg));
	*(mcodeptr++) = 0xd3;
	x86_64_emit_reg((opc),(reg));
}


void x86_64_shift_membase(s8 opc, s8 basereg, s8 disp) {
	x86_64_emit_rex(1,0,0,(basereg));
	*(mcodeptr++) = 0xd3;
	x86_64_emit_membase((basereg),(disp),(opc));
}


void x86_64_shiftl_membase(s8 opc, s8 basereg, s8 disp) {
	x86_64_emit_rex(0,0,0,(basereg));
	*(mcodeptr++) = 0xd3;
	x86_64_emit_membase((basereg),(disp),(opc));
}


void x86_64_shift_imm_reg(s8 opc, s8 imm, s8 dreg) {
	if ((imm) == 1) {
		x86_64_emit_rex(1,0,0,(dreg));
		*(mcodeptr++) = 0xd1;
		x86_64_emit_reg((opc),(dreg));
	} else {
		x86_64_emit_rex(1,0,0,(dreg));
		*(mcodeptr++) = 0xc1;
		x86_64_emit_reg((opc),(dreg));
		x86_64_emit_imm8((imm));
	}
}


void x86_64_shiftl_imm_reg(s8 opc, s8 imm, s8 dreg) {
	if ((imm) == 1) {
		x86_64_emit_rex(0,0,0,(dreg));
		*(mcodeptr++) = 0xd1;
		x86_64_emit_reg((opc),(dreg));
	} else {
		x86_64_emit_rex(0,0,0,(dreg));
		*(mcodeptr++) = 0xc1;
		x86_64_emit_reg((opc),(dreg));
		x86_64_emit_imm8((imm));
	}
}


void x86_64_shift_imm_membase(s8 opc, s8 imm, s8 basereg, s8 disp) {
	if ((imm) == 1) {
		x86_64_emit_rex(1,0,0,(basereg));
		*(mcodeptr++) = 0xd1;
		x86_64_emit_membase((basereg),(disp),(opc));
	} else {
		x86_64_emit_rex(1,0,0,(basereg));
		*(mcodeptr++) = 0xc1;
		x86_64_emit_membase((basereg),(disp),(opc));
		x86_64_emit_imm8((imm));
	}
}


void x86_64_shiftl_imm_membase(s8 opc, s8 imm, s8 basereg, s8 disp) {
	if ((imm) == 1) {
		x86_64_emit_rex(0,0,0,(basereg));
		*(mcodeptr++) = 0xd1;
		x86_64_emit_membase((basereg),(disp),(opc));
	} else {
		x86_64_emit_rex(0,0,0,(basereg));
		*(mcodeptr++) = 0xc1;
		x86_64_emit_membase((basereg),(disp),(opc));
		x86_64_emit_imm8((imm));
	}
}



/*
 * jump operations
 */
void x86_64_jmp_imm(s8 imm) {
	*(mcodeptr++) = 0xe9;
	x86_64_emit_imm32((imm));
}


void x86_64_jmp_reg(s8 reg) {
	x86_64_emit_rex(0,0,0,(reg));
	*(mcodeptr++) = 0xff;
	x86_64_emit_reg(4,(reg));
}


void x86_64_jcc(s8 opc, s8 imm) {
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = (0x80 + x86_64_cc_map[(opc)]);
	x86_64_emit_imm32((imm));
}



/*
 * conditional set and move operations
 */

/* we need the rex byte to get all low bytes */
void x86_64_setcc_reg(s8 opc, s8 reg) {
	*(mcodeptr++) = (0x40 | (((reg) >> 3) & 0x01));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = (0x90 + x86_64_cc_map[(opc)]);
	x86_64_emit_reg(0,(reg));
}


/* we need the rex byte to get all low bytes */
void x86_64_setcc_membase(s8 opc, s8 basereg, s8 disp) {
	*(mcodeptr++) = (0x40 | (((basereg) >> 3) & 0x01));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = (0x90 + x86_64_cc_map[(opc)]);
	x86_64_emit_membase((basereg),(disp),0);
}


void x86_64_cmovcc_reg_reg(s8 opc, s8 reg, s8 dreg) {
	x86_64_emit_rex(1,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = (0x40 + x86_64_cc_map[(opc)]);
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_cmovccl_reg_reg(s8 opc, s8 reg, s8 dreg) {
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = (0x40 + x86_64_cc_map[(opc)]);
	x86_64_emit_reg((dreg),(reg));
}



void x86_64_neg_reg(s8 reg) {
	x86_64_emit_rex(1,0,0,(reg));
	*(mcodeptr++) = 0xf7;
	x86_64_emit_reg(3,(reg));
}


void x86_64_negl_reg(s8 reg) {
	x86_64_emit_rex(0,0,0,(reg));
	*(mcodeptr++) = 0xf7;
	x86_64_emit_reg(3,(reg));
}


void x86_64_neg_membase(s8 basereg, s8 disp) {
	x86_64_emit_rex(1,0,0,(basereg));
	*(mcodeptr++) = 0xf7;
	x86_64_emit_membase((basereg),(disp),3);
}


void x86_64_negl_membase(s8 basereg, s8 disp) {
	x86_64_emit_rex(0,0,0,(basereg));
	*(mcodeptr++) = 0xf7;
	x86_64_emit_membase((basereg),(disp),3);
}



void x86_64_push_imm(s8 imm) {
	*(mcodeptr++) = 0x68;
	x86_64_emit_imm32((imm));
}


void x86_64_pop_reg(s8 reg) {
	x86_64_emit_rex(0,0,0,(reg));
	*(mcodeptr++) = 0x58 + (0x07 & (reg));
}


void x86_64_xchg_reg_reg(s8 reg, s8 dreg) {
	x86_64_emit_rex(1,(reg),0,(dreg));
	*(mcodeptr++) = 0x87;
	x86_64_emit_reg((reg),(dreg));
}


void x86_64_nop() {
    *(mcodeptr++) = 0x90;
}



/*
 * call instructions
 */
void x86_64_call_reg(s8 reg) {
	x86_64_emit_rex(1,0,0,(reg));
	*(mcodeptr++) = 0xff;
	x86_64_emit_reg(2,(reg));
}


void x86_64_call_imm(s8 imm) {
	*(mcodeptr++) = 0xe8;
	x86_64_emit_imm32((imm));
}



/*
 * floating point instructions (SSE2)
 */
void x86_64_addsd_reg_reg(s8 reg, s8 dreg) {
	*(mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x58;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_addss_reg_reg(s8 reg, s8 dreg) {
	*(mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x58;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_cvtsi2ssq_reg_reg(s8 reg, s8 dreg) {
	*(mcodeptr++) = 0xf3;
	x86_64_emit_rex(1,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x2a;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_cvtsi2ss_reg_reg(s8 reg, s8 dreg) {
	*(mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x2a;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_cvtsi2sdq_reg_reg(s8 reg, s8 dreg) {
	*(mcodeptr++) = 0xf2;
	x86_64_emit_rex(1,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x2a;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_cvtsi2sd_reg_reg(s8 reg, s8 dreg) {
	*(mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x2a;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_cvtss2sd_reg_reg(s8 reg, s8 dreg) {
	*(mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x5a;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_cvtsd2ss_reg_reg(s8 reg, s8 dreg) {
	*(mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x5a;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_cvttss2siq_reg_reg(s8 reg, s8 dreg) {
	*(mcodeptr++) = 0xf3;
	x86_64_emit_rex(1,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x2c;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_cvttss2si_reg_reg(s8 reg, s8 dreg) {
	*(mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x2c;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_cvttsd2siq_reg_reg(s8 reg, s8 dreg) {
	*(mcodeptr++) = 0xf2;
	x86_64_emit_rex(1,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x2c;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_cvttsd2si_reg_reg(s8 reg, s8 dreg) {
	*(mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x2c;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_divss_reg_reg(s8 reg, s8 dreg) {
	*(mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x5e;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_divsd_reg_reg(s8 reg, s8 dreg) {
	*(mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x5e;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_movd_reg_freg(s8 reg, s8 freg) {
	*(mcodeptr++) = 0x66;
	x86_64_emit_rex(1,(freg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x6e;
	x86_64_emit_reg((freg),(reg));
}


void x86_64_movd_freg_reg(s8 freg, s8 reg) {
	*(mcodeptr++) = 0x66;
	x86_64_emit_rex(1,(freg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x7e;
	x86_64_emit_reg((freg),(reg));
}


void x86_64_movd_reg_membase(s8 reg, s8 basereg, s8 disp) {
	*(mcodeptr++) = 0x66;
	x86_64_emit_rex(0,(reg),0,(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x7e;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_movd_reg_memindex(s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale) {
	*(mcodeptr++) = 0x66;
	x86_64_emit_rex(0,(reg),(indexreg),(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x7e;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_movd_membase_reg(s8 basereg, s8 disp, s8 dreg) {
	*(mcodeptr++) = 0x66;
	x86_64_emit_rex(1,(dreg),0,(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x6e;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_movdl_membase_reg(s8 basereg, s8 disp, s8 dreg) {
	*(mcodeptr++) = 0x66;
	x86_64_emit_rex(0,(dreg),0,(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x6e;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_movd_memindex_reg(s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 dreg) {
	*(mcodeptr++) = 0x66;
	x86_64_emit_rex(0,(dreg),(indexreg),(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x6e;
	x86_64_emit_memindex((dreg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_movq_reg_reg(s8 reg, s8 dreg) {
	*(mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x7e;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_movq_reg_membase(s8 reg, s8 basereg, s8 disp) {
	*(mcodeptr++) = 0x66;
	x86_64_emit_rex(0,(reg),0,(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0xd6;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_movq_membase_reg(s8 basereg, s8 disp, s8 dreg) {
	*(mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(dreg),0,(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x7e;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_movss_reg_reg(s8 reg, s8 dreg) {
	*(mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(reg),0,(dreg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x10;
	x86_64_emit_reg((reg),(dreg));
}


void x86_64_movsd_reg_reg(s8 reg, s8 dreg) {
	*(mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(reg),0,(dreg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x10;
	x86_64_emit_reg((reg),(dreg));
}


void x86_64_movss_reg_membase(s8 reg, s8 basereg, s8 disp) {
	*(mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(reg),0,(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x11;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_movsd_reg_membase(s8 reg, s8 basereg, s8 disp) {
	*(mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(reg),0,(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x11;
	x86_64_emit_membase((basereg),(disp),(reg));
}


void x86_64_movss_membase_reg(s8 basereg, s8 disp, s8 dreg) {
	*(mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(dreg),0,(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x10;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_movlps_membase_reg(s8 basereg, s8 disp, s8 dreg) {
	x86_64_emit_rex(0,(dreg),0,(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x12;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_movsd_membase_reg(s8 basereg, s8 disp, s8 dreg) {
	*(mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(dreg),0,(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x10;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_movlpd_membase_reg(s8 basereg, s8 disp, s8 dreg) {
	*(mcodeptr++) = 0x66;
	x86_64_emit_rex(0,(dreg),0,(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x12;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_movss_reg_memindex(s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale) {
	*(mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(reg),(indexreg),(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x11;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_movsd_reg_memindex(s8 reg, s8 disp, s8 basereg, s8 indexreg, s8 scale) {
	*(mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(reg),(indexreg),(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x11;
	x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_movss_memindex_reg(s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 dreg) {
	*(mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(dreg),(indexreg),(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x10;
	x86_64_emit_memindex((dreg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_movsd_memindex_reg(s8 disp, s8 basereg, s8 indexreg, s8 scale, s8 dreg) {
	*(mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(dreg),(indexreg),(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x10;
	x86_64_emit_memindex((dreg),(disp),(basereg),(indexreg),(scale));
}


void x86_64_mulss_reg_reg(s8 reg, s8 dreg) {
	*(mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x59;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_mulsd_reg_reg(s8 reg, s8 dreg) {
	*(mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x59;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_subss_reg_reg(s8 reg, s8 dreg) {
	*(mcodeptr++) = 0xf3;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x5c;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_subsd_reg_reg(s8 reg, s8 dreg) {
	*(mcodeptr++) = 0xf2;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x5c;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_ucomiss_reg_reg(s8 reg, s8 dreg) {
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x2e;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_ucomisd_reg_reg(s8 reg, s8 dreg) {
	*(mcodeptr++) = 0x66;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x2e;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_xorps_reg_reg(s8 reg, s8 dreg) {
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x57;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_xorps_membase_reg(s8 basereg, s8 disp, s8 dreg) {
	x86_64_emit_rex(0,(dreg),0,(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x57;
	x86_64_emit_membase((basereg),(disp),(dreg));
}


void x86_64_xorpd_reg_reg(s8 reg, s8 dreg) {
	*(mcodeptr++) = 0x66;
	x86_64_emit_rex(0,(dreg),0,(reg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x57;
	x86_64_emit_reg((dreg),(reg));
}


void x86_64_xorpd_membase_reg(s8 basereg, s8 disp, s8 dreg) {
	*(mcodeptr++) = 0x66;
	x86_64_emit_rex(0,(dreg),0,(basereg));
	*(mcodeptr++) = 0x0f;
	*(mcodeptr++) = 0x57;
	x86_64_emit_membase((basereg),(disp),(dreg));
}

#endif

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
