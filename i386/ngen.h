/* i386/ngen.h *****************************************************************

    Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

    See file COPYRIGHT for information on usage and disclaimer of warranties

    Contains the machine dependent code generator definitions and macros for an
    Alpha processor.

    Authors: Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at
             Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

    Last Change: 1998/11/04

*******************************************************************************/

#ifndef _NGEN_H
#define _NGEN_H

/* see also file calling.doc for explanation of calling conventions           */

/* preallocated registers *****************************************************/

/* integer registers */
  
#define REG_RESULT      I386_EAX /* to deliver method results                 */
#define REG_RESULT2     I386_EDX /* to deliver long method results            */

#define REG_RA          26   /* return address                                */
#define REG_PV          27   /* procedure vector, must be provided by caller  */
#define REG_METHODPTR   28   /* pointer to the place from where the procedure */
                             /* vector has been fetched                       */
#define REG_ITMP1       I386_EAX /* temporary register                        */
#define REG_ITMP2       I386_EDX /* temporary register and method pointer     */
#define REG_ITMP3       I386_ECX /* temporary register                        */

#define REG_ITMP1_XPTR  I386_EAX /* exception pointer = temporary register 1  */
#define REG_ITMP2_XPC   I386_EDX /* exception pc = temporary register 2       */

#define REG_SP          I386_ESP /* stack pointer                             */
#define REG_ZERO        31   /* always zero                                   */

/* floating point registers */

#define REG_FRESULT     0    /* to deliver floating point method results      */
#define REG_FTMP1       0    /* temporary floating point register             */
#define REG_FTMP2       1    /* temporary floating point register             */
#define REG_FTMP3       2    /* temporary floating point register             */

#define REG_IFTMP       28   /* temporary integer and floating point register */

/* register descripton - array ************************************************/

/* #define REG_RES   0         reserved register for OS or code generator     */
/* #define REG_RET   1         return value register                          */
/* #define REG_EXC   2         exception value register (only old jit)        */
/* #define REG_SAV   3         (callee) saved register                        */
/* #define REG_TMP   4         scratch temporary register (caller saved)      */
/* #define REG_ARG   5         argument register (caller saved)               */

/* #define REG_END   -1        last entry in tables */

int nregdescint[] = {
/*      REG_RET, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, */
    REG_RET, REG_RES, REG_RES, REG_SAV, REG_RES, REG_SAV, REG_SAV, REG_TMP,
    REG_END };

/* for use of reserved registers, see comment above */

int nregdescfloat[] = {
/*      REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, REG_RES, */
    REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_TMP, REG_RES, REG_RES, REG_RES,
    REG_END };

/* for use of reserved registers, see comment above */


/* parameter allocation mode */

int nreg_parammode = PARAMMODE_NUMBERED;  

   /* parameter-registers will be allocated by assigning the
      1. parameter:   int/float-reg 16
      2. parameter:   int/float-reg 17  
      3. parameter:   int/float-reg 18 ....
   */


/* stackframe-infos ***********************************************************/

int parentargs_base; /* offset in stackframe for the parameter from the caller*/

/* -> see file 'calling.doc' */


/* macros to create code ******************************************************/

#define M_OP3(op,fu,a,b,c,const) \
do { \
        printf("M_OP3: %d\n", __LINE__); \
	*((s4 *) (((s4) mcodeptr)++)) = 0x0F; \
        *((s4 *) (((s4) mcodeptr)++)) = 0x04; \
} while (0)

#define M_FOP3(op,fu,a,b,c)	M_OP3(0,0,0,0,0,0)
#define M_BRA(op,a,disp)	M_OP3(0,0,0,0,0,0)
#define M_MEM(op,a,b,disp)	M_OP3(0,0,0,0,0,0)



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
    do { \
        *(((u1 *) mcodeptr)++) = ((((mod) & 0x03) << 6) | (((reg) & 0x07) << 3) | (((rm) & 0x07))); \
    } while (0)


#define i386_emit_reg(reg,rm) \
    do { \
        i386_address_byte(3,(reg),(rm)); \
    } while (0)


#define i386_is_imm8(imm)	(((int)(imm) >= -128 && (int)(imm) <= 127))


#define i386_emit_imm8(imm) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) ((imm) & 0xff); \
    } while (0)


#define i386_emit_imm16(imm) \
    do { \
        i386_imm_buf imb; \
        imb.i = (int) (imm); \
        *(((u1 *) mcodeptr)++) = imb.b[0]; \
        *(((u1 *) mcodeptr)++) = imb.b[1]; \
    } while (0)


#define i386_emit_imm32(imm) \
    do { \
        i386_imm_buf imb; \
        imb.i = (int) (imm); \
        *(((u1 *) mcodeptr)++) = imb.b[0]; \
        *(((u1 *) mcodeptr)++) = imb.b[1]; \
        *(((u1 *) mcodeptr)++) = imb.b[2]; \
        *(((u1 *) mcodeptr)++) = imb.b[3]; \
    } while (0)


#define i386_emit_float32(imm) \
    do { \
        i386_imm_buf imb; \
        imb.f = (float) (imm); \
        *(((u1 *) mcodeptr)++) = imb.b[0]; \
        *(((u1 *) mcodeptr)++) = imb.b[1]; \
        *(((u1 *) mcodeptr)++) = imb.b[2]; \
        *(((u1 *) mcodeptr)++) = imb.b[3]; \
    } while (0)


#define i386_emit_double64_low(imm) \
    do { \
        i386_imm_buf imb; \
        imb.d = (double) (imm); \
        *(((u1 *) mcodeptr)++) = imb.b[0]; \
        *(((u1 *) mcodeptr)++) = imb.b[1]; \
        *(((u1 *) mcodeptr)++) = imb.b[2]; \
        *(((u1 *) mcodeptr)++) = imb.b[3]; \
    } while (0)


#define i386_emit_double64_high(imm) \
    do { \
        i386_imm_buf imb; \
        imb.d = (double) (imm); \
        *(((u1 *) mcodeptr)++) = imb.b[4]; \
        *(((u1 *) mcodeptr)++) = imb.b[5]; \
        *(((u1 *) mcodeptr)++) = imb.b[6]; \
        *(((u1 *) mcodeptr)++) = imb.b[7]; \
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



/*
 * mov ops
 */
#define i386_mov_reg_reg(reg,dreg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x89; \
        i386_emit_reg((reg),(dreg)); \
    } while (0)


#define i386_mov_imm_reg(imm,reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xb8 + ((reg) & 0x07); \
        i386_emit_imm32((imm)); \
    } while (0)


#define i386_mov_float_reg(imm,reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xb8 + ((reg) & 0x07); \
        i386_emit_float32((imm)); \
    } while (0)


#define i386_mov_reg_mem(reg,mem) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x89; \
        i386_emit_mem((reg),(mem)); \
    } while (0)


#define i386_mov_mem_reg(mem,reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x8b; \
        i386_emit_mem((reg),(mem)); \
    } while (0)


#define i386_mov_membase_reg(basereg,disp,reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x8b; \
        i386_emit_membase((basereg),(disp),(reg)); \
    } while (0)


/*
 * this one is for INVOKEVIRTUAL/INVOKEINTERFACE to have a
 * constant membase immediate length of 32bit
 */
#define i386_mov_membase32_reg(basereg,disp,reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x8b; \
        i386_address_byte(2, (reg), (basereg)); \
        i386_emit_imm32((disp)); \
    } while (0)


#define i386_movw_membase_reg(basereg,disp,reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x66; \
        i386_mov_membase_reg((basereg),(disp),(reg)); \
    } while (0)


#define i386_movb_membase_reg(basereg,disp,reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x8a; \
        i386_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define i386_mov_reg_membase(reg,basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x89; \
        i386_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define i386_movw_reg_membase(reg,basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x66; \
        *(((u1 *) mcodeptr)++) = (u1) 0x89; \
        i386_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define i386_movb_reg_membase(reg,basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x88; \
        i386_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define i386_mov_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x8b; \
        i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_movw_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x66; \
        *(((u1 *) mcodeptr)++) = (u1) 0x8b; \
        i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_movb_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x8a; \
        i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_mov_reg_memindex(reg,disp,basereg,indexreg,scale) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x89; \
        i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_movw_reg_memindex(reg,disp,basereg,indexreg,scale) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x66; \
        *(((u1 *) mcodeptr)++) = (u1) 0x89; \
        i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_movb_reg_memindex(reg,disp,basereg,indexreg,scale) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x88; \
        i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_mov_imm_membase(imm,basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xc7; \
        i386_emit_membase((basereg),(disp),0); \
        i386_emit_imm32((imm)); \
    } while (0)


#define i386_mov_float_membase(imm,basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xc7; \
        i386_emit_membase((basereg),(disp),0); \
        i386_emit_float32((imm)); \
    } while (0)


#define i386_mov_double_low_membase(imm,basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xc7; \
        i386_emit_membase((basereg),(disp),0); \
        i386_emit_double64_low((imm)); \
    } while (0)


#define i386_mov_double_high_membase(imm,basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xc7; \
        i386_emit_membase((basereg),(disp),0); \
        i386_emit_double64_high((imm)); \
    } while (0)


#define i386_movsbl_reg_reg(reg,dreg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x0f; \
        *(((u1 *) mcodeptr)++) = (u1) 0xbe; \
        i386_emit_reg((reg),(dreg)); \
    } while (0)


#define i386_movswl_reg_reg(reg,dreg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x0f; \
        *(((u1 *) mcodeptr)++) = (u1) 0xbf; \
        i386_emit_reg((reg),(dreg)); \
    } while (0)


#define i386_movzbl_reg_reg(reg,dreg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x0f; \
        *(((u1 *) mcodeptr)++) = (u1) 0xb6; \
        /* XXX: why do reg and dreg have to be exchanged */ \
        i386_emit_reg((dreg),(reg)); \
    } while (0)


#define i386_movzwl_reg_reg(reg,dreg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x0f; \
        *(((u1 *) mcodeptr)++) = (u1) 0xb7; \
        /* XXX: why do reg and dreg have to be exchanged */ \
        i386_emit_reg((dreg),(reg)); \
    } while (0)


#define i386_movsbl_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x0f; \
        *(((u1 *) mcodeptr)++) = (u1) 0xbe; \
        i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_movswl_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x0f; \
        *(((u1 *) mcodeptr)++) = (u1) 0xbf; \
        i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_movzbl_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x0f; \
        *(((u1 *) mcodeptr)++) = (u1) 0xb6; \
        i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_movzwl_memindex_reg(disp,basereg,indexreg,scale,reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x0f; \
        *(((u1 *) mcodeptr)++) = (u1) 0xb7; \
        i386_emit_memindex((reg),(disp),(basereg),(indexreg),(scale)); \
    } while (0)



/*
 * alu operations
 */
#define i386_alu_reg_reg(opc,reg,dreg) \
    do { \
        *(((u1 *) mcodeptr)++) = (((u1) (opc)) << 3) + 1; \
        i386_emit_reg((reg),(dreg)); \
    } while (0)


#define i386_alu_reg_membase(opc,reg,basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (((u1) (opc)) << 3) + 1; \
        i386_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define i386_alu_membase_reg(opc,basereg,disp,reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (((u1) (opc)) << 3) + 3; \
        i386_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define i386_alu_imm_reg(opc,imm,dreg) \
    do { \
        if (i386_is_imm8(imm)) { \
            *(((u1 *) mcodeptr)++) = (u1) 0x83; \
            i386_emit_reg((opc),(dreg)); \
            i386_emit_imm8((imm)); \
        } else { \
            *(((u1 *) mcodeptr)++) = (u1) 0x81; \
            i386_emit_reg((opc),(dreg)); \
            i386_emit_imm32((imm)); \
        } \
    } while (0)


#define i386_alu_imm_membase(opc,imm,basereg,disp) \
    do { \
        if (i386_is_imm8(imm)) { \
            *(((u1 *) mcodeptr)++) = (u1) 0x83; \
            i386_emit_membase((basereg),(disp),(opc)); \
            i386_emit_imm8((imm)); \
        } else { \
            *(((u1 *) mcodeptr)++) = (u1) 0x81; \
            i386_emit_membase((basereg),(disp),(opc)); \
            i386_emit_imm32((imm)); \
        } \
    } while (0)


#define i386_test_reg_reg(reg,dreg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x85; \
        i386_emit_reg((reg),(dreg)); \
    } while (0)


#define i386_test_imm_reg(imm,reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xf7; \
        i386_emit_reg(0,(reg)); \
        i386_emit_imm32((imm)); \
    } while (0)


#define i386_testw_imm_reg(imm,reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x66; \
        *(((u1 *) mcodeptr)++) = (u1) 0xf7; \
        i386_emit_reg(0,(reg)); \
        i386_emit_imm16((imm)); \
    } while (0)


#define i386_testb_imm_reg(imm,reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xf6; \
        i386_emit_reg(0,(reg)); \
        i386_emit_imm8((imm)); \
    } while (0)



/*
 * inc, dec operations
 */
#define i386_inc_reg(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x40 + ((reg) & 0x07); \
    } while (0)


#define i386_inc_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xff; \
        i386_emit_membase((basereg),(disp),0); \
    } while (0)


#define i386_dec_reg(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x48 + ((reg) & 0x07); \
    } while (0)

        
#define i386_dec_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xff; \
        i386_emit_membase((basereg),(disp),1); \
    } while (0)




#define i386_cltd() \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x99; \
    } while (0)



#define i386_imul_reg_reg(reg,dreg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x0f; \
        *(((u1 *) mcodeptr)++) = (u1) 0xaf; \
        i386_emit_reg((dreg),(reg)); \
    } while (0)


#define i386_imul_membase_reg(basereg,disp,dreg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x0f; \
        *(((u1 *) mcodeptr)++) = (u1) 0xaf; \
        i386_emit_membase((basereg),(disp),(dreg)); \
    } while (0)


#define i386_imul_imm_reg(imm,dreg) \
    do { \
        if (i386_is_imm8((imm))) { \
            *(((u1 *) mcodeptr)++) = (u1) 0x6b; \
            i386_emit_reg(0,(dreg)); \
            i386_emit_imm8((imm)); \
        } else { \
            *(((u1 *) mcodeptr)++) = (u1) 0x69; \
            i386_emit_reg(0,(dreg)); \
            i386_emit_imm32((imm)); \
        } \
    } while (0)


#define i386_imul_imm_reg_reg(imm,reg,dreg) \
    do { \
        if (i386_is_imm8((imm))) { \
            *(((u1 *) mcodeptr)++) = (u1) 0x6b; \
            i386_emit_reg((dreg),(reg)); \
            i386_emit_imm8((imm)); \
        } else { \
            *(((u1 *) mcodeptr)++) = (u1) 0x69; \
            i386_emit_reg((dreg),(reg)); \
            i386_emit_imm32((imm)); \
        } \
    } while (0)


#define i386_imul_imm_membase_reg(imm,basereg,disp,dreg) \
    do { \
        if (i386_is_imm8((imm))) { \
            *(((u1 *) mcodeptr)++) = (u1) 0x6b; \
            i386_emit_membase((basereg),(disp),(dreg)); \
            i386_emit_imm8((imm)); \
        } else { \
            *(((u1 *) mcodeptr)++) = (u1) 0x69; \
            i386_emit_membase((basereg),(disp),(dreg)); \
            i386_emit_imm32((imm)); \
        } \
    } while (0)


#define i386_mul_reg(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xf7; \
        i386_emit_reg(4,(reg)); \
    } while (0)


#define i386_mul_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xf7; \
        i386_emit_membase((basereg),(disp),4); \
    } while (0)


#define i386_idiv_reg(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xf7; \
        i386_emit_reg(7,(reg)); \
    } while (0)


#define i386_idiv_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xf7; \
        i386_emit_membase((basereg),(disp),7); \
    } while (0)



#define i386_ret() \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xc3; \
    } while (0)


#define i386_leave() \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xc9; \
    } while (0)



/*
 * shift ops
 */
#define i386_shift_reg(opc,reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd3; \
        i386_emit_reg((opc),(reg)); \
    } while (0)


#define i386_shift_membase(opc,basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd3; \
        i386_emit_membase((basereg),(disp),(opc)); \
    } while (0)


#define i386_shift_imm_reg(opc,imm,dreg) \
    do { \
        if ((imm) == 1) { \
            *(((u1 *) mcodeptr)++) = (u1) 0xd1; \
            i386_emit_reg((opc),(dreg)); \
        } else { \
            *(((u1 *) mcodeptr)++) = (u1) 0xc1; \
            i386_emit_reg((opc),(dreg)); \
            i386_emit_imm8((imm)); \
        } \
    } while (0)


#define i386_shift_imm_membase(opc,imm,basereg,disp) \
    do { \
        if ((imm) == 1) { \
            *(((u1 *) mcodeptr)++) = (u1) 0xd1; \
            i386_emit_membase((basereg),(disp),(opc)); \
        } else { \
            *(((u1 *) mcodeptr)++) = (u1) 0xc1; \
            i386_emit_membase((basereg),(disp),(opc)); \
            i386_emit_imm8((imm)); \
        } \
    } while (0)


#define i386_shld_reg_reg(reg,dreg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x0f; \
        *(((u1 *) mcodeptr)++) = (u1) 0xa5; \
        i386_emit_reg((reg),(dreg)); \
    } while (0)


#define i386_shld_imm_reg_reg(imm,reg,dreg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x0f; \
        *(((u1 *) mcodeptr)++) = (u1) 0xa4; \
        i386_emit_reg((reg),(dreg)); \
        i386_emit_imm8((imm)); \
    } while (0)


#define i386_shld_reg_membase(reg,basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x0f; \
        *(((u1 *) mcodeptr)++) = (u1) 0xa5; \
        i386_emit_membase((basereg),(disp),(reg)); \
    } while (0)


#define i386_shrd_reg_reg(reg,dreg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x0f; \
        *(((u1 *) mcodeptr)++) = (u1) 0xad; \
        i386_emit_reg((reg),(dreg)); \
    } while (0)


#define i386_shrd_imm_reg_reg(imm,reg,dreg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x0f; \
        *(((u1 *) mcodeptr)++) = (u1) 0xac; \
        i386_emit_reg((reg),(dreg)); \
        i386_emit_imm8((imm)); \
    } while (0)


#define i386_shrd_reg_membase(reg,basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x0f; \
        *(((u1 *) mcodeptr)++) = (u1) 0xad; \
        i386_emit_membase((basereg),(disp),(reg)); \
    } while (0)



/*
 * jump operations
 */
#define i386_jmp_imm(imm) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xe9; \
        i386_emit_imm32((imm)); \
    } while (0)


#define i386_jmp_reg(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xff; \
        i386_emit_reg(4,(reg)); \
    } while (0)


#define i386_jcc(opc,imm) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x0f; \
        *(((u1 *) mcodeptr)++) = (u1) (0x80 + i386_jcc_map[(opc)]); \
        i386_emit_imm32((imm)); \
    } while (0)



/*
 * conditional set operations
 */
#define i386_setcc_reg(opc,reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x0f; \
        *(((u1 *) mcodeptr)++) = (u1) (0x90 + i386_jcc_map[(opc)]); \
        i386_emit_reg(0,(reg)); \
    } while (0)


#define i386_setcc_membase(opc,basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x0f; \
        *(((u1 *) mcodeptr)++) = (u1) (0x90 + i386_jcc_map[(opc)]); \
        i386_emit_membase((basereg),(disp),0); \
    } while (0)



#define i386_neg_reg(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xf7; \
        i386_emit_reg(3,(reg)); \
    } while (0)


#define i386_neg_mem(mem) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xf7; \
        i386_emit_mem(3,(mem)); \
    } while (0)


#define i386_neg_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xf7; \
        i386_emit_membase((basereg),(disp),3); \
    } while (0)



#define i386_push_reg(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x50 + (0x07 & (reg)); \
    } while (0)


#define i386_push_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xff; \
        i386_emit_membase((basereg),(disp),6); \
    } while (0)


#define i386_push_imm(imm) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x68; \
        i386_emit_imm32((imm)); \
    } while (0)


#define i386_pop_reg(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x58 + (0x07 & (reg)); \
    } while (0)


#define i386_nop() \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x90; \
    } while (0)



/*
 * call instructions
 */
#define i386_call_reg(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xff; \
        i386_emit_reg(2,(reg)); \
    } while (0)


#define i386_call_imm(imm) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xe8; \
        i386_emit_imm32((imm)); \
    } while (0)



/*
 * floating point instructions
 */
#define i386_fld1() \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd9; \
        *(((u1 *) mcodeptr)++) = (u1) 0xe8; \
    } while (0)


#define i386_fldz() \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd9; \
        *(((u1 *) mcodeptr)++) = (u1) 0xee; \
    } while (0)


#define i386_fld_reg(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd9; \
        *(((u1 *) mcodeptr)++) = (u1) 0xc0 + (0x07 & (reg)); \
    } while (0)


#define i386_flds_mem(mem) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd9; \
        i386_emit_mem(0,(mem)); \
    } while (0)


#define i386_fldl_mem(mem) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdd; \
        i386_emit_mem(0,(mem)); \
    } while (0)


#define i386_flds_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd9; \
        i386_emit_membase((basereg),(disp),0); \
    } while (0)


#define i386_fldl_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdd; \
        i386_emit_membase((basereg),(disp),0); \
    } while (0)


#define i386_flds_memindex(disp,basereg,indexreg,scale) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd9; \
        i386_emit_memindex(0,(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_fldl_memindex(disp,basereg,indexreg,scale) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdd; \
        i386_emit_memindex(0,(disp),(basereg),(indexreg),(scale)); \
    } while (0)




#define i386_fildl_mem(mem) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdb; \
        i386_emit_mem(0,(mem)); \
    } while (0)


#define i386_fildl_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdb; \
        i386_emit_membase((basereg),(disp),0); \
    } while (0)


#define i386_fildll_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdf; \
        i386_emit_membase((basereg),(disp),5); \
    } while (0)




#define i386_fst_reg(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdd; \
        *(((u1 *) mcodeptr)++) = (u1) 0xd0 + (0x07 & (reg)); \
    } while (0)


#define i386_fsts_mem(mem) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd9; \
        i386_emit_mem(2,(mem)); \
    } while (0)


#define i386_fstl_mem(mem) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdd; \
        i386_emit_mem(2,(mem)); \
    } while (0)


#define i386_fsts_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd9; \
        i386_emit_membase((basereg),(disp),2); \
    } while (0)


#define i386_fstl_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdd; \
        i386_emit_membase((basereg),(disp),2); \
    } while (0)


#define i386_fsts_memindex(disp,basereg,indexreg,scale) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd9; \
        i386_emit_memindex(2,(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_fstl_memindex(disp,basereg,indexreg,scale) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdd; \
        i386_emit_memindex(2,(disp),(basereg),(indexreg),(scale)); \
    } while (0)




#define i386_fstp_reg(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdd; \
        *(((u1 *) mcodeptr)++) = (u1) 0xd8 + (0x07 & (reg)); \
    } while (0)


#define i386_fstps_mem(mem) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd9; \
        i386_emit_mem(3,(mem)); \
    } while (0)


#define i386_fstpl_mem(mem) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdd; \
        i386_emit_mem(3,(mem)); \
    } while (0)


#define i386_fstps_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd9; \
        i386_emit_membase((basereg),(disp),3); \
    } while (0)


#define i386_fstpl_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdd; \
        i386_emit_membase((basereg),(disp),3); \
    } while (0)


#define i386_fstps_memindex(disp,basereg,indexreg,scale) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd9; \
        i386_emit_memindex(3,(disp),(basereg),(indexreg),(scale)); \
    } while (0)


#define i386_fstpl_memindex(disp,basereg,indexreg,scale) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdd; \
        i386_emit_memindex(3,(disp),(basereg),(indexreg),(scale)); \
    } while (0)




#define i386_fistpl_mem(mem) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdb; \
        i386_emit_mem(3,(mem)); \
    } while (0)


#define i386_fistpll_mem(mem) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdf; \
        i386_emit_mem(7,(mem)); \
    } while (0)


#define i386_fistl_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdb; \
        i386_emit_membase((basereg),(disp),2); \
    } while (0)


#define i386_fistpl_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdb; \
        i386_emit_membase((basereg),(disp),3); \
    } while (0)


#define i386_fistpll_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdf; \
        i386_emit_membase((basereg),(disp),7); \
    } while (0)




#define i386_fchs() \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd9; \
        *(((u1 *) mcodeptr)++) = (u1) 0xe0; \
    } while (0)


#define i386_faddp() \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xde; \
        *(((u1 *) mcodeptr)++) = (u1) 0xc1; \
    } while (0)


#define i386_fadd_reg_st(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd8; \
        *(((u1 *) mcodeptr)++) = (u1) 0xc0 + (0x0f & (reg)); \
    } while (0)


#define i386_fadd_st_reg(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdc; \
        *(((u1 *) mcodeptr)++) = (u1) 0xc0 + (0x0f & (reg)); \
    } while (0)


#define i386_faddp_st_reg(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xde; \
        *(((u1 *) mcodeptr)++) = (u1) 0xc0 + (0x0f & (reg)); \
    } while (0)


#define i386_fadds_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd8; \
        i386_emit_membase((basereg),(disp),0); \
    } while (0)


#define i386_faddl_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdc; \
        i386_emit_membase((basereg),(disp),0); \
    } while (0)


#define i386_fsub_reg_st(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd8; \
        *(((u1 *) mcodeptr)++) = (u1) 0xe0 + (0x07 & (reg)); \
    } while (0)


#define i386_fsub_st_reg(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdc; \
        *(((u1 *) mcodeptr)++) = (u1) 0xe8 + (0x07 & (reg)); \
    } while (0)


#define i386_fsubp_st_reg(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xde; \
        *(((u1 *) mcodeptr)++) = (u1) 0xe8 + (0x07 & (reg)); \
    } while (0)


#define i386_fsubp() \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xde; \
        *(((u1 *) mcodeptr)++) = (u1) 0xe9; \
    } while (0)


#define i386_fsubs_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd8; \
        i386_emit_membase((basereg),(disp),4); \
    } while (0)


#define i386_fsubl_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdc; \
        i386_emit_membase((basereg),(disp),4); \
    } while (0)


#define i386_fmul_reg_st(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd8; \
        *(((u1 *) mcodeptr)++) = (u1) 0xc8 + (0x07 & (reg)); \
    } while (0)


#define i386_fmul_st_reg(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdc; \
        *(((u1 *) mcodeptr)++) = (u1) 0xc8 + (0x07 & (reg)); \
    } while (0)


#define i386_fmulp() \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xde; \
        *(((u1 *) mcodeptr)++) = (u1) 0xc9; \
    } while (0)


#define i386_fmulp_st_reg(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xde; \
        *(((u1 *) mcodeptr)++) = (u1) 0xc8 + (0x07 & (reg)); \
    } while (0)


#define i386_fmuls_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd8; \
        i386_emit_membase((basereg),(disp),1); \
    } while (0)


#define i386_fmull_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdc; \
        i386_emit_membase((basereg),(disp),1); \
    } while (0)


#define i386_fdivp() \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xde; \
        *(((u1 *) mcodeptr)++) = (u1) 0xf9; \
    } while (0)


#define i386_fxch() \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd9; \
        *(((u1 *) mcodeptr)++) = (u1) 0xc9; \
    } while (0)


#define i386_fxch_reg(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd9; \
        *(((u1 *) mcodeptr)++) = (u1) 0xc8 + (0x07 & (reg)); \
    } while (0)


#define i386_fprem() \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd9; \
        *(((u1 *) mcodeptr)++) = (u1) 0xf8; \
    } while (0)


#define i386_fprem1() \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd9; \
        *(((u1 *) mcodeptr)++) = (u1) 0xf5; \
    } while (0)


#define i386_fucom() \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdd; \
        *(((u1 *) mcodeptr)++) = (u1) 0xe1; \
    } while (0)


#define i386_fucom_reg(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdd; \
        *(((u1 *) mcodeptr)++) = (u1) 0xe0 + (0x07 & (reg)); \
    } while (0)


#define i386_fucomp_reg(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdd; \
        *(((u1 *) mcodeptr)++) = (u1) 0xe8 + (0x07 & (reg)); \
    } while (0)


#define i386_fucompp() \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xda; \
        *(((u1 *) mcodeptr)++) = (u1) 0xe9; \
    } while (0)


#define i386_fnstsw() \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdf; \
        *(((u1 *) mcodeptr)++) = (u1) 0xe0; \
    } while (0)


#define i386_sahf() \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x9e; \
    } while (0)


#define i386_finit() \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x9b; \
        *(((u1 *) mcodeptr)++) = (u1) 0xdb; \
        *(((u1 *) mcodeptr)++) = (u1) 0xe3; \
    } while (0)


#define i386_fldcw_membase(basereg,disp) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd9; \
        i386_emit_membase((basereg),(disp),5); \
    } while (0)


#define i386_wait() \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0x9b; \
    } while (0)


#define i386_ffree_reg(reg) \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xdd; \
        *(((u1 *) mcodeptr)++) = (u1) 0xc0 + (0x07 & (reg)); \
    } while (0)


#define i386_fdecstp() \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd9; \
        *(((u1 *) mcodeptr)++) = (u1) 0xf6; \
    } while (0)


#define i386_fincstp() \
    do { \
        *(((u1 *) mcodeptr)++) = (u1) 0xd9; \
        *(((u1 *) mcodeptr)++) = (u1) 0xf7; \
    } while (0)



/* macros for all used commands (see an Alpha-manual for description) *********/ 

#define M_LDA(a,b,disp)         M_MEM (0x08,a,b,disp)           /* low const  */
#define M_LDAH(a,b,disp)        M_MEM (0x09,a,b,disp)           /* high const */
#define M_BLDU(a,b,disp)        M_MEM (0x0a,a,b,disp)           /*  8 load    */
#define M_SLDU(a,b,disp)        M_MEM (0x0c,a,b,disp)           /* 16 load    */
#define M_ILD(a,b,disp)         M_MEM (0x28,a,b,disp)           /* 32 load    */
#define M_LLD(a,b,disp)         M_MEM (0x29,a,b,disp)           /* 64 load    */
#define M_ALD(a,b,disp)         M_MEM (0x29,a,b,disp)           /* addr load  */
#define M_BST(a,b,disp)         M_MEM (0x0e,a,b,disp)           /*  8 store   */
#define M_SST(a,b,disp)         M_MEM (0x0d,a,b,disp)           /* 16 store   */
#define M_IST(a,b,disp)         M_MEM (0x2c,a,b,disp)           /* 32 store   */
#define M_LST(a,b,disp)         M_MEM (0x2d,a,b,disp)           /* 64 store   */
#define M_AST(a,b,disp)         M_MEM (0x2d,a,b,disp)           /* addr store */

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

#define M_IADD(a,b,c)           M_OP3 (0x10,0x0,  a,b,c,0)      /* 32 add     */
#define M_LADD(a,b,c)           M_OP3 (0x10,0x20, a,b,c,0)      /* 64 add     */
#define M_ISUB(a,b,c)           M_OP3 (0x10,0x09, a,b,c,0)      /* 32 sub     */
#define M_LSUB(a,b,c)           M_OP3 (0x10,0x29, a,b,c,0)      /* 64 sub     */
#define M_IMUL(a,b,c)           M_OP3 (0x13,0x00, a,b,c,0)      /* 32 mul     */
#define M_LMUL(a,b,c)           M_OP3 (0x13,0x20, a,b,c,0)      /* 64 mul     */

#define M_IADD_IMM(a,b,c)       M_OP3 (0x10,0x0,  a,b,c,1)      /* 32 add     */
#define M_LADD_IMM(a,b,c)       M_OP3 (0x10,0x20, a,b,c,1)      /* 64 add     */
#define M_ISUB_IMM(a,b,c)       M_OP3 (0x10,0x09, a,b,c,1)      /* 32 sub     */
#define M_LSUB_IMM(a,b,c)       M_OP3 (0x10,0x29, a,b,c,1)      /* 64 sub     */
#define M_IMUL_IMM(a,b,c)       M_OP3 (0x13,0x00, a,b,c,1)      /* 32 mul     */
#define M_LMUL_IMM(a,b,c)       M_OP3 (0x13,0x20, a,b,c,1)      /* 64 mul     */

#define M_CMPEQ(a,b,c)          M_OP3 (0x10,0x2d, a,b,c,0)      /* c = a == b */
#define M_CMPLT(a,b,c)          M_OP3 (0x10,0x4d, a,b,c,0)      /* c = a <  b */
#define M_CMPLE(a,b,c)          M_OP3 (0x10,0x6d, a,b,c,0)      /* c = a <= b */

#define M_CMPULE(a,b,c)         M_OP3 (0x10,0x3d, a,b,c,0)      /* c = a <= b */
#define M_CMPULT(a,b,c)         M_OP3 (0x10,0x1d, a,b,c,0)      /* c = a <= b */

#define M_CMPEQ_IMM(a,b,c)      M_OP3 (0x10,0x2d, a,b,c,1)      /* c = a == b */
#define M_CMPLT_IMM(a,b,c)      M_OP3 (0x10,0x4d, a,b,c,1)      /* c = a <  b */
#define M_CMPLE_IMM(a,b,c)      M_OP3 (0x10,0x6d, a,b,c,1)      /* c = a <= b */

#define M_CMPULE_IMM(a,b,c)     M_OP3 (0x10,0x3d, a,b,c,1)      /* c = a <= b */
#define M_CMPULT_IMM(a,b,c)     M_OP3 (0x10,0x1d, a,b,c,1)      /* c = a <= b */

#define M_AND(a,b,c)            M_OP3 (0x11,0x00, a,b,c,0)      /* c = a &  b */
#define M_OR( a,b,c)            M_OP3 (0x11,0x20, a,b,c,0)      /* c = a |  b */
#define M_XOR(a,b,c)            M_OP3 (0x11,0x40, a,b,c,0)      /* c = a ^  b */

#define M_AND_IMM(a,b,c)        M_OP3 (0x11,0x00, a,b,c,1)      /* c = a &  b */
#define M_OR_IMM( a,b,c)        M_OP3 (0x11,0x20, a,b,c,1)      /* c = a |  b */
#define M_XOR_IMM(a,b,c)        M_OP3 (0x11,0x40, a,b,c,1)      /* c = a ^  b */

#define M_MOV(a,c)              M_OR (a,a,c)                    /* c = a      */
#define M_CLR(c)                M_OR (31,31,c)                  /* c = 0      */
#define M_NOP                   M_OR (31,31,31)                 /* ;          */

#define M_SLL(a,b,c)            M_OP3 (0x12,0x39, a,b,c,0)      /* c = a << b */
#define M_SRA(a,b,c)            M_OP3 (0x12,0x3c, a,b,c,0)      /* c = a >> b */
#define M_SRL(a,b,c)            M_OP3 (0x12,0x34, a,b,c,0)      /* c = a >>>b */

#define M_SLL_IMM(a,b,c)        M_OP3 (0x12,0x39, a,b,c,1)      /* c = a << b */
#define M_SRA_IMM(a,b,c)        M_OP3 (0x12,0x3c, a,b,c,1)      /* c = a >> b */
#define M_SRL_IMM(a,b,c)        M_OP3 (0x12,0x34, a,b,c,1)      /* c = a >>>b */

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

#define M_FADDS(a,b,c)          M_FOP3 (0x16, 0x580, a,b,c)     /* flt add    */
#define M_DADDS(a,b,c)          M_FOP3 (0x16, 0x5a0, a,b,c)     /* dbl add    */
#define M_FSUBS(a,b,c)          M_FOP3 (0x16, 0x581, a,b,c)     /* flt sub    */
#define M_DSUBS(a,b,c)          M_FOP3 (0x16, 0x5a1, a,b,c)     /* dbl sub    */
#define M_FMULS(a,b,c)          M_FOP3 (0x16, 0x582, a,b,c)     /* flt mul    */
#define M_DMULS(a,b,c)          M_FOP3 (0x16, 0x5a2, a,b,c)     /* dbl mul    */
#define M_FDIVS(a,b,c)          M_FOP3 (0x16, 0x583, a,b,c)     /* flt div    */
#define M_DDIVS(a,b,c)          M_FOP3 (0x16, 0x5a3, a,b,c)     /* dbl div    */

#define M_CVTDF(b,c)            M_FOP3 (0x16, 0x0ac, 31,b,c)    /* dbl2long   */
#define M_CVTLF(b,c)            M_FOP3 (0x16, 0x0bc, 31,b,c)    /* long2flt   */
#define M_CVTLD(b,c)            M_FOP3 (0x16, 0x0be, 31,b,c)    /* long2dbl   */
#define M_CVTDL(b,c)            M_FOP3 (0x16, 0x1af, 31,b,c)    /* dbl2long   */
#define M_CVTDL_C(b,c)          M_FOP3 (0x16, 0x12f, 31,b,c)    /* dbl2long   */
#define M_CVTLI(b,c)            M_FOP3 (0x17, 0x130, 31,b,c)    /* long2int   */

#define M_CVTDFS(b,c)           M_FOP3 (0x16, 0x5ac, 31,b,c)    /* dbl2long   */
#define M_CVTDLS(b,c)           M_FOP3 (0x16, 0x5af, 31,b,c)    /* dbl2long   */
#define M_CVTDL_CS(b,c)         M_FOP3 (0x16, 0x52f, 31,b,c)    /* dbl2long   */
#define M_CVTLIS(b,c)           M_FOP3 (0x17, 0x530, 31,b,c)    /* long2int   */

#define M_FCMPEQ(a,b,c)         M_FOP3 (0x16, 0x0a5, a,b,c)     /* c = a==b   */
#define M_FCMPLT(a,b,c)         M_FOP3 (0x16, 0x0a6, a,b,c)     /* c = a<b    */

#define M_FCMPEQS(a,b,c)        M_FOP3 (0x16, 0x5a5, a,b,c)     /* c = a==b   */
#define M_FCMPLTS(a,b,c)        M_FOP3 (0x16, 0x5a6, a,b,c)     /* c = a<b    */

#define M_FMOV(fa,fb)           M_FOP3 (0x17, 0x020, fa,fa,fb)  /* b = a      */
#define M_FMOVN(fa,fb)          M_FOP3 (0x17, 0x021, fa,fa,fb)  /* b = -a     */

#define M_FNOP                  M_FMOV (31,31)

#define M_FBEQZ(fa,disp)        M_BRA (0x31,fa,disp)            /* br a == 0.0*/

/* macros for special commands (see an Alpha-manual for description) **********/ 

#define M_TRAPB                 M_MEM (0x18,0,0,0x0000)        /* trap barrier*/

#define M_S4ADDL(a,b,c)         M_OP3 (0x10,0x02, a,b,c,0)     /* c = a*4 + b */
#define M_S4ADDQ(a,b,c)         M_OP3 (0x10,0x22, a,b,c,0)     /* c = a*4 + b */
#define M_S4SUBL(a,b,c)         M_OP3 (0x10,0x0b, a,b,c,0)     /* c = a*4 - b */
#define M_S4SUBQ(a,b,c)         M_OP3 (0x10,0x2b, a,b,c,0)     /* c = a*4 - b */
#define M_S8ADDL(a,b,c)         M_OP3 (0x10,0x12, a,b,c,0)     /* c = a*8 + b */
#define M_S8ADDQ(a,b,c)         M_OP3 (0x10,0x32, a,b,c,0)     /* c = a*8 + b */
#define M_S8SUBL(a,b,c)         M_OP3 (0x10,0x1b, a,b,c,0)     /* c = a*8 - b */
#define M_S8SUBQ(a,b,c)         M_OP3 (0x10,0x3b, a,b,c,0)     /* c = a*8 - b */
#define M_SAADDQ(a,b,c)         M_S8ADDQ(a,b,c)                /* c = a*8 + b */

#define M_S4ADDL_IMM(a,b,c)     M_OP3 (0x10,0x02, a,b,c,1)     /* c = a*4 + b */
#define M_S4ADDQ_IMM(a,b,c)     M_OP3 (0x10,0x22, a,b,c,1)     /* c = a*4 + b */
#define M_S4SUBL_IMM(a,b,c)     M_OP3 (0x10,0x0b, a,b,c,1)     /* c = a*4 - b */
#define M_S4SUBQ_IMM(a,b,c)     M_OP3 (0x10,0x2b, a,b,c,1)     /* c = a*4 - b */
#define M_S8ADDL_IMM(a,b,c)     M_OP3 (0x10,0x12, a,b,c,1)     /* c = a*8 + b */
#define M_S8ADDQ_IMM(a,b,c)     M_OP3 (0x10,0x32, a,b,c,1)     /* c = a*8 + b */
#define M_S8SUBL_IMM(a,b,c)     M_OP3 (0x10,0x1b, a,b,c,1)     /* c = a*8 - b */
#define M_S8SUBQ_IMM(a,b,c)     M_OP3 (0x10,0x3b, a,b,c,1)     /* c = a*8 - b */

#define M_LLD_U(a,b,disp)       M_MEM (0x0b,a,b,disp)          /* unalign ld  */
#define M_LST_U(a,b,disp)       M_MEM (0x0f,a,b,disp)          /* unalign st  */

#define M_ZAP(a,b,c)            M_OP3 (0x12,0x30, a,b,c,0)
#define M_ZAPNOT(a,b,c)         M_OP3 (0x12,0x31, a,b,c,0)

#define M_ZAP_IMM(a,b,c)        M_OP3 (0x12,0x30, a,b,c,1)
#define M_ZAPNOT_IMM(a,b,c)     M_OP3 (0x12,0x31, a,b,c,1)

#define M_BZEXT(a,b)            M_ZAPNOT_IMM(a, 0x01, b)       /*  8 zeroext  */
#define M_CZEXT(a,b)            M_ZAPNOT_IMM(a, 0x03, b)       /* 16 zeroext  */
#define M_IZEXT(a,b)            M_ZAPNOT_IMM(a, 0x0f, b)       /* 32 zeroext  */

#define M_EXTBL(a,b,c)          M_OP3 (0x12,0x06, a,b,c,0)
#define M_EXTWL(a,b,c)          M_OP3 (0x12,0x16, a,b,c,0)
#define M_EXTLL(a,b,c)          M_OP3 (0x12,0x26, a,b,c,0)
#define M_EXTQL(a,b,c)          M_OP3 (0x12,0x36, a,b,c,0)
#define M_EXTWH(a,b,c)          M_OP3 (0x12,0x5a, a,b,c,0)
#define M_EXTLH(a,b,c)          M_OP3 (0x12,0x6a, a,b,c,0)
#define M_EXTQH(a,b,c)          M_OP3 (0x12,0x7a, a,b,c,0)
#define M_INSBL(a,b,c)          M_OP3 (0x12,0x0b, a,b,c,0)
#define M_INSWL(a,b,c)          M_OP3 (0x12,0x1b, a,b,c,0)
#define M_INSLL(a,b,c)          M_OP3 (0x12,0x2b, a,b,c,0)
#define M_INSQL(a,b,c)          M_OP3 (0x12,0x3b, a,b,c,0)
#define M_INSWH(a,b,c)          M_OP3 (0x12,0x57, a,b,c,0)
#define M_INSLH(a,b,c)          M_OP3 (0x12,0x67, a,b,c,0)
#define M_INSQH(a,b,c)          M_OP3 (0x12,0x77, a,b,c,0)
#define M_MSKBL(a,b,c)          M_OP3 (0x12,0x02, a,b,c,0)
#define M_MSKWL(a,b,c)          M_OP3 (0x12,0x12, a,b,c,0)
#define M_MSKLL(a,b,c)          M_OP3 (0x12,0x22, a,b,c,0)
#define M_MSKQL(a,b,c)          M_OP3 (0x12,0x32, a,b,c,0)
#define M_MSKWH(a,b,c)          M_OP3 (0x12,0x52, a,b,c,0)
#define M_MSKLH(a,b,c)          M_OP3 (0x12,0x62, a,b,c,0)
#define M_MSKQH(a,b,c)          M_OP3 (0x12,0x72, a,b,c,0)

#define M_EXTBL_IMM(a,b,c)      M_OP3 (0x12,0x06, a,b,c,1)
#define M_EXTWL_IMM(a,b,c)      M_OP3 (0x12,0x16, a,b,c,1)
#define M_EXTLL_IMM(a,b,c)      M_OP3 (0x12,0x26, a,b,c,1)
#define M_EXTQL_IMM(a,b,c)      M_OP3 (0x12,0x36, a,b,c,1)
#define M_EXTWH_IMM(a,b,c)      M_OP3 (0x12,0x5a, a,b,c,1)
#define M_EXTLH_IMM(a,b,c)      M_OP3 (0x12,0x6a, a,b,c,1)
#define M_EXTQH_IMM(a,b,c)      M_OP3 (0x12,0x7a, a,b,c,1)
#define M_INSBL_IMM(a,b,c)      M_OP3 (0x12,0x0b, a,b,c,1)
#define M_INSWL_IMM(a,b,c)      M_OP3 (0x12,0x1b, a,b,c,1)
#define M_INSLL_IMM(a,b,c)      M_OP3 (0x12,0x2b, a,b,c,1)
#define M_INSQL_IMM(a,b,c)      M_OP3 (0x12,0x3b, a,b,c,1)
#define M_INSWH_IMM(a,b,c)      M_OP3 (0x12,0x57, a,b,c,1)
#define M_INSLH_IMM(a,b,c)      M_OP3 (0x12,0x67, a,b,c,1)
#define M_INSQH_IMM(a,b,c)      M_OP3 (0x12,0x77, a,b,c,1)
#define M_MSKBL_IMM(a,b,c)      M_OP3 (0x12,0x02, a,b,c,1)
#define M_MSKWL_IMM(a,b,c)      M_OP3 (0x12,0x12, a,b,c,1)
#define M_MSKLL_IMM(a,b,c)      M_OP3 (0x12,0x22, a,b,c,1)
#define M_MSKQL_IMM(a,b,c)      M_OP3 (0x12,0x32, a,b,c,1)
#define M_MSKWH_IMM(a,b,c)      M_OP3 (0x12,0x52, a,b,c,1)
#define M_MSKLH_IMM(a,b,c)      M_OP3 (0x12,0x62, a,b,c,1)
#define M_MSKQH_IMM(a,b,c)      M_OP3 (0x12,0x72, a,b,c,1)

#define M_UMULH(a,b,c)          M_OP3 (0x13,0x30, a,b,c,0)     /* 64 umulh    */

#define M_UMULH_IMM(a,b,c)      M_OP3 (0x13,0x30, a,b,c,1)     /* 64 umulh    */

#define M_CMOVEQ(a,b,c)         M_OP3 (0x11,0x24, a,b,c,0)     /* a==0 ? c=b  */
#define M_CMOVNE(a,b,c)         M_OP3 (0x11,0x26, a,b,c,0)     /* a!=0 ? c=b  */
#define M_CMOVLT(a,b,c)         M_OP3 (0x11,0x44, a,b,c,0)     /* a< 0 ? c=b  */
#define M_CMOVGE(a,b,c)         M_OP3 (0x11,0x46, a,b,c,0)     /* a>=0 ? c=b  */
#define M_CMOVLE(a,b,c)         M_OP3 (0x11,0x64, a,b,c,0)     /* a<=0 ? c=b  */
#define M_CMOVGT(a,b,c)         M_OP3 (0x11,0x66, a,b,c,0)     /* a> 0 ? c=b  */

#define M_CMOVEQ_IMM(a,b,c)     M_OP3 (0x11,0x24, a,b,c,1)     /* a==0 ? c=b  */
#define M_CMOVNE_IMM(a,b,c)     M_OP3 (0x11,0x26, a,b,c,1)     /* a!=0 ? c=b  */
#define M_CMOVLT_IMM(a,b,c)     M_OP3 (0x11,0x44, a,b,c,1)     /* a< 0 ? c=b  */
#define M_CMOVGE_IMM(a,b,c)     M_OP3 (0x11,0x46, a,b,c,1)     /* a>=0 ? c=b  */
#define M_CMOVLE_IMM(a,b,c)     M_OP3 (0x11,0x64, a,b,c,1)     /* a<=0 ? c=b  */
#define M_CMOVGT_IMM(a,b,c)     M_OP3 (0x11,0x66, a,b,c,1)     /* a> 0 ? c=b  */

/* macros for unused commands (see an Alpha-manual for description) ***********/ 

#define M_ANDNOT(a,b,c,const)   M_OP3 (0x11,0x08, a,b,c,const) /* c = a &~ b  */
#define M_ORNOT(a,b,c,const)    M_OP3 (0x11,0x28, a,b,c,const) /* c = a |~ b  */
#define M_XORNOT(a,b,c,const)   M_OP3 (0x11,0x48, a,b,c,const) /* c = a ^~ b  */

#define M_CMPBGE(a,b,c,const)   M_OP3 (0x10,0x0f, a,b,c,const)

#define M_FCMPUN(a,b,c)         M_FOP3 (0x16, 0x0a4, a,b,c)    /* unordered   */
#define M_FCMPLE(a,b,c)         M_FOP3 (0x16, 0x0a7, a,b,c)    /* c = a<=b    */

#define M_FCMPUNS(a,b,c)        M_FOP3 (0x16, 0x5a4, a,b,c)    /* unordered   */
#define M_FCMPLES(a,b,c)        M_FOP3 (0x16, 0x5a7, a,b,c)    /* c = a<=b    */

#define M_FBNEZ(fa,disp)        M_BRA (0x35,fa,disp)
#define M_FBLEZ(fa,disp)        M_BRA (0x33,fa,disp)

#define M_JMP_CO(a,b)           M_MEM (0x1a,a,b,0xc000)        /* call cosub  */


/* function gen_resolvebranch **************************************************

    backpatches a branch instruction; Alpha branch instructions are very
    regular, so it is only necessary to overwrite some fixed bits in the
    instruction.

    parameters: ip ... pointer to instruction after branch (void*)
                so ... offset of instruction after branch  (s4)
                to ... offset of branch target             (s4)

*******************************************************************************/

#define gen_resolvebranch(ip,so,to) \
    do { \
        *((s4 *) (((u1 *) (ip)) - 4)) = ((s4) ((to) - (so))); \
    } while (0)

#define SOFTNULLPTRCHECK       /* soft null pointer check supportet as option */

#endif
