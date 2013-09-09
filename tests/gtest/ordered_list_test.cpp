/* tests/gtest/ordered_list_test.cpp - ordered_list tests

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
#include "toolbox/ordered_list.hpp"

#include <queue>

using namespace cacao;

namespace {

/**
 * check a ordered_list<int> for size and content
 */
inline void check_list(ordered_list<int> &list, unsigned size, unsigned array[]) {
	// check size
	EXPECT_EQ(list.size(), size);

	unsigned counter = 0;
	for (ordered_list<int>::iterator i = list.begin(), e = list.end();
			i != e; ++i) {
		EXPECT_EQ(*i, array[counter++]);
	}
}

} // end anonymous namespace

TEST(ordered_list, test_push_back) {
	ordered_list<int> list;

	list.push_back(0);
	list.push_back(1);
	list.push_back(2);
	list.push_back(3);

	unsigned array[] = {0, 1, 2, 3};
	check_list(list,4,array);
}

TEST(ordered_list, test_push_front) {
	ordered_list<int> list;

	list.push_front(0);
	list.push_front(1);
	list.push_front(2);
	list.push_front(3);

	unsigned array[] = {3, 2 ,1, 0 };
	check_list(list,4,array);
}

TEST(ordered_list, test_insert) {
	ordered_list<int> list;

	list.push_back(0);
	list.push_back(1);
	list.push_back(2);
	list.push_back(3);

	EXPECT_EQ(list.size(), 4);

	// insert before the third element
	{
		ordered_list<int>::iterator i = list.begin();
		std::advance(i,2);
		list.insert(i,4);
	}

	unsigned array[] = {0, 1, 4, 2, 3};
	check_list(list,5,array);
}

/**
 * test if the ordered_list<int>::iterator does not iterater
 * to a newly inserted element that was inserted before the
 * current one
 */
TEST(ordered_list, test_insert2) {
	ordered_list<int> list;

	list.push_back(0);
	list.push_back(1);
	list.push_back(2);
	list.push_back(3);

	EXPECT_EQ(list.size(), 4);

	{
		ordered_list<int>::iterator i = list.begin();
		EXPECT_EQ(*i++,0);
		EXPECT_EQ(*i++,1);
		EXPECT_EQ((*i  ),2);
		list.insert(i,4);
		EXPECT_EQ(*i++,2);
		EXPECT_EQ(*i++,3);

		EXPECT_EQ(i,list.end());
	}

	unsigned array[] = {0, 1, 4, 2, 3};
	check_list(list,5,array);
}

/**
 * test the total ordering of MachineInstruction
 */
TEST(ordered_list, test_order) {
	ordered_list<int> list;

	list.push_back(0);
	list.push_back(1);
	list.push_back(2);
	list.push_back(3);

	EXPECT_EQ(list.size(), 4);

	// store ordered_list<int>::iterators
	ordered_list<int>::iterator smi[4];

	unsigned counter = 0;
	for (ordered_list<int>::iterator i = list.begin(), e = list.end();
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
TEST(ordered_list, test_order2) {
	ordered_list<int> list;

	list.push_back(0);
	list.push_back(1);
	list.push_back(2);
	list.push_back(3);

	EXPECT_EQ(list.size(), 4);

	// store ordered_list<int>::iterators
	ordered_list<int>::iterator smi[4];

	unsigned counter = 0;
	for (ordered_list<int>::iterator i = list.begin(), e = list.end();
			i != e; ++i) {
		smi[counter++] = i;
	}

	EXPECT_LT(smi[0],smi[1]);
	EXPECT_LT(smi[1],smi[2]);
	EXPECT_LT(smi[2],smi[3]);

	// insert after the third element
	ordered_list<int>::iterator inserted = list.begin();
	std::advance(inserted,3);
	list.insert(inserted,4);
	--inserted;

	EXPECT_EQ((*inserted), 4);

	unsigned array[] = {0, 1, 2, 4, 3};
	check_list(list,5,array);

	EXPECT_LT(smi[0],inserted);
	EXPECT_LT(smi[1],inserted);
	EXPECT_LT(smi[2],inserted);
	EXPECT_GT(smi[3],inserted);
}

/**
 * test priority queue
 */
TEST(ordered_list, test_priority_queue) {
	ordered_list<int> list;

	list.push_back(0);
	list.push_back(1);
	list.push_back(2);
	list.push_back(3);

	EXPECT_EQ(list.size(), 4);

	std::priority_queue<ordered_list<int>::iterator,
		std::vector<ordered_list<int>::iterator>,
		std::greater<ordered_list<int>::iterator> > q;

	for (ordered_list<int>::iterator i = list.begin(), e = list.end();
			i != e; ++i) {
		q.push(i);
	}

	EXPECT_EQ(*q.top(), 0); q.pop();
	EXPECT_EQ(*q.top(), 1); q.pop();
	EXPECT_EQ(*q.top(), 2); q.pop();
	EXPECT_EQ(*q.top(), 3); q.pop();

	EXPECT_TRUE(q.empty());
}

/**
 * test priority queue
 */
TEST(ordered_list, test_priority_queue1) {
	ordered_list<int> list;

	list.push_back(0);
	list.push_back(1);
	list.push_back(2);
	list.push_back(3);

	EXPECT_EQ(list.size(), 4);

	std::priority_queue<ordered_list<int>::iterator,
		std::vector<ordered_list<int>::iterator>,
		std::greater<ordered_list<int>::iterator> > q;

	for (ordered_list<int>::iterator i = list.begin(), e = list.end();
			i != e; ++i) {
		q.push(i);
	}

	EXPECT_EQ(*q.top(), 0); q.pop();
	EXPECT_EQ(*q.top(), 1); q.pop();
	EXPECT_EQ(*q.top(), 2); q.pop();
	// insert after the second element and add to queue
	{
		ordered_list<int>::iterator i = list.begin();
		std::advance(i,3);
		list.insert(i,4);
		--i;
		EXPECT_EQ(*i, 4);
		q.push(i);
	}
	EXPECT_EQ(*q.top(), 4); q.pop();
	EXPECT_EQ(*q.top(), 3); q.pop();

	EXPECT_TRUE(q.empty());
}

TEST(ordered_list, test_copy_ctor) {
	ordered_list<int> list;

	list.push_back(0);
	list.push_back(1);
	list.push_back(2);
	list.push_back(3);

	ordered_list<int> list2(list);

	EXPECT_TRUE(list == list2);
	list.clear();
	EXPECT_TRUE(list.empty());
	EXPECT_TRUE(list != list2);

	unsigned array[] = {0, 1, 2, 3};
	check_list(list2,4,array);
}

TEST(ordered_list, test_copy_assigment) {
	ordered_list<int> list;

	list.push_back(0);
	list.push_back(1);
	list.push_back(2);
	list.push_back(3);

	ordered_list<int> list2;
	list2 = list;

	EXPECT_TRUE(list == list2);
	list.clear();
	EXPECT_TRUE(list.empty());
	EXPECT_TRUE(list != list2);

	unsigned array[] = {0, 1, 2, 3};
	check_list(list2,4,array);
}

TEST(ordered_list, test_swap) {
	ordered_list<int> list1;

	list1.push_back(0);
	list1.push_back(1);
	list1.push_back(2);
	list1.push_back(3);

	ordered_list<int> list2;

	list2.push_front(0);
	list2.push_front(1);
	list2.push_front(2);
	list2.push_front(3);

	unsigned array1[] = {0, 1, 2, 3};
	unsigned array2[] = {3, 2, 1, 0};

	check_list(list1,4,array1);
	check_list(list2,4,array2);

	swap(list1,list2);

	check_list(list1,4,array2);
	check_list(list2,4,array1);
}

TEST(ordered_list, test_const_iterator1) {
	ordered_list<int> list;

	list.push_back(0);
	list.push_back(1);
	list.push_back(2);
	list.push_back(3);

	EXPECT_EQ(list.size(),4);

	const ordered_list<int> &clist = list;

	int counter = 0;
	for (ordered_list<int>::const_iterator i = clist.begin(),
			e = clist.end(); i != e; ++i) {
		EXPECT_EQ(*i, counter++);
	}
}

TEST(ordered_list, test_const_iterator2) {
	ordered_list<int> list;

	list.push_back(0);
	list.push_back(1);
	list.push_back(2);
	list.push_back(3);

	EXPECT_EQ(list.size(),4);

	int counter = 0;
	for (ordered_list<int>::const_iterator i = list.begin(),
			e = list.end(); i != e; ++i) {
		EXPECT_EQ(*i, counter++);
	}
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
