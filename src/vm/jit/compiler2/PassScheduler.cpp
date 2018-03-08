/* src/vm/jit/compiler2/PassScheduler.cpp

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

#include "vm/jit/compiler2/PassScheduler.hpp"

#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"

#include <algorithm>
#include <map>
#include <queue>

#define DEBUG_NAME "compiler2/PassScheduler"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace {

static const char* name(PassInfo::IDTy id) {
	return PassManager::get().get_Pass_name(id);
}

static const char* aname(ArtifactInfo::IDTy id) {
	return PassManager::get().get_Artifact_name(id);
}

class PassScheduler {
public:
	explicit PassScheduler(const PassIDSetTy& passes, PassUsageMapTy& passusage_map)
		: unhandled_passes(passes), passusage_map(passusage_map), available_passes(PQCompare(passusage_map)) {}

	void schedule_passes();
	std::vector<PassInfo::IDTy> get_schedule() const { return schedule; }

private:
	PassIDSetTy unhandled_passes;
	PassUsageMapTy& passusage_map;

	std::vector<PassInfo::IDTy> schedule;

	// Used for ordering the priority queue that holds available passes
	// for scheduling. We prefer passes that do not modify/destroy artifacts
	// since we might safe some re-runs.
	struct PQCompare {
		PassUsageMapTy& passusage_map;
		PQCompare(PassUsageMapTy& passusage_map) : passusage_map(passusage_map) {}

		bool operator()(PassInfo::IDTy lhs, PassInfo::IDTy rhs) const {
			bool lhs_modifies_artifacts = passusage_map[lhs].modifies_begin() != passusage_map[lhs].modifies_end();
			bool rhs_modifies_artifacts = passusage_map[rhs].modifies_begin() != passusage_map[rhs].modifies_end();
			
			if (lhs_modifies_artifacts) return true;
			if (rhs_modifies_artifacts && !lhs_modifies_artifacts) return false;
			return true;
		}
	};
	std::priority_queue<PassInfo::IDTy, std::vector<PassInfo::IDTy>, PQCompare> available_passes;

	// Unifies before/after scheduling dependencies in a single data structure
	std::map<PassInfo::IDTy, std::vector<PassInfo::IDTy>> pass_edges;

	std::unordered_set<ArtifactInfo::IDTy> ready;
	std::map<ArtifactInfo::IDTy, std::unordered_set<PassInfo::IDTy>> reverse_require_pass;
	std::map<ArtifactInfo::IDTy, std::unordered_set<ArtifactInfo::IDTy>> reverse_require_artifact;
	std::map<ArtifactInfo::IDTy, PassInfo::IDTy> provided_by_map;

	// For each pass we also have a set of passes that needs to run immediately before/after
	std::map<PassInfo::IDTy, std::unordered_set<PassInfo::IDTy>> imm_before_map;
	std::map<PassInfo::IDTy, std::unordered_set<PassInfo::IDTy>> imm_after_map;
	
	bool artifacts_ready(PassInfo::IDTy id);
	bool is_debug_pass(PassInfo::IDTy id);

	void prepare_relative_edges();
	void prepare_reverse_require();
	void prepare_provided_by_map();
	void prepare_debug_passes();
	
	void find_available_passes();
	void schedule_pass(PassInfo::IDTy id);
};

void PassScheduler::schedule_passes() {
	LOG(BoldYellow << "\nRunning PassScheduler\n\n" << reset_color);

	prepare_relative_edges();
	prepare_reverse_require();
	prepare_provided_by_map();
	prepare_debug_passes();

	find_available_passes();

	while (!available_passes.empty()) {
		auto pass_id = available_passes.top();
		available_passes.pop();
		
		LOG1("Scheduling " << BoldWhite << name(pass_id) << reset_color << nl);
		schedule_pass(pass_id);
		find_available_passes();
	}

	if (!unhandled_passes.empty()) {
		ABORT_MSG("Not all passes scheduled.", "Check dependency graph for cycles/missing dependencies!");
	}
}

void PassScheduler::prepare_relative_edges() {
	for (const auto& id_to_usage : passusage_map) {
		auto id = id_to_usage.first;
		auto& pa = id_to_usage.second;

		std::for_each(pa.after_begin(), pa.after_end(), [&](auto to) {
			pass_edges[id].push_back(to);
		});
		std::for_each(pa.before_begin(), pa.before_end(), [&](auto from) {
			pass_edges[from].push_back(id);
		});
	}
}

void PassScheduler::prepare_reverse_require() {
	for (const auto& id_to_usage : passusage_map) {
		auto id = id_to_usage.first;
		auto& pa = id_to_usage.second;

		// Establishes two relations for Pass P that requires Artifact A
		//   Required artifact A -> pass P
		//   Required artifact A -> all artifacts produced by pass P
		std::for_each(pa.requires_begin(), pa.requires_end(), [&](auto required) {
			reverse_require_pass[required].insert(id);
			std::for_each(pa.provides_begin(), pa.provides_end(), [&](auto provided) {
				reverse_require_artifact[required].insert(provided);
			});
		});
	}
}

void PassScheduler::prepare_provided_by_map() {
	for (const auto& id_to_usage : passusage_map) {
		auto id = id_to_usage.first;
		auto& pa = id_to_usage.second;
	
		std::for_each(pa.provides_begin(), pa.provides_end(), [&](auto provided) {
			provided_by_map[provided] = id;
		});
	}
}

// We consider debug passes all passes that:
//     - Do not provide/modify any artifacts
//     - Have no before/after
//     - Use only (minimum of 1) immediately_before/after
// These ones are removed from the unhandled set, and automatically inserted
// when the passes with the imm_before/after dependencies are scheduled
void PassScheduler::prepare_debug_passes() {
	for (const auto& id_to_usage : passusage_map) {
		auto id = id_to_usage.first;
		auto& pa = id_to_usage.second;

		if (is_debug_pass(id)) {
			LOG1("Pass " << name(id) << " is a debug pass. Considering immediately_before/after dependencies!\n");
			unhandled_passes.erase(id);

			std::for_each(pa.imm_after_begin(), pa.imm_after_end(), [&](auto pass) {
				imm_after_map[pass].insert(id);
			});
			std::for_each(pa.imm_before_begin(), pa.imm_before_end(), [&](auto pass) {
				imm_before_map[pass].insert(id);
			});
		}
	}
}

bool PassScheduler::is_debug_pass(PassInfo::IDTy id) {
	auto& pa = passusage_map[id];

	if (pa.provides_begin() != pa.provides_end()) return false;
	if (pa.modifies_begin() != pa.modifies_end()) return false;
	if (pa.after_begin() != pa.after_end() || pa.before_begin() != pa.before_end()) return false;
	if (pa.imm_after_begin() == pa.imm_after_end() && pa.imm_before_begin() == pa.imm_before_end())
		return false;
	return true;
}

// Adds all passes to the available set that fulfill these requirements:
//     - All required Artifacts are in the "ready" set
//     - All outgoing relative edges are in the schedule
void PassScheduler::find_available_passes() {
	for (auto i = unhandled_passes.begin(), e = unhandled_passes.end(); i != e;) {
		auto id = *i;

		bool relative_dependencies_ok = std::all_of(pass_edges[id].begin(), pass_edges[id].end(), [&](auto before) {
			return std::find(schedule.begin(), schedule.end(), before) != schedule.end();
		});

		if (artifacts_ready(id) && relative_dependencies_ok) {
			available_passes.push(id);
			i = unhandled_passes.erase(i);
			LOG1("Adding " << name(id) << " to available pass set\n");
		} else {
			++i;
		}
	}
}

bool PassScheduler::artifacts_ready(PassInfo::IDTy id) {
	auto& pa = passusage_map[id];
	return std::all_of(pa.requires_begin(), pa.requires_end(), [&](auto required_aid) {
		return ready.count(required_aid) == 1;
	});
}

// When a pass is added to the schedule, all the artifacts it
// provides are added to the ready set
void PassScheduler::schedule_pass(PassInfo::IDTy id) {
	// Schedule all debug passes running before the pass we want to schedule
	for (auto before_pass : imm_before_map[id]) {
		assert(artifacts_ready(before_pass) && "Scheduled debug pass with unready artifacts!");
		schedule.push_back(before_pass);
	}
	
	schedule.push_back(id);
	
	auto& pu = passusage_map[id];
	
	// If the scheduled pass P modifies some artifact A, we
	// need to take some additional steps:
	//     - remove all artifacts B from the ready set that require A
	//     - if there are any passes in the unhandled_set left, that require
	//       artifact B, add the pass that provides B back to the unhandled_set
	std::for_each(pu.modifies_begin(), pu.modifies_end(), [&](auto modified_artifact) {
		LOG3(Cyan << "Pass " << name(id) << " modifies artifact " << aname(modified_artifact) << reset_color << nl);

		for (auto invalid_artifact : reverse_require_artifact[modified_artifact]) {
			bool removed = ready.erase(invalid_artifact) == 1;
			if (!removed) continue;

			LOG3("Removed artifact " << aname(invalid_artifact) << " from ready set!\n");

			// Are there unhandled passes that require the just invalidated artifact?
			// If so add the producing pass back into the unhandled set
			for (auto unhandled_pass : unhandled_passes) {
				if (reverse_require_pass[invalid_artifact].count(unhandled_pass) == 1) {
					unhandled_passes.insert(provided_by_map[invalid_artifact]);
					LOG3("Added Pass " << name(provided_by_map[invalid_artifact])
						 << " back to the unhandled_set since it provides an artifact that will be needed again.\n");
				}
			}
		}
			
	});

	// Add all provided artifacts to the ready set
	ready.insert(pu.provides_begin(), pu.provides_end());

	// Schedule all debug passes running after the pass we want to schedule
	for (auto after_pass : imm_after_map[id]) {
		assert(artifacts_ready(after_pass) && "Scheduled debug pass with unready artifats!");
		schedule.push_back(after_pass);
	}
}

} // end anonymous namespace

std::vector<PassInfo::IDTy> GetPassSchedule(const PassIDSetTy& passes, PassUsageMapTy& passusage_map)
{
	PassScheduler scheduler(passes, passusage_map);
	scheduler.schedule_passes();
	return scheduler.get_schedule();
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
