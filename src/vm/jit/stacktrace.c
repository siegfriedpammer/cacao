/* vm/jit/stacktrace.c

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

   $Id: stacktrace.c 1774 2004-12-20 20:16:57Z jowenn $

*/


#include <stdlib.h>
#include <string.h>

#include "asmoffsets.h"
#include "mm/boehm.h"
#include "native/native.h"
#include "vm/global.h"                   /* required here for native includes */
#include "native/include/java_lang_ClassLoader.h"
#include "toolbox/logging.h"
#include "vm/builtin.h"
#include "vm/tables.h"
#include "vm/jit/codegen.inc.h"


#undef JWDEBUG
/*JoWenn: simplify collectors (trace doesn't contain internal methods)*/

extern classinfo *class_java_lang_Class;
extern classinfo *class_java_lang_SecurityManager;

/* the line number is only u2, but to avoid alignment problems it is made the same size as a native
	pointer. In the structures where this is used, values of -1 or -2 have a special meainging, so
	if java bytecode is ever extended to support more than 65535 lines/file, this could will have to
	be changed.*/

#ifdef _ALPHA_
	#define LineNumber u8
#else
	#define LineNumber u4
#endif

typedef struct lineNumberTableEntry {
/* The special value of -1 means that a inlined function starts, a value of -2 means that an inlined function ends*/
	LineNumber lineNr;
	void *pc;
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
#ifdef JWDEBUG
		log_text("addEntry (stacktrace):");
		if (method) utf_display(method->name); else printf("Native");
		if (method) {printf("\n");utf_display(method->class->name);}
		printf("\nLine:%ld\n",line);
#endif
	} else {
		stacktraceelement *newBuffer=(stacktraceelement*)
			malloc((buffer->size+BLOCK_SIZEINCREMENT)*sizeof(stacktraceelement));
		if (newBuffer==0) panic("OOM during stacktrace creation");
		memcpy(newBuffer,buffer->start,buffer->size*sizeof(stacktraceelement));
		if (buffer->needsFree) free(buffer->start);
		buffer->start=newBuffer;
		buffer->size=buffer->size+BLOCK_SIZEINCREMENT;
		buffer->needsFree=1;
		addEntry(buffer,method,line);
	}
}

static int fillInStackTrace_methodRecursive(stackTraceBuffer *buffer,methodinfo 
		*method,lineNumberTableEntry *startEntry, lineNumberTableEntry **entry, size_t *entriesAhead,void *adress) {

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
			} else panic("trace point before method");
		}
	}
	ent--;
	addEntry(buffer,method,ent->lineNr);
	return 1;
	
}

static void fillInStackTrace_method(stackTraceBuffer *buffer,methodinfo *method,char *dataSeg, void* adress) {
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
		calc=dataSeg+LineNumberTableStart;
		/*		printf("position of line number table start reference in data segment: %p\n",calc);
				printf("line number table start as found in table: %p\n",*calc);*/
		ent=(lineNumberTableEntry *) (((char*)(*calc) - (sizeof(lineNumberTableEntry)-sizeof(void*))));
		/*		printf("line number table start as calculated: %p\n",ent);*/
		ent-=(lineNumberTableSize-1);
		startEntry=ent;
		/*		printf("line number table real start (bottom end) as calculated(2): %p\n",startEntry);*/

		if (!fillInStackTrace_methodRecursive(buffer,method,startEntry,&ent,&lineNumberTableSize,adress)) {
			panic("Trace point not found in suspected method");
		}
	}
}


void  cacao_stacktrace_fillInStackTrace(void **target,CacaoStackTraceCollector coll)
{

	stacktraceelement primaryBlock[BLOCK_INITIALSIZE*sizeof(stacktraceelement)]; 
		/*In most cases this should be enough -> one malloc less. I don't think temporary data should be
		allocated with the GC, only the result*/
	stackTraceBuffer buffer;
	buffer.needsFree=0;
	buffer.start=primaryBlock;
	buffer.size=BLOCK_INITIALSIZE*sizeof(stacktraceelement);
	buffer.full=0;


	{
		struct native_stackframeinfo *info=(*(((void**)(builtin_asm_get_stackframeinfo()))));
		if (!info) {
			log_text("info ==0");
			*target=0;
			return;
		} else {
			char *dataseg; /*make it byte addressable*/
			methodinfo *currentMethod=0;
			void *returnAdress;
			char* stackPtr;

/*			utf_display(info->method->class->name);
			utf_display(info->method->name);*/
			
			while ((currentMethod!=0) ||  (info!=0)) {
				if (currentMethod==0) { /*some builtin native */
					currentMethod=info->method;
					returnAdress=info->returnToFromNative;
					/*log_text("native");*/
					if (currentMethod) {
						/*utf_display(currentMethod->class->name);
						utf_display(currentMethod->name);*/
						addEntry(&buffer,currentMethod,0);
					}
#if defined(__ALPHA__)
					if (info->savedpv!=0)
						dataseg=info->savedpv;
					else
						dataseg=codegen_findmethod(returnAdress);
#elif defined(__I386__)
					dataseg=codegen_findmethod(returnAdress);
#endif
					currentMethod=(*((methodinfo**)(dataseg+MethodPointer)));
					if (info->beginOfJavaStackframe==0)
						stackPtr=((char*)info)+sizeof(native_stackframeinfo);
					else
#if defined(__ALPHA__)
						stackPtr=(char*)(info->beginOfJavaStackframe);
#elif defined(__I386__)
						stackPtr=(char*)(info->beginOfJavaStackframe)+sizeof(void*);
#endif
					info=info->oldThreadspecificHeadValue;
				} else { /*method created by jit*/
					u4 frameSize;
					/*log_text("JIT");*/
#if defined (__ALPHA__)
					if (currentMethod->isleafmethod) {
#ifdef JWDEBUG
						printf("class.method:%s.%s\n",currentMethod->class->name->text,currentMethod->name->text);
#endif
						panic("How could that happen ??? A leaf method in the middle of a stacktrace ??");
					}
#endif
					/*utf_display(currentMethod->class->name);
					utf_display(currentMethod->name);*/
					fillInStackTrace_method(&buffer,currentMethod,dataseg,returnAdress);
					frameSize=*((u4*)(dataseg+FrameSize));
#if defined(__ALPHA__)
					/* cacao saves the return adress as the first element of the stack frame on alphas*/
					dataseg=codegen_findmethod(*((void**)(stackPtr+frameSize-sizeof(void*))));
					returnAdress=(*((void**)(stackPtr+frameSize-sizeof(void*))));
#elif defined(__I386__)
					/* on i386 the return adress is the first element before the stack frme*/
					returnAdress=(*((void**)(stackPtr+frameSize)));
					dataseg=codegen_findmethod(*((void**)(stackPtr+frameSize)));
#endif
/*					printf ("threadrootmethod %p\n",builtin_asm_get_threadrootmethod());
					if (currentMethod==builtin_asm_get_threadrootmethod()) break;*/
					currentMethod=(*((methodinfo**)(dataseg+MethodPointer)));
#if defined(__ALPHA__)
					stackPtr+=frameSize;
#elif defined(__I386__)
					stackPtr+=frameSize+sizeof(void*);
#endif
				}
			}
			
			if (coll) coll(target,&buffer);
			if (buffer.needsFree) free(buffer.start);
			return;
		}
		/*log_text("\n=========================================================");*/
	}
	*target=0;

}


static
void stackTraceCollector(void **target, stackTraceBuffer *buffer) {
	stackTraceBuffer *dest=*target=heap_allocate(sizeof(stackTraceBuffer)+buffer->full*sizeof(stacktraceelement),true,0);
	memcpy(*target,buffer,sizeof(stackTraceBuffer));
	memcpy(dest+1,buffer->start,buffer->full*sizeof(stacktraceelement));

	dest->needsFree=0;
	dest->size=dest->full;
	dest->start=dest+1;

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



static
void classContextCollector(void **target, stackTraceBuffer *buffer) {
        java_objectarray *tmpArray;
        int i;
        stacktraceelement *current;
        stacktraceelement *start;
        classinfo *c;
        size_t size;
	size_t targetSize;

	size=buffer->full;
	targetSize=0;
	for (i=0;i<size;i++)
		if (buffer->start[i].method!=0) targetSize++;
	start=buffer->start;
	start++;
	targetSize--;
        if (!class_java_lang_Class)
                class_java_lang_Class = class_new(utf_new_char("java/lang/Class"));

        if (!class_java_lang_SecurityManager)
                class_java_lang_SecurityManager =
                        class_new(utf_new_char("java/lang/SecurityManager"));

        if (targetSize > 0) {
                if ((start->method) && (start->method->class== class_java_lang_SecurityManager)) {
                        targetSize--;
                        start++;
                }
        }

        tmpArray =
                builtin_newarray(targetSize, class_array_of(class_java_lang_Class)->vftbl);

        for(i = 0, current = start; i < targetSize; i++, current++) {
                if (current->method==0) { i--; continue;}
		/*printf("adding item to class context array:%s\n",current->method->class->name->text);
		printf("method for class: :%s\n",current->method->name->text);*/
                use_class_as_object(current->method->class);
                tmpArray->data[i] = (java_objectheader *) current->method->class;
        }

        *target=tmpArray;
}



java_objectarray *cacao_createClassContextArray() {
	java_objectarray *array=0;
	cacao_stacktrace_fillInStackTrace(&array,&classContextCollector);
	return array;
	
}


static
void classLoaderCollector(void **target, stackTraceBuffer *buffer) {
        int i;
        stacktraceelement *current;
        stacktraceelement *start;
        methodinfo *m;
        classinfo *privilegedAction;
        size_t size;

        size = buffer->full;


        if (!class_java_lang_SecurityManager)
                class_java_lang_SecurityManager =
                        class_new(utf_new_char("java/lang/SecurityManager"));

        if (size > 1) {
		size--;
		start=&(buffer->start[1]);
                if (start == class_java_lang_SecurityManager) {
                        size--;
                        start--;
                }
        } else {
		start=0;
		size=0;
	}
        privilegedAction=class_new(utf_new_char("java/security/PrivilegedAction"));

        for(i=0, current = start; i < size; i++, current++) {
                m=start->method;
		if (!m) continue;

                if (m->class == privilegedAction) {
			*target=NULL;
                        return;
		}

                if (m->class->classloader) {
                        *target= (java_lang_ClassLoader *) m->class->classloader;
			return;
		}
        }

        *target=NULL;
}

java_objectheader *cacao_currentClassLoader() {
	java_objectheader *header=0;
	cacao_stacktrace_fillInStackTrace(&header,&classLoaderCollector);
	return header;
}


static
void callingMethodCollector(void **target, stackTraceBuffer *buffer) {	
        if (buffer->full >2) (*target)=buffer->start[2].method;
	else (*target=0);
}

methodinfo *cacao_callingMethod() {
	methodinfo *method;
	cacao_stacktrace_fillInStackTrace(&method,&callingMethodCollector);
	return method;
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
