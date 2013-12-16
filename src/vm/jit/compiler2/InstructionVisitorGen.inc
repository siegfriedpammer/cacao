/* src/vm/jit/compiler2/InstructionVisitorGen.inc - Instruction Visitor

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

virtual void visit(LoadInst* I);
virtual void visit(UnaryInst* I);
virtual void visit(BinaryInst* I);
virtual void visit(NOPInst* I);
virtual void visit(POPInst* I);
virtual void visit(CHECKNULLInst* I);
virtual void visit(ARRAYLENGTHInst* I);
virtual void visit(ARRAYBOUNDSCHECKInst* I);
virtual void visit(NEGInst* I);
virtual void visit(CASTInst* I);
virtual void visit(ADDInst* I);
virtual void visit(SUBInst* I);
virtual void visit(MULInst* I);
virtual void visit(DIVInst* I);
virtual void visit(REMInst* I);
virtual void visit(SHLInst* I);
virtual void visit(USHRInst* I);
virtual void visit(ANDInst* I);
virtual void visit(ORInst* I);
virtual void visit(XORInst* I);
virtual void visit(CMPInst* I);
virtual void visit(CONSTInst* I);
virtual void visit(GETFIELDInst* I);
virtual void visit(PUTFIELDInst* I);
virtual void visit(PUTSTATICInst* I);
virtual void visit(GETSTATICInst* I);
virtual void visit(INCInst* I);
virtual void visit(ASTOREInst* I);
virtual void visit(ALOADInst* I);
virtual void visit(RETInst* I);
virtual void visit(LOADInst* I);
virtual void visit(STOREInst* I);
virtual void visit(NEWInst* I);
virtual void visit(NEWARRAYInst* I);
virtual void visit(ANEWARRAYInst* I);
virtual void visit(MULTIANEWARRAYInst* I);
virtual void visit(CHECKCASTInst* I);
virtual void visit(INSTANCEOFInst* I);
virtual void visit(GOTOInst* I);
virtual void visit(JSRInst* I);
virtual void visit(BUILTINInst* I);
virtual void visit(INVOKEVIRTUALInst* I);
virtual void visit(INVOKESPECIALInst* I);
virtual void visit(INVOKESTATICInst* I);
virtual void visit(INVOKEINTERFACEInst* I);
virtual void visit(IFInst* I);
virtual void visit(IF_CMPInst* I);
virtual void visit(TABLESWITCHInst* I);
virtual void visit(LOOKUPSWITCHInst* I);
virtual void visit(RETURNInst* I);
virtual void visit(THROWInst* I);
virtual void visit(COPYInst* I);
virtual void visit(MOVEInst* I);
virtual void visit(GETEXCEPTIONInst* I);
virtual void visit(PHIInst* I);
virtual void visit(ContainerInst* I);
virtual void visit(BeginInst* I);
virtual void visit(EndInst* I);

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
