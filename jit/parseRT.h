/* jit/parseRT.h - RTA parser header

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

   Authors: Carolyn Oates

   $Id: parseRT.h 662 2003-11-21 18:06:25Z jowenn $

*/


#ifndef _PARSERT_H
#define _PARSERT_H

#include "global.h"
#include <stdio.h>  
#include <string.h>
#include "jit.h"
#include "parse.h"
#include "loader.h"
#include "natcalls.h"

#include "parseRTprint.h"
#include "parseRTstats.h"
#include "sets.h"

#include "tables.h"
#include"toolbox/loging.h"
#include "toolbox/memory.h"

#include "types.h"

extern bool XTAOPTbypass;
extern bool XTAOPTbypass2;
extern bool XTAOPTbypass3;
extern int XTAdebug;
extern int XTAfld;

extern int pWhenMarked;
extern int pCallgraph;  /* 0 - dont print 1 - print at end from main                             */
                        /* 2 - print at end of RT parse call                                     */
                        /* 3- print after each method RT parse                                   */
extern int pClassHeir;  /* 0 - dont print 1 - print at end from main                             */
                        /* 2 - print at end of RT parse call  3-print after each method RT parse */
extern int pClassHeirStatsOnly;  /* usually 2 Print only the statistical summary info for class heirarchy     */

extern int pOpcodes;    /* 0 - don't print 1- print in parse RT 2- print in parse                */
                        /* 3 - print in both                                                     */
extern int pWhenMarked; /* 0 - don't print 1 - print when added to callgraph + when native parsed*/
                        /* 2 - print when marked+methods called                                  */
                        /* 3 - print when class/method looked at                                 */
extern int pStats;       


extern int methRT;
extern int methRTlast;
extern int methRTmax;
extern methodinfo **callgraph;
 
extern int methXTA;
extern int methXTAlast;
extern int methXTAmax;
extern methodinfo **XTAcallgraph;

extern void RT_jit_parse(methodinfo *m);

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
