/* src/vm/jit/compiler2/InstructionSchedule.hpp - InstructionSchedule

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

#ifndef _JIT_COMPILER2_INSTRUCTIONSCHEDULE
#define _JIT_COMPILER2_INSTRUCTIONSCHEDULE

#include "future/unordered_map.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

class BeginInst;

/**
 * InstructionSchedule
 * TODO: more info
 */
template <class _Inst>
class InstructionSchedule {
public:
	typedef typename std::vector<_Inst*> InstructionListTy;
	typedef typename InstructionListTy::const_iterator const_inst_iterator;
	typedef typename InstructionListTy::const_reverse_iterator const_reverse_inst_iterator;
protected:
	typedef unordered_map<const BeginInst*, InstructionListTy> MapTy;
	MapTy map;
public:
	InstructionSchedule() {}
	_Inst* operator[](const BeginInst* BI) const {
		return get(BI);
	}
	_Inst* get(const BeginInst* BI) const {
		typename MapTy::const_iterator i = map.find(BI);
		if (i == map.end()) {
			return NULL;
		}
		return i->second;
	}
	const_inst_iterator inst_begin(const BeginInst* BI) const {
		typename MapTy::const_iterator i = map.find(BI);
		assert(i != map.end());
		return i->second.begin();
	}
	const_inst_iterator inst_end(const BeginInst* BI) const {
		typename MapTy::const_iterator i = map.find(BI);
		assert(i != map.end());
		return i->second.end();
	}
	const_reverse_inst_iterator inst_rbegin(const BeginInst* BI) const {
		typename MapTy::const_iterator i = map.find(BI);
		assert(i != map.end());
		return i->second.rbegin();
	}
	const_reverse_inst_iterator inst_rend(const BeginInst* BI) const {
		typename MapTy::const_iterator i = map.find(BI);
		assert(i != map.end());
		return i->second.rend();
	}
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_INSTRUCTIONSCHEDULE */


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
