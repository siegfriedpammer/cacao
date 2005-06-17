/* src/vm/jit/alpha/types.h - machine specific definitions for Alpha

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
            Andreas  Krall

   Changes: Christian Thalinger

   $Id: types.h 2731 2005-06-17 12:14:45Z twisti $

*/


#ifndef _TYPES_H
#define _TYPES_H

/* Define the sizes of the integer types used internally by cacao. */

typedef signed char             s1;
typedef unsigned char           u1;
 
typedef signed short int        s2;
typedef unsigned short int      u2;

typedef signed int              s4;
typedef unsigned int            u4;

typedef signed long int         s8;
typedef unsigned long int       u8;


/* Define the size of a function pointer used in function pointer casts. */

typedef u8                      ptrint;

#endif /* _TYPES_H */


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
