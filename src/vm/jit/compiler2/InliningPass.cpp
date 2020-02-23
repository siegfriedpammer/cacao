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
#include "vm/jit/compiler2/HIRManipulations.hpp"
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

class Heuristic {
	private: 
		bool is_currently_monomorphic(INVOKEInst* I){
			return I->get_fmiref()->p.method->flags & ACC_METHOD_MONOMORPHIC;
		}
	protected: 
		Heuristic(){}
		bool can_inline(INVOKEInst* I){
			bool is_monomorphic_call = I->get_opcode() == Instruction::INVOKESTATICInstID ||
									   I->get_opcode() == Instruction::INVOKESPECIALInstID;

			bool is_polymorphic_call = I->get_opcode() == Instruction::INVOKEVIRTUALInstID ||
									   I->get_opcode() == Instruction::INVOKEINTERFACEInstID;

			if(!(is_monomorphic_call || (is_polymorphic_call && is_currently_monomorphic(I)))) return false;

			auto source_method = I->get_Method();
			auto target_method = I->to_INVOKEInst()->get_fmiref()->p.method;
			auto is_recursive_call = source_method->get_class_name_utf8() == target_method->clazz->name &&
									source_method->get_name_utf8() == target_method->name &&
									source_method->get_desc_utf8() == target_method->descriptor;
			return !is_recursive_call;
		}
	public:
		virtual bool has_next() = 0;
		virtual INVOKEInst* next() = 0;
		virtual void on_new_instruction(Instruction* instruction) = 0;
};

class EverythingPossibleHeuristic : public Heuristic {
	private:
		List<INVOKEInst*> work_list;
		Method* M;
	public:
		EverythingPossibleHeuristic(Method* method) : M(method){
			auto is_invoke = [&](Instruction* i) { return i->to_INVOKEInst() != NULL && can_inline(i->to_INVOKEInst()); };
			auto i_iter = boost::make_filter_iterator(is_invoke, M->begin(), M->end());
			auto i_end = boost::make_filter_iterator(is_invoke, M->end(), M->end());
			
			for (;i_iter != i_end; i_iter++) {
				work_list.push_back((INVOKEInst*) *i_iter);
			}
		}

		bool has_next(){
			return work_list.size() > 0;
		}

		INVOKEInst* next(){
			auto result = work_list.front();
			work_list.pop_front();
			return result;
		}

		void on_new_instruction(Instruction* instruction){
			auto invoke_inst = instruction->to_INVOKEInst();
			if(invoke_inst != NULL && can_inline(invoke_inst)){
				work_list.push_back(invoke_inst);
			}
		}
};

class LimitedBreathFirstHeuristic : public Heuristic {
	private:
		List<INVOKEInst*> work_list;
		Method* M;
		int max_size;
		int current_size;
		

		bool is_within_budget(INVOKEInst* instruction){
			// Only estimate size to limit the number of methods which need compiler2 compilation
			int estimated_size = instruction->get_fmiref()->p.method->jcodelength;
			return current_size + estimated_size <= max_size;
		}

		bool should_inline(Instruction* I){
			auto invoke_inst = I->to_INVOKEInst();
			return invoke_inst != NULL && can_inline(invoke_inst) && is_within_budget(invoke_inst);
		}
	public:
		LimitedBreathFirstHeuristic(Method* method, int max_size) : M(method), max_size(max_size) {
			current_size = M->size();
			auto is_invoke = [&](Instruction* i) { return should_inline(i); };
			auto i_iter = boost::make_filter_iterator(is_invoke, M->begin(), M->end());
			auto i_end = boost::make_filter_iterator(is_invoke, M->end(), M->end());
			
			LOG("LimitedBreathFirstHeuristic: Instruction in the heuristic work list (max: " << max_size << ")." << nl);
			for (;i_iter != i_end; i_iter++) {
				auto I = (INVOKEInst*) *i_iter;
				LOG("LimitedBreathFirstHeuristic: Adding " << I << nl);
				work_list.push_back(I);
			}
		}

		bool has_next(){
			current_size = M->size();
			auto found_instruction_within_budget = false;
			while(work_list.size() > 0 && !found_instruction_within_budget){
				auto next = work_list.front();
				if (is_within_budget(next)){
					found_instruction_within_budget = true;
				} else {
					LOG("Skipping " << next << " as it does not fit in the budget (current: " << current_size << ", max: " << max_size << ")");
					work_list.pop_front();
				}
			}
			return work_list.size() > 0;
		}

		INVOKEInst* next(){
			auto result = work_list.front();
			work_list.pop_front();
			return result;
		}

		void on_new_instruction(Instruction* instruction){
			if (should_inline(instruction)) {
				work_list.push_back((INVOKEInst*) instruction);
			}
		}
};

class InliningOperationBase {
	protected: 
		INVOKEInst* call_site;
		Method* caller_method;
		Method* callee_method;
		Heuristic* heuristic;

		InliningOperationBase (INVOKEInst* site, Method* callee, Heuristic* heuristic) : call_site(site), callee_method(callee), heuristic(heuristic){
			caller_method = call_site->get_Method();
		}
		
		static bool is_source_state_or_begin_inst (Instruction* I){
			auto op_code = I->get_opcode();
			return op_code != Instruction::SourceStateInstID && op_code != Instruction::BeginInstID;
		}

		static bool is_state_change_for_other_instruction (Instruction* I){
			return std::any_of(I->rdep_begin(), I->rdep_end(), InliningOperationBase::is_source_state_or_begin_inst);
		}

		static Instruction* get_depending_instruction (Instruction* I){
			return *std::find_if(I->rdep_begin(), I->rdep_end(), InliningOperationBase::is_source_state_or_begin_inst);
		}

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

		void on_inlined_inst(Instruction* instruction){
			this->heuristic->on_new_instruction(instruction);
		}
};

class SingleBBInliningOperation : InliningOperationBase {
private:
	BeginInst* call_site_bb;
	BeginInst* caller_bb;

	bool needs_null_check(){
		return call_site->to_INVOKESTATICInst() == NULL;
	}

	void correct_scheduling_edges(Instruction* I){
		if(!I->has_side_effects()) return;

		auto state_change = I->get_last_state_change();
		// If the last state change points to the basic block, then there is no state changing instruction
		// before this instruction in this bb. Therefore the new state change has to be the last state changing
		// instruction before the initial call site (or the new bb).
		if (state_change->to_BeginInst()) {
			Instruction* first_dependency_before_inlined_region = *std::next(call_site->dep_begin(), 1);
			LOG("Setting last state change for " << I << " to " << first_dependency_before_inlined_region << nl);
			I->replace_state_change_dep(first_dependency_before_inlined_region);
		}

		// If the call site is the last state change for an instruction, this instruction now needs to depend
		// on the last state changing instruction of the inlined region, or the state change before the invocation.
		if(is_state_change_for_other_instruction(call_site) && !is_state_change_for_other_instruction(I)){
			auto first_dependency_after_inlined_region = get_depending_instruction (call_site);
			LOG("Setting last state change for " << first_dependency_after_inlined_region << " to " << I << nl);
			first_dependency_after_inlined_region->replace_state_change_dep(I);
		}
	}

	void add_call_site_bbs()
	{
		LOG("add_call_site_bbs" << nl);
		List<Instruction*> to_remove;
		
		Instruction* null_check_inst = NULL;
		if(needs_null_check()){
			auto is_null_check_inst = [](Instruction* i) { return i->get_opcode() == Instruction::CHECKNULLInstID; };
			auto null_check_inst_it = std::find_if(call_site->dep_begin(), call_site->dep_end(), is_null_check_inst);
			if(null_check_inst_it != call_site->dep_end()) {
				null_check_inst = *null_check_inst_it;
				LOG("Null check inst " << null_check_inst << nl);
				call_site_bb->get_EndInst()->append_dep(null_check_inst);
			}
		}

		for (auto it = callee_method->begin(); it != callee_method->end(); it++) {
			Instruction* I = *it;
			LOG("Adding to call site " << *I << nl);

			if(I->get_opcode() == Instruction::BeginInstID){
				to_remove.push_back(I);
				continue; // ignore begin instruction
			} else if(I->get_opcode() == Instruction::RETURNInstID){
				auto returnInst = I->to_RETURNInst();
				LOG("return operation " << returnInst << " with op size " << returnInst->op_size() << nl);
				if(returnInst->op_size() > 0){
					auto result = *(returnInst->op_begin());
					call_site->replace_value(result);
					LOG("result: " << result << nl);
				}
				to_remove.push_back(returnInst);
				continue;
			}

			caller_method->add_Instruction(I);
			I->replace_dep(caller_bb, call_site_bb);
			I->set_BeginInst_unsafe(call_site_bb);

			correct_scheduling_edges(I);

			if(null_check_inst != NULL){
				I->append_dep(null_check_inst);
			}

			on_inlined_inst(I);
		}

		for (auto it = to_remove.begin(); it != to_remove.end(); it++) {
			remove_instruction(*it);
		}
	}

public:
	SingleBBInliningOperation(INVOKEInst* site, Method* callee, Heuristic* heuristic) : InliningOperationBase(site, callee, heuristic)
	{
		call_site_bb = call_site->get_BeginInst();
		caller_bb = callee->get_init_bb();
	}

	virtual void execute()
	{
		LOG("Inserting new basic blocks into original method" << nl);
		add_call_site_bbs();
		replace_method_parameters();
	}
};

class ComplexInliningOperation : InliningOperationBase {
	private:
		BeginInst* old_call_site_bb;
		BeginInst* pre_call_site_bb;
		BeginInst* post_call_site_bb;
		PHIInst* phi;

		bool needs_phi(){
			return call_site->get_type() != Type::VoidTypeID;
		}

		Instruction* transform_instruction(Instruction* I)
		{
			LOG("Transforming " << I << nl);
			switch (I->get_opcode()) {
				case Instruction::RETURNInstID:{
					auto begin_inst = I->get_BeginInst();
					auto end_instruction = new GOTOInst(begin_inst, post_call_site_bb);
					begin_inst->set_EndInst(end_instruction);
					LOG("Rewriting return instruction " << I << " to " << end_instruction << nl);
					// TODO inlining: delete I
					return end_instruction;
				}
				default:
					return I;
			}
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
				LOG("Adding to call site " << I << nl);

				if(I->get_opcode() == Instruction::RETURNInstID){
					auto returnInst = I->to_RETURNInst();
					if(needs_phi())
						phi->append_op(*(returnInst->op_begin()));
				}

				auto new_inst = transform_instruction(I);
				HIRManipulations::move_instruction_to_method(new_inst, caller_method);
				on_inlined_inst(new_inst);
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
		ComplexInliningOperation(INVOKEInst* site, Method* callee, Heuristic* heuristic) : InliningOperationBase(site, callee, heuristic)
		{
			old_call_site_bb = call_site->get_BeginInst();
		}

		virtual void execute()
		{
			LOG("Inserting new basic blocks into original method" << nl);
			pre_call_site_bb = old_call_site_bb;
			post_call_site_bb = HIRManipulations::split_basic_block(pre_call_site_bb, call_site);
			add_call_site_bbs();
			replace_method_parameters();
			wire_up_call_site_with_post_call_site_bb();
		}
};

class CodeFactory {
	public:
		virtual ~CodeFactory(){}
		virtual JITData create_ssa(INVOKEInst* I){
			auto callee = I->get_fmiref()->p.method;
			auto* jd = jit_jitdata_new(callee);
			jit_jitdata_init_for_recompilation(jd);
			JITData JD(jd);

			PassRunner runner;
			runner.runPassesUntil<SSAConstructionPass>(JD);
			return JD;
		}
};

class GuardedCodeFactory : public CodeFactory {
	private:
		Method* target_method;

		BeginInst* create_false_branch(INVOKEInst* I){
			auto new_bb = new BeginInst();
			target_method->add_bb(new_bb);
			new_bb->append_dep(I);
			auto source_state = I->get_SourceStateInst();
			source_state->replace_dep(I->get_BeginInst(), new_bb);
			I->set_BeginInst_unsafe(new_bb);
			
			auto return_inst = new RETURNInst(new_bb, I);
			target_method->add_Instruction(return_inst);

			return new_bb;
		}

		BeginInst* create_guard(INVOKEInst* I){
			auto true_branch = target_method->get_init_bb();
			auto false_branch = create_false_branch(I);
			
			// TODO inlining: Can we get the current class, or not?
			auto check_bb = new BeginInst(); 
			auto current_object = I->get_operand(0);
			// TODO inlining: get the adress of the initial object
			auto if_inst = new IFInst(check_bb, current_object, current_object, Conditional::EQ, true_branch, false_branch);

			check_bb->set_EndInst(if_inst);
			target_method->add_bb(check_bb);
			target_method->add_Instruction(if_inst);

			return check_bb;
		}
	public:
		JITData create_ssa(INVOKEInst* I){
			auto JD = CodeFactory::create_ssa(I);
			target_method = JD.get_Method();
			auto new_init_bb = create_guard(I);
			target_method->set_init_bb(new_init_bb);
			return JD;
		}
};

void remove_call_site(INVOKEInst* call_site){
	// append source state to bb (workaround)
	auto is_source_state = [](Instruction* I){ return I->get_opcode() == Instruction::SourceStateInstID;};
	auto source_state_it = std::find_if(call_site->rdep_begin(), call_site->rdep_end(), is_source_state);
	if(source_state_it != call_site->rdep_end()){
		auto source_state = *source_state_it;
		LOG("Appending source state " << source_state << " to bb " << call_site->get_BeginInst() << nl);
		source_state->to_SourceStateInst()->replace_dep(call_site, call_site->get_BeginInst());
	}
	
	remove_instruction(call_site);
}

CodeFactory* get_code_factory(INVOKEInst *I){
	switch (I->get_opcode())
	{
	case Instruction::INVOKESTATICInstID:
	case Instruction::INVOKESPECIALInstID:
		LOG("Using normal code factory");
		return new CodeFactory();
	case Instruction::INVOKEVIRTUALInstID:
	case Instruction::INVOKEINTERFACEInstID:
		LOG("Using guarded code factory");
		return new GuardedCodeFactory();
	default:
		LOG("Cannot get code factory for " << I << nl);
		throw std::runtime_error("Unsupported invoke instruction!");
	}
}

void inline_instruction(INVOKEInst* I, Heuristic* heuristic)
{
	auto codeFactory = get_code_factory(I);
	LOG("Inlining invoke instruction " << I << nl);
	auto callee_method = codeFactory->create_ssa(I);
	delete codeFactory;
	LOG("Successfully retrieved SSA-Code for instruction " << nl);
	if (callee_method.get_Method()->bb_size() == 1){
    	SingleBBInliningOperation(I, callee_method.get_Method(), heuristic).execute();
	} else {
    	ComplexInliningOperation(I, callee_method.get_Method(), heuristic).execute();
	}
}

bool InliningPass::run(JITData& JD)
{
	LOG("Start of inlining pass." << nl);
	Method* M = JD.get_Method();
	LOG("Inlining for class: " << M->get_class_name_utf8() << nl);
	LOG("Inlining for method: " << M->get_name_utf8() << nl);

	LimitedBreathFirstHeuristic heuristic(M, 100);
    List<INVOKEInst*> to_remove;
	while(heuristic.has_next()){
		auto I = heuristic.next();
		inline_instruction(I, &heuristic);
		auto op_code = I->get_opcode();
		// don't delete guarded instructions
		if(op_code == Instruction::INVOKESTATICInstID || op_code == Instruction::INVOKESPECIALInstID){
			to_remove.push_back(I);
		}
		STATISTICS(inlined_method_invocations++);
	}

	LOG("Removing all invoke instructions" << nl);
	for(auto it = to_remove.begin(); it != to_remove.end(); it++){
		auto call_site = *it;
		auto op_code = call_site->get_opcode();
		remove_call_site(call_site);
	}

	LOG("End of inlining pass." << nl);

	return true;
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
