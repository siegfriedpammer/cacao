/* src/vm/jit/compiler2/GlobalSchedule.hpp - GlobalSchedule

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

#ifndef _JIT_COMPILER2_GLOBALSCHEDULE
#define _JIT_COMPILER2_GLOBALSCHEDULE

#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/Method.hpp"
#include <map>
#include <set>

namespace cacao {
namespace jit {
namespace compiler2 {


/**
 * GlobalSchedule
 * TODO: more info
 */
class GlobalSchedule {
public:
	typedef std::set<Instruction*>::const_iterator const_inst_iterator;
protected:
	std::map<const Instruction*, BeginInst*> map;
	std::map<const BeginInst*, std::set<Instruction*> > bb_map;
	void set_schedule(const Method* M) {
		map.clear();
		for (Method::InstructionListTy::const_iterator i = M->begin(),
				e = M->end() ; i != e ; ++i) {
			Instruction *I = *i;
			BeginInst *BI= I->get_BeginInst();
			if (BI) {
				map[I] = BI;
				bb_map[BI].insert(I);
			}
		}
	}
public:
	GlobalSchedule() {}
	void add_Instruction(Instruction *I, BeginInst* BI) {
		map[I] = BI;
		bb_map[BI].insert(I);
	}

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
	const_inst_iterator inst_begin(const BeginInst* BI) const {
		std::map<const BeginInst*, std::set<Instruction*> >::const_iterator i = bb_map.find(BI);
		assert(i != bb_map.end());
		return i->second.begin();
	}
	const_inst_iterator inst_end(const BeginInst* BI) const {
		std::map<const BeginInst*, std::set<Instruction*> >::const_iterator i = bb_map.find(BI);
		assert(i != bb_map.end());
		return i->second.end();
	}
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_GLOBALSCHEDULE */


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
