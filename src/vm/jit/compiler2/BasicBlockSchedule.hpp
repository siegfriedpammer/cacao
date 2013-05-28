/* src/vm/jit/compiler2/BasicBlockSchedule.hpp - BasicBlockSchedule

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

#ifndef _JIT_COMPILER2_BASICBLOCKSCHEDULE
#define _JIT_COMPILER2_BASICBLOCKSCHEDULE

#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/Method.hpp"
#include <map>

namespace cacao {
namespace jit {
namespace compiler2 {


/**
 * BasicBlockSchedule
 * TODO: more info
 */
class BasicBlockSchedule {
protected:
	std::map<const Instruction*, BeginInst*> map;
	void set_schedule(const Method* M) {
		map.clear();
		for (Method::InstructionListTy::const_iterator i = M->begin(),
				e = M->end() ; i != e ; ++i) {
			Instruction *I = *i;
			BeginInst *BI= I->get_BeginInst();
			if (BI) {
				map[I] = BI;
			}
		}
	}
public:
	BasicBlockSchedule() {}
	BeginInst* operator[](const Instruction* I) const {
		return get(I);
	}
	BeginInst* get(const Instruction* I) const {
		std::map<const Instruction*, BeginInst*>::const_iterator i = map.find(I);
		if (i == map.end()) {
			return NULL;
		}
		return i->second;
	}
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_BASICBLOCKSCHEDULE */


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
