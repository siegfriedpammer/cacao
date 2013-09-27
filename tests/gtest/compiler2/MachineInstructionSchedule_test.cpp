/* tests/gtest/compiler2/MachineInstructionSchedule_test.cpp - MachineInstructionSchedule tests

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

template <class _Iterator>
void check(_Iterator begin, _Iterator end, MachineBasicBlock *expect[],size_t size) {
	size_t counter = 0;
	for (; begin != end; ++begin) {
		EXPECT_EQ(expect[counter++], *begin);
	}
	EXPECT_EQ(counter, size);
}

}

TEST(MachineInstructionSchedule, test_reverse) {
	MachineInstructionSchedule MIS;

	MachineBasicBlock *BB1 = *MIS.push_back(MBBBuilder());
	MachineBasicBlock *BB2 = *MIS.push_back(MBBBuilder());
	MachineBasicBlock *BB3 = *MIS.push_back(MBBBuilder());
	MachineBasicBlock *BB4 = *MIS.push_back(MBBBuilder());

	EXPECT_EQ(MIS.size(), 4);

	{
		MachineBasicBlock *expect[] = {BB1, BB2, BB3, BB4};
		check(MIS.begin(), MIS.end(), expect,4);
	}
	{
		MachineBasicBlock *expect[] = {BB4, BB3, BB2, BB1};
		check(MIS.rbegin(), MIS.rend(), expect,4);
	}
	const MachineInstructionSchedule &cMIS = MIS;
	{
		MachineBasicBlock *expect[] = {BB1, BB2, BB3, BB4};
		check(cMIS.begin(), cMIS.end(), expect,4);
	}
	{
		MachineBasicBlock *expect[] = {BB4, BB3, BB2, BB1};
		check(cMIS.rbegin(), cMIS.rend(), expect,4);
	}
}

#if 0
/**
 * test the total ordering of MachineBasicBlock
 */
TEST(MachineInstructionSchedule, test_order2) {
	MachineInstructionSchedule MIS;
	MachineBasicBlock *MBB = *MIS.push_front(MBBBuilder());

	// store first id
	unsigned first = MBB->get_id();
	MIS.push_back(MBB);
	MIS.push_back(new MachineBasicBlock());
	MIS.push_back(new MachineBasicBlock());
	MIS.push_back(new MachineBasicBlock());

	EXPECT_EQ(MIS.size(), 4);

	// store MBBIterators
	MBBIterator smi[4];

	unsigned counter = 0;
	for (MBBIterator i = MIS.begin(), e = MIS.end();
			i != e; ++i) {
		smi[counter++] = i;
	}

	EXPECT_LT(smi[0],smi[1]);
	EXPECT_LT(smi[1],smi[2]);
	EXPECT_LT(smi[2],smi[3]);

	// insert after the third element
	MBBIterator inserted = MIS.begin();
	std::advance(inserted,2);
	MIS.insert_after(inserted,new MachineBasicBlock());
	++inserted;

	EXPECT_EQ((*inserted)->get_id(), first + 4);

	unsigned array[] = {0, 1, 2, 4, 3};
	check_MIS(MIS,first,5,array);

	EXPECT_LT(smi[0],inserted);
	EXPECT_LT(smi[1],inserted);
	EXPECT_LT(smi[2],inserted);
	EXPECT_GT(smi[3],inserted);
}

/**
 * test priority queue
 */
TEST(MachineInstructionSchedule, test_priority_queue) {
	MachineInstructionSchedule MIS;

	// store first id
	MachineBasicBlock *MBB = new MachineBasicBlock();
	unsigned first = MBB->get_id();
	MIS.push_back(MBB);
	MIS.push_back(new MachineBasicBlock());
	MIS.push_back(new MachineBasicBlock());
	MIS.push_back(new MachineBasicBlock());

	EXPECT_EQ(MIS.size(), 4);

	std::priority_queue<MBBIterator,
		std::vector<MBBIterator>,
		std::greater<MBBIterator> > q;

	for (MBBIterator i = MIS.begin(), e = MIS.end();
			i != e; ++i) {
		q.push(i);
	}

	EXPECT_EQ(q.top()->get_id(), first + 0); q.pop();
	EXPECT_EQ(q.top()->get_id(), first + 1); q.pop();
	EXPECT_EQ(q.top()->get_id(), first + 2); q.pop();
	EXPECT_EQ(q.top()->get_id(), first + 3); q.pop();

	EXPECT_TRUE(q.empty());
}

/**
 * test priority queue
 */
TEST(MachineInstructionSchedule, test_priority_queue1) {
	MachineInstructionSchedule MIS;

	// store first id
	MachineBasicBlock *MBB = new MachineBasicBlock();
	unsigned first = MBB->get_id();
	MIS.push_back(MBB);
	MIS.push_back(new MachineBasicBlock());
	MIS.push_back(new MachineBasicBlock());
	MIS.push_back(new MachineBasicBlock());

	EXPECT_EQ(MIS.size(), 4);

	std::priority_queue<MBBIterator,
		std::vector<MBBIterator>,
		std::greater<MBBIterator> > q;

	for (MBBIterator i = MIS.begin(), e = MIS.end();
			i != e; ++i) {
		q.push(i);
	}

	EXPECT_EQ(q.top()->get_id(), first + 0); q.pop();
	EXPECT_EQ(q.top()->get_id(), first + 1); q.pop();
	EXPECT_EQ(q.top()->get_id(), first + 2); q.pop();
	// insert after the second element and add to queue
	{
		MBBIterator i = ++MIS.begin();
		MIS.insert_after(i,new MachineBasicBlock());
		++i;
		EXPECT_EQ(i->get_id(), first + 4);
		q.push(i);
	}
	EXPECT_EQ(q.top()->get_id(), first + 4); q.pop();
	EXPECT_EQ(q.top()->get_id(), first + 3); q.pop();

	EXPECT_TRUE(q.empty());
}


#endif
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
