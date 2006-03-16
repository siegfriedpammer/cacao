/* vm/jit/code.c - codeinfo struct for representing compiled code

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
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

   Contact: cacao@cacaojvm.org

   Authors: Edwin Steiner

   Changes:

   $Id$

*/


#include "config.h"
#include "vm/types.h"

#include <assert.h>

#include "vm/jit/code.h"
#include "mm/memory.h"
#include "vm/options.h"
#include "arch.h"

/* code_codeinfo_new ***********************************************************

   Create a new codeinfo for the given method.
   
   IN:
       m................method to create a new codeinfo for

   Note: codeinfo.m is set to m, all other fields are zeroed.

   RETURN VALUE:
       a new, initialized codeinfo, or
	   NULL if an exception occurred.
  
*******************************************************************************/

codeinfo *code_codeinfo_new(methodinfo *m)
{
	codeinfo *code;

	code = NEW(codeinfo);

	memset(code,0,sizeof(codeinfo));

	code->m = m;
	
	return code;
}

/* code_get_sync_slot_count ****************************************************

   Return the number of stack slots used for storing the synchronized object
   (and the return value around monitorExit calls) by the given code.
   
   IN:
       code.............the codeinfo of the code in question
	                    (must be != NULL)

   RETURN VALUE:
       the number of stack slots used for synchronization
  
*******************************************************************************/

int code_get_sync_slot_count(codeinfo *code)
{
	assert(code);

#ifdef USE_THREADS
	if (!checksync)
		return 0;

	if (!(code->m->flags & ACC_SYNCHRONIZED))
		return 0;

	/* XXX generalize to all archs */
#ifdef HAS_4BYTE_STACKSLOT
	return (IS_2_WORD_TYPE(code->m->parseddesc->returntype.type)) ? 2 : 1;
#else
	return 1;
#endif
#else /* !USE_THREADS */
	return 0;
#endif /* USE_THREADS */
}

/* code_get_stack_frame_size ***************************************************

   Return the number of stack slots that the stack frame of the given code
   comprises.

   IMPORTANT: The return value does *not* include the saved return address 
              slot, although it is part of non-leaf stack frames on RISC
			  architectures. The rationale behind this is that the saved
			  return address is never moved or changed by replacement, and
			  this way CISC and RISC architectures can be treated the same.
			  (See also doc/stack_frames.txt.)
   
   IN:
       code.............the codeinfo of the code in question
	                    (must be != NULL)

   RETURN VALUE:
       the number of stack slots
  
*******************************************************************************/

int code_get_stack_frame_size(codeinfo *code)
{
	int count;
	
	assert(code);

	/* XXX generalize to all archs */
#ifdef HAS_4BYTE_STACKSLOT
	count = code->memuse + code->savedintcount + 2*code->savedfltcount;
#else
	count = code->memuse + code->savedintcount + code->savedfltcount;
#endif

	count += code_get_sync_slot_count(code);

#if defined(__X86_64__)
	/* keep stack 16-byte aligned */
	if (!code->isleafmethod || opt_verbosecall)
		count |= 1;
#endif

	return count;
}

/* code_codeinfo_free **********************************************************

   Free the memory used by a codeinfo.
   
   IN:
       code.............the codeinfo to free

*******************************************************************************/

void code_codeinfo_free(codeinfo *code)
{
	if (!code)
		return;

	if (code->mcode)
		CFREE((void *) (ptrint) code->mcode, code->mcodelength);

	replace_free_replacement_points(code);

	FREE(code,codeinfo);
}

/* code_free_code_of_method ****************************************************

   Free all codeinfos of the given method
   
   IN:
       m................the method of which the codeinfos are to be freed

*******************************************************************************/

void code_free_code_of_method(methodinfo *m)
{
	codeinfo *nextcode;
	codeinfo *code;

	if (!m)
		return;
	
	nextcode = m->code;
	while (nextcode) {
		code = nextcode;
		nextcode = code->prev;
		code_codeinfo_free(code);
	}

	m->code = NULL;
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
