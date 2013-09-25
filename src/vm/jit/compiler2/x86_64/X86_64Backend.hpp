/* src/vm/jit/compiler2/X86_64Backend.hpp - X86_64Backend

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

#ifndef _JIT_COMPILER2_X86_64BACKEND
#define _JIT_COMPILER2_X86_64BACKEND

#include "vm/jit/compiler2/x86_64/X86_64.hpp"
#include "vm/jit/compiler2/Backend.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace x86_64 {


class LoweringVisitor : public LoweringVisitorBase {
public:
	LoweringVisitor(Backend *backend) : LoweringVisitorBase(backend) {}

	// make LoweringVisitorBases visit visible
	using LoweringVisitorBase::visit;

	virtual void visit(LOADInst *I);
	virtual void visit(IFInst *I);
	virtual void visit(ADDInst *I);
	virtual void visit(ANDInst *I);
	virtual void visit(SUBInst *I);
	virtual void visit(MULInst *I);
	virtual void visit(DIVInst *I);
	virtual void visit(REMInst *I);
	virtual void visit(RETURNInst *I);
	virtual void visit(CASTInst *I);
	virtual void visit(INVOKESTATICInst *I);
	virtual void visit(GETSTATICInst *I);
	virtual void visit(LOOKUPSWITCHInst *I);
};

} // end namespace x86_64
} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_X86_64BACKEND */


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
