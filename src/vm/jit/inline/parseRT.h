/* jit/parseRT.h - RTA parser header

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

   Authors: Carolyn Oates

   $Id: parseRT.h 1744 2004-12-09 10:17:12Z carolyn $

*/

#ifndef _PARSERT_H
#define _PARSERT_H

#include "vm/global.h"


extern FILE *rtMissed;  /* Methods missed during RTA parse of Main  */

typedef struct {
        listnode linkage;
        methodinfo *method;
        } rtaNode ;


extern int RT_jit_parse(methodinfo *m);

/**** Methods: called directly by cacao ***/
#define MAINCLASS mainstring
#define MAINMETH "main"
#define MAINDESC "([Ljava/lang/String;)V"

#define EXITCLASS "java/lang/System"
#define EXITMETH  "exit"
#define EXITDESC  "(I)V"

#if defined(USE_THREADS)
 #define THREADCLASS "java/lang/Thread"
 #define THREADMETH  "<init>"
 #define THREADDESC  "(Ljava/lang/VMThread;Ljava/lang/String;IZ)V"

 #define THREADGROUPCLASS "java/lang/ThreadGroup"
 #define THREADGROUPMETH  "addThread"
 #define THREADGROUPDESC  "(Ljava/lang/Thread;)V"
#endif


#endif /* _PARSERT_H */

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

