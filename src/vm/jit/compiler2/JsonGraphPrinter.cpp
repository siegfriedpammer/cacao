/* src/vm/jit/compiler2/ArrayBoundsCheckRemoverPassss.cpp - JsonGraphPrinter

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

#include "vm/jit/compiler2/JsonGraphPrinter.hpp"

#include <fstream>
#include <iostream>
#include <streambuf>
#include <sstream>
#include <map>
#include "vm/jit/compiler2/CodeGenPass.hpp"

#include "toolbox/logging.hpp"
#include "vm/jit/jit.hpp"
#include "vm/jit/disass.hpp"
#include "vm/jit/code.hpp"

#define DEBUG_NAME "compiler2/JsonGraphPrinter"

namespace cacao {
namespace jit {
namespace compiler2 {

struct DependencyGraphNode {
	const char* name;
	bool is_pass;
	bool is_artifact;
	bool is_enabled;
   unsigned long id;

   static unsigned long id_counter;

	DependencyGraphNode() : DependencyGraphNode("illegal") {}
	DependencyGraphNode(const char* name)
	    : name(name), is_pass(false), is_artifact(false), is_enabled(true), id(id_counter++)
	{
	}
	DependencyGraphNode(const DependencyGraphNode& other) = default;

	bool operator==(const DependencyGraphNode& other) const
	{
		return std::strcmp(name, other.name) == 0;
	}
};

unsigned long DependencyGraphNode::id_counter = 0;

#define print_dependency_graph_edges(begin, end, typeValue, node_id_from, lookup_map) \
std::for_each(begin, end, [&](const auto& id) {                               \
     auto node_to = lookup_map.find(id);                                      \
     if (!first) {                                                            \
         OS << ", ";                                                          \
     }                                                                        \
     OS << "\n\t\t\t{\"from\": " << ((unsigned long) node_id_from) << ", ";   \
     OS << "\"to\": " << ((unsigned long) node_to->second.id) << ", ";        \
     OS << "\"type\": \"" << typeValue << "\"";                               \
     OS << "}";                                                               \
     first = false;                                                           \
 });                                                                          \

void JsonGraphPrinter::initialize(JITData &JD) {
   hasMoreMachineIndependendPasses = true;
   Method *M = JD.get_Method();

	static unsigned id_counter = 0;

   file_name = M->get_name_utf8().begin();
   file_name += std::to_string(id_counter++);
   file_name += ".json";
   file = fopen(file_name.c_str(),"w");
   passNr = 0;
   instrNr = 0;
   edgeNr = 0;

   OStream	OS(file);
   OS << "{ \n\t\"class\": \"" << M->get_class_name_utf8() << "\",";
   OS << "\n\t\"method\": \"" << M->get_name_utf8() << "\",";
   OS << "\n\t\"desc\": \""<< M->get_desc_utf8() << "\",";

   // print pass dependency graph
   printDependencyGraph(OS);

   OS << "\n\t\"passes\": [\n";
 }

/**
 * The dependency graph contains passes and artifacts and their relations to each other.
 * Some passes can also represent an artifact.
 */
void JsonGraphPrinter::printDependencyGraph(OStream &OS) {
   
   std::map<ArtifactInfo::IDTy, DependencyGraphNode> artifact_map;
	std::map<PassInfo::IDTy, DependencyGraphNode> pass_map;
   std::vector<DependencyGraphNode> node_list;

   // handle passes
   auto& PM = PassManager::get();
   for (auto i = PM.registered_begin(), e = PM.registered_end(); i != e; ++i) {
      DependencyGraphNode node(i->second->get_name());
      node.is_pass = true;
      node_list.push_back(node);
      pass_map.emplace(i->first, node);
   }

   // handle artifacts
   for (auto i = PM.registered_artifacts_begin(), e = PM.registered_artifacts_end(); i != e;
		     ++i) {
      DependencyGraphNode node(i->second->get_name());
      node.is_artifact = true;

      auto iter = std::find_if(node_list.begin(), node_list.end(),
                                 [&](const auto& other) { return node == other; });

      if (iter != node_list.end()) {
         iter->is_artifact = true;
         artifact_map.emplace(i->first, *iter);
      }
      else {
         node_list.push_back(node);
         artifact_map.emplace(i->first, node);
      }
   }

   // evaluate which passes/artifacts are enabled
   for (auto i = PM.registered_begin(), e = PM.registered_end(); i != e; ++i) {
			std::unique_ptr<Pass> pass(i->second->create_Pass());
			PassUsage PU;
			pass->get_PassUsage(PU);

			DependencyGraphNode tmp(i->second->get_name());
			auto iter = std::find_if(node_list.begin(), node_list.end(),
			                         [&](const auto& other) { return tmp == other; });
			auto& node = *iter;
			node.is_enabled = pass->is_enabled();
   }

   // print nodes
   OS << "\n\t\"passDependencyGraph\": {";   
   OS<<"\n\t\t\"nodes\": [";   	
   bool first = true;  
   for(auto i = node_list.begin(), e = node_list.end(); i != e; ++i) {
      DependencyGraphNode node = *i;
      if (!first) {
         OS << ", ";
      }
      OS<<"\n\t\t\t{\"id\": " << node.id << ", ";
      OS<<"\"name\": \"" << node.name << "\", ";
      OS<<"\"pass\": \"" << node.is_pass << "\", ";
      OS<<"\"artifact\": \"" << node.is_artifact << "\", ";
      OS<<"\"enabled\": \"" << node.is_enabled << "\"";
      OS<<"}";
      first = false;
   }

   OS<<"],\n";
   // print edges
   OS<<"\n\t\t\"edges\": [";
   first = true; 
   for(PassManager::PassInfoMapTy::const_iterator i = PM.registered_begin(),
                e = PM.registered_end(); i != e; ++i) {

       Pass *pass = i->second->create_Pass();
       PassUsage PU;
       pass->get_PassUsage(PU);
       delete pass;

      DependencyGraphNode tmp(i->second->get_name());
      auto iter = std::find_if(node_list.begin(), node_list.end(),
                                 [&](const auto& other) { return tmp == other; });
      auto& node = *iter;

      print_dependency_graph_edges(PU.provides_begin(), PU.provides_end(), "provides", node.id, artifact_map);
      print_dependency_graph_edges(PU.requires_begin(), PU.requires_end(), "requires", node.id, artifact_map);
      print_dependency_graph_edges(PU.modifies_begin(), PU.modifies_end(), "modifies", node.id, artifact_map);

      print_dependency_graph_edges(PU.before_begin(), PU.before_end(), "schedule-before", node.id, pass_map);
      print_dependency_graph_edges(PU.after_begin(), PU.after_end(), "schedule-after", node.id, pass_map);      
      print_dependency_graph_edges(PU.imm_before_begin(), PU.imm_before_end(), "schedule-imm-before", node.id, pass_map);
      print_dependency_graph_edges(PU.imm_after_begin(), PU.imm_after_end(), "schedule-imm-after", node.id, pass_map);
   }
   OS<<"]\n";
   OS<<"\n\t\t},";
}

void JsonGraphPrinter::close() {
   OStream	OS(file);
   OS << "\n\t]\n}";
   fclose(file);
}

void JsonGraphPrinter::printPass(JITData &JD, Pass* pass, const char* passName, uint64_t duration) {

   OStream	OS(file);
   Method *M = JD.get_Method();
   if (passNr++ > 0) {
      OS << ", \n";
   }
   OS<<"\t\t{\n\t\t\"name\": \"" << passName<< "\", \n";
   if (duration > 0) {
      OS<<"\t\t\"time\":" <<  duration;
   }

   // schedule can be stored as globalSchedule or in the instruction
   GlobalSchedule* schedule = dynamic_cast<GlobalSchedule*>(pass);
   MachineInstructionSchedule* instructionSelection = dynamic_cast<MachineInstructionSchedule*>(pass);
   if (instructionSelection) {
      hasMoreMachineIndependendPasses = false;
   }

   // print HIR
   if (hasMoreMachineIndependendPasses && M->begin() != M->end()) {
      OS << ",\n\t\t\"HIR\": {";
      // nodes
      instrNr = 0;
      OS << "\n\t\t\t\"nodes\": [";
      for (Method::const_iterator i = M->begin(), e = M->end(); i != e; i++) {
         Instruction *I = *i;
         bool isRoot = M->get_init_bb() == I;
         printHIRInstruction(OS, I, isRoot, schedule);
      }
      OS << "],\n";

      // edges
      edgeNr = 0;
      OS << "\t\t\t\"edges\": [";
      for (Method::const_iterator i = M->begin(), e = M->end(); i != e; i++) {
         printHIREdges(OS, *i);
      }
      OS << "]\n\t\t\t}";
   }

   CodeGenPass* codeGenPass = dynamic_cast<CodeGenPass*>(pass);
   if (!hasMoreMachineIndependendPasses && !codeGenPass) {
      MachineInstructionSchedulingPass *MIS = PR->get_Artifact<LIRInstructionScheduleArtifact>()->MIS;
      if (MIS) {
         instrNr = 0;
         OS<<",\n\t\t\"LIR\": {";
         OS << "\n\t\t\t\"instructions\": [";
         for (MachineInstructionSchedule::iterator i = MIS->begin(), e = MIS->end(); i != e; ++i) {
            MachineBasicBlock *MBB = *i;
            printLIRInstruction(OS, MBB);
         }
         OS << "]";
         OS << "\n\t\t\t}";
      }
   #if defined(ENABLE_DISASSEMBLER)      
   } else if(codeGenPass) {         
      codeinfo *cd = JD.get_jitdata()->code;
      u1 *start = cd->entrypoint;	
      u1 *end = cd->mcode + cd->mcodelength;
      // write assembler to second file
      file_name += ".asm";
      FILE *fp = freopen(file_name.c_str(), "w+", stdout);
      for (CodeGenPass::BasicBlockMap::const_iterator i = codeGenPass->begin(), e = codeGenPass->end(); i != e ; ++i) {      
         u1 *end = start + i->second;
         LOG(*i->first << " " <<nl);
         for (; start < end; )
            start = disassinstr(start);
         start = end;   
      }
      fclose(fp);
   #endif
	}
   
   OS << "\n\t\t}";
}

void JsonGraphPrinter::printLIRInstruction(OStream &OS, MachineBasicBlock *MBB) {

   // print label
   printMachineInstruction(OS, MBB, MBB->front());

   // print phi
   for (MachineBasicBlock::const_phi_iterator i = MBB->phi_begin(),
                e = MBB->phi_end(); i != e; ++i) {
      MachinePhiInst *phi = *i;
      printMachineInstruction(OS, MBB, phi);
   }

   // print remaining instructions
   for (MachineBasicBlock::iterator i = ++MBB->begin(), e = MBB->end();
        i != e; ++i) {
      MachineInstruction *MI = *i;
      printMachineInstruction(OS, MBB, MI);
   }
}

void JsonGraphPrinter::printMachineInstruction(OStream &OS, MachineBasicBlock *MBB, MachineInstruction *MI) {
   if (instrNr++ > 0) {
      OS<<",";
   }
   OS<<"\n\t\t\t\t{\"id\": " << MI->get_id() << ", ";
   OS<<"\"name\": \"" << MI->get_name() << "\", ";
   OS<<"\"BB\": \"" << *MBB << "\", ";

   // print operands
   std::size_t operandNr = 0;
   OS << "\"operands\": [";
   for (MachineInstruction::const_operand_iterator i = MI->begin(), e = MI->end(); i != e ; ++i) {
      if (operandNr++ > 0) {
         OS << ", ";
      }
      OS << "\"" << (*i) << "\"";
   }
   OS << "], ";

   // print result
   OS<<"\"result\": \"";
   MachineOperand *result = MI->get_result().op;
   if (!result->to_VoidOperand()) {
      OS << result;
   }
   OS<<"\"";

   // print successors
   if (!MI->successor_empty()) {
      std::size_t index = 0;
      OS << ", \"successors\": [";
      for (MachineInstruction::const_successor_iterator i = MI->successor_begin(), e = MI->successor_end(); i != e; ++i) {
         if (operandNr++ > 0) {
            OS << ", ";
         }
         OS << "\"";
         MI->print_successor_label(OS, index);
         OS << "=" << (**i) << "\"";
         ++ index;
      }
      OS << "]";
   }
   OS<<"}";
}

void JsonGraphPrinter::printHIRInstruction(OStream &OS, Instruction *I, bool isRoot, GlobalSchedule* schedule) {

   if (instrNr++ > 0) {
      OS << ", ";
   }
   OS<<"\n\t\t\t\t{\"id\": " << I->get_id() << ", ";
   OS<<"\"name\": \"" << I->get_name() << "\", ";
   OS<<"\"type\": \"" << I->get_type() << "\"";
   if (isRoot) {
      OS<<", \"root\": " << isRoot << "";
   }

   // print the basic block scheduled
   Instruction* beginInst = (schedule) ? schedule->get(I) : I->get_BeginInst();
   if (beginInst) {
      OS<<", \"BB\": "  << beginInst->get_id();
   }

   if (I->has_side_effects()) {
      OS << ", \"sideEffects\": " << I->has_side_effects();
   }

   if (I->to_IFInst()) {
      Conditional::CondID C = I->to_IFInst()->get_condition();
      OS << ", \"condition\": \"" << C << "\"";
   }

   if (I->to_CONSTInst()) {
      CONSTInst* inst = I->to_CONSTInst();
      OS << ", \"value\": ";
      switch(inst->get_type()) {
         case Type::LongTypeID:   OS << inst->get_Long(); break;
         case Type::IntTypeID:    OS << inst->get_Int(); break;
         case Type::FloatTypeID:  OS << inst->get_Float(); break;
         case Type::DoubleTypeID: OS << inst->get_Double(); break;
         default: assert(0);
      }
   }

   if (I->op_size() > 0) {
      int operandNr = 0;
      OS << ", \"operands\": [";
      for (Instruction::OperandListTy::const_iterator ii = I->op_begin(), ee = I->op_end(); ii != ee; ++ii) {
         Instruction *II = (*ii)->to_Instruction();
         if (II) {
            if (operandNr++ > 0) {
               OS << ", ";
            }
            OS << II->get_id();
         }
      }
      OS << "]";
   }
   OS<<"}";
}

void JsonGraphPrinter::printHIREdges(OStream &OS, Instruction *I) {

   // operands
   if (I->op_size() > 0) {
      for (Instruction::OperandListTy::const_iterator ii = I->op_begin(), ee = I->op_end(); ii != ee; ++ii) {
         Instruction *II = (*ii)->to_Instruction();
         if (II) {
            if (edgeNr++ > 0) {
               OS << ", ";
            }
            OS << "\n\t\t\t\t{\"from\": " << II->get_id() << ", \"to\": " << I->get_id() << ", \"type\": \"op\"}";
         }
      }
   }

   // control flow
   BeginInst *BI = I->to_BeginInst();
   if (BI) {
      EndInst *EI = BI->get_EndInst();
      if (edgeNr++ > 0) {
         OS << ", ";
      }
      OS << "\n\t\t\t\t{\"from\": " << BI->get_id() << ", \"to\": " << EI->get_id() << ", \"type\": \"bb\"}";
   }

   EndInst *EI = I->to_EndInst();
   if (EI) {
      for (EndInst::SuccessorListTy::const_iterator ii = EI->succ_begin(), ee = EI->succ_end(); ii != ee; ++ii) {
         Instruction *II = ii->get()->to_Instruction();
         if (II) {
            if (edgeNr++ > 0) {
               OS << ", ";
            }
            OS << "\n\t\t\t\t{\"from\": " << EI->get_id() << ", \"to\": " << II->get_id() << ", \"type\": \"cfg\"";

            if (EI->to_IFInst()) {
               IFInst *ifInst = EI->to_IFInst();
               bool isTrueBranch = ifInst->get_then_target() == II;
               OS << ", \"trueBranch\": " << isTrueBranch;
            }
            OS << "}";
         }
      }
   }

   // scheduling dependencies
   if (I->dep_size() > 0) {
      for (Instruction::DepListTy::const_iterator ii = I->dep_begin(), ee = I->dep_end(); ii != ee; ++ii) {
         Instruction *II = (*ii);
         if (II) {
            if (edgeNr++ > 0) {
               OS << ", ";
            }
            OS << "\n\t\t\t\t{\"from\": " << II->get_id() << ", \"to\": " << I->get_id() <<
            ", \"type\": \"sched\"}";
         }
      }
   }
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