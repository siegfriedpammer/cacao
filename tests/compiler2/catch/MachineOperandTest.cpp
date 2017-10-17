/* tests/compiler2/catch/MachineOperandTest.cpp - MachineOperandTest
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
#include "vm/jit/compiler2/x86_64/X86_64Register.hpp"

using cacao::jit::compiler2::Backend;
using cacao::jit::compiler2::MachineOperand;
using cacao::jit::compiler2::OperandFile;
using cacao::jit::compiler2::VirtualRegister;

using Ty = cacao::jit::compiler2::Type;

using namespace cacao::jit::compiler2::x86_64;

TEST_CASE("Aquivalent works for 2 equivalent native registers", "[X86_64]")
{
	auto reg1 = std::make_unique<NativeRegister>(Ty::LongTypeID, &RDI);
	auto reg2 = std::make_unique<NativeRegister>(Ty::LongTypeID, &RDI);

	REQUIRE(!reg1->aquivalence_less(*reg2));
	REQUIRE(!reg2->aquivalence_less(*reg1));

	REQUIRE(reg1->aquivalent(*reg2));
}

TEST_CASE("Aquivalent works for 2 different native registers", "[x86_64]")
{
	auto reg1 = std::make_unique<NativeRegister>(Ty::LongTypeID, &RDI);
	auto reg2 = std::make_unique<NativeRegister>(Ty::LongTypeID, &RSI);

	REQUIRE(!reg1->aquivalence_less(*reg2));
	REQUIRE(reg2->aquivalence_less(*reg1));

	REQUIRE(!reg1->aquivalent(*reg2));
}

static bool op_cmp(MachineOperand* lhs, MachineOperand* rhs)
{
	return lhs->aquivalence_less(*rhs);
}

TEST_CASE("Set intersection works for native register lists", "[x86_64]")
{
	auto reg1 = std::make_unique<NativeRegister>(Ty::LongTypeID, &RDI);
	auto reg2 = std::make_unique<NativeRegister>(Ty::LongTypeID, &RDI);
	auto reg3 = std::make_unique<NativeRegister>(Ty::LongTypeID, &RSI);

	std::vector<MachineOperand*> vec1 {reg1.get()};
	std::vector<MachineOperand*> vec2 {reg2.get()};
	std::vector<MachineOperand*> vec3 {reg3.get()};

	std::vector<MachineOperand*> intersection1;
	std::vector<MachineOperand*> intersection2;
	
	std::set_intersection(vec1.begin(), vec1.end(), vec2.begin(), vec2.end(), std::back_inserter(intersection1), op_cmp);
	std::set_intersection(vec1.begin(), vec1.end(), vec3.begin(), vec3.end(), std::back_inserter(intersection2), op_cmp);
	
	REQUIRE(intersection1.size() == 1);
	REQUIRE(intersection1.front()->aquivalent(*reg1));

	REQUIRE(intersection2.size() == 0);
}

TEST_CASE("OperandFile works with set intersection", "[x86_64, Special]")
{
	auto vreg = std::make_unique<VirtualRegister>(Ty::LongTypeID);

	auto BE = Backend::factory(nullptr);
	OperandFile op_file;
	BE->get_OperandFile(op_file, vreg.get());

	std::vector<MachineOperand*> operands;
	std::copy(op_file.begin(), op_file.end(), std::back_inserter(operands));
	std::sort(operands.begin(), operands.end(), op_cmp);

	auto reg1 = std::make_unique<NativeRegister>(Ty::LongTypeID, &RDI);

	std::vector<MachineOperand*> vec1 { reg1.get() };
	std::vector<MachineOperand*> intersection;

	std::set_intersection(vec1.begin(), vec1.end(), operands.begin(), operands.end(), std::back_inserter(intersection), op_cmp);
	
	REQUIRE(intersection.size() == 1);
	REQUIRE(intersection.front()->aquivalent(*reg1));
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
