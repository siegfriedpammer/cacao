/* x86_64/ngen.h ***************************************************************

    Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

    See file COPYRIGHT for information on usage and disclaimer of warranties

    Contains the machine dependent code generator definitions and macros for an
    x86_64 processor.

    Authors: Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at
             Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
             Christian Thalinger EMAIL: cacao@complang.tuwien.ac.at

    Last Change: $Id: ngen.h 465 2003-09-17 23:14:48Z twisti $

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
#define REG_ITMP2       R10      /* temporary register and method pointer     */
#define REG_ITMP3       R11      /* temporary register                        */

#define REG_ITMP1_XPTR  RAX      /* exception pointer = temporary register 1  */
#define REG_ITMP2_XPC   R10      /* exception pc = temporary register 2       */

#define REG_SP          RSP      /* stack pointer                             */

/* floating point registers */

#define REG_FRESULT     XMM0     /* to deliver floating point method results  */

#define REG_FTMP1       XMM8     /* temporary floating point register         */
#define REG_FTMP2       XMM9     /* temporary floating point register         */
#define REG_FTMP3       XMM10    /* temporary floating point register         */

/* register descripton - array ************************************************/

/* #define REG_RES   0         reserved register for OS or code generator     */
/* #define REG_RET   1         return value register                          */
/* #define REG_EXC   2         exception value register (only old jit)        */
/* #define REG_SAV   3         (callee) saved register                        */
/* #define REG_TMP   4         scratch temporary register (caller saved)      */
/* #define REG_ARG   5         argument register (caller saved)               */

/* #define REG_END   -1        last entry in tables */

int nregdescint[] = {
    REG_RET, REG_ARG, REG_ARG, REG_TMP, REG_RES, REG_SAV, REG_ARG, REG_ARG,
    REG_ARG, REG_ARG, REG_RES, REG_RES, REG_SAV, REG_SAV, REG_SAV, REG_SAV,
    REG_END
};

/* for use of reserved registers, see comment above */

int nregdescfloat[] = {
    REG_ARG, REG_ARG, REG_ARG, REG_ARG, REG_TMP, REG_TMP, REG_TMP, REG_TMP,
    REG_RES, REG_RES, REG_RES, REG_SAV, REG_SAV, REG_SAV, REG_SAV, REG_SAV,
    REG_END
};

/* for use of reserved registers, see comment above */


/* stackframe-infos ***********************************************************/

int parentargs_base; /* offset in stackframe for the parameter from the caller*/


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
} x86_64_imm_buf;


/*
 * x86_64 register numbers
 */
typedef enum {
    RIP = -1,
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
    XMM0 = 0,
    XMM1 = 1,
    XMM2 = 2,
    XMM3 = 3,
    XMM4 = 4,
    XMM5 = 5,
    XMM6 = 6,
    XMM7 = 7,
    XMM8 = 8,
    XMM9 = 9,
    XMM10 = 10,
    XMM11 = 11,
    XMM12 = 12,
    XMM13 = 13,
    XMM14 = 14,
    XMM15 = 15,
    NREGF
} X86_64_RegF_No;


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

static const unsigned char x86_64_cc_map[] = {
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
#define x86_64_address_byte(mod,reg,rm) \
    *(mcodeptr++) = ((((mod) & 0x03) << 6) | (((reg) & 0x07) << 3) | ((rm) & 0x07));


#define x86_64_emit_reg(reg,rm) \
    x86_64_address_byte(3,(reg),(rm));


#define x86_64_emit_rex(size,reg,index,rm) \
    if ((size) == 1 || (reg) > 7 || (index) > 7 || (rm) > 7) { \
        *(mcodeptr++) = (0x40 | (((size) & 0x01) << 3) | ((((reg) >> 3) & 0x01) << 2) | ((((index) >> 3) & 0x01) << 1) | (((rm) >> 3) & 0x01)); \
    }


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
            } else if (x86_64_is_imm8((disp))) { \
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
        if (x86_64_is_imm8((disp))) { \
            x86_64_address_byte(1,(dreg),(basereg)); \
            x86_64_emit_imm8((disp)); \
        } else { \
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
        } else if (x86_64_is_imm8((disp))) { \
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


#define x86_64_is_imm8(imm) \
    (((long)(imm) >= -128 && (long)(imm) <= 127))


#define x86_64_is_imm32(imm) \
    ((long)(imm) >= (-2147483647-1) && (long)(imm) <= 2147483647)


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


#define x86_64_emit_fimm32(imm) \
    do { \
        x86_64_imm_buf imb; \
        imb.f = (float) (imm); \
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


#define x86_64_emit_fimm64(imm) \
    do { \
        x86_64_imm_buf imb; \
        imb.d = (double) (imm); \
        *(mcodeptr++) = imb.b[0]; \
        *(mcodeptr++) = imb.b[1]; \
        *(mcodeptr++) = imb.b[2]; \
        *(mcodeptr++) = imb.b[3]; \
        *(mcodeptr++) = imb.b[4]; \
        *(mcodeptr++) = imb.b[5]; \
        *(mcodeptr++) = imb.b[6]; \
        *(mcodeptr++) = imb.b[7]; \
    } while (0)



/*
 * mov ops
 */
#define x86_64_mov_reg_reg(reg,dreg) \
    do { \
        x86_64_emit_rex(1,(reg),0,(dreg)); \
        *(mcodeptr++) = (u1) 0x89; \
        x86_64_emit_reg((reg),(dreg)); \
    } while (0)


#define x86_64_mov_imm_reg(imm,reg) \
    do { \
        x86_64_emit_rex(1,0,0,(reg)); \
        *(mcodeptr++) = (u1) 0xb8 + ((reg) & 0x07); \
        x86_64_emit_imm64((imm)); \
    } while (0)


#define x86_64_mov_fimm_reg(imm,reg) \
    do { \
        x86_64_emit_rex(1,0,0,(reg)); \
        *(mcodeptr++) = (u1) 0xb8 + ((reg) & 0x07); \
        x86_64_emit_fimm64((imm)); \
    } while (0)


#define x86_64_movl_imm_reg(imm,reg) \
    do { \
        x86_64_emit_rex(0,0,0,(reg)); \
        *(mcodeptr++) = (u1) 0xb8 + ((reg) & 0x07); \
        x86_64_emit_imm32((imm)); \
    } while (0)


#define x86_64_movl_fimm_reg(imm,reg) \
    do { \
        x86_64_emit_rex(0,0,0,(reg)); \
        *(mcodeptr++) = (u1) 0xb8 + ((reg) & 0x07); \
        x86_64_emit_fimm32((imm)); \
    } while (0)


#define x86_64_mov_membase_reg(basereg,disp,reg) \
    do { \
        x86_64_emit_rex(1,(reg),0,(basereg)); \
        *(mcodeptr++) = (u1) 0x8b; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_movl_membase_reg(basereg,disp,reg) \
    do { \
        x86_64_emit_rex(0,(reg),0,(basereg)); \
        *(mcodeptr++) = (u1) 0x8b; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


/*
 * this one is for INVOKEVIRTUAL/INVOKEINTERFACE to have a
 * constant membase immediate length of 32bit
 */
#define x86_64_mov_membase32_reg(basereg,disp,reg) \
    do { \
        x86_64_emit_rex(1,(reg),0,(basereg)); \
        *(mcodeptr++) = (u1) 0x8b; \
        x86_64_address_byte(2, (reg), (basereg)); \
        x86_64_emit_imm32((disp)); \
    } while (0)


#define x86_64_mov_reg_membase(reg,basereg,disp) \
    do { \
        x86_64_emit_rex(1,(reg),0,(basereg)); \
        *(mcodeptr++) = (u1) 0x89; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_movl_reg_membase(reg,basereg,disp) \
    do { \
        x86_64_emit_rex(0,(reg),0,(basereg)); \
        *(mcodeptr++) = (u1) 0x89; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_mov_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        x86_64_emit_rex(1,(reg),(indexreg),(basereg)); \
        *(mcodeptr++) = (u1) 0x8b; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_movl_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        x86_64_emit_rex(0,(reg),(indexreg),(basereg)); \
        *(mcodeptr++) = (u1) 0x8b; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_movw_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        *(mcodeptr++) = (u1) 0x66; \
        x86_64_emit_rex(0,(reg),(indexreg),(basereg)); \
        *(mcodeptr++) = (u1) 0x8b; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_movb_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        x86_64_emit_rex(0,(reg),(indexreg),(basereg)); \
        *(mcodeptr++) = (u1) 0x8a; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_mov_reg_memindex(reg,disp,basereg,indexreg,scale) \
    do { \
        x86_64_emit_rex(1,(reg),(indexreg),(basereg)); \
        *(mcodeptr++) = (u1) 0x89; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_movl_reg_memindex(reg,disp,basereg,indexreg,scale) \
    do { \
        x86_64_emit_rex(0,(reg),(indexreg),(basereg)); \
        *(mcodeptr++) = (u1) 0x89; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_movw_reg_memindex(reg,disp,basereg,indexreg,scale) \
    do { \
        *(mcodeptr++) = (u1) 0x66; \
        x86_64_emit_rex(0,(reg),(indexreg),(basereg)); \
        *(mcodeptr++) = (u1) 0x89; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_movb_reg_memindex(reg,disp,basereg,indexreg,scale) \
    do { \
        x86_64_emit_rex(0,(reg),(indexreg),(basereg)); \
        *(mcodeptr++) = (u1) 0x88; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_mov_imm_membase(imm,basereg,disp) \
    do { \
        x86_64_emit_rex(1,0,0,(basereg)); \
        *(mcodeptr++) = (u1) 0xc7; \
        x86_64_emit_membase((basereg),(disp),0); \
        x86_64_emit_imm32((imm)); \
    } while (0)


#define x86_64_movl_imm_membase(imm,basereg,disp) \
    do { \
        x86_64_emit_rex(0,0,0,(basereg)); \
        *(mcodeptr++) = (u1) 0xc7; \
        x86_64_emit_membase((basereg),(disp),0); \
        x86_64_emit_imm32((imm)); \
    } while (0)


#define x86_64_movsbq_reg_reg(reg,dreg) \
    do { \
        x86_64_emit_rex(1,(dreg),0,(reg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xbe; \
        /* XXX: why do reg and dreg have to be exchanged */ \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_movsbq_membase_reg(basereg,disp,dreg) \
    do { \
        x86_64_emit_rex(1,(dreg),0,(basereg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xbe; \
        x86_64_emit_membase((basereg),(disp),(dreg)); \
    } while (0)


#define x86_64_movsbl_reg_reg(reg,dreg) \
    do { \
        x86_64_emit_rex(0,(reg),0,(dreg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xbe; \
        x86_64_emit_reg((reg),(dreg)); \
    } while (0)


#define x86_64_movswq_reg_reg(reg,dreg) \
    do { \
        x86_64_emit_rex(1,(dreg),0,(reg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xbf; \
        /* XXX: why do reg and dreg have to be exchanged */ \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_movswq_membase_reg(basereg,disp,dreg) \
    do { \
        x86_64_emit_rex(1,(dreg),0,(basereg)); \
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
        x86_64_emit_rex(1,(dreg),0,(reg)); \
        *(mcodeptr++) = (u1) 0x63; \
        /* XXX: why do reg and dreg have to be exchanged */ \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_movslq_membase_reg(basereg,disp,dreg) \
    do { \
        x86_64_emit_rex(1,(dreg),0,(basereg)); \
        *(mcodeptr++) = (u1) 0x63; \
        x86_64_emit_membase((basereg),(disp),(dreg)); \
    } while (0)


#define x86_64_movzwq_reg_reg(reg,dreg) \
    do { \
        x86_64_emit_rex(1,(dreg),0,(reg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xb7; \
        /* XXX: why do reg and dreg have to be exchanged */ \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_movzwq_membase_reg(basereg,disp,dreg) \
    do { \
        x86_64_emit_rex(1,(dreg),0,(basereg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xb7; \
        x86_64_emit_membase((basereg),(disp),(dreg)); \
    } while (0)


#define x86_64_movswq_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        x86_64_emit_rex(1,(reg),(indexreg),(basereg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xbf; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_movsbq_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        x86_64_emit_rex(1,(reg),(indexreg),(basereg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xbe; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_movzwq_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        x86_64_emit_rex(1,(reg),(indexreg),(basereg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xb7; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_movzbq_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        x86_64_emit_rex(1,(reg),(indexreg),(basereg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xb6; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)



/*
 * alu operations
 */
#define x86_64_alu_reg_reg(opc,reg,dreg) \
    do { \
        x86_64_emit_rex(1,(reg),0,(dreg)); \
        *(mcodeptr++) = (((u1) (opc)) << 3) + 1; \
        x86_64_emit_reg((reg),(dreg)); \
    } while (0)


#define x86_64_alul_reg_reg(opc,reg,dreg) \
    do { \
        x86_64_emit_rex(0,(reg),0,(dreg)); \
        *(mcodeptr++) = (((u1) (opc)) << 3) + 1; \
        x86_64_emit_reg((reg),(dreg)); \
    } while (0)


#define x86_64_alu_reg_membase(opc,reg,basereg,disp) \
    do { \
        x86_64_emit_rex(1,(reg),0,(basereg)); \
        *(mcodeptr++) = (((u1) (opc)) << 3) + 1; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_alul_reg_membase(opc,reg,basereg,disp) \
    do { \
        x86_64_emit_rex(0,(reg),0,(basereg)); \
        *(mcodeptr++) = (((u1) (opc)) << 3) + 1; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_alu_membase_reg(opc,basereg,disp,reg) \
    do { \
        x86_64_emit_rex(1,(reg),0,(basereg)); \
        *(mcodeptr++) = (((u1) (opc)) << 3) + 3; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_alul_membase_reg(opc,basereg,disp,reg) \
    do { \
        x86_64_emit_rex(0,(reg),0,(basereg)); \
        *(mcodeptr++) = (((u1) (opc)) << 3) + 3; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_alu_imm_reg(opc,imm,dreg) \
    do { \
        if (x86_64_is_imm8(imm)) { \
            x86_64_emit_rex(1,0,0,(dreg)); \
            *(mcodeptr++) = (u1) 0x83; \
            x86_64_emit_reg((opc),(dreg)); \
            x86_64_emit_imm8((imm)); \
        } else { \
            x86_64_emit_rex(1,0,0,(dreg)); \
            *(mcodeptr++) = (u1) 0x81; \
            x86_64_emit_reg((opc),(dreg)); \
            x86_64_emit_imm32((imm)); \
        } \
    } while (0)


#define x86_64_alul_imm_reg(opc,imm,dreg) \
    do { \
        if (x86_64_is_imm8(imm)) { \
            x86_64_emit_rex(0,0,0,(dreg)); \
            *(mcodeptr++) = (u1) 0x83; \
            x86_64_emit_reg((opc),(dreg)); \
            x86_64_emit_imm8((imm)); \
        } else { \
            x86_64_emit_rex(0,0,0,(dreg)); \
            *(mcodeptr++) = (u1) 0x81; \
            x86_64_emit_reg((opc),(dreg)); \
            x86_64_emit_imm32((imm)); \
        } \
    } while (0)


#define x86_64_alu_imm_membase(opc,imm,basereg,disp) \
    do { \
        if (x86_64_is_imm8(imm)) { \
            x86_64_emit_rex(1,(basereg),0,0); \
            *(mcodeptr++) = (u1) 0x83; \
            x86_64_emit_membase((basereg),(disp),(opc)); \
            x86_64_emit_imm8((imm)); \
        } else { \
            x86_64_emit_rex(1,(basereg),0,0); \
            *(mcodeptr++) = (u1) 0x81; \
            x86_64_emit_membase((basereg),(disp),(opc)); \
            x86_64_emit_imm32((imm)); \
        } \
    } while (0)


#define x86_64_alul_imm_membase(opc,imm,basereg,disp) \
    do { \
        if (x86_64_is_imm8(imm)) { \
            x86_64_emit_rex(0,(basereg),0,0); \
            *(mcodeptr++) = (u1) 0x83; \
            x86_64_emit_membase((basereg),(disp),(opc)); \
            x86_64_emit_imm8((imm)); \
        } else { \
            x86_64_emit_rex(0,(basereg),0,0); \
            *(mcodeptr++) = (u1) 0x81; \
            x86_64_emit_membase((basereg),(disp),(opc)); \
            x86_64_emit_imm32((imm)); \
        } \
    } while (0)


#define x86_64_test_reg_reg(reg,dreg) \
    do { \
        x86_64_emit_rex(1,(reg),0,(dreg)); \
        *(mcodeptr++) = (u1) 0x85; \
        x86_64_emit_reg((reg),(dreg)); \
    } while (0)


#define x86_64_testl_reg_reg(reg,dreg) \
    do { \
        x86_64_emit_rex(0,(reg),0,(dreg)); \
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


#define x86_64_lea_membase_reg(basereg, disp, reg) \
    do { \
        x86_64_emit_rex(1,(reg),0,(basereg)); \
        *(mcodeptr++) = 0x8d; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_leal_membase_reg(basereg, disp, reg) \
    do { \
        x86_64_emit_rex(0,(reg),0,(basereg)); \
        *(mcodeptr++) = 0x8d; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)



/*
 * inc, dec operations
 */
#define x86_64_inc_reg(reg) \
    do { \
        x86_64_emit_rex(1,0,0,(reg)); \
        *(mcodeptr++) = 0xff; \
        x86_64_emit_reg(0,(reg)); \
    } while (0)


#define x86_64_incl_reg(reg) \
    do { \
        x86_64_emit_rex(0,0,0,(reg)); \
        *(mcodeptr++) = 0xff; \
        x86_64_emit_reg(0,(reg)); \
    } while (0)


#define x86_64_inc_membase(basereg,disp) \
    do { \
        x86_64_emit_rex(1,(basereg),0,0); \
        *(mcodeptr++) = 0xff; \
        x86_64_emit_membase((basereg),(disp),0); \
    } while (0)


#define x86_64_incl_membase(basereg,disp) \
    do { \
        x86_64_emit_rex(0,(basereg),0,0); \
        *(mcodeptr++) = 0xff; \
        x86_64_emit_membase((basereg),(disp),0); \
    } while (0)


#define x86_64_dec_reg(reg) \
    do { \
        x86_64_emit_rex(1,0,0,(reg)); \
        *(mcodeptr++) = 0xff; \
        x86_64_emit_reg(1,(reg)); \
    } while (0)

        
#define x86_64_decl_reg(reg) \
    do { \
        x86_64_emit_rex(0,0,0,(reg)); \
        *(mcodeptr++) = 0xff; \
        x86_64_emit_reg(1,(reg)); \
    } while (0)

        
#define x86_64_dec_membase(basereg,disp) \
    do { \
        x86_64_emit_rex(1,(basereg),0,0); \
        *(mcodeptr++) = 0xff; \
        x86_64_emit_membase((basereg),(disp),1); \
    } while (0)


#define x86_64_decl_membase(basereg,disp) \
    do { \
        x86_64_emit_rex(0,(basereg),0,0); \
        *(mcodeptr++) = 0xff; \
        x86_64_emit_membase((basereg),(disp),1); \
    } while (0)




#define x86_64_cltd() \
    *(mcodeptr++) = 0x99;


#define x86_64_cqto() \
    do { \
        x86_64_emit_rex(1,0,0,0); \
        *(mcodeptr++) = 0x99; \
    } while (0)



#define x86_64_imul_reg_reg(reg,dreg) \
    do { \
        x86_64_emit_rex(1,(dreg),0,(reg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xaf; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_imull_reg_reg(reg,dreg) \
    do { \
        x86_64_emit_rex(0,(dreg),0,(reg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xaf; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_imul_membase_reg(basereg,disp,dreg) \
    do { \
        x86_64_emit_rex(1,(dreg),0,(basereg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xaf; \
        x86_64_emit_membase((basereg),(disp),(dreg)); \
    } while (0)


#define x86_64_imull_membase_reg(basereg,disp,dreg) \
    do { \
        x86_64_emit_rex(0,(dreg),0,(basereg)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) 0xaf; \
        x86_64_emit_membase((basereg),(disp),(dreg)); \
    } while (0)


#define x86_64_imul_imm_reg(imm,dreg) \
    do { \
        if (x86_64_is_imm8((imm))) { \
            x86_64_emit_rex(1,0,0,(dreg)); \
            *(mcodeptr++) = (u1) 0x6b; \
            x86_64_emit_reg(0,(dreg)); \
            x86_64_emit_imm8((imm)); \
        } else { \
            x86_64_emit_rex(1,0,0,(dreg)); \
            *(mcodeptr++) = (u1) 0x69; \
            x86_64_emit_reg(0,(dreg)); \
            x86_64_emit_imm32((imm)); \
        } \
    } while (0)


#define x86_64_imul_imm_reg_reg(imm,reg,dreg) \
    do { \
        if (x86_64_is_imm8((imm))) { \
            x86_64_emit_rex(1,(dreg),0,(reg)); \
            *(mcodeptr++) = (u1) 0x6b; \
            x86_64_emit_reg((dreg),(reg)); \
            x86_64_emit_imm8((imm)); \
        } else { \
            x86_64_emit_rex(1,(dreg),0,(reg)); \
            *(mcodeptr++) = (u1) 0x69; \
            x86_64_emit_reg((dreg),(reg)); \
            x86_64_emit_imm32((imm)); \
        } \
    } while (0)


#define x86_64_imull_imm_reg_reg(imm,reg,dreg) \
    do { \
        if (x86_64_is_imm8((imm))) { \
            x86_64_emit_rex(0,(dreg),0,(reg)); \
            *(mcodeptr++) = (u1) 0x6b; \
            x86_64_emit_reg((dreg),(reg)); \
            x86_64_emit_imm8((imm)); \
        } else { \
            x86_64_emit_rex(0,(dreg),0,(reg)); \
            *(mcodeptr++) = (u1) 0x69; \
            x86_64_emit_reg((dreg),(reg)); \
            x86_64_emit_imm32((imm)); \
        } \
    } while (0)


#define x86_64_imul_imm_membase_reg(imm,basereg,disp,dreg) \
    do { \
        if (x86_64_is_imm8((imm))) { \
            x86_64_emit_rex(1,(dreg),0,(basereg)); \
            *(mcodeptr++) = (u1) 0x6b; \
            x86_64_emit_membase((basereg),(disp),(dreg)); \
            x86_64_emit_imm8((imm)); \
        } else { \
            x86_64_emit_rex(1,(dreg),0,(basereg)); \
            *(mcodeptr++) = (u1) 0x69; \
            x86_64_emit_membase((basereg),(disp),(dreg)); \
            x86_64_emit_imm32((imm)); \
        } \
    } while (0)


#define x86_64_imull_imm_membase_reg(imm,basereg,disp,dreg) \
    do { \
        if (x86_64_is_imm8((imm))) { \
            x86_64_emit_rex(0,(dreg),0,(basereg)); \
            *(mcodeptr++) = (u1) 0x6b; \
            x86_64_emit_membase((basereg),(disp),(dreg)); \
            x86_64_emit_imm8((imm)); \
        } else { \
            x86_64_emit_rex(0,(dreg),0,(basereg)); \
            *(mcodeptr++) = (u1) 0x69; \
            x86_64_emit_membase((basereg),(disp),(dreg)); \
            x86_64_emit_imm32((imm)); \
        } \
    } while (0)


#define x86_64_idiv_reg(reg) \
    do { \
        x86_64_emit_rex(1,0,0,(reg)); \
        *(mcodeptr++) = (u1) 0xf7; \
        x86_64_emit_reg(7,(reg)); \
    } while (0)


#define x86_64_idivl_reg(reg) \
    do { \
        x86_64_emit_rex(0,0,0,(reg)); \
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
        x86_64_emit_rex(1,0,0,(reg)); \
        *(mcodeptr++) = (u1) 0xd3; \
        x86_64_emit_reg((opc),(reg)); \
    } while (0)


#define x86_64_shiftl_reg(opc,reg) \
    do { \
        x86_64_emit_rex(0,0,0,(reg)); \
        *(mcodeptr++) = (u1) 0xd3; \
        x86_64_emit_reg((opc),(reg)); \
    } while (0)


#define x86_64_shift_membase(opc,basereg,disp) \
    do { \
        x86_64_emit_rex(1,0,0,(basereg)); \
        *(mcodeptr++) = (u1) 0xd3; \
        x86_64_emit_membase((basereg),(disp),(opc)); \
    } while (0)


#define x86_64_shiftl_membase(opc,basereg,disp) \
    do { \
        x86_64_emit_rex(0,0,0,(basereg)); \
        *(mcodeptr++) = (u1) 0xd3; \
        x86_64_emit_membase((basereg),(disp),(opc)); \
    } while (0)


#define x86_64_shift_imm_reg(opc,imm,dreg) \
    do { \
        if ((imm) == 1) { \
            x86_64_emit_rex(1,0,0,(dreg)); \
            *(mcodeptr++) = (u1) 0xd1; \
            x86_64_emit_reg((opc),(dreg)); \
        } else { \
            x86_64_emit_rex(1,0,0,(dreg)); \
            *(mcodeptr++) = (u1) 0xc1; \
            x86_64_emit_reg((opc),(dreg)); \
            x86_64_emit_imm8((imm)); \
        } \
    } while (0)


#define x86_64_shiftl_imm_reg(opc,imm,dreg) \
    do { \
        if ((imm) == 1) { \
            x86_64_emit_rex(0,0,0,(dreg)); \
            *(mcodeptr++) = (u1) 0xd1; \
            x86_64_emit_reg((opc),(dreg)); \
        } else { \
            x86_64_emit_rex(0,0,0,(dreg)); \
            *(mcodeptr++) = (u1) 0xc1; \
            x86_64_emit_reg((opc),(dreg)); \
            x86_64_emit_imm8((imm)); \
        } \
    } while (0)


#define x86_64_shift_imm_membase(opc,imm,basereg,disp) \
    do { \
        if ((imm) == 1) { \
            x86_64_emit_rex(1,0,0,(basereg)); \
            *(mcodeptr++) = (u1) 0xd1; \
            x86_64_emit_membase((basereg),(disp),(opc)); \
        } else { \
            x86_64_emit_rex(1,0,0,(basereg)); \
            *(mcodeptr++) = (u1) 0xc1; \
            x86_64_emit_membase((basereg),(disp),(opc)); \
            x86_64_emit_imm8((imm)); \
        } \
    } while (0)


#define x86_64_shiftl_imm_membase(opc,imm,basereg,disp) \
    do { \
        if ((imm) == 1) { \
            x86_64_emit_rex(0,0,0,(basereg)); \
            *(mcodeptr++) = (u1) 0xd1; \
            x86_64_emit_membase((basereg),(disp),(opc)); \
        } else { \
            x86_64_emit_rex(0,0,0,(basereg)); \
            *(mcodeptr++) = (u1) 0xc1; \
            x86_64_emit_membase((basereg),(disp),(opc)); \
            x86_64_emit_imm8((imm)); \
        } \
    } while (0)



/*
 * jump operations
 */
#define x86_64_jmp_imm(imm) \
    do { \
        *(mcodeptr++) = 0xe9; \
        x86_64_emit_imm32((imm)); \
    } while (0)


#define x86_64_jmp_reg(reg) \
    do { \
        x86_64_emit_rex(0,0,0,(reg)); \
        *(mcodeptr++) = 0xff; \
        x86_64_emit_reg(4,(reg)); \
    } while (0)


#define x86_64_jcc(opc,imm) \
    do { \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = (0x80 + x86_64_cc_map[(opc)]); \
        x86_64_emit_imm32((imm)); \
    } while (0)



/*
 * conditional set and move operations
 */

/* we need the rex byte to get all low bytes */
#define x86_64_setcc_reg(opc,reg) \
    do { \
        *(mcodeptr++) = (0x40 | (((reg) >> 3) & 0x01)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = (0x90 + x86_64_cc_map[(opc)]); \
        x86_64_emit_reg(0,(reg)); \
    } while (0)


/* we need the rex byte to get all low bytes */
#define x86_64_setcc_membase(opc,basereg,disp) \
    do { \
        *(mcodeptr++) = (0x40 | (((basereg) >> 3) & 0x01)); \
        *(mcodeptr++) = (u1) 0x0f; \
        *(mcodeptr++) = (u1) (0x90 + x86_64_cc_map[(opc)]); \
        x86_64_emit_membase((basereg),(disp),0); \
    } while (0)


#define x86_64_cmovcc_reg_reg(opc,reg,dreg) \
    do { \
        x86_64_emit_rex(1,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x40 + x86_64_cc_map[(opc)]; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_cmovccl_reg_reg(opc,reg,dreg) \
    do { \
        x86_64_emit_rex(0,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x40 + x86_64_cc_map[(opc)]; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)



#define x86_64_neg_reg(reg) \
    do { \
        x86_64_emit_rex(1,0,0,(reg)); \
        *(mcodeptr++) = (u1) 0xf7; \
        x86_64_emit_reg(3,(reg)); \
    } while (0)


#define x86_64_negl_reg(reg) \
    do { \
        x86_64_emit_rex(0,0,0,(reg)); \
        *(mcodeptr++) = (u1) 0xf7; \
        x86_64_emit_reg(3,(reg)); \
    } while (0)


#define x86_64_neg_membase(basereg,disp) \
    do { \
        x86_64_emit_rex(1,0,0,(basereg)); \
        *(mcodeptr++) = (u1) 0xf7; \
        x86_64_emit_membase((basereg),(disp),3); \
    } while (0)


#define x86_64_negl_membase(basereg,disp) \
    do { \
        x86_64_emit_rex(0,0,0,(basereg)); \
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
    do { \
        x86_64_emit_rex(0,0,0,(reg)); \
        *(mcodeptr++) = (u1) 0x58 + (0x07 & (reg)); \
    } while (0)


#define x86_64_xchg_reg_reg(reg,dreg) \
    do { \
        x86_64_emit_rex(1,(reg),0,(dreg)); \
        *(mcodeptr++) = 0x87; \
        x86_64_emit_reg((reg),(dreg)); \
    } while (0)


#define x86_64_nop() \
    *(mcodeptr++) = (u1) 0x90;


#define x86_64_hlt() \
    *(mcodeptr++) = 0xf4;



/*
 * call instructions
 */
#define x86_64_call_reg(reg) \
    do { \
        x86_64_emit_rex(1,0,0,(reg)); \
        *(mcodeptr++) = (u1) 0xff; \
        x86_64_emit_reg(2,(reg)); \
    } while (0)


#define x86_64_call_imm(imm) \
    do { \
        *(mcodeptr++) = (u1) 0xe8; \
        x86_64_emit_imm32((imm)); \
    } while (0)



/*
 * floating point instructions (SSE2)
 */
#define x86_64_addsd_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = 0xf2; \
        x86_64_emit_rex(0,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x58; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_addss_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = 0xf3; \
        x86_64_emit_rex(0,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x58; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_cvtsi2ssq_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = 0xf3; \
        x86_64_emit_rex(1,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x2a; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_cvtsi2ss_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = 0xf3; \
        x86_64_emit_rex(0,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x2a; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_cvtsi2sdq_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = 0xf2; \
        x86_64_emit_rex(1,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x2a; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_cvtsi2sd_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = 0xf2; \
        x86_64_emit_rex(0,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x2a; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_cvtss2sd_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = 0xf3; \
        x86_64_emit_rex(0,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x5a; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_cvtsd2ss_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = 0xf2; \
        x86_64_emit_rex(0,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x5a; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_cvttss2siq_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = 0xf3; \
        x86_64_emit_rex(1,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x2c; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_cvttss2si_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = 0xf3; \
        x86_64_emit_rex(0,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x2c; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_cvttsd2siq_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = 0xf2; \
        x86_64_emit_rex(1,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x2c; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_cvttsd2si_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = 0xf2; \
        x86_64_emit_rex(0,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x2c; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_divss_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = 0xf3; \
        x86_64_emit_rex(0,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x5e; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_divsd_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = 0xf2; \
        x86_64_emit_rex(0,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x5e; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_movd_reg_freg(reg,freg) \
    do { \
        *(mcodeptr++) = 0x66; \
        x86_64_emit_rex(1,(freg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x6e; \
        x86_64_emit_reg((freg),(reg)); \
    } while (0)


#define x86_64_movd_freg_reg(freg,reg) \
    do { \
        *(mcodeptr++) = 0x66; \
        x86_64_emit_rex(1,(freg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x7e; \
        x86_64_emit_reg((freg),(reg)); \
    } while (0)


#define x86_64_movd_reg_membase(reg,basereg,disp) \
    do { \
        *(mcodeptr++) = 0x66; \
        x86_64_emit_rex(0,(reg),0,(basereg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x7e; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_movd_reg_memindex(reg,disp,basereg,indexreg,scale) \
    do { \
        *(mcodeptr++) = 0x66; \
        x86_64_emit_rex(0,(reg),(indexreg),(basereg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x7e; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_movd_membase_reg(basereg,disp,dreg) \
    do { \
        *(mcodeptr++) = 0x66; \
        x86_64_emit_rex(1,(dreg),0,(basereg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x6e; \
        x86_64_emit_membase((basereg),(disp),(dreg)); \
    } while (0)


#define x86_64_movdl_membase_reg(basereg,disp,dreg) \
    do { \
        *(mcodeptr++) = 0x66; \
        x86_64_emit_rex(0,(dreg),0,(basereg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x6e; \
        x86_64_emit_membase((basereg),(disp),(dreg)); \
    } while (0)


#define x86_64_movd_memindex_reg(disp,basereg,indexreg,scale,dreg) \
    do { \
        *(mcodeptr++) = 0x66; \
        x86_64_emit_rex(0,(dreg),(indexreg),(basereg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x6e; \
        x86_64_emit_memindex((dreg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_movq_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = 0xf3; \
        x86_64_emit_rex(0,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x7e; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_movq_reg_membase(reg,basereg,disp) \
    do { \
        *(mcodeptr++) = 0x66; \
        x86_64_emit_rex(0,(reg),0,(basereg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0xd6; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_movq_membase_reg(basereg,disp,dreg) \
    do { \
        *(mcodeptr++) = 0xf3; \
        x86_64_emit_rex(0,(dreg),0,(basereg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x7e; \
        x86_64_emit_membase((basereg),(disp),(dreg)); \
    } while (0)


#define x86_64_movss_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = 0xf3; \
        x86_64_emit_rex(0,(reg),0,(dreg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x10; \
        x86_64_emit_reg((reg),(dreg)); \
    } while (0)


#define x86_64_movsd_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = 0xf2; \
        x86_64_emit_rex(0,(reg),0,(dreg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x10; \
        x86_64_emit_reg((reg),(dreg)); \
    } while (0)


#define x86_64_movss_reg_membase(reg,basereg,disp) \
    do { \
        *(mcodeptr++) = 0xf3; \
        x86_64_emit_rex(0,(reg),0,(basereg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x11; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_movsd_reg_membase(reg,basereg,disp) \
    do { \
        *(mcodeptr++) = 0xf2; \
        x86_64_emit_rex(0,(reg),0,(basereg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x11; \
        x86_64_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define x86_64_movss_membase_reg(basereg,disp,dreg) \
    do { \
        *(mcodeptr++) = 0xf3; \
        x86_64_emit_rex(0,(dreg),0,(basereg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x10; \
        x86_64_emit_membase((basereg),(disp),(dreg)); \
    } while (0)


#define x86_64_movlps_membase_reg(basereg,disp,dreg) \
    do { \
        x86_64_emit_rex(0,(dreg),0,(basereg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x12; \
        x86_64_emit_membase((basereg),(disp),(dreg)); \
    } while (0)


#define x86_64_movsd_membase_reg(basereg,disp,dreg) \
    do { \
        *(mcodeptr++) = 0xf2; \
        x86_64_emit_rex(0,(dreg),0,(basereg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x10; \
        x86_64_emit_membase((basereg),(disp),(dreg)); \
    } while (0)


#define x86_64_movlpd_membase_reg(basereg,disp,dreg) \
    do { \
        *(mcodeptr++) = 0x66; \
        x86_64_emit_rex(0,(dreg),0,(basereg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x12; \
        x86_64_emit_membase((basereg),(disp),(dreg)); \
    } while (0)


#define x86_64_movss_reg_memindex(reg,disp,basereg,indexreg,scale) \
    do { \
        *(mcodeptr++) = 0xf3; \
        x86_64_emit_rex(0,(reg),(indexreg),(basereg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x11; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_movsd_reg_memindex(reg,disp,basereg,indexreg,scale) \
    do { \
        *(mcodeptr++) = 0xf2; \
        x86_64_emit_rex(0,(reg),(indexreg),(basereg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x11; \
        x86_64_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_movss_memindex_reg(disp,basereg,indexreg,scale,dreg) \
    do { \
        *(mcodeptr++) = 0xf3; \
        x86_64_emit_rex(0,(dreg),(indexreg),(basereg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x10; \
        x86_64_emit_memindex((dreg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_movsd_memindex_reg(disp,basereg,indexreg,scale,dreg) \
    do { \
        *(mcodeptr++) = 0xf2; \
        x86_64_emit_rex(0,(dreg),(indexreg),(basereg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x10; \
        x86_64_emit_memindex((dreg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define x86_64_mulss_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = 0xf3; \
        x86_64_emit_rex(0,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x59; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_mulsd_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = 0xf2; \
        x86_64_emit_rex(0,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x59; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_subss_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = 0xf3; \
        x86_64_emit_rex(0,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x5c; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_subsd_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = 0xf2; \
        x86_64_emit_rex(0,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x5c; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_ucomiss_reg_reg(reg,dreg) \
    do { \
        x86_64_emit_rex(0,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x2e; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_ucomisd_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = 0x66; \
        x86_64_emit_rex(0,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x2e; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_xorps_reg_reg(reg,dreg) \
    do { \
        x86_64_emit_rex(0,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x57; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_xorps_membase_reg(basereg,disp,dreg) \
    do { \
        x86_64_emit_rex(0,(dreg),0,(basereg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x57; \
        x86_64_emit_membase((basereg),(disp),(dreg)); \
    } while (0)


#define x86_64_xorpd_reg_reg(reg,dreg) \
    do { \
        *(mcodeptr++) = 0x66; \
        x86_64_emit_rex(0,(dreg),0,(reg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x57; \
        x86_64_emit_reg((dreg),(reg)); \
    } while (0)


#define x86_64_xorpd_membase_reg(basereg,disp,dreg) \
    do { \
        *(mcodeptr++) = 0x66; \
        x86_64_emit_rex(0,(dreg),0,(basereg)); \
        *(mcodeptr++) = 0x0f; \
        *(mcodeptr++) = 0x57; \
        x86_64_emit_membase((basereg),(disp),(dreg)); \
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
