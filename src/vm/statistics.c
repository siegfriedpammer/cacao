/* vm/statistics.c - global varables for statistics

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Christian Thalinger

   $Id: statistics.c 1954 2005-02-17 19:47:23Z christian $

*/


#include <sys/time.h>
#include <sys/resource.h>

#include "toolbox/logging.h"
#include "vm/global.h"
#include "vm/options.h"
#include "vm/statistics.h"


/* global variables */

static s8 loadingtime = 0;              /* accumulated loading time           */
static s8 loadingstarttime = 0;
static s8 loadingstoptime = 0;
static s4 loadingtime_recursion = 0;

static s8 compilingtime = 0;            /* accumulated compile time           */
static s8 compilingstarttime = 0;
static s8 compilingstoptime = 0;
static s4 compilingtime_recursion = 0;

s4 memoryusage = 0;
s4 maxmemusage = 0;
s4 maxdumpsize = 0;

s4 globalallocateddumpsize = 0;
s4 globaluseddumpsize = 0;

int count_class_infos = 0;              /* variables for measurements         */
int count_const_pool_len = 0;
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

int count_jit_calls = 0;
int count_methods = 0;
int count_spills = 0;
int count_spills_read = 0;
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
int count_nstub_len = 0;
int count_max_new_stack = 0;
int count_upper_bound_new_stack = 0;
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


/* getcputime *********************************** ******************************

   Returns the used CPU time in microseconds
	
*******************************************************************************/

s8 getcputime()
{
	struct rusage ru;
	int sec, usec;

	getrusage(RUSAGE_SELF, &ru);
	sec = ru.ru_utime.tv_sec + ru.ru_stime.tv_sec;
	usec = ru.ru_utime.tv_usec + ru.ru_stime.tv_usec;

	return sec * 1000000 + usec;
}


/* loadingtime_stop ************************************************************

   XXX

*******************************************************************************/

void loadingtime_start()
{
	loadingtime_recursion++;

	if (loadingtime_recursion == 1)
		loadingstarttime = getcputime();
}


/* loadingtime_stop ************************************************************

   XXX

*******************************************************************************/

void loadingtime_stop()
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

void compilingtime_start()
{
	compilingtime_recursion++;

	if (compilingtime_recursion == 1)
		compilingstarttime = getcputime();
}


/* compilingtime_stop **********************************************************

   XXX

*******************************************************************************/

void compilingtime_stop()
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

void print_times()
{
	s8 totaltime = getcputime();
	s8 runtime = totaltime - loadingtime - compilingtime;
	char logtext[MAXLOGTEXT];

#if defined(__I386__) || defined(__POWERPC__)
	sprintf(logtext, "Time for loading classes: %lld secs, %lld millis",
#else
	sprintf(logtext, "Time for loading classes: %ld secs, %ld millis",
#endif
			loadingtime / 1000000, (loadingtime % 1000000) / 1000);
	log_text(logtext);

#if defined(__I386__) || defined(__POWERPC__) 
	sprintf(logtext, "Time for compiling code:  %lld secs, %lld millis",
#else
	sprintf(logtext, "Time for compiling code:  %ld secs, %ld millis",
#endif
			compilingtime / 1000000, (compilingtime % 1000000) / 1000);
	log_text(logtext);

#if defined(__I386__) || defined(__POWERPC__) 
	sprintf(logtext, "Time for running program: %lld secs, %lld millis",
#else
	sprintf(logtext, "Time for running program: %ld secs, %ld millis",
#endif
			runtime / 1000000, (runtime % 1000000) / 1000);
	log_text(logtext);

#if defined(__I386__) || defined(__POWERPC__) 
	sprintf(logtext, "Total time: %lld secs, %lld millis",
#else
	sprintf(logtext, "Total time: %ld secs, %ld millis",
#endif
			totaltime / 1000000, (totaltime % 1000000) / 1000);
	log_text(logtext);
}


/* print_stats *****************************************************************

   outputs detailed compiler statistics

*******************************************************************************/

void print_stats()
{
	char logtext[MAXLOGTEXT];

	sprintf(logtext, "Number of JitCompiler Calls: %d", count_jit_calls);
	log_text(logtext);
	sprintf(logtext, "Number of compiled Methods: %d", count_methods);
	log_text(logtext);
        if (opt_rt) {
	  sprintf(logtext, "Number of Methods marked Used: %d", count_methods_marked_used);
	  log_text(logtext);
	  }
	sprintf(logtext, "Number of max basic blocks per method: %d", count_max_basic_blocks);
	log_text(logtext);
	sprintf(logtext, "Number of compiled basic blocks: %d", count_basic_blocks);
	log_text(logtext);
	sprintf(logtext, "Number of max JavaVM-Instructions per method: %d", count_max_javainstr);
	log_text(logtext);
	sprintf(logtext, "Number of compiled JavaVM-Instructions: %d", count_javainstr);
	log_text(logtext);
	sprintf(logtext, "Size of compiled JavaVM-Instructions:   %d(%d)", count_javacodesize,
			count_javacodesize - count_methods * 18);
	log_text(logtext);
	sprintf(logtext, "Size of compiled Exception Tables:      %d", count_javaexcsize);
	log_text(logtext);
	sprintf(logtext, "Number of Machine-Instructions: %d", count_code_len >> 2);
	log_text(logtext);
	sprintf(logtext, "Number of Spills (write to memory): %d", count_spills);
	log_text(logtext);
	sprintf(logtext, "Number of Spills (read from memory): %d", count_spills_read);
	log_text(logtext);
	sprintf(logtext, "Number of Activ    Pseudocommands: %5d", count_pcmd_activ);
	log_text(logtext);
	sprintf(logtext, "Number of Drop     Pseudocommands: %5d", count_pcmd_drop);
	log_text(logtext);
	sprintf(logtext, "Number of Const    Pseudocommands: %5d (zero:%5d)", count_pcmd_load, count_pcmd_zero);
	log_text(logtext);
	sprintf(logtext, "Number of ConstAlu Pseudocommands: %5d (cmp: %5d, store:%5d)", count_pcmd_const_alu, count_pcmd_const_bra, count_pcmd_const_store);
	log_text(logtext);
	sprintf(logtext, "Number of Move     Pseudocommands: %5d", count_pcmd_move);
	log_text(logtext);
	sprintf(logtext, "Number of Load     Pseudocommands: %5d", count_load_instruction);
	log_text(logtext);
	sprintf(logtext, "Number of Store    Pseudocommands: %5d (combined: %5d)", count_pcmd_store, count_pcmd_store - count_pcmd_store_comb);
	log_text(logtext);
	sprintf(logtext, "Number of OP       Pseudocommands: %5d", count_pcmd_op);
	log_text(logtext);
	sprintf(logtext, "Number of DUP      Pseudocommands: %5d", count_dup_instruction);
	log_text(logtext);
	sprintf(logtext, "Number of Mem      Pseudocommands: %5d", count_pcmd_mem);
	log_text(logtext);
	sprintf(logtext, "Number of Method   Pseudocommands: %5d", count_pcmd_met);
	log_text(logtext);
	sprintf(logtext, "Number of Branch   Pseudocommands: %5d (rets:%5d, Xrets: %5d)",
			count_pcmd_bra, count_pcmd_return, count_pcmd_returnx);
	log_text(logtext);
	sprintf(logtext, "Number of Table    Pseudocommands: %5d", count_pcmd_table);
	log_text(logtext);
	sprintf(logtext, "Number of Useful   Pseudocommands: %5d", count_pcmd_table +
			count_pcmd_bra + count_pcmd_load + count_pcmd_mem + count_pcmd_op);
	log_text(logtext);
	sprintf(logtext, "Number of Null Pointer Checks:     %5d", count_check_null);
	log_text(logtext);
	sprintf(logtext, "Number of Array Bound Checks:      %5d", count_check_bound);
	log_text(logtext);
	sprintf(logtext, "Number of Try-Blocks: %d", count_tryblocks);
	log_text(logtext);
	sprintf(logtext, "Maximal count of stack elements:   %d", count_max_new_stack);
	log_text(logtext);
	sprintf(logtext, "Upper bound of max stack elements: %d", count_upper_bound_new_stack);
	log_text(logtext);
	sprintf(logtext, "Distribution of stack sizes at block boundary");
	log_text(logtext);
	sprintf(logtext, "    0    1    2    3    4    5    6    7    8    9    >=10");
	log_text(logtext);
	sprintf(logtext, "%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d", count_block_stack[0],
			count_block_stack[1], count_block_stack[2], count_block_stack[3], count_block_stack[4],
			count_block_stack[5], count_block_stack[6], count_block_stack[7], count_block_stack[8],
			count_block_stack[9], count_block_stack[10]);
	log_text(logtext);
	sprintf(logtext, "Distribution of store stack depth");
	log_text(logtext);
	sprintf(logtext, "    0    1    2    3    4    5    6    7    8    9    >=10");
	log_text(logtext);
	sprintf(logtext, "%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d", count_store_depth[0],
			count_store_depth[1], count_store_depth[2], count_store_depth[3], count_store_depth[4],
			count_store_depth[5], count_store_depth[6], count_store_depth[7], count_store_depth[8],
			count_store_depth[9], count_store_depth[10]);
	log_text(logtext);
	sprintf(logtext, "Distribution of store creator chains first part");
	log_text(logtext);
	sprintf(logtext, "    0    1    2    3    4    5    6    7    8    9  ");
	log_text(logtext);
	sprintf(logtext, "%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d", count_store_length[0],
			count_store_length[1], count_store_length[2], count_store_length[3], count_store_length[4],
			count_store_length[5], count_store_length[6], count_store_length[7], count_store_length[8],
			count_store_length[9]);
	log_text(logtext);
	sprintf(logtext, "Distribution of store creator chains second part");
	log_text(logtext);
	sprintf(logtext, "   10   11   12   13   14   15   16   17   18   19  >=20");
	log_text(logtext);
	sprintf(logtext, "%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d", count_store_length[10],
			count_store_length[11], count_store_length[12], count_store_length[13], count_store_length[14],
			count_store_length[15], count_store_length[16], count_store_length[17], count_store_length[18],
			count_store_length[19], count_store_length[20]);
	log_text(logtext);
	sprintf(logtext, "Distribution of analysis iterations");
	log_text(logtext);
	sprintf(logtext, "    1    2    3    4    >=5");
	log_text(logtext);
	sprintf(logtext, "%5d%5d%5d%5d%5d", count_analyse_iterations[0], count_analyse_iterations[1],
			count_analyse_iterations[2], count_analyse_iterations[3], count_analyse_iterations[4]);
	log_text(logtext);
	sprintf(logtext, "Distribution of basic blocks per method");
	log_text(logtext);
	sprintf(logtext, " <= 5 <=10 <=15 <=20 <=30 <=40 <=50 <=75  >75");
	log_text(logtext);
	sprintf(logtext, "%5d%5d%5d%5d%5d%5d%5d%5d%5d", count_method_bb_distribution[0],
			count_method_bb_distribution[1], count_method_bb_distribution[2], count_method_bb_distribution[3],
			count_method_bb_distribution[4], count_method_bb_distribution[5], count_method_bb_distribution[6],
			count_method_bb_distribution[7], count_method_bb_distribution[8]);
	log_text(logtext);
	sprintf(logtext, "Distribution of basic block sizes");
	log_text(logtext);
	sprintf(logtext,
			 "  0    1    2    3    4   5   6   7   8   9 <13 <15 <17 <19 <21 <26 <31 >30");
	log_text(logtext);
	sprintf(logtext, "%3d%5d%5d%5d%4d%4d%4d%4d%4d%4d%4d%4d%4d%4d%4d%4d%4d%4d",
			count_block_size_distribution[0], count_block_size_distribution[1], count_block_size_distribution[2],
			count_block_size_distribution[3], count_block_size_distribution[4], count_block_size_distribution[5],
			count_block_size_distribution[6], count_block_size_distribution[7], count_block_size_distribution[8],
			count_block_size_distribution[9], count_block_size_distribution[10], count_block_size_distribution[11],
			count_block_size_distribution[12], count_block_size_distribution[13], count_block_size_distribution[14],
			count_block_size_distribution[15], count_block_size_distribution[16], count_block_size_distribution[17]);
	log_text(logtext);
	sprintf(logtext, "Size of Code Area (Kb):  %10.3f", (float) count_code_len / 1024);
	log_text(logtext);
	sprintf(logtext, "Size of data Area (Kb):  %10.3f", (float) count_data_len / 1024);
	log_text(logtext);
	sprintf(logtext, "Size of Class Infos (Kb):%10.3f", (float) (count_class_infos) / 1024);
	log_text(logtext);
	sprintf(logtext, "Size of Const Pool (Kb): %10.3f", (float) (count_const_pool_len + count_utf_len) / 1024);
	log_text(logtext);
	sprintf(logtext, "Size of Vftbl (Kb):      %10.3f", (float) count_vftbl_len / 1024);
	log_text(logtext);
	sprintf(logtext, "Size of comp stub (Kb):  %10.3f", (float) count_cstub_len / 1024);
	log_text(logtext);
	sprintf(logtext, "Size of native stub (Kb):%10.3f", (float) count_nstub_len / 1024);
	log_text(logtext);
	sprintf(logtext, "Size of Utf (Kb):        %10.3f", (float) count_utf_len / 1024);
	log_text(logtext);
	sprintf(logtext, "Size of VMCode (Kb):     %10.3f(%d)", (float) count_vmcode_len / 1024,
			count_vmcode_len - 18 * count_all_methods);
	log_text(logtext);
	sprintf(logtext, "Size of ExTable (Kb):    %10.3f", (float) count_extable_len / 1024);
	log_text(logtext);
	sprintf(logtext, "Number of class loads:   %d", count_class_loads);
	log_text(logtext);
	sprintf(logtext, "Number of class inits:   %d", count_class_inits);
	log_text(logtext);
	sprintf(logtext, "Number of loaded Methods: %d\n\n", count_all_methods);
	log_text(logtext);

	sprintf(logtext, "Calls of utf_new: %22d", count_utf_new);
	log_text(logtext);
	sprintf(logtext, "Calls of utf_new (element found): %6d\n\n", count_utf_new_found);
	log_text(logtext);

	sprintf(logtext, "Methods allocated by LSRA:         %6d", count_methods_allocated_by_lsra);
	log_text(logtext);
	sprintf(logtext, "Conflicts between local Variables: %6d", count_locals_conflicts);
	log_text(logtext);
	sprintf(logtext, "Local Variables held in Memory:    %6d", count_locals_spilled);
	log_text(logtext);
	sprintf(logtext, "Local Variables held in Registers: %6d", count_locals_register);
	log_text(logtext);
	sprintf(logtext, "Stackslots held in Memory:         %6d", count_ss_spilled);
	log_text(logtext);
	sprintf(logtext, "Stackslots held in Registers:      %6d",count_ss_register );
	log_text(logtext);
	sprintf(logtext, "Memory moves at BB Boundaries:     %6d",count_mem_move_bb );
	log_text(logtext);
	sprintf(logtext, "Number of interface slots:         %6d\n\n",count_interface_size );
	log_text(logtext);
	sprintf(logtext, "Number of Argument stack slots in register:  %6d",count_argument_reg_ss );
	log_text(logtext);
	sprintf(logtext, "Number of Argument stack slots in memory:    %6d\n\n",count_argument_mem_ss );
	log_text(logtext);
	sprintf(logtext, "Number of Methods kept in registers:         %6d\n\n",count_method_in_register );
	log_text(logtext);
}


/* mem_usagelog ****************************************************************

   prints some memory related infos

*******************************************************************************/

void mem_usagelog(bool givewarnings)
{
	if ((memoryusage != 0) && givewarnings) {
		dolog("Allocated memory not returned: %d", (s4) memoryusage);
	}

	if ((globalallocateddumpsize != 0) && givewarnings) {
		dolog("Dump memory not returned: %d", (s4) globalallocateddumpsize);
	}

	dolog("Random/Dump - max. memory usage: %dkB/%dkB", 
		  (s4) ((maxmemusage + 1023) / 1024),
		  (s4) ((maxdumpsize + 1023) / 1024));
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
