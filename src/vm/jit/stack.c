/* src/vm/jit/stack.c - stack analysis

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

   Changes: Edwin Steiner
            Christian Thalinger
	    Christian Ullrich

   $Id: stack.c 2774 2005-06-22 09:47:44Z christian $

*/


#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "disass.h"
#include "types.h"
#include "md-abi.h"
#include "codegen.h"

#include "mm/memory.h"
#include "native/native.h"
#include "toolbox/logging.h"
#include "vm/global.h"
#include "vm/builtin.h"
#include "vm/options.h"
#include "vm/resolve.h"
#include "vm/statistics.h"
#include "vm/stringlocal.h"
#include "vm/tables.h"
#include "vm/jit/codegen.inc.h"
#include "vm/jit/jit.h"
#include "vm/jit/reg.h"
#include "vm/jit/stack.h"
#include "vm/jit/lsra.h"


/**********************************************************************/
/* analyse_stack                                                      */
/**********************************************************************/

/* analyse_stack uses the intermediate code created by parse.c to
 * build a model of the JVM operand stack for the current method.
 *
 * The following checks are performed:
 *   - check for operand stack underflow (before each instruction)
 *   - check for operand stack overflow (after[1] each instruction)
 *   - check for matching stack depth at merging points
 *   - check for matching basic types[2] at merging points
 *   - check basic types for instruction input (except for BUILTIN*
 *         opcodes, INVOKE* opcodes and MULTIANEWARRAY)
 *
 * [1]) Checking this after the instruction should be ok. parse.c
 * counts the number of required stack slots in such a way that it is
 * only vital that we don't exceed `maxstack` at basic block
 * boundaries.
 *
 * [2]) 'basic types' means the distinction between INT, LONG, FLOAT,
 * DOUBLE and ADDRESS types. Subtypes of INT and different ADDRESS
 * types are not discerned.
 */

methodinfo *analyse_stack(methodinfo *m, codegendata *cd, registerdata *rd)
{
	int           b_count;
	int           b_index;
	int           stackdepth;
	stackptr      curstack;
	stackptr      new;
	stackptr      copy;
	int           opcode, i, len, loops;
	int           superblockend, repeat, deadcode;
	instruction  *iptr;
	basicblock   *bptr;
	basicblock   *tbptr;
	s4           *s4ptr;
	void        **tptr;
	s4           *argren;

	builtintable_entry *bte;
	unresolved_method  *um;
	methoddesc         *md;

#ifdef LSRA
	m->maxlifetimes = 0;
#endif

	argren = DMNEW(s4, cd->maxlocals);   /* table for argument renaming       */
	for (i = 0; i < cd->maxlocals; i++)
		argren[i] = i;
	
	new = m->stack;
	loops = 0;
	m->basicblocks[0].flags = BBREACHED;
	m->basicblocks[0].instack = 0;
	m->basicblocks[0].indepth = 0;

	for (i = 0; i < cd->exceptiontablelength; i++) {
		bptr = &m->basicblocks[m->basicblockindex[cd->exceptiontable[i].handlerpc]];
		bptr->flags = BBREACHED;
		bptr->type = BBTYPE_EXH;
		bptr->instack = new;
		bptr->indepth = 1;
		bptr->pre_count = 10000;
		STACKRESET;
		NEWXSTACK;
	}

#if CONDITIONAL_LOADCONST
	b_count = m->basicblockcount;
	bptr = m->basicblocks;
	while (--b_count >= 0) {
		if (bptr->icount != 0) {
			iptr = bptr->iinstr + bptr->icount - 1;
			switch (iptr->opc) {
			case ICMD_RET:
			case ICMD_RETURN:
			case ICMD_IRETURN:
			case ICMD_LRETURN:
			case ICMD_FRETURN:
			case ICMD_DRETURN:
			case ICMD_ARETURN:
			case ICMD_ATHROW:
				break;

			case ICMD_IFEQ:
			case ICMD_IFNE:
			case ICMD_IFLT:
			case ICMD_IFGE:
			case ICMD_IFGT:
			case ICMD_IFLE:

			case ICMD_IFNULL:
			case ICMD_IFNONNULL:

			case ICMD_IF_ICMPEQ:
			case ICMD_IF_ICMPNE:
			case ICMD_IF_ICMPLT:
			case ICMD_IF_ICMPGE:
			case ICMD_IF_ICMPGT:
			case ICMD_IF_ICMPLE:

			case ICMD_IF_ACMPEQ:
			case ICMD_IF_ACMPNE:
				bptr[1].pre_count++;
			case ICMD_GOTO:
				m->basicblocks[m->basicblockindex[iptr->op1]].pre_count++;
				break;

			case ICMD_TABLESWITCH:
				s4ptr = iptr->val.a;
				m->basicblocks[m->basicblockindex[*s4ptr++]].pre_count++;
				i = *s4ptr++;                               /* low     */
				i = *s4ptr++ - i + 1;                       /* high    */
				while (--i >= 0) {
					m->basicblocks[m->basicblockindex[*s4ptr++]].pre_count++;
				}
				break;
					
			case ICMD_LOOKUPSWITCH:
				s4ptr = iptr->val.a;
				m->basicblocks[m->basicblockindex[*s4ptr++]].pre_count++;
				i = *s4ptr++;                               /* count   */
				while (--i >= 0) {
					m->basicblocks[m->basicblockindex[s4ptr[1]]].pre_count++;
					s4ptr += 2;
				}
				break;
			default:
				bptr[1].pre_count++;
				break;
			}
		}
		bptr++;
	}
#endif /* CONDITIONAL_LOADCONST */


	do {
		loops++;
		b_count = m->basicblockcount;
		bptr = m->basicblocks;
		superblockend = true;
		repeat = false;
		STACKRESET;
		deadcode = true;

		while (--b_count >= 0) {
			if (bptr->flags == BBDELETED) {
				/* do nothing */

			} else if (superblockend && (bptr->flags < BBREACHED)) {
				repeat = true;

			} else if (bptr->flags <= BBREACHED) {
				if (superblockend) {
					stackdepth = bptr->indepth;

				} else if (bptr->flags < BBREACHED) {
					COPYCURSTACK(copy);
					bptr->instack = copy;
					bptr->indepth = stackdepth;

				} else if (bptr->indepth != stackdepth) {
					show_icmd_method(m, cd, rd);
					printf("Block: %d, required depth: %d, current depth: %d\n", bptr->debug_nr, bptr->indepth, stackdepth);
					log_text("Stack depth mismatch");
					assert(0);
				}

				curstack = bptr->instack;
				deadcode = false;
				superblockend = false;
				bptr->flags = BBFINISHED;
				len = bptr->icount;
				iptr = bptr->iinstr;
				b_index = bptr - m->basicblocks;

				while (--len >= 0)  {
					opcode = iptr->opc;
					 /* XXX TWISTI: why is this set to NULL here? */
 /*                 iptr->target = NULL; */

#if defined(USEBUILTINTABLE)
					bte = builtintable_get_automatic(opcode);

					if (bte && bte->opcode == opcode) {
						iptr->opc = ICMD_BUILTIN;
						iptr->op1 = bte->md->paramcount;
						iptr->val.a = bte;
						m->isleafmethod = false;
						goto builtin;
					}
#endif /* defined(USEBUILTINTABLE) */
					
					switch (opcode) {

						/* pop 0 push 0 */

					case ICMD_CHECKNULL:
						COUNT(count_check_null);
					case ICMD_NOP:
					case ICMD_CHECKASIZE:
					case ICMD_CHECKEXCEPTION:

					case ICMD_IFEQ_ICONST:
					case ICMD_IFNE_ICONST:
					case ICMD_IFLT_ICONST:
					case ICMD_IFGE_ICONST:
					case ICMD_IFGT_ICONST:
					case ICMD_IFLE_ICONST:
					case ICMD_ELSE_ICONST:
						SETDST;
						break;

					case ICMD_RET:
						rd->locals[iptr->op1][TYPE_ADR].type = TYPE_ADR;
					case ICMD_RETURN:
						COUNT(count_pcmd_return);
						SETDST;
						superblockend = true;
						break;

						/* pop 0 push 1 const */
						
					case ICMD_ICONST:
						COUNT(count_pcmd_load);
						if (len > 0) {
							switch (iptr[1].opc) {
							case ICMD_IADD:
								iptr[0].opc = ICMD_IADDCONST;
							icmd_iconst_tail:
								iptr[1].opc = ICMD_NOP;
								OP1_1(TYPE_INT, TYPE_INT);
								COUNT(count_pcmd_op);
								break;
							case ICMD_ISUB:
								iptr[0].opc = ICMD_ISUBCONST;
								goto icmd_iconst_tail;
#if SUPPORT_CONST_MUL
							case ICMD_IMUL:
								iptr[0].opc = ICMD_IMULCONST;
								goto icmd_iconst_tail;
#else /* SUPPORT_CONST_MUL */
							case ICMD_IMUL:
								if (iptr[0].val.i == 0x00000002)
									iptr[0].val.i = 1;
								else if (iptr[0].val.i == 0x00000004)
									iptr[0].val.i = 2;
								else if (iptr[0].val.i == 0x00000008)
									iptr[0].val.i = 3;
								else if (iptr[0].val.i == 0x00000010)
									iptr[0].val.i = 4;
								else if (iptr[0].val.i == 0x00000020)
									iptr[0].val.i = 5;
								else if (iptr[0].val.i == 0x00000040)
									iptr[0].val.i = 6;
								else if (iptr[0].val.i == 0x00000080)
									iptr[0].val.i = 7;
								else if (iptr[0].val.i == 0x00000100)
									iptr[0].val.i = 8;
								else if (iptr[0].val.i == 0x00000200)
									iptr[0].val.i = 9;
								else if (iptr[0].val.i == 0x00000400)
									iptr[0].val.i = 10;
								else if (iptr[0].val.i == 0x00000800)
									iptr[0].val.i = 11;
								else if (iptr[0].val.i == 0x00001000)
									iptr[0].val.i = 12;
								else if (iptr[0].val.i == 0x00002000)
									iptr[0].val.i = 13;
								else if (iptr[0].val.i == 0x00004000)
									iptr[0].val.i = 14;
								else if (iptr[0].val.i == 0x00008000)
									iptr[0].val.i = 15;
								else if (iptr[0].val.i == 0x00010000)
									iptr[0].val.i = 16;
								else if (iptr[0].val.i == 0x00020000)
									iptr[0].val.i = 17;
								else if (iptr[0].val.i == 0x00040000)
									iptr[0].val.i = 18;
								else if (iptr[0].val.i == 0x00080000)
									iptr[0].val.i = 19;
								else if (iptr[0].val.i == 0x00100000)
									iptr[0].val.i = 20;
								else if (iptr[0].val.i == 0x00200000)
									iptr[0].val.i = 21;
								else if (iptr[0].val.i == 0x00400000)
									iptr[0].val.i = 22;
								else if (iptr[0].val.i == 0x00800000)
									iptr[0].val.i = 23;
								else if (iptr[0].val.i == 0x01000000)
									iptr[0].val.i = 24;
								else if (iptr[0].val.i == 0x02000000)
									iptr[0].val.i = 25;
								else if (iptr[0].val.i == 0x04000000)
									iptr[0].val.i = 26;
								else if (iptr[0].val.i == 0x08000000)
									iptr[0].val.i = 27;
								else if (iptr[0].val.i == 0x10000000)
									iptr[0].val.i = 28;
								else if (iptr[0].val.i == 0x20000000)
									iptr[0].val.i = 29;
								else if (iptr[0].val.i == 0x40000000)
									iptr[0].val.i = 30;
								else if (iptr[0].val.i == 0x80000000)
									iptr[0].val.i = 31;
								else {
									PUSHCONST(TYPE_INT);
									break;
								}
								iptr[0].opc = ICMD_IMULPOW2;
								goto icmd_iconst_tail;
#endif /* SUPPORT_CONST_MUL */
							case ICMD_IDIV:
								if (iptr[0].val.i == 0x00000002)
									iptr[0].val.i = 1;
								else if (iptr[0].val.i == 0x00000004)
									iptr[0].val.i = 2;
								else if (iptr[0].val.i == 0x00000008)
									iptr[0].val.i = 3;
								else if (iptr[0].val.i == 0x00000010)
									iptr[0].val.i = 4;
								else if (iptr[0].val.i == 0x00000020)
									iptr[0].val.i = 5;
								else if (iptr[0].val.i == 0x00000040)
									iptr[0].val.i = 6;
								else if (iptr[0].val.i == 0x00000080)
									iptr[0].val.i = 7;
								else if (iptr[0].val.i == 0x00000100)
									iptr[0].val.i = 8;
								else if (iptr[0].val.i == 0x00000200)
									iptr[0].val.i = 9;
								else if (iptr[0].val.i == 0x00000400)
									iptr[0].val.i = 10;
								else if (iptr[0].val.i == 0x00000800)
									iptr[0].val.i = 11;
								else if (iptr[0].val.i == 0x00001000)
									iptr[0].val.i = 12;
								else if (iptr[0].val.i == 0x00002000)
									iptr[0].val.i = 13;
								else if (iptr[0].val.i == 0x00004000)
									iptr[0].val.i = 14;
								else if (iptr[0].val.i == 0x00008000)
									iptr[0].val.i = 15;
								else if (iptr[0].val.i == 0x00010000)
									iptr[0].val.i = 16;
								else if (iptr[0].val.i == 0x00020000)
									iptr[0].val.i = 17;
								else if (iptr[0].val.i == 0x00040000)
									iptr[0].val.i = 18;
								else if (iptr[0].val.i == 0x00080000)
									iptr[0].val.i = 19;
								else if (iptr[0].val.i == 0x00100000)
									iptr[0].val.i = 20;
								else if (iptr[0].val.i == 0x00200000)
									iptr[0].val.i = 21;
								else if (iptr[0].val.i == 0x00400000)
									iptr[0].val.i = 22;
								else if (iptr[0].val.i == 0x00800000)
									iptr[0].val.i = 23;
								else if (iptr[0].val.i == 0x01000000)
									iptr[0].val.i = 24;
								else if (iptr[0].val.i == 0x02000000)
									iptr[0].val.i = 25;
								else if (iptr[0].val.i == 0x04000000)
									iptr[0].val.i = 26;
								else if (iptr[0].val.i == 0x08000000)
									iptr[0].val.i = 27;
								else if (iptr[0].val.i == 0x10000000)
									iptr[0].val.i = 28;
								else if (iptr[0].val.i == 0x20000000)
									iptr[0].val.i = 29;
								else if (iptr[0].val.i == 0x40000000)
									iptr[0].val.i = 30;
								else if (iptr[0].val.i == 0x80000000)
									iptr[0].val.i = 31;
								else {
									PUSHCONST(TYPE_INT);
									break;
								}
								iptr[0].opc = ICMD_IDIVPOW2;
								goto icmd_iconst_tail;
							case ICMD_IREM:
								/*log_text("stack.c: ICMD_ICONST/ICMD_IREM");*/
								if ((iptr[0].val.i == 0x00000002) ||
									(iptr[0].val.i == 0x00000004) ||
									(iptr[0].val.i == 0x00000008) ||
									(iptr[0].val.i == 0x00000010) ||
									(iptr[0].val.i == 0x00000020) ||
									(iptr[0].val.i == 0x00000040) ||
									(iptr[0].val.i == 0x00000080) ||
									(iptr[0].val.i == 0x00000100) ||
									(iptr[0].val.i == 0x00000200) ||
									(iptr[0].val.i == 0x00000400) ||
									(iptr[0].val.i == 0x00000800) ||
									(iptr[0].val.i == 0x00001000) ||
									(iptr[0].val.i == 0x00002000) ||
									(iptr[0].val.i == 0x00004000) ||
									(iptr[0].val.i == 0x00008000) ||
									(iptr[0].val.i == 0x00010000) ||
									(iptr[0].val.i == 0x00020000) ||
									(iptr[0].val.i == 0x00040000) ||
									(iptr[0].val.i == 0x00080000) ||
									(iptr[0].val.i == 0x00100000) ||
									(iptr[0].val.i == 0x00200000) ||
									(iptr[0].val.i == 0x00400000) ||
									(iptr[0].val.i == 0x00800000) ||
									(iptr[0].val.i == 0x01000000) ||
									(iptr[0].val.i == 0x02000000) ||
									(iptr[0].val.i == 0x04000000) ||
									(iptr[0].val.i == 0x08000000) ||
									(iptr[0].val.i == 0x10000000) ||
									(iptr[0].val.i == 0x20000000) ||
									(iptr[0].val.i == 0x40000000) ||
									(iptr[0].val.i == 0x80000000)) {
									iptr[0].opc = ICMD_IREMPOW2;
									iptr[0].val.i -= 1;
									goto icmd_iconst_tail;
								}
								PUSHCONST(TYPE_INT);
								break;
#if SUPPORT_CONST_LOGICAL
							case ICMD_IAND:
								iptr[0].opc = ICMD_IANDCONST;
								goto icmd_iconst_tail;
							case ICMD_IOR:
								iptr[0].opc = ICMD_IORCONST;
								goto icmd_iconst_tail;
							case ICMD_IXOR:
								iptr[0].opc = ICMD_IXORCONST;
								goto icmd_iconst_tail;
#endif /* SUPPORT_CONST_LOGICAL */
							case ICMD_ISHL:
								iptr[0].opc = ICMD_ISHLCONST;
								goto icmd_iconst_tail;
							case ICMD_ISHR:
								iptr[0].opc = ICMD_ISHRCONST;
								goto icmd_iconst_tail;
							case ICMD_IUSHR:
								iptr[0].opc = ICMD_IUSHRCONST;
								goto icmd_iconst_tail;
#if SUPPORT_LONG_SHIFT
							case ICMD_LSHL:
								iptr[0].opc = ICMD_LSHLCONST;
								goto icmd_lconst_tail;
							case ICMD_LSHR:
								iptr[0].opc = ICMD_LSHRCONST;
								goto icmd_lconst_tail;
							case ICMD_LUSHR:
								iptr[0].opc = ICMD_LUSHRCONST;
								goto icmd_lconst_tail;
#endif /* SUPPORT_LONG_SHIFT */
							case ICMD_IF_ICMPEQ:
								iptr[0].opc = ICMD_IFEQ;
							icmd_if_icmp_tail:
								iptr[0].op1 = iptr[1].op1;
								bptr->icount--;
								len--;
#if 1
								/* iptr[1].opc = ICMD_NOP; */
								OP1_0(TYPE_INT);
								tbptr = m->basicblocks + m->basicblockindex[iptr->op1];

								iptr[0].target = (void *) tbptr;

								MARKREACHED(tbptr, copy);
								COUNT(count_pcmd_bra);
#else
								goto icmd_if;
#endif
								break;
							case ICMD_IF_ICMPLT:
								iptr[0].opc = ICMD_IFLT;
								goto icmd_if_icmp_tail;
							case ICMD_IF_ICMPLE:
								iptr[0].opc = ICMD_IFLE;
								goto icmd_if_icmp_tail;
							case ICMD_IF_ICMPNE:
								iptr[0].opc = ICMD_IFNE;
								goto icmd_if_icmp_tail;
							case ICMD_IF_ICMPGT:
								iptr[0].opc = ICMD_IFGT;
								goto icmd_if_icmp_tail;
							case ICMD_IF_ICMPGE:
								iptr[0].opc = ICMD_IFGE;
								goto icmd_if_icmp_tail;

#if SUPPORT_CONST_STORE
							case ICMD_IASTORE:
							case ICMD_BASTORE:
							case ICMD_CASTORE:
							case ICMD_SASTORE:
#if SUPPORT_CONST_STORE_ZERO_ONLY
								if (iptr[0].val.i == 0) {
#endif /* SUPPORT_CONST_STORE_ZERO_ONLY */
									switch (iptr[1].opc) {
									case ICMD_IASTORE:
										iptr[0].opc = ICMD_IASTORECONST;
										break;
									case ICMD_BASTORE:
										iptr[0].opc = ICMD_BASTORECONST;
										break;
									case ICMD_CASTORE:
										iptr[0].opc = ICMD_CASTORECONST;
										break;
									case ICMD_SASTORE:
										iptr[0].opc = ICMD_SASTORECONST;
										break;
									}

									iptr[1].opc = ICMD_NOP;
									OPTT2_0(TYPE_INT, TYPE_ADR);
									COUNT(count_pcmd_op);
#if SUPPORT_CONST_STORE_ZERO_ONLY
								} else
									PUSHCONST(TYPE_INT);
#endif /* SUPPORT_CONST_STORE_ZERO_ONLY */
								break;

							case ICMD_PUTSTATIC:
							case ICMD_PUTFIELD:
#if SUPPORT_CONST_STORE_ZERO_ONLY
								if (iptr[0].val.i == 0) {
#endif /* SUPPORT_CONST_STORE_ZERO_ONLY */
									switch (iptr[1].opc) {
									case ICMD_PUTSTATIC:
										iptr[0].opc = ICMD_PUTSTATICCONST;
										SETDST;
										break;
									case ICMD_PUTFIELD:
										iptr[0].opc = ICMD_PUTFIELDCONST;
										OP1_0(TYPE_ADR);
										break;
									}

									iptr[1].opc = ICMD_NOP;
									iptr[0].op1 = TYPE_INT;
									COUNT(count_pcmd_op);
#if SUPPORT_CONST_STORE_ZERO_ONLY
								} else
									PUSHCONST(TYPE_INT);
#endif /* SUPPORT_CONST_STORE_ZERO_ONLY */
								break;
#endif /* SUPPORT_CONST_STORE */
							default:
								PUSHCONST(TYPE_INT);
							}
						}
						else
							PUSHCONST(TYPE_INT);
						break;

					case ICMD_LCONST:
						COUNT(count_pcmd_load);
						if (len > 0) {
							switch (iptr[1].opc) {
#if SUPPORT_LONG_ADD
							case ICMD_LADD:
								iptr[0].opc = ICMD_LADDCONST;
							icmd_lconst_tail:
								iptr[1].opc = ICMD_NOP;
								OP1_1(TYPE_LNG,TYPE_LNG);
								COUNT(count_pcmd_op);
								break;
							case ICMD_LSUB:
								iptr[0].opc = ICMD_LSUBCONST;
								goto icmd_lconst_tail;
#endif /* SUPPORT_LONG_ADD */
#if SUPPORT_LONG_MUL && SUPPORT_CONST_MUL
							case ICMD_LMUL:
								iptr[0].opc = ICMD_LMULCONST;
								goto icmd_lconst_tail;
#else /* SUPPORT_LONG_MUL && SUPPORT_CONST_MUL */
# if SUPPORT_LONG_SHIFT
							case ICMD_LMUL:
								if (iptr[0].val.l == 0x00000002)
									iptr[0].val.i = 1;
								else if (iptr[0].val.l == 0x00000004)
									iptr[0].val.i = 2;
								else if (iptr[0].val.l == 0x00000008)
									iptr[0].val.i = 3;
								else if (iptr[0].val.l == 0x00000010)
									iptr[0].val.i = 4;
								else if (iptr[0].val.l == 0x00000020)
									iptr[0].val.i = 5;
								else if (iptr[0].val.l == 0x00000040)
									iptr[0].val.i = 6;
								else if (iptr[0].val.l == 0x00000080)
									iptr[0].val.i = 7;
								else if (iptr[0].val.l == 0x00000100)
									iptr[0].val.i = 8;
								else if (iptr[0].val.l == 0x00000200)
									iptr[0].val.i = 9;
								else if (iptr[0].val.l == 0x00000400)
									iptr[0].val.i = 10;
								else if (iptr[0].val.l == 0x00000800)
									iptr[0].val.i = 11;
								else if (iptr[0].val.l == 0x00001000)
									iptr[0].val.i = 12;
								else if (iptr[0].val.l == 0x00002000)
									iptr[0].val.i = 13;
								else if (iptr[0].val.l == 0x00004000)
									iptr[0].val.i = 14;
								else if (iptr[0].val.l == 0x00008000)
									iptr[0].val.i = 15;
								else if (iptr[0].val.l == 0x00010000)
									iptr[0].val.i = 16;
								else if (iptr[0].val.l == 0x00020000)
									iptr[0].val.i = 17;
								else if (iptr[0].val.l == 0x00040000)
									iptr[0].val.i = 18;
								else if (iptr[0].val.l == 0x00080000)
									iptr[0].val.i = 19;
								else if (iptr[0].val.l == 0x00100000)
									iptr[0].val.i = 20;
								else if (iptr[0].val.l == 0x00200000)
									iptr[0].val.i = 21;
								else if (iptr[0].val.l == 0x00400000)
									iptr[0].val.i = 22;
								else if (iptr[0].val.l == 0x00800000)
									iptr[0].val.i = 23;
								else if (iptr[0].val.l == 0x01000000)
									iptr[0].val.i = 24;
								else if (iptr[0].val.l == 0x02000000)
									iptr[0].val.i = 25;
								else if (iptr[0].val.l == 0x04000000)
									iptr[0].val.i = 26;
								else if (iptr[0].val.l == 0x08000000)
									iptr[0].val.i = 27;
								else if (iptr[0].val.l == 0x10000000)
									iptr[0].val.i = 28;
								else if (iptr[0].val.l == 0x20000000)
									iptr[0].val.i = 29;
								else if (iptr[0].val.l == 0x40000000)
									iptr[0].val.i = 30;
								else if (iptr[0].val.l == 0x80000000)
									iptr[0].val.i = 31;
								else {
									PUSHCONST(TYPE_LNG);
									break;
								}
								iptr[0].opc = ICMD_LMULPOW2;
								goto icmd_lconst_tail;
# endif /* SUPPORT_LONG_SHIFT */
#endif /* SUPPORT_LONG_MUL && SUPPORT_CONST_MUL */
#if SUPPORT_LONG_DIV
							case ICMD_LDIV:
								if (iptr[0].val.l == 0x00000002)
									iptr[0].val.i = 1;
								else if (iptr[0].val.l == 0x00000004)
									iptr[0].val.i = 2;
								else if (iptr[0].val.l == 0x00000008)
									iptr[0].val.i = 3;
								else if (iptr[0].val.l == 0x00000010)
									iptr[0].val.i = 4;
								else if (iptr[0].val.l == 0x00000020)
									iptr[0].val.i = 5;
								else if (iptr[0].val.l == 0x00000040)
									iptr[0].val.i = 6;
								else if (iptr[0].val.l == 0x00000080)
									iptr[0].val.i = 7;
								else if (iptr[0].val.l == 0x00000100)
									iptr[0].val.i = 8;
								else if (iptr[0].val.l == 0x00000200)
									iptr[0].val.i = 9;
								else if (iptr[0].val.l == 0x00000400)
									iptr[0].val.i = 10;
								else if (iptr[0].val.l == 0x00000800)
									iptr[0].val.i = 11;
								else if (iptr[0].val.l == 0x00001000)
									iptr[0].val.i = 12;
								else if (iptr[0].val.l == 0x00002000)
									iptr[0].val.i = 13;
								else if (iptr[0].val.l == 0x00004000)
									iptr[0].val.i = 14;
								else if (iptr[0].val.l == 0x00008000)
									iptr[0].val.i = 15;
								else if (iptr[0].val.l == 0x00010000)
									iptr[0].val.i = 16;
								else if (iptr[0].val.l == 0x00020000)
									iptr[0].val.i = 17;
								else if (iptr[0].val.l == 0x00040000)
									iptr[0].val.i = 18;
								else if (iptr[0].val.l == 0x00080000)
									iptr[0].val.i = 19;
								else if (iptr[0].val.l == 0x00100000)
									iptr[0].val.i = 20;
								else if (iptr[0].val.l == 0x00200000)
									iptr[0].val.i = 21;
								else if (iptr[0].val.l == 0x00400000)
									iptr[0].val.i = 22;
								else if (iptr[0].val.l == 0x00800000)
									iptr[0].val.i = 23;
								else if (iptr[0].val.l == 0x01000000)
									iptr[0].val.i = 24;
								else if (iptr[0].val.l == 0x02000000)
									iptr[0].val.i = 25;
								else if (iptr[0].val.l == 0x04000000)
									iptr[0].val.i = 26;
								else if (iptr[0].val.l == 0x08000000)
									iptr[0].val.i = 27;
								else if (iptr[0].val.l == 0x10000000)
									iptr[0].val.i = 28;
								else if (iptr[0].val.l == 0x20000000)
									iptr[0].val.i = 29;
								else if (iptr[0].val.l == 0x40000000)
									iptr[0].val.i = 30;
								else if (iptr[0].val.l == 0x80000000)
									iptr[0].val.i = 31;
								else {
									PUSHCONST(TYPE_LNG);
									break;
								}
								iptr[0].opc = ICMD_LDIVPOW2;
								goto icmd_lconst_tail;
							case ICMD_LREM:
								if ((iptr[0].val.l == 0x00000002) ||
									(iptr[0].val.l == 0x00000004) ||
									(iptr[0].val.l == 0x00000008) ||
									(iptr[0].val.l == 0x00000010) ||
									(iptr[0].val.l == 0x00000020) ||
									(iptr[0].val.l == 0x00000040) ||
									(iptr[0].val.l == 0x00000080) ||
									(iptr[0].val.l == 0x00000100) ||
									(iptr[0].val.l == 0x00000200) ||
									(iptr[0].val.l == 0x00000400) ||
									(iptr[0].val.l == 0x00000800) ||
									(iptr[0].val.l == 0x00001000) ||
									(iptr[0].val.l == 0x00002000) ||
									(iptr[0].val.l == 0x00004000) ||
									(iptr[0].val.l == 0x00008000) ||
									(iptr[0].val.l == 0x00010000) ||
									(iptr[0].val.l == 0x00020000) ||
									(iptr[0].val.l == 0x00040000) ||
									(iptr[0].val.l == 0x00080000) ||
									(iptr[0].val.l == 0x00100000) ||
									(iptr[0].val.l == 0x00200000) ||
									(iptr[0].val.l == 0x00400000) ||
									(iptr[0].val.l == 0x00800000) ||
									(iptr[0].val.l == 0x01000000) ||
									(iptr[0].val.l == 0x02000000) ||
									(iptr[0].val.l == 0x04000000) ||
									(iptr[0].val.l == 0x08000000) ||
									(iptr[0].val.l == 0x10000000) ||
									(iptr[0].val.l == 0x20000000) ||
									(iptr[0].val.l == 0x40000000) ||
									(iptr[0].val.l == 0x80000000)) {
									iptr[0].opc = ICMD_LREMPOW2;
									iptr[0].val.l -= 1;
									goto icmd_lconst_tail;
								}
								PUSHCONST(TYPE_LNG);
								break;
#endif /* SUPPORT_LONG_DIV */
#if SUPPORT_LONG_LOGICAL && SUPPORT_CONST_LOGICAL

							case ICMD_LAND:
								iptr[0].opc = ICMD_LANDCONST;
								goto icmd_lconst_tail;
							case ICMD_LOR:
								iptr[0].opc = ICMD_LORCONST;
								goto icmd_lconst_tail;
							case ICMD_LXOR:
								iptr[0].opc = ICMD_LXORCONST;
								goto icmd_lconst_tail;
#endif /* SUPPORT_LONG_LOGICAL && SUPPORT_CONST_LOGICAL */
#if !defined(NOLONG_CONDITIONAL)
							case ICMD_LCMP:
								if ((len > 1) && (iptr[2].val.i == 0)) {
									switch (iptr[2].opc) {
									case ICMD_IFEQ:
										iptr[0].opc = ICMD_IF_LEQ;
									icmd_lconst_lcmp_tail:
										iptr[0].op1 = iptr[2].op1;
										bptr->icount -= 2;
										len -= 2;
										/* iptr[1].opc = ICMD_NOP;
										   iptr[2].opc = ICMD_NOP; */
										OP1_0(TYPE_LNG);
										tbptr = m->basicblocks + m->basicblockindex[iptr->op1];

										iptr[0].target = (void *) tbptr;

										MARKREACHED(tbptr, copy);
										COUNT(count_pcmd_bra);
										COUNT(count_pcmd_op);
										break;
									case ICMD_IFNE:
										iptr[0].opc = ICMD_IF_LNE;
										goto icmd_lconst_lcmp_tail;
									case ICMD_IFLT:
										iptr[0].opc = ICMD_IF_LLT;
										goto icmd_lconst_lcmp_tail;
									case ICMD_IFGT:
										iptr[0].opc = ICMD_IF_LGT;
										goto icmd_lconst_lcmp_tail;
									case ICMD_IFLE:
										iptr[0].opc = ICMD_IF_LLE;
										goto icmd_lconst_lcmp_tail;
									case ICMD_IFGE:
										iptr[0].opc = ICMD_IF_LGE;
										goto icmd_lconst_lcmp_tail;
									default:
										PUSHCONST(TYPE_LNG);
									} /* switch (iptr[2].opc) */
								} /* if (iptr[2].val.i == 0) */
								else
									PUSHCONST(TYPE_LNG);
								break;
#endif /* !defined(NOLONG_CONDITIONAL) */

#if SUPPORT_CONST_STORE
							case ICMD_LASTORE:
#if SUPPORT_CONST_STORE_ZERO_ONLY
								if (iptr[0].val.l == 0) {
#endif /* SUPPORT_CONST_STORE_ZERO_ONLY */
									iptr[0].opc = ICMD_LASTORECONST;
									iptr[1].opc = ICMD_NOP;
									OPTT2_0(TYPE_INT, TYPE_ADR);
									COUNT(count_pcmd_op);
#if SUPPORT_CONST_STORE_ZERO_ONLY
								} else
									PUSHCONST(TYPE_LNG);
#endif /* SUPPORT_CONST_STORE_ZERO_ONLY */
								break;

							case ICMD_PUTSTATIC:
							case ICMD_PUTFIELD:
#if SUPPORT_CONST_STORE_ZERO_ONLY
								if (iptr[0].val.l == 0) {
#endif /* SUPPORT_CONST_STORE_ZERO_ONLY */
									switch (iptr[1].opc) {
									case ICMD_PUTSTATIC:
										iptr[0].opc = ICMD_PUTSTATICCONST;
										SETDST;
										break;
									case ICMD_PUTFIELD:
										iptr[0].opc = ICMD_PUTFIELDCONST;
										OP1_0(TYPE_ADR);
										break;
									}

									iptr[1].opc = ICMD_NOP;
									iptr[0].op1 = TYPE_LNG;
									COUNT(count_pcmd_op);
#if SUPPORT_CONST_STORE_ZERO_ONLY
								} else
									PUSHCONST(TYPE_LNG);
#endif /* SUPPORT_CONST_STORE_ZERO_ONLY */
								break;
#endif /* SUPPORT_CONST_STORE */
							default:
								PUSHCONST(TYPE_LNG);
							}
						}
						else
							PUSHCONST(TYPE_LNG);
						break;

					case ICMD_FCONST:
						COUNT(count_pcmd_load);
						PUSHCONST(TYPE_FLT);
						break;

					case ICMD_DCONST:
						COUNT(count_pcmd_load);
						PUSHCONST(TYPE_DBL);
						break;

					case ICMD_ACONST:
						COUNT(count_pcmd_load);
#if SUPPORT_CONST_STORE
						if (len > 0 && iptr->val.a == 0) {
							switch (iptr[1].opc) {
							case ICMD_BUILTIN:
								if (iptr[1].val.fp != BUILTIN_aastore) {
									PUSHCONST(TYPE_ADR);
									break;
								}
								/* fall through */
							case ICMD_PUTSTATIC:
							case ICMD_PUTFIELD:
								switch (iptr[1].opc) {
								case ICMD_BUILTIN:
									iptr[0].opc = ICMD_AASTORECONST;
									OPTT2_0(TYPE_INT, TYPE_ADR);
									break;
								case ICMD_PUTSTATIC:
									iptr[0].opc = ICMD_PUTSTATICCONST;
									iptr[0].op1 = TYPE_ADR;
									SETDST;
									break;
								case ICMD_PUTFIELD:
									iptr[0].opc = ICMD_PUTFIELDCONST;
									iptr[0].op1 = TYPE_ADR;
									OP1_0(TYPE_ADR);
									break;
								}

								iptr[1].opc = ICMD_NOP;
								COUNT(count_pcmd_op);
								break;

							default:
								PUSHCONST(TYPE_ADR);
							}
						} else
#endif /* SUPPORT_CONST_STORE */
							PUSHCONST(TYPE_ADR);
						break;

						/* pop 0 push 1 load */
						
					case ICMD_ILOAD:
					case ICMD_LLOAD:
					case ICMD_FLOAD:
					case ICMD_DLOAD:
					case ICMD_ALOAD:
						COUNT(count_load_instruction);
						i = opcode-ICMD_ILOAD;
						iptr->op1 = argren[iptr->op1];
						rd->locals[iptr->op1][i].type = i;
						LOAD(i, LOCALVAR, iptr->op1);
						break;

						/* pop 2 push 1 */

					case ICMD_LALOAD:
					case ICMD_IALOAD:
					case ICMD_FALOAD:
					case ICMD_DALOAD:
					case ICMD_AALOAD:
						COUNT(count_check_null);
						COUNT(count_check_bound);
						COUNT(count_pcmd_mem);
						OP2IAT_1(opcode-ICMD_IALOAD);
						break;

					case ICMD_BALOAD:
					case ICMD_CALOAD:
					case ICMD_SALOAD:
						COUNT(count_check_null);
						COUNT(count_check_bound);
						COUNT(count_pcmd_mem);
						OP2IAT_1(TYPE_INT);
						break;

						/* pop 0 push 0 iinc */

					case ICMD_IINC:
#if defined(STATISTICS)
						if (opt_stat) {
							i = stackdepth;
							if (i >= 10)
								count_store_depth[10]++;
							else
								count_store_depth[i]++;
						}
#endif
						copy = curstack;
						i = stackdepth - 1;
						while (copy) {
							if ((copy->varkind == LOCALVAR) &&
								(copy->varnum == iptr->op1)) {
								copy->varkind = TEMPVAR;
								copy->varnum = i;
							}
							i--;
							copy = copy->prev;
						}
						SETDST;
						break;

						/* pop 1 push 0 store */

					case ICMD_ISTORE:
					case ICMD_LSTORE:
					case ICMD_FSTORE:
					case ICMD_DSTORE:
					case ICMD_ASTORE:
					icmd_store:
						REQUIRE_1;

					i = opcode - ICMD_ISTORE;
					rd->locals[iptr->op1][i].type = i;
#if defined(STATISTICS)
					if (opt_stat) {
						count_pcmd_store++;
						i = new - curstack;
						if (i >= 20)
							count_store_length[20]++;
						else
							count_store_length[i]++;
						i = stackdepth - 1;
						if (i >= 10)
							count_store_depth[10]++;
						else
							count_store_depth[i]++;
					}
#endif
					copy = curstack->prev;
					i = stackdepth - 2;
					while (copy) {
						if ((copy->varkind == LOCALVAR) &&
							(copy->varnum == iptr->op1)) {
							copy->varkind = TEMPVAR;
							copy->varnum = i;
						}
						i--;
						copy = copy->prev;
					}
					if ((new - curstack) == 1) {
						curstack->varkind = LOCALVAR;
						curstack->varnum = iptr->op1;
					};
					STORE(opcode-ICMD_ISTORE);
					break;

					/* pop 3 push 0 */

					case ICMD_IASTORE:
					case ICMD_AASTORE:
					case ICMD_LASTORE:
					case ICMD_FASTORE:
					case ICMD_DASTORE:
						COUNT(count_check_null);
						COUNT(count_check_bound);
						COUNT(count_pcmd_mem);
						OP3TIA_0(opcode-ICMD_IASTORE);
						break;

					case ICMD_BASTORE:
					case ICMD_CASTORE:
					case ICMD_SASTORE:
						COUNT(count_check_null);
						COUNT(count_check_bound);
						COUNT(count_pcmd_mem);
						OP3TIA_0(TYPE_INT);
						break;

						/* pop 1 push 0 */

					case ICMD_POP:
#ifdef TYPECHECK_STACK_COMPCAT
						if (opt_verify) {
							REQUIRE_1;
							if (IS_2_WORD_TYPE(curstack->type)) {
								*exceptionptr = new_verifyerror(m, "Attempt to split long or double on the stack");
								return NULL;
							}
						}
#endif
						OP1_0ANY;
						break;

					case ICMD_IRETURN:
					case ICMD_LRETURN:
					case ICMD_FRETURN:
					case ICMD_DRETURN:
					case ICMD_ARETURN:
						COUNT(count_pcmd_return);
						OP1_0(opcode-ICMD_IRETURN);
						superblockend = true;
						break;

					case ICMD_ATHROW:
						COUNT(count_check_null);
						OP1_0(TYPE_ADR);
						STACKRESET;
						SETDST;
						superblockend = true;
						break;

					case ICMD_PUTSTATIC:
						COUNT(count_pcmd_mem);
						OP1_0(iptr->op1);
						break;

						/* pop 1 push 0 branch */

					case ICMD_IFNULL:
					case ICMD_IFNONNULL:
						COUNT(count_pcmd_bra);
						OP1_0(TYPE_ADR);
						tbptr = m->basicblocks + m->basicblockindex[iptr->op1];

						iptr[0].target = (void *) tbptr;

						MARKREACHED(tbptr, copy);
						break;

					case ICMD_IFEQ:
					case ICMD_IFNE:
					case ICMD_IFLT:
					case ICMD_IFGE:
					case ICMD_IFGT:
					case ICMD_IFLE:
#if 0
					icmd_if:
#endif
						COUNT(count_pcmd_bra);
#if CONDITIONAL_LOADCONST
						tbptr = m->basicblocks + b_index;
						if ((b_count >= 3) &&
							((b_index + 2) == m->basicblockindex[iptr[0].op1]) &&
							(tbptr[1].pre_count == 1) &&
							(tbptr[1].iinstr[0].opc == ICMD_ICONST) &&
							(tbptr[1].iinstr[1].opc == ICMD_GOTO)   &&
							((b_index + 3) == m->basicblockindex[tbptr[1].iinstr[1].op1]) &&
							(tbptr[2].pre_count == 1) &&
							(tbptr[2].iinstr[0].opc == ICMD_ICONST)  &&
							(tbptr[2].icount==1)) {
								/*printf("tbptr[2].icount=%d\n",tbptr[2].icount);*/
								OP1_1(TYPE_INT, TYPE_INT);
								switch (iptr[0].opc) {
									case ICMD_IFEQ:
										iptr[0].opc = ICMD_IFNE_ICONST;
										break;
									case ICMD_IFNE:
										iptr[0].opc = ICMD_IFEQ_ICONST;
										break;
									case ICMD_IFLT:
										iptr[0].opc = ICMD_IFGE_ICONST;
										break;
									case ICMD_IFGE:
										iptr[0].opc = ICMD_IFLT_ICONST;
										break;
									case ICMD_IFGT:
										iptr[0].opc = ICMD_IFLE_ICONST;
										break;
									case ICMD_IFLE:
										iptr[0].opc = ICMD_IFGT_ICONST;
										break;
									}
#if 1
								iptr[0].val.i = iptr[1].val.i;
								iptr[1].opc = ICMD_ELSE_ICONST;
								iptr[1].val.i = iptr[3].val.i;
								iptr[2].opc = ICMD_NOP;
								iptr[3].opc = ICMD_NOP;
#else
							/* HACK: save compare value in iptr[1].op1 */ 	 
							iptr[1].op1 = iptr[0].val.i; 	 
							iptr[0].val.i = tbptr[1].iinstr[0].val.i; 	 
							iptr[1].opc = ICMD_ELSE_ICONST; 	 
							iptr[1].val.i = tbptr[2].iinstr[0].val.i; 	 
							tbptr[1].iinstr[0].opc = ICMD_NOP; 	 
							tbptr[1].iinstr[1].opc = ICMD_NOP; 	 
							tbptr[2].iinstr[0].opc = ICMD_NOP; 	 
#endif
								tbptr[1].flags = BBDELETED;
								tbptr[2].flags = BBDELETED;
								tbptr[1].icount = 0;
								tbptr[2].icount = 0;
								if (tbptr[3].pre_count == 2) {
									len += tbptr[3].icount + 3;
									bptr->icount += tbptr[3].icount + 3;
									tbptr[3].flags = BBDELETED;
									tbptr[3].icount = 0;
									b_index++;
								}
								else {
									bptr->icount++;
									len ++;
								}
							b_index += 2;
							break;
						}
#endif /* CONDITIONAL_LOADCONST */

						OP1_0(TYPE_INT);
						tbptr = m->basicblocks + m->basicblockindex[iptr->op1];

						iptr[0].target = (void *) tbptr;

						MARKREACHED(tbptr, copy);
						break;

						/* pop 0 push 0 branch */

					case ICMD_GOTO:
						COUNT(count_pcmd_bra);
						tbptr = m->basicblocks + m->basicblockindex[iptr->op1];

						iptr[0].target = (void *) tbptr;

						MARKREACHED(tbptr, copy);
						SETDST;
						superblockend = true;
						break;

						/* pop 1 push 0 table branch */

					case ICMD_TABLESWITCH:
						COUNT(count_pcmd_table);
						OP1_0(TYPE_INT);
						s4ptr = iptr->val.a;
						tbptr = m->basicblocks + m->basicblockindex[*s4ptr++];
						MARKREACHED(tbptr, copy);
						i = *s4ptr++;                          /* low     */
						i = *s4ptr++ - i + 1;                  /* high    */

						tptr = DMNEW(void*, i+1);
						iptr->target = (void *) tptr;

						tptr[0] = (void *) tbptr;
						tptr++;

						while (--i >= 0) {
							tbptr = m->basicblocks + m->basicblockindex[*s4ptr++];

							tptr[0] = (void *) tbptr;
							tptr++;

							MARKREACHED(tbptr, copy);
						}
						SETDST;
						superblockend = true;
						break;
							
						/* pop 1 push 0 table branch */

					case ICMD_LOOKUPSWITCH:
						COUNT(count_pcmd_table);
						OP1_0(TYPE_INT);
						s4ptr = iptr->val.a;
						tbptr = m->basicblocks + m->basicblockindex[*s4ptr++];
						MARKREACHED(tbptr, copy);
						i = *s4ptr++;                          /* count   */

						tptr = DMNEW(void*, i+1);
						iptr->target = (void *) tptr;

						tptr[0] = (void *) tbptr;
						tptr++;

						while (--i >= 0) {
							tbptr = m->basicblocks + m->basicblockindex[s4ptr[1]];

							tptr[0] = (void *) tbptr;
							tptr++;
								
							MARKREACHED(tbptr, copy);
							s4ptr += 2;
						}
						SETDST;
						superblockend = true;
						break;

					case ICMD_MONITORENTER:
						COUNT(count_check_null);
					case ICMD_MONITOREXIT:
						OP1_0(TYPE_ADR);
						break;

						/* pop 2 push 0 branch */

					case ICMD_IF_ICMPEQ:
					case ICMD_IF_ICMPNE:
					case ICMD_IF_ICMPLT:
					case ICMD_IF_ICMPGE:
					case ICMD_IF_ICMPGT:
					case ICMD_IF_ICMPLE:
						COUNT(count_pcmd_bra);
						OP2_0(TYPE_INT);
						tbptr = m->basicblocks + m->basicblockindex[iptr->op1];
							
						iptr[0].target = (void *) tbptr;

						MARKREACHED(tbptr, copy);
						break;

					case ICMD_IF_ACMPEQ:
					case ICMD_IF_ACMPNE:
						COUNT(count_pcmd_bra);
						OP2_0(TYPE_ADR);
						tbptr = m->basicblocks + m->basicblockindex[iptr->op1];

						iptr[0].target = (void *) tbptr;

						MARKREACHED(tbptr, copy);
						break;

						/* pop 2 push 0 */

					case ICMD_PUTFIELD:
						COUNT(count_check_null);
						COUNT(count_pcmd_mem);
						OPTT2_0(iptr->op1,TYPE_ADR);
						break;

					case ICMD_POP2:
						REQUIRE_1;
						if (!IS_2_WORD_TYPE(curstack->type)) {
							/* ..., cat1 */
#ifdef TYPECHECK_STACK_COMPCAT
							if (opt_verify) {
								REQUIRE_2;
								if (IS_2_WORD_TYPE(curstack->prev->type)) {
									*exceptionptr = new_verifyerror(m, "Attempt to split long or double on the stack");
									return NULL;
								}
							}
#endif
							OP1_0ANY;                /* second pop */
						}
						else
							iptr->opc = ICMD_POP;
						OP1_0ANY;
						break;

						/* pop 0 push 1 dup */
						
					case ICMD_DUP:
#ifdef TYPECHECK_STACK_COMPCAT
						if (opt_verify) {
							REQUIRE_1;
							if (IS_2_WORD_TYPE(curstack->type)) {
								*exceptionptr = new_verifyerror(m, "Attempt to split long or double on the stack");
								return NULL;
							}
						}
#endif
						COUNT(count_dup_instruction);
						DUP;
						break;

					case ICMD_DUP2:
						REQUIRE_1;
						if (IS_2_WORD_TYPE(curstack->type)) {
							/* ..., cat2 */
							iptr->opc = ICMD_DUP;
							DUP;
						}
						else {
							REQUIRE_2;
							/* ..., ????, cat1 */
#ifdef TYPECHECK_STACK_COMPCAT
							if (opt_verify) {
								if (IS_2_WORD_TYPE(curstack->prev->type)) {
									*exceptionptr = new_verifyerror(m, "Attempt to split long or double on the stack");
									return NULL;
								}
							}
#endif
							copy = curstack;
							NEWSTACK(copy->prev->type, copy->prev->varkind,
									 copy->prev->varnum);
							NEWSTACK(copy->type, copy->varkind,
									 copy->varnum);
							SETDST;
							stackdepth += 2;
						}
						break;

						/* pop 2 push 3 dup */
						
					case ICMD_DUP_X1:
#ifdef TYPECHECK_STACK_COMPCAT
						if (opt_verify) {
							REQUIRE_2;
							if (IS_2_WORD_TYPE(curstack->type) ||
								IS_2_WORD_TYPE(curstack->prev->type)) {
									*exceptionptr = new_verifyerror(m, "Attempt to split long or double on the stack");
									return NULL;
							}
						}
#endif
						DUP_X1;
						break;

					case ICMD_DUP2_X1:
						REQUIRE_2;
						if (IS_2_WORD_TYPE(curstack->type)) {
							/* ..., ????, cat2 */
#ifdef TYPECHECK_STACK_COMPCAT
							if (opt_verify) {
								if (IS_2_WORD_TYPE(curstack->prev->type)) {
									*exceptionptr = new_verifyerror(m, "Attempt to split long or double on the stack");
									return NULL;
								}
							}
#endif
							iptr->opc = ICMD_DUP_X1;
							DUP_X1;
						}
						else {
							/* ..., ????, cat1 */
#ifdef TYPECHECK_STACK_COMPCAT
							if (opt_verify) {
								REQUIRE_3;
								if (IS_2_WORD_TYPE(curstack->prev->type)
									|| IS_2_WORD_TYPE(curstack->prev->prev->type)) {
									*exceptionptr = new_verifyerror(m, "Attempt to split long or double on the stack");
									return NULL;
								}
							}
#endif
							DUP2_X1;
						}
						break;

						/* pop 3 push 4 dup */
						
					case ICMD_DUP_X2:
						REQUIRE_2;
						if (IS_2_WORD_TYPE(curstack->prev->type)) {
							/* ..., cat2, ???? */
#ifdef TYPECHECK_STACK_COMPCAT
							if (opt_verify) {
								if (IS_2_WORD_TYPE(curstack->type)) {
									*exceptionptr = new_verifyerror(m, "Attempt to split long or double on the stack");
									return NULL;
								}
							}
#endif
							iptr->opc = ICMD_DUP_X1;
							DUP_X1;
						}
						else {
							/* ..., cat1, ???? */
#ifdef TYPECHECK_STACK_COMPCAT
							if (opt_verify) {
								REQUIRE_3;
								if (IS_2_WORD_TYPE(curstack->type)
									|| IS_2_WORD_TYPE(curstack->prev->prev->type)) {
									*exceptionptr = new_verifyerror(m, "Attempt to split long or double on the stack");
									return NULL;
								}
							}
#endif
							DUP_X2;
						}
						break;

					case ICMD_DUP2_X2:
						REQUIRE_2;
						if (IS_2_WORD_TYPE(curstack->type)) {
							/* ..., ????, cat2 */
							if (IS_2_WORD_TYPE(curstack->prev->type)) {
								/* ..., cat2, cat2 */
								iptr->opc = ICMD_DUP_X1;
								DUP_X1;
							}
							else {
								/* ..., cat1, cat2 */
#ifdef TYPECHECK_STACK_COMPCAT
								if (opt_verify) {
									REQUIRE_3;
									if (IS_2_WORD_TYPE(curstack->prev->prev->type)) {
										*exceptionptr = new_verifyerror(m, "Attempt to split long or double on the stack");
										return NULL;
									}
								}
#endif
								iptr->opc = ICMD_DUP_X2;
								DUP_X2;
							}
						}
						else {
							REQUIRE_3;
							/* ..., ????, ????, cat1 */
							if (IS_2_WORD_TYPE(curstack->prev->prev->type)) {
								/* ..., cat2, ????, cat1 */
#ifdef TYPECHECK_STACK_COMPCAT
								if (opt_verify) {
									if (IS_2_WORD_TYPE(curstack->prev->type)) {
										*exceptionptr = new_verifyerror(m, "Attempt to split long or double on the stack");
										return NULL;
									}
								}
#endif
								iptr->opc = ICMD_DUP2_X1;
								DUP2_X1;
							}
							else {
								/* ..., cat1, ????, cat1 */
#ifdef TYPECHECK_STACK_COMPCAT
								if (opt_verify) {
									REQUIRE_4;
									if (IS_2_WORD_TYPE(curstack->prev->type)
										|| IS_2_WORD_TYPE(curstack->prev->prev->prev->type)) {
										*exceptionptr = new_verifyerror(m, "Attempt to split long or double on the stack");
										return NULL;
									}
								}
#endif
								DUP2_X2;
							}
						}
						break;

						/* pop 2 push 2 swap */
						
					case ICMD_SWAP:
#ifdef TYPECHECK_STACK_COMPCAT
						if (opt_verify) {
							REQUIRE_2;
							if (IS_2_WORD_TYPE(curstack->type)
								|| IS_2_WORD_TYPE(curstack->prev->type)) {
								*exceptionptr = new_verifyerror(m, "Attempt to split long or double on the stack");
								return NULL;
							}
						}
#endif
						SWAP;
						break;

						/* pop 2 push 1 */
						
					case ICMD_IDIV:
#if !SUPPORT_DIVISION
						bte = builtintable_get_internal(BUILTIN_idiv);
						iptr->opc = ICMD_BUILTIN;
						iptr->op1 = bte->md->paramcount;
						iptr->val.a = bte;
						m->isleafmethod = false;
						goto builtin;
#endif

					case ICMD_IREM:
#if !SUPPORT_DIVISION
						bte = builtintable_get_internal(BUILTIN_irem);
						iptr->opc = ICMD_BUILTIN;
						iptr->op1 = bte->md->paramcount;
						iptr->val.a = bte;
						m->isleafmethod = false;
						goto builtin;
#endif

					case ICMD_ISHL:
					case ICMD_ISHR:
					case ICMD_IUSHR:
					case ICMD_IADD:
					case ICMD_ISUB:
					case ICMD_IMUL:
					case ICMD_IAND:
					case ICMD_IOR:
					case ICMD_IXOR:
						COUNT(count_pcmd_op);
						OP2_1(TYPE_INT);
						break;

					case ICMD_LDIV:
#if !(SUPPORT_DIVISION && SUPPORT_LONG && SUPPORT_LONG_DIV)
						bte = builtintable_get_internal(BUILTIN_ldiv);
						iptr->opc = ICMD_BUILTIN;
						iptr->op1 = bte->md->paramcount;
						iptr->val.a = bte;
						m->isleafmethod = false;
						goto builtin;
#endif

					case ICMD_LREM:
#if !(SUPPORT_DIVISION && SUPPORT_LONG && SUPPORT_LONG_DIV)
						bte = builtintable_get_internal(BUILTIN_lrem);
						iptr->opc = ICMD_BUILTIN;
						iptr->op1 = bte->md->paramcount;
						iptr->val.a = bte;
						m->isleafmethod = false;
						goto builtin;
#endif

					case ICMD_LMUL:
					case ICMD_LADD:
					case ICMD_LSUB:
#if SUPPORT_LONG_LOGICAL
					case ICMD_LAND:
					case ICMD_LOR:
					case ICMD_LXOR:
#endif /* SUPPORT_LONG_LOGICAL */
						COUNT(count_pcmd_op);
						OP2_1(TYPE_LNG);
						break;

					case ICMD_LSHL:
					case ICMD_LSHR:
					case ICMD_LUSHR:
						COUNT(count_pcmd_op);
						OP2IT_1(TYPE_LNG);
						break;

					case ICMD_FADD:
					case ICMD_FSUB:
					case ICMD_FMUL:
					case ICMD_FDIV:
					case ICMD_FREM:
						COUNT(count_pcmd_op);
						OP2_1(TYPE_FLT);
						break;

					case ICMD_DADD:
					case ICMD_DSUB:
					case ICMD_DMUL:
					case ICMD_DDIV:
					case ICMD_DREM:
						COUNT(count_pcmd_op);
						OP2_1(TYPE_DBL);
						break;

					case ICMD_LCMP:
						COUNT(count_pcmd_op);
#if !defined(NOLONG_CONDITIONAL)
						if ((len > 0) && (iptr[1].val.i == 0)) {
							switch (iptr[1].opc) {
							case ICMD_IFEQ:
								iptr[0].opc = ICMD_IF_LCMPEQ;
							icmd_lcmp_if_tail:
								iptr[0].op1 = iptr[1].op1;
								len--;
								bptr->icount--;
								/* iptr[1].opc = ICMD_NOP; */
								OP2_0(TYPE_LNG);
								tbptr = m->basicblocks + m->basicblockindex[iptr->op1];
			
								iptr[0].target = (void *) tbptr;

								MARKREACHED(tbptr, copy);
								COUNT(count_pcmd_bra);
								break;
							case ICMD_IFNE:
								iptr[0].opc = ICMD_IF_LCMPNE;
								goto icmd_lcmp_if_tail;
							case ICMD_IFLT:
								iptr[0].opc = ICMD_IF_LCMPLT;
								goto icmd_lcmp_if_tail;
							case ICMD_IFGT:
								iptr[0].opc = ICMD_IF_LCMPGT;
								goto icmd_lcmp_if_tail;
							case ICMD_IFLE:
								iptr[0].opc = ICMD_IF_LCMPLE;
								goto icmd_lcmp_if_tail;
							case ICMD_IFGE:
								iptr[0].opc = ICMD_IF_LCMPGE;
								goto icmd_lcmp_if_tail;
							default:
								OPTT2_1(TYPE_LNG, TYPE_INT);
							}
						}
						else
#endif
							OPTT2_1(TYPE_LNG, TYPE_INT);
						break;
					case ICMD_FCMPL:
					case ICMD_FCMPG:
						COUNT(count_pcmd_op);
						OPTT2_1(TYPE_FLT, TYPE_INT);
						break;
					case ICMD_DCMPL:
					case ICMD_DCMPG:
						COUNT(count_pcmd_op);
						OPTT2_1(TYPE_DBL, TYPE_INT);
						break;

						/* pop 1 push 1 */
						
					case ICMD_INEG:
					case ICMD_INT2BYTE:
					case ICMD_INT2CHAR:
					case ICMD_INT2SHORT:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_INT, TYPE_INT);
						break;
					case ICMD_LNEG:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_LNG, TYPE_LNG);
						break;
					case ICMD_FNEG:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_FLT, TYPE_FLT);
						break;
					case ICMD_DNEG:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_DBL, TYPE_DBL);
						break;

					case ICMD_I2L:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_INT, TYPE_LNG);
						break;
					case ICMD_I2F:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_INT, TYPE_FLT);
						break;
					case ICMD_I2D:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_INT, TYPE_DBL);
						break;
					case ICMD_L2I:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_LNG, TYPE_INT);
						break;
					case ICMD_L2F:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_LNG, TYPE_FLT);
						break;
					case ICMD_L2D:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_LNG, TYPE_DBL);
						break;
					case ICMD_F2I:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_FLT, TYPE_INT);
						break;
					case ICMD_F2L:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_FLT, TYPE_LNG);
						break;
					case ICMD_F2D:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_FLT, TYPE_DBL);
						break;
					case ICMD_D2I:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_DBL, TYPE_INT);
						break;
					case ICMD_D2L:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_DBL, TYPE_LNG);
						break;
					case ICMD_D2F:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_DBL, TYPE_FLT);
						break;

					case ICMD_CHECKCAST:
						OP1_1(TYPE_ADR, TYPE_ADR);
						break;

					case ICMD_INSTANCEOF:
					case ICMD_ARRAYLENGTH:
						OP1_1(TYPE_ADR, TYPE_INT);
						break;

					case ICMD_NEWARRAY:
					case ICMD_ANEWARRAY:
						OP1_1(TYPE_INT, TYPE_ADR);
						break;

					case ICMD_GETFIELD:
						COUNT(count_check_null);
						COUNT(count_pcmd_mem);
						OP1_1(TYPE_ADR, iptr->op1);
						break;

						/* pop 0 push 1 */
						
					case ICMD_GETSTATIC:
						COUNT(count_pcmd_mem);
						OP0_1(iptr->op1);
						break;

					case ICMD_NEW:
						OP0_1(TYPE_ADR);
						break;

					case ICMD_JSR:
						OP0_1(TYPE_ADR);
						tbptr = m->basicblocks + m->basicblockindex[iptr->op1];

						iptr[0].target = (void *) tbptr;

						/* This is a dirty hack. The typechecker
						 * needs it because the OP1_0ANY below
						 * overwrites iptr->dst.
						 */
						iptr->val.a = (void *) iptr->dst;

						tbptr->type = BBTYPE_SBR;

						/* We need to check for overflow right here because
						 * the pushed value is poped after MARKREACHED. */
						CHECKOVERFLOW;
						MARKREACHED(tbptr, copy);
						OP1_0ANY;
						break;

					/* pop many push any */

					case ICMD_BUILTIN:
					builtin:
						bte = (builtintable_entry *) iptr->val.a;
						md = bte->md;
						goto _callhandling;

					case ICMD_INVOKESTATIC:
					case ICMD_INVOKESPECIAL:
					case ICMD_INVOKEVIRTUAL:
					case ICMD_INVOKEINTERFACE:
						COUNT(count_pcmd_met);
						um = iptr->target;
						md = um->methodref->parseddesc.md;
/*                          if (lm->flags & ACC_STATIC) */
/*                              {COUNT(count_check_null);} */ 	 
					_callhandling:
						i = iptr->op1;

						if (md->memuse > rd->memuse)
							rd->memuse = md->memuse;
						if (md->argintreguse > rd->argintreguse)
							rd->argintreguse = md->argintreguse;
						if (md->argfltreguse > rd->argintreguse)
							rd->argfltreguse = md->argintreguse;

						REQUIRE(i);

						copy = curstack;
						for (i-- ; i >= 0; i--) {
							if (!(copy->flags & SAVEDVAR)) {
								copy->varkind = ARGVAR;
								copy->varnum = i;
								if (md->params[i].inmemory) {
									copy->flags = INMEMORY;
									copy->regoff = md->params[i].regoff;
								} else {
									copy->flags = 0;
									if (IS_FLT_DBL_TYPE(copy->type))
										copy->regoff =
										   rd->argfltregs[md->params[i].regoff];
									else {
										copy->regoff =
									       rd->argintregs[md->params[i].regoff];
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
										if (IS_2_WORD_TYPE(copy->type))
											SET_SECOND_REG( copy->regoff,
									  rd->argintregs[md->params[i].regoff + 1]);
#endif
									}
								}
							}
							copy = copy->prev;
						}

						while (copy) {
							copy->flags |= SAVEDVAR;
							copy = copy->prev;
						}

						i = iptr->op1;
						POPMANY(i);
						if (md->returntype.type != TYPE_VOID)
							OP0_1(md->returntype.type);
						break;

					case ICMD_INLINE_START:
					case ICMD_INLINE_END:
						SETDST;
						break;

					case ICMD_MULTIANEWARRAY:
						if (rd->argintreguse < 3)                                   
							rd->argintreguse = 3;	

						i = iptr->op1;

						REQUIRE(i);
#if defined(SPECIALMEMUSE)
# if defined(__DARWIN__)
						if (rd->memuse < (i + INT_ARG_CNT + LA_WORD_SIZE))
							rd->memuse = i + LA_WORD_SIZE + INT_ARG_CNT;
# else
						if (rd->memuse < (i + LA_WORD_SIZE + 3))
							rd->memuse = i + LA_WORD_SIZE + 3;
# endif
#else
# if defined(__I386__)
						if (rd->memuse < i + 3)
							rd->memuse = i + 3; /* n integer args spilled on stack */
# else
						if (rd->memuse < i)
							rd->memuse = i; /* n integer args spilled on stack */
# endif /* defined(__I386__) */
#endif
						copy = curstack;
						while (--i >= 0) {
							/* check INT type here? Currently typecheck does this. */
							if (!(copy->flags & SAVEDVAR)) {
								copy->varkind = ARGVAR;
								copy->varnum = i + INT_ARG_CNT;
								copy->flags |= INMEMORY;
#if defined(SPECIALMEMUSE)
# if defined(__DARWIN__)
								copy->regoff = i + LA_WORD_SIZE + INT_ARG_CNT;
# else
								copy->regoff = i + LA_WORD_SIZE + 3;
# endif
#else
# if defined(__I386__)
								copy->regoff = i + 3;
# else
								copy->regoff = i;
# endif /* defined(__I386__) */
#endif /* defined(SPECIALMEMUSE) */
							}
							copy = copy->prev;
						}
						while (copy) {
							copy->flags |= SAVEDVAR;
							copy = copy->prev;
						}
						i = iptr->op1;
						POPMANY(i);
						OP0_1(TYPE_ADR);
						break;

					case ICMD_CLEAR_ARGREN:
						for (i = iptr->op1; i < cd->maxlocals; i++)
							argren[i] = i;
						iptr->opc = opcode = ICMD_NOP;
						SETDST;
						break;
						
					case ICMD_READONLY_ARG:
					case ICMD_READONLY_ARG+1:
					case ICMD_READONLY_ARG+2:
					case ICMD_READONLY_ARG+3:
					case ICMD_READONLY_ARG+4:

						REQUIRE_1;
						if (curstack->varkind == LOCALVAR) {
							i = curstack->varnum;
							argren[iptr->op1] = i;
							iptr->op1 = i;
						}
						opcode = iptr->opc = opcode - ICMD_READONLY_ARG + ICMD_ISTORE;
						goto icmd_store;

						break;

					default:
						*exceptionptr =
							new_exception_message(string_java_lang_InternalError,
												  "Unknown ICMD");
						return NULL;
					} /* switch */

					CHECKOVERFLOW;
					iptr++;
				} /* while instructions */

				bptr->outstack = curstack;
				bptr->outdepth = stackdepth;
				BBEND(curstack, i);
			} /* if */
			else
				superblockend = true;
			bptr++;
		} /* while blocks */
	} while (repeat && !deadcode);

#if defined(STATISTICS)
	if (opt_stat) {
		if (m->basicblockcount > count_max_basic_blocks)
			count_max_basic_blocks = m->basicblockcount;
		count_basic_blocks += m->basicblockcount;
		if (m->instructioncount > count_max_javainstr)			count_max_javainstr = m->instructioncount;
		count_javainstr += m->instructioncount;
		if (m->stackcount > count_upper_bound_new_stack)
			count_upper_bound_new_stack = m->stackcount;
		if ((new - m->stack) > count_max_new_stack)
			count_max_new_stack = (new - m->stack);

		b_count = m->basicblockcount;
		bptr = m->basicblocks;
		while (--b_count >= 0) {
			if (bptr->flags > BBREACHED) {
				if (bptr->indepth >= 10)
					count_block_stack[10]++;
				else
					count_block_stack[bptr->indepth]++;
				len = bptr->icount;
				if (len < 10) 
					count_block_size_distribution[len]++;
				else if (len <= 12)
					count_block_size_distribution[10]++;
				else if (len <= 14)
					count_block_size_distribution[11]++;
				else if (len <= 16)
					count_block_size_distribution[12]++;
				else if (len <= 18)
					count_block_size_distribution[13]++;
				else if (len <= 20)
					count_block_size_distribution[14]++;
				else if (len <= 25)
					count_block_size_distribution[15]++;
				else if (len <= 30)
					count_block_size_distribution[16]++;
				else
					count_block_size_distribution[17]++;
			}
			bptr++;
		}

		if (loops == 1)
			count_analyse_iterations[0]++;
		else if (loops == 2)
			count_analyse_iterations[1]++;
		else if (loops == 3)
			count_analyse_iterations[2]++;
		else if (loops == 4)
			count_analyse_iterations[3]++;
		else
			count_analyse_iterations[4]++;

		if (m->basicblockcount <= 5)
			count_method_bb_distribution[0]++;
		else if (m->basicblockcount <= 10)
			count_method_bb_distribution[1]++;
		else if (m->basicblockcount <= 15)
			count_method_bb_distribution[2]++;
		else if (m->basicblockcount <= 20)
			count_method_bb_distribution[3]++;
		else if (m->basicblockcount <= 30)
			count_method_bb_distribution[4]++;
		else if (m->basicblockcount <= 40)
			count_method_bb_distribution[5]++;
		else if (m->basicblockcount <= 50)
			count_method_bb_distribution[6]++;
		else if (m->basicblockcount <= 75)
			count_method_bb_distribution[7]++;
		else
			count_method_bb_distribution[8]++;
	}
#endif

	/* just return methodinfo* to signal everything was ok */

	return m;
}


/**********************************************************************/
/* DEBUGGING HELPERS                                                  */
/**********************************************************************/

void icmd_print_stack(codegendata *cd, stackptr s)
{
	int i, j;
	stackptr t;

	i = cd->maxstack;
	t = s;
	
	while (t) {
		i--;
		t = t->prev;
	}
	j = cd->maxstack - i;
	while (--i >= 0)
		printf("    ");
	while (s) {
		j--;
		/* DEBUG */ /*printf("(%d,%d,%d,%d)",s->varkind,s->flags,s->regoff,s->varnum); fflush(stdout);*/
		if (s->flags & SAVEDVAR)
			switch (s->varkind) {
			case TEMPVAR:
				if (s->flags & INMEMORY)
					printf(" M%02d", s->regoff);
#ifdef HAS_ADDRESS_REGISTER_FILE
				else if (s->type == TYPE_ADR)
					printf(" R%02d", s->regoff);
#endif
				else if (IS_FLT_DBL_TYPE(s->type))
					printf(" F%02d", s->regoff);
				else {
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
					if (IS_2_WORD_TYPE(s->type))
						printf(" %3s/%3s", regs[GET_LOW_REG(s->regoff)],
                            regs[GET_HIGH_REG(s->regoff)]);
					else
#endif
						printf(" %3s", regs[s->regoff]);
				}
				break;
			case STACKVAR:
				printf(" I%02d", s->varnum);
				break;
			case LOCALVAR:
				printf(" L%02d", s->varnum);
				break;
			case ARGVAR:
				printf(" A%02d", s->varnum);
#ifdef INVOKE_NEW_DEBUG
				if (s->flags & INMEMORY)
					printf("(M%i)", s->regoff);
				else
					printf("(R%i)", s->regoff);
#endif
				break;
			default:
				printf(" !%02d", j);
			}
		else
			switch (s->varkind) {
			case TEMPVAR:
				if (s->flags & INMEMORY)
					printf(" m%02d", s->regoff);
#ifdef HAS_ADDRESS_REGISTER_FILE
				else if (s->type == TYPE_ADR)
					printf(" r%02d", s->regoff);
#endif
				else if (IS_FLT_DBL_TYPE(s->type))
					printf(" f%02d", s->regoff);
				else {
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
					if (IS_2_WORD_TYPE(s->type))
						printf(" %3s/%3s", regs[GET_LOW_REG(s->regoff)],
                            regs[GET_HIGH_REG(s->regoff)]);
					else
#endif
						printf(" %3s", regs[s->regoff]);
				}
				break;
			case STACKVAR:
				printf(" i%02d", s->varnum);
				break;
			case LOCALVAR:
				printf(" l%02d", s->varnum);
				break;
			case ARGVAR:
				printf(" a%02d", s->varnum);
#ifdef INVOKE_NEW_DEBUG
				if (s->flags & INMEMORY)
					printf("(M%i)", s->regoff);
				else
					printf("(R%i)", s->regoff);
#endif
				break;
			default:
				printf(" ?%02d", j);
			}
		s = s->prev;
	}
}


#if 0
static void print_reg(stackptr s) {
	if (s) {
		if (s->flags & SAVEDVAR)
			switch (s->varkind) {
			case TEMPVAR:
				if (s->flags & INMEMORY)
					printf(" tm%02d", s->regoff);
				else
					printf(" tr%02d", s->regoff);
				break;
			case STACKVAR:
				printf(" s %02d", s->varnum);
				break;
			case LOCALVAR:
				printf(" l %02d", s->varnum);
				break;
			case ARGVAR:
				printf(" a %02d", s->varnum);
				break;
			default:
				printf(" ! %02d", s->varnum);
			}
		else
			switch (s->varkind) {
			case TEMPVAR:
				if (s->flags & INMEMORY)
					printf(" Tm%02d", s->regoff);
				else
					printf(" Tr%02d", s->regoff);
				break;
			case STACKVAR:
				printf(" S %02d", s->varnum);
				break;
			case LOCALVAR:
				printf(" L %02d", s->varnum);
				break;
			case ARGVAR:
				printf(" A %02d", s->varnum);
				break;
			default:
				printf(" ? %02d", s->varnum);
			}
	}
	else
		printf("     ");
		
}
#endif


static char *jit_type[] = {
	"int",
	"lng",
	"flt",
	"dbl",
	"adr"
};


void show_icmd_method(methodinfo *m, codegendata *cd, registerdata *rd)
{
	int i, j;
	basicblock *bptr;
	exceptiontable *ex;

	printf("\n");
	utf_fprint_classname(stdout, m->class->name);
	printf(".");
	utf_fprint(stdout, m->name);
	utf_fprint(stdout, m->descriptor);
	printf("\n\nMax locals: %d\n", (int) cd->maxlocals);
	printf("Max stack:  %d\n", (int) cd->maxstack);

	printf("Line number table length: %d\n", m->linenumbercount);

	printf("Exceptions (Number: %d):\n", cd->exceptiontablelength);
	for (ex = cd->exceptiontable; ex != NULL; ex = ex->down) {
		printf("    L%03d ... ", ex->start->debug_nr );
		printf("L%03d  = ", ex->end->debug_nr);
		printf("L%03d", ex->handler->debug_nr);
		printf("  (catchtype: ");
		if (ex->catchtype.any)
			if (IS_CLASSREF(ex->catchtype))
				utf_display_classname(ex->catchtype.ref->name);
			else
				utf_display_classname(ex->catchtype.cls->name);
		else
			printf("ANY");
		printf(")\n");
	}
	
	printf("Local Table:\n");
	for (i = 0; i < cd->maxlocals; i++) {
		printf("   %3d: ", i);
		for (j = TYPE_INT; j <= TYPE_ADR; j++)
			if (rd->locals[i][j].type >= 0) {
				printf("   (%s) ", jit_type[j]);
				if (rd->locals[i][j].flags & INMEMORY)
					printf("m%2d", rd->locals[i][j].regoff);
#ifdef HAS_ADDRESS_REGISTER_FILE
				else if (j == TYPE_ADR)
					printf("r%02d", rd->locals[i][j].regoff);
#endif
				else if ((j == TYPE_FLT) || (j == TYPE_DBL))
					printf("f%02d", rd->locals[i][j].regoff);
				else {
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
					if (IS_2_WORD_TYPE(j))
						printf(" %3s/%3s",
						    regs[GET_LOW_REG(rd->locals[i][j].regoff)],
                            regs[GET_HIGH_REG(rd->locals[i][j].regoff)]);
					else
#endif
						printf("%3s", regs[rd->locals[i][j].regoff]);
				}
			}
		printf("\n");
	}
	printf("\n");
#ifdef LSRA
	if (!opt_lsra) {
#endif
	printf("Interface Table:\n");
	for (i = 0; i < cd->maxstack; i++) {
		if ((rd->interfaces[i][0].type >= 0) ||
			(rd->interfaces[i][1].type >= 0) ||
		    (rd->interfaces[i][2].type >= 0) ||
			(rd->interfaces[i][3].type >= 0) ||
		    (rd->interfaces[i][4].type >= 0)) {
			printf("   %3d: ", i);
			for (j = TYPE_INT; j <= TYPE_ADR; j++)
				if (rd->interfaces[i][j].type >= 0) {
					printf("   (%s) ", jit_type[j]);
					if (rd->interfaces[i][j].flags & SAVEDVAR) {
						if (rd->interfaces[i][j].flags & INMEMORY)
							printf("M%2d", rd->interfaces[i][j].regoff);
#ifdef HAS_ADDRESS_REGISTER_FILE
						else if (j == TYPE_ADR)
							printf("R%02d", rd->interfaces[i][j].regoff);
#endif
						else if ((j == TYPE_FLT) || (j == TYPE_DBL))
							printf("F%02d", rd->interfaces[i][j].regoff);
						else {
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (IS_2_WORD_TYPE(j))
								printf(" %3s/%3s",
                             regs[GET_LOW_REG(rd->interfaces[i][j].regoff)],
                             regs[GET_HIGH_REG(rd->interfaces[i][j].regoff)]);
							else
#endif
								printf("%3s",regs[rd->interfaces[i][j].regoff]);
						}
					}
					else {
						if (rd->interfaces[i][j].flags & INMEMORY)
							printf("m%2d", rd->interfaces[i][j].regoff);
#ifdef HAS_ADDRESS_REGISTER_FILE
						else if (j == TYPE_ADR)
							printf("r%02d", rd->interfaces[i][j].regoff);
#endif
						else if ((j == TYPE_FLT) || (j == TYPE_DBL))
							printf("f%02d", rd->interfaces[i][j].regoff);
						else {
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (IS_2_WORD_TYPE(j))
								printf(" %3s/%3s",
                             regs[GET_LOW_REG(rd->interfaces[i][j].regoff)],
							 regs[GET_HIGH_REG(rd->interfaces[i][j].regoff)]);
							else
#endif
								printf("%3s",regs[rd->interfaces[i][j].regoff]);
						}
					}
				}
			printf("\n");
		}
	}
	printf("\n");
#ifdef LSRA
  	}
#endif
	if (showdisassemble) {
#if defined(__I386__) || defined(__X86_64__)
		u1 *u1ptr;
		s4 a;

		u1ptr = (u1 *) ((ptrint) m->mcode + cd->dseglen);
		for (i = 0; i < m->basicblocks[0].mpc;) {
			a = disassinstr(u1ptr);
			i += a;
			u1ptr += a;
		}
		printf("\n");
#elif defined(__XDSPCORE__)
		s4 *s4ptr;
		s4 a;

		s4ptr = (s4 *) ((ptrint) m->mcode + cd->dseglen);
		for (i = 0; i < m->basicblocks[0].mpc;) {
			a = disassinstr(stdout, s4ptr);
			printf("\n");
			i += a * 4;
			s4ptr += a;
		}
		printf("\n");
#else
		s4 *s4ptr;

		s4ptr = (s4 *) ((ptrint) m->mcode + cd->dseglen);
		for (i = 0; i < m->basicblocks[0].mpc; i += 4, s4ptr++) {
			disassinstr(s4ptr);
		}
		printf("\n");
#endif
	}
	
	for (bptr = m->basicblocks; bptr != NULL; bptr = bptr->next) {
		show_icmd_block(m, cd, bptr);
	}
}


void show_icmd_block(methodinfo *m, codegendata *cd, basicblock *bptr)
{
	int i, j;
	int deadcode;
	instruction *iptr;

	if (bptr->flags != BBDELETED) {
		deadcode = bptr->flags <= BBREACHED;
		printf("[");
		if (deadcode)
			for (j = cd->maxstack; j > 0; j--)
				printf(" ?  ");
		else
			icmd_print_stack(cd, bptr->instack);
		printf("] L%03d(%d - %d) flags=%d:\n", bptr->debug_nr, bptr->icount, bptr->pre_count,bptr->flags);
		iptr = bptr->iinstr;

		for (i = 0; i < bptr->icount; i++, iptr++) {
			printf("[");
			if (deadcode) {
				for (j = cd->maxstack; j > 0; j--)
					printf(" ?  ");
			}
			else
				icmd_print_stack(cd, iptr->dst);
			printf("]     %4d  ", i);

#ifdef LSRA_EDX
			if (icmd_uses_tmp[iptr->opc][0])
				printf("  ---");
			else
				printf("  EAX");
			if (icmd_uses_tmp[iptr->opc][1])
				printf(" ---");
			else
				printf(" ECX");
			if (icmd_uses_tmp[iptr->opc][2])
				printf(" ---  ");
			else
				printf(" EDX  ");
#endif

			show_icmd(iptr, deadcode);
			printf("\n");
		}

		if (showdisassemble && (!deadcode)) {
#if defined(__I386__) || defined(__X86_64__)
			u1 *u1ptr;
			s4 a;

			printf("\n");
			i = bptr->mpc;
			u1ptr = (u1 *) ((ptrint) m->mcode + cd->dseglen + i);

			if (bptr->next != NULL) {
				for (; i < bptr->next->mpc; ) {
					a = disassinstr(u1ptr);
					i += a;
					u1ptr += a;
				}
				printf("\n");

			} else {
				for (; u1ptr < (u1 *) ((ptrint) m->mcode + m->mcodelength); ) {
					a = disassinstr(u1ptr); 
					i += a;
					u1ptr += a;
				}
				printf("\n");
			}
#elif defined(__XDSPCORE__)
			s4 *s4ptr;
			s4 a;

			printf("\n");
			i = bptr->mpc;
			s4ptr = (s4 *) ((ptrint) m->mcode + cd->dseglen + i);

			if (bptr->next != NULL) {
				for (; i < bptr->next->mpc;) {
					a = disassinstr(stdout, s4ptr);
					printf("\n");
					i += a * 4;
					s4ptr += a;
				}
				printf("\n");

			} else {
				for (; s4ptr < (s4 *) ((ptrint) m->mcode + m->mcodelength); ) {
					a = disassinstr(stdout, s4ptr);
					i += a * 4;
					s4ptr += a;
				}
				printf("\n");
			}
#else
			s4 *s4ptr;

			printf("\n");
			i = bptr->mpc;
			s4ptr = (s4 *) ((ptrint) m->mcode + cd->dseglen + i);

			if (bptr->next != NULL) {
				for (; i < bptr->next->mpc; i += 4, s4ptr++)
					disassinstr(s4ptr);
				printf("\n");

			} else {
				for (; s4ptr < (s4 *) ((ptrint) m->mcode + m->mcodelength); i += 4, s4ptr++)
					disassinstr(s4ptr);
				printf("\n");
			}
#endif
		}
	}
}


void show_icmd(instruction *iptr, bool deadcode)
{
	int j;
	s4  *s4ptr;
	void **tptr = NULL;
	
	printf("%s", icmd_names[iptr->opc]);

	switch (iptr->opc) {
	case ICMD_IADDCONST:
	case ICMD_ISUBCONST:
	case ICMD_IMULCONST:
	case ICMD_IMULPOW2:
	case ICMD_IDIVPOW2:
	case ICMD_IREMPOW2:
	case ICMD_IANDCONST:
	case ICMD_IORCONST:
	case ICMD_IXORCONST:
	case ICMD_ISHLCONST:
	case ICMD_ISHRCONST:
	case ICMD_IUSHRCONST:
	case ICMD_LSHLCONST:
	case ICMD_LSHRCONST:
	case ICMD_LUSHRCONST:
	case ICMD_ICONST:
	case ICMD_ELSE_ICONST:
	case ICMD_IASTORECONST:
	case ICMD_BASTORECONST:
	case ICMD_CASTORECONST:
	case ICMD_SASTORECONST:
		printf(" %d (0x%08x)", iptr->val.i, iptr->val.i);
		break;

	case ICMD_IFEQ_ICONST:
	case ICMD_IFNE_ICONST:
	case ICMD_IFLT_ICONST:
	case ICMD_IFGE_ICONST:
	case ICMD_IFGT_ICONST:
	case ICMD_IFLE_ICONST:
		printf("(%d) %d", iptr[1].op1, iptr->val.i);
		break;

	case ICMD_LADDCONST:
	case ICMD_LSUBCONST:
	case ICMD_LMULCONST:
	case ICMD_LMULPOW2:
	case ICMD_LDIVPOW2:
	case ICMD_LREMPOW2:
	case ICMD_LANDCONST:
	case ICMD_LORCONST:
	case ICMD_LXORCONST:
	case ICMD_LCONST:
	case ICMD_LASTORECONST:
#if SIZEOF_VOID_P == 4
		printf(" %lld (0x%016llx)", iptr->val.l, iptr->val.l);
#else
		printf(" %ld (0x%016lx)", iptr->val.l, iptr->val.l);
#endif
		break;

	case ICMD_FCONST:
		printf(" %f", iptr->val.f);
		break;

	case ICMD_DCONST:
		printf(" %f", iptr->val.d);
		break;

	case ICMD_ACONST:
	case ICMD_AASTORECONST:
		printf(" %p", iptr->val.a);
		break;

	case ICMD_GETFIELD:
	case ICMD_PUTFIELD:
		if (iptr->val.a) 	 
			printf(" %d, ", ((fieldinfo *) iptr->val.a)->offset);
		else 	 
			printf(" (NOT RESOLVED), ");
		utf_display_classname(((unresolved_field *) iptr->target)->fieldref->classref->name);
		printf(".");
		utf_display(((unresolved_field *) iptr->target)->fieldref->name);
		printf(" (type ");
		utf_display(((unresolved_field *) iptr->target)->fieldref->descriptor);
		printf(")"); 
		break;

 	case ICMD_PUTSTATIC:
	case ICMD_GETSTATIC:
		if (iptr->val.a)
			printf(" ");
		else
			printf(" (NOT RESOLVED) ");
		utf_display_classname(((unresolved_field *) iptr->target)->fieldref->classref->name);
		printf(".");
		utf_display(((unresolved_field *) iptr->target)->fieldref->name);
		printf(" (type ");
		utf_display(((unresolved_field *) iptr->target)->fieldref->descriptor);
		printf(")");
		break;

	case ICMD_PUTSTATICCONST:
	case ICMD_PUTFIELDCONST:
		switch (iptr[1].op1) {
		case TYPE_INT:
			printf(" %d,", iptr->val.i);
			break;
		case TYPE_LNG:
#if SIZEOF_VOID_P == 4
			printf(" %lld,", iptr->val.l);
#else
			printf(" %ld,", iptr->val.l);
#endif
			break;
		case TYPE_ADR:
			printf(" %p,", iptr->val.a);
			break;
		case TYPE_FLT:
			printf(" %g,", iptr->val.f);
			break;
		case TYPE_DBL:
			printf(" %g,", iptr->val.d);
			break;
		}
		if (iptr->opc == ICMD_PUTFIELDCONST) 	 
			printf(" NOT RESOLVED,"); 	 
		printf(" "); 	 
		utf_display_classname(((unresolved_field *) iptr[1].target)->fieldref->classref->name); 	 
		printf("."); 	 
		utf_display(((unresolved_field *) iptr[1].target)->fieldref->name); 	 
		printf(" (type "); 	 
		utf_display(((unresolved_field *) iptr[1].target)->fieldref->descriptor); 	 
		printf(")"); 	 
		break;

	case ICMD_IINC:
		printf(" %d + %d", iptr->op1, iptr->val.i);
		break;

	case ICMD_IASTORE:
	case ICMD_SASTORE:
	case ICMD_BASTORE:
	case ICMD_CASTORE:
	case ICMD_LASTORE:
	case ICMD_DASTORE:
	case ICMD_FASTORE:
	case ICMD_AASTORE:

	case ICMD_IALOAD:
	case ICMD_SALOAD:
	case ICMD_BALOAD:
	case ICMD_CALOAD:
	case ICMD_LALOAD:
	case ICMD_DALOAD:
	case ICMD_FALOAD:
	case ICMD_AALOAD:
		if (iptr->op1 != 0)
			printf("(opt.)");
		break;

	case ICMD_RET:
	case ICMD_ILOAD:
	case ICMD_LLOAD:
	case ICMD_FLOAD:
	case ICMD_DLOAD:
	case ICMD_ALOAD:
	case ICMD_ISTORE:
	case ICMD_LSTORE:
	case ICMD_FSTORE:
	case ICMD_DSTORE:
	case ICMD_ASTORE:
		printf(" %d", iptr->op1);
		break;

	case ICMD_NEW:
		printf(" ");
		utf_display_classname(((classinfo *) iptr->val.a)->name);
		break;

	case ICMD_NEWARRAY:
		switch (iptr->op1) {
		case 4:
			printf(" boolean");
			break;
		case 5:
			printf(" char");
			break;
		case 6:
			printf(" float");
			break;
		case 7:
			printf(" double");
			break;
		case 8:
			printf(" byte");
			break;
		case 9:
			printf(" short");
			break;
		case 10:
			printf(" int");
			break;
		case 11:
			printf(" long");
			break;
		}
		break;

	case ICMD_ANEWARRAY:
		if (iptr->op1) {
			printf(" ");
			utf_display_classname(((classinfo *) iptr->val.a)->name);
		}
		break;

	case ICMD_MULTIANEWARRAY:
		if (iptr->target) {
			printf(" (NOT RESOLVED) %d ",iptr->op1);
			utf_display(((constant_classref *) iptr->val.a)->name);
		} else {
			printf(" %d ",iptr->op1);
			utf_display_classname(((vftbl_t *) iptr->val.a)->class->name);
		}
		break;

	case ICMD_CHECKCAST:
	case ICMD_INSTANCEOF:
		if (iptr->op1) {
			classinfo *c = iptr->val.a;
			if (c) {
				if (c->flags & ACC_INTERFACE)
					printf(" (INTERFACE) ");
				else
					printf(" (CLASS,%3d) ", c->vftbl->diffval);
			} else {
				printf(" (NOT RESOLVED) ");
			}
			utf_display_classname(((constant_classref *) iptr->target)->name);
		}
		break;

	case ICMD_INLINE_START:
		printf("\t\t\t%s.%s%s depth=%i",iptr->method->class->name->text,iptr->method->name->text,iptr->method->descriptor->text, iptr->op1);
		break;
	case ICMD_INLINE_END:
		break;

	case ICMD_BUILTIN:
		printf(" %s", ((builtintable_entry *) iptr->val.a)->name);
		break;

	case ICMD_INVOKEVIRTUAL:
	case ICMD_INVOKESPECIAL:
	case ICMD_INVOKESTATIC:
	case ICMD_INVOKEINTERFACE:
		if (!iptr->val.a)
			printf(" (NOT RESOLVED) ");
		else
			printf(" ");
		utf_display_classname(((unresolved_method *) iptr->target)->methodref->classref->name);
		printf(".");
		utf_display(((unresolved_method *) iptr->target)->methodref->name);
		utf_display(((unresolved_method *) iptr->target)->methodref->descriptor);
		break;

	case ICMD_IFEQ:
	case ICMD_IFNE:
	case ICMD_IFLT:
	case ICMD_IFGE:
	case ICMD_IFGT:
	case ICMD_IFLE:
		if (deadcode || !iptr->target)
			printf("(%d) op1=%d", iptr->val.i, iptr->op1);
		else
			printf("(%d) L%03d", iptr->val.i, ((basicblock *) iptr->target)->debug_nr);
		break;

	case ICMD_IF_LEQ:
	case ICMD_IF_LNE:
	case ICMD_IF_LLT:
	case ICMD_IF_LGE:
	case ICMD_IF_LGT:
	case ICMD_IF_LLE:
		if (deadcode || !iptr->target)
#if SIZEOF_VOID_P == 4
			printf("(%lld) op1=%d", iptr->val.l, iptr->op1);
#else
			printf("(%ld) op1=%d", iptr->val.l, iptr->op1);
#endif
		else
#if SIZEOF_VOID_P == 4
			printf("(%lld) L%03d", iptr->val.l, ((basicblock *) iptr->target)->debug_nr);
#else
			printf("(%ld) L%03d", iptr->val.l, ((basicblock *) iptr->target)->debug_nr);
#endif
		break;

	case ICMD_JSR:
	case ICMD_GOTO:
	case ICMD_IFNULL:
	case ICMD_IFNONNULL:
	case ICMD_IF_ICMPEQ:
	case ICMD_IF_ICMPNE:
	case ICMD_IF_ICMPLT:
	case ICMD_IF_ICMPGE:
	case ICMD_IF_ICMPGT:
	case ICMD_IF_ICMPLE:
	case ICMD_IF_LCMPEQ:
	case ICMD_IF_LCMPNE:
	case ICMD_IF_LCMPLT:
	case ICMD_IF_LCMPGE:
	case ICMD_IF_LCMPGT:
	case ICMD_IF_LCMPLE:
	case ICMD_IF_ACMPEQ:
	case ICMD_IF_ACMPNE:
		if (deadcode || !iptr->target)
			printf(" op1=%d", iptr->op1);
		else
			printf(" L%03d", ((basicblock *) iptr->target)->debug_nr);
		break;

	case ICMD_TABLESWITCH:
		s4ptr = (s4*)iptr->val.a;

		if (deadcode || !iptr->target) {
			printf(" %d;", *s4ptr);
		}
		else {
			tptr = (void **) iptr->target;
			printf(" L%03d;", ((basicblock *) *tptr)->debug_nr); 
			tptr++;
		}

		s4ptr++;         /* skip default */
		j = *s4ptr++;                               /* low     */
		j = *s4ptr++ - j;                           /* high    */
		while (j >= 0) {
			if (deadcode || !*tptr)
				printf(" %d", *s4ptr++);
			else {
				printf(" L%03d", ((basicblock *) *tptr)->debug_nr);
				tptr++;
			}
			j--;
		}
		break;

	case ICMD_LOOKUPSWITCH:
		s4ptr = (s4*)iptr->val.a;

		if (deadcode || !iptr->target) {
			printf(" %d;", *s4ptr);
		}
		else {
			tptr = (void **) iptr->target;
			printf(" L%03d;", ((basicblock *) *tptr)->debug_nr);
			tptr++;
		}
		s4ptr++;                                         /* default */
		j = *s4ptr++;                                    /* count   */

		while (--j >= 0) {
			if (deadcode || !*tptr) {
				s4ptr++; /* skip value */
				printf(" %d",*s4ptr++);
			}
			else {
				printf(" L%03d", ((basicblock *) *tptr)->debug_nr);
				tptr++;
			}
		}
		break;
	}
  	printf(" Line number: %d, method:",iptr->line);
/*        printf("\t\t");
  	utf_display(iptr->method->class->name); 
  	printf("."); 
  	utf_display(iptr->method->name); */
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
