/* jit/alpha/types.h - machine specific definitions for alpha processor

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   Institut f. Computersprachen, TU Wien
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser, M. Probst,
   S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich,
   J. Wenninger

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
            Michael Gschwind

   $Id: types.h 557 2003-11-02 22:51:59Z twisti $

*/


#ifndef _TYPES_H
#define _TYPES_H

#define POINTERSIZE         8
#define WORDS_BIGENDIAN     0

#define SUPPORT_DIVISION    0
#define SUPPORT_LONG        1
#define SUPPORT_FLOAT       1
#define SUPPORT_DOUBLE      1

#define SUPPORT_LONG_ADD    1
#define SUPPORT_LONG_CMP    1
#define SUPPORT_LONG_LOG    1
#define SUPPORT_LONG_SHIFT  1
#define SUPPORT_LONG_MULDIV 1
#define SUPPORT_LONG_ICVT   1
#define SUPPORT_LONG_FCVT   1

#define CONDITIONAL_LOADCONST

#define U8_AVAILABLE        1


typedef signed char             s1;
typedef unsigned char           u1;
 
typedef signed short int        s2;
typedef unsigned short int      u2;

typedef signed int              s4;
typedef unsigned int            u4;

#if U8_AVAILABLE
typedef signed long int         s8;
typedef unsigned long int       u8; 
#else
typedef struct {u4 low, high;}  u8;
#define s8 u8
#endif

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


