/* src/vm/jit/code.c - codeinfo struct for representing compiled code

   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
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

#include "vm/types.h"

#include "arch.h"

#include "mm/memory.h"

#if defined(ENABLE_THREADS)
# include "threads/native/lock.h"
#endif

#include "vm/jit/code.h"
#include "vm/jit/codegen-common.h"
#include "vm/jit/patcher-common.h"

#include "vmcore/options.h"


/* code_init *******************************************************************

   Initialize the code-subsystem.

*******************************************************************************/

bool code_init(void)
{
	/* check for offset of code->m == 0 (see comment in code.h) */

	assert(OFFSET(codeinfo, m) == 0);

	/* everything's ok */

	return true;
}


/* code_codeinfo_new ***********************************************************

   Create a new codeinfo for the given method.
   
   IN:
       m................method to create a new codeinfo for

   The following fields are set in codeinfo:
       m
       patchers

   RETURN VALUE:
       a new, initialized codeinfo, or
	   NULL if an exception occurred.
  
*******************************************************************************/

codeinfo *code_codeinfo_new(methodinfo *m)
{
	codeinfo *code;

	code = NEW(codeinfo);

	code->m = m;

	patcher_list_create(code);

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		size_codeinfo += sizeof(codeinfo);
#endif

	return code;
}


/* code_find_codeinfo_for_pc ***************************************************

   Return the codeinfo for the compilation unit that contains the
   given PC.

   IN:
       pc...............machine code position

   RETURN VALUE:
       the codeinfo * for the given PC

*******************************************************************************/

codeinfo *code_find_codeinfo_for_pc(u1 *pc)
{
	u1 *pv;

	pv = codegen_get_pv_from_pc(pc);
	assert(pv);

	return code_get_codeinfo_for_pv(pv);
}


/* code_find_codeinfo_for_pc ***************************************************

   Return the codeinfo for the compilation unit that contains the
   given PC. This method does not check the return value and is used
   by the GC.

   IN:
       pc...............machine code position

   RETURN VALUE:
       the codeinfo * for the given PC, or NULL

*******************************************************************************/

codeinfo *code_find_codeinfo_for_pc_nocheck(u1 *pc)
{
	u1 *pv;

	pv = codegen_get_pv_from_pc_nocheck(pc);

	if (pv == NULL)
		return NULL;

	return code_get_codeinfo_for_pv(pv);
}


/* code_get_methodinfo_for_pv **************************************************

   Return the methodinfo for the given PV.

   IN:
       pv...............PV

   RETURN VALUE:
       the methodinfo *

*******************************************************************************/

methodinfo *code_get_methodinfo_for_pv(u1 *pv)
{
	codeinfo *code;

	code = code_get_codeinfo_for_pv(pv);

	/* This is the case for asm_vm_call_method. */

	if (code == NULL)
		return NULL;

	return code->m;
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

#if defined(ENABLE_REPLACEMENT)
int code_get_sync_slot_count(codeinfo *code)
{
#ifdef ENABLE_THREADS
	int count;
	
	assert(code);

	if (!checksync)
		return 0;

	if (!code_is_synchronized(code))
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

#else /* !ENABLE_THREADS */
	
	return 0;

#endif /* ENABLE_THREADS */
}
#endif /* defined(ENABLE_REPLACEMENT) */


/* code_codeinfo_free **********************************************************

   Free the memory used by a codeinfo.
   
   IN:
       code.............the codeinfo to free

*******************************************************************************/

void code_codeinfo_free(codeinfo *code)
{
	if (code == NULL)
		return;

	if (code->mcode != NULL)
		CFREE((void *) (ptrint) code->mcode, code->mcodelength);

	patcher_list_free(code);

#if defined(ENABLE_REPLACEMENT)
	replace_free_replacement_points(code);
#endif

	FREE(code, codeinfo);

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		size_codeinfo -= sizeof(codeinfo);
#endif
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
