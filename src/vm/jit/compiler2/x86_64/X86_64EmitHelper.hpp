/* src/vm/jit/compiler2/X86_64EmitHelper.hpp - X86_64EmitHelper

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

#ifndef _JIT_COMPILER2_X86_64EMITHELPER
#define _JIT_COMPILER2_X86_64EMITHELPER

#include "vm/jit/compiler2/x86_64/X86_64Register.hpp"
#include "vm/jit/compiler2/CodeMemory.hpp"
#include "vm/jit/compiler2/alloc/deque.hpp"
#include "vm/jit/compiler2/StackSlotManager.hpp"
#include "vm/types.hpp"
#include "toolbox/logging.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace x86_64 {

/**
 * Opcode ref field.
 */
struct OpReg {
	X86_64Register *reg;
	OpReg(X86_64Register *reg) : reg(reg) {}
	X86_64Register* operator->() const {
		return reg;
	}
	X86_64Register* operator*() const {
		return reg;
	}
};

/**
 * REX Prefix Builder.
 */
struct REX {
	enum Field {
		W = 3, ///< 0 = Operand size determined by CS.D
			   ///< 1 = 64 Bit Operand Size
		R = 2, ///< Extension of the ModR/M reg field
		X = 1, ///< Extension of the SIB index field
		B = 0  ///< Extension of the ModR/M r/m field, SIB base field, or Opcode reg field
	};
	u1 rex;

	REX() : rex(0x4 << 4) {}

	/// convert to u1
	operator u1() {
		return rex;
	}
	/**
	 * @note (binary) operator+ is evaluated left to right
	 */
	REX& operator+(Field f) {
		rex |= 1 << f;
		return *this;
	}
	/**
	 * @note (binary) operator- is evaluated left to right
	 */
	REX& operator-(Field f) {
		rex &= ~(1 << f);
		return *this;
	}
	REX& operator+(OpReg reg) {
		if (reg->extented)
			rex |= 1 << B;
		return *this;
	}
};

inline u1 get_rex(X86_64Register *reg, X86_64Register *rm = NULL,
		bool opsiz64 = true) {
	const unsigned rex_w = 3;
	const unsigned rex_r = 2;
	//const unsigned rex_x = 1;
	const unsigned rex_b = 0;

	u1 rex = 0x40;

	// 64-bit operand size
	if (opsiz64) {
		rex |= (1 << rex_w);
	}

	if (reg && reg->extented) {
		rex |= (1 << rex_r);
	}
	if (rm && rm->extented) {
		rex |= (1 << rex_b);
	}
	return rex;
}

inline u1 get_rex(const X86_64Register *reg1, X86_64Register *reg2 = NULL, 
			GPInstruction::OperandSize op_size = GPInstruction::OS_32, 
			X86_64Register *reg3 = NULL) {
	const unsigned rex_w = 3;
	const unsigned rex_r = 2;
	const unsigned rex_x = 1;
	const unsigned rex_b = 0;

	u1 rex = 0x40;

	// 64-bit operand size
	if (op_size == GPInstruction::OS_64) {
		rex |= (1 << rex_w);
	}
	if (reg1 && reg1->extented) {
		rex |= (1 << rex_r);
	}
	if (reg2 && reg2->extented) {
		rex |= (1 << rex_b);
	}
	if (reg3 && reg3->extented) {
		rex |= (1 << rex_x);
	}
	return rex;

}

inline bool use_sib(X86_64Register *base, X86_64Register *index) {
	return index || base == &RSP || base == &R12;
}

inline u1 get_modrm(u1 reg, u1 base, s4 disp, bool use_sib = false) {
	u1 modrm_mod = 6;
	u1 modrm_reg = 3;
	u1 modrm_rm = 0;

	u1 mod = 0;
	u1 rm = 0;
	u1 modrm = 0;

	if (disp == 0 || base == 0x05 /* RBP */) {
		// no disp
		mod = 0x00; //0b00
	}
	else if (fits_into<s1>(disp)) {
		// disp8
		mod = 0x01; // 0b01
	} else if (fits_into<s4>(disp)) {
		// disp32
		mod = 0x02; // 0b10
	}
	else {
		ABORT_MSG("Illegal displacement", "Displacement: "<<disp);
	}

	if (use_sib) {
		rm = 0x04; // 0b100
	}
	else {
		rm = base;
	}

	modrm = mod << modrm_mod;
	modrm |= reg << modrm_reg;
	modrm |= rm << modrm_rm;

	return modrm;
}

inline u1 get_modrm (X86_64Register *reg, X86_64Register *base, s4 disp, bool use_sib = false) {
	return get_modrm((reg != NULL) ? reg->get_index() : 0, (base != NULL) ? base->get_index() : 0, disp, use_sib);
}

inline u1 get_sib(X86_64Register *base, X86_64Register *index = NULL, u1 scale = 1) {
	u1 sib_scale = 6;
	u1 sib_index = 3;
	u1 sib_base = 0;

	u1 sib = 0;

	sib = scale << sib_scale;
	if (index) {
		sib |= index->get_index() << sib_index;
	}
	else {
		sib |= RSP.get_index() << sib_index;
	}

	sib |= base->get_index() << sib_base;

	return sib;
}


inline u1 get_modrm_u1(u1 mod, u1 reg, u1 rm) {
	const unsigned modrm_mod = 6;
	const unsigned modrm_reg = 3;
	const unsigned modrm_rm = 0;

	u1 modrm = (0x3 & mod) << modrm_mod;
	modrm |= (0x7 & reg) << modrm_reg;
	modrm |= (0x7 & rm) << modrm_rm;

	return modrm;
}
inline u1 get_modrm(u1 mod, X86_64Register *reg, X86_64Register *rm) {
	return get_modrm_u1(mod,reg->get_index(), rm->get_index());
}
inline u1 get_modrm_reg2reg(X86_64Register *reg, X86_64Register *rm) {
	return get_modrm(0x3,reg,rm);
}
inline u1 get_modrm_1reg(u1 reg, X86_64Register *rm) {
	return get_modrm_u1(0x3,reg,rm->get_index());
}

int get_stack_position(MachineOperand *op) {
	if (op->is_StackSlot()) {
		StackSlot *slot = op->to_StackSlot();
		return slot->get_index() * 8;
	} else if (op->is_ManagedStackSlot()) {
		ManagedStackSlot *slot = op->to_ManagedStackSlot();
		StackSlotManager *SSM = slot->get_parent();
		return -(SSM->get_number_of_machine_slots() - slot->get_index()) * 8;
	}
	assert(false && "Not a stackslot");
}

class CodeSegmentBuilder {
private:
	typedef alloc::deque<u1>::type Container;
public:
	typedef Container::iterator iterator;
	typedef Container::const_iterator const_iterator;
	typedef Container::value_type value_type;

	void push_front(u1 d) { data.push_front(d); }
	void push_back(u1 d) { data.push_back(d); }
	CodeSegmentBuilder& operator+=(u1 d) {
		push_back(d);
		return *this;
	}
	u1& operator[](std::size_t i) {
		if (i >= size()) {
			data.resize(i+1,0);
		}
		return data[i];
	}
	u1 operator[](std::size_t i) const {
		assert(i < size());
		return data[i];
	}
	std::size_t size() const { return data.size(); }
	const_iterator begin() const { return data.begin(); }
	const_iterator end()   const { return data.end(); }
	iterator begin() { return data.begin(); }
	iterator end()   { return data.end(); }
private:
	Container data;
};

void add_CodeSegmentBuilder(CodeMemory *CM, const CodeSegmentBuilder &CSB) {
	CodeFragment CF = CM->get_CodeFragment(CSB.size());
	for (std::size_t i = 0, e = CSB.size(); i < e; ++i) {
		CF[i] = CSB[i];
	}
}

template <>
inline StackSlot* cast_to<StackSlot>(MachineOperand *op) {
	switch (op->get_OperandID()) {
	case MachineOperand::ManagedStackSlotID:
		{
			ManagedStackSlot *mslot = op->to_ManagedStackSlot();
			assert(mslot);
			StackSlot *slot = mslot->to_StackSlot();
			assert(slot);
			return slot;
		}
	case MachineOperand::StackSlotID:
		{
			StackSlot *slot = op->to_StackSlot();
			assert(slot);
			return slot;
		}
	default: break;
	}
	assert(0 && "Not a stackslot");
	return NULL;
}

template <>
inline Immediate* cast_to<Immediate>(MachineOperand *op) {
	Immediate* imm = op->to_Immediate();
	assert(imm);
	return imm;
}

struct InstructionEncoding {
	template <class T>
	static void reg2reg(CodeMemory *CM, T opcode,
			X86_64Register *reg, X86_64Register *rm) {
		CodeFragment code = CM->get_CodeFragment(2 + sizeof(T));

		code[0] = get_rex(reg,rm);

		for (int i = 0, e = sizeof(T) ; i < e ; ++i) {
			code[i + 1] = (u1) 0xff & (opcode >> (8 * (e - i - 1)));
		}

		code[1 + sizeof(T)] = get_modrm_reg2reg(reg,rm);
	}
	template <class T>
	static void reg2rbp_disp8(CodeMemory *CM, T opcode,
			X86_64Register *reg, s1 disp) {
		CodeFragment code = CM->get_CodeFragment(3 + sizeof(T));

		code[0] = get_rex(reg);

		for (int i = 0, e = sizeof(T) ; i < e ; ++i) {
			code[i + 1] = (u1) 0xff & (opcode >> (8 * (e - i - 1)));
		}

		code[1 + sizeof(T)] = get_modrm(0x1,reg,&RBP);
		code[2 + sizeof(T)] = u1(disp);
	}
	template <class T>
	static void reg2rbp_disp32(CodeMemory *CM, T opcode,
			X86_64Register *reg, s4 disp) {
		CodeFragment code = CM->get_CodeFragment(6 + sizeof(T));

		code[0] = get_rex(reg);

		for (int i = 0, e = sizeof(T) ; i < e ; ++i) {
			code[i + 1] = (u1) 0xff & (opcode >> (8 * (e - i - 1)));
		}

		code[1 + sizeof(T)] = get_modrm(0x2,reg,&RBP);
		code[2 + sizeof(T)] = u1( 0xff & (disp >> (0 * 8)));
		code[3 + sizeof(T)] = u1( 0xff & (disp >> (1 * 8)));
		code[4 + sizeof(T)] = u1( 0xff & (disp >> (2 * 8)));
		code[5 + sizeof(T)] = u1( 0xff & (disp >> (3 * 8)));
	}
	template <class O,class I>
	static void reg2imm(CodeMemory *CM, O opcode,
			X86_64Register *reg, I imm) {
		CodeFragment code = CM->get_CodeFragment(1 + sizeof(O) + sizeof(I));

		code[0] = get_rex(reg);

		for (int i = 0, e = sizeof(O) ; i < e ; ++i) {
			code[i + 1] = (u1) 0xff & (opcode >> (8 * (e - i - 1)));
		}
		for (int i = 0, e = sizeof(I) ; i < e ; ++i) {
			code[i + sizeof(O) + 1] = (u1) 0xff & (imm >> (8 * i));
		}

	}
	template <class O,class I>
	static void reg2imm_modrm(CodeMemory *CM, O opcode,
			X86_64Register* reg, X86_64Register *rm, I imm) {
		CodeFragment code = CM->get_CodeFragment(2 + sizeof(O) + sizeof(I));

		code[0] = get_rex(reg, rm);

		for (int i = 0, e = sizeof(O) ; i < e ; ++i) {
			code[i + 1] = (u1) 0xff & (opcode >> (8 * (e - i - 1)));
		}
		code[1 + sizeof(O)] = get_modrm_1reg(reg->get_index(),rm);
		for (int i = 0, e = sizeof(I) ; i < e ; ++i) {
			code[i + sizeof(O) + 2] = (u1) 0xff & (imm >> (8 * i));
		}

	}
	template <class O,class I>
	static void imm_op(CodeMemory *CM, O opcode, I imm) {
		CodeFragment code = CM->get_CodeFragment(sizeof(O) + sizeof(I));

		for (int i = 0, e = sizeof(O) ; i < e ; ++i) {
			code[i] = (u1) 0xff & (opcode >> (8 * (e - i - 1)));
		}
		for (int i = 0, e = sizeof(I) ; i < e ; ++i) {
			code[i + sizeof(O)] = (u1) 0xff & (imm >> (8 * i));
		}

	}
	template <class O,class I>
	static void imm_op(CodeFragment &code, O opcode, I imm) {
		assert(code.size() == (sizeof(O) + sizeof(I)));

		for (int i = 0, e = sizeof(O) ; i < e ; ++i) {
			code[i] = (u1) 0xff & (opcode >> (8 * (e - i - 1)));
		}
		for (int i = 0, e = sizeof(I) ; i < e ; ++i) {
			code[i + sizeof(O)] = (u1) 0xff & (imm >> (8 * i));
		}

	}
	template <class I,class Seg>
	static void imm(Seg seg,I imm) {
		assert(seg.size() == sizeof(I));

		for (int i = 0, e = sizeof(I) ; i < e ; ++i) {
			seg[i] = (u1) 0xff & *(reinterpret_cast<u1*>(&imm) + i);
		}

	}

	static void emit (CodeMemory* CM, u1 primary_opcode, GPInstruction::OperandSize op_size, MachineOperand *src, MachineOperand *dst, u1 secondary_opcode = 0, u1 op_reg = 0, u1 prefix = 0, bool prefix_0f = false, bool encode_dst = true, bool imm_sign_extended = false) {
		CodeSegmentBuilder code;
		if (dst->is_Register()) {
			X86_64Register *dst_reg = cast_to<X86_64Register>(dst);
			if (src->is_Register()) {
				X86_64Register *src_reg = cast_to<X86_64Register>(src);
				u1 rex = get_rex(dst_reg,src_reg,op_size == GPInstruction::OS_64);
				if (rex != 0x40)
					code += rex;
				if (prefix)
					code += prefix;
				if (prefix_0f)
					code += 0x0F;
				code += primary_opcode;
				if (secondary_opcode != 0x00) {
					code += secondary_opcode;
				}
				code += get_modrm_reg2reg (dst_reg,src_reg);
			}
			else if (src->is_Immediate()) {
				Immediate *src_imm = cast_to<Immediate>(src);
				u1 rex = get_rex(NULL,dst_reg,op_size == GPInstruction::OS_64);
				if (rex != 0x40)
					code += rex;
				if (prefix)
					code += prefix;
				if (prefix_0f)
					code += 0x0F;
				if (!encode_dst)
					primary_opcode += dst_reg->get_index();
				code += primary_opcode;
				if (secondary_opcode != 0x00) {
					code += secondary_opcode;
				}
				if (encode_dst) {
					code += get_modrm_1reg(op_reg,dst_reg);
				}
				if (!imm_sign_extended) {
					switch(op_size) {
						case GPInstruction::OS_8:
						{
							s1 immval = src_imm->get_value<s1>();
							code += (u1) 0xff & (immval >> 0x00);
							break;
						}
						case GPInstruction::OS_16:
						{
							s2 immval = src_imm->get_value<s2>();
							code += (u1) 0xff & (immval >> 0x00);
							code += (u1) 0xff & (immval >> 0x08);
							break;
						}
						case GPInstruction::OS_32:
						{
							s4 immval = src_imm->get_value<s4>();
							code += (u1) 0xff & (immval >> 0x00);
							code += (u1) 0xff & (immval >> 0x08);
							code += (u1) 0xff & (immval >> 0x10);
							code += (u1) 0xff & (immval >> 0x18);
							break;
						}
						case GPInstruction::OS_64:
						{
							s8 immval = src_imm->get_value<s8>();
							code += (u1) 0xff & (immval >> 0x00);
							code += (u1) 0xff & (immval >> 0x08);
							code += (u1) 0xff & (immval >> 0x10);
							code += (u1) 0xff & (immval >> 0x18);
							code += (u1) 0xff & (immval >> 0x20);
							code += (u1) 0xff & (immval >> 0x28);
							code += (u1) 0xff & (immval >> 0x30);
							code += (u1) 0xff & (immval >> 0x38);
							break;
						}
						case GPInstruction::NO_SIZE:
							ABORT_MSG("Invalid Immediate Size",
								"dst: " << dst_reg << " src: " << src_imm << " op_code: " << primary_opcode);
							break;
					}
				}
				else {
					if (fits_into<s1>(src_imm->get_value<s8>())) {	
						s1 immval = src_imm->get_value<s1>();
						code += (u1) 0xff & (immval >> 0x00);
					}	
					else if (fits_into<s2>(src_imm->get_value<s8>())) {
						s2 immval = src_imm->get_value<s2>();
						code += (u1) 0xff & (immval >> 0x00);
						code += (u1) 0xff & (immval >> 0x08);
					}
					else if (fits_into<s4>(src_imm->get_value<s8>())) {
						s4 immval = src_imm->get_value<s4>();
						code += (u1) 0xff & (immval >> 0x00);
						code += (u1) 0xff & (immval >> 0x08);
						code += (u1) 0xff & (immval >> 0x10);
						code += (u1) 0xff & (immval >> 0x18);
					}
					else {
						s8 immval = src_imm->get_value<s8>();
						code += (u1) 0xff & (immval >> 0x00);
						code += (u1) 0xff & (immval >> 0x08);
						code += (u1) 0xff & (immval >> 0x10);
						code += (u1) 0xff & (immval >> 0x18);
						code += (u1) 0xff & (immval >> 0x20);
						code += (u1) 0xff & (immval >> 0x28);
						code += (u1) 0xff & (immval >> 0x30);
						code += (u1) 0xff & (immval >> 0x38);
					}
				}
			}
			else if (src->is_stackslot()) {
				s4 index = get_stack_position(src);
				u1 rex = get_rex(dst_reg,NULL,op_size == GPInstruction::OS_64);
				if (rex != 0x40)
					code += rex;
				if (prefix)
					code += prefix;
				if (prefix_0f)
					code += 0x0F;
				code += primary_opcode;
				if (secondary_opcode != 0x00) {
					code += secondary_opcode;
				}
				if (fits_into<s1>(index)) {
					// Shouldn't mod be 0x0 for RIP relative addressing? needs test
					code += get_modrm(0x1,dst_reg,&RBP);
					code += index;
				} else {
					code += get_modrm(0x2,dst_reg,&RBP);
					code += (u1) 0xff & (index >> 0x00);
					code += (u1) 0xff & (index >> 0x08);
					code += (u1) 0xff & (index >> 0x10);
					code += (u1) 0xff & (index >> 0x18);
				}
			}
			else if (src->is_Address()) {
				X86_64ModRMOperand *src_mod = cast_to<X86_64ModRMOperand>(src);
				X86_64Register *base = src_mod->getBase();
				X86_64Register *index = src_mod->getIndex();
				s4 disp = src_mod->getDisp();
				u1 scale = src_mod->getScale();
				u1 rex = get_rex(dst_reg, base, op_size, index);
				if (rex != 0x40)
					code += rex;
				if (prefix)
					code += prefix;
				if (prefix_0f)
					code += 0x0F;
				code += primary_opcode;
				if (secondary_opcode != 0x00) {
					code += secondary_opcode;
				}
				bool sib = use_sib(base, index);
				code += get_modrm(dst_reg,base,disp,sib);
				if (sib) {
					code += get_sib(base,index,scale);
				}
				if (disp != 0) {
					if (fits_into<s1>(disp) && base != &RBP) {
						code += (s1)disp;
					}
					else {
						code += (u1) 0xff & (disp >> 0x00);
						code += (u1) 0xff & (disp >> 0x08);
						code += (u1) 0xff & (disp >> 0x10);
						code += (u1) 0xff & (disp >> 0x18);
					}
				}
			}
			else {
				ABORT_MSG("Operand(s) not supported",
						"dst_reg: " << dst_reg << " src: " << src << " op_code: " << primary_opcode);
			}
		}
		else if (dst->is_stackslot()) {
			s4 index = get_stack_position(dst);
			if (src->is_Register()) {
				X86_64Register *src_reg = cast_to<X86_64Register>(src);
				u1 rex = get_rex(src_reg,NULL,op_size == GPInstruction::OS_64);
				if (rex != 0x40)
					code += rex;
				if (prefix)
					code += prefix;
				if (prefix_0f)
					code += 0x0F;
				code += primary_opcode;
				if (secondary_opcode != 0x00) {
					code += secondary_opcode;
				}
				if (fits_into<s1>(index)) {
					// Shouldn't mod be 0x0 for RIP relative addressing? needs test
					code += get_modrm(0x1,src_reg,&RBP);
					code += index;
				} else {
					code += get_modrm(0x2,src_reg,&RBP);
					code += (u1) 0xff & (index >> 0x00);
					code += (u1) 0xff & (index >> 0x08);
					code += (u1) 0xff & (index >> 0x10);
					code += (u1) 0xff & (index >> 0x18);
				}
			}
			else {
				ABORT_MSG("Operand(s) not supported",
						"dst: " << dst << " src: " << src << " op_code: " << primary_opcode);
			}
		}
		else if (dst->is_Address()) {
			X86_64ModRMOperand *dst_mod = cast_to<X86_64ModRMOperand>(dst);
			X86_64Register *base = dst_mod->getBase();
			X86_64Register *index = dst_mod->getIndex();
			s4 disp = dst_mod->getDisp();
			u1 scale = dst_mod->getScale();
			if (src->is_Register()) {
				X86_64Register *src_reg = cast_to<X86_64Register>(src);
				u1 rex = get_rex(src_reg, base, op_size, index);
				if (rex != 0x40)
					code += rex;
				if (prefix)
					code += prefix;
				if (prefix_0f)
					code += 0x0F;
				code += primary_opcode;
				if (secondary_opcode != 0x00) {
					code += secondary_opcode;
				}
				bool sib = use_sib(base, index);
				code += get_modrm(src_reg,base,disp,sib);
				if (sib) {
					code += get_sib(base,index,scale);
				}
				if (disp != 0) {
					if (fits_into<s1>(disp) && base != &RBP) {
						code += (s1)disp;
					}
					else {
						code += (u1) 0xff & (disp >> 0x00);
						code += (u1) 0xff & (disp >> 0x08);
						code += (u1) 0xff & (disp >> 0x10);
						code += (u1) 0xff & (disp >> 0x18);
					}
				}
			}
			else if (src->is_Immediate()) {
				Immediate *src_imm = cast_to<Immediate>(src);
				u1 rex = get_rex(NULL, base, op_size, index);
				if (rex != 0x40)
					code += rex;
				if (prefix)
					code += prefix;
				if (prefix_0f)
					code += 0x0F;
				code += primary_opcode;
				if (secondary_opcode != 0x00) {
					code += secondary_opcode;
				}
				bool sib = use_sib(base, index);
				code += get_modrm(op_reg,base->get_index(),disp,sib);
				if (sib) {
					code += get_sib(base,index,scale);
				}
				if (disp != 0) {
					if (fits_into<s1>(disp) && base != &RBP) {
						code += (s1)disp;
					}
					else {
						code += (u1) 0xff & (disp >> 0x00);
						code += (u1) 0xff & (disp >> 0x08);
						code += (u1) 0xff & (disp >> 0x10);
						code += (u1) 0xff & (disp >> 0x18);
					}
				}
				switch(op_size) {
					case GPInstruction::OS_8:
					{
						s1 immval = src_imm->get_value<s1>();
						code += (u1) 0xff & (immval >> 0x00);
						break;
					}
					case GPInstruction::OS_16:
					{
						s2 immval = src_imm->get_value<s2>();
						code += (u1) 0xff & (immval >> 0x00);
						code += (u1) 0xff & (immval >> 0x08);
						break;
					}
					case GPInstruction::OS_32:
					{
						s4 immval = src_imm->get_value<s4>();
						code += (u1) 0xff & (immval >> 0x00);
						code += (u1) 0xff & (immval >> 0x08);
						code += (u1) 0xff & (immval >> 0x10);
						code += (u1) 0xff & (immval >> 0x18);
						break;
					}
					case GPInstruction::OS_64:
					{
						if (disp != 0) {
							ABORT_MSG("Invalid Immediate Size (imm64 with disp not allowed)",
								"dst: " << dst_mod << " src: " << src_imm << " op_code: " << primary_opcode);
						}
						s8 immval = src_imm->get_value<s8>();
						code += (u1) 0xff & (immval >> 0x00);
						code += (u1) 0xff & (immval >> 0x08);
						code += (u1) 0xff & (immval >> 0x10);
						code += (u1) 0xff & (immval >> 0x18);
						code += (u1) 0xff & (immval >> 0x20);
						code += (u1) 0xff & (immval >> 0x28);
						code += (u1) 0xff & (immval >> 0x30);
						code += (u1) 0xff & (immval >> 0x38);
						break;
					}
					case GPInstruction::NO_SIZE:
						ABORT_MSG("Invalid Immediate Size",
							"dst: " << dst_mod << " src: " << src_imm << " op_code: " << primary_opcode);
						break;
				}
			}
			else {
				ABORT_MSG("Operand(s) not supported",
						"dst: " << dst_mod << " src: " << src << " op_code: " << primary_opcode);
			}
		}
		else {
			ABORT_MSG("Operand(s) not supported!",
					"dst: " << dst << " src: " << src << " op_code: " << primary_opcode);
		}
			

		add_CodeSegmentBuilder(CM,code);
	}

};
#if 0
template <>
static void InstructionEncoding::reg2reg<u1>(
		CodeMemory *CM, u1 opcode,
		X86_64Register *src, X86_64Register *dst) {
	CodeFragment code = CM->get_CodeFragment(3);
	code[0] = get_rex(src,dst);
	code[1] = opcode;
	code[2] = get_modrm_reg2reg(src,dst);
}
#endif

} // end namespace x86_64
} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_X86_64EMITHELPER */


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
