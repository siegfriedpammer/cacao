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

#include <sstream>
#include "vm/jit/compiler2/CodeGenPass.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

#define print_dependency_graph_edges(begin, end, typeValue)                  \
for (DependencyGraphSet::const_iterator i = begin, e = end; i != e; i++) {   \
     DependencyGraphId PI_TO = *i;                                           \
     if (nodeNr++ > 0) {                                                     \
         OS << ", ";                                                         \
     }                                                                       \
     OS<<"\n\t\t\t{\"from\": " << (unsigned long) PI << ", ";                \
     OS<<"\"to\": " << (unsigned long) PI_TO << ", ";                        \
     OS<<"\"type\": \"" << typeValue << "\"";                                \
     OS<<"}";                                                                \
 }                                                                           \

 void JsonGraphPrinter::initialize(JITData &JD) {
    hasMoreMachineIndependendPasses = true;
    Method *M = JD.get_Method();

	static unsigned id_counter = 0;

    std::string filename = M->get_name_utf8().begin();
	filename += std::to_string(id_counter++);
    filename += ".json";
    file = fopen(filename.c_str(),"w");
    passNr = 0;
    instrNr = 0;
    edgeNr = 0;

    OStream	OS(file);
    OS << "{ \n\t\"class\": \"" << M->get_class_name_utf8() << "\",";
    OS << "\n\t\"method\": \"" << M->get_name_utf8() << "\",";
    OS << "\n\t\"desc\": \""<< M->get_desc_utf8() << "\",";

    // print pass dependency graph
    OS << "\n\t\"graph\": [";
    OS<<"\n\t\t{";
    OS<<"\n\t\t\"type\": \"PassDependencyGraph\",";

    // print nodes
    OS<<"\n\t\t\"nodes\": [";
    int nodeNr = 0;
	auto& PM = PassManager::get();
    for(PassManager::PassInfoMapTy::const_iterator i = PM.registered_begin(),
                e = PM.registered_end(); i != e; ++i) {
       PassInfo::IDTy PI = i->first;
       if (nodeNr++ > 0) {
          OS << ", ";
       }
       OS<<"\n\t\t\t{\"id\": " << (unsigned long) PI << ", ";
       OS<<"\"name\": \"" << i->second->get_name() << "\"";
       OS<<"}";
    }
    OS<<"],\n";
    // print edges
    OS<<"\n\t\t\"edges\": [";
    nodeNr = 0;
    for(PassManager::PassInfoMapTy::const_iterator i = PM.registered_begin(),
                e = PM.registered_end(); i != e; ++i) {

       DependencyGraphId PI = i->first;

       Pass *pass = i->second->create_Pass();
       PassUsage PU;
       pass->get_PassUsage(PU);
       delete pass;

       print_dependency_graph_edges(PU.requires_begin(), PU.requires_end(), "requires");
       print_dependency_graph_edges(PU.modifies_begin(), PU.modifies_end(), "modifies");
	   print_dependency_graph_edges(PU.requires_end(), PU.requires_end(), "destroys");
	   print_dependency_graph_edges(PU.requires_end(), PU.requires_end(), "schedule-after");
	   print_dependency_graph_edges(PU.requires_end(), PU.requires_end(), "schedule-before");
	   print_dependency_graph_edges(PU.requires_end(), PU.requires_end(), "run-before");
       //print_dependency_graph_edges(PU.destroys_begin(), PU.destroys_end(), "destroys");
       //print_dependency_graph_edges(PU.schedule_after_begin(), PU.schedule_after_end(), "schedule-after");
       //print_dependency_graph_edges(PU.run_before_begin(), PU.run_before_end(), "schedule-before");
       //print_dependency_graph_edges(PU.schedule_before_begin(), PU.schedule_before_end(), "run-before");
    }
    OS<<"]\n";
    OS<<"\n\t\t}],";

    OS << "\n\t\"passes\": [\n";
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
       OS<<"\t\t\"time\":" <<  duration << ", \n";;
    }

    // schedule can be stored as globalSchedule or in the instruction
    GlobalSchedule* schedule = dynamic_cast<GlobalSchedule*>(pass);
    MachineInstructionSchedule* instructionSelection = dynamic_cast<MachineInstructionSchedule*>(pass);
    if (instructionSelection) {
       hasMoreMachineIndependendPasses = false;
    }

    OS<<"\t\t\"graph\": [";

    // print HIR
    if (hasMoreMachineIndependendPasses) {
       OS << "\n\t\t\t{";
       OS << "\n\t\t\t\"type\": \"HIR\",";
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
    OS<<"]";

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
    OS<<"\"type\": \"" << get_type_name(I->get_type()) << "\"";
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