#define ALIGNMENT  3   /* 64 bit */

/* Sorry, I couldn't use a while loop, because my compiler (gcc) on an
   Alpha inserts useless "nop" and "unop" instructions into the inner 
   loop. Additionally it reorders basic blocks such that a block from
   within the loop is branched to and branched back from (adding to
   unnecessary branches.

   Could anyone explain me why? 
   
   Apparently the later problem is related to using "continue"; if I use
   explicit "goto"s, I'm left with _only_ the unnecessary "nop" and 
   "unop"; that isn't acceptable either... */

/* So why did I do this? Well, this can be easily explained... using the 
   data from one of our test runs we found:

	 >> LOG: 9301464 bytes for 196724 objects allocated.
	 >> LOG: 15 garbage collections performed.
	 >> LOG: 6568440 heapblocks visited, 469249 objects marked
	 >> LOG:     1064447 visits to objects already marked.
	 >> LOG:     988270 potential references not aligned.
	 >> LOG:     4049446 out of heap.
	 >> LOG:     5236 not an object.

   So how much did we save? Eliminating the inner loop inefficiencies saves
   at least 6568440 * 2 instruction (1 nop, 1 unop), folding the test for
   non-objects and marked objects into one saves about 1000000 array accesses,
   shifts, compares and untaken branches. The pseudo-recursion saves around
   15 instructions (mostly load/stores for saved registers) for every one of 
   the 469249 marked objects. Adding this up we would save about 25000000
   instructions (and that's a careful estimate), most of them loads or stores.
*/

/* gc_mark_object_at now contains assembler code: This became necessary
   to get rid of the register saving overhead (callee-saved registers)
   involved in calling the recursive functions. -- phil. */

/* Important note (1): 
   I use the low bit of the "addr" stored on the stack to indicate whether 
   this is the outermost loop (the actual function and not a pseudo-recursion): 
   this bit is clear, if this is a recursion, but set when it is the outermost 
   function. In the latter case the value retrieved is not used, so I don't 
   need to align it. -- phil. */

/* Rambling (1):
   Weird... apparently the alpha compiler backend scatters rather useless 
   instructions throughout the code...
   
   "pattern |= (0x3 << offset);" yields:

     54:	01 74 e0 47 	mov  0x3,t0
     58:	21 07 22 48 	sll	 t0,t1,t0
     5c:	01 00 3f 40 	addl t0,zero,t0
     60:	04 04 81 44 	or   t3,t0,t3

   That's rather depressing... either I am simply dumb, or the "addl" 
   instruction shouldn't be there. Sigh. -- phil. */

void gc_mark_object_at(void** addr, unsigned long*  bitmap, void* heap_base_reg, void* heap_size_reg)
{
	register long   pattern;
	register long   offset;
	register long*  pattern_addr;
	register void*  deref_addr;
	register void** end;
	register void*  stack;
	register long   constant_one;

	if ((unsigned long)addr & ((1 << ALIGNMENT) - 1))
		return;

	if (((unsigned long)addr - (unsigned long)heap_base_reg) >= (unsigned long)heap_size_reg)
		return;

	pattern_addr = &bitmap[(unsigned long)addr >> 8];
	pattern = *pattern_addr;
	offset = ((unsigned long)addr >> 2) & ((1 << 5) - 2);
	pattern >> offset;

	if (pattern != 0x2)
		return;

	constant_one = 1;

	/* grow stack & mark as outermost loop */
	asm("subq $30, %0, $30" : : "i"(sizeof(void*)) : "$30");
	asm("stq %0, %1 ($30)" : : "r"(constant_one), "i"(0) : "memory");

 recurse:
	/* mark */
	*pattern_addr = pattern | (constant_one << offset);
	
	/* detect length */
	end = addr + *(long*)addr; /* FIXME: insert the real code necessary */

 loop:	
	if (addr >= end)
		goto unwind;

	deref_addr = *addr;
	++addr;
		
	if ((unsigned long)(deref_addr) & ((1 << ALIGNMENT) - 1))
		goto loop;
	
	if (((unsigned long)addr - (unsigned long)heap_base_reg) >= (unsigned long)heap_size_reg)
		goto loop;

	pattern_addr = &bitmap[(unsigned long)(deref_addr) >> 8];
	pattern = *pattern_addr;
	offset = ((unsigned long)addr >> 2) & ((1 << 5) - 2);
	pattern >> offset;
		
	if (pattern != 0x2)
	    goto loop;

	/* grow the stack */
	asm("subq $30, %0, $30" : : "i"(2 * sizeof(void*)) : "$30");

	/* store the current context on the stack: addr, end */
	asm("stq %0, %1 ($30)" : : "r"(addr), "i"(0) : "memory" );
	asm("stq %0, %1 ($30)" : : "r"(end), "i"(sizeof(void*)) : "memory" );
		
	addr = deref_addr;

	goto recurse;

 unwind:
	/* load the addr field of the previous context */
	asm("ldq %0, %1 ($30)" : "=r"(addr) : "i"(0) : "memory");	

	/* check whether we are returning from the outermost pseudo-recursion */
	if ((long)addr & 1)
		goto done;

	/* load the end field of the previous context */
	asm("ldq %0, %1 ($30)" : "=r"(end) : "i"(sizeof(void*)) : "memory" );

	/* shrink the stack */
	asm("addq $30, %0, $30" : : "i"(2 * sizeof(void*)) : "$30");

	/* continue the loop in this context */
	goto loop;

 done:
	/* pop the no_recursion flag */
	asm("addq $30, %0, $30" : : "i"(sizeof(void*)) : "$30");
	return;
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
