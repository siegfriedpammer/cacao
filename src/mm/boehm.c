/* boehm.c *********************************************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Contains the interface to the Boehm GC

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
	Changes: Andi Krall          EMAIL: cacao@complang.tuwien.ac.at
	         Mark Probst         EMAIL: cacao@complang.tuwien.ac.at
			 Philipp Tomsich     EMAIL: cacao@complang.tuwien.ac.at

	Last Change: $Id: boehm.c 512 2003-10-22 20:47:18Z twisti $

*******************************************************************************/

#include "global.h"
#include "threads/thread.h"
#include "asmpart.h"

/* this is temporary workaround */
#if defined(__X86_64__)
#define GC_DEBUG
#endif

#include "gc.h"


struct otherstackcall;

typedef void *(*calltwoargs)(void *, u4);

struct otherstackcall {
	calltwoargs p2;
	void *p;
	u4 l;
};

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

#ifdef USE_THREADS
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
	asm_calljavamethod(ob->vftbl->class->finalizer, ob, NULL, NULL, NULL);
}

void *heap_allocate (u4 bytelength, bool references, methodinfo *finalizer)
{
	void *result;
	if (references)
		{ MAINTHREADCALL(result, stackcall_malloc, NULL, bytelength); }
	else
		{ MAINTHREADCALL(result, stackcall_malloc_atomic, NULL, bytelength); }
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

void heap_init (u4 size, u4 startsize, void **stackbottom)
{
	GC_INIT();
}

void heap_close()
{
}

void heap_addreference (void **reflocation)
{
}

int collectverbose;

void gc_init()
{
}

void gc_call()
{
  	if (collectverbose) {
		sprintf(logtext, "Garbage Collection:  previous/now = %d / %d ",
				0, 0);
		dolog();
  	}

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
