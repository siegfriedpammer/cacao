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
inline void check_MBB(MachineBasicBlock &MBB, unsigned first, unsigned size, unsigned array[]) {
	// check size
	EXPECT_EQ(MBB.size(), size);

	unsigned counter = 0;
	for (MIIterator i = MBB.begin(), e = MBB.end();
			i != e; ++i) {
		EXPECT_EQ((*i)->get_id(),first + array[counter++]);
	}
}

} // end anonymous namespace

TEST(MachineBasicBlock, test_push_back) {
	MachineBasicBlock MBB;

	// store first id
	MachineInstruction *MI = new TestMachineInstruction();
	unsigned first = MI->get_id();
	MBB.push_back(MI);
	MBB.push_back(new TestMachineInstruction());
	MBB.push_back(new TestMachineInstruction());
	MBB.push_back(new TestMachineInstruction());

	unsigned array[] = {0, 1, 2, 3};
	check_MBB(MBB,first,4,array);
}

TEST(MachineBasicBlock, test_push_front) {
	MachineBasicBlock MBB;

	// store first id
	MachineInstruction *MI = new TestMachineInstruction();
	unsigned first = MI->get_id();
	MBB.push_front(MI);
	MBB.push_front(new TestMachineInstruction());
	MBB.push_front(new TestMachineInstruction());
	MBB.push_front(new TestMachineInstruction());

	unsigned array[] = {3, 2 ,1, 0 };
	check_MBB(MBB,first,4,array);
}

TEST(MachineBasicBlock, test_insert_after) {
	MachineBasicBlock MBB;

	// store first id
	MachineInstruction *MI = new TestMachineInstruction();
	unsigned first = MI->get_id();
	MBB.push_back(MI);
	MBB.push_back(new TestMachineInstruction());
	MBB.push_back(new TestMachineInstruction());
	MBB.push_back(new TestMachineInstruction());

	EXPECT_EQ(MBB.size(), 4);

	// insert after the third element
	{
		MIIterator i = MBB.begin();
		std::advance(i,2);
		MBB.insert_after(i,new TestMachineInstruction());
	}

	unsigned array[] = {0, 1, 2, 4, 3};
	check_MBB(MBB,first,5,array);
}

/**
 * test if the MIIterator corretly iterates
 * to a newly inserted element that was inserted after the
 * current one
 */
TEST(MachineBasicBlock, test_insert_after2) {
	MachineBasicBlock MBB;

	// store first id
	MachineInstruction *MI = new TestMachineInstruction();
	unsigned first = MI->get_id();
	MBB.push_back(MI);
	MBB.push_back(new TestMachineInstruction());
	MBB.push_back(new TestMachineInstruction());
	MBB.push_back(new TestMachineInstruction());

	EXPECT_EQ(MBB.size(), 4);

	{
		MIIterator i = MBB.begin();
		EXPECT_EQ((*i++)->get_id(),first + 0);
		EXPECT_EQ((*i++)->get_id(),first + 1);
		EXPECT_EQ((*i  )->get_id(),first + 2);
		MBB.insert_after(i,new TestMachineInstruction());
		EXPECT_EQ((*i++)->get_id(),first + 2);
		EXPECT_EQ((*i++)->get_id(),first + 4);
		EXPECT_EQ((*i++)->get_id(),first + 3);

		EXPECT_EQ(i,MBB.end());
	}

	unsigned array[] = {0, 1, 2, 4, 3};
	check_MBB(MBB,first,5,array);
}

TEST(MachineBasicBlock, test_insert_before) {
	MachineBasicBlock MBB;

	// store first id
	MachineInstruction *MI = new TestMachineInstruction();
	unsigned first = MI->get_id();
	MBB.push_back(MI);
	MBB.push_back(new TestMachineInstruction());
	MBB.push_back(new TestMachineInstruction());
	MBB.push_back(new TestMachineInstruction());

	EXPECT_EQ(MBB.size(), 4);

	// insert before the third element
	{
		MIIterator i = MBB.begin();
		std::advance(i,2);
		MBB.insert_before(i,new TestMachineInstruction());
	}

	unsigned array[] = {0, 1, 4, 2, 3};
	check_MBB(MBB,first,5,array);
}

/**
 * test if the MIIterator does not iterater
 * to a newly inserted element that was inserted before the
 * current one
 */
TEST(MachineBasicBlock, test_insert_before2) {
	MachineBasicBlock MBB;

	// store first id
	MachineInstruction *MI = new TestMachineInstruction();
	unsigned first = MI->get_id();
	MBB.push_back(MI);
	MBB.push_back(new TestMachineInstruction());
	MBB.push_back(new TestMachineInstruction());
	MBB.push_back(new TestMachineInstruction());

	EXPECT_EQ(MBB.size(), 4);

	{
		MIIterator i = MBB.begin();
		EXPECT_EQ((*i++)->get_id(),first + 0);
		EXPECT_EQ((*i++)->get_id(),first + 1);
		EXPECT_EQ((*i  )->get_id(),first + 2);
		MBB.insert_before(i,new TestMachineInstruction());
		EXPECT_EQ((*i++)->get_id(),first + 2);
		EXPECT_EQ((*i++)->get_id(),first + 3);

		EXPECT_EQ(i,MBB.end());
	}

	unsigned array[] = {0, 1, 4, 2, 3};
	check_MBB(MBB,first,5,array);
}

/**
 * test the total ordering of MachineInstruction
 */
TEST(MachineBasicBlock, test_order) {
	MachineBasicBlock MBB;

	// store first id
	MachineInstruction *MI = new TestMachineInstruction();
	unsigned first = MI->get_id();
	MBB.push_back(MI);
	MBB.push_back(new TestMachineInstruction());
	MBB.push_back(new TestMachineInstruction());
	MBB.push_back(new TestMachineInstruction());

	EXPECT_EQ(MBB.size(), 4);

	// store MIIterators
	MIIterator smi[4];

	unsigned counter = 0;
	for (MIIterator i = MBB.begin(), e = MBB.end();
			i != e; ++i) {
		smi[counter++] = i;
	}

	EXPECT_LT(smi[0],smi[1]);
	EXPECT_LT(smi[1],smi[2]);
	EXPECT_LT(smi[2],smi[3]);
}

/**
 * test the total ordering of MachineInstruction
 */
TEST(MachineBasicBlock, test_order2) {
	MachineBasicBlock MBB;

	// store first id
	MachineInstruction *MI = new TestMachineInstruction();
	unsigned first = MI->get_id();
	MBB.push_back(MI);
	MBB.push_back(new TestMachineInstruction());
	MBB.push_back(new TestMachineInstruction());
	MBB.push_back(new TestMachineInstruction());

	EXPECT_EQ(MBB.size(), 4);

	// store MIIterators
	MIIterator smi[4];

	unsigned counter = 0;
	for (MIIterator i = MBB.begin(), e = MBB.end();
			i != e; ++i) {
		smi[counter++] = i;
	}

	EXPECT_LT(smi[0],smi[1]);
	EXPECT_LT(smi[1],smi[2]);
	EXPECT_LT(smi[2],smi[3]);

	// insert after the third element
	MIIterator inserted = MBB.begin();
	std::advance(inserted,2);
	MBB.insert_after(inserted,new TestMachineInstruction());
	++inserted;

	EXPECT_EQ((*inserted)->get_id(), first + 4);

	unsigned array[] = {0, 1, 2, 4, 3};
	check_MBB(MBB,first,5,array);

	EXPECT_LT(smi[0],inserted);
	EXPECT_LT(smi[1],inserted);
	EXPECT_LT(smi[2],inserted);
	EXPECT_GT(smi[3],inserted);
}

/**
 * test priority queue
 */
TEST(MachineBasicBlock, test_priority_queue) {
	MachineBasicBlock MBB;

	// store first id
	MachineInstruction *MI = new TestMachineInstruction();
	unsigned first = MI->get_id();
	MBB.push_back(MI);
	MBB.push_back(new TestMachineInstruction());
	MBB.push_back(new TestMachineInstruction());
	MBB.push_back(new TestMachineInstruction());

	EXPECT_EQ(MBB.size(), 4);

	std::priority_queue<MIIterator,
		std::vector<MIIterator>,
		std::greater<MIIterator> > q;

	for (MIIterator i = MBB.begin(), e = MBB.end();
			i != e; ++i) {
		q.push(i);
	}

	EXPECT_EQ((*q.top())->get_id(), first + 0); q.pop();
	EXPECT_EQ((*q.top())->get_id(), first + 1); q.pop();
	EXPECT_EQ((*q.top())->get_id(), first + 2); q.pop();
	EXPECT_EQ((*q.top())->get_id(), first + 3); q.pop();

	EXPECT_TRUE(q.empty());
}

/**
 * test priority queue
 */
TEST(MachineBasicBlock, test_priority_queue1) {
	MachineBasicBlock MBB;

	// store first id
	MachineInstruction *MI = new TestMachineInstruction();
	unsigned first = MI->get_id();
	MBB.push_back(MI);
	MBB.push_back(new TestMachineInstruction());
	MBB.push_back(new TestMachineInstruction());
	MBB.push_back(new TestMachineInstruction());

	EXPECT_EQ(MBB.size(), 4);

	std::priority_queue<MIIterator,
		std::vector<MIIterator>,
		std::greater<MIIterator> > q;

	for (MIIterator i = MBB.begin(), e = MBB.end();
			i != e; ++i) {
		q.push(i);
	}

	EXPECT_EQ((*q.top())->get_id(), first + 0); q.pop();
	EXPECT_EQ((*q.top())->get_id(), first + 1); q.pop();
	EXPECT_EQ((*q.top())->get_id(), first + 2); q.pop();
	// insert after the second element and add to queue
	{
		MIIterator i = ++MBB.begin();
		MBB.insert_after(i,new TestMachineInstruction());
		++i;
		EXPECT_EQ((*i)->get_id(), first + 4);
		q.push(i);
	}
	EXPECT_EQ((*q.top())->get_id(), first + 4); q.pop();
	EXPECT_EQ((*q.top())->get_id(), first + 3); q.pop();

	EXPECT_TRUE(q.empty());
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
