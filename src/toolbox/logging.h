/* toolbox/logging.h - contains logging functions

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

   $Id: logging.h 1761 2004-12-15 15:48:43Z twisti $

*/


#ifndef _LOGGING_H
#define _LOGGING_H

#include <stdio.h>

#include "vm/global.h"


#define MAXLOGTEXT 500

/* function prototypes */

void log_init(char *fname);
void log_text(char *txt);
void log_plain(char *txt); /* same as log_text without "LOG: " and newline */
void log_flush();          /* fflush logfile */
void log_nl();             /* newline and fflush */

void log_cputime();

void log_message_class(const char *msg, classinfo *c);
void log_message_method(const char *msg, methodinfo *m);

void dolog(char *txt, ...);
void dolog_plain(char *txt, ...); /* same as dolog without "LOG: " and newline */
void error(char *txt, ...);

/* XXX this is just a quick hack on darwin */
#if !defined(__DARWIN__)
void panic(char *txt);
#endif

FILE *get_logfile(); /* return the current logfile */

#endif /* _LOGGING_H */


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
