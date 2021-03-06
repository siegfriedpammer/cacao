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

#include "boost/iterator/filter_iterator.hpp"
#include "vm/jit/compiler2/HIRManipulations.hpp"
#include "vm/jit/compiler2/HIRUtils.hpp"
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

#include "toolbox/logging.hpp"

// define name for debugging (see logging.hpp)
#define DEBUG_NAME "compiler2/InliningPass"

#define GUARDED_INLINING 0
// this value serves as a hard limit for heuristics which tend to inline very small methods, even though the budget is exceeded.
#define MAXIMUM_METHOD_SIZE 250

STAT_DECLARE_GROUP(compiler2_stat)
STAT_REGISTER_SUBGROUP(compiler2_inliningpass_stat, "inliningpass", "inliningpass", compiler2_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,
                        inlined_method_invocations,
                        0,
                        "# of inlined method invocations",
                        "number of inlined method invocations",
                        compiler2_inliningpass_stat)

namespace cacao {
namespace jit {
namespace compiler2 {

namespace option {
Option<const char*> op_heuristic("InliningHeuristic",
                                 "compiler2: selects the heurstic which is used for inlining "
                                 "(Everything, BreadthFirst, Knapsack)",
                                 "Knapsack",
                                 ::cacao::option::xx_root());
Option<unsigned int> op_knapsack_budget("KnapsackBudget",
                                "compiler2: budget for knapsack heuristic",
                                100,
                                ::cacao::option::xx_root());
Option<unsigned int> op_breadth_first_method_size(
    "BreathFirstMethodSize",
    "compiler2: target method size for limited breadth first heuristik",
    100,
    ::cacao::option::xx_root());
} // namespace options

/*
 *   IMPORTANT: The current implementation does not work with on-stack replacement. Therefore the
 *   InliningPass should not be opted in, when testing or working with deoptimization and
 *   on-stack replacement. The reason for this is, that the SourceStateInstructions are not handled
 *   correctly.
 *
 *   TODO The following parts of the problem may require a correction of source states.
 *   - GuardedCodeFactory
 *   -- The then block could require changes
 *   -- The else block definetely require changes, as only a workaround source_state is created
 *   - InliningPass
 *   -- Inlining any instruction could need changes (merge source states?)
 *   -- Replacing a LoadInst could require changes
 *   -- Adding a new phi node could require changes
 *   -- Removing the invoke instructions could require changes
 *   - Coalesce Basic Blocks
 *   -- Merging two basic blocks could require cahnges
 */
SourceStateInst* create_work_around_source_state(Instruction* for_inst)
{
	LOG2("Creating work around source state for " << for_inst << nl);
	auto source_state = new SourceStateInst(123, for_inst);
	for_inst->get_Method()->add_Instruction(source_state);
	LOG2("Work around source state created" << nl);
	return source_state;
}

void ensure_source_state(Instruction* for_inst)
{
	auto source_state_it =
	    std::find_if(for_inst->rdep_begin(), for_inst->rdep_end(), [](Instruction* i) {
		    return i->get_opcode() == Instruction::SourceStateInstID;
	    });
	if (source_state_it == for_inst->rdep_end()) {
		create_work_around_source_state(for_inst);
	}
}

class Heuristic {
private:
	inline bool is_monomorphic_and_implemented(INVOKEInst* I)
	{
		auto flags = I->get_fmiref()->p.method->flags;
		// Monomorphic and implemented ensured that the implementation of the method
		// can be found under the given fmi ref.
		return (flags & ACC_METHOD_MONOMORPHIC) && (flags & ACC_METHOD_IMPLEMENTED);
	}

	inline bool is_abstract(INVOKEInst* I)
	{
		return I->get_fmiref()->p.method->flags & ACC_ABSTRACT;
	}

protected:
	Method* M;

	Heuristic(Method* method) : M(method)
	{
	}

	bool can_inline(INVOKEInst* I)
	{
		bool is_monomorphic_call = I->get_opcode() == Instruction::INVOKESTATICInstID ||
		                           I->get_opcode() == Instruction::INVOKESPECIALInstID;

#if (GUARDED_INLINING)
		bool is_polymorphic_call = I->get_opcode() == Instruction::INVOKEVIRTUALInstID;
		// Searching for the implementation of an abstract method is not supported at the moment.
		bool is_inlineable_polymorphic_call =
		    is_polymorphic_call && is_monomorphic_and_implemented(I) && !is_abstract(I);
#else
		bool is_inlineable_polymorphic_call = false;
#endif

		return (is_monomorphic_call || is_inlineable_polymorphic_call) &&
		       I->get_Method()->size() <= MAXIMUM_METHOD_SIZE;
	}

	virtual bool should_inline(INVOKEInst* I) = 0;

	virtual size_t size() = 0;

	virtual INVOKEInst* front() = 0;

	virtual void pop() = 0;

	virtual void add_inst(INVOKEInst* inst) = 0;

public:

	void initialize(){
		auto is_invoke = [&](Instruction* i) {
			return i->to_INVOKEInst() != NULL && can_inline(i->to_INVOKEInst());
		};
		auto i_iter = boost::make_filter_iterator(is_invoke, M->begin(), M->end());
		auto i_end = boost::make_filter_iterator(is_invoke, M->end(), M->end());

		for (; i_iter != i_end; i_iter++) {
			add_inst((INVOKEInst*)*i_iter);
		}
	}

	bool has_next()
	{
		auto found_instruction_within_budget = false;
		while (size() > 0 && !found_instruction_within_budget) {
			auto next = front();
			if (should_inline(next)) {
				found_instruction_within_budget = true;
			}
			else {
				LOG("Heuristic: Skipping " << next << nl);
				pop();
			}
		}
		return size() > 0;
	}

	INVOKEInst* next()
	{
		auto result = front();
		pop();
		return result;
	}

	void on_new_instruction(Instruction* instruction)
	{
		auto invoke = instruction->to_INVOKEInst();
		if (invoke != NULL && can_inline(invoke) && should_inline(invoke)) {
			add_inst((INVOKEInst*)instruction);
		}
	}
};

/*
 *	This Heuristic will inline all possible call sites. This should only be used in testing
 *  scenarios.
 */
class EverythingPossibleHeuristic : public Heuristic {
private:
	List<INVOKEInst*> work_list;

protected:

	virtual bool should_inline(INVOKEInst* I) {
		return true;
	}

	virtual size_t size() {
		return work_list.size();
	};

	virtual INVOKEInst* front() {
		return work_list.front();
	}

	virtual void pop() {
		work_list.pop_front();
	};

	virtual void add_inst(INVOKEInst* inst) {
		work_list.push_back(inst);
	};

public:
	EverythingPossibleHeuristic(Method* method) : Heuristic(method)
	{
	}
};

/*
 *	Iterates over all instructions in a FIFO fashion. Inlining stops when the max method size is
 *	exceeded.
 */
class LimitedBreadthFirstHeuristic : public Heuristic {
private:
	List<INVOKEInst*> work_list;
	size_t max_size;

	bool is_within_budget(INVOKEInst* instruction)
	{
		// Only estimate size to limit the number of methods which need compiler2 compilation
		int estimated_size = instruction->get_fmiref()->p.method->jcodelength;
		return M->size() + estimated_size <= max_size;
	}

protected:

	virtual bool should_inline(INVOKEInst* I) {
		return is_within_budget(I);
	}

	virtual size_t size() {
		return work_list.size();
	};

	virtual INVOKEInst* front() {
		return work_list.front();
	}

	virtual void pop() {
		work_list.pop_front();
	};

	virtual void add_inst(INVOKEInst* inst) {
		work_list.push_back(inst);
	};

public:
	LimitedBreadthFirstHeuristic(Method* method, size_t max_size) : Heuristic(method), max_size(max_size)
	{
	}
};

/*
 *	Models the problem with the KNAPSACK problem. Every call site is assigned a benefit and a cost.
 *	The call sites are inlined acording to their priority (benefit / cost).
 */
class KnapsackHeuristic : public Heuristic {
private:
	static float getBenefit(INVOKEInst* invoke) { return 1; }

	static int getCost(INVOKEInst* invoke) { return invoke->get_fmiref()->p.method->jcodelength; }

	struct CompareInvokes {
		bool operator()(INVOKEInst* l, INVOKEInst* r)
		{
			float priorityLeft = getBenefit(l) / getCost(l);
			float priorityRight = getBenefit(r) / getCost(r);
			return priorityLeft > priorityRight;
		}
	};

	std::priority_queue<INVOKEInst*, std::vector<INVOKEInst*>, CompareInvokes> work_queue;
	Method* M;
	int budget;

	bool is_within_budget(INVOKEInst* instruction)
	{
		// Only estimate size to limit the number of methods which need compiler2 compilation
		int estimated_size = instruction->get_fmiref()->p.method->jcodelength;
		// guarantees that small methods get inlined.
		int offset = -5;
		return (offset + estimated_size) <= budget;
	}

protected:

	virtual bool should_inline(INVOKEInst* I) {
		return is_within_budget(I);
	}

	virtual size_t size() {
		return work_queue.size();
	};

	virtual INVOKEInst* front() {
		return work_queue.top();
	}

	virtual void pop() {
		work_queue.pop();
	};

	virtual void add_inst(INVOKEInst* inst) {
		work_queue.push(inst);
	};

public: 
	KnapsackHeuristic(Method* method, size_t budget) : Heuristic(method), budget(budget)
	{
	}
};

/*
 *	This class
 */
class InliningOperation {
private:
	INVOKEInst* call_site;
	Method* caller_method;
	Method* callee_method;
	Instruction* do_not_inline;
	Heuristic* heuristic;
	BeginInst* old_call_site_bb;
	BeginInst* pre_call_site_bb;
	BeginInst* post_call_site_bb;
	std::list<Instruction*> phi_operands;
	std::list<Instruction*> to_remove;

	void replace_method_parameters()
	{
		LOG3("InliningOperation: replace_method_parameters" << nl);
		List<Instruction*> to_remove_after_replace;
		for (auto it = callee_method->begin(); it != callee_method->end(); it++) {
			auto I = *it;
			if (I->get_opcode() == Instruction::LOADInstID) {
				auto index = (I->to_LOADInst())->get_index();
				auto given_operand = call_site->get_operand(index);
				LOG("InliningOperation: Replacing Load inst" << I << " with " << given_operand
				                                             << nl);
				HIRManipulations::replace_value_without_source_states(I, given_operand);
				to_remove_after_replace.push_back(I);
			}
		}
		for (auto it = to_remove_after_replace.begin(); it != to_remove_after_replace.end(); it++) {
			HIRManipulations::remove_instruction(*it);
		}
	}

	void on_inlined_inst(Instruction* instruction)
	{
		if (instruction != do_not_inline) {
			this->heuristic->on_new_instruction(instruction);
		}
	}

	bool needs_phi() { return call_site->get_type() != Type::VoidTypeID; }

	Instruction* transform_instruction(Instruction* I)
	{
		LOG3("InliningOperation: Transforming " << I << nl);
		switch (I->get_opcode()) {
			case Instruction::RETURNInstID: {
				auto return_inst = I->to_RETURNInst();
				if (return_inst->op_size() > 0) {
					auto operand = (*return_inst->op_begin())->to_Instruction();
					LOG("InliningOperation: Appending phi operand "
					    << operand << " in " << operand->get_BeginInst() << nl);
					phi_operands.push_back(operand);
				}
				auto begin_inst = I->get_BeginInst();
				auto end_instruction = new GOTOInst(begin_inst, post_call_site_bb);
				begin_inst->set_EndInst(end_instruction);
				LOG("InliningOperation: Rewriting return instruction " << I << " to "
				                                                       << end_instruction << nl);
				to_remove.push_back(I);
				return end_instruction;
			}
			default: return I;
		}
	}

	void replace_invoke_with_result()
	{
		LOG3("InliningOperation: replace_invoke_with_result for " << call_site << nl);
		// no phi needed, if there is only one return point
		if (phi_operands.size() == 1) {
			LOG("InliningOperation: Phi node not necessary" << nl);
			auto result = *phi_operands.begin();
			HIRManipulations::replace_value_without_source_states(call_site, result);
			return;
		}

		// the phi will be the replacement for the dependencies to the invoke inst in the post call
		// site bb
		LOG("InliningOperation: Phi node is necessary" << nl);
		auto return_type = call_site->get_type();
		auto phi = new PHIInst(return_type, post_call_site_bb);

		for (auto it = phi_operands.begin(); it != phi_operands.end(); it++) {
			LOG("InliningOperation: Registered operands: " << *it << " (" << (*it)->get_BeginInst()
			                                               << ")" << nl);
		}

		for (auto it = post_call_site_bb->pred_begin(); it != post_call_site_bb->pred_end(); it++) {
			auto bb = *it;
			LOG2("InliningOperation: Searching for phi operand in " << bb << nl);
			auto is_in_bb = [bb](Instruction* inst) { return inst->get_BeginInst() == bb; };
			auto is_in_no_bb = [](Instruction* inst) { return inst->get_BeginInst() == NULL; };
			auto next_op_it = std::find_if(phi_operands.begin(), phi_operands.end(), is_in_bb);
			if (next_op_it == phi_operands.end()) {
				next_op_it = std::find_if(phi_operands.begin(), phi_operands.end(), is_in_no_bb);
			}
			assert(next_op_it != phi_operands.end());
			auto next_op = *next_op_it;
			LOG("InliningOperation: Found operand " << next_op << nl);
			phi_operands.erase(next_op_it);
			phi->append_op(next_op);
		}

		LOG("InliningOperation: Adding phi " << phi << nl);
		caller_method->add_Instruction(phi);
		HIRManipulations::replace_value_without_source_states(call_site, phi);
	}

	void add_call_site_bbs()
	{
		LOG3("InliningOperation: add_call_site_bbs " << callee_method->get_name_utf8() << nl);
		for (auto it = callee_method->begin(); it != callee_method->end(); it++) {
			Instruction* I = *it;
			LOG2("InliningOperation: Adding to call site " << I << nl);
			auto new_inst = transform_instruction(I);
			LOG3("InliningOperation: transformed inst " << new_inst << nl);
			HIRManipulations::move_instruction_to_method(new_inst, caller_method);
			on_inlined_inst(new_inst);
		}

		LOG("InliningOperation: All basic blockes moved." << nl);

		if (needs_phi()) {
			replace_invoke_with_result();
		}
	}

public:
	InliningOperation(INVOKEInst* site,
	                  Method* callee,
	                  Instruction* do_not_inline,
	                  Heuristic* heuristic)
	    : call_site(site), callee_method(callee), do_not_inline(do_not_inline), heuristic(heuristic)
	{
		caller_method = call_site->get_Method();
		old_call_site_bb = call_site->get_BeginInst();
	}

	virtual void execute()
	{
		LOG("InliningOperation: Inserting new basic blocks into original method" << nl);
		pre_call_site_bb = old_call_site_bb;
		post_call_site_bb = HIRManipulations::split_basic_block(pre_call_site_bb, call_site);
		ensure_source_state(
		    post_call_site_bb); // this workaround ensures that there is a source_state
		add_call_site_bbs();
		replace_method_parameters();
		HIRManipulations::connect_with_jump(pre_call_site_bb, callee_method->get_init_bb());
		for (auto it = to_remove.begin(); it != to_remove.end(); it++) {
			HIRManipulations::remove_instruction(*it);
		}
	}
};

class CodeFactory {
public:
	virtual ~CodeFactory() {}

	/**
	 * Returns an instruction which should not be inlined. Used to avoid inlining
	 * guarded call sites.
	 */
	virtual Instruction* do_not_inline() { return NULL; }

	virtual JITData create_ssa(INVOKEInst* I)
	{
		auto callee = I->get_fmiref()->p.method;
		auto* jd = jit_jitdata_new(callee);
		jit_jitdata_init_for_recompilation(jd);
		JITData JD(jd);

		PassRunner runner;
		runner.runPassesUntil<SSAConstructionPass>(JD);
		return JD;
	}
};

#if (GUARDED_INLINING)
class GuardedCodeFactory : public CodeFactory {
private:
	Method* target_method;
	INVOKEInst* guarded_call;

	virtual Instruction* do_not_inline() { return guarded_call; }

	BeginInst* create_false_branch(INVOKEInst* I)
	{
		auto new_bb = new BeginInst();
		target_method->add_bb(new_bb);
		auto source_state = create_work_around_source_state(new_bb);
		auto source_state_after_call_site = create_work_around_source_state(new_bb);

		guarded_call = new INVOKEVIRTUALInst(I->get_type(), I->get_MethodDescriptor().size(),
		                                     I->get_fmiref(), new_bb, new_bb, source_state);
		source_state_after_call_site->append_dep(guarded_call);
		LOG("Created new call " << guarded_call << " within new basic block " << nl);
		target_method->add_Instruction(guarded_call);
		for (auto it = I->op_begin(); it != I->op_end(); it++) {
			auto op = *it;
			guarded_call->append_parameter(op);
		}

		auto return_inst = new RETURNInst(new_bb, guarded_call);
		target_method->add_Instruction(return_inst);

		return new_bb;
	}

	BeginInst* create_guard(INVOKEInst* I)
	{
		auto then_branch = target_method->get_init_bb();
		LOG3("GuardedCodeFactory: Then branch" << then_branch << nl);
		auto else_branch = create_false_branch(I);
		LOG3("GuardedCodeFactory: Created else branch " << else_branch << nl);

		/**
		 * TODO
		 * Currently, Compiler2 does not support invoking native methods such as
		 * java/lang/Object.getClass(). Therefore a "dummy" assertion (obj == obj) was introduced.
		 * After implementing native methods in Compiler2, this should be fixed.
		 * Alternatively another approach (e.g. new HIRInstruction) could be used.
		 */
		auto check_bb = new BeginInst();
		LOG2("GuardedCodeFactory: Created guard basic block " << check_bb << nl);
		auto current_object = I->get_operand(0);
		LOG2("GuardedCodeFactory: Receiver " << current_object << nl);

		auto if_inst = new IFInst(check_bb, current_object, current_object, Conditional::EQ,
		                          then_branch, else_branch);
		LOG("GuardedCodeFactory: Created guard " << if_inst << nl);

		check_bb->set_EndInst(if_inst);
		target_method->add_bb(check_bb);
		target_method->add_Instruction(if_inst);
		create_work_around_source_state(check_bb);

		LOG("GuardedCodeFactory: Added guard to target method" << nl);

		return check_bb;
	}

public:
	JITData create_ssa(INVOKEInst* I)
	{
		auto JD = CodeFactory::create_ssa(I);
		LOG2("GuardedCodeFactory: Retrieved code from base class." << nl);
		target_method = JD.get_Method();
		auto new_init_bb = create_guard(I);
		target_method->set_init_bb(new_init_bb);
		return JD;
	}
};
#endif

void remove_call_site(INVOKEInst* call_site)
{
	// append source state to bb (workaround)
	auto is_source_state = [](Instruction* I) {
		return I->get_opcode() == Instruction::SourceStateInstID;
	};
	auto source_state_it =
	    std::find_if(call_site->rdep_begin(), call_site->rdep_end(), is_source_state);
	if (source_state_it != call_site->rdep_end()) {
		auto source_state = *source_state_it;
		LOG2("Appending source state " << source_state << " to bb " << call_site->get_BeginInst()
		                               << nl);
		source_state->to_SourceStateInst()->replace_dep(call_site, call_site->get_BeginInst());
	}

	HIRManipulations::remove_instruction(call_site);
}

CodeFactory* get_code_factory(INVOKEInst* I)
{
	switch (I->get_opcode()) {
		case Instruction::INVOKESTATICInstID:
		case Instruction::INVOKESPECIALInstID:
			LOG("Using normal code factory" << nl);
			return new CodeFactory();
#if (GUARDED_INLINING)
		case Instruction::INVOKEVIRTUALInstID:
			LOG("Using guarded code factory" << nl);
			return new GuardedCodeFactory();
#endif
		default:
			LOG("Cannot get code factory for " << I << nl);
			throw std::runtime_error("Unsupported invoke instruction!");
	}
}

void inline_instruction(INVOKEInst* I, Heuristic* heuristic)
{
	std::unique_ptr<CodeFactory> code_factory(get_code_factory(I));
	LOG("Inlining invoke instruction " << I << nl);
	auto callee_method = code_factory->create_ssa(I);
	LOG3("Successfully retrieved SSA-Code for instruction " << nl);
	InliningOperation(I, callee_method.get_Method(), code_factory->do_not_inline(), heuristic)
	    .execute();
}

Heuristic* create_heuristic(Method* M)
{
	auto heuristic = option::op_heuristic.get();
	if (strcmp(heuristic, "Knapsack") == 0) {
		auto budget = option::op_knapsack_budget.get();
		LOG("Using KnapsackHeuristic(" << budget << ")" << nl);
		return new KnapsackHeuristic(M, budget);
	}
	else if (strcmp(heuristic, "BreadthFirst") == 0) {
		auto method_size = option::op_breadth_first_method_size.get();
		LOG("Using LimitedBreadthFirstHeuristic(" << method_size << ")" << nl);
		return new LimitedBreadthFirstHeuristic(M, method_size);
	}

	LOG("Using EverythingPossibleHeuristic" << nl);
	return new EverythingPossibleHeuristic(M);
}

bool InliningPass::run(JITData& JD)
{
	LOG("Start of inlining pass." << nl);
	M = JD.get_Method();

	LOG("Inlining for class: " << M->get_class_name_utf8() << nl);
	LOG("Inlining for method: " << M->get_name_utf8() << nl);

	std::unique_ptr<Heuristic> heuristic(create_heuristic(M));
	heuristic->initialize();
	List<INVOKEInst*> to_remove;
	while (heuristic->has_next()) {
		auto I = heuristic->next();
		inline_instruction(I, heuristic.get());
		to_remove.push_back(I);
		STATISTICS(inlined_method_invocations++);
	}

	LOG("Removing all invoke instructions" << nl);
	for (auto it = to_remove.begin(); it != to_remove.end(); it++) {
		auto call_site = *it;
		remove_call_site(call_site);
	}

	LOG("Invoking coalesce_basic_blocks." << nl);
	HIRManipulations::coalesce_basic_blocks(M);
	LOG("End of inlining pass." << nl);
	return true;
}

bool InliningPass::verify() const
{
	return HIRUtils::verify_all_instructions(M);
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
