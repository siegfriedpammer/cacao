/* jit.h ***********************************************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	new compiler header file for inclusion in other moduls.

	Authors: Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at
	         Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/11/05

*******************************************************************************/

#ifndef __jit__
#define __jit__

/* compiler switches (set by main function) ***********************************/

extern bool runverbose;         /* trace all method invocation                */
extern bool compileverbose;     /* trace compiler actions                     */
extern bool showdisassemble;    /* generate disassembler listing              */
extern bool showddatasegment;   /* generate data segment listing              */
extern bool showintermediate;   /* generate intermediate code listing         */
extern int  optimizelevel;      /* optimzation level  (0 = no optimization)   */

extern bool useinlining;	/* use method inlining			      */
extern bool inlinevirtuals; /* inline unique virtual methods */
extern bool inlineexceptions; /* inline methods, that contain excptions */
extern bool inlineparamopt; /* optimize parameter passing to inlined methods */
extern bool inlineoutsiders; /* inline methods, that are not member of the invoker's class */


extern bool checkbounds;        /* check array bounds                         */
extern bool opt_loops;          /* optimize array accesses in loops           */
extern bool checknull;          /* check null pointers                        */
extern bool checkfloats;        /* implement ieee compliant floats            */
extern bool checksync;          /* do synchronization                         */

extern bool getcompilingtime;   /* compute compile time                       */
extern long compilingtime;      /* accumulated compile time                   */

extern int  has_ext_instr_set;  /* has instruction set extensions */

extern bool statistics;         

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

/* prototypes *****************************************************************/

methodptr jit_compile (methodinfo *m);  /* compile a method with jit compiler */

void jit_init();                        /* compiler initialisation            */
void jit_close();                       /* compiler finalisation              */

u1 *createcompilerstub (methodinfo *m);
u1 *createnativestub (functionptr f, methodinfo *m);

void removecompilerstub (u1 *stub);
void removenativestub (u1 *stub);

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
