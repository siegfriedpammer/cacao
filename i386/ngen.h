/* i386/ngen.h - code generation macros and definitions for i386

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

   $Id: ngen.h 551 2003-11-01 20:35:28Z twisti $

*/


#ifndef _NGEN_H
#define _NGEN_H

/* include here, so function definitions are available in mcode.c */
#include "methodtable.h"


/* see also file calling.doc for explanation of calling conventions           */

/* preallocated registers *****************************************************/

/* integer registers */
  
#define REG_RESULT      I386_EAX /* to deliver method results                 */
#define REG_RESULT2     I386_EDX /* to deliver long method results            */

#define REG_ITMP1       I386_EAX /* temporary register                        */
#define REG_ITMP2       I386_EDX /* temporary register and method pointer     */
#define REG_ITMP3       I386_ECX /* temporary register                        */

#define REG_NULL        -1       /* used for reg_of_var where d is not needed */

#define REG_ITMP1_XPTR  I386_EAX /* exception pointer = temporary register 1  */
#define REG_ITMP2_XPC   I386_EDX /* exception pc = temporary register 2       */

#define REG_SP          I386_ESP /* stack pointer                             */

/* floating point registers */

#define REG_FRESULT     0    /* to deliver floating point method results      */
#define REG_FTMP1       6    /* temporary floating point register             */
#define REG_FTMP2       7    /* temporary floating point register             */
#define REG_FTMP3       7    /* temporary floating point register             */

/* register descripton - array ************************************************/

/* #define REG_RES   0         reserved register for OS or code generator     */
/* #define REG_RET   1         return value register                          */
/* #define REG_EXC   2         exception value register (only old jit)        */
/* #define REG_SAV   3         (callee) saved register                        */
/* #define REG_TMP   4         scratch temporary register (caller saved)      */
/* #define REG_ARG   5         argument register (caller saved)               */

/* #define REG_END   -1        last entry in tables */

int nregdescint[] = {
    REG_RET, REG_RES, REG_RES, REG_SAV, REG_RES, REG_SAV, REG_TMP, REG_TMP,
    REG_END };

/* for use of reserved registers, see comment above */

int nregdescfloat[] = {
  /* rounding problems with callee saved registers */
/*      REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_TMP, REG_TMP, REG_RES, REG_RES, */
/*      REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_RES, REG_RES, */
    REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES,
    REG_END };

/* for use of reserved registers, see comment above */


/* stackframe-infos ***********************************************************/

int parentargs_base; /* offset in stackframe for the parameter from the caller*/

/* -> see file 'calling.doc' */


static u1 fpu_in_24bit_mode = 0;

static u2 fpu_ctrlwrd_24bit = 0x007f;    /* Round to nearest, 24-bit mode, exceptions masked */
static u2 fpu_ctrlwrd_53bit = 0x027f;    /* Round to nearest, 53-bit mode, exceptions masked */

static u4 subnormal_bias1[3] = { 0x00000000, 0x80000000, 0x03ff };    /* 2^(-15360) */
static u4 subnormal_bias2[3] = { 0x00000000, 0x80000000, 0x7bff };    /* 2^(+15360) */


/* macros to create code ******************************************************/

/*
 * immediate data union
 */
typedef union {
    s4 i;
    s8 l;
    float f;
    double d;
    void *a;
    u1 b[8];
} i386_imm_buf;


/*
 * x86 register numbers
 */
typedef enum {
    I386_EAX = 0,
    I386_ECX = 1,
    I386_EDX = 2,
    I386_EBX = 3,
    I386_ESP = 4,
    I386_EBP = 5,
    I386_ESI = 6,
    I386_EDI = 7,
    I386_NREG
} I386_Reg_No;

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


/*
 * opcodes for alu instructions
 */
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

static const unsigned char i386_jcc_map[] = {
    0x00, /* o  */
    0x01, /* no */
    0x02, /* b, lt  */
    0x03, /* ae */
    0x04, /* e  */
    0x05, /* ne */
    0x06, /* be */
    0x07, /* a  */
    0x08, /* s  */
    0x09, /* ns */
    0x0a, /* p  */
    0x0b, /* np */
    0x0c, /* l  */
    0x0d, /* ge */
    0x0e, /* le */
    0x0f  /* g  */
};



/*
 * modrm and stuff
 */
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
        i386_imm_buf imb; \
        imb.i = (int) (imm); \
        *(mcodeptr++) = imb.b[0]; \
        *(mcodeptr++) = imb.b[1]; \
    } while (0)


#define i386_emit_imm32(imm) \
    do { \
        i386_imm_buf imb; \
        imb.i = (int) (imm); \
        *(mcodeptr++) = imb.b[0]; \
        *(mcodeptr++) = imb.b[1]; \
        *(mcodeptr++) = imb.b[2]; \
        *(mcodeptr++) = imb.b[3]; \
    } while (0)


#define i386_emit_mem(r,disp) \
    do { \
        i386_address_byte(0,(r),5); \
        i386_emit_imm32((disp)); \
    } while (0)


#define i386_emit_membase(basereg,disp,dreg) \
    do { \
        if ((basereg) == I386_ESP) { \
            if ((disp) == 0) { \
                i386_address_byte(0, (dreg), I386_ESP); \
                i386_address_byte(0, I386_ESP, I386_ESP); \
            } else if (i386_is_imm8((disp))) { \
                i386_address_byte(1, (dreg), I386_ESP); \
                i386_address_byte(0, I386_ESP, I386_ESP); \
                i386_emit_imm8((disp)); \
            } else { \
                i386_address_byte(2, (dreg), I386_ESP); \
                i386_address_byte(0, I386_ESP, I386_ESP); \
                i386_emit_imm32((disp)); \
            } \
            break; \
        } \
        \
        if ((disp) == 0 && (basereg) != I386_EBP) { \
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
        } else if ((disp) == 0 && (basereg) != I386_EBP) { \
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



void i386_emit_ialu(s4 alu_op, stackptr src, instruction *iptr);
void i386_emit_ialuconst(s4 alu_op, stackptr src, instruction *iptr);
void i386_emit_lalu(s4 alu_op, stackptr src, instruction *iptr);
void i386_emit_laluconst(s4 alu_op, stackptr src, instruction *iptr);
void i386_emit_ishift(s4 shift_op, stackptr src, instruction *iptr);
void i386_emit_ishiftconst(s4 shift_op, stackptr src, instruction *iptr);
void i386_emit_ifcc_iconst(s4 if_op, stackptr src, instruction *iptr);



#if 0

/*
 * mov ops
 */
#define i386_mov_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = (u1) 0x89; \
        i386_emit_reg((reg),(dreg)); \
    } while (0)


#define i386_mov_imm_reg(imm,reg) \
    do { \
        *(mcodeptr++) = (u1) 0xb8 + ((reg) & 0x07); \
        i386_emit_imm32((imm)); \
    } while (0)


#define i386_movb_imm_reg(imm,reg) \
    do { \
        *(mcodeptr++) = (u1) 0xc6; \
        i386_emit_reg(0,(reg)); \
        i386_emit_imm8((imm)); \
    } while (0)


#define i386_mov_float_reg(imm,reg) \
    do { \
        *(mcodeptr++) = (u1) 0xb8 + ((reg) & 0x07); \
        i386_emit_float32((imm)); \
    } while (0)


#define i386_mov_reg_mem(reg,mem) \
    do { \
        *(mcodeptr++) = (u1) 0x89; \
        i386_emit_mem((reg),(mem)); \
    } while (0)


#define i386_mov_mem_reg(mem,reg) \
    do { \
        *(mcodeptr++) = (u1) 0x8b; \
        i386_emit_mem((reg),(mem)); \
    } while (0)


#define i386_mov_membase_reg(basereg,disp,reg) \
    do { \
        *(mcodeptr++) = (u1) 0x8b; \
        i386_emit_membase((basereg),(disp),(reg)); \
    } while (0)


/*
 * this one is for INVOKEVIRTUAL/INVOKEINTERFACE to have a
 * constant membase immediate length of 32bit
 */
#define i386_mov_membase32_reg(basereg,disp,reg) \
    do { \
        *(mcodeptr++) = (u1) 0x8b; \
        i386_address_byte(2, (reg), (basereg)); \
        i386_emit_imm32((disp)); \
    } while (0)


#define i386_movw_membase_reg(basereg,disp,reg) \
    do { \
        *(mcodeptr++) = (u1) 0x66; \
        i386_mov_membase_reg((basereg),(disp),(reg)); \
    } while (0)


#define i386_movb_membase_reg(basereg,disp,reg) \
    do { \
        *(mcodeptr++) = (u1) 0x8a; \
        i386_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define i386_mov_reg_membase(reg,basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0x89; \
        i386_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define i386_movw_reg_membase(reg,basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0x66; \
        *(mcodeptr++) = (u1) 0x89; \
        i386_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define i386_movb_reg_membase(reg,basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0x88; \
        i386_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define i386_mov_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        *(mcodeptr++) = (u1) 0x8b; \
        i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_movw_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        *(mcodeptr++) = (u1) 0x66; \
        *(mcodeptr++) = (u1) 0x8b; \
        i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_movb_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        *(mcodeptr++) = (u1) 0x8a; \
        i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_mov_reg_memindex(reg,disp,basereg,indexreg,scale) \
    do { \
        *(mcodeptr++) = (u1) 0x89; \
        i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_movw_reg_memindex(reg,disp,basereg,indexreg,scale) \
    do { \
        *(mcodeptr++) = (u1) 0x66; \
        *(mcodeptr++) = (u1) 0x89; \
        i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_movb_reg_memindex(reg,disp,basereg,indexreg,scale) \
    do { \
        *(mcodeptr++) = (u1) 0x88; \
        i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_mov_imm_membase(imm,basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xc7; \
        i386_emit_membase((basereg),(disp),0); \
        i386_emit_imm32((imm)); \
    } while (0)


#define i386_mov_float_membase(imm,basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xc7; \
        i386_emit_membase((basereg),(disp),0); \
        i386_emit_float32((imm)); \
    } while (0)


#define i386_mov_double_low_membase(imm,basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xc7; \
        i386_emit_membase((basereg),(disp),0); \
        i386_emit_double64_low((imm)); \
    } while (0)


#define i386_mov_double_high_membase(imm,basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xc7; \
        i386_emit_membase((basereg),(disp),0); \
        i386_emit_double64_high((imm)); \
    } while (0)


#define i386_movsbl_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xbe; \
        i386_emit_reg((reg),(dreg)); \
    } while (0)


#define i386_movswl_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xbf; \
        i386_emit_reg((reg),(dreg)); \
    } while (0)


#define i386_movzbl_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xb6; \
        /* XXX: why do reg and dreg have to be exchanged */ \
        i386_emit_reg((dreg),(reg)); \
    } while (0)


#define i386_movzwl_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xb7; \
        /* XXX: why do reg and dreg have to be exchanged */ \
        i386_emit_reg((dreg),(reg)); \
    } while (0)


#define i386_movsbl_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xbe; \
        i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_movswl_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xbf; \
        i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_movzbl_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xb6; \
        i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_movzwl_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xb7; \
        i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)



/*
 * alu operations
 */
#define i386_alu_reg_reg(opc,reg,dreg) \
    do { \
        *(mcodeptr++) = (((u1) (opc)) << 3) + 1; \
        i386_emit_reg((reg),(dreg)); \
    } while (0)


#define i386_alu_reg_membase(opc,reg,basereg,disp) \
    do { \
        *(mcodeptr++) = (((u1) (opc)) << 3) + 1; \
        i386_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define i386_alu_membase_reg(opc,basereg,disp,reg) \
    do { \
        *(mcodeptr++) = (((u1) (opc)) << 3) + 3; \
        i386_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define i386_alu_imm_reg(opc,imm,dreg) \
    do { \
        if (i386_is_imm8(imm)) { \
            *(mcodeptr++) = (u1) 0x83; \
            i386_emit_reg((opc),(dreg)); \
            i386_emit_imm8((imm)); \
        } else { \
            *(mcodeptr++) = (u1) 0x81; \
            i386_emit_reg((opc),(dreg)); \
            i386_emit_imm32((imm)); \
        } \
    } while (0)


#define i386_alu_imm_membase(opc,imm,basereg,disp) \
    do { \
        if (i386_is_imm8(imm)) { \
            *(mcodeptr++) = (u1) 0x83; \
            i386_emit_membase((basereg),(disp),(opc)); \
            i386_emit_imm8((imm)); \
        } else { \
            *(mcodeptr++) = (u1) 0x81; \
            i386_emit_membase((basereg),(disp),(opc)); \
            i386_emit_imm32((imm)); \
        } \
    } while (0)


#define i386_test_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = (u1) 0x85; \
        i386_emit_reg((reg),(dreg)); \
    } while (0)


#define i386_test_imm_reg(imm,reg) \
    do { \
        *(mcodeptr++) = (u1) 0xf7; \
        i386_emit_reg(0,(reg)); \
        i386_emit_imm32((imm)); \
    } while (0)


#define i386_testw_imm_reg(imm,reg) \
    do { \
        *(mcodeptr++) = (u1) 0x66; \
        *(mcodeptr++) = (u1) 0xf7; \
        i386_emit_reg(0,(reg)); \
        i386_emit_imm16((imm)); \
    } while (0)


#define i386_testb_imm_reg(imm,reg) \
    do { \
        *(mcodeptr++) = (u1) 0xf6; \
        i386_emit_reg(0,(reg)); \
        i386_emit_imm8((imm)); \
    } while (0)



/*
 * inc, dec operations
 */
#define i386_inc_reg(reg) \
    *(mcodeptr++) = (u1) 0x40 + ((reg) & 0x07);


#define i386_inc_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xff; \
        i386_emit_membase((basereg),(disp),0); \
    } while (0)


#define i386_dec_reg(reg) \
    *(mcodeptr++) = (u1) 0x48 + ((reg) & 0x07);

        
#define i386_dec_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xff; \
        i386_emit_membase((basereg),(disp),1); \
    } while (0)




#define i386_cltd() \
    *(mcodeptr++) = (u1) 0x99;



#define i386_imul_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xaf; \
        i386_emit_reg((dreg),(reg)); \
    } while (0)


#define i386_imul_membase_reg(basereg,disp,dreg) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xaf; \
        i386_emit_membase((basereg),(disp),(dreg)); \
    } while (0)


#define i386_imul_imm_reg(imm,dreg) \
    do { \
        if (i386_is_imm8((imm))) { \
            *(mcodeptr++) = (u1) 0x6b; \
            i386_emit_reg(0,(dreg)); \
            i386_emit_imm8((imm)); \
        } else { \
            *(mcodeptr++) = (u1) 0x69; \
            i386_emit_reg(0,(dreg)); \
            i386_emit_imm32((imm)); \
        } \
    } while (0)


#define i386_imul_imm_reg_reg(imm,reg,dreg) \
    do { \
        if (i386_is_imm8((imm))) { \
            *(mcodeptr++) = (u1) 0x6b; \
            i386_emit_reg((dreg),(reg)); \
            i386_emit_imm8((imm)); \
        } else { \
            *(mcodeptr++) = (u1) 0x69; \
            i386_emit_reg((dreg),(reg)); \
            i386_emit_imm32((imm)); \
        } \
    } while (0)


#define i386_imul_imm_membase_reg(imm,basereg,disp,dreg) \
    do { \
        if (i386_is_imm8((imm))) { \
            *(mcodeptr++) = (u1) 0x6b; \
            i386_emit_membase((basereg),(disp),(dreg)); \
            i386_emit_imm8((imm)); \
        } else { \
            *(mcodeptr++) = (u1) 0x69; \
            i386_emit_membase((basereg),(disp),(dreg)); \
            i386_emit_imm32((imm)); \
        } \
    } while (0)


#define i386_mul_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xf7; \
        i386_emit_reg(4,(reg)); \
    } while (0)


#define i386_mul_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xf7; \
        i386_emit_membase((basereg),(disp),4); \
    } while (0)


#define i386_idiv_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xf7; \
        i386_emit_reg(7,(reg)); \
    } while (0)


#define i386_idiv_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xf7; \
        i386_emit_membase((basereg),(disp),7); \
    } while (0)



#define i386_ret() \
    *(mcodeptr++) = (u1) 0xc3;


#define i386_leave() \
    *(mcodeptr++) = (u1) 0xc9;



/*
 * shift ops
 */
#define i386_shift_reg(opc,reg) \
    do { \
        *(mcodeptr++) = (u1) 0xd3; \
        i386_emit_reg((opc),(reg)); \
    } while (0)


#define i386_shift_membase(opc,basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xd3; \
        i386_emit_membase((basereg),(disp),(opc)); \
    } while (0)


#define i386_shift_imm_reg(opc,imm,dreg) \
    do { \
        if ((imm) == 1) { \
            *(mcodeptr++) = (u1) 0xd1; \
            i386_emit_reg((opc),(dreg)); \
        } else { \
            *(mcodeptr++) = (u1) 0xc1; \
            i386_emit_reg((opc),(dreg)); \
            i386_emit_imm8((imm)); \
        } \
    } while (0)


#define i386_shift_imm_membase(opc,imm,basereg,disp) \
    do { \
        if ((imm) == 1) { \
            *(mcodeptr++) = (u1) 0xd1; \
            i386_emit_membase((basereg),(disp),(opc)); \
        } else { \
            *(mcodeptr++) = (u1) 0xc1; \
            i386_emit_membase((basereg),(disp),(opc)); \
            i386_emit_imm8((imm)); \
        } \
    } while (0)


#define i386_shld_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xa5; \
        i386_emit_reg((reg),(dreg)); \
    } while (0)


#define i386_shld_imm_reg_reg(imm,reg,dreg) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xa4; \
        i386_emit_reg((reg),(dreg)); \
        i386_emit_imm8((imm)); \
    } while (0)


#define i386_shld_reg_membase(reg,basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xa5; \
        i386_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define i386_shrd_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xad; \
        i386_emit_reg((reg),(dreg)); \
    } while (0)


#define i386_shrd_imm_reg_reg(imm,reg,dreg) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xac; \
        i386_emit_reg((reg),(dreg)); \
        i386_emit_imm8((imm)); \
    } while (0)


#define i386_shrd_reg_membase(reg,basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xad; \
        i386_emit_membase((basereg),(disp),(reg)); \
    } while (0)



/*
 * jump operations
 */
#define i386_jmp_imm(imm) \
    do { \
        *(mcodeptr++) = (u1) 0xe9; \
        i386_emit_imm32((imm)); \
    } while (0)


#define i386_jmp_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xff; \
        i386_emit_reg(4,(reg)); \
    } while (0)


#define i386_jcc(opc,imm) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) (0x80 + i386_jcc_map[(opc)]); \
        i386_emit_imm32((imm)); \
    } while (0)



/*
 * conditional set operations
 */
#define i386_setcc_reg(opc,reg) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) (0x90 + i386_jcc_map[(opc)]); \
        i386_emit_reg(0,(reg)); \
    } while (0)


#define i386_setcc_membase(opc,basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) (0x90 + i386_jcc_map[(opc)]); \
        i386_emit_membase((basereg),(disp),0); \
    } while (0)



#define i386_neg_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xf7; \
        i386_emit_reg(3,(reg)); \
    } while (0)


#define i386_neg_mem(mem) \
    do { \
        *(mcodeptr++) = (u1) 0xf7; \
        i386_emit_mem(3,(mem)); \
    } while (0)


#define i386_neg_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xf7; \
        i386_emit_membase((basereg),(disp),3); \
    } while (0)



#define i386_push_reg(reg) \
    *(mcodeptr++) = (u1) 0x50 + (0x07 & (reg));


#define i386_push_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xff; \
        i386_emit_membase((basereg),(disp),6); \
    } while (0)


#define i386_push_imm(imm) \
    do { \
        *(mcodeptr++) = (u1) 0x68; \
        i386_emit_imm32((imm)); \
    } while (0)


#define i386_pop_reg(reg) \
    *(mcodeptr++) = (u1) 0x58 + (0x07 & (reg));


#define i386_nop() \
    *(mcodeptr++) = (u1) 0x90;



/*
 * call instructions
 */
#define i386_call_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xff; \
        i386_emit_reg(2,(reg)); \
    } while (0)


#define i386_call_imm(imm) \
    do { \
        *(mcodeptr++) = (u1) 0xe8; \
        i386_emit_imm32((imm)); \
    } while (0)



/*
 * floating point instructions
 */
#define i386_fld1() \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        *(mcodeptr++) = (u1) 0xe8; \
    } while (0)


#define i386_fldz() \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        *(mcodeptr++) = (u1) 0xee; \
    } while (0)


#define i386_fld_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        *(mcodeptr++) = (u1) 0xc0 + (0x07 & (reg)); \
    } while (0)


#define i386_flds_mem(mem) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        i386_emit_mem(0,(mem)); \
    } while (0)


#define i386_fldl_mem(mem) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        i386_emit_mem(0,(mem)); \
    } while (0)


#define i386_fldt_mem(mem) \
    do { \
        *(mcodeptr++) = (u1) 0xdb; \
        i386_emit_mem(5,(mem)); \
    } while (0)


#define i386_flds_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        i386_emit_membase((basereg),(disp),0); \
    } while (0)


#define i386_fldl_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        i386_emit_membase((basereg),(disp),0); \
    } while (0)


#define i386_fldt_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdb; \
        i386_emit_membase((basereg),(disp),5); \
    } while (0)


#define i386_flds_memindex(disp,basereg,indexreg,scale) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        i386_emit_memindex(0,(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_fldl_memindex(disp,basereg,indexreg,scale) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        i386_emit_memindex(0,(disp),(basereg),(indexreg),(scale)); \
    } while (0)




#define i386_fildl_mem(mem) \
    do { \
        *(mcodeptr++) = (u1) 0xdb; \
        i386_emit_mem(0,(mem)); \
    } while (0)


#define i386_fildl_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdb; \
        i386_emit_membase((basereg),(disp),0); \
    } while (0)


#define i386_fildll_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdf; \
        i386_emit_membase((basereg),(disp),5); \
    } while (0)




#define i386_fst_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        *(mcodeptr++) = (u1) 0xd0 + (0x07 & (reg)); \
    } while (0)


#define i386_fsts_mem(mem) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        i386_emit_mem(2,(mem)); \
    } while (0)


#define i386_fstl_mem(mem) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        i386_emit_mem(2,(mem)); \
    } while (0)


#define i386_fsts_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        i386_emit_membase((basereg),(disp),2); \
    } while (0)


#define i386_fstl_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        i386_emit_membase((basereg),(disp),2); \
    } while (0)


#define i386_fsts_memindex(disp,basereg,indexreg,scale) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        i386_emit_memindex(2,(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_fstl_memindex(disp,basereg,indexreg,scale) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        i386_emit_memindex(2,(disp),(basereg),(indexreg),(scale)); \
    } while (0)




#define i386_fstp_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        *(mcodeptr++) = (u1) 0xd8 + (0x07 & (reg)); \
    } while (0)


#define i386_fstps_mem(mem) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        i386_emit_mem(3,(mem)); \
    } while (0)


#define i386_fstpl_mem(mem) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        i386_emit_mem(3,(mem)); \
    } while (0)


#define i386_fstps_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        i386_emit_membase((basereg),(disp),3); \
    } while (0)


#define i386_fstpl_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        i386_emit_membase((basereg),(disp),3); \
    } while (0)


#define i386_fstpt_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdb; \
        i386_emit_membase((basereg),(disp),7); \
    } while (0)


#define i386_fstps_memindex(disp,basereg,indexreg,scale) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        i386_emit_memindex(3,(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_fstpl_memindex(disp,basereg,indexreg,scale) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        i386_emit_memindex(3,(disp),(basereg),(indexreg),(scale)); \
    } while (0)




#define i386_fistpl_mem(mem) \
    do { \
        *(mcodeptr++) = (u1) 0xdb; \
        i386_emit_mem(3,(mem)); \
    } while (0)


#define i386_fistpll_mem(mem) \
    do { \
        *(mcodeptr++) = (u1) 0xdf; \
        i386_emit_mem(7,(mem)); \
    } while (0)


#define i386_fistl_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdb; \
        i386_emit_membase((basereg),(disp),2); \
    } while (0)


#define i386_fistpl_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdb; \
        i386_emit_membase((basereg),(disp),3); \
    } while (0)


#define i386_fistpll_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdf; \
        i386_emit_membase((basereg),(disp),7); \
    } while (0)




#define i386_fchs() \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        *(mcodeptr++) = (u1) 0xe0; \
    } while (0)


#define i386_faddp() \
    do { \
        *(mcodeptr++) = (u1) 0xde; \
        *(mcodeptr++) = (u1) 0xc1; \
    } while (0)


#define i386_fadd_reg_st(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xd8; \
        *(mcodeptr++) = (u1) 0xc0 + (0x0f & (reg)); \
    } while (0)


#define i386_fadd_st_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xdc; \
        *(mcodeptr++) = (u1) 0xc0 + (0x0f & (reg)); \
    } while (0)


#define i386_faddp_st_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xde; \
        *(mcodeptr++) = (u1) 0xc0 + (0x0f & (reg)); \
    } while (0)


#define i386_fadds_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xd8; \
        i386_emit_membase((basereg),(disp),0); \
    } while (0)


#define i386_faddl_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdc; \
        i386_emit_membase((basereg),(disp),0); \
    } while (0)


#define i386_fsub_reg_st(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xd8; \
        *(mcodeptr++) = (u1) 0xe0 + (0x07 & (reg)); \
    } while (0)


#define i386_fsub_st_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xdc; \
        *(mcodeptr++) = (u1) 0xe8 + (0x07 & (reg)); \
    } while (0)


#define i386_fsubp_st_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xde; \
        *(mcodeptr++) = (u1) 0xe8 + (0x07 & (reg)); \
    } while (0)


#define i386_fsubp() \
    do { \
        *(mcodeptr++) = (u1) 0xde; \
        *(mcodeptr++) = (u1) 0xe9; \
    } while (0)


#define i386_fsubs_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xd8; \
        i386_emit_membase((basereg),(disp),4); \
    } while (0)


#define i386_fsubl_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdc; \
        i386_emit_membase((basereg),(disp),4); \
    } while (0)


#define i386_fmul_reg_st(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xd8; \
        *(mcodeptr++) = (u1) 0xc8 + (0x07 & (reg)); \
    } while (0)


#define i386_fmul_st_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xdc; \
        *(mcodeptr++) = (u1) 0xc8 + (0x07 & (reg)); \
    } while (0)


#define i386_fmulp() \
    do { \
        *(mcodeptr++) = (u1) 0xde; \
        *(mcodeptr++) = (u1) 0xc9; \
    } while (0)


#define i386_fmulp_st_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xde; \
        *(mcodeptr++) = (u1) 0xc8 + (0x07 & (reg)); \
    } while (0)


#define i386_fmuls_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xd8; \
        i386_emit_membase((basereg),(disp),1); \
    } while (0)


#define i386_fmull_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdc; \
        i386_emit_membase((basereg),(disp),1); \
    } while (0)


#define i386_fdiv_reg_st(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xd8; \
        *(mcodeptr++) = (u1) 0xf0 + (0x07 & (reg)); \
    } while (0)


#define i386_fdiv_st_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xdc; \
        *(mcodeptr++) = (u1) 0xf8 + (0x07 & (reg)); \
    } while (0)


#define i386_fdivp() \
    do { \
        *(mcodeptr++) = (u1) 0xde; \
        *(mcodeptr++) = (u1) 0xf9; \
    } while (0)


#define i386_fdivp_st_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xde; \
        *(mcodeptr++) = (u1) 0xf8 + (0x07 & (reg)); \
    } while (0)


#define i386_fxch() \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        *(mcodeptr++) = (u1) 0xc9; \
    } while (0)


#define i386_fxch_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        *(mcodeptr++) = (u1) 0xc8 + (0x07 & (reg)); \
    } while (0)


#define i386_fprem() \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        *(mcodeptr++) = (u1) 0xf8; \
    } while (0)


#define i386_fprem1() \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        *(mcodeptr++) = (u1) 0xf5; \
    } while (0)


#define i386_fucom() \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        *(mcodeptr++) = (u1) 0xe1; \
    } while (0)


#define i386_fucom_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        *(mcodeptr++) = (u1) 0xe0 + (0x07 & (reg)); \
    } while (0)


#define i386_fucomp_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        *(mcodeptr++) = (u1) 0xe8 + (0x07 & (reg)); \
    } while (0)


#define i386_fucompp() \
    do { \
        *(mcodeptr++) = (u1) 0xda; \
        *(mcodeptr++) = (u1) 0xe9; \
    } while (0)


#define i386_fnstsw() \
    do { \
        *(mcodeptr++) = (u1) 0xdf; \
        *(mcodeptr++) = (u1) 0xe0; \
    } while (0)


#define i386_sahf() \
    *(mcodeptr++) = (u1) 0x9e;


#define i386_finit() \
    do { \
        *(mcodeptr++) = (u1) 0x9b; \
        *(mcodeptr++) = (u1) 0xdb; \
        *(mcodeptr++) = (u1) 0xe3; \
    } while (0)


#define i386_fldcw_mem(mem) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        i386_emit_mem(5,(mem)); \
    } while (0)


#define i386_fldcw_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        i386_emit_membase((basereg),(disp),5); \
    } while (0)


#define i386_wait() \
    *(mcodeptr++) = (u1) 0x9b;


#define i386_ffree_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        *(mcodeptr++) = (u1) 0xc0 + (0x07 & (reg)); \
    } while (0)


#define i386_fdecstp() \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        *(mcodeptr++) = (u1) 0xf6; \
    } while (0)


#define i386_fincstp() \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        *(mcodeptr++) = (u1) 0xf7; \
    } while (0)

#else

/*
 * integer instructions
 */
void i386_mov_reg_reg(s4 reg, s4 dreg);
void i386_mov_imm_reg(s4 imm, s4 dreg);
void i386_movb_imm_reg(s4 imm, s4 dreg);
void i386_mov_membase_reg(s4 basereg, s4 disp, s4 reg);
void i386_mov_membase32_reg(s4 basereg, s4 disp, s4 reg);
void i386_mov_reg_membase(s4 reg, s4 basereg, s4 disp);
void i386_mov_memindex_reg(s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg);
void i386_mov_reg_memindex(s4 reg, s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_movw_reg_memindex(s4 reg, s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_movb_reg_memindex(s4 reg, s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_mov_imm_membase(s4 imm, s4 basereg, s4 disp);
void i386_movsbl_memindex_reg(s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg);
void i386_movswl_memindex_reg(s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg);
void i386_movzwl_memindex_reg(s4 disp, s4 basereg, s4 indexreg, s4 scale, s4 reg);
void i386_alu_reg_reg(s4 opc, s4 reg, s4 dreg);
void i386_alu_reg_membase(s4 opc, s4 reg, s4 basereg, s4 disp);
void i386_alu_membase_reg(s4 opc, s4 basereg, s4 disp, s4 reg);
void i386_alu_imm_reg(s4 opc, s4 imm, s4 reg);
void i386_alu_imm_membase(s4 opc, s4 imm, s4 basereg, s4 disp);
void i386_test_reg_reg(s4 reg, s4 dreg);
void i386_test_imm_reg(s4 imm, s4 dreg);
void i386_inc_reg(s4 reg);
void i386_inc_membase(s4 basereg, s4 disp);
void i386_dec_reg(s4 reg);
void i386_dec_membase(s4 basereg, s4 disp);
void i386_cltd();
void i386_imul_reg_reg(s4 reg, s4 dreg);
void i386_imul_membase_reg(s4 basereg, s4 disp, s4 dreg);
void i386_imul_imm_reg(s4 imm, s4 reg);
void i386_imul_imm_reg_reg(s4 imm, s4 reg, s4 dreg);
void i386_imul_imm_membase_reg(s4 imm, s4 basereg, s4 disp, s4 dreg);
void i386_mul_membase(s4 basereg, s4 disp);
void i386_idiv_reg(s4 reg);
void i386_ret();
void i386_shift_reg(s4 opc, s4 reg);
void i386_shift_membase(s4 opc, s4 basereg, s4 disp);
void i386_shift_imm_reg(s4 opc, s4 imm, s4 reg);
void i386_shift_imm_membase(s4 opc, s4 imm, s4 basereg, s4 disp);
void i386_shld_reg_reg(s4 reg, s4 dreg);
void i386_shld_imm_reg_reg(s4 imm, s4 reg, s4 dreg);
void i386_shld_reg_membase(s4 reg, s4 basereg, s4 disp);
void i386_shrd_reg_reg(s4 reg, s4 dreg);
void i386_shrd_imm_reg_reg(s4 imm, s4 reg, s4 dreg);
void i386_shrd_reg_membase(s4 reg, s4 basereg, s4 disp);
void i386_jmp_imm(s4 imm);
void i386_jmp_reg(s4 reg);
void i386_jcc(s4 opc, s4 imm);
void i386_setcc_reg(s4 opc, s4 reg);
void i386_setcc_membase(s4 opc, s4 basereg, s4 disp);
void i386_neg_reg(s4 reg);
void i386_neg_membase(s4 basereg, s4 disp);
void i386_push_imm(s4 imm);
void i386_pop_reg(s4 reg);
void i386_nop();
void i386_call_reg(s4 reg);
void i386_call_imm(s4 imm);



/*
 * floating point instructions
 */
void i386_fld1();
void i386_fldz();
void i386_fld_reg(s4 reg);
void i386_flds_membase(s4 basereg, s4 disp);
void i386_fldl_membase(s4 basereg, s4 disp);
void i386_fldt_membase(s4 basereg, s4 disp);
void i386_flds_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_fldl_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_fildl_membase(s4 basereg, s4 disp);
void i386_fildll_membase(s4 basereg, s4 disp);
void i386_fst_reg(s4 reg);
void i386_fsts_membase(s4 basereg, s4 disp);
void i386_fstl_membase(s4 basereg, s4 disp);
void i386_fsts_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_fstl_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_fstp_reg(s4 reg);
void i386_fstps_membase(s4 basereg, s4 disp);
void i386_fstpl_membase(s4 basereg, s4 disp);
void i386_fstpt_membase(s4 basereg, s4 disp);
void i386_fstps_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_fstpl_memindex(s4 disp, s4 basereg, s4 indexreg, s4 scale);
void i386_fistl_membase(s4 basereg, s4 disp);
void i386_fistpl_membase(s4 basereg, s4 disp);
void i386_fistpll_membase(s4 basereg, s4 disp);
void i386_fchs();
void i386_faddp();
void i386_fadd_reg_st(s4 reg);
void i386_fadd_st_reg(s4 reg);
void i386_faddp_st_reg(s4 reg);
void i386_fadds_membase(s4 basereg, s4 disp);
void i386_faddl_membase(s4 basereg, s4 disp);
void i386_fsub_reg_st(s4 reg);
void i386_fsub_st_reg(s4 reg);
void i386_fsubp_st_reg(s4 reg);
void i386_fsubp();
void i386_fsubs_membase(s4 basereg, s4 disp);
void i386_fsubl_membase(s4 basereg, s4 disp);
void i386_fmul_reg_st(s4 reg);
void i386_fmul_st_reg(s4 reg);
void i386_fmulp();
void i386_fmulp_st_reg(s4 reg);
void i386_fmuls_membase(s4 basereg, s4 disp);
void i386_fmull_membase(s4 basereg, s4 disp);
void i386_fdiv_reg_st(s4 reg);
void i386_fdiv_st_reg(s4 reg);
void i386_fdivp();
void i386_fdivp_st_reg(s4 reg);
void i386_fxch();
void i386_fxch_reg(s4 reg);
void i386_fprem();
void i386_fprem1();
void i386_fucom();
void i386_fucom_reg(s4 reg);
void i386_fucomp_reg(s4 reg);
void i386_fucompp();
void i386_fnstsw();
void i386_sahf();
void i386_finit();
void i386_fldcw_mem(s4 mem);
void i386_fldcw_membase(s4 basereg, s4 disp);
void i386_wait();
void i386_ffree_reg(s4 reg);
void i386_fdecstp();
void i386_fincstp();

#endif



/* function gen_resolvebranch **************************************************

    backpatches a branch instruction

    parameters: ip ... pointer to instruction after branch (void*)
                so ... offset of instruction after branch  (s4)
                to ... offset of branch target             (s4)

*******************************************************************************/

#define gen_resolvebranch(ip,so,to) \
    *((void **) ((ip) - 4)) = (void **) ((to) - (so));

#endif
