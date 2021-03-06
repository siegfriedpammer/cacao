/* src/vm/jit/compiler2/SSAPrinterPass.cpp - SSAPrinterPass

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

#include "vm/jit/compiler2/SSAPrinterPass.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/MethodC2.hpp"
#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"

#include "vm/jit/compiler2/SSAConstructionPass.hpp"
#include "vm/jit/compiler2/ScheduleEarlyPass.hpp"
#include "vm/jit/compiler2/ScheduleLatePass.hpp"
#include "vm/jit/compiler2/ScheduleClickPass.hpp"

#include "toolbox/GraphPrinter.hpp"

#include "vm/utf8.hpp"
#include "vm/jit/jit.hpp"
#include "vm/class.hpp"

#include <sstream>

namespace {
std::string get_filename(methodinfo *m, jitdata *jd, std::string prefix = "cfg_", std::string suffix=".dot");
std::string get_filename(methodinfo *m, jitdata *jd, std::string prefix, std::string suffix)
{
	std::string filename = prefix;
	filename += Utf8String(m->clazz->name).begin();
	filename += ".";
	filename += Utf8String(m->name).begin();
	filename += Utf8String(m->descriptor).begin();
	filename += suffix;
	/* replace unprintable chars */
	for (size_t i = filename.find_first_of('/');
		 i != std::string::npos;
		 i = filename.find_first_of('/',i+1)) {
		filename.replace(i,1,1,'.');
	}
	const char *unchar = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.";
	for (size_t i = filename.find_first_not_of(unchar);
		i != std::string::npos;
		i = filename.find_first_not_of(unchar,i+1)) {
		filename.replace(i,1,1,'_');
	}

	return filename;
}
} // end anonymous namespace

namespace cacao {
namespace jit {
namespace compiler2 {

namespace {

class SSAGraph : public PrintableGraph<Method*,Instruction*> {
protected:
    const Method &M;
	std::string name;
	GlobalSchedule *sched;
    bool verbose;
	alloc::set<EdgeType>::type data_dep;
	alloc::set<EdgeType>::type sched_dep;
	alloc::set<EdgeType>::type cfg_edges;
	alloc::set<EdgeType>::type begin2end_edges;

public:

	SSAGraph(const Method &M, std::string name, GlobalSchedule *sched = NULL, bool verbose = true)
			: M(M), name(name), sched(sched), verbose(verbose) {
		init();
	}
	SSAGraph(const Method &M, std::string name, bool verbose)
			: M(M), name(name), sched(NULL), verbose(verbose) {
		init();
	}
	void init() {
		for(Method::InstructionListTy::const_iterator i = M.begin(),
		    e = M.end(); i != e; ++i) {
			Instruction *I = *i;
			if (I == NULL)
				continue;
			// if no verbose only print end begin
			if (!verbose && (!(I->to_BeginInst() || I->to_EndInst())) )
				continue;
			nodes.insert(I);
			if(verbose) {
				// add operator link
				for(Instruction::OperandListTy::const_iterator ii = I->op_begin(), ee = I->op_end();
					ii != ee; ++ii) {
					Value *v = (*ii);
					if (v) {
						Instruction *II = (*ii)->to_Instruction();
						if (II) {
							EdgeType edge = std::make_pair(II,I);
							data_dep.insert(edge);
							edges.insert(edge);
						}
					}
				}
				// add dependency link
				for(Instruction::DepListTy::const_iterator ii = I->dep_begin(), ee = I->dep_end();
					ii != ee; ++ii) {
					Instruction *II = (*ii);
					if (II) {
						EdgeType edge = std::make_pair(II,I);
						sched_dep.insert(edge);
						edges.insert(edge);
					}
				}
			}
			// add successor link
			EndInst *EI = I->to_EndInst();
			if (EI) {
				for(EndInst::SuccessorListTy::const_iterator ii = EI->succ_begin(), ee = EI->succ_end();
					ii != ee; ++ii) {
					Value *v = ii->get();
					if (v) {
						Instruction *II = ii->get()->to_Instruction();
						if (II) {
							EdgeType edge = std::make_pair(EI,II);
							cfg_edges.insert(edge);
							edges.insert(edge);
						}
					}
				}
				EdgeType edge = std::make_pair(EI->get_BeginInst(),EI);
				begin2end_edges.insert(edge);
				edges.insert(edge);
			}
			// clustering
			BeginInst *bi = (sched) ? sched->get(I) : I->get_BeginInst();
			if (bi) {
				clusters[(unsigned long)bi].insert(I);
			}
		}
	}

	virtual unsigned long getNodeID(Instruction *const &node) const {
		return node->get_id();
	}

    virtual OStream& getGraphName(OStream& OS) const {
		return OS << name;
	}

    virtual OStream& getNodeLabel(OStream& OS, Instruction *const &node) const {
		#if 0
		OS << "[" << node->get_id() << ": " << node->get_name() << " ("
				<< get_type_name(node->get_type()) << ")]";
		#endif
		OS << node;
		for(Instruction::OperandListTy::const_iterator ii = node->op_begin(), ee = node->op_end();
				ii != ee; ++ii) {
			OS << " ";
			Value *v = (*ii);
			if (v) {
				Instruction *II = (*ii)->to_Instruction();
				OS << "[" << II->get_id() << "]";
			} else {
				OS << "NULL";
			}
		}
		return OS;
	}

#if 0
    StringBuf getNodeAttributes(const SSAGraph::NodeType &node) const ;

    StringBuf getEdgeLabel(const SSAGraph::EdgeType &e) const ;

#endif
    OStream& getEdgeAttributes(OStream& OS, const SSAGraph::EdgeType &e) const;
};

class EdgeAttributeVisitor : public InstructionVisitor {
private:
	OStream &OS;
	BeginInst *target;
public:
	EdgeAttributeVisitor(OStream &OS, BeginInst *target)
		: OS(OS), target(target) {}
	virtual void visit_default(Instruction *I) {
	}
	// make InstructionVisitors visit visible
	using InstructionVisitor::visit;

	virtual void visit(TABLESWITCHInst* I);

};

void EdgeAttributeVisitor::visit(TABLESWITCHInst* I) {
	int index = I->get_successor_index(target);
	if (index == -1) return;
	if (std::size_t(index) == I->succ_size() - 1) {
		OS << "label=\"default\"";
	}
	else {
		OS << "label=\"" << index << "\"";
	}
}

inline OStream& SSAGraph::getEdgeAttributes(OStream& OS, const SSAGraph::EdgeType &e) const {
	if (data_dep.find(e) != data_dep.end()) {
		OS << "color=red,";
	}
	if (sched_dep.find(e) != sched_dep.end()) {
		OS << "color=blue,";
	}
	if (begin2end_edges.find(e) != begin2end_edges.end()) {
		OS << "style=dashed,";
	}
	BeginInst* begin = e.second->to_BeginInst();
	if (begin) {
		EdgeAttributeVisitor visitor(OS,begin);
		e.first->accept(visitor, true);
	}
	return OS;
}

} // end anonymous namespace

// BEGIN SSAPrinterPass

PassUsage& SSAPrinterPass::get_PassUsage(PassUsage &PU) const {
	PU.requires<HIRInstructionsArtifact>();
	PU.immediately_after<SSAConstructionPass>();
	return PU;
}

Option<bool> SSAPrinterPass::enabled("SSAPrinterPass","compiler2: enable SSAPrinterPass",false,::cacao::option::xx_root());

// register pass
static PassRegistry<SSAPrinterPass> X("SSAPrinterPass");

// run pass
bool SSAPrinterPass::run(JITData &JD) {
	std::string name = get_filename(JD.get_jitdata()->m,JD.get_jitdata(),"","");
	std::string filename = "ssa_";
	filename+=name+".dot";
	GraphPrinter<SSAGraph>::print(filename.c_str(), SSAGraph(*(JD.get_Method()), name));
	return true;
}
// END SSAPrinterPass

// BEGIN BasicBlockPrinterPass

PassUsage& BasicBlockPrinterPass::get_PassUsage(PassUsage &PU) const {
	PU.requires<HIRInstructionsArtifact>();
	PU.immediately_after<SSAConstructionPass>();
	return PU;
}

Option<bool> BasicBlockPrinterPass::enabled("BasicBlockPrinterPass","compiler2: enable BasicBlockPrinterPass",false,::cacao::option::xx_root());

// register pass
static PassRegistry<BasicBlockPrinterPass> Z("BasicBlockPrinterPass");

// run pass
bool BasicBlockPrinterPass::run(JITData &JD) {
	std::string name = get_filename(JD.get_jitdata()->m,JD.get_jitdata(),"","");
	std::string filename = "basicblocks_";
	filename+=name+".dot";
	GraphPrinter<SSAGraph>::print(filename.c_str(), SSAGraph(*(JD.get_Method()), name, false));
	return true;
}
// END BasicBlockPrinterPass

// BEGIN GlobalSchedulePrinterPass

template <class _T>
PassUsage& GlobalSchedulePrinterPass<_T>::get_PassUsage(PassUsage &PU) const {
	PU.requires<_T>();
	PU.immediately_after<_T>();
	return PU;
}

Option<bool> schedule_printer_enabled("GlobalSchedulePrinterPass","compiler2: enable GlobalSchedulePrinterPass",false,::cacao::option::xx_root());
// run pass
template <class _T>
bool GlobalSchedulePrinterPass<_T>::run(JITData &JD) {
	std::string name = get_filename(JD.get_jitdata()->m,JD.get_jitdata(),"","");
	std::string filename = "bb_sched_";

	GlobalSchedule* sched = get_Artifact<_T>();

	filename += GlobalSchedulePrinterPass<_T>::name;
	filename += "_" + name + ".dot";
	GraphPrinter<SSAGraph>::print(filename.c_str(), SSAGraph(*(JD.get_Method()), name, sched));
	return true;
}

// set names
template <>
const char* GlobalSchedulePrinterPass<ScheduleLatePass>::name = "late";
template <>
const char* GlobalSchedulePrinterPass<ScheduleEarlyPass>::name = "early";
template <>
const char* GlobalSchedulePrinterPass<ScheduleClickPass>::name = "click";


// register pass
static PassRegistry<GlobalSchedulePrinterPass<ScheduleLatePass> > X_late("GlobalSchedulePrinterPass(late)");
static PassRegistry<GlobalSchedulePrinterPass<ScheduleEarlyPass> > X_early("GlobalSchedulePrinterPass(early)");
static PassRegistry<GlobalSchedulePrinterPass<ScheduleClickPass> > X_click("GlobalSchedulePrinterPass(click)");

// END GlobalSchedulePrinterPass

} // end namespace cacao
} // end namespace jit
} // end namespace compiler2


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
