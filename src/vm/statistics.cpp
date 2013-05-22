/* src/vm/statistics.c - global variables for statistics

   Copyright (C) 1996-2005, 2006, 2007, 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/


#include "config.h"

#include <stdint.h>
#include <string.h>

#if defined(HAVE_TIME_H)
# include <time.h>
#endif

#if defined(HAVE_SYS_TIME_H)
# include <sys/time.h>
#endif

#if defined(HAVE_SYS_RESOURCE_H)
# include <sys/resource.h>
#endif

#include "vm/types.hpp"

#include "mm/gc.hpp"

#include "toolbox/logging.hpp"

#include "vm/class.hpp"
#include "vm/field.hpp"
#include "vm/global.hpp"
#include "vm/method.hpp"
#include "vm/options.hpp"
#include "vm/statistics.hpp"

#include "vm/jit/code.hpp"


/* global variables ***********************************************************/

static s8 loadingtime = 0;              /* accumulated loading time           */
static s8 loadingstarttime = 0;
static s8 loadingstoptime = 0;
static s4 loadingtime_recursion = 0;

static s8 compilingtime = 0;            /* accumulated compile time           */
static s8 compilingstarttime = 0;
static s8 compilingstoptime = 0;
static s4 compilingtime_recursion = 0;

s4 codememusage = 0;
s4 maxcodememusage = 0;

s4 memoryusage = 0;
s4 maxmemusage = 0;

s4 maxdumpsize = 0;

s4 globalallocateddumpsize = 0;
s4 globaluseddumpsize = 0;


/* variables for measurements *************************************************/

s4 size_classinfo        = 0;
s4 size_fieldinfo        = 0;
s4 size_methodinfo       = 0;
s4 size_lineinfo         = 0;
s4 size_codeinfo         = 0;

s4 size_stub_native      = 0;

s4 size_stack_map        = 0;
s4 size_string           = 0;

s4 size_threadobject     = 0;
int32_t size_thread_index_t = 0;
int32_t size_stacksize      = 0;

s4 size_lock_record      = 0;
s4 size_lock_hashtable   = 0;
s4 size_lock_waiter      = 0;

int32_t count_linenumbertable = 0;
int32_t size_linenumbertable  = 0;

s4 size_patchref         = 0;

u8 count_calls_java_to_native = 0;
u8 count_calls_native_to_java = 0;

int count_const_pool_len = 0;
int count_classref_len = 0;
int count_parsed_desc_len = 0;
int count_vftbl_len = 0;
int count_all_methods = 0;
int count_methods_marked_used = 0;  /* RTA */

int count_vmcode_len = 0;
int count_extable_len = 0;
int count_class_loads = 0;
int count_class_inits = 0;

int count_utf_len = 0;                  /* size of utf hash                   */
int count_utf_new = 0;                  /* calls of utf_new                   */
int count_utf_new_found  = 0;           /* calls of utf_new with fast return  */

int count_locals_conflicts = 0;         /* register allocator statistics */
int count_locals_spilled = 0;
int count_locals_register = 0;
int count_ss_spilled = 0;
int count_ss_register = 0;
int count_methods_allocated_by_lsra = 0;
int count_mem_move_bb = 0;
int count_interface_size = 0;
int count_argument_mem_ss = 0;
int count_argument_reg_ss = 0;
int count_method_in_register = 0;
int count_mov_reg_reg = 0;
int count_mov_mem_reg = 0;
int count_mov_reg_mem = 0;
int count_mov_mem_mem = 0;

int count_jit_calls = 0;
int count_methods = 0;
int count_spills_read_ila = 0;
int count_spills_read_flt = 0;
int count_spills_read_dbl = 0;
int count_spills_write_ila = 0;
int count_spills_write_flt = 0;
int count_spills_write_dbl = 0;
int count_pcmd_activ = 0;
int count_pcmd_drop = 0;
int count_pcmd_zero = 0;
int count_pcmd_const_store = 0;
int count_pcmd_const_alu = 0;
int count_pcmd_const_bra = 0;
int count_pcmd_load = 0;
int count_pcmd_move = 0;
int count_load_instruction = 0;
int count_pcmd_store = 0;
int count_pcmd_store_comb = 0;
int count_dup_instruction = 0;
int count_pcmd_op = 0;
int count_pcmd_mem = 0;
int count_pcmd_met = 0;
int count_pcmd_bra = 0;
int count_pcmd_table = 0;
int count_pcmd_return = 0;
int count_pcmd_returnx = 0;
int count_check_null = 0;
int count_check_bound = 0;
int count_max_basic_blocks = 0;
int count_basic_blocks = 0;
int count_javainstr = 0;
int count_max_javainstr = 0;
int count_javacodesize = 0;
int count_javaexcsize = 0;
int count_calls = 0;
int count_tryblocks = 0;
int count_code_len = 0;
int count_data_len = 0;
int count_cstub_len = 0;
int count_max_new_stack = 0;
int count_upper_bound_new_stack = 0;

int count_emit_branch = 0;
int count_emit_branch_8bit = 0;
int count_emit_branch_16bit = 0;
int count_emit_branch_32bit = 0;
int count_emit_branch_64bit = 0;

s4 count_branches_resolved   = 0;
s4 count_branches_unresolved = 0;

u8 count_jni_callXmethod_calls=0;
u8 count_jni_calls=0;


static int count_block_stack_init[11] = {
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0
};
int *count_block_stack = count_block_stack_init;
static int count_analyse_iterations_init[5] = {
	0, 0, 0, 0, 0
};
int *count_analyse_iterations = count_analyse_iterations_init;
static int count_method_bb_distribution_init[9] = {
	0, 0, 0, 0, 0,
	0, 0, 0, 0
};
int *count_method_bb_distribution = count_method_bb_distribution_init;
static int count_block_size_distribution_init[18] = {
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0
};
int *count_block_size_distribution = count_block_size_distribution_init;
static int count_store_length_init[21] = {
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0
};
int *count_store_length = count_store_length_init;
static int count_store_depth_init[11] = {
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0
};
int *count_store_depth = count_store_depth_init;


/* instruction scheduler statistics *******************************************/

s4 count_schedule_basic_blocks = 0;
s4 count_schedule_nodes = 0;
s4 count_schedule_leaders = 0;
s4 count_schedule_max_leaders = 0;
s4 count_schedule_critical_path = 0;



/* getcputime *********************************** ******************************

   Returns the used CPU time in microseconds

*******************************************************************************/

s8 getcputime(void)
{
#if defined(HAVE_GETRUSAGE)
	struct rusage ru;
	int sec, usec;

	getrusage(RUSAGE_SELF, &ru);

	sec  = ru.ru_utime.tv_sec + ru.ru_stime.tv_sec;
	usec = ru.ru_utime.tv_usec + ru.ru_stime.tv_usec;

	return sec * 1000000 + usec;
#else
	/* If we don't have getrusage, simply return 0. */

	return 0;
#endif
}


/* loadingtime_stop ************************************************************

   XXX

*******************************************************************************/

void loadingtime_start(void)
{
	loadingtime_recursion++;

	if (loadingtime_recursion == 1)
		loadingstarttime = getcputime();
}


/* loadingtime_stop ************************************************************

   XXX

*******************************************************************************/

void loadingtime_stop(void)
{
	if (loadingtime_recursion == 1) {
		loadingstoptime = getcputime();
		loadingtime += (loadingstoptime - loadingstarttime);
	}

	loadingtime_recursion--;
}


/* compilingtime_stop **********************************************************

   XXX

*******************************************************************************/

void compilingtime_start(void)
{
	compilingtime_recursion++;

	if (compilingtime_recursion == 1)
		compilingstarttime = getcputime();
}


/* compilingtime_stop **********************************************************

   XXX

*******************************************************************************/

void compilingtime_stop(void)
{
	if (compilingtime_recursion == 1) {
		compilingstoptime = getcputime();
		compilingtime += (compilingstoptime - compilingstarttime);
	}

	compilingtime_recursion--;
}


/* print_times *****************************************************************

   Prints a summary of CPU time usage.

*******************************************************************************/

void print_times(void)
{
	s8 totaltime;
	s8 runtime;

	totaltime = getcputime();
	runtime = totaltime - loadingtime - compilingtime;

#if SIZEOF_VOID_P == 8
	dolog("Time for loading classes: %6ld ms", loadingtime / 1000);
	dolog("Time for compiling code:  %6ld ms", compilingtime / 1000);
	dolog("Time for running program: %6ld ms", runtime / 1000);
	dolog("Total time:               %6ld ms", totaltime / 1000);
#else
	dolog("Time for loading classes: %6lld ms", loadingtime / 1000);
	dolog("Time for compiling code:  %6lld ms", compilingtime / 1000);
	dolog("Time for running program: %6lld ms", runtime / 1000);
	dolog("Total time:               %6lld ms", totaltime / 1000);
#endif
}


/* print_stats *****************************************************************

   outputs detailed compiler statistics

*******************************************************************************/

void print_stats(void)
{
	s4    i;
	float f;
	s4    sum;


	dolog("Number of JIT compiler calls: %6d", count_jit_calls);
	dolog("Number of compiled methods:   %6d", count_methods);

	dolog("Number of compiled basic blocks:               %6d",
		  count_basic_blocks);
	dolog("Number of max. basic blocks per method:        %6d",
		  count_max_basic_blocks);

	dolog("Number of compiled JavaVM instructions:        %6d",
		  count_javainstr);
	dolog("Number of max. JavaVM instructions per method: %6d",
		  count_max_javainstr);
	dolog("Size of compiled JavaVM instructions:          %6d(%d)",
		  count_javacodesize, count_javacodesize - count_methods * 18);

	dolog("Size of compiled Exception Tables:      %d", count_javaexcsize);
	dolog("Number of Machine-Instructions: %d", count_code_len >> 2);
	dolog("Number of Spills (write to memory) <all [i/l/a|flt|dbl]>: %d [%d|%d|%d]",
		count_spills_write_ila + count_spills_write_flt + count_spills_write_dbl,
		count_spills_write_ila, count_spills_write_flt, count_spills_write_dbl);
	dolog("Number of Spills (read from memory) <all [i/l/a|flt|dbl]>: %d [%d|%d|%d]",
		count_spills_read_ila + count_spills_read_flt + count_spills_read_dbl,
		count_spills_read_ila, count_spills_read_flt, count_spills_read_dbl);
	dolog("Number of Activ    Pseudocommands: %6d", count_pcmd_activ);
	dolog("Number of Drop     Pseudocommands: %6d", count_pcmd_drop);
	dolog("Number of Const    Pseudocommands: %6d (zero:%5d)",
		  count_pcmd_load, count_pcmd_zero);
	dolog("Number of ConstAlu Pseudocommands: %6d (cmp: %5d, store:%5d)",
		  count_pcmd_const_alu, count_pcmd_const_bra, count_pcmd_const_store);
	dolog("Number of Move     Pseudocommands: %6d", count_pcmd_move);
	dolog("Number of Load     Pseudocommands: %6d", count_load_instruction);
	dolog("Number of Store    Pseudocommands: %6d (combined: %5d)",
		  count_pcmd_store, count_pcmd_store - count_pcmd_store_comb);
	dolog("Number of OP       Pseudocommands: %6d", count_pcmd_op);
	dolog("Number of DUP      Pseudocommands: %6d", count_dup_instruction);
	dolog("Number of Mem      Pseudocommands: %6d", count_pcmd_mem);
	dolog("Number of Method   Pseudocommands: %6d", count_pcmd_met);
	dolog("Number of Branch   Pseudocommands: %6d (rets:%5d, Xrets: %5d)",
		  count_pcmd_bra, count_pcmd_return, count_pcmd_returnx);
	log_println("                resolved branches: %6d", count_branches_resolved);
	log_println("              unresolved branches: %6d", count_branches_unresolved);
	dolog("Number of Table    Pseudocommands: %6d", count_pcmd_table);
	dolog("Number of Useful   Pseudocommands: %6d", count_pcmd_table +
		  count_pcmd_bra + count_pcmd_load + count_pcmd_mem + count_pcmd_op);
	dolog("Number of Null Pointer Checks:     %6d", count_check_null);
	dolog("Number of Array Bound Checks:      %6d", count_check_bound);
	dolog("Number of Try-Blocks: %d", count_tryblocks);

	dolog("Number of branch_emit (total, 8bit/16bit/32bit/64bit offset): %d, %d/%d/%d/%d",
		count_emit_branch,  count_emit_branch_8bit,  count_emit_branch_16bit,
							count_emit_branch_32bit, count_emit_branch_64bit);

	dolog("Maximal count of stack elements:   %d", count_max_new_stack);
	dolog("Upper bound of max stack elements: %d", count_upper_bound_new_stack);
	dolog("Distribution of stack sizes at block boundary");
	dolog("     0     1     2     3     4     5     6     7     8     9  >=10");
	dolog("%6d%6d%6d%6d%6d%6d%6d%6d%6d%6d%6d",
		  count_block_stack[0], count_block_stack[1], count_block_stack[2],
		  count_block_stack[3], count_block_stack[4], count_block_stack[5],
		  count_block_stack[6], count_block_stack[7], count_block_stack[8],
		  count_block_stack[9], count_block_stack[10]);
	dolog("Distribution of store stack depth");
	dolog("     0     1     2     3     4     5     6     7     8     9  >=10");
	dolog("%6d%6d%6d%6d%6d%6d%6d%6d%6d%6d%6d",
		  count_store_depth[0], count_store_depth[1], count_store_depth[2],
		  count_store_depth[3], count_store_depth[4], count_store_depth[5],
		  count_store_depth[6], count_store_depth[7], count_store_depth[8],
		  count_store_depth[9], count_store_depth[10]);
	dolog("Distribution of store creator chains first part");
	dolog("     0     1     2     3     4     5     6     7     8     9");
	dolog("%6d%6d%6d%6d%6d%6d%6d%6d%6d%6d",
		  count_store_length[0], count_store_length[1], count_store_length[2],
		  count_store_length[3], count_store_length[4], count_store_length[5],
		  count_store_length[6], count_store_length[7], count_store_length[8],
		  count_store_length[9]);
	dolog("Distribution of store creator chains second part");
	dolog("    10    11    12    13    14    15    16    17    18    19  >=20");
	dolog("%6d%6d%6d%6d%6d%6d%6d%6d%6d%6d%6d",
		  count_store_length[10], count_store_length[11],
		  count_store_length[12], count_store_length[13],
		  count_store_length[14], count_store_length[15],
		  count_store_length[16], count_store_length[17],
		  count_store_length[18], count_store_length[19],
		  count_store_length[20]);
	dolog("Distribution of analysis iterations");
	dolog("     1     2     3     4   >=5");
	dolog("%6d%6d%6d%6d%6d",
		  count_analyse_iterations[0], count_analyse_iterations[1],
		  count_analyse_iterations[2], count_analyse_iterations[3],
		  count_analyse_iterations[4]);


	/* Distribution of basic blocks per method ********************************/

	log_println("Distribution of basic blocks per method:");
	log_println("   <=5  <=10  <=15  <=20  <=30  <=40  <=50  <=75   >75");

	log_start();
	for (i = 0; i <= 8; i++)
		log_print("%6d", count_method_bb_distribution[i]);
	log_finish();

	/* print ratio */

	f = (float) count_methods;

	log_start();
	for (i = 0; i <= 8; i++)
		log_print("%6.2f", (float) count_method_bb_distribution[i] / f);
	log_finish();

	/* print cumulated ratio */

	log_start();
	for (i = 0, sum = 0; i <= 8; i++) {
		sum += count_method_bb_distribution[i];
		log_print("%6.2f", (float) sum / f);
	}
	log_finish();


	/* Distribution of basic block sizes **************************************/

	log_println("Distribution of basic block sizes:");
	log_println("     0     1     2     3     4     5     6     7     8     9   <13   <15   <17   <19   <21   <26   <31   >30");

	/* print block sizes */

	log_start();
	for (i = 0; i <= 17; i++)
		log_print("%6d", count_block_size_distribution[i]);
	log_finish();

	/* print ratio */

	f = (float) count_basic_blocks;

	log_start();
	for (i = 0; i <= 17; i++)
		log_print("%6.2f", (float) count_block_size_distribution[i] / f);
	log_finish();

	/* print cumulated ratio */

	log_start();
	for (i = 0, sum = 0; i <= 17; i++) {
		sum += count_block_size_distribution[i];
		log_print("%6.2f", (float) sum / f);
	}
	log_finish();

	statistics_print_memory_usage();

	dolog("Number of class loads:    %6d", count_class_loads);
	dolog("Number of class inits:    %6d", count_class_inits);
	dolog("Number of loaded Methods: %6d\n", count_all_methods);

	dolog("Calls of utf_new:                 %6d", count_utf_new);
	dolog("Calls of utf_new (element found): %6d\n", count_utf_new_found);


	/* LSRA statistics ********************************************************/

	dolog("Moves reg -> reg:     %6d", count_mov_reg_reg);
	dolog("Moves mem -> reg:     %6d", count_mov_mem_reg);
	dolog("Moves reg -> mem:     %6d", count_mov_reg_mem);
	dolog("Moves mem -> mem:     %6d", count_mov_mem_mem);

	dolog("Methods allocated by LSRA:         %6d",
		  count_methods_allocated_by_lsra);
	dolog("Conflicts between local Variables: %6d", count_locals_conflicts);
	dolog("Local Variables held in Memory:    %6d", count_locals_spilled);
	dolog("Local Variables held in Registers: %6d", count_locals_register);
	dolog("Stackslots held in Memory:         %6d", count_ss_spilled);
	dolog("Stackslots held in Registers:      %6d", count_ss_register);
	dolog("Memory moves at BB Boundaries:     %6d", count_mem_move_bb);
	dolog("Number of interface slots:         %6d\n", count_interface_size);
	dolog("Number of Argument stack slots in register:  %6d",
		  count_argument_reg_ss);
	dolog("Number of Argument stack slots in memory:    %6d\n",
		  count_argument_mem_ss);
	dolog("Number of Methods kept in registers:         %6d\n",
		  count_method_in_register);


	/* instruction scheduler statistics ***************************************/

#if defined(USE_SCHEDULER)
	dolog("Instruction scheduler statistics:");
	dolog("Number of basic blocks:       %7d", count_schedule_basic_blocks);
	dolog("Number of nodes:              %7d", count_schedule_nodes);
	dolog("Number of leaders nodes:      %7d", count_schedule_leaders);
	dolog("Number of max. leaders nodes: %7d", count_schedule_max_leaders);
	dolog("Length of critical path:      %7d\n", count_schedule_critical_path);
#endif


	/* call statistics ********************************************************/

	dolog("Function call statistics:");
	dolog("Number of jni->CallXMethod function invokations: %ld",
		  count_jni_callXmethod_calls);
	dolog("Overall number of jni invokations:               %ld",
		  count_jni_calls);

	log_println("java-to-native calls:   %10ld", count_calls_java_to_native);
	log_println("native-to-java calls:   %10ld", count_calls_native_to_java);


	/* now print other statistics ********************************************/

#if defined(ENABLE_INTRP)
	print_dynamic_super_statistics();
#endif
}


/* statistics_print_date *******************************************************

   Print current date and time.

*******************************************************************************/

void statistics_print_date(void)
{
  time_t t;
  struct tm tm;

#if defined(HAVE_TIME)
  time(&t);
#else
# error !HAVE_TIME
#endif

#if defined(HAVE_LOCALTIME_R)
  localtime_r(&t, &tm);
#else
# error !HAVE_LOCALTIME_R
#endif

  log_println("%d-%02d-%02d %02d:%02d:%02d",
			  1900 + tm.tm_year, tm.tm_mon + 1, tm.tm_mday,
			  tm.tm_hour, tm.tm_min, tm.tm_sec);
}


/* statistics_print_memory_usage ***********************************************

   Print current memory usage.

*******************************************************************************/

void statistics_print_memory_usage(void)
{
	s4 sum;

	log_println("memory usage ----------------------");
	log_println("");
	log_println("code:                      %10d", count_code_len);
	log_println("data:                      %10d", count_data_len);
	log_println("                            ----------");

	sum =
		count_code_len +
		count_data_len;

	log_println("                           %10d", sum);

	log_println("");

	log_println("classinfo  (%3d B):        %10d", (int) sizeof(classinfo), size_classinfo);
	log_println("fieldinfo  (%3d B):        %10d", (int) sizeof(fieldinfo), size_fieldinfo);
	log_println("methodinfo (%3d B):        %10d", (int) sizeof(methodinfo), size_methodinfo);
	log_println("lineinfo   (%3d B):        %10d", (int) sizeof(lineinfo), size_lineinfo);
	log_println("codeinfo   (%3d B):        %10d", (int) sizeof(codeinfo), size_codeinfo);
	log_println("                            ----------");

	sum =
		size_classinfo +
		size_fieldinfo +
		size_methodinfo +
		size_lineinfo +
		size_codeinfo;

	log_println("                           %10d", sum);

	log_println("");

	log_println("linenumber tables (%5d): %10d", count_linenumbertable, size_linenumbertable);
	log_println("exception tables:          %10d", count_extable_len);
	log_println("patcher references:        %10d", size_patchref);
	log_println("                            ----------");

	sum =
		size_linenumbertable +
		count_extable_len +
		size_patchref;

	log_println("                           %10d", sum);

	log_println("");

	log_println("constant pool:             %10d", count_const_pool_len);
	log_println("classref:                  %10d", count_classref_len);
	log_println("parsed descriptors:        %10d", count_parsed_desc_len);
	log_println("vftbl:                     %10d", count_vftbl_len);
	log_println("compiler stubs:            %10d", count_cstub_len);
	log_println("native stubs:              %10d", size_stub_native);
	log_println("utf:                       %10d", count_utf_len);
	log_println("vmcode:                    %10d", count_vmcode_len);
	log_println("stack map:                 %10d", size_stack_map);
	log_println("string:                    %10d", size_string);
	log_println("threadobject:              %10d", size_threadobject);
	log_println("thread index:              %10d", size_thread_index_t);
	log_println("stack size:                %10d", size_stacksize);
	log_println("lock record:               %10d", size_lock_record);
	log_println("lock hashtable:            %10d", size_lock_hashtable);
	log_println("lock waiter:               %10d", size_lock_waiter);
	log_println("                            ----------");

	sum =
		count_const_pool_len +
		count_classref_len +
		count_parsed_desc_len +
		count_vftbl_len +
		count_cstub_len +
		size_stub_native +
		count_utf_len +
		count_vmcode_len +
		size_stack_map +
		size_string +
		size_threadobject +
		size_thread_index_t +
		size_stacksize +
		size_lock_record +
		size_lock_hashtable +
		size_lock_waiter;

	log_println("                           %10d", sum);

	log_println("");

	log_println("max. code memory:          %10d", maxcodememusage);
	log_println("max. heap memory:          %10d", maxmemusage);
	log_println("max. dump memory:          %10d", maxdumpsize);
	log_println("");
	log_println("heap memory not freed:     %10d", (int32_t) memoryusage);
	log_println("dump memory not freed:     %10d", (int32_t) globalallocateddumpsize);

	log_println("");
}


/* statistics_print_gc_memory_usage ********************************************

   Print current GC memory usage.

*******************************************************************************/

void statistics_print_gc_memory_usage(void)
{
	static int64_t count = 0;
	int64_t max;
	int64_t size;
	int64_t free;
	int64_t used;
	int64_t total;

	count++;

	max   = gc_get_max_heap_size();
	size  = gc_get_heap_size();
	free  = gc_get_free_bytes();
	used  = size - free;
	total = gc_get_total_bytes();

	if (opt_ProfileMemoryUsageGNUPlot) {
		if (count == 1)
			fprintf(opt_ProfileMemoryUsageGNUPlot, "plot \"profile.dat\" using 1:2 with lines title \"max. Java heap size\", \"profile.dat\" using 1:3 with lines title \"Java heap size\", \"profile.dat\" using 1:4 with lines title \"used\", \"profile.dat\" using 1:5 with lines title \"free\"\n");

#if SIZEOF_VOID_P == 8
		fprintf(opt_ProfileMemoryUsageGNUPlot, "%ld %ld %ld %ld %ld\n", count, max, size, used, free);
#else
		fprintf(opt_ProfileMemoryUsageGNUPlot, "%lld %lld %lld %lld %lld\n", count, max, size, used, free);
#endif

		fflush(opt_ProfileMemoryUsageGNUPlot);
	}
	else {
		log_println("GC memory usage -------------------");
		log_println("");
		log_println("max. Java heap size: %10lld", max);
		log_println("");
		log_println("Java heap size:      %10lld", size);
		log_println("used:                %10lld", used);
		log_println("free:                %10lld", free);
		log_println("totally used:        %10lld", total);
		log_println("");
	}
}


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
