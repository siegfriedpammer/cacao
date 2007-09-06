/* src/vm/jit/argument.c - argument passing from and to JIT methods

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


#include "config.h"

#include <assert.h>
#include <stdint.h>

#include "vm/global.h"
#include "vm/vm.h"

#include "vmcore/descriptor.h"


/* argument_jitarray_load ******************************************************
 
   Returns the argument specified by index from one of the passed arrays
   and returns it.

*******************************************************************************/

imm_union argument_jitarray_load(methoddesc *md, int32_t index,
								 uint64_t *arg_regs, uint64_t *stack)
{
	imm_union  ret;
	paramdesc *pd;

	pd = &md->params[index];

	switch (md->paramtypes[index].type) {
		case TYPE_INT:
		case TYPE_ADR:
			if (pd->inmemory) {
#if (SIZEOF_VOID_P == 8)
				ret.l = (int64_t)stack[pd->index];
#else
				ret.l = *(int32_t *)(stack + pd->index);
#endif
			} else {
#if (SIZEOF_VOID_P == 8)
				ret.l = arg_regs[index];
#else
				ret.l = *(int32_t *)(arg_regs + index);
#endif
			}
			break;
		case TYPE_LNG:
			if (pd->inmemory) {
				ret.l = (int64_t)stack[pd->index];
			} else {
				ret.l = (int64_t)arg_regs[index];
			}
			break;
		case TYPE_FLT:
			if (pd->inmemory) {
				ret.l = (int64_t)stack[pd->index];
			} else {
				ret.l = (int64_t)arg_regs[index];
			}
			break;
		case TYPE_DBL:
			if (pd->inmemory) {
				ret.l = (int64_t)stack[pd->index];
			} else {
				ret.l = (int64_t)arg_regs[index];
			}
			break;
	}

	return ret;
}


/* argument_jitarray_store *****************************************************
 
   Stores the argument into one of the passed arrays at a slot specified
   by index.

*******************************************************************************/

void argument_jitarray_store(methoddesc *md, int32_t index,
							 uint64_t *arg_regs, uint64_t *stack,
							 imm_union param)
{
	paramdesc *pd;

	pd = &md->params[index];

	switch (md->paramtypes[index].type) {
		case TYPE_ADR:
			if (pd->inmemory) {
#if (SIZEOF_VOID_P == 8)
				stack[pd->index] = param.l;
#else
				assert(0);
#endif
			} else {
				arg_regs[pd->index] = param.l;
			}
			break;
		default:
			vm_abort("_array_store_param: type not implemented");
			break;
	}
}


/* argument_jitreturn_load *****************************************************

   Loads the proper return value form the return register and returns it.

*******************************************************************************/

imm_union argument_jitreturn_load(methoddesc *md, uint64_t *return_regs)
{
	imm_union ret;

	switch (md->returntype.type) {
		case TYPE_INT:
		case TYPE_ADR:
#if (SIZEOF_VOID_P == 8)
			ret.l = return_regs[0];
#else
			ret.l = *(int32_t *)return_regs;
#endif
			break;
		case TYPE_LNG:
			ret.l = *(int64_t *)return_regs;
			break;
		case TYPE_FLT:
			ret.l = *(int64_t *)return_regs;
			break;
		case TYPE_DBL:
			ret.l = *(int64_t *)return_regs;
			break;
	}

	return ret;
}


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
