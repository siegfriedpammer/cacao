/* tests/compiler2/catch/framework/PassRunner.hpp - PassRunner
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

#ifndef _TESTS_COMPILER2_CATCH_FRAMEWORK_PASSRUNNER
#define _TESTS_COMPILER2_CATCH_FRAMEWORK_PASSRUNNER

#include <string>

#include "config.h"
#include <assert.h>

#include "mm/dumpmemory.hpp"

#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"

#include "CacaoVM.hpp"

using cacao::jit::compiler2::JITData;
using cacao::jit::compiler2::PassInfo;
using cacao::jit::compiler2::PassRunner;

/**
 * For these tests we do not use a PassScheduler but instead keep
 * a fixed PassSchedule to keep tests more stable.
 * A default schedule (without optimization passes) is stored
 * as a static field.
 */
class TestPassSchedule {
public:
	static TestPassSchedule& get_default();

	template <typename PassName>
	TestPassSchedule& add()
	{
		schedule.push_back(PassName::template ID<PassName>());
		return *this;
	}

	std::vector<PassInfo::IDTy>::iterator begin() { return schedule.begin(); }
	std::vector<PassInfo::IDTy>::iterator end() { return schedule.end(); }
	std::vector<PassInfo::IDTy>::reference back() { return schedule.back(); }

private:
	std::vector<PassInfo::IDTy> schedule;

	static TestPassSchedule default_schedule;
};

// This is just a helper class, do not use it directly
class CustomPassRunner : public PassRunner {
public:
	CustomPassRunner(const TestPassSchedule& s) : schedule(s) { until = schedule.back(); }
	CustomPassRunner() : CustomPassRunner(TestPassSchedule::get_default()) {}

	void runPasses(JITData& JD) override;
	void set_until(const PassInfo::IDTy& id) { until = id; }

	template <class Pass>
	Pass* get_pass()
	{
		return get_Pass_result<Pass>();
	}

private:
	TestPassSchedule schedule;
	PassInfo::IDTy until;
};

/**
 * Main interface to use.
 * It will load a given Java method and run the compiler2 pass pipeline
 * on that method. Afterwards the individual passes can be queried and checked.
 *
 * If no custom TestPassSchedule is provided, the default one is used.
 */ 
class TestPassRunner {
public:
	TestPassRunner(const TestPassSchedule& s) : schedule(s) { CacaoVM::initialize(); }
	TestPassRunner() : TestPassRunner(TestPassSchedule::get_default()) {}

	/**
	 * Runs the pass schedule and stops AFTER the given PassName is executed
	 */
	template <typename PassName>
	void until(const std::string& clazz, const std::string& method, const std::string& desc)
	{
		auto id = PassName::template ID<PassName>();
		runner.reset(new CustomPassRunner(schedule));
		runner->set_until(id);
		run_until(clazz, method, desc);
	}

	template <class Pass>
	Pass* get_Pass()
	{
		return runner->get_pass<Pass>();
	}

private:
	TestPassSchedule schedule;
	std::unique_ptr<CustomPassRunner> runner;

	methodinfo*
	load_method(const std::string& clazz, const std::string& method, const std::string& desc) const;
	void run_until(const std::string& clazz, const std::string& method, const std::string& desc);
};

#endif /* _TESTS_COMPILER2_CATCH_FRAMEWORK_PASSRUNNER */

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
