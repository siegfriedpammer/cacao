/* src/vm/jit/compiler2/Instruction.hpp - Instruction

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

#ifndef _JIT_COMPILER2_INSTRUCTION
#define _JIT_COMPILER2_INSTRUCTION

#include "vm/jit/compiler2/Value.hpp"
#include "vm/jit/compiler2/Type.hpp"
#include <stddef.h>

namespace cacao {
namespace jit {
namespace compiler2 {

// Forward declarations
class BasicBlock;

// Instructions forward defs

class NOPInst;
class POPInst;
class CHECKNULLInst;
class ARRAYLENGTHInst;

class NEGInst;

class CASTInst;

/* binary */
class ADDInst;
class SUBInst;
class MULInst;
class DIVInst;
class REMInst;
class SHLInst;
class USHRInst;
class ANDInst;
class ORInst;
class XORInst;
class CMPInst;
class CONSTInst;
class GETFIELDInst;
class PUTFIELDInst;
class PUTSTATICInst;
class GETSTATICInst;
class INCInst;
class ASTOREInst;
class ALOADInst;
class RETInst;
class LOADInst;
class STOREInst;
class NEWInst;
class NEWARRAYInst;
class ANEWARRAYInst;
class MULTIANEWARRAYInst;
class CHECKCASTInst;
class INSTANCEOFInst;
#if 0
class INLINE_STARTInst;
class INLINE_ENDInst;
class INLINE_BODYInst;
#endif
class GOTOInst;
class JSRInst;
class BUILTINInst;
class INVOKEVIRTUALInst;
class INVOKESPECIALInst;
class INVOKESTATICInst;
class INVOKEINTERFACEInst;
class IFInst;
class IF_CMPInst;
class TABLESWITCHInst;
class LOOKUPSWITCHInst;
class RETURNInst;
class THROWInst;
class COPYInst;
class MOVEInst;
class GETEXCEPTIONInst;
class PHIInst;

/**
 * Instruction super class.
 * This is the base class for all instruction. The functions 'toXInstruction()'
 * can be used to cast Instructions. If casting is not possible these functions
 * return NULL. Therefor it can be used to check for a specific Instruction, e.g.:
 * <code>
 *  if (CmdInstruction* ti = i.toCmdInstruction()
 *  { // 'i' is a CmpInstruction
 *    ...
 *  }
 * </code>
 */
class Instruction : public Value {
public:
	enum InstID {
		NOPInstID,
		POPInstID,
		CHECKNULLInstID,
		ARRAYLENGTHInstID,
		NEGInstID,
		CASTInstID,
		ADDInstID,
		SUBInstID,
		MULInstID,
		DIVInstID,
		REMInstID,
		SHLInstID,
		USHRInstID,
		ANDInstID,
		ORInstID,
		XORInstID,
		CMPInstID,
		CONSTInstID,
		GETFIELDInstID,
		PUTFIELDInstID,
		PUTSTATICInstID,
		GETSTATICInstID,
		INCInstID,
		ASTOREInstID,
		ALOADInstID,
		RETInstID,
		LOADInstID,
		STOREInstID,
		NEWInstID,
		NEWARRAYInstID,
		ANEWARRAYInstID,
		MULTIANEWARRAYInstID,
		CHECKCASTInstID,
		INSTANCEOFInstID,
		GOTOInstID,
		JSRInstID,
		BUILTINInstID,
		INVOKEVIRTUALInstID,
		INVOKESPECIALInstID,
		INVOKESTATICInstID,
		INVOKEINTERFACEInstID,
		IFInstID,
		IF_CMPInstID,
		TABLESWITCHInstID,
		LOOKUPSWITCHInstID,
		RETURNInstID,
		THROWInstID,
		COPYInstID,
		MOVEInstID,
		GETEXCEPTIONInstID,
		PHIInstID,
		NoInstID
	};

private:
	BasicBlock *parent;				   ///< BasicBlock containing the instruction or NULL
	Type *type;
	const InstID id;

	explicit Instruction() : id(NoInstID) {}

public:
	explicit Instruction(InstID id, Type *type) : id(id), type(type) {}

	InstID getOpcode() const { return id; } ///< return the opcode of the instruction (icmd.hpp)
	bool isTerminator() const;              ///< true if the instruction terminates a basic block
	BasicBlock *getParent() const;          ///< get the BasicBlock in which the instruction is contained.
                                            ///< NULL if not attached to any block.

	// casting functions
	virtual Instruction* toInstruction() { return this;}

	virtual NOPInst*              toNOPInst()              { return NULL; }
	virtual POPInst*              toPOPInst()              { return NULL; }
	virtual CHECKNULLInst*        toCHECKNULLInst()        { return NULL; }
	virtual ARRAYLENGTHInst*      toARRAYLENGTHInst()      { return NULL; }
	virtual NEGInst*              toNEGInst()              { return NULL; }
	virtual CASTInst*             toCASTInst()             { return NULL; }
	virtual ADDInst*              toADDInst()              { return NULL; }
	virtual SUBInst*              toSUBInst()              { return NULL; }
	virtual MULInst*              toMULInst()              { return NULL; }
	virtual DIVInst*              toDIVInst()              { return NULL; }
	virtual REMInst*              toREMInst()              { return NULL; }
	virtual SHLInst*              toSHLInst()              { return NULL; }
	virtual USHRInst*             toUSHRInst()             { return NULL; }
	virtual ANDInst*              toANDInst()              { return NULL; }
	virtual ORInst*               toORInst()               { return NULL; }
	virtual XORInst*              toXORInst()              { return NULL; }
	virtual CMPInst*              toCMPInst()              { return NULL; }
	virtual CONSTInst*            toCONSTInst()            { return NULL; }
	virtual GETFIELDInst*         toGETFIELDInst()         { return NULL; }
	virtual PUTFIELDInst*         toPUTFIELDInst()         { return NULL; }
	virtual PUTSTATICInst*        toPUTSTATICInst()        { return NULL; }
	virtual GETSTATICInst*        toGETSTATICInst()        { return NULL; }
	virtual INCInst*              toINCInst()              { return NULL; }
	virtual ASTOREInst*           toASTOREInst()           { return NULL; }
	virtual ALOADInst*            toALOADInst()            { return NULL; }
	virtual RETInst*              toRETInst()              { return NULL; }
	virtual LOADInst*             toLOADInst()             { return NULL; }
	virtual STOREInst*            toSTOREInst()            { return NULL; }
	virtual NEWInst*              toNEWInst()              { return NULL; }
	virtual NEWARRAYInst*         toNEWARRAYInst()         { return NULL; }
	virtual ANEWARRAYInst*        toANEWARRAYInst()        { return NULL; }
	virtual MULTIANEWARRAYInst*   toMULTIANEWARRAYInst()   { return NULL; }
	virtual CHECKCASTInst*        toCHECKCASTInst()        { return NULL; }
	virtual INSTANCEOFInst*       toINSTANCEOFInst()       { return NULL; }
	virtual GOTOInst*             toGOTOInst()             { return NULL; }
	virtual JSRInst*              toJSRInst()              { return NULL; }
	virtual BUILTINInst*          toBUILTINInst()          { return NULL; }
	virtual INVOKEVIRTUALInst*    toINVOKEVIRTUALInst()    { return NULL; }
	virtual INVOKESPECIALInst*    toINVOKESPECIALInst()    { return NULL; }
	virtual INVOKESTATICInst*     toINVOKESTATICInst()     { return NULL; }
	virtual INVOKEINTERFACEInst*  toINVOKEINTERFACEInst()  { return NULL; }
	virtual IFInst*               toIFInst()               { return NULL; }
	virtual IF_CMPInst*           toIF_CMPInst()           { return NULL; }
	virtual TABLESWITCHInst*      toTABLESWITCHInst()      { return NULL; }
	virtual LOOKUPSWITCHInst*     toLOOKUPSWITCHInst()     { return NULL; }
	virtual RETURNInst*           toRETURNInst()           { return NULL; }
	virtual THROWInst*            toTHROWInst()            { return NULL; }
	virtual COPYInst*             toCOPYInst()             { return NULL; }
	virtual MOVEInst*             toMOVEInst()             { return NULL; }
	virtual GETEXCEPTIONInst*     toGETEXCEPTIONInst()     { return NULL; }
	virtual PHIInst*              toPHIInst()              { return NULL; }

};


} // end namespace cacao
} // end namespace jit
} // end namespace compiler2

#endif /* _JIT_COMPILER2_INSTRUCTION */


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
