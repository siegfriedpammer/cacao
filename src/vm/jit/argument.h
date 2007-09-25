/* src/vm/jit/argument.h - argument passing from and to JIT methods

   Copyright (C) 2007 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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


#ifndef _VM_JIT_ARGUMENT_H
#define _VM_JIT_ARGUMENT_H

#include "config.h"

#include <stdint.h>

#include "vm/global.h"


/* function prototypes ********************************************************/

imm_union argument_jitarray_load(methoddesc *md, int32_t index,
								 uint64_t *arg_regs, uint64_t *stack);
void      argument_jitarray_store(methoddesc *md, int32_t index,
								  uint64_t *arg_regs, uint64_t *stack,
								  imm_union param);

imm_union argument_jitreturn_load(methoddesc *md, uint64_t *return_regs);
void      argument_jitreturn_store(methoddesc *md, uint64_t *return_regs,
								   imm_union ret);

#endif /* _VM_JIT_ARGUMENT_H */


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
