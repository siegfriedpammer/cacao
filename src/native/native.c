/* src/native/native.c - table of native functions

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
            Roman Obermaisser
            Andreas Krall

   Changes: Christian Thalinger

   $Id: native.c 2186 2005-04-02 00:43:25Z edwin $

*/


#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <utime.h>

/* Include files for IO functions */

#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#ifdef _OSF_SOURCE 
#include <sys/mode.h>
#endif
#include <sys/stat.h>

#include "config.h"
#include "cacao/cacao.h"
#include "mm/memory.h"
#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_Throwable.h"
#include "toolbox/logging.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/stringlocal.h"
#include "vm/tables.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/jit.h"
#include "vm/resolve.h"


/* include table of native functions ******************************************/

#include "nativetable.inc"


/************* use classinfo structure as java.lang.Class object **************/

void use_class_as_object(classinfo *c) 
{
	if (!c->classvftbl) {
		/* is the class loaded */
		if (!c->loaded)
/*  			if (!class_load(c)) */
/*  				throw_exception_exit(); */
			panic("use_class_as_object: class_load should not happen");

		/* is the class linked */
		if (!c->linked)
			if (!link_class(c))
				throw_exception_exit();

		/*if (class_java_lang_Class ==0) panic("java/lang/Class not loaded in use_class_as_object");
		if (class_java_lang_Class->vftbl ==0) panic ("vftbl == 0 in use_class_as_object");*/
		c->header.vftbl = class_java_lang_Class->vftbl;
		c->classvftbl = true;
  	}
}


/************************** tables for methods ********************************/

#undef JOWENN_DEBUG
#undef JOWENN_DEBUG1

#ifdef STATIC_CLASSPATH
#define NATIVETABLESIZE  (sizeof(nativetable)/sizeof(struct nativeref))

/* table for fast string comparison */
static nativecompref nativecomptable[NATIVETABLESIZE];

/* string comparsion table initialized */
static bool nativecompdone = false;
#endif


/* XXX don't define this in a header file!!! */

static struct nativeCall nativeCalls[] =
{
#include "nativecalls.inc"
};

#define NATIVECALLSSIZE    (sizeof(nativeCalls) / sizeof(struct nativeCall))

struct nativeCompCall nativeCompCalls[NATIVECALLSSIZE];


/* native_loadclasses **********************************************************

   Load classes required for native methods.

*******************************************************************************/

bool native_init(void)
{
#if !defined(STATIC_CLASSPATH)
	void *p;

	/* We need to access the dummy native table, not only to remove a warning */
	/* but to be sure that the table is not optimized away (gcc does this     */
	/* since 3.4).                                                            */
	p = &dummynativetable;
#endif

	/* everything's ok */

	return true;
}


/*********************** Function: native_findfunction *************************

	Looks up a method (must have the same class name, method name, descriptor
	and 'static'ness) and returns a function pointer to it.
	Returns: function pointer or NULL (if there is no such method)

	Remark: For faster operation, the names/descriptors are converted from C
		strings to Unicode the first time this function is called.

*******************************************************************************/

functionptr native_findfunction(utf *cname, utf *mname, 
								utf *desc, bool isstatic)
{
#ifdef STATIC_CLASSPATH
	int i;
	/* entry of table for fast string comparison */
	struct nativecompref *n;
	/* for warning message if no function is found */
	char *buffer;	         	
	int buffer_len;

	isstatic = isstatic ? true : false;
	
	if (!nativecompdone) {
		for (i = 0; i < NATIVETABLESIZE; i++) {
			nativecomptable[i].classname  = 
				utf_new_char(nativetable[i].classname);
			nativecomptable[i].methodname = 
				utf_new_char(nativetable[i].methodname);
			nativecomptable[i].descriptor = 
				utf_new_char(nativetable[i].descriptor);
			nativecomptable[i].isstatic   = 
				nativetable[i].isstatic;
			nativecomptable[i].func       = 
				nativetable[i].func;
		}
		nativecompdone = true;
	}

#ifdef JOWENN_DEBUG
	buffer_len = 
		utf_strlen(cname) + utf_strlen(mname) + utf_strlen(desc) + 64;
	
	buffer = MNEW(char, buffer_len);

	strcpy(buffer, "searching matching function in native table:");
	utf_sprint(buffer+strlen(buffer), mname);
	strcpy(buffer+strlen(buffer), ": ");
	utf_sprint(buffer+strlen(buffer), desc);
	strcpy(buffer+strlen(buffer), " for class ");
	utf_sprint(buffer+strlen(buffer), cname);

	log_text(buffer);	

	MFREE(buffer, char, buffer_len);
#endif
		
	for (i = 0; i < NATIVETABLESIZE; i++) {
		n = &(nativecomptable[i]);

		if (cname == n->classname && mname == n->methodname &&
		    desc == n->descriptor && isstatic == n->isstatic)
			return n->func;
#ifdef JOWENN_DEBUG
			else {
				if (cname == n->classname && mname == n->methodname )  log_text("static and descriptor mismatch");
			
				else {
					buffer_len = 
					  utf_strlen(n->classname) + utf_strlen(n->methodname) + utf_strlen(n->descriptor) + 64;
	
					buffer = MNEW(char, buffer_len);

					strcpy(buffer, "comparing with:");
					utf_sprint(buffer+strlen(buffer), n->methodname);
					strcpy (buffer+strlen(buffer), ": ");
					utf_sprint(buffer+strlen(buffer), n->descriptor);
					strcpy(buffer+strlen(buffer), " for class ");
					utf_sprint(buffer+strlen(buffer), n->classname);

					log_text(buffer);	

					MFREE(buffer, char, buffer_len);
				}
			} 
#endif
	}

		
	/* no function was found, display warning */

	buffer_len = 
		utf_strlen(cname) + utf_strlen(mname) + utf_strlen(desc) + 64;

	buffer = MNEW(char, buffer_len);

	strcpy(buffer, "warning: native function ");
	utf_sprint(buffer + strlen(buffer), mname);
	strcpy(buffer + strlen(buffer), ": ");
	utf_sprint(buffer + strlen(buffer), desc);
	strcpy(buffer + strlen(buffer), " not found in class ");
	utf_sprint(buffer + strlen(buffer), cname);

	log_text(buffer);	

	MFREE(buffer, char, buffer_len);

/*  	exit(1); */

	/* keep compiler happy */
	return NULL;
#else
/* dynamic classpath */
  return 0;
#endif
}


/****************** function class_findfield_approx ****************************
	
	searches in 'classinfo'-structure for a field with the
	specified name

*******************************************************************************/
 
fieldinfo *class_findfield_approx(classinfo *c, utf *name)
{
	s4 i;

	for (i = 0; i < c->fieldscount; i++) {
		/* compare field names */
		if ((c->fields[i].name == name))
			return &(c->fields[i]);
	}

	/* field was not found, raise exception */	
	*exceptionptr = new_exception(string_java_lang_NoSuchFieldException);

	return NULL;
}


s4 class_findfield_index_approx(classinfo *c, utf *name)
{
	s4 i;

	for (i = 0; i < c->fieldscount; i++) {
		/* compare field names */
		if ((c->fields[i].name == name))
			return i;
	}

	/* field was not found, raise exception */	
	*exceptionptr = new_exception(string_java_lang_NoSuchFieldException);

	return -1;
}


/* native_new_and_init *********************************************************

   Creates a new object on the heap and calls the initializer.
   Returns the object pointer or NULL if memory is exhausted.
			
*******************************************************************************/

java_objectheader *native_new_and_init(classinfo *c)
{
	methodinfo *m;
	java_objectheader *o;

	if (!c)
		return *exceptionptr;

	/* create object */

	o = builtin_new(c);
	
	if (!o)
		return NULL;

	/* find initializer */

	m = class_findmethod(c, utf_init, utf_void__void);
	                      	                      
	/* initializer not found */

	if (!m)
		return o;

	/* call initializer */

	asm_calljavafunction(m, o, NULL, NULL, NULL);

	return o;
}


java_objectheader *native_new_and_init_string(classinfo *c, java_lang_String *s)
{
	methodinfo *m;
	java_objectheader *o;

	if (!c)
		return *exceptionptr;

	/* create object */

	o = builtin_new(c);

	if (!o)
		return NULL;

	/* find initializer */

	m = class_resolveclassmethod(c,
								 utf_init,
								 utf_java_lang_String__void,
								 NULL,
								 true);

	/* initializer not found */

	if (!m)
		return NULL;

	/* call initializer */

	asm_calljavafunction(m, o, s, NULL, NULL);

	return o;
}


java_objectheader *native_new_and_init_int(classinfo *c, s4 i)
{
	methodinfo *m;
	java_objectheader *o;

	if (!c)
		return *exceptionptr;

	/* create object */

	o = builtin_new(c);
	
	if (!o)
		return NULL;

	/* find initializer */

	m = class_resolveclassmethod(c, utf_init, utf_int__void, NULL, true);

	/* initializer not found  */

	if (!m)
		return NULL;

	/* call initializer */

	asm_calljavafunction(m, o, (void *) (ptrint) i, NULL, NULL);

	return o;
}


java_objectheader *native_new_and_init_throwable(classinfo *c, java_lang_Throwable *t)
{
	methodinfo *m;
	java_objectheader *o;

	if (!c)
		return *exceptionptr;

	/* create object */

	o = builtin_new(c);
	
	if (!o)
		return NULL;

	/* find initializer */

	m = class_findmethod(c, utf_init, utf_java_lang_Throwable__void);
	                      	                      
	/* initializer not found */

	if (!m)
		return NULL;

	/* call initializer */

	asm_calljavafunction(m, o, t, NULL, NULL);

	return o;
}


void copy_vftbl(vftbl_t **dest, vftbl_t *src)
{
    *dest = src;
#if 0
    /* XXX this kind of copying does not work (in the general
     * case). The interface tables would have to be copied, too. I
     * don't see why we should make a copy anyway. -Edwin
     */
	*dest = mem_alloc(sizeof(vftbl) + sizeof(methodptr)*(src->vftbllength-1));
	memcpy(*dest, src, sizeof(vftbl) - sizeof(methodptr));
	memcpy(&(*dest)->table, &src->table, src->vftbllength * sizeof(methodptr));
#endif
}


/****************************************************************************************** 											   			

	creates method signature (excluding return type) from array of 
	class-objects representing the parameters of the method 

*******************************************************************************************/


utf *create_methodsig(java_objectarray* types, char *retType)
{
    char *buffer;       /* buffer for building the desciptor */
    char *pos;          /* current position in buffer */
    utf *result;        /* the method signature */
    u4 buffer_size = 3; /* minimal size=3: room for parenthesis and returntype */
    u4 i, j;
 
    if (!types) return NULL;

    /* determine required buffer-size */    
    for (i = 0; i < types->header.size; i++) {
		classinfo *c = (classinfo *) types->data[i];
		buffer_size  = buffer_size + c->name->blength + 2;
    }

    if (retType) buffer_size += strlen(retType);

    /* allocate buffer */
    buffer = MNEW(char, buffer_size);
    pos    = buffer;
    
    /* method-desciptor starts with parenthesis */
    *pos++ = '(';

    for (i = 0; i < types->header.size; i++) {
		char ch;	   

		/* current argument */
	    classinfo *c = (classinfo *) types->data[i];

	    /* current position in utf-text */
	    char *utf_ptr = c->name->text; 
	    
	    /* determine type of argument */
	    if ((ch = utf_nextu2(&utf_ptr)) == '[') {
	    	/* arrayclass */
	        for (utf_ptr--; utf_ptr < utf_end(c->name); utf_ptr++) {
				*pos++ = *utf_ptr; /* copy text */
			}

	    } else {	   	
			/* check for primitive types */
			for (j = 0; j < PRIMITIVETYPE_COUNT; j++) {
				char *utf_pos	= utf_ptr - 1;
				char *primitive = primitivetype_table[j].wrapname;

				/* compare text */
				while (utf_pos < utf_end(c->name)) {
					if (*utf_pos++ != *primitive++) goto nomatch;
				}

				/* primitive type found */
				*pos++ = primitivetype_table[j].typesig;
				goto next_type;

			nomatch:
				;
			}

			/* no primitive type and no arrayclass, so must be object */
			*pos++ = 'L';

			/* copy text */
			for (utf_ptr--; utf_ptr < utf_end(c->name); utf_ptr++) {
				*pos++ = *utf_ptr;
			}

			*pos++ = ';';

		next_type:
			;
		}  
    }	    

    *pos++ = ')';

    if (retType) {
		for (i = 0; i < strlen(retType); i++) {
			*pos++ = retType[i];
		}
    }

    /* create utf-string */
    result = utf_new(buffer, (pos - buffer));
    MFREE(buffer, char, buffer_size);

    return result;
}


/* get_parametertypes **********************************************************

   use the descriptor of a method to generate a java/lang/Class array
   which contains the classes of the parametertypes of the method

*******************************************************************************/

java_objectarray* get_parametertypes(methodinfo *m) 
{
    methoddesc *descr =  m->parseddesc;    /* method-descriptor */ 
    java_objectarray* result;
    int parametercount = descr->paramcount;
    int i;

    /* create class-array */
    result = builtin_anewarray(parametercount, class_java_lang_Class);

    /* get classes */
    for (i = 0; i < parametercount; i++) {
		if (!resolve_class_from_typedesc(descr->paramtypes + i,false,
					(classinfo **) (result->data + i)))
			return NULL; /* exception */
		use_class_as_object((classinfo*) result->data[i]);
	}

    return result;
}


/* get_exceptiontypes **********************************************************

   get the exceptions which can be thrown by a method

*******************************************************************************/

java_objectarray* get_exceptiontypes(methodinfo *m)
{
    u2 excount;
    u2 i;
    java_objectarray *result;

	excount = m->thrownexceptionscount;

    /* create class-array */
    result = builtin_anewarray(excount, class_java_lang_Class);

    for (i = 0; i < excount; i++) {
		java_objectheader *o = (java_objectheader *) (m->thrownexceptions[i]);
		use_class_as_object((classinfo *) o);
		result->data[i] = o;
    }

    return result;
}


/******************************************************************************************

	get the returntype class of a method

*******************************************************************************************/

classinfo *get_returntype(methodinfo *m) 
{
	classinfo *cls;

	if (!resolve_class_from_typedesc(&(m->parseddesc->returntype),false,&cls))
		return NULL; /* exception */

	use_class_as_object(cls);
	return cls;
}


/*****************************************************************************/
/*****************************************************************************/


/*--------------------------------------------------------*/
void printNativeCall(nativeCall nc) {
  int i,j;

  printf("\n%s's Native Methods call:\n",nc.classname); fflush(stdout);
  for (i=0; i<nc.methCnt; i++) {  
      printf("\tMethod=%s %s\n",nc.methods[i].methodname, nc.methods[i].descriptor);fflush(stdout);

    for (j=0; j<nc.callCnt[i]; j++) {  
        printf("\t\t<%i,%i>aCalled = %s %s %s\n",i,j,
	nc.methods[i].methodCalls[j].classname, 
	nc.methods[i].methodCalls[j].methodname, 
	nc.methods[i].methodCalls[j].descriptor);fflush(stdout);
      }
    }
  printf("-+++++--------------------\n");fflush(stdout);
}

/*--------------------------------------------------------*/
void printCompNativeCall(nativeCompCall nc) {
  int i,j;
  printf("printCompNativeCall BEGIN\n");fflush(stdout); 
  printf("\n%s's Native Comp Methods call:\n",nc.classname->text);fflush(stdout);
  utf_display(nc.classname); fflush(stdout);
  
  for (i=0; i<nc.methCnt; i++) {  
    printf("\tMethod=%s %s\n",nc.methods[i].methodname->text,nc.methods[i].descriptor->text);fflush(stdout);
    utf_display(nc.methods[i].methodname); fflush(stdout);
    utf_display(nc.methods[i].descriptor);fflush(stdout);
    printf("\n");fflush(stdout);

    for (j=0; j<nc.callCnt[i]; j++) {  
      printf("\t\t<%i,%i>bCalled = ",i,j);fflush(stdout);
	utf_display(nc.methods[i].methodCalls[j].classname);fflush(stdout);
	utf_display(nc.methods[i].methodCalls[j].methodname); fflush(stdout);
	utf_display(nc.methods[i].methodCalls[j].descriptor);fflush(stdout);
	printf("\n");fflush(stdout);
      }
    }
printf("---------------------\n");fflush(stdout);
}


/*--------------------------------------------------------*/
classMeth findNativeMethodCalls(utf *c, utf *m, utf *d ) 
{
    int i = 0;
    int j = 0;
    int cnt = 0;
    classMeth mc;
    mc.i_class = i;
    mc.j_method = j;
    mc.methCnt = cnt;

    return mc;
}

/*--------------------------------------------------------*/
nativeCall* findNativeClassCalls(char *aclassname ) {
int i;

for (i=0;i<NATIVECALLSSIZE; i++) {
   /* convert table to utf later to speed up search */ 
   if (strcmp(nativeCalls[i].classname, aclassname) == 0) 
	return &nativeCalls[i];
   }

return NULL;
}
/*--------------------------------------------------------*/
/*--------------------------------------------------------*/
void utfNativeCall(nativeCall nc, nativeCompCall *ncc) {
  int i,j;


  ncc->classname = utf_new_char(nc.classname); 
  ncc->methCnt = nc.methCnt;
  
  for (i=0; i<nc.methCnt; i++) {  
    ncc->methods[i].methodname = utf_new_char(nc.methods[i].methodname);
    ncc->methods[i].descriptor = utf_new_char(nc.methods[i].descriptor);
    ncc->callCnt[i] = nc.callCnt[i];

    for (j=0; j<nc.callCnt[i]; j++) {  

	ncc->methods[i].methodCalls[j].classname  = utf_new_char(nc.methods[i].methodCalls[j].classname);

        if (strcmp("", nc.methods[i].methodCalls[j].methodname) != 0) {
          ncc->methods[i].methodCalls[j].methodname = utf_new_char(nc.methods[i].methodCalls[j].methodname);
          ncc->methods[i].methodCalls[j].descriptor = utf_new_char(nc.methods[i].methodCalls[j].descriptor);
          }
        else {
          ncc->methods[i].methodCalls[j].methodname = NULL;
          ncc->methods[i].methodCalls[j].descriptor = NULL;
          }
      }
    }
}



/*--------------------------------------------------------*/

bool natcall2utf(bool natcallcompdone) {
int i;

if (natcallcompdone) 
	return true;

for (i=0;i<NATIVECALLSSIZE; i++) {
   utfNativeCall  (nativeCalls[i], &nativeCompCalls[i]);  
   }

return true;
}

/*--------------------------------------------------------*/


java_objectarray *builtin_asm_createclasscontextarray(classinfo **end, classinfo **start)
{
#if defined(__GNUC__)
#warning platform dependend
#endif
	java_objectarray *tmpArray;
	int i;
	classinfo **current;
	classinfo *c;
	size_t size;

	size = (((size_t) start) - ((size_t) end)) / sizeof(classinfo*);

	/*printf("end %p, start %p, size %ld\n",end,start,size);*/
	if (!class_java_lang_Class)
		class_java_lang_Class = class_new(utf_new_char("java/lang/Class"));

	if (!class_java_lang_SecurityManager)
		class_java_lang_SecurityManager =
			class_new(utf_new_char("java/lang/SecurityManager"));

	if (size > 0) {
		if (*start == class_java_lang_SecurityManager) {
			size--;
			start--;
		}
	}

	tmpArray = (java_objectarray*)
		builtin_newarray(size, class_array_of(class_java_lang_Class)->vftbl);

	for(i = 0, current = start; i < size; i++, current--) {
		c = *current;
		/*		printf("%d\n",i);
                utf_display(c->name);*/
		use_class_as_object(c);
		tmpArray->data[i] = (java_objectheader *) c;
	}

	return tmpArray;
}


java_lang_ClassLoader *builtin_asm_getclassloader(classinfo **end, classinfo **start)
{
#if defined(__GNUC__)
#warning platform dependend
#endif
	int i;
	classinfo **current;
	classinfo *c;
	classinfo *privilegedAction;
	size_t size;

	size = (((size_t) start) - ((size_t) end)) / sizeof(classinfo*);

	/*	log_text("builtin_asm_getclassloader");
        printf("end %p, start %p, size %ld\n",end,start,size);*/

	if (!class_java_lang_SecurityManager)
		class_java_lang_SecurityManager =
			class_new(utf_new_char("java/lang/SecurityManager"));

	if (size > 0) {
		if (*start == class_java_lang_SecurityManager) {
			size--;
			start--;
		}
	}

	privilegedAction=class_new(utf_new_char("java/security/PrivilegedAction"));

	for(i = 0, current = start; i < size; i++, current--) {
		c = *current;

		if (c == privilegedAction)
			return NULL;

		if (c->classloader)
			return (java_lang_ClassLoader *) c->classloader;
	}

	return NULL;
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
