#ifndef __mm_lifespan_h__
#define __mm_lifespan_h__

void lifespan_init(void* heap_base, unsigned long heap_size);
void lifespan_close();
void lifespan_emit();

void lifespan_free(void* from, void* limit);
void lifespan_alloc(void* addr, unsigned long size);

#endif

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
