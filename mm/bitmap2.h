/* 
 * cacao/mm/bitmap.h
 * $Id: bitmap2.h 115 1999-01-20 01:52:45Z phil $ 
 */

#ifndef __mm_bitmap_h_
#define __mm_bitmap_h_

#include "mm.h"

#if U8_AVAILABLE
#   define BITBLOCK		u8
#	define OFFSET_T		u8
#else
#	define BITBLOCK		u4
#	define OFFSET_T		u4
#endif

typedef struct {
	BITBLOCK*	    bitmap;			/* accessor, usually copied */
	unsigned long	bytesize;       /* used internally */
	void*           bitmap_beyond_addr;
	OFFSET_T		bitmap_top_block;
	void*           bitmap_memory;  /* internal: the real address */
} bitmap_t;

void       bitmap_setbit(BITBLOCK* bitmap, void* addr);
void       bitmap_clearbit(BITBLOCK* bitmap, void* addr);
bool       bitmap_testbit(BITBLOCK* bitmap, void* addr);

void       bitmap_checking_setbit(bitmap_t* bitmap, void* addr);
void       bitmap_checking_clearbit(bitmap_t* bitmap, void* addr);
bool       bitmap_checking_testbit(bitmap_t* bitmap, void* addr);

void	      bitmap_clear(bitmap_t* bitmap);
bitmap_t*  bitmap_allocate(void* zero_offset, OFFSET_T size);
void       bitmap_release(bitmap_t* bitmap);

void*      bitmap_find_next_setbit(bitmap_t* bitmap, void* addr);
void*      bitmap_find_next_combination_set_unset(bitmap_t* bitmap, bitmap_t* invertedmap, void* addr);

void       bitmap_mask_with_bitmap(bitmap_t* bitmap, bitmap_t* mask);
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




