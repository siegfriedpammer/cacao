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
#include "vm/jit/compiler2/Method.hpp"

#include <vector>
#include <set>
#include <algorithm>

#include <cstddef>

namespace cacao {
namespace jit {
namespace compiler2 {

// Forward declarations
class BasicBlock;

// Instruction groups
class LoadInst;
class UnaryInst;
class BinaryInst;

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

// Control Flow Meta Instructions
class BeginInst;
class EndInst;

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
	typedef std::vector<Value*> OperandListTy;
	typedef std::vector<Instruction*> DepListTy;

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
		// Control Flow Meta Instructions
		BeginInstID,
		EndInstID,

		NoInstID
	};
private:
	OperandListTy op_list;
	DepListTy dep_list;

protected:
	const InstID id;
	BasicBlock *parent;				   ///< BasicBlock containing the instruction or NULL
	Type::TypeID type;
	Method* method;

	explicit Instruction() : id(NoInstID) {}

	void append_op(Value* v) {
		op_list.push_back(v);
		v->append_user(this);
	}

	bool replace_op(Value* v_old, Value* v_new) {
		OperandListTy::iterator f = std::find(op_list.begin(),op_list.end(),v_old);
		if (f != op_list.end()) {
			std::replace(f,f+1,v_old,v_new);
			v_old->remove_user(this);
			if (v_new)
				v_new->append_user(this);
			return true;
		}
		// nothing to replace
		return false;
	}

	void append_dep(Instruction* I) {
		dep_list.push_back(I);
	}

public:
	explicit Instruction(InstID id, Type::TypeID type) : id(id), type(type) {}

	virtual ~Instruction() {
		// remove from users
		for( OperandListTy::iterator i = op_list.begin(), e = op_list.end(); i != e ; ++i) {
			(*i)->remove_user(this);
		}
	}

	InstID get_opcode() const { return id; } ///< return the opcode of the instruction (icmd.hpp)

	void set_method(Method* M) { method = M; }
	Method* get_method() const { return method; }

	OperandListTy::const_iterator op_begin() const { return op_list.begin(); }
	OperandListTy::const_iterator op_end()   const { return op_list.end(); }
	size_t op_size() const { return op_list.size(); }

	DepListTy::const_iterator dep_begin() const { return dep_list.begin(); }
	DepListTy::const_iterator dep_end()   const { return dep_list.end(); }
	size_t dep_size() const { return dep_list.size(); }

	/**
	 * Get the corresponding BeginInst.
	 *
	 * BeginInst are used to mark control flow joins (aka the start of a basic block).
	 * @return The directly dominating BeginInst. NULL if there is none (eg. several
	 *         cadidates or dead code).
	 */
	virtual BeginInst* get_BeginInst() const;

	// casting functions
	virtual Instruction*          to_Instruction()          { return this; }

	virtual NOPInst*              to_NOPInst()              { return NULL; }
	virtual POPInst*              to_POPInst()              { return NULL; }
	virtual CHECKNULLInst*        to_CHECKNULLInst()        { return NULL; }
	virtual ARRAYLENGTHInst*      to_ARRAYLENGTHInst()      { return NULL; }
	virtual NEGInst*              to_NEGInst()              { return NULL; }
	virtual CASTInst*             to_CASTInst()             { return NULL; }
	virtual ADDInst*              to_ADDInst()              { return NULL; }
	virtual SUBInst*              to_SUBInst()              { return NULL; }
	virtual MULInst*              to_MULInst()              { return NULL; }
	virtual DIVInst*              to_DIVInst()              { return NULL; }
	virtual REMInst*              to_REMInst()              { return NULL; }
	virtual SHLInst*              to_SHLInst()              { return NULL; }
	virtual USHRInst*             to_USHRInst()             { return NULL; }
	virtual ANDInst*              to_ANDInst()              { return NULL; }
	virtual ORInst*               to_ORInst()               { return NULL; }
	virtual XORInst*              to_XORInst()              { return NULL; }
	virtual CMPInst*              to_CMPInst()              { return NULL; }
	virtual CONSTInst*            to_CONSTInst()            { return NULL; }
	virtual GETFIELDInst*         to_GETFIELDInst()         { return NULL; }
	virtual PUTFIELDInst*         to_PUTFIELDInst()         { return NULL; }
	virtual PUTSTATICInst*        to_PUTSTATICInst()        { return NULL; }
	virtual GETSTATICInst*        to_GETSTATICInst()        { return NULL; }
	virtual INCInst*              to_INCInst()              { return NULL; }
	virtual ASTOREInst*           to_ASTOREInst()           { return NULL; }
	virtual ALOADInst*            to_ALOADInst()            { return NULL; }
	virtual RETInst*              to_RETInst()              { return NULL; }
	virtual LOADInst*             to_LOADInst()             { return NULL; }
	virtual STOREInst*            to_STOREInst()            { return NULL; }
	virtual NEWInst*              to_NEWInst()              { return NULL; }
	virtual NEWARRAYInst*         to_NEWARRAYInst()         { return NULL; }
	virtual ANEWARRAYInst*        to_ANEWARRAYInst()        { return NULL; }
	virtual MULTIANEWARRAYInst*   to_MULTIANEWARRAYInst()   { return NULL; }
	virtual CHECKCASTInst*        to_CHECKCASTInst()        { return NULL; }
	virtual INSTANCEOFInst*       to_INSTANCEOFInst()       { return NULL; }
	virtual GOTOInst*             to_GOTOInst()             { return NULL; }
	virtual JSRInst*              to_JSRInst()              { return NULL; }
	virtual BUILTINInst*          to_BUILTINInst()          { return NULL; }
	virtual INVOKEVIRTUALInst*    to_INVOKEVIRTUALInst()    { return NULL; }
	virtual INVOKESPECIALInst*    to_INVOKESPECIALInst()    { return NULL; }
	virtual INVOKESTATICInst*     to_INVOKESTATICInst()     { return NULL; }
	virtual INVOKEINTERFACEInst*  to_INVOKEINTERFACEInst()  { return NULL; }
	virtual IFInst*               to_IFInst()               { return NULL; }
	virtual IF_CMPInst*           to_IF_CMPInst()           { return NULL; }
	virtual TABLESWITCHInst*      to_TABLESWITCHInst()      { return NULL; }
	virtual LOOKUPSWITCHInst*     to_LOOKUPSWITCHInst()     { return NULL; }
	virtual RETURNInst*           to_RETURNInst()           { return NULL; }
	virtual THROWInst*            to_THROWInst()            { return NULL; }
	virtual COPYInst*             to_COPYInst()             { return NULL; }
	virtual MOVEInst*             to_MOVEInst()             { return NULL; }
	virtual GETEXCEPTIONInst*     to_GETEXCEPTIONInst()     { return NULL; }
	virtual PHIInst*              to_PHIInst()              { return NULL; }

	virtual BeginInst*            to_BeginInst()            { return NULL; }
	virtual EndInst*              to_EndInst()              { return NULL; }

	const char* get_name() const {
		switch (id) {
			case NOPInstID:              return "NOPInst";
			case POPInstID:              return "POPInst";
			case CHECKNULLInstID:        return "CHECKNULLInst";
			case ARRAYLENGTHInstID:      return "ARRAYLENGTHInst";
			case NEGInstID:              return "NEGInst";
			case CASTInstID:             return "CASTInst";
			case ADDInstID:              return "ADDInst";
			case SUBInstID:              return "SUBInst";
			case MULInstID:              return "MULInst";
			case DIVInstID:              return "DIVInst";
			case REMInstID:              return "REMInst";
			case SHLInstID:              return "SHLInst";
			case USHRInstID:             return "USHRInst";
			case ANDInstID:              return "ANDInst";
			case ORInstID:               return "ORInst";
			case XORInstID:              return "XORInst";
			case CMPInstID:              return "CMPInst";
			case CONSTInstID:            return "CONSTInst";
			case GETFIELDInstID:         return "GETFIELDInst";
			case PUTFIELDInstID:         return "PUTFIELDInst";
			case PUTSTATICInstID:        return "PUTSTATICInst";
			case GETSTATICInstID:        return "GETSTATICInst";
			case INCInstID:              return "INCInst";
			case ASTOREInstID:           return "ASTOREInst";
			case ALOADInstID:            return "ALOADInst";
			case RETInstID:              return "RETInst";
			case LOADInstID:             return "LOADInst";
			case STOREInstID:            return "STOREInst";
			case NEWInstID:              return "NEWInst";
			case NEWARRAYInstID:         return "NEWARRAYInst";
			case ANEWARRAYInstID:        return "ANEWARRAYInst";
			case MULTIANEWARRAYInstID:   return "MULTIANEWARRAYInst";
			case CHECKCASTInstID:        return "CHECKCASTInst";
			case INSTANCEOFInstID:       return "INSTANCEOFInst";
			case GOTOInstID:             return "GOTOInst";
			case JSRInstID:              return "JSRInst";
			case BUILTINInstID:          return "BUILTINInst";
			case INVOKEVIRTUALInstID:    return "INVOKEVIRTUALInst";
			case INVOKESPECIALInstID:    return "INVOKESPECIALInst";
			case INVOKESTATICInstID:     return "INVOKESTATICInst";
			case INVOKEINTERFACEInstID:  return "INVOKEINTERFACEInst";
			case IFInstID:               return "IFInst";
			case IF_CMPInstID:           return "IF_CMPInst";
			case TABLESWITCHInstID:      return "TABLESWITCHInst";
			case LOOKUPSWITCHInstID:     return "LOOKUPSWITCHInst";
			case RETURNInstID:           return "RETURNInst";
			case THROWInstID:            return "THROWInst";
			case COPYInstID:             return "COPYInst";
			case MOVEInstID:             return "MOVEInst";
			case GETEXCEPTIONInstID:     return "GETEXCEPTIONInst";
			case PHIInstID:              return "PHIInst";

			case BeginInstID:            return "BeginInst";
			case EndInstID:              return "EndInst";

			case NoInstID:               return "NoInst";
		}
		return "Unknown Instruction";
	}

	// needed to access replace_op in replace_value
	friend class Value;
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
