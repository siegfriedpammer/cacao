/*
 * cacao/mm/lifespan.c
 * $Id: lifespan.c 115 1999-01-20 01:52:45Z phil $
 */

#include "mm.h"
#include "lifespan.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned long time;
	unsigned long number;
	unsigned long size;
} lifespan_object;

static unsigned long     current_time = 0;
static unsigned long     current_number = 0;
static lifespan_object** lifespan_objects = NULL;
static lifespan_object** lifespan_objects_end = NULL;
static void*             lifespan_objects_off = NULL;
static FILE*             lifespan_file = NULL;

static unsigned long     lifespan_histo_size[64] = { 0 };
static unsigned long     lifespan_histo_lifespan[64] = { 0 };

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
	int size, high = 0;
	/* file format: alloc time, size, lifespan */

	if (*o) {
		fprintf(lifespan_file, 
				"%ld\t%ld\t%ld\t%ld\t%ld\n", 
				(*o)->number, 
				(*o)->time, 
				(*o)->size, 
				current_number - (*o)->number,
				current_time - (*o)->time);
		
		/* histo_size */
		size = (*o)->size;
		while (size) {
			++high;
			size = size >> 1;
		}

		++lifespan_histo_size[high];

		/* histo_life */
		high = 0;
		size = current_time - (*o)->time;
		while (size) {
			++high;
			size = size >> 1;
		}

		++lifespan_histo_lifespan[high];

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
	int i;

	for (i = 4; i < 32; ++i)
#if 0
		fprintf(stderr, "%Lu-%Lu\t%Lu\n", 
				(1LL << (i-1)), 
				(1LL << i) - 1,
				lifespan_histo_size[i]);
#else
		fprintf(stderr, "%Lu\n", 
				lifespan_histo_size[i]);
#endif

	fprintf(stderr, "\n\n\n");

	for (i = 4; i < 32; ++i)
		fprintf(stderr, "%Lu-%Lu\t%Lu\n", 
				(1LL << (i-1)), 
				(1LL << i) - 1,
				lifespan_histo_lifespan[i]);
}

void lifespan_free(void** from, void** limit)
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
	int high = 0;
	lifespan_object**  object;
	object = (lifespan_object**)((unsigned long)addr + (unsigned long)lifespan_objects_off);

	*object = (lifespan_object*)malloc(sizeof(lifespan_object));
	if (!*object) {
		fprintf(stderr, "oops.\n");
		exit(-10);
	}
	(*object)->time = current_time;
	(*object)->size = size;
	(*object)->number = ++current_number;

	while (size) {
		++high;
		size = size >> 1;
	}
	++lifespan_histo_size[high];
	
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
