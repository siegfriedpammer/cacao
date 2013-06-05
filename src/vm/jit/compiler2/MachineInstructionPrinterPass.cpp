/* src/vm/jit/compiler2/MachineInstructionPrinterPass.cpp - MachineInstructionPrinterPass

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

#include "vm/jit/compiler2/MachineInstructionPrinterPass.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/Method.hpp"
#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/LoweringPass.hpp"
#include "vm/jit/compiler2/ListSchedulingPass.hpp"

#include "toolbox/GraphTraits.hpp"

#include "vm/utf8.hpp"
#include "vm/jit/jit.hpp"

#include <sstream>

#define DEBUG_NAME "compiler2/MachineInstructionPrinterPass"

namespace {
std::string get_filename(methodinfo *m, jitdata *jd, std::string prefix = "cfg_", std::string suffix=".dot");
std::string get_filename(methodinfo *m, jitdata *jd, std::string prefix, std::string suffix)
{
	std::string filename = prefix;
	filename += Utf8String(m->clazz->name).begin();
	filename += ".";
	filename += Utf8String(m->name).begin();
	filename += Utf8String(m->descriptor).begin();
	filename += suffix;
	/* replace unprintable chars */
	for (size_t i = filename.find_first_of('/');
		 i != std::string::npos;
		 i = filename.find_first_of('/',i+1)) {
		filename.replace(i,1,1,'.');
	}
	const char *unchar = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.";
	for (size_t i = filename.find_first_not_of(unchar);
		i != std::string::npos;
		i = filename.find_first_not_of(unchar,i+1)) {
		filename.replace(i,1,1,'_');
	}

	return filename;
}
} // end anonymous namespace

namespace cacao {
namespace jit {
namespace compiler2 {

namespace {

StringBuf MachineOperandToString(MachineOperand* MO) {
	if (!MO) {
		return "NULL";
	}
	std::ostringstream sstream;
	sstream << MO->get_name();
	if (Register *reg = MO->to_Register()) {
		if (VirtualRegister *vreg = reg->to_VirtualRegister() ) {
			sstream << vreg->get_id();
		}
	 } else if (StackSlot *slot = MO->to_StackSlot()) {
		sstream << "(" << slot->get_index() << ")";
	 }

	return sstream.str();
}

class MachineInstructionGraph : public GraphTraits<Method,MachineInstruction> {
protected:
    const Method &M;
	LoweringPass *LP;
	StringBuf name;
    bool verbose;
	std::set<EdgeType> data_dep;
	std::set<EdgeType> sched_dep;
	std::set<EdgeType> cfg_edges;
	std::set<EdgeType> begin2end_edges;

public:

    MachineInstructionGraph(const Method &M, LoweringPass *LP,
			InstructionSchedule<Instruction> *IS,
			StringBuf name = "MachineInstructionGraph",bool verbose = false)
			: M(M), LP(LP), name(name), verbose(verbose) {
		for(Method::InstructionListTy::const_iterator i = M.begin(),
		    e = M.end(); i != e; ++i) {
			Instruction *I = *i;
			if (I == NULL)
				continue;

			// set cluster name
			#if 0
			StringBuf c_name = "";
			c_name += (unsigned long)I->get_id();
			c_name += " ";
			c_name += I->get_name();
			cluster_name[(unsigned long)I] = c_name;
			#endif
			cluster_name[(unsigned long)I] = I->get_name();

			LoweredInstDAG *dag = LP->get_LoweredInstDAG(I);
			if (!dag)
				continue;
			for (LoweredInstDAG::const_mi_iterator i = dag->mi_begin(), e = dag->mi_end() ;
					i != e; ++i) {
				MachineInstruction *MI = *i;
				nodes.insert(MI);
				// clustering
				clusters[(unsigned long)I].insert(MI);
			}
			// add operator link
			unsigned op_idx = 0;
			for(Instruction::OperandListTy::const_iterator ii = I->op_begin(), ee = I->op_end();
				ii != ee; ++ii) {
				Value *v = (*ii);
				if (v) {
					Instruction *II = (*ii)->to_Instruction();
					if (II) {

						LoweredInstDAG *op_dag = LP->get_LoweredInstDAG(II);
						assert(op_dag);
						MachineInstruction *result = op_dag->get_result();
						assert(result);
						MachineInstruction *op = (*dag)[op_idx].first;
						LOG("Data Edge: " << result << " -> " << op << nl);
						EdgeType edge = std::make_pair(result,op);
						data_dep.insert(edge);
						edges.insert(edge);
					}
				}
				op_idx++;
			}
			// add dependency link
			for(Instruction::DepListTy::const_iterator ii = I->dep_begin(), ee = I->dep_end();
				ii != ee; ++ii) {
				Instruction *II = (*ii);
				if (II) {
					LoweredInstDAG *op_dag = LP->get_LoweredInstDAG(II);
					assert(op_dag);
					MachineInstruction *result = op_dag->get_result();
					assert(result);
					MachineInstruction *op = dag->get_result();
					EdgeType edge = std::make_pair(result,op);
					sched_dep.insert(edge);
					edges.insert(edge);
				}
			}
			// add successor link
			EndInst *EI = I->to_EndInst();
			if (EI) {
				for(EndInst::SuccessorListTy::const_iterator ii = EI->succ_begin(), ee = EI->succ_end();
					ii != ee; ++ii) {
					Value *v = (*ii);
					if (v) {
						Instruction *II = (*ii)->to_Instruction();
						if (II) {
							LoweredInstDAG *op_dag = LP->get_LoweredInstDAG(II);
							assert(op_dag);
							MachineInstruction *result = op_dag->get_result();
							assert(result);
							MachineInstruction *op = dag->get_result();
							EdgeType edge = std::make_pair(result,op);
							cfg_edges.insert(edge);
							edges.insert(edge);
						}
					}
				}
				#if 0
				EdgeType edge = std::make_pair(EI->get_BeginInst(),EI);
				begin2end_edges.insert(edge);
				edges.insert(edge);
				#endif
			}
			#if 0
			if (IS) {
				for (Method::const_bb_iterator i = M.bb_begin(), e = M.bb_end();
						i != e; ++i) {
					BeginInst *BI = *i;
					MachineInstruction *old = NULL;
					for(InstructionSchedule<Instruction>::const_inst_iterator
							i = IS->inst_begin(BI), e = IS->inst_end(BI); i != e ; ++i) {
						Instruction *I = *i;
						LoweredInstDAG *op_dag = LP->get_LoweredInstDAG(I);
						assert(op_dag);
						MachineInstruction *result = op_dag->get_result();
						assert(result);
						if (old) {
							EdgeType edge = std::make_pair(old,result);
							edges.insert(edge);
						}
						old = result;
					}

				}
			}
			#endif
#if 0
			BeginInst *bi = I->get_BeginInst();
			if (bi) {
				clusters[(unsigned long)bi].insert(I);
			}
#endif
		}
	}

	unsigned long getNodeID(const MachineInstruction &node) const {
		return node.get_id();
	}

    StringBuf getGraphName() const {
		return name;
	}

    StringBuf getNodeLabel(const MachineInstruction &node) const {
		std::ostringstream sstream;
		MachineOperand *result = node.get_result();
		assert(result);
		sstream << MachineOperandToString(result)
		        << " = [" << node.get_id() << "] "
		        << node.get_name() << " ";

		for (MachineInstruction::const_operand_iterator i = node.begin(), e = node.end();
				i != e; ++i) {
			MachineOperand *MO = *i;
			//assert(MO);
			sstream << MachineOperandToString(MO) << ", ";
		}
		#if 0
		for(Instruction::OperandListTy::const_iterator ii = node.op_begin(), ee = node.op_end();
				ii != ee; ++ii) {
			sstream << " ";
			Value *v = (*ii);
			if (v) {
				Instruction *II = (*ii)->to_Instruction();
				sstream << "[" << II->get_id() << "]";
			} else {
				sstream << "NULL";
			}
		}
		#endif
		return sstream.str();
	}

#if 0
    StringBuf getNodeAttributes(const MachineInstructionGraph::NodeType &node) const ;

    StringBuf getEdgeLabel(const MachineInstructionGraph::EdgeType &e) const ;
#endif

    StringBuf getEdgeAttributes(const MachineInstructionGraph::EdgeType &e) const {
		StringBuf attr;
		if (data_dep.find(e) != data_dep.end()) {
			attr +="color=red,";
		}
		if (sched_dep.find(e) != sched_dep.end()) {
			attr +="color=blue,";
		}
		if (begin2end_edges.find(e) != begin2end_edges.end()) {
			attr +="style=dashed,";
		}
		return attr;
	}
};

} // end anonymous namespace

PassUsage& MachineInstructionPrinterPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires(LoweringPass::ID);
	return PU;
}
// the address of this variable is used to identify the pass
char MachineInstructionPrinterPass::ID = 0;

// register pass
static PassRegistery<MachineInstructionPrinterPass> X("MachineInstructionPrinterPass");

// run pass
bool MachineInstructionPrinterPass::run(JITData &JD) {
	LoweringPass *LP = get_Pass<LoweringPass>();
	InstructionSchedule<Instruction> *IS = get_Pass_if_available<ListSchedulingPass>();

	StringBuf name = get_filename(JD.get_jitdata()->m,JD.get_jitdata(),"","");
	StringBuf filename = "minst_";
	filename+=name+".dot";
	GraphPrinter<MachineInstructionGraph>::print(filename.c_str(), MachineInstructionGraph(*(JD.get_Method()),LP,IS,name));
	return true;
}

} // end namespace cacao
} // end namespace jit
} // end namespace compiler2


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
