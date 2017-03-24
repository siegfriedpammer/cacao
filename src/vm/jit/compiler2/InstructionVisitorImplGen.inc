/* src/vm/jit/compiler2/InstructionVisitorImplGen.inc - Instruction Visitor

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

/*
WARNING: THIS FILE IS AUTO-GENERATED! DO NOT ALTER!
Instead have a look at the generator (./instruction_gen.py)
and the input file (./instruction_table.csv).
*/

void InstructionVisitor::visit(LoadInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(UnaryInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(BinaryInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(NOPInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(POPInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(CHECKNULLInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(ARRAYLENGTHInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(ARRAYBOUNDSCHECKInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(NEGInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(CASTInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(ADDInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(SUBInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(MULInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(DIVInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(REMInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(SHLInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(USHRInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(ANDInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(ORInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(XORInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(CMPInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(CONSTInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(GETFIELDInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(PUTFIELDInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(PUTSTATICInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(GETSTATICInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(INCInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(AREFInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(ASTOREInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(ALOADInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(RETInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(LOADInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(STOREInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(NEWInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(NEWARRAYInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(ANEWARRAYInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(MULTIANEWARRAYInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(CHECKCASTInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(INSTANCEOFInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(GOTOInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(JSRInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(BUILTINInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(INVOKEInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(INVOKEVIRTUALInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(INVOKESPECIALInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(INVOKESTATICInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(INVOKEINTERFACEInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(IFInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(IF_CMPInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(TABLESWITCHInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(LOOKUPSWITCHInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(RETURNInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(THROWInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(COPYInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(MOVEInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(GETEXCEPTIONInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(PHIInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(ContainerInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(BeginInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(EndInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(SourceStateInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(ReplacementEntryInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(AssumptionInst* I, bool copyOperands) {visit_default(I);}
void InstructionVisitor::visit(DeoptimizeInst* I, bool copyOperands) {visit_default(I);}

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
