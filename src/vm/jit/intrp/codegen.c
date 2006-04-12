/* src/vm/jit/intrp/codegen.c - code generator for Interpreter

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

   Authors: Christian Thalinger
            Anton Ertl
			
   Changes: Edwin Steiner

   $Id: codegen.c 4760 2006-04-12 20:06:23Z edwin $

*/


#include "config.h"

#include <assert.h>
#include <stdio.h>

#if defined(WITH_FFI)
# include <ffi.h>
#elif defined(WITH_FFCALL)
# include <avcall.h>
#else
# error neither WITH_FFI nor WITH_FFCALL defined
#endif

#include "vm/types.h"

#include "arch.h"

#include "vm/jit/intrp/codegen.h"
#include "vm/jit/intrp/intrp.h"

#include "native/native.h"
#include "vm/builtin.h"
#include "vm/class.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/options.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/codegen-common.h"
#include "vm/jit/dseg.h"
#include "vm/jit/jit.h"
#include "vm/jit/parse.h"
#include "vm/jit/patcher.h"


#define gen_branch(_inst) { \
  gen_##_inst(cd, 0); \
  codegen_addreference(cd, (basicblock *) (iptr->target), cd->mcodeptr); \
}

#define index2offset(_i) (-(_i) * SIZEOF_VOID_P)

/* functions used by cacao-gen.i */

void genarg_v(codegendata *cd1, Cell v)
{
	Inst **mcodepp = (Inst **) &(cd1->mcodeptr);
	*((Cell *) *mcodepp) = v;
	(*mcodepp)++;
}

void genarg_i(codegendata *cd1, s4 i)
{
	Inst **mcodepp = (Inst **) &(cd1->mcodeptr);
	*((Cell *) *mcodepp) = i;
	(*mcodepp)++;
}

void genarg_b(codegendata *cd1, s4 i)
{
  genarg_i(cd1, i);
}

void genarg_f(codegendata *cd1, float f)
{
	s4 fi;

	vm_f2Cell(f,fi);
	genarg_i(cd1, fi);
}

void genarg_l(codegendata *cd1, s8 l)
{
	Inst **mcodepp = (Inst **) &(cd1->mcodeptr);
	vm_l2twoCell(l, ((Cell *) *mcodepp)[1], ((Cell *) *mcodepp)[0]);
	(*mcodepp) +=2;
}

void genarg_aRef(codegendata *cd1, java_objectheader *a)
{
	Inst **mcodepp = (Inst **) &(cd1->mcodeptr);
	*((java_objectheader **) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_aArray(codegendata *cd1, java_arrayheader *a)
{
	Inst **mcodepp = (Inst **) &(cd1->mcodeptr);
	*((java_arrayheader **) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_aaTarget(codegendata *cd1, Inst **a)
{
	Inst **mcodepp = (Inst **) &(cd1->mcodeptr);
	*((Inst ***) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_aClass(codegendata *cd1, classinfo *a)
{
	Inst **mcodepp = (Inst **) &(cd1->mcodeptr);
	*((classinfo **) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_acr(codegendata *cd1, constant_classref *a)
{
	Inst **mcodepp = (Inst **) &(cd1->mcodeptr);
	*((constant_classref **) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_addr(codegendata *cd1, u1 *a)
{
	Inst **mcodepp = (Inst **) &(cd1->mcodeptr);
	*((u1 **) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_af(codegendata *cd1, functionptr a)
{
	Inst **mcodepp = (Inst **) &(cd1->mcodeptr);
	*((functionptr *) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_afi(codegendata *cd1, fieldinfo *a)
{
	Inst **mcodepp = (Inst **) &(cd1->mcodeptr);
	*((fieldinfo **) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_am(codegendata *cd1, methodinfo *a)
{
	Inst **mcodepp = (Inst **) &(cd1->mcodeptr);
	*((methodinfo **) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_acell(codegendata *cd1, Cell *a)
{
	Inst **mcodepp = (Inst **) &(cd1->mcodeptr);
	*((Cell **) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_ainst(codegendata *cd1, Inst *a)
{
	Inst **mcodepp = (Inst **) &(cd1->mcodeptr);
	*((Inst **) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_auf(codegendata *cd1, unresolved_field *a)
{
	Inst **mcodepp = (Inst **) &(cd1->mcodeptr);
	*((unresolved_field **) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_aum(codegendata *cd1, unresolved_method *a)
{
	Inst **mcodepp = (Inst **) &(cd1->mcodeptr);
	*((unresolved_method **) *mcodepp) = a;
	(*mcodepp)++;
}

void genarg_avftbl(codegendata *cd1, vftbl_t *a)
{
	Inst **mcodepp = (Inst **) &(cd1->mcodeptr);
	*((vftbl_t **) *mcodepp) = a;
	(*mcodepp)++;
}


/* include the interpreter generation functions *******************************/

#include "vm/jit/intrp/java-gen.i"


typedef void (*genfunctionptr) (codegendata *);

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

#if !(SUPPORT_FLOAT && SUPPORT_LONG && SUPPORT_F2L)
    {BUILTIN_f2l,                     gen_F2L,             },
#endif

#if !(SUPPORT_DOUBLE && SUPPORT_LONG && SUPPORT_D2L)
    {BUILTIN_d2l,					  gen_D2L, 			   },
#endif

#if !(SUPPORT_FLOAT && SUPPORT_F2I)
    {BUILTIN_f2i,					  gen_F2I, 			   },
#endif

#if !(SUPPORT_DOUBLE && SUPPORT_D2I)
    {BUILTIN_d2i,					  gen_D2I, 			   },
#endif

#if !SUPPORT_DIVISION
    {BUILTIN_idiv,					  gen_IDIV,			   },
    {BUILTIN_irem,					  gen_IREM,			   },
#endif

#if !(SUPPORT_DIVISION && SUPPORT_LONG && SUPPORT_LONG_DIV)
    {BUILTIN_ldiv,					  gen_LDIV,			   },
    {BUILTIN_lrem,					  gen_LREM,			   },
#endif

    {BUILTIN_frem,					  gen_FREM,			   },
    {BUILTIN_drem,					  gen_DREM,            },
};


/* codegen *********************************************************************

   Generates machine code.

*******************************************************************************/

bool intrp_codegen(jitdata *jd)
{
	methodinfo         *m;
	codegendata        *cd;
	registerdata       *rd;
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

	/* get required compiler data */

	m  = jd->m;
	cd = jd->cd;
	rd = jd->rd;

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

#if 0	
	/* initialize mcode variables */
	
	cd->mcodeptr = cd->mcodebase;
	cd->mcodeend = (s4 *) (cd->mcodebase + cd->mcodesize);
#endif

	gen_BBSTART;

#if defined(USE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		if (m->flags & ACC_STATIC)
			gen_ACONST(cd, (java_objectheader *) m->class);
		else
			gen_ALOAD(cd, 0);
		
		gen_MONITORENTER(cd);
	}			
#endif

	if (opt_verbosecall)
		gen_TRACECALL(cd, m);

	gen_BBEND;

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

			gen_CHECKNULL(cd);
			break;

		/* constant operations ************************************************/

		case ICMD_ICONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.i = constant                    */

			gen_ICONST(cd, iptr->val.i);
			break;

		case ICMD_LCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.l = constant                    */

			gen_LCONST(cd, iptr->val.l);
			break;

		case ICMD_FCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.f = constant                    */
			{
				ptrint fi;

				vm_f2Cell(iptr->val.f, fi);
				gen_ICONST(cd, fi);
			}
			break;
			
		case ICMD_DCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.d = constant                    */

			gen_LCONST(cd, *(s8 *)&(iptr->val.d));
			break;

		case ICMD_ACONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.a = constant                    */

			if ((iptr->target != NULL) && (iptr->val.a == NULL))
				gen_PATCHER_ACONST(cd, NULL, iptr->target);
			else
				gen_ACONST(cd, iptr->val.a);
			break;


		/* load/store operations **********************************************/

		case ICMD_ILOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			gen_ILOAD(cd, index2offset(iptr->op1));
			break;

		case ICMD_LLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			gen_LLOAD(cd, index2offset(iptr->op1));
			break;

		case ICMD_ALOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			gen_ALOAD(cd, index2offset(iptr->op1));
			break;

		case ICMD_FLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			gen_ILOAD(cd, index2offset(iptr->op1));
			break;

		case ICMD_DLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			gen_LLOAD(cd, index2offset(iptr->op1));
			break;


		case ICMD_ISTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			gen_ISTORE(cd, index2offset(iptr->op1));
			break;

		case ICMD_LSTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			gen_LSTORE(cd, index2offset(iptr->op1));
			break;

		case ICMD_ASTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			gen_ASTORE(cd, index2offset(iptr->op1));
			break;


		case ICMD_FSTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			gen_ISTORE(cd, index2offset(iptr->op1));
			break;

		case ICMD_DSTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			gen_LSTORE(cd, index2offset(iptr->op1));
			break;


		/* pop/dup/swap operations ********************************************/

		/* attention: double and longs are only one entry in CACAO ICMDs      */

		/* stack.c changes stack manipulation operations to treat
		   longs/doubles as occupying a single slot.  Here we are
		   undoing that (and only those things that stack.c did). */

		case ICMD_POP:        /* ..., value  ==> ...                          */

			if (IS_2_WORD_TYPE(src->type))
				gen_POP2(cd);
			else
				gen_POP(cd);
			break;

		case ICMD_POP2:       /* ..., value, value  ==> ...                   */

			gen_POP2(cd);
			break;

		case ICMD_DUP:        /* ..., a ==> ..., a, a                         */

			if (IS_2_WORD_TYPE(src->type))
				gen_DUP2(cd);
			else
				gen_DUP(cd);
			break;

		case ICMD_DUP_X1:     /* ..., a, b ==> ..., b, a, b                   */

			if (IS_2_WORD_TYPE(src->type)) {
				if (IS_2_WORD_TYPE(src->prev->type)) {
					gen_DUP2_X2(cd);
				} else {
					gen_DUP2_X1(cd);
				}
			} else {
				if (IS_2_WORD_TYPE(src->prev->type)) {
					gen_DUP_X2(cd);
				} else {
					gen_DUP_X1(cd);
				}
			}
			break;

		case ICMD_DUP_X2:     /* ..., a, b, c ==> ..., c, a, b, c             */

			if (IS_2_WORD_TYPE(src->type)) {
				gen_DUP2_X2(cd);
			} else
				gen_DUP_X2(cd);
			break;

		case ICMD_DUP2:       /* ..., a, b ==> ..., a, b, a, b                */

			gen_DUP2(cd);
			break;

		case ICMD_DUP2_X1:    /* ..., a, b, c ==> ..., b, c, a, b, c          */

			if (IS_2_WORD_TYPE(src->prev->prev->type))
				gen_DUP2_X2(cd);
			else
				gen_DUP2_X1(cd);
			break;

		case ICMD_DUP2_X2:    /* ..., a, b, c, d ==> ..., c, d, a, b, c, d    */

			gen_DUP2_X2(cd);
			break;

		case ICMD_SWAP:       /* ..., a, b ==> ..., b, a                      */

			gen_SWAP(cd);
			break;


		/* integer operations *************************************************/

		case ICMD_INEG:       /* ..., value  ==> ..., - value                 */

			gen_INEG(cd);
			break;

		case ICMD_LNEG:       /* ..., value  ==> ..., - value                 */

			gen_LNEG(cd);
			break;

		case ICMD_I2L:        /* ..., value  ==> ..., value                   */

			gen_I2L(cd);
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */

			gen_L2I(cd);
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			gen_INT2BYTE(cd);
			break;

		case ICMD_INT2CHAR:   /* ..., value  ==> ..., value                   */

			gen_INT2CHAR(cd);
			break;

		case ICMD_INT2SHORT:  /* ..., value  ==> ..., value                   */

			gen_INT2SHORT(cd);
			break;


		case ICMD_IADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			gen_IADD(cd);
			break;

		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			gen_ICONST(cd, iptr->val.i);
			gen_IADD(cd);
			break;

		case ICMD_LADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			gen_LADD(cd);
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.l = constant                             */

			gen_LCONST(cd, iptr->val.l);
			gen_LADD(cd);
			break;

		case ICMD_ISUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			gen_ISUB(cd);
			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			gen_ICONST(cd, iptr->val.i);
			gen_ISUB(cd);
			break;

		case ICMD_LSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			gen_LSUB(cd);
			break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* val.l = constant                             */

			gen_LCONST(cd, iptr->val.l);
			gen_LSUB(cd);
			break;

		case ICMD_IMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			gen_IMUL(cd);
			break;

		case ICMD_IMULCONST:  /* ..., val1, val2  ==> ..., val1 * val2        */

			gen_ICONST(cd, iptr->val.i);
			gen_IMUL(cd);
			break;

		case ICMD_LMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			gen_LMUL(cd);
			break;

		case ICMD_LMULCONST:  /* ..., val1, val2  ==> ..., val1 * val2        */

			gen_LCONST(cd, iptr->val.l);
			gen_LMUL(cd);
			break;

		case ICMD_IDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			gen_IDIV(cd);
			break;

		case ICMD_IREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			gen_IREM(cd);
			break;

		case ICMD_LDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			gen_LDIV(cd);
			break;

		case ICMD_LREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			gen_LREM(cd);
			break;

		case ICMD_IDIVPOW2:   /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */
		                      
			gen_IDIVPOW2(cd, iptr->val.i);
			break;

		case ICMD_IREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.i = constant                             */

			gen_IREMPOW2(cd, iptr->val.i);
			break;

		case ICMD_LDIVPOW2:   /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */
		                      
			gen_LDIVPOW2(cd, iptr->val.i);
			break;

		case ICMD_LREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.l = constant                             */

			gen_LREMPOW2(cd, iptr->val.i);
			break;

		case ICMD_ISHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			gen_ISHL(cd);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			gen_ICONST(cd, iptr->val.i);
			gen_ISHL(cd);
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			gen_ISHR(cd);
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			gen_ICONST(cd, iptr->val.i);
			gen_ISHR(cd);
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			gen_IUSHR(cd);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			gen_ICONST(cd, iptr->val.i);
			gen_IUSHR(cd);
			break;

		case ICMD_LSHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			gen_LSHL(cd);
			break;

		case ICMD_LSHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			gen_ICONST(cd, iptr->val.i);
			gen_LSHL(cd);
			break;

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			gen_LSHR(cd);
			break;

		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			gen_ICONST(cd, iptr->val.i);
			gen_LSHR(cd);
			break;

		case ICMD_LUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			gen_LUSHR(cd);
			break;

		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			gen_ICONST(cd, iptr->val.i);
			gen_LUSHR(cd);
			break;

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			gen_IAND(cd);
			break;

		case ICMD_IANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.i = constant                             */

			gen_ICONST(cd, iptr->val.i);
			gen_IAND(cd);
			break;

		case ICMD_LAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			gen_LAND(cd);
			break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.l = constant                             */

			gen_LCONST(cd, iptr->val.l);
			gen_LAND(cd);
			break;

		case ICMD_IOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			gen_IOR(cd);
			break;

		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.i = constant                             */

			gen_ICONST(cd, iptr->val.i);
			gen_IOR(cd);
			break;

		case ICMD_LOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			gen_LOR(cd);
			break;

		case ICMD_LORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.l = constant                             */

			gen_LCONST(cd, iptr->val.l);
			gen_LOR(cd);
			break;

		case ICMD_IXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			gen_IXOR(cd);
			break;

		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.i = constant                             */

			gen_ICONST(cd, iptr->val.i);
			gen_IXOR(cd);
			break;

		case ICMD_LXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			gen_LXOR(cd);
			break;

		case ICMD_LXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.l = constant                             */

			gen_LCONST(cd, iptr->val.l);
			gen_LXOR(cd);
			break;


		case ICMD_LCMP:       /* ..., val1, val2  ==> ..., val1 cmp val2      */

			gen_LCMP(cd);
			break;


		case ICMD_IINC:       /* ..., value  ==> ..., value + constant        */
		                      /* op1 = variable, val.i = constant             */

			gen_IINC(cd, index2offset(iptr->op1), iptr->val.i);
			break;


		/* floating operations ************************************************/

		case ICMD_FNEG:       /* ..., value  ==> ..., - value                 */

			gen_FNEG(cd);
			break;

		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */

			gen_DNEG(cd);
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			gen_FADD(cd);
			break;

		case ICMD_DADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			gen_DADD(cd);
			break;

		case ICMD_FSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			gen_FSUB(cd);
			break;

		case ICMD_DSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			gen_DSUB(cd);
			break;

		case ICMD_FMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			gen_FMUL(cd);
			break;

		case ICMD_DMUL:       /* ..., val1, val2  ==> ..., val1 *** val2      */

			gen_DMUL(cd);
			break;

		case ICMD_FDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			gen_FDIV(cd);
			break;

		case ICMD_DDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			gen_DDIV(cd);
			break;
		
		case ICMD_FREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			gen_FREM(cd);
			break;

		case ICMD_DREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			gen_DREM(cd);
			break;
		
		case ICMD_I2F:       /* ..., value  ==> ..., (float) value            */

			gen_I2F(cd);
			break;

		case ICMD_L2F:       /* ..., value  ==> ..., (float) value            */

			gen_L2F(cd);
			break;

		case ICMD_I2D:       /* ..., value  ==> ..., (double) value           */

			gen_I2D(cd);
			break;
			
		case ICMD_L2D:       /* ..., value  ==> ..., (double) value           */

			gen_L2D(cd);
			break;
			
		case ICMD_F2I:       /* ..., value  ==> ..., (int) value              */

			gen_F2I(cd);
			break;
		
		case ICMD_D2I:       /* ..., value  ==> ..., (int) value              */

			gen_D2I(cd);
			break;
		
		case ICMD_F2L:       /* ..., value  ==> ..., (long) value             */

			gen_F2L(cd);
			break;

		case ICMD_D2L:       /* ..., value  ==> ..., (long) value             */

			gen_D2L(cd);
			break;

		case ICMD_F2D:       /* ..., value  ==> ..., (double) value           */

			gen_F2D(cd);
			break;
					
		case ICMD_D2F:       /* ..., value  ==> ..., (float) value            */

			gen_D2F(cd);
			break;
		
		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */

			gen_FCMPL(cd);
			break;
			
		case ICMD_DCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */

			gen_DCMPL(cd);
			break;
			
		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */

			gen_FCMPG(cd);
			break;

		case ICMD_DCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */

			gen_DCMPG(cd);
			break;


		/* memory operations **************************************************/

		case ICMD_ARRAYLENGTH: /* ..., arrayref  ==> ..., length              */

			gen_ARRAYLENGTH(cd);
			break;

		case ICMD_BALOAD:     /* ..., arrayref, index  ==> ..., value         */

			gen_BALOAD(cd);
			break;

		case ICMD_CALOAD:     /* ..., arrayref, index  ==> ..., value         */

			gen_CALOAD(cd);
			break;			

		case ICMD_SALOAD:     /* ..., arrayref, index  ==> ..., value         */

			gen_SALOAD(cd);
			break;

		case ICMD_IALOAD:     /* ..., arrayref, index  ==> ..., value         */

			gen_IALOAD(cd);
			break;

		case ICMD_FALOAD:     /* ..., arrayref, index  ==> ..., value         */

			gen_FALOAD(cd);
			break;

		case ICMD_LALOAD:     /* ..., arrayref, index  ==> ..., value         */
		case ICMD_DALOAD:     /* ..., arrayref, index  ==> ..., value         */

			gen_LALOAD(cd);
			break;

		case ICMD_AALOAD:     /* ..., arrayref, index  ==> ..., value         */

			gen_AALOAD(cd);
			break;


		case ICMD_BASTORE:    /* ..., arrayref, index, value  ==> ...         */

			gen_BASTORE(cd);
			break;

		case ICMD_CASTORE:    /* ..., arrayref, index, value  ==> ...         */
		case ICMD_SASTORE:    /* ..., arrayref, index, value  ==> ...         */

			gen_CASTORE(cd);
			break;

		case ICMD_IASTORE:    /* ..., arrayref, index, value  ==> ...         */

			gen_IASTORE(cd);
			break;

		case ICMD_FASTORE:    /* ..., arrayref, index, value  ==> ...         */

			gen_FASTORE(cd);
			break;

		case ICMD_LASTORE:    /* ..., arrayref, index, value  ==> ...         */
		case ICMD_DASTORE:    /* ..., arrayref, index, value  ==> ...         */

			gen_LASTORE(cd);
			break;

		case ICMD_AASTORE:    /* ..., arrayref, index, value  ==> ...         */

			gen_AASTORE(cd);
			break;


		case ICMD_GETSTATIC:  /* ...  ==> ..., value                          */
		                      /* op1 = type, val.a = field address            */

			{
			fieldinfo *fi = NULL;
			unresolved_field *uf = NULL;

			if (INSTRUCTION_IS_UNRESOLVED(iptr))
				uf = INSTRUCTION_UNRESOLVED_FIELD(iptr);
			else
				fi = INSTRUCTION_RESOLVED_FIELDINFO(iptr);

			switch (iptr->op1) {
			case TYPE_INT:
				if (fi == NULL) {
					gen_PATCHER_GETSTATIC_INT(cd, 0, uf);
				} else if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					gen_PATCHER_GETSTATIC_CLINIT_INT(cd, 0, fi);
				} else {
					gen_GETSTATIC_INT(cd, (u1 *)&(fi->value.i), fi);
				}
				break;
			case TYPE_FLT:
				if (fi == NULL) {
					gen_PATCHER_GETSTATIC_FLOAT(cd, 0, uf);
				} else if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					gen_PATCHER_GETSTATIC_CLINIT_FLOAT(cd, 0, fi);
				} else {
					gen_GETSTATIC_FLOAT(cd, (u1 *)&(fi->value.i), fi);
				}
				break;
			case TYPE_LNG:
			case TYPE_DBL:
				if (fi == NULL) {
					gen_PATCHER_GETSTATIC_LONG(cd, 0, uf);
				} else if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					gen_PATCHER_GETSTATIC_CLINIT_LONG(cd, 0, fi);
				} else {
					gen_GETSTATIC_LONG(cd, (u1 *)&(fi->value.l), fi);
				}
				break;
			case TYPE_ADR:
				if (fi == NULL) {
					gen_PATCHER_GETSTATIC_CELL(cd, 0, uf);
				} else if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					gen_PATCHER_GETSTATIC_CLINIT_CELL(cd, 0, fi);
				} else {
					gen_GETSTATIC_CELL(cd, (u1 *)&(fi->value.a), fi);
				}
				break;
			}
			}
			break;

		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */
		                      /* op1 = type, val.a = field address            */

			{
			fieldinfo *fi = NULL;
			unresolved_field *uf = NULL;

			if (INSTRUCTION_IS_UNRESOLVED(iptr))
				uf = INSTRUCTION_UNRESOLVED_FIELD(iptr);
			else
				fi = INSTRUCTION_RESOLVED_FIELDINFO(iptr);

			switch (iptr->op1) {
			case TYPE_INT:
				if (fi == NULL) {
					gen_PATCHER_PUTSTATIC_INT(cd, 0, uf);
				} else if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					gen_PATCHER_PUTSTATIC_CLINIT_INT(cd, 0, fi);
				} else {
					gen_PUTSTATIC_INT(cd, (u1 *)&(fi->value.i), fi);
				}
				break;
			case TYPE_FLT:
				if (fi == NULL) {
					gen_PATCHER_PUTSTATIC_FLOAT(cd, 0, uf);
				} else if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					gen_PATCHER_PUTSTATIC_CLINIT_FLOAT(cd, 0, fi);
				} else {
					gen_PUTSTATIC_FLOAT(cd, (u1 *)&(fi->value.i), fi);
				}
				break;
			case TYPE_LNG:
			case TYPE_DBL:
				if (fi == NULL) {
					gen_PATCHER_PUTSTATIC_LONG(cd, 0, uf);
				} else if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					gen_PATCHER_PUTSTATIC_CLINIT_LONG(cd, 0, fi);
				} else {
					gen_PUTSTATIC_LONG(cd, (u1 *)&(fi->value.l), fi);
				}
				break;
			case TYPE_ADR:
				if (fi == NULL) {
					gen_PATCHER_PUTSTATIC_CELL(cd, 0, uf);
				} else if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					gen_PATCHER_PUTSTATIC_CLINIT_CELL(cd, 0, fi);
				} else {
					gen_PUTSTATIC_CELL(cd, (u1 *)&(fi->value.a), fi);
				}
				break;
			}
			}
			break;


		case ICMD_GETFIELD:   /* ...  ==> ..., value                          */
		                      /* op1 = type, val.a = field address            */

			{
			fieldinfo *fi = NULL;
			unresolved_field *uf = NULL;

			if (INSTRUCTION_IS_UNRESOLVED(iptr))
				uf = INSTRUCTION_UNRESOLVED_FIELD(iptr);
			else
				fi = INSTRUCTION_RESOLVED_FIELDINFO(iptr);

			switch (iptr->op1) {
			case TYPE_INT:
				if (fi == NULL) {
					gen_PATCHER_GETFIELD_INT(cd, 0, uf);
				} else {
					gen_GETFIELD_INT(cd, fi->offset, fi);
				}
				break;
			case TYPE_FLT:
				if (fi == NULL) {
					gen_PATCHER_GETFIELD_FLOAT(cd, 0, uf);
				} else {
					gen_GETFIELD_FLOAT(cd, fi->offset, fi);
				}
				break;
			case TYPE_LNG:
			case TYPE_DBL:
				if (fi == NULL) {
					gen_PATCHER_GETFIELD_LONG(cd, 0, uf);
				} else {
					gen_GETFIELD_LONG(cd, fi->offset, fi);
				}
				break;
			case TYPE_ADR:
				if (fi == NULL) {
					gen_PATCHER_GETFIELD_CELL(cd, 0, uf);
				} else {
					gen_GETFIELD_CELL(cd, fi->offset, fi);
				}
				break;
			}
			}
			break;

		case ICMD_PUTFIELD:   /* ..., objectref, value  ==> ...               */
		                      /* op1 = type, val.a = field address            */

			{
			fieldinfo *fi = NULL;
			unresolved_field *uf = NULL;

			if (INSTRUCTION_IS_UNRESOLVED(iptr))
				uf = INSTRUCTION_UNRESOLVED_FIELD(iptr);
			else
				fi = INSTRUCTION_RESOLVED_FIELDINFO(iptr);

			switch (iptr->op1) {
			case TYPE_INT:
				if (fi == NULL) {
					gen_PATCHER_PUTFIELD_INT(cd, 0, uf);
				} else {
					gen_PUTFIELD_INT(cd, fi->offset, fi);
				}
				break;
			case TYPE_FLT:
				if (fi == NULL) {
					gen_PATCHER_PUTFIELD_FLOAT(cd, 0, uf);
				} else {
					gen_PUTFIELD_FLOAT(cd, fi->offset, fi);
				}
				break;
			case TYPE_LNG:
			case TYPE_DBL:
				if (fi == NULL) {
					gen_PATCHER_PUTFIELD_LONG(cd, 0, uf);
				} else {
					gen_PUTFIELD_LONG(cd, fi->offset, fi);
				}
				break;
			case TYPE_ADR:
				if (fi == NULL) {
					gen_PATCHER_PUTFIELD_CELL(cd, 0, uf);
				} else {
					gen_PUTFIELD_CELL(cd, fi->offset, fi);
				}
				break;
			}
			}
			break;


		/* branch operations **************************************************/

		case ICMD_ATHROW:       /* ..., objectref ==> ... (, objectref)       */

			gen_ATHROW(cd);
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

			gen_RET(cd, index2offset(iptr->op1));
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
				gen_ICONST(cd, iptr->val.i);
				gen_branch(IF_ICMPEQ);
			}
			break;

		case ICMD_IFLT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (iptr->val.i == 0) {
				gen_branch(IFLT);
			} else {
				gen_ICONST(cd, iptr->val.i);
				gen_branch(IF_ICMPLT);
			}
			break;

		case ICMD_IFLE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (iptr->val.i == 0) {
				gen_branch(IFLE);
			} else {
				gen_ICONST(cd, iptr->val.i);
				gen_branch(IF_ICMPLE);
			}
			break;

		case ICMD_IFNE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (iptr->val.i == 0) {
				gen_branch(IFNE);
			} else {
				gen_ICONST(cd, iptr->val.i);
				gen_branch(IF_ICMPNE);
			}
			break;

		case ICMD_IFGT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (iptr->val.i == 0) {
				gen_branch(IFGT);
			} else {
				gen_ICONST(cd, iptr->val.i);
				gen_branch(IF_ICMPGT);
			}
			break;

		case ICMD_IFGE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (iptr->val.i == 0) {
				gen_branch(IFGE);
			} else {
				gen_ICONST(cd, iptr->val.i);
				gen_branch(IF_ICMPGE);
			}
			break;

		case ICMD_IF_LEQ:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			gen_LCONST(cd, iptr->val.l);
			gen_branch(IF_LCMPEQ);
			break;

		case ICMD_IF_LLT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			gen_LCONST(cd, iptr->val.l);
			gen_branch(IF_LCMPLT);
			break;

		case ICMD_IF_LLE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			gen_LCONST(cd, iptr->val.l);
			gen_branch(IF_LCMPLE);
			break;

		case ICMD_IF_LNE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			gen_LCONST(cd, iptr->val.l);
			gen_branch(IF_LCMPNE);
			break;

		case ICMD_IF_LGT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			gen_LCONST(cd, iptr->val.l);
			gen_branch(IF_LCMPGT);
			break;

		case ICMD_IF_LGE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			gen_LCONST(cd, iptr->val.l);
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
					gen_ACONST(cd, (java_objectheader *) m->class);
				} else {
					gen_ALOAD(cd, 0);
				}
				gen_MONITOREXIT(cd);
			}
#endif
			if (opt_verbosecall)
				gen_TRACERETURN(cd, m);

			gen_IRETURN(cd, index2offset(cd->maxlocals));
			break;

		case ICMD_LRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_DRETURN:      /* ..., retvalue ==> ...                      */

#if defined(USE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				if (m->flags & ACC_STATIC) {
					gen_ACONST(cd, (java_objectheader *) m->class);
				} else {
					gen_ALOAD(cd, 0);
				}
				gen_MONITOREXIT(cd);
			}
#endif
			if (opt_verbosecall)
				gen_TRACELRETURN(cd, m);

			gen_LRETURN(cd, index2offset(cd->maxlocals));
			break;

		case ICMD_RETURN:       /* ...  ==> ...                               */

#if defined(USE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				if (m->flags & ACC_STATIC) {
					gen_ACONST(cd, (java_objectheader *) m->class);
				} else {
					gen_ALOAD(cd, 0);
				}
				gen_MONITOREXIT(cd);
			}
#endif
			if (opt_verbosecall)
				gen_TRACERETURN(cd, m);

			gen_RETURN(cd, index2offset(cd->maxlocals));
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

			/* arguments: low, range, datasegment address, table
		       offset in datasegment, default target */

			gen_TABLESWITCH(cd, l, i, NULL, 0, NULL);

			/* actually -3 cells offset */

			dseg_adddata(cd, (cd->mcodeptr - 2 * sizeof(Inst)));
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
			gen_LOOKUPSWITCH(cd, i, NULL, 0, NULL);
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
					(bg->gen)(cd);
					goto gen_builtin_end;
				}
			}
			assert(0);
		gen_builtin_end:
			break;

		case ICMD_INVOKESTATIC: /* ..., [arg1, [arg2 ...]] ==> ...            */
		                        /* op1 = arg count, val.a = method pointer    */

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				um = INSTRUCTION_UNRESOLVED_METHOD(iptr);
				md = um->methodref->parseddesc.md;
				gen_PATCHER_INVOKESTATIC(cd, 0, md->paramslots, um);

			} else {
				lm = INSTRUCTION_RESOLVED_METHODINFO(iptr);
				md = lm->parseddesc;
				gen_INVOKESTATIC(cd, (Inst **)lm->stubroutine, md->paramslots, lm);
			}
			break;

		case ICMD_INVOKESPECIAL:/* ..., objectref, [arg1, [arg2 ...]] ==> ... */

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				um = INSTRUCTION_UNRESOLVED_METHOD(iptr);
				md = um->methodref->parseddesc.md;
				gen_PATCHER_INVOKESPECIAL(cd, 0, md->paramslots, um);

			} else {
				lm = INSTRUCTION_RESOLVED_METHODINFO(iptr);
				md = lm->parseddesc;
				gen_INVOKESPECIAL(cd, (Inst **)lm->stubroutine, md->paramslots, lm);
			}
			break;

		case ICMD_INVOKEVIRTUAL:/* op1 = arg count, val.a = method pointer    */

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				um = INSTRUCTION_UNRESOLVED_METHOD(iptr);
				md = um->methodref->parseddesc.md;
				gen_PATCHER_INVOKEVIRTUAL(cd, 0, md->paramslots, um);

			} else {
				lm = INSTRUCTION_RESOLVED_METHODINFO(iptr);
				md = lm->parseddesc;

				s1 = OFFSET(vftbl_t, table[0]) +
					sizeof(methodptr) * lm->vftblindex;

				gen_INVOKEVIRTUAL(cd, s1, md->paramslots, lm);
			}
			break;

		case ICMD_INVOKEINTERFACE:/* op1 = arg count, val.a = method pointer  */

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				um = INSTRUCTION_UNRESOLVED_METHOD(iptr);
				md = um->methodref->parseddesc.md;
				gen_PATCHER_INVOKEINTERFACE(cd, 0, 0, md->paramslots, um);

			} else {
				lm = INSTRUCTION_RESOLVED_METHODINFO(iptr);
				md = lm->parseddesc;

				s1 = OFFSET(vftbl_t, interfacetable[0]) -
					sizeof(methodptr*) * lm->class->index;

				s2 = sizeof(methodptr) * (lm - lm->class->methods);

				gen_INVOKEINTERFACE(cd, s1, s2, md->paramslots, lm);
			}
			break;


		case ICMD_CHECKCAST:  /* ..., objectref ==> ..., objectref            */
		                      /* op1:   0 == array, 1 == class                */
		                      /* val.a: (classinfo *) superclass              */

			if (iptr->op1 == 1) {
				if (iptr->val.a == NULL)
					gen_PATCHER_CHECKCAST(cd, NULL, iptr->target);
				else
					gen_CHECKCAST(cd, iptr->val.a, NULL);
			} else {
				if (iptr->val.a == NULL)
					gen_PATCHER_ARRAYCHECKCAST(cd, NULL, iptr->target);
				else
					gen_ARRAYCHECKCAST(cd, iptr->val.a, NULL);
			}
			break;

		case ICMD_INSTANCEOF: /* ..., objectref ==> ..., intresult            */
		                      /* op1:   0 == array, 1 == class                */
		                      /* val.a: (classinfo *) superclass              */

			if (iptr->val.a == NULL)
				gen_PATCHER_INSTANCEOF(cd, NULL, iptr->target);
			else
				gen_INSTANCEOF(cd, iptr->val.a, iptr->target);
			break;

		case ICMD_MULTIANEWARRAY:/* ..., cnt1, [cnt2, ...] ==> ..., arrayref  */
		                      /* op1 = dimension, val.a = class               */

			if (iptr->val.a == NULL) {
				gen_PATCHER_MULTIANEWARRAY(cd, NULL, iptr->op1, iptr->target);
			} else {
				gen_MULTIANEWARRAY(cd, iptr->val.a, iptr->op1, NULL);
			}
			break;

		default:
			*exceptionptr = new_internalerror("Unknown ICMD %d", iptr->opc);
			return false;
	} /* switch */
		
	} /* for instruction */

	gen_BBEND;

	} /* if (bptr->flags >= BBREACHED) */
	} /* for basic block */

	dseg_createlinenumbertable(cd);

	codegen_finish(jd, (s4) (cd->mcodeptr - cd->mcodebase));

#ifdef VM_PROFILING
	vm_block_insert(jd->code->mcode + jd->code->mcodelength);
#endif

	/* branch resolving (walk through all basic blocks) */

	for (bptr = m->basicblocks; bptr != NULL; bptr = bptr->next) {
		branchref *brefs;

		for (brefs = bptr->branchrefs; brefs != NULL; brefs = brefs->next) {
			gen_resolveanybranch(((u1*) jd->code->entrypoint) + brefs->branchpos,
			                     ((u1 *)jd->code->entrypoint) + bptr->mpc);
		}
	}

	/* everything's ok */

	return true;
}


/* createcompilerstub **********************************************************

   Creates a stub routine which calls the compiler.

   A stub consists of:

   +-------------+
   | methodinfo* |
   +-------------+ <-- stub
   | codeptr     |
   +-------------+
   | maxlocals   |
   +-------------+
   | TRANSLATE   |
   +-------------+
   | methodinfo  |
   +-------------+

   codeptr points either to TRANSLATE or to the translated threaded code

   all methods are called indirectly through methodptr

*******************************************************************************/

#define COMPILERSTUB_DATASIZE    1
#define COMPILERSTUB_CODESIZE    4

#define COMPILERSTUB_SIZE        COMPILERSTUB_DATASIZE + COMPILERSTUB_CODESIZE


u1 *intrp_createcompilerstub(methodinfo *m)
{
	Inst        *s;
	Inst        *d;
	codegendata *cd;
	s4           dumpsize;

	s = CNEW(Inst, COMPILERSTUB_SIZE);

	/* set data pointer and code pointer */

	d = s;
	s = s + COMPILERSTUB_DATASIZE;

	/* Store the methodinfo* in the same place as in the methodheader
	   for compiled methods. */

	d[0] = (Inst *) m;

	/* mark start of dump memory area */

	dumpsize = dump_size();
	
	cd = DNEW(codegendata);
    cd->mcodeptr = (u1 *) s;
	cd->lastinstwithoutdispatch = ~0;
	cd->superstarts = NULL;

	genarg_ainst(cd, s + 2);

	if (m->flags & ACC_NATIVE) {
		genarg_i(cd, m->parseddesc->paramslots);
	} else {
		genarg_i(cd, m->maxlocals);
	}

	gen_BBSTART;
	gen_TRANSLATE(cd, m);
	gen_BBEND;
	
#ifdef VM_PROFILING
	vm_block_insert(cd->mcodeptr);
#endif

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		count_cstub_len += COMPILERSTUB_SIZE;
#endif

	/* release dump area */

	dump_release(dumpsize);
	
	return (u1 *) s;
}


#if defined(WITH_FFI)
static ffi_type *cacaotype2ffitype(s4 cacaotype)
{
	switch (cacaotype) {
	case TYPE_INT:
#if SIZEOF_VOID_P == 8
		return &ffi_type_sint64;
#else
		return &ffi_type_sint32;
#endif
	case TYPE_LNG:
		return &ffi_type_sint64;
	case TYPE_FLT:
		return &ffi_type_float;
	case TYPE_DBL:
		return &ffi_type_double;
	case TYPE_ADR:
		return &ffi_type_pointer;
	case TYPE_VOID:
		return &ffi_type_void;
	default:
		assert(false);
	}
}
#endif


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

#if defined(WITH_FFI)
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
#endif


u1 *intrp_createnativestub(functionptr f, jitdata *jd, methoddesc *nmd)
{
	methodinfo   *m;
	codegendata  *cd;
	registerdata *rd;
#if defined(WITH_FFI)
	ffi_cif *cif;
#else
	u1      *cif;
#endif

	/* get required compiler data */

	m  = jd->m;
	cd = jd->cd;
	rd = jd->rd;

	/* create method header */

	(void) dseg_addaddress(cd, m);                          /* MethodPointer  */
	(void) dseg_adds4(cd, nmd->paramslots * SIZEOF_VOID_P); /* FrameSize      */
	(void) dseg_adds4(cd, 0);                               /* IsSync         */
	(void) dseg_adds4(cd, 0);                               /* IsLeaf         */
	(void) dseg_adds4(cd, 0);                               /* IntSave        */
	(void) dseg_adds4(cd, 0);                               /* FltSave        */
	dseg_addlinenumbertablesize(cd);
	(void) dseg_adds4(cd, 0);                               /* ExTableSize    */

#if defined(WITH_FFI)
	/* prepare ffi cif structure */

	cif = createnativecif(m, nmd);
#else
	cif = NULL;
#endif

	gen_BBSTART;

	if (opt_verbosecall)
		gen_TRACECALL(cd, m);

	if (f == NULL) {
		gen_PATCHER_NATIVECALL(cd, m, f, (u1 *)cif);
	} else {
		if (opt_verbosecall)
			gen_TRACENATIVECALL(cd, m, f, (u1 *)cif);
		else
			gen_NATIVECALL(cd, m, f, (u1 *)cif);
	}

	gen_BBEND;

	codegen_finish(jd, (s4) (cd->mcodeptr - cd->mcodebase));

#ifdef VM_PROFILING
	vm_block_insert(jd->code->mcode + jd->code->mcodelength);
#endif

	return jd->code->entrypoint;
}


/* call jni function */
Cell *nativecall(functionptr f, methodinfo *m, Cell *sp, Inst *ra, Cell *fp, u1 *addrcif)
{
#if defined(WITH_FFCALL)
	av_alist alist;
	methoddesc *md;
	Cell *p;
	Cell *endsp;
	s4 i;

	struct {
		stackframeinfo sfi;
		localref_table lrt;
	} s;

	md = m->parseddesc;

	switch (md->returntype.type) {
	case TYPE_INT:
		endsp = sp - 1 + md->paramslots;
		av_start_long(alist, f, endsp);
		break;
	case TYPE_LNG:
		endsp = sp - 2 + md->paramslots;
		av_start_longlong(alist, f, endsp);
		break;
	case TYPE_FLT:
		endsp = sp - 1 + md->paramslots;
		av_start_float(alist, f, endsp);
		break;
	case TYPE_DBL:
		endsp = sp - 2 + md->paramslots;
		av_start_double(alist, f, endsp);
		break;
	case TYPE_ADR:
		endsp = sp - 1 + md->paramslots;
		av_start_ptr(alist, f, void *, endsp);
		break;
	case TYPE_VOID:
		endsp = sp + md->paramslots;
		av_start_void(alist, f);
		break;
	default:
		assert(false);
	}

	av_ptr(alist, _Jv_JNIEnv *, _Jv_env);

	if (m->flags & ACC_STATIC)
		av_ptr(alist, classinfo *, m->class);

	for (i = 0, p = sp + md->paramslots; i < md->paramcount; i++) {
		switch (md->paramtypes[i].type) {
		case TYPE_INT:
			p -= 1;
			av_long(alist, *p);
			break;
		case TYPE_LNG:
			p -= 2;
			av_longlong(alist, *(s8 *)p);
			break;
		case TYPE_FLT:
			p -= 1;
			av_float(alist, *((float *) p));
			break;
		case TYPE_DBL:
			p -= 2;
			av_double(alist, *(double *) p);
			break;
		case TYPE_ADR:
			p -= 1;
			av_ptr(alist, void *, *(void **) p);
			break;
		default:
			assert(false);
		}
	}

	global_sp = sp;

	/* create stackframe info structure */

	codegen_start_native_call(((u1 *) &s) + sizeof(s), m->code->entrypoint,
							  (u1 *) fp, (u1 *) ra);

	av_call(alist);

	codegen_finish_native_call(((u1 *) &s) + sizeof(s));

	CLEAR_global_sp;

	return endsp;
#elif defined(WITH_FFI)
	methoddesc  *md = m->parseddesc; 
	ffi_cif     *pcif;
	void        *values[md->paramcount + 2];
	void       **pvalues = values;
	Cell        *p;
	Cell        *endsp;
	s4           i;
	_Jv_JNIEnv  *penv;

	struct {
		stackframeinfo sfi;
		localref_table lrt;
	} s;

	pcif = (ffi_cif *) addrcif;

	/* pass env pointer */

	penv = (_Jv_JNIEnv *) _Jv_env;
	*pvalues++ = &penv;

	/* for static methods, pass class pointer */

	if (m->flags & ACC_STATIC)
		*pvalues++ = &m->class;

	/* pass parameter to native function */

	for (i = 0, p = sp + md->paramslots; i < md->paramcount; i++) {
		if (IS_2_WORD_TYPE(md->paramtypes[i].type))
			p -= 2;
		else
			p--;

		*pvalues++ = p;
	}

	/* calculate position of return value */

	if (md->returntype.type == TYPE_VOID)
		endsp = sp + md->paramslots;
	else
		endsp = sp - (IS_2_WORD_TYPE(md->returntype.type) ? 2 : 1) + md->paramslots;

	global_sp = sp;

	/* create stackframe info structure */

	codegen_start_native_call(((u1 *) &s) + sizeof(s), m->code->entrypoint,
							  (u1 *) fp, (u1 *) ra);

	ffi_call(pcif, FFI_FN(f), endsp, values);

	codegen_finish_native_call(((u1 *) &s) + sizeof(s));

	CLEAR_global_sp;

	return endsp;
#endif
}


u1 *createcalljavafunction(methodinfo *m)
{
	methodinfo         *tmpm;
	u1                 *entrypoint;
	jitdata            *jd;
	codegendata        *cd;
	registerdata       *rd;
	s4                  dumpsize;
	methoddesc         *md;

	/* mark dump memory */

	dumpsize = dump_size();

	/* allocate memory */

	jd = DNEW(jitdata);

	tmpm = DNEW(methodinfo);
	cd = DNEW(codegendata);
	rd = DNEW(registerdata);

	jd->m = tmpm;
	jd->flags = 0;
	jd->cd = cd;
	jd->rd = rd;

	/* Allocate codeinfo memory from the heap as we need to keep them. */

	jd->code = code_codeinfo_new(tmpm); /* XXX check allocation */

	/* setup code generation stuff */

	MSET(tmpm, 0, u1, sizeof(methodinfo));

	codegen_setup(jd);

	md = m->parseddesc;

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
	gen_INVOKESTATIC(cd, (Inst **)m->stubroutine, md->paramslots, 0);
	gen_END(cd);

	gen_BBEND;

	codegen_finish(jd, (s4) (cd->mcodeptr - cd->mcodebase));

#ifdef VM_PROFILING
	vm_block_insert(jd->code->mcode + jd->code->mcodelength);
#endif
	entrypoint = jd->code->entrypoint;

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
 * vim:noexpandtab:sw=4:ts=4:
 */
