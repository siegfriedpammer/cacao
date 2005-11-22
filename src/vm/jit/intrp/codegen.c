/* src/vm/jit/intrp/codegen.c - code generator for Interpreter

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
            Reinhard Grafl

   Changes: Christian Thalinger
            Anton Ertl

   $Id: codegen.c 3731 2005-11-22 11:39:17Z twisti $

*/


#include <stdio.h>

#include "config.h"
#include "vm/types.h"

#include "arch.h"

#include "vm/jit/intrp/codegen.h"

#include "cacao/cacao.h"
#include "native/native.h"
#include "vm/builtin.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/stringlocal.h"
#include "vm/tables.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/codegen.inc"
#include "vm/jit/jit.h"

#include "vm/jit/parse.h"
#include "vm/jit/patcher.h"

#include "vm/jit/intrp/intrp.h"

#include "libffi/include/ffi.h"


#define gen_branch(_inst) { \
  gen_##_inst(((Inst **)cd), 0); \
  codegen_addreference(cd, (basicblock *) (iptr->target), cd->mcodeptr); \
}

#define index2offset(_i) (-(_i) * SIZEOF_VOID_P)

/* functions used by cacao-gen.i */

/* vmgen-0.6.2 generates gen_... calls with Inst ** as first
   parameter, but we need to pass in cd to make last_compiled
   thread-safe */

void genarg_v(Inst **cd1, Cell v)
{
	Inst **mcodepp = (Inst **) &(((codegendata *) cd1)->mcodeptr);
	*((Cell *) *mcodepp) = v;
	(*mcodepp)++;
}

void genarg_i(Inst **cd1, s4 i)
{
	Inst **mcodepp = (Inst **) &(((codegendata *) cd1)->mcodeptr);
	*((Cell *) *mcodepp) = i;
	(*mcodepp)++;
}

void genarg_b(Inst ** cd1, s4 i)
{
  genarg_i(cd1, i);
}

void genarg_f(Inst ** cd1, float f)
{
	s4 fi;

	vm_f2Cell(f,fi);
	genarg_i(cd1, fi);
}

void genarg_l(Inst ** cd1, s8 l)
{
	Inst **mcodepp = (Inst **) &(((codegendata *) cd1)->mcodeptr);
	vm_l2twoCell(l, ((Cell *) *mcodepp)[1], ((Cell *) *mcodepp)[0]);
	(*mcodepp) +=2;
}

void genarg_aRef(Inst ** cd1, java_objectheader *a)
{
	Inst **mcodepp = (Inst **) &(((codegendata *) cd1)->mcodeptr);
	*((java_objectheader **) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_aArray(Inst ** cd1, java_arrayheader *a)
{
	Inst **mcodepp = (Inst **) &(((codegendata *) cd1)->mcodeptr);
	*((java_arrayheader **) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_aaTarget(Inst ** cd1, Inst **a)
{
	Inst **mcodepp = (Inst **) &(((codegendata *)cd1)->mcodeptr);
	*((Inst ***) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_aClass(Inst ** cd1, classinfo *a)
{
	Inst **mcodepp = (Inst **) &(((codegendata *)cd1)->mcodeptr);
	*((classinfo **) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_acr(Inst ** cd1, constant_classref *a)
{
	Inst **mcodepp = (Inst **) &(((codegendata *)cd1)->mcodeptr);
	*((constant_classref **) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_addr(Inst ** cd1, u1 *a)
{
	Inst **mcodepp = (Inst **) &(((codegendata *)cd1)->mcodeptr);
	*((u1 **) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_af(Inst ** cd1, functionptr a)
{
	Inst **mcodepp = (Inst **) &(((codegendata *)cd1)->mcodeptr);
	*((functionptr *) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_am(Inst ** cd1, methodinfo *a)
{
	Inst **mcodepp = (Inst **) &(((codegendata *)cd1)->mcodeptr);
	*((methodinfo **) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_acell(Inst ** cd1, Cell *a)
{
	Inst **mcodepp = (Inst **) &(((codegendata *)cd1)->mcodeptr);
	*((Cell **) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_ainst(Inst ** cd1, Inst *a)
{
	Inst **mcodepp = (Inst **) &(((codegendata *)cd1)->mcodeptr);
	*((Inst **) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_auf(Inst ** cd1, unresolved_field *a)
{
	Inst **mcodepp = (Inst **) &(((codegendata *)cd1)->mcodeptr);
	*((unresolved_field **) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_aum(Inst ** cd1, unresolved_method *a)
{
	Inst **mcodepp = (Inst **) &(((codegendata *)cd1)->mcodeptr);
	*((unresolved_method **) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_avftbl(Inst ** cd1, vftbl_t *a)
{
	Inst **mcodepp = (Inst **) &(((codegendata *)cd1)->mcodeptr);
	*((vftbl_t **) *mcodepp) = a;
	(*mcodepp)++;
}


/* include the interpreter generation functions *******************************/

#include "vm/jit/intrp/java-gen.i"


typedef void (*genfunctionptr) (Inst **);

typedef struct builtin_gen builtin_gen;

struct builtin_gen {
	functionptr builtin;
	genfunctionptr gen;
};

struct builtin_gen builtin_gen_table[] = {
    {BUILTIN_new,                     gen_NEW,             }, 
    {BUILTIN_newarray,                gen_NEWARRAY,        },
    {BUILTIN_newarray_boolean,        gen_NEWARRAY_BOOLEAN,},
    {BUILTIN_newarray_byte,           gen_NEWARRAY_BYTE,   },
    {BUILTIN_newarray_char,           gen_NEWARRAY_CHAR,   },
    {BUILTIN_newarray_short,          gen_NEWARRAY_SHORT,  },    
    {BUILTIN_newarray_int,            gen_NEWARRAY_INT,    },
    {BUILTIN_newarray_long,           gen_NEWARRAY_LONG,   },
    {BUILTIN_newarray_float,          gen_NEWARRAY_FLOAT,  },    
    {BUILTIN_newarray_double,         gen_NEWARRAY_DOUBLE, },
    {BUILTIN_arrayinstanceof,         gen_ARRAYINSTANCEOF, },
#if defined(USE_THREADS)
    {BUILTIN_monitorenter,            gen_MONITORENTER,    },
    {BUILTIN_monitorexit,             gen_MONITOREXIT,     },
#endif
    {BUILTIN_f2l,                     gen_F2L,             },
    {BUILTIN_d2l,					  gen_D2L, 			   },
    {BUILTIN_f2i,					  gen_F2I, 			   },
    {BUILTIN_d2i,					  gen_D2I, 			   },
    {BUILTIN_idiv,					  gen_IDIV,			   },
    {BUILTIN_irem,					  gen_IREM,			   },
    {BUILTIN_ldiv,					  gen_LDIV,			   },
    {BUILTIN_lrem,					  gen_LREM,			   },
    {BUILTIN_frem,					  gen_FREM,			   },
    {BUILTIN_drem,					  gen_DREM,            },
};


/* codegen *********************************************************************

   Generates machine code.

*******************************************************************************/

bool codegen(methodinfo *m, codegendata *cd, registerdata *rd)
{
	s4                  i, len, s1, s2, d;
	stackptr            src;
	basicblock         *bptr;
	instruction        *iptr;
	exceptiontable     *ex;
	u2                  currentline;
	methodinfo         *lm;             /* local methodinfo for ICMD_INVOKE*  */
	unresolved_method  *um;
	builtintable_entry *bte;
	methoddesc         *md;

	/* prevent compiler warnings */

	d = 0;
	currentline = 0;
	lm = NULL;
	bte = NULL;

	/* create method header */

	(void) dseg_addaddress(cd, m);                          /* MethodPointer  */
	(void) dseg_adds4(cd, m->maxlocals * SIZEOF_VOID_P);    /* FrameSize      */

#if defined(USE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		(void) dseg_adds4(cd, 1);                           /* IsSync         */
	else
#endif
		(void) dseg_adds4(cd, 0);                           /* IsSync         */
	                                       
	(void) dseg_adds4(cd, 0);                               /* IsLeaf         */
	(void) dseg_adds4(cd, 0);                               /* IntSave        */
	(void) dseg_adds4(cd, 0);                               /* FltSave        */

	dseg_addlinenumbertablesize(cd);

	(void) dseg_adds4(cd, cd->exceptiontablelength);        /* ExTableSize    */

	/* create exception table */

	for (ex = cd->exceptiontable; ex != NULL; ex = ex->down) {
		dseg_addtarget(cd, ex->start);
   		dseg_addtarget(cd, ex->end);
		dseg_addtarget(cd, ex->handler);
		(void) dseg_addaddress(cd, ex->catchtype.cls);
	}
	
	/* initialize mcode variables */
	
	cd->mcodeptr = cd->mcodebase;
	cd->mcodeend = (s4 *) (cd->mcodebase + cd->mcodesize);

	gen_BBSTART;

#if defined(USE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		if (m->flags & ACC_STATIC)
			gen_ACONST(((Inst **)cd), (java_objectheader *) m->class);
		else
			gen_ALOAD(((Inst **)cd), 0);
		
		gen_MONITORENTER(((Inst **)cd));
	}			
#endif

	if (runverbose)
		gen_TRACECALL(((Inst **)cd));

	/* walk through all basic blocks */

	for (bptr = m->basicblocks; bptr != NULL; bptr = bptr->next) {

		bptr->mpc = (s4) (cd->mcodeptr - cd->mcodebase);

		if (bptr->flags >= BBREACHED) {

		/* walk through all instructions */
		
		src = bptr->instack;
		len = bptr->icount;

		gen_BBSTART;

		for (iptr = bptr->iinstr; len > 0; src = iptr->dst, len--, iptr++) {
			if (iptr->line != currentline) {
				dseg_addlinenumber(cd, iptr->line, cd->mcodeptr);
				currentline = iptr->line;
			}

		MCODECHECK(64);       /* an instruction usually needs < 64 words      */
		switch (iptr->opc) {

		case ICMD_INLINE_START:
		case ICMD_INLINE_END:
			break;

		case ICMD_NOP:        /* ...  ==> ...                                 */
			break;

		case ICMD_CHECKNULL:  /* ..., objectref  ==> ..., objectref           */

			gen_CHECKNULL(((Inst **)cd));
			break;

		/* constant operations ************************************************/

		case ICMD_ICONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.i = constant                    */

			gen_ICONST(((Inst **)cd), iptr->val.i);
			break;

		case ICMD_LCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.l = constant                    */

			gen_LCONST(((Inst **)cd), iptr->val.l);
			break;

		case ICMD_FCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.f = constant                    */
			{
				s4 fi;

				vm_f2Cell(iptr->val.f, fi);
				gen_ICONST(((Inst **)cd), fi);
			}
			break;
			
		case ICMD_DCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.d = constant                    */

			gen_LCONST(((Inst **)cd), *(s8 *)&(iptr->val.d));
			break;

		case ICMD_ACONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.a = constant                    */

			if ((iptr->target != NULL) && (iptr->val.a == NULL))
				gen_PATCHER_ACONST(((Inst **) cd), NULL, iptr->target);
			else
				gen_ACONST(((Inst **) cd), iptr->val.a);
			break;


		/* load/store operations **********************************************/

		case ICMD_ILOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			gen_ILOAD(((Inst **)cd), index2offset(iptr->op1));
			break;

		case ICMD_LLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			gen_LLOAD(((Inst **)cd), index2offset(iptr->op1));
			break;

		case ICMD_ALOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			gen_ALOAD(((Inst **)cd), index2offset(iptr->op1));
			break;

		case ICMD_FLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			gen_ILOAD(((Inst **)cd), index2offset(iptr->op1));
			break;

		case ICMD_DLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			gen_LLOAD(((Inst **)cd), index2offset(iptr->op1));
			break;


		case ICMD_ISTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			gen_ISTORE(((Inst **)cd), index2offset(iptr->op1));
			break;

		case ICMD_LSTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			gen_LSTORE(((Inst **)cd), index2offset(iptr->op1));
			break;

		case ICMD_ASTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			gen_ASTORE(((Inst **)cd), index2offset(iptr->op1));
			break;


		case ICMD_FSTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			gen_ISTORE(((Inst **)cd), index2offset(iptr->op1));
			break;

		case ICMD_DSTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			gen_LSTORE(((Inst **)cd), index2offset(iptr->op1));
			break;


		/* pop/dup/swap operations ********************************************/

		/* attention: double and longs are only one entry in CACAO ICMDs      */

		/* stack.c changes stack manipulation operations to treat
		   longs/doubles as occupying a single slot.  Here we are
		   undoing that (and only those things that stack.c did). */

		case ICMD_POP:        /* ..., value  ==> ...                          */

			if (IS_2_WORD_TYPE(src->type))
				gen_POP2(((Inst **)cd));
			else
				gen_POP(((Inst **)cd));
			break;

		case ICMD_POP2:       /* ..., value, value  ==> ...                   */

			gen_POP2(((Inst **)cd));
			break;

		case ICMD_DUP:        /* ..., a ==> ..., a, a                         */

			if (IS_2_WORD_TYPE(src->type))
				gen_DUP2(((Inst **)cd));
			else
				gen_DUP(((Inst **)cd));
			break;

		case ICMD_DUP_X1:     /* ..., a, b ==> ..., b, a, b                   */

			if (IS_2_WORD_TYPE(src->type)) {
				if (IS_2_WORD_TYPE(src->prev->type)) {
					gen_DUP2_X2(((Inst **)cd));
				} else {
					gen_DUP2_X1(((Inst **)cd));
				}
			} else {
				if (IS_2_WORD_TYPE(src->prev->type)) {
					gen_DUP_X2(((Inst **)cd));
				} else {
					gen_DUP_X1(((Inst **)cd));
				}
			}
			break;

		case ICMD_DUP_X2:     /* ..., a, b, c ==> ..., c, a, b, c             */

			if (IS_2_WORD_TYPE(src->type)) {
				gen_DUP2_X2(((Inst **)cd));
			} else
				gen_DUP_X2(((Inst **)cd));
			break;

		case ICMD_DUP2:       /* ..., a, b ==> ..., a, b, a, b                */

			gen_DUP2(((Inst **)cd));
			break;

		case ICMD_DUP2_X1:    /* ..., a, b, c ==> ..., b, c, a, b, c          */

			if (IS_2_WORD_TYPE(src->prev->prev->type))
				gen_DUP2_X2(((Inst **)cd));
			else
				gen_DUP2_X1(((Inst **)cd));
			break;

		case ICMD_DUP2_X2:    /* ..., a, b, c, d ==> ..., c, d, a, b, c, d    */

			gen_DUP2_X2(((Inst **)cd));
			break;

		case ICMD_SWAP:       /* ..., a, b ==> ..., b, a                      */

			gen_SWAP(((Inst **)cd));
			break;


		/* integer operations *************************************************/

		case ICMD_INEG:       /* ..., value  ==> ..., - value                 */

			gen_INEG(((Inst **)cd));
			break;

		case ICMD_LNEG:       /* ..., value  ==> ..., - value                 */

			gen_LNEG(((Inst **)cd));
			break;

		case ICMD_I2L:        /* ..., value  ==> ..., value                   */

			gen_I2L(((Inst **)cd));
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */

			gen_L2I(((Inst **)cd));
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			gen_INT2BYTE(((Inst **)cd));
			break;

		case ICMD_INT2CHAR:   /* ..., value  ==> ..., value                   */

			gen_INT2CHAR(((Inst **)cd));
			break;

		case ICMD_INT2SHORT:  /* ..., value  ==> ..., value                   */

			gen_INT2SHORT(((Inst **)cd));
			break;


		case ICMD_IADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			gen_IADD(((Inst **)cd));
			break;

		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			gen_ICONST(((Inst **)cd), iptr->val.i);
			gen_IADD(((Inst **)cd));
			break;

		case ICMD_LADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			gen_LADD(((Inst **)cd));
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.l = constant                             */

			gen_LCONST(((Inst **)cd), iptr->val.l);
			gen_LADD(((Inst **)cd));
			break;

		case ICMD_ISUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			gen_ISUB(((Inst **)cd));
			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			gen_ICONST(((Inst **)cd), iptr->val.i);
			gen_ISUB(((Inst **)cd));
			break;

		case ICMD_LSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			gen_LSUB(((Inst **)cd));
			break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* val.l = constant                             */

			gen_LCONST(((Inst **)cd), iptr->val.l);
			gen_LSUB(((Inst **)cd));
			break;

		case ICMD_IMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			gen_IMUL(((Inst **)cd));
			break;

		case ICMD_IMULCONST:  /* ..., val1, val2  ==> ..., val1 * val2        */

			gen_ICONST(((Inst **)cd), iptr->val.i);
			gen_IMUL(((Inst **)cd));
			break;

		case ICMD_LMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			gen_LMUL(((Inst **)cd));
			break;

		case ICMD_LMULCONST:  /* ..., val1, val2  ==> ..., val1 * val2        */

			gen_LCONST(((Inst **)cd), iptr->val.l);
			gen_LMUL(((Inst **)cd));
			break;

		case ICMD_IDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			gen_IDIV(((Inst **)cd));
			break;

		case ICMD_IREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			gen_IREM(((Inst **)cd));
			break;

		case ICMD_LDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			gen_LDIV(((Inst **)cd));
			break;

		case ICMD_LREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			gen_LREM(((Inst **)cd));
			break;

		case ICMD_IDIVPOW2:   /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */
		                      
			gen_IDIVPOW2(((Inst **)cd), iptr->val.i);
			break;

		case ICMD_IREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.i = constant                             */

			gen_IREMPOW2(((Inst **)cd), iptr->val.i);
			break;

		case ICMD_LDIVPOW2:   /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */
		                      
			gen_LDIVPOW2(((Inst **)cd), iptr->val.i);
			break;

		case ICMD_LREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.l = constant                             */

			gen_LREMPOW2(((Inst **)cd), iptr->val.i);
			break;

		case ICMD_ISHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			gen_ISHL(((Inst **)cd));
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			gen_ICONST(((Inst **)cd), iptr->val.i);
			gen_ISHL(((Inst **)cd));
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			gen_ISHR(((Inst **)cd));
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			gen_ICONST(((Inst **)cd), iptr->val.i);
			gen_ISHR(((Inst **)cd));
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			gen_IUSHR(((Inst **)cd));
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			gen_ICONST(((Inst **)cd), iptr->val.i);
			gen_IUSHR(((Inst **)cd));
			break;

		case ICMD_LSHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			gen_LSHL(((Inst **)cd));
			break;

		case ICMD_LSHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			gen_ICONST(((Inst **)cd), iptr->val.i);
			gen_LSHL(((Inst **)cd));
			break;

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			gen_LSHR(((Inst **)cd));
			break;

		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			gen_ICONST(((Inst **)cd), iptr->val.i);
			gen_LSHR(((Inst **)cd));
			break;

		case ICMD_LUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			gen_LUSHR(((Inst **)cd));
			break;

		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			gen_ICONST(((Inst **)cd), iptr->val.i);
			gen_LUSHR(((Inst **)cd));
			break;

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			gen_IAND(((Inst **)cd));
			break;

		case ICMD_IANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.i = constant                             */

			gen_ICONST(((Inst **)cd), iptr->val.i);
			gen_IAND(((Inst **)cd));
			break;

		case ICMD_LAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			gen_LAND(((Inst **)cd));
			break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.l = constant                             */

			gen_LCONST(((Inst **)cd), iptr->val.l);
			gen_LAND(((Inst **)cd));
			break;

		case ICMD_IOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			gen_IOR(((Inst **)cd));
			break;

		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.i = constant                             */

			gen_ICONST(((Inst **)cd), iptr->val.i);
			gen_IOR(((Inst **)cd));
			break;

		case ICMD_LOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			gen_LOR(((Inst **)cd));
			break;

		case ICMD_LORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.l = constant                             */

			gen_LCONST(((Inst **)cd), iptr->val.l);
			gen_LOR(((Inst **)cd));
			break;

		case ICMD_IXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			gen_IXOR(((Inst **)cd));
			break;

		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.i = constant                             */

			gen_ICONST(((Inst **)cd), iptr->val.i);
			gen_IXOR(((Inst **)cd));
			break;

		case ICMD_LXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			gen_LXOR(((Inst **)cd));
			break;

		case ICMD_LXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.l = constant                             */

			gen_LCONST(((Inst **)cd), iptr->val.l);
			gen_LXOR(((Inst **)cd));
			break;


		case ICMD_LCMP:       /* ..., val1, val2  ==> ..., val1 cmp val2      */

			gen_LCMP(((Inst **)cd));
			break;


		case ICMD_IINC:       /* ..., value  ==> ..., value + constant        */
		                      /* op1 = variable, val.i = constant             */

			gen_IINC(((Inst **)cd), index2offset(iptr->op1), iptr->val.i);
			break;


		/* floating operations ************************************************/

		case ICMD_FNEG:       /* ..., value  ==> ..., - value                 */

			gen_FNEG(((Inst **)cd));
			break;

		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */

			gen_DNEG(((Inst **)cd));
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			gen_FADD(((Inst **)cd));
			break;

		case ICMD_DADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			gen_DADD(((Inst **)cd));
			break;

		case ICMD_FSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			gen_FSUB(((Inst **)cd));
			break;

		case ICMD_DSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			gen_DSUB(((Inst **)cd));
			break;

		case ICMD_FMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			gen_FMUL(((Inst **)cd));
			break;

		case ICMD_DMUL:       /* ..., val1, val2  ==> ..., val1 *** val2      */

			gen_DMUL(((Inst **)cd));
			break;

		case ICMD_FDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			gen_FDIV(((Inst **)cd));
			break;

		case ICMD_DDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			gen_DDIV(((Inst **)cd));
			break;
		
		case ICMD_FREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			gen_FREM(((Inst **)cd));
			break;

		case ICMD_DREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			gen_DREM(((Inst **)cd));
			break;
		
		case ICMD_I2F:       /* ..., value  ==> ..., (float) value            */

			gen_I2F(((Inst **)cd));
			break;

		case ICMD_L2F:       /* ..., value  ==> ..., (float) value            */

			gen_L2F(((Inst **)cd));
			break;

		case ICMD_I2D:       /* ..., value  ==> ..., (double) value           */

			gen_I2D(((Inst **)cd));
			break;
			
		case ICMD_L2D:       /* ..., value  ==> ..., (double) value           */

			gen_L2D(((Inst **)cd));
			break;
			
		case ICMD_F2I:       /* ..., value  ==> ..., (int) value              */

			gen_F2I(((Inst **)cd));
			break;
		
		case ICMD_D2I:       /* ..., value  ==> ..., (int) value              */

			gen_D2I(((Inst **)cd));
			break;
		
		case ICMD_F2L:       /* ..., value  ==> ..., (long) value             */

			gen_F2L(((Inst **)cd));
			break;

		case ICMD_D2L:       /* ..., value  ==> ..., (long) value             */

			gen_D2L(((Inst **)cd));
			break;

		case ICMD_F2D:       /* ..., value  ==> ..., (double) value           */

			gen_F2D(((Inst **)cd));
			break;
					
		case ICMD_D2F:       /* ..., value  ==> ..., (float) value            */

			gen_D2F(((Inst **)cd));
			break;
		
		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */

			gen_FCMPL(((Inst **)cd));
			break;
			
		case ICMD_DCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */

			gen_DCMPL(((Inst **)cd));
			break;
			
		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */

			gen_FCMPG(((Inst **)cd));
			break;

		case ICMD_DCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */

			gen_DCMPG(((Inst **)cd));
			break;


		/* memory operations **************************************************/

		case ICMD_ARRAYLENGTH: /* ..., arrayref  ==> ..., length              */

			gen_ARRAYLENGTH(((Inst **)cd));
			break;

		case ICMD_BALOAD:     /* ..., arrayref, index  ==> ..., value         */

			gen_BALOAD(((Inst **)cd));
			break;

		case ICMD_CALOAD:     /* ..., arrayref, index  ==> ..., value         */

			gen_CALOAD(((Inst **)cd));
			break;			

		case ICMD_SALOAD:     /* ..., arrayref, index  ==> ..., value         */

			gen_SALOAD(((Inst **)cd));
			break;

		case ICMD_IALOAD:     /* ..., arrayref, index  ==> ..., value         */
		case ICMD_FALOAD:     /* ..., arrayref, index  ==> ..., value         */

			gen_IALOAD(((Inst **)cd));
			break;

		case ICMD_LALOAD:     /* ..., arrayref, index  ==> ..., value         */
		case ICMD_DALOAD:     /* ..., arrayref, index  ==> ..., value         */

			gen_LALOAD(((Inst **)cd));
			break;

		case ICMD_AALOAD:     /* ..., arrayref, index  ==> ..., value         */

			gen_AALOAD(((Inst **)cd));
			break;


		case ICMD_BASTORE:    /* ..., arrayref, index, value  ==> ...         */

			gen_BASTORE(((Inst **)cd));
			break;

		case ICMD_CASTORE:    /* ..., arrayref, index, value  ==> ...         */
		case ICMD_SASTORE:    /* ..., arrayref, index, value  ==> ...         */

			gen_CASTORE(((Inst **)cd));
			break;

		case ICMD_IASTORE:    /* ..., arrayref, index, value  ==> ...         */
		case ICMD_FASTORE:    /* ..., arrayref, index, value  ==> ...         */

			gen_IASTORE(((Inst **)cd));
			break;

		case ICMD_LASTORE:    /* ..., arrayref, index, value  ==> ...         */
		case ICMD_DASTORE:    /* ..., arrayref, index, value  ==> ...         */

			gen_LASTORE(((Inst **)cd));
			break;

		case ICMD_AASTORE:    /* ..., arrayref, index, value  ==> ...         */

			gen_AASTORE(((Inst **)cd));
			break;


		case ICMD_GETSTATIC:  /* ...  ==> ..., value                          */
		                      /* op1 = type, val.a = field address            */

			{
			fieldinfo *fi = iptr->val.a;
			unresolved_field *uf = iptr->target;

			switch (iptr->op1) {
			case TYPE_INT:
			case TYPE_FLT:
				if (fi == NULL || !fi->class->initialized) {
					gen_PATCHER_GETSTATIC_INT(((Inst **)cd), 0, uf);
				} else {
					gen_GETSTATIC_INT(((Inst **)cd), (u1 *)&(fi->value.i), uf);
				}
				break;
			case TYPE_LNG:
			case TYPE_DBL:
				if (fi == NULL || !fi->class->initialized) {
					gen_PATCHER_GETSTATIC_LONG(((Inst **)cd), 0, uf);
				} else {
					gen_GETSTATIC_LONG(((Inst **)cd), (u1 *)&(fi->value.l), uf);
				}
				break;
			case TYPE_ADR:
				if (fi == NULL || !fi->class->initialized) {
					gen_PATCHER_GETSTATIC_CELL(((Inst **)cd), 0, uf);
				} else {
					gen_GETSTATIC_CELL(((Inst **)cd), (u1 *)&(fi->value.a), uf);
				}
				break;
			}
			}
			break;

		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */
		                      /* op1 = type, val.a = field address            */

			{
			fieldinfo *fi = iptr->val.a;
			unresolved_field *uf = iptr->target;

			switch (iptr->op1) {
			case TYPE_INT:
			case TYPE_FLT:
				if (fi == NULL || !fi->class->initialized) {
					gen_PATCHER_PUTSTATIC_INT(((Inst **)cd), 0, uf);
				} else {
					gen_PUTSTATIC_INT(((Inst **)cd), (u1 *)&(fi->value.i), uf);
				}
				break;
			case TYPE_LNG:
			case TYPE_DBL:
				if (fi == NULL || !fi->class->initialized) {
					gen_PATCHER_PUTSTATIC_LONG(((Inst **)cd), 0, uf);
				} else {
					gen_PUTSTATIC_LONG(((Inst **)cd), (u1 *)&(fi->value.l), uf);
				}
				break;
			case TYPE_ADR:
				if (fi == NULL || !fi->class->initialized) {
					gen_PATCHER_PUTSTATIC_CELL(((Inst **)cd), 0, uf);
				} else {
					gen_PUTSTATIC_CELL(((Inst **)cd), (u1 *)&(fi->value.a), uf);
				}
				break;
			}
			}
			break;

		case ICMD_PUTSTATICCONST: /* ...  ==> ...                             */
		                          /* val = value (in current instruction)     */
		                          /* op1 = type, val.a = field address (in    */
		                          /* following NOP)                           */

			{
			fieldinfo *fi = iptr[1].val.a;
			unresolved_field *uf = iptr[1].target;

			switch (iptr->op1) {
			case TYPE_INT:
			case TYPE_FLT:
				gen_ICONST(((Inst **)cd), iptr->val.i);
				if (fi == NULL || !fi->class->initialized) {
					gen_PATCHER_PUTSTATIC_INT(((Inst **)cd), 0, uf);
				} else {
					gen_PUTSTATIC_INT(((Inst **)cd), (u1 *)&(fi->value.i), uf);
				}
				break;
			case TYPE_LNG:
			case TYPE_DBL:
				gen_LCONST(((Inst **)cd), iptr->val.l);
				if (fi == NULL || !fi->class->initialized) {
					gen_PATCHER_PUTSTATIC_LONG(((Inst **)cd), 0, uf);
				} else {
					gen_PUTSTATIC_LONG(((Inst **)cd), (u1 *)&(fi->value.l), uf);
				}
				break;
			case TYPE_ADR:
				gen_ACONST(((Inst **)cd), iptr->val.a);
				if (fi == NULL || !fi->class->initialized) {
					gen_PATCHER_PUTSTATIC_CELL(((Inst **)cd), 0, uf);
				} else {
					gen_PUTSTATIC_CELL(((Inst **)cd), (u1 *)&(fi->value.a), uf);
				}
				break;
			}
			}
			break;


		case ICMD_GETFIELD:   /* ...  ==> ..., value                          */
		                      /* op1 = type, val.a = field address            */

			{
			fieldinfo *fi = iptr->val.a;
			unresolved_field *uf = iptr->target;

			switch (iptr->op1) {
			case TYPE_INT:
			case TYPE_FLT:
				if (fi == NULL) {
					gen_PATCHER_GETFIELD_INT(((Inst **)cd), 0, uf);
				} else {
					gen_GETFIELD_INT(((Inst **)cd), fi->offset, uf);
				}
				break;
			case TYPE_LNG:
			case TYPE_DBL:
				if (fi == NULL) {
					gen_PATCHER_GETFIELD_LONG(((Inst **)cd), 0, uf);
				} else {
					gen_GETFIELD_LONG(((Inst **)cd), fi->offset, uf);
				}
				break;
			case TYPE_ADR:
				if (fi == NULL) {
					gen_PATCHER_GETFIELD_CELL(((Inst **)cd), 0, uf);
				} else {
					gen_GETFIELD_CELL(((Inst **)cd), fi->offset, uf);
				}
				break;
			}
			}
			break;

		case ICMD_PUTFIELD:   /* ..., objectref, value  ==> ...               */
		                      /* op1 = type, val.a = field address            */

			{
			fieldinfo *fi = iptr->val.a;
			unresolved_field *uf = iptr->target;

			switch (iptr->op1) {
			case TYPE_INT:
			case TYPE_FLT:
				if (fi == NULL) {
					gen_PATCHER_PUTFIELD_INT(((Inst **)cd), 0, uf);
				} else {
					gen_PUTFIELD_INT(((Inst **)cd), fi->offset, uf);
				}
				break;
			case TYPE_LNG:
			case TYPE_DBL:
				if (fi == NULL) {
					gen_PATCHER_PUTFIELD_LONG(((Inst **)cd), 0, uf);
				} else {
					gen_PUTFIELD_LONG(((Inst **)cd), fi->offset, uf);
				}
				break;
			case TYPE_ADR:
				if (fi == NULL) {
					gen_PATCHER_PUTFIELD_CELL(((Inst **)cd), 0, uf);
				} else {
					gen_PUTFIELD_CELL(((Inst **)cd), fi->offset, uf);
				}
				break;
			}
			}
			break;

		case ICMD_PUTFIELDCONST:  /* ..., objectref  ==> ...                  */
		                          /* val = value (in current instruction)     */
		                          /* op1 = type, val.a = field address (in    */
		                          /* following NOP)                           */

			{
			fieldinfo *fi = iptr[1].val.a;
			unresolved_field *uf = iptr[1].target;

			switch (iptr[1].op1) {
			case TYPE_INT:
			case TYPE_FLT:
				gen_ICONST(((Inst **)cd), iptr->val.i);
				if (fi == NULL) {
					gen_PATCHER_PUTFIELD_INT(((Inst **)cd), 0, uf);
				} else {
					gen_PUTFIELD_INT(((Inst **)cd), fi->offset, uf);
				}
				break;
			case TYPE_LNG:
			case TYPE_DBL:
				gen_LCONST(((Inst **)cd), iptr->val.l);
				if (fi == NULL) {
					gen_PATCHER_PUTFIELD_LONG(((Inst **)cd), 0, uf);
				} else {
					gen_PUTFIELD_LONG(((Inst **)cd), fi->offset, uf);
				}
				break;
			case TYPE_ADR:
				gen_ACONST(((Inst **)cd), iptr->val.a);
				if (fi == NULL) {
					gen_PATCHER_PUTFIELD_CELL(((Inst **)cd), 0, uf);
				} else {
					gen_PUTFIELD_CELL(((Inst **)cd), fi->offset, uf);
				}
				break;
			}
			}
			break;


		/* branch operations **************************************************/

		case ICMD_ATHROW:       /* ..., objectref ==> ... (, objectref)       */

			gen_ATHROW(((Inst **)cd));
			break;

		case ICMD_GOTO:         /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */
			gen_branch(GOTO);
			break;

		case ICMD_JSR:          /* ... ==> ...                                */
		                        /* op1 = target JavaVM pc                     */
			gen_branch(JSR);
			break;
			
		case ICMD_RET:          /* ... ==> ...                                */
		                        /* op1 = local variable                       */

			gen_RET(((Inst **)cd), index2offset(iptr->op1));
			break;

		case ICMD_IFNULL:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			gen_branch(IFNULL);
			break;

		case ICMD_IFNONNULL:    /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc                     */

			gen_branch(IFNONNULL);
			break;

		case ICMD_IFEQ:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (iptr->val.i == 0) {
				gen_branch(IFEQ);
			} else {
				gen_ICONST(((Inst **)cd), iptr->val.i);
				gen_branch(IF_ICMPEQ);
			}
			break;

		case ICMD_IFLT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (iptr->val.i == 0) {
				gen_branch(IFLT);
			} else {
				gen_ICONST(((Inst **)cd), iptr->val.i);
				gen_branch(IF_ICMPLT);
			}
			break;

		case ICMD_IFLE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (iptr->val.i == 0) {
				gen_branch(IFLE);
			} else {
				gen_ICONST(((Inst **)cd), iptr->val.i);
				gen_branch(IF_ICMPLE);
			}
			break;

		case ICMD_IFNE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (iptr->val.i == 0) {
				gen_branch(IFNE);
			} else {
				gen_ICONST(((Inst **)cd), iptr->val.i);
				gen_branch(IF_ICMPNE);
			}
			break;

		case ICMD_IFGT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (iptr->val.i == 0) {
				gen_branch(IFGT);
			} else {
				gen_ICONST(((Inst **)cd), iptr->val.i);
				gen_branch(IF_ICMPGT);
			}
			break;

		case ICMD_IFGE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (iptr->val.i == 0) {
				gen_branch(IFGE);
			} else {
				gen_ICONST(((Inst **)cd), iptr->val.i);
				gen_branch(IF_ICMPGE);
			}
			break;

		case ICMD_IF_LEQ:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			gen_LCONST(((Inst **)cd), iptr->val.l);
			gen_branch(IF_LCMPEQ);
			break;

		case ICMD_IF_LLT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			gen_LCONST(((Inst **)cd), iptr->val.l);
			gen_branch(IF_LCMPLT);
			break;

		case ICMD_IF_LLE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			gen_LCONST(((Inst **)cd), iptr->val.l);
			gen_branch(IF_LCMPLE);
			break;

		case ICMD_IF_LNE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			gen_LCONST(((Inst **)cd), iptr->val.l);
			gen_branch(IF_LCMPNE);
			break;

		case ICMD_IF_LGT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			gen_LCONST(((Inst **)cd), iptr->val.l);
			gen_branch(IF_LCMPGT);
			break;

		case ICMD_IF_LGE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			gen_LCONST(((Inst **)cd), iptr->val.l);
			gen_branch(IF_LCMPGE);
			break;

		case ICMD_IF_ICMPEQ:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			gen_branch(IF_ICMPEQ);
			break;

		case ICMD_IF_LCMPEQ:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			gen_branch(IF_LCMPEQ);
			break;

		case ICMD_IF_ACMPEQ:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			gen_branch(IF_ACMPEQ);
			break;

		case ICMD_IF_ICMPNE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			gen_branch(IF_ICMPNE);
			break;

		case ICMD_IF_LCMPNE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			gen_branch(IF_LCMPNE);
			break;

		case ICMD_IF_ACMPNE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			gen_branch(IF_ACMPNE);
			break;

		case ICMD_IF_ICMPLT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			gen_branch(IF_ICMPLT);
			break;

		case ICMD_IF_LCMPLT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			gen_branch(IF_LCMPLT);
			break;

		case ICMD_IF_ICMPGT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			gen_branch(IF_ICMPGT);
			break;

		case ICMD_IF_LCMPGT:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			gen_branch(IF_LCMPGT);
			break;

		case ICMD_IF_ICMPLE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			gen_branch(IF_ICMPLE);
			break;

		case ICMD_IF_LCMPLE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			gen_branch(IF_LCMPLE);
			break;

		case ICMD_IF_ICMPGE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			gen_branch(IF_ICMPGE);
			break;

		case ICMD_IF_LCMPGE:    /* ..., value, value ==> ...                  */
		                        /* op1 = target JavaVM pc                     */

			gen_branch(IF_LCMPGE);
			break;


		case ICMD_ARETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_IRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_FRETURN:      /* ..., retvalue ==> ...                      */

#if defined(USE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				if (m->flags & ACC_STATIC) {
					gen_ACONST(((Inst **)cd), (java_objectheader *) m->class);
				} else {
					gen_ALOAD(((Inst **)cd), 0);
				}
				gen_MONITOREXIT(((Inst **)cd));
			}
#endif
			if (runverbose)
				gen_TRACERETURN(((Inst **)cd));

			gen_IRETURN(((Inst **)cd), index2offset(cd->maxlocals));
			break;

		case ICMD_LRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_DRETURN:      /* ..., retvalue ==> ...                      */

#if defined(USE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				if (m->flags & ACC_STATIC) {
					gen_ACONST(((Inst **)cd), (java_objectheader *) m->class);
				} else {
					gen_ALOAD(((Inst **)cd), 0);
				}
				gen_MONITOREXIT(((Inst **)cd));
			}
#endif
			if (runverbose)
				gen_TRACELRETURN(((Inst **)cd));

			gen_LRETURN(((Inst **)cd), index2offset(cd->maxlocals));
			break;

		case ICMD_RETURN:       /* ...  ==> ...                               */

#if defined(USE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				if (m->flags & ACC_STATIC) {
					gen_ACONST(((Inst **)cd), (java_objectheader *) m->class);
				} else {
					gen_ALOAD(((Inst **)cd), 0);
				}
				gen_MONITOREXIT(((Inst **)cd));
			}
#endif
			if (runverbose)
				gen_TRACERETURN(((Inst **)cd));

			gen_RETURN(((Inst **)cd), index2offset(cd->maxlocals));
			break;


		case ICMD_TABLESWITCH:  /* ..., index ==> ...                         */
			{
			s4 i, l, *s4ptr;
			void **tptr;

			tptr = (void **) iptr->target;

			s4ptr = iptr->val.a;
			l = s4ptr[1];                          /* low     */
			i = s4ptr[2];                          /* high    */
			
			i = i - l + 1;

			/* arguments: low, range, datasegment address, table offset in     */
			/* datasegment, default target                                    */
			gen_TABLESWITCH(((Inst **)cd), l, i, NULL, 0, NULL);
			dseg_adddata(cd, (cd->mcodeptr - 2*sizeof(Inst))); /* actually -3 cells offset*/
			codegen_addreference(cd, (basicblock *) tptr[0], cd->mcodeptr);

			/* build jump table top down and use address of lowest entry */

			tptr += i;

			while (--i >= 0) {
				dseg_addtarget(cd, (basicblock *) tptr[0]); 
				--tptr;
			}
			}

			/* length of dataseg after last dseg_addtarget is used by load */
			((ptrint *)(cd->mcodeptr))[-2] = (ptrint) -(cd->dseglen);
			break;


		case ICMD_LOOKUPSWITCH: /* ..., key ==> ...                           */
			{
			s4 i, *s4ptr;
			void **tptr;

			tptr = (void **) iptr->target;

			s4ptr = iptr->val.a;

			/* s4ptr[0] is equal to tptr[0] */
			i = s4ptr[1];                          /* count    */
			
			/* arguments: count, datasegment address, table offset in         */
			/* datasegment, default target                                    */
			gen_LOOKUPSWITCH(((Inst **)cd), i, NULL, 0, NULL);
			dseg_adddata(cd, (cd->mcodeptr - 2*sizeof(Inst))); /* actually -3 cells offset*/
			codegen_addreference(cd, (basicblock *) tptr[0], cd->mcodeptr);

			/* build jump table top down and use address of lowest entry */

			tptr += i;
			s4ptr += i * 2;

			while (--i >= 0) {
				dseg_addtarget(cd, (basicblock *) tptr[0]); 
				dseg_addaddress(cd, s4ptr[0]);
				--tptr;
				s4ptr -= 2;
			}
			}

			/* length of dataseg after last dseg_addtarget is used by load */
			((ptrint *)(cd->mcodeptr))[-2] = (ptrint) -(cd->dseglen);
			break;


		case ICMD_BUILTIN:      /* ..., arg1, arg2, arg3 ==> ...              */
		                        /* op1 = arg count val.a = builtintable entry */
			bte = iptr->val.a;

			for (i = 0; i < sizeof(builtin_gen_table)/sizeof(builtin_gen); i++) {
				builtin_gen *bg = &builtin_gen_table[i];
				if (bg->builtin == bte->fp) {
					(bg->gen)(((Inst **)cd));
					goto gen_builtin_end;
				}
			}
			assert(0);
		gen_builtin_end:
			break;

		case ICMD_INVOKESTATIC: /* ..., [arg1, [arg2 ...]] ==> ...            */
		                        /* op1 = arg count, val.a = method pointer    */

			lm = iptr->val.a;
			um = iptr->target;

			if (lm == NULL) {
				md = um->methodref->parseddesc.md;
				gen_PATCHER_INVOKESTATIC(((Inst **)cd), 0, md->paramslots, um);

			} else {
				md = lm->parseddesc;
				gen_INVOKESTATIC(((Inst **)cd), (Inst **)lm->stubroutine, md->paramslots, um);
			}
			break;

		case ICMD_INVOKESPECIAL:/* ..., objectref, [arg1, [arg2 ...]] ==> ... */

			lm = iptr->val.a;
			um = iptr->target;

			if (lm == NULL) {
				md = um->methodref->parseddesc.md;
				gen_PATCHER_INVOKESPECIAL(((Inst **)cd), 0, md->paramslots, um);

			} else {
				md = lm->parseddesc;
				gen_INVOKESPECIAL(((Inst **)cd), (Inst **)lm->stubroutine, md->paramslots, um);
			}
			break;

		case ICMD_INVOKEVIRTUAL:/* op1 = arg count, val.a = method pointer    */

			lm = iptr->val.a;
			um = iptr->target;

			if (lm == NULL) {
				md = um->methodref->parseddesc.md;
				gen_PATCHER_INVOKEVIRTUAL(((Inst **)cd), 0, md->paramslots, um);

			} else {
				md = lm->parseddesc;

				s1 = OFFSET(vftbl_t, table[0]) +
					sizeof(methodptr) * lm->vftblindex;

				gen_INVOKEVIRTUAL(((Inst **)cd), s1, md->paramslots, um);
			}
			break;

		case ICMD_INVOKEINTERFACE:/* op1 = arg count, val.a = method pointer  */

			lm = iptr->val.a;
			um = iptr->target;

			if (lm == NULL) {
				md = um->methodref->parseddesc.md;
				gen_PATCHER_INVOKEINTERFACE(((Inst **)cd), 0, 0, md->paramslots, um);

			} else {
				md = lm->parseddesc;

				s1 = OFFSET(vftbl_t, interfacetable[0]) -
					sizeof(methodptr*) * lm->class->index;

				s2 = sizeof(methodptr) * (lm - lm->class->methods);

				gen_INVOKEINTERFACE(((Inst **)cd), s1, s2, md->paramslots, um);
			}
			break;


		case ICMD_CHECKCAST:  /* ..., objectref ==> ..., objectref            */
		                      /* op1:   0 == array, 1 == class                */
		                      /* val.a: (classinfo *) superclass              */

			if (iptr->op1 == 1) {
				if (iptr->val.a == NULL)
					gen_PATCHER_CHECKCAST(((Inst **) cd), NULL, iptr->target);
				else
					gen_CHECKCAST(((Inst **) cd), iptr->val.a, NULL);
			} else {
				if (iptr->val.a == NULL)
					gen_PATCHER_ARRAYCHECKCAST(((Inst **) cd), NULL, iptr->target);
				else
					gen_ARRAYCHECKCAST(((Inst **) cd), iptr->val.a, NULL);
			}
			break;

		case ICMD_INSTANCEOF: /* ..., objectref ==> ..., intresult            */
		                      /* op1:   0 == array, 1 == class                */
		                      /* val.a: (classinfo *) superclass              */

			if (iptr->val.a == NULL)
				gen_PATCHER_INSTANCEOF(((Inst **) cd), 0, iptr->target);
			else
				gen_INSTANCEOF(((Inst **) cd), iptr->val.a, iptr->target);
			break;

		case ICMD_MULTIANEWARRAY:/* ..., cnt1, [cnt2, ...] ==> ..., arrayref  */
		                      /* op1 = dimension, val.a = array descriptor    */

			if (iptr->target) {
				gen_PATCHER_MULTIANEWARRAY(((Inst **)cd), 0, iptr->op1, iptr->val.a);
			} else {
				gen_MULTIANEWARRAY(((Inst **)cd), iptr->val.a, iptr->op1, 0);
			}
			break;

		default:
			*exceptionptr = new_internalerror("Unknown ICMD %d", iptr->opc);
			return false;
	} /* switch */
		
	} /* for instruction */
		
	} /* if (bptr -> flags >= BBREACHED) */
	} /* for basic block */

	codegen_createlinenumbertable(cd);

	codegen_finish(m, cd, (s4) (cd->mcodeptr - cd->mcodebase));

#ifdef VM_PROFILING
	vm_block_insert(m->mcode + m->mcodelength);
#endif

	/* branch resolving (walk through all basic blocks) */

	for (bptr = m->basicblocks; bptr != NULL; bptr = bptr->next) {
		branchref *brefs;

		for (brefs = bptr->branchrefs; brefs != NULL; brefs = brefs->next) {
			gen_resolveanybranch(((u1*) m->entrypoint) + brefs->branchpos,
			                     ((u1 *)m->entrypoint) + bptr->mpc);
		}
	}

	/* everything's ok */

	return true;
}


/* a stub consists of

+---------+
|codeptr  |
+---------+
|maxlocals|
+---------+
|TRANSLATE|
+---------+
|methodinf|
+---------+

codeptr points either to TRANSLATE or to the translated threaded code

all methods are called indirectly through methodptr
*/

#define COMPILERSTUB_SIZE 4

functionptr createcompilerstub (methodinfo *m)
{
	Inst        *s;
	codegendata *cd;
	s4           dumpsize;

	s = CNEW(Inst, COMPILERSTUB_SIZE);

	/* mark start of dump memory area */

	dumpsize = dump_size();
	
	cd = DNEW(codegendata);
    cd->mcodeptr = (u1 *) s;

	genarg_ainst((Inst **) cd, s + 2);

	if (m->flags & ACC_NATIVE) {
		genarg_i((Inst **) cd, m->parseddesc->paramslots);
	} else {
		genarg_i((Inst **) cd, m->maxlocals);
	}

	gen_BBSTART;
	gen_TRANSLATE((Inst **) cd, m);
	
#ifdef VM_PROFILING
	vm_block_insert(cd->mcodeptr);
#endif

#if defined(STATISTICS)
	if (opt_stat)
		count_cstub_len += COMPILERSTUB_SIZE;
#endif

	/* release dump area */

	dump_release(dumpsize);
	
	return (functionptr) s;
}


/* native stub:
+---------+
|NATIVECALL|
+---------+
|methodinf|
+---------+
|function |
+---------+
|cif      |
+---------+
*/

static ffi_cif *createnativecif(methodinfo *m, methoddesc *nmd)
{
	methoddesc  *md = m->parseddesc; 
	ffi_cif     *pcif = NEW(ffi_cif);
	ffi_type   **types = MNEW(ffi_type *, nmd->paramcount);
	ffi_type   **ptypes = types;
	s4           i;

	/* pass env pointer */

	*ptypes++ = &ffi_type_pointer;

	/* for static methods, pass class pointer */

	if (m->flags & ACC_STATIC)
		*ptypes++ = &ffi_type_pointer;

	/* pass parameter to native function */

	for (i = 0; i < md->paramcount; i++)
		*ptypes++ = cacaotype2ffitype(md->paramtypes[i].type);

	assert(ptypes - types == nmd->paramcount);

    if (ffi_prep_cif(pcif, FFI_DEFAULT_ABI, nmd->paramcount, cacaotype2ffitype(md->returntype.type), types) != FFI_OK)
		assert(0);

	return pcif;
}


functionptr createnativestub(functionptr f, methodinfo *m, codegendata *cd,
							 registerdata *rd, methoddesc *nmd)
{
	ffi_cif *cif;

	cd->mcodeptr = cd->mcodebase;
	cd->mcodeend = (s4 *) (cd->mcodebase + cd->mcodesize);

	/* create method header */

	(void) dseg_addaddress(cd, m);                          /* MethodPointer  */
	(void) dseg_adds4(cd, nmd->paramslots * SIZEOF_VOID_P); /* FrameSize      */
	(void) dseg_adds4(cd, 0);                               /* IsSync         */
	(void) dseg_adds4(cd, 0);                               /* IsLeaf         */
	(void) dseg_adds4(cd, 0);                               /* IntSave        */
	(void) dseg_adds4(cd, 0);                               /* FltSave        */
	dseg_addlinenumbertablesize(cd);
	(void) dseg_adds4(cd, 0);                               /* ExTableSize    */

	/* prepare ffi cif structure */

	cif = createnativecif(m, nmd);

	gen_BBSTART;

	if (runverbose)
		gen_TRACECALL(((Inst **)cd));

	if (f == NULL) {
		gen_PATCHER_NATIVECALL(((Inst **)cd), m, f, (u1 *)cif);
	} else {
		if (runverbose)
			gen_TRACENATIVECALL(((Inst **)cd), m, f, (u1 *)cif);
		else
			gen_NATIVECALL(((Inst **)cd), m, f, (u1 *)cif);
	}

	codegen_finish(m, cd, (s4) (cd->mcodeptr - cd->mcodebase));

#ifdef VM_PROFILING
	vm_block_insert(m->mcode + m->mcodelength);
#endif

	return m->entrypoint;
}


functionptr createcalljavafunction(methodinfo *m)
{
	methodinfo         *tmpm;
	functionptr         entrypoint;
	codegendata        *cd;
	registerdata       *rd;
	t_inlining_globals *id;
	s4                  dumpsize;
	methoddesc         *md;

	/* mark dump memory */

	dumpsize = dump_size();

	tmpm = DNEW(methodinfo);
	cd = DNEW(codegendata);
	rd = DNEW(registerdata);
	id = DNEW(t_inlining_globals);

	/* setup code generation stuff */

	MSET(tmpm, 0, u1, sizeof(methodinfo));

	inlining_setup(tmpm, id);
	codegen_setup(tmpm, cd, id);

	md = m->parseddesc;

	cd->mcodeptr = cd->mcodebase;
	cd->mcodeend = (s4 *) (cd->mcodebase + cd->mcodesize);

	/* create method header */

	(void) dseg_addaddress(cd, NULL);                       /* MethodPointer  */
	(void) dseg_adds4(cd, md->paramslots * SIZEOF_VOID_P);  /* FrameSize      */
	(void) dseg_adds4(cd, 0);                               /* IsSync         */
	(void) dseg_adds4(cd, 0);                               /* IsLeaf         */
	(void) dseg_adds4(cd, 0);                               /* IntSave        */
	(void) dseg_adds4(cd, 0);                               /* FltSave        */
	dseg_addlinenumbertablesize(cd);
	(void) dseg_adds4(cd, 0);                               /* ExTableSize    */


	/* generate code */
	
	gen_BBSTART;
	gen_INVOKESTATIC(((Inst **)cd), (Inst **)m->stubroutine, md->paramslots, 0);
	gen_END(((Inst **)cd));
  
	codegen_finish(tmpm, cd, (s4) (cd->mcodeptr - cd->mcodebase));

#ifdef VM_PROFILING
	vm_block_insert(tmpm->mcode + tmpm->mcodelength);
#endif
	entrypoint = tmpm->entrypoint;

	/* release memory */

	dump_release(dumpsize);

	return entrypoint;
}


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
