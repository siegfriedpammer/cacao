/* src/vm/jit/compiler2/LivetimeAnalysisPass.cpp - LivetimeAnalysisPass

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

#include "vm/jit/compiler2/LivetimeAnalysisPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/MachineLoopPass.hpp"
#include "vm/jit/compiler2/MachineOperand.hpp"

#include "toolbox/logging.hpp"
#include "toolbox/Option.hpp"

#define DEBUG_NAME "compiler2/LivetimeAnalysis"

namespace cacao {
namespace jit {
namespace compiler2 {

void LivetimeAnalysisPass::initialize() {
	lti_map.clear();
	MIS = NULL;
}

namespace {

MachineInstruction::successor_iterator
get_successor_begin(MachineBasicBlock* MBB) {
	return MBB->back()->successor_begin();
}

MachineInstruction::successor_iterator
get_successor_end(MachineBasicBlock* MBB) {
	return MBB->back()->successor_end();
}


typedef LivetimeAnalysisPass::LiveInSetTy LiveInSetTy;
typedef LivetimeAnalysisPass::LiveInMapTy LiveInMapTy;
typedef LivetimeAnalysisPass::LivetimeIntervalMapTy LivetimeIntervalMapTy;

/**
 * @Cpp11 use std::function
 */
class InsertPhiOperands : public std::unary_function<MachineBasicBlock*,void> {
private:
	/// Phi inserter
	struct PhiOperandInserter : public std::unary_function<MachinePhiInst*,void> {
		LiveInSetTy &live;
		std::size_t index;
		/// constructor
		PhiOperandInserter(LiveInSetTy &live, std::size_t index)
			: live(live), index(index) {}

		void operator()(MachinePhiInst* phi) {
			live.insert(phi->get(index).op);
		}
	};

	LiveInSetTy &live;
	MachineBasicBlock *current;
public:
	/// constructor
	InsertPhiOperands(LiveInSetTy &live, MachineBasicBlock *current)
		: live(live), current(current) {}

	void operator()(MachineBasicBlock* MBB) {
		std::for_each(MBB->phi_begin(), MBB->phi_end(),
			PhiOperandInserter(live,MBB->get_predecessor_index(current)));
	}
};

/**
 * @Cpp11 use std::function
 */
struct UnionLiveIn : public std::unary_function<MachineBasicBlock*,void> {
	LiveInSetTy &live_set;
	LiveInMapTy &live_map;

	/// Constructor
	UnionLiveIn(LiveInSetTy &live_set, LiveInMapTy &live_map)
		: live_set(live_set), live_map(live_map) {}

	void operator()(MachineBasicBlock* MBB) {
		LiveInSetTy &other_set = live_map[MBB];
		live_set.insert(other_set.begin(), other_set.end());
	}
};


inline LivetimeInterval& find_or_insert(LivetimeIntervalMapTy &lti_map, MachineOperand* op) {
	LivetimeIntervalMapTy::iterator i = lti_map.find(op);
	if (i == lti_map.end()) {
		LivetimeInterval lti(op);
		i = lti_map.insert(std::make_pair(op,lti)).first;
	}
	return i->second;
}

/**
 * @Cpp11 use std::function
 */
class AddOperandInterval : public std::unary_function<MachineOperand*,void> {
private:
	LivetimeIntervalMapTy &lti_map;
	MachineBasicBlock *BB;
public:
	/// Constructor
	AddOperandInterval(LivetimeIntervalMapTy &lti_map, MachineBasicBlock *BB)
		: lti_map(lti_map), BB(BB) {}
	void operator()(MachineOperand* op) {
		assert(op);
		if (op->needs_allocation()) {
			LOG2("AddOperandInterval: op=" << *op << " BasicBlock: " << *BB << nl);
			find_or_insert(lti_map,op).add_range(UseDef(UseDef::PseudoUse,BB->mi_first()),
				UseDef(UseDef::PseudoDef,BB->mi_last()));
		}
	}
};

/**
 * @Cpp11 use std::function
 */
class ProcessOutOperands : public std::unary_function<MachineOperandDesc,void> {
private:
	LivetimeIntervalMapTy &lti_map;
	MIIterator i;
	MIIterator e;
	LiveInSetTy &live;
public:
	/// Constructor
	ProcessOutOperands(LivetimeIntervalMapTy &lti_map, MIIterator i, MIIterator e,
		LiveInSetTy &live) : lti_map(lti_map), i(i), e(e), live(live) {}
	void operator()(MachineOperandDesc &op) {
		if (op.op && op.op->needs_allocation()) {
			LOG2("ProcessOutOperand: op=" << *(op.op) << " set form: " << **i << nl);
			LivetimeInterval &lti = find_or_insert(lti_map,op.op);
			lti.set_from(UseDef(UseDef::Def,i,&op), UseDef(UseDef::PseudoDef,e));
			live.erase(op.op);
			// set register hints
			if ((*i)->is_move()) {
				MachineOperand *op = (*i)->get(0).op;
				if (op->needs_allocation()) {
					LOG2("set hint " << op << " inst: " << *i << " lti: " << lti << nl);
					lti.set_hint(op);
				}
			}
		}
	}
};
/**
 * @Cpp11 use std::function
 */
class ProcessInOperands : public std::unary_function<MachineOperandDesc,void> {
private:
	LivetimeIntervalMapTy &lti_map;
	MachineBasicBlock *BB;
	MIIterator i;
	LiveInSetTy &live;
public:
	/// Constructor
	ProcessInOperands(LivetimeIntervalMapTy &lti_map, MachineBasicBlock *BB,
		MIIterator i, LiveInSetTy &live) : lti_map(lti_map), BB(BB), i(i), live(live) {}
	void operator()(MachineOperandDesc &op) {
		if (op.op && op.op->needs_allocation()) {
			LOG2("ProcessInOperand: op=" << *(op.op) << " range form: "
				<< BB->front() << " to " << **i << nl);
			LivetimeInterval &lti = find_or_insert(lti_map,op.op);
			lti.add_range(UseDef(UseDef::PseudoUse,BB->mi_first()),
				UseDef(UseDef::Use,i,&op));
			live.insert(op.op);
		}
	}
};

/**
 * @Cpp11 use std::function
 */
struct RemovePhi : public std::unary_function<MachinePhiInst*,void> {
	LiveInSetTy &live;
	/// constructor
	RemovePhi(LiveInSetTy &live) : live(live) {}

	void operator()(MachinePhiInst* phi) {
		LOG2("Remove phi result" << *phi->get_result().op << nl);
		live.erase(phi->get_result().op);
	}
};

/**
 * @Cpp11 use std::function
 */
class ProcessLoops : public std::unary_function<MachineLoop*,void> {
private:
	struct ForEachLiveOperand : public std::unary_function<MachineOperand*,void> {
		LivetimeIntervalMapTy &lti_map;
		MachineBasicBlock *BB;
		MachineLoop *loop;
		/// contructor
		ForEachLiveOperand(LivetimeIntervalMapTy &lti_map, MachineBasicBlock *BB,MachineLoop *loop)
			:  lti_map(lti_map), BB(BB), loop(loop) {}
		void operator()(MachineOperand* op) {
			assert(op->needs_allocation());
			LOG2("ProcessLoops: operand " << op << " range from " << BB->mi_first()
				<< " to " << loop->get_exit()->mi_last() << nl );
			find_or_insert(lti_map,op).add_range(UseDef(UseDef::PseudoUse,BB->mi_first()),
				UseDef(UseDef::PseudoDef,loop->get_exit()->mi_last()));
		}
	};
	LivetimeIntervalMapTy &lti_map;
	MachineBasicBlock *BB;
	LiveInSetTy &live;
public:
	/// Constructor
	ProcessLoops(LivetimeIntervalMapTy &lti_map, MachineBasicBlock *BB,
		LiveInSetTy &live) : lti_map(lti_map), BB(BB), live(live) {}
	void operator()(MachineLoop* loop) {
		LOG2(Cyan << "ProcessLoop: " << *loop << nl);
		std::for_each(live.begin(),live.end(), ForEachLiveOperand(lti_map, BB, loop));
	}
};

namespace option {
	Option<bool> print_intervals("PrintLivetimeIntervals","compiler2: print livetime intervals",false,::cacao::option::xx_root());
}

} // end anonymous namespace

bool LivetimeAnalysisPass::run(JITData &JD) {
	LOG("LivetimeAnalysisPass::run()" << nl);
	MIS = get_Artifact<LIRInstructionScheduleArtifact>()->MIS;
	MachineLoopTree *MLT = get_Artifact<MachineLoopPass>();
	// TODO use better data structure for the register set
	LiveInMapTy liveIn;

	// for all basic blocks in reverse order
	for (MachineInstructionSchedule::const_reverse_iterator i = MIS->rbegin(),
			e = MIS->rend(); i != e ; ++i) {
		MachineBasicBlock* BB = *i;
		LOG1("BasicBlock: " << *BB << nl);
		// get live set
		LiveInSetTy &live = liveIn[BB];

		// live == union of successor.liveIn for each successor of BB
		std::for_each(get_successor_begin(BB), get_successor_end(BB), UnionLiveIn(live,liveIn));

		// for each phi function of successors of BB
		std::for_each(get_successor_begin(BB), get_successor_end(BB), InsertPhiOperands(live,BB));

		// for each operand in live
		std::for_each(live.begin(),live.end(),AddOperandInterval(lti_map,BB));

		// for each operation op of BB in reverse order
		for(MachineBasicBlock::reverse_iterator i = BB->rbegin(), e = BB->rend(); i != e ; ++i) {
			// phis are handled separately
			if ((*i)->is_phi()) continue;

			LOG2("MInst: " << **i << nl);
			// for each output operand of MI
			ProcessOutOperands(lti_map, BB->convert(i), BB->mi_last(), live)((*i)->get_result());

			// for each input operand of MI
			std::for_each((*i)->begin(),(*i)->end(), ProcessInOperands(lti_map, BB, BB->convert(i), live));

			// for each dummy operand of MI
			std::for_each((*i)->dummy_begin(),(*i)->dummy_end(), ProcessInOperands(lti_map, BB, BB->convert(i), live));
		}

		// for each phi of BB
		std::for_each(BB->phi_begin(), BB->phi_end(), RemovePhi(live));

		// if b is loop header
		if (MLT->is_loop_header(BB)) {
			MachineLoopTree::ConstLoopIteratorPair loops = MLT->get_Loops_from_header(BB);
			std::for_each (loops.first, loops.second, ProcessLoops(lti_map, BB, live));
		}

		LOG1("LiveIn " << *BB << ": ");
		DEBUG1(print_container(dbg(),live.begin(), live.end()) << nl);
	}

	if (option::print_intervals) {
		OStream fout(fopen("LiveIntervalTable.csv","w"));
		print(fout);
	}
	return true;
}

namespace {
/**
 * @Cpp11 use std::function
 */
struct PrintLivetimeState : public std::unary_function<LivetimeIntervalMapTy::value_type,void> {
	OStream &OS;
	MIIterator pos;
	PrintLivetimeState(OStream &OS, MIIterator pos) : OS(OS), pos(pos) {}
	void operator()(LivetimeIntervalMapTy::value_type lti) {
		switch (lti.second.get_State(pos)){
		case LivetimeInterval::Active:
		{
			bool use = lti.second.is_use_at(pos);
			bool def = lti.second.is_def_at(pos);
			if (use)
				OS << "use";
			if (def)
				OS << "def";
			if (!(use || def))
				OS << "active";
			break;
		}
		case LivetimeInterval::Inactive: OS << "inactive"; break;
		default: break;
		}
		OS << ";";
	}
};

} // end anonymous namespace

OStream& LivetimeAnalysisPass::print(OStream& OS) const {
	OS << "BasicBlock;Instruction;";
	for (LivetimeIntervalMapTy::const_iterator i = lti_map.begin(),
			e = lti_map.end(); i != e ; ++i) {
		MachineOperand *op = i->first;
		OS << *op << ";";
	}
	OS << nl;
	for (MachineInstructionSchedule::const_iterator i = MIS->begin(), e = MIS->end(); i != e; ++i) {
		MachineBasicBlock *BB = *i;
		// print label
		MIIterator pos = BB->mi_first();
		OS << *BB << ";" << **pos << ";";
		std::for_each(lti_map.begin(),lti_map.end(),PrintLivetimeState(OS,pos));
		OS << nl;
		// print phis
		for (MachineBasicBlock::const_phi_iterator i = BB->phi_begin(), e = BB->phi_end(); i != e; ++i) {
			OS << *BB << ";" << **i << ";";
			std::for_each(lti_map.begin(),lti_map.end(),PrintLivetimeState(OS,pos));
			OS << nl;
		}
		// print other instructions
		for (MachineBasicBlock::iterator i = ++BB->begin(), e = BB->end(); i != e; ++i) {
			MIIterator pos = BB->convert(i);
			OS << *BB << ";" << **pos << ";";
			std::for_each(lti_map.begin(),lti_map.end(),PrintLivetimeState(OS,pos));
			OS << nl;
		}
	}
	return OS;
}

bool LivetimeAnalysisPass::verify() const {
#if 0
	for (LivetimeIntervalMapTy::const_iterator i = lti_map.begin(),
			e = lti_map.end(); i != e ; ++i) {
		const LivetimeInterval &lti = i->second;
		signed old = -1;
		for (LivetimeInterval::const_use_iterator i = lti.use_begin(),
				e = lti.use_end(); i != e; ++i) {
			if ( old > (signed)i->first) {
				err() << "use not ordered " << old << " > " << i->first << nl;
				return false;
			}
			old = i->first;
		}
		old = -1;
		for (LivetimeInterval::const_def_iterator i = lti.def_begin(),
				e = lti.def_end(); i != e; ++i) {
			if ( old > (signed)i->first) {
				err() << "def not ordered " << old << " > " << i->first << nl;
				return false;
			}
			old = i->first;
		}
	}
#endif
	return true;
}

// pass usage
PassUsage& LivetimeAnalysisPass::get_PassUsage(PassUsage &PU) const {
	return PU;
}

// register pass
static PassRegistry<LivetimeAnalysisPass> X("LivetimeAnalysisPass");

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
