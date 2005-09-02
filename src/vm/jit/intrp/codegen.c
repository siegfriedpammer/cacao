/* src/vm/jit/alpha/codegen.c - machine code generator for Alpha

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

   Changes: Joseph Wenninger
            Christian Thalinger
            Christian Ullrich

   $Id: codegen.c 3140 2005-09-02 15:17:20Z twisti $

*/


#include <stdio.h>

#include "config.h"

#include "machine-instr.h"

#include "vm/jit/intrp/arch.h"
#include "vm/jit/intrp/codegen.h"
#include "vm/jit/intrp/types.h"

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

#define gen_branch(_inst) { \
  gen_##_inst(&mcodeptr, 0); \
  codegen_addreference(cd, (basicblock *) (iptr->target), mcodeptr); \
}


/* functions used by cacao-gen.i */

void
genarg_v(Inst ** ptr2current_threaded, Cell v)
{
  *((Cell *) *ptr2current_threaded) = v;
  (*ptr2current_threaded)++;
}

void
genarg_i(Inst ** ptr2cur_threaded, s4 i)
{
  *((Cell *) *ptr2cur_threaded) = i;
  (*ptr2cur_threaded)++;
}

void
genarg_b(Inst ** ptr2cur_threaded, s4 i)
{
  genarg_i(ptr2cur_threaded, i);
}

void
genarg_f(Inst ** ptr2cur_threaded, float f)
{
	s4 fi;

	vm_f2Cell(f,fi);
	genarg_i(ptr2cur_threaded, fi);
}

void
genarg_l(Inst ** ptr2cur_threaded, s8 l)
{
  vm_l2twoCell(l, ((Cell*)*ptr2cur_threaded)[1], ((Cell*)*ptr2cur_threaded)[0]);
  (*ptr2cur_threaded) +=2;
}

void
genarg_aRef(Inst ** ptr2current_threaded, java_objectheader *a)
{
  *((java_objectheader **) *ptr2current_threaded) = a;
  (*ptr2current_threaded)++;
}

void
genarg_aArray(Inst ** ptr2current_threaded, java_arrayheader *a)
{
  *((java_arrayheader **) *ptr2current_threaded) = a;
  (*ptr2current_threaded)++;
}

void
genarg_aaTarget(Inst ** ptr2current_threaded, Inst **a)
{
  *((Inst ***) *ptr2current_threaded) = a;
  (*ptr2current_threaded)++;
}

void
genarg_aClass(Inst ** ptr2current_threaded, classinfo *a)
{
  *((classinfo **) *ptr2current_threaded) = a;
  (*ptr2current_threaded)++;
}

void
genarg_acr(Inst ** ptr2current_threaded, constant_classref *a)
{
  *((constant_classref **) *ptr2current_threaded) = a;
  (*ptr2current_threaded)++;
}

void
genarg_addr(Inst ** ptr2current_threaded, u1 *a)
{
  *((u1 **) *ptr2current_threaded) = a;
  (*ptr2current_threaded)++;
}

void
genarg_af(Inst ** ptr2current_threaded, functionptr a)
{
  *((functionptr *) *ptr2current_threaded) = a;
  (*ptr2current_threaded)++;
}

void
genarg_am(Inst ** ptr2current_threaded, methodinfo *a)
{
  *((methodinfo **) *ptr2current_threaded) = a;
  (*ptr2current_threaded)++;
}

void
genarg_acell(Inst ** ptr2current_threaded, Cell *a)
{
  *((Cell **) *ptr2current_threaded) = a;
  (*ptr2current_threaded)++;
}

void
genarg_ainst(Inst ** ptr2current_threaded, Inst *a)
{
  *((Inst **) *ptr2current_threaded) = a;
  (*ptr2current_threaded)++;
}

void
genarg_auf(Inst ** ptr2current_threaded, unresolved_field *a)
{
  *((unresolved_field **) *ptr2current_threaded) = a;
  (*ptr2current_threaded)++;
}

void
genarg_aum(Inst ** ptr2current_threaded, unresolved_method *a)
{
  *((unresolved_method **) *ptr2current_threaded) = a;
  (*ptr2current_threaded)++;
}

void
genarg_avftbl(Inst ** ptr2current_threaded, vftbl_t *a)
{
  *((vftbl_t **) *ptr2current_threaded) = a;
  (*ptr2current_threaded)++;
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
};

/*
  The following ones cannot use the BUILTIN mechanism, because they
  need the class as immediate arguments of the patcher

	PATCHER_builtin_new,		     gen_PATCHER_NEW,	 
	PATCHER_builtin_newarray,	     gen_PATCHER_NEWARRAY,	 
	PATCHER_builtin_arrayinstanceof, gen_PATCHER_ARRAYINSTANCEOF,
*/




/* codegen *********************************************************************

   Generates machine code.

*******************************************************************************/

void codegen(methodinfo *m, codegendata *cd, registerdata *rd)
{
	s4                  i, len, s1, s2, d;
	Inst               *mcodeptr;
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

#if defined(USE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		(void) dseg_adds4(cd, 1);                           /* IsSync         */
	else
#endif
		(void) dseg_adds4(cd, 0);                           /* IsSync         */
	                                       
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
	
	mcodeptr = (s4 *) cd->mcodebase;
	cd->mcodeend = (s4 *) (cd->mcodebase + cd->mcodesize);

	if (runverbose)
		gen_TRACECALL(&mcodeptr);

	/* walk through all basic blocks */

	for (bptr = m->basicblocks; bptr != NULL; bptr = bptr->next) {

		bptr->mpc = (s4) ((u1 *) mcodeptr - cd->mcodebase);

		if (bptr->flags >= BBREACHED) {

		/* walk through all instructions */
		
		src = bptr->instack;
		len = bptr->icount;

		for (iptr = bptr->iinstr; len > 0; src = iptr->dst, len--, iptr++) {
			if (iptr->line != currentline) {
				dseg_addlinenumber(cd, iptr->line, (u1 *) mcodeptr);
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

			gen_CHECKNULL(&mcodeptr);
			break;

		/* constant operations ************************************************/

		case ICMD_ICONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.i = constant                    */

			gen_ICONST(&mcodeptr, iptr->val.i);
			break;

		case ICMD_LCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.l = constant                    */

			gen_LCONST(&mcodeptr, iptr->val.l);
			break;

		case ICMD_FCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.f = constant                    */
			{
				s4 fi;

				vm_f2Cell(iptr->val.f, fi);
				gen_ICONST(&mcodeptr, fi);
			}
			break;
			
		case ICMD_DCONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.d = constant                    */

			gen_LCONST(&mcodeptr, *(s8 *)&(iptr->val.d));
			break;

		case ICMD_ACONST:     /* ...  ==> ..., constant                       */
		                      /* op1 = 0, val.a = constant                    */

			gen_ACONST(&mcodeptr, iptr->val.a);
			break;


		/* load/store operations **********************************************/

		case ICMD_ILOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			gen_ILOAD(&mcodeptr, iptr->op1);
			break;

		case ICMD_LLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			gen_LLOAD(&mcodeptr, iptr->op1);
			break;

		case ICMD_ALOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			gen_ALOAD(&mcodeptr, iptr->op1);
			break;

		case ICMD_FLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			gen_ILOAD(&mcodeptr, iptr->op1);
			break;

		case ICMD_DLOAD:      /* ...  ==> ..., content of local variable      */
		                      /* op1 = local variable                         */

			gen_LLOAD(&mcodeptr, iptr->op1);
			break;


		case ICMD_ISTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			gen_ISTORE(&mcodeptr, iptr->op1);
			break;

		case ICMD_LSTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			gen_LSTORE(&mcodeptr, iptr->op1);
			break;

		case ICMD_ASTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			gen_ASTORE(&mcodeptr, iptr->op1);
			break;


		case ICMD_FSTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			gen_ISTORE(&mcodeptr, iptr->op1);
			break;

		case ICMD_DSTORE:     /* ..., value  ==> ...                          */
		                      /* op1 = local variable                         */

			gen_LSTORE(&mcodeptr, iptr->op1);
			break;


		/* pop/dup/swap operations ********************************************/

		/* attention: double and longs are only one entry in CACAO ICMDs      */

		case ICMD_POP:        /* ..., value  ==> ...                          */

			gen_POP(&mcodeptr);
			break;

		case ICMD_POP2:       /* ..., value, value  ==> ...                   */

			gen_POP2(&mcodeptr);
			break;

		case ICMD_DUP:        /* ..., a ==> ..., a, a                         */

			gen_DUP(&mcodeptr);
			break;

		case ICMD_DUP_X1:     /* ..., a, b ==> ..., b, a, b                   */

			gen_DUP_X1(&mcodeptr);
			break;

		case ICMD_DUP_X2:     /* ..., a, b, c ==> ..., c, a, b, c             */

			gen_DUP_X1(&mcodeptr);
			break;

		case ICMD_DUP2:       /* ..., a, b ==> ..., a, b, a, b                */

			gen_DUP2(&mcodeptr);
			break;

		case ICMD_DUP2_X1:    /* ..., a, b, c ==> ..., b, c, a, b, c          */

			gen_DUP2_X1(&mcodeptr);
			break;

		case ICMD_DUP2_X2:    /* ..., a, b, c, d ==> ..., c, d, a, b, c, d    */

			gen_DUP2_X2(&mcodeptr);
			break;

		case ICMD_SWAP:       /* ..., a, b ==> ..., b, a                      */

			gen_SWAP(&mcodeptr);
			break;


		/* integer operations *************************************************/

		case ICMD_INEG:       /* ..., value  ==> ..., - value                 */

			gen_INEG(&mcodeptr);
			break;

		case ICMD_LNEG:       /* ..., value  ==> ..., - value                 */

			gen_LNEG(&mcodeptr);
			break;

		case ICMD_I2L:        /* ..., value  ==> ..., value                   */

			gen_I2L(&mcodeptr);
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */

			gen_L2I(&mcodeptr);
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			gen_INT2BYTE(&mcodeptr);
			break;

		case ICMD_INT2CHAR:   /* ..., value  ==> ..., value                   */

			gen_INT2CHAR(&mcodeptr);
			break;

		case ICMD_INT2SHORT:  /* ..., value  ==> ..., value                   */

			gen_INT2SHORT(&mcodeptr);
			break;


		case ICMD_IADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			gen_IADD(&mcodeptr);
			break;

		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			gen_ICONST(&mcodeptr, iptr->val.i);
			gen_IADD(&mcodeptr);
			break;

		case ICMD_LADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			gen_LADD(&mcodeptr);
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.l = constant                             */

			gen_LCONST(&mcodeptr, iptr->val.l);
			gen_LADD(&mcodeptr);
			break;

		case ICMD_ISUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			gen_ISUB(&mcodeptr);
			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* val.i = constant                             */

			gen_ICONST(&mcodeptr, iptr->val.i);
			gen_ISUB(&mcodeptr);
			break;

		case ICMD_LSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			gen_LSUB(&mcodeptr);
			break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* val.l = constant                             */

			gen_LCONST(&mcodeptr, iptr->val.l);
			gen_LSUB(&mcodeptr);
			break;

		case ICMD_IMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			gen_IMUL(&mcodeptr);
			break;

		case ICMD_IMULCONST:  /* ..., val1, val2  ==> ..., val1 * val2        */

			gen_ICONST(&mcodeptr, iptr->val.i);
			gen_IMUL(&mcodeptr);
			break;

		case ICMD_LMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			gen_LMUL(&mcodeptr);
			break;

		case ICMD_LMULCONST:  /* ..., val1, val2  ==> ..., val1 * val2        */

			gen_LCONST(&mcodeptr, iptr->val.l);
			gen_LMUL(&mcodeptr);
			break;

		case ICMD_IDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			gen_IDIV(&mcodeptr);
			break;

		case ICMD_IREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			gen_IREM(&mcodeptr);
			break;

		case ICMD_LDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			gen_LDIV(&mcodeptr);
			break;

		case ICMD_LREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			gen_LREM(&mcodeptr);
			break;

		case ICMD_IDIVPOW2:   /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */
		                      
			gen_IDIVPOW2(&mcodeptr, iptr->val.i);
			break;

		case ICMD_IREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.i = constant                             */

			gen_IREMPOW2(&mcodeptr, iptr->val.i);
			break;

		case ICMD_LDIVPOW2:   /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */
		                      
			gen_LDIVPOW2(&mcodeptr, iptr->val.i);
			break;

		case ICMD_LREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* val.l = constant                             */

			gen_LREMPOW2(&mcodeptr, iptr->val.i);
			break;

		case ICMD_ISHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			gen_ISHL(&mcodeptr);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			gen_ICONST(&mcodeptr, iptr->val.i);
			gen_ISHL(&mcodeptr);
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			gen_ISHR(&mcodeptr);
			break;

		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			gen_ICONST(&mcodeptr, iptr->val.i);
			gen_ISHR(&mcodeptr);
			break;

		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			gen_IUSHR(&mcodeptr);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			gen_ICONST(&mcodeptr, iptr->val.i);
			gen_IUSHR(&mcodeptr);
			break;

		case ICMD_LSHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			gen_LSHL(&mcodeptr);
			break;

		case ICMD_LSHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* val.i = constant                             */

			gen_ICONST(&mcodeptr, iptr->val.i);
			gen_LSHL(&mcodeptr);
			break;

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			gen_LSHR(&mcodeptr);
			break;

		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* val.i = constant                             */

			gen_ICONST(&mcodeptr, iptr->val.i);
			gen_LSHR(&mcodeptr);
			break;

		case ICMD_LUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			gen_LUSHR(&mcodeptr);
			break;

		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* val.i = constant                             */

			gen_ICONST(&mcodeptr, iptr->val.i);
			gen_LUSHR(&mcodeptr);
			break;

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			gen_IAND(&mcodeptr);
			break;

		case ICMD_IANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.i = constant                             */

			gen_ICONST(&mcodeptr, iptr->val.i);
			gen_IAND(&mcodeptr);
			break;

		case ICMD_LAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			gen_LAND(&mcodeptr);
			break;

		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* val.l = constant                             */

			gen_LCONST(&mcodeptr, iptr->val.l);
			gen_LAND(&mcodeptr);
			break;

		case ICMD_IOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			gen_IOR(&mcodeptr);
			break;

		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.i = constant                             */

			gen_ICONST(&mcodeptr, iptr->val.i);
			gen_IOR(&mcodeptr);
			break;

		case ICMD_LOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			gen_LOR(&mcodeptr);
			break;

		case ICMD_LORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* val.l = constant                             */

			gen_LCONST(&mcodeptr, iptr->val.l);
			gen_LOR(&mcodeptr);
			break;

		case ICMD_IXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			gen_IXOR(&mcodeptr);
			break;

		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.i = constant                             */

			gen_ICONST(&mcodeptr, iptr->val.i);
			gen_IXOR(&mcodeptr);
			break;

		case ICMD_LXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			gen_LXOR(&mcodeptr);
			break;

		case ICMD_LXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* val.l = constant                             */

			gen_LCONST(&mcodeptr, iptr->val.l);
			gen_LXOR(&mcodeptr);
			break;


		case ICMD_LCMP:       /* ..., val1, val2  ==> ..., val1 cmp val2      */

			gen_LCMP(&mcodeptr);
			break;


		case ICMD_IINC:       /* ..., value  ==> ..., value + constant        */
		                      /* op1 = variable, val.i = constant             */

			gen_IINC(&mcodeptr, iptr->op1, iptr->val.i);
			break;


		/* floating operations ************************************************/

		case ICMD_FNEG:       /* ..., value  ==> ..., - value                 */

			gen_FNEG(&mcodeptr);
			break;

		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */

			gen_DNEG(&mcodeptr);
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			gen_FADD(&mcodeptr);
			break;

		case ICMD_DADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			gen_DADD(&mcodeptr);
			break;

		case ICMD_FSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			gen_FSUB(&mcodeptr);
			break;

		case ICMD_DSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			gen_DSUB(&mcodeptr);
			break;

		case ICMD_FMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			gen_FMUL(&mcodeptr);
			break;

		case ICMD_DMUL:       /* ..., val1, val2  ==> ..., val1 *** val2      */

			gen_DMUL(&mcodeptr);
			break;

		case ICMD_FDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			gen_FDIV(&mcodeptr);
			break;

		case ICMD_DDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			gen_DDIV(&mcodeptr);
			break;
		
		case ICMD_I2F:       /* ..., value  ==> ..., (float) value            */

			gen_I2F(&mcodeptr);
			break;

		case ICMD_L2F:       /* ..., value  ==> ..., (float) value            */

			gen_L2F(&mcodeptr);
			break;

		case ICMD_I2D:       /* ..., value  ==> ..., (double) value           */

			gen_I2D(&mcodeptr);
			break;
			
		case ICMD_L2D:       /* ..., value  ==> ..., (double) value           */

			gen_L2D(&mcodeptr);
			break;
			
		case ICMD_F2I:       /* ..., value  ==> ..., (int) value              */

			gen_F2I(&mcodeptr);
			break;
		
		case ICMD_D2I:       /* ..., value  ==> ..., (int) value              */

			gen_D2I(&mcodeptr);
			break;
		
		case ICMD_F2L:       /* ..., value  ==> ..., (long) value             */

			gen_F2L(&mcodeptr);
			break;

		case ICMD_D2L:       /* ..., value  ==> ..., (long) value             */

			gen_D2L(&mcodeptr);
			break;

		case ICMD_F2D:       /* ..., value  ==> ..., (double) value           */

			gen_F2D(&mcodeptr);
			break;
					
		case ICMD_D2F:       /* ..., value  ==> ..., (float) value            */

			gen_D2F(&mcodeptr);
			break;
		
		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */

			gen_FCMPL(&mcodeptr);
			break;
			
		case ICMD_DCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */

			gen_DCMPL(&mcodeptr);
			break;
			
		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */

			gen_FCMPG(&mcodeptr);
			break;

		case ICMD_DCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */

			gen_DCMPG(&mcodeptr);
			break;


		/* memory operations **************************************************/

		case ICMD_ARRAYLENGTH: /* ..., arrayref  ==> ..., length              */

			gen_ARRAYLENGTH(&mcodeptr);
			break;

		case ICMD_BALOAD:     /* ..., arrayref, index  ==> ..., value         */

			gen_BALOAD(&mcodeptr);
			break;

		case ICMD_CALOAD:     /* ..., arrayref, index  ==> ..., value         */

			gen_CALOAD(&mcodeptr);
			break;			

		case ICMD_SALOAD:     /* ..., arrayref, index  ==> ..., value         */

			gen_SALOAD(&mcodeptr);
			break;

		case ICMD_IALOAD:     /* ..., arrayref, index  ==> ..., value         */
		case ICMD_FALOAD:     /* ..., arrayref, index  ==> ..., value         */

			gen_IALOAD(&mcodeptr);
			break;

		case ICMD_LALOAD:     /* ..., arrayref, index  ==> ..., value         */
		case ICMD_DALOAD:     /* ..., arrayref, index  ==> ..., value         */

			gen_LALOAD(&mcodeptr);
			break;

		case ICMD_AALOAD:     /* ..., arrayref, index  ==> ..., value         */

			gen_AALOAD(&mcodeptr);
			break;


		case ICMD_BASTORE:    /* ..., arrayref, index, value  ==> ...         */

			gen_BASTORE(&mcodeptr);
			break;

		case ICMD_CASTORE:    /* ..., arrayref, index, value  ==> ...         */
		case ICMD_SASTORE:    /* ..., arrayref, index, value  ==> ...         */

			gen_CASTORE(&mcodeptr);
			break;

		case ICMD_IASTORE:    /* ..., arrayref, index, value  ==> ...         */
		case ICMD_FASTORE:    /* ..., arrayref, index, value  ==> ...         */

			gen_IASTORE(&mcodeptr);
			break;

		case ICMD_LASTORE:    /* ..., arrayref, index, value  ==> ...         */
		case ICMD_DASTORE:    /* ..., arrayref, index, value  ==> ...         */

			gen_LASTORE(&mcodeptr);
			break;

		case ICMD_AASTORE:    /* ..., arrayref, index, value  ==> ...         */

			gen_AASTORE(&mcodeptr);
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
					gen_PATCHER_GETSTATIC_INT(&mcodeptr, 0, uf);
				} else {
					gen_GETSTATIC_INT(&mcodeptr, (u1 *)&(fi->value.i), uf);
				}
				break;
			case TYPE_LNG:
			case TYPE_DBL:
				if (fi == NULL || !fi->class->initialized) {
					gen_PATCHER_GETSTATIC_LONG(&mcodeptr, 0, uf);
				} else {
					gen_GETSTATIC_LONG(&mcodeptr, (u1 *)&(fi->value.l), uf);
				}
				break;
			case TYPE_ADR:
				if (fi == NULL || !fi->class->initialized) {
					gen_PATCHER_GETSTATIC_CELL(&mcodeptr, 0, uf);
				} else {
					gen_GETSTATIC_CELL(&mcodeptr, (u1 *)&(fi->value.a), uf);
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
					gen_PATCHER_PUTSTATIC_INT(&mcodeptr, 0, uf);
				} else {
					gen_PUTSTATIC_INT(&mcodeptr, (u1 *)&(fi->value.i), uf);
				}
				break;
			case TYPE_LNG:
			case TYPE_DBL:
				if (fi == NULL || !fi->class->initialized) {
					gen_PATCHER_PUTSTATIC_LONG(&mcodeptr, 0, uf);
				} else {
					gen_PUTSTATIC_LONG(&mcodeptr, (u1 *)&(fi->value.l), uf);
				}
				break;
			case TYPE_ADR:
				if (fi == NULL || !fi->class->initialized) {
					gen_PATCHER_PUTSTATIC_CELL(&mcodeptr, 0, uf);
				} else {
					gen_PUTSTATIC_CELL(&mcodeptr, (u1 *)&(fi->value.a), uf);
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
				gen_ICONST(&mcodeptr, iptr->val.i);
				if (fi == NULL || !fi->class->initialized) {
					gen_PATCHER_PUTSTATIC_INT(&mcodeptr, 0, uf);
				} else {
					gen_PUTSTATIC_INT(&mcodeptr, (u1 *)&(fi->value.i), uf);
				}
				break;
			case TYPE_LNG:
			case TYPE_DBL:
				gen_LCONST(&mcodeptr, iptr->val.l);
				if (fi == NULL || !fi->class->initialized) {
					gen_PATCHER_PUTSTATIC_LONG(&mcodeptr, 0, uf);
				} else {
					gen_PUTSTATIC_LONG(&mcodeptr, (u1 *)&(fi->value.l), uf);
				}
				break;
			case TYPE_ADR:
				gen_ACONST(&mcodeptr, iptr->val.a);
				if (fi == NULL || !fi->class->initialized) {
					gen_PATCHER_PUTSTATIC_CELL(&mcodeptr, 0, uf);
				} else {
					gen_PUTSTATIC_CELL(&mcodeptr, (u1 *)&(fi->value.a), uf);
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
					gen_PATCHER_GETFIELD_INT(&mcodeptr, 0, uf);
				} else {
					gen_GETFIELD_INT(&mcodeptr, fi->offset, uf);
				}
				break;
			case TYPE_LNG:
			case TYPE_DBL:
				if (fi == NULL) {
					gen_PATCHER_GETFIELD_LONG(&mcodeptr, 0, uf);
				} else {
					gen_GETFIELD_LONG(&mcodeptr, fi->offset, uf);
				}
				break;
			case TYPE_ADR:
				if (fi == NULL) {
					gen_PATCHER_GETFIELD_CELL(&mcodeptr, 0, uf);
				} else {
					gen_GETFIELD_CELL(&mcodeptr, fi->offset, uf);
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
					gen_PATCHER_PUTFIELD_INT(&mcodeptr, 0, uf);
				} else {
					gen_PUTFIELD_INT(&mcodeptr, fi->offset, uf);
				}
				break;
			case TYPE_LNG:
			case TYPE_DBL:
				if (fi == NULL) {
					gen_PATCHER_PUTFIELD_LONG(&mcodeptr, 0, uf);
				} else {
					gen_PUTFIELD_LONG(&mcodeptr, fi->offset, uf);
				}
				break;
			case TYPE_ADR:
				if (fi == NULL) {
					gen_PATCHER_PUTFIELD_CELL(&mcodeptr, 0, uf);
				} else {
					gen_PUTFIELD_CELL(&mcodeptr, fi->offset, uf);
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
				gen_ICONST(&mcodeptr, iptr->val.i);
				if (fi == NULL) {
					gen_PATCHER_PUTFIELD_INT(&mcodeptr, 0, uf);
				} else {
					gen_PUTFIELD_INT(&mcodeptr, fi->offset, uf);
				}
				break;
			case TYPE_LNG:
			case TYPE_DBL:
				gen_LCONST(&mcodeptr, iptr->val.l);
				if (fi == NULL) {
					gen_PATCHER_PUTFIELD_LONG(&mcodeptr, 0, uf);
				} else {
					gen_PUTFIELD_LONG(&mcodeptr, fi->offset, uf);
				}
				break;
			case TYPE_ADR:
				gen_ACONST(&mcodeptr, iptr->val.a);
				if (fi == NULL) {
					gen_PATCHER_PUTFIELD_CELL(&mcodeptr, 0, uf);
				} else {
					gen_PUTFIELD_CELL(&mcodeptr, fi->offset, uf);
				}
				break;
			}
			}
			break;


		/* branch operations **************************************************/

		case ICMD_ATHROW:       /* ..., objectref ==> ... (, objectref)       */

			gen_ATHROW(&mcodeptr);
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

			gen_RET(&mcodeptr, iptr->op1);
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
				gen_ICONST(&mcodeptr, iptr->val.i);
				gen_branch(IF_ICMPEQ);
			}
			break;

		case ICMD_IFLT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (iptr->val.i == 0) {
				gen_branch(IFLT);
			} else {
				gen_ICONST(&mcodeptr, iptr->val.i);
				gen_branch(IF_ICMPLT);
			}
			break;

		case ICMD_IFLE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (iptr->val.i == 0) {
				gen_branch(IFLE);
			} else {
				gen_ICONST(&mcodeptr, iptr->val.i);
				gen_branch(IF_ICMPLE);
			}
			break;

		case ICMD_IFNE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (iptr->val.i == 0) {
				gen_branch(IFNE);
			} else {
				gen_ICONST(&mcodeptr, iptr->val.i);
				gen_branch(IF_ICMPNE);
			}
			break;

		case ICMD_IFGT:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (iptr->val.i == 0) {
				gen_branch(IFGT);
			} else {
				gen_ICONST(&mcodeptr, iptr->val.i);
				gen_branch(IF_ICMPGT);
			}
			break;

		case ICMD_IFGE:         /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.i = constant   */

			if (iptr->val.i == 0) {
				gen_branch(IFGE);
			} else {
				gen_ICONST(&mcodeptr, iptr->val.i);
				gen_branch(IF_ICMPGE);
			}
			break;

		case ICMD_IF_LEQ:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			gen_LCONST(&mcodeptr, iptr->val.l);
			gen_branch(IF_LCMPEQ);
			break;

		case ICMD_IF_LLT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			gen_LCONST(&mcodeptr, iptr->val.l);
			gen_branch(IF_LCMPLT);
			break;

		case ICMD_IF_LLE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			gen_LCONST(&mcodeptr, iptr->val.l);
			gen_branch(IF_LCMPLE);
			break;

		case ICMD_IF_LNE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			gen_LCONST(&mcodeptr, iptr->val.l);
			gen_branch(IF_LCMPNE);
			break;

		case ICMD_IF_LGT:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			gen_LCONST(&mcodeptr, iptr->val.l);
			gen_branch(IF_LCMPGT);
			break;

		case ICMD_IF_LGE:       /* ..., value ==> ...                         */
		                        /* op1 = target JavaVM pc, val.l = constant   */

			gen_LCONST(&mcodeptr, iptr->val.l);
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
					gen_ACONST(&mcodeptr, m->class);
				} else {
					gen_ALOAD(&mcodeptr, 0);
				}
				gen_MONITOREXIT(&mcodeptr);
			}
#endif
			if (runverbose)
				gen_TRACERETURN(&mcodeptr);

			gen_IRETURN(&mcodeptr, cd->maxlocals);
			break;

		case ICMD_LRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_DRETURN:      /* ..., retvalue ==> ...                      */

#if defined(USE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				if (m->flags & ACC_STATIC) {
					gen_ACONST(&mcodeptr, m->class);
				} else {
					gen_ALOAD(&mcodeptr, 0);
				}
				gen_MONITOREXIT(&mcodeptr);
			}
#endif
			if (runverbose)
				gen_TRACELRETURN(&mcodeptr);

			gen_LRETURN(&mcodeptr, cd->maxlocals);
			break;

		case ICMD_RETURN:       /* ...  ==> ...                               */

#if defined(USE_THREADS)
			if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
				if (m->flags & ACC_STATIC) {
					gen_ACONST(&mcodeptr, m->class);
				} else {
					gen_ALOAD(&mcodeptr, 0);
				}
				gen_MONITOREXIT(&mcodeptr);
			}
#endif
			if (runverbose)
				gen_TRACERETURN(&mcodeptr);

			gen_RETURN(&mcodeptr, cd->maxlocals);
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
			gen_TABLESWITCH(&mcodeptr, l, i, NULL, 0, NULL);
			dseg_adddata(cd, (u1 *) (mcodeptr - 2)); /* actually mcodeptr[-3] */
			codegen_addreference(cd, (basicblock *) tptr[0], mcodeptr);

			/* build jump table top down and use address of lowest entry */

			tptr += i;

			while (--i >= 0) {
				dseg_addtarget(cd, (basicblock *) tptr[0]); 
				--tptr;
			}
			}

			/* length of dataseg after last dseg_addtarget is used by load */
			mcodeptr[-2] = (Inst) (ptrint) -(cd->dseglen);
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
			gen_LOOKUPSWITCH(&mcodeptr, i, NULL, 0, NULL);
			dseg_adddata(cd, (u1 *) (mcodeptr - 2)); /* actually mcodeptr[-3] */
			codegen_addreference(cd, (basicblock *) tptr[0], mcodeptr);

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
			mcodeptr[-2] = (Inst) (ptrint) -(cd->dseglen);
			break;


		case ICMD_BUILTIN:      /* ..., arg1, arg2, arg3 ==> ...              */
		                        /* op1 = arg count val.a = builtintable entry */
			bte = iptr->val.a;
			if (bte->fp == PATCHER_builtin_new) {
				gen_PATCHER_NEW(&mcodeptr, 0);
			} else if (bte->fp == PATCHER_builtin_newarray) {
				gen_PATCHER_NEWARRAY(&mcodeptr, 0);
			} else if (bte->fp == PATCHER_builtin_arrayinstanceof) {
				gen_PATCHER_ARRAYINSTANCEOF(&mcodeptr, 0);
			} else {
				for (i = 0; i < sizeof(builtin_gen_table)/sizeof(builtin_gen); i++) {
					builtin_gen *bg = &builtin_gen_table[i];
					if (bg->builtin == bte->fp) {
						(bg->gen)(&mcodeptr);
						break;
					}
				}
			}
			break;

		case ICMD_INVOKESTATIC: /* ..., [arg1, [arg2 ...]] ==> ...            */
		                        /* op1 = arg count, val.a = method pointer    */

			lm = iptr->val.a;
			um = iptr->target;

			if (lm == NULL) {
				md = um->methodref->parseddesc.md;
				gen_PATCHER_INVOKESTATIC(&mcodeptr, 0, md->paramslots, um);

			} else {
				md = lm->parseddesc;
				gen_INVOKESTATIC(&mcodeptr, (Inst **)lm->stubroutine, md->paramslots, um);
			}
			break;

		case ICMD_INVOKESPECIAL:/* ..., objectref, [arg1, [arg2 ...]] ==> ... */

			lm = iptr->val.a;
			um = iptr->target;

			if (lm == NULL) {
				md = um->methodref->parseddesc.md;
				gen_PATCHER_INVOKESPECIAL(&mcodeptr, 0, md->paramslots, um);

			} else {
				md = lm->parseddesc;
				gen_INVOKESPECIAL(&mcodeptr, (Inst **)lm->stubroutine, md->paramslots, um);
			}
			break;

		case ICMD_INVOKEVIRTUAL:/* op1 = arg count, val.a = method pointer    */

			lm = iptr->val.a;
			um = iptr->target;

			if (lm == NULL) {
				md = um->methodref->parseddesc.md;
				gen_PATCHER_INVOKEVIRTUAL(&mcodeptr, 0, md->paramslots, um);

			} else {
				md = lm->parseddesc;

				s1 = OFFSET(vftbl_t, table[0]) +
					sizeof(methodptr) * lm->vftblindex;

				gen_INVOKEVIRTUAL(&mcodeptr, s1, md->paramslots, um);
			}
			break;

		case ICMD_INVOKEINTERFACE:/* op1 = arg count, val.a = method pointer  */

			lm = iptr->val.a;
			um = iptr->target;

			if (lm == NULL) {
				md = um->methodref->parseddesc.md;
				gen_PATCHER_INVOKEINTERFACE(&mcodeptr, 0, 0, md->paramslots, um);

			} else {
				md = lm->parseddesc;

				s1 = OFFSET(vftbl_t, interfacetable[0]) -
					sizeof(methodptr*) * lm->class->index;

				s2 = sizeof(methodptr) * (lm - lm->class->methods);

				gen_INVOKEINTERFACE(&mcodeptr, s1, s2, md->paramslots, um);
			}
			break;


		case ICMD_CHECKCAST:  /* ..., objectref ==> ..., objectref            */
		                      /* op1:   0 == array, 1 == class                */
		                      /* val.a: (classinfo *) superclass              */

			if (iptr->val.a == NULL) {
				gen_PATCHER_CHECKCAST(&mcodeptr, 0, iptr->target);

			} else {
				gen_CHECKCAST(&mcodeptr, iptr->val.a, iptr->target);
			}
			
			break;

		case ICMD_ARRAYCHECKCAST: /* ..., objectref ==> ..., objectref        */
		                          /* op1: 1... resolved, 0... not resolved    */

			if (iptr->op1 == 0) {
				gen_PATCHER_ARRAYCHECKCAST(&mcodeptr, 0, iptr->target);
			} else {
				gen_ARRAYCHECKCAST(&mcodeptr, iptr->target, 0);
			}
			break;

		case ICMD_INSTANCEOF: /* ..., objectref ==> ..., intresult            */
		                      /* op1:   0 == array, 1 == class                */
		                      /* val.a: (classinfo *) superclass              */

			if (iptr->val.a == NULL) {
				gen_PATCHER_INSTANCEOF(&mcodeptr, 0, iptr->target);
			} else {
				gen_INSTANCEOF(&mcodeptr, iptr->val.a, iptr->target);
			}
			
			break;


		case ICMD_CHECKASIZE:  /* ..., size ==> ..., size                     */

			/* XXX remove me! */
			break;

		case ICMD_CHECKEXCEPTION:    /* ..., objectref ==> ..., objectref     */

			gen_CHECKEXCEPTION(&mcodeptr);
			break;

		case ICMD_MULTIANEWARRAY:/* ..., cnt1, [cnt2, ...] ==> ..., arrayref  */
		                      /* op1 = dimension, val.a = array descriptor    */

			if (iptr->target) {
				gen_PATCHER_MULTIANEWARRAY(&mcodeptr, 0, iptr->op1, iptr->val.a);
			} else {
				gen_MULTIANEWARRAY(&mcodeptr, iptr->val.a, iptr->op1, 0);
			}
			break;

		default:
			throw_cacao_exception_exit(string_java_lang_InternalError,
									   "Unknown ICMD %d", iptr->opc);
	} /* switch */
		
	} /* for instruction */
		
	} /* if (bptr -> flags >= BBREACHED) */
	} /* for basic block */

	codegen_createlinenumbertable(cd);

	codegen_finish(m, cd, (s4) ((u1 *) mcodeptr - cd->mcodebase));


	/* branch resolving (walk through all basic blocks) */

	for (bptr = m->basicblocks; bptr != NULL; bptr = bptr->next) {
		branchref *brefs;

		for (brefs = bptr->branchrefs; brefs != NULL; brefs = brefs->next) {
			gen_resolveanybranch(((u1*) m->entrypoint) + brefs->branchpos,
			                     ((u1 *)m->entrypoint) + bptr->mpc);
		}
	}
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

#define STUBSIZE 4

functionptr createcompilerstub (methodinfo *m)
{
	Inst * t = CNEW(Inst, STUBSIZE);
	Inst * codeptr=t;
	genarg_ainst(&t, t+2);

	if (m->flags & ACC_NATIVE) {
		genarg_i(&t, m->parseddesc->paramslots);
	} else {
		genarg_i(&t, m->maxlocals);
	}

	gen_TRANSLATE(&t, m);
	return (functionptr)codeptr;
}


/* native stub:
+---------+
|NATIVECALL|
+---------+
|methodinf|
+---------+
|function |
+---------+

where maxlocals==paramslots
*/

#define NATIVE_STUBSIZE 5

functionptr createnativestub(functionptr f, methodinfo *m, codegendata *cd,
							 registerdata *rd, methoddesc *md)
{
	Inst *mcodeptr;

	mcodeptr = (s4 *) cd->mcodebase;
	cd->mcodeend = (s4 *) (cd->mcodebase + cd->mcodesize);

	/* create method header */

	(void) dseg_addaddress(cd, m);                          /* MethodPointer  */
	(void) dseg_adds4(cd, 0);                               /* IsSync         */
	dseg_addlinenumbertablesize(cd);
	(void) dseg_adds4(cd, 0);                               /* ExTableSize    */

	if (runverbose)
		gen_TRACECALL(&mcodeptr);

	if (f == NULL) {
		gen_PATCHER_NATIVECALL(&mcodeptr, m, f);
	} else {
		if (runverbose)
			gen_TRACENATIVECALL(&mcodeptr, m, f);
		else
			gen_NATIVECALL(&mcodeptr, m, f);
	}

	codegen_finish(m, cd, (s4) ((u1 *) mcodeptr - cd->mcodebase));

	return m->entrypoint;
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
