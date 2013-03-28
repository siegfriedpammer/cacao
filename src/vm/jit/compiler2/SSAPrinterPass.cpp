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
#include "vm/jit/compiler2/Method.hpp"
#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/Instructions.hpp"

#include "toolbox/GraphTraits.hpp"

#include <sstream>

namespace cacao {
namespace jit {
namespace compiler2 {

namespace {

class SSAGraph : public GraphTraits<Method,Instruction> {
protected:
    const Method &M;
    bool verbose;
	std::set<EdgeType> data_dep;
	std::set<EdgeType> sched_dep;
	std::set<EdgeType> cfg_edges;
	std::set<EdgeType> begin2end_edges;

public:

    SSAGraph(const Method &M, bool verbose = false) : M(M), verbose(verbose) {
		for(Method::InstructionList::const_iterator i = M.begin(),
		    e = M.end(); i != e; ++i) {
			Instruction *I = *i;
			nodes.insert(I);
			// add operator link
			for(Instruction::OperandListTy::const_iterator ii = I->op_begin(), ee = I->op_end();
				ii != ee; ++ii) {
				Value *v = (*ii);
				if (v) {
					Instruction *II = (*ii)->to_Instruction();
					if (II) {
						EdgeType edge = std::make_pair(I,II);
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
					EdgeType edge = std::make_pair(I,II);
					sched_dep.insert(edge);
					edges.insert(edge);
				}
			}
			// add successor link
			EndInst *EI = I->to_EndInst();
			if (EI) {
				for(EndInst::SuccessorListTy::const_iterator ii = EI->succ_begin(), ee = EI->succ_end();
					ii != ee; ++ii) {
					Value *v = (*ii);
					if (v) {
						Instruction *II = (*ii)->to_Instruction();
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
		}
	}

    StringBuf getGraphName() const {
		return "SSAGraph";
	}

    StringBuf getNodeLabel(const Instruction &node) const {
		std::ostringstream sstream;
		sstream << "[" << getNodeID(node) << "] "
		        << node.get_name();
		for(Instruction::OperandListTy::const_iterator ii = node.op_begin(), ee = node.op_end();
				ii != ee; ++ii) {
			sstream << " ";
			Value *v = (*ii);
			if (v) {
				Instruction *II = (*ii)->to_Instruction();
				sstream << getNodeID(*II);
			} else {
				sstream << "NULL";
			}
		}
		return sstream.str();
	}

#if 0
    StringBuf getNodeAttributes(const SSAGraph::NodeType &node) const ;

    StringBuf getEdgeLabel(const SSAGraph::EdgeType &e) const ;

#endif
    StringBuf getEdgeAttributes(const SSAGraph::EdgeType &e) const {
		StringBuf attr;
		if (data_dep.find(e) != data_dep.end()) {
			attr +="color=red,";
		}
		if (sched_dep.find(e) != sched_dep.end()) {
			attr +="color=blue,";
		}
		if (begin2end_edges.find(e) != begin2end_edges.end()) {
			attr +="style=dashed,";
		}
		return attr;
	}
};

} // end anonymous namespace


bool SSAPrinterPass::run(JITData &JD) {
	GraphPrinter<SSAGraph>::print("test.dot", SSAGraph(*(JD.get_Method())));
	return true;
}

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
