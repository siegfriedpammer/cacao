/* alpha/ngen.c ****************************************************************

	Copyright (c) 1997, 2003 A. Krall, R. Grafl, M. Gschwind, M. Probst, S. Ring

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Contains the codegenerator for a PowerPC processor.
	This module generates PowerPC machine code for a sequence of
	pseudo commands (ICMDs).

	Authors: Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at
	         Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: $Id: ngen.c 321 2003-05-24 08:17:49Z stefan $

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
	if (checknull) {\
		M_TST((objreg));\
		M_BEQ(0);\
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

#define M_INTMOVE(a,b) if((a)!=(b)){M_MOV(a,b);}

#define M_TINTMOVE(t,a,b) \
	if ((t)==TYPE_LNG) \
		{M_INTMOVE(a,b); M_INTMOVE(secondregs[a],secondregs[b]);} \
	else \
		M_INTMOVE(a,b);


/* M_FLTMOVE:
    generates a floating-point-move from register a to b.
    if a and b are the same float-register, no code will be generated
*/ 

#define M_FLTMOVE(a,b) if((a)!=(b)){M_FMOV(a,b);}


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

#define var_to_reg_int0(regnr,v,tempnr,a,b) { \
	if ((v)->flags & INMEMORY) \
		{COUNT_SPILLS;if (a) M_ILD(tempnr,REG_SP,4*(v)->regoff); \
		regnr=tempnr; \
		if ((b) && IS_2_WORD_TYPE((v)->type)) \
			M_ILD((a)?secondregs[tempnr]:tempnr,REG_SP,4*(v)->regoff+4);} \
	else regnr=(!(a)&&(b)) ? secondregs[(v)->regoff] : (v)->regoff; \
}
#define var_to_reg_int(regnr,v,tempnr) var_to_reg_int0(regnr,v,tempnr,1,1)


#define var_to_reg_flt(regnr,v,tempnr) { \
	if ((v)->flags & INMEMORY) \
		{COUNT_SPILLS;M_DLD(tempnr,REG_SP,4*(v)->regoff);regnr=tempnr;} \
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

#define store_reg_to_var_int0(sptr, tempregnum, a, b) {       \
	if ((sptr)->flags & INMEMORY) {                    \
		COUNT_SPILLS;                                  \
		if (a) M_IST(tempregnum, REG_SP, 4 * (sptr)->regoff); \
		if ((b) && IS_2_WORD_TYPE((sptr)->type)) \
			M_IST(secondregs[tempregnum], REG_SP, 4 * (sptr)->regoff + 4); \
		}                                              \
	}

#define store_reg_to_var_int(sptr, tempregnum) \
	store_reg_to_var_int0(sptr, tempregnum, 1, 1)

#define store_reg_to_var_flt(sptr, tempregnum) {       \
	if ((sptr)->flags & INMEMORY) {                    \
		COUNT_SPILLS;                                  \
		if ((sptr)->type==TYPE_DBL) \
			M_DST(tempregnum, REG_SP, 4 * (sptr)->regoff); \
		else \
			M_FST(tempregnum, REG_SP, 4 * (sptr)->regoff); \
		}                                              \
	}


/* NullPointerException signal handler for hardware null pointer check */

void catch_NullPointerException(int sig, int code)
{
#if 0
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
#endif
}


void init_exceptions(void)
{
	/* install signal handlers we need to convert to exceptions */

	return;
	if (!checknull) {

#if defined(SIGSEGV)
		signal(SIGSEGV, (void*) catch_NullPointerException);
#endif

#if defined(SIGBUS)
		signal(SIGBUS, (void*) catch_NullPointerException);
#endif
		}
}

void doadjust_argvars(stackptr s, int d, int *a, int *ia)
{
	if (!d) {
		*a = 0; *ia = 0;
		return;
	}
	doadjust_argvars(s->prev, d-1, a, ia);
	if (s->varkind == ARGVAR)
		s->varnum = (IS_FLT_DBL_TYPE(s->type)) ? *a : *ia;
	*a += 1;
	*ia += (IS_2_WORD_TYPE(s->type)) ? 2 : 1;
}

int adjust_argvars(stackptr s, int d)
{
	int a, ia;
	doadjust_argvars(s, d, &a, &ia);
}

void nocode()
{
	printf("NOCODE\n");
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
	int paramsize, maxparamsize;

	maxparamsize = 0;
	for (bptr = block; bptr != NULL; bptr = bptr->next) {
		len = bptr->icount;
		for (iptr = bptr->iinstr;
		    len > 0;
		    src = iptr->dst, len--, iptr++)
		{
			switch (iptr->opc) {
				case ICMD_BUILTIN1:
					s3 = 1;
					goto countparams;
				case ICMD_BUILTIN2:
					s3 = 2;
					goto countparams;
				case ICMD_BUILTIN3:
					s3 = 3;
					goto countparams;

				case ICMD_INVOKEVIRTUAL:
				case ICMD_INVOKESPECIAL:
				case ICMD_INVOKESTATIC:
				case ICMD_INVOKEINTERFACE:
					s3 = iptr->op1;
countparams:
					adjust_argvars(src, s3);
					paramsize = 0;
					for (; --s3 >= 0; src = src->prev) {
						paramsize += IS_2_WORD_TYPE(src->type) ? 2 : 1;
						}
					maxparamsize = paramsize > maxparamsize ? paramsize : maxparamsize;
			}
		}
	}

	savedregs_num = isleafmethod ? 0 : 6;

	/* space to save used callee saved registers */

	savedregs_num += (savintregcnt - maxsavintreguse);
	savedregs_num += 2*(savfltregcnt - maxsavfltreguse);

	parentargs_base = maxparamsize + maxmemuse + savedregs_num;

#ifdef USE_THREADS                 /* space to save argument of monitor_enter */

	if (checksync && (method->flags & ACC_SYNCHRONIZED))
		parentargs_base++;

#endif

	/* create method header */

#if POINTERSIZE==4
	(void) dseg_addaddress(method);                         /* Filler         */
#endif
	(void) dseg_addaddress(method);                         /* MethodPointer  */
	(void) dseg_adds4(parentargs_base * 4);                 /* FrameSize      */

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

	if (!isleafmethod) {
		M_MFLR(REG_ITMP3);
		M_AST(REG_ITMP3, REG_SP, 8);
	}
	if (parentargs_base)
		{M_STWU (REG_SP, REG_SP, -parentargs_base * 4);}

	/* save return address and used callee saved registers */

	p = parentargs_base;
	for (r = savintregcnt - 1; r >= maxsavintreguse; r--)
		{p--; M_IST (savintregs[r], REG_SP, 4 * p);}
	for (r = savfltregcnt - 1; r >= maxsavfltreguse; r--)
		{p-=2; M_DST (savfltregs[r], REG_SP, 4 * p);}

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

#if 0
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
#endif

	/* take arguments out of register or stack frame */

	{
		int narg=0, niarg=0;
		int arg, iarg;
 	for (p = 0, l = 0; p < mparamcount; p++) {
		arg = narg; iarg = niarg;
 		t = mparamtypes[p];
 		var = &(locals[l][t]);
 		l++, narg++, niarg++;
 		if (IS_2_WORD_TYPE(t))    /* increment local counter for 2 word types */
 			l++, niarg++;
 		if (var->type < 0)
 			continue;
 		r = var->regoff; 
		if (IS_INT_LNG_TYPE(t)) {                    /* integer args          */
 			if (iarg < INT_ARG_CNT) {                /* register arguments    */
 				if (!(var->flags & INMEMORY))        /* reg arg -> register   */
 					{M_TINTMOVE (t, argintregs[iarg], r);}
 				else                                 /* reg arg -> spilled    */
					{
						M_IST (argintregs[iarg], REG_SP, 4 * r);
						if (IS_2_WORD_TYPE(t))
							M_IST (secondregs[argintregs[iarg]], REG_SP, 4 * r + 4);
					}
 				}
 			else {                                   /* stack arguments       */
 				pa = iarg - INT_ARG_CNT;
 				if (!(var->flags & INMEMORY))        /* stack arg -> register */ 
					{
						M_ILD (r, REG_SP, 4 * (parentargs_base + pa));
						if (IS_2_WORD_TYPE(t))
							M_ILD (secondregs[r], REG_SP, 4 * (parentargs_base + pa) + 4);
					}
 				else {                               /* stack arg -> spilled  */
 					M_ILD (REG_ITMP1, REG_SP, 4 * (parentargs_base + pa));
 					M_IST (REG_ITMP1, REG_SP, 4 * r);
					if (IS_2_WORD_TYPE(t)) {
						M_ILD (REG_ITMP1, REG_SP, 4 * (parentargs_base + pa) + 4);
						M_IST (REG_ITMP1, REG_SP, 4 * r + 4);
						}
 					}
 				}
 			}
 		else {                                       /* floating args         */   
 			if (arg < FLT_ARG_CNT) {                 /* register arguments    */
 				if (!(var->flags & INMEMORY))        /* reg arg -> register   */
 					{M_FLTMOVE (argfltregs[arg], r);}
 				else				                 /* reg arg -> spilled    */
					M_DST (argfltregs[arg], REG_SP, 4 * r);
 				}
 			else {                                   /* stack arguments       */
 				pa = arg - FLT_ARG_CNT;
 				if (!(var->flags & INMEMORY))        /* stack-arg -> register */
					if (IS_2_WORD_TYPE(t))
						M_DLD (r, REG_SP, 4 * (parentargs_base + pa) );
					else
						M_FLD (r, REG_SP, 4 * (parentargs_base + pa) );
 				else {                               /* stack-arg -> spilled  */
					if (IS_2_WORD_TYPE(t))
						M_DLD (REG_FTMP1, REG_SP, 4 * (parentargs_base + pa));
					else
						M_FLD (REG_FTMP1, REG_SP, 4 * (parentargs_base + pa));
					M_DST (REG_FTMP1, REG_SP, 4 * r);
 					}
 				}
 			}
		}  /* end for */
	}

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
							M_DLD(d, REG_SP, 4 * interfaces[len][s2].regoff);
							}
						store_reg_to_var_flt(src, d);
						}
					else {
						if (!(interfaces[len][s2].flags & INMEMORY)) {
							s1 = interfaces[len][s2].regoff;
							M_TINTMOVE(s2,s1,d);
							}
						else {
							M_ILD(d, REG_SP, 4 * interfaces[len][s2].regoff);
							if (IS_2_WORD_TYPE(s2))
								M_ILD(secondregs[d], REG_SP, 4 * interfaces[len][s2].regoff + 4);
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
			M_TST(s1);
			M_BEQ(0);
			mcode_addxnullrefs(mcodeptr);
			break;

		/* constant operations ************************************************/

#define ICONST(r,c) if(((c)>=-32768)&&((c)<= 32767)){M_LDA(r,REG_ZERO,c);} \
                    else{a=dseg_adds4(c);M_ILD(r,REG_PV,a);}

#define LCONST(r,c) ICONST(r,((s8)(c)>>32)&0xffffffff); ICONST(secondregs[r],(s8)(c)&0xffffffff);

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
			ICONST(d, (u4) iptr->val.a);
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
			if (var->flags & INMEMORY) {
				M_ILD(d, REG_SP, 4 * var->regoff);
				if (IS_2_WORD_TYPE(var->type))
					M_ILD(secondregs[d], REG_SP, 4 * var->regoff + 4);
			} else
				{M_TINTMOVE(var->type,var->regoff,d);}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_FLOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_DLOAD:      /* op1 = local variable                         */

			d = reg_of_var(iptr->dst, REG_FTMP1);
			if ((iptr->dst->varkind == LOCALVAR) &&
			    (iptr->dst->varnum == iptr->op1))
				break;
			var = &(locals[iptr->op1][iptr->opc - ICMD_ILOAD]);
			if (var->flags & INMEMORY)
				M_DLD(d, REG_SP, 4 * var->regoff);
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
			var = &(locals[iptr->op1][iptr->opc - ICMD_ISTORE]);
			if (var->flags & INMEMORY) {
				var_to_reg_int(s1, src, REG_ITMP1);
				M_IST(s1, REG_SP, 4 * var->regoff);
				if (IS_2_WORD_TYPE(var->type))
					M_IST(secondregs[s1], REG_SP, 4 * var->regoff + 4);
				}
			else {
				var_to_reg_int(s1, src, var->regoff);
				M_TINTMOVE(var->type, s1, var->regoff);
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
				if (var->type==TYPE_DBL)
					M_DST(s1, REG_SP, 4 * var->regoff);
				else
					M_FST(s1, REG_SP, 4 * var->regoff);
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
					M_TINTMOVE(from->type,s1,d); \
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
			M_NEG(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LNEG:       /* ..., value  ==> ..., - value                 */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_SUBFIC(secondregs[s1], 0, secondregs[d]);
			M_SUBFZE(s1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_I2L:        /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_INTMOVE(s1, secondregs[d]);
			M_CLR(d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP2);
			M_INTMOVE(secondregs[s1], d );
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_BSEXT(s1, d);
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
			M_SSEXT(s1, d);
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

			var_to_reg_int0(s1, src->prev, REG_ITMP1, 0, 1);
			var_to_reg_int0(s2, src, REG_ITMP2, 0, 1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_ADDC(s1, s2, secondregs[d]);
			store_reg_to_var_int0(iptr->dst, d, 0, 1);
			var_to_reg_int0(s1, src->prev, REG_ITMP1, 1, 0);
			var_to_reg_int0(s2, src, REG_ITMP2, 1, 0);
			M_ADDE(s1, s2, d);
			store_reg_to_var_int0(iptr->dst, d, 1, 0);
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			{
				s4 l = iptr->val.l;
				if (l >= -32768 && l <= 32767)
					M_ADDIC(secondregs[s1], l, secondregs[d]);
				else {
					ICONST(REG_ITMP3, l);
					M_ADDC(secondregs[s1], REG_ITMP3, secondregs[d]);
					}
				if (l == iptr->val.l)
					if (l>=0)
						M_ADDZE(s1, d);
					else
						M_ADDME(s1, d);
				else {
					ICONST(REG_ITMP3, iptr->val.l>>32);
					M_ADDE(s1, REG_ITMP3, d);
					}
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
				ICONST(REG_ITMP2, -iptr->val.i);
				M_IADD(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			var_to_reg_int0(s1, src->prev, REG_ITMP1, 0, 1);
			var_to_reg_int0(s2, src, REG_ITMP2, 0, 1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_SUBC(s1, s2, d);
			store_reg_to_var_int0(iptr->dst, d, 0, 1);
			var_to_reg_int0(s1, src->prev, REG_ITMP1, 1, 0);
			var_to_reg_int0(s2, src, REG_ITMP2, 1, 0);
			M_SUBE(s1, s2, secondregs[d]);
			store_reg_to_var_int0(iptr->dst, d, 1, 0);
			break;


		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			{
				s4 l = -iptr->val.l;
				if (l >= -32768 && l <= 32767)
					M_ADDIC(secondregs[s1], l, secondregs[d]);
				else {
					ICONST(REG_ITMP3, l);
					M_ADDC(secondregs[s1], REG_ITMP3, secondregs[d]);
					}
				if (l == -iptr->val.l)
					if (l>=0)
						M_ADDZE(s1, d);
					else
						M_ADDME(s1, d);
				else {
					ICONST(REG_ITMP3, (-iptr->val.l)>>32);
					M_ADDE(s1, REG_ITMP3, d);
					}
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

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
			if ((iptr->val.i >= -32768) && (iptr->val.i <= 32767)) {
				M_IMUL_IMM(s1, iptr->val.i, d);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_IMUL(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			nocode();
			//CUT
			break;

		case ICMD_LMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* val.l = constant                             */

			nocode();
			//CUT
			break;

		case ICMD_IDIVPOW2:   /* ..., value  ==> ..., value << constant       */
		                      
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_SRA_IMM(s1, iptr->val.i, d);
			M_ADDZE(d, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LDIVPOW2:   /* val.i = constant                             */

			nocode();
			//CUT
			break;

		case ICMD_ISHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_AND_IMM(s2, 0x1f, REG_ITMP3);
			M_SLL(s1, REG_ITMP3, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_SLL_IMM(s1, iptr->val.i & 0x1f, d);
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
			M_SRL(d, REG_ITMP2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_SRL_IMM(d, iptr->val.i & 0x1f, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LSHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			nocode();
			//CUT
			break;

		case ICMD_LSHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.l = constant                             */

			nocode();
			//CUT
			break;

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			nocode();
			//CUT
			break;

		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.l = constant                             */

			nocode();
			//CUT
			break;

		case ICMD_LUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			nocode();
			//CUT
			break;

		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.l = constant                             */

			nocode();
			//CUT
			break;

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_AND(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LAND:
			var_to_reg_int0(s1, src->prev, REG_ITMP1, 0, 1);
			var_to_reg_int0(s2, src, REG_ITMP2, 0, 1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_AND(s1, s2, secondregs[d]);
			store_reg_to_var_int0(iptr->dst, d, 0, 1);
			var_to_reg_int0(s1, src->prev, REG_ITMP1, 1, 0);
			var_to_reg_int0(s2, src, REG_ITMP2, 1, 0);
			M_AND(s1, s2, d);
			store_reg_to_var_int0(iptr->dst, d, 1, 0);
			break;

		case ICMD_IANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.i = constant                             */
		case ICMD_IREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 65535)) {
				M_AND_IMM(s1, iptr->val.i, d);
				}
			else if (iptr->val.i == 0xffffff) {
				M_RLWINM(s1, 0, 8, 31, d);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_AND(s1, REG_ITMP2, d);
				}
			store_reg_to_var_int(iptr->dst, d);
			break;

			/*
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			{
				int b;
				u4 m = iptr->val.i;
				while (m>>=1)
					++b;
				M_RLWINM(s1, 0, 31-b, 31, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;
			*/

		case ICMD_IREM0X10001:  /* ..., value  ==> ..., value % 0x100001      */
		
/*          b = value & 0xffff;
			a = value >> 16;
			a = ((b - a) & 0xffff) + (b < a);
*/
			nocode();
			//CUT
			break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.l = constant                             */
		case ICMD_LREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.l = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			{
				s4 l = iptr->val.l;
				int sr = secondregs[s1];
				int dr = secondregs[d];
				if (!l)
					M_CLR(dr);
				else if (!~l)
					{M_INTMOVE(sr, dr);}
				else if ((l&0xffff) == l)
					M_AND_IMM(sr, l&0xffff, dr);
				else if ((l&0xffff) == 0)
					M_ANDIS(sr, l>>16, dr);
				else {
					ICONST(REG_ITMP3, l);
					M_AND(REG_ITMP3, sr, dr);
					}
			}
			{
				s4 l = iptr->val.l;
				int sr = s1;
				int dr = d;
				if (!l)
					M_CLR(dr);
				else if (!~l)
					{M_INTMOVE(sr, dr);}
				else if ((l&0xffff) == l)
					M_AND_IMM(sr, l&0xffff, dr);
				else if ((l&0xffff) == 0)
					M_ANDIS(sr, l>>16, dr);
				else {
					ICONST(REG_ITMP3, l);
					M_AND(REG_ITMP3, sr, dr);
					}
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

			/*
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			{
				int b;
				s8 m = iptr->val.i;
				if (m<0)
					panic("Illegal LREMPOW2 constant");
				while (m>>=1)
					++b;
				if (b>=32)
					{M_INTMOVE(secondregs[s1], secondregs[d]);}
				else
					M_RLWINM(secondregs[s1], 0, 31-b, 31, secondregs[d]);
				if (b<=32)
					M_CLR(d);
				else
					M_RLWINM(s1, 0, 31-(b-32), 31, d);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;
			*/

		case ICMD_LREM0X10001:/* ..., value  ==> ..., value % 0x10001         */

			nocode();
			//CUT
			break;

		case ICMD_IOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_OR( s1,s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LOR:
			var_to_reg_int0(s1, src->prev, REG_ITMP1, 0, 1);
			var_to_reg_int0(s2, src, REG_ITMP2, 0, 1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_OR(s1, s2, d);
			store_reg_to_var_int0(iptr->dst, d, 0, 1);
			var_to_reg_int0(s1, src->prev, REG_ITMP1, 1, 0);
			var_to_reg_int0(s2, src, REG_ITMP2, 1, 0);
			M_OR(s1, s2, secondregs[d]);
			store_reg_to_var_int0(iptr->dst, d, 1, 0);
			break;

		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 65535)) {
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
			{
				s4 l = iptr->val.l;
				int sr = secondregs[s1];
				int dr = secondregs[d];
				if (!l)
					{M_INTMOVE(sr, dr);}
				else if (!~l)
					M_IADD_IMM(REG_ZERO, 0xffff, dr);
				else if ((l&0xffff) == l)
					M_OR_IMM(sr, l&0xffff, dr);
				else if ((l&0xffff) == 0)
					M_ORIS(sr, l>>16, dr);
				else {
					ICONST(REG_ITMP3, l);
					M_OR(REG_ITMP3, sr, dr);
					}
			}
			{
				s4 l = iptr->val.l;
				int sr = s1;
				int dr = d;
				if (!l)
					{M_INTMOVE(sr, dr);}
				else if (!~l)
					M_IADD_IMM(REG_ZERO, 0xffff, dr);
				else if ((l&0xffff) == l)
					M_OR_IMM(sr, l&0xffff, dr);
				else if ((l&0xffff) == 0)
					M_ORIS(sr, l>>16, dr);
				else {
					ICONST(REG_ITMP3, l);
					M_OR(REG_ITMP3, sr, dr);
					}
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_XOR(s1, s2, d);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LXOR:
			var_to_reg_int0(s1, src->prev, REG_ITMP1, 0, 1);
			var_to_reg_int0(s2, src, REG_ITMP2, 0, 1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_XOR(s1, s2, d);
			store_reg_to_var_int0(iptr->dst, d, 0, 1);
			var_to_reg_int0(s1, src->prev, REG_ITMP1, 1, 0);
			var_to_reg_int0(s2, src, REG_ITMP2, 1, 0);
			M_XOR(s1, s2, secondregs[d]);
			store_reg_to_var_int0(iptr->dst, d, 1, 0);
			break;

		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.i = constant                             */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			if ((iptr->val.i >= 0) && (iptr->val.i <= 65535)) {
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
			{
				s4 l = iptr->val.l;
				int sr = secondregs[s1];
				int dr = secondregs[d];
				if (!l)
					{M_INTMOVE(sr, dr);}
				else if (!~l)
					M_NOT(sr, dr);
				else if ((l&0xffff) == l)
					M_XOR_IMM(sr, l&0xffff, dr);
				else if ((l&0xffff) == 0)
					M_XORIS(sr, l>>16, dr);
				else {
					ICONST(REG_ITMP3, l);
					M_XOR(REG_ITMP3, sr, dr);
					}
			}
			{
				s4 l = iptr->val.l;
				int sr = s1;
				int dr = d;
				if (!l)
					{M_INTMOVE(sr, dr);}
				else if (!~l)
					M_NOT(sr, dr);
				else if ((l&0xffff) == l)
					M_XOR_IMM(sr, l&0xffff, dr);
				else if ((l&0xffff) == 0)
					M_XORIS(sr, l>>16, dr);
				else {
					ICONST(REG_ITMP3, l);
					M_XOR(REG_ITMP3, sr, dr);
					}
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_LCMP:       /* ..., val1, val2  ==> ..., val1 cmp val2      */

			var_to_reg_int0(s1, src->prev, REG_ITMP1, 1, 0);
			var_to_reg_int0(s2, src, REG_ITMP2, 1, 0);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			{
				s4 *br1;
				M_IADD_IMM(REG_ZERO, 1, d);
				M_CMP(s1, s2);
				M_BGT(0);
				br1 = mcodeptr;
				M_BLT(0);
				var_to_reg_int0(s1, src->prev, REG_ITMP1, 0, 1);
				var_to_reg_int0(s2, src, REG_ITMP2, 0, 1);
				M_CMPU(s1, s2);
				M_BGT(12);
				M_BEQ(4);
				M_IADD_IMM(d, -1, d);
				M_IADD_IMM(d, -1, d);
				gen_resolvebranch(br1, br1+1, mcodeptr);
				gen_resolvebranch(br1+1, br1+2, mcodeptr-2);
			}
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_IINC:       /* ..., value  ==> ..., value + constant        */
		                      /* op1 = variable, val.i = constant             */

            var = &(locals[iptr->op1][TYPE_INT]);
            if (var->flags & INMEMORY) {
                s1 = REG_ITMP1;
                M_ILD(s1, REG_SP, 4 * var->regoff);
                }
            else
                s1 = var->regoff;
			{
				u4 m = iptr->val.i;
				if (m>0 && (m&0x8000))
					m += 65536;
				if (m<0 && !(m&0x8000))
					m -= 65536;
				if (m&0xffff0000)
					M_ADDIS(s1, m>>16, d);
				if (m&0xffff)
					M_IADD_IMM((m&0xffff0000)?d:s1, m&0xffff, d);
			}
            if (var->flags & INMEMORY)
				M_IST(s1, REG_SP, 4 * var->regoff);
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

			nocode();
			//CUT
		    break;

		case ICMD_DREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			nocode();
			//CUT
		    break;

		case ICMD_I2F:       /* ..., value  ==> ..., (float) value            */

			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			a = dseg_adddouble(0.0);
			M_XORIS(s1, 0x8000, s1);
			M_IST(s1, REG_PV, a+4);
			M_ADDIS(REG_ZERO, 0x4330, REG_ITMP1);
			M_IST(REG_ITMP1, REG_PV, a);
			M_DLD(REG_FTMP2, REG_PV, a);
			a = dseg_adds8(0x4330000080000000LL);
			M_DLD(REG_FTMP1, REG_PV, a);
			M_FSUB(REG_FTMP2, REG_FTMP1, d);
			M_CVTDF(d, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_L2F:       /* ..., value  ==> ..., (float) value            */
			nocode();
			//CUT
			break;

		case ICMD_I2D:       /* ..., value  ==> ..., (double) value           */
			var_to_reg_int(s1, src, REG_ITMP1);
			d = reg_of_var(iptr->dst, REG_FTMP3);
			a = dseg_adddouble(0.0);
			M_XORIS(s1, 0x8000, s1);
			M_IST(s1, REG_PV, a+4);
			M_ADDIS(REG_ZERO, 0x4330, REG_ITMP1);
			M_IST(REG_ITMP1, REG_PV, a);
			M_DLD(REG_FTMP2, REG_PV, a);
//			a = dseg_(0x4330000080000000LL);
			a = dseg_adddouble(-0.0);
			M_DLD(REG_FTMP1, REG_PV, a);
			M_FSUB(REG_FTMP2, REG_FTMP1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
			
		case ICMD_L2D:       /* ..., value  ==> ..., (double) value           */
			nocode();
			//CUT
			break;

		case ICMD_F2I:       /* ..., value  ==> ..., (int) value              */
		case ICMD_D2I:
			var_to_reg_flt(s1, src, REG_FTMP1);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			a = dseg_adds4(0);
			M_CVTDL_C(s1, REG_FTMP1);
			M_LDA (REG_ITMP1, REG_PV, a);
			M_STFIWX(REG_FTMP1, 0, REG_ITMP1);
			M_ILD (d, REG_PV, a);
			store_reg_to_var_int(iptr->dst, d);
			break;
		
		case ICMD_F2L:       /* ..., value  ==> ..., (long) value             */
		case ICMD_D2L:
			nocode();
			//CUT
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
			M_CVTDF(s1, d);
			store_reg_to_var_flt(iptr->dst, d);
			break;
		
		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */
		case ICMD_DCMPL:
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_FCMPU(s1, s2);
			M_IADD_IMM(0, 1, d);
			M_BGT(12);
			M_IADD_IMM(0, 0, d);
			M_BGE(4);
			M_IADD_IMM(0, -1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;
			
		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */
		case ICMD_DCMPG:
			var_to_reg_flt(s1, src->prev, REG_FTMP1);
			var_to_reg_flt(s2, src, REG_FTMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			M_FCMPU(s2, s1);
			M_IADD_IMM(0, -1, d);
			M_BGT(12);
			M_IADD_IMM(0, 0, d);
			M_BGE(4);
			M_IADD_IMM(0, 1, d);
			store_reg_to_var_int(iptr->dst, d);
			break;


		/* memory operations **************************************************/

#define gen_bound_check \
            if (checkbounds) { \
				M_ILD(REG_ITMP3, s1, OFFSET(java_arrayheader, size));\
				M_CMPU(s2, REG_ITMP3);\
				M_BEQ(0);\
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
			M_SLL_IMM(s2, 2, REG_ITMP1);
			M_IADD_IMM(REG_ITMP1, OFFSET(java_objectarray, data[0]), REG_ITMP1);
			M_LWZX(d, s1, REG_ITMP1);
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
			M_SLL_IMM(s2, 3, REG_ITMP1);
			M_IADD_IMM(REG_ITMP1, OFFSET(java_longarray, data[0]), REG_ITMP1);
			M_LWZX(d, s1, REG_ITMP1);
			M_IADD_IMM(REG_ITMP1, 4, REG_ITMP1);
			M_LWZX(secondregs[d], s1, REG_ITMP1);
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
			M_SLL_IMM(s2, 2, REG_ITMP1);
			M_IADD_IMM(REG_ITMP1, OFFSET(java_intarray, data[0]), REG_ITMP1);
			M_LWZX(d, s1, REG_ITMP1);
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
			M_SLL_IMM(s2, 2, REG_ITMP1);
			M_IADD_IMM(REG_ITMP1, OFFSET(java_floatarray, data[0]), REG_ITMP1);
			M_LFSX(d, s1, REG_ITMP1);
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
			M_SLL_IMM(s2, 3, REG_ITMP1);
			M_IADD_IMM(REG_ITMP1, OFFSET(java_doublearray, data[0]), REG_ITMP1);
			M_LFDX(d, s1, REG_ITMP1);
			store_reg_to_var_flt(iptr->dst, d);
			break;

		case ICMD_BALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			M_IADD_IMM(s2, OFFSET(java_chararray, data[0]), REG_ITMP1);
			M_LBZX(d, s1, REG_ITMP1);
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
			M_SLL_IMM(s2, 1, REG_ITMP1);
			M_IADD_IMM(REG_ITMP1, OFFSET(java_shortarray, data[0]), REG_ITMP1);
			M_LHZX(d, s1, REG_ITMP1);
			store_reg_to_var_int(iptr->dst, d);
			break;

		case ICMD_CALOAD:     /* ..., arrayref, index  ==> ..., value         */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			d = reg_of_var(iptr->dst, REG_ITMP3);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			M_SLL_IMM(s2, 1, REG_ITMP1);
			M_IADD_IMM(REG_ITMP1, OFFSET(java_chararray, data[0]), REG_ITMP1);
			M_LHZX(d, s1, REG_ITMP1);
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
			M_SLL_IMM(s2, 2, REG_ITMP1);
			M_IADD_IMM(REG_ITMP1, OFFSET(java_objectarray, data[0]), REG_ITMP1);
			M_STWX(s3, s1, REG_ITMP1);
			break;

		case ICMD_LASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_int(s3, src, REG_ITMP3);
			M_SLL_IMM(s2, 3, REG_ITMP1);
			M_IADD_IMM(REG_ITMP1, OFFSET(java_intarray, data[0]), REG_ITMP1);
			M_STWX(s3, s1, REG_ITMP1);
			M_IADD_IMM(REG_ITMP1, 4, REG_ITMP1);
			M_STWX(secondregs[s3], s1, REG_ITMP1);
			break;

		case ICMD_IASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_int(s3, src, REG_ITMP3);
			M_SLL_IMM(s2, 2, REG_ITMP1);
			M_IADD_IMM(REG_ITMP1, OFFSET(java_intarray, data[0]), REG_ITMP1);
			M_STWX(s3, s1, REG_ITMP1);
			break;

		case ICMD_FASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_flt(s3, src, REG_FTMP3);
			M_SLL_IMM(s2, 2, REG_ITMP1);
			M_IADD_IMM(REG_ITMP1, OFFSET(java_floatarray, data[0]), REG_ITMP1);
			M_STFSX(s3, s1, REG_ITMP1);
			break;

		case ICMD_DASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_flt(s3, src, REG_FTMP3);
			M_SLL_IMM(s2, 2, REG_ITMP1);
			M_IADD_IMM(REG_ITMP1, OFFSET(java_doublearray, data[0]), REG_ITMP1);
			M_STFDX(s3, s1, REG_ITMP1);
			break;

		case ICMD_BASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_int(s3, src, REG_ITMP3);
			M_IADD_IMM(s2, OFFSET(java_bytearray, data[0]), REG_ITMP1);
			M_STBX(s3, s1, REG_ITMP1);
			break;

		case ICMD_SASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_int(s3, src, REG_ITMP3);
			M_SLL_IMM(s2, 1, REG_ITMP1);
			M_IADD_IMM(REG_ITMP1, OFFSET(java_shortarray, data[0]), REG_ITMP1);
			M_STHX(s3, s1, REG_ITMP1);
			break;

		case ICMD_CASTORE:    /* ..., arrayref, index, value  ==> ...         */

			var_to_reg_int(s1, src->prev->prev, REG_ITMP1);
			var_to_reg_int(s2, src->prev, REG_ITMP2);
			if (iptr->op1 == 0) {
				gen_nullptr_check(s1);
				gen_bound_check;
				}
			var_to_reg_int(s3, src, REG_ITMP3);
			M_SLL_IMM(s2, 1, REG_ITMP1);
			M_IADD_IMM(REG_ITMP1, OFFSET(java_chararray, data[0]), REG_ITMP1);
			M_STHX(s3, s1, REG_ITMP1);
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
					var_to_reg_int(s2, src, REG_ITMP3);
					M_IST(s2, REG_ITMP1, 0);
					M_IST(secondregs[s2], REG_ITMP1, 4);
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
					M_ILD(d, REG_ITMP1, 0);
					M_ILD(secondregs[d], REG_ITMP1, 4);
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
					var_to_reg_int(s2, src, REG_ITMP3);
					gen_nullptr_check(s1);
					M_IST(s2, s1, a);
					M_IST(secondregs[s2], s1, a+4);
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
					M_ILD(d, s1, a);
					M_ILD(secondregs[d], s1, a+4);
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

			a = dseg_addaddress(asm_handle_exception);
			M_ALD(REG_ITMP2, REG_PV, a);
			M_MTCTR(REG_ITMP2);
			var_to_reg_int(s1, src, REG_ITMP1);
			M_INTMOVE(s1, REG_ITMP1_XPTR);

			if (isleafmethod) M_MFLR(REG_ITMP3);  /* save LR */
			M_BL(0);            /* get current PC */
			M_MFLR(REG_ITMP2_XPC);
			if (isleafmethod) M_MTLR(REG_ITMP3);  /* restore LR */
			M_RTS;              /* jump to CTR */

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

			if (isleafmethod) M_MFLR(REG_ITMP2);
			M_BL(0);
			M_MFLR(REG_ITMP1);
			M_IADD_IMM(REG_ITMP1, isleafmethod?16:12, REG_ITMP1);
			if (isleafmethod) M_MTLR(REG_ITMP2);
			M_BR(0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;
			
		case ICMD_RET:          /* ... ==> ...                                */
		                        /* op1 = local variable                       */

			var = &(locals[iptr->op1][TYPE_ADR]);
			if (var->flags & INMEMORY) {
				M_ALD(REG_ITMP1, REG_SP, 4 * var->regoff);
				M_MTCTR(REG_ITMP1);
				}
			else
				M_MTCTR(var->regoff);
			M_RTS;
			ALIGNCODENOP;
			break;

		case ICMD_IFNULL:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_TST(s1);
			M_BEQ(0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFNONNULL:    /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src, REG_ITMP1);
			M_TST(s1);
			M_BNE(0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IFLT:
		case ICMD_IFLE:
		case ICMD_IFNE:
		case ICMD_IFGT:
		case ICMD_IFGE:
		case ICMD_IFEQ:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			var_to_reg_int(s1, src, REG_ITMP1);
			if ((iptr->val.i > -32768) && (iptr->val.i <= 32767)) {
				M_CMPI(s1, iptr->val.i);
				}
			else {
				ICONST(REG_ITMP2, iptr->val.i);
				M_CMP(s1, REG_ITMP2);
				}
			switch (iptr->opc)
			{
				case ICMD_IFLT:
					M_BLT(0);
					break;
				case ICMD_IFLE:
					M_BLE(0);
					break;
				case ICMD_IFNE:
					M_BNE(0);
					break;
				case ICMD_IFGT:
					M_BGT(0);
					break;
				case ICMD_IFGE:
					M_BGE(0);
					break;
				case ICMD_IFEQ:
					M_BEQ(0);
					break;
				}
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;


		case ICMD_IF_LEQ:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */
		case ICMD_IF_LLT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */
		case ICMD_IF_LLE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */
		case ICMD_IF_LNE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */
		case ICMD_IF_LGT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */
		case ICMD_IF_LGE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */
			nocode();
			//CUT
			break;

			//CUT: alle _L
		case ICMD_IF_ICMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPEQ:    /* op1 = target JavaVM pc                     */
		case ICMD_IF_ACMPEQ:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BEQ(0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPNE:    /* op1 = target JavaVM pc                     */
		case ICMD_IF_ACMPNE:

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BNE(0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPLT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPLT:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BLT(0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPGT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPGT:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BGT(0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPLE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPLE:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BLE(0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
			break;

		case ICMD_IF_ICMPGE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPGE:    /* op1 = target JavaVM pc                     */

			var_to_reg_int(s1, src->prev, REG_ITMP1);
			var_to_reg_int(s2, src, REG_ITMP2);
			M_CMP(s1, s2);
			M_BGE(0);
			mcode_addreference(BlockPtrOfPC(iptr->op1), mcodeptr);
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
			M_TINTMOVE(src->type, s1, REG_RESULT);
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

			if (!isleafmethod) {
				M_ALD (REG_ITMP3, REG_SP, 4 * p + 8);
				M_MTLR (REG_ITMP3);
				}

			/* restore saved registers                                        */

			for (r = savintregcnt - 1; r >= maxsavintreguse; r--)
					{p--; M_ILD(savintregs[r], REG_SP, 4 * p);}
			for (r = savfltregcnt - 1; r >= maxsavfltreguse; r--)
					{p-=2; M_DLD(savfltregs[r], REG_SP, 4 * p);}

			/* deallocate stack                                               */

			if (parentargs_base)
				{M_LDA(REG_SP, REG_SP, parentargs_base*4);}

			/* call trace function */

#if 0
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
#endif

			M_RET;
			ALIGNCODENOP;
			}
			break;


#if 0
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

#endif

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
						M_TINTMOVE(src->type, d, s1);
						}
					else  {
						var_to_reg_int(d, src, REG_ITMP1);
						M_IST(d, REG_SP, 4 * (s3 - INT_ARG_CNT));
						if (IS_2_WORD_TYPE(src->type))
							M_IST(secondregs[d], REG_SP, 4 * (s3 - INT_ARG_CNT) + 4);
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
						if (IS_2_WORD_TYPE(src->type))
							M_DST(d, REG_SP, 4 * (s3 - FLT_ARG_CNT));
						else
							M_FST(d, REG_SP, 4 * (s3 - FLT_ARG_CNT));
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

			M_MTCTR(REG_PV);
			M_JSR;

			/* recompute pv */

			s1 = (int)((u1*) mcodeptr - mcodebase);
			M_MFLR(REG_ITMP1);
			if (s1<=32768) M_LDA (REG_PV, REG_ITMP1, -s1);
			else {
				s4 ml=-s1, mh=0;
				while (ml<-32768) { ml+=65536; mh--; }
				M_LDA (REG_PV, REG_ITMP1, ml );
				M_LDAH (REG_PV, REG_PV, mh );
				}

			/* d contains return type */

			if (d != TYPE_VOID) {
				if (IS_INT_LNG_TYPE(iptr->dst->type)) {
					s1 = reg_of_var(iptr->dst, REG_RESULT);
					M_TINTMOVE(iptr->dst->type, REG_RESULT, s1);
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


#if 0
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
#endif
	
   

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
					M_DST(s1, REG_SP, 4 * interfaces[len][s2].regoff);
					}
				}
			else {
				var_to_reg_int(s1, src, REG_ITMP1);
				if (!(interfaces[len][s2].flags & INMEMORY)) {
					M_TINTMOVE(s2,s1,interfaces[len][s2].regoff);
					}
				else {
					M_IST(s1, REG_SP, 4 * interfaces[len][s2].regoff);
					if (IS_2_WORD_TYPE(s2))
						M_IST(secondregs[s1], REG_SP, 4 * interfaces[len][s2].regoff + 4);
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

			//CUT
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

			//CUT
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

			//CUT
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

			//CUT
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
#if 0
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

u1 *createnativestub (functionptr f, methodinfo *m)
{
	int disp;
	u8 *s = CNEW (u8, NATIVESTUBSIZE);  /* memory to hold the stub            */
#if 0
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

	if (runverbose) {
		M_ALD(REG_ITMP1, REG_PV, -6*8);
		M_ALD(REG_PV, REG_PV, -5*8);

		M_JSR(REG_RA, REG_PV);
		disp = -(int) (p - (s4*) cs)*4;
		M_LDA(REG_PV, REG_RA, disp);
	}

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
			3 + (runverbose ? 6 : 0));  /* if no exception then return        */

	if (runverbose) {
		M_ALD(argintregs[0], REG_PV, -6*8);
		M_MOV(REG_RESULT, argintregs[1]);
		M_FMOV(REG_FRESULT, argfltregs[2]);
		M_FMOV(REG_FRESULT, argfltregs[3]);
		M_ALD(REG_PV, REG_PV, -7*8);
		M_JSR(REG_RA, REG_PV);
	}

	M_ALD  (REG_RA, REG_SP, 0);         /* load return address                */
	M_LDA  (REG_SP, REG_SP, 8);         /* remove stackframe                  */

	M_RET  (REG_ZERO, REG_RA);          /* return to caller                   */
	
	M_AST  (REG_ZERO, REG_ITMP3, 0);    /* store NULL into exceptionptr       */

	if (runverbose) {
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
