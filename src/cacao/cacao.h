/* main.c - main header, contains global variables

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

   Authors: Christian Thalinger

   $Id: cacao.h 1094 2004-05-27 15:52:27Z twisti $

*/


#ifndef _MAIN_H
#define _MAIN_H

#include "global.h"

/* global variables */

extern bool compileall;
extern bool verbose;
extern bool runverbose;
extern bool verboseexception;
extern bool collectverbose;

extern bool loadverbose;         /* Print debug messages during loading */
extern bool linkverbose;
extern bool initverbose;         /* Log class initialization */ 

extern bool opt_rt;
extern bool opt_xta;
extern bool opt_vta;

extern bool opt_liberalutf;      /* Don't check overlong UTF-8 sequences */

extern bool showmethods;
extern bool showconstantpool;
extern bool showutf;

extern bool compileverbose;
extern bool showstack;
extern bool showdisassemble;
extern bool showddatasegment;
extern bool showintermediate;

extern bool useinlining;
extern bool inlinevirtuals;
extern bool inlineexceptions;
extern bool inlineparamopt;
extern bool inlineoutsiders;

extern bool checkbounds;
extern bool checknull;
extern bool opt_noieee;
extern bool checksync;
extern bool opt_loops;

extern bool makeinitializations;

extern bool getloadingtime;
extern s8 loadingtime;

extern bool getcompilingtime;
extern s8 compilingtime;

extern int has_ext_instr_set;

extern bool opt_stat;

extern bool opt_eager;

extern char *mainstring;    /* class.method with main method */

#endif /* _MAIN_H */


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
