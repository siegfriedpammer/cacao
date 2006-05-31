/* vm/jit/show.c - showing the intermediate representation

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

	if (opt_showdisassemble) {
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

	if (opt_showdisassemble && opt_showexceptionstubs) {
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
		if (opt_showdisassemble && (!deadcode)) {
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
