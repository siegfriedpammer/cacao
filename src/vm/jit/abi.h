/* src/vm/jit/abi.h - common ABI defines

   Copyright (C) 1996-2013
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


#ifndef _ABI_H
#define _ABI_H

#include "config.h"
#include "vm/types.hpp"

#include "arch.h"

#include "vm/jit/abi-asm.h"
#include "vm/jit/jit.hpp"
#include "vm/jit/stack.hpp"


/* ABI externs ****************************************************************/

extern s4 nregdescint[];
extern char *regs[];
extern s4 nregdescfloat[];

#if defined(HAS_ADDRESS_REGISTER_FILE)
extern s4 nregdescadr[];

extern const char *abi_registers_address_name[];
extern const s4    abi_registers_address_argument[];
extern const s4    abi_registers_address_saved[];
extern const s4    abi_registers_address_temporary[];
#endif

extern const char *abi_registers_integer_name[];
extern const s4    abi_registers_integer_argument[];
extern const s4    abi_registers_integer_saved[];
extern const s4    abi_registers_integer_temporary[];

extern const char *abi_registers_float_name[];
extern const s4    abi_registers_float_argument[];
extern const s4    abi_registers_float_saved[];
extern const s4    abi_registers_float_temporary[];


/* function prototypes ********************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* machine dependent descriptor function */
void md_param_alloc(methoddesc *md);
void md_param_alloc_native(methoddesc *md);

/* machine dependent return value handling function */
void md_return_alloc(jitdata *jd, stackelement_t *stackslot);

#ifdef __cplusplus
}
#endif

#endif /* _ABI_H */


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
