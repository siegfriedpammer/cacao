/* jit/mips/codegen.c - machine code generator for mips

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
            Reinhard Grafl

   Contains the codegenerator for an MIPS (R4000 or higher) processor.
   This module generates MIPS machine code for a sequence of
   intermediate code commands (ICMDs).

   $Id: codegen.c 857 2004-01-06 16:32:59Z twisti $

*/


#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include "types.h"
#include "main.h"
#include "codegen.h"
#include "jit.h"
#include "reg.h"
#include "builtin.h"
#include "asmpart.h"
#include "jni.h"
#include "loader.h"
#include "tables.h"
#include "native.h"

/* include independent code generation stuff */
#include "codegen.inc"
#include "reg.inc"


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
 
int nregdescint[] = {
	REG_RES, REG_RES, REG_RET, REG_RES, REG_ARG, REG_ARG, REG_ARG, REG_ARG, 
	REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_TMP, REG_TMP, REG_TMP, REG_TMP, 
	REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV,
	REG_TMP, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES,
	REG_END };

/* for use of reserved registers, see comment above */
	
int nregdescfloat[] = {
	REG_RET, REG_RES, REG_RES, REG_RES, REG_TMP, REG_TMP, REG_TMP, REG_TMP,
	REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_ARG, REG_ARG, REG_ARG, REG_ARG, 
	REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_TMP, REG_TMP, REG_TMP, REG_TMP,
	REG_SAV, REG_TMP, REG_SAV, REG_TMP, REG_SAV, REG_TMP, REG_SAV, REG_TMP,
	REG_END };

/* for use of reserved registers, see comment above */


/* parameter allocation mode */

int nreg_parammode = PARAMMODE_NUMBERED;  

   /* parameter-registers will be allocated by assigning the
      1. parameter:   int/float-reg a0
      2. parameter:   int/float-reg a1  
      3. parameter:   int/float-reg a2 ....
   */


/* stackframe-infos ***********************************************************/

int parentargs_base; /* offset in stackframe for the parameter from the caller*/

/* -> see file 'calling.doc' */


/* additional functions and macros to generate code ***************************/

/* #define BlockPtrOfPC(pc)        block+block_index[pc] */
#define BlockPtrOfPC(pc)  ((basicblock *) iptr->target)


#ifdef STATISTICS
#define COUNT_SPILLS count_spills++
#else
#define COUNT_SPILLS
#endif


/* gen_nullptr_check(objreg) */

#ifdef SOFTNULLPTRCHECK
#define gen_nullptr_check(objreg) \
	if (checknull) {\
	M_BEQZ((objreg), 0);\
	codegen_addxnullrefs(mcodeptr);\
	M_NOP;\
	}
#else
#define gen_nullptr_check(objreg)
#endif


/* MCODECHECK(icnt) */

#define MCODECHECK(icnt) \
	if((mcodeptr+(icnt))>mcodeend)mcodeptr=codegen_increase((u1*)mcodeptr)

/* M_INTMOVE:
     generates an integer-move from register a to b.
     if a and b are the same int-register, no code will be generated.
*/ 

#define M_INTMOVE(a,b) if(a!=b){M_MOV(a,b);}


/* M_FLTMOVE:
    generates a floating-point-move from register a to b.
    if a and b are the same float-register, no code will be generated
*/ 

#define M_FLTMOVE(a,b) {if(a!=b){M_DMOV(a,b);}}

#define M_TFLTMOVE(t,a,b) \
	{if(a!=b) \
		if ((t)==TYPE_DBL) \
		    {M_DMOV(a,b);} \
		else {M_FMOV(a,b);} \
	}

#define M_TFLD(t,a,b,disp) \
    if ((t)==TYPE_DBL) \
	  {M_DLD(a,b,disp);} \
    else \
	  {M_FLD(a,b,disp);}

#define M_TFST(t,a,b,disp) \
    if ((t)==TYPE_DBL) \
	  {M_DST(a,b,disp);} \
    else \
	  {M_FST(a,b,disp);}

#define M_CCFLTMOVE(t1,t2,a,b) \
	if ((t1)==(t2)) \
	  {M_TFLTMOVE(t1,a,b);} \
	else \
	  if ((t1)==TYPE_DBL) \
		{M_CVTDF(a,b);} \
	  else \
		{M_CVTFD(a,b);}

#define M_CCFLD(t1,t2,a,b,disp) \
    if ((t1)==(t2)) \
	  {M_DLD(a,b,disp);} \
	else { \
	  M_DLD(REG_FTMP1,b,disp); \
	  if ((t1)==TYPE_DBL) \
	    {M_CVTDF(REG_FTMP1,a);} \
	  else \
	    {M_CVTFD(REG_FTMP1,a);} \
	}
	  
#define M_CCFST(t1,t2,a,b,disp) \
    if ((t1)==(t2)) \
	  {M_DST(a,b,disp);} \
	else { \
	  if ((t1)==TYPE_DBL) \
	    {M_CVTDF(a,REG_FTMP1);} \
	  else \
	    {M_CVTFD(a,REG_FTMP1);} \
	  M_DST(REG_FTMP1,b,disp); \
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

#define var_to_reg_int(regnr,v,tempnr) { \
	if ((v)->flags & INMEMORY) \
		{COUNT_SPILLS;M_LLD(tempnr,REG_SP,8*(v)->regoff);regnr=tempnr;} \
	else regnr=(v)->regoff; \
}


#define var_to_reg_flt(regnr,v,tempnr) { \
	if ((v)->flags & INMEMORY) \
		{COUNT_SPILLS;M_DLD(tempnr,REG_SP,8*(v)->regoff);regnr=tempnr;} \
	else regnr=(v)->regoff; \
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

#define store_reg_to_var_int(sptr, tempregnum) {       \
	if ((sptr)->flags & INMEMORY) {                    \
		COUNT_SPILLS;                                  \
		M_LST(tempregnum, REG_SP, 8 * (sptr)->regoff); \
		}                                              \
	}

#define store_reg_to_var_flt(sptr, tempregnum) {       \
	if ((sptr)->flags & INMEMORY) {                    \
		COUNT_SPILLS;                                  \
		M_DST(tempregnum, REG_SP, 8 * (sptr)->regoff); \
		}                                              \
	}


/* NullPointerException handlers and exception handling initialisation        */

/* NullPointerException signal handler for hardware null pointer check */

void catch_NullPointerException(int sig, int code, struct sigcontext *sigctx)
{
	sigset_t nsig;
	int      instr;
	long     faultaddr;

	/* Reset signal handler - necessary for SysV, does no harm for BSD */

	instr = *((int*)(sigctx->sc_pc));
	faultaddr = sigctx->sc_regs[(instr >> 21) & 0x1f];

	if (faultaddr == 0) {
		sigemptyset(&nsig);
		sigaddset(&nsig, sig);
		sigprocmask(SIG_UNBLOCK, &nsig, NULL);           /* unblock signal    */
		sigctx->sc_regs[REG_ITMP1_XPTR] =
		                            (long) proto_java_lang_NullPointerException;
		sigctx->sc_regs[REG_ITMP2_XPC] = sigctx->sc_pc;
		sigctx->sc_pc = (long) asm_handle_exception;
		}
	else {
        faultaddr += (long) ((instr << 16) >> 16);
		fprintf(stderr, "faulting address: 0x%lx at 0x%lx\n", (long) faultaddr, (long) sigctx->sc_pc);
		panic("Stack overflow");
		}
}


#include <sys/fpu.h>

void init_exceptions(void)
{
	struct sigaction sa;
	sigset_t unblockmask;

	createcalljava();
	
	/* The Boehm GC initialization blocks the SIGSEGV signal. So we do a
	   dummy allocation here to ensure that the GC is initialized.
	*/
	heap_allocate(1, 0, NULL);

	/* install signal handlers we need to convert to exceptions */

	sigemptyset(&unblockmask);
	sa.sa_flags = 0;
	sa.sa_sigaction = catch_NullPointerException;
	sigemptyset(&sa.sa_mask);

	if (!checknull) {

#if defined(SIGSEGV)
		sigaction(SIGSEGV, &sa, NULL);
		sigaddset(&unblockmask, SIGSEGV);
#endif

#if defined(SIGBUS)
		sigaction(SIGBUS, &sa, NULL);
		sigaddset(&unblockmask, SIGBUS);
#endif
	}

	sigprocmask(SIG_UNBLOCK, &unblockmask, NULL);

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

#define    	MethodPointer   -8
#define    	FrameSize       -12
#define     IsSync          -16
#define     IsLeaf          -20
#define     IntSave         -24
#define     FltSave         -28
#define     ExTableSize     -32
#define     ExTableStart    -32

#if POINTERSIZE==8
#define     ExEntrySize     -32
#define     ExStartPC       -8
#define     ExEndPC         -16
#define     ExHandlerPC     -24
#define     ExCatchType     -32
#else
#define     ExEntrySize     -16
#define     ExStartPC       -4
#define     ExEndPC         -8
#define     ExHandlerPC     -12
#define     ExCatchType     -16
#endif

void codegen()
{
	int  len, s1, s2, s3, d;
	s4   a;
	s4          *mcodeptr;
	stackptr    src;
	varinfo     *var;
	basicblock  *bptr;
	instruction *iptr;
	xtable *ex;

	{
	int p, pa, t, l, r;

	savedregs_num = (isleafmethod) ? 0 : 1;           /* space to save the RA */

	/* space to save used callee saved registers */

	savedregs_num += (savintregcnt - maxsavintreguse);
	savedregs_num += (savfltregcnt - maxsavfltreguse);

	parentargs_base = maxmemuse + savedregs_num;

#ifdef USE_THREADS                 /* space to save argument of monitor_enter */

	if (checksync && (method->flags & ACC_SYNCHRONIZED))
		parentargs_base++;

#endif

	/* adjust frame size for 16 byte alignment */

	if (parentargs_base & 1)
		parentargs_base++;

	/* create method header */

#if POINTERSIZE==4
	(void) dseg_addaddress(method);                         /* Filler         */
#endif
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
	
	mcodeptr = (s4*) mcodebase;
	mcodeend = (s4*) (mcodebase + mcodesize);
	MCODECHECK(128 + mparamcount);

	/* create stack frame (if necessary) */

	if (parentargs_base)
		{M_LDA (REG_SP, REG_SP, -parentargs_base * 8);}

	/* save return address and used callee saved registers */

	p = parentargs_base;
	if (!isleafmethod)
		{p--;  M_LST (REG_RA, REG_SP, 8*p);}
	for (r = savintregcnt - 1; r >= maxsavintreguse; r--)
		{p--; M_LST (savintregs[r], REG_SP, 8 * p);}
	for (r = savfltregcnt - 1; r >= maxsavfltreguse; r--)
		{p--; M_DST (savfltregs[r], REG_SP, 8 * p);}

	/* save monitorenter argument */

#ifdef USE_THREADS
	if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
		if (method->flags & ACC_STATIC) {
			p = dseg_addaddress (class);
			M_ALD(REG_ITMP1, REG_PV, p);
			M_AST(REG_ITMP1, REG_SP, 8 * maxmemuse);
			} 
		else {
			M_AST (argintregs[0], REG_SP, 8 * maxmemuse);
			}
		}			
#endif

	/* copy argument registers to stack and call trace function with pointer
	   to arguments on stack. ToDo: save floating point registers !!!!!!!!!
	*/

	if (runverbose) {
		M_LDA (REG_SP, REG_SP, -(18*8));

		M_LST(REG_RA,        REG_SP,  1*8);

		M_LST(argintregs[0], REG_SP,  2*8);
		M_LST(argintregs[1], REG_SP,  3*8);
		M_LST(argintregs[2], REG_SP,  4*8);
		M_LST(argintregs[3], REG_SP,  5*8);
		M_LST(argintregs[4], REG_SP,  6*8);
		M_LST(argintregs[5], REG_SP,  7*8);
		M_LST(argintregs[6], REG_SP,  8*8);
		M_LST(argintregs[7], REG_SP,  9*8);

		M_DST(argfltregs[0], REG_SP, 10*8);
		M_DST(argfltregs[1], REG_SP, 11*8);
		M_DST(argfltregs[2], REG_SP, 12*8);
		M_DST(argfltregs[3], REG_SP, 13*8);
		M_DST(argfltregs[4], REG_SP, 14*8);
		M_DST(argfltregs[5], REG_SP, 15*8);
		M_DST(argfltregs[6], REG_SP, 16*8);
		M_DST(argfltregs[7], REG_SP, 17*8);

		p = dseg_addaddress (method);
		M_ALD(REG_ITMP1, REG_PV, p);
		M_LST(REG_ITMP1, REG_SP, 0);
		p = dseg_addaddress ((void*) (builtin_trace_args));
		M_ALD(REG_ITMP3, REG_PV, p);
		M_JSR(REG_RA, REG_ITMP3);
		M_NOP;

		M_LLD(REG_RA,        REG_SP,  1*8);

		M_LLD(argintregs[0], REG_SP,  2*8);
		M_LLD(argintregs[1], REG_SP,  3*8);
		M_LLD(argintregs[2], REG_SP,  4*8);
		M_LLD(argintregs[3], REG_SP,  5*8);
		M_LLD(argintregs[4], REG_SP,  6*8);
		M_LLD(argintregs[5], REG_SP,  7*8);
		M_LLD(argintregs[6], REG_SP,  8*8);
		M_LLD(argintregs[7], REG_SP,  9*8);

		M_DLD(argfltregs[0], REG_SP, 10*8);
		M_DLD(argfltregs[1], REG_SP, 11*8);
		M_DLD(argfltregs[2], REG_SP, 12*8);
		M_DLD(argfltregs[3], REG_SP, 13*8);
		M_DLD(argfltregs[4], REG_SP, 14*8);
		M_DLD(argfltregs[5], REG_SP, 15*8);
		M_DLD(argfltregs[6], REG_SP, 16*8);
		M_DLD(argfltregs[7], REG_SP, 17*8);

		M_LDA (REG_SP, REG_SP, 18*8);
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
 			if (p < INT_ARG_CNT) {                   /* register arguments    */
 				if (!(var->flags & INMEMORY))        /* reg arg -> register   */
 					{M_INTMOVE (argintregs[p], r);}
 				else                                 /* reg arg -> spilled    */
 					M_LST (argintregs[p], REG_SP, 8 * r);
 				}
 			else {                                   /* stack arguments       */
 				pa = p - INT_ARG_CNT;
 				if (!(var->flags & INMEMORY))        /* stack arg -> register */ 
 					M_LLD (r, REG_SP, 8 * (parentargs_base + pa));
 				else {                               /* stack arg -> spilled  */
 					M_LLD (REG_ITMP1, REG_SP, 8 * (parentargs_base + pa));
 					M_LST (REG_ITMP1, REG_SP, 8 * r);
 					}
 				}
 			}
 		else {                                       /* floating args         */   
 			if (p < FLT_ARG_CNT) {                   /* register arguments    */
 				if (!(var->flags & INMEMORY))        /* reg arg -> register   */
 					{M_TFLTMOVE (var->type, argfltregs[p], r);}
 				else				                 /* reg arg -> spilled    */
 					M_DST (argfltregs[p], REG_SP, 8 * r);
 				}
 			else {                                   /* stack arguments       */
 				pa = p - FLT_ARG_CNT;
 				if (!(var->flags & INMEMORY)) {      /* stack-arg -> register */
 					M_DLD (r, REG_SP, 8 * (parentargs_base + pa) );
				} else {                             /* stack-arg -> spilled  */
 					M_DLD (REG_FTMP1, REG_SP, 8 * (parentargs_base + pa));
 					M_DST (REG_FTMP1, REG_SP, 8 * r);
 					}
 				}
 			}
		}  /* end for */

	/* call trace function */

#if 0
	if (runverbose && !isleafmethod) {
		M_LDA (REG_SP, REG_SP, -8);
		p = dseg_addaddress (method);
		M_ALD(REG_ITMP1, REG_PV, p);
		M_AST(REG_ITMP1, REG_SP, 0);
		p = dseg_addaddress ((void*) (builtin_trace_args));
		M_ALD(REG_ITMP3, REG_PV, p);
		M_JSR(REG_RA, REG_ITMP3);
		M_NOP;
		M_LDA(REG_SP, REG_SP, 8);
		}
#endif

	/* call monitorenter function */

#ifdef USE_THREADS
	if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
		int disp;
		p = dseg_addaddress ((void*) (builtin_monitorenter));
		M_ALD(REG_ITMP3, REG_PV, p);
		M_JSR(REG_RA, REG_ITMP3);
		M_ALD(argintregs[0], REG_SP, 8 * maxmemuse);
		disp = -(int)((u1*) mcodeptr - mcodebase);
		M_LDA(REG_PV, REG_RA, disp);
		}			
#endif
	}

	/* end of header generation */

	/* walk through all basic blocks */
	for (bptr = block; bptr != NULL; bptr = bptr->next) {

		bptr -> mpc = (int)((u1*) mcodeptr - mcodebase);

		if (bptr->flags >= BBREACHED) {

		/* branch resolving */

		{
		branchref *brefs;
		for (brefs = bptr->branchrefs; brefs != NULL; brefs = brefs->next) {
			gen_resolvebranch((u1*) mcodebase + brefs->branchpos, 
			                  brefs->branchpos, bptr->mpc);
			}
		}

		/* copy interface registers to their destination */

		src = bptr->instack;
		len = bptr->indepth;
		MCODECHECK(64+len);
		while (src != NULL) {
			len--;
			if ((len == 0) && (bptr->type != BBTYPE_STD)) {
				d = reg_of_var(src, REG_ITMP1);
				M_INTMOVE(REG_ITMP1, d);
				store_reg_to_var_int(src, d);
				}
			else {
				d = reg_of_var(src, REG_IFTMP);
				if ((src->varkind != STACKVAR)) {
					s2 = src->type;
					if (IS_FLT_DBL_TYPE(s2)) {
						if (!(interfaces[len][s2].flags & INMEMORY)) {
							s1 = interfaces[len][s2].regoff;
							M_TFLTMOVE(s2,s1,d);
							}
						else {
							M_DLD(d, REG_SP, 8 * interfaces[len][s2].regoff);
							}
						store_reg_to_var_flt(src, d);
						}
					else {
						if (!(interfaces[len][s2].flags & INMEMORY)) {
							s1 = interfaces[len][s2].regoff;
							M_INTMOVE(s1,d);
							}
						else {
							M_LLD(d, REG_SP, 8 * interfaces[len][s2].regoff);
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

			var_to_reg_int(s1, src, REG_ITMP1);
			M_BEQZ(s1, 0);
			codegen_addxnullrefs(mcodeptr);
			M_NOP;
			break;

		/* constant operations ************************************************/

#define ICONST(r,c) if(((c)>=-32768)&&((c)<= 32767)){M_IADD_IMM(REG_ZERO,c,r);} \
                    else if(((c)>=0)&&((c)<=0xffff)){M_OR_IMM(REG_ZERO,c,r);} \
                    else{a=dseg_adds4(c);M_ILD(r,REG_PV,a);}

#define LCONST(r,c) if(((c)>=-32768)&&((c)<= 32767)){M_LADD_IMM(REG_ZERO,c,r);} \
                    else if(((c)>=0)&&((c)<=0xffff)){M_OR_IMM(REG_ZERO,c,r);} \
                    else{a=dseg_adds8(c);M_LLD(r,REG_PV,a);}

		case ICMD_ICONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.i = constant                    */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			ICONST(d, iptr->val.i);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.l = constant                    */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			LCONST(d, iptr->val.l);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.f = constant                    */

			d = reg_of_var (iptr->dst, REG_FTMP1);
			a = dseg_addfloat (iptr->val.f);
			M_FLD(d, REG_PV, a);
			store_reg_to_var_flt (iptr->dst, d);
			break;
			
		case ICMD_DCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.d = constant                    */

			d = reg_of_var (iptr->dst, REG_FTMP1);
			a = dseg_adddouble (iptr->val.d);
			M_DLD(d, REG_PV, a);
			store_reg_to_var_flt (iptr->dst, d);
			break;

		case ICMD_ACONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.a = constant                    */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->val.a) {
				a = dseg_addaddress (iptr->val.a);
				M_ALD(d, REG_PV, a);
				}
			else {
				M_INTMOVE(REG_ZERO, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;


		/* load/store operations **********************************************/

		case ICMD_ILOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_LLOAD:      /* op1 = local variable                         */
		case ICMD_ALOAD:

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1))
				break;
			var = &(locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY)
				M_LLD(d, REG_SP, 8 * var->regoff);
			else
				{M_INTMOVE(var->regoff,d);}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FLOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_DLOAD:      /* op1 = local variable                         */

			d = reg_of_var(iptr->dst, REG_FTMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1))
				break;
			var = &(locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			{
				int t2 = ((iptr->opc == ICMD_FLOAD) ? TYPE_FLT : TYPE_DBL);
				if (var->flags & INMEMORY)
					{M_CCFLD(var->type,t2,d, REG_SP, 8 * var->regoff);}
				else
					{M_CCFLTMOVE(var->type,t2,var->regoff,d);}
			}
			store_reg_to_var_flt(iptr->dst, d);
			break;


		case ICMD_ISTORE:     /* ..., value  ==> ...                          */
		case ICMD_LSTORE:     /* op1 = local variable                         */
		case ICMD_ASTORE:

			if ((src->varkind == LOCALVAR) &&
			    (src->varnum == iptr->op1))
				break;
			var = &(locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
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
			var = &(locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			{
				int t1 = ((iptr->opc == ICMD_FSTORE) ? TYPE_FLT : TYPE_DBL);
				if (var->flags & INMEMORY) {
					var_to_reg_flt(s1, src, REG_FTMP1);
					M_CCFST(t1,var->type,s1, REG_SP, 8 * var->regoff);
					}
				else {
					var_to_reg_flt(s1, src, var->regoff);
					M_CCFLTMOVE(t1,var->type,s1, var->regoff);
					}
			}
			break;


		/* pop/dup/swap operations ********************************************/

		/* attention: double and longs are only one entry in CACAO ICMDs      */

		case ICMD_POP:        /* ..., value  ==> ...                          */
		case ICMD_POP2:       /* ..., value, value  ==> ...                   */
			break;

#define M_COPY(from,to) \
			d = reg_of_var(to, REG_IFTMP); \
			if ((from->regoff != to->regoff) || \
			    ((from->flags ^ to->flags) & INMEMORY)) { \
				if (IS_FLT_DBL_TYPE(from->type)) { \
					var_to_reg_flt(s1, from, d); \
					M_TFLTMOVE(from->type,s1,d); \
					store_reg_to_var_flt(to, d); \
					}\
				else { \
					var_to_reg_int(s1, from, d); \
					M_INTMOVE(s1,d); \
					store_reg_to_var_int(to, d); \
					}\
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

			var_to_reg_int(s1, src, REG_ITMP1); 
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_ISUB(REG_ZERO, s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_LSUB(REG_ZERO, s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_I2L:        /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_ISLL_IMM(s1, 0, d );
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_LSLL_IMM(s1, 56, d);
			M_LSRA_IMM( d, 56, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2CHAR:   /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
            M_CZEXT(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2SHORT:  /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_LSLL_IMM(s1, 48, d);
			M_LSRA_IMM( d, 48, d);
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_IADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_IADD(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_LADD(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_ISUB(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_LSUB(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_IMUL(s1, s2);
			M_MFLO(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_LMUL(s1, s2);
			M_MFLO(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_LSRA_IMM(s1, 63, REG_ITMP2);
			M_LSRL_IMM(REG_ITMP2, 64 - iptr->val.i, REG_ITMP2);
			M_LADD(s1, REG_ITMP2, REG_ITMP2);
			M_LSRA_IMM(REG_ITMP2, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_ISLL(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_ISLL_IMM(s1, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_ISRA(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_ISRA_IMM(s1, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_ISRL(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_ISRL_IMM(s1, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_LSLL(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_LSLL_IMM(s1, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_LSRA(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_LSRA_IMM(s1, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_LSRL(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_LSRL_IMM(s1, iptr->val.i, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */
		case ICMD_LAND:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_AND(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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

		case ICMD_IREM0X10001:  /* ..., value  ==> ..., value % 0x100001      */
		
/*          b = value & 0xffff;
			a = value >> 16;
			a = ((b - a) & 0xffff) + (b < a);
*/
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			if (s1 == d) {
				M_MOV(s1, REG_ITMP3);
				s1 = REG_ITMP3;
				}
			M_BLTZ(s1, 7);
            M_CZEXT(s1, REG_ITMP2);                             /* delay slot */
			M_ISRA_IMM(s1, 16, d);
			M_CMPLT(REG_ITMP2, d, REG_ITMP1);
			M_ISUB(REG_ITMP2, d, d);
            M_CZEXT(d, d);
			M_BR(7);
			M_IADD(d, REG_ITMP1, d);                            /* delay slot */

			M_LUI(REG_ITMP2, 1);
			M_IADD_IMM(REG_ITMP2, 1, REG_ITMP2);
			M_IDIV(s1, REG_ITMP2);
			M_MFHI(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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

		case ICMD_LREM0X10001:/* ..., value  ==> ..., value % 0x10001         */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_LUI(REG_ITMP2, 1);
			M_LADD_IMM(REG_ITMP2, 1, REG_ITMP2);
			M_LDIV(s1, REG_ITMP2);
			M_MFHI(d);
			M_NOP;
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IOR:        /* ..., val1, val2  ==> ..., val1 | val2        */
		case ICMD_LOR:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_OR( s1,s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_XOR(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_CMPLT(s1, s2, REG_ITMP3);
			M_CMPLT(s2, s1, REG_ITMP1);
			M_LSUB (REG_ITMP1, REG_ITMP3, d);
			store_reg_to_var_int(iptr->dst, d);
			break;


		case ICMD_IINC:       /* ..., value  ==> ..., value + constant        */
		                      /* op1 = variable, val.i = constant             */

			var = &(locals[iptr->op1][TYPE_INT]);
			if (var->flags & INMEMORY) {
				s1 = REG_ITMP1;
				M_LLD(s1, REG_SP, 8 * var->regoff);
				}
			else
				s1 = var->regoff;
			M_IADD_IMM(s1, iptr->val.i, s1);
			if (var->flags & INMEMORY)
				M_LST(s1, REG_SP, 8 * var->regoff);
			break;


		/* floating operations ************************************************/

		case ICMD_FNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			M_FNEG(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			M_DNEG(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			M_FADD(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			M_DADD(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			M_FSUB(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			M_DSUB(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			M_FMUL(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DMUL:       /* ..., val1, val2  ==> ..., val1 *** val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			M_DMUL(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			M_FDIV(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			M_DDIV(s1, s2, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
		
		case ICMD_FREM:       /* ..., val1, val2  ==> ..., val1 % val2        */
			panic("FREM");

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_FTMP3);
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
			d = reg_of_var(iptr->dst, REG_FTMP3);
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
			d = reg_of_var(iptr->dst, REG_FTMP3);
			M_MOVLD(s1, d);
			M_CVTLF(d, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_I2D:       /* ..., value  ==> ..., (double) value           */
		case ICMD_L2D:
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			M_MOVLD(s1, d);
			M_CVTLD(d, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
			
		case ICMD_F2I:       /* ..., (float) value  ==> ..., (int) value      */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_TRUNCFI(s1, REG_FTMP1);
			M_MOVDI(REG_FTMP1, d);
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;
		
		case ICMD_D2I:       /* ..., (double) value  ==> ..., (int) value     */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_TRUNCDI(s1, REG_FTMP1);
			M_MOVDI(REG_FTMP1, d);
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;
		
		case ICMD_F2L:       /* ..., (float) value  ==> ..., (long) value     */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_TRUNCFL(s1, REG_FTMP1);
			M_MOVDL(REG_FTMP1, d);
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_D2L:       /* ..., (double) value  ==> ..., (long) value    */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_TRUNCDL(s1, REG_FTMP1);
			M_MOVDL(REG_FTMP1, d);
			M_NOP;
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_F2D:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			M_CVTFD(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
					
		case ICMD_D2F:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			M_CVTDF(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
		
		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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

#define gen_bound_check \
			if (checkbounds) {\
				M_ILD(REG_ITMP3, s1, OFFSET(java_arrayheader, size));\
				M_CMPULT(s2, REG_ITMP3, REG_ITMP3);\
				M_BEQZ(REG_ITMP3, 0);\
				codegen_addxboundrefs(mcodeptr);\
				M_NOP;\
				}

		case ICMD_ARRAYLENGTH: /* ..., arrayref  ==> ..., length              */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			gen_nullptr_check(s1);
			M_ILD(d, s1, OFFSET(java_arrayheader, size));
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
			M_ASLL_IMM(s2, POINTERSHIFT, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_ALD(d, REG_ITMP1, OFFSET(java_objectarray, data[0]));
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
			M_ASLL_IMM(s2, 2, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_ILD(d, REG_ITMP1, OFFSET(java_intarray, data[0]));
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
			M_ASLL_IMM(s2, 3, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_LLD(d, REG_ITMP1, OFFSET(java_longarray, data[0]));
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
			M_ASLL_IMM(s2, 2, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_FLD(d, REG_ITMP1, OFFSET(java_floatarray, data[0]));
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
			M_ASLL_IMM(s2, 3, REG_ITMP2);
			M_AADD(REG_ITMP2, s1, REG_ITMP1);
			M_DLD(d, REG_ITMP1, OFFSET(java_doublearray, data[0]));
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
			M_AADD(s2, s1, REG_ITMP1);
			M_AADD(s2, REG_ITMP1, REG_ITMP1);
			M_SLDU(d, REG_ITMP1, OFFSET(java_chararray, data[0]));
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
			M_AADD(s2, s1, REG_ITMP1);
			M_AADD(s2, REG_ITMP1, REG_ITMP1);
			M_SLDS(d, REG_ITMP1, OFFSET(java_chararray, data[0]));
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


		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */
		                      /* op1 = type, val.a = field address            */

			/* if class isn't yet initialized, do it */
  			if (!((fieldinfo *) iptr->val.a)->class->initialized) {
				/* call helper function which patches this code */
				a = dseg_addaddress(((fieldinfo *) iptr->val.a)->class);
				M_ALD(REG_ITMP1, REG_PV, a);
				a = dseg_addaddress(asm_check_clinit);
				M_ALD(REG_ITMP3, REG_PV, a);
				M_JSR(REG_RA, REG_ITMP3);
				M_NOP;
  			}

			a = dseg_addaddress(&(((fieldinfo *) iptr->val.a)->value));
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
			default: panic ("internal error");
			}
			break;

		case ICMD_GETSTATIC:  /* ...  ==> ..., value                          */
		                      /* op1 = type, val.a = field address            */

			/* if class isn't yet initialized, do it */
  			if (!((fieldinfo *) iptr->val.a)->class->initialized) {
				/* call helper function which patches this code */
				a = dseg_addaddress(((fieldinfo *) iptr->val.a)->class);
				M_ALD(REG_ITMP1, REG_PV, a);
				a = dseg_addaddress(asm_check_clinit);
				M_ALD(REG_ITMP3, REG_PV, a);
				M_JSR(REG_RA, REG_ITMP3);
				M_NOP;
  			}

			a = dseg_addaddress(&(((fieldinfo *) iptr->val.a)->value));
			M_ALD(REG_ITMP1, REG_PV, a);
			switch (iptr->op1) {
			case TYPE_INT:
				d = reg_of_var(iptr->dst, REG_ITMP3);
				M_ILD(d, REG_ITMP1, 0);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_LNG:
				d = reg_of_var(iptr->dst, REG_ITMP3);
				M_LLD(d, REG_ITMP1, 0);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_ADR:
				d = reg_of_var(iptr->dst, REG_ITMP3);
				M_ALD(d, REG_ITMP1, 0);
				store_reg_to_var_int(iptr->dst, d);
				break;
			case TYPE_FLT:
				d = reg_of_var(iptr->dst, REG_FTMP1);
				M_FLD(d, REG_ITMP1, 0);
				store_reg_to_var_flt(iptr->dst, d);
				break;
			case TYPE_DBL:				
				d = reg_of_var(iptr->dst, REG_FTMP1);
				M_DLD(d, REG_ITMP1, 0);
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
				default: panic ("internal error");
				}
			break;

		case ICMD_GETFIELD:   /* ...  ==> ..., value                          */
		                      /* op1 = type, val.i = field offset             */

			a = ((fieldinfo *)(iptr->val.a))->offset;
			switch (iptr->op1) {
				case TYPE_INT:
					var_to_reg_int(s1, src, REG_ITMP1);
					d = reg_of_var(iptr->dst, REG_ITMP3);
					gen_nullptr_check(s1);
					M_ILD(d, s1, a);
					store_reg_to_var_int(iptr->dst, d);
					break;
				case TYPE_LNG:
					var_to_reg_int(s1, src, REG_ITMP1);
					d = reg_of_var(iptr->dst, REG_ITMP3);
					gen_nullptr_check(s1);
					M_LLD(d, s1, a);
					store_reg_to_var_int(iptr->dst, d);
					break;
				case TYPE_ADR:
					var_to_reg_int(s1, src, REG_ITMP1);
					d = reg_of_var(iptr->dst, REG_ITMP3);
					gen_nullptr_check(s1);
					M_ALD(d, s1, a);
					store_reg_to_var_int(iptr->dst, d);
					break;
				case TYPE_FLT:
					var_to_reg_int(s1, src, REG_ITMP1);
					d = reg_of_var(iptr->dst, REG_FTMP1);
					gen_nullptr_check(s1);
					M_FLD(d, s1, a);
					store_reg_to_var_flt(iptr->dst, d);
					break;
				case TYPE_DBL:				
					var_to_reg_int(s1, src, REG_ITMP1);
					d = reg_of_var(iptr->dst, REG_FTMP1);
					gen_nullptr_check(s1);
					M_DLD(d, s1, a);
					store_reg_to_var_flt(iptr->dst, d);
					break;
				default: panic ("internal error");
				}
			break;


		/* branch operations **************************************************/

#define ALIGNCODENOP {if((int)((long)mcodeptr&7)){M_NOP;}}

		case ICMD_ATHROW:       /* ..., objectref ==> ... (, objectref)       */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_INTMOVE(s1, REG_ITMP1_XPTR);
			a = dseg_addaddress(asm_handle_exception);
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
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			ALIGNCODENOP;
			break;

		case ICMD_JSR:          /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */

			dseg_addtarget(BlockPtrOfPC(iptr->op1));
			M_ALD(REG_ITMP1, REG_PV, -dseglen);
			M_JSR(REG_ITMP1, REG_ITMP1);        /* REG_ITMP1 = return address */
			M_NOP;
			break;
			
		case ICMD_RET:          /* ... ==> ...                                */
		                        /* op1 = local variable                       */
			var = &(locals[iptr->op1][TYPE_ADR]);
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
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IFNONNULL:    /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_BNEZ(s1, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_ICMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPEQ:    /* op1 = target JavaVM pc                     */
		case ICMD_IF_ACMPEQ:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_BEQ(s1, s2, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_ICMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPNE:    /* op1 = target JavaVM pc                     */
		case ICMD_IF_ACMPNE:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_BNE(s1, s2, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_ICMPLT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPLT:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPLT(s1, s2, REG_ITMP1);
			M_BNEZ(REG_ITMP1, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_ICMPGT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPGT:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPGT(s1, s2, REG_ITMP1);
			M_BNEZ(REG_ITMP1, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_ICMPLE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPLE:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPGT(s1, s2, REG_ITMP1);
			M_BEQZ(REG_ITMP1, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

		case ICMD_IF_ICMPGE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPGE:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPLT(s1, s2, REG_ITMP1);
			M_BEQZ(REG_ITMP1, 0);
			codegen_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			M_NOP;
			break;

#ifdef CONDITIONAL_LOADCONST
		/* (value xx 0) ? IFxx_ICONST : ELSE_ICONST                           */

		case ICMD_ELSE_ICONST:  /* handled by IFxx_ICONST                     */
			break;

		case ICMD_IFEQ_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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

#ifdef USE_THREADS
			if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
				int disp;
				a = dseg_addaddress ((void*) (builtin_monitorexit));
				M_ALD(REG_ITMP3, REG_PV, a);
				M_JSR(REG_RA, REG_ITMP3);
				M_ALD(argintregs[0], REG_SP, 8 * maxmemuse);    /* delay slot */
				disp = -(int)((u1*) mcodeptr - mcodebase);
				M_LDA(REG_PV, REG_RA, disp);
				}			
#endif
			var_to_reg_int(s1, src, REG_RESULT);
			M_INTMOVE(s1, REG_RESULT);
			goto nowperformreturn;

		case ICMD_FRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_DRETURN:

#ifdef USE_THREADS
			if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
				int disp;
				a = dseg_addaddress ((void*) (builtin_monitorexit));
				M_ALD(REG_ITMP3, REG_PV, a);
				M_JSR(REG_RA, REG_ITMP3);
				M_ALD(argintregs[0], REG_SP, 8 * maxmemuse);    /* delay slot */
				disp = -(int)((u1*) mcodeptr - mcodebase);
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

#ifdef USE_THREADS
			if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
				int disp;
				a = dseg_addaddress ((void*) (builtin_monitorexit));
				M_ALD(REG_ITMP3, REG_PV, a);
				M_JSR(REG_RA, REG_ITMP3);
				M_ALD(argintregs[0], REG_SP, 8 * maxmemuse);    /* delay slot */
				disp = -(int)((u1*) mcodeptr - mcodebase);
				M_LDA(REG_PV, REG_RA, disp);
				}			
#endif

nowperformreturn:
			{
			int r, p;
			
			p = parentargs_base;
			
			/* restore return address                                         */

			if (!isleafmethod)
				{p--;  M_LLD (REG_RA, REG_SP, 8 * p);}

			/* restore saved registers                                        */

			for (r = savintregcnt - 1; r >= maxsavintreguse; r--)
					{p--; M_LLD(savintregs[r], REG_SP, 8 * p);}
			for (r = savfltregcnt - 1; r >= maxsavfltreguse; r--)
					{p--; M_DLD(savfltregs[r], REG_SP, 8 * p);}

			/* call trace function */

			if (runverbose) {
				M_LDA (REG_SP, REG_SP, -24);
				M_LST(REG_RA, REG_SP, 0);
				M_LST(REG_RESULT, REG_SP, 8);
				M_DST(REG_FRESULT, REG_SP,16);
				a = dseg_addaddress (method);
				M_ALD(argintregs[0], REG_PV, a);
				M_MOV(REG_RESULT, argintregs[1]);
				M_FLTMOVE(REG_FRESULT, argfltregs[2]);
				M_FMOV(REG_FRESULT, argfltregs[3]);
				a = dseg_addaddress ((void*) (builtin_displaymethodstop));
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

			if (parentargs_base)
				{M_LDA(REG_SP, REG_SP, parentargs_base*8);}
			else
				{M_NOP;}
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
			codegen_addreference((basicblock *) tptr[0], mcodeptr);
			M_ASLL_IMM(REG_ITMP1, POINTERSHIFT, REG_ITMP1);      /* delay slot*/

			/* build jump table top down and use address of lowest entry */

			/* s4ptr += 3 + i; */
			tptr += i;

			while (--i >= 0) {
				/* dseg_addtarget(BlockPtrOfPC(*--s4ptr)); */
				dseg_addtarget((basicblock *) tptr[0]); 
				--tptr;
				}
			}

			/* length of dataseg after last dseg_addtarget is used by load */

			M_AADD(REG_ITMP1, REG_PV, REG_ITMP2);
			M_ALD(REG_ITMP2, REG_ITMP2, -dseglen);
			M_JMP(REG_ITMP2);
			M_NOP;
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
				ICONST(REG_ITMP2, val);
				M_BEQ(s1, REG_ITMP2, 0);
				codegen_addreference((basicblock *) tptr[0], mcodeptr); 
				M_NOP;
				}

			M_BR(0);
			tptr = (void **) iptr->target;
			codegen_addreference((basicblock *) tptr[0], mcodeptr);
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
			methodinfo   *m;
			classinfo    *ci;

			MCODECHECK((s3 << 1) + 64);

			/* copy arguments to registers or stack location                  */

			for (; --s3 >= 0; src = src->prev) {
				if (src->varkind == ARGVAR)
					continue;
				if (IS_INT_LNG_TYPE(src->type)) {
					if (s3 < INT_ARG_CNT) {
						s1 = argintregs[s3];
						var_to_reg_int(d, src, s1);
						M_INTMOVE(d, s1);
						}
					else  {
						var_to_reg_int(d, src, REG_ITMP1);
						M_LST(d, REG_SP, 8 * (s3 - INT_ARG_CNT));
						}
					}
				else
					if (s3 < FLT_ARG_CNT) {
						s1 = argfltregs[s3];
						var_to_reg_flt(d, src, s1);
						M_TFLTMOVE(src->type,d, s1);
						}
					else {
						var_to_reg_flt(d, src, REG_FTMP1);
						M_DST(d, REG_SP, 8 * (s3 - FLT_ARG_CNT));
						}
				} /* end of for */

			m = iptr->val.a;
			switch (iptr->opc) {
				case ICMD_BUILTIN3:
				case ICMD_BUILTIN2:
				case ICMD_BUILTIN1:
					a = dseg_addaddress ((void*) (m));
					M_ALD(REG_ITMP3, REG_PV, a); /* built-in-function pointer */
					M_JSR (REG_RA, REG_ITMP3);
					M_NOP;
					d = iptr->op1;                             /* return type */
					goto afteractualcall;

				case ICMD_INVOKESTATIC:
				case ICMD_INVOKESPECIAL:
					a = dseg_addaddress (m->stubroutine);

					M_ALD(REG_PV, REG_PV, a );        /* method pointer in pv */

					d = m->returntype;
					goto makeactualcall;

				case ICMD_INVOKEVIRTUAL:

					gen_nullptr_check(argintregs[0]);
					M_ALD(REG_METHODPTR, argintregs[0],
					                         OFFSET(java_objectheader, vftbl));
					M_ALD(REG_PV, REG_METHODPTR, OFFSET(vftbl, table[0]) +
					                        sizeof(methodptr) * m->vftblindex);

					d = m->returntype;
					goto makeactualcall;

				case ICMD_INVOKEINTERFACE:
					ci = m->class;
					
					gen_nullptr_check(argintregs[0]);
					M_ALD(REG_METHODPTR, argintregs[0],
					                         OFFSET(java_objectheader, vftbl));    
					M_ALD(REG_METHODPTR, REG_METHODPTR,
					      OFFSET(vftbl, interfacetable[0]) -
					      sizeof(methodptr*) * ci->index);
					M_ALD(REG_PV, REG_METHODPTR,
					                    sizeof(methodptr) * (m - ci->methods));

					d = m->returntype;
					goto makeactualcall;

				default:
					d = 0;
					error ("Unkown ICMD-Command: %d", iptr->opc);
				}

makeactualcall:

			M_JSR (REG_RA, REG_PV);
			M_NOP;

			/* recompute pv */

afteractualcall:

			s1 = (int)((u1*) mcodeptr - mcodebase);
			if (s1<=32768) M_LDA (REG_PV, REG_RA, -s1);
			else {
					s4 ml=-s1, mh=0;
					while (ml<-32768) { ml+=65536; mh--; }
					M_LUI(REG_PV, mh);
					M_IADD_IMM(REG_PV, ml, REG_PV);
					M_LADD(REG_PV, REG_RA, REG_PV);
				}

			/* d contains return type */

			if (d != TYPE_VOID) {
				if (IS_INT_LNG_TYPE(iptr->dst->type)) {
					s1 = reg_of_var(iptr->dst, REG_RESULT);
					M_INTMOVE(REG_RESULT, s1);
					store_reg_to_var_int(iptr->dst, s1);
					}
				else {
					s1 = reg_of_var(iptr->dst, REG_FRESULT);
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
			
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
					M_ILD(REG_ITMP2, REG_ITMP1, OFFSET(vftbl, interfacetablelength));
					M_IADD_IMM(REG_ITMP2, - super->index, REG_ITMP2);
					M_BLEZ(REG_ITMP2, 3);
					M_NOP;
					M_ALD(REG_ITMP1, REG_ITMP1,
					      OFFSET(vftbl, interfacetable[0]) -
					      super->index * sizeof(methodptr*));
					M_CMPULT(REG_ZERO, REG_ITMP1, d);      /* REG_ITMP1 != 0  */
					}
				else {                                     /* class           */
					/*
					s2 = super->vftbl->diffval;
					M_BEQZ(s1, 5);
					M_NOP;
					M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
					M_ILD(REG_ITMP1, REG_ITMP1, OFFSET(vftbl, baseval));
					M_IADD_IMM(REG_ITMP1, - super->vftbl->baseval, REG_ITMP1);
					M_CMPULT_IMM(REG_ITMP1, s2 + 1, d);
					*/

					M_BEQZ(s1, 9);
					M_NOP;
					M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
                    a = dseg_addaddress ((void*) super->vftbl);
                    M_ALD(REG_ITMP2, REG_PV, a);
                    M_ILD(REG_ITMP1, REG_ITMP1, OFFSET(vftbl, baseval));
                    M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl, baseval));
                    M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl, diffval));
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
			
			d = reg_of_var(iptr->dst, REG_ITMP3);
			var_to_reg_int(s1, src, d);
			if (iptr->op1) {                               /* class/interface */
				if (super->flags & ACC_INTERFACE) {        /* interface       */
					M_BEQZ(s1, 9);
					M_NOP;
					M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
					M_ILD(REG_ITMP2, REG_ITMP1, OFFSET(vftbl, interfacetablelength));
					M_IADD_IMM(REG_ITMP2, - super->index, REG_ITMP2);
					M_BLEZ(REG_ITMP2, 0);
					codegen_addxcastrefs(mcodeptr);
					M_NOP;
					M_ALD(REG_ITMP2, REG_ITMP1,
					      OFFSET(vftbl, interfacetable[0]) -
					      super->index * sizeof(methodptr*));
					M_BEQZ(REG_ITMP2, 0);
					codegen_addxcastrefs(mcodeptr);
					M_NOP;
					}
				else {                                     /* class           */

					/*
					s2 = super->vftbl->diffval;
					M_BEQZ(s1, 6 + (s2 != 0));
					M_NOP;
					M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
					M_ILD(REG_ITMP1, REG_ITMP1, OFFSET(vftbl, baseval));
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
                    a = dseg_addaddress ((void*) super->vftbl);
                    M_ALD(REG_ITMP2, REG_PV, a);
                    M_ILD(REG_ITMP1, REG_ITMP1, OFFSET(vftbl, baseval));
					if (d != REG_ITMP3) {
						M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl, baseval));
						M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl, diffval));
						M_ISUB(REG_ITMP1, REG_ITMP3, REG_ITMP1); 
					} else {
						M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl, baseval));
						M_ISUB(REG_ITMP1, REG_ITMP2, REG_ITMP1); 
						M_ALD(REG_ITMP2, REG_PV, a);
						M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl, diffval));
					}
                    M_CMPULT(REG_ITMP2, REG_ITMP1, REG_ITMP2);
					M_BNEZ(REG_ITMP2, 0);

					codegen_addxcastrefs(mcodeptr);
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
			codegen_addxcheckarefs(mcodeptr);
			M_NOP;
			break;

		case ICMD_MULTIANEWARRAY:/* ..., cnt1, [cnt2, ...] ==> ..., arrayref  */
		                      /* op1 = dimension, val.a = array descriptor    */

			/* check for negative sizes and copy sizes to stack if necessary  */

			MCODECHECK((iptr->op1 << 1) + 64);

			for (s1 = iptr->op1; --s1 >= 0; src = src->prev) {
				var_to_reg_int(s2, src, REG_ITMP1);
				M_BLTZ(s2, 0);
				codegen_addxcheckarefs(mcodeptr);
				M_NOP;

				/* copy sizes to stack (argument numbers >= INT_ARG_CNT)      */

				if (src->varkind != ARGVAR) {
					M_LST(s2, REG_SP, 8 * (s1 + INT_ARG_CNT));
					}
				}

			/* a0 = dimension count */

			ICONST(argintregs[0], iptr->op1);

			/* a1 = arraydescriptor */

			a = dseg_addaddress(iptr->val.a);
			M_ALD(argintregs[1], REG_PV, a);

			/* a2 = pointer to dimensions = stack pointer */

			M_INTMOVE(REG_SP, argintregs[2]);

			a = dseg_addaddress((void*) (builtin_nmultianewarray));
			M_ALD(REG_ITMP3, REG_PV, a);
			M_JSR(REG_RA, REG_ITMP3);
			M_NOP;
			s1 = (int)((u1*) mcodeptr - mcodebase);
			if (s1 <= 32768)
				M_LDA (REG_PV, REG_RA, -s1);
			else {
				panic("To big");
			    }
			s1 = reg_of_var(iptr->dst, REG_RESULT);
			M_INTMOVE(REG_RESULT, s1);
			store_reg_to_var_int(iptr->dst, s1);
			break;


		default: error ("Unknown pseudo command: %d", iptr->opc);
	
   

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
					M_TFLTMOVE(s2,s1,interfaces[len][s2].regoff);
					}
				else {
					M_DST(s1, REG_SP, 8 * interfaces[len][s2].regoff);
					}
				}
			else {
				var_to_reg_int(s1, src, REG_ITMP1);
				if (!(interfaces[len][s2].flags & INMEMORY)) {
					M_INTMOVE(s1,interfaces[len][s2].regoff);
					}
				else {
					M_LST(s1, REG_SP, 8 * interfaces[len][s2].regoff);
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

	s4 *xcodeptr = NULL;
	
	for (; xboundrefs != NULL; xboundrefs = xboundrefs->next) {
		if ((exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1*) mcodebase + xboundrefs->branchpos, 
				xboundrefs->branchpos, (u1*) xcodeptr - (u1*) mcodebase - 4);
			continue;
			}


		gen_resolvebranch((u1*) mcodebase + xboundrefs->branchpos, 
		                  xboundrefs->branchpos, (u1*) mcodeptr - mcodebase);

		MCODECHECK(8);

		M_LDA(REG_ITMP2_XPC, REG_PV, xboundrefs->branchpos - 4);

		if (xcodeptr != NULL) {
			int disp = xcodeptr-mcodeptr;
			M_BR(disp-1);
			M_NOP;
			}
		else {
			xcodeptr = mcodeptr;

			a = dseg_addaddress(asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);

			M_JMP(REG_ITMP3);
			a = dseg_addaddress(proto_java_lang_ArrayIndexOutOfBoundsException);
			M_ALD(REG_ITMP1_XPTR, REG_PV, a);
			}
		}

	/* generate negative array size check stubs */

	xcodeptr = NULL;
	
	for (; xcheckarefs != NULL; xcheckarefs = xcheckarefs->next) {
		if ((exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1*) mcodebase + xcheckarefs->branchpos, 
				xcheckarefs->branchpos, (u1*) xcodeptr - (u1*) mcodebase - 4);
			continue;
			}

		gen_resolvebranch((u1*) mcodebase + xcheckarefs->branchpos, 
		                  xcheckarefs->branchpos, (u1*) mcodeptr - mcodebase);

		MCODECHECK(8);

		M_LDA(REG_ITMP2_XPC, REG_PV, xcheckarefs->branchpos - 4);

		if (xcodeptr != NULL) {
			int disp = xcodeptr-mcodeptr;
			M_BR(disp-1);
			M_NOP;
			}
		else {
			xcodeptr = mcodeptr;

			a = dseg_addaddress(asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);

			M_JMP(REG_ITMP3);
			a = dseg_addaddress(proto_java_lang_NegativeArraySizeException);
			M_ALD(REG_ITMP1_XPTR, REG_PV, a);
			}
		}

	/* generate cast check stubs */

	xcodeptr = NULL;
	
	for (; xcastrefs != NULL; xcastrefs = xcastrefs->next) {
		if ((exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1*) mcodebase + xcastrefs->branchpos, 
				xcastrefs->branchpos, (u1*) xcodeptr - (u1*) mcodebase - 4);
			continue;
			}

		gen_resolvebranch((u1*) mcodebase + xcastrefs->branchpos, 
		                  xcastrefs->branchpos, (u1*) mcodeptr - mcodebase);

		MCODECHECK(8);

		M_LDA(REG_ITMP2_XPC, REG_PV, xcastrefs->branchpos - 4);

		if (xcodeptr != NULL) {
			int disp = xcodeptr-mcodeptr;
			M_BR(disp-1);
			M_NOP;
			}
		else {
			xcodeptr = mcodeptr;

			a = dseg_addaddress(asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);

			M_JMP(REG_ITMP3);
			a = dseg_addaddress(proto_java_lang_ClassCastException);
			M_ALD(REG_ITMP1_XPTR, REG_PV, a);
			}
		}


#ifdef SOFTNULLPTRCHECK

	/* generate null pointer check stubs */

	xcodeptr = NULL;

	for (; xnullrefs != NULL; xnullrefs = xnullrefs->next) {
		if ((exceptiontablelength == 0) && (xcodeptr != NULL)) {
			gen_resolvebranch((u1*) mcodebase + xnullrefs->branchpos, 
				xnullrefs->branchpos, (u1*) xcodeptr - (u1*) mcodebase - 4);
			continue;
			}

		gen_resolvebranch((u1*) mcodebase + xnullrefs->branchpos, 
		                  xnullrefs->branchpos, (u1*) mcodeptr - mcodebase);

		MCODECHECK(8);

		M_LDA(REG_ITMP2_XPC, REG_PV, xnullrefs->branchpos - 4);

		if (xcodeptr != NULL) {
			int disp = xcodeptr-mcodeptr;
			M_BR(disp-1);
			M_NOP;
			}
		else {
			xcodeptr = mcodeptr;

			a = dseg_addaddress(asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);

			M_JMP(REG_ITMP3);
			a = dseg_addaddress(proto_java_lang_NullPointerException);
			M_ALD(REG_ITMP1_XPTR, REG_PV, a);
			}
		}

#endif
	}

	codegen_finish((int)((u1*) mcodeptr - mcodebase));

	docacheflush((void*) method->entrypoint,
	                  ((u1*) mcodeptr - mcodebase));
}


/* function createcompilerstub *************************************************

	creates a stub routine which calls the compiler
	
*******************************************************************************/

#define COMPSTUBSIZE 4

u1 *createcompilerstub(methodinfo *m)
{
	u8 *s = CNEW(u8, COMPSTUBSIZE);     /* memory to hold the stub            */
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

#ifdef STATISTICS
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

#define NATIVESTUBSIZE      60
#define NATIVESTUBOFFSET    9

u1 *createnativestub(functionptr f, methodinfo *m)
{
	u8 *s = CNEW(u8, NATIVESTUBSIZE);   /* memory to hold the stub            */
	u8 *cs = s + NATIVESTUBOFFSET;
	s4 *mcodeptr = (s4 *) (cs);         /* code generation pointer            */

	*(cs-1) = (u8) f;                   /* address of native method           */
	*(cs-2) = (u8) (&exceptionptr);     /* address of exceptionptr            */
	*(cs-3) = (u8) asm_handle_nat_exception;/* addr of asm exception handler  */
	*(cs-4) = (u8) (&env);              /* addr of jni_environement           */
 	*(cs-5) = (u8) asm_builtin_trace;
	*(cs-6) = (u8) m;
 	*(cs-7) = (u8) asm_builtin_exittrace;
	*(cs-8) = (u8) m->class;

	reg_init();
	descriptor2types(m);                /* set paramcount and paramtypes      */

	M_LDA(REG_SP, REG_SP, -8);          /* build up stackframe                */
	M_LST(REG_RA, REG_SP, 0);           /* store return address               */

	if (runverbose) {
		M_ALD(REG_ITMP1, REG_PV, -6 * 8);
		M_ALD(REG_ITMP3, REG_PV, -5 * 8);
		M_JSR(REG_RA, REG_ITMP3);
		M_NOP;
	}

	if (m->flags & ACC_STATIC) {
		M_MOV(argintregs[5], argintregs[7]);

		M_DMFC1(REG_ITMP1, argfltregs[5]);
		M_DMTC1(REG_ITMP1, argfltregs[7]);

		M_MOV(argintregs[4], argintregs[6]);

		M_DMFC1(REG_ITMP1, argfltregs[4]);
		M_DMTC1(REG_ITMP1, argfltregs[6]);

		M_MOV(argintregs[3], argintregs[5]);
		M_DMFC1(REG_ITMP1, argfltregs[3]);

		M_MOV(argintregs[2], argintregs[4]);
		M_DMTC1(REG_ITMP1, argfltregs[5]);

		M_MOV(argintregs[1], argintregs[3]);
		M_DMFC1(REG_ITMP1, argfltregs[2]);

		M_MOV(argintregs[0], argintregs[2]);
		M_DMTC1(REG_ITMP1, argfltregs[4]);

		M_DMFC1(REG_ITMP1, argfltregs[1]);
		M_DMTC1(REG_ITMP1, argfltregs[3]);

		M_DMFC1(REG_ITMP1, argfltregs[0]);
		M_DMTC1(REG_ITMP1, argfltregs[2]);

		M_ALD(argintregs[1], REG_PV, -8 * 8);

	} else {
		M_MOV(argintregs[6], argintregs[7]);

		M_DMFC1(REG_ITMP1, argfltregs[6]);
		M_DMTC1(REG_ITMP1, argfltregs[7]);

		M_MOV(argintregs[5], argintregs[6]);

		M_DMFC1(REG_ITMP1, argfltregs[5]);
		M_DMTC1(REG_ITMP1, argfltregs[6]);

		M_MOV(argintregs[4], argintregs[5]);
		M_DMFC1(REG_ITMP1, argfltregs[4]);

		M_MOV(argintregs[3], argintregs[4]);
		M_DMTC1(REG_ITMP1, argfltregs[5]);

		M_MOV(argintregs[2], argintregs[3]);
		M_DMFC1(REG_ITMP1, argfltregs[3]);

		M_MOV(argintregs[1], argintregs[2]);
		M_DMTC1(REG_ITMP1, argfltregs[4]);

		M_MOV(argintregs[0], argintregs[1]);
		M_DMFC1(REG_ITMP1, argfltregs[2]);

		M_DMTC1(REG_ITMP1, argfltregs[3]);

		M_DMFC1(REG_ITMP1, argfltregs[1]);
		M_DMFC1(REG_ITMP2, argfltregs[0]);

		M_DMTC1(REG_ITMP1, argfltregs[2]);
		M_DMTC1(REG_ITMP2, argfltregs[1]);
	}

	M_ALD(argintregs[0], REG_PV, -4 * 8); /* load adress of jni_environement  */
	M_ALD(REG_ITMP3, REG_PV, -1 * 8);   /* load adress of native method       */
	M_JSR(REG_RA, REG_ITMP3);           /* call native method                 */
	M_NOP;                              /* delay slot                         */

	if (runverbose) {
		M_ALD(argintregs[0], REG_PV, -6 * 8);
		M_MOV(REG_RESULT, argintregs[1]);
		M_DMFC1(REG_ITMP1, REG_FRESULT);
		M_DMTC1(REG_ITMP1, argfltregs[2]);
		M_DMTC1(REG_ITMP1, argfltregs[3]);
		M_ALD(REG_ITMP3, REG_PV, -7 * 8);/* asm_builtin_exittrace             */
		M_JSR(REG_RA, REG_ITMP3);
		M_NOP;
	}

	M_ALD(REG_ITMP3, REG_PV, -2 * 8);   /* get address of exceptionptr        */

	M_LLD(REG_RA, REG_SP, 0);           /* load return address                */
	M_ALD(REG_ITMP1, REG_ITMP3, 0);     /* load exception into reg. itmp1     */

	M_BNEZ(REG_ITMP1, 2);               /* if no exception then return        */
	M_LDA(REG_SP, REG_SP, 8);           /* remove stackframe, delay slot      */

	M_RET(REG_RA);                      /* return to caller                   */
	M_NOP;                              /* delay slot                         */
	
	M_AST(REG_ZERO, REG_ITMP3, 0);      /* store NULL into exceptionptr       */
	M_ALD(REG_ITMP3, REG_PV, -3 * 8);   /* load asm exception handler address */

	M_JMP(REG_ITMP3);                   /* jump to asm exception handler      */
	M_LDA(REG_ITMP2, REG_RA, -4);       /* move fault address into reg. itmp2 */
	                                    /* delay slot                         */

	(void) docacheflush((void*) cs, (char*) mcodeptr - (char*) cs);

#if 0
	dolog_plain("stubsize: %d (for %d params)\n", 
				(int) (mcodeptr - (s4*) s), m->paramcount);
#endif

#ifdef STATISTICS
	count_nstub_len += NATIVESTUBSIZE * 8;
#endif

	return (u1 *) (s + NATIVESTUBOFFSET);
}


/* function: removenativestub **************************************************

    removes a previously created native-stub from memory
    
*******************************************************************************/

void removenativestub (u1 *stub)
{
	CFREE((u8*) stub - NATIVESTUBOFFSET, NATIVESTUBSIZE * 8);
}


/* function: createcalljava ****************************************************

	creates the asm_calljavamethod (MIPS assembler does not like data in the
	text segment). Documentation can be found in asmpart.c.
	
*******************************************************************************/

#define REG_FSS0    20
#define REG_FSS1    22
#define REG_FSS2    25
#define REG_FSS3    27
#define REG_FSS4    29
#define REG_FSS5    31

#define CALL_JAVA_MEM_SIZE     60
#define CALL_JAVA2_MEM_SIZE    112
#define CALL_JAVA_ENTRY        20
#define CALL_JAVA_XHANDLER     55
#define CALL_JAVA2_XHANDLER    105

static s4 calljavamem[CALL_JAVA_MEM_SIZE];
static s4 calljava2mem[CALL_JAVA2_MEM_SIZE];

void asm_calljavafunction_asm();
void asm_calljavafunction2_asm();

void createcalljava ()
{
	s4 *p;
	
	*((void**)(calljavamem + 4)) = NULL;
	*((void**)(calljavamem + 6)) = (void*) (calljavamem + CALL_JAVA_XHANDLER);
	*((void**)(calljavamem + 8)) = (void*) (calljavamem + CALL_JAVA_XHANDLER);
	*((void**)(calljavamem +10)) = (void*) (calljavamem + CALL_JAVA_ENTRY);
	*((void**)(calljava2mem + 4)) = NULL;
	*((void**)(calljava2mem + 6)) = (void*) (calljava2mem + CALL_JAVA2_XHANDLER);
	*((void**)(calljava2mem + 8)) = (void*) (calljava2mem + CALL_JAVA2_XHANDLER);
	*((void**)(calljava2mem +10)) = (void*) (calljava2mem + CALL_JAVA_ENTRY);
	
	calljavamem[12] = 1;                /* extable size                       */
	calljavamem[13] = 0;                /* fltsave                            */
	calljavamem[14] = 0;                /* intsave                            */
	calljavamem[15] = 0;                /* isleaf                             */
	calljavamem[16] = 0;                /* IsSync                             */
	calljavamem[17] = 80;               /* frame size                         */
	calljavamem[18] = 0;                /* method pointer (NULL)              */
	calljavamem[19] = 0;                /* method pointer (NULL)              */

	calljava2mem[12] = 1;                /* extable size                      */
	calljava2mem[13] = 0;                /* fltsave                           */
	calljava2mem[14] = 1;                /* intsave                           */
	calljava2mem[15] = 0;                /* isleaf                            */
	calljava2mem[16] = 0;                /* IsSync                            */
	calljava2mem[17] = 88;               /* frame size                        */
	calljava2mem[18] = 0;                /* method pointer (NULL)             */
	calljava2mem[19] = 0;                /* method pointer (NULL)             */

	p = calljavamem + CALL_JAVA_ENTRY;  /* code generation pointer            */
	memcpy(p, asm_calljavafunction_asm,
			(CALL_JAVA_MEM_SIZE - CALL_JAVA_ENTRY) * (int) sizeof(s4));

	p = calljava2mem + CALL_JAVA_ENTRY;  /* code generation pointer           */
	memcpy(p, asm_calljavafunction2_asm,
			(CALL_JAVA2_MEM_SIZE - CALL_JAVA_ENTRY) * (int) sizeof(s4));

	(void) docacheflush((void*)(calljavamem + CALL_JAVA_ENTRY),
	       (CALL_JAVA_MEM_SIZE - CALL_JAVA_ENTRY) * (int) sizeof(s4));
	(void) docacheflush((void*)(calljava2mem + CALL_JAVA_ENTRY),
	       (CALL_JAVA2_MEM_SIZE - CALL_JAVA_ENTRY) * (int) sizeof(s4));
}


typedef java_objectheader* (*asm_fptr)(methodinfo*, void*, void*, void*, void*);
typedef java_objectheader* (*asm_fptr2)(methodinfo*, u4, u4, void*);
typedef jdouble (*asm_fptr2double)(methodinfo*, u4, u4, void*);
typedef jlong (*asm_fptr2long)(methodinfo*, u4, u4, void*);


java_objectheader *asm_calljavafunction(methodinfo *m, void *arg1, void *arg2,
										void *arg3, void *arg4)
{
	return ((asm_fptr)(calljavamem + 20))(m, arg1, arg2, arg3, arg4);
}

java_objectheader *asm_calljavafunction2(methodinfo *m, u4 count, u4 size, void *callblock)
{
	return ((asm_fptr2)(calljava2mem + 20))(m, count, size, callblock);
}

jdouble asm_calljavafunction2double(methodinfo *m, u4 count, u4 size, void *callblock)
{
	return ((asm_fptr2double)(calljava2mem + 20))(m, count, size, callblock);
}

jlong asm_calljavafunction2long(methodinfo *m, u4 count, u4 size, void *callblock)
{
	return ((asm_fptr2long)(calljava2mem + 20))(m, count, size, callblock);
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
