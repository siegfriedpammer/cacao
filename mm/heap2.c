#include <stddef.h>
#include <unistd.h>		/* getpagesize, mmap, ... */
#include <sys/mman.h>

#include <sys/types.h>
#include <stdio.h>
#include <assert.h>

#include "asmpart.h"
#include "callargs.h"
#include "threads/thread.h"
#include "threads/locks.h"

#include "lifespan.h"
#include "mm.h"

#if !defined(HAVE_MAP_FAILED)
#define MAP_FAILED ((void*) -1)
#endif


#define PAGESIZE_MINUS_ONE	(getpagesize() - 1)

#undef ALIGN
#undef OFFSET

#define HEURISTIC_SEL    0
#define HEURISTIC_PARAM  2UL


#define next_collection_heuristic_init()   \
             (void*)((long)heap_top + (((long)heap_limit - (long)heap_top) >> 4))

#if HEURISTIC_SEL == 0
#define next_collection_heuristic()   \
             (void*)((long)heap_top + (((long)heap_limit - (long)heap_top) >> HEURISTIC_PARAM))
#elif HEURISTIC_SEL == 1
#define next_collection_heuristic()   \
             (void*)((long)heap_top + (((long)heap_top - (long)heap_base) << HEURISTIC_PARAM))
#elif HEURISTIC_SEL == 2
#define next_collection_heuristic()   \
             (void*)((long)heap_top + HEURISTIC_PARAM)	 
#endif

	 /*
//#define PSEUDO_GENERATIONAL
//#define COLLECT_LIFESPAN
//#define NEW_COLLECT_LIFESPAN
//#define COLLECT_FRAGMENTATION
//#define COLLECT_SIZES

//#define GC_COLLECT_STATISTICS
//#define FINALIZER_COUNTING
*/

#undef STRUCTURES_ON_HEAP
	 /*
//#define STRUCTURES_ON_HEAP
*/

#define false 0
#define true 1

#include "allocator.h" /* rev. 1 allocator */
#include "bitmap2.h"   /* rev. 2 bitmap management */

bool collectverbose;

void gc_call (void);

/* --- macros */

#define align_size(size)	((size) & ~((1 << ALIGN) - 1))
#define MAP_ADDRESS			(void*) 0x10000000

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

static unsigned long gc_mark_null_pointer = 0;

#endif

#ifdef FINALIZER_COUNTING

static unsigned long gc_finalizers_executed = 0;
static unsigned long gc_finalizers_detected = 0;

#endif

#ifdef USE_THREADS
static iMux  alloc_mutex;
#endif

#ifdef COLLECT_LIFESPAN
static FILE* tracefile;
#endif

#ifdef COLLECT_FRAGMENTATION
static FILE* fragfile;
static FILE* fragsizefile;
#endif

/* --- implementation */

void 
heap_init (SIZE size, 
		   SIZE startsize, /* when should we collect for the first time ? */
		   void **in_stackbottom)
{
	/* 1. Initialise the freelists & reset the allocator's state */
	allocator_init(); 

	/* 2. Allocate at least (alignment!) size bytes of memory for the heap */
	heap_size = align_size(size + ((1 << ALIGN) - 1));

#if !(defined(HAVE_MAP_ANONYMOUS))
	heap_base = malloc(heap_size);
#else
	heap_base = (void*) mmap (NULL, 
							  ((size_t)heap_size + PAGESIZE_MINUS_ONE) & ~PAGESIZE_MINUS_ONE,
							  PROT_READ | PROT_WRITE, 
							  MAP_PRIVATE | MAP_ANONYMOUS, 
							  -1, 
							  (off_t) 0);
#endif

	if (heap_base == (void*)MAP_FAILED) {
		/* unable to allocate the requested amount of memory */
		fprintf(stderr, "heap2.c: The queen, mylord, is dead! (mmap failed)\n");
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
	heap_next_collection = next_collection_heuristic_init();

	/* 7. Init the global reference lists & finalizer addresses */
	references = NULL;
	finalizers = NULL;

#ifdef STRUCTURES_ON_HEAP
	heap_addreference(&references);
	heap_addreference(&finalizers);
#endif

#ifdef USE_THREADS
	/* 8. Init the mutexes for synchronization */
	alloc_mutex.holder = 0;
#endif

	/* 9. Set up collection of lifespan data */
#ifdef COLLECT_LIFESPAN
#if 0
	tracefile = fopen("heap.trace", "w");
#else
	tracefile = popen("gzip -9 >heap.trace.gz", "w");
#endif
	if (!tracefile) {
		fprintf(stderr, "heap2.c: Radio Ga Ga! (fopen failed)\n");
		exit(-2);
	}

	fprintf(tracefile, "heap_base\t0x%lx\n", heap_base);	
	fprintf(tracefile, "heap_limit\t0x%lx\n", heap_limit);
	fprintf(tracefile, "heap_top\t0x%lx\n", heap_top);
#endif

#if defined(NEW_COLLECT_LIFESPAN) || defined(COLLECT_SIZES)
	lifespan_init(heap_base, heap_size);
#endif

	/* 10. Set up collection of fragmentation data */
#ifdef COLLECT_FRAGMENTATION
	fragfile = popen("gzip -9 >fragmentation.gz", "w");
	fragsizefile = popen("gzip -9 >freeblocks.gz", "w");
#endif
}

inline
static
void
heap_call_finalizer_for_object_at(java_objectheader* object_addr)
{
	asm_calljavamethod(object_addr->vftbl->class->finalizer, object_addr, NULL, NULL, NULL);
#ifdef FINALIZER_COUNTING
	++gc_finalizers_executed;
#endif
}

void 
heap_close (void)
{
	address_list_node* curr = finalizers;

	/* 0. clean up lifespan module */
#ifdef COLLECT_LIFESPAN
#if 0
	fclose(tracefile);
#else
	pclose(tracefile);
#endif
#endif

#if defined(NEW_COLLECT_LIFESPAN)
	lifespan_close();
#endif

#ifdef COLLECT_FRAGMENTATION
	pclose(fragfile);
	pclose(fragsizefile);
#endif

	/* 1. Clean up on the heap... finalize all remaining objects */
#if 1
	while (curr) {
		address_list_node* prev = curr;
		java_objectheader* addr = (java_objectheader*)(curr->address);
		
		if (addr && bitmap_testbit(start_bits, addr))
			heap_call_finalizer_for_object_at(addr);
		
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

	/* 4. emit statistical data */
#ifdef GC_COLLECT_STATISTICS
	sprintf(logtext, "%ld bytes for %ld objects allocated.", 
			gc_alloc_total, gc_alloc_count); 
	dolog();
	sprintf(logtext, "%ld garbage collections performed.", gc_collections_count);
	dolog();
	sprintf(logtext, "%ld heapblocks visited, %ld objects marked", 
			gc_mark_heapblocks_visited, gc_mark_objects_marked);
	dolog();
	sprintf(logtext, "    %ld null pointers.", gc_mark_null_pointer);
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

#ifdef FINALIZER_COUNTING
	sprintf(logtext, "%ld objects with a finalizer", gc_finalizers_detected);
	dolog();

	if (gc_finalizers_detected == gc_finalizers_executed)
		sprintf(logtext, "    all finalizers executed.");
	else
		sprintf(logtext, "    only %ld finalizers executed.", gc_finalizers_executed);
	dolog();
#endif

#if defined(NEW_COLLECT_LIFESPAN) || defined(COLLECT_SIZES)
	lifespan_emit();
#endif
}

inline
static
void 
heap_add_address_to_address_list(address_list_node** list, void* address)
{
	/* Note: address lists are kept sorted to simplify finalization */

	address_list_node* new_node = malloc(sizeof(address_list_node));
	new_node->address = address;
	new_node->next = NULL;

	while (*list && (*list)->next) {
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


inline
static
void 
heap_add_address_to_address_list_unsorted(address_list_node** list, 
										  void* address)
{
	address_list_node* new_node = malloc(sizeof(address_list_node));
	new_node->address = address;
	new_node->next = *list;
	*list = new_node;
}


inline
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

#ifdef COLLECT_LIFESPAN
	fprintf(tracefile, "finalizer\t0x%lx\n", addr);
#endif
}

void* 
heap_allocate (SIZE		  in_length, 
			   bool 	  references, 
			   methodinfo *finalizer)
{
	SIZE	length = align_size(in_length + ((1 << ALIGN) - 1)); 
	void*	free_chunk = NULL;

#if 0
	/* check for misaligned in_length parameter */
	if (length != in_length)
		fprintf(stderr,
				"heap2.c: heap_allocate was passed unaligned in_length parameter: %ld, \n         aligned to %ld. (mistrust)\n",
				in_length, length);
#endif

#ifdef FINALIZER_COUNTING
	if (finalizer)
		++gc_finalizers_detected;
#endif

#if defined(COLLECT_LIFESPAN) || defined(NEW_COLLECT_LIFESPAN)
	/* perform garbage collection to collect data for lifespan analysis */
	if (heap_top > heap_base)
		gc_call();
#endif

#ifdef USE_THREADS
	lock_mutex(&alloc_mutex);
#endif	
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
	/* 3.a. mark all necessary bits, store the finalizer & return the newly allocated block */

	/* I don't mark the object-start anymore, as it always is at the beginning of a free-block,
	   which already is marked (Note: The first free-block gets marked in heap_init). -- phil. */
  	bitmap_setbit(start_bits, free_chunk); /* mark the new object */

#ifndef SIZE_FROM_CLASSINFO
	bitmap_setbit(start_bits, (void*)((long)free_chunk + (long)length)); /* mark the freespace behind the new object */
#endif

	if (references)
		bitmap_setbit(reference_bits, free_chunk);
	else 
		bitmap_clearbit(reference_bits, free_chunk);

	/* store a hint, that there's a finalizer for this address */
	if (finalizer)
		heap_add_finalizer_for_object_at(free_chunk);

#ifdef GC_COLLECT_STATISTICS
	gc_alloc_total += length;
	++gc_alloc_count;
#endif

#ifdef COLLECT_LIFESPAN
	fprintf(tracefile, "alloc\t0x%lx\t0x%lx\n", 
			free_chunk, (long)free_chunk + length);
#endif

#if defined(NEW_COLLECT_LIFESPAN) || defined(COLLECT_SIZES)
	lifespan_alloc(free_chunk, length);
#endif

 failure:
#ifdef USE_THREADS
	unlock_mutex(&alloc_mutex);
#endif	
	return free_chunk;
}

void 
heap_addreference (void **reflocation)
{
	/* I currently use a separate linked list (as in the original code) to hold
	   the global reference locations, but I'll change this to allocate these
	   in blocks on the heap; we'll have to add JIT-Marker code for those Java
	   objects then. -- phil. */

	heap_add_address_to_address_list_unsorted(&references, reflocation);
}

static
inline
void gc_finalize (void)
{
	/* This will have to be slightly rewritten as soon the JIT-marked heap-based lists are used. -- phil. */

	address_list_node*  curr = finalizers;
	address_list_node*  prev;

#if 0
	/* FIXME: new code, please! */
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
}


inline
static 
void gc_reclaim (void)
{
#ifdef PSEUDO_GENERATIONAL
	static void*  generation_start = 0;
	static int    generation_num = 0;
	void* addr = heap_base;
#endif
	void* free_start;
	void* free_end = heap_base;
	BITBLOCK* temp_bits;
	bitmap_t* temp_bitmap;

#ifdef COLLECT_FRAGMENTATION
	unsigned long  free_size = 0;
	unsigned long  free_fragments = 0;
#endif

#ifdef PSEUDO_GENERATIONAL
	if (!generation_start || !(generation_num % 5))
		generation_start = heap_base;

	++generation_num;
#endif

	/* 1. reset the freelists */

#if 0
	allocator_mark_free_kludge(start_bits); /* this line will be kicked out, when
											   the SIZE_FROM_CLASSINFO reclaim
											   is implemented (very soon!!) */
#endif

#ifdef PSEUDO_GENERATIONAL
	for (addr = heap_base; addr <= generation_start; ++addr) {
		if (bitmap_testbit(start_bits, addr))
			bitmap_setbit(mark_bits, addr);
	}

	allocator_mark_free_kludge(start_bits); /* this line will be kicked out, when
											   the SIZE_FROM_CLASSINFO reclaim
											   is implemented (very soon!!) */
#endif

	allocator_reset();

	/* 2. reclaim unmarked objects */
#if 0
	if (!testbit(start_bits, heap_base))
		free_start = heap_base;
	else
		free_start = bitmap_find_next_combination_set_unset(start_bitmap,
															mark_bitmap,
															free_start);

#else
	while (free_end < heap_top) {
		free_start = bitmap_find_next_combination_set_unset(start_bitmap,
															mark_bitmap,
															free_end);

		if (free_start < heap_top) {
			free_end = bitmap_find_next_setbit(mark_bitmap, (void*)((long)free_start + 8)); /* FIXME: constant used */

			if (free_end < heap_top) {
				allocator_free(free_start, (long)free_end - (long)free_start);

#ifdef COLLECT_FRAGMENTATION
				free_size += (long)free_end - (long)free_start;
				free_fragments++;
#endif

#ifdef COLLECT_LIFESPAN
				fprintf(tracefile, 
						"free\t0x%lx\t0x%lx\n", 
						free_start,
						(long)free_end);
#endif

#ifdef NEW_COLLECT_LIFESPAN
				lifespan_free(free_start, free_end);
#endif

#ifndef SIZE_FROM_CLASSINFO
				/* would make trouble with JIT-Marker support. The Marker for unused blocks 
				   might be called, leading to a bad dereference. -- phil. */
				bitmap_setbit(mark_bits, free_start); /* necessary to calculate obj-size bitmap based. */
#endif
			}
		} else {
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

#if 0 /* operation already handled in allocate */
	/* 3.2. mask reference bitmap */
	bitmap_mask_with_bitmap(reference_bitmap, start_bitmap);
#endif

	/* 3.3. update heap_top */
	if (free_start < heap_top) {
		heap_top = free_start;
#ifdef NEW_COLLECT_LIFESPAN
			lifespan_free(free_start, free_end);
#endif
	}

#if 0
	if (heap_top < heap_limit)
		bitmap_setbit(start_bits, heap_top);
#endif

	/* 3.4. emit fragmentation info */
#ifdef COLLECT_FRAGMENTATION
	{
		unsigned long heap_full = (unsigned long)heap_top - (unsigned long)heap_base;
		unsigned long heap_life = (unsigned long)heap_top - (unsigned long)heap_base - free_size;

		fprintf(fragfile, 
				"%ld\t%ld\t%ld\t%ld\t%f\t%f\t%f\n", 
				heap_full,
				heap_life,
				free_size, 
				free_fragments,
				100*(float)free_size/(free_fragments ? free_fragments : 1),
				100*(float)heap_life/(heap_full ? heap_full : 1),
				100*(float)free_size/(heap_full ? heap_full : 1)
				);
	}
	fflush(fragfile);

	allocator_dump_to_file(fragsizefile);
#endif

	/* 4. adjust the collection threshold */
	heap_next_collection = next_collection_heuristic();
	if (heap_next_collection > heap_limit)
		heap_next_collection = heap_limit;

#ifdef COLLECT_LIFESPAN
	fprintf(tracefile, "heap_top\t0x%lx\n", heap_top);
#endif

#ifdef PSEUDO_GENERATIONAL
	generation_start = heap_top;
#endif
}

inline
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
	 * >> LOG: 9301464 bytes for 196724 objects allocated.
	 * >> LOG: 15 garbage collections performed.
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
	 * >> LOG: 9301464 bytes for 196724 objects allocated.
	 * >> LOG: 15 garbage collections performed.
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

	/* 1.a. if addr doesn't point into the heap, return. */
	if ((unsigned long)addr - (unsigned long)heap_base >= 
		((long)heap_top - (long)heap_base)) {
#ifdef GC_COLLECT_STATISTICS
		if (addr == NULL)
			++gc_mark_null_pointer;
		else
			++gc_mark_not_inheap;
#endif
		return;
	}

	/* 1.b. if align(addr) has already been marked during this collection, return. */
	if (bitmap_testbit(mark_bits, (void*)addr)) {
#ifdef GC_COLLECT_STATISTICS
		++gc_mark_already_marked;
#endif
		return;
	}

	/* 1.c. if align(addr) doesn't point to the start of an object, return. */
	if (!bitmap_testbit(start_bits, (void*)addr)) {
#ifdef GC_COLLECT_STATISTICS
		++gc_mark_not_object;
#endif
		return;
	}

	/* 1.d. if addr is not properly aligned, return. */
	if ((long)addr & ((1 << ALIGN) - 1)) {
#ifdef GC_COLLECT_STATISTICS
		++gc_mark_not_aligned;
#endif
		return;
	}

	/* 2. Mark the object at addr */
	bitmap_setbit(mark_bits, (void*)addr); 
#ifdef GC_COLLECT_STATISTICS
	++gc_mark_objects_marked;
#endif

#ifdef JIT_MARKER_SUPPORT
	asm_calljavamethod(addr->vftbl->class->marker, addr, NULL, NULL, NULL);
#else

	/* 3. mark the references contained within the extents of the object at addr */
	if (bitmap_testbit(reference_bits, addr)) {
		/* 3.1. find the end of the object */
		void** end;

#ifdef SIZE_FROM_CLASSINFO
		if (((java_objectheader*)addr)->vftbl == class_array->vftbl)
			end = (void**)((long)addr + (long)((java_arrayheader*)addr)->alignedsize);
		else
			end = (void**)((long)addr + (long)((java_objectheader*)addr)->vftbl->class->alignedsize);
#else
		end = (void**)bitmap_find_next_setbit(start_bitmap, addr + 1); /* points just behind the object */
#endif

		/* 3.2. mark the references within the object at addr */
#ifdef GC_COLLECT_STATISTICS
		gc_mark_heapblocks_visited += ((long)end - (long)addr) >> ALIGN;
#endif
		while (addr < end)
			gc_mark_object_at(*(addr++));
#endif	
	}
	
	return;
}


inline
static
void gc_mark_references (void)
{
	address_list_node* curr = references;

	while (curr) {
#ifdef GC_COLLECT_STATISTICS
		++gc_mark_heapblocks_visited;
#endif		
		gc_mark_object_at(*((void**)(curr->address)));
		curr = curr->next;
	}
}

inline
static
void 
markreferences(void** start, void** end)
{
	while (start < end) {
#ifdef GC_COLLECT_STATISTICS
		++gc_mark_heapblocks_visited;
#endif		
		gc_mark_object_at(*(start++));
	}
}

inline
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
			gc_mark_object_at((void*)aThread);
			if (CONTEXT(aThread).usedStackTop > CONTEXT(aThread).stackEnd)
				markreferences((void**)CONTEXT(aThread).stackEnd,
							   (void**)CONTEXT(aThread).usedStackTop);
			else 	
				markreferences((void**)CONTEXT(aThread).usedStackTop,
							   (void**)CONTEXT(aThread).stackEnd);
	    }

		markreferences((void**)&threadQhead[0],
					   (void**)&threadQhead[MAX_THREAD_PRIO]);
	}
#else
    void **top_of_stack = &dummy;

    if (top_of_stack > stackbottom)
        markreferences(stackbottom, top_of_stack);
	else
        markreferences(top_of_stack, stackbottom);
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
	u1 dummy;

	assert(blockInts == 0);

	intsDisable();
	if (currentThread == NULL || currentThread == mainThread) {
		CONTEXT(mainThread).usedStackTop = &dummy;
		gc_run();
		}
	else
		asm_switchstackandcall(CONTEXT(mainThread).usedStackTop, gc_run,
							   (void**)&(CONTEXT(currentThread).usedStackTop));
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

