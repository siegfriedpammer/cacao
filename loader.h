/* loader.c - class loader header

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

   $Id: loader.h 557 2003-11-02 22:51:59Z twisti $

*/


#ifndef _LOADER_H
#define _LOADER_H

/************************* program switches ***********************************/

extern bool loadverbose;         /* Print debug messages during loading */
extern bool linkverbose;
extern bool initverbose;         /* Log class initialization */ 
extern bool makeinitializations; /* Initialize classes automatically */

extern bool getloadingtime;
extern long int loadingtime;     /* CPU time for class loading */

extern list unlinkedclasses;     /* List containing all unlinked classes */
extern list linkedclasses;       /* List containing all linked classes */


/************************ prototypes ******************************************/

/* initialize laoder, load important systemclasses */
void loader_init ();

void suck_init(char *cpath);

/* free resources */
void loader_close ();

/* load a class and all referenced classes */
classinfo *loader_load (utf *topname);

/* initializes all loaded classes */
void loader_initclasses ();

void loader_compute_subclasses ();

/* retrieve constantpool element */
voidptr class_getconstant (classinfo *class, u4 pos, u4 ctype);

/* determine type of a constantpool element */
u4 class_constanttype (classinfo *class, u4 pos);

/* search class for a field */
fieldinfo *class_findfield (classinfo *c, utf *name, utf *desc);

/* search for a method with a specified name and descriptor */
methodinfo *class_findmethod (classinfo *c, utf *name, utf *desc);
methodinfo *class_resolvemethod (classinfo *c, utf *name, utf *dest);

/* search for a method with specified name and arguments (returntype ignored) */
methodinfo *class_findmethod_approx (classinfo *c, utf *name, utf *desc);
methodinfo *class_resolvemethod_approx (classinfo *c, utf *name, utf *dest);

bool class_issubclass (classinfo *sub, classinfo *super);

/* call initializer of class */
void class_init (classinfo *c);

/* debug purposes */
void class_showconstantpool (classinfo *c);
void class_showmethods (classinfo *c);

classinfo *loader_load(utf *topname);

/* set buffer for reading classdata */
void classload_buffer(u1 *buf,int len);

/* create class representing specific arraytype */
classinfo *create_array_class(utf *u);

/* create the arraydescriptor for the arraytype specified by the utf-string */
constant_arraydescriptor * buildarraydescriptor(char *utf, u4 namelen);

extern void class_link (classinfo *c);

void field_display (fieldinfo *f);

void method_display(methodinfo *m);

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
