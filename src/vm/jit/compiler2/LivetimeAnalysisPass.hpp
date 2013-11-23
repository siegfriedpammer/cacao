/* src/vm/jit/compiler2/LivetimeAnalysisPass.hpp - LivetimeAnalysisPass

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

#ifndef _JIT_COMPILER2_LIVETIMEANALYSISPASS
#define _JIT_COMPILER2_LIVETIMEANALYSISPASS

#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/LivetimeInterval.hpp"
#include "vm/jit/compiler2/MachineOperand.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

/**
 * LivetimeAnalysisPass
 *
 * Based on the approach from "Linear scan register allocation on SSA form"
 * by Wimmer and Franz @cite Wimmer2010.
 */
class LivetimeAnalysisPass : public Pass {
public:
	typedef alloc::map<MachineOperand*,LivetimeInterval,MachineOperandComp>::type LivetimeIntervalMapTy;
	typedef LivetimeIntervalMapTy::const_iterator const_iterator;
	typedef LivetimeIntervalMapTy::iterator iterator;

	typedef alloc::set<MachineOperand*>::type LiveInSetTy;
	typedef alloc::map<MachineBasicBlock*,LiveInSetTy>::type LiveInMapTy;
private:


	LivetimeIntervalMapTy lti_map;
	MachineInstructionSchedule *MIS;
public:
	static char ID;
	LivetimeAnalysisPass() : Pass() {}
	virtual bool run(JITData &JD);
	virtual PassUsage& get_PassUsage(PassUsage &PA) const;

	virtual bool verify() const;
	virtual void initialize();

	const_iterator begin() const {
		return lti_map.begin();
	}
	const_iterator end() const {
		return lti_map.end();
	}
	iterator begin() {
		return lti_map.begin();
	}
	iterator end() {
		return lti_map.end();
	}
	std::size_t size() const {
		return lti_map.size();
	}
	LivetimeInterval& get(MachineOperand* operand) {
		iterator i = lti_map.find(operand);
		assert(i != lti_map.end());
		return i->second;
	}
	OStream& print(OStream& OS) const;
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_LIVETIMEANALYSISPASS */


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
