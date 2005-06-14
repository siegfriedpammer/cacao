/* src/mm/nogc.c - allocates memory through malloc (no GC)

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Christian Thalinger

   Changes:

   $Id: nogc.c 2678 2005-06-14 16:08:58Z twisti $

*/


#include <stdlib.h>

#include "boehm-gc/include/gc.h"

#include "mm/boehm.h"
#include "toolbox/logging.h"
#include "vm/options.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/stringlocal.h"
#include "vm/tables.h"


void *heap_alloc_uncollectable(u4 size)
{
	void *m;

	m = calloc(size, 1);

	if (!m)
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Out of memory");
	
	return m;
}


void *heap_allocate(u4 size, bool references, methodinfo *finalizer)
{
	void *m;

	m = calloc(size, 1);

	if (!m)
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Out of memory");
	
	return m;
}


void *heap_reallocate(void *p, u4 size)
{
	void *m;

	m = realloc(p, size);

	if (!m)
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Out of memory");
	
	return m;
}


static void gc_ignore_warnings(char *msg, GC_word arg)
{
}


void gc_init(u4 heapmaxsize, u4 heapstartsize)
{
}


void gc_call()
{
	log_text("GC call: nothing done...");
	/* nop */
}


s8 gc_get_heap_size()
{
	return 0;
}


s8 gc_get_free_bytes()
{
	return 0;
}


s8 gc_get_max_heap_size()
{
	return 0;
}


void gc_invoke_finalizers()
{
	/* nop */
}


void gc_finalize_all()
{
	/* nop */
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
 */
