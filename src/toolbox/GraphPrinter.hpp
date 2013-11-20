/* toolbox/GraphPrinter.hpp - Graph Printer

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

#ifndef _GRAPH_PRINTER_HPP
#define _GRAPH_PRINTER_HPP

#include "toolbox/OStream.hpp"

#include <string>
#include <set>
#include <map>
#include <utility>

#include <cstdio>

namespace cacao {

template<typename GraphTy, typename NodeTy>
class PrintableGraph {
public:
	typedef NodeTy NodeType;
	typedef std::pair<NodeType,NodeType> EdgeType;
	typedef typename std::set<NodeType> NodeListType;
	typedef typename std::set<NodeType>::iterator iterator;
	typedef typename std::set<NodeType>::const_iterator const_iterator;
	typedef typename std::set<EdgeType>::iterator edge_iterator;
	typedef typename std::set<EdgeType>::const_iterator const_edge_iterator;
	typedef typename std::map<unsigned long,std::set<NodeType> >::iterator cluster_iterator;
	typedef typename std::map<unsigned long,std::set<NodeType> >::const_iterator const_cluster_iterator;
	typedef typename std::map<unsigned long,std::string>::iterator cluster_name_iterator;
	typedef typename std::map<unsigned long,std::string>::const_iterator const_cluster_name_iterator;

protected:
	std::set<NodeType> nodes;
	std::set<EdgeType> edges;
	std::set<EdgeType> successors;
	std::map<unsigned long,NodeListType> clusters;
	std::map<unsigned long,std::string> cluster_name;
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

	cluster_name_iterator cluster_name_begin() {
		return cluster_name.begin();
	}

	const_cluster_name_iterator cluster_name_begin() const {
		return cluster_name.begin();
	}

	cluster_name_iterator cluster_name_end() {
		return cluster_name.end();
	}

	const_cluster_name_iterator cluster_name_end() const {
		return cluster_name.end();
	}

	cluster_name_iterator cluster_name_find(unsigned long k) {
		return cluster_name.find(k);
	}

	const_cluster_name_iterator cluster_name_find(unsigned long k) const {
		return cluster_name.find(k);
	}

	virtual OStream& getGraphName(OStream &OS) const {
		return OS;
	}

	virtual unsigned long getNodeID(const NodeType &node) const {
		return (unsigned long)node;
	}

	virtual OStream& getNodeLabel(OStream &OS, const NodeType &node) const {
		return OS;
	}

	virtual OStream& getNodeAttributes(OStream &OS, const NodeType &node) const {
		return OS;
	}

	virtual OStream& getEdgeLabel(OStream &OS, const EdgeType &e) const {
		return OS;
	}

	virtual OStream& getEdgeAttributes(OStream &OS, const EdgeType &e) const {
		return OS;
	}

};

template <class PrintableGraphTy>
class GraphPrinter {
public:
	static void print(const char *filename, const PrintableGraphTy &G) {
		FILE* file = fopen(filename,"w");
		OStream OS(file);
		printHeader(OS,G);
		printNodes(OS,G);
		printEdges(OS,G);
		printCluster(OS,G);
		printFooter(OS,G);
		fclose(file);
	}

	static void printHeader(OStream &OS, const PrintableGraphTy &G) {
		OS<<"digraph g1 {\n";
		OS<<"node [shape = box];\n";
		OS<<"label = \"";
		G.getGraphName(OS) <<"\";\n";
	}

	static void printNodes(OStream &OS, const PrintableGraphTy &G) {
		for(typename PrintableGraphTy::const_iterator i = G.begin(), e = G.end(); i != e; ++i) {
			typename PrintableGraphTy::NodeType node(*i);
			OS<< "\"node_" << G.getNodeID(node) << "\"" << "[label=\"";
			G.getNodeLabel(OS,node) << "\", ";
			G.getNodeAttributes(OS,node) << "];\n";
		}
	}

	static void printCluster(OStream &OS, const PrintableGraphTy &G) {
		for(typename PrintableGraphTy::const_cluster_iterator i = G.cluster_begin(),
				e = G.cluster_end(); i != e; ++i) {
			unsigned long cid = i->first;
			const std::set<typename PrintableGraphTy::NodeType> &set = i->second;
			OS<<"subgraph cluster_" << cid << " {\n";
			typename PrintableGraphTy::const_cluster_name_iterator name_it = G.cluster_name_find(cid);
			if (name_it != G.cluster_name_end()) {
				OS<<"label = \""<< name_it->second <<"\";\n";
			}
			for (typename std::set<typename PrintableGraphTy::NodeType>::const_iterator ii = set.begin(),
					ee = set.end(); ii != ee; ++ii) {
				typename PrintableGraphTy::NodeType node = (*ii);
				OS<< "\"node_" << G.getNodeID(node) << "\";\n";
			}
			OS<<"}\n";
		}
	}
	static void printEdges(OStream &OS, const PrintableGraphTy &G) {
		for(typename PrintableGraphTy::const_edge_iterator i = G.edge_begin(),
		    e = G.edge_end(); i != e; ++i) {
			typename PrintableGraphTy::NodeType a(i->first);
			typename PrintableGraphTy::NodeType b(i->second);
			OS<< "\"node_" << G.getNodeID(a) << "\" -> " << "\"node_" << G.getNodeID(b) << "\""
			  << "[label=\"";
			G.getEdgeLabel(OS,*i) << "\", ";
			G.getEdgeAttributes(OS,*i) << "];\n";
		}
	}

	static void printFooter(OStream &OS, const PrintableGraphTy &G) {
		OS<<"}\n";
	}
};

} // end namespace cacao

#endif  // _GRAPH_PRINTER_HPP

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
