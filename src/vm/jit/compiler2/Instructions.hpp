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
#include "vm/jit/ir/icmd.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

class NOPInstruction : public Instruction {
public:
	NOPInstruction() : Instruction(ICMD_NOP) {}
	virtual NOPInstruction* toNOPInstruction() { return this;}
};

class ACONSTInstruction : public Instruction {
public:
	ACONSTInstruction() : Instruction(ICMD_ACONST) {}
	virtual ACONSTInstruction* toACONSTInstruction() { return this;}

};

class CHECKNULLInstruction : public Instruction {
public:
	CHECKNULLInstruction() : Instruction(ICMD_CHECKNULL) {}
	virtual CHECKNULLInstruction* toCHECKNULLInstruction() { return this;}

};

class ICONSTInstruction : public Instruction {
public:
	ICONSTInstruction() : Instruction(ICMD_ICONST) {}
	virtual ICONSTInstruction* toICONSTInstruction() { return this;}

};

class IDIVPOW2Instruction : public Instruction {
public:
	IDIVPOW2Instruction() : Instruction(ICMD_IDIVPOW2) {}
	virtual IDIVPOW2Instruction* toIDIVPOW2Instruction() { return this;}

};

class LDIVPOW2Instruction : public Instruction {
public:
	LDIVPOW2Instruction() : Instruction(ICMD_LDIVPOW2) {}
	virtual LDIVPOW2Instruction* toLDIVPOW2Instruction() { return this;}

};

class LCONSTInstruction : public Instruction {
public:
	LCONSTInstruction() : Instruction(ICMD_LCONST) {}
	virtual LCONSTInstruction* toLCONSTInstruction() { return this;}

};

class LCMPCONSTInstruction : public Instruction {
public:
	LCMPCONSTInstruction() : Instruction(ICMD_LCMPCONST) {}
	virtual LCMPCONSTInstruction* toLCMPCONSTInstruction() { return this;}

};

class FCONSTInstruction : public Instruction {
public:
	FCONSTInstruction() : Instruction(ICMD_FCONST) {}
	virtual FCONSTInstruction* toFCONSTInstruction() { return this;}

};

class DCONSTInstruction : public Instruction {
public:
	DCONSTInstruction() : Instruction(ICMD_DCONST) {}
	virtual DCONSTInstruction* toDCONSTInstruction() { return this;}

};

class COPYInstruction : public Instruction {
public:
	COPYInstruction() : Instruction(ICMD_COPY) {}
	virtual COPYInstruction* toCOPYInstruction() { return this;}

};

class MOVEInstruction : public Instruction {
public:
	MOVEInstruction() : Instruction(ICMD_MOVE) {}
	virtual MOVEInstruction* toMOVEInstruction() { return this;}

};

class ILOADInstruction : public Instruction {
public:
	ILOADInstruction() : Instruction(ICMD_ILOAD) {}
	virtual ILOADInstruction* toILOADInstruction() { return this;}

};

class LLOADInstruction : public Instruction {
public:
	LLOADInstruction() : Instruction(ICMD_LLOAD) {}
	virtual LLOADInstruction* toLLOADInstruction() { return this;}

};

class FLOADInstruction : public Instruction {
public:
	FLOADInstruction() : Instruction(ICMD_FLOAD) {}
	virtual FLOADInstruction* toFLOADInstruction() { return this;}

};

class DLOADInstruction : public Instruction {
public:
	DLOADInstruction() : Instruction(ICMD_DLOAD) {}
	virtual DLOADInstruction* toDLOADInstruction() { return this;}

};

class ALOADInstruction : public Instruction {
public:
	ALOADInstruction() : Instruction(ICMD_ALOAD) {}
	virtual ALOADInstruction* toALOADInstruction() { return this;}

};

class IADDCONSTInstruction : public Instruction {
public:
	IADDCONSTInstruction() : Instruction(ICMD_IADDCONST) {}
	virtual IADDCONSTInstruction* toIADDCONSTInstruction() { return this;}

};

class ISUBCONSTInstruction : public Instruction {
public:
	ISUBCONSTInstruction() : Instruction(ICMD_ISUBCONST) {}
	virtual ISUBCONSTInstruction* toISUBCONSTInstruction() { return this;}

};

class IMULCONSTInstruction : public Instruction {
public:
	IMULCONSTInstruction() : Instruction(ICMD_IMULCONST) {}
	virtual IMULCONSTInstruction* toIMULCONSTInstruction() { return this;}

};

class IANDCONSTInstruction : public Instruction {
public:
	IANDCONSTInstruction() : Instruction(ICMD_IANDCONST) {}
	virtual IANDCONSTInstruction* toIANDCONSTInstruction() { return this;}

};

class IORCONSTInstruction : public Instruction {
public:
	IORCONSTInstruction() : Instruction(ICMD_IORCONST) {}
	virtual IORCONSTInstruction* toIORCONSTInstruction() { return this;}

};

class IXORCONSTInstruction : public Instruction {
public:
	IXORCONSTInstruction() : Instruction(ICMD_IXORCONST) {}
	virtual IXORCONSTInstruction* toIXORCONSTInstruction() { return this;}

};

class ISHLCONSTInstruction : public Instruction {
public:
	ISHLCONSTInstruction() : Instruction(ICMD_ISHLCONST) {}
	virtual ISHLCONSTInstruction* toISHLCONSTInstruction() { return this;}

};

class ISHRCONSTInstruction : public Instruction {
public:
	ISHRCONSTInstruction() : Instruction(ICMD_ISHRCONST) {}
	virtual ISHRCONSTInstruction* toISHRCONSTInstruction() { return this;}

};

class IUSHRCONSTInstruction : public Instruction {
public:
	IUSHRCONSTInstruction() : Instruction(ICMD_IUSHRCONST) {}
	virtual IUSHRCONSTInstruction* toIUSHRCONSTInstruction() { return this;}

};

class IREMPOW2Instruction : public Instruction {
public:
	IREMPOW2Instruction() : Instruction(ICMD_IREMPOW2) {}
	virtual IREMPOW2Instruction* toIREMPOW2Instruction() { return this;}

};

class LADDCONSTInstruction : public Instruction {
public:
	LADDCONSTInstruction() : Instruction(ICMD_LADDCONST) {}
	virtual LADDCONSTInstruction* toLADDCONSTInstruction() { return this;}

};

class LSUBCONSTInstruction : public Instruction {
public:
	LSUBCONSTInstruction() : Instruction(ICMD_LSUBCONST) {}
	virtual LSUBCONSTInstruction* toLSUBCONSTInstruction() { return this;}

};

class LMULCONSTInstruction : public Instruction {
public:
	LMULCONSTInstruction() : Instruction(ICMD_LMULCONST) {}
	virtual LMULCONSTInstruction* toLMULCONSTInstruction() { return this;}

};

class LANDCONSTInstruction : public Instruction {
public:
	LANDCONSTInstruction() : Instruction(ICMD_LANDCONST) {}
	virtual LANDCONSTInstruction* toLANDCONSTInstruction() { return this;}

};

class LORCONSTInstruction : public Instruction {
public:
	LORCONSTInstruction() : Instruction(ICMD_LORCONST) {}
	virtual LORCONSTInstruction* toLORCONSTInstruction() { return this;}

};

class LXORCONSTInstruction : public Instruction {
public:
	LXORCONSTInstruction() : Instruction(ICMD_LXORCONST) {}
	virtual LXORCONSTInstruction* toLXORCONSTInstruction() { return this;}

};

class LSHLCONSTInstruction : public Instruction {
public:
	LSHLCONSTInstruction() : Instruction(ICMD_LSHLCONST) {}
	virtual LSHLCONSTInstruction* toLSHLCONSTInstruction() { return this;}

};

class LSHRCONSTInstruction : public Instruction {
public:
	LSHRCONSTInstruction() : Instruction(ICMD_LSHRCONST) {}
	virtual LSHRCONSTInstruction* toLSHRCONSTInstruction() { return this;}

};

class LUSHRCONSTInstruction : public Instruction {
public:
	LUSHRCONSTInstruction() : Instruction(ICMD_LUSHRCONST) {}
	virtual LUSHRCONSTInstruction* toLUSHRCONSTInstruction() { return this;}

};

class LREMPOW2Instruction : public Instruction {
public:
	LREMPOW2Instruction() : Instruction(ICMD_LREMPOW2) {}
	virtual LREMPOW2Instruction* toLREMPOW2Instruction() { return this;}

};

class IALOADInstruction : public Instruction {
public:
	IALOADInstruction() : Instruction(ICMD_IALOAD) {}
	virtual IALOADInstruction* toIALOADInstruction() { return this;}

};

class LALOADInstruction : public Instruction {
public:
	LALOADInstruction() : Instruction(ICMD_LALOAD) {}
	virtual LALOADInstruction* toLALOADInstruction() { return this;}

};

class FALOADInstruction : public Instruction {
public:
	FALOADInstruction() : Instruction(ICMD_FALOAD) {}
	virtual FALOADInstruction* toFALOADInstruction() { return this;}

};

class DALOADInstruction : public Instruction {
public:
	DALOADInstruction() : Instruction(ICMD_DALOAD) {}
	virtual DALOADInstruction* toDALOADInstruction() { return this;}

};

class AALOADInstruction : public Instruction {
public:
	AALOADInstruction() : Instruction(ICMD_AALOAD) {}
	virtual AALOADInstruction* toAALOADInstruction() { return this;}

};

class BALOADInstruction : public Instruction {
public:
	BALOADInstruction() : Instruction(ICMD_BALOAD) {}
	virtual BALOADInstruction* toBALOADInstruction() { return this;}

};

class CALOADInstruction : public Instruction {
public:
	CALOADInstruction() : Instruction(ICMD_CALOAD) {}
	virtual CALOADInstruction* toCALOADInstruction() { return this;}

};

class SALOADInstruction : public Instruction {
public:
	SALOADInstruction() : Instruction(ICMD_SALOAD) {}
	virtual SALOADInstruction* toSALOADInstruction() { return this;}

};

class ISTOREInstruction : public Instruction {
public:
	ISTOREInstruction() : Instruction(ICMD_ISTORE) {}
	virtual ISTOREInstruction* toISTOREInstruction() { return this;}

};

class LSTOREInstruction : public Instruction {
public:
	LSTOREInstruction() : Instruction(ICMD_LSTORE) {}
	virtual LSTOREInstruction* toLSTOREInstruction() { return this;}

};

class FSTOREInstruction : public Instruction {
public:
	FSTOREInstruction() : Instruction(ICMD_FSTORE) {}
	virtual FSTOREInstruction* toFSTOREInstruction() { return this;}

};

class DSTOREInstruction : public Instruction {
public:
	DSTOREInstruction() : Instruction(ICMD_DSTORE) {}
	virtual DSTOREInstruction* toDSTOREInstruction() { return this;}

};

class ASTOREInstruction : public Instruction {
public:
	ASTOREInstruction() : Instruction(ICMD_ASTORE) {}
	virtual ASTOREInstruction* toASTOREInstruction() { return this;}

};

class IF_LEQInstruction : public Instruction {
public:
	IF_LEQInstruction() : Instruction(ICMD_IF_LEQ) {}
	virtual IF_LEQInstruction* toIF_LEQInstruction() { return this;}

};

class IF_LNEInstruction : public Instruction {
public:
	IF_LNEInstruction() : Instruction(ICMD_IF_LNE) {}
	virtual IF_LNEInstruction* toIF_LNEInstruction() { return this;}

};

class IF_LLTInstruction : public Instruction {
public:
	IF_LLTInstruction() : Instruction(ICMD_IF_LLT) {}
	virtual IF_LLTInstruction* toIF_LLTInstruction() { return this;}

};

class IF_LGEInstruction : public Instruction {
public:
	IF_LGEInstruction() : Instruction(ICMD_IF_LGE) {}
	virtual IF_LGEInstruction* toIF_LGEInstruction() { return this;}

};

class IF_LGTInstruction : public Instruction {
public:
	IF_LGTInstruction() : Instruction(ICMD_IF_LGT) {}
	virtual IF_LGTInstruction* toIF_LGTInstruction() { return this;}

};

class IF_LLEInstruction : public Instruction {
public:
	IF_LLEInstruction() : Instruction(ICMD_IF_LLE) {}
	virtual IF_LLEInstruction* toIF_LLEInstruction() { return this;}

};

class IF_LCMPEQInstruction : public Instruction {
public:
	IF_LCMPEQInstruction() : Instruction(ICMD_IF_LCMPEQ) {}
	virtual IF_LCMPEQInstruction* toIF_LCMPEQInstruction() { return this;}

};

class IF_LCMPNEInstruction : public Instruction {
public:
	IF_LCMPNEInstruction() : Instruction(ICMD_IF_LCMPNE) {}
	virtual IF_LCMPNEInstruction* toIF_LCMPNEInstruction() { return this;}

};

class IF_LCMPLTInstruction : public Instruction {
public:
	IF_LCMPLTInstruction() : Instruction(ICMD_IF_LCMPLT) {}
	virtual IF_LCMPLTInstruction* toIF_LCMPLTInstruction() { return this;}

};

class IF_LCMPGEInstruction : public Instruction {
public:
	IF_LCMPGEInstruction() : Instruction(ICMD_IF_LCMPGE) {}
	virtual IF_LCMPGEInstruction* toIF_LCMPGEInstruction() { return this;}

};

class IF_LCMPGTInstruction : public Instruction {
public:
	IF_LCMPGTInstruction() : Instruction(ICMD_IF_LCMPGT) {}
	virtual IF_LCMPGTInstruction* toIF_LCMPGTInstruction() { return this;}

};

class IF_LCMPLEInstruction : public Instruction {
public:
	IF_LCMPLEInstruction() : Instruction(ICMD_IF_LCMPLE) {}
	virtual IF_LCMPLEInstruction* toIF_LCMPLEInstruction() { return this;}

};

class IASTOREInstruction : public Instruction {
public:
	IASTOREInstruction() : Instruction(ICMD_IASTORE) {}
	virtual IASTOREInstruction* toIASTOREInstruction() { return this;}

};

class LASTOREInstruction : public Instruction {
public:
	LASTOREInstruction() : Instruction(ICMD_LASTORE) {}
	virtual LASTOREInstruction* toLASTOREInstruction() { return this;}

};

class FASTOREInstruction : public Instruction {
public:
	FASTOREInstruction() : Instruction(ICMD_FASTORE) {}
	virtual FASTOREInstruction* toFASTOREInstruction() { return this;}

};

class DASTOREInstruction : public Instruction {
public:
	DASTOREInstruction() : Instruction(ICMD_DASTORE) {}
	virtual DASTOREInstruction* toDASTOREInstruction() { return this;}

};

class AASTOREInstruction : public Instruction {
public:
	AASTOREInstruction() : Instruction(ICMD_AASTORE) {}
	virtual AASTOREInstruction* toAASTOREInstruction() { return this;}

};

class BASTOREInstruction : public Instruction {
public:
	BASTOREInstruction() : Instruction(ICMD_BASTORE) {}
	virtual BASTOREInstruction* toBASTOREInstruction() { return this;}

};

class CASTOREInstruction : public Instruction {
public:
	CASTOREInstruction() : Instruction(ICMD_CASTORE) {}
	virtual CASTOREInstruction* toCASTOREInstruction() { return this;}

};

class SASTOREInstruction : public Instruction {
public:
	SASTOREInstruction() : Instruction(ICMD_SASTORE) {}
	virtual SASTOREInstruction* toSASTOREInstruction() { return this;}

};

class POPInstruction : public Instruction {
public:
	POPInstruction() : Instruction(ICMD_POP) {}
	virtual POPInstruction* toPOPInstruction() { return this;}

};

class POP2Instruction : public Instruction {
public:
	POP2Instruction() : Instruction(ICMD_POP2) {}
	virtual POP2Instruction* toPOP2Instruction() { return this;}

};

class DUPInstruction : public Instruction {
public:
	DUPInstruction() : Instruction(ICMD_DUP) {}
	virtual DUPInstruction* toDUPInstruction() { return this;}

};

class DUP_X1Instruction : public Instruction {
public:
	DUP_X1Instruction() : Instruction(ICMD_DUP_X1) {}
	virtual DUP_X1Instruction* toDUP_X1Instruction() { return this;}

};

class DUP_X2Instruction : public Instruction {
public:
	DUP_X2Instruction() : Instruction(ICMD_DUP_X2) {}
	virtual DUP_X2Instruction* toDUP_X2Instruction() { return this;}

};

class DUP2Instruction : public Instruction {
public:
	DUP2Instruction() : Instruction(ICMD_DUP2) {}
	virtual DUP2Instruction* toDUP2Instruction() { return this;}

};

class DUP2_X1Instruction : public Instruction {
public:
	DUP2_X1Instruction() : Instruction(ICMD_DUP2_X1) {}
	virtual DUP2_X1Instruction* toDUP2_X1Instruction() { return this;}

};

class DUP2_X2Instruction : public Instruction {
public:
	DUP2_X2Instruction() : Instruction(ICMD_DUP2_X2) {}
	virtual DUP2_X2Instruction* toDUP2_X2Instruction() { return this;}

};

class SWAPInstruction : public Instruction {
public:
	SWAPInstruction() : Instruction(ICMD_SWAP) {}
	virtual SWAPInstruction* toSWAPInstruction() { return this;}

};

class IADDInstruction : public Instruction {
public:
	IADDInstruction() : Instruction(ICMD_IADD) {}
	virtual IADDInstruction* toIADDInstruction() { return this;}

};

class LADDInstruction : public Instruction {
public:
	LADDInstruction() : Instruction(ICMD_LADD) {}
	virtual LADDInstruction* toLADDInstruction() { return this;}

};

class FADDInstruction : public Instruction {
public:
	FADDInstruction() : Instruction(ICMD_FADD) {}
	virtual FADDInstruction* toFADDInstruction() { return this;}

};

class DADDInstruction : public Instruction {
public:
	DADDInstruction() : Instruction(ICMD_DADD) {}
	virtual DADDInstruction* toDADDInstruction() { return this;}

};

class ISUBInstruction : public Instruction {
public:
	ISUBInstruction() : Instruction(ICMD_ISUB) {}
	virtual ISUBInstruction* toISUBInstruction() { return this;}

};

class LSUBInstruction : public Instruction {
public:
	LSUBInstruction() : Instruction(ICMD_LSUB) {}
	virtual LSUBInstruction* toLSUBInstruction() { return this;}

};

class FSUBInstruction : public Instruction {
public:
	FSUBInstruction() : Instruction(ICMD_FSUB) {}
	virtual FSUBInstruction* toFSUBInstruction() { return this;}

};

class DSUBInstruction : public Instruction {
public:
	DSUBInstruction() : Instruction(ICMD_DSUB) {}
	virtual DSUBInstruction* toDSUBInstruction() { return this;}

};

class IMULInstruction : public Instruction {
public:
	IMULInstruction() : Instruction(ICMD_IMUL) {}
	virtual IMULInstruction* toIMULInstruction() { return this;}

};

class LMULInstruction : public Instruction {
public:
	LMULInstruction() : Instruction(ICMD_LMUL) {}
	virtual LMULInstruction* toLMULInstruction() { return this;}

};

class FMULInstruction : public Instruction {
public:
	FMULInstruction() : Instruction(ICMD_FMUL) {}
	virtual FMULInstruction* toFMULInstruction() { return this;}

};

class DMULInstruction : public Instruction {
public:
	DMULInstruction() : Instruction(ICMD_DMUL) {}
	virtual DMULInstruction* toDMULInstruction() { return this;}

};

class IDIVInstruction : public Instruction {
public:
	IDIVInstruction() : Instruction(ICMD_IDIV) {}
	virtual IDIVInstruction* toIDIVInstruction() { return this;}

};

class LDIVInstruction : public Instruction {
public:
	LDIVInstruction() : Instruction(ICMD_LDIV) {}
	virtual LDIVInstruction* toLDIVInstruction() { return this;}

};

class FDIVInstruction : public Instruction {
public:
	FDIVInstruction() : Instruction(ICMD_FDIV) {}
	virtual FDIVInstruction* toFDIVInstruction() { return this;}

};

class DDIVInstruction : public Instruction {
public:
	DDIVInstruction() : Instruction(ICMD_DDIV) {}
	virtual DDIVInstruction* toDDIVInstruction() { return this;}

};

class IREMInstruction : public Instruction {
public:
	IREMInstruction() : Instruction(ICMD_IREM) {}
	virtual IREMInstruction* toIREMInstruction() { return this;}

};

class LREMInstruction : public Instruction {
public:
	LREMInstruction() : Instruction(ICMD_LREM) {}
	virtual LREMInstruction* toLREMInstruction() { return this;}

};

class FREMInstruction : public Instruction {
public:
	FREMInstruction() : Instruction(ICMD_FREM) {}
	virtual FREMInstruction* toFREMInstruction() { return this;}

};

class DREMInstruction : public Instruction {
public:
	DREMInstruction() : Instruction(ICMD_DREM) {}
	virtual DREMInstruction* toDREMInstruction() { return this;}

};

class INEGInstruction : public Instruction {
public:
	INEGInstruction() : Instruction(ICMD_INEG) {}
	virtual INEGInstruction* toINEGInstruction() { return this;}

};

class LNEGInstruction : public Instruction {
public:
	LNEGInstruction() : Instruction(ICMD_LNEG) {}
	virtual LNEGInstruction* toLNEGInstruction() { return this;}

};

class FNEGInstruction : public Instruction {
public:
	FNEGInstruction() : Instruction(ICMD_FNEG) {}
	virtual FNEGInstruction* toFNEGInstruction() { return this;}

};

class DNEGInstruction : public Instruction {
public:
	DNEGInstruction() : Instruction(ICMD_DNEG) {}
	virtual DNEGInstruction* toDNEGInstruction() { return this;}

};

class ISHLInstruction : public Instruction {
public:
	ISHLInstruction() : Instruction(ICMD_ISHL) {}
	virtual ISHLInstruction* toISHLInstruction() { return this;}

};

class LSHLInstruction : public Instruction {
public:
	LSHLInstruction() : Instruction(ICMD_LSHL) {}
	virtual LSHLInstruction* toLSHLInstruction() { return this;}

};

class ISHRInstruction : public Instruction {
public:
	ISHRInstruction() : Instruction(ICMD_ISHR) {}
	virtual ISHRInstruction* toISHRInstruction() { return this;}

};

class LSHRInstruction : public Instruction {
public:
	LSHRInstruction() : Instruction(ICMD_LSHR) {}
	virtual LSHRInstruction* toLSHRInstruction() { return this;}

};

class IUSHRInstruction : public Instruction {
public:
	IUSHRInstruction() : Instruction(ICMD_IUSHR) {}
	virtual IUSHRInstruction* toIUSHRInstruction() { return this;}

};

class LUSHRInstruction : public Instruction {
public:
	LUSHRInstruction() : Instruction(ICMD_LUSHR) {}
	virtual LUSHRInstruction* toLUSHRInstruction() { return this;}

};

class IANDInstruction : public Instruction {
public:
	IANDInstruction() : Instruction(ICMD_IAND) {}
	virtual IANDInstruction* toIANDInstruction() { return this;}

};

class LANDInstruction : public Instruction {
public:
	LANDInstruction() : Instruction(ICMD_LAND) {}
	virtual LANDInstruction* toLANDInstruction() { return this;}

};

class IORInstruction : public Instruction {
public:
	IORInstruction() : Instruction(ICMD_IOR) {}
	virtual IORInstruction* toIORInstruction() { return this;}

};

class LORInstruction : public Instruction {
public:
	LORInstruction() : Instruction(ICMD_LOR) {}
	virtual LORInstruction* toLORInstruction() { return this;}

};

class IXORInstruction : public Instruction {
public:
	IXORInstruction() : Instruction(ICMD_IXOR) {}
	virtual IXORInstruction* toIXORInstruction() { return this;}

};

class LXORInstruction : public Instruction {
public:
	LXORInstruction() : Instruction(ICMD_LXOR) {}
	virtual LXORInstruction* toLXORInstruction() { return this;}

};

class IINCInstruction : public Instruction {
public:
	IINCInstruction() : Instruction(ICMD_IINC) {}
	virtual IINCInstruction* toIINCInstruction() { return this;}

};

class I2LInstruction : public Instruction {
public:
	I2LInstruction() : Instruction(ICMD_I2L) {}
	virtual I2LInstruction* toI2LInstruction() { return this;}

};

class I2FInstruction : public Instruction {
public:
	I2FInstruction() : Instruction(ICMD_I2F) {}
	virtual I2FInstruction* toI2FInstruction() { return this;}

};

class I2DInstruction : public Instruction {
public:
	I2DInstruction() : Instruction(ICMD_I2D) {}
	virtual I2DInstruction* toI2DInstruction() { return this;}

};

class L2IInstruction : public Instruction {
public:
	L2IInstruction() : Instruction(ICMD_L2I) {}
	virtual L2IInstruction* toL2IInstruction() { return this;}

};

class L2FInstruction : public Instruction {
public:
	L2FInstruction() : Instruction(ICMD_L2F) {}
	virtual L2FInstruction* toL2FInstruction() { return this;}

};

class L2DInstruction : public Instruction {
public:
	L2DInstruction() : Instruction(ICMD_L2D) {}
	virtual L2DInstruction* toL2DInstruction() { return this;}

};

class F2IInstruction : public Instruction {
public:
	F2IInstruction() : Instruction(ICMD_F2I) {}
	virtual F2IInstruction* toF2IInstruction() { return this;}

};

class F2LInstruction : public Instruction {
public:
	F2LInstruction() : Instruction(ICMD_F2L) {}
	virtual F2LInstruction* toF2LInstruction() { return this;}

};

class F2DInstruction : public Instruction {
public:
	F2DInstruction() : Instruction(ICMD_F2D) {}
	virtual F2DInstruction* toF2DInstruction() { return this;}

};

class D2IInstruction : public Instruction {
public:
	D2IInstruction() : Instruction(ICMD_D2I) {}
	virtual D2IInstruction* toD2IInstruction() { return this;}

};

class D2LInstruction : public Instruction {
public:
	D2LInstruction() : Instruction(ICMD_D2L) {}
	virtual D2LInstruction* toD2LInstruction() { return this;}

};

class D2FInstruction : public Instruction {
public:
	D2FInstruction() : Instruction(ICMD_D2F) {}
	virtual D2FInstruction* toD2FInstruction() { return this;}

};

class INT2BYTEInstruction : public Instruction {
public:
	INT2BYTEInstruction() : Instruction(ICMD_INT2BYTE) {}
	virtual INT2BYTEInstruction* toINT2BYTEInstruction() { return this;}

};

class INT2CHARInstruction : public Instruction {
public:
	INT2CHARInstruction() : Instruction(ICMD_INT2CHAR) {}
	virtual INT2CHARInstruction* toINT2CHARInstruction() { return this;}

};

class INT2SHORTInstruction : public Instruction {
public:
	INT2SHORTInstruction() : Instruction(ICMD_INT2SHORT) {}
	virtual INT2SHORTInstruction* toINT2SHORTInstruction() { return this;}

};

class LCMPInstruction : public Instruction {
public:
	LCMPInstruction() : Instruction(ICMD_LCMP) {}
	virtual LCMPInstruction* toLCMPInstruction() { return this;}

};

class FCMPLInstruction : public Instruction {
public:
	FCMPLInstruction() : Instruction(ICMD_FCMPL) {}
	virtual FCMPLInstruction* toFCMPLInstruction() { return this;}

};

class FCMPGInstruction : public Instruction {
public:
	FCMPGInstruction() : Instruction(ICMD_FCMPG) {}
	virtual FCMPGInstruction* toFCMPGInstruction() { return this;}

};

class DCMPLInstruction : public Instruction {
public:
	DCMPLInstruction() : Instruction(ICMD_DCMPL) {}
	virtual DCMPLInstruction* toDCMPLInstruction() { return this;}

};

class DCMPGInstruction : public Instruction {
public:
	DCMPGInstruction() : Instruction(ICMD_DCMPG) {}
	virtual DCMPGInstruction* toDCMPGInstruction() { return this;}

};

class IFEQInstruction : public Instruction {
public:
	IFEQInstruction() : Instruction(ICMD_IFEQ) {}
	virtual IFEQInstruction* toIFEQInstruction() { return this;}

};

class IFNEInstruction : public Instruction {
public:
	IFNEInstruction() : Instruction(ICMD_IFNE) {}
	virtual IFNEInstruction* toIFNEInstruction() { return this;}

};

class IFLTInstruction : public Instruction {
public:
	IFLTInstruction() : Instruction(ICMD_IFLT) {}
	virtual IFLTInstruction* toIFLTInstruction() { return this;}

};

class IFGEInstruction : public Instruction {
public:
	IFGEInstruction() : Instruction(ICMD_IFGE) {}
	virtual IFGEInstruction* toIFGEInstruction() { return this;}

};

class IFGTInstruction : public Instruction {
public:
	IFGTInstruction() : Instruction(ICMD_IFGT) {}
	virtual IFGTInstruction* toIFGTInstruction() { return this;}

};

class IFLEInstruction : public Instruction {
public:
	IFLEInstruction() : Instruction(ICMD_IFLE) {}
	virtual IFLEInstruction* toIFLEInstruction() { return this;}

};

class IF_ICMPEQInstruction : public Instruction {
public:
	IF_ICMPEQInstruction() : Instruction(ICMD_IF_ICMPEQ) {}
	virtual IF_ICMPEQInstruction* toIF_ICMPEQInstruction() { return this;}

};

class IF_ICMPNEInstruction : public Instruction {
public:
	IF_ICMPNEInstruction() : Instruction(ICMD_IF_ICMPNE) {}
	virtual IF_ICMPNEInstruction* toIF_ICMPNEInstruction() { return this;}

};

class IF_ICMPLTInstruction : public Instruction {
public:
	IF_ICMPLTInstruction() : Instruction(ICMD_IF_ICMPLT) {}
	virtual IF_ICMPLTInstruction* toIF_ICMPLTInstruction() { return this;}

};

class IF_ICMPGEInstruction : public Instruction {
public:
	IF_ICMPGEInstruction() : Instruction(ICMD_IF_ICMPGE) {}
	virtual IF_ICMPGEInstruction* toIF_ICMPGEInstruction() { return this;}

};

class IF_ICMPGTInstruction : public Instruction {
public:
	IF_ICMPGTInstruction() : Instruction(ICMD_IF_ICMPGT) {}
	virtual IF_ICMPGTInstruction* toIF_ICMPGTInstruction() { return this;}

};

class IF_ICMPLEInstruction : public Instruction {
public:
	IF_ICMPLEInstruction() : Instruction(ICMD_IF_ICMPLE) {}
	virtual IF_ICMPLEInstruction* toIF_ICMPLEInstruction() { return this;}

};

class IF_ACMPEQInstruction : public Instruction {
public:
	IF_ACMPEQInstruction() : Instruction(ICMD_IF_ACMPEQ) {}
	virtual IF_ACMPEQInstruction* toIF_ACMPEQInstruction() { return this;}

};

class IF_ACMPNEInstruction : public Instruction {
public:
	IF_ACMPNEInstruction() : Instruction(ICMD_IF_ACMPNE) {}
	virtual IF_ACMPNEInstruction* toIF_ACMPNEInstruction() { return this;}

};

class GOTOInstruction : public Instruction {
public:
	GOTOInstruction() : Instruction(ICMD_GOTO) {}
	virtual GOTOInstruction* toGOTOInstruction() { return this;}

};

class JSRInstruction : public Instruction {
public:
	JSRInstruction() : Instruction(ICMD_JSR) {}
	virtual JSRInstruction* toJSRInstruction() { return this;}

};

class RETInstruction : public Instruction {
public:
	RETInstruction() : Instruction(ICMD_RET) {}
	virtual RETInstruction* toRETInstruction() { return this;}

};

class TABLESWITCHInstruction : public Instruction {
public:
	TABLESWITCHInstruction() : Instruction(ICMD_TABLESWITCH) {}
	virtual TABLESWITCHInstruction* toTABLESWITCHInstruction() { return this;}

};

class LOOKUPSWITCHInstruction : public Instruction {
public:
	LOOKUPSWITCHInstruction() : Instruction(ICMD_LOOKUPSWITCH) {}
	virtual LOOKUPSWITCHInstruction* toLOOKUPSWITCHInstruction() { return this;}

};

class IRETURNInstruction : public Instruction {
public:
	IRETURNInstruction() : Instruction(ICMD_IRETURN) {}
	virtual IRETURNInstruction* toIRETURNInstruction() { return this;}

};

class LRETURNInstruction : public Instruction {
public:
	LRETURNInstruction() : Instruction(ICMD_LRETURN) {}
	virtual LRETURNInstruction* toLRETURNInstruction() { return this;}

};

class FRETURNInstruction : public Instruction {
public:
	FRETURNInstruction() : Instruction(ICMD_FRETURN) {}
	virtual FRETURNInstruction* toFRETURNInstruction() { return this;}

};

class DRETURNInstruction : public Instruction {
public:
	DRETURNInstruction() : Instruction(ICMD_DRETURN) {}
	virtual DRETURNInstruction* toDRETURNInstruction() { return this;}

};

class ARETURNInstruction : public Instruction {
public:
	ARETURNInstruction() : Instruction(ICMD_ARETURN) {}
	virtual ARETURNInstruction* toARETURNInstruction() { return this;}

};

class RETURNInstruction : public Instruction {
public:
	RETURNInstruction() : Instruction(ICMD_RETURN) {}
	virtual RETURNInstruction* toRETURNInstruction() { return this;}

};

class GETSTATICInstruction : public Instruction {
public:
	GETSTATICInstruction() : Instruction(ICMD_GETSTATIC) {}
	virtual GETSTATICInstruction* toGETSTATICInstruction() { return this;}

};

class PUTSTATICInstruction : public Instruction {
public:
	PUTSTATICInstruction() : Instruction(ICMD_PUTSTATIC) {}
	virtual PUTSTATICInstruction* toPUTSTATICInstruction() { return this;}

};

class GETFIELDInstruction : public Instruction {
public:
	GETFIELDInstruction() : Instruction(ICMD_GETFIELD) {}
	virtual GETFIELDInstruction* toGETFIELDInstruction() { return this;}

};

class PUTFIELDInstruction : public Instruction {
public:
	PUTFIELDInstruction() : Instruction(ICMD_PUTFIELD) {}
	virtual PUTFIELDInstruction* toPUTFIELDInstruction() { return this;}

};

class INVOKEVIRTUALInstruction : public Instruction {
public:
	INVOKEVIRTUALInstruction() : Instruction(ICMD_INVOKEVIRTUAL) {}
	virtual INVOKEVIRTUALInstruction* toINVOKEVIRTUALInstruction() { return this;}

};

class INVOKESPECIALInstruction : public Instruction {
public:
	INVOKESPECIALInstruction() : Instruction(ICMD_INVOKESPECIAL) {}
	virtual INVOKESPECIALInstruction* toINVOKESPECIALInstruction() { return this;}

};

class INVOKESTATICInstruction : public Instruction {
public:
	INVOKESTATICInstruction() : Instruction(ICMD_INVOKESTATIC) {}
	virtual INVOKESTATICInstruction* toINVOKESTATICInstruction() { return this;}

};

class INVOKEINTERFACEInstruction : public Instruction {
public:
	INVOKEINTERFACEInstruction() : Instruction(ICMD_INVOKEINTERFACE) {}
	virtual INVOKEINTERFACEInstruction* toINVOKEINTERFACEInstruction() { return this;}

};

class NEWInstruction : public Instruction {
public:
	NEWInstruction() : Instruction(ICMD_NEW) {}
	virtual NEWInstruction* toNEWInstruction() { return this;}

};

class NEWARRAYInstruction : public Instruction {
public:
	NEWARRAYInstruction() : Instruction(ICMD_NEWARRAY) {}
	virtual NEWARRAYInstruction* toNEWARRAYInstruction() { return this;}

};

class ANEWARRAYInstruction : public Instruction {
public:
	ANEWARRAYInstruction() : Instruction(ICMD_ANEWARRAY) {}
	virtual ANEWARRAYInstruction* toANEWARRAYInstruction() { return this;}

};

class ARRAYLENGTHInstruction : public Instruction {
public:
	ARRAYLENGTHInstruction() : Instruction(ICMD_ARRAYLENGTH) {}
	virtual ARRAYLENGTHInstruction* toARRAYLENGTHInstruction() { return this;}

};

class ATHROWInstruction : public Instruction {
public:
	ATHROWInstruction() : Instruction(ICMD_ATHROW) {}
	virtual ATHROWInstruction* toATHROWInstruction() { return this;}

};

class CHECKCASTInstruction : public Instruction {
public:
	CHECKCASTInstruction() : Instruction(ICMD_CHECKCAST) {}
	virtual CHECKCASTInstruction* toCHECKCASTInstruction() { return this;}

};

class INSTANCEOFInstruction : public Instruction {
public:
	INSTANCEOFInstruction() : Instruction(ICMD_INSTANCEOF) {}
	virtual INSTANCEOFInstruction* toINSTANCEOFInstruction() { return this;}

};

class MONITORENTERInstruction : public Instruction {
public:
	MONITORENTERInstruction() : Instruction(ICMD_MONITORENTER) {}
	virtual MONITORENTERInstruction* toMONITORENTERInstruction() { return this;}

};

class MONITOREXITInstruction : public Instruction {
public:
	MONITOREXITInstruction() : Instruction(ICMD_MONITOREXIT) {}
	virtual MONITOREXITInstruction* toMONITOREXITInstruction() { return this;}

};

class MULTIANEWARRAYInstruction : public Instruction {
public:
	MULTIANEWARRAYInstruction() : Instruction(ICMD_MULTIANEWARRAY) {}
	virtual MULTIANEWARRAYInstruction* toMULTIANEWARRAYInstruction() { return this;}

};

class IFNULLInstruction : public Instruction {
public:
	IFNULLInstruction() : Instruction(ICMD_IFNULL) {}
	virtual IFNULLInstruction* toIFNULLInstruction() { return this;}

};

class IFNONNULLInstruction : public Instruction {
public:
	IFNONNULLInstruction() : Instruction(ICMD_IFNONNULL) {}
	virtual IFNONNULLInstruction* toIFNONNULLInstruction() { return this;}

};

class BREAKPOINTInstruction : public Instruction {
public:
	BREAKPOINTInstruction() : Instruction(ICMD_BREAKPOINT) {}
	virtual BREAKPOINTInstruction* toBREAKPOINTInstruction() { return this;}

};

class IASTORECONSTInstruction : public Instruction {
public:
	IASTORECONSTInstruction() : Instruction(ICMD_IASTORECONST) {}
	virtual IASTORECONSTInstruction* toIASTORECONSTInstruction() { return this;}

};

class LASTORECONSTInstruction : public Instruction {
public:
	LASTORECONSTInstruction() : Instruction(ICMD_LASTORECONST) {}
	virtual LASTORECONSTInstruction* toLASTORECONSTInstruction() { return this;}

};

class FASTORECONSTInstruction : public Instruction {
public:
	FASTORECONSTInstruction() : Instruction(ICMD_FASTORECONST) {}
	virtual FASTORECONSTInstruction* toFASTORECONSTInstruction() { return this;}

};

class DASTORECONSTInstruction : public Instruction {
public:
	DASTORECONSTInstruction() : Instruction(ICMD_DASTORECONST) {}
	virtual DASTORECONSTInstruction* toDASTORECONSTInstruction() { return this;}

};

class AASTORECONSTInstruction : public Instruction {
public:
	AASTORECONSTInstruction() : Instruction(ICMD_AASTORECONST) {}
	virtual AASTORECONSTInstruction* toAASTORECONSTInstruction() { return this;}

};

class BASTORECONSTInstruction : public Instruction {
public:
	BASTORECONSTInstruction() : Instruction(ICMD_BASTORECONST) {}
	virtual BASTORECONSTInstruction* toBASTORECONSTInstruction() { return this;}

};

class CASTORECONSTInstruction : public Instruction {
public:
	CASTORECONSTInstruction() : Instruction(ICMD_CASTORECONST) {}
	virtual CASTORECONSTInstruction* toCASTORECONSTInstruction() { return this;}

};

class SASTORECONSTInstruction : public Instruction {
public:
	SASTORECONSTInstruction() : Instruction(ICMD_SASTORECONST) {}
	virtual SASTORECONSTInstruction* toSASTORECONSTInstruction() { return this;}

};

class PUTSTATICCONSTInstruction : public Instruction {
public:
	PUTSTATICCONSTInstruction() : Instruction(ICMD_PUTSTATICCONST) {}
	virtual PUTSTATICCONSTInstruction* toPUTSTATICCONSTInstruction() { return this;}

};

class PUTFIELDCONSTInstruction : public Instruction {
public:
	PUTFIELDCONSTInstruction() : Instruction(ICMD_PUTFIELDCONST) {}
	virtual PUTFIELDCONSTInstruction* toPUTFIELDCONSTInstruction() { return this;}

};

class IMULPOW2Instruction : public Instruction {
public:
	IMULPOW2Instruction() : Instruction(ICMD_IMULPOW2) {}
	virtual IMULPOW2Instruction* toIMULPOW2Instruction() { return this;}

};

class LMULPOW2Instruction : public Instruction {
public:
	LMULPOW2Instruction() : Instruction(ICMD_LMULPOW2) {}
	virtual LMULPOW2Instruction* toLMULPOW2Instruction() { return this;}

};

class GETEXCEPTIONInstruction : public Instruction {
public:
	GETEXCEPTIONInstruction() : Instruction(ICMD_GETEXCEPTION) {}
	virtual GETEXCEPTIONInstruction* toGETEXCEPTIONInstruction() { return this;}

};

class PHIInstruction : public Instruction {
public:
	PHIInstruction() : Instruction(ICMD_PHI) {}
	virtual PHIInstruction* toPHIInstruction() { return this;}

};

class INLINE_STARTInstruction : public Instruction {
public:
	INLINE_STARTInstruction() : Instruction(ICMD_INLINE_START) {}
	virtual INLINE_STARTInstruction* toINLINE_STARTInstruction() { return this;}

};

class INLINE_ENDInstruction : public Instruction {
public:
	INLINE_ENDInstruction() : Instruction(ICMD_INLINE_END) {}
	virtual INLINE_ENDInstruction* toINLINE_ENDInstruction() { return this;}

};

class INLINE_BODYInstruction : public Instruction {
public:
	INLINE_BODYInstruction() : Instruction(ICMD_INLINE_BODY) {}
	virtual INLINE_BODYInstruction* toINLINE_BODYInstruction() { return this;}

};

class BUILTINInstruction : public Instruction {
public:
	BUILTINInstruction() : Instruction(ICMD_BUILTIN) {}
	virtual BUILTINInstruction* toBUILTINInstruction() { return this;}

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
