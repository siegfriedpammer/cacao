/* -*- mode: c; tab-width: 4; c-basic-offset: 4 -*- */
/****************************** headers.c **************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Dieser Modul ersetzt f"ur den Headerfile-Betrieb den Modul 'main',
	und 'f"alscht' einige Verweise auf externe Module (damit nicht schon 
	alle Module des eigentlichen Programmes fertig sein m"ussen, was ja
	unm"oglich w"are, da die Headerfile-Tabellen ja erst hier und jetzt
	generiert werden).

	Dieser Modul ist ein ziemlich schneller Hack und dementsprechend
	schlecht (nicht) kommentiert.

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
	Changes: Mark Probst         EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/05/23

*******************************************************************************/

#include "global.h"

#include "tables.h"
#include "loader.h"


/******* verschiedene externe Funktionen "faelschen" (=durch Dummys ersetzen), 
  damit der Linker zufrieden ist *********/
 
functionptr native_findfunction 
  (unicode *cname, unicode *mname, unicode *desc, bool isstatic)
{ return NULL; }

java_objectheader *literalstring_new (unicode *text)
{ return NULL; }

java_objectheader *javastring_new (unicode *text)         /* schani */
{ return NULL; }

void synchronize_caches() { }
void asm_call_jit_compiler () { }
void asm_calljavamethod () { }
void asm_dumpregistersandcall () { }

s4 new_builtin_idiv (s4 a, s4 b) {return 0;}
s4 new_builtin_irem (s4 a, s4 b) {return 0;}
s8 new_builtin_ldiv (s8 a, s8 b) {return 0;}
s8 new_builtin_lrem (s8 a, s8 b) {return 0;}


void new_builtin_monitorenter (java_objectheader *o) {}
void new_builtin_monitorexit (java_objectheader *o) {}

s4 new_builtin_checkcast(java_objectheader *o, classinfo *c)
                        {return 0;}
s4 new_builtin_checkarraycast
	(java_objectheader *o, constant_arraydescriptor *d)
	{return 0;}

void new_builtin_aastore (java_objectarray *a, s4 index, java_objectheader *o) {}

u1 *createcompilerstub (methodinfo *m) {return NULL;}
u1 *createnativestub (functionptr f, methodinfo *m) {return NULL;}
u1 *ncreatenativestub (functionptr f, methodinfo *m) {return NULL;}

void removecompilerstub (u1 *stub) {}
void removenativestub (u1 *stub) {}

void perform_alpha_threadswitch (u1 **from, u1 **to) {}
u1* initialize_thread_stack (void *func, u1 *stack) { return NULL; }
u1* used_stack_top (void) { return NULL; }

java_objectheader *native_new_and_init (void *p) { return NULL; }

/************************ globale Variablen **********************/

java_objectheader *exceptionptr;                       /* schani */
int  newcompiler = true;
bool verbose =  false;

static chain *nativechain;
static FILE *file = NULL;

static void printIDpart (int c) 
{
		if (     (c>='a' && c<='z')
		      || (c>='A' && c<='Z')
		      || (c>='0' && c<='9')
		      || (c=='_') )          
		           putc (c,file);
        else       putc ('_',file);

}

static void printID (unicode *name)
{
	int i;
	for (i=0; i<name->length; i++) {
		printIDpart (name->text[i]);
    	}
}


u4 outputsize;
bool dopadding;

static void addoutputsize (int len)
{
	u4 newsize,i;
	if (!dopadding) return;

	newsize = ALIGN (outputsize, len);
	
	for (i=outputsize; i<newsize; i++) fprintf (file, "   u1 pad%d\n",(int) i);
	outputsize = newsize;
}


static u2 *printtype (u2 *desc)
{
	u2 c;

	switch (*(desc++)) {
		case 'V': fprintf (file, "void");
		          break;
		case 'I':
		case 'S':
		case 'B':
		case 'C':
		case 'Z': addoutputsize (4);
                  fprintf (file, "s4");
                  break;
		case 'J': addoutputsize (8);
                  fprintf (file, "s8");
                  break;
		case 'F': addoutputsize (4);
                  fprintf (file, "float");
                  break;
		case 'D': addoutputsize (8);
                  fprintf (file, "double");
                  break;
		case '[':
			addoutputsize ( sizeof(java_arrayheader*) ); 
			switch (*(desc++)) {
				case 'I':  fprintf (file, "java_intarray*"); break;
				case 'J':  fprintf (file, "java_longarray*"); break;
				case 'Z':  fprintf (file, "java_booleanarray*"); break;
				case 'B':  fprintf (file, "java_bytearray*"); break;
				case 'S':  fprintf (file, "java_shortarray*"); break;
				case 'C':  fprintf (file, "java_chararray*"); break;
				case 'F':  fprintf (file, "java_floatarray*"); break;
				case 'D':  fprintf (file, "java_doublearray*"); break;
				
				case '[':  fprintf (file, "java_arrayarray*");
				           while ((*desc) == '[') desc++;
				           if ((*desc)!='L') desc++;
				           else while (*(desc++) != ';');
                           break;
                           
				case 'L':  fprintf (file, "java_objectarray*");
				           while ( *(desc++) != ';');
				           break;
				default: panic ("invalid type descriptor");
				}
			break;
		
		case 'L': 
			addoutputsize ( sizeof(java_objectheader*));
            fprintf (file, "struct ");
            while ( (c = *(desc++)) != ';' ) printIDpart (c);   	 
            fprintf (file, "*");
			break;
					
		default:  panic ("Unknown type in field descriptor");
	}
	
	return (desc);
}



static void printfields (classinfo *c)
{
	u4 i;
	fieldinfo *f;
	
	if (!c) {
		addoutputsize ( sizeof(java_objectheader) );
		fprintf (file, "   java_objectheader header;\n");
		return;
		}
		
	printfields (c->super);
	
	for (i=0; i<c->fieldscount; i++) {
		f = &(c->fields[i]);
		
		if (! (f->flags & ACC_STATIC) ) {
			fprintf (file,"   ");
			printtype (f->descriptor->text);
			fprintf (file, " ");
			unicode_fprint (file, f->name);
			fprintf (file, ";\n");
			}
		}
}




static void remembermethods (classinfo *c)
{
	u2 i;
	methodinfo *m;

	for (i=0; i<c->methodscount; i++) {
		m = &(c->methods[i]);

		if (m->flags & ACC_NATIVE) {
			chain_addlast (nativechain, m);
			}
					
		}
}




static void printmethod (methodinfo *m)
{
	u2 *d;
	u2 paramnum=1;
	
	d = m->descriptor->text;
	while (*(d++) != ')');
				
	printtype (d);
	fprintf (file," ");
	printID (m->class->name);
	fprintf (file,"_");
	printID (m->name);
	fprintf (file," (");
					
	d = m->descriptor->text+1;
			
	if (! (m->flags & ACC_STATIC) ) {
		fprintf (file, "struct ");
		printID (m->class->name);
		fprintf (file, "* this");
		if ((*d)!=')') fprintf (file, ", ");
		}
			
	while ((*d)!=')') {
		d = printtype (d);
		fprintf (file, " par%d", paramnum++);
		if ((*d)!=')') fprintf (file, ", ");
		}
			
	fprintf (file, ");\n");
}


static void headers_generate (classinfo *c)
{
	fprintf (file, "/* Structure information for class: ");
	unicode_fprint (file, c->name);
	fprintf (file, " */\n\n");

	fprintf (file, "typedef struct ");
	printID (c->name);
	fprintf (file, " {\n");
	
	outputsize=0;
	dopadding=true;
	printfields (c);

	fprintf (file, "} ");
	printID (c->name);
	fprintf (file, ";\n\n");

	remembermethods (c);
	

	fprintf (file, "\n\n");
}



static void printnativetableentry (methodinfo *m)
{
	fprintf (file, "   { \"");
	unicode_fprint (file, m->class->name);
	fprintf (file, "\",\n     \"");
	unicode_fprint (file, m->name);
	fprintf (file, "\",\n     \"");
	unicode_fprint (file, m->descriptor);
	fprintf (file, "\",\n     ");
	if ( (m->flags & ACC_STATIC) !=0)  fprintf (file, "true");
	                              else fprintf (file, "false");
	fprintf (file, ",\n     ");
	fprintf (file, "(functionptr) ");
	printID (m->class->name);
	fprintf (file,"_");
	printID (m->name);
	fprintf (file,"\n   },\n");
}





static void headers_start ()
{
	file = fopen ("nativetypes.hh", "w");
	if (!file) panic ("Can not open file 'native.h' to store header information");
	
	fprintf (file, "/* Headerfile for native methods: nativetypes.hh */\n");
	fprintf (file, "/* This file is machine generated, don't edit it !*/\n\n"); 

	nativechain = chain_new ();
}


static void headers_finish ()
{
	methodinfo *m;
	
	fprintf (file, "\n/* Prototypes for native methods */\n\n");
	
	m = chain_first (nativechain);
	while (m) {
		dopadding=false;		
		printmethod (m);
		
		m = chain_next (nativechain);
		}


	file = fopen ("nativetable.hh", "w");
	if (!file) panic ("Can not open file 'nativetable' to store native-link-table");

	fprintf (file, "/* Table of native methods: nativetables.hh */\n");
	fprintf (file, "/* This file is machine generated, don't edit it !*/\n\n"); 

	while ( (m = chain_first (nativechain)) != NULL) {
		chain_remove (nativechain);
		
		printnativetableentry (m);
		
		}
		
	chain_free (nativechain);
	fclose (file);
}





/******************** interne Funktion: print_usage ************************

Gibt die richtige Aufrufsyntax des JAVA-Header-Generators auf stdout aus.

***************************************************************************/

static void print_usage()
{
	printf ("USAGE: jch class [class..]\n");
}   




/************************** Funktion: main *******************************

   Das Hauptprogramm.
   Wird vom System zu Programstart aufgerufen (eh klar).
   
**************************************************************************/

int main(int argc, char **argv)
{
	s4 i,a;
	char *cp;
	classinfo *topclass;
	void *dummy;
		

   /********** interne (nur fuer main relevante Optionen) **************/
   
	char classpath[500] = "";
	u4 heapsize = 100000;

   /*********** Optionen, damit wirklich nur headers generiert werden ***/
   
   makeinitializations=false;
   

   /************ Infos aus der Environment lesen ************************/

	cp = getenv ("CLASSPATH");
	if (cp) {
		strcpy (classpath + strlen(classpath), ":");
		strcpy (classpath + strlen(classpath), cp);
		}

	if (argc < 2) {
   		print_usage ();
   		exit(10);
   		}


   /**************************** Programmstart *****************************/

	log_init (NULL);
	log_text ("Java - header-generator started");
	
	
	suck_init (classpath);
	
	unicode_init ();
	heap_init (heapsize, heapsize, &dummy);
	loader_init ();


   /*********************** JAVA-Klassen laden  ***************************/
   
	headers_start ();

	
	for (a=1; a<argc; a++) {   
   		cp = argv[a];
   		for (i=strlen(cp)-1; i>=0; i--) {     /* Punkte im Klassennamen */
 	  		if (cp[i]=='.') cp[i]='/';        /* auf slashes umbauen */
  	 		}

		topclass = loader_load ( unicode_new_char (cp) );
		
		headers_generate (topclass);
		}
	

	headers_finish ();


   /************************ Freigeben aller Resourcen *******************/

	loader_close ();
	heap_close ();
	unicode_close (NULL);
	

   /* Endemeldung ausgeben und mit entsprechendem exit-Status terminieren */

	log_text ("Java - header-generator stopped");
	log_cputime ();
	mem_usagelog(1);
	
	return 0;
}


