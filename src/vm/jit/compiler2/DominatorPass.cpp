/* src/vm/jit/compiler2/DominatorPass.cpp - DominatorPass

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

#include "vm/jit/compiler2/DominatorPass.hpp"

#include "vm/jit/compiler2/CFGMetaPass.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

template <>
DominatorPass::NodeTy* DominatorPass::get_init_node(JITData& JD)
{
	auto method = JD.get_Method();
	return method->get_init_bb();
}

template<>
std::size_t DominatorPass::get_nodes_size(JITData &JD)
{
	auto method = JD.get_Method();
	return method->bb_size();
}

template<>
auto DominatorPass::node_begin(JITData &JD)
{
	auto method = JD.get_Method();
	return method->bb_begin();
}

template<>
auto DominatorPass::node_end(JITData &JD)
{
	auto method = JD.get_Method();
	return method->bb_end();
}

template<>
EndInst::succ_const_iterator DominatorPass::succ_begin(DominatorPass::NodeTy* v)
{
	EndInst *ve = v->get_EndInst();
	assert(ve);
	return ve->succ_begin();
}

template<>
EndInst::succ_const_iterator DominatorPass::succ_end(DominatorPass::NodeTy* v)
{
	EndInst *ve = v->get_EndInst();
	assert(ve);
	return ve->succ_end();
}

template<>
template<>
inline DominatorPass::NodeTy* DominatorPass::get(BeginInstRef ref) {
	return ref.get();
}

// pass usage
template <>
PassUsage& DominatorPass::get_PassUsage(PassUsage& PU) const
{
	PU.add_requires<CFGMetaPass>();
	return PU;
}

// register pass
static PassRegistry<DominatorPass> X("DominatorPass");

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
