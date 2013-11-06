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

struct AddEdge : public std::unary_function<PassInfo::IDTy,void> {
	typedef std::pair<argument_type,argument_type> EdgeType;
	std::set<EdgeType> &edges;
	std::set<EdgeType> &set;
	argument_type from;
	bool reverse;
	// constructor
	AddEdge(std::set<EdgeType> &edges, std::set<EdgeType> &set, argument_type from, bool reverse=false)
		: edges(edges), set(set), from(from), reverse(reverse) {}
	// call operator
	void operator()(const argument_type &to) {
		EdgeType edge;
		if (reverse) {
			edge = std::make_pair(to,from);
		}
		else {
			edge = std::make_pair(from,to);
		}
		edges.insert(edge);
		set.insert(edge);
	}
};

template <class InputIterator, class ValueType>
inline bool contains(InputIterator begin, InputIterator end, const ValueType &val) {
	return std::find(begin,end,val) != end;
}

class PassDependencyGraphPrinter : public PrintableGraph<PassManager,PassInfo::IDTy> {
private:
	std::map<PassInfo::IDTy,const char*> names;
	std::set<EdgeType> req;
	std::set<EdgeType> mod;
	std::set<EdgeType> dstr;
	std::set<EdgeType> schedule_after;
	std::set<EdgeType> run_before;
	std::set<EdgeType> schedule_before;
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
			std::for_each(PU.requires_begin(), PU.requires_end(),AddEdge(edges,req,PI));
			std::for_each(PU.modifies_begin(), PU.modifies_end(),AddEdge(edges,mod,PI));
			std::for_each(PU.destroys_begin(), PU.destroys_end(),AddEdge(edges,dstr,PI));
			std::for_each(PU.schedule_after_begin(), PU.schedule_after_end(),AddEdge(edges,schedule_after,PI));
			std::for_each(PU.run_before_begin(), PU.run_before_end(),AddEdge(edges,run_before,PI,true));
			std::for_each(PU.schedule_before_begin(), PU.schedule_before_end(),AddEdge(edges,schedule_before,PI,true));
		}
	}

    virtual OStream& getGraphName(OStream& OS) const {
		return OS << "PassDependencyGraph";
	}

    virtual OStream& getNodeLabel(OStream& OS, const PassInfo::IDTy &node) const {
		return OS << names.find(node)->second;
	}
    virtual OStream& getEdgeLabel(OStream& OS, const EdgeType &edge) const {
		if (contains(req.begin(),req.end(),edge)) OS << "r";
		if (contains(mod.begin(),mod.end(),edge)) OS << "m";
		if (contains(dstr.begin(),dstr.end(),edge)) OS << "d";
		if (contains(schedule_after.begin(),schedule_after.end(),edge)) OS << "a";
		if (contains(run_before.begin(),run_before.end(),edge)) OS << "b";
		if (contains(schedule_before.begin(),schedule_before.end(),edge)) OS << "s";
		return OS;
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
