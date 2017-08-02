/* src/vm/jit/compiler2/Matcher.cpp - Matcher class implementation

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
#include <string>
#include "toolbox/logging.hpp"

#include "Matcher.hpp"
#include "Instructions.hpp"

#define DEBUG_NAME "compiler2/Matcher"

#define OP_LABEL(p) ((p)->get_opcode())
#define LEFT_CHILD(p) (getOperand(p, 0))
#define RIGHT_CHILD(p) (getOperand(p, 1))
#define STATE_LABEL(p) (state_labels[p])

namespace cacao {
namespace jit {
namespace compiler2{

// No Instruction class, to be used as wildcard in pattern matching
class NoInst : public Instruction {
public:
	explicit NoInst(BeginInst* begin) : Instruction(Instruction::NoInstID, Type::VoidTypeID, begin) {}
	virtual NoInst* to_NoInst() { return this; }
	virtual void accept(InstructionVisitor& v, bool copyOperands) { 
		os::abort("Wildcard Instruction does not support Visitor");
	}
};

#define GENSTATIC
#include "Grammar.inc"

// TODO do this somewhat less hacky
Instruction::InstID tmp[] = {
		#include "GrammarExcludedNodes.inc"
};
const Matcher::ExcludeSetTy  Matcher::excluded_nodes (tmp, tmp + sizeof(tmp)/ sizeof(tmp[0]));

#define GENMETHODS
#include "Grammar.inc"

void Matcher::run(){
	findRoots();
	LOG("Performing pattern matching" << nl);
	for (DependencyMapTy::const_iterator i = roots.begin(), e = roots.end(); i != e; ++i){
		Instruction* inst = i->first;
		LOG("Handling root node " << inst << nl);	
		if (checkIsNodeExcluded(inst)){
			LOG("Result: root is excluded node, must be handled manually" << nl);
			// set dependencies usually done in label pass
			for (Instruction::const_op_iterator it = inst->op_begin(), e = inst->op_end(); it != e; ++it){
				Instruction* _inst = (*it)->to_Instruction();
				if (roots.find(_inst) != roots.end()){
					roots[inst]->insert(_inst);
					revdeps[_inst]->insert(inst);
				}
			}
		} else {
			burm_label(inst, inst);
			LOG("Result: " << nl);
			dumpCover(inst, 1, 0);
		}
	}
	scheduleTrees();
}

// matching 
void Matcher::findRoots(){
	LOG("Determining tree root nodes" << nl);
	// Collect all instructions within basic block
	for (GlobalSchedule::const_inst_iterator _i = sched->inst_begin(basicBlock), 
		 _e = sched->inst_end(basicBlock); _i != _e; ++_i){
		Instruction* inst = *_i;
		// skip begin inst. as they're not part of matching
		if (inst->to_BeginInst() ){
			continue;
		}
		// skip begin/end/phi inst. as they're not part of matching
		if (inst->to_PHIInst()){
			phiInst.insert(inst);
			continue;
		}
		inst_in_bb.insert(inst);
	}

	for (InstSetTy::const_iterator it = inst_in_bb.begin(), 
		 e = inst_in_bb.end(); it != e; ++it){
		Instruction* inst = *it;

		// if excluded, handle manually, all ops are roots
		if (checkIsNodeExcluded(inst)){
			LOG("Found root: " << inst << " Reason: excluded node" << nl);
			roots[inst] = std::shared_ptr<InstSetTy>(new InstSetTy());
			revdeps[inst] = std::shared_ptr<InstSetTy>(new InstSetTy());
			continue;
		}

		// consider inst with multiple users
		if (inst->user_size() > 1){
			LOG("Found root: " << inst << " Reason: multiple users" << nl);
			roots[inst] = std::shared_ptr<InstSetTy>(new InstSetTy());
			revdeps[inst] = std::shared_ptr<InstSetTy>(new InstSetTy());
			continue;
		}

		// if inst has no users
		if (inst->user_size() == 0){
			LOG("Found root: " << inst << " Reason: no users" << nl);
			roots[inst] = std::shared_ptr<InstSetTy>(new InstSetTy());
			// not necessarily needed
			revdeps[inst] = std::shared_ptr<InstSetTy>(new InstSetTy());
			continue;
		}

		// if users are outside of basic block
		Instruction *user = *(inst->user_begin());
		if (inst_in_bb.find(user) == inst_in_bb.end()){
			LOG("Found root: " << inst << " Reason: users in another basic block" << nl);

			roots[inst] = std::shared_ptr<InstSetTy>(new InstSetTy());
			// not necessarily needed
			revdeps[inst] = std::shared_ptr<InstSetTy>(new InstSetTy());
			continue;
		}

		if (checkIsNodeExcluded(user)){
			LOG("Found root: " << inst << " Reason: used by excluded node" << nl);
			roots[inst] = std::shared_ptr<InstSetTy>(new InstSetTy());
			revdeps[inst] = std::shared_ptr<InstSetTy>(new InstSetTy());
			continue;
		}
	}
	LOG("Found " << roots.size() << " roots" << nl);
}

bool Matcher::checkIsNodeExcluded(Instruction* inst){
	return excluded_nodes.find(inst->get_opcode()) != excluded_nodes.end();
}

Instruction* Matcher::getOperand(Instruction* op, unsigned pos){
	// always use proxy, if it already exists
	if (instrProxies.find(op) != instrProxies.end()){
		std::shared_ptr<ProxyMapByInstTy> proxyops = instrProxies[op];
		if (proxyops->find(pos) != proxyops->end()){
			std::shared_ptr<Instruction> proxy = (proxyops->find(pos)->second);
			return &*proxy;
		}
	}

	bool dep = false;
	bool no_operand = (pos >= op->op_size());
	Instruction* operand = NULL;

	if (!no_operand){
		// if no proxy available, get the instruction that is operand
		operand = op->get_operand(pos)->to_Instruction();
		// if its a dependency btw. subtrees of the BB
		if (roots.find(operand) != roots.end())
			dep = true;
	}

	// if the tree requires a proxy, create and return it
	// also track tree dependencies
	// no_operand has to be checked first to avoid accessing null pointers
	if (no_operand || dep || 
		(inst_in_bb.find(operand) == inst_in_bb.end()) ||
		(excluded_nodes.find(operand->get_opcode()) != excluded_nodes.end()) ){
		return createProxy(op, pos, operand, dep);
	}
	// return actual operand
	return operand;
}

Instruction* Matcher::createProxy(Instruction* op, unsigned pos, Instruction* operand, bool dependency){
	// create map for operator, if it does not exist
	if (instrProxies.find(op) == instrProxies.end()){
		instrProxies[op] = std::shared_ptr<ProxyMapByInstTy>(new ProxyMapByInstTy());
	}
	std::shared_ptr<ProxyMapByInstTy> proxyops = instrProxies[op];

	// create proxy
	std::shared_ptr<Instruction> proxy = std::shared_ptr<Instruction>(new NoInst(op->get_BeginInst()));
	(*proxyops)[pos] = proxy;
	LOG("Creating proxy: " << op << " op " << pos << " (= " << operand << ")" << nl);

	// add dependency btw. trees
	if (dependency){
		Instruction* root = STATE_LABEL(op)->root;
		roots[root]->insert(operand);
		revdeps[operand]->insert(root);
		LOG("Proxied operand is dependency for " << root << nl);
	}
	// return proxy op
	return &*proxy;
}

// debug output
void Matcher::dumpCover(Instruction* p, int goalnt, int indent) {
	int eruleno = burm_rule(STATE_LABEL(p), goalnt);
	const short *nts = burm_nts[eruleno];
	Instruction* kids[10];
	int i;

	std::string indentation;
	for (i = 0; i < indent; i++)
		indentation += " ";
	LOG(indentation << burm_string[eruleno] << "  (Rule: " << eruleno << " Root: " << p << nl);
	burm_kids(p, eruleno, kids);
	for (i = 0; nts[i]; i++)
		dumpCover(kids[i], nts[i], indent + 1);
}

void Matcher::printDependencies(Instruction* inst){
		LOG("root: " << inst << nl);
		LOG("root deps(" << roots[inst]->size() << "):" << nl);
		for (InstSetTy::const_iterator it = roots[inst]->begin(), e = roots[inst]->end(); it != e; ++it){
			LOG(*it << nl);
		}
		LOG("revdeps(" << revdeps[inst]->size() << "):" << nl);
		for (InstSetTy::const_iterator it = revdeps[inst]->begin(), e = revdeps[inst]->end(); it != e; ++it){
			LOG(*it << nl);
		}
}

// scheduling and lowering
void Matcher::scheduleTrees(){
	LOG("Scheduling:" << nl);
	LOG("Scheduling Begin Instruction " << basicBlock << nl);
	basicBlock->accept(*LV, true);

	LOG("Scheduling Phi Instructions:" << nl);
	for (InstSetTy::const_iterator i = phiInst.begin(), e = phiInst.end();
		 i != e; ++i){
		LOG((*i) << nl);
		(*i)->accept(*LV, true);
	}

	LOG("Dependencies btw. roots" << nl);
	for (DependencyMapTy::const_iterator i = roots.begin(), e = roots.end(); i != e; ++i){
		printDependencies(i->first);
	}

	LOG("Scheduling Trees:" << nl);
	while (roots.begin() != roots.end()){
		LOG(roots.size() << " trees left to schedule" << nl);

		Instruction* candidate=NULL;
		for (DependencyMapTy::const_iterator i = roots.begin(), e = roots.end(); i != e; ++i){
			// skip if it has deps
			if (i->second->size() > 0) continue;
			// set candidate, if no candidate is set yet
			if (!candidate) {
				if (i->first->to_EndInst() && (roots.size() > 1)) continue;
				candidate = i->first;
				continue;
			}
			// set candidate, if root has more dependants
			if (revdeps[candidate]->size() < revdeps[i->first]->size()){
				candidate = i->first;
			}
		}

		LOG("Scheduling " << candidate << nl);
		lowerTree(candidate, 1);

		// remove scheduled node from datastructures
		for (DependencyMapTy::const_iterator i = roots.begin(), e = roots.end(); i != e; ++i){
			i->second->erase(candidate);
		}
		roots.erase(candidate);
		revdeps.erase(candidate);
	}
}

void Matcher::lowerTree(Instruction* p, int goalnt){
	if (checkIsNodeExcluded(p)){
		LOG("Excluded node is lowered manually" << nl);
		p->accept(*LV, true);
		return;
	}
	int eruleno = burm_rule(STATE_LABEL(p), goalnt);
	const short *nts = burm_nts[eruleno];
	Instruction* kids[10];
	burm_kids(p, eruleno, kids);
	for (int i = 0; nts[i]; i++)
		lowerTree(kids[i], nts[i]);
	lowerRule(p, (RuleId) eruleno);
}

void Matcher::lowerRule(Instruction* inst, RuleId id){
	if (burm_isinstruction[id]){
		if (inst->get_opcode() == Instruction::NoInstID){
			LOG(inst << " is wildcard, no lowering required" << nl);
		} else {
			LOG("Rule " << burm_templates[id] << " is basic Instruction, using LoweringVisitor" << nl);
			inst->accept(*LV, STATE_LABEL(inst)->isLeaf);
		}
	} else {
		LOG("Lowering " << inst << " with rule " << burm_templates[id] << nl);
		LV->lowerComplex(inst, id);
	}
}

}
}
}

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
