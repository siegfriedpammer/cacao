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
private:
	bool done;
	/**
	 * This function performs the patching.
	 */
	virtual bool patch_impl() = 0;
public:
	Patcher();
	virtual ~Patcher();

	/**
	 * This a wrapper to set the done flag.
	 */
	bool patch() {
		if (patch_impl()) {
			done = true;
			return true;
		}
		return false;
	}
	/**
	 * get the absolute position in code segment
	 *
	 * @deprecated
	 */
	virtual uintptr_t get_mpc() const = 0;
	/**
	 * Generates the code for the patcher traps.
	 */
	virtual void emit() = 0;
	/**
	 * reposition to another base
	 */
	virtual void reposition(intptr_t base) = 0;
	/**
	 * Check already patched
	 *
	 * In contrast to check_is_patched this method simply
	 * queries a boolean variable whereas check_is_patched
	 * inspects to machine code.
	 *
	 * @see check_is_patched
	 */
	bool is_patched() const {
		return done;
	}
	/**
	 * Check if the patcher is already patched.
	 *
	 * This is done by comparing the machine instruction.
	 *
	 * @return true if patched, false otherwise.
	 * @see is_patched
	 */
	virtual bool check_is_patched() const = 0;
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
	jitdata *jd;
	patchref_t pr;

	/**
	 * Call the legacy patching function
	 */
	virtual bool patch_impl() {
		// define the patcher function
		bool (*patcher_function)(patchref_t *);
		// cast the passed function to a patcher function
		patcher_function = (bool (*)(patchref_t *)) (ptrint) pr.patcher;

		bool result = (patcher_function)(&pr);

		if (result) {
			// XXX this might be omitted
			pr.done = true;
			return true;
		}
		return false;
	}

public:

	LegacyPatcher(jitdata *jd, const patchref_t &pr)
		: Patcher(), jd(jd), pr(pr) {}

	/**
	 * return the raw resource
	 */
	patchref_t* get() {
		return &pr;
	}

	virtual uintptr_t get_mpc() const {
		return pr.mpc;
	}
	virtual void emit();
	virtual void reposition(intptr_t base) {
		pr.mpc   += base;
		pr.datap  = (intptr_t) (pr.disp + base);
	}
	/**
	 * Check if the patcher is already patched.
	 *
	 * This is done by comparing the machine instruction.
	 *
	 * @return true if patched, false otherwise.
	 */
	virtual bool check_is_patched() const;

	virtual const char* get_name() const;
	virtual OStream& print(OStream &OS) const;
};

/**
 * Base class for all (non-legacy) patcher
 */
class PatcherBase : public Patcher {
private:
	uint32_t  mcode;
	uintptr_t mpc;
public:
	PatcherBase(uintptr_t mpc) : Patcher(), mcode(0), mpc(mpc) {}

	virtual void set_mcode(uint32_t mcode) {
		this->mcode = mcode;
	}
	#if 0
	virtual uintptr_t get_mcode() const {
		return mcode;
	}
	#endif
	virtual uintptr_t get_mpc() const {
		return mpc;
	}
	virtual void reposition(intptr_t base) {
		this->mpc   += base;
	}
	virtual const char* get_name() const {
		return "PatcherBase";
	}
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
