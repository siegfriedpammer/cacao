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

   $Id: loader.c 2680 2005-06-14 17:14:08Z twisti $

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
#include "vm/classcache.h"

#if defined(USE_ZLIB)
# include "vm/unzip.h"
#endif

#include "vm/jit/asmpart.h"
#include "vm/jit/codegen.inc.h"

/******************************************************************************/
/* DEBUG HELPERS                                                              */
/******************************************************************************/

/*#define LOADER_VERBOSE*/

#ifndef NDEBUG
#define LOADER_DEBUG
#endif

#ifdef LOADER_DEBUG
#define LOADER_ASSERT(cond)  assert(cond)
#else
#define LOADER_ASSERT(cond)
#endif

#undef JOWENN_DEBUG
#undef JOWENN_DEBUG1
#undef JOWENN_DEBUG2

#ifdef LOADER_VERBOSE
static int loader_recursion = 0;
#define LOADER_INDENT(str)   do { int i; for(i=0;i<loader_recursion*4;++i) {str[i]=' ';} str[i]=0;} while (0)
#define LOADER_INC()  loader_recursion++
#define LOADER_DEC()  loader_recursion--
#else
#define LOADER_INC()
#define LOADER_DEC()
#endif


/********************************************************************
   list of classpath entries (either filesystem directories or 
   ZIP/JAR archives
********************************************************************/

classpath_info *classpath_entries = NULL;


/* loader_init *****************************************************************

   Initializes all lists and loads all classes required for the system
   or the compiler.

*******************************************************************************/
 
bool loader_init(u1 *stackbottom)
{
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	classpath_info *cpi;

	/* Initialize the monitor pointer for zip/jar file locking.               */

	for (cpi = classpath_entries; cpi != NULL; cpi = cpi->next) {
		if (cpi->type == CLASSPATH_ARCHIVE)
			initObjectLock(&cpi->header);
	}
#endif

	/* load some important classes */

	if (!(class_java_lang_Object = load_class_bootstrap(utf_java_lang_Object)))
		return false;

	if (!(class_java_lang_String = load_class_bootstrap(utf_java_lang_String)))
		return false;

	if (!(class_java_lang_Cloneable =
		  load_class_bootstrap(utf_java_lang_Cloneable)))
		return false;

	if (!(class_java_io_Serializable =
		  load_class_bootstrap(utf_java_io_Serializable)))
		return false;


	/* load classes for wrapping primitive types */

	if (!(class_java_lang_Void = load_class_bootstrap(utf_java_lang_Void)))
		return false;

	if (!(class_java_lang_Boolean =
		  load_class_bootstrap(utf_java_lang_Boolean)))
		return false;

	if (!(class_java_lang_Byte = load_class_bootstrap(utf_java_lang_Byte)))
		return false;

	if (!(class_java_lang_Character =
		  load_class_bootstrap(utf_java_lang_Character)))
		return false;

	if (!(class_java_lang_Short = load_class_bootstrap(utf_java_lang_Short)))
		return false;

	if (!(class_java_lang_Integer =
		  load_class_bootstrap(utf_java_lang_Integer)))
		return false;

	if (!(class_java_lang_Long = load_class_bootstrap(utf_java_lang_Long)))
		return false;

	if (!(class_java_lang_Float = load_class_bootstrap(utf_java_lang_Float)))
		return false;

	if (!(class_java_lang_Double = load_class_bootstrap(utf_java_lang_Double)))
		return false;


	/* load some other important classes */

	if (!(class_java_lang_Class = load_class_bootstrap(utf_java_lang_Class)))
		return false;

	if (!(class_java_lang_ClassLoader =
		  load_class_bootstrap(utf_java_lang_ClassLoader)))
		return false;

	if (!(class_java_lang_SecurityManager =
		  load_class_bootstrap(utf_java_lang_SecurityManager)))
		return false;

	if (!(class_java_lang_System = load_class_bootstrap(utf_java_lang_System)))
		return false;

	if (!(class_java_lang_ThreadGroup =
		  load_class_bootstrap(utf_java_lang_ThreadGroup)))
		return false;


	/* some classes which may be used more often */

	if (!(class_java_lang_StackTraceElement =
		  load_class_bootstrap(utf_java_lang_StackTraceElement)))
		return false;

	if (!(class_java_lang_reflect_Constructor =
		  load_class_bootstrap(utf_java_lang_reflect_Constructor)))
		return false;

	if (!(class_java_lang_reflect_Field =
		  load_class_bootstrap(utf_java_lang_reflect_Field)))
		return false;

	if (!(class_java_lang_reflect_Method =
		  load_class_bootstrap(utf_java_lang_reflect_Method)))
		return false;

	if (!(class_java_security_PrivilegedAction =
		  load_class_bootstrap(utf_new_char("java/security/PrivilegedAction"))))
		return false;

	if (!(class_java_util_Vector = load_class_bootstrap(utf_java_util_Vector)))
		return false;

	if (!(arrayclass_java_lang_Object =
		  load_class_bootstrap(utf_new_char("[Ljava/lang/Object;"))))
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


/* loader_load_all_classes *****************************************************

   Loads all classes specified in the BOOTCLASSPATH.

*******************************************************************************/

void loader_load_all_classes(void)
{
	classpath_info *cpi;
	classinfo      *c;

	for (cpi = classpath_entries; cpi != 0; cpi = cpi->next) {
#if defined(USE_ZLIB)
		if (cpi->type == CLASSPATH_ARCHIVE) {
			cacao_entry_s *ce;
			unz_s *s;

			s = (unz_s *) cpi->uf;
			ce = s->cacao_dir_list;
				
			while (ce) {
				/* check for .properties files */

				if (!strstr(ce->name->text, ".properties"))
					c = load_class_bootstrap(ce->name);

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
		if (!found) {
			dolog("Warning: Can not open class file '%s'", filename);

			if (strcmp(filename, "org/mortbay/util/MultiException.class") == 0) {
				static int i = 0;
				i++;
				if (i == 3)
					assert(0);
			}
		}

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
	fprintflags(stdout,f);
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


/* load_constantpool ***********************************************************

   Loads the constantpool of a class, the entries are transformed into
   a simpler format by resolving references (a detailed overview of
   the compact structures can be found in global.h).

*******************************************************************************/

static bool load_constantpool(classbuffer *cb, descriptor_pool *descpool)
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
			nfc = DNEW(forward_class);

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
			nfs = DNEW(forward_string);
				
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
			nfn = DNEW(forward_nameandtype);
				
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
			nff = DNEW(forward_fieldmethint);
			
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
				assert(0);
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

		if (opt_verify && !is_valid_name_utf(name)) {
			*exceptionptr = 
				new_classformaterror(c, "Class reference with invalid name");
			return false;
		}

		/* add all class references to the descriptor_pool */

		if (!descriptor_pool_add_class(descpool, name))
			return false;

		cptags[forward_classes->thisindex] = CONSTANT_Class;

		if (opt_eager) {
			classinfo *tc;

			if (!(tc = load_class_bootstrap(name)))
				return false;

			/* link the class later, because we cannot link the class currently
			   loading */
			list_addfirst(&unlinkedclasses, tc);
		}

		/* the classref is created later */
		cpinfos[forward_classes->thisindex] = name;

		nfc = forward_classes;
		forward_classes = forward_classes->next;
	}

	while (forward_strings) {
		utf *text =
			class_getconstant(c, forward_strings->string_index, CONSTANT_Utf8);

		/* resolve utf-string */
		cptags[forward_strings->thisindex] = CONSTANT_String;
		cpinfos[forward_strings->thisindex] = text;
		
		nfs = forward_strings;
		forward_strings = forward_strings->next;
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

		/* add all descriptors in {Field,Method}ref to the descriptor_pool */

		if (!descriptor_pool_add(descpool, nat->descriptor, NULL))
			return false;

		/* the classref is created later */

		fmi->classref = (constant_classref *) (size_t) forward_fieldmethints->class_index;
		fmi->name = nat->name;
		fmi->descriptor = nat->descriptor;

		cptags[forward_fieldmethints->thisindex] = forward_fieldmethints->tag;
		cpinfos[forward_fieldmethints->thisindex] = fmi;
	
		nff = forward_fieldmethints;
		forward_fieldmethints = forward_fieldmethints->next;
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

static bool load_field(classbuffer *cb, fieldinfo *f, descriptor_pool *descpool)
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
	f->parseddesc = NULL;

	if (!descriptor_pool_add(descpool, u, NULL))
		return false;

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

static bool load_method(classbuffer *cb, methodinfo *m, descriptor_pool *descpool)
{
	classinfo *c;
	int argcount;
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
	m->parseddesc = NULL;

	if (!descriptor_pool_add(descpool, u, &argcount))
		return false;

	if (opt_verify) {
		if (!is_valid_name_utf(m->name)) {
			log_text("Method with invalid name");
			assert(0);
		}

		if (m->name->text[0] == '<' &&
			m->name != utf_init && m->name != utf_clinit) {
			log_text("Method with invalid special name");
			assert(0);
		}
	}
	
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
								ACC_NATIVE | ACC_ABSTRACT)) {
					log_text("Instance initialization method has invalid flags set");
					assert(0);
				}
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
					m->exceptiontable[j].catchtype.any = NULL;

				} else {
					/* the classref is created later */
					if (!(m->exceptiontable[j].catchtype.any =
						  (utf*)class_getconstant(c, idx, CONSTANT_Class)))
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

			m->thrownexceptions = MNEW(classref_or_classinfo, m->thrownexceptionscount);

			for (j = 0; j < m->thrownexceptionscount; j++) {
				/* the classref is created later */
				if (!((m->thrownexceptions)[j].any =
					  (utf*) class_getconstant(c, suck_u2(cb), CONSTANT_Class)))
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

				info->inner_class.ref =
					innerclass_getconstant(c, suck_u2(cb), CONSTANT_Class);
				info->outer_class.ref =
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


/* load_class_from_sysloader ***************************************************

   Load the class with the given name using the system class loader

   IN:
       name.............the classname

   RETURN VALUE:
       the loaded class

*******************************************************************************/

classinfo *load_class_from_sysloader(utf *name)
{
	methodinfo        *m;
	java_objectheader *cl;
	classinfo         *r;

#ifdef LOADER_VERBOSE
	char logtext[MAXLOGTEXT];
	LOADER_INDENT(logtext);
	sprintf(logtext+strlen(logtext),"load_class_from_sysloader(");
	utf_sprint(logtext+strlen(logtext),name);strcat(logtext,")");
	log_text(logtext);
#endif

	LOADER_ASSERT(class_java_lang_Object);
	LOADER_ASSERT(class_java_lang_ClassLoader);
	LOADER_ASSERT(class_java_lang_ClassLoader->linked);
	
	m = class_resolveclassmethod(class_java_lang_ClassLoader,
								 utf_getSystemClassLoader,
								 utf_void__java_lang_ClassLoader,
								 class_java_lang_Object,
								 false);

	if (!m)
		return false;

	cl = (java_objectheader *) asm_calljavafunction(m, NULL, NULL, NULL, NULL);
	if (!cl)
		return false;

	LOADER_INC();
	r = load_class_from_classloader(name, cl);
	LOADER_DEC();

	return r;
}

/* load_class_from_classloader *************************************************

   Load the class with the given name using the given user-defined class loader.

   IN:
       name.............the classname
	   cl...............user-defined class loader
	   
   RETURN VALUE:
       the loaded class

*******************************************************************************/

classinfo *load_class_from_classloader(utf *name, java_objectheader *cl)
{
	classinfo *r;

#ifdef LOADER_VERBOSE
	char logtext[MAXLOGTEXT];
	LOADER_INDENT(logtext);
	strcat(logtext,"load_class_from_classloader(");
	utf_sprint(logtext+strlen(logtext),name);sprintf(logtext+strlen(logtext),",%p,",(void*)cl);
	if (!cl) strcat(logtext,"<bootstrap>");
	else if (cl->vftbl && cl->vftbl->class) utf_sprint(logtext+strlen(logtext),cl->vftbl->class->name);
	else strcat(logtext,"<unknown class>");
	strcat(logtext,")");
	log_text(logtext);
#endif

	LOADER_ASSERT(name);

	/* lookup if this class has already been loaded */

	r = classcache_lookup(cl, name);

#ifdef LOADER_VERBOSE
	if (*result)
		dolog("        cached -> %p",(void*)(*result));
#endif

	if (r)
		return r;

	/* if other class loader than bootstrap, call it */

	if (cl) {
		methodinfo *lc;

		/* handle array classes */
		if (name->text[0] == '[') {
			char      *utf_ptr;
			s4         len;
			classinfo *comp;
			utf       *u;

			utf_ptr = name->text + 1;
			len = name->blength - 1;

			switch (*utf_ptr) {
			case 'L':
				utf_ptr++;
				len -= 2;
				/* FALLTHROUGH */
			case '[':
				/* load the component class */
				u = utf_new(utf_ptr, len);
				LOADER_INC();
				if (!(comp = load_class_from_classloader(u, cl))) {
					LOADER_DEC();
					return false;
				}
				LOADER_DEC();
				/* create the array class */
				r = class_array_of(comp, false);
				return r;
				break;
			default:
				/* primitive array classes are loaded by the bootstrap loader */
				LOADER_INC();
				r = load_class_bootstrap(name);
				LOADER_DEC();
				return r;
			}
		}
		
		LOADER_ASSERT(class_java_lang_Object);

		lc = class_resolveclassmethod(cl->vftbl->class,
									  utf_loadClass,
									  utf_java_lang_String__java_lang_Class,
									  class_java_lang_Object,
									  true);

		if (!lc)
			return false; /* exception */

		LOADER_INC();
		r = (classinfo *) asm_calljavafunction(lc,
											   cl,
											   javastring_new_slash_to_dot(name),
											   NULL, NULL);
		LOADER_DEC();

		/* store this class in the loaded class cache */

		if (r && !classcache_store(cl, r)) {
			r->loaded = false;
			class_free(r);
			r = NULL; /* exception */
		}

		return r;
	} 

	LOADER_INC();
	r = load_class_bootstrap(name);
	LOADER_DEC();

	return r;
}


/* load_class_bootstrap ********************************************************
	
   Load the class with the given name using the bootstrap class loader.

   IN:
       name.............the classname

   OUT:

   RETURN VALUE:
       loaded classinfo

   SYNCHRONIZATION:
       load_class_bootstrap is synchronized. It can be treated as an
	   atomic operation.

*******************************************************************************/

classinfo *load_class_bootstrap(utf *name)
{
	classbuffer *cb;
	classinfo   *c;
	classinfo   *r;
#ifdef LOADER_VERBOSE
	char logtext[MAXLOGTEXT];
#endif

	/* for debugging */

	LOADER_ASSERT(name);
	LOADER_INC();

	/* synchronize */

	LOADER_LOCK();

	/* lookup if this class has already been loaded */

	if ((r = classcache_lookup(NULL, name)))
		goto success;

	/* check if this class has already been defined */

	if ((r = classcache_lookup_defined(NULL, name)))
		goto success;

#ifdef LOADER_VERBOSE
	LOADER_INDENT(logtext);
	strcat(logtext,"load_class_bootstrap(");
	utf_sprint(logtext+strlen(logtext),name);strcat(logtext,")");
	log_text(logtext);
#endif

	/* create the classinfo */

	c = class_create_classinfo(name);
	
	/* handle array classes */

	if (name->text[0] == '[') {
		if (!load_newly_created_array(c, NULL))
			goto return_exception;
		LOADER_ASSERT(c->loaded);
		r = c;
		goto success;
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
		/* this normally means, the classpath was not set properly */

		if (name == utf_java_lang_Object)
			throw_cacao_exception_exit(string_java_lang_NoClassDefFoundError,
									   "java/lang/Object");

		*exceptionptr =
			new_exception_utfmessage(string_java_lang_NoClassDefFoundError,
									 name);
		goto return_exception;
	}
	
	/* load the class from the buffer */

	r = load_class_from_classbuffer(cb);

	/* free memory */

	suck_stop(cb);

#if defined(STATISTICS)
	/* measure time */

	if (getloadingtime)
		loadingtime_stop();

	if (getcompilingtime)
		compilingtime_start();
#endif

	if (!r) {
		/* the class could not be loaded, free the classinfo struct */

		class_free(c);
	}

	/* store this class in the loaded class cache    */
	/* this step also checks the loading constraints */

	if (r && !classcache_store(NULL, c)) {
		class_free(c);
		r = NULL; /* exception */
	}

	if (!r)
		goto return_exception;

success:
	LOADER_UNLOCK();
	LOADER_DEC();

	return r;

return_exception:
	LOADER_UNLOCK();
	LOADER_DEC();

	return NULL;
}


/* load_class_from_classbuffer *************************************************
	
   Loads everything interesting about a class from the class file. The
   'classinfo' structure must have been allocated previously.

   The super class and the interfaces implemented by this class need
   not be loaded. The link is set later by the function 'class_link'.

   The loaded class is removed from the list 'unloadedclasses' and
   added to the list 'unlinkedclasses'.
	
   SYNCHRONIZATION:
       This function is NOT synchronized!
   
*******************************************************************************/

classinfo *load_class_from_classbuffer(classbuffer *cb)
{
	classinfo *c;
	utf *name;
	utf *supername;
	u4 i,j;
	u4 ma, mi;
	s4 dumpsize;
	descriptor_pool *descpool;
#if defined(STATISTICS)
	u4 classrefsize;
	u4 descsize;
#endif
#ifdef LOADER_VERBOSE
	char logtext[MAXLOGTEXT];
#endif

	/* get the classbuffer's class */

	c = cb->class;

	/* maybe the class is already loaded */

	if (c->loaded)
		return c;

#ifdef LOADER_VERBOSE
	LOADER_INDENT(logtext);
	strcat(logtext,"load_class_from_classbuffer(");
	utf_sprint(logtext+strlen(logtext),c->name);strcat(logtext,")");
	log_text(logtext);
	LOADER_INC();
#endif

#if defined(STATISTICS)
	if (opt_stat)
		count_class_loads++;
#endif

	/* output for debugging purposes */

	if (loadverbose)
		log_message_class("Loading class: ", c);
	
	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* class is somewhat loaded */

	c->loaded = true;

	if (!check_classbuffer_size(cb, 4 + 2 + 2))
		goto return_exception;

	/* check signature */

	if (suck_u4(cb) != MAGIC) {
		*exceptionptr = new_classformaterror(c, "Bad magic number");

		goto return_exception;
	}

	/* check version */

	mi = suck_u2(cb);
	ma = suck_u2(cb);

	if (!(ma < MAJOR_VERSION || (ma == MAJOR_VERSION && mi <= MINOR_VERSION))) {
		*exceptionptr =
			new_unsupportedclassversionerror(c,
											 "Unsupported major.minor version %d.%d",
											 ma, mi);

		goto return_exception;
	}

	/* create a new descriptor pool */

	descpool = descriptor_pool_new(c);

	/* load the constant pool */

	if (!load_constantpool(cb, descpool))
		goto return_exception;

	/*JOWENN*/
	c->erroneous_state = 0;
	c->initializing_thread = 0;	
	/*JOWENN*/
	c->classUsed = NOTUSED; /* not used initially CO-RT */
	c->impldBy = NULL;

	/* ACC flags */

	if (!check_classbuffer_size(cb, 2))
		goto return_exception;

	c->flags = suck_u2(cb);

	/* check ACC flags consistency */

	if (c->flags & ACC_INTERFACE) {
		if (!(c->flags & ACC_ABSTRACT)) {
			/* We work around this because interfaces in JDK 1.1 are
			 * not declared abstract. */

			c->flags |= ACC_ABSTRACT;
		}

		if (c->flags & ACC_FINAL) {
			*exceptionptr =
				new_classformaterror(c,
									 "Illegal class modifiers: 0x%X", c->flags);

			goto return_exception;
		}

		if (c->flags & ACC_SUPER) {
			c->flags &= ~ACC_SUPER; /* kjc seems to set this on interfaces */
		}
	}

	if ((c->flags & (ACC_ABSTRACT | ACC_FINAL)) == (ACC_ABSTRACT | ACC_FINAL)) {
		*exceptionptr =
			new_classformaterror(c, "Illegal class modifiers: 0x%X", c->flags);

		goto return_exception;
	}

	if (!check_classbuffer_size(cb, 2 + 2))
		goto return_exception;

	/* this class */

	i = suck_u2(cb);
	if (!(name = (utf *) class_getconstant(c, i, CONSTANT_Class)))
		goto return_exception;

	if (c->name == utf_not_named_yet) {
		/* we finally have a name for this class */
		c->name = name;
		class_set_packagename(c);

	} else if (name != c->name) {
		char *msg;
		s4    msglen;

		msglen = utf_strlen(c->name) + strlen(" (wrong name: ") +
			utf_strlen(name) + strlen(")") + strlen("0");

		msg = MNEW(char, msglen);

		utf_sprint(msg, c->name);
		strcat(msg, " (wrong name: ");
		utf_strcat(msg, name);
		strcat(msg, ")");

		*exceptionptr =
			new_exception_message(string_java_lang_NoClassDefFoundError, msg);

		MFREE(msg, char, msglen);

		goto return_exception;
	}
	
	/* retrieve superclass */

	c->super.any = NULL;
	if ((i = suck_u2(cb))) {
		if (!(supername = (utf *) class_getconstant(c, i, CONSTANT_Class)))
			goto return_exception;

		/* java.lang.Object may not have a super class. */

		if (c->name == utf_java_lang_Object) {
			*exceptionptr =
				new_exception_message(string_java_lang_ClassFormatError,
									  "java.lang.Object with superclass");

			goto return_exception;
		}

		/* Interfaces must have java.lang.Object as super class. */

		if ((c->flags & ACC_INTERFACE) &&
			supername != utf_java_lang_Object) {
			*exceptionptr =
				new_exception_message(string_java_lang_ClassFormatError,
									  "Interfaces must have java.lang.Object as superclass");

			goto return_exception;
		}

	} else {
		supername = NULL;

		/* This is only allowed for java.lang.Object. */

		if (c->name != utf_java_lang_Object) {
			*exceptionptr = new_classformaterror(c, "Bad superclass index");

			goto return_exception;
		}
	}
			 
	/* retrieve interfaces */

	if (!check_classbuffer_size(cb, 2))
		goto return_exception;

	c->interfacescount = suck_u2(cb);

	if (!check_classbuffer_size(cb, 2 * c->interfacescount))
		goto return_exception;

	c->interfaces = MNEW(classref_or_classinfo, c->interfacescount);
	for (i = 0; i < c->interfacescount; i++) {
		/* the classrefs are created later */
		if (!(c->interfaces[i].any = (utf *) class_getconstant(c, suck_u2(cb), CONSTANT_Class)))
			goto return_exception;
	}

	/* load fields */
	if (!check_classbuffer_size(cb, 2))
		goto return_exception;

	c->fieldscount = suck_u2(cb);
	c->fields = GCNEW(fieldinfo, c->fieldscount);
/*  	c->fields = MNEW(fieldinfo, c->fieldscount); */
	for (i = 0; i < c->fieldscount; i++) {
		if (!load_field(cb, &(c->fields[i]),descpool))
			goto return_exception;
	}

	/* load methods */
	if (!check_classbuffer_size(cb, 2))
		goto return_exception;

	c->methodscount = suck_u2(cb);
/*  	c->methods = GCNEW(methodinfo, c->methodscount); */
	c->methods = MNEW(methodinfo, c->methodscount);
	for (i = 0; i < c->methodscount; i++) {
		if (!load_method(cb, &(c->methods[i]),descpool))
			goto return_exception;
	}

	/* create the class reference table */

	c->classrefs =
		descriptor_pool_create_classrefs(descpool, &(c->classrefcount));

	/* allocate space for the parsed descriptors */

	descriptor_pool_alloc_parsed_descriptors(descpool);
	c->parseddescs =
		descriptor_pool_get_parsed_descriptors(descpool, &(c->parseddescsize));

#if defined(STATISTICS)
	if (opt_stat) {
		descriptor_pool_get_sizes(descpool, &classrefsize, &descsize);
		count_classref_len += classrefsize;
		count_parsed_desc_len += descsize;
	}
#endif

	/* put the classrefs in the constant pool */
	for (i = 0; i < c->cpcount; i++) {
		if (c->cptags[i] == CONSTANT_Class) {
			utf *name = (utf *) c->cpinfos[i];
			c->cpinfos[i] = descriptor_pool_lookup_classref(descpool, name);
		}
	}

	/* set the super class reference */

	if (supername) {
		c->super.ref = descriptor_pool_lookup_classref(descpool, supername);
		if (!c->super.ref)
			goto return_exception;
	}

	/* set the super interfaces references */

	for (i = 0; i < c->interfacescount; i++) {
		c->interfaces[i].ref =
			descriptor_pool_lookup_classref(descpool,
											(utf *) c->interfaces[i].any);
		if (!c->interfaces[i].ref)
			goto return_exception;
	}

	/* parse field descriptors */

	for (i = 0; i < c->fieldscount; i++) {
		c->fields[i].parseddesc =
			descriptor_pool_parse_field_descriptor(descpool,
												   c->fields[i].descriptor);
		if (!c->fields[i].parseddesc)
			goto return_exception;
	}

	/* parse method descriptors */

	for (i = 0; i < c->methodscount; i++) {
		methodinfo *m = &c->methods[i];
		m->parseddesc =
			descriptor_pool_parse_method_descriptor(descpool, m->descriptor,
													m->flags);
		if (!m->parseddesc)
			goto return_exception;

		for (j = 0; j < m->exceptiontablelength; j++) {
			if (!m->exceptiontable[j].catchtype.any)
				continue;
			if ((m->exceptiontable[j].catchtype.ref =
				 descriptor_pool_lookup_classref(descpool,
						(utf *) m->exceptiontable[j].catchtype.any)) == NULL)
				goto return_exception;
		}

		for (j = 0; j < m->thrownexceptionscount; j++) {
			if (!m->thrownexceptions[j].any)
				continue;
			if ((m->thrownexceptions[j].ref = descriptor_pool_lookup_classref(descpool,
						(utf *) m->thrownexceptions[j].any)) == NULL)
				goto return_exception;
		}
	}

	/* parse the loaded descriptors */

	for (i = 0; i < c->cpcount; i++) {
		constant_FMIref *fmi;
		s4               index;
		
		switch (c->cptags[i]) {
		case CONSTANT_Fieldref:
			fmi = (constant_FMIref *) c->cpinfos[i];
			fmi->parseddesc.fd =
				descriptor_pool_parse_field_descriptor(descpool,
													   fmi->descriptor);
			if (!fmi->parseddesc.fd)
				goto return_exception;
			index = (int) (size_t) fmi->classref;
			fmi->classref =
				(constant_classref *) class_getconstant(c, index,
														CONSTANT_Class);
			if (!fmi->classref)
				goto return_exception;
			break;
		case CONSTANT_Methodref:
		case CONSTANT_InterfaceMethodref:
			fmi = (constant_FMIref *) c->cpinfos[i];
			fmi->parseddesc.md =
				descriptor_pool_parse_method_descriptor(descpool,
														fmi->descriptor,
														ACC_UNDEF);
			if (!fmi->parseddesc.md)
				goto return_exception;
			index = (int) (size_t) fmi->classref;
			fmi->classref =
				(constant_classref *) class_getconstant(c, index,
														CONSTANT_Class);
			if (!fmi->classref)
				goto return_exception;
			break;
		}
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

						goto return_exception;
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

						goto return_exception;
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
		goto return_exception;

	if (!load_attributes(cb, suck_u2(cb)))
		goto return_exception;

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
			goto return_exception;
		}
	}
#endif

	/* release dump area */

	dump_release(dumpsize);
	
	if (loadverbose)
		log_message_class("Loading done class: ", c);

	LOADER_DEC();
	return c;

return_exception:
	/* release dump area */

	dump_release(dumpsize);

	/* an exception has been thrown */

	LOADER_DEC();
	return NULL;
}


/* load_newly_created_array ****************************************************

   Load a newly created array class.

   Note:
   This is an internal function. Do not use it unless you know exactly
   what you are doing!

   Use one of the load_class_... functions for general array class loading.

*******************************************************************************/

bool load_newly_created_array(classinfo *c, java_objectheader *loader)
{
	classinfo         *comp = NULL;
	methodinfo        *clone;
	methoddesc        *clonedesc;
	constant_classref *classrefs;
	s4                 namelen;
	java_objectheader *definingloader = NULL;
	utf               *u;

#ifdef LOADER_VERBOSE
	char logtext[MAXLOGTEXT];
	LOADER_INDENT(logtext);
	strcat(logtext,"load_newly_created_array(");utf_sprint_classname(logtext+strlen(logtext),c->name);
	sprintf(logtext+strlen(logtext),") loader=%p",loader);
	log_text(logtext);
#endif

	/* Check array class name */

	namelen = c->name->blength;
	if (namelen < 2 || c->name->text[0] != '[') {
		*exceptionptr = new_internalerror("Invalid array class name");
		return false;
	}

	/* Check the component type */

	switch (c->name->text[1]) {
	case '[':
		/* c is an array of arrays. We have to create the component class. */

		u = utf_new_intern(c->name->text + 1, namelen - 1);
		LOADER_INC();
		if (!(comp = load_class_from_classloader(u, loader))) {
			LOADER_DEC();
			return false;
		}
		LOADER_DEC();
		LOADER_ASSERT(comp->loaded);
		if (opt_eager)
			if (!link_class(comp))
				return false;
		definingloader = comp->classloader;
		break;

	case 'L':
		/* c is an array of objects. */
		if (namelen < 4 || c->name->text[namelen - 1] != ';') {
			*exceptionptr = new_internalerror("Invalid array class name");
			return false;
		}

		u = utf_new_intern(c->name->text + 2, namelen - 3);

		LOADER_INC();
		if (!(comp = load_class_from_classloader(u, loader))) {
			LOADER_DEC();
			return false;
		}
		LOADER_DEC();
		LOADER_ASSERT(comp->loaded);
		if (opt_eager)
			if (!link_class(comp))
				return false;
		definingloader = comp->classloader;
		break;
	}

	LOADER_ASSERT(class_java_lang_Object);
	LOADER_ASSERT(class_java_lang_Cloneable);
	LOADER_ASSERT(class_java_io_Serializable);

	/* Setup the array class */

	c->super.cls = class_java_lang_Object;
	c->flags = ACC_PUBLIC | ACC_FINAL | ACC_ABSTRACT;

    c->interfacescount = 2;
    c->interfaces = MNEW(classref_or_classinfo, 2);

	if (opt_eager) {
		classinfo *tc;

		tc = class_java_lang_Cloneable;
		LOADER_ASSERT(tc->loaded);
		list_addfirst(&unlinkedclasses, tc);
		c->interfaces[0].cls = tc;

		tc = class_java_io_Serializable;
		LOADER_ASSERT(tc->loaded);
		list_addfirst(&unlinkedclasses, tc);
		c->interfaces[1].cls = tc;

	} else {
		c->interfaces[0].cls = class_java_lang_Cloneable;
		c->interfaces[1].cls = class_java_io_Serializable;
	}

	c->methodscount = 1;
	c->methods = MNEW(methodinfo, c->methodscount);

	classrefs = MNEW(constant_classref, 1);
	CLASSREF_INIT(classrefs[0], c, utf_java_lang_Object);

	clonedesc = NEW(methoddesc);
	clonedesc->returntype.type = TYPE_ADDRESS;
	clonedesc->returntype.classref = classrefs;
	clonedesc->returntype.arraydim = 0;
	clonedesc->paramcount = 0;
	clonedesc->paramslots = 0;

	/* parse the descriptor to get the register allocation */

	if (!descriptor_params_from_paramtypes(clonedesc, ACC_NONE))
		return false;

	clone = c->methods;
	MSET(clone, 0, methodinfo, 1);
	clone->flags = ACC_PUBLIC;
	clone->name = utf_clone;
	clone->descriptor = utf_void__java_lang_Object;
	clone->parseddesc = clonedesc;
	clone->class = c;
	clone->stubroutine =
		codegen_createnativestub((functionptr) &builtin_clone_array, clone);
	clone->monoPoly = MONO;

	/* XXX: field: length? */

	/* array classes are not loaded from class files */

	c->loaded = true;
	c->parseddescs = (u1*) clonedesc;
	c->parseddescsize = sizeof(methodinfo);
	c->classrefs = classrefs;
	c->classrefcount = 1;
	c->classloader = definingloader;

	/* insert class into the loaded class cache */

	if (!classcache_store(loader, c))
		return false;

	return true;
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

	log_text("Can not find field given in CONSTANT_Fieldref");
	assert(0);

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
		fi = class_resolvefield_int(c->interfaces[i].cls, name, desc);
		if (fi)
			return fi;
	}

	/* try superclass */

	if (c->super.cls)
		return class_resolvefield_int(c->super.cls, name, desc);

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
	s4          i;

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

		log_text("Method not found");
		assert(0);
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
				char *desc_end = UTF_END(desc);         
				char *meth_end = UTF_END(meth_descr);   
				char ch;

				/* compare argument types */
				while (desc_utf_ptr < desc_end && meth_utf_ptr < meth_end) {

					if ((ch = *desc_utf_ptr++) != (*meth_utf_ptr++))
						break; /* no match */

					if (ch == ')')
						return &(c->methods[i]); /* all parameter types equal */
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
		c = c->super.cls;
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

		c = c->super.cls;
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
	s4          i;
	
	m = class_findmethod(c, name, desc);

	if (m)
		return m;

	/* try the superinterfaces */

	for (i = 0; i < c->interfacescount; i++) {
		m = class_resolveinterfacemethod_intern(c->interfaces[i].cls,
												name, desc);

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

   If the method cannot be resolved the return value is NULL. If
   EXCEPT is true *exceptionptr is set, too.

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

		cls = cls->super.cls;
	}

	/* try the superinterfaces */

	for (i = 0; i < c->interfacescount; i++) {
		mi = class_resolveinterfacemethod_intern(c->interfaces[i].cls,
												 name, desc);

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
		sub = sub->super.cls;
	}
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
			utf_display(((constant_classref*)e)->name);
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
				utf_display(fmi->classref->name);
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
			log_text("Invalid type of ConstantPool-Entry");
			assert(0);
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
				utf_display ( ((constant_classref*)e) -> name );
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
					utf_display ( fmi->classref->name );
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
				log_text("Invalid type of ConstantPool-Entry");
				assert(0);
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
	if (c->super.cls) {
		printf ("Super: "); utf_display (c->super.cls->name); printf ("\n");
		}
	printf ("Index: %d\n", c->index);
	
	printf ("interfaces:\n");	
	for (i=0; i < c-> interfacescount; i++) {
		printf ("   ");
		utf_display (c -> interfaces[i].cls -> name);
		printf (" (%d)\n", c->interfaces[i].cls -> index);
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


/* loader_close ****************************************************************

   Frees all resources.
	
*******************************************************************************/

void loader_close(void)
{
	/* empty */
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
