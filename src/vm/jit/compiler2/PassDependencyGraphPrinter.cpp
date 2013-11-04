/* src/vm/jit/compiler2/PassDependencyGraphPrinter.cpp - PassDependencyGraphPrinter

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

#include "vm/jit/compiler2/PassDependencyGraphPrinter.hpp"
#include "toolbox/GraphPrinter.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "toolbox/logging.hpp"
#include <algorithm>

#define DEBUG_NAME "compiler2/PassDependencyGraphPrinter"

namespace cacao {
namespace jit {
namespace compiler2 {

namespace {

class PassDependencyGraphPrinter : public PrintableGraph<PassManager,PassInfo::IDTy> {
private:
	std::map<PassInfo::IDTy,const char*> names;
public:

    PassDependencyGraphPrinter(PassManager &PM) {
		for(PassManager::PassInfoMapTy::const_iterator i = PM.registered_begin(),
				e = PM.registered_end(); i != e; ++i) {
			PassInfo::IDTy PI = i->first;
			names[PI] = i->second->get_name();
			nodes.insert(i->first);

			LOG("Pass: " << i->second->get_name() << " ID: " << PI << nl);

			// create Pass
			Pass *pass = i->second->create_Pass();
			PassUsage PU;
			pass->get_PassUsage(PU);
			delete pass;
			for (PassUsage::PIIDSet::const_iterator i = PU.requires_begin(),
					e = PU.requires_end(); i != e; ++i) {
				LOG("  requires: " << names[*i] << " ID: " << *i << nl);
				EdgeType edge = std::make_pair(PI,*i);
				edges.insert(edge);
			}
		}
		#if 0
		std::for_each(LT->loop_begin(),LT->loop_end(),insert_loop(this));
		for(Method::BBListTy::const_iterator i = M.bb_begin(),
		    e = M.bb_end(); i != e; ++i) {
			BeginInst *BI = *i;
			if (BI == NULL)
				continue;
			nodes.insert(BI);
			Loop loop = LT->get_Loop(BI);
			if (idom) {
				EdgeType edge = std::make_pair(idom,BI);
				edges.insert(edge);
			}
		}
		#endif
	}

    OStream& getGraphName(OStream& OS) const {
		return OS << "PassDependencyGraph";
	}

    OStream& getNodeLabel(OStream& OS, const PassInfo::IDTy &node) const {
		return OS << names.find(node)->second;
	}

};

} // end anonymous namespace

// run pass
void print_PassDependencyGraph(PassManager &PM) {
	GraphPrinter<PassDependencyGraphPrinter>::print("PassDependencyGraph.dot", PassDependencyGraphPrinter(PM));
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
