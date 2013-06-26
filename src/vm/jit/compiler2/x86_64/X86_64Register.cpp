/* src/vm/jit/compiler2/X86_64Register.cpp - X86_64Register

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

#include "vm/jit/compiler2/x86_64/X86_64Register.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace x86_64 {

NativeRegister RAX("RAX",0x0,false);
NativeRegister RCX("RCX",0x1,false);
NativeRegister RDX("RDX",0x2,false);
NativeRegister RBX("RBX",0x3,false);
NativeRegister RSP("RSP",0x4,false);
NativeRegister RBP("RBP",0x5,false);
NativeRegister RSI("RSI",0x6,false);
NativeRegister RDI("RDI",0x7,false);
NativeRegister R8 ("R8", 0x0,true );
NativeRegister R9 ("R9", 0x1,true );
NativeRegister R10("R10",0x2,true );
NativeRegister R11("R11",0x3,true );
NativeRegister R12("R12",0x4,true );
NativeRegister R13("R13",0x5,true );
NativeRegister R14("R14",0x6,true );
NativeRegister R15("R15",0x7,true );

NativeRegister* IntegerArgumentRegisters[] = {
&RDI, &RSI, &RDX, &RCX, &R8, &R9
};

NativeRegister XMM0 ("XMM0" ,0x0,false);
NativeRegister XMM1 ("XMM1" ,0x1,false);
NativeRegister XMM2 ("XMM2" ,0x2,false);
NativeRegister XMM3 ("XMM3" ,0x3,false);
NativeRegister XMM4 ("XMM4" ,0x4,false);
NativeRegister XMM5 ("XMM5" ,0x5,false);
NativeRegister XMM6 ("XMM6" ,0x6,false);
NativeRegister XMM7 ("XMM7" ,0x7,false);
NativeRegister XMM8 ("XMM8" ,0x0,true );
NativeRegister XMM9 ("XMM9" ,0x1,true );
NativeRegister XMM10("XMM10",0x2,true );
NativeRegister XMM11("XMM11",0x3,true );
NativeRegister XMM12("XMM12",0x4,true );
NativeRegister XMM13("XMM13",0x5,true );
NativeRegister XMM14("XMM14",0x6,true );
NativeRegister XMM15("XMM15",0x7,true );

NativeRegister* FloatArgumentRegisters[] = {
&XMM0, &XMM1, &XMM2, &XMM3, &XMM4, &XMM5, &XMM6, &XMM7
};

} // end namespace x86_64
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

