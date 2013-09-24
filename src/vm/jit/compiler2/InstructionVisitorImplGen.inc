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

void InstructionVisitor::visit(LoadInst* I) {visit_default(I);}
void InstructionVisitor::visit(UnaryInst* I) {visit_default(I);}
void InstructionVisitor::visit(BinaryInst* I) {visit_default(I);}
void InstructionVisitor::visit(NOPInst* I) {visit_default(I);}
void InstructionVisitor::visit(POPInst* I) {visit_default(I);}
void InstructionVisitor::visit(CHECKNULLInst* I) {visit_default(I);}
void InstructionVisitor::visit(ARRAYLENGTHInst* I) {visit_default(I);}
void InstructionVisitor::visit(NEGInst* I) {visit_default(I);}
void InstructionVisitor::visit(CASTInst* I) {visit_default(I);}
void InstructionVisitor::visit(ADDInst* I) {visit_default(I);}
void InstructionVisitor::visit(SUBInst* I) {visit_default(I);}
void InstructionVisitor::visit(MULInst* I) {visit_default(I);}
void InstructionVisitor::visit(DIVInst* I) {visit_default(I);}
void InstructionVisitor::visit(REMInst* I) {visit_default(I);}
void InstructionVisitor::visit(SHLInst* I) {visit_default(I);}
void InstructionVisitor::visit(USHRInst* I) {visit_default(I);}
void InstructionVisitor::visit(ANDInst* I) {visit_default(I);}
void InstructionVisitor::visit(ORInst* I) {visit_default(I);}
void InstructionVisitor::visit(XORInst* I) {visit_default(I);}
void InstructionVisitor::visit(CMPInst* I) {visit_default(I);}
void InstructionVisitor::visit(CONSTInst* I) {visit_default(I);}
void InstructionVisitor::visit(GETFIELDInst* I) {visit_default(I);}
void InstructionVisitor::visit(PUTFIELDInst* I) {visit_default(I);}
void InstructionVisitor::visit(PUTSTATICInst* I) {visit_default(I);}
void InstructionVisitor::visit(GETSTATICInst* I) {visit_default(I);}
void InstructionVisitor::visit(INCInst* I) {visit_default(I);}
void InstructionVisitor::visit(ASTOREInst* I) {visit_default(I);}
void InstructionVisitor::visit(ALOADInst* I) {visit_default(I);}
void InstructionVisitor::visit(RETInst* I) {visit_default(I);}
void InstructionVisitor::visit(LOADInst* I) {visit_default(I);}
void InstructionVisitor::visit(STOREInst* I) {visit_default(I);}
void InstructionVisitor::visit(NEWInst* I) {visit_default(I);}
void InstructionVisitor::visit(NEWARRAYInst* I) {visit_default(I);}
void InstructionVisitor::visit(ANEWARRAYInst* I) {visit_default(I);}
void InstructionVisitor::visit(MULTIANEWARRAYInst* I) {visit_default(I);}
void InstructionVisitor::visit(CHECKCASTInst* I) {visit_default(I);}
void InstructionVisitor::visit(INSTANCEOFInst* I) {visit_default(I);}
void InstructionVisitor::visit(GOTOInst* I) {visit_default(I);}
void InstructionVisitor::visit(JSRInst* I) {visit_default(I);}
void InstructionVisitor::visit(BUILTINInst* I) {visit_default(I);}
void InstructionVisitor::visit(INVOKEVIRTUALInst* I) {visit_default(I);}
void InstructionVisitor::visit(INVOKESPECIALInst* I) {visit_default(I);}
void InstructionVisitor::visit(INVOKESTATICInst* I) {visit_default(I);}
void InstructionVisitor::visit(INVOKEINTERFACEInst* I) {visit_default(I);}
void InstructionVisitor::visit(IFInst* I) {visit_default(I);}
void InstructionVisitor::visit(IF_CMPInst* I) {visit_default(I);}
void InstructionVisitor::visit(TABLESWITCHInst* I) {visit_default(I);}
void InstructionVisitor::visit(LOOKUPSWITCHInst* I) {visit_default(I);}
void InstructionVisitor::visit(RETURNInst* I) {visit_default(I);}
void InstructionVisitor::visit(THROWInst* I) {visit_default(I);}
void InstructionVisitor::visit(COPYInst* I) {visit_default(I);}
void InstructionVisitor::visit(MOVEInst* I) {visit_default(I);}
void InstructionVisitor::visit(GETEXCEPTIONInst* I) {visit_default(I);}
void InstructionVisitor::visit(PHIInst* I) {visit_default(I);}
void InstructionVisitor::visit(ContainerInst* I) {visit_default(I);}
void InstructionVisitor::visit(BeginInst* I) {visit_default(I);}
void InstructionVisitor::visit(EndInst* I) {visit_default(I);}

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
