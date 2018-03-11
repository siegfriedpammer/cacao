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

#include <memory>

#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/MachineInstructions.hpp"
#include "vm/jit/compiler2/InstructionVisitor.hpp"
#include "vm/jit/compiler2/memory/Manager.hpp"

MM_MAKE_NAME(Backend)

namespace cacao {
namespace jit {
namespace compiler2 {

// forward declarations
class StackSlotManager;
class JITData;
class MachineBasicBlock;
class OperandSet;

class RegisterClass {
public:
	virtual bool handles_type(Type::TypeID type) const = 0;
	virtual Type::TypeID default_type() const = 0;

	virtual unsigned count() const = 0;
	virtual const OperandSet& get_All() const = 0;

	virtual unsigned caller_saved_count() const = 0;
	virtual const OperandSet& get_CallerSaved() const = 0;

	virtual unsigned callee_saved_count() const = 0;
	virtual const OperandSet& get_CalleeSaved() const = 0;

	virtual ~RegisterClass() = default;
};

class RegisterInfo {
public:
	RegisterInfo(const RegisterInfo&) = delete;
	RegisterInfo& operator=(const RegisterInfo&) = delete;

	explicit RegisterInfo(JITData* JD) : JD(JD) {}

	unsigned class_count() const { return classes.size(); };
	const RegisterClass& get_class(unsigned idx) const {
		assert(idx < classes.size() && "This RI does not have that many classes!");
		return *classes[idx];
	}
	
	/// Returns all callee saved registers from all register classes 
	OperandSet get_AllCalleeSaved() const;

protected:
	JITData* JD;

	using RegisterClassUPtrTy = std::unique_ptr<RegisterClass>;
	std::vector<RegisterClassUPtrTy> classes;
};

template<typename Target>
class RegisterInfoBase : public RegisterInfo {
public:
	RegisterInfoBase(JITData *JD) : RegisterInfo(JD) {}
};

class Backend : public memory::ManagerMixin<Backend>  {
private:
	JITData *JD;
	RegisterInfo *RI;
protected:
	Backend(JITData *JD, RegisterInfo *RI) : JD(JD), RI(RI) {}
public:
	static Backend* factory(JITData *JD);
	JITData* get_JITData() const { return JD; }
	RegisterInfo* get_RegisterInfo() const { return RI; }

	/// Do not confuse this factory with the one from JITData. The JITData factory
	/// provides ALL non platform specific operands, the native factory only provides
	/// the concrete platform dependent register operands
	virtual MachineOperandFactory* get_NativeFactory() const = 0;

	virtual OperandFile& get_OperandFile(OperandFile& OF,MachineOperand *MO) const = 0;
	virtual MachineInstruction* create_Move(MachineOperand *src,
		MachineOperand* dst) const = 0;
	virtual MachineInstruction* create_Jump(MachineBasicBlock *target) const = 0;
	virtual MachineInstruction* create_Swap(MachineOperand *op1, MachineOperand *op2) const = 0;
	virtual void create_frame(CodeMemory* CM, StackSlotManager *SSM, const OperandSet&) const = 0;
	
	/// Simple type that ties together a callee saved register with the stack slot it is
	/// saved to
	using CalleeSavedRegisters = std::vector<std::pair<MachineOperand*,MachineOperand*>>;
	virtual void create_prolog(MachineBasicBlock*, const CalleeSavedRegisters&) const = 0;
	virtual void create_epilog(MachineBasicBlock*, const CalleeSavedRegisters&) const = 0;

	virtual const char* get_name() const = 0;
	virtual ~Backend() = default;
};
/**
 * Machine Backend
 *
 * This class containes all target dependent information.
 */
template <typename Target>
class BackendBase : public Backend {
public:
	BackendBase(JITData *JD, RegisterInfo *RI) : Backend(JD, RI) {}

	virtual MachineOperandFactory* get_NativeFactory() const;
	virtual OperandFile& get_OperandFile(OperandFile& OF,MachineOperand *MO) const;
	virtual MachineInstruction* create_Move(MachineOperand *src,
		MachineOperand* dst) const;
	virtual MachineInstruction* create_Jump(MachineBasicBlock *target) const;
	virtual MachineInstruction* create_Swap(MachineOperand *op1,
		MachineOperand* op2) const;
	virtual void create_frame(CodeMemory* CM, StackSlotManager *SSM, const OperandSet&) const;

	virtual void create_prolog(MachineBasicBlock*, const CalleeSavedRegisters&) const;
	virtual void create_epilog(MachineBasicBlock*, const CalleeSavedRegisters&) const;

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
	MachineInstructionSchedule *schedule;
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

	// Forward operand factory calls, less typing for lowering visitors
	VirtualRegister* CreateVirtualRegister(Type::TypeID type);
	
public:
	LoweringVisitorBase(Backend *backend,MachineBasicBlock* current,
		MapTy &map, InstructionMapTy &inst_map, MachineInstructionSchedule *schedule)
			: backend(backend), current(current), map(map), inst_map(inst_map),
			schedule(schedule) {}

	virtual void visit_default(Instruction *I) {
		ABORT_MSG("LoweringVisitor","Instruction " << BoldWhite
		  << *I << reset_color << " not yet handled by the Backend");
	}
	MachineBasicBlock* get_current() const { return current; }
	void set_current(MachineBasicBlock* MBB) { current = MBB; }
	MachineBasicBlock* new_block() const;
	// make InstructionVisitors visit visible
	using InstructionVisitor::visit;

	virtual void visit(BeginInst* I, bool copyOperands);
	virtual void visit(GOTOInst* I, bool copyOperands);
	virtual void visit(PHIInst* I, bool copyOperands);
	virtual void visit(CONSTInst* I, bool copyOperands);
	virtual void visit(SourceStateInst* I, bool copyOperands);
	virtual void visit(ReplacementEntryInst* I, bool copyOperands);

	void lower_source_state_dependencies(MachineReplacementPointInst *MI,
		SourceStateInst *source_state);

	void place_deoptimization_marker(SourceStateAwareInst *I);

	/* RuleId enum cannot be used in interface, as including MatcherDefs.hpp 
	 * requires Grammar.inc. Backend.hpp is included in several other files 
	 * that are compiled without CXXFLAGS containing arch folder.
	 * Interface uses int, implicit conversion is done within method implementation.
	 */
	virtual void lowerComplex(Instruction* I, int ruleId) = 0;
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
