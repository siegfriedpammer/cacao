/* jit/parse.h - parser header

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

   Author: Christian Thalinger

   $Id: parse.h 557 2003-11-02 22:51:59Z twisti $

*/


#ifndef _PARSE_H
#define _PARSE_H

#include "global.h"

/* macros for byte code fetching ***********************************************

	fetch a byte code of given size from position p in code array jcode

*******************************************************************************/

#define code_get_u1(p)  jcode[p]
#define code_get_s1(p)  ((s1)jcode[p])
#define code_get_u2(p)  ((((u2)jcode[p]) << 8) + jcode[p + 1])
#define code_get_s2(p)  ((s2)((((u2)jcode[p]) << 8) + jcode[p + 1]))
#define code_get_u4(p)  ((((u4)jcode[p]) << 24) + (((u4)jcode[p + 1]) << 16) \
                        +(((u4)jcode[p + 2]) << 8) + jcode[p + 3])
#define code_get_s4(p)  ((s4)((((u4)jcode[p]) << 24) + (((u4)jcode[p + 1]) << 16) \
                             +(((u4)jcode[p + 2]) << 8) + jcode[p + 3]))


extern classinfo  *rt_class;
extern methodinfo *rt_method;
extern utf *rt_descriptor;
extern int rt_jcodelength;
extern u1  *rt_jcode;


/* function prototypes */
void compiler_addinitclass(classinfo *c);
classSetNode * descriptor2typesL(methodinfo *m);
void descriptor2types(methodinfo *m);
void parse();

#endif /* _PARSE_H */


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


