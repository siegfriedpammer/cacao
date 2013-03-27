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

#include "vm/jit/jit.hpp"
#include "vm/jit/show.hpp"
#include "vm/jit/ir/icmd.hpp"

#include "toolbox/logging.hpp"

namespace {
const char * get_var_type(int type)
{
	switch(type) {
	case TYPE_INT: return "TYPE_INT";
	case TYPE_LNG: return "TYPE_LNG";
	case TYPE_FLT: return "TYPE_FLT";
	case TYPE_DBL: return "TYPE_DBL";
	case TYPE_ADR: return "TYPE_ADR";
	case TYPE_RET: return "TYPE_RET";
	case TYPE_VOID: return "TYPE_VOID";
	}
	return "(unknown type)";
}
} // end anonymous namespace

namespace cacao {

namespace {

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
		if (stage >= SHOW_STACK && iptr->sx.s23.s3.javaindex != UNUSED)
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

#define DEBUG_NAME "compiler2/ssa"

void SSAConstructionPass::write_variable(size_t varindex, size_t bb, Value* v) {
	current_def[varindex][bb] = v;
}

Value* SSAConstructionPass::read_variable(size_t varindex, size_t bb) const {
	Value* v = current_def[varindex][bb];
	return v;
}

bool SSAConstructionPass::run(JITData &JD) {
	Method &M = *(JD.get_Method());

	basicblock *bb;
	jitdata *jd = JD.jitdata();

	// Local Value Numbering Map, size #bb times #var, initialized to NULL
	current_def.resize(jd->vartop,std::vector<Value*>(jd->basicblockcount,NULL));

	show_method(jd, SHOW_CFG);

	for(size_t i = 0, e = jd->vartop; i != e ; ++i) {
		varinfo &v = jd->var[i];
		LOG("var#" << i << " type: " << get_var_type(v.type) << nl);
	}
	LOG("# variables: " << jd->vartop << nl);

	FOR_EACH_BASICBLOCK(jd,bb) {
		instruction *iptr;
		LOG("basicblock: " << bb->nr << nl);
		FOR_EACH_INSTRUCTION(bb,iptr) {
			LOG("iptr: " << icmd_table[iptr->opc].name << nl);
			switch (iptr->opc) {
			case ICMD_NOP:
				//M.add_instruction(new NOPInst());
				break;
			case ICMD_POP:
			case ICMD_CHECKNULL:
		//		SHOW_S1(OS, iptr);
		//		break;

		//		/* unary */
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
		//		SHOW_S1(OS, iptr);
		//		SHOW_DST(OS, iptr);
		//		break;

				/* binary */
			case ICMD_IADD:
				goto _default;
			case ICMD_LADD:
				{
					Value *s1 = read_variable(iptr->s1.varindex, bb->nr);
					Value *s2 = read_variable(iptr->sx.s23.s2.varindex,bb->nr);
					Instruction *result = new ADDInst(Type::LongTypeID, s1, s2);
					write_variable(iptr->dst.varindex,bb->nr,result);
					M.add_instruction(result);
				}
				break;
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
		//		SHOW_S1(OS, ipt);
		//		SHOW_S2(OS, iptr);
		//		SHOW_DST(OS, iptr);
		//		break;

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
		//		SHOW_S1(OS, iptr);
		//		SHOW_INT_CONST(OS, iptr->sx.val.i);	
		//		SHOW_DST(OS, iptr);
		//		break;

				/* ?ASTORECONST (trinary/const INT) */
			case ICMD_IASTORECONST:
			case ICMD_BASTORECONST:
			case ICMD_CASTORECONST:
			case ICMD_SASTORECONST:
		//		SHOW_S1(OS, iptr);
		//		SHOW_S2(OS, iptr);
		//		SHOW_INT_CONST(OS, iptr->sx.s23.s3.constval);
		//		break;

				/* const INT */
			case ICMD_ICONST:
		//		SHOW_INT_CONST(OS, iptr->sx.val.i);	
		//		SHOW_DST(OS, iptr);
		//		break;

				/* binary/const LNG */
			case ICMD_LADDCONST:
				goto _default;
			case ICMD_LSUBCONST:
				{
					Instruction *konst = new CONSTInst(iptr->sx.val.l);
					Value *s1 = read_variable(iptr->s1.varindex,bb->nr);
					Instruction *result = new SUBInst(Type::LongTypeID, s1, konst);
					M.add_instruction(konst);
					M.add_instruction(result);
				}
				break;
			case ICMD_LMULCONST:
			case ICMD_LMULPOW2:
			case ICMD_LDIVPOW2:
			case ICMD_LREMPOW2:
			case ICMD_LANDCONST:
			case ICMD_LORCONST:
			case ICMD_LXORCONST:
		//		SHOW_S1(OS, iptr);
		//		SHOW_LNG_CONST(OS, iptr->sx.val.l);
		//		SHOW_DST(OS, iptr);
		//		break;

				/* trinary/const LNG (<= pointer size) */
			case ICMD_LASTORECONST:
		//		SHOW_S1(OS, iptr);
		//		SHOW_S2(OS, iptr);
		//		SHOW_ADR_CONST(OS, iptr->sx.s23.s3.constval);
		//		break;

				/* const LNG */
				goto _default;
			case ICMD_LCONST:
				{
					Instruction *I = new CONSTInst(iptr->sx.val.l);
					write_variable(iptr->dst.varindex,bb->nr,I);
					M.add_instruction(I);
				}
				break;
		//		SHOW_LNG_CONST(OS, iptr->sx.val.l);	
		//		SHOW_DST(OS, iptr);
		//		break;

				/* const FLT */
			case ICMD_FCONST:
		//		SHOW_FLT_CONST(OS, iptr->sx.val.f);	
		//		SHOW_DST(OS, iptr);
		//		break;

				/* const DBL */
			case ICMD_DCONST:
		//		SHOW_DBL_CONST(OS, iptr->sx.val.d);	
		//		SHOW_DST(OS, iptr);
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

			case ICMD_AASTORECONST:
		//		SHOW_S1(OS, iptr);
		//		SHOW_S2(OS, iptr);
		//		printf("%p ", (void*) iptr->sx.s23.s3.constval);
		//		break;

			case ICMD_GETFIELD:        /* 1 -> 1 */
			case ICMD_PUTFIELD:        /* 2 -> 0 */
			case ICMD_PUTSTATIC:       /* 1 -> 0 */
			case ICMD_GETSTATIC:       /* 0 -> 1 */
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

			case ICMD_IINC:
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
		//		SHOW_S1(OS, iptr);
		//		SHOW_S2(OS, iptr);
		//		SHOW_S3(OS, iptr);
		//		break;

			case ICMD_IALOAD:
			case ICMD_SALOAD:
			case ICMD_BALOAD:
			case ICMD_CALOAD:
			case ICMD_LALOAD:
			case ICMD_DALOAD:
			case ICMD_FALOAD:
			case ICMD_AALOAD:
		//		SHOW_S1(OS, iptr);
		//		SHOW_S2(OS, iptr);
		//		SHOW_DST(OS, iptr);
		//		break;

			case ICMD_RET:
		//		SHOW_S1_LOCAL(OS, iptr);
		//		if (stage >= SHOW_STACK) {
		//			printf(" ---> L%03d", iptr->dst.block->nr);
		//		}
		//		break;
				goto _default;

			case ICMD_ILOAD:
				{
					Instruction *I = new LOADInst(Type::IntTypeID, iptr->s1.varindex);
					write_variable(iptr->dst.varindex,bb->nr,I);
					M.add_instruction(I);
				}
				break;
			case ICMD_LLOAD:
				{
					Instruction *I = new LOADInst(Type::LongTypeID, iptr->s1.varindex);
					write_variable(iptr->dst.varindex,bb->nr,I);
					M.add_instruction(I);
				}
				break;
			case ICMD_FLOAD:
				{
					Instruction *I = new LOADInst(Type::FloatTypeID, iptr->s1.varindex);
					write_variable(iptr->dst.varindex,bb->nr,I);
					M.add_instruction(I);
				}
				break;
			case ICMD_DLOAD:
				{
					Instruction *I = new LOADInst(Type::DoubleTypeID, iptr->s1.varindex);
					write_variable(iptr->dst.varindex,bb->nr,I);
					M.add_instruction(I);
				}
				break;
			case ICMD_ALOAD:
		//		SHOW_S1_LOCAL(OS, iptr);
		//		SHOW_DST(OS, iptr);
		//		break;

			case ICMD_ISTORE:
			case ICMD_LSTORE:
			case ICMD_FSTORE:
			case ICMD_DSTORE:
			case ICMD_ASTORE:
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

			case ICMD_BUILTIN:
		//		if (stage >= SHOW_STACK) {
		//			argp = iptr->sx.s23.s2.args;
		//			i = iptr->s1.argcount;
		//			while (i--) {
		//				if ((iptr->s1.argcount - 1 - i) == iptr->sx.s23.s3.bte->md->paramcount)
		//					printf(" pass-through: ");
		//				SHOW_VARIABLE(OS, *(argp++));
		//			}
		//		}
		//		printf("%s ", iptr->sx.s23.s3.bte->cname);
		//		if (iptr->sx.s23.s3.bte->md->returntype.type != TYPE_VOID) {
		//			SHOW_DST(OS, iptr);
		//		}
		//		break;

			case ICMD_INVOKEVIRTUAL:
			case ICMD_INVOKESPECIAL:
				goto _default;
			case ICMD_INVOKESTATIC:
				{
					methoddesc *md;
					constant_FMIref   *fmiref;
					INSTRUCTION_GET_METHODDESC(iptr, md);
					INSTRUCTION_GET_METHODREF(iptr, fmiref);

					// get return type
					Type::TypeID type;
					switch (fmiref->parseddesc.md->returntype.type) {
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
						//type = Type::TypeID;
						//break;
					case TYPE_RET:
						//type = Type::TypeID;
						//break;
					default:
						err() << BoldRed << "error: " << reset_color << " type " << BoldWhite
							  << fmiref->parseddesc.md->returntype.type << reset_color
							  << " not yet supported! (see vm/global.h)" << nl;
						assert(false);
					}
					// create instruction
					INVOKESTATICInst *I = new INVOKESTATICInst(type);
					// get arguments
					s4 *argp = iptr->sx.s23.s2.args;
					int32_t i = iptr->s1.argcount;
					while (i--) {
						// TODO understand
						//if ((iptr->s1.argcount - 1 - i) == md->paramcount)
						//	printf(" pass-through: ");
						I->add_operand(current_def[bb->nr][*(argp++)]);
					}

					M.add_instruction(I);
				}
				break;
			case ICMD_INVOKEINTERFACE:
		//		if (stage >= SHOW_STACK) {
		//			methoddesc *md;
		//			INSTRUCTION_GET_METHODDESC(iptr, md);
		//			argp = iptr->sx.s23.s2.args;
		//			i = iptr->s1.argcount;
		//			while (i--) {
		//				if ((iptr->s1.argcount - 1 - i) == md->paramcount)
		//					printf(" pass-through: ");
		//				SHOW_VARIABLE(OS, *(argp++));
		//			}
		//		}
		//		INSTRUCTION_GET_METHODREF(iptr, fmiref);
		//		method_methodref_print(fmiref);
		//		if (fmiref->parseddesc.md->returntype.type != TYPE_VOID) {
		//			putchar(' ');
		//			SHOW_DST(OS, iptr);
		//		}
		//		break;

			case ICMD_IFEQ:
			case ICMD_IFNE:
			case ICMD_IFLT:
			case ICMD_IFGE:
			case ICMD_IFGT:
			case ICMD_IFLE:
		//		SHOW_S1(OS, iptr);
		//		SHOW_INT_CONST(OS, iptr->sx.val.i);	
		//		SHOW_TARGET(OS, iptr->dst);
		//		break;

			case ICMD_IF_LEQ:
				goto _default;
			case ICMD_IF_LNE:
				{
					Instruction *konst = new CONSTInst(iptr->sx.val.l);
					Value *s1 = read_variable(iptr->s1.varindex,bb->nr);
					Instruction *result = new IFInst(Type::LongTypeID, s1, konst, Conditional::NE);
					M.add_instruction(konst);
					M.add_instruction(result);
				}
				break;
			case ICMD_IF_LLT:
				goto _default;
			case ICMD_IF_LGE:
				{
					Instruction *konst = new CONSTInst(iptr->sx.val.l);
					Value *s1 = read_variable(iptr->s1.varindex,bb->nr);
					assert(s1);
					Instruction *result = new IFInst(Type::LongTypeID, s1, konst, Conditional::GE);
					M.add_instruction(konst);
					M.add_instruction(result);
				}
				break;
			case ICMD_IF_LGT:
			case ICMD_IF_LLE:
		//		SHOW_S1(OS, iptr);
		//		SHOW_LNG_CONST(OS, iptr->sx.val.l);	
		//		SHOW_TARGET(OS, iptr->dst);
		//		break;
		//
			case ICMD_GOTO:
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
		//		SHOW_S1(OS, iptr);
		//		SHOW_S2(OS, iptr);
		//		SHOW_TARGET(OS, iptr->dst);
		//		break;

			case ICMD_TABLESWITCH:
		//		SHOW_S1(OS, iptr);
		//		table = iptr->dst.table;
		//
		//		i = iptr->sx.s23.s3.tablehigh - iptr->sx.s23.s2.tablelow + 1;
		//
		//		printf("high=%d low=%d count=%d\n", iptr->sx.s23.s3.tablehigh, iptr->sx.s23.s2.tablelow, i);
		//		while (--i >= 0) {
		//			printf("\t\t%d --> ", (int) (table - iptr->dst.table));
		//			printf("L%03d\n", table->block->nr);
		//			table++;
		//		}
		//
		//		break;

			case ICMD_LOOKUPSWITCH:
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
				{
					Value *s1 = read_variable(iptr->s1.varindex,bb->nr);
					Instruction *result = new RETURNInst(Type::LongTypeID, s1);
					M.add_instruction(result);
				}
				break;
		//		SHOW_S1(OS, iptr);
		//		break;
		//
			case ICMD_ARETURN:
			case ICMD_ATHROW:
		//		SHOW_S1(OS, iptr);
		//		if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
		//			/* XXX this needs more work */
		#if 0
					unresolved_class_debug_dump(iptr->sx.s23.s2.uc, stdout);
		#endif
		//		}
		//		break;

			case ICMD_COPY:
			case ICMD_MOVE:
		//		SHOW_S1(OS, iptr);
		//		SHOW_DST(OS, iptr);
		//		break;
			case ICMD_GETEXCEPTION:
		//		SHOW_DST(OS, iptr);
		//		break;
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
		#endif
			default:
				goto _default;
			}
			continue;

			_default:
				err() << BoldRed << "error: " << reset_color << "operation " << BoldWhite
					  << icmd_table[iptr->opc].name << reset_color << " not yet supported!" << nl;
				assert(false);
		}
	}
	return true;
}

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
