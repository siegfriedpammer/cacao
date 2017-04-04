/* tests/gtest/compiler2/MachineBasicBlock_test.cpp - MachineBasicBlock tests

   Copyright (C) 1996-2013
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

#include "gtest/gtest.h"
#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedule.hpp"
#include "vm/jit/compiler2/MachineInstruction.hpp"

#include <queue>

namespace cacao {
namespace jit {
namespace compiler2 {

namespace {

/**
 * Dummy MachineInstruction
 */
class TestMachineInstruction : public MachineInstruction {
public:
	TestMachineInstruction() : MachineInstruction("TestMachineInstruction", NULL, 0) {}
};

/**
 * check a MachineBasicBlock for size and content
 */
inline void check_MBB(MachineBasicBlock *MBB, unsigned first, unsigned size, unsigned array[]) {
	// check size
	EXPECT_EQ(MBB->size(), size);

	unsigned counter = 0;
	for (MachineBasicBlock::iterator i = MBB->begin(), e = MBB->end();
			i != e; ++i) {
		EXPECT_EQ((*i)->get_id(),first + array[counter++]);
	}
}

} // end anonymous namespace

TEST(MachineBasicBlock, test_push_back) {
	MachineInstructionSchedule MIS;
	MachineBasicBlock *MBB = *MIS.push_front(MBBBuilder());

	// store first id
	MachineInstruction *MI = new TestMachineInstruction();
	unsigned first = MI->get_id();
	MBB->push_back(MI);
	MBB->push_back(new TestMachineInstruction());
	MBB->push_back(new TestMachineInstruction());
	MBB->push_back(new TestMachineInstruction());

	unsigned array[] = {0, 1, 2, 3};
	check_MBB(MBB,first,4,array);
}

TEST(MachineBasicBlock, test_push_front) {
	MachineInstructionSchedule MIS;
	MachineBasicBlock *MBB = *MIS.push_front(MBBBuilder());

	// store first id
	MachineInstruction *MI = new TestMachineInstruction();
	unsigned first = MI->get_id();
	MBB->push_front(MI);
	MBB->push_front(new TestMachineInstruction());
	MBB->push_front(new TestMachineInstruction());
	MBB->push_front(new TestMachineInstruction());

	unsigned array[] = {3, 2 ,1, 0 };
	check_MBB(MBB,first,4,array);
}

TEST(MachineBasicBlock, test_insert_after) {
	MachineInstructionSchedule MIS;
	MachineBasicBlock *MBB = *MIS.push_front(MBBBuilder());

	// store first id
	MachineInstruction *MI = new TestMachineInstruction();
	unsigned first = MI->get_id();
	MBB->push_back(MI);
	MBB->push_back(new TestMachineInstruction());
	MBB->push_back(new TestMachineInstruction());
	MBB->push_back(new TestMachineInstruction());

	EXPECT_EQ(MBB->size(), 4);

	// insert after the third element
	{
		MachineBasicBlock::iterator i = MBB->begin();
		std::advance(i,2);
		MBB->insert_after(i,new TestMachineInstruction());
	}

	unsigned array[] = {0, 1, 2, 4, 3};
	check_MBB(MBB,first,5,array);
}

/**
 * test if the MachineBasicBlock::iterator corretly iterates
 * to a newly inserted element that was inserted after the
 * current one
 */
TEST(MachineBasicBlock, test_insert_after2) {
	MachineInstructionSchedule MIS;
	MachineBasicBlock *MBB = *MIS.push_front(MBBBuilder());

	// store first id
	MachineInstruction *MI = new TestMachineInstruction();
	unsigned first = MI->get_id();
	MBB->push_back(MI);
	MBB->push_back(new TestMachineInstruction());
	MBB->push_back(new TestMachineInstruction());
	MBB->push_back(new TestMachineInstruction());

	EXPECT_EQ(MBB->size(), 4);

	{
		MachineBasicBlock::iterator i = MBB->begin();
		EXPECT_EQ((*i++)->get_id(),first + 0);
		EXPECT_EQ((*i++)->get_id(),first + 1);
		EXPECT_EQ((*i  )->get_id(),first + 2);
		MBB->insert_after(i,new TestMachineInstruction());
		EXPECT_EQ((*i++)->get_id(),first + 2);
		EXPECT_EQ((*i++)->get_id(),first + 4);
		EXPECT_EQ((*i++)->get_id(),first + 3);

		EXPECT_EQ(i,MBB->end());
	}

	unsigned array[] = {0, 1, 2, 4, 3};
	check_MBB(MBB,first,5,array);
}

TEST(MachineBasicBlock, test_insert_before) {
	MachineInstructionSchedule MIS;
	MachineBasicBlock *MBB = *MIS.push_front(MBBBuilder());

	// store first id
	MachineInstruction *MI = new TestMachineInstruction();
	unsigned first = MI->get_id();
	MBB->push_back(MI);
	MBB->push_back(new TestMachineInstruction());
	MBB->push_back(new TestMachineInstruction());
	MBB->push_back(new TestMachineInstruction());

	EXPECT_EQ(MBB->size(), 4);

	// insert before the third element
	{
		MachineBasicBlock::iterator i = MBB->begin();
		std::advance(i,2);
		MBB->insert_before(i,new TestMachineInstruction());
	}

	unsigned array[] = {0, 1, 4, 2, 3};
	check_MBB(MBB,first,5,array);
}

/**
 * test if the MachineBasicBlock::iterator does not iterater
 * to a newly inserted element that was inserted before the
 * current one
 */
TEST(MachineBasicBlock, test_insert_before2) {
	MachineInstructionSchedule MIS;
	MachineBasicBlock *MBB = *MIS.push_front(MBBBuilder());

	// store first id
	MachineInstruction *MI = new TestMachineInstruction();
	unsigned first = MI->get_id();
	MBB->push_back(MI);
	MBB->push_back(new TestMachineInstruction());
	MBB->push_back(new TestMachineInstruction());
	MBB->push_back(new TestMachineInstruction());

	EXPECT_EQ(MBB->size(), 4);

	{
		MachineBasicBlock::iterator i = MBB->begin();
		EXPECT_EQ((*i++)->get_id(),first + 0);
		EXPECT_EQ((*i++)->get_id(),first + 1);
		EXPECT_EQ((*i  )->get_id(),first + 2);
		MBB->insert_before(i,new TestMachineInstruction());
		EXPECT_EQ((*i++)->get_id(),first + 2);
		EXPECT_EQ((*i++)->get_id(),first + 3);

		EXPECT_EQ(i,MBB->end());
	}

	unsigned array[] = {0, 1, 4, 2, 3};
	check_MBB(MBB,first,5,array);
}

/**
 * test the total ordering of MachineInstruction
 */
TEST(MachineBasicBlock, test_order) {
	MachineInstructionSchedule MIS;
	MachineBasicBlock *MBB = *MIS.push_front(MBBBuilder());

	// store first id
	MachineInstruction *MI = new TestMachineInstruction();
	//unsigned first = MI->get_id();
	MBB->push_back(MI);
	MBB->push_back(new TestMachineInstruction());
	MBB->push_back(new TestMachineInstruction());
	MBB->push_back(new TestMachineInstruction());

	EXPECT_EQ(MBB->size(), 4);

	// store MachineBasicBlock::iterators
	MachineBasicBlock::iterator smi[4];

	unsigned counter = 0;
	for (MachineBasicBlock::iterator i = MBB->begin(), e = MBB->end();
			i != e; ++i) {
		smi[counter++] = i;
	}

	EXPECT_LT(smi[0],smi[1]);
	EXPECT_LT(smi[1],smi[2]);
	EXPECT_LT(smi[2],smi[3]);
}

TEST(MachineBasicBlock, test_const_iterator) {
	MachineInstructionSchedule MIS;
	MachineBasicBlock *MBB = *MIS.push_front(MBBBuilder());

	// store first id
	MachineInstruction *MI = new TestMachineInstruction();
	unsigned first = MI->get_id();
	MBB->push_back(MI);
	MBB->push_back(new TestMachineInstruction());
	MBB->push_back(new TestMachineInstruction());
	MBB->push_back(new TestMachineInstruction());

	unsigned array[] = {0, 1, 2, 3};
	unsigned counter = 0;
	for (MachineBasicBlock::const_iterator i = MBB->begin(), e = MBB->end();
			i != e; ++i) {
		EXPECT_EQ((*i)->get_id(),first + array[counter++]);
	}
}

TEST(MachineBasicBlock, test_MIiterator) {
	MachineInstructionSchedule MIS;
	MachineBasicBlock *MBB = *MIS.push_front(MBBBuilder());

	// store first id
	MachineInstruction *MI = new TestMachineInstruction();
	unsigned first = MI->get_id();
	MBB->push_back(MI);
	MBB->push_back(new TestMachineInstruction());
	MBB->push_back(new TestMachineInstruction());
	MBB->push_back(new TestMachineInstruction());

	unsigned array[] = {0, 1, 2, 3};
	unsigned counter = 0;
	for (MIIterator i = MBB->convert(MBB->begin()), e = MBB->convert(MBB->end());
			i != e; ++i) {
		EXPECT_EQ((*i)->get_id(),first + array[counter++]);
	}
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
