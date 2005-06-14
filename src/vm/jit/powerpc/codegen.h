/* src/vm/jit/powerpc/codegen.h - code generation macros and definitions for
                              32-bit PowerPC

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
            Stefan Ring

   Changes: Christian Thalinger

   $Id: codegen.h 2693 2005-06-14 18:34:47Z twisti $

*/


#ifndef _CODEGEN_H
#define _CODEGEN_H

#include "md-abi.h"

#include "vm/global.h"
#include "vm/jit/reg.h"


/* additional functions and macros to generate code ***************************/

#if defined(STATISTICS)
#define COUNT_SPILLS count_spills++
#else
#define COUNT_SPILLS
#endif


/* gen_nullptr_check(objreg) */

#define gen_nullptr_check(objreg) \
    if (checknull) { \
        M_TST((objreg)); \
        M_BEQ(0); \
        codegen_addxnullrefs(cd, mcodeptr); \
    }

#define gen_bound_check \
    if (checkbounds) { \
        M_ILD(REG_ITMP3, s1, OFFSET(java_arrayheader, size));\
        M_CMPU(s2, REG_ITMP3);\
        M_BGE(0);\
        codegen_addxboundrefs(cd, mcodeptr, s2); \
    }


/* MCODECHECK(icnt) */

#define MCODECHECK(icnt) \
	if ((mcodeptr + (icnt)) > cd->mcodeend) \
        mcodeptr = codegen_increase(cd, (u1 *) mcodeptr)


/* M_INTMOVE:
     generates an integer-move from register a to b.
     if a and b are the same int-register, no code will be generated.
*/ 

#define M_INTMOVE(a,b) if ((a) != (b)) { M_MOV(a, b); }

#define M_TINTMOVE(t,a,b) \
	if ((t) == TYPE_LNG) { \
		if ((a) <= (b)) \
            M_INTMOVE(rd->secondregs[(a)], rd->secondregs[(b)]); \
		M_INTMOVE((a), (b)); \
        if ((a) > (b)) \
            M_INTMOVE(rd->secondregs[(a)], rd->secondregs[(b)]); \
	} else { \
		M_INTMOVE((a), (b)); \
    }


/* M_FLTMOVE:
    generates a floating-point-move from register a to b.
    if a and b are the same float-register, no code will be generated
*/ 

#define M_FLTMOVE(a,b) if ((a) != (b)) { M_FMOV(a, b); }


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
	if ((v)->flags & INMEMORY) { \
		COUNT_SPILLS; \
        if ((a)) M_ILD((tempnr), REG_SP, 4 * (v)->regoff); \
		regnr = tempnr; \
		if ((b) && IS_2_WORD_TYPE((v)->type)) \
			M_ILD((a) ? rd->secondregs[(tempnr)] : (tempnr), REG_SP, 4 * (v)->regoff + 4); \
    } else \
        regnr = (!(a) && (b)) ? rd->secondregs[(v)->regoff] : (v)->regoff; \
}
#define var_to_reg_int(regnr,v,tempnr) var_to_reg_int0(regnr,v,tempnr,1,1)


#define var_to_reg_flt(regnr,v,tempnr) { \
	if ((v)->flags & INMEMORY) { \
		COUNT_SPILLS; \
		if ((v)->type==TYPE_DBL) \
			M_DLD(tempnr,REG_SP,4*(v)->regoff); \
		else \
			M_FLD(tempnr,REG_SP,4*(v)->regoff); \
		regnr=tempnr; \
	} else regnr=(v)->regoff; \
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
			M_IST(rd->secondregs[tempregnum], REG_SP, 4 * (sptr)->regoff + 4); \
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


#define ICONST(reg,c) \
    if (((c) >= 0 && (c) <= 32767) || ((c) >= -32768 && (c) < 0)) {\
        M_LDA((reg), REG_ZERO, (c)); \
    } else { \
        a = dseg_adds4(cd, c); \
        M_ILD((reg), REG_PV, a); \
    }

#define LCONST(reg,c) \
    ICONST((reg), (s4) ((s8) (c) >> 32)); \
    ICONST(rd->secondregs[(reg)], (s4) ((s8) (c)));


#define M_COPY(from,to) \
			d = reg_of_var(rd, to, REG_IFTMP); \
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


#define ALIGNCODENOP \
    if ((s4) ((ptrint) mcodeptr & 7)) { \
        M_NOP; \
    }


/* macros to create code ******************************************************/

#define M_OP3(x,y,oe,rc,d,a,b) \
	*(mcodeptr++) = (((x) << 26) | ((d) << 21) | ((a) << 16) | ((b) << 11) | ((oe) << 10) | ((y) << 1) | (rc))

#define M_OP4(x,y,rc,d,a,b,c) \
	*(mcodeptr++) = (((x) << 26) | ((d) << 21) | ((a) << 16) | ((b) << 11) | ((c) << 6) | ((y) << 1) | (rc))

#define M_OP2_IMM(x,d,a,i) \
	*(mcodeptr++) = (((x) << 26) | ((d) << 21) | ((a) << 16) | ((i) & 0xffff))

#define M_BRMASK     0x0000fffc                     /* (((1 << 16) - 1) & ~3) */
#define M_BRAMASK    0x03fffffc                     /* (((1 << 26) - 1) & ~3) */

#define M_BRA(x,i,a,l) \
	*(mcodeptr++) = (((x) << 26) | ((((i) * 4) + 4) & M_BRAMASK) | ((a) << 1) | (l))

#define M_BRAC(x,bo,bi,i,a,l) \
	*(mcodeptr++) = (((x) << 26) | ((bo) << 21) | ((bi) << 16) | (((i) * 4 + 4) & M_BRMASK) | ((a) << 1) | (l))


/* instruction macros *********************************************************/

#define M_IADD(a,b,c)                   M_OP3(31, 266, 0, 0, c, a, b)
#define M_IADD_IMM(a,b,c)               M_OP2_IMM(14, c, a, b)
#define M_ADDC(a,b,c)                   M_OP3(31, 10, 0, 0, c, a, b)
#define M_ADDIC(a,b,c)                  M_OP2_IMM(12, c, a, b)
#define M_ADDICTST(a,b,c)               M_OP2_IMM(13, c, a, b)
#define M_ADDE(a,b,c)                   M_OP3(31, 138, 0, 0, c, a, b)
#define M_ADDZE(a,b)                    M_OP3(31, 202, 0, 0, b, a, 0)
#define M_ADDME(a,b)                    M_OP3(31, 234, 0, 0, b, a, 0)
#define M_ISUB(a,b,c)                   M_OP3(31, 40, 0, 0, c, b, a)
#define M_ISUBTST(a,b,c)                M_OP3(31, 40, 0, 1, c, b, a)
#define M_SUBC(a,b,c)                   M_OP3(31, 8, 0, 0, c, b, a)
#define M_SUBIC(a,b,c)                  M_OP2_IMM(8, c, b, a)
#define M_SUBE(a,b,c)                   M_OP3(31, 136, 0, 0, c, b, a)
#define M_SUBZE(a,b)                    M_OP3(31, 200, 0, 0, b, a, 0)
#define M_SUBME(a,b)                    M_OP3(31, 232, 0, 0, b, a, 0)

#define M_AND(a,b,c)                    M_OP3(31, 28, 0, 0, a, c, b)
#define M_AND_IMM(a,b,c)                M_OP2_IMM(28, a, c, b)
#define M_ANDIS(a,b,c)                  M_OP2_IMM(29, a, c, b)
#define M_OR(a,b,c)                     M_OP3(31, 444, 0, 0, a, c, b)
#define M_OR_IMM(a,b,c)                 M_OP2_IMM(24, a, c, b)
#define M_ORIS(a,b,c)                   M_OP2_IMM(25, a, c, b)
#define M_XOR(a,b,c)                    M_OP3(31, 316, 0, 0, a, c, b)
#define M_XOR_IMM(a,b,c)                M_OP2_IMM(26, a, c, b)
#define M_XORIS(a,b,c)                  M_OP2_IMM(27, a, c, b)

#define M_SLL(a,b,c)                    M_OP3(31, 24, 0, 0, a, c, b)
#define M_SRL(a,b,c)                    M_OP3(31, 536, 0, 0, a, c, b)
#define M_SRA(a,b,c)                    M_OP3(31, 792, 0, 0, a, c, b)
#define M_SRA_IMM(a,b,c)                M_OP3(31, 824, 0, 0, a, c, b)

#define M_IMUL(a,b,c)                   M_OP3(31, 235, 0, 0, c, a, b)
#define M_IMUL_IMM(a,b,c)               M_OP2_IMM(7, c, a, b)
#define M_IDIV(a,b,c)                   M_OP3(31, 491, 0, 0, c, a, b)

#define M_NEG(a,b)                      M_OP3(31, 104, 0, 0, b, a, 0)
#define M_NOT(a,b)                      M_OP3(31, 124, 0, 0, a, b, a)

#define M_SUBFIC(a,b,c)                 M_OP2_IMM(8, c, a, b)
#define M_SUBFZE(a,b)                   M_OP3(31, 200, 0, 0, b, a, 0)
#define M_RLWINM(a,b,c,d,e)             M_OP4(21, d, 0, a, e, b, c)
#define M_ADDZE(a,b)                    M_OP3(31, 202, 0, 0, b, a, 0)
#define M_SLL_IMM(a,b,c)                M_RLWINM(a,b,0,31-(b),c)
#define M_SRL_IMM(a,b,c)                M_RLWINM(a,32-(b),b,31,c)
#define M_ADDIS(a,b,c)                  M_OP2_IMM(15, c, a, b)
#define M_STFIWX(a,b,c)                 M_OP3(31, 983, 0, 0, a, b, c)
#define M_LWZX(a,b,c)                   M_OP3(31, 23, 0, 0, a, b, c)
#define M_LHZX(a,b,c)                   M_OP3(31, 279, 0, 0, a, b, c)
#define M_LHAX(a,b,c)                   M_OP3(31, 343, 0, 0, a, b, c)
#define M_LBZX(a,b,c)                   M_OP3(31, 87, 0, 0, a, b, c)
#define M_LFSX(a,b,c)                   M_OP3(31, 535, 0, 0, a, b, c)
#define M_LFDX(a,b,c)                   M_OP3(31, 599, 0, 0, a, b, c)
#define M_STWX(a,b,c)                   M_OP3(31, 151, 0, 0, a, b, c)
#define M_STHX(a,b,c)                   M_OP3(31, 407, 0, 0, a, b, c)
#define M_STBX(a,b,c)                   M_OP3(31, 215, 0, 0, a, b, c)
#define M_STFSX(a,b,c)                  M_OP3(31, 663, 0, 0, a, b, c)
#define M_STFDX(a,b,c)                  M_OP3(31, 727, 0, 0, a, b, c)
#define M_STWU(a,b,c)                   M_OP2_IMM(37, a, b, c)
#define M_LDAH(a,b,c)                   M_ADDIS(b, c, a)
#define M_TRAP                          M_OP3(31, 4, 0, 0, 31, 0, 0)

#define M_NOP                           M_OR_IMM(0, 0, 0)
#define M_MOV(a,b)                      M_OR(a, a, b)
#define M_TST(a)                        M_OP3(31, 444, 0, 1, a, a, a)

#define M_DADD(a,b,c)                   M_OP3(63, 21, 0, 0, c, a, b)
#define M_FADD(a,b,c)                   M_OP3(59, 21, 0, 0, c, a, b)
#define M_DSUB(a,b,c)                   M_OP3(63, 20, 0, 0, c, a, b)
#define M_FSUB(a,b,c)                   M_OP3(59, 20, 0, 0, c, a, b)
#define M_DMUL(a,b,c)                   M_OP4(63, 25, 0, c, a, 0, b)
#define M_FMUL(a,b,c)                   M_OP4(59, 25, 0, c, a, 0, b)
#define M_DDIV(a,b,c)                   M_OP3(63, 18, 0, 0, c, a, b)
#define M_FDIV(a,b,c)                   M_OP3(59, 18, 0, 0, c, a, b)

#define M_FABS(a,b)                     M_OP3(63, 264, 0, 0, b, 0, a)
#define M_CVTDL(a,b)                    M_OP3(63, 14, 0, 0, b, 0, a)
#define M_CVTDL_C(a,b)                  M_OP3(63, 15, 0, 0, b, 0, a)
#define M_CVTDF(a,b)                    M_OP3(63, 12, 0, 0, b, 0, a)
#define M_FMOV(a,b)                     M_OP3(63, 72, 0, 0, b, 0, a)
#define M_FMOVN(a,b)                    M_OP3(63, 40, 0, 0, b, 0, a)
#define M_DSQRT(a,b)                    M_OP3(63, 22, 0, 0, b, 0, a)
#define M_FSQRT(a,b)                    M_OP3(59, 22, 0, 0, b, 0, a)

#define M_FCMPU(a,b)                    M_OP3(63, 0, 0, 0, 0, a, b)
#define M_FCMPO(a,b)                    M_OP3(63, 32, 0, 0, 0, a, b)

#define M_BST(a,b,c)                    M_OP2_IMM(38, a, b, c)
#define M_SST(a,b,c)                    M_OP2_IMM(44, a, b, c)
#define M_IST(a,b,c)                    M_OP2_IMM(36, a, b, c)
#define M_AST(a,b,c)                    M_OP2_IMM(36, a, b, c)
#define M_BLDU(a,b,c)                   M_OP2_IMM(34, a, b, c)
#define M_SLDU(a,b,c)                   M_OP2_IMM(40, a, b, c)
#define M_ILD(a,b,c)                    M_OP2_IMM(32, a, b, c)
#define M_ALD(a,b,c)                    M_OP2_IMM(32, a, b, c)

#define M_BSEXT(a,b)                    M_OP3(31, 954, 0, 0, a, b, 0)
#define M_SSEXT(a,b)                    M_OP3(31, 922, 0, 0, a, b, 0)
#define M_CZEXT(a,b)                    M_RLWINM(a,0,16,31,b)

#define M_BR(a)                         M_BRA(18, a, 0, 0);
#define M_BL(a)                         M_BRA(18, a, 0, 1);
#define M_RET                           M_OP3(19, 16, 0, 0, 20, 0, 0);
#define M_JSR                           M_OP3(19, 528, 0, 1, 20, 0, 0);
#define M_RTS                           M_OP3(19, 528, 0, 0, 20, 0, 0);

#define M_CMP(a,b)                      M_OP3(31, 0, 0, 0, 0, a, b);
#define M_CMPU(a,b)                     M_OP3(31, 32, 0, 0, 0, a, b);
#define M_CMPI(a,b)                     M_OP2_IMM(11, 0, a, b);
#define M_CMPUI(a,b)                    M_OP2_IMM(10, 0, a, b);

#define M_BLT(a)                        M_BRAC(16, 12, 0, a, 0, 0);
#define M_BLE(a)                        M_BRAC(16, 4, 1, a, 0, 0);
#define M_BGT(a)                        M_BRAC(16, 12, 1, a, 0, 0);
#define M_BGE(a)                        M_BRAC(16, 4, 0, a, 0, 0);
#define M_BEQ(a)                        M_BRAC(16, 12, 2, a, 0, 0);
#define M_BNE(a)                        M_BRAC(16, 4, 2, a, 0, 0);
#define M_BNAN(a)                       M_BRAC(16, 12, 3, a, 0, 0);

#define M_DLD(a,b,c)                    M_OP2_IMM(50, a, b, c)
#define M_DST(a,b,c)                    M_OP2_IMM(54, a, b, c)
#define M_FLD(a,b,c)                    M_OP2_IMM(48, a, b, c)
#define M_FST(a,b,c)                    M_OP2_IMM(52, a, b, c)

#define M_MFLR(a)                       M_OP3(31, 339, 0, 0, a, 8, 0)
#define M_MFXER(a)                      M_OP3(31, 339, 0, 0, a, 1, 0)
#define M_MFCTR(a)                      M_OP3(31, 339, 0, 0, a, 9, 0)
#define M_MTLR(a)                       M_OP3(31, 467, 0, 0, a, 8, 0)
#define M_MTXER(a)                      M_OP3(31, 467, 0, 0, a, 1, 0)
#define M_MTCTR(a)                      M_OP3(31, 467, 0, 0, a, 9, 0)

#define M_LDA(a,b,c)                    M_IADD_IMM(b, c, a)
#define M_LDATST(a,b,c)                 M_ADDICTST(b, c, a)
#define M_CLR(a)                        M_IADD_IMM(0, 0, a)
#define M_AADD_IMM(a,b,c)               M_IADD_IMM(a, b, c)


/* function gen_resolvebranch **************************************************

	parameters: ip ... pointer to instruction after branch (void*)
	            so ... offset of instruction after branch  (s4)
	            to ... offset of branch target             (s4)

*******************************************************************************/

#define gen_resolvebranch(ip,so,to) \
	*((s4*)(ip)-1)=(*((s4*)(ip)-1) & ~M_BRMASK) | (((s4)((to)-(so))+4)&((((*((s4*)(ip)-1)>>26)&63)==18)?M_BRAMASK:M_BRMASK))


/* function prototypes */

void preregpass(methodinfo *m, registerdata *rd);
void docacheflush(u1 *p, long bytelen);

#endif /* _CODEGEN_H */


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
