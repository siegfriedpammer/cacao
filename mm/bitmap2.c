/*
 * cacao/mm/bitmap2.c
 *
 * Copyright (c) 1998 A. Krall, R. Grafl, M. Gschwind, M. Probst, P. Tomsich
 * See file COPYRIGHT for information on usage and disclaimer of warranties
 *
 * Authors: Philipp Tomsich     EMAIL: cacao@complang.tuwien.ac.at
 *
 * $Id: bitmap2.c 37 1998-11-04 12:39:19Z phil $
 */

/*
 * This file contains the new bitmap management; it's used within the garbage 
 * collector and allocator to maintain and access the objectstart-, marker- 
 * and reference-bitmaps.
 *
 * Notes:
 * 1. For efficencies sake, the memory used for the bitmap is never accessed 
 *    directly using a heap-offset, but through a pointer offset at bitmap 
 *    creation; this saves all the subtractions and additions necessary to 
 *    convert between an address on the heap and an offset into the heap.
 *
 * 2. I finally added a bitmap_t structure to encapsulate some state 
 *    information. It contains everything necessary to access the bitmap, 
 *    caches a few precomputable values and allows for freeing the bitmap.
 *
 * 3. There's no checks whatsoever to determine whether an address passed is 
 *    valid or not. These were removed, in order to allow passing the offset 
 *    address of the bitmap instead of the bitmap structure.
 *    I.e., checking addresses to determine whether they are pointing onto 
 *          the heap must be done before passing them to the bitmap management
 *          functions.
 *
 * 4. Just added exceptions to note 3: Now we have new functions checking the 
 *    validity of the address to aid in debugging. They are named 
 *    bitmap_checking_{set|clear|test}bit and will cause an assertion to fail 
 *    if the addr argument is out of range.
 */

#include <assert.h>  /* assert */
#include <stdio.h>   /* printf, fprintf */
#include <stdlib.h>  /* malloc, free */
#include <string.h>  /* memset */
#include <unistd.h>  /* exit */

#include "bitmap2.h"

#warning "bitmap2.c untested in 32-bit mode -- phil."

/* FIXME: I haven't decided, whether to keep these macro definitions here or 
 *        move them to the header file; they are implementation details, so 
 *        they should stay here, but moving them would make the SETBIT,
 *        TESTBIT and CLEARBIT macros available to the general public. --phil.
 */

/* FIXME: HEAPBLOCK_SIZE is an alias for POINTER_ALIGNMENT and should be 
 *        removed. --phil. 
 */

/* Please note: All values are log2 */
#if U8_AVAILABLE
#define POINTER_ALIGNMENT  			3  /* alignment of pointers onto the 
										  heap == heapblocksize */
#define BITBLOCK_SIZE				3  /* bytes/BITBLOCK */
#else
#define POINTER_ALIGNMENT  			2
#define BITBLOCK_SIZE				2
#endif

#define HEAPBLOCK_SIZE				POINTER_ALIGNMENT
#define BITS_PER_BYTE				3  /* actually quite self-explanatory */
#define HEAPBLOCKS_PER_BITBLOCK		(BITBLOCK_SIZE + BITS_PER_BYTE)

#define ADDR_TO_BLOCK(addr)			((OFFSET_T)(addr) >> (HEAPBLOCKS_PER_BITBLOCK + POINTER_ALIGNMENT))
#define ADDR_TO_OFFSET(addr)		(((OFFSET_T)(addr) & ((1 << (HEAPBLOCKS_PER_BITBLOCK + POINTER_ALIGNMENT)) - 1)) >> HEAPBLOCK_SIZE)
#define BLOCK_TO_ADDR(block)	  	((block) << (HEAPBLOCKS_PER_BITBLOCK + POINTER_ALIGNMENT))
#define OFFSET_TO_ADDR(offset)		((offset) << HEAPBLOCK_SIZE)
#define CALC_BITMAP_SIZE(size)		(((size) + ((1 << (POINTER_ALIGNMENT + HEAPBLOCK_SIZE)) - 1)) >> BITS_PER_BYTE)

#define CHECK_ADDR_ALIGNMENT(addr)	{ assert(!((OFFSET_T)addr & ((1 << POINTER_ALIGNMENT) - 1))); }
#define CALC_ZERO_OFFSET(zero)		(((OFFSET_T)(zero) >> (HEAPBLOCK_SIZE + BITS_PER_BYTE)))

#define BIT_AT_OFFSET(offset)		((BITBLOCK)1 << (offset))
#define BIT_FOR_ADDR(addr)			BIT_AT_OFFSET(ADDR_TO_OFFSET((addr)))

#define SETBIT(bitmap, addr)		{ bitmap[ADDR_TO_BLOCK(addr)] |=  BIT_FOR_ADDR(addr); }
#define CLEARBIT(bitmap, addr)   	{ bitmap[ADDR_TO_BLOCK(addr)] &= ~BIT_FOR_ADDR(addr); }
#define TESTBIT(bitmap, addr)   	( (bitmap[ADDR_TO_BLOCK(addr)] & BIT_FOR_ADDR(addr)) != 0 )


/* Operations per macro (relative cost):
 *  
 *   Macro            | & | &= | |= | ~ | >> | << | + | dereference |
 *   -----------------+---+----+----+---+----+----+---+-------------+
 *   ADDR_TO_BLOCK    | 0 |  0 |  0 | 0 |  1 |  0 | 0 |           0 |
 *   ADDR_TO_OFFSET   | 1 |  0 |  0 | 0 |  1 |  0 | 0 |           0 |
 *   BLOCK_TO_ADDR    | 0 |  0 |  0 | 0 |  0 |  1 | 0 |           0 |
 *   OFFSET_TO_ADDR   | 0 |  0 |  0 | 0 |  0 |  1 | 0 |           0 |
 *   CALC_BITMAP_SIZE | 0 |  0 |  0 | 0 |  0 |  1 | 0 |           0 |
 *   CALC_ZERO_OFFSET | 0 |  0 |  0 | 0 |  1 |  0 | 0 |           0 |
 *   -----------------+---+----+----+---+----+----+---+-------------+
 *   BIT_AT_OFFSET    | 0 |  0 |  0 | 0 |  0 |  1 | 0 |           0 |
 *   BIT_FOR_ADDR     | 1 |  0 |  0 | 0 |  1 |  1 | 0 |           0 |
 *   -----------------+---+----+----+---+----+----+---+-------------+
 *   SETBIT           | 1 |  0 |  1 | 0 |  2 |  1 | 0 |           1 |
 *   CLEARBIT         | 1 |  1 |  0 | 1 |  2 |  1 | 0 |           1 |
 *   TESTBIT          | 2 |  0 |  0 | 0 |  2 |  1 | 0 |           1 |
 */


__inline__
void bitmap_setbit(BITBLOCK* bitmap, 
				   void* addr)
{ 
	SETBIT(bitmap, addr); 
}

__inline__
void bitmap_clearbit(BITBLOCK* bitmap, 
					 void* addr)
{ 
	CLEARBIT(bitmap, addr); 
}

__inline__
bool bitmap_testbit(BITBLOCK* bitmap, 
					void* addr)
{ 
	return TESTBIT(bitmap, addr); 
}

__inline__
static
void bitmap_boundscheck(bitmap_t* bitmap,
						void*     addr)
{
	/* check the lower bound */
	assert((void*)&bitmap->bitmap[ADDR_TO_BLOCK(addr)] >= bitmap->bitmap_memory);
	/* check the upper bound */
	assert(&bitmap->bitmap[
						   ADDR_TO_BLOCK(addr)] 
		   <= &bitmap->bitmap[bitmap->bitmap_top_block]);
	assert(addr < bitmap->bitmap_beyond_addr); /* a stricter way to check the upper bound */
}

__inline__
void bitmap_checking_setbit(bitmap_t* bitmap,
							void*	  addr)
{
	bitmap_boundscheck(bitmap, addr);
	bitmap_setbit(bitmap->bitmap, addr);
}

__inline__
void bitmap_checking_clearbit(bitmap_t* bitmap,
							  void*	  addr)
{
	bitmap_boundscheck(bitmap, addr);
	bitmap_clearbit(bitmap->bitmap, addr);
}

__inline__
bool bitmap_checking_testbit(bitmap_t* bitmap,
							 void*	  addr)
{
	bitmap_boundscheck(bitmap, addr);
	return bitmap_testbit(bitmap->bitmap, addr);
}

__inline__
void bitmap_clear(bitmap_t* bitmap)	
{
	memset(bitmap->bitmap_memory, 0, bitmap->bytesize);
}

__inline__
bitmap_t* bitmap_allocate(void* 	zero_addr, 
						  OFFSET_T	size)
{
	bitmap_t*  bitmap;

	unsigned long bytesize = CALC_BITMAP_SIZE(size);                                         /* align */
	bitmap = (bitmap_t*)malloc(sizeof(bitmap_t));

	CHECK_ADDR_ALIGNMENT(zero_addr);                        /* ensure correct alignment for zero_addr */

	bitmap->bytesize = bytesize;
	bitmap->bitmap_memory = malloc(bytesize);
	bitmap->bitmap = bitmap->bitmap_memory - CALC_ZERO_OFFSET(zero_addr);   /* offset for fast access */

	bitmap->bitmap_beyond_addr = zero_addr + size;
	bitmap->bitmap_top_block = ADDR_TO_BLOCK(bitmap->bitmap_beyond_addr - sizeof(BITBLOCK));

	if (!bitmap->bitmap_memory) {
		/* handle error -- unable to allocate enough memory */
		fprintf(stderr, "bitmap2.c: Failed to allocate memory for a bitmap.\n");
		exit(-1);  /* FIXME: I should probably call some sort of cacao-wide "panic" function ?! -- phil. */
	}

	bitmap_clear(bitmap);
	
	return bitmap;
}

__inline__
void bitmap_release(bitmap_t* bitmap)
{
	if (bitmap) {
		if (bitmap->bitmap_memory)
			free(bitmap->bitmap_memory);
		free(bitmap);
	}
}


__inline__
static 
int offset_for_lowest(BITBLOCK i)
{
	/* Maintainer, don't despair! 
	 *
	 * The OFFSET_TO_ADDR adjusts an argument according to the size of heap-allocation blocks,
	 * so that the resulting value can be used as an offset to a pointer.
	 * I.e.: addr + OFFSET_TO_ADDR(i) is equivalent to addr + (sizeof(heap_block) * i)
	 *
	 * The definition of lowest just looks inefficient, everything should be expanded at
	 * compile-time, though. -- phil.
	 */

	static signed char lowest[256] = { 
		OFFSET_TO_ADDR(0),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0, 
		OFFSET_TO_ADDR(3),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0, 
		OFFSET_TO_ADDR(4),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0,
		OFFSET_TO_ADDR(3),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0,
		OFFSET_TO_ADDR(5),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0, 
		OFFSET_TO_ADDR(3),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0, 
		OFFSET_TO_ADDR(4),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0,
		OFFSET_TO_ADDR(3),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0,
		OFFSET_TO_ADDR(6),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0, 
		OFFSET_TO_ADDR(3),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0, 
		OFFSET_TO_ADDR(4),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0,
		OFFSET_TO_ADDR(3),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0,
		OFFSET_TO_ADDR(5),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0, 
		OFFSET_TO_ADDR(3),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0, 
		OFFSET_TO_ADDR(4),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0,
		OFFSET_TO_ADDR(3),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0,
		OFFSET_TO_ADDR(7),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0, 
		OFFSET_TO_ADDR(3),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0, 
		OFFSET_TO_ADDR(4),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0,
		OFFSET_TO_ADDR(3),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0,
		OFFSET_TO_ADDR(5),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0, 
		OFFSET_TO_ADDR(3),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0, 
		OFFSET_TO_ADDR(4),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0,
		OFFSET_TO_ADDR(3),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0,
		OFFSET_TO_ADDR(6),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0, 
		OFFSET_TO_ADDR(3),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0, 
		OFFSET_TO_ADDR(4),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0,
		OFFSET_TO_ADDR(3),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0,
		OFFSET_TO_ADDR(5),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0, 
		OFFSET_TO_ADDR(3),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0, 
		OFFSET_TO_ADDR(4),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0,
		OFFSET_TO_ADDR(3),  0, OFFSET_TO_ADDR(1), 0, OFFSET_TO_ADDR(2), 0, OFFSET_TO_ADDR(1), 0,
	};

#if U8_AVAILABLE
	if (i & 0xffffffffffULL)
#endif
		if (i & 0xffffUL)
			if (i & 0xffUL) 
				return OFFSET_TO_ADDR(0) + lowest[(unsigned char)i];
			else      		 
				return OFFSET_TO_ADDR(8) + lowest[(unsigned char)(i>>8)];
		else
			if (i & 0xff0000UL) 
				return OFFSET_TO_ADDR(16) + lowest[(unsigned char)(i>>16)];
			else      		 
				return OFFSET_TO_ADDR(24) + lowest[(unsigned char)(i>>24)];
#if U8_AVAILABLE
	else
		if (i & 0xffff00000000ULL)
			if (i & 0xff00000000ULL) 
				return OFFSET_TO_ADDR(32) + lowest[(unsigned char)(i>>32)];
			else      		 
				return OFFSET_TO_ADDR(40) + lowest[(unsigned char)(i>>40)];
		else
			if (i & 0xff000000000000ULL) 
				return OFFSET_TO_ADDR(48) + lowest[(unsigned char)(i>>48)];
			else      		 
				return OFFSET_TO_ADDR(56) + lowest[(unsigned char)(i>>56)];
#endif
}

__inline__
void*
bitmap_find_next_setbit(bitmap_t* bitmap, 
						void*     addr)
{
	OFFSET_T  block = ADDR_TO_BLOCK(addr);
	OFFSET_T  offset = ADDR_TO_OFFSET(addr);
	BITBLOCK  pattern;

	bitmap_boundscheck(bitmap, addr);

	/* 1. check the current block, starting from the bit indicated by addr */
	pattern = bitmap->bitmap[block] >> offset;
	if (pattern)
		return (void*)(addr + offset_for_lowest(pattern));

	/* 2. iteratively check block by block until the end of the bitmap */
	while (block < bitmap->bitmap_top_block) {
		pattern = bitmap->bitmap[++block];

		if (pattern)
			return (void*)(BLOCK_TO_ADDR(block) + offset_for_lowest(pattern));
	}

	/* 3. failed to find a set bit... */
	return bitmap->bitmap_beyond_addr;
}

__inline__
void*
bitmap_find_next_combination_set_unset(bitmap_t* bitmap, 
									   bitmap_t* invertedmap, 
									   void*     addr)
{
	OFFSET_T  block = ADDR_TO_BLOCK(addr);
	OFFSET_T  offset = ADDR_TO_OFFSET(addr);
	BITBLOCK  pattern;

	bitmap_boundscheck(bitmap, addr);

	/* 1. check the current block, starting from the bit indicated by addr */
	pattern = (bitmap->bitmap[block] & ~invertedmap->bitmap[block]) >> offset;
	if (pattern)
		return (void*)(addr + offset_for_lowest(pattern));

	/* 2. iteratively check block by block until the end of the bitmap */
	while (block < bitmap->bitmap_top_block) {
		pattern = bitmap->bitmap[++block] & ~invertedmap->bitmap[block];

		if (pattern)
			return (void*)(BLOCK_TO_ADDR(block) + offset_for_lowest(pattern));
	}

	/* 3. failed to find a combination... */
	return bitmap->bitmap_beyond_addr;
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
