/* src/vm/loader.c - class loader functions

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

   Changes: Andreas Krall
            Roman Obermaiser
            Mark Probst
            Edwin Steiner
            Christian Thalinger

   $Id: loader.c 2148 2005-03-30 16:49:40Z twisti $

*/


#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>

#include "config.h"
#include "mm/memory.h"
#include "native/native.h"
#include "native/include/java_lang_Throwable.h"

#if defined(USE_THREADS)
# if defined(NATIVE_THREADS)
#  include "threads/native/threads.h"
# else
#  include "threads/green/threads.h"
#  include "threads/green/locks.h"
# endif
#endif

#include "toolbox/logging.h"
#include "toolbox/util.h"
#include "vm/exceptions.h"
#include "vm/builtin.h"
#include "vm/global.h"
#include "vm/linker.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/statistics.h"
#include "vm/stringlocal.h"
#include "vm/tables.h"

#if defined(USE_ZLIB)
# include "vm/unzip.h"
#endif

#include "vm/jit/asmpart.h"
#include "vm/jit/codegen.inc.h"


#undef JOWENN_DEBUG
#undef JOWENN_DEBUG1
#undef JOWENN_DEBUG2


/********************************************************************
   list of classpath entries (either filesystem directories or 
   ZIP/JAR archives
********************************************************************/

classpath_info *classpath_entries = NULL;


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

    The number of arguments is returned. A long or double argument is counted
    as two arguments.
	
*******************************************************************************/

static int checkmethoddescriptor(classinfo *c, utf *descriptor)
{
	char *utf_ptr;                      /* current position in utf text       */
	char *end_pos;                      /* points behind utf string           */
	s4 argcount = 0;                    /* number of arguments                */

	utf_ptr = descriptor->text;
	end_pos = utf_end(descriptor);

	/* method descriptor must start with parenthesis */
	if (utf_ptr == end_pos || *utf_ptr++ != '(')
		panic ("Missing '(' in method descriptor");

    /* check arguments */
    while (utf_ptr != end_pos && *utf_ptr != ')') {
		/* We cannot count the this argument here because
		 * we don't know if the method is static. */
		if (*utf_ptr == 'J' || *utf_ptr == 'D')
			argcount+=2;
		else
			argcount++;
		class_from_descriptor(utf_ptr,end_pos,&utf_ptr,
							  CLASSLOAD_NEW
							  | CLASSLOAD_NULLPRIMITIVE
							  | CLASSLOAD_NOVOID);
	}

	if (utf_ptr == end_pos)
		panic("Missing ')' in method descriptor");

    utf_ptr++; /* skip ')' */

	class_from_descriptor(utf_ptr,
						  end_pos,
						  NULL,
						  CLASSLOAD_NEW |
						  CLASSLOAD_NULLPRIMITIVE |
						  CLASSLOAD_CHECKEND);

	if (argcount > 255) {
		*exceptionptr =
			new_classformaterror(c, "Too many arguments in signature");

		return 0;
	}

	return argcount;

	/* XXX use the following if -noverify */
#if 0
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


/* loader_init *****************************************************************

   Initializes all lists and loads all classes required for the system
   or the compiler.

*******************************************************************************/
 
bool loader_init(u1 *stackbottom)
{
	classpath_info *cpi;

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	/* Initialize the monitor pointer for zip/jar file locking.               */

	for (cpi = classpath_entries; cpi != NULL; cpi = cpi->next) {
		if (cpi->type == CLASSPATH_ARCHIVE)
			initObjectLock(&cpi->header);
	}
#endif

	/* load some important classes */

	if (!load_class_bootstrap(class_java_lang_Object))
		return false;

	if (!load_class_bootstrap(class_java_lang_String))
		return false;

	if (!load_class_bootstrap(class_java_lang_Cloneable))
		return false;

	if (!load_class_bootstrap(class_java_io_Serializable))
		return false;

#if defined(USE_THREADS)
	if (stackbottom != 0)
		initLocks();
#endif

	return true;
}


/************* functions for reading classdata *********************************

    getting classdata in blocks of variable size
    (8,16,32,64-bit integer or float)

*******************************************************************************/

/* check_classbuffer_size ******************************************************

   assert that at least <len> bytes are left to read
   <len> is limited to the range of non-negative s4 values

*******************************************************************************/

inline bool check_classbuffer_size(classbuffer *cb, s4 len)
{
	if (len < 0 || ((cb->data + cb->size) - cb->pos - 1) < len) {
		*exceptionptr =
			new_classformaterror((cb)->class, "Truncated class file");

		return false;
	}

	return true;
}


/* suck_nbytes *****************************************************************

   transfer block of classfile data into a buffer

*******************************************************************************/

inline void suck_nbytes(u1 *buffer, classbuffer *cb, s4 len)
{
	memcpy(buffer, cb->pos + 1, len);
	cb->pos += len;
}


/* skip_nbytes ****************************************************************

   skip block of classfile data

*******************************************************************************/

inline void skip_nbytes(classbuffer *cb, s4 len)
{
	cb->pos += len;
}


inline u1 suck_u1(classbuffer *cb)
{
	return *++(cb->pos);
}


inline u2 suck_u2(classbuffer *cb)
{
	u1 a = suck_u1(cb);
	u1 b = suck_u1(cb);
	return ((u2) a << 8) + (u2) b;
}


inline u4 suck_u4(classbuffer *cb)
{
	u1 a = suck_u1(cb);
	u1 b = suck_u1(cb);
	u1 c = suck_u1(cb);
	u1 d = suck_u1(cb);
	return ((u4) a << 24) + ((u4) b << 16) + ((u4) c << 8) + (u4) d;
}


/* get u8 from classfile data */
static u8 suck_u8(classbuffer *cb)
{
#if U8_AVAILABLE
	u8 lo, hi;
	hi = suck_u4(cb);
	lo = suck_u4(cb);
	return (hi << 32) + lo;
#else
	u8 v;
	v.high = suck_u4(cb);
	v.low = suck_u4(cb);
	return v;
#endif
}


/* get float from classfile data */
static float suck_float(classbuffer *cb)
{
	float f;

#if !WORDS_BIGENDIAN 
	u1 buffer[4];
	u2 i;

	for (i = 0; i < 4; i++)
		buffer[3 - i] = suck_u1(cb);

	memcpy((u1*) (&f), buffer, 4);
#else
	suck_nbytes((u1*) (&f), cb, 4);
#endif

	if (sizeof(float) != 4) {
		*exceptionptr = new_exception_message(string_java_lang_InternalError,
											  "Incompatible float-format");

		/* XXX should we exit in such a case? */
		throw_exception_exit();
	}
	
	return f;
}


/* get double from classfile data */
static double suck_double(classbuffer *cb)
{
	double d;

#if !WORDS_BIGENDIAN 
	u1 buffer[8];
	u2 i;	

	for (i = 0; i < 8; i++)
		buffer[7 - i] = suck_u1(cb);

	memcpy((u1*) (&d), buffer, 8);
#else 
	suck_nbytes((u1*) (&d), cb, 8);
#endif

	if (sizeof(double) != 8) {
		*exceptionptr = new_exception_message(string_java_lang_InternalError,
											  "Incompatible double-format");

		/* XXX should we exit in such a case? */
		throw_exception_exit();
	}
	
	return d;
}


/************************** function suck_init *********************************

	called once at startup, sets the searchpath for the classfiles

*******************************************************************************/

void suck_init(char *classpath)
{
	char           *start;
	char           *end;
	char           *filename;
	s4              filenamelen;
	bool            is_zip;
	classpath_info *cpi;
	classpath_info *lastcpi;
	char           *cwd;
	s4              cwdlen;

	/* search for last classpath entry (only if there already some) */

	if ((lastcpi = classpath_entries)) {
		while (lastcpi->next)
			lastcpi = lastcpi->next;
	}

	for (start = classpath; (*start) != '\0';) {

		/* search for ':' delimiter to get the end of the current entry */
		for (end = start; ((*end) != '\0') && ((*end) != ':'); end++);

		if (start != end) {
			is_zip = false;
			filenamelen = end - start;

			if (filenamelen > 3) {
				if (strncasecmp(end - 3, "zip", 3) == 0 ||
					strncasecmp(end - 3, "jar", 3) == 0) {
					is_zip = true;
				}
			}

			/* save classpath entries as absolute pathnames */

			cwd = NULL;
			cwdlen = 0;

			if (*start != '/') {                      /* XXX fix me for win32 */
				cwd = _Jv_getcwd();
				cwdlen = strlen(cwd) + strlen("/");
			}

			/* allocate memory for filename and fill it */

			filename = MNEW(char, filenamelen + cwdlen + strlen("/") +
							strlen("0"));

			if (cwd) {
				strcpy(filename, cwd);
				strcat(filename, "/");
				strncat(filename, start, filenamelen);

				/* add cwd length to file length */
				filenamelen += cwdlen;

			} else {
				strncpy(filename, start, filenamelen);
				filename[filenamelen] = '\0';
			}

			cpi = NULL;

			if (is_zip) {
#if defined(USE_ZLIB)
				unzFile uf = unzOpen(filename);

				if (uf) {
					cpi = NEW(classpath_info);
					cpi->type = CLASSPATH_ARCHIVE;
					cpi->uf = uf;
					cpi->next = NULL;
					cpi->path = filename;
					cpi->pathlen = filenamelen;
				}

#else
				throw_cacao_exception_exit(string_java_lang_InternalError,
										   "zip/jar files not supported");
#endif
				
			} else {
				cpi = NEW(classpath_info);
				cpi->type = CLASSPATH_PATH;
				cpi->next = NULL;

				if (filename[filenamelen - 1] != '/') {/*PERHAPS THIS SHOULD BE READ FROM A GLOBAL CONFIGURATION */
					filename[filenamelen] = '/';
					filename[filenamelen + 1] = '\0';
					filenamelen++;
				}

				cpi->path = filename;
				cpi->pathlen = filenamelen;
			}

			/* attach current classpath entry */

			if (cpi) {
				if (!classpath_entries)
					classpath_entries = cpi;
				else
					lastcpi->next = cpi;

				lastcpi = cpi;
			}
		}

		/* goto next classpath entry, skip ':' delimiter */

		if ((*end) == ':') {
			start = end + 1;

		} else {
			start = end;
		}
	}
}


void create_all_classes()
{
	classpath_info *cpi;

	for (cpi = classpath_entries; cpi != 0; cpi = cpi->next) {
#if defined(USE_ZLIB)
		if (cpi->type == CLASSPATH_ARCHIVE) {
			cacao_entry_s *ce;
			unz_s *s;

			s = (unz_s *) cpi->uf;
			ce = s->cacao_dir_list;
				
			while (ce) {
				(void) class_new(ce->name);
				ce = ce->next;
			}

		} else {
#endif
#if defined(USE_ZLIB)
		}
#endif
	}
}


/* suck_start ******************************************************************

   Returns true if classbuffer is already loaded or a file for the
   specified class has succussfully been read in. All directories of
   the searchpath are used to find the classfile (<classname>.class).
   Returns false if no classfile is found and writes an error message.
	
*******************************************************************************/

classbuffer *suck_start(classinfo *c)
{
	classpath_info *cpi;
	char           *filename;
	s4             filenamelen;
	char *path;
	FILE *classfile;
	bool found;
	s4 len;
	struct stat buffer;
	classbuffer *cb;

	/* initialize return value */

	found = false;
	cb = NULL;

	filenamelen = utf_strlen(c->name) + strlen(".class") + strlen("0");
	filename = MNEW(char, filenamelen);

	utf_sprint(filename, c->name);
	strcat(filename, ".class");

	/* walk through all classpath entries */

	for (cpi = classpath_entries; cpi != NULL && cb == NULL; cpi = cpi->next) {
#if defined(USE_ZLIB)
		if (cpi->type == CLASSPATH_ARCHIVE) {

#if defined(USE_THREADS)
			/* enter a monitor on zip/jar archives */

			builtin_monitorenter((java_objectheader *) cpi);
#endif

			if (cacao_locate(cpi->uf, c->name) == UNZ_OK) {
				unz_file_info file_info;

				if (unzGetCurrentFileInfo(cpi->uf, &file_info, filename,
										  sizeof(filename), NULL, 0, NULL, 0) == UNZ_OK) {
					if (unzOpenCurrentFile(cpi->uf) == UNZ_OK) {
						cb = NEW(classbuffer);
						cb->class = c;
						cb->size = file_info.uncompressed_size;
						cb->data = MNEW(u1, cb->size);
						cb->pos = cb->data - 1;

						len = unzReadCurrentFile(cpi->uf, cb->data, cb->size);

						if (len != cb->size) {
							suck_stop(cb);
							log_text("Error while unzipping");

						} else {
							found = true;
						}

					} else {
						log_text("Error while opening file in archive");
					}

				} else {
					log_text("Error while retrieving fileinfo");
				}
			}
			unzCloseCurrentFile(cpi->uf);

#if defined(USE_THREADS)
			/* leave the monitor */

			builtin_monitorexit((java_objectheader *) cpi);
#endif

		} else {
#endif /* defined(USE_ZLIB) */
			
			path = MNEW(char, cpi->pathlen + filenamelen);
			strcpy(path, cpi->path);
			strcat(path, filename);

			classfile = fopen(path, "r");

			if (classfile) {                                   /* file exists */
				if (!stat(path, &buffer)) {            /* read classfile data */
					cb = NEW(classbuffer);
					cb->class = c;
					cb->size = buffer.st_size;
					cb->data = MNEW(u1, cb->size);
					cb->pos = cb->data - 1;

					/* read class data */
					len = fread(cb->data, 1, cb->size, classfile);

					if (len != buffer.st_size) {
						suck_stop(cb);
/*  						if (ferror(classfile)) { */
/*  						} */

					} else {
						found = true;
					}
				}
			}

			MFREE(path, char, cpi->pathlen + filenamelen);
#if defined(USE_ZLIB)
		}
#endif
	}

	if (opt_verbose)
		if (!found)
			dolog("Warning: Can not open class file '%s'", filename);

	MFREE(filename, char, filenamelen);

	return cb;
}


/************************** function suck_stop *********************************

	frees memory for buffer with classfile data.
	Caution: this function may only be called if buffer has been allocated
	         by suck_start with reading a file
	
*******************************************************************************/

void suck_stop(classbuffer *cb)
{
	/* free memory */

	MFREE(cb->data, u1, cb->size);
	FREE(cb, classbuffer);
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


/********************** Function: skipattributebody ****************************

	skips an attribute after the 16 bit reference to attribute_name has already
	been read
	
*******************************************************************************/

static bool skipattributebody(classbuffer *cb)
{
	u4 len;

	if (!check_classbuffer_size(cb, 4))
		return false;

	len = suck_u4(cb);

	if (!check_classbuffer_size(cb, len))
		return false;

	skip_nbytes(cb, len);

	return true;
}


/************************* Function: skipattributes ****************************

	skips num attribute structures
	
*******************************************************************************/

static bool skipattributes(classbuffer *cb, u4 num)
{
	u4 i;
	u4 len;

	for (i = 0; i < num; i++) {
		if (!check_classbuffer_size(cb, 2 + 4))
			return false;

		suck_u2(cb);
		len = suck_u4(cb);

		if (!check_classbuffer_size(cb, len))
			return false;

		skip_nbytes(cb, len);
	}

	return true;
}


/******************** function:: class_getconstant *****************************

	retrieves the value at position 'pos' of the constantpool of a class
	if the type of the value is other than 'ctype' the system is stopped

*******************************************************************************/

voidptr class_getconstant(classinfo *c, u4 pos, u4 ctype)
{
	/* check index and type of constantpool entry */
	/* (pos == 0 is caught by type comparison) */
	if (pos >= c->cpcount || c->cptags[pos] != ctype) {
		*exceptionptr = new_classformaterror(c, "Illegal constant pool index");
		return NULL;
	}

	return c->cpinfos[pos];
}


/******************** function: innerclass_getconstant ************************

    like class_getconstant, but if cptags is ZERO null is returned
	
*******************************************************************************/

voidptr innerclass_getconstant(classinfo *c, u4 pos, u4 ctype)
{
	/* invalid position in constantpool */
	if (pos >= c->cpcount) {
		*exceptionptr = new_classformaterror(c, "Illegal constant pool index");
		return NULL;
	}

	/* constantpool entry of type 0 */	
	if (!c->cptags[pos])
		return NULL;

	/* check type of constantpool entry */
	if (c->cptags[pos] != ctype) {
		*exceptionptr = new_classformaterror(c, "Illegal constant pool index");
		return NULL;
	}
		
	return c->cpinfos[pos];
}


/********************* Function: class_constanttype ****************************

	Determines the type of a class entry in the ConstantPool
	
*******************************************************************************/

u4 class_constanttype(classinfo *c, u4 pos)
{
	if (pos <= 0 || pos >= c->cpcount) {
		*exceptionptr = new_classformaterror(c, "Illegal constant pool index");
		return 0;
	}

	return c->cptags[pos];
}


/* load_constantpool ***********************************************************

   Loads the constantpool of a class, the entries are transformed into
   a simpler format by resolving references (a detailed overview of
   the compact structures can be found in global.h).

*******************************************************************************/

static bool load_constantpool(classbuffer *cb)
{

	/* The following structures are used to save information which cannot be 
	   processed during the first pass. After the complete constantpool has 
	   been traversed the references can be resolved. 
	   (only in specific order)						   */
	
	/* CONSTANT_Class entries */
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


	classinfo *c;
	u4 idx;

	forward_class *forward_classes = NULL;
	forward_string *forward_strings = NULL;
	forward_nameandtype *forward_nameandtypes = NULL;
	forward_fieldmethint *forward_fieldmethints = NULL;

	forward_class *nfc;
	forward_string *nfs;
	forward_nameandtype *nfn;
	forward_fieldmethint *nff;

	u4 cpcount;
	u1 *cptags;
	voidptr *cpinfos;

	c = cb->class;

	/* number of entries in the constant_pool table plus one */
	if (!check_classbuffer_size(cb, 2))
		return false;

	cpcount = c->cpcount = suck_u2(cb);

	/* allocate memory */
	cptags  = c->cptags  = MNEW(u1, cpcount);
	cpinfos = c->cpinfos = MNEW(voidptr, cpcount);

	if (cpcount < 1) {
		*exceptionptr = new_classformaterror(c, "Illegal constant pool size");
		return false;
	}
	
#if defined(STATISTICS)
	if (opt_stat)
		count_const_pool_len += (sizeof(voidptr) + 1) * cpcount;
#endif
	
	/* initialize constantpool */
	for (idx = 0; idx < cpcount; idx++) {
		cptags[idx] = CONSTANT_UNUSED;
		cpinfos[idx] = NULL;
	}

			
	/******* first pass *******/
	/* entries which cannot be resolved now are written into 
	   temporary structures and traversed again later        */
		   
	idx = 1;
	while (idx < cpcount) {
		u4 t;

		/* get constant type */
		if (!check_classbuffer_size(cb, 1))
			return false;

		t = suck_u1(cb);

		switch (t) {
		case CONSTANT_Class:
			nfc = NEW(forward_class);

			nfc->next = forward_classes;
			forward_classes = nfc;

			nfc->thisindex = idx;
			/* reference to CONSTANT_NameAndType */
			if (!check_classbuffer_size(cb, 2))
				return false;

			nfc->name_index = suck_u2(cb);

			idx++;
			break;
			
		case CONSTANT_String:
			nfs = NEW(forward_string);
				
			nfs->next = forward_strings;
			forward_strings = nfs;
				
			nfs->thisindex = idx;

			/* reference to CONSTANT_Utf8_info with string characters */
			if (!check_classbuffer_size(cb, 2))
				return false;

			nfs->string_index = suck_u2(cb);
				
			idx++;
			break;

		case CONSTANT_NameAndType:
			nfn = NEW(forward_nameandtype);
				
			nfn->next = forward_nameandtypes;
			forward_nameandtypes = nfn;
				
			nfn->thisindex = idx;

			if (!check_classbuffer_size(cb, 2 + 2))
				return false;

			/* reference to CONSTANT_Utf8_info containing simple name */
			nfn->name_index = suck_u2(cb);

			/* reference to CONSTANT_Utf8_info containing field or method
			   descriptor */
			nfn->sig_index = suck_u2(cb);
				
			idx++;
			break;

		case CONSTANT_Fieldref:
		case CONSTANT_Methodref:
		case CONSTANT_InterfaceMethodref:
			nff = NEW(forward_fieldmethint);
			
			nff->next = forward_fieldmethints;
			forward_fieldmethints = nff;

			nff->thisindex = idx;
			/* constant type */
			nff->tag = t;

			if (!check_classbuffer_size(cb, 2 + 2))
				return false;

			/* class or interface type that contains the declaration of the
			   field or method */
			nff->class_index = suck_u2(cb);

			/* name and descriptor of the field or method */
			nff->nameandtype_index = suck_u2(cb);

			idx++;
			break;
				
		case CONSTANT_Integer: {
			constant_integer *ci = NEW(constant_integer);

#if defined(STATISTICS)
			if (opt_stat)
				count_const_pool_len += sizeof(constant_integer);
#endif

			if (!check_classbuffer_size(cb, 4))
				return false;

			ci->value = suck_s4(cb);
			cptags[idx] = CONSTANT_Integer;
			cpinfos[idx] = ci;

			idx++;
			break;
		}
				
		case CONSTANT_Float: {
			constant_float *cf = NEW(constant_float);

#if defined(STATISTICS)
			if (opt_stat)
				count_const_pool_len += sizeof(constant_float);
#endif

			if (!check_classbuffer_size(cb, 4))
				return false;

			cf->value = suck_float(cb);
			cptags[idx] = CONSTANT_Float;
			cpinfos[idx] = cf;

			idx++;
			break;
		}
				
		case CONSTANT_Long: {
			constant_long *cl = NEW(constant_long);
					
#if defined(STATISTICS)
			if (opt_stat)
				count_const_pool_len += sizeof(constant_long);
#endif

			if (!check_classbuffer_size(cb, 8))
				return false;

			cl->value = suck_s8(cb);
			cptags[idx] = CONSTANT_Long;
			cpinfos[idx] = cl;
			idx += 2;
			if (idx > cpcount) {
				*exceptionptr =
					new_classformaterror(c, "Invalid constant pool entry");
				return false;
			}
			break;
		}
			
		case CONSTANT_Double: {
			constant_double *cd = NEW(constant_double);
				
#if defined(STATISTICS)
			if (opt_stat)
				count_const_pool_len += sizeof(constant_double);
#endif

			if (!check_classbuffer_size(cb, 8))
				return false;

			cd->value = suck_double(cb);
			cptags[idx] = CONSTANT_Double;
			cpinfos[idx] = cd;
			idx += 2;
			if (idx > cpcount) {
				*exceptionptr =
					new_classformaterror(c, "Invalid constant pool entry");
				return false;
			}
			break;
		}
				
		case CONSTANT_Utf8: { 
			u4 length;

			/* number of bytes in the bytes array (not string-length) */
			if (!check_classbuffer_size(cb, 2))
				return false;

			length = suck_u2(cb);
			cptags[idx] = CONSTANT_Utf8;

			/* validate the string */
			if (!check_classbuffer_size(cb, length))
				return false;

			if (opt_verify &&
				!is_valid_utf((char *) (cb->pos + 1),
							  (char *) (cb->pos + 1 + length))) {
				dolog("Invalid UTF-8 string (constant pool index %d)",idx);
				panic("Invalid UTF-8 string");
			}
			/* insert utf-string into the utf-symboltable */
			cpinfos[idx] = utf_new_intern((char *) (cb->pos + 1), length);

			/* skip bytes of the string (buffer size check above) */
			skip_nbytes(cb, length);
			idx++;
			break;
		}
										
		default:
			*exceptionptr =
				new_classformaterror(c, "Illegal constant pool type");
			return false;
		}  /* end switch */
	} /* end while */


	/* resolve entries in temporary structures */

	while (forward_classes) {
		utf *name =
			class_getconstant(c, forward_classes->name_index, CONSTANT_Utf8);

		if (opt_verify && !is_valid_name_utf(name))
			panic("Class reference with invalid name");

		cptags[forward_classes->thisindex] = CONSTANT_Class;
		/* retrieve class from class-table */
		if (opt_eager) {
			classinfo *tc;
			tc = class_new_intern(name);

			if (!load_class_from_classloader(tc, c->classloader))
				return false;

			/* link the class later, because we cannot link the class currently
			   loading */
			list_addfirst(&unlinkedclasses, tc);

			cpinfos[forward_classes->thisindex] = tc;

		} else {
			cpinfos[forward_classes->thisindex] = class_new(name);
		}

		nfc = forward_classes;
		forward_classes = forward_classes->next;
		FREE(nfc, forward_class);
	}

	while (forward_strings) {
		utf *text =
			class_getconstant(c, forward_strings->string_index, CONSTANT_Utf8);

		/* resolve utf-string */
		cptags[forward_strings->thisindex] = CONSTANT_String;
		cpinfos[forward_strings->thisindex] = text;
		
		nfs = forward_strings;
		forward_strings = forward_strings->next;
		FREE(nfs, forward_string);
	}

	while (forward_nameandtypes) {
		constant_nameandtype *cn = NEW(constant_nameandtype);	

#if defined(STATISTICS)
		if (opt_stat)
			count_const_pool_len += sizeof(constant_nameandtype);
#endif

		/* resolve simple name and descriptor */
		cn->name = class_getconstant(c,
									 forward_nameandtypes->name_index,
									 CONSTANT_Utf8);

		cn->descriptor = class_getconstant(c,
										   forward_nameandtypes->sig_index,
										   CONSTANT_Utf8);

		if (opt_verify) {
			/* check name */
			if (!is_valid_name_utf(cn->name)) {
				*exceptionptr =
					new_classformaterror(c,
										 "Illegal Field name \"%s\"",
										 cn->name->text);

				return false;
			}

			/* disallow referencing <clinit> among others */
			if (cn->name->text[0] == '<' && cn->name != utf_init) {
				*exceptionptr =
					new_exception_utfmessage(string_java_lang_InternalError,
											 cn->name);
				return false;
			}
		}

		cptags[forward_nameandtypes->thisindex] = CONSTANT_NameAndType;
		cpinfos[forward_nameandtypes->thisindex] = cn;

		nfn = forward_nameandtypes;
		forward_nameandtypes = forward_nameandtypes->next;
		FREE(nfn, forward_nameandtype);
	}

	while (forward_fieldmethints) {
		constant_nameandtype *nat;
		constant_FMIref *fmi = NEW(constant_FMIref);

#if defined(STATISTICS)
		if (opt_stat)
			count_const_pool_len += sizeof(constant_FMIref);
#endif
		/* resolve simple name and descriptor */
		nat = class_getconstant(c,
								forward_fieldmethints->nameandtype_index,
								CONSTANT_NameAndType);

		fmi->class = class_getconstant(c,
									   forward_fieldmethints->class_index,
									   CONSTANT_Class);
		fmi->name = nat->name;
		fmi->descriptor = nat->descriptor;

		cptags[forward_fieldmethints->thisindex] = forward_fieldmethints->tag;
		cpinfos[forward_fieldmethints->thisindex] = fmi;
	
		switch (forward_fieldmethints->tag) {
		case CONSTANT_Fieldref:  /* check validity of descriptor */
			checkfielddescriptor(fmi->descriptor->text,
								 utf_end(fmi->descriptor));
			break;
		case CONSTANT_InterfaceMethodref:
		case CONSTANT_Methodref: /* check validity of descriptor */
			checkmethoddescriptor(c, fmi->descriptor);
			break;
		}
	
		nff = forward_fieldmethints;
		forward_fieldmethints = forward_fieldmethints->next;
		FREE(nff, forward_fieldmethint);
	}

	/* everything was ok */

	return true;
}


/* load_field ******************************************************************

   Load everything about a class field from the class file and fill a
   'fieldinfo' structure. For static fields, space in the data segment
   is allocated.

*******************************************************************************/

#define field_load_NOVALUE  0xffffffff /* must be bigger than any u2 value! */

static bool load_field(classbuffer *cb, fieldinfo *f)
{
	classinfo *c;
	u4 attrnum, i;
	u4 jtype;
	u4 pindex = field_load_NOVALUE;     /* constantvalue_index */
	utf *u;

	c = cb->class;

	if (!check_classbuffer_size(cb, 2 + 2 + 2))
		return false;

	f->flags = suck_u2(cb);

	if (!(u = class_getconstant(c, suck_u2(cb), CONSTANT_Utf8)))
		return false;
	f->name = u;

	if (!(u = class_getconstant(c, suck_u2(cb), CONSTANT_Utf8)))
		return false;
	f->descriptor = u;

	if (opt_verify) {
		/* check name */
		if (!is_valid_name_utf(f->name) || f->name->text[0] == '<') {
			*exceptionptr = new_classformaterror(c,
												 "Illegal Field name \"%s\"",
												 f->name->text);
			return false;
		}

		/* check flag consistency */
		i = f->flags & (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED);

		if ((i != 0 && i != ACC_PUBLIC && i != ACC_PRIVATE && i != ACC_PROTECTED) ||
			((f->flags & (ACC_FINAL | ACC_VOLATILE)) == (ACC_FINAL | ACC_VOLATILE))) {
			*exceptionptr =
				new_classformaterror(c,
									 "Illegal field modifiers: 0x%X",
									 f->flags);
			return false;
		}

		if (c->flags & ACC_INTERFACE) {
			if (((f->flags & (ACC_STATIC | ACC_PUBLIC | ACC_FINAL))
				!= (ACC_STATIC | ACC_PUBLIC | ACC_FINAL)) ||
				f->flags & ACC_TRANSIENT) {
				*exceptionptr =
					new_classformaterror(c,
										 "Illegal field modifiers: 0x%X",
										 f->flags);
				return false;
			}
		}

		/* check descriptor */
		checkfielddescriptor(f->descriptor->text, utf_end(f->descriptor));
	}
		
	f->type = jtype = desc_to_type(f->descriptor);    /* data type            */
	f->offset = 0;                             /* offset from start of object */
	f->class = c;
	f->xta = NULL;
	
	switch (f->type) {
	case TYPE_INT:     f->value.i = 0; break;
	case TYPE_FLOAT:   f->value.f = 0.0; break;
	case TYPE_DOUBLE:  f->value.d = 0.0; break;
	case TYPE_ADDRESS: f->value.a = NULL; break;
	case TYPE_LONG:
#if U8_AVAILABLE
		f->value.l = 0; break;
#else
		f->value.l.low = 0; f->value.l.high = 0; break;
#endif
	}

	/* read attributes */
	if (!check_classbuffer_size(cb, 2))
		return false;

	attrnum = suck_u2(cb);
	for (i = 0; i < attrnum; i++) {
		if (!check_classbuffer_size(cb, 2))
			return false;

		if (!(u = class_getconstant(c, suck_u2(cb), CONSTANT_Utf8)))
			return false;

		if (u == utf_ConstantValue) {
			if (!check_classbuffer_size(cb, 4 + 2))
				return false;

			/* check attribute length */
			if (suck_u4(cb) != 2) {
				*exceptionptr =
					new_classformaterror(c, "Wrong size for VALUE attribute");
				return false;
			}
			
			/* constant value attribute */
			if (pindex != field_load_NOVALUE) {
				*exceptionptr =
					new_classformaterror(c,
										 "Multiple ConstantValue attributes");
				return false;
			}
			
			/* index of value in constantpool */		
			pindex = suck_u2(cb);
		
			/* initialize field with value from constantpool */		
			switch (jtype) {
			case TYPE_INT: {
				constant_integer *ci; 

				if (!(ci = class_getconstant(c, pindex, CONSTANT_Integer)))
					return false;

				f->value.i = ci->value;
			}
			break;
					
			case TYPE_LONG: {
				constant_long *cl; 

				if (!(cl = class_getconstant(c, pindex, CONSTANT_Long)))
					return false;

				f->value.l = cl->value;
			}
			break;

			case TYPE_FLOAT: {
				constant_float *cf;

				if (!(cf = class_getconstant(c, pindex, CONSTANT_Float)))
					return false;

				f->value.f = cf->value;
			}
			break;
											
			case TYPE_DOUBLE: {
				constant_double *cd;

				if (!(cd = class_getconstant(c, pindex, CONSTANT_Double)))
					return false;

				f->value.d = cd->value;
			}
			break;
						
			case TYPE_ADDRESS:
				if (!(u = class_getconstant(c, pindex, CONSTANT_String)))
					return false;

				/* create javastring from compressed utf8-string */
				f->value.a = literalstring_new(u);
				break;
	
			default: 
				log_text("Invalid Constant - Type");
			}

		} else {
			/* unknown attribute */
			if (!skipattributebody(cb))
				return false;
		}
	}

	/* everything was ok */

	return true;
}


/* load_method *****************************************************************

   Loads a method from the class file and fills an existing
   'methodinfo' structure. For native methods, the function pointer
   field is set to the real function pointer, for JavaVM methods a
   pointer to the compiler is used preliminarily.
	
*******************************************************************************/

static bool load_method(classbuffer *cb, methodinfo *m)
{
	classinfo *c;
	s4 argcount;
	s4 i, j;
	u4 attrnum;
	u4 codeattrnum;
	utf *u;

	c = cb->class;

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	initObjectLock(&m->header);
#endif

#ifdef STATISTICS
	if (opt_stat)
		count_all_methods++;
#endif

	m->thrownexceptionscount = 0;
	m->linenumbercount = 0;
	m->linenumbers = 0;
	m->class = c;
	m->nativelyoverloaded = false;
	
	if (!check_classbuffer_size(cb, 2 + 2 + 2))
		return false;

	m->flags = suck_u2(cb);

	if (!(u = class_getconstant(c, suck_u2(cb), CONSTANT_Utf8)))
		return false;
	m->name = u;

	if (!(u = class_getconstant(c, suck_u2(cb), CONSTANT_Utf8)))
		return false;
	m->descriptor = u;

	if (opt_verify) {
		if (!is_valid_name_utf(m->name))
			panic("Method with invalid name");

		if (m->name->text[0] == '<'
			&& m->name != utf_init && m->name != utf_clinit)
			panic("Method with invalid special name");
	}
	
	argcount = checkmethoddescriptor(c, m->descriptor);

	if (!(m->flags & ACC_STATIC))
		argcount++; /* count the 'this' argument */

	if (opt_verify) {
		if (argcount > 255) {
			*exceptionptr =
				new_classformaterror(c, "Too many arguments in signature");
			return false;
		}

		/* check flag consistency */
		if (m->name != utf_clinit) {
			i = (m->flags & (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED));

			if (i != 0 && i != ACC_PUBLIC && i != ACC_PRIVATE && i != ACC_PROTECTED) {
				*exceptionptr =
					new_classformaterror(c,
										 "Illegal method modifiers: 0x%X",
										 m->flags);
				return false;
			}

			if (m->flags & ACC_ABSTRACT) {
				if ((m->flags & (ACC_FINAL | ACC_NATIVE | ACC_PRIVATE |
								 ACC_STATIC | ACC_STRICT | ACC_SYNCHRONIZED))) {
					*exceptionptr =
						new_classformaterror(c,
											 "Illegal method modifiers: 0x%X",
											 m->flags);
					return false;
				}
			}

			if (c->flags & ACC_INTERFACE) {
				if ((m->flags & (ACC_ABSTRACT | ACC_PUBLIC)) != (ACC_ABSTRACT | ACC_PUBLIC)) {
					*exceptionptr =
						new_classformaterror(c,
											 "Illegal method modifiers: 0x%X",
											 m->flags);
					return false;
				}
			}

			if (m->name == utf_init) {
				if (m->flags & (ACC_STATIC | ACC_FINAL | ACC_SYNCHRONIZED |
								ACC_NATIVE | ACC_ABSTRACT))
					panic("Instance initialization method has invalid flags set");
			}
		}
	}
		
	m->jcode = NULL;
	m->basicblockcount = 0;
	m->basicblocks = NULL;
	m->basicblockindex = NULL;
	m->instructioncount = 0;
	m->instructions = NULL;
	m->stackcount = 0;
	m->stack = NULL;
	m->exceptiontable = NULL;
	m->stubroutine = NULL;
	m->mcode = NULL;
	m->entrypoint = NULL;
	m->methodUsed = NOTUSED;    
	m->monoPoly = MONO;    
	m->subRedefs = 0;
	m->subRedefsUsed = 0;

	m->xta = NULL;

	if (!check_classbuffer_size(cb, 2))
		return false;
	
	attrnum = suck_u2(cb);
	for (i = 0; i < attrnum; i++) {
		utf *aname;

		if (!check_classbuffer_size(cb, 2))
			return false;

		if (!(aname = class_getconstant(c, suck_u2(cb), CONSTANT_Utf8)))
			return false;

		if (aname == utf_Code) {
			if (m->flags & (ACC_ABSTRACT | ACC_NATIVE)) {
					*exceptionptr =
						new_classformaterror(c,
											 "Code attribute in native or abstract methods");

					return false;
			}
			
			if (m->jcode) {
				*exceptionptr =
					new_classformaterror(c, "Multiple Code attributes");

				return false;
			}

			if (!check_classbuffer_size(cb, 4 + 2 + 2))
				return false;

			suck_u4(cb);
			m->maxstack = suck_u2(cb);
			m->maxlocals = suck_u2(cb);

			if (m->maxlocals < argcount) {
				*exceptionptr =
					new_classformaterror(c, "Arguments can't fit into locals");

				return false;
			}
			
			if (!check_classbuffer_size(cb, 4))
				return false;

			m->jcodelength = suck_u4(cb);

			if (m->jcodelength == 0) {
				*exceptionptr =
					new_classformaterror(c, "Code of a method has length 0");

				return false;
			}
			
			if (m->jcodelength > 65535) {
				*exceptionptr =
					new_classformaterror(c,
										 "Code of a method longer than 65535 bytes");

				return false;
			}

			if (!check_classbuffer_size(cb, m->jcodelength))
				return false;

			m->jcode = MNEW(u1, m->jcodelength);
			suck_nbytes(m->jcode, cb, m->jcodelength);

			if (!check_classbuffer_size(cb, 2))
				return false;

			m->exceptiontablelength = suck_u2(cb);
			if (!check_classbuffer_size(cb, (2 + 2 + 2 + 2) * m->exceptiontablelength))
				return false;

			m->exceptiontable = MNEW(exceptiontable, m->exceptiontablelength);

#if defined(STATISTICS)
			if (opt_stat) {
				count_vmcode_len += m->jcodelength + 18;
				count_extable_len += 8 * m->exceptiontablelength;
			}
#endif

			for (j = 0; j < m->exceptiontablelength; j++) {
				u4 idx;
				m->exceptiontable[j].startpc = suck_u2(cb);
				m->exceptiontable[j].endpc = suck_u2(cb);
				m->exceptiontable[j].handlerpc = suck_u2(cb);

				idx = suck_u2(cb);
				if (!idx) {
					m->exceptiontable[j].catchtype = NULL;

				} else {
					if (!(m->exceptiontable[j].catchtype =
						  class_getconstant(c, idx, CONSTANT_Class)))
						return false;
				}
			}

			if (!check_classbuffer_size(cb, 2))
				return false;

			codeattrnum = suck_u2(cb);

			for (; codeattrnum > 0; codeattrnum--) {
				utf *caname;

				if (!check_classbuffer_size(cb, 2))
					return false;

				if (!(caname = class_getconstant(c, suck_u2(cb), CONSTANT_Utf8)))
					return false;

				if (caname == utf_LineNumberTable) {
					u2 lncid;

					if (!check_classbuffer_size(cb, 4 + 2))
						return false;

					suck_u4(cb);
					m->linenumbercount = suck_u2(cb);

					if (!check_classbuffer_size(cb,
												(2 + 2) * m->linenumbercount))
						return false;

					m->linenumbers = MNEW(lineinfo, m->linenumbercount);
					
					for (lncid = 0; lncid < m->linenumbercount; lncid++) {
						m->linenumbers[lncid].start_pc = suck_u2(cb);
						m->linenumbers[lncid].line_number = suck_u2(cb);
					}
					codeattrnum--;

					if (!skipattributes(cb, codeattrnum))
						return false;
					
					break;

				} else {
					if (!skipattributebody(cb))
						return false;
				}
			}

		} else if (aname == utf_Exceptions) {
			s4 j;

			if (m->thrownexceptions) {
				*exceptionptr =
					new_classformaterror(c, "Multiple Exceptions attributes");
				return false;
			}

			if (!check_classbuffer_size(cb, 4 + 2))
				return false;

			suck_u4(cb); /* length */
			m->thrownexceptionscount = suck_u2(cb);

			if (!check_classbuffer_size(cb, 2 * m->thrownexceptionscount))
				return false;

			m->thrownexceptions = MNEW(classinfo*, m->thrownexceptionscount);

			for (j = 0; j < m->thrownexceptionscount; j++) {
				if (!((m->thrownexceptions)[j] =
					  class_getconstant(c, suck_u2(cb), CONSTANT_Class)))
					return false;
			}
				
		} else {
			if (!skipattributebody(cb))
				return false;
		}
	}

	if (!m->jcode && !(m->flags & (ACC_ABSTRACT | ACC_NATIVE))) {
		*exceptionptr = new_classformaterror(c, "Missing Code attribute");

		return false;
	}

	/* everything was ok */

	return true;
}


/* load_attribute **************************************************************

   Read attributes from classfile.
	
*******************************************************************************/

static bool load_attributes(classbuffer *cb, u4 num)
{
	classinfo *c;
	utf       *aname;
	u4 i, j;

	c = cb->class;

	for (i = 0; i < num; i++) {
		/* retrieve attribute name */
		if (!check_classbuffer_size(cb, 2))
			return false;

		if (!(aname = class_getconstant(c, suck_u2(cb), CONSTANT_Utf8)))
			return false;

		if (aname == utf_InnerClasses) {
			/* innerclasses attribute */
			if (c->innerclass) {
				*exceptionptr =
					new_classformaterror(c, "Multiple InnerClasses attributes");
				return false;
			}
				
			if (!check_classbuffer_size(cb, 4 + 2))
				return false;

			/* skip attribute length */
			suck_u4(cb);

			/* number of records */
			c->innerclasscount = suck_u2(cb);

			if (!check_classbuffer_size(cb, (2 + 2 + 2 + 2) * c->innerclasscount))
				return false;

			/* allocate memory for innerclass structure */
			c->innerclass = MNEW(innerclassinfo, c->innerclasscount);

			for (j = 0; j < c->innerclasscount; j++) {
				/* The innerclass structure contains a class with an encoded
				   name, its defining scope, its simple name and a bitmask of
				   the access flags. If an inner class is not a member, its
				   outer_class is NULL, if a class is anonymous, its name is
				   NULL. */
   								
				innerclassinfo *info = c->innerclass + j;

				info->inner_class =
					innerclass_getconstant(c, suck_u2(cb), CONSTANT_Class);
				info->outer_class =
					innerclass_getconstant(c, suck_u2(cb), CONSTANT_Class);
				info->name =
					innerclass_getconstant(c, suck_u2(cb), CONSTANT_Utf8);
				info->flags = suck_u2(cb);
			}

		} else if (aname == utf_SourceFile) {
			if (!check_classbuffer_size(cb, 4 + 2))
				return false;

			if (suck_u4(cb) != 2) {
				*exceptionptr =
					new_classformaterror(c, "Wrong size for VALUE attribute");
				return false;
			}

			if (c->sourcefile) {
				*exceptionptr =
					new_classformaterror(c, "Multiple SourceFile attributes");
				return false;
			}

			if (!(c->sourcefile = class_getconstant(c, suck_u2(cb), CONSTANT_Utf8)))
				return false;

		} else {
			/* unknown attribute */
			if (!skipattributebody(cb))
				return false;
		}
	}

	return true;
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


/* load_class_from_classloader *************************************************

   XXX
	
*******************************************************************************/

classinfo *load_class_from_classloader(classinfo *c, java_objectheader *cl)
{
	classinfo *r;

	/* if other class loader than bootstrap, call it */

	if (cl) {
		methodinfo *lc;

		lc = class_resolveclassmethod(cl->vftbl->class,
									  utf_loadClass,
									  utf_java_lang_String__java_lang_Class,
									  class_java_lang_Object,
									  true);

		if (!lc)
			return NULL;

		r = (classinfo *) asm_calljavafunction(lc,
											   cl,
											   javastring_new(c->name),
											   NULL, NULL);

		return r;

	} else {
		return load_class_bootstrap(c);
	}
}


/* load_class_bootstrap ********************************************************
	
   XXX

*******************************************************************************/

classinfo *load_class_bootstrap(classinfo *c)
{
	classbuffer *cb;
	classinfo *r;

#if defined(USE_THREADS)
	/* enter a monitor on the class */

	builtin_monitorenter((java_objectheader *) c);
#endif

	/* maybe the class is already loaded */

	if (c->loaded) {
#if defined(USE_THREADS)
		builtin_monitorexit((java_objectheader *) c);
#endif

		return c;
	}

#if defined(STATISTICS)
	/* measure time */

	if (getcompilingtime)
		compilingtime_stop();

	if (getloadingtime)
		loadingtime_start();
#endif

	/* load classdata, throw exception on error */

	if ((cb = suck_start(c)) == NULL) {
		/* this means, the classpath was not set properly */
		if (c->name == utf_java_lang_Object)
			throw_cacao_exception_exit(string_java_lang_NoClassDefFoundError,
									   "java/lang/Object");

		*exceptionptr =
			new_exception_utfmessage(string_java_lang_NoClassDefFoundError,
									 c->name);

#if defined(USE_THREADS)
		builtin_monitorexit((java_objectheader *) c);
#endif

		return NULL;
	}
	
	/* load the class from the buffer */

	r = load_class_from_classbuffer(cb);

	/* if return value is NULL, we had a problem and the class is not loaded */
	if (!r) {
		c->loaded = false;

		/* now free the allocated memory, otherwise we could ran into a DOS */
		class_remove(c);
	}

	/* free memory */
	suck_stop(cb);

#if defined(STATISTICS)
	/* measure time */

	if (getloadingtime)
		loadingtime_stop();

	if (getcompilingtime)
		compilingtime_start();
#endif

#if defined(USE_THREADS)
	/* leave the monitor */

	builtin_monitorexit((java_objectheader *) c);
#endif

	return r;
}


/* load_class_from_classbuffer *************************************************
	
   Loads everything interesting about a class from the class file. The
   'classinfo' structure must have been allocated previously.

   The super class and the interfaces implemented by this class need
   not be loaded. The link is set later by the function 'class_link'.

   The loaded class is removed from the list 'unloadedclasses' and
   added to the list 'unlinkedclasses'.
	
*******************************************************************************/

classinfo *load_class_from_classbuffer(classbuffer *cb)
{
	classinfo *c;
	classinfo *tc;
	u4 i;
	u4 ma, mi;
	char msg[MAXLOGTEXT];               /* maybe we get an exception */

	/* get the classbuffer's class */
	c = cb->class;

	/* maybe the class is already loaded */
	if (c->loaded)
		return c;

#if defined(STATISTICS)
	if (opt_stat)
		count_class_loads++;
#endif

	/* output for debugging purposes */
	if (loadverbose)
		log_message_class("Loading class: ", c);
	
	/* class is somewhat loaded */
	c->loaded = true;

	if (!check_classbuffer_size(cb, 4 + 2 + 2))
		return NULL;

	/* check signature */
	if (suck_u4(cb) != MAGIC) {
		*exceptionptr = new_classformaterror(c, "Bad magic number");

		return NULL;
	}

	/* check version */
	mi = suck_u2(cb);
	ma = suck_u2(cb);

	if (!(ma < MAJOR_VERSION || (ma == MAJOR_VERSION && mi <= MINOR_VERSION))) {
		*exceptionptr =
			new_unsupportedclassversionerror(c,
											 "Unsupported major.minor version %d.%d",
											 ma, mi);

		return NULL;
	}

	/* load the constant pool */
	if (!load_constantpool(cb))
		return NULL;

	/*JOWENN*/
	c->erroneous_state = 0;
	c->initializing_thread = 0;	
	/*JOWENN*/
	c->classUsed = NOTUSED; /* not used initially CO-RT */
	c->impldBy = NULL;

	/* ACC flags */
	if (!check_classbuffer_size(cb, 2))
		return NULL;

	c->flags = suck_u2(cb);
	/*if (!(c->flags & ACC_PUBLIC)) { log_text("CLASS NOT PUBLIC"); } JOWENN*/

	/* check ACC flags consistency */
	if (c->flags & ACC_INTERFACE) {
		if (!(c->flags & ACC_ABSTRACT)) {
			/* We work around this because interfaces in JDK 1.1 are
			 * not declared abstract. */

			c->flags |= ACC_ABSTRACT;
			/* panic("Interface class not declared abstract"); */
		}

		if (c->flags & ACC_FINAL) {
			*exceptionptr =
				new_classformaterror(c,
									 "Illegal class modifiers: 0x%X", c->flags);

			return NULL;
		}

		if (c->flags & ACC_SUPER) {
			c->flags &= ~ACC_SUPER; /* kjc seems to set this on interfaces */
		}
	}

	if ((c->flags & (ACC_ABSTRACT | ACC_FINAL)) == (ACC_ABSTRACT | ACC_FINAL)) {
		*exceptionptr =
			new_classformaterror(c, "Illegal class modifiers: 0x%X", c->flags);

		return NULL;
	}

	if (!check_classbuffer_size(cb, 2 + 2))
		return NULL;

	/* this class */
	i = suck_u2(cb);
	if (!(tc = class_getconstant(c, i, CONSTANT_Class)))
		return NULL;

	if (tc != c) {
		utf_sprint(msg, c->name);
		sprintf(msg + strlen(msg), " (wrong name: ");
		utf_sprint(msg + strlen(msg), tc->name);
		sprintf(msg + strlen(msg), ")");

		*exceptionptr =
			new_exception_message(string_java_lang_NoClassDefFoundError, msg);

		return NULL;
	}
	
	/* retrieve superclass */
	if ((i = suck_u2(cb))) {
		if (!(c->super = class_getconstant(c, i, CONSTANT_Class)))
			return NULL;

		/* java.lang.Object may not have a super class. */
		if (c->name == utf_java_lang_Object) {
			*exceptionptr =
				new_exception_message(string_java_lang_ClassFormatError,
									  "java.lang.Object with superclass");

			return NULL;
		}

		/* Interfaces must have java.lang.Object as super class. */
		if ((c->flags & ACC_INTERFACE) &&
			c->super->name != utf_java_lang_Object) {
			*exceptionptr =
				new_exception_message(string_java_lang_ClassFormatError,
									  "Interfaces must have java.lang.Object as superclass");

			return NULL;
		}

	} else {
		c->super = NULL;

		/* This is only allowed for java.lang.Object. */
		if (c->name != utf_java_lang_Object) {
			*exceptionptr = new_classformaterror(c, "Bad superclass index");

			return NULL;
		}
	}
			 
	/* retrieve interfaces */
	if (!check_classbuffer_size(cb, 2))
		return NULL;

	c->interfacescount = suck_u2(cb);

	if (!check_classbuffer_size(cb, 2 * c->interfacescount))
		return NULL;

	c->interfaces = MNEW(classinfo*, c->interfacescount);
	for (i = 0; i < c->interfacescount; i++) {
		if (!(c->interfaces[i] = class_getconstant(c, suck_u2(cb), CONSTANT_Class)))
			return NULL;
	}

	/* load fields */
	if (!check_classbuffer_size(cb, 2))
		return NULL;

	c->fieldscount = suck_u2(cb);
	c->fields = GCNEW(fieldinfo, c->fieldscount);
/*  	c->fields = MNEW(fieldinfo, c->fieldscount); */
	for (i = 0; i < c->fieldscount; i++) {
		if (!load_field(cb, &(c->fields[i])))
			return NULL;
	}

	/* load methods */
	if (!check_classbuffer_size(cb, 2))
		return NULL;

	c->methodscount = suck_u2(cb);
/*  	c->methods = GCNEW(methodinfo, c->methodscount); */
	c->methods = MNEW(methodinfo, c->methodscount);
	for (i = 0; i < c->methodscount; i++) {
		if (!load_method(cb, &(c->methods[i])))
			return NULL;
	}

	/* Check if all fields and methods can be uniquely
	 * identified by (name,descriptor). */
	if (opt_verify) {
		/* We use a hash table here to avoid making the
		 * average case quadratic in # of methods, fields.
		 */
		static int shift = 0;
		u2 *hashtab;
		u2 *next; /* for chaining colliding hash entries */
		size_t len;
		size_t hashlen;
		u2 index;
		u2 old;

		/* Allocate hashtable */
		len = c->methodscount;
		if (len < c->fieldscount) len = c->fieldscount;
		hashlen = 5 * len;
		hashtab = MNEW(u2,(hashlen + len));
		next = hashtab + hashlen;

		/* Determine bitshift (to get good hash values) */
		if (!shift) {
			len = sizeof(utf);
			while (len) {
				len >>= 1;
				shift++;
			}
		}

		/* Check fields */
		memset(hashtab, 0, sizeof(u2) * (hashlen + len));

		for (i = 0; i < c->fieldscount; ++i) {
			fieldinfo *fi = c->fields + i;

			/* It's ok if we lose bits here */
			index = ((((size_t) fi->name) +
					  ((size_t) fi->descriptor)) >> shift) % hashlen;

			if ((old = hashtab[index])) {
				old--;
				next[i] = old;
				do {
					if (c->fields[old].name == fi->name &&
						c->fields[old].descriptor == fi->descriptor) {
						*exceptionptr =
							new_classformaterror(c,
												 "Repetitive field name/signature");

						return NULL;
					}
				} while ((old = next[old]));
			}
			hashtab[index] = i + 1;
		}
		
		/* Check methods */
		memset(hashtab, 0, sizeof(u2) * (hashlen + hashlen/5));

		for (i = 0; i < c->methodscount; ++i) {
			methodinfo *mi = c->methods + i;

			/* It's ok if we lose bits here */
			index = ((((size_t) mi->name) +
					  ((size_t) mi->descriptor)) >> shift) % hashlen;

			/*{ JOWENN
				int dbg;
				for (dbg=0;dbg<hashlen+hashlen/5;++dbg){
					printf("Hash[%d]:%d\n",dbg,hashtab[dbg]);
				}
			}*/

			if ((old = hashtab[index])) {
				old--;
				next[i] = old;
				do {
					if (c->methods[old].name == mi->name &&
						c->methods[old].descriptor == mi->descriptor) {
						*exceptionptr =
							new_classformaterror(c,
												 "Repetitive method name/signature");

						return NULL;
					}
				} while ((old = next[old]));
			}
			hashtab[index] = i + 1;
		}
		
		MFREE(hashtab, u2, (hashlen + len));
	}

#if defined(STATISTICS)
	if (opt_stat) {
		count_class_infos += sizeof(classinfo*) * c->interfacescount;
		count_class_infos += sizeof(fieldinfo) * c->fieldscount;
		count_class_infos += sizeof(methodinfo) * c->methodscount;
	}
#endif

	/* load attribute structures */
	if (!check_classbuffer_size(cb, 2))
		return NULL;

	if (!load_attributes(cb, suck_u2(cb)))
		return NULL;

#if 0
	/* Pre java 1.5 version don't check this. This implementation is like
	   java 1.5 do it: for class file version 45.3 we don't check it, older
	   versions are checked.
	 */
	if ((ma == 45 && mi > 3) || ma > 45) {
		/* check if all data has been read */
		s4 classdata_left = ((cb->data + cb->size) - cb->pos - 1);

		if (classdata_left > 0) {
			*exceptionptr =
				new_classformaterror(c, "Extra bytes at the end of class file");
			return NULL;
		}
	}
#endif

	if (loadverbose)
		log_message_class("Loading done class: ", c);

	return c;
}



/******************* Function: class_new_array *********************************

    This function is called by class_new to setup an array class.

*******************************************************************************/

void class_new_array(classinfo *c)
{
	classinfo *comp = NULL;
	methodinfo *clone;
	int namelen;

	/* Check array class name */
	namelen = c->name->blength;
	if (namelen < 2 || c->name->text[0] != '[')
		panic("Invalid array class name");

	/* Check the component type */
	switch (c->name->text[1]) {
	case '[':
		/* c is an array of arrays. We have to create the component class. */
		if (opt_eager) {
			comp = class_new_intern(utf_new_intern(c->name->text + 1,
												   namelen - 1));
			load_class_from_classloader(comp, c->classloader);
			list_addfirst(&unlinkedclasses, comp);

		} else {
			comp = class_new(utf_new_intern(c->name->text + 1, namelen - 1));
		}
		break;

	case 'L':
		/* c is an array of objects. */
		if (namelen < 4 || c->name->text[namelen - 1] != ';')
			panic("Invalid array class name");

		if (opt_eager) {
			comp = class_new_intern(utf_new_intern(c->name->text + 2,
												   namelen - 3));
			load_class_from_classloader(comp, c->classloader);
			list_addfirst(&unlinkedclasses, comp);

		} else {
			comp = class_new(utf_new_intern(c->name->text + 2, namelen - 3));
		}
		break;
	}

	/* Setup the array class */
	c->super = class_java_lang_Object;
	c->flags = ACC_PUBLIC | ACC_FINAL | ACC_ABSTRACT;

    c->interfacescount = 2;
    c->interfaces = MNEW(classinfo*, 2);

	if (opt_eager) {
		classinfo *tc;

		tc = class_java_lang_Cloneable;
		load_class_bootstrap(tc);
		list_addfirst(&unlinkedclasses, tc);
		c->interfaces[0] = tc;

		tc = class_java_io_Serializable;
		load_class_bootstrap(tc);
		list_addfirst(&unlinkedclasses, tc);
		c->interfaces[1] = tc;

	} else {
		c->interfaces[0] = class_java_lang_Cloneable;
		c->interfaces[1] = class_java_io_Serializable;
	}

	c->methodscount = 1;
	c->methods = MNEW(methodinfo, c->methodscount);

	clone = c->methods;
	MSET(clone, 0, methodinfo, 1);
	clone->flags = ACC_PUBLIC;
	clone->name = utf_new_char("clone");
	clone->descriptor = utf_void__java_lang_Object;
	clone->class = c;
	clone->stubroutine = createnativestub((functionptr) &builtin_clone_array, clone);
	clone->monoPoly = MONO;

	/* XXX: field: length? */

	/* array classes are not loaded from class files */
	c->loaded = true;
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


/****************** Function: class_resolvefield_int ***************************

    This is an internally used helper function. Do not use this directly.

	Tries to resolve a field having the given name and type.
    If the field cannot be resolved, NULL is returned.

*******************************************************************************/

static fieldinfo *class_resolvefield_int(classinfo *c, utf *name, utf *desc)
{
	fieldinfo *fi;
	s4         i;

	/* search for field in class c */

	for (i = 0; i < c->fieldscount; i++) { 
		if ((c->fields[i].name == name) && (c->fields[i].descriptor == desc)) {
			return &(c->fields[i]);
		}
    }

	/* try superinterfaces recursively */

	for (i = 0; i < c->interfacescount; i++) {
		fi = class_resolvefield_int(c->interfaces[i], name, desc);
		if (fi)
			return fi;
	}

	/* try superclass */

	if (c->super)
		return class_resolvefield_int(c->super, name, desc);

	/* not found */

	return NULL;
}


/********************* Function: class_resolvefield ***************************
	
	Resolves a reference from REFERER to a field with NAME and DESC in class C.

    If the field cannot be resolved the return value is NULL. If EXCEPT is
    true *exceptionptr is set, too.

*******************************************************************************/

fieldinfo *class_resolvefield(classinfo *c, utf *name, utf *desc,
							  classinfo *referer, bool except)
{
	fieldinfo *fi;

	/* XXX resolve class c */
	/* XXX check access from REFERER to C */
	
	fi = class_resolvefield_int(c, name, desc);

	if (!fi) {
		if (except)
			*exceptionptr =
				new_exception_utfmessage(string_java_lang_NoSuchFieldError,
										 name);

		return NULL;
	}

	/* XXX check access rights */

	return fi;
}


/* class_findmethod ************************************************************
	
   Searches a 'classinfo' structure for a method having the given name
   and descriptor. If descriptor is NULL, it is ignored.

*******************************************************************************/

methodinfo *class_findmethod(classinfo *c, utf *name, utf *desc)
{
	methodinfo *m;
	s4 i;

	for (i = 0; i < c->methodscount; i++) {
		m = &(c->methods[i]);

		if ((m->name == name) && ((desc == NULL) || (m->descriptor == desc)))
			return m;
	}

	return NULL;
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


/* class_resolvemethod *********************************************************
	
   Searches a class and it's super classes for a method.

*******************************************************************************/

methodinfo *class_resolvemethod(classinfo *c, utf *name, utf *desc)
{
	methodinfo *m;

	while (c) {
		m = class_findmethod(c, name, desc);

		if (m)
			return m;

		c = c->super;
	}

	return NULL;
}


/* class_resolveinterfacemethod_intern *****************************************

   Internally used helper function. Do not use this directly.

*******************************************************************************/

static methodinfo *class_resolveinterfacemethod_intern(classinfo *c,
													   utf *name, utf *desc)
{
	methodinfo *m;
	s4 i;
	
	m = class_findmethod(c, name, desc);

	if (m)
		return m;

	/* try the superinterfaces */

	for (i = 0; i < c->interfacescount; i++) {
		m = class_resolveinterfacemethod_intern(c->interfaces[i], name, desc);

		if (m)
			return m;
	}
	
	return NULL;
}

/* class_resolveinterfacemethod ************************************************

   Resolves a reference from REFERER to a method with NAME and DESC in
   interface C.

   If the method cannot be resolved the return value is NULL. If
   EXCEPT is true *exceptionptr is set, too.

*******************************************************************************/

methodinfo *class_resolveinterfacemethod(classinfo *c, utf *name, utf *desc,
										 classinfo *referer, bool except)
{
	methodinfo *mi;

	/* XXX resolve class c */
	/* XXX check access from REFERER to C */
	
	if (!(c->flags & ACC_INTERFACE)) {
		if (except)
			*exceptionptr =
				new_exception(string_java_lang_IncompatibleClassChangeError);

		return NULL;
	}

	mi = class_resolveinterfacemethod_intern(c, name, desc);

	if (mi)
		return mi;

	/* try class java.lang.Object */
	mi = class_findmethod(class_java_lang_Object, name, desc);

	if (mi)
		return mi;

	if (except)
		*exceptionptr =
			new_exception_utfmessage(string_java_lang_NoSuchMethodError, name);

	return NULL;
}


/* class_resolveclassmethod ****************************************************
	
    Resolves a reference from REFERER to a method with NAME and DESC in
    class C.

    If the method cannot be resolved the return value is NULL. If EXCEPT is
    true *exceptionptr is set, too.

*******************************************************************************/

methodinfo *class_resolveclassmethod(classinfo *c, utf *name, utf *desc,
									 classinfo *referer, bool except)
{
	classinfo  *cls;
	methodinfo *mi;
	s4          i;
	char       *msg;
	s4          msglen;

	/* XXX resolve class c */
	/* XXX check access from REFERER to C */
	
/*  	if (c->flags & ACC_INTERFACE) { */
/*  		if (except) */
/*  			*exceptionptr = */
/*  				new_exception(string_java_lang_IncompatibleClassChangeError); */
/*  		return NULL; */
/*  	} */

	/* try class c and its superclasses */

	cls = c;

	while (cls) {
		mi = class_findmethod(cls, name, desc);

		if (mi)
			goto found;

		cls = cls->super;
	}

	/* try the superinterfaces */

	for (i = 0; i < c->interfacescount; i++) {
		mi = class_resolveinterfacemethod_intern(c->interfaces[i], name, desc);

		if (mi)
			goto found;
	}
	
	if (except) {
		msglen = utf_strlen(c->name) + strlen(".") + utf_strlen(name) +
			utf_strlen(desc) + strlen("0");

		msg = MNEW(char, msglen);

		utf_sprint(msg, c->name);
		strcat(msg, ".");
		utf_sprint(msg + strlen(msg), name);
		utf_sprint(msg + strlen(msg), desc);

		*exceptionptr =
			new_exception_message(string_java_lang_NoSuchMethodError, msg);

		MFREE(msg, char, msglen);
	}

	return NULL;

 found:
	if ((mi->flags & ACC_ABSTRACT) && !(c->flags & ACC_ABSTRACT)) {
		if (except)
			*exceptionptr = new_exception(string_java_lang_AbstractMethodError);

		return NULL;
	}

	/* XXX check access rights */

	return mi;
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

static classinfo *class_init_intern(classinfo *c);

classinfo *class_init(classinfo *c)
{
	classinfo *r;

	if (!makeinitializations)
		return c;

#if defined(USE_THREADS)
	/* enter a monitor on the class */

	builtin_monitorenter((java_objectheader *) c);
#endif

	/* maybe the class is already initalized or the current thread, which can
	   pass the monitor, is currently initalizing this class */

	/* JOWENN: In future we need an additinal flag: initializationfailed,
		since further access to the class should cause a NoClassDefFound,
		if the static initializer failed once
        */

	if (c->initialized || c->initializing) {
#if defined(USE_THREADS)
		builtin_monitorexit((java_objectheader *) c);
#endif

		return c;
	}

	/* this initalizing run begins NOW */
	c->initializing = true;

	/* call the internal function */
	r = class_init_intern(c);

	/* if return value is not NULL everything was ok and the class is
	   initialized */
	if (r)
		c->initialized = true;

	/* this initalizing run is done */
	c->initializing = false;

#if defined(USE_THREADS)
	/* leave the monitor */

	builtin_monitorexit((java_objectheader *) c);
#endif

	return r;
}


/* this function MUST NOT be called directly, because of thread <clinit>
   race conditions */

static classinfo *class_init_intern(classinfo *c)
{
	methodinfo *m;
	s4 i;
#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
	int b;
#endif

	/* maybe the class is not already loaded */
	if (!c->loaded)
		if (!load_class_bootstrap(c))
			return NULL;

	/* maybe the class is not already linked */
	if (!c->linked)
		if (!link_class(c))
			return NULL;

#if defined(STATISTICS)
	if (opt_stat)
		count_class_inits++;
#endif

	/* initialize super class */

	if (c->super) {
		if (!c->super->initialized) {
			if (initverbose) {
				char logtext[MAXLOGTEXT];
				sprintf(logtext, "Initialize super class ");
				utf_sprint_classname(logtext + strlen(logtext), c->super->name);
				sprintf(logtext + strlen(logtext), " from ");
				utf_sprint_classname(logtext + strlen(logtext), c->name);
				log_text(logtext);
			}

			if (!class_init(c->super))
				return NULL;
		}
	}

	/* initialize interface classes */

	for (i = 0; i < c->interfacescount; i++) {
		if (!c->interfaces[i]->initialized) {
			if (initverbose) {
				char logtext[MAXLOGTEXT];
				sprintf(logtext, "Initialize interface class ");
				utf_sprint_classname(logtext + strlen(logtext), c->interfaces[i]->name);
				sprintf(logtext + strlen(logtext), " from ");
				utf_sprint_classname(logtext + strlen(logtext), c->name);
				log_text(logtext);
			}
			
			if (!class_init(c->interfaces[i]))
				return NULL;
		}
	}

	m = class_findmethod(c, utf_clinit, utf_void__void);

	if (!m) {
		if (initverbose) {
			char logtext[MAXLOGTEXT];
			sprintf(logtext, "Class ");
			utf_sprint_classname(logtext + strlen(logtext), c->name);
			sprintf(logtext + strlen(logtext), " has no static class initializer");
			log_text(logtext);
		}

		return c;
	}

	/* Sun's and IBM's JVM don't care about the static flag */
/*  	if (!(m->flags & ACC_STATIC)) { */
/*  		panic("Class initializer is not static!"); */

	if (initverbose)
		log_message_class("Starting static class initializer for class: ", c);

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
	b = blockInts;
	blockInts = 0;
#endif

	/* now call the initializer */
	asm_calljavafunction(m, NULL, NULL, NULL, NULL);

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
	assert(blockInts == 0);
	blockInts = b;
#endif

	/* we have an exception or error */
	if (*exceptionptr) {
		/* class is NOT initialized */
		c->initialized = false;

		/* is this an exception, than wrap it */
		if (builtin_instanceof(*exceptionptr, class_java_lang_Exception)) {
			java_objectheader *xptr;
			java_objectheader *cause;

			/* get the cause */
			cause = *exceptionptr;

			/* clear exception, because we are calling jit code again */
			*exceptionptr = NULL;

			/* wrap the exception */
			xptr =
				new_exception_throwable(string_java_lang_ExceptionInInitializerError,
										(java_lang_Throwable *) cause);

			/* XXX should we exit here? */
			if (*exceptionptr)
				throw_exception();

			/* set new exception */
			*exceptionptr = xptr;
		}

		return NULL;
	}

	if (initverbose)
		log_message_class("Finished static class initializer for class: ", c);

	return c;
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

*******************************************************************************/

classinfo *class_from_descriptor(char *utf_ptr, char *end_ptr,
								 char **next, int mode)
{
	char *start = utf_ptr;
	bool error = false;
	utf *name;

	SKIP_FIELDDESCRIPTOR_SAFE(utf_ptr, end_ptr, error);

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
			  name = utf_new(start, utf_ptr - start);
			  if (opt_eager) {
				  classinfo *tc;

				  tc = class_new_intern(name);
				  load_class_from_classloader(tc, NULL);
				  list_addfirst(&unlinkedclasses, tc);

				  return tc;

			  } else {
				  return (mode & CLASSLOAD_LOAD)
					  ? load_class_from_classloader(class_new(name), NULL) : class_new(name); /* XXX handle errors */
			  }
		}
	}

	/* An error occurred */
	if (mode & CLASSLOAD_NOPANIC)
		return NULL;

	log_plain("Invalid descriptor at beginning of '");
	log_plain_utf(utf_new(start, end_ptr - start));
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


/******************** Function: loader_close ***********************************

	Frees all resources
	
*******************************************************************************/

void loader_close()
{
	classinfo *c;
	s4 slot;

	for (slot = 0; slot < class_hash.size; slot++) {
		c = class_hash.ptr[slot];

		while (c) {
			class_free(c);
			c = c->hashlink;
		}
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
