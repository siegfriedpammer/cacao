/* loader.c - class loader functions

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser,
   M. Probst, S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck,
   P. Tomsich, J. Wenninger

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
   Changes: Andreas Krall
            Roman Obermaiser
            Mark Probst
			Edwin Steiner

   $Id: loader.c 867 2004-01-07 22:05:04Z edwin $

*/


#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include "global.h"
#include "loader.h"
#include "main.h"
#include "native.h"
#include "tables.h"
#include "builtin.h"
#include "jit.h"
#include "asmpart.h"
#include "toolbox/memory.h"
#include "toolbox/loging.h"
#include "threads/thread.h"
#include "threads/locks.h"
#include <sys/stat.h>

#ifdef USE_ZLIB
#include "unzip.h"
#endif

#undef JOWENN_DEBUG
#undef JOWENN_DEBUG1
#undef JOWENN_DEBUG2

/* global variables ***********************************************************/

int count_class_infos = 0;      /* variables for measurements                 */
int count_const_pool_len = 0;
int count_vftbl_len = 0;
int count_all_methods = 0;
int count_vmcode_len = 0;
int count_extable_len = 0;
int count_class_loads = 0;
int count_class_inits = 0;

static s4 interfaceindex;       /* sequential numbering of interfaces         */
static s4 classvalue;

list unloadedclasses;       /* list of all referenced but not loaded classes  */
list unlinkedclasses;       /* list of all loaded but not linked classes      */
list linkedclasses;         /* list of all completely linked classes          */


/* utf-symbols for pointer comparison of frequently used strings */

static utf *utf_innerclasses; 		/* InnerClasses            */
static utf *utf_constantvalue; 		/* ConstantValue           */
static utf *utf_code;			    /* Code                    */
static utf *utf_finalize;		    /* finalize                */
static utf *utf_fidesc;   		    /* ()V changed             */
static utf *utf_init;  		        /* <init>                  */
static utf *utf_clinit;  		    /* <clinit>                */
static utf *utf_initsystemclass;	/* initializeSystemClass   */
static utf *utf_systemclass;		/* java/lang/System        */
static utf *utf_vmclassloader;      /* java/lang/VMClassLoader */
static utf *utf_vmclass;            /* java/lang/VMClassLoader */
static utf *utf_initialize;
static utf *utf_initializedesc;
static utf *utf_java_lang_Object;   /* java/lang/Object        */




utf* clinit_desc(){
	return utf_fidesc;
}
utf* clinit_name(){
	return utf_clinit;
}


/* important system classes ***************************************************/

classinfo *class_java_lang_Object;
classinfo *class_java_lang_String;

classinfo *class_java_lang_Throwable;
classinfo *class_java_lang_Cloneable;
classinfo *class_java_io_Serializable;

/* Pseudo classes for the typechecker */
classinfo *pseudo_class_Arraystub = NULL;
classinfo *pseudo_class_Null = NULL;
classinfo *pseudo_class_New = NULL;
vftbl *pseudo_class_Arraystub_vftbl = NULL;

/* stefan */
/* These are made static so they cannot be used for throwing in native */
/* functions.                                                          */
static classinfo *class_java_lang_ClassCastException;
static classinfo *class_java_lang_NullPointerException;
static classinfo *class_java_lang_ArrayIndexOutOfBoundsException;
static classinfo *class_java_lang_NegativeArraySizeException;
static classinfo *class_java_lang_OutOfMemoryError;
static classinfo *class_java_lang_ArithmeticException;
static classinfo *class_java_lang_ArrayStoreException;
static classinfo *class_java_lang_ThreadDeath;

static int loader_inited = 0;


/********************************************************************
   list of classpath entries (either filesystem directories or 
   ZIP/JAR archives
********************************************************************/
static classpath_info *classpath_entries=0;


/******************************************************************************

   structure for primitive classes: contains the class for wrapping the 
   primitive type, the primitive class, the name of the class for wrapping, 
   the one character type signature and the name of the primitive class
 
 ******************************************************************************/

/* CAUTION: Don't change the order of the types. This table is indexed
 * by the ARRAYTYPE_ constants (expcept ARRAYTYPE_OBJECT).
 */
primitivetypeinfo primitivetype_table[PRIMITIVETYPE_COUNT] = { 
	{ NULL, NULL, "java/lang/Integer",   'I', "int"     , "[I", NULL, NULL },
	{ NULL, NULL, "java/lang/Long",      'J', "long"    , "[J", NULL, NULL },
	{ NULL, NULL, "java/lang/Float",     'F', "float"   , "[F", NULL, NULL },
	{ NULL, NULL, "java/lang/Double",    'D', "double"  , "[D", NULL, NULL },
	{ NULL, NULL, "java/lang/Byte",	     'B', "byte"    , "[B", NULL, NULL },
	{ NULL, NULL, "java/lang/Character", 'C', "char"    , "[C", NULL, NULL },
	{ NULL, NULL, "java/lang/Short",     'S', "short"   , "[S", NULL, NULL },
	{ NULL, NULL, "java/lang/Boolean",   'Z', "boolean" , "[Z", NULL, NULL },
	{ NULL, NULL, "java/lang/Void",	     'V', "void"    , NULL, NULL, NULL }
};


/* instances of important system classes **************************************/

java_objectheader *proto_java_lang_ClassCastException;
java_objectheader *proto_java_lang_NullPointerException;
java_objectheader *proto_java_lang_ArrayIndexOutOfBoundsException;
java_objectheader *proto_java_lang_NegativeArraySizeException;
java_objectheader *proto_java_lang_OutOfMemoryError;
java_objectheader *proto_java_lang_ArithmeticException;
java_objectheader *proto_java_lang_ArrayStoreException;
java_objectheader *proto_java_lang_ThreadDeath;

/************* functions for reading classdata *********************************

    getting classdata in blocks of variable size
    (8,16,32,64-bit integer or float)

*******************************************************************************/

static char *classpath    = "";     /* searchpath for classfiles              */
static u1 *classbuffer    = NULL;   /* pointer to buffer with classfile-data  */
static u1 *classbuf_pos;            /* current position in classfile buffer   */
static int classbuffer_size;        /* size of classfile-data                 */

/* assert that at least <len> bytes are left to read */
/* <len> is limited to the range of non-negative s4 values */
#define ASSERT_LEFT(len)												\
	do {if ( ((s4)(len)) < 0											\
			 || ((classbuffer + classbuffer_size) - classbuf_pos - 1) < (len)) \
			panic("Unexpected end of classfile"); } while(0)

/* transfer block of classfile data into a buffer */

#define suck_nbytes(buffer,len)						\
	do {ASSERT_LEFT(len);							\
		memcpy(buffer,classbuf_pos+1,len);			\
		classbuf_pos+=len;} while (0)

/* skip block of classfile data */

#define skip_nbytes(len)						\
	do {ASSERT_LEFT(len);						\
		classbuf_pos+=len;} while(0)

inline u1 suck_u1()
{
	ASSERT_LEFT(1);
	return *++classbuf_pos;
}
inline u2 suck_u2()
{
	u1 a=suck_u1(), b=suck_u1();
	return ((u2)a<<8)+(u2)b;
}
inline u4 suck_u4()
{
	u1 a=suck_u1(), b=suck_u1(), c=suck_u1(), d=suck_u1();
	return ((u4)a<<24)+((u4)b<<16)+((u4)c<<8)+(u4)d;
}
#define suck_s8() (s8) suck_u8()
#define suck_s2() (s2) suck_u2()
#define suck_s4() (s4) suck_u4()
#define suck_s1() (s1) suck_u1()


/* get u8 from classfile data */
static u8 suck_u8()
{
#if U8_AVAILABLE
	u8 lo, hi;
	hi = suck_u4();
	lo = suck_u4();
	return (hi << 32) + lo;
#else
	u8 v;
	v.high = suck_u4();
	v.low = suck_u4();
	return v;
#endif
}


/* get float from classfile data */
static float suck_float()
{
	float f;

#if !WORDS_BIGENDIAN 
		u1 buffer[4];
		u2 i;
		for (i = 0; i < 4; i++) buffer[3 - i] = suck_u1();
		memcpy((u1*) (&f), buffer, 4);
#else 
		suck_nbytes((u1*) (&f), 4);
#endif

	PANICIF (sizeof(float) != 4, "Incompatible float-format");
	
	return f;
}


/* get double from classfile data */
static double suck_double()
{
	double d;

#if !WORDS_BIGENDIAN 
		u1 buffer[8];
		u2 i;	
		for (i = 0; i < 8; i++) buffer[7 - i] = suck_u1 ();
		memcpy((u1*) (&d), buffer, 8);
#else 
		suck_nbytes((u1*) (&d), 8);
#endif

	PANICIF (sizeof(double) != 8, "Incompatible double-format" );
	
	return d;
}


/************************** function suck_init *********************************

	called once at startup, sets the searchpath for the classfiles

*******************************************************************************/

void suck_init(char *cpath)
{
	char *filename=0;
	char *start;
	char *end;
	int isZip;
	int filenamelen;
	union classpath_info *tmp;
	union classpath_info *insertAfter=0;
	if (!cpath) return;
	classpath   = cpath;
	classbuffer = NULL;
	
	if (classpath_entries) panic("suck_init should be called only once");
	for(start=classpath;(*start)!='\0';) {
		for (end=start; ((*end)!='\0') && ((*end)!=':') ; end++);
		if (start!=end) {
			isZip=0;
			filenamelen=(end-start);
			if  (filenamelen>3) {
				if ( 
					(
						(
							((*(end-1))=='p') ||
							((*(end-1))=='P')
						) &&
						(
							((*(end-2))=='i') ||
							((*(end-2))=='I')
						) &&
						(
							((*(end-3))=='z') ||
							((*(end-3))=='Z')
						)
					) ||
					(
						(
							((*(end-1))=='r') ||
							((*(end-1))=='R')
						) &&
						(
							((*(end-2))=='a') ||
							((*(end-2))=='A')
						) &&
						(
							((*(end-3))=='j') ||
							((*(end-3))=='J')
						)
					)
				) isZip=1;
#if 0
				if ( 
					(  (  (*(end - 1)) == 'p') || ( (*(end - 1))  == 'P')) &&
				     (((*(end - 2)) == 'i') || ( (*(end - 2)) == 'I'))) &&
					 (( (*(end - 3)) == 'z') || ((*(end - 3)) == 'Z'))) 
				) {
					isZip=1;
				} else	if ( (( (*(end - 1)) == 'r') || ( (*(end - 1))  == 'R')) &&
				     ((*(end - 2)) == 'a') || ( (*(end - 2)) == 'A')) &&
					 (( (*(end - 3)) == 'j') || ((*(end - 3)) == 'J')) ) {
					isZip=1;
				}
#endif
			}
			if (filenamelen>=(CLASSPATH_MAXFILENAME-1)) panic("path length >= MAXFILENAME in suck_init"); 
			if (!filename)
				filename=(char*)malloc(CLASSPATH_MAXFILENAME);
			strncpy(filename,start,filenamelen);
			filename[filenamelen+1]='\0';
			tmp=0;

			if (isZip) {
#ifdef USE_ZLIB
				unzFile uf=unzOpen(filename);
				if (uf) {
					tmp=(union classpath_info*)malloc(sizeof(classpath_info));
					tmp->archive.type=CLASSPATH_ARCHIVE;
					tmp->archive.uf=uf;
					tmp->archive.next=0;
					filename=0;
				}
#else
				panic("Zip/JAR not supported");
#endif
				
			} else {
				tmp=(union classpath_info*)malloc(sizeof(classpath_info));
				tmp->filepath.type=CLASSPATH_PATH;
				tmp->filepath.next=0;
				if (filename[filenamelen-1]!='/') {/*PERHAPS THIS SHOULD BE READ FROM A GLOBAL CONFIGURATION */
					filename[filenamelen]='/';
					filename[filenamelen+1]='\0';
					filenamelen++;
				}
			
				tmp->filepath.filename=filename;
				tmp->filepath.pathlen=filenamelen;				
				filename=0;
			}

		if (tmp) {
			if (insertAfter) {
				insertAfter->filepath.next=tmp;
			} else {
				classpath_entries=tmp;	
			}
			insertAfter=tmp;
		}
	}
		if ((*end)==':') start=end+1; else start=end;
	
	
	if (filename!=0) free(filename);
}

}

/************************** function suck_start ********************************

	returns true if classbuffer is already loaded or a file for the
	specified class has succussfully been read in. All directories of
	the searchpath are used to find the classfile (<classname>.class).
	Returns false if no classfile is found and writes an error message. 
	
*******************************************************************************/

bool suck_start(utf *classname)
{
	classpath_info *currPos;
	char *utf_ptr;
	char c;
	char filename[CLASSPATH_MAXFILENAME+10];  /* room for '.class'                      */	
	int  filenamelen=0;
	FILE *classfile;
	int  err;
	struct stat buffer;

	if (classbuffer) return true; /* classbuffer is already valid */

	utf_ptr = classname->text;
	while (utf_ptr < utf_end(classname)) {
		PANICIF (filenamelen >= CLASSPATH_MAXFILENAME, "Filename too long");
		c = *utf_ptr++;
		if ((c <= ' ' || c > 'z') && (c != '/'))     /* invalid character */
			c = '?';
		filename[filenamelen++] = c;
	}
	strcpy(filename + filenamelen, ".class");
	filenamelen+=6;
	for (currPos=classpath_entries;currPos!=0;currPos=currPos->filepath.next) {
#ifdef USE_ZLIB
		if (currPos->filepath.type==CLASSPATH_ARCHIVE) {
			if (cacao_locate(currPos->archive.uf,classname) == UNZ_OK) {
				unz_file_info file_info;
				/*log_text("Class found in zip file");*/
			        if (unzGetCurrentFileInfo(currPos->archive.uf, &file_info, filename,
						sizeof(filename), NULL, 0, NULL, 0) == UNZ_OK) {
					if (unzOpenCurrentFile(currPos->archive.uf) == UNZ_OK) {
						classbuffer_size = file_info.uncompressed_size;				
						classbuffer      = MNEW(u1, classbuffer_size);
						classbuf_pos     = classbuffer - 1;
						/*printf("classfile size: %d\n",file_info.uncompressed_size);*/
						if (unzReadCurrentFile(currPos->archive.uf, classbuffer, classbuffer_size) == classbuffer_size) {
							unzCloseCurrentFile(currPos->archive.uf);
							return true;
						} else {
							MFREE(classbuffer, u1, classbuffer_size);
							log_text("Error while unzipping");
						}
					} else log_text("Error while opening file in archive");
				} else log_text("Error while retrieving fileinfo");
			}
			unzCloseCurrentFile(currPos->archive.uf);

		} else {
#endif
			if ((currPos->filepath.pathlen+filenamelen)>=CLASSPATH_MAXFILENAME) continue;
			strcpy(currPos->filepath.filename+currPos->filepath.pathlen,filename);
			classfile = fopen(currPos->filepath.filename, "r");
			if (classfile) {                                       /* file exists */

				/* determine size of classfile */

				/* dolog("File: %s",filename); */

				err = stat(currPos->filepath.filename, &buffer);

				if (!err) {                                /* read classfile data */				
					classbuffer_size = buffer.st_size;				
					classbuffer      = MNEW(u1, classbuffer_size);
					classbuf_pos     = classbuffer - 1;
					fread(classbuffer, 1, classbuffer_size, classfile);
					fclose(classfile);
					return true;
				}
			}
#ifdef USE_ZLIB
		}
#endif
	}

	if (verbose) {
		dolog("Warning: Can not open class file '%s'", filename);
	}

	return false;

}

/************************** function suck_stop *********************************

	frees memory for buffer with classfile data.
	Caution: this function may only be called if buffer has been allocated
	         by suck_start with reading a file
	
*******************************************************************************/

void suck_stop()
{
	/* determine amount of classdata not retrieved by suck-operations         */

	int classdata_left = ((classbuffer + classbuffer_size) - classbuf_pos - 1);

	if (classdata_left > 0) {
		/* surplus */        	
		dolog("There are %d extra bytes at end of classfile",
				classdata_left);
		/* The JVM spec disallows extra bytes. */
		panic("Extra bytes at end of classfile");
	}

	/* free memory */

	MFREE(classbuffer, u1, classbuffer_size);
	classbuffer = NULL;
}


/******************************************************************************/
/******************* Some support functions ***********************************/
/******************************************************************************/

void fprintflags (FILE *fp, u2 f)
{
   if ( f & ACC_PUBLIC )       fprintf (fp," PUBLIC");
   if ( f & ACC_PRIVATE )      fprintf (fp," PRIVATE");
   if ( f & ACC_PROTECTED )    fprintf (fp," PROTECTED");
   if ( f & ACC_STATIC )       fprintf (fp," STATIC");
   if ( f & ACC_FINAL )        fprintf (fp," FINAL");
   if ( f & ACC_SYNCHRONIZED ) fprintf (fp," SYNCHRONIZED");
   if ( f & ACC_VOLATILE )     fprintf (fp," VOLATILE");
   if ( f & ACC_TRANSIENT )    fprintf (fp," TRANSIENT");
   if ( f & ACC_NATIVE )       fprintf (fp," NATIVE");
   if ( f & ACC_INTERFACE )    fprintf (fp," INTERFACE");
   if ( f & ACC_ABSTRACT )     fprintf (fp," ABSTRACT");
}


/********** internal function: printflags  (only for debugging) ***************/

void printflags(u2 f)
{
   if ( f & ACC_PUBLIC )       printf (" PUBLIC");
   if ( f & ACC_PRIVATE )      printf (" PRIVATE");
   if ( f & ACC_PROTECTED )    printf (" PROTECTED");
   if ( f & ACC_STATIC )       printf (" STATIC");
   if ( f & ACC_FINAL )        printf (" FINAL");
   if ( f & ACC_SYNCHRONIZED ) printf (" SYNCHRONIZED");
   if ( f & ACC_VOLATILE )     printf (" VOLATILE");
   if ( f & ACC_TRANSIENT )    printf (" TRANSIENT");
   if ( f & ACC_NATIVE )       printf (" NATIVE");
   if ( f & ACC_INTERFACE )    printf (" INTERFACE");
   if ( f & ACC_ABSTRACT )     printf (" ABSTRACT");
}


/************************* Function: skipattribute *****************************

	skips a (1) 'attribute' structure in the class file

*******************************************************************************/

static void skipattribute()
{
	u4 len;
	suck_u2 ();
	len = suck_u4();
	skip_nbytes(len);	
}


/********************** Function: skipattributebody ****************************

	skips an attribute after the 16 bit reference to attribute_name has already
	been read
	
*******************************************************************************/

static void skipattributebody()
{
	u4 len;
	len = suck_u4();
	skip_nbytes(len);
}


/************************* Function: skipattributes ****************************

	skips num attribute structures
	
*******************************************************************************/

static void skipattributes(u4 num)
{
	u4 i;
	for (i = 0; i < num; i++)
		skipattribute();
}


/******************** function: innerclass_getconstant ************************

    like class_getconstant, but if cptags is ZERO null is returned					 
	
*******************************************************************************/

voidptr innerclass_getconstant(classinfo *c, u4 pos, u4 ctype) 
{
	/* invalid position in constantpool */
	if (pos >= c->cpcount)
		panic("Attempt to access constant outside range");

	/* constantpool entry of type 0 */	
	if (!c->cptags[pos])
		return NULL;

	/* check type of constantpool entry */
	if (c->cptags[pos] != ctype) {
		error("Type mismatch on constant: %d requested, %d here (innerclass_getconstant)",
			  (int) ctype, (int) c->cptags[pos] );
	}
		
	return c->cpinfos[pos];
}


/************************ function: attribute_load ****************************

    read attributes from classfile
	
*******************************************************************************/

static void attribute_load(u4 num, classinfo *c)
{
	u4 i, j;

	for (i = 0; i < num; i++) {
		/* retrieve attribute name */
		utf *aname = class_getconstant(c, suck_u2(), CONSTANT_Utf8);

		if (aname == utf_innerclasses) {
			/* innerclasses attribute */

			if (c->innerclass != NULL)
				panic("Class has more than one InnerClasses attribute");
				
			/* skip attribute length */						
			suck_u4(); 
			/* number of records */
			c->innerclasscount = suck_u2();
			/* allocate memory for innerclass structure */
			c->innerclass = MNEW(innerclassinfo, c->innerclasscount);

			for (j = 0; j < c->innerclasscount; j++) {
				/*  The innerclass structure contains a class with an encoded name, 
				    its defining scope, its simple name  and a bitmask of the access flags. 
				    If an inner class is not a member, its outer_class is NULL, 
				    if a class is anonymous, its name is NULL.  			    */
   								
				innerclassinfo *info = c->innerclass + j;

				info->inner_class = innerclass_getconstant(c, suck_u2(), CONSTANT_Class); /* CONSTANT_Class_info index */
				info->outer_class = innerclass_getconstant(c, suck_u2(), CONSTANT_Class); /* CONSTANT_Class_info index */
				info->name  = innerclass_getconstant(c, suck_u2(), CONSTANT_Utf8);        /* CONSTANT_Utf8_info index  */
				info->flags = suck_u2();					          /* access_flags bitmask      */
			}

		} else {
			/* unknown attribute */
			skipattributebody();
		}
	}
}


/******************* function: checkfielddescriptor ****************************

	checks whether a field-descriptor is valid and aborts otherwise
	all referenced classes are inserted into the list of unloaded classes
	
*******************************************************************************/

static void checkfielddescriptor (char *utf_ptr, char *end_pos)
{
	class_from_descriptor(utf_ptr,end_pos,NULL,
						  CLASSLOAD_NEW
						  | CLASSLOAD_NULLPRIMITIVE
						  | CLASSLOAD_NOVOID
						  | CLASSLOAD_CHECKEND);
	
	/* XXX use the following if -noverify */
#if 0
	char *tstart;  /* pointer to start of classname */
	char ch;
	char *start = utf_ptr;

	switch (*utf_ptr++) {
	  case 'B':
	  case 'C':
	  case 'I':
	  case 'S':
	  case 'Z':  
	  case 'J':  
	  case 'F':  
	  case 'D':
		  /* primitive type */  
		  break;
		  
	  case '[':
	  case 'L':
		  if (!class_from_descriptor(start,end_pos,&utf_ptr,CLASSLOAD_NEW))
			  panic ("Ill formed descriptor");
		  break;
		  
	  default:   
		  panic ("Ill formed descriptor");
	}			
	
	/* exceeding characters */        	
	if (utf_ptr!=end_pos) panic ("descriptor has exceeding chars");
#endif
}


/******************* function checkmethoddescriptor ****************************

    checks whether a method-descriptor is valid and aborts otherwise.
    All referenced classes are inserted into the list of unloaded classes.
	
*******************************************************************************/

static void checkmethoddescriptor (utf *d)
{
	char *utf_ptr = d->text;     /* current position in utf text   */
	char *end_pos = utf_end(d);  /* points behind utf string       */
	int argcount = 0;            /* number of arguments            */

	/* method descriptor must start with parenthesis */
	if (utf_ptr == end_pos || *utf_ptr++ != '(') panic ("Missing '(' in method descriptor");

    /* check arguments */
    while (utf_ptr != end_pos && *utf_ptr != ')') {
		/* XXX we cannot count the this argument here because
		 * we don't know if the method is static. */
		if (*utf_ptr == 'J' || *utf_ptr == 'D')
			argcount++;
		if (++argcount > 255)
			panic("Invalid method descriptor: too many arguments");
		class_from_descriptor(utf_ptr,end_pos,&utf_ptr,
							  CLASSLOAD_NEW
							  | CLASSLOAD_NULLPRIMITIVE
							  | CLASSLOAD_NOVOID);
	}

	if (utf_ptr == end_pos) panic("Missing ')' in method descriptor");
    utf_ptr++; /* skip ')' */

	class_from_descriptor(utf_ptr,end_pos,NULL,
						  CLASSLOAD_NEW
						  | CLASSLOAD_NULLPRIMITIVE
						  | CLASSLOAD_CHECKEND);

	/* XXX use the following if -noverify */
#if 0
	/* XXX check length */
	/* check arguments */
	while ((c = *utf_ptr++) != ')') {
		start = utf_ptr-1;
		
		switch (c) {
		case 'B':
		case 'C':
		case 'I':
		case 'S':
		case 'Z':  
		case 'J':  
		case 'F':  
		case 'D':
			/* primitive type */  
			break;

		case '[':
		case 'L':
			if (!class_from_descriptor(start,end_pos,&utf_ptr,CLASSLOAD_NEW))
				panic ("Ill formed method descriptor");
			break;
			
		default:   
			panic ("Ill formed methodtype-descriptor");
		}
	}

	/* check returntype */
	if (*utf_ptr=='V') {
		/* returntype void */
		if ((utf_ptr+1) != end_pos) panic ("Method-descriptor has exceeding chars");
	}
	else
		/* treat as field-descriptor */
		checkfielddescriptor (utf_ptr,end_pos);
#endif
}


/***************** Function: print_arraydescriptor ****************************

	Debugging helper for displaying an arraydescriptor
	
*******************************************************************************/

void print_arraydescriptor(FILE *file, arraydescriptor *desc)
{
	if (!desc) {
		fprintf(file, "<NULL>");
		return;
	}

	fprintf(file, "{");
	if (desc->componentvftbl) {
		if (desc->componentvftbl->class)
			utf_fprint(file, desc->componentvftbl->class->name);
		else
			fprintf(file, "<no classinfo>");
	}
	else
		fprintf(file, "0");
		
	fprintf(file, ",");
	if (desc->elementvftbl) {
		if (desc->elementvftbl->class)
			utf_fprint(file, desc->elementvftbl->class->name);
		else
			fprintf(file, "<no classinfo>");
	}
	else
		fprintf(file, "0");
	fprintf(file, ",%d,%d,%d,%d}", desc->arraytype, desc->dimension,
			desc->dataoffset, desc->componentsize);
}


/******************************************************************************/
/**************************  Functions for fields  ****************************/
/******************************************************************************/


/************************ Function: field_load *********************************

	Load everything about a class field from the class file and fill a
	'fieldinfo' structure. For static fields, space in the data segment is
	allocated.

*******************************************************************************/

#define field_load_NOVALUE  0xffffffff /* must be bigger than any u2 value! */

static void field_load(fieldinfo *f, classinfo *c)
{
	u4 attrnum,i;
	u4 jtype;
	u4 pindex = field_load_NOVALUE;                               /* constantvalue_index */

	f->flags = suck_u2();                                           /* ACC flags         */
	f->name = class_getconstant(c, suck_u2(), CONSTANT_Utf8);       /* name of field     */
	f->descriptor = class_getconstant(c, suck_u2(), CONSTANT_Utf8); /* JavaVM descriptor */
	f->type = jtype = desc_to_type(f->descriptor);		            /* data type         */
	f->offset = 0;						                  /* offset from start of object */
	f->class = c;
	f->xta = NULL;
	
	/* check flag consistency */
	if (opt_verify) {
		i = (f->flags & (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED));
		if (i != 0 && i != ACC_PUBLIC && i != ACC_PRIVATE && i != ACC_PROTECTED)
			panic("Field has invalid access flags");
		if ((f->flags & (ACC_FINAL | ACC_VOLATILE)) == (ACC_FINAL | ACC_VOLATILE))
			panic("Field is declared final and volatile");
		if ((c->flags & ACC_INTERFACE) != 0) {
			if ((f->flags & (ACC_STATIC | ACC_PUBLIC | ACC_FINAL))
				!= (ACC_STATIC | ACC_PUBLIC | ACC_FINAL))
				panic("Interface field is not declared static final public");
		}
	}
		
	switch (f->type) {
	case TYPE_INT:        f->value.i = 0; break;
	case TYPE_FLOAT:      f->value.f = 0.0; break;
	case TYPE_DOUBLE:     f->value.d = 0.0; break;
	case TYPE_ADDRESS:    f->value.a = NULL; break;
	case TYPE_LONG:
#if U8_AVAILABLE
		f->value.l = 0; break;
#else
		f->value.l.low = 0; f->value.l.high = 0; break;
#endif
	}

	/* read attributes */
	attrnum = suck_u2();
	for (i = 0; i < attrnum; i++) {
		utf *aname;

		aname = class_getconstant(c, suck_u2(), CONSTANT_Utf8);
		
		if (aname != utf_constantvalue) {
			/* unknown attribute */
			skipattributebody();

		} else {
			/* constant value attribute */

			if (pindex != field_load_NOVALUE)
				panic("Field has more than one ConstantValue attribute");
			
			/* check attribute length */
			if (suck_u4() != 2)
				panic("ConstantValue attribute has invalid length");
			
			/* index of value in constantpool */		
			pindex = suck_u2();
		
			/* initialize field with value from constantpool */		
			switch (jtype) {
			case TYPE_INT: {
				constant_integer *ci = 
					class_getconstant(c, pindex, CONSTANT_Integer);
				f->value.i = ci->value;
			}
			break;
					
			case TYPE_LONG: {
				constant_long *cl = 
					class_getconstant(c, pindex, CONSTANT_Long);
				f->value.l = cl->value;
			}
			break;

			case TYPE_FLOAT: {
				constant_float *cf = 
					class_getconstant(c, pindex, CONSTANT_Float);
				f->value.f = cf->value;
			}
			break;
											
			case TYPE_DOUBLE: {
				constant_double *cd = 
					class_getconstant(c, pindex, CONSTANT_Double);
				f->value.d = cd->value;
			}
			break;
						
			case TYPE_ADDRESS: { 
				utf *u = class_getconstant(c, pindex, CONSTANT_String);
				/* create javastring from compressed utf8-string */					
				f->value.a = literalstring_new(u);
			}
			break;
	
			default: 
				log_text ("Invalid Constant - Type");
			}
		}
	}
}


/********************** function: field_free **********************************/

static void field_free (fieldinfo *f)
{
	/* empty */
}


/**************** Function: field_display (debugging only) ********************/

void field_display(fieldinfo *f)
{
	printf("   ");
	printflags(f->flags);
	printf(" ");
	utf_display(f->name);
	printf(" ");
	utf_display(f->descriptor);	
	printf(" offset: %ld\n", (long int) (f->offset));
}


/******************************************************************************/
/************************* Functions for methods ******************************/ 
/******************************************************************************/


/*********************** Function: method_load *********************************

	Loads a method from the class file and fills an existing 'methodinfo'
	structure. For native methods, the function pointer field is set to the
	real function pointer, for JavaVM methods a pointer to the compiler is used
	preliminarily.
	
*******************************************************************************/

static void method_load(methodinfo *m, classinfo *c)
{
	u4 attrnum, i, e;
	
#ifdef STATISTICS
	count_all_methods++;
#endif

	m->class = c;
	
	m->flags = suck_u2();
	m->name = class_getconstant(c, suck_u2(), CONSTANT_Utf8);
	m->descriptor = class_getconstant(c, suck_u2(), CONSTANT_Utf8);
	checkmethoddescriptor(m->descriptor);	

	/* check flag consistency */
	if (opt_verify) {
		/* XXX could check if <clinit> is STATIC */
		if (m->name != utf_clinit) {
			i = (m->flags & (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED));
			if (i != 0 && i != ACC_PUBLIC && i != ACC_PRIVATE && i != ACC_PROTECTED)
				panic("Method has invalid access flags");
			if ((m->flags & ACC_ABSTRACT) != 0) {
				if ((m->flags & (ACC_FINAL | ACC_NATIVE | ACC_PRIVATE | ACC_STATIC
								 | ACC_STRICT | ACC_SYNCHRONIZED)) != 0)
					panic("Abstract method has invalid flags set");
			}
			if ((c->flags & ACC_INTERFACE) != 0) {
				if ((m->flags & (ACC_ABSTRACT | ACC_PUBLIC))
					!= (ACC_ABSTRACT | ACC_PUBLIC))
					panic("Interface method is not declared abstract and public");
			}
			if (m->name == utf_init) {
				if ((m->flags & (ACC_STATIC | ACC_FINAL | ACC_SYNCHRONIZED
								 | ACC_NATIVE | ACC_ABSTRACT)) != 0)
					panic("Instance initialization method has invalid flags set");
			}
		}
	}
		
	m->jcode = NULL;
	m->exceptiontable = NULL;
	m->entrypoint = NULL;
	m->mcode = NULL;
	m->stubroutine = NULL;
	m->methodUsed = NOTUSED;    
	m->monoPoly = MONO;    
	m->subRedefs = 0;
	m->subRedefsUsed = 0;

	m->xta = NULL;
	
	if (!(m->flags & ACC_NATIVE)) {
		m->stubroutine = createcompilerstub(m);

	} else {
		functionptr f = native_findfunction(c->name, m->name, m->descriptor, 
											(m->flags & ACC_STATIC) != 0);
		if (f) {
			m->stubroutine = createnativestub(f, m);
		}
	}
	
	
	attrnum = suck_u2();
	for (i = 0; i < attrnum; i++) {
		utf *aname;

		aname = class_getconstant(c, suck_u2(), CONSTANT_Utf8);

		if (aname != utf_code) {
			skipattributebody();

		} else {
			u4 codelen;
			if ((m->flags & (ACC_ABSTRACT | ACC_NATIVE)) != 0)
				panic("Code attribute for native or abstract method");
			
			if (m->jcode)
				panic("Method has more than one Code attribute");

			suck_u4();
			m->maxstack = suck_u2();
			m->maxlocals = suck_u2();
			codelen = suck_u4();
			if (codelen == 0)
				panic("bytecode has zero length");
			if (codelen > 65536)
				panic("bytecode too long");
			m->jcodelength = codelen;
			m->jcode = MNEW(u1, m->jcodelength);
			suck_nbytes(m->jcode, m->jcodelength);
			m->exceptiontablelength = suck_u2();
			m->exceptiontable = 
				MNEW(exceptiontable, m->exceptiontablelength);

#ifdef STATISTICS
			count_vmcode_len += m->jcodelength + 18;
			count_extable_len += 8 * m->exceptiontablelength;
#endif

			for (e = 0; e < m->exceptiontablelength; e++) {
				u4 idx;
				m->exceptiontable[e].startpc = suck_u2();
				m->exceptiontable[e].endpc = suck_u2();
				m->exceptiontable[e].handlerpc = suck_u2();

				idx = suck_u2();
				if (!idx) {
					m->exceptiontable[e].catchtype = NULL;

				} else {
					m->exceptiontable[e].catchtype = 
				      class_getconstant(c, idx, CONSTANT_Class);
				}
			}			

			skipattributes(suck_u2());
		}
	}

	if (!m->jcode && (m->flags & (ACC_ABSTRACT | ACC_NATIVE)) == 0)
		panic("Method missing Code attribute");
}


/********************* Function: method_free ***********************************

	frees all memory that was allocated for this method

*******************************************************************************/

static void method_free(methodinfo *m)
{
	if (m->jcode)
		MFREE(m->jcode, u1, m->jcodelength);

	if (m->exceptiontable)
		MFREE(m->exceptiontable, exceptiontable, m->exceptiontablelength);

	if (m->mcode)
		CFREE(m->mcode, m->mcodelength);

	if (m->stubroutine) {
		if (m->flags & ACC_NATIVE) {
			removenativestub(m->stubroutine);

		} else {
			removecompilerstub(m->stubroutine);
		}
	}
}


/************** Function: method_display  (debugging only) **************/

void method_display(methodinfo *m)
{
	printf("   ");
	printflags(m->flags);
	printf(" ");
	utf_display(m->name);
	printf(" "); 
	utf_display(m->descriptor);
	printf("\n");
}


/******************** Function: method_canoverwrite ****************************

	Check if m and old are identical with respect to type and name. This means
	that old can be overwritten with m.
	
*******************************************************************************/  

static bool method_canoverwrite (methodinfo *m, methodinfo *old)
{
	if (m->name != old->name) return false;
	if (m->descriptor != old->descriptor) return false;
	if (m->flags & ACC_STATIC) return false;
	return true;
}




/******************************************************************************/
/************************ Functions for class *********************************/
/******************************************************************************/


/******************** function:: class_getconstant ******************************

	retrieves the value at position 'pos' of the constantpool of a class
	if the type of the value is other than 'ctype' the system is stopped

*******************************************************************************/

voidptr class_getconstant(classinfo *c, u4 pos, u4 ctype)
{
	/* invalid position in constantpool */
	/* (pos == 0 is caught by type comparison) */
	if (pos >= c->cpcount)
		panic("Attempt to access constant outside range");

	/* check type of constantpool entry */

	if (c->cptags[pos] != ctype) {
		class_showconstantpool(c);
		error("Type mismatch on constant: %d requested, %d here (class_getconstant)",
			  (int) ctype, (int) c->cptags[pos]);
	}
		
	return c->cpinfos[pos];
}


/********************* Function: class_constanttype ****************************

	Determines the type of a class entry in the ConstantPool
	
*******************************************************************************/

u4 class_constanttype(classinfo *c, u4 pos)
{
	if (pos >= c->cpcount)
		panic("Attempt to access constant outside range");

	return c->cptags[pos];
}


/******************** function: class_loadcpool ********************************

	loads the constantpool of a class, 
	the entries are transformed into a simpler format 
	by resolving references
	(a detailed overview of the compact structures can be found in global.h)	

*******************************************************************************/

static void class_loadcpool(classinfo *c)
{

	/* The following structures are used to save information which cannot be 
	   processed during the first pass. After the complete constantpool has 
	   been traversed the references can be resolved. 
	   (only in specific order)						   */
	
	/* CONSTANT_Class_info entries */
	typedef struct forward_class {      
		struct forward_class *next; 
		u2 thisindex;               
		u2 name_index;              
	} forward_class;                    
                                        
	/* CONSTANT_String */                                      
	typedef struct forward_string {	
		struct forward_string *next; 
		u2 thisindex;	             
		u2 string_index;
	} forward_string;

	/* CONSTANT_NameAndType */
	typedef struct forward_nameandtype {
		struct forward_nameandtype *next;
		u2 thisindex;
		u2 name_index;
		u2 sig_index;
	} forward_nameandtype;

	/* CONSTANT_Fieldref, CONSTANT_Methodref or CONSTANT_InterfaceMethodref */
	typedef struct forward_fieldmethint {	
		struct forward_fieldmethint *next;
		u2 thisindex;
		u1 tag;
		u2 class_index;
		u2 nameandtype_index;
	} forward_fieldmethint;


	u4 idx;
	long int dumpsize = dump_size ();

	forward_class *forward_classes = NULL;
	forward_string *forward_strings = NULL;
	forward_nameandtype *forward_nameandtypes = NULL;
	forward_fieldmethint *forward_fieldmethints = NULL;

	/* number of entries in the constant_pool table plus one */
	u4 cpcount       = c -> cpcount = suck_u2();
	/* allocate memory */
	u1 *cptags       = c -> cptags  = MNEW (u1, cpcount);
	voidptr *cpinfos = c -> cpinfos = MNEW (voidptr, cpcount);

	if (!cpcount)
		panic("Invalid constant_pool_count (0)");
	
#ifdef STATISTICS
	count_const_pool_len += (sizeof(voidptr) + 1) * cpcount;
#endif
	
	/* initialize constantpool */
	for (idx=0; idx<cpcount; idx++) {
		cptags[idx] = CONSTANT_UNUSED;
		cpinfos[idx] = NULL;
		}

			
		/******* first pass *******/
		/* entries which cannot be resolved now are written into 
		   temporary structures and traversed again later        */
		   
	idx = 1;
	while (idx < cpcount) {
		/* get constant type */
		u4 t = suck_u1 (); 
		switch ( t ) {

			case CONSTANT_Class: { 
				forward_class *nfc = DNEW(forward_class);

				nfc -> next = forward_classes; 											
				forward_classes = nfc;

				nfc -> thisindex = idx;
				/* reference to CONSTANT_NameAndType */
				nfc -> name_index = suck_u2 (); 

				idx++;
				break;
				}
			
			case CONSTANT_Fieldref:
			case CONSTANT_Methodref:
			case CONSTANT_InterfaceMethodref: { 
				forward_fieldmethint *nff = DNEW (forward_fieldmethint);
				
				nff -> next = forward_fieldmethints;
				forward_fieldmethints = nff;

				nff -> thisindex = idx;
				/* constant type */
				nff -> tag = t;
				/* class or interface type that contains the declaration of the field or method */
				nff -> class_index = suck_u2 (); 
				/* name and descriptor of the field or method */
				nff -> nameandtype_index = suck_u2 ();

				idx ++;
				break;
				}
				
			case CONSTANT_String: {
				forward_string *nfs = DNEW (forward_string);
				
				nfs -> next = forward_strings;
				forward_strings = nfs;
				
				nfs -> thisindex = idx;
				/* reference to CONSTANT_Utf8_info with string characters */
				nfs -> string_index = suck_u2 ();
				
				idx ++;
				break;
				}

			case CONSTANT_NameAndType: {
				forward_nameandtype *nfn = DNEW (forward_nameandtype);
				
				nfn -> next = forward_nameandtypes;
				forward_nameandtypes = nfn;
				
				nfn -> thisindex = idx;
				/* reference to CONSTANT_Utf8_info containing simple name */
				nfn -> name_index = suck_u2 ();
				/* reference to CONSTANT_Utf8_info containing field or method descriptor */
				nfn -> sig_index = suck_u2 ();
				
				idx ++;
				break;
				}

			case CONSTANT_Integer: {
				constant_integer *ci = NEW (constant_integer);

#ifdef STATISTICS
	count_const_pool_len += sizeof(constant_integer);
#endif

				ci -> value = suck_s4 ();
				cptags [idx] = CONSTANT_Integer;
				cpinfos [idx] = ci;
				idx ++;
				
				break;
				}
				
			case CONSTANT_Float: {
				constant_float *cf = NEW (constant_float);

#ifdef STATISTICS
	count_const_pool_len += sizeof(constant_float);
#endif

				cf -> value = suck_float ();
				cptags [idx] = CONSTANT_Float;
				cpinfos[idx] = cf;
				idx ++;
				break;
				}
				
			case CONSTANT_Long: {
				constant_long *cl = NEW(constant_long);
					
#ifdef STATISTICS
	count_const_pool_len += sizeof(constant_long);
#endif

				cl -> value = suck_s8 ();
				cptags [idx] = CONSTANT_Long;
				cpinfos [idx] = cl;
				idx += 2;
				if (idx > cpcount)
					panic("Long constant exceeds constant pool");
				break;
				}
			
			case CONSTANT_Double: {
				constant_double *cd = NEW(constant_double);
				
#ifdef STATISTICS
	count_const_pool_len += sizeof(constant_double);
#endif

				cd -> value = suck_double ();
				cptags [idx] = CONSTANT_Double;
				cpinfos [idx] = cd;
				idx += 2;
				if (idx > cpcount)
					panic("Double constant exceeds constant pool");
				break;
				}
				
			case CONSTANT_Utf8: { 

				/* number of bytes in the bytes array (not string-length) */
				u4 length = suck_u2();
				cptags [idx]  = CONSTANT_Utf8;
				/* validate the string */
				ASSERT_LEFT(length);
				if (opt_verify &&
					!is_valid_utf(classbuf_pos+1, classbuf_pos+1+length))
					panic("Invalid UTF-8 string"); 
				/* insert utf-string into the utf-symboltable */
				cpinfos [idx] = utf_new(classbuf_pos+1, length);
				/* skip bytes of the string */
				skip_nbytes(length);
				idx++;
				break;
				}
										
			default:
				error ("Unkown constant type: %d",(int) t);
		
			}  /* end switch */
			
		} /* end while */
		


	   /* resolve entries in temporary structures */

	while (forward_classes) {
		utf *name =
		  class_getconstant (c, forward_classes -> name_index, CONSTANT_Utf8);

		cptags  [forward_classes -> thisindex] = CONSTANT_Class;
		/* retrieve class from class-table */
		cpinfos [forward_classes -> thisindex] = class_new (name);

		forward_classes = forward_classes -> next;
		
		}

	while (forward_strings) {
		utf *text = 
		  class_getconstant (c, forward_strings -> string_index, CONSTANT_Utf8);
/*
		log_text("forward_string:");
		printf(	"classpoolid: %d\n",forward_strings -> thisindex);
		utf_display(text);
		log_text("\n------------------"); */
		/* resolve utf-string */		
		cptags   [forward_strings -> thisindex] = CONSTANT_String;
		cpinfos  [forward_strings -> thisindex] = text;
		
		forward_strings = forward_strings -> next;
		}	

	while (forward_nameandtypes) {
		constant_nameandtype *cn = NEW (constant_nameandtype);	

#ifdef STATISTICS
		count_const_pool_len += sizeof(constant_nameandtype);
#endif

		/* resolve simple name and descriptor */
		cn -> name = class_getconstant 
		   (c, forward_nameandtypes -> name_index, CONSTANT_Utf8);
		cn -> descriptor = class_getconstant
		   (c, forward_nameandtypes -> sig_index, CONSTANT_Utf8);
		 
		cptags   [forward_nameandtypes -> thisindex] = CONSTANT_NameAndType;
		cpinfos  [forward_nameandtypes -> thisindex] = cn;
		
		forward_nameandtypes = forward_nameandtypes -> next;
		}


	while (forward_fieldmethints)  {
		constant_nameandtype *nat;
		constant_FMIref *fmi = NEW (constant_FMIref);

#ifdef STATISTICS
		count_const_pool_len += sizeof(constant_FMIref);
#endif
		/* resolve simple name and descriptor */
		nat = class_getconstant
			(c, forward_fieldmethints -> nameandtype_index, CONSTANT_NameAndType);

#ifdef JOWENN_DEBUG
		log_text("trying to resolve:");
		log_text(nat->name->text);
		switch(forward_fieldmethints ->tag) {
		case CONSTANT_Fieldref: 
				log_text("CONSTANT_Fieldref");
				break;
		case CONSTANT_InterfaceMethodref: 
				log_text("CONSTANT_InterfaceMethodref");
				break;
		case CONSTANT_Methodref: 
				log_text("CONSTANT_Methodref");
		                break;
		}
#endif
		fmi -> class = class_getconstant 
			(c, forward_fieldmethints -> class_index, CONSTANT_Class);
		fmi -> name = nat -> name;
		fmi -> descriptor = nat -> descriptor;

		cptags [forward_fieldmethints -> thisindex] = forward_fieldmethints -> tag;
		cpinfos [forward_fieldmethints -> thisindex] = fmi;
	
		switch (forward_fieldmethints -> tag) {
		case CONSTANT_Fieldref:  /* check validity of descriptor */
					 checkfielddescriptor (fmi->descriptor->text,utf_end(fmi->descriptor));
		                         break;
		case CONSTANT_InterfaceMethodref: 
		case CONSTANT_Methodref: /* check validity of descriptor */
			/* XXX check special names (<init>) */
					 checkmethoddescriptor (fmi->descriptor);
		                         break;
		}		
	
		forward_fieldmethints = forward_fieldmethints -> next;

		}

/*	class_showconstantpool(c); */

	dump_release (dumpsize);
}


/********************** Function: class_load ***********************************
	
	Loads everything interesting about a class from the class file. The
	'classinfo' structure must have been allocated previously.

	The super class and the interfaces implemented by this class need not be
	loaded. The link is set later by the function 'class_link'.

	The loaded class is removed from the list 'unloadedclasses' and added to
	the list 'unlinkedclasses'.
	
*******************************************************************************/

static int class_load(classinfo *c)
{
	u4 i;
	u4 mi,ma;

#ifdef STATISTICS
	count_class_loads++;
#endif

	/* output for debugging purposes */
	if (loadverbose) {		
		char logtext[MAXLOGTEXT];
		sprintf(logtext, "Loading class: ");
		utf_sprint(logtext + strlen(logtext), c->name);
		log_text(logtext);
	}
	
	/* load classdata, throw exception on error */

	if (!suck_start(c->name)) {
		throw_noclassdeffounderror_message(c->name);
		return false;
	}
	
	/* check signature */		
	if (suck_u4() != MAGIC) panic("Can not find class-file signature");   	
	/* check version */
	mi = suck_u2(); 
	ma = suck_u2();
	if (ma != MAJOR_VERSION && (ma != MAJOR_VERSION+1 || mi != 0)) {
		error("File version %d.%d is not supported", (int) ma, (int) mi);
	}

	class_loadcpool(c);
	/*JOWENN*/
	c->erroneous_state = 0;
	c->initializing_thread = 0;	
	/*JOWENN*/
	c->classUsed = NOTUSED; /* not used initially CO-RT */
	c->impldBy = NULL;

	/* ACC flags */
	c->flags = suck_u2(); 
	/*if (!(c->flags & ACC_PUBLIC)) { log_text("CLASS NOT PUBLIC"); } JOWENN*/

	/* check ACC flags consistency */
	if ((c->flags & ACC_INTERFACE) != 0) {
		if ((c->flags & ACC_ABSTRACT) == 0)
			panic("Interface class not declared abstract");
		if ((c->flags & (ACC_FINAL | ACC_SUPER)) != 0)
			panic("Interface class has invalid flags");
	}

	/* this class */
	i = suck_u2();
	if (class_getconstant(c, i, CONSTANT_Class) != c)
		panic("Invalid this_class in class file");
	
	/* retrieve superclass */
	if ((i = suck_u2())) {
		c->super = class_getconstant(c, i, CONSTANT_Class);

		/* java.lang.Object may not have a super class. */
		if (c->name == utf_java_lang_Object)
			panic("java.lang.Object with super class");

		/* Interfaces must have j.l.O as super class. */
		if ((c->flags & ACC_INTERFACE) != 0
			&& c->super->name != utf_java_lang_Object)
		{
			panic("Interface with super class other than java.lang.Object");
		}
	} else {
		c->super = NULL;

		/* This is only allowed for java.lang.Object. */
		if (c->name != utf_java_lang_Object)
			panic("Class (not java.lang.Object) without super class");
	}
			 
	/* retrieve interfaces */
	c->interfacescount = suck_u2();
	c->interfaces = MNEW(classinfo*, c->interfacescount);
	for (i = 0; i < c->interfacescount; i++) {
		c->interfaces [i] = 
			class_getconstant(c, suck_u2(), CONSTANT_Class);
	}

	/* load fields */
	c->fieldscount = suck_u2();
/*	utf_display(c->name);
	printf(" ,Fieldscount: %d\n",c->fieldscount);*/

	c->fields = GCNEW(fieldinfo, c->fieldscount);
	for (i = 0; i < c->fieldscount; i++) {
		field_load(&(c->fields[i]), c);
	}

	/* load methods */
	c->methodscount = suck_u2();
	c->methods = MNEW(methodinfo, c->methodscount);
	for (i = 0; i < c->methodscount; i++) {
		method_load(&(c->methods[i]), c);
	}

#ifdef STATISTICS
	count_class_infos += sizeof(classinfo*) * c->interfacescount;
	count_class_infos += sizeof(fieldinfo) * c->fieldscount;
	count_class_infos += sizeof(methodinfo) * c->methodscount;
#endif

	/* load variable-length attribute structures */	
	attribute_load(suck_u2(), c);

	/* free memory */
	suck_stop();

	/* remove class from list of unloaded classes and 
	   add to list of unlinked classes	          */
	list_remove(&unloadedclasses, c);
	list_addlast(&unlinkedclasses, c);

	c->loaded = true;

	return true;
}



/************** internal Function: class_highestinterface ***********************

	Used by the function class_link to determine the amount of memory needed
	for the interface table.

*******************************************************************************/

static s4 class_highestinterface(classinfo *c) 
{
	s4 h;
	s4 i;
	
	if (!(c->flags & ACC_INTERFACE)) {
		char logtext[MAXLOGTEXT];
	  	sprintf(logtext, "Interface-methods count requested for non-interface:  ");
    	utf_sprint(logtext + strlen(logtext), c->name);
    	error("%s",logtext);
	}
    
    h = c->index;
	for (i = 0; i < c->interfacescount; i++) {
		s4 h2 = class_highestinterface(c->interfaces[i]);
		if (h2 > h) h = h2;
	}

	return h;
}


/* class_addinterface **********************************************************

	Is needed by class_link for adding a VTBL to a class. All interfaces
	implemented by ic are added as well.

*******************************************************************************/	

static void class_addinterface (classinfo *c, classinfo *ic)
{
	s4     j, m;
	s4     i     = ic->index;
	vftbl *vftbl = c->vftbl;
	
	if (i >= vftbl->interfacetablelength)
		panic ("Inernal error: interfacetable overflow");
	if (vftbl->interfacetable[-i])
		return;

	if (ic->methodscount == 0) {  /* fake entry needed for subtype test */
		vftbl->interfacevftbllength[i] = 1;
		vftbl->interfacetable[-i] = MNEW(methodptr, 1);
		vftbl->interfacetable[-i][0] = NULL;
		}
	else {
		vftbl->interfacevftbllength[i] = ic->methodscount;
		vftbl->interfacetable[-i] = MNEW(methodptr, ic->methodscount); 

#ifdef STATISTICS
	count_vftbl_len += sizeof(methodptr) *
	                         (ic->methodscount + (ic->methodscount == 0));
#endif

		for (j=0; j<ic->methodscount; j++) {
			classinfo *sc = c;
			while (sc) {
				for (m = 0; m < sc->methodscount; m++) {
					methodinfo *mi = &(sc->methods[m]);
					if (method_canoverwrite(mi, &(ic->methods[j]))) {
						vftbl->interfacetable[-i][j] = 
						                          vftbl->table[mi->vftblindex];
						goto foundmethod;
						}
					}
				sc = sc->super;
				}
			 foundmethod: ;
			}
		}

	for (j = 0; j < ic->interfacescount; j++) 
		class_addinterface(c, ic->interfaces[j]);
}


/******************* Function: class_new_array *********************************

    This function is called by class_new to setup an array class.

*******************************************************************************/

void class_new_array(classinfo *c)
{
	classinfo *comp = NULL;
	methodinfo *clone;
	int namelen;

	/* XXX remove */ /* dolog("class_new_array: %s",c->name->text); */

	/* Array classes are not loaded from classfiles. */
	list_remove(&unloadedclasses, c);

	/* Check array class name */
	namelen = c->name->blength;
	if (namelen < 2 || c->name->text[0] != '[')
		panic("Invalid array class name");

	/* Check the component type */
	switch (c->name->text[1]) {
	  case '[':
		  /* c is an array of arrays. We have to create the component class. */
		  comp = class_new(utf_new(c->name->text + 1,namelen - 1));
		  break;

	  case 'L':
		  /* c is an array of objects. */
		  if (namelen < 4 || c->name->text[namelen - 1] != ';')
			  panic("Invalid array class name");
		  comp = class_new(utf_new(c->name->text + 2,namelen - 3));
		  break;
	}

	/* Setup the array class */
	c->super = class_java_lang_Object;
	c->flags = ACC_PUBLIC | ACC_FINAL | ACC_ABSTRACT;

    c->interfacescount = 2;
    c->interfaces = MNEW(classinfo*,2); /* XXX use GC? */
    c->interfaces[0] = class_java_lang_Cloneable;
    c->interfaces[1] = class_java_io_Serializable;

	c->methodscount = 1;
	c->methods = MNEW (methodinfo, c->methodscount); /* XXX use GC? */

	clone = c->methods;
	memset(clone, 0, sizeof(methodinfo));
	clone->flags = ACC_PUBLIC; /* XXX protected? */
	clone->name = utf_new_char("clone");
	clone->descriptor = utf_new_char("()Ljava/lang/Object;");
	clone->class = c;
	clone->stubroutine = createnativestub((functionptr) &builtin_clone_array, clone);
	clone->monoPoly = MONO; /* XXX should be poly? */

	/* XXX: field: length? */

	/* The array class has to be linked */
	list_addlast(&unlinkedclasses,c);
	
	/*
     * Array classes which are created after the other classes have been
     * loaded and linked are linked explicitely.
     */
        c->loaded=true;
     
	if (loader_inited)
		loader_load(c->name); /* XXX handle errors */
}


/****************** Function: class_link_array *********************************

    This function is called by class_link to create the
    arraydescriptor for an array class.

    This function returns NULL if the array cannot be linked because
    the component type has not been linked yet.

*******************************************************************************/

static arraydescriptor *class_link_array(classinfo *c)
{
	classinfo *comp = NULL;
	int namelen = c->name->blength;
	arraydescriptor *desc;
	vftbl *compvftbl;

	/* Check the component type */
	switch (c->name->text[1]) {
	  case '[':
		  /* c is an array of arrays. */
		  comp = class_get(utf_new(c->name->text + 1,namelen - 1));
		  if (!comp) panic("Could not find component array class.");
		  break;

	  case 'L':
		  /* c is an array of objects. */
		  comp = class_get(utf_new(c->name->text + 2,namelen - 3));
		  if (!comp) panic("Could not find component class.");
		  break;
	}

	/* If the component type has not been linked return NULL */
	if (comp && !comp->linked)
		return NULL;

	/* Allocate the arraydescriptor */
	desc = NEW(arraydescriptor);

	if (comp) {
		/* c is an array of references */
		desc->arraytype = ARRAYTYPE_OBJECT;
		desc->componentsize = sizeof(void*);
		desc->dataoffset = OFFSET(java_objectarray,data);
		
		compvftbl = comp->vftbl;
		if (!compvftbl)
			panic("Component class has no vftbl");
		desc->componentvftbl = compvftbl;
		
		if (compvftbl->arraydesc) {
			desc->elementvftbl = compvftbl->arraydesc->elementvftbl;
			if (compvftbl->arraydesc->dimension >= 255)
				panic("Creating array of dimension >255");
			desc->dimension = compvftbl->arraydesc->dimension + 1;
			desc->elementtype = compvftbl->arraydesc->elementtype;
		}
		else {
			desc->elementvftbl = compvftbl;
			desc->dimension = 1;
			desc->elementtype = ARRAYTYPE_OBJECT;
		}

	} else {
		/* c is an array of a primitive type */
		switch (c->name->text[1]) {
		case 'Z': desc->arraytype = ARRAYTYPE_BOOLEAN;
			desc->dataoffset = OFFSET(java_booleanarray,data);
			desc->componentsize = sizeof(u1);
			break;

		case 'B': desc->arraytype = ARRAYTYPE_BYTE;
			desc->dataoffset = OFFSET(java_bytearray,data);
			desc->componentsize = sizeof(u1);
			break;

		case 'C': desc->arraytype = ARRAYTYPE_CHAR;
			desc->dataoffset = OFFSET(java_chararray,data);
			desc->componentsize = sizeof(u2);
			break;

		case 'D': desc->arraytype = ARRAYTYPE_DOUBLE;
			desc->dataoffset = OFFSET(java_doublearray,data);
			desc->componentsize = sizeof(double);
			break;

		case 'F': desc->arraytype = ARRAYTYPE_FLOAT;
			desc->dataoffset = OFFSET(java_floatarray,data);
			desc->componentsize = sizeof(float);
			break;

		case 'I': desc->arraytype = ARRAYTYPE_INT;
			desc->dataoffset = OFFSET(java_intarray,data);
			desc->componentsize = sizeof(s4);
			break;

		case 'J': desc->arraytype = ARRAYTYPE_LONG;
			desc->dataoffset = OFFSET(java_longarray,data);
			desc->componentsize = sizeof(s8);
			break;

		case 'S': desc->arraytype = ARRAYTYPE_SHORT;
			desc->dataoffset = OFFSET(java_shortarray,data);
			desc->componentsize = sizeof(s2);
			break;

		default:
			panic("Invalid array class name");
		}
		
		desc->componentvftbl = NULL;
		desc->elementvftbl = NULL;
		desc->dimension = 1;
		desc->elementtype = desc->arraytype;
	}

	return desc;
}


/********************** Function: class_link ***********************************

	Tries to link a class. The super class and every implemented interface must
	already have been linked. The function calculates the length in bytes that
	an instance of this class requires as well as the VTBL for methods and
	interface methods.
	
	If the class can be linked, it is removed from the list 'unlinkedclasses'
	and added to 'linkedclasses'. Otherwise, it is moved to the end of
	'unlinkedclasses'.

	Attention: If cyclical class definitions are encountered, the program gets
	into an infinite loop (we'll have to work that out)

*******************************************************************************/

void class_link(classinfo *c)
{
	s4 supervftbllength;          /* vftbllegnth of super class               */
	s4 vftbllength;               /* vftbllength of current class             */
	s4 interfacetablelength;      /* interface table length                   */
	classinfo *super = c->super;  /* super class                              */
	classinfo *ic, *c2;           /* intermediate class variables             */
	vftbl *v;                     /* vftbl of current class                   */
	s4 i;                         /* interface/method/field counter           */
	arraydescriptor *arraydesc = NULL;  /* descriptor for array classes       */


	/*  check if all superclasses are already linked, if not put c at end of
	    unlinked list and return. Additionally initialize class fields.       */

	/*  check interfaces */

	for (i = 0; i < c->interfacescount; i++) {
		ic = c->interfaces[i];
		if (!ic->linked) {
			list_remove(&unlinkedclasses, c);
			list_addlast(&unlinkedclasses, c);
			return;	
		}
		if ((ic->flags & ACC_INTERFACE) == 0)
			panic("Specified interface is not declared as interface");
	}
	
	/*  check super class */

	if (super == NULL) {          /* class java.long.Object */
		c->index = 0;
        c->classUsed = USED;     /* Object class is always used CO-RT*/
		c->impldBy = NULL;
		c->instancesize = sizeof(java_objectheader);
		
		vftbllength = supervftbllength = 0;

		c->finalizer = NULL;

	} else {
		if (!super->linked) {
			list_remove(&unlinkedclasses, c);
			list_addlast(&unlinkedclasses, c);
			return;	
		}

		if ((super->flags & ACC_INTERFACE) != 0)
			panic("Interface specified as super class");

		/* handle array classes */
		/* The component class must have been linked already. */
		if (c->name->text[0] == '[')
			if ((arraydesc = class_link_array(c)) == NULL) {
				list_remove(&unlinkedclasses, c);
				list_addlast(&unlinkedclasses, c);
				return;	
			}

		/* Don't allow extending final classes */
		if ((super->flags & ACC_FINAL) != 0)
			panic("Trying to extend final class");
		
		if (c->flags & ACC_INTERFACE)
			c->index = interfaceindex++;
		else
			c->index = super->index + 1;
		
		c->instancesize = super->instancesize;
		
		vftbllength = supervftbllength = super->vftbl->vftbllength;
		
		c->finalizer = super->finalizer;
	}


	if (linkverbose) {
		char logtext[MAXLOGTEXT];
		sprintf(logtext, "Linking Class: ");
		utf_sprint(logtext + strlen(logtext), c->name );
		log_text(logtext);
	}

	/* compute vftbl length */

	for (i = 0; i < c->methodscount; i++) {
		methodinfo *m = &(c->methods[i]);
			
		if (!(m->flags & ACC_STATIC)) { /* is instance method */
			classinfo *sc = super;
			while (sc) {
				int j;
				for (j = 0; j < sc->methodscount; j++) {
					if (method_canoverwrite(m, &(sc->methods[j]))) {
						if ((sc->methods[j].flags & ACC_FINAL) != 0)
							panic("Trying to overwrite final method");
						m->vftblindex = sc->methods[j].vftblindex;
						goto foundvftblindex;
					}
				}
				sc = sc->super;
			}
			m->vftblindex = (vftbllength++);
		foundvftblindex: ;
		}
	}	
	
#ifdef STATISTICS
	count_vftbl_len += sizeof(vftbl) + (sizeof(methodptr) * (vftbllength - 1));
#endif

	/* compute interfacetable length */

	interfacetablelength = 0;
	c2 = c;
	while (c2) {
		for (i = 0; i < c2->interfacescount; i++) {
			s4 h = class_highestinterface (c2->interfaces[i]) + 1;
			if (h > interfacetablelength)
				interfacetablelength = h;
		}
		c2 = c2->super;
	}

	/* allocate virtual function table */

	v = (vftbl*) mem_alloc(sizeof(vftbl) + sizeof(methodptr) *
						   (vftbllength - 1) + sizeof(methodptr*) *
						   (interfacetablelength - (interfacetablelength > 0)));
	v = (vftbl*) (((methodptr*) v) + (interfacetablelength - 1) *
				  (interfacetablelength > 1));
	c->header.vftbl = c->vftbl = v;
	v->class = c;
	v->vftbllength = vftbllength;
	v->interfacetablelength = interfacetablelength;
	v->arraydesc = arraydesc;

	/* store interface index in vftbl */
	if (c->flags & ACC_INTERFACE)
		v->baseval = -(c->index);

	/* copy virtual function table of super class */

	for (i = 0; i < supervftbllength; i++) 
		v->table[i] = super->vftbl->table[i];
	
	/* add method stubs into virtual function table */

	for (i = 0; i < c->methodscount; i++) {
		methodinfo *m = &(c->methods[i]);
		if (!(m->flags & ACC_STATIC)) {
			v->table[m->vftblindex] = m->stubroutine;
		}
	}

	/* compute instance size and offset of each field */
	
	for (i = 0; i < c->fieldscount; i++) {
		s4 dsize;
		fieldinfo *f = &(c->fields[i]);
		
		if (!(f->flags & ACC_STATIC) ) {
			dsize = desc_typesize(f->descriptor);
			c->instancesize = ALIGN(c->instancesize, dsize);
			f->offset = c->instancesize;
			c->instancesize += dsize;
		}
	}

	/* initialize interfacetable and interfacevftbllength */
	
	v->interfacevftbllength = MNEW(s4, interfacetablelength);

#ifdef STATISTICS
	count_vftbl_len += (4 + sizeof(s4)) * v->interfacetablelength;
#endif

	for (i = 0; i < interfacetablelength; i++) {
		v->interfacevftbllength[i] = 0;
		v->interfacetable[-i] = NULL;
	}
	
	/* add interfaces */
	
	for (c2 = c; c2 != NULL; c2 = c2->super)
		for (i = 0; i < c2->interfacescount; i++) {
			class_addinterface(c, c2->interfaces[i]);
		}

	/* add finalizer method (not for java.lang.Object) */

	if (super != NULL) {
		methodinfo *fi;
		static utf *finame = NULL;
		static utf *fidesc = NULL;

		if (finame == NULL)
			finame = utf_finalize;
		if (fidesc == NULL)
			fidesc = utf_fidesc;

		fi = class_findmethod(c, finame, fidesc);
		if (fi != NULL) {
			if (!(fi->flags & ACC_STATIC)) {
				c->finalizer = fi;
			}
		}
	}

	/* final tasks */

	c->linked = true;	

	list_remove(&unlinkedclasses, c);
	list_addlast(&linkedclasses, c);
}


/******************* Function: class_freepool **********************************

	Frees all resources used by this classes Constant Pool.

*******************************************************************************/

static void class_freecpool (classinfo *c)
{
	u4 idx;
	u4 tag;
	voidptr info;
	
	for (idx=0; idx < c->cpcount; idx++) {
		tag = c->cptags[idx];
		info = c->cpinfos[idx];
		
		if (info != NULL) {
			switch (tag) {
			case CONSTANT_Fieldref:
			case CONSTANT_Methodref:
			case CONSTANT_InterfaceMethodref:
				FREE (info, constant_FMIref);
				break;
			case CONSTANT_Integer:
				FREE (info, constant_integer);
				break;
			case CONSTANT_Float:
				FREE (info, constant_float);
				break;
			case CONSTANT_Long:
				FREE (info, constant_long);
				break;
			case CONSTANT_Double:
				FREE (info, constant_double);
				break;
			case CONSTANT_NameAndType:
				FREE (info, constant_nameandtype);
				break;
			}
			}
		}

	MFREE (c -> cptags,  u1, c -> cpcount);
	MFREE (c -> cpinfos, voidptr, c -> cpcount);
}


/*********************** Function: class_free **********************************

	Frees all resources used by the class.

*******************************************************************************/

static void class_free(classinfo *c)
{
	s4 i;
	vftbl *v;
		
	class_freecpool(c);

	MFREE(c->interfaces, classinfo*, c->interfacescount);

	for (i = 0; i < c->fieldscount; i++)
		field_free(&(c->fields[i]));
	
	for (i = 0; i < c->methodscount; i++)
		method_free(&(c->methods[i]));
	MFREE(c->methods, methodinfo, c->methodscount);

	if ((v = c->vftbl) != NULL) {
		if (v->arraydesc)
			mem_free(v->arraydesc,sizeof(arraydescriptor));
		
		for (i = 0; i < v->interfacetablelength; i++) {
			MFREE(v->interfacetable[-i], methodptr, v->interfacevftbllength[i]);
		}
		MFREE(v->interfacevftbllength, s4, v->interfacetablelength);

		i = sizeof(vftbl) + sizeof(methodptr) * (v->vftbllength - 1) +
		    sizeof(methodptr*) * (v->interfacetablelength -
		                         (v->interfacetablelength > 0));
		v = (vftbl*) (((methodptr*) v) - (v->interfacetablelength - 1) *
	                                     (v->interfacetablelength > 1));
		mem_free(v, i);
	}

	if (c->innerclasscount)
		MFREE(c->innerclass, innerclassinfo, c->innerclasscount);

	/*	if (c->classvftbl)
		mem_free(c->header.vftbl, sizeof(vftbl) + sizeof(methodptr)*(c->vftbl->vftbllength-1)); */
	
	GCFREE(c);
}


/************************* Function: class_findfield ***************************
	
	Searches a 'classinfo' structure for a field having the given name and
	type.

*******************************************************************************/

fieldinfo *class_findfield(classinfo *c, utf *name, utf *desc)
{
	s4 i;
	
	for (i = 0; i < c->fieldscount; i++) { 
		if ((c->fields[i].name == name) && (c->fields[i].descriptor == desc)) 
			return &(c->fields[i]);					   			
    }

	panic("Can not find field given in CONSTANT_Fieldref");

	/* keep compiler happy */
	return NULL;
}


/************************* Function: class_findmethod **************************
	
	Searches a 'classinfo' structure for a method having the given name and
	type and returns the index in the class info structure.
	If type is NULL, it is ignored.

*******************************************************************************/

s4 class_findmethodIndex(classinfo *c, utf *name, utf *desc)
{
	s4 i;
#if defined(JOWENN_DEBUG1) || defined(JOWENN_DEBUG2)
	char *buffer;	         	
	int buffer_len, pos;
#endif
#ifdef JOWENN_DEBUG1

	buffer_len = 
		utf_strlen(name) + utf_strlen(desc) + utf_strlen(c->name) + 64;
	
	buffer = MNEW(char, buffer_len);

	strcpy(buffer, "class_findmethod: method:");
	utf_sprint(buffer + strlen(buffer), name);
	strcpy(buffer + strlen(buffer), ", desc: ");
	utf_sprint(buffer + strlen(buffer), desc);
	strcpy(buffer + strlen(buffer), ", classname: ");
	utf_sprint(buffer + strlen(buffer), c->name);
	
	log_text(buffer);	

	MFREE(buffer, char, buffer_len);
#endif	
	for (i = 0; i < c->methodscount; i++) {
#ifdef JOWENN_DEBUG2
		buffer_len = 
			utf_strlen(c->methods[i].name) + utf_strlen(c->methods[i].descriptor)+ 64;
	
		buffer = MNEW(char, buffer_len);

		strcpy(buffer, "class_findmethod: comparing to method:");
		utf_sprint(buffer + strlen(buffer), c->methods[i].name);
		strcpy(buffer + strlen(buffer), ", desc: ");
		utf_sprint(buffer + strlen(buffer), c->methods[i].descriptor);
	
		log_text(buffer);	

		MFREE(buffer, char, buffer_len);
#endif	
		
		
		if ((c->methods[i].name == name) && ((desc == NULL) ||
											 (c->methods[i].descriptor == desc))) {
			return i;
		}
	}

#ifdef JOWENN_DEBUG2	
	class_showconstantpool(c);
	log_text("class_findmethod: returning NULL");
#endif

	return -1;
}


/************************* Function: class_findmethod **************************
	
	Searches a 'classinfo' structure for a method having the given name and
	type.
	If type is NULL, it is ignored.

*******************************************************************************/

methodinfo *class_findmethod(classinfo *c, utf *name, utf *desc)
{
#if 0
	s4 i;
#if defined(JOWENN_DEBUG1) || defined(JOWENN_DEBUG2)
	char *buffer;	         	
	int buffer_len, pos;
#endif
#ifdef JOWENN_DEBUG1

	buffer_len = 
		utf_strlen(name) + utf_strlen(desc) + utf_strlen(c->name) + 64;
	
	buffer = MNEW(char, buffer_len);

	strcpy(buffer, "class_findmethod: method:");
	utf_sprint(buffer + strlen(buffer), name);
	strcpy(buffer + strlen(buffer), ", desc: ");
	utf_sprint(buffer + strlen(buffer), desc);
	strcpy(buffer + strlen(buffer), ", classname: ");
	utf_sprint(buffer + strlen(buffer), c->name);
	
	log_text(buffer);	

	MFREE(buffer, char, buffer_len);
#endif	
	for (i = 0; i < c->methodscount; i++) {
#ifdef JOWENN_DEBUG2
		buffer_len = 
			utf_strlen(c->methods[i].name) + utf_strlen(c->methods[i].descriptor)+ 64;
	
		buffer = MNEW(char, buffer_len);

		strcpy(buffer, "class_findmethod: comparing to method:");
		utf_sprint(buffer + strlen(buffer), c->methods[i].name);
		strcpy(buffer + strlen(buffer), ", desc: ");
		utf_sprint(buffer + strlen(buffer), c->methods[i].descriptor);
	
		log_text(buffer);	

		MFREE(buffer, char, buffer_len);
#endif	
		
		if ((c->methods[i].name == name) && ((desc == NULL) ||
											 (c->methods[i].descriptor == desc))) {
			return &(c->methods[i]);
		}
	}
#ifdef JOWENN_DEBUG2	
	class_showconstantpool(c);
	log_text("class_findmethod: returning NULL");
#endif
	return NULL;
#endif

	s4 idx=class_findmethodIndex(c, name, desc);
/*	if (idx==-1) log_text("class_findmethod: method not found");*/
	if (idx == -1) return NULL;

	return &(c->methods[idx]);
}


/*********************** Function: class_fetchmethod **************************
	
    like class_findmethod, but aborts with an error if the method is not found

*******************************************************************************/

methodinfo *class_fetchmethod(classinfo *c, utf *name, utf *desc)
{
	methodinfo *mi;
	mi = class_findmethod(c, name, desc);

	if (!mi) {
		log_plain("Class: "); if (c) log_plain_utf(c->name); log_nl();
		log_plain("Method: "); if (name) log_plain_utf(name); log_nl();
		log_plain("Descriptor: "); if (desc) log_plain_utf(desc); log_nl();
		panic("Method not found");
	}

	return mi;
}


/************************* Function: class_findmethod_approx ******************
	
	like class_findmethod but ignores the return value when comparing the
	descriptor.

*******************************************************************************/

methodinfo *class_findmethod_approx(classinfo *c, utf *name, utf *desc)
{
	s4 i;

	for (i = 0; i < c->methodscount; i++) {
		if (c->methods[i].name == name) {
			utf *meth_descr = c->methods[i].descriptor;
			
			if (desc == NULL) 
				/* ignore type */
				return &(c->methods[i]);

			if (desc->blength <= meth_descr->blength) {
				/* current position in utf text   */
				char *desc_utf_ptr = desc->text;      
				char *meth_utf_ptr = meth_descr->text;					  
				/* points behind utf strings */
				char *desc_end = utf_end(desc);         
				char *meth_end = utf_end(meth_descr);   
				char ch;

				/* compare argument types */
				while (desc_utf_ptr < desc_end && meth_utf_ptr < meth_end) {

					if ((ch = *desc_utf_ptr++) != (*meth_utf_ptr++))
						break; /* no match */

					if (ch == ')')
						return &(c->methods[i]);   /* all parameter types equal */
				}
			}
		}
	}

	return NULL;
}


/***************** Function: class_resolvemethod_approx ***********************
	
	Searches a class and every super class for a method (without paying
	attention to the return value)

*******************************************************************************/

methodinfo *class_resolvemethod_approx(classinfo *c, utf *name, utf *desc)
{
	while (c) {
		/* search for method (ignore returntype) */
		methodinfo *m = class_findmethod_approx(c, name, desc);
		/* method found */
		if (m) return m;
		/* search superclass */
		c = c->super;
	}

	return NULL;
}


/************************* Function: class_resolvemethod ***********************
	
	Searches a class and every super class for a method.

*******************************************************************************/

methodinfo *class_resolvemethod(classinfo *c, utf *name, utf *desc)
{
	while (c) {
		methodinfo *m = class_findmethod(c, name, desc);
		if (m) return m;
		/* search superclass */
		c = c->super;
	}

	return NULL;
}


/************************* Function: class_issubclass **************************

	Checks if sub is a descendant of super.
	
*******************************************************************************/

bool class_issubclass(classinfo *sub, classinfo *super)
{
	for (;;) {
		if (!sub) return false;
		if (sub == super) return true;
		sub = sub->super;
	}
}


/****************** Initialization function for classes ******************

	In Java, every class can have a static initialization function. This
	function has to be called BEFORE calling other methods or accessing static
	variables.

*******************************************************************************/

void class_init(classinfo *c)
{
	methodinfo *m;
	s4 i;
#ifdef USE_THREADS
	int b;
#endif

	if (!makeinitializations)
		return;
	if (c->initialized)
		return;

	c->initialized = true;

#ifdef STATISTICS
	count_class_inits++;
#endif

	/* initialize super class */
	if (c->super) {
		if (initverbose) {
			char logtext[MAXLOGTEXT];
			sprintf(logtext, "Initialize super class ");
			utf_sprint(logtext + strlen(logtext), c->super->name);
			sprintf(logtext + strlen(logtext), " from ");
			utf_sprint(logtext + strlen(logtext), c->name);
			log_text(logtext);
		}
		class_init(c->super);
	}

	/* initialize interface classes */
	for (i = 0; i < c->interfacescount; i++) {
		if (initverbose) {
			char logtext[MAXLOGTEXT];
			sprintf(logtext, "Initialize interface class ");
			utf_sprint(logtext + strlen(logtext), c->interfaces[i]->name);
			sprintf(logtext + strlen(logtext), " from ");
			utf_sprint(logtext + strlen(logtext), c->name);
			log_text(logtext);
		}
		class_init(c->interfaces[i]);  /* real */
	}

	m = class_findmethod(c, utf_clinit, utf_fidesc);
	if (!m) {
		if (initverbose) {
			char logtext[MAXLOGTEXT];
			sprintf(logtext, "Class ");
			utf_sprint(logtext + strlen(logtext), c->name);
			sprintf(logtext + strlen(logtext), " has no initializer");
			log_text(logtext);
		}
		/*              goto callinitialize;*/
		return;
	}

	if (!(m->flags & ACC_STATIC))
		panic("Class initializer is not static!");

	if (initverbose) {
		char logtext[MAXLOGTEXT];
		sprintf(logtext, "Starting initializer for class: ");
		utf_sprint(logtext + strlen(logtext), c->name);
		log_text(logtext);
	}

#ifdef USE_THREADS
	b = blockInts;
	blockInts = 0;
#endif

	/* now call the initializer */
	asm_calljavafunction(m, NULL, NULL, NULL, NULL);

#ifdef USE_THREADS
	assert(blockInts == 0);
	blockInts = b;
#endif

	/* we have to throw an exception */
	if (*exceptionptr) {
		printf("Exception in thread \"main\" java.lang.ExceptionInInitializerError\n");
		printf("Caused by: ");
		utf_display((*exceptionptr)->vftbl->class->name);
		printf("\n");
		fflush(stdout);
		exit(1);
	}

	if (initverbose) {
		char logtext[MAXLOGTEXT];
		sprintf(logtext, "Finished initializer for class: ");
		utf_sprint(logtext + strlen(logtext), c->name);
		log_text(logtext);
	}

	if (c->name == utf_systemclass) {
		/* class java.lang.System requires explicit initialization */

		if (initverbose)
			printf("#### Initializing class System");

                /* find initializing method */
		m = class_findmethod(c,
							 utf_initsystemclass,
							 utf_fidesc);

		if (!m) {
			/* no method found */
			/* printf("initializeSystemClass failed"); */
			return;
		}

#ifdef USE_THREADS
		b = blockInts;
		blockInts = 0;
#endif

		asm_calljavafunction(m, NULL, NULL, NULL, NULL);

#ifdef USE_THREADS
		assert(blockInts == 0);
		blockInts = b;
#endif

		if (*exceptionptr) {
			printf("#### initializeSystemClass has thrown: ");
			utf_display((*exceptionptr)->vftbl->class->name);
			printf("\n");
			fflush(stdout);
		}
	}
}


/********* Function: find_class_method_constant *********/

int find_class_method_constant (classinfo *c, utf * c1, utf* m1, utf* d1)  
{
	u4 i;
	voidptr e;

	for (i=0; i<c->cpcount; i++) {
		
		e = c -> cpinfos [i];
		if (e) {
			
			switch (c -> cptags [i]) {
			case CONSTANT_Methodref:
				{
					constant_FMIref *fmi = e;
					if (	   (fmi->class->name == c1)  
							   && (fmi->name == m1)
							   && (fmi->descriptor == d1)) {
					
						return i;
					}
				}
				break;

			case CONSTANT_InterfaceMethodref:
				{
					constant_FMIref *fmi = e;
					if (	   (fmi->class->name == c1)  
							   && (fmi->name == m1)
							   && (fmi->descriptor == d1)) {

						return i;
					}
				}
				break;
			}
		}
	}

	return -1;
}


void class_showconstanti(classinfo *c, int ii) 
{
	u4 i = ii;
	voidptr e;
		
	e = c->cpinfos [i];
	printf ("#%d:  ", (int) i);
	if (e) {
		switch (c->cptags [i]) {
		case CONSTANT_Class:
			printf("Classreference -> ");
			utf_display(((classinfo*)e)->name);
			break;
				
		case CONSTANT_Fieldref:
			printf("Fieldref -> "); goto displayFMIi;
		case CONSTANT_Methodref:
			printf("Methodref -> "); goto displayFMIi;
		case CONSTANT_InterfaceMethodref:
			printf("InterfaceMethod -> "); goto displayFMIi;
		displayFMIi:
			{
				constant_FMIref *fmi = e;
				utf_display(fmi->class->name);
				printf(".");
				utf_display(fmi->name);
				printf(" ");
				utf_display(fmi->descriptor);
			}
			break;

		case CONSTANT_String:
			printf("String -> ");
			utf_display(e);
			break;
		case CONSTANT_Integer:
			printf("Integer -> %d", (int) (((constant_integer*)e)->value));
			break;
		case CONSTANT_Float:
			printf("Float -> %f", ((constant_float*)e)->value);
			break;
		case CONSTANT_Double:
			printf("Double -> %f", ((constant_double*)e)->value);
			break;
		case CONSTANT_Long:
			{
				u8 v = ((constant_long*)e)->value;
#if U8_AVAILABLE
				printf("Long -> %ld", (long int) v);
#else
				printf("Long -> HI: %ld, LO: %ld\n", 
					    (long int) v.high, (long int) v.low);
#endif 
			}
			break;
		case CONSTANT_NameAndType:
			{ 
				constant_nameandtype *cnt = e;
				printf("NameAndType: ");
				utf_display(cnt->name);
				printf(" ");
				utf_display(cnt->descriptor);
			}
			break;
		case CONSTANT_Utf8:
			printf("Utf8 -> ");
			utf_display(e);
			break;
		default: 
			panic("Invalid type of ConstantPool-Entry");
		}
	}
	printf("\n");
}


void class_showconstantpool (classinfo *c) 
{
	u4 i;
	voidptr e;

	printf ("---- dump of constant pool ----\n");

	for (i=0; i<c->cpcount; i++) {
		printf ("#%d:  ", (int) i);
		
		e = c -> cpinfos [i];
		if (e) {
			
			switch (c -> cptags [i]) {
			case CONSTANT_Class:
				printf ("Classreference -> ");
				utf_display ( ((classinfo*)e) -> name );
				break;
				
			case CONSTANT_Fieldref:
				printf ("Fieldref -> "); goto displayFMI;
			case CONSTANT_Methodref:
				printf ("Methodref -> "); goto displayFMI;
			case CONSTANT_InterfaceMethodref:
				printf ("InterfaceMethod -> "); goto displayFMI;
			displayFMI:
				{
					constant_FMIref *fmi = e;
					utf_display ( fmi->class->name );
					printf (".");
					utf_display ( fmi->name);
					printf (" ");
					utf_display ( fmi->descriptor );
				}
				break;

			case CONSTANT_String:
				printf ("String -> ");
				utf_display (e);
				break;
			case CONSTANT_Integer:
				printf ("Integer -> %d", (int) ( ((constant_integer*)e) -> value) );
				break;
			case CONSTANT_Float:
				printf ("Float -> %f", ((constant_float*)e) -> value);
				break;
			case CONSTANT_Double:
				printf ("Double -> %f", ((constant_double*)e) -> value);
				break;
			case CONSTANT_Long:
				{
					u8 v = ((constant_long*)e) -> value;
#if U8_AVAILABLE
					printf ("Long -> %ld", (long int) v);
#else
					printf ("Long -> HI: %ld, LO: %ld\n", 
							(long int) v.high, (long int) v.low);
#endif 
				}
				break;
			case CONSTANT_NameAndType:
				{
					constant_nameandtype *cnt = e;
					printf ("NameAndType: ");
					utf_display (cnt->name);
					printf (" ");
					utf_display (cnt->descriptor);
				}
				break;
			case CONSTANT_Utf8:
				printf ("Utf8 -> ");
				utf_display (e);
				break;
			default: 
				panic ("Invalid type of ConstantPool-Entry");
			}
		}

		printf ("\n");
	}
}



/********** Function: class_showmethods   (debugging only) *************/

void class_showmethods (classinfo *c)
{
	s4 i;
	
	printf ("--------- Fields and Methods ----------------\n");
	printf ("Flags: ");	printflags (c->flags);	printf ("\n");

	printf ("This: "); utf_display (c->name); printf ("\n");
	if (c->super) {
		printf ("Super: "); utf_display (c->super->name); printf ("\n");
		}
	printf ("Index: %d\n", c->index);
	
	printf ("interfaces:\n");	
	for (i=0; i < c-> interfacescount; i++) {
		printf ("   ");
		utf_display (c -> interfaces[i] -> name);
		printf (" (%d)\n", c->interfaces[i] -> index);
		}

	printf ("fields:\n");		
	for (i=0; i < c -> fieldscount; i++) {
		field_display (&(c -> fields[i]));
		}

	printf ("methods:\n");
	for (i=0; i < c -> methodscount; i++) {
		methodinfo *m = &(c->methods[i]);
		if ( !(m->flags & ACC_STATIC)) 
			printf ("vftblindex: %d   ", m->vftblindex);

		method_display ( m );

		}

	printf ("Virtual function table:\n");
	for (i=0; i<c->vftbl->vftbllength; i++) {
		printf ("entry: %d,  %ld\n", i, (long int) (c->vftbl->table[i]) );
		}

}



/******************************************************************************/
/******************* General functions for the class loader *******************/
/******************************************************************************/

/********************* Function: loader_load ***********************************

	Loads and links the class desired class and each class and interface
	referenced by it.
	Returns: a pointer to this class

*******************************************************************************/

static int loader_load_running = 0;

classinfo *loader_load(utf *topname)
{
	classinfo *top;
	classinfo *c;
	s8 starttime = 0;
	s8 stoptime = 0;
	classinfo *notlinkable;

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	pthread_mutex_lock(&compiler_mutex);
#endif

	/* avoid recursive calls */
	if (loader_load_running)
		return class_new(topname);

	loader_load_running++;
	
	intsDisable();

	if (getloadingtime)
		starttime = getcputime();

	top = class_new(topname);

	/* load classes */
	while ((c = list_first(&unloadedclasses))) {
		if (!class_load(c)) {
			if (linkverbose)
				dolog("Failed to load class");
			list_remove(&unloadedclasses, c);
			top = NULL;
		}
	}

	/* link classes */
	if (linkverbose)
		dolog("Linking...");

	/* XXX added a hack to break infinite linking loops. A better
	 * linking algorithm would be nice. -Edwin */
	notlinkable = NULL;
	while ((c = list_first(&unlinkedclasses))) {
		class_link(c);
		if (!c->linked) {
			if (!notlinkable)
				notlinkable = c;
			else if (notlinkable == c) {
				/* We tried to link this class for the second time and
				 * no other classes were linked in between, so we are
				 * caught in a loop.
				 */
				if (linkverbose)
					dolog("Cannot resolve linking dependencies");
				top = NULL;
				if (!*exceptionptr)
					throw_linkageerror_message(c->name);
				break;
			}

		} else
			notlinkable = NULL;
	}
	if (linkverbose)
		dolog("Linking done.");

	if (loader_inited)
		loader_compute_subclasses();

	/* measure time */
	if (getloadingtime) {
		stoptime = getcputime();
		loadingtime += (stoptime - starttime);
	}


	loader_load_running--;
	
	/* check if a former loader_load call tried to load/link the class and 
	   failed. This is needed because the class didn't appear in the 
	   undloadclasses or unlinkedclasses list during this class. */
	if (top) {
		if (!top->loaded) {
			if (linkverbose) dolog("Failed to load class (former call)");
			throw_noclassdeffounderror_message(top->name);
			top = NULL;
			
		} else if (!top->linked) {
			if (linkverbose)
				dolog("Failed to link class (former call)");
			throw_linkageerror_message(top->name);
			top = NULL;
		}
	}

	intsRestore();
	
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	pthread_mutex_unlock(&compiler_mutex);
#endif

	/* XXX DEBUG */ if (linkverbose && !top) dolog("returning NULL from loader_load");
	
	return top; 
}


/****************** Function: loader_load_sysclass ****************************

	Loads and links the class desired class and each class and interface
	referenced by it.

    The pointer to the classinfo is stored in *top if top != NULL.
    The pointer is also returned.

    If the class could not be loaded the function aborts with an error.

*******************************************************************************/

classinfo *loader_load_sysclass(classinfo **top, utf *topname)
{
	classinfo *cls;

	if ((cls = loader_load(topname)) == NULL) {
		log_plain("Could not load important system class: ");
		log_plain_utf(topname);
		log_nl();
		panic("Could not load important system class");
	}

	if (top) *top = cls;

	return cls;
}


/**************** function: create_primitive_classes ***************************

	create classes representing primitive types 

********************************************************************************/


void create_primitive_classes()
{  
	int i;

	for (i=0;i<PRIMITIVETYPE_COUNT;i++) {
		/* create primitive class */
		classinfo *c = class_new ( utf_new_char(primitivetype_table[i].name) );
                c -> classUsed = NOTUSED; /* not used initially CO-RT */		
		c -> impldBy = NULL;
		
		/* prevent loader from loading primitive class */
		list_remove (&unloadedclasses, c);
		c->loaded=true;
		/* add to unlinked classes */
		list_addlast (&unlinkedclasses, c);		
/*JOWENN primitive types don't have objects as super class		c -> super = class_java_lang_Object; */
		class_link (c);

		primitivetype_table[i].class_primitive = c;

		/* create class for wrapping the primitive type */
		primitivetype_table[i].class_wrap =
	        	class_new( utf_new_char(primitivetype_table[i].wrapname) );
                primitivetype_table[i].class_wrap -> classUsed = NOTUSED; /* not used initially CO-RT */
		primitivetype_table[i].class_wrap  -> impldBy = NULL;

		/* create the primitive array class */
		if (primitivetype_table[i].arrayname) {
			c = class_new( utf_new_char(primitivetype_table[i].arrayname) );
			primitivetype_table[i].arrayclass = c;
			c->loaded=true;
			if (!c->linked) class_link(c);
			primitivetype_table[i].arrayvftbl = c->vftbl;
		}
	}
}

/**************** function: class_primitive_from_sig ***************************

	return the primitive class indicated by the given signature character

    If the descriptor does not indicate a valid primitive type the
    return value is NULL.

********************************************************************************/

classinfo *class_primitive_from_sig(char sig)
{
	switch (sig) {
	  case 'I': return primitivetype_table[PRIMITIVETYPE_INT].class_primitive;
	  case 'J': return primitivetype_table[PRIMITIVETYPE_LONG].class_primitive;
	  case 'F': return primitivetype_table[PRIMITIVETYPE_FLOAT].class_primitive;
	  case 'D': return primitivetype_table[PRIMITIVETYPE_DOUBLE].class_primitive;
	  case 'B': return primitivetype_table[PRIMITIVETYPE_BYTE].class_primitive;
	  case 'C': return primitivetype_table[PRIMITIVETYPE_CHAR].class_primitive;
	  case 'S': return primitivetype_table[PRIMITIVETYPE_SHORT].class_primitive;
	  case 'Z': return primitivetype_table[PRIMITIVETYPE_BOOLEAN].class_primitive;
	  case 'V': return primitivetype_table[PRIMITIVETYPE_VOID].class_primitive;
	}
	return NULL;
}

/****************** function: class_from_descriptor ****************************

    return the class indicated by the given descriptor

    utf_ptr....first character of descriptor
    end_ptr....first character after the end of the string
    next.......if non-NULL, *next is set to the first character after
               the descriptor. (Undefined if an error occurs.)

    mode.......a combination (binary or) of the following flags:

               (Flags marked with * are the default settings.)

               What to do if a reference type descriptor is parsed successfully:

                   CLASSLOAD_SKIP...skip it and return something != NULL
				 * CLASSLOAD_NEW....get classinfo * via class_new
                   CLASSLOAD_LOAD...get classinfo * via loader_load

               How to handle primitive types:

			     * CLASSLOAD_PRIMITIVE.......return primitive class (eg. "int")
                   CLASSLOAD_NULLPRIMITIVE...return NULL for primitive types

               How to handle "V" descriptors:

			     * CLASSLOAD_VOID.....handle it like other primitive types
                   CLASSLOAD_NOVOID...treat it as an error

               How to deal with extra characters after the end of the
               descriptor:

			     * CLASSLOAD_NOCHECKEND...ignore (useful for parameter lists)
                   CLASSLOAD_CHECKEND.....treat them as an error

               How to deal with errors:

			     * CLASSLOAD_PANIC....abort execution with an error message
                   CLASSLOAD_NOPANIC..return NULL on error

********************************************************************************/

classinfo *class_from_descriptor(char *utf_ptr, char *end_ptr,
								 char **next, int mode)
{
	char *start = utf_ptr;
	bool error = false;
	utf *name;

	SKIP_FIELDDESCRIPTOR_SAFE(utf_ptr,end_ptr,error);

	if (mode & CLASSLOAD_CHECKEND)
		error |= (utf_ptr != end_ptr);
	
	if (!error) {
		if (next) *next = utf_ptr;
		
		switch (*start) {
		  case 'V':
			  if (mode & CLASSLOAD_NOVOID)
				  break;
			  /* FALLTHROUGH! */
		  case 'I':
		  case 'J':
		  case 'F':
		  case 'D':
		  case 'B':
		  case 'C':
		  case 'S':
		  case 'Z':
			  return (mode & CLASSLOAD_NULLPRIMITIVE)
				  ? NULL
				  : class_primitive_from_sig(*start);
			  
		  case 'L':
			  start++;
			  utf_ptr--;
			  /* FALLTHROUGH! */
		  case '[':
			  if (mode & CLASSLOAD_SKIP) return class_java_lang_Object;
			  name = utf_new(start,utf_ptr-start);
			  return (mode & CLASSLOAD_LOAD)
				  ? loader_load(name) : class_new(name); /* XXX */
		}
	}

	/* An error occurred */
	if (mode & CLASSLOAD_NOPANIC)
		return NULL;

	log_plain("Invalid descriptor at beginning of '");
	log_plain_utf(utf_new(start, end_ptr-start));
	log_plain("'");
	log_nl();
						  
	panic("Invalid descriptor");

	/* keep compiler happy */
	return NULL;
}


/******************* function: type_from_descriptor ****************************

    return the basic type indicated by the given descriptor

    This function parses a descriptor and returns its basic type as
    TYPE_INT, TYPE_LONG, TYPE_FLOAT, TYPE_DOUBLE, TYPE_ADDRESS or TYPE_VOID.

    cls...if non-NULL the referenced variable is set to the classinfo *
          returned by class_from_descriptor.

    For documentation of the arguments utf_ptr, end_ptr, next and mode
    see class_from_descriptor. The only difference is that
    type_from_descriptor always uses CLASSLOAD_PANIC.

********************************************************************************/

int type_from_descriptor(classinfo **cls, char *utf_ptr, char *end_ptr,
						 char **next, int mode)
{
	classinfo *mycls;
	if (!cls) cls = &mycls;
	*cls = class_from_descriptor(utf_ptr, end_ptr, next, mode & (~CLASSLOAD_NOPANIC));
	switch (*utf_ptr) {
	  case 'B': 
	  case 'C':
	  case 'I':
	  case 'S':  
	  case 'Z':
		  return TYPE_INT;
	  case 'D':
		  return TYPE_DOUBLE;
	  case 'F':
		  return TYPE_FLOAT;
	  case 'J':
		  return TYPE_LONG;
	  case 'V':
		  return TYPE_VOID;
	}
	return TYPE_ADDRESS;
}


/*************** function: create_pseudo_classes *******************************

	create pseudo classes used by the typechecker

********************************************************************************/

static void create_pseudo_classes()
{
    /* pseudo class for Arraystubs (extends java.lang.Object) */
    
    pseudo_class_Arraystub = class_new(utf_new_char("$ARRAYSTUB$"));
    list_remove(&unloadedclasses, pseudo_class_Arraystub);

    pseudo_class_Arraystub->super = class_java_lang_Object;
    pseudo_class_Arraystub->interfacescount = 2;
    pseudo_class_Arraystub->interfaces = MNEW(classinfo*, 2);
    pseudo_class_Arraystub->interfaces[0] = class_java_lang_Cloneable;
    pseudo_class_Arraystub->interfaces[1] = class_java_io_Serializable;

    list_addlast(&unlinkedclasses, pseudo_class_Arraystub);
    class_link(pseudo_class_Arraystub);

	pseudo_class_Arraystub_vftbl = pseudo_class_Arraystub->vftbl;

    /* pseudo class representing the null type */
    
    pseudo_class_Null = class_new(utf_new_char("$NULL$"));
    list_remove(&unloadedclasses, pseudo_class_Null);

    pseudo_class_Null->super = class_java_lang_Object;

    list_addlast(&unlinkedclasses, pseudo_class_Null);
    class_link(pseudo_class_Null);	

    /* pseudo class representing new uninitialized objects */
    
    pseudo_class_New = class_new(utf_new_char("$NEW$"));
    list_remove(&unloadedclasses, pseudo_class_New);

    pseudo_class_New->super = class_java_lang_Object;

    list_addlast(&unlinkedclasses, pseudo_class_New);
    class_link(pseudo_class_New);	
}


/********************** Function: loader_init **********************************

	Initializes all lists and loads all classes required for the system or the
	compiler.

*******************************************************************************/
 
void loader_init(u1 *stackbottom)
{
	interfaceindex = 0;
	
	list_init(&unloadedclasses, OFFSET(classinfo, listnode));
	list_init(&unlinkedclasses, OFFSET(classinfo, listnode));
	list_init(&linkedclasses, OFFSET(classinfo, listnode));

	/* create utf-symbols for pointer comparison of frequently used strings */
	utf_innerclasses    = utf_new_char("InnerClasses");
	utf_constantvalue   = utf_new_char("ConstantValue");
	utf_code	        = utf_new_char("Code");
	utf_finalize	    = utf_new_char("finalize");
	utf_fidesc	        = utf_new_char("()V");
	utf_init	        = utf_new_char("<init>");
	utf_clinit	        = utf_new_char("<clinit>");
	utf_initsystemclass = utf_new_char("initializeSystemClass");
	utf_systemclass     = utf_new_char("java/lang/System");
	utf_vmclassloader   = utf_new_char("java/lang/VMClassLoader");
	utf_initialize      = utf_new_char("initialize");
	utf_initializedesc  = utf_new_char("(I)V");

	utf_vmclass         = utf_new_char("java/lang/VMClass");
	utf_java_lang_Object= utf_new_char("java/lang/Object");

	/* create some important classes */
	/* These classes have to be created now because the classinfo
	 * pointers are used in the loading code.
	 */
	class_java_lang_Object = class_new(utf_new_char("java/lang/Object"));
	class_java_lang_String = class_new(utf_new_char("java/lang/String"));
	class_java_lang_Cloneable = class_new(utf_new_char("java/lang/Cloneable"));
	class_java_io_Serializable = class_new(utf_new_char("java/io/Serializable"));

	if (verbose) log_text("loader_init: java/lang/Object");
	/* load the classes which were created above */
	loader_load_sysclass(NULL, class_java_lang_Object->name);

	loader_inited = 1; /*JOWENN*/

	loader_load_sysclass(&class_java_lang_Throwable,
						 utf_new_char("java/lang/Throwable"));

	if (verbose) log_text("loader_init:  loader_load: java/lang/ClassCastException");
	loader_load_sysclass(&class_java_lang_ClassCastException,
						 utf_new_char ("java/lang/ClassCastException"));
	loader_load_sysclass(&class_java_lang_NullPointerException,
						 utf_new_char ("java/lang/NullPointerException"));
	loader_load_sysclass(&class_java_lang_ArrayIndexOutOfBoundsException,
						 utf_new_char ("java/lang/ArrayIndexOutOfBoundsException"));
	loader_load_sysclass(&class_java_lang_NegativeArraySizeException,
						 utf_new_char ("java/lang/NegativeArraySizeException"));
	loader_load_sysclass(&class_java_lang_OutOfMemoryError,
						 utf_new_char ("java/lang/OutOfMemoryError"));
	loader_load_sysclass(&class_java_lang_ArrayStoreException,
						 utf_new_char ("java/lang/ArrayStoreException"));
	loader_load_sysclass(&class_java_lang_ArithmeticException,
						 utf_new_char ("java/lang/ArithmeticException"));
	loader_load_sysclass(&class_java_lang_ThreadDeath,
						 utf_new_char ("java/lang/ThreadDeath"));
		
	/* create classes representing primitive types */
	create_primitive_classes();

	/* create classes used by the typechecker */
	create_pseudo_classes();

	/* correct vftbl-entries (retarded loading of class java/lang/String) */
	stringtable_update();

#ifdef USE_THREADS
	if (stackbottom!=0)
		initLocks();
#endif

	if (verbose) log_text("loader_init: creating global proto_java_lang_ClassCastException");
	proto_java_lang_ClassCastException =
		builtin_new(class_java_lang_ClassCastException);

	if (verbose) log_text("loader_init: proto_java_lang_ClassCastException has been initialized");

	proto_java_lang_NullPointerException =
		builtin_new(class_java_lang_NullPointerException);
	if (verbose) log_text("loader_init: proto_java_lang_NullPointerException has been initialized");

	proto_java_lang_ArrayIndexOutOfBoundsException =
		builtin_new(class_java_lang_ArrayIndexOutOfBoundsException);

	proto_java_lang_NegativeArraySizeException =
		builtin_new(class_java_lang_NegativeArraySizeException);

	proto_java_lang_OutOfMemoryError =
		builtin_new(class_java_lang_OutOfMemoryError);

	proto_java_lang_ArithmeticException =
		builtin_new(class_java_lang_ArithmeticException);

	proto_java_lang_ArrayStoreException =
		builtin_new(class_java_lang_ArrayStoreException);

	proto_java_lang_ThreadDeath =
		builtin_new(class_java_lang_ThreadDeath);

	loader_inited = 1;
}


/********************* Function: loader_initclasses ****************************

	Initializes all loaded but uninitialized classes

*******************************************************************************/

#if 0
/* XXX TWISTI: i think we do not need this */
void loader_initclasses ()
{
	classinfo *c;
	
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	pthread_mutex_lock(&compiler_mutex);
#endif

	intsDisable();                     /* schani */

	if (makeinitializations) {
		c = list_first(&linkedclasses);
		while (c) {
			class_init(c);
			c = list_next(&linkedclasses, c);
		}
	}

	intsRestore();                      /* schani */
	
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	pthread_mutex_unlock(&compiler_mutex);
#endif
}
#endif


static void loader_compute_class_values(classinfo *c)
{
	classinfo *subs;

	c->vftbl->baseval = ++classvalue;

	subs = c->sub;
	while (subs != NULL) {
		loader_compute_class_values(subs);
		subs = subs->nextsub;
	}
	c->vftbl->diffval = classvalue - c->vftbl->baseval;
	
	/*
	{
	int i;
	for (i = 0; i < c->index; i++)
		printf(" ");
	printf("%3d  %3d  ", (int) c->vftbl->baseval, c->vftbl->diffval);
	utf_display(c->name);
	printf("\n");
	}
	*/
}


void loader_compute_subclasses()
{
	classinfo *c;
	
	intsDisable();                     /* schani */

	c = list_first(&linkedclasses);
	while (c) {
		if (!(c->flags & ACC_INTERFACE)) {
			c->nextsub = 0;
			c->sub = 0;
		}
		c = list_next(&linkedclasses, c);
	}

	c = list_first(&linkedclasses);
	while (c) {
		if (!(c->flags & ACC_INTERFACE) && (c->super != NULL)) {
			c->nextsub = c->super->sub;
			c->super->sub = c;
		}
		c = list_next(&linkedclasses, c);
	}

	classvalue = 0;
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	cast_lock();
#endif
	loader_compute_class_values(class_java_lang_Object);
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	cast_unlock();
#endif

	intsRestore();                      /* schani */
}


/******************** function classloader_buffer ******************************
 
    sets buffer for reading classdata

*******************************************************************************/

void classload_buffer(u1 *buf, int len)
{
	classbuffer      = buf;
	classbuffer_size = len;
	classbuf_pos     = buf - 1;
}


/******************** Function: loader_close ***********************************

	Frees all resources
	
*******************************************************************************/

void loader_close()
{
	classinfo *c;

	while ((c = list_first(&unloadedclasses))) {
		list_remove(&unloadedclasses, c);
		class_free(c);
	}
	while ((c = list_first(&unlinkedclasses))) {
		list_remove(&unlinkedclasses, c);
		class_free(c);
	}
	while ((c = list_first(&linkedclasses))) {
		list_remove(&linkedclasses, c);
		class_free(c);
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
