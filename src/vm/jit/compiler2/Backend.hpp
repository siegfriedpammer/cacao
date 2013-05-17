/* src/vm/jit/compiler2/Backend.hpp - Backend

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

#ifndef _JIT_COMPILER2_BACKEND
#define _JIT_COMPILER2_BACKEND

#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/LoweredInstDAG.hpp"
#include "vm/jit/compiler2/MachineInstructions.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

class Backend {
protected:
	virtual LoweredInstDAG* lowerBeginInst(BeginInst *I) const = 0;
	virtual LoweredInstDAG* lowerLOADInst(LOADInst *I) const = 0;
public:
	static Backend* factory();
	virtual LoweredInstDAG* lower(Instruction *I) const;

	virtual const char* get_name() const = 0;
};
/**
 * Machine Backend
 *
 * This class containes all target dependent information.
 */
template <typename Target>
class BackendTraits : public Backend {
protected:
	virtual LoweredInstDAG* lowerBeginInst(BeginInst *I) const;
	virtual LoweredInstDAG* lowerLOADInst(LOADInst *I) const;
public:
	virtual const char* get_name() const;
};

template<typename Target>
LoweredInstDAG* BackendTraits<Target>::lowerBeginInst(BeginInst *I) const {
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	MachineLabelInst *label = new MachineLabelInst();
	dag->add(label);
	dag->set_result(label);
	return dag;
}


} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_BACKEND */


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
