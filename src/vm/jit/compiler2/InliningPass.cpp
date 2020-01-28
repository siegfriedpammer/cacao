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
#include "vm/jit/compiler2/alloc/queue.hpp"
#include "vm/jit/jit.hpp"
#include "boost/iterator/filter_iterator.hpp"

#include "toolbox/logging.hpp"

// define name for debugging (see logging.hpp)
#define DEBUG_NAME "compiler2/InliningPass"


STAT_DECLARE_GROUP(compiler2_stat)
STAT_REGISTER_SUBGROUP(compiler2_inliningpass_stat,
	"inliningpasspass","inliningpasspass",compiler2_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,inlined_method_invocations,0,"# of inlined method invocations",
	"number of inlined method invocations",compiler2_inliningpasspass_stat)

namespace cacao {
namespace jit {
namespace compiler2 {
	
/*
	IMPORTANT: The current implementation does not work with on-stack-replacement. Therefore the InliningPass should
	not be opted in, when testing or working with deoptimization and on-stack-replacement. The reason for this is, 
	that the SourceStateInstructions are not handled correctly. For example the source state instruction for the
	inlined call will be deleted completely, which causes information loss.
*/
bool depends_on(Instruction* first, Instruction* second)
{
    if(first == second){
		return true;
	}

	// if first is not in the same bb, first cannot depend on second anymore. Both start in the same bb and the traversal
	// is only towards the start of the method.
	if(first->get_BeginInst() != second->get_BeginInst()){
		return false;
	}
	
	auto is_dependent_rec = [&](Instruction* i){return depends_on(i, second);};
	auto is_dependent_rec_op = [&](Value* v){
		Instruction* i = v->to_Instruction();
		return i ? depends_on(i, second) : false;
	};

	return std::any_of(first->dep_begin(),first->dep_end(),is_dependent_rec) ||
		   std::any_of(first->op_begin(),first->op_end(),is_dependent_rec_op);
}

void remove_instruction(Instruction* inst){
	LOG("removing " << inst << " with (r)dependencies" << nl);

	LOG("Removing deps " << inst << nl);
	auto it = inst->dep_begin();
	while (it != inst->dep_end()){
		auto I = *it;
		LOG("dep " << I << nl);
		inst->remove_dep(I);
		it = inst->dep_begin();
	}

	LOG("Removing reverse deps " << inst << nl);
	it = inst->rdep_begin();
	while (it != inst->rdep_end()){
		auto I = *it;
		LOG("rdep " << I << nl);
		I->remove_dep(inst);
		it = inst->rdep_begin();
	}

	if(inst->get_opcode() == Instruction::BeginInstID){
		inst->get_Method()->remove_bb(inst->to_BeginInst());	
	} else {
		inst->get_Method()->remove_Instruction(inst);
	}
}

void print_node(Instruction* I){
	LOG(I << " within " << I->get_BeginInst() << nl);
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
		auto begin = I->to_BeginInst();
		for (auto itt = begin->pred_begin(); itt != begin->pred_end(); itt++) {
			LOG("pred " << *(itt) << nl);
		}

		auto end = begin->get_EndInst();
		LOG("end " << end << nl);
	}
}

class SingleBBInliningOperation {
	private:
		BeginInst* call_site_bb;
		INVOKEInst* call_site;
		Method* caller_method;
		Method* callee_method;

		void replace_method_parameters(){
			List<Instruction*> to_remove;
			for (auto it = callee_method->begin(); it != callee_method->end(); it++) {
				auto I = *it;
				if(I->get_opcode() == Instruction::LOADInstID){
					auto index = (I->to_LOADInst())->get_index();
					auto given_operand = call_site->get_operand(index);
					LOG("Replacing Load inst" << I << " with " << given_operand << nl);
					I->replace_value(given_operand);
					to_remove.push_back(I);
				}
			}
			for (auto it = to_remove.begin(); it != to_remove.end(); it++) {
				remove_instruction(*it);
			}
		}

		void add_call_site_bbs()
		{
			LOG("add_call_site_bbs" << nl);
			Value* result;

			for (auto it = callee_method->begin(); it != callee_method->end(); it++) {
				Instruction* I = *it;
				LOG("Adding to call site " << *I << nl);

				if(I->get_opcode() == Instruction::BeginInstID){
					continue; // ignore begin instruction
				} else if (I->get_opcode() == Instruction::SourceStateInstID){
					continue; // ignore source states for now
				} else if(I->get_opcode() == Instruction::RETURNInstID){
					auto returnInst = I->to_RETURNInst();
					LOG(returnInst<<nl);
					result = *(returnInst->op_begin());
					LOG(result<<nl);
					continue;
				}

				caller_method->add_Instruction(I);
				I->set_BeginInst(call_site_bb);
				// TODO inlining how to ensure correct scheduling via scheduling edges
			}
			call_site->replace_value(result);
		}

	public:
	SingleBBInliningOperation(INVOKEInst* site, Method* callee)
	{
		call_site = site;
		call_site_bb = call_site->get_BeginInst();
		caller_method = call_site->get_Method();
		callee_method = callee;
	}

	virtual void execute()
	{
		LOG("Inserting new basic blocks into original method" << nl);
		add_call_site_bbs();
		replace_method_parameters();
	}
};

class ComplexInliningOperation {
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

		bool does_not_belong_to_post_call_site_bb(Instruction* I) {
			// Always include the end instruction of the original bb.
			if(I == old_call_site_bb->get_EndInst()) return false;

			return I->get_opcode() == Instruction::BeginInstID ||
				I == call_site ||
				I->get_BeginInst() != old_call_site_bb ||
				!depends_on(I, call_site);
		}

		// TODO inlining: extract into base class
		void replace_method_parameters(){
			List<Instruction*> to_remove;
			for (auto it = callee_method->begin(); it != callee_method->end(); it++) {
				auto I = *it;
				if(I->get_opcode() == Instruction::LOADInstID){
					auto index = (I->to_LOADInst())->get_index();
					auto given_operand = call_site->get_operand(index);
					LOG("Replacing Load inst" << I << " with " << given_operand << nl);
					I->replace_value(given_operand);
					to_remove.push_back(I);
				}
			}
			for (auto it = to_remove.begin(); it != to_remove.end(); it++) {
				remove_instruction(*it);
			}
		}

		void create_post_call_site_bb()
		{
			post_call_site_bb = new BeginInst();
			LOG("create_post_call_site_bb " << post_call_site_bb << nl);
			caller_method->add_bb(post_call_site_bb);
			// TODO inlining: only iterate bb
			for (auto it = caller_method->begin(); it != caller_method->end(); it++) {
				auto I = *it;

				if (does_not_belong_to_post_call_site_bb(I))
					continue;

				LOG("Adding " << I << " to post call site bb " << post_call_site_bb << nl);

				if (I == old_call_site_bb->get_EndInst()) {
					LOG("Setting end inst of post call site bb " << I << nl);
					post_call_site_bb->set_EndInst(I->to_EndInst());
				}

				// TODO inlining: check if floating
				I->set_BeginInst(post_call_site_bb);

				LOG("Replacing " << old_call_site_bb << " dep for " << post_call_site_bb << " in " << I << nl);
				I->replace_dep(old_call_site_bb, post_call_site_bb);
			}
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
			LOG("add_call_site_bbs" << callee_method->get_desc_utf8() << nl);
			if(needs_phi()){
				// the phi will be the replacement for the dependencies to the invoke inst in the post call site bb
				auto return_type = call_site->get_type();
				phi = new PHIInst(return_type, post_call_site_bb);
			}

			for (auto it = callee_method->begin(); it != callee_method->end(); it++) {
				Instruction* I = *it;
				LOG("Adding to call site " << *I << nl);

				if(I->get_opcode() == Instruction::BeginInstID) {
					caller_method->add_bb(I->to_BeginInst());
					continue;
				}

				if(I->get_opcode() == Instruction::RETURNInstID){
					auto returnInst = I->to_RETURNInst();
					if(needs_phi())
						phi->append_op(*(returnInst->op_begin()));
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

		void wire_up_call_site_with_post_call_site_bb(){
			auto call_site_bb_init = callee_method->get_init_bb();
			LOG("Rewriting next bb of pre call site bb to " << call_site_bb_init << nl);
			auto end_inst = new GOTOInst(pre_call_site_bb, call_site_bb_init);
			pre_call_site_bb->set_EndInst(end_inst);
			caller_method->add_Instruction(end_inst);
		}

	public:
		ComplexInliningOperation(INVOKEInst* site, Method* callee)
		{
			call_site = site;
			old_call_site_bb = call_site->get_BeginInst();
			caller_method = call_site->get_Method();
			callee_method = callee;
		}

		virtual void execute()
		{
			LOG("Inserting new basic blocks into original method" << nl);
			pre_call_site_bb = old_call_site_bb;
			create_post_call_site_bb();
			add_call_site_bbs();
			replace_method_parameters();
			wire_up_call_site_with_post_call_site_bb();
		}
};

class Heuristic {
	protected: 
		bool can_inline(INVOKEInst* I){
			bool can_inline_instruction = I->get_opcode() == Instruction::INVOKESTATICInstID ||
										I->get_opcode() == Instruction::INVOKESPECIALInstID;

			if(!can_inline_instruction) return false;

			auto source_method = I->get_Method();
			auto target_method = I->to_INVOKEInst()->get_fmiref()->p.method;

			// TODO inlining ctor
			auto is_ctor_call = target_method->name == "<init>";
			auto is_recursive_call = source_method->get_class_name_utf8() == target_method->clazz->name &&
									source_method->get_name_utf8() == target_method->name &&
									source_method->get_desc_utf8() == target_method->descriptor;

			return !is_ctor_call && !is_recursive_call;
		}
	public:
		virtual bool should_inline(INVOKEInst* I);
};

class EverythingPossibleHeuristic : public Heuristic {
	public:
		virtual bool should_inline(INVOKEInst* I){
			return can_inline(I);
		}
};

class CodeFactory {
	public:
		JITData create_ssa(INVOKEInst* I){
			auto callee = I->get_fmiref()->p.method;
			auto* jd = jit_jitdata_new(callee);
			jit_jitdata_init_for_recompilation(jd);
			JITData JD(jd);

			PassRunner runner;
			runner.runPassesUntil<SSAConstructionPass>(JD);
			return JD;
		}
};

void print_all_nodes(Method* M)
{
	// TODO inlining: remove
	LOG("==========================="<<nl);
	for (auto it = M->begin(); it != M->end(); it++) {
		print_node(*it);
	}
	LOG("==========================="<<nl);
}

void remove_call_site(INVOKEInst* call_site){
	auto is_source_state = [](Instruction* I){ return I->get_opcode() == Instruction::SourceStateInstID;};
	auto source_state = *std::find_if(call_site->rdep_begin(), call_site->rdep_end(), is_source_state);
	source_state->to_SourceStateInst()->replace_dep(call_site, call_site->get_BeginInst());

	// TODO inlining: inlined method calls
 	// the second dependency is special for invoke instructions and needs to be preserved.
	auto is_invoke_inst = [](Instruction* I){ return I->to_INVOKEInst() != NULL;};
	auto invoke_inst = *std::find_if(call_site->rdep_begin(), call_site->rdep_end(), is_invoke_inst);
	if(invoke_inst){
		auto replace_with = *std::next(call_site->dep_begin(), 1);
		invoke_inst->to_INVOKEInst()->replace_dep(call_site, replace_with);
	}

	remove_instruction(call_site);
}

bool InliningPass::run(JITData& JD)
{
	LOG("Start of inlining pass." << nl);
	Method* M = JD.get_Method();
	LOG("Inlining for class: " << M->get_class_name_utf8() << nl);
	LOG("Inlining for method: " << M->get_name_utf8() << nl);

	// LOG("BEFORE" << nl);
	// print_all_nodes(M);
	EverythingPossibleHeuristic heuristic;
    List<INVOKEInst*> to_remove;
	auto is_invoke = [&](Instruction* i) { return i->to_INVOKEInst() != NULL; };
	auto i_iter = boost::make_filter_iterator(is_invoke, M->begin(), M->end());
	auto i_end = boost::make_filter_iterator(is_invoke, M->end(), M->end());
	for (;i_iter != i_end; i_iter++) {
		auto I = (INVOKEInst*) *i_iter;

		if (heuristic.should_inline(I)) {
			inline_instruction(I);
			STATISTICS(inlined_method_invocations++);
			to_remove.push_back(I->to_INVOKEInst());
		}
	}

	LOG("Removing all invoke instructions" << nl);
	for(auto it = to_remove.begin(); it != to_remove.end(); it++){
		auto call_site = *it;
		remove_call_site(call_site);
	}

	// LOG("AFTER" << nl);
	// print_all_nodes(M);

	LOG("End of inlining pass." << nl);

	return true;
}

void InliningPass::inline_instruction(INVOKEInst* I)
{
	CodeFactory codeFactory;
	LOG("Inlining invoke instruction " << I << nl);
	auto callee_method = codeFactory.create_ssa(I);
	LOG("Successfully retrieved SSA-Code for instruction " << nl);
	if (callee_method.get_Method()->bb_size() == 1){
    	SingleBBInliningOperation(I, callee_method.get_Method()).execute();
	} else {
    	ComplexInliningOperation(I, callee_method.get_Method()).execute();
	}
}

// pass usage
PassUsage& InliningPass::get_PassUsage(PassUsage& PU) const
{
	// TODO inlining: better way?
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
