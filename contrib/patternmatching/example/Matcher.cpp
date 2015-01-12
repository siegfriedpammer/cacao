/* src/vm/jit/compiler2/Matcher.cpp - Matcher class implementation and example

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
#include "Matcher.hpp"

#define LOG printf

namespace cacao {
namespace jit {
namespace compiler2{

#define GENSTATIC
#include "Grammar.inc"


// TODO do this somewhat less hacky
Instruction::InstID tmp[] = {
		#include "GrammarExcludedNodes.inc"
};

const std::set<Instruction::InstID>  Matcher::excluded_nodes (tmp, tmp + sizeof(tmp)/ sizeof(tmp[0]));


#define GENMETHODS
#include "Grammar.inc"

void Matcher::burm_trace(Instruction* p, int eruleno, int cost, int bestcost) {
	#ifdef TRACE
	//extern const char *burm_string[];

	fprintf(stderr, /*"0x%p */ "matched %s with cost %d vs. %d\n", /*p,*/
		burm_string[eruleno], cost, bestcost);
	#endif
}

void Matcher::run(){
	LOG("\n\n--------- finding roots and excluded nodes to prepare DAG for matching ----------\n");
	findRoots();

	LOG("*** %lu root nodes for matching detected\n", roots.size());


	LOG("\n\n----------- perform matching and output for every tree            ---------------\n");

	for (auto i = roots.begin(), e = roots.end(); i != e; ++i){

		Instruction* inst = i->first;

		LOG("\n\n--> handling root node with id %d\n", inst->get_id());	


		if (checkIsNodeExcluded(inst)){
			LOG("*** root is excluded node, skipping matching! Must be handled manually\n");
			// TODO: still needs dependency handling
			for (auto it = inst->op_begin(), e = inst->op_end(); it != e; ++it){
				if (roots.find(*it) != roots.end()){
					roots[inst]->insert(*it);
				}
			}
		} else {
			burm_label(inst);
			LOG("*** result:\n");
			dumpCover(inst, 1, 0);
		}
		printDependencies(inst);

	}
	
	scheduleTrees();

	LOG("\n\n");
}

void Matcher::printDependencies(Instruction* inst){
		LOG("root deps(%lu): ", roots[inst]->size());
		for (auto it = roots[inst]->begin(), e = roots[inst]->end(); it != e; ++it){
			LOG("%u, ", (*it)->get_id());
		}

		LOG("\trevdeps(%lu): ", revdeps[inst]->size());
		for (auto it = revdeps[inst]->begin(), e = revdeps[inst]->end(); it != e; ++it){
			LOG("%u, ", (*it)->get_id());
		}
		LOG("\n");
}

void Matcher::scheduleTrees(){

	LOG("\n\n----------- perform scheduling                                    ---------------\n");
	// do loop until all roots are scheduled
	

	while (roots.begin() != roots.end()){
		LOG("roots left to schedule: %lu\n", roots.size());

		Instruction* candidate=NULL;// = roots.begin()->first;
		for (auto i = roots.begin(), e = roots.end(); i != e; ++i){
			// skip if it has deps
			if (i->second->size() > 0) continue;
			// set candidate, if no candidate is set yet
			if (!candidate) {
				candidate = i->first;
				continue;
			}
			// set candidate, if root has more dependants
			if (revdeps[candidate]->size() < revdeps[i->first]->size()){
				candidate = i->first;
			}
		}

		// found candidate -> schedule it
		LOG("*** schedule root %d\n", candidate->get_id());

		printDependencies(candidate);

		if (checkIsNodeExcluded(candidate)){
			LOG("excluded node is handled manually\n");
		} else {
			dumpCover(candidate, 1, 0);
			lowerTree(candidate, 1);
			
		}

		// remove scheduled node from datastructures
		for (auto i = roots.begin(), e = roots.end(); i != e; ++i){
			i->second->erase(candidate);
		}
		roots.erase(candidate);
		revdeps.erase(candidate);

		LOG("\n\n");
	}
}

bool Matcher::checkIsNodeExcluded(Instruction* inst){
	return excluded_nodes.find(inst->get_opcode()) != excluded_nodes.end();
}

Instruction* Matcher::getOperand(Instruction* op, int pos){
	// check if operand is set
	if (pos > op->op_size()) return NULL;


	// always use proxy, if it already exists
	if (instrProxies.find(op) != instrProxies.end()){
		std::unordered_map<int, Instruction*> *proxyops = instrProxies[op];
		if (proxyops->find(pos) != proxyops->end()){
			return proxyops->at(pos);
		}
	}


	Instruction* operand = op->get_operand(pos);

	// if its a dependency btw. subtrees of the BB
	bool dep = false;
	if (roots.find(operand) != roots.end())
		dep = true;

	// if the tree requires a proxy, create it - if it does not yet exist - and return it
	// also track tree dependencies
	if (dep || 
		(op->get_BeginInst() != operand->get_BeginInst()) ||
		(operand->get_opcode() == Instruction::PHIInstID)		){

		// create data structure if it does not exist yet
		if (instrProxies.find(op) == instrProxies.end()){
			instrProxies[op] = new std::unordered_map<int, Instruction*>();;
		}
		std::unordered_map<int, Instruction*> *proxyops = instrProxies[op];

		// create proxy
		Instruction* proxy = new Instruction(Instruction::NoInstID, operand->get_type(), op->get_BeginInst());
		(*proxyops)[pos] = proxy;

		LOG("I %u op %u (= I %u) -> proxy\n", 
			op->get_id(), pos, operand->get_id());

		// add dependency btw. trees
		if (dep){
			// FIXME: find root -> should be passed ideally
			Instruction* root = findRoot(op);
			roots[root]->insert(operand);

			revdeps[operand]->insert(root);

			LOG("I %u op %u (= I %u) = dep. at root I %u\n",
				op->get_id(), pos, operand->get_id(), root->get_id());
			
		}
		// return proxy op
		return proxy;
	}

	// return actual operand
	return operand;
}

Instruction* Matcher::findRoot(Instruction* inst){
	Instruction* root = inst;
	while (roots.find(root) == roots.end()){
		root = *root->user_begin();
	}
	return root;
}


void Matcher::dumpCover(Instruction* p, int goalnt, int indent) {
	#ifdef TRACE
	int eruleno = burm_rule(STATE_LABEL(p), goalnt);
	const short *nts = burm_nts[eruleno];
	Instruction* kids[10];
	int i;

	for (i = 0; i < indent; i++)
		LOG(" ");
	LOG("%s\t\truleno: %d\tinstrId: %d\n", burm_string[eruleno], eruleno, p->get_id());
	burm_kids(p, eruleno, kids);
	for (i = 0; nts[i]; i++)
		dumpCover(kids[i], nts[i], indent + 1);
	#endif
}

void Matcher::lowerTree(Instruction* p, int goalnt){
	int eruleno = burm_rule(STATE_LABEL(p), goalnt);
	const short *nts = burm_nts[eruleno];
	Instruction* kids[*nts+1];
	burm_kids(p, eruleno, kids);
	for (int i = 0; nts[i]; i++)
		lowerTree(kids[i], nts[i]);

	lowerRule(p, (RuleId) eruleno);
}


void Matcher::lowerRule(Instruction* inst, RuleId id){

	if (burm_isinstruction[id]){
		if (inst->get_opcode() == Instruction::NoInstID){
			LOG("NoInstID is wildcard, no lowering required\n");
		} else {
			LOG("Rule %s is basic Instruction, using LoweringVisitor\n", burm_templates[id]);
		}
	} else {
		switch (id){
			case AddImmImm:
				LOG("Lowering I %d with rule %s\n", inst->get_id(), burm_templates[id]);
				break;
			default:
				LOG("NOT IMPLEMENTED Rule %s: Lowering I %d\n", burm_templates[id], inst->get_id());
				break;
		}
	}

}



void Matcher::findRoots(){

	for (std::list<Instruction*>::const_iterator i = instructions->begin(), 
		 e = instructions->end(); i != e; ++i){
		Instruction* inst = *i;

		// if multiop instruction, handle manually, all ops are roots
		if (checkIsNodeExcluded(inst)){
			LOG("I %d = excluded node\n", inst->get_id());
			roots[inst] = new std::set<Instruction*>();
			revdeps[inst] = new std::set<Instruction*>();
			continue;
		}

		// consider inst with multiple users
		if (inst->user_size() > 1){
			LOG("I %d has multiple users\n", inst->get_id());
			roots[inst] = new std::set<Instruction*>();
			revdeps[inst] = new std::set<Instruction*>();
			continue;
		}

		// if inst has no users
		if (inst->user_size() == 0){
			LOG("I %d has no users\n", inst->get_id());
			roots[inst] = new std::set<Instruction*>();
			// not necessarily needed
			revdeps[inst] = new std::set<Instruction*>();
			continue;
		}

		// if users are outside of basic block
		Instruction *user = *(inst->user_begin());
		if (user->get_BeginInst() != basicBlock){
			LOG("I %d has users in another BB\n", inst->get_id());
			roots[inst] = new std::set<Instruction*>();
			// not necessarily needed
			revdeps[inst] = new std::set<Instruction*>();
			continue;
		}

		if (checkIsNodeExcluded(user)){
			LOG("I %d is used by excluded node\n", inst->get_id());
			roots[inst] = new std::set<Instruction*>();
			revdeps[inst] = new std::set<Instruction*>();
			continue;
		}


		// LOG("Instruction %d is not a root node\n", inst->get_id());
	}

}


}
}
}


using namespace cacao::jit::compiler2;

// test method
int main(void) {

	std::list<Instruction*> instructions;

	// 0, 1
	Instruction *begin = new Instruction(Instruction::BeginInstID, Type::VoidTypeID);
	Instruction *other_begin = new Instruction(Instruction::BeginInstID, Type::VoidTypeID);

	// 2,3,4
	Instruction *add = new Instruction(Instruction::ADDInstID, Type::IntTypeID, begin);
	Instruction *const1 = new Instruction(Instruction::CONSTInstID, Type::IntTypeID, begin);
	Instruction *const2 = new Instruction(Instruction::CONSTInstID, Type::IntTypeID, other_begin);

	add->append_op(const1);
	add->append_op(const2);

	// 5,6
	Instruction *add2 = new Instruction(Instruction::ADDInstID, Type::IntTypeID, begin);
	Instruction *const3 = new Instruction(Instruction::CONSTInstID, Type::IntTypeID, begin);	
	// Instruction *const4 = new Instruction(Instruction::CONSTInstID, begin);	

	add2->append_op(const3);
	add2->append_op(const1);

	// 7
	Instruction *add3 = new Instruction(Instruction::ADDInstID, Type::IntTypeID, begin);

	add3->append_op(add);
	add3->append_op(add2);

	// test handling of phi
	// 8,9
	Instruction *neg = new Instruction(Instruction::NEGInstID, Type::BooleanTypeID, begin);
	Instruction *phi = new Instruction(Instruction::PHIInstID, Type::VoidTypeID, begin);
	neg->append_op(phi);

	// 10
	Instruction *invoke = new Instruction(Instruction::INVOKEVIRTUALInstID, Type::BooleanTypeID, begin);
	invoke->append_op(neg);

	// 11
	Instruction *neg3 = new Instruction(Instruction::NEGInstID, Type::BooleanTypeID, begin);
	neg3->append_op(invoke);

	// 12,13,14
	Instruction *add4 = new Instruction(Instruction::ADDInstID, Type::IntTypeID, begin);
	Instruction *const5 = new Instruction(Instruction::CONSTInstID, Type::IntTypeID, begin);
	Instruction *const6 = new Instruction(Instruction::CONSTInstID, Type::IntTypeID, begin);

	add4->append_op(const5);
	add4->append_op(const6);

	instructions.push_back(add);
	instructions.push_back(const1);
	// instructions.push_back(const2);

	instructions.push_back(add2);
	instructions.push_back(const3);
	// instructions.push_back(const4);

	instructions.push_back(add3);

	instructions.push_back(phi);
	instructions.push_back(neg);
	instructions.push_back(invoke);
	instructions.push_back(neg3);

	instructions.push_back(add4);
	instructions.push_back(const5);
	instructions.push_back(const6);

	// outside of bb instructions
	Instruction *neg2 = new Instruction(Instruction::NEGInstID, Type::BooleanTypeID, other_begin);
	neg2->append_op(add3);

	Matcher m = Matcher(&instructions);

	m.run();

	return 0;
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