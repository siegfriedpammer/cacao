/****************************** compiler.c *************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Enth"alt die Funktionen mit denen die JavaVM - Methoden in Maschinencode
	"ubersetzt werden.
	Ein Aufruf vom compiler_compile "ubersetzt genau eine Methode.
	Alle in diesem Modul global definierten Variablen gelten nur f"ur 
	eben diese gerade in der "Ubersetzung befindlichen Methode.

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
	         Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at
	Changes: Mark Probst         EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/10/22

*******************************************************************************/

#include "global.h"
#include "compiler.h"

#include "loader.h"
#include "tables.h"
#include "builtin.h"
#include "native.h"
#include "asmpart.h"

#include "threads/thread.h"              /* schani */


/*************************** globale Schalter ********************************/

/**************************  now all in newcomp.c

bool compileverbose = false;
bool showstack = false;
bool showintermediate = false;
bool showdisassemble = false; 
int  optimizelevel = 0;

bool checkbounds = true;
bool checknull = true;
bool checkfloats = true;
bool checksync = true;

bool getcompilingtime = false;
long int compilingtime = 0;
int  has_ext_instr_set = 0;

bool statistics = false;         

int count_jit_calls = 0;
int count_methods = 0;
int count_spills = 0;
int count_pcmd_activ = 0;
int count_pcmd_drop = 0;
int count_pcmd_zero = 0;
int count_pcmd_const_store = 0;
int count_pcmd_const_alu = 0;
int count_pcmd_const_bra = 0;
int count_pcmd_load = 0;
int count_pcmd_move = 0;
int count_pcmd_store = 0;
int count_pcmd_store_comb = 0;
int count_pcmd_op = 0;
int count_pcmd_mem = 0;
int count_pcmd_met = 0;
int count_pcmd_bra = 0;
int count_pcmd_table = 0;
int count_pcmd_return = 0;
int count_pcmd_returnx = 0;
int count_check_null = 0;
int count_javainstr = 0;
int count_javacodesize = 0;
int count_javaexcsize = 0;
int count_calls = 0;
int count_tryblocks = 0;
int count_code_len = 0;
int count_data_len = 0;
int count_cstub_len = 0;
int count_nstub_len = 0;

********************/


/************************ die Datentypen f"ur den Compiler *******************/ 

#include "comp/defines.c"



/******************* globale Variablen fuer den Compiler *********************/

static methodinfo *method;      /* Zeiger auf die Methodenstruktur */
static unicode   *descriptor;   /* Typbeschreibung der Methode */
static classinfo *class;        /* Klasse, in der die Methode steht */
	
static s4 maxstack;             /* maximale Gr"osse des JavaVM-Stacks */
static s4 maxlocals;            /* maximale Anzahl der JavaVM-Variablen */
static u4 jcodelength;          /* L"ange des JavaVM-Codes */
static u1 *jcode;               /* Zeiger auf den JavaVM-Code */
static s4 exceptiontablelength; /* L"ange der Exceptiontable */
static exceptiontable *extable; /* Zeiger auf die Exceptiontable */


static list reachedblocks;      /* Die Listenstruktur f"ur alle vom Parser 
                                   bereits irgendwie erreichten Bl"ocke */
static list finishedblocks;     /* Die Listenstruktur f"ur alle Bl"ocke, die
                                   vom Parser bereits durchgearbeitet wurden */

static basicblock **blocks;     /* Eine Tabelle, so lang wie der JavaVM-Code, */
                                /* in der an jeder Stelle, an der ein */
                                /* Basicblock beginnt, der Zeiger auf die */
                                /* ensprechende Basicblock-Struktur einge- */
                                /* tragen ist. */

static bool isleafmethod;       /* true, wenn die Methode KEINE weiteren 
                                   Unterprogramme mehr aufruft */

static s4        mparamnum;     /* Die Anzahl der Parameter (incl. this) */
static u1       *mparamtypes;   /* Die Typen aller Parameter (TYPE_INT,...) */
static s4        mreturntype;   /* R"uckgabewert der Methode */
static varinfo **mparamvars;    /* Die PCMD-Variablen, die die Parameter */
                                /*   zu Methodenstart enthalten sollen */


static chain *uninitializedclasses;  
                                /* Eine Tabelle aller von der Methode */
                                /* irgendwie ben"otigten Klassen, die */
                                /* vor dem Start der Methode initialisiert */
                                /* werden m"ussen (wenn sie es noch nicht */
                                /* sind) */
                                

/************************ Subsysteme des Compilers ***************************/


#include "comp/tools.c"    /* ein paar n"utzliche Hilfsfunktionen */
#include "comp/mcode.c"    /* systemUNabh"angiger Teil des Codegenerators */

#include "comp/reg.c"      /* Registerverwaltung */
#include "comp/var.c"      /* Die Verwaltung der PCMD-Variblen */
#include "comp/pcmd.c"     /* Funktionen f"ur die Pseudocommandos (=PCMD) */
#include "comp/local.c"    /* Verwaltung der lokalen JavaVM-Variablen */
#include "comp/stack.c"    /* Verwaltung des JavaVM-Stacks */
#include "comp/regalloc.c" /* Registerallokator */

#include "sysdep/gen.c"    /* systemABh"angiger Codegenerator */
#include "sysdep/disass.c" /* Disassembler (nur zu Debug-Zwecken) */ 

#include "comp/block.c"    /* Verwaltung der basic blocks */ 
#include "comp/parse.c"    /* JavaVM - parser */




/****** Die Dummy-Function (wird verwendet, wenn kein Code vorhanden ist) ****/

static void* do_nothing_function() 
{
	return NULL;
}




/******************************************************************************/
/*********************** eine Methode compilieren *****************************/
/******************************************************************************/


methodptr compiler_compile (methodinfo *m)
{
	u4 i;
	basicblock *b;
	long int starttime=0,stoptime=0;
	long int dumpsize;
	

	/*** Wenn schon ein Maschinencode vorliegt, dann sofort beenden ****/

	count_jit_calls++;             /* andi   */
	if (m->entrypoint) return m->entrypoint;

	intsDisable();                 /* schani */
	

	/**************** Marke fuer den DUMP-Speicher aufheben *****************/

	dumpsize = dump_size ();


	/**************** Zeit messen *******************************************/

	count_methods++;             /* andi   */
	if (getcompilingtime) starttime=getcputime();

	/*** Meldung ausgeben. Wenn kein JavaVM-Code vorhanden ist, beenden ****/

	if (m->jcode) {
		if (compileverbose) {
			sprintf (logtext, "Compiling: ");
			unicode_sprint (logtext+strlen(logtext), m->class->name);
			strcpy (logtext+strlen(logtext), ".");
			unicode_sprint (logtext+strlen(logtext), m->name);
			unicode_sprint (logtext+strlen(logtext), m->descriptor);
			dolog ();
			}
		}
	else {
		sprintf (logtext, "No code given for: ");
		unicode_sprint (logtext+strlen(logtext), m->class->name);
		strcpy (logtext+strlen(logtext), ".");
		unicode_sprint (logtext+strlen(logtext), m->name);
		unicode_sprint (logtext+strlen(logtext), m->descriptor);
		dolog ();
		intsRestore();           /* schani */
		return (methodptr) do_nothing_function;
		}


	/*********** Initialisieren der Variablen und Subsysteme *****************/

	isleafmethod = true;              /* bis sich das Gegenteil herausstellt */

	method = m;
	descriptor = m->descriptor;
	class = m->class;
	
	maxstack = m->maxstack;
	maxlocals = m->maxlocals;
	jcodelength = m->jcodelength;
	jcode = m->jcode;
	count_tryblocks += (exceptiontablelength = m->exceptiontablelength);
	extable = m->exceptiontable;

#ifdef STATISTICS
	count_javacodesize += jcodelength + 18;
	count_javaexcsize += exceptiontablelength * 8;
#endif

	list_init (&reachedblocks,   OFFSET(basicblock, linkage) );
	list_init (&finishedblocks,  OFFSET(basicblock, linkage) );

	blocks = DMNEW (basicblock*, jcodelength);
	for (i=0; i<jcodelength; i++) blocks[i] = NULL;


	descriptor2types (descriptor, (m->flags & ACC_STATIC) != 0,
	                  &mparamnum, &mparamtypes, &mreturntype);
	m->paramcount = mparamnum;
	m->returntype = mreturntype;
	mparamvars = DMNEW (varid, mparamnum);

	reg_init ();
	mcode_init ();
	var_init();
	local_init();

	uninitializedclasses = chain_new(); 
		/* aktuelle Klasse zur Liste der m"oglicherweise zu 
		   initialisierenden Klassen dazugeben */
	compiler_addinitclass (m->class);


	/************************ Compilieren  ************************/
	
            /* Fuer jedes Sprungziel einen eigenen Block erzeugen */
	block_firstscann ();

			/* Den ersten Block als erreicht markieren, der Stack ist leer */
	stack_init();
	subroutine_set(NULL);
	block_reach ( block_find(0) );
	
			/* Alle schon erreichten Bl"ocke durchgehen und fertig machen */
	while ( (b = list_first (&reachedblocks)) ) {
		list_remove (&reachedblocks, b);
		list_addlast (&finishedblocks, b);
		b -> finished = true;

		pcmd_init ( &(b->pcmdlist) );
		parse (b);
		}
	
	input_args_prealloc ();

			/* F"ur alle Bl"ocke die Registerbelegung durchfuehren */
	b = list_first (&finishedblocks);
	while (b) {
		regalloc (b);
		b = list_next (&finishedblocks, b);
		}


		    /* Registerbelegung fuer die lokalen JAVA-Variablen */
	local_regalloc ();

	

	
	/**************** Maschinencode generieren **********************/

	gen_computestackframe ();
	gen_header ();


	for (i=0; i<jcodelength; i++) {
		b = blocks[i];
		if (b) if (b->reached) block_genmcode (b);
		}
	
	b = list_first (&finishedblocks);
	while (b) {
		if (b->type != BLOCKTYPE_JAVA) block_genmcode (b);
		b = list_next (&finishedblocks, b);
		}

	
	mcode_finish ();

	
	/*********** Zwischendarstellungen auf Wunsch ausgeben **********/
		
	if (showintermediate) {
		printf ("Leaf-method: %s\n", isleafmethod ? "YES":"NO");
		printf ("Parameters: ");
		for (i=0; i<mparamnum; i++) var_display (mparamvars[i]);
		printf ("\n");
		printf ("Max locals: %d\n", (int) maxlocals);
		printf ("Max stack:  %d\n", (int) maxstack);

		for (i=0; i<jcodelength; i++) {
			b = blocks[i];
			if (b) if (b->reached) block_display (b);
			}
		b = list_first (&finishedblocks);
		while (b) {
			if (b->type != BLOCKTYPE_JAVA) block_display (b);
			b = list_next (&finishedblocks, b);
			}
			
		var_displayall();
		fflush (stdout);
		}

	if (showdisassemble) {
		dseg_display ();
		disassemble ( (void*) (m->mcode + dseglen), mcodelen);
		fflush (stdout);
		}



	/***************** Dump-Speicher zurueckgeben *************/

	dump_release (dumpsize);


	/******************* Zeit messen **************************/
	
	if (getcompilingtime) {
		stoptime = getcputime();
		compilingtime += (stoptime-starttime); 
		}

	/******** Alle Klassen initialisieren, die gebraucht wurden *******/

	{
		chain *u = uninitializedclasses;    /* wegen reentrant-F"ahigkeit */ 
		classinfo *c;                       /* d"urfen ab hier keine */
		                                    /* globalen Variablen verwendet */
		while ( (c = chain_first(u)) ) {    /* werden */
			chain_remove (u);
			
			class_init (c);          	 	/* ruft unter Umst"anden wieder */
		                              		/* den Compiler auf */
			}
		chain_free (u);
	}

	intsRestore();                   /* schani */



	/****** Return pointer to the methods entry point **********/
		
	return m -> entrypoint;

}



/***************** Funktionen zum Initialisieren und Terminieren *************/

void compiler_init ()
{
	u4 i;

	has_ext_instr_set = ! has_no_x_instr_set();

	for (i=0; i<256;i++) stdopdescriptors[i]=NULL;

	for (i=0; i<sizeof(stdopdescriptortable)/sizeof(stdopdescriptor); i++) {
		
		if (stdopdescriptortable[i].isfloat && checkfloats) {
			stdopdescriptortable[i].supported = false;
			}

		stdopdescriptors[stdopdescriptortable[i].opcode] = 
		   &(stdopdescriptortable[i]);
		}
}


void compiler_close()
{
	reg_close ();
	mcode_close();
}

