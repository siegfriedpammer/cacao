/* vm/options.h - define global options extern

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

   Authors: Christian Thalinger

   $Id: options.h 1752 2004-12-13 08:28:10Z twisti $

*/


#ifndef _OPTIONS_H
#define _OPTIONS_H


#include "vm/global.h"


/* reserved option numbers ****************************************************/

#define OPT_DONE       -1
#define OPT_ERROR       0
#define OPT_IGNORE      1


typedef struct opt_struct opt_struct;

struct opt_struct {
	char *name;
	bool  arg;
	int   value;
};


/* global variables ***********************************************************/

extern bool opt_verbose;
extern bool compileall;
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

/*#undef INAFTERMAIN*/  /*use to inline system methods before main is called*/
#define INAFTERMAIN T /*use to turn off inlining before main called */
extern bool useinliningm;
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
extern bool getcompilingtime;

extern int has_ext_instr_set;

extern bool opt_stat;
extern bool opt_verify;
extern bool opt_eager;

#ifdef LSRA
extern bool opt_lsra;
#endif

extern int opt_ind;
extern char *opt_arg;


/* function prototypes ********************************************************/

int get_opt(int argc, char **argv, opt_struct *opts);


#endif /* _OPTIONS_H */


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
