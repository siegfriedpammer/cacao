/* src/vm/jit/compiler2/lsra/RegisterAssignmentPass.hpp - RegisterAssignmentPass

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

#ifndef _JIT_COMPILER2_LSRA_REGISTERASSIGNMENT
#define _JIT_COMPILER2_LSRA_REGISTERASSIGNMENT

#include <list>
#include <map>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/lsra/NewLivetimeAnalysisPass.hpp"

MM_MAKE_NAME(RegisterAssignmentPass)

namespace cacao {
namespace jit {
namespace compiler2 {

class Backend;
class MachineBasicBlock;
class MachineOperand;

/**
 * Contains a bitset where bitset[i] = true iff operand
 * i has a color assigned.
 * Contains a list where list[i] = assigned_color for operand i
 * that has bitset[i] = true.
 */
class AllocatedVariables {
public:
	AllocatedVariables(unsigned variables_size, unsigned col_size)
	    : variables(variables_size, 0ul), colors(variables_size, 0), colors_size(col_size)
	{
	}

	// Remove all variables from the allocated set that are not live
	void intersect(const LiveTy& live_in) { variables &= live_in; }

	boost::dynamic_bitset<> get_colors() const
	{
		boost::dynamic_bitset<> result(colors_size, 0ul);
		for (std::size_t i = 0; i < variables.size(); ++i) {
			if (!variables[i])
				continue;

			result[colors[i]] = true;
		}
		return result;
	}

	unsigned get_color(MachineOperand* operand) const;

	void remove_variable(unsigned operand) { variables[operand] = false; }

	void set_color(unsigned operand, unsigned color)
	{
		variables[operand] = true;
		colors[operand] = color;
	}

private:
	boost::dynamic_bitset<> variables;
	std::vector<unsigned> colors;

	unsigned colors_size;
};

class AssignableColors {
public:
	AssignableColors(unsigned size) : colors(size, 0ul), operands(size, nullptr) {}
	void set_operand(unsigned idx, MachineOperand* operand) { operands[idx] = operand; }

	MachineOperand* get(unsigned id) { return operands[id]; }

private:
	boost::dynamic_bitset<> colors;
	std::vector<MachineOperand*> operands;
};

class RegisterAssignmentPass : public Pass, public memory::ManagerMixin<RegisterAssignmentPass> {
public:
	RegisterAssignmentPass() : Pass() {}
	virtual bool run(JITData& JD);
	virtual PassUsage& get_PassUsage(PassUsage& PU) const;

private:
	Backend* backend;
	std::unique_ptr<AssignableColors> assignable_colors;
	unsigned variables_size;
	unsigned colors_size;
	std::map<MachineBasicBlock*, AllocatedVariables> allocated_variables_map;

	// Creates a new AlloctedVariables& instance and puts it in the map.
	// If an immediate dominator is specified, the data is copied from there
	AllocatedVariables& create_for(MachineBasicBlock* block, MachineBasicBlock* idom)
	{
		if (idom) {
			auto& idom_vars = allocated_variables_map.at(idom); // throw exception if its not there
			allocated_variables_map.emplace(std::make_pair(block, AllocatedVariables(idom_vars)));
		}
		else {
			allocated_variables_map.emplace(
			    std::make_pair(block, AllocatedVariables(variables_size, colors_size)));
		}
		return allocated_variables_map.at(block);
	}

	// TODO: This got copied from SpillPass, move it to a utility class!!!
	using BlockOrder = std::list<MachineBasicBlock*>;
	BlockOrder reverse_postorder;

	void build_reverse_postorder(MachineBasicBlock* block);

	void create_borders(MachineBasicBlock* block);
	void assign(MachineBasicBlock* assign);

	struct LivetimeInterval {
		MachineOperand* operand;
		unsigned step;
		bool is_def;
		bool is_real;
	};
	using IntervalList = std::list<LivetimeInterval>;
	std::map<MachineBasicBlock*, IntervalList> interval_map;

	IntervalList* current_list;

	void update_current_list(MachineBasicBlock* block)
	{
		auto pair = interval_map.emplace(block, IntervalList());
		current_list = &pair.first->second;
	}

	void add_interval(MachineOperand* operand, unsigned step, bool def, bool real);

	void add_def(MachineOperand* operand, unsigned step, bool real)
	{
		add_interval(operand, step, 1, real);
	}

	void add_use(MachineOperand* operand, unsigned step, bool real)
	{
		add_interval(operand, step, 0, real);
	}

	MachineOperand* get(const AllocatedVariables& allocated_variables, MachineOperand* operand)
	{
		auto color = allocated_variables.get_color(operand);
		return assignable_colors->get(color);
	}
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_LSRA_REGISTERASSIGNMENT */

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
