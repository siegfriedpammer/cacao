/* statistics.h - exports global varables for statistics

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   Institut f. Computersprachen, TU Wien
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser, M. Probst,
   S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich,
   J. Wenninger

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

   $Id: statistics.h 1438 2004-11-05 09:52:49Z twisti $

*/


#ifndef _STATISTICS_H
#define _STATISTICS_H


#include "global.h"


/* global variables */

extern s4 memoryusage;
extern s4 maxmemusage;
extern s4 maxdumpsize;

extern s4 globalallocateddumpsize;
extern s4 globaluseddumpsize;

extern int count_class_infos;           /* variables for measurements         */
extern int count_const_pool_len;
extern int count_vftbl_len;
extern int count_all_methods;
extern int count_methods_marked_used;  /*RTA*/
extern int count_vmcode_len;
extern int count_extable_len;
extern int count_class_loads;
extern int count_class_inits;

extern int count_utf_len;               /* size of utf hash                   */
extern int count_utf_new;
extern int count_utf_new_found;

extern int count_jit_calls;
extern int count_methods;
extern int count_spills;
extern int count_pcmd_activ;
extern int count_pcmd_drop;
extern int count_pcmd_zero;
extern int count_pcmd_const_store;
extern int count_pcmd_const_alu;
extern int count_pcmd_const_bra;
extern int count_pcmd_load;
extern int count_pcmd_move;
extern int count_load_instruction;
extern int count_pcmd_store;
extern int count_pcmd_store_comb;
extern int count_dup_instruction;
extern int count_pcmd_op;
extern int count_pcmd_mem;
extern int count_pcmd_met;
extern int count_pcmd_bra;
extern int count_pcmd_table;
extern int count_pcmd_return;
extern int count_pcmd_returnx;
extern int count_check_null;
extern int count_check_bound;
extern int count_max_basic_blocks;
extern int count_basic_blocks;
extern int count_max_javainstr;
extern int count_javainstr;
extern int count_javacodesize;
extern int count_javaexcsize;
extern int count_calls;
extern int count_tryblocks;
extern int count_code_len;
extern int count_data_len;
extern int count_cstub_len;
extern int count_nstub_len;
extern int count_max_new_stack;
extern int count_upper_bound_new_stack;
extern int *count_block_stack;
extern int *count_analyse_iterations;
extern int *count_method_bb_distribution;
extern int *count_block_size_distribution;
extern int *count_store_length;
extern int *count_store_depth;


/* function prototypes */

s8 getcputime();

void loadingtime_start();
void loadingtime_stop();
void compilingtime_start();
void compilingtime_stop();

void print_times();
void print_stats();

void mem_usagelog(bool givewarnings);
 
#endif /* _STATISTICS_H */


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
