/* src/vm/jit/compiler2/CodeGenPass.cpp - CodeGenPass

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

#include "vm/jit/compiler2/CodeGenPass.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/SSADeconstructionPass.hpp"
#include "vm/jit/compiler2/MachineRegister.hpp"
#include "vm/jit/compiler2/lsra/RegisterAssignmentPass.hpp"

#include "toolbox/logging.hpp"

#include "mm/codememory.hpp"
#include "vm/types.hpp"
#include "vm/jit/executionstate.hpp"
#include "vm/jit/jit.hpp"
#include "vm/jit/methodtree.hpp"
#include "vm/jit/linenumbertable.hpp"
#include "vm/jit/exceptiontable.hpp"

#include "vm/jit/disass.hpp"

#include "mm/memory.hpp"

#include "md.hpp"

#include "vm/jit/replace.hpp"

#define DEBUG_NAME "compiler2/CodeGen"

STAT_DECLARE_VAR(std::size_t, compiler_last_codesize, 0)
STAT_DECLARE_VAR(std::size_t, num_remaining_moves,0)

namespace cacao {
namespace jit {
namespace compiler2 {

namespace {
static void emit_nop(CodeFragment code, int length) {
	assert(length >= 0 && length <= 9);
	unsigned mcodeptr = 0;
	switch (length) {
	case 0:
		break;
	case 1:
		code[mcodeptr++] = 0x90;
		break;
	case 2:
		code[mcodeptr++] = 0x66;
		code[mcodeptr++] = 0x90;
		break;
	case 3:
		code[mcodeptr++] = 0x0f;
		code[mcodeptr++] = 0x1f;
		code[mcodeptr++] = 0x00;
		break;
	case 4:
		code[mcodeptr++] = 0x0f;
		code[mcodeptr++] = 0x1f;
		code[mcodeptr++] = 0x40;
		code[mcodeptr++] = 0x00;
		break;
	case 5:
		code[mcodeptr++] = 0x0f;
		code[mcodeptr++] = 0x1f;
		code[mcodeptr++] = 0x44;
		code[mcodeptr++] = 0x00;
		code[mcodeptr++] = 0x00;
		break;
	case 6:
		code[mcodeptr++] = 0x66;
		code[mcodeptr++] = 0x0f;
		code[mcodeptr++] = 0x1f;
		code[mcodeptr++] = 0x44;
		code[mcodeptr++] = 0x00;
		code[mcodeptr++] = 0x00;
		break;
	case 7:
		code[mcodeptr++] = 0x0f;
		code[mcodeptr++] = 0x1f;
		code[mcodeptr++] = 0x80;
		code[mcodeptr++] = 0x00;
		code[mcodeptr++] = 0x00;
		code[mcodeptr++] = 0x00;
		code[mcodeptr++] = 0x00;
		break;
	case 8:
		code[mcodeptr++] = 0x0f;
		code[mcodeptr++] = 0x1f;
		code[mcodeptr++] = 0x84;
		code[mcodeptr++] = 0x00;
		code[mcodeptr++] = 0x00;
		code[mcodeptr++] = 0x00;
		code[mcodeptr++] = 0x00;
		code[mcodeptr++] = 0x00;
		break;
	case 9:
		code[mcodeptr++] = 0x66;
		code[mcodeptr++] = 0x0f;
		code[mcodeptr++] = 0x1f;
		code[mcodeptr++] = 0x84;
		code[mcodeptr++] = 0x00;
		code[mcodeptr++] = 0x00;
		code[mcodeptr++] = 0x00;
		code[mcodeptr++] = 0x00;
		code[mcodeptr++] = 0x00;
		break;
	}
}
}

#ifdef ENABLE_LOGGING
Option<bool> CodeGenPass::print_code("PrintCodeSegment","compiler2: print code segment",false,::cacao::option::xx_root());
Option<bool> CodeGenPass::print_data("PrintDataSegment","compiler2: print data segment",false,::cacao::option::xx_root());
#endif

bool CodeGenPass::run(JITData &JD) {
	MachineInstructionSchedule *MIS = get_Artifact<LIRInstructionScheduleArtifact>()->MIS;
	CodeMemory *CM = JD.get_CodeMemory();
	CodeSegment &CS = CM->get_CodeSegment();
	StackSlotManager *SSM = JD.get_StackSlotManager();
	MachineOperandFactory *MOF = JD.get_MachineOperandFactory();

	// Create Prolog and Epilog, first calculate all used callee saved registers
	// and assign each callee saved register a stackslot and a native register
	Backend::CalleeSavedRegisters registers;
	auto used_operands = get_Artifact<RegisterAssignmentPass>()->get_used_operands();
	auto RI = JD.get_Backend()->get_RegisterInfo();
	for (unsigned idx = 0; idx < RI->class_count(); ++idx) {
		const auto& regclass = RI->get_class(idx);
		auto class_operands = used_operands & regclass.get_CalleeSaved();

		std::for_each(class_operands.begin(), class_operands.end(), [&](auto& operand) {
			MachineOperand* native_reg = MOF->CreateNativeRegister<NativeRegister>(regclass.default_type(), &operand);
			auto slot = SSM->create_slot(regclass.default_type());

			registers.push_back({native_reg, slot});
		});
	}

	SSM->finalize();

	JD.get_Backend()->create_prolog(*MIS->begin(), registers);
	for (const auto block : *MIS) {
		if (block->back()->is_end()) {
			JD.get_Backend()->create_epilog(block, registers);
		}
	}

	// NOTE reverse so we see jump targets (which are not backedges) before
	// the jump.
	MachineBasicBlock *MBB = NULL;
	std::size_t bb_start = 0;
	for (MachineInstructionSchedule::const_reverse_iterator i = MIS->rbegin(),
			e = MIS->rend() ; i != e ; ++i ) {
		MBB = *i;
		bb_start = CS.size();
		for (MachineBasicBlock::const_reverse_iterator i = MBB->rbegin(),
				e = MBB->rend(); i != e ; ++i) {
			MachineInstruction *MI = *i;
			std::size_t start = CS.size();
			LOG2("MInst: " << MI << " emitted instruction:" << nl);
			MI->emit(CM);
			std::size_t end = CS.size();
			instruction_positions[MI] = start;
			instruction_sizes[MI] = end - start;
			#if defined(ENABLE_STATISTICS)
			if (MI->is_move() && start != end) {
				STATISTICS(++num_remaining_moves);
			}
			#endif
			if (DEBUG_COND_N(2)) {
				if ( start == end) {
					LOG2("none" << nl);
				} else {
					alloc::vector<u1>::type tmp;
					while(start != end--) {
						tmp.push_back(CS.at(end));
					}
	#if defined(ENABLE_DISASSEMBLER)
					disassemble(&tmp.front(), &tmp.front() + tmp.size());
	#else
					// TODO print hex code
	#endif
				}
			}
		}
		std::size_t bb_end = CS.size();
		bbmap[MBB] = bb_end - bb_start;
	}
	assert(MBB != NULL);

	// Align code
	/// @todo This needs to be platform independent
#if defined(__X86_64__)
	CodeFragment CF = CM->get_aligned_CodeFragment(0);
	emit_nop(CF,CF.size());
#endif

	// fix last block (frame start, alignment)
	bbmap[MBB] = CS.size() - bb_start;
	// finish
	finish(JD);
	return true;
}

std::size_t CodeGenPass::get_block_size(MachineBasicBlock *MBB) const {
	BasicBlockMap::const_iterator i = bbmap.find(MBB);
	if (i == bbmap.end())
		return 0;
	return i->second;
}

namespace {

#ifdef ENABLE_LOGGING
void print_hex(OStream &OS, u1 *start, u1 *end, uint32_t num_bytes_per_line = 8) {
	OS << hex;
	for(u1 *ptr = start, *e = end; ptr < e; ++ptr) {
		if ( (ptr - start) % num_bytes_per_line == 0) {
			OS << nl << "0x" << setz(16) << (u8) ptr << ": ";
		}
		OS << setz(2) << *ptr << ' ';
	}
	OS << dec << nl;
}
#endif

template<class OutputIt>
void find_all_replacement_points(MachineInstructionSchedule *MIS,
		OutputIt outIterator) {
	MachineBasicBlock *MBB = NULL;
	for (MachineInstructionSchedule::const_iterator i = MIS->begin(),
			e = MIS->end() ; i != e ; ++i ) {
		MBB = *i;
		for (MachineBasicBlock::const_iterator i = MBB->begin(),
				e = MBB->end(); i != e ; ++i) {
			MachineInstruction *MI = *i;
			if (MI->to_MachineReplacementPointInst()) {
				*outIterator = MI;
				outIterator++;
			}
		}
	}
}

} // end anonymous namespace

template<class ForwardIt>
void CodeGenPass::resolve_replacement_points(ForwardIt first, ForwardIt last, JITData &JD) {
	codeinfo *code = JD.get_jitdata()->code;
	CodeMemory *CM = JD.get_CodeMemory();
	CodeSegment &CS = CM->get_CodeSegment();

	code->rplpointcount = std::distance(first, last);
	code->rplpoints = MNEW(rplpoint, code->rplpointcount);

	rplpoint *rp = code->rplpoints;
	for (ForwardIt i = first; i != last; i++) {
		MachineReplacementPointInst *MI = (*i)->to_MachineReplacementPointInst();
		assert(MI);
		std::size_t offset = CS.size() - instruction_positions[MI];

		u1 *position = code->entrypoint + offset;

		// initialize rplpoint structure
		rp->method        = JD.get_jitdata()->m;
		rp->pc            = position;
		rp->regalloc      = MNEW(rplalloc, MI->op_size());
		rp->regalloccount = MI->op_size();
		rp->flags         = 0;
		rp->id            = MI->get_source_id();
		rp->patch_target_addr = NULL;

		if (MI->to_MachineDeoptInst()) {
			rp->flags |= rplpoint::FLAG_DEOPTIMIZE;
		}

		if (MI->to_MachineReplacementPointCallSiteInst()) {
			rp->callsize = instruction_sizes[MI->to_MachineReplacementPointCallSiteInst()->get_call_inst()];
		
			if (MI->to_MachineReplacementPointStaticSpecialInst()) {
				DataSegment::IdxTy idx = MI->to_MachineReplacementPointStaticSpecialInst()->get_idx();
				// The data and code segment were already copied, 
				// with the DataSegment directly starting at code->mcode
				rp->patch_target_addr = code->mcode + idx.idx;
			}
		}

		// store allocation infos
		rplalloc *ra = rp->regalloc;
		int op_index = 0;
		for (MachineInstruction::operand_iterator i = MI->begin(),
				e = MI->end(); i != e; i++) {
			MachineOperand *mop = i->op;

			ra->index = MI->get_javalocal_index(op_index);
			ra->type = convert_to_type(mop->get_type());

			Register *reg = mop->to_Register();
			if (reg) {
				// the operand has been allocated to a register
				MachineRegister *machine_reg = reg->to_MachineRegister();
				if (!machine_reg && reg->is_virtual()) {
					// XXX Somehow we end up with a ModRM operand here (??)
					//     that looks like a virtual register, but is really an address
					op_index++;
					ra++;
					continue;
				}
				assert(machine_reg);
				ra->inmemory = false;

				// XXX The `regoff` member of the `rplalloc` structure has to
				// store a register identifier that is compatible to those in
				// vm/jit/TARGET/md-abi.hpp, which is part of the baseiline
				// compiler. We somehow need to obtain such a register identifier
				// from the compiler2's `MachineRegister`. It seems that
				// `id_offset()` and `id_size()` can be used for this purpose,
				// but we had to change their visibility from `protected` to
				// `public`. Is this approach ok?
				ra->regoff = machine_reg->id_offset() / machine_reg->id_size();
			} else if (mop->is_StackSlot()) {
				// the operand has been allocated to a stack slot
				StackSlot *stack_slot = mop->to_StackSlot();
				assert(stack_slot);
				ra->inmemory = true;
				ra->regoff = stack_slot->get_index();
			} else if (mop->is_ManagedStackSlot()) {
				LOG(BoldRed << "Error: Managed Stackslot (" << mop 
				            << ") is specified in replacement point, but unhandled at the moment\n"
							<< reset_color);
				ra->inmemory = true;
			}

			op_index++;
			ra++;
		}

		LOG("resolved replacement point " << rp << nl);

		rp++;
	}
}

void CodeGenPass::finish(JITData &JD) {
	CodeMemory *CM = JD.get_CodeMemory();
	CodeSegment &CS = CM->get_CodeSegment();
	DataSegment &DS = CM->get_DataSegment();
#if 0
	s4       alignedmcodelen;
	jumpref *jr;
	s4       alignedlen;
#endif
	u1      *epoint;

	/* Get required compiler data. */

	codeinfo*     code = JD.get_jitdata()->code;
#if 0
	codegendata*  cd   = jd->cd;
	registerdata* rd   = jd->rd;
#endif

	/* Generate the method header. It simply contains a pointer to the
	   method's codeinfo structure that has to be placed at a fixed offset
	   from the end of the data segment (the offset is defined as
	   `CodeinfoPointer` in src/vm/jit/methodheader.hpp). For more info have
	   a look at the baseline compiler's `codegen_emit` function in
	   src/vm/jit/codegen-common.cpp. */

	DataFragment codeinfo_ptr = DS.get_Ref(sizeof(codeinfo *));
	// TODO unify with `InstructionEncoding::imm`
	for (int i = 0, e = sizeof(codeinfo *); i < e; i++) {
		codeinfo_ptr[i] = (reinterpret_cast<u1*>(&code))[i];
	}

	/* Link code memory */

	CM->link();

	/* calculate the code length */

	#if 0
	s4 mcodelen = (s4) (JD.get_CodeMemory()->get_end() - JD.get_CodeMemory()->get_start());
	s4 alignedmcodelen = MEMORY_ALIGN(mcodelen, MAX_ALIGN);
	s4 dseglen = (s4) JD.get_CodeMemory()->data_size();
	s4 aligneddseglen = MEMORY_ALIGN(dseglen, MAX_ALIGN);


	s4 alignedlen = alignedmcodelen + aligneddseglen;
	#endif

#if 0
	STATISTICS(count_code_len += mcodelen);
	STATISTICS(count_data_len += cd->dseglen);
#endif

	/* allocate new memory */

	// Note that the DataSegment and the CodeSegment shall be aligned!
	code->mcodelength = DS.size() + CS.size();
	code->mcode       = CNEW(u1, code->mcodelength);

	/* set the entrypoint of the method */

	assert(code->entrypoint == NULL);
	code->entrypoint = epoint = (code->mcode + DS.size());

	/* fill the data segment (code->entrypoint must already be set!) */
	MCOPY((void *) code->mcode, DS.get_start(), u1, DS.size());

	#if 0
	size_t offset = 1;
	/// @Cpp11 Could use vector::data()
	for (CodeMemory::const_data_iterator i = JD.get_CodeMemory()->data_begin(),
			e = JD.get_CodeMemory()->data_end() ; i != e; ++i) {
		u1* ptr = epoint - offset++;
		*ptr = *i;
		LOG3("" << ptr
		  << ": " << hex << *i << " " << *ptr << dec << nl);
		assert(ptr >= code->mcode);
	}
	#endif

	LOG2("mcode: " << code->mcode << nl);
	LOG2("entry: " << code->entrypoint << nl);

	if (DEBUG_COND_N(2)) {
		for(u8 *ptr = reinterpret_cast<u8*>(code->mcode),
				*e = reinterpret_cast<u8*>(code->entrypoint); ptr < e; ++ptr) {
			LOG2("" << setz(16) << hex << ptr
			  << ": " << setz(16) << (u8)*ptr << dec << nl);
		}
	}

	STATISTICS(compiler_last_codesize = code->mcodelength);

#if 0
	STATISTICS(count_code_len += mcodelen);
	STATISTICS(count_data_len += cd->dseglen);
#endif


#if 0
	dseg_finish(jd);
#endif

	/* copy code to the new location */

	CS.reverse();
	MCOPY((void *) code->entrypoint, CS.get_start(), u1, CS.size());

	/* Fill runtime information about generated code. */

#if 0
	code->stackframesize     = cd->stackframesize;
	code->synchronizedoffset = rd->memuse * 8;
	code->savedintcount      = INT_SAV_CNT - rd->savintreguse;
	code->savedfltcount      = FLT_SAV_CNT - rd->savfltreguse;
#else
	code->stackframesize     = JD.get_StackSlotManager()->get_frame_size() / SIZE_OF_STACKSLOT;
	code->synchronizedoffset = 0;
	code->savedintcount      = 0;
	code->savedfltcount      = 0;
#endif
#if defined(HAS_ADDRESS_REGISTER_FILE)
	code->savedadrcount      = ADR_SAV_CNT - rd->savadrreguse;
#endif

#if defined(__X86_64__)
	// Stackslots are 16 byte aligned
	code->stackframesize += (code->stackframesize % 2);
#endif

#if defined(__AARCH64__) || defined(__X86_64__)
	// Since we use the EnterInst on method entry, the generated code saves the
	// frame pointer (register RBP) of the calling method on the stack. We
	// therefore have to provide an additional stack slot.
	code->stackframesize++;
	code_flag_using_frameptr(code);
#endif

#if defined(__X86_64__)
	code_unflag_leafmethod(code);
#endif

#if defined(__AARCH64__)
	// We do not handle leaf methods yet in the aarch64 compiler2 backend
	// During replacement, assumptions are made on the stacklayout regarding
	// leaf methods that do not hold up when using the compiler2 backend
	code_unflag_leafmethod(code);

	// On aarch64 the replacement code also assumes that the stacksize includes
	// the return address
	code->stackframesize++;
#endif

	/* Create the exception table. */

	exceptiontable_create(JD.get_jitdata());


	/* Create the linenumber table. */

	code->linenumbertable = new LinenumberTable(JD.get_jitdata());

#if 0
	/* jump table resolving */
	for (jr = cd->jumpreferences; jr != NULL; jr = jr->next)
		*((functionptr *) ((ptrint) epoint + jr->tablepos)) =
			(functionptr) ((ptrint) epoint + (ptrint) jr->target->mpc);

#endif
	/* patcher resolving */

	patcher_resolve(code);
	LOG2("Patchers:" << nl);
	#if !defined(NDEBUG)
	DEBUG2(patcher_list_show(code));
	#endif
	/* replacement point resolving */
	{
		MInstListTy rplpoints;
		MachineInstructionSchedule *MIS = get_Artifact<LIRInstructionScheduleArtifact>()->MIS;
		find_all_replacement_points(MIS, std::back_inserter(rplpoints));
		resolve_replacement_points(rplpoints.begin(), rplpoints.end(), JD);
	}

	/* Insert method into methodtree to find the entrypoint. */

	methodtree_insert(code->entrypoint, code->entrypoint + CS.size());

#if defined(__I386__) || defined(__X86_64__) || defined(__XDSPCORE__) || defined(__M68K__) || defined(ENABLE_INTRP)
	/* resolve data segment references */

#if 0
	dseg_resolve_datareferences(jd);
#endif
#endif

	/* flush the instruction and data caches */

	md_cacheflush(code->mcode, code->mcodelength);

#ifdef ENABLE_LOGGING
	if (print_data) {
		dbg() << nl << "Data Segment: " << *JD.get_Method() << hex;
		print_hex(dbg(), code->mcode, code->entrypoint);
	}
	if (print_code) {
		dbg() << nl << "Code Segment: " << *JD.get_Method() << hex;
		print_hex(dbg(), code->entrypoint, code->mcode + code->mcodelength);
	}
#endif

}

// pass usage
PassUsage& CodeGenPass::get_PassUsage(PassUsage &PU) const {
	PU.provides<CodeGenPass>();
	PU.requires<LIRInstructionScheduleArtifact>();
	PU.requires<SSADeconstructionPass>();
	PU.requires<RegisterAssignmentPass>();
	return PU;
}

// registrate Pass
static PassRegistry<CodeGenPass> X("CodeGenPass");
static ArtifactRegistry<CodeGenPass> Y("CodeGenPass");

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
