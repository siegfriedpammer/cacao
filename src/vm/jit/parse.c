/* src/vm/jit/parse.c - parser for JavaVM to intermediate code translation

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

   Author: Andreas Krall

   Changes: Carolyn Oates
            Edwin Steiner
            Joseph Wenninger
            Christian Thalinger

   $Id: parse.c 4600 2006-03-14 23:00:44Z edwin $

*/


#include <assert.h>
#include <string.h>

#include "config.h"

#include "vm/types.h"

#include "mm/memory.h"
#include "native/native.h"
#include "toolbox/logging.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/linker.h"
#include "vm/loader.h"
#include "vm/resolve.h"
#include "vm/options.h"
#include "vm/statistics.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/jit.h"
#include "vm/jit/parse.h"
#include "vm/jit/patcher.h"
#include "vm/jit/loop/loop.h"

/*******************************************************************************

	function 'parse' scans the JavaVM code and generates intermediate code

	During parsing the block index table is used to store at bit pos 0
	a flag which marks basic block starts and at position 1 to 31 the
	intermediate instruction index. After parsing the block index table
	is scanned, for marked positions a block is generated and the block
	number is stored in the block index table.

*******************************************************************************/

static exceptiontable * fillextable(methodinfo *m, 
									exceptiontable *extable, 
									exceptiontable *raw_extable, 
        							int exceptiontablelength, 
									int *block_count)
{
	int b_count, p, src, insertBlock;
	
	if (exceptiontablelength == 0) 
		return extable;

	b_count = *block_count;

	for (src = exceptiontablelength-1; src >=0; src--) {
   		p = raw_extable[src].startpc;
		extable->startpc = p;
		bound_check(p);
		block_insert(p);
		
		p = raw_extable[src].endpc; /* see JVM Spec 4.7.3 */
		if (p <= raw_extable[src].startpc) {
			*exceptionptr = new_verifyerror(m,
				"Invalid exception handler range");
			return NULL;
		}

		if (p >m->jcodelength) {
			*exceptionptr = new_verifyerror(m,
				"Invalid exception handler end is after code end");
			return NULL;
		}

		if (p<m->jcodelength) insertBlock=1; else insertBlock=0;
		extable->endpc = p;
		bound_check_exclusive(p);
		/* if (p < m->jcodelength) block_insert(p); */
        if (insertBlock) 
			block_insert(p);

		p = raw_extable[src].handlerpc;
		extable->handlerpc = p;
		bound_check(p);
		block_insert(p);

		extable->catchtype  = raw_extable[src].catchtype;
		extable->next = NULL;
		extable->down = &extable[1];
		extable--;
	}

	*block_count = b_count;
	return extable; /*&extable[i];*/  /* return the next free xtable* */
}



methodinfo *parse(methodinfo *m, codegendata *cd)
{
	int  p;                     /* java instruction counter           */
	int  nextp;                 /* start of next java instruction     */
	int  opcode;                /* java opcode                        */
	int  i;                     /* temporary for different uses (ctrs)*/
	int  ipc = 0;               /* intermediate instruction counter   */
	int  b_count = 0;           /* basic block counter                */
	int  s_count = 0;           /* stack element counter              */
	bool blockend = false;      /* true if basic block end has been reached   */
	bool iswide = false;        /* true if last instruction was a wide*/
	instruction *iptr;          /* current ptr into instruction array */
	int gp;                     /* global java instruction counter    */

	int firstlocal = 0;         /* first local variable of method     */
	exceptiontable* nextex;     /* points next free entry in extable  */
	u1 *instructionstart;       /* 1 for pcs which are valid instr. starts    */

	constant_classref  *cr;
	constant_classref  *compr;
	classinfo          *c;
	builtintable_entry *bte;

	u2 lineindex = 0;
	u2 currentline = 0;
	u2 linepcchange = 0;

	u2 skipBasicBlockChange;

	/* allocate instruction array and block index table */
	
	/* 1 additional for end ipc  */
	m->basicblockindex = DMNEW(s4, m->jcodelength + 1);
	memset(m->basicblockindex, 0, sizeof(s4) * (m->jcodelength + 1));

	instructionstart = DMNEW(u1, m->jcodelength + 1);
	memset(instructionstart, 0, sizeof(u1) * (m->jcodelength + 1));

	/* 1 additional for TRACEBUILTIN and 4 for MONITORENTER/EXIT */
	/* additional MONITOREXITS are reached by branches which are 3 bytes */
	
	iptr = m->instructions = DMNEW(instruction, m->jcodelength + 5);

	/* Zero the intermediate instructions array so we don't have any
	 * invalid pointers in it if we cannot finish analyse_stack(). */

	memset(iptr, 0, sizeof(instruction) * (m->jcodelength + 5));
	
	/* compute branch targets of exception table */

	nextex = fillextable(m, 
 	  &(cd->exceptiontable[cd->exceptiontablelength-1]), m->exceptiontable, 
	  	m->exceptiontablelength, &b_count);
	if (!nextex)
		return NULL;
	s_count = 1 + m->exceptiontablelength; /* initialize stack element counter   */

#if defined(USE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		m->isleafmethod = false;
	}			
#endif

	/* scan all java instructions */
	currentline = 0;
	linepcchange = 0;

	if (m->linenumbercount == 0) {
		lineindex = 0;
	} else {
		linepcchange = m->linenumbers[0].start_pc;
	}

	skipBasicBlockChange=0;
	for (p = 0, gp = 0; p < m->jcodelength; gp += (nextp - p), p = nextp) {
	  
		/* mark this position as a valid instruction start */
		if (!iswide) {
			instructionstart[gp] = 1;
			if (linepcchange==p) {
				if (m->linenumbercount > lineindex) {
					currentline = m->linenumbers[lineindex].line_number;
					lineindex++;
					if (lineindex < m->linenumbercount)
						linepcchange = m->linenumbers[lineindex].start_pc;
				}
			}
		}

		/* fetch next opcode  */

		opcode = code_get_u1(p, m);

		if (!skipBasicBlockChange) {
			m->basicblockindex[gp] |= (ipc << 1); /*store intermed cnt*/
		} else skipBasicBlockChange=0;

		/* some compilers put a JAVA_NOP after a blockend instruction */

		if ((opcode != JAVA_NOP) && (blockend == true)) {
			/* start new block */

			block_insert(gp);
			blockend = false;
		}

		nextp = p + jcommandsize[opcode];   /* compute next instruction start */

		if (nextp > m->jcodelength) {
			*exceptionptr = new_verifyerror(m,
					"Unexpected end of bytecode");
			return NULL;
		}

		s_count += stackreq[opcode];      	/* compute stack element count    */
		switch (opcode) {
		case JAVA_NOP:
			break;

			/* pushing constants onto the stack p */

		case JAVA_BIPUSH:
			LOADCONST_I(code_get_s1(p+1,m));
			break;

		case JAVA_SIPUSH:
			LOADCONST_I(code_get_s2(p+1,m));
			break;

		case JAVA_LDC1:
			i = code_get_u1(p + 1, m);
			goto pushconstantitem;

		case JAVA_LDC2:
		case JAVA_LDC2W:
			i = code_get_u2(p + 1, m);

		pushconstantitem:

			if (i >= m->class->cpcount) {
				*exceptionptr = new_verifyerror(m,
					"Attempt to access constant outside range");
				return NULL;
			}

			switch (m->class->cptags[i]) {
			case CONSTANT_Integer:
				LOADCONST_I(((constant_integer *) (m->class->cpinfos[i]))->value);
				break;
			case CONSTANT_Long:
				LOADCONST_L(((constant_long *) (m->class->cpinfos[i]))->value);
				break;
			case CONSTANT_Float:
				LOADCONST_F(((constant_float *) (m->class->cpinfos[i]))->value);
				break;
			case CONSTANT_Double:
				LOADCONST_D(((constant_double *) (m->class->cpinfos[i]))->value);
				break;
			case CONSTANT_String:
				LOADCONST_A(literalstring_new((utf *) (m->class->cpinfos[i])));
				break;
			case CONSTANT_Class:
				cr = (constant_classref *) (m->class->cpinfos[i]);

				if (!resolve_classref(m, cr, resolveLazy, true,
									  true, &c))
					return NULL;

				/* if not resolved, c == NULL */

				LOADCONST_A_CLASS(c, cr);
				break;
			default:
				*exceptionptr = new_verifyerror(m,
						"Invalid constant type to push");
				return NULL;
			}
			break;

		case JAVA_ACONST_NULL:
			LOADCONST_A(NULL);
			break;

		case JAVA_ICONST_M1:
		case JAVA_ICONST_0:
		case JAVA_ICONST_1:
		case JAVA_ICONST_2:
		case JAVA_ICONST_3:
		case JAVA_ICONST_4:
		case JAVA_ICONST_5:
			LOADCONST_I(opcode - JAVA_ICONST_0);
			break;

		case JAVA_LCONST_0:
		case JAVA_LCONST_1:
			LOADCONST_L(opcode - JAVA_LCONST_0);
			break;

		case JAVA_FCONST_0:
		case JAVA_FCONST_1:
		case JAVA_FCONST_2:
			LOADCONST_F(opcode - JAVA_FCONST_0);
			break;

		case JAVA_DCONST_0:
		case JAVA_DCONST_1:
			LOADCONST_D(opcode - JAVA_DCONST_0);
			break;

			/* loading variables onto the stack */

		case JAVA_ILOAD:
		case JAVA_LLOAD:
		case JAVA_FLOAD:
		case JAVA_DLOAD:
		case JAVA_ALOAD:
			if (!iswide) {
				i = code_get_u1(p + 1,m);
			} else {
				i = code_get_u2(p + 1,m);
				nextp = p + 3;
				iswide = false;
			}
			OP1LOAD(opcode, i + firstlocal);
			break;

		case JAVA_ILOAD_0:
		case JAVA_ILOAD_1:
		case JAVA_ILOAD_2:
		case JAVA_ILOAD_3:
			OP1LOAD(ICMD_ILOAD, opcode - JAVA_ILOAD_0 + firstlocal);
			break;

		case JAVA_LLOAD_0:
		case JAVA_LLOAD_1:
		case JAVA_LLOAD_2:
		case JAVA_LLOAD_3:
			OP1LOAD(ICMD_LLOAD, opcode - JAVA_LLOAD_0 + firstlocal);
			break;

		case JAVA_FLOAD_0:
		case JAVA_FLOAD_1:
		case JAVA_FLOAD_2:
		case JAVA_FLOAD_3:
			OP1LOAD(ICMD_FLOAD, opcode - JAVA_FLOAD_0 + firstlocal);
			break;

		case JAVA_DLOAD_0:
		case JAVA_DLOAD_1:
		case JAVA_DLOAD_2:
		case JAVA_DLOAD_3:
			OP1LOAD(ICMD_DLOAD, opcode - JAVA_DLOAD_0 + firstlocal);
			break;

		case JAVA_ALOAD_0:
		case JAVA_ALOAD_1:
		case JAVA_ALOAD_2:
		case JAVA_ALOAD_3:
			OP1LOAD(ICMD_ALOAD, opcode - JAVA_ALOAD_0 + firstlocal);
			break;

			/* storing stack values into local variables */

		case JAVA_ISTORE:
		case JAVA_LSTORE:
		case JAVA_FSTORE:
		case JAVA_DSTORE:
		case JAVA_ASTORE:
			if (!iswide) {
				i = code_get_u1(p + 1,m);
			} else {
				i = code_get_u2(p + 1,m);
				iswide = false;
				nextp = p + 3;
			}
			OP1STORE(opcode, i + firstlocal);
			break;

		case JAVA_ISTORE_0:
		case JAVA_ISTORE_1:
		case JAVA_ISTORE_2:
		case JAVA_ISTORE_3:
			OP1STORE(ICMD_ISTORE, opcode - JAVA_ISTORE_0 + firstlocal);
			break;

		case JAVA_LSTORE_0:
		case JAVA_LSTORE_1:
		case JAVA_LSTORE_2:
		case JAVA_LSTORE_3:
			OP1STORE(ICMD_LSTORE, opcode - JAVA_LSTORE_0 + firstlocal);
			break;

		case JAVA_FSTORE_0:
		case JAVA_FSTORE_1:
		case JAVA_FSTORE_2:
		case JAVA_FSTORE_3:
			OP1STORE(ICMD_FSTORE, opcode - JAVA_FSTORE_0 + firstlocal);
			break;

		case JAVA_DSTORE_0:
		case JAVA_DSTORE_1:
		case JAVA_DSTORE_2:
		case JAVA_DSTORE_3:
			OP1STORE(ICMD_DSTORE, opcode - JAVA_DSTORE_0 + firstlocal);
			break;

		case JAVA_ASTORE_0:
		case JAVA_ASTORE_1:
		case JAVA_ASTORE_2:
		case JAVA_ASTORE_3:
			OP1STORE(ICMD_ASTORE, opcode - JAVA_ASTORE_0 + firstlocal);
			break;

		case JAVA_IINC:
			{
				int v;
				
				if (!iswide) {
					i = code_get_u1(p + 1,m);
					v = code_get_s1(p + 2,m);

				} else {
					i = code_get_u2(p + 1,m);
					v = code_get_s2(p + 3,m);
					iswide = false;
					nextp = p + 5;
				}
				INDEX_ONEWORD(i + firstlocal);
				OP2I(opcode, i + firstlocal, v);
			}
			break;

			/* wider index for loading, storing and incrementing */

		case JAVA_WIDE:
			iswide = true;
			nextp = p + 1;
			break;

		/* managing arrays ****************************************************/

		case JAVA_NEWARRAY:
			switch (code_get_s1(p + 1, m)) {
			case 4:
				bte = builtintable_get_internal(BUILTIN_newarray_boolean);
				break;
			case 5:
				bte = builtintable_get_internal(BUILTIN_newarray_char);
				break;
			case 6:
				bte = builtintable_get_internal(BUILTIN_newarray_float);
				break;
			case 7:
				bte = builtintable_get_internal(BUILTIN_newarray_double);
				break;
			case 8:
				bte = builtintable_get_internal(BUILTIN_newarray_byte);
				break;
			case 9:
				bte = builtintable_get_internal(BUILTIN_newarray_short);
				break;
			case 10:
				bte = builtintable_get_internal(BUILTIN_newarray_int);
				break;
			case 11:
				bte = builtintable_get_internal(BUILTIN_newarray_long);
				break;
			default:
				*exceptionptr = new_verifyerror(m,
						"Invalid array-type to create");
				return NULL;
			}
			BUILTIN(bte, true, NULL, currentline);
			break;

		case JAVA_ANEWARRAY:
			i = code_get_u2(p + 1, m);
			compr = (constant_classref *) class_getconstant(m->class, i, CONSTANT_Class);
			if (!compr)
				return NULL;

			if (!(cr = class_get_classref_multiarray_of(1, compr)))
				return NULL;

			if (!resolve_classref(m, cr, resolveLazy, true, true, &c))
				return NULL;

			LOADCONST_A_BUILTIN(c, cr);
			bte = builtintable_get_internal(BUILTIN_newarray);
			BUILTIN(bte, true, NULL, currentline);
			s_count++;
			break;

		case JAVA_MULTIANEWARRAY:
			m->isleafmethod = false;
			i = code_get_u2(p + 1, m);
			{
				s4 v = code_get_u1(p + 3, m);

				cr = (constant_classref *) class_getconstant(m->class, i, CONSTANT_Class);
				if (!cr)
					return NULL;

				if (!resolve_classref(m, cr, resolveLazy, true, true, &c))
					return NULL;

				/* if unresolved, c == NULL */
				OP2AT(opcode, v, c, cr, currentline);
			}
			break;

		case JAVA_IFEQ:
		case JAVA_IFLT:
		case JAVA_IFLE:
		case JAVA_IFNE:
		case JAVA_IFGT:
		case JAVA_IFGE:
		case JAVA_IFNULL:
		case JAVA_IFNONNULL:
		case JAVA_IF_ICMPEQ:
		case JAVA_IF_ICMPNE:
		case JAVA_IF_ICMPLT:
		case JAVA_IF_ICMPGT:
		case JAVA_IF_ICMPLE:
		case JAVA_IF_ICMPGE:
		case JAVA_IF_ACMPEQ:
		case JAVA_IF_ACMPNE:
		case JAVA_GOTO:
		case JAVA_JSR:
			i = p + code_get_s2(p + 1,m);
			bound_check(i);
			block_insert(i);
			blockend = true;
			OP1(opcode, i);
			break;

		case JAVA_GOTO_W:
		case JAVA_JSR_W:
			i = p + code_get_s4(p + 1,m);
			bound_check(i);
			block_insert(i);
			blockend = true;
			OP1(opcode, i);
			break;

		case JAVA_RET:
			if (!iswide) {
				i = code_get_u1(p + 1,m);
			} else {
				i = code_get_u2(p + 1,m);
				nextp = p + 3;
				iswide = false;
			}
			blockend = true;
				
			OP1LOAD(opcode, i + firstlocal);
			break;

		case JAVA_IRETURN:
		case JAVA_LRETURN:
		case JAVA_FRETURN:
		case JAVA_DRETURN:
		case JAVA_ARETURN:
		case JAVA_RETURN:
			blockend = true;
			/* zero val.a so no patcher is inserted */
			/* the type checker may set this later  */
			iptr->val.a = NULL;
			OP(opcode);
			break;

		case JAVA_ATHROW:
			blockend = true;
			/* zero val.a so no patcher is inserted */
			/* the type checker may set this later  */
			iptr->val.a = NULL;
			OP(opcode);
			break;
				

		/* table jumps ********************************************************/

		case JAVA_LOOKUPSWITCH:
			{
				s4 num, j;
				s4 *tablep;
				s4 prevvalue=0;

				blockend = true;
				nextp = ALIGN((p + 1), 4);

				if (nextp + 8 > m->jcodelength) {
					*exceptionptr = new_verifyerror(m,
							"Unexpected end of bytecode");
					return NULL;
				}

				tablep = (s4 *) (m->jcode + nextp);

				OP2A(opcode, 0, tablep, currentline);

				/* default target */

				j =  p + code_get_s4(nextp, m);
				*tablep = j;     /* restore for little endian */
				tablep++;
				nextp += 4;
				bound_check(j);
				block_insert(j);

				/* number of pairs */

				num = code_get_u4(nextp, m);
				*tablep = num;
				tablep++;
				nextp += 4;

				if (nextp + 8 * num > m->jcodelength) {
					*exceptionptr = new_verifyerror(m,
						"Unexpected end of bytecode");
					return NULL;
				}

				for (i = 0; i < num; i++) {
					/* value */

					j = code_get_s4(nextp, m);
					*tablep = j; /* restore for little endian */
					tablep++;
					nextp += 4;

					/* check if the lookup table is sorted correctly */
					
					if (i && (j <= prevvalue)) {
						*exceptionptr = new_verifyerror(m, "Unsorted lookup switch");
						return NULL;
					}
					prevvalue = j;

					/* target */

					j = p + code_get_s4(nextp,m);
					*tablep = j; /* restore for little endian */
					tablep++;
					nextp += 4;
					bound_check(j);
					block_insert(j);
				}

				break;
			}


		case JAVA_TABLESWITCH:
			{
				s4 num, j;
				s4 *tablep;

				blockend = true;
				nextp = ALIGN((p + 1), 4);
				if (nextp + 12 > m->jcodelength) {
					*exceptionptr = new_verifyerror(m,
						"Unexpected end of bytecode");
					return NULL;
				}

				tablep = (s4 *) (m->jcode + nextp);

				OP2A(opcode, 0, tablep, currentline);

				/* default target */

				j = p + code_get_s4(nextp, m);
				*tablep = j;     /* restore for little endian */
				tablep++;
				nextp += 4;
				bound_check(j);
				block_insert(j);

				/* lower bound */

				j = code_get_s4(nextp, m);
				*tablep = j;     /* restore for little endian */
				tablep++;
				nextp += 4;

				/* upper bound */

				num = code_get_s4(nextp, m);
				*tablep = num;   /* restore for little endian */
				tablep++;
				nextp += 4;

				num -= j;  /* difference of upper - lower */

				if (num < 0) {
					*exceptionptr = new_verifyerror(m,
							"invalid TABLESWITCH: upper bound < lower bound");
					return NULL;
				}

				if (nextp + 4 * (num + 1) > m->jcodelength) {
					*exceptionptr = new_verifyerror(m,
						"Unexpected end of bytecode");
					return NULL;
				}

				for (i = 0; i <= num; i++) {
					j = p + code_get_s4(nextp,m);
					*tablep = j; /* restore for little endian */
					tablep++;
					nextp += 4;
					bound_check(j);
					block_insert(j);
				}

				break;
			}


		/* load and store of object fields ************************************/

		case JAVA_AASTORE:
			OP(opcode);
			m->isleafmethod = false;
			break;

		case JAVA_GETSTATIC:
		case JAVA_PUTSTATIC:
		case JAVA_GETFIELD:
		case JAVA_PUTFIELD:
			{
				constant_FMIref  *fr;
				unresolved_field *uf;
				fieldinfo        *fi;

				i = code_get_u2(p + 1, m);
				fr = class_getconstant(m->class, i,
									   CONSTANT_Fieldref);
				if (!fr)
					return NULL;

				OP2A_NOINC(opcode, fr->parseddesc.fd->type, fr, currentline);

				if (!(uf = create_unresolved_field(m->class,
												   m,
												   iptr)))
					return NULL;

				/* store unresolved_field pointer */

				iptr->target = uf;

				/* only with -noverify, otherwise the typechecker does this */

				if (!opt_verify) {
					if (!resolve_field(uf, resolveLazy, &fi))
						return NULL;

					iptr->val.a = fi;

				} else {
					iptr->val.a = NULL;
				}
				PINC;
			}
			break;


		/* method invocation **************************************************/

		case JAVA_INVOKESTATIC:
			i = code_get_u2(p + 1, m);
			{
				constant_FMIref   *mr;
				methoddesc        *md;
				unresolved_method *um;
				methodinfo        *mi;

				m->isleafmethod = false;

				mr = class_getconstant(m->class, i,
									   CONSTANT_Methodref);
				if (!mr)
					return NULL;

				md = mr->parseddesc.md;

				if (!md->params)
					if (!descriptor_params_from_paramtypes(md, ACC_STATIC))
						return NULL;

				OP2A_NOINC(opcode, 0, mr, currentline);

				um = create_unresolved_method(m->class,
											  m, iptr);

				if (!um)
					return NULL;

				/* store the unresolved_method pointer */

				iptr->target = um;

				/* only with -noverify, otherwise the typechecker does this */

				if (!opt_verify) {
					if (!resolve_method(um, resolveLazy, &mi))
						return NULL;

					iptr->val.a = mi;
				}
				else {
					iptr->val.a = NULL;
				}
				PINC;
			}
			break;

		case JAVA_INVOKESPECIAL:
		case JAVA_INVOKEVIRTUAL:
			{
				constant_FMIref   *mr;
				methoddesc        *md;
				unresolved_method *um;
				methodinfo        *mi;

				m->isleafmethod = false;

				i = code_get_u2(p + 1, m);
				mr = class_getconstant(m->class, i,
									   CONSTANT_Methodref);
				if (!mr)
					return NULL;

				md = mr->parseddesc.md;

				if (!md->params)
					if (!descriptor_params_from_paramtypes(md, 0))
						return NULL;
				
				OP2A_NOINC(opcode, 0, mr, currentline);

				um = create_unresolved_method(m->class,
											  m, iptr);

				if (!um)
					return NULL;

				/* store the unresolved_method* */

				iptr->target = um;

				/* only with -noverify, otherwise the typechecker does this */

				if (!opt_verify) {
					if (!resolve_method(um, resolveLazy, &mi))
						return NULL;

					iptr->val.a = mi;
				}
				else {
					iptr->val.a = NULL;
				}
				PINC;
			}
			break;

		case JAVA_INVOKEINTERFACE:
			i = code_get_u2(p + 1, m);
			{
				constant_FMIref   *mr;
				methoddesc        *md;
				unresolved_method *um;
				methodinfo        *mi;
				
				m->isleafmethod = false;

				mr = class_getconstant(m->class, i,
									   CONSTANT_InterfaceMethodref);
				if (!mr)
					return NULL;

				md = mr->parseddesc.md;

				if (!md->params)
					if (!descriptor_params_from_paramtypes(md, 0))
						return NULL;

				OP2A_NOINC(opcode, 0, mr, currentline);

				um = create_unresolved_method(m->class,
											  m, iptr);

				if (!um)
					return NULL;

				/* store the unresolved_method* */

				iptr->target = um;

				/* only with -noverify, otherwise the typechecker does this */

				if (!opt_verify) {
					if (!resolve_method(um, resolveLazy, &mi))
						return NULL;

					iptr->val.a = mi;
				}
				else {
					iptr->val.a = NULL;
				}
				PINC;
			}
			break;

		/* miscellaneous object operations ************************************/

		case JAVA_NEW:
			i = code_get_u2(p + 1, m);
			cr = (constant_classref *) class_getconstant(m->class, i, CONSTANT_Class);
			if (!cr)
				return NULL;

			if (!resolve_classref(m, cr, resolveLazy, true, true,
								  &c))
				return NULL;

			LOADCONST_A_BUILTIN(c, cr);
			bte = builtintable_get_internal(BUILTIN_new);
			BUILTIN(bte, true, NULL, currentline);
			s_count++;
			break;

		case JAVA_CHECKCAST:
			i = code_get_u2(p + 1, m);
			cr = (constant_classref *) class_getconstant(m->class, i, CONSTANT_Class);
			if (!cr)
				return NULL;

			if (!resolve_classref(m, cr, resolveLazy, true,
								  true, &c))
				return NULL;

			if (cr->name->text[0] == '[') {
				/* array type cast-check */
				OP2AT(opcode, 0, c, cr, currentline);
				m->isleafmethod = false;

			} else {
				/* object type cast-check */
				OP2AT(opcode, 1, c, cr, currentline);
			}
			break;

		case JAVA_INSTANCEOF:
			i = code_get_u2(p + 1,m);
			cr = (constant_classref *) class_getconstant(m->class, i, CONSTANT_Class);
			if (!cr)
				return NULL;

			if (!resolve_classref(m, cr, resolveLazy, true, true, &c))
				return NULL;

			if (cr->name->text[0] == '[') {
				/* array type cast-check */
				LOADCONST_A_BUILTIN(c, cr);
				bte = builtintable_get_internal(BUILTIN_arrayinstanceof);
				BUILTIN(bte, false, NULL, currentline);
				s_count++;

			} else {
				/* object type cast-check */
				OP2AT(opcode, 1, c, cr, currentline);
			}
			break;

		case JAVA_MONITORENTER:
#if defined(USE_THREADS)
			if (checksync) {
				OP(ICMD_CHECKNULL);
				bte = builtintable_get_internal(BUILTIN_monitorenter);
				BUILTIN(bte, false, NULL, currentline);
			} else
#endif
				{
					OP(ICMD_CHECKNULL);
					OP(ICMD_POP);
				}
			break;

		case JAVA_MONITOREXIT:
#if defined(USE_THREADS)
			if (checksync) {
				bte = builtintable_get_internal(BUILTIN_monitorexit);
				BUILTIN(bte, false, NULL, currentline);
			} else
#endif
				{
					OP(ICMD_POP);
				}
			break;

		/* any other basic operation ******************************************/

		case JAVA_IDIV:
#if !SUPPORT_DIVISION
			bte = builtintable_get_internal(BUILTIN_idiv);
			OP2A(opcode, bte->md->paramcount, bte, currentline);
			m->isleafmethod = false;
#else
			OP(opcode);
#endif
			break;

		case JAVA_IREM:
#if !SUPPORT_DIVISION
			bte = builtintable_get_internal(BUILTIN_irem);
			OP2A(opcode, bte->md->paramcount, bte, currentline);
			m->isleafmethod = false;
#else
			OP(opcode);
#endif
			break;

		case JAVA_LDIV:
#if !(SUPPORT_DIVISION && SUPPORT_LONG && SUPPORT_LONG_DIV)
			bte = builtintable_get_internal(BUILTIN_ldiv);
			OP2A(opcode, bte->md->paramcount, bte, currentline);
			m->isleafmethod = false;
#else
			OP(opcode);
#endif
			break;

		case JAVA_LREM:
#if !(SUPPORT_DIVISION && SUPPORT_LONG && SUPPORT_LONG_DIV)
			bte = builtintable_get_internal(BUILTIN_lrem);
			OP2A(opcode, bte->md->paramcount, bte, currentline);
			m->isleafmethod = false;
#else
			OP(opcode);
#endif
			break;

		case JAVA_FREM:
#if defined(__I386__)
			OP(opcode);
#else
			bte = builtintable_get_internal(BUILTIN_frem);
			BUILTIN(bte, false, NULL, currentline);
#endif
			break;

		case JAVA_DREM:
#if defined(__I386__)
			OP(opcode);
#else
			bte = builtintable_get_internal(BUILTIN_drem);
			BUILTIN(bte, false, NULL, currentline);
#endif
			break;

		case JAVA_F2I:
#if defined(__ALPHA__)
			if (!opt_noieee) {
				bte = builtintable_get_internal(BUILTIN_f2i);
				BUILTIN(bte, false, NULL, currentline);
			} else
#endif
				{
					OP(opcode);
				}
			break;

		case JAVA_F2L:
#if defined(__ALPHA__)
			if (!opt_noieee) {
				bte = builtintable_get_internal(BUILTIN_f2l);
				BUILTIN(bte, false, NULL, currentline);
			} else 
#endif
				{
					OP(opcode);
				}
			break;

		case JAVA_D2I:
#if defined(__ALPHA__)
			if (!opt_noieee) {
				bte = builtintable_get_internal(BUILTIN_d2i);
				BUILTIN(bte, false, NULL, currentline);
			} else
#endif
				{
					OP(opcode);
				}
			break;

		case JAVA_D2L:
#if defined(__ALPHA__)
			if (!opt_noieee) {
				bte = builtintable_get_internal(BUILTIN_d2l);
				BUILTIN(bte, false, NULL, currentline);
			} else
#endif
				{
					OP(opcode);
				}
			break;

		case JAVA_BREAKPOINT:
			*exceptionptr =
				new_verifyerror(m, "Quick instructions shouldn't appear yet.");
			return NULL;

		case 186: /* unused opcode */
		case 203:
		case 204:
		case 205:
		case 206:
		case 207:
		case 208:
		case 209:
		case 210:
		case 211:
		case 212:
		case 213:
		case 214:
		case 215:
		case 216:
		case 217:
		case 218:
		case 219:
		case 220:
		case 221:
		case 222:
		case 223:
		case 224:
		case 225:
		case 226:
		case 227:
		case 228:
		case 229:
		case 230:
		case 231:
		case 232:
		case 233:
		case 234:
		case 235:
		case 236:
		case 237:
		case 238:
		case 239:
		case 240:
		case 241:
		case 242:
		case 243:
		case 244:
		case 245:
		case 246:
		case 247:
		case 248:
		case 249:
		case 250:
		case 251:
		case 252:
		case 253:
		case 254:
		case 255:
			*exceptionptr =
				new_verifyerror(m,"Illegal opcode %d at instr %d\n",
								  opcode, ipc);
			return NULL;
			break;

		default:
			OP(opcode);
			break;
				
		} /* end switch */

		/* If WIDE was used correctly, iswide should have been reset by now. */
		if (iswide && opcode != JAVA_WIDE) {
			*exceptionptr = new_verifyerror(m,
					"Illegal instruction: WIDE before incompatible opcode");
			return NULL;
		}

	} /* end for */

	if (p != m->jcodelength) {
		printf("p (%d) != m->jcodelength (%d)\n",p,m->jcodelength);
		*exceptionptr = new_verifyerror(m,
				"Command-sequence crosses code-boundary");
		return NULL;
	}

	if (!blockend) {
		*exceptionptr = new_verifyerror(m, "Falling off the end of the code");
		return NULL;
	}

	/* adjust block count if target 0 is not first intermediate instruction */

	if (!m->basicblockindex[0] || (m->basicblockindex[0] > 1))
		b_count++;

	/* copy local to method variables */

	m->instructioncount = ipc;
	m->basicblockcount = b_count;
	m->stackcount = s_count + m->basicblockcount * m->maxstack;

	/* allocate stack table */

	m->stack = DMNEW(stackelement, m->stackcount);

	{
		basicblock *bptr;

		bptr = m->basicblocks = DMNEW(basicblock, b_count + 1);    /* one more for end ipc */

		b_count = 0;
		m->c_debug_nr = 0;
	
		/* additional block if target 0 is not first intermediate instruction */

		if (!m->basicblockindex[0] || (m->basicblockindex[0] > 1)) {
			bptr->iinstr = m->instructions;
			bptr->mpc = -1;
			bptr->flags = -1;
			bptr->type = BBTYPE_STD;
			bptr->branchrefs = NULL;
			bptr->pre_count = 0;
			bptr->debug_nr = m->c_debug_nr++;
			bptr++;
			b_count++;
			(bptr - 1)->next = bptr;
		}

		/* allocate blocks */

  		for (p = 0; p < m->jcodelength; p++) { 
		/* for (p = 0; p < m->jcodelength; p++) { */
			if (m->basicblockindex[p] & 1) {
				/* Check if this block starts at the beginning of an          */
				/* instruction.                                               */

				if (!instructionstart[p]) {
					*exceptionptr = new_verifyerror(m,
						"Branch into middle of instruction");
					return NULL;
				}

				/* allocate the block */
				bptr->iinstr = m->instructions + (m->basicblockindex[p] >> 1);
				bptr->debug_nr = m->c_debug_nr++;
				if (b_count != 0)
					(bptr - 1)->icount = bptr->iinstr - (bptr - 1)->iinstr;
				bptr->mpc = -1;
				bptr->flags = -1;
				bptr->lflags = 0;
				bptr->type = BBTYPE_STD;
				bptr->branchrefs = NULL;
				m->basicblockindex[p] = b_count;
				bptr->pre_count = 0;
				bptr++;
				b_count++;
				(bptr - 1)->next = bptr;
			}
		}

		/* allocate additional block at end */

		bptr->instack = bptr->outstack = NULL;
		bptr->indepth = bptr->outdepth = 0;
		bptr->iinstr = NULL;
		(bptr - 1)->icount = (m->instructions + m->instructioncount) - (bptr - 1)->iinstr;
		bptr->icount = 0;
		bptr->mpc = -1;
		bptr->flags = -1;
		bptr->lflags = 0;
		bptr->type = BBTYPE_STD;
		bptr->branchrefs = NULL;
		bptr->pre_count = 0;
		bptr->debug_nr = m->c_debug_nr++;
		(bptr - 1)->next = bptr;
		bptr->next = NULL;

		if (cd->exceptiontablelength > 0) {
			cd->exceptiontable[cd->exceptiontablelength - 1].down = NULL;
		}
		
		for (i = 0; i < cd->exceptiontablelength; ++i) {
			p = cd->exceptiontable[i].startpc;
			cd->exceptiontable[i].start = m->basicblocks + m->basicblockindex[p];

			p = cd->exceptiontable[i].endpc;
			cd->exceptiontable[i].end = (p == m->jcodelength) ? (m->basicblocks + m->basicblockcount /*+ 1*/) : (m->basicblocks + m->basicblockindex[p]);

			p = cd->exceptiontable[i].handlerpc;
			cd->exceptiontable[i].handler = m->basicblocks + m->basicblockindex[p];
	    }
	}

	/* just return methodinfo* to signal everything was ok */

	return m;
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
