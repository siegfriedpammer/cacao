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

namespace cacao {
namespace jit {
namespace compiler2 {

// Forward declarations
class BasicBlock;

// Instructions forward defs
class NOPInstruction;
class ACONSTInstruction;
class CHECKNULLInstruction;
class ICONSTInstruction;
class IDIVPOW2Instruction;
class LDIVPOW2Instruction;
class LCONSTInstruction;
class LCMPCONSTInstruction;
class FCONSTInstruction;
class DCONSTInstruction;
class COPYInstruction;
class MOVEInstruction;
class ILOADInstruction;
class LLOADInstruction;
class FLOADInstruction;
class DLOADInstruction;
class ALOADInstruction;
class IADDCONSTInstruction;
class ISUBCONSTInstruction;
class IMULCONSTInstruction;
class IANDCONSTInstruction;
class IORCONSTInstruction;
class IXORCONSTInstruction;
class ISHLCONSTInstruction;
class ISHRCONSTInstruction;
class IUSHRCONSTInstruction;
class IREMPOW2Instruction;
class LADDCONSTInstruction;
class LSUBCONSTInstruction;
class LMULCONSTInstruction;
class LANDCONSTInstruction;
class LORCONSTInstruction;
class LXORCONSTInstruction;
class LSHLCONSTInstruction;
class LSHRCONSTInstruction;
class LUSHRCONSTInstruction;
class LREMPOW2Instruction;
class IALOADInstruction;
class LALOADInstruction;
class FALOADInstruction;
class DALOADInstruction;
class AALOADInstruction;
class BALOADInstruction;
class CALOADInstruction;
class SALOADInstruction;
class ISTOREInstruction;
class LSTOREInstruction;
class FSTOREInstruction;
class DSTOREInstruction;
class ASTOREInstruction;
class IF_LEQInstruction;
class IF_LNEInstruction;
class IF_LLTInstruction;
class IF_LGEInstruction;
class IF_LGTInstruction;
class IF_LLEInstruction;
class IF_LCMPEQInstruction;
class IF_LCMPNEInstruction;
class IF_LCMPLTInstruction;
class IF_LCMPGEInstruction;
class IF_LCMPGTInstruction;
class IF_LCMPLEInstruction;
class IASTOREInstruction;
class LASTOREInstruction;
class FASTOREInstruction;
class DASTOREInstruction;
class AASTOREInstruction;
class BASTOREInstruction;
class CASTOREInstruction;
class SASTOREInstruction;
class POPInstruction;
class POP2Instruction;
class DUPInstruction;
class DUP_X1Instruction;
class DUP_X2Instruction;
class DUP2Instruction;
class DUP2_X1Instruction;
class DUP2_X2Instruction;
class SWAPInstruction;
class IADDInstruction;
class LADDInstruction;
class FADDInstruction;
class DADDInstruction;
class ISUBInstruction;
class LSUBInstruction;
class FSUBInstruction;
class DSUBInstruction;
class IMULInstruction;
class LMULInstruction;
class FMULInstruction;
class DMULInstruction;
class IDIVInstruction;
class LDIVInstruction;
class FDIVInstruction;
class DDIVInstruction;
class IREMInstruction;
class LREMInstruction;
class FREMInstruction;
class DREMInstruction;
class INEGInstruction;
class LNEGInstruction;
class FNEGInstruction;
class DNEGInstruction;
class ISHLInstruction;
class LSHLInstruction;
class ISHRInstruction;
class LSHRInstruction;
class IUSHRInstruction;
class LUSHRInstruction;
class IANDInstruction;
class LANDInstruction;
class IORInstruction;
class LORInstruction;
class IXORInstruction;
class LXORInstruction;
class IINCInstruction;
class I2LInstruction;
class I2FInstruction;
class I2DInstruction;
class L2IInstruction;
class L2FInstruction;
class L2DInstruction;
class F2IInstruction;
class F2LInstruction;
class F2DInstruction;
class D2IInstruction;
class D2LInstruction;
class D2FInstruction;
class INT2BYTEInstruction;
class INT2CHARInstruction;
class INT2SHORTInstruction;
class LCMPInstruction;
class FCMPLInstruction;
class FCMPGInstruction;
class DCMPLInstruction;
class DCMPGInstruction;
class IFEQInstruction;
class IFNEInstruction;
class IFLTInstruction;
class IFGEInstruction;
class IFGTInstruction;
class IFLEInstruction;
class IF_ICMPEQInstruction;
class IF_ICMPNEInstruction;
class IF_ICMPLTInstruction;
class IF_ICMPGEInstruction;
class IF_ICMPGTInstruction;
class IF_ICMPLEInstruction;
class IF_ACMPEQInstruction;
class IF_ACMPNEInstruction;
class GOTOInstruction;
class JSRInstruction;
class RETInstruction;
class TABLESWITCHInstruction;
class LOOKUPSWITCHInstruction;
class IRETURNInstruction;
class LRETURNInstruction;
class FRETURNInstruction;
class DRETURNInstruction;
class ARETURNInstruction;
class RETURNInstruction;
class GETSTATICInstruction;
class PUTSTATICInstruction;
class GETFIELDInstruction;
class PUTFIELDInstruction;
class INVOKEVIRTUALInstruction;
class INVOKESPECIALInstruction;
class INVOKESTATICInstruction;
class INVOKEINTERFACEInstruction;
class NEWInstruction;
class NEWARRAYInstruction;
class ANEWARRAYInstruction;
class ARRAYLENGTHInstruction;
class ATHROWInstruction;
class CHECKCASTInstruction;
class INSTANCEOFInstruction;
class MONITORENTERInstruction;
class MONITOREXITInstruction;
class MULTIANEWARRAYInstruction;
class IFNULLInstruction;
class IFNONNULLInstruction;
class BREAKPOINTInstruction;
class IASTORECONSTInstruction;
class LASTORECONSTInstruction;
class FASTORECONSTInstruction;
class DASTORECONSTInstruction;
class AASTORECONSTInstruction;
class BASTORECONSTInstruction;
class CASTORECONSTInstruction;
class SASTORECONSTInstruction;
class PUTSTATICCONSTInstruction;
class PUTFIELDCONSTInstruction;
class IMULPOW2Instruction;
class LMULPOW2Instruction;
class GETEXCEPTIONInstruction;
class PHIInstruction;
class INLINE_STARTInstruction;
class INLINE_ENDInstruction;
class INLINE_BODYInstruction;
class BUILTINInstruction;

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
private:
	instruction *iptr;                 ///< reference to the 'old' instruction format
	BasicBlock *parent;				   ///< BasicBlock containing the instruction or NULL
public:
	unsigned getOpcode() const;        ///< return the opcode of the instruction (icmd.hpp)
	bool isTerminator() const;         ///< true if the instruction terminates a basic block
	BasicBlock *getParent() const;     ///< get the BasicBlock in which the instruction is contained.
                                       ///< NULL if not attached to any block.

	// casting functions
	virtual Instruction* toInstruction() { return this;}
	virtual NOPInstruction*                 toNOPInstruction()                 { return NULL;}
	virtual ACONSTInstruction*              toACONSTInstruction()              { return NULL;}
	virtual CHECKNULLInstruction*           toCHECKNULLInstruction()           { return NULL;}
	virtual ICONSTInstruction*              toICONSTInstruction()              { return NULL;}
	virtual IDIVPOW2Instruction*            toIDIVPOW2Instruction()            { return NULL;}
	virtual LDIVPOW2Instruction*            toLDIVPOW2Instruction()            { return NULL;}
	virtual LCONSTInstruction*              toLCONSTInstruction()              { return NULL;}
	virtual LCMPCONSTInstruction*           toLCMPCONSTInstruction()           { return NULL;}
	virtual FCONSTInstruction*              toFCONSTInstruction()              { return NULL;}
	virtual DCONSTInstruction*              toDCONSTInstruction()              { return NULL;}
	virtual COPYInstruction*                toCOPYInstruction()                { return NULL;}
	virtual MOVEInstruction*                toMOVEInstruction()                { return NULL;}
	virtual ILOADInstruction*               toILOADInstruction()               { return NULL;}
	virtual LLOADInstruction*               toLLOADInstruction()               { return NULL;}
	virtual FLOADInstruction*               toFLOADInstruction()               { return NULL;}
	virtual DLOADInstruction*               toDLOADInstruction()               { return NULL;}
	virtual ALOADInstruction*               toALOADInstruction()               { return NULL;}
	virtual IADDCONSTInstruction*           toIADDCONSTInstruction()           { return NULL;}
	virtual ISUBCONSTInstruction*           toISUBCONSTInstruction()           { return NULL;}
	virtual IMULCONSTInstruction*           toIMULCONSTInstruction()           { return NULL;}
	virtual IANDCONSTInstruction*           toIANDCONSTInstruction()           { return NULL;}
	virtual IORCONSTInstruction*            toIORCONSTInstruction()            { return NULL;}
	virtual IXORCONSTInstruction*           toIXORCONSTInstruction()           { return NULL;}
	virtual ISHLCONSTInstruction*           toISHLCONSTInstruction()           { return NULL;}
	virtual ISHRCONSTInstruction*           toISHRCONSTInstruction()           { return NULL;}
	virtual IUSHRCONSTInstruction*          toIUSHRCONSTInstruction()          { return NULL;}
	virtual IREMPOW2Instruction*            toIREMPOW2Instruction()            { return NULL;}
	virtual LADDCONSTInstruction*           toLADDCONSTInstruction()           { return NULL;}
	virtual LSUBCONSTInstruction*           toLSUBCONSTInstruction()           { return NULL;}
	virtual LMULCONSTInstruction*           toLMULCONSTInstruction()           { return NULL;}
	virtual LANDCONSTInstruction*           toLANDCONSTInstruction()           { return NULL;}
	virtual LORCONSTInstruction*            toLORCONSTInstruction()            { return NULL;}
	virtual LXORCONSTInstruction*           toLXORCONSTInstruction()           { return NULL;}
	virtual LSHLCONSTInstruction*           toLSHLCONSTInstruction()           { return NULL;}
	virtual LSHRCONSTInstruction*           toLSHRCONSTInstruction()           { return NULL;}
	virtual LUSHRCONSTInstruction*          toLUSHRCONSTInstruction()          { return NULL;}
	virtual LREMPOW2Instruction*            toLREMPOW2Instruction()            { return NULL;}
	virtual IALOADInstruction*              toIALOADInstruction()              { return NULL;}
	virtual LALOADInstruction*              toLALOADInstruction()              { return NULL;}
	virtual FALOADInstruction*              toFALOADInstruction()              { return NULL;}
	virtual DALOADInstruction*              toDALOADInstruction()              { return NULL;}
	virtual AALOADInstruction*              toAALOADInstruction()              { return NULL;}
	virtual BALOADInstruction*              toBALOADInstruction()              { return NULL;}
	virtual CALOADInstruction*              toCALOADInstruction()              { return NULL;}
	virtual SALOADInstruction*              toSALOADInstruction()              { return NULL;}
	virtual ISTOREInstruction*              toISTOREInstruction()              { return NULL;}
	virtual LSTOREInstruction*              toLSTOREInstruction()              { return NULL;}
	virtual FSTOREInstruction*              toFSTOREInstruction()              { return NULL;}
	virtual DSTOREInstruction*              toDSTOREInstruction()              { return NULL;}
	virtual ASTOREInstruction*              toASTOREInstruction()              { return NULL;}
	virtual IF_LEQInstruction*              toIF_LEQInstruction()              { return NULL;}
	virtual IF_LNEInstruction*              toIF_LNEInstruction()              { return NULL;}
	virtual IF_LLTInstruction*              toIF_LLTInstruction()              { return NULL;}
	virtual IF_LGEInstruction*              toIF_LGEInstruction()              { return NULL;}
	virtual IF_LGTInstruction*              toIF_LGTInstruction()              { return NULL;}
	virtual IF_LLEInstruction*              toIF_LLEInstruction()              { return NULL;}
	virtual IF_LCMPEQInstruction*           toIF_LCMPEQInstruction()           { return NULL;}
	virtual IF_LCMPNEInstruction*           toIF_LCMPNEInstruction()           { return NULL;}
	virtual IF_LCMPLTInstruction*           toIF_LCMPLTInstruction()           { return NULL;}
	virtual IF_LCMPGEInstruction*           toIF_LCMPGEInstruction()           { return NULL;}
	virtual IF_LCMPGTInstruction*           toIF_LCMPGTInstruction()           { return NULL;}
	virtual IF_LCMPLEInstruction*           toIF_LCMPLEInstruction()           { return NULL;}
	virtual IASTOREInstruction*             toIASTOREInstruction()             { return NULL;}
	virtual LASTOREInstruction*             toLASTOREInstruction()             { return NULL;}
	virtual FASTOREInstruction*             toFASTOREInstruction()             { return NULL;}
	virtual DASTOREInstruction*             toDASTOREInstruction()             { return NULL;}
	virtual AASTOREInstruction*             toAASTOREInstruction()             { return NULL;}
	virtual BASTOREInstruction*             toBASTOREInstruction()             { return NULL;}
	virtual CASTOREInstruction*             toCASTOREInstruction()             { return NULL;}
	virtual SASTOREInstruction*             toSASTOREInstruction()             { return NULL;}
	virtual POPInstruction*                 toPOPInstruction()                 { return NULL;}
	virtual POP2Instruction*                toPOP2Instruction()                { return NULL;}
	virtual DUPInstruction*                 toDUPInstruction()                 { return NULL;}
	virtual DUP_X1Instruction*              toDUP_X1Instruction()              { return NULL;}
	virtual DUP_X2Instruction*              toDUP_X2Instruction()              { return NULL;}
	virtual DUP2Instruction*                toDUP2Instruction()                { return NULL;}
	virtual DUP2_X1Instruction*             toDUP2_X1Instruction()             { return NULL;}
	virtual DUP2_X2Instruction*             toDUP2_X2Instruction()             { return NULL;}
	virtual SWAPInstruction*                toSWAPInstruction()                { return NULL;}
	virtual IADDInstruction*                toIADDInstruction()                { return NULL;}
	virtual LADDInstruction*                toLADDInstruction()                { return NULL;}
	virtual FADDInstruction*                toFADDInstruction()                { return NULL;}
	virtual DADDInstruction*                toDADDInstruction()                { return NULL;}
	virtual ISUBInstruction*                toISUBInstruction()                { return NULL;}
	virtual LSUBInstruction*                toLSUBInstruction()                { return NULL;}
	virtual FSUBInstruction*                toFSUBInstruction()                { return NULL;}
	virtual DSUBInstruction*                toDSUBInstruction()                { return NULL;}
	virtual IMULInstruction*                toIMULInstruction()                { return NULL;}
	virtual LMULInstruction*                toLMULInstruction()                { return NULL;}
	virtual FMULInstruction*                toFMULInstruction()                { return NULL;}
	virtual DMULInstruction*                toDMULInstruction()                { return NULL;}
	virtual IDIVInstruction*                toIDIVInstruction()                { return NULL;}
	virtual LDIVInstruction*                toLDIVInstruction()                { return NULL;}
	virtual FDIVInstruction*                toFDIVInstruction()                { return NULL;}
	virtual DDIVInstruction*                toDDIVInstruction()                { return NULL;}
	virtual IREMInstruction*                toIREMInstruction()                { return NULL;}
	virtual LREMInstruction*                toLREMInstruction()                { return NULL;}
	virtual FREMInstruction*                toFREMInstruction()                { return NULL;}
	virtual DREMInstruction*                toDREMInstruction()                { return NULL;}
	virtual INEGInstruction*                toINEGInstruction()                { return NULL;}
	virtual LNEGInstruction*                toLNEGInstruction()                { return NULL;}
	virtual FNEGInstruction*                toFNEGInstruction()                { return NULL;}
	virtual DNEGInstruction*                toDNEGInstruction()                { return NULL;}
	virtual ISHLInstruction*                toISHLInstruction()                { return NULL;}
	virtual LSHLInstruction*                toLSHLInstruction()                { return NULL;}
	virtual ISHRInstruction*                toISHRInstruction()                { return NULL;}
	virtual LSHRInstruction*                toLSHRInstruction()                { return NULL;}
	virtual IUSHRInstruction*               toIUSHRInstruction()               { return NULL;}
	virtual LUSHRInstruction*               toLUSHRInstruction()               { return NULL;}
	virtual IANDInstruction*                toIANDInstruction()                { return NULL;}
	virtual LANDInstruction*                toLANDInstruction()                { return NULL;}
	virtual IORInstruction*                 toIORInstruction()                 { return NULL;}
	virtual LORInstruction*                 toLORInstruction()                 { return NULL;}
	virtual IXORInstruction*                toIXORInstruction()                { return NULL;}
	virtual LXORInstruction*                toLXORInstruction()                { return NULL;}
	virtual IINCInstruction*                toIINCInstruction()                { return NULL;}
	virtual I2LInstruction*                 toI2LInstruction()                 { return NULL;}
	virtual I2FInstruction*                 toI2FInstruction()                 { return NULL;}
	virtual I2DInstruction*                 toI2DInstruction()                 { return NULL;}
	virtual L2IInstruction*                 toL2IInstruction()                 { return NULL;}
	virtual L2FInstruction*                 toL2FInstruction()                 { return NULL;}
	virtual L2DInstruction*                 toL2DInstruction()                 { return NULL;}
	virtual F2IInstruction*                 toF2IInstruction()                 { return NULL;}
	virtual F2LInstruction*                 toF2LInstruction()                 { return NULL;}
	virtual F2DInstruction*                 toF2DInstruction()                 { return NULL;}
	virtual D2IInstruction*                 toD2IInstruction()                 { return NULL;}
	virtual D2LInstruction*                 toD2LInstruction()                 { return NULL;}
	virtual D2FInstruction*                 toD2FInstruction()                 { return NULL;}
	virtual INT2BYTEInstruction*            toINT2BYTEInstruction()            { return NULL;}
	virtual INT2CHARInstruction*            toINT2CHARInstruction()            { return NULL;}
	virtual INT2SHORTInstruction*           toINT2SHORTInstruction()           { return NULL;}
	virtual LCMPInstruction*                toLCMPInstruction()                { return NULL;}
	virtual FCMPLInstruction*               toFCMPLInstruction()               { return NULL;}
	virtual FCMPGInstruction*               toFCMPGInstruction()               { return NULL;}
	virtual DCMPLInstruction*               toDCMPLInstruction()               { return NULL;}
	virtual DCMPGInstruction*               toDCMPGInstruction()               { return NULL;}
	virtual IFEQInstruction*                toIFEQInstruction()                { return NULL;}
	virtual IFNEInstruction*                toIFNEInstruction()                { return NULL;}
	virtual IFLTInstruction*                toIFLTInstruction()                { return NULL;}
	virtual IFGEInstruction*                toIFGEInstruction()                { return NULL;}
	virtual IFGTInstruction*                toIFGTInstruction()                { return NULL;}
	virtual IFLEInstruction*                toIFLEInstruction()                { return NULL;}
	virtual IF_ICMPEQInstruction*           toIF_ICMPEQInstruction()           { return NULL;}
	virtual IF_ICMPNEInstruction*           toIF_ICMPNEInstruction()           { return NULL;}
	virtual IF_ICMPLTInstruction*           toIF_ICMPLTInstruction()           { return NULL;}
	virtual IF_ICMPGEInstruction*           toIF_ICMPGEInstruction()           { return NULL;}
	virtual IF_ICMPGTInstruction*           toIF_ICMPGTInstruction()           { return NULL;}
	virtual IF_ICMPLEInstruction*           toIF_ICMPLEInstruction()           { return NULL;}
	virtual IF_ACMPEQInstruction*           toIF_ACMPEQInstruction()           { return NULL;}
	virtual IF_ACMPNEInstruction*           toIF_ACMPNEInstruction()           { return NULL;}
	virtual GOTOInstruction*                toGOTOInstruction()                { return NULL;}
	virtual JSRInstruction*                 toJSRInstruction()                 { return NULL;}
	virtual RETInstruction*                 toRETInstruction()                 { return NULL;}
	virtual TABLESWITCHInstruction*         toTABLESWITCHInstruction()         { return NULL;}
	virtual LOOKUPSWITCHInstruction*        toLOOKUPSWITCHInstruction()        { return NULL;}
	virtual IRETURNInstruction*             toIRETURNInstruction()             { return NULL;}
	virtual LRETURNInstruction*             toLRETURNInstruction()             { return NULL;}
	virtual FRETURNInstruction*             toFRETURNInstruction()             { return NULL;}
	virtual DRETURNInstruction*             toDRETURNInstruction()             { return NULL;}
	virtual ARETURNInstruction*             toARETURNInstruction()             { return NULL;}
	virtual RETURNInstruction*              toRETURNInstruction()              { return NULL;}
	virtual GETSTATICInstruction*           toGETSTATICInstruction()           { return NULL;}
	virtual PUTSTATICInstruction*           toPUTSTATICInstruction()           { return NULL;}
	virtual GETFIELDInstruction*            toGETFIELDInstruction()            { return NULL;}
	virtual PUTFIELDInstruction*            toPUTFIELDInstruction()            { return NULL;}
	virtual INVOKEVIRTUALInstruction*       toINVOKEVIRTUALInstruction()       { return NULL;}
	virtual INVOKESPECIALInstruction*       toINVOKESPECIALInstruction()       { return NULL;}
	virtual INVOKESTATICInstruction*        toINVOKESTATICInstruction()        { return NULL;}
	virtual INVOKEINTERFACEInstruction*     toINVOKEINTERFACEInstruction()     { return NULL;}
	virtual NEWInstruction*                 toNEWInstruction()                 { return NULL;}
	virtual NEWARRAYInstruction*            toNEWARRAYInstruction()            { return NULL;}
	virtual ANEWARRAYInstruction*           toANEWARRAYInstruction()           { return NULL;}
	virtual ARRAYLENGTHInstruction*         toARRAYLENGTHInstruction()         { return NULL;}
	virtual ATHROWInstruction*              toATHROWInstruction()              { return NULL;}
	virtual CHECKCASTInstruction*           toCHECKCASTInstruction()           { return NULL;}
	virtual INSTANCEOFInstruction*          toINSTANCEOFInstruction()          { return NULL;}
	virtual MONITORENTERInstruction*        toMONITORENTERInstruction()        { return NULL;}
	virtual MONITOREXITInstruction*         toMONITOREXITInstruction()         { return NULL;}
	virtual MULTIANEWARRAYInstruction*      toMULTIANEWARRAYInstruction()      { return NULL;}
	virtual IFNULLInstruction*              toIFNULLInstruction()              { return NULL;}
	virtual IFNONNULLInstruction*           toIFNONNULLInstruction()           { return NULL;}
	virtual BREAKPOINTInstruction*          toBREAKPOINTInstruction()          { return NULL;}
	virtual IASTORECONSTInstruction*        toIASTORECONSTInstruction()        { return NULL;}
	virtual LASTORECONSTInstruction*        toLASTORECONSTInstruction()        { return NULL;}
	virtual FASTORECONSTInstruction*        toFASTORECONSTInstruction()        { return NULL;}
	virtual DASTORECONSTInstruction*        toDASTORECONSTInstruction()        { return NULL;}
	virtual AASTORECONSTInstruction*        toAASTORECONSTInstruction()        { return NULL;}
	virtual BASTORECONSTInstruction*        toBASTORECONSTInstruction()        { return NULL;}
	virtual CASTORECONSTInstruction*        toCASTORECONSTInstruction()        { return NULL;}
	virtual SASTORECONSTInstruction*        toSASTORECONSTInstruction()        { return NULL;}
	virtual PUTSTATICCONSTInstruction*      toPUTSTATICCONSTInstruction()      { return NULL;}
	virtual PUTFIELDCONSTInstruction*       toPUTFIELDCONSTInstruction()       { return NULL;}
	virtual IMULPOW2Instruction*            toIMULPOW2Instruction()            { return NULL;}
	virtual LMULPOW2Instruction*            toLMULPOW2Instruction()            { return NULL;}
	virtual GETEXCEPTIONInstruction*        toGETEXCEPTIONInstruction()        { return NULL;}
	virtual PHIInstruction*                 toPHIInstruction()                 { return NULL;}
	virtual INLINE_STARTInstruction*        toINLINE_STARTInstruction()        { return NULL;}
	virtual INLINE_ENDInstruction*          toINLINE_ENDInstruction()          { return NULL;}
	virtual INLINE_BODYInstruction*         toINLINE_BODYInstruction()         { return NULL;}
	virtual BUILTINInstruction*             toBUILTINInstruction()             { return NULL;}

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
