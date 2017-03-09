/* src/vm/jit/compiler2/UnionFind.hpp - UnionFind

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

#ifndef _TOOLBOX_UNIONFIND_HPP
#define _TOOLBOX_UNIONFIND_HPP

#include <list>
#include <set>

namespace cacao {


template <typename T>
class UnionFind {
public:
	/**
	 * Create a new set
	 *
	 * @param s elements
	 **/
	virtual void make_set(T s) = 0;
	/**
	 * union the set that contains r with the set that contains s
	 *
	 * @param r repres of a set
	 * @param s repres of a set
	 * @return the new repres
	 *
	 **/
	virtual T set_union(T r, T s) = 0;
	/**
	 * find the repres to of a given element x
	 *
	 * @param x an element of the union find ds
	 * @return the repres of x
	 **/
	virtual T find(T x) = 0;

};

template <typename T>
class UnionFindImpl : public UnionFind<T> {
private:
	typedef typename std::set<T> SetTy;
	typedef typename std::set<SetTy> SetSetTy;
	SetSetTy sets;
	T &my_end;

	typename SetSetTy::iterator find_it(T x) {
		for (typename SetSetTy::iterator i = sets.begin(), e = sets.end();
				i != e; ++i) {
			SetTy set = *i;
			if (std::find(set.begin(),set.end(),x) != set.end()) {
				return i;
			}
		}
		return sets.end();
	}
public:
	UnionFindImpl(T &end) : my_end(end) {}

	virtual void make_set(T s) {
		assert(find(s) == end() && "Element already in a set!");
		//LOG2("UnionFindImpl:make_set: " << s << nl);
		SetTy list;
		list.insert(s);
		sets.insert(list);
	}

	virtual T set_union(T r, T s) {
		typename SetSetTy::iterator r_it, s_it;
		r_it = find_it(r);
		s_it = find_it(s);

		if ( r_it == s_it )
			return *(r_it->begin());
		if ( r_it == sets.end() || s_it == sets.end() )
			return end();
		SetTy merged;
		merged.insert(s_it->begin(), s_it->end());
		merged.insert(r_it->begin(), r_it->end());

		//LOG2("UnionFindImpl:merginig: ");
		//DEBUG2(print_container(dbg(), s_it->begin(), s_it->end()) << " and ");
		//DEBUG2(print_container(dbg(), r_it->begin(), r_it->end()) << nl);
		//LOG2("UnionFindImpl:result: ");
		//DEBUG2(print_container(dbg(), merged.begin(), merged.end()) << nl);

		sets.erase(s_it);
		sets.erase(r_it);
		sets.insert(merged);
		return *merged.begin();
	}

	virtual T find(T x) {
		typename SetSetTy::iterator i = find_it(x);
		//LOG2("UnionFindImpl:find: " << x << nl);
		if (i != sets.end())
			return *(*i).begin();
		return end();
	}

	T &end() {
		return my_end;
	}

};

} // end namespace cacao

#endif /* _TOOLBOX_UNIONFIND_HPP */

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

