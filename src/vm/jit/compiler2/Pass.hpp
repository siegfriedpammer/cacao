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

namespace cacao {
namespace jit {
namespace compiler2 {

// forward declaration
class PassManager;
class Method;


/**
 * Pass superclass
 * All compiler passes should inheritate this class.
 * TODO: more info
 */
class Pass {
private:
	PassManager *pm;
public:
	Pass(PassManager *PM) : pm(PM) {}

	/**
	 * get the result of a previous compiler pass
	 */
	template<typename ResultType>
	ResultType *getResult() const;

	/**
	 * Initialize teh Pass.
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
	 */
	virtual void run(Method &M) = 0;

	/**
	 * Name of the Pass.
	 */
	static const char* name();

	/**
	 * Destructor
	 */
	virtual ~Pass() {}
};


} // end namespace cacao
} // end namespace jit
} // end namespace compiler2

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
