/*
 * cacao/mm/allocator.h
 * $Id: allocator.h 115 1999-01-20 01:52:45Z phil $
 */

#ifndef __allocator_h_
#define __allocator_h_

#include "bitmap2.h"

#ifndef YES
#	define YES		1
#endif
#ifndef NO
#	define NO	    0
#endif

#define COUNT_ALLOCATIONS	NO

#define FASTER_FAIL    	NO

#define PRINTF				NO
#define SMALL_TABLES		YES
#define MSB_TO_LSB_SEARCH	NO

#define ADDRESS	64
//#define ADDRESS 32

#if ADDRESS == 64
#  define SIZE	unsigned long long
#else
#  define SIZE	unsigned long
#endif

#undef ALIGN
#define ALIGN	3	/* 64bit alignment */
//#define ALIGN	2	/* 32bit allignment */

#define EXACT_TOP_BIT  8    /* usually somewhere in the range [8..10]
							   the largest exact chunk is 1 << EXACT_TOP_BIT bytes large &
							   there'll be 1 << (EXACT_TOP_BIT - ALIGN) exact bins 
							   
							   Supported values are between 0 and 16...
							   ...any value less than ALIGN mean "no exact bins"...
							   ...actually, there'll be a very small first bin containing
							   chunks exactly allocationblock sized.
				   
							   Setting EXACT_TOP_BIT to anything less than (SUBBIT + ALIGN)
							   doesn't really make sense either, but is possible...
							*/

/* check EXACT_TOP_BIT early ... */

#if (EXACT_TOP_BIT > 16)
#	warning "EXACT_TOP_BIT > 16: bad idea & unsupported value; using 16"
#   undef EXACT_TOP_BIT
#	define EXACT_TOP_BIT 16
#endif


#if EXACT_TOP_BIT >= ALIGN
# define EXACT	(EXACT_TOP_BIT - ALIGN)
#else
# define EXACT  0
#endif
#define LARGE	(ADDRESS - EXACT - ALIGN)

#define SUBBIT  0	/* set this to 0 to disable the additional hashing for the large bins;
					   otherwise, good values are in the range [3..5]; the best setting for
					   a particular application is highly dependent on the VM's heap-size 
					   and the cache sizes. Remember, the allocator will create 
					   (LARGE << SUBBIT) sorted bins. In the case of a 64bit machine with
					   256 exact bins (EXACT_TOP_BIT == 9), a SUBBIT setting of 3 will
					   create 64-9=55 bins consisting of 2^3=8 subbins; thus, a pointer
					   array with 55*8 fields will be allocated, eating up 3520 bytes. */

/* check config */

#if (SUBBIT < 0)
#	warning "SUBBIT < 0: using 0."
#	undef SUBBIT
#	define SUBBIT	0
#endif

#if (SUBBIT > (EXACT_TOP_BIT - ALIGN))
#  warning "SUBBIT > (EXACT_TOP_BIT - ALIGN): wasting subbins!"
#endif

#if (((LARGE << SUBBIT) << ALIGN) > (1 << 16))
#  warning "((LARGE << SUBBIT) << ALIGN) > 64K: are you sure you want it this big??" 
#endif

typedef struct freeblock_exact {
	struct freeblock_exact*	next;
} FREE_EXACT;

typedef struct freeblock_large {
	SIZE					size;
	struct freeblock_large*	next;
} FREE_LARGE;


void allocator_free(void* chunk, SIZE size);
void* allocator_alloc(SIZE size);
void allocator_init(void);
void allocator_reset(void);

void allocator_mark_free_kludge(BITBLOCK* bitmap);

/* unsigned char find_highest(SIZE bits); */

#endif /* !defined(__allocator_h_) */


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

