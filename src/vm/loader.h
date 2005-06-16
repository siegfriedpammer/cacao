/* src/vm/loader.h - class loader header

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

   $Id: loader.h 2725 2005-06-16 19:10:35Z edwin $
*/


#ifndef _LOADER_H
#define _LOADER_H

#include <stdio.h>


/* forward typedefs ***********************************************************/

typedef struct classbuffer classbuffer;
typedef struct classpath_info classpath_info;


#include "vm/global.h"
#include "vm/utf8.h"
#include "vm/references.h"
#include "vm/descriptor.h"
#include "vm/method.h"

#if defined(USE_ZLIB)
# include "vm/unzip.h"
#endif


/* signed suck defines ********************************************************/

#define suck_s8(a)    (s8) suck_u8((a))
#define suck_s2(a)    (s2) suck_u2((a))
#define suck_s4(a)    (s4) suck_u4((a))
#define suck_s1(a)    (s1) suck_u1((a))


/* constant pool entries *******************************************************

	All constant pool entries need a data structure which contain the entrys
	value. In some cases this structure exist already, in the remaining cases
	this structure must be generated:

		kind                      structure                     generated?
	----------------------------------------------------------------------
    CONSTANT_Class               classinfo                           no   XXX this will change
    CONSTANT_Fieldref            constant_FMIref                    yes
    CONSTANT_Methodref           constant_FMIref                    yes
    CONSTANT_InterfaceMethodref  constant_FMIref                    yes
    CONSTANT_String              unicode                             no
    CONSTANT_Integer             constant_integer                   yes
    CONSTANT_Float               constant_float                     yes
    CONSTANT_Long                constant_long                      yes
    CONSTANT_Double              constant_double                    yes
    CONSTANT_NameAndType         constant_nameandtype               yes
    CONSTANT_Utf8                unicode                             no
    CONSTANT_UNUSED              -

*******************************************************************************/

typedef struct {            /* Integer                                        */
	s4 value;
} constant_integer;

	
typedef struct {            /* Float                                          */
	float value;
} constant_float;


typedef struct {            /* Long                                           */
	s8 value;
} constant_long;
	

typedef struct {            /* Double                                         */
	double value;
} constant_double;


typedef struct {            /* NameAndType (Field or Method)                  */
	utf *name;              /* field/method name                              */
	utf *descriptor;        /* field/method type descriptor string            */
} constant_nameandtype;


/* classbuffer ****************************************************************/

struct classbuffer {
	classinfo *class;                   /* pointer to classinfo structure     */
	u1        *data;                    /* pointer to byte code               */
	s4         size;                    /* size of the byte code              */
	u1        *pos;                     /* current read position              */
};


/* classpath_info *************************************************************/

#define CLASSPATH_PATH       0
#define CLASSPATH_ARCHIVE    1

struct classpath_info {
#if defined(USE_THREADS)
	/* Required for monitor locking on zip/jar files. */
	java_objectheader  header;
#endif
	s4                 type;
	char              *path;
	s4                 pathlen;
#if defined(USE_ZLIB)
	unzFile            uf;
#endif
	classpath_info    *next;
};


/* export variables ***********************************************************/

#if defined(USE_THREADS)
extern int blockInts;
#endif

extern classpath_info *classpath_entries;


/* function prototypes ********************************************************/

/* initialize loader, load important systemclasses */
bool loader_init(u1 *stackbottom);

void suck_init(char *cpath);
void loader_load_all_classes(void);
void suck_stop(classbuffer *cb);

inline bool check_classbuffer_size(classbuffer *cb, s4 len);

inline u1 suck_u1(classbuffer *cb);
inline u2 suck_u2(classbuffer *cb);
inline u4 suck_u4(classbuffer *cb);

/* free resources */
void loader_close(void);

/* class loading functions */
classinfo *load_class_from_sysloader(utf *name);
classinfo *load_class_from_classloader(utf *name, java_objectheader *cl);
classinfo *load_class_bootstrap(utf *name);

/* (don't use the following directly) */
classinfo *load_class_from_classbuffer(classbuffer *cb);
classinfo *load_newly_created_array(classinfo *c,java_objectheader *loader);

/* search class for a field */
fieldinfo *class_findfield(classinfo *c, utf *name, utf *desc);
fieldinfo *class_resolvefield(classinfo *c, utf *name, utf *desc, classinfo *referer, bool except);

/* search for a method with a specified name and descriptor */
methodinfo *class_findmethod(classinfo *c, utf *name, utf *desc);
methodinfo *class_fetchmethod(classinfo *c, utf *name, utf *desc);
methodinfo *class_resolvemethod(classinfo *c, utf *name, utf *dest);
methodinfo *class_resolveclassmethod(classinfo *c, utf *name, utf *dest, classinfo *referer, bool except);
methodinfo *class_resolveinterfacemethod(classinfo *c, utf *name, utf *dest, classinfo *referer, bool except);

/* search for a method with specified name and arguments (returntype ignored) */
methodinfo *class_findmethod_approx(classinfo *c, utf *name, utf *desc);
methodinfo *class_resolvemethod_approx(classinfo *c, utf *name, utf *dest);

bool class_issubclass(classinfo *sub, classinfo *super);

/* debug purposes */
void class_showmethods(classinfo *c);
void class_showconstantpool(classinfo *c);

/* return the primitive class inidicated by the given signature character */
classinfo *class_primitive_from_sig(char sig);


/* debug helpers */
void fprintflags(FILE *fp, u2 f);
void printflags(u2 f);

#endif /* _LOADER_H */

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
