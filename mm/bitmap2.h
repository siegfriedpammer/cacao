/* 
 * cacao/mm/bitmap.h
 * $Id: bitmap2.h 93 1998-11-25 11:49:36Z phil $ 
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

__inline__ void       bitmap_setbit(BITBLOCK* bitmap, void* addr);
__inline__ void       bitmap_clearbit(BITBLOCK* bitmap, void* addr);
__inline__ bool       bitmap_testbit(BITBLOCK* bitmap, void* addr);

__inline__ void       bitmap_checking_setbit(bitmap_t* bitmap, void* addr);
__inline__ void       bitmap_checking_clearbit(bitmap_t* bitmap, void* addr);
__inline__ bool       bitmap_checking_testbit(bitmap_t* bitmap, void* addr);

__inline__ void	      bitmap_clear(bitmap_t* bitmap);
__inline__ bitmap_t*  bitmap_allocate(void* zero_offset, OFFSET_T size);
__inline__ void       bitmap_release(bitmap_t* bitmap);

__inline__ void*      bitmap_find_next_setbit(bitmap_t* bitmap, void* addr);
__inline__ void*      bitmap_find_next_combination_set_unset(bitmap_t* bitmap, bitmap_t* invertedmap, void* addr);

__inline__ void       bitmap_mask_with_bitmap(bitmap_t* bitmap, bitmap_t* mask);
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




