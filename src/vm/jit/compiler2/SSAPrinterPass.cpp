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
						edges.insert(edge);
					}
				}
			}
			// add successor link
			for(Instruction::SuccessorListTy::const_iterator ii = I->succ_begin(), ee = I->succ_end();
				ii != ee; ++ii) {
				Value *v = (*ii);
				if (v) {
					Instruction *II = (*ii)->to_Instruction();
					if (II) {
						EdgeType edge = std::make_pair(I,II);
						edges.insert(edge);
					}
				}
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

    StringBuf getEdgeAttributes(const SSAGraph::EdgeType &e) const ;
#endif
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
