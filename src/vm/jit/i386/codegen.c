/* jit/i386/codegen.c - machine code generator for i386

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

   $Id: codegen.c 1082 2004-05-26 15:04:54Z jowenn $

*/

#include "global.h"
#include <stdio.h>
#include <signal.h>
#include "types.h"
#include "main.h"
#include "parse.h"
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
#include "offsets.h"

/* include independent code generation stuff */
#include "codegen.inc"
#include "reg.inc"


/* register descripton - array ************************************************/

/* #define REG_RES   0         reserved register for OS or code generator     */
/* #define REG_RET   1         return value register                          */
/* #define REG_EXC   2         exception value register (only old jit)        */
/* #define REG_SAV   3         (callee) saved register                        */
/* #define REG_TMP   4         scratch temporary register (caller saved)      */
/* #define REG_ARG   5         argument register (caller saved)               */

/* #define REG_END   -1        last entry in tables */

/* 
   we initially try to use %edx as scratch register, it cannot be used if we
   have one of these ICMDs:
   LMUL, LMULCONST, IDIV, IREM, LALOAD, AASTORE, LASTORE, IASTORE, CASTORE,
   SASTORE, BASTORE, INSTANCEOF, CHECKCAST, I2L, F2L, D2L
*/
int nregdescint[] = {
    REG_RET, REG_TMP, REG_TMP, REG_TMP, REG_RES, REG_SAV, REG_SAV, REG_SAV,
    REG_END
};


int nregdescfloat[] = {
  /* rounding problems with callee saved registers */
/*      REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_TMP, REG_TMP, REG_RES, REG_RES, */
/*      REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_RES, REG_RES, */
    REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES,
    REG_END
};


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
    else if ((reg) == EBP) (var) += 1;


#define CALCIMMEDIATEBYTES(var, val) \
    if ((s4) (val) < -128 || (s4) (val) > 127) (var) += 4; \
    else (var) += 1;


/* gen_nullptr_check(objreg) */

#define gen_nullptr_check(objreg) \
	if (checknull) { \
        i386_test_reg_reg((objreg), (objreg)); \
        i386_jcc(I386_CC_E, 0); \
 	    codegen_addxnullrefs(mcodeptr); \
	}


/* MCODECHECK(icnt) */

#define MCODECHECK(icnt) \
	if ((mcodeptr + (icnt)) > (u1*) mcodeend) mcodeptr = (u1*) codegen_increase((u1*) mcodeptr)

/* M_INTMOVE:
     generates an integer-move from register a to b.
     if a and b are the same int-register, no code will be generated.
*/ 

#define M_INTMOVE(reg,dreg) if ((reg) != (dreg)) { i386_mov_reg_reg((reg),(dreg)); }


/* M_FLTMOVE:
    generates a floating-point-move from register a to b.
    if a and b are the same float-register, no code will be generated
*/

#define M_FLTMOVE(reg,dreg) panic("M_FLTMOVE");

#define M_LNGMEMMOVE(reg,dreg) \
    do { \
        i386_mov_membase_reg(REG_SP, (reg) * 8, REG_ITMP1); \
        i386_mov_reg_membase(REG_ITMP1, REG_SP, (dreg) * 8); \
        i386_mov_membase_reg(REG_SP, (reg) * 8 + 4, REG_ITMP1); \
        i386_mov_reg_membase(REG_ITMP1, REG_SP, (dreg) * 8 + 4); \
    } while (0)


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
        i386_mov_membase_reg(REG_SP, (v)->regoff * 8, tempnr); \
        regnr = tempnr; \
    } else { \
        regnr = (v)->regoff; \
    }



#define var_to_reg_flt(regnr,v,tempnr) \
    if ((v)->type == TYPE_FLT) { \
        if ((v)->flags & INMEMORY) { \
            COUNT_SPILLS; \
            i386_flds_membase(REG_SP, (v)->regoff * 8); \
            fpu_st_offset++; \
            regnr = tempnr; \
        } else { \
            i386_fld_reg((v)->regoff + fpu_st_offset); \
            fpu_st_offset++; \
            regnr = (v)->regoff; \
        } \
    } else { \
        if ((v)->flags & INMEMORY) { \
            COUNT_SPILLS; \
            i386_fldl_membase(REG_SP, (v)->regoff * 8); \
            fpu_st_offset++; \
            regnr = tempnr; \
        } else { \
            i386_fld_reg((v)->regoff + fpu_st_offset); \
            fpu_st_offset++; \
            regnr = (v)->regoff; \
        } \
    }

#define NEW_var_to_reg_flt(regnr,v,tempnr) \
    if ((v)->type == TYPE_FLT) { \
       if ((v)->flags & INMEMORY) { \
            COUNT_SPILLS; \
            i386_flds_membase(REG_SP, (v)->regoff * 8); \
            fpu_st_offset++; \
            regnr = tempnr; \
        } else { \
            regnr = (v)->regoff; \
        } \
    } else { \
        if ((v)->flags & INMEMORY) { \
            COUNT_SPILLS; \
            i386_fldl_membase(REG_SP, (v)->regoff * 8); \
            fpu_st_offset++; \
            regnr = tempnr; \
        } else { \
            regnr = (v)->regoff; \
        } \
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
		}
		else
			if (v->varnum < intreg_argnum) {
				v->regoff = argintregs[v->varnum];
				return(argintregs[v->varnum]);
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
        i386_mov_reg_membase(tempregnum, REG_SP, (sptr)->regoff * 8); \
    }


#define store_reg_to_var_flt(sptr, tempregnum) \
    if ((sptr)->type == TYPE_FLT) { \
        if ((sptr)->flags & INMEMORY) { \
             COUNT_SPILLS; \
             i386_fstps_membase(REG_SP, (sptr)->regoff * 8); \
             fpu_st_offset--; \
        } else { \
/*                  i386_fxch_reg((sptr)->regoff);*/ \
             i386_fstp_reg((sptr)->regoff + fpu_st_offset); \
             fpu_st_offset--; \
        } \
    } else { \
        if ((sptr)->flags & INMEMORY) { \
            COUNT_SPILLS; \
            i386_fstpl_membase(REG_SP, (sptr)->regoff * 8); \
            fpu_st_offset--; \
        } else { \
/*                  i386_fxch_reg((sptr)->regoff);*/ \
            i386_fstp_reg((sptr)->regoff + fpu_st_offset); \
            fpu_st_offset--; \
        } \
    }


void codegen_stubcalled() {
	log_text("Stub has been called");
}


#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void thread_restartcriticalsection(ucontext_t *uc)
{
	void *critical;
	if ((critical = thread_checkcritical((void*) uc->uc_mcontext.gregs[REG_EIP])) != NULL)
		uc->uc_mcontext.gregs[REG_EIP] = (u4) critical;
}
#endif


#define PREPARE_NATIVE_STACKINFO \
				i386_push_reg(REG_ITMP1);	/*save itmp1, needed by some stubs */ \
                                i386_alu_imm_reg(I386_SUB, 2*4, REG_SP); /* build stack frame (2 * 4 bytes), together with previous =3*4 */ \
                                i386_mov_imm_reg((s4) codegen_stubcalled,REG_ITMP1); \
                                i386_call_reg(REG_ITMP1);                /*call    codegen_stubcalled*/ \
                                i386_mov_imm_reg((s4) builtin_asm_get_stackframeinfo,REG_ITMP1); \
                                i386_call_reg(REG_ITMP1);                /*call    builtin_asm_get_stackframeinfo*/ \
                                i386_mov_reg_membase(REG_RESULT,REG_SP,1*4); /* save thread pointer  to native call stack*/ \
                                i386_mov_membase_reg(REG_RESULT,0,REG_ITMP2); /* get old value of thread specific native call stack */ \
                                i386_mov_reg_membase(REG_ITMP2,REG_SP,0*4);     /* store value on stack */ \
                                i386_mov_reg_membase(REG_SP,REG_RESULT,0); /* store pointer to new stack frame information */ \
				i386_mov_membase_reg(REG_SP,2*4,REG_ITMP1); /* restore ITMP1, need for some stubs*/ \
                                i386_mov_imm_membase(0,REG_SP, 2*4);    /* builtin */ 


#define REMOVE_NATIVE_STACKINFO \
                        i386_mov_membase_reg(REG_SP,0,REG_ITMP2); \
                        i386_mov_membase_reg(REG_SP,4,REG_ITMP3); \
                        i386_mov_reg_membase(REG_ITMP2,REG_ITMP3,0); \
                        i386_alu_imm_reg(I386_ADD,3*4,REG_SP);



/* NullPointerException signal handler for hardware null pointer check */

void catch_NullPointerException(int sig)
{
	sigset_t nsig;
/*  	long     faultaddr; */

	void **_p = (void **) &sig;
	struct sigcontext *sigctx = (struct sigcontext *) ++_p;

	/* Reset signal handler - necessary for SysV, does no harm for BSD */

/*  	instr = *((int*)(sigctx->eip)); */
/*    	faultaddr = sigctx->sc_regs[(instr >> 16) & 0x1f]; */

/*  	fprintf(stderr, "null=%d %p addr=%p\n", sig, sigctx, sigctx->eip);*/

/*  	if (faultaddr == 0) { */
		signal(sig, (void *) catch_NullPointerException);    /* reinstall handler */
		sigemptyset(&nsig);
		sigaddset(&nsig, sig);
		sigprocmask(SIG_UNBLOCK, &nsig, NULL);               /* unblock signal    */

		sigctx->ecx = sigctx->eip;    /* REG_ITMP2_XPC     */
		sigctx->eip = (s4) asm_throw_and_handle_hardware_nullpointer_exception;
		

		return;

/*  	} else { */
/*  		faultaddr += (long) ((instr << 16) >> 16); */
/*  		fprintf(stderr, "faulting address: 0x%08x\n", faultaddr); */
/*  		panic("Stack overflow"); */
/*  	} */
}


/* ArithmeticException signal handler for hardware divide by zero check       */

void catch_ArithmeticException(int sig)
{
	sigset_t nsig;

	void **_p = (void **) &sig;
	struct sigcontext *sigctx = (struct sigcontext *) ++_p;

	/* Reset signal handler - necessary for SysV, does no harm for BSD        */

	signal(sig, (void *) catch_ArithmeticException);     /* reinstall handler */
	sigemptyset(&nsig);
	sigaddset(&nsig, sig);
	sigprocmask(SIG_UNBLOCK, &nsig, NULL);               /* unblock signal    */

	sigctx->ecx = sigctx->eip;                           /* REG_ITMP2_XPC     */
	sigctx->eip = (s4) asm_throw_and_handle_hardware_arithmetic_exception;

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


/* function codegen ************************************************************

	generates machine code

*******************************************************************************/

/* global code generation pointer */
u1 *mcodeptr;

void codegen()
{
	int  len, s1, s2, s3, d;
	s4   a;
	stackptr    src;
	varinfo     *var;
	basicblock  *bptr;
	instruction *iptr;
	u2 currentline=0;
	int fpu_st_offset = 0;

	xtable *ex;

	if (compileverbose) {
		char logtext[MAXLOGTEXT];
		sprintf(logtext, "Generating code: ");
		utf_sprint_classname(logtext + strlen(logtext), class->name);
		sprintf(logtext + strlen(logtext), ".");
		utf_sprint(logtext + strlen(logtext), method->name);
		utf_sprint_classname(logtext + strlen(logtext), descriptor);
		log_text(logtext);
	}

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
	(void) dseg_addlinenumbertablesize();			/* adds a reference for the length of the line number counter
								We don't know the size yet, since we evaluate the information
								during code generation, to save one additional iteration over
								the whole instructions. During code optimization the position
								could have changed to the information gotten from the class file*/
	(void) dseg_adds4(exceptiontablelength);                /* ExTableSize    */
	
	/* create exception table */

	for (ex = extable; ex != NULL; ex = ex->down) {
		dseg_addtarget(ex->start);
   		dseg_addtarget(ex->end);
		dseg_addtarget(ex->handler);
		(void) dseg_addaddress(ex->catchtype);
	}

	
	/* initialize mcode variables */
	
	mcodeptr = (u1*) mcodebase;
	mcodeend = (s4*) (mcodebase + mcodesize);
	MCODECHECK(128 + mparamcount);

	/* create stack frame (if necessary) */

	if (parentargs_base) {
		i386_alu_imm_reg(I386_SUB, parentargs_base * 8, REG_SP);
	}

	/* save return address and used callee saved registers */

  	p = parentargs_base;
	for (r = savintregcnt - 1; r >= maxsavintreguse; r--) {
 		p--; i386_mov_reg_membase(savintregs[r], REG_SP, p * 8);
	}
	for (r = savfltregcnt - 1; r >= maxsavfltreguse; r--) {
		p--; i386_fld_reg(savfltregs[r]); i386_fstpl_membase(REG_SP, p * 8);
	}

	/* save monitorenter argument */

#ifdef USE_THREADS
	if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
		if (method->flags & ACC_STATIC) {
			i386_mov_imm_reg((s4) class, REG_ITMP1);
			i386_mov_reg_membase(REG_ITMP1, REG_SP, maxmemuse * 8);

		} else {
			i386_mov_membase_reg(REG_SP, parentargs_base * 8 + 4, REG_ITMP1);
			i386_mov_reg_membase(REG_ITMP1, REG_SP, maxmemuse * 8);
		}
	}			
#endif

	/* copy argument registers to stack and call trace function with pointer
	   to arguments on stack.
	*/

	if (runverbose) {
		i386_alu_imm_reg(I386_SUB, TRACE_ARGS_NUM * 8 + 4, REG_SP);

		for (p = 0; p < mparamcount && p < TRACE_ARGS_NUM; p++) {
			t = mparamtypes[p];

			if (IS_INT_LNG_TYPE(t)) {
				if (IS_2_WORD_TYPE(t)) {
					i386_mov_membase_reg(REG_SP, 4 + (parentargs_base + TRACE_ARGS_NUM + p) * 8 + 4, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, p * 8);
					i386_mov_membase_reg(REG_SP, 4 + (parentargs_base + TRACE_ARGS_NUM + p) * 8 + 4 + 4, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, p * 8 + 4);

/*  				} else if (t == TYPE_ADR) { */
				} else {
					i386_mov_membase_reg(REG_SP, 4 + (parentargs_base + TRACE_ARGS_NUM + p) * 8 + 4, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, p * 8);
					i386_alu_reg_reg(I386_XOR, REG_ITMP1, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, p * 8 + 4);

/*  				} else { */
/*  					i386_mov_membase_reg(REG_SP, 4 + (parentargs_base + TRACE_ARGS_NUM + p) * 8 + 4, EAX); */
/*  					i386_cltd(); */
/*  					i386_mov_reg_membase(EAX, REG_SP, p * 8); */
/*  					i386_mov_reg_membase(EDX, REG_SP, p * 8 + 4); */
				}

			} else {
				if (!IS_2_WORD_TYPE(t)) {
					i386_flds_membase(REG_SP, 4 + (parentargs_base + TRACE_ARGS_NUM + p) * 8 + 4);
					i386_fstps_membase(REG_SP, p * 8);
					i386_alu_reg_reg(I386_XOR, REG_ITMP1, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, p * 8 + 4);

				} else {
					i386_fldl_membase(REG_SP, 4 + (parentargs_base + TRACE_ARGS_NUM + p) * 8 + 4);
					i386_fstpl_membase(REG_SP, p * 8);
				}
			}
		}

		/* fill up the remaining arguments */
		i386_alu_reg_reg(I386_XOR, REG_ITMP1, REG_ITMP1);
		for (p = mparamcount; p < TRACE_ARGS_NUM; p++) {
			i386_mov_reg_membase(REG_ITMP1, REG_SP, p * 8);
			i386_mov_reg_membase(REG_ITMP1, REG_SP, p * 8 + 4);
		}

		i386_mov_imm_membase((s4) method, REG_SP, TRACE_ARGS_NUM * 8);
  		i386_mov_imm_reg((s4) builtin_trace_args, REG_ITMP1);
		i386_call_reg(REG_ITMP1);

		i386_alu_imm_reg(I386_ADD, TRACE_ARGS_NUM * 8 + 4, REG_SP);
	}

	/* take arguments out of register or stack frame */

 	for (p = 0, l = 0; p < mparamcount; p++) {
 		t = mparamtypes[p];
 		var = &(locals[l][t]);
 		l++;
 		if (IS_2_WORD_TYPE(t))    /* increment local counter for 2 word types */
 			l++;
 		if (var->type < 0)
 			continue;
 		r = var->regoff; 
		if (IS_INT_LNG_TYPE(t)) {                    /* integer args          */
 			if (p < intreg_argnum) {                 /* register arguments    */
				panic("integer register argument");
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
/*   					M_INTMOVE (argintregs[p], r); */

				} else {                             /* reg arg -> spilled    */
/*   					M_LST (argintregs[p], REG_SP, 8 * r); */
 				}
			} else {                                 /* stack arguments       */
 				pa = p - intreg_argnum;
 				if (!(var->flags & INMEMORY)) {      /* stack arg -> register */ 
 					i386_mov_membase_reg(REG_SP, (parentargs_base + pa) * 8 + 4, r);            /* + 4 for return address */
				} else {                             /* stack arg -> spilled  */
					if (!IS_2_WORD_TYPE(t)) {
						i386_mov_membase_reg(REG_SP, (parentargs_base + pa) * 8 + 4, REG_ITMP1);    /* + 4 for return address */
						i386_mov_reg_membase(REG_ITMP1, REG_SP, r * 8);

					} else {
						i386_mov_membase_reg(REG_SP, (parentargs_base + pa) * 8 + 4, REG_ITMP1);    /* + 4 for return address */
						i386_mov_reg_membase(REG_ITMP1, REG_SP, r * 8);
						i386_mov_membase_reg(REG_SP, (parentargs_base + pa) * 8 + 4 + 4, REG_ITMP1);    /* + 4 for return address */
						i386_mov_reg_membase(REG_ITMP1, REG_SP, r * 8 + 4);
					}
				}
			}
		
		} else {                                     /* floating args         */   
 			if (p < fltreg_argnum) {                 /* register arguments    */
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
					panic("There are no float argument registers!");

 				} else {			                 /* reg arg -> spilled    */
					panic("There are no float argument registers!");
 				}

 			} else {                                 /* stack arguments       */
 				pa = p - fltreg_argnum;
 				if (!(var->flags & INMEMORY)) {      /* stack-arg -> register */
					if (t == TYPE_FLT) {
						i386_flds_membase(REG_SP, (parentargs_base + pa) * 8 + 4);
						fpu_st_offset++;
						i386_fstp_reg(r + fpu_st_offset);
						fpu_st_offset--;

					} else {
						i386_fldl_membase(REG_SP, (parentargs_base + pa) * 8 + 4);
						fpu_st_offset++;
						i386_fstp_reg(r + fpu_st_offset);
						fpu_st_offset--;
					}

 				} else {                              /* stack-arg -> spilled  */
/*   					i386_mov_membase_reg(REG_SP, (parentargs_base + pa) * 8 + 4, REG_ITMP1); */
/*   					i386_mov_reg_membase(REG_ITMP1, REG_SP, r * 8); */
					if (t == TYPE_FLT) {
						i386_flds_membase(REG_SP, (parentargs_base + pa) * 8 + 4);
						i386_fstps_membase(REG_SP, r * 8);

					} else {
						i386_fldl_membase(REG_SP, (parentargs_base + pa) * 8 + 4);
						i386_fstpl_membase(REG_SP, r * 8);
					}
				}
			}
		}
	}  /* end for */

	/* call monitorenter function */

#ifdef USE_THREADS
	if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
		i386_mov_membase_reg(REG_SP, maxmemuse * 8, REG_ITMP1);
		i386_alu_imm_reg(I386_SUB, 4, REG_SP);
		i386_mov_reg_membase(REG_ITMP1, REG_SP, 0);
		i386_mov_imm_reg((s4) builtin_monitorenter, REG_ITMP1);
		i386_call_reg(REG_ITMP1);
		i386_alu_imm_reg(I386_ADD, 4, REG_SP);
	}			
#endif
	}

	/* end of header generation */

	/* walk through all basic blocks */
	for (bptr = block; bptr != NULL; bptr = bptr->next) {

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
				if (!IS_2_WORD_TYPE(src->type)) {
					if (bptr->type == BBTYPE_SBR) {
						d = reg_of_var(src, REG_ITMP1);
						i386_pop_reg(d);
						store_reg_to_var_int(src, d);

					} else if (bptr->type == BBTYPE_EXH) {
						d = reg_of_var(src, REG_ITMP1);
						M_INTMOVE(REG_ITMP1, d);
						store_reg_to_var_int(src, d);
					}

				} else {
					panic("copy interface registers: longs have to me in memory (begin 1)");
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
							if (s2 == TYPE_FLT) {
								i386_flds_membase(REG_SP, s1 * 8);

							} else {
								i386_fldl_membase(REG_SP, s1 * 8);
							}
						}
						store_reg_to_var_flt(src, d);

					} else {
						s1 = interfaces[len][s2].regoff;
						if (!IS_2_WORD_TYPE(interfaces[len][s2].type)) {
							if (!(interfaces[len][s2].flags & INMEMORY)) {
								M_INTMOVE(s1, d);

							} else {
								i386_mov_membase_reg(REG_SP, s1 * 8, d);
							}
							store_reg_to_var_int(src, d);

						} else {
							if (interfaces[len][s2].flags & INMEMORY) {
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

		/* walk through all instructions */
		
		src = bptr->instack;
		len = bptr->icount;
		for (iptr = bptr->iinstr;
		    len > 0;
		    src = iptr->dst, len--, iptr++) {

	if (iptr->line!=currentline) {
		dseg_addlinenumber(iptr->line,mcodeptr);
		currentline=iptr->line;
	}
	MCODECHECK(64);           /* an instruction usually needs < 64 words      */
	switch (iptr->opc) {

		case ICMD_NOP:        /* ...  ==> ...                                 */
			break;

		case ICMD_NULLCHECKPOP: /* ..., objectref  ==> ...                    */
			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(I386_CMP, 0, REG_SP, src->regoff * 8);

			} else {
				i386_test_reg_reg(src->regoff, src->regoff);
			}
			i386_jcc(I386_CC_E, 0);
			codegen_addxnullrefs(mcodeptr);
			break;

		/* constant operations ************************************************/

		case ICMD_ICONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.i = constant                    */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				i386_mov_imm_membase(iptr->val.i, REG_SP, iptr->dst->regoff * 8);

			} else {
				if (iptr->val.i == 0) {
					i386_alu_reg_reg(I386_XOR, d, d);

				} else {
					i386_mov_imm_reg(iptr->val.i, d);
				}
			}
			break;

		case ICMD_LCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.l = constant                    */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				i386_mov_imm_membase(iptr->val.l, REG_SP, iptr->dst->regoff * 8);
				i386_mov_imm_membase(iptr->val.l >> 32, REG_SP, iptr->dst->regoff * 8 + 4);
				
			} else {
				panic("LCONST: longs have to be in memory");
			}
			break;

		case ICMD_FCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.f = constant                    */

			d = reg_of_var(iptr->dst, REG_FTMP1);
			if (iptr->val.f == 0.0) {
				i386_fldz();
				fpu_st_offset++;

				/* -0.0 */
				if (iptr->val.i == 0x80000000) {
					i386_fchs();
				}

			} else if (iptr->val.f == 1.0) {
				i386_fld1();
				fpu_st_offset++;

			} else if (iptr->val.f == 2.0) {
				i386_fld1();
				i386_fld1();
				i386_faddp();
				fpu_st_offset++;

			} else {
  				a = dseg_addfloat(iptr->val.f);
				i386_mov_imm_reg(0, REG_ITMP1);
				dseg_adddata(mcodeptr);
				i386_flds_membase(REG_ITMP1, a);
				fpu_st_offset++;
			}
			store_reg_to_var_flt(iptr->dst, d);
			break;
		
		case ICMD_DCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.d = constant                    */

			d = reg_of_var(iptr->dst, REG_FTMP1);
			if (iptr->val.d == 0.0) {
				i386_fldz();
				fpu_st_offset++;

				/* -0.0 */
				if (iptr->val.l == 0x8000000000000000LL) {
					i386_fchs();
				}

			} else if (iptr->val.d == 1.0) {
				i386_fld1();
				fpu_st_offset++;

			} else if (iptr->val.d == 2.0) {
				i386_fld1();
				i386_fld1();
				i386_faddp();
				fpu_st_offset++;

			} else {
				a = dseg_adddouble(iptr->val.d);
				i386_mov_imm_reg(0, REG_ITMP1);
				dseg_adddata(mcodeptr);
				i386_fldl_membase(REG_ITMP1, a);
				fpu_st_offset++;
			}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_ACONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.a = constant                    */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				i386_mov_imm_membase((s4) iptr->val.a, REG_SP, iptr->dst->regoff * 8);

			} else {
				if ((s4) iptr->val.a == 0) {
					i386_alu_reg_reg(I386_XOR, d, d);

				} else {
					i386_mov_imm_reg((s4) iptr->val.a, d);
				}
			}
			break;


		/* load/store operations **********************************************/

		case ICMD_ILOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_ALOAD:      /* op1 = local variable                         */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1)) {
				break;
			}
			var = &(locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (iptr->dst->flags & INMEMORY) {
				if (var->flags & INMEMORY) {
					i386_mov_membase_reg(REG_SP, var->regoff * 8, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else {
					i386_mov_reg_membase(var->regoff, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (var->flags & INMEMORY) {
					i386_mov_membase_reg(REG_SP, var->regoff * 8, iptr->dst->regoff);

				} else {
					M_INTMOVE(var->regoff, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_LLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1)) {
				break;
			}
			var = &(locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
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

			d = reg_of_var(iptr->dst, REG_FTMP1);
  			if ((iptr->dst->varkind == LOCALVAR) &&
  			    (iptr->dst->varnum == iptr->op1)) {
    				break;
  			}
			var = &(locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY) {
				i386_flds_membase(REG_SP, var->regoff * 8);
				fpu_st_offset++;
			} else {
				i386_fld_reg(var->regoff + fpu_st_offset);
				fpu_st_offset++;
			}
  			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			d = reg_of_var(iptr->dst, REG_FTMP1);
  			if ((iptr->dst->varkind == LOCALVAR) &&
  			    (iptr->dst->varnum == iptr->op1)) {
    				break;
  			}
			var = &(locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY) {
				i386_fldl_membase(REG_SP, var->regoff * 8);
				fpu_st_offset++;
			} else {
				i386_fld_reg(var->regoff + fpu_st_offset);
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
			var = &(locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, var->regoff * 8);
					
				} else {
					i386_mov_reg_membase(src->regoff, REG_SP, var->regoff * 8);
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
			var = &(locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
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
			var = &(locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				var_to_reg_flt(s1, src, REG_FTMP1);
				i386_fstps_membase(REG_SP, var->regoff * 8);
				fpu_st_offset--;
			} else {
				var_to_reg_flt(s1, src, var->regoff);
/*  				M_FLTMOVE(s1, var->regoff); */
				i386_fstp_reg(var->regoff + fpu_st_offset);
				fpu_st_offset--;
			}
			break;

		case ICMD_DSTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			if ((src->varkind == LOCALVAR) &&
			    (src->varnum == iptr->op1)) {
				break;
			}
			var = &(locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				var_to_reg_flt(s1, src, REG_FTMP1);
				i386_fstpl_membase(REG_SP, var->regoff * 8);
				fpu_st_offset--;
			} else {
				var_to_reg_flt(s1, src, var->regoff);
/*  				M_FLTMOVE(s1, var->regoff); */
				i386_fstp_reg(var->regoff + fpu_st_offset);
				fpu_st_offset--;
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
/*					M_FLTMOVE(s1, d);*/ \
					store_reg_to_var_flt(to, d); \
				} else { \
                    if (!IS_2_WORD_TYPE(from->type)) { \
                        if (to->flags & INMEMORY) { \
                             if (from->flags & INMEMORY) { \
                                 i386_mov_membase_reg(REG_SP, from->regoff * 8, REG_ITMP1); \
                                 i386_mov_reg_membase(REG_ITMP1, REG_SP, to->regoff * 8); \
                             } else { \
                                 i386_mov_reg_membase(from->regoff, REG_SP, to->regoff * 8); \
                             } \
                        } else { \
                             if (from->flags & INMEMORY) { \
                                 i386_mov_membase_reg(REG_SP, from->regoff * 8, to->regoff); \
                             } else { \
                                 i386_mov_reg_reg(from->regoff, to->regoff); \
                             } \
                        } \
                    } else { \
                        M_LNGMEMMOVE(from->regoff, to->regoff); \
                    } \
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
						i386_neg_membase(REG_SP, iptr->dst->regoff * 8);

					} else {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_neg_reg(REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					i386_mov_reg_membase(src->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_neg_membase(REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);
					i386_neg_reg(iptr->dst->regoff);

				} else {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					i386_neg_reg(iptr->dst->regoff);
				}
			}
			break;

		case ICMD_LNEG:       /* ..., value  ==> ..., - value                 */

			d = reg_of_var(iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_neg_membase(REG_SP, iptr->dst->regoff * 8);
						i386_alu_imm_membase(I386_ADC, 0, REG_SP, iptr->dst->regoff * 8 + 4);
						i386_neg_membase(REG_SP, iptr->dst->regoff * 8 + 4);

					} else {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_neg_reg(REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP1);
						i386_alu_imm_reg(I386_ADC, 0, REG_ITMP1);
						i386_neg_reg(REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);
					}
				}
			}
			break;

		case ICMD_I2L:        /* ..., value  ==> ..., value                   */

			d = reg_of_var(iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, EAX);
					i386_cltd();
					i386_mov_reg_membase(EAX, REG_SP, iptr->dst->regoff * 8);
					i386_mov_reg_membase(EDX, REG_SP, iptr->dst->regoff * 8 + 4);

				} else {
					M_INTMOVE(src->regoff, EAX);
					i386_cltd();
					i386_mov_reg_membase(EAX, REG_SP, iptr->dst->regoff * 8);
					i386_mov_reg_membase(EDX, REG_SP, iptr->dst->regoff * 8 + 4);
				}
			}
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */

			d = reg_of_var(iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			d = reg_of_var(iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					i386_shift_imm_reg(I386_SHL, 24, REG_ITMP1);
					i386_shift_imm_reg(I386_SAR, 24, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else {
					i386_mov_reg_membase(src->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_shift_imm_membase(I386_SHL, 24, REG_SP, iptr->dst->regoff * 8);
					i386_shift_imm_membase(I386_SAR, 24, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);
					i386_shift_imm_reg(I386_SHL, 24, iptr->dst->regoff);
					i386_shift_imm_reg(I386_SAR, 24, iptr->dst->regoff);

				} else {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					i386_shift_imm_reg(I386_SHL, 24, iptr->dst->regoff);
					i386_shift_imm_reg(I386_SAR, 24, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_INT2CHAR:   /* ..., value  ==> ..., value                   */

			d = reg_of_var(iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_imm_membase(I386_AND, 0x0000ffff, REG_SP, iptr->dst->regoff * 8);

					} else {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_imm_reg(I386_AND, 0x0000ffff, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					i386_mov_reg_membase(src->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_alu_imm_membase(I386_AND, 0x0000ffff, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);
					i386_alu_imm_reg(I386_AND, 0x0000ffff, iptr->dst->regoff);

				} else {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					i386_alu_imm_reg(I386_AND, 0x0000ffff, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_INT2SHORT:  /* ..., value  ==> ..., value                   */

			d = reg_of_var(iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					i386_shift_imm_reg(I386_SHL, 16, REG_ITMP1);
					i386_shift_imm_reg(I386_SAR, 16, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else {
					i386_mov_reg_membase(src->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_shift_imm_membase(I386_SHL, 16, REG_SP, iptr->dst->regoff * 8);
					i386_shift_imm_membase(I386_SAR, 16, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);
					i386_shift_imm_reg(I386_SHL, 16, iptr->dst->regoff);
					i386_shift_imm_reg(I386_SAR, 16, iptr->dst->regoff);

				} else {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					i386_shift_imm_reg(I386_SHL, 16, iptr->dst->regoff);
					i386_shift_imm_reg(I386_SAR, 16, iptr->dst->regoff);
				}
			}
			break;


		case ICMD_IADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_ialu(I386_ADD, src, iptr);
			break;

		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			/* should we use a inc optimization for smaller code size? */
			i386_emit_ialuconst(I386_ADD, src, iptr);
			break;

		case ICMD_LADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_alu_reg_membase(I386_ADD, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP1);
						i386_alu_reg_membase(I386_ADC, REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);

					} else if (src->prev->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_reg_membase(I386_ADD, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP1);
						i386_alu_reg_membase(I386_ADC, REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);

					} else {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_alu_membase_reg(I386_ADD, REG_SP, src->regoff * 8, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP1);
						i386_alu_membase_reg(I386_ADC, REG_SP, src->regoff * 8 + 4, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);
					}

				}
			}
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_imm_membase(I386_ADD, iptr->val.l, REG_SP, iptr->dst->regoff * 8);
						i386_alu_imm_membase(I386_ADC, iptr->val.l >> 32, REG_SP, iptr->dst->regoff * 8 + 4);

					} else {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_imm_reg(I386_ADD, iptr->val.l, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP1);
						i386_alu_imm_reg(I386_ADC, iptr->val.l >> 32, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);
					}
				}
			}
			break;

		case ICMD_ISUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_reg_membase(I386_SUB, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

					} else {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_alu_membase_reg(I386_SUB, REG_SP, src->regoff * 8, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, REG_ITMP1);
					i386_alu_membase_reg(I386_SUB, REG_SP, src->regoff * 8, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						i386_alu_reg_membase(I386_SUB, src->regoff, REG_SP, iptr->dst->regoff * 8);

					} else {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_alu_reg_reg(I386_SUB, src->regoff, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					i386_mov_reg_membase(src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_alu_reg_membase(I386_SUB, src->regoff, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, d);
					i386_alu_membase_reg(I386_SUB, REG_SP, src->regoff * 8, d);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, d);
					i386_alu_membase_reg(I386_SUB, REG_SP, src->regoff * 8, d);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					/* workaround for reg alloc */
					if (src->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_alu_reg_reg(I386_SUB, src->regoff, REG_ITMP1);
						M_INTMOVE(REG_ITMP1, d);

					} else {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, d);
						i386_alu_reg_reg(I386_SUB, src->regoff, d);
					}

				} else {
					/* workaround for reg alloc */
					if (src->regoff == iptr->dst->regoff) {
						M_INTMOVE(src->prev->regoff, REG_ITMP1);
						i386_alu_reg_reg(I386_SUB, src->regoff, REG_ITMP1);
						M_INTMOVE(REG_ITMP1, d);

					} else {
						M_INTMOVE(src->prev->regoff, d);
						i386_alu_reg_reg(I386_SUB, src->regoff, d);
					}
				}
			}
			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_ialuconst(I386_SUB, src, iptr);
			break;

		case ICMD_LSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_reg_membase(I386_SUB, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP1);
						i386_alu_reg_membase(I386_SBB, REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);

					} else {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_alu_membase_reg(I386_SUB, REG_SP, src->regoff * 8, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP1);
						i386_alu_membase_reg(I386_SBB, REG_SP, src->regoff * 8 + 4, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);
					}
				}
			}
			break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_imm_membase(I386_SUB, iptr->val.l, REG_SP, iptr->dst->regoff * 8);
						i386_alu_imm_membase(I386_SBB, iptr->val.l >> 32, REG_SP, iptr->dst->regoff * 8 + 4);

					} else {
						/* TODO: could be size optimized with lea -- see gcc output */
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_imm_reg(I386_SUB, iptr->val.l, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP1);
						i386_alu_imm_reg(I386_SBB, iptr->val.l >> 32, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);
					}
				}
			}
			break;

		case ICMD_IMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
					i386_imul_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					i386_imul_reg_reg(src->prev->regoff, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
					i386_imul_reg_reg(src->regoff, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else {
					i386_mov_reg_reg(src->prev->regoff, REG_ITMP1);
					i386_imul_reg_reg(src->regoff, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
					i386_imul_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
					i386_imul_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					i386_imul_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);

				} else {
					if (src->regoff == iptr->dst->regoff) {
						i386_imul_reg_reg(src->prev->regoff, iptr->dst->regoff);

					} else {
						M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
						i386_imul_reg_reg(src->regoff, iptr->dst->regoff);
					}
				}
			}
			break;

		case ICMD_IMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_imul_imm_membase_reg(iptr->val.i, REG_SP, src->regoff * 8, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else {
					i386_imul_imm_reg_reg(iptr->val.i, src->regoff, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_imul_imm_membase_reg(iptr->val.i, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else {
					i386_imul_imm_reg_reg(iptr->val.i, src->regoff, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_LMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, EAX);             /* mem -> EAX             */
					/* optimize move EAX -> REG_ITMP3 is slower??? */
/*    					i386_mov_reg_reg(EAX, REG_ITMP3); */
					i386_mul_membase(REG_SP, src->regoff * 8);                            /* mem * EAX -> EDX:EAX   */

					/* TODO: optimize move EAX -> REG_ITMP3 */
  					i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2);   /* mem -> ITMP3           */
					i386_imul_membase_reg(REG_SP, src->regoff * 8, REG_ITMP2);            /* mem * ITMP3 -> ITMP3   */
					i386_alu_reg_reg(I386_ADD, REG_ITMP2, EDX);                      /* ITMP3 + EDX -> EDX     */

					i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP2);       /* mem -> ITMP3           */
					i386_imul_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2);        /* mem * ITMP3 -> ITMP3   */

					i386_alu_reg_reg(I386_ADD, REG_ITMP2, EDX);                      /* ITMP3 + EDX -> EDX     */
					i386_mov_reg_membase(EAX, REG_SP, iptr->dst->regoff * 8);
					i386_mov_reg_membase(EDX, REG_SP, iptr->dst->regoff * 8 + 4);
				}
			}
			break;

		case ICMD_LMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_mov_imm_reg(iptr->val.l, EAX);                                   /* imm -> EAX             */
					i386_mul_membase(REG_SP, src->regoff * 8);                            /* mem * EAX -> EDX:EAX   */
					/* TODO: optimize move EAX -> REG_ITMP3 */
					i386_mov_imm_reg(iptr->val.l >> 32, REG_ITMP2);                       /* imm -> ITMP3           */
					i386_imul_membase_reg(REG_SP, src->regoff * 8, REG_ITMP2);            /* mem * ITMP3 -> ITMP3   */

					i386_alu_reg_reg(I386_ADD, REG_ITMP2, EDX);                      /* ITMP3 + EDX -> EDX     */
					i386_mov_imm_reg(iptr->val.l, REG_ITMP2);                             /* imm -> ITMP3           */
					i386_imul_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2);        /* mem * ITMP3 -> ITMP3   */

					i386_alu_reg_reg(I386_ADD, REG_ITMP2, EDX);                      /* ITMP3 + EDX -> EDX     */
					i386_mov_reg_membase(EAX, REG_SP, iptr->dst->regoff * 8);
					i386_mov_reg_membase(EDX, REG_SP, iptr->dst->regoff * 8 + 4);
				}
			}
			break;

#define gen_div_check(v) \
    if (checknull) { \
        if ((v)->flags & INMEMORY) { \
            i386_alu_imm_membase(I386_CMP, 0, REG_SP, src->regoff * 8); \
        } else { \
            i386_test_reg_reg(src->regoff, src->regoff); \
        } \
        i386_jcc(I386_CC_E, 0); \
        codegen_addxdivrefs(mcodeptr); \
    }

		case ICMD_IDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
			var_to_reg_int(s1, src, REG_ITMP2);
  			gen_div_check(src);
	        if (src->prev->flags & INMEMORY) {
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, EAX);

			} else {
				M_INTMOVE(src->prev->regoff, EAX);
			}
			
			i386_alu_imm_reg(I386_CMP, 0x80000000, EAX);    /* check as described in jvm spec */
			i386_jcc(I386_CC_NE, 3 + 6);
			i386_alu_imm_reg(I386_CMP, -1, s1);
			i386_jcc(I386_CC_E, 1 + 2);

  			i386_cltd();
			i386_idiv_reg(s1);

			if (iptr->dst->flags & INMEMORY) {
				i386_mov_reg_membase(EAX, REG_SP, iptr->dst->regoff * 8);

			} else {
				M_INTMOVE(EAX, iptr->dst->regoff);
			}
			break;

		case ICMD_IREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
			var_to_reg_int(s1, src, REG_ITMP2);
  			gen_div_check(src);
			if (src->prev->flags & INMEMORY) {
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, EAX);

			} else {
				M_INTMOVE(src->prev->regoff, EAX);
			}
			
			i386_alu_imm_reg(I386_CMP, 0x80000000, EAX);    /* check as described in jvm spec */
			i386_jcc(I386_CC_NE, 2 + 3 + 6);
			i386_alu_reg_reg(I386_XOR, EDX, EDX);
			i386_alu_imm_reg(I386_CMP, -1, s1);
			i386_jcc(I386_CC_E, 1 + 2);

  			i386_cltd();
			i386_idiv_reg(s1);

			if (iptr->dst->flags & INMEMORY) {
				i386_mov_reg_membase(EDX, REG_SP, iptr->dst->regoff * 8);

			} else {
				M_INTMOVE(EDX, iptr->dst->regoff);
			}
			break;

		case ICMD_IDIVPOW2:   /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			/* TODO: optimize for `/ 2' */
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP1);

			M_INTMOVE(s1, d);
			i386_test_reg_reg(d, d);
			a = 2;
			CALCIMMEDIATEBYTES(a, (1 << iptr->val.i) - 1);
			i386_jcc(I386_CC_NS, a);
			i386_alu_imm_reg(I386_ADD, (1 << iptr->val.i) - 1, d);
				
			i386_shift_imm_reg(I386_SAR, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LDIVPOW2:   /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					a = 2;
					CALCIMMEDIATEBYTES(a, (1 << iptr->val.i) - 1);
					a += 3;
					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2);

					i386_test_reg_reg(REG_ITMP2, REG_ITMP2);
					i386_jcc(I386_CC_NS, a);
					i386_alu_imm_reg(I386_ADD, (1 << iptr->val.i) - 1, REG_ITMP1);
					i386_alu_imm_reg(I386_ADC, 0, REG_ITMP2);
					i386_shrd_imm_reg_reg(iptr->val.i, REG_ITMP2, REG_ITMP1);
					i386_shift_imm_reg(I386_SAR, iptr->val.i, REG_ITMP2);

					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
				}
			}
			break;

		case ICMD_IREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP2);
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
			i386_alu_imm_reg(I386_AND, iptr->val.i, d);
			i386_test_reg_reg(s1, s1);
			i386_jcc(I386_CC_GE, a);
 			i386_mov_reg_reg(s1, d);
  			i386_neg_reg(d);
  			i386_alu_imm_reg(I386_AND, iptr->val.i, d);
			i386_neg_reg(d);

/*  			M_INTMOVE(s1, EAX); */
/*  			i386_cltd(); */
/*  			i386_alu_reg_reg(I386_XOR, EDX, EAX); */
/*  			i386_alu_reg_reg(I386_SUB, EDX, EAX); */
/*  			i386_alu_reg_reg(I386_AND, iptr->val.i, EAX); */
/*  			i386_alu_reg_reg(I386_XOR, EDX, EAX); */
/*  			i386_alu_reg_reg(I386_SUB, EDX, EAX); */
/*  			M_INTMOVE(EAX, d); */

/*  			i386_alu_reg_reg(I386_XOR, d, d); */
/*  			i386_mov_imm_reg(iptr->val.i, ECX); */
/*  			i386_shrd_reg_reg(s1, d); */
/*  			i386_shift_imm_reg(I386_SHR, 32 - iptr->val.i, d); */

			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					/* Intel algorithm -- does not work, because constant is wrong */
/*  					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1); */
/*  					i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP3); */

/*  					M_INTMOVE(REG_ITMP1, REG_ITMP2); */
/*  					i386_test_reg_reg(REG_ITMP3, REG_ITMP3); */
/*  					i386_jcc(I386_CC_NS, offset); */
/*  					i386_alu_imm_reg(I386_ADD, (1 << iptr->val.l) - 1, REG_ITMP2); */
/*  					i386_alu_imm_reg(I386_ADC, 0, REG_ITMP3); */
					
/*  					i386_shrd_imm_reg_reg(iptr->val.l, REG_ITMP3, REG_ITMP2); */
/*  					i386_shift_imm_reg(I386_SAR, iptr->val.l, REG_ITMP3); */
/*  					i386_shld_imm_reg_reg(iptr->val.l, REG_ITMP2, REG_ITMP3); */

/*  					i386_shift_imm_reg(I386_SHL, iptr->val.l, REG_ITMP2); */

/*  					i386_alu_reg_reg(I386_SUB, REG_ITMP2, REG_ITMP1); */
/*  					i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2); */
/*  					i386_alu_reg_reg(I386_SBB, REG_ITMP3, REG_ITMP2); */

/*  					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8); */
/*  					i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4); */

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

					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2);
					
					i386_alu_imm_reg(I386_AND, iptr->val.l, REG_ITMP1);
					i386_alu_imm_reg(I386_AND, iptr->val.l >> 32, REG_ITMP2);
					i386_alu_imm_membase(I386_CMP, 0, REG_SP, src->regoff * 8 + 4);
					i386_jcc(I386_CC_GE, a);

					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2);
					
					i386_neg_reg(REG_ITMP1);
					i386_alu_imm_reg(I386_ADC, 0, REG_ITMP2);
					i386_neg_reg(REG_ITMP2);
					
					i386_alu_imm_reg(I386_AND, iptr->val.l, REG_ITMP1);
					i386_alu_imm_reg(I386_AND, iptr->val.l >> 32, REG_ITMP2);
					
					i386_neg_reg(REG_ITMP1);
					i386_alu_imm_reg(I386_ADC, 0, REG_ITMP2);
					i386_neg_reg(REG_ITMP2);

					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
				}
			}
			break;

		case ICMD_ISHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_ishift(I386_SHL, src, iptr);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_ishiftconst(I386_SHL, src, iptr);
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_ishift(I386_SAR, src, iptr);
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_ishiftconst(I386_SAR, src, iptr);
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_ishift(I386_SHR, src, iptr);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_ishiftconst(I386_SHR, src, iptr);
			break;

		case ICMD_LSHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			d = reg_of_var(iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY ){
				if (src->prev->flags & INMEMORY) {
/*  					if (src->prev->regoff == iptr->dst->regoff) { */
/*  						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1); */

/*  						if (src->flags & INMEMORY) { */
/*  							i386_mov_membase_reg(REG_SP, src->regoff * 8, ECX); */
/*  						} else { */
/*  							M_INTMOVE(src->regoff, ECX); */
/*  						} */

/*  						i386_test_imm_reg(32, ECX); */
/*  						i386_jcc(I386_CC_E, 2 + 2); */
/*  						i386_mov_reg_reg(REG_ITMP1, REG_ITMP2); */
/*  						i386_alu_reg_reg(I386_XOR, REG_ITMP1, REG_ITMP1); */
						
/*  						i386_shld_reg_membase(REG_ITMP1, REG_SP, src->prev->regoff * 8 + 4); */
/*  						i386_shift_membase(I386_SHL, REG_SP, iptr->dst->regoff * 8); */

/*  					} else { */
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP3);
						
						if (src->flags & INMEMORY) {
							i386_mov_membase_reg(REG_SP, src->regoff * 8, ECX);
						} else {
							M_INTMOVE(src->regoff, ECX);
						}
						
						i386_test_imm_reg(32, ECX);
						i386_jcc(I386_CC_E, 2 + 2);
						i386_mov_reg_reg(REG_ITMP1, REG_ITMP3);
						i386_alu_reg_reg(I386_XOR, REG_ITMP1, REG_ITMP1);
						
						i386_shld_reg_reg(REG_ITMP1, REG_ITMP3);
						i386_shift_reg(I386_SHL, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_reg_membase(REG_ITMP3, REG_SP, iptr->dst->regoff * 8 + 4);
/*  					} */
				}
			}
			break;

        case ICMD_LSHLCONST:  /* ..., value  ==> ..., value << constant       */
 			                  /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY ) {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
				i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2);

				if (iptr->val.i & 0x20) {
					i386_mov_reg_reg(REG_ITMP1, REG_ITMP2);
					i386_alu_reg_reg(I386_XOR, REG_ITMP1, REG_ITMP1);
					i386_shld_imm_reg_reg(iptr->val.i & 0x3f, REG_ITMP1, REG_ITMP2);

				} else {
					i386_shld_imm_reg_reg(iptr->val.i & 0x3f, REG_ITMP1, REG_ITMP2);
					i386_shift_imm_reg(I386_SHL, iptr->val.i & 0x3f, REG_ITMP1);
				}

				i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
			}
			break;

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			d = reg_of_var(iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY ){
				if (src->prev->flags & INMEMORY) {
/*  					if (src->prev->regoff == iptr->dst->regoff) { */
  						/* TODO: optimize */
/*  						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1); */
/*  						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2); */

/*  						if (src->flags & INMEMORY) { */
/*  							i386_mov_membase_reg(REG_SP, src->regoff * 8, ECX); */
/*  						} else { */
/*  							M_INTMOVE(src->regoff, ECX); */
/*  						} */

/*  						i386_test_imm_reg(32, ECX); */
/*  						i386_jcc(I386_CC_E, 2 + 3); */
/*  						i386_mov_reg_reg(REG_ITMP2, REG_ITMP1); */
/*  						i386_shift_imm_reg(I386_SAR, 31, REG_ITMP2); */
						
/*  						i386_shrd_reg_reg(REG_ITMP2, REG_ITMP1); */
/*  						i386_shift_reg(I386_SAR, REG_ITMP2); */
/*  						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8); */
/*  						i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4); */

/*  					} else { */
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP3);

						if (src->flags & INMEMORY) {
							i386_mov_membase_reg(REG_SP, src->regoff * 8, ECX);
						} else {
							M_INTMOVE(src->regoff, ECX);
						}

						i386_test_imm_reg(32, ECX);
						i386_jcc(I386_CC_E, 2 + 3);
						i386_mov_reg_reg(REG_ITMP3, REG_ITMP1);
						i386_shift_imm_reg(I386_SAR, 31, REG_ITMP3);
						
						i386_shrd_reg_reg(REG_ITMP3, REG_ITMP1);
						i386_shift_reg(I386_SAR, REG_ITMP3);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_reg_membase(REG_ITMP3, REG_SP, iptr->dst->regoff * 8 + 4);
/*  					} */
				}
			}
			break;

		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY ) {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
				i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2);

				if (iptr->val.i & 0x20) {
					i386_mov_reg_reg(REG_ITMP2, REG_ITMP1);
					i386_shift_imm_reg(I386_SAR, 31, REG_ITMP2);
					i386_shrd_imm_reg_reg(iptr->val.i & 0x3f, REG_ITMP2, REG_ITMP1);

				} else {
					i386_shrd_imm_reg_reg(iptr->val.i & 0x3f, REG_ITMP2, REG_ITMP1);
					i386_shift_imm_reg(I386_SAR, iptr->val.i & 0x3f, REG_ITMP2);
				}

				i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
			}
			break;

		case ICMD_LUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			d = reg_of_var(iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY ){
				if (src->prev->flags & INMEMORY) {
/*  					if (src->prev->regoff == iptr->dst->regoff) { */
  						/* TODO: optimize */
/*  						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1); */
/*  						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2); */

/*  						if (src->flags & INMEMORY) { */
/*  							i386_mov_membase_reg(REG_SP, src->regoff * 8, ECX); */
/*  						} else { */
/*  							M_INTMOVE(src->regoff, ECX); */
/*  						} */

/*  						i386_test_imm_reg(32, ECX); */
/*  						i386_jcc(I386_CC_E, 2 + 2); */
/*  						i386_mov_reg_reg(REG_ITMP2, REG_ITMP1); */
/*  						i386_alu_reg_reg(I386_XOR, REG_ITMP2, REG_ITMP2); */
						
/*  						i386_shrd_reg_reg(REG_ITMP2, REG_ITMP1); */
/*  						i386_shift_reg(I386_SHR, REG_ITMP2); */
/*  						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8); */
/*  						i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4); */

/*  					} else { */
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP3);

						if (src->flags & INMEMORY) {
							i386_mov_membase_reg(REG_SP, src->regoff * 8, ECX);
						} else {
							M_INTMOVE(src->regoff, ECX);
						}

						i386_test_imm_reg(32, ECX);
						i386_jcc(I386_CC_E, 2 + 2);
						i386_mov_reg_reg(REG_ITMP3, REG_ITMP1);
						i386_alu_reg_reg(I386_XOR, REG_ITMP3, REG_ITMP3);
						
						i386_shrd_reg_reg(REG_ITMP3, REG_ITMP1);
						i386_shift_reg(I386_SHR, REG_ITMP3);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_reg_membase(REG_ITMP3, REG_SP, iptr->dst->regoff * 8 + 4);
/*  					} */
				}
			}
			break;

  		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
  		                      /* val.l = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			if (iptr->dst->flags & INMEMORY ) {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
				i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2);

				if (iptr->val.i & 0x20) {
					i386_mov_reg_reg(REG_ITMP2, REG_ITMP1);
					i386_alu_reg_reg(I386_XOR, REG_ITMP2, REG_ITMP2);
					i386_shrd_imm_reg_reg(iptr->val.i & 0x3f, REG_ITMP2, REG_ITMP1);

				} else {
					i386_shrd_imm_reg_reg(iptr->val.i & 0x3f, REG_ITMP2, REG_ITMP1);
					i386_shift_imm_reg(I386_SHR, iptr->val.i & 0x3f, REG_ITMP2);
				}

				i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
			}
  			break;

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_ialu(I386_AND, src, iptr);
			break;

		case ICMD_IANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_ialuconst(I386_AND, src, iptr);
			break;

		case ICMD_LAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_lalu(I386_AND, src, iptr);
			break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_laluconst(I386_AND, src, iptr);
			break;

		case ICMD_IOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_ialu(I386_OR, src, iptr);
			break;

		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_ialuconst(I386_OR, src, iptr);
			break;

		case ICMD_LOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_lalu(I386_OR, src, iptr);
			break;

		case ICMD_LORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_laluconst(I386_OR, src, iptr);
			break;

		case ICMD_IXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_ialu(I386_XOR, src, iptr);
			break;

		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_ialuconst(I386_XOR, src, iptr);
			break;

		case ICMD_LXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_lalu(I386_XOR, src, iptr);
			break;

		case ICMD_LXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_laluconst(I386_XOR, src, iptr);
			break;

		case ICMD_IINC:       /* ..., value  ==> ..., value + constant        */
		                      /* op1 = variable, val.i = constant             */

			var = &(locals[iptr->op1][TYPE_INT]);
			if (var->flags & INMEMORY) {
				i386_alu_imm_membase(I386_ADD, iptr->val.i, REG_SP, var->regoff * 8);

			} else {
				i386_alu_imm_reg(I386_ADD, iptr->val.i, var->regoff);
			}
			break;


		/* floating operations ************************************************/
#if 0
#define ROUND_TO_SINGLE \
			i386_fstps_membase(REG_SP, -8); \
			i386_flds_membase(REG_SP, -8);

#define ROUND_TO_DOUBLE \
			i386_fstpl_membase(REG_SP, -8); \
			i386_fldl_membase(REG_SP, -8);

#define FPU_SET_24BIT_MODE \
			if (!fpu_in_24bit_mode) { \
				i386_fldcw_mem(&fpu_ctrlwrd_24bit); \
				fpu_in_24bit_mode = 1; \
			}

#define FPU_SET_53BIT_MODE \
			if (fpu_in_24bit_mode) { \
				i386_fldcw_mem(&fpu_ctrlwrd_53bit); \
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
			d = reg_of_var(iptr->dst, REG_FTMP3);
			i386_fchs();
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */

			FPU_SET_53BIT_MODE;
			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			i386_fchs();
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			FPU_SET_24BIT_MODE;
			d = reg_of_var(iptr->dst, REG_FTMP3);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			i386_faddp();
			fpu_st_offset--;
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			FPU_SET_53BIT_MODE;
			d = reg_of_var(iptr->dst, REG_FTMP3);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			i386_faddp();
			fpu_st_offset--;
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			FPU_SET_24BIT_MODE;
			d = reg_of_var(iptr->dst, REG_FTMP3);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			i386_fsubp();
			fpu_st_offset--;
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			FPU_SET_53BIT_MODE;
			d = reg_of_var(iptr->dst, REG_FTMP3);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			i386_fsubp();
			fpu_st_offset--;
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			FPU_SET_24BIT_MODE;
			d = reg_of_var(iptr->dst, REG_FTMP3);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			i386_fmulp();
			fpu_st_offset--;
			ROUND_TO_SINGLE;
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			FPU_SET_53BIT_MODE;
			d = reg_of_var(iptr->dst, REG_FTMP3);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);

/*  			i386_fldt_mem(subnormal_bias1); */
/*  			i386_fmulp(); */

			var_to_reg_flt(s2, src, REG_FTMP2);

			i386_fmulp();
			fpu_st_offset--;

/*  			i386_fldt_mem(subnormal_bias2); */
/*  			i386_fmulp(); */

			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			FPU_SET_24BIT_MODE;
			d = reg_of_var(iptr->dst, REG_FTMP3);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			i386_fdivp();
			fpu_st_offset--;
			ROUND_TO_SINGLE;
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			FPU_SET_53BIT_MODE;
			d = reg_of_var(iptr->dst, REG_FTMP3);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);

/*  			i386_fldt_mem(subnormal_bias1); */
/*  			i386_fmulp(); */

			var_to_reg_flt(s2, src, REG_FTMP2);

			i386_fdivp();
			fpu_st_offset--;

/*  			i386_fldt_mem(subnormal_bias2); */
/*  			i386_fmulp(); */

			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			FPU_SET_24BIT_MODE;
			/* exchanged to skip fxch */
			var_to_reg_flt(s2, src, REG_FTMP2);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_FTMP3);
/*  			i386_fxch(); */
			i386_fprem();
			i386_wait();
			i386_fnstsw();
			i386_sahf();
			i386_jcc(I386_CC_P, -(2 + 1 + 2 + 1 + 6));
			store_reg_to_var_flt(iptr->dst, d);
			i386_ffree_reg(0);
			i386_fincstp();
			fpu_st_offset--;
			break;

		case ICMD_DREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			FPU_SET_53BIT_MODE;
			/* exchanged to skip fxch */
			var_to_reg_flt(s2, src, REG_FTMP2);
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_FTMP3);
/*  			i386_fxch(); */
			i386_fprem();
			i386_wait();
			i386_fnstsw();
			i386_sahf();
			i386_jcc(I386_CC_P, -(2 + 1 + 2 + 1 + 6));
			store_reg_to_var_flt(iptr->dst, d);
			i386_ffree_reg(0);
			i386_fincstp();
			fpu_st_offset--;
			break;

		case ICMD_I2F:       /* ..., value  ==> ..., (float) value            */
		case ICMD_I2D:       /* ..., value  ==> ..., (double) value           */

			d = reg_of_var(iptr->dst, REG_FTMP1);
			if (src->flags & INMEMORY) {
				i386_fildl_membase(REG_SP, src->regoff * 8);
				fpu_st_offset++;

			} else {
				a = dseg_adds4(0);
				i386_mov_imm_reg(0, REG_ITMP1);
				dseg_adddata(mcodeptr);
				i386_mov_reg_membase(src->regoff, REG_ITMP1, a);
				i386_fildl_membase(REG_ITMP1, a);
				fpu_st_offset++;
			}
  			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_L2F:       /* ..., value  ==> ..., (float) value            */
		case ICMD_L2D:       /* ..., value  ==> ..., (double) value           */

			d = reg_of_var(iptr->dst, REG_FTMP1);
			if (src->flags & INMEMORY) {
				i386_fildll_membase(REG_SP, src->regoff * 8);
				fpu_st_offset++;

			} else {
				panic("L2F: longs have to be in memory");
			}
  			store_reg_to_var_flt(iptr->dst, d);
			break;
			
		case ICMD_F2I:       /* ..., value  ==> ..., (int) value              */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_NULL);

			a = dseg_adds4(0x0e7f);    /* Round to zero, 53-bit mode, exception masked */
			i386_mov_imm_reg(0, REG_ITMP1);
			dseg_adddata(mcodeptr);
			i386_fldcw_membase(REG_ITMP1, a);

			if (iptr->dst->flags & INMEMORY) {
				i386_fistpl_membase(REG_SP, iptr->dst->regoff * 8);
				fpu_st_offset--;

				a = dseg_adds4(0x027f);    /* Round to nearest, 53-bit mode, exceptions masked */
				i386_fldcw_membase(REG_ITMP1, a);

				i386_alu_imm_membase(I386_CMP, 0x80000000, REG_SP, iptr->dst->regoff * 8);

				a = 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				a += 5 + 2 + 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 8);

			} else {
				a = dseg_adds4(0);
				i386_fistpl_membase(REG_ITMP1, a);
				fpu_st_offset--;
				i386_mov_membase_reg(REG_ITMP1, a, iptr->dst->regoff);

				a = dseg_adds4(0x027f);    /* Round to nearest, 53-bit mode, exceptions masked */
				i386_fldcw_membase(REG_ITMP1, a);

				i386_alu_imm_reg(I386_CMP, 0x80000000, iptr->dst->regoff);

				a = 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				a += 5 + 2 + ((REG_RESULT == iptr->dst->regoff) ? 0 : 2);
			}

			i386_jcc(I386_CC_NE, a);

			/* XXX: change this when we use registers */
			i386_flds_membase(REG_SP, src->regoff * 8);
			i386_mov_imm_reg((s4) asm_builtin_f2i, REG_ITMP1);
			i386_call_reg(REG_ITMP1);

			if (iptr->dst->flags & INMEMORY) {
				i386_mov_reg_membase(REG_RESULT, REG_SP, iptr->dst->regoff * 8);

			} else {
				M_INTMOVE(REG_RESULT, iptr->dst->regoff);
			}
			break;

		case ICMD_D2I:       /* ..., value  ==> ..., (int) value              */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_NULL);

			a = dseg_adds4(0x0e7f);    /* Round to zero, 53-bit mode, exception masked */
			i386_mov_imm_reg(0, REG_ITMP1);
			dseg_adddata(mcodeptr);
			i386_fldcw_membase(REG_ITMP1, a);

			if (iptr->dst->flags & INMEMORY) {
				i386_fistpl_membase(REG_SP, iptr->dst->regoff * 8);
				fpu_st_offset--;

				a = dseg_adds4(0x027f);    /* Round to nearest, 53-bit mode, exceptions masked */
				i386_fldcw_membase(REG_ITMP1, a);

  				i386_alu_imm_membase(I386_CMP, 0x80000000, REG_SP, iptr->dst->regoff * 8);

				a = 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				a += 5 + 2 + 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 8);

			} else {
				a = dseg_adds4(0);
				i386_fistpl_membase(REG_ITMP1, a);
				fpu_st_offset--;
				i386_mov_membase_reg(REG_ITMP1, a, iptr->dst->regoff);

				a = dseg_adds4(0x027f);    /* Round to nearest, 53-bit mode, exceptions masked */
				i386_fldcw_membase(REG_ITMP1, a);

				i386_alu_imm_reg(I386_CMP, 0x80000000, iptr->dst->regoff);

				a = 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				a += 5 + 2 + ((REG_RESULT == iptr->dst->regoff) ? 0 : 2);
			}

			i386_jcc(I386_CC_NE, a);

			/* XXX: change this when we use registers */
			i386_fldl_membase(REG_SP, src->regoff * 8);
			i386_mov_imm_reg((s4) asm_builtin_d2i, REG_ITMP1);
			i386_call_reg(REG_ITMP1);

			if (iptr->dst->flags & INMEMORY) {
				i386_mov_reg_membase(REG_RESULT, REG_SP, iptr->dst->regoff * 8);
			} else {
				M_INTMOVE(REG_RESULT, iptr->dst->regoff);
			}
			break;

		case ICMD_F2L:       /* ..., value  ==> ..., (long) value             */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_NULL);

			a = dseg_adds4(0x0e7f);    /* Round to zero, 53-bit mode, exception masked */
			i386_mov_imm_reg(0, REG_ITMP1);
			dseg_adddata(mcodeptr);
			i386_fldcw_membase(REG_ITMP1, a);

			if (iptr->dst->flags & INMEMORY) {
				i386_fistpll_membase(REG_SP, iptr->dst->regoff * 8);
				fpu_st_offset--;

				a = dseg_adds4(0x027f);    /* Round to nearest, 53-bit mode, exceptions masked */
				i386_fldcw_membase(REG_ITMP1, a);

  				i386_alu_imm_membase(I386_CMP, 0x80000000, REG_SP, iptr->dst->regoff * 8 + 4);

				a = 6 + 4;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 8);
				a += 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				a += 5 + 2;
				a += 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 8);
				a += 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 8 + 4);

				i386_jcc(I386_CC_NE, a);

  				i386_alu_imm_membase(I386_CMP, 0, REG_SP, iptr->dst->regoff * 8);

				a = 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				a += 5 + 2 + 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 8);

				i386_jcc(I386_CC_NE, a);

				/* XXX: change this when we use registers */
				i386_flds_membase(REG_SP, src->regoff * 8);
				i386_mov_imm_reg((s4) asm_builtin_f2l, REG_ITMP1);
				i386_call_reg(REG_ITMP1);
				i386_mov_reg_membase(REG_RESULT, REG_SP, iptr->dst->regoff * 8);
				i386_mov_reg_membase(REG_RESULT2, REG_SP, iptr->dst->regoff * 8 + 4);

			} else {
				panic("F2L: longs have to be in memory");
			}
			break;

		case ICMD_D2L:       /* ..., value  ==> ..., (long) value             */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_NULL);

			a = dseg_adds4(0x0e7f);    /* Round to zero, 53-bit mode, exception masked */
			i386_mov_imm_reg(0, REG_ITMP1);
			dseg_adddata(mcodeptr);
			i386_fldcw_membase(REG_ITMP1, a);

			if (iptr->dst->flags & INMEMORY) {
				i386_fistpll_membase(REG_SP, iptr->dst->regoff * 8);
				fpu_st_offset--;

				a = dseg_adds4(0x027f);    /* Round to nearest, 53-bit mode, exceptions masked */
				i386_fldcw_membase(REG_ITMP1, a);

  				i386_alu_imm_membase(I386_CMP, 0x80000000, REG_SP, iptr->dst->regoff * 8 + 4);

				a = 6 + 4;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 8);
				a += 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				a += 5 + 2;
				a += 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 8);
				a += 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 8 + 4);

				i386_jcc(I386_CC_NE, a);

  				i386_alu_imm_membase(I386_CMP, 0, REG_SP, iptr->dst->regoff * 8);

				a = 3;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				a += 5 + 2 + 3;
				CALCOFFSETBYTES(a, REG_SP, iptr->dst->regoff * 8);

				i386_jcc(I386_CC_NE, a);

				/* XXX: change this when we use registers */
				i386_fldl_membase(REG_SP, src->regoff * 8);
				i386_mov_imm_reg((s4) asm_builtin_d2l, REG_ITMP1);
				i386_call_reg(REG_ITMP1);
				i386_mov_reg_membase(REG_RESULT, REG_SP, iptr->dst->regoff * 8);
				i386_mov_reg_membase(REG_RESULT2, REG_SP, iptr->dst->regoff * 8 + 4);

			} else {
				panic("D2L: longs have to be in memory");
			}
			break;

		case ICMD_F2D:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			/* nothing to do */
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_D2F:       /* ..., value  ==> ..., (float) value            */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			/* nothing to do */
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */
		case ICMD_DCMPL:

			/* exchanged to skip fxch */
			var_to_reg_flt(s2, src->prev, REG_FTMP1);
			var_to_reg_flt(s1, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_ITMP1);
/*    			i386_fxch(); */
			i386_fucompp();
			fpu_st_offset -= 2;
			i386_fnstsw();
			i386_test_imm_reg(0x400, EAX);    /* unordered treat as GT */
			i386_jcc(I386_CC_E, 6);
			i386_alu_imm_reg(I386_AND, 0x000000ff, EAX);
 			i386_sahf();
			i386_mov_imm_reg(0, d);    /* does not affect flags */
  			i386_jcc(I386_CC_E, 6 + 3 + 5 + 3);
  			i386_jcc(I386_CC_B, 3 + 5);
			i386_alu_imm_reg(I386_SUB, 1, d);
			i386_jmp_imm(3);
			i386_alu_imm_reg(I386_ADD, 1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */
		case ICMD_DCMPG:

			/* exchanged to skip fxch */
			var_to_reg_flt(s2, src->prev, REG_FTMP1);
			var_to_reg_flt(s1, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_ITMP1);
/*    			i386_fxch(); */
			i386_fucompp();
			fpu_st_offset -= 2;
			i386_fnstsw();
			i386_test_imm_reg(0x400, EAX);    /* unordered treat as LT */
			i386_jcc(I386_CC_E, 3);
			i386_movb_imm_reg(1, I386_AH);
 			i386_sahf();
			i386_mov_imm_reg(0, d);    /* does not affect flags */
  			i386_jcc(I386_CC_E, 6 + 3 + 5 + 3);
  			i386_jcc(I386_CC_B, 3 + 5);
			i386_alu_imm_reg(I386_SUB, 1, d);
			i386_jmp_imm(3);
			i386_alu_imm_reg(I386_ADD, 1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;


		/* memory operations **************************************************/

#define gen_bound_check \
    if (checkbounds) { \
        i386_alu_membase_reg(I386_CMP, s1, OFFSET(java_arrayheader, size), s2); \
        i386_jcc(I386_CC_AE, 0); \
        codegen_addxboundrefs(mcodeptr, s2); \
    }

		case ICMD_ARRAYLENGTH: /* ..., arrayref  ==> ..., length              */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP1);
			gen_nullptr_check(s1);
			i386_mov_membase_reg(s1, OFFSET(java_arrayheader, size), d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_AALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			i386_mov_memindex_reg(OFFSET(java_objectarray, data[0]), s1, s2, 2, d);
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
			
			if (iptr->dst->flags & INMEMORY) {
				i386_mov_memindex_reg(OFFSET(java_longarray, data[0]), s1, s2, 3, REG_ITMP3);
				i386_mov_reg_membase(REG_ITMP3, REG_SP, iptr->dst->regoff * 8);
				i386_mov_memindex_reg(OFFSET(java_longarray, data[0]) + 4, s1, s2, 3, REG_ITMP3);
				i386_mov_reg_membase(REG_ITMP3, REG_SP, iptr->dst->regoff * 8 + 4);
			}
			break;

		case ICMD_IALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			i386_mov_memindex_reg(OFFSET(java_intarray, data[0]), s1, s2, 2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_FTMP1);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			i386_flds_memindex(OFFSET(java_floatarray, data[0]), s1, s2, 2);
			fpu_st_offset++;
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
			i386_fldl_memindex(OFFSET(java_doublearray, data[0]), s1, s2, 3);
			fpu_st_offset++;
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_CALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			i386_movzwl_memindex_reg(OFFSET(java_chararray, data[0]), s1, s2, 1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;			

		case ICMD_SALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			i386_movswl_memindex_reg(OFFSET(java_shortarray, data[0]), s1, s2, 1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_BALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
   			i386_movsbl_memindex_reg(OFFSET(java_bytearray, data[0]), s1, s2, 0, d);
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
			i386_mov_reg_memindex(s3, OFFSET(java_objectarray, data[0]), s1, s2, 2);
			break;

		case ICMD_LASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}

			if (src->flags & INMEMORY) {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP3);
				i386_mov_reg_memindex(REG_ITMP3, OFFSET(java_longarray, data[0]), s1, s2, 3);
				i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP3);
				i386_mov_reg_memindex(REG_ITMP3, OFFSET(java_longarray, data[0]) + 4, s1, s2, 3);
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
			i386_mov_reg_memindex(s3, OFFSET(java_intarray, data[0]), s1, s2, 2);
			break;

		case ICMD_FASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_flt(s3, src, REG_FTMP1);
			i386_fstps_memindex(OFFSET(java_floatarray, data[0]), s1, s2, 2);
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
			i386_fstpl_memindex(OFFSET(java_doublearray, data[0]), s1, s2, 3);
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
			i386_movw_reg_memindex(s3, OFFSET(java_chararray, data[0]), s1, s2, 1);
			break;

		case ICMD_SASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			i386_movw_reg_memindex(s3, OFFSET(java_shortarray, data[0]), s1, s2, 1);
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
			i386_movb_reg_memindex(s3, OFFSET(java_bytearray, data[0]), s1, s2, 0);
			break;


		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */
		                      /* op1 = type, val.a = field address            */

			/* if class isn't yet initialized, do it */
  			if (!((fieldinfo *) iptr->val.a)->class->initialized) {
				/* call helper function which patches this code */
				i386_mov_imm_reg((s4) ((fieldinfo *) iptr->val.a)->class, REG_ITMP1);
				i386_mov_imm_reg((s4) asm_check_clinit, REG_ITMP2);
				i386_call_reg(REG_ITMP2);
  			}

			a = dseg_addaddress(&(((fieldinfo *) iptr->val.a)->value));
			/* here it's slightly slower */
			i386_mov_imm_reg(0, REG_ITMP2);
			dseg_adddata(mcodeptr);
			i386_mov_membase_reg(REG_ITMP2, a, REG_ITMP2);
			switch (iptr->op1) {
			case TYPE_INT:
			case TYPE_ADR:
				var_to_reg_int(s2, src, REG_ITMP1);
				i386_mov_reg_membase(s2, REG_ITMP2, 0);
				break;
			case TYPE_LNG:
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_ITMP2, 0);
					i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_ITMP2, 0 + 4);
				} else {
					panic("PUTSTATIC: longs have to be in memory");
				}
				break;
			case TYPE_FLT:
				var_to_reg_flt(s2, src, REG_FTMP1);
				i386_fstps_membase(REG_ITMP2, 0);
				fpu_st_offset--;
				break;
			case TYPE_DBL:
				var_to_reg_flt(s2, src, REG_FTMP1);
				i386_fstpl_membase(REG_ITMP2, 0);
				fpu_st_offset--;
				break;
			default: panic ("internal error");
			}
			break;

		case ICMD_GETSTATIC:  /* ...  ==> ..., value                          */
		                      /* op1 = type, val.a = field address            */

			/* if class isn't yet initialized, do it */
  			if (!((fieldinfo *) iptr->val.a)->class->initialized) {
				/* call helper function which patches this code */
				i386_mov_imm_reg((s4) ((fieldinfo *) iptr->val.a)->class, REG_ITMP1);
				i386_mov_imm_reg((s4) asm_check_clinit, REG_ITMP2);
				i386_call_reg(REG_ITMP2);
			}

			a = dseg_addaddress(&(((fieldinfo *) iptr->val.a)->value));
			i386_mov_imm_reg(0, REG_ITMP2);
			dseg_adddata(mcodeptr);
			i386_mov_membase_reg(REG_ITMP2, a, REG_ITMP2);
			switch (iptr->op1) {
			case TYPE_INT:
			case TYPE_ADR:
				d = reg_of_var(iptr->dst, REG_ITMP1);
				i386_mov_membase_reg(REG_ITMP2, 0, d);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_LNG:
				d = reg_of_var(iptr->dst, REG_NULL);
				if (iptr->dst->flags & INMEMORY) {
					i386_mov_membase_reg(REG_ITMP2, 0, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					i386_mov_membase_reg(REG_ITMP2, 0 + 4, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);
				} else {
					panic("GETSTATIC: longs have to be in memory");
				}
				break;
			case TYPE_FLT:
				d = reg_of_var(iptr->dst, REG_FTMP1);
				i386_flds_membase(REG_ITMP2, 0);
				fpu_st_offset++;
				store_reg_to_var_flt(iptr->dst, d);
				break;
			case TYPE_DBL:				
				d = reg_of_var(iptr->dst, REG_FTMP1);
				i386_fldl_membase(REG_ITMP2, 0);
				fpu_st_offset++;
				store_reg_to_var_flt(iptr->dst, d);
				break;
			default: panic ("internal error");
			}
			break;

		case ICMD_PUTFIELD:   /* ..., value  ==> ...                          */
		                      /* op1 = type, val.i = field offset             */

			a = ((fieldinfo *)(iptr->val.a))->offset;
			switch (iptr->op1) {
				case TYPE_INT:
				case TYPE_ADR:
					var_to_reg_int(s1, src->prev, REG_ITMP1);
					var_to_reg_int(s2, src, REG_ITMP2);
					gen_nullptr_check(s1);
					i386_mov_reg_membase(s2, s1, a);
					break;
				case TYPE_LNG:
					var_to_reg_int(s1, src->prev, REG_ITMP1);
					gen_nullptr_check(s1);
					if (src->flags & INMEMORY) {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP2);
						i386_mov_reg_membase(REG_ITMP2, s1, a);
						i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2);
						i386_mov_reg_membase(REG_ITMP2, s1, a + 4);
					} else {
						panic("PUTFIELD: longs have to be in memory");
					}
					break;
				case TYPE_FLT:
					var_to_reg_int(s1, src->prev, REG_ITMP1);
					var_to_reg_flt(s2, src, REG_FTMP1);
					gen_nullptr_check(s1);
					i386_fstps_membase(s1, a);
					fpu_st_offset--;
					break;
				case TYPE_DBL:
					var_to_reg_int(s1, src->prev, REG_ITMP1);
					var_to_reg_flt(s2, src, REG_FTMP1);
					gen_nullptr_check(s1);
					i386_fstpl_membase(s1, a);
					fpu_st_offset--;
					break;
				default: panic ("internal error");
				}
			break;

		case ICMD_GETFIELD:   /* ...  ==> ..., value                          */
		                      /* op1 = type, val.i = field offset             */

			a = ((fieldinfo *)(iptr->val.a))->offset;
			switch (iptr->op1) {
				case TYPE_INT:
				case TYPE_ADR:
					var_to_reg_int(s1, src, REG_ITMP1);
					d = reg_of_var(iptr->dst, REG_ITMP2);
					gen_nullptr_check(s1);
					i386_mov_membase_reg(s1, a, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
				case TYPE_LNG:
					var_to_reg_int(s1, src, REG_ITMP1);
					d = reg_of_var(iptr->dst, REG_NULL);
					gen_nullptr_check(s1);
					i386_mov_membase_reg(s1, a, REG_ITMP2);
					i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8);
					i386_mov_membase_reg(s1, a + 4, REG_ITMP2);
					i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
					break;
				case TYPE_FLT:
					var_to_reg_int(s1, src, REG_ITMP1);
					d = reg_of_var(iptr->dst, REG_FTMP1);
					gen_nullptr_check(s1);
					i386_flds_membase(s1, a);
					fpu_st_offset++;
   					store_reg_to_var_flt(iptr->dst, d);
					break;
				case TYPE_DBL:				
					var_to_reg_int(s1, src, REG_ITMP1);
					d = reg_of_var(iptr->dst, REG_FTMP1);
					gen_nullptr_check(s1);
					i386_fldl_membase(s1, a);
					fpu_st_offset++;
  					store_reg_to_var_flt(iptr->dst, d);
					break;
				default: panic ("internal error");
				}
			break;


		/* branch operations **************************************************/

			/* TWISTI */
/*  #define ALIGNCODENOP {if((int)((long)mcodeptr&7)){M_NOP;}} */
#define ALIGNCODENOP do {} while (0)

		case ICMD_ATHROW:       /* ..., objectref ==> ... (, objectref)       */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_INTMOVE(s1, REG_ITMP1_XPTR);

			i386_call_imm(0);                    /* passing exception pointer */
			i386_pop_reg(REG_ITMP2_XPC);

  			i386_mov_imm_reg((s4) asm_handle_exception, REG_ITMP3);
  			i386_jmp_reg(REG_ITMP3);
			ALIGNCODENOP;
			break;

		case ICMD_GOTO:         /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */

			i386_jmp_imm(0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
  			ALIGNCODENOP;
			break;

		case ICMD_JSR:          /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */

  			i386_call_imm(0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;
			
		case ICMD_RET:          /* ... ==> ...                                */
		                        /* op1 = local variable                       */

			var = &(locals[iptr->op1][TYPE_ADR]);
			var_to_reg_int(s1, var, REG_ITMP1);
			i386_jmp_reg(s1);
			break;

		case ICMD_IFNULL:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(I386_CMP, 0, REG_SP, src->regoff * 8);

			} else {
				i386_test_reg_reg(src->regoff, src->regoff);
			}
			i386_jcc(I386_CC_E, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFNONNULL:    /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(I386_CMP, 0, REG_SP, src->regoff * 8);

			} else {
				i386_test_reg_reg(src->regoff, src->regoff);
			}
			i386_jcc(I386_CC_NE, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFEQ:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(I386_CMP, iptr->val.i, REG_SP, src->regoff * 8);

			} else {
				i386_alu_imm_reg(I386_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(I386_CC_E, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFLT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(I386_CMP, iptr->val.i, REG_SP, src->regoff * 8);

			} else {
				i386_alu_imm_reg(I386_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(I386_CC_L, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFLE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(I386_CMP, iptr->val.i, REG_SP, src->regoff * 8);

			} else {
				i386_alu_imm_reg(I386_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(I386_CC_LE, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFNE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(I386_CMP, iptr->val.i, REG_SP, src->regoff * 8);

			} else {
				i386_alu_imm_reg(I386_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(I386_CC_NE, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFGT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(I386_CMP, iptr->val.i, REG_SP, src->regoff * 8);

			} else {
				i386_alu_imm_reg(I386_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(I386_CC_G, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFGE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(I386_CMP, iptr->val.i, REG_SP, src->regoff * 8);

			} else {
				i386_alu_imm_reg(I386_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(I386_CC_GE, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_LEQ:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			if (src->flags & INMEMORY) {
				if (iptr->val.l == 0) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					i386_alu_membase_reg(I386_OR, REG_SP, src->regoff * 8 + 4, REG_ITMP1);

				} else {
					i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2);
					i386_alu_imm_reg(I386_XOR, iptr->val.l >> 32, REG_ITMP2);
					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					i386_alu_imm_reg(I386_XOR, iptr->val.l, REG_ITMP1);
					i386_alu_reg_reg(I386_OR, REG_ITMP2, REG_ITMP1);
				}
			}
			i386_test_reg_reg(REG_ITMP1, REG_ITMP1);
			i386_jcc(I386_CC_E, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_LLT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(I386_CMP, iptr->val.l >> 32, REG_SP, src->regoff * 8 + 4);
				i386_jcc(I386_CC_L, 0);
				codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);

				a = 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				CALCIMMEDIATEBYTES(a, iptr->val.l);

				i386_jcc(I386_CC_G, a);

				i386_alu_imm_membase(I386_CMP, iptr->val.l, REG_SP, src->regoff * 8);
				i386_jcc(I386_CC_B, 0);
				codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			}			
			break;

		case ICMD_IF_LLE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(I386_CMP, iptr->val.l >> 32, REG_SP, src->regoff * 8 + 4);
				i386_jcc(I386_CC_L, 0);
				codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);

				a = 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				CALCIMMEDIATEBYTES(a, iptr->val.l);
				
				i386_jcc(I386_CC_G, a);

				i386_alu_imm_membase(I386_CMP, iptr->val.l, REG_SP, src->regoff * 8);
				i386_jcc(I386_CC_BE, 0);
				codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			}			
			break;

		case ICMD_IF_LNE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			if (src->flags & INMEMORY) {
				if (iptr->val.l == 0) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					i386_alu_membase_reg(I386_OR, REG_SP, src->regoff * 8 + 4, REG_ITMP1);

				} else {
					i386_mov_imm_reg(iptr->val.l, REG_ITMP1);
					i386_alu_membase_reg(I386_XOR, REG_SP, src->regoff * 8, REG_ITMP1);
					i386_mov_imm_reg(iptr->val.l >> 32, REG_ITMP2);
					i386_alu_membase_reg(I386_XOR, REG_SP, src->regoff * 8 + 4, REG_ITMP2);
					i386_alu_reg_reg(I386_OR, REG_ITMP2, REG_ITMP1);
				}
			}
			i386_test_reg_reg(REG_ITMP1, REG_ITMP1);
			i386_jcc(I386_CC_NE, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_LGT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(I386_CMP, iptr->val.l >> 32, REG_SP, src->regoff * 8 + 4);
				i386_jcc(I386_CC_G, 0);
				codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);

				a = 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				CALCIMMEDIATEBYTES(a, iptr->val.l);

				i386_jcc(I386_CC_L, a);

				i386_alu_imm_membase(I386_CMP, iptr->val.l, REG_SP, src->regoff * 8);
				i386_jcc(I386_CC_A, 0);
				codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			}			
			break;

		case ICMD_IF_LGE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(I386_CMP, iptr->val.l >> 32, REG_SP, src->regoff * 8 + 4);
				i386_jcc(I386_CC_G, 0);
				codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);

				a = 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);
				CALCIMMEDIATEBYTES(a, iptr->val.l);

				i386_jcc(I386_CC_L, a);

				i386_alu_imm_membase(I386_CMP, iptr->val.l, REG_SP, src->regoff * 8);
				i386_jcc(I386_CC_AE, 0);
				codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			}			
			break;

		case ICMD_IF_ICMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_ACMPEQ:    /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_reg_membase(I386_CMP, REG_ITMP1, REG_SP, src->prev->regoff * 8);

			} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
				i386_alu_membase_reg(I386_CMP, REG_SP, src->regoff * 8, src->prev->regoff);

			} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_alu_reg_membase(I386_CMP, src->regoff, REG_SP, src->prev->regoff * 8);

			} else {
				i386_alu_reg_reg(I386_CMP, src->regoff, src->prev->regoff);
			}
			i386_jcc(I386_CC_E, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_LCMPEQ:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2);
				i386_alu_membase_reg(I386_XOR, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_membase_reg(I386_XOR, REG_SP, src->regoff * 8 + 4, REG_ITMP2);
				i386_alu_reg_reg(I386_OR, REG_ITMP2, REG_ITMP1);
				i386_test_reg_reg(REG_ITMP1, REG_ITMP1);
			}			
			i386_jcc(I386_CC_E, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_ACMPNE:    /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_reg_membase(I386_CMP, REG_ITMP1, REG_SP, src->prev->regoff * 8);

			} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
				i386_alu_membase_reg(I386_CMP, REG_SP, src->regoff * 8, src->prev->regoff);

			} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_alu_reg_membase(I386_CMP, src->regoff, REG_SP, src->prev->regoff * 8);

			} else {
				i386_alu_reg_reg(I386_CMP, src->regoff, src->prev->regoff);
			}
			i386_jcc(I386_CC_NE, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_LCMPNE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2);
				i386_alu_membase_reg(I386_XOR, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_membase_reg(I386_XOR, REG_SP, src->regoff * 8 + 4, REG_ITMP2);
				i386_alu_reg_reg(I386_OR, REG_ITMP2, REG_ITMP1);
				i386_test_reg_reg(REG_ITMP1, REG_ITMP1);
			}			
			i386_jcc(I386_CC_NE, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPLT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_reg_membase(I386_CMP, REG_ITMP1, REG_SP, src->prev->regoff * 8);

			} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
				i386_alu_membase_reg(I386_CMP, REG_SP, src->regoff * 8, src->prev->regoff);

			} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_alu_reg_membase(I386_CMP, src->regoff, REG_SP, src->prev->regoff * 8);

			} else {
				i386_alu_reg_reg(I386_CMP, src->regoff, src->prev->regoff);
			}
			i386_jcc(I386_CC_L, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_LCMPLT:    /* ..., value, value ==> ...                  */
	                            /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP1);
				i386_alu_membase_reg(I386_CMP, REG_SP, src->regoff * 8 + 4, REG_ITMP1);
				i386_jcc(I386_CC_L, 0);
				codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);

				a = 3 + 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->prev->regoff * 8);
				CALCOFFSETBYTES(a, REG_SP, src->regoff);

				i386_jcc(I386_CC_G, a);

				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_alu_membase_reg(I386_CMP, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_jcc(I386_CC_B, 0);
				codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			}			
			break;

		case ICMD_IF_ICMPGT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_reg_membase(I386_CMP, REG_ITMP1, REG_SP, src->prev->regoff * 8);

			} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
				i386_alu_membase_reg(I386_CMP, REG_SP, src->regoff * 8, src->prev->regoff);

			} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_alu_reg_membase(I386_CMP, src->regoff, REG_SP, src->prev->regoff * 8);

			} else {
				i386_alu_reg_reg(I386_CMP, src->regoff, src->prev->regoff);
			}
			i386_jcc(I386_CC_G, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_LCMPGT:    /* ..., value, value ==> ...                  */
                                /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP1);
				i386_alu_membase_reg(I386_CMP, REG_SP, src->regoff * 8 + 4, REG_ITMP1);
				i386_jcc(I386_CC_G, 0);
				codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);

				a = 3 + 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->prev->regoff * 8);
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);

				i386_jcc(I386_CC_L, a);

				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_alu_membase_reg(I386_CMP, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_jcc(I386_CC_A, 0);
				codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			}			
			break;

		case ICMD_IF_ICMPLE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_reg_membase(I386_CMP, REG_ITMP1, REG_SP, src->prev->regoff * 8);

			} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
				i386_alu_membase_reg(I386_CMP, REG_SP, src->regoff * 8, src->prev->regoff);

			} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_alu_reg_membase(I386_CMP, src->regoff, REG_SP, src->prev->regoff * 8);

			} else {
				i386_alu_reg_reg(I386_CMP, src->regoff, src->prev->regoff);
			}
			i386_jcc(I386_CC_LE, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_LCMPLE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP1);
				i386_alu_membase_reg(I386_CMP, REG_SP, src->regoff * 8 + 4, REG_ITMP1);
				i386_jcc(I386_CC_L, 0);
				codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);

				a = 3 + 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->prev->regoff * 8);
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);

				i386_jcc(I386_CC_G, a);

				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_alu_membase_reg(I386_CMP, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_jcc(I386_CC_BE, 0);
				codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			}			
			break;

		case ICMD_IF_ICMPGE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_reg_membase(I386_CMP, REG_ITMP1, REG_SP, src->prev->regoff * 8);

			} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
				i386_alu_membase_reg(I386_CMP, REG_SP, src->regoff * 8, src->prev->regoff);

			} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_alu_reg_membase(I386_CMP, src->regoff, REG_SP, src->prev->regoff * 8);

			} else {
				i386_alu_reg_reg(I386_CMP, src->regoff, src->prev->regoff);
			}
			i386_jcc(I386_CC_GE, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_LCMPGE:    /* ..., value, value ==> ...                  */
	                            /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP1);
				i386_alu_membase_reg(I386_CMP, REG_SP, src->regoff * 8 + 4, REG_ITMP1);
				i386_jcc(I386_CC_G, 0);
				codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);

				a = 3 + 3 + 6;
				CALCOFFSETBYTES(a, REG_SP, src->prev->regoff * 8);
				CALCOFFSETBYTES(a, REG_SP, src->regoff * 8);

				i386_jcc(I386_CC_L, a);

				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_alu_membase_reg(I386_CMP, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_jcc(I386_CC_AE, 0);
				codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			}			
			break;

		/* (value xx 0) ? IFxx_ICONST : ELSE_ICONST                           */

		case ICMD_ELSE_ICONST:  /* handled by IFxx_ICONST                     */
			break;

		case ICMD_IFEQ_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_ifcc_iconst(I386_CC_NE, src, iptr);
			break;

		case ICMD_IFNE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_ifcc_iconst(I386_CC_E, src, iptr);
			break;

		case ICMD_IFLT_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_ifcc_iconst(I386_CC_GE, src, iptr);
			break;

		case ICMD_IFGE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_ifcc_iconst(I386_CC_L, src, iptr);
			break;

		case ICMD_IFGT_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_ifcc_iconst(I386_CC_LE, src, iptr);
			break;

		case ICMD_IFLE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			d = reg_of_var(iptr->dst, REG_NULL);
			i386_emit_ifcc_iconst(I386_CC_G, src, iptr);
			break;


		case ICMD_IRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_ARETURN:

			var_to_reg_int(s1, src, REG_RESULT);
			M_INTMOVE(s1, REG_RESULT);

#ifdef USE_THREADS
			if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
				i386_mov_membase_reg(REG_SP, maxmemuse * 8, REG_ITMP2);
				i386_mov_reg_membase(REG_RESULT, REG_SP, maxmemuse * 8);

				i386_alu_imm_reg(I386_SUB, 1 * 8, REG_SP);
				i386_mov_reg_membase(REG_ITMP2, REG_SP, 0 * 8);
				i386_mov_imm_reg((s4) asm_builtin_monitorexit, REG_ITMP1);
				i386_call_reg(REG_ITMP1);
				i386_alu_imm_reg(I386_ADD, 1 * 8, REG_SP);

				i386_mov_membase_reg(REG_SP, maxmemuse * 8, REG_RESULT);
			}
#endif

			goto nowperformreturn;

		case ICMD_LRETURN:      /* ..., retvalue ==> ...                      */

			if (src->flags & INMEMORY) {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_RESULT);
				i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_RESULT2);

			} else {
				panic("LRETURN: longs have to be in memory");
			}

#ifdef USE_THREADS
			if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
				i386_mov_membase_reg(REG_SP, maxmemuse * 8, REG_ITMP2);
				i386_mov_reg_membase(REG_RESULT, REG_SP, maxmemuse * 8);
				i386_mov_reg_membase(REG_RESULT2, REG_SP, maxmemuse * 8 + 4);

				i386_alu_imm_reg(I386_SUB, 1 * 8, REG_SP);
				i386_mov_reg_membase(REG_ITMP2, REG_SP, 0 * 8);
				i386_mov_imm_reg((s4) builtin_monitorexit, REG_ITMP1);
				i386_call_reg(REG_ITMP1);
				i386_alu_imm_reg(I386_ADD, 1 * 8, REG_SP);

				i386_mov_membase_reg(REG_SP, maxmemuse * 8, REG_RESULT);
				i386_mov_membase_reg(REG_SP, maxmemuse * 8 + 4, REG_RESULT2);
			}
#endif

			goto nowperformreturn;

		case ICMD_FRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_DRETURN:      /* ..., retvalue ==> ...                      */

			var_to_reg_flt(s1, src, REG_FRESULT);
			/* this may be an early return -- keep the offset correct for the remaining code */
			fpu_st_offset--;

#ifdef USE_THREADS
			if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
				i386_mov_membase_reg(REG_SP, maxmemuse * 8, REG_ITMP2);
				i386_fstl_membase(REG_SP, maxmemuse * 8);

				i386_alu_imm_reg(I386_SUB, 1 * 8, REG_SP);
				i386_mov_reg_membase(REG_ITMP2, REG_SP, 0 * 8);
				i386_mov_imm_reg((s4) builtin_monitorexit, REG_ITMP1);
				i386_call_reg(REG_ITMP1);
				i386_alu_imm_reg(I386_ADD, 1 * 8, REG_SP);

				i386_fldl_membase(REG_SP, maxmemuse * 8);
			}
#endif

			goto nowperformreturn;

		case ICMD_RETURN:      /* ...  ==> ...                                */

#ifdef USE_THREADS
			if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
				i386_mov_membase_reg(REG_SP, 8 * maxmemuse, REG_ITMP1);
				i386_alu_imm_reg(I386_SUB, 4, REG_SP);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, 0);
				i386_mov_imm_reg((s4) builtin_monitorexit, REG_ITMP1);
				i386_call_reg(REG_ITMP1);
				i386_alu_imm_reg(I386_ADD, 4, REG_SP);
			}
#endif

nowperformreturn:
			{
			int r, p;
			
  			p = parentargs_base;
			
			/* restore saved registers                                        */
			for (r = savintregcnt - 1; r >= maxsavintreguse; r--) {
				p--;
				i386_mov_membase_reg(REG_SP, p * 8, savintregs[r]);
			}
			for (r = savfltregcnt - 1; r >= maxsavfltreguse; r--) {
  				p--;
				i386_fldl_membase(REG_SP, p * 8);
				fpu_st_offset++;
				if (iptr->opc == ICMD_FRETURN || iptr->opc == ICMD_DRETURN) {
					i386_fstp_reg(savfltregs[r] + fpu_st_offset + 1);
				} else {
					i386_fstp_reg(savfltregs[r] + fpu_st_offset);
				}
				fpu_st_offset--;
			}

			/* deallocate stack                                               */
			if (parentargs_base) {
				i386_alu_imm_reg(I386_ADD, parentargs_base * 8, REG_SP);
			}

			/* call trace function */
			if (runverbose) {
				i386_alu_imm_reg(I386_SUB, 4 + 8 + 8 + 4, REG_SP);

				i386_mov_imm_membase((s4) method, REG_SP, 0);

				i386_mov_reg_membase(REG_RESULT, REG_SP, 4);
				i386_mov_reg_membase(REG_RESULT2, REG_SP, 4 + 4);
				
				i386_fstl_membase(REG_SP, 4 + 8);
				i386_fsts_membase(REG_SP, 4 + 8 + 8);

  				i386_mov_imm_reg((s4) builtin_displaymethodstop, REG_ITMP1);
/*  				i386_mov_imm_reg(asm_builtin_exittrace, REG_ITMP1); */
				i386_call_reg(REG_ITMP1);

				i386_mov_membase_reg(REG_SP, 4, REG_RESULT);
				i386_mov_membase_reg(REG_SP, 4 + 4, REG_RESULT2);

				i386_alu_imm_reg(I386_ADD, 4 + 8 + 8 + 4, REG_SP);
			}

			i386_ret();
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
					i386_alu_imm_reg(I386_SUB, l, REG_ITMP1);
				}
				i = i - l + 1;

                /* range check */

				i386_alu_imm_reg(I386_CMP, i - 1, REG_ITMP1);
				i386_jcc(I386_CC_A, 0);

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

				i386_mov_imm_reg(0, REG_ITMP2);
				dseg_adddata(mcodeptr);
				i386_mov_memindex_reg(-dseglen, REG_ITMP2, REG_ITMP1, 2, REG_ITMP1);
				i386_jmp_reg(REG_ITMP1);
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
					i386_alu_imm_reg(I386_CMP, val, s1);
					i386_jcc(I386_CC_E, 0);
					/* codegen_addreference(BlockPtrOfPC(s4ptr[1]), mcodeptr); */
					codegen_addreference((basicblock *) tptr[0], mcodeptr); 
				}

				i386_jmp_imm(0);
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

			MCODECHECK((s3 << 1) + 64);

			if ( (iptr->opc == ICMD_BUILTIN1) ||
				(iptr->opc == ICMD_BUILTIN2) ||
				(iptr->opc == ICMD_BUILTIN3) ) {
#if 0
				i386_push_reg(REG_ITMP1);
				i386_push_reg(REG_ITMP2);
				i386_push_reg(REG_ITMP3);

			        i386_mov_imm_reg((s4) builtin_asm_new_stackframeinfo, REG_ITMP1);
			        i386_call_reg(REG_ITMP1);

				i386_pop_reg(REG_ITMP3);
				i386_pop_reg(REG_ITMP2);
				i386_pop_reg(REG_ITMP1);

#if 0
			        i386_mov_membase_reg(REG_SP, 0 , REG_ITMP2); /*save return adress*/
			        i386_mov_membase_reg(REG_RESULT, 0 , REG_ITMP3); /*get direct access to structure*/

			        i386_mov_imm_membase(0x1111, REG_ITMP3, offreturnfromnative); /*store return adress in stack frame info block*/
			        i386_mov_imm_membase((s4) m, REG_ITMP3, offmethodnative); /*store methodpointer in stack frame info block*/
			        i386_mov_imm_membase(1111,REG_ITMP3,offaddrreturnfromnative);
#endif
#endif
			}
			/* copy arguments to registers or stack location                  */

			for (; --s3 >= 0; src = src->prev) {
				if (src->varkind == ARGVAR) {
					continue;
				}

				if (IS_INT_LNG_TYPE(src->type)) {
					if (s3 < intreg_argnum) {
						panic("No integer argument registers available!");

					} else {
						if (!IS_2_WORD_TYPE(src->type)) {
							if (src->flags & INMEMORY) {
								i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
								i386_mov_reg_membase(REG_ITMP1, REG_SP, s3 * 8);

							} else {
								i386_mov_reg_membase(src->regoff, REG_SP, s3 * 8);
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
					if (s3 < fltreg_argnum) {
						panic("No float argument registers available!");

					} else {
						var_to_reg_flt(d, src, REG_FTMP1);
						if (src->type == TYPE_FLT) {
							i386_fstps_membase(REG_SP, s3 * 8);

						} else {
							i386_fstpl_membase(REG_SP, s3 * 8);
						}
					}
				}
			} /* end of for */

			m = iptr->val.a;
			switch (iptr->opc) {
				case ICMD_BUILTIN3:
				case ICMD_BUILTIN2:
				case ICMD_BUILTIN1:

					a = (u4) m;
					d = iptr->op1;

					i386_mov_imm_reg(a, REG_ITMP1);
					i386_call_reg(REG_ITMP1);
					break;

				case ICMD_INVOKESTATIC:

					a = (u4) m->stubroutine;
					d = m->returntype;

					i386_mov_imm_reg(a, REG_ITMP2);
					i386_call_reg(REG_ITMP2);
					break;

				case ICMD_INVOKESPECIAL:

					a = (u4) m->stubroutine;
					d = m->returntype;

					i386_mov_membase_reg(REG_SP, 0, REG_ITMP1);
					gen_nullptr_check(REG_ITMP1);
					i386_mov_membase_reg(REG_ITMP1, 0, REG_ITMP1);    /* access memory for hardware nullptr */

					i386_mov_imm_reg(a, REG_ITMP2);
					i386_call_reg(REG_ITMP2);
					break;

				case ICMD_INVOKEVIRTUAL:

					d = m->returntype;

					i386_mov_membase_reg(REG_SP, 0, REG_ITMP1);
					gen_nullptr_check(REG_ITMP1);
					i386_mov_membase_reg(REG_ITMP1, OFFSET(java_objectheader, vftbl), REG_ITMP2);
					i386_mov_membase32_reg(REG_ITMP2, OFFSET(vftbl, table[0]) + sizeof(methodptr) * m->vftblindex, REG_ITMP1);

					i386_call_reg(REG_ITMP1);
					break;

				case ICMD_INVOKEINTERFACE:

					ci = m->class;
					d = m->returntype;

					i386_mov_membase_reg(REG_SP, 0, REG_ITMP1);
					gen_nullptr_check(REG_ITMP1);
					i386_mov_membase_reg(REG_ITMP1, OFFSET(java_objectheader, vftbl), REG_ITMP1);
					i386_mov_membase_reg(REG_ITMP1, OFFSET(vftbl, interfacetable[0]) - sizeof(methodptr) * ci->index, REG_ITMP2);
					i386_mov_membase32_reg(REG_ITMP2, sizeof(methodptr) * (m - ci->methods), REG_ITMP1);

					i386_call_reg(REG_ITMP1);
					break;

				default:
					d = 0;
					error("Unkown ICMD-Command: %d", iptr->opc);
				}

			/* d contains return type */

			if (d != TYPE_VOID) {
				d = reg_of_var(iptr->dst, REG_NULL);

				if (IS_INT_LNG_TYPE(iptr->dst->type)) {
					if (IS_2_WORD_TYPE(iptr->dst->type)) {
						if (iptr->dst->flags & INMEMORY) {
							i386_mov_reg_membase(REG_RESULT, REG_SP, iptr->dst->regoff * 8);
							i386_mov_reg_membase(REG_RESULT2, REG_SP, iptr->dst->regoff * 8 + 4);

						} else {
  							panic("RETURN: longs have to be in memory");
						}

					} else {
						if (iptr->dst->flags & INMEMORY) {
							i386_mov_reg_membase(REG_RESULT, REG_SP, iptr->dst->regoff * 8);

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
			codegen_threadcritrestart(mcodeptr - mcodebase);
#endif
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			if (s1 == d) {
				M_INTMOVE(s1, REG_ITMP1);
				s1 = REG_ITMP1;
			}
			i386_alu_reg_reg(I386_XOR, d, d);
			if (iptr->op1) {                               /* class/interface */
				if (super->flags & ACC_INTERFACE) {        /* interface       */
					i386_test_reg_reg(s1, s1);

					/* TODO: clean up this calculation */
					a = 2;
					CALCOFFSETBYTES(a, s1, OFFSET(java_objectheader, vftbl));

					a += 2;
					CALCOFFSETBYTES(a, REG_ITMP1, OFFSET(vftbl, interfacetablelength));
					
					a += 2;
/*  					CALCOFFSETBYTES(a, super->index); */
					CALCIMMEDIATEBYTES(a, super->index);
					
					a += 3;
					a += 6;

					a += 2;
					CALCOFFSETBYTES(a, REG_ITMP1, OFFSET(vftbl, interfacetable[0]) - super->index * sizeof(methodptr*));

					a += 3;

					a += 6;    /* jcc */
					a += 5;

					i386_jcc(I386_CC_E, a);

					i386_mov_membase_reg(s1, OFFSET(java_objectheader, vftbl), REG_ITMP1);
					i386_mov_membase_reg(REG_ITMP1, OFFSET(vftbl, interfacetablelength), REG_ITMP2);
					i386_alu_imm_reg(I386_SUB, super->index, REG_ITMP2);
					/* TODO: test */
					i386_alu_imm_reg(I386_CMP, 0, REG_ITMP2);

					/* TODO: clean up this calculation */
					a = 0;
					a += 2;
					CALCOFFSETBYTES(a, REG_ITMP1, OFFSET(vftbl, interfacetable[0]) - super->index * sizeof(methodptr*));

					a += 3;

					a += 6;    /* jcc */
					a += 5;

					i386_jcc(I386_CC_LE, a);
					i386_mov_membase_reg(REG_ITMP1, OFFSET(vftbl, interfacetable[0]) - super->index * sizeof(methodptr*), REG_ITMP1);
					/* TODO: test */
					i386_alu_imm_reg(I386_CMP, 0, REG_ITMP1);
/*  					i386_setcc_reg(I386_CC_A, d); */
/*  					i386_jcc(I386_CC_BE, 5); */
					i386_jcc(I386_CC_E, 5);
					i386_mov_imm_reg(1, d);
					

				} else {                                   /* class           */
					i386_test_reg_reg(s1, s1);

					/* TODO: clean up this calculation */
					a = 2;
					CALCOFFSETBYTES(a, s1, OFFSET(java_objectheader, vftbl));
					a += 5;
					a += 2;
					CALCOFFSETBYTES(a, REG_ITMP1, OFFSET(vftbl, baseval));
					a += 2;
					CALCOFFSETBYTES(a, REG_ITMP2, OFFSET(vftbl, baseval));
					
					a += 2;
					CALCOFFSETBYTES(a, REG_ITMP2, OFFSET(vftbl, diffval));
					
					a += 2;
					a += 2;    /* xor */

					a += 2;

					a += 6;    /* jcc */
					a += 5;

					i386_jcc(I386_CC_E, a);

					i386_mov_membase_reg(s1, OFFSET(java_objectheader, vftbl), REG_ITMP1);
					i386_mov_imm_reg((s4) super->vftbl, REG_ITMP2);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
					codegen_threadcritstart(mcodeptr - mcodebase);
#endif
					i386_mov_membase_reg(REG_ITMP1, OFFSET(vftbl, baseval), REG_ITMP1);
					i386_mov_membase_reg(REG_ITMP2, OFFSET(vftbl, baseval), REG_ITMP3);
					i386_mov_membase_reg(REG_ITMP2, OFFSET(vftbl, diffval), REG_ITMP2);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
					codegen_threadcritstop(mcodeptr - mcodebase);
#endif
					i386_alu_reg_reg(I386_SUB, REG_ITMP3, REG_ITMP1);
					i386_alu_reg_reg(I386_XOR, d, d);

					i386_alu_reg_reg(I386_CMP, REG_ITMP2, REG_ITMP1);
					i386_jcc(I386_CC_A, 5);
					i386_mov_imm_reg(1, d);
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
			codegen_threadcritrestart(mcodeptr - mcodebase);
#endif
			d = reg_of_var(iptr->dst, REG_ITMP3);
			var_to_reg_int(s1, src, d);
			if (iptr->op1) {                               /* class/interface */
				if (super->flags & ACC_INTERFACE) {        /* interface       */
					i386_test_reg_reg(s1, s1);

					/* TODO: clean up this calculation */
					a = 2;
					CALCOFFSETBYTES(a, s1, OFFSET(java_objectheader, vftbl));

					a += 2;
					CALCOFFSETBYTES(a, REG_ITMP1, OFFSET(vftbl, interfacetablelength));

					a += 2;
/*  					CALCOFFSETBYTES(a, super->index); */
					CALCIMMEDIATEBYTES(a, super->index);

					a += 3;
					a += 6;

					a += 2;
					CALCOFFSETBYTES(a, REG_ITMP1, OFFSET(vftbl, interfacetable[0]) - super->index * sizeof(methodptr*));

					a += 3;
					a += 6;

					i386_jcc(I386_CC_E, a);

					i386_mov_membase_reg(s1, OFFSET(java_objectheader, vftbl), REG_ITMP1);
					i386_mov_membase_reg(REG_ITMP1, OFFSET(vftbl, interfacetablelength), REG_ITMP2);
					i386_alu_imm_reg(I386_SUB, super->index, REG_ITMP2);
					/* TODO: test */
					i386_alu_imm_reg(I386_CMP, 0, REG_ITMP2);
					i386_jcc(I386_CC_LE, 0);
					codegen_addxcastrefs(mcodeptr);
					i386_mov_membase_reg(REG_ITMP1, OFFSET(vftbl, interfacetable[0]) - super->index * sizeof(methodptr*), REG_ITMP2);
					/* TODO: test */
					i386_alu_imm_reg(I386_CMP, 0, REG_ITMP2);
					i386_jcc(I386_CC_E, 0);
					codegen_addxcastrefs(mcodeptr);

				} else {                                     /* class           */
					i386_test_reg_reg(s1, s1);

					/* TODO: clean up this calculation */
					a = 2;
					CALCOFFSETBYTES(a, s1, OFFSET(java_objectheader, vftbl));

					a += 5;

					a += 2;
					CALCOFFSETBYTES(a, REG_ITMP1, OFFSET(vftbl, baseval));

					if (d != REG_ITMP3) {
						a += 2;
						CALCOFFSETBYTES(a, REG_ITMP2, OFFSET(vftbl, baseval));
						
						a += 2;
						CALCOFFSETBYTES(a, REG_ITMP2, OFFSET(vftbl, diffval));

						a += 2;
						
					} else {
						a += 2;
						CALCOFFSETBYTES(a, REG_ITMP2, OFFSET(vftbl, baseval));

						a += 2;

						a += 5;

						a += 2;
						CALCOFFSETBYTES(a, REG_ITMP2, OFFSET(vftbl, diffval));
					}

					a += 2;

					a += 6;

					i386_jcc(I386_CC_E, a);

					i386_mov_membase_reg(s1, OFFSET(java_objectheader, vftbl), REG_ITMP1);
					i386_mov_imm_reg((s4) super->vftbl, REG_ITMP2);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
					codegen_threadcritstart(mcodeptr - mcodebase);
#endif
					i386_mov_membase_reg(REG_ITMP1, OFFSET(vftbl, baseval), REG_ITMP1);
					if (d != REG_ITMP3) {
						i386_mov_membase_reg(REG_ITMP2, OFFSET(vftbl, baseval), REG_ITMP3);
						i386_mov_membase_reg(REG_ITMP2, OFFSET(vftbl, diffval), REG_ITMP2);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
						codegen_threadcritstop(mcodeptr - mcodebase);
#endif
						i386_alu_reg_reg(I386_SUB, REG_ITMP3, REG_ITMP1);

					} else {
						i386_mov_membase_reg(REG_ITMP2, OFFSET(vftbl, baseval), REG_ITMP2);
						i386_alu_reg_reg(I386_SUB, REG_ITMP2, REG_ITMP1);
						i386_mov_imm_reg((s4) super->vftbl, REG_ITMP2);
						i386_mov_membase_reg(REG_ITMP2, OFFSET(vftbl, diffval), REG_ITMP2);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
						codegen_threadcritstop(mcodeptr - mcodebase);
#endif
					}

					i386_alu_reg_reg(I386_CMP, REG_ITMP2, REG_ITMP1);
					i386_jcc(I386_CC_A, 0);    /* (u) REG_ITMP1 > (u) REG_ITMP2 -> jump */
					codegen_addxcastrefs(mcodeptr);
				}

			} else
				panic ("internal error: no inlined array checkcast");
			}
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_CHECKASIZE:  /* ..., size ==> ..., size                     */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(I386_CMP, 0, REG_SP, src->regoff * 8);
				
			} else {
				i386_test_reg_reg(src->regoff, src->regoff);
			}
			i386_jcc(I386_CC_L, 0);
			codegen_addxcheckarefs(mcodeptr);
			break;

		case ICMD_CHECKOOM:  /* ... ==> ...                                   */

			i386_test_reg_reg(REG_RESULT, REG_RESULT);
			i386_jcc(I386_CC_E, 0);
			codegen_addxoomrefs(mcodeptr);
			break;

		case ICMD_MULTIANEWARRAY:/* ..., cnt1, [cnt2, ...] ==> ..., arrayref  */
		                      /* op1 = dimension, val.a = array descriptor    */

			/* check for negative sizes and copy sizes to stack if necessary  */

  			MCODECHECK((iptr->op1 << 1) + 64);

			for (s1 = iptr->op1; --s1 >= 0; src = src->prev) {
				if (src->flags & INMEMORY) {
					i386_alu_imm_membase(I386_CMP, 0, REG_SP, src->regoff * 8);

				} else {
					i386_test_reg_reg(src->regoff, src->regoff);
				}
				i386_jcc(I386_CC_L, 0);
				codegen_addxcheckarefs(mcodeptr);

				/* 
				 * copy sizes to new stack location, be cause native function
				 * builtin_nmultianewarray access them as (int *)
				 */
				i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, -(iptr->op1 - s1) * 4);

				/* copy sizes to stack (argument numbers >= INT_ARG_CNT)      */

				if (src->varkind != ARGVAR) {
					if (src->flags & INMEMORY) {
						i386_mov_membase_reg(REG_SP, (src->regoff + intreg_argnum) * 8, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, (s1 + intreg_argnum) * 8);

					} else {
						i386_mov_reg_membase(src->regoff, REG_SP, (s1 + intreg_argnum) * 8);
					}
				}
			}
			i386_alu_imm_reg(I386_SUB, iptr->op1 * 4, REG_SP);

			/* a0 = dimension count */

			/* save stack pointer */
			M_INTMOVE(REG_SP, REG_ITMP1);

			i386_alu_imm_reg(I386_SUB, 12, REG_SP);
			i386_mov_imm_membase(iptr->op1, REG_SP, 0);

			/* a1 = arraydescriptor */

			i386_mov_imm_membase((s4) iptr->val.a, REG_SP, 4);

			/* a2 = pointer to dimensions = stack pointer */

			i386_mov_reg_membase(REG_ITMP1, REG_SP, 8);

			i386_mov_imm_reg((s4) (builtin_nmultianewarray), REG_ITMP1);
			i386_call_reg(REG_ITMP1);
			i386_alu_imm_reg(I386_ADD, 12 + iptr->op1 * 4, REG_SP);

			s1 = reg_of_var(iptr->dst, REG_RESULT);
			M_INTMOVE(REG_RESULT, s1);
			store_reg_to_var_int(iptr->dst, s1);
			break;

		default:
			error ("Unknown pseudo command: %d", iptr->opc);
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
					M_FLTMOVE(s1,interfaces[len][s2].regoff);

				} else {
					panic("double store");
/*  					M_DST(s1, REG_SP, 8 * interfaces[len][s2].regoff); */
				}

			} else {
				var_to_reg_int(s1, src, REG_ITMP1);
				if (!IS_2_WORD_TYPE(interfaces[len][s2].type)) {
					if (!(interfaces[len][s2].flags & INMEMORY)) {
						M_INTMOVE(s1, interfaces[len][s2].regoff);

					} else {
						i386_mov_reg_membase(s1, REG_SP, interfaces[len][s2].regoff * 8);
					}

				} else {
					if (interfaces[len][s2].flags & INMEMORY) {
						M_LNGMEMMOVE(s1, interfaces[len][s2].regoff);

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

	codegen_createlinenumbertable();

	/* bptr -> mpc = (int)((u1*) mcodeptr - mcodebase); */

	{

	/* generate bound check stubs */
	u1 *xcodeptr = NULL;
	
	for (; xboundrefs != NULL; xboundrefs = xboundrefs->next) {
		if ((exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1*) mcodebase + xboundrefs->branchpos, 
							  xboundrefs->branchpos,
							  (u1*) xcodeptr - (u1*) mcodebase - (2 + 5 + 5 + 2));
			continue;
		}


		gen_resolvebranch((u1*) mcodebase + xboundrefs->branchpos, 
		                  xboundrefs->branchpos, (u1*) mcodeptr - mcodebase);

		MCODECHECK(8);

		/* move index register into REG_ITMP1 */
		i386_mov_reg_reg(xboundrefs->reg, REG_ITMP1);              /* 2 bytes */

		i386_mov_imm_reg(0, REG_ITMP2_XPC);                        /* 5 bytes */
		dseg_adddata(mcodeptr);
		i386_mov_imm_reg(xboundrefs->branchpos - 6, REG_ITMP3);    /* 5 bytes */
		i386_alu_reg_reg(I386_ADD, REG_ITMP3, REG_ITMP2_XPC);      /* 2 bytes */

		if (xcodeptr != NULL) {
			i386_jmp_imm(((u1 *) xcodeptr - (u1 *) mcodeptr) - 5);

		} else {
			xcodeptr = mcodeptr;

			i386_push_reg(REG_ITMP2_XPC);

			PREPARE_NATIVE_STACKINFO	

			i386_alu_imm_reg(I386_SUB, 2 * 4, REG_SP);
			i386_mov_imm_membase((s4) string_java_lang_ArrayIndexOutOfBoundsException, REG_SP, 0 * 4);
			i386_mov_reg_membase(REG_ITMP1, REG_SP, 1 * 4);
			i386_mov_imm_reg((s4) new_exception_int, REG_ITMP1);
			i386_call_reg(REG_ITMP1);    /* return value is REG_ITMP1_XPTR */
			i386_alu_imm_reg(I386_ADD, 2 * 4, REG_SP);

			REMOVE_NATIVE_STACKINFO

			i386_pop_reg(REG_ITMP2_XPC);

			i386_mov_imm_reg((s4) asm_handle_exception, REG_ITMP3);
			i386_jmp_reg(REG_ITMP3);
		}
	}

	/* generate negative array size check stubs */
	xcodeptr = NULL;
	
	for (; xcheckarefs != NULL; xcheckarefs = xcheckarefs->next) {
		if ((exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1*) mcodebase + xcheckarefs->branchpos, 
							  xcheckarefs->branchpos,
							  (u1*) xcodeptr - (u1*) mcodebase - (5 + 5 + 2));
			continue;
		}

		gen_resolvebranch((u1*) mcodebase + xcheckarefs->branchpos, 
		                  xcheckarefs->branchpos, (u1*) mcodeptr - mcodebase);

		MCODECHECK(8);

		i386_mov_imm_reg(0, REG_ITMP2_XPC);                         /* 5 bytes */
		dseg_adddata(mcodeptr);
		i386_mov_imm_reg(xcheckarefs->branchpos - 6, REG_ITMP1);    /* 5 bytes */
		i386_alu_reg_reg(I386_ADD, REG_ITMP1, REG_ITMP2_XPC);       /* 2 bytes */

		if (xcodeptr != NULL) {
			i386_jmp_imm(((u1 *) xcodeptr - (u1 *) mcodeptr) - 5);

		} else {
			xcodeptr = mcodeptr;

			i386_push_reg(REG_ITMP2_XPC);

			PREPARE_NATIVE_STACKINFO

			i386_alu_imm_reg(I386_SUB, 1 * 4, REG_SP);
			i386_mov_imm_membase((s4) string_java_lang_NegativeArraySizeException, REG_SP, 0 * 4);
			i386_mov_imm_reg((s4) new_exception, REG_ITMP1);
			i386_call_reg(REG_ITMP1);    /* return value is REG_ITMP1_XPTR */
			i386_alu_imm_reg(I386_ADD, 1 * 4, REG_SP);


			REMOVE_NATIVE_STACKINFO

			i386_pop_reg(REG_ITMP2_XPC);

			i386_mov_imm_reg((s4) asm_handle_exception, REG_ITMP3);
			i386_jmp_reg(REG_ITMP3);
		}
	}

	/* generate cast check stubs */
	xcodeptr = NULL;
	
	for (; xcastrefs != NULL; xcastrefs = xcastrefs->next) {
		if ((exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1*) mcodebase + xcastrefs->branchpos, 
							  xcastrefs->branchpos,
							  (u1*) xcodeptr - (u1*) mcodebase - (5 + 5 + 2));
			continue;
		}

		gen_resolvebranch((u1*) mcodebase + xcastrefs->branchpos, 
		                  xcastrefs->branchpos, (u1*) mcodeptr - mcodebase);

		MCODECHECK(8);

		i386_mov_imm_reg(0, REG_ITMP2_XPC);    /* 5 bytes */
		dseg_adddata(mcodeptr);
		i386_mov_imm_reg(xcastrefs->branchpos - 6, REG_ITMP1);    /* 5 bytes */
		i386_alu_reg_reg(I386_ADD, REG_ITMP1, REG_ITMP2_XPC);    /* 2 bytes */

		if (xcodeptr != NULL) {
			i386_jmp_imm(((u1 *) xcodeptr - (u1 *) mcodeptr) - 5);
		
		} else {
			xcodeptr = mcodeptr;

			i386_push_reg(REG_ITMP2_XPC);

			i386_alu_imm_reg(I386_SUB, 1 * 4, REG_SP);
			i386_mov_imm_membase((s4) string_java_lang_ClassCastException, REG_SP, 0 * 4);
			i386_mov_imm_reg((s4) new_exception, REG_ITMP1);
			i386_call_reg(REG_ITMP1);    /* return value is REG_ITMP1_XPTR */
			i386_alu_imm_reg(I386_ADD, 1 * 4, REG_SP);

			i386_pop_reg(REG_ITMP2_XPC);

			i386_mov_imm_reg((s4) asm_handle_exception, REG_ITMP3);
			i386_jmp_reg(REG_ITMP3);
		}
	}

	/* generate divide by zero check stubs */
	xcodeptr = NULL;
	
	for (; xdivrefs != NULL; xdivrefs = xdivrefs->next) {
		if ((exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1*) mcodebase + xdivrefs->branchpos, 
							  xdivrefs->branchpos,
							  (u1*) xcodeptr - (u1*) mcodebase - (5 + 5 + 2));
			continue;
		}

		gen_resolvebranch((u1*) mcodebase + xdivrefs->branchpos, 
		                  xdivrefs->branchpos, (u1*) mcodeptr - mcodebase);

		MCODECHECK(8);

		i386_mov_imm_reg(0, REG_ITMP2_XPC);    /* 5 bytes */
		dseg_adddata(mcodeptr);
		i386_mov_imm_reg(xdivrefs->branchpos - 6, REG_ITMP1);    /* 5 bytes */
		i386_alu_reg_reg(I386_ADD, REG_ITMP1, REG_ITMP2_XPC);    /* 2 bytes */

		if (xcodeptr != NULL) {
			i386_jmp_imm(((u1 *) xcodeptr - (u1 *) mcodeptr) - 5);
		
		} else {
			xcodeptr = mcodeptr;

			i386_push_reg(REG_ITMP2_XPC);

			PREPARE_NATIVE_STACKINFO

			i386_alu_imm_reg(I386_SUB, 2 * 4, REG_SP);
			i386_mov_imm_membase((s4) string_java_lang_ArithmeticException, REG_SP, 0 * 4);
			i386_mov_imm_membase((s4) string_java_lang_ArithmeticException_message, REG_SP, 1 * 4);
			i386_mov_imm_reg((s4) new_exception_message, REG_ITMP1);
			i386_call_reg(REG_ITMP1);    /* return value is REG_ITMP1_XPTR */
			i386_alu_imm_reg(I386_ADD, 2 * 4, REG_SP);

			REMOVE_NATIVE_STACKINFO

			i386_pop_reg(REG_ITMP2_XPC);

			i386_mov_imm_reg((s4) asm_handle_exception, REG_ITMP3);
			i386_jmp_reg(REG_ITMP3);
		}
	}

	/* generate oom check stubs */
	xcodeptr = NULL;
	
	for (; xoomrefs != NULL; xoomrefs = xoomrefs->next) {
		if ((exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1*) mcodebase + xoomrefs->branchpos, 
							  xoomrefs->branchpos,
							  (u1*) xcodeptr - (u1*) mcodebase - (5 + 5 + 2));
			continue;
		}

		gen_resolvebranch((u1*) mcodebase + xoomrefs->branchpos, 
		                  xoomrefs->branchpos, (u1*) mcodeptr - mcodebase);

		MCODECHECK(8);

		i386_mov_imm_reg(0, REG_ITMP2_XPC);    /* 5 bytes */
		dseg_adddata(mcodeptr);
		i386_mov_imm_reg(xoomrefs->branchpos - 6, REG_ITMP1);    /* 5 bytes */
		i386_alu_reg_reg(I386_ADD, REG_ITMP1, REG_ITMP2_XPC);    /* 2 bytes */

		if (xcodeptr != NULL) {
			i386_jmp_imm(((u1 *) xcodeptr - (u1 *) mcodeptr) - 5);
		
		} else {
			xcodeptr = mcodeptr;

			i386_push_reg(REG_ITMP2_XPC);
			PREPARE_NATIVE_STACKINFO
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
			i386_mov_imm_reg((s4) &builtin_get_exceptionptrptr, REG_ITMP1);
			i386_call_reg(REG_ITMP1);
			i386_mov_membase_reg(REG_RESULT, 0, REG_ITMP3);
			i386_mov_imm_membase(0, REG_RESULT, 0);
			i386_mov_reg_reg(REG_ITMP3, REG_ITMP1_XPTR);
#else
			i386_mov_imm_reg((s4) &_exceptionptr, REG_ITMP3);
			i386_mov_membase_reg(REG_ITMP3, 0, REG_ITMP1_XPTR);
			i386_mov_imm_membase(0, REG_ITMP3, 0);
#endif
			REMOVE_NATIVE_STACKINFO
			i386_pop_reg(REG_ITMP2_XPC);

			i386_mov_imm_reg((s4) asm_handle_exception, REG_ITMP3);
			i386_jmp_reg(REG_ITMP3);
		}
	}

	/* generate null pointer check stubs */
	xcodeptr = NULL;
	
	for (; xnullrefs != NULL; xnullrefs = xnullrefs->next) {
		if ((exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1*) mcodebase + xnullrefs->branchpos, 
							  xnullrefs->branchpos,
							  (u1*) xcodeptr - (u1*) mcodebase - (5 + 5 + 2));
			continue;
		}

		gen_resolvebranch((u1*) mcodebase + xnullrefs->branchpos, 
						  xnullrefs->branchpos, (u1*) mcodeptr - mcodebase);
		
		MCODECHECK(8);

		i386_mov_imm_reg(0, REG_ITMP2_XPC);                       /* 5 bytes */
		dseg_adddata(mcodeptr);
		i386_mov_imm_reg(xnullrefs->branchpos - 6, REG_ITMP1);    /* 5 bytes */
		i386_alu_reg_reg(I386_ADD, REG_ITMP1, REG_ITMP2_XPC);     /* 2 bytes */
		
		if (xcodeptr != NULL) {
			i386_jmp_imm(((u1 *) xcodeptr - (u1 *) mcodeptr) - 5);
			
		} else {
			xcodeptr = mcodeptr;
			
/*  			i386_mov_imm_reg((s4) proto_java_lang_NullPointerException, REG_ITMP1_XPTR); */

			i386_push_reg(REG_ITMP2_XPC);



			PREPARE_NATIVE_STACKINFO
#if 0
			/* create native call block*/
		                i386_alu_imm_reg(I386_SUB, 3*4, REG_SP); /* build stack frame (4 * 4 bytes) */


				i386_mov_imm_reg((s4) codegen_stubcalled,REG_ITMP1);
				i386_call_reg(REG_ITMP1);                /*call    codegen_stubcalled*/

				i386_mov_imm_reg((s4) builtin_asm_get_stackframeinfo,REG_ITMP1);
				i386_call_reg(REG_ITMP1);                /*call    builtin_asm_get_stackframeinfo*/
                		i386_mov_imm_membase(0,REG_SP, 2*4);	/* builtin */
                		i386_mov_reg_membase(REG_RESULT,REG_SP,1*4); /* save thread pointer  to native call stack*/
				i386_mov_membase_reg(REG_RESULT,0,REG_ITMP2); /* get old value of thread specific native call stack */
				i386_mov_reg_membase(REG_ITMP2,REG_SP,0*4);	/* store value on stack */
   				i386_mov_reg_membase(REG_SP,REG_RESULT,0); /* store pointer to new stack frame information */
#endif				




			/* create exception*/
			i386_alu_imm_reg(I386_SUB, 1 * 4, REG_SP);
			i386_mov_imm_membase((s4) string_java_lang_NullPointerException, REG_SP, 0 * 4);
			i386_mov_imm_reg((s4) new_exception, REG_ITMP1);
			i386_call_reg(REG_ITMP1);    /* return value is REG_ITMP1_XPTR */
			i386_alu_imm_reg(I386_ADD, 1 * 4, REG_SP);


			REMOVE_NATIVE_STACKINFO
#if 0
			/* restore native call stack */
			i386_mov_membase_reg(REG_SP,0,REG_ITMP2);
			i386_mov_membase_reg(REG_SP,4,REG_ITMP3);
			i386_mov_reg_membase(REG_ITMP2,REG_ITMP3,0);
			i386_alu_imm_reg(I386_ADD,3*4,REG_SP);
#endif


			i386_pop_reg(REG_ITMP2_XPC);

			i386_mov_imm_reg((s4) asm_handle_exception, REG_ITMP3);
			i386_jmp_reg(REG_ITMP3);
		}
	}
	}
	
	codegen_finish((int)((u1*) mcodeptr - mcodebase));

	if (compileverbose) {
		char logtext[MAXLOGTEXT];
		sprintf(logtext, "Generating code done: ");
		utf_sprint_classname(logtext + strlen(logtext), class->name);
		sprintf(logtext + strlen(logtext), ".");
		utf_sprint(logtext + strlen(logtext), method->name);
		utf_sprint_classname(logtext + strlen(logtext), descriptor);
		log_text(logtext);
	}
}


/* function createcompilerstub *************************************************

   creates a stub routine which calls the compiler
	
*******************************************************************************/

#define COMPSTUBSIZE 12

u1 *createcompilerstub(methodinfo *m)
{
    u1 *s = CNEW(u1, COMPSTUBSIZE);     /* memory to hold the stub            */
    mcodeptr = s;                       /* code generation pointer            */

    /* code for the stub */
    i386_mov_imm_reg((s4) m, REG_ITMP1);/* pass method pointer to compiler    */

	/* we use REG_ITMP3 cause ECX (REG_ITMP2) is used for patching            */
    i386_mov_imm_reg((s4) asm_call_jit_compiler, REG_ITMP3);  /* load address */
    i386_jmp_reg(REG_ITMP3);            /* jump to compiler                   */

#ifdef STATISTICS
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

#define NATIVESTUBSIZE 340

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
		printf("base addr:%p, methodinfo:%p\n",*p,(methodinfo*)((*p)+8));
		methodinfo *m=*((methodinfo**)((*p)+8));
		utf_display(m->name);
		printf("\n");
		p=*p;
	}
	

}

u1 *createnativestub(functionptr f, methodinfo *m)
{
    u1 *s = CNEW(u1, NATIVESTUBSIZE);   /* memory to hold the stub            */

    u1 *tptr;
    int i;
    int stackframesize = 4+12;           /* initial 4 bytes is space for jni env,
					 	+ 4 byte thread pointer + 4 byte previous pointer + method info*/
    int stackframeoffset = 4;

    int p, t;

	mcodeptr = s;                       /* make macros work                   */

	if (m->flags & ACC_STATIC) {
		stackframesize += 4;
		stackframeoffset += 4;
	}

    reg_init();
    descriptor2types(m);                     /* set paramcount and paramtypes */
  
/*DEBUG*/
/* 	i386_push_reg(REG_SP);
        i386_mov_imm_reg((s4) i386_native_stub_debug, REG_ITMP1);
        i386_call_reg(REG_ITMP1);
 	i386_pop_reg(REG_ITMP1);*/


	/* if function is static, check for initialized */

	if (m->flags & ACC_STATIC) {
	/* if class isn't yet initialized, do it */
		if (!m->class->initialized) {
			/* call helper function which patches this code */
			i386_mov_imm_reg((s4) m->class, REG_ITMP1);
			i386_mov_imm_reg((s4) asm_check_clinit, REG_ITMP2);
			i386_call_reg(REG_ITMP2);
		}
	}

    if (runverbose) {
        i386_alu_imm_reg(I386_SUB, TRACE_ARGS_NUM * 8 + 4, REG_SP);
        
        for (p = 0; p < m->paramcount; p++) {
            t = m->paramtypes[p];
            if (IS_INT_LNG_TYPE(t)) {
                if (IS_2_WORD_TYPE(t)) {
                    i386_mov_membase_reg(REG_SP, 4 + (TRACE_ARGS_NUM + p) * 8 + 4, REG_ITMP1);
                    i386_mov_membase_reg(REG_SP, 4 + (TRACE_ARGS_NUM + p) * 8 + 4 + 4, REG_ITMP2);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, p * 8);
					i386_mov_reg_membase(REG_ITMP2, REG_SP, p * 8 + 4);

                } else if (t == TYPE_ADR) {
                    i386_mov_membase_reg(REG_SP, 4 + (TRACE_ARGS_NUM + p) * 8 + 4, REG_ITMP1);
                    i386_alu_reg_reg(I386_XOR, REG_ITMP2, REG_ITMP2);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, p * 8);
					i386_mov_reg_membase(REG_ITMP2, REG_SP, p * 8 + 4);

                } else {
                    i386_mov_membase_reg(REG_SP, 4 + (TRACE_ARGS_NUM + p) * 8 + 4, EAX);
                    i386_cltd();
					i386_mov_reg_membase(EAX, REG_SP, p * 8);
					i386_mov_reg_membase(EDX, REG_SP, p * 8 + 4);
                }

            } else {
                if (!IS_2_WORD_TYPE(t)) {
                    i386_flds_membase(REG_SP, 4 + (TRACE_ARGS_NUM + p) * 8 + 4);
                    i386_fstps_membase(REG_SP, p * 8);
                    i386_alu_reg_reg(I386_XOR, REG_ITMP2, REG_ITMP2);
                    i386_mov_reg_membase(REG_ITMP2, REG_SP, p * 8 + 4);

                } else {
                    i386_fldl_membase(REG_SP, 4 + (TRACE_ARGS_NUM + p) * 8 + 4);
                    i386_fstpl_membase(REG_SP, p * 8);
                }
            }
        }
		
        i386_alu_reg_reg(I386_XOR, REG_ITMP1, REG_ITMP1);
        for (p = m->paramcount; p < TRACE_ARGS_NUM; p++) {
            i386_mov_reg_membase(REG_ITMP1, REG_SP, p * 8);
            i386_mov_reg_membase(REG_ITMP1, REG_SP, p * 8 + 4);
        }

        i386_mov_imm_membase((s4) m, REG_SP, TRACE_ARGS_NUM * 8);

        i386_mov_imm_reg((s4) builtin_trace_args, REG_ITMP1);
        i386_call_reg(REG_ITMP1);

        i386_alu_imm_reg(I386_ADD, TRACE_ARGS_NUM * 8 + 4, REG_SP);
    }

    /*
	 * mark the whole fpu stack as free for native functions
	 * (only for saved register count == 0)
	 */
    i386_ffree_reg(0);
    i386_ffree_reg(1);
    i386_ffree_reg(2);
    i386_ffree_reg(3);
    i386_ffree_reg(4);
    i386_ffree_reg(5);
    i386_ffree_reg(6);
    i386_ffree_reg(7);

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

    i386_alu_imm_reg(I386_SUB, stackframesize, REG_SP);

/* CREATE DYNAMIC STACK INFO -- BEGIN*/
   i386_mov_imm_membase((s4) m, REG_SP,stackframesize-4);
   i386_mov_imm_reg((s4) builtin_asm_get_stackframeinfo, REG_ITMP1);
   i386_call_reg(REG_ITMP1);
   i386_mov_reg_membase(REG_RESULT,REG_SP,stackframesize-8); /*save thread specific pointer*/
   i386_mov_membase_reg(REG_RESULT,0,REG_ITMP2); 
   i386_mov_reg_membase(REG_ITMP2,REG_SP,stackframesize-12); /*save previous value of memory adress pointed to by thread specific pointer*/
   i386_mov_reg_reg(REG_SP,REG_ITMP2);
   i386_alu_imm_reg(I386_ADD,stackframesize-12,REG_ITMP2);
   i386_mov_reg_membase(REG_ITMP2,REG_RESULT,0);

/*TESTING ONLY */
/*   i386_mov_imm_membase((s4) m, REG_SP,stackframesize-4);
   i386_mov_imm_membase((s4) m, REG_SP,stackframesize-8);
   i386_mov_imm_membase((s4) m, REG_SP,stackframesize-12);*/

/* CREATE DYNAMIC STACK INFO -- END*/


    tptr = m->paramtypes;
    for (i = 0; i < m->paramcount; i++) {
        switch (*tptr++) {
        case TYPE_INT:
        case TYPE_FLT:
        case TYPE_ADR:
            i386_mov_membase_reg(REG_SP, stackframesize + (1 * 4) + i * 8, REG_ITMP1);
            i386_mov_reg_membase(REG_ITMP1, REG_SP, stackframeoffset);
            stackframeoffset += 4;
            break;

        case TYPE_LNG:
        case TYPE_DBL:
            i386_mov_membase_reg(REG_SP, stackframesize + (1 * 4) + i * 8, REG_ITMP1);
            i386_mov_membase_reg(REG_SP, stackframesize + (1 * 4) + i * 8 + 4, REG_ITMP2);
            i386_mov_reg_membase(REG_ITMP1, REG_SP, stackframeoffset);
            i386_mov_reg_membase(REG_ITMP2, REG_SP, stackframeoffset + 4);
            stackframeoffset += 8;
            break;

        default:
            panic("unknown parameter type in native function");
        }
    }

	if (m->flags & ACC_STATIC) {
		/* put class into second argument */
		i386_mov_imm_membase((s4) m->class, REG_SP, 4);
	}

	/* put env into first argument */
	i386_mov_imm_membase((s4) &env, REG_SP, 0);

    i386_mov_imm_reg((s4) f, REG_ITMP1);
    i386_call_reg(REG_ITMP1);
/*REMOVE DYNAMIC STACK INFO -BEGIN */
    i386_push_reg(REG_RESULT2);
    i386_mov_membase_reg(REG_SP,stackframesize-8,REG_ITMP2); /*old value*/
    i386_mov_membase_reg(REG_SP,stackframesize-4,REG_RESULT2); /*pointer*/
    i386_mov_reg_membase(REG_ITMP2,REG_RESULT2,0);
    i386_pop_reg(REG_RESULT2);
/*REMOVE DYNAMIC STACK INFO -END */

    i386_alu_imm_reg(I386_ADD, stackframesize, REG_SP);


    if (runverbose) {
        i386_alu_imm_reg(I386_SUB, 4 + 8 + 8 + 4, REG_SP);
		
        i386_mov_imm_membase((s4) m, REG_SP, 0);
		
        i386_mov_reg_membase(REG_RESULT, REG_SP, 4);
        i386_mov_reg_membase(REG_RESULT2, REG_SP, 4 + 4);
		
        i386_fstl_membase(REG_SP, 4 + 8);
        i386_fsts_membase(REG_SP, 4 + 8 + 8);
		
        i386_mov_imm_reg((s4) builtin_displaymethodstop, REG_ITMP1);
        i386_call_reg(REG_ITMP1);
		
        i386_mov_membase_reg(REG_SP, 4, REG_RESULT);
        i386_mov_membase_reg(REG_SP, 4 + 4, REG_RESULT2);
		
        i386_alu_imm_reg(I386_ADD, 4 + 8 + 8 + 4, REG_SP);
    }


	/* we can't use REG_ITMP3 == REG_RESULT2 */
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	i386_push_reg(REG_RESULT);
	i386_push_reg(REG_RESULT2);
	i386_call_mem((s4) &callgetexceptionptrptr);
	i386_mov_membase_reg(REG_RESULT, 0, REG_ITMP2);
	i386_test_reg_reg(REG_ITMP2, REG_ITMP2);
	i386_pop_reg(REG_RESULT2);
	i386_pop_reg(REG_RESULT);
#else
	i386_mov_imm_reg((s4) &_exceptionptr, REG_ITMP2);
	i386_mov_membase_reg(REG_ITMP2, 0, REG_ITMP2);
	i386_test_reg_reg(REG_ITMP2, REG_ITMP2);
#endif
	i386_jcc(I386_CC_NE, 1);

	i386_ret();

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	i386_push_reg(REG_ITMP2);
	i386_call_mem((s4) &callgetexceptionptrptr);
	i386_mov_imm_membase(0, REG_RESULT, 0);
	i386_pop_reg(REG_ITMP1_XPTR);
#else
	i386_mov_reg_reg(REG_ITMP2, REG_ITMP1_XPTR);
	i386_mov_imm_reg((s4) &_exceptionptr, REG_ITMP2);
	i386_mov_imm_membase(0, REG_ITMP2, 0);
#endif
	i386_mov_membase_reg(REG_SP, 0, REG_ITMP2_XPC);
	i386_alu_imm_reg(I386_SUB, 2, REG_ITMP2_XPC);

	i386_mov_imm_reg((s4) asm_handle_nat_exception, REG_ITMP3);
	i386_jmp_reg(REG_ITMP3);

#ifdef STATISTICS
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



void i386_emit_ialu(s4 alu_op, stackptr src, instruction *iptr)
{
	if (iptr->dst->flags & INMEMORY) {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (src->regoff == iptr->dst->regoff) {
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_alu_reg_membase(alu_op, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

			} else if (src->prev->regoff == iptr->dst->regoff) {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_reg_membase(alu_op, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

			} else {
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_alu_membase_reg(alu_op, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
			}

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			if (src->regoff == iptr->dst->regoff) {
				i386_alu_reg_membase(alu_op, src->prev->regoff, REG_SP, iptr->dst->regoff * 8);

			} else {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_reg_reg(alu_op, src->prev->regoff, REG_ITMP1);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
			}

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (src->prev->regoff == iptr->dst->regoff) {
				i386_alu_reg_membase(alu_op, src->regoff, REG_SP, iptr->dst->regoff * 8);
						
			} else {
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_alu_reg_reg(alu_op, src->regoff, REG_ITMP1);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
			}

		} else {
			i386_mov_reg_membase(src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
			i386_alu_reg_membase(alu_op, src->regoff, REG_SP, iptr->dst->regoff * 8);
		}

	} else {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
			i386_alu_membase_reg(alu_op, REG_SP, src->regoff * 8, iptr->dst->regoff);

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
			i386_alu_membase_reg(alu_op, REG_SP, src->regoff * 8, iptr->dst->regoff);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			M_INTMOVE(src->regoff, iptr->dst->regoff);
			i386_alu_membase_reg(alu_op, REG_SP, src->prev->regoff * 8, iptr->dst->regoff);

		} else {
			if (src->regoff == iptr->dst->regoff) {
				i386_alu_reg_reg(alu_op, src->prev->regoff, iptr->dst->regoff);

			} else {
				M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
				i386_alu_reg_reg(alu_op, src->regoff, iptr->dst->regoff);
			}
		}
	}
}



void i386_emit_ialuconst(s4 alu_op, stackptr src, instruction *iptr)
{
	if (iptr->dst->flags & INMEMORY) {
		if (src->flags & INMEMORY) {
			if (src->regoff == iptr->dst->regoff) {
				i386_alu_imm_membase(alu_op, iptr->val.i, REG_SP, iptr->dst->regoff * 8);

			} else {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_imm_reg(alu_op, iptr->val.i, REG_ITMP1);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
			}

		} else {
			i386_mov_reg_membase(src->regoff, REG_SP, iptr->dst->regoff * 8);
			i386_alu_imm_membase(alu_op, iptr->val.i, REG_SP, iptr->dst->regoff * 8);
		}

	} else {
		if (src->flags & INMEMORY) {
			i386_mov_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);
			i386_alu_imm_reg(alu_op, iptr->val.i, iptr->dst->regoff);

		} else {
			M_INTMOVE(src->regoff, iptr->dst->regoff);
			i386_alu_imm_reg(alu_op, iptr->val.i, iptr->dst->regoff);
		}
	}
}



void i386_emit_lalu(s4 alu_op, stackptr src, instruction *iptr)
{
	if (iptr->dst->flags & INMEMORY) {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (src->regoff == iptr->dst->regoff) {
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_alu_reg_membase(alu_op, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP1);
				i386_alu_reg_membase(alu_op, REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);

			} else if (src->prev->regoff == iptr->dst->regoff) {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_reg_membase(alu_op, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP1);
				i386_alu_reg_membase(alu_op, REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);

			} else {
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_alu_membase_reg(alu_op, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP1);
				i386_alu_membase_reg(alu_op, REG_SP, src->regoff * 8 + 4, REG_ITMP1);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);
			}
		}
	}
}



void i386_emit_laluconst(s4 alu_op, stackptr src, instruction *iptr)
{
	if (iptr->dst->flags & INMEMORY) {
		if (src->flags & INMEMORY) {
			if (src->regoff == iptr->dst->regoff) {
				i386_alu_imm_membase(alu_op, iptr->val.l, REG_SP, iptr->dst->regoff * 8);
				i386_alu_imm_membase(alu_op, iptr->val.l >> 32, REG_SP, iptr->dst->regoff * 8 + 4);

			} else {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_imm_reg(alu_op, iptr->val.l, REG_ITMP1);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP1);
				i386_alu_imm_reg(alu_op, iptr->val.l >> 32, REG_ITMP1);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8 + 4);
			}
		}
	}
}



void i386_emit_ishift(s4 shift_op, stackptr src, instruction *iptr)
{
	if (iptr->dst->flags & INMEMORY) {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (src->prev->regoff == iptr->dst->regoff) {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, ECX);
				i386_shift_membase(shift_op, REG_SP, iptr->dst->regoff * 8);

			} else {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, ECX);
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_shift_reg(shift_op, REG_ITMP1);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
			}

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			i386_mov_membase_reg(REG_SP, src->regoff * 8, ECX);
			i386_mov_reg_membase(src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
			i386_shift_membase(shift_op, REG_SP, iptr->dst->regoff * 8);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			if (src->prev->regoff == iptr->dst->regoff) {
				M_INTMOVE(src->regoff, ECX);
				i386_shift_membase(shift_op, REG_SP, iptr->dst->regoff * 8);

			} else {
				M_INTMOVE(src->regoff, ECX);
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_shift_reg(shift_op, REG_ITMP1);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
			}

		} else {
			M_INTMOVE(src->regoff, ECX);
			i386_mov_reg_membase(src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
			i386_shift_membase(shift_op, REG_SP, iptr->dst->regoff * 8);
		}

	} else {
		if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			i386_mov_membase_reg(REG_SP, src->regoff * 8, ECX);
			i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
			i386_shift_reg(shift_op, iptr->dst->regoff);

		} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
			i386_mov_membase_reg(REG_SP, src->regoff * 8, ECX);
			M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
			i386_shift_reg(shift_op, iptr->dst->regoff);

		} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
			M_INTMOVE(src->regoff, ECX);
			i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
			i386_shift_reg(shift_op, iptr->dst->regoff);

		} else {
			M_INTMOVE(src->regoff, ECX);
			M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
			i386_shift_reg(shift_op, iptr->dst->regoff);
		}
	}
}



void i386_emit_ishiftconst(s4 shift_op, stackptr src, instruction *iptr)
{
	if ((src->flags & INMEMORY) && (iptr->dst->flags & INMEMORY)) {
		if (src->regoff == iptr->dst->regoff) {
			i386_shift_imm_membase(shift_op, iptr->val.i, REG_SP, iptr->dst->regoff * 8);

		} else {
			i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
			i386_shift_imm_reg(shift_op, iptr->val.i, REG_ITMP1);
			i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
		}

	} else if ((src->flags & INMEMORY) && !(iptr->dst->flags & INMEMORY)) {
		i386_mov_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);
		i386_shift_imm_reg(shift_op, iptr->val.i, iptr->dst->regoff);
				
	} else if (!(src->flags & INMEMORY) && (iptr->dst->flags & INMEMORY)) {
		i386_mov_reg_membase(src->regoff, REG_SP, iptr->dst->regoff * 8);
		i386_shift_imm_membase(shift_op, iptr->val.i, REG_SP, iptr->dst->regoff * 8);

	} else {
		M_INTMOVE(src->regoff, iptr->dst->regoff);
		i386_shift_imm_reg(shift_op, iptr->val.i, iptr->dst->regoff);
	}
}



void i386_emit_ifcc_iconst(s4 if_op, stackptr src, instruction *iptr)
{
	if (iptr->dst->flags & INMEMORY) {
		int offset = 0;

		if (src->flags & INMEMORY) {
			i386_alu_imm_membase(I386_CMP, 0, REG_SP, src->regoff * 8);

		} else {
			i386_test_reg_reg(src->regoff, src->regoff);
		}

		offset += 7;
		CALCOFFSETBYTES(offset, REG_SP, iptr->dst->regoff * 8);
	
		i386_jcc(if_op, offset + (iptr[1].opc == ICMD_ELSE_ICONST) ? 5 + offset : 0);
		i386_mov_imm_membase(iptr->val.i, REG_SP, iptr->dst->regoff * 8);

		if ((iptr[1].opc == ICMD_ELSE_ICONST)) {
			i386_jmp_imm(offset);
			i386_mov_imm_membase(iptr[1].val.i, REG_SP, iptr->dst->regoff * 8);
		}

	} else {
		if (src->flags & INMEMORY) {
			i386_alu_imm_membase(I386_CMP, 0, REG_SP, src->regoff * 8);

		} else {
			i386_test_reg_reg(src->regoff, src->regoff);
		}

		i386_jcc(if_op, (iptr[1].opc == ICMD_ELSE_ICONST) ? 10 : 5);
		i386_mov_imm_reg(iptr->val.i, iptr->dst->regoff);

		if ((iptr[1].opc == ICMD_ELSE_ICONST)) {
			i386_jmp_imm(5);
			i386_mov_imm_reg(iptr[1].val.i, iptr->dst->regoff);
		}
	}
}



/*
 * mov ops
 */
void i386_mov_reg_reg(s4 reg, s4 dreg) {
	*(mcodeptr++) = (u1) 0x89;
	i386_emit_reg((reg),(dreg));
}


void i386_mov_imm_reg(s4 imm, s4 reg) {
	*(mcodeptr++) = (u1) 0xb8 + ((reg) & 0x07);
	i386_emit_imm32((imm));
}


void i386_movb_imm_reg(s4 imm, s4 reg) {
	*(mcodeptr++) = (u1) 0xc6;
	i386_emit_reg(0,(reg));
	i386_emit_imm8((imm));
}


void i386_mov_membase_reg(s4 basereg, s4 disp, s4 reg) {
	*(mcodeptr++) = (u1) 0x8b;
	i386_emit_membase((basereg),(disp),(reg));
}


/*
 * this one is for INVOKEVIRTUAL/INVOKEINTERFACE to have a
 * constant membase immediate length of 32bit
 */
void i386_mov_membase32_reg(s4 basereg, s4 disp, s4 reg) {
	*(mcodeptr++) = (u1) 0x8b;
	i386_address_byte(2, (reg), (basereg));
	i386_emit_imm32((disp));
}


void i386_mov_reg_membase(s4 reg, s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0x89;
	i386_emit_membase((basereg),(disp),(reg));
}


void i386_mov_memindex_reg(s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg) {
	*(mcodeptr++) = (u1) 0x8b;
	i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void i386_mov_reg_memindex(s4 reg, s4 disp, s4 basereg, s4 indexreg, s4 scale) {
	*(mcodeptr++) = (u1) 0x89;
	i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void i386_mov_mem_reg(s4 mem, s4 dreg) {
	*(mcodeptr++) = (u1) 0x8b;
	i386_emit_mem((dreg),(mem));
}


void i386_movw_reg_memindex(s4 reg, s4 disp, s4 basereg, s4 indexreg, s4 scale) {
	*(mcodeptr++) = (u1) 0x66;
	*(mcodeptr++) = (u1) 0x89;
	i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void i386_movb_reg_memindex(s4 reg, s4 disp, s4 basereg, s4 indexreg, s4 scale) {
	*(mcodeptr++) = (u1) 0x88;
	i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void i386_mov_imm_membase(s4 imm, s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xc7;
	i386_emit_membase((basereg),(disp),0);
	i386_emit_imm32((imm));
}


void i386_movsbl_memindex_reg(s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xbe;
	i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void i386_movswl_memindex_reg(s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xbf;
	i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}


void i386_movzwl_memindex_reg(s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xb7;
	i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale));
}



/*
 * alu operations
 */
void i386_alu_reg_reg(s4 opc, s4 reg, s4 dreg) {
	*(mcodeptr++) = (((u1) (opc)) << 3) + 1;
	i386_emit_reg((reg),(dreg));
}


void i386_alu_reg_membase(s4 opc, s4 reg, s4 basereg, s4 disp) {
	*(mcodeptr++) = (((u1) (opc)) << 3) + 1;
	i386_emit_membase((basereg),(disp),(reg));
}


void i386_alu_membase_reg(s4 opc, s4 basereg, s4 disp, s4 reg) {
	*(mcodeptr++) = (((u1) (opc)) << 3) + 3;
	i386_emit_membase((basereg),(disp),(reg));
}


void i386_alu_imm_reg(s4 opc, s4 imm, s4 dreg) {
	if (i386_is_imm8(imm)) { 
		*(mcodeptr++) = (u1) 0x83;
		i386_emit_reg((opc),(dreg));
		i386_emit_imm8((imm));
	} else { 
		*(mcodeptr++) = (u1) 0x81;
		i386_emit_reg((opc),(dreg));
		i386_emit_imm32((imm));
	} 
}


void i386_alu_imm_membase(s4 opc, s4 imm, s4 basereg, s4 disp) {
	if (i386_is_imm8(imm)) { 
		*(mcodeptr++) = (u1) 0x83;
		i386_emit_membase((basereg),(disp),(opc));
		i386_emit_imm8((imm));
	} else { 
		*(mcodeptr++) = (u1) 0x81;
		i386_emit_membase((basereg),(disp),(opc));
		i386_emit_imm32((imm));
	} 
}


void i386_test_reg_reg(s4 reg, s4 dreg) {
	*(mcodeptr++) = (u1) 0x85;
	i386_emit_reg((reg),(dreg));
}


void i386_test_imm_reg(s4 imm, s4 reg) {
	*(mcodeptr++) = (u1) 0xf7;
	i386_emit_reg(0,(reg));
	i386_emit_imm32((imm));
}



/*
 * inc, dec operations
 */
void i386_dec_mem(s4 mem) {
	*(mcodeptr++) = (u1) 0xff;
	i386_emit_mem(1,(mem));
}



void i386_cltd() {
	*(mcodeptr++) = (u1) 0x99;
}



void i386_imul_reg_reg(s4 reg, s4 dreg) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xaf;
	i386_emit_reg((dreg),(reg));
}


void i386_imul_membase_reg(s4 basereg, s4 disp, s4 dreg) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xaf;
	i386_emit_membase((basereg),(disp),(dreg));
}


void i386_imul_imm_reg(s4 imm, s4 dreg) {
	if (i386_is_imm8((imm))) { 
		*(mcodeptr++) = (u1) 0x6b;
		i386_emit_reg(0,(dreg));
		i386_emit_imm8((imm));
	} else { 
		*(mcodeptr++) = (u1) 0x69;
		i386_emit_reg(0,(dreg));
		i386_emit_imm32((imm));
	} 
}


void i386_imul_imm_reg_reg(s4 imm, s4 reg, s4 dreg) {
	if (i386_is_imm8((imm))) { 
		*(mcodeptr++) = (u1) 0x6b;
		i386_emit_reg((dreg),(reg));
		i386_emit_imm8((imm));
	} else { 
		*(mcodeptr++) = (u1) 0x69;
		i386_emit_reg((dreg),(reg));
		i386_emit_imm32((imm));
	} 
}


void i386_imul_imm_membase_reg(s4 imm, s4 basereg, s4 disp, s4 dreg) {
	if (i386_is_imm8((imm))) { 
		*(mcodeptr++) = (u1) 0x6b;
		i386_emit_membase((basereg),(disp),(dreg));
		i386_emit_imm8((imm));
	} else { 
		*(mcodeptr++) = (u1) 0x69;
		i386_emit_membase((basereg),(disp),(dreg));
		i386_emit_imm32((imm));
	} 
}


void i386_mul_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xf7;
	i386_emit_membase((basereg),(disp),4);
}


void i386_idiv_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xf7;
	i386_emit_reg(7,(reg));
}



void i386_ret() {
	*(mcodeptr++) = (u1) 0xc3;
}



/*
 * shift ops
 */
void i386_shift_reg(s4 opc, s4 reg) {
	*(mcodeptr++) = (u1) 0xd3;
	i386_emit_reg((opc),(reg));
}


void i386_shift_membase(s4 opc, s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xd3;
	i386_emit_membase((basereg),(disp),(opc));
}


void i386_shift_imm_reg(s4 opc, s4 imm, s4 dreg) {
	if ((imm) == 1) { 
		*(mcodeptr++) = (u1) 0xd1;
		i386_emit_reg((opc),(dreg));
	} else { 
		*(mcodeptr++) = (u1) 0xc1;
		i386_emit_reg((opc),(dreg));
		i386_emit_imm8((imm));
	} 
}


void i386_shift_imm_membase(s4 opc, s4 imm, s4 basereg, s4 disp) {
	if ((imm) == 1) { 
		*(mcodeptr++) = (u1) 0xd1;
		i386_emit_membase((basereg),(disp),(opc));
	} else { 
		*(mcodeptr++) = (u1) 0xc1;
		i386_emit_membase((basereg),(disp),(opc));
		i386_emit_imm8((imm));
	} 
}


void i386_shld_reg_reg(s4 reg, s4 dreg) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xa5;
	i386_emit_reg((reg),(dreg));
}


void i386_shld_imm_reg_reg(s4 imm, s4 reg, s4 dreg) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xa4;
	i386_emit_reg((reg),(dreg));
	i386_emit_imm8((imm));
}


void i386_shld_reg_membase(s4 reg, s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xa5;
	i386_emit_membase((basereg),(disp),(reg));
}


void i386_shrd_reg_reg(s4 reg, s4 dreg) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xad;
	i386_emit_reg((reg),(dreg));
}


void i386_shrd_imm_reg_reg(s4 imm, s4 reg, s4 dreg) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xac;
	i386_emit_reg((reg),(dreg));
	i386_emit_imm8((imm));
}


void i386_shrd_reg_membase(s4 reg, s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xad;
	i386_emit_membase((basereg),(disp),(reg));
}



/*
 * jump operations
 */
void i386_jmp_imm(s4 imm) {
	*(mcodeptr++) = (u1) 0xe9;
	i386_emit_imm32((imm));
}


void i386_jmp_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xff;
	i386_emit_reg(4,(reg));
}


void i386_jcc(s4 opc, s4 imm) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) (0x80 + (opc));
	i386_emit_imm32((imm));
}



/*
 * conditional set operations
 */
void i386_setcc_reg(s4 opc, s4 reg) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) (0x90 + (opc));
	i386_emit_reg(0,(reg));
}


void i386_setcc_membase(s4 opc, s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) (0x90 + (opc));
	i386_emit_membase((basereg),(disp),0);
}


void i386_xadd_reg_mem(s4 reg, s4 mem) {
	*(mcodeptr++) = (u1) 0x0f;
	*(mcodeptr++) = (u1) 0xc1;
	i386_emit_mem((reg),(mem));
}


void i386_neg_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xf7;
	i386_emit_reg(3,(reg));
}


void i386_neg_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xf7;
	i386_emit_membase((basereg),(disp),3);
}



void i386_push_imm(s4 imm) {
	*(mcodeptr++) = (u1) 0x68;
	i386_emit_imm32((imm));
}


void i386_pop_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0x58 + (0x07 & (reg));
}


void i386_push_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0x50 + (0x07 & (reg));
}


void i386_nop() {
	*(mcodeptr++) = (u1) 0x90;
}


void i386_lock() {
	*(mcodeptr++) = (u1) 0xf0;
}


/*
 * call instructions
 */
void i386_call_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xff;
	i386_emit_reg(2,(reg));
}


void i386_call_imm(s4 imm) {
	*(mcodeptr++) = (u1) 0xe8;
	i386_emit_imm32((imm));
}


void i386_call_mem(s4 mem) {
	*(mcodeptr++) = (u1) 0xff;
	i386_emit_mem(2, (mem));
}



/*
 * floating point instructions
 */
void i386_fld1() {
	*(mcodeptr++) = (u1) 0xd9;
	*(mcodeptr++) = (u1) 0xe8;
}


void i386_fldz() {
	*(mcodeptr++) = (u1) 0xd9;
	*(mcodeptr++) = (u1) 0xee;
}


void i386_fld_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xd9;
	*(mcodeptr++) = (u1) 0xc0 + (0x07 & (reg));
}


void i386_flds_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xd9;
	i386_emit_membase((basereg),(disp),0);
}


void i386_fldl_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdd;
	i386_emit_membase((basereg),(disp),0);
}


void i386_fldt_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdb;
	i386_emit_membase((basereg),(disp),5);
}


void i386_flds_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale) {
	*(mcodeptr++) = (u1) 0xd9;
	i386_emit_memindex(0,(disp),(basereg),(indexreg),(scale));
}


void i386_fldl_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale) {
	*(mcodeptr++) = (u1) 0xdd;
	i386_emit_memindex(0,(disp),(basereg),(indexreg),(scale));
}




void i386_fildl_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdb;
	i386_emit_membase((basereg),(disp),0);
}


void i386_fildll_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdf;
	i386_emit_membase((basereg),(disp),5);
}




void i386_fst_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xdd;
	*(mcodeptr++) = (u1) 0xd0 + (0x07 & (reg));
}


void i386_fsts_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xd9;
	i386_emit_membase((basereg),(disp),2);
}


void i386_fstl_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdd;
	i386_emit_membase((basereg),(disp),2);
}


void i386_fsts_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale) {
	*(mcodeptr++) = (u1) 0xd9;
	i386_emit_memindex(2,(disp),(basereg),(indexreg),(scale));
}


void i386_fstl_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale) {
	*(mcodeptr++) = (u1) 0xdd;
	i386_emit_memindex(2,(disp),(basereg),(indexreg),(scale));
}


void i386_fstp_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xdd;
	*(mcodeptr++) = (u1) 0xd8 + (0x07 & (reg));
}


void i386_fstps_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xd9;
	i386_emit_membase((basereg),(disp),3);
}


void i386_fstpl_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdd;
	i386_emit_membase((basereg),(disp),3);
}


void i386_fstpt_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdb;
	i386_emit_membase((basereg),(disp),7);
}


void i386_fstps_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale) {
	*(mcodeptr++) = (u1) 0xd9;
	i386_emit_memindex(3,(disp),(basereg),(indexreg),(scale));
}


void i386_fstpl_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale) {
	*(mcodeptr++) = (u1) 0xdd;
	i386_emit_memindex(3,(disp),(basereg),(indexreg),(scale));
}


void i386_fistl_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdb;
	i386_emit_membase((basereg),(disp),2);
}


void i386_fistpl_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdb;
	i386_emit_membase((basereg),(disp),3);
}


void i386_fistpll_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdf;
	i386_emit_membase((basereg),(disp),7);
}


void i386_fchs() {
	*(mcodeptr++) = (u1) 0xd9;
	*(mcodeptr++) = (u1) 0xe0;
}


void i386_faddp() {
	*(mcodeptr++) = (u1) 0xde;
	*(mcodeptr++) = (u1) 0xc1;
}


void i386_fadd_reg_st(s4 reg) {
	*(mcodeptr++) = (u1) 0xd8;
	*(mcodeptr++) = (u1) 0xc0 + (0x0f & (reg));
}


void i386_fadd_st_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xdc;
	*(mcodeptr++) = (u1) 0xc0 + (0x0f & (reg));
}


void i386_faddp_st_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xde;
	*(mcodeptr++) = (u1) 0xc0 + (0x0f & (reg));
}


void i386_fadds_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xd8;
	i386_emit_membase((basereg),(disp),0);
}


void i386_faddl_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdc;
	i386_emit_membase((basereg),(disp),0);
}


void i386_fsub_reg_st(s4 reg) {
	*(mcodeptr++) = (u1) 0xd8;
	*(mcodeptr++) = (u1) 0xe0 + (0x07 & (reg));
}


void i386_fsub_st_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xdc;
	*(mcodeptr++) = (u1) 0xe8 + (0x07 & (reg));
}


void i386_fsubp_st_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xde;
	*(mcodeptr++) = (u1) 0xe8 + (0x07 & (reg));
}


void i386_fsubp() {
	*(mcodeptr++) = (u1) 0xde;
	*(mcodeptr++) = (u1) 0xe9;
}


void i386_fsubs_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xd8;
	i386_emit_membase((basereg),(disp),4);
}


void i386_fsubl_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdc;
	i386_emit_membase((basereg),(disp),4);
}


void i386_fmul_reg_st(s4 reg) {
	*(mcodeptr++) = (u1) 0xd8;
	*(mcodeptr++) = (u1) 0xc8 + (0x07 & (reg));
}


void i386_fmul_st_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xdc;
	*(mcodeptr++) = (u1) 0xc8 + (0x07 & (reg));
}


void i386_fmulp() {
	*(mcodeptr++) = (u1) 0xde;
	*(mcodeptr++) = (u1) 0xc9;
}


void i386_fmulp_st_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xde;
	*(mcodeptr++) = (u1) 0xc8 + (0x07 & (reg));
}


void i386_fmuls_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xd8;
	i386_emit_membase((basereg),(disp),1);
}


void i386_fmull_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xdc;
	i386_emit_membase((basereg),(disp),1);
}


void i386_fdiv_reg_st(s4 reg) {
	*(mcodeptr++) = (u1) 0xd8;
	*(mcodeptr++) = (u1) 0xf0 + (0x07 & (reg));
}


void i386_fdiv_st_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xdc;
	*(mcodeptr++) = (u1) 0xf8 + (0x07 & (reg));
}


void i386_fdivp() {
	*(mcodeptr++) = (u1) 0xde;
	*(mcodeptr++) = (u1) 0xf9;
}


void i386_fdivp_st_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xde;
	*(mcodeptr++) = (u1) 0xf8 + (0x07 & (reg));
}


void i386_fxch() {
	*(mcodeptr++) = (u1) 0xd9;
	*(mcodeptr++) = (u1) 0xc9;
}


void i386_fxch_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xd9;
	*(mcodeptr++) = (u1) 0xc8 + (0x07 & (reg));
}


void i386_fprem() {
	*(mcodeptr++) = (u1) 0xd9;
	*(mcodeptr++) = (u1) 0xf8;
}


void i386_fprem1() {
	*(mcodeptr++) = (u1) 0xd9;
	*(mcodeptr++) = (u1) 0xf5;
}


void i386_fucom() {
	*(mcodeptr++) = (u1) 0xdd;
	*(mcodeptr++) = (u1) 0xe1;
}


void i386_fucom_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xdd;
	*(mcodeptr++) = (u1) 0xe0 + (0x07 & (reg));
}


void i386_fucomp_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xdd;
	*(mcodeptr++) = (u1) 0xe8 + (0x07 & (reg));
}


void i386_fucompp() {
	*(mcodeptr++) = (u1) 0xda;
	*(mcodeptr++) = (u1) 0xe9;
}


void i386_fnstsw() {
	*(mcodeptr++) = (u1) 0xdf;
	*(mcodeptr++) = (u1) 0xe0;
}


void i386_sahf() {
	*(mcodeptr++) = (u1) 0x9e;
}


void i386_finit() {
	*(mcodeptr++) = (u1) 0x9b;
	*(mcodeptr++) = (u1) 0xdb;
	*(mcodeptr++) = (u1) 0xe3;
}


void i386_fldcw_mem(s4 mem) {
	*(mcodeptr++) = (u1) 0xd9;
	i386_emit_mem(5,(mem));
}


void i386_fldcw_membase(s4 basereg, s4 disp) {
	*(mcodeptr++) = (u1) 0xd9;
	i386_emit_membase((basereg),(disp),5);
}


void i386_wait() {
	*(mcodeptr++) = (u1) 0x9b;
}


void i386_ffree_reg(s4 reg) {
	*(mcodeptr++) = (u1) 0xdd;
	*(mcodeptr++) = (u1) 0xc0 + (0x07 & (reg));
}


void i386_fdecstp() {
	*(mcodeptr++) = (u1) 0xd9;
	*(mcodeptr++) = (u1) 0xf6;
}


void i386_fincstp() {
	*(mcodeptr++) = (u1) 0xd9;
	*(mcodeptr++) = (u1) 0xf7;
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
