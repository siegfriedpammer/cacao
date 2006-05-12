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

   The following fields are set in codeinfo:
       m
	   isleafmethod
   all other fields are zeroed

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
	code->isleafmethod = m->isleafmethod; /* XXX will be moved to codeinfo */
	
	return code;
}

/* code_get_sync_slot_count ****************************************************

   Return the number of stack slots used for storing the synchronized object
   (and the return value around lock_monitor_exit calls) by the given code.
   
   IN:
       code.............the codeinfo of the code in question
	                    (must be != NULL)

   RETURN VALUE:
       the number of stack slots used for synchronization
  
*******************************************************************************/

int code_get_sync_slot_count(codeinfo *code)
{
#ifdef USE_THREADS
	int count;
	
	assert(code);

	if (!checksync)
		return 0;

	if (!(code->m->flags & ACC_SYNCHRONIZED))
		return 0;

	count = 1;

#ifdef HAS_4BYTE_STACKSLOT
	/* long and double need 2 4-byte slots */
	if (IS_2_WORD_TYPE(code->m->parseddesc->returntype.type))
		count++;
#endif

#if defined(__POWERPC__)
	/* powerpc needs an extra slot */
	count++;
#endif

	return count;

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

	/* slots allocated by register allocator plus saved registers */

#ifdef HAS_4BYTE_STACKSLOT
	count = code->memuse + code->savedintcount + 2*code->savedfltcount;
#else
	count = code->memuse + code->savedintcount + code->savedfltcount;
#endif

	/* add slots needed in synchronized methods */

	count += code_get_sync_slot_count(code);

	/* keep stack aligned */

#if defined(__X86_64__)
	/* the x86_64 codegen only aligns the stack in non-leaf methods */
	if (!code->isleafmethod || opt_verbosecall)
		count |= 1; /* even when return address is added */
#endif

	/* XXX align stack on alpha */
#if defined(__MIPS__)
	if (code->isleafmethod)
		count = (count + 1) & ~1;
	else
		count |= 1; /* even when return address is added */
#endif

#if defined(__POWERPC__)
	/* keep stack 16-byte aligned */
	count = (count + 3) & ~3;
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
