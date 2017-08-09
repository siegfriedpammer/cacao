/* src/vm/jit/compiler2/Pass.hpp - Pass

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

#ifndef _JIT_COMPILER2_PASS
#define _JIT_COMPILER2_PASS

#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/memory/Manager.hpp"
#include <cstddef>

MM_MAKE_NAME(Pass)

namespace cacao {
namespace jit {
namespace compiler2 {

// forward declaration
class PassUsage;
class JITData;

/**
 * Pass superclass
 * All compiler passes should inheritate this class.
 * TODO: more info
 */
class Pass { 
private:
	static PassInfo::IDTy id_counter;

	PassRunner *pm;
	bool allowed_to_use_result(const PassInfo::IDTy &id) const;
public:
	Pass() : pm(NULL) {}

	void set_PassRunner(PassRunner *pr) {
		pm = pr;
	}

	/** 
	 * This template will return a unique ID for each type that it is called with.
	 * 
	 * This is needed since Pass IDs are accessed statically via a concrete Pass type
	 * and not via a concrete instance.
	 */ 
	template<class T>
	static PassInfo::IDTy ID() {
		static PassInfo::IDTy ID = id_counter++;
		return ID;
	}

	/**
	 * Get the result of a previous compiler pass
	 *
	 * Can only be used if ResultType is added to required in get_PassUsage().
	 */
	template<class _PassClass>
	_PassClass *get_Pass() const {
		if (!allowed_to_use_result(_PassClass::template ID<_PassClass>())) {
			assert(0 && "Not allowed to get result (not declared in get_PassUsage())");
			return NULL;
		}
		return pm->get_Pass_result<_PassClass>();
	}

	/**
	 * Get the result of a previous compiler pass
	 *
	 * Can only be used if ResultType is added to required in get_PassUsage().
	 */
	template<class _PassClass>
	_PassClass *get_Pass_if_available() const {
		if (!pm->result_ready[_PassClass::template ID<_PassClass>()])
			return NULL;
		return pm->get_Pass_result<_PassClass>();
	}
	/**
	 * Set the requirements for the pass
	 */
	virtual PassUsage& get_PassUsage(PassUsage &PU) const {
		// default: require nothing, destroy nothing
		return PU;
	}

	/**
	 * Allows concrete passes to enable/disable themselves the way they like
	 */
	virtual bool is_enabled() const {
		return true;
	}

	/**
	 * Passes need to set this to true if they are not required by any other passes,
	 * and if they do not specify a scheduling dependency themselves using add_run_before
	 * or add_schedule_before.
	 *
	 * This is usually the case for debug/printer passes.
	 *
	 * The enable/disable flag supercedes this flag, it is only used for enabled passes.
	 */
	virtual bool force_scheduling() const {
		return false;
	}

	/**
	 * Initialize the Pass.
	 * This method is called by the PassManager before the pass is started. It should be used to
	 * initialize e.g. data structures. A Pass object might be reused so the construtor can not be
	 * used in some cases.
	 */
	virtual void initialize() {}

	/**
	 * Finalize the Pass.
	 * This method is called by the PassManager after the pass is no longer used. It should be used to
	 * clean up stuff done in initialize().
	 */
	virtual void finalize() {}

	/**
	 * Run the Pass.
	 * This method implements the compiler pass.
	 *
	 * @return false if a problem occurred, true otherwise
	 */
	virtual bool run(JITData &JD) = 0;

	/**
	 * Verify the Result.
	 * This method is used to verify the result of the pass. It has the same motivation
	 * than the assert() statement. It should be only used for debugging purposes
	 * and might not be called in release builds.
	 */
	virtual bool verify() const { return true; }

	/**
	 * Destructor
	 */
	virtual ~Pass() {}
};


} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_PASS */


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
