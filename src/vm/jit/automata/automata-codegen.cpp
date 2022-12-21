#include "vm/descriptor.hpp"
#include "vm/field.hpp"

#include "automata.hpp"
#include "codegen.hpp"
#include "vm/jit/code.hpp"
#include "vm/jit/codegen-common.hpp"

#include "vm/jit/builtin.hpp"
#include "vm/jit/dseg.hpp"
#include "vm/jit/disass.hpp"
#include "vm/jit/exceptiontable.hpp"
#include "vm/jit/emit-common.hpp"
#include "vm/jit/jit.hpp"
#include "vm/jit/linenumbertable.hpp"
#include "vm/jit/ir/instruction.hpp"
#include "vm/jit/patcher-common.hpp"

#include "vm/jit/optimizing/profile.hpp"

#include "automata-codegen.hpp"

void gen_method(struct jitdata *jd, instruction* iptr, builtintable_entry* bte, methoddesc *md);

extern "C" void codegen_emit_throw(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);
	codegendata*  cd   = jd->cd;
    int32_t s1;
    // We might leave this method, stop profiling.
    PROFILE_CYCLE_STOP;

    s1 = emit_load_s1(jd, iptr, REG_ITMP1);
    // XXX Sparc64: We use REG_ITMP2_XPTR here, fix me!
    emit_imove(cd, s1, REG_ITMP1_XPTR);

#ifdef ENABLE_VERIFIER
    if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
        unresolved_class *uc = iptr->sx.s23.s2.uc;
        patcher_add_patch_ref(jd, PATCHER_resolve_class, uc, 0);
    }
#endif /* ENABLE_VERIFIER */

    // Generate architecture specific instructions.
    codegen_emit_instruction(jd, iptr);
    ALIGNCODENOP;
}

extern "C" void codegen_emit_arraylength(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);
    log("emitting arraylength\n");
	codegendata*  cd   = jd->cd;
    int32_t s1 = emit_load_s1(jd, iptr, REG_ITMP1);
    int32_t d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
    /* implicit null-pointer check */
    // XXX PPC64: Here we had an explicit null-pointer check
    //     which I think was obsolete, please confirm. Otherwise:
    // emit_nullpointer_check(cd, iptr, s1);
    M_ILD(d, s1, OFFSET(java_array_t, size));
    emit_store_dst(jd, iptr, d);
}

extern "C" void codegen_emit_inline_end(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);
    log("emitting inline_end\n");
	codegendata*  cd   = jd->cd;
    linenumbertable_list_entry_add_inline_end(cd, iptr);
    linenumbertable_list_entry_add(cd, iptr->line);
}

extern "C" void codegen_emit_simple_instruction(struct jitdata *jd, struct instruction *iptr) {
    log("%s in codegen_emit_simple_instruction\n", burm_opname[iptr->opc]);
    codegen_emit_instruction(jd, iptr);
}

extern "C" void codegen_emit_combined_instruction(struct jitdata *jd, struct instruction *iptr) {
    /*instruction *input1 = PREV_LEFT(bnode);
    log("%s(%s)\n", burm_opname[iptr->opc], burm_opname[input1->opc]);
    int32_t s1, d;
    struct jitdata *jd = bnode->jd;
    codegendata*  cd   = jd->cd;
    switch (iptr->opc) {
        case ICMD_IADD:
            assert(input1->opc == ICMD_ICONST);
            s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);

			M_INTMOVE(s1, d);
			M_IADD_IMM(input1->sx.val.i, d);
			emit_store_dst(jd, iptr, d);
            break;
        case ICMD_LADD:
            assert(input1->opc == ICMD_LCONST);
        	s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
			M_INTMOVE(s1, d);
			if (IS_IMM32(input1->sx.val.l))
				M_LADD_IMM(input1->sx.val.l, d);
			else {
				M_MOV_IMM(input1->sx.val.l, REG_ITMP2);
				M_LADD(REG_ITMP2, d);
			}
			emit_store_dst(jd, iptr, d);
            break;
    }
    #if AUTOMATON_KIND_FSM
    bnode->prev = NULL;
    #endif*/
}

extern "C" void codegen_emit_iadd(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);
    int32_t s1, s2, d;
    codegendata*  cd   = jd->cd;
    s1 = emit_load_s1(jd, iptr, REG_ITMP1);
    s2 = emit_load_s2(jd, iptr, REG_ITMP2);
    d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
    if (s2 == d)
        M_IADD(s1, d);
    else {
        M_INTMOVE(s1, d);
        M_IADD(s2, d);
    }
    emit_store_dst(jd, iptr, d);
}

extern "C" void codegen_emit_iaddconst(struct jitdata *jd, struct instruction *iptr, struct instruction *constant) {
    log("codegen_emit_iaddconst: %s(%s)\n", burm_opname[iptr->opc], burm_opname[constant->opc]);
    assert(iptr->opc == ICMD_IADD);
    assert(constant->opc == ICMD_ICONST);
    int32_t s1, d;
    codegendata*  cd   = jd->cd;
    s1 = emit_load_s1(jd, iptr, REG_ITMP1);
    d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);

    M_INTMOVE(s1, d);
    M_IADD_IMM(constant->sx.val.i, d);
    emit_store_dst(jd, iptr, d);
}

extern "C" void codegen_emit_iaddconst_right(struct jitdata *jd, struct instruction *iptr, struct instruction *constant) {
    log("codegen_emit_iaddconst_right: %s(%s)\n", burm_opname[iptr->opc], burm_opname[constant->opc]);
    assert(iptr->opc == ICMD_IADD);
    assert(constant->opc == ICMD_ICONST);
    int32_t s1, d;
    codegendata*  cd   = jd->cd;
    s1 = emit_load_s1(jd, iptr, REG_ITMP1);
    d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);

    M_INTMOVE(s1, d);
    M_IADD_IMM(constant->sx.val.i, d);
    emit_store_dst(jd, iptr, d);
}

extern "C" void codegen_push_to_reg(struct jitdata *jd, struct instruction *iptr) {
    codegendata*  cd   = jd->cd;
    assert(iptr !=NULL);
    log("push to reg: %d: %s\n", iptr->opc, burm_opname[iptr->opc]);
    assert(iptr->opc == ICMD_LCONST || iptr->opc == ICMD_ICONST);
    int32_t d;
    switch ((int)iptr->opc) {
        case ICMD_LCONST:
            d = codegen_reg_of_dst(jd, iptr, REG_LTMP12);
            LCONST(d, iptr->sx.val.l);
            emit_store_dst(jd, iptr, d);
            break;
        case ICMD_ICONST:
            d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
            ICONST(d, iptr->sx.val.i);
            emit_store_dst(jd, iptr, d);
            break;
    }
}

extern "C" void remember_instruction(struct jitdata *jd, struct instruction *iptr) {

}

extern "C" void codegen_emit_breakpoint(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);
	codegendata*  cd   = jd->cd;
    patcher_add_patch_ref(jd, PATCHER_breakpoint, iptr->sx.val.anyptr, 0);
	PATCHER_NOPS;
}

extern "C" void codegen_emit_checknull(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);
	codegendata*  cd   = jd->cd;
    int32_t s1 = emit_load_s1(jd, iptr, REG_ITMP1);
    emit_nullpointer_check(cd, iptr, s1);
}

extern "C" void codegen_emit_iconst(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);
	codegendata*  cd   = jd->cd;
    int32_t d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
    ICONST(d, iptr->sx.val.i);
    emit_store_dst(jd, iptr, d);
}

extern "C" void codegen_emit_lconst(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);
	codegendata*  cd   = jd->cd;
    int32_t d = codegen_reg_of_dst(jd, iptr, REG_LTMP12);
    LCONST(d, iptr->sx.val.l);
    emit_store_dst(jd, iptr, d);
}

extern "C" void codegen_emit_undef(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);
    assert(false);
}

extern "C" void codegen_emit_astore(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);
    if (!(iptr->flags.bits & INS_FLAG_RETADDR))
		emit_copy(jd, iptr);
}

extern "C" void codegen_emit_branch(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);

	codegendata*  cd   = jd->cd;
    int32_t s1, s2;
    switch (iptr->opc) {
        case ICMD_IFEQ:       /* ..., value ==> ...                       */
        case ICMD_IFNE:
        case ICMD_IFLT:
        case ICMD_IFLE:
        case ICMD_IFGT:
        case ICMD_IFGE:

            // XXX Sparc64: int compares must not branch on the
            // register directly. Reason is, that register content is
            // not 32-bit clean. Fix this!

            assert(iptr->dst.block->type != basicblock::TYPE_EXH);

#if SUPPORT_BRANCH_CONDITIONAL_ONE_INTEGER_REGISTER
            if (iptr->sx.val.i == 0) {
                s1 = emit_load_s1(jd, iptr, REG_ITMP1);
                emit_bccz(cd, iptr->dst.block, iptr->opc - ICMD_IFEQ, s1, BRANCH_OPT_NONE);
            } else {
                // Generate architecture specific instructions.
                codegen_emit_instruction(jd, iptr);
            }
#elif SUPPORT_BRANCH_CONDITIONAL_CONDITION_REGISTER
            s1 = emit_load_s1(jd, iptr, REG_ITMP1);
            emit_icmp_imm(cd, s1, iptr->sx.val.i);
            emit_bcc(cd, iptr->dst.block, iptr->opc - ICMD_IFEQ, BRANCH_OPT_NONE);
#else
# error Unable to generate code for this configuration!
#endif
            break;

        case ICMD_IF_LEQ:     /* ..., value ==> ...                       */
        case ICMD_IF_LNE:
        case ICMD_IF_LLT:
        case ICMD_IF_LGE:
        case ICMD_IF_LGT:
        case ICMD_IF_LLE:

            assert(iptr->dst.block->type != basicblock::TYPE_EXH);

            // Generate architecture specific instructions.
            codegen_emit_instruction(jd, iptr);
            break;

        case ICMD_IF_ACMPEQ:  /* ..., value, value ==> ...                */
        case ICMD_IF_ACMPNE:  /* op1 = target JavaVM pc                   */

            assert(iptr->dst.block->type != basicblock::TYPE_EXH);

            s1 = emit_load_s1(jd, iptr, REG_ITMP1);
            s2 = emit_load_s2(jd, iptr, REG_ITMP2);
#if SUPPORT_BRANCH_CONDITIONAL_TWO_INTEGER_REGISTERS
            switch (iptr->opc) {
                case ICMD_IF_ACMPEQ:
                    emit_beq(cd, iptr->dst.block, s1, s2);
                    break;
                case ICMD_IF_ACMPNE:
                    emit_bne(cd, iptr->dst.block, s1, s2);
                    break;
                default:
                    break;
            }
#elif SUPPORT_BRANCH_CONDITIONAL_CONDITION_REGISTER
            M_ACMP(s1, s2);
            emit_bcc(cd, iptr->dst.block, iptr->opc - ICMD_IF_ACMPEQ, BRANCH_OPT_NONE);
#elif SUPPORT_BRANCH_CONDITIONAL_ONE_INTEGER_REGISTER
            M_CMPEQ(s1, s2, REG_ITMP1);
            switch (iptr->opc) {
                case ICMD_IF_ACMPEQ:
                    emit_bnez(cd, iptr->dst.block, REG_ITMP1);
                    break;
                case ICMD_IF_ACMPNE:
                    emit_beqz(cd, iptr->dst.block, REG_ITMP1);
                    break;
                default:
                    break;
            }
#else
# error Unable to generate code for this configuration!
#endif
            break;

        case ICMD_IF_ICMPEQ:  /* ..., value, value ==> ...                */
        case ICMD_IF_ICMPNE:  /* op1 = target JavaVM pc                   */

            assert(iptr->dst.block->type != basicblock::TYPE_EXH);

#if SUPPORT_BRANCH_CONDITIONAL_TWO_INTEGER_REGISTERS
            s1 = emit_load_s1(jd, iptr, REG_ITMP1);
            s2 = emit_load_s2(jd, iptr, REG_ITMP2);
            switch (iptr->opc) {
                case ICMD_IF_ICMPEQ:
                    emit_beq(cd, iptr->dst.block, s1, s2);
                    break;
                case ICMD_IF_ICMPNE:
                    emit_bne(cd, iptr->dst.block, s1, s2);
                    break;
            }
            break;
#else
            /* fall-through */
#endif

        case ICMD_IF_ICMPLT:  /* ..., value, value ==> ...                */
        case ICMD_IF_ICMPGT:  /* op1 = target JavaVM pc                   */
        case ICMD_IF_ICMPLE:
        case ICMD_IF_ICMPGE:

            assert(iptr->dst.block->type != basicblock::TYPE_EXH);

            s1 = emit_load_s1(jd, iptr, REG_ITMP1);
            s2 = emit_load_s2(jd, iptr, REG_ITMP2);
#if SUPPORT_BRANCH_CONDITIONAL_CONDITION_REGISTER
# if defined(__I386__) || defined(__X86_64__)
            // XXX Fix this soon!!!
            M_ICMP(s2, s1);
# else
            M_ICMP(s1, s2);
# endif
            emit_bcc(cd, iptr->dst.block, iptr->opc - ICMD_IF_ICMPEQ, BRANCH_OPT_NONE);
#elif SUPPORT_BRANCH_CONDITIONAL_ONE_INTEGER_REGISTER
            // Generate architecture specific instructions.
            codegen_emit_instruction(jd, iptr);
#else
# error Unable to generate code for this configuration!
#endif
            break;

        case ICMD_IF_LCMPEQ:  /* ..., value, value ==> ...                */
        case ICMD_IF_LCMPNE:  /* op1 = target JavaVM pc                   */
        case ICMD_IF_LCMPLT:
        case ICMD_IF_LCMPGT:
        case ICMD_IF_LCMPLE:
        case ICMD_IF_LCMPGE:

            assert(iptr->dst.block->type != basicblock::TYPE_EXH);

            // Generate architecture specific instructions.
            codegen_emit_instruction(jd, iptr);
            break;
        default:
            assert(false);
            break;
    }
}

extern "C" void codegen_emit_copy(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);
    emit_copy(jd, iptr);
}

extern "C" void codegen_emit_getexception(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);
	codegendata*  cd   = jd->cd;
    int32_t d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
	emit_imove(cd, REG_ITMP1, d);
	emit_store_dst(jd, iptr, d);
}

extern "C" void codegen_emit_getstatic(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);
	codegendata*  cd   = jd->cd;
    int32_t d, s1;
#if !defined(__I386__)
	int32_t             fieldtype;
	int32_t             disp;
#endif

#if defined(__I386__)
        // Generate architecture specific instructions.
        codegen_emit_instruction(jd, iptr);
        break;
#else
    {
        fieldinfo* fi;
        //patchref_t* pr;
        if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
            unresolved_field* uf = iptr->sx.s23.s3.uf;
            fieldtype = uf->fieldref->parseddesc.fd->type;
            disp      = dseg_add_unique_address(cd, 0);

            //pr = patcher_add_patch_ref(jd, PATCHER_get_putstatic, uf, disp);
            patcher_add_patch_ref(jd, PATCHER_get_putstatic, uf, disp);

            fi = NULL;		/* Silence compiler warning */
        }
        else {
            fi        = iptr->sx.s23.s3.fmiref->p.field;
            fieldtype = fi->type;
            disp      = dseg_add_address(cd, fi->value);

            if (!class_is_or_almost_initialized(fi->clazz)) {
                PROFILE_CYCLE_STOP;
                patcher_add_patch_ref(jd, PATCHER_initialize_class, fi->clazz, 0);
                PROFILE_CYCLE_START;
            }

            //pr = NULL;		/* Silence compiler warning */
        }

        // XXX X86_64: Here We had this:
        /* This approach is much faster than moving the field
            address inline into a register. */

        M_ALD_DSEG(REG_ITMP1, disp);

        switch (fieldtype) {
        case TYPE_ADR:
            d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
            M_ALD(d, REG_ITMP1, 0);
            break;
        case TYPE_INT:
#if defined(ENABLE_SOFTFLOAT)
        case TYPE_FLT:
#endif
            d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
            M_ILD(d, REG_ITMP1, 0);
            break;
        case TYPE_LNG:
#if defined(ENABLE_SOFTFLOAT)
        case TYPE_DBL:
#endif
            d = codegen_reg_of_dst(jd, iptr, REG_LTMP23);
            M_LLD(d, REG_ITMP1, 0);
            break;
#if !defined(ENABLE_SOFTFLOAT)
        case TYPE_FLT:
            d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
            M_FLD(d, REG_ITMP1, 0);
            break;
        case TYPE_DBL:
            d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
            M_DLD(d, REG_ITMP1, 0);
            break;
#endif
        default:
            // Silence compiler warning.
            d = 0;
        }
        emit_store_dst(jd, iptr, d);
    }
#endif
}

extern "C" void codegen_emit_putstatic(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);
	codegendata*  cd   = jd->cd;
    int32_t s1;
#if !defined(__I386__)
	int32_t             fieldtype;
	int32_t             disp;
#endif

#if defined(__I386__)
    // Generate architecture specific instructions.
    codegen_emit_instruction(jd, iptr);
    break;
#else
{
    fieldinfo* fi;
#if defined(USES_PATCHABLE_MEMORY_BARRIER)
    patchref_t* pr = NULL;
#endif

    if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
        unresolved_field* uf = iptr->sx.s23.s3.uf;
        fieldtype = uf->fieldref->parseddesc.fd->type;
        disp      = dseg_add_unique_address(cd, 0);

#if defined(USES_PATCHABLE_MEMORY_BARRIER)
        pr =
#endif
        patcher_add_patch_ref(jd, PATCHER_get_putstatic, uf, disp);

        fi = NULL;		/* Silence compiler warning */
    }
    else {
        fi = iptr->sx.s23.s3.fmiref->p.field;
        fieldtype = fi->type;
        disp      = dseg_add_address(cd, fi->value);

        if (!class_is_or_almost_initialized(fi->clazz)) {
            PROFILE_CYCLE_STOP;
            patcher_add_patch_ref(jd, PATCHER_initialize_class, fi->clazz, 0);
            PROFILE_CYCLE_START;
        }
    }

    // XXX X86_64: Here We had this:
    /* This approach is much faster than moving the field
        address inline into a register. */

    M_ALD_DSEG(REG_ITMP1, disp);

    switch (fieldtype) {
    case TYPE_ADR:
        s1 = emit_load_s1(jd, iptr, REG_ITMP2);
        M_AST(s1, REG_ITMP1, 0);
        break;
    case TYPE_INT:
#if defined(ENABLE_SOFTFLOAT)
    case TYPE_FLT:
#endif
        s1 = emit_load_s1(jd, iptr, REG_ITMP2);
        M_IST(s1, REG_ITMP1, 0);
        break;
    case TYPE_LNG:
#if defined(ENABLE_SOFTFLOAT)
    case TYPE_DBL:
#endif
        s1 = emit_load_s1(jd, iptr, REG_LTMP23);
        M_LST(s1, REG_ITMP1, 0);
        break;
#if !defined(ENABLE_SOFTFLOAT)
    case TYPE_FLT:
        s1 = emit_load_s1(jd, iptr, REG_FTMP2);
        M_FST(s1, REG_ITMP1, 0);
        break;
    case TYPE_DBL:
        s1 = emit_load_s1(jd, iptr, REG_FTMP2);
        M_DST(s1, REG_ITMP1, 0);
        break;
#endif
    }
#if defined(USES_PATCHABLE_MEMORY_BARRIER)
    codegen_emit_patchable_barrier(iptr, cd, pr, fi);
#endif
}
#endif
}

extern "C" void codegen_emit_ifnull(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);
	codegendata*  cd   = jd->cd;
    int32_t s1;

    assert(iptr->dst.block->type != basicblock::TYPE_EXH);
    s1 = emit_load_s1(jd, iptr, REG_ITMP1);
#if SUPPORT_BRANCH_CONDITIONAL_ONE_INTEGER_REGISTER
    emit_bccz(cd, iptr->dst.block, iptr->opc - ICMD_IFNULL, s1, BRANCH_OPT_NONE);
#elif SUPPORT_BRANCH_CONDITIONAL_CONDITION_REGISTER
    M_TEST(s1);
    emit_bcc(cd, iptr->dst.block, iptr->opc - ICMD_IFNULL, BRANCH_OPT_NONE);
#else
# error Unable to generate code for this configuration!
#endif
}

extern "C" void codegen_emit_inline_body(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);
	codegendata*  cd   = jd->cd;
    linenumbertable_list_entry_add_inline_start(cd, iptr);
    linenumbertable_list_entry_add(cd, iptr->line);
}

extern "C" void codegen_emit_inline_start(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);
    assert(false);
}

extern "C" void codegen_emit_invoke(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);

#if defined(ENABLE_REPLACEMENT)
    codegen_set_replacement_point(cd);
#endif
    methoddesc *md;
    if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
        unresolved_method* um = iptr->sx.s23.s3.um;
        md = um->methodref->parseddesc.md;
    }
    else {
        methodinfo* lm = iptr->sx.s23.s3.fmiref->p.method;
        md = lm->parseddesc;
    }

    gen_method(jd, iptr, NULL, md);
}

/**
 * Fix up register locations in the case where control is transferred to an
 * exception handler block via normal control flow (no exception).
 */
static void fixup_exc_handler_interface(struct jitdata *jd, basicblock *bptr)
{
	// Exception handlers have exactly 1 in-slot
	assert(bptr->indepth == 1);
	varinfo *var = VAR(bptr->invars[0]);
	int32_t d = codegen_reg_of_var(0, var, REG_ITMP1_XPTR);
	emit_load(jd, NULL, var, d);
	// Copy the interface variable to ITMP1 (XPTR) because that's where
	// the handler expects it.
	emit_imove(jd->cd, d, REG_ITMP1_XPTR);
}

extern "C" void codegen_emit_jump(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);
	codegendata*  cd   = jd->cd;
    if (iptr->opc == ICMD_JSR) {
        assert(iptr->sx.s23.s3.jsrtarget.block->type != basicblock::TYPE_EXH);
        emit_br(cd, iptr->sx.s23.s3.jsrtarget.block);
        ALIGNCODENOP;
    }
    else
    {
#if defined(ENABLE_SSA)
        // In case of a goto, phimoves have to be inserted
        // before the jump.
        if (ls != NULL) {
            last_cmd_was_goto = true;
            codegen_emit_phi_moves(jd, bptr);
        }
#endif
        if (iptr->dst.block->type == basicblock::TYPE_EXH)
            fixup_exc_handler_interface(jd, iptr->dst.block);
        emit_br(cd, iptr->dst.block);
        ALIGNCODENOP;
    }
}

extern "C" void codegen_emit_lookup(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);
	codegendata*  cd   = jd->cd;
    int32_t s1 = emit_load_s1(jd, iptr, REG_ITMP1);
    int i = iptr->sx.s23.s2.lookupcount;

    // XXX Again we need to check this
    MCODECHECK((i<<2)+8);   // Alpha, ARM, i386, MIPS, Sparc64
    MCODECHECK((i<<3)+8);   // PPC64
    MCODECHECK(8 + ((7 + 6) * i) + 5);   // X86_64, S390

    // Compare keys.
    for (lookup_target_t* lookup = iptr->dst.lookup; i > 0; ++lookup, --i) {
#if SUPPORT_BRANCH_CONDITIONAL_CONDITION_REGISTER
        emit_icmp_imm(cd, s1, lookup->value);
        emit_beq(cd, lookup->target.block);
#elif SUPPORT_BRANCH_CONDITIONAL_TWO_INTEGER_REGISTERS
        ICONST(REG_ITMP2, lookup->value);
        emit_beq(cd, lookup->target.block, s1, REG_ITMP2);
#elif SUPPORT_BRANCH_CONDITIONAL_ONE_INTEGER_REGISTER
        emit_icmpeq_imm(cd, s1, lookup->value, REG_ITMP2);
        emit_bnez(cd, lookup->target.block, REG_ITMP2);
#else
# error Unable to generate code for this configuration!
#endif
    }

    // Default branch.
    emit_br(cd, iptr->sx.s23.s3.lookupdefault.block);
    ALIGNCODENOP;
}

extern "C" void codegen_emit_phi(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);
    assert(false);
}

extern "C" void codegen_emit_unknown(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);
    assert(false);
}

extern "C" void codegen_emit_return(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);
	int32_t             s1;
	codeinfo*     code = jd->code;
	codegendata*  cd   = jd->cd;
	registerdata* rd   = jd->rd;

    switch (iptr->opc) {
        case ICMD_RETURN:     /* ...  ==> ...                             */
            goto nowperformreturn;

        case ICMD_ARETURN:    /* ..., retvalue ==> ...                    */
            s1 = emit_load_s1(jd, iptr, REG_RESULT);
            // XXX Sparc64: Here this should be REG_RESULT_CALLEE!
            emit_imove(cd, s1, REG_RESULT);

#ifdef ENABLE_VERIFIER
            if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
                PROFILE_CYCLE_STOP;
                unresolved_class *uc = iptr->sx.s23.s2.uc;
                patcher_add_patch_ref(jd, PATCHER_resolve_class, uc, 0);
                PROFILE_CYCLE_START;
            }
#endif /* ENABLE_VERIFIER */
            goto nowperformreturn;

        case ICMD_IRETURN:    /* ..., retvalue ==> ...                    */
#if defined(ENABLE_SOFTFLOAT)
        case ICMD_FRETURN:
#endif

            s1 = emit_load_s1(jd, iptr, REG_RESULT);
            M_NOP;
            M_NOP;
            M_NOP;
            // XXX Sparc64: Here this should be REG_RESULT_CALLEE!
            emit_imove(cd, s1, REG_RESULT);
            goto nowperformreturn;

        case ICMD_LRETURN:    /* ..., retvalue ==> ...                    */
#if defined(ENABLE_SOFTFLOAT)
        case ICMD_DRETURN:
#endif

            s1 = emit_load_s1(jd, iptr, REG_LRESULT);
            // XXX Sparc64: Here this should be REG_RESULT_CALLEE!
            emit_lmove(cd, s1, REG_LRESULT);
            goto nowperformreturn;

#if !defined(ENABLE_SOFTFLOAT)
        case ICMD_FRETURN:    /* ..., retvalue ==> ...                    */

            s1 = emit_load_s1(jd, iptr, REG_FRESULT);
#if defined(SUPPORT_PASS_FLOATARGS_IN_INTREGS)
            M_CAST_F2I(s1, REG_RESULT);
#else
            emit_fmove(cd, s1, REG_FRESULT);
#endif
            goto nowperformreturn;

        case ICMD_DRETURN:    /* ..., retvalue ==> ...                    */

            s1 = emit_load_s1(jd, iptr, REG_FRESULT);
#if defined(SUPPORT_PASS_FLOATARGS_IN_INTREGS)
            M_CAST_D2L(s1, REG_LRESULT);
#else
            emit_dmove(cd, s1, REG_FRESULT);
#endif
            goto nowperformreturn;
#endif
        default:
            assert(false);
            break;
    }

    nowperformreturn:
#if !defined(NDEBUG)
        // Call trace function.
        if (JITDATA_HAS_FLAG_VERBOSECALL(jd))
            emit_verbosecall_exit(jd);
#endif

        // Emit code to call monitorexit function.
        if (checksync && code_is_synchronized(code)) {
            emit_monitor_exit(jd, rd->memuse * 8);
        }

        // Generate method profiling code.
        PROFILE_CYCLE_STOP;

        // Emit code for the method epilog.
        codegen_emit_epilog(jd);
        ALIGNCODENOP;
}

extern "C" void codegen_emit_builtin(struct jitdata *jd, struct instruction *iptr) {
    log("%s\n", burm_opname[iptr->opc]);
	codegendata*  cd   = jd->cd;
    builtintable_entry* bte = iptr->sx.s23.s3.bte;
    methoddesc *md  = bte->md;

#if defined(ENABLE_ESCAPE_REASON) && defined(__I386__)
    if (bte->fp == BUILTIN_escape_reason_new) {
        void set_escape_reasons(void *);
        M_ASUB_IMM(8, REG_SP);
        M_MOV_IMM(iptr->escape_reasons, REG_ITMP1);
        M_AST(EDX, REG_SP, 4);
        M_AST(REG_ITMP1, REG_SP, 0);
        M_MOV_IMM(set_escape_reasons, REG_ITMP1);
        M_CALL(REG_ITMP1);
        M_ALD(EDX, REG_SP, 4);
        M_AADD_IMM(8, REG_SP);
    }
#endif

    // Emit the fast-path if available.
    if (bte->emit_fastpath != NULL) {
        void (*emit_fastpath)(jitdata* jd, instruction* iptr, int d);
        emit_fastpath = (void (*)(jitdata* jd, instruction* iptr, int d)) bte->emit_fastpath;

        assert(md->returntype.type == TYPE_VOID);
        int32_t d = REG_ITMP1;

        // Actually call the fast-path emitter.
        emit_fastpath(jd, iptr, d);

        // If fast-path succeeded, jump to the end of the builtin
        // invocation.
        // XXX Actually the slow-path block below should be moved
        // out of the instruction stream and the jump below should be
        // inverted.
#if SUPPORT_BRANCH_CONDITIONAL_ONE_INTEGER_REGISTER
        os::abort("codegen_emit: Implement jump over slow-path for this configuration.");
#elif SUPPORT_BRANCH_CONDITIONAL_CONDITION_REGISTER
        M_TEST(d);
        emit_label_bne(cd, BRANCH_LABEL_10);
#else
# error Unable to generate code for this configuration!
#endif
    }

    gen_method(jd, iptr, bte, md);
}

void gen_method(struct jitdata *jd, instruction* iptr, builtintable_entry* bte, methoddesc *md) {
	varinfo*            var;
	codegendata*  cd   = jd->cd;

    int32_t s1, d;
    int i = md->paramcount;

    // XXX Check this again!
    MCODECHECK((i << 1) + 64);   // PPC

    // Copy arguments to registers or stack location.
    for (i = i - 1; i >= 0; i--) {
        var = VAR(iptr->sx.s23.s2.args[i]);
        d   = md->params[i].regoff;

        // Already pre-allocated?
        if (var->flags & PREALLOC)
            continue;

        if (!md->params[i].inmemory) {
            switch (var->type) {
            case TYPE_ADR:
            case TYPE_INT:
#if defined(ENABLE_SOFTFLOAT)
            case TYPE_FLT:
#endif
                s1 = emit_load(jd, iptr, var, d);
                emit_imove(cd, s1, d);
                break;

            case TYPE_LNG:
#if defined(ENABLE_SOFTFLOAT)
            case TYPE_DBL:
#endif
                s1 = emit_load(jd, iptr, var, d);
                emit_lmove(cd, s1, d);
                break;

#if !defined(ENABLE_SOFTFLOAT)
            case TYPE_FLT:
#if !defined(SUPPORT_PASS_FLOATARGS_IN_INTREGS)
                s1 = emit_load(jd, iptr, var, d);
                emit_fmove(cd, s1, d);
#else
                s1 = emit_load(jd, iptr, var, REG_FTMP1);
                M_CAST_F2I(s1, d);
#endif
                break;

            case TYPE_DBL:
#if !defined(SUPPORT_PASS_FLOATARGS_IN_INTREGS)
                s1 = emit_load(jd, iptr, var, d);
                emit_dmove(cd, s1, d);
#else
                s1 = emit_load(jd, iptr, var, REG_FTMP1);
                M_CAST_D2L(s1, d);
#endif
                break;
#endif
            default:
                assert(false);
                break;
            }
        }
        else {
            switch (var->type) {
            case TYPE_ADR:
                s1 = emit_load(jd, iptr, var, REG_ITMP1);
                // XXX Sparc64: Here this actually was:
                //     M_STX(s1, REG_SP, JITSTACK + d);
                M_AST(s1, REG_SP, d);
                break;

            case TYPE_INT:
#if defined(ENABLE_SOFTFLOAT)
            case TYPE_FLT:
#endif
#if SIZEOF_VOID_P == 4
                s1 = emit_load(jd, iptr, var, REG_ITMP1);
                M_IST(s1, REG_SP, d);
                break;
#else
                /* fall-through */
#endif

            case TYPE_LNG:
#if defined(ENABLE_SOFTFLOAT)
            case TYPE_DBL:
#endif
                s1 = emit_load(jd, iptr, var, REG_LTMP12);
                // XXX Sparc64: Here this actually was:
                //     M_STX(s1, REG_SP, JITSTACK + d);
                M_LST(s1, REG_SP, d);
                break;

#if !defined(ENABLE_SOFTFLOAT)
            case TYPE_FLT:
                s1 = emit_load(jd, iptr, var, REG_FTMP1);
                M_FST(s1, REG_SP, d);
                break;

            case TYPE_DBL:
                s1 = emit_load(jd, iptr, var, REG_FTMP1);
                // XXX Sparc64: Here this actually was:
                //     M_DST(s1, REG_SP, JITSTACK + d);
                M_DST(s1, REG_SP, d);
                break;
#endif
            default:
                assert(false);
                break;
            }
        }
    }

    // Generate method profiling code.
    PROFILE_CYCLE_STOP;

    // Generate architecture specific instructions.
    codegen_emit_instruction(jd, iptr);

    // Generate method profiling code.
    PROFILE_CYCLE_START;

#if defined(ENABLE_REPLACEMENT)
    // Store size of call code in replacement point.
    if (iptr->opc != ICMD_BUILTIN) {
        // Store size of call code in replacement point.
        cd->replacementpoint[-1].callsize = (cd->mcodeptr - cd->mcodebase)
            - (ptrint) cd->replacementpoint[-1].pc;
    }
#endif

    // Recompute the procedure vector (PV).
    emit_recompute_pv(cd);

    // Store return value.
#if defined(ENABLE_SSA)
    if ((ls == NULL) /* || (!IS_TEMPVAR_INDEX(iptr->dst.varindex)) */ ||
        (ls->lifetime[iptr->dst.varindex].type != jitdata::UNUSED))
        /* a "living" stackslot */
#endif
    switch (md->returntype.type) {
    case TYPE_INT:
    case TYPE_ADR:
#if defined(ENABLE_SOFTFLOAT)
    case TYPE_FLT:
#endif
        s1 = codegen_reg_of_dst(jd, iptr, REG_RESULT);
        // XXX Sparc64: This should actually be REG_RESULT_CALLER, fix this!
        emit_imove(cd, REG_RESULT, s1);
        emit_store_dst(jd, iptr, s1);
        break;

    case TYPE_LNG:
#if defined(ENABLE_SOFTFLOAT)
    case TYPE_DBL:
#endif
        s1 = codegen_reg_of_dst(jd, iptr, REG_LRESULT);
        // XXX Sparc64: This should actually be REG_RESULT_CALLER, fix this!
        emit_lmove(cd, REG_LRESULT, s1);
        emit_store_dst(jd, iptr, s1);
        break;

#if !defined(ENABLE_SOFTFLOAT)
    case TYPE_FLT:
#if !defined(SUPPORT_PASS_FLOATARGS_IN_INTREGS)
        s1 = codegen_reg_of_dst(jd, iptr, REG_FRESULT);
        emit_fmove(cd, REG_FRESULT, s1);
#else
        s1 = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
        M_CAST_I2F(REG_RESULT, s1);
#endif
        emit_store_dst(jd, iptr, s1);
        break;

    case TYPE_DBL:
#if !defined(SUPPORT_PASS_FLOATARGS_IN_INTREGS)
        s1 = codegen_reg_of_dst(jd, iptr, REG_FRESULT);
        emit_dmove(cd, REG_FRESULT, s1);
#else
        s1 = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
        M_CAST_L2D(REG_LRESULT, s1);
#endif
        emit_store_dst(jd, iptr, s1);
        break;
#endif

    case TYPE_VOID:
        break;
    default:
        assert(false);
        break;
    }

    // If we are emitting a fast-path block, this is the label for
    // successful fast-path execution.
    if ((iptr->opc == ICMD_BUILTIN) && (bte->emit_fastpath != NULL)) {
        emit_label(cd, BRANCH_LABEL_10);
    }
}

void codegen_emit_result(struct jitdata *jd, struct instruction *iptr) {

}

void codegen_emit_const_return(struct jitdata *jd, struct instruction *iptr) {

}

int get_opc(struct instruction *iptr) { return iptr->opc; }