/* src/vm/jit/compiler2/PassUsage.hpp - PassUsage

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

#ifndef _JIT_COMPILER2_PASSUSAGE
#define _JIT_COMPILER2_PASSUSAGE

#include "vm/jit/compiler2/alloc/unordered_map.hpp"
#include "vm/jit/compiler2/alloc/unordered_set.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

// forward declaration
class Pass;

class PassInfo {
public:
	using IDTy = uint32_t;
	using ConstructorTy = Pass* (*)();
private:
	const char *const name;
	/// Constructor function pointer
	ConstructorTy ctor;
public:
	PassInfo::IDTy const ID;
	PassInfo(const char* name, PassInfo::IDTy ID,  ConstructorTy ctor) : name(name),  ctor(ctor), ID(ID) {}
	const char* get_name() const {
		return name;
	}
	Pass* create_Pass() const {
		return ctor();
	}
};

class Artifact;
class ArtifactInfo {
public:
	using IDTy = uint32_t;
	using ConstructorTy = Artifact* (*)();
private:
	const char *const name;
	ConstructorTy ctor;
public:
	ArtifactInfo::IDTy const AID;
	ArtifactInfo(const char* name, ArtifactInfo::IDTy AID, ConstructorTy ctor) : name(name), ctor(ctor), AID(AID) {}
	const char* get_name() const {
		return name;
	}
	Artifact* create_Artifact() const {
		return ctor();
	}
};


/**
 * Stores the interdependencies of a pass and artifacts.
 *
 * requires<ArtifactName> and provides<ArtifactName> is the basic method on how
 * passes are scheduled. If the scheduler ran successfully, it is guaranteed that
 * all required artifacts are ready and up-to-date when the pass is run.
 * 
 * modifies<ArtifactName> can be used to signal that this pass modifies an artifact.
 * Any artifact depending on a modified artifact, will be rebuilt if it is needed
 * again later on.
 * 
 * For more fine-tuning and scheduling optimization passes (which usually do not
 * provide any artifacts), before<PassName> and after<PassName> can be used. These
 * guarantee that this pass is run either before or after the given pass.
 * 
 * A special case are debug/printer passes. A pass is considered a debug/printer pass
 * if it neither provides nor modifies artifacts; and if it does not use before/after
 * for placement. Instead there 2 methods "immediately_before" and "immediatley_after"
 * that can be used at multiple locations to add these kinds of passes all over the schedule.
 */
class PassUsage {
public:
	typedef alloc::unordered_set<PassInfo::IDTy>::type PIIDSet;
	typedef PIIDSet::const_iterator const_iterator;
	typedef alloc::unordered_set<ArtifactInfo::IDTy>::type AIIDSet;
private:
	AIIDSet provides_;
	AIIDSet requires_;
	AIIDSet modifies_;

	PIIDSet after_;
	PIIDSet before_;

	PIIDSet imm_after_;
	PIIDSet imm_before_;

	bool is_required(const ArtifactInfo::IDTy &ID) const {
		return requires_.find(ID) != requires_.end();
	}

public:
	PassUsage() {}

	/**
	 * This pass will provide the specified artifact.
	 * Make sure to also override the "provide_Artifact" method in your pass
	 * so the pass infrastructure can wire up everythign correctly.
	 */
	template<typename ArtifactName>
	void provides() {
		provides_.insert(ArtifactName::template AID<ArtifactName>());
	}

	/**
	 * This pass requires artifact.
	 * Every artifact specified in this manner can be retrieved using
	 * "get_Artifact<>" during a pass run.
	 */
	template<typename ArtifactName>
	void requires() {
		requires_.insert(ArtifactName::template AID<ArtifactName>());
	}

	/**
	 * Signals that this pass modifies the specified artifact.
	 * This might result in passes being re-scheduled.
	 */
	template<typename ArtifactName>
	void modifies() {
		modifies_.insert(ArtifactName::template AID<ArtifactName>());
	}

	/**
	 * This pass needs to run after the specified pass.
	 * E.g.: Can be used to order optimizations.
	 */
	template<typename PassName>
	void after() {
		after_.insert(PassName::template ID<PassName>());
	}

	/**
	 * This pass needs to run before the specified pass.
	 */
	template<typename PassName>
	void before() {
		before_.insert(PassName::template ID<PassName>());
	}

	/**
	 * If this pass is considered a debug/printer pass (see above for precise conditions),
	 * this can be used to add a run of this pass after the specified pass.
	 */
	template<typename PassName>
	void immediately_after() {
		imm_after_.insert(PassName::template ID<PassName>());
	}

	/**
	 * If this pass is considered a debug/printer pass (see above for precise conditions),
	 * this can be used to add a run of this pass before the specified pass.
	 */
	template<typename PassName>
	void immediately_before() {
		imm_before_.insert(PassName::template ID<PassName>());
	}

	const_iterator provides_begin() const { return provides_.begin(); }
	const_iterator provides_end() const { return provides_.end(); }

	const_iterator requires_begin() const { return requires_.begin(); }
	const_iterator requires_end() const { return requires_.end(); }

	const_iterator modifies_begin() const { return modifies_.begin(); }
	const_iterator modifies_end() const { return modifies_.end(); }

	const_iterator after_begin() const { return after_.begin(); }
	const_iterator after_end() const { return after_.end(); }

	const_iterator before_begin() const { return before_.begin(); }
	const_iterator before_end() const { return before_.end(); }

	const_iterator imm_after_begin() const { return imm_after_.begin(); }
	const_iterator imm_after_end() const { return imm_after_.end(); }

	const_iterator imm_before_begin() const { return imm_before_.begin(); }
	const_iterator imm_before_end() const { return imm_before_.end(); }

	friend class Pass;
};


} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_PASSUSAGE */


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
