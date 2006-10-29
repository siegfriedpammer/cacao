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

   $Id: parse.c 5859 2006-10-29 23:33:30Z edwin $

*/


#include "config.h"

#include <assert.h>
#include <string.h>

#include "vm/types.h"

#include "mm/memory.h"
#include "native/native.h"

#if defined(ENABLE_THREADS)
# include "threads/native/lock.h"
#endif

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
#include "vm/suck.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/jit.h"
#include "vm/jit/parse.h"
#include "vm/jit/patcher.h"
#include "vm/jit/loop/loop.h"

#define INSTRUCTIONS_INCREMENT  5  /* number of additional instructions to    */
                                   /* allocate if space runs out              */


/* parserdata_t ***************************************************************/

typedef struct parsedata_t parsedata_t;

struct parsedata_t {
	instruction *instructions;          /* instruction array                  */
	s4           instructionslength;    /* length of the instruction array    */
	u1          *instructionstart;
};


/* parse_setup *****************************************************************

   Fills the passed parsedata_t structure.

*******************************************************************************/

static void parse_setup(jitdata *jd, parsedata_t *pd)
{
	methodinfo *m;

	/* get required compiler data */

	m = jd->m;

	/* Allocate instruction array and block index table (1 additional
	   for end ipc). */

	jd->basicblockindex = DMNEW(s4, m->jcodelength + 1);
	pd->instructionstart = DMNEW(u1, m->jcodelength + 1);

	MZERO(jd->basicblockindex, s4, m->jcodelength + 1);
	MZERO(pd->instructionstart, u1, m->jcodelength + 1);

	/* Set the length of the instruction array.  We simply add 5 more
	   instruction, as this seems to be a reasonable value. */

	pd->instructionslength = m->jcodelength + 1;

	/* allocate the instruction array */

	pd->instructions = DMNEW(instruction, pd->instructionslength);

	/* Zero the intermediate instructions array so we don't have any
	   invalid pointers in it if we cannot finish stack_analyse(). */

	MZERO(pd->instructions, instruction, pd->instructionslength);
}


/* parse_realloc_instructions **************************************************

   Reallocate the instructions array so there is room for at least N 
   additional instructions.

   RETURN VALUE:
       the new value for iptr

*******************************************************************************/

static instruction *parse_realloc_instructions(parsedata_t *pd, s4 ipc, s4 n)
{
	/* increase the size of the instruction array */

	pd->instructionslength += (n + INSTRUCTIONS_INCREMENT);

	/* reallocate the array */

	pd->instructions = DMREALLOC(pd->instructions, instruction, ipc,
								 pd->instructionslength);

	/* return the iptr */

	return pd->instructions + ipc;
}


/* parse_mark_exception_boundaries *********************************************

   Mark exception handlers and the boundaries of the handled regions as
   basic block boundaries.

   IN:
       jd...............current jitdata

   RETURN VALUE:
       >= 0.............the number of new basic blocks marked
	   -1...............an exception has been thrown

*******************************************************************************/

static int parse_mark_exception_boundaries(jitdata *jd)
{
	s4                   b_count;
	s4                   pc;
	s4                   i;
	s4                   len;
	raw_exception_entry *rex;
	methodinfo          *m;

	m = jd->m;
	
	len = m->rawexceptiontablelength;

	if (len == 0)
		return 0;

	b_count = 0;
	rex = m->rawexceptiontable;

	for (i = 0; i < len; ++i, ++rex) {

		/* the start of the handled region becomes a basic block start */

   		pc = rex->startpc;
		CHECK_BYTECODE_INDEX(pc);
		MARK_BASICBLOCK(pc);
		
		pc = rex->endpc; /* see JVM Spec 4.7.3 */
		CHECK_BYTECODE_INDEX_EXCLUSIVE(pc);

		/* check that the range is valid */

#if defined(ENABLE_VERIFIER)
		if (pc <= rex->startpc) {
			exceptions_throw_verifyerror(m,
				"Invalid exception handler range");
			return -1;
		}
#endif
		
		/* end of handled region becomes a basic block boundary  */
		/* (If it is the bytecode end, we'll use the special     */
		/* end block that is created anyway.)                    */

		if (pc < m->jcodelength)
			MARK_BASICBLOCK(pc);

		/* the start of the handler becomes a basic block start  */

		pc = rex->handlerpc;
		CHECK_BYTECODE_INDEX(pc);
		MARK_BASICBLOCK(pc);
	}

	/* everything ok */

	return b_count;

#if defined(ENABLE_VERIFIER)
throw_invalid_bytecode_index:
	exceptions_throw_verifyerror(m,
								 "Illegal bytecode index in exception table");
	return -1;
#endif
}


/* parse_resolve_exception_table ***********************************************

   Enter the exception handlers and their ranges, resolved to basicblock *s,
   in the jitdata.

   IN:
       jd...............current jitdata

   RETURN VALUE:
	   true.............everything ok
	   false............an exception has been thrown

*******************************************************************************/

static bool parse_resolve_exception_table(jitdata *jd)
{
	methodinfo          *m;
	raw_exception_entry *rex;
	exception_entry     *ex;
	s4                   i;
	s4                   len;
	classinfo           *exclass;

	m = jd->m;

	len = m->rawexceptiontablelength;

	/* common case: no handler entries */

	if (len == 0)
		return true;

	/* allocate the exception table */

	jd->exceptiontablelength = len;
	jd->exceptiontable = DMNEW(exception_entry, len + 1); /* XXX why +1? */

	/* copy and resolve the entries */

	ex = jd->exceptiontable;
	rex = m->rawexceptiontable;

	for (i = 0; i < len; ++i, ++rex, ++ex) {
		/* resolve instruction indices to basic blocks */

		ex->start   = BLOCK_OF(rex->startpc);
		ex->end     = BLOCK_OF(rex->endpc);
		ex->handler = BLOCK_OF(rex->handlerpc);

		/* lazily resolve the catchtype */

		if (rex->catchtype.any != NULL) {
			if (!resolve_classref_or_classinfo(m,
											   rex->catchtype,
											   resolveLazy, true, false,
											   &exclass))
				return false;

			/* if resolved, enter the result of resolution in the table */

			if (exclass != NULL)
				rex->catchtype.cls = exclass;
		}

		ex->catchtype = rex->catchtype;
		ex->next = NULL;   /* set by loop analysis */
		ex->down = ex + 1; /* link to next exception entry */
	}

	/* terminate the ->down linked list */

	assert(ex != jd->exceptiontable);
	ex[-1].down = NULL;

	return true;
}


/*******************************************************************************

	function 'parse' scans the JavaVM code and generates intermediate code

	During parsing the block index table is used to store at bit pos 0
	a flag which marks basic block starts and at position 1 to 31 the
	intermediate instruction index. After parsing the block index table
	is scanned, for marked positions a block is generated and the block
	number is stored in the block index table.

*******************************************************************************/

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

bool parse(jitdata *jd)
{
	methodinfo  *m;                     /* method being parsed                */
	codeinfo    *code;
	parsedata_t  pd;
	instruction *iptr;                  /* current ptr into instruction array */
	s4           ipc;                   /* intermediate instruction counter   */
	s4           p;                     /* java instruction counter           */
	s4           nextp;                 /* start of next java instruction     */
	s4           opcode;                /* java opcode                        */
	s4           i;
	s4           j;
	int  b_count;               /* basic block counter                      */
	int  s_count = 0;           /* stack element counter                    */
	bool blockend = false;      /* true if basic block end has been reached */
	bool iswide = false;        /* true if last instruction was a wide      */
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
	basicblock         *bptr;

 	int                *local_map; /* local pointer to renaming structore     */
	                               /* is assigned to rd->local_map at the end */
	/* get required compiler data */

	m    = jd->m;
	code = jd->code;

	/* allocate buffers for local variable renaming */
	local_map = DMNEW(int, m->maxlocals * 5);
	for (i = 0; i < m->maxlocals; i++) {
		local_map[i * 5 + 0] = 0;
		local_map[i * 5 + 1] = 0;
		local_map[i * 5 + 2] = 0;
		local_map[i * 5 + 3] = 0;
		local_map[i * 5 + 4] = 0;
	}

 	/* initialize the parse data structures */
  
 	parse_setup(jd, &pd);
  
 	/* initialize local variables */
  
 	iptr = pd.instructions;
 	ipc  = 0;
  
	/* mark basic block boundaries for exception table */

	b_count = parse_mark_exception_boundaries(jd);
	if (b_count < 0)
		return false;

	/* initialize stack element counter */

	s_count = 1 + m->rawexceptiontablelength;

#if defined(ENABLE_THREADS)
	if (checksync && (m->flags & ACC_SYNCHRONIZED))
		jd->isleafmethod = false;
#endif

	/* setup line number info */

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

		pd.instructionstart[p] = 1;

		/* change the current line number, if necessary */

		/* XXX rewrite this using pointer arithmetic */

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

fetch_opcode:
		/* fetch next opcode  */	

		opcode = SUCK_BE_U1(m->jcode + p);

		/* some compilers put a JAVA_NOP after a blockend instruction */

		if (blockend && (opcode != JAVA_NOP)) {
			/* start new block */

			MARK_BASICBLOCK(p);
			blockend = false;
		}

		/* We need a NOP as last instruction in each basic block
		   for basic block reordering (may be replaced with a GOTO
		   later). */

		if (jd->basicblockindex[p] & 1) {
			INSTRUCTIONS_CHECK(1);
			OP(ICMD_NOP);
		}

		/* store intermediate instruction count (bit 0 mark block starts) */

		jd->basicblockindex[p] |= (ipc << 1);

		/* compute next instruction start */

		nextp = p + jcommandsize[opcode];

		CHECK_END_OF_BYTECODE(nextp);

		/* add stack elements produced by this instruction */

		s_count += stackreq[opcode];

		/* We check here for the space of 1 instruction in the
		   instruction array.  If an opcode is converted to more than
		   1 instruction, this is checked in the corresponding
		   case. */

		INSTRUCTIONS_CHECK(1);

		/* translate this bytecode instruction */
		switch (opcode) {

		case JAVA_NOP:
			break;

		/* pushing constants onto the stack ***********************************/

		case JAVA_BIPUSH:
			OP_LOADCONST_I(SUCK_BE_S1(m->jcode + p + 1));
			break;

		case JAVA_SIPUSH:
			OP_LOADCONST_I(SUCK_BE_S2(m->jcode + p + 1));
			break;

		case JAVA_LDC1:
			i = SUCK_BE_U1(m->jcode + p + 1);
			goto pushconstantitem;

		case JAVA_LDC2:
		case JAVA_LDC2W:
			i = SUCK_BE_U2(m->jcode + p + 1);

		pushconstantitem:

#if defined(ENABLE_VERIFIER)
			if (i >= m->class->cpcount) {
				exceptions_throw_verifyerror(m,
					"Attempt to access constant outside range");
				return false;
			}
#endif

			switch (m->class->cptags[i]) {
			case CONSTANT_Integer:
				OP_LOADCONST_I(((constant_integer *) (m->class->cpinfos[i]))->value);
				break;
			case CONSTANT_Long:
				OP_LOADCONST_L(((constant_long *) (m->class->cpinfos[i]))->value);
				break;
			case CONSTANT_Float:
				OP_LOADCONST_F(((constant_float *) (m->class->cpinfos[i]))->value);
				break;
			case CONSTANT_Double:
				OP_LOADCONST_D(((constant_double *) (m->class->cpinfos[i]))->value);
				break;
			case CONSTANT_String:
				OP_LOADCONST_STRING(literalstring_new((utf *) (m->class->cpinfos[i])));
				break;
			case CONSTANT_Class:
				cr = (constant_classref *) (m->class->cpinfos[i]);

				if (!resolve_classref(m, cr, resolveLazy, true, true, &c))
					return false;

				/* if not resolved, c == NULL */

				OP_LOADCONST_CLASSINFO_OR_CLASSREF_CHECK(c, cr);

				break;

#if defined(ENABLE_VERIFIER)
			default:
				exceptions_throw_verifyerror(m,
						"Invalid constant type to push");
				return false;
#endif
			}
			break;

		case JAVA_ACONST_NULL:
			OP_LOADCONST_NULL();
			break;

		case JAVA_ICONST_M1:
		case JAVA_ICONST_0:
		case JAVA_ICONST_1:
		case JAVA_ICONST_2:
		case JAVA_ICONST_3:
		case JAVA_ICONST_4:
		case JAVA_ICONST_5:
			OP_LOADCONST_I(opcode - JAVA_ICONST_0);
			break;

		case JAVA_LCONST_0:
		case JAVA_LCONST_1:
			OP_LOADCONST_L(opcode - JAVA_LCONST_0);
			break;

		case JAVA_FCONST_0:
		case JAVA_FCONST_1:
		case JAVA_FCONST_2:
			OP_LOADCONST_F(opcode - JAVA_FCONST_0);
			break;

		case JAVA_DCONST_0:
		case JAVA_DCONST_1:
			OP_LOADCONST_D(opcode - JAVA_DCONST_0);
			break;

		/* stack operations ***************************************************/

		/* We need space for additional ICMDs so we can translate these       */
		/* instructions to sequences of ICMD_COPY and ICMD_MOVE instructions. */

		case JAVA_DUP_X1:
			INSTRUCTIONS_CHECK(4);
			OP(opcode);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			break;

		case JAVA_DUP_X2:
			INSTRUCTIONS_CHECK(6);
			OP(opcode);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			break;

		case JAVA_DUP2:
			INSTRUCTIONS_CHECK(2);
			OP(opcode);
			OP(ICMD_NOP);
			break;

		case JAVA_DUP2_X1:
			INSTRUCTIONS_CHECK(7);
			OP(opcode);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			break;

		case JAVA_DUP2_X2:
			INSTRUCTIONS_CHECK(9);
			OP(opcode);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			break;

		case JAVA_SWAP:
			INSTRUCTIONS_CHECK(3);
			OP(opcode);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			break;

		/* local variable access instructions *********************************/

		case JAVA_ILOAD:
		case JAVA_FLOAD:
		case JAVA_ALOAD:
			if (iswide == false) {
				i = SUCK_BE_U1(m->jcode + p + 1);
			}
			else {
				i = SUCK_BE_U2(m->jcode + p + 1);
				nextp = p + 3;
				iswide = false;
			}
			OP_LOAD_ONEWORD(opcode, i, opcode - JAVA_ILOAD);
			break;

		case JAVA_LLOAD:
		case JAVA_DLOAD:
			if (iswide == false) {
				i = SUCK_BE_U1(m->jcode + p + 1);
			}
			else {
				i = SUCK_BE_U2(m->jcode + p + 1);
				nextp = p + 3;
				iswide = false;
			}
			OP_LOAD_TWOWORD(opcode, i, opcode - JAVA_ILOAD);
			break;

		case JAVA_ILOAD_0:
		case JAVA_ILOAD_1:
		case JAVA_ILOAD_2:
		case JAVA_ILOAD_3:
			OP_LOAD_ONEWORD(ICMD_ILOAD, opcode - JAVA_ILOAD_0, TYPE_INT);
			break;

		case JAVA_LLOAD_0:
		case JAVA_LLOAD_1:
		case JAVA_LLOAD_2:
		case JAVA_LLOAD_3:
			OP_LOAD_TWOWORD(ICMD_LLOAD, opcode - JAVA_LLOAD_0, TYPE_LNG);
			break;

		case JAVA_FLOAD_0:
		case JAVA_FLOAD_1:
		case JAVA_FLOAD_2:
		case JAVA_FLOAD_3:
			OP_LOAD_ONEWORD(ICMD_FLOAD, opcode - JAVA_FLOAD_0, TYPE_FLT);
			break;

		case JAVA_DLOAD_0:
		case JAVA_DLOAD_1:
		case JAVA_DLOAD_2:
		case JAVA_DLOAD_3:
			OP_LOAD_TWOWORD(ICMD_DLOAD, opcode - JAVA_DLOAD_0, TYPE_DBL);
			break;

		case JAVA_ALOAD_0:
		case JAVA_ALOAD_1:
		case JAVA_ALOAD_2:
		case JAVA_ALOAD_3:
			OP_LOAD_ONEWORD(ICMD_ALOAD, opcode - JAVA_ALOAD_0, TYPE_ADR);
			break;

		case JAVA_ISTORE:
		case JAVA_FSTORE:
		case JAVA_ASTORE:
			if (iswide == false) {
				i = SUCK_BE_U1(m->jcode + p + 1);
			}
			else {
				i = SUCK_BE_U2(m->jcode + p + 1);
				iswide = false;
				nextp = p + 3;
			}
			OP_STORE_ONEWORD(opcode, i, opcode - JAVA_ISTORE);
			break;

		case JAVA_LSTORE:
		case JAVA_DSTORE:
			if (iswide == false) {
				i = SUCK_BE_U1(m->jcode + p + 1);
			}
			else {
				i = SUCK_BE_U2(m->jcode + p + 1);
				iswide = false;
				nextp = p + 3;
			}
			OP_STORE_TWOWORD(opcode, i, opcode - JAVA_ISTORE);
			break;

		case JAVA_ISTORE_0:
		case JAVA_ISTORE_1:
		case JAVA_ISTORE_2:
		case JAVA_ISTORE_3:
			OP_STORE_ONEWORD(ICMD_ISTORE, opcode - JAVA_ISTORE_0, TYPE_INT);
			break;

		case JAVA_LSTORE_0:
		case JAVA_LSTORE_1:
		case JAVA_LSTORE_2:
		case JAVA_LSTORE_3:
			OP_STORE_TWOWORD(ICMD_LSTORE, opcode - JAVA_LSTORE_0, TYPE_LNG);
			break;

		case JAVA_FSTORE_0:
		case JAVA_FSTORE_1:
		case JAVA_FSTORE_2:
		case JAVA_FSTORE_3:
			OP_STORE_ONEWORD(ICMD_FSTORE, opcode - JAVA_FSTORE_0, TYPE_FLT);
			break;

		case JAVA_DSTORE_0:
		case JAVA_DSTORE_1:
		case JAVA_DSTORE_2:
		case JAVA_DSTORE_3:
			OP_STORE_TWOWORD(ICMD_DSTORE, opcode - JAVA_DSTORE_0, TYPE_DBL);
			break;

		case JAVA_ASTORE_0:
		case JAVA_ASTORE_1:
		case JAVA_ASTORE_2:
		case JAVA_ASTORE_3:
			OP_STORE_ONEWORD(ICMD_ASTORE, opcode - JAVA_ASTORE_0, TYPE_ADR);
			break;

		case JAVA_IINC:
			{
				int v;

				if (iswide == false) {
					i = SUCK_BE_U1(m->jcode + p + 1);
					v = SUCK_BE_S1(m->jcode + p + 2);

				}
				else {
					i = SUCK_BE_U2(m->jcode + p + 1);
					v = SUCK_BE_S2(m->jcode + p + 3);
					iswide = false;
					nextp = p + 5;
				}
				INDEX_ONEWORD(i);
				LOCALTYPE_USED(i, TYPE_INT);
				OP_LOCALINDEX_I(opcode, i, v);
			}
			break;

		/* wider index for loading, storing and incrementing ******************/

		case JAVA_WIDE:
			iswide = true;
			p++;
			goto fetch_opcode;

		/* managing arrays ****************************************************/

		case JAVA_NEWARRAY:
			switch (SUCK_BE_S1(m->jcode + p + 1)) {
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
				exceptions_throw_verifyerror(m, "Invalid array-type to create");
				return false;
#endif
			}
			OP_BUILTIN_CHECK_EXCEPTION(bte);
			break;

		case JAVA_ANEWARRAY:
			i = SUCK_BE_U2(m->jcode + p + 1);
			compr = (constant_classref *) class_getconstant(m->class, i, CONSTANT_Class);
			if (!compr)
				return false;

			if (!(cr = class_get_classref_multiarray_of(1, compr)))
				return false;

			if (!resolve_classref(m, cr, resolveLazy, true, true, &c))
				return false;

			INSTRUCTIONS_CHECK(2);
			OP_LOADCONST_CLASSINFO_OR_CLASSREF_NOCHECK(c, cr);
			bte = builtintable_get_internal(BUILTIN_newarray);
			OP_BUILTIN_CHECK_EXCEPTION(bte);
			s_count++;
			break;

		case JAVA_MULTIANEWARRAY:
			jd->isleafmethod = false;
			i = SUCK_BE_U2(m->jcode + p + 1);
 			j = SUCK_BE_U1(m->jcode + p + 3);
  
 			cr = (constant_classref *) class_getconstant(m->class, i, CONSTANT_Class);
 			if (cr == NULL)
 				return false;
  
 			if (!resolve_classref(m, cr, resolveLazy, true, true, &c))
 				return false;
  
 			/* if unresolved, c == NULL */
  
 			iptr->s1.argcount = j;
 			OP_S3_CLASSINFO_OR_CLASSREF(opcode, c, cr, 0 /* flags */);
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
			i = p + SUCK_BE_S2(m->jcode + p + 1);
			CHECK_BYTECODE_INDEX(i);
			MARK_BASICBLOCK(i);
			blockend = true;
			OP_INSINDEX(opcode, i);
			break;

		case JAVA_GOTO_W:
			i = p + SUCK_BE_S4(m->jcode + p + 1);
			CHECK_BYTECODE_INDEX(i);
			MARK_BASICBLOCK(i);
			blockend = true;
			OP_INSINDEX(opcode, i);
			break;

		case JAVA_JSR:
			i = p + SUCK_BE_S2(m->jcode + p + 1);
jsr_tail:
			CHECK_BYTECODE_INDEX(i);
			MARK_BASICBLOCK(i);
			blockend = true;
			OP_PREPARE_ZEROFLAGS(JAVA_JSR);
			iptr->sx.s23.s3.jsrtarget.insindex = i;
			PINC;
			break;

		case JAVA_JSR_W:
			i = p + SUCK_BE_S4(m->jcode + p + 1);
			goto jsr_tail;

		case JAVA_RET:
			if (iswide == false) {
				i = SUCK_BE_U1(m->jcode + p + 1);
			}
			else {
				i = SUCK_BE_U2(m->jcode + p + 1);
				nextp = p + 3;
				iswide = false;
			}
			blockend = true;

			OP_LOAD_ONEWORD(opcode, i, TYPE_ADR);
			break;

		case JAVA_IRETURN:
		case JAVA_LRETURN:
		case JAVA_FRETURN:
		case JAVA_DRETURN:
		case JAVA_ARETURN:
		case JAVA_RETURN:
			blockend = true;
			/* XXX ARETURN will need a flag in the typechecker */
			OP(opcode);
			break;

		case JAVA_ATHROW:
			blockend = true;
			/* XXX ATHROW will need a flag in the typechecker */
			OP(opcode);
			break;


		/* table jumps ********************************************************/

		case JAVA_LOOKUPSWITCH:
			{
				s4 num, j;
				lookup_target_t *lookup;
#if defined(ENABLE_VERIFIER)
				s4 prevvalue = 0;
#endif
				blockend = true;
				nextp = ALIGN((p + 1), 4);

				CHECK_END_OF_BYTECODE(nextp + 8);

				OP_PREPARE_ZEROFLAGS(opcode);

				/* default target */

				j = p + SUCK_BE_S4(m->jcode + nextp);
				iptr->sx.s23.s3.lookupdefault.insindex = j;
				nextp += 4;
				CHECK_BYTECODE_INDEX(j);
				MARK_BASICBLOCK(j);

				/* number of pairs */

				num = SUCK_BE_U4(m->jcode + nextp);
				iptr->sx.s23.s2.lookupcount = num;
				nextp += 4;

				/* allocate the intermediate code table */

				lookup = DMNEW(lookup_target_t, num);
				iptr->dst.lookup = lookup;

				/* iterate over the lookup table */

				CHECK_END_OF_BYTECODE(nextp + 8 * num);

				for (i = 0; i < num; i++) {
					/* value */

					j = SUCK_BE_S4(m->jcode + nextp);
					lookup->value = j;

					nextp += 4;

#if defined(ENABLE_VERIFIER)
					/* check if the lookup table is sorted correctly */

					if (i && (j <= prevvalue)) {
						exceptions_throw_verifyerror(m, "Unsorted lookup switch");
						return false;
					}
					prevvalue = j;
#endif
					/* target */

					j = p + SUCK_BE_S4(m->jcode + nextp);
					lookup->target.insindex = j;
					lookup++;
					nextp += 4;
					CHECK_BYTECODE_INDEX(j);
					MARK_BASICBLOCK(j);
				}

				PINC;
				break;
			}


		case JAVA_TABLESWITCH:
			{
				s4 num, j;
				s4 deftarget;
				branch_target_t *table;

				blockend = true;
				nextp = ALIGN((p + 1), 4);

				CHECK_END_OF_BYTECODE(nextp + 12);

				OP_PREPARE_ZEROFLAGS(opcode);

				/* default target */

				deftarget = p + SUCK_BE_S4(m->jcode + nextp);
				nextp += 4;
				CHECK_BYTECODE_INDEX(deftarget);
				MARK_BASICBLOCK(deftarget);

				/* lower bound */

				j = SUCK_BE_S4(m->jcode + nextp);
				iptr->sx.s23.s2.tablelow = j;
				nextp += 4;

				/* upper bound */

				num = SUCK_BE_S4(m->jcode + nextp);
				iptr->sx.s23.s3.tablehigh = num;
				nextp += 4;

				/* calculate the number of table entries */

				num = num - j + 1;

#if defined(ENABLE_VERIFIER)
				if (num < 1) {
					exceptions_throw_verifyerror(m,
							"invalid TABLESWITCH: upper bound < lower bound");
					return false;
				}
#endif
				/* create the intermediate code table */
				/* the first entry is the default target */

				table = DMNEW(branch_target_t, 1 + num);
				iptr->dst.table = table;
				(table++)->insindex = deftarget;

				/* iterate over the target table */

				CHECK_END_OF_BYTECODE(nextp + 4 * num);

				for (i = 0; i < num; i++) {
					j = p + SUCK_BE_S4(m->jcode + nextp);
					(table++)->insindex = j;
					nextp += 4;
					CHECK_BYTECODE_INDEX(j);
					MARK_BASICBLOCK(j);
				}

				PINC;
				break;
			}


		/* load and store of object fields ************************************/

		case JAVA_AASTORE:
			OP(opcode);
			jd->isleafmethod = false;
			break;

		case JAVA_GETSTATIC:
		case JAVA_PUTSTATIC:
		case JAVA_GETFIELD:
		case JAVA_PUTFIELD:
			{
				constant_FMIref  *fr;
				unresolved_field *uf;

				i = SUCK_BE_U2(m->jcode + p + 1);
				fr = class_getconstant(m->class, i, CONSTANT_Fieldref);
				if (!fr)
					return false;

				OP_PREPARE_ZEROFLAGS(opcode);
				iptr->sx.s23.s3.fmiref = fr;

				/* only with -noverify, otherwise the typechecker does this */

#if defined(ENABLE_VERIFIER)
				if (!JITDATA_HAS_FLAG_VERIFY(jd)) {
#endif
					result = resolve_field_lazy(m, fr);
					if (result == resolveFailed)
						return false;

					if (result != resolveSucceeded) {
						uf = resolve_create_unresolved_field(m->class, m, iptr);

						if (uf == NULL)
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
			i = SUCK_BE_U2(m->jcode + p + 1);
			mr = class_getconstant(m->class, i, CONSTANT_Methodref);

			if (mr == NULL)
				return false;

			md = mr->parseddesc.md;

			if (md->params == NULL)
				if (!descriptor_params_from_paramtypes(md, ACC_STATIC))
					return false;

			goto invoke_method;

		case JAVA_INVOKEINTERFACE:
			i = SUCK_BE_U2(m->jcode + p + 1);

			mr = class_getconstant(m->class, i, CONSTANT_InterfaceMethodref);

			goto invoke_nonstatic_method;

		case JAVA_INVOKESPECIAL:
		case JAVA_INVOKEVIRTUAL:
			i = SUCK_BE_U2(m->jcode + p + 1);
			mr = class_getconstant(m->class, i, CONSTANT_Methodref);

invoke_nonstatic_method:
			if (mr == NULL)
				return false;

			md = mr->parseddesc.md;

			if (md->params == NULL)
				if (!descriptor_params_from_paramtypes(md, 0))
					return false;

invoke_method:
			jd->isleafmethod = false;

			OP_PREPARE_ZEROFLAGS(opcode);
			iptr->sx.s23.s3.fmiref = mr;

			/* only with -noverify, otherwise the typechecker does this */

#if defined(ENABLE_VERIFIER)
			if (!JITDATA_HAS_FLAG_VERIFY(jd)) {
#endif
				result = resolve_method_lazy(m, mr, 
						(opcode == JAVA_INVOKESPECIAL));
				if (result == resolveFailed)
					return false;

				if (result == resolveSucceeded) {
					methodinfo *mi = iptr->sx.s23.s3.fmiref->p.method;

					/* if this call is monomorphic, turn it into an INVOKESPECIAL */

					assert(IS_FMIREF_RESOLVED(iptr->sx.s23.s3.fmiref));

					if ((iptr->opc == ICMD_INVOKEVIRTUAL)
						&& (mi->flags & (ACC_FINAL | ACC_PRIVATE)))
					{
						iptr->opc = ICMD_INVOKESPECIAL;
					}
				}
				else {
					um = resolve_create_unresolved_method(m->class, m, mr,
							(opcode == JAVA_INVOKESTATIC),
							(opcode == JAVA_INVOKESPECIAL));

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
			i = SUCK_BE_U2(m->jcode + p + 1);
			cr = (constant_classref *) class_getconstant(m->class, i, CONSTANT_Class);
			if (!cr)
				return false;

			if (!resolve_classref(m, cr, resolveLazy, true, true, &c))
				return false;

			INSTRUCTIONS_CHECK(2);
			OP_LOADCONST_CLASSINFO_OR_CLASSREF_NOCHECK(c, cr);
			bte = builtintable_get_internal(BUILTIN_new);
			OP_BUILTIN_CHECK_EXCEPTION(bte);
			s_count++;
			break;

		case JAVA_CHECKCAST:
			i = SUCK_BE_U2(m->jcode + p + 1);
			cr = (constant_classref *) class_getconstant(m->class, i, CONSTANT_Class);
			if (cr == NULL)
				return false;

			if (!resolve_classref(m, cr, resolveLazy, true, true, &c))
				return false;

			if (cr->name->text[0] == '[') {
				/* array type cast-check */
				flags = INS_FLAG_ARRAY;
				jd->isleafmethod = false;
			}
			else {
				/* object type cast-check */
				flags = 0;
			}
			OP_S3_CLASSINFO_OR_CLASSREF(opcode, c, cr, flags);
			break;

		case JAVA_INSTANCEOF:
			i = SUCK_BE_U2(m->jcode + p + 1);
			cr = (constant_classref *) class_getconstant(m->class, i, CONSTANT_Class);
			if (cr == NULL)
				return false;

			if (!resolve_classref(m, cr, resolveLazy, true, true, &c))
				return false;

			if (cr->name->text[0] == '[') {
				/* array type cast-check */
				INSTRUCTIONS_CHECK(2);
				OP_LOADCONST_CLASSINFO_OR_CLASSREF_NOCHECK(c, cr);
				bte = builtintable_get_internal(BUILTIN_arrayinstanceof);
				OP_BUILTIN_NO_EXCEPTION(bte);
				s_count++;
			}
			else {
				/* object type cast-check */
				OP_S3_CLASSINFO_OR_CLASSREF(opcode, c, cr, 0 /* flags*/);
			}
			break;

		/* synchronization instructions ***************************************/

		case JAVA_MONITORENTER:
#if defined(ENABLE_THREADS)
			if (checksync) {
				bte = builtintable_get_internal(LOCK_monitor_enter);
				OP_BUILTIN_CHECK_EXCEPTION(bte);
			}
			else
#endif
			{
				OP(ICMD_CHECKNULL_POP);
			}
			break;

		case JAVA_MONITOREXIT:
#if defined(ENABLE_THREADS)
			if (checksync) {
				bte = builtintable_get_internal(LOCK_monitor_exit);
				OP_BUILTIN_CHECK_EXCEPTION(bte);
			}
			else
#endif
			{
				OP(ICMD_CHECKNULL_POP);
			}
			break;

		/* arithmetic instructions that may become builtin functions **********/

		case JAVA_IDIV:
#if !SUPPORT_DIVISION
			bte = builtintable_get_internal(BUILTIN_idiv);
			OP_BUILTIN_ARITHMETIC(opcode, bte);
#else
			OP(opcode);
#endif
			break;

		case JAVA_IREM:
#if !SUPPORT_DIVISION
			bte = builtintable_get_internal(BUILTIN_irem);
			OP_BUILTIN_ARITHMETIC(opcode, bte);
#else
			OP(opcode);
#endif
			break;

		case JAVA_LDIV:
#if !(SUPPORT_DIVISION && SUPPORT_LONG && SUPPORT_LONG_DIV)
			bte = builtintable_get_internal(BUILTIN_ldiv);
			OP_BUILTIN_ARITHMETIC(opcode, bte);
#else
			OP(opcode);
#endif
			break;

		case JAVA_LREM:
#if !(SUPPORT_DIVISION && SUPPORT_LONG && SUPPORT_LONG_DIV)
			bte = builtintable_get_internal(BUILTIN_lrem);
			OP_BUILTIN_ARITHMETIC(opcode, bte);
#else
			OP(opcode);
#endif
			break;

		case JAVA_FREM:
#if defined(__I386__)
			OP(opcode);
#else
			bte = builtintable_get_internal(BUILTIN_frem);
			OP_BUILTIN_NO_EXCEPTION(bte);
#endif
			break;

		case JAVA_DREM:
#if defined(__I386__)
			OP(opcode);
#else
			bte = builtintable_get_internal(BUILTIN_drem);
			OP_BUILTIN_NO_EXCEPTION(bte);
#endif
			break;

		case JAVA_F2I:
#if defined(__ALPHA__)
			if (!opt_noieee) {
				bte = builtintable_get_internal(BUILTIN_f2i);
				OP_BUILTIN_NO_EXCEPTION(bte);
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
				OP_BUILTIN_NO_EXCEPTION(bte);
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
				OP_BUILTIN_NO_EXCEPTION(bte);
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
				OP_BUILTIN_NO_EXCEPTION(bte);
			}
			else
#endif
			{
				OP(opcode);
			}
			break;

		/* invalid opcodes ****************************************************/

			/* check for invalid opcodes if the verifier is enabled */
#if defined(ENABLE_VERIFIER)
		case JAVA_BREAKPOINT:
			exceptions_throw_verifyerror(m, "Quick instructions shouldn't appear, yet.");
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
			exceptions_throw_verifyerror(m, "Illegal opcode %d at instr %d\n",
										 opcode, ipc);
			return false;
			break;
#endif /* defined(ENABLE_VERIFIER) */

		/* opcodes that don't require translation *****************************/

		default:
			/* straight-forward translation to ICMD */
			OP(opcode);
			break;

		} /* end switch */

		/* verifier checks ****************************************************/

#if defined(ENABLE_VERIFIER)
		/* If WIDE was used correctly, iswide should have been reset by now. */
		if (iswide) {
			exceptions_throw_verifyerror(m,
					"Illegal instruction: WIDE before incompatible opcode");
			return false;
		}
#endif /* defined(ENABLE_VERIFIER) */

	} /* end for */

	if (JITDATA_HAS_FLAG_REORDER(jd)) {
		/* add a NOP to the last basic block */

		INSTRUCTIONS_CHECK(1);
		OP(ICMD_NOP);
	}

	/*** END OF LOOP **********************************************************/

	/* assert that we did not write more ICMDs than allocated */

	assert(ipc <= pd.instructionslength);
	assert(ipc == (iptr - pd.instructions));

	/*** verifier checks ******************************************************/

#if defined(ENABLE_VERIFIER)
	if (p != m->jcodelength) {
		exceptions_throw_verifyerror(m,
				"Command-sequence crosses code-boundary");
		return false;
	}

	if (!blockend) {
		exceptions_throw_verifyerror(m, "Falling off the end of the code");
		return false;
	}
#endif /* defined(ENABLE_VERIFIER) */

	/*** setup the methodinfo, allocate stack and basic blocks ****************/

	/* adjust block count if target 0 is not first intermediate instruction */

	if (!jd->basicblockindex[0] || (jd->basicblockindex[0] > 1))
		b_count++;

	/* copy local to method variables */

	jd->instructions = pd.instructions;
	jd->instructioncount = ipc;
	jd->basicblockcount = b_count;
	jd->stackcount = s_count + jd->basicblockcount * m->maxstack; /* in-stacks */

	/* allocate stack table */

	jd->stack = DMNEW(stackelement, jd->stackcount);

	/* build basic block list */

	bptr = jd->basicblocks = DMNEW(basicblock, b_count + 1);    /* one more for end ipc */

	/* zero out all basic block structures */

	MZERO(bptr, basicblock, b_count + 1);

	b_count = 0;

	/* additional block if target 0 is not first intermediate instruction */

	if (!jd->basicblockindex[0] || (jd->basicblockindex[0] > 1)) {
		BASICBLOCK_INIT(bptr, m);

		bptr->iinstr = jd->instructions;
		/* bptr->icount is set when the next block is allocated */

		bptr->nr = b_count++;
		bptr++;
		bptr[-1].next = bptr;
	}

	/* allocate blocks */

	for (p = 0; p < m->jcodelength; p++) {
		if (jd->basicblockindex[p] & 1) {
#if defined(ENABLE_VERIFIER)
			/* Check if this block starts at the beginning of an
			   instruction. */

			if (!pd.instructionstart[p]) {
				exceptions_throw_verifyerror(m,
						"Branch into middle of instruction");
				return false;
			}
#endif

			/* allocate the block */

			BASICBLOCK_INIT(bptr, m);

			bptr->iinstr = jd->instructions + (jd->basicblockindex[p] >> 1);
			if (b_count) {
				bptr[-1].icount = bptr->iinstr - bptr[-1].iinstr;
			}
			/* bptr->icount is set when the next block is allocated */

			jd->basicblockindex[p] = b_count;

			bptr->nr = b_count++;
			bptr++;
			bptr[-1].next = bptr;
		}
	}

	/* set instruction count of last real block */

	if (b_count) {
		bptr[-1].icount = (jd->instructions + jd->instructioncount) - bptr[-1].iinstr;
	}

	/* allocate additional block at end */

	BASICBLOCK_INIT(bptr,m);
	bptr->nr = b_count;
	jd->basicblockindex[m->jcodelength] = b_count;

	/* set basicblock pointers in exception table */

	if (!parse_resolve_exception_table(jd))
		return false;

	/* store the local map */

	jd->local_map = local_map;

	/* calculate local variable renaming */

	{
		s4 nlocals = 0;
		s4 i;

		s4 *mapptr;

		mapptr = local_map;

		/* iterate over local_map[0..m->maxlocals*5] and set all existing  */
		/* index,type pairs (local_map[index*5+type]==1) to an unique value */
		/* -> == new local var index */
		for(i = 0; i < (m->maxlocals * 5); i++, mapptr++) {
			if (*mapptr)
				*mapptr = nlocals++;
			else
				*mapptr = UNUSED;
		}

		jd->localcount = nlocals;

		/* calculate the (maximum) number of variables needed */

		jd->varcount = 
			  nlocals                                      /* local variables */
			+ jd->basicblockcount * m->maxstack                 /* invars */
			+ s_count;         /* variables created within blocks (non-invar) */

		/* reserve the first indices for local variables */

		jd->vartop = nlocals;

		/* reserve extra variables needed by stack analyse */

		jd->varcount += STACK_EXTRA_VARS;
		jd->vartop   += STACK_EXTRA_VARS;

		/* The verifier needs space for saving invars in some cases and */
		/* extra variables.                                             */

#if defined(ENABLE_VERIFIER)
		jd->varcount += VERIFIER_EXTRA_LOCALS + VERIFIER_EXTRA_VARS + m->maxstack;
		jd->vartop   += VERIFIER_EXTRA_LOCALS + VERIFIER_EXTRA_VARS + m->maxstack;
#endif
		/* allocate and initialize the variable array */

		jd->var = DMNEW(varinfo, jd->varcount);
		MZERO(jd->var, varinfo, jd->varcount);

		/* set types of all locals in jd->var */

		for(mapptr = local_map, i = 0; i < (m->maxlocals * 5); i++, mapptr++)
			if (*mapptr != UNUSED)
				VAR(*mapptr)->type = i%5;
	}

	/* everything's ok */

	return true;

	/*** goto labels for throwing verifier exceptions *************************/

#if defined(ENABLE_VERIFIER)

throw_unexpected_end_of_bytecode:
	exceptions_throw_verifyerror(m, "Unexpected end of bytecode");
	return false;

throw_invalid_bytecode_index:
	exceptions_throw_verifyerror(m, "Illegal target of branch instruction");
	return false;

throw_illegal_local_variable_number:
	exceptions_throw_verifyerror(m, "Illegal local variable number");
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
