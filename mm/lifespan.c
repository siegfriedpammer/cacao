/*
 * cacao/mm/lifespan.c
 * $Id: lifespan.c 95 1998-11-30 14:53:05Z phil $
 */

#include "lifespan.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned long time;
	unsigned long size;
} lifespan_object;

static unsigned long     current_time = 0;
static lifespan_object** lifespan_objects = NULL;
static lifespan_object** lifespan_objects_end = NULL;
static void*             lifespan_objects_off = NULL;
static FILE*             lifespan_file = NULL;

void lifespan_init(void* heap_base, unsigned long heap_size)
{
	current_time = 0;

	lifespan_objects = (lifespan_object**)malloc(heap_size);
	lifespan_objects_end = ((lifespan_object**)((unsigned long)lifespan_objects + (unsigned long)heap_size));
	lifespan_objects_off = (void*)((unsigned long)lifespan_objects - (unsigned long)heap_base);
	lifespan_file = popen("gzip -9 >lifespans.gz", "w");

	if (!lifespan_objects || !lifespan_file) {
		fprintf(stderr, "Failed to allocate memory for lifespan-object index / popen lifespan output stream\n");
		exit(-1);
	}

	memset(lifespan_objects, 0, heap_size);
}

static inline void lifespan_free_object(lifespan_object** o)
{
	/* file format: alloc time, size, lifespan */

	if (*o) {
		fprintf(lifespan_file, "%ld\t%ld\t%ld\n", (*o)->time, (*o)->size, current_time - (*o)->time);
		free(*o);
		*o = NULL;
	}
}

void lifespan_close()
{
	if (lifespan_objects) {
		while(lifespan_objects < lifespan_objects_end)
			lifespan_free_object(--lifespan_objects_end);

		free(lifespan_objects);
	}

	if (lifespan_file)
		pclose(lifespan_file);
}

void lifespan_emit()
{
	/* emit summary */
}

void lifespan_free(void* from, void* limit)
{
	lifespan_object**  object;
	object = (lifespan_object**)((unsigned long)from + (unsigned long)lifespan_objects_off);

	while (from < limit) {
		lifespan_free_object(object++);
		++from;
	}
}

void lifespan_alloc(void* addr, unsigned long size) 
{
	lifespan_object**  object;
	object = (lifespan_object**)((unsigned long)addr + (unsigned long)lifespan_objects_off);

	*object = (lifespan_object*)malloc(sizeof(lifespan_object));
	if (!*object) {
		fprintf(stderr, "oops.\n");
		exit(-10);
	}
	(*object)->time = current_time;
	(*object)->size = size;
	
	current_time += size;
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
