/* src/vm/jit/compiler2/MachineOperandSet.cpp - MachineOperandSet

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

#include "vm/jit/compiler2/MachineOperandSet.hpp"

#include "vm/jit/compiler2/MachineOperand.hpp"
#include "vm/jit/compiler2/MachineOperandFactory.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

void OperandSet::add(const MachineOperand* operand)
{
	assert(operand->get_id() < set.size());
	set[operand->get_id()] = true;
}

void OperandSet::remove(const MachineOperand* operand)
{
	assert(operand->get_id() < set.size());
	set[operand->get_id()] = false;
}

bool OperandSet::contains(const MachineOperand* operand) const
{
	assert(operand->get_id() < set.size());
	return set[operand->get_id()];
}

void OperandSet::clear()
{
	set.reset();
}

void OperandSet::swap(OperandSet& other)
{
	assert(set.size() == other.set.size() &&
	       "We only allow operand sets of same max size to be swapped!");
	set.swap(other.set);
}

OperandSet& OperandSet::operator-=(const OperandSet& other)
{
	set -= other.set;
	return *this;
}

OperandSet& OperandSet::operator|=(const OperandSet& other)
{
	set |= other.set;
	return *this;
}

OperandSet& OperandSet::operator&=(const OperandSet& other)
{
	set &= other.set;
	return *this;
}

MachineOperand* OperandSet::get(size_t index)
{
	assert(index >= 0 && index < set.size() && "Index out-of-bounds!");
	assert(set[index] && "Operand is not a member of this set!");

	return MOF->operands[index].get();
}

const MachineOperand* OperandSet::get(size_t index) const
{
	assert(index >= 0 && index < set.size() && "Index out-of-bounds!");
	assert(set[index] && "Operand is not a member of this set!");

	return MOF->operands[index].get();
}

std::unique_ptr<OperandList> OperandSet::ToList()
{
	auto list = std::make_unique<OperandList>();

	for (auto& operand : *this) {
		list->push_back(&operand);
	}

	return list;
}

OperandSet& OperandSet::operator=(const OperandList& list)
{
	clear();
	std::for_each(list.cbegin(), list.cend(), [&](const auto operand) { this->add(operand); });
	return *this;
}

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
