/* i386/ngen.c *****************************************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Contains the codegenerator for an i386 processor.
	This module generates i386 machine code for a sequence of
	pseudo commands (ICMDs).

	Authors: Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at
	         Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: $Id: ngen.c 240 2003-02-27 20:51:04Z twisti $

*******************************************************************************/

#include "jitdef.h"   /* phil */

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
	if (checknull) { \
        i386_alu_imm_reg(I386_CMP, 0, (objreg)); \
        i386_jcc(I386_CC_E, 0); \
 	    mcode_addxnullrefs(mcodeptr); \
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

#define M_INTMOVE(reg,dreg) if ((reg) != (dreg)) { i386_mov_reg_reg((reg),(dreg)); }


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

#define var_to_reg_int(regnr,v,tempnr) \
    do { \
        if ((v)->flags & INMEMORY) { \
            COUNT_SPILLS; \
            i386_mov_membase_reg(REG_SP, (v)->regoff * 8, tempnr); \
            regnr = tempnr; \
        } else { \
            regnr = (v)->regoff; \
        } \
    } while (0)



#define var_to_reg_flt(regnr,v,tempnr) \
    do { \
        if ((v)->flags & INMEMORY) { \
            COUNT_SPILLS; \
            i386_fstps_membase(REG_SP, (v)->regoff * 8); \
            regnr = tempnr; \
        } else { \
            panic("floats have to be in memory"); \
        } \
    } while (0)


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

/* TWISTI */
/*  #define store_reg_to_var_int(sptr, tempregnum) {       \ */
/*  	if ((sptr)->flags & INMEMORY) {                    \ */
/*  		COUNT_SPILLS;                                  \ */
/*  		M_LST(tempregnum, REG_SP, 8 * (sptr)->regoff); \ */
/*  		}                                              \ */
/*  	} */
#define store_reg_to_var_int(sptr, tempregnum) \
    do { \
        if ((sptr)->flags & INMEMORY) { \
            COUNT_SPILLS; \
            i386_mov_reg_membase(tempregnum, REG_SP, (sptr)->regoff * 8); \
        } \
    } while (0)


/* TWISTI */
/*  #define store_reg_to_var_flt(sptr, tempregnum) {       \ */
/*  	if ((sptr)->flags & INMEMORY) {                    \ */
/*  		COUNT_SPILLS;                                  \ */
/*  		M_DST(tempregnum, REG_SP, 8 * (sptr)->regoff); \ */
/*  		}                                              \ */
/*  	} */
#define store_reg_to_var_flt(sptr, tempregnum) \
    do { \
        if ((sptr)->type == TYPE_FLT) { \
            if ((sptr)->flags & INMEMORY) { \
                COUNT_SPILLS; \
                i386_fsts_membase(REG_SP, (sptr)->regoff * 8); \
            } else { \
                panic("floats have to be in memory"); \
            } \
        } else { \
            if ((sptr)->flags & INMEMORY) { \
                COUNT_SPILLS; \
                i386_fstl_membase(REG_SP, (sptr)->regoff * 8); \
            } else { \
                panic("doubles have to be in memory"); \
            } \
        } \
    } while (0)


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
		sigctx->sc_pc = (long) asm_handle_nat_exception;
		return;
		}
	else {
		faultaddr += (long) ((instr << 16) >> 16);
		fprintf(stderr, "faulting address: 0x%16lx\n", faultaddr);
		panic("Stack overflow");
		}
}

void init_exceptions(void)
{
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

#if POINTERSIZE == 8
#    define     ExEntrySize     -32
#    define     ExStartPC       -8
#    define     ExEndPC         -16
#    define     ExHandlerPC     -24
#    define     ExCatchType     -32
#else
#    define     ExEntrySize     -16
#    define     ExStartPC       -4
#    define     ExEndPC         -8
#    define     ExHandlerPC     -12
#    define     ExCatchType     -16
#endif

static void gen_mcode()
{
	int  len, s1, s2, s3, d, bbs;
	s4   a;
	s4          *mcodeptr;
	stackptr    src;
	varinfo     *var;
	varinfo     *dst;
	basicblock  *bptr;
	instruction *iptr;

	xtable *ex;

	{
	int p, pa, t, l, r;

	/* TWISTI */
/*  	savedregs_num = (isleafmethod) ? 0 : 1;           /* space to save the RA */

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

	if (parentargs_base) {
		i386_alu_imm_reg(I386_SUB, parentargs_base * 8, REG_SP);
	}

	/* save return address and used callee saved registers */

	p = parentargs_base;
	if (!isleafmethod) {
		/* p--; M_AST (REG_RA, REG_SP, 8*p); -- do we really need this on i386 */
	}
	for (r = savintregcnt - 1; r >= maxsavintreguse; r--) {
 		p--; i386_mov_reg_membase(savintregs[r], REG_SP, p * 8);
	}
	for (r = savfltregcnt - 1; r >= maxsavfltreguse; r--) {
		p--; M_DST (savfltregs[r], REG_SP, 8 * p);
	}

	/* save monitorenter argument */

#ifdef USE_THREADS
	if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
		if (method->flags & ACC_STATIC) {
			p = dseg_addaddress (class);
			M_ALD(REG_ITMP1, REG_PV, p);
			M_AST(REG_ITMP1, REG_SP, 8 * maxmemuse);

		} else {
			i386_mov_membase_reg(REG_SP, parentargs_base * 8 + 4, REG_ITMP1);
			i386_mov_reg_membase(REG_ITMP1, REG_SP, 8 * maxmemuse);
		}
	}			
#endif

	/* copy argument registers to stack and call trace function with pointer
	   to arguments on stack. ToDo: save floating point registers !!!!!!!!!
	*/

	if (runverbose && isleafmethod) {
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
/*  		p = dseg_addaddress ((void*) (builtin_trace_args)); */
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
#if 0
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
#endif

/*  	printf("mparamcount=%d\n", mparamcount); */
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
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
 					M_INTMOVE (argintregs[p], r);
				} else {                             /* reg arg -> spilled    */
 					M_LST (argintregs[p], REG_SP, 8 * r);
 				}
			} else {                                 /* stack arguments       */
 				pa = p - INT_ARG_CNT;
 				if (!(var->flags & INMEMORY)) {      /* stack arg -> register */ 
 					i386_mov_membase_reg(REG_SP, (parentargs_base + pa) * 8 + 4, r);            /* + 4 for return address */
				} else {                             /* stack arg -> spilled  */
					if (t == TYPE_LNG) {
						i386_mov_membase_reg(REG_SP, (parentargs_base + pa) * 8 + 4, REG_ITMP1);    /* + 4 for return address */
						i386_mov_membase_reg(REG_SP, (parentargs_base + pa) * 8 + 4 + 4, REG_ITMP2);    /* + 4 for return address */
						i386_mov_reg_membase(REG_ITMP1, REG_SP, r * 8);
						i386_mov_reg_membase(REG_ITMP2, REG_SP, r * 8 + 4);

					} else {
						i386_mov_membase_reg(REG_SP, (parentargs_base + pa) * 8 + 4, REG_ITMP1);    /* + 4 for return address */
						i386_mov_reg_membase(REG_ITMP1, REG_SP, r * 8);
					}
				}
			}
		
		} else {                                     /* floating args         */   
 			if (p < FLT_ARG_CNT) {                   /* register arguments    */
 				if (!(var->flags & INMEMORY)) {      /* reg arg -> register   */
 					M_FLTMOVE (argfltregs[p], r);
 				} else {			                 /* reg arg -> spilled    */
 					M_DST (argfltregs[p], REG_SP, 8 * r);
 				}
 			} else {                                 /* stack arguments       */
 				pa = p - FLT_ARG_CNT;
 				if (!(var->flags & INMEMORY)) {      /* stack-arg -> register */
 					i386_mov_membase_reg(REG_SP, (parentargs_base + pa) * 8 + 4, r);
 				} else {                              /* stack-arg -> spilled  */
/*  					printf("float memory\n"); */
 					i386_mov_membase_reg(REG_SP, (parentargs_base + pa) * 8 + 4, REG_ITMP1);
 					i386_mov_reg_membase(REG_ITMP1, REG_SP, r * 8);
				}
			}
		}
	}  /* end for */

	/* call trace function */

	if (runverbose && !isleafmethod) {
		M_LDA (REG_SP, REG_SP, -8);
		p = dseg_addaddress (method);
		M_ALD(REG_ITMP1, REG_PV, p);
		M_AST(REG_ITMP1, REG_SP, 0);
/*  		p = dseg_addaddress ((void*) (builtin_trace_args)); */
		M_ALD(REG_PV, REG_PV, p);
		M_JSR(REG_RA, REG_PV);
		M_LDA(REG_PV, REG_RA, -(int)((u1*) mcodeptr - mcodebase));
		M_LDA(REG_SP, REG_SP, 8);
		}

	/* call monitorenter function */

#ifdef USE_THREADS
	if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
		i386_mov_membase_reg(REG_SP, 8 * maxmemuse, REG_ITMP1);
		i386_alu_imm_reg(I386_SUB, 4, REG_SP);
		i386_mov_reg_membase(REG_ITMP1, REG_SP, 0);
		i386_mov_imm_reg(builtin_monitorenter, REG_ITMP1);
		i386_call_reg(REG_ITMP1);
		i386_alu_imm_reg(I386_ADD, 4, REG_SP);
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

		/* TWISTI */
/*  		src = bptr->instack; */
/*  		len = bptr->indepth; */
/*  		MCODECHECK(64+len); */
/*  		while (src != NULL) { */
/*  			len--; */
/*  			if ((len == 0) && (bptr->type != BBTYPE_STD)) { */
/*  				d = reg_of_var(src, REG_ITMP1); */
/*  				M_INTMOVE(REG_ITMP1, d); */
/*  				store_reg_to_var_int(src, d); */
/*  				} */
/*  			else { */
/*  				d = reg_of_var(src, REG_IFTMP); */
/*  				if ((src->varkind != STACKVAR)) { */
/*  					s2 = src->type; */
/*  					if (IS_FLT_DBL_TYPE(s2)) { */
/*  						if (!(interfaces[len][s2].flags & INMEMORY)) { */
/*  							s1 = interfaces[len][s2].regoff; */
/*  							M_FLTMOVE(s1,d); */
/*  							} */
/*  						else { */
/*  							M_DLD(d, REG_SP, 8 * interfaces[len][s2].regoff); */
/*  							} */
/*  						store_reg_to_var_flt(src, d); */
/*  						} */
/*  					else { */
/*  						if (!(interfaces[len][s2].flags & INMEMORY)) { */
/*  							s1 = interfaces[len][s2].regoff; */
/*  							M_INTMOVE(s1,d); */
/*  							} */
/*  						else { */
/*  							M_LLD(d, REG_SP, 8 * interfaces[len][s2].regoff); */
/*  							} */
/*  						store_reg_to_var_int(src, d); */
/*  						} */
/*  					} */
/*  				} */
/*  			src = src->prev; */
/*  			} */
		src = bptr->instack;
		len = bptr->indepth;
		MCODECHECK(64+len);
		while (src != NULL) {
			len--;
			if ((len == 0) && (bptr->type != BBTYPE_STD)) {
				d = reg_of_var(src, REG_ITMP1);
				M_INTMOVE(REG_ITMP1, d);
				store_reg_to_var_int(src, d);

			} else {
				d = reg_of_var(src, REG_ITMP1);
				if ((src->varkind != STACKVAR)) {
					s2 = src->type;
					if (IS_FLT_DBL_TYPE(s2)) {
						if (!(interfaces[len][s2].flags & INMEMORY)) {
							s1 = interfaces[len][s2].regoff;
							M_FLTMOVE(s1, d);

						} else {
							M_DLD(d, REG_SP, 8 * interfaces[len][s2].regoff);
						}
						store_reg_to_var_flt(src, d);

					} else {
						if (!(interfaces[len][s2].flags & INMEMORY)) {
							s1 = interfaces[len][s2].regoff;
							M_INTMOVE(s1, d);

						} else {
							i386_mov_membase_reg(REG_SP, interfaces[len][s2].regoff * 8, d);
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
				i386_alu_imm_membase(I386_CMP, 0, REG_SP, src->regoff * 8);

			} else {
				/* TODO: optimize: test reg,reg */
				i386_alu_imm_reg(I386_CMP, 0, src->regoff);
			}
			i386_jcc(I386_CC_E, 0);
			mcode_addxnullrefs(mcodeptr);
			break;

		/* constant operations ************************************************/

		case ICMD_ICONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.i = constant                    */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				i386_mov_imm_membase(iptr->val.i, REG_SP, iptr->dst->regoff * 8);

			} else {
				i386_mov_imm_reg(iptr->val.i, d);
			}
			break;

		case ICMD_LCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.l = constant                    */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				i386_mov_imm_membase(iptr->val.l, REG_SP, iptr->dst->regoff * 8);
				i386_mov_imm_membase(iptr->val.l >> 32, REG_SP, iptr->dst->regoff * 8 + 4);
				
			} else {
				panic("longs have to be in memory");
			}
			break;

		case ICMD_FCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.f = constant                    */

			d = reg_of_var(iptr->dst, REG_FTMP1);
			if (iptr->val.f == 0) {
				i386_fldz();

			} else if (iptr->val.f == 1) {
				i386_fld1();

			} else if (iptr->val.f == 2) {
				i386_fld1();
				i386_fld1();
				i386_faddp();

			} else {
				a = dseg_addfloat(iptr->val.f);
				i386_mov_imm_reg(0, REG_ITMP1);
				dseg_adddata(mcodeptr);
				i386_flds_membase(REG_ITMP1, a);
			}
			store_reg_to_var_flt(iptr->dst, d);
			break;
		
		case ICMD_DCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.d = constant                    */

			d = reg_of_var(iptr->dst, REG_FTMP1);
			if (iptr->val.d == 0) {
				i386_fldz();

			} else if (iptr->val.d == 1) {
				i386_fld1();

			} else {
				a = dseg_adddouble(iptr->val.d);
				i386_mov_imm_reg(0, REG_ITMP1);
				dseg_adddata(mcodeptr);
				i386_fldl_membase(REG_ITMP1, a);
			}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_ACONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.a = constant                    */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				i386_mov_imm_membase(iptr->val.a, REG_SP, iptr->dst->regoff * 8);
			} else {

				i386_mov_imm_reg(iptr->val.a, iptr->dst->regoff);
			}
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
			if (iptr->dst->flags & INMEMORY) {
				if (var->flags & INMEMORY) {
					i386_mov_membase_reg(REG_SP, var->regoff * 8, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else {
					i386_mov_reg_membase(var->regoff, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				printf("ILOAD: dst not in memory?\n");
			}
			break;

		case ICMD_ALOAD:      /* ...  ==> ..., content of local variable      */
                              /* op1 = local variable                         */

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
				printf("ALOAD: dst not in memory?\n");
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
					i386_mov_membase_reg(REG_SP, var->regoff * 8, REG_ITMP1);
					i386_mov_membase_reg(REG_SP, var->regoff * 8 + 4, REG_ITMP2);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 4);
					i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 4 + 4);

				} else {
					i386_mov_reg_membase(var->regoff, REG_SP, iptr->dst->regoff * 4);
				}

			} else {
				printf("LLOAD: dst not in memory?\n");
			}
			break;

		case ICMD_FLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			d = reg_of_var(iptr->dst, REG_FTMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1)) {
/*  				break; */
			}
			var = &(locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			i386_flds_membase(REG_SP, var->regoff * 8);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_DLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			d = reg_of_var(iptr->dst, REG_FTMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1)) {
/*  				break; */
			}
			var = &(locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			i386_fldl_membase(REG_SP, var->regoff * 8);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_ISTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

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

		case ICMD_ASTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

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
				var_to_reg_int(s1, src, REG_ITMP1);
				i386_mov_reg_membase(s1, REG_SP, var->regoff * 8);

			} else {
				var_to_reg_int(s1, src, var->regoff);
				M_INTMOVE(s1, var->regoff);
			}
			break;

		case ICMD_FSTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			var = &(locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
/*  			i386_fstps_membase(REG_SP, var->regoff * 8); */
			break;

		case ICMD_DSTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			var = &(locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
/*  			i386_fstpl_membase(REG_SP, var->regoff * 8); */
			break;


		/* pop/dup/swap operations ********************************************/

		/* attention: double and longs are only one entry in CACAO ICMDs      */

		case ICMD_POP:        /* ..., value  ==> ...                          */
		case ICMD_POP2:       /* ..., value, value  ==> ...                   */
			break;

			/* TWISTI */
/*  #define M_COPY(from,to) \ */
/*  			d = reg_of_var(to, REG_IFTMP); \ */
/*  			if ((from->regoff != to->regoff) || \ */
/*  			    ((from->flags ^ to->flags) & INMEMORY)) { \ */
/*  				if (IS_FLT_DBL_TYPE(from->type)) { \ */
/*  					var_to_reg_flt(s1, from, d); \ */
/*  					M_FLTMOVE(s1,d); \ */
/*  					store_reg_to_var_flt(to, d); \ */
/*  					}\ */
/*  				else { \ */
/*  					var_to_reg_int(s1, from, d); \ */
/*  					M_INTMOVE(s1,d); \ */
/*  					store_reg_to_var_int(to, d); \ */
/*  					}\ */
/*  				} */
/*  			printf("DUP: from,regoff=%d,%d to,regoff=%d,%d\n", from->flags & INMEMORY, from->regoff, to->flags & INMEMORY, to->regoff); \ */
#define M_COPY(from,to) \
			if ((from->regoff != to->regoff) || \
			    ((from->flags ^ to->flags) & INMEMORY)) { \
				if (IS_FLT_DBL_TYPE(from->type)) { \
			        d = reg_of_var(to, REG_IFTMP); \
					var_to_reg_flt(s1, from, d); \
					M_FLTMOVE(s1,d); \
					store_reg_to_var_flt(to, d); \
				} else { \
			        d = reg_of_var(to, REG_ITMP1); \
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

			d = reg_of_var(iptr->dst, REG_ITMP3);
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

			d = reg_of_var(iptr->dst, REG_ITMP3);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_neg_membase(REG_SP, iptr->dst->regoff * 8);
						i386_alu_imm_membase(I386_ADC, 0, REG_SP, iptr->dst->regoff * 8 + 4);
						i386_neg_membase(REG_SP, iptr->dst->regoff * 8 + 4);

					} else {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2);
						i386_neg_reg(REG_ITMP1);
						i386_alu_imm_reg(I386_ADC, 0, REG_ITMP2);
						i386_neg_reg(REG_ITMP2);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
					}
				}
			}
			break;

		case ICMD_I2L:        /* ..., value  ==> ..., value                   */

			d = reg_of_var(iptr->dst, REG_ITMP3);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(REG_SP, src->regoff * 4, I386_EAX);
					i386_cltd();
					i386_mov_reg_membase(I386_EAX, REG_SP, iptr->dst->regoff * 8);
					i386_mov_reg_membase(I386_EDX, REG_SP, iptr->dst->regoff * 8 + 4);

				} else {
					M_INTMOVE(src->regoff, I386_EAX);
					i386_cltd();
					i386_mov_reg_membase(I386_EAX, REG_SP, iptr->dst->regoff * 8);
					i386_mov_reg_membase(I386_EDX, REG_SP, iptr->dst->regoff * 8 + 4);
				}
			}
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */

			d = reg_of_var(iptr->dst, REG_ITMP3);
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

			d = reg_of_var(iptr->dst, REG_ITMP3);
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

			d = reg_of_var(iptr->dst, REG_ITMP3);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					i386_alu_imm_reg(I386_AND, 0x0000ffff, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

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

			d = reg_of_var(iptr->dst, REG_ITMP3);
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

			d = reg_of_var(iptr->dst, REG_ITMP3);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_alu_reg_membase(I386_ADD, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

					} else if (src->prev->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_reg_membase(I386_ADD, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

					} else {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_alu_membase_reg(I386_ADD, REG_SP, src->regoff * 8, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_reg_membase(I386_ADD, src->prev->regoff, REG_SP, iptr->dst->regoff * 8);

					} else {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_reg_reg(I386_ADD, src->prev->regoff, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						i386_alu_reg_membase(I386_ADD, src->regoff, REG_SP, iptr->dst->regoff * 8);
						
					} else {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_alu_reg_reg(I386_ADD, src->regoff, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					i386_mov_reg_membase(src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_alu_reg_membase(I386_ADD, src->regoff, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
					i386_alu_membase_reg(I386_ADD, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
					i386_alu_membase_reg(I386_ADD, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					i386_alu_membase_reg(I386_ADD, REG_SP, src->prev->regoff * 8, iptr->dst->regoff);

				} else {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_reg_reg(I386_ADD, src->prev->regoff, iptr->dst->regoff);

					} else {
						M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
						i386_alu_reg_reg(I386_ADD, src->regoff, iptr->dst->regoff);
					}
				}
			}
			break;

		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_ITMP3);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_imm_membase(I386_ADD, iptr->val.i, REG_SP, iptr->dst->regoff * 8);

					} else {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_imm_reg(I386_ADD, iptr->val.i, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					i386_mov_reg_membase(src->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_alu_imm_membase(I386_ADD, iptr->val.i, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);
					i386_alu_imm_reg(I386_ADD, iptr->val.i, iptr->dst->regoff);

				} else {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					i386_alu_imm_reg(I386_ADD, iptr->val.i, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_LADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			d = reg_of_var(iptr->dst, REG_ITMP3);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2);
						i386_alu_reg_membase(I386_ADD, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_alu_reg_membase(I386_ADC, REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);

					} else if (src->prev->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2);
						i386_alu_reg_membase(I386_ADD, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_alu_reg_membase(I386_ADC, REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);

					} else {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2);
						i386_alu_membase_reg(I386_ADD, REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_membase_reg(I386_ADC, REG_SP, src->regoff * 8 + 4, REG_ITMP2);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
					}

				}
			}
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(iptr->dst, REG_ITMP3);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_imm_membase(I386_ADD, iptr->val.l, REG_SP, iptr->dst->regoff * 8);
						i386_alu_imm_membase(I386_ADC, iptr->val.l >> 32, REG_SP, iptr->dst->regoff * 8 + 4);

					} else {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2);
						i386_alu_imm_reg(I386_ADD, iptr->val.l, REG_ITMP1);
						i386_alu_imm_reg(I386_ADC, iptr->val.l >> 32, REG_ITMP2);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
					}
				}
			}
			break;

		case ICMD_ISUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			d = reg_of_var(iptr->dst, REG_ITMP3);
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
					i386_mov_reg_membase(src->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_alu_reg_membase(I386_SUB, src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
					i386_alu_membase_reg(I386_SUB, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
					i386_alu_membase_reg(I386_SUB, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
					i386_alu_reg_reg(I386_SUB, src->regoff, iptr->dst->regoff);

				} else {
					M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
					i386_alu_reg_reg(I386_SUB, src->regoff, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_ITMP3);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_imm_membase(I386_SUB, iptr->val.i, REG_SP, iptr->dst->regoff * 8);

					} else {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_imm_reg(I386_SUB, iptr->val.i, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					i386_mov_reg_membase(src->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_alu_imm_membase(I386_SUB, iptr->val.i, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);
					i386_alu_imm_reg(I386_SUB, iptr->val.i, iptr->dst->regoff);

				} else {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					i386_alu_imm_reg(I386_SUB, iptr->val.i, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_LSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			d = reg_of_var(iptr->dst, REG_ITMP3);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2);
						i386_alu_reg_membase(I386_SUB, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_alu_reg_membase(I386_SBB, REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);

					} else {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2);
						i386_alu_membase_reg(I386_SUB, REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_membase_reg(I386_SBB, REG_SP, src->regoff * 8 + 4, REG_ITMP2);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
					}
				}
			}
			break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(iptr->dst, REG_ITMP3);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_imm_membase(I386_SUB, iptr->val.l, REG_SP, iptr->dst->regoff * 8);
						i386_alu_imm_membase(I386_SBB, iptr->val.l >> 32, REG_SP, iptr->dst->regoff * 8 + 4);

					} else {
						/* could be optimized with lea -- see gcc */
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2);
						i386_alu_imm_reg(I386_SUB, iptr->val.l, REG_ITMP1);
						i386_alu_imm_reg(I386_SBB, iptr->val.l >> 32, REG_ITMP2);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
					}
				}
			}
			break;

		case ICMD_IMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			d = reg_of_var(iptr->dst, REG_ITMP3);
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

			d = reg_of_var(iptr->dst, REG_ITMP3);
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

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
/*  					i386_mov_membase_reg(REG_SP, src->regoff * 8, I386_EAX);              /* mem -> EAX             */
/*  					i386_mul_membase(REG_SP, src->prev->regoff * 8);                      /* mem * EAX -> EDX:EAX   */
/*  					i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);       /* mem -> ITMP1           */
/*  					i386_mov_reg_reg(REG_ITMP1, REG_ITMP2);                               /* ITMP1 -> ITMP2         */
/*  					i386_imul_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2);        /* mem * ITMP2 -> ITMP2   */
/*  					i386_mov_reg_reg(I386_EDX, REG_ITMP1);                                /* EDX -> ITMP1           */
/*  					i386_alu_reg_reg(I386_ADD, REG_ITMP2, REG_ITMP1);                     /* ITMP2 + ITMP1 -> ITMP1 */
/*  					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP2);             /* mem -> ITMP2           */
/*  					i386_imul_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2);  /* mem * ITMP2 -> ITMP2   */

/*  					i386_alu_reg_reg(I386_ADD, REG_ITMP2, REG_ITMP1);                     /* ITMP2 + ITMP1 -> ITMP1 */
/*  					i386_mov_reg_reg(REG_ITMP1, I386_EDX);                                /* ITMP1 -> EDX           */
/*  					i386_mov_reg_membase(I386_EAX, REG_SP, iptr->dst->regoff * 8); */
/*  					i386_mov_reg_membase(I386_EDX, REG_SP, iptr->dst->regoff * 8 + 4); */

					i386_mov_membase_reg(REG_SP, src->regoff * 8, I386_EAX);              /* mem -> EAX             */
					i386_mul_membase(REG_SP, src->prev->regoff * 8);                      /* mem * EAX -> EDX:EAX   */
					/* TODO: optimize move EAX -> REG_ITMP3 */
					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP3);             /* mem -> ITMP3           */

					i386_imul_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP3);  /* mem * ITMP3 -> ITMP3   */
					i386_alu_reg_reg(I386_ADD, REG_ITMP3, I386_EDX);                      /* ITMP3 + EDX -> EDX     */
					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP3);             /* mem -> ITMP3           */
					i386_imul_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP3);  /* mem * ITMP3 -> ITMP3   */
					i386_alu_reg_reg(I386_ADD, REG_ITMP3, I386_EDX);                      /* ITMP3 + EDX -> EDX     */
					i386_mov_reg_membase(I386_EAX, REG_SP, iptr->dst->regoff * 8);
					i386_mov_reg_membase(I386_EDX, REG_SP, iptr->dst->regoff * 8 + 4);
				}
			}
			break;

		case ICMD_LMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					i386_mov_imm_reg(iptr->val.l, I386_EAX);                              /* imm -> EAX             */
					i386_mul_membase(REG_SP, src->regoff * 8);                            /* mem * EAX -> EDX:EAX   */
					/* TODO: optimize move EAX -> REG_ITMP3 */
					i386_mov_imm_reg(iptr->val.l, REG_ITMP3);                             /* imm -> ITMP3           */

					i386_imul_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP3);        /* mem * ITMP3 -> ITMP3   */
					i386_alu_reg_reg(I386_ADD, REG_ITMP3, I386_EDX);                      /* ITMP3 + EDX -> EDX     */
					i386_mov_imm_reg(iptr->val.l >> 32, REG_ITMP3);                       /* imm -> ITMP3           */
					i386_imul_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP3);        /* mem * ITMP3 -> ITMP3   */
					i386_alu_reg_reg(I386_ADD, REG_ITMP3, I386_EDX);                      /* ITMP3 + EDX -> EDX     */
					i386_mov_reg_membase(I386_EAX, REG_SP, iptr->dst->regoff * 8);
					i386_mov_reg_membase(I386_EDX, REG_SP, iptr->dst->regoff * 8 + 4);
				}
			}
			break;

		case ICMD_IDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			d = reg_of_var(iptr->dst, REG_ITMP3);
			if (src->prev->flags & INMEMORY) {
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, I386_EAX);

			} else {
				M_INTMOVE(src->prev->regoff, I386_EAX);
			}
			
			i386_cltd();

			if (src->flags & INMEMORY) {
				i386_idiv_membase(REG_SP, src->regoff * 8);

			} else {
				i386_idiv_reg(src->regoff);
			}

			if (iptr->dst->flags & INMEMORY) {
				i386_mov_reg_membase(I386_EAX, REG_SP, iptr->dst->regoff * 8);

			} else {
				M_INTMOVE(I386_EAX, iptr->dst->regoff);
			}
			break;

		case ICMD_IREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			d = reg_of_var(iptr->dst, REG_ITMP3);
			if (src->prev->flags & INMEMORY) {
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, I386_EAX);

			} else {
				M_INTMOVE(src->prev->regoff, I386_EAX);
			}
			
			i386_cltd();

			if (src->flags & INMEMORY) {
				i386_idiv_membase(REG_SP, src->regoff * 8);

			} else {
				i386_idiv_reg(src->regoff);
			}

			if (iptr->dst->flags & INMEMORY) {
				i386_mov_reg_membase(I386_EDX, REG_SP, iptr->dst->regoff * 8);

			} else {
				M_INTMOVE(I386_EDX, iptr->dst->regoff);
			}
			break;

		case ICMD_IDIVPOW2:   /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

/*  			d = reg_of_var(iptr->dst, REG_ITMP3); */
/*  			if (iptr->dst->flags & INMEMORY) { */
/*  				if (src->flags & INMEMORY) { */
/*  					if (src->regoff == iptr->dst->regoff) { */
/*  						i386_shift_imm_membase(I386_SAR, iptr->val.i, REG_SP, iptr->dst->regoff * 8); */

/*  					} else { */
/*  						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1); */
/*  						i386_shift_imm_reg(I386_SAR, iptr->val.i, REG_ITMP1); */
/*  						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8); */
/*  					} */

/*  				} else { */
/*  					i386_mov_reg_membase(src->regoff, REG_SP, iptr->dst->regoff * 8); */
/*  					i386_shift_imm_membase(I386_SAR, iptr->val.i, REG_SP, iptr->dst->regoff * 8); */
/*  				} */

/*  			} else { */
/*  				if (src->flags & INMEMORY) { */
/*  					i386_mov_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff); */
/*  					i386_shift_imm_reg(I386_SAR, iptr->val.i, iptr->dst->regoff); */

/*  				} else { */
/*  					M_INTMOVE(src->regoff, iptr->dst->regoff); */
/*  					i386_shift_imm_reg(I386_SAR, iptr->val.i, iptr->dst->regoff); */
/*  				} */
/*  			} */
			panic("IDIVPOW2");
			break;

		case ICMD_LDIVPOW2:   /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

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

			d = reg_of_var(iptr->dst, REG_ITMP2);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, I386_ECX);
						i386_shift_membase(I386_SHL, REG_SP, iptr->dst->regoff * 8);

					} else {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, I386_ECX);
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_shift_reg(I386_SHL, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, I386_ECX);
					i386_mov_reg_membase(src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_shift_membase(I386_SHL, REG_SP, iptr->dst->regoff * 8);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						M_INTMOVE(src->regoff, I386_ECX);
						i386_shift_membase(I386_SHL, REG_SP, iptr->dst->regoff * 8);
					} else {
						M_INTMOVE(src->regoff, I386_ECX);
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_shift_reg(I386_SHL, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					M_INTMOVE(src->regoff, I386_ECX);
					i386_mov_reg_membase(src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_shift_membase(I386_SHL, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, I386_ECX);
					i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
					i386_shift_reg(I386_SHL, iptr->dst->regoff);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, I386_ECX);
					M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
					i386_shift_reg(I386_SHL, iptr->dst->regoff);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->regoff, I386_ECX);
					i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
					i386_shift_reg(I386_SHL, iptr->dst->regoff);

				} else {
					M_INTMOVE(src->regoff, I386_ECX);
					M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
					i386_shift_reg(I386_SHL, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if ((src->flags & INMEMORY) && (iptr->dst->flags & INMEMORY)) {
				if (src->regoff == iptr->dst->regoff) {
					i386_shift_imm_membase(I386_SHL, iptr->val.i, REG_SP, iptr->dst->regoff * 8);

				} else {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					i386_shift_imm_reg(I386_SHL, iptr->val.i, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				}

			} else if ((src->flags & INMEMORY) && !(iptr->dst->flags & INMEMORY)) {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);
				i386_shift_imm_reg(I386_SHL, iptr->val.i, iptr->dst->regoff);
				
			} else if (!(src->flags & INMEMORY) && (iptr->dst->flags & INMEMORY)) {
				i386_mov_reg_membase(src->regoff, REG_SP, iptr->dst->regoff * 8);
				i386_shift_imm_membase(I386_SHL, iptr->val.i, REG_SP, iptr->dst->regoff * 8);

			} else {
				M_INTMOVE(src->regoff, iptr->dst->regoff);
				i386_shift_imm_reg(I386_SHL, iptr->val.i, iptr->dst->regoff);
			}
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			d = reg_of_var(iptr->dst, REG_ITMP2);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, I386_ECX);
						i386_shift_membase(I386_SAR, REG_SP, iptr->dst->regoff * 8);

					} else {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, I386_ECX);
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_shift_reg(I386_SAR, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, I386_ECX);
					i386_mov_reg_membase(src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_shift_membase(I386_SAR, REG_SP, iptr->dst->regoff * 8);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						M_INTMOVE(src->regoff, I386_ECX);
						i386_shift_membase(I386_SAR, REG_SP, iptr->dst->regoff * 8);

					} else {
						M_INTMOVE(src->regoff, I386_ECX);
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_shift_reg(I386_SAR, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					M_INTMOVE(src->regoff, I386_ECX);
					i386_mov_reg_membase(src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_shift_membase(I386_SAR, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, I386_ECX);
					i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
					i386_shift_reg(I386_SAR, iptr->dst->regoff);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, I386_ECX);
					M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
					i386_shift_reg(I386_SAR, iptr->dst->regoff);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->regoff, I386_ECX);
					i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
					i386_shift_reg(I386_SAR, iptr->dst->regoff);

				} else {
					M_INTMOVE(src->regoff, I386_ECX);
					M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
					i386_shift_reg(I386_SAR, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if ((src->flags & INMEMORY) && (iptr->dst->flags & INMEMORY)) {
				if (src->regoff == iptr->dst->regoff) {
					i386_shift_imm_membase(I386_SAR, iptr->val.i, REG_SP, iptr->dst->regoff * 8);

				} else {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					i386_shift_imm_reg(I386_SAR, iptr->val.i, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				}

			} else if ((src->flags & INMEMORY) && !(iptr->dst->flags & INMEMORY)) {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);
				i386_shift_imm_reg(I386_SAR, iptr->val.i, iptr->dst->regoff);
				
			} else if (!(src->flags & INMEMORY) && (iptr->dst->flags & INMEMORY)) {
				i386_mov_reg_membase(src->regoff, REG_SP, iptr->dst->regoff * 8);
				i386_shift_imm_membase(I386_SAR, iptr->val.i, REG_SP, iptr->dst->regoff * 8);

			} else {
				M_INTMOVE(src->regoff, iptr->dst->regoff);
				i386_shift_imm_reg(I386_SAR, iptr->val.i, iptr->dst->regoff);
			}
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			d = reg_of_var(iptr->dst, REG_ITMP2);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, I386_ECX);
						i386_shift_membase(I386_SHR, REG_SP, iptr->dst->regoff * 8);

					} else {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, I386_ECX);
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_shift_reg(I386_SHR, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, I386_ECX);
					i386_mov_reg_membase(src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_shift_membase(I386_SHR, REG_SP, iptr->dst->regoff * 8);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->prev->regoff == iptr->dst->regoff) {
						M_INTMOVE(src->regoff, I386_ECX);
						i386_shift_membase(I386_SHR, REG_SP, iptr->dst->regoff * 8);
					} else {
						M_INTMOVE(src->regoff, I386_ECX);
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_shift_reg(I386_SHR, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					M_INTMOVE(src->regoff, I386_ECX);
					i386_mov_reg_membase(src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_shift_membase(I386_SHR, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, I386_ECX);
					i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
					i386_shift_reg(I386_SHR, iptr->dst->regoff);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, I386_ECX);
					M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
					i386_shift_reg(I386_SHR, iptr->dst->regoff);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->regoff, I386_ECX);
					i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
					i386_shift_reg(I386_SHR, iptr->dst->regoff);

				} else {
					M_INTMOVE(src->regoff, I386_ECX);
					M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
					i386_shift_reg(I386_SHR, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if ((src->flags & INMEMORY) && (iptr->dst->flags & INMEMORY)) {
				if (src->regoff == iptr->dst->regoff) {
					i386_shift_imm_membase(I386_SHR, iptr->val.i, REG_SP, iptr->dst->regoff * 8);

				} else {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					i386_shift_imm_reg(I386_SHR, iptr->val.i, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
				}

			} else if ((src->flags & INMEMORY) && !(iptr->dst->flags & INMEMORY)) {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);
				i386_shift_imm_reg(I386_SHR, iptr->val.i, iptr->dst->regoff);
				
			} else if (!(src->flags & INMEMORY) && (iptr->dst->flags & INMEMORY)) {
				i386_mov_reg_membase(src->regoff, REG_SP, iptr->dst->regoff * 8);
				i386_shift_imm_membase(I386_SHR, iptr->val.i, REG_SP, iptr->dst->regoff * 8);

			} else {
				M_INTMOVE(src->regoff, iptr->dst->regoff);
				i386_shift_imm_reg(I386_SHR, iptr->val.i, iptr->dst->regoff);
			}
			break;

		case ICMD_LSHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY ){
				if (src->prev->flags & INMEMORY) {
					if (src->prev->regoff == iptr->dst->regoff) {
						if (src->flags & INMEMORY) {
							i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
							i386_mov_membase_reg(REG_SP, src->regoff * 8, I386_ECX);
							i386_shld_reg_membase(REG_ITMP1, REG_SP, src->prev->regoff * 8 + 4);
							i386_shift_membase(I386_SHL, REG_SP, iptr->dst->regoff * 8);

						} else {
							i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
							M_INTMOVE(src->regoff, I386_ECX);
							i386_shld_reg_membase(REG_ITMP1, REG_SP, src->prev->regoff * 8 + 4);
							i386_shift_membase(I386_SHL, REG_SP, iptr->dst->regoff * 8);
						}

					} else {
						if (src->flags & INMEMORY) {
							i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
							i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2);
							i386_mov_membase_reg(REG_SP, src->regoff * 8, I386_ECX);
							i386_shld_reg_reg(REG_ITMP1, REG_ITMP2);
							i386_shift_reg(I386_SHL, REG_ITMP1);
							i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
							i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);

						} else {
							i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
							i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2);
							M_INTMOVE(src->regoff, I386_ECX);
							i386_shld_reg_reg(REG_ITMP1, REG_ITMP2);
							i386_shift_reg(I386_SHL, REG_ITMP1);
							i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
							i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
						}
					}
				}
			}
			break;

/*  		case ICMD_LSHLCONST:  /* ..., value  ==> ..., value << constant       */
/*  		                      /* val.l = constant                             */

/*  			var_to_reg_int(s1, src, REG_ITMP1); */
/*  			d = reg_of_var(iptr->dst, REG_ITMP3); */
/*  			M_SLL_IMM(s1, iptr->val.l & 0x3f, d); */
/*  			store_reg_to_var_int(iptr->dst, d); */
/*  			break; */

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY ){
				if (src->prev->flags & INMEMORY) {
					if (src->prev->regoff == iptr->dst->regoff) {
						if (src->flags & INMEMORY) {
							i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
							i386_mov_membase_reg(REG_SP, src->regoff * 8, I386_ECX);
							i386_shrd_reg_membase(REG_ITMP1, REG_SP, src->prev->regoff * 8 + 4);
							i386_shift_membase(I386_SAR, REG_SP, iptr->dst->regoff * 8);

						} else {
							i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
							M_INTMOVE(src->regoff, I386_ECX);
							i386_shrd_reg_membase(REG_ITMP1, REG_SP, src->prev->regoff * 8 + 4);
							i386_shift_membase(I386_SAR, REG_SP, iptr->dst->regoff * 8);
						}

					} else {
						if (src->flags & INMEMORY) {
							i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
							i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2);
							i386_mov_membase_reg(REG_SP, src->regoff * 8, I386_ECX);
							i386_shrd_reg_reg(REG_ITMP1, REG_ITMP2);
							i386_shift_reg(I386_SAR, REG_ITMP1);
							i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
							i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);

						} else {
							i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
							i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2);
							M_INTMOVE(src->regoff, I386_ECX);
							i386_shrd_reg_reg(REG_ITMP1, REG_ITMP2);
							i386_shift_reg(I386_SAR, REG_ITMP1);
							i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
							i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
						}
					}
				}
			}
			break;

/*  		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
/*  		                      /* val.l = constant                             */

/*  			var_to_reg_int(s1, src, REG_ITMP1); */
/*  			d = reg_of_var(iptr->dst, REG_ITMP3); */
/*  			M_SRA_IMM(s1, iptr->val.l & 0x3f, d); */
/*  			store_reg_to_var_int(iptr->dst, d); */
/*  			break; */

		case ICMD_LUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY ){
				if (src->prev->flags & INMEMORY) {
					if (src->prev->regoff == iptr->dst->regoff) {
						if (src->flags & INMEMORY) {
							i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
							i386_mov_membase_reg(REG_SP, src->regoff * 8, I386_ECX);
							i386_shrd_reg_membase(REG_ITMP1, REG_SP, src->prev->regoff * 8 + 4);
							i386_shift_membase(I386_SHR, REG_SP, iptr->dst->regoff * 8);

						} else {
							i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
							M_INTMOVE(src->regoff, I386_ECX);
							i386_shrd_reg_membase(REG_ITMP1, REG_SP, src->prev->regoff * 8 + 4);
							i386_shift_membase(I386_SHR, REG_SP, iptr->dst->regoff * 8);
						}

					} else {
						if (src->flags & INMEMORY) {
							i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
							i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2);
							i386_mov_membase_reg(REG_SP, src->regoff * 8, I386_ECX);
							i386_shrd_reg_reg(REG_ITMP1, REG_ITMP2);
							i386_shift_reg(I386_SHR, REG_ITMP1);
							i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
							i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);

						} else {
							i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
							i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2);
							M_INTMOVE(src->regoff, I386_ECX);
							i386_shrd_reg_reg(REG_ITMP1, REG_ITMP2);
							i386_shift_reg(I386_SHR, REG_ITMP1);
							i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
							i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
						}
					}
				}
			}
			break;

/*  		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
/*  		                      /* val.l = constant                             */

/*  			var_to_reg_int(s1, src, REG_ITMP1); */
/*  			d = reg_of_var(iptr->dst, REG_ITMP3); */
/*  			M_SRL_IMM(s1, iptr->val.l & 0x3f, d); */
/*  			store_reg_to_var_int(iptr->dst, d); */
/*  			break; */

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_alu_reg_membase(I386_AND, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

					} else if (src->prev->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_reg_membase(I386_AND, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

					} else {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_alu_membase_reg(I386_AND, REG_SP, src->regoff * 8, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					i386_alu_reg_reg(I386_AND, src->prev->regoff, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
					i386_alu_reg_reg(I386_AND, src->regoff, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else {
					i386_mov_reg_membase(src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_alu_reg_membase(I386_AND, src->regoff, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
					i386_alu_membase_reg(I386_AND, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
					i386_alu_membase_reg(I386_AND, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					i386_alu_membase_reg(I386_AND, REG_SP, src->prev->regoff * 8, iptr->dst->regoff);

				} else {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_reg_reg(I386_AND, src->prev->regoff, iptr->dst->regoff);

					} else {
						M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
						i386_alu_reg_reg(I386_AND, src->regoff, iptr->dst->regoff);
					}
				}
			}
			break;

		case ICMD_IANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_imm_membase(I386_AND, iptr->val.i, REG_SP, iptr->dst->regoff * 8);

					} else {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_imm_reg(I386_AND, iptr->val.i, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					i386_mov_reg_membase(src->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_alu_imm_membase(I386_AND, iptr->val.i, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);
					i386_alu_imm_reg(I386_AND, iptr->val.i, iptr->dst->regoff);

				} else {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					i386_alu_imm_reg(I386_AND, iptr->val.i, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_LAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2);
						i386_alu_reg_membase(I386_AND, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_alu_reg_membase(I386_AND, REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);

					} else if (src->prev->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2);
						i386_alu_reg_membase(I386_AND, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_alu_reg_membase(I386_AND, REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);

					} else {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2);
						i386_alu_membase_reg(I386_AND, REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_membase_reg(I386_AND, REG_SP, src->regoff * 8 + 4, REG_ITMP2);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
					}
				}
			}
			break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_imm_membase(I386_AND, iptr->val.l, REG_SP, iptr->dst->regoff * 8);
						i386_alu_imm_membase(I386_AND, iptr->val.l >> 32, REG_SP, iptr->dst->regoff * 8 + 4);

					} else {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2);
						i386_alu_imm_reg(I386_AND, iptr->val.l, REG_ITMP1);
						i386_alu_imm_reg(I386_AND, iptr->val.l >> 32, REG_ITMP2);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
					}
				}
			}
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
/*  				ICONST(REG_ITMP2, iptr->val.i); */
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
/*  				LCONST(REG_ITMP2, iptr->val.l); */
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

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_alu_reg_membase(I386_OR, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

					} else if (src->prev->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_reg_membase(I386_OR, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

					} else {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_alu_membase_reg(I386_OR, REG_SP, src->regoff * 8, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					i386_alu_reg_reg(I386_OR, src->prev->regoff, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
					i386_alu_reg_reg(I386_OR, src->regoff, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else {
					i386_mov_reg_membase(src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_alu_reg_membase(I386_OR, src->regoff, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
					i386_alu_membase_reg(I386_OR, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
					i386_alu_membase_reg(I386_OR, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					i386_alu_membase_reg(I386_OR, REG_SP, src->prev->regoff * 8, iptr->dst->regoff);

				} else {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_reg_reg(I386_OR, src->prev->regoff, iptr->dst->regoff);

					} else {
						M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
						i386_alu_reg_reg(I386_OR, src->regoff, iptr->dst->regoff);
					}
				}
			}
			break;

		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_imm_membase(I386_OR, iptr->val.i, REG_SP, iptr->dst->regoff * 8);

					} else {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_imm_reg(I386_OR, iptr->val.i, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					i386_mov_reg_membase(src->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_alu_imm_membase(I386_OR, iptr->val.i, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);
					i386_alu_imm_reg(I386_OR, iptr->val.i, iptr->dst->regoff);

				} else {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					i386_alu_imm_reg(I386_OR, iptr->val.i, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_LOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2);
						i386_alu_reg_membase(I386_OR, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_alu_reg_membase(I386_OR, REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);

					} else if (src->prev->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2);
						i386_alu_reg_membase(I386_OR, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_alu_reg_membase(I386_OR, REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);

					} else {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2);
						i386_alu_membase_reg(I386_OR, REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_membase_reg(I386_OR, REG_SP, src->regoff * 8 + 4, REG_ITMP2);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
					}
				}
			}
			break;

		case ICMD_LORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_imm_membase(I386_OR, iptr->val.l, REG_SP, iptr->dst->regoff * 8);
						i386_alu_imm_membase(I386_OR, iptr->val.l >> 32, REG_SP, iptr->dst->regoff * 8 + 4);

					} else {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2);
						i386_alu_imm_reg(I386_OR, iptr->val.l, REG_ITMP1);
						i386_alu_imm_reg(I386_OR, iptr->val.l >> 32, REG_ITMP2);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
					}
				}
			}
			break;

		case ICMD_IXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_alu_reg_membase(I386_XOR, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

					} else if (src->prev->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_reg_membase(I386_XOR, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

					} else {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_alu_membase_reg(I386_XOR, REG_SP, src->regoff * 8, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					i386_alu_reg_reg(I386_XOR, src->prev->regoff, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
					i386_alu_reg_reg(I386_XOR, src->regoff, REG_ITMP1);
					i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);

				} else {
					i386_mov_reg_membase(src->prev->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_alu_reg_membase(I386_XOR, src->regoff, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, iptr->dst->regoff);
					i386_alu_membase_reg(I386_XOR, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if ((src->flags & INMEMORY) && !(src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
					i386_alu_membase_reg(I386_XOR, REG_SP, src->regoff * 8, iptr->dst->regoff);

				} else if (!(src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					i386_alu_membase_reg(I386_XOR, REG_SP, src->prev->regoff * 8, iptr->dst->regoff);

				} else {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_reg_reg(I386_XOR, src->prev->regoff, iptr->dst->regoff);

					} else {
						M_INTMOVE(src->prev->regoff, iptr->dst->regoff);
						i386_alu_reg_reg(I386_XOR, src->regoff, iptr->dst->regoff);
					}
				}
			}
			break;

		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.i = constant                             */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_imm_membase(I386_XOR, iptr->val.i, REG_SP, iptr->dst->regoff * 8);

					} else {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_imm_reg(I386_XOR, iptr->val.i, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
					}

				} else {
					i386_mov_reg_membase(src->regoff, REG_SP, iptr->dst->regoff * 8);
					i386_alu_imm_membase(I386_XOR, iptr->val.i, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, iptr->dst->regoff);
					i386_alu_imm_reg(I386_XOR, iptr->val.i, iptr->dst->regoff);

				} else {
					M_INTMOVE(src->regoff, iptr->dst->regoff);
					i386_alu_imm_reg(I386_XOR, iptr->val.i, iptr->dst->regoff);
				}
			}
			break;

		case ICMD_LXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
					if (src->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2);
						i386_alu_reg_membase(I386_XOR, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_alu_reg_membase(I386_XOR, REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);

					} else if (src->prev->regoff == iptr->dst->regoff) {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2);
						i386_alu_reg_membase(I386_XOR, REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_alu_reg_membase(I386_XOR, REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);

					} else {
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP2);
						i386_alu_membase_reg(I386_XOR, REG_SP, src->regoff * 8, REG_ITMP1);
						i386_alu_membase_reg(I386_XOR, REG_SP, src->regoff * 8 + 4, REG_ITMP2);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
					}
				}
			}
			break;

		case ICMD_LXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.l = constant                             */

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				if (src->flags & INMEMORY) {
					if (src->regoff == iptr->dst->regoff) {
						i386_alu_imm_membase(I386_XOR, iptr->val.l, REG_SP, iptr->dst->regoff * 8);
						i386_alu_imm_membase(I386_XOR, iptr->val.l >> 32, REG_SP, iptr->dst->regoff * 8 + 4);

					} else {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2);
						i386_alu_imm_reg(I386_XOR, iptr->val.l, REG_ITMP1);
						i386_alu_imm_reg(I386_XOR, iptr->val.l >> 32, REG_ITMP2);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, iptr->dst->regoff * 8);
						i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8 + 4);
					}
				}
			}
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
				if (iptr->val.i == 1) {
					i386_inc_membase(REG_SP, var->regoff * 8);
 
				} else if (iptr->val.i == -1) {
					i386_dec_membase(REG_SP, var->regoff * 8);

				} else {
					i386_alu_imm_membase(I386_ADD, iptr->val.i, REG_SP, var->regoff * 8);
				}

			} else {
				if (iptr->val.i == 1) {
					i386_inc_reg(var->regoff);
 
				} else if (iptr->val.i == -1) {
					i386_dec_reg(var->regoff);

				} else {
					i386_alu_imm_reg(I386_ADD, iptr->val.i, var->regoff);
				}
			}
			break;


		/* floating operations ************************************************/

		case ICMD_FNEG:       /* ..., value  ==> ..., - value                 */
		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */

			d = reg_of_var(iptr->dst, REG_FTMP3);
			i386_fchs();
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */
		case ICMD_DADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			d = reg_of_var(iptr->dst, REG_FTMP3);
			i386_faddp();
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */
		case ICMD_DSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			d = reg_of_var(iptr->dst, REG_FTMP3);
			i386_fsubp();
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */
		case ICMD_DMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			d = reg_of_var(iptr->dst, REG_FTMP3);
			i386_fmulp();
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */
		case ICMD_DDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			d = reg_of_var(iptr->dst, REG_FTMP3);
			i386_fdivp();
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_FREM:       /* ..., val1, val2  ==> ..., val1 % val2        */
		case ICMD_DREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			d = reg_of_var(iptr->dst, REG_FTMP3);
			i386_fprem1();
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_I2F:       /* ..., value  ==> ..., (float) value            */
		case ICMD_I2D:       /* ..., value  ==> ..., (double) value           */

			d = reg_of_var(iptr->dst, REG_FTMP1);
			if (src->flags & INMEMORY) {
				i386_fildl_membase(REG_SP, src->regoff * 8);

			} else {
				a = dseg_adds4(0);
				i386_mov_imm_reg(0, REG_ITMP1);
				dseg_adddata(mcodeptr);
				i386_mov_reg_membase(src->regoff, REG_ITMP1, a);
				i386_fildl_membase(REG_ITMP1, a);
			}
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_L2F:       /* ..., value  ==> ..., (float) value            */
		case ICMD_L2D:       /* ..., value  ==> ..., (double) value           */

			d = reg_of_var(iptr->dst, REG_FTMP1);
			if (src->flags & INMEMORY) {
				i386_fildll_membase(REG_SP, src->regoff * 8);
			} else {
				panic("longs have to be in memory");
			}
			store_reg_to_var_flt(iptr->dst, d);
			break;
			
		case ICMD_F2I:       /* ..., value  ==> ..., (int) value              */
		case ICMD_D2I:

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				i386_fistl_membase(REG_SP, iptr->dst->regoff * 8);

			} else {
				a = dseg_adds4(0);
				i386_mov_imm_reg(0, REG_ITMP1);
				dseg_adddata(mcodeptr);
				i386_fistpl_membase(REG_ITMP1, a);
				i386_mov_membase_reg(REG_ITMP1, a, iptr->dst->regoff);
			}
			break;

		case ICMD_F2L:       /* ..., value  ==> ..., (long) value             */
		case ICMD_D2L:

			d = reg_of_var(iptr->dst, REG_ITMP1);
			if (iptr->dst->flags & INMEMORY) {
				i386_fistpll_membase(REG_SP, iptr->dst->regoff * 8);
			} else {
				panic("longs have to be in memory");
			}
			break;

		case ICMD_F2D:       /* ..., value  ==> ..., (double) value           */

			/* nothing to do */
			break;

		case ICMD_D2F:       /* ..., value  ==> ..., (float) value            */

			/* nothing to do */
			break;

		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */
		case ICMD_DCMPL:

			d = reg_of_var(iptr->dst, REG_ITMP3);
/*  			i386_fxch(); */
			i386_fucom();
			i386_alu_reg_reg(I386_XOR, I386_EAX, I386_EAX);
			i386_fnstsw();
			i386_alu_imm_reg(I386_AND, 0x00004500, I386_EAX);
			i386_alu_imm_reg(I386_CMP, 0x00004000, I386_EAX);
/*  			i386_test_imm_reg(0x00004500, I386_EAX); */
/*  			i386_testb_imm_reg(0x45, 4); */

			if (iptr->dst->flags & INMEMORY) {
				int offset = 7;
				
				if (iptr->dst->regoff > 0) offset += 1;
				if (iptr->dst->regoff > 31) offset += 3;
				
				i386_jcc(I386_CC_NE, offset + 5);
				i386_mov_imm_membase(0, REG_SP, iptr->dst->regoff * 8);
				i386_jmp(offset);
				i386_mov_imm_membase(1, REG_SP, iptr->dst->regoff * 8);

/*  				i386_mov_imm_membase(0, REG_SP, iptr->dst->regoff * 8); */
/*  				i386_setcc_membase(I386_CC_E, REG_SP, iptr->dst->regoff * 8); */

			} else {
				i386_mov_imm_reg(0, iptr->dst->regoff);
				i386_setcc_reg(I386_CC_E, iptr->dst->regoff);
			}
			break;

		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */
		case ICMD_DCMPG:

			d = reg_of_var(iptr->dst, REG_ITMP3);
/*  			i386_fxch(); */
			i386_fucom();
			i386_fnstsw();
			i386_test_imm_reg(0x4500, I386_EAX);

			if (iptr->dst->flags & INMEMORY) {
				int offset = 7;

				if (iptr->dst->regoff > 0) offset += 1;
				if (iptr->dst->regoff > 31) offset += 3;

				i386_jcc(I386_CC_NE, offset + 5);
				i386_mov_imm_membase(-1, REG_SP, iptr->dst->regoff * 8);
				i386_jmp(offset);
				i386_mov_imm_membase(1, REG_SP, iptr->dst->regoff * 8);

			} else {
				i386_jcc(I386_CC_NE, 5 + 5);
				i386_mov_imm_reg(-1, iptr->dst->regoff);
				i386_jmp(5);
				i386_mov_imm_reg(1, iptr->dst->regoff);
			}
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

			/* TWISTI */
/*  #define gen_bound_check \ */
/*              if (checkbounds) { \ */
/*  				M_ILD(REG_ITMP3, s1, OFFSET(java_arrayheader, size));\ */
/*  				M_CMPULT(s2, REG_ITMP3, REG_ITMP3);\ */
/*  				M_BEQZ(REG_ITMP3, 0);\ */
/*  				mcode_addxboundrefs(mcodeptr); \ */
/*                  } */
#define gen_bound_check \
            if (checkbounds) { \
                i386_alu_reg_membase(I386_CMP, s2, s1, OFFSET(java_arrayheader, size)); \
                i386_jcc(I386_CC_L, 0); \
				mcode_addxboundrefs(mcodeptr); \
            }

		case ICMD_ARRAYLENGTH: /* ..., arrayref  ==> ..., length              */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			gen_nullptr_check(s1);
			i386_mov_membase_reg(s1, OFFSET(java_arrayheader, size), REG_ITMP3);
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
			i386_mov_memindex_reg(OFFSET(java_objectarray, data[0]), s1, s2, 2, REG_ITMP3);
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
				i386_mov_memindex_reg(OFFSET(java_longarray, data[0]), s1, s2, 2, REG_ITMP3);
				i386_mov_reg_membase(REG_ITMP3, REG_SP, iptr->dst->regoff * 8);
				i386_mov_memindex_reg(OFFSET(java_longarray, data[0]) + 4, s1, s2, 2, REG_ITMP3);
				i386_mov_reg_membase(REG_ITMP3, REG_SP, iptr->dst->regoff * 8 + 4);
			}
			break;

		case ICMD_IALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
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
			d = reg_of_var(iptr->dst, REG_FTMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			i386_flds_memindex(OFFSET(java_floatarray, data[0]), s1, s2, 2);
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
			i386_fldl_memindex(OFFSET(java_doublearray, data[0]), s1, s2, 2);
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
			M_INTMOVE(s1, REG_ITMP1);
			i386_movzwl_memindex_reg(OFFSET(java_chararray, data[0]), s1, s2, 1, d);
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
			M_INTMOVE(s1, REG_ITMP1);
			i386_movswl_memindex_reg(OFFSET(java_shortarray, data[0]), s1, s2, 1, d);
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
			M_INTMOVE(s1, REG_ITMP1);
			i386_movzbl_memindex_reg(OFFSET(java_bytearray, data[0]), s1, s2, 0, d);
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
				i386_mov_reg_memindex(REG_ITMP3, OFFSET(java_longarray, data[0]), s1, s2, 2);
				i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP3);
				i386_mov_reg_memindex(REG_ITMP3, OFFSET(java_longarray, data[0]) + 4, s1, s2, 2);
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
			i386_fsts_memindex(OFFSET(java_floatarray, data[0]), s1, s2, 2);
			break;

		case ICMD_DASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			i386_fstl_memindex(OFFSET(java_doublearray, data[0]), s1, s2, 2);
			break;

		case ICMD_CASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
			}
			var_to_reg_int(s3, src, REG_ITMP3);
			M_INTMOVE(s1, REG_ITMP1);
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
			M_INTMOVE(s1, REG_ITMP1);
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
			M_INTMOVE(s1, REG_ITMP1);
			i386_movb_reg_memindex(s3, OFFSET(java_bytearray, data[0]), s1, s2, 0);
			break;


		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */
		                      /* op1 = type, val.a = field address            */

			a = dseg_addaddress(&(((fieldinfo *)(iptr->val.a))->value));
			switch (iptr->op1) {
				case TYPE_INT:
				case TYPE_ADR:
					var_to_reg_int(s2, src, REG_ITMP1);
					i386_mov_imm_reg(0, REG_ITMP2);
					dseg_adddata(mcodeptr);
					i386_mov_membase_reg(REG_ITMP2, a, REG_ITMP3);
					i386_mov_reg_membase(REG_ITMP1, REG_ITMP3, 0);
					break;
				case TYPE_LNG:
					if (src->flags & INMEMORY) {
						i386_mov_imm_reg(0, REG_ITMP2);
						dseg_adddata(mcodeptr);
						i386_mov_membase_reg(REG_ITMP2, a, REG_ITMP3);
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP2);
						i386_mov_reg_membase(REG_ITMP1, REG_ITMP3, 0);
						i386_mov_reg_membase(REG_ITMP2, REG_ITMP3, 0 + 4);
					} else {
						panic("longs have to be in memory");
					}
					break;
				case TYPE_FLT:
					if (src->flags & INMEMORY) {
						i386_mov_imm_reg(0, REG_ITMP1);
						dseg_adddata(mcodeptr);
						i386_mov_membase_reg(REG_ITMP1, a, REG_ITMP2);
						i386_flds_membase(REG_SP, src->regoff * 8);
						i386_fsts_membase(REG_ITMP2, 0);
					} else {
						panic("floats have to be in memory");
					}
					break;
				case TYPE_DBL:
					if (src->flags & INMEMORY) {
						i386_mov_imm_reg(0, REG_ITMP1);
						dseg_adddata(mcodeptr);
						i386_mov_membase_reg(REG_ITMP1, a, REG_ITMP2);
						i386_fldl_membase(REG_SP, src->regoff * 8);
						i386_fstl_membase(REG_ITMP2, 0);
					} else {
						panic("doubles have to be in memory");
					}
					break;
				default: panic ("internal error");
				}
			break;

		case ICMD_GETSTATIC:  /* ...  ==> ..., value                          */
		                      /* op1 = type, val.a = field address            */

			a = dseg_addaddress(&(((fieldinfo *)(iptr->val.a))->value));
			i386_mov_imm_reg(0, REG_ITMP1);
			dseg_adddata(mcodeptr);
			i386_mov_membase_reg(REG_ITMP1, a, REG_ITMP1);
			switch (iptr->op1) {
				case TYPE_INT:
				case TYPE_ADR:
					d = reg_of_var(iptr->dst, REG_ITMP3);
					i386_mov_membase_reg(REG_ITMP1, 0, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
				case TYPE_LNG:
					d = reg_of_var(iptr->dst, REG_ITMP3);
					if (iptr->dst->flags & INMEMORY) {
						i386_mov_membase_reg(REG_ITMP1, 0, REG_ITMP2);
						i386_mov_membase_reg(REG_ITMP1, 4, REG_ITMP3);
						i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8);
						i386_mov_reg_membase(REG_ITMP3, REG_SP, iptr->dst->regoff * 8 + 4);
					} else {
						panic("longs have to be in memory");
					}
					break;
				case TYPE_FLT:
					d = reg_of_var(iptr->dst, REG_ITMP3);
					if (iptr->dst->flags & INMEMORY) {
						i386_flds_membase(REG_ITMP1, 0);
						i386_fsts_membase(REG_SP, iptr->dst->regoff * 8);
					} else {
						panic("floats have to be in memory");
					}
					break;
				case TYPE_DBL:				
					d = reg_of_var(iptr->dst, REG_ITMP3);
					if (iptr->dst->flags & INMEMORY) {
						i386_fldl_membase(REG_ITMP1, 0);
						i386_fstl_membase(REG_SP, iptr->dst->regoff * 8);
					} else {
						panic("doubles have to be in memory");
					}
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
						i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP3);
						i386_mov_reg_membase(REG_ITMP2, REG_ITMP1, a);
						i386_mov_reg_membase(REG_ITMP3, REG_ITMP1, a + 4);
					} else {
						panic("longs have to be in memory");
					}
					break;
				case TYPE_FLT:
					var_to_reg_int(s1, src->prev, REG_ITMP1);
					gen_nullptr_check(s1);
					if (src->flags & INMEMORY) {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP2);
						i386_mov_reg_membase(REG_ITMP2, REG_ITMP1, a);
					} else {
						panic("floats have to be in memory");
					}
					break;
				case TYPE_DBL:
					var_to_reg_int(s1, src->prev, REG_ITMP1);
					gen_nullptr_check(s1);
					if (src->flags & INMEMORY) {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP2);
						i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_ITMP3);
						i386_mov_reg_membase(REG_ITMP2, REG_ITMP1, a);
						i386_mov_reg_membase(REG_ITMP3, REG_ITMP1, a + 4);
					} else {
						panic("doubles have to be in memory");
					}
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
					i386_mov_membase_reg(s1, a, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
				case TYPE_LNG:
					var_to_reg_int(s1, src, REG_ITMP1);
/*  					d = reg_of_var(iptr->dst, REG_ITMP3); */
					gen_nullptr_check(s1);
					i386_mov_membase_reg(s1, a, REG_ITMP2);
					i386_mov_membase_reg(s1, a + 4, REG_ITMP3);
					i386_mov_reg_membase(REG_ITMP2, REG_SP, iptr->dst->regoff * 8);
					i386_mov_reg_membase(REG_ITMP3, REG_SP, iptr->dst->regoff * 8 + 4);
/*  					store_reg_to_var_int(iptr->dst, d); */
					break;
				case TYPE_ADR:
					var_to_reg_int(s1, src, REG_ITMP1);
					d = reg_of_var(iptr->dst, REG_ITMP3);
					gen_nullptr_check(s1);
					i386_mov_membase_reg(s1, a, d);
					store_reg_to_var_int(iptr->dst, d);
					break;
				case TYPE_FLT:
					var_to_reg_int(s1, src, REG_ITMP1);
					d = reg_of_var(iptr->dst, REG_FTMP1);
					gen_nullptr_check(s1);
					i386_flds_membase(s1, a);
/*    					store_reg_to_var_flt(iptr->dst, d); */
					break;
				case TYPE_DBL:				
					var_to_reg_int(s1, src, REG_ITMP1);
					d = reg_of_var(iptr->dst, REG_FTMP1);
					gen_nullptr_check(s1);
					i386_fldl_membase(s1, a);
/*  					store_reg_to_var_flt(iptr->dst, d); */
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
			i386_mov_imm_reg(asm_handle_exception, REG_ITMP2);
			i386_call_reg(REG_ITMP2);
			i386_nop();         /* nop ensures that XPC is less than the end */
			                    /* of basic block                            */
			ALIGNCODENOP;
			break;

		case ICMD_GOTO:         /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */

			i386_jmp(0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
  			ALIGNCODENOP;
			break;

		case ICMD_JSR:          /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */

			i386_call_imm(0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;
			
		case ICMD_RET:          /* ... ==> ...                                */
		                        /* op1 = local variable                       */

			i386_ret();
			break;

		case ICMD_IFNULL:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(I386_CMP, 0, REG_SP, src->regoff * 8);

			} else {
				i386_alu_imm_reg(I386_CMP, 0, src->regoff);
			}
			i386_jcc(I386_CC_E, 0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFNONNULL:    /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(I386_CMP, 0, REG_SP, src->regoff * 8);

			} else {
				i386_alu_imm_reg(I386_CMP, 0, src->regoff);
			}
			i386_jcc(I386_CC_NE, 0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFEQ:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(I386_CMP, iptr->val.i, REG_SP, src->regoff * 8);

			} else {
				i386_alu_imm_reg(I386_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(I386_CC_E, 0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFLT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(I386_CMP, iptr->val.i, REG_SP, src->regoff * 8);

			} else {
				i386_alu_imm_reg(I386_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(I386_CC_L, 0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFLE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(I386_CMP, iptr->val.i, REG_SP, src->regoff * 8);

			} else {
				i386_alu_imm_reg(I386_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(I386_CC_LE, 0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFNE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(I386_CMP, iptr->val.i, REG_SP, src->regoff * 8);

			} else {
				i386_alu_imm_reg(I386_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(I386_CC_NE, 0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFGT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(I386_CMP, iptr->val.i, REG_SP, src->regoff * 8);

			} else {
				i386_alu_imm_reg(I386_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(I386_CC_G, 0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFGE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(I386_CMP, iptr->val.i, REG_SP, src->regoff * 8);

			} else {
				i386_alu_imm_reg(I386_CMP, iptr->val.i, src->regoff);
			}
			i386_jcc(I386_CC_GE, 0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_LEQ:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			if (src->flags & INMEMORY) {
				if (iptr->val.l == 0) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					i386_alu_membase_reg(I386_OR, REG_SP, src->regoff * 8 + 4, REG_ITMP1);

				} else if (iptr->val.l > 0 && iptr->val.l <= 0x00000000ffffffff) {
					i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
					i386_alu_imm_reg(I386_XOR, iptr->val.l, REG_ITMP1);
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
			i386_jcc(I386_CC_NE, 0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_LLT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			/* TODO: optimize as in IF_LEQ */
			if (src->flags & INMEMORY) {
				int offset;
				i386_alu_imm_membase(I386_CMP, iptr->val.l >> 32, REG_SP, src->regoff * 8 + 4);
				i386_jcc(I386_CC_L, 0);
				mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);

				/* TODO: regoff exceeds 1 byte */
				offset = 4 + 6;
				if (src->regoff > 0) offset++;
				
				i386_jcc(I386_CC_G, offset);

				i386_alu_imm_membase(I386_CMP, iptr->val.l, REG_SP, src->regoff * 8);
				i386_jcc(I386_CC_B, 0);
				mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			}			
			break;

		case ICMD_IF_LLE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			/* TODO: optimize as in IF_LEQ */
			if (src->flags & INMEMORY) {
				int offset;
				i386_alu_imm_membase(I386_CMP, iptr->val.l >> 32, REG_SP, src->regoff * 8 + 4);
				i386_jcc(I386_CC_L, 0);
				mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);

				/* TODO: regoff exceeds 1 byte */
				offset = 4 + 6;
				if (src->regoff > 0) offset++;
				
				i386_jcc(I386_CC_G, offset);

				i386_alu_imm_membase(I386_CMP, iptr->val.l, REG_SP, src->regoff * 8);
				i386_jcc(I386_CC_BE, 0);
				mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			}			
			break;

		case ICMD_IF_LNE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			/* TODO: optimize for val.l == 0 */
			if (src->flags & INMEMORY) {
				i386_mov_imm_reg(iptr->val.l, REG_ITMP1);
				i386_mov_imm_reg(iptr->val.l >> 32, REG_ITMP2);
				i386_alu_membase_reg(I386_XOR, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_alu_membase_reg(I386_XOR, REG_SP, src->regoff * 8 + 4, REG_ITMP2);
				i386_alu_reg_reg(I386_OR, REG_ITMP2, REG_ITMP1);
				i386_test_reg_reg(REG_ITMP1, REG_ITMP1);
			}			
			i386_jcc(I386_CC_NE, 0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_LGT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			/* TODO: optimize as in IF_LEQ */
			if (src->flags & INMEMORY) {
				int offset;
				i386_alu_imm_membase(I386_CMP, iptr->val.l >> 32, REG_SP, src->regoff * 8 + 4);
				i386_jcc(I386_CC_G, 0);
				mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);

				/* TODO: regoff exceeds 1 byte */
				offset = 4 + 6;
				if (src->regoff > 0) offset++;
				if ((iptr->val.l & 0x00000000ffffffff) < -128 || (iptr->val.l & 0x00000000ffffffff) > 127) offset += 3;

				i386_jcc(I386_CC_L, offset);

				i386_alu_imm_membase(I386_CMP, iptr->val.l, REG_SP, src->regoff * 8);
				i386_jcc(I386_CC_A, 0);
				mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			}			
			break;

		case ICMD_IF_LGE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			/* TODO: optimize as in IF_LEQ */
			if (src->flags & INMEMORY) {
				int offset;
				i386_alu_imm_membase(I386_CMP, iptr->val.l >> 32, REG_SP, src->regoff * 8 + 4);
				i386_jcc(I386_CC_G, 0);
				mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);

				/* TODO: regoff exceeds 1 byte */
				offset = 4 + 6;
				if (src->regoff > 0) offset++;

				i386_jcc(I386_CC_L, offset);

				i386_alu_imm_membase(I386_CMP, iptr->val.l, REG_SP, src->regoff * 8);
				i386_jcc(I386_CC_AE, 0);
				mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_LCMPLT:    /* ..., value, value ==> ...                  */
	                            /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				int offset;
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP1);
				i386_alu_membase_reg(I386_CMP, REG_SP, src->regoff * 8 + 4, REG_ITMP1);
				i386_jcc(I386_CC_L, 0);
				mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);

				/* TODO: regoff exceeds 1 byte */
				offset = 3 + 3 + 6;
				if (src->prev->regoff > 0) offset++;
				if (src->regoff > 0) offset++;
				i386_jcc(I386_CC_G, offset);

				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_alu_membase_reg(I386_CMP, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_jcc(I386_CC_B, 0);
				mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_LCMPGT:    /* ..., value, value ==> ...                  */
                                /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				int offset;
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP1);
				i386_alu_membase_reg(I386_CMP, REG_SP, src->regoff * 8 + 4, REG_ITMP1);
				i386_jcc(I386_CC_G, 0);
				mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);

				/* TODO: regoff exceeds 1 byte */
				offset = 3 + 3 + 6;
				if (src->prev->regoff > 0) offset++;
				if (src->regoff > 0) offset++;
				i386_jcc(I386_CC_L, offset);

				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_alu_membase_reg(I386_CMP, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_jcc(I386_CC_A, 0);
				mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_LCMPLE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				int offset;
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP1);
				i386_alu_membase_reg(I386_CMP, REG_SP, src->regoff * 8 + 4, REG_ITMP1);
				i386_jcc(I386_CC_L, 0);
				mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);

				/* TODO: regoff exceeds 1 byte */
				offset = 3 + 3 + 6;
				if (src->prev->regoff > 0) offset++;
				if (src->regoff > 0) offset++;
				i386_jcc(I386_CC_G, offset);

				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_alu_membase_reg(I386_CMP, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_jcc(I386_CC_BE, 0);
				mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_LCMPGE:    /* ..., value, value ==> ...                  */
	                            /* op1 = target JavaVM pc                     */

			if ((src->flags & INMEMORY) && (src->prev->flags & INMEMORY)) {
				int offset;
				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8 + 4, REG_ITMP1);
				i386_alu_membase_reg(I386_CMP, REG_SP, src->regoff * 8 + 4, REG_ITMP1);
				i386_jcc(I386_CC_G, 0);
				mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);

				/* TODO: regoff exceeds 1 byte */
				offset = 3 + 3 + 6;
				if (src->prev->regoff > 0) offset++;
				if (src->regoff > 0) offset++;
				i386_jcc(I386_CC_L, offset);

				i386_mov_membase_reg(REG_SP, src->prev->regoff * 8, REG_ITMP1);
				i386_alu_membase_reg(I386_CMP, REG_SP, src->regoff * 8, REG_ITMP1);
				i386_jcc(I386_CC_AE, 0);
				mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			}			
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
/*  				ICONST(d, iptr[1].val.i); */
				}
			if ((s3 >= 0) && (s3 <= 255)) {
				M_CMOVEQ_IMM(s1, s3, d);
				}
			else {
/*  				ICONST(REG_ITMP2, s3); */
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
/*  				ICONST(d, iptr[1].val.i); */
				}
			if ((s3 >= 0) && (s3 <= 255)) {
				M_CMOVNE_IMM(s1, s3, d);
				}
			else {
/*  				ICONST(REG_ITMP2, s3); */
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
/*  				ICONST(d, iptr[1].val.i); */
				}
			if ((s3 >= 0) && (s3 <= 255)) {
				M_CMOVLT_IMM(s1, s3, d);
				}
			else {
/*  				ICONST(REG_ITMP2, s3); */
				M_CMOVLT(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFGE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			d = reg_of_var(iptr->dst, REG_ITMP3);
			if ((iptr[1].opc == ICMD_ELSE_ICONST)) {
				if (iptr->dst->flags & INMEMORY) {
					i386_mov_imm_membase(iptr[1].val.i, REG_SP, iptr->dst->regoff * 8);

				} else {
					i386_mov_imm_reg(iptr[1].val.i, iptr->dst->regoff);
				}
			}

			if (iptr->dst->flags & INMEMORY) {
				int offset = 7;

				if (iptr->dst->regoff > 0) offset += 1;
				if (iptr->dst->regoff > 31) offset += 3;
						
				if (src->flags & INMEMORY) {
					i386_alu_imm_membase(I386_CMP, 0, REG_SP, src->regoff * 8);
					i386_jcc(I386_CC_L, offset);
					i386_mov_imm_membase(iptr->val.i, REG_SP, iptr->dst->regoff * 8);

				} else {
					i386_alu_imm_reg(I386_CMP, 0, src->regoff);
					i386_jcc(I386_CC_L, offset);
					i386_mov_imm_membase(iptr->val.i, REG_SP, iptr->dst->regoff * 8);
				}

			} else {
				if (src->flags & INMEMORY) {
					i386_alu_imm_membase(I386_CMP, 0, REG_SP, src->regoff * 8);
					i386_jcc(I386_CC_L, 5);
					i386_mov_imm_reg(iptr->val.i, iptr->dst->regoff);

				} else {
					i386_alu_imm_reg(I386_CMP, 0, src->regoff);
					i386_jcc(I386_CC_L, 5);
					i386_mov_imm_reg(iptr->val.i, iptr->dst->regoff);
				}
			}
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
/*  				ICONST(d, iptr[1].val.i); */
				}
			if ((s3 >= 0) && (s3 <= 255)) {
				M_CMOVGT_IMM(s1, s3, d);
				}
			else {
/*  				ICONST(REG_ITMP2, s3); */
				M_CMOVGT(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IFLE_ICONST:  /* ..., value ==> ..., constant               */
		                        /* val.i = constant                           */

			d = reg_of_var(iptr->dst, REG_ITMP3);
			if ((iptr[1].opc == ICMD_ELSE_ICONST)) {
				if (iptr->dst->flags & INMEMORY) {
					i386_mov_imm_membase(iptr[1].val.i, REG_SP, iptr->dst->regoff * 8);

				} else {
					i386_mov_imm_reg(iptr[1].val.i, iptr->dst->regoff);
				}
			}

			if (iptr->dst->flags & INMEMORY) {
				int offset = 7;

				if (iptr->dst->regoff > 0) offset += 1;
				if (iptr->dst->regoff > 31) offset += 3;
						
				if (src->flags & INMEMORY) {
					i386_alu_imm_membase(I386_CMP, 0, REG_SP, src->regoff * 8);

				} else {
					i386_alu_imm_reg(I386_CMP, 0, src->regoff);
				}

				i386_jcc(I386_CC_G, offset);
				i386_mov_imm_membase(iptr->val.i, REG_SP, iptr->dst->regoff * 8);

			} else {
				if (src->flags & INMEMORY) {
					i386_alu_imm_membase(I386_CMP, 0, REG_SP, src->regoff * 8);

				} else {
					i386_alu_imm_reg(I386_CMP, 0, src->regoff);
				}

				i386_jcc(I386_CC_G, 5);
				i386_mov_imm_reg(iptr->val.i, iptr->dst->regoff);
			}
			break;


		case ICMD_IRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_ARETURN:

#ifdef USE_THREADS
			if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
				i386_mov_membase_reg(REG_SP, 8 * maxmemuse, REG_ITMP1);
				i386_alu_imm_reg(I386_SUB, 4, REG_SP);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, 0);
				i386_mov_imm_reg(builtin_monitorexit, REG_ITMP1);
				i386_call_reg(REG_ITMP1);
				i386_alu_imm_reg(I386_ADD, 4, REG_SP);
			}
#endif
			var_to_reg_int(s1, src, REG_RESULT);
			M_INTMOVE(s1, REG_RESULT);
			goto nowperformreturn;

		case ICMD_LRETURN:      /* ..., retvalue ==> ...                      */

#ifdef USE_THREADS
			if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
				i386_mov_membase_reg(REG_SP, 8 * maxmemuse, REG_ITMP1);
				i386_alu_imm_reg(I386_SUB, 4, REG_SP);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, 0);
				i386_mov_imm_reg(builtin_monitorexit, REG_ITMP1);
				i386_call_reg(REG_ITMP1);
				i386_alu_imm_reg(I386_ADD, 4, REG_SP);
			}
#endif
			if (src->flags & INMEMORY) {
				i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_RESULT);
				i386_mov_membase_reg(REG_SP, src->regoff * 8 + 4, REG_RESULT2);

			} else {
				panic("longs have to be in memory");
			}
			goto nowperformreturn;

		case ICMD_FRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_DRETURN:

#ifdef USE_THREADS
			if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
				i386_mov_membase_reg(REG_SP, 8 * maxmemuse, REG_ITMP1);
				i386_alu_imm_reg(I386_SUB, 4, REG_SP);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, 0);
				i386_mov_imm_reg(builtin_monitorexit, REG_ITMP1);
				i386_call_reg(REG_ITMP1);
				i386_alu_imm_reg(I386_ADD, 4, REG_SP);
			}
#endif
			/* TWISTI */
/*  			var_to_reg_flt(s1, src, REG_FRESULT); */
/*  			M_FLTMOVE(s1, REG_FRESULT); */
			/* should already be in st(0) */
			goto nowperformreturn;

		case ICMD_RETURN:      /* ...  ==> ...                                */

#ifdef USE_THREADS
			if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
				i386_mov_membase_reg(REG_SP, 8 * maxmemuse, REG_ITMP1);
				i386_alu_imm_reg(I386_SUB, 4, REG_SP);
				i386_mov_reg_membase(REG_ITMP1, REG_SP, 0);
				i386_mov_imm_reg(builtin_monitorexit, REG_ITMP1);
				i386_call_reg(REG_ITMP1);
				i386_alu_imm_reg(I386_ADD, 4, REG_SP);
			}
#endif

nowperformreturn:
			{
			int r, p;
			
			p = parentargs_base;
			
			/* restore return address                                         */

			/* TWISTI */
/* 			if (!isleafmethod) */
/* 				{p--;  M_LLD (REG_RA, REG_SP, 8 * p);} */
			if (!isleafmethod) {
				/* p--; M_LLD (REG_RA, REG_SP, 8 * p); -- do we really need this on i386 */
			}

			/* restore saved registers                                        */

			/* TWISTI */
/* 			for (r = savintregcnt - 1; r >= maxsavintreguse; r--) */
/* 					{p--; M_LLD(savintregs[r], REG_SP, 8 * p);} */
/* 			for (r = savfltregcnt - 1; r >= maxsavfltreguse; r--) */
/* 					{p--; M_DLD(savfltregs[r], REG_SP, 8 * p);} */
			for (r = savintregcnt - 1; r >= maxsavintreguse; r--) {
				p--; i386_pop_reg(savintregs[r]);
			}
			for (r = savfltregcnt - 1; r >= maxsavfltreguse; r--) {
				p--; M_DLD(savfltregs[r], REG_SP, 8 * p);
			}

			/* deallocate stack                                               */

			/* TWISTI */
/* 			if (parentargs_base) */
/* 				{M_LDA(REG_SP, REG_SP, parentargs_base*8);} */
			if (parentargs_base) {
				i386_alu_imm_reg(I386_ADD, parentargs_base * 8, REG_SP);
			}

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

			/* TWISTI */
/* 			M_RET(REG_ZERO, REG_RA); */
/* 			ALIGNCODENOP; */
			i386_ret();
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

			/* TWISTI */			
/*  			var_to_reg_int(s1, src, REG_ITMP1); */
/*  			if (l == 0) */
/*  				{M_INTMOVE(s1, REG_ITMP1);} */
/*  			else if (l <= 32768) { */
/*  				M_LDA(REG_ITMP1, s1, -l); */
/*  				} */
/*  			else { */
/*  				ICONST(REG_ITMP2, l); */
/*  				M_ISUB(s1, REG_ITMP2, REG_ITMP1); */
/*  				} */
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

			/* TWISTI */
/*  			for (; --s3 >= 0; src = src->prev) { */
/*  				if (src->varkind == ARGVAR) */
/*  					continue; */
/*  				if (IS_INT_LNG_TYPE(src->type)) { */
/*  					if (s3 < INT_ARG_CNT) { */
/*  						s1 = argintregs[s3]; */
/*  						var_to_reg_int(d, src, s1); */
/*  						M_INTMOVE(d, s1); */
/*  						} */
/*  					else  { */
/*  						var_to_reg_int(d, src, REG_ITMP1); */
/*  						M_LST(d, REG_SP, 8 * (s3 - INT_ARG_CNT)); */
/*  						} */
/*  					} */
/*  				else */
/*  					if (s3 < FLT_ARG_CNT) { */
/*  						s1 = argfltregs[s3]; */
/*  						var_to_reg_flt(d, src, s1); */
/*  						M_FLTMOVE(d, s1); */
/*  						} */
/*  					else { */
/*  						var_to_reg_flt(d, src, REG_FTMP1); */
/*  						M_DST(d, REG_SP, 8 * (s3 - FLT_ARG_CNT)); */
/*  						} */
/*  				} /* end of for */

#if 0
			s2 = s3;    /* save count of arguments */
			if (s3) {
				int i = s3;
				stackptr tmpsrc = src;
				s2 = 0;
				for (; --i >= 0; tmpsrc = tmpsrc->prev) {
					switch (tmpsrc->type) {
					case TYPE_INT:
					case TYPE_ADR:
						s2++;
						break;

					case TYPE_LNG:
						s2 += 2;
						break;
					}
				}

				i386_alu_imm_reg(I386_SUB, s2 * 4, REG_SP);
			}
#endif
			for (; --s3 >= 0; src = src->prev) {
/*  				printf("regoff=%d\n", src->regoff); */
				if (src->varkind == ARGVAR) {
/*  					printf("ARGVAR\n"); */
					continue;
				}
#if 0
					if (src->flags & INMEMORY) {
/*  						printf("INMEMORY: type=%d\n", src->type); */
						if (src->type == TYPE_LNG) {
							i386_mov_membase_reg(REG_SP, s2 * 4 + src->regoff * 8, REG_ITMP1);
							i386_mov_reg_membase(REG_ITMP1, REG_SP, s3 * 4);
							i386_mov_membase_reg(REG_SP, s2 * 4 + src->regoff * 8 + 4, REG_ITMP1);
							i386_mov_reg_membase(REG_ITMP1, REG_SP, s3 * 4 + 4);

						} else {
							i386_mov_membase_reg(REG_SP, s2 * 4 + src->regoff * 8, REG_ITMP1);
							i386_mov_reg_membase(REG_ITMP1, REG_SP, s3 * 4);
						}

					} else {
						i386_mov_reg_membase(src->regoff, REG_SP, s3 * 4);
					}

				} else if (IS_INT_LNG_TYPE(src->type)) {
#endif
				if (IS_INT_LNG_TYPE(src->type)) {
					if (src->flags & INMEMORY) {
						i386_mov_membase_reg(REG_SP, src->regoff * 8, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, s3 * 8);

					} else {
						i386_mov_reg_membase(src->regoff, REG_SP, s3 * 4);
					}
				} else
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
					/* TWISTI */
/*  					a = dseg_addaddress ((void*) (m)); */

/*  					M_ALD(REG_PV, REG_PV, a); /* Pointer to built-in-function */
/*  					d = iptr->op1; */

					a = (s4) m;
					d = iptr->op1;
					i386_mov_imm_reg(a, REG_ITMP1);
					i386_call_reg(REG_ITMP1);
					goto makeactualcall;

				case ICMD_INVOKESTATIC:
				case ICMD_INVOKESPECIAL:
					/* TWISTI */
/* 					a = dseg_addaddress (m->stubroutine); */

/* 					M_ALD(REG_PV, REG_PV, a );       /* method pointer in r27 */

/* 					d = m->returntype; */
/* 					goto makeactualcall; */

					a = (s4) m->stubroutine;
					d = m->returntype;
					i386_mov_imm_reg(a, REG_ITMP2);
					i386_call_reg(REG_ITMP2);
					goto makeactualcall;

				case ICMD_INVOKEVIRTUAL:

					/* TWISTI */
/*  					gen_nullptr_check(argintregs[0]); */
/*  					M_ALD(REG_METHODPTR, argintregs[0], */
/*  					                         OFFSET(java_objectheader, vftbl)); */
/*  					M_ALD(REG_PV, REG_METHODPTR, OFFSET(vftbl, table[0]) + */
/*  					                        sizeof(methodptr) * m->vftblindex); */

/*  					d = m->returntype; */

					i386_mov_membase_reg(REG_SP, 0, REG_ITMP2);
					gen_nullptr_check(REG_ITMP2);
					i386_mov_membase_reg(REG_ITMP2, OFFSET(java_objectheader, vftbl), REG_ITMP3);
					i386_mov_membase_reg(REG_ITMP3, OFFSET(vftbl, table[0]) + sizeof(methodptr) * m->vftblindex, REG_ITMP1);

					d = m->returntype;
					i386_call_reg(REG_ITMP1);
					goto makeactualcall;

				case ICMD_INVOKEINTERFACE:
					ci = m->class;

					/* TWISTI */					
/*  					gen_nullptr_check(argintregs[0]); */
/*  					M_ALD(REG_METHODPTR, argintregs[0], */
/*  					                         OFFSET(java_objectheader, vftbl));     */
/*  					M_ALD(REG_METHODPTR, REG_METHODPTR, */
/*  					      OFFSET(vftbl, interfacetable[0]) - */
/*  					      sizeof(methodptr*) * ci->index); */
/*  					M_ALD(REG_PV, REG_METHODPTR, */
/*  					                    sizeof(methodptr) * (m - ci->methods)); */

					i386_mov_membase_reg(REG_SP, 0, REG_ITMP2);
					gen_nullptr_check(REG_ITMP2);
					i386_mov_membase_reg(REG_ITMP2, OFFSET(java_objectheader, vftbl), REG_ITMP3);
					i386_mov_membase_reg(REG_ITMP3, OFFSET(vftbl, interfacetable[0]) - sizeof(methodptr) * ci->index, REG_ITMP3);
					i386_mov_membase_reg(REG_ITMP3, sizeof(methodptr) * (m - ci->methods), REG_ITMP1);

					d = m->returntype;
					i386_call_reg(REG_ITMP1);
					goto makeactualcall;

				default:
					d = 0;
					sprintf (logtext, "Unkown ICMD-Command: %d", iptr->opc);
					error ();
				}

makeactualcall:

			/* TWISTI */
/* 			M_JSR (REG_RA, REG_PV); */
/*  			i386_call_reg(I386_EAX); */

/*  			if (s2) { */
/*  				i386_alu_imm_reg(I386_ADD, s2 * 4, I386_ESP); */
/*  			} */

			/* d contains return type */

			/* TWISTI */
/*  			if (d != TYPE_VOID) { */
/*  				if (IS_INT_LNG_TYPE(iptr->dst->type)) { */
/*  					s1 = reg_of_var(iptr->dst, REG_RESULT); */
/*  					M_INTMOVE(REG_RESULT, s1); */
/*  					store_reg_to_var_int(iptr->dst, s1); */
/*  					} */
/*  				else { */
/*  					s1 = reg_of_var(iptr->dst, REG_FRESULT); */
/*  					M_FLTMOVE(REG_FRESULT, s1); */
/*  					store_reg_to_var_flt(iptr->dst, s1); */
/*  					} */
/*  				} */
/*  			} */
			if (d != TYPE_VOID) {
				d = reg_of_var(iptr->dst, REG_ITMP3);

				if (IS_INT_LNG_TYPE(iptr->dst->type)) {
					if (iptr->dst->type == TYPE_LNG) {
						if (iptr->dst->flags & INMEMORY) {
							i386_mov_reg_membase(REG_RESULT, REG_SP, iptr->dst->regoff * 8);
							i386_mov_reg_membase(REG_RESULT2, REG_SP, iptr->dst->regoff * 8 + 4);

						} else {
							panic("longs have to be in memory");
						}

					} else {
						if (iptr->dst->flags & INMEMORY) {
							i386_mov_reg_membase(REG_RESULT, REG_SP, iptr->dst->regoff * 8);

						} else {
							M_INTMOVE(REG_RESULT, iptr->dst->regoff);
						}
					}

				} else {
					/* nothing to do */
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
/*  			if (s1 == d) { */
/*  				M_MOV(s1, REG_ITMP1); */
/*  				s1 = REG_ITMP1; */
/*  			} */
  			i386_alu_reg_reg(I386_XOR, d, d);
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
					/* TWISTI */					
/*  					M_BEQZ(s1, 7); */
/*  					M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl)); */
/*  					a = dseg_addaddress ((void*) super->vftbl); */
/*  					M_ALD(REG_ITMP2, REG_PV, a); */
/*  					M_ILD(REG_ITMP1, REG_ITMP1, OFFSET(vftbl, baseval)); */
/*  					M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl, baseval)); */
/*  					M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl, diffval)); */
/*  					M_ISUB(REG_ITMP1, REG_ITMP3, REG_ITMP1); */
/*  					M_CMPULE(REG_ITMP1, REG_ITMP2, d); */

					int offset = 0;
					i386_alu_imm_reg(I386_CMP, 0, s1);

					/* TODO: clean up this calculation */
					offset += 2;
					if (OFFSET(java_objectheader, vftbl) > 0) offset += 1;
					if (OFFSET(java_objectheader, vftbl) > 255) offset += 3;

					offset += 5;

					offset += 2;
					if (OFFSET(vftbl, baseval) > 0) offset += 1;
					if (OFFSET(vftbl, baseval) > 255) offset += 3;
					
					offset += 2;
					if (OFFSET(vftbl, baseval) > 0) offset += 1;
					if (OFFSET(vftbl, baseval) > 255) offset += 3;
					
					offset += 2;
					if (OFFSET(vftbl, diffval) > 0) offset += 1;
					if (OFFSET(vftbl, diffval) > 255) offset += 3;
					
					offset += 2;
					offset += 2;
					offset += 2;

					offset += 3;

					i386_jcc(I386_CC_E, offset);

					i386_mov_membase_reg(s1, OFFSET(java_objectheader, vftbl), REG_ITMP1);
					i386_mov_imm_reg((void *) super->vftbl, REG_ITMP2);
					i386_mov_membase_reg(REG_ITMP1, OFFSET(vftbl, baseval), REG_ITMP1);
					i386_mov_membase_reg(REG_ITMP2, OFFSET(vftbl, baseval), REG_ITMP3);
					i386_mov_membase_reg(REG_ITMP2, OFFSET(vftbl, diffval), REG_ITMP2);
					i386_alu_reg_reg(I386_SUB, REG_ITMP3, REG_ITMP1);
					i386_alu_reg_reg(I386_XOR, d, d);
					i386_alu_reg_reg(I386_CMP, REG_ITMP2, REG_ITMP1);
					i386_setcc_reg(I386_CC_BE, d);
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
					/* TWISTI */
/*  					M_BEQZ(s1, 8 + (d == REG_ITMP3)); */
/*  					M_ALD(REG_ITMP1, s1, OFFSET(java_objectheader, vftbl)); */
/*  					a = dseg_addaddress ((void*) super->vftbl); */
/*  					M_ALD(REG_ITMP2, REG_PV, a); */
/*  					M_ILD(REG_ITMP1, REG_ITMP1, OFFSET(vftbl, baseval)); */
/*  					if (d != REG_ITMP3) { */
/*  						M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl, baseval)); */
/*  						M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl, diffval)); */
/*  						M_ISUB(REG_ITMP1, REG_ITMP3, REG_ITMP1); */
/*  						} */
/*  					else { */
/*  						M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl, baseval)); */
/*  						M_ISUB(REG_ITMP1, REG_ITMP2, REG_ITMP1); */
/*  						M_ALD(REG_ITMP2, REG_PV, a); */
/*  						M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl, diffval)); */
/*  						} */
/*  					M_CMPULE(REG_ITMP1, REG_ITMP2, REG_ITMP2); */
/*  					M_BEQZ(REG_ITMP2, 0); */
/*  					mcode_addxcastrefs(mcodeptr); */

					int offset = 0;
					i386_alu_imm_reg(I386_CMP, 0, s1);

					/* TODO: calculate the offset for jumping over the whole stuff */
					offset += 2;
					if (OFFSET(java_objectheader, vftbl) > 0) offset += 1;
					if (OFFSET(java_objectheader, vftbl) > 255) offset += 3;

					offset += 5;

					offset += 2;
					if (OFFSET(vftbl, baseval) > 0) offset += 1;
					if (OFFSET(vftbl, baseval) > 255) offset += 3;

					if (d != REG_ITMP3) {
						offset += 2;
						if (OFFSET(vftbl, baseval) > 0) offset += 1;
						if (OFFSET(vftbl, baseval) > 255) offset += 3;
						
						offset += 2;
						if (OFFSET(vftbl, diffval) > 0) offset += 1;
						if (OFFSET(vftbl, diffval) > 255) offset += 3;

						offset += 2;
						
					} else {
						offset += 2;
						if (OFFSET(vftbl, baseval) > 0) offset += 1;
						if (OFFSET(vftbl, baseval) > 255) offset += 3;

						offset += 2;

						offset += 5;

						offset += 2;
						if (OFFSET(vftbl, diffval) > 0) offset += 1;
						if (OFFSET(vftbl, diffval) > 255) offset += 3;
					}

					offset += 2;

					offset += 6;

					i386_jcc(I386_CC_E, offset);

					i386_mov_membase_reg(s1, OFFSET(java_objectheader, vftbl), REG_ITMP1);
					i386_mov_imm_reg((void *) super->vftbl, REG_ITMP2);
					i386_mov_membase_reg(REG_ITMP1, OFFSET(vftbl, baseval), REG_ITMP1);
					if (d != REG_ITMP3) {
						i386_mov_membase_reg(REG_ITMP2, OFFSET(vftbl, baseval), REG_ITMP3);
						i386_mov_membase_reg(REG_ITMP2, OFFSET(vftbl, diffval), REG_ITMP2);
						i386_alu_reg_reg(I386_SUB, REG_ITMP3, REG_ITMP1);

					} else {
						i386_mov_membase_reg(REG_ITMP2, OFFSET(vftbl, baseval), REG_ITMP2);
						i386_alu_reg_reg(I386_SUB, REG_ITMP2, REG_ITMP1);
						i386_mov_imm_reg((void *) super->vftbl, REG_ITMP2);
						i386_mov_membase_reg(REG_ITMP2, OFFSET(vftbl, diffval), REG_ITMP2);
					}
					i386_alu_reg_reg(I386_CMP, REG_ITMP2, REG_ITMP1);
					i386_jcc(I386_CC_B, 0);
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

			if (src->flags & INMEMORY) {
				i386_alu_imm_membase(I386_CMP, 0, REG_SP, src->regoff * 8);
				
			} else {
				i386_alu_imm_reg(I386_CMP, 0, src->regoff);
			}
			i386_jcc(I386_CC_L, 0);
			mcode_addxcheckarefs(mcodeptr);
			break;

		case ICMD_MULTIANEWARRAY:/* ..., cnt1, [cnt2, ...] ==> ..., arrayref  */
		                      /* op1 = dimension, val.a = array descriptor    */

			/* check for negative sizes and copy sizes to stack if necessary  */

			MCODECHECK((iptr->op1 << 1) + 64);

			for (s1 = iptr->op1; --s1 >= 0; src = src->prev) {
/*  			var_to_reg_int(s2, src, REG_ITMP1); */
				if (src->flags & INMEMORY) {
					i386_alu_imm_membase(I386_CMP, 0, REG_SP, src->regoff * 8);

				} else {
					i386_alu_imm_reg(I386_CMP, 0, src->regoff);
				}
				mcode_addxcheckarefs(mcodeptr);

				/* copy sizes to stack (argument numbers >= INT_ARG_CNT)      */

				if (src->varkind != ARGVAR) {
/*  				M_LST(s2, REG_SP, 8 * (s1 + INT_ARG_CNT)); */
					if (src->flags & INMEMORY) {
						i386_mov_membase_reg(REG_SP, (src->regoff + INT_ARG_CNT) * 8, REG_ITMP1);
						i386_mov_reg_membase(REG_ITMP1, REG_SP, (s1 + INT_ARG_CNT) * 8);

					} else {
						i386_mov_reg_membase(src->regoff, REG_SP, (s1 + INT_ARG_CNT) * 8);
					}
				}
			}

			/* a0 = dimension count */

			/* TWISTI */
/*  			ICONST(argintregs[0], iptr->op1); */
			i386_mov_imm_membase(iptr->op1, REG_SP, -12);

			/* a1 = arraydescriptor */

/*  			a = dseg_addaddress(iptr->val.a); */
/*  			M_ALD(argintregs[1], REG_PV, a); */
			i386_mov_imm_membase(iptr->val.a, REG_SP, -8);

			/* a2 = pointer to dimensions = stack pointer */

/*  			M_INTMOVE(REG_SP, argintregs[2]); */
			i386_mov_reg_membase(REG_SP, REG_SP, -4);

/*  			a = dseg_addaddress((void*) (builtin_nmultianewarray)); */
/*  			M_ALD(REG_PV, REG_PV, a); */
/*  			M_JSR(REG_RA, REG_PV); */
			i386_call_imm((void*) (builtin_nmultianewarray));
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

		i386_mov_imm_reg(0, REG_ITMP1);
		dseg_adddata(mcodeptr);
		i386_mov_membase_reg(REG_ITMP1, xboundrefs->branchpos - 4, REG_ITMP2_XPC);

		if (xcodeptr != NULL) {
			i386_jmp((xcodeptr - mcodeptr) - 1);

		} else {
			xcodeptr = mcodeptr;

			i386_mov_imm_reg(proto_java_lang_ArrayIndexOutOfBoundsException, REG_ITMP1_XPTR);
			i386_mov_imm_reg(asm_handle_exception, REG_ITMP3);
			i386_jmp_reg(REG_ITMP3);
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

		i386_mov_imm_reg(0, REG_ITMP1);
		dseg_adddata(mcodeptr);
		i386_mov_membase_reg(REG_ITMP1, xcheckarefs->branchpos - 4, REG_ITMP2_XPC);

		if (xcodeptr != NULL) {
			i386_jmp((xcodeptr - mcodeptr) - 1);

		} else {
			xcodeptr = mcodeptr;

			i386_mov_imm_reg(proto_java_lang_NegativeArraySizeException, REG_ITMP1_XPTR);
			i386_mov_imm_reg(asm_handle_exception, REG_ITMP3);
			i386_jmp_reg(REG_ITMP3);
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

		i386_mov_imm_reg(0, REG_ITMP1);
		dseg_adddata(mcodeptr);
		i386_mov_membase_reg(REG_ITMP1, xcastrefs->branchpos - 4, REG_ITMP2_XPC);

		if (xcodeptr != NULL) {
			i386_jmp((xcodeptr - mcodeptr) - 1);
		
		} else {
			xcodeptr = mcodeptr;

			i386_mov_imm_reg(proto_java_lang_ClassCastException, REG_ITMP1_XPTR);
			i386_mov_imm_reg(asm_handle_exception, REG_ITMP3);
			i386_jmp_reg(REG_ITMP3);
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

		i386_mov_imm_reg(0, REG_ITMP1);
		dseg_adddata(mcodeptr);
		i386_mov_membase_reg(REG_ITMP1, xnullrefs->branchpos - 4, REG_ITMP2_XPC);

		if (xcodeptr != NULL) {
			M_BR((xcodeptr-mcodeptr)-1);

		} else {
			xcodeptr = mcodeptr;

			i386_mov_imm_reg(proto_java_lang_NullPointerException, REG_ITMP1_XPTR);
			i386_mov_imm_reg(asm_handle_exception, REG_ITMP3);
			i386_jmp_reg(REG_ITMP3);
		}
	}

#endif
	}

	mcode_finish((int)((u1*) mcodeptr - mcodebase));
}


/* function createcompilerstub *************************************************

	creates a stub routine which calls the compiler
	
*******************************************************************************/

#define COMPSTUBSIZE 3

u1 *createcompilerstub (methodinfo *m)
{
	u8 *s = CNEW (u8, COMPSTUBSIZE);    /* memory to hold the stub            */
	s4 *p = (s4*) s;                    /* code generation pointer            */

	s4 *mcodeptr = p;					/* make macros work                   */
	
	                                    /* code for the stub                  */
	i386_mov_imm_reg(m, I386_EAX);      /* pass method pointer to compiler    */
	i386_mov_imm_reg(asm_call_jit_compiler, REG_ITMP2);    /* load address    */
	i386_jmp_reg(REG_ITMP2);            /* jump to compiler                   */

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

#define NATIVESTUBSIZE 18

u1 *createnativestub (functionptr f, methodinfo *m)
{
	u8 *s = CNEW (u8, NATIVESTUBSIZE);  /* memory to hold the stub            */
	s4 *p = (s4*) s;                    /* code generation pointer            */

	/* TWISTI: get rid of those 2nd defines */
	s4 *mcodeptr = p;
	
	reg_init();

	/* TWISTI */
/*  	M_MOV  (argintregs[4],argintregs[5]);  */
/*  	M_FMOV (argfltregs[4],argfltregs[5]); */

/*  	M_MOV  (argintregs[3],argintregs[4]); */
/*  	M_FMOV (argfltregs[3],argfltregs[4]); */

/*  	M_MOV  (argintregs[2],argintregs[3]); */
/*  	M_FMOV (argfltregs[2],argfltregs[3]); */

/*  	M_MOV  (argintregs[1],argintregs[2]); */
/*  	M_FMOV (argfltregs[1],argfltregs[2]); */

/*  	M_MOV  (argintregs[0],argintregs[1]); */
/*  	M_FMOV (argfltregs[0],argfltregs[1]); */

/*  	M_ALD  (argintregs[0], REG_PV, 17*8); /* load adress of jni_environement  */

/*  	M_LDA  (REG_SP, REG_SP, -8);        /* build up stackframe                */
/*  	M_AST  (REG_RA, REG_SP, 0);         /* store return address               */

/*  	M_ALD  (REG_PV, REG_PV, 14*8);      /* load adress of native method       */
/*  	M_JSR  (REG_RA, REG_PV);            /* call native method                 */

	i386_alu_imm_reg(I386_SUB, 24, REG_SP); /* 20 = 5 * 4 (5 params * 4 bytes)    */

	i386_mov_membase_reg(REG_SP, 24 + 4, REG_ITMP1);
	i386_mov_reg_membase(REG_ITMP1, REG_SP, 4);

	i386_mov_membase_reg(REG_SP, 32 + 4, REG_ITMP1);
	i386_mov_reg_membase(REG_ITMP1, REG_SP, 8);

	i386_mov_membase_reg(REG_SP, 40 + 4, REG_ITMP1);
	i386_mov_reg_membase(REG_ITMP1, REG_SP, 12);

	i386_mov_membase_reg(REG_SP, 48 + 4, REG_ITMP1);
	i386_mov_reg_membase(REG_ITMP1, REG_SP, 16);

	i386_mov_membase_reg(REG_SP, 56 + 4, REG_ITMP1);
	i386_mov_reg_membase(REG_ITMP1, REG_SP, 20);

	i386_mov_imm_membase(&env, REG_SP, 0);

	i386_mov_imm_reg(f, REG_ITMP1);
	i386_call_reg(REG_ITMP1);

	i386_alu_imm_reg(I386_ADD, 24, REG_SP);

/*  	M_LDA  (REG_PV, REG_RA, -15*4);      /* recompute pv from ra               */
/*  	M_ALD  (REG_ITMP3, REG_PV, 15*8);    /* get address of exceptionptr        */

/*  	M_ALD  (REG_RA, REG_SP, 0);         /* load return address                */
/*  	M_ALD  (REG_ITMP1, REG_ITMP3, 0);   /* load exception into reg. itmp1     */

/*  	M_LDA  (REG_SP, REG_SP, 8);         /* remove stackframe                  */
/*  	M_BNEZ (REG_ITMP1, 1);              /* if no exception then return        */

/*  	M_RET  (REG_ZERO, REG_RA);          /* return to caller                   */
	i386_ret();

/*  	M_AST  (REG_ZERO, REG_ITMP3, 0);    /* store NULL into exceptionptr       */
/*  	M_LDA  (REG_ITMP2, REG_RA, -4);     /* move fault address into reg. itmp2 */

/*  	M_ALD  (REG_ITMP3, REG_PV,16*8);    /* load asm exception handler address */
/*  	M_JMP  (REG_ZERO, REG_ITMP3);       /* jump to asm exception handler      */


	/* TWISTI */	
/*  	s[14] = (u8) f;                      /* address of native method          */
/*  	s[15] = (u8) (&exceptionptr);        /* address of exceptionptr           */
/*  	s[16] = (u8) (asm_handle_nat_exception); /* addr of asm exception handler */
/*  	s[17] = (u8) (&env);                  /* addr of jni_environement         */
	s[14] = (u4) f;                      /* address of native method          */
	s[15] = (u8) (&exceptionptr);        /* address of exceptionptr           */
	s[16] = (u8) (asm_handle_nat_exception); /* addr of asm exception handler */
	s[17] = (u8) (&env);                  /* addr of jni_environement         */

#ifdef STATISTICS
	count_nstub_len += NATIVESTUBSIZE * 8;
#endif

	return (u1*) s;
}

/* function: removenativestub **************************************************

    removes a previously created native-stub from memory
    
*******************************************************************************/

void removenativestub (u1 *stub)
{
	CFREE (stub, NATIVESTUBSIZE * 8);
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
