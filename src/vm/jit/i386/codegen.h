/* jit/i386/codegen.h - code generation macros and definitions for i386

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser,
   M. Probst, S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck,
   P. Tomsich, J. Wenninger

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

   $Id: codegen.h 1203 2004-06-22 23:14:55Z twisti $

*/


#ifndef _CODEGEN_H
#define _CODEGEN_H


#include <ucontext.h>
#include "jit/jit.h"


/* define x86 register numbers */
#define EAX    0
#define ECX    1
#define EDX    2
#define EBX    3
#define ESP    4
#define EBP    5
#define ESI    6
#define EDI    7


/* preallocated registers *****************************************************/

/* integer registers */
  
#define REG_RESULT      EAX      /* to deliver method results                 */
#define REG_RESULT2     EDX      /* to deliver long method results            */

#define REG_ITMP1       EAX      /* temporary register                        */
#define REG_ITMP2       ECX      /* temporary register                        */
#define REG_ITMP3       EDX      /* temporary register                        */

#define REG_NULL        -1       /* used for reg_of_var where d is not needed */

#define REG_ITMP1_XPTR  EAX      /* exception pointer = temporary register 1  */
#define REG_ITMP2_XPC   ECX      /* exception pc = temporary register 2       */

#define REG_SP          ESP      /* stack pointer                             */

/* floating point registers */

#define REG_FRESULT     0    /* to deliver floating point method results      */
#define REG_FTMP1       6    /* temporary floating point register             */
#define REG_FTMP2       7    /* temporary floating point register             */
#define REG_FTMP3       7    /* temporary floating point register             */


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

#define gen_bound_check \
    if (checkbounds) { \
        i386_alu_membase_reg(I386_CMP, s1, OFFSET(java_arrayheader, size), s2); \
        i386_jcc(I386_CC_AE, 0); \
        codegen_addxboundrefs(mcodeptr, s2); \
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


/* macros to create code ******************************************************/

typedef enum {
    I386_AL = 0,
    I386_CL = 1,
    I386_DL = 2,
    I386_BL = 3,
    I386_AH = 4,
    I386_CH = 5,
    I386_DH = 6,
    I386_BH = 7,
    I386_NREGB
} I386_RegB_No;


/* opcodes for alu instructions */

typedef enum {
    I386_ADD = 0,
    I386_OR  = 1,
    I386_ADC = 2,
    I386_SBB = 3,
    I386_AND = 4,
    I386_SUB = 5,
    I386_XOR = 6,
    I386_CMP = 7,
    I386_NALU
} I386_ALU_Opcode;

typedef enum {
    I386_ROL = 0,
    I386_ROR = 1,
    I386_RCL = 2,
    I386_RCR = 3,
    I386_SHL = 4,
    I386_SHR = 5,
    I386_SAR = 7,
    I386_NSHIFT = 8
} I386_Shift_Opcode;

typedef enum {
    I386_CC_O = 0,
    I386_CC_NO = 1,
    I386_CC_B = 2, I386_CC_C = 2, I386_CC_NAE = 2,
    I386_CC_BE = 6, I386_CC_NA = 6,
    I386_CC_AE = 3, I386_CC_NB = 3, I386_CC_NC = 3,
    I386_CC_E = 4, I386_CC_Z = 4,
    I386_CC_NE = 5, I386_CC_NZ = 5,
    I386_CC_A = 7, I386_CC_NBE = 7,
    I386_CC_S = 8, I386_CC_LZ = 8,
    I386_CC_NS = 9, I386_CC_GEZ = 9,
    I386_CC_P = 0x0a, I386_CC_PE = 0x0a,
    I386_CC_NP = 0x0b, I386_CC_PO = 0x0b,
    I386_CC_L = 0x0c, I386_CC_NGE = 0x0c,
    I386_CC_GE = 0x0d, I386_CC_NL = 0x0d,
    I386_CC_LE = 0x0e, I386_CC_NG = 0x0e,
    I386_CC_G = 0x0f, I386_CC_NLE = 0x0f,
    I386_NCC
} I386_CC;


/* modrm and stuff */

#define i386_address_byte(mod, reg, rm) \
    *(mcodeptr++) = ((((mod) & 0x03) << 6) | (((reg) & 0x07) << 3) | (((rm) & 0x07)));


#define i386_emit_reg(reg,rm) \
    i386_address_byte(3,(reg),(rm));


#define i386_is_imm8(imm) \
    (((int)(imm) >= -128 && (int)(imm) <= 127))


#define i386_emit_imm8(imm) \
    *(mcodeptr++) = (u1) ((imm) & 0xff);


#define i386_emit_imm16(imm) \
    do { \
        imm_union imb; \
        imb.i = (int) (imm); \
        *(mcodeptr++) = imb.b[0]; \
        *(mcodeptr++) = imb.b[1]; \
    } while (0)


#define i386_emit_imm32(imm) \
    do { \
        imm_union imb; \
        imb.i = (int) (imm); \
        *(mcodeptr++) = imb.b[0]; \
        *(mcodeptr++) = imb.b[1]; \
        *(mcodeptr++) = imb.b[2]; \
        *(mcodeptr++) = imb.b[3]; \
    } while (0)


#define i386_emit_mem(r,mem) \
    do { \
        i386_address_byte(0,(r),5); \
        i386_emit_imm32((mem)); \
    } while (0)


#define i386_emit_membase(basereg,disp,dreg) \
    do { \
        if ((basereg) == ESP) { \
            if ((disp) == 0) { \
                i386_address_byte(0, (dreg), ESP); \
                i386_address_byte(0, ESP, ESP); \
            } else if (i386_is_imm8((disp))) { \
                i386_address_byte(1, (dreg), ESP); \
                i386_address_byte(0, ESP, ESP); \
                i386_emit_imm8((disp)); \
            } else { \
                i386_address_byte(2, (dreg), ESP); \
                i386_address_byte(0, ESP, ESP); \
                i386_emit_imm32((disp)); \
            } \
            break; \
        } \
        \
        if ((disp) == 0 && (basereg) != EBP) { \
            i386_address_byte(0, (dreg), (basereg)); \
            break; \
        } \
        \
        if (i386_is_imm8((disp))) { \
            i386_address_byte(1, (dreg), (basereg)); \
            i386_emit_imm8((disp)); \
        } else { \
            i386_address_byte(2, (dreg), (basereg)); \
            i386_emit_imm32((disp)); \
        } \
    } while (0)


#define i386_emit_memindex(reg,disp,basereg,indexreg,scale) \
    do { \
        if ((basereg) == -1) { \
            i386_address_byte(0, (reg), 4); \
            i386_address_byte((scale), (indexreg), 5); \
            i386_emit_imm32((disp)); \
        \
        } else if ((disp) == 0 && (basereg) != EBP) { \
            i386_address_byte(0, (reg), 4); \
            i386_address_byte((scale), (indexreg), (basereg)); \
        \
        } else if (i386_is_imm8((disp))) { \
            i386_address_byte(1, (reg), 4); \
            i386_address_byte((scale), (indexreg), (basereg)); \
            i386_emit_imm8 ((disp)); \
        \
        } else { \
            i386_address_byte(2, (reg), 4); \
            i386_address_byte((scale), (indexreg), (basereg)); \
            i386_emit_imm32((disp)); \
        }    \
     } while (0)


/* function gen_resolvebranch **************************************************

    backpatches a branch instruction

    parameters: ip ... pointer to instruction after branch (void*)
                so ... offset of instruction after branch  (s4)
                to ... offset of branch target             (s4)

*******************************************************************************/

#define gen_resolvebranch(ip,so,to) \
    *((void **) ((ip) - 4)) = (void **) ((to) - (so));


/* function prototypes */

void codegen_init();
void *codegen_findmethod(void *pc);
void init_exceptions();
void codegen(methodinfo *m);
void codegen_close();
void dseg_display(s4 *s4ptr);
void thread_restartcriticalsection(ucontext_t*);

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
