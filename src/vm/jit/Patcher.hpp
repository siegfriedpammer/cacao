/* src/vm/jit/Patcher.hpp - Patcher class

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

#ifndef _PATCHER_HPP
#define _PATCHER_HPP

#include "vm/jit/patcher-common.hpp"

namespace cacao {

// forward declaration
class OStream;

/**
 * Patcher super class
 *
 * This class is intended to replace the patchref_t patcher references.
 * The goal is to replace function pointers by virtual functions and
 * void pointers by member variables. Although we need a vtbl_ptr for
 * virtual functions and a smart pointer for list storage this might even
 * reduce the overall memory consumption because not all fields of patchref_t
 * are needed by all patchers. But the main focus is on encapsulation and
 * usability.
 *
 * @todo Remove/adopt the text above if the legacy patchref_t framework has
 * been removed.
 */
class Patcher {
public:
	Patcher();
	virtual ~Patcher();

	/**
	 * This function performs the patching.
	 */
	virtual bool patch() = 0;

	/**
	 * set the machine code to be patched back in
	 *
	 * @deprecated
	 */
	virtual void set_mcode(uint32_t mcode) = 0;
	/**
	 * get the machine code to be patched back in
	 *
	 * @deprecated
	 */
	virtual uintptr_t get_mcode() const = 0;
	/**
	 * get the absolute position in code segment
	 *
	 * @deprecated
	 */
	virtual uintptr_t get_mpc() const = 0;
	/**
	 * reposition to another base
	 */
	virtual void reposition(intptr_t base) = 0;
	/**
	 * already patched
	 */
	virtual bool is_patched() const = 0;
	/**
	 * print patcher information
	 */
	virtual const char* get_name() const;
	/**
	 * print patcher information
	 */
	virtual OStream& print(OStream &OS) const;
};

class LegacyPatcher : public Patcher {
private:
	patchref_t pr;
public:

	LegacyPatcher(const patchref_t &pr) : Patcher(), pr(pr) {}

	/**
	 * return the raw resource
	 */
	patchref_t* get() {
		return &pr;
	}

	/**
	 * Call the legacy patching function
	 */
	virtual bool patch() {
		// define the patcher function
		bool (*patcher_function)(patchref_t *);
		// cast the passed function to a patcher function
		patcher_function = (bool (*)(patchref_t *)) (ptrint) pr.patcher;

		bool result = (patcher_function)(&pr);

		if (result) {
			pr.done = true;
			return true;
		}
		return false;
	}
	virtual void set_mcode(uint32_t mcode) {
		pr.mcode = mcode;
	}
	virtual uintptr_t get_mcode() const {
		return pr.mcode;
	}
	virtual uintptr_t get_mpc() const {
		return pr.mpc;
	}
	virtual void reposition(intptr_t base) {
		pr.mpc   += base;
		pr.datap  = (intptr_t) (pr.disp + base);
	}
	virtual bool is_patched() const {
		return pr.done;
	}
	virtual const char* get_name() const;
	virtual OStream& print(OStream &OS) const;
};

} // end namespace cacao

#endif // _PATCHER_HPP


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
