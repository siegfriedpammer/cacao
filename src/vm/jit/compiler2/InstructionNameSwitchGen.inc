/* src/vm/jit/compiler2/InstructionNameSwitchGen.inc - Instruction name switch

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

case LoadInstID: return "LoadInst";
case UnaryInstID: return "UnaryInst";
case BinaryInstID: return "BinaryInst";
case NOPInstID: return "NOPInst";
case POPInstID: return "POPInst";
case CHECKNULLInstID: return "CHECKNULLInst";
case ARRAYLENGTHInstID: return "ARRAYLENGTHInst";
case ARRAYBOUNDSCHECKInstID: return "ARRAYBOUNDSCHECKInst";
case NEGInstID: return "NEGInst";
case CASTInstID: return "CASTInst";
case ADDInstID: return "ADDInst";
case SUBInstID: return "SUBInst";
case MULInstID: return "MULInst";
case DIVInstID: return "DIVInst";
case REMInstID: return "REMInst";
case SHLInstID: return "SHLInst";
case USHRInstID: return "USHRInst";
case ANDInstID: return "ANDInst";
case ORInstID: return "ORInst";
case XORInstID: return "XORInst";
case CMPInstID: return "CMPInst";
case CONSTInstID: return "CONSTInst";
case GETFIELDInstID: return "GETFIELDInst";
case PUTFIELDInstID: return "PUTFIELDInst";
case PUTSTATICInstID: return "PUTSTATICInst";
case GETSTATICInstID: return "GETSTATICInst";
case INCInstID: return "INCInst";
case ASTOREInstID: return "ASTOREInst";
case ALOADInstID: return "ALOADInst";
case RETInstID: return "RETInst";
case LOADInstID: return "LOADInst";
case STOREInstID: return "STOREInst";
case NEWInstID: return "NEWInst";
case NEWARRAYInstID: return "NEWARRAYInst";
case ANEWARRAYInstID: return "ANEWARRAYInst";
case MULTIANEWARRAYInstID: return "MULTIANEWARRAYInst";
case CHECKCASTInstID: return "CHECKCASTInst";
case INSTANCEOFInstID: return "INSTANCEOFInst";
case GOTOInstID: return "GOTOInst";
case JSRInstID: return "JSRInst";
case BUILTINInstID: return "BUILTINInst";
case INVOKEVIRTUALInstID: return "INVOKEVIRTUALInst";
case INVOKESPECIALInstID: return "INVOKESPECIALInst";
case INVOKESTATICInstID: return "INVOKESTATICInst";
case INVOKEINTERFACEInstID: return "INVOKEINTERFACEInst";
case IFInstID: return "IFInst";
case IF_CMPInstID: return "IF_CMPInst";
case TABLESWITCHInstID: return "TABLESWITCHInst";
case LOOKUPSWITCHInstID: return "LOOKUPSWITCHInst";
case RETURNInstID: return "RETURNInst";
case THROWInstID: return "THROWInst";
case COPYInstID: return "COPYInst";
case MOVEInstID: return "MOVEInst";
case GETEXCEPTIONInstID: return "GETEXCEPTIONInst";
case PHIInstID: return "PHIInst";
case ContainerInstID: return "ContainerInst";
case BeginInstID: return "BeginInst";
case EndInstID: return "EndInst";

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
