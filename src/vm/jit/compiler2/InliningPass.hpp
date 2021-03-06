/* src/vm/jit/compiler2/InliningPass.hpp - InliningPass

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

#ifndef _JIT_COMPILER2_INLININGPASS
#define _JIT_COMPILER2_INLININGPASS

#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/alloc/unordered_map.hpp"

MM_MAKE_NAME(InliningPass)

namespace cacao {
namespace jit {
namespace compiler2 {

class Instruction;
class INVOKESTATICInst;
class INVOKESPECIALInst;
class INVOKEInst;
class BeginInst;
class PHIInst;
class JITData;
class Method;

/**
 * InliningPass. 
 * 
 * The inliner tries to reduce the performance problems introduced
 * by methods. This is done by copying the SSA-Graph of the callee
 * to the caller. The goal is to reduce the overhead introduced
 * by method invocation and offer more optimization
 * possibilities to subsequent passes.
 * 
 * See @cite SchwarzingerBScThesis for further details.
 */
class InliningPass : public Pass, public memory::ManagerMixin<InliningPass> {
private: 
   Method* M; // used for verification
public:
	static Option<bool> enabled;
	InliningPass() : Pass() {}
	virtual bool run(JITData& JD);
   virtual bool verify() const;
	virtual PassUsage& get_PassUsage(PassUsage& PU) const;
	bool is_enabled() const override { return InliningPass::enabled; }
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_INLININGPASS */

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
