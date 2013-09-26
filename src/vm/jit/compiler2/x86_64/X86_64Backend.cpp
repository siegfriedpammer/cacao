/* src/vm/jit/compiler2/X86_64Backend.hpp - X86_64Backend

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

#include "vm/jit/compiler2/x86_64/X86_64Backend.hpp"
#include "vm/jit/compiler2/x86_64/X86_64Instructions.hpp"
#include "vm/jit/compiler2/x86_64/X86_64Register.hpp"
#include "vm/jit/compiler2/x86_64/X86_64MachineMethodDescriptor.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/LoweredInstDAG.hpp"
#include "vm/jit/compiler2/MethodDescriptor.hpp"
#include "vm/jit/compiler2/CodeMemory.hpp"
#include "vm/jit/compiler2/StackSlotManager.hpp"
#include "vm/jit/Patcher.hpp"
#include "vm/jit/jit.hpp"
#include "vm/jit/code.hpp"
#include "vm/class.hpp"
#include "vm/field.hpp"

#include "toolbox/OStream.hpp"
#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/x86_64"

// code.hpp fix
#undef RAX
#undef XMM0

namespace cacao {
namespace jit {
namespace compiler2 {

// BackendBase must be specialized in namespace compiler2!
using namespace x86_64;

template<>
const char* BackendBase<X86_64>::get_name() const {
	return "x86_64";
}

template<>
MachineInstruction* BackendBase<X86_64>::create_Move(MachineOperand *src,
		MachineOperand* dst) const {
	Type::TypeID type = dst->get_type();
	assert(type == src->get_type());
	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		return new MovInst(
			SrcOp(src),
			DstOp(dst),
			get_OperandSize_from_Type(type));
	case Type::DoubleTypeID:
		switch (src->get_OperandID()) {
		case MachineOperand::ImmediateID:
			return new MovImmSDInst(
				SrcOp(src),
				DstOp(dst));
		default:
			return new MovSDInst(
				SrcOp(src),
				DstOp(dst));
		}
	default: break;
	}
	ABORT_MSG("x86_64: Move not supported",
		"Inst: " << src << " -> " << dst << " type: " << type);
	return NULL;
}

template<>
MachineInstruction* BackendBase<X86_64>::create_Jump(BeginInstRef &target) const {
	return new JumpInstStub(target.get());
}

namespace {

template <unsigned size, class T>
inline T align_to(T val) {
	T rem =(val % size);
	return val + ( rem == 0 ? 0 : size - rem);
}

} // end anonymous namespace

template<>
void BackendBase<X86_64>::create_frame(CodeMemory* CM, StackSlotManager *SSM) const {
	EnterInst enter(align_to<16>(SSM->get_frame_size()));
	enter.emit(CM);
	// fix alignment
	CodeFragment CF = CM->get_aligned_CodeFragment(0);
	emit_nop(CF,CF.size());
}

void LoweringVisitor::visit(LOADInst *I) {
	assert(I);
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	//MachineInstruction *minst = loadParameter(I->get_index(), I->get_type());
	const MethodDescriptor &MD = I->get_Method()->get_MethodDescriptor();
	//FIXME inefficient
	const MachineMethodDescriptor MMD(MD);
	Type::TypeID type = I->get_type();
	VirtualRegister *dst = new VirtualRegister(type);
	MachineInstruction *move = NULL;
	switch (type) {
	case Type::ByteTypeID:
	case Type::ShortTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		move = new MovInst(
			SrcOp(MMD[I->get_index()]),
			DstOp(dst),
			get_OperandSize_from_Type(type));
			break;
	case Type::DoubleTypeID:
		move = new MovSDInst(
			SrcOp(MMD[I->get_index()]),
			DstOp(dst));
			break;
	default:
		ABORT_MSG("x86_64 type not supported: ",
			  I << " type: " << type);
	}
	dag->add(move);
	dag->set_result(move);
	set_dag(dag);
}

void LoweringVisitor::visit(IFInst *I) {
	assert(I);
	Type::TypeID type = I->get_type();
	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
	{
		// Integer types
		LoweredInstDAG *dag = new LoweredInstDAG(I);
		CmpInst *cmp = new CmpInst(
			Src2Op(new UnassignedReg(type)),
			Src1Op(new UnassignedReg(type)),
			get_OperandSize_from_Type(type));

		MachineInstruction *cjmp = NULL;
		BeginInstRef &then = I->get_then_target();
		BeginInstRef &els = I->get_else_target();

		switch (I->get_condition()) {
		case Conditional::EQ:
			cjmp = new CondJumpInstStub(Cond::E, then.get());
			break;
		case Conditional::LT:
			cjmp = new CondJumpInstStub(Cond::L, then.get());
			break;
		case Conditional::LE:
			cjmp = new CondJumpInstStub(Cond::LE, then.get());
			break;
		case Conditional::GE:
			cjmp = new CondJumpInstStub(Cond::GE, then.get());
			break;
		default:
			ABORT_MSG("x86_64 Conditioal not supported: ",
				  I << " cond: " << I->get_condition());
		}
		MachineInstruction *jmp = new JumpInstStub(els.get());
		dag->add(cmp);
		dag->add(cjmp);
		dag->add(jmp);

		dag->set_input(0,cmp,0);
		dag->set_input(1,cmp,1);
		dag->set_result(jmp);
		set_dag(dag);
		return;
	}
	default: break;
	}
	ABORT_MSG("x86_64: Lowering not supported",
		"Inst: " << I << " type: " << type);
}

void LoweringVisitor::visit(ADDInst *I) {
	assert(I);
	Type::TypeID type = I->get_type();
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	VirtualRegister *dst = new VirtualRegister(type);
	MachineInstruction *mov = get_Backend()->create_Move(new UnassignedReg(type),dst);
	MachineInstruction *alu = NULL;

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		alu = new AddInst(
			Src2Op(new UnassignedReg(type)),
			DstSrc1Op(dst),
			get_OperandSize_from_Type(type));
		break;
	case Type::DoubleTypeID:
		alu = new AddSDInst(
			Src2Op(new UnassignedReg(type)),
			DstSrc1Op(dst));
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	dag->add(mov);
	dag->add(alu);
	dag->set_input(1,alu,1);
	dag->set_input(0,mov,0);
	dag->set_result(alu);
	set_dag(dag);
}

void LoweringVisitor::visit(ANDInst *I) {
	assert(I);
	Type::TypeID type = I->get_type();
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	VirtualRegister *dst = new VirtualRegister(type);
	MachineInstruction *mov = get_Backend()->create_Move(new UnassignedReg(type),dst);
	MachineInstruction *alu = NULL;

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		alu = new AndInst(
			Src2Op(new UnassignedReg(type)),
			DstSrc1Op(dst),
			get_OperandSize_from_Type(type));
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	dag->add(mov);
	dag->add(alu);
	dag->set_input(1,alu,1);
	dag->set_input(0,mov,0);
	dag->set_result(alu);
	set_dag(dag);
}

void LoweringVisitor::visit(SUBInst *I) {
	assert(I);
	Type::TypeID type = I->get_type();
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	VirtualRegister *dst = new VirtualRegister(type);
	MachineInstruction *mov = get_Backend()->create_Move(new UnassignedReg(type),dst);
	MachineInstruction *alu = NULL;

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		alu = new SubInst(
			Src2Op(new UnassignedReg(type)),
			DstSrc1Op(dst),
			get_OperandSize_from_Type(type));
		break;
	case Type::DoubleTypeID:
		alu = new SubSDInst(
			Src2Op(new UnassignedReg(type)),
			DstSrc1Op(dst));
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	dag->add(mov);
	dag->add(alu);
	dag->set_input(1,alu,1);
	dag->set_input(0,mov,0);
	dag->set_result(alu);
	set_dag(dag);
}

void LoweringVisitor::visit(MULInst *I) {
	assert(I);
	Type::TypeID type = I->get_type();

	LoweredInstDAG *dag = new LoweredInstDAG(I);
	VirtualRegister *dst = new VirtualRegister(type);
	MachineInstruction *mov = get_Backend()->create_Move(new UnassignedReg(type),dst);
	MachineInstruction *alu = NULL;

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		alu = new IMulInst(
			Src2Op(new UnassignedReg(type)),
			DstSrc1Op(dst),
			get_OperandSize_from_Type(type));
		break;
	case Type::DoubleTypeID:
		alu = new MulSDInst(
			Src2Op(new UnassignedReg(type)),
			DstSrc1Op(dst));
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	dag->add(mov);
	dag->add(alu);
	dag->set_input(1,alu,1);
	dag->set_input(0,mov,0);
	dag->set_result(alu);
	set_dag(dag);
}

void LoweringVisitor::visit(DIVInst *I) {
	assert(I);
	Type::TypeID type = I->get_type();

	LoweredInstDAG *dag = new LoweredInstDAG(I);
	VirtualRegister *dst = new VirtualRegister(type);
	MachineInstruction *mov = get_Backend()->create_Move(new UnassignedReg(type), dst);
	MachineInstruction *alu = NULL;

	switch (type) {
	#if 0
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		break;
	#endif
	case Type::DoubleTypeID:
		alu = new DivSDInst(
			Src2Op(new UnassignedReg(type)),
			DstSrc1Op(dst));
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	dag->add(mov);
	dag->add(alu);
	dag->set_input(1,alu,1);
	dag->set_input(0,mov,0);
	dag->set_result(alu);
	set_dag(dag);
}

void LoweringVisitor::visit(REMInst *I) {
	assert(I);
	Type::TypeID type = I->get_type();

	LoweredInstDAG *dag = new LoweredInstDAG(I);
	#if 0
	VirtualRegister *dst = new VirtualRegister(type);
	MachineInstruction *mov = get_Backend()->create_Move(new UnassignedReg(type), dst);
	MachineInstruction *alu;

	switch (type) {
	#if 0
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		break;
	#endif
	case Type::DoubleTypeID:
		alu = new DivSDInst(
			Src2Op(new UnassignedReg(type)),
			DstSrc1Op(dst));
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	dag->add(mov);
	dag->add(alu);
	dag->set_input(1,alu,1);
	dag->set_input(0,mov,0);
	dag->set_result(alu);
	#endif
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	set_dag(dag);
}

void LoweringVisitor::visit(RETURNInst *I) {
	assert(I);
	Type::TypeID type = I->get_type();
	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
	{
		LoweredInstDAG *dag = new LoweredInstDAG(I);
		MachineInstruction *reg = new MovInst(
			SrcOp(new UnassignedReg(type)),
			DstOp(new NativeRegister(type,&RAX)),
			get_OperandSize_from_Type(type));
		LeaveInst *leave = new LeaveInst();
		RetInst *ret = new RetInst(get_OperandSize_from_Type(type));
		dag->add(reg);
		dag->add(leave);
		dag->add(ret);
		dag->set_input(reg);
		dag->set_result(ret);
		set_dag(dag);
		return;
	}
	case Type::DoubleTypeID:
	{
		LoweredInstDAG *dag = new LoweredInstDAG(I);
		MachineInstruction *reg = new MovSDInst(
			SrcOp(new UnassignedReg(type)),
			DstOp(new NativeRegister(type,&XMM0)));
		LeaveInst *leave = new LeaveInst();
		RetInst *ret = new RetInst(get_OperandSize_from_Type(type));
		dag->add(reg);
		dag->add(leave);
		dag->add(ret);
		dag->set_input(reg);
		dag->set_result(ret);
		set_dag(dag);
		return;
	}
	default: break;
	}
	ABORT_MSG("x86_64 Lowering not supported",
		"Inst: " << I << " type: " << type);
}

void LoweringVisitor::visit(CASTInst *I) {
  assert(I);
  LoweredInstDAG *dag = new LoweredInstDAG(I);
  Type::TypeID from = I->get_operand(0)->to_Instruction()->get_type();
  Type::TypeID to = I->get_type();

  switch (from) {
  case Type::IntTypeID:
	  switch (to) {
	  case Type::LongTypeID:
	  {
		  MachineInstruction *mov = new MovSXInst(
			  SrcOp(new UnassignedReg(from)),
			  DstOp(new VirtualRegister(to)),
			  GPInstruction::OS_32, GPInstruction::OS_64);
		  dag->add(mov);
		  dag->set_input(mov);
		  dag->set_result(mov);
		  set_dag(dag);
		  return;
	  }
	  default: break;
	  }
	  break;
  case Type::LongTypeID:
	  switch (to) {
	  case Type::DoubleTypeID:
	  {
		  MachineInstruction *mov = new CVTSI2SDInst(
			  SrcOp(new UnassignedReg(from)),
			  DstOp(new VirtualRegister(to)),
			  GPInstruction::OS_64, GPInstruction::OS_64);
		  dag->add(mov);
		  dag->set_input(mov);
		  dag->set_result(mov);
		  set_dag(dag);
		  return;
	  }
	  default: break;
	  }
	  break;
  default: break;
  }
  ABORT_MSG("x86_64 Cast not supported!", "From " << from << " to " << to );
}

void LoweringVisitor::visit(INVOKESTATICInst *I) {
	assert(I);
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	Type::TypeID type = I->get_type();
	MethodDescriptor &MD = I->get_MethodDescriptor();
	MachineMethodDescriptor MMD(MD);

	// operands for the call
	VirtualRegister *addr = new VirtualRegister(Type::ReferenceTypeID);
	MachineOperand *result = &NoOperand;

	// get return value
	switch (type) {
	case Type::IntTypeID:
	case Type::LongTypeID:
		result = new NativeRegister(type,&RAX);
		break;
	default:
		ABORT_MSG("x86_64 Lowering not supported",
		"Inst: " << I << " type: " << type);
	}

	// create call
	MachineInstruction* call = new CallInst(SrcOp(addr),DstOp(result),I->op_size());
	// move values to parameters
	for (std::size_t i = 0; i < I->op_size(); ++i ) {
		MachineInstruction* mov = get_Backend()->create_Move(
			new UnassignedReg(MD[i]),
			MMD[i]
		);
		dag->add(mov);
		dag->set_input(i,mov,0);
		// set call operand
		call->set_operand(i+1,MMD[i]);
	}
	// spill caller saved

	// load address
	DataSegment &DS = get_Backend()->get_JITData()->get_CodeMemory()->get_DataSegment();
	DataSegment::IdxTy idx = DS.get_index(DSFMIRef(I->get_fmiref()));
	if (DataSegment::is_invalid(idx)) {
		DataFragment data = DS.get_Ref(sizeof(void*));
		idx = DS.insert_tag(DSFMIRef(I->get_fmiref()),data);
	}
	if (I->is_resolved()) {
		LOG2("INVOKESTATICInst: is resolved" << nl);
		// write stubroutine
		//methodinfo*         lm;             // Local methodinfo for ICMD_INVOKE*.
		//lm = I->get_fmiref()->p.method;
		//lm->stubroutine;
	} else {
		LOG2("INVOKESTATICInst: is notresolved" << nl);
	}
	MovDSEGInst *dmov = new MovDSEGInst(DstOp(addr),idx);
	dag->add(dmov);

	// add call
	dag->add(call);
	// get result
	if (result != &NoOperand) {
		MachineInstruction *reg = new MovInst(
			SrcOp(result),
			DstOp(new VirtualRegister(type)),
			get_OperandSize_from_Type(type));
		dag->add(reg);
		dag->set_result(reg);
	}
	set_dag(dag);
	#if 0
	if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
		unresolved_method*  um;
		um = iptr->sx.s23.s3.um;
		disp = dseg_add_unique_address(cd, um);

		patcher_add_patch_ref(jd, PATCHER_invokestatic_special,
							  um, disp);
	}
	else {
		methodinfo*         lm;             // Local methodinfo for ICMD_INVOKE*.
		lm = iptr->sx.s23.s3.fmiref->p.method;
		disp = dseg_add_functionptr(cd, lm->stubroutine);
	}
	#endif
}

void LoweringVisitor::visit(GETSTATICInst *I) {
	assert(I);
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	DataSegment &DS = get_Backend()->get_JITData()->get_CodeMemory()->get_DataSegment();
	DataSegment::IdxTy idx = DS.get_index(DSFMIRef(I->get_fmiref()));
	if (DataSegment::is_invalid(idx)) {
		DataFragment data = DS.get_Ref(sizeof(void*));
		idx = DS.insert_tag(DSFMIRef(I->get_fmiref()),data);
	}

	if (I->is_resolved()) {
		fieldinfo* fi = I->get_fmiref()->p.field;

		if (!class_is_or_almost_initialized(fi->clazz)) {
			//PROFILE_CYCLE_STOP;
			Patcher *patcher = new InitializeClassPatcher(fi->clazz);
			PatcherPtrTy ptr(patcher);
			get_Backend()->get_JITData()->get_jitdata()->code->patchers->push_back(ptr);
			MachineInstruction *pi = new PatchInst(patcher);
			dag->add(pi);
			//PROFILE_CYCLE_START;
		}

	} else {
		assert(0 && "Not yet implemented");
		#if 0
		unresolved_field* uf = iptr->sx.s23.s3.uf;
		fieldtype = uf->fieldref->parseddesc.fd->type;
		disp      = dseg_add_unique_address(cd, 0);

		pr = patcher_add_patch_ref(jd, PATCHER_get_putstatic, uf, disp);

		fi = NULL;		/* Silence compiler warning */
		#endif
	}
	VirtualRegister *addr = new VirtualRegister(Type::ReferenceTypeID);
	MovDSEGInst *dmov = new MovDSEGInst(DstOp(addr),idx);
	MachineInstruction* mov = get_Backend()->create_Move(addr,new VirtualRegister(I->get_type()));
	dag->add(dmov);
	dag->add(mov);
	dag->set_result(mov);
	set_dag(dag);
}

void LoweringVisitor::visit(LOOKUPSWITCHInst *I) {
	assert(I);
	Type::TypeID type = I->get_type();
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	VirtualRegister *src = new VirtualRegister(type);
	MachineInstruction *mov = get_Backend()->create_Move(new UnassignedReg(type), src);
	dag->add(mov);

	LOOKUPSWITCHInst::succ_const_iterator s = I->succ_begin();
	for(LOOKUPSWITCHInst::match_iterator i = I->match_begin(),
			e = I->match_end(); i != e; ++i) {
		CmpInst *cmp = new CmpInst(
			Src2Op(new Immediate(*i,Type::IntType())),
			Src1Op(src),
			get_OperandSize_from_Type(type));
		MachineInstruction *cjmp = new CondJumpInstStub(Cond::E, s->get());
		dag->add(cmp);
		dag->add(cjmp);
		++s;
	}

	// default
	MachineInstruction *jmp = new JumpInstStub(s->get());
	dag->add(jmp);
	assert(++s == I->succ_end());

	dag->set_input(0,mov,0);
	dag->set_result(jmp);
	set_dag(dag);
}

void LoweringVisitor::visit(TABLESWITCHInst *I) {
	assert(I);
	Type::TypeID type = I->get_type();
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	VirtualRegister *src = new VirtualRegister(type);
	MachineInstruction *mov = get_Backend()->create_Move(new UnassignedReg(type), src);
	dag->add(mov);

	s4 low = I->get_low();
	s4 high = I->get_high();

	// adjust offset
	if (low != 0) {
		SubInst *sub = new SubInst(
			Src2Op(new Immediate(low,Type::IntType())),
			DstSrc1Op(src),
			get_OperandSize_from_Type(type)
		);
		dag->add(sub);
		high -= low;
	}
	// check range
	CmpInst *cmp = new CmpInst(
		Src2Op(new Immediate(high,Type::IntType())),
		Src1Op(src),
		get_OperandSize_from_Type(type));
	MachineInstruction *cjmp = new CondJumpInstStub(Cond::G, I->succ_front().get());
	dag->add(cmp);
	dag->add(cjmp);

	// TODO load data segment and jump
	// load address
	DataSegment &DS = get_Backend()->get_JITData()->get_CodeMemory()->get_DataSegment();
	DataFragment data = DS.get_Ref(sizeof(void*) * (high - low + 1));
	DataSegment::IdxTy idx = data.get_begin();
	VirtualRegister *addr = new VirtualRegister(Type::ReferenceTypeID);
	WARNING_MSG("TODO","add offset");
	MovDSEGInst *dmov = new MovDSEGInst(DstOp(addr),idx);
	dag->add(dmov);
	IndirectJumpStub *jmp = new IndirectJumpStub(SrcOp(addr));
	// adding targets
	for(TABLESWITCHInst::succ_const_iterator i = ++I->succ_begin(),
			e = I->succ_end(); i != e; ++i) {
		jmp->add_target(i->get());
	}
	dag->add(jmp);
	// assert(0 && "load data segment and jump"));
	// load table entry
	dag->set_input(0,mov,0);
	dag->set_result(cjmp);
	set_dag(dag);
}

template<>
compiler2::RegisterFile*
BackendBase<X86_64>::get_RegisterFile(Type::TypeID type) const {
	return new x86_64::RegisterFile(type);
}

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao


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
