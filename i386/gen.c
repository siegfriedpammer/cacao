/***************************** alpha/gen.c *************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Contains the codegenerator for an Alpha processor.
	This module generates Alpha machine code for a sequence of
	pseudo commands (PCMDs).

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
	         Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/10/22

*******************************************************************************/



/*******************************************************************************

Datatypes and Register Allocations:
----------------------------------- 

On 64-bit-machines (like the Alpha) all operands are stored in the
registers in a 64-bit form, even when the correspondig JavaVM 
operands only need 32 bits.
This is done by a canonical representation:

32-bit integers are allways stored as sign-extended 64-bit values
(this approach is directly supported by the Alpha architecture and
is very easy to implement).

32-bit-floats are stored in a 64-bit doubleprecision register by
simply expanding the exponent and mantissa with zeroes.
(also supported by the architecture)


Stackframes:

The calling conventions and the layout of the stack is 
explained in detail in the documention file: calling.doc

*******************************************************************************/


/************************ Preallocated registers ******************************/

/* integer registers */
  
#define REG_RESULT      0    /* to deliver method results                     */ 
#define REG_EXCEPTION  	1    /* to throw an exception across method bounds    */

#define REG_RA			26   /* return address                                */
#define REG_PV  		27   /* procedure vector, must be provided by caller  */
#define REG_METHODPTR	28   /* pointer to the place from where the procedure */
                             /* vector has been fetched                       */
#define REG_ITMP1		25   /* temporary register                            */
#define REG_ITMP2		28   /* temporary register                            */
#define REG_ITMP3		29   /* temporary register                            */

#define REG_SP  		30   /* stack pointer                                 */
#define REG_ZERO 		31   /* allways zero                                  */

/* floating point registers */

#define REG_FRESULT     0    /* to deliver floating point method results      */ 
#define REG_FTMP1       28   /* temporary floating point register             */
#define REG_FTMP2       29   /* temporary floating point register             */
#define REG_FTMP3       30   /* temporary floating point register             */


/******************** register descripton - array *****************************/

/* #define REG_RES   0         reserved register for OS or code generator */
/* #define REG_RET   1         return value register */
/* #define REG_EXC   2         exception value register */
/* #define REG_SAV   3         (callee) saved register */
/* #define REG_TMP   4         scratch temporary register (caller saved) */
/* #define REG_ARG   5         argument register (caller saved) */

/* #define REG_END   -1        last entry in tables */
 
int regdescint[] = {
	REG_RET, REG_EXC, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, 
	REG_TMP, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, 
	REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_TMP, REG_TMP,
	REG_TMP, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES,
	REG_END };

#define INT_SAV_FST      9   /* first int callee saved register               */
#define INT_SAV_CNT      7   /* number of int callee saved registers          */
#define INT_ARG_FST     16   /* first int callee saved register               */
#define INT_ARG_CNT      6   /* number of int callee saved registers          */

/* for use of reserved registers, see comment above */
	
int regdescfloat[] = {
	REG_RET, REG_TMP, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV,
	REG_SAV, REG_SAV, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, 
	REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_TMP, REG_TMP,
	REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_RES, REG_RES, REG_RES, REG_RES,
	REG_END };

#define FLT_SAV_FST      2   /* first int callee saved register               */
#define FLT_SAV_CNT      8   /* number of int callee saved registers          */
#define FLT_ARG_FST     16   /* first int callee saved register               */
#define FLT_ARG_CNT      6   /* number of int callee saved registers          */

/* for use of reserved registers, see comment above */


/*** parameter allocation mode ***/

int reg_parammode = PARAMMODE_NUMBERED;  

   /* parameter-registers will be allocated by assigning the
      1. parameter:   int/float-reg 16
      2. parameter:   int/float-reg 17  
      3. parameter:   int/float-reg 18 ....
   */


/************************** stackframe-infos **********************************/

int localvars_base;  /* offset in stackframe for the local variables          */
int savedregs_base;  /* offset in stackframe for the save area                */
int parentargs_base; /* offset in stackframe for the parameter from the caller*/

/* -> see file 'calling.doc' */


/******************** macros to create code ***********************************/

/* 3-address-operations: M_OP3
      op ..... opcode
      fu ..... function-number
      a  ..... register number source 1
      b  ..... register number or constant integer source 2
      c  ..... register number destination
      const .. switch to use b as constant integer 
                 (0 means: use b as register number)
                 (1 means: use b as constant 8-bit-integer)
*/      
#define M_OP3(op,fu,a,b,c,const) \
	mcode_adds4 ( (((s4)(op))<<26)|((a)<<21)|((b)<<(16-3*(const)))| \
	((const)<<12)|((fu)<<5)|((c)) )

/* 3-address-floating-point-operation: M_FOP3 
     op .... opcode
     fu .... function-number
     a,b ... source floating-point registers
     c ..... destination register
*/ 
#define M_FOP3(op,fu,a,b,c) \
	mcode_adds4 ( (((s4)(op))<<26)|((a)<<21)|((b)<<16)|((fu)<<5)|(c) )

/* branch instructions: M_BRA 
      op ..... opcode
      a ...... register to be tested
      disp ... relative address to be jumped to (divided by 4)
*/
#define M_BRA(op,a,disp) \
	mcode_adds4 ( (((s4)(op))<<26)|((a)<<21)|((disp)&0x1fffff) )


/* memory operations: M_MEM
      op ..... opcode
      a ...... source/target register for memory access
      b ...... base register
      disp ... displacement (16 bit signed) to be added to b
*/ 
#define M_MEM(op,a,b,disp) \
	mcode_adds4 ( (((s4)(op))<<26)|((a)<<21)|((b)<<16)|((disp)&0xffff) )


/***** macros for all used commands (see an Alpha-manual for description) *****/ 

#define M_LDA(a,b,disp)         M_MEM (0x08,a,b,disp)           /* low const  */
#define M_LDAH(a,b,disp)        M_MEM (0x09,a,b,disp)           /* high const */
#define M_BLDU(a,b,disp)        M_MEM (0x0a,a,b,disp)           /*  8 load    */
#define M_SLDU(a,b,disp)        M_MEM (0x0c,a,b,disp)           /* 16 load    */
#define M_ILD(a,b,disp)         M_MEM (0x28,a,b,disp)           /* 32 load    */
#define M_LLD(a,b,disp)         M_MEM (0x29,a,b,disp)           /* 64 load    */
#define M_BST(a,b,disp)         M_MEM (0x0e,a,b,disp)           /*  8 store   */
#define M_SST(a,b,disp)         M_MEM (0x0d,a,b,disp)           /* 16 store   */
#define M_IST(a,b,disp)         M_MEM (0x2c,a,b,disp)           /* 32 store   */
#define M_LST(a,b,disp)         M_MEM (0x2d,a,b,disp)           /* 64 store   */

#define M_BSEXT(b,c)            M_OP3 (0x1c,0x0,REG_ZERO,b,c,0) /*  8 signext */
#define M_SSEXT(b,c)            M_OP3 (0x1c,0x1,REG_ZERO,b,c,0) /* 16 signext */

#define M_BR(disp)              M_BRA (0x30,REG_ZERO,disp)      /* branch     */
#define M_BSR(ra,disp)          M_BRA (0x34,ra,disp)            /* branch sbr */
#define M_BEQZ(a,disp)          M_BRA (0x39,a,disp)             /* br a == 0  */
#define M_BLTZ(a,disp)          M_BRA (0x3a,a,disp)             /* br a <  0  */
#define M_BLEZ(a,disp)          M_BRA (0x3b,a,disp)             /* br a <= 0  */
#define M_BNEZ(a,disp)          M_BRA (0x3d,a,disp)             /* br a != 0  */
#define M_BGEZ(a,disp)          M_BRA (0x3e,a,disp)             /* br a >= 0  */
#define M_BGTZ(a,disp)          M_BRA (0x3f,a,disp)             /* br a >  0  */

#define M_JMP(a,b)              M_MEM (0x1a,a,b,0x0000)         /* jump       */
#define M_JSR(a,b)              M_MEM (0x1a,a,b,0x4000)         /* call sbr   */
#define M_RET(a,b)              M_MEM (0x1a,a,b,0x8000)         /* return     */

#define M_IADD(a,b,c,const)     M_OP3 (0x10,0x0,  a,b,c,const)  /* 32 add     */
#define M_LADD(a,b,c,const)     M_OP3 (0x10,0x20, a,b,c,const)  /* 64 add     */
#define M_ISUB(a,b,c,const)     M_OP3 (0x10,0x09, a,b,c,const)  /* 32 sub     */
#define M_LSUB(a,b,c,const)     M_OP3 (0x10,0x29, a,b,c,const)  /* 64 sub     */
#define M_IMUL(a,b,c,const)     M_OP3 (0x13,0x00, a,b,c,const)  /* 32 mul     */
#define M_LMUL(a,b,c,const)     M_OP3 (0x13,0x20, a,b,c,const)  /* 64 mul     */

#define M_CMPEQ(a,b,c,const)    M_OP3 (0x10,0x2d, a,b,c,const)  /* c = a == b */
#define M_CMPLT(a,b,c,const)    M_OP3 (0x10,0x4d, a,b,c,const)  /* c = a <  b */
#define M_CMPLE(a,b,c,const)    M_OP3 (0x10,0x6d, a,b,c,const)  /* c = a <= b */

#define M_CMPULE(a,b,c,const)   M_OP3 (0x10,0x3d, a,b,c,const)  /* c = a <= b */

#define M_AND(a,b,c,const)      M_OP3 (0x11,0x00, a,b,c,const)  /* c = a &  b */
#define M_OR(a,b,c,const)       M_OP3 (0x11,0x20, a,b,c,const)  /* c = a |  b */
#define M_XOR(a,b,c,const)      M_OP3 (0x11,0x40, a,b,c,const)  /* c = a ^  b */

#define M_SLL(a,b,c,const)      M_OP3 (0x12,0x39, a,b,c,const)  /* c = a << b */
#define M_SRA(a,b,c,const)      M_OP3 (0x12,0x3c, a,b,c,const)  /* c = a >> b */
#define M_SRL(a,b,c,const)      M_OP3 (0x12,0x34, a,b,c,const)  /* c = a >>>b */

#define M_FLD(a,b,disp)         M_MEM (0x22,a,b,disp)           /* load flt   */
#define M_DLD(a,b,disp)         M_MEM (0x23,a,b,disp)           /* load dbl   */
#define M_FST(a,b,disp)         M_MEM (0x26,a,b,disp)           /* store flt  */
#define M_DST(a,b,disp)         M_MEM (0x27,a,b,disp)           /* store dbl  */

#define M_FADD(a,b,c)           M_FOP3 (0x16, 0x080, a,b,c)     /* flt add    */
#define M_DADD(a,b,c)           M_FOP3 (0x16, 0x0a0, a,b,c)     /* dbl add    */
#define M_FSUB(a,b,c)           M_FOP3 (0x16, 0x081, a,b,c)     /* flt sub    */
#define M_DSUB(a,b,c)           M_FOP3 (0x16, 0x0a1, a,b,c)     /* dbl sub    */
#define M_FMUL(a,b,c)           M_FOP3 (0x16, 0x082, a,b,c)     /* flt mul    */
#define M_DMUL(a,b,c)           M_FOP3 (0x16, 0x0a2, a,b,c)     /* dbl mul    */
#define M_FDIV(a,b,c)           M_FOP3 (0x16, 0x083, a,b,c)     /* flt div    */
#define M_DDIV(a,b,c)           M_FOP3 (0x16, 0x0a3, a,b,c)     /* dbl div    */

#define M_CVTLF(a,b,c)          M_FOP3 (0x16, 0x0bc, a,b,c)     /* long2flt   */
#define M_CVTLD(a,b,c)          M_FOP3 (0x16, 0x0be, a,b,c)     /* long2dbl   */
#define M_CVTDL(a,b,c)          M_FOP3 (0x16, 0x0af, a,b,c)     /* dbl2long   */
#define M_CVTDL_C(a,b,c)        M_FOP3 (0x16, 0x02f, a,b,c)     /* dbl2long   */

#define M_FCMPEQ(a,b,c)         M_FOP3 (0x16, 0x0a5, a,b,c)     /* c = a==b   */
#define M_FCMPLT(a,b,c)         M_FOP3 (0x16, 0x0a6, a,b,c)     /* c = a<b    */

#define M_FMOV(fa,fb)           M_FOP3 (0x17, 0x020, fa,fa,fb)  /* b = a      */
#define M_FMOVN(fa,fb)          M_FOP3 (0x17, 0x021, fa,fa,fb)  /* b = -a     */

#define M_FBEQZ(fa,disp)        M_BRA (0x31,fa,disp)            /* br a == 0.0*/

/****** macros for special commands (see an Alpha-manual for description) *****/ 

#define M_S4ADDL(a,b,c,const)   M_OP3 (0x10,0x02, a,b,c,const) /* c = a*4 + b */
#define M_S4ADDQ(a,b,c,const)   M_OP3 (0x10,0x22, a,b,c,const) /* c = a*4 + b */
#define M_S4SUBL(a,b,c,const)   M_OP3 (0x10,0x0b, a,b,c,const) /* c = a*4 - b */
#define M_S4SUBQ(a,b,c,const)   M_OP3 (0x10,0x2b, a,b,c,const) /* c = a*4 - b */
#define M_S8ADDL(a,b,c,const)   M_OP3 (0x10,0x12, a,b,c,const) /* c = a*8 + b */
#define M_S8ADDQ(a,b,c,const)   M_OP3 (0x10,0x32, a,b,c,const) /* c = a*8 + b */
#define M_S8SUBL(a,b,c,const)   M_OP3 (0x10,0x1b, a,b,c,const) /* c = a*8 - b */
#define M_S8SUBQ(a,b,c,const)   M_OP3 (0x10,0x3b, a,b,c,const) /* c = a*8 - b */

#define M_LLD_U(a,b,disp)       M_MEM (0x0b,a,b,disp)          /* unalign ld  */
#define M_LST_U(a,b,disp)       M_MEM (0x0f,a,b,disp)          /* unalign st  */

#define M_ZAP(a,b,c,const)      M_OP3 (0x12,0x30, a,b,c,const)
#define M_ZAPNOT(a,b,c,const)   M_OP3 (0x12,0x31, a,b,c,const)
#define M_EXTBL(a,b,c,const)    M_OP3 (0x12,0x06, a,b,c,const)
#define M_EXTWL(a,b,c,const)    M_OP3 (0x12,0x16, a,b,c,const)
#define M_EXTLL(a,b,c,const)    M_OP3 (0x12,0x26, a,b,c,const)
#define M_EXTQL(a,b,c,const)    M_OP3 (0x12,0x36, a,b,c,const)
#define M_EXTWH(a,b,c,const)    M_OP3 (0x12,0x5a, a,b,c,const)
#define M_EXTLH(a,b,c,const)    M_OP3 (0x12,0x6a, a,b,c,const)
#define M_EXTQH(a,b,c,const)    M_OP3 (0x12,0x7a, a,b,c,const)
#define M_INSBL(a,b,c,const)    M_OP3 (0x12,0x0b, a,b,c,const)
#define M_INSWL(a,b,c,const)    M_OP3 (0x12,0x1b, a,b,c,const)
#define M_INSLL(a,b,c,const)    M_OP3 (0x12,0x2b, a,b,c,const)
#define M_INSQL(a,b,c,const)    M_OP3 (0x12,0x3b, a,b,c,const)
#define M_INSWH(a,b,c,const)    M_OP3 (0x12,0x57, a,b,c,const)
#define M_INSLH(a,b,c,const)    M_OP3 (0x12,0x67, a,b,c,const)
#define M_INSQH(a,b,c,const)    M_OP3 (0x12,0x77, a,b,c,const)
#define M_MSKBL(a,b,c,const)    M_OP3 (0x12,0x02, a,b,c,const)
#define M_MSKWL(a,b,c,const)    M_OP3 (0x12,0x12, a,b,c,const)
#define M_MSKLL(a,b,c,const)    M_OP3 (0x12,0x22, a,b,c,const)
#define M_MSKQL(a,b,c,const)    M_OP3 (0x12,0x32, a,b,c,const)
#define M_MSKWH(a,b,c,const)    M_OP3 (0x12,0x52, a,b,c,const)
#define M_MSKLH(a,b,c,const)    M_OP3 (0x12,0x62, a,b,c,const)
#define M_MSKQH(a,b,c,const)    M_OP3 (0x12,0x72, a,b,c,const)


/****** macros for unused commands (see an Alpha-manual for description) ******/ 

#define M_ANDNOT(a,b,c,const)   M_OP3 (0x11,0x08, a,b,c,const) /* c = a &~ b  */
#define M_ORNOT(a,b,c,const)    M_OP3 (0x11,0x28, a,b,c,const) /* c = a |~ b  */
#define M_XORNOT(a,b,c,const)   M_OP3 (0x11,0x48, a,b,c,const) /* c = a ^~ b  */

#define M_CMOVEQ(a,b,c,const)   M_OP3 (0x11,0x24, a,b,c,const) /* a==0 ? c=b  */
#define M_CMOVNE(a,b,c,const)   M_OP3 (0x11,0x26, a,b,c,const) /* a!=0 ? c=b  */
#define M_CMOVLT(a,b,c,const)   M_OP3 (0x11,0x44, a,b,c,const) /* a< 0 ? c=b  */
#define M_CMOVGE(a,b,c,const)   M_OP3 (0x11,0x46, a,b,c,const) /* a>=0 ? c=b  */
#define M_CMOVLE(a,b,c,const)   M_OP3 (0x11,0x64, a,b,c,const) /* a<=0 ? c=b  */
#define M_CMOVGT(a,b,c,const)   M_OP3 (0x11,0x66, a,b,c,const) /* a> 0 ? c=b  */

#define M_CMPULT(a,b,c,const)   M_OP3 (0x10,0x1d, a,b,c,const)
#define M_CMPBGE(a,b,c,const)   M_OP3 (0x10,0x0f, a,b,c,const)

#define M_FCMPUN(a,b,c)         M_FOP3 (0x16, 0x0a4, a,b,c)    /* unordered   */
#define M_FCMPLE(a,b,c)         M_FOP3 (0x16, 0x0a7, a,b,c)    /* c = a<=b    */

#define M_FBNEZ(fa,disp)        M_BRA (0x35,fa,disp)
#define M_FBLEZ(fa,disp)        M_BRA (0x33,fa,disp)

#define M_JMP_CO(a,b)           M_MEM (0x1a,a,b,0xc000)        /* call cosub  */


/******************** system independent macros *******************************/
 


/************** additional functions to generate code *************************/


/* M_INTMOVE:
     generates an integer-move from register a to b.
     if a and b are the same int-register, no code will be generated.
*/ 

static void M_INTMOVE(int a, int b)
{
	if (a == b) return;
	M_OR (a,a,b, 0);
}


/* M_FLTMOVE:
    generates a floating-point-move from register a to b.
    if a and b are the same float-register, no code will be generated
*/ 

static void M_FLTMOVE(int a, int b)
{
	if (a == b) return;
	M_FMOV (a, b);
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

static int var_to_reg_int (varid v, int tempregnum)
{
	reginfo *ri = v->reg;

	if (!(ri->typeflags & REG_INMEMORY))
		return ri->num;
#ifdef STATISTICS
	count_spills++;
#endif
	M_LLD (tempregnum, REG_SP, 8 * (localvars_base + ri->num) );
	return tempregnum;
}


static int var_to_reg_flt (varid v, int tempregnum)
{
	reginfo *ri = v->reg;


	if (!(ri->typeflags & REG_INMEMORY))
		return ri->num;
#ifdef STATISTICS
	count_spills++;
#endif
	M_DLD (tempregnum, REG_SP, 8 * (localvars_base + ri->num) );
	return tempregnum;
}


/* reg_of_var:
     This function determines a register, to which the result of an
     operation should go, when it is ultimatively intended to store the result
     in pseudoregister v.
     If v is assigned to an actual register, this register will be
     returned.
     Otherwise (when v is spilled) this function returns tempregnum.
*/        

static int reg_of_var(varid v, int tempregnum)
{
	if (!(v->reg->typeflags & REG_INMEMORY))
		return v->reg->num;
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

static void store_reg_to_var_int (varid v, int tempregnum)
{
	reginfo *ri = v->reg;

	if (!(ri->typeflags & REG_INMEMORY))
		return;
#ifdef STATISTICS
	count_spills++;
#endif
	M_LST (tempregnum, REG_SP, 8 * (localvars_base + ri->num) );
}

static void store_reg_to_var_flt (varid v, int tempregnum)
{
	reginfo *ri = v->reg;

	if (!(ri->typeflags & REG_INMEMORY))
		return;
#ifdef STATISTICS
	count_spills++;
#endif
	M_DST (tempregnum, REG_SP, 8 * (localvars_base + ri->num) );
}


/***************** functions to process the pseudo commands *******************/

/* gen_method:
    function to generate method-call
*/

static void gen_method (pcmd *c)
{
	int p, pa, r;
	s4  a;
	reginfo *ri;
	classinfo *ci;
	
	for (p = 0; p < c->u.method.paramnum; p++) {
 		ri = c->u.method.params[p]->reg;

 		if (p < INT_ARG_CNT) {            /* arguments that go into registers */
 			if (IS_INT_LNG_REG(ri->typeflags)) {
 				if (!(ri->typeflags & REG_INMEMORY))
 					M_INTMOVE(ri->num, INT_ARG_FST+p);
 				else
 					M_LLD(INT_ARG_FST+p, REG_SP, 8*(localvars_base + ri->num));
 				}
 			else {
 				if (!(ri->typeflags & REG_INMEMORY))
 					M_FLTMOVE (ri->num, FLT_ARG_FST+p);
 				else
 					M_DLD(FLT_ARG_FST+p, REG_SP, 8*(localvars_base + ri->num));
 				}
 			}
 		else {                             /* arguments that go into memory   */
 			pa = p - INT_ARG_CNT;
 			if (pa >= arguments_num)
 				panic ("invalid stackframe structure");
 			
 			if (IS_INT_LNG_REG(ri->typeflags)) {
 				r = var_to_reg_int (c->u.method.params[p], REG_ITMP1);
				M_LST(r, REG_SP, 8 * (0 + pa));
 				}
 			else {
 				r = var_to_reg_flt (c->u.method.params[p], REG_FTMP1);
				M_DST (r, REG_SP, 8 * (0 + pa));
				}
 			}
 		}  /* end of for */

	switch (c->opcode) {
		case CMD_BUILTIN:
			a = dseg_addaddress ( (void*) (c->u.method.builtin) );

			M_LLD (REG_PV, REG_PV, a);        /* Pointer to built-in-function */
			goto makeactualcall;

		case CMD_INVOKESTATIC:
		case CMD_INVOKESPECIAL:
			a = dseg_addaddress ( c->u.method.method->stubroutine );

			M_LLD (REG_PV, REG_PV, a );              /* Method-Pointer in r27 */

			goto makeactualcall;

		case CMD_INVOKEVIRTUAL:

			M_LLD (REG_METHODPTR, 16, OFFSET(java_objectheader, vftbl));
			M_LLD (REG_PV, REG_METHODPTR,
			   OFFSET(vftbl,table[0]) + 
			   sizeof(methodptr) * c->u.method.method->vftblindex );

			goto makeactualcall;

		case CMD_INVOKEINTERFACE:
			ci = c->u.method.method->class;
			
			M_LLD (REG_METHODPTR, 16, OFFSET(java_objectheader, vftbl));    
			M_LLD (REG_METHODPTR, REG_METHODPTR, OFFSET(vftbl, interfacetable[0]) -
			       sizeof(methodptr*) * ci->index);
			M_LLD (REG_PV, REG_METHODPTR, sizeof(methodptr) * (c->u.method.method - ci->methods) );

			goto makeactualcall;

		default:
			sprintf (logtext, "Unkown PCMD-Command: %d", c->opcode);
			error ();
		}


makeactualcall:


	M_JSR (REG_RA, REG_PV);
	if (mcodelen<=32768) M_LDA (REG_PV, REG_RA, -mcodelen);
	else {
		s4 ml=-mcodelen, mh=0;
		while (ml<-32768) { ml+=65536; mh--; }
		M_LDA (REG_PV, REG_RA, ml );
		M_LDAH (REG_PV, REG_PV, mh );
	}
	
	if ( c->dest ) {
		ri = c->dest->reg;

		if (IS_INT_LNG_REG(ri->typeflags)) {
			if (!(ri->typeflags & REG_INMEMORY))
				M_INTMOVE (REG_RESULT, ri->num);
			else  M_LST (REG_RESULT, REG_SP, 8 * (localvars_base + ri->num) );
			}
		else {
			if (!(ri->typeflags & REG_INMEMORY))
				M_FLTMOVE (REG_RESULT, ri->num);
			else  M_DST (REG_RESULT, REG_SP, 8 * (localvars_base + ri->num) );
			}
		}

	if (c->u.method.exceptionvar) {
		ri = c->u.method.exceptionvar->reg;
		if (!(ri->typeflags & REG_INMEMORY))
			M_INTMOVE (REG_EXCEPTION, ri->num);	
		else M_LST (REG_EXCEPTION, REG_SP, 8 *(localvars_base + ri->num) );
		}
}


/************************ function block_genmcode ******************************

	generates machine code for a complete basic block

*******************************************************************************/


static void block_genmcode(basicblock *b)
{
	int  s1, s2, s3, d;
	s4   a;
	pcmd *c;
			
	mcode_blockstart (b);

	for(c = list_first(&(b->pcmdlist));
	    c != NULL;
	    c = list_next(&(b->pcmdlist), c))

	switch (c->opcode) {

		case CMD_TRACEBUILT:
			M_LDA (REG_SP, REG_SP, -8);
			a = dseg_addaddress (c->u.a.value);
			M_LLD(REG_ITMP1, REG_PV, a);
			M_LST(REG_ITMP1, REG_SP, 0);
/*  			a = dseg_addaddress ((void*) (builtin_trace_args)); */
			M_LLD(REG_PV, REG_PV, a);
			M_JSR (REG_RA, REG_PV);
			if (mcodelen<=32768) M_LDA (REG_PV, REG_RA, -mcodelen);
			else {
				s4 ml=-mcodelen, mh=0;
				while (ml<-32768) { ml+=65536; mh--; }
				M_LDA (REG_PV, REG_RA, ml );
				M_LDAH (REG_PV, REG_PV, mh );
				}
			M_LDA (REG_SP, REG_SP, 8);
			b -> mpc = mcodelen;
			break;


		/*********************** constant operations **************************/

		case CMD_LOADCONST_I:
			d = reg_of_var(c->dest, REG_ITMP1);
			if ( (c->u.i.value >= -32768) && (c->u.i.value <= 32767) ) {
				M_LDA (d, REG_ZERO, c->u.i.value);
				} 
			else {
				a = dseg_adds4 (c->u.i.value);
				M_ILD (d, REG_PV, a);
				}
			store_reg_to_var_int(c->dest, d);
			break;

		case CMD_LOADCONST_L:
			d = reg_of_var(c->dest, REG_ITMP1);
#if U8_AVAILABLE
			if ((c->u.l.value >= -32768) && (c->u.l.value <= 32767) ) {
				M_LDA (d, REG_ZERO, c->u.l.value);
				} 
			else {
				a = dseg_adds8 (c->u.l.value);
				M_LLD (d, REG_PV, a);
				}
#else
			a = dseg_adds8 (c->u.l.value);
			M_LLD (d, REG_PV, a);
#endif
			store_reg_to_var_int(c->dest, d);
			break;

		case CMD_LOADCONST_F:
			d = reg_of_var (c->dest, REG_FTMP1);
			a = dseg_addfloat (c->u.f.value);
			M_FLD (d, REG_PV, a);
			store_reg_to_var_flt (c->dest, d);
			break;
			
		case CMD_LOADCONST_D:
			d = reg_of_var (c->dest, REG_FTMP1);
			a = dseg_adddouble (c->u.d.value);
			M_DLD (d, REG_PV, a);
			store_reg_to_var_flt (c->dest, d);
			break;


		case CMD_LOADCONST_A:
			d = reg_of_var(c->dest, REG_ITMP1);
			if (c->u.a.value) {
				a = dseg_addaddress (c->u.a.value);
				M_LLD (d, REG_PV, a);
				}
			else {
				M_INTMOVE (REG_ZERO, d);
				}
			store_reg_to_var_int(c->dest, d);
			break;

		/************************* move operation *****************************/

		case CMD_MOVE:
			if (IS_INT_LNG_REG(c->source1->reg->typeflags)) {
				d = reg_of_var(c->dest, REG_ITMP1);
				s1 = var_to_reg_int(c->source1, d);
				M_INTMOVE(s1,d);
				store_reg_to_var_int(c->dest, d);
				}
			else {
				d = reg_of_var(c->dest, REG_FTMP1);
				s1 = var_to_reg_flt(c->source1, d);
				M_FLTMOVE(s1,d);
				store_reg_to_var_flt(c->dest, d);
				}
			break;


		/********************* integer operations *****************************/

		case CMD_IINC:
			s1 = var_to_reg_int(c->source1, REG_ITMP1); 
			d = reg_of_var(c->dest, REG_ITMP3);
			if ((c->u.i.value >= 0) && (c->u.i.value <= 256)) {
				M_IADD (s1, c->u.i.value, d, 1);
				}
			else if ((c->u.i.value >= -256) && (c->u.i.value < 0)) {
				M_ISUB (s1, (-c->u.i.value), d, 1);
				}
			else {
				M_LDA  (d, s1, c->u.i.value);
				M_IADD (d, REG_ZERO, d, 0);
				}
			store_reg_to_var_int(c->dest, d);
			break;

		case CMD_INEG:
			s1 = var_to_reg_int(c->source1, REG_ITMP1); 
			d = reg_of_var(c->dest, REG_ITMP3);
			M_ISUB (REG_ZERO, s1, d, 0);
			store_reg_to_var_int(c->dest, d);
			break;

		case CMD_LNEG:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_LSUB (REG_ZERO, s1, d, 0);
			store_reg_to_var_int(c->dest, d);
			break;

		case CMD_I2L:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_INTMOVE (s1, d);
			store_reg_to_var_int(c->dest, d);
			break;

		case CMD_L2I:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_IADD (s1, REG_ZERO, d , 0);
			store_reg_to_var_int(c->dest, d);
			break;

		case CMD_INT2BYTE:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			d = reg_of_var(c->dest, REG_ITMP3);
			if (has_ext_instr_set) {
				M_BSEXT  (s1, d);
				}
			else {
				M_SLL (s1,56, d, 1);
				M_SRA ( d,56, d, 1);
				}
			store_reg_to_var_int(c->dest, d);
			break;

		case CMD_INT2CHAR:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			d = reg_of_var(c->dest, REG_ITMP3);
            M_ZAPNOT (s1, 0x03, d, 1);
			store_reg_to_var_int(c->dest, d);
			break;

		case CMD_INT2SHORT:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			d = reg_of_var(c->dest, REG_ITMP3);
			if (has_ext_instr_set) {
				M_SSEXT  (s1, d);
				}
			else {
				M_SLL ( s1, 48, d, 1);
				M_SRA (  d, 48, d, 1);
				}
			store_reg_to_var_int(c->dest, d);
			break;

		case CMD_IADD:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_IADD (s1, s2, d,  0);
			store_reg_to_var_int(c->dest, d);
			break;
		case CMD_LADD:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_LADD (s1, s2, d,  0);
			store_reg_to_var_int(c->dest, d);
			break;

		case CMD_ISUB:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_ISUB (s1, s2, d, 0);
			store_reg_to_var_int(c->dest, d);
			break;
		case CMD_LSUB:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_LSUB (s1, s2, d, 0);
			store_reg_to_var_int(c->dest, d);
			break;

		case CMD_IMUL:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_IMUL (s1, s2, d, 0);
			store_reg_to_var_int(c->dest, d);
			break;
		case CMD_LMUL:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_LMUL (s1, s2, d, 0);
			store_reg_to_var_int(c->dest, d);
			break;

		    
		case CMD_ISHL:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_AND (s2, 0x1f, REG_ITMP3, 1);
			M_SLL (s1, REG_ITMP3, d, 0);
			M_IADD (d, REG_ZERO, d, 0);
			store_reg_to_var_int(c->dest, d);
			break;

		case CMD_ISHR:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_AND (s2, 0x1f, REG_ITMP3,  1);
			M_SRA (s1, REG_ITMP3, d,   0);
			store_reg_to_var_int(c->dest, d);
			break;

		case CMD_IUSHR:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_AND    (s2, 0x1f, REG_ITMP2,  1);
            M_ZAPNOT (s1, 0x0f, d, 1);
			M_SRL    ( d, REG_ITMP2, d, 0);
			M_IADD   ( d, REG_ZERO, d, 0);
			store_reg_to_var_int(c->dest, d);
			break;

		case CMD_LSHL:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_SLL (s1, s2, d,  0);
			store_reg_to_var_int(c->dest, d);
			break;

		case CMD_LSHR:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_SRA (s1, s2, d,  0);
			store_reg_to_var_int(c->dest, d);
			break;

		case CMD_LUSHR:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_SRL (s1, s2, d,  0);
			store_reg_to_var_int(c->dest, d);
			break;

		case CMD_IAND:
		case CMD_LAND:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_AND (s1, s2, d, 0);
			store_reg_to_var_int(c->dest, d);
			break;

		case CMD_IOR:
		case CMD_LOR:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_OR   ( s1,s2, d, 0);
			store_reg_to_var_int(c->dest, d);
			break;

		case CMD_IXOR:
		case CMD_LXOR:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_XOR (s1, s2, d, 0);
			store_reg_to_var_int(c->dest, d);
			break;


		case CMD_LCMP:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_CMPLT  (s1, s2, REG_ITMP3, 0);
			M_CMPLT  (s2, s1, REG_ITMP1, 0);
			M_LSUB   (REG_ITMP1, REG_ITMP3, d, 0);
			store_reg_to_var_int(c->dest, d);
			break;
			

		/*********************** floating operations **************************/

		case CMD_FNEG:
			s1 = var_to_reg_flt(c->source1, REG_FTMP1);
			d = reg_of_var(c->dest, REG_FTMP3);
			M_FMOVN (s1, d);
			store_reg_to_var_flt(c->dest, d);
			break;
		case CMD_DNEG:
			s1 = var_to_reg_flt(c->source1, REG_FTMP1);
			d = reg_of_var(c->dest, REG_FTMP3);
			M_FMOVN (s1, d);
			store_reg_to_var_flt(c->dest, d);
			break;

		case CMD_FADD:
			s1 = var_to_reg_flt(c->source1, REG_FTMP1);
			s2 = var_to_reg_flt(c->source2, REG_FTMP2);
			d = reg_of_var(c->dest, REG_FTMP3);
			M_FADD ( s1,s2, d);
			store_reg_to_var_flt(c->dest, d);
			break;
		case CMD_DADD:
			s1 = var_to_reg_flt(c->source1, REG_FTMP1);
			s2 = var_to_reg_flt(c->source2, REG_FTMP2);
			d = reg_of_var(c->dest, REG_FTMP3);
			M_DADD (s1, s2, d);
			store_reg_to_var_flt(c->dest, d);
			break;

		case CMD_FSUB:
			s1 = var_to_reg_flt(c->source1, REG_FTMP1);
			s2 = var_to_reg_flt(c->source2, REG_FTMP2);
			d = reg_of_var(c->dest, REG_FTMP3);
			M_FSUB (s1, s2, d);
			store_reg_to_var_flt(c->dest, d);
			break;
		case CMD_DSUB:
			s1 = var_to_reg_flt(c->source1, REG_FTMP1);
			s2 = var_to_reg_flt(c->source2, REG_FTMP2);
			d = reg_of_var(c->dest, REG_FTMP3);
			M_DSUB (s1, s2, d);
			store_reg_to_var_flt(c->dest, d);
			break;

		case CMD_FMUL:
			s1 = var_to_reg_flt(c->source1, REG_FTMP1);
			s2 = var_to_reg_flt(c->source2, REG_FTMP2);
			d = reg_of_var(c->dest, REG_FTMP3);
			M_FMUL   (s1, s2, d);
			store_reg_to_var_flt(c->dest, d);
			break;
		case CMD_DMUL:
			s1 = var_to_reg_flt(c->source1, REG_FTMP1);
			s2 = var_to_reg_flt(c->source2, REG_FTMP2);
			d = reg_of_var(c->dest, REG_FTMP3);
			M_DMUL   (s1, s2, d);
			store_reg_to_var_flt(c->dest, d);
			break;

		case CMD_FDIV:
			s1 = var_to_reg_flt(c->source1, REG_FTMP1);
			s2 = var_to_reg_flt(c->source2, REG_FTMP2);
			d = reg_of_var(c->dest, REG_FTMP3);
			M_FDIV   (s1, s2, d);
			store_reg_to_var_flt(c->dest, d);
			break;
		case CMD_DDIV:
			s1 = var_to_reg_flt(c->source1, REG_FTMP1);
			s2 = var_to_reg_flt(c->source2, REG_FTMP2);
			d = reg_of_var(c->dest, REG_FTMP3);
			M_DDIV   (s1, s2, d);
			store_reg_to_var_flt(c->dest, d);
			break;
		
		case CMD_FREM:
			s1 = var_to_reg_flt(c->source1, REG_FTMP1);
			s2 = var_to_reg_flt(c->source2, REG_FTMP2);
			d = reg_of_var(c->dest, REG_FTMP3);
			M_FDIV   ( s1,s2, REG_FTMP3 );
			M_CVTDL_C ( REG_ZERO, REG_FTMP3, REG_FTMP3 ); /* round to integer */
			M_CVTLF ( REG_ZERO, REG_FTMP3, REG_FTMP3 );
			M_FMUL ( REG_FTMP3, s2, REG_FTMP3 );
			M_FSUB ( s1, REG_FTMP3, d);
			store_reg_to_var_flt(c->dest, d);
		    break;
		case CMD_DREM:
			s1 = var_to_reg_flt(c->source1, REG_FTMP1);
			s2 = var_to_reg_flt(c->source2, REG_FTMP2);
			d = reg_of_var(c->dest, REG_FTMP3);
			M_DDIV   ( s1,s2, REG_FTMP3 );
			M_CVTDL_C ( REG_ZERO, REG_FTMP3, REG_FTMP3 ); /* round to integer */
			M_CVTLD ( REG_ZERO, REG_FTMP3, REG_FTMP3 );
			M_DMUL ( REG_FTMP3, s2, REG_FTMP3 );
			M_DSUB ( s1, REG_FTMP3, d);
			store_reg_to_var_flt(c->dest, d);
		    break;

		case CMD_I2F:
		case CMD_L2F:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			d = reg_of_var(c->dest, REG_FTMP3);
			a = dseg_adddouble(0.0);
			M_LST (s1, REG_PV, a);
			M_DLD (d, REG_PV, a);
			M_CVTLF ( REG_ZERO, d, d);
			store_reg_to_var_flt(c->dest, d);
			break;

		case CMD_I2D:
		case CMD_L2D:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			d = reg_of_var(c->dest, REG_FTMP3);
			a = dseg_adddouble(0.0);
			M_LST (s1, REG_PV, a);
			M_DLD (d, REG_PV, a);
			M_CVTLD ( REG_ZERO, d, d);
			store_reg_to_var_flt(c->dest, d);
			break;
			
		case CMD_F2I:
		case CMD_D2I:
			s1 = var_to_reg_flt(c->source1, REG_FTMP1);
			d = reg_of_var(c->dest, REG_ITMP3);
			a = dseg_adddouble(0.0);
			M_CVTDL_C (REG_ZERO, s1, REG_FTMP1);
			M_DST (REG_FTMP1, REG_PV, a);
			M_ILD (d, REG_PV, a);
			store_reg_to_var_int(c->dest, d);
			break;
		
		case CMD_F2L:
		case CMD_D2L:
			s1 = var_to_reg_flt(c->source1, REG_FTMP1);
			d = reg_of_var(c->dest, REG_ITMP3);
			a = dseg_adddouble(0.0);
			M_CVTDL_C (REG_ZERO, s1, REG_FTMP1);
			M_DST (REG_FTMP1, REG_PV, a);
			M_LLD (d, REG_PV, a);
			store_reg_to_var_int(c->dest, d);
			break;

		case CMD_F2D:
			s1 = var_to_reg_flt(c->source1, REG_FTMP1);
			d = reg_of_var(c->dest, REG_FTMP3);
			M_FLTMOVE (s1, d);
			store_reg_to_var_flt(c->dest, d);
			break;
					
		case CMD_D2F:
			s1 = var_to_reg_flt(c->source1, REG_FTMP1);
			d = reg_of_var(c->dest, REG_FTMP3);
			M_FADD (s1, REG_ZERO, d);
			store_reg_to_var_flt(c->dest, d);
			break;
		
		case CMD_FCMPL:
		case CMD_DCMPL:
			s1 = var_to_reg_flt(c->source1, REG_FTMP1);
			s2 = var_to_reg_flt(c->source2, REG_FTMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_LSUB   (REG_ZERO, 1, d, 1);
			M_FCMPEQ (s1, s2, REG_FTMP3);
			M_FBEQZ  (REG_FTMP3, 1);   /* jump over next instructions         */
			M_OR     (REG_ZERO, REG_ZERO, d, 0);
			M_FCMPLT (s2, s1, REG_FTMP3);
			M_FBEQZ  (REG_FTMP3, 1);   /* jump over next instruction          */
			M_LADD   (REG_ZERO, 1, d, 1);
			store_reg_to_var_int(c->dest, d);
			break;
			
		case CMD_FCMPG:
		case CMD_DCMPG:
			s1 = var_to_reg_flt(c->source1, REG_FTMP1);
			s2 = var_to_reg_flt(c->source2, REG_FTMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_LADD   (REG_ZERO, 1, d, 1);
			M_FCMPEQ (s1, s2, REG_FTMP3);
			M_FBEQZ  (REG_FTMP3, 1);   /* jump over next instruction          */
			M_OR     (REG_ZERO, REG_ZERO, d, 0);
			M_FCMPLT (s1, s2, REG_FTMP3);
			M_FBEQZ  (REG_FTMP3, 1);   /* jump over next instruction          */
			M_LSUB   (REG_ZERO, 1, d, 1);
			store_reg_to_var_int(c->dest, d);
			break;


		/********************** memory operations *****************************/

		case CMD_ARRAYLENGTH:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_ILD	 (d, s1, OFFSET(java_arrayheader, size));
			store_reg_to_var_int(c->dest, d);
			break;

		case CMD_AALOAD:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_S8ADDQ (s2, s1, REG_ITMP1, 0);
			M_LLD    ( d, REG_ITMP1, OFFSET (java_objectarray, data[0]));
			store_reg_to_var_int(c->dest, d);
			break;
		case CMD_LALOAD:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_S8ADDQ (s2, s1, REG_ITMP1, 0);
			M_LLD    ( d, REG_ITMP1, OFFSET (java_longarray, data[0]));
			store_reg_to_var_int(c->dest, d);
			break;
		case CMD_IALOAD:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			M_S4ADDQ (s2, s1, REG_ITMP1, 0);
			M_ILD    ( d, REG_ITMP1, OFFSET (java_intarray, data[0]));
			store_reg_to_var_int(c->dest, d);
			break;
		case CMD_FALOAD:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_FTMP3);
			M_S4ADDQ (s2, s1, REG_ITMP1, 0);
			M_FLD    ( d, REG_ITMP1, OFFSET (java_floatarray, data[0]));
			store_reg_to_var_flt(c->dest, d);
			break;
		case CMD_DALOAD:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_FTMP3);
			M_S8ADDQ (s2, s1, REG_ITMP1, 0);
			M_DLD    ( d, REG_ITMP1, OFFSET (java_doublearray, data[0]));
			store_reg_to_var_flt(c->dest, d);
			break;
		case CMD_CALOAD:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			if (has_ext_instr_set) {
				M_LADD   (s2, s1, REG_ITMP1, 0);
				M_LADD   (s2, REG_ITMP1, REG_ITMP1, 0);
				M_SLDU   (d, REG_ITMP1, OFFSET (java_chararray, data[0]));
				}
			else {
				M_LADD   (s2, s1, REG_ITMP1,  0);
				M_LADD   (s2, REG_ITMP1, REG_ITMP1, 0);
				M_LLD_U  (REG_ITMP2, REG_ITMP1, OFFSET(java_chararray, data[0]));
				M_LDA    (REG_ITMP1, REG_ITMP1, OFFSET(java_chararray, data[0]));
				M_EXTWL  (REG_ITMP2, REG_ITMP1, d, 0);
				}
			store_reg_to_var_int(c->dest, d);
			break;			
		case CMD_SALOAD:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			if (has_ext_instr_set) {
				M_LADD   (s2, s1, REG_ITMP1, 0);
				M_LADD   (s2, REG_ITMP1, REG_ITMP1, 0);
				M_SLDU   (d, REG_ITMP1, OFFSET (java_shortarray, data[0]));
				M_SSEXT  (d, d);
				}
			else {
				M_LADD   (s2, s1, REG_ITMP1,  0);
				M_LADD   (s2, REG_ITMP1, REG_ITMP1, 0);
				M_LLD_U  (REG_ITMP2, REG_ITMP1, OFFSET(java_shortarray, data[0]));
				M_LDA    (REG_ITMP1, REG_ITMP1, OFFSET(java_shortarray, data[0]) + 2);
				M_EXTQH  (REG_ITMP2, REG_ITMP1, d, 0);
				M_SRA    (d, 48, d, 1);
				}
			store_reg_to_var_int(c->dest, d);
			break;
		case CMD_BALOAD:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			d = reg_of_var(c->dest, REG_ITMP3);
			if (has_ext_instr_set) {
				M_LADD   (s2, s1, REG_ITMP1, 0);
				M_BLDU   (d, REG_ITMP1, OFFSET (java_shortarray, data[0]));
				M_BSEXT  (d, d);
				}
			else {
				M_LADD   (s2, s1, REG_ITMP1,  0);
				M_LLD_U  (REG_ITMP2, REG_ITMP1, OFFSET(java_bytearray, data[0]));
				M_LDA    (REG_ITMP1, REG_ITMP1, OFFSET(java_bytearray, data[0]) + 1);
				M_EXTQH  (REG_ITMP2, REG_ITMP1, d,   0);
				M_SRA    (d, 56, d,  1);
				}
			store_reg_to_var_int(c->dest, d);
			break;

		case CMD_AASTORE:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			s3 = var_to_reg_int(c->source3, REG_ITMP3);
			M_S8ADDQ (s2, s1, REG_ITMP1, 0);
			M_LST    (s3, REG_ITMP1, OFFSET (java_objectarray, data[0]));
			break;
		case CMD_LASTORE:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			s3 = var_to_reg_int(c->source3, REG_ITMP3);
			M_S8ADDQ (s2, s1, REG_ITMP1, 0);
			M_LST    (s3, REG_ITMP1, OFFSET (java_longarray, data[0]));
			break;
		case CMD_IASTORE:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			s3 = var_to_reg_int(c->source3, REG_ITMP3);
			M_S4ADDQ (s2, s1, REG_ITMP1, 0);
			M_IST    (s3, REG_ITMP1, OFFSET (java_intarray, data[0]));
			break;
		case CMD_FASTORE:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			s3 = var_to_reg_flt(c->source3, REG_FTMP3);
			M_S4ADDQ (s2, s1, REG_ITMP1, 0);
			M_FST    (s3, REG_ITMP1, OFFSET (java_floatarray, data[0]));
			break;
		case CMD_DASTORE:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			s3 = var_to_reg_flt(c->source3, REG_FTMP3);
			M_S8ADDQ (s2, s1, REG_ITMP1, 0);
			M_DST    (s3, REG_ITMP1, OFFSET (java_doublearray, data[0]));
			break;
		case CMD_CASTORE:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			s3 = var_to_reg_int(c->source3, REG_ITMP3);
			if (has_ext_instr_set) {
				M_LADD   (s2, s1, REG_ITMP1, 0);
				M_LADD   (s2, REG_ITMP1, REG_ITMP1, 0);
				M_SST    (s3, REG_ITMP1, OFFSET (java_chararray, data[0]));
				}
			else {
				M_LADD   (s2, s1, REG_ITMP1, 0);
				M_LADD   (s2, REG_ITMP1, REG_ITMP1, 0);
				M_LLD_U  (REG_ITMP2, REG_ITMP1, OFFSET(java_chararray, data[0]));
				M_LDA    (REG_ITMP1, REG_ITMP1, OFFSET(java_chararray, data[0]));
				M_INSWL  (s3, REG_ITMP1, REG_ITMP3, 0);
				M_MSKWL  (REG_ITMP2, REG_ITMP1, REG_ITMP2, 0);
				M_OR     (REG_ITMP2, REG_ITMP3, REG_ITMP2, 0);
				M_LST_U  (REG_ITMP2, REG_ITMP1, 0);
				}
			break;
		case CMD_SASTORE:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			s3 = var_to_reg_int(c->source3, REG_ITMP3);
			if (has_ext_instr_set) {
				M_LADD   (s2, s1, REG_ITMP1, 0);
				M_LADD   (s2, REG_ITMP1, REG_ITMP1, 0);
				M_SST    (s3, REG_ITMP1, OFFSET (java_shortarray, data[0]));
				}
			else {
				M_LADD   (s2, s1, REG_ITMP1, 0);
				M_LADD   (s2, REG_ITMP1, REG_ITMP1, 0);
				M_LLD_U  (REG_ITMP2, REG_ITMP1, OFFSET(java_shortarray, data[0]));
				M_LDA    (REG_ITMP1, REG_ITMP1, OFFSET(java_shortarray, data[0]));
				M_INSWL  (s3, REG_ITMP1, REG_ITMP3, 0);
				M_MSKWL  (REG_ITMP2, REG_ITMP1, REG_ITMP2, 0);
				M_OR     (REG_ITMP2, REG_ITMP3, REG_ITMP2, 0);
				M_LST_U  (REG_ITMP2, REG_ITMP1, 0);
				}
			break;
		case CMD_BASTORE:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			s3 = var_to_reg_int(c->source3, REG_ITMP3);
			if (has_ext_instr_set) {
				M_LADD   (s2, s1, REG_ITMP1, 0);
				M_BST    (s3, REG_ITMP1, OFFSET (java_bytearray, data[0]));
				}
			else {
				M_LADD   (s2, s1, REG_ITMP1,  0);
				M_LLD_U  (REG_ITMP2, REG_ITMP1, OFFSET (java_bytearray, data[0]));
				M_LDA    (REG_ITMP1, REG_ITMP1, OFFSET (java_bytearray, data[0]));
				M_INSBL  (s3, REG_ITMP1, REG_ITMP3,     0);
				M_MSKBL  (REG_ITMP2, REG_ITMP1, REG_ITMP2,  0);
				M_OR     (REG_ITMP2, REG_ITMP3, REG_ITMP2,  0);
				M_LST_U  (REG_ITMP2, REG_ITMP1, 0);
				}
			break;


		case CMD_PUTFIELD:
			switch (c->u.mem.type) {
				case TYPE_INT:
					s1 = var_to_reg_int(c->source1, REG_ITMP1);
					s2 = var_to_reg_int(c->source2, REG_ITMP2);
					M_IST (s2, s1, c->u.mem.offset);
					break;
				case TYPE_LONG:
				case TYPE_ADDRESS:
					s1 = var_to_reg_int(c->source1, REG_ITMP1);
					s2 = var_to_reg_int(c->source2, REG_ITMP2);
					M_LST (s2, s1, c->u.mem.offset);
					break;
				case TYPE_FLOAT:
					s1 = var_to_reg_int(c->source1, REG_ITMP1);
					s2 = var_to_reg_flt(c->source2, REG_FTMP2);
					M_FST (s2, s1, c->u.mem.offset);
					break;
				case TYPE_DOUBLE:
					s1 = var_to_reg_int(c->source1, REG_ITMP1);
					s2 = var_to_reg_flt(c->source2, REG_FTMP2);
					M_DST (s2, s1, c->u.mem.offset);
					break;
				default: panic ("internal error");
				}
			break;

		case CMD_GETFIELD:
			switch (c->u.mem.type) {
				case TYPE_INT:
					s1 = var_to_reg_int(c->source1, REG_ITMP1);
					d = reg_of_var(c->dest, REG_ITMP3);
					M_ILD (d, s1, c->u.mem.offset);
					store_reg_to_var_int(c->dest, d);
					break;
				case TYPE_LONG:
				case TYPE_ADDRESS:
					s1 = var_to_reg_int(c->source1, REG_ITMP1);
					d = reg_of_var(c->dest, REG_ITMP3);
					M_LLD (d, s1, c->u.mem.offset);
					store_reg_to_var_int(c->dest, d);
					break;
				case TYPE_FLOAT:
					s1 = var_to_reg_int(c->source1, REG_ITMP1);
					d = reg_of_var(c->dest, REG_FTMP1);
					M_FLD (d, s1, c->u.mem.offset);
					store_reg_to_var_flt(c->dest, d);
					break;
				case TYPE_DOUBLE:				
					s1 = var_to_reg_int(c->source1, REG_ITMP1);
					d = reg_of_var(c->dest, REG_FTMP1);
					M_DLD (d, s1, c->u.mem.offset);
					store_reg_to_var_flt(c->dest, d);
					break;
				default: panic ("internal error");
				}
			break;


		/********************** branch operations *****************************/

		case CMD_GOTO:
			mcode_addreference (c->u.bra.target);
			M_BR (0);
			break;

		case CMD_JSR: {
			reginfo *di = c->dest->reg;

			if (di->typeflags & REG_INMEMORY)
				panic ("Can not put returnaddress into memory var");

			mcode_addreference (c->u.bra.target);
			M_BSR (di->num, 0);
			}
			break;
			
		case CMD_RET:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			M_RET (REG_ZERO, s1);
			break;

		case CMD_IFEQ:
		case CMD_IFEQL:
		case CMD_IFNULL:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			mcode_addreference (c->u.bra.target);
			M_BEQZ (s1, 0);
			break;
		case CMD_IFLT:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			mcode_addreference (c->u.bra.target);
			M_BLTZ (s1, 0);
			break;
		case CMD_IFLE:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			mcode_addreference (c->u.bra.target);
			M_BLEZ (s1, 0);
			break;
		case CMD_IFNE:
		case CMD_IFNONNULL:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			mcode_addreference (c->u.bra.target);
			M_BNEZ (s1, 0);
			break;
		case CMD_IFGT:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			mcode_addreference (c->u.bra.target);
			M_BGTZ (s1, 0);
			break;
		case CMD_IFGE:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			mcode_addreference (c->u.bra.target);
			M_BGEZ (s1, 0);
			break;

		case CMD_IF_ICMPEQ:
		case CMD_IF_ACMPEQ:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			M_CMPEQ (s1, s2, REG_ITMP1, 0);
			mcode_addreference (c->u.bra.target);
			M_BNEZ (REG_ITMP1, 0);
			break;
		case CMD_IF_ICMPNE:
		case CMD_IF_ACMPNE:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			M_CMPEQ (s1, s2, REG_ITMP1, 0);
			mcode_addreference (c->u.bra.target);
			M_BEQZ (REG_ITMP1, 0);
			break;
		case CMD_IF_ICMPLT:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			M_CMPLT (s1, s2, REG_ITMP1, 0);
			mcode_addreference (c->u.bra.target);
			M_BNEZ (REG_ITMP1, 0);
			break;
		case CMD_IF_ICMPGT:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			M_CMPLE (s1, s2, REG_ITMP1, 0);
			mcode_addreference ( c->u.bra.target );
			M_BEQZ (REG_ITMP1, 0);
			break;
		case CMD_IF_ICMPLE:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			M_CMPLE (s1, s2, REG_ITMP1, 0);
			mcode_addreference (c->u.bra.target);
			M_BNEZ (REG_ITMP1, 0);
			break;
		case CMD_IF_ICMPGE:
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			M_CMPLT (s1, s2, REG_ITMP1,  0);
			mcode_addreference (c->u.bra.target);
			M_BEQZ (REG_ITMP1, 0);
			break;

		case CMD_IF_UCMPGE: /* branch if the unsigned value of s1 is 
		                       greater that s2 (note, that s2 has
		                       to be >= 0) */
			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			M_CMPULE (s2, s1, REG_ITMP1, 0);
			mcode_addreference (c->u.bra.target);
			M_BNEZ (REG_ITMP1, 0);
            break;


		case CMD_IRETURN:
		case CMD_LRETURN:
		case CMD_ARETURN:
			s1 = var_to_reg_int(c->source1, REG_RESULT);
			M_INTMOVE (s1, REG_RESULT);
			goto nowperformreturn;
		case CMD_FRETURN:
		case CMD_DRETURN:
			s1 = var_to_reg_flt(c->source1, REG_FRESULT);
			M_FLTMOVE (s1, REG_FRESULT);
			goto nowperformreturn;

		case CMD_RETURN:
nowperformreturn:
			{
			int r, p;
			
			s2 = var_to_reg_int(c->source2, REG_ITMP2);
			M_INTMOVE (s2, REG_EXCEPTION);

			p = parentargs_base;
			if (!isleafmethod)
				{p--;  M_LLD (REG_RA, REG_SP, 8*p);}
			for (r = INT_SAV_FST; r < INT_SAV_FST + INT_SAV_CNT; r++)
				if (!(intregs[r].typeflags & REG_ISUNUSED))
					{p--; M_LLD (r, REG_SP, 8*p);}
			for (r = FLT_SAV_FST; r < FLT_SAV_FST + FLT_SAV_CNT; r++)
				if (!(floatregs[r].typeflags & REG_ISUNUSED))
					{p--; M_DLD (r, REG_SP, 8*p);}

			if (parentargs_base)
				{M_LDA (REG_SP, REG_SP, parentargs_base*8);}
			if (runverbose) {
				M_LDA (REG_SP, REG_SP, -32);
				M_LST(REG_RA, REG_SP, 0);
				M_LST(REG_RESULT, REG_SP, 8);
				M_DST(REG_FRESULT, REG_SP,16);
				M_LST(REG_EXCEPTION, REG_SP, 24);
				a = dseg_addaddress (method);
				M_LLD(INT_ARG_FST, REG_PV, a);
				M_OR(REG_RESULT, REG_RESULT, INT_ARG_FST + 1, 0);
				M_FLTMOVE(REG_FRESULT, FLT_ARG_FST + 2);
				a = dseg_addaddress ((void*) (builtin_displaymethodstop));
				M_LLD(REG_PV, REG_PV, a);
				M_JSR (REG_RA, REG_PV);
				if (mcodelen<=32768) M_LDA (REG_PV, REG_RA, -mcodelen);
				else {
					s4 ml=-mcodelen, mh=0;
					while (ml<-32768) { ml+=65536; mh--; }
					M_LDA (REG_PV, REG_RA, ml );
					M_LDAH (REG_PV, REG_PV, mh );
					}
				M_LLD(REG_EXCEPTION, REG_SP, 24);
				M_DLD(REG_FRESULT, REG_SP,16);
				M_LLD(REG_RESULT, REG_SP, 8);
				M_LLD(REG_RA, REG_SP, 0);
				M_LDA (REG_SP, REG_SP, 32);
				}
			M_RET (REG_ZERO, REG_RA);
			}
			break;


		case CMD_TABLEJUMP:
			{
			int i;

			/* build jump table top down and use address of lowest entry */

			a = 0;
			for (i = c->u.tablejump.targetcount - 1; i >= 0; i--) {
				a = dseg_addtarget (c->u.tablejump.targets[i]);
				}
			}

			/* last offset a returned by dseg_addtarget is used by load */

			s1 = var_to_reg_int(c->source1, REG_ITMP1);
			M_S8ADDQ (s1, REG_PV, REG_ITMP2, 0);
			M_LLD (REG_ITMP3, REG_ITMP2, a);
			M_JMP (REG_ZERO, REG_ITMP3);
			break;


		case CMD_INVOKESTATIC:
		case CMD_INVOKESPECIAL:
		case CMD_INVOKEVIRTUAL:
		case CMD_INVOKEINTERFACE:
		case CMD_BUILTIN:
			gen_method (c);
			break;

		case CMD_DROP:
		case CMD_ACTIVATE:
			break;

		default: sprintf (logtext, "Unknown pseudo command: %d(%d)", c->opcode,
		                  c->tag);
				 error();
	}
}


/*********************** function input_args_prealloc **************************

	preallocates the input arguments (on the Alpha only for leaf methods)

*******************************************************************************/


static void input_args_prealloc()
{
	int     i, t;
	varid   p;
	reginfo *r;

	if (isleafmethod)
		for (i = 0; (i < mparamnum) && (i < INT_ARG_CNT); i++) {
			t = mparamtypes[i];
			p = mparamvars[i];
			p->saved = !isleafmethod;
			if (t==TYPE_DOUBLE || t==TYPE_FLOAT)
				r = &floatregs[FLT_ARG_FST + i];
			else
				r = &intregs[INT_ARG_FST + i];
			r->typeflags &= ~REG_ISFREEUNUSED;
			p->reg = r;
			}
}


/********************* function gen_computestackframe **************************

	computes the size and the layout of a stack frame.
	The values of localvars_base, savedregs_base, parentargs_base 
    are numbers of 8-byte slots in the stackframe (every spilled or
    saved register value will be stored in a 64-bit slot, regardless
    of its type: INT/LONG/FLOAT/DOUBLE/ADDRESS)
	(a detailed description of the stack frame is contained in 'calling.doc')


*******************************************************************************/


static void gen_computestackframe()
{
	int i;

	savedregs_num = (isleafmethod) ? 0 : 1;   /* space to save the RA */

	/* space to save used callee saved registers */

	for (i = INT_SAV_FST; i < INT_SAV_FST + INT_SAV_CNT; i++)
		if (! (intregs[i].typeflags & REG_ISUNUSED))
			savedregs_num++;
	for (i = FLT_SAV_FST; i < FLT_SAV_FST + FLT_SAV_CNT; i++)
		if (! (floatregs[i].typeflags & REG_ISUNUSED))
			savedregs_num++;

	localvars_base = arguments_num;
	savedregs_base = localvars_base + localvars_num;
	parentargs_base = savedregs_base + savedregs_num;
}


/******************** function gen_header **************************************

	using the data computed by gen_computestackframe it generates a function
	header which:
	
		- saves the necessary registers
		- copies arguments to registers or to stack slots
		
*******************************************************************************/


static void gen_header ()
{
	int p, pa, r;
	reginfo *ri;


	/* create stack frame (if necessary) */

	if (parentargs_base)
		{M_LDA (REG_SP, REG_SP, -parentargs_base * 8);}

	/* save return address and used callee saved registers */

	p = parentargs_base;
	if (!isleafmethod)
		{p--;  M_LST (REG_RA, REG_SP, 8*p);}
	for (r = INT_SAV_FST; r < INT_SAV_FST + INT_SAV_CNT; r++)
		if (!(intregs[r].typeflags & REG_ISUNUSED))
			{p--; M_LST (r, REG_SP, 8 * p);}
	for (r = FLT_SAV_FST; r < FLT_SAV_FST + FLT_SAV_CNT; r++)
		if (!(floatregs[r].typeflags & REG_ISUNUSED))
			{p--; M_DST (r, REG_SP, 8 * p);}

	/* take arguments out of register or stack frame */

 	for (p = 0; p < mparamnum; p++) {
 		ri = mparamvars[p]->reg;

 		if (p < INT_ARG_CNT) {                       /* register arguments    */
 			if (IS_INT_LNG_REG(ri->typeflags)) {     /* integer args          */ 
 				if (!(ri->typeflags & REG_INMEMORY)) /* reg arg -> register   */
 					M_INTMOVE (16+p, ri->num);
 				else                                 /* reg arg -> spilled    */
 					M_LST (16+p, REG_SP, 8 * (localvars_base + ri->num));
 				}
 			else {                                   /* floating args         */   
 				if (!(ri->typeflags & REG_INMEMORY)) /* reg-arg -> register   */
 					M_FLTMOVE (16+p, ri->num);
 				else				                 /* reg-arg -> spilled    */
 					M_DST (16+p, REG_SP, 8 * (localvars_base + ri->num));
 				}
 			}

 		else {                                       /* stack arguments       */
 			pa = p - INT_ARG_CNT;
 			if (IS_INT_LNG_REG(ri->typeflags)) {     /* integer args          */
 				if (!(ri->typeflags & REG_INMEMORY)) /* stack arg -> register */ 
 					M_LLD (ri->num, REG_SP, 8 * (parentargs_base + pa));
 				else {                               /* stack arg -> spilled  */
 					M_LLD (REG_ITMP1, REG_SP, 8 * (parentargs_base + pa));
 					M_LST (REG_ITMP1, REG_SP, 8 * (localvars_base + ri->num));
 					}
 				}
 			else {
 				if (!(ri->typeflags & REG_INMEMORY)) /* stack-arg -> register */
 					M_DLD (ri->num, REG_SP, 8 * (parentargs_base + pa) );
 				else {                               /* stack-arg -> spilled  */
 					M_DLD (REG_FTMP1, REG_SP, 8 * (parentargs_base + pa));
 					M_DST (REG_FTMP1, REG_SP, 8 * (localvars_base + ri->num));
 					}
 				}
 			}
		}  /* end for */
}  /* end gen_header */


/************************ function gen_resolvebranch ***************************

	backpatches a branch instruction; Alpha branch instructions are very
	regular, so it is only necessary to overwrite some fixed bits in the
	instruction.

	parameters: mcodepiece .. start of code area
	            sourcepos ... offset of branch instruction
	            targetpos ... offset of branch target

*******************************************************************************/


static void gen_resolvebranch ( void* mcodepiece, s4 sourcepos, s4 targetpos)
{
	s4 *command = mcodepiece;
	s4 offset = targetpos - (sourcepos+4);

	(*command) |=  ((offset >> 2) & ((s4) 0x1fffff) );
}



/******** redefinition of code generation makros (compilinf into array) *******/

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


#if 0

/************************ function createcompilerstub **************************

	creates a stub routine which calls the compiler
	
*******************************************************************************/

#define COMPSTUBSIZE 3

u1 *createcompilerstub (methodinfo *m)
{
	u8 *s = CNEW (u8, COMPSTUBSIZE);    /* memory to hold the stub            */
	s4 *p = (s4*) s;                    /* code generation pointer            */
	
	                                    /* code for the stub                  */
	M_LLD (REG_PV, REG_PV, 16);         /* load pointer to the compiler       */
	M_JMP (0, REG_PV);                  /* jump to the compiler, return address
	                                       in reg 0 is used as method pointer */
	s[1] = (u8) m;                      /* literals to be adressed            */  
	s[2] = (u8) asm_call_jit_compiler;  /* jump directly via PV from above    */

#ifdef STATISTICS
	count_cstub_len += COMPSTUBSIZE * 8;
#endif

	return (u1*) s;
}


/************************* function removecompilerstub *************************

     deletes a compilerstub from memory  (simply by freeing it)

*******************************************************************************/

void removecompilerstub (u1 *stub) 
{
	CFREE (stub, COMPSTUBSIZE * 8);
}


/************************ function: removenativestub ***************************

    removes a previously created native-stub from memory
    
*******************************************************************************/

void removenativestub (u1 *stub)
{
	CFREE (stub, NATIVESTUBSIZE * 8);
}

#endif /* 0 */


/********************* Funktion: createnativestub ******************************

	creates a stub routine which calls a native method
	
*******************************************************************************/

#define NATIVESTUBSIZE 11

u1 *oldcreatenativestub (functionptr f, methodinfo *m)
{
	u8 *s = CNEW (u8, NATIVESTUBSIZE);  /* memory to hold the stub      */
	s4 *p = (s4*) s;                    /* code generation pointer      */

	M_LDA  (REG_SP, REG_SP, -8);        /* build up stackframe          */
	M_LST  (REG_RA, REG_SP, 0);         /* store return address         */

	M_LLD  (REG_PV, REG_PV, 6*8);       /* load adress of native method */
	M_JSR  (REG_RA, REG_PV);            /* call native method           */
	M_LDA  (REG_PV, REG_RA, -4*4);      /* recompute pv from ra         */

	M_LLD  (22, REG_PV, 7*8);           /* get address of exceptionptr  */
	M_LLD  (REG_EXCEPTION, 22, 0);      /* load exception into reg. 1   */
	M_LST  (REG_ZERO, 22, 0);           /* store NULL into exceptionptr */

	M_LLD  (REG_RA, REG_SP, 0);         /* load return address          */
	M_LDA  (REG_SP, REG_SP, 8);         /* remove stackframe            */
	
	M_RET  (REG_ZERO, REG_RA);          /* return to caller             */

	/* TWISTI */
/*  	s[6] = (u8) f;                      /* address of native method     */
/*  	s[7] = (u8) (&exceptionptr);        /* address of exceptionptr      */
	s[6] = (u4) f;                      /* address of native method     */
	s[7] = (u8) (&exceptionptr);        /* address of exceptionptr      */

#ifdef STATISTICS
	count_nstub_len += NATIVESTUBSIZE * 8;
#endif

	return (u1*) s;
}
