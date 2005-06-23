/* src/vm/jit/stacktrace.c

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

   Authors: Joseph Wenninger

   Changes: Christian Thalinger

   $Id: stacktrace.c 2799 2005-06-23 09:54:26Z twisti $

*/


#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "asmoffsets.h"
#include "mm/boehm.h"
#include "native/native.h"
#include "vm/global.h"                   /* required here for native includes */
#include "native/include/java_lang_ClassLoader.h"
#include "toolbox/logging.h"
#include "vm/builtin.h"
#include "vm/class.h"
#include "vm/tables.h"
#include "vm/jit/codegen.inc.h"
#include "vm/loader.h"
#include "vm/stringlocal.h"
#include "vm/exceptions.h"

#undef JWDEBUG
#undef JWDEBUG2
#undef JWDEBUG3

/*JoWenn: simplify collectors (trace doesn't contain internal methods)*/

/* the line number is only u2, but to avoid alignment problems it is made the same size as a native
	pointer. In the structures where this is used, values of -1 or -2 have a special meainging, so
	if java bytecode is ever extended to support more than 65535 lines/file, this could will have to
	be changed.*/

#if defined(_ALPHA_) || defined(__X86_64__)
	#define LineNumber u8
#else
	#define LineNumber u4
#endif

typedef struct lineNumberTableEntry {
/* The special value of -1 means that a inlined function starts, a value of -2 means that an inlined function ends*/
	LineNumber lineNr;
	u1 *pc;
} lineNumberTableEntry;

typedef struct lineNumberTableEntryInlineBegin {
/*this should have the same layout and size as the lineNumberTableEntry*/
	LineNumber lineNrOuter;
	methodinfo *method;
} lineNumberTableEntryInlineBegin;


typedef void(*CacaoStackTraceCollector)(void **,stackTraceBuffer*);

#define BLOCK_INITIALSIZE 40
#define BLOCK_SIZEINCREMENT 40

static void addEntry(stackTraceBuffer* buffer,methodinfo*method ,LineNumber line) {
	if (buffer->size>buffer->full) {
		stacktraceelement *tmp=&(buffer->start[buffer->full]);
		tmp->method=method;
		tmp->linenumber=line;
		buffer->full = buffer->full + 1;
#if (defined(JWDEBUG) || defined (JWDEBUG2))
		log_text("addEntry (stacktrace):");
		printf("method %p\n",method);
		if (method) printf("method->name %p\n",method->name);
		if (method) utf_display(method->name); else printf("Native");
		if (method) {printf("\n");utf_display(method->class->name);}
		printf("\nnext buffer item %d\nLine:%ld\n",buffer->full,line);
#endif
	} else {
		stacktraceelement *newBuffer;

#ifdef JWDEBUG
		log_text("stacktrace buffer full, resizing");
#endif

		newBuffer =
			(stacktraceelement *) malloc((buffer->size + BLOCK_SIZEINCREMENT) *
										 sizeof(stacktraceelement));

		if (newBuffer==0) {
			log_text("OOM during stacktrace creation");
			assert(0);
		}

		memcpy(newBuffer,buffer->start,buffer->size*sizeof(stacktraceelement));
		if (buffer->needsFree) free(buffer->start);
		buffer->start=newBuffer;
		buffer->size=buffer->size+BLOCK_SIZEINCREMENT;
		buffer->needsFree=1;
		addEntry(buffer,method,line);
	}
}

static int fillInStackTrace_methodRecursive(stackTraceBuffer *buffer,methodinfo 
		*method,lineNumberTableEntry *startEntry, lineNumberTableEntry **entry, size_t *entriesAhead,u1 *adress) {

	size_t ahead=*entriesAhead;
	lineNumberTableEntry *ent=*entry;
	lineNumberTableEntryInlineBegin *ilStart;

	for (;ahead>0;ahead--,ent++) {
		if (adress>=ent->pc) {
			switch (ent->lineNr) {
				case -1: /*begin of inlined method */
					ilStart=(lineNumberTableEntryInlineBegin*)(++ent);
					ent ++;
					ahead--; ahead--;
					if (fillInStackTrace_methodRecursive(buffer,ilStart->method,ent,&ent,&ahead,adress)) {
						addEntry(buffer,method,ilStart->lineNrOuter);
						return 1;
					}
					break;
				case -2: /*end of inlined method*/
					*entry=ent;
					*entriesAhead=ahead;
					return 0;
					break;
				default:
					if (adress==ent->pc) {
						addEntry(buffer,method,ent->lineNr);
						return 1;
					}
					break;
			}
		} else {
			if (adress>startEntry->pc) {
				ent--;
				addEntry(buffer,method,ent->lineNr);
				return 1;	
			} else {
#ifdef JWDEBUG
				printf("trace point: %p\n",adress);
#endif
				log_text("trace point before method");
				assert(0);
			}
		}
	}
	ent--;
	addEntry(buffer,method,ent->lineNr);
	return 1;
	
}

static void fillInStackTrace_method(stackTraceBuffer *buffer,methodinfo *method,u1 *dataSeg, u1* adress) {
	size_t lineNumberTableSize=(*((size_t*)(dataSeg+LineNumberTableSize)));


	if ( lineNumberTableSize == 0) {
		/*right now this happens only on 
		i386,if the native stub causes an exception in a <clinit> invocation (jowenn)*/
		addEntry(buffer,method,0);
		return;
	} else {
		lineNumberTableEntry *ent; /*=(lineNumberTableEntry*) ((*((char**)(dataSeg+LineNumberTableStart))) - (sizeof(lineNumberTableEntry)-sizeof(void*)));*/
		void **calc;
		lineNumberTableEntry *startEntry;

		/*		printf("dataSeg: %p\n",dataSeg);*/
		calc=(void**)(dataSeg+LineNumberTableStart);
		/*		printf("position of line number table start reference in data segment: %p\n",calc);
				printf("line number table start as found in table: %p\n",*calc);*/
		ent=(lineNumberTableEntry *) (((char*)(*calc) - (sizeof(lineNumberTableEntry)-sizeof(void*))));
		/*		printf("line number table start as calculated: %p\n",ent);*/
		ent-=(lineNumberTableSize-1);
		startEntry=ent;
		/*		printf("line number table real start (bottom end) as calculated(2): %p\n",startEntry);*/

		if (!fillInStackTrace_methodRecursive(buffer,method,startEntry,&ent,&lineNumberTableSize,adress)) {
			log_text("Trace point not found in suspected method");
			assert(0);
		}
	}
}


typedef union {
	u1*         u1ptr;
	functionptr funcptr;
	void*       voidptr;
} u1ptr_functionptr_union;

#define cast_funcptr_u1ptr(dest,src) \
	{ u1f.funcptr=src;\
	dest=u1f.u1ptr; }	

#define cast_u1ptr_funcptr(dest,src) \
	{ u1f.u1ptr=src;\
	dest=u1f.funcptr; }	

void  cacao_stacktrace_fillInStackTrace(void **target,CacaoStackTraceCollector coll)
{

	stacktraceelement primaryBlock[BLOCK_INITIALSIZE*sizeof(stacktraceelement)]; 
		/*In most cases this should be enough -> one malloc less. I don't think temporary data should be
		allocated with the GC, only the result*/
	stackTraceBuffer buffer;
	buffer.needsFree=0;
	buffer.start=primaryBlock;
	buffer.size=BLOCK_INITIALSIZE; /*  *sizeof(stacktraceelement); */
	buffer.full=0;
#ifdef JWDEBUG
	log_text("entering cacao_stacktrace_fillInStacktrace");
	{
		int i=0;
		struct native_stackframeinfo *tmpinfo;
		for (tmpinfo=(*(((void**)(builtin_asm_get_stackframeinfo()))));tmpinfo;tmpinfo=tmpinfo->oldThreadspecificHeadValue)
			i++;
		printf("native function depth:%d\n",i);
		
	}
#endif
	{
		native_stackframeinfo *info;

		info = *((void **) (builtin_asm_get_stackframeinfo()));

		if (!info) {
			log_text("info ==0");
			*target=0;
			return;

		} else {
			u1 *tmp;
			u1 *dataseg; /*make it byte addressable*/
			methodinfo *currentMethod;
			functionptr returnAdress;
			u1* stackPtr;
			u1ptr_functionptr_union u1f;

/*			utf_display(info->method->class->name);
			utf_display(info->method->name);*/

			currentMethod = NULL;

			while (currentMethod || info) {
				/* some builtin native */

				if (currentMethod == NULL) {
					currentMethod = info->method;
					returnAdress = (functionptr) info->returnToFromNative;
#ifdef JWDEBUG
					log_text("native");
					printf("return to %p\n",returnAdress);
#endif
					if (currentMethod) {
#ifdef JWDEBUG
						log_text("real native (not an internal helper)\n");
						printf("returnaddress %p, methodpointer %p, stackframe begin %p\n",info->returnToFromNative,info->method, info->beginOfJavaStackframe);
#if 0
						utf_display(currentMethod->class->name);
						utf_display(currentMethod->name);
#endif
#endif

						addEntry(&buffer, currentMethod, 0);
					}
#if defined(__ALPHA__)
					if (info->savedpv!=0)
						dataseg=info->savedpv;
					else
						dataseg=codegen_findmethod(returnAdress);
#elif defined(__I386__) || defined (__X86_64__)
					cast_funcptr_u1ptr(dataseg,codegen_findmethod(returnAdress));
#endif
					currentMethod=(*((methodinfo**)(dataseg+MethodPointer)));
					if (info->beginOfJavaStackframe==0)
						stackPtr=((u1*)info)+sizeof(native_stackframeinfo);
					else
#if defined(__ALPHA__)
						stackPtr=(u1*)(info->beginOfJavaStackframe);
#elif defined(__I386__) || defined (__X86_64__)
						stackPtr=(u1*)(info->beginOfJavaStackframe)+sizeof(void*);
#endif
					info=info->oldThreadspecificHeadValue;
				} else { /*method created by jit*/
					u4 frameSize;
#ifdef JWDEBUG
					log_text("JIT");
#endif
#if defined (__ALPHA__)
					if (currentMethod->isleafmethod) {
#ifdef JWDEBUG
						printf("class.method:%s.%s\n",currentMethod->class->name->text,currentMethod->name->text);
#endif
						log_text("How could that happen ??? A leaf method in the middle of a stacktrace ??");
						assert(0);
					}
#endif
					/*utf_display(currentMethod->class->name);
					utf_display(currentMethod->name);*/
					cast_funcptr_u1ptr(tmp,returnAdress);
					fillInStackTrace_method(&buffer,currentMethod,dataseg,tmp-1);
					frameSize=*((u4*)(dataseg+FrameSize));
#if defined(__ALPHA__)
					/* cacao saves the return adress as the first element of the stack frame on alphas*/
					cast_funcptr_u1ptr (dataseg,codegen_findmethod(*((void**)(stackPtr+frameSize-sizeof(void*)))));
					returnAdress=(*((void**)(stackPtr+frameSize-sizeof(void*))));
#elif defined(__I386__) || defined (__X86_64__)
					/* on i386 the return adress is the first element before the stack frme*/
					cast_u1ptr_funcptr(returnAdress,*((u1**)(stackPtr+frameSize)));
					cast_funcptr_u1ptr(dataseg,codegen_findmethod(returnAdress));
#endif
/*					printf ("threadrootmethod %p\n",builtin_asm_get_threadrootmethod());
					if (currentMethod==builtin_asm_get_threadrootmethod()) break;*/
					currentMethod=(*((methodinfo**)(dataseg+MethodPointer)));
#if defined(__ALPHA__)
					stackPtr+=frameSize;
#elif defined(__I386__) || defined (__X86_64__)
					stackPtr+=frameSize+sizeof(void*);
#endif
				}
			}
			
			if (coll) coll(target,&buffer);
			if (buffer.needsFree) free(buffer.start);
#ifdef JWDEBUG
			log_text("leaving cacao_stacktrace_fillInStacktrace");
#endif

			return;
		}
		/*log_text("\n=========================================================");*/
	}
	*target=0;
#ifdef JWDEBUG
		log_text("leaving(2) cacao_stacktrace_fillInStacktrace");
#endif

}


static
void stackTraceCollector(void **target, stackTraceBuffer *buffer) {
	stackTraceBuffer *dest=*target=heap_allocate(sizeof(stackTraceBuffer)+buffer->full*sizeof(stacktraceelement),true,0);
	memcpy(*target,buffer,sizeof(stackTraceBuffer));
	memcpy(dest+1,buffer->start,buffer->full*sizeof(stacktraceelement));

	dest->needsFree=0;
	dest->size=dest->full;
	dest->start=(stacktraceelement*)(dest+1);

	/*
	if (buffer->full>0) {
		printf("SOURCE BUFFER:%s\n",buffer->start[0].method->name->text);
		printf("DEST BUFFER:%s\n",dest->start[0].method->name->text);
	} else printf("Buffer is empty\n");
	*/
}


void  cacao_stacktrace_NormalTrace(void **target) {
	cacao_stacktrace_fillInStackTrace(target,&stackTraceCollector);
}



static void classContextCollector(void **target, stackTraceBuffer *buffer)
{
	java_objectarray  *tmpArray;
	stacktraceelement *current;
	stacktraceelement *start;
	size_t size;
	size_t targetSize;
	s4 i;

	size = buffer->full;
	targetSize = 0;

	for (i = 0; i < size; i++)
		if (buffer->start[i].method != 0)
			targetSize++;

	start = buffer->start;
	start++;
	targetSize--;

	if (targetSize > 0) {
		if (start->method && (start->method->class == class_java_lang_SecurityManager)) {
			targetSize--;
			start++;
		}
	}

	tmpArray = builtin_anewarray(targetSize, class_java_lang_Class);

	for(i = 0, current = start; i < targetSize; i++, current++) {
		/* XXX TWISTI: should we use this skipping for native stubs? */

		if (!current->method) {
			i--;
			continue;
		}

		use_class_as_object(current->method->class);

		tmpArray->data[i] = (java_objectheader *) current->method->class;
	}

	*target = tmpArray;
}



java_objectarray *cacao_createClassContextArray() {
	java_objectarray *array=0;
	cacao_stacktrace_fillInStackTrace((void**)&array,&classContextCollector);
	return array;
	
}


static void classLoaderCollector(void **target, stackTraceBuffer *buffer)
{
	stacktraceelement *current;
	stacktraceelement *start;
	methodinfo        *m;
	ptrint             size;
	s4                 i;

	size = buffer->full;
	start = &(buffer->start[0]);

	for(i = 0, current = start; i < size; i++, current++) {
		m = current->method;

		if (!m)
			continue;

		if (m->class == class_java_security_PrivilegedAction) {
			*target = NULL;
			return;
		}

		if (m->class->classloader) {
			*target = (java_lang_ClassLoader *) m->class->classloader;
			return;
		}
	}

	*target = NULL;
}


java_objectheader *cacao_currentClassLoader() {
	java_objectheader *header=0;
	cacao_stacktrace_fillInStackTrace((void**)&header,&classLoaderCollector);
	return header;
}


static
void callingMethodCollector(void **target, stackTraceBuffer *buffer) {	
        if (buffer->full >2) (*target)=buffer->start[2].method;
	else (*target=0);
}

methodinfo *cacao_callingMethod() {
	methodinfo *method;
	cacao_stacktrace_fillInStackTrace((void**)&method,&callingMethodCollector);
	return method;
}


static
void getStackCollector(void **target, stackTraceBuffer *buffer)
{
	java_objectarray *classes;
	java_objectarray *methodnames;
	java_objectarray **result=(java_objectarray**)target;
	java_lang_String *str;
	classinfo *c;
	stacktraceelement *current;
	int i,size;

	/*log_text("getStackCollector");*/

	size = buffer->full;

	*result = builtin_anewarray(2, arrayclass_java_lang_Object);

	if (!(*result))
		return;

	classes = builtin_anewarray(size, class_java_lang_Class);

	if (!classes)
		return;

	methodnames = builtin_anewarray(size, class_java_lang_String);

	if (!methodnames)
		return;

	(*result)->data[0] = (java_objectheader *) classes;
	(*result)->data[1] = (java_objectheader *) methodnames;

	/*log_text("Before for loop");*/
	for (i = 0, current = &(buffer->start[0]); i < size; i++, current++) {
		/*log_text("In loop");*/
		c = current->method->class;
		use_class_as_object(c);
		classes->data[i] = (java_objectheader *) c;
		str = javastring_new(current->method->name);
		if (!str)
			return;
		methodnames->data[i] = (java_objectheader *) str;
		/*printf("getStackCollector: %s.%s\n",c->name->text,current->method->name->text);*/
	}

	/*if (*exceptionptr) panic("Exception in getStackCollector");*/

	/*log_text("loop left");*/
        return;

}


java_objectarray *cacao_getStackForVMAccessController()
{
	java_objectarray *result=0;
	cacao_stacktrace_fillInStackTrace((void**)&result,&getStackCollector);
	return result;
}


/* stacktrace_dump_trace *******************************************************

   This method is call from signal_handler_sigusr1 to dump the
   stacktrace of the current thread to stdout.

*******************************************************************************/

void stacktrace_dump_trace(void)
{
	stackTraceBuffer      *buffer;
	stacktraceelement     *element;
	methodinfo            *m;
	s4                     i;

#if 0
	/* get thread stackframeinfo */

	info = &THREADINFO->_stackframeinfo;

	/* fill stackframeinfo structure */

	tmp.oldThreadspecificHeadValue = *info;
	tmp.addressOfThreadspecificHead = info;
	tmp.method = NULL;
	tmp.beginOfJavaStackframe = NULL;
	tmp.returnToFromNative = _mc->gregs[REG_RIP];

	*info = &tmp;
#endif

	/* generate stacktrace */

	cacao_stacktrace_NormalTrace((void **) &buffer);

	/* print stacktrace */

	if (buffer) {
		element = buffer->start;

		for (i = 0; i < buffer->size; i++, element++) {
			m = element->method;

			printf("\tat ");
			utf_display_classname(m->class->name);
			printf(".");
			utf_display(m->name);
			utf_display(m->descriptor);

			if (m->flags & ACC_NATIVE) {
				printf("(Native Method)\n");

			} else {
				printf("(");
				utf_display(m->class->sourcefile);
				printf(":%d)\n", (u4) element->linenumber);
			}
		}
	}

	/* flush stdout */

	fflush(stdout);
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
