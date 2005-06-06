/* src/cacaoh/headers.c - functions for header generation

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

   Authors: Reinhard Grafl

   Changes: Mark Probst
            Philipp Tomsich
            Christian Thalinger

   $Id: headers.c 2551 2005-06-06 14:44:56Z twisti $

*/


#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
# if defined(__DARWIN__)
#  include <signal.h>
# endif
# include <ucontext.h>
#endif

#include "mm/boehm.h"
#include "mm/memory.h"
#include "native/include/java_lang_String.h"
#include "native/include/java_lang_Throwable.h"
#include "toolbox/chain.h"
#include "toolbox/logging.h"
#include "vm/builtin.h"
#include "vm/class.h"
#include "vm/global.h"
#include "vm/method.h"
#include "vm/tables.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"


/* for raising exceptions from native methods */

#if !defined(USE_THREADS) || !defined(NATIVE_THREADS)
java_objectheader *_exceptionptr = NULL;
#endif


/******* replace some external functions  *********/
 
functionptr native_findfunction(utf *cname, utf *mname, utf *desc, bool isstatic)
{ return NULL; }

java_objectheader *native_new_and_init(classinfo *c) { return NULL; }
java_objectheader *native_new_and_init_string(classinfo *c, java_lang_String *s) { return NULL; }
java_objectheader *native_new_and_init_int(classinfo *c, s4 i) { return NULL; }
java_objectheader *native_new_and_init_throwable(classinfo *c, java_lang_Throwable *t) { return NULL; }


java_objectheader *asm_calljavafunction(methodinfo *m, void *arg1, void *arg2,
										void *arg3, void *arg4) { return NULL; }

void asm_call_jit_compiler(void) {}

/* code patching functions */
void patcher_builtin_new(u1 *sp) {}
void patcher_builtin_newarray(u1 *sp) {}
void patcher_builtin_arraycheckcast(u1 *sp) {}
void patcher_builtin_arrayinstanceof(u1 *sp) {}

s4 asm_builtin_idiv(s4 a, s4 b) { return 0; }
s4 asm_builtin_irem(s4 a, s4 b) { return 0; }
s8 asm_builtin_ldiv(s8 a, s8 b) { return 0; }
s8 asm_builtin_lrem(s8 a, s8 b) { return 0; }

s4 asm_builtin_f2i(float a) { return 0; }
s8 asm_builtin_f2l(float a) { return 0; }
s4 asm_builtin_d2i(double a) { return 0; }
s8 asm_builtin_d2l(double a) { return 0; }

void use_class_as_object(void) {}
void asm_builtin_monitorenter(java_objectheader *o) {}
void *asm_builtin_monitorexit(java_objectheader *o) { return NULL; }

s4 asm_builtin_arraycheckcast(java_objectheader *obj, vftbl_t *target)
{
	return 0;
}

#if defined(__MIPS__)
long compare_and_swap(long *p, long oldval, long newval)
{
	if (*p == oldval) {
		*p = newval;
		return oldval;
	} else
		return *p;
}
#endif


void asm_builtin_aastore(java_objectarray *a, s4 index, java_objectheader *o) {}

u1 *createcompilerstub(methodinfo *m) { return NULL; }
u1 *createnativestub(functionptr f, methodinfo *m) { return NULL; }
u1 *oldcreatenativestub(functionptr f, methodinfo *m) { return NULL; }

void removecompilerstub(u1 *stub) {}
void removenativestub(u1 *stub) {}

void asm_perform_threadswitch(u1 **from, u1 **to, u1 **stackTop) {}
u1* asm_initialize_thread_stack(void *func, u1 *stack) { return NULL; }

void *asm_switchstackandcall(void *stack, void *func, void **stacktopsave, void * p) { return NULL; }

void asm_handle_builtin_exception(classinfo *c) {}
void asm_getclassvalues_atomic(vftbl_t *super, vftbl_t *sub, castinfo *out) {}


void setVMClassField(classinfo *c) {}
void *Java_java_lang_VMObject_clone(void *env, void *clazz, void * this)
{
	return NULL;
}

tristate_t
typeinfo_is_assignable_to_class(typeinfo *value,classref_or_classinfo dest)
{
	return true;
}


/* exception functions ********************************************************/

/* these should not be called */

void throw_exception(void) { assert(0); }
void throw_exception_exit(void) { assert(0); }

java_objectheader *new_verifyerror(methodinfo *m, const char *message)
{
	assert(0);

	/* keep compiler happy */

	return NULL;
}

java_objectheader *new_exception_throwable(const char *classname, java_lang_Throwable *throwable)
{
	assert(0);

	/* keep compiler happy */

	return NULL;
}


void throw_cacao_exception_exit(const char *exception, const char *message, ...)
{
	va_list ap;

	fprintf(stderr, "%s: ", exception);

	va_start(ap, message);
	vfprintf(stderr, message, ap);
	va_end(ap);

	exit(1);
}


java_objectheader *new_exception(const char *classname)
{
	fprintf(stderr, "%s\n", classname);
	exit(1);

	/* keep compiler happy */

	return NULL;
}


java_objectheader *new_exception_message(const char *classname, const char *message)
{
	fprintf(stderr, "%s: %s\n", classname, message);
	exit(1);

	/* keep compiler happy */

	return NULL;
}


java_objectheader *new_exception_utfmessage(const char *classname, utf *message)
{
	fprintf(stderr, "%s: ", classname);
	utf_display(message);
	fputc('\n', stderr);

	exit(1);

	/* keep compiler happy */

	return NULL;
}


java_objectheader *new_classformaterror(classinfo *c, const char *message, ...)
{
	va_list ap;

	utf_display(c->name);
	fprintf(stderr, ": ");

	va_start(ap, message);
	vfprintf(stderr, message, ap);
	va_end(ap);

	fputc('\n', stderr);

	exit(1);

	/* keep compiler happy */

	return NULL;
}


java_objectheader *new_internalerror(const char *message, ...)
{
	va_list ap;

	fprintf(stderr, "%s: ", string_java_lang_InternalError);

	va_start(ap, message);
	vfprintf(stderr, message, ap);
	va_end(ap);

	exit(1);

	/* keep compiler happy */

	return NULL;
}


java_objectheader *new_unsupportedclassversionerror(classinfo *c, const char *message, ...)
{
	va_list ap;

	fprintf(stderr, "%s: ", string_java_lang_UnsupportedClassVersionError);

	utf_display(c->name);
	fprintf(stderr, ": ");

	va_start(ap, message);
	vfprintf(stderr, message, ap);
	va_end(ap);

	exit(1);

	/* keep compiler happy */

	return NULL;
}


java_objectheader *new_negativearraysizeexception(void)
{
	fprintf(stderr, "%s", string_java_lang_NegativeArraySizeException);
	exit(1);

	/* keep compiler happy */

	return NULL;
}


java_objectheader *new_nullpointerexception(void)
{
	fprintf(stderr, "%s", string_java_lang_NullPointerException);
	exit(1);

	/* keep compiler happy */

	return NULL;
}


/* machine dependent stuff ****************************************************/

#if defined(__DARWIN__)
int cacao_catch_Handler(void) {}
#endif

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
threadcritnode asm_criticalsections;
void thread_restartcriticalsection(ucontext_t *uc) {}
#endif

void md_param_alloc(methoddesc *md) {}


/************************ global variables **********************/

chain *nativemethod_chain;              /* chain with native methods          */
chain *nativeclass_chain;               /* chain with processed classes       */
chain *ident_chain;     /* chain with method and field names in current class */
FILE *file = NULL;
static u4 outputsize;
static bool dopadding;


static void printIDpart(int c)
{
	if ((c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		(c >= '0' && c <= '9') ||
		(c == '_'))
		putc(c, file);
	else
		putc('_', file);
}


void printID(utf *u)
{
	char *utf_ptr = u->text;
	int i;

	for (i = 0; i < utf_strlen(u); i++) 
		printIDpart(utf_nextu2(&utf_ptr));
}


static void addoutputsize (int len)
{
	u4 newsize,i;
	if (!dopadding) return;

	newsize = ALIGN(outputsize, len);
	
	for (i = outputsize; i < newsize; i++) fprintf(file, "   u1 pad%d\n", (int) i);
	outputsize = newsize;
}


void printOverloadPart(utf *desc)
{
	char *utf_ptr=desc->text;
	u2 c;

	fprintf(file, "__");

	while ((c = utf_nextu2(&utf_ptr)) != ')') {
		switch (c) {
		case 'I':
		case 'S':
		case 'B':
		case 'C':
		case 'Z':
		case 'J':
		case 'F':
		case 'D': 
			fprintf(file, "%c", (char) c);
			break;
		case '[':
			fprintf(file, "_3");
			break;
		case 'L':
			putc('L', file);
			while ((c = utf_nextu2(&utf_ptr)) != ';')
				printIDpart(c);
			fprintf(file, "_2");
			break;
		case '(':
			break;
		default: 
			log_text("invalid method descriptor");
			assert(0);
		}
	}
}

static char *printtype(char *utf_ptr)
{
	u2 c;

	switch (utf_nextu2(&utf_ptr)) {
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
		switch (utf_nextu2(&utf_ptr)) {
		case 'I':  fprintf (file, "java_intarray*"); break;
		case 'J':  fprintf (file, "java_longarray*"); break;
		case 'Z':  fprintf (file, "java_booleanarray*"); break;
		case 'B':  fprintf (file, "java_bytearray*"); break;
		case 'S':  fprintf (file, "java_shortarray*"); break;
		case 'C':  fprintf (file, "java_chararray*"); break;
		case 'F':  fprintf (file, "java_floatarray*"); break;
		case 'D':  fprintf (file, "java_doublearray*"); break;
				
		case '[': fprintf(file, "java_objectarray*");
			while ((c = utf_nextu2(&utf_ptr)) == '[');
			if (c == 'L')
				while (utf_nextu2(&utf_ptr) != ';');
			break;
                           
		case 'L':  fprintf(file, "java_objectarray*");
			while (utf_nextu2(&utf_ptr) != ';');
			break;
		default:
			log_text("invalid type descriptor");
			assert(0);
		}
		break;
		
	case 'L': 
		addoutputsize ( sizeof(java_objectheader*));
		fprintf (file, "struct ");
		while ( (c = utf_nextu2(&utf_ptr)) != ';' ) printIDpart (c);   	 
		fprintf (file, "*");
		break;
					
	default:
		log_text("Unknown type in field descriptor");
		assert(0);
	}
	
	return utf_ptr;
}


/***** determine the number of entries of a utf string in the ident chain *****/

static int searchidentchain_utf(utf *ident) 
{
	utf *u = chain_first(ident_chain);     /* first element of list */
	int count = 0;

	while (u) {
		if (u==ident) count++;         /* string found */
		u = chain_next(ident_chain);   /* next element in list */ 
	}

	return count;
}


/************** print structure for direct access to objects ******************/

static void printfields(classinfo *c)
{
	u4 i;
	fieldinfo *f;
	int ident_count;
	
	if (!c) {
		addoutputsize(sizeof(java_objectheader));
		fprintf(file, "   java_objectheader header;\n");
		return;
	}
		
	printfields(c->super.cls);
	
	for (i = 0; i < c->fieldscount; i++) {
		f = &(c->fields[i]);
		
		if (!(f->flags & ACC_STATIC)) {
			fprintf(file, "   ");
			printtype(f->descriptor->text);
			fprintf(file, " ");
			utf_fprint(file, f->name);

			/* rename multiple fieldnames */
			if ((ident_count = searchidentchain_utf(f->name)))
				fprintf(file, "%d", ident_count - 1);
			chain_addlast(ident_chain, f->name);	

			fprintf(file, ";\n");
		}
	}
}


/***************** store prototype for native method in file ******************/

void printmethod(methodinfo *m)
{
	char *utf_ptr;
	u2 paramnum = 1;

	/* search for return-type in descriptor */	
	utf_ptr = m->descriptor->text;
	while (utf_nextu2(&utf_ptr) != ')');

	/* create remarks */
	fprintf(file, "\n/*\n * Class:     ");
	utf_fprint(file, m->class->name);
	fprintf(file, "\n * Method:    ");
	utf_fprint(file, m->name);
	fprintf(file, "\n * Signature: ");
	utf_fprint(file, m->descriptor);
	fprintf(file, "\n */\n");

	/* create prototype */ 			
	fprintf(file, "JNIEXPORT ");
	printtype(utf_ptr);
	fprintf(file, " JNICALL Java_");
	printID(m->class->name);

	chain_addlast(ident_chain, m->name);

	fprintf(file, "_");
	printID(m->name);
	if (m->nativelyoverloaded) printOverloadPart(m->descriptor);
	fprintf(file, "(JNIEnv *env");
	
	utf_ptr = m->descriptor->text + 1;
			
	if (!(m->flags & ACC_STATIC)) {
		fprintf(file, ", struct ");
		printID(m->class->name);
		fprintf(file, "* this");

	} else {
		fprintf(file, ", jclass clazz");
	}

	if ((*utf_ptr) != ')') fprintf(file, ", ");
			
	while ((*utf_ptr) != ')') {
		utf_ptr = printtype(utf_ptr);
		fprintf(file, " par%d", paramnum++);
		if ((*utf_ptr)!=')') fprintf(file, ", ");
	}
			
	fprintf(file, ");\n\n");
}


/******* remove package-name in fully-qualified classname *********************/

void gen_header_filename(char *buffer, utf *u)
{
	s4 i;
  
	for (i = 0; i < utf_strlen(u); i++) {
		if ((u->text[i] == '/') || (u->text[i] == '$')) {
			buffer[i] = '_';  /* convert '$' and '/' to '_' */

		} else {
			buffer[i] = u->text[i];
		}
	}
	buffer[utf_strlen(u)] = '\0';
}


/* create headerfile for classes and store native methods in chain ************/

void headerfile_generate(classinfo *c, char *opt_directory)
{
	char header_filename[1024] = "";
	char classname[1024]; 
	char uclassname[1024];
	u2 i;
	methodinfo *m;	      		
	u2 i2;
	methodinfo *m2;
	u2 nativelyoverloaded;	      		
		      
	/* store class in chain */		      
	chain_addlast(nativeclass_chain, c);
			    	
	/* open headerfile for class */
	gen_header_filename(classname, c->name);

	/* create chain for renaming fields */
	ident_chain = chain_new();
	
	if (opt_directory) {
		sprintf(header_filename, "%s/%s.h", opt_directory, classname);

	} else {
		sprintf(header_filename, "%s.h", classname);
	}

   	file = fopen(header_filename, "w");
   	if (!file) {
		log_text("Can not open file to store header information");
		assert(0);
	}

   	fprintf(file, "/* This file is machine generated, don't edit it! */\n\n");

	/* convert to uppercase */
	for (i = 0; classname[i]; i++) {
		uclassname[i] = toupper(classname[i]);
	}
	uclassname[i] = '\0';

	fprintf(file, "#ifndef _%s_H\n#define _%s_H\n\n", uclassname, uclassname);

	/* create structure for direct access to objects */	
	fprintf(file, "/* Structure information for class: ");
	utf_fprint(file, c->name);
	fprintf(file, " */\n\n");
	fprintf(file, "typedef struct ");
	printID(c->name);							
	fprintf(file, " {\n");
	outputsize = 0;
	dopadding = true;

	printfields(c);

	fprintf(file, "} ");
	printID(c->name);
	fprintf(file, ";\n\n");

	/* create chain for renaming overloaded methods */
	chain_free(ident_chain);
	ident_chain = chain_new();

	/* create method-prototypes */
		      		
	/* find overloaded methods */
	for (i = 0; i < c->methodscount; i++) {

		m = &(c->methods[i]);

		if (!(m->flags & ACC_NATIVE)) continue;
		if (!m->nativelyoverloaded) {
			nativelyoverloaded=false;
			for (i2=i+1;i2<c->methodscount; i2++) {
				m2 = &(c->methods[i2]);
				if (!(m2->flags & ACC_NATIVE)) continue;
				if (m->name==m2->name) {
					m2->nativelyoverloaded=true;
					nativelyoverloaded=true;
				}
			}
			m->nativelyoverloaded=nativelyoverloaded;
		}

	}

	for (i = 0; i < c->methodscount; i++) {

		m = &(c->methods[i]);

		if (m->flags & ACC_NATIVE) {
			chain_addlast(nativemethod_chain, m);
			printmethod(m);
		}
	}

	chain_free(ident_chain);

	fprintf(file, "#endif\n\n");

   	fclose(file);
}


/******** print classname, '$' used to seperate inner-class name ***********/

void print_classname(classinfo *clazz)
{
	utf *u = clazz->name;
    char *endpos  = u->text + u->blength;
    char *utf_ptr = u->text; 
	u2 c;

    while (utf_ptr < endpos) {
		if ((c = utf_nextu2(&utf_ptr)) == '_')
			putc('$', file);
		else
			putc(c, file);
	}
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
