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

   $Id: parse.c 4993 2006-05-30 23:38:32Z edwin $

*/


#include "config.h"

#include <assert.h>
#include <string.h>

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
	int b_count, p, src;
	
	if (exceptiontablelength == 0) 
		return extable;

	b_count = *block_count;

	for (src = exceptiontablelength-1; src >=0; src--) {
		/* the start of the handled region becomes a basic block start */
   		p = raw_extable[src].startpc;
		CHECK_BYTECODE_INDEX(p);
		extable->startpc = p;
		block_insert(p);
		
		p = raw_extable[src].endpc; /* see JVM Spec 4.7.3 */
		CHECK_BYTECODE_INDEX_EXCLUSIVE(p);

#if defined(ENABLE_VERIFIER)
		if (p <= raw_extable[src].startpc) {
			*exceptionptr = new_verifyerror(m,
				"Invalid exception handler range");
			return NULL;
		}
#endif
		extable->endpc = p;
		
		/* end of handled region becomes a basic block boundary  */
		/* (If it is the bytecode end, we'll use the special     */
		/* end block that is created anyway.)                    */
		if (p < m->jcodelength) 
			block_insert(p);

		/* the start of the handler becomes a basic block start  */
		p = raw_extable[src].handlerpc;
		CHECK_BYTECODE_INDEX(p);
		extable->handlerpc = p;
		block_insert(p);

		extable->catchtype = raw_extable[src].catchtype;
		extable->next = NULL;
		extable->down = &extable[1];
		extable--;
	}

	*block_count = b_count;
	
	/* everything ok */
	return extable;

#if defined(ENABLE_VERIFIER)
throw_invalid_bytecode_index:
	*exceptionptr =
		new_verifyerror(m, "Illegal bytecode index in exception table");
	return NULL;
#endif
}

/*** macro for checking the length of the bytecode ***/

#if defined(ENABLE_VERIFIER)
#define CHECK_END_OF_BYTECODE(neededlength) \
	do { \
		if ((neededlength) > m->jcodelength) \
			goto throw_unexpected_end_of_bytecode; \
	} while (0)
#else /* !ENABLE_VERIFIER */
#define CHECK_END_OF_BYTECODE(neededlength)
#endif /* ENABLE_VERIFIER */

bool new_parse(jitdata *jd)
{
	methodinfo  *m;             /* method being parsed                      */
	codegendata *cd;
	int  p;                     /* java instruction counter                 */
	int  nextp;                 /* start of next java instruction           */
	int  opcode;                /* java opcode                              */
	int  i;                     /* temporary for different uses (ctrs)      */
	int  ipc = 0;               /* intermediate instruction counter         */
	int  b_count = 0;           /* basic block counter                      */
	int  s_count = 0;           /* stack element counter                    */
	bool blockend = false;      /* true if basic block end has been reached */
	bool iswide = false;        /* true if last instruction was a wide      */
	new_instruction *iptr;      /* current ptr into instruction array       */
	u1 *instructionstart;       /* 1 for pcs which are valid instr. starts  */
	constant_classref  *cr;
	constant_classref  *compr;
	classinfo          *c;
	builtintable_entry *bte;
	constant_FMIref    *mr;
	methoddesc         *md;
	unresolved_method  *um;
	resolve_result_t    result;
	u2                  lineindex = 0;
	u2                  currentline = 0;
	u2                  linepcchange = 0;
	u4                  flags;

	/* get required compiler data */

	m  = jd->m;
	cd = jd->cd;

	/* allocate instruction array and block index table */

	/* 1 additional for end ipc  */
	m->basicblockindex = DMNEW(s4, m->jcodelength + 1);
	memset(m->basicblockindex, 0, sizeof(s4) * (m->jcodelength + 1));

	instructionstart = DMNEW(u1, m->jcodelength + 1);
	memset(instructionstart, 0, sizeof(u1) * (m->jcodelength + 1));

	/* 1 additional for TRACEBUILTIN and 4 for MONITORENTER/EXIT */
	/* additional MONITOREXITS are reached by branches which are 3 bytes */

	iptr = jd->new_instructions = DMNEW(new_instruction, m->jcodelength + 5);

	/* Zero the intermediate instructions array so we don't have any
	 * invalid pointers in it if we cannot finish analyse_stack(). */

	memset(iptr, 0, sizeof(new_instruction) * (m->jcodelength + 5)); /* XXX remove this? */

	/* compute branch targets of exception table */

	if (!fillextable(m,
			&(cd->exceptiontable[cd->exceptiontablelength-1]),
			m->exceptiontable,
			m->exceptiontablelength,
			&b_count))
	{
		return false;
	}

	s_count = 1 + m->exceptiontablelength; /* initialize stack element counter   */

#if defined(ENABLE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		m->isleafmethod = false;
	}
#endif

	/* scan all java instructions */
	currentline = 0;
	linepcchange = 0;

	if (m->linenumbercount == 0) {
		lineindex = 0;
	}
	else {
		linepcchange = m->linenumbers[0].start_pc;
	}

	/*** LOOP OVER ALL BYTECODE INSTRUCTIONS **********************************/

	for (p = 0; p < m->jcodelength; p = nextp) {

		/* mark this position as a valid instruction start */

		instructionstart[p] = 1;

		/* change the current line number, if necessary */

		if (linepcchange == p) {
			if (m->linenumbercount > lineindex) {
next_linenumber:
				currentline = m->linenumbers[lineindex].line_number;
				lineindex++;
				if (lineindex < m->linenumbercount) {
					linepcchange = m->linenumbers[lineindex].start_pc;
					if (linepcchange == p)
						goto next_linenumber;
				}
			}
		}

		/* fetch next opcode  */
fetch_opcode:
		opcode = code_get_u1(p, m);

		/* store intermediate instruction count (bit 0 mark block starts) */

		m->basicblockindex[p] |= (ipc << 1);

		/* some compilers put a JAVA_NOP after a blockend instruction */

		if (blockend && (opcode != JAVA_NOP)) {
			/* start new block */

			block_insert(p);
			blockend = false;
		}

		/* compute next instruction start */

		nextp = p + jcommandsize[opcode];

		CHECK_END_OF_BYTECODE(nextp);

		/* add stack elements produced by this instruction */

		s_count += stackreq[opcode];

		/* translate this bytecode instruction */

		switch (opcode) {

		case JAVA_NOP:
			break;

		/* pushing constants onto the stack ***********************************/

		case JAVA_BIPUSH:
			NEW_OP_LOADCONST_I(code_get_s1(p+1,m));
			break;

		case JAVA_SIPUSH:
			NEW_OP_LOADCONST_I(code_get_s2(p+1,m));
			break;

		case JAVA_LDC1:
			i = code_get_u1(p + 1, m);
			goto pushconstantitem;

		case JAVA_LDC2:
		case JAVA_LDC2W:
			i = code_get_u2(p + 1, m);

		pushconstantitem:

#if defined(ENABLE_VERIFIER)
			if (i >= m->class->cpcount) {
				*exceptionptr = new_verifyerror(m,
					"Attempt to access constant outside range");
				return false;
			}
#endif

			switch (m->class->cptags[i]) {
			case CONSTANT_Integer:
				NEW_OP_LOADCONST_I(((constant_integer *) (m->class->cpinfos[i]))->value);
				break;
			case CONSTANT_Long:
				NEW_OP_LOADCONST_L(((constant_long *) (m->class->cpinfos[i]))->value);
				break;
			case CONSTANT_Float:
				NEW_OP_LOADCONST_F(((constant_float *) (m->class->cpinfos[i]))->value);
				break;
			case CONSTANT_Double:
				NEW_OP_LOADCONST_D(((constant_double *) (m->class->cpinfos[i]))->value);
				break;
			case CONSTANT_String:
				NEW_OP_LOADCONST_STRING(literalstring_new((utf *) (m->class->cpinfos[i])));
				break;
			case CONSTANT_Class:
				cr = (constant_classref *) (m->class->cpinfos[i]);

				if (!resolve_classref(m, cr, resolveLazy, true,
									  true, &c))
					return false;

				/* if not resolved, c == NULL */

				NEW_OP_LOADCONST_CLASSINFO_OR_CLASSREF(c, cr, 0 /* no extra flags */);

				break;

#if defined(ENABLE_VERIFIER)
			default:
				*exceptionptr = new_verifyerror(m,
						"Invalid constant type to push");
				return false;
#endif
			}
			break;

		case JAVA_ACONST_NULL:
			NEW_OP_LOADCONST_NULL();
			break;

		case JAVA_ICONST_M1:
		case JAVA_ICONST_0:
		case JAVA_ICONST_1:
		case JAVA_ICONST_2:
		case JAVA_ICONST_3:
		case JAVA_ICONST_4:
		case JAVA_ICONST_5:
			NEW_OP_LOADCONST_I(opcode - JAVA_ICONST_0);
			break;

		case JAVA_LCONST_0:
		case JAVA_LCONST_1:
			NEW_OP_LOADCONST_L(opcode - JAVA_LCONST_0);
			break;

		case JAVA_FCONST_0:
		case JAVA_FCONST_1:
		case JAVA_FCONST_2:
			NEW_OP_LOADCONST_F(opcode - JAVA_FCONST_0);
			break;

		case JAVA_DCONST_0:
		case JAVA_DCONST_1:
			NEW_OP_LOADCONST_D(opcode - JAVA_DCONST_0);
			break;

		/* local variable access instructions *********************************/

		case JAVA_ILOAD:
		case JAVA_FLOAD:
		case JAVA_ALOAD:
			if (!iswide) {
				i = code_get_u1(p + 1,m);
			}
			else {
				i = code_get_u2(p + 1,m);
				nextp = p + 3;
				iswide = false;
			}
			NEW_OP_LOAD_ONEWORD(opcode, i);
			break;

		case JAVA_LLOAD:
		case JAVA_DLOAD:
			if (!iswide) {
				i = code_get_u1(p + 1,m);
			}
			else {
				i = code_get_u2(p + 1,m);
				nextp = p + 3;
				iswide = false;
			}
			NEW_OP_LOAD_TWOWORD(opcode, i);
			break;

		case JAVA_ILOAD_0:
		case JAVA_ILOAD_1:
		case JAVA_ILOAD_2:
		case JAVA_ILOAD_3:
			NEW_OP_LOAD_ONEWORD(ICMD_ILOAD, opcode - JAVA_ILOAD_0);
			break;

		case JAVA_LLOAD_0:
		case JAVA_LLOAD_1:
		case JAVA_LLOAD_2:
		case JAVA_LLOAD_3:
			NEW_OP_LOAD_TWOWORD(ICMD_LLOAD, opcode - JAVA_LLOAD_0);
			break;

		case JAVA_FLOAD_0:
		case JAVA_FLOAD_1:
		case JAVA_FLOAD_2:
		case JAVA_FLOAD_3:
			NEW_OP_LOAD_ONEWORD(ICMD_FLOAD, opcode - JAVA_FLOAD_0);
			break;

		case JAVA_DLOAD_0:
		case JAVA_DLOAD_1:
		case JAVA_DLOAD_2:
		case JAVA_DLOAD_3:
			NEW_OP_LOAD_TWOWORD(ICMD_DLOAD, opcode - JAVA_DLOAD_0);
			break;

		case JAVA_ALOAD_0:
		case JAVA_ALOAD_1:
		case JAVA_ALOAD_2:
		case JAVA_ALOAD_3:
			NEW_OP_LOAD_ONEWORD(ICMD_ALOAD, opcode - JAVA_ALOAD_0);
			break;

		case JAVA_ISTORE:
		case JAVA_FSTORE:
		case JAVA_ASTORE:
			if (!iswide) {
				i = code_get_u1(p + 1,m);
			}
			else {
				i = code_get_u2(p + 1,m);
				iswide = false;
				nextp = p + 3;
			}
			NEW_OP_STORE_ONEWORD(opcode, i);
			break;

		case JAVA_LSTORE:
		case JAVA_DSTORE:
			if (!iswide) {
				i = code_get_u1(p + 1,m);
			}
			else {
				i = code_get_u2(p + 1,m);
				iswide = false;
				nextp = p + 3;
			}
			NEW_OP_STORE_TWOWORD(opcode, i);
			break;

		case JAVA_ISTORE_0:
		case JAVA_ISTORE_1:
		case JAVA_ISTORE_2:
		case JAVA_ISTORE_3:
			NEW_OP_STORE_ONEWORD(ICMD_ISTORE, opcode - JAVA_ISTORE_0);
			break;

		case JAVA_LSTORE_0:
		case JAVA_LSTORE_1:
		case JAVA_LSTORE_2:
		case JAVA_LSTORE_3:
			NEW_OP_STORE_TWOWORD(ICMD_LSTORE, opcode - JAVA_LSTORE_0);
			break;

		case JAVA_FSTORE_0:
		case JAVA_FSTORE_1:
		case JAVA_FSTORE_2:
		case JAVA_FSTORE_3:
			NEW_OP_STORE_ONEWORD(ICMD_FSTORE, opcode - JAVA_FSTORE_0);
			break;

		case JAVA_DSTORE_0:
		case JAVA_DSTORE_1:
		case JAVA_DSTORE_2:
		case JAVA_DSTORE_3:
			NEW_OP_STORE_TWOWORD(ICMD_DSTORE, opcode - JAVA_DSTORE_0);
			break;

		case JAVA_ASTORE_0:
		case JAVA_ASTORE_1:
		case JAVA_ASTORE_2:
		case JAVA_ASTORE_3:
			NEW_OP_STORE_ONEWORD(ICMD_ASTORE, opcode - JAVA_ASTORE_0);
			break;

		case JAVA_IINC:
			{
				int v;

				if (!iswide) {
					i = code_get_u1(p + 1,m);
					v = code_get_s1(p + 2,m);

				}
				else {
					i = code_get_u2(p + 1,m);
					v = code_get_s2(p + 3,m);
					iswide = false;
					nextp = p + 5;
				}
				INDEX_ONEWORD(i);
				NEW_OP_LOCALINDEX_I(opcode, i, v);
			}
			break;

		/* wider index for loading, storing and incrementing ******************/

		case JAVA_WIDE:
			iswide = true;
			p++;
			goto fetch_opcode;

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
#if defined(ENABLE_VERIFIER)
			default:
				*exceptionptr = new_verifyerror(m,
						"Invalid array-type to create");
				return false;
#endif
			}
			NEW_OP_BUILTIN_CHECK_EXCEPTION(bte);
			break;

		case JAVA_ANEWARRAY:
			i = code_get_u2(p + 1, m);
			compr = (constant_classref *) class_getconstant(m->class, i, CONSTANT_Class);
			if (!compr)
				return false;

			if (!(cr = class_get_classref_multiarray_of(1, compr)))
				return false;

			if (!resolve_classref(m, cr, resolveLazy, true, true, &c))
				return false;

			NEW_OP_LOADCONST_CLASSINFO_OR_CLASSREF(c, cr, INS_FLAG_NOCHECK);
			bte = builtintable_get_internal(BUILTIN_newarray);
			NEW_OP_BUILTIN_CHECK_EXCEPTION(bte);
			s_count++;
			break;

		case JAVA_MULTIANEWARRAY:
			m->isleafmethod = false;
			i = code_get_u2(p + 1, m);
			{
				s4 v = code_get_u1(p + 3, m);

				cr = (constant_classref *) class_getconstant(m->class, i, CONSTANT_Class);
				if (!cr)
					return false;

				if (!resolve_classref(m, cr, resolveLazy, true, true, &c))
					return false;

				/* if unresolved, c == NULL */

				iptr->s1.argcount = v; /* XXX */
				NEW_OP_S3_CLASSINFO_OR_CLASSREF(opcode, c, cr, 0 /* flags */);
			}
			break;

		/* control flow instructions ******************************************/

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
			CHECK_BYTECODE_INDEX(i);
			block_insert(i);
			blockend = true;
			NEW_OP_INSINDEX(opcode, i);
			break;

		case JAVA_GOTO_W:
		case JAVA_JSR_W:
			i = p + code_get_s4(p + 1,m);
			CHECK_BYTECODE_INDEX(i);
			block_insert(i);
			blockend = true;
			NEW_OP_INSINDEX(opcode, i);
			break;

		case JAVA_RET:
			if (!iswide) {
				i = code_get_u1(p + 1,m);
			}
			else {
				i = code_get_u2(p + 1,m);
				nextp = p + 3;
				iswide = false;
			}
			blockend = true;

			NEW_OP_LOAD_ONEWORD(opcode, i);
			break;

		case JAVA_IRETURN:
		case JAVA_LRETURN:
		case JAVA_FRETURN:
		case JAVA_DRETURN:
		case JAVA_ARETURN:
		case JAVA_RETURN:
			blockend = true;
			/* XXX ARETURN will need a flag in the typechecker */
			NEW_OP(opcode);
			break;

		case JAVA_ATHROW:
			blockend = true;
			/* XXX ATHROW will need a flag in the typechecker */
			NEW_OP(opcode);
			break;


		/* table jumps ********************************************************/

		case JAVA_LOOKUPSWITCH: /* XXX */
			{
				s4 num, j;
				s4 *tablep;
#if defined(ENABLE_VERIFIER)
				s4 prevvalue = 0;
#endif

				blockend = true;
				nextp = ALIGN((p + 1), 4);

				CHECK_END_OF_BYTECODE(nextp + 8);

				tablep = (s4 *) (m->jcode + nextp);

				NEW_OP_PREPARE(opcode);
				iptr->dst.lookuptable = (void**) tablep; /* XXX */
				PINC;

				/* default target */

				j =  p + code_get_s4(nextp, m);
				*tablep = j;     /* restore for little endian */
				tablep++;
				nextp += 4;
				CHECK_BYTECODE_INDEX(j);
				block_insert(j);

				/* number of pairs */

				num = code_get_u4(nextp, m);
				*tablep = num;
				tablep++;
				nextp += 4;

				CHECK_END_OF_BYTECODE(nextp + 8 * num);

				for (i = 0; i < num; i++) {
					/* value */

					j = code_get_s4(nextp, m);
					*tablep = j; /* restore for little endian */
					tablep++;
					nextp += 4;

#if defined(ENABLE_VERIFIER)
					/* check if the lookup table is sorted correctly */

					if (i && (j <= prevvalue)) {
						*exceptionptr = new_verifyerror(m, "Unsorted lookup switch");
						return false;
					}
					prevvalue = j;
#endif
					/* target */

					j = p + code_get_s4(nextp,m);
					*tablep = j; /* restore for little endian */
					tablep++;
					nextp += 4;
					CHECK_BYTECODE_INDEX(j);
					block_insert(j);
				}

				break;
			}


		case JAVA_TABLESWITCH: /*XXX*/
			{
				s4 num, j;
				s4 *tablep;

				blockend = true;
				nextp = ALIGN((p + 1), 4);

				CHECK_END_OF_BYTECODE(nextp + 12);

				tablep = (s4 *) (m->jcode + nextp);

				NEW_OP_PREPARE(opcode);
				iptr->dst.targettable = (basicblock **) tablep; /* XXX */
				PINC;

				/* default target */

				j = p + code_get_s4(nextp, m);
				*tablep = j;     /* restore for little endian */
				tablep++;
				nextp += 4;
				CHECK_BYTECODE_INDEX(j);
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

#if defined(ENABLE_VERIFIER)
				if (num < 0) {
					*exceptionptr = new_verifyerror(m,
							"invalid TABLESWITCH: upper bound < lower bound");
					return false;
				}
#endif
				CHECK_END_OF_BYTECODE(nextp + 4 * (num + 1));

				for (i = 0; i <= num; i++) {
					j = p + code_get_s4(nextp,m);
					*tablep = j; /* restore for little endian */
					tablep++;
					nextp += 4;
					CHECK_BYTECODE_INDEX(j);
					block_insert(j);
				}

				break;
			}


		/* load and store of object fields ************************************/

		case JAVA_AASTORE:
			NEW_OP(opcode);
			m->isleafmethod = false;
			break;

		case JAVA_GETSTATIC:
		case JAVA_PUTSTATIC:
		case JAVA_GETFIELD:
		case JAVA_PUTFIELD:
			{
				constant_FMIref  *fr;
				unresolved_field *uf;

				i = code_get_u2(p + 1, m);
				fr = class_getconstant(m->class, i, CONSTANT_Fieldref);
				if (!fr)
					return false;

				NEW_OP_PREPARE(opcode);
				iptr->sx.s23.s3.fmiref = fr;

				/* only with -noverify, otherwise the typechecker does this */

#if defined(ENABLE_VERIFIER)
				if (!opt_verify) {
#endif
					result = resolve_field_lazy(/* XXX */(instruction *)iptr, NULL, m);
					if (result == resolveFailed)
						return false;

					if (result != resolveSucceeded) {
						uf = create_unresolved_field(m->class, m, /* XXX */(instruction *)iptr);

						if (!uf)
							return false;

						/* store the unresolved_field pointer */

						iptr->sx.s23.s3.uf = uf;
						iptr->flags.bits = INS_FLAG_UNRESOLVED;
					}
#if defined(ENABLE_VERIFIER)
				}
#endif
				PINC;
			}
			break;


		/* method invocation **************************************************/

		case JAVA_INVOKESTATIC:
			i = code_get_u2(p + 1, m);
			mr = class_getconstant(m->class, i, CONSTANT_Methodref);
			if (!mr)
				return false;

			md = mr->parseddesc.md;

			if (!md->params)
				if (!descriptor_params_from_paramtypes(md, ACC_STATIC))
					return false;

			goto invoke_method;

		case JAVA_INVOKEINTERFACE:
			i = code_get_u2(p + 1, m);

			mr = class_getconstant(m->class, i,
					CONSTANT_InterfaceMethodref);

			goto invoke_nonstatic_method;

		case JAVA_INVOKESPECIAL:
		case JAVA_INVOKEVIRTUAL:
			i = code_get_u2(p + 1, m);
			mr = class_getconstant(m->class, i, CONSTANT_Methodref);

invoke_nonstatic_method:
			if (!mr)
				return false;

			md = mr->parseddesc.md;

			if (!md->params)
				if (!descriptor_params_from_paramtypes(md, 0))
					return false;

invoke_method:
			m->isleafmethod = false;

			NEW_OP_PREPARE(opcode);
			iptr->sx.s23.s3.fmiref = mr;

			/* only with -noverify, otherwise the typechecker does this */

#if defined(ENABLE_VERIFIER)
			if (!opt_verify) {
#endif
				result = resolve_method_lazy(/* XXX */(instruction *)iptr, NULL, m);
				if (result == resolveFailed)
					return false;

				if (result != resolveSucceeded) {
					um = create_unresolved_method(m->class, m, /* XXX */(instruction *)iptr);

					if (!um)
						return false;

					/* store the unresolved_method pointer */

					iptr->sx.s23.s3.um = um;
					iptr->flags.bits = INS_FLAG_UNRESOLVED;
				}
#if defined(ENABLE_VERIFIER)
			}
#endif
			PINC;
			break;

		/* instructions taking class arguments ********************************/

		case JAVA_NEW:
			i = code_get_u2(p + 1, m);
			cr = (constant_classref *) class_getconstant(m->class, i, CONSTANT_Class);
			if (!cr)
				return false;

			if (!resolve_classref(m, cr, resolveLazy, true, true, &c))
				return false;

			NEW_OP_LOADCONST_CLASSINFO_OR_CLASSREF(c, cr, INS_FLAG_NOCHECK);
			bte = builtintable_get_internal(BUILTIN_new);
			NEW_OP_BUILTIN_CHECK_EXCEPTION(bte);
			s_count++;
			break;

		case JAVA_CHECKCAST:
			i = code_get_u2(p + 1, m);
			cr = (constant_classref *) class_getconstant(m->class, i, CONSTANT_Class);
			if (!cr)
				return false;

			if (!resolve_classref(m, cr, resolveLazy, true, true, &c))
				return false;

			if (cr->name->text[0] == '[') {
				/* array type cast-check */
				flags = INS_FLAG_ARRAY;
				m->isleafmethod = false;
			}
			else {
				/* object type cast-check */
				flags = 0;
			}
			NEW_OP_S3_CLASSINFO_OR_CLASSREF(opcode, c, cr, flags);
			break;

		case JAVA_INSTANCEOF:
			i = code_get_u2(p + 1,m);
			cr = (constant_classref *) class_getconstant(m->class, i, CONSTANT_Class);
			if (!cr)
				return false;

			if (!resolve_classref(m, cr, resolveLazy, true, true, &c))
				return false;

			if (cr->name->text[0] == '[') {
				/* array type cast-check */
				NEW_OP_LOADCONST_CLASSINFO_OR_CLASSREF(c, cr, INS_FLAG_NOCHECK);
				bte = builtintable_get_internal(BUILTIN_arrayinstanceof);
				NEW_OP_BUILTIN_NO_EXCEPTION(bte);
				s_count++;
			}
			else {
				/* object type cast-check */
				NEW_OP_S3_CLASSINFO_OR_CLASSREF(opcode, c, cr, 0 /* flags*/);
			}
			break;

		/* synchronization instructions ***************************************/

		case JAVA_MONITORENTER:
#if defined(ENABLE_THREADS)
			if (checksync) {
				NEW_OP(ICMD_CHECKNULL);
				bte = builtintable_get_internal(BUILTIN_monitorenter);
				NEW_OP_BUILTIN_NO_EXCEPTION(bte);
			}
			else
#endif
				{
					NEW_OP(ICMD_CHECKNULL);
					NEW_OP(ICMD_POP);
				}
			break;

		case JAVA_MONITOREXIT:
#if defined(ENABLE_THREADS)
			if (checksync) {
				bte = builtintable_get_internal(BUILTIN_monitorexit);
				NEW_OP_BUILTIN_NO_EXCEPTION(bte);
			}
			else
#endif
				{
					NEW_OP(ICMD_POP);
				}
			break;

		/* arithmetic instructions they may become builtin functions **********/

		case JAVA_IDIV:
#if !SUPPORT_DIVISION
			bte = builtintable_get_internal(BUILTIN_idiv);
			NEW_OP_BUILTIN_ARITHMETIC(opcode, bte);
#else
			OP(opcode);
#endif
			break;

		case JAVA_IREM:
#if !SUPPORT_DIVISION
			bte = builtintable_get_internal(BUILTIN_irem);
			NEW_OP_BUILTIN_ARITHMETIC(opcode, bte);
#else
			OP(opcode);
#endif
			break;

		case JAVA_LDIV:
#if !(SUPPORT_DIVISION && SUPPORT_LONG && SUPPORT_LONG_DIV)
			bte = builtintable_get_internal(BUILTIN_ldiv);
			NEW_OP_BUILTIN_ARITHMETIC(opcode, bte);
#else
			OP(opcode);
#endif
			break;

		case JAVA_LREM:
#if !(SUPPORT_DIVISION && SUPPORT_LONG && SUPPORT_LONG_DIV)
			bte = builtintable_get_internal(BUILTIN_lrem);
			NEW_OP_BUILTIN_ARITHMETIC(opcode, bte);
#else
			OP(opcode);
#endif
			break;

		case JAVA_FREM:
#if defined(__I386__)
			NEW_OP(opcode);
#else
			bte = builtintable_get_internal(BUILTIN_frem);
			NEW_OP_BUILTIN_NO_EXCEPTION(bte);
#endif
			break;

		case JAVA_DREM:
#if defined(__I386__)
			NEW_OP(opcode);
#else
			bte = builtintable_get_internal(BUILTIN_drem);
			NEW_OP_BUILTIN_NO_EXCEPTION(bte);
#endif
			break;

		case JAVA_F2I:
#if defined(__ALPHA__)
			if (!opt_noieee) {
				bte = builtintable_get_internal(BUILTIN_f2i);
				NEW_OP_BUILTIN_NO_EXCEPTION(bte);
			}
			else
#endif
			{
				NEW_OP(opcode);
			}
			break;

		case JAVA_F2L:
#if defined(__ALPHA__)
			if (!opt_noieee) {
				bte = builtintable_get_internal(BUILTIN_f2l);
				NEW_OP_BUILTIN_NO_EXCEPTION(bte);
			}
			else
#endif
			{
				NEW_OP(opcode);
			}
			break;

		case JAVA_D2I:
#if defined(__ALPHA__)
			if (!opt_noieee) {
				bte = builtintable_get_internal(BUILTIN_d2i);
				NEW_OP_BUILTIN_NO_EXCEPTION(bte);
			}
			else
#endif
			{
				NEW_OP(opcode);
			}
			break;

		case JAVA_D2L:
#if defined(__ALPHA__)
			if (!opt_noieee) {
				bte = builtintable_get_internal(BUILTIN_d2l);
				NEW_OP_BUILTIN_NO_EXCEPTION(bte);
			}
			else
#endif
			{
				NEW_OP(opcode);
			}
			break;

		/* invalid opcodes ****************************************************/

			/* check for invalid opcodes if the verifier is enabled */
#if defined(ENABLE_VERIFIER)
		case JAVA_BREAKPOINT:
			*exceptionptr =
				new_verifyerror(m, "Quick instructions shouldn't appear yet.");
			return false;

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
			return false;
			break;
#endif /* defined(ENABLE_VERIFIER) */

		/* opcodes that don't require translation *****************************/

		default:
			/* straight-forward translation to ICMD */
			NEW_OP(opcode);
			break;

		} /* end switch */

#if defined(ENABLE_VERIFIER)
		/* If WIDE was used correctly, iswide should have been reset by now. */
		if (iswide) {
			*exceptionptr = new_verifyerror(m,
					"Illegal instruction: WIDE before incompatible opcode");
			return false;
		}
#endif /* defined(ENABLE_VERIFIER) */

	} /* end for */

	/*** END OF LOOP **********************************************************/

#if defined(ENABLE_VERIFIER)
	if (p != m->jcodelength) {
		*exceptionptr = new_verifyerror(m,
				"Command-sequence crosses code-boundary");
		return false;
	}

	if (!blockend) {
		*exceptionptr = new_verifyerror(m, "Falling off the end of the code");
		return false;
	}
#endif /* defined(ENABLE_VERIFIER) */

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
			BASICBLOCK_INIT(bptr,m);

			bptr->iinstr = m->instructions;
			/* bptr->icount is set when the next block is allocated */

			bptr++;
			b_count++;
			bptr[-1].next = bptr;
		}

		/* allocate blocks */

		for (p = 0; p < m->jcodelength; p++) {
			if (m->basicblockindex[p] & 1) {
				/* Check if this block starts at the beginning of an          */
				/* instruction.                                               */
#if defined(ENABLE_VERIFIER)
				if (!instructionstart[p]) {
					*exceptionptr = new_verifyerror(m,
						"Branch into middle of instruction");
					return false;
				}
#endif

				/* allocate the block */

				BASICBLOCK_INIT(bptr,m);

				bptr->iinstr = m->instructions + (m->basicblockindex[p] >> 1);
				if (b_count) {
					bptr[-1].icount = bptr->iinstr - bptr[-1].iinstr;
				}
				/* bptr->icount is set when the next block is allocated */

				m->basicblockindex[p] = b_count;

				bptr++;
				b_count++;
				bptr[-1].next = bptr;
			}
		}

		/* set instruction count of last real block */

		if (b_count) {
			bptr[-1].icount = (m->instructions + m->instructioncount) - bptr[-1].iinstr;
		}

		/* allocate additional block at end */

		BASICBLOCK_INIT(bptr,m);

		bptr->instack = bptr->outstack = NULL;
		bptr->indepth = bptr->outdepth = 0;
		bptr->iinstr = NULL;
		bptr->icount = 0;
		bptr->next = NULL;

		/* set basicblock pointers in exception table */

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

		/* XXX activate this if you want to try inlining */
#if 0
		for (i = 0; i < m->exceptiontablelength; ++i) {
			p = m->exceptiontable[i].startpc;
			m->exceptiontable[i].start = m->basicblocks + m->basicblockindex[p];

			p = m->exceptiontable[i].endpc;
			m->exceptiontable[i].end = (p == m->jcodelength) ? (m->basicblocks + m->basicblockcount /*+ 1*/) : (m->basicblocks + m->basicblockindex[p]);

			p = m->exceptiontable[i].handlerpc;
			m->exceptiontable[i].handler = m->basicblocks + m->basicblockindex[p];
	    }
#endif

	}

	/* everything's ok */

	return true;

#if defined(ENABLE_VERIFIER)

throw_unexpected_end_of_bytecode:
	*exceptionptr = new_verifyerror(m, "Unexpected end of bytecode");
	return false;

throw_invalid_bytecode_index:
	*exceptionptr =
		new_verifyerror(m, "Illegal target of branch instruction");
	return false;

throw_illegal_local_variable_number:
	*exceptionptr =
		new_verifyerror(m, "Illegal local variable number");
	return false;

#endif /* ENABLE_VERIFIER */
}


bool parse(jitdata *jd)
{
	methodinfo  *m;
	codegendata *cd;
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

	u1 *instructionstart;       /* 1 for pcs which are valid instr. starts    */

	constant_classref  *cr;
	constant_classref  *compr;
	classinfo          *c;
	builtintable_entry *bte;

	constant_FMIref   *mr;
	methoddesc        *md;
	unresolved_method *um;
	resolve_result_t   result;

	u2 lineindex = 0;
	u2 currentline = 0;
	u2 linepcchange = 0;

	/* get required compiler data */

	m  = jd->m;
	cd = jd->cd;

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

	if (!fillextable(m, 
 	  		&(cd->exceptiontable[cd->exceptiontablelength-1]), 
	  		m->exceptiontable, 
	  		m->exceptiontablelength, 
			&b_count))
	{
		return false;
	}

	s_count = 1 + m->exceptiontablelength; /* initialize stack element counter   */

#if defined(ENABLE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		m->isleafmethod = false;
	}			
#endif

	/* scan all java instructions */
	currentline = 0;
	linepcchange = 0;

	if (m->linenumbercount == 0) {
		lineindex = 0;
	} 
	else {
		linepcchange = m->linenumbers[0].start_pc;
	}

	for (p = 0; p < m->jcodelength; p = nextp) {
	  
		/* mark this position as a valid instruction start */
		instructionstart[p] = 1;
		if (linepcchange == p) {
			if (m->linenumbercount > lineindex) {
next_linenumber:
				currentline = m->linenumbers[lineindex].line_number;
				lineindex++;
				if (lineindex < m->linenumbercount) {
					linepcchange = m->linenumbers[lineindex].start_pc;
					if (linepcchange == p)
						goto next_linenumber;
				}
			}
		}

		/* fetch next opcode  */
fetch_opcode:
		opcode = code_get_u1(p, m);

		m->basicblockindex[p] |= (ipc << 1); /*store intermed cnt*/

		/* some compilers put a JAVA_NOP after a blockend instruction */

		if (blockend && (opcode != JAVA_NOP)) {
			/* start new block */

			block_insert(p);
			blockend = false;
		}

		nextp = p + jcommandsize[opcode];   /* compute next instruction start */

		CHECK_END_OF_BYTECODE(nextp);

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

#if defined(ENABLE_VERIFIER)
			if (i >= m->class->cpcount) {
				*exceptionptr = new_verifyerror(m,
					"Attempt to access constant outside range");
				return false;
			}
#endif

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
					return false;

				/* if not resolved, c == NULL */

				if (c) {
					iptr->target = (void*) 0x02; /* XXX target used temporarily as flag */
					LOADCONST_A(c);
				}
				else {
					iptr->target = (void*) 0x03; /* XXX target used temporarily as flag */
					LOADCONST_A(cr);
				}
				break;

#if defined(ENABLE_VERIFIER)
			default:
				*exceptionptr = new_verifyerror(m,
						"Invalid constant type to push");
				return false;
#endif
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
		case JAVA_FLOAD:
		case JAVA_ALOAD:
			if (!iswide) {
				i = code_get_u1(p + 1,m);
			} 
			else {
				i = code_get_u2(p + 1,m);
				nextp = p + 3;
				iswide = false;
			}
			OP1LOAD_ONEWORD(opcode, i);
			break;

		case JAVA_LLOAD:
		case JAVA_DLOAD:
			if (!iswide) {
				i = code_get_u1(p + 1,m);
			} 
			else {
				i = code_get_u2(p + 1,m);
				nextp = p + 3;
				iswide = false;
			}
			OP1LOAD_TWOWORD(opcode, i);
			break;

		case JAVA_ILOAD_0:
		case JAVA_ILOAD_1:
		case JAVA_ILOAD_2:
		case JAVA_ILOAD_3:
			OP1LOAD_ONEWORD(ICMD_ILOAD, opcode - JAVA_ILOAD_0);
			break;

		case JAVA_LLOAD_0:
		case JAVA_LLOAD_1:
		case JAVA_LLOAD_2:
		case JAVA_LLOAD_3:
			OP1LOAD_TWOWORD(ICMD_LLOAD, opcode - JAVA_LLOAD_0);
			break;

		case JAVA_FLOAD_0:
		case JAVA_FLOAD_1:
		case JAVA_FLOAD_2:
		case JAVA_FLOAD_3:
			OP1LOAD_ONEWORD(ICMD_FLOAD, opcode - JAVA_FLOAD_0);
			break;

		case JAVA_DLOAD_0:
		case JAVA_DLOAD_1:
		case JAVA_DLOAD_2:
		case JAVA_DLOAD_3:
			OP1LOAD_TWOWORD(ICMD_DLOAD, opcode - JAVA_DLOAD_0);
			break;

		case JAVA_ALOAD_0:
		case JAVA_ALOAD_1:
		case JAVA_ALOAD_2:
		case JAVA_ALOAD_3:
			OP1LOAD_ONEWORD(ICMD_ALOAD, opcode - JAVA_ALOAD_0);
			break;

			/* storing stack values into local variables */

		case JAVA_ISTORE:
		case JAVA_FSTORE:
		case JAVA_ASTORE:
			if (!iswide) {
				i = code_get_u1(p + 1,m);
			} 
			else {
				i = code_get_u2(p + 1,m);
				iswide = false;
				nextp = p + 3;
			}
			OP1STORE_ONEWORD(opcode, i);
			break;

		case JAVA_LSTORE:
		case JAVA_DSTORE:
			if (!iswide) {
				i = code_get_u1(p + 1,m);
			} 
			else {
				i = code_get_u2(p + 1,m);
				iswide = false;
				nextp = p + 3;
			}
			OP1STORE_TWOWORD(opcode, i);
			break;

		case JAVA_ISTORE_0:
		case JAVA_ISTORE_1:
		case JAVA_ISTORE_2:
		case JAVA_ISTORE_3:
			OP1STORE_ONEWORD(ICMD_ISTORE, opcode - JAVA_ISTORE_0);
			break;

		case JAVA_LSTORE_0:
		case JAVA_LSTORE_1:
		case JAVA_LSTORE_2:
		case JAVA_LSTORE_3:
			OP1STORE_TWOWORD(ICMD_LSTORE, opcode - JAVA_LSTORE_0);
			break;

		case JAVA_FSTORE_0:
		case JAVA_FSTORE_1:
		case JAVA_FSTORE_2:
		case JAVA_FSTORE_3:
			OP1STORE_ONEWORD(ICMD_FSTORE, opcode - JAVA_FSTORE_0);
			break;

		case JAVA_DSTORE_0:
		case JAVA_DSTORE_1:
		case JAVA_DSTORE_2:
		case JAVA_DSTORE_3:
			OP1STORE_TWOWORD(ICMD_DSTORE, opcode - JAVA_DSTORE_0);
			break;

		case JAVA_ASTORE_0:
		case JAVA_ASTORE_1:
		case JAVA_ASTORE_2:
		case JAVA_ASTORE_3:
			OP1STORE_ONEWORD(ICMD_ASTORE, opcode - JAVA_ASTORE_0);
			break;

		case JAVA_IINC:
			{
				int v;
				
				if (!iswide) {
					i = code_get_u1(p + 1,m);
					v = code_get_s1(p + 2,m);

				} 
				else {
					i = code_get_u2(p + 1,m);
					v = code_get_s2(p + 3,m);
					iswide = false;
					nextp = p + 5;
				}
				INDEX_ONEWORD(i);
				OP2I(opcode, i, v);
			}
			break;

			/* wider index for loading, storing and incrementing */

		case JAVA_WIDE:
			iswide = true;
			p++;
			goto fetch_opcode;

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
#if defined(ENABLE_VERIFIER)
			default:
				*exceptionptr = new_verifyerror(m,
						"Invalid array-type to create");
				return false;
#endif
			}
			BUILTIN(bte, true, NULL, currentline);
			break;

		case JAVA_ANEWARRAY:
			i = code_get_u2(p + 1, m);
			compr = (constant_classref *) class_getconstant(m->class, i, CONSTANT_Class);
			if (!compr)
				return false;

			if (!(cr = class_get_classref_multiarray_of(1, compr)))
				return false;

			if (!resolve_classref(m, cr, resolveLazy, true, true, &c))
				return false;

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
					return false;

				if (!resolve_classref(m, cr, resolveLazy, true, true, &c))
					return false;

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
			CHECK_BYTECODE_INDEX(i);
			block_insert(i);
			blockend = true;
			OP1(opcode, i);
			break;

		case JAVA_GOTO_W:
		case JAVA_JSR_W:
			i = p + code_get_s4(p + 1,m);
			CHECK_BYTECODE_INDEX(i);
			block_insert(i);
			blockend = true;
			OP1(opcode, i);
			break;

		case JAVA_RET:
			if (!iswide) {
				i = code_get_u1(p + 1,m);
			} 
			else {
				i = code_get_u2(p + 1,m);
				nextp = p + 3;
				iswide = false;
			}
			blockend = true;
				
			OP1LOAD_ONEWORD(opcode, i);
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
#if defined(ENABLE_VERIFIER)
				s4 prevvalue = 0;
#endif

				blockend = true;
				nextp = ALIGN((p + 1), 4);

				CHECK_END_OF_BYTECODE(nextp + 8);

				tablep = (s4 *) (m->jcode + nextp);

				OP2A(opcode, 0, tablep, currentline);

				/* default target */

				j =  p + code_get_s4(nextp, m);
				*tablep = j;     /* restore for little endian */
				tablep++;
				nextp += 4;
				CHECK_BYTECODE_INDEX(j);
				block_insert(j);

				/* number of pairs */

				num = code_get_u4(nextp, m);
				*tablep = num;
				tablep++;
				nextp += 4;

				CHECK_END_OF_BYTECODE(nextp + 8 * num);

				for (i = 0; i < num; i++) {
					/* value */

					j = code_get_s4(nextp, m);
					*tablep = j; /* restore for little endian */
					tablep++;
					nextp += 4;

#if defined(ENABLE_VERIFIER)
					/* check if the lookup table is sorted correctly */
					
					if (i && (j <= prevvalue)) {
						*exceptionptr = new_verifyerror(m, "Unsorted lookup switch");
						return false;
					}
					prevvalue = j;
#endif

					/* target */

					j = p + code_get_s4(nextp,m);
					*tablep = j; /* restore for little endian */
					tablep++;
					nextp += 4;
					CHECK_BYTECODE_INDEX(j);
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

				CHECK_END_OF_BYTECODE(nextp + 12);

				tablep = (s4 *) (m->jcode + nextp);

				OP2A(opcode, 0, tablep, currentline);

				/* default target */

				j = p + code_get_s4(nextp, m);
				*tablep = j;     /* restore for little endian */
				tablep++;
				nextp += 4;
				CHECK_BYTECODE_INDEX(j);
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

#if defined(ENABLE_VERIFIER)
				if (num < 0) {
					*exceptionptr = new_verifyerror(m,
							"invalid TABLESWITCH: upper bound < lower bound");
					return false;
				}
#endif

				CHECK_END_OF_BYTECODE(nextp + 4 * (num + 1));

				for (i = 0; i <= num; i++) {
					j = p + code_get_s4(nextp,m);
					*tablep = j; /* restore for little endian */
					tablep++;
					nextp += 4;
					CHECK_BYTECODE_INDEX(j);
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

				i = code_get_u2(p + 1, m);
				fr = class_getconstant(m->class, i,
									   CONSTANT_Fieldref);
				if (!fr)
					return false;

				OP2A_NOINC(opcode, fr->parseddesc.fd->type, fr, currentline);

				/* only with -noverify, otherwise the typechecker does this */

#if defined(ENABLE_VERIFIER)
				if (!opt_verify) {
#endif
					result = resolve_field_lazy(iptr,NULL,m);
					if (result == resolveFailed)
						return false;

					if (result != resolveSucceeded) {
						uf = create_unresolved_field(m->class,
													 m, iptr);

						if (!uf)
							return false;

						/* store the unresolved_field pointer */

						/* XXX this will be changed */
						iptr->val.a = uf;
						iptr->target = (void*) 0x01; /* XXX target temporarily used as flag */
					}
					else {
						iptr->target = NULL;
					}
#if defined(ENABLE_VERIFIER)
				}
				else {
					iptr->target = NULL;
				}
#endif
				PINC;
			}
			break;
				

		/* method invocation **************************************************/

		case JAVA_INVOKESTATIC:
			i = code_get_u2(p + 1, m);
			mr = class_getconstant(m->class, i,
					CONSTANT_Methodref);
			if (!mr)
				return false;

			md = mr->parseddesc.md;

			if (!md->params)
				if (!descriptor_params_from_paramtypes(md, ACC_STATIC))
					return false;

			goto invoke_method;

		case JAVA_INVOKEINTERFACE:
			i = code_get_u2(p + 1, m);
				
			mr = class_getconstant(m->class, i,
					CONSTANT_InterfaceMethodref);

			goto invoke_nonstatic_method;

		case JAVA_INVOKESPECIAL:
		case JAVA_INVOKEVIRTUAL:
			i = code_get_u2(p + 1, m);
			mr = class_getconstant(m->class, i,
					CONSTANT_Methodref);

invoke_nonstatic_method:
			if (!mr)
				return false;

			md = mr->parseddesc.md;

			if (!md->params)
				if (!descriptor_params_from_paramtypes(md, 0))
					return false;

invoke_method:
			m->isleafmethod = false;

			OP2A_NOINC(opcode, 0, mr, currentline);

			/* only with -noverify, otherwise the typechecker does this */

#if defined(ENABLE_VERIFIER)
			if (!opt_verify) {
#endif
				result = resolve_method_lazy(iptr,NULL,m);
				if (result == resolveFailed)
					return false;

				if (result != resolveSucceeded) {
					um = create_unresolved_method(m->class,
							m, iptr);

					if (!um)
						return false;

					/* store the unresolved_method pointer */

					/* XXX this will be changed */
					iptr->val.a = um;
					iptr->target = (void*) 0x01; /* XXX target temporarily used as flag */
				}
				else {
					/* the method could be resolved */
					iptr->target = NULL;
				}
#if defined(ENABLE_VERIFIER)
			}
			else {
				iptr->target = NULL;
			}
#endif
			PINC;
			break;

		/* miscellaneous object operations ************************************/

		case JAVA_NEW:
			i = code_get_u2(p + 1, m);
			cr = (constant_classref *) class_getconstant(m->class, i, CONSTANT_Class);
			if (!cr)
				return false;

			if (!resolve_classref(m, cr, resolveLazy, true, true,
								  &c))
				return false;

			LOADCONST_A_BUILTIN(c, cr);
			bte = builtintable_get_internal(BUILTIN_new);
			BUILTIN(bte, true, NULL, currentline);
			s_count++;
			break;

		case JAVA_CHECKCAST:
			i = code_get_u2(p + 1, m);
			cr = (constant_classref *) class_getconstant(m->class, i, CONSTANT_Class);
			if (!cr)
				return false;

			if (!resolve_classref(m, cr, resolveLazy, true,
								  true, &c))
				return false;

			if (cr->name->text[0] == '[') {
				/* array type cast-check */
				OP2AT(opcode, 0, c, cr, currentline);
				m->isleafmethod = false;

			} 
			else {
				/* object type cast-check */
				OP2AT(opcode, 1, c, cr, currentline);
			}
			break;

		case JAVA_INSTANCEOF:
			i = code_get_u2(p + 1,m);
			cr = (constant_classref *) class_getconstant(m->class, i, CONSTANT_Class);
			if (!cr)
				return false;

			if (!resolve_classref(m, cr, resolveLazy, true, true, &c))
				return false;

			if (cr->name->text[0] == '[') {
				/* array type cast-check */
				LOADCONST_A_BUILTIN(c, cr);
				bte = builtintable_get_internal(BUILTIN_arrayinstanceof);
				BUILTIN(bte, false, NULL, currentline);
				s_count++;

			} 
			else {
				/* object type cast-check */
				OP2AT(opcode, 1, c, cr, currentline);
			}
			break;

		case JAVA_MONITORENTER:
#if defined(ENABLE_THREADS)
			if (checksync) {
				OP(ICMD_CHECKNULL);
				bte = builtintable_get_internal(BUILTIN_monitorenter);
				BUILTIN(bte, false, NULL, currentline);
			} 
			else
#endif
				{
					OP(ICMD_CHECKNULL);
					OP(ICMD_POP);
				}
			break;

		case JAVA_MONITOREXIT:
#if defined(ENABLE_THREADS)
			if (checksync) {
				bte = builtintable_get_internal(BUILTIN_monitorexit);
				BUILTIN(bte, false, NULL, currentline);
			} 
			else
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
			} 
			else
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
			} 
			else 
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
			} 
			else
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
			} 
			else
#endif
				{
					OP(opcode);
				}
			break;

			/* check for invalid opcodes if the verifier is enabled */
#if defined(ENABLE_VERIFIER)
		case JAVA_BREAKPOINT:
			*exceptionptr =
				new_verifyerror(m, "Quick instructions shouldn't appear yet.");
			return false;

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
			return false;
			break;
#endif /* defined(ENABLE_VERIFIER) */

		default:
			/* straight-forward translation to ICMD */
			OP(opcode);
			break;
				
		} /* end switch */

#if defined(ENABLE_VERIFIER)
		/* If WIDE was used correctly, iswide should have been reset by now. */
		if (iswide) {
			*exceptionptr = new_verifyerror(m,
					"Illegal instruction: WIDE before incompatible opcode");
			return false;
		}
#endif /* defined(ENABLE_VERIFIER) */

	} /* end for */

#if defined(ENABLE_VERIFIER)
	if (p != m->jcodelength) {
		*exceptionptr = new_verifyerror(m,
				"Command-sequence crosses code-boundary");
		return false;
	}

	if (!blockend) {
		*exceptionptr = new_verifyerror(m, "Falling off the end of the code");
		return false;
	}
#endif /* defined(ENABLE_VERIFIER) */

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
			BASICBLOCK_INIT(bptr,m);

			bptr->iinstr = m->instructions;
			/* bptr->icount is set when the next block is allocated */

			bptr++;
			b_count++;
			bptr[-1].next = bptr;
		}

		/* allocate blocks */

  		for (p = 0; p < m->jcodelength; p++) { 
			if (m->basicblockindex[p] & 1) {
				/* Check if this block starts at the beginning of an          */
				/* instruction.                                               */
#if defined(ENABLE_VERIFIER)
				if (!instructionstart[p]) {
					*exceptionptr = new_verifyerror(m,
						"Branch into middle of instruction");
					return false;
				}
#endif

				/* allocate the block */

				BASICBLOCK_INIT(bptr,m);

				bptr->iinstr = m->instructions + (m->basicblockindex[p] >> 1);
				if (b_count) {
					bptr[-1].icount = bptr->iinstr - bptr[-1].iinstr;
				}
				/* bptr->icount is set when the next block is allocated */

				m->basicblockindex[p] = b_count;

				bptr++;
				b_count++;
				bptr[-1].next = bptr;
			}
		}

		/* set instruction count of last real block */

		if (b_count) {
			bptr[-1].icount = (m->instructions + m->instructioncount) - bptr[-1].iinstr;
		}

		/* allocate additional block at end */

		BASICBLOCK_INIT(bptr,m);
		
		bptr->instack = bptr->outstack = NULL;
		bptr->indepth = bptr->outdepth = 0;
		bptr->iinstr = NULL;
		bptr->icount = 0;
		bptr->next = NULL;

		/* set basicblock pointers in exception table */

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

		/* XXX activate this if you want to try inlining */
#if 0
		for (i = 0; i < m->exceptiontablelength; ++i) {
			p = m->exceptiontable[i].startpc;
			m->exceptiontable[i].start = m->basicblocks + m->basicblockindex[p];

			p = m->exceptiontable[i].endpc;
			m->exceptiontable[i].end = (p == m->jcodelength) ? (m->basicblocks + m->basicblockcount /*+ 1*/) : (m->basicblocks + m->basicblockindex[p]);

			p = m->exceptiontable[i].handlerpc;
			m->exceptiontable[i].handler = m->basicblocks + m->basicblockindex[p];
	    }
#endif

	}

	/* everything's ok */

	return true;

#if defined(ENABLE_VERIFIER)

throw_unexpected_end_of_bytecode:
	*exceptionptr = new_verifyerror(m, "Unexpected end of bytecode");
	return false;

throw_invalid_bytecode_index:
	*exceptionptr =
		new_verifyerror(m, "Illegal target of branch instruction");
	return false;

throw_illegal_local_variable_number:
	*exceptionptr =
		new_verifyerror(m, "Illegal local variable number");
	return false;
		
#endif /* ENABLE_VERIFIER */
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
