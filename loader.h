/* loader.h - class loader header

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

   $Id: loader.h 723 2003-12-08 19:51:32Z edwin $
*/


#ifndef _LOADER_H
#define _LOADER_H

#include <stdio.h>


/************************* program switches ***********************************/

extern bool loadverbose;         /* Print debug messages during loading */
extern bool linkverbose;
extern bool initverbose;         /* Log class initialization */ 
extern bool makeinitializations; /* Initialize classes automatically */

extern bool getloadingtime;
extern long int loadingtime;     /* CPU time for class loading */

extern list unlinkedclasses;     /* List containing all unlinked classes */
extern list linkedclasses;       /* List containing all linked classes */

#ifdef USE_THREADS
extern int blockInts;
#endif


/************************ prototypes ******************************************/

/* initialize laoder, load important systemclasses */
void loader_init();

void suck_init(char *cpath);

/* free resources */
void loader_close();

/* load a class and all referenced classes */
classinfo *loader_load(utf *topname);

/* initializes all loaded classes */
void loader_initclasses();

void loader_compute_subclasses();

/* retrieve constantpool element */
voidptr class_getconstant(classinfo *class, u4 pos, u4 ctype);

/* determine type of a constantpool element */
u4 class_constanttype(classinfo *class, u4 pos);

s4 class_findmethodIndex(classinfo *c, utf *name, utf *desc);

/* search class for a field */
fieldinfo *class_findfield(classinfo *c, utf *name, utf *desc);

/* search for a method with a specified name and descriptor */
methodinfo *class_findmethod(classinfo *c, utf *name, utf *desc);
methodinfo *class_resolvemethod(classinfo *c, utf *name, utf *dest);

/* search for a method with specified name and arguments (returntype ignored) */
methodinfo *class_findmethod_approx(classinfo *c, utf *name, utf *desc);
methodinfo *class_resolvemethod_approx(classinfo *c, utf *name, utf *dest);

bool class_issubclass(classinfo *sub, classinfo *super);

/* call initializer of class */
void class_init(classinfo *c);

void class_showconstanti(classinfo *c, int ii);

/* debug purposes */
void class_showmethods(classinfo *c);
void class_showconstantpool(classinfo *c);
void print_arraydescriptor(FILE *file, arraydescriptor *desc);

classinfo *loader_load(utf *topname);

/* set buffer for reading classdata */
void classload_buffer(u1 *buf, int len);

/* return the primitive class inidicated by the given signature character */
classinfo *class_primitive_from_sig(char sig);


/* return the class indicated by the given descriptor */
/* (see loader.c for documentation) */
#define CLASSLOAD_NEW           0      /* default */
#define CLASSLOAD_LOAD          0x0001
#define CLASSLOAD_SKIP          0x0002
#define CLASSLOAD_PANIC         0      /* default */
#define CLASSLOAD_NOPANIC       0x0010
#define CLASSLOAD_PRIMITIVE     0      /* default */
#define CLASSLOAD_NULLPRIMITIVE 0x0020
#define CLASSLOAD_VOID          0      /* default */
#define CLASSLOAD_NOVOID        0x0040
#define CLASSLOAD_NOCHECKEND    0      /* default */
#define CLASSLOAD_CHECKEND      0x1000

classinfo *class_from_descriptor(char *utf_ptr,char *end_ptr,char **next,int mode);
int type_from_descriptor(classinfo **cls,char *utf_ptr,char *end_ptr,char **next,int mode);

/* (used by class_new, don't use directly) */
void class_new_array(classinfo *c);

void class_link(classinfo *c);

void field_display(fieldinfo *f);

void method_display(methodinfo *m);

utf* clinit_desc();
utf* clinit_name();

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
