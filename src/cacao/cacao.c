/* main.c **********************************************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Contains main() and variables for the global options.
	This module does the following tasks:
	   - Command line option handling
	   - Calling initialization routines
	   - Calling the class loader
	   - Running the main method

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
	Changes: Andi Krall          EMAIL: cacao@complang.tuwien.ac.at
	         Mark Probst         EMAIL: cacao@complang.tuwien.ac.at
			 Philipp Tomsich     EMAIL: cacao@complang.tuwien.ac.at

	Last Change: $Id: cacao.c 196 2003-01-21 12:03:06Z stefan $

*******************************************************************************/

#include "global.h"

#include "tables.h"
#include "loader.h"
#include "jit.h"
#ifdef OLD_COMPILER
#include "compiler.h"
#endif

#include "asmpart.h"
#include "builtin.h"
#include "native.h"

#include "threads/thread.h"

bool compileall = false;
int  newcompiler = true;     		
bool verbose =  false;
#ifdef NEW_GC
bool new_gc = false;
#endif

static bool showmethods = false;
static bool showconstantpool = false;
static bool showutf = false;
static classinfo *topclass;

#ifndef USE_THREADS
void **stackbottom = 0;
#endif


/* internal function: get_opt *************************************************
	
	decodes the next command line option
	
******************************************************************************/

#define OPT_DONE  -1
#define OPT_ERROR  0
#define OPT_IGNORE 1

#define OPT_CLASSPATH   2
#define OPT_D           3
#define OPT_MS          4
#define OPT_MX          5
#define OPT_VERBOSE1    6
#define OPT_VERBOSE     7
#define OPT_VERBOSEGC   8
#define OPT_VERBOSECALL 9
#define OPT_IEEE        10
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
#ifdef OLD_COMPILER
#define OPT_OLD         21
#endif
#ifdef NEW_GC
#define OPT_GC1         22
#define OPT_GC2         23
#endif
#define OPT_OLOOP       24

struct {char *name; bool arg; int value;} opts[] = {
	{"classpath",   true,   OPT_CLASSPATH},
	{"D",           true,   OPT_D},
	{"ms",          true,   OPT_MS},
	{"mx",          true,   OPT_MX},
	{"noasyncgc",   false,  OPT_IGNORE},
	{"noverify",    false,  OPT_IGNORE},
	{"oss",         true,   OPT_IGNORE},
	{"ss",          true,   OPT_IGNORE},
	{"v",           false,  OPT_VERBOSE1},
	{"verbose",     false,  OPT_VERBOSE},
	{"verbosegc",   false,  OPT_VERBOSEGC},
	{"verbosecall", false,  OPT_VERBOSECALL},
	{"ieee",        false,  OPT_IEEE},
	{"softnull",    false,  OPT_SOFTNULL},
	{"time",        false,  OPT_TIME},
	{"stat",        false,  OPT_STAT},
	{"log",         true,   OPT_LOG},
	{"c",           true,   OPT_CHECK},
	{"l",           false,  OPT_LOAD},
	{"m",           true,   OPT_METHOD},
	{"sig",         true,   OPT_SIGNATURE},
	{"s",           true,   OPT_SHOW},
	{"all",         false,  OPT_ALL},
#ifdef OLD_COMPILER
	{"old",         false,  OPT_OLD},
#endif
#ifdef NEW_GC
	{"gc1",         false,  OPT_GC1},
	{"gc2",         false,  OPT_GC2},
#endif
	{"oloop",       false,  OPT_OLOOP},
	{NULL,  false, 0}
};

static int opt_ind = 1;
static char *opt_arg;

static int get_opt (int argc, char **argv) 
{
	char *a;
	int i;
	
	if (opt_ind >= argc) return OPT_DONE;
	
	a = argv[opt_ind];
	if (a[0] != '-') return OPT_DONE;

	for (i=0; opts[i].name; i++) {
		if (! opts[i].arg) {
			if (strcmp(a+1, opts[i].name) == 0) {  /* boolean option found */
				opt_ind++;
				return opts[i].value;
			}
		}
		else {
			if (strcmp(a+1, opts[i].name) == 0) { /* parameter option found */
				opt_ind++;
				if (opt_ind < argc) {
					opt_arg = argv[opt_ind];
					opt_ind++;
					return opts[i].value;
				}
				return OPT_ERROR;
			}
			else {
				size_t l = strlen(opts[i].name);
				if (strlen(a+1) > l) {
					if (memcmp (a+1, opts[i].name, l)==0) {
						opt_ind++;
						opt_arg = a+1+l;
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
	printf ("USAGE: cacao [options] classname [program arguments\n");
	printf ("Options:\n");
	printf ("          -classpath path ...... specify a path to look for classes\n");
	printf ("          -Dpropertyname=value . add an entry to the property list\n");
	printf ("          -mx maxmem[k|m] ...... specify the size for the heap\n");
	printf ("          -ms initmem[k|m] ..... specify the initial size for the heap\n");
	printf ("          -v ................... write state-information\n");
	printf ("          -verbose ............. write more information\n");
	printf ("          -verbosegc ........... write message for each GC\n");
	printf ("          -verbosecall ......... write message for each call\n");
	printf ("          -ieee ................ use ieee compliant arithmetic\n");
	printf ("          -softnull ............ use software nullpointer check\n");
	printf ("          -time ................ measure the runtime\n");
	printf ("          -stat ................ detailed compiler statistics\n");
	printf ("          -log logfile ......... specify a name for the logfile\n");
	printf ("          -c(heck)b(ounds) ..... don't check array bounds\n");
	printf ("                  s(ync) ....... don't check for synchronization\n");
	printf ("          -oloop ............... optimize array accesses in loops\n"); 
	printf ("          -l ................... don't start the class after loading\n");
	printf ("          -all ................. compile all methods, no execution\n");
#ifdef OLD_COMPILER
	printf ("          -old ................. use old JIT compiler\n");
#endif
#ifdef NEW_GC
	printf ("          -gc1 ................. use the old garbage collector (default)\n");
	printf ("          -gc2 ................. use the new garbage collector\n");
#endif
	printf ("          -m ................... compile only a specific method\n");
	printf ("          -sig ................. specify signature for a specific method\n");
	printf ("          -s(how)a(ssembler) ... show disassembled listing\n");
	printf ("                 c(onstants) ... show the constant pool\n");
	printf ("                 d(atasegment).. show data segment listing\n");
	printf ("                 i(ntermediate). show intermediate representation\n");
	printf ("                 m(ethods)...... show class fields and methods\n");
#ifdef OLD_COMPILER
	printf ("                 s(tack) ....... show stack for every javaVM-command\n");
#endif
	printf ("                 u(tf) ......... show the utf - hash\n");
}   



/***************************** Function: print_times *********************

	Prints a summary of CPU time usage.

**************************************************************************/

static void print_times()
{
	long int totaltime = getcputime();
	long int runtime = totaltime - loadingtime - compilingtime;

	sprintf (logtext, "Time for loading classes: %ld secs, %ld millis",
	     loadingtime / 1000000, (loadingtime % 1000000) / 1000);
	dolog();
	sprintf (logtext, "Time for compiling code:  %ld secs, %ld millis",
	     compilingtime / 1000000, (compilingtime % 1000000) / 1000);
	dolog();
	sprintf (logtext, "Time for running program: %ld secs, %ld millis",
	     runtime / 1000000, (runtime % 1000000) / 1000);
	dolog();
	sprintf (logtext, "Total time: %ld secs, %ld millis",
	     totaltime / 1000000, (totaltime % 1000000) / 1000);
	dolog();
}






/***************************** Function: print_stats *********************

	outputs detailed compiler statistics

**************************************************************************/

static void print_stats()
{
	sprintf (logtext, "Number of JitCompiler Calls: %d", count_jit_calls);
	dolog();
	sprintf (logtext, "Number of compiled Methods: %d", count_methods);
	dolog();
	sprintf (logtext, "Number of max basic blocks per method: %d", count_max_basic_blocks);
	dolog();
	sprintf (logtext, "Number of compiled basic blocks: %d", count_basic_blocks);
	dolog();
	sprintf (logtext, "Number of max JavaVM-Instructions per method: %d", count_max_javainstr);
	dolog();
	sprintf (logtext, "Number of compiled JavaVM-Instructions: %d", count_javainstr);
	dolog();
	sprintf (logtext, "Size of compiled JavaVM-Instructions:   %d(%d)", count_javacodesize,
	                                              count_javacodesize - count_methods * 18);
	dolog();
	sprintf (logtext, "Size of compiled Exception Tables:      %d", count_javaexcsize);
	dolog();
	sprintf (logtext, "Value of extended instruction set var:  %d", has_ext_instr_set);
	dolog();
	sprintf (logtext, "Number of Alpha-Instructions: %d", count_code_len >> 2);
	dolog();
	sprintf (logtext, "Number of Spills: %d", count_spills);
	dolog();
	sprintf (logtext, "Number of Activ    Pseudocommands: %5d", count_pcmd_activ);
	dolog();
	sprintf (logtext, "Number of Drop     Pseudocommands: %5d", count_pcmd_drop);
	dolog();
	sprintf (logtext, "Number of Const    Pseudocommands: %5d (zero:%5d)", count_pcmd_load, count_pcmd_zero);
	dolog();
	sprintf (logtext, "Number of ConstAlu Pseudocommands: %5d (cmp: %5d, store:%5d)", count_pcmd_const_alu, count_pcmd_const_bra, count_pcmd_const_store);
	dolog();
	sprintf (logtext, "Number of Move     Pseudocommands: %5d", count_pcmd_move);
	dolog();
	sprintf (logtext, "Number of Load     Pseudocommands: %5d", count_load_instruction);
	dolog();
	sprintf (logtext, "Number of Store    Pseudocommands: %5d (combined: %5d)", count_pcmd_store, count_pcmd_store - count_pcmd_store_comb);
	dolog();
	sprintf (logtext, "Number of OP       Pseudocommands: %5d", count_pcmd_op);
	dolog();
	sprintf (logtext, "Number of DUP      Pseudocommands: %5d", count_dup_instruction);
	dolog();
	sprintf (logtext, "Number of Mem      Pseudocommands: %5d", count_pcmd_mem);
	dolog();
	sprintf (logtext, "Number of Method   Pseudocommands: %5d", count_pcmd_met);
	dolog();
	sprintf (logtext, "Number of Branch   Pseudocommands: %5d (rets:%5d, Xrets: %5d)",
	                  count_pcmd_bra, count_pcmd_return, count_pcmd_returnx);
	dolog();
	sprintf (logtext, "Number of Table    Pseudocommands: %5d", count_pcmd_table);
	dolog();
	sprintf (logtext, "Number of Useful   Pseudocommands: %5d", count_pcmd_table +
	         count_pcmd_bra + count_pcmd_load + count_pcmd_mem + count_pcmd_op);
	dolog();
	sprintf (logtext, "Number of Null Pointer Checks:     %5d", count_check_null);
	dolog();
	sprintf (logtext, "Number of Array Bound Checks:      %5d", count_check_bound);
	dolog();
	sprintf (logtext, "Number of Try-Blocks: %d", count_tryblocks);
	dolog();
	sprintf (logtext, "Maximal count of stack elements:   %d", count_max_new_stack);
	dolog();
	sprintf (logtext, "Upper bound of max stack elements: %d", count_upper_bound_new_stack);
	dolog();
	sprintf (logtext, "Distribution of stack sizes at block boundary");
	dolog();
	sprintf (logtext, "    0    1    2    3    4    5    6    7    8    9    >=10");
	dolog();
	sprintf (logtext, "%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d", count_block_stack[0],
		count_block_stack[1],count_block_stack[2],count_block_stack[3],count_block_stack[4],
		count_block_stack[5],count_block_stack[6],count_block_stack[7],count_block_stack[8],
		count_block_stack[9],count_block_stack[10]);
	dolog();
	sprintf (logtext, "Distribution of store stack depth");
	dolog();
	sprintf (logtext, "    0    1    2    3    4    5    6    7    8    9    >=10");
	dolog();
	sprintf (logtext, "%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d", count_store_depth[0],
		count_store_depth[1],count_store_depth[2],count_store_depth[3],count_store_depth[4],
		count_store_depth[5],count_store_depth[6],count_store_depth[7],count_store_depth[8],
		count_store_depth[9],count_store_depth[10]);
	dolog();
	sprintf (logtext, "Distribution of store creator chains first part");
	dolog();
	sprintf (logtext, "    0    1    2    3    4    5    6    7    8    9  ");
	dolog();
	sprintf (logtext, "%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d", count_store_length[0],
		count_store_length[1],count_store_length[2],count_store_length[3],count_store_length[4],
		count_store_length[5],count_store_length[6],count_store_length[7],count_store_length[8],
		count_store_length[9]);
	dolog();
	sprintf (logtext, "Distribution of store creator chains second part");
	dolog();
	sprintf (logtext, "   10   11   12   13   14   15   16   17   18   19  >=20");
	dolog();
	sprintf (logtext, "%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d", count_store_length[10],
		count_store_length[11],count_store_length[12],count_store_length[13],count_store_length[14],
		count_store_length[15],count_store_length[16],count_store_length[17],count_store_length[18],
		count_store_length[19],count_store_length[20]);
	dolog();
	sprintf (logtext, "Distribution of analysis iterations");
	dolog();
	sprintf (logtext, "    1    2    3    4    >=5");
	dolog();
	sprintf (logtext, "%5d%5d%5d%5d%5d", count_analyse_iterations[0],count_analyse_iterations[1],
		count_analyse_iterations[2],count_analyse_iterations[3],count_analyse_iterations[4]);
	dolog();
	sprintf (logtext, "Distribution of basic blocks per method");
	dolog();
	sprintf (logtext, " <= 5 <=10 <=15 <=20 <=30 <=40 <=50 <=75  >75");
	dolog();
	sprintf (logtext, "%5d%5d%5d%5d%5d%5d%5d%5d%5d", count_method_bb_distribution[0],
		count_method_bb_distribution[1],count_method_bb_distribution[2],count_method_bb_distribution[3],
		count_method_bb_distribution[4],count_method_bb_distribution[5],count_method_bb_distribution[6],
		count_method_bb_distribution[7],count_method_bb_distribution[8]);
	dolog();
	sprintf (logtext, "Distribution of basic block sizes");
	dolog();
	sprintf (logtext,
	"  0    1    2    3    4   5   6   7   8   9 <13 <15 <17 <19 <21 <26 <31 >30");
	dolog();
	sprintf (logtext, "%3d%5d%5d%5d%4d%4d%4d%4d%4d%4d%4d%4d%4d%4d%4d%4d%4d%4d",
		count_block_size_distribution[0], count_block_size_distribution[1], count_block_size_distribution[2],
		count_block_size_distribution[3], count_block_size_distribution[4], count_block_size_distribution[5],
		count_block_size_distribution[6], count_block_size_distribution[7], count_block_size_distribution[8],
		count_block_size_distribution[9], count_block_size_distribution[10],count_block_size_distribution[11],
		count_block_size_distribution[12],count_block_size_distribution[13],count_block_size_distribution[14],
		count_block_size_distribution[15],count_block_size_distribution[16],count_block_size_distribution[17]);
	dolog();
	sprintf (logtext, "Size of Code Area (Kb):  %10.3f", (float) count_code_len / 1024);
	dolog();
	sprintf (logtext, "Size of data Area (Kb):  %10.3f", (float) count_data_len / 1024);
	dolog();
	sprintf (logtext, "Size of Class Infos (Kb):%10.3f", (float) (count_class_infos) / 1024);
	dolog();
	sprintf (logtext, "Size of Const Pool (Kb): %10.3f", (float) (count_const_pool_len + count_utf_len) / 1024);
	dolog();
	sprintf (logtext, "Size of Vftbl (Kb):      %10.3f", (float) count_vftbl_len / 1024);
	dolog();
	sprintf (logtext, "Size of comp stub (Kb):  %10.3f", (float) count_cstub_len / 1024);
	dolog();
	sprintf (logtext, "Size of native stub (Kb):%10.3f", (float) count_nstub_len / 1024);
	dolog();
	sprintf (logtext, "Size of Utf (Kb):        %10.3f", (float) count_utf_len / 1024);
	dolog();
	sprintf (logtext, "Size of VMCode (Kb):     %10.3f(%d)", (float) count_vmcode_len / 1024,
	                                              count_vmcode_len - 18 * count_all_methods);
	dolog();
	sprintf (logtext, "Size of ExTable (Kb):    %10.3f", (float) count_extable_len / 1024);
	dolog();
	sprintf (logtext, "Number of class loads:   %d", count_class_loads);
	dolog();
	sprintf (logtext, "Number of class inits:   %d", count_class_inits);
	dolog();
	sprintf (logtext, "Number of loaded Methods: %d\n\n", count_all_methods);
	dolog();

	sprintf (logtext, "Calls of utf_new: %22d", count_utf_new);
	dolog();
	sprintf (logtext, "Calls of utf_new (element found): %6d\n\n", count_utf_new_found);
	dolog();
}


/********** Function: class_compile_methods   (debugging only) ********/

void class_compile_methods ()
{
	int        i;
	classinfo  *c;
	methodinfo *m;
	
	c = list_first (&linkedclasses);
	while (c) {
		for (i = 0; i < c -> methodscount; i++) {
			m = &(c->methods[i]);
			if (m->jcode) {
#ifdef OLD_COMPILER
				if (newcompiler)
#endif
					(void) jit_compile(m);
#ifdef OLD_COMPILER
				else
					(void) compiler_compile(m);
#endif
				}
			}
		c = list_next (&linkedclasses, c);
		}
}

/*
 * void exit_handler(void)
 * -----------------------
 * The exit_handler function is called upon program termination to shutdown
 * the various subsystems and release the resources allocated to the VM.
 */

void exit_handler(void)
{
	/********************* Print debug tables ************************/
				
	if (showmethods) class_showmethods (topclass);
	if (showconstantpool)  class_showconstantpool (topclass);
	if (showutf)           utf_show ();

#ifdef USE_THREADS
	clear_thread_flags();		/* restores standard file descriptor
								   flags */
#endif

	/************************ Free all resources *******************/

	heap_close ();				/* must be called before compiler_close and
								   loader_close because finalization occurs
								   here */

#ifdef OLD_COMPILER
	compiler_close ();
#endif
	loader_close ();
	tables_close ( literalstring_free );

	if (verbose || getcompilingtime || statistics) {
		log_text ("CACAO terminated");
		if (statistics)
			print_stats ();
		if (getcompilingtime)
			print_times ();
		mem_usagelog(1);
	}
}

/************************** Function: main *******************************

   The main program.
   
**************************************************************************/

int main(int argc, char **argv)
{
	s4 i,j;
	char *cp;
	java_objectheader *local_exceptionptr = 0;
	void *dummy;
	
	/********** interne (nur fuer main relevante Optionen) **************/
   
	char logfilename[200] = "";
	u4 heapsize = 64000000;
	u4 heapstartsize = 200000;
	char classpath[500] = ".:/usr/local/lib/java/classes";
	bool startit = true;
	char *specificmethodname = NULL;
	char *specificsignature = NULL;

#ifndef USE_THREADS
	stackbottom = &dummy;
#endif
	
	if (0 != atexit(exit_handler))
		panic("unable to register exit_handler");

	/************ Collect info from the environment ************************/

	cp = getenv ("CLASSPATH");
	if (cp) {
		strcpy (classpath, cp);
	}

	/***************** Interpret the command line *****************/
   
	checknull = false;
	checkfloats = false;

	while ( (i = get_opt(argc,argv)) != OPT_DONE) {

		switch (i) {
		case OPT_IGNORE: break;
			
		case OPT_CLASSPATH:    
			strcpy (classpath + strlen(classpath), ":");
			strcpy (classpath + strlen(classpath), opt_arg);
			break;
				
		case OPT_D:
			{
				int n,l=strlen(opt_arg);
				for (n=0; n<l; n++) {
					if (opt_arg[n]=='=') {
						opt_arg[n] = '\0';
						attach_property (opt_arg, opt_arg+n+1);
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
			if (opt_arg[strlen(opt_arg)-1] == 'k') {
				j = 1024 * atoi(opt_arg);
			}
			else if (opt_arg[strlen(opt_arg)-1] == 'm') {
				j = 1024 * 1024 * atoi(opt_arg);
			}
			else j = atoi(opt_arg);
				
			if (i==OPT_MX) heapsize = j;
			else heapstartsize = j;
			break;

		case OPT_VERBOSE1:
			verbose = true;
			break;
								
		case OPT_VERBOSE:
			verbose = true;
			loadverbose = true;
			initverbose = true;
			compileverbose = true;
			break;
				
		case OPT_VERBOSEGC:
			collectverbose = true;
			break;
				
		case OPT_VERBOSECALL:
			runverbose = true;
			break;
				
		case OPT_IEEE:
			checkfloats = true;
			break;

		case OPT_SOFTNULL:
			checknull = true;
			break;

		case OPT_TIME:
			getcompilingtime = true;
			getloadingtime = true;
			break;
					
		case OPT_STAT:
			statistics = true;
			break;
					
		case OPT_LOG:
			strcpy (logfilename, opt_arg);
			break;
			
			
		case OPT_CHECK:
			for (j=0; j<strlen(opt_arg); j++) {
				switch (opt_arg[j]) {
				case 'b': checkbounds=false; break;
				case 's': checksync=false; break;
				default:  print_usage();
					exit(10);
				}
			}
			break;
			
		case OPT_LOAD:
			startit = false;
			makeinitializations = false;
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
         		
#ifdef OLD_COMPILER
		case OPT_OLD:
			newcompiler = false;     		
			checknull = true;
			break;
#endif

#ifdef NEW_GC
		case OPT_GC2:
			new_gc = true;
			break;

		case OPT_GC1:
			new_gc = false;
			break;
#endif
         		
		case OPT_SHOW:       /* Display options */
			for (j=0; j<strlen(opt_arg); j++) {		
				switch (opt_arg[j]) {
				case 'a':  showdisassemble=true; compileverbose=true; break;
				case 'c':  showconstantpool=true; break;
				case 'd':  showddatasegment=true; break;
				case 'i':  showintermediate=true; compileverbose=true; break;
				case 'm':  showmethods=true; break;
#ifdef OLD_COMPILER
				case 's':  showstack=true; compileverbose=true; break;
#endif
				case 'u':  showutf=true; break;
				default:   print_usage();
					exit(10);
				}
			}
			break;
			
		case OPT_OLOOP:
			opt_loops = true;
			break;

		default:
			print_usage();
			exit(10);
		}
			
			
	}
   
   
	if (opt_ind >= argc) {
   		print_usage ();
   		exit(10);
	}


	/**************************** Program start *****************************/

	log_init (logfilename);
	if (verbose) {
		log_text (
				  "CACAO started -------------------------------------------------------");
	}
	
	suck_init (classpath);
	native_setclasspath (classpath);
		
	tables_init();
	heap_init(heapsize, heapstartsize, &dummy);
	loader_init();
#ifdef OLD_COMPILER
	compiler_init();
#endif
	jit_init();

	native_loadclasses ();


	/*********************** Load JAVA classes  ***************************/
   
   	cp = argv[opt_ind++];
   	for (i=strlen(cp)-1; i>=0; i--) {     /* Transform dots into slashes */
 	 	if (cp[i]=='.') cp[i]='/';        /* in the class name */
	}

	topclass = loader_load ( utf_new_char (cp) );

	if (exceptionptr != 0)
	{
		printf ("#### Class loader has thrown: ");
		utf_display (exceptionptr->vftbl->class->name);
		printf ("\n");

		exceptionptr = 0;
	}

	if (topclass == 0)
	{
		printf("#### Could not find top class - exiting\n");
		exit(1);
	}

	gc_init();

#ifdef USE_THREADS
	initThreads((u1*)&dummy);                   /* schani */
#endif

	/************************* Start worker routines ********************/

	if (startit) {
		methodinfo *mainmethod;
		java_objectarray *a; 

		heap_addreference((void**) &a);

		mainmethod = class_findmethod (
									   topclass,
									   utf_new_char ("main"), 
									   utf_new_char ("([Ljava/lang/String;)V")
									   );
		if (!mainmethod) panic ("Can not find method 'void main(String[])'");
		if ((mainmethod->flags & ACC_STATIC) != ACC_STATIC) panic ("main is not static!");
			
		a = builtin_anewarray (argc - opt_ind, class_java_lang_String);
		for (i=opt_ind; i<argc; i++) {
			a->data[i-opt_ind] = javastring_new (utf_new_char (argv[i]) );
		}
		local_exceptionptr = asm_calljavamethod (mainmethod, a, NULL,NULL,NULL );
	
		if (local_exceptionptr) {
			printf ("#### Program has thrown: ");
			utf_display (local_exceptionptr->vftbl->class->name);
			printf ("\n");
		}

#ifdef USE_THREADS
		killThread(currentThread);
#endif
		fprintf(stderr, "still here\n");
	}

	/************* If requested, compile all methods ********************/

	if (compileall) {
		class_compile_methods();
	}


	/******** If requested, compile a specific method ***************/

	if (specificmethodname) {
		methodinfo *m;
		if (specificsignature)
			m = class_findmethod(topclass, 
								 utf_new_char(specificmethodname),
								 utf_new_char(specificsignature));
		else
			m = class_findmethod(topclass, 
								 utf_new_char(specificmethodname), NULL);
		if (!m) panic ("Specific method not found");
#ifdef OLD_COMPILER
		if (newcompiler)
#endif
			(void) jit_compile(m);
#ifdef OLD_COMPILER
		else
			(void) compiler_compile(m);
#endif
	}

	exit(0);
}



/************************************ Shutdown function *********************************

	Terminates the system immediately without freeing memory explicitly (to be
	used only for abnormal termination)
	
*****************************************************************************************/

void cacao_shutdown(s4 status)
{
	if (verbose || getcompilingtime || statistics) {
		log_text ("CACAO terminated by shutdown");
		if (statistics)
			print_stats ();
		if (getcompilingtime)
			print_times ();
		mem_usagelog(0);
		sprintf (logtext, "Exit status: %d\n", (int) status);
		dolog();
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
