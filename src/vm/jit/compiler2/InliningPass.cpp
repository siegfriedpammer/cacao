/* src/vm/jit/compiler2/InliningPass.cpp - InliningPass

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

#include <algorithm>
#include <cassert>
#include <iostream>

#include "vm/jit/compiler2/InliningPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/MethodC2.hpp"
#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/SSAConstructionPass.hpp"
#include "vm/jit/compiler2/SourceStateAttachmentPass.hpp"
#include "vm/jit/compiler2/alloc/queue.hpp"
#include "vm/jit/jit.hpp"

#include "toolbox/logging.hpp"

// define name for debugging (see logging.hpp)
#define DEBUG_NAME "compiler2/InliningPass"

namespace cacao {
namespace jit {
namespace compiler2 {

bool InliningPass::run(JITData &JD) {
    LOG("Start of inlining pass." << nl);
    Method* M = JD.get_Method();
    LOG("Inlining for class: " << M->get_class_name_utf8() << nl);
    LOG("Inlining for method: " << M->get_name_utf8() << nl);

	for (auto it = M->begin(); it != M->end(); it++) {
		auto I = *it;

		if (can_inline(I)) {
            inline_instruction(I);
            break;
		}
	}

    LOG("End of inlining pass." << nl);

    return true;
}

bool InliningPass::can_inline(Instruction* I){
    LOG("Can inline instruction " << I << nl);
    return I->get_opcode() == Instruction::INVOKESTATICInstID;
}

void InliningPass::inline_instruction(Instruction* I){
    LOG("Inlining instruction" << I << nl);
    switch (I->get_opcode())
    {
        case Instruction::INVOKESTATICInstID:
            inline_invoke_static_instruction(I->to_INVOKESTATICInst());
            break;
        default:
            break;
    }
}

void InliningPass::inline_invoke_static_instruction(INVOKESTATICInst* I){
    LOG("Inlining static invoke instruction " << I << nl);
    auto callee = I->get_fmiref()->p.method;
    auto *jd = jit_jitdata_new(callee);
	jit_jitdata_init_for_recompilation(jd);
    JITData JD(jd);
    
    // TODO inlining don't run all passes.
    PassRunner runner;
	runner.runPasses(JD);
    auto callee_method = JD.get_Method();

    LOG("Successfully retrieved SSA-Code for instruction " << nl);

    auto original_method = I->get_Method();
    auto caller_begin_inst = I->get_BeginInst();

    LOG("Inserting new basic blocks into original method" << nl);
    transform_caller_bb(I, callee_method);
    LOG("Removing invoke instruction" << nl);
    caller_begin_inst->remove_dep(I);
    // TODO inlining better
    for(auto dep_it = I->rdep_begin(); dep_it != I->rdep_end(); dep_it++){
        auto X = *dep_it;
        X->remove_dep(I);
        dep_it = I->rdep_begin();
    }
    original_method->remove_Instruction(I);
    original_method->remove_bb(caller_begin_inst);
}

void InliningPass::transform_caller_bb(Instruction* callee, Method* to_inline){
    auto original_method = callee->get_Method();
    BeginInst* BI = callee->get_BeginInst();
    LOG("Transforming basic block with BeginInst " << BI  << " within method " << to_inline->get_name_utf8() << nl);

    // instructions which do not depend on the method call within this basic block
    auto pre_callsite_BI = new BeginInst();
    List<Instruction*> dependent_instructions;
    List<Instruction*> source_state_instructions;
    pre_callsite_BI->set_Method(original_method);
	for (auto it = original_method->begin(); it != original_method->end(); it++) {
        auto I = *it;
        
        if(I->get_BeginInst() != BI)
            continue;

        if(!is_dependent_on(I, callee)){
            if(I->to_SourceStateInst()) {
                LOG("Adding source state inst " << I << " to pre call site bb " << nl);
                LOG("Appending " << pre_callsite_BI << nl);
                I->append_dep(pre_callsite_BI);
                source_state_instructions.push_back(I);
            }
            else{
                LOG("Adding to pre call site bb " << I << " with begin inst " << pre_callsite_BI << nl);
                I->set_BeginInst(pre_callsite_BI);
                pre_callsite_BI->append_dep(I);
            }
        } else {
            dependent_instructions.push_back(I);
        }
    }
        

    LOG("Adding pre call bb " << pre_callsite_BI << nl);
    original_method->add_bb(pre_callsite_BI);
    
    if(original_method->get_init_bb() == BI)
        original_method->set_init_bb(pre_callsite_BI);
    
    auto post_callsite_BI = new BeginInst();
    post_callsite_BI->set_Method(original_method);
    // TODO inlining add basic block with dependent instructions
    for(auto it = dependent_instructions.begin(); it != dependent_instructions.end(); it++){
        auto I = *it;
        
        if(I == callee)
            continue;

        if(I->to_SourceStateInst()) {
            LOG("Adding source state inst " << I << " to post call site bb " << nl);
            LOG("Appending " << post_callsite_BI << nl);
            I->append_dep(post_callsite_BI);
            source_state_instructions.push_back(I);
        } else{
            LOG("Adding to post call site bb " << *it << nl);
            I->set_BeginInst(post_callsite_BI);
            post_callsite_BI->append_dep(I);
        }
    }
    LOG("Adding post call bb " << post_callsite_BI << nl);
    original_method->add_bb(post_callsite_BI);

	for (auto it = source_state_instructions.begin(); it != source_state_instructions.end(); it++){
        auto I = *it;
        for(auto dep_it = I->dep_begin(); dep_it != I->dep_end(); dep_it++)
            LOG("dep " << *dep_it << nl);
        I->remove_dep(BI);
    }

    // move the instructions from the callee to the caller
    auto operator_it = callee->op_begin();
	for (auto it = to_inline->begin(); it != to_inline->end(); it++) {
		Instruction *I = *it;
        LOG("Adding to call site " << *I << nl);

        if(I->to_BeginInst()){
            BeginInst* begin_inst = I->to_BeginInst();
            LOG("Adding bb from other method " << begin_inst << nl);
            original_method->add_bb(begin_inst);

            if((begin_inst) == to_inline->get_init_bb()){
                LOG("Rewriting end instruction of pre call site bb to " << begin_inst << nl);
                auto goto_inst = new GOTOInst(pre_callsite_BI, begin_inst);
                original_method->add_Instruction(goto_inst);
                pre_callsite_BI->set_EndInst(goto_inst);
            }
         } else if (I->to_RETURNInst()) {
            // TODO inlining return vs goto
            auto begin_inst = I->get_BeginInst();
            auto goto_inst = new GOTOInst(begin_inst, post_callsite_BI);
            LOG("Rewriting end instruction " << I << " to " << goto_inst << nl);
            original_method->add_Instruction(goto_inst);
            begin_inst->set_EndInst(goto_inst);
            // TODO inlining delete?
         } else {
            original_method->add_Instruction(I);
        }
    }
}

bool InliningPass::is_dependent_on(Instruction* first, Instruction* second){
    if(first == second)
        return true;
    
    bool is_dependent = false;
    for (auto bb_it = first->dep_begin(); bb_it != first->dep_end(); bb_it++)
        is_dependent |= is_dependent_on(*bb_it, second);
    return is_dependent;
}

// pass usage
PassUsage& InliningPass::get_PassUsage(PassUsage &PU) const {
    PU.requires<HIRInstructionsArtifact>();
	PU.immediately_after<SSAConstructionPass>(); // TODO inlining
	return PU;
}

// register pass
static PassRegistry<InliningPass> X("InliningPass");

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao


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
