/* src/vm/jit/compiler2/MachineOperand.hpp - MachineOperand

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

#ifndef _JIT_COMPILER2_MACHINEOPERAND
#define _JIT_COMPILER2_MACHINEOPERAND

#include <vector>
#include <cassert>

namespace cacao {

// forward declarations
class OStream;

namespace jit {
namespace compiler2 {


class MachineOperand {
public:
	enum TYPE {
		NONE,
		REGISTER_VALUE  = 1<<0,
		REGISTER_ADDR   = 1<<1,
		IMMEDIATE_VALUE = 1<<2,
		IMMEDIATE_ADDR  = 1<<3,
		ALL = REGISTER_VALUE | REGISTER_ADDR | IMMEDIATE_ADDR | IMMEDIATE_ADDR
	};
private:
	unsigned type;
public:
	/// default constructor
	MachineOperand() {
		type = NONE;
	}
	MachineOperand(unsigned t) {
		set_type(t);
	}
	/// copy constructor
	MachineOperand(const MachineOperand &MO) {
		type = MO.type;
	}
	/// copy assignment operator
	MachineOperand& operator=(const MachineOperand &rhs) {
		type = rhs.type;
		return *this;
	}
	bool takes(const TYPE t) const {
		return (type & t);
	}
	unsigned get_type() const {
		return type;
	}
	void set_type(unsigned t) {
		assert( t <= ALL);
		type = t;
	}
};


OStream& operator<<(OStream &OS, const MachineOperand &MO);

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_MACHINEOPERAND */


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
