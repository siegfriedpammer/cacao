/* mm/boehm.c - interface for boehm gc

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser,
   M. Probst, S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck,
   P. Tomsich, J. Wenninger

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

   Authors: Stefan Ring

   $Id: boehm.c 953 2004-03-11 23:02:26Z stefan $

*/

#if defined(USE_THREADS) && defined(NATIVE_THREADS) && defined(__LINUX__)
#define GC_LINUX_THREADS
#endif

#include "main.h"
#include "boehm.h"
#include "global.h"
#include "native.h"
#include "asmpart.h"
#include "builtin.h"
#include "threads/thread.h"
#include "toolbox/loging.h"
#include "gc.h"


static void *stackcall_twoargs(struct otherstackcall *p)
{
	return (*p->p2)(p->p, p->l);
}


static void *stackcall_malloc(void *p, u4 bytelength)
{
	return GC_MALLOC(bytelength);
}


static void *stackcall_malloc_atomic(void *p, u4 bytelength)
{
	return GC_MALLOC_ATOMIC(bytelength);
}


static void *stackcall_malloc_uncollectable(void *p, u4 bytelength)
{
	return GC_MALLOC_UNCOLLECTABLE(bytelength);
}


static void *stackcall_realloc(void *p, u4 bytelength)
{
	return GC_REALLOC(p, bytelength);
}

static void *stackcall_free(void *p, u4 bytelength)
{
	GC_FREE(p);
	return NULL;
}


#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
#define MAINTHREADCALL(r,m,pp,ll) \
	if (currentThread == NULL || currentThread == mainThread) { \
		r = m(pp, ll); \
	} else { \
		struct otherstackcall sc; \
		sc.p2 = m; \
		sc.p = pp; \
		sc.l = ll; \
		r = (*asm_switchstackandcall)(CONTEXT(mainThread).usedStackTop, \
				stackcall_twoargs, \
				(void**)&(CONTEXT(currentThread).usedStackTop), &sc); \
	}
#else
#define MAINTHREADCALL(r,m,pp,ll) \
	{ r = m(pp, ll); }
#endif


void *heap_alloc_uncollectable(u4 bytelength)
{
	void *result;
	MAINTHREADCALL(result, stackcall_malloc_uncollectable, NULL, bytelength);
	return result;
}


void runboehmfinalizer(void *o, void *p)
{
	java_objectheader *ob = (java_objectheader *) o;
	asm_calljavafunction(ob->vftbl->class->finalizer, ob, NULL, NULL, NULL);
	
	/* if we had an exception in the finalizer, ignore it */
	*exceptionptr = NULL;
}


void *heap_allocate(u4 bytelength, bool references, methodinfo *finalizer)
{
	void *result;

	if (references) {
		MAINTHREADCALL(result, stackcall_malloc, NULL, bytelength);

	} else {
		MAINTHREADCALL(result, stackcall_malloc_atomic, NULL, bytelength);
	}

	if (finalizer)
		GC_REGISTER_FINALIZER(result, runboehmfinalizer, 0, 0, 0);

	return (u1*) result;
}


void *heap_reallocate(void *p, u4 bytelength)
{
	void *result;

	MAINTHREADCALL(result, stackcall_realloc, p, bytelength);

	return result;
}

void heap_free(void *p)
{
	void *result;

	MAINTHREADCALL(result, stackcall_free, p, 0);
}


void heap_init(u4 size, u4 startsize, void **stackbottom)
{
	GC_INIT();
}


void heap_close()
{
}


void gc_init()
{
}


void gc_call()
{
  	if (collectverbose)
		dolog("Garbage Collection:  previous/now = %d / %d ",
			  0, 0);

	GC_gcollect();
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
