/* alpha/ngen.h ****************************************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Contains the machine dependent code generator definitions and macros for an
	Alpha processor.

	Authors: Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at
	         Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1998/11/11

*******************************************************************************/

/* see also file calling.doc for explanation of calling conventions           */

/* preallocated registers *****************************************************/

/* integer registers */
  
#define REG_RESULT       3   /* to deliver method results                     */ 

//#define REG_RA          26   /* return address                                */
#define REG_PV          13   /* procedure vector, must be provided by caller  */
#define REG_METHODPTR   12   /* pointer to the place from where the procedure */
                             /* vector has been fetched                       */
#define REG_ITMP1       11   /* temporary register                            */
#define REG_ITMP2       12   /* temporary register and method pointer         */
#define REG_ITMP3        0   /* temporary register                            */

#define REG_ITMP1_XPTR  11   /* exception pointer = temporary register 1      */
#define REG_ITMP2_XPC   12   /* exception pc = temporary register 2           */

#define REG_SP           1   /* stack pointer                                 */
#define REG_ZERO         0   /* allways zero                                  */

/* floating point registers */

#define REG_FRESULT      1   /* to deliver floating point method results      */ 
#define REG_FTMP1       16   /* temporary floating point register             */
#define REG_FTMP2       17   /* temporary floating point register             */
#define REG_FTMP3        0   /* temporary floating point register             */

#define REG_IFTMP        0   /* temporary integer and floating point register */

/* register descripton - array ************************************************/

/* #define REG_RES   0         reserved register for OS or code generator     */
/* #define REG_RET   1         return value register                          */
/* #define REG_EXC   2         exception value register (only old jit)        */
/* #define REG_SAV   3         (callee) saved register                        */
/* #define REG_TMP   4         scratch temporary register (caller saved)      */
/* #define REG_ARG   5         argument register (caller saved)               */

/* #define REG_END   -1        last entry in tables */
 
int nregdescint[] = {
	REG_RES, REG_RES, REG_TMP, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, 
	REG_ARG, REG_ARG, REG_ARG, REG_RES, REG_RES, REG_RES, REG_SAV, REG_SAV, 
	REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP,
	REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV,
	REG_END };

//#define INT_SAV_CNT     19   /* number of int callee saved registers          */
#define INT_ARG_CNT      8   /* number of int argument registers              */

/* for use of reserved registers, see comment above */
	
int nregdescfloat[] = {
	REG_RES, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG,
	REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_SAV, REG_SAV, 
	REG_RES, REG_RES, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP,
	REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV,
	REG_END };

//#define FLT_SAV_CNT     18   /* number of flt callee saved registers          */
#define FLT_ARG_CNT     13   /* number of flt argument registers              */

/* for use of reserved registers, see comment above */


/* stackframe-infos ***********************************************************/

int parentargs_base; /* offset in stackframe for the parameter from the caller*/

/* -> see file 'calling.doc' */


/* macros to create code ******************************************************/

#define M_OP3(x,y,oe,rc,d,a,b) \
	*mcodeptr++ = (((x)<<26) | ((d)<<21) | ((a)<<16) | ((b)<<11) | ((oe)<<10) | ((y)<<1) | (rc))

#define M_OP4(x,y,rc,d,a,b,c) \
	*mcodeptr++ = (((x)<<26) | ((d)<<21) | ((a)<<16) | ((b)<<11) | ((c)<<6) | ((y)<<1) | (rc))

#define M_OP2_IMM(x,d,a,i) \
	*mcodeptr++ = (((x)<<26) | ((d)<<21) | ((a)<<16) | ((i)&0xffff))

#define M_BRMASK (((1<<16)-1)&~3)
#define M_BRAMASK (((1<<26)-1)&~3)

#define M_BRA(x,i,a,l) \
	*mcodeptr++ = (((x)<<26) | (((i)*4+4)&M_BRAMASK) | ((a)<<1) | (l));

#define M_BRAC(x,bo,bi,i,a,l) \
	*mcodeptr++ = (((x)<<26) | ((bo)<<21) | ((bi)<<16) | (((i)*4+4)&M_BRMASK) | ((a)<<1) | (l));

#define M_IADD(a,b,c) M_OP3(31, 266, 0, 0, c, a, b)
#define M_IADD_IMM(a,b,c) M_OP2_IMM(14, c, a, b)
#define M_ADDC(a,b,c) M_OP3(31, 10, 0, 0, c, a, b)
#define M_ADDIC(a,b,c) M_OP2_IMM(12, c, a, b)
#define M_ADDICTST(a,b,c) M_OP2_IMM(13, c, a, b)
#define M_ADDE(a,b,c) M_OP3(31, 138, 0, 0, c, a, b)
#define M_ADDZE(a,b) M_OP3(31, 202, 0, 0, b, a, 0)
#define M_ADDME(a,b) M_OP3(31, 234, 0, 0, b, a, 0)
#define M_ISUB(a,b,c) M_OP3(31, 40, 0, 0, c, b, a)
#define M_ISUBTST(a,b,c) M_OP3(31, 40, 0, 1, c, b, a)
#define M_SUBC(a,b,c) M_OP3(31, 8, 0, 0, c, b, a)
#define M_SUBIC(a,b,c) M_OP2_IMM(8, c, b, a)
#define M_SUBE(a,b,c) M_OP3(31, 136, 0, 0, c, b, a)
#define M_SUBZE(a,b) M_OP3(31, 200, 0, 0, b, a, 0)
#define M_SUBME(a,b) M_OP3(31, 232, 0, 0, b, a, 0)
#define M_AND(a,b,c) M_OP3(31, 28, 0, 0, a, c, b)
#define M_AND_IMM(a,b,c) M_OP2_IMM(28, a, c, b)
#define M_ANDIS(a,b,c) M_OP2_IMM(29, a, c, b)
#define M_OR(a,b,c) M_OP3(31, 444, 0, 0, a, c, b)
#define M_OR_IMM(a,b,c) M_OP2_IMM(24, a, c, b)
#define M_ORIS(a,b,c) M_OP2_IMM(25, a, c, b)
#define M_XOR(a,b,c) M_OP3(31, 316, 0, 0, a, c, b)
#define M_XOR_IMM(a,b,c) M_OP2_IMM(26, a, c, b)
#define M_XORIS(a,b,c) M_OP2_IMM(27, a, c, b)
#define M_SLL(a,b,c) M_OP3(31, 24, 0, 0, a, c, b)
#define M_SRL(a,b,c) M_OP3(31, 536, 0, 0, a, c, b)
#define M_SRA(a,b,c) M_OP3(31, 792, 0, 0, a, c, b)
#define M_SRA_IMM(a,b,c) M_OP3(31, 824, 0, 0, a, c, b)
#define M_IMUL(a,b,c) M_OP3(31, 235, 0, 0, c, a, b)
#define M_IMUL_IMM(a,b,c) M_OP2_IMM(7, c, a, b)
#define M_IDIV(a,b,c) M_OP3(31, 491, 0, 0, c, a, b)
#define M_NEG(a,b) M_OP3(31, 104, 0, 0, b, a, 0)
#define M_NOT(a,b) M_OP3(31, 124, 0, 0, a, b, a)

#define M_SUBFIC(a,b,c) M_OP2_IMM(8, c, a, b)
#define M_SUBFZE(a,b) M_OP3(31, 200, 0, 0, b, a, 0)
#define M_RLWINM(a,b,c,d,e) M_OP4(21, d, 0, a, e, b, c)
#define M_ADDZE(a,b) M_OP3(31, 202, 0, 0, b, a, 0)
#define M_SLL_IMM(a,b,c) M_RLWINM(a,b,0,31-(b),c)
#define M_SRL_IMM(a,b,c) M_RLWINM(a,32-(b),b,31,c)
#define M_ADDIS(a,b,c) M_OP2_IMM(15, c, a, b)
#define M_STFIWX(a,b,c) M_OP3(31, 983, 0, 0, a, b, c)
#define M_LWZX(a,b,c) M_OP3(31, 23, 0, 0, a, b, c)
#define M_LHZX(a,b,c) M_OP3(31, 279, 0, 0, a, b, c)
#define M_LHAX(a,b,c) M_OP3(31, 343, 0, 0, a, b, c)
#define M_LBZX(a,b,c) M_OP3(31, 87, 0, 0, a, b, c)
#define M_LFSX(a,b,c) M_OP3(31, 535, 0, 0, a, b, c)
#define M_LFDX(a,b,c) M_OP3(31, 599, 0, 0, a, b, c)
#define M_STWX(a,b,c) M_OP3(31, 151, 0, 0, a, b, c)
#define M_STHX(a,b,c) M_OP3(31, 407, 0, 0, a, b, c)
#define M_STBX(a,b,c) M_OP3(31, 215, 0, 0, a, b, c)
#define M_STFSX(a,b,c) M_OP3(31, 663, 0, 0, a, b, c)
#define M_STFDX(a,b,c) M_OP3(31, 727, 0, 0, a, b, c)
#define M_STWU(a,b,c) M_OP2_IMM(37, a, b, c)
#define M_LDAH(a,b,c) M_ADDIS(b, c, a)
#define M_TRAP M_OP3(31, 4, 0, 0, 31, 0, 0)

#define M_NOP M_OR_IMM(0, 0, 0)
#define M_MOV(a,b) M_OR(a, a, b)
#define M_TST(a) M_OP3(31, 444, 0, 1, a, a, a)

#define M_DADD(a,b,c) M_OP3(63, 21, 0, 0, c, a, b)
#define M_FADD(a,b,c) M_OP3(59, 21, 0, 0, c, a, b)
#define M_DSUB(a,b,c) M_OP3(63, 20, 0, 0, c, a, b)
#define M_FSUB(a,b,c) M_OP3(59, 20, 0, 0, c, a, b)
#define M_DMUL(a,b,c) M_OP4(63, 25, 0, c, a, 0, b)
#define M_FMUL(a,b,c) M_OP4(59, 25, 0, c, a, 0, b)
#define M_DDIV(a,b,c) M_OP3(63, 18, 0, 0, c, a, b)
#define M_FDIV(a,b,c) M_OP3(59, 18, 0, 0, c, a, b)

#define M_FABS(a,b) M_OP3(63, 264, 0, 0, b, 0, a)
#define M_CVTDL(a,b) M_OP3(63, 14, 0, 0, b, 0, a)
#define M_CVTDL_C(a,b) M_OP3(63, 15, 0, 0, b, 0, a)
#define M_CVTDF(a,b) M_OP3(63, 12, 0, 0, b, 0, a)
#define M_FMOV(a,b) M_OP3(63, 72, 0, 0, b, 0, a)
#define M_FMOVN(a,b) M_OP3(63, 40, 0, 0, b, 0, a)
#define M_DSQRT(a,b) M_OP3(63, 22, 0, 0, b, 0, a)
#define M_FSQRT(a,b) M_OP3(59, 22, 0, 0, b, 0, a)

#define M_FCMPU(a,b) M_OP3(63, 0, 0, 0, 0, a, b)
#define M_FCMPO(a,b) M_OP3(63, 32, 0, 0, 0, a, b)

#define M_BST(a,b,c) M_OP2_IMM(38, a, b, c)
#define M_SST(a,b,c) M_OP2_IMM(44, a, b, c)
#define M_IST(a,b,c) M_OP2_IMM(36, a, b, c)
#define M_AST(a,b,c) M_OP2_IMM(36, a, b, c)
#define M_BLDU(a,b,c) M_OP2_IMM(34, a, b, c)
#define M_SLDU(a,b,c) M_OP2_IMM(40, a, b, c)
#define M_ILD(a,b,c) M_OP2_IMM(32, a, b, c)
#define M_ALD(a,b,c) M_OP2_IMM(32, a, b, c)

#define M_BSEXT(a,b) M_OP3(31, 954, 0, 0, a, b, 0)
#define M_SSEXT(a,b) M_OP3(31, 922, 0, 0, a, b, 0)
#define M_CZEXT(a,b) M_RLWINM(a,0,16,31,b)

#define M_BR(a) M_BRA(18, a, 0, 0);
#define M_BL(a) M_BRA(18, a, 0, 1);
#define M_RET M_OP3(19, 16, 0, 0, 20, 0, 0);
#define M_JSR M_OP3(19, 528, 0, 1, 20, 0, 0);
#define M_RTS M_OP3(19, 528, 0, 0, 20, 0, 0);

#define M_CMP(a,b) M_OP3(31, 0, 0, 0, 0, a, b);
#define M_CMPU(a,b) M_OP3(31, 32, 0, 0, 0, a, b);
#define M_CMPI(a,b) M_OP2_IMM(11, 0, a, b);
#define M_CMPUI(a,b) M_OP2_IMM(10, 0, a, b);

#define M_BLT(a) M_BRAC(16, 12, 0, a, 0, 0);
#define M_BLE(a) M_BRAC(16, 4, 1, a, 0, 0);
#define M_BGT(a) M_BRAC(16, 12, 1, a, 0, 0);
#define M_BGE(a) M_BRAC(16, 4, 0, a, 0, 0);
#define M_BEQ(a) M_BRAC(16, 12, 2, a, 0, 0);
#define M_BNE(a) M_BRAC(16, 4, 2, a, 0, 0);
#define M_BNAN(a) M_BRAC(16, 12, 3, a, 0, 0);

#define M_DLD(a,b,c) M_OP2_IMM(50, a, b, c)
#define M_DST(a,b,c) M_OP2_IMM(54, a, b, c)
#define M_FLD(a,b,c) M_OP2_IMM(48, a, b, c)
#define M_FST(a,b,c) M_OP2_IMM(52, a, b, c)

#define M_MFLR(a) M_OP3(31, 339, 0, 0, a, 8, 0)
#define M_MFXER(a) M_OP3(31, 339, 0, 0, a, 1, 0)
#define M_MFCTR(a) M_OP3(31, 339, 0, 0, a, 9, 0)
#define M_MTLR(a) M_OP3(31, 467, 0, 0, a, 8, 0)
#define M_MTXER(a) M_OP3(31, 467, 0, 0, a, 1, 0)
#define M_MTCTR(a) M_OP3(31, 467, 0, 0, a, 9, 0)

#define M_LDA(a,b,c) M_IADD_IMM(b, c, a)
#define M_LDATST(a,b,c) M_ADDICTST(b, c, a)
#define M_CLR(a) M_IADD_IMM(0, 0, a)

/* function gen_resolvebranch **************************************************

	parameters: ip ... pointer to instruction after branch (void*)
	            so ... offset of instruction after branch  (s4)
	            to ... offset of branch target             (s4)

*******************************************************************************/

#define gen_resolvebranch(ip,so,to) \
	*((s4*)(ip)-1)=(*((s4*)(ip)-1) & ~M_BRMASK) | (((s4)((to)-(so))+4)&((((*((s4*)(ip)-1)>>26)&63)==18)?M_BRAMASK:M_BRMASK))

#define SOFTNULLPTRCHECK       /* soft null pointer check supported as option */
