/* src/vm/jit/compiler2/DomTreePrinterPass.cpp - DomTreePrinterPass

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

#include "vm/jit/compiler2/DomTreePrinterPass.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/MethodC2.hpp"
#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"

#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/DominatorPass.hpp"
#include "vm/jit/compiler2/GraphHelper.hpp"

#include "toolbox/GraphPrinter.hpp"
#include "vm/class.hpp"
#include "vm/jit/jit.hpp"

#include <sstream>

namespace cacao {
namespace jit {
namespace compiler2 {

namespace {

class DomTreeGraph : public PrintableGraph<Method*,BeginInst*> {
protected:
    const Method &M;
    bool verbose;
	DominatorTree &DT;
	const DFSTraversal<BeginInst> dfs;

public:

    DomTreeGraph(const Method &M, DominatorTree &DT, bool verbose = false) :
			M(M), verbose(verbose), DT(DT), dfs(M.get_init_bb()) {
		for(Method::BBListTy::const_iterator i = M.bb_begin(),
		    e = M.bb_end(); i != e; ++i) {
			BeginInst *BI = *i;
			if (BI == NULL)
				continue;
			nodes.insert(BI);
			BeginInst *idom = DT.get_idominator(BI);
			if (idom) {
				EdgeType edge = std::make_pair(idom,BI);
				edges.insert(edge);
			}
		}
	}

    virtual OStream& getGraphName(OStream& OS) const {
		return OS << "DomTreeGraph";
	}

    virtual OStream& getNodeLabel(OStream& OS, BeginInst *const &node) const {
		return OS << *node;
	}

};

} // end anonymous namespace

PassUsage& DomTreePrinterPass::get_PassUsage(PassUsage &PU) const {
	PU.requires<DominatorPass>();
	PU.immediately_after<DominatorPass>();
	return PU;
}

Option<bool> DomTreePrinterPass::enabled("DomTreePrinterPass","compiler2: enable DomTreePrinterPass",false,::cacao::option::xx_root());

// register pass
static PassRegistry<DomTreePrinterPass> X("DomTreePrinterPass");

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

// run pass
bool DomTreePrinterPass::run(JITData &JD) {
	// get dominator tree
	DominatorTree *DT = get_Artifact<DominatorPass>();
	assert(DT);
	std::string name = get_filename(JD.get_jitdata()->m,JD.get_jitdata(),"domtree_");
	GraphPrinter<DomTreeGraph>::print(name.c_str(), DomTreeGraph(*(JD.get_Method()),*DT));
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
