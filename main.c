/* main.c - contains main() and variables for the global options

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

   Authors: Reinhard Grafl

   Changes: Andi Krall
            Mark Probst
            Philipp Tomsich

   This module does the following tasks:
     - Command line option handling
     - Calling initialization routines
     - Calling the class loader
     - Running the main method

   $Id: main.c 1188 2004-06-19 12:32:57Z twisti $

*/


#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "global.h"
#include "tables.h"
#include "loader.h"
#include "jit/jit.h"
#include "asmpart.h"
#include "builtin.h"
#include "native.h"
#include "mm/boehm.h"
#include "threads/thread.h"
#include "toolbox/logging.h"
#include "toolbox/memory.h"
#include "jit/parseRTstats.h"
#include "nat/java_io_File.h"            /* required by java_lang_Runtime.h   */
#include "nat/java_util_Properties.h"    /* required by java_lang_Runtime.h   */
#include "nat/java_lang_Runtime.h"
#include "nat/java_lang_Throwable.h"

#ifdef TYPEINFO_DEBUG_TEST
#include "typeinfo.h"
#endif


bool cacao_initializing;

/* command line option */
bool verbose = false;
bool compileall = false;
bool runverbose = false;       /* trace all method invocation                */
bool verboseexception = false;
bool collectverbose = false;

bool loadverbose = false;
bool linkverbose = false;
bool initverbose = false;

bool opt_rt = false;           /* true if RTA parse should be used     RT-CO */
bool opt_xta = false;          /* true if XTA parse should be used    XTA-CO */
bool opt_vta = false;          /* true if VTA parse should be used    VTA-CO */

bool opt_liberalutf = false;   /* Don't check overlong UTF-8 sequences       */

bool showmethods = false;
bool showconstantpool = false;
bool showutf = false;

bool compileverbose =  false;  /* trace compiler actions                     */
bool showstack = false;
bool showdisassemble = false;  /* generate disassembler listing              */
bool showddatasegment = false; /* generate data segment listing              */
bool showintermediate = false; /* generate intermediate code listing         */

bool useinlining = false;      /* use method inlining                        */
bool inlinevirtuals = false;   /* inline unique virtual methods              */
bool inlineexceptions = false; /* inline methods, that contain excptions     */
bool inlineparamopt = false;   /* optimize parameter passing to inlined methods */
bool inlineoutsiders = false;  /* inline methods, that are not member of the invoker's class */

bool checkbounds = true;       /* check array bounds                         */
bool checknull = true;         /* check null pointers                        */
bool opt_noieee = false;       /* don't implement ieee compliant floats      */
bool checksync = true;         /* do synchronization                         */
bool opt_loops = false;        /* optimize array accesses in loops           */

bool makeinitializations = true;

bool getloadingtime = false;   /* to measure the runtime                     */
s8 loadingtime = 0;

bool getcompilingtime = false; /* compute compile time                       */
s8 compilingtime = 0;          /* accumulated compile time                   */

int has_ext_instr_set = 0;     /* has instruction set extensions */

bool opt_stat = false;

bool opt_verify = true;        /* true if classfiles should be verified      */

bool opt_eager = false;

char *mainstring;
static classinfo *mainclass;

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
void **stackbottom = 0;
#endif


/* internal function: get_opt *************************************************
	
	decodes the next command line option
	
******************************************************************************/

#define OPT_DONE       -1
#define OPT_ERROR       0
#define OPT_IGNORE      1

#define OPT_CLASSPATH   2
#define OPT_D           3
#define OPT_MS          4
#define OPT_MX          5
#define OPT_VERBOSE1    6
#define OPT_VERBOSE     7
#define OPT_VERBOSEGC   8
#define OPT_VERBOSECALL 9
#define OPT_NOIEEE      10
#define OPT_SOFTNULL    11
#define OPT_TIME        12
#define OPT_STAT        13
#define OPT_LOG         14
#define OPT_CHECK       15
#define OPT_LOAD        16
#define OPT_METHOD      17
#define OPT_SIGNATURE   18
#define OPT_SHOW        19
#define OPT_ALL         20
#define OPT_OLOOP       24
#define OPT_INLINING	25
#define OPT_RT          26
#define OPT_XTA         27 
#define OPT_VTA         28
#define OPT_VERBOSETC   29
#define OPT_NOVERIFY    30
#define OPT_LIBERALUTF  31
#define OPT_VERBOSEEXCEPTION 32
#define OPT_EAGER            33


struct {char *name; bool arg; int value;} opts[] = {
	{"classpath",        true,   OPT_CLASSPATH},
	{"cp",               true,   OPT_CLASSPATH},
	{"D",                true,   OPT_D},
	{"Xms",              true,   OPT_MS},
	{"Xmx",              true,   OPT_MX},
	{"ms",               true,   OPT_MS},
	{"mx",               true,   OPT_MX},
	{"noasyncgc",        false,  OPT_IGNORE},
	{"noverify",         false,  OPT_NOVERIFY},
	{"liberalutf",       false,  OPT_LIBERALUTF},
	{"oss",              true,   OPT_IGNORE},
	{"ss",               true,   OPT_IGNORE},
	{"v",                false,  OPT_VERBOSE1},
	{"verbose",          false,  OPT_VERBOSE},
	{"verbosegc",        false,  OPT_VERBOSEGC},
	{"verbosecall",      false,  OPT_VERBOSECALL},
	{"verboseexception", false,  OPT_VERBOSEEXCEPTION},
#ifdef TYPECHECK_VERBOSE
	{"verbosetc",        false,  OPT_VERBOSETC},
#endif
#if defined(__ALPHA__)
	{"noieee",           false,  OPT_NOIEEE},
#endif
	{"softnull",         false,  OPT_SOFTNULL},
	{"time",             false,  OPT_TIME},
	{"stat",             false,  OPT_STAT},
	{"log",              true,   OPT_LOG},
	{"c",                true,   OPT_CHECK},
	{"l",                false,  OPT_LOAD},
    { "eager",            false,  OPT_EAGER },
	{"m",                true,   OPT_METHOD},
	{"sig",              true,   OPT_SIGNATURE},
	{"s",                true,   OPT_SHOW},
	{"all",              false,  OPT_ALL},
	{"oloop",            false,  OPT_OLOOP},
	{"i",		         true,   OPT_INLINING},
	{"rt",               false,  OPT_RT},
	{"xta",              false,  OPT_XTA},
	{"vta",              false,  OPT_VTA},
	{NULL,               false,  0}
};

static int opt_ind = 1;
static char *opt_arg;


static int get_opt(int argc, char **argv)
{
	char *a;
	int i;
	
	if (opt_ind >= argc) return OPT_DONE;
	
	a = argv[opt_ind];
	if (a[0] != '-') return OPT_DONE;

	for (i = 0; opts[i].name; i++) {
		if (!opts[i].arg) {
			if (strcmp(a + 1, opts[i].name) == 0) { /* boolean option found */
				opt_ind++;
				return opts[i].value;
			}

		} else {
			if (strcmp(a + 1, opts[i].name) == 0) { /* parameter option found */
				opt_ind++;
				if (opt_ind < argc) {
					opt_arg = argv[opt_ind];
					opt_ind++;
					return opts[i].value;
				}
				return OPT_ERROR;

			} else {
				size_t l = strlen(opts[i].name);
				if (strlen(a + 1) > l) {
					if (memcmp(a + 1, opts[i].name, l) == 0) {
						opt_ind++;
						opt_arg = a + 1 + l;
						return opts[i].value;
					}
				}
			}
		}
	} /* end for */	

	return OPT_ERROR;
}


/******************** interne Function: print_usage ************************

Prints the correct usage syntax to stdout.

***************************************************************************/

static void print_usage()
{
	printf("USAGE: cacao [options] classname [program arguments]\n");
	printf("Options:\n");
	printf("          -cp path ............. specify a path to look for classes\n");
	printf("          -classpath path ...... specify a path to look for classes\n");
	printf("          -Dpropertyname=value . add an entry to the property list\n");
	printf("          -Xmx maxmem[kK|mM] ... specify the size for the heap\n");
	printf("          -Xms initmem[kK|mM] .. specify the initial size for the heap\n");
	printf("          -mx maxmem[kK|mM] .... specify the size for the heap\n");
	printf("          -ms initmem[kK|mM] ... specify the initial size for the heap\n");
	printf("          -v ................... write state-information\n");
	printf("          -verbose ............. write more information\n");
	printf("          -verbosegc ........... write message for each GC\n");
	printf("          -verbosecall ......... write message for each call\n");
	printf("          -verboseexception .... write message for each step of stack unwinding\n");
#ifdef TYPECHECK_VERBOSE
	printf("          -verbosetc ........... write debug messages while typechecking\n");
#endif
#if defined(__ALPHA__)
	printf("          -noieee .............. don't use ieee compliant arithmetic\n");
#endif
	printf("          -noverify ............ don't verify classfiles\n");
	printf("          -liberalutf........... don't warn about overlong UTF-8 sequences\n");
	printf("          -softnull ............ use software nullpointer check\n");
	printf("          -time ................ measure the runtime\n");
	printf("          -stat ................ detailed compiler statistics\n");
	printf("          -log logfile ......... specify a name for the logfile\n");
	printf("          -c(heck)b(ounds) ..... don't check array bounds\n");
	printf("                  s(ync) ....... don't check for synchronization\n");
	printf("          -oloop ............... optimize array accesses in loops\n"); 
	printf("          -l ................... don't start the class after loading\n");
	printf("          -eager ............... perform eager class loading and linking\n");
	printf("          -all ................. compile all methods, no execution\n");
	printf("          -m ................... compile only a specific method\n");
	printf("          -sig ................. specify signature for a specific method\n");
	printf("          -s(how)a(ssembler) ... show disassembled listing\n");
	printf("                 c(onstants) ... show the constant pool\n");
	printf("                 d(atasegment).. show data segment listing\n");
	printf("                 i(ntermediate). show intermediate representation\n");
	printf("                 m(ethods)...... show class fields and methods\n");
	printf("                 u(tf) ......... show the utf - hash\n");
	printf("          -i     n ............. activate inlining\n");
	printf("                 v ............. inline virtual methods\n");
	printf("                 e ............. inline methods with exceptions\n");
	printf("                 p ............. optimize argument renaming\n");
	printf("                 o ............. inline methods of foreign classes\n");
	printf("          -rt .................. use rapid type analysis\n");
	printf("          -xta ................. use x type analysis\n");
	printf("          -vta ................. use variable type analysis\n");
}   


/***************************** Function: print_times *********************

	Prints a summary of CPU time usage.

**************************************************************************/

static void print_times()
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


/***************************** Function: print_stats *********************

	outputs detailed compiler statistics

**************************************************************************/

static void print_stats()
{
	char logtext[MAXLOGTEXT];

	sprintf(logtext, "Number of JitCompiler Calls: %d", count_jit_calls);
	log_text(logtext);
	sprintf(logtext, "Number of compiled Methods: %d", count_methods);
	log_text(logtext);
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
	sprintf(logtext, "Value of extended instruction set var:  %d", has_ext_instr_set);
	log_text(logtext);
	sprintf(logtext, "Number of Machine-Instructions: %d", count_code_len >> 2);
	log_text(logtext);
	sprintf(logtext, "Number of Spills: %d", count_spills);
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
}


#ifdef TYPECHECK_STATISTICS
void typecheck_print_statistics(FILE *file);
#endif


/*
 * void exit_handler(void)
 * -----------------------
 * The exit_handler function is called upon program termination to shutdown
 * the various subsystems and release the resources allocated to the VM.
 */
void exit_handler(void)
{
	/********************* Print debug tables ************************/
				
	if (showmethods) class_showmethods(mainclass);
	if (showconstantpool) class_showconstantpool(mainclass);
	if (showutf) utf_show();

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
	clear_thread_flags();		/* restores standard file descriptor
	                               flags */
#endif

	/************************ Free all resources *******************/

	/*heap_close();*/               /* must be called before compiler_close and
	                               loader_close because finalization occurs
	                               here */

	loader_close();
	tables_close();

	if (verbose || getcompilingtime || opt_stat) {
		log_text("CACAO terminated");
		if (opt_stat) {
			print_stats();
#ifdef TYPECHECK_STATISTICS
			typecheck_print_statistics(get_logfile());
#endif
		}
		if (getcompilingtime)
			print_times();
		mem_usagelog(1);
	}
}


/************************** Function: main *******************************

   The main program.
   
**************************************************************************/

int main(int argc, char **argv)
{
	s4 i, j;
	void *dummy;
	
	/********** interne (nur fuer main relevante Optionen) **************/
   
	char logfilename[200] = "";
	u4 heapmaxsize = 64 * 1024 * 1024;
	u4 heapstartsize = 200 * 1024;
	char *cp;
	char classpath[500] = ".";
	bool startit = true;
	char *specificmethodname = NULL;
	char *specificsignature = NULL;

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
	stackbottom = &dummy;
#endif
	
	if (0 != atexit(exit_handler)) 
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "unable to register exit_handler");


	/************ Collect info from the environment ************************/

	cp = getenv("CLASSPATH");
	if (cp) {
		strcpy(classpath, cp);
	}

	/***************** Interpret the command line *****************/
   
	checknull = false;
	opt_noieee = false;

	while ((i = get_opt(argc, argv)) != OPT_DONE) {
		switch (i) {
		case OPT_IGNORE: break;
			
		case OPT_CLASSPATH:    
			strcpy(classpath + strlen(classpath), ":");
			strcpy(classpath + strlen(classpath), opt_arg);
			break;
				
		case OPT_D:
			{
				int n;
				int l = strlen(opt_arg);
				for (n = 0; n < l; n++) {
					if (opt_arg[n] == '=') {
						opt_arg[n] = '\0';
						attach_property(opt_arg, opt_arg + n + 1);
						goto didit;
					}
				}
				print_usage();
				exit(10);
					
			didit: ;
			}	
			break;

		case OPT_MS:
		case OPT_MX:
			{
				char c;
				c = opt_arg[strlen(opt_arg) - 1];

				if (c == 'k' || c == 'K') {
					j = 1024 * atoi(opt_arg);

				} else if (c == 'm' || c == 'M') {
					j = 1024 * 1024 * atoi(opt_arg);

				} else j = atoi(opt_arg);

				if (i == OPT_MX) heapmaxsize = j;
				else heapstartsize = j;
			}
			break;

		case OPT_VERBOSE1:
			verbose = true;
			break;

		case OPT_VERBOSE:
			verbose = true;
			loadverbose = true;
			linkverbose = true;
			initverbose = true;
			compileverbose = true;
			break;

		case OPT_VERBOSEEXCEPTION:
			verboseexception = true;
			break;

		case OPT_VERBOSEGC:
			collectverbose = true;
			break;

#ifdef TYPECHECK_VERBOSE
		case OPT_VERBOSETC:
			typecheckverbose = true;
			break;
#endif
				
		case OPT_VERBOSECALL:
			runverbose = true;
			break;
				
		case OPT_NOIEEE:
			opt_noieee = true;
			break;

		case OPT_NOVERIFY:
			opt_verify = false;
			break;

		case OPT_LIBERALUTF:
			opt_liberalutf = true;
			break;

		case OPT_SOFTNULL:
			checknull = true;
			break;

		case OPT_TIME:
			getcompilingtime = true;
			getloadingtime = true;
			break;
					
		case OPT_STAT:
			opt_stat = true;
			break;
					
		case OPT_LOG:
			strcpy(logfilename, opt_arg);
			break;
			
		case OPT_CHECK:
			for (j = 0; j < strlen(opt_arg); j++) {
				switch (opt_arg[j]) {
				case 'b':
					checkbounds = false;
					break;
				case 's':
					checksync = false;
					break;
				default:
					print_usage();
					exit(10);
				}
			}
			break;
			
		case OPT_LOAD:
			startit = false;
			makeinitializations = false;
			break;

		case OPT_EAGER:
			opt_eager = true;
			break;

		case OPT_METHOD:
			startit = false;
			specificmethodname = opt_arg;     		
			makeinitializations = false;
			break;
         		
		case OPT_SIGNATURE:
			specificsignature = opt_arg;     		
			break;
         		
		case OPT_ALL:
			compileall = true;     		
			startit = false;
			makeinitializations = false;
			break;
         		
		case OPT_SHOW:       /* Display options */
			for (j = 0; j < strlen(opt_arg); j++) {		
				switch (opt_arg[j]) {
				case 'a':
					showdisassemble = true;
					compileverbose=true;
					break;
				case 'c':
					showconstantpool = true;
					break;
				case 'd':
					showddatasegment = true;
					break;
				case 'i':
					showintermediate = true;
					compileverbose = true;
					break;
				case 'm':
					showmethods = true;
					break;
				case 'u':
					showutf = true;
					break;
				default:
					print_usage();
					exit(10);
				}
			}
			break;
			
		case OPT_OLOOP:
			opt_loops = true;
			break;

		case OPT_INLINING:
			for (j = 0; j < strlen(opt_arg); j++) {		
				switch (opt_arg[j]) {
				case 'n':
					useinlining = true;
					break;
				case 'v':
					inlinevirtuals = true;
					break;
				case 'e':
					inlineexceptions = true;
					break;
				case 'p':
					inlineparamopt = true;
					break;
				case 'o':
					inlineoutsiders = true;
					break;
				default:
					print_usage();
					exit(10);
				}
			}
			break;

		case OPT_RT:
			opt_rt = true;
			break;

		case OPT_XTA:
			opt_xta = false; /**not yet **/
			break;

		case OPT_VTA:
			/***opt_vta = true; not yet **/
			break;

		default:
			print_usage();
			exit(10);
		}
	}
   
   
	if (opt_ind >= argc) {
   		print_usage();
   		exit(10);
	}

   	mainstring = argv[opt_ind++];
   	for (i = strlen(mainstring) - 1; i >= 0; i--) {     /* Transform dots into slashes */
 	 	if (mainstring[i] == '.') mainstring[i] = '/';  /* in the class name */
	}


	/**************************** Program start *****************************/

	log_init(logfilename);
	if (verbose)
		log_text("CACAO started -------------------------------------------------------");

	/* initialize the garbage collector */
	gc_init(heapmaxsize, heapstartsize);

	native_setclasspath(classpath);
		
	tables_init();
	suck_init(classpath);

	cacao_initializing = true;

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
  	initThreadsEarly();
#endif

	loader_init((u1 *) &dummy);

	native_loadclasses();

	jit_init();

#if defined(USE_THREADS)
  	initThreads((u1*) &dummy);
#endif

	*threadrootmethod=0;

	/*That's important, otherwise we get into trouble, if the Runtime static
	  initializer is called before (circular dependency. This is with
	  classpath 0.09. Another important thing is, that this has to happen
	  after initThreads!!! */

	if (!class_init(class_new(utf_new_char("java/lang/System"))))
		throw_main_exception_exit();

	cacao_initializing = false;

	/************************* Start worker routines ********************/

	if (startit) {
		methodinfo *mainmethod;
		java_objectarray *a; 

		/* create, load and link the main class */
		mainclass = class_new(utf_new_char(mainstring));

		if (!class_load(mainclass))
			throw_main_exception_exit();

		if (!class_link(mainclass))
			throw_main_exception_exit();

		mainmethod = class_resolveclassmethod(mainclass,
											  utf_new_char("main"), 
											  utf_new_char("([Ljava/lang/String;)V"),
											  mainclass,
											  false);

		/* problems with main method? */
/*  		if (*exceptionptr) */
/*  			throw_exception_exit(); */

		/* there is no main method or it isn't static */
		if (!mainmethod || !(mainmethod->flags & ACC_STATIC)) {
			*exceptionptr =
				new_exception_message(string_java_lang_NoSuchMethodError,
									  "main");
			throw_main_exception_exit();
		}

		a = builtin_anewarray(argc - opt_ind, class_java_lang_String);
		for (i = opt_ind; i < argc; i++) {
			a->data[i - opt_ind] = 
				(java_objectheader *) javastring_new(utf_new_char(argv[i]));
		}

#ifdef TYPEINFO_DEBUG_TEST
		/* test the typeinfo system */
		typeinfo_test();
#endif
		/*class_showmethods(currentThread->group->header.vftbl->class);	*/

		*threadrootmethod = mainmethod;


		/* here we go... */
		asm_calljavafunction(mainmethod, a, NULL, NULL, NULL);

		/* exception occurred? */
		if (*exceptionptr)
			throw_main_exception();

#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
		joinAllThreads();
#else
  		killThread(currentThread);
#endif
#endif

		/* now exit the JavaVM */

		cacao_exit(0);
	}

	/************* If requested, compile all methods ********************/

	if (compileall) {
		classinfo *c;
		methodinfo *m;
		u4 slot;
		s4 i;

		/* create all classes found in the classpath */
		/* XXX currently only works with zip/jar's */
		create_all_classes();

		/* load and link all classes */
		for (slot = 0; slot < class_hash.size; slot++) {
			c = class_hash.ptr[slot];

			while (c) {
				if (!c->loaded)
					class_load(c);

				if (!c->linked)
					class_link(c);

				/* compile all class methods */
				for (i = 0; i < c->methodscount; i++) {
					m = &(c->methods[i]);
					if (m->jcode) {
						(void) jit_compile(m);
					}
				}

				c = c->hashlink;
			}
		}
	}


	/******** If requested, compile a specific method ***************/

	if (specificmethodname) {
		methodinfo *m;

		/* create, load and link the main class */
		mainclass = class_new(utf_new_char(mainstring));

		if (!class_load(mainclass))
			throw_main_exception_exit();

		if (!class_link(mainclass))
			throw_main_exception_exit();

		if (specificsignature) {
			m = class_resolveclassmethod(mainclass,
										 utf_new_char(specificmethodname),
										 utf_new_char(specificsignature),
										 mainclass,
										 false);
		} else {
			m = class_resolveclassmethod(mainclass,
										 utf_new_char(specificmethodname),
										 NULL,
										 mainclass,
										 false);
		}

		if (!m) {
			char message[MAXLOGTEXT];
			sprintf(message, "%s%s", specificmethodname,
					specificsignature ? specificsignature : "");

			*exceptionptr =
				new_exception_message(string_java_lang_NoSuchMethodException,
									  message);
										 
			throw_main_exception_exit();
		}
		
		jit_compile(m);
	}

	cacao_shutdown(0);

	/* keep compiler happy */

	return 0;
}


/* cacao_exit ******************************************************************

   Calls java.lang.Runtime.exit(I)V to exit the JavaVM correctly.

*******************************************************************************/

void cacao_exit(s4 status)
{
	classinfo *c;
	methodinfo *m;
	java_lang_Runtime *rt;

	/* class should already be loaded, but who knows... */

	c = class_new(utf_new_char("java/lang/Runtime"));

	if (!class_load(c))
		throw_main_exception_exit();

	if (!class_link(c))
		throw_main_exception_exit();

	/* first call Runtime.getRuntime()Ljava.lang.Runtime; */

	m = class_resolveclassmethod(c,
								 utf_new_char("getRuntime"),
								 utf_new_char("()Ljava/lang/Runtime;"),
								 class_java_lang_Object,
								 true);

	if (!m)
		throw_main_exception_exit();

	rt = (java_lang_Runtime *) asm_calljavafunction(m,
													(void *) 0,
													NULL,
													NULL,
													NULL);

	/* exception occurred? */

	if (*exceptionptr)
		throw_main_exception_exit();

	/* then call Runtime.exit(I)V */

	m = class_resolveclassmethod(c,
								 utf_new_char("exit"),
								 utf_new_char("(I)V"),
								 class_java_lang_Object,
								 true);
	
	if (!m)
		throw_main_exception_exit();

	asm_calljavafunction(m, rt, (void *) 0, NULL, NULL);

	/* this should never happen */

	throw_cacao_exception_exit(string_java_lang_InternalError,
							   "Problems with Runtime.exit(I)V");
}


/*************************** Shutdown function *********************************

	Terminates the system immediately without freeing memory explicitly (to be
	used only for abnormal termination)
	
*******************************************************************************/

void cacao_shutdown(s4 status)
{
	/**** RTAprint ***/

	if (verbose || getcompilingtime || opt_stat) {
		log_text("CACAO terminated by shutdown");
		if (opt_stat)
			print_stats();
		if (getcompilingtime)
			print_times();
		mem_usagelog(0);
		dolog("Exit status: %d\n", (int) status);
	}

	exit(status);
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
