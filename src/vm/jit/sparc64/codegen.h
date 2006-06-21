/* vm/jit/sparc64/codegen.h - code generation macros and definitions for Sparc

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Contact: cacao@cacaojvm.org

   Authors: Andreas Krall
            Reinhard Grafl
            Alexander Jordan

   Changes:

   $Id: codegen.h 4722 2006-04-03 15:36:00Z twisti $

*/

#ifndef _CODEGEN_H
#define _CODEGEN_H

#include "config.h"
#include "vm/types.h"

#include "vm/jit/jit.h"


/* additional functions and macros to generate code ***************************/

#define gen_nullptr_check(objreg) \
    if (checknull) { \
        M_BEQZ(objreg, 0); \
        codegen_add_nullpointerexception_ref(cd); \
        M_NOP; \
    }

#define gen_bound_check \
    if (checkbounds) { \
        M_ILD(REG_ITMP3, s1, OFFSET(java_arrayheader, size)); \
        M_CMP(s2, REG_ITMP3); \
        M_XBUGE(0); \
        codegen_add_arrayindexoutofboundsexception_ref(cd, s2); \
        M_NOP; \
    }

#define gen_div_check(r) \
    do { \
        M_BEQZ((r), 0); \
        codegen_add_arithmeticexception_ref(cd); \
        M_NOP; \
    } while (0)

/* MCODECHECK(icnt) */

#define MCODECHECK(icnt) \
	do { \
		if ((cd->mcodeptr + (icnt) * 4) > cd->mcodeend) \
        		codegen_increase(cd); \
	} while (0)


#define ALIGNCODENOP \
    if ((s4) ((ptrint) mcodeptr & 7)) { \
        M_NOP; \
    }


/* M_INTMOVE:
     generates an integer-move from register rs to rd.
     if rs and rd are the same int-register, no code will be generated.
*/ 

#define M_INTMOVE(rs,rd) if (rs != rd) { M_MOV(rs, rd); }


/* M_DBLMOVE:
    generates a double floating-point-move from register (pair) rs to rd.
    if rs and rd are the same double-register, no code will be generated
*/ 

#define M_DBLMOVE(rs, rd) if (rs != rd) { M_DMOV (rs, rd); }


/* M_FLTMOVE:
    generates a double floating-point-move from pseudo register rs to rd.
	(ie. lower register of double rs pair to lower register of double rd pair)
    if rs and rd are the same double-register, no code will be generated
*/ 
#define M_FLTMOVE(rs, rd) if (rs != rd) { M_FMOV (rs, rd); }



#define M_COPY(s,d)                     emit_copy(jd, iptr, (s), (d))




/********************** instruction formats ***********************************/

#define REG   	0
#define IMM 	1

/* 3-address-operations: M_OP3
 *       op  ..... opcode
 *       op3 ..... operation
 *       rs1 ..... register number source 1
 *       rs2 ..... register number or constant integer source 2
 *       rd  ..... register number destination
 *       imm ..... switch to use rs2 as constant 13bit integer 
 *                  (REG means: use b as register number)
 *                  (IMM means: use b as signed immediate value)
 *                                                                       */

#define M_OP3(op,op3,rd,rs1,rs2,imm) \
	*(mcodeptr++) =  ((((s4) (op)) << 30) | ((rd) << 25) | ((op3) << 19) | ((rs1) << 14) | ((imm)<<13) | (imm?((rs2)&0x1fff):(rs2)) )

/* 3-address-operations: M_OP3C
 *       rcond ... condition opcode
 *       rs2 ..... register number or 10bit signed immediate
 *
 */
 
#define M_OP3C(op,op3,rcond,rd,rs1,rs2,imm) \
	*(mcodeptr++) =  ((((s4) (op)) << 30) | ((rd) << 25) | ((op3) << 19) | ((rs1) << 14) | ((imm)<<13) | \
		((rcond) << 10) | (imm?((rs2)&0x3ff):(rs2)) )
	


/* shift Format 3
 *    op ..... opcode
 *    op3..... op3 code
 *    rs1 .... source 1
 *    rs2 .... source 2 or constant
 *    rd ..... dest reg
 *    imm .... switch for constant
 *    x ...... 0 => 32, 1 => 64 bit shift 
 */
#define M_SHFT(op,op3,rs1,rs2,rd,imm,x) \
	*(mcodeptr++) =  ( (((s4)(op)) << 30) | ((op3) << 19) | ((rd) << 25) | ((rs1) << 14) | ((rs2) << 0) | \
		      ((imm) << 13) | ((x) << 12)  )

/* Format 4
 *    op ..... opcode
 *    op3..... op3 code
 *    cond ... condition opcode
 *    rs2 .... source 2 or signed 11-bit constant
 *    rd ..... dest reg
 *    imm .... switch for constant
 *    cc{0-2}  32-bit 64-bit or fp condition
 */
 
 #define M_FMT4(op,op3,rd,rs2,cond,cc2,cc1,cc0,imm) \
 	*(mcodeptr++) =  ( (((s4)(op)) << 30) | ((op3) << 19) | ((rd) << 25) | ((cc2) << 18) |  ((cond) << 14) | \
 		((imm) << 13) | ((cc1) << 12) | ((cc0) << 11) | ((rs2) << 0) )




/* 3-address-floating-point-operation
     op .... opcode
     op3,opf .... function-number
     XXX
*/ 
#define M_FOP3(op,op3,opf,rd,rs1,rs2) \
	*(mcodeptr++) =  ( (((s4)(op))<<30) | ((rd)<<25) | ((op3)<<19) | ((rs1) << 14) | ((opf)<<5) | (rs2) )


/**** format 2 operations ********/

/* branch on integer reg instruction 
      op ..... opcode
      rcond ...... condition to be tested
      disp16 ... 16-bit relative address to be jumped to (divided by 4)
      rs1 ..... register to be tested
      p ..... prediction bit
      anul .... annullment bit
*/
#define M_BRAREG(op,rcond,rs1,disp16,p,anul) \
	*(mcodeptr++) = ( (((s4)(op))<<30) | ((anul)<<29) | (0<<28) | ((rcond)<<25) | (3<<22) | \
		( ((disp16)& 0xC000) << 6 ) | (p << 19) | ((rs1) << 14) | ((disp16)&0x3fff) )
		
/* branch on integer reg instruction 
      op,op2 .... opcodes
      cond ...... condition to be tested
      disp19 ... 19-bit relative address to be jumped to (divided by 4)
      ccx ..... 32(0) or 64(2) bit test
      p ..... prediction bit
      anul .... annullment bit
*/		
#define M_BRACC(op,op2,cond,disp19,ccx,p,anul) \
	*(mcodeptr++) = ( (((s4)(op))<<30) | ((anul)<<29) | ((cond)<<25) | (op2<<22) | (ccx<<20) | \
		(p << 19 ) | (disp19)  )    
        
/************** end-user instructions (try to follow asm style) ***************/


#define M_SETHI(imm22, rd) \
	*(mcodeptr++) = ((((s4)(0x00)) << 30) | ((rd) << 25) | ((0x04)<<22) | ((imm22)&0x3FFFFF) )


#define M_NOP (M_SETHI(0,0))	/* nop	*/

#define M_AND(rs1,rs2,rd)       M_OP3(0x02,0x01,rd,rs1,rs2,REG)     /* 64b c = a &  b */
#define M_AND_IMM(rs1,rs2,rd)	M_OP3(0x02,0x01,rd,rs1,rs2,IMM)
#define M_ANDCC(rs1,rs2,rd)		M_OP3(0x02,0x11,rd,rs1,rs2,REG)
#define M_ANDCC_IMM(rs1,rs2,rd)	M_OP3(0x02,0x11,rd,rs1,rs2,IMM)

#define M_OR(rs1,rs2,rd)        M_OP3(0x02,0x02,rd,rs1,rs2,REG)     /* rd = rs1 | rs2     */
#define M_OR_IMM(rs1,rs2,rd)    M_OP3(0x02,0x02,rd,rs1,rs2,IMM)
#define M_XOR(rs1,rs2,rd)       M_OP3(0x02,0x03,rs1,rs2,rd,REG)     /* rd = rs1 ^  rs2    */
#define M_XOR_IMM(rs1,rs2,rd)   M_OP3(0x02,0x03,rs1,rs2,rd,IMM)

#define M_MOV(rs,rd)            M_OR(REG_ZERO, rs, rd)              /* rd = rs            */



#define M_SLLX(rs1,rs2,rd)		M_SHFT(0x02,0x25,rs1,rs2,rd,REG,1)	/* 64b rd = rs << rs2 */
#define M_SLLX_IMM(rs1,rs2,rd)	M_SHFT(0x02,0x25,rs1,rs2,rd,IMM,1)
#define M_SRLX(rs1,rs2,rd)		M_SHFT(0x02,0x26,rs1,rs2,rd,REG,1)	/* 64b rd = rs >>>rs2 */
#define M_SRLX_IMM(rs1,rs2,rd)	M_SHFT(0x02,0x26,rs1,rs2,rd,IMM,1)
#define M_SRL(rs1,rs2,rd)		M_SHFT(0x02,0x26,rs1,rs2,rd,REG,0)	/* 32b rd = rs >>>rs2 */
#define M_SRL_IMM(rs1,rs2,rd)	M_SHFT(0x02,0x26,rs1,rs2,rd,IMM,0)
#define M_SRAX(rs1,rs2,rd)		M_SHFT(0x02,0x27,rs1,rs2,rd,REG,1)	/* 64b rd = rs >> rs2 */
#define M_SRAX_IMM(rs1,rs2,rd)	M_SHFT(0x02,0x27,rs1,rs2,rd,IMM,1)
#define M_SRA(rs1,rs2,rd)		M_SHFT(0x02,0x27,rs1,rs2,rd,REG,0)	/* 32b rd = rs >> rs2 */
#define M_SRA_IMM(rs1,rs2,rd)	M_SHFT(0x02,0x27,rs1,rs2,rd,IMM,0)

#define M_ISEXT(rs,rd) 			M_SRA_IMM(rs,0,rd)                  /* sign extend 32 bits*/


#define M_ADD(rs1,rs2,rd)   	M_OP3(0x02,0x00,rd,rs1,rs2,REG)  	/* 64b rd = rs1 + rs2 */
#define M_ADD_IMM(rs1,rs2,rd)   M_OP3(0x02,0x00,rd,rs1,rs2,IMM)
#define M_SUB(rs1,rs2,rd)       M_OP3(0x02,0x04,rd,rs1,rs2,REG) 	/* 64b rd = rs1 - rs2 */
#define M_SUB_IMM(rs1,rs2,rd)   M_OP3(0x02,0x04,rd,rs1,rs2,IMM)
#define M_MULX(rs1,rs2,rd)      M_OP3(0x02,0x09,rd,rs1,rs2,REG)  	/* 64b rd = rs1 * rs2 */
#define M_MULX_IMM(rs1,rs2,rd)  M_OP3(0x02,0x09,rd,rs1,rs2,IMM)
#define M_DIVX(rs1,rs2,rd)      M_OP3(0x02,0x2d,rd,rs1,rs2,REG)  	/* 64b rd = rs1 / rs2 */



/**** compare and conditional ALU operations ***********/

#define M_CMP(rs1,rs2)          M_SUB(rs1,rs2,REG_ZERO)             /* sets xcc and icc   */
#define M_CMP_IMM(rs1,rs2)      M_SUB_IMM(rs1,rs2,REG_ZERO)

/* move integer register on (64-bit) condition */

#define M_XCMOVEQ(rs,rd)        M_FMT4(0x2,0x2c,rd,rs,0x1,1,1,0,REG)   /* a==b ? rd=rs    */
#define M_XCMOVNE(rs,rd)        M_FMT4(0x2,0x2c,rd,rs,0x9,1,1,0,REG)   /* a!=b ? rd=rs    */
#define M_XCMOVLT(rs,rd)        M_FMT4(0x2,0x2c,rd,rs,0x3,1,1,0,REG)   /* a<b  ? rd=rs    */
#define M_XCMOVGE(rs,rd)        M_FMT4(0x2,0x2c,rd,rs,0xb,1,1,0,REG)   /* a>=b ? rd=rs    */
#define M_XCMOVLE(rs,rd)        M_FMT4(0x2,0x2c,rd,rs,0x2,1,1,0,REG)   /* a<=b ? rd=rs    */
#define M_XCMOVGT(rs,rd)        M_FMT4(0x2,0x2c,rd,rs,0xa,1,1,0,REG)   /* a>b  ? rd=rs    */

#define M_XCMOVEQ_IMM(rs,rd)    M_FMT4(0x2,0x2c,rd,rs,0x1,1,1,0,IMM)   /* a==b ? rd=rs    */
#define M_XCMOVNE_IMM(rs,rd)    M_FMT4(0x2,0x2c,rd,rs,0x9,1,1,0,IMM)   /* a!=b ? rd=rs    */
#define M_XCMOVLT_IMM(rs,rd)    M_FMT4(0x2,0x2c,rd,rs,0x3,1,1,0,IMM)   /* a<b  ? rd=rs    */
#define M_XCMOVGE_IMM(rs,rd)    M_FMT4(0x2,0x2c,rd,rs,0xb,1,1,0,IMM)   /* a>=b ? rd=rs    */
#define M_XCMOVLE_IMM(rs,rd)    M_FMT4(0x2,0x2c,rd,rs,0x2,1,1,0,IMM)   /* a<=b ? rd=rs    */
#define M_XCMOVGT_IMM(rs,rd)    M_FMT4(0x2,0x2c,rd,rs,0xa,1,1,0,IMM)   /* a>b  ? rd=rs    */

/* move integer register on (fcc0) floating point condition */

#define M_CMOVFGT_IMM(rs,rd)    M_FMT4(0x2,0x2c,rd,rs,0x6,0,0,0,IMM)   /* fa>fb  ? rd=rs  */
#define M_CMOVFLT_IMM(rs,rd)    M_FMT4(0x2,0x2c,rd,rs,0x4,0,0,0,IMM)   /* fa<fb  ? rd=rs  */
#define M_CMOVFEQ_IMM(rs,rd)    M_FMT4(0x2,0x2c,rd,rs,0x9,0,0,0,IMM)   /* fa==fb ? rd=rs  */

/* move integer register on (32-bit) condition */



/* move integer register on register condition */

#define M_CMOVREQ(rs1,rs2,rd) M_OP3C(0x2,0x2f,0x1,rd,rs1,rs2,REG)      /* rs1==0 ? rd=rs2 */
#define M_CMOVRNE(rs1,rs2,rd) M_OP3C(0x2,0x2f,0x5,rd,rs1,rs2,REG)      /* rs1!=0 ? rd=rs2 */
#define M_CMOVRLE(rs1,rs2,rd) M_OP3C(0x2,0x2f,0x2,rd,rs1,rs2,REG)      /* rs1<=0 ? rd=rs2 */
#define M_CMOVRLT(rs1,rs2,rd) M_OP3C(0x2,0x2f,0x3,rd,rs1,rs2,REG)      /* rs1<0  ? rd=rs2 */
#define M_CMOVRGT(rs1,rs2,rd) M_OP3C(0x2,0x2f,0x6,rd,rs1,rs2,REG)      /* rs1>0  ? rd=rs2 */
#define M_CMOVRGE(rs1,rs2,rd) M_OP3C(0x2,0x2f,0x7,rd,rs1,rs2,REG)      /* rs1>=0 ? rd=rs2 */

#define M_CMOVREQ_IMM(rs1,rs2,rd) M_OP3C(0x2,0x2f,0x1,rd,rs1,rs2,IMM)  /* rs1==0 ? rd=rs2 */
#define M_CMOVRNE_IMM(rs1,rs2,rd) M_OP3C(0x2,0x2f,0x5,rd,rs1,rs2,IMM)  /* rs1!=0 ? rd=rs2 */
#define M_CMOVRLE_IMM(rs1,rs2,rd) M_OP3C(0x2,0x2f,0x2,rd,rs1,rs2,IMM)  /* rs1<=0 ? rd=rs2 */
#define M_CMOVRLT_IMM(rs1,rs2,rd) M_OP3C(0x2,0x2f,0x3,rd,rs1,rs2,IMM)  /* rs1<0  ? rd=rs2 */
#define M_CMOVRGT_IMM(rs1,rs2,rd) M_OP3C(0x2,0x2f,0x6,rd,rs1,rs2,IMM)  /* rs1>0  ? rd=rs2 */
#define M_CMOVRGE_IMM(rs1,rs2,rd) M_OP3C(0x2,0x2f,0x7,rd,rs1,rs2,IMM)  /* rs1>=0 ? rd=rs2 */


/**** load/store operations ********/

#define M_SLDU(rd,rs,disp)      M_OP3(0x03,0x02,rd,rs,disp,IMM)        /* 16-bit load, uns*/
#define M_SLDS(rd,rs,disp)      M_OP3(0x03,0x0a,rd,rs,disp,IMM)        /* 16-bit load, sig*/
#define M_BLDS(rd,rs,disp)      M_OP3(0x03,0x09,rd,rs,disp,IMM)        /* 8-bit load, sig */


#define M_LDX_INTERN(rd,rs,disp) M_OP3(0x03,0x0b,rd,rs,disp,IMM)       /* 64-bit load, sig*/
#define M_LDX(rd,rs,disp) \
	do { \
        s4 lo = (short) (disp); \
        s4 hi = (short) (((disp) - lo) >> 13); \
        if (hi == 0) { \
            M_LDX_INTERN(rd,rs,lo); \
        } else { \
            M_SETHI(hi&0x3ffff8,rd); \
            M_AADD(rs,rd,rd); \
            M_LDX_INTERN(rd,rd,lo); \
        } \
    } while (0)

#define M_ILD_INTERN(rd,rs,disp) M_OP3(0x03,0x08,rd,rs,disp,IMM)       /* 32-bit load, sig */
#define M_ILD(rd,rs,disp) \
	do { \
        s4 lo = (short) (disp); \
        s4 hi = (short) (((disp) - lo) >> 13); \
        if (hi == 0) { \
            M_ILD_INTERN(rd,rs,lo); \
        } else { \
            M_SETHI(hi&0x3ffff8,rd); \
            M_AADD(rs,rd,rd); \
            M_ILD_INTERN(rd,rd,lo); \
        } \
    } while (0)



#define M_SST(rd,rs,disp)       M_OP3(0x03,0x06,rd,rs,disp,IMM)        /* 16-bit store     */
#define M_BST(rd,rs,disp)       M_OP3(0x03,0x05,rd,rs,disp,IMM)        /*  8-bit store     */

/* Stores with displacement overflow should only happen with PUTFIELD or on   */
/* the stack. The PUTFIELD instruction does not use REG_ITMP3 and a           */
/* reg_of_var call should not use REG_ITMP3!!!                                */

#define M_STX_INTERN(rd,rs,disp) M_OP3(0x03,0x0e,rd,rs,disp,IMM)       /* 64-bit store    */
#define M_STX(rd,rs,disp) \
	do { \
        s4 lo = (short) (disp); \
        s4 hi = (short) (((disp) - lo) >> 13); \
        if (hi == 0) { \
            M_STX_INTERN(rd,rs,lo); \
        } else { \
            M_SETHI(hi&0x3ffff8,REG_ITMP3); /* sethi has a 22bit imm, only set upper 19 bits */ \
            M_AADD(rs,REG_ITMP3,REG_ITMP3); \
            M_STX_INTERN(rd,REG_ITMP3,lo); \
        } \
    } while (0)


#define M_IST_INTERN(rd,rs,disp) M_OP3(0x03,0x04,rd,rs,disp,IMM)       /* 32-bit store    */
#define M_IST(rd,rs,disp) \
    do { \
        s4 lo = (short) (disp); \
        s4 hi = (short) (((disp) - lo) >> 13); \
        if (hi == 0) { \
            M_IST_INTERN(rd,rs,lo); \
        } else { \
            M_SETHI(hi&0x3ffff8,REG_ITMP3); /* sethi has a 22bit imm, only set upper 19 bits */ \
            M_AADD(rs,REG_ITMP3,REG_ITMP3); \
            M_IST_INTERN(rd,REG_ITMP3,lo); \
        } \
    } while (0)


/**** branch operations ********/
/* XXX prediction and annul bits currently set to defaults, but could be used for optimizations */

/* branch on integer register */

#define M_BEQZ(r,disp)          M_BRAREG(0x0,0x1,r,disp,1,0)          /* br r == 0   */
#define M_BLEZ(r,disp)          M_BRAREG(0x0,0x2,r,disp,1,0)          /* br r <= 0   */
#define M_BLTZ(r,disp)          M_BRAREG(0x0,0x3,r,disp,1,0)          /* br r < 0    */
#define M_BNEZ(r,disp)          M_BRAREG(0x0,0x5,r,disp,1,0)          /* br r != 0   */
#define M_BGTZ(r,disp)          M_BRAREG(0x0,0x6,r,disp,1,0)          /* br r > 0    */
#define M_BGEZ(r,disp)          M_BRAREG(0x0,0x7,r,disp,1,0)          /* br r >= 0   */


/* branch on (64-bit) integer condition codes */

#define M_XBEQ(disp)            M_BRACC(0x00,0x1,0x1,disp,2,1,0)      /* branch a==b */
#define M_XBNEQ(disp)           M_BRACC(0x00,0x1,0x9,disp,2,1,0)      /* branch a!=b */
#define M_XBGT(disp)            M_BRACC(0x00,0x1,0xa,disp,2,1,0)      /* branch a>b  */
#define M_XBLT(disp)            M_BRACC(0x00,0x1,0x3,disp,2,1,0)      /* branch a<b  */
#define M_XBGE(disp)            M_BRACC(0x00,0x1,0xb,disp,2,1,0)      /* branch a>=b */
#define M_XBLE(disp)            M_BRACC(0x00,0x1,0x2,disp,2,1,0)      /* branch a<=b */
#define M_XBUGE(disp)           M_BRACC(0x00,0x1,0xd,disp,2,1,0)      /* br uns a>=b */

/* branch on (32-bit) integer condition codes */

#define M_BR(disp)              M_BRACC(0x00,0x1,0x8,disp,0,1,0)      /* branch      */
#define M_BEQ(disp)             M_BRACC(0x00,0x1,0x1,disp,0,1,0)      /* branch a==b */
#define M_BNEQ(disp)            M_BRACC(0x00,0x1,0x9,disp,0,1,0)      /* branch a!=b */
#define M_BGT(disp)             M_BRACC(0x00,0x1,0xa,disp,0,1,0)      /* branch a>b  */
#define M_BLT(disp)             M_BRACC(0x00,0x1,0x3,disp,0,1,0)      /* branch a<b  */
#define M_BGE(disp)             M_BRACC(0x00,0x1,0xb,disp,0,1,0)      /* branch a>=b */
#define M_BLE(disp)             M_BRACC(0x00,0x1,0x2,disp,0,1,0)      /* branch a<=b */







#define M_JMP(rd,rs1,rs2)       M_OP3(0x02,0x38,rd, rs1,rs2,REG)  /* jump to rs1+rs2, adr of instr. saved to rd */
#define M_JMP_IMM(rd,rs1,rs2)   M_OP3(0x02,0x38,rd, rs1,rs2,IMM)
#define M_RET(rs)				M_OP3(0x02,0x38,REG_ZERO,rs,REG_ZERO,REG)

#define M_RETURN(rs)            M_OP3(0x02,0x39,0,rs,REG_ZERO,REG) /* like ret, does window restore */
 
/**** floating point operations **/


#define M_DMOV(rs,rd)           M_FOP3(0x02,0x34,0x02,rd,0,rs)      /* rd = rs */
#define M_FMOV(rs,rd)           M_FOP3(0x02,0x34,0x01,rd*2,0,rs*2)  /* rd = rs */

#define M_FNEG(rs,rd)          	M_FOP3(0x02,0x34,0x05,rd,0,rs)	 	/* rd = -rs     */
#define M_DNEG(rs,rd)          	M_FOP3(0x02,0x34,0x06,rd,0,rs)  	/* rd = -rs     */

#define M_FADD(rs1,rs2,rd)      M_FOP3(0x02,0x34,0x41,rd,rs1,rs2)  /* float add    */
#define M_DADD(rs1,rs2,rd)      M_FOP3(0x02,0x34,0x42,rd,rs1,rs2)  /* double add   */
#define M_FSUB(rs1,rs2,rd)      M_FOP3(0x02,0x34,0x045,rd,rs1,rs2)	/* float sub    */
#define M_DSUB(rs1,rs2,rd)      M_FOP3(0x02,0x34,0x046,rd,rs1,rs2) /* double sub   */
#define M_FMUL(rs1,rs2,rd)      M_FOP3(0x02,0x34,0x049,rd,rs1,rs2) /* float mul    */
#define M_DMUL(rs1,rs2,rd)      M_FOP3(0x02,0x34,0x04a,rd,rs1,rs2) /* double mul   */
#define M_FDIV(rs1,rs2,rd)      M_FOP3(0x02,0x34,0x04d,rd,rs1,rs2) /* float div    */
#define M_DDIV(rs1,rs2,rd)      M_FOP3(0x02,0x34,0x04e,rd,rs1,rs2) /* double div   */


/**** compare and conditional FPU operations ***********/

/* rd field 0 ==> fcc target unit is fcc0 */
#define M_FCMP(rs1,rs2)		    M_FOP3(0x02,0x35,0x051,0,rs1*2,rs2*2) /* set fcc flt  */
#define M_DCMP(rs1,rs2)		    M_FOP3(0x02,0x35,0x052,0,rs1,rs2)     /* set fcc dbl  */

/* conversion functions */

#define M_CVTIF(rs,rd)          M_FOP3(0x02,0x34,0x0c4,rd*2,0,rs*2)/* int2flt      */
#define M_CVTID(rs,rd)          M_FOP3(0x02,0x34,0x0c8,rd,0,rs*2)  /* int2dbl      */
#define M_CVTLF(rs,rd)          M_FOP3(0x02,0x34,0x084,rd*2,0,rs)  /* long2flt     */
#define M_CVTLD(rs,rd)          M_FOP3(0x02,0x34,0x088,rd,0,rs)    /* long2dbl     */

#define M_CVTFI(rs,rd)          M_FOP3(0x02,0x34,0x0d1,rd*2,0,rs*2)   /* flt2int   */
#define M_CVTDI(rs,rd)          M_FOP3(0x02,0x34,0x0d2,rd*2,0,rs)     /* dbl2int   */
#define M_CVTFL(rs,rd)          M_FOP3(0x02,0x34,0x081,rd,0,rs*2)     /* flt2long  */
#define M_CVTDL(rs,rd)          M_FOP3(0x02,0x34,0x082,rd,0,rs)       /* dbl2long  */

#define M_CVTFD(rs,rd)          M_FOP3(0x02,0x34,0x0c9,rd,0,rs*2)     /* flt2dbl   */
#define M_CVTDF(rs,rd)          M_FOP3(0x02,0x34,0x0c6,rd*2,0,rs)     /* dbl2float */


/* translate logical double register index to float index. (e.g. %d1 -> %f2, %d2 -> %f4, etc.) */
/* we don't have to pack the 6-bit register number, since we are not using the upper 16 doubles */
/* floats reside in lower register of a double pair, use same translation as above */
#define M_DLD_INTERN(rd,rs1,disp) M_OP3(0x03,0x23,rd*2,rs1,disp,IMM)    /* double (64-bit) load */
#define M_DLD(rd,rs,disp) \
	do { \
        s4 lo = (short) (disp); \
        s4 hi = (short) (((disp) - lo) >> 13); \
        if (hi == 0) { \
            M_DLD_INTERN(rd,rs,lo); \
        } else { \
            M_SETHI(hi&0x3ffff8,rd); \
            M_AADD(rs,rd,rd); \
            M_DLD_INTERN(rd,rd,lo); \
        } \
    } while (0)
/* Note for SETHI: sethi has a 22bit imm, only set upper 19 bits */ 

#define M_FLD_INTERN(rd,rs1,disp) M_OP3(0x03,0x20,rd*2,rs1,disp,IMM)    /* float (32-bit) load */
#define M_FLD(rd,rs,disp) \
	do { \
        s4 lo = (short) (disp); \
        s4 hi = (short) (((disp) - lo) >> 13); \
        if (hi == 0) { \
            M_FLD_INTERN(rd,rs,lo); \
        } else { \
            M_SETHI(hi&0x3ffff8,rd); \
            M_AADD(rs,rd,rd); \
            M_FLD_INTERN(rd,rd,lo); \
        } \
    } while (0)


#define M_FST_INTERN(rd,rs,disp) M_OP3(0x03,0x24,rd*2,rs,disp,IMM)    /* float (32-bit) store  */
#define M_FST(rd,rs,disp) \
    do { \
        s4 lo = (short) (disp); \
        s4 hi = (short) (((disp) - lo) >> 13); \
        if (hi == 0) { \
            M_FST_INTERN(rd,rs,lo); \
        } else { \
            M_SETHI(hi&0x3ffff8,REG_ITMP3); \
            M_AADD(rs,REG_ITMP3,REG_ITMP3); \
            M_FST_INTERN(rd,REG_ITMP3,lo); \
        } \
    } while (0)
    

#define M_DST_INTERN(rd,rs1,disp) M_OP3(0x03,0x27,rd,rs1,disp,IMM)    /* double (64-bit) store */
#define M_DST(rd,rs,disp) \
    do { \
        s4 lo = (short) (disp); \
        s4 hi = (short) (((disp) - lo) >> 13); \
        if (hi == 0) { \
            M_DST_INTERN(rd,rs,lo); \
        } else { \
            M_SETHI(hi&0x3ffff8,REG_ITMP3); \
            M_AADD(rs,REG_ITMP3,REG_ITMP3); \
            M_DST_INTERN(rd,REG_ITMP3,lo); \
        } \
    } while (0)
    
    
    
/*
 * Address pseudo instruction
 */

#define POINTERSHIFT 3 /* x8 */


#define M_ALD_INTERN(a,b,disp)  M_LDX_INTERN(a,b,disp)
#define M_ALD(a,b,disp)         M_LDX(a,b,disp)
#define M_AST_INTERN(a,b,disp)  M_STX_INTERN(a,b,disp)
#define M_AST(a,b,disp)         M_STX(a,b,disp)
#define M_AADD(a,b,c)           M_ADD(a,b,c)
#define M_AADD_IMM(a,b,c)       M_ADD_IMM(a,b,c)
#define M_ASUB_IMM(a,b,c)       M_SUB_IMM(a,b,c)
#define M_ASLL_IMM(a,b,c)       M_SLLX_IMM(a,b,c)




/* var_to_reg_xxx **************************************************************

   This function generates code to fetch data from a pseudo-register
   into a real register. If the pseudo-register has actually been
   assigned to a real register, no code will be emitted, since
   following operations can use this register directly.
    
   v: pseudoregister to be fetched from
   tempregnum: temporary register to be used if v is actually spilled to ram

   return: the register number, where the operand can be found after 
           fetching (this wil be either tempregnum or the register
           number allready given to v)

*******************************************************************************/

#define var_to_reg_int(regnr,v,tempnr) \
    do { \
        if ((v)->flags & INMEMORY) { \
            COUNT_SPILLS; \
            M_LDX(tempnr, REG_SP, (v)->regoff * 8); \
            regnr = tempnr; \
        } else { \
            regnr = (v)->regoff; \
        } \
    } while (0)
																  

/* gen_resolvebranch ***********************************************************
 *
 *    backpatches a branch instruction
 *    On Sparc all there is to do, is replace the 22bit disp at the end of the 
 *    instruction.
 *    THIS APPLIES TO THE (V8) BICC INSTRUCTION ONLY.
 *
 *    parameters: ip ... pointer to instruction after branch (void*)
 *                so ... offset of instruction after branch  (s4)
 *                to ... offset of branch target  (s4)
 *
 *******************************************************************************/

#define gen_resolvebranch(ip,so,to) \
	((s4 *) (ip))[-1] |= ((s4) (to) - (so)) >> 2 & 0x1fffff



																  
	
#endif /* _CODEGEN_H */
