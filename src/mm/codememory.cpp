/* src/mm/codememory.c - code memory management

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


#include "config.h"

#include <stdint.h>
#include <stdlib.h>

#include "threads/mutex.hpp"
#include "threads/thread.hpp"

#include "mm/codememory.hpp"
#include "mm/memory.hpp"

#include "vm/global.hpp"
#include "vm/options.hpp"
#include "vm/os.hpp"
#include "vm/statistics.hpp"

/* global code memory variables ***********************************************/

#define DEFAULT_CODE_MEMORY_SIZE    128 * 1024 /* defaulting to 128kB         */

#if defined(ENABLE_THREADS)
static Mutex *code_memory_mutex = NULL;
#endif
static void  *code_memory       = NULL;
static int    code_memory_size  = 0;
static int    pagesize          = 0;


/* codememory_init *************************************************************

   Initialize the code memory subsystem.

*******************************************************************************/

void codememory_init(void)
{
	TRACESUBSYSTEMINITIALIZATION("codememory_init");

#if defined(ENABLE_THREADS)
	/* create mutex for code memory */

	code_memory_mutex = new Mutex();
#endif

	/* Get the pagesize of this architecture. */

	pagesize = os::getpagesize();
}


/* codememory_get **************************************************************

   Allocates memory from the heap via mmap and make the memory read-,
   write-, and executeable.

*******************************************************************************/

void *codememory_get(size_t size)
{
	void *p;

	code_memory_mutex->lock();

	size = MEMORY_ALIGN(size, ALIGNSIZE);

	/* check if enough memory is available */

	if (size > code_memory_size) {
		/* set default code size */

		code_memory_size = DEFAULT_CODE_MEMORY_SIZE;

		/* do we need more? */

		if (size > code_memory_size)
			code_memory_size = size;

		/* align the size of the memory to be allocated */

		code_memory_size = MEMORY_ALIGN(code_memory_size, pagesize);

#if defined(ENABLE_STATISTICS)
		if (opt_stat) {
			codememusage += code_memory_size;

			if (codememusage > maxcodememusage)
				maxcodememusage = codememusage;
		}
#endif

		/* allocate the memory */

		p = os::mmap_anonymous(NULL, code_memory_size,
							   PROT_READ | PROT_WRITE | PROT_EXEC,
							   MAP_PRIVATE);

		/* set global code memory pointer */

		code_memory = p;
	}

	/* get a memory chunk of the allocated memory */

	p = code_memory;

	code_memory       = (void *) ((ptrint) code_memory + size);
	code_memory_size -= size;

	code_memory_mutex->unlock();

	return p;
}


/* codememory_release **********************************************************

   Release the code memory and return it to the code memory
   management.

   IN:
       p ...... pointer to the code memory
	   size ... size of the code memory

*******************************************************************************/

void codememory_release(void *p, size_t size)
{
	/* do nothing */
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
