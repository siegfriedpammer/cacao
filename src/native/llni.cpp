/* src/native/llni.c - low level native interfarce (LLNI)

   Copyright (C) 2007-2013
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

#include <assert.h>

#include "threads/thread.hpp"


/* LLNI critical sections ******************************************************

   Start and end a LLNI critical section which prevents the GC from
   moving objects around the GC heap.

*******************************************************************************/

#if defined(ENABLE_THREADS) && defined(ENABLE_GC_CACAO)
void llni_critical_start_thread(threadobject *t)
{
	assert(!t->gc_critical);
	t->gc_critical = true;
}


void llni_critical_end_thread(threadobject *t)
{
	assert(t->gc_critical);
	t->gc_critical = false;
}


void llni_critical_start()
{
	llni_critical_start_thread(THREADOBJECT);
}


void llni_critical_end()
{
	llni_critical_end_thread(THREADOBJECT);
}
#endif /* defined(ENABLE_THREADS) && defined(ENABLE_GC_CACAO) */


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