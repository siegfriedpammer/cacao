#include <stddef.h>
#include <unistd.h>		/* getpagesize, mmap, ... */
#include <sys/mman.h>
#ifndef MAP_FAILED
#define MAP_FAILED ((void*) -1)
#endif
#include <sys/types.h>
#include <stdio.h>
#include "../asmpart.h"
#include "../callargs.h"
#include "../threads/thread.h"
#include "../threads/locks.h"
#include <assert.h>

#include "mm.h"

#define PAGESIZE_MINUS_ONE	(getpagesize() - 1)

#undef ALIGN
#undef OFFSET

#define GC_COLLECT_STATISTICS
#define FINALIZER_COUNTING

#undef STRUCTURES_ON_HEAP
//#define STRUCTURES_ON_HEAP

#define false 0
#define true 1

#include "allocator.h" /* rev. 1 allocator */
#include "bitmap2.h"   /* rev. 2 bitmap management */

bool collectverbose;

void gc_call (void);

/* --- macros */

#define align_size(size)	((size) & ~((1 << ALIGN) - 1))
#define MAP_ADDRESS			(void*) 0x10000000

#define VERBOSITY_MESSAGE	1
#define VERBOSITY_DEBUG		2
#define VERBOSITY_MISTRUST	3
#define VERBOSITY_TRACE		4
#define VERBOSITY_PARANOIA	5
#define VERBOSITY_LIFESPAN	6

//#define VERBOSITY			VERBOSITY_MESSAGE
//#define VERBOSITY			VERBOSITY_PARANOIA
#define VERBOSITY			VERBOSITY_LIFESPAN

/* --- file-wide variables */

static void*	heap_base = NULL;
static SIZE		heap_size = 0;
static void*	heap_top = NULL;
static void*	heap_limit = NULL;
static void*	heap_next_collection = NULL;

static bitmap_t* start_bitmap = NULL;
static BITBLOCK* start_bits = NULL;
static bitmap_t* reference_bitmap = NULL;
static BITBLOCK* reference_bits = NULL;
static bitmap_t* mark_bitmap = NULL;
static BITBLOCK* mark_bits = NULL;

static void**	stackbottom = NULL;

typedef struct address_list_node {
	void* address;
	struct address_list_node* prev;
	struct address_list_node* next;
} address_list_node;

static address_list_node* references = NULL;
static address_list_node* finalizers = NULL;

#ifdef GC_COLLECT_STATISTICS

static unsigned long gc_collections_count = 0;

static unsigned long gc_alloc_total = 0;
static unsigned long gc_alloc_count = 0;

static unsigned long gc_mark_heapblocks_visited = 0;
static unsigned long gc_mark_not_aligned = 0;
static unsigned long gc_mark_not_inheap = 0;
static unsigned long gc_mark_not_object = 0;
static unsigned long gc_mark_objects_marked = 0;
static unsigned long gc_mark_already_marked = 0;

#endif

#ifdef FINALIZER_COUNTING

static unsigned long gc_finalizers_executed = 0;
static unsigned long gc_finalizers_detected = 0;

#endif

/* --- implementation */

void 
heap_init (SIZE size, 
		   SIZE startsize, /* when should we collect for the first time ? */
		   void **in_stackbottom)
{
#if VERBOSITY == VERBOSITY_MESSAGE
	fprintf(stderr, "The heap is verbose.\n");
#elif VERBOSITY == VERBOSITY_DEBUG
	fprintf(stderr, "The heap is in debug mode.\n");
#elif VERBOSITY == VERBOSITY_MISTRUST
	fprintf(stderr, "The heap is mistrusting us.\n");
#elif VERBOSITY == VERBOSITY_TRACE
	fprintf(stderr, "The heap is emitting trace information.\n");
#elif VERBOSITY == VERBOSITY_PARANOIA
	fprintf(stderr, "The heap is paranoid.\n");
#elif VERBOSITY == VERBOSITY_LIFESPAN
	fprintf(stderr, "The heap is collecting lifespan information.\n");
#endif

	/* 1. Initialise the freelists & reset the allocator's state */
	allocator_init(); 

	/* 2. Allocate at least (alignment!) size bytes of memory for the heap */
	heap_size = align_size(size + ((1 << ALIGN) - 1));

#if 1
	/* For now, we'll try to map in the stack at a fixed address...
	   ...this will change soon. -- phil. */
	heap_base = (void*) mmap (MAP_ADDRESS, 
							  ((size_t)heap_size + PAGESIZE_MINUS_ONE) & ~PAGESIZE_MINUS_ONE,
							  PROT_READ | PROT_WRITE, 
							  MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, 
							  -1, 
							  (off_t) 0);
#else
#if 0
	heap_base = malloc(heap_size);
#else
	heap_base = (void*) mmap (NULL, 
							  ((size_t)heap_size + PAGESIZE_MINUS_ONE) & ~PAGESIZE_MINUS_ONE,
							  PROT_READ | PROT_WRITE, 
							  MAP_PRIVATE | MAP_ANONYMOUS, 
							  -1, 
							  (off_t) 0);
#endif
	fprintf(stderr, "heap @ 0x%lx\n", heap_base);
#endif

	if (heap_base == (void*)MAP_FAILED) {
		/* unable to allocate the requested amount of memory */
		fprintf(stderr, "The queen, mylord, is dead!\n");
		exit(-1);
	}

	/* 3. Allocate the bitmaps */
	start_bitmap = bitmap_allocate(heap_base, heap_size);
	reference_bitmap = bitmap_allocate(heap_base, heap_size);
	mark_bitmap = bitmap_allocate(heap_base, heap_size);

	start_bits = start_bitmap->bitmap;
    reference_bits = reference_bitmap->bitmap;
    mark_bits = mark_bitmap->bitmap;

	/* 4. Mark the first free-area as an object-start */
	bitmap_setbit(start_bits, heap_base);

	/* 5. Initialise the heap's state (heap_top, etc.) */
	stackbottom = in_stackbottom; /* copy the stackbottom */

	heap_top = heap_base; /* the current end of the heap (just behind the last allocated object) */
	heap_limit = (void*)((long)heap_base + heap_size); /* points just behind the last accessible block of the heap */

	/* 6. calculate a useful first collection limit */
	/* This is extremly primitive at this point... 
	   we should replace it with something more useful -- phil. */
	heap_next_collection = (void*)((long)heap_base + (heap_size / 8));

	/* 7. Init the global reference lists & finalizer addresses */
	references = NULL;
	finalizers = NULL;

#ifdef STRUCTURES_ON_HEAP
	heap_addreference(&references);
	heap_addreference(&finalizers);
#endif
}

void 
heap_close (void)
{
	address_list_node* curr = finalizers;

	/* 1. Clean up on the heap... finalize all remaining objects */
#if 1
	while (curr) {
		address_list_node* prev = curr;
		
		if (curr->address) {
			if (bitmap_testbit(start_bits, curr->address)) {
#ifdef FINALIZER_COUNTING
				++gc_finalizers_executed;
#endif
				asm_calljavamethod(((java_objectheader*)(curr->address))->vftbl->class->finalizer, curr->address, NULL, NULL, NULL);

			}
		}
		
		curr = curr->next;
		free(prev);
	}
#endif

	/* 2. Release the bitmaps */
	bitmap_release(start_bitmap);
	bitmap_release(reference_bitmap);
	bitmap_release(mark_bitmap);

	/* 3. Release the memory allocated to the heap */
	if (heap_base)
		munmap(heap_base, heap_size);

#ifdef FINALIZER_COUNTING
	sprintf(logtext, "%ld objects with a finalizer", gc_finalizers_detected);
	dolog();

#ifdef GC_COLLECT_STATISTICS
	/* 4. output statical data */
	sprintf(logtext, "%ld bytes for %ld objects allocated.", 
			gc_alloc_total, gc_alloc_count); 
	dolog();
	sprintf(logtext, "%ld garbage collections performed.", gc_collections_count);
	dolog();
	sprintf(logtext, "%ld heapblocks visited, %ld objects marked", 
			gc_mark_heapblocks_visited, gc_mark_objects_marked);
	dolog();
	sprintf(logtext, "    %ld out of heap.", gc_mark_not_inheap);
	dolog();
	sprintf(logtext, "    %ld visits to objects already marked.", gc_mark_already_marked);
	dolog();
	sprintf(logtext, "    %ld not an object.", gc_mark_not_object);
	dolog();
	sprintf(logtext, "    %ld potential references not aligned.", gc_mark_not_aligned);
	dolog();
#endif

	if (gc_finalizers_detected == gc_finalizers_executed)
		sprintf(logtext, "    all finalizers executed.");
	else
		sprintf(logtext, "    only %ld finalizers executed.", gc_finalizers_executed);
	dolog();
#endif

}

__inline__
static
void 
heap_add_address_to_address_list(address_list_node** list, void* address)
{
	/* Note: address lists are kept sorted to simplify finalization */

	address_list_node* new_node = malloc(sizeof(address_list_node));
	new_node->address = address;
	new_node->next = NULL;

	while (*list && (*list)->next) {
#if VERBOSITY >= VERBOSITY_PARANOIA
		if ((*list)->address == address)
			fprintf(stderr,
					"Attempt to add a duplicate adress to an adress list.\n");
#endif

		if ((*list)->next->address < address)
			list = &(*list)->next;
		else {
			new_node->next = *list;
			*list = new_node;
			return;
		}			
	}

	new_node->next = *list;
	*list = new_node;
}


__inline__
static
void
heap_add_finalizer_for_object_at(void* addr)
{
	/* Finalizers seem to be very rare... for this reason, I keep a linked
	   list of object addresses, which have a finalizer attached. This list
	   is kept in ascending order according to the order garbage is freed.
	   This list is currently kept separate from the heap, but should be 
	   moved onto it, but some JIT-marker code to handle these special
	   objects will need to be added first. -- phil. */

	heap_add_address_to_address_list(&finalizers, addr);
}

void* 
heap_allocate (SIZE		  in_length, 
			   bool 	  references, 
			   methodinfo *finalizer)
{
	SIZE	length = align_size(in_length + ((1 << ALIGN) - 1)); 
	void*	free_chunk = NULL;

#if VERBOSITY >= VERBOSITY_MISTRUST && 0
	/* check for misaligned in_length parameter */
	if (length != in_length)
		fprintf(stderr,
				"heap2.c: heap_allocate was passed unaligned in_length parameter: %ld, \n         aligned to %ld. (mistrust)\n",
				in_length, length);
#endif

	// fprintf(stderr, "heap_allocate: 0x%lx (%ld) requested, 0x%lx (%ld) aligned\n", in_length, in_length, length, length);

#ifdef FINALIZER_COUNTING
	if (finalizer)
		++gc_finalizers_detected;
#endif

#if VERBOSITY >= VERBOSITY_LIFESPAN && 0
	/* perform garbage collection to collect data for lifespan analysis */
	if (heap_top > heap_base)
		gc_call();
#endif

	intsDisable();

 retry:
	/* 1. attempt to get a free block with size >= length from the freelists */
	free_chunk = allocator_alloc(length);

	/* 2. if unsuccessful, try alternative allocation strategies */
	if (!free_chunk) {
		/* 2.a if the collection threshold would be exceeded, collect the heap */
		if ((long)heap_top + length > (long)heap_next_collection) {
			/* 2.a.1. collect if the next_collection threshold would be exceeded */
			gc_call();

			/* 2.a.2. we just ran a collection, recheck the freelists */
			free_chunk = allocator_alloc(length);
			if (free_chunk)
				goto success;

			/* 2.a.3. we can't satisfy the request from the freelists, check
			          against the heap_limit whether growing the heap is possible */
			if ((long)heap_top + length > (long)heap_limit)
				goto failure;
		}

		/* 2.b. grow the heap */
		free_chunk = heap_top;
		heap_top = (void*)((long)heap_top + length);
	}

 success:
	/* 3.a. mark all necessary bits, store the finalizer & return the newly allocate block */
#if 0
	/* I don't mark the object-start anymore, as it always is at the beginning of a free-block,
	   which already is marked (Note: The first free-block gets marked in heap_init). -- phil. */
  	bitmap_setbit(start_bits, free_chunk); /* mark the new object */
#endif
	bitmap_setbit(start_bits, (void*)((long)free_chunk + (long)length)); /* mark the freespace behind the new object */
	if (references)
		bitmap_setbit(reference_bits, free_chunk);
	else 
		bitmap_clearbit(reference_bits, free_chunk);

#if 1
	/* store a hint, that there's a finalizer */
	if (finalizer)
		heap_add_finalizer_for_object_at(free_chunk);
#else
#   warning "finalizers disabled."
#endif

#if VERBOSITY >= VERBOSITY_TRACE && 0
	fprintf(stderr, "heap_allocate: returning free block of 0x%lx bytes at 0x%lx\n", length, free_chunk);
#endif
#if VERBOSITY >= VERBOSITY_PARANOIA && 0
	fprintf(stderr, "heap_allocate: reporting heap state variables:\n");
	fprintf(stderr, "\theap top              0x%lx\n", heap_top);
	fprintf(stderr, "\theap size             0x%lx (%ld)\n", heap_size, heap_size);
	fprintf(stderr, "\theap_limit            0x%lx\n", heap_limit);
	fprintf(stderr, "\theap next collection  0x%lx\n", heap_next_collection);
#endif

#ifdef GC_COLLECT_STATISTICS
	gc_alloc_total += length;
	++gc_alloc_count;
#endif

	return free_chunk;
	
 failure:
	/* 3.b. failure to allocate enough memory... fail gracefully */
#if VERBOSITY >= VERBOSITY_MESSAGE && 0
	fprintf(stderr, 
			"heap2.c: heap_allocate was unable to allocate 0x%lx bytes on the VM heap.\n",
			length);
#endif

#if VERBOSITY >= VERBOSITY_TRACE
	fprintf(stderr, "heap_allocate: returning NULL\n");
#endif

	return NULL;

	intsRestore();
}

void 
heap_addreference (void **reflocation)
{
	/* I currently use a separate linked list (as in the original code) to hold
	   the global reference locations, but I'll change this to allocate these
	   in blocks on the heap; we'll have to add JIT-Marker code for those Java
	   objects then. -- phil. */

	heap_add_address_to_address_list(&references, reflocation);
}

static
__inline__
void gc_finalize (void)
{
	/* This will have to be slightly rewritten as soon the JIT-marked heap-based lists are used. -- phil. */

	address_list_node*  curr = finalizers;
	address_list_node*  prev;

//	fprintf(stderr, "finalizing\n");

#if 0
	while (curr) {
		if (!bitmap_testbit(mark_bits, curr->address)) {
			/* I need a way to save derefs. Any suggestions? -- phil. */
			asm_calljavamethod(((java_objectheader*)curr->address)->vftbl->class->finalizer, 
							   curr->address, NULL, NULL, NULL);

			prev->next = curr->next;			
			free(curr);
		} else {
			prev = curr;
			curr = curr->next;
		}
	}
#else
	while (curr) {
		if (curr->address) {
			if (!bitmap_testbit(mark_bits, curr->address)) {

#ifdef FINALIZER_COUNTING
				++gc_finalizers_executed;
#endif
				asm_calljavamethod(((java_objectheader*)curr->address)->vftbl->class->finalizer, 
								   curr->address, NULL, NULL, NULL);
				curr->address = 0;
			}
		}

		curr = curr->next;
	}
#endif

//	fprintf(stderr, "done\n");
}


__inline__
static 
void gc_reclaim (void)
{
	void* free_start;
	void* free_end = heap_base;
	BITBLOCK* temp_bits;
	bitmap_t* temp_bitmap;

	/* 1. reset the freelists */
	allocator_reset();

	/* 2. reclaim unmarked objects */
#if 0 && SIZE_FROM_CLASSINFO
	free_start = bitmap_find_next_combination_set_unset(start_bitmap,
														mark_bitmap,
														free_end);
	while (free_start < heap_top) {
		if (!bitmap_testbit(start_bits, free_start) || 
			bitmap_testbit(mark_bits, free_start)) {
			fprintf(stderr, "gc_reclaim: inconsistent bit info for 0x%lx\n", free_start);
		}

		free_end = free_start;
		while((free_end < heap_top) && 
			  (!bitmap_testbit(mark_bits, free_end)) {
			free_end += 
		}



bitmap_find_next_setbit(mark_bitmap, free_start + 8); /* FIXME: constant used */

			//			fprintf(stderr, "%lx -- %lx\n", free_start, free_end);
			
			if (free_end < heap_top) {
				allocator_free(free_start, free_end - free_start);

				//				fprintf(stderr, "gc_reclaim: freed 0x%lx bytes at 0x%lx\n", free_end - free_start, free_start);

#if !defined(JIT_MARKER_SUPPORT)
				/* might make trouble with JIT-Marker support. The Marker for unused blocks 
				   might be called, leading to a bad dereference. -- phil. */
				bitmap_setbit(mark_bits, free_start);
#endif
			}
		} else {
			//			fprintf(stderr, "hmmm... found freeable area of 0 bytes at heaptop ?!\n");
			free_end = heap_top;
		}			
	}
#else
	while (free_end < heap_top) {
		free_start = bitmap_find_next_combination_set_unset(start_bitmap,
															mark_bitmap,
															free_end);

		if (!bitmap_testbit(start_bits, free_start) || 
			bitmap_testbit(mark_bits, free_start))
			fprintf(stderr, "gc_reclaim: inconsistent bit info for 0x%lx\n", free_start);

		if (free_start < heap_top) {
			free_end   = bitmap_find_next_setbit(mark_bitmap, (void*)((long)free_start + 8)); /* FIXME: constant used */

			//			fprintf(stderr, "%lx -- %lx\n", free_start, free_end);
			
			if (free_end < heap_top) {
				allocator_free(free_start, (long)free_end - (long)free_start);

				//				fprintf(stderr, "gc_reclaim: freed 0x%lx bytes at 0x%lx\n", free_end - free_start, free_start);

#if !defined(JIT_MARKER_SUPPORT)
				/* might make trouble with JIT-Marker support. The Marker for unused blocks 
				   might be called, leading to a bad dereference. -- phil. */
				bitmap_setbit(mark_bits, free_start);
#endif
			}
		} else {
			//			fprintf(stderr, "hmmm... found freeable area of 0 bytes at heaptop ?!\n");
			free_end = heap_top;
		}			
	}
#endif

	/* 3.1. swap mark & start bitmaps */
	temp_bits = mark_bits;
	mark_bits = start_bits;
	start_bits = temp_bits;

	temp_bitmap = mark_bitmap;
	mark_bitmap = start_bitmap;
	start_bitmap = temp_bitmap;

	/* 3.2. mask reference bitmap */
#warning "bitmap masking unimplemented --- references will not be cleared"

	/* 3.3. update heap_top */
	if (free_start < heap_top)
		heap_top = free_start;

	if (heap_top < heap_limit)
		bitmap_setbit(start_bits, heap_top);

	/* 4. adjust the collection threshold */
	heap_next_collection = (void*)((long)heap_top + ((long)heap_limit - (long)heap_top) / 8);
	if (heap_next_collection > heap_limit)
		heap_next_collection = heap_limit;

#if VERBOSITY >= VERBOSITY_PARANOIA && 0
	fprintf(stderr, "gc_reclaim: reporting heap state variables:\n");
	fprintf(stderr, "\theap top              0x%lx\n", heap_top);
	fprintf(stderr, "\theap size             0x%lx (%ld)\n", heap_size, heap_size);
	fprintf(stderr, "\theap_limit            0x%lx\n", heap_limit);
	fprintf(stderr, "\theap next collection  0x%lx\n", heap_next_collection);

	allocator_dump();
#endif
}

__inline__
static
void 
gc_mark_object_at (void** addr)
{
	/*
	 * A note concerning the order of the tests:
	 *
	 * Statistics collected during a test run, where alignment
	 * was tested before checking whether the addr points into 
	 * the heap:
	 * >> LOG: 9301464 bytes for 196724 objects allocated.
	 * >> LOG: 15 garbage collections performed.
	 * >> LOG: 6568440 heapblocks visited, 469249 objects marked
	 * >> LOG:     1064447 visits to objects already marked.
	 * >> LOG:     988270 potential references not aligned.
	 * >> LOG:     4049446 out of heap.
	 * >> LOG:     5236 not an object.
	 *
	 * These results show, that only about 1/4 of all heapblocks 
	 * point to objects; The single most important reason why a 
	 * heapblock can not point at an object is, that it's value 
	 * doesn't fall within the heap area (this test was performed
	 * with a 3MB heap).
	 *
	 * From the results, the various tests have to be conducted 
	 * in the following order for maximum efficiency:
	 *    1. addr in heap?
	 *    2. already marked ?
	 *    3. aligned ?
	 *    4. object ?
	 *
	 * The results after reordering:
	 * >> LOG: 6568440 heapblocks visited, 469249 objects marked
	 * >> LOG:     1064447 visits to objects already marked.
	 * >> LOG:     350 potential references not aligned.
	 * >> LOG:     5037366 out of heap.
	 * >> LOG:     5236 not an object.
	 *
	 * And using:
	 *    1. addr in heap?
	 *    2. already marked ?
	 *    3. object ?
	 *    4. aligned ?
	 * 
	 * >> LOG: 6568440 heapblocks visited, 469249 objects marked
	 * >> LOG:     5037366 out of heap.
	 * >> LOG:     1064456 visits to objects already marked.
	 * >> LOG:     5539 not an object.
	 * >> LOG:     38 potential references not aligned.
	 *
	 * Apparently, most unaligned values will already be eliminated
	 * when checking against the bounds of the heap. Checking this
	 * property first, should thus improve collection times.
	 */


#ifdef GC_COLLECT_STATISTICS
	if (!((void*)addr >= heap_base && (void*)addr < heap_top)) {
		++gc_mark_not_inheap;
		return;
	}

	if (bitmap_testbit(mark_bits, (void*)addr)) {
		++gc_mark_already_marked;
		return;
	}

	if (!bitmap_testbit(start_bits, (void*)addr)) {
		++gc_mark_not_object;
		return;
	}

	if ((long)addr & ((1 << ALIGN) - 1)) {
		++gc_mark_not_aligned;
		return;
	}

#endif

#if 0
	//	fprintf(stderr, "gc_mark_object_at: called for address 0x%lx\n", addr);

	if ((long)addr & ((1 << ALIGN) - 1)) {
		fprintf(stderr, "non aligned pointer\n");
		return;
	}

	if (!((void*)addr >= heap_base && (void*)addr < heap_top)) {
		fprintf(stderr, "not an address on the heap.\n");
		return;
	}

	if (!bitmap_testbit(start_bits, (void*)addr)) {
		fprintf(stderr, "not an object.\n");
		return;
	}

	if (bitmap_testbit(mark_bits, (void*)addr)) {
		fprintf(stderr, "already marked.\n");
		return;
	}
#endif

	/* 1. is the addr aligned, on the heap && the start of an object */
	if (!((long)addr & ((1 << ALIGN) - 1)) &&
		(void*)addr >= heap_base && 
		(void*)addr < heap_top && 
		bitmap_testbit(start_bits, (void*)addr) &&
		!bitmap_testbit(mark_bits, (void*)addr)) {
		bitmap_setbit(mark_bits, (void*)addr); 

#ifdef GC_COLLECT_STATISTICS
		++gc_mark_objects_marked;
#endif

		//		fprintf(stderr, "gc_mark_object_at: called for address 0x%lx: ", addr);
		//		fprintf(stderr, "marking object.\n");

#ifdef JIT_MARKER_SUPPORT
		/* 1.a. invoke the JIT-marker method */
   		asm_calljavamethod(addr->vftbl->class->marker, addr, NULL, NULL, NULL);
#else
		/* 1.b. mark the references contained */
		if (bitmap_testbit(reference_bits, addr)) {
			void** end;
#ifdef SIZE_FROM_CLASSINFO
			void** old_end;

			if (((java_objectheader*)addr)->vftbl == class_array->vftbl) {
				end = (void**)((long)addr + (long)((java_arrayheader*)addr)->alignedsize); 
//				fprintf(stderr, "size found for array at 0x%lx: 0x%lx\n", addr, ((java_arrayheader*)addr)->alignedsize);
			}
			else {
				end = (void**)((long)addr + (long)((java_objectheader*)addr)->vftbl->class->alignedsize);							   
//				fprintf(stderr, "size found for 0x%lx: 0x%lx\n", addr, ((java_objectheader*)addr)->vftbl->class->alignedsize);
			}

			old_end = (void**)bitmap_find_next_setbit(start_bitmap, (void*)addr + 8);
			if (end != old_end) {
				fprintf(stderr, "inconsistent object length for object at 0x%lx:", addr);
				fprintf(stderr, " old = 0x%lx ; new = 0x%lx\n", old_end, end);
			}
#else
			end = (void**)bitmap_find_next_setbit(start_bitmap, addr + 8); /* points just behind the object */
#endif

#ifdef GC_COLLECT_STATISTICS
		gc_mark_heapblocks_visited += ((long)end - (long)addr) >> ALIGN;
#endif

			while (addr < end)
				gc_mark_object_at(*(addr++));
		}
#endif	
	}

	return;
}


__inline__
static
void gc_mark_references (void)
{
	address_list_node* curr = references;

	//	fprintf(stderr, "gc_mark_references\n");

	while (curr) {
		gc_mark_object_at(*((void**)(curr->address)));
		curr = curr->next;
	}
}

__inline__
static
void 
markreferences(void** start, void** end)
{
	while (start < end)
		gc_mark_object_at(*(start++));
}

__inline__
static
void gc_mark_stack (void)
{
	void *dummy;

#ifdef USE_THREADS 
    thread *aThread;
	
	if (currentThread == NULL) {
		void **top_of_stack = &dummy;
		
		if (top_of_stack > stackbottom)
			markreferences(stackbottom, top_of_stack);
		else
			markreferences(top_of_stack, stackbottom);
	}
	else {
		for (aThread = liveThreads; aThread != 0;
			 aThread = CONTEXT(aThread).nextlive) {
			mark_object_at((void*)aThread);
			if (aThread == currentThread) {
				void **top_of_stack = &dummy;
				
				if (top_of_stack > (void**)CONTEXT(aThread).stackEnd)
					markreferences((void**)CONTEXT(aThread).stackEnd, top_of_stack);
				else 	
					markreferences(top_of_stack, (void**)CONTEXT(aThread).stackEnd);
			}
			else {
				if (CONTEXT(aThread).usedStackTop > CONTEXT(aThread).stackEnd)
					markreferences((void**)CONTEXT(aThread).stackEnd,
								   (void**)CONTEXT(aThread).usedStackTop);
				else 	
					markreferences((void**)CONTEXT(aThread).usedStackTop,
								   (void**)CONTEXT(aThread).stackEnd);
			}
	    }

		markreferences((void**)&threadQhead[0],
					   (void**)&threadQhead[MAX_THREAD_PRIO]);
	}
#else
    void **top_of_stack = &dummy;

	//	fprintf(stderr, "gc_mark_stack\n");
	
    if (top_of_stack > stackbottom) {
		//		fprintf(stderr, "stack growing upward\n");
        markreferences(stackbottom, top_of_stack);
	} else {
		//		fprintf(stderr, "stack growing downward\n");
        markreferences(top_of_stack, stackbottom);
	}
#endif
}


static 
void gc_run (void)
{
	static int armageddon_is_near = 0;

	if (armageddon_is_near) {
		/* armageddon_is_here! */
		fprintf(stderr, "Oops, seems like there's a slight problem here: gc_run() called while still running?!\n");
		return;
	}

	armageddon_is_near = true;
	heap_next_collection = heap_limit;  /* try to avoid finalizer-induced collections */

	bitmap_clear(mark_bitmap);

	asm_dumpregistersandcall(gc_mark_stack);
	gc_mark_references();
	gc_finalize();
	gc_reclaim();

	armageddon_is_near = false;

#ifdef GC_COLLECT_STATISTICS
	++gc_collections_count;
#endif
}


/************************* Function: gc_init **********************************

  Initializes anything that must be initialized to call the gc on the right
  stack.

******************************************************************************/

void
gc_init (void)
{
}

/************************** Function: gc_call ********************************

  Calls the garbage collector. The garbage collector should always be called
  using this function since it ensures that enough stack space is available.

******************************************************************************/

void
gc_call (void)
{
#ifdef USE_THREADS
	assert(blockInts == 0);

	intsDisable();
	if (currentThread == NULL || currentThread == mainThread)
		gc_run();
	else
		asm_switchstackandcall(CONTEXT(mainThread).usedStackTop, gc_run);
	intsRestore();
#else
	gc_run();
#endif
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
