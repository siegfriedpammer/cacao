/* src/vm/jit/parse.c - parser for JavaVM to intermediate code translation

   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
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

*/


#include "config.h"

#include <assert.h>
#include <string.h>

#include "vm/types.h"

#include "mm/memory.h"

#include "native/native.h"

#include "threads/lock-common.h"

#include "toolbox/logging.h"

#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/stringlocal.h"

#include "vm/jit/asmpart.h"
#include "vm/jit/jit.h"
#include "vm/jit/parse.h"
#include "vm/jit/loop/loop.h"

#include "vm/jit/ir/bytecode.h"

#include "vmcore/linker.h"
#include "vmcore/loader.h"
#include "vmcore/options.h"
#include "vm/resolve.h"

#if defined(ENABLE_STATISTICS)
# include "vmcore/statistics.h"
#endif

#include "vmcore/suck.h"

#define INSTRUCTIONS_INCREMENT  5  /* number of additional instructions to    */
                                   /* allocate if space runs out              */


/* local macros ***************************************************************/

#define BYTECODEINDEX_TO_BASICBLOCK(dst) \
    do { \
        (dst).block = \
            parse_bytecodeindex_to_basicblock(jd, &pd, (dst).insindex); \
    } while (0)


/* parserdata_t ***************************************************************/

typedef struct parsedata_t parsedata_t;

struct parsedata_t {
	u1          *bytecodestart;         /* start of bytecode instructions     */
	u1          *basicblockstart;       /* start of bytecode basic-blocks     */

	s4          *bytecodemap;           /* bytecode to IR mapping             */
	
	instruction *instructions;          /* instruction array                  */
	s4           instructionslength;    /* length of the instruction array    */

	s4          *instructionmap;        /* IR to basic-block mapping          */
};


/* parse_setup *****************************************************************

   Fills the passed parsedata_t structure.

*******************************************************************************/

static void parse_setup(jitdata *jd, parsedata_t *pd)
{
	methodinfo *m;

	/* get required compiler data */

	m = jd->m;

	/* bytecode start array */

	pd->bytecodestart = DMNEW(u1, m->jcodelength + 1);
	MZERO(pd->bytecodestart, u1, m->jcodelength + 1);

	/* bytecode basic-block start array */

	pd->basicblockstart = DMNEW(u1, m->jcodelength + 1);
	MZERO(pd->basicblockstart, u1, m->jcodelength + 1);

	/* bytecode instruction index to IR instruction mapping */

	pd->bytecodemap = DMNEW(s4, m->jcodelength + 1);
	MSET(pd->bytecodemap, -1, s4, m->jcodelength + 1);

	/* allocate the instruction array */

	pd->instructionslength = m->jcodelength + 1;
	pd->instructions = DMNEW(instruction, pd->instructionslength);

	/* Zero the intermediate instructions array so we don't have any
	   invalid pointers in it if we cannot finish stack_analyse(). */

	MZERO(pd->instructions, instruction, pd->instructionslength);

	/* The instructionmap is allocated later when we know the count of
	   instructions. */

	pd->instructionmap = NULL;
}


/* parse_realloc_instructions **************************************************

   Reallocate the instructions array so there is room for at least N 
   additional instructions.

   RETURN VALUE:
       the new value for iptr

*******************************************************************************/

static instruction *parse_realloc_instructions(parsedata_t *pd, s4 icount, s4 n)
{
	/* increase the size of the instruction array */

	pd->instructionslength += (n + INSTRUCTIONS_INCREMENT);

	/* reallocate the array */

	pd->instructions = DMREALLOC(pd->instructions, instruction, icount,
								 pd->instructionslength);
	MZERO(pd->instructions + icount, instruction,
		  (pd->instructionslength - icount));

	/* return the iptr */

	return pd->instructions + icount;
}


/* parse_bytecodeindex_to_basicblock *******************************************

   Resolves a bytecode index to the corresponding basic block.

*******************************************************************************/

static basicblock *parse_bytecodeindex_to_basicblock(jitdata *jd,
													 parsedata_t *pd,
													 s4 bcindex)
{
	s4          irindex;
	basicblock *bb;

	irindex = pd->bytecodemap[bcindex];
	bb      = jd->basicblocks + pd->instructionmap[irindex];

	return bb;
}


/* parse_mark_exception_boundaries *********************************************

   Mark exception handlers and the boundaries of the handled regions as
   basic block boundaries.

   IN:
       jd...............current jitdata

   RETURN VALUE:
       true.............everything ok
	   false............an exception has been thrown

*******************************************************************************/

static bool parse_mark_exception_boundaries(jitdata *jd, parsedata_t *pd)
{
	s4                   bcindex;
	s4                   i;
	s4                   len;
	raw_exception_entry *rex;
	methodinfo          *m;

	m = jd->m;
	
	len = m->rawexceptiontablelength;

	if (len == 0)
		return true;

	rex = m->rawexceptiontable;

	for (i = 0; i < len; ++i, ++rex) {

		/* the start of the handled region becomes a basic block start */

   		bcindex = rex->startpc;
		CHECK_BYTECODE_INDEX(bcindex);
		MARK_BASICBLOCK(pd, bcindex);
		
		bcindex = rex->endpc; /* see JVM Spec 4.7.3 */
		CHECK_BYTECODE_INDEX_EXCLUSIVE(bcindex);

		/* check that the range is valid */

#if defined(ENABLE_VERIFIER)
		if (bcindex <= rex->startpc) {
			exceptions_throw_verifyerror(m, "Invalid exception handler range");
			return false;
		}
#endif
		
		/* End of handled region becomes a basic block boundary (if it
		   is the bytecode end, we'll use the special end block that
		   is created anyway). */

		if (bcindex < m->jcodelength)
			MARK_BASICBLOCK(pd, bcindex);
		else
			jd->branchtoend = true;

		/* the start of the handler becomes a basic block start  */

		bcindex = rex->handlerpc;
		CHECK_BYTECODE_INDEX(bcindex);
		MARK_BASICBLOCK(pd, bcindex);
	}

	/* everything ok */

	return true;

#if defined(ENABLE_VERIFIER)
throw_invalid_bytecode_index:
	exceptions_throw_verifyerror(m,
								 "Illegal bytecode index in exception table");
	return false;
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

static bool parse_resolve_exception_table(jitdata *jd, parsedata_t *pd)
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

		ex->start   = parse_bytecodeindex_to_basicblock(jd, pd, rex->startpc);
		ex->end     = parse_bytecodeindex_to_basicblock(jd, pd, rex->endpc);
		ex->handler = parse_bytecodeindex_to_basicblock(jd, pd, rex->handlerpc);

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

	s4           bcindex;               /* bytecode instruction index         */
	s4           nextbc;                /* start of next bytecode instruction */
	s4           opcode;                /* bytecode instruction opcode        */

	s4           irindex;               /* IR instruction index               */
	s4           ircount;               /* IR instruction count               */

	s4           bbcount;               /* basic block count                  */

	int  s_count = 0;             /* stack element counter                    */
	bool blockend;                /* true if basic block end has been reached */
	bool iswide;                  /* true if last instruction was a wide      */

	constant_classref  *cr;
	constant_classref  *compr;
	classinfo          *c;
	builtintable_entry *bte;
	constant_FMIref    *fmi;
	methoddesc         *md;
	unresolved_method  *um;
	unresolved_field   *uf;

	resolve_result_t    result;
	u2                  lineindex = 0;
	u2                  currentline = 0;
	u2                  linepcchange = 0;
	u4                  flags;
	basicblock         *bptr;

 	int                *local_map; /* local pointer to renaming map           */
	                               /* is assigned to rd->local_map at the end */
	branch_target_t *table;
	lookup_target_t *lookup;
	s4               i;
	s4               j;

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
  
 	iptr     = pd.instructions;
 	ircount  = 0;
	bbcount  = 0;
	blockend = false;
	iswide   = false;

	/* mark basic block boundaries for exception table */

	if (!parse_mark_exception_boundaries(jd, &pd))
		return false;

	/* initialize stack element counter */

	s_count = 1 + m->rawexceptiontablelength;

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

	for (bcindex = 0; bcindex < m->jcodelength; bcindex = nextbc) {

		/* mark this position as a valid bytecode instruction start */

		pd.bytecodestart[bcindex] = 1;

		/* change the current line number, if necessary */

		/* XXX rewrite this using pointer arithmetic */

		if (linepcchange == bcindex) {
			if (m->linenumbercount > lineindex) {
next_linenumber:
				currentline = m->linenumbers[lineindex].line_number;
				lineindex++;
				if (lineindex < m->linenumbercount) {
					linepcchange = m->linenumbers[lineindex].start_pc;
					if (linepcchange == bcindex)
						goto next_linenumber;
				}
			}
		}

fetch_opcode:
		/* fetch next opcode  */	

		opcode = SUCK_BE_U1(m->jcode + bcindex);

		/* If the previous instruction was a block-end instruction,
		   mark the current bytecode instruction as basic-block
		   starting instruction. */

		/* NOTE: Some compilers put a BC_nop after a blockend
		   instruction. */

		if (blockend && (opcode != BC_nop)) {
			MARK_BASICBLOCK(&pd, bcindex);
			blockend = false;
		}

		/* If the current bytecode instruction was marked as
		   basic-block starting instruction before (e.g. blockend,
		   forward-branch target), mark the current IR instruction
		   too. */

		if (pd.basicblockstart[bcindex] != 0) {
			/* We need a NOP as last instruction in each basic block
			   for basic block reordering (may be replaced with a GOTO
			   later). */

			INSTRUCTIONS_CHECK(1);
			OP(ICMD_NOP);
		}

		/* store intermediate instruction count (bit 0 mark block starts) */

		pd.bytecodemap[bcindex] = ircount;

		/* compute next instruction start */

		nextbc = bcindex + bytecode[opcode].length;

		CHECK_END_OF_BYTECODE(nextbc);

		/* add stack elements produced by this instruction */

		s_count += bytecode[opcode].slots;

		/* We check here for the space of 1 instruction in the
		   instruction array.  If an opcode is converted to more than
		   1 instruction, this is checked in the corresponding
		   case. */

		INSTRUCTIONS_CHECK(1);

		/* translate this bytecode instruction */
		switch (opcode) {

		case BC_nop:
			break;

		/* pushing constants onto the stack ***********************************/

		case BC_bipush:
			OP_LOADCONST_I(SUCK_BE_S1(m->jcode + bcindex + 1));
			break;

		case BC_sipush:
			OP_LOADCONST_I(SUCK_BE_S2(m->jcode + bcindex + 1));
			break;

		case BC_ldc1:
			i = SUCK_BE_U1(m->jcode + bcindex + 1);
			goto pushconstantitem;

		case BC_ldc2:
		case BC_ldc2w:
			i = SUCK_BE_U2(m->jcode + bcindex + 1);

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

		case BC_aconst_null:
			OP_LOADCONST_NULL();
			break;

		case BC_iconst_m1:
		case BC_iconst_0:
		case BC_iconst_1:
		case BC_iconst_2:
		case BC_iconst_3:
		case BC_iconst_4:
		case BC_iconst_5:
			OP_LOADCONST_I(opcode - BC_iconst_0);
			break;

		case BC_lconst_0:
		case BC_lconst_1:
			OP_LOADCONST_L(opcode - BC_lconst_0);
			break;

		case BC_fconst_0:
		case BC_fconst_1:
		case BC_fconst_2:
			OP_LOADCONST_F(opcode - BC_fconst_0);
			break;

		case BC_dconst_0:
		case BC_dconst_1:
			OP_LOADCONST_D(opcode - BC_dconst_0);
			break;

		/* stack operations ***************************************************/

		/* We need space for additional instruction so we can
		   translate these instructions to sequences of ICMD_COPY and
		   ICMD_MOVE instructions. */

		case BC_dup_x1:
			INSTRUCTIONS_CHECK(4);
			OP(opcode);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			break;

		case BC_dup_x2:
			INSTRUCTIONS_CHECK(6);
			OP(opcode);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			break;

		case BC_dup2:
			INSTRUCTIONS_CHECK(2);
			OP(opcode);
			OP(ICMD_NOP);
			break;

		case BC_dup2_x1:
			INSTRUCTIONS_CHECK(7);
			OP(opcode);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			break;

		case BC_dup2_x2:
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

		case BC_swap:
			INSTRUCTIONS_CHECK(3);
			OP(opcode);
			OP(ICMD_NOP);
			OP(ICMD_NOP);
			break;

		/* local variable access instructions *********************************/

		case BC_iload:
		case BC_fload:
		case BC_aload:
			if (iswide == false) {
				i = SUCK_BE_U1(m->jcode + bcindex + 1);
			}
			else {
				i = SUCK_BE_U2(m->jcode + bcindex + 1);
				nextbc = bcindex + 3;
				iswide = false;
			}
			OP_LOAD_ONEWORD(opcode, i, opcode - BC_iload);
			break;

		case BC_lload:
		case BC_dload:
			if (iswide == false) {
				i = SUCK_BE_U1(m->jcode + bcindex + 1);
			}
			else {
				i = SUCK_BE_U2(m->jcode + bcindex + 1);
				nextbc = bcindex + 3;
				iswide = false;
			}
			OP_LOAD_TWOWORD(opcode, i, opcode - BC_iload);
			break;

		case BC_iload_0:
		case BC_iload_1:
		case BC_iload_2:
		case BC_iload_3:
			OP_LOAD_ONEWORD(ICMD_ILOAD, opcode - BC_iload_0, TYPE_INT);
			break;

		case BC_lload_0:
		case BC_lload_1:
		case BC_lload_2:
		case BC_lload_3:
			OP_LOAD_TWOWORD(ICMD_LLOAD, opcode - BC_lload_0, TYPE_LNG);
			break;

		case BC_fload_0:
		case BC_fload_1:
		case BC_fload_2:
		case BC_fload_3:
			OP_LOAD_ONEWORD(ICMD_FLOAD, opcode - BC_fload_0, TYPE_FLT);
			break;

		case BC_dload_0:
		case BC_dload_1:
		case BC_dload_2:
		case BC_dload_3:
			OP_LOAD_TWOWORD(ICMD_DLOAD, opcode - BC_dload_0, TYPE_DBL);
			break;

		case BC_aload_0:
		case BC_aload_1:
		case BC_aload_2:
		case BC_aload_3:
			OP_LOAD_ONEWORD(ICMD_ALOAD, opcode - BC_aload_0, TYPE_ADR);
			break;

		case BC_istore:
		case BC_fstore:
		case BC_astore:
			if (iswide == false) {
				i = SUCK_BE_U1(m->jcode + bcindex + 1);
			}
			else {
				i = SUCK_BE_U2(m->jcode + bcindex + 1);
				nextbc = bcindex + 3;
				iswide = false;
			}
			OP_STORE_ONEWORD(opcode, i, opcode - BC_istore);
			break;

		case BC_lstore:
		case BC_dstore:
			if (iswide == false) {
				i = SUCK_BE_U1(m->jcode + bcindex + 1);
			}
			else {
				i = SUCK_BE_U2(m->jcode + bcindex + 1);
				nextbc = bcindex + 3;
				iswide = false;
			}
			OP_STORE_TWOWORD(opcode, i, opcode - BC_istore);
			break;

		case BC_istore_0:
		case BC_istore_1:
		case BC_istore_2:
		case BC_istore_3:
			OP_STORE_ONEWORD(ICMD_ISTORE, opcode - BC_istore_0, TYPE_INT);
			break;

		case BC_lstore_0:
		case BC_lstore_1:
		case BC_lstore_2:
		case BC_lstore_3:
			OP_STORE_TWOWORD(ICMD_LSTORE, opcode - BC_lstore_0, TYPE_LNG);
			break;

		case BC_fstore_0:
		case BC_fstore_1:
		case BC_fstore_2:
		case BC_fstore_3:
			OP_STORE_ONEWORD(ICMD_FSTORE, opcode - BC_fstore_0, TYPE_FLT);
			break;

		case BC_dstore_0:
		case BC_dstore_1:
		case BC_dstore_2:
		case BC_dstore_3:
			OP_STORE_TWOWORD(ICMD_DSTORE, opcode - BC_dstore_0, TYPE_DBL);
			break;

		case BC_astore_0:
		case BC_astore_1:
		case BC_astore_2:
		case BC_astore_3:
			OP_STORE_ONEWORD(ICMD_ASTORE, opcode - BC_astore_0, TYPE_ADR);
			break;

		case BC_iinc:
			{
				int v;

				if (iswide == false) {
					i = SUCK_BE_U1(m->jcode + bcindex + 1);
					v = SUCK_BE_S1(m->jcode + bcindex + 2);
				}
				else {
					i = SUCK_BE_U2(m->jcode + bcindex + 1);
					v = SUCK_BE_S2(m->jcode + bcindex + 3);
					nextbc = bcindex + 5;
					iswide = false;
				}
				INDEX_ONEWORD(i);
				LOCALTYPE_USED(i, TYPE_INT);
				OP_LOCALINDEX_I(opcode, i, v);
			}
			break;

		/* wider index for loading, storing and incrementing ******************/

		case BC_wide:
			bcindex++;
			iswide = true;
			goto fetch_opcode;

		/* managing arrays ****************************************************/

		case BC_newarray:
			switch (SUCK_BE_S1(m->jcode + bcindex + 1)) {
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

		case BC_anewarray:
			i = SUCK_BE_U2(m->jcode + bcindex + 1);
			compr = (constant_classref *) class_getconstant(m->class, i, CONSTANT_Class);
			if (compr == NULL)
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

		case BC_multianewarray:
			i = SUCK_BE_U2(m->jcode + bcindex + 1);
 			j = SUCK_BE_U1(m->jcode + bcindex + 3);
  
 			cr = (constant_classref *) class_getconstant(m->class, i, CONSTANT_Class);
 			if (cr == NULL)
 				return false;
  
 			if (!resolve_classref(m, cr, resolveLazy, true, true, &c))
 				return false;
  
 			/* if unresolved, c == NULL */
  
 			iptr->s1.argcount = j;
 			OP_S3_CLASSINFO_OR_CLASSREF(opcode, c, cr, INS_FLAG_CHECK);
			code_unflag_leafmethod(code);
			break;

		/* control flow instructions ******************************************/

		case BC_ifeq:
		case BC_iflt:
		case BC_ifle:
		case BC_ifne:
		case BC_ifgt:
		case BC_ifge:
		case BC_ifnull:
		case BC_ifnonnull:
		case BC_if_icmpeq:
		case BC_if_icmpne:
		case BC_if_icmplt:
		case BC_if_icmpgt:
		case BC_if_icmple:
		case BC_if_icmpge:
		case BC_if_acmpeq:
		case BC_if_acmpne:
		case BC_goto:
			i = bcindex + SUCK_BE_S2(m->jcode + bcindex + 1);
			CHECK_BYTECODE_INDEX(i);
			MARK_BASICBLOCK(&pd, i);
			blockend = true;
			OP_INSINDEX(opcode, i);
			break;

		case BC_goto_w:
			i = bcindex + SUCK_BE_S4(m->jcode + bcindex + 1);
			CHECK_BYTECODE_INDEX(i);
			MARK_BASICBLOCK(&pd, i);
			blockend = true;
			OP_INSINDEX(ICMD_GOTO, i);
			break;

		case BC_jsr:
			i = bcindex + SUCK_BE_S2(m->jcode + bcindex + 1);
jsr_tail:
			CHECK_BYTECODE_INDEX(i);
			MARK_BASICBLOCK(&pd, i);
			blockend = true;
			OP_PREPARE_ZEROFLAGS(BC_jsr);
			iptr->sx.s23.s3.jsrtarget.insindex = i;
			PINC;
			break;

		case BC_jsr_w:
			i = bcindex + SUCK_BE_S4(m->jcode + bcindex + 1);
			goto jsr_tail;

		case BC_ret:
			if (iswide == false) {
				i = SUCK_BE_U1(m->jcode + bcindex + 1);
			}
			else {
				i = SUCK_BE_U2(m->jcode + bcindex + 1);
				nextbc = bcindex + 3;
				iswide = false;
			}
			blockend = true;

			OP_LOAD_ONEWORD(opcode, i, TYPE_ADR);
			break;

		case BC_ireturn:
		case BC_lreturn:
		case BC_freturn:
		case BC_dreturn:
		case BC_areturn:
		case BC_return:
			blockend = true;
			/* XXX ARETURN will need a flag in the typechecker */
			OP(opcode);
			break;

		case BC_athrow:
			blockend = true;
			/* XXX ATHROW will need a flag in the typechecker */
			OP(opcode);
			break;


		/* table jumps ********************************************************/

		case BC_lookupswitch:
			{
				s4 num, j;
				lookup_target_t *lookup;
#if defined(ENABLE_VERIFIER)
				s4 prevvalue = 0;
#endif
				blockend = true;
				nextbc = MEMORY_ALIGN((bcindex + 1), 4);

				CHECK_END_OF_BYTECODE(nextbc + 8);

				OP_PREPARE_ZEROFLAGS(opcode);

				/* default target */

				j = bcindex + SUCK_BE_S4(m->jcode + nextbc);
				iptr->sx.s23.s3.lookupdefault.insindex = j;
				nextbc += 4;
				CHECK_BYTECODE_INDEX(j);
				MARK_BASICBLOCK(&pd, j);

				/* number of pairs */

				num = SUCK_BE_U4(m->jcode + nextbc);
				iptr->sx.s23.s2.lookupcount = num;
				nextbc += 4;

				/* allocate the intermediate code table */

				lookup = DMNEW(lookup_target_t, num);
				iptr->dst.lookup = lookup;

				/* iterate over the lookup table */

				CHECK_END_OF_BYTECODE(nextbc + 8 * num);

				for (i = 0; i < num; i++) {
					/* value */

					j = SUCK_BE_S4(m->jcode + nextbc);
					lookup->value = j;

					nextbc += 4;

#if defined(ENABLE_VERIFIER)
					/* check if the lookup table is sorted correctly */

					if (i && (j <= prevvalue)) {
						exceptions_throw_verifyerror(m, "Unsorted lookup switch");
						return false;
					}
					prevvalue = j;
#endif
					/* target */

					j = bcindex + SUCK_BE_S4(m->jcode + nextbc);
					lookup->target.insindex = j;
					lookup++;
					nextbc += 4;
					CHECK_BYTECODE_INDEX(j);
					MARK_BASICBLOCK(&pd, j);
				}

				PINC;
				break;
			}

		case BC_tableswitch:
			{
				s4 num, j;
				s4 deftarget;
				branch_target_t *table;

				blockend = true;
				nextbc = MEMORY_ALIGN((bcindex + 1), 4);

				CHECK_END_OF_BYTECODE(nextbc + 12);

				OP_PREPARE_ZEROFLAGS(opcode);

				/* default target */

				deftarget = bcindex + SUCK_BE_S4(m->jcode + nextbc);
				nextbc += 4;
				CHECK_BYTECODE_INDEX(deftarget);
				MARK_BASICBLOCK(&pd, deftarget);

				/* lower bound */

				j = SUCK_BE_S4(m->jcode + nextbc);
				iptr->sx.s23.s2.tablelow = j;
				nextbc += 4;

				/* upper bound */

				num = SUCK_BE_S4(m->jcode + nextbc);
				iptr->sx.s23.s3.tablehigh = num;
				nextbc += 4;

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

				CHECK_END_OF_BYTECODE(nextbc + 4 * num);

				for (i = 0; i < num; i++) {
					j = bcindex + SUCK_BE_S4(m->jcode + nextbc);
					(table++)->insindex = j;
					nextbc += 4;
					CHECK_BYTECODE_INDEX(j);
					MARK_BASICBLOCK(&pd, j);
				}

				PINC;
				break;
			}


		/* load and store of object fields ************************************/

		case BC_aastore:
			OP(opcode);
			code_unflag_leafmethod(code);
			break;

		case BC_getstatic:
		case BC_putstatic:
		case BC_getfield:
		case BC_putfield:
			i = SUCK_BE_U2(m->jcode + bcindex + 1);
			fmi = class_getconstant(m->class, i, CONSTANT_Fieldref);

			if (fmi == NULL)
				return false;

			OP_PREPARE_ZEROFLAGS(opcode);
			iptr->sx.s23.s3.fmiref = fmi;

			/* only with -noverify, otherwise the typechecker does this */

#if defined(ENABLE_VERIFIER)
			if (!JITDATA_HAS_FLAG_VERIFY(jd)) {
#endif
				result = resolve_field_lazy(m, fmi);

				if (result == resolveFailed)
					return false;

				if (result != resolveSucceeded) {
					uf = resolve_create_unresolved_field(m->class, m, iptr);

					if (uf == NULL)
						return false;

					/* store the unresolved_field pointer */

					iptr->sx.s23.s3.uf = uf;
					iptr->flags.bits |= INS_FLAG_UNRESOLVED;
				}
#if defined(ENABLE_VERIFIER)
			}
#endif
			PINC;
			break;


		/* method invocation **************************************************/

		case BC_invokestatic:
			OP_PREPARE_ZEROFLAGS(opcode);

			i = SUCK_BE_U2(m->jcode + bcindex + 1);
			fmi = class_getconstant(m->class, i, CONSTANT_Methodref);

			if (fmi == NULL)
				return false;

			md = fmi->parseddesc.md;

			if (md->params == NULL)
				if (!descriptor_params_from_paramtypes(md, ACC_STATIC))
					return false;

			goto invoke_method;

		case BC_invokespecial:
			OP_PREPARE_FLAGS(opcode, INS_FLAG_CHECK);

			i = SUCK_BE_U2(m->jcode + bcindex + 1);
			fmi = class_getconstant(m->class, i, CONSTANT_Methodref);

			goto invoke_nonstatic_method;

		case BC_invokeinterface:
			OP_PREPARE_ZEROFLAGS(opcode);

			i = SUCK_BE_U2(m->jcode + bcindex + 1);
			fmi = class_getconstant(m->class, i, CONSTANT_InterfaceMethodref);

			goto invoke_nonstatic_method;

		case BC_invokevirtual:
			OP_PREPARE_ZEROFLAGS(opcode);

			i = SUCK_BE_U2(m->jcode + bcindex + 1);
			fmi = class_getconstant(m->class, i, CONSTANT_Methodref);

invoke_nonstatic_method:
			if (fmi == NULL)
				return false;

			md = fmi->parseddesc.md;

			if (md->params == NULL)
				if (!descriptor_params_from_paramtypes(md, 0))
					return false;

invoke_method:
			code_unflag_leafmethod(code);

			iptr->sx.s23.s3.fmiref = fmi;

			/* only with -noverify, otherwise the typechecker does this */

#if defined(ENABLE_VERIFIER)
			if (!JITDATA_HAS_FLAG_VERIFY(jd)) {
#endif
				result = resolve_method_lazy(m, fmi, 
											 (opcode == BC_invokespecial));

				if (result == resolveFailed)
					return false;

				if (result == resolveSucceeded) {
					methodinfo *mi = iptr->sx.s23.s3.fmiref->p.method;

					/* if this call is monomorphic, turn it into an
					   INVOKESPECIAL */

					assert(IS_FMIREF_RESOLVED(iptr->sx.s23.s3.fmiref));

					if ((iptr->opc == ICMD_INVOKEVIRTUAL)
						&& (mi->flags & (ACC_FINAL | ACC_PRIVATE)))
					{
						iptr->opc         = ICMD_INVOKESPECIAL;
						iptr->flags.bits |= INS_FLAG_CHECK;
					}
				}
				else {
					um = resolve_create_unresolved_method(m->class, m, fmi,
							(opcode == BC_invokestatic),
							(opcode == BC_invokespecial));

					if (um == NULL)
						return false;

					/* store the unresolved_method pointer */

					iptr->sx.s23.s3.um = um;
					iptr->flags.bits |= INS_FLAG_UNRESOLVED;
				}
#if defined(ENABLE_VERIFIER)
			}
#endif
			PINC;
			break;

		/* instructions taking class arguments ********************************/

		case BC_new:
			i = SUCK_BE_U2(m->jcode + bcindex + 1);
			cr = class_getconstant(m->class, i, CONSTANT_Class);

			if (cr == NULL)
				return false;

			if (!resolve_classref(m, cr, resolveLazy, true, true, &c))
				return false;

			INSTRUCTIONS_CHECK(2);
			OP_LOADCONST_CLASSINFO_OR_CLASSREF_NOCHECK(c, cr);
			bte = builtintable_get_internal(BUILTIN_new);
			OP_BUILTIN_CHECK_EXCEPTION(bte);
			s_count++;
			break;

		case BC_checkcast:
			i = SUCK_BE_U2(m->jcode + bcindex + 1);
			cr = class_getconstant(m->class, i, CONSTANT_Class);

			if (cr == NULL)
				return false;

			if (!resolve_classref(m, cr, resolveLazy, true, true, &c))
				return false;

			if (cr->name->text[0] == '[') {
				/* array type cast-check */
				flags = INS_FLAG_CHECK | INS_FLAG_ARRAY;
				code_unflag_leafmethod(code);
			}
			else {
				/* object type cast-check */
				flags = INS_FLAG_CHECK;
			}
			OP_S3_CLASSINFO_OR_CLASSREF(opcode, c, cr, flags);
			break;

		case BC_instanceof:
			i = SUCK_BE_U2(m->jcode + bcindex + 1);
			cr = class_getconstant(m->class, i, CONSTANT_Class);

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

		case BC_monitorenter:
#if defined(ENABLE_THREADS)
			if (checksync) {
				bte = builtintable_get_internal(LOCK_monitor_enter);
				OP_BUILTIN_CHECK_EXCEPTION(bte);
			}
			else
#endif
			{
				OP_CHECK_EXCEPTION(ICMD_CHECKNULL);
				OP(ICMD_POP);
			}
			break;

		case BC_monitorexit:
#if defined(ENABLE_THREADS)
			if (checksync) {
				bte = builtintable_get_internal(LOCK_monitor_exit);
				OP_BUILTIN_CHECK_EXCEPTION(bte);
			}
			else
#endif
			{
				OP_CHECK_EXCEPTION(ICMD_CHECKNULL);
				OP(ICMD_POP);
			}
			break;

		/* arithmetic instructions that may become builtin functions **********/

		case BC_idiv:
#if !SUPPORT_DIVISION
			bte = builtintable_get_internal(BUILTIN_idiv);
			OP_BUILTIN_ARITHMETIC(opcode, bte);
#else
# if SUPPORT_HARDWARE_DIVIDE_BY_ZERO
			OP(opcode);
# else
			OP_CHECK_EXCEPTION(opcode);
# endif
#endif
			break;

		case BC_irem:
#if !SUPPORT_DIVISION
			bte = builtintable_get_internal(BUILTIN_irem);
			OP_BUILTIN_ARITHMETIC(opcode, bte);
#else
# if SUPPORT_HARDWARE_DIVIDE_BY_ZERO
			OP(opcode);
# else
			OP_CHECK_EXCEPTION(opcode);
# endif
#endif
			break;

		case BC_ldiv:
#if !(SUPPORT_DIVISION && SUPPORT_LONG && SUPPORT_LONG_DIV)
			bte = builtintable_get_internal(BUILTIN_ldiv);
			OP_BUILTIN_ARITHMETIC(opcode, bte);
#else
# if SUPPORT_HARDWARE_DIVIDE_BY_ZERO
			OP(opcode);
# else
			OP_CHECK_EXCEPTION(opcode);
# endif
#endif
			break;

		case BC_lrem:
#if !(SUPPORT_DIVISION && SUPPORT_LONG && SUPPORT_LONG_DIV)
			bte = builtintable_get_internal(BUILTIN_lrem);
			OP_BUILTIN_ARITHMETIC(opcode, bte);
#else
# if SUPPORT_HARDWARE_DIVIDE_BY_ZERO
			OP(opcode);
# else
			OP_CHECK_EXCEPTION(opcode);
# endif
#endif
			break;

		case BC_frem:
#if defined(__I386__)
			OP(opcode);
#else
			bte = builtintable_get_internal(BUILTIN_frem);
			OP_BUILTIN_NO_EXCEPTION(bte);
#endif
			break;

		case BC_drem:
#if defined(__I386__)
			OP(opcode);
#else
			bte = builtintable_get_internal(BUILTIN_drem);
			OP_BUILTIN_NO_EXCEPTION(bte);
#endif
			break;

		case BC_f2i:
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

		case BC_f2l:
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

		case BC_d2i:
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

		case BC_d2l:
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
		case BC_breakpoint:
			exceptions_throw_verifyerror(m, "Quick instructions shouldn't appear, yet.");
			return false;


		/* Unused opcodes ************************************************** */

		case 186:
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
										 opcode, ircount);
			return false;
			break;
#endif /* defined(ENABLE_VERIFIER) */

		/* opcodes that don't require translation *****************************/

		default:
			/* Straight-forward translation to HIR. */
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

	assert(ircount <= pd.instructionslength);
	assert(ircount == (iptr - pd.instructions));

	/*** verifier checks ******************************************************/

#if defined(ENABLE_VERIFIER)
	if (bcindex != m->jcodelength) {
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

	/* identify basic blocks */

	/* check if first instruction is a branch target */

	if (pd.basicblockstart[0] == 1) {
		jd->branchtoentry = true;
	}
	else {
		/* first instruction always starts a basic block */

		iptr = pd.instructions;

		iptr->flags.bits |= INS_FLAG_BASICBLOCK;
	}

	/* Iterate over all bytecode instructions and set missing
	   basic-block starts in IR instructions. */

	for (bcindex = 0; bcindex < m->jcodelength; bcindex++) {
		/* Does the current bytecode instruction start a basic
		   block? */

		if (pd.basicblockstart[bcindex] == 1) {
#if defined(ENABLE_VERIFIER)
			/* Check if this bytecode basic-block start at the
			   beginning of a bytecode instruction. */

			if (pd.bytecodestart[bcindex] == 0) {
				exceptions_throw_verifyerror(m,
										 "Branch into middle of instruction");
				return false;
			}
#endif

			/* Get the IR instruction mapped to the bytecode
			   instruction and set the basic block flag. */

			irindex = pd.bytecodemap[bcindex];
			iptr    = pd.instructions + irindex;

			iptr->flags.bits |= INS_FLAG_BASICBLOCK;
		}
	}

	/* IR instruction index to basic-block index mapping */

	pd.instructionmap = DMNEW(s4, ircount);
	MZERO(pd.instructionmap, s4, ircount);

	/* Iterate over all IR instructions and count the basic blocks. */

	iptr = pd.instructions;

	bbcount = 0;

	for (i = 0; i < ircount; i++, iptr++) {
		if (INSTRUCTION_STARTS_BASICBLOCK(iptr)) {
			/* store the basic-block number in the IR instruction
			   map */

			pd.instructionmap[i] = bbcount;

			/* post-increment the basic-block count */

			bbcount++;
		}
	}

	/* Allocate basic block array (one more for end ipc). */

	jd->basicblocks = DMNEW(basicblock, bbcount + 1);
	MZERO(jd->basicblocks, basicblock, bbcount + 1);

	/* Now iterate again over all IR instructions and initialize the
	   basic block structures and, in the same loop, resolve the
	   branch-target instruction indices to basic blocks. */

	iptr = pd.instructions;
	bptr = jd->basicblocks;

	bbcount = 0;

	for (i = 0; i < ircount; i++, iptr++) {
		/* check for basic block */

		if (INSTRUCTION_STARTS_BASICBLOCK(iptr)) {
			/* intialize the basic block */

			BASICBLOCK_INIT(bptr, m);

			bptr->iinstr = iptr;

			if (bbcount > 0) {
				bptr[-1].icount = bptr->iinstr - bptr[-1].iinstr;
			}

			/* bptr->icount is set when the next block is allocated */

			bptr->nr = bbcount++;
			bptr++;
			bptr[-1].next = bptr;
		}

		/* resolve instruction indices to basic blocks */

		switch (iptr->opc) {
		case ICMD_IFEQ:
		case ICMD_IFLT:
		case ICMD_IFLE:
		case ICMD_IFNE:
		case ICMD_IFGT:
		case ICMD_IFGE:
		case ICMD_IFNULL:
		case ICMD_IFNONNULL:
		case ICMD_IF_ICMPEQ:
		case ICMD_IF_ICMPNE:
		case ICMD_IF_ICMPLT:
		case ICMD_IF_ICMPGT:
		case ICMD_IF_ICMPLE:
		case ICMD_IF_ICMPGE:
		case ICMD_IF_ACMPEQ:
		case ICMD_IF_ACMPNE:
		case ICMD_GOTO:
			BYTECODEINDEX_TO_BASICBLOCK(iptr->dst);
			break;

		case ICMD_JSR:
			BYTECODEINDEX_TO_BASICBLOCK(iptr->sx.s23.s3.jsrtarget);
			break;

		case ICMD_TABLESWITCH:
			table = iptr->dst.table;

			BYTECODEINDEX_TO_BASICBLOCK(*table);
			table++;

			j = iptr->sx.s23.s3.tablehigh - iptr->sx.s23.s2.tablelow + 1;

			while (--j >= 0) {
				BYTECODEINDEX_TO_BASICBLOCK(*table);
				table++;
			}
			break;

		case ICMD_LOOKUPSWITCH:
			BYTECODEINDEX_TO_BASICBLOCK(iptr->sx.s23.s3.lookupdefault);

			lookup = iptr->dst.lookup;

			j = iptr->sx.s23.s2.lookupcount;

			while (--j >= 0) {
				BYTECODEINDEX_TO_BASICBLOCK(lookup->target);
				lookup++;
			}
			break;
		}
	}

	/* set instruction count of last real block */

	if (bbcount > 0) {
		bptr[-1].icount = (pd.instructions + ircount) - bptr[-1].iinstr;
	}

	/* allocate additional block at end */

	BASICBLOCK_INIT(bptr, m);
	bptr->nr = bbcount;

	/* set basicblock pointers in exception table */

	if (!parse_resolve_exception_table(jd, &pd))
		return false;

	/* store the local map */

	jd->local_map = local_map;

	/* calculate local variable renaming */

	{
		s4 nlocals = 0;
		s4 i;
		s4 *mapptr;

		mapptr = local_map;

		/* iterate over local_map[0..m->maxlocals*5-1] and allocate a unique */
		/* variable index for each _used_ (javaindex,type) pair.             */
		/* (local_map[javaindex*5+type] = cacaoindex)                        */
		/* Unused (javaindex,type) pairs are marked with UNUSED.             */

		for (i = 0; i < (m->maxlocals * 5); i++, mapptr++) {
			if (*mapptr)
				*mapptr = nlocals++;
			else
				*mapptr = UNUSED;
		}

		jd->localcount = nlocals;

		/* calculate the (maximum) number of variables needed */

		jd->varcount = 
			  nlocals                                      /* local variables */
			+ bbcount * m->maxstack                                 /* invars */
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

		for (mapptr = local_map, i = 0; i < (m->maxlocals * 5); i++, mapptr++)
			if (*mapptr != UNUSED)
				VAR(*mapptr)->type = i%5;
	}

	/* assign local variables to method variables */

	jd->instructions     = pd.instructions;
	jd->instructioncount = ircount;
	jd->basicblockcount  = bbcount;
	jd->stackcount       = s_count + bbcount * m->maxstack; /* in-stacks */

	/* allocate stack table */

	jd->stack = DMNEW(stackelement, jd->stackcount);

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
