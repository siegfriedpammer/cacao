/* options.c - contains global options

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

   $Id: options.c 1529 2004-11-17 17:19:14Z twisti $

*/


#include <string.h>
#include "options.h"
#include "types.h"


/* command line option */

bool verbose = false;
bool compileall = false;
bool runverbose = false;       /* trace all method invocation                */
bool verboseexception = false;
bool collectverbose = false;

bool loadverbose = false;
bool linkverbose = false;
bool initverbose = false;

bool opt_rt = false;           /* true if RTA parse should be used     RT-CO */
bool opt_xta = false;          /* true if XTA parse should be used    XTA-CO */
bool opt_vta = false;          /* true if VTA parse should be used    VTA-CO */

bool opt_liberalutf = false;   /* Don't check overlong UTF-8 sequences       */

bool showmethods = false;
bool showconstantpool = false;
bool showutf = false;

bool compileverbose =  false;  /* trace compiler actions                     */
bool showstack = false;
bool showdisassemble = false;  /* generate disassembler listing              */
bool showddatasegment = false; /* generate data segment listing              */
bool showintermediate = false; /* generate intermediate code listing         */

bool useinliningm = false;      /* use method inlining                        */
bool useinlining = false;      /* use method inlining                        */
bool inlinevirtuals = false;   /* inline unique virtual methods              */
bool inlineexceptions = false; /* inline methods, that contain excptions     */
bool inlineparamopt = false;   /* optimize parameter passing to inlined methods */
bool inlineoutsiders = false;  /* inline methods, that are not member of the invoker's class */

bool checkbounds = true;       /* check array bounds                         */
bool checknull = true;         /* check null pointers                        */
bool opt_noieee = false;       /* don't implement ieee compliant floats      */
bool checksync = true;         /* do synchronization                         */
bool opt_loops = false;        /* optimize array accesses in loops           */

bool makeinitializations = true;

bool getloadingtime = false;   /* to measure the runtime                     */
bool getcompilingtime = false; /* compute compile time                       */

int has_ext_instr_set = 0;     /* has instruction set extensions */

bool opt_stat = false;
bool opt_verify = true;        /* true if classfiles should be verified      */
bool opt_eager = false;


int opt_ind = 1;               /* index of processed arguments               */
char *opt_arg;                 /* this one exports the option argument       */


/* get_opt *********************************************************************

   DOCUMENT ME!!!

*******************************************************************************/

int get_opt(int argc, char **argv, opt_struct *opts)
{
	char *a;
	int i;
	
	if (opt_ind >= argc)
		return OPT_DONE;
	
	a = argv[opt_ind];
	if (a[0] != '-')
		return OPT_DONE;

	for (i = 0; opts[i].name; i++) {
		if (!opts[i].arg) {
			if (strcmp(a + 1, opts[i].name) == 0) { /* boolean option found */
				opt_ind++;
				return opts[i].value;
			}

		} else {
			if (strcmp(a + 1, opts[i].name) == 0) { /* parameter option found */
				opt_ind++;
				if (opt_ind < argc) {
					opt_arg = argv[opt_ind];
					opt_ind++;
					return opts[i].value;
				}
				return OPT_ERROR;

			} else {
				size_t l = strlen(opts[i].name);
				if (strlen(a + 1) > l) {
					if (memcmp(a + 1, opts[i].name, l) == 0) {
						opt_ind++;
						opt_arg = a + 1 + l;
						return opts[i].value;
					}
				}
			}
		}
	} /* end for */	

	return OPT_ERROR;
}


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
