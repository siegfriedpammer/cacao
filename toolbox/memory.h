/************************* toolbox/memory.h ************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Macros for memory management

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1996/10/03

*******************************************************************************/

#define CODEMMAP

#ifdef USE_BOEHM
/* Uncollectable memory which can contain references */
void *heap_alloc_uncollectable(u4 bytelen);
#define GCNEW(type,num) heap_alloc_uncollectable(sizeof(type) * (num))
#endif

#define ALIGN(pos,size)       ( ( ((pos)+(size)-1) / (size))*(size) )
#define PADDING(pos,size)     ( ALIGN((pos),(size)) - (pos) )
#define OFFSET(s,el)          ( (int) ( (size_t) &( ((s*)0) -> el ) ) )


#define NEW(type)             ((type*) mem_alloc ( sizeof(type) ))
#define FREE(ptr,type)        mem_free (ptr, sizeof(type) )

#define LNEW(type)             ((type*) lit_mem_alloc ( sizeof(type) ))
#define LFREE(ptr,type)        lit_mem_free (ptr, sizeof(type) )

#define MNEW(type,num)        ((type*) mem_alloc ( sizeof(type) * (num) ))
#define MFREE(ptr,type,num)   mem_free (ptr, sizeof(type) * (num) )
#define MREALLOC(ptr,type,num1,num2) mem_realloc (ptr, sizeof(type) * (num1), \
                                                       sizeof(type) * (num2) )

#define DNEW(type)            ((type*) mem_alloc ( sizeof(type) ))
#define DMNEW(type,num)       ((type*) mem_alloc ( sizeof(type) * (num) ))
#define DMREALLOC(ptr,type,num1,num2)  mem_realloc (ptr, sizeof(type)*(num1),\
                                                       sizeof(type) * (num2) )

#define MCOPY(dest,src,type,num)  memcpy (dest,src, sizeof(type)* (num) )

#ifdef CODEMMAP
#define CNEW(type,num)        ((type*) mem_mmap ( sizeof(type) * (num) ))
#define CFREE(ptr,num)
#else
#define CNEW(type,num)        ((type*) mem_alloc ( sizeof(type) * (num) ))
#define CFREE(ptr,num)        mem_free (ptr, num)
#endif

void *mem_alloc(int length);
void *mem_mmap(int length);
void *lit_mem_alloc(int length);
void mem_free(void *m, int length);
void lit_mem_free(void *m, int length);
void *mem_realloc(void *m, int len1, int len2);
long int mem_usage();

void *dump_alloc(int length);
void *dump_realloc(void *m, int len1, int len2);
long int dump_size();
void dump_release(long int size);

void mem_usagelog(int givewarnings);
 
 
 
/* 
---------------------------- Interface description -----------------------

There are two possible choices for allocating memory:

	1.   explicit allocating / deallocating

			mem_alloc ..... allocate a memory block 
			mem_free ...... free a memory block
			mem_realloc ... change size of a memory block (position may change)
			mem_usage ..... amount of allocated memory


	2.   explicit allocating, automatic deallocating
	
			dump_alloc .... allocate a memory block in the dump area
			dump_realloc .. change size of a memory block (position may change)
			dump_size ..... marks the current top of dump
			dump_release .. free all memory requested after the mark
			                
	
There are some useful macros:

	NEW (type) ....... allocate memory for an element of type `type`
	FREE (ptr,type) .. free memory
	
	MNEW (type,num) .. allocate memory for an array
	MFREE (ptr,type,num) .. free memory
	
	MREALLOC (ptr,type,num1,num2) .. enlarge the array to size num2
	                                 
These macros do the same except they operate on the dump area:
	
	DNEW,  DMNEW, DMREALLOC   (there is no DFREE)


-------------------------------------------------------------------------------

Some more macros:

	ALIGN (pos, size) ... make pos divisible by size. always returns an
						  address >= pos.
	                      
	
	OFFSET (s,el) ....... returns the offset of 'el' in structure 's' in bytes.
	                      
	MCOPY (dest,src,type,num) ... copy 'num' elements of type 'type'.
	

*/
