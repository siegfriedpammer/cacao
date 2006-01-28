/* src/cacaoh/headers.c - functions for header generation

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Contact: cacao@cacaojvm.org

   Authors: Reinhard Grafl

   Changes: Mark Probst
            Philipp Tomsich
            Christian Thalinger

   $Id: headers.c 4381 2006-01-28 14:18:06Z twisti $

*/


#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "vm/types.h"

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
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"


#if defined(ENABLE_INTRP)
/* dummy interpreter stack to keep the compiler happy */

u1 intrp_main_stack[1];
#endif


/* for raising exceptions from native methods *********************************/

#if !defined(USE_THREADS) || !defined(NATIVE_THREADS)
java_objectheader *_no_threads_exceptionptr = NULL;
#endif


/* replace some non-vmcore functions ******************************************/

functionptr native_findfunction(utf *cname, utf *mname, utf *desc,
								bool isstatic)
{
	/* return something different than NULL, otherwise we get an exception */

	return (functionptr) 1;
}

java_objectheader *native_new_and_init(classinfo *c) { return NULL; }
java_objectheader *native_new_and_init_string(classinfo *c, java_lang_String *s) { return NULL; }
java_objectheader *native_new_and_init_int(classinfo *c, s4 i) { return NULL; }
java_objectheader *native_new_and_init_throwable(classinfo *c, java_lang_Throwable *t) { return NULL; }


#if defined(ENABLE_JIT)
java_objectheader *asm_calljavafunction(methodinfo *m,
										void *arg1, void *arg2,
										void *arg3, void *arg4)
{ return NULL; }
#endif

#if defined(ENABLE_INTRP)
java_objectheader *intrp_asm_calljavafunction(methodinfo *m,
											  void *arg1, void *arg2,
											  void *arg3, void *arg4)
{ return NULL; }
#endif


/* code patching functions */
void patcher_builtin_arraycheckcast(u1 *sp) {}

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


u1 *createcompilerstub(methodinfo *m) { return NULL; }
#if defined(ENABLE_INTRP)
u1 *intrp_createcompilerstub(methodinfo *m) { return NULL; }
#endif

u1 *codegen_createnativestub(functionptr f, methodinfo *m) { return NULL; }

void removecompilerstub(u1 *stub) {}
void removenativestub(u1 *stub) {}

void asm_perform_threadswitch(u1 **from, u1 **to, u1 **stackTop) {}
u1* asm_initialize_thread_stack(void *func, u1 *stack) { return NULL; }

void *asm_switchstackandcall(void *stack, void *func, void **stacktopsave, void * p) { return NULL; }

void asm_handle_builtin_exception(classinfo *c) {}


#if defined(ENABLE_JIT)
void asm_getclassvalues_atomic(vftbl_t *super, vftbl_t *sub, castinfo *out) {}
#endif

#if defined(ENABLE_INTRP)
void intrp_asm_getclassvalues_atomic(vftbl_t *super, vftbl_t *sub, castinfo *out) {}
#endif


void *Java_java_lang_VMObject_clone(void *env, void *clazz, void * this)
{
	return NULL;
}

typecheck_result typeinfo_is_assignable_to_class(typeinfo *value,classref_or_classinfo dest)
{
	return typecheck_TRUE;
}

void typeinfo_init_classinfo(typeinfo *info,classinfo *c)
{
}

bool typeinfo_init_class(typeinfo *info,classref_or_classinfo c)
{
	return true;
}

void typeinfo_print(FILE *file,typeinfo *info,int indent) {}

void exceptions_print_exception(java_objectheader *xptr) {}
void stacktrace_print_trace(java_objectheader *xptr) {}


/* exception functions ********************************************************/

/* these should not be called */

void throw_main_exception_exit(void) { assert(0); }
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

	fputc('\n', stderr);

	exit(1);
}


void exceptions_throw_outofmemory_exit(void)
{
	fprintf(stderr, "java.lang.InternalError: Out of memory\n");
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


java_objectheader *new_exception_javastring(const char *classname,
											java_lang_String *message)
{
	fprintf(stderr, "%s: ", classname);
	/* TODO print message */
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


void exceptions_throw_classformaterror(classinfo *c, const char *message, ...)
{
	va_list ap;

	va_start(ap, message);
	(void) new_classformaterror(c, message, ap);
	va_end(ap);
}


java_objectheader *new_classnotfoundexception(utf *name)
{
	fprintf(stderr, "java.lang.ClassNotFoundException: ");
	utf_fprint(stderr, name);
	fputc('\n', stderr);

	exit(1);

	/* keep compiler happy */

	return NULL;
}


java_objectheader *new_noclassdeffounderror(utf *name)
{
	fprintf(stderr, "java.lang.NoClassDefFoundError: ");
	utf_fprint(stderr, name);
	fputc('\n', stderr);

	exit(1);

	/* keep compiler happy */

	return NULL;
}


java_objectheader *exceptions_new_linkageerror(const char *message,
											   classinfo *c)
{
	fprintf(stderr, "java.lang.LinkageError: %s",message);
	if (c) {
		utf_fprint_classname(stderr, c->name);
	}
	fputc('\n', stderr);

	exit(1);

	/* keep compiler happy */

	return NULL;
}

java_objectheader *exceptions_new_nosuchmethoderror(classinfo *c,
													utf *name, utf *desc)
{
	fprintf(stderr, "java.lang.NoSuchMethodError: ");
	utf_fprint(stderr, c->name);
	fprintf(stderr, ".");
	utf_fprint(stderr, name);
	utf_fprint(stderr, desc);
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


java_objectheader *new_illegalmonitorstateexception(void)
{
	fprintf(stderr, "%s", string_java_lang_IllegalMonitorStateException);
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


void exceptions_throw_negativearraysizeexception(void)
{
	(void) new_negativearraysizeexception();
}


java_objectheader *new_nullpointerexception(void)
{
	fprintf(stderr, "%s", string_java_lang_NullPointerException);
	exit(1);

	/* keep compiler happy */

	return NULL;
}


void exceptions_throw_nullpointerexception(void)
{
	(void) new_nullpointerexception();
}


void classnotfoundexception_to_noclassdeffounderror(void)
{
}

/* machine dependent stuff ****************************************************/

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
threadcritnode asm_criticalsections;
void thread_restartcriticalsection(ucontext_t *uc) {}
#endif

void md_param_alloc(methoddesc *md) {}


#if defined(ENABLE_INTRP)
void print_dynamic_super_statistics(void) {}
#endif


/************************ global variables **********************/

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

	/* ATTENTION: We use the methodinfo's isleafmethod variable as
	   nativelyoverloaded, so we can save some space during
	   runtime. */

	if (m->isleafmethod)
		printOverloadPart(m->descriptor);

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
	u2 j;
	methodinfo *m2;
	bool nativelyoverloaded;	      		
		      
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

		if (!(m->flags & ACC_NATIVE))
			continue;

		/* We use the methodinfo's isleafmethod variable as
		   nativelyoverloaded, so we can save some space during
		   runtime. */

		if (!m->isleafmethod) {
			nativelyoverloaded = false;

			for (j = i + 1; j < c->methodscount; j++) {
				m2 = &(c->methods[j]);

				if (!(m2->flags & ACC_NATIVE))
					continue;

				if (m->name == m2->name) {
					m2->isleafmethod = true;
					nativelyoverloaded = true;
				}
			}
		}

		m->isleafmethod = nativelyoverloaded;
	}

	for (i = 0; i < c->methodscount; i++) {
		m = &(c->methods[i]);

		if (m->flags & ACC_NATIVE)
			printmethod(m);
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
