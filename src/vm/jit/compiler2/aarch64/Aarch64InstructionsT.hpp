/* src/vm/jit/compiler2/aarch64/Aarch64EmitterT.hpp

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

#ifndef _INSIDE_INSTRUCTIONS_HPP
#error This header is only allowed to be included in Aarch64Instructions.hpp!
#endif

template<typename T>
void MovInst<T>::emit(Emitter& em) const {
	T dst = this->reg_res();
	T src = this->reg_op(0);

	if (dst.reg == src.reg) return;

	em.mov(dst, src);
}

template<typename T>
void StoreInst<T>::emit(Emitter& em) const {
	s4 off = this->offset();
	T src = this->reg_src();
	X base = this->reg_base();
	if (off < 0) {
		em.stur(src, base, off);
	} else {
		em.str(src, base, off);
	}
}

template<typename T>
void LoadInst<T>::emit(Emitter& em) const {
	s4 off = this->offset();
	T dst = this->reg_res();
	X base = this->reg_base();

	if (off < 0) {
		em.ldur(dst, base, off);
	} else {
		em.ldr(dst, base, off);
	}
}

template<typename T>
void AddInst<T>::emit(Emitter& em) const {
	if (this->get(0).op->is_Register() && this->get(1).op->is_Register()) {
		T dst = this->reg_res();
		T rn = this->reg_op(0);
		T rm = this->reg_op(1);

		em.add(dst, rn, rm, this->shift, this->amount);
	} else {
		ABORT_MSG("AddInst<T>::emit", "Operand not in register.");
	}
}

template<typename T>
void SubInst<T>::emit(Emitter& em) const {
	em.sub(this->reg_res(), this->reg_op(0), this->reg_op(1));
}

template<typename T>
void DivInst<T>::emit(Emitter& em) const {
	em.sdiv(this->reg_res(), this->reg_op(0), this->reg_op(1));
}

template<typename T>
void NegInst<T>::emit(Emitter& em) const {
	em.neg(this->reg_res(), this->reg_op(0));
}

template<typename T>
void MulSubInst<T>::emit(Emitter& em) const {
	em.msub(this->reg_res(), this->reg_op(0), this->reg_op(1), this->reg_op(2));
}

template<typename T>
void AndInst<T>::emit(Emitter& em) const {
	em.andd(this->reg_res(), this->reg_op(0), this->reg_op(1));
}

template<typename T>
void CSelInst<T>::emit(Emitter& em) const {
	em.csel(this->reg_res(), this->reg_op(0), this->reg_op(1), this->cond);
}

template<typename T>
void FCmpInst<T>::emit(Emitter& em) const {
	em.fcmp(this->reg_op(0), this->reg_op(1));
}

template<typename T>
void FMovInst<T>::emit(Emitter& em) const {
	T dst = this->reg_res();
	T src = this->reg_op(0);

	if (dst.reg == src.reg) return;

	em.fmov(dst, src);
}

template<typename T>
void FAddInst<T>::emit(Emitter& em) const {
	em.fadd(this->reg_res(), this->reg_op(0), this->reg_op(1));
}

template<typename T>
void FDivInst<T>::emit(Emitter& em) const {
	em.fdiv(this->reg_res(), this->reg_op(0), this->reg_op(1));
}

template<typename T>
void FMulInst<T>::emit(Emitter& em) const {
	em.fmul(this->reg_res(), this->reg_op(0), this->reg_op(1));
}

template<typename T>
void FSubInst<T>::emit(Emitter& em) const {
	em.fsub(this->reg_res(), this->reg_op(0), this->reg_op(1));
}

template<typename T>
void FNegInst<T>::emit(Emitter& em) const {
	em.fneg(this->reg_res(), this->reg_op(0));
}

template<typename O, typename R>
void FcvtInst<O, R>::emit(Emitter& em) const {
	em.fcvt(this->reg_res(), this->reg_op(0));
}

template<typename O, typename R>
void IntToFpInst<O, R>::emit(Emitter& em) const {
	em.scvtf(this->reg_res(), this->reg_op(0));
}

template<typename T>
void IntToByteInst<T>::emit(Emitter& em) const {
	em.sxtb(this->reg_res(), this->reg_op(0));
}

template<typename T>
void IntToShortInst<T>::emit(Emitter& em) const {
	em.sxth(this->reg_res(), this->reg_op(0));
}

template<typename T>
void CmpInst<T>::emit(Emitter& em) const {
	T reg0 = this->reg_op(0);

	if (this->get(1).op->is_Register()) {
		em.cmp(reg0, this->reg_op(1));
	} else if (this->get(1).op->is_Immediate()) {
		Immediate *imm = cast_to<Immediate>(this->get(1).op);
		em.cmp(reg0, imm->get_Int());
	}
}

template<typename T>
void DsegAddrInst<T>::emit(CodeMemory* cm) const {
	// Load DSeg address
	CodeFragment code = cm->get_CodeFragment(4);
	cm->require_linking(this, code);
}

template<typename T>
void DsegAddrInst<T>::link(CodeFragment &cf) const {
	CodeMemory &cm = cf.get_Segment().get_CodeMemory();
	s4 offset = cm.get_offset(data_index, cf);
	#if 0
	LOG2(this << " offset: " << offset << " data index: " << data_index.idx
	     << " CF end index: " << cf.get_end().idx << nl);
	LOG2("dseg->size " << cm.get_DataSegment().size() << " cseg->size "
		 << cm.get_CodeSegment().size() << nl);
	#endif
	assert(offset != 0);

	Emitter em;
	em.ldr(this->reg_res(), offset);
	em.emit(cf);
}

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
