/* alpha/ngen.c ****************************************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Contains the codegenerator for an Alpha processor.
	This module generates Alpha machine code for a sequence of
	pseudo commands (ICMDs).

	Authors: Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at
	         Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: $Id: ngen.c 474 2003-10-04 18:55:10Z stefan $

*******************************************************************************/

#include "jitdef.h"   /* phil */

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
	mcode_addxnullrefs(mcodeptr);\
	}
#else
#define gen_nullptr_check(objreg)
#endif


/* MCODECHECK(icnt) */

#define MCODECHECK(icnt) \
	if((mcodeptr+(icnt))>mcodeend)mcodeptr=mcode_increase((u1*)mcodeptr)

/* M_INTMOVE:
     generates an integer-move from register a to b.
     if a and b are the same int-register, no code will be generated.
*/ 

#define M_INTMOVE(a,b) if(a!=b){M_MOV(a,b);}


/* M_FLTMOVE:
    generates a floating-point-move from register a to b.
    if a and b are the same float-register, no code will be generated
*/ 

#define M_FLTMOVE(a,b) if(a!=b){M_FMOV(a,b);}


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


/* NullPointerException signal handler for hardware null pointer check */

void catch_NullPointerException(int sig, int code, sigctx_struct *sigctx)
{
	sigset_t nsig;
	int      instr;
	long     faultaddr;

	/* Reset signal handler - necessary for SysV, does no harm for BSD */

	instr = *((int*)(sigctx->sc_pc));
	faultaddr = sigctx->sc_regs[(instr >> 16) & 0x1f];

	if (faultaddr == 0) {
		signal(sig, (void*) catch_NullPointerException); /* reinstall handler */
		sigemptyset(&nsig);
		sigaddset(&nsig, sig);
		sigprocmask(SIG_UNBLOCK, &nsig, NULL);           /* unblock signal    */
		sigctx->sc_regs[REG_ITMP1_XPTR] =
		                            (long) proto_java_lang_NullPointerException;
		sigctx->sc_regs[REG_ITMP2_XPC] = sigctx->sc_pc;
		sigctx->sc_pc = (long) asm_handle_exception;
		return;
		}
	else {
		faultaddr += (long) ((instr << 16) >> 16);
		fprintf(stderr, "faulting address: 0x%16lx\n", faultaddr);
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
/* initialize floating point control */

#if 0
ieee_set_fp_control(ieee_get_fp_control()
                    & ~IEEE_TRAP_ENABLE_INV
                    & ~IEEE_TRAP_ENABLE_DZE
/*                  & ~IEEE_TRAP_ENABLE_UNF   we dont want underflow */
                    & ~IEEE_TRAP_ENABLE_INE
                    & ~IEEE_TRAP_ENABLE_DNO
                    & ~IEEE_TRAP_ENABLE_OVF);
#endif
#endif

	/* install signal handlers we need to convert to exceptions */

	if (!checknull) {

#if defined(SIGSEGV)
		signal(SIGSEGV, (void*) catch_NullPointerException);
#endif

#if defined(SIGBUS)
		signal(SIGBUS, (void*) catch_NullPointerException);
#endif
		}
}


void print_float(double d)
{
	static int count;
	int *i1 = (int*) &d;
//	printf("%d %08x %08x\n", ++count, i1[1], i1[0]);
	char buf[30];
	sprintf(buf, "%g", d);
	printf("%d %s\n", ++count, buf+1);
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

#define     ExEntrySize     -32
#define     ExStartPC       -8
#define     ExEndPC         -16
#define     ExHandlerPC     -24
#define     ExCatchType     -32

void my_bpt();

static void gen_mcode()
{
	int  len, s1, s2, s3, d, bbs;
	s4   a;
	s4          *mcodeptr;
	stackptr    src;
	varinfo     *var;
	basicblock  *bptr;
	instruction *iptr;
	xtable *ex;

	{
	int p, pa, t, l, r;
	static int mcount;

	if (++mcount == 99) {
	}

	savedregs_num = (isleafmethod) ? 0 : 1;           /* space to save the RA */

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
		{p--;  M_AST (REG_RA, REG_SP, 8*p);}
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
		M_LDA (REG_SP, REG_SP, -(14*8));
		M_AST(REG_RA, REG_SP, 1*8);

		M_LST(argintregs[0], REG_SP,  2*8);
		M_LST(argintregs[1], REG_SP,  3*8);
		M_LST(argintregs[2], REG_SP,  4*8);
		M_LST(argintregs[3], REG_SP,  5*8);
		M_LST(argintregs[4], REG_SP,  6*8);
		M_LST(argintregs[5], REG_SP,  7*8);

		M_DST(argfltregs[0], REG_SP,  8*8);
		M_DST(argfltregs[1], REG_SP,  9*8);
		M_DST(argfltregs[2], REG_SP, 10*8);
		M_DST(argfltregs[3], REG_SP, 11*8);
		M_DST(argfltregs[4], REG_SP, 12*8);
		M_DST(argfltregs[5], REG_SP, 13*8);

		p = dseg_addaddress (method);
		M_ALD(REG_ITMP1, REG_PV, p);
		M_AST(REG_ITMP1, REG_SP, 0);
		p = dseg_addaddress ((void*) (builtin_trace_args));
		M_ALD(REG_PV, REG_PV, p);
		M_JSR(REG_RA, REG_PV);
		M_LDA(REG_PV, REG_RA, -(int)((u1*) mcodeptr - mcodebase));
		M_ALD(REG_RA, REG_SP, 1*8);

		M_LLD(argintregs[0], REG_SP,  2*8);
		M_LLD(argintregs[1], REG_SP,  3*8);
		M_LLD(argintregs[2], REG_SP,  4*8);
		M_LLD(argintregs[3], REG_SP,  5*8);
		M_LLD(argintregs[4], REG_SP,  6*8);
		M_LLD(argintregs[5], REG_SP,  7*8);

		M_DLD(argfltregs[0], REG_SP,  8*8);
		M_DLD(argfltregs[1], REG_SP,  9*8);
		M_DLD(argfltregs[2], REG_SP, 10*8);
		M_DLD(argfltregs[3], REG_SP, 11*8);
		M_DLD(argfltregs[4], REG_SP, 12*8);
		M_DLD(argfltregs[5], REG_SP, 13*8);

		M_LDA (REG_SP, REG_SP, 14*8);
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
 					{M_FLTMOVE (argfltregs[p], r);}
 				else				                 /* reg arg -> spilled    */
 					M_DST (argfltregs[p], REG_SP, 8 * r);
 				}
 			else {                                   /* stack arguments       */
 				pa = p - FLT_ARG_CNT;
 				if (!(var->flags & INMEMORY))        /* stack-arg -> register */
 					M_DLD (r, REG_SP, 8 * (parentargs_base + pa) );
 				else {                               /* stack-arg -> spilled  */
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
		M_ALD(REG_PV, REG_PV, p);
		M_JSR(REG_RA, REG_PV);
		M_LDA(REG_PV, REG_RA, -(int)((u1*) mcodeptr - mcodebase));
		M_LDA(REG_SP, REG_SP, 8);
		}
#endif

	/* call monitorenter function */

#ifdef USE_THREADS
	if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
		p = dseg_addaddress ((void*) (builtin_monitorenter));
		M_ALD(REG_PV, REG_PV, p);
		M_ALD(argintregs[0], REG_SP, 8 * maxmemuse);
		M_JSR(REG_RA, REG_PV);
		M_LDA(REG_PV, REG_RA, -(int)((u1*) mcodeptr - mcodebase));
		}			
#endif
	}

	/* end of header generation */

	/* walk through all basic blocks */
	for (/* bbs = block_count, */ bptr = block; /* --bbs >= 0 */ bptr != NULL; bptr = bptr->next) {

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
							M_FLTMOVE(s1,d);
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
			mcode_addxnullrefs(mcodeptr);
			break;

		/* constant operations ************************************************/

#define ICONST(r,c) if(((c)>=-32768)&&((c)<= 32767)){M_LDA(r,REG_ZERO,c);} \
                    else{a=dseg_adds4(c);M_ILD(r,REG_PV,a);}

#define LCONST(r,c) if(((c)>=-32768)&&((c)<= 32767)){M_LDA(r,REG_ZERO,c);} \
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
#if 1
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1))
				break;
#endif
			var = &(locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY)
				M_DLD(d, REG_SP, 8 * var->regoff);
			else
				{M_FLTMOVE(var->regoff,d);}
			store_reg_to_var_flt(iptr->dst, d);

#if 0
			M_LDA(REG_SP, REG_SP, -8 * 38);
			M_LST(REG_RA, REG_SP, 0);
			M_LST(1, REG_SP, 8 * 1);
			M_LST(2, REG_SP, 8 * 2);
			M_LST(3, REG_SP, 8 * 3);
			M_LST(4, REG_SP, 8 * 4);
			M_LST(5, REG_SP, 8 * 5);
			M_LST(6, REG_SP, 8 * 6);
			M_LST(7, REG_SP, 8 * 7);
			M_LST(8, REG_SP, 8 * 8);
			M_LST(16, REG_SP, 8 * 9);
			M_LST(17, REG_SP, 8 * 10);
			M_LST(18, REG_SP, 8 * 11);
			M_LST(19, REG_SP, 8 * 12);
			M_LST(20, REG_SP, 8 * 13);
			M_LST(21, REG_SP, 8 * 14);
			M_LST(22, REG_SP, 8 * 15);
			M_LST(23, REG_SP, 8 * 16);
			M_LST(24, REG_SP, 8 * 17);
			M_LST(REG_PV, REG_SP, 8 * 18);

			M_DST(1, REG_SP, 8 * 19);
			M_DST(10, REG_SP, 8 * 20);
			M_DST(11, REG_SP, 8 * 21);
			M_DST(12, REG_SP, 8 * 22);
			M_DST(13, REG_SP, 8 * 23);
			M_DST(14, REG_SP, 8 * 24);
			M_DST(15, REG_SP, 8 * 25);
			M_DST(16, REG_SP, 8 * 26);
			M_DST(17, REG_SP, 8 * 27);
			M_DST(18, REG_SP, 8 * 28);
			M_DST(19, REG_SP, 8 * 29);
			M_DST(20, REG_SP, 8 * 30);
			M_DST(21, REG_SP, 8 * 31);
			M_DST(22, REG_SP, 8 * 32);
			M_DST(23, REG_SP, 8 * 33);
			M_DST(24, REG_SP, 8 * 34);
			M_DST(25, REG_SP, 8 * 35);
			M_DST(26, REG_SP, 8 * 36);
			M_DST(27, REG_SP, 8 * 37);

			M_FLTMOVE(d, 16);
			a = dseg_addaddress (print_float);
			M_ALD(REG_PV, REG_PV, a);
			M_JSR(REG_RA, REG_PV);
			
			M_LLD(REG_RA, REG_SP, 0);
			M_LLD(1, REG_SP, 8 * 1);
			M_LLD(2, REG_SP, 8 * 2);
			M_LLD(3, REG_SP, 8 * 3);
			M_LLD(4, REG_SP, 8 * 4);
			M_LLD(5, REG_SP, 8 * 5);
			M_LLD(6, REG_SP, 8 * 6);
			M_LLD(7, REG_SP, 8 * 7);
			M_LLD(8, REG_SP, 8 * 8);
			M_LLD(16, REG_SP, 8 * 9);
			M_LLD(17, REG_SP, 8 * 10);
			M_LLD(18, REG_SP, 8 * 11);
			M_LLD(19, REG_SP, 8 * 12);
			M_LLD(20, REG_SP, 8 * 13);
			M_LLD(21, REG_SP, 8 * 14);
			M_LLD(22, REG_SP, 8 * 15);
			M_LLD(23, REG_SP, 8 * 16);
			M_LLD(24, REG_SP, 8 * 17);
			M_LLD(REG_PV, REG_SP, 8 * 18);

			M_DLD(1, REG_SP, 8 * 19);
			M_DLD(10, REG_SP, 8 * 20);
			M_DLD(11, REG_SP, 8 * 21);
			M_DLD(12, REG_SP, 8 * 22);
			M_DLD(13, REG_SP, 8 * 23);
			M_DLD(14, REG_SP, 8 * 24);
			M_DLD(15, REG_SP, 8 * 25);
			M_DLD(16, REG_SP, 8 * 26);
			M_DLD(17, REG_SP, 8 * 27);
			M_DLD(18, REG_SP, 8 * 28);
			M_DLD(19, REG_SP, 8 * 29);
			M_DLD(20, REG_SP, 8 * 30);
			M_DLD(21, REG_SP, 8 * 31);
			M_DLD(22, REG_SP, 8 * 32);
			M_DLD(23, REG_SP, 8 * 33);
			M_DLD(24, REG_SP, 8 * 34);
			M_DLD(25, REG_SP, 8 * 35);
			M_DLD(26, REG_SP, 8 * 36);
			M_DLD(27, REG_SP, 8 * 37);
			M_LDA(REG_SP, REG_SP, 8 * 38);
#endif

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

#define M_COPY(from,to) \
			d = reg_of_var(to, REG_IFTMP); \
			if ((from->regoff != to->regoff) || \
			    ((from->flags ^ to->flags) & INMEMORY)) { \
				if (IS_FLT_DBL_TYPE(from->type)) { \
					var_to_reg_flt(s1, from, d); \
					M_FLTMOVE(s1,d); \
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
			M_IADD(s1, REG_ZERO, d );
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
            M_CZEXT(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2SHORT:  /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_IADD(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_LADD(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_ISUB(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_LSUB(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_IMUL(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_LMUL (s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_AND_IMM(s2, 0x1f, REG_ITMP3);
			M_SLL(s1, REG_ITMP3, d);
			M_IADD(d, REG_ZERO, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_SLL_IMM(s1, iptr->val.i & 0x1f, d);
			M_IADD(d, REG_ZERO, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_AND_IMM(s2, 0x1f, REG_ITMP3);
			M_SRA(s1, REG_ITMP3, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_SRA_IMM(s1, iptr->val.i & 0x1f, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_AND_IMM(s2, 0x1f, REG_ITMP2);
            M_IZEXT(s1, d);
			M_SRL(d, REG_ITMP2, d);
			M_IADD(d, REG_ZERO, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
            M_IZEXT(s1, d);
			M_SRL_IMM(d, iptr->val.i & 0x1f, d);
			M_IADD(d, REG_ZERO, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_SLL(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_SLL_IMM(s1, iptr->val.i & 0x3f, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_SRA(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_SRA_IMM(s1, iptr->val.i & 0x3f, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_SRL(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_SRL_IMM(s1, iptr->val.i & 0x3f, d);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
            M_CZEXT(s1, REG_ITMP2);
			M_SRA_IMM(s1, 16, d);
			M_CMPLT(REG_ITMP2, d, REG_ITMP1);
			M_ISUB(REG_ITMP2, d, d);
            M_CZEXT(d, d);
			M_IADD(d, REG_ITMP1, d);
			M_BR(11 + (s1 == REG_ITMP1));
			M_ISUB(REG_ZERO, s1, REG_ITMP1);
            M_CZEXT(REG_ITMP1, REG_ITMP2);
			M_SRA_IMM(REG_ITMP1, 16, d);
			M_CMPLT(REG_ITMP2, d, REG_ITMP1);
			M_ISUB(REG_ITMP2, d, d);
            M_CZEXT(d, d);
			M_IADD(d, REG_ITMP1, d);
			M_ISUB(REG_ZERO, d, d);
			if (s1 == REG_ITMP1) {
				var_to_reg_int(s1, src, REG_ITMP1);
				}
			M_SLL_IMM(s1, 33, REG_ITMP2);
			M_CMPEQ(REG_ITMP2, REG_ZERO, REG_ITMP2);
			M_ISUB(d, REG_ITMP2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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

		case ICMD_LREM0X10001:/* ..., value  ==> ..., value % 0x10001         */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			if (s1 == d) {
				M_MOV(s1, REG_ITMP3);
				s1 = REG_ITMP3;
				}
			M_CZEXT(s1, REG_ITMP2);
			M_SRA_IMM(s1, 16, d);
			M_CMPLT(REG_ITMP2, d, REG_ITMP1);
			M_LSUB(REG_ITMP2, d, d);
            M_CZEXT(d, d);
			M_LADD(d, REG_ITMP1, d);
			M_LDA(REG_ITMP2, REG_ZERO, -1);
			M_SRL_IMM(REG_ITMP2, 33, REG_ITMP2);
			if (s1 == REG_ITMP1) {
				var_to_reg_int(s1, src, REG_ITMP1);
				}
			M_CMPULT(s1, REG_ITMP2, REG_ITMP2);
			M_BNEZ(REG_ITMP2, 11);
			M_LDA(d, REG_ZERO, -257);
			M_ZAPNOT_IMM(d, 0xcd, d);
			M_LSUB(REG_ZERO, s1, REG_ITMP2);
			M_CMOVGE(s1, s1, REG_ITMP2);
			M_UMULH(REG_ITMP2, d, REG_ITMP2);
			M_SRL_IMM(REG_ITMP2, 16, REG_ITMP2);
			M_LSUB(REG_ZERO, REG_ITMP2, d);
			M_CMOVGE(s1, REG_ITMP2, d);
			M_SLL_IMM(d, 16, REG_ITMP2);
			M_LADD(d, REG_ITMP2, d);
			M_LSUB(s1, d, d);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_XOR(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_FTMP3);
			M_FMOVN(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			M_FMOVN(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			if (checkfloats) {
				M_FADDS(s1, s2, d);
				M_TRAPB;
				}
			else {
				M_FADD(s1, s2, d);
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			if (checkfloats) {
				M_DADDS(s1, s2, d);
				M_TRAPB;
				}
			else {
				M_DADD(s1, s2, d);
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			if (checkfloats) {
				M_FSUBS(s1, s2, d);
				M_TRAPB;
				}
			else {
				M_FSUB(s1, s2, d);
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			if (checkfloats) {
				M_DSUBS(s1, s2, d);
				M_TRAPB;
				}
			else {
				M_DSUB(s1, s2, d);
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			if (checkfloats) {
				M_FMULS(s1, s2, d);
				M_TRAPB;
				}
			else {
				M_FMUL(s1, s2, d);
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DMUL:       /* ..., val1, val2  ==> ..., val1 *** val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			if (checkfloats) {
				M_DMULS(s1, s2, d);
				M_TRAPB;
				}
			else {
				M_DMUL(s1, s2, d);
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			if (checkfloats) {
				M_FDIVS(s1, s2, d);
				M_TRAPB;
				}
			else {
				M_FDIV(s1, s2, d);
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			if (checkfloats) {
				M_DDIVS(s1, s2, d);
				M_TRAPB;
				}
			else {
				M_DDIV(s1, s2, d);
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;
		
		case ICMD_FREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			if (checkfloats) {
				M_FDIVS(s1,s2, REG_FTMP3);
				M_TRAPB;
				M_CVTDL_CS(REG_FTMP3, REG_FTMP3); /* round to integer */
				M_TRAPB;
				M_CVTLF(REG_FTMP3, REG_FTMP3);
				M_FMULS(REG_FTMP3, s2, REG_FTMP3);
				M_TRAPB;
				M_FSUBS(s1, REG_FTMP3, d);
				M_TRAPB;
				}
			else {
				M_FDIV(s1,s2, REG_FTMP3);
				M_CVTDL_C(REG_FTMP3, REG_FTMP3); /* round to integer */
				M_CVTLF(REG_FTMP3, REG_FTMP3);
				M_FMUL(REG_FTMP3, s2, REG_FTMP3);
				M_FSUB(s1, REG_FTMP3, d);
				}
			store_reg_to_var_flt(iptr->dst, d);
		    break;

		case ICMD_DREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			if (checkfloats) {
				M_DDIVS(s1,s2, REG_FTMP3);
				M_TRAPB;
				M_CVTDL_CS(REG_FTMP3, REG_FTMP3); /* round to integer */
				M_TRAPB;
				M_CVTLD(REG_FTMP3, REG_FTMP3);
				M_DMULS(REG_FTMP3, s2, REG_FTMP3);
				M_TRAPB;
				M_DSUBS(s1, REG_FTMP3, d);
				M_TRAPB;
				}
			else {
				M_DDIV(s1,s2, REG_FTMP3);
				M_CVTDL_C(REG_FTMP3, REG_FTMP3); /* round to integer */
				M_CVTLD(REG_FTMP3, REG_FTMP3);
				M_DMUL(REG_FTMP3, s2, REG_FTMP3);
				M_DSUB(s1, REG_FTMP3, d);
				}
			store_reg_to_var_flt(iptr->dst, d);
		    break;

		case ICMD_I2F:       /* ..., value  ==> ..., (float) value            */
		case ICMD_L2F:
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			a = dseg_adddouble(0.0);
			M_LST (s1, REG_PV, a);
			M_DLD (d, REG_PV, a);
			M_CVTLF(d, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_I2D:       /* ..., value  ==> ..., (double) value           */
		case ICMD_L2D:
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			a = dseg_adddouble(0.0);
			M_LST (s1, REG_PV, a);
			M_DLD (d, REG_PV, a);
			M_CVTLD(d, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
			
		case ICMD_F2I:       /* ..., value  ==> ..., (int) value              */
		case ICMD_D2I:
			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			a = dseg_adddouble(0.0);
			if (checkfloats) {
				M_CVTDL_CS(s1, REG_FTMP1);
				M_TRAPB;
				M_CVTLIS(REG_FTMP1, REG_FTMP2);
				M_TRAPB;
				}
			else {
				M_CVTDL_C(s1, REG_FTMP1);
				M_CVTLI(REG_FTMP1, REG_FTMP2);
				}
			M_DST (REG_FTMP1, REG_PV, a);
			M_ILD (d, REG_PV, a);
			store_reg_to_var_int(iptr->dst, d);
			break;
		
		case ICMD_F2L:       /* ..., value  ==> ..., (long) value             */
		case ICMD_D2L:
			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			a = dseg_adddouble(0.0);
			if (checkfloats) {
				M_CVTDL_CS(s1, REG_FTMP1);
				M_TRAPB;
				}
			else {
				M_CVTDL_C(s1, REG_FTMP1);
				}
			M_DST (REG_FTMP1, REG_PV, a);
			M_LLD (d, REG_PV, a);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_F2D:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			M_FLTMOVE(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
					
		case ICMD_D2F:       /* ..., value  ==> ..., (double) value           */

			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			if (checkfloats) {
				M_CVTDFS(s1, d);
				M_TRAPB;
				}
			else {
				M_CVTDF(s1, d);
				}
			store_reg_to_var_flt(iptr->dst, d);
			break;
		
		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */
		case ICMD_DCMPL:
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			if (checkfloats) {
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
			else {
				M_LSUB_IMM(REG_ZERO, 1, d);
				M_FCMPEQ(s1, s2, REG_FTMP3);
				M_FBEQZ (REG_FTMP3, 1);        /* jump over next instructions */
				M_CLR   (d);
				M_FCMPLT(s2, s1, REG_FTMP3);
				M_FBEQZ (REG_FTMP3, 1);        /* jump over next instruction  */
				M_LADD_IMM(REG_ZERO, 1, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;
			
		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */
		case ICMD_DCMPG:
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			if (checkfloats) {
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
			else {
				M_LADD_IMM(REG_ZERO, 1, d);
				M_FCMPEQ(s1, s2, REG_FTMP3);
				M_FBEQZ (REG_FTMP3, 1);        /* jump over next instruction  */
				M_CLR   (d);
				M_FCMPLT(s1, s2, REG_FTMP3);
				M_FBEQZ (REG_FTMP3, 1);        /* jump over next instruction  */
				M_LSUB_IMM(REG_ZERO, 1, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;


		/* memory operations **************************************************/

			/* #define gen_bound_check \
			if (checkbounds) {\
				M_ILD(REG_ITMP3, s1, OFFSET(java_arrayheader, size));\
				M_CMPULT(s2, REG_ITMP3, REG_ITMP3);\
				M_BEQZ(REG_ITMP3, 0);\
				mcode_addxboundrefs(mcodeptr);\
				}
			*/

#define gen_bound_check \
            if (checkbounds) { \
				M_ILD(REG_ITMP3, s1, OFFSET(java_arrayheader, size));\
				M_CMPULT(s2, REG_ITMP3, REG_ITMP3);\
				M_BEQZ(REG_ITMP3, 0);\
				mcode_addxboundrefs(mcodeptr); \
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
			M_SAADDQ(s2, s1, REG_ITMP1);
			M_ALD( d, REG_ITMP1, OFFSET(java_objectarray, data[0]));
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
			M_S8ADDQ(s2, s1, REG_ITMP1);
			M_LLD(d, REG_ITMP1, OFFSET(java_longarray, data[0]));
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
		  
			M_S4ADDQ(s2, s1, REG_ITMP1);
			M_ILD(d, REG_ITMP1, OFFSET(java_intarray, data[0]));
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
			M_S4ADDQ(s2, s1, REG_ITMP1);
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
			M_S8ADDQ(s2, s1, REG_ITMP1);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_ITMP3);
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


		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */
		                      /* op1 = type, val.a = field address            */

			a = dseg_addaddress (&(((fieldinfo *)(iptr->val.a))->value));
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

			a = dseg_addaddress (&(((fieldinfo *)(iptr->val.a))->value));
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
			M_JMP(REG_ITMP2_XPC, REG_ITMP2);
			M_NOP;              /* nop ensures that XPC is less than the end */
			                    /* of basic block                            */
			ALIGNCODENOP;
			break;

		case ICMD_GOTO:         /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */
			M_BR(0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			ALIGNCODENOP;
			break;

		case ICMD_JSR:          /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */

			M_BSR(REG_ITMP1, 0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;
			
		case ICMD_RET:          /* ... ==> ...                                */
		                        /* op1 = local variable                       */

			var = &(locals[iptr->op1][TYPE_ADR]);
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
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFNONNULL:    /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_BNEZ(s1, 0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPEQ:    /* op1 = target JavaVM pc                     */
		case ICMD_IF_ACMPEQ:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPEQ(s1, s2, REG_ITMP1);
			M_BNEZ(REG_ITMP1, 0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPNE:    /* op1 = target JavaVM pc                     */
		case ICMD_IF_ACMPNE:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPEQ(s1, s2, REG_ITMP1);
			M_BEQZ(REG_ITMP1, 0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPLT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPLT:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPLT(s1, s2, REG_ITMP1);
			M_BNEZ(REG_ITMP1, 0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPGT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPGT:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPLE(s1, s2, REG_ITMP1);
			M_BEQZ(REG_ITMP1, 0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPLE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPLE:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPLE(s1, s2, REG_ITMP1);
			M_BNEZ(REG_ITMP1, 0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPGE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPGE:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMPLT(s1, s2, REG_ITMP1);
			M_BEQZ(REG_ITMP1, 0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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


		case ICMD_IRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_LRETURN:
		case ICMD_ARETURN:

#ifdef USE_THREADS
			if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
				a = dseg_addaddress ((void*) (builtin_monitorexit));
				M_ALD(REG_PV, REG_PV, a);
				M_ALD(argintregs[0], REG_SP, 8 * maxmemuse);
				M_JSR(REG_RA, REG_PV);
				M_LDA(REG_PV, REG_RA, -(int)((u1*) mcodeptr - mcodebase));
				}			
#endif
			var_to_reg_int(s1, src, REG_RESULT);
			M_INTMOVE(s1, REG_RESULT);
			goto nowperformreturn;

		case ICMD_FRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_DRETURN:

#ifdef USE_THREADS
			if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
				a = dseg_addaddress ((void*) (builtin_monitorexit));
				M_ALD(REG_PV, REG_PV, a);
				M_ALD(argintregs[0], REG_SP, 8 * maxmemuse);
				M_JSR(REG_RA, REG_PV);
				M_LDA(REG_PV, REG_RA, -(int)((u1*) mcodeptr - mcodebase));
				}			
#endif
			var_to_reg_flt(s1, src, REG_FRESULT);
			M_FLTMOVE(s1, REG_FRESULT);
			goto nowperformreturn;

		case ICMD_RETURN:      /* ...  ==> ...                                */

#ifdef USE_THREADS
			if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
				a = dseg_addaddress ((void*) (builtin_monitorexit));
				M_ALD(REG_PV, REG_PV, a);
				M_ALD(argintregs[0], REG_SP, 8 * maxmemuse);
				M_JSR(REG_RA, REG_PV);
				M_LDA(REG_PV, REG_RA, -(int)((u1*) mcodeptr - mcodebase));
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

			/* deallocate stack                                               */

			if (parentargs_base)
				{M_LDA(REG_SP, REG_SP, parentargs_base*8);}

			/* call trace function */

			if (runverbose) {
				M_LDA (REG_SP, REG_SP, -24);
				M_AST(REG_RA, REG_SP, 0);
				M_LST(REG_RESULT, REG_SP, 8);
				M_DST(REG_FRESULT, REG_SP,16);
				a = dseg_addaddress (method);
				M_ALD(argintregs[0], REG_PV, a);
				M_MOV(REG_RESULT, argintregs[1]);
				M_FLTMOVE(REG_FRESULT, argfltregs[2]);
				M_FLTMOVE(REG_FRESULT, argfltregs[3]);
				a = dseg_addaddress ((void*) (builtin_displaymethodstop));
				M_ALD(REG_PV, REG_PV, a);
				M_JSR (REG_RA, REG_PV);
				s1 = (int)((u1*) mcodeptr - mcodebase);
				if (s1<=32768) M_LDA (REG_PV, REG_RA, -s1);
				else {
					s4 ml=-s1, mh=0;
					while (ml<-32768) { ml+=65536; mh--; }
					M_LDA (REG_PV, REG_RA, ml );
					M_LDAH (REG_PV, REG_PV, mh );
					}
				M_DLD(REG_FRESULT, REG_SP,16);
				M_LLD(REG_RESULT, REG_SP, 8);
				M_ALD(REG_RA, REG_SP, 0);
				M_LDA (REG_SP, REG_SP, 24);
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


			/* mcode_addreference(BlockPtrOfPC(s4ptr[0]), mcodeptr); */
			mcode_addreference((basicblock *) tptr[0], mcodeptr);

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

			M_SAADDQ(REG_ITMP1, REG_PV, REG_ITMP2);
			M_ALD(REG_ITMP2, REG_ITMP2, -dseglen);
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
						a = dseg_adds4 (val);
						M_ILD(REG_ITMP2, REG_PV, a);
						}
					M_CMPEQ(s1, REG_ITMP2, REG_ITMP2);
					}
				M_BNEZ(REG_ITMP2, 0);
				/* mcode_addreference(BlockPtrOfPC(s4ptr[1]), mcodeptr); */
				mcode_addreference((basicblock *) tptr[0], mcodeptr); 
				}

			M_BR(0);
			/* mcode_addreference(BlockPtrOfPC(l), mcodeptr); */
			
			tptr = (void **) iptr->target;
			mcode_addreference((basicblock *) tptr[0], mcodeptr);

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
						M_FLTMOVE(d, s1);
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

					M_ALD(REG_PV, REG_PV, a); /* Pointer to built-in-function */
					d = iptr->op1;
					goto makeactualcall;

				case ICMD_INVOKESTATIC:
				case ICMD_INVOKESPECIAL:
					a = dseg_addaddress (m->stubroutine);

					M_ALD(REG_PV, REG_PV, a );       /* method pointer in r27 */

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
					sprintf (logtext, "Unkown ICMD-Command: %d", iptr->opc);
					error ();
				}

makeactualcall:

			M_JSR (REG_RA, REG_PV);

			/* recompute pv */

			s1 = (int)((u1*) mcodeptr - mcodebase);
			if (s1<=32768) M_LDA (REG_PV, REG_RA, -s1);
			else {
				s4 ml=-s1, mh=0;
				while (ml<-32768) { ml+=65536; mh--; }
				M_LDA (REG_PV, REG_RA, ml );
				M_LDAH (REG_PV, REG_PV, mh );
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
				M_MOV(s1, REG_ITMP1);
				s1 = REG_ITMP1;
				}
			M_CLR(d);
			if (iptr->op1) {                               /* class/interface */
				if (super->flags & ACC_INTERFACE) {        /* interface       */
					M_BEQZ(s1, 6);
					M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
					M_ILD(REG_ITMP2, REG_ITMP1, OFFSET(vftbl, interfacetablelength));
					M_LDA(REG_ITMP2, REG_ITMP2, - super->index);
					M_BLEZ(REG_ITMP2, 2);
					M_ALD(REG_ITMP1, REG_ITMP1,
					      OFFSET(vftbl, interfacetable[0]) -
					      super->index * sizeof(methodptr*));
					M_CMPULT(REG_ZERO, REG_ITMP1, d);      /* REG_ITMP1 != 0  */
					}
				else {                                     /* class           */
/*
					s2 = super->vftbl->diffval;
					M_BEQZ(s1, 4 + (s2 > 255));
					M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
					M_ILD(REG_ITMP1, REG_ITMP1, OFFSET(vftbl, baseval));
					M_LDA(REG_ITMP1, REG_ITMP1, - super->vftbl->baseval);
					if (s2 <= 255)
						M_CMPULE_IMM(REG_ITMP1, s2, d);
					else {
						M_LDA(REG_ITMP2, REG_ZERO, s2);
						M_CMPULE(REG_ITMP1, REG_ITMP2, d);
						}
*/
					M_BEQZ(s1, 7);
					M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
					a = dseg_addaddress ((void*) super->vftbl);
					M_ALD(REG_ITMP2, REG_PV, a);
					M_ILD(REG_ITMP1, REG_ITMP1, OFFSET(vftbl, baseval));
					M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl, baseval));
					M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl, diffval));
					M_ISUB(REG_ITMP1, REG_ITMP3, REG_ITMP1);
					M_CMPULE(REG_ITMP1, REG_ITMP2, d);
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
					M_BEQZ(s1, 6);
					M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
					M_ILD(REG_ITMP2, REG_ITMP1, OFFSET(vftbl, interfacetablelength));
					M_LDA(REG_ITMP2, REG_ITMP2, - super->index);
					M_BLEZ(REG_ITMP2, 0);
					mcode_addxcastrefs(mcodeptr);
					M_ALD(REG_ITMP2, REG_ITMP1,
					      OFFSET(vftbl, interfacetable[0]) -
					      super->index * sizeof(methodptr*));
					M_BEQZ(REG_ITMP2, 0);
					mcode_addxcastrefs(mcodeptr);
					}
				else {                                     /* class           */
/*
					s2 = super->vftbl->diffval;
					M_BEQZ(s1, 4 + (s2 != 0) + (s2 > 255));
					M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
					M_ILD(REG_ITMP1, REG_ITMP1, OFFSET(vftbl, baseval));
					M_LDA(REG_ITMP1, REG_ITMP1, - super->vftbl->baseval);
					if (s2 == 0) {
						M_BNEZ(REG_ITMP1, 0);
						}
					else if (s2 <= 255) {
						M_CMPULE_IMM(REG_ITMP1, s2, REG_ITMP2);
						M_BEQZ(REG_ITMP2, 0);
						}
					else {
						M_LDA(REG_ITMP2, REG_ZERO, s2);
						M_CMPULE(REG_ITMP1, REG_ITMP2, REG_ITMP2);
						M_BEQZ(REG_ITMP2, 0);
						}
*/
					M_BEQZ(s1, 8 + (d == REG_ITMP3));
					M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl));
					a = dseg_addaddress ((void*) super->vftbl);
					M_ALD(REG_ITMP2, REG_PV, a);
					M_ILD(REG_ITMP1, REG_ITMP1, OFFSET(vftbl, baseval));
					if (d != REG_ITMP3) {
						M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl, baseval));
						M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl, diffval));
						M_ISUB(REG_ITMP1, REG_ITMP3, REG_ITMP1);
						}
					else {
						M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl, baseval));
						M_ISUB(REG_ITMP1, REG_ITMP2, REG_ITMP1);
						M_ALD(REG_ITMP2, REG_PV, a);
						M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl, diffval));
						}
					M_CMPULE(REG_ITMP1, REG_ITMP2, REG_ITMP2);
					M_BEQZ(REG_ITMP2, 0);
					mcode_addxcastrefs(mcodeptr);
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
			mcode_addxcheckarefs(mcodeptr);
			break;

		case ICMD_MULTIANEWARRAY:/* ..., cnt1, [cnt2, ...] ==> ..., arrayref  */
		                      /* op1 = dimension, val.a = array descriptor    */

			/* check for negative sizes and copy sizes to stack if necessary  */

			MCODECHECK((iptr->op1 << 1) + 64);

			for (s1 = iptr->op1; --s1 >= 0; src = src->prev) {
				var_to_reg_int(s2, src, REG_ITMP1);
				M_BLTZ(s2, 0);
				mcode_addxcheckarefs(mcodeptr);

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
			M_ALD(REG_PV, REG_PV, a);
			M_JSR(REG_RA, REG_PV);
			s1 = (int)((u1*) mcodeptr - mcodebase);
			if (s1 <= 32768)
				M_LDA (REG_PV, REG_RA, -s1);
			else {
				s4 ml = -s1, mh = 0;
				while (ml < -32768) {ml += 65536; mh--;}
				M_LDA(REG_PV, REG_RA, ml);
				M_LDAH(REG_PV, REG_PV, mh);
			    }
			s1 = reg_of_var(iptr->dst, REG_RESULT);
			M_INTMOVE(REG_RESULT, s1);
			store_reg_to_var_int(iptr->dst, s1);
			break;


		default: sprintf (logtext, "Unknown pseudo command: %d", iptr->opc);
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
					M_FLTMOVE(s1,interfaces[len][s2].regoff);
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
			M_BR((xcodeptr-mcodeptr)-1);
			}
		else {
			xcodeptr = mcodeptr;

			a = dseg_addaddress(proto_java_lang_ArrayIndexOutOfBoundsException);
			M_ALD(REG_ITMP1_XPTR, REG_PV, a);

			a = dseg_addaddress(asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);

			M_JMP(REG_ZERO, REG_ITMP3);
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
			M_BR((xcodeptr-mcodeptr)-1);
			}
		else {
			xcodeptr = mcodeptr;

			a = dseg_addaddress(proto_java_lang_NegativeArraySizeException);
			M_ALD(REG_ITMP1_XPTR, REG_PV, a);

			a = dseg_addaddress(asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);

			M_JMP(REG_ZERO, REG_ITMP3);
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
			M_BR((xcodeptr-mcodeptr)-1);
			}
		else {
			xcodeptr = mcodeptr;

			a = dseg_addaddress(proto_java_lang_ClassCastException);
			M_ALD(REG_ITMP1_XPTR, REG_PV, a);

			a = dseg_addaddress(asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);

			M_JMP(REG_ZERO, REG_ITMP3);
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
			M_BR((xcodeptr-mcodeptr)-1);
			}
		else {
			xcodeptr = mcodeptr;

			a = dseg_addaddress(proto_java_lang_NullPointerException);
			M_ALD(REG_ITMP1_XPTR, REG_PV, a);

			a = dseg_addaddress(asm_handle_exception);
			M_ALD(REG_ITMP3, REG_PV, a);

			M_JMP(REG_ZERO, REG_ITMP3);
			}
		}

#endif
	}

	mcode_finish((int)((u1*) mcodeptr - mcodebase));
}


/* redefinition of code generation macros (compiling into array) **************/

/* 
These macros are newly defined to allow code generation into an array.
This is necessary, because the original M_.. macros generate code by
calling 'mcode_adds4' that uses an additional data structure to
receive the code.

For a faster (but less flexible) version to generate code, these
macros directly use the (s4* p) - pointer to put the code directly
in a locally defined array.
This makes sense only for the stub-generation-routines below.
*/

#undef M_OP3
#define M_OP3(op,fu,a,b,c,const) \
	*(p++) = ( (((s4)(op))<<26)|((a)<<21)|((b)<<(16-3*(const)))| \
	((const)<<12)|((fu)<<5)|((c)) )
#undef M_FOP3
#define M_FOP3(op,fu,a,b,c) \
	*(p++) = ( (((s4)(op))<<26)|((a)<<21)|((b)<<16)|((fu)<<5)|(c) )
#undef M_BRA
#define M_BRA(op,a,disp) \
	*(p++) = ( (((s4)(op))<<26)|((a)<<21)|((disp)&0x1fffff) )
#undef M_MEM
#define M_MEM(op,a,b,disp) \
	*(p++) = ( (((s4)(op))<<26)|((a)<<21)|((b)<<16)|((disp)&0xffff) )


/* function createcompilerstub *************************************************

	creates a stub routine which calls the compiler
	
*******************************************************************************/

#define COMPSTUBSIZE 3

u1 *createcompilerstub (methodinfo *m)
{
	u8 *s = CNEW (u8, COMPSTUBSIZE);    /* memory to hold the stub            */
	s4 *p = (s4*) s;                    /* code generation pointer            */
	
	                                    /* code for the stub                  */
	M_ALD (REG_PV, REG_PV, 16);         /* load pointer to the compiler       */
	M_JMP (0, REG_PV);                  /* jump to the compiler, return address
	                                       in reg 0 is used as method pointer */
	s[1] = (u8) m;                      /* literals to be adressed            */  
	s[2] = (u8) asm_call_jit_compiler;  /* jump directly via PV from above    */

#ifdef STATISTICS
	count_cstub_len += COMPSTUBSIZE * 8;
#endif

	return (u1*) s;
}


/* function removecompilerstub *************************************************

     deletes a compilerstub from memory  (simply by freeing it)

*******************************************************************************/

void removecompilerstub (u1 *stub) 
{
	CFREE (stub, COMPSTUBSIZE * 8);
}

/* function: createnativestub **************************************************

	creates a stub routine which calls a native method

*******************************************************************************/

#define NATIVESTUBSIZE 34
#define NATIVESTUBOFFSET 8

int runverbosenat = 0;

u1 *createnativestub (functionptr f, methodinfo *m)
{
	int disp;
	u8 *s = CNEW (u8, NATIVESTUBSIZE);  /* memory to hold the stub            */
	u8 *cs = s + NATIVESTUBOFFSET;
	s4 *p = (s4*) (cs);                 /* code generation pointer            */

	*(cs-1) = (u8) f;                   /* address of native method           */
	*(cs-2) = (u8) (&exceptionptr);     /* address of exceptionptr            */
	*(cs-3) = (u8) asm_handle_nat_exception; /* addr of asm exception handler */
	*(cs-4) = (u8) (&env);              /* addr of jni_environement           */
	*(cs-5) = (u8) asm_builtin_trace;
	*(cs-6) = (u8) m;
	*(cs-7) = (u8) asm_builtin_exittrace;
	*(cs-8) = (u8) builtin_trace_exception;

#if 0
	printf("stub: ");
	utf_display(m->class->name);
	printf(".");
	utf_display(m->name);
	printf(" 0x%p\n", cs);
#endif

	M_LDA  (REG_SP, REG_SP, -8);        /* build up stackframe                */
	M_AST  (REG_RA, REG_SP, 0);         /* store return address               */

#if 1
	if (runverbosenat) {
		M_ALD(REG_ITMP1, REG_PV, -6*8);
		M_ALD(REG_PV, REG_PV, -5*8);

		M_JSR(REG_RA, REG_PV);
		disp = -(int) (p - (s4*) cs)*4;
		M_LDA(REG_PV, REG_RA, disp);
	}
#endif

	reg_init();

	M_MOV  (argintregs[4],argintregs[5]); 
	M_FMOV (argfltregs[4],argfltregs[5]);

	M_MOV  (argintregs[3],argintregs[4]);
	M_FMOV (argfltregs[3],argfltregs[4]);

	M_MOV  (argintregs[2],argintregs[3]);
	M_FMOV (argfltregs[2],argfltregs[3]);

	M_MOV  (argintregs[1],argintregs[2]);
	M_FMOV (argfltregs[1],argfltregs[2]);

	M_MOV  (argintregs[0],argintregs[1]);
	M_FMOV (argfltregs[0],argfltregs[1]);
	
	M_ALD  (argintregs[0], REG_PV, -4*8);/* load adress of jni_environement   */

	M_ALD  (REG_PV, REG_PV, -1*8);      /* load adress of native method       */
	M_JSR  (REG_RA, REG_PV);            /* call native method                 */

	disp = -(int) (p - (s4*) cs)*4;
	M_LDA  (REG_PV, REG_RA, disp);      /* recompute pv from ra               */
	M_ALD  (REG_ITMP3, REG_PV, -2*8);   /* get address of exceptionptr        */

	M_ALD  (REG_ITMP1, REG_ITMP3, 0);   /* load exception into reg. itmp1     */
	M_BNEZ (REG_ITMP1,
			3 + (runverbosenat ? 6 : 0));  /* if no exception then return        */

#if 1
	if (runverbosenat) {
		M_ALD(argintregs[0], REG_PV, -6*8);
		M_MOV(REG_RESULT, argintregs[1]);
		M_FMOV(REG_FRESULT, argfltregs[2]);
		M_FMOV(REG_FRESULT, argfltregs[3]);
		M_ALD(REG_PV, REG_PV, -7*8);
		M_JSR(REG_RA, REG_PV);
	}
#endif

	M_ALD  (REG_RA, REG_SP, 0);         /* load return address                */
	M_LDA  (REG_SP, REG_SP, 8);         /* remove stackframe                  */

	M_RET  (REG_ZERO, REG_RA);          /* return to caller                   */
	
	M_AST  (REG_ZERO, REG_ITMP3, 0);    /* store NULL into exceptionptr       */

#if 1
	if (runverbosenat) {
		M_LDA(REG_SP, REG_SP, -8);
		M_AST(REG_ITMP1, REG_SP, 0);
		M_MOV(REG_ITMP1, argintregs[0]);
		M_ALD(argintregs[1], REG_PV, -6*8);
		M_ALD(argintregs[2], REG_SP, 0);
		M_CLR(argintregs[3]);
		M_ALD(REG_PV, REG_PV, -8*8);
		M_JSR(REG_RA, REG_PV);
		disp = -(int) (p - (s4*) cs)*4;
		M_LDA  (REG_PV, REG_RA, disp);
		M_ALD(REG_ITMP1, REG_SP, 0);
		M_LDA(REG_SP, REG_SP, 8);
	}
#endif

	M_ALD  (REG_RA, REG_SP, 0);         /* load return address                */
	M_LDA  (REG_SP, REG_SP, 8);         /* remove stackframe                  */

	M_LDA  (REG_ITMP2, REG_RA, -4);     /* move fault address into reg. itmp2 */

	M_ALD  (REG_ITMP3, REG_PV, -3*8);   /* load asm exception handler address */
	M_JMP  (REG_ZERO, REG_ITMP3);       /* jump to asm exception handler      */
	
#if 0
	{
		static int stubprinted;
		if (!stubprinted)
			printf("stubsize: %d/2\n", (int) (p - (s4*) s));
		stubprinted = 1;
	}
#endif

#ifdef STATISTICS
	count_nstub_len += NATIVESTUBSIZE * 8;
#endif

	return (u1*) (s + NATIVESTUBOFFSET);
}

/* function: removenativestub **************************************************

    removes a previously created native-stub from memory
    
*******************************************************************************/

void removenativestub (u1 *stub)
{
	CFREE ((u8*) stub - NATIVESTUBOFFSET, NATIVESTUBSIZE * 8);
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
