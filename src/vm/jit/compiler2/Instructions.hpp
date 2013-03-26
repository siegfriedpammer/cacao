/* src/vm/jit/compiler2/Instructions.hpp - Instructions

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

/**
 * This file contains the Instruction class.
 * The Instruction class is the base class for all other instruction types.
 * They are defined in Instructions.hpp.
 */

#ifndef _JIT_COMPILER2_INSTRUCTIONS
#define _JIT_COMPILER2_INSTRUCTIONS

#include "vm/jit/compiler2/Instruction.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

struct NOPInst : public Instruction {
	explicit NOPInst(Type *type) : Instruction(NOPInstID, type) {}
	virtual toNOPInst() { return this; }
};

struct POPInst : public Instruction {
	explicit POPInst(Type *type) : Instruction(POPInstID, type) {}
	virtual toPOPInst() { return this; }
};

struct CHECKNULLInst : public Instruction {
	explicit CHECKNULLInst(Type *type) : Instruction(CHECKNULLInstID, type) {}
	virtual toCHECKNULLInst() { return this; }
};

struct ARRAYLENGTHInst : public Instruction {
	explicit ARRAYLENGTHInst(Type *type) : Instruction(ARRAYLENGTHInstID, type) {}
	virtual toARRAYLENGTHInst() { return this; }
};

struct NEGInst : public Instruction {
	explicit NEGInst(Type *type) : Instruction(NEGInstID, type) {}
	virtual toNEGInst() { return this; }
};

struct CASTInst : public Instruction {
	explicit CASTInst(Type *type) : Instruction(CASTInstID, type) {}
	virtual toCASTInst() { return this; }
};

struct ADDInst : public Instruction {
	explicit ADDInst(Type *type) : Instruction(ADDInstID, type) {}
	virtual toADDInst() { return this; }
};

struct SUBInst : public Instruction {
	explicit SUBInst(Type *type) : Instruction(SUBInstID, type) {}
	virtual toSUBInst() { return this; }
};

struct MULInst : public Instruction {
	explicit MULInst(Type *type) : Instruction(MULInstID, type) {}
	virtual toMULInst() { return this; }
};

struct DIVInst : public Instruction {
	explicit DIVInst(Type *type) : Instruction(DIVInstID, type) {}
	virtual toDIVInst() { return this; }
};

struct REMInst : public Instruction {
	explicit REMInst(Type *type) : Instruction(REMInstID, type) {}
	virtual toREMInst() { return this; }
};

struct SHLInst : public Instruction {
	explicit SHLInst(Type *type) : Instruction(SHLInstID, type) {}
	virtual toSHLInst() { return this; }
};

struct USHRInst : public Instruction {
	explicit USHRInst(Type *type) : Instruction(USHRInstID, type) {}
	virtual toUSHRInst() { return this; }
};

struct ANDInst : public Instruction {
	explicit ANDInst(Type *type) : Instruction(ANDInstID, type) {}
	virtual toANDInst() { return this; }
};

struct ORInst : public Instruction {
	explicit ORInst(Type *type) : Instruction(ORInstID, type) {}
	virtual toORInst() { return this; }
};

struct XORInst : public Instruction {
	explicit XORInst(Type *type) : Instruction(XORInstID, type) {}
	virtual toXORInst() { return this; }
};

struct CMPInst : public Instruction {
	explicit CMPInst(Type *type) : Instruction(CMPInstID, type) {}
	virtual toCMPInst() { return this; }
};

struct CONSTInst : public Instruction {
	explicit CONSTInst(Type *type) : Instruction(CONSTInstID, type) {}
	virtual toCONSTInst() { return this; }
};

struct GETFIELDInst : public Instruction {
	explicit GETFIELDInst(Type *type) : Instruction(GETFIELDInstID, type) {}
	virtual toGETFIELDInst() { return this; }
};

struct PUTFIELDInst : public Instruction {
	explicit PUTFIELDInst(Type *type) : Instruction(PUTFIELDInstID, type) {}
	virtual toPUTFIELDInst() { return this; }
};

struct PUTSTATICInst : public Instruction {
	explicit PUTSTATICInst(Type *type) : Instruction(PUTSTATICInstID, type) {}
	virtual toPUTSTATICInst() { return this; }
};

struct GETSTATICInst : public Instruction {
	explicit GETSTATICInst(Type *type) : Instruction(GETSTATICInstID, type) {}
	virtual toGETSTATICInst() { return this; }
};

struct INCInst : public Instruction {
	explicit INCInst(Type *type) : Instruction(INCInstID, type) {}
	virtual toINCInst() { return this; }
};

struct ASTOREInst : public Instruction {
	explicit ASTOREInst(Type *type) : Instruction(ASTOREInstID, type) {}
	virtual toASTOREInst() { return this; }
};

struct ALOADInst : public Instruction {
	explicit ALOADInst(Type *type) : Instruction(ALOADInstID, type) {}
	virtual toALOADInst() { return this; }
};

struct RETInst : public Instruction {
	explicit RETInst(Type *type) : Instruction(RETInstID, type) {}
	virtual toRETInst() { return this; }
};

struct LOADInst : public Instruction {
	explicit LOADInst(Type *type) : Instruction(LOADInstID, type) {}
	virtual toLOADInst() { return this; }
};

struct STOREInst : public Instruction {
	explicit STOREInst(Type *type) : Instruction(STOREInstID, type) {}
	virtual toSTOREInst() { return this; }
};

struct NEWInst : public Instruction {
	explicit NEWInst(Type *type) : Instruction(NEWInstID, type) {}
	virtual toNEWInst() { return this; }
};

struct NEWARRAYInst : public Instruction {
	explicit NEWARRAYInst(Type *type) : Instruction(NEWARRAYInstID, type) {}
	virtual toNEWARRAYInst() { return this; }
};

struct ANEWARRAYInst : public Instruction {
	explicit ANEWARRAYInst(Type *type) : Instruction(ANEWARRAYInstID, type) {}
	virtual toANEWARRAYInst() { return this; }
};

struct MULTIANEWARRAYInst : public Instruction {
	explicit MULTIANEWARRAYInst(Type *type) : Instruction(MULTIANEWARRAYInstID, type) {}
	virtual toMULTIANEWARRAYInst() { return this; }
};

struct CHECKCASTInst : public Instruction {
	explicit CHECKCASTInst(Type *type) : Instruction(CHECKCASTInstID, type) {}
	virtual toCHECKCASTInst() { return this; }
};

struct INSTANCEOFInst : public Instruction {
	explicit INSTANCEOFInst(Type *type) : Instruction(INSTANCEOFInstID, type) {}
	virtual toINSTANCEOFInst() { return this; }
};

struct GOTOInst : public Instruction {
	explicit GOTOInst(Type *type) : Instruction(GOTOInstID, type) {}
	virtual toGOTOInst() { return this; }
};

struct JSRInst : public Instruction {
	explicit JSRInst(Type *type) : Instruction(JSRInstID, type) {}
	virtual toJSRInst() { return this; }
};

struct BUILTINInst : public Instruction {
	explicit BUILTINInst(Type *type) : Instruction(BUILTINInstID, type) {}
	virtual toBUILTINInst() { return this; }
};

struct INVOKEVIRTUALInst : public Instruction {
	explicit INVOKEVIRTUALInst(Type *type) : Instruction(INVOKEVIRTUALInstID, type) {}
	virtual toINVOKEVIRTUALInst() { return this; }
};

struct INVOKESPECIALInst : public Instruction {
	explicit INVOKESPECIALInst(Type *type) : Instruction(INVOKESPECIALInstID, type) {}
	virtual toINVOKESPECIALInst() { return this; }
};

struct INVOKESTATICInst : public Instruction {
	explicit INVOKESTATICInst(Type *type) : Instruction(INVOKESTATICInstID, type) {}
	virtual toINVOKESTATICInst() { return this; }
};

struct INVOKEINTERFACEInst : public Instruction {
	explicit INVOKEINTERFACEInst(Type *type) : Instruction(INVOKEINTERFACEInstID, type) {}
	virtual toINVOKEINTERFACEInst() { return this; }
};

struct IFInst : public Instruction {
	explicit IFInst(Type *type) : Instruction(IFInstID, type) {}
	virtual toIFInst() { return this; }
};

struct IF_CMPInst : public Instruction {
	explicit IF_CMPInst(Type *type) : Instruction(IF_CMPInstID, type) {}
	virtual toIF_CMPInst() { return this; }
};

struct TABLESWITCHInst : public Instruction {
	explicit TABLESWITCHInst(Type *type) : Instruction(TABLESWITCHInstID, type) {}
	virtual toTABLESWITCHInst() { return this; }
};

struct LOOKUPSWITCHInst : public Instruction {
	explicit LOOKUPSWITCHInst(Type *type) : Instruction(LOOKUPSWITCHInstID, type) {}
	virtual toLOOKUPSWITCHInst() { return this; }
};

struct RETURNInst : public Instruction {
	explicit RETURNInst(Type *type) : Instruction(RETURNInstID, type) {}
	virtual toRETURNInst() { return this; }
};

struct THROWInst : public Instruction {
	explicit THROWInst(Type *type) : Instruction(THROWInstID, type) {}
	virtual toTHROWInst() { return this; }
};

struct COPYInst : public Instruction {
	explicit COPYInst(Type *type) : Instruction(COPYInstID, type) {}
	virtual toCOPYInst() { return this; }
};

struct MOVEInst : public Instruction {
	explicit MOVEInst(Type *type) : Instruction(MOVEInstID, type) {}
	virtual toMOVEInst() { return this; }
};

struct GETEXCEPTIONInst : public Instruction {
	explicit GETEXCEPTIONInst(Type *type) : Instruction(GETEXCEPTIONInstID, type) {}
	virtual toGETEXCEPTIONInst() { return this; }
};

struct PHIInst : public Instruction {
	explicit PHIInst(Type *type) : Instruction(PHIInstID, type) {}
	virtual toPHIInst() { return this; }
};


} // end namespace cacao
} // end namespace jit
} // end namespace compiler2

#endif /* _JIT_COMPILER2_INSTRUCTIONS */


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
