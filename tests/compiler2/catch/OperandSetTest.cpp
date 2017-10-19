/* tests/compiler2/catch/OperandSetTest.cpp - OperandSetTest
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

#include "framework/catch.hpp"

#include <memory>
#include <vector>

#include "vm/jit/compiler2/MachineOperand.hpp"
#include "vm/jit/compiler2/MachineOperandFactory.hpp"

using namespace cacao::jit::compiler2;
using Ty = cacao::jit::compiler2::Type;

TEST_CASE("Sets can be constructed from Factory", "[OperandSet]")
{
	MachineOperandFactory MOF;
	MOF.CreateVirtualRegister(Ty::LongTypeID);
	MOF.CreateVirtualRegister(Ty::LongTypeID);
	
	auto set = MOF.EmptySet();

	REQUIRE(set.max_size() == 2);
	REQUIRE(set.size() == 0);
	REQUIRE(set.empty() == true);
}

TEST_CASE("Add, remove, contains, clear work", "[OperandSet]")
{
	MachineOperandFactory MOF;
	auto vreg1 = MOF.CreateVirtualRegister(Ty::LongTypeID);
	auto vreg2 = MOF.CreateVirtualRegister(Ty::LongTypeID);
	auto vreg3 = MOF.CreateVirtualRegister(Ty::LongTypeID);
	
	auto set = MOF.EmptySet();

	set.add(vreg2);
	REQUIRE(set.size() == 1);
	REQUIRE(set.contains(vreg2) == true);

	set.add(vreg3);
	REQUIRE(set.size() == 2);
	REQUIRE(set.contains(vreg3) == true);

	set.remove(vreg2);
	REQUIRE(set.size() == 1);
	REQUIRE(set.contains(vreg2) == false);

	set.clear();
	REQUIRE(set.size() == 0);
	REQUIRE(set.empty() == true);
}

TEST_CASE("ForwardIter implementation works", "[OperandSet]")
{
	MachineOperandFactory MOF;
	auto vreg1 = MOF.CreateVirtualRegister(Ty::LongTypeID);
	auto vreg2 = MOF.CreateVirtualRegister(Ty::LongTypeID);
	auto vreg3 = MOF.CreateVirtualRegister(Ty::LongTypeID);
	auto vreg4 = MOF.CreateVirtualRegister(Ty::LongTypeID);
	
	auto set = MOF.EmptySet();

	set.add(vreg2);
	set.add(vreg4);

	auto iter = set.cbegin();

	REQUIRE(iter->aquivalent(*vreg2));

	++iter;
	REQUIRE(iter->aquivalent(*vreg4));

	++iter;
	REQUIRE(iter == set.cend());
}

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
