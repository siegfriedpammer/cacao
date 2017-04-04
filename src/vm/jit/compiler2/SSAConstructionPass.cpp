/* src/vm/jit/compiler2/SSAConstructionPass.cpp - SSAConstructionPass

   Copyright (C) 2013
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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

#include "vm/jit/compiler2/SSAConstructionPass.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/Type.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/CFGConstructionPass.hpp"

#include "vm/descriptor.hpp"
#include "vm/field.hpp"

#include "vm/jit/jit.hpp"
#include "vm/jit/show.hpp"
#include "vm/jit/ir/icmd.hpp"
#include "vm/jit/ir/instruction.hpp"

#include "mm/dumpmemory.hpp"

#include "vm/statistics.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/SSAConstruction"

STAT_DECLARE_GROUP(compiler2_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,num_trivial_phis,0,"# trivial phis","number of trivial phis",compiler2_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,num_icmd_inst,0,"icmd instructions",
	"ICMD instructions processed (by the compiler2)",compiler2_stat)


namespace cacao {

namespace {
#if 0
OStream& show_variable_intern_OS(OStream &OS, const jitdata *jd, s4 index, int stage)
{
	char type;
	char kind;
	varinfo *v;

	if (index < 0 || index >= jd->vartop) {
		OS << "<INVALID INDEX:" << index << ">";
		return OS;
	}

	v = VAR(index);

	switch (v->type) {
		case TYPE_INT: type = 'i'; break;
		case TYPE_LNG: type = 'l'; break;
		case TYPE_FLT: type = 'f'; break;
		case TYPE_DBL: type = 'd'; break;
		case TYPE_ADR: type = 'a'; break;
		case TYPE_RET: type = 'r'; break;
		default:       type = '?';
	}

	if (index < jd->localcount) {
		kind = 'L';
		if (v->flags & (PREALLOC | INOUT))
				OS << "<INVALID FLAGS!>";
	}
	else {
		if (v->flags & PREALLOC) {
			kind = 'A';
			if (v->flags & INOUT) {
				/* PREALLOC is used to avoid allocation of TYPE_RET */
				if (v->type == TYPE_RET)
					kind = 'i';
				else
					OS << "<INVALID FLAGS!>";
			}
		}
		else if (v->flags & INOUT)
			kind = 'I';
		else
			kind = 'T';
	}

	printf("%c%c%d", kind, type, index);
	OS << kind << type << index;

	if (v->flags & SAVEDVAR)
		OS << '!';

	if (stage >= SHOW_REGS || (v->flags & PREALLOC)) {
		OS << '(';
		OS << "TODO: show_allocation(v->type, v->flags, v->vv.regoff);";
		OS << ')';
	}

	if (v->type == TYPE_RET && (v->flags & PREALLOC)) {
		printf("(L%03d)", v->vv.retaddr->nr);
		OS << "(L" << setw(3) << v->vv.retaddr->nr;
	}

	return OS;
}
#endif

#define SHOW_TARGET(OS, target)                                      \
        if (stage >= SHOW_PARSE) {                                   \
            OS << "--> L" << setw(3) << (target).block->nr;          \
        }                                                            \
        else {                                                       \
            OS << "--> insindex " << (target).insindex;              \
        }

#define SHOW_INT_CONST(OS, val)                                          \
        if (stage >= SHOW_PARSE) {                                   \
            OS << (int32_t) (val) << "0x" << hex << setw(8) << (int32_t) (val) << dec; \
        }                                                            \
        else {                                                       \
            OS << "iconst ";                                         \
        }

#define SHOW_LNG_CONST(OS, val)                                      \
        if (stage >= SHOW_PARSE)                                     \
            OS << (val) << "0x" << hex << setw(16) << (val) << dec;\
        else                                                         \
            OS << "lconst ";

#define SHOW_ADR_CONST(OS, val)                                      \
        if (stage >= SHOW_PARSE)                                     \
            OS << "0x" << hex << setw(8) << (ptrint) (val) << dec; \
        else                                                         \
            OS << "aconst ";

#define SHOW_FLT_CONST(OS, val)                                      \
        if (stage >= SHOW_PARSE) {                                   \
            imm_union v;                                             \
            v.f = (val);                                             \
            OS << (val) << hex << setw(8) << v.i << dec;           \
        }                                                            \
        else {                                                       \
            OS << "fconst ";                                         \
        }

#define SHOW_DBL_CONST(OS, val)                                      \
        if (stage >= SHOW_PARSE) {                                   \
            imm_union v;                                             \
            v.d = (val);                                             \
            OS << (val) << hex << setw(16) << v.l << dec;          \
        }                                                            \
        else                                                         \
            OS << "dconst ";

#define SHOW_INDEX(OS, index)                                        \
        if (stage >= SHOW_PARSE) {                                   \
            OS << index << "";                                       \
        }                                                            \
        else {                                                       \
            OS << "index";                                           \
        }

#define SHOW_STRING(OS, val)                                         \
        if (stage >= SHOW_PARSE) {                                   \
            OS << "\"";                                              \
            OS << (Utf8String)JavaString((java_handle_t*) (val)).to_utf8();\
            OS << "\" ";                                             \
        }                                                            \
        else {                                                       \
            OS << "string ";                                         \
        }
#if 0
#define SHOW_CLASSREF_OR_CLASSINFO(OS, c)                            \
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
#else
#define SHOW_CLASSREF_OR_CLASSINFO(OS, c)                            \
        if (stage >= SHOW_PARSE) {                                   \
            OS << "class (TODO)";                                    \
        }                                                            \
        else {                                                       \
            OS << "class ";                                          \
        }
#endif
#if 0
#define SHOW_FIELD(OS, fmiref)                                           \
        if (stage >= SHOW_PARSE) {                                   \
            field_fieldref_print(fmiref);                            \
            putchar(' ');                                            \
        }                                                            \
        else {                                                       \
            printf("field ");                                        \
        }
#else
#define SHOW_FIELD(OS, fmiref)                                           \
        if (stage >= SHOW_PARSE) {                                   \
            OS << "field (TODO)";                                    \
        }                                                            \
        else {                                                       \
            OS << "field ";                                          \
        }
#endif

#define SHOW_VARIABLE(OS, v)                                             \
    show_variable_intern_OS(OS, jd, (v), stage)

#define SHOW_S1(OS, iptr)                                                \
        if (stage >= SHOW_STACK) {                                   \
            SHOW_VARIABLE(OS, iptr->s1.varindex);                        \
        }

#define SHOW_S2(OS, iptr)                                                \
        if (stage >= SHOW_STACK) {                                   \
            SHOW_VARIABLE(OS, iptr->sx.s23.s2.varindex);                 \
        }

#define SHOW_S3(OS, iptr)                                                \
    if (stage >= SHOW_STACK) {                                       \
        SHOW_VARIABLE(OS, iptr->sx.s23.s3.varindex);                     \
    }

#define SHOW_DST(OS, iptr)                                           \
    if (stage >= SHOW_STACK) {                                       \
        OS << "=> ";                                                 \
        SHOW_VARIABLE(OS, iptr->dst.varindex);                       \
    }

#define SHOW_S1_LOCAL(OS, iptr)                                      \
    if (stage >= SHOW_STACK) {                                       \
        OS << "L" << iptr->s1.varindex;                              \
    }                                                                \
    else {                                                           \
        OS << "JavaL" << iptr->s1.varindex << " ";                   \
    }

#define SHOW_DST_LOCAL(OS, iptr)                                     \
    if (stage >= SHOW_STACK) {                                       \
        OS << "=> L" << iptr->dst.varindex << " ";                   \
    }                                                                \
    else {                                                           \
        OS << "=> JavaL" << iptr->dst.varindex << " ";               \
    }
#if 0
//cacao::OStream& operator<<(cacao::OStream &OS, const struct instruction &inst) {
cacao::OStream& print_instruction_OS(cacao::OStream &OS, const jitdata *jd, const struct instruction &inst) {
	const instruction *iptr = &inst;
	int stage = SHOW_CFG;
	u2                 opcode;
	branch_target_t   *table;
	lookup_target_t   *lookup;
	constant_FMIref   *fmiref;
	s4                *argp;
	s4                 i;

	/* get the opcode and the condition */

	opcode    =  iptr->opc;

	if (stage < SHOW_PARSE)
		return OS;

	/* Print the condition for conditional instructions. */

	/* XXX print condition from flags */

	if (iptr->flags.bits & INS_FLAG_UNRESOLVED)
		OS << "(UNRESOLVED) ";

	switch (inst.opc) {
	case ICMD_POP:
	case ICMD_CHECKNULL:
		SHOW_S1(OS, iptr);
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
		SHOW_S1(OS, iptr);
		SHOW_DST(OS, iptr);
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
		SHOW_S1(OS, iptr);
		SHOW_S2(OS, iptr);
		SHOW_DST(OS, iptr);
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
		SHOW_S1(OS, iptr);
		SHOW_INT_CONST(OS, iptr->sx.val.i);	
		SHOW_DST(OS, iptr);
		break;

		/* ?ASTORECONST (trinary/const INT) */
	case ICMD_IASTORECONST:
	case ICMD_BASTORECONST:
	case ICMD_CASTORECONST:
	case ICMD_SASTORECONST:
		SHOW_S1(OS, iptr);
		SHOW_S2(OS, iptr);
		SHOW_INT_CONST(OS, iptr->sx.s23.s3.constval);
		break;

		/* const INT */
	case ICMD_ICONST:
		SHOW_INT_CONST(OS, iptr->sx.val.i);	
		SHOW_DST(OS, iptr);
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
		SHOW_S1(OS, iptr);
		SHOW_LNG_CONST(OS, iptr->sx.val.l);
		SHOW_DST(OS, iptr);
		break;

		/* trinary/const LNG (<= pointer size) */
	case ICMD_LASTORECONST:
		SHOW_S1(OS, iptr);
		SHOW_S2(OS, iptr);
		SHOW_ADR_CONST(OS, iptr->sx.s23.s3.constval);
		break;

		/* const LNG */
	case ICMD_LCONST:
		SHOW_LNG_CONST(OS, iptr->sx.val.l);	
		SHOW_DST(OS, iptr);
		break;

		/* const FLT */
	case ICMD_FCONST:
		SHOW_FLT_CONST(OS, iptr->sx.val.f);	
		SHOW_DST(OS, iptr);
		break;

		/* const DBL */
	case ICMD_DCONST:
		SHOW_DBL_CONST(OS, iptr->sx.val.d);	
		SHOW_DST(OS, iptr);
		break;

		/* const ADR */
	case ICMD_ACONST:
		if (iptr->flags.bits & INS_FLAG_CLASS) {
			SHOW_ADR_CONST(OS, iptr->sx.val.anyptr);
			SHOW_CLASSREF_OR_CLASSINFO(OS, iptr->sx.val.c);
		}
		else if (iptr->sx.val.anyptr == NULL) {
			OS << "NULL ";
		}
		else {
			SHOW_ADR_CONST(OS, iptr->sx.val.anyptr);
			SHOW_STRING(OS, iptr->sx.val.stringconst);
		}
		SHOW_DST(OS, iptr);
		break;

	case ICMD_AASTORECONST:
		SHOW_S1(OS, iptr);
		SHOW_S2(OS, iptr);
		printf("%p ", (void*) iptr->sx.s23.s3.constval);
		break;

	case ICMD_GETFIELD:        /* 1 -> 1 */
	case ICMD_PUTFIELD:        /* 2 -> 0 */
 	case ICMD_PUTSTATIC:       /* 1 -> 0 */
	case ICMD_GETSTATIC:       /* 0 -> 1 */
	case ICMD_PUTSTATICCONST:  /* 0 -> 0 */
	case ICMD_PUTFIELDCONST:   /* 1 -> 0 */
		if (opcode != ICMD_GETSTATIC && opcode != ICMD_PUTSTATICCONST) {
			SHOW_S1(OS, iptr);
			if (opcode == ICMD_PUTFIELD) {
				SHOW_S2(OS, iptr);
			}
		}
		INSTRUCTION_GET_FIELDREF(iptr, fmiref);
		SHOW_FIELD(OS, fmiref);

		if (opcode == ICMD_GETSTATIC || opcode == ICMD_GETFIELD) {
			SHOW_DST(OS, iptr);
		}
		break;

	case ICMD_IINC:
		SHOW_S1_LOCAL(OS, iptr);
		SHOW_INT_CONST(OS, iptr->sx.val.i);
		SHOW_DST_LOCAL(OS, iptr);
		break;

	case ICMD_IASTORE:
	case ICMD_SASTORE:
	case ICMD_BASTORE:
	case ICMD_CASTORE:
	case ICMD_LASTORE:
	case ICMD_DASTORE:
	case ICMD_FASTORE:
	case ICMD_AASTORE:
		SHOW_S1(OS, iptr);
		SHOW_S2(OS, iptr);
		SHOW_S3(OS, iptr);
		break;

	case ICMD_IALOAD:
	case ICMD_SALOAD:
	case ICMD_BALOAD:
	case ICMD_CALOAD:
	case ICMD_LALOAD:
	case ICMD_DALOAD:
	case ICMD_FALOAD:
	case ICMD_AALOAD:
		SHOW_S1(OS, iptr);
		SHOW_S2(OS, iptr);
		SHOW_DST(OS, iptr);
		break;

	case ICMD_RET:
		SHOW_S1_LOCAL(OS, iptr);
		if (stage >= SHOW_STACK) {
			printf(" ---> L%03d", iptr->dst.block->nr);
		}
		break;

	case ICMD_ILOAD:
	case ICMD_LLOAD:
	case ICMD_FLOAD:
	case ICMD_DLOAD:
	case ICMD_ALOAD:
		SHOW_S1_LOCAL(OS, iptr);
		SHOW_DST(OS, iptr);
		break;

	case ICMD_ISTORE:
	case ICMD_LSTORE:
	case ICMD_FSTORE:
	case ICMD_DSTORE:
	case ICMD_ASTORE:
		SHOW_S1(OS, iptr);
		SHOW_DST_LOCAL(OS, iptr);
		if (stage >= SHOW_STACK && iptr->sx.s23.s3.javaindex != jitdata::UNUSED)
			printf(" (javaindex %d)", iptr->sx.s23.s3.javaindex);
		if (iptr->flags.bits & INS_FLAG_RETADDR) {
			printf(" (retaddr L%03d)", RETADDR_FROM_JAVALOCAL(iptr->sx.s23.s2.retaddrnr));
		}
		break;

	case ICMD_NEW:
		SHOW_DST(OS, iptr);
		break;

	case ICMD_NEWARRAY:
		SHOW_DST(OS, iptr);
		break;

	case ICMD_ANEWARRAY:
		SHOW_DST(OS, iptr);
		break;

	case ICMD_MULTIANEWARRAY:
		if (stage >= SHOW_STACK) {
			argp = iptr->sx.s23.s2.args;
			i = iptr->s1.argcount;
			while (i--) {
				SHOW_VARIABLE(OS, *(argp++));
			}
		}
		else {
			printf("argcount=%d ", iptr->s1.argcount);
		}
		class_classref_or_classinfo_print(iptr->sx.s23.s3.c);
		putchar(' ');
		SHOW_DST(OS, iptr);
		break;

	case ICMD_CHECKCAST:
		SHOW_S1(OS, iptr);
		class_classref_or_classinfo_print(iptr->sx.s23.s3.c);
		putchar(' ');
		SHOW_DST(OS, iptr);
		break;

	case ICMD_INSTANCEOF:
		SHOW_S1(OS, iptr);
		SHOW_DST(OS, iptr);
		break;

	case ICMD_INLINE_START:
	case ICMD_INLINE_END:
	case ICMD_INLINE_BODY:
#if 0
#if defined(ENABLE_INLINING)
		{
			insinfo_inline *ii = iptr->sx.s23.s3.inlineinfo;
			show_inline_info(jd, ii, opcode, stage);
		}
#endif
#endif
		break;

	case ICMD_BUILTIN:
		if (stage >= SHOW_STACK) {
			argp = iptr->sx.s23.s2.args;
			i = iptr->s1.argcount;
			while (i--) {
				if ((iptr->s1.argcount - 1 - i) == iptr->sx.s23.s3.bte->md->paramcount)
					printf(" pass-through: ");
				SHOW_VARIABLE(OS, *(argp++));
			}
		}
		printf("%s ", iptr->sx.s23.s3.bte->cname);
		if (iptr->sx.s23.s3.bte->md->returntype.type != TYPE_VOID) {
			SHOW_DST(OS, iptr);
		}
		break;

	case ICMD_INVOKEVIRTUAL:
	case ICMD_INVOKESPECIAL:
	case ICMD_INVOKESTATIC:
	case ICMD_INVOKEINTERFACE:
		if (stage >= SHOW_STACK) {
			methoddesc *md;
			INSTRUCTION_GET_METHODDESC(iptr, md);
			argp = iptr->sx.s23.s2.args;
			i = iptr->s1.argcount;
			while (i--) {
				if ((iptr->s1.argcount - 1 - i) == md->paramcount)
					printf(" pass-through: ");
				SHOW_VARIABLE(OS, *(argp++));
			}
		}
		INSTRUCTION_GET_METHODREF(iptr, fmiref);
		method_methodref_print(fmiref);
		if (fmiref->parseddesc.md->returntype.type != TYPE_VOID) {
			putchar(' ');
			SHOW_DST(OS, iptr);
		}
		break;

	case ICMD_IFEQ:
	case ICMD_IFNE:
	case ICMD_IFLT:
	case ICMD_IFGE:
	case ICMD_IFGT:
	case ICMD_IFLE:
		SHOW_S1(OS, iptr);
		SHOW_INT_CONST(OS, iptr->sx.val.i);	
		SHOW_TARGET(OS, iptr->dst);
		break;

	case ICMD_IF_LEQ:
	case ICMD_IF_LNE:
	case ICMD_IF_LLT:
	case ICMD_IF_LGE:
	case ICMD_IF_LGT:
	case ICMD_IF_LLE:
		SHOW_S1(OS, iptr);
		SHOW_LNG_CONST(OS, iptr->sx.val.l);	
		SHOW_TARGET(OS, iptr->dst);
		break;

	case ICMD_GOTO:
		SHOW_TARGET(OS, iptr->dst);
		break;

	case ICMD_JSR:
		SHOW_TARGET(OS, iptr->sx.s23.s3.jsrtarget);
		SHOW_DST(OS, iptr);
		break;

	case ICMD_IFNULL:
	case ICMD_IFNONNULL:
		SHOW_S1(OS, iptr);
		SHOW_TARGET(OS, iptr->dst);
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

	case ICMD_IF_ACMPEQ:
	case ICMD_IF_ACMPNE:
		SHOW_S1(OS, iptr);
		SHOW_S2(OS, iptr);
		SHOW_TARGET(OS, iptr->dst);
		break;

	case ICMD_TABLESWITCH:
		SHOW_S1(OS, iptr);
		table = iptr->dst.table;

		i = iptr->sx.s23.s3.tablehigh - iptr->sx.s23.s2.tablelow + 1;

		printf("high=%d low=%d count=%d\n", iptr->sx.s23.s3.tablehigh, iptr->sx.s23.s2.tablelow, i);
		while (--i >= 0) {
			printf("\t\t%d --> ", (int) (table - iptr->dst.table));
			printf("L%03d\n", table->block->nr);
			table++;
		}

		break;

	case ICMD_LOOKUPSWITCH:
		SHOW_S1(OS, iptr);

		printf("count=%d, default=L%03d\n",
			   iptr->sx.s23.s2.lookupcount,
			   iptr->sx.s23.s3.lookupdefault.block->nr);

		lookup = iptr->dst.lookup;
		i = iptr->sx.s23.s2.lookupcount;

		while (--i >= 0) {
			printf("\t\t%d --> L%03d\n",
				   lookup->value,
				   lookup->target.block->nr);
			lookup++;
		}
		break;

	case ICMD_FRETURN:
	case ICMD_IRETURN:
	case ICMD_DRETURN:
	case ICMD_LRETURN:
		SHOW_S1(OS, iptr);
		break;

	case ICMD_ARETURN:
	case ICMD_ATHROW:
		SHOW_S1(OS, iptr);
		if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
			/* XXX this needs more work */
#if 0
			unresolved_class_debug_dump(iptr->sx.s23.s2.uc, stdout);
#endif
		}
		break;

 	case ICMD_COPY:
 	case ICMD_MOVE:
		SHOW_S1(OS, iptr);
		SHOW_DST(OS, iptr);
		break;
	case ICMD_GETEXCEPTION:
		SHOW_DST(OS, iptr);
		break;
#if defined(ENABLE_SSA)
	case ICMD_PHI:
		printf("[ ");
		for (i = 0; i < iptr->s1.argcount; ++i) {
			SHOW_VARIABLE(OS, iptr->sx.s23.s2.iargs[i]->dst.varindex);
		}
		printf("] ");
		SHOW_DST(OS, iptr);
		if (iptr->flags.bits & (1 << 0)) OS << "used ";
		if (iptr->flags.bits & (1 << 1)) OS << "redundantAll ";
		if (iptr->flags.bits & (1 << 2)) OS << "redundantOne ";
		break;
#endif
	}
	return OS << icmd_table[inst.opc].name;
}
#endif

} // end anonymous namespace

namespace jit {
namespace compiler2 {

class InVarPhis {
private:
	PHIInst *phi;
	jitdata *jd;
	std::size_t bb_num;
	std::size_t in_var_index;
	SSAConstructionPass *parent;
public:
	// constructs
	InVarPhis(PHIInst *phi, jitdata *jd, std::size_t bb_num, std::size_t in_var_index,
		SSAConstructionPass *parent)
		: phi(phi), jd(jd), bb_num(bb_num), in_var_index(in_var_index), parent(parent) {}

	//  fill phi operands
	void fill_operands() {
		basicblock *bb = &jd->basicblocks[bb_num];
		for(s4 i = 0; i < bb->predecessorcount; ++i) {
			basicblock *pred = bb->predecessors[i];
			assert(pred->outdepth == bb->indepth);
			s4 pred_nr = pred->nr;
			s4 var = pred->outvars[in_var_index];
			// append operand
			phi->append_op(parent->read_variable(var,pred_nr));
		}
	}
};

void SSAConstructionPass::write_variable(size_t varindex, size_t bb, Value* v) {
	LOG2("write variable(" << varindex << "," << bb << ") = " << v << nl);
	current_def[varindex][bb] = v;
}

Value* SSAConstructionPass::read_variable(size_t varindex, size_t bb) {
	LOG2("read variable(" << varindex << "," << bb << ")" << nl);
	Value* v = current_def[varindex][bb];
	#if 0
	if (v) {
		// local value numbering
		return v;
	}
	// global value numbering
	return read_variable_recursive(varindex, bb);
	#endif
	if (v) {
		// local value numbering
		//v = v;
	} else {
		// global value numbering
		v = read_variable_recursive(varindex, bb);
	}
	LOG2("Value: " << v << nl);
	return v;
}

Value* SSAConstructionPass::read_variable_recursive(size_t varindex, size_t bb) {
	Value *val; // current definition
	if(BB[bb]->pred_size() == 0) {
		// variable not defined oO
		// create constant for now but seriously, this should not happen!
		#if 0
		varinfo *v = VAR(varindex);
		Type::TypeID type = convert_to_typeid(v->type);
		Instruction *konst;
		switch (type) {
		case Type::IntTypeID:
			konst = new CONSTInst(0,Type::IntType());
			break;
		case Type::LongTypeID:
			konst = new CONSTInst(0,Type::LongType());
			break;
		case Type::FloatTypeID:
			konst = new CONSTInst(0,Type::FloatType());
			break;
		case Type::DoubleTypeID:
			konst = new CONSTInst(0,Type::DoubleType());
			break;
		default: assert(0);
			konst = NULL;
		}
		ERROR_MSG("no predecessor ","basic block " << bb << " var index " << varindex
			<< " constant: " << konst );
		M->add_Instruction(konst);
		return konst;
		#endif
		ABORT_MSG("no predecessor ","basic block " << bb << " var index " << varindex);
		return NULL;
	}
	if (!sealed_blocks[bb]) {
		PHIInst *phi = new PHIInst(var_type_tbl[varindex], BB[bb]);
		incomplete_phi[bb][varindex] = phi;
		M->add_Instruction(phi);
		val = phi;
	} else if (BB[bb]->pred_size() == 1) { // one predecessor
		// get first (and only predecessor
		BeginInst *pred = *(BB[bb]->pred_begin());
		val = read_variable(varindex, beginToIndex[pred]);
	} else {
		PHIInst *phi = new PHIInst(var_type_tbl[varindex], BB[bb]);
		write_variable(varindex, bb, phi);
		M->add_Instruction(phi);
		val = add_phi_operands(varindex, phi);
		// might get deleted by try_remove_trivial_phi()
		assert(val->to_Instruction());
	}
	write_variable(varindex, bb, val);
	return val;
}

Value* SSAConstructionPass::add_phi_operands(size_t varindex, PHIInst *phi) {
	// determine operands from predecessors
	BeginInst *BI = phi->get_BeginInst();
	for (BeginInst::PredecessorListTy::const_iterator i = BI->pred_begin(),
			e = BI->pred_end(); i != e; ++i) {
		//
		phi->append_op(read_variable(varindex,beginToIndex[*i]));
	}
	return try_remove_trivial_phi(phi);
}

Value* SSAConstructionPass::try_remove_trivial_phi(PHIInst *phi) {
	Value *same = NULL;
	for(Instruction::OperandListTy::const_iterator i = phi->op_begin(),
			e = phi->op_end(); i != e; ++i) {
		Value *op = *i;
		if ( (op == same) || (op == (Value*)phi) ) {
			continue; // unique value or self-reference
		}
		if (same != NULL) {
			return phi; // not trivial (merges at least two values)
		}
		same = op;
	}
	if (same == NULL) {
		same = NULL; // Phi instruction not reachable
	}
	LOG(BoldGreen << "going to remove PHI: " << reset_color << phi << nl);
	if (DEBUG_COND_N(2)) {
		for (std::size_t i = 0, e = current_def.size(); i < e; ++i) {
			for (std::size_t j = 0, e = current_def[i].size(); j < e; ++j) {
				if (current_def[i][j] == phi) {
					LOG2("current_def["<<i<<"]["<<j<<"] will be invalid" << nl);
				}
			}
		}
	}
	STATISTICS(num_trivial_phis++);
	alloc::list<Instruction*>::type users(phi->user_begin(),phi->user_end());
	users.remove(phi);
	phi->replace_value(same);
	// update table
	// XXX this should be done in a smarter way (e.g. by using value references)
	for (std::size_t i = 0, e = current_def.size(); i < e; ++i) {
		for (std::size_t j = 0, e = current_def[i].size(); j < e; ++j) {
			if (current_def[i][j] == phi) {
				current_def[i][j]  = same;
			}
		}
	}
	// TODO delete phi
	M->remove_Instruction(phi);

	for(alloc::list<Instruction*>::type::iterator i = users.begin(), e = users.end();
			i != e; ++i) {
		assert(*i);
		PHIInst *p = (*i)->to_PHIInst();
		if (p) {
			try_remove_trivial_phi(p);
		}
	}

	return same;
}

void SSAConstructionPass::seal_block(size_t bb) {
	LOG("sealing basic block: " << bb << nl);
	alloc::vector<PHIInst*>::type &inc_phi_bb = incomplete_phi[bb];
	for (int i = 0, e = inc_phi_bb.size(); i != e ; ++i) {
		PHIInst *phi = inc_phi_bb[i];
		if (phi) {
			add_phi_operands(i,phi);
		}
	}
	alloc::list<InVarPhis*>::type &in_var_bb = incomplete_in_phi[bb];
	std::for_each(in_var_bb.begin(),in_var_bb.end(),
		std::mem_fun(&InVarPhis::fill_operands));
	sealed_blocks[bb] = true;
}


bool SSAConstructionPass::try_seal_block(basicblock *bb) {
	if (sealed_blocks[bb->nr])
		return true;

	LOG("try sealing basic block: " << bb->nr << nl);
	for(int i = 0; i < bb->predecessorcount; ++i) {
		int pred_bbindex = bb->predecessors[i]->nr;
		if (!filled_blocks[pred_bbindex] && !skipped_blocks[pred_bbindex]) {
			return false;
		}
	}

	// seal it!
	seal_block(bb->nr);
	#if 0
	// try seal successors
	for(int i = 0; i < bb->successorcount; ++i) {
		try_seal_block(bb->successors[i]);
	}
	#endif

	return true;
}

void SSAConstructionPass::print_current_def() const {
	for(alloc::vector<alloc::vector<Value*>::type >::type::const_iterator i = current_def.begin(),
			e = current_def.end(); i != e ; ++i) {
		for(alloc::vector<Value*>::type::const_iterator ii = (*i).begin(),
				ee = (*i).end(); ii != ee ; ++ii) {
			Value *v = *ii;
			Instruction *I;
			int max = 20;
			if (!v) {
				out() << setw(max) << "NULL";
			} else if ( (I = v->to_Instruction()) ) {
				out() << setw(max) << I->get_name();
			} else {
				out() << setw(max) << "VALUE";
			}
		}
		out() << nl;
	}
}

void SSAConstructionPass::install_javalocal_dependencies(
		SourceStateInst *source_state,
		s4 *javalocals,
		basicblock *bb) {
	if (javalocals) {
		for (s4 i = 0; i < bb->method->maxlocals; ++i) {
			s4 varindex = javalocals[i];
			if (varindex == jitdata::UNUSED) {
				continue;
			}

			if (varindex >= 0) {
				Value *v = read_variable(
						static_cast<size_t>(varindex),
						bb->nr);
				SourceStateInst::Javalocal javalocal(v,
						static_cast<size_t>(i));
				source_state->append_javalocal(javalocal);
				LOG("register javalocal " << varindex << " " << v
						<< " at " << source_state << nl);
			} else {
				assert(0);
				// TODO
			}
		}
	}
}

void SSAConstructionPass::install_stackvar_dependencies(
		SourceStateInst *source_state,
		s4 *stack,
		s4 stackdepth,
		s4 paramcount,
		basicblock *bb) {
	for (s4 stack_index = 0; stack_index < stackdepth; stack_index++) {
		s4 varindex = stack[stack_index];
		Value *v = read_variable(
				static_cast<size_t>(varindex),
				bb->nr);

		if (stack_index < paramcount) {
			source_state->append_param(v);
			LOG("register param " << varindex << " " << v
					<< " at " << source_state << nl);
		} else {
			source_state->append_stackvar(v);
			LOG("register stackvar " << varindex << " " << v
					<< " at " << source_state << nl);
		}
	}
}

SourceStateInst *SSAConstructionPass::record_source_state(
		Instruction *I,
		instruction *iptr,
		basicblock *bb,
		s4 *javalocals,
		s4 *stack,
		s4 stackdepth) {
	s4 source_id = iptr->flags.bits >> INS_FLAG_ID_SHIFT;
	LOG("record source state after instruction " << source_id << " " << I << nl);
	SourceStateInst *source_state = new SourceStateInst(source_id, I);
	install_javalocal_dependencies(source_state, javalocals, bb);
	install_stackvar_dependencies(source_state, stack, stackdepth, 0, bb);
	M->add_Instruction(source_state);
	return source_state;
}

void SSAConstructionPass::deoptimize(int bbindex) {
	assert(BB[bbindex]);
	LOG("Instruction not supported, deoptimize instead" << nl);
	DeoptimizeInst *deopt = new DeoptimizeInst(BB[bbindex]);
	M->add_Instruction(deopt);
	skipped_blocks[bbindex] = true;
}

bool SSAConstructionPass::skipped_all_predecessors(basicblock *bb) {
	for (int i = 0; i < bb->predecessorcount; i++) {
		basicblock *pred = bb->predecessors[i];
		if (visited_blocks[pred->nr] && !skipped_blocks[pred->nr]) {
			return false;
		}
	}
	return true;
}

void SSAConstructionPass::remove_unreachable_blocks() {
	alloc::vector<BeginInst*>::type unreachable_blocks;

	Method::const_bb_iterator end = M->bb_end();
	for (Method::const_bb_iterator i = M->bb_begin(); i != end; i++) {
		BeginInst *begin = *i;
		if (begin->pred_size() == 0 && begin != M->get_init_bb()) {
			unreachable_blocks.push_back(begin);
		}
	}

	while (!unreachable_blocks.empty()) {
		BeginInst *begin = unreachable_blocks.back();
		unreachable_blocks.pop_back();

		LOG("Remove unreachable block " << begin << nl);
		EndInst *end = begin->get_EndInst();
		assert(end);
		M->remove_Instruction(end);
		M->remove_bb(begin);
	}
}

bool SSAConstructionPass::run(JITData &JD) {
	M = JD.get_Method();
	LOG("SSAConstructionPass: " << *M << nl);

	basicblock *bb;
	jd = JD.get_jitdata();

	// **** BEGIN initializations

	/**
	 * There are two kinds of variables in the baseline ir. The javalocals as defined
	 * in the JVM specification (2.6.1.). These are used for arguments and other values
	 * not stored on the JVM stack. These variables are addressed using an index. Every
	 * 'slot' can contain values of each basic type. The type is defined by the access
	 * instruction (e.g. ILOAD, ISTORE for integer). These instructions are introduced
	 * by the java compiler (usually javac).
	 * The other group of variables are intended to replace the jvm stack and are created
	 * by the baseline parse/stackanalysis pass. In contrast to the javalocals these
	 * variables have a fixed typed. They are used as arguments and results for
	 * baseline IR instructions.
	 * For simplicity of the compiler2 IR both variables groups are treated the same way.
	 * Their current definitions are stored in a value numbering table.
	 * The number of variables is already known at this point for both types.
	 */
	//unsigned int num_variables = jd->vartop;
	// last block used for argument loading
	unsigned int num_basicblocks = jd->basicblockcount;
	unsigned int init_basicblock = 0;
	bool extra_init_bb = jd->basicblocks[0].predecessorcount;

	// Used to track at each point the javalocals that are live.
	s4 *live_javalocals = (s4*) DumpMemory::allocate(sizeof(s4) * jd->maxlocals);

	visited_blocks.clear();
	visited_blocks.resize(num_basicblocks, false);

	skipped_blocks.clear();
	skipped_blocks.resize(num_basicblocks, false);

	assert(jd->basicblockcount);
	if (extra_init_bb) {
		// The first basicblock is the target of a jump (e.g. loophead).
		// We have to create a new one to load the arguments
		++num_basicblocks;
		init_basicblock = jd->basicblockcount;
		LOG("Extra basic block added (index = " << init_basicblock << ")" << nl);
	}

	// store basicblock BeginInst
	beginToIndex.clear();
	BB.clear();
	BB.resize(num_basicblocks, NULL);
	for(std::size_t i = 0; i < num_basicblocks; ++i) {
		BeginInst *bi  = new BeginInst();
		BB[i] = bi;
		beginToIndex.insert(std::make_pair(bi,i));
	}
	//
	// pseudo variable index for global state
	std::size_t global_state = jd->vartop;

	// init incomplete_phi
	incomplete_phi.clear();
	incomplete_phi.resize(num_basicblocks,alloc::vector<PHIInst*>::type(global_state + 1,NULL));
	incomplete_in_phi.clear();
	incomplete_in_phi.resize(num_basicblocks);

	// (Local,Global) Value Numbering Map, size #bb times #var, initialized to NULL
	current_def.clear();
	current_def.resize(global_state+1,alloc::vector<Value*>::type(num_basicblocks,NULL));
	// sealed blocks
	sealed_blocks.clear();
	sealed_blocks.resize(num_basicblocks,false);
	// filled blocks
	filled_blocks.clear();
	filled_blocks.resize(num_basicblocks,false);
	// initialize
	var_type_tbl.clear();
	var_type_tbl.resize(global_state+1,Type::VoidTypeID);

	// create variable type map
	for(size_t i = 0, e = jd->vartop; i != e ; ++i) {
		varinfo &v = jd->var[i];
		var_type_tbl[i] = convert_to_typeid(v.type);
	}
	// set global state
	var_type_tbl[global_state] = Type::GlobalStateTypeID;

	// initialize arguments
	methoddesc *md = jd->m->parseddesc;
	assert(md);
	assert(md->paramtypes);

	if (extra_init_bb) {
		M->add_bb(BB[init_basicblock]);
	}
	LOG("parameter count: i = " << md->paramcount << " slot count = " << md->paramslots << nl);
	for (int i = 0, slot = 0; i < md->paramcount; ++i) {
		int type = md->paramtypes[i].type;
		int varindex = jd->local_map[slot * 5 + type];
		LOG("parameter: i = " << i << " slot = " << slot << " type " << get_type_name(type) << nl);

		if (varindex != jitdata::UNUSED) {
			// only load if variable is used
			Instruction *I = new LOADInst(convert_to_typeid(type), i, BB[init_basicblock]);
			write_variable(varindex,init_basicblock,I);
			M->add_Instruction(I);
		}

		switch (type) {
			case TYPE_LNG:
			case TYPE_DBL:
				slot +=2;
				break;
			default:
				++slot;
		}
	}
	if (extra_init_bb) {
		BeginInst *targetBlock = BB[0];
		Instruction *result = new GOTOInst(BB[init_basicblock], targetBlock);
		M->add_Instruction(result);
		// now we mark it filled and sealed
		filled_blocks[init_basicblock] = true;
		sealed_blocks[init_basicblock] = true;
	}

	// set start begin inst
	M->set_init_bb(BB[init_basicblock]);

	current_def[global_state][init_basicblock] = BB[init_basicblock];

	// **** END initializations

	#if defined(ENABLE_LOGGING)
	// print BeginInsts
	for(alloc::vector<BeginInst*>::type::iterator i = BB.begin(), e = BB.end();
			i != e; ++i) {
		Instruction *v = *i;
		LOG("BB: " << (void*)v << nl);
	}
	// print variables
	for(size_t i = 0, e = jd->vartop; i != e ; ++i) {
		varinfo &v = jd->var[i];
		LOG("var#" << i << " type: " << get_type_name(v.type) << nl);
	}
	#endif
	LOG("# variables: " << jd->vartop << nl);
	LOG("# javalocals: " << jd->localcount << nl);
	// print arguments
	LOG("# parameters: " << md->paramcount << nl);
	LOG("# parameter slots: " << md->paramslots << nl);
	for (int i = 0; i < md->paramslots; ++i) {
		int type = md->paramtypes[i].type;
		LOG("argument type: " << get_type_name(type)
		    << " (index " << i << ")"
		    << " (var_local " << jd->local_map[i * 5 + type] << ")" << nl);
		switch (type) {
			case TYPE_LNG:
			case TYPE_DBL:
				++i;
				break;
		}
	}
	for (int i = 0; i < jd->maxlocals; ++i) {
		LOG("localindex: " << i << nl);
		for (int j = 0; j < 5; ++j) {
			s4 entry = jd->local_map[i*5+j];
			if (entry == jitdata::UNUSED)
				LOG("  type " <<get_type_name(j) << " UNUSED" << nl);
			else
				LOG("  type " <<get_type_name(j) << " " << entry << nl);
		}
	}

	FOR_EACH_BASICBLOCK(jd,bb) {
		visited_blocks[bb->nr] = true;

		if (bb->icount == 0 ) {
			// this is the last (empty) basicblock
			assert(bb->nr == jd->basicblockcount);
			assert(bb->predecessorcount == 0);
			assert(bb->successorcount == 0);
			// we dont need it
			continue;
		}

		if ((skipped_all_predecessors(bb) && bb->predecessorcount > 0) || bb->type == basicblock::TYPE_EXH) {
			skipped_blocks[bb->nr] = true;
			DeoptimizeInst *deopt = new DeoptimizeInst(BB[bb->nr]);
			M->add_Instruction(deopt);
		}

		std::size_t bbindex = (std::size_t)bb->nr;
		instruction *iptr;
		LOG("basicblock: " << bbindex << nl);

		if (!skipped_blocks[bbindex]) {
			// handle invars
			for(s4 i = 0; i < bb->indepth; ++i) {
				std::size_t varindex = bb->invars[i];
				PHIInst *phi = new PHIInst(var_type_tbl[varindex], BB[bbindex]);
				write_variable(varindex, bbindex, phi);
				if (!sealed_blocks[bbindex]) {
					incomplete_in_phi[bbindex].push_back(new InVarPhis(phi, jd, bbindex, i, this));
				}
				else {
					InVarPhis invar(phi, jd, bbindex, i, this);
					invar.fill_operands();
				}
				M->add_Instruction(phi);
			}
		}

		// add begin block
		assert(BB[bbindex]);
		M->add_bb(BB[bbindex]);
		// every merge node is a possible statechange
		// XXX maybe we can handle this via phi nodes but we must ensure that
		// they do not get deleted!
		current_def[global_state][bbindex] = BB[bbindex];

		// Get javalocals that are live at the begin of the block.
		assert((jd->maxlocals > 0) == (bb->javalocals != NULL));
		MCOPY(live_javalocals, bb->javalocals, s4, jd->maxlocals);

		if (!skipped_blocks[bbindex]) {
			if (bb->predecessorcount > 1 || bb->nr == 0) {
				// Record the source state at method entry and control-flow merges.
				SourceStateInst *source_state = record_source_state(BB[bbindex],
						bb->iinstr, bb, live_javalocals, bb->invars, bb->indepth);

#if defined(ENABLE_COUNTDOWN_TRAPS)
				// For now it is only possible to jump into optimized code at the begin
				// of methods. Hence, we currently only place a ReplacementEntryInst at
				// method entry.
				if (bb->nr == 0) {
					ReplacementEntryInst *rplentry = new ReplacementEntryInst(BB[bbindex], source_state);
					M->add_Instruction(rplentry);
				}
#endif
			}
		}

		FOR_EACH_INSTRUCTION(bb,iptr) {
			if (skipped_blocks[bb->nr]) {
				break;
			}

			STATISTICS(++num_icmd_inst);
			#if !defined(NDEBUG)
			LOG("iptr: " << icmd_table[iptr->opc].name << nl);
			#endif

			Instruction *old_global_state = read_variable(global_state,bbindex)->to_Instruction();
			Instruction *new_global_state = NULL;

			switch (iptr->opc) {
			case ICMD_NOP:
				//M->add_Instruction(new NOPInst());
				break;
			case ICMD_POP:
			case ICMD_CHECKNULL:
				{
					deoptimize(bbindex);
					break;
				}
		//		/* unary */
			case ICMD_INEG:
			case ICMD_LNEG:
			case ICMD_FNEG:
			case ICMD_DNEG:
				{
					Value *s1 = read_variable(iptr->s1.varindex, bbindex);
					Type::TypeID type;
					switch (iptr->opc) {
					case ICMD_INEG:
						type = Type::IntTypeID;
						break;
					case ICMD_LNEG:
						type = Type::LongTypeID;
						break;
					case ICMD_FNEG:
						type = Type::FloatTypeID;
						break;
					case ICMD_DNEG:
						type = Type::DoubleTypeID;
						break;
					default: assert(0);
						type = Type::VoidTypeID;
					}
					Instruction *result = new NEGInst(type,s1);
					write_variable(iptr->dst.varindex,bbindex,result);
					M->add_Instruction(result);
					break;
				}
			case ICMD_ARRAYLENGTH:
				{
					Value *s1 = read_variable(iptr->s1.varindex, bbindex);
					Instruction *result = new ARRAYLENGTHInst(s1);
					write_variable(iptr->dst.varindex,bbindex,result);
					M->add_Instruction(result);
					break;
				}
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
				{
					Value *s1 = read_variable(iptr->s1.varindex, bbindex);
					//Type::TypeID type_from;
					Type::TypeID type_to;
					switch (iptr->opc) {
					case ICMD_INT2SHORT:
						type_to = Type::ShortTypeID;
						break;
					case ICMD_INT2CHAR:
						type_to = Type::CharTypeID;
						break;
					case ICMD_INT2BYTE:
						type_to = Type::ByteTypeID;
						break;
					case ICMD_I2L:
						//type_from = Type::IntTypeID;
						type_to = Type::LongTypeID;
						break;
					case ICMD_I2F:
						//type_from = Type::IntTypeID;
						type_to = Type::FloatTypeID;
						break;
					case ICMD_I2D:
						//type_from = Type::IntTypeID;
						type_to = Type::DoubleTypeID;
						break;
					case ICMD_L2I:
						//type_from = Type::LongTypeID;
						type_to = Type::IntTypeID;
						break;
					case ICMD_L2F:
						//type_from = Type::LongTypeID;
						type_to = Type::FloatTypeID;
						break;
					case ICMD_L2D:
						//type_from = Type::LongTypeID;
						type_to = Type::DoubleTypeID;
						break;
					case ICMD_F2I:
						//type_from = Type::FloatTypeID;
						type_to = Type::IntTypeID;
						break;
					case ICMD_F2L:
						//type_from = Type::FloatTypeID;
						type_to = Type::LongTypeID;
						break;
					case ICMD_F2D:
						//type_from = Type::FloatTypeID;
						type_to = Type::DoubleTypeID;
						break;
					case ICMD_D2I:
						//type_from = Type::DoubleTypeID;
						type_to = Type::IntTypeID;
						break;
					case ICMD_D2L:
						//type_from = Type::DoubleTypeID;
						type_to = Type::LongTypeID;
						break;
					case ICMD_D2F:
						//type_from = Type::DoubleTypeID;
						type_to = Type::FloatTypeID;
						break;
					default:
						assert(0);
						type_to = Type::VoidTypeID;
					}
					Instruction *result = new CASTInst(type_to, s1);
					write_variable(iptr->dst.varindex,bbindex,result);
					M->add_Instruction(result);
					break;
				}

			case ICMD_IADD:
			case ICMD_LADD:
			case ICMD_FADD:
			case ICMD_DADD:
				{
					Value *s1 = read_variable(iptr->s1.varindex, bbindex);
					Value *s2 = read_variable(iptr->sx.s23.s2.varindex,bbindex);
					Type::TypeID type;
					switch (iptr->opc) {
					case ICMD_IADD:
						type = Type::IntTypeID;
						break;
					case ICMD_LADD:
						type = Type::LongTypeID;
						break;
					case ICMD_FADD:
						type = Type::FloatTypeID;
						break;
					case ICMD_DADD:
						type = Type::DoubleTypeID;
						break;
					default: assert(0);
						type = Type::VoidTypeID;
					}
					Instruction *result = new ADDInst(type, s1, s2);
					write_variable(iptr->dst.varindex,bbindex,result);
					M->add_Instruction(result);
				}
				break;
			case ICMD_ISUB:
			case ICMD_LSUB:
			case ICMD_FSUB:
			case ICMD_DSUB:
				{
					Value *s1 = read_variable(iptr->s1.varindex, bbindex);
					Value *s2 = read_variable(iptr->sx.s23.s2.varindex,bbindex);
					Type::TypeID type;
					switch (iptr->opc) {
					case ICMD_ISUB:
						type = Type::IntTypeID;
						break;
					case ICMD_LSUB:
						type = Type::LongTypeID;
						break;
					case ICMD_FSUB:
						type = Type::FloatTypeID;
						break;
					case ICMD_DSUB:
						type = Type::DoubleTypeID;
						break;
					default: assert(0);
						type = Type::VoidTypeID;
					}
					Instruction *result = new SUBInst(type, s1, s2);
					write_variable(iptr->dst.varindex,bbindex,result);
					M->add_Instruction(result);
				}
				break;
			case ICMD_IMUL:
			case ICMD_LMUL:
			case ICMD_FMUL:
			case ICMD_DMUL:
				{
					Value *s1 = read_variable(iptr->s1.varindex, bbindex);
					Value *s2 = read_variable(iptr->sx.s23.s2.varindex,bbindex);
					Type::TypeID type;
					switch (iptr->opc) {
					case ICMD_IMUL:
						type = Type::IntTypeID;
						break;
					case ICMD_LMUL:
						type = Type::LongTypeID;
						break;
					case ICMD_FMUL:
						type = Type::FloatTypeID;
						break;
					case ICMD_DMUL:
						type = Type::DoubleTypeID;
						break;
					default: assert(0);
						type = Type::VoidTypeID;
					}
					Instruction *result = new MULInst(type, s1, s2);
					write_variable(iptr->dst.varindex,bbindex,result);
					M->add_Instruction(result);
				}
				break;
			case ICMD_IDIV:
			case ICMD_LDIV:
			case ICMD_FDIV:
			case ICMD_DDIV:
				{
					Value *s1 = read_variable(iptr->s1.varindex, bbindex);
					Value *s2 = read_variable(iptr->sx.s23.s2.varindex,bbindex);
					Type::TypeID type;
					switch (iptr->opc) {
					case ICMD_IDIV:
						type = Type::IntTypeID;
						break;
					case ICMD_LDIV:
						type = Type::LongTypeID;
						break;
					case ICMD_FDIV:
						type = Type::FloatTypeID;
						break;
					case ICMD_DDIV:
						type = Type::DoubleTypeID;
						break;
					default: assert(0);
						type = Type::VoidTypeID;
					}
					Instruction *result = new DIVInst(type, s1, s2);
					write_variable(iptr->dst.varindex,bbindex,result);
					M->add_Instruction(result);
				}
				break;
			case ICMD_IREM:
			case ICMD_LREM:
			case ICMD_FREM:
			case ICMD_DREM:
				{
					Value *s1 = read_variable(iptr->s1.varindex, bbindex);
					Value *s2 = read_variable(iptr->sx.s23.s2.varindex,bbindex);
					Type::TypeID type;
					switch (iptr->opc) {
					case ICMD_IREM:
						type = Type::IntTypeID;
						break;
					case ICMD_LREM:
						type = Type::LongTypeID;
						break;
					case ICMD_FREM:
						type = Type::FloatTypeID;
						break;
					case ICMD_DREM:
						type = Type::DoubleTypeID;
						break;
					default: assert(0);
						type = Type::VoidTypeID;
					}
					Instruction *result = new REMInst(type, s1, s2);
					write_variable(iptr->dst.varindex,bbindex,result);
					M->add_Instruction(result);
				}
				break;
			case ICMD_ISHL:
			case ICMD_LSHL:
			case ICMD_ISHR:
			case ICMD_LSHR:
			case ICMD_IUSHR:
			case ICMD_LUSHR:
				{
					deoptimize(bbindex);
					break;
				}
			case ICMD_IOR:
			case ICMD_LOR:
			case ICMD_IXOR:
			case ICMD_LXOR:
			case ICMD_IAND:
			case ICMD_LAND:
				{
					Value *s1 = read_variable(iptr->s1.varindex, bbindex);
					Value *s2 = read_variable(iptr->sx.s23.s2.varindex,bbindex);
					Type::TypeID type;
					Instruction *result;
					// type
					switch (iptr->opc) {
					case ICMD_IAND:
					case ICMD_IOR:
					case ICMD_IXOR:
						type = Type::IntTypeID;
						break;
					case ICMD_LAND:
					case ICMD_LOR:
					case ICMD_LXOR:
						type = Type::LongTypeID;
						break;
					default: assert(0);
						type = Type::VoidTypeID;
					}
					// operation
					switch (iptr->opc) {
					case ICMD_IAND:
					case ICMD_LAND:
						result = new ANDInst(type, s1, s2);
						break;
					case ICMD_IOR:
					case ICMD_LOR:
						result = new ORInst(type, s1, s2);
						break;
					case ICMD_IXOR:
					case ICMD_LXOR:
						result = new XORInst(type, s1, s2);
						break;
					default: assert(0);
						result = NULL;
					}
					write_variable(iptr->dst.varindex,bbindex,result);
					M->add_Instruction(result);
				}
				break;
			case ICMD_LCMP:
			case ICMD_FCMPL:
			case ICMD_FCMPG:
			case ICMD_DCMPL:
			case ICMD_DCMPG:
				{
					Value *s1 = read_variable(iptr->s1.varindex, bbindex);
					Value *s2 = read_variable(iptr->sx.s23.s2.varindex,bbindex);
					CMPInst::FloatHandling handling;
					switch (iptr->opc) {
					case ICMD_FCMPL:
					case ICMD_DCMPL:
						handling = CMPInst::L;
						break;
					case ICMD_FCMPG:
					case ICMD_DCMPG:
						handling = CMPInst::G;
						break;
					default: assert(0);
						handling = CMPInst::DontCare;
					}
					Instruction *result = new CMPInst(s1, s2, handling);
					write_variable(iptr->dst.varindex,bbindex,result);
					M->add_Instruction(result);
				}
				break;
		//		SHOW_S1(OS, ipt);
		//		SHOW_S2(OS, iptr);
		//		SHOW_DST(OS, iptr);
		//		break;

				/* binary/const INT */
			case ICMD_IADDCONST:
			case ICMD_ISUBCONST:
				{
					Value *s1 = read_variable(iptr->s1.varindex,bbindex);
					Instruction *konst = new CONSTInst(iptr->sx.val.i,Type::IntType());
					Instruction *result;
					switch (iptr->opc) {
					case ICMD_IADDCONST:
						result = new ADDInst(Type::IntTypeID, s1, konst);
						break;
					case ICMD_ISUBCONST:
						result = new SUBInst(Type::IntTypeID, s1, konst);
						break;
					default: assert(0);
						result = 0;
					}
					M->add_Instruction(konst);
					write_variable(iptr->dst.varindex,bbindex,result);
					M->add_Instruction(result);
				}
				break;
			case ICMD_IMULCONST:
				{
					Value *s1 = read_variable(iptr->s1.varindex,bbindex);
					Instruction *konst = new CONSTInst(iptr->sx.val.i,Type::IntType());
					Instruction *result;
					result = new MULInst(Type::IntTypeID, s1, konst);
					M->add_Instruction(konst);
					write_variable(iptr->dst.varindex,bbindex,result);
					M->add_Instruction(result);
				}
				break;
			case ICMD_IMULPOW2:
				{
					deoptimize(bbindex);
					break;
				}
			case ICMD_IREMPOW2:
			case ICMD_IDIVPOW2:
				// FIXME we should lower this to a shift
				// XXX it the constant the divisor or the shift amount?
			case ICMD_IANDCONST:
				{
					Value *s1 = read_variable(iptr->s1.varindex,bbindex);
					Instruction *konst = new CONSTInst(iptr->sx.val.i,Type::IntType());
					Instruction *result;
					switch (iptr->opc) {
					case ICMD_IANDCONST:
						result = new ANDInst(Type::IntTypeID, s1, konst);
						break;
					case ICMD_IDIVPOW2:
						result = new DIVInst(Type::IntTypeID, s1, konst);
						break;
					case ICMD_IREMPOW2:
						result = new REMInst(Type::IntTypeID, s1, konst);
						break;
					default: assert(0);
						result = NULL;
					}
					M->add_Instruction(konst);
					write_variable(iptr->dst.varindex,bbindex,result);
					M->add_Instruction(result);
				}
				break;
			case ICMD_IORCONST:
			case ICMD_IXORCONST:
			case ICMD_ISHLCONST:
			case ICMD_ISHRCONST:
			case ICMD_IUSHRCONST:
			case ICMD_LSHLCONST:
			case ICMD_LSHRCONST:
			case ICMD_LUSHRCONST:
		//		SHOW_S1(OS, iptr);
		//		SHOW_INT_CONST(OS, iptr->sx.val.i);	
		//		SHOW_DST(OS, iptr);
		//		break;

				/* ?ASTORECONST (trinary/const INT) */

			case ICMD_BASTORECONST:
			case ICMD_CASTORECONST:
			case ICMD_SASTORECONST:
			case ICMD_AASTORECONST:
				{
					deoptimize(bbindex);
					break;
				}
			case ICMD_IASTORECONST:
			case ICMD_LASTORECONST:
				{
					Value *s1 = read_variable(iptr->s1.varindex, bbindex);
					Value *s2 = read_variable(iptr->sx.s23.s2.varindex,bbindex);
					Instruction *konst;
					Type::TypeID type;
					switch (iptr->opc) {
					case ICMD_IASTORECONST:
						type = Type::IntTypeID;
						konst = new CONSTInst(iptr->sx.s23.s3.constval,Type::IntType());
						break;
					#if 0
					case ICMD_BASTORECONST:
						type = Type::ByteTypeID;
						konst = new CONSTInst(iptr->sx.s23.s3.constval,Type::ByteType());
						break;
					case ICMD_CASTORECONST:
						type = Type::CharTypeID;
						konst = new CONSTInst(iptr->sx.s23.s3.constval,Type::CharType());
						break;
					case ICMD_SASTORECONST:
						type = Type::ShortTypeID;
						konst = new CONSTInst(iptr->sx.s23.s3.constval,Type::ShortType());
						break;
					case ICMD_AASTORECONST:
						type = Type::ReferenceTypeID;
						konst = new CONSTInst(iptr->sx.s23.s3.constval,Type::ReferenceType());
						break;
					#endif
					case ICMD_LASTORECONST:
						type = Type::LongTypeID;
						konst = new CONSTInst(iptr->sx.s23.s3.constval,Type::LongType());
						break;
					default: assert(0);
						type = Type::VoidTypeID;
						konst = NULL;
					}
					M->add_Instruction(konst);
					Instruction *state_change = read_variable(global_state,bbindex)->to_Instruction();
					assert(state_change);
					
					Instruction *ref = new AREFInst(type, s1, s2);
					M->add_Instruction(ref);
					Instruction *result = new ASTOREInst(type, ref, konst, BB[bbindex], state_change);
					write_variable(global_state,bbindex,result);
					M->add_Instruction(result);
					Instruction *boundscheck = new ARRAYBOUNDSCHECKInst(type, s1, s2);
					ref->append_dep(boundscheck);
					M->add_Instruction(boundscheck);
				}
				break;
		//		SHOW_S1(OS, iptr);
		//		SHOW_S2(OS, iptr);
		//		SHOW_INT_CONST(OS, iptr->sx.s23.s3.constval);
		//		break;

			case ICMD_ICONST:
			case ICMD_LCONST:
			case ICMD_FCONST:
			case ICMD_DCONST:
				{
					Instruction *I;
					switch (iptr->opc) {
					case ICMD_ICONST:
						I = new CONSTInst(iptr->sx.val.i,Type::IntType());
						break;
					case ICMD_LCONST:
						I = new CONSTInst(iptr->sx.val.l,Type::LongType());
						break;
					case ICMD_FCONST:
						I = new CONSTInst(iptr->sx.val.f,Type::FloatType());
						break;
					case ICMD_DCONST:
						I = new CONSTInst(iptr->sx.val.d,Type::DoubleType());
						break;
					default: assert(0);
						I = NULL;
					}
					write_variable(iptr->dst.varindex,bbindex,I);
					M->add_Instruction(I);
				}
				break;

				/* binary/const LNG */
			case ICMD_LADDCONST:
				{
					Instruction *konst = new CONSTInst(iptr->sx.val.l,Type::LongType());
					Value *s1 = read_variable(iptr->s1.varindex,bbindex);
					Instruction *result = new ADDInst(Type::LongTypeID, s1, konst);
					M->add_Instruction(konst);
					write_variable(iptr->dst.varindex,bbindex,result);
					M->add_Instruction(result);
				}
				break;
			case ICMD_LSUBCONST:
				{
					Instruction *konst = new CONSTInst(iptr->sx.val.l,Type::LongType());
					Value *s1 = read_variable(iptr->s1.varindex,bbindex);
					Instruction *result = new SUBInst(Type::LongTypeID, s1, konst);
					M->add_Instruction(konst);
					write_variable(iptr->dst.varindex,bbindex,result);
					M->add_Instruction(result);
				}
				break;
			case ICMD_LMULCONST:
			{
				Instruction *konst = new CONSTInst(iptr->sx.val.l,Type::LongType());
				Value *s1 = read_variable(iptr->s1.varindex,bbindex);
				Instruction *result = new MULInst(Type::LongTypeID, s1, konst);
				M->add_Instruction(konst);
				write_variable(iptr->dst.varindex,bbindex,result);
				M->add_Instruction(result);
			}
			break;
			case ICMD_LANDCONST:
				{
					Value *s1 = read_variable(iptr->s1.varindex,bbindex);
					Instruction *konst = new CONSTInst(iptr->sx.val.l,Type::LongType());
					Instruction *result;
					switch (iptr->opc) {
					case ICMD_LANDCONST:
						result = new ANDInst(Type::LongTypeID, s1, konst);
						break;
					default: assert(0);
						result = 0;
					}
					M->add_Instruction(konst);
					write_variable(iptr->dst.varindex,bbindex,result);
					M->add_Instruction(result);
				}
				break;
			case ICMD_LMULPOW2:
				{
					deoptimize(bbindex);
					break;
				}
			case ICMD_LDIVPOW2:
				{
					Value *s1 = read_variable(iptr->s1.varindex,bbindex);
					Instruction *konst = new CONSTInst(iptr->sx.val.l,Type::LongType());
					Instruction *result;
					switch (iptr->opc) {
					case ICMD_LDIVPOW2:
						// FIXME this is not right!
						result = new SUBInst(Type::LongTypeID, s1, konst);
						break;
					default: assert(0);
						result = 0;
					}
					M->add_Instruction(konst);
					write_variable(iptr->dst.varindex,bbindex,result);
					M->add_Instruction(result);
				}
				break;
			case ICMD_LREMPOW2:
			case ICMD_LORCONST:
			case ICMD_LXORCONST:
		//		SHOW_S1(OS, iptr);
		//		SHOW_LNG_CONST(OS, iptr->sx.val.l);
		//		SHOW_DST(OS, iptr);
		//		break;

				/* trinary/const LNG (<= pointer size) */
		//		SHOW_S1(OS, iptr);
		//		SHOW_S2(OS, iptr);
		//		SHOW_ADR_CONST(OS, iptr->sx.s23.s3.constval);
		//		break;

				/* const ADR */
			case ICMD_ACONST:
		//		if (iptr->flags.bits & INS_FLAG_CLASS) {
		//			SHOW_ADR_CONST(OS, iptr->sx.val.anyptr);
		//			SHOW_CLASSREF_OR_CLASSINFO(OS, iptr->sx.val.c);
		//		}
		//		else if (iptr->sx.val.anyptr == NULL) {
		//			OS << "NULL ";
		//		}
		//		else {
		//			SHOW_ADR_CONST(OS, iptr->sx.val.anyptr);
		//			SHOW_STRING(OS, iptr->sx.val.stringconst);
		//		}
		//		SHOW_DST(OS, iptr);
		//		break;

		//		SHOW_S1(OS, iptr);
		//		SHOW_S2(OS, iptr);
		//		printf("%p ", (void*) iptr->sx.s23.s3.constval);
		//		break;

			{
				assert(INSTRUCTION_IS_RESOLVED(iptr));
				Instruction *I = new CONSTInst(iptr->sx.val.anyptr, Type::ReferenceType());
				write_variable(iptr->dst.varindex,bbindex,I);
				M->add_Instruction(I);
			}
			break;
			case ICMD_GETFIELD:        /* 1 -> 1 */
				{
					if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
						deoptimize(bbindex);
						break;
					}

					constant_FMIref *fmiref;
					INSTRUCTION_GET_FIELDREF(iptr, fmiref);
					fieldinfo* field = fmiref->p.field;

					Instruction *state_change = read_variable(global_state,bbindex)->to_Instruction();
					assert(state_change);

					Type::TypeID type = convert_to_typeid(fmiref->parseddesc.fd->type);
					Value *objectref = read_variable(iptr->s1.varindex,bbindex);
					Instruction *getfield = new GETFIELDInst(type, objectref, field, BB[bbindex], state_change);

					write_variable(iptr->dst.varindex, bbindex, getfield);
					write_variable(global_state, bbindex, getfield);

					M->add_Instruction(getfield);
				}
				break;
			case ICMD_PUTFIELD:        /* 2 -> 0 */
				{
					if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
						deoptimize(bbindex);
						break;
					}

					constant_FMIref *fmiref;
					INSTRUCTION_GET_FIELDREF(iptr, fmiref);
					fieldinfo* field = fmiref->p.field;

					Instruction *state_change = read_variable(global_state, bbindex)->to_Instruction();
					assert(state_change);

					Value *objectref = read_variable(iptr->s1.varindex, bbindex);
					Value *value = read_variable(iptr->sx.s23.s2.varindex, bbindex);
					Instruction *putfield = new PUTFIELDInst(objectref, value, field, BB[bbindex],
							state_change);

					write_variable(global_state, bbindex, putfield);

					M->add_Instruction(putfield);
				}
				break;
			case ICMD_GETSTATIC:       /* 0 -> 1 */
				{
					if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
						deoptimize(bbindex);
						break;
					}

					constant_FMIref *fmiref;
					INSTRUCTION_GET_FIELDREF(iptr, fmiref);
					fieldinfo* field = fmiref->p.field;

					if (!class_is_or_almost_initialized(field->clazz)) {
						deoptimize(bbindex);
						break;
					}

					Instruction *state_change = read_variable(global_state,bbindex)->to_Instruction();
					assert(state_change);

					Type::TypeID type = convert_to_typeid(fmiref->parseddesc.fd->type);
					Instruction *getstatic = new GETSTATICInst(type, field,
							BB[bbindex], state_change);

					write_variable(iptr->dst.varindex, bbindex, getstatic);
					write_variable(global_state, bbindex, getstatic);

					M->add_Instruction(getstatic);
				}
				break;
			case ICMD_PUTSTATIC:       /* 1 -> 0 */
				{
					if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
						deoptimize(bbindex);
						break;
					}

					constant_FMIref *fmiref;
					INSTRUCTION_GET_FIELDREF(iptr, fmiref);
					fieldinfo* field = fmiref->p.field;

					if (!class_is_or_almost_initialized(field->clazz)) {
						deoptimize(bbindex);
						break;
					}

					Instruction *state_change = read_variable(global_state,bbindex)->to_Instruction();
					assert(state_change);

					Value *s1 = read_variable(iptr->s1.varindex,bbindex);
					Instruction *putstatic = new PUTSTATICInst(s1, field,
							BB[bbindex], state_change);

					write_variable(global_state, bbindex, putstatic);

					M->add_Instruction(putstatic);
				}
				break;
			case ICMD_PUTSTATICCONST:  /* 0 -> 0 */
			case ICMD_PUTFIELDCONST:   /* 1 -> 0 */
		//		if (opcode != ICMD_GETSTATIC && opcode != ICMD_PUTSTATICCONST) {
		//			SHOW_S1(OS, iptr);
		//			if (opcode == ICMD_PUTFIELD) {
		//				SHOW_S2(OS, iptr);
		//			}
		//		}
		//		INSTRUCTION_GET_FIELDREF(iptr, fmiref);
		//		SHOW_FIELD(OS, fmiref);
		//
		//		if (opcode == ICMD_GETSTATIC || opcode == ICMD_GETFIELD) {
		//			SHOW_DST(OS, iptr);
		//		}
		//		break;

				{
					deoptimize(bbindex);
					break;
				}
			case ICMD_IINC:
				{
					Value *s1 = read_variable(iptr->s1.varindex,bbindex);
					Instruction *konst = new CONSTInst(iptr->sx.val.i,Type::IntType());
					Instruction *result;
					switch (iptr->opc) {
					case ICMD_IINC:
						result = new ADDInst(Type::IntTypeID, s1, konst);
						break;
					default: assert(0);
						result = 0;
					}
					M->add_Instruction(konst);
					write_variable(iptr->dst.varindex,bbindex,result);
					M->add_Instruction(result);
				}
				break;
		//		SHOW_S1_LOCAL(OS, iptr);
		//		SHOW_INT_CONST(OS, iptr->sx.val.i);
		//		SHOW_DST_LOCAL(OS, iptr);
		//		break;

			case ICMD_IASTORE:
			case ICMD_SASTORE:
			case ICMD_BASTORE:
			case ICMD_CASTORE:
			case ICMD_LASTORE:
			case ICMD_DASTORE:
			case ICMD_FASTORE:
			case ICMD_AASTORE:
				{
					Value *s1 = read_variable(iptr->s1.varindex, bbindex);
					Value *s2 = read_variable(iptr->sx.s23.s2.varindex,bbindex);
					Value *s3 = read_variable(iptr->sx.s23.s3.varindex,bbindex);
					Type::TypeID type;
					switch (iptr->opc) {
					case ICMD_BASTORE:
						type = Type::ByteTypeID;
						break;
					case ICMD_CASTORE:
						type = Type::CharTypeID;
						break;
					case ICMD_SASTORE:
						type = Type::ShortTypeID;
						break;
					case ICMD_IASTORE:
						type = Type::IntTypeID;
						break;
					case ICMD_LASTORE:
						type = Type::LongTypeID;
						break;
					case ICMD_AASTORE:
						type = Type::ReferenceTypeID;
						break;
					case ICMD_FASTORE:
						type = Type::FloatTypeID;
						break;
					case ICMD_DASTORE:
						type = Type::DoubleTypeID;
						break;
					default: assert(0);
						type = Type::VoidTypeID;
					}
					Instruction *state_change = read_variable(global_state,bbindex)->to_Instruction();
					assert(state_change);
					Instruction *ref = new AREFInst(s3->get_type(), s1, s2);
					M->add_Instruction(ref);
					Instruction *result = new ASTOREInst(type, ref, s3, BB[bbindex], state_change);
					write_variable(global_state,bbindex,result);
					M->add_Instruction(result);
					Instruction *boundscheck = new ARRAYBOUNDSCHECKInst(type, s1, s2);
					ref->append_dep(boundscheck);
					M->add_Instruction(boundscheck);
				}
				break;
			case ICMD_IALOAD:
			case ICMD_SALOAD:
			case ICMD_BALOAD:
			case ICMD_CALOAD:
			case ICMD_LALOAD:
			case ICMD_DALOAD:
			case ICMD_FALOAD:
			case ICMD_AALOAD:
				{
					Value *s1 = read_variable(iptr->s1.varindex, bbindex);
					Value *s2 = read_variable(iptr->sx.s23.s2.varindex,bbindex);
					Type::TypeID type;
					switch (iptr->opc) {
					case ICMD_BALOAD:
						type = Type::ByteTypeID;
						break;
					case ICMD_CALOAD:
						type = Type::CharTypeID;
						break;
					case ICMD_SALOAD:
						type = Type::ShortTypeID;
						break;
					case ICMD_IALOAD:
						type = Type::IntTypeID;
						break;
					case ICMD_LALOAD:
						type = Type::LongTypeID;
						break;
					case ICMD_AALOAD:
						type = Type::ReferenceTypeID;
						break;
					case ICMD_FALOAD:
						type = Type::FloatTypeID;
						break;
					case ICMD_DALOAD:
						type = Type::DoubleTypeID;
						break;
					default: assert(0);
						type = Type::VoidTypeID;
					}
					Instruction *state_change = read_variable(global_state,bbindex)->to_Instruction();
					assert(state_change);
					Instruction *ref = new AREFInst(type, s1, s2);
					M->add_Instruction(ref);
					Instruction *result = new ALOADInst(type, ref, BB[bbindex], state_change);
					write_variable(iptr->dst.varindex,bbindex,result);
					write_variable(global_state,bbindex,result);
					M->add_Instruction(result);
					Instruction *boundscheck = new ARRAYBOUNDSCHECKInst(type, s1, s2);
					ref->append_dep(boundscheck);
					M->add_Instruction(boundscheck);
				}
				break;
		//		SHOW_S1(OS, iptr);
		//		SHOW_S2(OS, iptr);
		//		SHOW_DST(OS, iptr);
		//		break;

			case ICMD_RET:
				{
					deoptimize(bbindex);
					break;
				}
		//		SHOW_S1_LOCAL(OS, iptr);
		//		if (stage >= SHOW_STACK) {
		//			printf(" ---> L%03d", iptr->dst.block->nr);
		//		}
		//		break;

			case ICMD_ILOAD:
			case ICMD_LLOAD:
			case ICMD_FLOAD:
			case ICMD_DLOAD:
			case ICMD_ALOAD:
				{
					#if 0
					Instruction *I;
					switch (iptr->opc) {
					case ICMD_ILOAD:
						I = new LOADInst(Type::IntTypeID, iptr->s1.varindex);
						break;
					case ICMD_LLOAD:
						I = new LOADInst(Type::LongTypeID, iptr->s1.varindex);
						break;
					case ICMD_FLOAD:
						I = new LOADInst(Type::FloatTypeID, iptr->s1.varindex);
						break;
					case ICMD_DLOAD:
						I = new LOADInst(Type::DoubleTypeID, iptr->s1.varindex);
						break;
					default: assert(0);
					}
					#endif
					Value *def = read_variable(iptr->s1.varindex,bbindex);
					// XXX dont yet know if this can happen
					#if 0
					if (!def) {
						// no definition found. Initialize with zero
						Instruction *konst;
						switch (iptr->opc) {
						case ICMD_ILOAD:
							konst = new CONSTInst((int)0);
							break;
						case ICMD_LLOAD:
							konst = new CONSTInst((long)0);
							break;
						case ICMD_FLOAD:
							konst = new CONSTInst((float)0);
							break;
						case ICMD_DLOAD:
							konst = new CONSTInst((double)0);
							break;
						default: assert(0);
						}
						M->add_Instruction(konst);
						def = konst;
					}
					#else
					assert(def);
					#endif
					write_variable(iptr->dst.varindex,bbindex,def);
				}
				break;
			case ICMD_ISTORE:
			case ICMD_LSTORE:
			case ICMD_FSTORE:
			case ICMD_DSTORE:
			case ICMD_ASTORE:
				{
					Value *s1 = read_variable(iptr->s1.varindex,bbindex);
					write_variable(iptr->dst.varindex,bbindex,s1);
					stack_javalocals_store(iptr, live_javalocals);
				}
				break;
		//		SHOW_S1(OS, iptr);
		//		SHOW_DST_LOCAL(OS, iptr);
		//		if (stage >= SHOW_STACK && iptr->sx.s23.s3.javaindex != UNUSED)
		//			printf(" (javaindex %d)", iptr->sx.s23.s3.javaindex);
		//		if (iptr->flags.bits & INS_FLAG_RETADDR) {
		//			printf(" (retaddr L%03d)", RETADDR_FROM_JAVALOCAL(iptr->sx.s23.s2.retaddrnr));
		//		}
		//		break;

			case ICMD_NEW:
		//		SHOW_DST(OS, iptr);
		//		break;
		//
			case ICMD_NEWARRAY:
		//		SHOW_DST(OS, iptr);
		//		break;

			case ICMD_ANEWARRAY:
		//		SHOW_DST(OS, iptr);
		//		break;

			case ICMD_MULTIANEWARRAY:
		//		if (stage >= SHOW_STACK) {
		//			argp = iptr->sx.s23.s2.args;
		//			i = iptr->s1.argcount;
		//			while (i--) {
		//				SHOW_VARIABLE(OS, *(argp++));
		//			}
		//		}
		//		else {
		//			printf("argcount=%d ", iptr->s1.argcount);
		//		}
		//		class_classref_or_classinfo_print(iptr->sx.s23.s3.c);
		//		putchar(' ');
		//		SHOW_DST(OS, iptr);
		//		break;

			case ICMD_CHECKCAST:
		//		SHOW_S1(OS, iptr);
		//		class_classref_or_classinfo_print(iptr->sx.s23.s3.c);
		//		putchar(' ');
		//		SHOW_DST(OS, iptr);
		//		break;
		//
			case ICMD_INSTANCEOF:
		//		SHOW_S1(OS, iptr);
		//		SHOW_DST(OS, iptr);
		//		break;

			case ICMD_INLINE_START:
			case ICMD_INLINE_END:
			case ICMD_INLINE_BODY:
		#if 0
		#if defined(ENABLE_INLINING)
				{
					insinfo_inline *ii = iptr->sx.s23.s3.inlineinfo;
					show_inline_info(jd, ii, opcode, stage);
				}
		#endif
		#endif
		//		break;
			case ICMD_INVOKESPECIAL:
				{
					assert(INSTRUCTION_MUST_CHECK(iptr));

					s4 receiver_index = *(iptr->sx.s23.s2.args);
					Value *receiver = read_variable(receiver_index,bbindex);
					CHECKNULLInst *check_null = new CHECKNULLInst(receiver);
					M->add_Instruction(check_null);
					write_variable(global_state, bbindex, check_null);
				}
				// fall through
			case ICMD_INVOKEVIRTUAL:
			case ICMD_INVOKEINTERFACE:
			case ICMD_INVOKESTATIC:
			case ICMD_BUILTIN:
				{
					if (!INSTRUCTION_IS_RESOLVED(iptr)) {
						deoptimize(bbindex);
						break;
					}

					methoddesc *md;
					constant_FMIref *fmiref;
					
					if (iptr->opc == ICMD_BUILTIN) {
						md = iptr->sx.s23.s3.bte->md;
					} else {
						INSTRUCTION_GET_METHODREF(iptr, fmiref);
						md = fmiref->parseddesc.md;
					}
					
					// Determine the return type of the invocation.
					Type::TypeID type;
					switch (md->returntype.type) {
					case TYPE_INT:
						type = Type::IntTypeID;
						break;
					case TYPE_LNG:
						type = Type::LongTypeID;
						break;
					case TYPE_FLT:
						type = Type::FloatTypeID;
						break;
					case TYPE_DBL:
						type = Type::DoubleTypeID;
						break;
					case TYPE_VOID:
						type = Type::VoidTypeID;
						break;
					case TYPE_ADR:
						type = Type::ReferenceTypeID;
						break;
					case TYPE_RET:
						// TODO Implement me.
					default:
						type = Type::VoidTypeID;
						err() << BoldRed << "error: " << reset_color << " type " << BoldWhite
							  << md->returntype.type << reset_color
							  << " not yet supported! (see vm/global.h)" << nl;
						assert(false);
					}

					Instruction *state_change = read_variable(global_state,bbindex)->to_Instruction();
					assert(state_change);

					// Create the actual instruction for the invocation.
					INVOKEInst *I = NULL;
					int32_t argcount = iptr->s1.argcount;
					switch (iptr->opc) {
					case ICMD_INVOKESPECIAL:
						I = new INVOKESPECIALInst(type,argcount,fmiref,BB[bbindex],state_change);
						break;
					case ICMD_INVOKEVIRTUAL:
						I = new INVOKEVIRTUALInst(type,argcount,fmiref,BB[bbindex],state_change);
						break;
					case ICMD_INVOKESTATIC:
						I = new INVOKESTATICInst(type,argcount,fmiref,BB[bbindex],state_change);
						break;
					case ICMD_INVOKEINTERFACE:
						I = new INVOKEINTERFACEInst(type,argcount,fmiref,BB[bbindex],state_change);
						break;
					case ICMD_BUILTIN:
						{
							builtintable_entry *bte = iptr->sx.s23.s3.bte;
							u1 *builtin_address = bte->stub == NULL ? reinterpret_cast<u1*>(bte->fp) : bte->stub;
							I = new BUILTINInst(type,builtin_address,argcount,BB[bbindex],state_change);
						}
						break;
					default:
						assert(false);
						break;
					}

					// Establish data dependencies to the arguments of the invocation.
					s4 *args = iptr->sx.s23.s2.args;
					for (int i = 0; i < argcount; i++) {
						// TODO understand
						//if ((iptr->s1.argcount - 1 - i) == md->paramcount)
						//	printf(" pass-through: ");
						I->append_parameter(read_variable(args[i],bbindex));
					}

					if (type != Type::VoidTypeID) {
						write_variable(iptr->dst.varindex,bbindex,I);
					}

					write_variable(global_state,bbindex,I);
					M->add_Instruction(I);

					LOG3("INVOKEInst: " << I << " dep = " << state_change << nl);
				}
				break;
			case ICMD_IFLT:
			case ICMD_IFGT:
			case ICMD_IFGE:
			case ICMD_IFEQ:
			case ICMD_IFNE:
			case ICMD_IFLE:
				{
					Conditional::CondID cond;
					switch (iptr->opc) {
					case ICMD_IFLT:
						cond = Conditional::LT;
						break;
					case ICMD_IFGT:
						cond = Conditional::GT;
						break;
					case ICMD_IFEQ:
						cond = Conditional::EQ;
						break;
					case ICMD_IFNE:
						cond = Conditional::NE;
						break;
					case ICMD_IFLE:
						cond = Conditional::LE;
						break;
					case ICMD_IFGE:
						cond = Conditional::GE;
						break;
					default:
						os::shouldnotreach();
						cond = Conditional::NoCond;
					}
					Instruction *konst = new CONSTInst(iptr->sx.val.i,Type::IntType());
					Value *s1 = read_variable(iptr->s1.varindex,bbindex);
					assert(s1);

					if (s1->get_type() != konst->get_type()) {
						deoptimize(bbindex);
						break;
					}

					assert(BB[iptr->dst.block->nr]);
					BeginInst *trueBlock = BB[iptr->dst.block->nr]->to_BeginInst();
					assert(trueBlock);
					assert(BB[bbindex+1]);
					BeginInst *falseBlock = BB[bbindex+1]->to_BeginInst();
					assert(falseBlock);
					Instruction *result = new IFInst(BB[bbindex], s1, konst, cond,
						trueBlock, falseBlock);
					M->add_Instruction(konst);
					M->add_Instruction(result);
				}
				break;
		//		SHOW_S1(OS, iptr);
		//		SHOW_INT_CONST(OS, iptr->sx.val.i);	
		//		SHOW_TARGET(OS, iptr->dst);
		//		break;

			case ICMD_IF_LLT:
			case ICMD_IF_LGT:
			case ICMD_IF_LLE:
			case ICMD_IF_LEQ:
			case ICMD_IF_LNE:
			case ICMD_IF_LGE:
				{
					Conditional::CondID cond;
					switch (iptr->opc) {
					case ICMD_IF_LLT:
						cond = Conditional::LT;
						break;
					case ICMD_IF_LGT:
						cond = Conditional::GT;
						break;
					case ICMD_IF_LLE:
						cond = Conditional::LE;
						break;
					case ICMD_IF_LEQ:
						cond = Conditional::EQ;
						break;
					case ICMD_IF_LNE:
						cond = Conditional::NE;
						break;
					case ICMD_IF_LGE:
						cond = Conditional::GE;
						break;
					default:
						os::shouldnotreach();
						cond = Conditional::NoCond;
					}
					Instruction *konst = new CONSTInst(iptr->sx.val.l,Type::LongType());
					Value *s1 = read_variable(iptr->s1.varindex,bbindex);
					assert(s1);

					if (s1->get_type() != konst->get_type()) {
						deoptimize(bbindex);
						break;
					}

					assert(BB[iptr->dst.block->nr]);
					BeginInst *trueBlock = BB[iptr->dst.block->nr]->to_BeginInst();
					assert(trueBlock);
					assert(BB[bbindex+1]);
					BeginInst *falseBlock = BB[bbindex+1]->to_BeginInst();
					assert(falseBlock);
					Instruction *result = new IFInst(BB[bbindex], s1, konst, cond,
						trueBlock, falseBlock);
					M->add_Instruction(konst);
					M->add_Instruction(result);
				}
				break;
			case ICMD_GOTO:
				{
					assert(BB[iptr->dst.block->nr]);
					BeginInst *targetBlock = BB[iptr->dst.block->nr];
					Instruction *result = new GOTOInst(BB[bbindex], targetBlock);
					M->add_Instruction(result);
				}
				break;
		//		SHOW_TARGET(OS, iptr->dst);
		//		break;

			case ICMD_JSR:
		//		SHOW_TARGET(OS, iptr->sx.s23.s3.jsrtarget);
		//		SHOW_DST(OS, iptr);
		//		break;

			case ICMD_IFNULL:
			case ICMD_IFNONNULL:
		//		SHOW_S1(OS, iptr);
		//		SHOW_TARGET(OS, iptr->dst);
		//		break;
				{
					deoptimize(bbindex);
					break;
				}
			case ICMD_IF_ICMPEQ:
			case ICMD_IF_ICMPNE:
			case ICMD_IF_ICMPLT:
			case ICMD_IF_ICMPGE:
			case ICMD_IF_ICMPGT:
			case ICMD_IF_ICMPLE:
				{
					Conditional::CondID cond;
					switch (iptr->opc) {
					case ICMD_IF_ICMPLE:
						cond = Conditional::LE;
						break;
					case ICMD_IF_ICMPEQ:
						cond = Conditional::EQ;
						break;
					case ICMD_IF_ICMPNE:
						cond = Conditional::NE;
						break;
					case ICMD_IF_ICMPLT:
						cond = Conditional::LT;
						break;
					case ICMD_IF_ICMPGE:
						cond = Conditional::GE;
						break;
					case ICMD_IF_ICMPGT:
						cond = Conditional::GT;
						break;
					default:
						os::shouldnotreach();
						cond = Conditional::NoCond;
					}
					Value *s1 = read_variable(iptr->s1.varindex,bbindex);
					Value *s2 = read_variable(iptr->sx.s23.s2.varindex,bbindex);
					assert(s1);
					assert(s2);

					if (s1->get_type() != s2->get_type()) {
						deoptimize(bbindex);
						break;
					}

					assert(BB[iptr->dst.block->nr]);
					BeginInst *trueBlock = BB[iptr->dst.block->nr]->to_BeginInst();
					assert(trueBlock);
					assert(BB[bbindex+1]);
					BeginInst *falseBlock = BB[bbindex+1]->to_BeginInst();
					assert(falseBlock);
					Instruction *result = new IFInst(BB[bbindex], s1, s2, cond,
						trueBlock, falseBlock);
					M->add_Instruction(result);
				}
				break;

			case ICMD_IF_LCMPEQ:
			case ICMD_IF_LCMPNE:
			case ICMD_IF_LCMPLT:
			case ICMD_IF_LCMPGT:
			case ICMD_IF_LCMPGE:
			case ICMD_IF_LCMPLE:
				{
					Conditional::CondID cond;
					switch (iptr->opc) {
					case ICMD_IF_LCMPEQ:
						cond = Conditional::EQ;
						break;
					case ICMD_IF_LCMPNE:
						cond = Conditional::NE;
						break;
					case ICMD_IF_LCMPLT:
						cond = Conditional::LT;
						break;
					case ICMD_IF_LCMPGT:
						cond = Conditional::GT;
						break;
					case ICMD_IF_LCMPGE:
						cond = Conditional::GE;
						break;
					case ICMD_IF_LCMPLE:
						cond = Conditional::LE;
						break;
					default:
						cond = Conditional::NoCond;
						os::shouldnotreach();
					}
					Value *s1 = read_variable(iptr->s1.varindex,bbindex);
					Value *s2 = read_variable(iptr->sx.s23.s2.varindex,bbindex);
					assert(s1);
					assert(s2);

					if (s1->get_type() != s2->get_type()) {
						deoptimize(bbindex);
						break;
					}

					assert(BB[iptr->dst.block->nr]);
					BeginInst *trueBlock = BB[iptr->dst.block->nr]->to_BeginInst();
					assert(trueBlock);
					assert(BB[bbindex+1]);
					BeginInst *falseBlock = BB[bbindex+1]->to_BeginInst();
					assert(falseBlock);
					Instruction *result = new IFInst(BB[bbindex], s1, s2, cond,
						trueBlock, falseBlock);
					M->add_Instruction(result);
				}
				break;

			case ICMD_IF_ACMPEQ:
			case ICMD_IF_ACMPNE:
		//		SHOW_S1(OS, iptr);
		//		SHOW_S2(OS, iptr);
		//		SHOW_TARGET(OS, iptr->dst);
		//		break;
				{
					deoptimize(bbindex);
					break;
				}
			case ICMD_TABLESWITCH:
				{
					s4 tablehigh = iptr->sx.s23.s3.tablehigh;
					s4 tablelow  = iptr->sx.s23.s2.tablelow;

					Value *s1 = read_variable(iptr->s1.varindex,bbindex);
					TABLESWITCHInst *result = new TABLESWITCHInst(BB[bbindex], s1,
						TABLESWITCHInst::LOW(tablelow),
						TABLESWITCHInst::HIGH(tablehigh));

					s4 count = tablehigh - tablelow + 1;
					LOG("tableswitch  high=" << tablehigh << " low=" << tablelow << " count=" << count << nl);
					branch_target_t *table = iptr->dst.table;
					BeginInst* def = BB[table[0].block->nr];
					LOG("idx: " << 0 << " BeginInst: " << BB[table[0].block->nr]
						<< "(block.nr=" << table[0].block->nr << ")"<< nl);
					++table;
					for (s4 i = 0; i < count ; ++i) {
						LOG("idx: " << i << " BeginInst: " << BB[table[i].block->nr]
							<< "(block.nr=" << table[i].block->nr << ")"<< nl);
						result->append_succ(BB[table[i].block->nr]);
					}
					result->append_succ(def);
					M->add_Instruction(result);
				}
				break;

			case ICMD_LOOKUPSWITCH:
				{
					s4 lookupcount = iptr->sx.s23.s2.lookupcount;

					Value *s1 = read_variable(iptr->s1.varindex,bbindex);
					LOOKUPSWITCHInst *result = new LOOKUPSWITCHInst(BB[bbindex], s1, lookupcount);

					// add case targets
					lookup_target_t *lookup = iptr->dst.lookup;
					for (s4 i = 0; i < lookupcount; ++i) {
						// lookup->value
						result->append_succ(BB[lookup->target.block->nr]);
						result->set_match(i, LOOKUPSWITCHInst::MATCH(lookup->value));
						++lookup;
					}
					// add default target
					BeginInst *defaultBlock = BB[iptr->sx.s23.s3.lookupdefault.block->nr];
					assert(defaultBlock);
					result->append_succ(defaultBlock);

					M->add_Instruction(result);
				}
				break;
		//		SHOW_S1(OS, iptr);
		//
		//		printf("count=%d, default=L%03d\n",
		//			   iptr->sx.s23.s2.lookupcount,
		//			   iptr->sx.s23.s3.lookupdefault.block->nr);
		//
		//		lookup = iptr->dst.lookup;
		//		i = iptr->sx.s23.s2.lookupcount;
		//
		//		while (--i >= 0) {
		//			printf("\t\t%d --> L%03d\n",
		//				   lookup->value,
		//				   lookup->target.block->nr);
		//			lookup++;
		//		}
		//		break;

			case ICMD_FRETURN:
			case ICMD_IRETURN:
			case ICMD_DRETURN:
			case ICMD_LRETURN:
			case ICMD_ARETURN:
				{
					Value *s1 = read_variable(iptr->s1.varindex,bbindex);
					Instruction *result = new RETURNInst(BB[bbindex], s1);
					M->add_Instruction(result);
				}
				break;
		//		SHOW_S1(OS, iptr);
		//		break;
		//
			case ICMD_RETURN:
				{
					Instruction *result = new RETURNInst(BB[bbindex]);
					M->add_Instruction(result);
				}
				break;
			case ICMD_ATHROW:
		//		SHOW_S1(OS, iptr);
		//		if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
		//			/* XXX this needs more work */
		#if 0
					unresolved_class_debug_dump(iptr->sx.s23.s2.uc, stdout);
		#endif
		//		}
		//		break;
				{
					deoptimize(bbindex);
					break;
				}
			case ICMD_COPY:
			case ICMD_MOVE:
				{
					Value *s1 = read_variable(iptr->s1.varindex, bbindex);
					write_variable(iptr->dst.varindex,bbindex,s1);
				}
				break;
		//		SHOW_S1(OS, iptr);
		//		SHOW_DST(OS, iptr);
		//		break;
			case ICMD_GETEXCEPTION:
		//		SHOW_DST(OS, iptr);
		//		break;
				{
					deoptimize(bbindex);
					break;
				}
		#if defined(ENABLE_SSA)	
			case ICMD_PHI:
		//		printf("[ ");
		//		for (i = 0; i < iptr->s1.argcount; ++i) {
		//			SHOW_VARIABLE(OS, iptr->sx.s23.s2.iargs[i]->dst.varindex);
		//		}
		//		printf("] ");
		//		SHOW_DST(OS, iptr);
		//		if (iptr->flags.bits & (1 << 0)) OS << "used ";
		//		if (iptr->flags.bits & (1 << 1)) OS << "redundantAll ";
		//		if (iptr->flags.bits & (1 << 2)) OS << "redundantOne ";
		//		break;
				{
					deoptimize(bbindex);
					break;
				}
		#endif
			default:
				#if !defined(NDEBUG)
				ABORT_MSG(icmd_table[iptr->opc].name << " (" << iptr->opc << ")",
					"Operation not yet supported!");
				#else
				ABORT_MSG("Opcode: (" << iptr->opc << ")",
					"Operation not yet supported!");
				#endif
				break;
			}

			// Record the source state if the last instruction was side-effecting.
			new_global_state = read_variable(global_state,bbindex)->to_Instruction();
			if (new_global_state != old_global_state && !new_global_state->to_BeginInst()) {
				assert(instruction_has_side_effects(iptr));
				record_source_state(new_global_state, iptr, bb, live_javalocals,
						iptr->stack_after, iptr->stackdepth_after);
			}
		}

		if (!BB[bbindex]->get_EndInst()) {
			// No end instruction yet. Adding GOTO
			assert(bbindex+1 < BB.size());
			BeginInst *targetBlock = BB[bbindex+1];
			Instruction *result = new GOTOInst(BB[bbindex], targetBlock);
			M->add_Instruction(result);
		}

		// block filled!
		filled_blocks[bbindex] = true;
		// try to seal block
		try_seal_block(bb);
		// try seal successors
		for(int i = 0; i < bb->successorcount; ++i) {
			try_seal_block(bb->successors[i]);
		}
	}

	#ifndef NDEBUG
	for(size_t i = 0; i< num_basicblocks; ++i) {
		if (!sealed_blocks[i]) {
			err() << BoldRed << "error: " << reset_color << "unsealed basic block: " << BoldWhite
				  << i << " (" << BB[i] << ")"
				  << reset_color << nl;
			assert(0 && "There is an unsealed basic block");
		}
	}
	#endif

	remove_unreachable_blocks();

	return true;
}

bool SSAConstructionPass::verify() const {
	for (Method::const_iterator i = M->begin(), e = M->end() ; i != e ; ++i) {
		Instruction *I = *i;
		if (!I->verify()) return false;
	}
	return true;
}

PassUsage& SSAConstructionPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires<CFGConstructionPass>();
	return PU;
}
// the address of this variable is used to identify the pass
char SSAConstructionPass::ID = 0;

// registrate Pass
static PassRegistry<SSAConstructionPass> X("SSAConstructionPass");

} // end namespace cacao
} // end namespace jit
} // end namespace compiler2


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
