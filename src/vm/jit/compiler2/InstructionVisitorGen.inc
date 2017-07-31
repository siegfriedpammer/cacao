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

virtual void visit(UnaryInst* I, bool copyOperands);
virtual void visit(BinaryInst* I, bool copyOperands);
virtual void visit(FieldAccessInst* I, bool copyOperands);
virtual void visit(CHECKNULLInst* I, bool copyOperands);
virtual void visit(ARRAYLENGTHInst* I, bool copyOperands);
virtual void visit(ARRAYBOUNDSCHECKInst* I, bool copyOperands);
virtual void visit(NEGInst* I, bool copyOperands);
virtual void visit(CASTInst* I, bool copyOperands);
virtual void visit(ADDInst* I, bool copyOperands);
virtual void visit(SUBInst* I, bool copyOperands);
virtual void visit(MULInst* I, bool copyOperands);
virtual void visit(DIVInst* I, bool copyOperands);
virtual void visit(REMInst* I, bool copyOperands);
virtual void visit(SHLInst* I, bool copyOperands);
virtual void visit(USHRInst* I, bool copyOperands);
virtual void visit(ANDInst* I, bool copyOperands);
virtual void visit(ORInst* I, bool copyOperands);
virtual void visit(XORInst* I, bool copyOperands);
virtual void visit(CMPInst* I, bool copyOperands);
virtual void visit(CONSTInst* I, bool copyOperands);
virtual void visit(GETFIELDInst* I, bool copyOperands);
virtual void visit(PUTFIELDInst* I, bool copyOperands);
virtual void visit(PUTSTATICInst* I, bool copyOperands);
virtual void visit(GETSTATICInst* I, bool copyOperands);
virtual void visit(INCInst* I, bool copyOperands);
virtual void visit(AREFInst* I, bool copyOperands);
virtual void visit(ASTOREInst* I, bool copyOperands);
virtual void visit(ALOADInst* I, bool copyOperands);
virtual void visit(LOADInst* I, bool copyOperands);
virtual void visit(NEWInst* I, bool copyOperands);
virtual void visit(NEWARRAYInst* I, bool copyOperands);
virtual void visit(ANEWARRAYInst* I, bool copyOperands);
virtual void visit(MULTIANEWARRAYInst* I, bool copyOperands);
virtual void visit(CHECKCASTInst* I, bool copyOperands);
virtual void visit(INSTANCEOFInst* I, bool copyOperands);
virtual void visit(GOTOInst* I, bool copyOperands);
virtual void visit(BUILTINInst* I, bool copyOperands);
virtual void visit(INVOKEInst* I, bool copyOperands);
virtual void visit(INVOKEVIRTUALInst* I, bool copyOperands);
virtual void visit(INVOKESPECIALInst* I, bool copyOperands);
virtual void visit(INVOKESTATICInst* I, bool copyOperands);
virtual void visit(INVOKEINTERFACEInst* I, bool copyOperands);
virtual void visit(IFInst* I, bool copyOperands);
virtual void visit(IF_CMPInst* I, bool copyOperands);
virtual void visit(TABLESWITCHInst* I, bool copyOperands);
virtual void visit(LOOKUPSWITCHInst* I, bool copyOperands);
virtual void visit(RETURNInst* I, bool copyOperands);
virtual void visit(THROWInst* I, bool copyOperands);
virtual void visit(PHIInst* I, bool copyOperands);
virtual void visit(BeginInst* I, bool copyOperands);
virtual void visit(EndInst* I, bool copyOperands);
virtual void visit(SourceStateInst* I, bool copyOperands);
virtual void visit(ReplacementEntryInst* I, bool copyOperands);
virtual void visit(AssumptionInst* I, bool copyOperands);
virtual void visit(DeoptimizeInst* I, bool copyOperands);

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
