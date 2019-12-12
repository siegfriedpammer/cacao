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


    // TODO inlining: remove
	for (auto it = M->begin(); it != M->end(); it++) {
        auto I = *it;
        LOG(I << nl);
	    for (auto itt = I->dep_begin(); itt != I->dep_end(); itt++) {
            LOG("dep " << *(itt) << nl);
        }
    }

	for (auto it = M->begin(); it != M->end(); it++) {
		auto I = *it;

		if (can_inline(I)) {
            inline_instruction(I);
            break;
		}
	}

    // TODO inlining: remove
	for (auto it = M->bb_begin(); it != M->bb_end(); it++) {
        auto BI = *it;
        LOG("BI " << BI << " with end " << BI->get_EndInst() << nl);
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

void InliningPass::transform_caller_bb(Instruction* call_site, Method* to_inline){
    auto caller_method = call_site->get_Method();
    BeginInst* BI = call_site->get_BeginInst();
    LOG("Transforming basic block with BeginInst " << BI  << " within method " << to_inline->get_name_utf8() << nl);

    auto pre_callsite_bb = create_pre_call_site_bb(BI, call_site);
    LOG("Adding pre call bb " << pre_callsite_bb << nl);
    caller_method->add_bb(pre_callsite_bb);
    
    auto post_callsite_bb = create_post_call_site_bb(BI, call_site);
    LOG("Adding post call bb " << pre_callsite_bb << nl);
    caller_method->add_bb(post_callsite_bb);

    add_call_site_bbs(to_inline, BI, post_callsite_bb, call_site);
    
    LOG("Test");

    // correct init bb if necessary
    if(caller_method->get_init_bb() == BI){
        LOG("Correcting init bb to " << pre_callsite_bb << nl);
        caller_method->set_init_bb(pre_callsite_bb);
    }

    auto call_site_bb_init = to_inline->get_init_bb();
    LOG("Rewriting end instruction of pre call site bb to " << call_site_bb_init << nl);
    auto end_inst = new GOTOInst(pre_callsite_bb, call_site_bb_init);
    pre_callsite_bb->set_EndInst(end_inst);
    caller_method->add_Instruction(end_inst);
}

BeginInst* InliningPass::create_pre_call_site_bb(BeginInst* bb, Instruction* call_site){
    LOG("create_pre_call_site_bb" << nl);
    auto caller_method = bb->get_Method();
    auto pre_callsite_BI = new BeginInst();
    pre_callsite_BI->set_Method(caller_method);
	for (auto it = caller_method->begin(); it != caller_method->end(); it++) {
        auto I = *it;
        
        if(I->get_BeginInst() == bb && !(bb->get_EndInst() == I)  && !is_dependent_on(I, call_site)) {
            if(I->to_SourceStateInst()) {
                LOG("Adding source state inst " << I << " to pre call site bb " << nl);
                LOG("Appending " << pre_callsite_BI << nl);
            }
            else {
                LOG("Adding to pre call site bb " << I << " with begin inst " << pre_callsite_BI << nl);
                I->set_BeginInst(pre_callsite_BI);
            }
            replace_dep(I, bb, pre_callsite_BI);
        }
    }
    return pre_callsite_BI;
}

BeginInst* InliningPass::create_post_call_site_bb(BeginInst* bb, Instruction* call_site){
    LOG("create_post_call_site_bb" << nl);
    auto original_method = bb->get_Method();
    auto post_callsite_BI = new BeginInst();
    post_callsite_BI->set_Method(original_method);
    Instruction* source_state_of_call_site = NULL;
	for (auto it = original_method->begin(); it != original_method->end(); it++) {
        auto I = *it;

        if(I == call_site) continue;

        if(I == bb->get_EndInst()) {
            I->set_BeginInst(post_callsite_BI);
            post_callsite_BI->set_EndInst(I->to_EndInst());
        } else if(I->get_BeginInst() == bb && is_dependent_on(I, call_site)) {
            if (I->to_SourceStateInst()) {
                LOG("Adding source state inst " << I << " to post call site bb " << nl);
                LOG("Appending " << post_callsite_BI << nl);

                if(*(I->dep_begin()) == call_site) {
                    source_state_of_call_site = I;
                    continue;
                }
                I->append_dep(post_callsite_BI); // TODO inlining: correct source state for invoke call
            } else {
                LOG("Adding to post call site bb " << *it << nl);
                I->set_BeginInst(post_callsite_BI);
            }
            replace_dep(I, bb, post_callsite_BI);
        }
    }
    
    // TODO inline: need to preserve? HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
    original_method->remove_Instruction(source_state_of_call_site);
    return post_callsite_BI;
}

void InliningPass::add_call_site_bbs(Method* to_inline, BeginInst* bb, BeginInst* post_call_site_bb, Instruction* call_site){
    LOG("create_call_site_bb" << nl);
    auto original_method = bb->get_Method();
	for (auto it = to_inline->begin(); it != to_inline->end(); it++) {
		Instruction *I = *it;
        LOG("Adding to call site " << *I << nl);

        if(I->to_BeginInst()){
            BeginInst* begin_inst = I->to_BeginInst();
            LOG("Adding bb from other method " << begin_inst << nl);
            original_method->add_bb(begin_inst);
         } else {
            original_method->add_Instruction(transform_instruction(I, post_call_site_bb));
        }
    }
}

Instruction* InliningPass::transform_instruction(Instruction* I, BeginInst* post_call_site_bb){
    LOG("Transforming " << I << nl);
    if(I->to_RETURNInst()){
        auto begin_inst = I->get_BeginInst();
        auto end_instruction = new GOTOInst(begin_inst, post_call_site_bb);
        begin_inst->set_EndInst(end_instruction);
        LOG("Rewriting return instruction " << I << " to " << end_instruction << nl);
        // TODO inlining delete?
        return end_instruction;
    }
    return I;
}

void InliningPass::replace_dep(Instruction* for_inst, Instruction* old_inst, Instruction* new_inst){
    LOG("Replacing dependencies for " << for_inst << nl);
    for(auto it = for_inst->dep_begin(); it != for_inst->dep_end(); it++){
        auto I = *it;
        if(I == old_inst){
            for_inst->remove_dep(I);
            for_inst->append_dep(new_inst);
            it = for_inst->dep_begin(); // TODO inlining
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

Option<bool> InliningPass::enabled("InliningPass","compiler2: enable InliningPrinterPass",false,::cacao::option::xx_root());

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
