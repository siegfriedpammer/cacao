/*
 * cacao/mm/allocator.c
 *
 * Copyright (c) 1998 A. Krall, R. Grafl, M. Gschwind, M. Probst, Ph. Tomsich
 * See file COPYRIGHT for information on usage and disclaimer of warranties
 *
 * Authors: Philipp Tomsich     EMAIL: cacao@complang.tuwien.ac.at
 *
 * $Id: allocator.c 93 1998-11-25 11:49:36Z phil $
 */

/* 
   NOTES: 

   Locality preservation has not been added... yet
*/

#include "allocator.h"

static FREE_EXACT*  __freelist_exact[1 << EXACT];
static FREE_EXACT** freelist_exact = &__freelist_exact[-1];
static FREE_LARGE*  freelist_large[LARGE << SUBBIT];

#if SMALL_TABLES
static unsigned char  highest[256];
#else
#	if EXACT_TOP_BIT < 8
#       if (ADDRESS == 64)
static unsigned char  highest_0[2048];
#       else
static unsigned char  highest_0[1024];
#       endif

static unsigned char* highest_8  = &highest_0[256];
static unsigned char* highest_16 = &highest_0[512];
static unsigned char* highest_24 = &highest_0[768];
#       if (ADDRESS == 64)
static unsigned char* highest_32 = &highest_0[1024];
static unsigned char* highest_40 = &highest_0[1280];
static unsigned char* highest_48 = &highest_0[1538];
static unsigned char* highest_56 = &highest_0[1792];
#       endif
#	else
#       if (ADDRESS == 64)
static unsigned char  highest_8[1792];
#       else
static unsigned char  highest_8[768];
#       endif

static unsigned char* highest_16 = &highest_8[256];
static unsigned char* highest_24 = &highest_8[512];
#       if (ADDRESS == 64)
static unsigned char* highest_32 = &highest_8[768];
static unsigned char* highest_40 = &highest_8[1280];
static unsigned char* highest_48 = &highest_8[1538];
static unsigned char* highest_56 = &highest_8[1792];
#       endif
#	endif
#endif

static
unsigned char 
find_highest(SIZE bits){
#if MSB_TO_LSB_SEARCH
	/* optimize for large allocation sizes */
#	if SMALL_TABLES
#		if ADDRESS == 64
			if (bits & 0xff00000000000000ULL)
				return 56 + highest[bits >> 56 & 0xff];
			if (bits & 0xff000000000000ULL)
				return 48 + highest[bits >> 48 & 0xff];
			if (bits & 0xff0000000000ULL)
				return 40 + highest[bits >> 40 & 0xff];
			if (bits & 0xff00000000ULL)
				return 32 + highest[bits >> 32 & 0xff];
#		endif

			if (bits & 0xff000000)
				return 24 + highest[bits >> 24 & 0xff];
			if (bits & 0xff0000)
				return 16 + highest[bits >> 16 & 0xff];
			if (bits & 0xff00)
				return 8 + highest[bits >> 8 & 0xff];

#   	if EXACT_TOP_BIT < 8
			return highest[bits & 0xff];
#		endif
#	else
#		if ADDRESS == 64
			if (bits & 0xff00000000000000ULL)
				return highest_56[bits >> 56 & 0xff];
			if (bits & 0xff000000000000ULL)
				return highest_48[bits >> 48 & 0xff];
			if (bits & 0xff0000000000ULL)
				return highest_40[bits >> 40 & 0xff];
			if (bits & 0xff00000000ULL)
				return highest_32[bits >> 32 & 0xff];
#		endif

			if (bits & 0xff000000)
				return highest_24[bits >> 24 & 0xff];
			if (bits & 0xff0000)
				return highest_16[bits >> 16 & 0xff];
			if (bits & 0xff00)
				return highest_8[bits >> 8 & 0xff];

#   	if EXACT_TOP_BIT < 8
			return highest_0[bits & 0xff];
#		endif			
#	endif

#else
	/* optimize for small allocation sizes */

#	if SMALL_TABLES
#   	if EXACT_TOP_BIT < 8
			if (!(bits & ~0xffULL))
				return highest[bits & 0xff];
#		endif

			if (!(bits & ~0xffffULL))
				return 8 + highest[bits >> 8 & 0xff];

			if (!(bits & ~0xffffffULL))
				return 16 + highest[bits >> 16 & 0xff];

#		if	ADDRESS == 32
			return 24 + highest[bits >> 24 & 0xff];
#		else
			if (!(bits & ~0xffffffffULL))
				return 24 + highest[bits >> 24 & 0xff];

			if (!(bits & ~0xffffffffffULL))
				return 32 + highest[bits >> 32 & 0xff];

			if (!(bits & ~0xffffffffffffULL))
				return 40 + highest[bits >> 40 & 0xff];

			if (!(bits & ~0xffffffffffffffULL))
				return 48 + highest[bits >> 48 & 0xff];

			return 56 + highest[bits >> 56 & 0xff];
#		endif
#	else
#   	if EXACT_TOP_BIT < 8
			if (!(bits & ~0xffULL))
				return highest_0[bits & 0xff];
#		endif

			if (!(bits & ~0xffffULL))
				return highest_8[bits >> 8 & 0xff];

			if (!(bits & ~0xffffffULL))
				return highest_16[bits >> 16 & 0xff];

#		if	ADDRESS == 32
			return highest_24[bits >> 24 & 0xff];
#		else
			if (!(bits & ~0xffffffffULL))
				return highest_24[bits >> 24 & 0xff];

			if (!(bits & ~0xffffffffffULL))
				return highest_32[bits >> 32 & 0xff];

			if (!(bits & ~0xffffffffffffULL))
				return highest_40[bits >> 40 & 0xff];

			if (!(bits & ~0xffffffffffffffULL))
				return highest_48[bits >> 48 & 0xff];

			return highest_56[bits >> 56 & 0xff];
#		endif
#	endif
#endif

};

static 
void 
init_table()
{
  static unsigned char r[10] = { 1, 1, 2, 4, 8, 16, 32, 64, 128 };
  int i, j, k, m;
  
  i = 0;
  j = 0;
  k = 0;
  m = 0;

  for (k = 0; k < 10; ++k) {
	  for (m = 0; m < r[k]; ++m) {
#if SMALL_TABLES
		  highest[i]  = j - (EXACT + ALIGN) - 1;
#else
#	if (EXACT_TOP_BIT < 8)
		  highest_0 [i]  = j - 8 - (EXACT + ALIGN) - 1;
#	endif
		  highest_8 [i]  = j + 8 - (EXACT + ALIGN) - 1;
		  highest_16 [i] =  8 + highest_8 [i];
		  highest_24 [i] = 16 + highest_8 [i];			
#   if (ADDRESS == 64)
		  highest_32 [i] = 24 + highest_8 [i];
		  highest_40 [i] = 32 + highest_8 [i];
		  highest_48 [i] = 40 + highest_8 [i];
		  highest_56 [i] = 48 + highest_8 [i];
#   endif
#endif
      
		  ++i;
	  }
	  
	  ++j;
  }
}

#define PRESERVE_LOCALITY 0

#if FASTER_FAIL
static SIZE	allocator_largest_free = 0;
#endif

#if PRESERVE_LOCALITY
static void* allocator_last_splitoff = 0;
static SIZE  allocator_last_splitoff_size = 0;
#endif

void
allocator_reset()
{
	int i;

#if FASTER_FAIL
	allocator_largest_free = 0;
#endif

#if PRESERVE_LOCALITY
	allocator_last_splitoff = 0;
	allocator_last_splitoff_size = 0;
#endif

  for (i = 1; i <= (1 << EXACT); ++i)
	  freelist_exact[i] = 0;
  
  for (i = 0; i < (LARGE << SUBBIT); ++i)
	  freelist_large[i] = 0;
}


#include <stdio.h>


#if COUNT_ALLOCATIONS
static unsigned long long 	allocator_allocations = 0;

void
allocator_display_count()
{
	fprintf(stderr, "total allocations: %d\n", allocator_allocations);
}
#endif


void 
allocator_init()
{
#if 0
	fprintf(stderr, 
			"allocator_init: $Id: allocator.c 93 1998-11-25 11:49:36Z phil $\n\n");
	
	fprintf(stderr, "\t%d bit addresses\n", ADDRESS);
	fprintf(stderr, "\t%d bit alignment\n", ALIGN);
	fprintf(stderr, "\t%d exact bits\n", EXACT );
	fprintf(stderr, "\t%d large bits\n", LARGE );
	fprintf(stderr, "\t%d subbin bits\n", SUBBIT );
	fprintf(stderr, "\tusing %s tables\n", 
			SMALL_TABLES ? "small (256 byte)" : "large (2048 byte)");
	fprintf(stderr, "\tusing %s highest-bit search strategy\n",
			MSB_TO_LSB_SEARCH ? "MSB-to-LSB" : "LSB-to-MSB");
	
	fprintf(stderr, 
			"\tusing %d bytes for exact bins, %d bytes for sorted bins\n\n",
			(1 << EXACT) << ALIGN,
			(LARGE << SUBBIT) << ALIGN);
#endif
	
	init_table();		/* set up the highest-table */
	allocator_reset();	/* clear the freelists & internal variables */
}

int
size_to_bin(SIZE  size)
{
#if PRINTF
	printf ("size: %d ", size);
#endif
  
#if EXACT > 0
	if (size > ((1 << EXACT) << ALIGN)) {
#endif
		// unsigned char	shift = shift_table_32[highbit];
		// int		bin = offset_table_32[highbit] + ( (size >> shift) & mask );
#if SUBBIT != 0
		unsigned char	highbit = find_highest(size);
		unsigned char	shift = highbit + (EXACT + ALIGN - SUBBIT);
		static int   	mask = (1 << SUBBIT) - 1;
		int				bin = (highbit << SUBBIT) + ( (size >> shift) & mask );

		// printf("shift: %d, %d; %d; large bin: %d\n", shift, (size >> shift) & mask, highbit,  bin);
#else
		int				bin = find_highest(size);
#endif

#if PRINTF
		printf("==> large bin: %d\n",  bin);
#endif
    
		return bin;
#if EXACT > 0
	} else {
#if PRINTF
		printf("==> exact bin: %d\n", (int)((size >> ALIGN) - 1));
#endif
    
		return (size >> ALIGN);
	}
#endif
}

#include <assert.h>


#define OLDEST_FIRST   1



/* FIXME: 
   Should we enforce an additional "lowest address first" - ordering
   for the freelist-entries ??? Right now, I'm using oldest-first for 
   the large bins and newest-first for the exact bins...

   Sorting by the address may reduce heap fragmentation by reducing the
   heap growth-rate... */

void
allocator_free(void* chunk, SIZE size)
{
	assert(chunk && size);

#if FASTER_FAIL
	if (size > allocator_largest_free)
		allocator_largest_free = size;
#endif

	if (size > ((1 << EXACT) << ALIGN)) {
		int 			bin = size_to_bin(size);
		FREE_LARGE*		curr = freelist_large[bin];

		((FREE_LARGE*)chunk)->size = size;

		if (!freelist_large[bin] || (freelist_large[bin]->size > size)) {
			/* prepend */
			((FREE_LARGE*)chunk)->next = freelist_large[bin];
			freelist_large[bin] = (FREE_LARGE*)chunk;
			return;
		}

		while ((curr->next) && (curr->next->size <= size)) {
			curr = curr->next;
		};

		/* insert or append */
		((FREE_LARGE*)chunk)->next = curr->next;
		curr->next=(FREE_LARGE*)chunk;
	} else {
		int bin = size >> ALIGN;

#if OLDEST_FIRST
		if (!freelist_exact[bin]) {
#endif
			/* simply prepend */
			((FREE_EXACT*)chunk)->next = freelist_exact[bin];
			freelist_exact[bin] = (FREE_EXACT*)chunk;
#if OLDEST_FIRST
		} else {
			FREE_EXACT*  curr = (FREE_EXACT*)freelist_exact[bin];
			while (curr->next && (curr < (FREE_EXACT*)chunk))
				curr = curr->next;

			/* insert */
			((FREE_EXACT*)chunk)->next = curr->next;
			curr->next = (FREE_EXACT*)chunk;			
		}
#endif
	}

	return;
}

void*
allocator_alloc(SIZE size)
{
	void*	result = NULL;
	int     maxbin;
	int		bin;

#if COUNT_ALLOCATIONS
	allocator_allocations++;
#endif

#if FASTER_FAIL
	if (size > allocator_largest_free)
		return NULL;
#endif

#if PRESERVE_LOCALITY
	allocator_last_splitoff = 0;
	allocator_last_splitoff_size = 0;
#endif

	if (size <= ((1 << EXACT) << ALIGN)) {
		/* search the exact bins */
		bin = size >> ALIGN;
		maxbin = 1 << EXACT;

		while (bin <= maxbin) {
			if (freelist_exact[bin]) {
				/* unlink */
				result = (void*)freelist_exact[bin];
				freelist_exact[bin]=freelist_exact[bin]->next;

				/* split */
				if ((bin << ALIGN) > size)
					allocator_free((void*)((long)result + size), (bin << ALIGN) - size);

				//				fprintf(stderr, "found chunk in exact bin %d\n", bin);

				return result;
			} else
				++bin;
		}

		bin = 0;
	} else {
		bin = size_to_bin(size);
	}

	maxbin = LARGE << SUBBIT;
	
	while (bin < maxbin) {
		if (freelist_large[bin]) {
			FREE_LARGE*  chunk = freelist_large[bin];

			//			fprintf(stderr, "found chunk in large bin %d\n", bin);

			if (chunk->size >= size) {
				/* unlink */
				freelist_large[bin] = chunk->next;

				/* split */
				if (chunk->size > size)
					allocator_free((void*)((long)chunk + size), (chunk->size - size));
				
				return (void*)chunk;
			}

			while (chunk->next && (chunk->next->size < size))
				chunk = chunk->next;

			if (chunk->next) {
				/* unlink */
				FREE_LARGE*  new_chunk = chunk->next;				
				chunk->next = new_chunk->next;

				/* split */
				if (new_chunk->size > size)
					allocator_free((void*)((long)new_chunk + size), (new_chunk->size - size));
				
				return (void*)new_chunk;
			}
		}
		
		++bin;
	}

	return result;  /* NULL */
}

void
allocator_dump()
{
	int i;

	for (i = 1; i <= 1 << EXACT; ++i) {
		int			count = 0;
		FREE_EXACT*	chunk = freelist_exact[i];

		while (chunk) {
			chunk = chunk->next;
			++count;
		}

		if (count)
			printf("exact bin %d (%d byte blocks): %d\n", i, i * (1 << ALIGN), count);
	}
	for (i = 0; i < LARGE << SUBBIT; ++i) {
		int			count = 0;
		FREE_LARGE*	chunk = freelist_large[i];

		if (chunk) 
			printf("large bin %d:\n", i);

		while (chunk) {
			printf("\t%d bytes @ 0x%lx\n", chunk->size, chunk);
			chunk = chunk->next;
			++count;
		}
	}

	printf("dump complete.\n");
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
