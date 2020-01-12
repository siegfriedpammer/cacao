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
#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/MethodC2.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
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
// TODO inlining: info abour source states
bool is_dependent_on(Instruction* first, Instruction* second)
{
	// TODO inlining:
    if(first->get_opcode() == Instruction::RETURNInstID)
        return true;
    
    if(first->get_opcode() == Instruction::SourceStateInstID && *(first->dep_begin()) == second)
        return true;

    return first == second;
}

void remove_all_deps(Instruction* inst){
	LOG("Removing deps " << inst << nl);
	auto it = inst->dep_begin();
	while (it != inst->dep_end()){
		inst->remove_dep(*it);
		it = inst->dep_begin();
	}

	LOG("Removing reverse deps " << inst << nl);
	it = inst->rdep_begin();
	while (it != inst->rdep_end()){
		auto I = *it;
		I->remove_dep(inst);
		it = inst->rdep_begin();
	}
}

class InliningOperation {
private:
	INVOKEInst* call_site;
	BeginInst* old_call_site_bb;
	Method* caller_method;
	Method* callee_method;
	BeginInst* pre_call_site_bb;
	BeginInst* post_call_site_bb;
    PHIInst* phi;

    bool needs_phi(){
        return call_site->get_type() != Type::VoidTypeID;
    }

	void create_pre_call_site_bb()
	{
		LOG("create_pre_call_site_bb" << nl);
		pre_call_site_bb = new BeginInst();
		caller_method->add_bb(pre_call_site_bb);
		List<Instruction*> to_remove;
		// TODO inlining: only iterate bb
		for (auto it = caller_method->begin(); it != caller_method->end(); it++) {
			auto I = *it;

			// ignore bb begins
			if(I->get_opcode() == Instruction::BeginInstID)
				continue;

			if (I->get_BeginInst() != old_call_site_bb || is_dependent_on(I, call_site))
				continue;

			// For now deoptimization is not supported.
			if(I->get_opcode() == Instruction::SourceStateInstID){
				remove_all_deps(I);
				to_remove.push_back(I);
				continue;
			}

            LOG("Adding " << I << " to pre call site bb " << pre_call_site_bb << nl);
            I->set_BeginInst(pre_call_site_bb);
            I->replace_dep(old_call_site_bb, pre_call_site_bb);
		}

		for (auto it = to_remove.begin(); it != to_remove.end(); it++){
			caller_method->remove_Instruction(*it);
		}
	}

	void create_post_call_site_bb()
	{
		LOG("create_post_call_site_bb" << nl);
		post_call_site_bb = new BeginInst();
		caller_method->add_bb(post_call_site_bb);
		// For now deoptimization is not supported.
		Instruction* source_state_of_call_site = NULL;
        // TODO only iterate bb
		for (auto it = caller_method->begin(); it != caller_method->end(); it++) {
			auto I = *it;

			// ignore bb begins
			if(I->get_opcode() == Instruction::BeginInstID)
				continue;

			if (I == call_site || I->get_BeginInst() != old_call_site_bb || !is_dependent_on(I, call_site))
				continue;

            LOG("Adding " << I << " to post call site bb " << post_call_site_bb << nl);

			if (I == old_call_site_bb->get_EndInst()) {
				post_call_site_bb->set_EndInst(I->to_EndInst());
			}
			else if (I->to_SourceStateInst() && *(I->dep_begin()) == call_site) { // remove source state of method call
                source_state_of_call_site = I;
                continue;
			}
			I->set_BeginInst(post_call_site_bb);
			I->replace_dep(old_call_site_bb, post_call_site_bb);
		}

		caller_method->remove_Instruction(source_state_of_call_site);
	}

    Instruction* transform_instruction(Instruction* I)
    {
        LOG("Transforming " << I << nl);
        switch (I->get_opcode()) {
            case Instruction::RETURNInstID:
                auto begin_inst = I->get_BeginInst();
                auto end_instruction = new GOTOInst(begin_inst, post_call_site_bb);
                begin_inst->set_EndInst(end_instruction);
                LOG("Rewriting return instruction " << I << " to " << end_instruction << nl);
                return end_instruction;
        }
        return I;
    }

	void add_call_site_bbs()
	{
		LOG("create_call_site_bb" << callee_method->get_desc_utf8() << nl);
		if(needs_phi()){
			// the phi will be the replacement for the dependencies to the invoke inst in the post call site bb
			auto return_type = call_site->get_type();
            phi = new PHIInst(return_type, post_call_site_bb);
        }

		for (auto it = callee_method->begin(); it != callee_method->end(); it++) {
			Instruction* I = *it;
			LOG("Adding to call site " << *I << nl);

			// For now deoptimization is not supported.
			if(I->get_opcode() == Instruction::SourceStateInstID) {
				remove_all_deps(I);
				delete I;
				continue;
			}

			if(I->get_opcode() == Instruction::BeginInstID) {
				caller_method->add_bb(I->to_BeginInst());
				continue;
			}

            if(I->get_opcode() == Instruction::RETURNInstID){
                auto returnInst = I->to_RETURNInst();
                if(needs_phi())
                    phi->append_op(*((returnInst->op_begin())));
            }
            caller_method->add_Instruction(transform_instruction(I));
		}
        
        if(needs_phi()){
			if(phi->op_size() == 1) {
				call_site->replace_value(*(phi->op_begin()));
				delete phi;
			} else {
				caller_method->add_Instruction(phi);
		    	call_site->replace_value(phi);
			}
		}
	}

	void replace_method_parameters(){
		List<Instruction*> to_delete;
		for (auto it = callee_method->begin(); it != callee_method->end(); it++) {
			auto I = *it;
			if(I->get_opcode() == Instruction::LOADInstID){
				auto index = (I->to_LOADInst())->get_index();
				auto given_operand = call_site->get_operand(index);
				LOG("Replacing Load inst" << I << " with " << given_operand << nl);
				I->replace_value(given_operand);
				to_delete.push_back(I);
			}
		}
		for (auto it = to_delete.begin(); it != to_delete.end(); it++) {
			caller_method->remove_Instruction(*it);
		}
	}

	void transform_caller_bb()
	{
		create_pre_call_site_bb();
		create_post_call_site_bb();
		add_call_site_bbs();
		replace_method_parameters();

		// correct init bb if necessary
		if (caller_method->get_init_bb() == old_call_site_bb) {
			LOG("Correcting init bb to " << pre_call_site_bb << nl);
			caller_method->set_init_bb(pre_call_site_bb);
 		}

		auto call_site_bb_init = callee_method->get_init_bb();
		LOG("Rewriting next bb of pre call site bb to " << call_site_bb_init << nl);
		auto end_inst = new GOTOInst(pre_call_site_bb, call_site_bb_init);
		pre_call_site_bb->set_EndInst(end_inst);
		caller_method->add_Instruction(end_inst);
	}

public:
	InliningOperation(INVOKEInst* site, Method* callee)
	{
		call_site = site;
		old_call_site_bb = call_site->get_BeginInst();
		caller_method = call_site->get_Method();
		callee_method = callee;
	}

	void execute()
	{
		LOG("Inserting new basic blocks into original method" << nl);
		transform_caller_bb();

		LOG("Removing invoke instruction" << nl);
		remove_all_deps(call_site);
		caller_method->remove_Instruction(call_site);
		caller_method->remove_bb(old_call_site_bb);
	}
};

void print_all_nodes(Method* M)
{
	// TODO inlining: remove
	LOG("==========================="<<nl);
	for (auto it = M->begin(); it != M->end(); it++) {
		auto I = *it;
		LOG(I << nl);
		for (auto itt = I->dep_begin(); itt != I->dep_end(); itt++) {
			LOG("dep " << *(itt) << nl);
		}
		for (auto itt = I->rdep_begin(); itt != I->rdep_end(); itt++) {
			LOG("rdep " << *(itt) << nl);
		}
		for (auto itt = I->op_begin(); itt != I->op_end(); itt++) {
			LOG("op " << *(itt) << nl);
		}
		for (auto itt = I->user_begin(); itt != I->user_end(); itt++) {
			LOG("user " << *(itt) << nl);
		}
		if(I->to_EndInst()){
			auto end = I->to_EndInst();
			for (auto itt = end->succ_begin(); itt != end->succ_end(); itt++) {
				LOG("succ " << *(itt) << nl);
			}
		}
		if(I->to_BeginInst()){
			auto end = I->to_BeginInst();
			for (auto itt = end->pred_begin(); itt != end->pred_end(); itt++) {
				LOG("pred " << *(itt) << nl);
			}
		}
	}
	LOG("==========================="<<nl);
}

bool InliningPass::run(JITData& JD)
{
	LOG("Start of inlining pass." << nl);
	Method* M = JD.get_Method();
	LOG("Inlining for class: " << M->get_class_name_utf8() << nl);
	LOG("Inlining for method: " << M->get_name_utf8() << nl);

	LOG("BEFORE" << nl);
	print_all_nodes(M);

	for (auto it = M->begin(); it != M->end(); it++) {
		auto I = *it;

		if (can_inline(I)) {
			inline_instruction(I);
			break;
		}
	}

	LOG("AFTER" << nl);
	print_all_nodes(M);

	LOG("End of inlining pass." << nl);

	return true;
}

bool InliningPass::can_inline(Instruction* I)
{
	return I->get_opcode() == Instruction::INVOKESTATICInstID;
}

void InliningPass::inline_instruction(Instruction* I)
{
	switch (I->get_opcode()) {
		case Instruction::INVOKESTATICInstID:
			inline_invoke_static_instruction(I->to_INVOKESTATICInst());
			break;
		default: break;
	}
}

void InliningPass::inline_invoke_static_instruction(INVOKESTATICInst* I)
{
	LOG("Inlining static invoke instruction " << I << nl);
	auto callee = I->get_fmiref()->p.method;
	auto* jd = jit_jitdata_new(callee);
	jit_jitdata_init_for_recompilation(jd);
	JITData JD(jd);

	// TODO inlining don't run all passes.
	PassRunner runner;
	runner.runPasses(JD);
	auto callee_method = JD.get_Method();

	LOG("Successfully retrieved SSA-Code for instruction " << nl);

    InliningOperation(I, callee_method).execute();
}

// pass usage
PassUsage& InliningPass::get_PassUsage(PassUsage& PU) const
{
    PU.immediately_after<SSAConstructionPass>();
	return PU;
}

// register pass
static PassRegistry<InliningPass> X("InliningPass");

Option<bool> InliningPass::enabled("InliningPass",
                                   "compiler2: enable InliningPrinterPass",
                                   false,
                                   ::cacao::option::xx_root());

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
