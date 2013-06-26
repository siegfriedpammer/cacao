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
#include "vm/jit/compiler2/Conditional.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {


// Instruction groups
class LoadInst : public Instruction {
public:
	explicit LoadInst(InstID id, Type::TypeID type) : Instruction(id, type) {}
};

class UnaryInst : public Instruction {
public:
	explicit UnaryInst(InstID id, Type::TypeID type, Value* S1) : Instruction(id, type) {
		operand_list.push_back(S1);
	}
};

class BinaryInst : public Instruction {
public:
	explicit BinaryInst(InstID id, Type::TypeID type, Value* S1, Value* S2) : Instruction(id, type) {
		operand_list.push_back(S1);
		operand_list.push_back(S2);
	}
};

class CondInst : public BinaryInst {
protected:
	Conditional::CondID cond;
public:
	explicit CondInst(InstID id, Type::TypeID type, Value* S1, Value* S2, Conditional::CondID cond)
		: BinaryInst(id, type, S1, S2), cond(cond) {}
};


// Instructions

class NOPInst : public Instruction {
public:
	explicit NOPInst() : Instruction(NOPInstID, Type::VoidTypeID) {}
	virtual NOPInst* to_NOPInst() { return this; }
};

class POPInst : public Instruction {
public:
	explicit POPInst(Type::TypeID type) : Instruction(POPInstID, type) {}
	virtual POPInst* to_POPInst() { return this; }
};

class CHECKNULLInst : public Instruction {
public:
	explicit CHECKNULLInst(Type::TypeID type) : Instruction(CHECKNULLInstID, type) {}
	virtual CHECKNULLInst* to_CHECKNULLInst() { return this; }
};

class ARRAYLENGTHInst : public Instruction {
public:
	explicit ARRAYLENGTHInst(Type::TypeID type) : Instruction(ARRAYLENGTHInstID, type) {}
	virtual ARRAYLENGTHInst* to_ARRAYLENGTHInst() { return this; }
};

class NEGInst : public Instruction {
public:
	explicit NEGInst(Type::TypeID type) : Instruction(NEGInstID, type) {}
	virtual NEGInst* to_NEGInst() { return this; }
};

class CASTInst : public Instruction {
public:
	explicit CASTInst(Type::TypeID type) : Instruction(CASTInstID, type) {}
	virtual CASTInst* to_CASTInst() { return this; }
};

class ADDInst : public Instruction {
public:
	explicit ADDInst(Type::TypeID type) : Instruction(ADDInstID, type) {}
	virtual ADDInst* to_ADDInst() { return this; }
};

class SUBInst : public Instruction {
public:
	explicit SUBInst(Type::TypeID type) : Instruction(SUBInstID, type) {}
	virtual SUBInst* to_SUBInst() { return this; }
};

class MULInst : public Instruction {
public:
	explicit MULInst(Type::TypeID type) : Instruction(MULInstID, type) {}
	virtual MULInst* to_MULInst() { return this; }
};

class DIVInst : public Instruction {
public:
	explicit DIVInst(Type::TypeID type) : Instruction(DIVInstID, type) {}
	virtual DIVInst* to_DIVInst() { return this; }
};

class REMInst : public Instruction {
public:
	explicit REMInst(Type::TypeID type) : Instruction(REMInstID, type) {}
	virtual REMInst* to_REMInst() { return this; }
};

class SHLInst : public Instruction {
public:
	explicit SHLInst(Type::TypeID type) : Instruction(SHLInstID, type) {}
	virtual SHLInst* to_SHLInst() { return this; }
};

class USHRInst : public Instruction {
public:
	explicit USHRInst(Type::TypeID type) : Instruction(USHRInstID, type) {}
	virtual USHRInst* to_USHRInst() { return this; }
};

class ANDInst : public Instruction {
public:
	explicit ANDInst(Type::TypeID type) : Instruction(ANDInstID, type) {}
	virtual ANDInst* to_ANDInst() { return this; }
};

class ORInst : public Instruction {
public:
	explicit ORInst(Type::TypeID type) : Instruction(ORInstID, type) {}
	virtual ORInst* to_ORInst() { return this; }
};

class XORInst : public Instruction {
public:
	explicit XORInst(Type::TypeID type) : Instruction(XORInstID, type) {}
	virtual XORInst* to_XORInst() { return this; }
};

class CMPInst : public Instruction {
public:
	explicit CMPInst(Type::TypeID type) : Instruction(CMPInstID, type) {}
	virtual CMPInst* to_CMPInst() { return this; }
};

class CONSTInst : public Instruction {
public:
	explicit CONSTInst(Type::TypeID type) : Instruction(CONSTInstID, type) {}
	virtual CONSTInst* to_CONSTInst() { return this; }
};

class GETFIELDInst : public Instruction {
public:
	explicit GETFIELDInst(Type::TypeID type) : Instruction(GETFIELDInstID, type) {}
	virtual GETFIELDInst* to_GETFIELDInst() { return this; }
};

class PUTFIELDInst : public Instruction {
public:
	explicit PUTFIELDInst(Type::TypeID type) : Instruction(PUTFIELDInstID, type) {}
	virtual PUTFIELDInst* to_PUTFIELDInst() { return this; }
};

class PUTSTATICInst : public Instruction {
public:
	explicit PUTSTATICInst(Type::TypeID type) : Instruction(PUTSTATICInstID, type) {}
	virtual PUTSTATICInst* to_PUTSTATICInst() { return this; }
};

class GETSTATICInst : public Instruction {
public:
	explicit GETSTATICInst(Type::TypeID type) : Instruction(GETSTATICInstID, type) {}
	virtual GETSTATICInst* to_GETSTATICInst() { return this; }
};

class INCInst : public Instruction {
public:
	explicit INCInst(Type::TypeID type) : Instruction(INCInstID, type) {}
	virtual INCInst* to_INCInst() { return this; }
};

class ASTOREInst : public Instruction {
public:
	explicit ASTOREInst(Type::TypeID type) : Instruction(ASTOREInstID, type) {}
	virtual ASTOREInst* to_ASTOREInst() { return this; }
};

class ALOADInst : public Instruction {
public:
	explicit ALOADInst(Type::TypeID type) : Instruction(ALOADInstID, type) {}
	virtual ALOADInst* to_ALOADInst() { return this; }
};

class RETInst : public Instruction {
public:
	explicit RETInst(Type::TypeID type) : Instruction(RETInstID, type) {}
	virtual RETInst* to_RETInst() { return this; }
};

class LOADInst : public LoadInst {
private:
	unsigned index;
public:
	explicit LOADInst(Type::TypeID type, unsigned index) : LoadInst(LOADInstID, type), index(index) {}
	virtual LOADInst* to_LOADInst() { return this; }
};

class STOREInst : public Instruction {
public:
	explicit STOREInst(Type::TypeID type) : Instruction(STOREInstID, type) {}
	virtual STOREInst* to_STOREInst() { return this; }
};

class NEWInst : public Instruction {
public:
	explicit NEWInst(Type::TypeID type) : Instruction(NEWInstID, type) {}
	virtual NEWInst* to_NEWInst() { return this; }
};

class NEWARRAYInst : public Instruction {
public:
	explicit NEWARRAYInst(Type::TypeID type) : Instruction(NEWARRAYInstID, type) {}
	virtual NEWARRAYInst* to_NEWARRAYInst() { return this; }
};

class ANEWARRAYInst : public Instruction {
public:
	explicit ANEWARRAYInst(Type::TypeID type) : Instruction(ANEWARRAYInstID, type) {}
	virtual ANEWARRAYInst* to_ANEWARRAYInst() { return this; }
};

class MULTIANEWARRAYInst : public Instruction {
public:
	explicit MULTIANEWARRAYInst(Type::TypeID type) : Instruction(MULTIANEWARRAYInstID, type) {}
	virtual MULTIANEWARRAYInst* to_MULTIANEWARRAYInst() { return this; }
};

class CHECKCASTInst : public Instruction {
public:
	explicit CHECKCASTInst(Type::TypeID type) : Instruction(CHECKCASTInstID, type) {}
	virtual CHECKCASTInst* to_CHECKCASTInst() { return this; }
};

class INSTANCEOFInst : public Instruction {
public:
	explicit INSTANCEOFInst(Type::TypeID type) : Instruction(INSTANCEOFInstID, type) {}
	virtual INSTANCEOFInst* to_INSTANCEOFInst() { return this; }
};

class GOTOInst : public Instruction {
public:
	explicit GOTOInst(Type::TypeID type) : Instruction(GOTOInstID, type) {}
	virtual GOTOInst* to_GOTOInst() { return this; }
};

class JSRInst : public Instruction {
public:
	explicit JSRInst(Type::TypeID type) : Instruction(JSRInstID, type) {}
	virtual JSRInst* to_JSRInst() { return this; }
};

class BUILTINInst : public Instruction {
public:
	explicit BUILTINInst(Type::TypeID type) : Instruction(BUILTINInstID, type) {}
	virtual BUILTINInst* to_BUILTINInst() { return this; }
};

class INVOKEVIRTUALInst : public Instruction {
public:
	explicit INVOKEVIRTUALInst(Type::TypeID type) : Instruction(INVOKEVIRTUALInstID, type) {}
	virtual INVOKEVIRTUALInst* to_INVOKEVIRTUALInst() { return this; }
};

class INVOKESPECIALInst : public Instruction {
public:
	explicit INVOKESPECIALInst(Type::TypeID type) : Instruction(INVOKESPECIALInstID, type) {}
	virtual INVOKESPECIALInst* to_INVOKESPECIALInst() { return this; }
};

class INVOKESTATICInst : public Instruction {
public:
	explicit INVOKESTATICInst(Type::TypeID type) : Instruction(INVOKESTATICInstID, type) {}
	virtual INVOKESTATICInst* to_INVOKESTATICInst() { return this; }
};

class INVOKEINTERFACEInst : public Instruction {
public:
	explicit INVOKEINTERFACEInst(Type::TypeID type) : Instruction(INVOKEINTERFACEInstID, type) {}
	virtual INVOKEINTERFACEInst* to_INVOKEINTERFACEInst() { return this; }
};

class IFInst : public CondInst {
public:
	explicit IFInst(Type::TypeID type, Value* S1, Value* S2, Conditional::CondID cond)
		: CondInst(IFInstID, type, S1, S2, cond) {}
	virtual IFInst* to_IFInst() { return this; }
};

class IF_CMPInst : public Instruction {
public:
	explicit IF_CMPInst(Type::TypeID type) : Instruction(IF_CMPInstID, type) {}
	virtual IF_CMPInst* to_IF_CMPInst() { return this; }
};

class TABLESWITCHInst : public Instruction {
public:
	explicit TABLESWITCHInst(Type::TypeID type) : Instruction(TABLESWITCHInstID, type) {}
	virtual TABLESWITCHInst* to_TABLESWITCHInst() { return this; }
};

class LOOKUPSWITCHInst : public Instruction {
public:
	explicit LOOKUPSWITCHInst(Type::TypeID type) : Instruction(LOOKUPSWITCHInstID, type) {}
	virtual LOOKUPSWITCHInst* to_LOOKUPSWITCHInst() { return this; }
};

class RETURNInst : public Instruction {
public:
	explicit RETURNInst(Type::TypeID type) : Instruction(RETURNInstID, type) {}
	virtual RETURNInst* to_RETURNInst() { return this; }
};

class THROWInst : public Instruction {
public:
	explicit THROWInst(Type::TypeID type) : Instruction(THROWInstID, type) {}
	virtual THROWInst* to_THROWInst() { return this; }
};

class COPYInst : public Instruction {
public:
	explicit COPYInst(Type::TypeID type) : Instruction(COPYInstID, type) {}
	virtual COPYInst* to_COPYInst() { return this; }
};

class MOVEInst : public Instruction {
public:
	explicit MOVEInst(Type::TypeID type) : Instruction(MOVEInstID, type) {}
	virtual MOVEInst* to_MOVEInst() { return this; }
};

class GETEXCEPTIONInst : public Instruction {
public:
	explicit GETEXCEPTIONInst(Type::TypeID type) : Instruction(GETEXCEPTIONInstID, type) {}
	virtual GETEXCEPTIONInst* to_GETEXCEPTIONInst() { return this; }
};

class PHIInst : public Instruction {
public:
	explicit PHIInst(Type::TypeID type) : Instruction(PHIInstID, type) {}
	virtual PHIInst* to_PHIInst() { return this; }
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
