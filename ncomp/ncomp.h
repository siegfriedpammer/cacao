/****************************** ncomp.h ****************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	new compiler header file for inclusion in other moduls.

	Authors: Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at
	         Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/11/05

*******************************************************************************/

/************** compiler switches (are set by main function) ******************/

extern bool runverbose;         /* Das Programm soll w"arend des Laufs alle
                                   Methodenaufrufe mitprotokollieren */
extern bool compileverbose;     /* Der Compiler soll sagen, was er macht */
extern bool showstack;          /* Alle Stackzust"ande ausgeben */
extern bool showdisassemble;    /* Disassemblerlisting ausgeben */
extern bool showintermediate;   /* Zwischencode ausgeben */
extern int  optimizelevel;      /* Optimierungsstufe  (0=keine) */

extern bool checkbounds;        /* Arraygrenzen "uberpr"ufen */
extern bool checknull;          /* auf Null-Pointer "uberpr"ufen */
extern bool checkfloats;        /* Fehler bei Fliesskommas abfangen */
extern bool checksync;          /* Thread-Synchronisation wirklich machen */

extern bool getcompilingtime;   
extern long compilingtime;      /* CPU-Zeit f"urs "Ubersetzen */

extern int  has_ext_instr_set;  /* has instruction set extensions */


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

/******************************* prototypes ***********************************/

methodptr new_compile (methodinfo *m);  /* compile a method with new compiler */

void ncomp_init();                      /* compiler initialisation            */
void ncomp_close();                     /* compiler finalisation              */

/*
u1 *createcompilerstub (methodinfo *m);
u1 *createnativestub (functionptr f, methodinfo *m);

void removecompilerstub (u1 *stub);
void removenativestub (u1 *stub);
*/
