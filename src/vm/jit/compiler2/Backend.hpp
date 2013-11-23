/* src/vm/jit/compiler2/Backend.hpp - Backend

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

#ifndef _JIT_COMPILER2_BACKEND
#define _JIT_COMPILER2_BACKEND

#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/MachineInstructions.hpp"
#include "vm/jit/compiler2/InstructionVisitor.hpp"
#include "vm/jit/compiler2/memory/Manager.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

// forward declarations
class StackSlotManager;
class JITData;
class RegisterFile;
class MachineBasicBlock;

class Backend : public memory::ManagerMixin<Backend>  {
private:
	JITData *JD;
protected:
	Backend(JITData *JD) : JD(JD) {}
public:
	static Backend* factory(JITData *JD);
	JITData* get_JITData() const { return JD; };

	//virtual RegisterFile* get_RegisterFile(Type::TypeID type) const = 0;
	virtual OperandFile& get_OperandFile(OperandFile& OF,MachineOperand *MO) const = 0;
	virtual MachineInstruction* create_Move(MachineOperand *src,
		MachineOperand* dst) const = 0;
	virtual MachineInstruction* create_Jump(MachineBasicBlock *target) const = 0;
	virtual void create_frame(CodeMemory* CM, StackSlotManager *SSM) const = 0;
	virtual const char* get_name() const = 0;
};
/**
 * Machine Backend
 *
 * This class containes all target dependent information.
 */
template <typename Target>
class BackendBase : public Backend {
public:
	BackendBase(JITData *JD) : Backend(JD) {}
	//virtual RegisterFile* get_RegisterFile(Type::TypeID type) const;
	virtual OperandFile& get_OperandFile(OperandFile& OF,MachineOperand *MO) const;
	virtual MachineInstruction* create_Move(MachineOperand *src,
		MachineOperand* dst) const;
	virtual MachineInstruction* create_Jump(MachineBasicBlock *target) const;
	virtual void create_frame(CodeMemory* CM, StackSlotManager *SSM) const;
	virtual const char* get_name() const;
};

class LoweringVisitorBase : public InstructionVisitor {
protected:
	typedef alloc::map<BeginInst*,MachineBasicBlock*>::type MapTy;
	typedef alloc::map<Instruction*,MachineOperand*>::type InstructionMapTy;
private:
	Backend *backend;
	MachineBasicBlock* current;
	MapTy &map;
	InstructionMapTy &inst_map;
protected:
	Backend* get_Backend() const {
		return backend;
	}
	MachineBasicBlock* get(BeginInst* BI) const {
		assert(BI);
		MapTy::const_iterator it = map.find(BI);
		assert_msg(it != map.end(), "MachineBasicBlock for BeginInst "<< *BI << " not found");
		return it->second;
	}
	MachineOperand* get_op(Instruction* I) const {
		assert(I);
		InstructionMapTy::const_iterator it = inst_map.find(I);
		assert_msg(it != inst_map.end(), "operand for instruction " << *I << " not found");
		return it->second;
	}
	void set_op(Instruction* I, MachineOperand* op) const {
		assert(I);
		assert(op);
		inst_map[I] = op;
	}
public:
	LoweringVisitorBase(Backend *backend,MachineBasicBlock* current,
		MapTy &map, InstructionMapTy &inst_map)
			: backend(backend), current(current), map(map), inst_map(inst_map) {}

	virtual void visit_default(Instruction *I) {
		ABORT_MSG("LoweringVisitor","Instruction " << BoldWhite
		  << *I << reset_color << " not yet handled by the Backend");
	}
	MachineBasicBlock* get_current() const { return current; }
	// make InstructionVisitors visit visible
	using InstructionVisitor::visit;

	virtual void visit(BeginInst* I);
	virtual void visit(GOTOInst* I);
	virtual void visit(PHIInst* I);
	virtual void visit(CONSTInst* I);
};


} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_BACKEND */


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
