/* x86_64/ngen.h ***************************************************************

    Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

    See file COPYRIGHT for information on usage and disclaimer of warranties

    Contains the machine dependent code generator definitions and macros for an
    x86_64 processor.

    Authors: Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at
             Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
             Christian Thalinger EMAIL: cacao@complang.tuwien.ac.at

    Last Change: $Id: ngen.h 399 2003-08-01 10:46:28Z twisti $

*******************************************************************************/

#ifndef _NGEN_H
#define _NGEN_H

/* include here, so function definitions are available in mcode.c */
#include "methodtable.h"


/* see also file calling.doc for explanation of calling conventions           */

/* preallocated registers *****************************************************/

/* integer registers */
  
#define REG_RESULT      RAX      /* to deliver method results                 */

#define REG_ITMP1       RAX      /* temporary register                        */
#define REG_ITMP2       RDX      /* temporary register and method pointer     */
#define REG_ITMP3       RCX      /* temporary register                        */

#define REG_ITMP1_XPTR  RAX      /* exception pointer = temporary register 1  */
#define REG_ITMP2_XPC   RDX      /* exception pc = temporary register 2       */

#define REG_SP          RSP      /* stack pointer                             */

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
    REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES,
    REG_END
};

/* for use of reserved registers, see comment above */

int nregdescfloat[] = {
    REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES,
    REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES,
    REG_END
};

/* for use of reserved registers, see comment above */


/* stackframe-infos ***********************************************************/

int parentargs_base; /* offset in stackframe for the parameter from the caller*/

/* -> see file 'calling.doc' */


/* macros to create code ******************************************************/

#define M_OP3(op,fu,a,b,c,const) \
do { \
        printf("M_OP3: %d\n", __LINE__); \
	*(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x04; \
} while (0)


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
} x86_64_imm_buf;


/*
 * x86_64 register numbers
 */
typedef enum {
    RAX = 0,
    RCX = 1,
    RDX = 2,
    RBX = 3,
    RSP = 4,
    RBP = 5,
    RSI = 6,
    RDI = 7,
    R8 = 8,
    R9 = 9,
    R10 = 10,
    R11 = 11,
    R12 = 12,
    R13 = 13,
    R14 = 14,
    R15 = 15,
    NREG
} X86_64_Reg_No;

typedef enum {
    EAX = 0,
    ECX = 1,
    EDX = 2,
    EBX = 3,
    ESP = 4,
    EBP = 5,
    ESI = 6,
    EDI = 7,
    R8D = 8,
    R9D = 9,
    R10D = 10,
    R11D = 11,
    R12D = 12,
    R13D = 13,
    R14D = 14,
    R15D = 15,
    NREGD
} X86_64_RegD_No;

typedef enum {
    AL = 0,
    CL = 1,
    DL = 2,
    BL = 3,
    SPL = 4,
    BPL = 5,
    SIL = 6,
    DIL = 7,
    R8B = 8,
    R9B = 9,
    R10B = 10,
    R11B = 11,
    R12B = 12,
    R13B = 13,
    R14B = 14,
    R15B = 15,
    NREGB
} X86_64_RegB_No;


/*
 * opcodes for alu instructions
 */
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

static const unsigned char x86_64_jcc_map[] = {
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
#define x86_64_address_byte(mod, reg, rm) \
    *(mcodeptr++) = ((((mod) & 0x03) << 6) | (((reg) & 0x07) << 3) | ((rm) & 0x07));


#define x86_64_emit_reg(reg,rm) \
    x86_64_address_byte(3,(reg),(rm));


#define x86_64_emit_rex(size,reg,rm) \
    if ((size) == 1 || ((size) == 0 && (reg) > 7)) { \
        *(mcodeptr++) = (0x40 | (((size) & 0x01) << 3) | ((((reg) >> 3) & 0x01) << 2) | (((rm) >> 3) & 0x01)); \
    }


#define x86_64_is_imm8(imm) \
    (((long)(imm) >= -128 && (long)(imm) <= 127))


#define x86_64_is_imm32(imm) \
    (((long)(imm) >= (-2147483647-1) && (long)(imm) <= 2147483647))


#define x86_64_emit_imm8(imm) \
    *(mcodeptr++) = (u1) ((imm) & 0xff);


#define x86_64_emit_imm16(imm) \
    do { \
        x86_64_imm_buf imb; \
        imb.i = (s4) (imm); \
        *(mcodeptr++) = imb.b[0]; \
        *(mcodeptr++) = imb.b[1]; \
    } while (0)


#define x86_64_emit_imm32(imm) \
    do { \
        x86_64_imm_buf imb; \
        imb.i = (s4) (imm); \
        *(mcodeptr++) = imb.b[0]; \
        *(mcodeptr++) = imb.b[1]; \
        *(mcodeptr++) = imb.b[2]; \
        *(mcodeptr++) = imb.b[3]; \
    } while (0)


#define x86_64_emit_imm64(imm) \
    do { \
        x86_64_imm_buf imb; \
        imb.l = (s8) (imm); \
        *(mcodeptr++) = imb.b[0]; \
        *(mcodeptr++) = imb.b[1]; \
        *(mcodeptr++) = imb.b[2]; \
        *(mcodeptr++) = imb.b[3]; \
        *(mcodeptr++) = imb.b[4]; \
        *(mcodeptr++) = imb.b[5]; \
        *(mcodeptr++) = imb.b[6]; \
        *(mcodeptr++) = imb.b[7]; \
    } while (0)


#define x86_64_emit_mem(r,disp) \
    do { \
        x86_64_address_byte(0,(r),5); \
        x86_64_emit_imm32((disp)); \
    } while (0)


#define x86_64_emit_membase(basereg,disp,dreg) \
    do { \
        if ((basereg) == REG_SP) { \
            if ((disp) == 0) { \
                x86_64_address_byte(0, (dreg), REG_SP); \
                x86_64_address_byte(0, REG_SP, REG_SP); \
            } else if (x86_64_is_imm8((disp))) { \
                x86_64_address_byte(1, (dreg), REG_SP); \
                x86_64_address_byte(0, REG_SP, REG_SP); \
                x86_64_emit_imm8((disp)); \
            } else { \
                x86_64_address_byte(2, (dreg), REG_SP); \
                x86_64_address_byte(0, REG_SP, REG_SP); \
                x86_64_emit_imm32((disp)); \
            } \
            break; \
        } \
        \
        if ((disp) == 0 && (basereg) != RBP) { \
            x86_64_address_byte(0, (dreg), (basereg)); \
            break; \
        } \
        \
        if (x86_64_is_imm8((disp))) { \
            x86_64_address_byte(1, (dreg), (basereg)); \
            x86_64_emit_imm8((disp)); \
        } else { \
            x86_64_address_byte(2, (dreg), (basereg)); \
            x86_64_emit_imm32((disp)); \
        } \
    } while (0)


#define x86_64_emit_memindex(reg,disp,basereg,indexreg,scale) \
    do { \
        if ((basereg) == -1) { \
            x86_64_address_byte(0, (reg), 4); \
            x86_64_address_byte((scale), (indexreg), 5); \
            x86_64_emit_imm32((disp)); \
        \
        } else if ((disp) == 0 && (basereg) != RBP) { \
            x86_64_address_byte(0, (reg), 4); \
            x86_64_address_byte((scale), (indexreg), (basereg)); \
        \
        } else if (x86_64_is_imm8((disp))) { \
            x86_64_address_byte(1, (reg), 4); \
            x86_64_address_byte((scale), (indexreg), (basereg)); \
            x86_64_emit_imm8 ((disp)); \
        \
        } else { \
            x86_64_address_byte(2, (reg), 4); \
            x86_64_address_byte((scale), (indexreg), (basereg)); \
            x86_64_emit_imm32((disp)); \
        }    \
     } while (0)



/*
 * mov ops
 */
#define x86_64_mov_reg_reg(reg,dreg) \
    do { \
        x86_64_emit_rex(1,(reg),(dreg)); \
        *(mcodeptr++) = (u1) 0x89; \
        x86_64_emit_reg((reg),(dreg)); \
    } while (0)


#define x86_64_mov_imm_reg(imm,reg) \
    do { \
        x86_64_emit_rex(1,(reg),0); \
        *(mcodeptr++) = (u1) 0xb8 + ((reg) & 0x07); \
        x86_64_emit_imm64((imm)); \
    } while (0)


#define x86_64_movl_imm_reg(imm,reg) \
    do { \
        x86_64_emit_rex(0,(reg),0); \
        *(mcodeptr++) = (u1) 0xb8 + ((reg) & 0x07); \
        x86_64_emit_imm32((imm)); \
    } while (0)


#define x86_64_movb_imm_reg(imm,reg) \
    do { \
        *(mcodeptr++) = (u1) 0xc6; \
        x86_64_emit_reg(0,(reg)); \
        x86_64_emit_imm8((imm)); \
    } while (0)


#define x86_64_mov_float_reg(imm,reg) \
    do { \
        *(mcodeptr++) = (u1) 0xb8 + ((reg) & 0x07); \
        x86_64_emit_float32((imm)); \
    } while (0)


#define x86_64_mov_membase_reg(basereg,disp,reg) \
    do { \
        x86_64_emit_rex(1,(basereg),(reg)); \
        *(mcodeptr++) = (u1) 0x8b; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_movl_membase_reg(basereg,disp,reg) \
    do { \
        x86_64_emit_rex(0,(basereg),(reg)); \
        *(mcodeptr++) = (u1) 0x8b; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


/*
 * this one is for INVOKEVIRTUAL/INVOKEINTERFACE to have a
 * constant membase immediate length of 32bit
 */
#define x86_64_mov_membase32_reg(basereg,disp,reg) \
    do { \
        x86_64_emit_rex(1,(basereg),(reg)); \
        *(mcodeptr++) = (u1) 0x8b; \
        x86_64_address_byte(2, (reg), (basereg)); \
        x86_64_emit_imm32((disp)); \
    } while (0)


#define x86_64_movw_membase_reg(basereg,disp,reg) \
    do { \
        *(mcodeptr++) = (u1) 0x66; \
        x86_64_mov_membase_reg((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_movb_membase_reg(basereg,disp,reg) \
    do { \
        *(mcodeptr++) = (u1) 0x8a; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_mov_reg_membase(reg,basereg,disp) \
    do { \
        x86_64_emit_rex(1,(basereg),(reg)); \
        *(mcodeptr++) = (u1) 0x89; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_movl_reg_membase(reg,basereg,disp) \
    do { \
        x86_64_emit_rex(0,(basereg),(reg)); \
        *(mcodeptr++) = (u1) 0x89; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_movw_reg_membase(reg,basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0x66; \
        *(mcodeptr++) = (u1) 0x89; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_movb_reg_membase(reg,basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0x88; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_mov_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        x86_64_emit_rex(1,(basereg),(indexreg)); \
        *(mcodeptr++) = (u1) 0x8b; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_movl_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        x86_64_emit_rex(0,(basereg),(indexreg)); \
        *(mcodeptr++) = (u1) 0x8b; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_movw_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        x86_64_emit_rex(0,(basereg),(indexreg)); \
        *(mcodeptr++) = (u1) 0x66; \
        *(mcodeptr++) = (u1) 0x8b; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_movb_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        x86_64_emit_rex(0,(basereg),(indexreg)); \
        *(mcodeptr++) = (u1) 0x8a; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_mov_reg_memindex(reg,disp,basereg,indexreg,scale) \
    do { \
        x86_64_emit_rex(1,(basereg),(indexreg)); \
        *(mcodeptr++) = (u1) 0x89; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_movl_reg_memindex(reg,disp,basereg,indexreg,scale) \
    do { \
        x86_64_emit_rex(0,(basereg),(indexreg)); \
        *(mcodeptr++) = (u1) 0x89; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_movw_reg_memindex(reg,disp,basereg,indexreg,scale) \
    do { \
        x86_64_emit_rex(0,(basereg),(indexreg)); \
        *(mcodeptr++) = (u1) 0x66; \
        *(mcodeptr++) = (u1) 0x89; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_movb_reg_memindex(reg,disp,basereg,indexreg,scale) \
    do { \
        x86_64_emit_rex(1,(basereg),(indexreg)); \
        *(mcodeptr++) = (u1) 0x88; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_mov_imm_membase(imm,basereg,disp) \
    do { \
        x86_64_emit_rex(1,(basereg),0); \
        *(mcodeptr++) = (u1) 0xc7; \
        x86_64_emit_membase((basereg),(disp),0); \
        x86_64_emit_imm32((imm)); \
    } while (0)


#define x86_64_movl_imm_membase(imm,basereg,disp) \
    do { \
        x86_64_emit_rex(0,(basereg),0); \
        *(mcodeptr++) = (u1) 0xc7; \
        x86_64_emit_membase((basereg),(disp),0); \
        x86_64_emit_imm32((imm)); \
    } while (0)


#define x86_64_mov_float_membase(imm,basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xc7; \
        x86_64_emit_membase((basereg),(disp),0); \
        x86_64_emit_float32((imm)); \
    } while (0)


#define x86_64_movsbq_reg_reg(reg,dreg) \
    do { \
        x86_64_emit_rex(1,(dreg),(reg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xbe; \
        /* XXX: why do reg and dreg have to be exchanged */ \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_movsbq_membase_reg(basereg,disp,dreg) \
    do { \
        x86_64_emit_rex(1,(basereg),(dreg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xbe; \
        x86_64_emit_membase((basereg),(disp),(dreg)); \
    } while (0)


#define x86_64_movsbl_reg_reg(reg,dreg) \
    do { \
        x86_64_emit_rex(0,(reg),(dreg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xbe; \
        x86_64_emit_reg((reg),(dreg)); \
    } while (0)


#define x86_64_movswq_reg_reg(reg,dreg) \
    do { \
        x86_64_emit_rex(1,(dreg),(reg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xbf; \
        /* XXX: why do reg and dreg have to be exchanged */ \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_movswq_membase_reg(basereg,disp,dreg) \
    do { \
        x86_64_emit_rex(1,(basereg),(dreg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xbf; \
        x86_64_emit_membase((basereg),(disp),(dreg)); \
    } while (0)


#define x86_64_movswl_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xbf; \
        x86_64_emit_reg((reg),(dreg)); \
    } while (0)


#define x86_64_movslq_reg_reg(reg,dreg) \
    do { \
        x86_64_emit_rex(1,(dreg),(reg)); \
        *(mcodeptr++) = (u1) 0x63; \
        /* XXX: why do reg and dreg have to be exchanged */ \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_movslq_membase_reg(basereg,disp,dreg) \
    do { \
        x86_64_emit_rex(1,(basereg),(dreg)); \
        *(mcodeptr++) = (u1) 0x63; \
        x86_64_emit_membase((basereg),(disp),(dreg)); \
    } while (0)


#define x86_64_movzbl_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xb6; \
        /* XXX: why do reg and dreg have to be exchanged */ \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_movzwq_reg_reg(reg,dreg) \
    do { \
        x86_64_emit_rex(1,(dreg),(reg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xb7; \
        /* XXX: why do reg and dreg have to be exchanged */ \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_movzwq_membase_reg(basereg,disp,dreg) \
    do { \
        x86_64_emit_rex(1,(basereg),(dreg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xb7; \
        x86_64_emit_membase((basereg),(disp),(dreg)); \
    } while (0)


#define x86_64_movzwl_reg_reg(reg,dreg) \
    do { \
        x86_64_emit_rex(0,(dreg),(reg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xb7; \
        /* XXX: why do reg and dreg have to be exchanged */ \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_movswq_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        x86_64_emit_rex(1,(basereg),(reg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xbf; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_movsbq_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        x86_64_emit_rex(1,(basereg),(reg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xbe; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_movzwq_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        x86_64_emit_rex(1,(basereg),(reg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xb7; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_movzbq_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        x86_64_emit_rex(1,(basereg),(reg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xb6; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)



/*
 * alu operations
 */
#define x86_64_alu_reg_reg(opc,reg,dreg) \
    do { \
        x86_64_emit_rex(1,(reg),(dreg)); \
        *(mcodeptr++) = (((u1) (opc)) << 3) + 1; \
        x86_64_emit_reg((reg),(dreg)); \
    } while (0)


#define x86_64_alul_reg_reg(opc,reg,dreg) \
    do { \
        x86_64_emit_rex(0,(reg),(dreg)); \
        *(mcodeptr++) = (((u1) (opc)) << 3) + 1; \
        x86_64_emit_reg((reg),(dreg)); \
    } while (0)


#define x86_64_alu_reg_membase(opc,reg,basereg,disp) \
    do { \
        x86_64_emit_rex(1,(basereg),(reg)); \
        *(mcodeptr++) = (((u1) (opc)) << 3) + 1; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_alul_reg_membase(opc,reg,basereg,disp) \
    do { \
        x86_64_emit_rex(0,(basereg),(reg)); \
        *(mcodeptr++) = (((u1) (opc)) << 3) + 1; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_alu_membase_reg(opc,basereg,disp,reg) \
    do { \
        x86_64_emit_rex(1,(basereg),(reg)); \
        *(mcodeptr++) = (((u1) (opc)) << 3) + 3; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_alul_membase_reg(opc,basereg,disp,reg) \
    do { \
        x86_64_emit_rex(0,(basereg),(reg)); \
        *(mcodeptr++) = (((u1) (opc)) << 3) + 3; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_alu_imm_reg(opc,imm,dreg) \
    do { \
        if (x86_64_is_imm8(imm)) { \
            x86_64_emit_rex(1,0,(dreg)); \
            *(mcodeptr++) = (u1) 0x83; \
            x86_64_emit_reg((opc),(dreg)); \
            x86_64_emit_imm8((imm)); \
        } else { \
            x86_64_emit_rex(1,0,(dreg)); \
            *(mcodeptr++) = (u1) 0x81; \
            x86_64_emit_reg((opc),(dreg)); \
            x86_64_emit_imm32((imm)); \
        } \
    } while (0)


#define x86_64_alul_imm_reg(opc,imm,dreg) \
    do { \
        if (x86_64_is_imm8(imm)) { \
            x86_64_emit_rex(0,0,(dreg)); \
            *(mcodeptr++) = (u1) 0x83; \
            x86_64_emit_reg((opc),(dreg)); \
            x86_64_emit_imm8((imm)); \
        } else { \
            x86_64_emit_rex(0,0,(dreg)); \
            *(mcodeptr++) = (u1) 0x81; \
            x86_64_emit_reg((opc),(dreg)); \
            x86_64_emit_imm32((imm)); \
        } \
    } while (0)


#define x86_64_alu_imm_membase(opc,imm,basereg,disp) \
    do { \
        if (x86_64_is_imm8(imm)) { \
            x86_64_emit_rex(1,(basereg),0); \
            *(mcodeptr++) = (u1) 0x83; \
            x86_64_emit_membase((basereg),(disp),(opc)); \
            x86_64_emit_imm8((imm)); \
        } else { \
            x86_64_emit_rex(1,(basereg),0); \
            *(mcodeptr++) = (u1) 0x81; \
            x86_64_emit_membase((basereg),(disp),(opc)); \
            x86_64_emit_imm32((imm)); \
        } \
    } while (0)


#define x86_64_alul_imm_membase(opc,imm,basereg,disp) \
    do { \
        if (x86_64_is_imm8(imm)) { \
            x86_64_emit_rex(0,(basereg),0); \
            *(mcodeptr++) = (u1) 0x83; \
            x86_64_emit_membase((basereg),(disp),(opc)); \
            x86_64_emit_imm8((imm)); \
        } else { \
            x86_64_emit_rex(0,(basereg),0); \
            *(mcodeptr++) = (u1) 0x81; \
            x86_64_emit_membase((basereg),(disp),(opc)); \
            x86_64_emit_imm32((imm)); \
        } \
    } while (0)


#define x86_64_test_reg_reg(reg,dreg) \
    do { \
        x86_64_emit_rex(1,(reg),(dreg)); \
        *(mcodeptr++) = (u1) 0x85; \
        x86_64_emit_reg((reg),(dreg)); \
    } while (0)


#define x86_64_testl_reg_reg(reg,dreg) \
    do { \
        x86_64_emit_rex(0,(reg),(dreg)); \
        *(mcodeptr++) = (u1) 0x85; \
        x86_64_emit_reg((reg),(dreg)); \
    } while (0)


#define x86_64_test_imm_reg(imm,reg) \
    do { \
        *(mcodeptr++) = (u1) 0xf7; \
        x86_64_emit_reg(0,(reg)); \
        x86_64_emit_imm32((imm)); \
    } while (0)


#define x86_64_testw_imm_reg(imm,reg) \
    do { \
        *(mcodeptr++) = (u1) 0x66; \
        *(mcodeptr++) = (u1) 0xf7; \
        x86_64_emit_reg(0,(reg)); \
        x86_64_emit_imm16((imm)); \
    } while (0)


#define x86_64_testb_imm_reg(imm,reg) \
    do { \
        *(mcodeptr++) = (u1) 0xf6; \
        x86_64_emit_reg(0,(reg)); \
        x86_64_emit_imm8((imm)); \
    } while (0)



/*
 * inc, dec operations
 */
#define x86_64_inc_reg(reg) \
    do { \
        x86_64_emit_rex(1,(reg),0); \
        *(mcodeptr++) = (u1) 0x40 + ((reg) & 0x07); \
    } while (0)


#define x86_64_incl_reg(reg) \
    do { \
        x86_64_emit_rex(0,(reg),0); \
        *(mcodeptr++) = (u1) 0x40 + ((reg) & 0x07); \
    } while (0)


#define x86_64_inc_membase(basereg,disp) \
    do { \
        x86_64_emit_rex(1,(basereg),0); \
        *(mcodeptr++) = (u1) 0xff; \
        x86_64_emit_membase((basereg),(disp),0); \
    } while (0)


#define x86_64_incl_membase(basereg,disp) \
    do { \
        x86_64_emit_rex(0,(basereg),0); \
        *(mcodeptr++) = (u1) 0xff; \
        x86_64_emit_membase((basereg),(disp),0); \
    } while (0)


#define x86_64_dec_reg(reg) \
    x86_64_emit_rex(1,(reg),0); \
    *(mcodeptr++) = (u1) 0x48 + ((reg) & 0x07);

        
#define x86_64_decl_reg(reg) \
    x86_64_emit_rex(0,(reg),0); \
    *(mcodeptr++) = (u1) 0x48 + ((reg) & 0x07);

        
#define x86_64_dec_membase(basereg,disp) \
    do { \
        x86_64_emit_rex(1,(basereg),0); \
        *(mcodeptr++) = (u1) 0xff; \
        x86_64_emit_membase((basereg),(disp),1); \
    } while (0)


#define x86_64_decl_membase(basereg,disp) \
    do { \
        x86_64_emit_rex(0,(basereg),0); \
        *(mcodeptr++) = (u1) 0xff; \
        x86_64_emit_membase((basereg),(disp),1); \
    } while (0)




#define x86_64_cltd() \
    *(mcodeptr++) = (u1) 0x99;


#define x86_64_cqto() \
    x86_64_emit_rex(1,0,0); \
    *(mcodeptr++) = (u1) 0x99;



#define x86_64_imul_reg_reg(reg,dreg) \
    do { \
        x86_64_emit_rex(1,(reg),(dreg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xaf; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_imull_reg_reg(reg,dreg) \
    do { \
        x86_64_emit_rex(0,(reg),(dreg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xaf; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_imul_membase_reg(basereg,disp,dreg) \
    do { \
        x86_64_emit_rex(1,(basereg),(dreg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xaf; \
        x86_64_emit_membase((basereg),(disp),(dreg)); \
    } while (0)


#define x86_64_imull_membase_reg(basereg,disp,dreg) \
    do { \
        x86_64_emit_rex(0,(basereg),(dreg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xaf; \
        x86_64_emit_membase((basereg),(disp),(dreg)); \
    } while (0)


#define x86_64_imul_imm_reg(imm,dreg) \
    do { \
        if (x86_64_is_imm8((imm))) { \
            x86_64_emit_rex(1,0,(dreg)); \
            *(mcodeptr++) = (u1) 0x6b; \
            x86_64_emit_reg(0,(dreg)); \
            x86_64_emit_imm8((imm)); \
        } else { \
            x86_64_emit_rex(1,0,(dreg)); \
            *(mcodeptr++) = (u1) 0x69; \
            x86_64_emit_reg(0,(dreg)); \
            x86_64_emit_imm32((imm)); \
        } \
    } while (0)


#define x86_64_imul_imm_reg_reg(imm,reg,dreg) \
    do { \
        if (x86_64_is_imm8((imm))) { \
            x86_64_emit_rex(1,(reg),(dreg)); \
            *(mcodeptr++) = (u1) 0x6b; \
            x86_64_emit_reg((dreg),(reg)); \
            x86_64_emit_imm8((imm)); \
        } else { \
            x86_64_emit_rex(1,(reg),(dreg)); \
            *(mcodeptr++) = (u1) 0x69; \
            x86_64_emit_reg((dreg),(reg)); \
            x86_64_emit_imm32((imm)); \
        } \
    } while (0)


#define x86_64_imull_imm_reg_reg(imm,reg,dreg) \
    do { \
        if (x86_64_is_imm8((imm))) { \
            x86_64_emit_rex(0,(reg),(dreg)); \
            *(mcodeptr++) = (u1) 0x6b; \
            x86_64_emit_reg((dreg),(reg)); \
            x86_64_emit_imm8((imm)); \
        } else { \
            x86_64_emit_rex(0,(reg),(dreg)); \
            *(mcodeptr++) = (u1) 0x69; \
            x86_64_emit_reg((dreg),(reg)); \
            x86_64_emit_imm32((imm)); \
        } \
    } while (0)


#define x86_64_imul_imm_membase_reg(imm,basereg,disp,dreg) \
    do { \
        if (x86_64_is_imm8((imm))) { \
            x86_64_emit_rex(1,(basereg),(dreg)); \
            *(mcodeptr++) = (u1) 0x6b; \
            x86_64_emit_membase((basereg),(disp),(dreg)); \
            x86_64_emit_imm8((imm)); \
        } else { \
            x86_64_emit_rex(1,(basereg),(dreg)); \
            *(mcodeptr++) = (u1) 0x69; \
            x86_64_emit_membase((basereg),(disp),(dreg)); \
            x86_64_emit_imm32((imm)); \
        } \
    } while (0)


#define x86_64_imull_imm_membase_reg(imm,basereg,disp,dreg) \
    do { \
        if (x86_64_is_imm8((imm))) { \
            x86_64_emit_rex(0,(basereg),(dreg)); \
            *(mcodeptr++) = (u1) 0x6b; \
            x86_64_emit_membase((basereg),(disp),(dreg)); \
            x86_64_emit_imm8((imm)); \
        } else { \
            x86_64_emit_rex(0,(basereg),(dreg)); \
            *(mcodeptr++) = (u1) 0x69; \
            x86_64_emit_membase((basereg),(disp),(dreg)); \
            x86_64_emit_imm32((imm)); \
        } \
    } while (0)


#define x86_64_idiv_reg(reg) \
    do { \
        x86_64_emit_rex(1,(reg),0); \
        *(mcodeptr++) = (u1) 0xf7; \
        x86_64_emit_reg(7,(reg)); \
    } while (0)


#define x86_64_idivl_reg(reg) \
    do { \
        x86_64_emit_rex(0,(reg),0); \
        *(mcodeptr++) = (u1) 0xf7; \
        x86_64_emit_reg(7,(reg)); \
    } while (0)



#define x86_64_ret() \
    *(mcodeptr++) = (u1) 0xc3;


#define x86_64_leave() \
    *(mcodeptr++) = (u1) 0xc9;



/*
 * shift ops
 */
#define x86_64_shift_reg(opc,reg) \
    do { \
        x86_64_emit_rex(1,(reg),0); \
        *(mcodeptr++) = (u1) 0xd3; \
        x86_64_emit_reg((opc),(reg)); \
    } while (0)


#define x86_64_shiftl_reg(opc,reg) \
    do { \
        x86_64_emit_rex(0,(reg),0); \
        *(mcodeptr++) = (u1) 0xd3; \
        x86_64_emit_reg((opc),(reg)); \
    } while (0)


#define x86_64_shift_membase(opc,basereg,disp) \
    do { \
        x86_64_emit_rex(1,(basereg),0); \
        *(mcodeptr++) = (u1) 0xd3; \
        x86_64_emit_membase((basereg),(disp),(opc)); \
    } while (0)


#define x86_64_shiftl_membase(opc,basereg,disp) \
    do { \
        x86_64_emit_rex(0,(basereg),0); \
        *(mcodeptr++) = (u1) 0xd3; \
        x86_64_emit_membase((basereg),(disp),(opc)); \
    } while (0)


#define x86_64_shift_imm_reg(opc,imm,dreg) \
    do { \
        if ((imm) == 1) { \
            x86_64_emit_rex(1,0,(dreg)); \
            *(mcodeptr++) = (u1) 0xd1; \
            x86_64_emit_reg((opc),(dreg)); \
        } else { \
            x86_64_emit_rex(1,0,(dreg)); \
            *(mcodeptr++) = (u1) 0xc1; \
            x86_64_emit_reg((opc),(dreg)); \
            x86_64_emit_imm8((imm)); \
        } \
    } while (0)


#define x86_64_shiftl_imm_reg(opc,imm,dreg) \
    do { \
        if ((imm) == 1) { \
            x86_64_emit_rex(0,0,(dreg)); \
            *(mcodeptr++) = (u1) 0xd1; \
            x86_64_emit_reg((opc),(dreg)); \
        } else { \
            x86_64_emit_rex(0,0,(dreg)); \
            *(mcodeptr++) = (u1) 0xc1; \
            x86_64_emit_reg((opc),(dreg)); \
            x86_64_emit_imm8((imm)); \
        } \
    } while (0)


#define x86_64_shift_imm_membase(opc,imm,basereg,disp) \
    do { \
        if ((imm) == 1) { \
            x86_64_emit_rex(1,(basereg),0); \
            *(mcodeptr++) = (u1) 0xd1; \
            x86_64_emit_membase((basereg),(disp),(opc)); \
        } else { \
            x86_64_emit_rex(1,(basereg),0); \
            *(mcodeptr++) = (u1) 0xc1; \
            x86_64_emit_membase((basereg),(disp),(opc)); \
            x86_64_emit_imm8((imm)); \
        } \
    } while (0)


#define x86_64_shiftl_imm_membase(opc,imm,basereg,disp) \
    do { \
        if ((imm) == 1) { \
            x86_64_emit_rex(0,(basereg),0); \
            *(mcodeptr++) = (u1) 0xd1; \
            x86_64_emit_membase((basereg),(disp),(opc)); \
        } else { \
            x86_64_emit_rex(0,(basereg),0); \
            *(mcodeptr++) = (u1) 0xc1; \
            x86_64_emit_membase((basereg),(disp),(opc)); \
            x86_64_emit_imm8((imm)); \
        } \
    } while (0)


#define x86_64_shld_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xa5; \
        x86_64_emit_reg((reg),(dreg)); \
    } while (0)


#define x86_64_shld_imm_reg_reg(imm,reg,dreg) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xa4; \
        x86_64_emit_reg((reg),(dreg)); \
        x86_64_emit_imm8((imm)); \
    } while (0)


#define x86_64_shld_reg_membase(reg,basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xa5; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_shrd_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xad; \
        x86_64_emit_reg((reg),(dreg)); \
    } while (0)


#define x86_64_shrd_imm_reg_reg(imm,reg,dreg) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xac; \
        x86_64_emit_reg((reg),(dreg)); \
        x86_64_emit_imm8((imm)); \
    } while (0)


#define x86_64_shrd_reg_membase(reg,basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xad; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)



/*
 * jump operations
 */
#define x86_64_jmp_imm(imm) \
    do { \
        *(mcodeptr++) = (u1) 0xe9; \
        x86_64_emit_imm32((imm)); \
    } while (0)


#define x86_64_jmp_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xff; \
        x86_64_emit_reg(4,(reg)); \
    } while (0)


#define x86_64_jcc(opc,imm) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) (0x80 + x86_64_jcc_map[(opc)]); \
        x86_64_emit_imm32((imm)); \
    } while (0)



/*
 * conditional set operations
 */
#define x86_64_setcc_reg(opc,reg) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) (0x90 + x86_64_jcc_map[(opc)]); \
        x86_64_emit_reg(0,(reg)); \
    } while (0)


#define x86_64_setcc_membase(opc,basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) (0x90 + x86_64_jcc_map[(opc)]); \
        x86_64_emit_membase((basereg),(disp),0); \
    } while (0)



#define x86_64_neg_reg(reg) \
    do { \
        x86_64_emit_rex(1,(reg),0); \
        *(mcodeptr++) = (u1) 0xf7; \
        x86_64_emit_reg(3,(reg)); \
    } while (0)


#define x86_64_negl_reg(reg) \
    do { \
        x86_64_emit_rex(0,(reg),0); \
        *(mcodeptr++) = (u1) 0xf7; \
        x86_64_emit_reg(3,(reg)); \
    } while (0)


#define x86_64_neg_membase(basereg,disp) \
    do { \
        x86_64_emit_rex(1,(basereg),0); \
        *(mcodeptr++) = (u1) 0xf7; \
        x86_64_emit_membase((basereg),(disp),3); \
    } while (0)


#define x86_64_negl_membase(basereg,disp) \
    do { \
        x86_64_emit_rex(0,(basereg),0); \
        *(mcodeptr++) = (u1) 0xf7; \
        x86_64_emit_membase((basereg),(disp),3); \
    } while (0)



#define x86_64_push_reg(reg) \
    *(mcodeptr++) = (u1) 0x50 + (0x07 & (reg));


#define x86_64_push_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xff; \
        x86_64_emit_membase((basereg),(disp),6); \
    } while (0)


#define x86_64_push_imm(imm) \
    do { \
        *(mcodeptr++) = (u1) 0x68; \
        x86_64_emit_imm32((imm)); \
    } while (0)


#define x86_64_pop_reg(reg) \
    *(mcodeptr++) = (u1) 0x58 + (0x07 & (reg));


#define x86_64_nop() \
    *(mcodeptr++) = (u1) 0x90;


#define x86_64_hlt() \
    *(mcodeptr++) = 0xf4;



/*
 * call instructions
 */
#define x86_64_call_reg(reg) \
    do { \
        x86_64_emit_rex(1,(reg),0); \
        *(mcodeptr++) = (u1) 0xff; \
        x86_64_emit_reg(2,(reg)); \
    } while (0)


#define x86_64_call_imm(imm) \
    do { \
        *(mcodeptr++) = (u1) 0xe8; \
        x86_64_emit_imm32((imm)); \
    } while (0)



/*
 * floating point instructions
 */
#define x86_64_fld1() \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        *(mcodeptr++) = (u1) 0xe8; \
    } while (0)


#define x86_64_fldz() \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        *(mcodeptr++) = (u1) 0xee; \
    } while (0)


#define x86_64_fld_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        *(mcodeptr++) = (u1) 0xc0 + (0x07 & (reg)); \
    } while (0)


#define x86_64_flds_mem(mem) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        x86_64_emit_mem(0,(mem)); \
    } while (0)


#define x86_64_fldl_mem(mem) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        x86_64_emit_mem(0,(mem)); \
    } while (0)


#define x86_64_fldt_mem(mem) \
    do { \
        *(mcodeptr++) = (u1) 0xdb; \
        x86_64_emit_mem(5,(mem)); \
    } while (0)


#define x86_64_flds_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        x86_64_emit_membase((basereg),(disp),0); \
    } while (0)


#define x86_64_fldl_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        x86_64_emit_membase((basereg),(disp),0); \
    } while (0)


#define x86_64_fldt_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdb; \
        x86_64_emit_membase((basereg),(disp),5); \
    } while (0)


#define x86_64_flds_memindex(disp,basereg,indexreg,scale) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        x86_64_emit_memindex(0,(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_fldl_memindex(disp,basereg,indexreg,scale) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        x86_64_emit_memindex(0,(disp),(basereg),(indexreg),(scale)); \
    } while (0)




#define x86_64_fildl_mem(mem) \
    do { \
        *(mcodeptr++) = (u1) 0xdb; \
        x86_64_emit_mem(0,(mem)); \
    } while (0)


#define x86_64_fildl_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdb; \
        x86_64_emit_membase((basereg),(disp),0); \
    } while (0)


#define x86_64_fildll_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdf; \
        x86_64_emit_membase((basereg),(disp),5); \
    } while (0)




#define x86_64_fst_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        *(mcodeptr++) = (u1) 0xd0 + (0x07 & (reg)); \
    } while (0)


#define x86_64_fsts_mem(mem) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        x86_64_emit_mem(2,(mem)); \
    } while (0)


#define x86_64_fstl_mem(mem) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        x86_64_emit_mem(2,(mem)); \
    } while (0)


#define x86_64_fsts_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        x86_64_emit_membase((basereg),(disp),2); \
    } while (0)


#define x86_64_fstl_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        x86_64_emit_membase((basereg),(disp),2); \
    } while (0)


#define x86_64_fsts_memindex(disp,basereg,indexreg,scale) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        x86_64_emit_memindex(2,(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_fstl_memindex(disp,basereg,indexreg,scale) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        x86_64_emit_memindex(2,(disp),(basereg),(indexreg),(scale)); \
    } while (0)




#define x86_64_fstp_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        *(mcodeptr++) = (u1) 0xd8 + (0x07 & (reg)); \
    } while (0)


#define x86_64_fstps_mem(mem) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        x86_64_emit_mem(3,(mem)); \
    } while (0)


#define x86_64_fstpl_mem(mem) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        x86_64_emit_mem(3,(mem)); \
    } while (0)


#define x86_64_fstps_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        x86_64_emit_membase((basereg),(disp),3); \
    } while (0)


#define x86_64_fstpl_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        x86_64_emit_membase((basereg),(disp),3); \
    } while (0)


#define x86_64_fstpt_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdb; \
        x86_64_emit_membase((basereg),(disp),7); \
    } while (0)


#define x86_64_fstps_memindex(disp,basereg,indexreg,scale) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        x86_64_emit_memindex(3,(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_fstpl_memindex(disp,basereg,indexreg,scale) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        x86_64_emit_memindex(3,(disp),(basereg),(indexreg),(scale)); \
    } while (0)




#define x86_64_fistpl_mem(mem) \
    do { \
        *(mcodeptr++) = (u1) 0xdb; \
        x86_64_emit_mem(3,(mem)); \
    } while (0)


#define x86_64_fistpll_mem(mem) \
    do { \
        *(mcodeptr++) = (u1) 0xdf; \
        x86_64_emit_mem(7,(mem)); \
    } while (0)


#define x86_64_fistl_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdb; \
        x86_64_emit_membase((basereg),(disp),2); \
    } while (0)


#define x86_64_fistpl_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdb; \
        x86_64_emit_membase((basereg),(disp),3); \
    } while (0)


#define x86_64_fistpll_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdf; \
        x86_64_emit_membase((basereg),(disp),7); \
    } while (0)




#define x86_64_fchs() \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        *(mcodeptr++) = (u1) 0xe0; \
    } while (0)


#define x86_64_faddp() \
    do { \
        *(mcodeptr++) = (u1) 0xde; \
        *(mcodeptr++) = (u1) 0xc1; \
    } while (0)


#define x86_64_fadd_reg_st(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xd8; \
        *(mcodeptr++) = (u1) 0xc0 + (0x0f & (reg)); \
    } while (0)


#define x86_64_fadd_st_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xdc; \
        *(mcodeptr++) = (u1) 0xc0 + (0x0f & (reg)); \
    } while (0)


#define x86_64_faddp_st_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xde; \
        *(mcodeptr++) = (u1) 0xc0 + (0x0f & (reg)); \
    } while (0)


#define x86_64_fadds_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xd8; \
        x86_64_emit_membase((basereg),(disp),0); \
    } while (0)


#define x86_64_faddl_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdc; \
        x86_64_emit_membase((basereg),(disp),0); \
    } while (0)


#define x86_64_fsub_reg_st(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xd8; \
        *(mcodeptr++) = (u1) 0xe0 + (0x07 & (reg)); \
    } while (0)


#define x86_64_fsub_st_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xdc; \
        *(mcodeptr++) = (u1) 0xe8 + (0x07 & (reg)); \
    } while (0)


#define x86_64_fsubp_st_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xde; \
        *(mcodeptr++) = (u1) 0xe8 + (0x07 & (reg)); \
    } while (0)


#define x86_64_fsubp() \
    do { \
        *(mcodeptr++) = (u1) 0xde; \
        *(mcodeptr++) = (u1) 0xe9; \
    } while (0)


#define x86_64_fsubs_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xd8; \
        x86_64_emit_membase((basereg),(disp),4); \
    } while (0)


#define x86_64_fsubl_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdc; \
        x86_64_emit_membase((basereg),(disp),4); \
    } while (0)


#define x86_64_fmul_reg_st(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xd8; \
        *(mcodeptr++) = (u1) 0xc8 + (0x07 & (reg)); \
    } while (0)


#define x86_64_fmul_st_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xdc; \
        *(mcodeptr++) = (u1) 0xc8 + (0x07 & (reg)); \
    } while (0)


#define x86_64_fmulp() \
    do { \
        *(mcodeptr++) = (u1) 0xde; \
        *(mcodeptr++) = (u1) 0xc9; \
    } while (0)


#define x86_64_fmulp_st_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xde; \
        *(mcodeptr++) = (u1) 0xc8 + (0x07 & (reg)); \
    } while (0)


#define x86_64_fmuls_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xd8; \
        x86_64_emit_membase((basereg),(disp),1); \
    } while (0)


#define x86_64_fmull_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xdc; \
        x86_64_emit_membase((basereg),(disp),1); \
    } while (0)


#define x86_64_fdiv_reg_st(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xd8; \
        *(mcodeptr++) = (u1) 0xf0 + (0x07 & (reg)); \
    } while (0)


#define x86_64_fdiv_st_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xdc; \
        *(mcodeptr++) = (u1) 0xf8 + (0x07 & (reg)); \
    } while (0)


#define x86_64_fdivp() \
    do { \
        *(mcodeptr++) = (u1) 0xde; \
        *(mcodeptr++) = (u1) 0xf9; \
    } while (0)


#define x86_64_fdivp_st_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xde; \
        *(mcodeptr++) = (u1) 0xf8 + (0x07 & (reg)); \
    } while (0)


#define x86_64_fxch() \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        *(mcodeptr++) = (u1) 0xc9; \
    } while (0)


#define x86_64_fxch_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        *(mcodeptr++) = (u1) 0xc8 + (0x07 & (reg)); \
    } while (0)


#define x86_64_fprem() \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        *(mcodeptr++) = (u1) 0xf8; \
    } while (0)


#define x86_64_fprem1() \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        *(mcodeptr++) = (u1) 0xf5; \
    } while (0)


#define x86_64_fucom() \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        *(mcodeptr++) = (u1) 0xe1; \
    } while (0)


#define x86_64_fucom_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        *(mcodeptr++) = (u1) 0xe0 + (0x07 & (reg)); \
    } while (0)


#define x86_64_fucomp_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        *(mcodeptr++) = (u1) 0xe8 + (0x07 & (reg)); \
    } while (0)


#define x86_64_fucompp() \
    do { \
        *(mcodeptr++) = (u1) 0xda; \
        *(mcodeptr++) = (u1) 0xe9; \
    } while (0)


#define x86_64_fnstsw() \
    do { \
        *(mcodeptr++) = (u1) 0xdf; \
        *(mcodeptr++) = (u1) 0xe0; \
    } while (0)


#define x86_64_sahf() \
    *(mcodeptr++) = (u1) 0x9e;


#define x86_64_finit() \
    do { \
        *(mcodeptr++) = (u1) 0x9b; \
        *(mcodeptr++) = (u1) 0xdb; \
        *(mcodeptr++) = (u1) 0xe3; \
    } while (0)


#define x86_64_fldcw_mem(mem) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        x86_64_emit_mem(5,(mem)); \
    } while (0)


#define x86_64_fldcw_membase(basereg,disp) \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        x86_64_emit_membase((basereg),(disp),5); \
    } while (0)


#define x86_64_wait() \
    *(mcodeptr++) = (u1) 0x9b;


#define x86_64_ffree_reg(reg) \
    do { \
        *(mcodeptr++) = (u1) 0xdd; \
        *(mcodeptr++) = (u1) 0xc0 + (0x07 & (reg)); \
    } while (0)


#define x86_64_fdecstp() \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        *(mcodeptr++) = (u1) 0xf6; \
    } while (0)


#define x86_64_fincstp() \
    do { \
        *(mcodeptr++) = (u1) 0xd9; \
        *(mcodeptr++) = (u1) 0xf7; \
    } while (0)



/* function gen_resolvebranch **************************************************

    backpatches a branch instruction

    parameters: ip ... pointer to instruction after branch (void*)
                so ... offset of instruction after branch  (s8)
                to ... offset of branch target             (s8)

*******************************************************************************/

#define gen_resolvebranch(ip,so,to) \
    *((s4*) ((ip) - 4)) = (s4) ((to) - (so));

#define SOFTNULLPTRCHECK       /* soft null pointer check supportet as option */

#endif
