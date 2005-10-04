/* src/vm/jit/i386/icm_uses_reg_res.inc - Table for reserved register usage

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

   Authors: Christian Ullrich

*/
#define NO   0 /* Register survives this ICMD */
#define S    1 /* Register can not be a source operand */
#define D    2 /* Register can not be a destination operand */
#define YES  4 /* Register does not survive this ICMD */

int lsra_reg_res[REG_RES_CNT]={EAX, ECX, EDX};

int icmd_uses_reg_res[256][REG_RES_CNT + 1] ={
/*EAX, ECX, EDX, OUTPUT */
{  NO,  NO,  NO, REG_NULL},    /* ICMD_NOP */
{ YES,  NO,  NO, REG_NULL},    /* ICMD_ACONST */	
{  NO,  NO,  NO, REG_NULL},    /* ICMD_NULLCHECKPOP */
{  NO,  NO,  NO, REG_NULL},    /* ICMD_ICONST */	
{  NO,  NO,  NO, REG_NULL}, /* ICMD_UNDEF4	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_IDIVPOW2	*/ 
{ YES, YES,  NO, REG_NULL},    /* ICMD_LDIVPOW2	*/
{  NO,  NO,  NO, REG_NULL}, /* ICMD_UNDEF7	*/
{  NO,  NO,  NO, REG_NULL}, /* ICMD_UNDEF8	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_LCONST */	
{ YES, YES, YES, REG_NULL}, /* ICMD_LCMPCONST	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_FCONST	*/	
{  NO,  NO,  NO, REG_NULL}, /* ICMD_UNDEF12	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_ELSE_ICONST	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_DCONST	*/	
{ YES, YES, YES, REG_NULL},    /* ICMD_IFEQ_ICONST	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IFNE_ICONST	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IFLT_ICONST	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IFGE_ICONST	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IFGT_ICONST	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IFLE_ICONST	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_ILOAD	*/ 
{  NO,  NO,  NO, REG_NULL},    /* ICMD_LLOAD	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_FLOAD	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_DLOAD	*/	
{ YES,  NO,  NO, REG_NULL},    /* ICMD_ALOAD	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_IADDCONST	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_ISUBCONST	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_IMULCONST	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_IANDCONST	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_IORCONST	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_IXORCONST	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_ISHLCONST	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_ISHRCONST	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_IUSHRCONST	*/
{ YES, YES,  NO, REG_NULL},    /* ICMD_IREMPOW2	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_LADDCONST	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_LSUBCONST	*/	
{ S|YES, S|YES, S|YES, REG_NULL},    /* ICMD_LMULCONST	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_LANDCONST	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_LORCONST	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_LXORCONST	*/
{ YES, YES,  NO, REG_NULL},    /* ICMD_LSHLCONST	*/
{ YES, YES,  NO, REG_NULL},    /* ICMD_LSHRCONST	*/
{ YES, YES,  NO, REG_NULL},    /* ICMD_LUSHRCONST	*/
{ YES, YES,  NO, REG_NULL},    /* ICMD_LREMPOW2	*/
{ S|YES, S|YES,  NO, REG_NULL},    /* ICMD_IALOAD	*/
{ S|YES, S|YES, S|YES, REG_NULL},  /* ICMD_LALOAD	*/
{ S|YES, S|YES,  NO, REG_NULL},    /* ICMD_FALOAD	*/
{ S|YES, S|YES,  NO, REG_NULL},    /* ICMD_DALOAD	*/
{ S|YES, S|YES,  NO, REG_NULL},    /* ICMD_AALOAD	*/
{ S|YES, S|YES,  NO, REG_NULL},    /* ICMD_BALOAD	*/
{ S|YES, S|YES,  NO, REG_NULL},    /* ICMD_CALOAD	*/
{ S|YES, S|YES,  NO, REG_NULL},    /* ICMD_SALOAD	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_ISTORE	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_LSTORE	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_FSTORE	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_DSTORE	*/	
{ YES,  NO,  NO, REG_NULL},    /* ICMD_ASTORE	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IF_LEQ	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IF_LNE	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IF_LLT	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IF_LGE	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IF_LGT	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IF_LLE	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IF_LCMPEQ	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IF_LCMPNE	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IF_LCMPLT	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IF_LCMPGE	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IF_LCMPGT	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IF_LCMPLE	*/
{  NO,  NO,  NO, REG_NULL}, /* ICMD_UNDEF71	*/
{  NO,  NO,  NO, REG_NULL}, /* ICMD_UNDEF72	*/
{  NO,  NO,  NO, REG_NULL}, /* ICMD_UNDEF73	*/
{  NO,  NO,  NO, REG_NULL}, /* ICMD_UNDEF74	*/
{  NO,  NO,  NO, REG_NULL}, /* ICMD_UNDEF75	*/
{  NO,  NO,  NO, REG_NULL}, /* ICMD_UNDEF76	*/
{  NO,  NO,  NO, REG_NULL}, /* ICMD_UNDEF77	*/
{  NO,  NO,  NO, REG_NULL}, /* ICMD_UNDEF78	*/
{ S|YES, S|YES, S|YES, REG_NULL},    /* ICMD_IASTORE	*/
{ S|YES, S|YES, S|YES, REG_NULL},    /* ICMD_LASTORE	*/
{ S|YES, S|YES,  NO, REG_NULL},    /* ICMD_FASTORE	*/
{ S|YES, S|YES,  NO, REG_NULL},    /* ICMD_DASTORE	*/
{ S|YES, S|YES, S|YES, REG_NULL},    /* ICMD_AASTORE	*/
{ S|YES, S|YES, S|YES, REG_NULL},    /* ICMD_BASTORE	*/
{ S|YES, S|YES, S|YES, REG_NULL},    /* ICMD_CASTORE	*/
{ S|YES, S|YES, S|YES, REG_NULL},    /* ICMD_SASTORE	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_POP	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_POP2	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_DUP	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_DUP_X1	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_DUP_X2	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_DUP2	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_DUP2_X1	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_DUP2_X2	*/	
{ YES,  NO,  NO, REG_NULL},    /* ICMD_SWAP	*/
{ S|YES,  NO,  NO, REG_NULL},    /* ICMD_IADD	*/
{ S|YES,  NO,  NO, REG_NULL},    /* ICMD_LADD	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_FADD	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_DADD	*/
{ S|YES,  NO,  NO, REG_NULL},    /* ICMD_ISUB	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_LSUB	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_FSUB	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_DSUB	*/
{ S|YES,  NO,  NO, EAX},    /* ICMD_IMUL	*/
{ S|YES, S|YES, S|YES, REG_NULL},    /* ICMD_LMUL	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_FMUL	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_DMUL	*/
{ S|YES, S|YES, S|YES, EAX},    /* ICMD_IDIV	*/ /* Really uses EDX? */
{ YES, YES, YES, REG_NULL}, /* ICMD_LDIV	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_FDIV	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_DDIV	*/
{ S|YES, S|YES, S|YES, EDX},    /* ICMD_IREM	*/     /* last checked */
{ YES, YES, YES, REG_NULL}, /* ICMD_LREM	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_FREM	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_DREM	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_INEG	*/	
{ YES,  NO,  NO, REG_NULL},    /* ICMD_LNEG	*/	
{  NO,  NO,  NO, REG_NULL},    /* ICMD_FNEG	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_DNEG	*/
{ YES, D|S|YES,  NO, REG_NULL},    /* ICMD_ISHL	*/
{ S|YES, YES, S|YES, REG_NULL},    /* ICMD_LSHL	*/
{ YES, D|S|YES,  NO, REG_NULL},    /* ICMD_ISHR	*/
{ S|YES, YES, S|YES, REG_NULL},    /* ICMD_LSHR	*/
{ YES, D|S|YES,  NO, REG_NULL},    /* ICMD_IUSHR	*/
{ S|YES, YES, S|YES, REG_NULL},    /* ICMD_LUSHR	*/
{ S|YES,  NO,  NO, REG_NULL},    /* ICMD_IAND	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_LAND	*/
{ S|YES,  NO,  NO, REG_NULL},    /* ICMD_IOR	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_LOR	*/
{ S|YES,  NO,  NO, REG_NULL},    /* ICMD_IXOR	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_LXOR	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_IINC	*/
{ YES,  NO, YES, REG_NULL},    /* ICMD_I2L	*/
{ S|YES,  NO,  NO, REG_NULL},    /* ICMD_I2F	*/
{ S|YES,  NO,  NO, REG_NULL},    /* ICMD_I2D	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_L2I	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_L2F	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_L2D	*/
{  D|YES, NO,  NO, EAX},         /* ICMD_F2I	*/
{  YES, NO, YES, REG_NULL},    /* ICMD_F2L	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_F2D	*/
{  D|YES, NO,  NO, EAX},         /* ICMD_D2I	*/
{  YES, NO, YES, REG_NULL},    /* ICMD_D2L	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_D2F	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_INT2BYTE	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_INT2CHAR	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_INT2SHORT	*/
{ YES, YES, YES, REG_NULL}, /* ICMD_LCMP	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_FCMPL	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_FCMPG	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_DCMPL	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_DCMPG	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IFEQ	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IFNE	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IFLT	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IFGE	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IFGT	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IFLE	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IF_ICMPEQ	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IF_ICMPNE	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IF_ICMPLT	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IF_ICMPGE	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IF_ICMPGT	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IF_ICMPLE	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IF_ACMPEQ	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IF_ACMPNE	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_GOTO	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_JSR	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_RET	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_TABLESWITCH	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_LOOKUPSWITCH	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IRETURN	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_LRETURN	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_FRETURN	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_DRETURN	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_ARETURN	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_RETURN	*/
{ S|YES, S|YES, S|YES, EAX},    /* ICMD_GETSTATIC*/   
{ S|YES, S|YES, S|YES, REG_NULL},    /* ICMD_PUTSTATIC*/   
{ S|YES, S|YES, S|YES, EAX},    /* ICMD_GETFIELD	*/    
{ S|YES, S|YES,  S|YES, REG_NULL},    /* ICMD_PUTFIELD	*/   
{ S|YES, YES, YES, EAX},        /* ICMD_INVOKEVIRTUAL	*/    
{ S|YES, YES, YES, EAX},        /* ICMD_INVOKESPECIAL	*/  
{ S|YES, YES, YES, EAX},        /* ICMD_INVOKESTATIC	*/     
{ S|YES, YES, YES, EAX},        /* ICMD_INVOKEINTERFACE*/
{  NO,  NO,  NO, REG_NULL},    /* UNDEF186 */
{ YES, YES, YES, REG_NULL}, /* ICMD_NEW	*/
{ YES, YES, YES, REG_NULL}, /* ICMD_NEWARRAY	*/
{ YES, YES, YES, REG_NULL}, /* ICMD_ANEWARRAY	*/
{ YES,  NO,  NO, REG_NULL},    /* ICMD_ARRAYLENGTH	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_ATHROW 	*/
{ YES, S|YES, S|YES, REG_NULL},  /* ICMD_CHECKCAST	*/
/* { S|D|YES, S|D|YES, S|D|YES, REG_NULL}, */    /* ICMD_CHECKCAST	*/
{ S|D|YES, S|D|YES, S|D|YES, REG_NULL},    /* ICMD_INSTANCEOF	*/
/*{ S|D|YES, S|D|YES, S|D|YES, REG_NULL}, */    /* ICMD_INSTANCEOF	*/
{ YES, YES, YES, REG_NULL}, /* ICMD_MONITORENTER	*/
{ YES, YES, YES, REG_NULL}, /* ICMD_MONITOREXIT	*/
{  NO,  NO,  NO, REG_NULL}, /* ICMD_UNDEF196	*/
{ S|YES, YES, YES, EAX},       /* ICMD_MULTIANEWARRAY	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IFNULL	*/
{ YES, YES, YES, REG_NULL},    /* ICMD_IFNONNULL	*/
{ YES, YES, YES, REG_NULL}, /* ICMD_UNDEF200	*/
{ YES, YES, YES, REG_NULL}, /* ICMD_UNDEF201	*/
{ YES, YES, YES, REG_NULL}, /* ICMD_UNDEF202	*/
{ NO,  NO,  NO, REG_NULL},    /* UNDEF203 */
{ S|YES, S|YES,  NO, REG_NULL},    /* ICMD_IASTORECONST	*/
{ S|YES, S|YES,  NO, REG_NULL},    /* ICMD_LASTORECONST	*/
{ S|YES, S|YES, YES, REG_NULL}, /* ICMD_FASTORECONST	*/
{ S|YES, S|YES, YES, REG_NULL}, /* ICMD_DASTORECONST	*/
{ S|YES, S|YES,  NO, REG_NULL},    /* ICMD_AASTORECONST	*/
{ S|YES, S|YES,  NO, REG_NULL},    /* ICMD_BASTORECONST	*/
{ S|YES, S|YES,  NO, REG_NULL},    /* ICMD_CASTORECONST	*/
{ S|YES, S|YES,  NO, REG_NULL},    /* ICMD_SASTORECONST	*/
{ S|YES, S|YES, S|YES, REG_NULL},    /* ICMD_PUTSTATICCONST */
{ S|YES,  S|YES,  S|YES, REG_NULL},    /* ICMD_PUTFIELDCONST */
{ YES, YES, YES, REG_NULL}, /* ICMD_IMULPOW2 */
{ YES, YES, YES, REG_NULL}, /* ICMD_LMULPOW2 */
{  NO,  NO,  NO, REG_NULL}, /* 	"UNDEF216"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF217"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF218"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF219"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF220",	*/
{  NO,  NO,  NO, REG_NULL}, /* 	"UNDEF"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF",	*/
{  NO,  NO,  NO, REG_NULL}, /* 	"UNDEF226"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF227"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF228"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF229"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF230",	*/
{  NO,  NO,  NO, REG_NULL}, /* 	"UNDEF"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF",	*/
{  NO,  NO,  NO, REG_NULL}, /* 	"UNDEF236"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF237"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF238"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF239"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF240",	*/
{  NO,  NO,  NO, REG_NULL}, /* 	"UNDEF"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF",	*/
{  NO,  NO,  NO, REG_NULL}, /* 	"UNDEF246"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF247"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF248"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF249"	*/
{  NO,  NO,  NO, REG_NULL}, /* 	,"UNDEF250",	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_INLINE_START	*/
{  NO,  NO,  NO, REG_NULL},    /* ICMD_INLINE_END	*/
{ S|YES, YES, YES, EAX},       /* ICMD_BUILTIN3	*/  
{ S|YES, YES, YES, EAX},       /* ICMD_BUILTIN2	*/   
{ S|YES, YES, YES, EAX},       /* ICMD_BUILTIN1	*/   
};


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
