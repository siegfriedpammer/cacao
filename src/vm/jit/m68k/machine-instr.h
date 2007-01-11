#ifndef _MACHINE_INSTR_H
#define _MACHINE_INSTR_H

static inline long compare_and_swap(long *p, long oldval, long newval)
{
	assert(0);
	return 0;
}

#define STORE_ORDER_BARRIER() 				assert(0);
#define MEMORY_BARRIER_BEFORE_ATOMIC()			assert(0); 
#define MEMORY_BARRIER_AFTER_ATOMIC() 			assert(0);
#define MEMORY_BARRIER() 				assert(0);

#endif
