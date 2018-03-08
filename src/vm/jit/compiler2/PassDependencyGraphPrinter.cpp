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
#include "toolbox/logging.hpp"
#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/alloc/map.hpp"
#include "vm/jit/compiler2/alloc/set.hpp"

#include <algorithm>
#include <cstring>

#define DEBUG_NAME "compiler2/PassDependencyGraphPrinter"

namespace cacao {
namespace jit {
namespace compiler2 {

namespace {

struct PassArtifactNode {
	const char* name;
	bool is_pass;
	bool is_artifact;
	bool is_enabled;
	unsigned long id;

	static unsigned long id_counter;

	PassArtifactNode() : PassArtifactNode("illegal") {}
	PassArtifactNode(const char* name)
	    : name(name), is_pass(false), is_artifact(false), is_enabled(true), id(id_counter++)
	{
	}
	PassArtifactNode(const PassArtifactNode& other) = default;

	bool operator==(const PassArtifactNode& other) const
	{
		return std::strcmp(name, other.name) == 0;
	}

	operator unsigned long() const { return id; }
};

unsigned long PassArtifactNode::id_counter = 0;

template <class InputIterator, class ValueType>
inline bool contains(InputIterator begin, InputIterator end, const ValueType& val)
{
	return std::find(begin, end, val) != end;
}

class PassDependencyGraphPrinter : public PrintableGraph<PassManager, PassArtifactNode> {
private:
	std::map<ArtifactInfo::IDTy, PassArtifactNode> artifact_map;
	std::map<PassInfo::IDTy, PassArtifactNode> pass_map;

	alloc::set<EdgeType>::type requires;
	alloc::set<EdgeType>::type provides;
	alloc::set<EdgeType>::type modifies;
	alloc::set<EdgeType>::type after;
	alloc::set<EdgeType>::type before;
	alloc::set<EdgeType>::type imm_after;
	alloc::set<EdgeType>::type imm_before;

public:
	PassDependencyGraphPrinter(PassManager& PM)
	{
		std::vector<PassArtifactNode> node_list;

		for (auto i = PM.registered_begin(), e = PM.registered_end(); i != e; ++i) {
			PassArtifactNode node(i->second->get_name());
			node.is_pass = true;
			node_list.push_back(node);
			pass_map.emplace(i->first, node);
		}

		for (auto i = PM.registered_artifacts_begin(), e = PM.registered_artifacts_end(); i != e;
		     ++i) {
			PassArtifactNode node(i->second->get_name());
			node.is_artifact = true;

			auto iter = std::find_if(node_list.begin(), node_list.end(),
			                         [&](const auto& other) { return node == other; });

			if (iter != node_list.end()) {
				iter->is_artifact = true;
				artifact_map.emplace(i->first, *iter);
			}
			else {
				node_list.push_back(node);
				artifact_map.emplace(i->first, node);
			}
		}

		for (auto i = PM.registered_begin(), e = PM.registered_end(); i != e; ++i) {
			std::unique_ptr<Pass> pass(i->second->create_Pass());
			PassUsage PU;
			pass->get_PassUsage(PU);

			PassArtifactNode tmp(i->second->get_name());
			auto iter = std::find_if(node_list.begin(), node_list.end(),
			                         [&](const auto& other) { return tmp == other; });
			auto& node = *iter;
			node.is_enabled = pass->is_enabled();

			std::for_each(PU.requires_begin(), PU.requires_end(), [&](const auto& aid) {
				edges.emplace(node, artifact_map[aid]);
				requires.emplace(node, artifact_map[aid]);
			});

			std::for_each(PU.provides_begin(), PU.provides_end(), [&](const auto& aid) {
				edges.emplace(node, artifact_map[aid]);
				provides.emplace(node, artifact_map[aid]);
			});

			std::for_each(PU.modifies_begin(), PU.modifies_end(), [&](const auto& aid) {
				edges.emplace(node, artifact_map[aid]);
				modifies.emplace(node, artifact_map[aid]);
			});

			std::for_each(PU.after_begin(), PU.after_end(), [&](const auto& pid) {
				edges.emplace(node, pass_map[pid]);
				after.emplace(node, pass_map[pid]);
			});

			std::for_each(PU.before_begin(), PU.before_end(), [&](const auto& pid) {
				edges.emplace(node, pass_map[pid]);
				before.emplace(node, pass_map[pid]);
			});

			std::for_each(PU.imm_after_begin(), PU.imm_after_end(), [&](const auto& pid) {
				edges.emplace(node, pass_map[pid]);
				imm_after.emplace(node, pass_map[pid]);
			});

			std::for_each(PU.imm_before_begin(), PU.imm_before_end(), [&](const auto& pid) {
				edges.emplace(node, pass_map[pid]);
				imm_before.emplace(node, pass_map[pid]);
			});
		}

		nodes.insert(node_list.begin(), node_list.end());
	}

	OStream& getGraphName(OStream& OS) const override { return OS << "PassDependencyGraph"; }

	OStream& getNodeLabel(OStream& OS, const PassArtifactNode& node) const override
	{
		return OS << node.name;
	}

	OStream& getNodeAttributes(OStream& OS, const PassArtifactNode& node) const override
	{
		if (!node.is_enabled)
			return OS << "color=\"gray\" fontcolor=\"gray\"";

		if (node.is_pass && node.is_artifact)
			OS << "color=\"darkviolet\"";
		else if (node.is_artifact)
			OS << "color=\"blue\"";
		return OS;
	}

	OStream& getEdgeLabel(OStream& OS, const EdgeType& edge) const override
	{
		if (contains(requires.begin(), requires.end(), edge)) OS << "r";
		if (contains(provides.begin(), provides.end(), edge)) OS << "p";
		if (contains(modifies.begin(), modifies.end(), edge)) OS << "m";
		if (contains(after.begin(), after.end(), edge)) OS << "a";
		if (contains(before.begin(), before.end(), edge)) OS << "b";
		if (contains(imm_after.begin(), imm_after.end(), edge)) OS << "ia";
		if (contains(imm_before.begin(), imm_before.end(), edge)) OS << "ib";
		return OS;
	}
};

} // end anonymous namespace

// run pass
void print_PassDependencyGraph()
{
	GraphPrinter<PassDependencyGraphPrinter>::print("PassDependencyGraph.dot",
	                                                PassDependencyGraphPrinter(PassManager::get()));
}

} // namespace compiler2
} // end namespace jit
} // namespace cacao

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
