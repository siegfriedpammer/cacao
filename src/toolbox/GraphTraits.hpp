/* toolbox/GraphTraits.hpp - Graph Traits

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

#ifndef _GRAPH_TRAITS_HPP
#define _GRAPH_TRAITS_HPP

#include "toolbox/OStream.hpp"
#include "toolbox/StringBuf.hpp"

#include <set>
#include <map>
#include <utility>

#include <cstdio>

namespace cacao {

/* The following parts are inspired by the LLVM GraphTraits framework */

template<typename GraphTy, typename NodeTy>
class GraphTraits {
public:
	typedef NodeTy NodeType;
	typedef std::pair<NodeType*,NodeType*> EdgeType;
	typedef typename std::set<NodeType*> NodeListType;
	typedef typename std::set<NodeType*>::iterator iterator;
	typedef typename std::set<NodeType*>::const_iterator const_iterator;
	typedef typename std::set<EdgeType>::iterator edge_iterator;
	typedef typename std::set<EdgeType>::const_iterator const_edge_iterator;
	typedef typename std::map<unsigned long,std::set<NodeType*> >::iterator cluster_iterator;
	typedef typename std::map<unsigned long,std::set<NodeType*> >::const_iterator const_cluster_iterator;

protected:
	std::set<NodeType*> nodes;
	std::set<EdgeType> edges;
	std::set<EdgeType> successors;
	std::map<unsigned long,NodeListType> clusters;
public:

	const_iterator begin() const {
		return nodes.begin();
	}

	const_iterator end() const {
		return nodes.end();
	}

	iterator begin() {
		return nodes.begin();
	}

	iterator end() {
		return nodes.end();
	}

	const_edge_iterator edge_begin() const {
		return edges.begin();
	}

	const_edge_iterator edge_end() const {
		return edges.end();
	}

	edge_iterator edge_begin() {
		return edges.begin();
	}

	edge_iterator edge_end() {
		return edges.end();
	}

	cluster_iterator cluster_begin() {
		return clusters.begin();
	}

	const_cluster_iterator cluster_begin() const {
		return clusters.begin();
	}

	cluster_iterator cluster_end() {
		return clusters.end();
	}

	const_cluster_iterator cluster_end() const {
		return clusters.end();
	}

	StringBuf getGraphName() const {
		return "";
	}

	unsigned long getNodeID(const GraphTraits<GraphTy,NodeTy>::NodeType &node) const {
		return (unsigned long)&node;
	}

	StringBuf getNodeLabel(const GraphTraits<GraphTy,NodeTy>::NodeType &node) const {
		return "";
	}

	StringBuf getNodeAttributes(const GraphTraits<GraphTy,NodeTy>::NodeType &node) const {
		return "";
	}

	StringBuf getEdgeLabel(const GraphTraits<GraphTy,NodeTy>::EdgeType &e) const {
		return "";
	}

	StringBuf getEdgeAttributes(const GraphTraits<GraphTy,NodeTy>::EdgeType &e) const {
		return "";
	}
};

template <class GraphTraitsTy>
class GraphPrinter {
public:
	static void print(const char *filename, const GraphTraitsTy &G) {
		FILE* file = fopen(filename,"w");
		OStream OS(file);
		printHeader(OS,G);
		printNodes(OS,G);
		printEdges(OS,G);
		printCluster(OS,G);
		printFooter(OS,G);
		fclose(file);
	}

	static void printHeader(OStream &OS, const GraphTraitsTy &G) {
		OS<<"digraph g1 {\n";
		OS<<"node [shape = box];\n";
		OS<<"label = \""<< G.getGraphName() <<"\";\n";
	}

	static void printNodes(OStream &OS, const GraphTraitsTy &G) {
		for(typename GraphTraitsTy::const_iterator i = G.begin(), e = G.end(); i != e; ++i) {
			typename GraphTraitsTy::NodeType &node(**i);
			OS<< "\"node_" << G.getNodeID(node) << "\"" << "[label=\"" << G.getNodeLabel(node)
			  << "\", " << G.getNodeAttributes(node) << "];\n";
		}
	}

	static void printCluster(OStream &OS, const GraphTraitsTy &G) {
		for(typename GraphTraitsTy::const_cluster_iterator i = G.cluster_begin(),
				e = G.cluster_end(); i != e; ++i) {
			unsigned long cid = i->first;
			const std::set<typename GraphTraitsTy::NodeType*> &set = i->second;
			OS<<"subgraph cluster_" << cid << " {\n";
			//OS<<"label = \""<< sb <<"\";\n";
			for (typename std::set<typename GraphTraitsTy::NodeType*>::const_iterator ii = set.begin(),
					ee = set.end(); ii != ee; ++ii) {
				typename GraphTraitsTy::NodeType &node = (**ii);
				OS<< "\"node_" << G.getNodeID(node) << "\";\n";
			}
			OS<<"}\n";
		}
	}
	static void printEdges(OStream &OS, const GraphTraitsTy &G) {
		for(typename GraphTraitsTy::const_edge_iterator i = G.edge_begin(),
		    e = G.edge_end(); i != e; ++i) {
			typename GraphTraitsTy::NodeType &a(*i->first);
			typename GraphTraitsTy::NodeType &b(*i->second);
			OS<< "\"node_" << G.getNodeID(a) << "\" -> " << "\"node_" << G.getNodeID(b) << "\""
			  << "[label=\"" << G.getEdgeLabel(*i)
			  << "\", " << G.getEdgeAttributes(*i) << "];\n";
		}
	}

	static void printFooter(OStream &OS, const GraphTraitsTy &G) {
		OS<<"}\n";
	}
};

} // end namespace cacao

#endif  // _GRAPH_TRAITS_HPP

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
