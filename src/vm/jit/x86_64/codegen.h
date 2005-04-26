/* src/vm/jit/x86_64/codegen.h - code generation macros for x86_64

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
            Christian Thalinger


   $Id: codegen.h 2393 2005-04-26 19:50:58Z twisti $

*/

#ifndef _CODEGEN_H
#define _CODEGEN_H

#include <ucontext.h>

#include "vm/jit/x86_64/types.h"

/* SET_ARG_STACKSLOTS *************************************************
Macro for stack.c to set Argument Stackslots

Sets the first call_argcount stackslots of curstack to varkind ARGVAR, if
they to not have the SAVEDVAR flag set. According to the calling
conventions these stackslots are assigned argument registers or memory
locations

-- in
i,call_argcount:  Number of Parameters
curstack:         stackptr to current instack
copy    :         stackptr

--- uses
i, copy

-- out
copy: points to first stackslot after the parameters
rd->ifmemuse: max. number of stackslots needed for spilled parameters so far
rd->argintreguse: max. number of used integer argument registers used so far
rd->fltintreguse: max. number of used float argument registers used so far          
************************************************************************/
#define SET_ARG_STACKSLOTS {											\
		s4 iarg = 0;													\
		s4 farg = 0;													\
		s4 stacksize;     /* Stackoffset for spilled arg */				\
		copy = curstack;												\
		while (--i >= 0) {												\
			(IS_FLT_DBL_TYPE(copy->type)) ? farg++ : iarg++;			\
			copy = copy->prev;											\
		}																\
		stacksize  = (farg < rd->fltreg_argnum)? 0 : (farg - rd->fltreg_argnum); \
		stacksize += (iarg < rd->intreg_argnum)? 0 : (iarg - rd->intreg_argnum); \
		if (rd->argintreguse < iarg)									\
			rd->argintreguse = iarg;									\
		if (rd->argfltreguse < farg)									\
			rd->argfltreguse = farg;									\
		if (rd->ifmemuse < stacksize)									\
			rd->ifmemuse = stacksize;									\
		i = call_argcount;												\
		copy = curstack;												\
		while (--i >= 0) {												\
			if (IS_FLT_DBL_TYPE(copy->type)) {							\
				farg--;													\
				if (!(copy->flags & SAVEDVAR)) {						\
					copy->varnum = farg;								\
					copy->varkind = ARGVAR;								\
					if (farg < rd->fltreg_argnum) {						\
						copy->flags = 0;								\
						copy->regoff = rd->argfltregs[farg];			\
					} else {											\
						copy->flags = INMEMORY;							\
						copy->regoff = --stacksize;						\
					}													\
				}														\
			} else { /* int_arg */										\
				iarg--;													\
				if (!(copy->flags & SAVEDVAR)) {						\
					copy->varnum = iarg;								\
					copy->varkind = ARGVAR;								\
					if (iarg < rd->intreg_argnum) {						\
						copy->flags = 0;								\
						copy->regoff = rd->argintregs[iarg];			\
					} else {											\
						copy->flags = INMEMORY;							\
						copy->regoff = --stacksize;						\
					}													\
				}														\
			}															\
			copy = copy->prev;											\
		}																\
	}																   


/* macros to create code ******************************************************/

/* immediate data union */

typedef union {
    s4 i;
    s8 l;
    float f;
    double d;
    void *a;
    u1 b[8];
} x86_64_imm_buf;


/* opcodes for alu instructions */

typedef enum {
    X86_64_ADD = 0,
    X86_64_OR  = 1,
    X86_64_ADC = 2,
    X86_64_SBB = 3,
    X86_64_AND = 4,
    X86_64_SUB = 5,
    X86_64_XOR = 6,
    X86_64_CMP = 7,
    X86_64_NALU
} X86_64_ALU_Opcode;


typedef enum {
    X86_64_ROL = 0,
    X86_64_ROR = 1,
    X86_64_RCL = 2,
    X86_64_RCR = 3,
    X86_64_SHL = 4,
    X86_64_SHR = 5,
    X86_64_SAR = 7,
    X86_64_NSHIFT = 8
} X86_64_Shift_Opcode;


typedef enum {
    X86_64_CC_O = 0,
    X86_64_CC_NO = 1,
    X86_64_CC_B = 2, X86_64_CC_C = 2, X86_64_CC_NAE = 2,
    X86_64_CC_BE = 6, X86_64_CC_NA = 6,
    X86_64_CC_AE = 3, X86_64_CC_NB = 3, X86_64_CC_NC = 3,
    X86_64_CC_E = 4, X86_64_CC_Z = 4,
    X86_64_CC_NE = 5, X86_64_CC_NZ = 5,
    X86_64_CC_A = 7, X86_64_CC_NBE = 7,
    X86_64_CC_S = 8, X86_64_CC_LZ = 8,
    X86_64_CC_NS = 9, X86_64_CC_GEZ = 9,
    X86_64_CC_P = 0x0a, X86_64_CC_PE = 0x0a,
    X86_64_CC_NP = 0x0b, X86_64_CC_PO = 0x0b,
    X86_64_CC_L = 0x0c, X86_64_CC_NGE = 0x0c,
    X86_64_CC_GE = 0x0d, X86_64_CC_NL = 0x0d,
    X86_64_CC_LE = 0x0e, X86_64_CC_NG = 0x0e,
    X86_64_CC_G = 0x0f, X86_64_CC_NLE = 0x0f,
    X86_64_NCC
} X86_64_CC;


#define IS_IMM8(imm) \
    (((long) (imm) >= -128) && ((long) (imm) <= 127))


#define IS_IMM32(imm) \
    (((long) (imm) >= (-2147483647-1)) && ((long) (imm) <= 2147483647))


/* modrm and stuff */

#define x86_64_address_byte(mod,reg,rm) \
    *(cd->mcodeptr++) = ((((mod) & 0x03) << 6) | (((reg) & 0x07) << 3) | ((rm) & 0x07));


#define x86_64_emit_reg(reg,rm) \
    x86_64_address_byte(3,(reg),(rm));


#define x86_64_emit_rex(size,reg,index,rm) \
    if ((size) == 1 || (reg) > 7 || (index) > 7 || (rm) > 7) { \
        *(cd->mcodeptr++) = (0x40 | (((size) & 0x01) << 3) | ((((reg) >> 3) & 0x01) << 2) | ((((index) >> 3) & 0x01) << 1) | (((rm) >> 3) & 0x01)); \
    }


#define x86_64_emit_byte_rex(reg,index,rm) \
    *(cd->mcodeptr++) = (0x40 | ((((reg) >> 3) & 0x01) << 2) | ((((index) >> 3) & 0x01) << 1) | (((rm) >> 3) & 0x01));


#define x86_64_emit_mem(r,disp) \
    do { \
        x86_64_address_byte(0,(r),5); \
        x86_64_emit_imm32((disp)); \
    } while (0)


#define x86_64_emit_membase(basereg,disp,dreg) \
    do { \
        if ((basereg) == REG_SP || (basereg) == R12) { \
            if ((disp) == 0) { \
                x86_64_address_byte(0,(dreg),REG_SP); \
                x86_64_address_byte(0,REG_SP,REG_SP); \
            } else if (IS_IMM8((disp))) { \
                x86_64_address_byte(1,(dreg),REG_SP); \
                x86_64_address_byte(0,REG_SP,REG_SP); \
                x86_64_emit_imm8((disp)); \
            } else { \
                x86_64_address_byte(2,(dreg),REG_SP); \
                x86_64_address_byte(0,REG_SP,REG_SP); \
                x86_64_emit_imm32((disp)); \
            } \
            break; \
        } \
        if ((disp) == 0 && (basereg) != RBP && (basereg) != R13) { \
            x86_64_address_byte(0,(dreg),(basereg)); \
            break; \
        } \
        \
        if ((basereg) == RIP) { \
            x86_64_address_byte(0,(dreg),RBP); \
            x86_64_emit_imm32((disp)); \
            break; \
        } \
        \
        if (IS_IMM8((disp))) { \
            x86_64_address_byte(1,(dreg),(basereg)); \
            x86_64_emit_imm8((disp)); \
        } else { \
            x86_64_address_byte(2,(dreg),(basereg)); \
            x86_64_emit_imm32((disp)); \
        } \
    } while (0)


#define x86_64_emit_membase32(basereg,disp,dreg) \
    do { \
        if ((basereg) == REG_SP || (basereg) == R12) { \
            x86_64_address_byte(2,(dreg),REG_SP); \
            x86_64_address_byte(0,REG_SP,REG_SP); \
            x86_64_emit_imm32((disp)); \
        } else {\
            x86_64_address_byte(2,(dreg),(basereg)); \
            x86_64_emit_imm32((disp)); \
        } \
    } while (0)


#define x86_64_emit_memindex(reg,disp,basereg,indexreg,scale) \
    do { \
        if ((basereg) == -1) { \
            x86_64_address_byte(0,(reg),4); \
            x86_64_address_byte((scale),(indexreg),5); \
            x86_64_emit_imm32((disp)); \
        \
        } else if ((disp) == 0 && (basereg) != RBP && (basereg) != R13) { \
            x86_64_address_byte(0,(reg),4); \
            x86_64_address_byte((scale),(indexreg),(basereg)); \
        \
        } else if (IS_IMM8((disp))) { \
            x86_64_address_byte(1,(reg),4); \
            x86_64_address_byte((scale),(indexreg),(basereg)); \
            x86_64_emit_imm8 ((disp)); \
        \
        } else { \
            x86_64_address_byte(2,(reg),4); \
            x86_64_address_byte((scale),(indexreg),(basereg)); \
            x86_64_emit_imm32((disp)); \
        }    \
     } while (0)


#define x86_64_emit_imm8(imm) \
    *(cd->mcodeptr++) = (u1) ((imm) & 0xff);


#define x86_64_emit_imm16(imm) \
    do { \
        x86_64_imm_buf imb; \
        imb.i = (s4) (imm); \
        *(cd->mcodeptr++) = imb.b[0]; \
        *(cd->mcodeptr++) = imb.b[1]; \
    } while (0)


#define x86_64_emit_imm32(imm) \
    do { \
        x86_64_imm_buf imb; \
        imb.i = (s4) (imm); \
        *(cd->mcodeptr++) = imb.b[0]; \
        *(cd->mcodeptr++) = imb.b[1]; \
        *(cd->mcodeptr++) = imb.b[2]; \
        *(cd->mcodeptr++) = imb.b[3]; \
    } while (0)


#define x86_64_emit_imm64(imm) \
    do { \
        x86_64_imm_buf imb; \
        imb.l = (s8) (imm); \
        *(cd->mcodeptr++) = imb.b[0]; \
        *(cd->mcodeptr++) = imb.b[1]; \
        *(cd->mcodeptr++) = imb.b[2]; \
        *(cd->mcodeptr++) = imb.b[3]; \
        *(cd->mcodeptr++) = imb.b[4]; \
        *(cd->mcodeptr++) = imb.b[5]; \
        *(cd->mcodeptr++) = imb.b[6]; \
        *(cd->mcodeptr++) = imb.b[7]; \
    } while (0)


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


#define CALCIMMEDIATEBYTES(var, val) \
    if ((s4) (val) < -128 || (s4) (val) > 127) (var) += 4; \
    else (var) += 1;


/* gen_nullptr_check(objreg) */

#define gen_nullptr_check(objreg) \
	if (checknull) { \
        x86_64_test_reg_reg(cd, (objreg), (objreg)); \
        x86_64_jcc(cd, X86_64_CC_E, 0); \
 	    codegen_addxnullrefs(cd, cd->mcodeptr); \
	}


#define gen_bound_check \
    if (checkbounds) { \
        x86_64_alul_membase_reg(cd, X86_64_CMP, s1, OFFSET(java_arrayheader, size), s2); \
        x86_64_jcc(cd, X86_64_CC_AE, 0); \
        codegen_addxboundrefs(cd, cd->mcodeptr, s2); \
    }


#define gen_div_check(v) \
    if (checknull) { \
        if ((v)->flags & INMEMORY) { \
            x86_64_alu_imm_membase(cd, X86_64_CMP, 0, REG_SP, src->regoff * 8); \
        } else { \
            x86_64_test_reg_reg(cd, src->regoff, src->regoff); \
        } \
        x86_64_jcc(cd, X86_64_CC_E, 0); \
        codegen_addxdivrefs(cd, cd->mcodeptr); \
    }


/* MCODECHECK(icnt) */

#define MCODECHECK(icnt) \
	if ((cd->mcodeptr + (icnt)) > (u1 *) cd->mcodeend) \
        cd->mcodeptr = (u1 *) codegen_increase(cd, cd->mcodeptr)

/* M_INTMOVE:
    generates an integer-move from register a to b.
    if a and b are the same int-register, no code will be generated.
*/ 

#define M_INTMOVE(reg,dreg) \
    if ((reg) != (dreg)) { \
        x86_64_mov_reg_reg(cd, (reg),(dreg)); \
    }


/* M_FLTMOVE:
    generates a floating-point-move from register a to b.
    if a and b are the same float-register, no code will be generated
*/ 

#define M_FLTMOVE(reg,dreg) \
    if ((reg) != (dreg)) { \
        x86_64_movq_reg_reg(cd, (reg),(dreg)); \
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
            x86_64_movl_membase_reg(cd, REG_SP, (v)->regoff * 8, tempnr); \
        } else { \
            x86_64_mov_membase_reg(cd, REG_SP, (v)->regoff * 8, tempnr); \
        } \
        regnr = tempnr; \
    } else { \
        regnr = (v)->regoff; \
    }



#define var_to_reg_flt(regnr,v,tempnr) \
    if ((v)->flags & INMEMORY) { \
        COUNT_SPILLS; \
        if ((v)->type == TYPE_FLT) { \
            x86_64_movlps_membase_reg(cd, REG_SP, (v)->regoff * 8, tempnr); \
        } else { \
            x86_64_movlpd_membase_reg(cd, REG_SP, (v)->regoff * 8, tempnr); \
        } \
/*        x86_64_movq_membase_reg(REG_SP, (v)->regoff * 8, tempnr);*/ \
        regnr = tempnr; \
    } else { \
        regnr = (v)->regoff; \
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
        x86_64_mov_reg_membase(cd, tempregnum, REG_SP, (sptr)->regoff * 8); \
    }


#define store_reg_to_var_flt(sptr, tempregnum) \
    if ((sptr)->flags & INMEMORY) { \
         COUNT_SPILLS; \
         x86_64_movq_reg_membase(cd, tempregnum, REG_SP, (sptr)->regoff * 8); \
    }


#define M_COPY(from,to) \
    d = reg_of_var(rd, to, REG_ITMP1); \
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


/* macros to create code ******************************************************/

#define M_NOP                   x86_64_nop(cd)                  /* ;          */


/* function gen_resolvebranch **************************************************

    backpatches a branch instruction

    parameters: ip ... pointer to instruction after branch (void*)
                so ... offset of instruction after branch  (s8)
                to ... offset of branch target             (s8)

*******************************************************************************/

#define gen_resolvebranch(ip,so,to) \
    *((s4*) ((ip) - 4)) = (s4) ((to) - (so));


/* function prototypes */

void thread_restartcriticalsection(ucontext_t *uc);

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
