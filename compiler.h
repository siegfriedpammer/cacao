/* -*- mode: c; tab-width: 4; c-basic-offset: 4 -*- */
/****************************** compiler.h *************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Contains the codegenerator for an Alpha processor.
	This module generates Alpha machine code for a sequence of
	pseudo commands (PCMDs).

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
	Changes: Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/10/22

*******************************************************************************/

/************** Compiler-switches (werden von main gesetzt) *****************/


extern bool compileverbose;     
extern bool showstack;          
extern bool showdisassemble;    
extern bool showddatasegment;    
extern bool showintermediate;   
extern int   optimizelevel;      

extern bool checkbounds;        
extern bool checknull;          
extern bool checkfloats;        
extern bool checksync;          

extern int  has_ext_instr_set;  

extern bool getcompilingtime;   
extern long int compilingtime;  

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
extern int count_pcmd_store;
extern int count_pcmd_store_comb;
extern int count_pcmd_op;
extern int count_pcmd_mem;
extern int count_pcmd_met;
extern int count_pcmd_bra;
extern int count_pcmd_table;
extern int count_pcmd_return;
extern int count_pcmd_returnx;
extern int count_check_null;
extern int count_check_bound;
extern int count_javainstr;
extern int count_javacodesize;
extern int count_javaexcsize;
extern int count_calls;
extern int count_tryblocks;
extern int count_code_len;
extern int count_data_len;
extern int count_cstub_len;
extern int count_nstub_len;


/******************************* Prototypes *********************************/

methodptr compiler_compile (methodinfo *m);

void compiler_init ();
void compiler_close ();

u1 *createcompilerstub (methodinfo *m);
u1 *createnativestub (functionptr f, methodinfo *m);
u1 *ncreatenativestub (functionptr f, methodinfo *m);

void removecompilerstub (u1 *stub);
void removenativestub (u1 *stub);

