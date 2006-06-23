/* src/vm/jit/show.c - showing the intermediate representation

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

   Changes: Edwin Steiner
            Christian Thalinger
            Christian Ullrich

   $Id$

*/


#include "config.h"
#include "vm/types.h"

#include <assert.h>

#include "mm/memory.h"
#include "vm/global.h"
#include "vm/options.h"
#include "vm/builtin.h"
#include "vm/stringlocal.h"
#include "vm/jit/jit.h"
#include "vm/jit/show.h"
#include "vm/jit/disass.h"

#if defined(ENABLE_THREADS)
#include "threads/native/lock.h"
#endif

/* global variables ***********************************************************/

#if defined(ENABLE_THREADS) && !defined(NDEBUG)
static java_objectheader *show_global_lock;
#endif


/* show_init *******************************************************************

   Initialized the show subsystem (called by jit_init).

*******************************************************************************/

#if !defined(NDEBUG)
bool show_init(void)
{
#if defined(ENABLE_THREADS)
	/* initialize the show lock */

	show_global_lock = NEW(java_objectheader);

	lock_init_object_lock(show_global_lock);
#endif

	/* everything's ok */

	return true;
}
#endif


/* show_print_stack ************************************************************

   Print the stack representation starting with the given top stackptr.

   NOTE: Currently this function may only be called after register allocation!

*******************************************************************************/

#if !defined(NDEBUG)
static void show_print_stack(codegendata *cd, stackptr s)
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
					if (IS_2_WORD_TYPE(s->type)) {
# if defined(ENABLE_JIT) && defined(ENABLE_DISASSEMBLER)
#  if defined(ENABLE_INTRP)
						if (opt_intrp)
							printf(" %3d/%3d", GET_LOW_REG(s->regoff),
								   GET_HIGH_REG(s->regoff));
						else
#  endif
							printf(" %3s/%3s", regs[GET_LOW_REG(s->regoff)],
								   regs[GET_HIGH_REG(s->regoff)]);
# else
						printf(" %3d/%3d", GET_LOW_REG(s->regoff),
							   GET_HIGH_REG(s->regoff));
# endif
					} 
					else 
#endif /* defined(SUPPORT_COMBINE_INTEGER_REGISTERS) */
						{
#if defined(ENABLE_JIT) && defined(ENABLE_DISASSEMBLER)
# if defined(ENABLE_INTRP)
							if (opt_intrp)
								printf(" %3d", s->regoff);
							else
# endif
								printf(" %3s", regs[s->regoff]);
#else
							printf(" %3d", s->regoff);
#endif
						}
				}
				break;
			case STACKVAR:
				printf(" I%02d", s->varnum);
				break;
			case LOCALVAR:
				printf(" L%02d", s->varnum);
				break;
			case ARGVAR:
				if (s->varnum == -1) {
					/* Return Value                                  */
					/* varkind ARGVAR "misused for this special case */
					printf("  V0");
				} 
				else /* "normal" Argvar */
					printf(" A%02d", s->varnum);
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
					if (IS_2_WORD_TYPE(s->type)) {
# if defined(ENABLE_JIT) && defined(ENABLE_DISASSEMBLER)
#  if defined(ENABLE_INTRP)
						if (opt_intrp)
							printf(" %3d/%3d", GET_LOW_REG(s->regoff),
								   GET_HIGH_REG(s->regoff));
						else
#  endif
							printf(" %3s/%3s", regs[GET_LOW_REG(s->regoff)],
								   regs[GET_HIGH_REG(s->regoff)]);
# else
						printf(" %3d/%3d", GET_LOW_REG(s->regoff),
							   GET_HIGH_REG(s->regoff));
# endif
					} 
					else
#endif /* defined(SUPPORT_COMBINE_INTEGER_REGISTERS) */
						{
#if defined(ENABLE_JIT) && defined(ENABLE_DISASSEMBLER)
# if defined(ENABLE_INTRP)
							if (opt_intrp)
								printf(" %3d", s->regoff);
							else
# endif
								printf(" %3s", regs[s->regoff]);
#else
							printf(" %3d", s->regoff);
#endif
						}
				}
				break;
			case STACKVAR:
				printf(" i%02d", s->varnum);
				break;
			case LOCALVAR:
				printf(" l%02d", s->varnum);
				break;
			case ARGVAR:
				if (s->varnum == -1) {
					/* Return Value                                  */
					/* varkind ARGVAR "misused for this special case */
					printf("  v0");
				} 
				else /* "normal" Argvar */
				printf(" a%02d", s->varnum);
				break;
			default:
				printf(" ?%02d", j);
			}
		s = s->prev;
	}
}
#endif /* !defined(NDEBUG) */


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


#if !defined(NDEBUG)
static char *jit_type[] = {
	"int",
	"lng",
	"flt",
	"dbl",
	"adr"
};
#endif


/* show_method *****************************************************************

   Print the intermediate representation of a method.

   NOTE: Currently this function may only be called after register allocation!

*******************************************************************************/

#if !defined(NDEBUG)
void new_show_method(jitdata *jd, int stage)
{
	methodinfo     *m;
	codeinfo       *code;
	codegendata    *cd;
	registerdata   *rd;
	basicblock     *bptr;
	exceptiontable *ex;
	s4              i, j;
	u1             *u1ptr;

	/* get required compiler data */

	m    = jd->m;
	code = jd->code;
	cd   = jd->cd;
	rd   = jd->new_rd;

#if defined(ENABLE_THREADS)
	/* We need to enter a lock here, since the binutils disassembler
	   is not reentrant-able and we could not read functions printed
	   at the same time. */

	builtin_monitorenter(show_global_lock);
#endif

	printf("\n");

	method_println(m);

	printf("\n(NEW INSTRUCTION FORMAT)\n");
	printf("\nBasic blocks: %d\n", jd->new_basicblockcount);
	printf("Max locals:   %d\n", cd->maxlocals);
	printf("Max stack:    %d\n", cd->maxstack);
	printf("Line number table length: %d\n", m->linenumbercount);

	if (stage >= SHOW_PARSE) {
		printf("Exceptions (Number: %d):\n", cd->exceptiontablelength);
		for (ex = cd->exceptiontable; ex != NULL; ex = ex->down) {
			printf("    L%03d ... ", ex->start->debug_nr );
			printf("L%03d  = ", ex->end->debug_nr);
			printf("L%03d", ex->handler->debug_nr);
			printf("  (catchtype: ");
			if (ex->catchtype.any)
				if (IS_CLASSREF(ex->catchtype))
					utf_display_printable_ascii_classname(ex->catchtype.ref->name);
				else
					utf_display_printable_ascii_classname(ex->catchtype.cls->name);
			else
				printf("ANY");
			printf(")\n");
		}
	}
	
	if (stage >= SHOW_PARSE && rd) {
	printf("Local Table:\n");
	for (i = 0; i < cd->maxlocals; i++) {
		printf("   %3d: ", i);

#if defined(ENABLE_JIT) && defined(ENABLE_DISASSEMBLER)
		for (j = TYPE_INT; j <= TYPE_ADR; j++) {
# if defined(ENABLE_INTRP)
			if (!opt_intrp) {
# endif
				if (rd->locals[i][j].type >= 0) {
					printf("   (%s) ", jit_type[j]);
					if (stage >= SHOW_REGS) {
						if (rd->locals[i][j].flags & INMEMORY)
							printf("m%2d", rd->locals[i][j].regoff);
# ifdef HAS_ADDRESS_REGISTER_FILE
						else if (j == TYPE_ADR)
							printf("r%02d", rd->locals[i][j].regoff);
# endif
						else if ((j == TYPE_FLT) || (j == TYPE_DBL))
							printf("f%02d", rd->locals[i][j].regoff);
						else {
# if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (IS_2_WORD_TYPE(j))
								printf(" %3s/%3s",
									   regs[GET_LOW_REG(rd->locals[i][j].regoff)],
									   regs[GET_HIGH_REG(rd->locals[i][j].regoff)]);
							else
# endif
								printf("%3s", regs[rd->locals[i][j].regoff]);
						}
					}
				}
# if defined(ENABLE_INTRP)
			}
# endif
		}
#endif /* defined(ENABLE_JIT) && defined(ENABLE_DISASSEMBLER) */

		printf("\n");
	}
	printf("\n");
	}

	if (stage >= SHOW_STACK && rd) {
#if defined(ENABLE_LSRA)
	if (!opt_lsra) {
#endif
#if defined(ENABLE_INTRP)
		if (!opt_intrp) {
#endif
	printf("Interface Table:\n");
	for (i = 0; i < cd->maxstack; i++) {
		if ((rd->interfaces[i][0].type >= 0) ||
			(rd->interfaces[i][1].type >= 0) ||
		    (rd->interfaces[i][2].type >= 0) ||
			(rd->interfaces[i][3].type >= 0) ||
		    (rd->interfaces[i][4].type >= 0)) {
			printf("   %3d: ", i);

#if defined(ENABLE_JIT) && defined(ENABLE_DISASSEMBLER)
# if defined(ENABLE_INTRP)
			if (!opt_intrp) {
# endif
				for (j = TYPE_INT; j <= TYPE_ADR; j++) {
					if (rd->interfaces[i][j].type >= 0) {
						printf("   (%s) ", jit_type[j]);
						if (stage >= SHOW_REGS) {
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
					}
				}
				printf("\n");
# if defined(ENABLE_INTRP)
			}
# endif
#endif /* defined(ENABLE_JIT) && defined(ENABLE_DISASSEMBLER) */

		}
	}
	printf("\n");

#if defined(ENABLE_INTRP)
		}
#endif
#if defined(ENABLE_LSRA)
  	}
#endif
	} /* if >= SHOW_STACK */

	if (code->rplpoints) {
		printf("Replacement Points:\n");
		replace_show_replacement_points(code);
		printf("\n");
	}

#if defined(ENABLE_DISASSEMBLER)
	/* show code before first basic block */

	if ((stage >= SHOW_CODE) && JITDATA_HAS_FLAG_SHOWDISASSEMBLE(jd)) {
		u1ptr = (u1 *) ((ptrint) code->mcode + cd->dseglen);

		for (; u1ptr < (u1 *) ((ptrint) code->mcode + cd->dseglen + jd->new_basicblocks[0].mpc);)
			DISASSINSTR(u1ptr);

		printf("\n");
	}
#endif

	/* show code of all basic blocks */

	for (bptr = jd->new_basicblocks; bptr != NULL; bptr = bptr->next)
		new_show_basicblock(jd, bptr, stage);

#if defined(ENABLE_DISASSEMBLER)
	/* show stubs code */

	if (stage >= SHOW_CODE && opt_showdisassemble && opt_showexceptionstubs) {
		printf("\nException stubs code:\n");
		printf("Length: %d\n\n", (s4) (code->mcodelength -
									   ((ptrint) cd->dseglen +
										jd->new_basicblocks[jd->new_basicblockcount].mpc)));

		u1ptr = (u1 *) ((ptrint) code->mcode + cd->dseglen +
						jd->new_basicblocks[jd->new_basicblockcount].mpc);

		for (; (ptrint) u1ptr < ((ptrint) code->mcode + code->mcodelength);)
			DISASSINSTR(u1ptr);

		printf("\n");
	}
#endif

#if defined(ENABLE_THREADS)
	builtin_monitorexit(show_global_lock);
#endif

	/* finally flush the output */

	fflush(stdout);
}
#endif /* !defined(NDEBUG) */

#if !defined(NDEBUG)
void show_method(jitdata *jd)
{
	methodinfo     *m;
	codeinfo       *code;
	codegendata    *cd;
	registerdata   *rd;
	basicblock     *bptr;
	exceptiontable *ex;
	s4              i, j;
	u1             *u1ptr;

	/* get required compiler data */

	m    = jd->m;
	code = jd->code;
	cd   = jd->cd;
	rd   = jd->rd;

#if defined(ENABLE_THREADS)
	/* We need to enter a lock here, since the binutils disassembler
	   is not reentrant-able and we could not read functions printed
	   at the same time. */

	builtin_monitorenter(show_global_lock);
#endif

	printf("\n");

	method_println(m);

	printf("\nBasic blocks: %d\n", m->basicblockcount);
	printf("Max locals:   %d\n", cd->maxlocals);
	printf("Max stack:    %d\n", cd->maxstack);
	printf("Line number table length: %d\n", m->linenumbercount);

	printf("Exceptions (Number: %d):\n", cd->exceptiontablelength);
	for (ex = cd->exceptiontable; ex != NULL; ex = ex->down) {
		printf("    L%03d ... ", ex->start->debug_nr );
		printf("L%03d  = ", ex->end->debug_nr);
		printf("L%03d", ex->handler->debug_nr);
		printf("  (catchtype: ");
		if (ex->catchtype.any)
			if (IS_CLASSREF(ex->catchtype))
				utf_display_printable_ascii_classname(ex->catchtype.ref->name);
			else
				utf_display_printable_ascii_classname(ex->catchtype.cls->name);
		else
			printf("ANY");
		printf(")\n");
	}
	
	printf("Local Table:\n");
	for (i = 0; i < cd->maxlocals; i++) {
		printf("   %3d: ", i);

#if defined(ENABLE_JIT) && defined(ENABLE_DISASSEMBLER)
		for (j = TYPE_INT; j <= TYPE_ADR; j++) {
# if defined(ENABLE_INTRP)
			if (!opt_intrp) {
# endif
				if (rd->locals[i][j].type >= 0) {
					printf("   (%s) ", jit_type[j]);
					if (rd->locals[i][j].flags & INMEMORY)
						printf("m%2d", rd->locals[i][j].regoff);
# ifdef HAS_ADDRESS_REGISTER_FILE
					else if (j == TYPE_ADR)
						printf("r%02d", rd->locals[i][j].regoff);
# endif
					else if ((j == TYPE_FLT) || (j == TYPE_DBL))
						printf("f%02d", rd->locals[i][j].regoff);
					else {
# if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
						if (IS_2_WORD_TYPE(j))
							printf(" %3s/%3s",
								   regs[GET_LOW_REG(rd->locals[i][j].regoff)],
								   regs[GET_HIGH_REG(rd->locals[i][j].regoff)]);
						else
# endif
							printf("%3s", regs[rd->locals[i][j].regoff]);
					}
				}
# if defined(ENABLE_INTRP)
			}
# endif
		}
#endif /* defined(ENABLE_JIT) && defined(ENABLE_DISASSEMBLER) */

		printf("\n");
	}
	printf("\n");

#if defined(ENABLE_LSRA)
	if (!opt_lsra) {
#endif
#if defined(ENABLE_INTRP)
		if (!opt_intrp) {
#endif
	printf("Interface Table:\n");
	for (i = 0; i < cd->maxstack; i++) {
		if ((rd->interfaces[i][0].type >= 0) ||
			(rd->interfaces[i][1].type >= 0) ||
		    (rd->interfaces[i][2].type >= 0) ||
			(rd->interfaces[i][3].type >= 0) ||
		    (rd->interfaces[i][4].type >= 0)) {
			printf("   %3d: ", i);

#if defined(ENABLE_JIT) && defined(ENABLE_DISASSEMBLER)
# if defined(ENABLE_INTRP)
			if (!opt_intrp) {
# endif
				for (j = TYPE_INT; j <= TYPE_ADR; j++) {
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
				}
				printf("\n");
# if defined(ENABLE_INTRP)
			}
# endif
#endif /* defined(ENABLE_JIT) && defined(ENABLE_DISASSEMBLER) */

		}
	}
	printf("\n");

#if defined(ENABLE_INTRP)
		}
#endif
#if defined(ENABLE_LSRA)
  	}
#endif

	if (code->rplpoints) {
		printf("Replacement Points:\n");
		replace_show_replacement_points(code);
		printf("\n");
	}

#if defined(ENABLE_DISASSEMBLER)
	/* show code before first basic block */

	if (JITDATA_HAS_FLAG_SHOWDISASSEMBLE(jd)) {
		u1ptr = (u1 *) ((ptrint) code->mcode + cd->dseglen);

		for (; u1ptr < (u1 *) ((ptrint) code->mcode + cd->dseglen + m->basicblocks[0].mpc);)
			DISASSINSTR(u1ptr);

		printf("\n");
	}
#endif

	/* show code of all basic blocks */

	for (bptr = m->basicblocks; bptr != NULL; bptr = bptr->next)
		show_basicblock(jd, bptr);

#if defined(ENABLE_DISASSEMBLER)
	/* show stubs code */

	if (JITDATA_HAS_FLAG_SHOWDISASSEMBLE(jd) && opt_showexceptionstubs) {
		printf("\nException stubs code:\n");
		printf("Length: %d\n\n", (s4) (code->mcodelength -
									   ((ptrint) cd->dseglen +
										m->basicblocks[m->basicblockcount].mpc)));

		u1ptr = (u1 *) ((ptrint) code->mcode + cd->dseglen +
						m->basicblocks[m->basicblockcount].mpc);

		for (; (ptrint) u1ptr < ((ptrint) code->mcode + code->mcodelength);)
			DISASSINSTR(u1ptr);

		printf("\n");
	}
#endif

#if defined(ENABLE_THREADS)
	builtin_monitorexit(show_global_lock);
#endif

	/* finally flush the output */

	fflush(stdout);
}
#endif /* !defined(NDEBUG) */


/* show_basicblock *************************************************************

   Print the intermediate representation of a basic block.

   NOTE: Currently this function may only be called after register allocation!

*******************************************************************************/

#if !defined(NDEBUG)
void new_show_basicblock(jitdata *jd, basicblock *bptr, int stage)
{
	methodinfo  *m;
	codeinfo    *code;
	codegendata *cd;
	s4           i;
	bool         deadcode;
	new_instruction *iptr;
	u1          *u1ptr;

	/* get required compiler data */

	m    = jd->m;
	code = jd->code;
	cd   = jd->cd;

	if (bptr->flags != BBDELETED) {
		deadcode = bptr->flags <= BBREACHED;

		printf("======== %sL%03d ======== (flags: %d, bitflags: %01x, next: %d, type: ",
				(bptr->bitflags & BBFLAG_REPLACEMENT) ? "<REPLACE> " : "",
			   bptr->debug_nr, bptr->flags, bptr->bitflags, 
			   (bptr->next) ? (bptr->next->debug_nr) : -1);

		switch (bptr->type) {
		case BBTYPE_STD:
			printf("STD");
			break;
		case BBTYPE_EXH:
			printf("EXH");
			break;
		case BBTYPE_SBR:
			printf("SBR");
			break;
		}

		printf(", instruction count: %d, predecessors: %d):\n",
			   bptr->icount, bptr->pre_count);

		iptr = /*XXX*/ (new_instruction *) bptr->iinstr;

		for (i = 0; i < bptr->icount; i++, iptr++) {
			printf("%4d:  ", iptr->line);

			new_show_icmd(jd, iptr, deadcode, stage);
			printf("\n");
		}

#if defined(ENABLE_DISASSEMBLER)
		if ((stage >= SHOW_CODE) && JITDATA_HAS_FLAG_SHOWDISASSEMBLE(jd) &&
			(!deadcode)) {
			printf("\n");
			u1ptr = (u1 *) ((ptrint) code->mcode + cd->dseglen + bptr->mpc);

			if (bptr->next != NULL) {
				for (; u1ptr < (u1 *) ((ptrint) code->mcode + cd->dseglen + bptr->next->mpc);)
					DISASSINSTR(u1ptr);

			} 
			else {
				for (; u1ptr < (u1 *) ((ptrint) code->mcode + code->mcodelength);)
					DISASSINSTR(u1ptr); 
			}
			printf("\n");
		}
#endif
	}
}
#endif /* !defined(NDEBUG) */

#if !defined(NDEBUG)
void show_basicblock(jitdata *jd, basicblock *bptr)
{
	methodinfo  *m;
	codeinfo    *code;
	codegendata *cd;
	s4           i, j;
	bool         deadcode;
	instruction *iptr;
	u1          *u1ptr;

	/* get required compiler data */

	m    = jd->m;
	code = jd->code;
	cd   = jd->cd;

	if (bptr->flags != BBDELETED) {
		deadcode = bptr->flags <= BBREACHED;

		printf("[");

		if (deadcode)
			for (j = cd->maxstack; j > 0; j--)
				printf(" ?  ");
		else
			show_print_stack(cd, bptr->instack);

		printf("] %sL%03d(flags: %d, bitflags: %01x, next: %d, type: ",
				(bptr->bitflags & BBFLAG_REPLACEMENT) ? "<REPLACE> " : "",
			   bptr->debug_nr, bptr->flags, bptr->bitflags, 
			   (bptr->next) ? (bptr->next->debug_nr) : -1);

		switch (bptr->type) {
		case BBTYPE_STD:
			printf("STD");
			break;
		case BBTYPE_EXH:
			printf("EXH");
			break;
		case BBTYPE_SBR:
			printf("SBR");
			break;
		}

		printf(", instruction count: %d, predecessors: %d):\n",
			   bptr->icount, bptr->pre_count);

		iptr = bptr->iinstr;

		for (i = 0; i < bptr->icount; i++, iptr++) {
			printf("[");

			if (deadcode)
				for (j = cd->maxstack; j > 0; j--)
					printf(" ?  ");
			else
				show_print_stack(cd, iptr->dst);

			printf("] %5d (line: %5d)  ", i, iptr->line);

			show_icmd(iptr, deadcode);
			printf("\n");
		}

#if defined(ENABLE_DISASSEMBLER)
		if (JITDATA_HAS_FLAG_SHOWDISASSEMBLE(jd) && (!deadcode)) {
			printf("\n");
			u1ptr = (u1 *) ((ptrint) code->mcode + cd->dseglen + bptr->mpc);

			if (bptr->next != NULL) {
				for (; u1ptr < (u1 *) ((ptrint) code->mcode + cd->dseglen + bptr->next->mpc);)
					DISASSINSTR(u1ptr);

			} 
			else {
				for (; u1ptr < (u1 *) ((ptrint) code->mcode + code->mcodelength);)
					DISASSINSTR(u1ptr); 
			}
			printf("\n");
		}
#endif
	}
}
#endif /* !defined(NDEBUG) */


/* show_icmd *******************************************************************

   Print the intermediate representation of an instruction.

   NOTE: Currently this function may only be called after register allocation!

*******************************************************************************/

#if !defined(NDEBUG)

#define SHOW_TARGET(target)                                          \
        if (stage >= SHOW_STACK) {                                   \
            printf("--> L%03d ", (target).block->debug_nr);          \
        }                                                            \
        else if (stage >= SHOW_PARSE) {                              \
            printf("--> insindex %d (L%03d) ", (target).insindex,    \
                jd->new_basicblocks[jd->new_basicblockindex[         \
                (target).insindex]].debug_nr);                       \
        }                                                            \
        else {                                                       \
            printf("--> insindex %d ", (target).insindex);           \
        }

#define SHOW_INT_CONST(val)                                          \
        if (stage >= SHOW_PARSE) {                                   \
            printf("%ld ", (long) (val));                            \
        }                                                            \
        else {                                                       \
            printf("iconst ");                                       \
        }

#define SHOW_LNG_CONST(val)                                          \
        if (stage >= SHOW_PARSE) {                                   \
            printf("%lld ", (long long)(val));                       \
        }                                                            \
        else {                                                       \
            printf("lconst ");                                       \
        }

#define SHOW_FLT_CONST(val)                                          \
        if (stage >= SHOW_PARSE) {                                   \
            printf("%g ", (val));                                    \
        }                                                            \
        else {                                                       \
            printf("fconst ");                                       \
        }

#define SHOW_DBL_CONST(val)                                          \
        if (stage >= SHOW_PARSE) {                                   \
            printf("%g ", (val));                                    \
        }                                                            \
        else {                                                       \
            printf("dconst ");                                       \
        }

#define SHOW_INDEX(index)                                            \
        if (stage >= SHOW_PARSE) {                                   \
            printf("%d ", index);                                    \
        }                                                            \
        else {                                                       \
            printf("index");                                         \
        }

#define SHOW_STRING(val)                                             \
        if (stage >= SHOW_PARSE) {                                   \
            putchar('"');                                            \
            utf_display_printable_ascii(                             \
                javastring_toutf((java_lang_String *)(val), false)); \
            printf("\" ");                                           \
        }                                                            \
        else {                                                       \
            printf("string ");                                       \
        }

#define SHOW_CLASSREF_OR_CLASSINFO(c)                                \
        if (stage >= SHOW_PARSE) {                                   \
            if (IS_CLASSREF(c))                                      \
                class_classref_print(c.ref);                         \
            else                                                     \
                class_print(c.cls);                                  \
            putchar(' ');                                            \
        }                                                            \
        else {                                                       \
            printf("class ");                                        \
        }

#define SHOW_FIELD(fmiref)                                           \
        if (stage >= SHOW_PARSE) {                                   \
            field_fieldref_print(fmiref);                            \
            putchar(' ');                                            \
        }                                                            \
        else {                                                       \
            printf("field ");                                        \
        }

#define SHOW_STACKVAR(sp)                                            \
        new_show_stackvar(jd, (sp), stage)

#define SHOW_S1(iptr)                                                \
        if (stage >= SHOW_STACK) {                                   \
            SHOW_STACKVAR(iptr->s1.var);                             \
        }

#define SHOW_S2(iptr)                                                \
        if (stage >= SHOW_STACK) {                                   \
            SHOW_STACKVAR(iptr->sx.s23.s2.var);                      \
        }

#define SHOW_S3(iptr)                                                \
        if (stage >= SHOW_STACK) {                                   \
            SHOW_STACKVAR(iptr->sx.s23.s3.var);                      \
        }

#define SHOW_DST(iptr)                                               \
        if (stage >= SHOW_STACK) {                                   \
            printf("=> ");                                           \
            SHOW_STACKVAR(iptr->dst.var);                            \
        }

#define SHOW_S1_LOCAL(iptr)                                          \
        if (stage >= SHOW_STACK) {                                   \
            printf("L%d ", iptr->s1.localindex);                     \
        }

#define SHOW_DST_LOCAL(iptr)                                         \
        if (stage >= SHOW_STACK) {                                   \
            printf("=> L%d ", iptr->dst.localindex);                 \
        }

static void new_show_stackvar(jitdata *jd, stackptr sp, int stage)
{
	char type;

	switch (sp->type) {
		case TYPE_INT: type = 'i'; break;
		case TYPE_LNG: type = 'l'; break;
		case TYPE_FLT: type = 'f'; break;
		case TYPE_DBL: type = 'd'; break;
		case TYPE_ADR: type = 'a'; break;
		default:       type = '?';
	}
	printf("S%c%d", type, (int) (sp - jd->new_stack));

	if (stage >= SHOW_REGS) {
		putchar('(');

		if (sp->flags & SAVEDVAR) {
			switch (sp->varkind) {
			case TEMPVAR:
				if (sp->flags & INMEMORY)
					printf("M%02d", sp->regoff);
#ifdef HAS_ADDRESS_REGISTER_FILE
				else if (sp->type == TYPE_ADR)
					printf("R%02d", sp->regoff);
#endif
				else if (IS_FLT_DBL_TYPE(sp->type))
					printf("F%02d", sp->regoff);
				else {
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
					if (IS_2_WORD_TYPE(sp->type)) {
# if defined(ENABLE_JIT) && defined(ENABLE_DISASSEMBLER)
#  if defined(ENABLE_INTRP)
						if (opt_intrp)
							printf("%3d/%3d", GET_LOW_REG(sp->regoff),
								   GET_HIGH_REG(sp->regoff));
						else
#  endif
							printf("%3s/%3s", regs[GET_LOW_REG(sp->regoff)],
								   regs[GET_HIGH_REG(sp->regoff)]);
# else
						printf("%3d/%3d", GET_LOW_REG(sp->regoff),
							   GET_HIGH_REG(sp->regoff));
# endif
					} 
					else 
#endif /* defined(SUPPORT_COMBINE_INTEGER_REGISTERS) */
						{
#if defined(ENABLE_JIT) && defined(ENABLE_DISASSEMBLER)
# if defined(ENABLE_INTRP)
							if (opt_intrp)
								printf("%3d", sp->regoff);
							else
# endif
								printf("%3s", regs[sp->regoff]);
#else
							printf("%3d", sp->regoff);
#endif
						}
				}
				break;
			case STACKVAR:
				printf("I%02d", sp->varnum);
				break;
			case LOCALVAR:
				printf("L%02d", sp->varnum);
				break;
			case ARGVAR:
				if (sp->varnum == -1) {
					/* Return Value                                  */
					/* varkind ARGVAR "misused for this special case */
					printf(" V0");
				} 
				else /* "normal" Argvar */
					printf("A%02d", sp->varnum);
				break;
			default:
				printf("!xx");
			}
		}
		else { /* not SAVEDVAR */
			switch (sp->varkind) {
			case TEMPVAR:
				if (sp->flags & INMEMORY)
					printf("m%02d", sp->regoff);
#ifdef HAS_ADDRESS_REGISTER_FILE
				else if (sp->type == TYPE_ADR)
					printf("r%02d", sp->regoff);
#endif
				else if (IS_FLT_DBL_TYPE(sp->type))
					printf("f%02d", sp->regoff);
				else {
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
					if (IS_2_WORD_TYPE(sp->type)) {
# if defined(ENABLE_JIT) && defined(ENABLE_DISASSEMBLER)
#  if defined(ENABLE_INTRP)
						if (opt_intrp)
							printf("%3d/%3d", GET_LOW_REG(sp->regoff),
								   GET_HIGH_REG(sp->regoff));
						else
#  endif
							printf("%3s/%3s", regs[GET_LOW_REG(sp->regoff)],
								   regs[GET_HIGH_REG(sp->regoff)]);
# else
						printf("%3d/%3d", GET_LOW_REG(sp->regoff),
							   GET_HIGH_REG(sp->regoff));
# endif
					} 
					else
#endif /* defined(SUPPORT_COMBINE_INTEGER_REGISTERS) */
						{
#if defined(ENABLE_JIT) && defined(ENABLE_DISASSEMBLER)
# if defined(ENABLE_INTRP)
							if (opt_intrp)
								printf("%3d", sp->regoff);
							else
# endif
								printf("%3s", regs[sp->regoff]);
#else
							printf("%3d", sp->regoff);
#endif
						}
				}
				break;
			case STACKVAR:
				printf("i%02d", sp->varnum);
				break;
			case LOCALVAR:
				printf("l%02d", sp->varnum);
				break;
			case ARGVAR:
				if (sp->varnum == -1) {
					/* Return Value                                  */
					/* varkind ARGVAR "misused for this special case */
					printf(" v0");
				} 
				else /* "normal" Argvar */
				printf("a%02d", sp->varnum);
				break;
			default:
				printf("?xx");
			}
		}

		putchar(')');
	}
	putchar(' ');
}

void new_show_icmd(jitdata *jd, new_instruction *iptr, bool deadcode, int stage)
{
	u2                 opcode;
	branch_target_t   *table;
	lookup_target_t   *lookup;
	constant_FMIref   *fmiref;
	stackptr          *argp;
	s4                 i;

	/* get the opcode and the condition */

	opcode    =  iptr->opc;

	printf("%s ", icmd_names[opcode]);

	if (stage < SHOW_PARSE)
		return;

	if (deadcode)
		stage = SHOW_PARSE;

	/* Print the condition for conditional instructions. */

	/* XXX print condition from flags */

	switch (opcode) {

	case ICMD_POP:
	case ICMD_CHECKNULL:
	case ICMD_CHECKNULL_POP:
		SHOW_S1(iptr);
		break;

		/* unary */
	case ICMD_ARRAYLENGTH:
	case ICMD_INEG:
	case ICMD_LNEG:
	case ICMD_FNEG:
	case ICMD_DNEG:
	case ICMD_I2L:
	case ICMD_I2F:
	case ICMD_I2D:
	case ICMD_L2I:
	case ICMD_L2F:
	case ICMD_L2D:
	case ICMD_F2I:
	case ICMD_F2L:
	case ICMD_F2D:
	case ICMD_D2I:
	case ICMD_D2L:
	case ICMD_D2F:
	case ICMD_INT2BYTE:
	case ICMD_INT2CHAR:
	case ICMD_INT2SHORT:
		SHOW_S1(iptr);
		SHOW_DST(iptr);
		break;

		/* binary */
	case ICMD_IADD:
	case ICMD_LADD:
	case ICMD_FADD:
	case ICMD_DADD:
	case ICMD_ISUB:
	case ICMD_LSUB:
	case ICMD_FSUB:
	case ICMD_DSUB:
	case ICMD_IMUL:
	case ICMD_LMUL:
	case ICMD_FMUL:
	case ICMD_DMUL:
	case ICMD_IDIV:
	case ICMD_LDIV:
	case ICMD_FDIV:
	case ICMD_DDIV:
	case ICMD_IREM:
	case ICMD_LREM:
	case ICMD_FREM:
	case ICMD_DREM:
	case ICMD_ISHL:
	case ICMD_LSHL:
	case ICMD_ISHR:
	case ICMD_LSHR:
	case ICMD_IUSHR:
	case ICMD_LUSHR:
	case ICMD_IAND:
	case ICMD_LAND:
	case ICMD_IOR:
	case ICMD_LOR:
	case ICMD_IXOR:
	case ICMD_LXOR:
	case ICMD_LCMP:
	case ICMD_FCMPL:
	case ICMD_FCMPG:
	case ICMD_DCMPL:
	case ICMD_DCMPG:
		SHOW_S1(iptr);
		SHOW_S2(iptr);
		SHOW_DST(iptr);
		break;

		/* binary/const INT */
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
		SHOW_S1(iptr);
		SHOW_INT_CONST(iptr->sx.val.i);	
		SHOW_DST(iptr);
		break;

		/* ?ASTORECONST */
	case ICMD_IASTORECONST:
	case ICMD_BASTORECONST:
	case ICMD_CASTORECONST:
	case ICMD_SASTORECONST:
		SHOW_S1(iptr);
		SHOW_S2(iptr);
		SHOW_INT_CONST(iptr->sx.s23.s3.constval);
		break;

		/* const INT */
	case ICMD_ICONST:
		SHOW_INT_CONST(iptr->sx.val.i);	
		SHOW_DST(iptr);
		break;

	case ICMD_IFEQ_ICONST:
	case ICMD_IFNE_ICONST:
	case ICMD_IFLT_ICONST:
	case ICMD_IFGE_ICONST:
	case ICMD_IFGT_ICONST:
	case ICMD_IFLE_ICONST:
		break;

	case ICMD_ELSE_ICONST:
		break;

		/* binary/const LNG */
	case ICMD_LADDCONST:
	case ICMD_LSUBCONST:
	case ICMD_LMULCONST:
	case ICMD_LMULPOW2:
	case ICMD_LDIVPOW2:
	case ICMD_LREMPOW2:
	case ICMD_LANDCONST:
	case ICMD_LORCONST:
	case ICMD_LXORCONST:
	case ICMD_LASTORECONST:
		SHOW_S1(iptr);
		SHOW_LNG_CONST(iptr->sx.val.l);	
		SHOW_DST(iptr);
		break;

		/* const LNG */
	case ICMD_LCONST:
		SHOW_LNG_CONST(iptr->sx.val.l);	
		SHOW_DST(iptr);
		break;

		/* const FLT */
	case ICMD_FCONST:
		SHOW_FLT_CONST(iptr->sx.val.f);	
		SHOW_DST(iptr);
		break;

		/* const DBL */
	case ICMD_DCONST:
		SHOW_DBL_CONST(iptr->sx.val.d);	
		SHOW_DST(iptr);
		break;

		/* const ADR */
	case ICMD_ACONST:
		if (iptr->flags.bits & INS_FLAG_CLASS) {
			SHOW_CLASSREF_OR_CLASSINFO(iptr->sx.val.c);
		}
		else if (iptr->sx.val.anyptr == NULL) {
			printf("NULL ");
		}
		else {
			SHOW_STRING(iptr->sx.val.stringconst);
		}
		SHOW_DST(iptr);
		break;

	case ICMD_AASTORECONST:
		SHOW_S1(iptr);
		SHOW_S2(iptr);
		printf("%p ", (void*) iptr->sx.s23.s3.constval);
		break;

	case ICMD_GETFIELD:        /* 1 -> 1 */
	case ICMD_PUTFIELD:        /* 2 -> 0 */
 	case ICMD_PUTSTATIC:       /* 1 -> 0 */
	case ICMD_GETSTATIC:       /* 0 -> 1 */
	case ICMD_PUTSTATICCONST:  /* 0 -> 0 */
	case ICMD_PUTFIELDCONST:   /* 1 -> 0 */
		if (opcode != ICMD_GETSTATIC && opcode != ICMD_PUTSTATICCONST) {
			SHOW_S1(iptr);
			if (opcode == ICMD_PUTFIELD) {
				SHOW_S2(iptr);
			}
		}
		if (iptr->flags.bits & INS_FLAG_UNRESOLVED)
			printf("(UNRESOLVED) ");
		NEW_INSTRUCTION_GET_FIELDREF(iptr, fmiref);
		SHOW_FIELD(fmiref);

		if (opcode == ICMD_GETSTATIC || opcode == ICMD_GETFIELD) {
			SHOW_DST(iptr);
		}
		break;

	case ICMD_IINC:
		SHOW_S1_LOCAL(iptr);
		SHOW_INT_CONST(iptr->sx.val.i);
		SHOW_DST_LOCAL(iptr);
		break;

	case ICMD_IASTORE:
	case ICMD_SASTORE:
	case ICMD_BASTORE:
	case ICMD_CASTORE:
	case ICMD_LASTORE:
	case ICMD_DASTORE:
	case ICMD_FASTORE:
	case ICMD_AASTORE:
		SHOW_S1(iptr);
		SHOW_S2(iptr);
		SHOW_S3(iptr);
		break;

	case ICMD_IALOAD:
	case ICMD_SALOAD:
	case ICMD_BALOAD:
	case ICMD_CALOAD:
	case ICMD_LALOAD:
	case ICMD_DALOAD:
	case ICMD_FALOAD:
	case ICMD_AALOAD:
		SHOW_S1(iptr);
		SHOW_S2(iptr);
		SHOW_DST(iptr);
		break;

	case ICMD_RET:
		SHOW_S1_LOCAL(iptr);
		break;

	case ICMD_ILOAD:
	case ICMD_LLOAD:
	case ICMD_FLOAD:
	case ICMD_DLOAD:
	case ICMD_ALOAD:
		SHOW_S1_LOCAL(iptr);
		SHOW_DST(iptr);
		break;

	case ICMD_ISTORE:
	case ICMD_LSTORE:
	case ICMD_FSTORE:
	case ICMD_DSTORE:
	case ICMD_ASTORE:
		SHOW_S1(iptr);
		SHOW_DST_LOCAL(iptr);
		break;

	case ICMD_NEW:
		SHOW_DST(iptr);
		break;

	case ICMD_NEWARRAY:
		SHOW_DST(iptr);
		break;

	case ICMD_ANEWARRAY:
		SHOW_DST(iptr);
		break;

	case ICMD_MULTIANEWARRAY:
		if (stage >= SHOW_STACK) {
			argp = iptr->sx.s23.s2.args;
			i = iptr->s1.argcount;
			while (i--) {
				SHOW_STACKVAR(*(argp++));
			}
		}
		else {
			printf("argcount=%d ", iptr->s1.argcount);
		}
		SHOW_DST(iptr);
		break;

	case ICMD_CHECKCAST:
		SHOW_S1(iptr);
		SHOW_DST(iptr);
		break;

	case ICMD_INSTANCEOF:
		SHOW_S1(iptr);
		SHOW_DST(iptr);
		break;

	case ICMD_INLINE_START:
	case ICMD_INLINE_END:
		break;

	case ICMD_BUILTIN:
		if (stage >= SHOW_STACK) {
			argp = iptr->sx.s23.s2.args;
			i = iptr->s1.argcount;
			while (i--) {
				SHOW_STACKVAR(*(argp++));
			}
		}
		printf("%s ", iptr->sx.s23.s3.bte->name);
		if (iptr->sx.s23.s3.bte->md->returntype.type != TYPE_VOID) {
			SHOW_DST(iptr);
		}
		break;

	case ICMD_INVOKEVIRTUAL:
	case ICMD_INVOKESPECIAL:
	case ICMD_INVOKESTATIC:
	case ICMD_INVOKEINTERFACE:
		if (stage >= SHOW_STACK) {
			argp = iptr->sx.s23.s2.args;
			i = iptr->s1.argcount;
			while (i--) {
				SHOW_STACKVAR(*(argp++));
			}
		}
		if (iptr->flags.bits & INS_FLAG_UNRESOLVED) {
			printf("(UNRESOLVED) ");
		}
		NEW_INSTRUCTION_GET_METHODREF(iptr, fmiref);
		method_methodref_print(fmiref);
		if (fmiref->parseddesc.md->returntype.type != TYPE_VOID) {
			SHOW_DST(iptr);
		}
		break;

	case ICMD_IFEQ:
	case ICMD_IFNE:
	case ICMD_IFLT:
	case ICMD_IFGE:
	case ICMD_IFGT:
	case ICMD_IFLE:
		SHOW_S1(iptr);
		SHOW_TARGET(iptr->dst);
		break;

	case ICMD_IF_LEQ:
	case ICMD_IF_LNE:
	case ICMD_IF_LLT:
	case ICMD_IF_LGE:
	case ICMD_IF_LGT:
	case ICMD_IF_LLE:
		SHOW_S1(iptr);
		SHOW_TARGET(iptr->dst);
		break;

	case ICMD_GOTO:
	case ICMD_INLINE_GOTO:
		SHOW_TARGET(iptr->dst);
		break;

	case ICMD_JSR:
		SHOW_TARGET(iptr->sx.s23.s3.jsrtarget);
		SHOW_DST(iptr);
		break;

	case ICMD_IFNULL:
	case ICMD_IFNONNULL:
		SHOW_S1(iptr);
		SHOW_TARGET(iptr->dst);
		break;

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

	case ICMD_IF_FCMPEQ:
	case ICMD_IF_FCMPNE:

	case ICMD_IF_FCMPL_LT:
	case ICMD_IF_FCMPL_GE:
	case ICMD_IF_FCMPL_GT:
	case ICMD_IF_FCMPL_LE:

	case ICMD_IF_FCMPG_LT:
	case ICMD_IF_FCMPG_GE:
	case ICMD_IF_FCMPG_GT:
	case ICMD_IF_FCMPG_LE:

	case ICMD_IF_DCMPEQ:
	case ICMD_IF_DCMPNE:

	case ICMD_IF_DCMPL_LT:
	case ICMD_IF_DCMPL_GE:
	case ICMD_IF_DCMPL_GT:
	case ICMD_IF_DCMPL_LE:

	case ICMD_IF_DCMPG_LT:
	case ICMD_IF_DCMPG_GE:
	case ICMD_IF_DCMPG_GT:
	case ICMD_IF_DCMPG_LE:

	case ICMD_IF_ACMPEQ:
	case ICMD_IF_ACMPNE:
		SHOW_S1(iptr);
		SHOW_S2(iptr);
		SHOW_TARGET(iptr->dst);
		break;

	case ICMD_TABLESWITCH:
		SHOW_S1(iptr);
		break;

	case ICMD_LOOKUPSWITCH:
		SHOW_S1(iptr);
		break;

	case ICMD_ARETURN:
		SHOW_S1(iptr);
		break;

	case ICMD_ATHROW:
		SHOW_S1(iptr);
		break;

	case ICMD_DUP:
		SHOW_S1(iptr);
		SHOW_DST(iptr);
		break;

	case ICMD_DUP2:
		if (stage >= SHOW_STACK) {
			SHOW_STACKVAR(iptr->dst.dupslots[0]);
			SHOW_STACKVAR(iptr->dst.dupslots[1]);
			printf("=> ");
			SHOW_STACKVAR(iptr->dst.dupslots[2+0]);
			SHOW_STACKVAR(iptr->dst.dupslots[2+1]);
		}
		break;

	case ICMD_DUP_X1:
		if (stage >= SHOW_STACK) {
			SHOW_STACKVAR(iptr->dst.dupslots[0]);
			SHOW_STACKVAR(iptr->dst.dupslots[1]);
			printf("=> ");
			SHOW_STACKVAR(iptr->dst.dupslots[2+0]);
			SHOW_STACKVAR(iptr->dst.dupslots[2+1]);
			SHOW_STACKVAR(iptr->dst.dupslots[2+2]);
		}
		break;

	case ICMD_DUP2_X1:
		if (stage >= SHOW_STACK) {
			SHOW_STACKVAR(iptr->dst.dupslots[0]);
			SHOW_STACKVAR(iptr->dst.dupslots[1]);
			SHOW_STACKVAR(iptr->dst.dupslots[2]);
			printf("=> ");
			SHOW_STACKVAR(iptr->dst.dupslots[3+0]);
			SHOW_STACKVAR(iptr->dst.dupslots[3+1]);
			SHOW_STACKVAR(iptr->dst.dupslots[3+2]);
			SHOW_STACKVAR(iptr->dst.dupslots[3+3]);
			SHOW_STACKVAR(iptr->dst.dupslots[3+4]);
		}
		break;

	case ICMD_DUP_X2:
		if (stage >= SHOW_STACK) {
			SHOW_STACKVAR(iptr->dst.dupslots[0]);
			SHOW_STACKVAR(iptr->dst.dupslots[1]);
			SHOW_STACKVAR(iptr->dst.dupslots[2]);
			printf("=> ");
			SHOW_STACKVAR(iptr->dst.dupslots[3+0]);
			SHOW_STACKVAR(iptr->dst.dupslots[3+1]);
			SHOW_STACKVAR(iptr->dst.dupslots[3+2]);
			SHOW_STACKVAR(iptr->dst.dupslots[3+3]);
		}
		break;

	case ICMD_DUP2_X2:
		if (stage >= SHOW_STACK) {
			SHOW_STACKVAR(iptr->dst.dupslots[0]);
			SHOW_STACKVAR(iptr->dst.dupslots[1]);
			SHOW_STACKVAR(iptr->dst.dupslots[2]);
			SHOW_STACKVAR(iptr->dst.dupslots[4]);
			printf("=> ");
			SHOW_STACKVAR(iptr->dst.dupslots[4+0]);
			SHOW_STACKVAR(iptr->dst.dupslots[4+1]);
			SHOW_STACKVAR(iptr->dst.dupslots[4+2]);
			SHOW_STACKVAR(iptr->dst.dupslots[4+3]);
			SHOW_STACKVAR(iptr->dst.dupslots[4+4]);
			SHOW_STACKVAR(iptr->dst.dupslots[4+5]);
		}
		break;

	case ICMD_SWAP:
		if (stage >= SHOW_STACK) {
			SHOW_STACKVAR(iptr->dst.dupslots[0]);
			SHOW_STACKVAR(iptr->dst.dupslots[1]);
			printf("=> ");
			SHOW_STACKVAR(iptr->dst.dupslots[2+0]);
			SHOW_STACKVAR(iptr->dst.dupslots[2+1]);
		}
		break;

	}
}
#endif /* !defined(NDEBUG) */

#if !defined(NDEBUG)
void show_icmd(instruction *iptr, bool deadcode)
{
	u2                 opcode;
	u2                 condition;
	int j;
	s4  *s4ptr;
	void **tptr = NULL;
	classinfo         *c;
	fieldinfo         *f;
	constant_classref *cr;
	unresolved_field  *uf;

	/* get the opcode and the condition */

	opcode    =  iptr->opc & ICMD_OPCODE_MASK;
	condition = (iptr->opc & ICMD_CONDITION_MASK) >> 8;

	printf("%s", icmd_names[opcode]);

	/* Print the condition for conditional instructions. */

	if (condition != 0)
		printf(" (condition: %s)", icmd_names[condition]);

	switch (opcode) {
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
		printf(" %d, %d (0x%08x)", iptr[1].op1, iptr->val.i, iptr->val.i);
		break;

	case ICMD_ELSE_ICONST:
		printf("    %d (0x%08x)", iptr->val.i, iptr->val.i);
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
		printf(" %f (0x%08x)", iptr->val.f, iptr->val.i);
		break;

	case ICMD_DCONST:
#if SIZEOF_VOID_P == 4
		printf(" %g (0x%016llx)", iptr->val.d, iptr->val.l);
#else
		printf(" %g (0x%016lx)", iptr->val.d, iptr->val.l);
#endif
		break;

	case ICMD_ACONST:
	case ICMD_AASTORECONST:
		/* check if this is a constant string or a class reference */

		if (ICMD_ACONST_IS_CLASS(iptr)) {
			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				printf(" (NOT RESOLVED) classref = ");
				class_classref_print(ICMD_ACONST_UNRESOLVED_CLASSREF(iptr));
			}
			else {
				printf(" class = ");
				class_print(ICMD_ACONST_RESOLVED_CLASSINFO(iptr));
			}
		}
		else {
			printf(" %p", iptr->val.a);

			if (iptr->val.a) {
				printf(", String = \"");
				utf_display_printable_ascii(javastring_toutf(iptr->val.a, false));
				printf("\"");
			}
		}
		break;

	case ICMD_GETFIELD:
	case ICMD_PUTFIELD:
		if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
			uf = INSTRUCTION_UNRESOLVED_FIELD(iptr);
			printf(" (NOT RESOLVED) ");

			field_fieldref_print(uf->fieldref);
		}
		else {
			f = INSTRUCTION_RESOLVED_FIELDINFO(iptr);
			printf(" %d, ", f->offset);

			field_print(f);
		}
		break;

 	case ICMD_PUTSTATIC:
	case ICMD_GETSTATIC:
		if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
			uf = INSTRUCTION_UNRESOLVED_FIELD(iptr);
			printf(" (NOT RESOLVED) ");

			field_fieldref_print(uf->fieldref);
		}
		else {
			f = INSTRUCTION_RESOLVED_FIELDINFO(iptr);
			if (!CLASS_IS_OR_ALMOST_INITIALIZED(f->class))
				printf(" (NOT INITIALIZED) ");
			else
				printf(" ");

			field_print(f);
		}
		break;

	case ICMD_PUTSTATICCONST:
	case ICMD_PUTFIELDCONST:
		switch (iptr[1].op1) {
		case TYPE_INT:
			printf(" %d (0x%08x),", iptr->val.i, iptr->val.i);
			break;
		case TYPE_LNG:
#if SIZEOF_VOID_P == 4
			printf(" %lld (0x%016llx),", iptr->val.l, iptr->val.l);
#else
			printf(" %ld (0x%016lx),", iptr->val.l, iptr->val.l);
#endif
			break;
		case TYPE_ADR:
			printf(" %p,", iptr->val.a);
			break;
		case TYPE_FLT:
			printf(" %g (0x%08x),", iptr->val.f, iptr->val.i);
			break;
		case TYPE_DBL:
#if SIZEOF_VOID_P == 4
			printf(" %g (0x%016llx),", iptr->val.d, iptr->val.l);
#else
			printf(" %g (0x%016lx),", iptr->val.d, iptr->val.l);
#endif
			break;
		}

		if (INSTRUCTION_IS_UNRESOLVED(iptr + 1)) {
			uf = INSTRUCTION_UNRESOLVED_FIELD(iptr + 1);
			printf(" (NOT RESOLVED) ");
			field_fieldref_print(uf->fieldref);
		}
		else {
			f = INSTRUCTION_RESOLVED_FIELDINFO(iptr + 1);
			if ((iptr->opc == ICMD_PUTSTATICCONST) &&
				!CLASS_IS_OR_ALMOST_INITIALIZED(f->class))
				printf(" (NOT INITIALIZED), ");
			else
				printf(" %d, ", f->offset);
			field_print(f);
		}
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
		c = iptr->val.a;
		printf(" ");
		utf_display_printable_ascii_classname(c->name);
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
			c = iptr->val.a;
			printf(" ");
			utf_display_printable_ascii_classname(c->name);
		}
		break;

	case ICMD_MULTIANEWARRAY:
		c  = iptr->val.a;
		cr = iptr->target;

		if (c == NULL) {
			printf(" (NOT RESOLVED) %d ", iptr->op1);
			utf_display_printable_ascii(cr->name);
		} 
		else {
			printf(" %d ", iptr->op1);
			utf_display_printable_ascii_classname(c->name);
		}
		break;

	case ICMD_CHECKCAST:
	case ICMD_INSTANCEOF:
		c  = iptr->val.a;
		cr = iptr->target;

		if (c) {
			if (c->flags & ACC_INTERFACE)
				printf(" (INTERFACE) ");
			else
				printf(" (CLASS,%3d) ", c->vftbl->diffval);
		} else
			printf(" (NOT RESOLVED) ");
		utf_display_printable_ascii_classname(cr->name);
		break;

	case ICMD_INLINE_START:
	case ICMD_INLINE_END:
		{
			insinfo_inline *insinfo = (insinfo_inline *) iptr->target;
			printf(" ");
			method_print(insinfo->method);
		}
		break;

	case ICMD_BUILTIN:
		printf(" %s", ((builtintable_entry *) iptr->val.a)->name);
		break;

	case ICMD_INVOKEVIRTUAL:
	case ICMD_INVOKESPECIAL:
	case ICMD_INVOKESTATIC:
	case ICMD_INVOKEINTERFACE:
		{
			constant_FMIref *mref;
			
			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				printf(" (NOT RESOLVED) ");
				mref = INSTRUCTION_UNRESOLVED_METHOD(iptr)->methodref;
			}
			else {
				printf(" ");
				mref = INSTRUCTION_RESOLVED_FMIREF(iptr);
			}
			method_methodref_print(mref);
		}
		break;

	case ICMD_IFEQ:
	case ICMD_IFNE:
	case ICMD_IFLT:
	case ICMD_IFGE:
	case ICMD_IFGT:
	case ICMD_IFLE:
		printf(" %d (0x%08x)", iptr->val.i, iptr->val.i);

		if ((iptr->opc & ICMD_CONDITION_MASK) == 0) {
			if (deadcode || !iptr->target)
				printf(" op1=%d", iptr->op1);
			else
				printf(" L%03d (%p)", ((basicblock *) iptr->target)->debug_nr, iptr->target);
		}
		break;

	case ICMD_IF_LEQ:
	case ICMD_IF_LNE:
	case ICMD_IF_LLT:
	case ICMD_IF_LGE:
	case ICMD_IF_LGT:
	case ICMD_IF_LLE:
#if SIZEOF_VOID_P == 4
		printf(" %lld (%016llx)", iptr->val.l, iptr->val.l);
#else
		printf(" %ld (%016lx)", iptr->val.l, iptr->val.l);
#endif

		if ((iptr->opc & ICMD_CONDITION_MASK) == 0) {
			if (deadcode || !iptr->target)
				printf(" op1=%d", iptr->op1);
			else
				printf(" L%03d", ((basicblock *) iptr->target)->debug_nr);
		}
		break;

	case ICMD_JSR:
	case ICMD_GOTO:
	case ICMD_INLINE_GOTO:
		if (deadcode || !iptr->target)
			printf(" op1=%d", iptr->op1);
		else
			printf(" L%03d (%p)", ((basicblock *) iptr->target)->debug_nr,iptr->target);
		break;

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

	case ICMD_IF_FCMPEQ:
	case ICMD_IF_FCMPNE:

	case ICMD_IF_FCMPL_LT:
	case ICMD_IF_FCMPL_GE:
	case ICMD_IF_FCMPL_GT:
	case ICMD_IF_FCMPL_LE:

	case ICMD_IF_FCMPG_LT:
	case ICMD_IF_FCMPG_GE:
	case ICMD_IF_FCMPG_GT:
	case ICMD_IF_FCMPG_LE:

	case ICMD_IF_DCMPEQ:
	case ICMD_IF_DCMPNE:

	case ICMD_IF_DCMPL_LT:
	case ICMD_IF_DCMPL_GE:
	case ICMD_IF_DCMPL_GT:
	case ICMD_IF_DCMPL_LE:

	case ICMD_IF_DCMPG_LT:
	case ICMD_IF_DCMPG_GE:
	case ICMD_IF_DCMPG_GT:
	case ICMD_IF_DCMPG_LE:

	case ICMD_IF_ACMPEQ:
	case ICMD_IF_ACMPNE:
		if (!(iptr->opc & ICMD_CONDITION_MASK)) {
			if (deadcode || !iptr->target)
				printf(" op1=%d", iptr->op1);
			else
				printf(" L%03d (%p)", ((basicblock *) iptr->target)->debug_nr,iptr->target);
		}
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

	case ICMD_ARETURN:
		if (iptr->val.a) {
			printf(" (NOT RESOLVED) Class = \"");
			utf_display_printable_ascii(((unresolved_class *) iptr->val.a)->classref->name);
			printf("\"");
		}
	}
}
#endif /* !defined(NDEBUG) */

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
