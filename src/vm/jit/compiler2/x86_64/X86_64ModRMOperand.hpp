/* src/vm/jit/compiler2/X86_64Register.hpp - X86_64Register

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

#ifndef _JIT_COMPILER2_X86_64MODRMOPERAND
#define _JIT_COMPILER2_X86_64MODRMOPERAND

#include "vm/jit/compiler2/x86_64/X86_64.hpp"
#include "vm/jit/compiler2/x86_64/X86_64Register.hpp"
#include "vm/jit/compiler2/MachineOperand.hpp"
#include "vm/jit/compiler2/MachineAddress.hpp"

#include "toolbox/logging.hpp"

#include "vm/jit/compiler2/alloc/vector.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace x86_64 {

struct IndexOp {
	MachineOperand *op;
	explicit IndexOp(MachineOperand *op) : op(op) {}
};

struct BaseOp {
	MachineOperand *op;
	explicit BaseOp(MachineOperand *op) : op(op) {}
};

class X86_64ModRMOperand;

class NativeAddress : public MachineAddress {
public:
	NativeAddress() {}
	virtual NativeAddress *to_NativeAddress(); 
	virtual X86_64ModRMOperand *to_X86_64ModRMOperand() = 0;
};

class X86_64ModRMOperand : public NativeAddress {
public:
	enum ScaleFactor {
          Scale1 = 0,
          Scale2 = 1,
          Scale4 = 2,
          Scale8 = 3
        };

	X86_64ModRMOperand(const BaseOp &base) 
		: disp(0)
		 ,scale(Scale1)
		 ,base86_64(NULL)
		 ,index86_64(NULL) {
		embedded_operands.push_back(EmbeddedMachineOperand(base.op));		 
	}
	X86_64ModRMOperand(const BaseOp &base, s4 disp) 
		: disp(disp) 
		 ,scale(Scale1)
		 ,base86_64(NULL)
		 ,index86_64(NULL) {
		embedded_operands.push_back(EmbeddedMachineOperand(base.op));		 
	}
	X86_64ModRMOperand(const BaseOp &base, const IndexOp &index, ScaleFactor scale, s4 disp=0) 
		: disp(disp)
		 ,scale(scale)
		 ,base86_64(NULL)
		 ,index86_64(NULL) {
		embedded_operands.push_back(EmbeddedMachineOperand(base.op));		 
		embedded_operands.push_back(EmbeddedMachineOperand(index.op));
	}
	X86_64ModRMOperand(const BaseOp &base, const IndexOp &index, Type::TypeID type, s4 disp=0) 
		: disp(disp)
		 ,scale(get_scale(type))
		 ,base86_64(NULL)
		 ,index86_64(NULL) {
		embedded_operands.push_back(EmbeddedMachineOperand(base.op));		 
		embedded_operands.push_back(EmbeddedMachineOperand(index.op));
	}
	X86_64ModRMOperand(const BaseOp &base, const IndexOp &index, s4 disp) 
		: disp(disp)
		 ,scale(Scale1) 
		 ,base86_64(NULL)
		 ,index86_64(NULL) {
		embedded_operands.push_back(EmbeddedMachineOperand(base.op));		 
		embedded_operands.push_back(EmbeddedMachineOperand(index.op));
	}

	virtual const char* get_name() const {
                return "ModRMOperand";
        }
	virtual X86_64ModRMOperand *to_X86_64ModRMOperand(); 
	virtual OStream& print(OStream &OS) const {
		if (disp)
			OS << disp;
		OS << '(';
		OS << embedded_operands[base].real->op;
		OS << ',';
		if (op_size() > index)
			OS << embedded_operands[index].real->op;
		OS << ',';
		OS << (1 << scale);
		OS << ')';
		return OS;
        }


	void prepareEmit();
	u1 getRex(const X86_64Register &reg, bool opsiz64);
	bool useSIB();
	u1 getModRM(const X86_64Register &reg);
	u1 getSIB(const X86_64Register &reg);
	bool useDisp8();
	bool useDisp32();
	s1 getDisp8();
	s1 getDisp32_1();
	s1 getDisp32_2();
	s1 getDisp32_3();
	s1 getDisp32_4();
	static ScaleFactor get_scale(Type::TypeID type); 
	static ScaleFactor get_scale(int32_t scale); 
	inline X86_64Register *getBase() {
		 return cast_to<X86_64Register>(embedded_operands[base].real->op);
	}

	inline X86_64Register *getIndex() {
		if (op_size() > index)
			return cast_to<X86_64Register>(embedded_operands[index].real->op);
		else
			return NULL;
	}

	inline s4 getDisp() {
		return disp;
	}

	inline u1 getScale() {
		return scale;
	}

private:
	const static unsigned base = 0;
	const static unsigned index = 1;
	s4 disp;
	ScaleFactor scale;
	X86_64Register *base86_64;
	X86_64Register *index86_64;
};

template <>
inline X86_64ModRMOperand* cast_to<X86_64ModRMOperand>(MachineOperand *op) {
        Address *addr = op->to_Address();
        assert(addr);
        MachineAddress *maddr = addr->to_MachineAddress();
        assert(maddr);
        NativeAddress *naddr = maddr->to_NativeAddress();
        assert(naddr);
        X86_64ModRMOperand *modrm = naddr->to_X86_64ModRMOperand();
        assert(modrm);
        return modrm;
}

} // end namespace x86_64
} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_X86_64MODRMOPERAND */


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
